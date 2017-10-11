/*=============================================================================
**
** Copyright (C) 2002-2003   All Rights Reserved.
**
** lib.h -- PacOS library common definitions
*/
#ifndef _LIB_H
#define _LIB_H

/*-----------------------------------------------------------------------------
**
** Include files
*/
#include "pal.h"
#include "pal_types.h"
#include "pal_socket.h"
#include "pal_log.h"
#include "pal_memory.h"

#ifdef HAVE_HA
#include "lib_ha.h"
#endif /* HAVE_HA */

#ifdef HAVE_MCAST_IPV4
#include "mcast/mcast_bitmap.h"
#endif /* HAVE_MCAST_IPV4 */

#include "memory.h"
#ifdef MEMMGR
#include "memmgr.h"
#include "memmgr_cli.h"
#endif /* MEMMGR */
#include "fifo.h"
#include "vector.h"
#include "prefix.h"
#include "vty.h"
#include "host.h"
#include "auth_md5.h"

#ifdef HAVE_MPLS
#include "mpls.h"
#endif /* HAVE_MPLS */

#include "if.h"
#include "filter.h"
#include "if_rmap.h"
#include "plist.h"
#include "routemap.h"
#include "entity.h"
#include "distribute.h"
#include "hash.h"
#ifdef HAVE_SNMP
#include "snmp.h"
#include "snmp_misc.h"
#endif /* HAVE_SNMP */
#include "api.h"
#include "message.h"
#include "nsm_message.h"

#ifdef HAVE_HA
#include "lib_ha.h"
#endif /* HAVE_HA */

#include "commsg.h"

#include "lib_fm_api.h"
#include "mod_stop.h"
#ifdef HAVE_VLOGD
#include "vlog_client.h"
#endif

#ifdef HAVE_TE
#include "qos_common.h"
#endif /* HAVE_TE */

#ifdef HAVE_GMPLS
#include "gmpls/gmpls.h"
#endif /* HAVE_GMPLS */

#ifdef HAVE_PBR
#include "pbr_filter.h"
#endif /* HAVE_PBR */


#if defined (HAVE_BFD) || defined (HAVE_MPLS_OAM)
struct bfd_server;
struct bfd_client;
#endif /* HAVE_BFD  || HAVE_MPLS_OAM */

#define APN_MAX(a,b) ((a) > (b) ? (a) : (b))
#define APN_MIN(a,b) ((a) < (b) ? (a) : (b))

/* An 'infinite' distance to the routing protocols. */
#define DISTANCE_INFINITY       255

#define LIB_MAX_PROG_NAME       15

/* The PacOS NSM communications port. */
#define NSM_PORT                2600
#define NSM_SERV_PATH           "/tmp/.nsmserv"

#ifdef HAVE_VLOGD
/* The PacOS VLOG communications port. */
#define VLOG_PORT                2800
#define VLOG_SERV_PATH           "/tmp/.vlogserv"
typedef ZRESULT (* VLOG_SET_LOG_FILE_CB)(struct lib_globals *zg,
                                         u_int32_t vr_id, char *fname);
typedef ZRESULT (* VLOG_UNSET_LOG_FILE_CB)(struct lib_globals *zg,
                                           u_int32_t vr_id);
typedef ZRESULT (* VLOG_GET_LOG_FILE_CB)(struct lib_globals *zg,
                                         u_int32_t vr_id, char **pfname);
#endif /*HAVE_VLOGD*/

#ifdef HAVE_SMI
/* SMI client/server communication ports */
#define SMI_PORT_NSM            3600
#define SMI_SERV_NSM_PATH       "/tmp/.nsmapiserv"
#define SMI_PORT_LACP           3601
#define SMI_SERV_LACP_PATH      "/tmp/.lacpapiserv"
#define SMI_PORT_MSTP           3602
#define SMI_SERV_MSTP_PATH      "/tmp/.mstpapiserv"
#define SMI_PORT_RMON           3603
#define SMI_SERV_RMON_PATH      "/tmp/.rmonapiserv"
#define SMI_PORT_ONM            3604
#define SMI_SERV_ONM_PATH       "/tmp/.onmapiserv"
#endif /* HAVE_SMI */

