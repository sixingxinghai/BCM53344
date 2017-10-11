/* Copyright (C) 2001-2003  All Rights Reserved. */

#ifndef _NSM_QOS_FILTER_H
#define _NSM_QOS_FILTER_H


#ifdef HAVE_QOS
enum qos_filter_type
  {
    QOS_FILTER_DENY,
    QOS_FILTER_PERMIT,
    QOS_FILTER_DYNAMIC,
    QOS_FILTER_SOURCE,
    QOS_FILTER_DESTINATION,
    QOS_FILTER_NO_MATCH
  };

enum qos_access_type
  {
    QOS_ACCESS_TYPE_STRING,
    QOS_ACCESS_TYPE_NUMBER
  };

enum qos_operation
  {
    QOS_NOOP,
    QOS_EQUAL,
    QOS_NOT_EQUAL,
    QOS_LESS_THAN,
    QOS_GREATER_THAN,
    QOS_OPERATION_MAX
  };

enum qos_protocol
  {
    QOS_ANY_PROTO,
    QOS_ETHERNET_II_PROTO,
    QOS_802_3_PROTO,
    QOS_SNAP_PROTO,
    QOS_LLC_PROTO,
    QOS_IP_PROTO,
    QOS_ICMP_PROTO,
    QOS_UDP_PROTO, 
    QOS_TCP_PROTO,
    QOS_PROTOCOL_MAX
  };

/* Access list */
struct qos_access_list
{
  char *name;
  char *remark;

  /* Acl type information - MAC or IP */
  int  acl_type;

  /* Reference count. */
  u_int32_t ref_cnt;

  struct nsm_qos_acl_master *master;

  enum qos_access_type type;

  struct qos_access_list *next;
  struct qos_access_list *prev;

  struct qos_filter_list *head;
  struct qos_filter_list *tail;
};


struct mac_filter_addr
{
  u_int8_t mac[6];
};

#define NSM_QOS_INVALID_DOT1P_PRI        (0xffffffff)


struct mac_filter_common
{
  /* Common access-list */
  int extended;
  int packet_format;
  struct mac_filter_addr a;
  struct mac_filter_addr a_mask;
  struct mac_filter_addr m;
  struct mac_filter_addr m_mask;
  int priority;
};

struct ip_filter_common
{
  /* Common access-list */
  int extended;
  struct pal_in4_addr addr;
  struct pal_in4_addr addr_mask;
  struct pal_in4_addr mask;
  struct pal_in4_addr mask_mask;
  struct pal_in4_addr addr_config;
  struct pal_in4_addr mask_config;
};

struct qos_filter_pacos
{
  /* If this filter is "exact" match then this flag is set. */
  int exact;

  /* Prefix information. */
  struct prefix prefix;
};

struct qos_filter_pacos_ext
{
  s_int16_t protocol;

  /* Source prefix. */
  struct prefix sprefix;

  /* Source port operator. */
  enum qos_operation sport_op;

  /* Source port. */
  int sport;

  /* Destination prefix. */
  struct prefix dprefix;

  /* Destination port operator. */
  enum qos_operation dport_op;

  /* Destination port. */
  int dport;
};



/* Filter element of access list */
struct qos_filter_list
{
  /* For doubly linked list. */
  struct qos_filter_list *next;
  struct qos_filter_list *prev;

  /* Acl type information - MAC or IP */
  int acl_type;

  /* Filter type information. */
  enum qos_filter_type type;

  /* Cisco access-list */
  int common;

#define QOS_FILTER_COMMON         0
#define QOS_FILTER_PACOS          1
#define QOS_FILTER_PACOS_EXT      2

  union
  {
    struct mac_filter_common mfilter;
    struct ip_filter_common ifilter;
    struct qos_filter_pacos zfilter;
    struct qos_filter_pacos_ext zextfilter;
  } u;
};

/* List of access_list. */
struct qos_access_list_list
{
  struct qos_access_list *head;
  struct qos_access_list *tail;
};


/* Master structure of access_list. */
struct nsm_qos_acl_master
{
  /* List of access_list which name is number. */
  struct qos_access_list_list num;

  /* List of access_list which name is string. */
  struct qos_access_list_list str;

#if 0 /* FFS */
  /* Hook function which is executed when new access_list is added. */
  void (*add_hook) (struct apn_vr *,
                    struct access_list *,
                    struct filter_list *);

  /* Hook function which is executed when access_list is deleted. */
  void (*delete_hook) (struct apn_vr *,
                       struct access_list *,
                       struct filter_list *);
#endif /* FFS */
};


/* Prototypes. */
struct nsm_qos_acl_master * nsm_qos_acl_master_get (struct nsm_master *nm);
struct qos_filter_list * qos_filter_new ();
void qos_filter_free (struct qos_filter_list *filter);
struct qos_access_list * qos_access_list_new ();
void qos_access_list_free (struct qos_access_list *access);
struct qos_access_list * qos_access_list_lock (struct qos_access_list *access);
void qos_access_list_unlock (struct qos_access_list *access);
void qos_access_list_delete (struct qos_access_list *access);
struct qos_access_list * qos_access_list_insert (struct nsm_qos_acl_master *master,
                                                 const char *name);
