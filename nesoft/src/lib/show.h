/* Copyright (C) 2003  All Rights Reserved. */

#ifndef _PACOS_SHOW_H
#define _PACOS_SHOW_H

#include "cli.h"

/* This is a buffer between producer and consumer.  Producer store
   information into the output buffer.  The buffer is built by page.
   Each page can have SHOW_PAGE_SIZE information.  */

#define SHOW_PAGE_SIZE                          4096
#define SHOW_LINE_SIZE                          256

/* Page for output information.  */
struct show_page
{
  /* Simple single linked list.  */
  struct show_page *next;

  /* Data.  Last 1 character is for '\0' termination when it is
     there.  */
  char buf[SHOW_PAGE_SIZE];

  /* First position of the data.  */
  int sp;

  /* Last position of the data.  */
  int cp;
};

/* This is a connection manager for "show" command.  */
struct show
{
  /* Read thread for commands.  */
  struct thread *t_read;

  /* Write thread for output.  */
  struct thread *t_write;

  /* First page.  */
  struct show_page *first_page;

  /* Last page.  */
  struct show_page *last_page;

  /* Storage.  */
  struct show_server *server;

  /* Socket for the connection.  */
  pal_sock_handle_t sock;

  /* Input line buffer.  */
  char buf[SHOW_LINE_SIZE];

  /* Struct CLI. */
  struct cli cli;
};

/* Storage for unused page.  */
struct show_server
{
  /* Globals pointer for thread.  */
  struct lib_globals *zg;

  /* Accept sock.  */
  pal_sock_handle_t sock;

  /* Threads for read, write and accept.  */
  struct thread *t_accept;

  /* Unused page handling.  */
  struct show_page *page_unuse;
  u_int32_t page_unuse_count;
  u_int32_t page_unuse_max;
#define SHOW_PAGE_UNUSE_MAX                     100

  /* CLI tree.  */
  struct cli_tree *ctree;

  /* Protocol running configuration function. */
  s_int32_t (*show_func) (struct cli *);

  /* Host information. */
  struct host *host;
};

#ifdef HAVE_TCP_MESSAGE
/* Each protocol module's show port. */
#define SHOW_PORT_BASE            4000
#define SHOW_PORT_BASE_N(n)       (SHOW_PORT_BASE + (n))
#define IMI_SHOW_PORT             SHOW_PORT_BASE_N(1)
#define NSM_SHOW_PORT             SHOW_PORT_BASE_N(2)
#define BGP_SHOW_PORT             SHOW_PORT_BASE_N(3)
#define OSPF_SHOW_PORT            SHOW_PORT_BASE_N(4)
#define OSPF6_SHOW_PORT           SHOW_PORT_BASE_N(5)
#define RIP_SHOW_PORT             SHOW_PORT_BASE_N(6)
#define RIPNG_SHOW_PORT           SHOW_PORT_BASE_N(7)
#define ISIS_SHOW_PORT            SHOW_PORT_BASE_N(8)
#define LDP_SHOW_PORT             SHOW_PORT_BASE_N(9)
#define RSVP_SHOW_PORT            SHOW_PORT_BASE_N(10)
#define PIMDM_SHOW_PORT           SHOW_PORT_BASE_N(11)
#define PIMSM_SHOW_PORT           SHOW_PORT_BASE_N(12)
#define DVMRP_SHOW_PORT           SHOW_PORT_BASE_N(13)
#define PIMPKTGEN_SHOW_PORT       SHOW_PORT_BASE_N(14)
#define STP_SHOW_PORT             SHOW_PORT_BASE_N(15)
#define RSTP_SHOW_PORT            SHOW_PORT_BASE_N(16)
#define MSTP_SHOW_PORT            SHOW_PORT_BASE_N(17)
#define AUTH_SHOW_PORT            SHOW_PORT_BASE_N(18)
#define LACP_SHOW_PORT            SHOW_PORT_BASE_N(19)
#define PIMSM6_SHOW_PORT          SHOW_PORT_BASE_N(20)
#define RMON_SHOW_PORT            SHOW_PORT_BASE_N(21)
#define ONM_SHOW_PORT             SHOW_PORT_BASE_N(22)
#define BFD_SHOW_PORT             SHOW_PORT_BASE_N(23)
#define VLOGD_SHOW_PORT           SHOW_PORT_BASE_N(24)
#define ELMI_SHOW_PORT            SHOW_PORT_BASE_N(25)
#define LMP_SHOW_PORT             SHOW_PORT_BASE_N(26)