/* HSL communications port. */
#define HSL_CMD_PORT             3200
#define HSL_CMD_PATH             "/tmp/.hsl_cmd"
#define HSL_ASYNC_PORT           3201
#define HSL_ASYNC_PATH           "/tmp/.hsl_async"


#ifdef HAVE_USER_HSL
/* Recv Buffer Max size from HSL */
#define RCV_BUFSIZ               1500
#endif /* HAVE_USER_HSL */

#if defined (HAVE_BFD) || defined (HAVE_MPLS_OAM)
/* The PacOS BFD communications port. */
#define BFD_PORT                4600
#define BFD_SERV_PATH           "/tmp/.bfdserv"
#endif /* HAVE_BFD || HAVE_MPLS_OAM */

#ifdef HAVE_ONMD
#ifdef HAVE_ELMID
/* The PacOS ELMI communications port. */
#define ONM_PORT                4800
#define ONM_SERVER_PATH         "/tmp/.onmserv"
#endif /* HAVE_ELMID*/
#endif /* HAVE_ONMD*/

/* Maximum VR name length
 * Required for IPNET to limit the per-VR loopback
 * interface name to the IP_IFNAMSIZ (16)
 * Max len = 16 - 1 ('\0') - 2 ("lo") - 1 (Seperator ".") = 12
 */
#define LIB_VR_MAX_NAMELEN                         12

struct apn_vr
{
  /* Pointer to globals. */
  struct lib_globals *zg;

  /* VR name. */
  char *name;

  /* VR ID. */
  u_int32_t id;

  /* Router ID. */
  struct pal_in4_addr router_id;

  u_int8_t flags;
#define LIB_FLAG_DELETE_VR_CONFIG_FILE    (1 << 0)

  /* Interface Master. */
  struct if_vr_master ifm;

  /* VRFs. */
  vector vrf_vec;

  /* VRFs. */
  struct apn_vrf *vrf_list;

  /* Protocol bindings. */
  u_int32_t protos;

  /* Host. */
  struct host *host;

  /* Access List. */
  struct access_master access_master_ipv4;
#ifdef HAVE_IPV6
  struct access_master access_master_ipv6;
#endif /* def HAVE_IPV6 */

  /* Prefix List. */
  struct prefix_master prefix_master_ipv4;
#ifdef HAVE_IPV6
  struct prefix_master prefix_master_ipv6;
#endif /* HAVE_IPV6 */
  struct prefix_master prefix_master_orf;

  /* Route Map. */
  vector route_match_vec;
  vector route_set_vec;
  struct route_map_list route_map_master;

  /* Key Chain. */
  struct list *keychain_list;

  /* Protocol Master. */
  void *proto;

  /* Config read event. */
  struct thread *t_config;

  /* VRF currently in context */
  struct apn_vrf *vrf_in_cxt;

  /* If stats update threshold timer */
  struct thread *t_if_stat_threshold;

  struct entLogicalEntry *entLogical;
  struct list *mappedPhyEntList;

  /* Community string to identify current VR */
  struct snmpCommunity snmp_community;

#ifdef HAVE_HA
  HA_CDR_REF lib_vr_cdr_ref;
#endif /* HAVE_HA */

#ifdef HAVE_PBR
  struct pbr_rmap_event pbr_event;
#endif /* HAVE_PBR */
};

/* Logical entity structure */
struct entLogicalEntry
{
  u_int32_t entLogicalIndex;
  char *entLogicalDescr;
  char *entLogicalType;
  char *entLogicalCommunity;
  char *entLogicalTAddress;
  char *entLogicalTDomain;
  char *entLogicalContextEngineId;
  char *entLogicalContextName;
};

struct apn_vrf
{
  /* VRF Linked List Pointers, List indexed by 'name' */
  struct apn_vrf *prev;
  struct apn_vrf *next;

  /* Pointer to VR. */
  struct apn_vr *vr;

  /* VRF ID. */
  vrf_id_t id;

