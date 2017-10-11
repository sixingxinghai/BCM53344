/* Copyright (C) 2001-2003  All Rights Reserved. */

#ifndef _PACOS_OSPF_DEBUG_H
#define _PACOS_OSPF_DEBUG_H

#include "stream.h"

#ifdef HAVE_OSPF_CSPF
struct ospf_te_lsa_data;
#endif /* HAVE_OSPF_CSPF */

/* Debug Flags. */
#define OSPF_DEBUG_HELLO        0x01
#define OSPF_DEBUG_DB_DESC      0x02
#define OSPF_DEBUG_LS_REQ       0x04
#define OSPF_DEBUG_LS_UPD       0x08
#define OSPF_DEBUG_LS_ACK       0x10
#define OSPF_DEBUG_PACKET_ALL   0x1f

#define OSPF_DEBUG_SEND         0x01
#define OSPF_DEBUG_RECV         0x02
#define OSPF_DEBUG_SEND_RECV    0x03
#define OSPF_DEBUG_DETAIL       0x04

#define OSPF_DEBUG_IFSM_STATUS  0x01
#define OSPF_DEBUG_IFSM_EVENTS  0x02
#define OSPF_DEBUG_IFSM_TIMERS  0x04
#define OSPF_DEBUG_IFSM         0x07
#define OSPF_DEBUG_NFSM_STATUS  0x01
#define OSPF_DEBUG_NFSM_EVENTS  0x02
#define OSPF_DEBUG_NFSM_TIMERS  0x04
#define OSPF_DEBUG_NFSM         0x07

#define OSPF_DEBUG_LSA_GENERATE 0x01
#define OSPF_DEBUG_LSA_FLOODING 0x02
#define OSPF_DEBUG_LSA_INSTALL  0x04
#define OSPF_DEBUG_LSA_REFRESH  0x08
#define OSPF_DEBUG_LSA_MAXAGE   0x10
#define OSPF_DEBUG_LSA          0x1F

#define OSPF_DEBUG_NSM_INTERFACE     0x01
#define OSPF_DEBUG_NSM_REDISTRIBUTE  0x02
#define OSPF_DEBUG_NSM               0x03

#define OSPF_DEBUG_EVENT_ABR          0x01
#define OSPF_DEBUG_EVENT_NSSA         0x02
#define OSPF_DEBUG_EVENT_ASBR         0x04
#define OSPF_DEBUG_EVENT_VLINK        0x08
#define OSPF_DEBUG_EVENT_LSA          0x10
#define OSPF_DEBUG_EVENT_OS           0x20
#define OSPF_DEBUG_EVENT_ROUTER       0x40
#define OSPF_DEBUG_EVENT              0x7f

#define OSPF_DEBUG_BFD               0x01

#define OSPF_DEBUG_RT_CALC_SPF 0x01
#define OSPF_DEBUG_RT_CALC_IA  0x02
#define OSPF_DEBUG_RT_CALC_ASE 0x04
#define OSPF_DEBUG_RT_INSTALL  0x08
#define OSPF_DEBUG_RT_ALL      0x0f

