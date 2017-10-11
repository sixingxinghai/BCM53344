/* Copyright (C) 2004  All Rights Reserved. */

#ifndef _HAL_STATS_API_H_
#define _HAL_STATS_API_H_

int
hal_statistics_vlan_get (u_int32_t vlan_id,struct hal_stats_vlan *);
int
hal_statistics_port_get (u_int32_t port_id,struct hal_stats_port *);
int
hal_statistics_host_get (struct hal_stats_host *);
int
hal_statistics_clear (u_int32_t port_id);

int
_hal_stats_port_parse (struct hal_nlmsghdr *h, void *data);
int
_hal_stats_host_parse (struct hal_nlmsghdr *h, void *data);
int
_hal_stats_vlan_parse (struct hal_nlmsghdr *h, void *data);


#endif /*_HAL_STATS_API_H_*/

