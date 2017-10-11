/* Copyright (C) 2001-2003  All Rights Reserved.  */

#include "pal.h"
#include "pal_mpls_client.h"

#include "lib.h"
#include "thread.h"
#if (!defined (HAVE_GMPLS) || defined (HAVE_PACKET))
#ifdef HAVE_DIFFSERV
#include "diffserv.h"
#endif /* HAVE_DIFFSERV */
#endif /* !HAVE_GMPLS || HAVE_PACKET */
#include "nsmd.h"
#include "nsm_mpls.h"
#ifdef HAVE_MPLS_VC
#include "nsm_mpls_vc.h"
#endif /* HAVE_MPLS_VC */
#include "nsm_mpls_rib.h"
#ifdef HAVE_DSTE
#include "dste.h"
#include "nsm_dste.h"
#endif /* HAVE_DSTE */
#include "mpls_common.h"

#include "pal_mpls_client.h"

/*---------------------------------------------------------------------------
 * open a netlink socket, connect it to the kernel rtnetlink module
 * and return its fd.
 *---------------------------------------------------------------------------*/
int
netlink_open()
{
  struct sockaddr_nl nl_addr;   /* our local netlink address */
  int                nl_addr_len;
  
  int                fd = socket (AF_NETLINK, SOCK_RAW, NETLINK_USERSOCK);
  
  if (fd < 0)
    {
      perror("netlink_open");
      return -1;
    }
  nl_addr.nl_family = AF_NETLINK;
  nl_addr.nl_pid    = 0;                /* let kernel fill the pid */
  nl_addr.nl_groups = 0;                /* not interested in multicasts */
  
  if (bind(fd, (struct sockaddr *)&nl_addr, sizeof(nl_addr)) < 0)
    {
      perror("netlink_open : bind");
      return -1;
    }
  
  nl_addr_len = sizeof(nl_addr);
  
  if (getsockname(fd, (struct sockaddr *)&nl_addr, &nl_addr_len) < 0)
    {
      perror("netlink_open : getsockname");
      return -1;
    }
  
  if (nl_addr_len != sizeof(nl_addr))
    {
      fprintf(stderr, "Wrong address length %d\nlmh", nl_addr_len);
      return -1;
    }
  if (nl_addr.nl_family != AF_NETLINK)
    {
      fprintf(stderr, "Wrong address family %d\nlmh", nl_addr.nl_family);
      return -1;
    }
  /*
   * now connect this socket to the kernel rtnetlink module. MPLS fib 
   * add/delete functions are a part of that.
   */
  
  if (connect(fd, (struct sockaddr *)&nl_addr, sizeof(nl_addr)) < 0)
    {
      perror("Cannot connect to kernel route netlink socket");
      return -1;
    }
  
  return fd;
}

/*---------------------------------------------------------------------------
 * user->kernel interaction.
 *--------------------------------------------------------------------------*/
int
netlink_send(int fd, u_char *buf, int len)
{
  struct sockaddr_nl nladdr;

  /* If socket is invalid, return. */
  if (fd <= 0)
    return fd;
  
  pal_mem_set(&nladdr, 0, sizeof(nladdr));
  nladdr.nl_family = AF_NETLINK;
  
  return sendto(fd, buf, len, 0, (struct sockaddr *)&nladdr,
                sizeof(nladdr));
}

/*
 * Send init/close message.
 */
int
send_mpls_control (int netlink, struct mpls_control *m_ctrl, int init)
{
  struct
  {
    struct nlmsghdr nlmh;
    struct mpls_control mc;
  }
  req_pack;
  void             *data = NLMSG_DATA(&req_pack);

  pal_mem_set(&req_pack, 0, sizeof(req_pack));
  req_pack.nlmh.nlmsg_len = NLMSG_LENGTH(sizeof(struct mpls_control));
  
  req_pack.nlmh.nlmsg_flags = NLM_F_REQUEST | NLM_F_CREATE;
  req_pack.nlmh.nlmsg_seq = MPLS_SEQ_NUM;
  
  /* Check whether this is an init msg or an end msg */
  if (init == MPLS_INIT)
    req_pack.nlmh.nlmsg_type = USERSOCK_MPLS_INIT;
  else if (init == MPLS_END)
    req_pack.nlmh.nlmsg_type = USERSOCK_MPLS_END;

  /* Copy over the ctrl data to the nl msg */
  pal_mem_cpy(data, m_ctrl, sizeof(struct mpls_control));

  return netlink_send(netlink, (u_char *)&req_pack, sizeof(req_pack));
}

/*
 * Send init/close message for interface.
 */
int
send_mpls_vrf_ctrl (int netlink, struct mpls_if *m_if, int init)
{
  struct
  {
    struct nlmsghdr nlmh;
    struct mpls_if mif;
  }
  req_pack;
  void             *data = NLMSG_DATA(&req_pack);

  pal_mem_set(&req_pack, 0, sizeof(req_pack));
  req_pack.nlmh.nlmsg_len = NLMSG_LENGTH(sizeof(struct mpls_if));
  
  req_pack.nlmh.nlmsg_flags = NLM_F_REQUEST | NLM_F_CREATE;
  req_pack.nlmh.nlmsg_seq = MPLS_SEQ_NUM;
  
  /* Check whether this is an init msg or an end msg */
  if (init == MPLS_INIT)
    req_pack.nlmh.nlmsg_type = USERSOCK_MPLS_VRF_INIT;
  else if (init == MPLS_END)
    req_pack.nlmh.nlmsg_type = USERSOCK_MPLS_VRF_END;

  /* Copy over the ctrl data to the nl msg */
  pal_mem_cpy(data, m_if, sizeof(struct mpls_if));
  
  return netlink_send(netlink, (u_char *)&req_pack, sizeof(req_pack));
}

/*
 * Send init/end message for interface.
 */
int
send_mpls_if_ctrl (int netlink, struct mpls_if *m_if, int init)
{
  struct
  {
    struct nlmsghdr nlmh;
    struct mpls_if mif;
  }
  req_pack;
  void             *data = NLMSG_DATA(&req_pack);

  pal_mem_set(&req_pack, 0, sizeof(req_pack));
  req_pack.nlmh.nlmsg_len = NLMSG_LENGTH(sizeof(struct mpls_if));
  
  req_pack.nlmh.nlmsg_flags = NLM_F_REQUEST | NLM_F_CREATE;
  req_pack.nlmh.nlmsg_seq = MPLS_SEQ_NUM;
  
  /* Check whether this is an init msg or an end msg */
  if (init == MPLS_INIT)
    req_pack.nlmh.nlmsg_type = USERSOCK_MPLS_IF_INIT;
  else if (init == MPLS_END)
    req_pack.nlmh.nlmsg_type = USERSOCK_MPLS_IF_END;

  /* Copy over the ctrl data to the nl msg */
  pal_mem_cpy(data, m_if, sizeof(struct mpls_if));
  
  return netlink_send(netlink, (u_char *)&req_pack, sizeof(req_pack));
}

/*
 * Send init/end message for Virtual Circuit.
 */
int
send_mpls_vc_ctrl (int netlink, struct mpls_if *m_if, int init)
{
  struct
  {
    struct nlmsghdr nlmh;
    struct mpls_if mif;
  }
  req_pack;
  void             *data = NLMSG_DATA(&req_pack);

  pal_mem_set(&req_pack, 0, sizeof(req_pack));
  req_pack.nlmh.nlmsg_len = NLMSG_LENGTH(sizeof(struct mpls_if));
  
  req_pack.nlmh.nlmsg_flags = NLM_F_REQUEST | NLM_F_CREATE;
  req_pack.nlmh.nlmsg_seq = MPLS_SEQ_NUM;
  
  /* Check whether this is an init msg or an end msg */
  if (init == MPLS_INIT)
    req_pack.nlmh.nlmsg_type = USERSOCK_MPLS_VC_INIT;
  else if (init == MPLS_END)
    req_pack.nlmh.nlmsg_type = USERSOCK_MPLS_VC_END;

  /* Copy over the ctrl data to the nl msg */
  pal_mem_cpy(data, m_if, sizeof(struct mpls_if));
  
  return netlink_send(netlink, (u_char *)&req_pack, sizeof(req_pack));
}