  /* Table ID. */
  fib_id_t fib_id;

  /* VRF name. */
  char *name;

  /* Router ID. */
  struct pal_in4_addr router_id;

  /* Interface Master. */
  struct if_vrf_master ifv;

  /* Protocol data. */
  void *proto;

#if defined HAVE_MCAST_IPV4 || defined HAVE_IGMP_SNOOP
  /* Function to obtain IGMP instance */
  void * (*apn_vrf_get_igmp_instance) (struct apn_vrf *);
#endif /* HAVE_MCAST_IPV4 || HAVE_IGMP_SNOOP */
#if defined HAVE_MCAST_IPV6 || defined HAVE_MLD_SNOOP
  /* Function to obtain MLD instance */
  void * (*apn_vrf_get_mld_instance) (struct apn_vrf *);
#endif /* HAVE_MCAST_IPV6 || HAVE_MLD_SNOOP */

#ifdef HAVE_HA
  HA_CDR_REF lib_vrf_cdr_ref;
#endif /* HAVE_HA */
};

enum vr_callback_type
{
  VR_CALLBACK_ADD,
  VR_CALLBACK_DELETE,
  VR_CALLBACK_CLOSE,
  VR_CALLBACK_UNBIND,
  VR_CALLBACK_CONFIG_READ,
  VR_CALLBACK_ADD_UNCHG,
  VR_CALLBACK_MAX
};

enum vrf_callback_type
{
  VRF_CALLBACK_ADD,
  VRF_CALLBACK_DELETE,
  VRF_CALLBACK_UPDATE,
  VRF_CALLBACK_ROUTER_ID,
  VRF_CALLBACK_MAX
};

/* Library Globals
   One of these is kept per daemon, or instance of daemon, depending upon
   implementation and support for such features as virtual routing. */
struct lib_globals
{
  char progname[LIB_MAX_PROG_NAME+1];

  /* Module ID defined in pal/api/pal_modules.def.  */
  module_id_t protocol;

  /* Flags */
  u_int8_t flags;
#define LIB_FLAG_SHUTDOWN_IN_PROGRESS           (1 << 0)
#ifdef HAVE_RESTART
#define LIB_FLAG_GRACEFUL_SHUTDOWN_IN_PROGRESS  (1 << 1)
#endif /* HAVE_RESTART */
  mod_stop_cause_t stop_cause;

  /* Banner configuration.  */
  char *motd;

  /* Current working directory.  */
  char *cwd;

  /* Thread master. */
  struct thread_master *master;
  struct thread *pend_read_thread;

  /* Interface Master. */
  struct if_master ifg;
 // struct pw_master pwg;//add by fxg
 // struct si_master sig;//add by fxg
  struct meg_master megg;// czh
 // struct ac_master acg;
 // struct tun_master tung;

  /* VR vector. */
  vector vr_vec;

  /* VRF vector for Kernel Table ID mapping. */
  vector fib2vrf;

  /* Log. */
  struct zlog *log;
  struct zlog *log_default;

#ifdef HAVE_SNMP
  /* snmp : agentx or smux */
  struct snmp_master snmp;
#endif /* HAVE_SNMP */

#ifdef HAVE_GMPLS
  s_int32_t gifindex;
#endif /* HAVE_GMPLS */

  /* Callback functions. */
  int (*vr_callback[VR_CALLBACK_MAX]) (struct apn_vr *);
  int (*vrf_callback[VRF_CALLBACK_MAX]) (struct apn_vrf *);
  int (*user_callback[USER_CALLBACK_MAX]) (struct apn_vr *,
                                           struct host_user *);

  /* NSM client. */
  struct nsm_client *nc;

#if defined (HAVE_BFD) || defined (HAVE_MPLS_OAM)
  /* BFD server.  This will be moved to bfd_globals later.  */
  struct bfd_server *bs;

  /* BFD client.  */
  struct bfd_client *bc;
#endif /* defined (HAVE_BFD) || defined (HAVE_MPLS_OAM) */

  #ifdef HAVE_VLOGD

