/* Copyright (C) 2002-2003   All Rights Reserved.  */

#include "pal.h"

#include "if.h"

#include "nsm/nsmd.h"
#include "nsm/rib/rib.h"
#include "nsm/nsm_api.h"

#include <endian.h>

#if __BYTE_ORDER == __LITTLE_ENDIAN
#include <linux/byteorder/little_endian.h>
#elif __BYTE_ORDER == __BIG_ENDIAN
#include <linux/byteorder/big_endian.h>
#endif

#include <linux/if_tunnel.h>


char *
pal_tunnel_mode2ifname (int mode)
{
  switch (mode)
    {
#ifdef HAVE_TUNNEL_IPV4
    case TUNNEL_MODE_APNP:
      return "tunl0";
    case TUNNEL_MODE_GREIP:
      return "gre0";
#endif /* HAVE_TUNNEL_IPV4 */
#ifdef HAVE_TUNNEL_IPV6
    case TUNNEL_MODE_IPV6IP:
    case TUNNEL_MODE_IPV6IP_6TO4:
    case TUNNEL_MODE_IPV6IP_ISATAP:
      return "sit0";
#endif /* HAVE_TUNNEL_IPV6 */
    default:
      return NULL;
    }
}

u_char
pal_tunnel_mode2proto (u_char mode)
{
  switch (mode)
    {
#ifdef HAVE_TUNNEL_IPV4
    case TUNNEL_MODE_APNP:
      return PAL_IPPROTO_APNP;
    case TUNNEL_MODE_GREIP:
      return PAL_IPPROTO_GRE;
#endif /* HAVE_TUNNEL_IPV4 */
#ifdef HAVE_TUNNEL_IPV6
    case TUNNEL_MODE_IPV6IP:
    case TUNNEL_MODE_IPV6IP_6TO4:
    case TUNNEL_MODE_IPV6IP_ISATAP:
      return PAL_IPPROTO_IPV6;
#endif /* HAVE_TUNNEL_IPV6 */
    default:
      return 0;
    }
}

u_char
pal_tunnel_proto2mode (struct ip_tunnel_parm *p)
{
  switch (p->iph.protocol)
    {
#ifdef HAVE_TUNNEL_IPV4
    case PAL_IPPROTO_APNP:
      return TUNNEL_MODE_APNP;
    case PAL_IPPROTO_GRE:
      return TUNNEL_MODE_GREIP;
#endif /* HAVE_TUNNEL_IPV4 */
#ifdef HAVE_TUNNEL_IPV6
    case PAL_IPPROTO_IPV6:
      if (p->iph.saddr && p->iph.daddr)
        return TUNNEL_MODE_IPV6IP;
      if (p->i_flags == SIT_ISATAP)
        return TUNNEL_MODE_IPV6IP_ISATAP;
      else
        return TUNNEL_MODE_IPV6IP_6TO4;
#endif /* HAVE_TUNNEL_IPV6 */
    default:
      return TUNNEL_MODE_NONE;
    }
}

int
pal_tunnel_if2parm (struct ip_tunnel_parm *p, struct interface *ifp)
{
  u_char proto;
  struct tunnel_if *tif = ifp->tunnel_if;

  if ((proto = pal_tunnel_mode2proto (tif->mode)) == 0)
    return -1;

  memset (p, 0, sizeof (struct ip_tunnel_parm));
  strcpy (p->name, if_kernel_name (ifp));

  p->iph.version = 4;
  p->iph.ihl = 5;
  p->iph.tos = tif->tos;
  p->iph.ttl = tif->ttl;

#ifdef HAVE_TUNNEL_IPV4
  /* Linux kernel automaticaly enables path MTU discovery when TTL is
     explicitly specified for GRE and IP-IP. */
  if (tif->mode == TUNNEL_MODE_GREIP || tif->mode == TUNNEL_MODE_APNP)
    if (tif->ttl)
      SET_FLAG (tif->config, TUNNEL_CONFIG_PMTUD);
#endif /* HAVE_TUNNEL_IPV4 */

  if (CHECK_FLAG (tif->config, TUNNEL_CONFIG_PMTUD))
    p->iph.frag_off = htons (IP_DF);

  p->iph.protocol = proto;

#ifdef HAVE_TUNNEL_IPV4
  if (tif->mode == TUNNEL_MODE_GREIP)
    if (CHECK_FLAG (tif->config, TUNNEL_CONFIG_CHECKSUM))
      {
        SET_FLAG (p->i_flags, GRE_CSUM);
        SET_FLAG (p->o_flags, GRE_CSUM);
      }
#endif /* HAVE_TUNNEL_IPV4 */

#ifdef HAVE_TUNNEL_IPV6
  if (tif->mode == TUNNEL_MODE_IPV6IP_ISATAP)
    p->i_flags = SIT_ISATAP;
#endif /* HAVE_TUNNEL_IPV6 */

  p->iph.saddr = tif->saddr4.s_addr;

#ifdef HAVE_TUNNEL_IPV6
  if (tif->mode == TUNNEL_MODE_IPV6IP_ISATAP)
    p->i_key = tif->daddr4.s_addr;
  else
#endif /* HAVE_TUNNEL_IPV6 */
    p->iph.daddr = tif->daddr4.s_addr;

  return 0;
}

