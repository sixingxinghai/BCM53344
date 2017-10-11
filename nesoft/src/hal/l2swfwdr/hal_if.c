/* Copyright (C) 2001-2003  All Rights Reserved. */

#include "pal.h"

#include "if.h"
#include "sockunion.h"
#include "log.h"
#include "prefix.h"
#include "hash.h"
#include "linklist.h"
#include "nsm_interface.h"
#include "linklist.h"
#include "if.h"

#include "nsmd.h"
#ifdef HAVE_L3
#include "nsm_connected.h"
#include "rib.h"
#include "nsm_rt.h"
#endif /* HAVE_L3 */
#include "hal_if.h"
#ifdef HAVE_GMPLS
#include "nsm/gmpls/nsm_gmpls.h"
#endif /* HAVE_GMPLS */

#define _PATH_PROC_NET_DEV        "/proc/net/dev"

extern int get_link_status_using_mii (char *ifname);

/* Return statistics data pointer. */
static char *
_interface_name_cut (char *buf, char **name)
{
  char *stat;

  /* Skip white space.  Line will include header spaces. */
  while (*buf == ' ')
    buf++;
  *name = buf;

  /* Cut interface name. */
  stat = strrchr (buf, ':');
  *stat++ = '\0';

  return stat;
}

/* Interface structure allocation by proc filesystem. */
int
hal_interface_list_proc ()
{
  FILE *fp;
  char buf[1024];
  struct interface *ifp;
  char *name;

  /* Open /proc/net/dev. */
  fp = fopen (_PATH_PROC_NET_DEV, "r");
  if (fp == NULL)
    {
      zlog_warn (NSM_ZG, "Can't open proc file %s: %s",
                 _PATH_PROC_NET_DEV, strerror (errno));
      return -1;
    }

  /* Drop header lines. */
  fgets (buf, 1024, fp);
  fgets (buf, 1024, fp);

  /* Only allocate interface structure.  Other jobs will be done in
     hal_if_ioctl.c. */
  while (fgets (buf, 1024, fp) != NULL)
    {
      _interface_name_cut (buf, &name);
      ifp = ifg_get_by_name (&NSM_ZG->ifg, name);
      if (ifp == NULL)
          zlog_warn (NSM_ZG, "Interface not found \n");
    }

  fclose (fp);

  return 0;
}

/* clear and set interface name string */
void
hal_ifreq_set_name (struct ifreq *ifreq,char  *ifname)
{
  strncpy (ifreq->ifr_name, ifname, IFNAMSIZ);
}

/* call ioctl system call */
int
hal_if_ioctl (int request, caddr_t buffer)
{
  int sock;
  int ret = 0;
  int err = 0;

  sock = socket (AF_INET, SOCK_DGRAM, 0);
  if (sock < 0)
    {
      perror ("socket");
      exit (1);
    }

  ret = ioctl (sock, request, buffer);
  if (ret < 0)
    {
      err = errno;
    }
  close (sock);
  
  if (ret < 0) 
    {
      errno = err;
      return ret;
    }
  return 0;
}

/* Interface looking up using infamous SIOCGIFCONF. */
int
interface_list_ioctl (int arbiter)
{
  int ret;
  int sock;
#define IFNUM_BASE 32
  int ifnum;
  struct ifreq *ifreq;
  struct ifconf ifconf;
  struct interface *ifp;
  int n;
  int update;

  /* Normally SIOCGIFCONF works with AF_INET socket. */
  sock = socket (AF_INET, SOCK_DGRAM, 0);
  if (sock < 0) 
    {
      zlog_warn (NSM_ZG, "Can't make AF_INET socket stream: %s", strerror (errno));
      return -1;
    }

  /* Set initial ifreq count.  This will be double when SIOCGIFCONF
     fail.  Solaris has SIOCGIFNUM. */
#ifdef SIOCGIFNUM
  ret = ioctl (sock, SIOCGIFNUM, &ifnum);
  if (ret < 0)
    ifnum = IFNUM_BASE;
  else
    ifnum++;
#else
  ifnum = IFNUM_BASE;
#endif /* SIOCGIFNUM */

  ifconf.ifc_buf = NULL;

  /* Loop until SIOCGIFCONF success. */
  for (;;) 
    {
      ifconf.ifc_len = sizeof (struct ifreq) * ifnum;
      ifconf.ifc_buf = XREALLOC (MTYPE_TMP, ifconf.ifc_buf, ifconf.ifc_len);

      ret = ioctl(sock, SIOCGIFCONF, &ifconf);
      if (ret < 0) 
        {
          zlog_warn (NSM_ZG, "SIOCGIFCONF: %s", strerror(errno));
          goto end;
        }
      /* When length is same as we prepared, assume it overflowed and
         try again */
      if (ifconf.ifc_len == sizeof (struct ifreq) * ifnum) 
        {
          ifnum += 10;
          continue;
        }
      /* Success. */
      break;
    }

  /* Allocate interface. */
  ifreq = ifconf.ifc_req;

  for (n = 0; n < ifconf.ifc_len; n += sizeof(struct ifreq))
    {
      update = 0;
      ifp = ifg_lookup_by_name (&NSM_ZG->ifg, ifreq->ifr_name);
      if (ifp == NULL)
        {
          ifp = ifg_get_by_name (&NSM_ZG->ifg, ifreq->ifr_name);
          update = 1;
        }

      if(arbiter)
        SET_FLAG (ifp->status, NSM_INTERFACE_ARBITER);

      if (update)
        nsm_if_add_update (ifp, (fib_id_t)FIB_ID_MAIN);
      ifreq++;
    }
 end:
  close (sock);
  /*XFREE (MTYPE_TMP, ifconf.ifc_buf);*/
  pal_mem_free (MTYPE_TMP, ifconf.ifc_buf);

  return ret;
}