#define SHOW_PORT_GET(P)                                                      \
    ((P) == APN_PROTO_NSM       ? NSM_SHOW_PORT :                             \
     (P) == APN_PROTO_RIP       ? RIP_SHOW_PORT :                             \
     (P) == APN_PROTO_RIPNG     ? RIPNG_SHOW_PORT :                           \
     (P) == APN_PROTO_OSPF      ? OSPF_SHOW_PORT :                            \
     (P) == APN_PROTO_OSPF6     ? OSPF6_SHOW_PORT :                           \
     (P) == APN_PROTO_ISIS      ? ISIS_SHOW_PORT :                            \
     (P) == APN_PROTO_BGP       ? BGP_SHOW_PORT :                             \
     (P) == APN_PROTO_LDP       ? LDP_SHOW_PORT :                             \
     (P) == APN_PROTO_RSVP      ? RSVP_SHOW_PORT :                            \
     (P) == APN_PROTO_PIMDM     ? PIMDM_SHOW_PORT :                           \
     (P) == APN_PROTO_PIMSM     ? PIMSM_SHOW_PORT :                           \
     (P) == APN_PROTO_PIMSM6    ? PIMSM6_SHOW_PORT :                          \
     (P) == APN_PROTO_DVMRP     ? DVMRP_SHOW_PORT :                           \
     (P) == APN_PROTO_MSTP      ? MSTP_SHOW_PORT :                            \
     (P) == APN_PROTO_RSTP      ? RSTP_SHOW_PORT :                            \
     (P) == APN_PROTO_STP       ? STP_SHOW_PORT :                             \
     (P) == APN_PROTO_8021X     ? AUTH_SHOW_PORT :                            \
     (P) == APN_PROTO_ONM       ? ONM_SHOW_PORT :                             \
     (P) == APN_PROTO_LACP      ? LACP_SHOW_PORT :                            \
     (P) == APN_PROTO_PIMPKTGEN ? PIMPKTGEN_SHOW_PORT :                       \
     (P) == APN_PROTO_IMI       ? IMI_SHOW_PORT :                             \
     (P) == APN_PROTO_RMON      ? RMON_SHOW_PORT :                            \
     (P) == APN_PROTO_OAM       ? BFD_SHOW_PORT :                             \
     (P) == APN_PROTO_ELMI      ? ELMI_SHOW_PORT :                            \
     (P) == APN_PROTO_VLOG      ? VLOGD_SHOW_PORT :                           \
     (P) == APN_PROTO_LMP       ? LMP_SHOW_PORT :                             \
     (P) == APN_PROTO_UNSPEC    ? -1 : -1)

#else /* HAVE_TCP_MESSAGE */
/* Each protocol module's "show" path.  */
#ifdef HAVE_SPLAT
#define SHOW_PATH_PREFIX          "/var/opt/OPSEC/iapn"
#else /* HAVE_SPLAT */
#define SHOW_PATH_PREFIX
#endif /* HAVE_SPLAT */
#define IMI_SHOW_PATH             SHOW_PATH_PREFIX "/tmp/.imi_show"
#define NSM_SHOW_PATH             SHOW_PATH_PREFIX "/tmp/.nsm_show"
#define BGP_SHOW_PATH             SHOW_PATH_PREFIX "/tmp/.bgp_show"
#define OSPF_SHOW_PATH            SHOW_PATH_PREFIX "/tmp/.ospf_show"
#define OSPF6_SHOW_PATH           SHOW_PATH_PREFIX "/tmp/.ospf6_show"
#define RIP_SHOW_PATH             SHOW_PATH_PREFIX "/tmp/.rip_show"
#define RIPNG_SHOW_PATH           SHOW_PATH_PREFIX "/tmp/.ripng_show"
#define ISIS_SHOW_PATH            SHOW_PATH_PREFIX "/tmp/.isis_show"
#define LDP_SHOW_PATH             SHOW_PATH_PREFIX "/tmp/.ldp_show"
#define RSVP_SHOW_PATH            SHOW_PATH_PREFIX "/tmp/.rsvp_show"
#define PIMDM_SHOW_PATH           SHOW_PATH_PREFIX "/tmp/.pdm_show"
#define PIMSM_SHOW_PATH           SHOW_PATH_PREFIX "/tmp/.pim_show"
#define DVMRP_SHOW_PATH           SHOW_PATH_PREFIX "/tmp/.dvmrp_show"
#define PIMPKTGEN_SHOW_PATH       SHOW_PATH_PREFIX "/tmp/.pimpktgen_show"
#define STP_SHOW_PATH             SHOW_PATH_PREFIX "/tmp/.stp_show"
#define RSTP_SHOW_PATH            SHOW_PATH_PREFIX "/tmp/.rstp_show"
#define MSTP_SHOW_PATH            SHOW_PATH_PREFIX "/tmp/.mstp_show"
#define AUTH_SHOW_PATH            SHOW_PATH_PREFIX "/tmp/.auth_show"
#define ONM_SHOW_PATH             SHOW_PATH_PREFIX "/tmp/.onm_show"
#define PIMSM6_SHOW_PATH          SHOW_PATH_PREFIX "/tmp/.pim6_show"
#define LACP_SHOW_PATH            SHOW_PATH_PREFIX "/tmp/.lacp_show"
#define RMON_SHOW_PATH            SHOW_PATH_PREFIX "/tmp/.rmon_show"
#define BFD_SHOW_PATH             SHOW_PATH_PREFIX "/tmp/.bfd_show"
#define HSL_SHOW_PATH             SHOW_PATH_PREFIX "/tmp/.hsl_show"
#define VLOG_SHOW_PATH            SHOW_PATH_PREFIX "/tmp/.vlog_show"
#define ELMI_SHOW_PATH            SHOW_PATH_PREFIX "/tmp/.elmi_show"
#define LMP_SHOW_PATH             SHOW_PATH_PREFIX "/tmp/.lmp_show"

