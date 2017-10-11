/* Copyright (C) 2006 i All Rights Reserved. */

#ifndef _IPFIREWALL_H
#define _IPFIREWALL_H

#ifdef HAVE_IPNET

#define IP_IFNAMSIZ 16
/*
 *===========================================================================
 *                      IPFIREWALL_USERDEF_ID_SIZE
 *===========================================================================
 * The maximum length of the user defined function identifier
 *
 */
#define IPFIREWALL_USERDEF_ID_SIZE  30

/*
 *===========================================================================
 *                      IPFIREWALL_USERDEF_PARAM_SIZE
 *===========================================================================
 * The maximum length of the user defined function parameter string
 *
 */
#define IPFIREWALL_USERDEF_PARAM_SIZE  64


/*
 *===========================================================================
 *                      IPFIREWALL_USERDEF_INFO_SIZE
 *===========================================================================
 * The length of the user defined information buffer size
 *
 */
#define IPFIREWALL_USERDEF_INFO_SIZE  256
/*
 *===========================================================================
 *                         IPFIREWALL_MAX_LOG_ENTRIES
 *===========================================================================
 *
 */
#define IPFIREWALL_MAX_LOG_ENTRIES  100


/*
 *===========================================================================
 *                         IPFIREWALL_MAX_RULES
 *===========================================================================
 *
 */
#define IPFIREWALL_MAX_RULES  100


/*
 *===========================================================================
 *                         Rule "port" keyword operators
 *===========================================================================
 */
#define     IPFIREWALL_OP_EQ            0
#define     IPFIREWALL_OP_NE            1
#define     IPFIREWALL_OP_LT            2
#define     IPFIREWALL_OP_GT            3
#define     IPFIREWALL_OP_LE            4
#define     IPFIREWALL_OP_GE            5
#define     IPFIREWALL_OP_OUTRANGE      6
#define     IPFIREWALL_OP_INRANGE       7


/*
 *===========================================================================
 *                         TCP flags
 *===========================================================================
 */
#define IPFIREWALL_TCPFLAG_URGENT   0x0020
#define IPFIREWALL_TCPFLAG_ACK      0x0010
#define IPFIREWALL_TCPFLAG_PUSH     0x0008
#define IPFIREWALL_TCPFLAG_RESET    0x0004
#define IPFIREWALL_TCPFLAG_SYN      0x0002
#define IPFIREWALL_TCPFLAG_FIN      0x0001


/*
 *===========================================================================
 *                      IPFIREWALL_CTRL_XXX
 *===========================================================================
 *  Defines the available firewall control commands used to pass data to and
 * from the the firewall kernel.
 *
 */
#define IPFIREWALL_CTRL_ENABLE           0
#define IPFIREWALL_CTRL_DISABLE          1

#define IPFIREWALL_CTRL_ADD_RULE         2
#define IPFIREWALL_CTRL_DEL_RULE         3
#define IPFIREWALL_CTRL_FLUSH_RULES      4
#define IPFIREWALL_CTRL_GET_RULE         5

#define IPFIREWALL_CTRL_GET_INFO         6
#define IPFIREWALL_CTRL_CLEAR_STATS      7

#define IPFIREWALL_CTRL_FLUSH_STATES     8
#define IPFIREWALL_CTRL_GET_STATE        9

#define IPFIREWALL_CTRL_FLUSH_LOG        10
#define IPFIREWALL_CTRL_GET_LOG          11

#define IPFIREWALL_CTRL_FLUSH_USERDEFS   12
#define IPFIREWALL_CTRL_GET_USERDEF      13

#define IPFIREWALL_CTRL_FLUSH_GROUP      14
#ifdef HAVE_IPV6
#define IP_IPV6_FW               51   /* Ipfirewall_ctrl; Firewall control */
#endif /* HAVE_IPV6 */
#define IP_IP_FW                51  /* Ipfirewall_ctrl; Firewall control */

/*
 *===========================================================================
 *                      IPFIREWALL_MAX_RULE_ARGS
 *===========================================================================
 *  Defines the maximum number of arguments in a firewall rule string
 *
 */
