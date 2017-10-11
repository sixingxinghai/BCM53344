/* Copyright (C) 2003  All Rights Reserved. */

#ifndef _NSM_STATS_API_H
#define _NSM_STATS_API_H

void nsm_stats_show_init (struct cli_tree *ctree);

#ifdef HAVE_HAL
int nsm_statistics_vlan_get (u_int32_t vlan_id,
                                  struct hal_stats_vlan *vlan);
int nsm_statistics_port_get (u_int32_t port_id,
                                  struct hal_stats_port *port);
int nsm_statistics_host_get (struct hal_stats_host *host);
#endif /* HAVE_HAL */
int nsm_statistics_clear (u_int32_t port_id);
#endif /* _NSM_STATS_API_H */ 