/* Get interface's index by ioctl. */
result_t
hal_if_get_index(struct interface *ifp)
{
  static int if_fake_index = 1;

#ifdef HAVE_BROKEN_ALIASES
  /* Linux 2.2.X does not provide individual interface index for aliases. */
  ifp->ifindex = if_fake_index++;
  return RESULT_OK;
#else
#ifdef SIOCGIFINDEX
  int ret;
  struct ifreq ifreq;

  hal_ifreq_set_name (&ifreq, ifp->name);

  ret = hal_if_ioctl (SIOCGIFINDEX, (caddr_t) &ifreq);
  if (ret < 0)
    {
      /* Linux 2.0.X does not have interface index. */
      ifp->ifindex = if_fake_index++;
      return RESULT_OK;
    }

  /* OK we got interface index. */
#ifdef ifr_ifindex
  ifp->ifindex = ifreq.ifr_ifindex;
#else
  ifp->ifindex = ifreq.ifr_index;
#endif
  return RESULT_OK;

#else
  ifp->ifindex = if_fake_index++;
  return RESULT_OK;
#endif /* SIOCGIFINDEX */
#endif /* HAVE_BROKEN_ALIASES */
}

/*
 * get interface metric
 *   -- if value is not avaliable set -1
 */
result_t
hal_if_get_metric (char *ifname, unsigned int ifindex, int *metric)
{
#ifdef SIOCGIFMETRIC
  struct ifreq ifreq;

  hal_ifreq_set_name (&ifreq, ifname);

  if (hal_if_ioctl (SIOCGIFMETRIC, (caddr_t) &ifreq) < 0) 
    return -1;
  *metric = ifreq.ifr_metric;
  if (*metric == 0)
    *metric = 1;
#else /* SIOCGIFMETRIC */
  *metric = -1;
#endif /* SIOCGIFMETRIC */
  return RESULT_OK;
}

/* set interface MTU */
result_t
hal_if_set_mtu (char *ifname, unsigned int ifindex, int mtu)
{
  struct ifreq ifreq;

  hal_ifreq_set_name (&ifreq, ifname);

  ifreq.ifr_mtu = mtu;
  
#if defined(SIOCSIFMTU)
  if (hal_if_ioctl (SIOCSIFMTU, (caddr_t) & ifreq) < 0) 
    {
      zlog_info (NSM_ZG, "Can't set mtu by ioctl(SIOCSIFMTU)");
      return -1;
    }
#else
  zlog (NSM_ZG, NULL, ZLOG_INFO, "Can't set mtu on this system");
#endif

  return RESULT_OK;
}


/* get interface MTU */
result_t
hal_if_get_mtu (char *ifname, unsigned int ifindex, int *metric)
{
  struct ifreq ifreq;

  hal_ifreq_set_name (&ifreq, ifname);

#if defined(SIOCGIFMTU)
  if (hal_if_ioctl (SIOCGIFMTU, (caddr_t) & ifreq) < 0) 
    {
      zlog_info (NSM_ZG, "Can't lookup mtu by ioctl(SIOCGIFMTU)");
      *metric = -1;
      return -1;
    }

  *metric = ifreq.ifr_mtu;
#else
  zlog (NSM_ZG, NULL, ZLOG_INFO, "Can't lookup mtu on this system");
  *metric = -1;
#endif
  return 0;
}