  /* VLOG client. */
  struct vlog_client *vlog_clt;

  /* Callbacks to be installed by IMI and VLOGD. */
  VLOG_SET_LOG_FILE_CB   vlog_file_set_cb;
  VLOG_UNSET_LOG_FILE_CB vlog_file_unset_cb;
  VLOG_GET_LOG_FILE_CB   vlog_file_get_cb;

#endif /* HAVE_VLOGD */

  struct onm_server *os;
  /* ONM client. */
  struct onm_client *oc;

  /* VR. */
  u_int32_t vr_instance;

  /* PAL. */
  pal_handle_t pal_debug;
  pal_handle_t pal_kernel;
  pal_handle_t pal_log;
  pal_handle_t pal_np;
  pal_handle_t pal_socket;
  pal_handle_t pal_stdlib;
  pal_handle_t pal_string;
  pal_handle_t pal_time;

  /* Vty master structure. */
  struct vty_server *vty_master;

  /* CLI tree. */
  struct cli_tree *ctree;

  /* IMI message handler.  */
  struct message_handler *imh;

  /* Show server.  */
  struct show_server *ss;

#ifdef HAVE_VRRP
  /* VRRPv2 PAL data structure pointer. */
  pal_handle_t pal_vrrp;
#endif /* HAVE_VRRP */

#ifdef HAVE_LICENSE_MGR
  lic_mgr_handle_t handle;
  struct thread *t_check_expiration;
#endif /* HAVE_LICENSE_MGR */

#ifdef HAVE_HA
  LIB_HA  lib_ha;
#endif /* HAVE_HA */

  /* Fault Management - Fault Recording. */
  void *lib_fm;

  /* Protocol Globals. */
  void *proto;

  /* VR currently in context */
  struct apn_vr *vr_in_cxt;

  /* Stream Socket-CB Zombies List */
  struct list *ssock_cb_zombie_list;

  /* Circular Queue Buffers Free List */
  struct cqueue_buf_list *cqueue_buf_free_list;

  /* Instance of COMMSG transport for this daemon. */
  COMMSG *commsg;

  /* Access-list add/delete notification callback. */
  filter_ntf_cb_t lib_acl_ntf_cb;

#ifdef HAVE_GMPLS
  /* gmpls_if should be allocated when needed */
  struct gmpls_if *gmif;
#endif /* HAVE_GMPLS */
};

#define IS_LIB_IN_SHUTDOWN(LIB_GLOB)                            \
  (CHECK_FLAG ((LIB_GLOB)->flags, LIB_FLAG_SHUTDOWN_IN_PROGRESS))
#define SET_LIB_IN_SHUTDOWN(LIB_GLOB)                           \
  SET_FLAG ((LIB_GLOB)->flags, LIB_FLAG_SHUTDOWN_IN_PROGRESS)
#define UNSET_LIB_IN_SHUTDOWN(LIB_GLOB)                         \
  UNSET_FLAG ((LIB_GLOB)->flags, LIB_FLAG_SHUTDOWN_IN_PROGRESS)

#define SET_LIB_STOP_CAUSE(LIB_GLOB, cause) \
  (LIB_GLOB)->stop_cause = (cause)
#define GET_LIB_STOP_CAUSE(LIB_GLOB) (LIB_GLOB)->stop_cause

#define LIB_CALLBACK_VERIFY(LIB_GLOB, GLOB, FUNC_PTR_ARR, CALLBACK_ID)        \
  ((! IS_LIB_IN_SHUTDOWN ((LIB_GLOB))) && ((GLOB)->FUNC_PTR_ARR[(CALLBACK_ID)]))

/*
 * Utility Macros for manipulating Library Structures
 */
#define APN_PVR_ID              0
#define IS_APN_VR_PRIVILEGED(V) ((V) != NULL && (V)->id == APN_PVR_ID)
#define IS_APN_VRF_DEFAULT(V)   ((V) == NULL || (V)->name == NULL)
#define IS_APN_VRF_PRIV_DEFAULT(V)   (IS_APN_VRF_DEFAULT (V)     \
                                      && IS_APN_VR_PRIVILEGED ((V)->vr))
