/* Copyright (C) 2001-2009  All Rights Reserved. */




#ifndef _PACOS_FILTER_H
#define _PACOS_FILTER_H

/* Kernel defines for ICMP Values needed for PacOS extended CLIs */
#define ICMP_ECHOREPLY          0       /* Echo Reply                   */
#define ICMP_DEST_UNREACH       3       /* Destination Unreachable      */
#define ICMP_SOURCE_QUENCH      4       /* Source Quench                */
#define ICMP_REDIRECT           5       /* Redirect (change route)      */
#define ICMP_ECHO               8       /* Echo Request                 */
#define ICMP_TIME_EXCEEDED      11      /* Time Exceeded                */
#define ICMP_PARAMETERPROB      12      /* Parameter Problem            */
#define ICMP_TIMESTAMP          13      /* Timestamp Request            */
#define ICMP_TIMESTAMPREPLY     14      /* Timestamp Reply              */
#define ICMP_INFO_REQUEST       15      /* Information Request          */
#define ICMP_INFO_REPLY         16      /* Information Reply            */
#define ICMP_ADDRESS            17      /* Address Mask Request         */
#define ICMP_ADDRESSREPLY       18      /* Address Mask Reply           */
#define NR_ICMP_TYPES           18

/* Number of access-list filter entries. */
#define ACCESS_LIST_ENTRY_MAX         4294967294UL

/* Filter type is made by `permit', `deny' and `dynamic'. */
enum filter_type
{
  FILTER_DENY,
  FILTER_PERMIT,
  FILTER_DYNAMIC,
  FILTER_NO_MATCH
};

enum access_type
{
  ACCESS_TYPE_STRING,
  ACCESS_TYPE_NUMBER
};

enum filter_ifdir
{
  IFDIR_NONE,
  IFDIR_INPUT,
  IFDIR_OUTPUT
};

enum operation
{
  NOOP,
  EQUAL,
  NOT_EQUAL,
  LESS_THAN,
  GREATER_THAN,
  RANGE,
  OPERATION_MAX
};

#define FILTER_MAX_PROTO_NUM 255
enum protocol
{
  IP_PROTO    =   0,
  ICMP_PROTO  =   1,
  IGMP_PROTO  =   2,
  IPV4_PROTO  =   4,
  TCP_PROTO   =   6,
  UDP_PROTO   =  17,
  IPV6_PROTO  =  41,
  RSVP_PROTO  =  46,
  GRE_PROTO   =  47,
  ESP_PROTO   =  50,
  AH_PROTO    =  51,
  IPV6_ICMP_PROTO  =  58,
  OSPF_PROTO  =  89,
  PIM_PROTO   = 103,
  IPCOMP_PROTO= 108,
  VRRP_PROTO  = 112,
  ANY_PROTO   = 256,
};

/* Supported in iptables TOS values. */
typedef enum tos_values
{
  TOS_NORMAL_SERVICE  = 0,
  TOS_MIN_COST        = 2,
  TOS_MAX_RELIABILITY = 4,
  TOS_MAX_THROUGHPUT  = 8,
  TOS_MIN_DELAT       = 16
} TOS_VALUES;


/* Access list */
struct access_list
{
  char *name;
  char *remark;

  /* Reference count. */
  u_int32_t ref_cnt;

  struct access_master *master;

  enum access_type type;

  struct access_list *next;
  struct access_list *prev;

  struct filter_list *head;
  struct filter_list *tail;
};

struct filter_common
{
  /* Common access-list */
  int extended;
  struct pal_in4_addr addr;
  struct pal_in4_addr addr_mask;
  struct pal_in4_addr mask;
  struct pal_in4_addr mask_mask;
};

struct filter_pacos
{
  /* If this filter is "exact" match then this flag is set. */
  int exact;

  /* Prefix information. */
  struct prefix prefix;
};

struct filter_pacos_ext
{
  s_int16_t protocol;

  /* Source prefix. */
  struct prefix_am sprefix;

  /* Source port operator. */
  enum operation sport_op;

  /* Source port: alone or high in range. */
  int sport;

  /* Source port: low in range */
  int sport_lo;

  /* Destination prefix. */
  struct prefix_am dprefix;

  /* Destination port operator. */
  enum operation dport_op;

  /* Destination port: alone or high in range. */
  int dport;

  /* Destination port: low in range. */
  int dport_lo;

  /* TCP established: 1 => set; 0 - unset; */
  int established;

  /* ICMP-Type: set if != -1 */
  int icmp_type;

  /* Set if != -1. */
  int precedence;

  /* A numeric label */
  int label;

  /* TOS op code only: set if != -1; NOOP (single value) or RANGE. */
  enum operation tos_op;

