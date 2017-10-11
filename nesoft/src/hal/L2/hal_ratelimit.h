/* Copyright (C) 2004  All Rights Reserved. */

#ifndef _HAL_RATELIMIT_H_
#define _HAL_RATELIMIT_H_

/* 
   Name: hal_ratelimit_init

   Description:
   This API initializes rate limiting.

   Parameters:
   None

   Returns:
   HAL_ERR_RATELIMIT_INIT
   HAL_SUCCESS
*/
int
hal_ratelimit_init (void);

/* 
   Name: hal_ratelimit_deinit

   Description:
   This API deinitializes rate limiting.

   Parameters:
   None

   Returns:
   HAL_ERR_RATELIMIT_DEINIT
   HAL_SUCCESS
*/
int
hal_ratelimit_deinit (void);

/* 
   Name: hal_l2_ratelimit_bcast

   Description:
   This API sets the level in percentage of the port bandwidth for broadcast
   storm suppression.

   Parameters:
   IN -> ifindex - port ifindex
   IN -> level - level in percentage of the port bandwidth
   IN -> fraction - fraction level in percentage of the port bandwidth

   Returns:
   HAL_ERR_RATELIMIT_BCAST
   HAL_SUCCESS
*/
int
hal_l2_ratelimit_bcast (unsigned int ifindex,
                        unsigned char level,
                        unsigned char fraction);

/* 
   Name: hal_l2_bcast_discards_get

   Description:
   IN -> ifindex - port ifindex
   OUT -> discards - number of discarded frames

   Returns:
   HAL_ERR_RATELIMIT_BCAST
   HAL_SUCCESS
*/
int 
hal_l2_bcast_discards_get (unsigned int ifindex,
                           unsigned int *discards);

/* 
   Name: hal_l2_ratelimit_mcast

   Description:
   This API sets the level in percentage of the port bandwidth for multicast

   Parameters:
   IN -> ifindex - port ifindex
   IN -> level - level in percentage of the port bandwidth
   IN -> fraction - fraction level in percentage of the port bandwidth

   Returns:
   HAL_ERR_RATELIMIT_MCAST
   HAL_SUCCESS
*/
int
hal_l2_ratelimit_mcast (unsigned int ifindex,
                        unsigned char level,
                        unsigned char fraction);

/* 
   Name: hal_l2_mcast_discards_get

   Description:
   IN -> ifindex - port ifindex
   OUT -> discards - number of discarded frames

   Returns:
   HAL_ERR_RATELIMIT_MCAST
   HAL_SUCCESS
*/
int 
hal_l2_mcast_discards_get (unsigned int ifindex,
                           unsigned int *discards);

/*
  Name: hal_l2_ratelimit_bcast_mcast

   Description:
   This API sets the level in percentage of the port bandwidth for
   Broadcast and multicast.

   Parameters:
   IN -> ifindex - port ifindex
   IN -> level - level in percentage of the port bandwidth

   Returns:
   HAL_ERR_RATELIMIT
   HAL_SUCCESS

*/
int
hal_l2_ratelimit_bcast_mcast (unsigned int ifindex,
                              unsigned char level,
                              unsigned char fraction);
/*
  Name: hal_l2_ratelimit_only_broadcast

   Description:
   This API sets the level in percentage of the port bandwidth for
   Broadcast.

   Parameters:
   IN -> ifindex - port ifindex
   IN -> level - level in percentage of the port bandwidth

   Returns:
   HAL_ERR_RATELIMIT
   HAL_SUCCESS

*/
int
hal_l2_ratelimit_only_broadcast (unsigned int ifindex,
                                 unsigned char level,
                                 unsigned char fraction);

/* 
   Name: hal_l2_ratelimit_dlf_bcast

   Description:
   This API sets the level in percentage of the port bandwidth for 
   dlf(destination lookup failure) broadcast

   Parameters:
   IN -> ifindex - port ifindex
   IN -> level - level in percentage of the port bandwidth
   IN -> fraction - fraction level in percentage of the port bandwidth

   Returns:
   HAL_ERR_RATELIMIT_MCAST
   HAL_SUCCESS
*/
int
hal_l2_ratelimit_dlf_bcast (unsigned int ifindex,
                            unsigned char level,
                            unsigned char fraction);

/* 
   Name: hal_l2_dlf_bcast_discards_get

   Description:
   IN -> ifindex - port ifindex
   OUT -> discards - number of discarded frames

   Returns:
   HAL_ERR_RATELIMIT_MCAST
   HAL_SUCCESS
*/
int 
hal_l2_dlf_bcast_discards_get (unsigned int ifindex,
                               unsigned int *discards);

#endif /* _HAL_RATELIMIT_H_ */
