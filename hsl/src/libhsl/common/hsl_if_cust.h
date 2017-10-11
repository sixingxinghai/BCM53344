/* Copyright (C) 2004  All Rights Reserved. */

#ifndef _HSL_IF_CUST_H_
#define _HSL_IF_CUST_H_

/*
  HSL Interface Manager registration/deregistration routines for callbacks.
*/
extern int hsl_if_cust_cb_init (void);
extern int hsl_if_cust_cb_deinit (void);

/* 
   HSL Interface Manager custom interface APIs. 
   These APIs are to be implemented for each custom component. 
*/
struct hsl_ifmgr_cust_callbacks
{

  /* Allocate ifindex. 

     Parameters:
     IN     -> ifp - interface pointer
     INOUT  -> ifindex
     Returns:
       0 -  Success;
       less than 0 - Error.  
  */
  int (*cust_if_alloc_ifindex) (struct hsl_if *ifp, hsl_ifIndex_t *ifindex);

  /* Free ifindex. 

     Parameters:
     IN  -> ifindex
     Returns:
       0 -  Success;
       less than 0 - Error.  
  */
  int (*cust_if_free_ifindex) (hsl_ifIndex_t ifindex);
};

#endif /* _HSL_IF_CUST_H_ */
