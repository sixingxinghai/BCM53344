/* Copyright (C) 2001-2009  All Rights Reserved. */

#ifndef _PACOS_VRRP_H
#define _PACOS_VRRP_H

#include "vrrp_api.h"
/*
 * --------------
 * Global Defines
 * --------------
 */

/*
 * VRRP specific (static) defines
 */
#define         VRRP_FOUND                      1
#define         VRRP_OK                         0
#define         VRRP_FAILURE                    -1
#define         VRRP_IGNORE                     -2

#define         VRRP_SET                        0
#define         VRRP_UNSET                      -1

#define         MAX_VRIDS                       255

#define         VRRP_VERSION                    2
#define         VRRP_IPV6_VERSION               3
#define         VRRP_ADVT_TYPE                  1
#define         IPPROTO_VRRP                    112
#define         VRRP_MCAST_ADDR                 0xe0000012
#define         VRRP_IP_TTL                     255

#define         VRRP_ADVT_INT_DFLT              1

#define         VRRP_AUTH_NONE                  0
#define         VRRP_AUTH_RES_1                 1
#define         VRRP_AUTH_RES_2                 2

#define         MAX_AUTH_LEN                    256   /* remove */

#define         VRRP_PROTOCOL_IP                1

#ifdef HAVE_IPV6
#define         VRRP_PROTOCOL_IPV6              2
#define         VRRP_MCAST_ADDR6                "ff02::12"
#define         VRRP_IPV6_RSVD                  0
#define         VRRP_IPV6_CHECKSUM_OFFSET       6
#endif /* HAVE_IPV6 */

/* These two are set by the CLI command enable/disable */
#define         VRRP_ADMIN_STATE_UP             1
#define         VRRP_ADMIN_STATE_DOWN           2

#define         VRRP_NOTIFICATIONCNTL_ENA       1
#define         VRRP_NOTIFICATIONCNTL_DIS       2
#define         VRRP_MIN_OPER_PRIORITY          0
#define         VRRP_MAX_OPER_PRIORITY          255
#define         VRRP_MIN_ADVERTISEMENT_INTERVAL 1
#define         VRRP_MAX_ADVERTISEMENT_INTERVAL 4096

#define         VRRP_VMAC_DISABLE               0
#define         VRRP_VMAC_ENABLE                1

#define         VRRP_IPV4_MIN_ADVERT_LEN        20

#define         VRRP_DEFAULT_IP_OWNER_PRIORITY      255
#define         VRRP_DEFAULT_NON_IP_OWNER_PRIORITY  100

#define         IP_OWNER       "master"
#define         NOT_IP_OWNER   "backup"
#define         MAX_STR_LEN     255

#define         VRRP_BUFFER_SIZE_DEFAULT        2048

/*SNMP Trap Values */

#define VRRP_TRAP_ALL                            (0)
#define VRRP_TRAP_ID_MIN                         (1)
#define VRRP_TRAP_ID_MAX                         (2)
#define VRRP_TRAP_VEC_MIN_SIZE                   (1)

#define AUTH_TYPE_INVALID         1
#define AUTH_TYPE_MISMATCH        2
#define AUTH_TYPE_FAILURE         3


/* Macro to set default priority based on IP ownership. */
#define VRRP_SET_DEFAULT_PRIORITY(A)                                  \
    do {                                                              \
      if ((A)->owner == PAL_TRUE)                                     \
          (A)->prio = VRRP_DEFAULT_IP_OWNER_PRIORITY;                 \
      else                                                            \
          (A)->prio = VRRP_DEFAULT_NON_IP_OWNER_PRIORITY;             \
    } while (0)


/* IP address definition fro applications that must handle both
   IPv4 and IPv6 addresses using the same APIs.
   Such APIs must take an address family identifire to distinguish between
   these two types of addresses.
*/
typedef struct vrrp_inx_addr {
  union {
    u_int8_t            in_addr[16];
    struct pal_in4_addr in4_addr;
#ifdef HAVE_IPV6
    struct pal_in6_addr in6_addr;
#endif
  } uia;
} vrrp_inx_addr_t;
/*
 * ---------------
 * VRRP Structures
 * ---------------
 */

