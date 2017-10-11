/* Copyright (C) 2001-2003  All Rights Reserved. */

#ifndef _PACOS_IMI_PPPOE_H
#define _PACOS_IMI_PPPOE_H

/* Name string. */
#define PPPOE_NAME        "Point-to-Point over Ethernet"
#define PPPOE_STR         PPPOE_NAME

/* Global pointer to DNS structure. */
#define PPPOE             imig->pppoe

/* PPPOE structures. */

/* PPPOE */
struct imi_pppoe
{
  /* On/off */
  bool_t enabled;
  bool_t disabling;

  /* Username. */
  char *username;

  /* Password. */
  char *passwd;

  /* Interface on which PPPOE is enabled. */
  struct interface *ifp;

  /* MTU for PPPOE encapsulated packets. */
  u_int32_t mtu;

  /* PAL. */
  pal_handle_t pal_pppoe;

  /* System configuration exists. */
  bool_t sysconfig;
  bool_t shutdown_flag;
};

/* Function prototypes: */
int imi_pppoe_enable ();
int imi_pppoe_disable ();

/* PPPOE Client Init. */
void imi_pppoe_init ();

/* PPPOE Client shutdown. */
void imi_pppoe_shutdown ();

/* Start PPPoE service. */
void imi_pppoe_service ();

/* Stop PPPoE service. */
void imi_pppoe_no_service ();

/* DNS Client configuration write. */
int imi_pppoe_config_write (struct cli *cli);

s_int32_t imi_pppoe_config_encode (struct imi_pppoe *pppoe, cfg_vect_t *cv);

/* Check if other parameters to enable PPPOE are set. */
int imi_pppoe_check_param (struct cli *cli);

#endif /* _PACOS_IMI_PPPOE_H */
