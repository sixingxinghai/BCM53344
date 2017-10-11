/* Copyright (C) 2004  All Rights Reserved. */

#include "pal.h"
#include "lib.h"

#include "hal_incl.h"

/* 
   Name: hal_if_get_list

   Description:
   This API gets the list of interfaces from the interface manager.

   Parameters:
   None

   Returns:
   < 0 on error
   HAL_SUCCESS
*/
int
hal_if_get_list (void)
{
  return HAL_SUCCESS;
}


/* 
   Name: hal_if_get_metric 

   Description: 
   This API gets the metric for a interface.

   Parameters:
   IN -> ifname - interface name
   IN -> ifindex - interface ifindex
   OUT -> metric - metric

   Returns:
   < 0 on error 
   HAL_SUCCESS
*/
int
hal_if_get_metric (char *ifname, unsigned int ifindex, int *metric)
{
  return HAL_SUCCESS;
}

/* 
   Name: hal_if_get_mtu 

   Description:
   This API get the mtu for a interface.

   Parameters:
   IN -> ifname - interface name
   IN -> ifindex - interface index
   OUT -> mtu - mtu

   Returns:
   < 0 on error
   HAL_SUCCESS
*/
int
hal_if_get_mtu (char *ifname, unsigned int ifindex, int *mtu)
{
  return HAL_SUCCESS;
}

/* 
   Name: hal_if_set_mtu

   Description:
   This API set the MTU for a interface.

   Parameters:
   IN -> ifname - interface name
   IN -> ifindex - interface index
   IN -> mtu - mtu

   Returns:
   < 0 on error
   HAL_SUCCESS
*/
int
hal_if_set_mtu (char *ifname, unsigned int ifindex, int mtu)
{
  return HAL_SUCCESS;
}

/*
   Name: hal_if_get_arp_ageing_timeout

   Description:
   This API set the arp ageing timeout for a interface.

   Parameters:
   IN -> ifname - interface name
   IN -> ifindex - interface index
   IN -> arp_ageing_timeout - arp ageing timeout value

   Returns:
   < 0 on error
   HAL_SUCCESS
*/
int
hal_if_set_arp_ageing_timeout (char *ifname, unsigned int ifindex, int arp_ageing_timeout)
{
  return HAL_SUCCESS;
}

/*
   Name: hal_if_get_arp_ageing_timeout

   Description:
   This API get the arp ageing timeout for a interface.

   Parameters:
   IN -> ifname - interface name
   IN -> ifindex - interface index
   OUT -> arp_ageing_timeout - arp ageing timeout value

   Returns:
   < 0 on error
   HAL_SUCCESS
*/
int
hal_if_get_arp_ageing_timeout (char *ifname, unsigned int ifindex, int *arp_ageing_timeout)
{
  return HAL_SUCCESS;
}


/* 
   Name: hal_if_get_duplex 

   Description:
   This API get the duplex for a interface.

   Parameters:
   IN -> ifname - interface name
   IN -> ifindex - interface index
   OUT -> duplex - duplex

   Returns:
   < 0 on error
   HAL_SUCCESS
*/
int
hal_if_get_duplex (char *ifname, unsigned int ifindex, int *duplex)
{
  return HAL_SUCCESS;
}

/*
   Name: hal_if_set_duplex

   Description:
   This API set the DUPLEX for a interface.

   Parameters:
   IN -> ifname - interface name
   IN -> ifindex - interface index
   IN -> duplex - duplex 

   Returns:
   < 0 on error
   HAL_SUCCESS
*/
int
hal_if_set_duplex (char *ifname, unsigned int ifindex, int duplex)
{
  return HAL_SUCCESS;
}

/*
   Name: hal_if_set_autonego

   Description:
   This API set the DUPLEX with auto-negotiate for a interface.

   Parameters:
   IN -> ifname - interface name
   IN -> ifindex - interface index
   IN -> autonego - autonego 

   Returns:
   < 0 on error
   HAL_SUCCESS
*/
int
hal_if_set_autonego (char *ifname, unsigned int ifindex, int autonego)
{
  return HAL_SUCCESS;
}