/* VRRP State. */
typedef enum vrrp_state {
  VRRP_STATE_INIT=1,
  VRRP_STATE_BACKUP,
  VRRP_STATE_MASTER
} vrrp_state_t;

#define VRRP_SESS_IS_DISABLED(sess) (sess->admin_state == VRRP_ADMIN_STATE_DOWN)

#define VRRP_SESS_IS_SHUTDOWN(sess) (sess->state == VRRP_STATE_INIT && \
                                     sess->admin_state == VRRP_ADMIN_STATE_UP)

#define VRRP_SESS_IS_RUNNING(sess) ((sess->state == VRRP_STATE_MASTER ||  \
                                     sess->state == VRRP_STATE_BACKUP) && \
                                     sess->admin_state == VRRP_ADMIN_STATE_UP)

typedef enum vrrp_init_msg_code
{
  VRRP_INIT_MSG_ADMIN_DOWN,
  VRRP_INIT_MSG_IF_NOT_RUNNING,
  VRRP_INIT_MSG_CIRCUIT_DOWN,
  VRRP_INIT_MSG_NO_SUBNET,
  VRRP_INIT_MSG_VIP_UNSET
} vrrp_init_msg_code_t;


#define         SIZE_VMAC_ADDR          sizeof(PAL_MAC_ADDR)
#define         SIZE_ARP_PACKET         (60*sizeof(char))
#ifdef HAVE_IPV6
#define         SIZE_ND_ADV_PACKET      (70*sizeof(char))
#endif /* HAVE_IPV6 */

/*                                                                           
 * VRRP Packet format for IPv4  *                                            
                                                                             
    0                   1                   2                   3            
    0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1          
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+         
   |Version| Type  | Virtual Rtr ID|   Priority    | Count IP Addrs|         
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+         
   |   Auth Type   |   Adver Int   |          Checksum             |         
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+         
   |                         IP Address (1)                        |         
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+         
   |                            .                                  |         
   |                            .                                  |         
   |                            .                                  |         
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+         
   |                         IP Address (n)                        |         
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+         
   |                     Authentication Data (1)                   |         
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+         
   |                     Authentication Data (2)                   |         
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+         
                                                                             
                                                                             
                                                                             
  *  VRRP Packet format for IPv6 *                                           
                                                                             
        0                   1                   2                   3        
       0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1       
      +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+      
      |Version| Type  | Virtual Rtr ID|   Priority    |Count IPv6 Addr|      
      +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+      
      |(rsvd) |       Adver Int       |          Checksum             |      
      +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+      
      |                                                               |      
      +                                                               +      
      |                       IPv6 Address(es)                        |      
      +                                                               +      
      +                                                               +      
      +                                                               +      
      +                                                               +      
      |                                                               |      
      +                                                               +      
      |                                                               |      
      +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+  */
                                                                             


/* Structure to hold the VRRP Packet. */
struct vrrp_advt_info
{
  /* Version & Type. */
  u_int8_t ver_type;

  /* VrId. */
  u_int8_t vrid;

  /* Priority. */
  u_int8_t priority;

  /* Number of IP addresses to backup. */
  u_int8_t num_ip_addrs;

  /* Auth type. */
  u_int8_t auth_type;

  /* Advertisement interval. */
  u_int8_t advt_int; /* rsvd + Adver Int */

  /* VRRP Checksum. */
  u_int16_t cksum;

  /* Virtual IP address. */
  struct pal_in4_addr vip_v4;

#ifdef HAVE_IPV6
  /* Virtual IPv6 address. */
  struct pal_in6_addr vip_v6;
#endif /*HAVE_IPV6 */

  /* Auth data. */
  unsigned int auth_data[2];

  vector vip;
};

