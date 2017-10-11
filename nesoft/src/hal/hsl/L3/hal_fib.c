/* Copyright (C) 2004  All Rights Reserved. */

#include "pal.h"
#include "lib.h"

#include "hal_incl.h"
#include "hal_debug.h"
#include "hal_msg.h"
#include "hal_netlink.h"
#include "hal_comm.h"

extern int hal_comm_initialized;
extern int hal_ecmp_cb (int ecmp);

/* Common function for FIB create/delete. */
static int
_hal_fib (unsigned int fib, int msgtype)
{
  int ret;
  struct hal_nlmsghdr *nlh;
  struct 
  {
    struct hal_nlmsghdr nlh;
    unsigned int msg;
  } req;

  if (! hal_comm_initialized)
    return 0;

  /* Set header. */
  nlh = &req.nlh;
  nlh->nlmsg_len = HAL_NLMSG_LENGTH (sizeof (u_int32_t));
  nlh->nlmsg_flags = HAL_NLM_F_CREATE | HAL_NLM_F_REQUEST;
  nlh->nlmsg_type = msgtype;
  nlh->nlmsg_seq = ++hallink_cmd.seq;

  /* Set message. */
  req.msg = fib;

  /* Send message. */
  ret = hal_talk (&hallink_cmd, nlh, NULL, NULL);
  if (ret < 0)
    return ret;

  return HAL_SUCCESS;
}

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
hal_fib_create (unsigned int fib)
{
  return _hal_fib (fib, HAL_MSG_FIB_CREATE);
}

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
hal_fib_delete (unsigned int fib)
{
  return _hal_fib (fib, HAL_MSG_FIB_DELETE);
}

int
hal_num_multipath_resp(struct hal_nlmsghdr *h, void *data)
{
  int resp;

  if(!h)
    return hal_ecmp_cb (1);
    
  resp = *(int *) HAL_NLMSG_DATA (h);

  hal_ecmp_cb (resp);
  return 0;
}

/* 
   Name: hal_ipv4_init

   Description:
   This API initializes the IPv4 hardware layer component.

   Parameters:
   OUT - maxpaths - ECMP maxpaths supported

   Results:
   < 0 on error
   HAL_SUCCESS
*/
int
hal_ipv4_init (void)
{
  return hal_msg_generic_request (HAL_MSG_IPV4_INIT, NULL, NULL);
}

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
hal_ipv4_deinit (void)
{
  return hal_msg_generic_request (HAL_MSG_IPV4_DEINIT, NULL, NULL);
}

#ifdef HAVE_IPV6
/* 
   Name: hal_ipv6_init

   Description:
   This API initializes the IPv6 hardware layer component.

   Parameters:
   OUT -> maxpaths - max ECMP paths supported

   Results:
   < 0 on error
   HAL_SUCCESS
*/
int
hal_ipv6_init (void)
{
  return hal_msg_generic_request (HAL_MSG_IPV6_INIT, NULL, NULL);
}

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
hal_ipv6_deinit (void)
{
  return hal_msg_generic_request (HAL_MSG_IPV6_DEINIT, NULL, NULL);
}
#endif /* HAVE_IPV6 */

