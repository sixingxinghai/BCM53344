/* Copyright (C) 2001-2003  All Rights Reserved. */

#ifndef _PACOS_NSM_RTADV_H
#define _PACOS_NSM_RTADV_H

/* Router Advertisement structure. */
struct rtadv
{
  /* Pointer to NSM master. */
  struct nsm_master *nm;

  /* RA socket. */
  pal_sock_handle_t sock;

  /* Stream buffers.  */
  struct stream *obuf;
  struct stream *ibuf;

  /* Threads. */
  struct thread *t_read;
};

/* Router advertisement parameter. From RFC2461. */
struct rtadv_if
{
  /* Top rtadv structure. */
  struct rtadv *top;

  /* Poniter to the interface structure. */
  struct interface *ifp;

  /* Configuration flags. */
  u_int32_t config;
#define NSM_RTADV_CONFIG_RA_ADVERTISE           (1 << 0)
#define NSM_RTADV_CONFIG_RA_INTERVAL            (1 << 1)
#define NSM_RTADV_CONFIG_ROUTER_LIFETIME        (1 << 2)
#define NSM_RTADV_CONFIG_ROUTER_REACHABLE_TIME  (1 << 3)
#define NSM_RTADV_CONFIG_RA_FLAG_MANAGED        (1 << 4)
#define NSM_RTADV_CONFIG_RA_FLAG_OTHER          (1 << 5)
#define NSM_RTADV_CONFIG_RA_PREFIX              (1 << 6)
#define NSM_RTADV_CONFIG_MIP6_HOME_AGENT        (1 << 7)
#define NSM_RTADV_CONFIG_MIP6_HA_INFO           (1 << 8)
#define NSM_RTADV_CONFIG_MIP6_ADV_INTERVAL      (1 << 9)
#define NSM_RTADV_CONFIG_ROUTER_RETRANS_TIMER      (1 << 10)
#define NSM_RTADV_CONFIG_CUR_HOPLIMIT            (1 << 11)
#define NSM_RTADV_CONFIG_LINKMTU                 (1 << 12)
#define NSM_RTADV_CONFIG_MIN_RA_INTERVAL         (1 << 13)
#define NSM_RTADV_CONFIG_PREFIX_VALID_LIFETIME           (1 << 14)
#define NSM_RTADV_CONFIG_PREFIX_PREFERRED_LIFETIME (1 << 15)
#define NSM_RTADV_CONFIG_PREFIX_OFFLINK                  (1 << 16)
#define NSM_RTADV_CONFIG_PREFIX_NO_AUTOCONFIG    (1 << 17)

  /* Router Advertisement field. */
  u_int16_t router_lifetime;            /* Router lifetime. */
  u_int32_t ra_interval;                /* RA interval. */
  u_int32_t reachable_time;             /* Reachable time. */
  u_int32_t retrans_timer;              /* Retrans timer. */
  u_int8_t curr_hoplimit;                     /* Current Hop Limit */
  u_int32_t min_ra_interval;            /* MIN RA interval.  */
  u_int8_t max_initial_rtr_advert;    /* RA send probes at fixed timer */
  u_int8_t suppress_ra_run;           /* Suppress RA on running conf   */
  u_int32_t prefix_valid_lifetime;      /* Prefix Valid Lifetime         */
  u_int32_t prefix_preferred_lifetime;  /* Prefix Preferred Lifetime     */
  u_int8_t  block_rs_on_delay;                /* Check Min Delay between Router Solicitation */

  struct route_table *rt_prefix;        /* Prefix lists. */

#ifdef HAVE_MIP6
  /* Home Agent field. */
  u_int16_t ha_preference;              /* Home Agent Preference. */
  u_int16_t ha_lifetime;                /* Home Agent Lifetime. */
  struct list *li_homeagent;            /* Home Agent list. */
#endif /* HAVE_MIP6 */

  /* Destination address for the unicast Router Advertisement. */
  struct pal_in6_addr dst;

  /* Threads. */
  struct thread *t_ra_solicited;        /* Solicited Router Advertisement. */
  struct thread *t_ra_unsolicited;      /* Unsolicited Router Advertisement. */
  struct thread *t_rs_solicited;        /* Solicited Router Solicitation */

};

#define NSM_RTADV_CONFIG_EMPTY(I)                                             \
   ((I)->config == 0)