#define         VRRP_ADVT_SIZE          sizeof(struct vrrp_advt_info) - sizeof (vector) 
/* IPV4 advertisment header length */
#ifdef HAVE_IPV6
#define         VRRP_ADVT_SIZE_V4       (VRRP_ADVT_SIZE - sizeof (struct pal_in6_addr))
#else
#define         VRRP_ADVT_SIZE_V4       VRRP_ADVT_SIZE
#endif

/* IPV6 advertisment header length
   size of vrrp_advt_info - size of ipv4 address - authentication info */
#ifdef HAVE_IPV6
#define         VRRP_ADVT_SIZE_V6       (VRRP_ADVT_SIZE - sizeof (struct pal_in4_addr) - 2*sizeof (int))
#endif /* HAVE_IPV6 */


/*  In VRRP advertizement packet                                         
    N - Count IP addrs                                                   
                                                                         
    For IPv4                                                             
                                                                         
    VRRP_ADVT_V4_FIXED_LEN   16                                          
    VRRP_ADVT_V4_VARIABLE_LEN(N)   N * 4  as IPv4 address - 32 bits      
                                                                         
    For IPv6                                                             
                                                                         
    VRRP_ADVT_V6_FIXED_LEN        8                                      
    VRRP_ADVT_V6_VARIABLE_LEN(N)  N * 16  as IPv6 address - 128 bits. */ 
                                                                         
                                                                         
/* Total length in Octets - including fixed and variable fields. */      
                                                                         
#define VRRP_ADVT_V4_TOTAL_LEN(NIP)   (6 * (sizeof (u_int8_t))   \
         + sizeof (u_int16_t) + (NIP * sizeof (struct pal_in4_addr))) \
         + (2 * sizeof (u_int32_t))                                      
                                                                         
#ifdef HAVE_IPV6                                                         
#define VRRP_ADVT_V6_TOTAL_LEN(NIP)   (6 * (sizeof (u_int8_t))   \
         + sizeof (u_int16_t) + (NIP * sizeof (struct pal_in6_addr)))    
#endif /* HAVE_IPV6 */                                                   
                                                                         
/* Checksum position in VRRP packet (in octets). */                      
#define VRRP_ADVT_CHECKSUM_POS   (6 * (sizeof (u_int8_t)))               


/* VRRP received packet statistics. */
int     vrrp_stats_checksum_err;        /* No. pkts rcvd with invalid chksum */
int     vrrp_stats_version_err;         /* No. pkts rcvd with invalid version */
int     vrrp_stats_invalid_vrid;        /* No. pkts rcvd with invalid vrid */


typedef struct vrrp_global VRRP_GLOBAL_DATA;

/* Structure to hold the VRRP Session information */
struct vrrp_session
{
  /* Pointer to VRRP global. */
  VRRP_GLOBAL_DATA *vrrp;

  /* VrId. */
  int vrid;

  /* Current State. */
  vrrp_state_t state;

  /* Address family */
  u_int8_t  af_type;

  /* Virtual IP address. */
  struct pal_in4_addr vip_v4;

#ifdef HAVE_IPV6
  /* Virtual IPv6 address. */
  struct pal_in6_addr vip_v6;
#endif /*HAVE_IPV6 */

  /* IP owner? */
  bool_t owner;

  /* Interface. */
  u_int32_t ifindex;
  struct interface *ifp;

  /* Priority. */
  int prio;
  int conf_prio;

  /* Advertisement interval. */
  int adv_int;

  /* Preempt mode. */
  bool_t preempt;

  /* Virtual Mac. */
  PAL_MAC_ADDR vmac;

  /* Countdown timer. */
  int timer;

  /* Flags. */
  int vip_status;
  bool_t shutdown_flag;

  /* Skew time. */
  int skew_time;

  /* Master down interval. */
  int master_down_int;

  /* Number of IP addresses. */
  int num_ip_addrs;

#ifdef HAVE_VRRP_LINK_ADDR
  /* VRRP Link address. */
  bool_t link_addr_deleted;
#endif /* HAVE_VRRP_LINK_ADDR */

