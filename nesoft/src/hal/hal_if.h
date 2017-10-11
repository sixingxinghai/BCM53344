/* Copyright (C) 2004  All Rights Reserved. */

#ifndef _HAL_IF_H_
#define _HAL_IF_H_

/*   Message structures. 
*/

#define HAL_BIT_MAP_MAX 128


struct hal_reg_addr
{
  u_int32_t reg_addr;
  u_int32_t value;
};

/**********************************************************************
 *                           STATISTICS                               *
 **********************************************************************/
struct hal_stats_vlan
{
  ut_int64_t ucastpkts;
  ut_int64_t ucastbytes;
  ut_int64_t mcastpkts;
  ut_int64_t mcastbytes;
  u_int32_t  vlan_id;
};

struct hal_port
{
  u_int32_t ifindex;
};

struct hal_stats_port
{
  ut_int64_t rxpkts;
  ut_int64_t rxdrops;
  ut_int64_t rxbytes;
  ut_int64_t rxmcac;
  ut_int64_t rxbcac;
  ut_int64_t txpkts;
  ut_int64_t txdrops;
  ut_int64_t txbytes;
  ut_int64_t txmcac;
  ut_int64_t txbcac;
  ut_int64_t fldpkts;
  ut_int64_t vlandrops;
  ut_int64_t stmdrops;
  ut_int64_t eddrops;
  u_int32_t port_id;
};

struct hal_stats_host
{
  ut_int64_t hostInpkts;
  ut_int64_t hostOutpkts;
  ut_int64_t hostOuterrs;
  ut_int64_t learnDrops;
};

/* Port Mirroring direction enumeration. */
enum hal_port_mirror_direction
  {
    HAL_PORT_MIRROR_DISABLE                =  0,
    HAL_PORT_MIRROR_DIRECTION_RECEIVE      = (1 << 0),
    HAL_PORT_MIRROR_DIRECTION_TRANSMIT     = (1 << 1),
    HAL_PORT_MIRROR_DIRECTION_BOTH         = (HAL_PORT_MIRROR_DIRECTION_RECEIVE | HAL_PORT_MIRROR_DIRECTION_TRANSMIT)
  };
/*
   Interface counters.
 */
struct hal_if_counters
  {
    ut_int64_t out_errors;
    ut_int64_t out_discards;
    ut_int64_t out_mc_pkts;
    ut_int64_t out_uc_pkts;
    ut_int64_t in_discards;
    ut_int64_t good_octets_rcv;
    ut_int64_t bad_octets_rcv;
    ut_int64_t mac_transmit_err;
    ut_int64_t good_pkts_rcv;
    ut_int64_t bad_pkts_rcv;
    ut_int64_t brdc_pkts_rcv;
    ut_int64_t mc_pkts_rcv;
    ut_int64_t pkts_64_octets;
    ut_int64_t pkts_65_127_octets;
    ut_int64_t pkts_128_255_octets;
    ut_int64_t pkts_256_511_octets;
    ut_int64_t pkts_512_1023_octets;
    ut_int64_t pkts_1024_max_octets;
    ut_int64_t good_octets_sent;
    ut_int64_t good_pkts_sent;
    ut_int64_t excessive_collisions;
    ut_int64_t mc_pkts_sent;
    ut_int64_t brdc_pkts_sent;
    ut_int64_t unrecog_mac_cntr_rcv;
    ut_int64_t fc_sent;
    ut_int64_t good_fc_rcv;
    ut_int64_t drop_events;
    ut_int64_t undersize_pkts;
    ut_int64_t fragments_pkts;
    ut_int64_t oversize_pkts;
    ut_int64_t jabber_pkts;
    ut_int64_t mac_rcv_error;
    ut_int64_t bad_crc;
    ut_int64_t collisions;
    ut_int64_t late_collisions;
    ut_int64_t bad_fc_rcv;
    ut_int64_t port_in_overflow_frames;
    ut_int64_t port_out_overflow_frames;
    ut_int64_t port_in_overflow_discards;
    ut_int64_t in_filtered;
    ut_int64_t out_filtered;
    ut_int64_t mtu_exceed;
  };

struct hal_vlan_if_counters
{
  ut_int64_t next_free_local_vlan_index;
  ut_int64_t tp_vlan_port_in_frames;
  ut_int64_t tp_vlan_port_out_frames;
  ut_int64_t tp_vlan_port_in_discards;
  ut_int64_t tp_vlan_port_in_overflow_frames;
  ut_int64_t tp_vlan_port_out_overflow_frames;
  ut_int64_t tp_vlan_port_in_overflow_discards;
  ut_int64_t tp_vlan_port_HC_in_frames;
  ut_int64_t tp_vlan_port_HC_out_frames;
  ut_int64_t tp_vlan_port_HC_in_discards;
 };
 
struct hal_msg_if_stat
{
   struct hal_if_counters cntrs;
   unsigned int ifindex;
};

struct hal_msg_if_clear_stat
{
   unsigned int ifindex;
};
 
