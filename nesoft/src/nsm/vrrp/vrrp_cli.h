/* Copyright (C) 2001-2003  All Rights Reserved. */

#ifndef _PACOS_VRRP_CLI_H
#define _PACOS_VRRP_CLI_H

void vrrp_debug_all_off (struct cli *);
int vrrp_debug_config_write (struct cli *);
void vrrp_cli_init(struct lib_globals *);
void vrrp_if_master_dump (struct cli *, struct interface *);

#endif /* _PACOS_VRRP_CLI_H */
