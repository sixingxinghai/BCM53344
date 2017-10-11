/* Copyright (C) 2001-2003  All Rights Reserved. */

#ifndef _PACOS_VTY_H
#define _PACOS_VTY_H

#include <pal.h>

#include "vector.h"
#include "cli.h"

/* Forward declaration. */
struct vty;

/* VTY server structure. */
struct vty_server
{
  /* Vector of vty's */
  vector vtyvec;

  /* Server thread vector. */
  vector Vvty_serv_thread;

  /* Current working directory. */
  char *vty_cwd;

  /* Command tree. */
  struct cli_tree *ctree;

#if defined (HAVE_IMI) && ! defined (HAVE_IMISH)
  /* IMI proxies VTY commands to PMs.  */
  int (*imi_vty_command) (struct vty *, struct cli_node *, char *);

  void (*vty_serv_vty_gone_cb)(struct vty *);

#endif /* HAVE_IMI && ! HAVE_IMISH */
};

/* VTY struct. */
struct vty
{
  /* CLI parameters.  */
  struct cli cli;

  /* VTY master pointer.  */
  struct vty_server *server;

  /* file descriptor of this vty */
  int fd;

  /* lib_globals. */
  struct lib_globals *zg;

  /* output handle for this vty */
  pal_sock_handle_t sock;

  /* Is this vty connect to file or not */
  enum
    {
      VTY_TERM,
      VTY_SHELL,
      VTY_SHELL_SERV
    } type;

  /* Mode status of this vty */
  int mode;

  /* Local and remote IPv4/v6 address for the VTY connection.  */
  struct prefix *local;
  struct prefix *remote;

  /* Failure count */
  int fail;
  int login_fail;

  /* Output buffer. */
  struct buffer *obuf;

  /* Command input buffer */
  char *buf;

  /* Command cursor point */
  int cp;

  /* Command length */
  int length;

  /* Command max length. */
  int max;

  /* Histry of command */
#define VTY_MAXHIST 20
  char *hist[VTY_MAXHIST];

  /* History lookup current point */
  int hp;

  /* History insert end point */
  int hindex;

  /* For current referencing point of interface, route-map,
     access-list etc... */
  void *index;

  /* For multiple level index treatment such as key chain and key. */
  void *index_sub;

  /* For escape character. */
  char escape;

  /* IAC handling */
  char iac;

  /* IAC SB handling */
  char iac_sb_in_progress;
  struct buffer *sb_buffer;

  /* Window width/height. */
  int width;
  int height;

  /* Configure lines. */
  int lines;

  /* Current executing function pointer. */
  int (*func) (struct vty *, void *arg);

  /* Terminal monitor. */
  int monitor;
#define VTY_MONITOR_CONFIG      (1 << 0)
#define VTY_MONITOR_OUTPUT      (1 << 1)

  /* In configure mode. */
  int config;

  /* Read and write thread. */
  struct thread *t_read;
  struct thread *t_write;

  /* Timeout seconds and thread. */
  u_int32_t v_timeout;
  struct thread *t_timeout;

  /* Thread output function. */
  struct thread *t_output;

  /* Should the command be added to history. */
  char history;

  /* For vty buffer.  */
  int lp;
  int lineno;
};

/* VTY default buffer size.  */
#define VTY_BUFSIZ         8192

/* Default time out value.  */
#define VTY_TIMEOUT_DEFAULT 600

/* Directory separator. */
#ifndef DIRECTORY_SEP
#define DIRECTORY_SEP '/'
#endif /* DIRECTORY_SEP */

#ifndef IS_DIRECTORY_SEP
#define IS_DIRECTORY_SEP(c) ((c) == DIRECTORY_SEP)
#endif

/* VTY port number.  */
#define NSM_VTY_PORT                    2601
#define RIP_VTY_PORT                    2602
#define RIPNG_VTY_PORT                  2603
#define OSPF_VTY_PORT                   2604
#define BGP_VTY_PORT                    2605
#define OSPF6_VTY_PORT                  2606
#define LDP_VTY_PORT                    2607
#define RSVP_VTY_PORT                   2608
#define ISIS_VTY_PORT                   2609
#define PIM_VTY_PORT                    2610
#define DVMRP_VTY_PORT                  2611
#define AUTH_VTY_PORT                   2612
#define STP_VTY_PORT                    2613
#define PIM6_VTY_PORT                   2615
#define RSTP_VTY_PORT                   2616
#define MSTP_VTY_PORT                   2618
#define LACP_VTY_PORT                   2619
#define PIMPKTGEN_VTY_PORT              2620
#define PDM_VTY_PORT                    2621
#define RMON_VTY_PORT                   2622
#define ONM_VTY_PORT                    2623
#define BFD_VTY_PORT                    2624
#define LMP_VTY_PORT                    2625
#define IMI_VTY_PORT                    2650
#define ONMD_VTY_PORT                   2651
#define HSL_VTY_PORT                    2652
#define ELMI_VTY_PORT                   2653

