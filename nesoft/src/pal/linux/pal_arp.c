/* Copyright (C) 2002  All Rights Reserved. */

#include <pal.h>
#include <lib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <net/if_arp.h>
#include <errno.h>
#include <ctype.h>


/* Define if linux/sysctl.h contains NET_IPV4_CONF_ARP_IGNORE definition */
#undef HAVE_SYSCTL_ARP_IGNORE

#ifdef HAVE_SYSCTL_ARP_IGNORE
#include <linux/sysctl.h>
#endif /* HAVE_SYSCTL_ARP_IGNORE */

/* Delete an entry from the ARP cache. */
int
pal_unset_arp (struct pal_in4_addr *ip)
{
  int sockfd = 0;
  struct arpreq req;
  struct sockaddr_in *in;
  int err,ret= -1;

  if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
    return ret;
  memset((char *) &req, 0, sizeof(req));

  if (ip == NULL)
    {
      close (sockfd);
      return ret;
    }

  req.arp_ha.sa_family = ARPHRD_ETHER;

  in = (struct sockaddr_in *) &req.arp_pa;
  pal_mem_cpy(&in->sin_addr, ip, sizeof (struct pal_in4_addr));
  in->sin_family = AF_INET;

  req.arp_flags = ATF_PERM;
  err = -1;

  /* Call the kernel. */
  if ((err = ioctl(sockfd, SIOCDARP, &req) < 0))
    {
      if (errno == ENXIO)
        {
          close (sockfd);
          return ret;
        }
    }
  close (sockfd);
  return 0;
}

/* Set an entry in the ARP cache. */
int
pal_set_arp ( struct pal_in4_addr *ip, char *hwa)
{
  int sockfd = 0;
  struct arpreq req;
  struct sockaddr_in *in;
  char mac[PAL_HW_ALEN];
  int flags,ret = -1;

  pal_strcpy(mac,hwa);
   /* Convert to network order */
  *(unsigned short *)&mac[0] = pal_hton16(*(unsigned short *)&hwa[0]);
  *(unsigned short *)&mac[2] = pal_hton16(*(unsigned short *)&hwa[2]);
  *(unsigned short *)&mac[4] = pal_hton16(*(unsigned short *)&hwa[4]);

  if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
    {
      return ret;
    }
  pal_mem_set((char *) &req, 0, sizeof(req));

  in = (struct sockaddr_in *) &req.arp_pa;
  pal_mem_cpy(&in->sin_addr, ip, sizeof (struct pal_in4_addr));
  in->sin_family = AF_INET;
  req.arp_ha.sa_family = ARPHRD_ETHER;
  pal_mem_cpy(&req.arp_ha.sa_data, mac, PAL_HW_ALEN) ;
  flags = ATF_PERM | ATF_COM;
  req.arp_flags = flags;

  /* Call the kernel. */
  if (ioctl(sockfd, SIOCSARP, &req) < 0)
    {
      close (sockfd);
      return ret;
    }
  close (sockfd);
  return 0;
}

/* Function to delete all the Dynamic entries from ARP Table*/
int  pal_unset_arp_all ()
{
  char ip[PAL_IPADDR_LEN];
  char mask[PAL_HWADDR_LEN];
  char line[PAL_BUFF];
  char dev[PAL_IF_SIZ];
  int type, arp_flags;
  int ret= -1;
  struct pal_in4_addr addr;
  FILE *fp;
  int num,i,r;

  /* Open the PROCps kernel table. */
  if ((fp = fopen(_PAL_PATH_PROCNET_ARP, "r")) == NULL)
    {
      return ret;
    }

  /* Bypass header -- read until newline */
  if (fgets(line, sizeof(line), fp) != (char *) NULL)
    {
      strcpy(mask, "-");
      strcpy(dev, "-");
    }

  /* Read the ARP cache entries. */
  for (; fgets(line, sizeof(line), fp);)
     {
       num = sscanf(line, "%s 0x%x 0x%x\n",
                    ip, &type, &arp_flags);

       if (num < 3)
         break;

       /* Skip the Static entries */
       if ((arp_flags & ATF_PERM))
         continue;

       i = pal_inet_pton (AF_INET, ip, &addr);
       r = pal_unset_arp(&addr);
       if (r != 0)
          {
            (void) fclose(fp);
            return ret;
          }
     }

  (void) fclose(fp);
  return 0;
}

