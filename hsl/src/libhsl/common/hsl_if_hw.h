/* Copyright (C) 2004  All Rights Reserved. */

#ifndef _HSL_IF_HW_H_
#define _HSL_IF_HW_H_

/*
  HSL Interface Manager registration/deregistration routines for callbacks.
*/
extern int hsl_if_hw_cb_init (void);
extern int hsl_if_hw_cb_deinit (void);

/* 
   HSL Interface Manager HW interface APIs. These APIs are to be
   implemented for each HW component. 
*/
struct hsl_ifmgr_hw_callbacks
{

  int (*hw_if_init) (void);

  /* Interface manager hardware deinitialization. */
  int (*hw_if_deinit) (void);

  /* Dump. */
  void (*hw_if_dump) (struct hsl_if *ifp);

  /* Unregister L2 port. 

     Parameters:
     IN -> ifp - interface pointer

     Returns:
     0 on success
     < 0 on error
  */
  int (*hw_l2_unregister) (struct hsl_if *ifp);

  /* Set L2 port flags. 

     Parameters:
     IN -> ifp - interface pointer
     IN -> flags - flags

     Returns:
     0 on success
     < 0 on error   
  */
  int (*hw_l2_if_flags_set) (struct hsl_if *ifp, unsigned long flags);

  /* Unset L2 port flags. 

     Parameters:
     IN -> ifp - interface pointer
     IN -> flags - flags

     Returns:
     0 on success
     < 0 on error   
  */
  int (*hw_l2_if_flags_unset) (struct hsl_if *ifp, unsigned long flags);

  /* Set packet types acceptable from this port.

     Parameters:
     IN -> ifp - interface pointer
     IN -> pkt_flags

     Returns:
     0 on success
     < 0 on error   
  */
  int (*hw_if_packet_types_set) (struct hsl_if *ifp, unsigned long pkt_flags);

  /* Unset packet types acceptable from this port.

     Parameters:
     IN -> ifp - interface pointer
     IN -> pkt_flags

     Returns:
     0 on success
     < 0 on error   
  */
  int (*hw_if_packet_types_unset) (struct hsl_if *ifp, unsigned long pkt_flags);

  /* Set MTU for interface.

     Parameters:
     IN -> ifp - interface pointer
     IN -> mtu - mtu

     Returns:
     0 on success
     < 0 on error
  */

  int (*hw_if_mtu_set) (struct hsl_if *ifp, int mtu);


  /*  Add/Remove members for Portbased vlan group
      Paramters :
      IN -> ifp - interface pointer
      IN -> pbitmap - Bitmap for ports to be added/removed
      IN -> status -  operation status for add /remove
      Returns:
      0 on success
      < 0 on error

  */
  int (*hw_if_portbased_vlan) (struct hsl_if *ifp, struct hal_port_map pbitmap);
                                                      
  /*
      Set cpu port default vlan id
      Parameters :
      IN -> vid - vlan id
      Returns:
      0 on success
      < 0 on error
  */

  int (*hw_if_cpu_default_vlan) (int vid);
 
  /*
      Set wayside port default vlan id
      Parameters :
      IN -> vid - vlan id
      Returns:
      0 on success
      < 0 on error
  */

  int (*hw_if_wayside_default_vlan) (int vid);  

  /*
     Preserve ce cos
     Parameters :
     IN - ifp - interface pointer
     Returns :
     0 on success
     < 0 on error
  */

  int (*hw_if_preserve_ce_cos)(struct hsl_if *ifp);

  /*   
      Set port egress mode    
      Parameters :
      IN -> ifp    - interface pointer
      IN -> egress - egress mode
      Returns:
      0 on success
      < 0 on error 
  */

  int (*hw_if_port_egress) (struct hsl_if *ifp,
                            int egress);


  /*  Set Force Vlan 
    
      parameters:
      IN -> ifp - interface pointer
      IN -> vid - VLAN id

      Returns:
      0 on success
      < 0 on error
  */
  int (*hw_if_set_force_vlan) (struct hsl_if *ifp, int vid);



  /*  Set Force Vlan

      parameters:
      IN -> ifp - interface pointer
      IN -> etype - ethernet type

      Returns:
      0 on success
      < 0 on error
  */
  int (*hw_if_set_ether_type) (struct hsl_if *ifp, u_int16_t etype);



  /*  Set Force Vlan

      parameters:
      IN -> ifp    - interface pointer
      IN -> enable - learn_disable enable/disable
      IN -> flag   - indicates set/get for  backend function 

      Returns:
      0 on success
      < 0 on error
  */
  int (*hw_if_learn_disable) (struct hsl_if *ifp, int *enable,
                              int flag);

   
  /*  Set Sw Reset
      Returns:
      0 on success
      < 0 on error
  */ 
  int (*hw_if_set_sw_reset) (void); 
  

  /* Set MTU for L3 interface.

     Parameters:
     IN -> ifp - interface pointer
     IN -> mtu - mtu

     Returns:
     0 on success
     < 0 on error
  */
  int (*hw_if_l3_mtu_set) (struct hsl_if *ifp, int mtu);