/*
 * Update VRF data for Interface.
 */
int
send_mpls_if_updt (int netlink, struct mpls_if *m_if)
{
struct
  {
    struct nlmsghdr nlmh;
    struct mpls_if mif;
  }
  req_pack;
  void             *data = NLMSG_DATA(&req_pack);

  pal_mem_set(&req_pack, 0, sizeof(req_pack));
  req_pack.nlmh.nlmsg_len = NLMSG_LENGTH(sizeof(struct mpls_if));
  
  req_pack.nlmh.nlmsg_flags = NLM_F_REQUEST | NLM_F_CREATE;
  req_pack.nlmh.nlmsg_seq = MPLS_SEQ_NUM;  
  req_pack.nlmh.nlmsg_type = USERSOCK_IF_UPDATE;


  /* Copy over the ctrl data to the nl msg */
  pal_mem_cpy(data, m_if, sizeof(struct mpls_if));
  
  return netlink_send(netlink, (u_char *)&req_pack, sizeof(req_pack));
}

/*
 * The following routine may be used to clean ILM/FTN entries
 * for a specified protocol.
 */
int
send_fib_clean (int netlink, struct mpls_control *m_ctrl)
{
  struct
  {
    struct nlmsghdr nlmh;
    struct mpls_control m;
  } req_pack;

  void             *data = NLMSG_DATA(&req_pack);

  pal_mem_set(&req_pack, 0, sizeof(req_pack));
  req_pack.nlmh.nlmsg_len = NLMSG_LENGTH(sizeof(struct mpls_control));
  
  req_pack.nlmh.nlmsg_flags = NLM_F_REQUEST | NLM_F_CREATE;
  req_pack.nlmh.nlmsg_type = USERSOCK_FIB_CLEAN;
  req_pack.nlmh.nlmsg_seq = MPLS_SEQ_NUM;
  
  /* Copy over the ctrl data to the nl msg */
  pal_mem_cpy(data, m_ctrl, sizeof(struct mpls_control));
  
  return netlink_send(netlink, (u_char *)&req_pack, sizeof(req_pack));
}

/*
 * The following routine may be used to remove all VRF tables
 * from the MPLS Forwarder.
 */
int
send_vrf_clean (int netlink, struct mpls_control *m_ctrl)
{
  struct
  {
    struct nlmsghdr nlmh;
    struct mpls_control m;
  } req_pack;

  void             *data = NLMSG_DATA(&req_pack);

  pal_mem_set(&req_pack, 0, sizeof(req_pack));
  req_pack.nlmh.nlmsg_len = NLMSG_LENGTH(sizeof(struct mpls_control));
  
  req_pack.nlmh.nlmsg_flags = NLM_F_REQUEST | NLM_F_CREATE;
  req_pack.nlmh.nlmsg_type = USERSOCK_VRF_CLEAN;
  req_pack.nlmh.nlmsg_seq = MPLS_SEQ_NUM;
  
  /* Copy over the ctrl data to the nl msg */
  pal_mem_cpy(data, m_ctrl, sizeof(struct mpls_control));
  
  return netlink_send(netlink, (u_char *)&req_pack, sizeof(req_pack));
}

/*---------------------------------------------------------------------------
 * function to send ILM add info to the kernel
 *--------------------------------------------------------------------------*/
int
send_ilm_add (int netlink, struct ilm_add_struct *ia_req) 
{
  struct
  {
    struct nlmsghdr   nlmh;
    struct ilm_add_struct m;
  }
  req_pack;
  void             *data = NLMSG_DATA(&req_pack);
  
  pal_mem_set(&req_pack, 0, sizeof(req_pack));
  req_pack.nlmh.nlmsg_len = NLMSG_LENGTH(sizeof(struct ilm_add_struct));
  
  req_pack.nlmh.nlmsg_flags = NLM_F_REQUEST | NLM_F_CREATE;
  req_pack.nlmh.nlmsg_type = USERSOCK_NEWILM;
  req_pack.nlmh.nlmsg_seq = MPLS_SEQ_NUM;
  
  ia_req->rtnl_family = AF_INET;
  pal_mem_cpy(data, ia_req, sizeof(struct ilm_add_struct));
  
  return netlink_send(netlink, (u_char *)&req_pack, sizeof(req_pack));
}

/*---------------------------------------------------------------------------
 * function to send ILM delete info to the kernel
 *--------------------------------------------------------------------------*/
int
send_ilm_del(int netlink, struct ilm_del_struct *id_req)
{
  struct
  {
    struct nlmsghdr   nlmh;
    struct ilm_del_struct m;
  }
  req_pack;
  void             *data = NLMSG_DATA(&req_pack);
  
  pal_mem_set(&req_pack, 0, sizeof(req_pack));
  req_pack.nlmh.nlmsg_len = NLMSG_LENGTH(sizeof(struct ilm_del_struct));
  
  req_pack.nlmh.nlmsg_flags = NLM_F_REQUEST | NLM_F_CREATE;
  req_pack.nlmh.nlmsg_type = USERSOCK_DELILM;
  req_pack.nlmh.nlmsg_seq = MPLS_SEQ_NUM;
  
  id_req->rtnl_family = AF_INET;
  pal_mem_cpy(data, id_req, sizeof(struct ilm_del_struct));
  
  return netlink_send(netlink, (u_char *)&req_pack, sizeof(req_pack));
}

/*---------------------------------------------------------------------------
 * function to send FTN add info to the kernel
 *--------------------------------------------------------------------------*/

int
send_ftn_add(int netlink, struct ftn_add_struct *fa_req)
{
  struct
  {
    struct nlmsghdr   nlmh;
    struct ftn_add_struct m;
  }
  req_pack;
  void             *data = NLMSG_DATA(&req_pack);
  
  pal_mem_set(&req_pack, 0, sizeof(req_pack));
  req_pack.nlmh.nlmsg_len = NLMSG_LENGTH(sizeof(struct ftn_add_struct));
  
  req_pack.nlmh.nlmsg_flags = NLM_F_REQUEST | NLM_F_CREATE;
  req_pack.nlmh.nlmsg_type = USERSOCK_NEWFTN;
  req_pack.nlmh.nlmsg_seq = MPLS_SEQ_NUM;
  
  fa_req->rtnl_family = AF_INET;
  pal_mem_cpy(data, fa_req, sizeof(struct ftn_add_struct));
  
  return netlink_send(netlink, (u_char *)&req_pack, sizeof(req_pack));
}

/*---------------------------------------------------------------------------
 * function to send FTN delete info to the kernel
 *--------------------------------------------------------------------------*/
int
send_ftn_del(int netlink, struct ftn_del_struct *fd_req)
{
  struct
  {
    struct nlmsghdr   nlmh;
    struct ftn_del_struct m;
  }
  req_pack;
  void             *data = NLMSG_DATA(&req_pack);
  
  pal_mem_set(&req_pack, 0, sizeof(req_pack));
  req_pack.nlmh.nlmsg_len = NLMSG_LENGTH(sizeof(struct ftn_del_struct));
  
  req_pack.nlmh.nlmsg_flags = NLM_F_REQUEST | NLM_F_CREATE;
  req_pack.nlmh.nlmsg_type = USERSOCK_DELFTN;
  req_pack.nlmh.nlmsg_seq = MPLS_SEQ_NUM;
  
  fd_req->rtnl_family = AF_INET;
  pal_mem_cpy(data, fd_req, sizeof(struct ftn_del_struct));
  
  return netlink_send(netlink, (u_char *)&req_pack, sizeof(req_pack));
}

