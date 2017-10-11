/* Copyright (C) 2001-2003  All Rights Reserved. */

#ifndef _PACOS_IMI_DHCPS_H
#define _PACOS_IMI_DHCPS_H

/* Name string. */
#define DHCP_STR        "Dynamic Host Configuration Protocol"

/* Global pointer. */
#define DHCP            imig->dhcp

/* Defaults. */
#define IMI_DHCP_LEASE_DEFAULT_SEC              86400
#define IMI_DHCP_LEASE_DEFAULT_MIN              0
#define IMI_DHCP_LEASE_DEFAULT_HR               0
#define IMI_DHCP_LEASE_DEFAULT_DAY              1
#define IMI_DHCP_POOL_ID_LEN_MAX                255
#define IMI_DHCP_DOMAIN_NAME_LEN_MAX            255

/* DHCP client structure. */
struct imi_dhcp_client
{
  /* Interface pointer. */
  struct interface *ifp;

  /* Currently suspended? */
  bool_t suspended;

  /* Client-id interface pointer. */
  struct interface *clientid;

  /* Hostname to use. */
  char *hostname;
};

/* DHCP server top structure. */
struct imi_dhcp
{
  /* On/off */
  bool_t enabled;

  /* Pool list. */
  struct list *pool_list;

  /* PAL. */
  pal_handle_t pal_dhcp;

  /* System configuration exists. */
  bool_t sysconfig;
  bool_t shutdown_flag;
};

/* DHCP pool structure.  */
struct imi_dhcp_pool
{
  /* Pool Id. */
  char *pool_id;

  /* Network */
  bool_t network_set;
  struct prefix_ipv4 network;

  /* Range list(s) */
  struct list *addr_list;
  struct list *range_list;

  /* Lease */
  bool_t infinite;
  u_int32_t     lease_seconds;
  u_int8_t      lease_minutes;
  u_int8_t      lease_hours;
  u_int8_t      lease_days;

  /* Domain name */
  char *domain_name;

  /* DNS server(s).*/
  struct list *dns_list;

  /* Default Router */
  struct list *dr_list;
};

/* DHCP range.  */
struct imi_dhcp_range
{
  struct prefix_ipv4 low;
  struct prefix_ipv4 high;
};

/* Function Prototypes: */

/* DHCP Server Init. */
void imi_dhcps_init ();

/* DHCP Server Init. */
void imi_dhcps_shutdown ();

/* Start DHCP server. */
int imi_dhcps_enable ();

/* DHCP Server configuration write. */
int imi_dhcps_service_write (struct cli *cli);
int imi_dhcps_service_encode (struct imi_dhcp *dhcp, cfg_vect_t *cv);

int imi_dhcps_config_write (struct cli *cli);
int imi_dhcps_config_encode (struct imi_dhcp *dhcp, cfg_vect_t *cv);

/* DHCP Server reset. */
result_t imi_dhcps_reset ();

/* DHCP server Refresh (after interface up information received from NSM). */
int imi_dhcps_refresh (struct interface *ifp);

/* DHCP Client Init. */
void imi_dhcpc_init ();

/* DHCP Client resume (after interface information received from NSM). */
int imi_dhcpc_resume (struct interface *ifp);

/* DHCP Client 'show interface IFNAME' display. */
void imi_dhcpc_show_interface (struct cli *cli, struct interface *ifp);

/* DHCP Client configuration write. */
void imi_dhcpc_config_write_if (struct cli *cli, struct interface *ifp);

/* Free DHCP client. */
void imi_dhcp_client_free (struct imi_interface *imi_if);

/* DHCP Client Shutdown. */
void imi_dhcpc_shutdown ();

#endif /* _PACOS_IMI_DHCPS_H */