/* Macro for setting debug option. */
#define CONF_DEBUG_PACKET_ON(a, b)          om->debug.conf.packet[a] |= (b)
#define CONF_DEBUG_PACKET_OFF(a, b)         om->debug.conf.packet[a] &= ~(b)
#define TERM_DEBUG_PACKET_ON(a, b)          om->debug.term.packet[a] |= (b)
#define TERM_DEBUG_PACKET_OFF(a, b)         om->debug.term.packet[a] &= ~(b)
#define DEBUG_PACKET_ON(v, t, f)                                              \
    do {                                                                      \
      if ((v)->mode == EXEC_MODE)                                             \
        {                                                                     \
          TERM_DEBUG_PACKET_ON (t - 1, f);                                    \
          if (CHECK_FLAG (f, OSPF_DEBUG_SEND) &&                              \
              CHECK_FLAG (f, OSPF_DEBUG_RECV))                                \
            {                                                                 \
              cli_out ((v), "OSPF packet %s%s debugging is on\n",             \
                       ospf_packet_type_str[t],                               \
                       CHECK_FLAG (f, OSPF_DEBUG_DETAIL) ? " detail" : "");   \
            }                                                                 \
          else                                                                \
            if (CHECK_FLAG (f, OSPF_DEBUG_SEND))                              \
              {                                                               \
                cli_out ((v), "OSPF packet %s send%s debugging is on\n",      \
                         ospf_packet_type_str[t],                             \
                         CHECK_FLAG (f, OSPF_DEBUG_DETAIL) ? " detail" : ""); \
              }                                                               \
            else if (CHECK_FLAG (f, OSPF_DEBUG_RECV))                         \
              {                                                               \
                cli_out ((v), "OSPF packet %s receive%s debugging is on\n",   \
                         ospf_packet_type_str[t],                             \
                         CHECK_FLAG (f, OSPF_DEBUG_DETAIL) ? " detail" : ""); \
              }                                                               \
        }                                                                     \
      else if ((v)->mode == CONFIG_MODE)                                      \
        {                                                                     \
          CONF_DEBUG_PACKET_ON (t - 1, f);                                    \
          TERM_DEBUG_PACKET_ON (t - 1, f);                                    \
        }                                                                     \
    } while (0)

#define DEBUG_PACKET_OFF(v, t, f)                                             \
    do {                                                                      \
      if ((v)->mode == EXEC_MODE)                                             \
        {                                                                     \
          TERM_DEBUG_PACKET_OFF (t - 1, f);                                   \
          if (CHECK_FLAG (f, OSPF_DEBUG_SEND) &&                              \
              CHECK_FLAG (f, OSPF_DEBUG_RECV))                                \
            {                                                                 \
              cli_out ((v), "OSPF packet %s%s debugging is off\n",            \
                       ospf_packet_type_str[t],                               \
                       CHECK_FLAG (f, OSPF_DEBUG_DETAIL) ? " detail" : "");   \
            }                                                                 \
          else                                                                \
            if (CHECK_FLAG (f, OSPF_DEBUG_SEND))                              \
              {                                                               \
                cli_out ((v), "OSPF packet %s send%s debugging is off\n",     \
                         ospf_packet_type_str[t],                             \
                         CHECK_FLAG (f, OSPF_DEBUG_DETAIL) ? " detail" : ""); \
              }                                                               \
            else if (CHECK_FLAG (f, OSPF_DEBUG_RECV))                         \
              {                                                               \
                cli_out ((v), "OSPF packet %s receive%s debugging is off\n",  \
                         ospf_packet_type_str[t],                             \
                         CHECK_FLAG (f, OSPF_DEBUG_DETAIL) ? " detail" : ""); \
              }                                                               \
        }                                                                     \
      else if ((v)->mode == CONFIG_MODE)                                      \
        {                                                                     \
          CONF_DEBUG_PACKET_OFF (t - 1, f);                                   \
          TERM_DEBUG_PACKET_OFF (t - 1, f);                                   \
        }                                                                     \
    } while (0)

#define CONF_DEBUG_ON(A, B)      om->debug.conf.A |= (OSPF_DEBUG_ ## B)
#define CONF_DEBUG_OFF(A, B)     om->debug.conf.A &= ~(OSPF_DEBUG_ ## B)
#define TERM_DEBUG_ON(A, B)      om->debug.term.A |= (OSPF_DEBUG_ ## B)
#define TERM_DEBUG_OFF(A, B)     om->debug.term.A &= ~(OSPF_DEBUG_ ## B)

#define DEBUG_ON(v, a, b, m)                                                  \
    do {                                                                      \
      if ((v)->mode == EXEC_MODE)                                             \
        {                                                                     \
          TERM_DEBUG_ON (a, b);                                               \
          cli_out ((v), m " debugging is on\n");                              \
        }                                                                     \
      else if ((v)->mode == CONFIG_MODE)                                      \
        {                                                                     \
          CONF_DEBUG_ON (a, b);                                               \
          TERM_DEBUG_ON (a, b);                                               \
        }                                                                     \
    } while (0)

