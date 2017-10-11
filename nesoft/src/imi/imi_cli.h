/* Copyright (C) 2003  All Rights Reserved. */

#ifndef _PACOS_IMI_CLI_H
#define _PACOS_IMI_CLI_H

#if defined (HAVE_RSTPD) || defined (HAVE_MSTPD) || defined (HAVE_STPD)
#define IF_BRIDGE_PROTO(M) \
  if ((M) == APN_PROTO_MSTP || (M) == APN_PROTO_RSTP || (M) == APN_PROTO_STP)
#endif /* HAVE_RSTPD || HAVE_MSTPD || HAVE_STPD */

int imi_terminal_monitor_set (u_int32_t, u_char, u_int32_t);
int imi_terminal_monitor_unset (u_int32_t, u_char, u_int32_t);

int imi_show_protocol (struct apn_vr *, module_id_t, char *, char *, void *,
                       void (*out_func) (void *, char *, module_id_t));

s_int32_t imi_show_scatter_gather (struct cli *);

int imi_cli_init (u_int16_t);
int imi_password_authentication (struct host *, char *, char *, struct cli *);

int imi_vty_pm_connect (struct apn_vr *vr);

#endif /* _PACOS_IMI_CLI_H */