#define IS_APN_VRF_UP(V)        ((V) != NULL && (V)->id != VRF_ID_DISABLE)

#define LIB_GLOB_SET_PROTO_GLOB(LIB_GLOB, PROTO_GLOB)                 \
    (((LIB_GLOB)->proto) = (PROTO_GLOB))
#define LIB_GLOB_GET_PROTO_GLOB(LIB_GLOB)  ((LIB_GLOB)->proto)

#define LIB_VR_SET_PROTO_VR(LIB_VR, PROTO_VR)                         \
    (((LIB_VR)->proto) = (PROTO_VR))
#define LIB_VR_GET_PROTO_VR(LIB_VR)        ((LIB_VR)->proto)
#define LIB_VR_GET_VR_ID(LIB_VR)           ((LIB_VR)->id)
#define LIB_VR_GET_VR_NAME(LIB_VR)         ((LIB_VR)->name)
#define LIB_VR_GET_VR_NAME_STR(LIB_VR)                                \
    ((LIB_VR)->name ? (LIB_VR)->name : "Default")
#define LIB_VR_GET_IF_MASTER(LIB_VR)       (&(LIB_VR)->ifm)
#define LIB_VR_GET_IF_TABLE(LIB_VR)        ((LIB_VR)->ifm.if_table)

#define LIB_VRF_SET_PROTO_VRF(LIB_VRF, PROTO_VRF)                     \
    (((LIB_VRF)->proto) = (PROTO_VRF))
#define LIB_VRF_GET_PROTO_VRF(LIB_VRF)     ((LIB_VRF)->proto)
#define LIB_VRF_GET_VRF_ID(LIB_VRF)        ((LIB_VRF)->id)
#define LIB_VRF_GET_VRF_NAME(LIB_VRF)      ((LIB_VRF)->name)
#define LIB_VRF_GET_VR(LIB_VRF)            ((LIB_VRF)->vr)
#define LIB_VRF_GET_VRF_NAME_STR(LIB_VRF)                             \
    ((LIB_VRF)->name ? (LIB_VRF)->name : "Default")
#define LIB_VRF_GET_FIB_ID(LIB_VRF)        ((LIB_VRF)->fib_id)
#define LIB_VRF_GET_IF_MASTER(LIB_VRF)     (&(LIB_VRF)->ifv)
#define LIB_VRF_GET_IF_TABLE(LIB_VRF)      ((LIB_VRF)->ifv.if_table)
#if defined (HAVE_MCAST_IPV4) || defined (HAVE_IGMP_SNOOP)
#define LIB_VRF_SET_IGMP_INSTANCE_GET_FUNC(LIB_VRF, FUNC)             \
  do {                                                                \
    (LIB_VRF)->apn_vrf_get_igmp_instance = FUNC;                      \
  } while (0)
#define LIB_VRF_GET_IGMP_INSTANCE(LIB_VRF)                            \
  ((LIB_VRF)->apn_vrf_get_igmp_instance ?                             \
   (LIB_VRF)->apn_vrf_get_igmp_instance (LIB_VRF) : NULL)
#endif /* (HAVE_MCAST_IPV4) || defined (HAVE_IGMP_SNOOP) */
#if defined (HAVE_MCAST_IPV6) || defined (HAVE_MLD_SNOOP)
#define LIB_VRF_SET_MLD_INSTANCE_GET_FUNC(LIB_VRF, FUNC)              \
  do {                                                                \
    (LIB_VRF)->apn_vrf_get_mld_instance = FUNC;                       \
  } while (0)
#define LIB_VRF_GET_MLD_INSTANCE(LIB_VRF)                             \
  ((LIB_VRF)->apn_vrf_get_mld_instance ?                              \
   (LIB_VRF)->apn_vrf_get_mld_instance (LIB_VRF) : NULL)
#endif /* (HAVE_MCAST_IPV6) || defined (HAVE_MLD_SNOOP) */
/*
 * Macros for Context-based execution
 */
