/* Copyright (C) 2001-2003  All Rights Reserved. */

#ifndef _PACOS_IMI_UTIL_H
#define _PACOS_IMI_UTIL_H

/* Data structures. */

/* WAN status. */
enum wan_status {
  WAN_STATIC,
  WAN_DHCP,
  WAN_PPPoE
};

#define IMI_WAN_STATIC_STRING   "static\n"
#define IMI_WAN_DHCP_STRING     "dhcp\n"
#define IMI_WAN_PPPoE_STRING    "pppoe\n"

/* Function prototypes: */

/* Default GW set/reset. */
int imi_default_gw_set (struct imi_serv *);
int imi_default_gw_reset (struct imi_serv *);

/* WAN Status get/set. */
enum wan_status imi_wan_status_get ();
int imi_wan_status_set (enum wan_status);
int imi_wan_status_reset_default ();

/* CLI lock/unlock. */
void imi_cli_lock ();
void imi_cli_unlock ();

/* Init. */
void imi_util_init ();

#endif /* _PACOS_IMI_DNS_H */