/*---------------------------------------------------------------------------
 * function to send VC specific FTN add info to the kernel
 *--------------------------------------------------------------------------*/

int
send_vc_ftn_add(int netlink, struct vc_ftn_add_struct *fa_req)
{
  struct
  {
    struct nlmsghdr   nlmh;
    struct vc_ftn_add_struct m;
  }
  req_pack;
  void             *data = NLMSG_DATA(&req_pack);
  
  pal_mem_set(&req_pack, 0, sizeof(req_pack));
  req_pack.nlmh.nlmsg_len = NLMSG_LENGTH(sizeof(struct vc_ftn_add_struct));
  
  req_pack.nlmh.nlmsg_flags = NLM_F_REQUEST | NLM_F_CREATE;
  req_pack.nlmh.nlmsg_type = USERSOCK_NEW_VC_FTN;
  req_pack.nlmh.nlmsg_seq = MPLS_SEQ_NUM;
  
  fa_req->rtnl_family = AF_INET;
  pal_mem_cpy(data, fa_req, sizeof(struct vc_ftn_add_struct));
  
  return netlink_send(netlink, (u_char *)&req_pack, sizeof(req_pack));
}

/*---------------------------------------------------------------------------
 * function to send VC specific FTN delete info to the kernel
 *--------------------------------------------------------------------------*/
int
send_vc_ftn_del(int netlink, struct vc_ftn_del_struct *fd_req)
{
  struct
  {
    struct nlmsghdr   nlmh;
    struct vc_ftn_del_struct m;
  }
  req_pack;
  void             *data = NLMSG_DATA(&req_pack);
  
  pal_mem_set(&req_pack, 0, sizeof(req_pack));
  req_pack.nlmh.nlmsg_len = NLMSG_LENGTH(sizeof(struct vc_ftn_del_struct));
  
  req_pack.nlmh.nlmsg_flags = NLM_F_REQUEST | NLM_F_CREATE;
  req_pack.nlmh.nlmsg_type = USERSOCK_DEL_VC_FTN;
  req_pack.nlmh.nlmsg_seq = MPLS_SEQ_NUM;
  
  fd_req->rtnl_family = AF_INET;
  pal_mem_cpy(data, fd_req, sizeof(struct vc_ftn_del_struct));
  
  return netlink_send(netlink, (u_char *)&req_pack, sizeof(req_pack));
}

/*---------------------------------------------------------------------------
 * function to enable IP Forwarding
 *--------------------------------------------------------------------------*/

char mpls_proc_ipv4_forwarding[] = "/proc/sys/net/ipv4/ip_forward";
void
enable_ip_fwding ()
{
  FILE *fp;
    
  /* Open file for write */
  fp = fopen (mpls_proc_ipv4_forwarding, "w");
  if (fp == NULL)
    {
      fprintf (stderr, "Could not get correct file pointer to %s",
               mpls_proc_ipv4_forwarding);
      return;
    }
  
  /* Put in char '1' into the file to turn on IP Forwarding */
  fprintf (fp, "1\n");

  /* Close file handle */
  fclose (fp);
}

/*---------------------------------------------------------------------------
 * Function to update TTL handling.
 *--------------------------------------------------------------------------*/
int
send_ttl (int netlink, struct mpls_ttl_handling *m_ttl)
{
  struct
  {
    struct nlmsghdr   nlmh;
    struct mpls_ttl_handling m;
  }
  req_pack;
  void             *data = NLMSG_DATA(&req_pack);
  
  pal_mem_set(&req_pack, 0, sizeof(req_pack));
  req_pack.nlmh.nlmsg_len = NLMSG_LENGTH(sizeof(struct mpls_ttl_handling));
  
  req_pack.nlmh.nlmsg_flags = NLM_F_REQUEST | NLM_F_CREATE;
  req_pack.nlmh.nlmsg_type = USERSOCK_TTL_HANDLING;
  req_pack.nlmh.nlmsg_seq = MPLS_SEQ_NUM;
  
  pal_mem_cpy(data, m_ttl, sizeof(struct mpls_ttl_handling));
  
  return netlink_send(netlink, (u_char *)&req_pack, sizeof(req_pack));
}

/*---------------------------------------------------------------------------
 * Function to update handling of locally generated TCP packets.
 *--------------------------------------------------------------------------*/
int
send_local_handle (int netlink, struct mpls_conf_handle *m_local)
{
  struct
  {
    struct nlmsghdr   nlmh;
    struct mpls_conf_handle m;
  }
  req_pack;
  void             *data = NLMSG_DATA(&req_pack);
  
  pal_mem_set(&req_pack, 0, sizeof(req_pack));
  req_pack.nlmh.nlmsg_len =
    NLMSG_LENGTH (sizeof (struct mpls_conf_handle));
  
  req_pack.nlmh.nlmsg_flags = NLM_F_REQUEST | NLM_F_CREATE;
  req_pack.nlmh.nlmsg_type = USERSOCK_LOCAL_PKT;
  req_pack.nlmh.nlmsg_seq = MPLS_SEQ_NUM;
  
  pal_mem_cpy(data, m_local, sizeof (struct mpls_conf_handle));
  
  return netlink_send(netlink, (u_char *)&req_pack, sizeof(req_pack));
}

/*---------------------------------------------------------------------------
 * Function to clear out MPLS Forwarding statistics.
 *--------------------------------------------------------------------------*/
int
send_clear_stats (int netlink, struct mpls_stat_clear_handle *m_stat_clear)
{
  struct
  {
    struct nlmsghdr   nlmh;
    struct mpls_stat_clear_handle m;
  }
  req_pack;
  void             *data = NLMSG_DATA(&req_pack);
  
  pal_mem_set(&req_pack, 0, sizeof(req_pack));
  req_pack.nlmh.nlmsg_len =
    NLMSG_LENGTH (sizeof (struct mpls_stat_clear_handle));
  
  req_pack.nlmh.nlmsg_flags = NLM_F_REQUEST | NLM_F_CREATE;
  req_pack.nlmh.nlmsg_type = USERSOCK_STATS_CLEAR;
  req_pack.nlmh.nlmsg_seq = MPLS_SEQ_NUM;
  
  pal_mem_cpy(data, m_stat_clear, sizeof (struct mpls_stat_clear_handle));
  
  return netlink_send(netlink, (u_char *)&req_pack, sizeof(req_pack));
}

/*-------------------------------------------------------------------------
 *         Send RSVP packet over the bypass tunnel forecebly
 *------------------------------------------------------------------------*/
int 
send_control_pkt_over_mpls (int netlink, u_int32_t ftn_index, u_int32_t cpkt_id,
                            u_int32_t flags, u_int32_t total_len,
                            u_int32_t seq_num, u_int32_t pkt_size, u_char *pkt)
{
  struct
  {
    struct nlmsghdr   nlmh;
    u_int32_t         ftn_index;
    u_int32_t         cpkt_id;
    u_int32_t         flags;
    u_int32_t         total_len;
    u_int32_t         seq_num;
    u_int32_t         pkt_size;
    u_char            buf [MPLS_PKT_SIZE_MAX];
  }
  req_pack;
  void             *data = NLMSG_DATA (&req_pack);

  pal_mem_set (&req_pack, 0, sizeof(req_pack));
  
  /* 24 is the size of the six int32's */
  req_pack.nlmh.nlmsg_len = NLMSG_LENGTH (24 + pkt_size);

  req_pack.ftn_index = ftn_index;
  req_pack.cpkt_id = cpkt_id;
  req_pack.flags = flags;
  req_pack.total_len = total_len;
  req_pack.seq_num = seq_num;
  req_pack.pkt_size = pkt_size;
  req_pack.nlmh.nlmsg_flags = NLM_F_REQUEST | NLM_F_CREATE;
  req_pack.nlmh.nlmsg_type = USERSOCK_BYPASS_SEND;
  req_pack.nlmh.nlmsg_seq = MPLS_SEQ_NUM;

  pal_mem_cpy(data + 24, pkt, pkt_size);
  return netlink_send (netlink, (u_char *)&req_pack, sizeof(req_pack));
}