/* Get interface bandwidth. */
result_t
hal_if_get_bw (char *ifname, unsigned int ifindex, u_int32_t*bandwidth)
{
  *bandwidth = 0;
  return 0;
}

int
hal_if_set_port_type (char *ifname, unsigned int ifindex,
                          enum hal_if_port_type type, unsigned int *retifindex)
{
  *retifindex = ifindex;
  return 0;  
}

int
hal_if_delete_done(char *ifname, u_int16_t ifindex)
{
  return 0;
}

int
hal_if_svi_create (char *ifname, unsigned int *ifindex)
{
  if (ifindex)
    *ifindex = 0;
  return 0;
}

int
hal_if_svi_delete (char *ifname, unsigned int ifindex)
{
  return 0;
}

#ifdef SIOCGIFHWADDR
int
hal_if_get_hwaddr (char *ifname, unsigned int ifindex, unsigned char *hwaddr,
               int *hwaddr_len)
{
  int ret;
  struct ifreq ifreq;
  int i;

  hal_ifreq_set_name (&ifreq, ifname);
  ifreq.ifr_addr.sa_family = AF_INET;

  /* Fetch Hardware address if available. */
  ret = hal_if_ioctl (SIOCGIFHWADDR, (caddr_t) &ifreq);
  if (ret < 0)
    *hwaddr_len = 0;
  else
    {
      memcpy (hwaddr, ifreq.ifr_hwaddr.sa_data, 6);

      for (i = 0; i < 6; i++)
        if (hwaddr[i] != 0)
          break;

      if (i == 6)
        *hwaddr_len = 0;
      else
        *hwaddr_len = 6;
    }
  return 0;
}
#endif /* SIOCGIFHWADDR */

#ifdef SIOCSIFHWADDR
int
hal_if_set_hwaddr (char *ifname, unsigned int ifindex, u_int8_t *hwaddr,
                   int hwlen)
{
  struct ifreq ifreq;
  memset (&ifreq, 0, sizeof (struct ifreq));
  hal_ifreq_set_name (&ifreq, ifname);
  ifreq.ifr_addr.sa_family = AF_LOCAL;
  memcpy (ifreq.ifr_hwaddr.sa_data, hwaddr, hwlen);
  
  /* Fetch Hardware address if available. */
  if ( hal_if_ioctl (SIOCSIFHWADDR, (caddr_t) &ifreq) < 0 )
    return -1;
  
  return 0;
}
#endif /* SIOCSIFHWADDR */

/* Fetch interface information via ioctl(). */
static void
interface_info_ioctl (int arbiter)
{
  struct interface *ifp;
  struct listnode *node;
  
  LIST_LOOP (NSM_ZG->ifg.if_list, ifp, node)
    {
#ifdef HAVE_GMPLS
      if (ifp->gifindex == 0)
        {
          ifp->gifindex = NSM_GMPLS_GIFINDEX_GET ();
        }
#endif /* HAVE_GMPLS */

      /* Add interface to database. */
      ifg_table_add (&NSM_ZG->ifg, ifp->ifindex, ifp);

#ifdef SIOCGIFHWADDR
      hal_if_get_hwaddr (ifp->name, ifp->ifindex, ifp->hw_addr, 
                         &(ifp->hw_addr_len));
#endif /* SIOCGIFHWADDR */
      hal_if_flags_get (ifp->name, ifp->ifindex, &(ifp->flags));
      hal_if_get_mtu (ifp->name, ifp->ifindex, &(ifp->mtu));
      hal_if_get_metric (ifp->name, ifp->ifindex, &(ifp->metric));
      hal_if_get_bw (ifp->name, ifp->ifindex, (u_int32_t *) &(ifp->bandwidth));

      /* Update interface information.  */
      nsm_if_add_update (ifp, (fib_id_t)FIB_ID_MAIN);
    }
}

/* Lookup all interface information. */
result_t
hal_if_scan(void)
{
  /* Linux can do both proc & ioctl, ioctl is the only way to get
     interface aliases in 2.2 series kernels. */
#ifdef HAVE_PROC_NET_DEV
  hal_interface_list_proc ();
#endif /* HAVE_PROC_NET_DEV */
  interface_list_ioctl (0);

  /* After listing is done, get index, address, flags and other
     interface's information. */
  interface_info_ioctl (0);

  return RESULT_OK;
}

