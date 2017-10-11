/* Copyright (C) 2001, 2002  All Rights Reserved. */

#ifndef _PACOS_OSPF_DEBUG_MSG_H
#define _PACOS_OSPF_DEBUG_MSG_H

#define OSPF_TIMER_STR_MAXLEN          9

/* Extern variables. */
extern struct message ospf_ifsm_state_msg[];
extern struct message ospf_nfsm_state_msg[];
extern struct message ospf_ifsm_event_msg[];
extern struct message ospf_nfsm_event_msg[];
extern struct message ospf_redist_proto_msg[];
extern struct message ospf_packet_type_msg[];
extern struct message ospf_if_network_type_msg[];
extern struct message ospf_nsm_route_type_msg[];
extern struct message ospf_area_type_msg[];
extern int ospf_ifsm_state_msg_max;
extern int ospf_nfsm_state_msg_max;
extern int ospf_ifsm_event_msg_max;
extern int ospf_nfsm_event_msg_max;
extern int ospf_redist_proto_msg_max;
extern int ospf_packet_type_msg_max;
extern int ospf_if_network_type_msg_max;
extern int ospf_nsm_route_type_msg_max;
extern int ospf_area_type_msg_max;

extern char *ospf_packet_type_str[];
extern char *ospf_path_code_str[];

/* Prototypes. */
char *ospf_if_type_string (u_char);
char *ospf_abr_type_string (u_char);
char *ospf_packet_type_string (u_char);
char *ospf_timer_dump (struct thread *, char *);
char *ospf_uptime_dump (pal_time_t, char *);

#endif /* _PACOS_OSPF_DEBUG_MSG_H */