int
pal_tunnel_parm2if (struct interface *ifp, struct ip_tunnel_parm *p)
{
  u_char mode;
  struct tunnel_if *tif = ifp->tunnel_if;

  if ((mode = pal_tunnel_proto2mode (p)) == TUNNEL_MODE_NONE)
    return -1;

  memset (tif, 0, sizeof (struct tunnel_if));

  tif->mode = mode;
  tif->tos = p->iph.tos;
  tif->ttl = p->iph.ttl;

  if (CHECK_FLAG (htons (IP_DF), p->iph.frag_off))
    SET_FLAG (tif->config, TUNNEL_CONFIG_PMTUD);

#ifdef HAVE_TUNNEL_IPV4
  if (tif->mode == TUNNEL_MODE_GREIP)
    if (CHECK_FLAG (p->i_flags, GRE_CSUM) && CHECK_FLAG (p->o_flags, GRE_CSUM))
      SET_FLAG (tif->config, TUNNEL_CONFIG_CHECKSUM);
#endif /* HAVE_TUNNEL_IPV4 */

  tif->saddr4.s_addr = p->iph.saddr;

#ifdef HAVE_TUNNEL_IPV6
  if (tif->mode == TUNNEL_MODE_IPV6IP_ISATAP)
    tif->daddr4.s_addr = p->i_key;
  else
#endif /* HAVE_TUNNEL_IPV6 */
    tif->daddr4.s_addr = p->iph.daddr;

  return 0;
}

int
pal_tunnel_cmp (struct interface *ifp, struct ip_tunnel_parm *p)
{
  struct tunnel_if *tif = ifp->tunnel_if;

  if (tif->mode != TUNNEL_MODE_IPV6IP)
    if (tif->mode != pal_tunnel_proto2mode (p))
      return -1;

  if (tif->tos != p->iph.tos)
    return -1;

  if (tif->ttl != p->iph.ttl)
    return -1;

  if (CHECK_FLAG (tif->config, TUNNEL_CONFIG_PMTUD))
    {
      if (! CHECK_FLAG (htons (IP_DF), p->iph.frag_off))
        return -1;
    }
  else
    {
      if (CHECK_FLAG (htons (IP_DF), p->iph.frag_off))
        return -1;
    }

#ifdef HAVE_TUNNEL_IPV4
  if (tif->mode == TUNNEL_MODE_GREIP)
    {
      if (CHECK_FLAG (tif->config, TUNNEL_CONFIG_CHECKSUM))
        {
          if (! CHECK_FLAG (p->i_flags, GRE_CSUM)
              || ! CHECK_FLAG (p->o_flags, GRE_CSUM))
            return -1;
        }
      else
        {
          if (CHECK_FLAG (p->i_flags, GRE_CSUM)
              || CHECK_FLAG (p->o_flags, GRE_CSUM))
            return -1;
        }
    }
#endif /* HAVE_TUNNEL_IPV4 */

  if (tif->saddr4.s_addr != p->iph.saddr)
    return -1;

#ifdef HAVE_TUNNEL_IPV6
  if (tif->mode == TUNNEL_MODE_IPV6IP_ISATAP)
    {
      if (tif->daddr4.s_addr != p->i_key)
        return -1;
    }
  else
#endif /* HAVE_TUNNEL_IPV6 */
    if (tif->daddr4.s_addr != p->iph.daddr)
      return -1;

  return 0;
}

int
pal_tunnel_ioctl (int cmd, struct ifreq *ifr)
{
  int fd;
  int ret;

  /* Open socket. */
  fd = socket (AF_INET, SOCK_DGRAM, 0);

  /* If failure return */
  if (fd < 0)
    return fd;

  /* Do ioctl. */
  ret = ioctl (fd, cmd, ifr);

  /* Close socket. */
  close (fd);

  return ret;
}

int
pal_tunnel_ioctl_up (struct ifreq *ifr)
{
  int fd;
  int ret;

  /* Open socket. */
  fd = socket (AF_INET, SOCK_DGRAM, 0);

  /* If failure return */
  if (fd < 0)
    return fd;

  /* Up the tunnel interface. */
  if ((ret = ioctl (fd, SIOCGIFFLAGS, ifr)) == 0)
    {
      ifr->ifr_flags |= IFF_UP;

      ret = ioctl (fd, SIOCSIFFLAGS, ifr);
    }

  /* Close socket. */
  close (fd);

  return ret;
}

