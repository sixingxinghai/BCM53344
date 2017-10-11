/* Copyright (C) 2002-2003   All Rights Reserved. */

#ifndef _MODBMAP_H_
#define _MODBMAP_H_

#include "pal.h"

/* Module bitmap Word Size. */
/* MODBMAP_WORD_BITS  should be derived from __WORDSIZE */
#define MODBMAP_WORD_BITS (32)

/* Module bitmap Size in bytes. */
#define MODBMAP_MAX_WORDS (2)

/* Module bitmap Size in bits. */
#define MODBMAP_MAX_BITS  ((MODBMAP_WORD_BITS) * (MODBMAP_MAX_WORDS))

/* Make sure the size of bit map is big enough to hold all modules. */
#if ((MODBMAP_MAX_BITS) < (APN_PROTO_MAX))
#error **** lib/modbmap.h: Increase MODBMAP_MAX_WORDS ****
#endif

/* Module bitmap structure */
typedef struct modbmap
{
  u_int32_t mbm_arr[MODBMAP_MAX_WORDS];
} modbmap_t;

/* Macro to set Module bitmap. */
#define MODBMAP_SET(mbm, bit) \
  (mbm).mbm_arr[(bit) / MODBMAP_WORD_BITS] |= (1<< ((bit) % MODBMAP_WORD_BITS))

/* Macro to unset Module bitmap. */
#define MODBMAP_UNSET(mbm, bit) \
  (mbm).mbm_arr[(bit) / MODBMAP_WORD_BITS] &= ~(1<< ((bit) % MODBMAP_WORD_BITS))

/* Macro to check whether the Module bitmap is set or not for a given bit. */
#define MODBMAP_ISSET(mbm, bit)                             \
  ((((mbm).mbm_arr[(bit) / MODBMAP_WORD_BITS] &             \
   (1<< ((bit) % MODBMAP_WORD_BITS))) != 0)? PAL_TRUE: PAL_FALSE)

modbmap_t modbmap_or (modbmap_t mbm1, modbmap_t mbm2);
modbmap_t modbmap_and (modbmap_t mbm1, modbmap_t mbm2);
modbmap_t modbmap_sub (modbmap_t mbm1, modbmap_t mbm2);
modbmap_t modbmap_vor (u_int8_t cnt, modbmap_t *mbm, ...);
modbmap_t modbmap_id2bit (u_int8_t bit_num);
bool_t modbmap_isempty (modbmap_t mbm);
bool_t modbmap_check (modbmap_t mbm1, modbmap_t mbm2);
void modbmap_printvalue (modbmap_t mbm);
void modbmap_init_all ();
u_int8_t modbmap_bit2id (modbmap_t mbm);

extern modbmap_t PM_EMPTY;
extern modbmap_t PM_NSM;
extern modbmap_t PM_MNT;
extern modbmap_t PM_RIP;
extern modbmap_t PM_RIPNG;
extern modbmap_t PM_OSPF;
extern modbmap_t PM_OSPF6;
extern modbmap_t PM_ISIS;
extern modbmap_t PM_BGP;
extern modbmap_t PM_LDP;
extern modbmap_t PM_RSVP;
extern modbmap_t PM_PDM;
extern modbmap_t PM_PIM;
extern modbmap_t PM_PIM6;
extern modbmap_t PM_DVMRP;
extern modbmap_t PM_AUTH;
extern modbmap_t PM_ONM;
extern modbmap_t PM_LACP;
extern modbmap_t PM_STP;
extern modbmap_t PM_RSTP;
extern modbmap_t PM_MSTP;
extern modbmap_t PM_IMI;
extern modbmap_t PM_IMISH;
extern modbmap_t PM_PIMPKTGEN;
extern modbmap_t PM_RMON;
extern modbmap_t PM_HSL;
extern modbmap_t PM_ELMI;

extern modbmap_t PM_UCAST;
extern modbmap_t PM_MCAST;
extern modbmap_t PM_MCAST_VR;
extern modbmap_t PM_MPLS;
extern modbmap_t PM_L2;
extern modbmap_t PM_ALL;
extern modbmap_t PM_IF;
extern modbmap_t PM_CA;
extern modbmap_t PM_CC;
extern modbmap_t PM_TEL;
extern modbmap_t PM_IFDESC;
extern modbmap_t PM_TUN_IF;
extern modbmap_t PM_VR;
extern modbmap_t PM_VRF;
extern modbmap_t PM_VRRP;
extern modbmap_t PM_RMM;
extern modbmap_t PM_KEYCHAIN;
extern modbmap_t PM_ACCESS;
extern modbmap_t PM_PREFIX;
extern modbmap_t PM_DISTRIBUTE;
extern modbmap_t PM_RMAP;
extern modbmap_t PM_IFRMAP;
extern modbmap_t PM_LOG;
extern modbmap_t PM_HOSTNAME;
extern modbmap_t PM_NPF;
extern modbmap_t PM_IGMP;
extern modbmap_t PM_MLD;
extern modbmap_t PM_CAL;
extern modbmap_t PM_FM;
extern modbmap_t PM_SNMP_DBG;
extern modbmap_t PM_OAM;
extern modbmap_t PM_LMP;
extern modbmap_t PM_VLOG;

#endif

