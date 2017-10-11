/* Copyright (C) 2004  All Rights Reserved. */

#ifndef _HSL_FIB_MC_HW_H_
#define _HSL_FIB_MC_HW_H_

/* 
   FIB HW callbacks.
*/
struct hsl_fib_mc_hw_callbacks
{

  /* Add ipv4 multicast route 
     Parameters:
     IN -> p_grp_src_entry - Group, Source entry information. 

     Returns:
     0 on success
     < 0 on error
  */
  int (*hw_ipv4_mc_route_add) (struct hsl_mc_fib_entry *p_grp_src_entry);


  /* Delete ipv4 multicast route 
     Parameters:
     IN -> p_grp_src_entry - Group, Source entry information. 

     Returns:
     0 on success
     < 0 on error
  */
  int (*hw_ipv4_mc_route_del) (struct hsl_mc_fib_entry *p_grp_src_entry);

#ifdef HAVE_L2

  /* Delete ipv4 multicast route 
     Parameters:
     IN -> p_grp_src_entry - Group, Source entry information. 
     IN -> vid - Vlan id being removed. 

     Returns:
     0 on success
     < 0 on error
  */
  int (*hw_l2_mc_route_del) (struct hsl_mc_fib_entry *p_grp_src_entry, hsl_vid_t vid);
#endif /* HAVE_L2 */


#ifdef  HAVE_MCAST_IPV4

  /* Init ipv4 multicast.  

     Returns:
     0 on success
     < 0 on error
  */
  int (*hw_ipv4_mc_init)();

  /* Deinit ipv4 multicast.  

     Returns:
     0 on success
     < 0 on error
  */
  int (*hw_ipv4_mc_deinit)();
  /* Get ipv4 multicast route usage  statistics
     Parameters:
     IN -> p_grp_src_entry - Group, Source entry information. 
     INOUT -> pktcnt   - Passed packet count.
     INOUT -> bytecnt  - Passed byte count.
     INOUT -> wrong_if - Wrong interface received. 

     Returns:
     0 on success
     < 0 on error
  */
  int (*hw_ipv4_mc_sg_stat) (struct hsl_mc_fib_entry *p_grp_src_entry,
                             u_int32_t *pktcnt,u_int32_t *bytecnt, u_int32_t *wrong_if);



  /* Add ipv4 multicast interface.

     Parameters:
     IN -> ifp      - interface pointer
     IN -> loc_addr - local address.
     IN -> rmt_addr - remote address.
     IN -> flags    - flags. 

     Returns:
     0 on success
     < 0 on error
  */
  int (*hw_ipv4_mc_vif_add) (struct hsl_if *ifp, u_int16_t flags); 

  /* Delete ipv4 multicast interface.

     Parameters:
     IN -> ifp      - interface pointer

     Returns:
     0 on success
     < 0 on error
  */
  int (*hw_ipv4_mc_vif_del) (struct hsl_if *ifp);


#endif /* HAVE_MCAST_IPV4 */

};

/* 
   IPv6 MC FIB HW callbacks.
*/
struct hsl_fib_ipv6_mc_hw_callbacks
{
  /* Add ipv6 multicast route 
     Parameters:
     IN -> p_grp_src_entry - G,S information.
     IN -> iif_vif     - Incoming interface.
     IN -> num_olist   - Number of outgoing interfaces.
     IN -> olist  - Outgoing  vif indices. 

     Returns:
     0 on success
     < 0 on error
  */
  int (*hw_ipv6_mc_route_add) (struct hsl_mc_fib_entry *p_grp_src_entry);


  /* Delete ipv6 multicast route 
     Parameters:
     IN -> p_grp_src_entry - G,S information.

     Returns:
     0 on success
     < 0 on error
  */
  int (*hw_ipv6_mc_route_del) (struct hsl_mc_fib_entry *p_grp_src_entry);

#ifdef HAVE_L2

  /* Delete ipv4 multicast route 
     Parameters:
     IN -> p_grp_src_entry - Group, Source entry information. 
     IN -> vid - Vlan id being removed. 

     Returns:
     0 on success
     < 0 on error
  */
  int (*hw_l2_mc_route_del) (struct hsl_mc_fib_entry *p_grp_src_entry, hsl_vid_t vid);
#endif /* HAVE_L2 */
#ifdef  HAVE_MCAST_IPV6

  /* Init ipv6 multicast.  

     Returns:
     0 on success
     < 0 on error
  */
  int (*hw_ipv6_mc_init)();

  /* Deinit ipv6 multicast.  

     Returns:
     0 on success
     < 0 on error
  */
  int (*hw_ipv6_mc_deinit)();

  /* Get ipv6 multicast route usage  statistics
     Parameters:
     IN -> p_grp_src_entry - G,S information.
     INOUT -> pktcnt   - Passed packet count.
     INOUT -> bytecnt  - Passed byte count.
     INOUT -> wrong_if - Wrong interface received. 

     Returns:
     0 on success
     < 0 on error
  */
  int (*hw_ipv6_mc_sg_stat) (struct hsl_mc_fib_entry *p_grp_src_entry, 
      u_int32_t *pktcnt, u_int32_t *bytecnt, 
      u_int32_t *wrong_if);



  /* Add ipv6 multicast interface.

     Parameters:
     IN -> ifp      - interface pointer
     IN -> flags    - flags. 

     Returns:
     0 on success
     < 0 on error
  */
  int (*hw_ipv6_mc_vif_add) (struct hsl_if *ifp, u_int16_t flags); 

  /* Delete ipv6 multicast interface.

     Parameters:
     IN -> ifp      - interface pointer

     Returns:
     0 on success
     < 0 on error
  */
  int (*hw_ipv6_mc_vif_del) (struct hsl_if *ifp);
#endif /* HAVE_MCAST_IPV6 */
};

#endif /* _HSL_FIB_MC_HW_H_ */