void
hal_if_update  (void)
{
  /* Linux can do both proc & ioctl, ioctl is the only way to get
     interface aliases in 2.2 series kernels. */
#ifdef HAVE_PROC_NET_DEV
  hal_interface_list_proc ();
#endif /* HAVE_PROC_NET_DEV */
  interface_list_ioctl (1);
  
  /* After listing is done, get index, address, flags and other
     interface's information. */
  interface_info_ioctl (1);
}

/* get interface flags */
result_t
hal_if_flags_get(char *ifname, unsigned int ifindex, u_int32_t *flags)
{
  struct ifreq ifreq;
  int link_status = 0;

  hal_ifreq_set_name (&ifreq, ifname);

  if ( hal_if_ioctl (SIOCGIFFLAGS, (caddr_t) &ifreq) < 0 )
    return -1;

  *flags = ifreq.ifr_flags & 0x0000ffff;

#if defined (SIOCGMIIPHY) && defined (SIOCGMIIREG)
  link_status = get_link_status_using_mii (ifname);
  if (link_status == 1)
    *flags |= IFF_RUNNING;
  else if (link_status == 0)
    *flags &= ~IFF_RUNNING;
  else
    return link_status;
#endif /* SIOCGMIIPHY && SIOCGMIIREG */

  return RESULT_OK;
}

/* Set interface flags */
result_t
hal_if_flags_set (char *ifname, unsigned int ifindex, unsigned int flags)
{
  struct ifreq ifreq;
  unsigned int oldflags;
  
  hal_ifreq_set_name (&ifreq, ifname);

  if ( hal_if_flags_get(ifname, ifindex, &oldflags) < 0 )
    return -1;
  
  ifreq.ifr_flags |= oldflags;
  ifreq.ifr_flags |= flags;

  if ( hal_if_ioctl (SIOCSIFFLAGS, (caddr_t) &ifreq) < 0 )
    {
      zlog_info (NSM_ZG, "can't set interface flags");
      return -1;
    }
  return RESULT_OK;
}

/* Unset interface's flag. */
result_t
hal_if_flags_unset (char *ifname, unsigned int ifindex,
    unsigned int flags)
{
  struct ifreq ifreq;
  unsigned int oldflags;
  
  hal_ifreq_set_name (&ifreq, ifname);

  if ( hal_if_flags_get(ifname, ifindex, &oldflags) < 0 )
    return -1;
  
  ifreq.ifr_flags |= oldflags;
  ifreq.ifr_flags &= ~flags;

  if (hal_if_ioctl (SIOCSIFFLAGS, (caddr_t) &ifreq) < 0 )
    {
      zlog_info (NSM_ZG, "can't unset interface flags");
      return -1;
    }
  return RESULT_OK;
}

int
hal_if_get_counters(unsigned int ifindex, struct hal_if_counters *if_stats)
{
  return RESULT_NO_SUPPORT;
}

int
hal_vlan_if_get_counters(unsigned int ifindex,unsigned int vlan,
                         struct hal_vlan_if_counters *if_stats)
{
  return 0;
}

int
hal_if_stats_flush (u_int16_t ifindex)
{
  return 0;
}

int
hal_if_set_sw_reset ()
{
   return HAL_SUCCESS;
}

int
hal_pro_vlan_set_dtag_mode (unsigned int ifindex,unsigned short dtag_mode)
{
  return HAL_SUCCESS;
}

int
hal_if_set_port_egress (unsigned int ifindex, int egress_mode)
{
  return HAL_SUCCESS;
}

int
hal_if_set_cpu_default_vid (unsigned int ifindex, int vid)
{
  return HAL_SUCCESS;
}

int
hal_if_set_wayside_default_vid (unsigned int ifindex, int vid)
{
  return HAL_SUCCESS;
}

int
hal_if_set_learn_disable (unsigned int ifindex, int enable)
{
  return HAL_SUCCESS;
}

int
hal_if_get_learn_disable (unsigned int ifindex, int* enable)
{
  return HAL_SUCCESS;
}

int
hal_if_set_force_vlan (unsigned int ifindex, int vid)
{
  return HAL_SUCCESS;
}

int
hal_if_set_ether_type (unsigned int ifindex, u_int16_t etype)
{
  return HAL_SUCCESS;
}

int
hal_if_set_mdix (unsigned int ifindex, unsigned int mdix)
{
  return HAL_SUCCESS;
}

int
hal_if_set_preserve_ce_cos (unsigned int ifindex)
{
  return HAL_SUCCESS;
}