/*
  Name: hal_if_get_hwaddr

  Description:
  This API gets the hardware address for a interface. This is the MAC 
  address in case of ethernet. The caller has to provide a buffer large
  enough to hold the address.

  Parameters:
  IN -> ifname - interface name
  IN -> ifindex - interface ifindex
  OUT -> hwaddr - hardware address
  OUT -> hwaddr_len - hardware address length

  Returns:
  < 0 on error
  HAL_SUCCESS
*/
int
hal_if_get_hwaddr (char *ifname, unsigned int ifindex, 
                   u_char *hwaddr, int *hwaddr_len)
{
  return HAL_SUCCESS;
}

/*
  Name: hal_if_set_hwaddr

  Description:
  This API sets the hardware address for a interface. This is the MAC 
  address in case of ethernet.

  Parameters:
  IN -> ifname - interface name
  IN -> ifindex - interface ifindex
  IN -> hwaddr - hardware address
  IN -> hwlen - hardware address length

  Returns:
  < 0 on error
  HAL_SUCCESS
*/
int
hal_if_set_hwaddr (char *ifname, unsigned int ifindex,
                   u_int8_t *hwaddr, int hwlen)
{
  return HAL_SUCCESS;
}

/* 
   Name: hal_if_flags_get

   Description:
   This API gets the flags for a interface. The flags are
   IFF_RUNNING
   IFF_UP
   IFF_BROADCAST
   IFF_LOOPBACK

   Parameters:
   IN -> ifname - interface name
   IN -> ifindex - interface ifindex
   OUT -> flags - flags
   
   Returns:
   < 0 on error
   HAL_SUCCES
*/
int
hal_if_flags_get (char *ifname, unsigned int ifindex, u_int32_t *flags)
{
  return HAL_SUCCESS;
}


/*
  Name: hal_if_flags_set
  
  Description:
  This API sets the flags for a interface. The flags are
  IFF_RUNNING
  IFF_UP
  IFF_BROADCAST
  IFF_LOOPBACK
  
   Parameters:
   IN -> ifname - interface name
   IN -> ifindex - interface ifindex
   IN -> flags - flags to set

   Returns:
   < 0 for error
   HAL_SUCCESS
*/
int
hal_if_flags_set (char *ifname, unsigned int ifindex, unsigned int flags)
{
  return 0;
}

/* 
   Name: hal_if_flags_unset

   Description:
   This API unsets the flags for a interface. The flags are
   IFF_RUNNING
   IFF_UP
   IFF_BROADCAST
   IFF_LOOPBACK
   
   Parameters:
   IN -> ifname - interface name
   IN -> ifindex - interface ifindex
   IN -> flags - flags to unset

   Returns:
   < 0 for error
   HAL_SUCCESS
*/
int
hal_if_flags_unset (char *ifname, unsigned int ifindex, unsigned int flags)
{
  return 0;
}

/* 
   Name: hal_if_get_bw

   Description:
   This API gets the bandwidth for the interface. This API should
   return the value in bytes per second.

   Parameters:
   IN -> ifname - interface name
   IN -> ifindex - interface ifindex
   OUT -> bandwidth - interface bandwidth

   Returns:
   < 0 for error
   HAL_SUCCESS
*/
int
hal_if_get_bw (char *ifname, unsigned int ifindex, u_int32_t *bandwidth)
{
  return HAL_SUCCESS;
}

/*
   Name: hal_if_set_bw

   Description:
   This API set the bandwidth for a interface.

   Parameters:
   IN -> ifname - interface name
   IN -> ifindex - interface index
   IN -> bandwidth - bandwidth

   Returns:
   < 0 on error
   HAL_SUCCESS
*/
int
hal_if_set_bw (char *ifname, unsigned int ifindex, unsigned int bandwidth)
{
  return HAL_SUCCESS;
}



#ifdef HAVE_L2
/*
  Name: hal_if_set_port_type 

  Description:
  This API set the port type i.e. ROUTER or SWITCH port for a interface.

  Parameters:
  IN -> name - name of the interface
  IN -> ifindex - ifindex
  IN -> type - the port type
  OUT -> retifindex - the ifindex of the new type of interface

  Returns:
  < 0 on error
  HAL_SUCCESS
*/
int
hal_if_set_port_type (char *name, unsigned int ifindex, 
                      enum hal_if_port_type type, unsigned int *retifindex)
{
  return HAL_SUCCESS;
}