  /* TOS value: alone or high in range */
  int tos;

  /* TOS value: low in range */
  int tos_lo;

  /* Packet size: LESS_THAN, GREATER_THAN or RANGE. */
  enum operation  pkt_size_op;
  int pkt_size;
  int pkt_size_lo;

  /* Fragments: 1: set - 0: not set. */
  int fragments;

  /* Log flag: 1: set - 0: not set */
  int log;

  /* Interface direction: DIR_NONE => no interface; DIR_IN or DIR_OUT*/
  enum filter_ifdir ifdir;
  char ifname[INTERFACE_NAMSIZ + 1];
};

/* Filter element of access list */
struct filter_list
{
  /* For doubly linked list. */
  struct filter_list *next;
  struct filter_list *prev;

  /* Filter type information. */
  enum filter_type type;

  /* Cisco access-list */
  int common;
#define FILTER_COMMON         0
#define FILTER_PACOS          1
#define FILTER_PACOS_EXT      2

  union
  {
    struct filter_common cfilter;
    struct filter_pacos zfilter;
    struct filter_pacos_ext zextfilter;
  } u;
};

/* List of access_list. */
struct access_list_list
{
  struct access_list *head;
  struct access_list *tail;
};

struct apn_vr;

/* Defines operation codes used by the new style notification function.
*/
typedef enum filter_opcode
{
  FILTER_OPCODE_DELETE,
  FILTER_OPCODE_CREATE
} filter_opcode_t;

/* Defines a callback function type used by the new style notification function.
*/
typedef int (* filter_ntf_cb_t)(struct apn_vr *,
                                struct access_list *,
                                struct filter_list *,
                                filter_opcode_t op_code);

/* Master structure of access_list. */
struct access_master
{
  afi_t afi;

  /* List of access_list which name is number. */
  struct access_list_list num;

  /* List of access_list which name is string. */
  struct access_list_list str;

  /* Hook function which is executed when new access_list is added. */
  void (*add_hook) (struct apn_vr *,
                    struct access_list *,
                    struct filter_list *);

  void (*delete_hook) (struct apn_vr *,
                       struct access_list *,
                       struct filter_list *);
  u_int32_t max_count;
};

struct cli;

/* Prototypes. */
void access_list_add_hook (struct apn_vr *,
                           void (*func) (struct apn_vr *,
                                         struct access_list *,
                                         struct filter_list *));
void access_list_delete_hook (struct apn_vr *,
                              void (*func) (struct apn_vr *,
                                            struct access_list *,
                                            struct filter_list *));
/* New style notification function. */
result_t filter_set_ntf_cb (struct lib_globals *zg, filter_ntf_cb_t ntf_cb);

struct access_list *access_list_lock (struct access_list *);
void access_list_unlock (struct access_list *);
void access_list_delete (struct access_list *);
struct access_list * access_list_insert (struct apn_vr *, afi_t, const char *);
enum filter_type access_list_apply (struct access_list *, void *);
enum filter_type access_list_custom_apply (struct access_list *,
                                           result_t (*) (void *, void *),
                                           void *);
enum filter_type filter_list_type_apply (struct filter_list *, void *,
                                         enum filter_type);
struct access_list *access_list_lookup (struct apn_vr *, afi_t, const char *);
s_int16_t protocol_type (char *);
bool_t access_list_reference_validate (char *);
struct access_list *access_list_get (struct apn_vr *, afi_t, const char *);
int filter_set_common (struct apn_vr *, char *, int, char *, char *, char *,
                       char *, int, int);
int filter_set_pacos (struct apn_vr *, char *, int, afi_t, char *, int, int);

result_t filter_set_pacos_extended (struct cli *, const char *,
                                    const char *, afi_t, const char *,
                                    const char *, const char *, const char *,
                                    const char *, const char *, const char *,
                                    const char *, const char *,
                                    u_char);

void filter_get_prefix (struct filter_list *, struct prefix *);
struct filter_list * access_list_first_filter_prefix (struct access_list *,
    struct prefix *);
struct filter_list * access_list_next_filter_prefix (struct filter_list *,
    struct prefix *);

void config_write_access_pacos_ext (struct cli *, struct filter_list *);
int config_write_access_ipv4 (struct cli *);
int config_encode_access_ipv4 (struct apn_vr *vr, cfg_vect_t *cv);

#ifdef HAVE_IPV6
int config_write_access_ipv6 (struct cli *);
int config_encode_access_ipv6 (struct apn_vr *vr, cfg_vect_t *cv);
#endif /* HAVE_IPV6 */

void access_list_init (struct lib_globals *);
void access_list_finish (struct apn_vr *);

#endif /* _PACOS_FILTER_H */
