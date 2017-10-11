/* Copyright (C) 2002-2003  All Rights Reserved. */

#ifndef _PACOS_IMI_VS_H
#define _PACOS_IMI_VS_H

struct imi_virtual_server
{
  /* Protocol. */
  char protocol[PROTO_STR_LEN];

  /* Private. */
  char priv_ip[IPV4_MAX_PREFIXLEN];
  u_int16_t priv_port;

  /* Public. */
  char pub_ip[IPV4_MAX_PREFIXLEN];
  u_int16_t pub_port;

  /* Description. */
  char *desc;
};

#ifdef HOME_GATEWAY
int imi_virtual_server_reset (struct imi_serv *);
#endif /* HOME_GATEWAY */

int imi_virtual_server_check (const char *, u_int16_t ,
                              struct pal_in4_addr *, u_int16_t);
int imi_virtual_server_if_address_add (struct connected *);
int imi_virtual_server_if_address_delete (struct connected *);
int imi_virtual_server_config_write (struct cli *);
int imi_virtual_server_config_encode (cfg_vect_t *cv);
int imi_virtual_server_init (struct lib_globals *);
int imi_virtual_server_finish (struct lib_globals *);

#endif /* _PACOS_IMI_VS_H */