int
pal_tunnel_ioctl_down (struct ifreq *ifr)
{
  int fd;
  int ret;

  /* Open socket. */
  fd = socket (AF_INET, SOCK_DGRAM, 0);

  /* If failure return */
  if (fd < 0)
    return fd;

  /* Down the tunnel interface. */
  if ((ret = ioctl (fd, SIOCGIFFLAGS, ifr)) == 0)
    {
      ifr->ifr_flags &= ~IFF_UP;

      ret = ioctl (fd, SIOCSIFFLAGS, ifr);
    }

  /* Close socket. */
  close (fd);

  return ret;
}


/* PAL tunnel APIs. */
int
pal_tunnel_add (struct interface *ifp)
{
  int ret;
  char *ifname;
  struct ifreq ifr;
  struct ip_tunnel_parm p;
  struct tunnel_if *tif = ifp->tunnel_if;

  if ((ifname = pal_tunnel_mode2ifname (tif->mode)) == NULL)
    return -1;

  if (pal_tunnel_if2parm (&p, ifp) < 0)
    return -1;

  /* Fill in the request. */
  memset (&ifr, 0, sizeof (struct ifreq));
  strcpy (ifr.ifr_name, ifname);
  ifr.ifr_ifru.ifru_data = (void *) &p;

  ret = pal_tunnel_ioctl (SIOCADDTUNNEL, &ifr);

  if (ret)
    return -1;
  else
    return 0;
}

int
pal_tunnel_del (struct interface *ifp)
{
  int ret;
  struct ifreq ifr;

  memset (&ifr, 0, sizeof (struct ifreq));
  strcpy (ifr.ifr_name, if_kernel_name (ifp));

  ret = pal_tunnel_ioctl (SIOCDELTUNNEL, &ifr);

  if (ret)
    return -1;
  else
    return 0;
}

int
pal_tunnel_update (struct interface *ifp)
{
  int ret;
  struct ifreq ifr;
  struct ip_tunnel_parm p;
#ifdef HAVE_TUNNEL_IPV4
  struct tunnel_if *tif = ifp->tunnel_if;

  /* Linux kernel doesn't allow to specify TTL without path MTU discovery
     for GRE and IP-IP interface. */
  if (tif->mode == TUNNEL_MODE_GREIP || tif->mode == TUNNEL_MODE_APNP)
    if (tif->ttl && ! CHECK_FLAG (tif->config, TUNNEL_CONFIG_PMTUD))
      return NSM_API_SET_ERR_TUNNEL_IF_UPDATE_FAIL_TTL;
#endif /* HAVE_TUNNEL_IPV4 */

  if (pal_tunnel_if2parm (&p, ifp) < 0)
    return NSM_API_SET_ERR_TUNNEL_IF_UPDATE_FAIL;

  /* Fill in the request. */
  memset (&ifr, 0, sizeof (struct ifreq));
  strcpy (ifr.ifr_name, if_kernel_name (ifp));
  ifr.ifr_ifru.ifru_data = (void *) &p;

  ret = pal_tunnel_ioctl (SIOCCHGTUNNEL, &ifr);

  if (ret || pal_tunnel_cmp (ifp, &p))
    return NSM_API_SET_ERR_TUNNEL_IF_UPDATE_FAIL;
  else
    return 0;
}

int
pal_tunnel_up (struct interface *ifp)
{
  int ret;
  struct ifreq ifr;

  memset (&ifr, 0, sizeof (struct ifreq));
  strcpy (ifr.ifr_name, if_kernel_name (ifp));

  ret = pal_tunnel_ioctl_up (&ifr);

  if (ret)
    return -1;
  else
    return 0;
}

int
pal_tunnel_down (struct interface *ifp)
{
  int ret;
  struct ifreq ifr;

  memset (&ifr, 0, sizeof (struct ifreq));
  strcpy (ifr.ifr_name, if_kernel_name (ifp));

  ret = pal_tunnel_ioctl_down (&ifr);

  if (ret)
    return -1;
  else
    return 0;
}

int
pal_tunnel_get (struct interface *ifp)
{
  int ret;
  struct ifreq ifr;
  struct ip_tunnel_parm p;

  memset (&ifr, 0, sizeof (struct ifreq));
  memset (&p, 0, sizeof (struct ip_tunnel_parm));
  strcpy (ifr.ifr_name, if_kernel_name (ifp));
  ifr.ifr_ifru.ifru_data = (void *) &p;

  ret = pal_tunnel_ioctl (SIOCGETTUNNEL, &ifr);

  if (ret)
    return -1;
  else if (pal_tunnel_parm2if (ifp, &p) < 0)
    return -1;
  else
    return 0;
}
