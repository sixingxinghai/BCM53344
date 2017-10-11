/* Copyright (C) 2012  All Rights Reserved.

L3 HAL
  like stub for pc compile

*/

#include "pal.h"
#include "lib.h"

#include "hal_types.h"
#include "hal_error.h"

int
hal_lacp_add_aggregator (char *name,
                         unsigned char mac[ETHER_ADDR_LEN],
                         int agg_type)
{
 return HAL_SUCCESS;
}

int
hal_lacp_delete_aggregator (char *name, unsigned int ifindex)
{
 return HAL_SUCCESS;
}

int
hal_lacp_attach_mux_to_aggregator (char *agg_name, unsigned int agg_ifindex,
                                   char *port_name, unsigned int port_ifindex)
{
 return HAL_SUCCESS;
}

int
hal_lacp_detach_mux_from_aggregator (char *agg_name, unsigned int agg_ifindex,
                                     char *port_name,
                                     unsigned int port_ifindex)
{
 return HAL_SUCCESS;
}

int
hal_lacp_psc_set (unsigned int ifindex,int psc)
{
 return HAL_SUCCESS;
}

