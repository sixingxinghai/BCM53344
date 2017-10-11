/* Copyright (C) 2002  All Rights Reserved. */

#ifndef _PACOS_OSPF_COMMON_H
#define _PACOS_OSPF_COMMON_H

#include "ospf_const.h"
#include "ospf_util.h"
#include "ospf_fsm.h"
#include "ospf_debug_msg.h"

/* Macros. */
#define IS_PROC_ACTIVE(O)       CHECK_FLAG ((O)->flags, OSPF_PROC_UP)
#define IS_OSPF_ABR(O)          CHECK_FLAG ((O)->flags, OSPF_ROUTER_ABR)
#define IS_OSPF_ASBR(O)         CHECK_FLAG ((O)->flags, OSPF_ROUTER_ASBR)
#define IS_BACKBONE_ACTIVE(B)   ((B) != NULL && (B)->full_nbr_count != 0)
#define IS_AREA_ACTIVE(A)       CHECK_FLAG ((A)->flags, OSPF_AREA_UP)
#define IS_AREA_BACKBONE(A)     ((A)->area_id.s_addr == 0)
#define IS_AREA_TRANSIT(A)      CHECK_FLAG ((A)->flags, OSPF_AREA_TRANSIT)
#define IS_AREA_DEFAULT(A)                                                    \
    ((A)->conf.external_routing == OSPF_AREA_DEFAULT)
#define IS_AREA_STUB(A)                                                       \
    ((A)->conf.external_routing == OSPF_AREA_STUB)
#define IS_AREA_NSSA(A)                                                       \
    ((A)->conf.external_routing == OSPF_AREA_NSSA)

#define IS_AREA_ID_BACKBONE(I)                                                \
    (IPV4_ADDR_SAME (&(I), &OSPFBackboneAreaID))

#define IS_AREA_SUSPENDED(A, T)                                               \
    (CHECK_FLAG ((A)->top->flags, OSPF_ASE_CALC_SUSPENDED)                    \
     || ((A)->t_suspend && (A)->t_suspend != (T)))

#define IS_LSDB_SUSPENDED(L, T)                                               \
     (L)->t_suspend && (L)->t_suspend != (T)

#define IS_AREA_IA_CALCULATE(A)                                               \
    (!IS_OSPF_ABR ((A)->top)                                                  \
     || IS_AREA_BACKBONE (A)                                                  \
     || (((A)->top->abr_type == OSPF_ABR_TYPE_CISCO                           \
          || (A)->top->abr_type == OSPF_ABR_TYPE_IBM)                         \
         && !IS_BACKBONE_ACTIVE ((A)->top->backbone)))

#define IS_AREA_IA_ORIGINATE(A, P)                                            \
    (IS_OSPF_ABR ((A)->top)                                                   \
     && ((P)->type == OSPF_PATH_CONNECTED                                     \
         || (P)->type == OSPF_PATH_INTRA_AREA                                 \
         || ((P)->type == OSPF_PATH_INTER_AREA                                \
             && IS_AREA_ID_BACKBONE ((P)->area_id)                            \
             && ((A)->top->abr_type == OSPF_ABR_TYPE_CISCO                    \
                 ? IS_BACKBONE_ACTIVE ((A)->top->backbone) : 1))))

#define IS_AREA_THREAD_SUSPENDED(A, T)                                        \
    ((T) != NULL && (T) == (A)->t_suspend)

#define IS_OSPF_NETWORK_STUB(I)                                               \
    ((I)->type == OSPF_IFTYPE_LOOPBACK                                        \
     || (I)->type == OSPF_IFTYPE_POINTOPOINT                                  \
     || (I)->type == OSPF_IFTYPE_POINTOMULTIPOINT                             \
     || (I)->type == OSPF_IFTYPE_POINTOMULTIPOINT_NBMA                        \
     || (((I)->type == OSPF_IFTYPE_BROADCAST || (I)->type == OSPF_IFTYPE_NBMA)\
         && ((I)->state == IFSM_Waiting || (I)->full_nbr_count == 0)))

#define IS_OSPF_NETWORK_NBMA(I)                                               \
    ((I)->type == OSPF_IFTYPE_NBMA                                            \
     || (I)->type == OSPF_IFTYPE_POINTOMULTIPOINT_NBMA)

#define IS_OSPF_IPV4_UNNUMBERED(I)                                            \
    ((I)->type == OSPF_IFTYPE_POINTOPOINT                                     \
     && if_is_ipv4_unnumbered ((I)->u.ifp))

#define IS_OSPF_IPV6_UNNUMBERED(I)                                            \
    ((I)->type == OSPF_IFTYPE_POINTOPOINT                                     \
     && if_is_ipv6_unnumbered ((I)->u.ifp))

#define OSPF_AREA_EXTERNAL_ROUTING(A)           (A)->conf.external_routing
#define OSPF_AREA_DEFAULT_COST(A)               (A)->conf.default_cost

#define LIST_DELETE(L)                                                        \
     do {                                                                     \
       list_delete ((struct list *)(L));                                      \
       (L) = NULL;                                                            \
     } while (0)

#define TABLE_FINISH(T)                                                       \
     do {                                                                     \
       ls_table_finish ((struct ls_table *)(T));                              \
       (T) = NULL;                                                            \
     } while (0);

#define OSPF_TIMER_ON(T,F,A,V)                                                \
    do {                                                                      \
      pal_assert ((A) != NULL);                                               \
      if (!(T))                                                               \
        (T) = thread_add_timer (ZG, (F), (A), (V));                           \
    } while (0)