#define DEBUG_OFF(v, a, b, m)                                                 \
    do {                                                                      \
      if ((v)->mode == EXEC_MODE)                                             \
        {                                                                     \
          TERM_DEBUG_OFF (a, b);                                              \
          cli_out ((v), m " debugging is off\n");                             \
        }                                                                     \
      else if ((v)->mode == CONFIG_MODE)                                      \
        {                                                                     \
          CONF_DEBUG_OFF (a, b);                                              \
          TERM_DEBUG_OFF (a, b);                                              \
        }                                                                     \
    } while (0)

/* Macro for checking debug option. */
#define IS_DEBUG_OSPF_PACKET(A, B)                                            \
        (om->debug.term.packet[(A) - 1] & OSPF_DEBUG_ ## B)
#define IS_DEBUG_OSPF(A, B)                                                   \
        (om->debug.term.A & OSPF_DEBUG_ ## B)
#define IS_DEBUG_NSM                                                          \
        (om->debug.term.nsm & OSPF_DEBUG_NSM)
#define IS_DEBUG_BFD                                                          \
        (om->debug.term.bfd & OSPF_DEBUG_BFD)
#define IS_CONF_DEBUG_OSPF_PACKET(A, B)                                       \
        (om->debug.conf.packet[(A) - 1] & OSPF_DEBUG_ ## B)
#define IS_CONF_DEBUG_OSPF(A, B)                                              \
        (om->debug.conf.A & OSPF_DEBUG_ ## B)

#define OSPF_AREA_STRING_MAXLEN        16
#define OSPF_AREA_DESC_STRING_MAXLEN   23
#define OSPF_IF_STRING_MAXLEN          40
#define OSPF_NBR_STRING_MAXLEN         56
#define OSPF_LSA_STRING_MAXLEN         55 /* 32 for short. */
#define OSPF_VLINK_STRING_MAXLEN       16
#define OSPF_NBR_STATE_STR_MAXLEN      16
#define OSPF_OPTION_STR_MAXLEN         21
#define OSPF_DD_BITS_STR_MAXLEN         9
#define OSPF_EXTENDED_OPTION_STR_MAXLEN 6 

#define NBR_NAME(N)      ospf_nbr_name_string ((N), nbr_str)
#define IF_NAME(I)       ospf_if_name_string ((I), if_str)
#define LSA_NAME(L)      ospf_lsa_name_string ((L), lsa_str)
#define VLINK_NAME(V)    ospf_vlink_name_string ((V), vlink_str)
#define IF_INDEX(I)      ((I)->type == OSPF_IFTYPE_VIRTUALLINK                \
                          ? 0 : (I)->u.ifp->ifindex)

/* Message Strings. */
extern char *ospf_lsa_type_str[];

/* Prototypes. */
char *ospf_area_fmt_string (struct pal_in4_addr, u_char, char *);
char *ospf_area_desc_string (struct ospf_area *, char *);
#ifdef HAVE_VRF_OSPF
char *ospf_domain_id_fmt_string (u_char *, char*);
#endif /* HAVE_VRF_OSPF */
char *ospf_if_name_string (struct ospf_interface *, char *);
char *ospf_nbr_name_string (struct ospf_neighbor *, char *);
char *ospf_lsa_name_string (struct ospf_lsa *, char *);
char *ospf_vlink_name_string (struct ospf_vlink *, char *);
char *ospf_nbr_state_message (struct ospf_neighbor *, char *);
char *ospf_option_dump (u_char, char *);
#ifdef HAVE_RESTART
char *ospf_extended_option_dump (u_int32_t, char *);
#endif /* HAVE_RESTART */
void ospf_packet_dump (struct ospf_master *, u_char *);
char *ospf_dd_bits_dump (u_char, char *);
void ospf_lsa_header_dump (struct lsa_header *);

void ospf_debug_init ();

#endif /* _PACOS_OSPF_DEBUG_H */
