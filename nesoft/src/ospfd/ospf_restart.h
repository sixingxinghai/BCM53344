/* Copyright (C) 2002-2003  All Rights Reserved. */

#ifndef _PACOS_OSPF_RESTART_H
#define _PACOS_OSPF_RESTART_H

#define OSPF_GRACE_PERIOD_DEFAULT                      120
#define OSPF_GRACE_PERIOD_MIN                            1
#define OSPF_GRACE_PERIOD_MAX                         1800

#define OSPF_RESTART_STALE_TIMER_DELAY                  40
#define OSPF_RESTART_STATE_TIMER_INTERVAL                3

#define OSPF_GRACE_LSA_TLV_DATA_SIZE_MAX                24

#define OSPF_GRACE_LSA_GRACE_PERIOD                      1
#define OSPF_GRACE_LSA_RESTART_REASON                    2
#define OSPF_GRACE_LSA_IP_INTERFACE_ADDRESS              3

#define OSPF_GRACE_LSA_GRACE_PERIOD_LEN                  4
#define OSPF_GRACE_LSA_RESTART_REASON_LEN                1
#define OSPF_GRACE_LSA_IP_INTERFACE_ADDRESS_LEN          4

#define IS_LSA_TYPE_CHECKED_FOR_RESTART(L)                                    \
    (((L)->data->type >= OSPF_ROUTER_LSA                                      \
      && (L)->data->type <= OSPF_AS_EXTERNAL_LSA)                             \
     || (L)->data->type == OSPF_AS_NSSA_LSA)

#define IS_GRACEFUL_RESTART(T)                                                \
    (CHECK_FLAG ((T)->flags, OSPF_ROUTER_RESTART)                             \
     && (T)->restart_method == OSPF_RESTART_METHOD_GRACEFUL)
#define IS_RESTART_SIGNALING(T)                                               \
    (CHECK_FLAG ((T)->flags, OSPF_ROUTER_RESTART)                             \
     && (T)->restart_method == OSPF_RESTART_METHOD_SIGNALING)

/* OSPF-NSM restart option TLV header

    0                   1                   2                   3
    0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |              Type             |             Length            |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

 */

struct ospf_restart_tlv
{
  u_int16_t type;
  u_int16_t length;
  u_char data[1];
};

#define OSPF_NSM_RESTART_REASON_TLV                      2
#define OSPF_NSM_RESTART_CRYPT_SEQNUM_TLV                4
#define OSPF_NSM_RESTART_VLINK_CRYPT_SEQNUM_TLV          5
#ifdef HAVE_OSPF_MULTI_AREA
#define OSPF_NSM_RESTART_MULTI_AREA_LINK_CRYPT_SEQNUM_TLV 6
#endif /* HAVE_OSPF_MULTI_AREA */

/* OSPF-NSM restart option - restart reason TLV

    0                   1                   2                   3
    0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |                2              |                 1             |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |     Reason      |
   +-+-+-+-+-+-+-+-+-+

*/

/* OSPF-NSM restart option - cryptographic sequence number TLV

    0                   1                   2                   3
    0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |                4              |              Length           |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |                       IP Interface Address                    |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |                         Interface Index                       |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |                   Cryptographic Sequence Number               |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |                              ...                              |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |                       IP Interface Address                    |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |                         Interface Index                       |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |                   Cryptographic Sequence Number               |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

 */

/* OSPF-NSM restart option - cryptographic sequence number TLV for Virtual-Link

    0                   1                   2                   3
    0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |                5              |              Length           |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |                        OSPF instance ID                       |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |                             Area ID                           |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |                           Neighbor ID                         |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |                   Cryptographic Sequence Number               |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |                              ...                              |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |                        OSPF instance ID                       |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |                             Area ID                           |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |                           Neighbor ID                         |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |                   Cryptographic Sequence Number               |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

 */