#define IPFIREWALL_MAX_RULE_ARGS    16


/*
 *===========================================================================
 *                      IPFIREWALL_MS_PER_SEC/MIN/HOUR/DAY
 *===========================================================================
 *
 */

#define IPFIREWALL_MS_PER_SEC   1000
#define IPFIREWALL_MS_PER_MIN   60000
#define IPFIREWALL_MS_PER_HOUR  3600000
#define IPFIREWALL_MS_PER_DAY   86400000

/*
 ****************************************************************************
 * 5                    TYPES
 ****************************************************************************
 */
typedef s_int32_t (* Ipfirewall_userdef_match) (Ipcom_pkt *pkt, void *cookie, void *info);
typedef s_int32_t (* Ipfirewall_userdef_check) (void *cookie, void *info, u_int32_t infolen);
typedef void (* Ipfirewall_userdef_destroy) (void *cookie, void *info, u_int32_t infolen);

/* Some Ipcom lib structures */
/*
 *===========================================================================
 *                         Ipcom_list
 *===========================================================================
 * List head and entry structure.
 */
typedef struct Ipcom_list_struct
{
  struct Ipcom_list_struct   *next;
  struct Ipcom_list_struct   *prev;
  struct Ipcom_list_struct   *head;
  u_int32_t                      size;
}
Ipcom_list;

typedef unsigned (*Ipcom_hash_obj_func)(void *obj);
typedef unsigned (*Ipcom_hash_key_func)(void *key);
typedef bool_t (*Ipcom_hash_cmp_func)(void *obj, void *key);
/*
 *===========================================================================
 *                         Ipcom_hash
 *===========================================================================
 * List head and entry structure.
 */
typedef struct Ipcom_hash_struct
{
    /* Function that creates a hash value from an object */
    Ipcom_hash_obj_func   obj_hash_func;

    /* Pointer to a function that creates a hash value from a search key */
    Ipcom_hash_key_func   key_hash_func;

    /* Function that compares an objects and a search key and return the result */
    Ipcom_hash_cmp_func   obj_key_cmp;

    unsigned  elem;     /* The number of elements currently in the table */
    unsigned  size;     /* The size of the hash table */
    void    **table;    /* The hash table */
}
Ipcom_hash;

/* Signature of timeout handler */
typedef void (*Ipnet_timeout_handler)(void *cookie);

typedef struct Ipnet_timeout_struct
{
    s_int32_t                           pq_index;  /* The index this timout has in the priority queue */
    u_int32_t                        msec;      /* The absolute time when this timeout is triggered */
    Ipnet_timeout_handler         handler;   /* The function that will be called when
                                                the current time is equal to 'timeout_msec' */
    void                         *cookie;    /* The (user specified) argument to the handler */
    struct Ipnet_timeout_struct **ptmo;      /* Pointer to the location the caller has stored the
                                                Ipnet_timeout handler, that location will be set to
                                                IP_NULL when the timer expires or is cancled. */
}
Ipnet_timeout;

/*
 *===========================================================================
 *                    Ip_timeval
 *===========================================================================
 */
#ifndef IP_TIMEVAL_TYPE
#define IP_TIMEVAL_TYPE
struct Ip_timeval
{
    long   tv_sec;         /* Seconds. */
    long   tv_usec;        /* Microseconds. */
};
#endif


/*
 *===========================================================================
 *                         Ipfirewall_stats
 *===========================================================================
 */
typedef struct Ipfirewall_stats_struct
{
    u_int32_t in_block;
    u_int32_t in_pass;
    u_int32_t in_nomatch;
    u_int32_t out_block;
    u_int32_t out_pass;
    u_int32_t out_nomatch;
    u_int32_t invalid;
    u_int32_t in_logged_block;
    u_int32_t in_logged_pass;
    u_int32_t out_logged_block;
    u_int32_t out_logged_pass;
    u_int32_t log_failures;
    u_int32_t states_added;
    u_int32_t states_expired;
    u_int32_t state_hits;
    u_int32_t state_failures;
}
Ipfirewall_stats;