  /* Set DUPLEX for interface.

     Parameters:
     IN -> ifp - interface pointer
     IN -> duplex - duplex

     Returns:
     0 on success
     < 0 on error
  */
  int (*hw_if_duplex_set) (struct hsl_if *ifp, int duplex);

  /* Set AUTONEGO for interface.

     Parameters:
     IN -> ifp - interface pointer
     IN -> autonego - autonego

     Returns:
     0 on success
     < 0 on error
  */
  int (*hw_if_autonego_set) (struct hsl_if *ifp, int autonego);

  /* Set BANDWIDTH for interface.

     Parameters:
     IN -> ifp - interface pointer
     IN -> bandwidth - bandwidth

     Returns:
     0 on success
     < 0 on error
  */
  int (*hw_if_bandwidth_set) (struct hsl_if *ifp, u_int32_t bandwidth);

  /* Set HW address for a interface.

     Parameters:
     IN -> ifp - interface pointer
     IN -> hwadderlen - address length
     IN -> hwaddr - address

     Returns:
     0 on success
     < 0 on error
  */
  int (*hw_if_hwaddr_set) (struct hsl_if *ifp, int hwaddrlen, u_char *hwaddr);

  /* Set secondary HW addresses for a interface.

     Parameters:
     IN -> ifp - interface pointer
     IN -> hwaddrlen - address length
     IN -> num - number of secondary addresses
     IN -> addresses - array of secondary addresses

     Returns:
     0 on success
     < 0 on error
  */
  int (*hw_if_secondary_hwaddrs_set) (struct hsl_if *ifp, int hwaddrlen, int num, u_char **addresses);

  /* Add secondary HW addresses for a interface.

     Parameters:
     IN -> ifp - interface pointer
     IN -> hwaddrlen - address length
     IN -> num - number of secondary addresses
     IN -> addresses - array of secondary addresses

     Returns:
     0 on success
     < 0 on error
  */
  int (*hw_if_secondary_hwaddrs_add) (struct hsl_if *ifp, int hwaddrlen, int num, u_char **addresses);

  /* Delete secondary HW addresses for a interface.

     Parameters:
     IN -> ifp - interface pointer
     IN -> hwaddrlen - address length
     IN -> num - number of secondary addresses
     IN -> addresses - array of secondary addresses

     Returns:
     0 on success
     < 0 on error
  */
  int (*hw_if_secondary_hwaddrs_delete) (struct hsl_if *ifp, int hwaddrlen, int num, u_char **addresses);

  /* Create L3 interface.

     Parameters:
     IN -> ifp - interface pointer
     IN -> data - system specific data
     
     Returns:
     HW L3 interface pointer as void *
     NULL on error
  */
  void *(*hw_l3_if_configure) (struct hsl_if *ifp, void *data);

  /* Perform any post configuration. This can typically be done
     after some interface binding is performed.

     Parameters:
     IN -> ifp - interface pointer
     IN -> ifp - interface pointer

     Returns:
     0 on success
     < 0 on error
  */
  int (*hw_if_post_configure) (struct hsl_if *ifpp, struct hsl_if *ifpc);

  /* Perform any pre unconfiguration. This can typically be done
     before some interface unbinding is performed.

     Parameters:
     IN -> ifp - interface pointer
     IN -> ifp - interface pointer

     Returns:
     0 on success
     < 0 on error
  */
  int (*hw_if_pre_unconfigure) (struct hsl_if *ifpp, struct hsl_if *ifpc);

  /* Delete L3 interface.

     Parameters:
     IN -> ifp - interface pointer

     Returns:
     0 on success
     < 0 on error
  */
  int (*hw_l3_if_unconfigure) (struct hsl_if *ifp);

  /* Set switching type for a port.

     Parameters:
     IN -> ifp 

     Returns:
     0 on success
     < 0 on error
  */
  int (*hw_set_switching_type) (struct hsl_if *ifp, hsl_IfSwitchType_t type);

  /* Set L3 port flags. 

     Parameters:
     IN -> ifp - interface pointer
     IN -> flags - flags

     Returns:
     0 on success
     < 0 on error   
  */
  int (*hw_l3_if_flags_set) (struct hsl_if *ifp, unsigned long flags);

  /* Unset L3 port flags. 

     Parameters:
     IN -> ifp - interface pointer
     IN -> flags - flags

     Returns:
     0 on success
     < 0 on error
  */
  int (*hw_l3_if_flags_unset) (struct hsl_if *ifp, unsigned long flags);

  /* Add a IP address to the interface.

     Parameters:
     IN -> ifp - interface pointer
     IN -> prefix - interface address and prefix
     IN -> flags - flags

     Returns:
     0 on success
     < 0 on error
  */
  int (*hw_l3_if_address_add) (struct hsl_if *ifp, 
                               hsl_prefix_t *prefix, u_char flags);