/* 
  Used for MDIX crossover. 
*/
struct hal_msg_if_mdix
{
  unsigned int mdix;
  unsigned int ifindex;
};
  
/*
  Port Bit Map
*/
struct hal_port_map
{
  u_int32_t bitmap[HAL_BIT_MAP_MAX];
};

/*
  Portbased Vlan 
*/
struct hal_msg_if_portbased_vlan
{
  struct hal_port_map pbitmap;
  unsigned int ifindex;
};

struct hal_msg_if_cpu_default_vid
{
  unsigned int ifindex;
  int vid;
};

/* 
  Port Egress mode 
*/
struct hal_msg_if_port_egress
{
  int egress;
  unsigned int ifindex;
};


/*
   Learn Disable
*/
struct hal_msg_if_learn_disable
{
  int enable;
  unsigned int ifindex;
};


/*
 Port Force Vlan  
*/
struct hal_msg_if_force_vlan
{
  int vid;
  unsigned int ifindex;
};

/*
 Port Ether Type
*/
struct hal_msg_if_ether_type
{
  u_int16_t etype;
  unsigned int ifindex;
};

/* 
   Port type. 
*/
enum hal_if_port_type
  {
    HAL_IF_SWITCH_PORT,
    HAL_IF_ROUTER_PORT
  };

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
hal_if_get_metric (char *ifname, unsigned int ifindex, int *metric);

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
hal_if_get_mtu (char *ifname, unsigned int ifindex, int *metric);

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
hal_if_set_mtu (char *ifname, unsigned int ifindex, int mtu);

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
hal_if_set_arp_ageing_timeout (char *ifname, unsigned int ifindex, int arp_ageing_timeout);

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
hal_if_get_arp_ageing_timeout (char *ifname, unsigned int ifindex, int *arp_ageing_timeout);

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
hal_if_set_duplex (char *ifname, unsigned int ifindex, int duplex);

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
hal_if_get_duplex (char *ifname, unsigned int ifindex, int *duplex);

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
hal_if_get_bw (char *ifname, unsigned int ifindex, u_int32_t *bandwidth);

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
hal_if_set_bw (char *ifname, unsigned int ifindex, unsigned int bandwidth);

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
hal_if_set_autonego (char *ifname, unsigned int ifindex, int autonego);

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
                   unsigned char *hwaddr, int *hwaddr_len);

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
                   u_int8_t *hwaddr, int hwlen);

/*
  Name: hal_if_sec_hwaddrs_set

  Description:
  This API sets the list of secondary MAC addresses for a 
  interface.

  Parameters:
  IN -> ifname  - interface name
  IN -> ifindex - interface index
  IN -> hw_addr_len - length of address
  IN -> nAddr s - number of MAC addresses
  IN -> addresses - array of MAC addresses

  Returns:
  < 0 on error
  HAL_SUCCESS;
*/
int
hal_if_sec_hwaddrs_set (char *ifname, unsigned int ifindex,
                        int hw_addr_len, int nAddrs, unsigned char **addresses);
  
/*
  Name: hal_if_sec_hwaddrs_add

  Description:
  This API adds the secondary hardware addresses to the list of MAC addresses
  for a interface.

  Parameters:
  IN -> ifname  - interface name
  IN -> ifindex - interface index
  IN -> hw_addr_len - length of address
  IN -> nAddr s - number of MAC addresses
  IN -> addresses - array of MAC addresses

  Returns:
  < 0 on error
  HAL_SUCCESS;
*/
int
hal_if_sec_hwaddrs_add (char *ifname, unsigned int ifindex,
                        int hw_addr_len, int nAddrs, unsigned char **addresses);

/*
  Name: hal_if_sec_hwaddrs_delete

  Description:
  This API deletes the secondary hardware addresses from the
  list of receive MAC addresses for a interface.

  Parameters:
  IN -> ifname  - interface name
  IN -> ifindex - interface index
  IN -> hw_addr_len - length of address
  IN -> nAddr s - number of MAC addresses
  IN -> addresses - array of MAC addresses

  Returns:
  < 0 on error
  HAL_SUCCESS;
*/
int
hal_if_sec_hwaddrs__delete (char *ifname, unsigned int ifindex,
                            int hw_addr_len, int nAddrs, unsigned char **addresses);
  
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
hal_if_flags_get (char *ifname, unsigned int ifindex, u_int32_t *flags);

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
hal_if_flags_set (char *ifname, unsigned int ifindex, unsigned int flags);

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
hal_if_flags_unset (char *ifname, unsigned int ifindex, unsigned int flags);

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
hal_if_set_port_type (char *ifname, unsigned int ifindex, 
                      enum hal_if_port_type type, unsigned int *retifindex);

#ifdef HAVE_L3
/* 
   Name: hal_if_bind_fib

   Description:
   This API is called to bind a interface to a FIB fib_id in the forwarding 
   plane. 

   Parameters:
   IN -> ifindex - ifindex of interface
   IN -> fib     - fib id

   Results:
   HAL_SUCCESS
   < 0 on error
*/
int hal_if_bind_fib (u_int32_t ifindex, u_int32_t fib);