/*
 *===========================================================================
 *                         Ipfirewall_configuration
 *===========================================================================
 */
typedef struct Ipfirewall_configuration_struct
{
    s_int32_t timeout_icmp;            /* icmp timeout */
    s_int32_t timeout_udp;             /* udp timeout */
    s_int32_t timeout_tcp;             /* tcp timeout */
    s_int32_t timeout_other;           /* tcp other */
    s_int32_t max_states;            /* max number of state entries */
}
Ipfirewall_config;

/*
 *===========================================================================
 *                         Ipfirewall_data
 *===========================================================================
 */

typedef struct Ipfirewall_data_struct
{
    bool_t open;                   /* Opened or closed */
    u_int32_t num_states;
    u_int32_t num_out_rules;
    u_int32_t num_in_rules;
    Ipcom_list head_state;          /* State list */
    Ipcom_list head_rule;           /* Rule list */
    Ipcom_list head_log;            /* Log list */
    Ipcom_list head_user;           /* User defined list */
#ifdef IPCOM_USE_INET
    Ipcom_hash *hash_v4;            /* Hash table for v4 state entries */
#endif
#ifdef IPCOM_USE_INET6
    Ipcom_hash *hash_v6;            /* Hash table for v6 state entries */
#endif
    Ipfirewall_stats stats;         /* Statistics */
    Ipfirewall_config config;       /* Configuration */
}
Ipfirewall_data;

/*
 *===========================================================================
 *                         Ipfirewall_addr
 *===========================================================================
 */
typedef struct Ipfirewall_addr_struct
{
    union
    {
#ifdef IPCOM_USE_INET
        u_int32_t v4;                     /* Network endian */
#endif
#ifdef IPCOM_USE_INET6
        u_int32_t v6[4];                  /* Network endian */
#endif
    }
    addr;
    union
    {
#ifdef IPCOM_USE_INET
        u_int32_t v4;                     /* Network endian */
#endif
#ifdef IPCOM_USE_INET6
        u_int32_t v6[4];                  /* Network endian */
#endif
    }
    mask;
}
Ipfirewall_addr;
/*
 *===========================================================================
 *                         Ipfirewall_rule
 *===========================================================================
 */