/*
  Name: hal_if_svi_create

  Description:
  This API creates a SVI(Switch Virtual Interface) for a specific VLAN. The
  the VLAN information is embedded in the name of the interface.

  Parameters:
  IN -> name - interface name
  OUT -> retifindex - ifindex for the interface

  Returns:
  < 0 on error
  HAL_SUCCESS
*/
int
hal_if_svi_create (char *name, unsigned int *retifindex)
{
  return HAL_SUCCESS;
}

/*
  Name: hal_if_svi_delete

  Description:
  This API deletes the SVI(Switch Virtual Interface) for a specific VLAN. 

  Parameters:
  IN -> name - interface name
  IN -> ifindex - ifindex

  Returns:
  < 0 on error
  HAL_SUCCESS
*/
int
hal_if_svi_delete (char *name, unsigned int ifindex)
{
  return HAL_SUCCESS;
}
#endif /* HAVE_L2 */

/* structure for returning interface statistics. */
struct _ifstat_resp
{
  struct hal_if_counters *if_stats;
};


/* 
   Name: _hal_if_get_counters

   Description: This API gets interface statistics for specific  
   ifindex.

   Parameters:
   IN -> ifindex - Interface index.
   IN -> command - Counters type. 
   OUT ->if_stats - the array of counters for interface. 

   Returns:
  Returns:
  < 0 on error
  HAL_SUCCESS
*/

static int
_hal_if_get_counters(unsigned int ifindex, int command, struct hal_if_counters *if_stats)
{
  return 0;
}

/* 
   Name: hal_if_get_counters

   Description: This API gets interface statistics for specific  
   ifindex.

   Parameters:
   IN -> ifindex - Interface index.
   OUT ->if_stats - the array of counters for interface. 

   Returns:
  Returns:
  < 0 on error
  HAL_SUCCESS
*/
int
hal_if_get_counters(unsigned int ifindex, struct hal_if_counters *if_stats)
{
  return 0;
}

int
hal_vlan_if_get_counters(unsigned int ifindex,unsigned int vlan,
                         struct hal_vlan_if_counters *if_stats)
{
  return 0;
}



int
hal_if_delete_done(char *ifname, u_int16_t ifindex)
{
  return 0;
}

int
hal_ip_set_access_group(struct hal_ip_access_grp access_grp,
                        char *ifname,
                        int action,
                        int dir)
{
  return 0;
}

int
hal_ip_set_access_group_interface (struct hal_ip_access_grp access_grp,
                                   char *vifname, char *ifname,
                                   int action, int dir)
{
  return 0;
}

int
hal_if_stats_flush (u_int16_t ifindex)
{
  return 0;
}

int
hal_if_set_mdix(unsigned int ifindex, unsigned int mdix)
{
  return 0;
}

int
hal_if_set_sw_reset ()
{
   return HAL_SUCCESS;
}

int
hal_pro_vlan_set_dtag_mode (unsigned int ifindex,unsigned short dtag_mode)
{
  return HAL_SUCCESS;
}

int
hal_if_set_port_egress (unsigned int ifindex, int egress_mode)
{
  return HAL_SUCCESS;
}

int
hal_if_set_cpu_default_vid (unsigned int ifindex, int vid)
{
  return HAL_SUCCESS;
}

int
hal_if_set_wayside_default_vid (unsigned int ifindex, int vid)
{
  return HAL_SUCCESS;
}

int
hal_if_set_learn_disable (unsigned int ifindex, int enable)
{
  return HAL_SUCCESS;
}

int
hal_if_get_learn_disable (unsigned int ifindex, int* enable)
{
  return HAL_SUCCESS;
}

int
hal_if_set_force_vlan (unsigned int ifindex, int vid)
{
  return HAL_SUCCESS;
}

int
hal_if_set_ether_type (unsigned int ifindex, u_int16_t etype)
{
  return HAL_SUCCESS;
}

int
hal_if_set_preserve_ce_cos (unsigned int ifindex)
{
  return HAL_SUCCESS;
}