  /* VRRP Circuit Failover. */
  char  *monitor_if;
  int    priority_delta;
  bool_t mc_status;
  s_int8_t mc_down_cnt;

  /* SNMP variables. */
  pal_time_t vrrp_uptime;
  int        admin_state;
  int        rowstatus;
  struct     vrrp_advt_info ai;
  int        primary_ipaddr_flag;
  int        storage_type;
  int        accept_mode;

  /* SNMP statistic information */
  int stats_become_master;              /* Times state transitioned to master */
  int stats_advt_rcvd;                  /* Advt rcvd */
  int stats_advt_int_errors;            /* Advt rcvd with wrong interval */
  int stats_ip_ttl_errors;              /* No. pkts rcvd with ttl != 255 */
  int stats_pri_zero_pkts_rcvd;         /* No. pkts rcvd with priority=0 */
  int stats_pri_zero_pkts_sent;         /* No. pkts sent with priority=0 */
  int stats_invalid_type_pkts_rcvd;     /* No. pkts rcvd with invalid type */
  int stats_addr_list_errors;           /* No. pkts rcvd with addr mismatch */
  int stats_invalid_auth_type;          /* No. pkts rcvd with unknown auth
                                           type */
  int stats_auth_type_mismatch;         /* No. pkts rcvd with auth type mismatch */
  int stats_pkt_len_errors;             /* Total num of pkts rcvd with pkt
                                           length < length of VRRP header */

  int mc_up_events;                      /* Monitored circuit up event. */
  int mc_down_events;                    /* Monitored circuit down event. */

  int new_master_reason;                /* Reason to transit to the master. */
  vrrp_init_msg_code_t init_msg_code;
};

/* Structure to hold a single VrrpAssociatedIpAddrEntry objects */
struct vrrp_asso
{
  VRRP_GLOBAL_DATA   *vrrp;
  u_int8_t            af_type;
  u_int32_t           ifindex;
  int                 vrid;
  struct pal_in4_addr ipad_v4;
#ifdef HAVE_IPV6
  struct pal_in6_addr ipad_v6;
#endif /* HAVE_IPV6 */
  int asso_rowstatus;
  int asso_storage_type;
};

typedef struct vrrp_asso VRRP_ASSO;

/*
 * -----------
 * Global Data
 * -----------
 */
#define VRRP_GET_APN_VR(id) ((struct apn_vr *)vector_slot(nzg->vr_vec, id))
#define VRRP_GET_GLOBAL(apn_vrid) ((struct nsm_master *)(VRRP_GET_APN_VR(apn_vrid))->proto)->vrrp

struct vrrp_global
{
  /* Pointer to NSM master. */
  struct nsm_master *nm;

  /*Flag For Trap Generation */
  int notificationcntl; /* 1 - Enabled, 2 - Disabled */

  /* Instance table. */
#define VRRP_SESS_TBL_INI_SZ 255
  int            sess_tbl_max;
  int            sess_tbl_cnt;
  VRRP_SESSION **sess_tbl;

#ifdef HAVE_SNMP
    /* Instance table. */
#define VRRP_ASSO_TBL_INI_SZ 255
  int            asso_tbl_max;
  int            asso_tbl_cnt;
  VRRP_ASSO    **asso_tbl;
#endif

  /* VRRP timer. */
  struct thread *t_vrrp_timer;

  /* Read thread. */
  struct thread *t_vrrp_read;
  struct thread *t_vrrp_prom;

  /* Send/Recv socket. */
  int sock;
  struct stream *ibuf;
  struct stream *i6buf;

  /* VRRP forwarding (layer 2) sockets. */
  int l2_sock;
  int sock_net;
  int sock_promisc;
  int sock_arp;
  PAL_MAC_ADDR vrrp_mac;

#ifdef HAVE_IPV6
   pal_sock_handle_t ipv6_sock;

#endif /* HAVE_IPV6 */

  int proto_err_reason;
};

/*
 * --------------------------------------------------
 * Functions internal to the nsm/vrrp subdir
 * --------------------------------------------------
 */