typedef struct Ipfirewall_rule_struct
{
    Ipcom_list  list_rule;          /* List */
    u_int32_t     rule_no;            /* Rule number in this group */
    u_int32_t     log_hit;            /* Log hit */
    u_int32_t     limit_level;        /* Current bucket level */
    u_int32_t     limit_first;        /* Boolean */
    u_int32_t     limit_ms;           /* Timeout is millisec */
#ifdef IPNET
    Ipnet_timeout  *limit_tmo;      /* Timeout handle */
#elif defined(IPLITE)
    Iplite_timeout *limit_tmo;      /* Timeout handle */
#endif
    Ipfirewall_userdef_match user_match;    /* User defined match function */
    Ipfirewall_userdef_check user_check;    /* User defined check function */
    Ipfirewall_userdef_destroy user_destroy;  /* User defined destroy function */
    void        *userdef_cookie;    /* User defined function cookie */
    void        *userdef_info;      /* User defined info */
    u_int32_t userdef_info_size; /* User defined info length */

    /* Do not add any entries before the group_head member */
    u_int8_t       group_head;     /* TRUE if head of group */
    u_int32_t     group_no;       /* Group number */
    s_int32_t      family;         /* IP_AF_INET or IP_AF_INET6 */
    u_int8_t       block;          /* action: block or pass */
    u_int8_t       in;             /* direction: incoming our outgoing */
    u_int8_t       log;            /* "log" (optional) */

    u_int8_t       log_first;      /* "first" (optional) */
    u_int8_t       quick;          /* "quick" (optional) */
    u_int8_t       on;             /* "on" (optional) */
    u_int8_t       on_not;         /* "on !" (optional) */

    u_int8_t       ifname[IP_IFNAMSIZ]; /* Interface name */

    u_int8_t       tos;            /* "tos"(optional) */
    u_int8_t       ttl;            /* "ttl" (optional) */
    u_int8_t       proto;          /* "proto"(optional) */
    u_int8_t       tcpudp;         /* "tcp/udp" */

    u_int8_t       icmp_type;      /* "icmp-type" (optional) */
    u_int8_t       icmp_code;      /* "code" (optional) */
    u_int8_t       icmp_num;       /* icmp type */
    u_int8_t       code;           /* icmp code */

    u_int8_t       from;           /* "from" (optional) */
    u_int8_t       src_any;        /* any source address */
    u_int8_t       src_not;        /* NOT source address */
    u_int8_t       src_port;       /* "port" (optional) */

    Ipfirewall_addr src;        /* source address and mask */
    u_int32_t    src_op;          /* operator */
    u_int16_t    src_port_lo;     /* port low limit in case of '<>' or '><' operator */
    u_int16_t    src_port_hi;     /* port number or port high limit in case of '<>' or '><' operator */

    u_int8_t  to;                  /* "to" (optional) */
    u_int8_t  dst_any;             /* any destination address */
    u_int8_t  dst_not;             /* NOT destination address */
    u_int8_t  dst_port;            /* "port" (optional) */

    Ipfirewall_addr dst;            /* destination address and mask */
    u_int32_t         dst_op;         /* operator */
    u_int16_t         dst_port_lo;    /* port low limit in case of '<>' or '><' operator */
    u_int16_t         dst_port_hi;    /* port number or port high limit in case of '<>' or '><' operator */

    u_int8_t           with;           /* "with" (optional) */
    u_int8_t           with_no;        /* "with no" (optional) */
    u_int8_t           ipopt;          /* "ipopts" (optional) */
    u_int8_t           frag;           /* "frag" (optional) */

    u_int16_t         tcp_flags;      /* tcp flags */
    u_int16_t         tcp_mask;       /* tcp flags mask */

    u_int8_t           flags;          /* "flags" (optional) */
    u_int8_t           keep_state;     /* "keep state" (optional) */
    u_int8_t           ttl_val;        /* ttl value */
    u_int8_t           tos_val;        /* tos value */

    u_int8_t           tos_mask;       /* tos mask */
    u_int8_t           limit;          /* limit */
    u_int8_t           limit_not;      /* invert limit rule */
    u_int8_t           userdef;        /* userdef */

    u_int8_t           limit_period;   /* limit period */
    u_int8_t           protocol;       /* protocol number */
    u_int8_t           src_me;         /* 'me' source address */
    u_int8_t           dst_me;         /* 'me' destination address */

    u_int32_t         limit_num;      /* limit number */
    u_int32_t         burst_num;      /* burst number */

    u_int8_t           userdef_id[IPFIREWALL_USERDEF_ID_SIZE]; /* user defined function id */
    u_char            userdef_param[IPFIREWALL_USERDEF_PARAM_SIZE];  /* user defined function parameter */
}
Ipfirewall_rule;


/*
 *===========================================================================
 *                         Ipfirewall_tuple
 *===========================================================================
 */
typedef struct Ipfirewall_tuple_struct
{
    u_int8_t          version;
    u_int8_t          protocol;
    u_int16_t        icmp_id_n;
    u_int16_t        src_port_n;
    u_int16_t        dst_port_n;
    u_int8_t          is_frag;
    u_int8_t          first_frag;
    u_int16_t        hlen;
    union
    {
#ifdef IPCOM_USE_INET
        u_int32_t v4;
#endif
#ifdef IPCOM_USE_INET6
        u_int32_t v6[4];
#endif
    }src_n;
    union
    {
#ifdef IPCOM_USE_INET
        u_int32_t v4;
#endif
#ifdef IPCOM_USE_INET6
        u_int32_t v6[4];
#endif
    }dst_n;
}
Ipfirewall_tuple;