/*---------------------------------------------------------------------------
 * Function to turn debugging on or off.
 *--------------------------------------------------------------------------*/
int
send_debugging (int netlink, struct mpls_conf_handle *m_debug)
{
  struct
  {
    struct nlmsghdr   nlmh;
    struct mpls_conf_handle m;
  }
  req_pack;
  void             *data = NLMSG_DATA(&req_pack);
  
  pal_mem_set(&req_pack, 0, sizeof(req_pack));
  req_pack.nlmh.nlmsg_len =
    NLMSG_LENGTH (sizeof (struct mpls_conf_handle));
  
  req_pack.nlmh.nlmsg_flags = NLM_F_REQUEST | NLM_F_CREATE;
  req_pack.nlmh.nlmsg_type = USERSOCK_DEBUGGING;
  req_pack.nlmh.nlmsg_seq = MPLS_SEQ_NUM;
  
  pal_mem_cpy(data, m_debug, sizeof (struct mpls_conf_handle));
  
  return netlink_send(netlink, (u_char *)&req_pack, sizeof(req_pack));
}

/*****************************************************************************/


/*
 * These APIs will be executed by the APN protocols in order to
 * interact with the MPLS forwarder.
 */

/* Static netlink socket definition */
static int netlink_sock = -1;
static struct thread *t_mpls_netlink = NULL;

OAM_NETLINK_CALLBACK oam_netlink_callback;

int
mpls_netlink_recv (struct thread *t)
{
  int ret;
  struct sockaddr_nl nladdr;
  struct nlmsghdr *nlmh;
  struct mpls_oam_ctrl_data *ctrl_data;
  u_char buf[4096];
  u_char *ptr;
  int len;
  int header_len;

  t_mpls_netlink = NULL;

  pal_mem_set(&nladdr, 0, sizeof(nladdr));
  pal_mem_set (buf, 0, 4096);
  ret = recvfrom (netlink_sock, buf, 4096, 0, (struct sockaddr *)&nladdr, &len);

  nlmh = (struct nlmsghdr *)buf;
  header_len = NLMSG_ALIGN (sizeof (struct nlmsghdr)) + 
               NLMSG_ALIGN (sizeof (struct mpls_oam_ctrl_data));
  len = nlmh->nlmsg_len - header_len;
  ctrl_data = (struct mpls_oam_ctrl_data *)(buf + NLMSG_ALIGN (sizeof (struct nlmsghdr)));
  ptr = buf + header_len;
 
  if (oam_netlink_callback)
    oam_netlink_callback (ptr, len, ctrl_data, nlmh->nlmsg_type);

  t_mpls_netlink = thread_add_read (t->zg, mpls_netlink_recv, ctrl_data, netlink_sock);
  
  return 0;
}

/* 
 * Initialize the layer-2 connection.
 */
int
pal_apn_mpls_init_all_handles (struct lib_globals *zg, u_char protocol)
{
  struct mpls_control mc;
  int ret;

  /* Enable IP Forwarding on this LSR */
  enable_ip_fwding ();
  
  /* Initialize the socket */
  netlink_sock = netlink_open ();
  if (netlink_sock <= 0)
    {
      perror ("pal_apn_mpls_init_all_handles : Could not intialize Netlink "
              "socket to MPLS Forwarder");
      return -1;
    }
  
  /* Set the protocol */
  mc.protocol = protocol;

  ret = send_mpls_control (netlink_sock, &mc, MPLS_INIT);
  if (ret < 0)
    {
      perror ("pal_apn_mpls_init_all_handles : Could not send init to MPLS "
              "Forwarder");
      return -1;
    }

  /* launch a read thread */
  t_mpls_netlink = thread_add_read (zg, mpls_netlink_recv, NULL, netlink_sock);

  return netlink_sock;
}

int
pal_register_mpls_oam_netlink_callback (OAM_NETLINK_CALLBACK callback) 
{
  oam_netlink_callback = callback;
  return 0;
}

/*
 * Close the layer-2 connection.
 */
int
pal_apn_mpls_close_all_handles (u_char protocol)
{
  struct mpls_control mc;
  int ret;

  if (netlink_sock < 0)
    return -1;

  /* Set the protocol */
  mc.protocol = protocol;

  ret = send_mpls_control (netlink_sock, &mc, MPLS_END);
  if (ret < 0)
    {
      perror ("pal_apn_mpls_close_all_handles : Could not send close signal "
              "to MPLS Forwarder");
      return -1;
    }      

  /* Close the previously opened global socket */
  if (netlink_sock > 0)
    {
      close (netlink_sock);
      netlink_sock = -1;
    }

  return 0;
}

/*
 * Create a VRF table in the MPLS Forwarder.
 */
int
pal_apn_mpls_vrf_init (int vrf_ident)
{
  struct mpls_if mif;
  int ret;

  if (netlink_sock < 0)
    return -1;

  /* Start with clean mif. */
  pal_mem_set (&mif, 0, sizeof (struct mpls_if));
  
  /* Set the vrf */
  mif.vrf_ident = vrf_ident;

  ret = send_mpls_vrf_ctrl (netlink_sock, &mif, MPLS_INIT);
  if (ret < 0)
    {
      perror ("pal_apn_mpls_vrf_init : Could not create VRF " 
              "in MPLS Forwarder");
      return -1;
    }

  return 0;
}
  
/*
 * Delete a VRF table from the MPLS Forwarder.
 */
int
pal_apn_mpls_vrf_end (int vrf_ident)
{
  struct mpls_if mif;
  int ret;

  if (netlink_sock < 0)
    return -1;

  /* Start with clean mif. */
  pal_mem_set (&mif, 0, sizeof (struct mpls_if));

  /* Set the vrf */
  mif.vrf_ident = vrf_ident;

  ret = send_mpls_vrf_ctrl (netlink_sock, &mif, MPLS_END);
  if (ret < 0)
    {
      perror ("pal_apn_mpls_vrf_end : Could not remove VRF " 
              "from MPLS Forwarder");
      return -1;
    }

  return 0;
}

/*
 * Enable an interface for MPLS.
 */
int
pal_apn_mpls_if_init (struct if_ident *if_info,
                  unsigned short label_space)
{
  struct mpls_if m_if;
  int ret;

  if (netlink_sock < 0)
    return -1;

  /* Start with clean mif. */
  pal_mem_set (&m_if, 0, sizeof (struct mpls_if));

  /* Set ifname */
  pal_strcpy (m_if.ifname, if_info->if_name);

  /* Set the ifindex */
  m_if.ifindex = if_info->if_index;

  /* Set the label space */
  m_if.label_space = label_space;

  ret = send_mpls_if_ctrl (netlink_sock, &m_if, MPLS_INIT);
  if (ret < 0)
    {
      perror ("pal_apn_mpls_if_init : Could not add interface data " 
              "to MPLS Forwarder");
      return -1;
    }

  return 0;
}

/*
 * Update the VRF that an MPLS interface points to.
 */
int
pal_apn_mpls_if_update_vrf (struct if_ident *if_info,
                            int vrf_ident)
{
  struct mpls_if m_if;
  int ret;

  if (netlink_sock < 0)
    return -1;

  /* Make sure that the loopback interface is not being passed here */
  if (if_info->if_index <= 1)
    {
      perror ("pal_apn_mpls_if_update_vrf: Invalid interface");
      return -1;
    }

  /* Start with clean mif. */
  pal_mem_set (&m_if, 0, sizeof (struct mpls_if));

  /* Set interface index */
  m_if.ifindex = if_info->if_index;
  
  /* set interface name */
  pal_strcpy (m_if.ifname, if_info->if_name);

  /* Set label space to zero */
  m_if.label_space = 0;

  /* Make sure that the vrf ident is not zero */
  pal_assert (vrf_ident != 0);

  /* Check for vrf identifier */
  m_if.vrf_ident = vrf_ident;

  ret = send_mpls_if_updt (netlink_sock, &m_if);
  if (ret < 0)
    {
      perror ("pal_apn_mpls_if_update_vrf : Could not update interface data "
              "in MPLS Forwarder");
      return -1;
    }

  return 0;
}