int vrrp_sess_tbl_walk (int apn_vrid, void *ref, vrrp_api_sess_walk_fun_t efun, int *num_sess);
int vrrp_sess_tbl_create (VRRP_GLOBAL_DATA *vrrp);
int vrrp_sess_tbl_delete (VRRP_GLOBAL_DATA *vrrp);

VRRP_SESSION *vrrp_sess_tbl_create_sess (VRRP_GLOBAL_DATA *vrrp, u_int8_t af_type, int  vrid, u_int32_t ifindex);
int vrrp_sess_tbl_delete_sess (VRRP_SESSION *sess);

VRRP_SESSION *vrrp_sess_tbl_lkup (int apn_vrid, u_int8_t af_type, int vrid, u_int32_t ifindex);
VRRP_SESSION **vrrp_sess_tbl_lkup_ref(VRRP_GLOBAL_DATA *vrrp, VRRP_SESSION *sess);
VRRP_SESSION *vrrp_sess_tbl_lkup_next (int apn_vrid, u_int8_t af_type, u_int8_t  vrid, u_int32_t ifindex);
VRRP_SESSION *vrrp_sess_tbl_add_ref (VRRP_SESSION *sess);
int vrrp_sess_tbl_check_ref (VRRP_SESSION *sess);
int vrrp_sess_tbl_del_ref (VRRP_SESSION *sess);


typedef int (* vrrp_api_asso_walk_fun_t) (VRRP_ASSO *asso, void *ref);

int vrrp_asso_tbl_create(VRRP_GLOBAL_DATA *vrrp);
int vrrp_asso_tbl_delete(VRRP_GLOBAL_DATA *vrrp);
VRRP_ASSO *vrrp_asso_tbl_lkup(int       apn_vrid,
                              u_int8_t  af_type,
                              int       vrid,
                              u_int32_t ifindex,
                              u_int8_t  *ipaddr);
VRRP_ASSO * vrrp_asso_tbl_lkup_next (int       apn_vrid,
                                     u_int8_t  af_type,
                                     u_int8_t  vrid,
                                     u_int32_t ifindex,
                                     u_int8_t *ipaddr);
int vrrp_asso_tbl_check_ref(VRRP_ASSO *asso);
VRRP_ASSO *vrrp_asso_tbl_add_ref (VRRP_ASSO *asso);
int vrrp_asso_tbl_walk(int apn_vrid,
                       void *ref,
                       vrrp_api_asso_walk_fun_t efun,
                       int *num_asso);
VRRP_ASSO *vrrp_asso_tbl_find_first(int       apn_vrid,
                                    u_int8_t  af_type,
                                    u_int8_t  vrid,
                                    u_int32_t ifindex);
VRRP_ASSO *vrrp_asso_tbl_create_asso (VRRP_GLOBAL_DATA *vrrp,
                                      u_int8_t  af_type,
                                      int       vrid,
                                      u_int32_t ifindex,
                                      u_int8_t  *ipaddr);
int vrrp_asso_tbl_del_ref(VRRP_ASSO *asso);
int vrrp_asso_tbl_delete_asso(VRRP_ASSO *asso);

/* VRRP core functions. */
int vrrp_enable_sess (VRRP_SESSION *sess);
int vrrp_shutdown_sess (VRRP_SESSION *sess);
int vrrp_packet_validate (struct vrrp_global *, struct stream *, u_int32_t,
                          struct pal_sockaddr_in4);

#ifdef HAVE_IPV6
int vrrp_handle_advert (VRRP_SESSION *sess, struct vrrp_advt_info *advt,
                        struct pal_sockaddr_in4 *from4,
                        struct pal_sockaddr_in6 *from6);
#else
int vrrp_handle_advert (VRRP_SESSION *sess, struct vrrp_advt_info *advt,
                        struct pal_sockaddr_in4 *from4,
                        struct pal_sockaddr_in4 *from6);
#endif /*HAVE_IPV6 */

