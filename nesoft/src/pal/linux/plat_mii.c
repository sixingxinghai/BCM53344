/* Copyright (C) 2003  All Rights Reserved. */

#include "plat_incl.h"

#if defined (SIOCGMIIPHY) && defined (SIOCGMIIREG)
struct mii_data
{
  unsigned short phy_id;
  unsigned short reg_num;
  unsigned short val_in;
  unsigned short val_out; 
};

/* Basic Mode Status Register */
#define MII_BMSR                0x01

/* Link status. */
#define BMSR_LSTATUS            0x0004

/* Get link status using MII registers. 
   Returns 0: if link absent
           1: if link present
          -1: if MII not supported. 
*/
int 
get_link_status_using_mii (char *ifname)
{
  struct ifreq ifr;
  struct mii_data *mii = (struct mii_data *)&ifr.ifr_data;
  int bmsr;
  int sock;

  sock = socket(AF_INET, SOCK_DGRAM, 0);
  if (sock < 0)
    return -1;

  strncpy (ifr.ifr_name, ifname, IFNAMSIZ);

  if (ioctl(sock, SIOCGMIIPHY, &ifr) < 0)
    {
      close (sock);
      return -1;
    }

  /* Get link status. */
  mii->reg_num = MII_BMSR;
  if (ioctl(sock, SIOCGMIIREG, &ifr) < 0)
    {
      close (sock);
      return -1;
    }
  bmsr = mii->val_out;

  close (sock);

  if (bmsr & BMSR_LSTATUS)
    return 1;
  else
    return 0;
}

#endif /* SIOCGMIIPHY && SIOCGMIIREG */