#define NSM_RTADV_CONFIG_CHECK(I,T)                                           \
   (CHECK_FLAG ((I)->config, NSM_RTADV_CONFIG_ ## T))
#define NSM_RTADV_CONFIG_SET(I,T)                                             \
   (SET_FLAG ((I)->config, NSM_RTADV_CONFIG_ ## T))
#define NSM_RTADV_CONFIG_UNSET(I,T)                                           \
   do {                                                                       \
     (I)->config &= ~(NSM_RTADV_CONFIG_ ## T);                                \
     if (NSM_RTADV_CONFIG_EMPTY (I))                                          \
       nsm_rtadv_if_free ((I)->ifp);                                          \
   } while (0)

#define RTADV_MAX_RA_INTERVAL           600
#define RTADV_MIN_RA_INTERVAL_FACTOR    0.33
#define RTADV_DEFAULT_ROUTER_LIFETIME   1800
#define RTADV_DEFAULT_REACHABLE_TIME    0
#define RTADV_DEFAULT_RETRANS_TIMER     0
#define RTADV_MIN_RETRANS_TIMER         1000
#define RTADV_MAX_RETRANS_TIMER         3600000
#define RTADV_DEFAULT_CUR_HOPLIMIT    64
#define RTADV_MIN_CUR_HOPLIMIT                0
#define RTADV_MAX_CUR_HOPLIMIT                255
#define RTADV_DEFAULT_MIN_RA_INTERVAL   198
#define RTADV_MIN_RA_INTERVAL           3
#define RTADV_MAX_INITIAL_RTR_ADVERTISEMENTS     3
#define RTADV_MAX_INITIAL_RTR_ADVERT_INTERVAL 16 /* seconds */
#define RTADV_DEFAULT_PREFIX_VALID_LIFETIME   2592000 /* seconds */
#define RTADV_DEFAULT_PREFIX_PREFERRED_LIFETIME 604800  /* seconds */
#define RTADV_MAX_PREFIX_VALID_LIFETIME               4294967295UL
#define RTADV_MAX_PREFIX_PREFERRED_LIFETIME   4294967295UL
#define RTADV_MIN_DELAY_BETWEEN_RAS           3 /* seconds */
#define RTADV_MAX_REACHABLE_TIME        3600000
#define RTADV_DEFAULT_HA_PREFERENCE     0
#define RTADV_DEFAULT_HA_LIFETIME       0

/* Router advertisement prefix. */
struct rtadv_prefix
{
  /* Pointer to the route node. */
  struct route_node *rn;

  /* Real Prefix length. */
  u_char prefixlen;

  /* Prefix information field. */
  u_char flags;                         /* Flags. */
  u_int32_t vlifetime;                  /* Valid lifetime. */
  u_int32_t plifetime;                  /* Preferred lifetime. */

  /* Threads. */
  struct thread *t_vlifetime;           /* Valid lifetime timer. */
  struct thread *t_plifetime;           /* Preferred lifetime timer. */
};

#define RTADV_DEFAULT_VALID_LIFETIME            2592000
#define RTADV_DEFAULT_PREFERRED_LIFETIME        604800

#ifdef HAVE_MIP6
/* Home Agent list entry. */
struct rtadv_home_agent
{
  /* Pointer to the RA interface structure. */
  struct rtadv_if *rif;

  /* Link-local address of the home agent. */
  struct pal_in6_addr lladdr;

  /* Home Agent information. */
  u_int16_t preference;                 /* Preference of the home agent. */
  u_int16_t lifetime;                   /* Lifetime of the home agent. */

  /* Home Agent address list. */
  struct route_table *rt_address;

  /* Threads. */
  struct thread *t_lifetime;            /* Home Agent Lifetime timer. */
};
#endif /* HAVE_MIP6 */


/* Macros. */
#define NSM_RTADV_IF_TIMER_ON(T,F,V)                                          \
    do {                                                                      \
      if (!(T))                                                               \
        (T) = thread_add_timer (NSM_ZG, (F), rif, (V));                       \
    } while (0)

#define NSM_RTADV_HA_TIMER_ON(T,F,V)                                          \
    do {                                                                      \
      if (!(T))                                                               \
        (T) = thread_add_timer (NSM_ZG, (F), ha, (V));                        \
    } while (0)

#define NSM_RTADV_PREFIX_TIMER_ON(T,F,V)                                      \
    do {                                                                      \
      if (!(T))                                                               \
        (T) = thread_add_timer (NSM_ZG, (F), rp, (V));                        \
    } while (0)

#define NSM_RTADV_RS_TIMER_ON(T,F,V)                                        \
  do {                                                                              \
    if (!(T))                                                               \
      (T) = thread_add_timer (NSM_ZG, (F), rif, (V));                       \
  } while (0)


#define NSM_RTADV_TIMER_OFF(X)      THREAD_TIMER_OFF(X)

#define NSM_RTADV_EVENT_EXECUTE(F,A)                                          \
    do {                                                                      \
      thread_execute (NSM_ZG, (F), (A), 0);                                   \
    } while (0)


/* Router Advertisement CLI-APIs. */
int nsm_rtadv_ra_set (u_int32_t, char *);
int nsm_rtadv_ra_unset (u_int32_t, char *);
int nsm_rtadv_ra_interval_set (u_int32_t, char *, u_int32_t);
int nsm_rtadv_ra_interval_unset (u_int32_t, char *);
int nsm_rtadv_router_lifetime_set (u_int32_t, char *, u_int16_t);
int nsm_rtadv_router_lifetime_unset (u_int32_t, char *);
int nsm_rtadv_router_reachable_time_set (u_int32_t, char *, u_int32_t);
int nsm_rtadv_router_reachable_time_unset (u_int32_t, char *);
int nsm_rtadv_router_retrans_timer_set (u_int32_t, char *, u_int32_t);
int nsm_rtadv_router_retrans_timer_unset (u_int32_t, char *);
int nsm_rtadv_curr_hoplimit_set (u_int32_t, char *, u_int8_t);
int nsm_rtadv_curr_hoplimit_unset (u_int32_t, char *);
int nsm_rtadv_linkmtu_set (u_int32_t, char *);
int nsm_rtadv_linkmtu_unset (u_int32_t, char *);
int nsm_rtadv_min_ra_interval_set (u_int32_t, char *, u_int32_t);
int nsm_rtadv_min_ra_interval_unset (u_int32_t, char *);
int nsm_rtadv_prefix_valid_lifetime_set (u_int32_t, char *, u_int32_t);
int nsm_rtadv_prefix_valid_lifetime_unset (u_int32_t, char *);
int nsm_rtadv_prefix_preferred_lifetime_set (u_int32_t, char *, u_int32_t);
int nsm_rtadv_prefix_preferred_lifetime_unset (u_int32_t, char *);
int nsm_rtadv_prefix_offlink_set (u_int32_t, char *);
int nsm_rtadv_prefix_offlink_unset (u_int32_t, char *);
int nsm_rtadv_prefix_noautoconf_set (u_int32_t, char *);
int nsm_rtadv_prefix_noautoconf_unset (u_int32_t, char *);
int nsm_rtadv_ra_managed_config_flag_set (u_int32_t, char *);
int nsm_rtadv_ra_managed_config_flag_unset (u_int32_t, char *);
int nsm_rtadv_ra_other_config_flag_set (u_int32_t, char *);
int nsm_rtadv_ra_other_config_flag_unset (u_int32_t, char *);
int nsm_rtadv_ra_prefix_set (u_int32_t, char *, struct pal_in6_addr *, u_char,
                             u_int32_t, u_int32_t, u_char);
int nsm_rtadv_ra_prefix_unset (u_int32_t, char *, struct pal_in6_addr *,
                               u_char);
#ifdef HAVE_MIP6
int nsm_rtadv_mip6_ha_set (u_int32_t, char *);
int nsm_rtadv_mip6_ha_unset (u_int32_t, char *);
int nsm_rtadv_mip6_ha_info_set (u_int32_t, char *, u_int16_t, u_int16_t);
int nsm_rtadv_mip6_ha_info_unset (u_int32_t, char *);
int nsm_rtadv_mip6_adv_interval_set (u_int32_t, char *);
int nsm_rtadv_mip6_adv_interval_unset (u_int32_t, char *);
int nsm_rtadv_mip6_prefix_set (u_int32_t, char *, struct pal_in6_addr *,
                               u_char, u_int32_t, u_int32_t, u_char);
int nsm_rtadv_mip6_prefix_unset (u_int32_t, char *, struct pal_in6_addr *,
                                 u_char);
#endif /* HAVE_MIP6 */


/* Prototypes. */
void nsm_rtadv_if_free (struct interface *);
struct rtadv_if *nsm_rtadv_if_lookup_by_name (struct nsm_master *, char *);
void nsm_rtadv_if_up (struct interface *);
void nsm_rtadv_if_down (struct interface *);
void nsm_rtadv_if_show (struct cli *, struct interface *);
void nsm_rtadv_if_config_write (struct cli *, struct interface *);
void nsm_rtadv_init (struct nsm_master *);
void nsm_rtadv_finish (struct nsm_master *);

#endif /* _PACOS_NSM_RTADV_H */