/*
 *===========================================================================
 *                         Ipfirewall_log
 *===========================================================================
 */
typedef struct Ipfirewall_log_struct
{
    Ipcom_list          list_log;
    struct Ip_timeval   time;                   /* Time when packet was sent/received */
    u_int8_t               ifname[IP_IFNAMSIZ];    /* Interface name */
    u_int32_t             ifindex;                /* Interface index */
    u_int32_t             group_no;               /* Group number that matched */
    u_int32_t             rule_no;                /* Rule number that matched */
    u_int8_t               block;
    u_int8_t               pad[3];
    Ipfirewall_tuple    tuple;
    u_int16_t             h_len;
    u_int16_t             ip_len;
    u_int16_t             tcp_flags;
    u_int8_t               icmp_type;
    u_int8_t               icmp_code;
}
Ipfirewall_log;


/*
 *===========================================================================
 *                         Ipfirewall_state
 *===========================================================================
 */
typedef struct Ipfirewall_state_struct
{
    Ipcom_list          list_state;     /* List */
#ifdef IPNET
    Ipnet_timeout       *tmo;           /* Timeout handle */
#elif defined(IPLITE)
    Iplite_timeout      *tmo;           /* Timeout handle */
#endif
    u_int32_t             sec;            /* Time until timeout */
    u_int32_t             group_no;       /* Rule group that generated this entry */
    u_int32_t             rule_no;        /* Rule number that generated this entry */
    u_int8_t               in;             /* 1 = incoming rule. 0 = outgoing rule. */
    u_int8_t               estab;          /* 1 = tcp session established. */
    u_int8_t               pad[2];
    Ipfirewall_tuple    tuple;
}
Ipfirewall_state;
/*
 *===========================================================================
 *                         Ipfirewall_user
 *===========================================================================
 */
typedef struct Ipfirewall_user_struct
{
    Ipcom_list                  list_user;          /* List */
    char                        identifier[IPFIREWALL_USERDEF_ID_SIZE]; /* Identifier */
    Ipfirewall_userdef_match    match;
    Ipfirewall_userdef_check    check;
    Ipfirewall_userdef_destroy  destroy;
    void                        *cookie;
}
Ipfirewall_user;


/*
 *===========================================================================
 *                         Ipfirewall_hash_key_v4
 *===========================================================================
 */
#ifdef IPCOM_USE_INET
typedef struct Ipfirewall_hash_key_v4_struct
{
    u_int32_t src_n;
    u_int32_t dst_n;
    u_int16_t src_port_n;
    u_int16_t dst_port_n;
    u_int8_t protocol;
}
Ipfirewall_hash_key_v4;
#endif
/*
 *===========================================================================
 *                         Ipfirewall_hash_key_v6
 *===========================================================================
 */

#ifdef IPCOM_USE_INET6
typedef struct Ipfirewall_hash_key_v6_struct
{
    u_int32_t src_n[4];
    u_int32_t dst_n[4];
    u_int16_t src_port_n;
    u_int16_t dst_port_n;
    u_int8_t protocol;
}
Ipfirewall_hash_key_v6;
#endif


/*
 *===========================================================================
 *                         Ipfirewall_ctrl
 *===========================================================================
 */
typedef struct Ipfirewall_ctrl_struct
{
    s_int32_t cmd;     /* Control command */
    s_int32_t seqno;   /* Optional sequence number */
    union
    {
        Ipfirewall_data     info;
        Ipfirewall_rule     rule;
        Ipfirewall_state    state;
        Ipfirewall_log      log;
        Ipfirewall_user     user;
    }
    type;   /* Optional type */
}
Ipfirewall_ctrl;

s_int32_t
pal_ipfirewall_get_rules (s_int32_t, struct pal_firewall_rule *);

s_int32_t
pal_ipfirewall_flush_rules (void);

s_int32_t
pal_ipfirewall_addremove_rule (struct pal_firewall_rule *, bool_t);
#endif /* HAVE_IPNET */

#endif /*_IPFIREWALL_H */
