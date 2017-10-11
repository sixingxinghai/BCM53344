/* Copyright (C) 2004  All Rights Reserved. */

#ifndef _PACOS_NSM_RATELIMIT_H
#define _PACOS_NSM_RATELIMIT_H

#define NSM_NO_RATELIMIT_LEVEL 100
#define NSM_ZERO_FRACTION      0

struct nsm_ratelimit_stats {
  int bcast_discards;
  int mcast_discards;
  int dlf_bcast_discards;
};

typedef enum nsm_ratelimit_type 
{
  NSM_BROADCAST_RATELIMIT = 1,
  NSM_MULTICAST_RATELIMIT = 2,
  NSM_BROADCAST_MULTICAST_RATELIMIT = 3,
  NSM_DLF_RATELIMIT = 4,
  NSM_ONLY_BROADCAST_RATELIMIT = 5
} nsm_ratelimit_type_t;

struct nsm_bridge_port;

/* Initialise the rate limit functionality */
void nsm_ratelimit_init(struct lib_globals *zg);

/* Write the rate limit configuration information */
int nsm_ratelimit_write(struct cli *, int);

/* Get the broadcast rate limit discards */
int nsm_broadcast_ratelimit_get (struct interface *ifp);

/* Set the broadcast rate limit level.
   type  - Type to be set, Broadcast, Multicasr, DLF.
   level - Percentage of max rate to be set */
int nsm_ratelimit_set (struct interface *ifp, u_int8_t level, 
                       u_int8_t fraction, nsm_ratelimit_type_t type);

/* Get teh broadcast rate limit level. */
int nsm_ratelimit_get_bcast_level (struct interface *ifp);

/* Get the broadcast rate limit discards */
int nsm_multicast_ratelimit_get (struct interface *ifp);

/* Get the broadcast rate limit discards */
int nsm_dlf_broadcast_ratelimit_get (struct interface *ifp);

/* Initialise Rate Limit values */
int nsm_ratelimit_if_init (struct interface *ifp);

/* Deinitialise Rate Limit Values */
int nsm_ratelimit_if_deinit (struct interface *ifp);
#endif /* _PACOS_NSM_L2_BCASTSUP_H */

