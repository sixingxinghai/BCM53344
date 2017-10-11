/* Copyright (C) 2001-2003  All Rights Reserved. */

#ifndef _PACOS_OSPF_IFSM_H
#define _PACOS_OSPF_IFSM_H

#define IS_DECLARED_DR(I)                                                     \
    IPV4_ADDR_SAME (&((I)->d_router), &((I)->address->u.prefix4))
#define IS_DECLARED_BACKUP(I)                                                 \
    IPV4_ADDR_SAME (&((I)->bd_router), &((I)->address->u.prefix4))

#define OSPF_WRITE_ON(T)                                                      \
    do {                                                                      \
      if ((T)->t_write == NULL)                                               \
        (T)->t_write =                                                        \
          thread_add_write (ZG, ospf_write, (T), (T)->fd);                    \
    } while (0)

#define OSPF_IFSM_WRITE_ON(I)                                                 \
    do {                                                                      \
      if (!CHECK_FLAG ((I)->flags, OSPF_IF_WRITE_Q))                          \
        {                                                                     \
          listnode_add ((I)->top->oi_write_q, (I));                           \
          SET_FLAG ((I)->flags, OSPF_IF_WRITE_Q);                             \
        }                                                                     \
      OSPF_WRITE_ON ((I)->top);                                               \
    } while (0)

/* Macro for OSPF IFSM timer turn on. */
#define OSPF_IFSM_TIMER_ON(T,F,V)   OSPF_IF_TIMER_ON ((T), (F), (V))

/* Macro for OSPF IFSM timer turn off. */
#define OSPF_IFSM_TIMER_OFF(X)      OSPF_TIMER_OFF ((X))

/* Macro for OSPF schedule event. */
#define OSPF_IFSM_EVENT_SCHEDULE(I,E)                                         \
    do {                                                                      \
      if (!CHECK_FLAG ((I)->flags, OSPF_IF_DESTROY))                          \
        thread_add_event (ZG, ospf_ifsm_event, (I), (E));                     \
    } while (0)

/* Macro for OSPF execute event. */
#define OSPF_IFSM_EVENT_EXECUTE(I,E)                                          \
    do {                                                                      \
      thread_execute (ZG, ospf_ifsm_event, (I), (E));                         \
    } while (0)

/* Prototypes. */
int ospf_dr_election (struct ospf_interface *);
int ospf_ifsm_event (struct thread *);

#endif /* _PACOS_OSPF_IFSM_H */