int pal_display_arp (int (*func)(struct show_arp *s ,void * t), void *temp)
{
  struct pal_in4_addr addr;
  char ip[PAL_IPADDR_LEN];
  char hwa[PAL_HWADDR_LEN];
  char mask[PAL_BUFF];
  char line[PAL_BUFF];
  char dev[PAL_IF_SIZ];
  int type, arp_flags;
  FILE *fp;
  int val, num, ret = -1;
  struct show_arp arp;

  /* Open the PROCps kernel table. */
  if ((fp = fopen(_PAL_PATH_PROCNET_ARP, "r")) == NULL)
    return ret;

  /* Bypass header -- read until newline */
  if (fgets(line, sizeof(line), fp) != (char *) NULL)
    {
      strcpy(mask, "-");
      strcpy(dev, "-");
      /* Read the ARP cache entries. */
      for (; fgets(line, sizeof(line), fp);)
         {
           num = sscanf(line, "%s 0x%x 0x%x %100s %100s %100s\n",
                        ip, &type, &arp_flags, hwa, mask, dev);
           if (num < 4)
             break;

           if (!(arp_flags & ATF_COM))
             continue;
           else
             {
              if (!(arp_flags & ATF_PERM))
                arp.flags = PAL_ARP_FLAG_DYNAMIC;
              else
                arp.flags = PAL_ARP_FLAG_STATIC ;
             }

           val = pal_inet_pton (AF_INET, ip, &addr);
           arp.ipaddr = addr;
           pal_strcpy(arp.hwaddr, hwa);
           pal_strcpy(arp.dev, dev);

           if (func(&arp, temp) < 0)
             {
               (void) fclose(fp);
               return ret;
             }
         }
    }

  (void) fclose(fp);
  return  0;
}

#ifdef HAVE_IPV6
s_int32_t
pal_ipv6_reachabletime_set (u_int8_t *ifname, u_int32_t ifindex, s_int32_t reachable_time_value)
{
  s_int32_t ret;
  FILE *fp1,*fp2;
  s_int32_t plen, ilen;
  char *proc_ipv6_reachable_time;
  char *proc_ipv6_reachable_time_ms;
  if (ifname == NULL)
    return -1;
  fp1 = fp2 = NULL;
  proc_ipv6_reachable_time_ms = proc_ipv6_reachable_time = NULL;
  plen = pal_strlen (PAL_PATH_NEIGH);
  ilen = pal_strlen (ifname);
  ret = -1;
  proc_ipv6_reachable_time_ms = XCALLOC (MTYPE_TMP,
                                       plen + pal_strlen (PAL_PATH_REACHABLE_TIME_MS) + ilen + 2); /* 2 for / and string terminator */
  if (NULL == proc_ipv6_reachable_time_ms)
    {
      ret = -1;
      goto ERR;
    }
  proc_ipv6_reachable_time = XCALLOC (MTYPE_TMP,
                                    plen + pal_strlen (PAL_PATH_REACHABLE_TIME) + ilen + 2);
  if (NULL == proc_ipv6_reachable_time)
    {
      ret = -1;
      goto ERR;
    }

  sprintf (proc_ipv6_reachable_time_ms, "%s%s/%s", PAL_PATH_NEIGH, ifname, PAL_PATH_REACHABLE_TIME_MS);
  sprintf (proc_ipv6_reachable_time, "%s%s/%s", PAL_PATH_NEIGH, ifname, PAL_PATH_REACHABLE_TIME);
  fp1 = fopen (proc_ipv6_reachable_time_ms, "w");
  if (NULL == fp1)
    {
      ret = -1;
      goto ERR;
    }
  fp2 = fopen (proc_ipv6_reachable_time, "w");
  if (fp2 == NULL)
    {
      ret = -1;
      goto ERR;
    }
  fprintf (fp1, "%d\n", reachable_time_value);
  fprintf (fp2, "%d\n",(reachable_time_value/1000));
  ret = 0;
 ERR:
  if (fp1)
    fclose (fp1);
  if (fp2)
    fclose (fp2);
  if (proc_ipv6_reachable_time)
    XFREE (MTYPE_TMP, proc_ipv6_reachable_time);
  if (proc_ipv6_reachable_time_ms)
    XFREE (MTYPE_TMP, proc_ipv6_reachable_time_ms);
  return (ret);
}

s_int32_t
pal_ipv6_curhoplimit_set(u_int8_t *ifname, u_int32_t ifindex, u_int8_t cur_hoplimit)
{
  FILE *fp = NULL;
  s_int32_t ret;
  char *proc_ipv6_curhoplimit;
  proc_ipv6_curhoplimit = XCALLOC (MTYPE_TMP, pal_strlen (PAL_PATH_IPV6_CONF) +
                                  pal_strlen (PAL_PATH_HOPLIMIT) + pal_strlen (ifname) + 2);

  if (NULL == proc_ipv6_curhoplimit)
    {
      ret = -1;
      goto ERR;
    }
  sprintf(proc_ipv6_curhoplimit, "%s%s/%s", PAL_PATH_IPV6_CONF, ifname, PAL_PATH_HOPLIMIT);

  fp = fopen (proc_ipv6_curhoplimit, "w");
  if (NULL == fp)
    {
      ret = -1;
      goto ERR;
    }
  fprintf (fp, "%d\n", cur_hoplimit);
  ret = 0;
 ERR:
  if (fp)
    fclose (fp);
  if (proc_ipv6_curhoplimit)
    XFREE (MTYPE_TMP, proc_ipv6_curhoplimit);
  return (ret);
}
#endif /* HAVE_IPV6 */