/* 
   Name: hal_if_unbind_fib

   Description:
   This API is called to unbind an interface from  FIB fib_id in the forwarding 
   plane. 

   Parameters:
   IN -> ifindex - ifindex of interface
   IN -> fib     - fib id

   Results:
   HAL_SUCCESS
   < 0 on error
*/
int hal_if_unbind_fib (u_int32_t ifindex, u_int32_t fib);
#endif /* HAVE_L3 */

/*
  Name: hal_if_svi_create

  Description:
  This API creates a SVI(Switch Virtual Interface) for a specific VLAN. The
  the VLAN information is embedded in the name of the interface.

  Parameters:
  IN -> name - interface name
  OUT -> ifindex - ifindex for the interface

  Returns:
  < 0 on error
  HAL_SUCCESS
*/
int
hal_if_svi_create (char *ifname, unsigned int *ifindex);

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
hal_if_svi_delete (char *ifname, unsigned int ifindex);

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
hal_if_get_counters(unsigned int ifindex, struct hal_if_counters *if_stats);

/*
   Name: hal_if_set_mdix

   Description: This API sets mdix value for an interface for specific ifindex.
  
   Parameters: 
   IN -> ifindex - Interface index.
   IN -> mdix - mdix crossover for interface.

   Returns:
  Returns:
  < 0 on error
  HAL_SUCCESS
*/
int
hal_if_set_mdix(unsigned int ifindex, unsigned int mdix);

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
hal_if_get_list (void);

/*
  Name: hal_if_delete_done

  Description:
  This API deletes interface from the interface manager after getting
  acknowledgement from the protocol modules.

  Parameters:
  IN -> name - interface name
  IN -> ifindex - ifindex

  Returns:
  < 0 on error
  HAL_SUCCESS
*/
int
hal_if_delete_done(char *ifname, u_int16_t ifindex);

int 
hal_if_set_port_egress (unsigned int ifindex, int egress_mode);


int
hal_if_set_portbased_vlan (unsigned int ifindex, struct hal_port_map bitmap);


int 
hal_if_set_cpu_default_vid (unsigned int ifindex, int vid);

int
hal_if_set_wayside_default_vid (unsigned int ifindex, int vid);

int
hal_if_set_force_vlan (unsigned int ifindex, int vid);


int
hal_if_set_ether_type (unsigned int ifindex, u_int16_t etype);

int
hal_if_set_learn_disable (unsigned int ifindex, int enable);

int
hal_if_get_learn_disable (unsigned int ifindex, int* enable);

int
hal_if_set_sw_reset ();

int
hal_if_set_preserve_ce_cos (unsigned int ifindex);

/*
  Name: hal_if_sec_hwaddrs_delete

  Description:
  This API deletes the secondary hardware addresses from the
  list of receive MAC addresses for a interface.

  Parameters:
  IN -> ifname  - interface name
  IN -> ifindex - interface index
  IN -> hw_addr_len - length of address
  IN -> nAddr s - number of MAC addresses
  IN -> addresses - array of MAC addresses

  Returns:
  < 0 on error
  HAL_SUCCESS;
*/

int
hal_if_sec_hwaddrs_delete (char *ifname, unsigned int ifindex,
                           int hw_addr_len, int nAddrs,
                           unsigned char **addresses);

#define HAL_IP_ACL_NAME_SIZE               20
#define HAL_IP_MAX_ACL_FILTER              30

#define HAL_IP_ACL_DIRECTION_INGRESS       1   /* Incoming */
#define HAL_IP_ACL_DIRECTION_EGRESS        2   /* Outgoing */


/* Definition for ip access group structure for communicating with HSL */
struct hal_filter_common
{
  /* Common access-list */
  int extended;
  uint32_t  addr;
  uint32_t addr_mask;
  uint32_t mask;
  uint32_t mask_mask;
};


struct hal_ip_filter_list
{
    int type;    /* permit/deny*/
    union 
    {
        struct hal_filter_common     ipfilter;
    }ace;
};

struct hal_ip_access_grp
{
    char name[HAL_IP_ACL_NAME_SIZE];
    int ace_number;
    struct hal_ip_filter_list hfilter[HAL_IP_MAX_ACL_FILTER];
};

int
hal_ip_set_access_group (struct hal_ip_access_grp access_grp,
                         char *ifname, int action, int dir);

int
hal_ip_set_access_group_interface (struct hal_ip_access_grp access_grp,
                                   char *vifname, char *ifname,
                                   int action, int dir);
int
hal_vlan_if_get_counters(unsigned int ifindex,unsigned int vlan, 
                         struct hal_vlan_if_counters *if_stats);

int
hal_if_stats_flush (u_int16_t ifindex);

int hal_if_clear_counters (unsigned int ifindex);

int 
hal_if_set_l3_enable_status (int l3_status);

#ifdef HAVE_IPV6
int hal_if_set_ipv6_l3_enable_status (int l3_status);
#endif

#endif /* _HAL_IF_H_ */
