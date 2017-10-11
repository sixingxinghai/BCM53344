/* Copyright (C) 2004  All Rights Reserved. */

#ifndef _HAL_FIB_H_
#define _HAL_FIB_H_

/* 
   Name: hal_fib_create

   Description:
   This API is called to create a FIB in the forwarding plane for the
   provided fid-id. 

   Parameters:
   IN -> fib - fib id

   Results:
   HAL_FIB_EXISTS
   HAL_SUCCESS
   < 0 on error
*/
int
hal_fib_create (unsigned int fib);

/* 
   Name: hal_fib_delete

   Description:
   This API is called to delete a FIB in the forwarding plane for the 
   provided fib-id.

   Parameters:
   IN -> fib - fib id

   Results:
   HAL_FIB_NOT_EXISTS
   HAL_SUCCESS
   < 0 on error
*/
int
hal_fib_delete (unsigned int fib);

/* 
   Name: hal_ipv4_init

   Description:
   This API initializes the IPv4 hardware layer component.

   Parameters:
     void

   Results:
   < 0 on error
   HAL_SUCCESS
*/
int
hal_ipv4_init ();

/* 
   Name: hal_ipv4_deinit

   Description:
   This API deinitializes the IPv4 hardware layer component.

   Parameters:
   None

   Results:
   < 0 on error
   HAL_SUCCESS
*/
int
hal_ipv4_deinit (void);


#ifdef HAVE_IPV6
/* 
   Name: hal_ipv6_init

   Description:
   This API initializes the IPv6 hardware layer component.

   Parameters:
     void

   Results:
   < 0 on error
   HAL_SUCCESS
*/
int
hal_ipv6_init (void);

/* 
   Name: hal_ipv6_deinit

   Description:
   This API deinitializes the IPv6 hardware layer component.

   Parameters:
   None

   Results:
   < 0 on error
   HAL_SUCCESS
*/
int
hal_ipv6_deinit (void);

#endif /* HAVE_IPV6 */

#endif /* _HAL_IP4_UC_H_ */