/* Link-Local Signaling Definition. */
#define OSPF_LLS_TLV_EXTENDED_OPTIONS           1
#define OSPF_LLS_TLV_CRYPT_AUTH                 2

#define OSPF_LLS_HEADER_LEN                     4
#define OSPF_LLS_TLV_HEADER_LEN                 4
#define OSPF_LLS_TLV_EXTENDED_OPTIONS_LEN       4
#define OSPF_LLS_TLV_CRYPT_AUTH_LEN             20

#define OSPF_EXTENDED_OPTION_LR                 (1 << 0)
#define OSPF_EXTENDED_OPTION_RS                 (1 << 1)

struct ospf_lls_header
{
  u_int16_t checksum;
  u_int16_t length;
};

struct ospf_lls_tlv_header
{
  u_int16_t type;
  u_int16_t length;
};

struct ospf_lls_eo_tlv
{
  u_int32_t options;
};

#ifdef HAVE_MD5
struct ospf_lls_ca_tlv
{
  u_int32_t crypt_seqnum;
  u_char digest[OSPF_AUTH_MD5_SIZE];
};
#endif /* HAVE_MD5 */

struct ospf_lls
{
  struct ospf_lls_eo_tlv eo;
#ifdef HAVE_MD5
  struct ospf_lls_ca_tlv ca;
#endif /* HAVE_MD5 */
};

/* Prototypes. */
int ospf_restart_router_lsa_check (struct ospf *, struct ospf_lsa *);
void ospf_restart_state_unset (struct ospf *);
void ospf_restart_exit (struct ospf *);
int ospf_restart_timer (struct thread *);
int ospf_restart_stale_timer (struct thread *);
int ospf_restart_state_timer (struct thread *);
int ospf_restart_signaling_state_check_timer (struct thread *thread);
int ospf_grace_ack_wait_timer (struct thread *thread);
void ospf_grace_lsa_recv_check (struct ospf *);
void ospf_restart_helper_enter_by_lsa (struct ospf_lsa *);
void ospf_restart_helper_exit_by_lsa (struct ospf_lsa *);
void ospf_restart_helper_exit (struct ospf_neighbor *);
void ospf_restart_helper_exit_all (struct ospf *);
u_int32_t ospf_restart_crypt_seqnum_lookup (struct ospf *,
                                            struct pal_in4_addr *, u_int32_t);
u_int32_t ospf_restart_vlink_crypt_seqnum_lookup (struct ospf *, u_int16_t,
                                                  struct pal_in4_addr *,
                                                  struct pal_in4_addr *);
#ifdef HAVE_OSPF_MULTI_AREA
u_int32_t ospf_restart_multi_area_link_crypt_seqnum_lookup (struct ospf *,
                                                       u_int16_t ,
                                                       struct pal_in4_addr *,
                                                       struct pal_in4_addr *);
#endif /* HAVE_OSPF_MULTI_AREA */
u_char ospf_restart_reason_lookup (struct ospf *);
void ospf_grace_lsa_flood_force (struct ospf_interface *);
void ospf_restart_option_tlv_parse (struct ospf_master *, struct stream *);
void ospf_grace_lsa_originate (struct ospf_interface *);
void ospf_restart_set_dr_by_hello (struct ospf_neighbor *,
                                   struct ospf_hello *);

u_int16_t ospf_restart_lls_size_get (struct ospf_interface *);
void ospf_restart_lls_block_set (struct ospf_interface *,
                                 struct ospf_packet *);
int ospf_restart_lls_block_get (struct ospf_header *, struct ospf_interface *,
                                u_char *, u_int16_t, struct ospf_lls *);
void ospf_restart_extended_options_check (struct ospf_neighbor *, u_int32_t);
int ospf_restart_dd_resync_proc (struct ospf_neighbor *,
                                 struct ospf_header *, struct ospf_db_desc *);

int ospf_restart_finish (struct ospf_master *);

#endif /* _PACOS_OSPF_RESTART_H */
