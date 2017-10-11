/* Copyright 2003  All Rights Reserved.
 
  APN 802.3 LACP Port Based Authentication
 
  Authors:  McLendon
 */
#ifndef __LACP_H__
#define __LACP_H__

#include "lib.h"
#include "thread.h"

#include "lacp_types.h"
#ifdef HAVE_SMI
#include "smi_message.h"
#endif /* HAVE_SMI */

extern struct lacp *lacp_instance;

/* Run the state machines for a link */
void
lacp_run_sm (struct lacp_link * link);

/* 802.1d 43.4.8 */
/* extern int  
begin(); */

/* CLI support routines below */
int
lacp_handle_packet (int ifindex, u_char * data, const int len);

extern void
lacp_show_system (struct cli * cli);

extern void
lacp_show_all (struct cli * cli);

extern void
lacp_show_interface (struct cli * cli, const char * const ifname);

extern void
lacp_show_interface_stats (struct cli * cli, const char * const ifname);

extern void
lacp_show_link_traffic_stats (struct cli *cli, const char *const ifname);

extern int
lacp_config_write (struct cli * cli);


int lacp_set_system_priority (unsigned int);
int lacp_get_system_priority (unsigned int *);
int lacp_unset_system_priority ();
int lacp_set_channel_priority (struct lacp_link *, unsigned int);

#ifdef HAVE_SMI
int lacp_get_channel_activity (struct lacp_link *, enum smi_lacp_mode *);
#endif /* HAVE_SMI */

int lacp_unset_channel_priority (struct lacp_link *);
int lacp_get_channel_priority (struct lacp_link *, unsigned int *);
int lacp_set_channel_timeout (struct lacp_link *, int);
int lacp_get_channel_timeout (struct lacp_link *, int *);
int lacp_set_channel_admin_key (struct lacp_link *, unsigned int);
int lacp_get_channel_admin_key (struct lacp_link *, unsigned int *);
int lacp_set_channel_activity (struct lacp_link *, unsigned int);
int recv_lacp (struct thread *);
int lacp_system_mac_address_set (u_char *);
int lacp_system_mac_address_get (u_char *);
int lacp_system_mac_address_unset ();
int lacp_system_mac_address_update ();
int lacp_aggregator_set_mac_address (struct lacp_aggregator *, u_char *);
int open_lacp ();
void close_lacp ();
#if 0
int send_lacp (struct lacp_addr *, unsigned char *, int);
#endif
void lacp_activate ();
void lacp_deactivate ();
int send_lacp (struct sockaddr_l2 *addr, unsigned char *data, int len);

#ifdef HAVE_SMI

int lacp_get_aggregator_idx(struct smi_lacp_agg_bmp *aggBmp);
int lacp_get_aggregrator_detail(u_int32_t, 
                                struct smi_lacp_channel *);
int lacp_get_aggregrator_summary (u_int32_t,
                                  struct smi_lacp_channel_summary *);
int lacp_get_aggregator_port_counters (struct lacp_link *, 
                                       struct smi_lacp_link_counters *);
#endif /* HAVE_SMI */

#ifdef HAVE_USER_HSL

#define LACP_ASYNC             1
#define RCV_BUFSIZ             1500

pal_sock_handle_t
lacp_sock_init (struct lib_globals *zg);

/*
   HSL transport socket structure.
*/
struct lacp_client
{
  struct message_handler *mc;
};

int lacp_client_create (struct lib_globals *zg, struct lacp_client *nl,
                        u_char async);
#endif /* HAVE_USER_HSL */


#endif /* __LACP_H__ */
