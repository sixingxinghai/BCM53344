/* Copyright (C) 2002-2003   All Rights Reserved. */

#include "pal.h"
#include "modbmap.h"

/*********************************************************************/
/* FILE       : modbmap.c                                            */
/* PURPOSE    : This file contains 'Module bitmap'                   */
/*              related function definitions.                        */
/* NAME-TAG   : 'modbmap_'                                           */
/*********************************************************************/

modbmap_t PM_EMPTY;
modbmap_t PM_NSM;
modbmap_t PM_MNT;
modbmap_t PM_RIP;
modbmap_t PM_RIPNG;
modbmap_t PM_OSPF;
modbmap_t PM_OSPF6;
modbmap_t PM_ISIS;
modbmap_t PM_BGP;
modbmap_t PM_LDP;
modbmap_t PM_RSVP;
modbmap_t PM_PDM;
modbmap_t PM_PIM;
modbmap_t PM_PIM6;
modbmap_t PM_DVMRP;
modbmap_t PM_AUTH;
modbmap_t PM_ONM;
modbmap_t PM_LACP;
modbmap_t PM_STP;
modbmap_t PM_RSTP;
modbmap_t PM_MSTP;
modbmap_t PM_IMI;
modbmap_t PM_IMISH;
modbmap_t PM_PIMPKTGEN;
modbmap_t PM_RMON;
modbmap_t PM_HSL;
modbmap_t PM_ELMI;
modbmap_t PM_VLOG;
modbmap_t PM_LMP;

modbmap_t PM_UCAST;
modbmap_t PM_MCAST;
modbmap_t PM_MCAST_VR;
modbmap_t PM_MPLS;
modbmap_t PM_L2;
modbmap_t PM_ALL;

modbmap_t PM_IF;
modbmap_t PM_CA;
modbmap_t PM_CC;
modbmap_t PM_TEL;
modbmap_t PM_IFDESC;
modbmap_t PM_TUN_IF;
modbmap_t PM_VR;
modbmap_t PM_VRF;
modbmap_t PM_VRRP;
modbmap_t PM_RMM;
modbmap_t PM_KEYCHAIN;
modbmap_t PM_ACCESS;
modbmap_t PM_PREFIX;
modbmap_t PM_DISTRIBUTE;
modbmap_t PM_RMAP;
modbmap_t PM_IFRMAP;
modbmap_t PM_LOG;
modbmap_t PM_HOSTNAME;
modbmap_t PM_NPF;
modbmap_t PM_IGMP;
modbmap_t PM_MLD;
modbmap_t PM_CAL;
modbmap_t PM_FM;
modbmap_t PM_SNMP_DBG;
modbmap_t PM_OAM;

/*
   Name: modbmap_id2bit

   Description:
   Initialize Module bitmap : allocate memory & sets the Module bitmap

   Parameters:
   bit_num - Protocol number

   Returns:
   module bitmap
*/
modbmap_t
modbmap_id2bit (u_int8_t bit_num)
{
  modbmap_t res_mbm = PM_EMPTY;

  MODBMAP_SET (res_mbm, bit_num);

  return (res_mbm);
}

/*
   Name: modbmap_vor

   Description:
   Performs logical-OR operation on "cnt" number of Module bitmaps.

   Parameters:
   *mbm1 - Address of Module bitmap
   cnt   - Count of the number of Module bitmaps sent as parameters

   Returns:
   Module bitmap
*/
modbmap_t
modbmap_vor (u_int8_t cnt, modbmap_t *mbm, ...)
{
  va_list va;
  modbmap_t res_mbm = *mbm;
  modbmap_t nxt_mbm = PM_EMPTY;

  va_start (va, mbm);

  while (cnt-- > 1)
  {
    nxt_mbm = *(va_arg (va, modbmap_t *));
    res_mbm = modbmap_or (res_mbm, nxt_mbm);
  }
  va_end (va);
  return(res_mbm);
}

/*
   Name: modbmap_check

   Description:
   Considering only one bit is getting set in second parameter modbmap_t mbm2.
   Checks whether the bit is set or not in the module bitmap mbm1.

   Parameters:
   mbm1 - Module bitmap
   mbm2 - Module bitmap

   Returns:
   PAL_TRUE  - if SET
   PAL_FALSE - if not SET
*/
bool_t
modbmap_check (modbmap_t mbm1, modbmap_t mbm2)
{
  int cnt;

  for(cnt = 0; cnt < MODBMAP_MAX_WORDS; cnt++)
    if(mbm2.mbm_arr[cnt] != 0)
      if(((mbm1.mbm_arr[cnt]) & (mbm2.mbm_arr[cnt]))!= 0)
        return PAL_TRUE;

  return PAL_FALSE;
}

/* Performs logical-AND operation on Module bitmaps and returns a Module bitmap. */
modbmap_t
modbmap_and (modbmap_t mbm1, modbmap_t mbm2)
{
  int cnt;
  modbmap_t temp = PM_EMPTY;

  for(cnt = 0; cnt < MODBMAP_MAX_WORDS; cnt++)
    temp.mbm_arr[cnt] = ((mbm1.mbm_arr[cnt]) & (mbm2.mbm_arr[cnt]));

  return temp;
}