#define SHOW_PATH_GET(P)                                                      \
    ((P) == APN_PROTO_NSM       ? NSM_SHOW_PATH :                             \
     (P) == APN_PROTO_RIP       ? RIP_SHOW_PATH :                             \
     (P) == APN_PROTO_RIPNG     ? RIPNG_SHOW_PATH :                           \
     (P) == APN_PROTO_OSPF      ? OSPF_SHOW_PATH :                            \
     (P) == APN_PROTO_OSPF6     ? OSPF6_SHOW_PATH :                           \
     (P) == APN_PROTO_ISIS      ? ISIS_SHOW_PATH :                            \
     (P) == APN_PROTO_BGP       ? BGP_SHOW_PATH :                             \
     (P) == APN_PROTO_LDP       ? LDP_SHOW_PATH :                             \
     (P) == APN_PROTO_RSVP      ? RSVP_SHOW_PATH :                            \
     (P) == APN_PROTO_PIMDM     ? PIMDM_SHOW_PATH :                           \
     (P) == APN_PROTO_PIMSM     ? PIMSM_SHOW_PATH :                           \
     (P) == APN_PROTO_PIMSM6    ? PIMSM6_SHOW_PATH :                          \
     (P) == APN_PROTO_DVMRP     ? DVMRP_SHOW_PATH :                           \
     (P) == APN_PROTO_MSTP      ? MSTP_SHOW_PATH :                            \
     (P) == APN_PROTO_RSTP      ? RSTP_SHOW_PATH :                            \
     (P) == APN_PROTO_STP       ? STP_SHOW_PATH :                             \
     (P) == APN_PROTO_8021X     ? AUTH_SHOW_PATH :                            \
     (P) == APN_PROTO_ONM       ? ONM_SHOW_PATH :                             \
     (P) == APN_PROTO_LACP      ? LACP_SHOW_PATH :                            \
     (P) == APN_PROTO_PIMPKTGEN ? PIMPKTGEN_SHOW_PATH :                       \
     (P) == APN_PROTO_IMI       ? IMI_SHOW_PATH :                             \
     (P) == APN_PROTO_RMON      ? RMON_SHOW_PATH :                            \
     (P) == APN_PROTO_HSL       ? HSL_SHOW_PATH :                             \
     (P) == APN_PROTO_OAM       ? BFD_SHOW_PATH :                             \
     (P) == APN_PROTO_ELMI      ? ELMI_SHOW_PATH :                            \
     (P) == APN_PROTO_VLOG      ? VLOG_SHOW_PATH :                            \
     (P) == APN_PROTO_LMP       ? LMP_SHOW_PATH :                             \
     (P) == APN_PROTO_UNSPEC    ? NULL : NULL)

#endif /* HAVE_TCP_MESSAGE */

/* Init and shutdown function.  */
int show_client_socket (struct lib_globals *, module_id_t);
struct show_server *show_server_init (struct lib_globals *);
void show_server_finish (struct lib_globals *);

int show_out (struct show *, const char *, ...);
int show_line_write (struct lib_globals *, pal_sock_handle_t,
                     char *, u_int16_t, u_int32_t);
void show_server_show_func (struct show_server *show,
                            int (*func) (struct cli *));


#endif /* _PACOS_SHOW_H */