void vrrp_timer (struct vrrp_global *);
u_int16_t vrrp_compute_cksum (u_int16_t *, int, u_int16_t);

/* VRRP circuit failover. */
u_int8_t vrrp_adjust_priority (VRRP_SESSION *sess);

/* Sys module */
int vrrp_sys_init (struct vrrp_global *);
int vrrp_sys_timer_up (struct thread *);
int vrrp_sys_tx_msg ();
void vrrp_sys_rx_msg ();

/* IP module */
int vrrp_ip_init (struct vrrp_global *);
int vrrp_ip_recv_advert (struct thread *);
int vrrp_multicast_join (int sock, struct interface *ifp);
int vrrp_multicast_leave (int, struct interface *);

int vrrp_ipv4_multicast_join (int sock, struct pal_in4_addr group, struct pal_in4_addr ifa, u_int32_t ifindex);
int vrrp_ipv4_multicast_leave (int sock, struct pal_in4_addr group, struct pal_in4_addr ifa, u_int32_t ifindex);

#ifdef HAVE_IPV6
int vrrp_ipv6_init (struct vrrp_global *);
int vrrp_ipv6_recv_advert (struct thread *);
int vrrp_ipv6_recv_advt (struct thread *);
int vrrp_ipv6_multicast_join (pal_sock_handle_t sock, struct pal_in6_addr addr,
                              struct interface *ifp);
int vrrp_ipv6_multicast_leave (pal_sock_handle_t sock, struct pal_in6_addr addr,
                               struct interface *ifp);
void vrrp_ipv6_join_solicit_node_multicast (VRRP_SESSION *sess);
int vrrp_ipv6_send_nd_nbadvt (VRRP_SESSION *sess);
int vrrp_fe_tx_nd_advt (struct stream *, struct interface *);
#endif /* HAVE_IPV6 */

int vrrp_ip_start_master(VRRP_SESSION *sess);

int vrrp_ip_send_advert (VRRP_SESSION *sess, u_int8_t prio);
int vrrp_ip_send_arp (VRRP_SESSION *sess);

/* FE (layer 2) module */
int vrrp_fe_init (struct vrrp_global *);
int vrrp_fe_tx_grat_arp (struct stream *, struct interface *);
#ifdef HAVE_CUSTOM1
int vrrp_fe_tx_advert (struct stream *, struct interface *);
#endif /* HAVE_CUSTOM1 */

int vrrp_fe_state_change (VRRP_SESSION *sess, vrrp_state_t curr_state, vrrp_state_t new_state,
                          bool_t owner);
int vrrp_fe_get_vmac_status ();
int vrrp_fe_set_vmac_status (int);

#ifdef HAVE_VRRP_LINK_ADDR
int vrrp_fe_vip_refresh (struct interface *, VRRP_SESSION *);
#endif /* HAVE_VRRP_LINK_ADDR */


/* Internal to VRRP module interface related functions.
*/
bool_t vrrp_if_can_vip_addr_be_used (VRRP_SESSION *sess);
int    vrrp_if_join_mcast_grp_first (VRRP_SESSION *sess);
void   vrrp_if_leave_mcast_grp_last (VRRP_SESSION *sess);
bool_t vrrp_if_monitored_circuit_state(VRRP_SESSION *sess);

                                                                                
int vrrp_vipaddr_vector_get_ipv4 (struct vrrp_advt_info *, struct stream *);    
int vrrp_vipaddr_vector_put_ipv4 (vector, struct stream *);                     
void vrrp_vipaddr_vector_free_ipv4 (vector);                                    
                                                                                
#ifdef HAVE_IPV6                                                                
int vrrp_vipaddr_vector_get_ipv6 (struct vrrp_advt_info *, struct stream *);    
int vrrp_vipaddr_vector_put_ipv6 (vector, struct stream *);                     
void vrrp_vipaddr_vector_free_ipv6 (vector);                                    
#endif /* HAVE_IPV6 */                                                          

void vrrp_cli_init (struct lib_globals *);

#endif /* _PACOS_VRRP_H */