/* Performs logical-OR operation on Module bitmaps and returns a Module bitmap. */
modbmap_t
modbmap_or (modbmap_t mbm1, modbmap_t mbm2)
{
  int cnt;
  modbmap_t temp = PM_EMPTY;

  for(cnt = 0; cnt < MODBMAP_MAX_WORDS; cnt++)
    temp.mbm_arr[cnt] = mbm1.mbm_arr[cnt] | mbm2.mbm_arr[cnt];

  return temp;
}

/* Performs Subtraction on Module bitmaps and returns a Module bitmap. */
modbmap_t
modbmap_sub (modbmap_t mbm1, modbmap_t mbm2)
{
  int cnt;
  modbmap_t temp = PM_EMPTY;

  for(cnt = 0; cnt < MODBMAP_MAX_WORDS; cnt++)
    temp.mbm_arr[cnt] = ((mbm1.mbm_arr[cnt]) & (~(mbm2.mbm_arr[cnt])));

  return temp;
}

/*
   Name: modbmap_isempty

   Description:
   Function to check whether a Module bitmap is NULL or not

   Parameters:
   mbm1 - Module bitmap

   Returns:
   PAL_TRUE  - if NULL
   PAL_FALSE - if not NULL
*/
bool_t
modbmap_isempty (modbmap_t mbm)
{
  int cnt;

  for(cnt = 0; cnt < MODBMAP_MAX_WORDS; cnt++)
    if(mbm.mbm_arr[cnt])
      return PAL_FALSE;

  return PAL_TRUE;
}

/* Function to print out the value of Module bitmap. */
void
modbmap_printvalue (modbmap_t mbm)
{
  int cnt;

  printf("\nMod: ");
  for(cnt = MODBMAP_MAX_WORDS - 1; cnt >= 0; cnt--)
    printf("%x ", mbm.mbm_arr[cnt]);

  printf("\n");
}

/* Function to return the protocol number from the input parameter module bitmap. */
u_int8_t
modbmap_bit2id (modbmap_t mbm)
{
  u_int8_t pos = 0;
  u_int8_t mask_size= 32, wix;
  u_int32_t x, mask= 0xFFFFFFFF;

  for (wix = 0; wix < MODBMAP_MAX_WORDS; wix++)
  {
    if ((x = mbm.mbm_arr[wix]) == 0)
    {
      pos += 32;
    }
    else
    {
      do
      {
        mask_size /= 2;
        mask >>= mask_size;

        if ((x & mask) != 0)
        {
          x = x & mask;
        }
        else
        {
          x = x >> mask_size;
          pos += mask_size;
        }
      } while (mask_size != 1);
      break;
    }
  }
  if (pos <= APN_PROTO_UNSPEC || pos >= APN_PROTO_MAX)
  {
    pos = APN_PROTO_UNSPEC;
  }
  return (pos);
}