/*
 * Disable MPLS on an interface.
 */
int
pal_apn_mpls_if_end (struct if_ident *if_info)
{
  struct mpls_if m_if;
  int ret;

  if (netlink_sock < 0)
    return -1;

  /* Make sure that the loopback interface is not being passed here */
  if (if_info->if_index <= 1)
    {
      perror ("pal_apn_mpls_if_end: Invalid interface");
      return -1;
    }

  /* Start with clean mif. */
  pal_mem_set (&m_if, 0, sizeof (struct mpls_if));

  /* Set interface name */
  pal_strcpy (m_if.ifname, if_info->if_name);

  /* Set interface index */
  m_if.ifindex = if_info->if_index;
  
  ret = send_mpls_if_ctrl (netlink_sock, &m_if, MPLS_END);
  if (ret < 0)
    {
      perror ("pal_apn_mpls_if_end : Could not remove interface data " 
              "from MPLS Forwarder");
      return -1;
    }

  return 0;
}

/*
 * Add an ILM entry to the ILM table.
 */
int
pal_apn_mpls_ilm_entry_add (u_char protocol,
                            unsigned int *label_id_in,
                            unsigned int *label_id_out,
                            struct if_ident *if_in,
                            struct if_ident *if_out,
                            struct pal_in4_addr *fec_addr,
                            u_char *fec_prefixlen,
                            struct pal_in4_addr *nexthop_addr,
                            u_char is_egress,

#if (!defined (HAVE_GMPLS) || defined (HAVE_PACKET))
#ifdef HAVE_DIFFSERV
                            struct ds_info_fwd *ds_info,
#endif /* HAVE_DIFFSERV */
#endif /* !HAVE_GMPLS || HAVE_PACKET */

                            char opcode,
                            u_int32_t nhlfe_ix,
                            struct mpls_owner_fwd *owner)
{
  struct ilm_add_struct ia;
  int ret;

  if (netlink_sock < 0)
    return -1;

  if (!label_id_in || !if_in || !if_out || !fec_addr ||
      !fec_prefixlen || !nexthop_addr)
    return -1;

  /* Set the protocol */
  ia.protocol = protocol;

  /* Set the incoming label */
  ia.in_label = *label_id_in;

  /* No interface specific label space */
  ia.inl2id = if_in->if_index;
  pal_strcpy (ia.inl2name, if_in->if_name);

  /* Set the fec info */
  ia.fec.family = AF_INET;
  ia.fec.u.prefix4.s_addr = fec_addr->s_addr;
  ia.fec.prefixlen = *fec_prefixlen;
  
  /* Set the nexthop addr */
  ia.nexthop = nexthop_addr->s_addr;

  /* Outgoing ifindex */
  ia.outl2id = if_out->if_index;
  pal_strcpy (ia.outintf, if_out->if_name);

  /* Set the outgoing label */
  if (label_id_out)
    {
      ia.out_label = *label_id_out;
    }
  if ((! label_id_out) || is_egress)
    {
      ia.out_label = -1;
    }

  /* Pass in the opcode */
  ia.opcode = opcode;
  
  /* Fill NHLFE index. */
  ia.nhlfe_ix = nhlfe_ix;

  /* Fill Owner information. */
  pal_mem_cpy (&ia.owner, owner, sizeof (struct mpls_owner_fwd));

  /* Now send the ILM entry to the kernel */
  ret = send_ilm_add (netlink_sock, &ia);
  if (ret < 0)
    {
      perror ("send_ilm_add : Netlink socket failed");
      return ret;
    }

  return 0;
}

#ifdef HAVE_IPV6
int
pal_apn_mpls_ilm6_entry_add (u_char protocol,
                            unsigned int *label_id_in,
                            unsigned int *label_id_out,
                            struct if_ident *if_in,
                            struct if_ident *if_out,
                            struct pal_in6_addr *fec_addr,
                            u_char *fec_prefixlen,
                            struct mpls_nh_fwd *nexthop_addr,
                            u_char is_egress,
#if (!defined (HAVE_GMPLS) || defined (HAVE_PACKET))
#ifdef HAVE_DIFFSERV
                            struct ds_info_fwd *ds_info,
#endif /* HAVE_DIFFSERV */
#endif /* !HAVE_GMPLS || HAVE_PACKET */

                            char opcode,
                            u_int32_t nhlfe_ix,
                            struct mpls_owner_fwd *owner)
{
  return 0;
}
#endif

/*
 * Add an FTN entry to the FTN table.
 */
int
pal_apn_mpls_ftn_entry_add (int vrf,
                            u_char protocol,
                            unsigned int *label_id_out,
                            struct pal_in4_addr *fec_addr,
                            u_char *prefixlen,
                            u_char *in_dscp,
                            u_int32_t *tunnel_id,
                            u_int32_t *qos_resrc_id,
                            struct pal_in4_addr *nexthop_addr,
                            struct if_ident *if_info,
#if (!defined (HAVE_GMPLS) || defined (HAVE_PACKET))
#ifdef HAVE_DIFFSERV
                            struct ds_info_fwd *ds_info,
#endif /* HAVE_DIFFSERV */
#endif /* !HAVE_GMPLS || HAVE_PACKET */

                            char opcode,
                            u_int32_t nhlfe_ix,
                            u_int32_t ftn_ix,
                            u_char ftn_type,
                            struct mpls_owner_fwd *owner,
                            u_int32_t bypass_ftn_ix,
                            u_char lsp_type,
			    int active_head)
{
  struct ftn_add_struct fa;
  int ret;

  if (netlink_sock < 0)
    return -1;

  if (!prefixlen || !fec_addr || !nexthop_addr || !if_info)
    return -1;

  /* Make sure that the opcode is correct */
  pal_assert ((opcode == PUSH) || (opcode == PUSH_AND_LOOKUP) ||
          (opcode == DLVR_TO_IP) || (opcode == FTN_LOOKUP));
  
  /* Set the vrf table that we want to add this FTN entry to */
  fa.vrf = vrf;

  /* Protocol set */
  fa.protocol = protocol;

  /* Pass in the opcode */
  fa.opcode = opcode;

  /* Set the FEC prefix */
  fa.fec.u.prefix4 = *fec_addr;

  /* Set the FEC prefixlen */
  fa.fec.prefixlen = *prefixlen;

  /* Set the FEC family */
  fa.fec.family = AF_INET;

  /* Set the nexthop */
  fa.nexthop = nexthop_addr->s_addr;

  /* Fill outgoing interface data. */
  fa.outl2id = if_info->if_index;
  pal_strcpy (fa.outintf, if_info->if_name);

  /* Set the outgoing label */
  fa.out_label = *label_id_out;

  /* Fill NHLFE index. */
  fa.nhlfe_ix = nhlfe_ix;

  /* Fill FTN index. */
  fa.ftn_ix = ftn_ix;

  /* Fill Bypass FTN index. */
  fa.bypass_ftn_ix = bypass_ftn_ix;

  /* Fill Bypass Flag. */
  fa.lsp_type = lsp_type;

  /* Fill active_head. */
  fa.active_head = active_head;

  /* Fill Owner information. */
  pal_mem_cpy (&fa.owner, owner, sizeof (struct mpls_owner_fwd));

  /* Now send the FTN add entry to the kernel */
  ret = send_ftn_add (netlink_sock, &fa);
  if (ret < 0)
    {
      perror ("send_ftn_add : Netlink socket failed");
      return ret;
    }

  return 0;
}