  /* Delete a IP address from the interface. 

     Parameters:
     IN -> ifp - interface pointer
     IN -> prefix - interface address and prefix

     Returns:
     0 on success
     < 0 on error
  */
  int (*hw_l3_if_address_delete) (struct hsl_if *ifp,
                                  hsl_prefix_t *prefix);

  
  /* Get interface MAC counters.

     Parameters:
     INOUT -> ifp - interface pointer

     Returns:
     0 on success
     < 0 on error
  */
  int (*hw_if_get_counters) (struct hsl_if *ifp); 

/* Clear the Interface Counters.
   
   Parameter 
   INOUT -> ifp - interface pointer

   Returns:
   0 in success
   < 0 in error
*/
  int (*hw_if_clear_counters) (struct hsl_if *ifp);

/* MDIX crossover.

   Parameter
   IN -> ifp - interface pointer
   IN -> mdix - MDIX crossover value
   Returns:
   0 in success
   < 0 in error
*/

  int (*hw_if_mdix_set) (struct hsl_if *ifp, int mdix);

#ifdef HAVE_L3

  /* Bind a interface to a FIB

     Parameters:
     IN -> ifp - interface pointer
     IN -> fib_id - FIB id 

     Returns:
     0 on success
     < 0 on error
  */
  int (*hw_l3_if_bind_fib) (struct hsl_if *ifp,
                                  hsl_fib_id_t fib_id);

  /* Unbind a interface from a FIB

     Parameters:
     IN -> ifp - interface pointer
     IN -> fib_id - FIB id 

     Returns:
     0 on success
     < 0 on error
  */
  int (*hw_l3_if_unbind_fib) (struct hsl_if *ifp,
                                  hsl_fib_id_t fib_id);

  /* Init port mirroring. 

     Parameters:
     void

     Returns:
     0 on success
     < 0 on error
  */

#endif /* HAVE_L3 */

  int (*hw_if_init_portmirror) (void);

  /* Deinit port mirroring. 

     Parameters:
     void

     Returns:
     0 on success
     < 0 on error
  */
  int (*hw_if_deinit_portmirror) (void);


  /* Set port mirroring.

     Parameters:
     IN -> ifp - mirroring interface
     IN -> ifp - mirrored  interface
     IN -> direction - mirrored traffic direction

     Returns:
     0 on success
     < 0 on error
  */
  int (*hw_if_set_portmirror) (struct hsl_if *ifp, struct hsl_if *ifp2, enum hal_port_mirror_direction direction);

  /* Unset port mirroring. 

     Parameters:
     IN -> ifp - mirroring interface
     IN -> ifp - mirrored  interface
     IN -> direction - mirrored traffic direction

     Returns:
     0 on success
     < 0 on error
  */
  int (*hw_if_unset_portmirror) (struct hsl_if *ifp, struct hsl_if *ifp2, enum hal_port_mirror_direction direction);
#ifdef HAVE_LACPD
  /* Set port selection criteria for aggregator. 

     Parameters:
     IN -> ifp - aggregator interface
     IN -> psc - port selection criteria.

     Returns:
     0 on success
     < 0 on error
  */
  int (*hw_if_lacp_psc_set) (struct hsl_if *ifp, int psc);

  /* Add aggregator. 

     Parameters:
     IN -> agg_name - aggregator name. 
     IN -> agg_mac  - aggregator hw address.
     IN -> agg_type - aggregator type.

     Returns:
     0 on success
     < 0 on error
  */
  int (*hw_if_lacp_agg_add) (struct hsl_if *ifp, int agg_type);

  /* Delete aggregator. 

     Parameters:
     IN -> ifp - aggregator interface. 

     Returns:
     0 on success
     < 0 on error
  */
  int (*hw_if_lacp_agg_del) (struct hsl_if *ifp);

  /* Attach port to aggregator. 

     Parameters:
     IN -> agg_ifp - aggregator interface. 
     IN -> port_ifp - port interface. 

     Returns:
     0 on success
     < 0 on error
  */
  int (*hw_if_lacp_agg_port_attach) (struct hsl_if *agg_ifp,struct hsl_if *port_ifp);

  /* Detach port from aggregator. 

     Parameters:
     IN -> agg_ifp - aggregator interface. 
     IN -> port_ifp - port interface. 

     Returns:
     0 on success
     < 0 on error
  */
  int (*hw_if_lacp_agg_port_detach) (struct hsl_if *agg_ifp,struct hsl_if *port_ifp);
#endif /* HAVE_LACPD */

#ifdef HAVE_MPLS
  /* Create MPLS interface.

     Parameters:
     IN -> ifp - interface pointer
     IN -> data - system specific data
     
     Returns:
     HW MPLS L3 interface pointer as void *
     NULL on error
  */
  void *(*hw_mpls_if_configure) (struct hsl_if *ifp, void *data);

  /* Delete MPLS interface.

     Parameters:
     IN -> ifp - interface pointer

     Returns:
     0 on success
     < 0 on error
  */
  int (*hw_mpls_if_unconfigure) (struct hsl_if *ifp);
#endif /* HAVE_MPLS */
};

#endif /* _HSL_IF_HW_H_ */
