/* Copyright (C) 2004  All Rights Reserved. */

#ifndef _HSL_IFMGR_OS_H_
#define _HSL_IFMGR_OS_H_

/*
  HSL Interface Manager registration/deregistration routines for callbacks.
*/
extern int hsl_if_os_cb_init (void);
extern int hsl_if_os_cb_deinit (void);
 
/* 
   HSL Interface Manager OS interface APIs. These APIs are to be
   implemented for each OS component. 
*/
struct hsl_ifmgr_os_callbacks
{
  /* Interface manager OS initialization. */
  int (*os_if_init) (void);

  /* Interface manager OS deinitialization. */
  int (*os_if_deinit) (void);

  /* Dump. */
  void (*os_if_dump) (struct hsl_if *ifp);

  /* Set L2 port flags. 

     Parameters:
     IN - interface pointer
     IN -> flags - flags
  */
  int (*os_l2_if_flags_set) (struct hsl_if *ifp, unsigned long flags);

  /* Unset L2 port flags. 

     Parameters:
     IN -> interface pointer
     IN -> flags - flags
  */
  int (*os_l2_if_flags_unset) (struct hsl_if *ifp, unsigned long flags);

  /* Create L3 interface.

     Parameters:
     IN -> name - interface name
     IN -> hwaddr - hardware address
     IN -> hwaddrlen - hardware address length
     OUT -> ifindex - interface index of the OS L3 interface

     Returns:
     OS L3 interface pointer as void *
     NULL on error
  */
  void *(*os_l3_if_configure) (struct hsl_if *ifp, char *name, u_char *hwaddr,
                               int hwaddrlen, hsl_ifIndex_t *ifindex);

  /* Delete L3 interface.

     Parameters:
     IN -> interface pointer

     Returns:
     0 on success
     < 0 on error
  */
  int (*os_l3_if_unconfigure) (struct hsl_if *ifp);

  /* Set MTU for interface.

     Parameters:
     IN -> ifp - interface pointer
     IN -> mtu - mtu

     Returns:
     0 on success
     < 0 on error
  */
  int (*os_l3_if_mtu_set) (struct hsl_if *ifp, int mtu);

  /* Set DUPLEX for interface.

     Parameters:
     IN -> ifp - interface pointer
     IN -> duplex - duplex

     Returns:
     0 on success
     < 0 on error
  */
  int (*os_l3_if_duplex_set) (struct hsl_if *ifp, int duplex);

  /* Set AUTONEGO for interface.

     Parameters:
     IN -> ifp - interface pointer
     IN -> autonego - autonego

     Returns:
     0 on success
     < 0 on error
  */
  int (*os_l3_if_autonego_set) (struct hsl_if *ifp, int autonego);

  /* Set HW address for a interface.

     Parameters:
     IN -> ifp - interface pointer
     IN -> hwadderlen - address length
     IN -> hwaddr - address
  */
  int (*os_l3_if_hwaddr_set) (struct hsl_if *ifp, int hwaddrlen, u_char *hwaddr);

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
  int (*os_if_secondary_hwaddrs_set) (struct hsl_if *ifp, int hwaddrlen, int num, u_char **addresses);

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
  int (*os_if_secondary_hwaddrs_add) (struct hsl_if *ifp, int hwaddrlen, int num, u_char **addresses);

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
  int (*os_if_secondary_hwaddrs_delete) (struct hsl_if *ifp, int hwaddrlen, int num, u_char **addresses);

  /* Set L3 port flags. 

     Parameters:
     IN -> interface pointer
     IN -> flags - flags
  */
  int (*os_l3_if_flags_set) (struct hsl_if *ifp, unsigned long flags);

  /* Unset L3 port flags. 

     Parameters:
     IN -> interface pointer
     IN -> flags - flags
  */
  int (*os_l3_if_flags_unset) (struct hsl_if *ifp, unsigned long flags);

  /* Add a IP address to the interface.

     Parameters:
     IN -> ifp - interface pointer
     IN -> prefix - interface address and prefix
     IN -> flags - flags

     Returns:
     0 on success
     < 0 on error
  */
  int (*os_l3_if_address_add) (struct hsl_if *ifp,
                               hsl_prefix_t *prefix, u_char flags);

  /* Delete a IP address from the interface. 

     Parameters:
     IN -> ifp - interface pointer
     IN -> prefix - interface address and prefix

     Returns:
     0 on success
     < 0 on error
  */
  int (*os_l3_if_address_delete) (struct hsl_if *ifp,
                                  hsl_prefix_t *prefix);

  /* Get interface MAC counters.

     Parameters:
     INOUT -> ifp - interface pointer

     Returns:
     0 on success
     < 0 on error
  */
  int (*os_if_get_counters) (struct hsl_if *ifp); 

  /* Set interface speed 

     Parameters:
     IN -> ifp - interface pointer
     IN -> speed - interface speed

     Returns:
     0 on success
     < 0 on error
  */
  int (*os_if_bandwidth_set) (struct hsl_if *ifp,u_int32_t bandwidth); 

#ifdef HAVE_L3

  /* Bind a interface to a FIB

     Parameters:
     IN -> ifp - interface pointer
     IN -> fib_id - FIB id 

     Returns:
     0 on success
     < 0 on error
  */
  int (*os_l3_if_bind_fib) (struct hsl_if *ifp,
                                  hsl_fib_id_t fib_id);

  /* Unbind a interface from a FIB

     Parameters:
     IN -> ifp - interface pointer
     IN -> fib_id - FIB id 

     Returns:
     0 on success
     < 0 on error
  */
  int (*os_l3_if_unbind_fib) (struct hsl_if *ifp,
                                  hsl_fib_id_t fib_id);

#endif /* HAVE_L3 */
};

#endif /* _HSL_IF_OS_H_ */