struct qos_access_list * qos_access_list_lookup (struct nsm_qos_acl_master *master,
                                                 const char *name);
struct qos_access_list * qos_access_list_get (struct nsm_qos_acl_master *master,
                                              const char *name);
int qos_access_list_filter_add (struct nsm_qos_acl_master *master,
                                struct qos_access_list *access,
                                struct qos_filter_list *filter);
void qos_access_list_filter_delete (struct nsm_qos_acl_master *master,
                                    struct qos_access_list *access,
                                    struct qos_filter_list *filter);
struct qos_filter_list * qos_filter_lookup_common (struct qos_access_list *access,
                                                   struct qos_filter_list *mnew);
int qos_filter_set_common (struct nsm_qos_acl_master *master,
                           char *name_str,
                           int type,
                           char *addr_str,
                           char *addr_mask_str,
                           char *mask_str,
                           char *mask_mask_str,
                           int extended,
                           int set,
                           int acl_type,
                           int ether_type);
int qos_filter_set_hw_common (struct nsm_qos_acl_master *master,
                           char *name_str,
                           int type,
                           char *addr_str,
                           char *addr_mask_str,
                           char *mask_str,
                           char *mask_mask_str,
                           int extended,
                           int set,
                           int acl_type,
                           int ether_type,
                           int priority);
void qos_config_write_access_pacos (struct cli *, struct qos_filter_list *);
void qos_config_write_access_common (struct cli *, struct qos_filter_list *);
void qos_config_write_access_pacos_ext (struct cli *, struct qos_filter_list *);
int qos_access_list_extended_set (struct nsm_qos_acl_master *master,
                                  char *name,
                                  int direct,
                                  char *addr_str,
                                  char *addr_mask_str,
                                  char *mask_str,
                                  char *mask_mask_str,
                                  int ether_type);

int qos_access_list_extended_unset (struct nsm_qos_acl_master *master,
                                    char *name,
                                    int direct,
                                    char *addr_str,
                                    char *addr_mask_str,
                                    char *mask_str,
                                    char *mask_mask_str,
                                    int ether_type);
result_t qos_filter_show (struct cli *cli, char *name);
int nsm_acl_list_clean (struct nsm_qos_acl_master *master);
int ip_access_list_standard_set (struct nsm_qos_acl_master *master,
                                 char *name,
                                 int direct,
                                 char *addr_str,
                                 char *addr_mask_str,
                                 int acl_type,
                                 int packet_format);
int mac_access_list_extended_set (struct nsm_qos_acl_master *master,
                                  char *name,
                                  int direct,
                                  char *addr_str,
                                  char *addr_mask_str,
                                  char *mask_str,
                                  char *mask_mask_str,
                                  int acl_type,
                                  int packet_format);
int mac_access_list_extended_hw_set (struct nsm_qos_acl_master *master,
                                  char *name,
                                  int direct,
                                  char *addr_str,
                                  char *addr_mask_str,
                                  char *mask_str,
                                  char *mask_mask_str,
                                  int acl_type,
                                  int packet_format,
                                  int priority);
int ip_access_list_standard_unset (struct nsm_qos_acl_master *master,
                                   char *name,
                                   int direct,
                                   char *addr_str,
                                   char *addr_mask_str,
                                   int acl_type,
                                   int packet_format);
int ip_access_list_extended_set (struct nsm_qos_acl_master *master,
                                 char *name,
                                 int direct,
                                 char *addr_str,
                                 char *addr_mask_str,
                                 char *mask_str,
                                 char *mask_mask_str,
                                 int acl_type,
                                 int packet_format);
int ip_access_list_extended_unset (struct nsm_qos_acl_master *master,
                                   char *name,
                                   int direct,
                                   char *addr_str,
                                   char *addr_mask_str,
                                   char *mask_str,
                                   char *mask_mask_str,
                                   int acl_type,
                                   int packet_format);
int qos_compare_mac_filter (u_int8_t *start_addr, u_int8_t val);
result_t qos_all_filter_show (struct cli *cli);
int mac_access_list_extended_unset (struct nsm_qos_acl_master *master,
                                    char *name,
                                    int direct,
                                    char *addr_str,
                                    char *addr_mask_str,
                                    char *mask_str,
                                    char *mask_mask_str,
                                    int acl_type,
                                    int packet_format);
int mac_access_list_extended_hw_unset (struct nsm_qos_acl_master *master,
                                    char *name,
                                    int direct,
                                    char *addr_str,
                                    char *addr_mask_str,
                                    char *mask_str,
                                    char *mask_mask_str,
                                    int acl_type,
                                    int packet_format,
                                    int priority);
#endif /* HAVE_QOS */
#endif /* _NSM_QOS_FILTER_H */