#ifdef HAVE_IPV6
int
pal_apn_mpls_ftn6_entry_add (int vrf,
                             u_char protocol,
                             unsigned int *label_id_out,
                             struct pal_in6_addr *fec_addr,
                             u_char *prefixlen,
                             u_char *in_dscp,
                             u_int32_t *tunnel_id,
                             u_int32_t *qos_resrc_id,
                             struct mpls_nh_fwd *nexthop_addr,
                             struct if_ident *if_info,
#if (!defined (HAVE_GMPLS) || defined (HAVE_PACKET))
#ifdef HAVE_DIFFSERV
                             struct ds_info_fwd *ds_info,
#endif /* HAVE_DIFFSERV */
#endif /* !HAVE_GMPLS || HAVE_PACKET */

                             char opcode,
                             u_int32_t nhlfe_ix,
                             u_int32_t ftn_ix,
                             u_char ftn_type,
                             struct mpls_owner_fwd *owner,
                             u_int32_t bypass_ftn_ix,
                             u_char lsp_type,
			     int active_head)
{
  return 0;
}

#endif

/*
 * Delete an ILM entry to the ILM table.
 *
 * Once the incoming label goes away, the entry becomes an
 * FTN entry.
 */
int
pal_apn_mpls_ilm_entry_del (u_char protocol,
                            unsigned int *label_id_in,
                            struct if_ident *if_info)
{
  struct ilm_del_struct id;
  int ret;

  if (netlink_sock < 0)
    return -1;

  /* Set the incoming label */
  if (label_id_in)
    id.in_label = *label_id_in;
  else
    return -1;

  /* Set protocol */
  id.protocol = protocol;

  if (if_info == NULL)
    {
      printf ("Invalid interface index passed\n");
      return -1;
    }
  
  /* No interface specific label space */
  id.inl2id = if_info->if_index;
  pal_strcpy (id.inl2name, if_info->if_name);

  /* Send the ILM del entry to the kernel */
  ret = send_ilm_del (netlink_sock, &id);
  if (ret < 0)
    {
      perror ("send_ilm_del : Netlink socket failed");
      return ret;
    }

  return 0;
}

/*
 * Delete an FTN entry to the FTN table.
 */
int
pal_apn_mpls_ftn_entry_del (int vrf,
                            u_char protocol,
                            u_int32_t *tunnel_id,
                            struct pal_in4_addr *fec_addr,
                            u_char *prefixlen,
                            u_int32_t ftn_ix)
{
  struct ftn_del_struct fd;
  int ret;

  if (netlink_sock < 0)
    return -1;
  
  if (!fec_addr || !prefixlen)
    return -1;

  /* Set the VRF where we want to add to */
  fd.vrf = vrf;

  /* Set protocol */
  fd.protocol = protocol;
      
  /* Set the fec prefix */
  fd.fec.u.prefix4 = *fec_addr;

  /* Set the fec prefixlen */
  fd.fec.prefixlen = *prefixlen;

  fd.ftn_ix = ftn_ix;
  /* Send the FTN del entry to the kernel */
  ret = send_ftn_del (netlink_sock, &fd);
  if (ret < 0)
    {
      perror ("send_ftn_del : Netlink socket failed");
      return ret;
    }

  return 0;
}

#ifdef HAVE_IPV6
int
pal_apn_mpls_ftn6_entry_del (int vrf,
                             u_char protocol,
                             u_int32_t *tunnel_id,
                             struct pal_in6_addr *fec_addr,
                             u_char *prefixlen,
                             u_int32_t ftn_ix)
{
  return 0;
}

#endif
/*
 * Clean the MPLS FIB of entries populated by the specified
 * protocol.
 */
int
pal_apn_mpls_clean_fib_for (u_char protocol)
{
  struct mpls_control mc;
  int ret;

  if (netlink_sock < 0)
    return -1;

  /* Set the protocol */
  mc.protocol = protocol;
  
  ret = send_fib_clean (netlink_sock, &mc);
  if (ret < 0)
    {
      perror ("pal_apn_mpls_clean_fib_for : "
              "Could not clean MPLS Forwarder");
      return -1;
    }

  return 0;
}

/*
 * Clean the MPLS FIB of entries populated by the specified
 * protocol.
 */
int
pal_apn_mpls_clean_vrf_for (u_char protocol)
{
  struct mpls_control mc;
  int ret;
  
  if (netlink_sock < 0)
    return -1;

  /* Set the protocol */
  mc.protocol = protocol;
  
  ret = send_vrf_clean (netlink_sock, &mc);
  if (ret < 0)
    {
      perror ("pal_apn_mpls_clean_vrf_for :"
              "Could not clean MPLS Forwarder");
      return -1;
    }

  return 0;
}

/*
 * Set the TTL value for ingress/egress for all LSPs.
 *
 * TTL value of -1 means that we wish to use the default handling of TTL.
 */
int
pal_apn_mpls_send_ttl (u_char protocol, u_char type, int ingress, int new_ttl)
{
  struct mpls_ttl_handling m_ttl;
  int ret;

  /* Currently only supporting this via the configuration utility */
  pal_assert (protocol == APN_PROTO_NSM);

  /* Set the protocol */
  m_ttl.protocol = protocol;

  /* Set the type */
  m_ttl.type = type;

  /* Set the ingress value */
  m_ttl.is_ingress = ingress;

  /* Set the new_ttl */
  m_ttl.new_ttl = new_ttl;

  ret = send_ttl (netlink_sock, &m_ttl);
  if (ret < 0)
    {
      perror ("pal_apn_mpls_send_ttl : "
              "Could not change TTL handling");
      return -1;
    }

  return 0;
}

/*
 * Enable/Disable the mapping of locally generated TCP packets.
 */
int
pal_apn_mpls_local_pkt_handle  (u_char protocol, int enable)
{
  struct mpls_conf_handle m_local;
  int ret;

  if (netlink_sock < 0)
    return -1;

  /* Currently only supporting this via the configuration utility */
  pal_assert (protocol == APN_PROTO_NSM);
  
  /* Set the protocol */
  m_local.protocol = protocol;

  /* Set msg type to max */
  m_local.msg_type = KERN_MSG_MAX;

  /* Set the enable/disable flag */
  m_local.enable = enable;

  ret = send_local_handle (netlink_sock, &m_local);
  if (ret < 0)
    {
      perror ("pal_apn_mpls_local_pkt_handle : "
              "Could not change handling of locally generated TCP packets");
      return -1;
    }
  
  return 0;
}

/*
 * Enable/Disable debugging.
 */
int
pal_apn_mpls_debugging_handle  (u_char protocol, u_char msg_type,
                            int enable)
{
  struct mpls_conf_handle m_debug;
  int ret;

  if (netlink_sock < 0)
    return -1;

  /* Currently only supporting this via the configuration utility */
  pal_assert (protocol == APN_PROTO_NSM);

  /* Message type cannot be max */
  pal_assert (msg_type != KERN_MSG_MAX);
  
  /* Set the protocol */
  m_debug.protocol = protocol;

  /* Set the msg_type */
  m_debug.msg_type = msg_type;

  /* Set the enable/disable flag */
  m_debug.enable = enable;

  ret = send_debugging (netlink_sock, &m_debug);
  if (ret < 0)
    {
      perror ("pal_apn_mpls_debugging_handle : "
              "Could not change debugging status");
      return -1;
    }
  
  return 0;
}

/*
 * Bind an interface to a Virtual Circuit.
 */