#define OSPF_TOP_TIMER_ON(T,F,V)                                              \
    do {                                                                      \
      if (!CHECK_FLAG (top->flags, OSPF_PROC_DESTROY))                        \
        OSPF_TIMER_ON ((T), (F), top, (V));                                   \
    } while (0)

#define OSPF_TV_TIMER_ON(T,F,A,V)                                             \
    do {                                                                      \
      if (!(T))                                                               \
        (T) = thread_add_timer_timeval (ZG, (F), (A), (V));                   \
    } while (0)

#define OSPF_IF_TIMER_ON(T,F,V)      OSPF_TIMER_ON ((T), (F), oi, (V))
#define OSPF_AREA_TIMER_ON(T,F,V)    OSPF_TIMER_ON ((T), (F), area, (V))
#define OSPF_POLL_TIMER_ON(T,F,V)    OSPF_TIMER_ON ((T), (F), nbr_static, (V))

#define OSPF_EVENT_ON(T,F,A)                                                  \
    do {                                                                      \
      if (!(T))                                                               \
        (T) = thread_add_event (ZG, (F), (A), 0);                             \
    } while (0)

#define OSPF_EVENT_EXECUTE(F,A,V)                                             \
    do {                                                                      \
      thread_execute (ZG, (F), (A), (V));                                     \
    } while (0)

#define OSPF_READ_ON(T,F,O)                                                   \
    do {                                                                      \
      if (!(T))                                                               \
        (T) = thread_add_read (ZG, (F), (O), (O)->fd);                        \
    } while (0)

#define OSPF_TIMER_OFF(X)                                                     \
    do {                                                                      \
      if (X)                                                                  \
        {                                                                     \
          thread_cancel (X);                                                  \
          (X) = NULL;                                                         \
        }                                                                     \
    } while (0)

#define OSPF_EVENT_OFF(X)       OSPF_TIMER_OFF (X)

#define OSPF_EVENT_SUSPEND_ON(T,F,A,V)                                        \
    do {                                                                      \
      if (!(T))                                                               \
        (A)->t_suspend = (T) = thread_add_event_low (ZG, (F), (A), (V));      \
    } while (0)

#define OSPF_EVENT_SUSPEND_OFF(T,A)                                           \
    do {                                                                      \
      if ((A)->t_suspend == (T))                                              \
        (A)->t_suspend = NULL;                                                \
      OSPF_EVENT_OFF(T);                                                      \
    } while (0)

#define OSPF_EVENT_SUSPEND_ON_LSDB(T,F,L,V)                                        \
    do {                                                                      \
      if (!(T))                                                               \
        (L)->t_suspend = (T) = thread_add_event_low (ZG, (F), (L), (V));      \
    } while (0)

#define OSPF_EVENT_SUSPEND_OFF_LSDB(T,L)                                           \
    do {                                                                      \
      if ((L)->t_suspend == (T))                                              \
        (L)->t_suspend = NULL;                                                \
      OSPF_EVENT_OFF(T);                                                      \
    } while (0)

#define OSPF_EVENT_PENDING_ON(L,F,A,V)                                        \
    do {                                                                      \
      struct thread *thread = thread_get (ZG, THREAD_EVENT, (F), (A));        \
      if (thread != NULL)                                                     \
        {                                                                     \
          thread->u.val = (V);                                                \
          thread_list_add (&(L), thread);                                     \
        }                                                                     \
    } while (0)

#define OSPF_EVENT_PENDING_EXECUTE(L)                                         \
    do {                                                                      \
      thread_list_execute (ZG, &(L));                                         \
    } while (0)

#define OSPF_EVENT_PENDING_CLEAR(L)                                           \
    do {                                                                      \
      thread_list_clear (ZG, &(L));                                           \
    } while (0)

#define LSDB_LOOP_HEAD(S)                                                     \
    ((S)->next ? (S)->next : (S)->head)

#define OSPF_RT_HEAD(A, T)                                                    \
    ((A)->rt_calc->rn_next[(T)] ? (A)->rt_calc->rn_next[(T)]                  \
     : ls_table_top ((A)->top->rt_network))

#define OSPF_RT_HEAD_ASBR(A, T)                                               \
    ((A)->rt_calc->rn_next_asbr[(T)] ? (A)->rt_calc->rn_next_asbr[(T)]        \
     : ls_table_top ((A)->top->rt_asbr))

/* this is notification mechanish functions */
struct ospf_notifier
{
  u_char event;
  int (*callback) (u_char, void *, void *, void *);
  void *arg;
  int priority;
};

enum
{
  OSPF_NOTIFY_PROCESS_NEW = 0,
  OSPF_NOTIFY_PROCESS_DEL,
  OSPF_NOTIFY_ROUTER_ID_CHANGED,
  OSPF_NOTIFY_LINK_NEW,
  OSPF_NOTIFY_LINK_DEL,
  OSPF_NOTIFY_LINK_UP,
  OSPF_NOTIFY_LINK_DOWN,
  OSPF_NOTIFY_ADDRESS_NEW,
  OSPF_NOTIFY_ADDRESS_DEL,
  OSPF_NOTIFY_NETWORK_NEW,
  OSPF_NOTIFY_NETWORK_DEL,
  OSPF_NOTIFY_AREA_NEW,
  OSPF_NOTIFY_AREA_DEL,
  OSPF_NOTIFY_IFSM_STATE_CHANGE,
  OSPF_NOTIFY_NFSM_STATE_CHANGE,
  OSPF_NOTIFY_WRITE_LINK_CONFIG,
  OSPF_NOTIFY_WRITE_AREA_CONFIG,
  OSPF_NOTIFY_WRITE_PROCESS_CONFIG,
  OSPF_NOTIFY_MAX
};

#endif /* _PACOS_OSPF_COMMON_H */