/* Initializes all the global predefined Module bit maps. */
void
modbmap_init_all ()
{
  pal_mem_set (&PM_EMPTY, 0, sizeof (PM_EMPTY));

  PM_NSM      = modbmap_id2bit (APN_PROTO_NSM);
  PM_RIP      = modbmap_id2bit (APN_PROTO_RIP);
  PM_RIPNG    = modbmap_id2bit (APN_PROTO_RIPNG);
  PM_OSPF     = modbmap_id2bit (APN_PROTO_OSPF);
  PM_OSPF6    = modbmap_id2bit (APN_PROTO_OSPF6);
  PM_ISIS     = modbmap_id2bit (APN_PROTO_ISIS);
  PM_BGP      = modbmap_id2bit (APN_PROTO_BGP);
  PM_LDP      = modbmap_id2bit (APN_PROTO_LDP);
  PM_RSVP     = modbmap_id2bit (APN_PROTO_RSVP);
  PM_PDM      = modbmap_id2bit (APN_PROTO_PIMDM);
  PM_PIM      = modbmap_id2bit (APN_PROTO_PIMSM);
  PM_PIM6     = modbmap_id2bit (APN_PROTO_PIMSM6);
  PM_DVMRP    = modbmap_id2bit (APN_PROTO_DVMRP);
  PM_AUTH     = modbmap_id2bit (APN_PROTO_8021X);
  PM_ONM      = modbmap_id2bit (APN_PROTO_ONM);
  PM_LACP     = modbmap_id2bit (APN_PROTO_LACP);
  PM_STP      = modbmap_id2bit (APN_PROTO_STP);
  PM_RSTP     = modbmap_id2bit (APN_PROTO_RSTP);
  PM_MSTP     = modbmap_id2bit (APN_PROTO_MSTP);
  PM_IMI      = modbmap_id2bit (APN_PROTO_IMI);
  PM_IMISH    = modbmap_id2bit (APN_PROTO_IMISH);
  PM_PIMPKTGEN = modbmap_id2bit (APN_PROTO_PIMPKTGEN);
  PM_RMON     = modbmap_id2bit (APN_PROTO_RMON);
  PM_HSL      = modbmap_id2bit (APN_PROTO_HSL);
  PM_OAM      = modbmap_id2bit (APN_PROTO_OAM);
  PM_LMP      = modbmap_id2bit (APN_PROTO_LMP);
  PM_VLOG     = modbmap_id2bit (APN_PROTO_VLOG);
  PM_ELMI     = modbmap_id2bit (APN_PROTO_ELMI);

  PM_UCAST     = modbmap_vor (6, &PM_RIP, &PM_RIPNG, &PM_OSPF, &PM_OSPF6,
                              &PM_ISIS, &PM_BGP);

  PM_MCAST     = modbmap_vor (4, &PM_PDM, &PM_PIM, &PM_PIM6, &PM_DVMRP);
  PM_MCAST_VR  = modbmap_vor (3, &PM_PDM, &PM_PIM, &PM_PIM6);
  PM_MPLS      = modbmap_vor (2, &PM_LDP, &PM_RSVP);
  PM_L2        = modbmap_vor (8, &PM_AUTH, &PM_LACP, &PM_STP, &PM_RSTP,
                              &PM_MSTP, &PM_RMON, &PM_ONM, &PM_ELMI);

  PM_ALL       = modbmap_vor (11, &PM_NSM, &PM_UCAST, &PM_MCAST, &PM_MPLS,
                              &PM_L2, &PM_PIMPKTGEN, &PM_RMON, &PM_HSL, 
                              &PM_VLOG, &PM_LMP, &PM_OAM);

  PM_IF        = modbmap_vor (11, &PM_NSM, &PM_RIP, &PM_RIPNG, &PM_OSPF,
                              &PM_OSPF6, &PM_ISIS, &PM_MCAST, &PM_MPLS, &PM_L2,
                              &PM_OAM, &PM_LMP);

  PM_IFDESC    = modbmap_vor (11, &PM_NSM, &PM_STP, &PM_RSTP, &PM_MSTP,
                              &PM_MCAST, &PM_RIP, &PM_RIPNG, &PM_OSPF, &PM_ONM,
                              &PM_OAM, &PM_ELMI, &PM_LMP);
  PM_CA        = modbmap_vor (3, &PM_NSM, &PM_RSVP, &PM_LMP); 
  PM_CC        = modbmap_vor (2, &PM_NSM, &PM_LMP); 
  PM_TEL       = modbmap_vor (2, &PM_NSM, &PM_OSPF); 

  PM_TUN_IF    = modbmap_vor (9, &PM_NSM, &PM_MCAST, &PM_OSPF, &PM_OSPF6,
                              &PM_RIP, &PM_RIP, &PM_RIPNG, &PM_ISIS, &PM_MPLS);

  PM_VR        = modbmap_vor (7, &PM_NSM, &PM_IMI, &PM_VLOG, &PM_UCAST, 
                              &PM_MCAST_VR, &PM_OAM, &PM_LMP);

  PM_VRF       = modbmap_vor (5, &PM_NSM, &PM_RIP, &PM_OSPF, &PM_BGP, 
                              &PM_MCAST_VR);
  PM_VRRP      = PM_NSM;
  PM_RMM       = PM_NSM;
  PM_KEYCHAIN  = modbmap_vor (2, &PM_RIP, &PM_ISIS);
  PM_ACCESS    = modbmap_vor (4, &PM_NSM, &PM_UCAST, &PM_LDP, &PM_MCAST);
  PM_PREFIX    = modbmap_vor (5, &PM_RIP, &PM_RIPNG, &PM_OSPF, &PM_OSPF6,
                              &PM_BGP);

  PM_DISTRIBUTE= modbmap_vor (2, &PM_RIP, &PM_RIPNG);
  PM_RMAP      = modbmap_vor (2, &PM_NSM, &PM_UCAST);
  PM_IFRMAP    = PM_RIPNG;
  PM_LOG       = PM_ALL;
  PM_HOSTNAME  = PM_ISIS;
  PM_NPF       = PM_NSM;
  PM_IGMP      = PM_NSM;
  PM_MLD       = PM_NSM;
  PM_CAL       = modbmap_vor(6, &PM_NSM, &PM_RIP, &PM_LACP, &PM_STP,
                             &PM_RSTP, &PM_MSTP);

  PM_FM        = PM_ALL;

#ifdef HAVE_SNMP
  PM_SNMP_DBG    = modbmap_vor (17, &PM_NSM, &PM_RIP, &PM_OSPF, &PM_OSPF6,
                                &PM_BGP, &PM_LDP, &PM_RSVP, &PM_PIM, &PM_ISIS,
                                &PM_RSTP, &PM_STP, &PM_AUTH, &PM_LACP, &PM_PIM6,
                                &PM_DVMRP, &PM_RMON, &PM_LMP);
#endif
}
