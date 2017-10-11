/* Copyright (C) 2002-2003  All Rights Reserved. */

#ifndef _PACOS_IMI_FW_H
#define _PACOS_IMI_FW_H

/* Length defines. */
#define IMI_NAT_ADDRESS_POOL_NAME_LEN 50
#define PREFIX_STR_LEN                100
#define PROTO_STR_LEN                 50
#define CHAIN_STR_LEN                 50
#define PORT_STR_LEN                  50
#define OPT_STR_LEN                   50

/* NAT address pool range. */
struct imi_nat_address_pool
{
  char name [IMI_NAT_ADDRESS_POOL_NAME_LEN];
  char min_ip[IPV4_MAX_PREFIXLEN];
  char max_ip[IPV4_MAX_PREFIXLEN];
  char mask[IPV4_MAX_PREFIXLEN];
  int  refcnt;
};

struct imi_rule_static
{
  /* Valid fields flag. */
  int flags;
#define IMI_RULE_PROTOCOL                (1 << 0)
#define IMI_RULE_IN_INTERFACE            (1 << 1)
#define IMI_RULE_OUT_INTERFACE           (1 << 2)
#define IMI_RULE_SOURCE                  (1 << 3)
#define IMI_RULE_DESTINATION             (1 << 4)
#define IMI_RULE_SOURCE_RANGE            (1 << 5)
#define IMI_RULE_DESTINATION_RANGE       (1 << 6)

  /* Inverse flag. */
  int inverse_flag;
#define IMI_INVERSE_PROTOCOL             (1 << 0)
#define IMI_INVERSE_ININTERFACE          (1 << 1)
#define IMI_INVERSE_OUTINTERFACE         (1 << 2)
#define IMI_INVERSE_SADDR                (1 << 3)
#define IMI_INVERSE_DADDR                (1 << 4)

  /* Protocol. */
  u_int16_t protocol;

  /* In interface. */
  char via_in [INTERFACE_NAMSIZ + 1];

  /* Out interface. */
  char via_out [INTERFACE_NAMSIZ + 1];

  /* Source. */
  struct prefix_am4 sprefix;

  /* Source port. */
  u_int16_t sport;

  /* Destination port. */
  u_int16_t dport;

  /* Destination. */
  struct prefix_am4 dprefix;

  /* Translation prefix. */
  struct pal_in4_addr min_ip;
  struct pal_in4_addr max_ip;
  u_int16_t min_port;
  u_int16_t max_port;
};

struct imi_rule_acl
{
  char acl[25];
  char pool[25];
};

struct imi_rule
{
  int flag;
#define IMI_RULE_ACL           0
#define IMI_RULE_STATIC        1

  int path;
  /* Filter paths. */
#define IMI_FILTER_INPUT       0
#define IMI_FILTER_OUTPUT      1
#define IMI_FILTER_FORWARD     2
  int filter_ref_cnt;
  union
  {
    struct imi_rule_acl arule;
    struct imi_rule_static srule;
  } u;
};

#define IMI_NAT_INSIDE         0
#define IMI_NAT_OUTSIDE        1
#define IMI_NAT_DIRECTION_MAX  2

#define IMI_NAT_SOURCE         0
#define IMI_NAT_DESTINATION    1
#define IMI_NAT_TRANS_MAX      2


struct imi_nat_ds_rlist
{
  struct list ds_rlist;
};

/* Representation of a single chain. */
struct imi_nat_chain
{
  u_int16_t    static_cnt;
  struct list  acl_num_list;
};


/* This will define the maximum number of transaltion lists.
*/
#define IMI_NAT_MAX_RLISTS (IMI_NAT_DIRECTION_MAX * IMI_NAT_TRANS_MAX)

/* This will help us to get the translation list for a give direction/scope
*/
#define IMI_NAT_RLIST(direction, scope) \
    &nat_table.rlist_tbl[((direction) << 1) | (scope)].ds_rlist

/* This will help us to find the number of translations for a given
   direction/scope.
 */
#define IMI_NAT_RLIST_COUNT(direction, scope) \
     LISTCOUNT (IMI_NAT_RLIST(direction, scope))

struct imi_nat_table
{
  struct imi_nat_ds_rlist rlist_tbl[IMI_NAT_MAX_RLISTS];


