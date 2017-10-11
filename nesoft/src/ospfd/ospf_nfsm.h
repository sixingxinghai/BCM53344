/* Copyright (C) 2001-2003  All Rights Reserved. */

#ifndef _PACOS_OSPF_NFSM_H
#define _PACOS_OSPF_NFSM_H

/* Macro for OSPF NFSM timer turn on. */
#define OSPF_NFSM_TIMER_ON(T,F,V)    OSPF_TIMER_ON ((T), (F), nbr, (V))

/* Macro for OSPF NFSM timer turn off. */
#define OSPF_NFSM_TIMER_OFF(X)       OSPF_TIMER_OFF ((X))

/* Macro for OSPF NFSM schedule event. */
#define OSPF_NFSM_EVENT_SCHEDULE(N,E)                                         \
    do {                                                                      \
      if (!(N)->t_events [(E)])                                               \
        (N)->t_events [(E)] =                                                 \
          thread_add_event (ZG, ospf_nfsm_event, (N), (E));                   \
    } while (0) 

/* Macro for OSPF NFSM execute event. */
#define OSPF_NFSM_EVENT_EXECUTE(N,E)                                          \
    thread_execute (ZG, ospf_nfsm_event, (N), (E))

/* Prototypes. */
int ospf_nfsm_event (struct thread *);
void ospf_nfsm_check_nbr_loading (struct ospf_neighbor *);
int ospf_db_summary_isempty (struct ospf_neighbor *);
int ospf_db_summary_count (struct ospf_neighbor *);
void ospf_db_summary_clear (struct ospf_neighbor *);
int ospf_inactivity_timer (struct thread *);
int ospf_helper_timer (struct thread *);

#endif /* _PACOS_OSPF_NFSM_H */