int
pal_apn_mpls_vc_init (struct if_ident *if_info, u_int32_t vc_id)
{
  struct mpls_if m_if;
  int ret;

  if (netlink_sock < 0)
    return -1;

  /* Make sure that the loopback interface is not being passed here */
  if (if_info->if_index <= 1)
    {
      perror ("pal_apn_mpls_vc_init: Invalid Interface");
      return -1;
    }

  /* Start with clean mif. */
  pal_mem_set (&m_if, 0, sizeof (struct mpls_if));

  /* Set ifname. */
  pal_strcpy (m_if.ifname, if_info->if_name);

  /* Set the ifindex. */
  m_if.ifindex = if_info->if_index;

  /* Set vc id. */
  m_if.vc_id = vc_id;
    
  ret = send_mpls_vc_ctrl (netlink_sock, &m_if, MPLS_INIT);
  if (ret < 0)
    {
      perror ("pal_apn_mpls_vc_init : Could not add Virtual Circuit data " 
              "to MPLS Forwarder");
      return -1;
    }

  return 0;
}

/*
 * Unbind an interface form a Virtual Circuit.
 */
int
pal_apn_mpls_vc_end (struct if_ident *if_info)
{
  struct mpls_if m_if;
  int ret;

  if (netlink_sock < 0)
    return -1;

  /* Make sure that the loopback interface is not being passed here */
  if (if_info->if_index <= 1)
    {
      perror ("pal_apn_mpls_vc_end: Invalid interface");
      return -1;
    }

  /* Start with clean mif. */
  pal_mem_set (&m_if, 0, sizeof (struct mpls_if));

  /* Set interface name. */
  pal_strcpy (m_if.ifname, if_info->if_name);

  /* Set interface index. */
  m_if.ifindex = if_info->if_index;
  
  ret = send_mpls_vc_ctrl (netlink_sock, &m_if, MPLS_END);
  if (ret < 0)
    {
      perror ("pal_apn_mpls_vc_end : Could not remove Virtual Cirtuit data "
              "from MPLS Forwarder");
      return -1;
    }
  
  return 0;
}

/*
 * Send a VC FTN add.
 */
int
pal_apn_mpls_vc_ftn_entry_add  (u_char protocol,
                            unsigned int *vc_id,
                            unsigned int *label_id_out,
                            struct pal_in4_addr *nexthop_addr,
                            struct if_ident *if_in,
                            struct if_ident *if_out,
                            char opcode,
                            u_int32_t tunnel_ftnix)
{
  struct vc_ftn_add_struct fa;
  int ret;

  if (netlink_sock < 0)
    return -1;

  if (! vc_id || ! label_id_out || ! nexthop_addr || ! if_in || ! if_out)
    return -1;

  /* Make sure that the opcode is correct. */
  pal_assert ((opcode == PUSH_FOR_VC) || (opcode == PUSH_AND_LOOKUP_FOR_VC));
  
  /* Protocol set. */
  fa.protocol = protocol;

  /* VC_ID. */
  fa.vc_id = *vc_id;

  /* Set the outgoing label. */
  fa.out_label = *label_id_out;

  /* Set the nexthop. */
  fa.nexthop = nexthop_addr->s_addr;

  /* Fill incoming interface data. */
  fa.inl2id = if_in->if_index;
  pal_strcpy (fa.inintf, if_in->if_name);

  /* Fill outgoing interface data. */
  fa.outl2id = if_out->if_index;
  pal_strcpy (fa.outintf, if_out->if_name);

  /* Pass in the opcode. */
  fa.opcode = opcode;

  /* Pass tunnel_label */
  fa.tunnel_ftnix = tunnel_ftnix;

  /* Now send the FTN add entry to the kernel. */
  ret = send_vc_ftn_add (netlink_sock, &fa);
  if (ret < 0)
    {
      perror ("send_vc_ftn_add : Netlink socket failed");
      return ret;
    }

  return 0;
}

#ifdef HAVE_IPV6
int
pal_apn_mpls_vc_ftn6_entry_add  (u_char protocol,
                                 unsigned int *vc_id,
                                 unsigned int *label_id_out,
                                 struct mpls_nh_fwd *nexthop_addr,
                                 struct if_ident *if_in,
                                 struct if_ident *if_out,
                                 char opcode,
                                 u_int32_t tunnel_ftnix)
{
  return 0;
}
#endif

/*
 * Send a MPLS VC FIB add
 */
int
pal_apn_mpls_l2_circuit_fib_add (u_char protocol,
                                 unsigned int *vc_id,
                                 unsigned int *vpls_id,
                                 unsigned int *in_label,
                                 unsigned int *out_label,
                                 struct pal_in4_addr *vc_nhop_addr,
                                 struct pal_in4_addr *vc_peer_addr,
                                 struct if_ident *if_in,
                                 struct if_ident *if_out,
                                 struct ilm_entry *ilm,
                                 char opcode)
{
  /* This is a wrapper function that will call existing VC FTN and ILM add
   * API's.
   */
  int ret;
  u_int32_t tunnel_ftnix = 0;
  /* FTN Entry */
  ret = pal_apn_mpls_vc_ftn_entry_add (protocol,
                                       vc_id,
                                       out_label,
                                       opcode == PUSH_FOR_VC ? vc_nhop_addr : vc_peer_addr,
                                       if_in,
                                       if_out,
                                       opcode,
                                       tunnel_ftnix);
   if (ret < 0)
     return ret;
  /* ILM Entry is not supported on the PAL. This is a generic entry.*/
  if (opcode == POP_FOR_VC)
    {
      struct prefix p;
      u_int32_t vpn_id = 0;
      struct pal_in4_addr *vc_peer = NULL;
      struct mpls_owner_fwd owner;
      u_int32_t is_egress = 1;

      p.family = ilm->family;
      p.prefixlen = ilm->prefixlen;

#ifdef HAVE_IPV6
      if (p.family == AF_INET6)
        pal_mem_cpy (&p.u.prefix6, ilm->prfx, sizeof (struct pal_in6_addr));
      else
#endif
        pal_mem_cpy (&p.u.prefix4, ilm->prfx, sizeof (struct pal_in4_addr));

      vpn_id = ilm->owner.u.vc_key.vc_id;
      vc_peer = &ilm->owner.u.vc_key.vc_peer;
      pal_mem_set (&owner, 0 , sizeof (struct mpls_owner_fwd));
      if (ilm->owner.owner == MPLS_RSVP &&
          ilm->owner.u.r_key.afi == AFI_IP)
        {
          struct rsvp_key_fwd *key = &owner.u.r_key;
          owner.protocol = APN_PROTO_RSVP;
          key->afi = AFI_IP;
          key->len = sizeof (struct rsvp_key_ipv4_fwd);
          key->u.ipv4.trunk_id = ilm->owner.u.r_key.u.ipv4.trunk_id;
          key->u.ipv4.lsp_id = ilm->owner.u.r_key.u.ipv4.lsp_id;
          IPV4_ADDR_COPY (&key->u.ipv4.egr,
                          &ilm->owner.u.r_key.u.ipv4.egr);
          IPV4_ADDR_COPY (&key->u.ipv4.ingr,
                          &ilm->owner.u.r_key.u.ipv4.ingr);
        }

      ret = pal_apn_mpls_ilm_entry_add (protocol,
                                        in_label,
                                        out_label,
                                        if_in,
                                        if_out,
                                        &p.u.prefix4,
                                        &p.prefixlen,
                                        vc_peer_addr,
                                        is_egress,
#if (!defined (HAVE_GMPLS) || defined (HAVE_PACKET))
#ifdef HAVE_DIFFSERV
                                        (struct ds_info_fwd *)&ilm->ds_info,
#endif /* HAVE_DIFFSERV */
#endif /* !HAVE_GMPLS || HAVE_PACKET */

                                        opcode,
                                        ILM_NHLFE (ilm)->nhlfe_ix,
                                        &owner);
      if (ret < 0)
        return ret;
    }
  return 0;
}

#ifdef HAVE_IPV6
int
pal_apn_mpls_l2_circuit_fib6_add (u_char protocol,
                                  unsigned int *vc_id,
                                  unsigned int *vpls_id,
                                  unsigned int *in_label,
                                  unsigned int *out_label,
                                  struct mpls_nh_fwd *vc_nhop_addr,
                                  struct mpls_nh_fwd *vc_peer_addr,
                                  struct if_ident *if_in,
                                  struct if_ident *if_out,
                                  struct ilm_entry *ilm,
                                  char opcode)
{
  return 0;
}
#endif