#define LIB_GLOB_SET_VR_CONTEXT(LIB_GLOB, VR_CXT)                     \
  do {                                                                \
    ((LIB_GLOB)->vr_in_cxt) = (VR_CXT);                               \
  } while (0)
#define LIB_GLOB_GET_VR_CONTEXT(LIB_GLOB)  ((LIB_GLOB)->vr_in_cxt)
#define LIB_VR_SET_VRF_CONTEXT(LIB_VR, VRF_CXT)                       \
  do {                                                                \
    ((LIB_VR)->vrf_in_cxt) = (VRF_CXT);                               \
  } while (0)
#define LIB_VR_GET_VRF_CONTEXT(LIB_VR)     ((LIB_VR)->vrf_in_cxt)

#ifdef HAVE_SNMP
#define SNMP_MASTER(ZG)                              (&(ZG)->snmp)
#endif /* HAVE_SNMP. */

#define apn_offsetof(type, mem) (pal_size_t)(&(((type *)0)->mem))

#define ARR_ELEM_TO_INDEX(TYPE,ARR,ELEM)  ((TYPE*)&(ELEM) - (TYPE*)&((ARR)[0]))

/* Macro to Stringize a variable */
#define VAR_STRINGIZE(VAR)             # VAR
#define VAR_SUPERSTR(VAR)              VAR_STRINGIZE (VAR)

/* Macro for Catenatiion a variables */
#define VAR_CATENIZE(VAR1, VAR2)       VAR1 ## VAR2
#define VAR_SUPERCAT(VAR1, VAR2)       VAR_CATENIZE (VAR1, VAR2)


/* Extern definition. */
extern char *loglevel[9];
extern char *modnames[13];
extern char *modnamel[13];

extern struct lib_globals *lib_create(char *progname);
extern result_t lib_stop (struct lib_globals *);
extern result_t lib_start (struct lib_globals *);

/* Prototypes. */
void apn_vr_delete (struct apn_vr *);
struct apn_vr *apn_vr_get (struct lib_globals *);
struct apn_vr *apn_vr_get_by_name (struct lib_globals *, char *);
struct apn_vr *apn_vr_get_by_id (struct lib_globals *, u_int32_t);
struct apn_vr *apn_vr_get_privileged (struct lib_globals *);
struct apn_vr *apn_vr_lookup_by_id (struct lib_globals *, u_int32_t);
struct apn_vr *apn_vr_lookup_by_name (struct lib_globals *, char *);
struct apn_vr *apn_vr_update_by_name (struct lib_globals *, char *, u_int32_t);

typedef ZRESULT (* APN_VRS_WALK_CB)(struct apn_vr *, intptr_t);

ZRESULT apn_vrs_walk_and_exec(struct lib_globals *zg,
                              APN_VRS_WALK_CB func,
                              intptr_t user_ref);
void apn_vrf_delete (struct apn_vrf *);
struct apn_vrf *apn_vrf_get_by_name (struct apn_vr *, char *);
struct apn_vrf *apn_vrf_lookup_by_name (struct apn_vr *, char *);
struct apn_vrf *apn_vrf_lookup_by_id (struct apn_vr *, u_int32_t);
struct apn_vrf *apn_vrf_lookup_default (struct apn_vr *);

void apn_vr_add_callback (struct lib_globals *, enum vr_callback_type,
                          int (*func) (struct apn_vr *));
void apn_vrf_add_callback (struct lib_globals *, enum vrf_callback_type,
                           int (*func) (struct apn_vrf *));

int protoid2routetype (int);

char *modname_strl (int);
char *modname_strs (int);
char *loglevel_str (int);

#ifdef HAVE_VLOGD
ZRESULT lib_reg_vlog_cbs(struct lib_globals *zg,
                         VLOG_SET_LOG_FILE_CB   vlog_file_set_cb,
                         VLOG_UNSET_LOG_FILE_CB vlog_file_unset_cb,
                         VLOG_GET_LOG_FILE_CB   vlog_file_get_cb);
#endif /* HAVE_VLOGD */

#endif /* _LIB_H */