#define APN_VTY_PORT(Z, P)                                                    \
    ((P) ? (P) :                                                              \
     (Z)->protocol == APN_PROTO_NSM       ? NSM_VTY_PORT :                    \
     (Z)->protocol == APN_PROTO_RIP       ? RIP_VTY_PORT :                    \
     (Z)->protocol == APN_PROTO_RIPNG     ? RIPNG_VTY_PORT :                  \
     (Z)->protocol == APN_PROTO_OSPF      ? OSPF_VTY_PORT :                   \
     (Z)->protocol == APN_PROTO_BGP       ? BGP_VTY_PORT :                    \
     (Z)->protocol == APN_PROTO_OSPF6     ? OSPF6_VTY_PORT :                  \
     (Z)->protocol == APN_PROTO_LDP       ? LDP_VTY_PORT :                    \
     (Z)->protocol == APN_PROTO_RSVP      ? RSVP_VTY_PORT :                   \
     (Z)->protocol == APN_PROTO_ISIS      ? ISIS_VTY_PORT :                   \
     (Z)->protocol == APN_PROTO_PIMSM     ? PIM_VTY_PORT :                    \
     (Z)->protocol == APN_PROTO_DVMRP     ? DVMRP_VTY_PORT :                  \
     (Z)->protocol == APN_PROTO_8021X     ? AUTH_VTY_PORT :                   \
     (Z)->protocol == APN_PROTO_STP       ? STP_VTY_PORT :                    \
     (Z)->protocol == APN_PROTO_PIMSM6    ? PIM6_VTY_PORT :                   \
     (Z)->protocol == APN_PROTO_RSTP      ? RSTP_VTY_PORT :                   \
     (Z)->protocol == APN_PROTO_MSTP      ? MSTP_VTY_PORT :                   \
     (Z)->protocol == APN_PROTO_LACP      ? LACP_VTY_PORT :                   \
     (Z)->protocol == APN_PROTO_PIMPKTGEN ? PIMPKTGEN_VTY_PORT :              \
     (Z)->protocol == APN_PROTO_IMI       ? IMI_VTY_PORT :                    \
     (Z)->protocol == APN_PROTO_PIMDM     ? PDM_VTY_PORT :                    \
     (Z)->protocol == APN_PROTO_ONM       ? ONM_VTY_PORT :                    \
     (Z)->protocol == APN_PROTO_ELMI      ? ELMI_VTY_PORT :                   \
     (Z)->protocol == APN_PROTO_LMP       ? LMP_VTY_PORT :                    \
     (Z)->protocol == APN_PROTO_OAM       ? BFD_VTY_PORT :                    \
     -1)

/* VTY shell path.  */
#define NSM_VTYSH_PATH                  "/tmp/.nsm"
#define RIP_VTYSH_PATH                  "/tmp/.ripd"
#define RIPNG_VTYSH_PATH                "/tmp/.ripngd"
#define OSPF_VTYSH_PATH                 "/tmp/.ospfd"
#define BGP_VTYSH_PATH                  "/tmp/.bgpd"
#define OSPF6_VTYSH_PATH                "/tmp/.ospf6d"
#define LDP_VTYSH_PATH                  "/tmp/.ldpd"
#define RSVP_VTYSH_PATH                 "/tmp/.rsvpd"
#define ISIS_VTYSH_PATH                 "/tmp/.isisd"
#define PDM_VTYSH_PATH                  "/tmp/.pdmd"
#define PIM_VTYSH_PATH                  "/tmp/.pimd"
#define DVMRP_VTYSH_PATH                "/tmp/.dvmrpd"
#define AUTH_VTYSH_PATH                 "/tmp/.authd"
#define STP_VTYSH_PATH                  "/tmp/.stpd"
#define PIM6_VTYSH_PATH                 "/tmp/.pim6d"
#define RSTP_VTYSH_PATH                 "/tmp/.rstpd"
#define MSTP_VTYSH_PATH                 "/tmp/.mstpd"
#define LACP_VTYSH_PATH                 "/tmp/.lacpd"
#define PIM_PKTGEN_VTYSH_PATH           "/tmp/.pimpktgen"
#define ELMI_VTYSH_PATH                 "/tmp/.elmid"
#define BFD_VTYSH_PATH                  "/tmp/.oamd"
#define LMP_VTYSH_PATH                  "/tmp/.lmpd"

/* Prototypes. */
int vty_serv_sock (struct lib_globals *, u_int16_t);
int vty_out (struct vty *, const char *, ...);
struct vty *vty_new (struct lib_globals *);
void vty_time_print (struct vty *, int);
void vty_close (struct vty *);
int vty_config_write (struct cli *);
char *vty_get_cwd (struct lib_globals *);
void vty_log (struct lib_globals *, const char *, const char *, const char *);
int vty_config_lock (struct vty *);
int vty_config_unlock (struct vty *);
int vty_shell (struct vty *);
int vty_shell_serv (struct vty *);
int vty_monitor_output (struct vty *);
void vty_prompt (struct vty*);
void vty_auth (struct vty *, char *);
int vty_execute (struct vty *);

struct vty_server *vty_server_new ();
void vty_master_free (struct vty_server **vty_master);
void vty_register_ctree (struct vty_server *, struct cli_tree *);
struct vty_server *vty_get_master (struct vty *vty);
void vty_cmd_init (struct cli_tree *ctree);
void vty_hist_add (struct vty *vty);
void vty_clear_buf (struct vty *vty);

#if defined(HAVE_IMI) && !defined(HAVE_IMISH)
void vty_serv_add_vty_gone_cb(struct lib_globals *, void (*func)(struct vty *));
#endif

#endif /* _PACOS_VTY_H */
