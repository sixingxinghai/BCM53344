/* Copyright (C) 2004  All Rights Reserved. */

#ifndef _HSL_FIB_MC_OS_H_
#define _HSL_FIB_MC_OS_H_

/* 
   FIB OS callbacks.
*/
struct hsl_fib_mc_os_callbacks
{
#ifdef  HAVE_MCAST_IPV4
  /* Enable pim 

     Returns:
     0 on success
     < 0 on error
  */
  int (*os_ipv4_mc_pim_init) ();

  /* Disable pim 

     Returns:
     0 on success
     < 0 on error
  */
  int (*os_ipv4_mc_pim_deinit) ();

  /* Add ipv4 multicast route 
     Parameters:
     IN -> src         - Source address.
     IN -> group       - Mcast group.
     IN -> iif_vif     - Incoming interface.
     IN -> num_olist   - Number of outgoing interfaces.
     IN -> olist_ttls  - Outgoing interfaces ttls. 

     Returns:
     0 on success
     < 0 on error
  */
  int (*os_ipv4_mc_route_add) (hsl_ipv4Address_t src,hsl_ipv4Address_t group,
                               u_int32_t iif_vif,u_int32_t num_olist,u_char *olist_ttls);

  /* Delete ipv4 multicast route 
     Parameters:
     IN -> src         - Source address.
     IN -> group       - Mcast group.

     Returns:
     0 on success
     < 0 on error
  */
  int (*os_ipv4_mc_route_del) (hsl_ipv4Address_t src,hsl_ipv4Address_t group);


  /* Add vif to os. 

     Parameters:
     IN -> vif_index  - mcast interface index. 
     IN -> loc_addr   - Local address.
     IN -> rmt_addr   - Remote address.  
     IN -> flags      - Vif flags. 
 
     Returns:
     0 on success
     < 0 on error
  */
  int (*os_ipv4_mc_vif_add) (u_int32_t vif_index, hsl_ipv4Address_t loc_addr,
                                hsl_ipv4Address_t rmt_addr, u_int16_t flags); 

  /* Delete vif from os. 

     Parameters:
     IN -> vif_index  - mcast interface index. 

     Returns:
     0 on success
     < 0 on error
  */
  int (*os_ipv4_mc_vif_del) (u_int32_t vif_index);

#endif /* HAVE_MCAST_IPV4 */

};

/* 
   IPv6 MC FIB OS callbacks.
*/
struct hsl_fib_ipv6_mc_os_callbacks
{
#ifdef  HAVE_MCAST_IPV6
  /* Enable pim 

     Returns:
     0 on success
     < 0 on error
  */
  int (*os_ipv6_mc_pim_init) ();

  /* Disable pim 

     Returns:
     0 on success
     < 0 on error
  */
  int (*os_ipv6_mc_pim_deinit) ();

  /* Add ipv6 multicast route 
     Parameters:
     IN -> src         - Source address.
     IN -> group       - Mcast group.
     IN -> iif_vif     - Incoming interface.
     IN -> num_olist   - Number of outgoing interfaces.
     IN -> olist       - Outgoing vif indices. 

     Returns:
     0 on success
     < 0 on error
  */
  int (*os_ipv6_mc_route_add) (hsl_ipv6Address_t *src, hsl_ipv6Address_t *group,
                               u_int32_t iif_vif, u_int32_t num_olist, 
                               u_int16_t *olist);

  /* Delete ipv6 multicast route 
     Parameters:
     IN -> src         - Source address.
     IN -> group       - Mcast group.

     Returns:
     0 on success
     < 0 on error
  */
  int (*os_ipv6_mc_route_del) (hsl_ipv6Address_t *src, hsl_ipv6Address_t *group);


  /* Add vif to os. 

     Parameters:
     IN -> vif_index  - mcast vif index. 
     IN -> ifindex    - interface index. 
     IN -> flags      - Vif flags. 
 
     Returns:
     0 on success
     < 0 on error
  */
  int (*os_ipv6_mc_vif_add) (u_int32_t vif_index, u_int32_t ifindex, 
      u_int16_t flags); 

  /* Delete vif from os. 

     Parameters:
     IN -> vif_index  - mcast interface index. 

     Returns:
     0 on success
     < 0 on error
  */
  int (*os_ipv6_mc_vif_del) (u_int32_t vif_index);

#endif /* HAVE_MCAST_IPV6 */
};
#endif /* _HSL_FIB_MC_OS_H_ */