  /* Indexed with "scope": IMI_NAT_SOURCE, IMI_NAT_DESTINATION. */

  struct imi_nat_chain chain_tbl[2];
};

/* NAT address pool list of struct imi_nat_address_pool */
struct list *imi_nat_address_pool_list;

/* NAT table. */
struct imi_nat_table nat_table;


/* Filter targets. */
#define IMI_FILTER_DENY        0
#define IMI_FILTER_ACCEPT      1

/* Number of options. */
#define OPT_NONE                0
#define OPT_SOURCE              1
#define OPT_DESTINATION         2
#define OPT_PROTOCOL            3
#define OPT_VIANAMEIN           4
#define OPT_VIANAMEOUT          5
#define OPT_SPORT               6
#define OPT_DPORT               7
#define OPT_TO_SOURCE           8
#define OPT_TO_DESTINATION      9
#define NUMBER_OF_OPT           10

#define IMI_NAT_MAX_TIME_OUTS   6
struct imi_nat_timeouts
{
  int flag;
#define IMI_GENERIC_TIMEOUT       (1 << 0)
#define IMI_ICMP_TIMEOUT          (1 << 1)
#define IMI_TCP_TIMEOUT           (1 << 2)
#define IMI_TCP_FIN_TIMEOUT       (1 << 3)
#define IMI_UDP_TIMEOUT           (1 << 4)
#define IMI_DNS_TIMEOUT           (1 << 5)

  int generic_timeout;
  int icmp_timeout;
  int tcp_timeout;
  int tcp_fin_timeout;
  int udp_timeout;
  int dns_timeout;

  int check_flag;
};

/* Error codes. */
#define IMI_NAT_SUCCESS            0
#define IMI_NAT_ERROR             -1
#define IMI_NAT_ACL_ERROR       -100
#define IMI_NAT_POOL_ERROR      -101
#define IMI_NAT_IF_ERROR        -102
#define IMI_NAT_NO_RULE_FOUND   -103
#define IMI_NAT_RULE_EXISTS     -104
#define IMI_NAT_RULE_NOT_EXISTS -105
#define IMI_NAT_ACL_NOT_PERMITTED -106
#define IMI_NAT_BAD_KERNEL_RULE  -107
#define IMI_NAT_POOL_NO_MATCH    -108
#define IMI_NAT_OUT_OF_MEM      -109


/* Prototypes. */
int imi_nat_config_write (struct cli *);
int imi_nat_config_encode (cfg_vect_t *cv);

result_t imi_nat_acl_ntf_cb (struct access_list *access,
                             struct filter_list *filter,
                             filter_opcode_t     opcode);
struct imi_nat_address_pool *imi_nat_address_pool_lookup (struct list *, char *);
int imi_nat_address_pool (char *, char *, char *, char *, u_int16_t);
void imi_nat_acl_hook (struct access_list *, struct filter_list *, int);
int imi_nat_translation_static (int, struct imi_rule *, int , int );
int imi_nat_translation_acl (int , struct imi_rule *, int , int );
int imi_nat_process_acl (int, int,
                         struct access_list *, struct imi_nat_address_pool *,
                         char *, char *, struct imi_interface *, int);
void imi_nat_init (struct lib_globals *);
void imi_nat_finish (struct lib_globals *);

int imi_nat_process_static (int direction, int scope,
                            struct imi_rule *rule,
                            char *via_in, char *via_out,
                            struct imi_interface *imi_if,
                            int op);
int imi_nat_cli_cmd_ret (struct cli *cli, int ret);
void imi_nat_translation_timeout_set (int nat_timeout);
void imi_nat_translation_timeout_unset (void);
void imi_nat_translation_icmp_timeout_set (int nat_timeout);
void imi_nat_translation_icmp_timeout_unset (void);
void imi_nat_translation_tcp_timeout_set (int nat_timeout);
void imi_nat_translation_tcp_timeout_unset ();
void imi_nat_translation_tcp_fin_timeout_set (int nat_timeout);
void imi_nat_translation_tcp_fin_timeout_unset ();
void imi_nat_translation_udp_timeout_set (int nat_timeout);
void imi_nat_translation_udp_timeout_unset ();
void imi_nat_translation_timeout_get (struct imi_nat_timeouts  *nat_timeout);

#endif /* _PACOS_IMI_FW_H */