/*
 * Send a MPLS VC FIB delete
 */
int
pal_apn_mpls_l2_circuit_fib_delete (u_char protocol,
                                    unsigned int *vc_id,
                                    struct if_ident *if_in)
{
  int ret;
  /* FTN Delete Process */
  ret = pal_apn_mpls_vc_ftn_entry_del  (protocol,
                                        if_in,
                                        vc_id);

  if (ret < 0)
    return ret;

  return 0;
}


/*
 * Send a VC FTN delete.
 */
int
pal_apn_mpls_vc_ftn_entry_del  (u_char protocol,
                            struct if_ident *if_in,
                            unsigned int *vc_id)
{
  struct vc_ftn_del_struct fd;
  int ret;

  if (netlink_sock < 0)
    return -1;
  
  if (! if_in || ! vc_id)
    return -1;

  /* Set protocol. */
  fd.protocol = protocol;

  /* Fill incoming interface data. */
  fd.inl2id = if_in->if_index;
  pal_strcpy (fd.inintf, if_in->if_name);

  /* VC_ID. */
  fd.vc_id = *vc_id;

  /* Send the FTN del entry to the kernel. */
  ret = send_vc_ftn_del (netlink_sock, &fd);
  if (ret < 0)
    {
      perror ("send_vc_ftn_del : Netlink socket failed");
      return ret;
    }

  return 0;
}

/*
 * Enable/Disable the mapping of locally generated TCP packets.
 */
int
pal_apn_mpls_clear_fwd_stats (u_char protocol, u_char type)
{
  struct mpls_stat_clear_handle m_stat_clear;
  int ret;

  if (netlink_sock < 0)
    return -1;

  /* Currently only supporting this via the configuration utility */
  pal_assert (protocol == APN_PROTO_NSM);
  m_stat_clear.protocol = protocol;

  /* Set clear type. */
  m_stat_clear.type = type;

  ret = send_clear_stats (netlink_sock, &m_stat_clear);
  if (ret < 0)
    {
      perror ("pal_apn_mpls_clear_fwd_stats : "
              "Could not clear out MPLS Forwarding statistics");
      return ret;
    }
  
  return 0;
}

int
pal_apn_mpls_fwd_over_tunnel (u_int32_t ftn_index, u_int32_t cpkt_id, 
                              u_int32_t flags, u_int32_t total_len,
                              u_int32_t seq_num, u_int32_t pkt_size,
                              u_char *pkt)
{
  int ret;

  if (netlink_sock < 0)
      return -1;

  ret = send_control_pkt_over_mpls (netlink_sock, ftn_index, cpkt_id, flags, 
                                    total_len, seq_num, pkt_size, pkt);
  if (ret < 0)
  {
      perror ("pal_apn_mpls_fwd_over_tunnel : "
              "Could not forward over MPLS bypass tunnel");
      return ret;
  }

  return 0;
}

int
pal_apn_mpls_fwd_oam_pkt     (u_int32_t ftn_index, u_int32_t vrf_id, 
                              u_int32_t flags, u_int32_t ttl,
                              u_int32_t pkt_size, u_char *pkt)
{
  int ret;

  if (netlink_sock < 0)
      return -1;

  ret = send_oam_pkt_over_mpls     (netlink_sock, ftn_index, vrf_id, flags, 
                                    ttl, pkt_size, pkt);
  if (ret < 0)
  {
      perror ("pal_apn_mpls_fwd_oam_pkt : "
              "Could not forward over MPLS ");
      return ret;
  }

  return 0;
}
int
pal_apn_mpls_fwd_oam_vc_pkt  (u_int32_t if_index, u_int32_t vc_id, 
                              u_int32_t flags, u_int32_t ttl,
                              u_int8_t cc_type, u_int8_t cc_input,
                              u_int32_t pkt_size, u_char *pkt)
{
  int ret;

  if (netlink_sock < 0)
      return -1;

  ret = send_oam_vc_pkt_over_mpls (netlink_sock, if_index, vc_id, flags,ttl,
                                   cc_type, cc_input,
                                   pkt_size, pkt);

  if (ret < 0)
  {
      perror ("pal_apn_mpls_fwd_oam_vc_pkt : "
              "Could not forward over MPLS ");
      return ret;
  }

  return 0;
}


/*-------------------------------------------------------------------------
 *         Send OAM packet over the forwarder
 *------------------------------------------------------------------------*/
int 
send_oam_pkt_over_mpls     (int netlink, u_int32_t ftn_index, u_int32_t vrf_id,
                            u_int32_t flags, u_int32_t ttl, 
                            u_int32_t pkt_size, u_char *pkt)
{
  struct
  {
    struct nlmsghdr   nlmh;
    u_int32_t         ftn_index;
    u_int32_t         vrf_id;
    u_int32_t         flags;
    u_int32_t         ttl;
    u_int32_t         pkt_size;
    u_char            buf [MPLS_PKT_SIZE_MAX];
  }
  req_pack;
  void             *data = NLMSG_DATA (&req_pack);

  pal_mem_set (&req_pack, 0, sizeof(req_pack));
  
  /* 20 is the size of the five int32's */
  req_pack.nlmh.nlmsg_len = NLMSG_LENGTH (20 + pkt_size);

  req_pack.ftn_index = ftn_index;
  req_pack.vrf_id = vrf_id;
  req_pack.flags = flags;
  req_pack.ttl = ttl;
  req_pack.pkt_size = pkt_size;
  req_pack.nlmh.nlmsg_flags = NLM_F_REQUEST | NLM_F_CREATE;
  req_pack.nlmh.nlmsg_type = USERSOCK_OAM_SEND;
  req_pack.nlmh.nlmsg_seq = MPLS_SEQ_NUM;

  pal_mem_cpy(data + 20, pkt, pkt_size);
  return netlink_send (netlink, (u_char *)&req_pack, sizeof(req_pack));
}
/*-------------------------------------------------------------------------
 *         Send OAM VC packet over the forwarder
 *------------------------------------------------------------------------*/
int 
send_oam_vc_pkt_over_mpls  (int netlink, u_int32_t if_index, u_int32_t vc_id,
                            u_int32_t flags, u_int32_t ttl, 
                            u_int8_t cc_type, u_int8_t cc_input,
                            u_int32_t pkt_size, u_char *pkt)
{
  struct
  {
    struct nlmsghdr   nlmh;
    u_int32_t         if_index;
    u_int32_t         vc_id;
    u_int32_t         flags;
    u_int32_t         ttl;
    u_int32_t         cc_type;
    u_int32_t         cc_input;
    u_int32_t         pkt_size;
    u_char            buf [MPLS_PKT_SIZE_MAX];
  }
  req_pack;
  void             *data = NLMSG_DATA (&req_pack);

  pal_mem_set (&req_pack, 0, sizeof(req_pack));
  
  /* 20 is the size of the five int32's  and two int8's*/
  req_pack.nlmh.nlmsg_len = NLMSG_LENGTH (28 + pkt_size);

  req_pack.if_index = if_index;
  req_pack.vc_id = vc_id;
  req_pack.flags = flags;
  req_pack.ttl = ttl;
  req_pack.cc_type = cc_type;
  req_pack.cc_input = cc_input;
  req_pack.pkt_size = pkt_size;
  req_pack.nlmh.nlmsg_flags = NLM_F_REQUEST | NLM_F_CREATE;
  req_pack.nlmh.nlmsg_type = USERSOCK_OAM_VC_SEND;
  req_pack.nlmh.nlmsg_seq = MPLS_SEQ_NUM;

  pal_mem_cpy(data + 28, pkt, pkt_size);
  return netlink_send (netlink, (u_char *)&req_pack, sizeof(req_pack));
}


