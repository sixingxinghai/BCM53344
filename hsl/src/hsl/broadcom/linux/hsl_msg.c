/* Copyright (C) 2004-2005   All Rights Reserved. */

#include "config.h"
#include "hsl_os.h"

#include "hsl_types.h"
                                                                                
#include "bcm_incl.h"
                                                                                
/* HAL includes. */
#include "hal_netlink.h"
#include "hal_msg.h"

#include "hsl_avl.h"
#include "hsl.h"
#include "hsl_oss.h"
#include "hsl_comm.h"
#include "hsl_logger.h"
#include "hsl_ifmgr.h"
#include "hsl_msg.h"
#include "hsl_tlv.h"
#include "hsl_ether.h"
#include "hsl_error.h"

#ifdef HAVE_L2
#include "hsl_vlan.h"
#include "hsl_bridge.h"
#include "bcm_incl.h"
#include "hsl_bcm_vlanclassifier.h"
#include "hsl_bcm_l2.h"
#include "hsl_bcm_lacp.h"
#endif /* HAVE_L2 */

#ifdef HAVE_L3
#include "hsl_table.h"
#include "hsl_fib.h"
#endif /* HAVE_L3 */
#if defined HAVE_MCAST_IPV4 || defined HAVE_MCAST_IPV6 || defined HAVE_IGMP_SNOOP
#include "hsl_mcast_fib.h"
#endif /* HAVE_MCAST_IPV4 || HAVE_MCAST_IPV6 || HAVE_IGMP_SNOOP */
                                                                                
#include "hsl_bcm_if.h"
#include "hsl_bcm_ifmap.h"
#include "hsl_bcm.h"

#ifdef HAVE_AUTHD
#include "hsl_bcm_auth.h"
#endif /* HAVE_AUTHD */

#ifdef HAVE_QOS
#include "hsl_bcm_qos.h"
#endif /* HAVE_QOS */

#include "ptpd.h"
#include "hal_ptp.h"

//#ifdef HAVE_VPWS   /*cai 2011-10 vpws module*/
#include "hsl_bcm_vpws.h"
//#endif /* HAVE_VPWS*/

#include "hal_vpls.h"
#include "hsl_bcm_vpls.h"  /*cyh add */
#include "hal_car.h"
#include "hsl_acl_car.h"



#ifdef HAVE_L2
#ifdef HAVE_VLAN_CLASS
extern int hsl_vlan_classifier_add (struct hal_msg_vlan_classifier_rule *msg);
extern int hsl_vlan_classifier_delete (struct hal_msg_vlan_classifier_rule *msg);
#endif /* HAVE_VLAN_CLASS */
#endif /* HAVE_L2 */

extern int
hsl_bcm_set_ip_access_group
                 (struct hal_msg_ip_set_access_grp *msg);
extern int
hsl_bcm_unset_ip_access_group
                 (struct hal_msg_ip_set_access_grp *msg);

extern int
hsl_bcm_set_ip_access_group_interface
                 (struct hal_msg_ip_set_access_grp_interface *msg);
extern int
hsl_bcm_unset_ip_access_group_interface
                 (struct hal_msg_ip_set_access_grp_interface *msg);


#ifdef HAVE_MPLS
#include "hal_mpls_types.h"
#include "hsl_mpls.h"
#endif /* HAVE_MPLS */

#ifdef HAVE_L2LERN
extern int hsl_bcm_mac_set_access_grp (struct hal_msg_mac_set_access_grp *msg);
extern int hsl_bcm_vlan_set_access_map (struct hal_msg_vlan_set_access_map *msg);
#endif

#ifdef HAVE_TUNNEL
extern int hsl_tunnel_initiator_set (struct hal_msg_tunnel_if *t_ifp);
extern int hsl_tunnel_initiator_clear (struct hal_msg_tunnel_if *t_ifp);
extern int hsl_tunnel_terminator_set (struct hal_msg_tunnel_if *t_ifp);
extern int hsl_tunnel_terminator_clear (struct hal_msg_tunnel_if *t_ifp);
extern int hsl_tunnel_if_add (struct hal_msg_tunnel_add_if *t_ifp);
extern int hsl_tunnel_if_delete (struct hal_msg_tunnel_if *t_ifp);
#endif /* HAVE_TUNNEL */

extern HSL_BOOL hsl_l3_enable_status;
#ifdef HAVE_IPV6
extern HSL_BOOL hsl_ipv6_l3_enable_status;
#endif

/* 
   Encode the interface information.
*/
static void
_hsl_map_if (struct hsl_if *ifp, struct hal_msg_if *msg, u_int32_t cindex)
{
  struct hsl_if *ifp2;
  
  if (ifp == NULL)
    return;

  memset (msg, 0, sizeof (struct hal_msg_if));

  msg->cindex = cindex;

  memcpy (msg->name, ifp->name, HAL_IFNAME_LEN + 1);
  msg->ifindex = ifp->ifindex;

  /* MTU */
  if (CHECK_CINDEX (cindex, HAL_MSG_CINDEX_IF_MTU))
    {
      if (ifp->type == HSL_IF_TYPE_IP)
        msg->mtu = ifp->u.ip.ipv4.mtu;
      else if (ifp->type == HSL_IF_TYPE_L2_ETHERNET)
        msg->mtu = ifp->u.l2_ethernet.mtu;
    }

  /* ARP AGEING TIMEOUT */
  if (CHECK_CINDEX (cindex, HAL_MSG_CINDEX_IF_ARP_AGEING_TIMEOUT))
    {
      if (ifp->type == HSL_IF_TYPE_IP)
        msg->arp_ageing_timeout = ifp->u.ip.arpTimeout;
      else if (ifp->type == HSL_IF_TYPE_L2_ETHERNET)
        msg->arp_ageing_timeout = 0;
    }
  
  /* Flags. */
  if (CHECK_CINDEX (cindex, HAL_MSG_CINDEX_IF_FLAGS))
    msg->flags = ifp->flags;

  /* Metric. */
  if (CHECK_CINDEX (cindex, HAL_MSG_CINDEX_IF_METRIC))
    {
      /* XXX Metric is currently set to 1. */
      msg->metric = 1;
    }

  switch (ifp->type)
    {
    case HSL_IF_TYPE_IP:
      {
        msg->type = HAL_IF_ROUTER_PORT;

        /* Hardware type, address. */
        if (CHECK_CINDEX (cindex, HAL_MSG_CINDEX_IF_HW))
          { 
            msg->hw_addr_len = ETHER_ADDR_LEN;
            memcpy (msg->hw_addr, ifp->u.ip.mac, ETHER_ADDR_LEN);
            msg->hw_type = HSL_IF_TYPE_L2_ETHERNET;
          }

        /* Get the first L2 interface to get the speed etc. */
        ifp2 = hsl_ifmgr_get_first_L2_port (ifp);             

        /* Bandwidth */
        if (CHECK_CINDEX (cindex, HAL_MSG_CINDEX_IF_BANDWIDTH))
          { 
            if (ifp2)
              msg->bandwidth = (ifp2->u.l2_ethernet.speed * 1000) / 8;
            else
              msg->bandwidth = 0;
          }
        /* Duplex */
        if (CHECK_CINDEX (cindex, HAL_MSG_CINDEX_IF_DUPLEX))
          { 
            if (ifp2)
              msg->duplex = ifp2->u.l2_ethernet.duplex;
            else
              msg->duplex = 0;
          }
        /* Autoneg. */ 
        if (CHECK_CINDEX (cindex, HAL_MSG_CINDEX_IF_AUTONEGO))
          {  
            if(ifp2)
              msg->autonego = ifp2->u.l2_ethernet.autonego;
            else  
              msg->autonego = 0;
          }

        if (ifp2)
          HSL_IFMGR_IF_REF_DEC (ifp2);
      }
      break;
    case HSL_IF_TYPE_L2_ETHERNET:
      {
        msg->type = HAL_IF_SWITCH_PORT;

        /* Hardware type, address. */
        if (CHECK_CINDEX (cindex, HAL_MSG_CINDEX_IF_HW))
          {
            msg->hw_addr_len = ETHER_ADDR_LEN;
            memcpy (msg->hw_addr, ifp->u.l2_ethernet.mac, ETHER_ADDR_LEN);
            msg->hw_type = HSL_IF_TYPE_L2_ETHERNET;
          }

        /* Bandwidth. */
        if (CHECK_CINDEX (cindex, HAL_MSG_CINDEX_IF_BANDWIDTH))
          {     
            msg->bandwidth = (ifp->u.l2_ethernet.speed * 1000) / 8;
            SET_CINDEX (msg->cindex, HAL_MSG_CINDEX_IF_BANDWIDTH);
          }
        /* Duplex */
        if (CHECK_CINDEX (cindex, HAL_MSG_CINDEX_IF_DUPLEX))
          { 
            msg->duplex = ifp->u.l2_ethernet.duplex;
            SET_CINDEX (msg->cindex, HAL_MSG_CINDEX_IF_DUPLEX);
          }
        /* Autoneg. */ 
        if (CHECK_CINDEX (cindex, HAL_MSG_CINDEX_IF_AUTONEGO))
          {  
            msg->autonego = ifp->u.l2_ethernet.autonego;
            SET_CINDEX (msg->cindex, HAL_MSG_CINDEX_IF_AUTONEGO);
          }
      }
      break;
    case HSL_IF_TYPE_LOOPBACK:
      msg->type = HAL_IF_ROUTER_PORT;

      if (CHECK_CINDEX (cindex, HAL_MSG_CINDEX_IF_HW))
        {
          msg->hw_type = HSL_IF_TYPE_LOOPBACK;
        }
      break;
    case HSL_IF_TYPE_TUNNEL:
      { 
        msg->type = HAL_IF_ROUTER_PORT;

        /* Hardware type, address. */
        if (CHECK_CINDEX (cindex, HAL_MSG_CINDEX_IF_HW))
          {
            msg->hw_addr_len = ETHER_ADDR_LEN;
            memcpy (msg->hw_addr, ifp->u.ip.mac, ETHER_ADDR_LEN);
            msg->hw_type = HAL_IF_TYPE_TUNNEL;
          }

        /* Get the first L2 interface to get the speed etc. */
        ifp2 = hsl_ifmgr_get_first_L2_port (ifp);
        
        /* Bandwidth */
        if (CHECK_CINDEX (cindex, HAL_MSG_CINDEX_IF_BANDWIDTH))
          {
            if (ifp2)
              msg->bandwidth = (ifp2->u.l2_ethernet.speed * 1000) / 8;
            else
              msg->bandwidth = 0;
          }
      
        /* Duplex */
        if (CHECK_CINDEX (cindex, HAL_MSG_CINDEX_IF_DUPLEX))
          {
            if (ifp2)
              msg->duplex = ifp2->u.l2_ethernet.duplex;
            else
              msg->duplex = 0;
          }
 
        /* Autoneg. */
        if (CHECK_CINDEX (cindex, HAL_MSG_CINDEX_IF_AUTONEGO))
          {
            if (ifp2)
              msg->autonego = ifp2->u.l2_ethernet.autonego;
            else
              msg->autonego = 0;
          }

        if (ifp2)
          HSL_IFMGR_IF_REF_DEC (ifp2);
       }
      break;
    default:
      break;
    }
     
  return;
}
#ifdef HAVE_L3
/* 
   Common function for interface address updates. 
*/
static int
_hsl_msg_addr_event (struct socket *sock, struct hsl_if *ifp, hsl_prefix_t *prefix, int cmd)
{
  u_char tbuf[256], *tpnt;
  int tsize, nbytes = -1;
  struct hal_msg_if_ipv4_addr msg_v4;
#ifdef HAVE_IPV6
  struct hal_msg_if_ipv6_addr msg_v6;
#endif /* HAVE_IPV6 */

  HSL_FN_ENTER(); 

  tpnt = tbuf;
  tsize = 256;

  /* Encode interface. */
  switch(cmd)
    {
    case HAL_MSG_IF_IPV4_DELADDR:   
    case HAL_MSG_IF_IPV4_NEWADDR:
      memset (&msg_v4, 0, sizeof (msg_v4));
      memcpy(msg_v4.name,ifp->name,hsl_strlen(ifp->name));     
      msg_v4.ifindex = ifp->ifindex;
      msg_v4.addr.s_addr = prefix->u.prefix4;
      msg_v4.ipmask = prefix->prefixlen;
      nbytes = hsl_msg_encode_ipv4_addr(&tpnt,(u_int32_t*)&tsize,&msg_v4);
      break; 
#ifdef HAVE_IPV6
    case HAL_MSG_IF_IPV6_DELADDR:   
    case HAL_MSG_IF_IPV6_NEWADDR:
      memset (&msg_v6, 0, sizeof (msg_v6));
      memcpy(msg_v6.name,ifp->name,hsl_strlen(ifp->name));     
      msg_v6.ifindex = ifp->ifindex;
      IPV6_ADDR_COPY(msg_v6.addr.in6_u.u6_addr32,prefix->u.prefix6.word);
      msg_v6.ipmask = prefix->prefixlen;
      nbytes = hsl_msg_encode_ipv6_addr(&tpnt,(u_int32_t*)&tsize,&msg_v6);
      break; 
#endif /* HAVE_IPV6 */
    default: 
      HSL_FN_EXIT(-1);  
    }
  if (nbytes < 0)
    HSL_FN_EXIT(STATUS_ERROR);

  /* Post the message. */
  hsl_sock_post_msg (sock, cmd, 0, 0, (char*)tbuf, nbytes);

  HSL_FN_EXIT(0);
}


/* 
   Address addition message.
*/
int
hsl_msg_ifnewaddr(struct socket *sock, void *param1, void *param2)
{
  int ret;
  int cmd;
  struct hsl_if *ifp = (struct hsl_if *) param1;
  hsl_prefix_t *prefix = (hsl_prefix_t *) param2; 

  HSL_FN_ENTER(); 

  if((!ifp) || !(prefix))
    HSL_FN_EXIT(-1);

  if(AF_INET == prefix->family) 
    cmd = HAL_MSG_IF_IPV4_NEWADDR;
#ifdef HAVE_IPV6
  else if(AF_INET6 == prefix->family) 
    cmd = HAL_MSG_IF_IPV6_NEWADDR;
#endif 
  else
    HSL_FN_EXIT(-1);

  ret = _hsl_msg_addr_event (sock, ifp, prefix ,cmd);
  if (ret < 0)
    goto ERR;

  HSL_FN_EXIT(STATUS_OK);

 ERR:  
  /* Close socket. */
  hsl_sock_release (sock);
  return -1;
}

/* 
   Address deletion message.
*/
int
hsl_msg_ifdeladdr(struct socket *sock, void *param1, void *param2)
{
  int ret;
  int cmd;
  struct hsl_if *ifp = (struct hsl_if *) param1;
  hsl_prefix_t *prefix = (hsl_prefix_t *) param2; 

  HSL_FN_ENTER(); 

  if((!ifp) || !(prefix))
    HSL_FN_EXIT(-1);

  if(AF_INET == prefix->family) 
    cmd = HAL_MSG_IF_IPV4_DELADDR;
#ifdef HAVE_IPV6
  else if(AF_INET6 == prefix->family) 
    cmd = HAL_MSG_IF_IPV6_DELADDR;
#endif 
  else
    HSL_FN_EXIT(-1);


  ret = _hsl_msg_addr_event (sock, ifp, prefix ,cmd);
  if (ret < 0)
    goto ERR;

  HSL_FN_EXIT(STATUS_OK);

 ERR:  
  /* Close socket. */
  hsl_sock_release (sock);
  return -1;
}
#endif /* HAVE_L3 */

#ifdef HAVE_ONMD

int
_hsl_msg_if_efm_event(struct socket *sock,  struct hsl_if *ifp, 
                       u_int32_t frame_errors, int cmd)
{
  u_char tbuf[256], *tpnt = NULL; 
  u_int32_t tsize = 0, nbytes = 0;
  /*struct hal_msg_efm_err_frame_secs_resp msg;*/
  struct hal_msg_efm_err_frame_stat msg;

  tpnt      = tbuf;
  tsize     = 256;  
  msg.err_frames = frame_errors;

  msg.index = (u_int32_t) ifp->ifindex;

  HSL_LOG (HSL_LOG_MSG, HSL_LEVEL_INFO, "Message: %s, ifIndex=%u, frame_errors=%u \n", 
      __FUNCTION__, (u_int32_t) ifp->ifindex, frame_errors);
 
  nbytes = hsl_msg_encode_err_frame_secs_resp(&tpnt, &tsize, &msg);  
  if(nbytes < 0)
    return -1;

  /* Post the message. */
  hsl_sock_post_msg (sock, cmd, 0, 0, (char*)tbuf, nbytes);

  return 0;    
  
}

#endif 

/* 
   Common function for IFNEW, IFDEL or IFFLAGS.
*/
static int
_hsl_msg_if_event (struct socket *sock, struct hsl_if *ifp, u_int32_t cindex, int cmd)
{
  u_char tbuf[256], *tpnt;
  int tsize, nbytes;
  struct hal_msg_if msg;

  /* Interface mapping. */
  _hsl_map_if (ifp, &msg, cindex);

  tpnt = tbuf;
  tsize = 256;

  /* Encode interface. */
  nbytes = hsl_msg_encode_if (&tpnt, (u_int32_t*)&tsize, &msg); 
  if (nbytes < 0)
    return -1;

  /* Post the message. */
  hsl_sock_post_msg (sock, cmd, 0, 0, (char*)tbuf, nbytes);

  return 0;
}

/*
  IFNEW message multicast.
*/
int
hsl_msg_ifnew (struct socket *sock, void *param1, void *unused)
{
  u_int32_t cindex = 0;
  int ret;
  struct hsl_if *ifp = (struct hsl_if *) param1;

  SET_CINDEX (cindex, HAL_MSG_CINDEX_IF_FLAGS);
  SET_CINDEX (cindex, HAL_MSG_CINDEX_IF_METRIC);
  SET_CINDEX (cindex, HAL_MSG_CINDEX_IF_MTU);
  SET_CINDEX (cindex, HAL_MSG_CINDEX_IF_DUPLEX);
  SET_CINDEX (cindex, HAL_MSG_CINDEX_IF_ARP_AGEING_TIMEOUT);
  SET_CINDEX (cindex, HAL_MSG_CINDEX_IF_AUTONEGO);
  SET_CINDEX (cindex, HAL_MSG_CINDEX_IF_HW);
  SET_CINDEX (cindex, HAL_MSG_CINDEX_IF_BANDWIDTH);

  ret = _hsl_msg_if_event (sock, ifp, cindex, HAL_MSG_IF_NEWLINK);
  if (ret < 0)
    goto ERR;

  return 0;

 ERR:  
  /* Close socket. */
  hsl_sock_release (sock);

  return -1;
}
/*
  IFAUTONEGO message multicast.
*/
int
hsl_msg_ifautonego(struct socket *sock, void *param1, void *unused)
{
  u_int32_t cindex = 0;
  int ret;
  struct hsl_if *ifp = (struct hsl_if *) param1;

  HSL_FN_ENTER(); 

  SET_CINDEX (cindex, HAL_MSG_CINDEX_IF_AUTONEGO);
  ret = _hsl_msg_if_event (sock, ifp, cindex, HAL_MSG_IF_UPDATE);
  if (ret < 0)
    goto ERR;

  HSL_FN_EXIT(STATUS_OK);

 ERR:  
  /* Close socket. */
  hsl_sock_release (sock);

  HSL_FN_EXIT(STATUS_ERROR);
}

/*
  IFHWADDR message multicast.
*/
int
hsl_msg_ifhwaddr(struct socket *sock, void *param1, void *unused)
{
  u_int32_t cindex = 0;
  int ret;
  struct hsl_if *ifp = (struct hsl_if *) param1;

  HSL_FN_ENTER(); 

  SET_CINDEX (cindex, HAL_MSG_CINDEX_IF_HW);
  ret = _hsl_msg_if_event (sock, ifp, cindex, HAL_MSG_IF_UPDATE);
  if (ret < 0)
    goto ERR;

  HSL_FN_EXIT(STATUS_OK);

 ERR:  
  /* Close socket. */
  hsl_sock_release (sock);

  HSL_FN_EXIT(STATUS_ERROR);
}

/*
  IFMTU message multicast.
*/
int
hsl_msg_ifmtu(struct socket *sock, void *param1, void *unused)
{
  u_int32_t cindex = 0;
  int ret;
  struct hsl_if *ifp = (struct hsl_if *) param1;

  HSL_FN_ENTER(); 

  SET_CINDEX (cindex, HAL_MSG_CINDEX_IF_MTU);
  ret = _hsl_msg_if_event (sock, ifp, cindex, HAL_MSG_IF_UPDATE);
  if (ret < 0)
    goto ERR;

  HSL_FN_EXIT(STATUS_OK);

 ERR:  
  /* Close socket. */
  hsl_sock_release (sock);

  HSL_FN_EXIT(STATUS_ERROR);
}

/*
  IFDUPLEX message multicast.
*/
int
hsl_msg_ifduplex(struct socket *sock, void *param1, void *unused)
{
  u_int32_t cindex = 0;
  int ret;
  struct hsl_if *ifp = (struct hsl_if *) param1;

  HSL_FN_ENTER(); 

  SET_CINDEX (cindex, HAL_MSG_CINDEX_IF_DUPLEX);
  ret = _hsl_msg_if_event (sock, ifp, cindex, HAL_MSG_IF_UPDATE);
  if (ret < 0)
    goto ERR;

  HSL_FN_EXIT(STATUS_OK);

 ERR:  
  /* Close socket. */
  hsl_sock_release (sock);

  HSL_FN_EXIT(STATUS_ERROR);
}

/* 
   IFDELETE message multicast. 
*/
int
hsl_msg_ifdelete (struct socket *sock, void *param1, void *param2)
{
  u_int32_t cindex = 0;
  int ret;
  struct hsl_if *ifp = (struct hsl_if *) param1;

  ret = _hsl_msg_if_event (sock, ifp, cindex, HAL_MSG_IF_DELLINK);
  if (ret < 0)
    goto ERR;

  return 0;

 ERR:  
  /* Close socket. */
  hsl_sock_release (sock);

  return -1;
}

/*
  IFFLAGS message multicast.
*/
int
hsl_msg_ifflags (struct socket *sock, void *param1, void *param2)
{
  u_int32_t cindex = 0;
  int ret;
  struct hsl_if *ifp = (struct hsl_if *) param1;

  HSL_FN_ENTER ();

  SET_CINDEX (cindex, HAL_MSG_CINDEX_IF_FLAGS);
  SET_CINDEX (cindex, HAL_MSG_CINDEX_IF_BANDWIDTH);
  SET_CINDEX (cindex, HAL_MSG_CINDEX_IF_DUPLEX);

  ret = _hsl_msg_if_event (sock, ifp, cindex, HAL_MSG_IF_UPDATE);
  if (ret < 0)
    goto ERR;

  HSL_FN_EXIT (0);

 ERR:  
  /* Close socket. */
  hsl_sock_release (sock);

  HSL_FN_EXIT (-1);
}

#ifdef HAVE_ONMD
/*
  Send HAL msg about expiry of Frame Period Window  
*/
int
hsl_msg_if_fpwindow_expiry(struct socket *sock, void *param1, void *param2)
{
  int ret;
  struct hsl_if *ifp     = (struct hsl_if *) param1;
  u_int32_t frame_errors = *((u_int32_t*)param2);

  HSL_FN_ENTER();
  ret = _hsl_msg_if_efm_event(sock, ifp, frame_errors, 
                               HAL_MSG_EFM_FRAME_PERIOD_WINDOW_EXPIRY);
  if (ret < 0)
    goto ERR;

  HSL_FN_EXIT(STATUS_OK);

 ERR:  
  /* Close socket. */
  hsl_sock_release (sock);

  HSL_FN_EXIT(STATUS_ERROR);
}

/*  HAL_MSG_EFM_SET_PORT_STATE message*/
int
hsl_msg_recv_efm_set_port_state (struct socket *sock,
                                 struct hal_nlmsghdr *hdr,
                                 char *msgbuf)
{   
  struct hal_msg_efm_port_state *msg;
  int ret;

  msg = (struct hal_msg_efm_port_state *) msgbuf;

  ret = hsl_bcm_efm_set_port_state (msg->index, msg->local_par_action,
                                                msg->local_mux_action);

  HSL_MSG_PROCESS_RETURN (sock, hdr, ret);

  return ret;
}
#endif /*HAVE_ONMD*/


/*
  HAL initialization.
*/
int
hsl_msg_recv_init (struct socket *sock, struct hal_nlmsghdr *hdr, char *msgbuf)
{
  HSL_LOG (HSL_LOG_MSG, HSL_LEVEL_INFO, "Message: HAL initialization\n");
  HSL_MSG_PROCESS_RETURN (sock, hdr, 0);
  return 0;
}

/* 
   HAL deinitialization.
*/
int
hsl_msg_recv_deinit (struct socket *sock, struct hal_nlmsghdr *hdr, char *msgbuf)
{
  HSL_LOG (HSL_LOG_MSG, HSL_LEVEL_INFO, "Message: HAL deinitialization\n");
  HSL_MSG_PROCESS_RETURN (sock, hdr, 0);
  return 0;
}


/* HAL_MSG_IF_CLEANUP_DONE message. */
int
hsl_msg_recv_if_delete_done (struct socket *sock, struct hal_nlmsghdr *hdr, char *msgbuf)
{
  u_char *pnt;
  u_int32_t size;
  struct hal_msg_if msg;
  int ret;

  HSL_FN_ENTER();
  HSL_LOG (HSL_LOG_MSG, HSL_LEVEL_INFO, "Message: HAL Interfece Cleanup done\n");

  pnt = (u_char*)msgbuf;
  size = hdr->nlmsg_len - HAL_NLMSG_ALIGN(HAL_NLMSGHDR_SIZE);

  ret = hsl_msg_decode_if (&pnt, &size, &msg);
  if (ret < 0)
    goto CLEANUP;

  ret = hsl_ifmgr_clean_up_complete(msg.ifindex);

  HSL_MSG_PROCESS_RETURN (sock, hdr, ret);
  HSL_FN_EXIT(ret);

 CLEANUP:
  /* Close socket. */
  hsl_sock_release (sock);

  HSL_FN_EXIT (-1);
}

#ifdef HAVE_L3

/*
  HAL_MSG_IF_L3_INIT message.
*/
int
hsl_msg_recv_if_init_l3(struct socket *sock, struct hal_nlmsghdr *hdr, char *msgbuf)
{
  struct hsl_bcm_if   *bcm_ifp;                   /* Broadcom interface info.    */
  struct hsl_if       **ifp_arr;                  /* Array of all L2 interfaces. */
  struct hsl_if       *ifp;                       /* Inteface information.       */
  struct hsl_if       *ifpp;                      /* New Inteface information.   */
  int ret;                                        /* General operation status.   */
  u_int16_t index;                                /* Index for iteration.        */
  u_int16_t count = 0;                            /* Interface size.             */  
  u_int8_t policy;
 
  HSL_FN_ENTER();
  
  HSL_LOG (HSL_LOG_MSG, HSL_LEVEL_INFO, "Message: Set interface init l3 mode\n"); 

  /* Get current interface manager policy. */
  policy = hsl_ifmgr_get_policy ();
  if (policy == HSL_IFMGR_IF_INIT_POLICY_L3)
    {
      HSL_MSG_PROCESS_RETURN (sock, hdr, 0);
      HSL_FN_EXIT (0);
    }
  
  /* Lock interface manager. */
  HSL_IFMGR_LOCK;
  
  /* Create a snapshot of all L2 interfaces (ports). */
  ret = hsl_ifmgr_get_L2_array(&ifp_arr, &count);
  if (ret < 0 )
    {
      HSL_LOG (HSL_LOG_MSG, HSL_LEVEL_INFO, "Failed to read l2 interface array.\n");
      HSL_IFMGR_UNLOCK;
      HSL_MSG_PROCESS_RETURN (sock, hdr, -1);
      HSL_FN_EXIT(-1);
    }

  /* Create L3 interface for every L2 saved in the snapshot. */
  for (index = 0 ;index < count; index++)
    {
      ifp  = ifp_arr[index];

      HSL_LOG (HSL_LOG_IFMGR, HSL_LEVEL_INFO, "Creating L3 interface %s(%d)\n", ifp->name, ifp->ifindex);
      bcm_ifp = (struct hsl_bcm_if *) ifp->system_info;
      if (! bcm_ifp)
        {
          HSL_LOG (HSL_LOG_IFMGR, HSL_LEVEL_ERROR, "Interface %s(%d) doesn't have a corresponding broadcom interface structure\n", ifp->name, ifp->ifindex);
          continue;
        }

      /* Enable L3 routing. */
      bcmx_port_l3_enable_set (bcm_ifp->u.l2.lport, 1);

      /* Create layer 3 interface. */
      ret = hsl_ifmgr_set_router_port (ifp, NULL, &ifpp, HSL_TRUE);
      if (ret < 0 )
        {
          HSL_LOG (HSL_LOG_IFMGR, HSL_LEVEL_ERROR, "  Interface %s(%d) \n", ifp->name, ifp->ifindex);
          continue;
        }

      /* Set interface flags. */
      if (ifpp->flags & IFF_UP)
        hsl_bcm_if_l3_flags_set (ifpp, IFF_UP);
    }

  oss_free(ifp_arr,OSS_MEM_HEAP);

  HSL_IFMGR_UNLOCK;

  /* Set policy. */
  hsl_ifmgr_set_policy (HSL_IFMGR_IF_INIT_POLICY_L3);

  HSL_MSG_PROCESS_RETURN (sock, hdr, 0);

  HSL_FN_EXIT(STATUS_OK);
}

/*
  HAL_MSG_IF_L3_DEINIT message.
*/
int
hsl_msg_recv_if_deinit_l3(struct socket *sock,
                         struct hal_nlmsghdr *hdr, char *msgbuf)
{
  struct hsl_bcm_if   *bcm_ifp;               /* Broadcom interface info.    */
  struct hsl_if       **ifp_arr;              /* Array of all L2 interfaces. */
  struct hsl_if       *ifp;                   /* Inteface information.       */
  struct hsl_if       *ifpp;                  /* Inteface information.       */
  int ret;                                    /* General operation status.   */
  u_int16_t index;                            /* Index for iteration.        */
  u_int16_t count = 0;                        /* Interface size.             */
  u_int8_t policy;
 
  HSL_FN_ENTER();
  
  HSL_LOG (HSL_LOG_MSG, HSL_LEVEL_INFO, "Message: deinit l3 mode\n"); 

  /* Get current interface manager policy. */
  policy = hsl_ifmgr_get_policy ();

  if (policy != HSL_IFMGR_IF_INIT_POLICY_L3)
    {
      HSL_MSG_PROCESS_RETURN (sock, hdr, 0);
      HSL_FN_EXIT (0);
    }
  
  /* Lock interface manager. */
  HSL_IFMGR_LOCK;
  
  /* Create a snapshot of all interfaces (ports).
   * The below function name is a misnomer
   */

  ret = hsl_ifmgr_get_L2_array (&ifp_arr, &count);

  if (ret < 0 )
    {
      HSL_LOG (HSL_LOG_MSG, HSL_LEVEL_INFO, "Failed to read l2 interface"
                                             " array.\n");
      HSL_IFMGR_UNLOCK;
      HSL_MSG_PROCESS_RETURN (sock, hdr, -1);
      HSL_FN_EXIT(-1);
    }

  /* Create L3 interface for every L2 saved in the snapshot. */
  for (index = 0 ;index < count; index++)
    {
      ifp  = ifp_arr[index];

      bcm_ifp = (struct hsl_bcm_if *) ifp->system_info;

      if (! bcm_ifp)
        {
          HSL_LOG (HSL_LOG_IFMGR, HSL_LEVEL_ERROR, "  Interface %s(%d) doesn't "
                   "have a corresponding broadcom interface structure\n",
                   ifp->name, ifp->ifindex);
          continue;
        }

      /* Disable L3 routing. */
      bcmx_port_l3_enable_set (bcm_ifp->u.l2.lport, 0);

      /* Reset to L2 mode. The check whether this is a L3 port
       * is in the function hsl_ifmgr_set_switch_port
       */
      ret = hsl_ifmgr_set_switch_port (ifp, &ifpp, HSL_FALSE);

      if (ret < 0 )
        {
          HSL_LOG (HSL_LOG_IFMGR, HSL_LEVEL_ERROR, "  Interface %s(%d) \n",
                   ifp->name, ifp->ifindex);
          continue;
        }
    }

  oss_free(ifp_arr,OSS_MEM_HEAP);

  HSL_IFMGR_UNLOCK;

  /* Set policy. */
  hsl_ifmgr_set_policy (HSL_IFMGR_IF_INIT_POLICY_L2);

  HSL_MSG_PROCESS_RETURN (sock, hdr, 0);

  HSL_FN_EXIT(STATUS_OK);

}

int 
hsl_msg_recv_if_l3_forwarding (struct socket *sock,
                             struct hal_nlmsghdr *hdr, char *msgbuf)
{
  struct hsl_if       *ifp = NULL;       /* Inteface information.       */
  struct hsl_avl_node *node = NULL;
  int ret = 0;                           /* General operation status.   */
  u_int8_t policy;
  struct hal_msg_if_l3_status *msg = (struct hal_msg_if_l3_status *)msgbuf;
  int l3_status = msg->status;
  
  HSL_FN_ENTER();
  
  HSL_LOG (HSL_LOG_MSG, HSL_LEVEL_INFO, 
      "Message: Enable/Disable L3 forwarding \n"); 
  
  policy = hsl_ifmgr_get_policy ();
  if (policy != HSL_IFMGR_IF_INIT_POLICY_L3)
    {
      HSL_MSG_PROCESS_RETURN (sock, hdr, 0);
      HSL_FN_EXIT (0);
    }
  /* Lock interface manager. */
  HSL_IFMGR_LOCK;
  
  HSL_IFMGR_L3_INTF_LOOP(HSL_IFMGR_TREE, ifp, node)
    {
        ret = hsl_bcm_if_add_or_remove_l3_table_entries (ifp, l3_status);
        if (ret != 0)
          {
            HSL_LOG (HSL_LOG_IFMGR, HSL_LEVEL_ERROR, 
                "L3 table updation on %s(%d) Failed \r\n",
                ifp->name, ifp->ifindex);
          } /*No rollback is done*/
    }
  
  HSL_IFMGR_UNLOCK;

  hsl_l3_enable_status = l3_status;
  
  HSL_MSG_PROCESS_RETURN (sock, hdr, 0);
  
  HSL_FN_EXIT(ret);
  
}

#ifdef HAVE_IPV6
int 
hsl_msg_recv_if_ipv6_l3_forwarding (struct socket *sock,
                             struct hal_nlmsghdr *hdr, char *msgbuf)
{
  struct hsl_if       *ifp = NULL;          /* Inteface information.       */
  struct hsl_avl_node *node = NULL;
  int ret = 0;                              /* General operation status.   */
  u_int8_t policy;
  struct hal_msg_if_l3_status *msg = (struct hal_msg_if_l3_status *)msgbuf;
  int l3_status = msg->status;
  
  HSL_FN_ENTER();
  
  HSL_LOG (HSL_LOG_MSG, HSL_LEVEL_INFO, 
      "Message: Enable/Disable IPv6 L3 forwarding \n"); 
  
  policy = hsl_ifmgr_get_policy ();
  if (policy != HSL_IFMGR_IF_INIT_POLICY_L3)
    {
      HSL_MSG_PROCESS_RETURN (sock, hdr, 0);
      HSL_FN_EXIT (0);
    }
  /* Lock interface manager. */
  HSL_IFMGR_LOCK;
  
  HSL_IFMGR_L3_INTF_LOOP(HSL_IFMGR_TREE, ifp, node)
    {
      ret = hsl_bcm_if_add_or_remove_ipv6_l3_table_entries (ifp, l3_status);
      if (ret != 0)
          {
            HSL_LOG (HSL_LOG_IFMGR, HSL_LEVEL_ERROR, 
                "%s: %d, IPv6: L3 table updation on %s(%d) Failed \r\n",
                __FILE__, __LINE__, ifp->name, ifp->ifindex);
          } /*No rollback is done*/
    }
  
  HSL_IFMGR_UNLOCK;

  hsl_ipv6_l3_enable_status = l3_status;
  
  HSL_MSG_PROCESS_RETURN (sock, hdr, 0);
  
  HSL_FN_EXIT(ret);
  
}
#endif
#ifdef HAVE_VRRP1
/* VRRP VIP Address Comparison for AVL tree. */
int
hsl_vrrp_vip_cmp (void *param1, void *param2)
  {
    struct hsl_vip_addr_struc *vs1 = (struct hsl_vip_addr_struc *) param1;
    struct hsl_vip_addr_struc *vs2 = (struct hsl_vip_addr_struc *) param2;

    return IPV4_ADDR_CMP(&vs1->vip, &vs2->vip);
  }

  /* Function to add VRRP VIP to AVL tree. */
int
hsl_vrrp_add_vip (struct hsl_if *ifp, int vrrp_vrid,
                  struct hal_in4_addr *vip_v4)
  {
    struct hsl_vip_addr_struc   *vs;

    vs = oss_malloc (sizeof (struct hsl_vip_addr_struc), OSS_MEM_HEAP);
    memcpy (&vs->vip, vip_v4, sizeof(struct hal_in4_addr));
    vs->vrid = vrrp_vrid;
    vs->pkt_cnt = 1;
    HSL_LOG (HSL_LOG_MSG, HSL_LEVEL_INFO, "Message: VRRP VIP ADD RECEIVED\n");

    /* If head pointer is still null, set it */
    if (ifp->vrrp_vip_tree == NULL)
      hsl_avl_create (&ifp->vrrp_vip_tree, 0, hsl_vrrp_vip_cmp);

    /* Add VIP to tree. */
    hsl_avl_insert (ifp->vrrp_vip_tree, (void *)vs);

    return 0;
  }

/* Function to del VRRP VIP from AVL tree. */
void
hsl_vrrp_del_vip (struct hsl_if *ifp, int vrrp_vrid,
                  struct hal_in4_addr *vip_v4)
{
  struct hsl_vip_addr_struc vs;
  struct hsl_vip_addr_struc *stored_vs;
  struct hsl_avl_node *node = NULL;

  HSL_LOG (HSL_LOG_MSG, HSL_LEVEL_INFO, "Message: VRRP VIP DEL RECEIVED\n");

  vs.vip.s_addr = vip_v4->s_addr;

  /* Delete VIP from Tree. */
  node = hsl_avl_lookup (ifp->vrrp_vip_tree, &vs);

  if (node)
    {
      stored_vs = node->info;
      hsl_avl_delete (ifp->vrrp_vip_tree, (void *) &vs);

      /* Free the data structure */
      oss_free (stored_vs, OSS_MEM_HEAP);
    }

    /* Reset head & tail pointers if necessary */
    if (hsl_avl_get_tree_size(ifp->vrrp_vip_tree) == 0)
      hsl_avl_tree_free (&ifp->vrrp_vip_tree, NULL);
}

#ifdef HAVE_IPV6
/* VIPV6 Address Comparison for AVL tree. */
int
hsl_vrrp_vip6_cmp (void *param1, void *param2)
{
  struct hsl_vip6_addr_struc *vs1 = (struct hsl_vip6_addr_struc *) param1;
  struct hsl_vip6_addr_struc *vs2 = (struct hsl_vip6_addr_struc *) param2;

  return IPV6_ADDR_CMP(&vs1->vip6, &vs2->vip6);
}
/* Function to add VRRP VIPV6 to AVL tree. */
int
hsl_vrrp_add_vip6 (struct hsl_if *ifp, int vrrp_vrid,
                   struct hal_in6_addr *vip_v6)
{
  struct hsl_vip6_addr_struc *vs;

  vs = oss_malloc (sizeof (struct hsl_vip6_addr_struc), OSS_MEM_HEAP);
  memcpy (&vs->vip6, vip_v6, sizeof(struct hal_in6_addr));
  vs->vrid = vrrp_vrid;

  HSL_LOG (HSL_LOG_MSG, HSL_LEVEL_INFO, "Message: VRRP VIPv6 ADD RECEIVED\n");

  /* If head pointer is still null, set it */
  if (ifp->vrrp_vip6_tree == NULL)
    hsl_avl_create (&ifp->vrrp_vip6_tree, 0, hsl_vrrp_vip6_cmp);

  /* Add VIP to tree. */
  hsl_avl_insert (ifp->vrrp_vip6_tree, (void *)vs);

  return 0;
}

/* Function to del VRRP VIP from AVL tree. */
void
hsl_vrrp_del_vip6 (struct hsl_if *ifp, int vrrp_vrid,
                   struct hal_in6_addr *vip_v6)
{
  struct hsl_vip6_addr_struc vs;
  struct hsl_vip6_addr_struc *stored_vs;
  struct hsl_avl_node *node = NULL;

  HSL_LOG (HSL_LOG_MSG, HSL_LEVEL_INFO, "Message: VRRP VIPv6 DEL RECEIVED\n");

  IPV6_ADDR_COPY (&vs.vip6, vip_v6);

  /* Delete VIP from Tree. */
  node = hsl_avl_lookup (ifp->vrrp_vip6_tree, &vs);
  if (node)
    {
      stored_vs = node->info;
      hsl_avl_delete (ifp->vrrp_vip6_tree, (void *) &vs);

      /* Free the data structure */
      oss_free (stored_vs, OSS_MEM_HEAP);
    }

    /* Reset head & tail pointers if necessary */
    if (hsl_avl_get_tree_size(ifp->vrrp_vip6_tree) == 0)
      hsl_avl_tree_free (&ifp->vrrp_vip6_tree, NULL);
}
#endif /* HAVE_IPV6*/

/* Function to process VRRP messages received from NSM. */
int
hsl_msg_recv_vrrp_update (struct socket *sock,
                          struct hal_nlmsghdr *hdr, char *msgbuf)
{
  struct hsl_if             *ifp = NULL;
  struct hal_msg_vrrp_msg   *vrrp_msg = NULL;

  int ret;

  HSL_FN_ENTER();

  vrrp_msg = (struct hal_msg_vrrp_msg *)msgbuf;

  HSL_LOG (HSL_LOG_MSG, HSL_LEVEL_INFO, "Message: VRRP UPDATE RECEIVED\n");

  /* Get current interface manager policy. */
  ifp = hsl_ifmgr_lookup_by_index (vrrp_msg->ifindex);

  if (ifp == NULL)
    {
      HSL_MSG_PROCESS_RETURN (sock, hdr, 0);
      HSL_FN_EXIT (0);
    }

  switch (vrrp_msg->msg_type)
    {
      case HSL_VRRP_VIP_ADD:
        if (vrrp_msg->af_type == AF_INET)
          hsl_vrrp_add_vip (ifp, vrrp_msg->vrid, &vrrp_msg->vip_v4);
#ifdef HAVE_IPV6
        else if (vrrp_msg->af_type == AF_INET6)
          hsl_vrrp_add_vip6 (ifp, vrrp_msg->vrid, &vrrp_msg->vip_v6);
#endif
        break;
      case HSL_VRRP_VIP_DEL:
        if (vrrp_msg->af_type == AF_INET)
          hsl_vrrp_del_vip (ifp, vrrp_msg->vrid, &vrrp_msg->vip_v4);
#ifdef HAVE_IPV6
        else if (vrrp_msg->af_type == AF_INET6)
          hsl_vrrp_del_vip6 (ifp, vrrp_msg->vrid, &vrrp_msg->vip_v6);
#endif
        break;

      default:
        HSL_IFMGR_IF_REF_DEC(ifp);
        HSL_MSG_PROCESS_RETURN (sock, hdr, 0);
        HSL_FN_EXIT (0);
    }
   HSL_IFMGR_IF_REF_DEC(ifp);
   HSL_MSG_PROCESS_RETURN (sock, hdr, 0);
   HSL_FN_EXIT(STATUS_OK);
 }
#endif /* HAVE_VRRP */

#endif /* HAVE_L3 */
/*
  HAL_MSG_IF_GETLINK message. 
*/
int
hsl_msg_recv_if_getlink (struct socket *sock, struct hal_nlmsghdr *hdr, char *msgbuf)
{
  struct hsl_avl_node *node;
  struct hsl_if *ifp;
  int size, totsz, nbytes;
  u_char buf[512], *pnt;
  struct hal_msg_if msg;
  u_int32_t cindex = 0;
  struct hal_nlmsghdr *nlh;
  hsl_prefix_list_t *ucaddr;

  HSL_LOG (HSL_LOG_MSG, HSL_LEVEL_INFO, "Message: HAL Get interface list\n");

  HSL_IFMGR_LOCK;

  for (node = hsl_avl_top (HSL_IFMGR_TREE); node; node = hsl_avl_next(node))
    {
      ifp = HSL_AVL_NODE_INFO (node);
      if (! ifp)
        {
          continue;
        }

      /* If directly mapped interface, skip this interface. */
      if (CHECK_FLAG (ifp->if_flags, HSL_IFMGR_IF_DIRECTLY_MAPPED))
        continue;

      cindex = 0;
      SET_CINDEX (cindex, HAL_MSG_CINDEX_IF_FLAGS);
      SET_CINDEX (cindex, HAL_MSG_CINDEX_IF_METRIC);
      SET_CINDEX (cindex, HAL_MSG_CINDEX_IF_MTU);
      SET_CINDEX (cindex, HAL_MSG_CINDEX_IF_DUPLEX);
      SET_CINDEX (cindex, HAL_MSG_CINDEX_IF_ARP_AGEING_TIMEOUT);
      SET_CINDEX (cindex, HAL_MSG_CINDEX_IF_AUTONEGO);
      SET_CINDEX (cindex, HAL_MSG_CINDEX_IF_HW);
      SET_CINDEX (cindex, HAL_MSG_CINDEX_IF_BANDWIDTH);

      /* Interface mapping. */
      _hsl_map_if (ifp, &msg, cindex);

      pnt = buf;
      totsz = 0;
      size = 512;
      nlh = (struct hal_nlmsghdr *) pnt;
      pnt += HAL_NLMSGHDR_SIZE;

      /* Encode interface. */
      nbytes = hsl_msg_encode_if (&pnt, (u_int32_t*)&size, &msg); 
      if (nbytes < 0)
        {
          goto UNLOCK;
        }
   
      /* Set header. */    
      nlh->nlmsg_len = HAL_NLMSG_LENGTH (nbytes);
      nlh->nlmsg_type = HAL_MSG_IF_NEWLINK;
      nlh->nlmsg_flags = HAL_NLM_F_MULTI;
      totsz += HAL_NLMSG_ALIGN(nlh->nlmsg_len);
      
      /* Post the message. */
      hsl_sock_post_buffer (sock, (char*)buf, totsz);

#ifdef HAVE_L3
      if (ifp->type == HSL_IF_TYPE_L2_ETHERNET)
        continue;

      /* Now send address sync information */
      ucaddr = ifp->u.ip.ipv4.ucAddr;
      while (ucaddr)
        {
          hsl_ifmgr_send_notification (HSL_IF_EVENT_IFNEWADDR, ifp, 
              &ucaddr->prefix);
          ucaddr = ucaddr->next;
        }
#ifdef HAVE_IPV6
      ucaddr = ifp->u.ip.ipv6.ucAddr;
      while (ucaddr)
        {
          hsl_ifmgr_send_notification (HSL_IF_EVENT_IFNEWADDR, ifp, 
              &ucaddr->prefix);
          ucaddr = ucaddr->next;
        }
#endif /* HAVE_IPV6 */
#endif /* HAVE_L3 */
    }

  HSL_IFMGR_UNLOCK;

  /* End the message with HAL_NLMSG_DONE. */
  nlh = (struct hal_nlmsghdr *) buf;
  nlh->nlmsg_len = HAL_NLMSG_LENGTH(0);
  nlh->nlmsg_type = HAL_NLMSG_DONE;
  totsz =  HAL_NLMSG_ALIGN(nlh->nlmsg_len);
  
  /* Post the message. */
  hsl_sock_post_buffer (sock, (char*)buf, totsz);

  return 0;
 UNLOCK:
  HSL_IFMGR_UNLOCK;
  return 0;
}


/*
  Generic function to process interface get messages.
*/
static int
_hsl_msg_process_if_get (struct socket *sock, struct hal_nlmsghdr *hdr, char *msgbuf, u_int32_t cindex)
{
  u_char tbuf[256];
  u_char *pnt, *tpnt;
  u_int32_t size, tsize;
  struct hal_msg_if msg;
  int ret, nbytes;
  struct hsl_if *ifp;

  /* Get requests should have ACK flag set. */
  if (hdr->nlmsg_flags & HAL_NLM_F_ACK)
    {
      pnt = (u_char*)msgbuf;
      size = hdr->nlmsg_len - HAL_NLMSG_ALIGN(HAL_NLMSGHDR_SIZE);

      ret = hsl_msg_decode_if (&pnt, &size, &msg);
      if (ret < 0)
        {
          HSL_LOG (HSL_LOG_MSG, HSL_LEVEL_ERROR, "Interface message decode failed\n");
          goto ERR;
        }

      /* Lookup interface. */
      ifp = hsl_ifmgr_lookup_by_index (msg.ifindex);
      if (! ifp)
        {
          HSL_LOG (HSL_LOG_MSG, HSL_LEVEL_INFO, "Interface %s(%d) not found in database\n", msg.name, msg.ifindex);
          /* Send error. */
          hsl_sock_post_ack (sock, hdr, 0, -1);
          
          return -1;
        }
    
      /* Interface mapping. */
      _hsl_map_if (ifp, &msg, cindex);
      HSL_IFMGR_IF_REF_DEC (ifp);

      tpnt = tbuf;
      tsize = 256;

      /* Encode interface. */
      nbytes = hsl_msg_encode_if (&tpnt, &tsize, &msg); 
      if (nbytes < 0)
        {
          HSL_LOG (HSL_LOG_MSG, HSL_LEVEL_ERROR, "Interface message encode failed\n");
          goto ERR;
        }

      /* Post the message. */
      hsl_sock_post_msg (sock, hdr->nlmsg_type, hdr->nlmsg_seq, 0, (char*)tbuf, nbytes);
    }

  return 0;

 ERR:
  /* Close the socket. */
  hsl_sock_release (sock);

  return -1;
}

/* HAL_MSG_IF_GET_METRIC message. */
int
hsl_msg_recv_if_get_metric (struct socket *sock, struct hal_nlmsghdr *hdr, char *msgbuf)
{
  int ret;
  u_int32_t cindex = 0;

  HSL_LOG (HSL_LOG_MSG, HSL_LEVEL_INFO, "Message: HAL Get interface metric\n");

  SET_CINDEX (cindex, HAL_MSG_CINDEX_IF_METRIC);
  ret = _hsl_msg_process_if_get (sock, hdr, msgbuf, cindex);

  return ret;
}

/* HAL_MSG_IF_GET_MTU message. */
int
hsl_msg_recv_if_get_mtu (struct socket *sock, struct hal_nlmsghdr *hdr, char *msgbuf)
{
  int ret;
  u_int32_t cindex = 0;

  HSL_LOG (HSL_LOG_MSG, HSL_LEVEL_INFO, "Message: HAL Get interface MTU\n");

  SET_CINDEX (cindex, HAL_MSG_CINDEX_IF_MTU);
  ret = _hsl_msg_process_if_get (sock, hdr, msgbuf, cindex);

  return ret;
}

/* HAL_MSG_IF_SET_MTU message. */
int
hsl_msg_recv_if_set_mtu (struct socket *sock, struct hal_nlmsghdr *hdr, char *msgbuf)
{
  u_char *pnt;
  u_int32_t size;
  struct hal_msg_if msg;
  int ret;

  HSL_LOG (HSL_LOG_MSG, HSL_LEVEL_INFO, "Message: HAL Set interface list\n");

  /* Get requests should have ACK flag set. */

  pnt = (u_char*)msgbuf;
  size = hdr->nlmsg_len - HAL_NLMSG_ALIGN(HAL_NLMSGHDR_SIZE);
      
  ret = hsl_msg_decode_if (&pnt, &size, &msg);
  if (ret < 0)
    goto CLEANUP;

  ret = hsl_ifmgr_set_mtu (msg.ifindex, msg.mtu, HSL_FALSE);
  HSL_MSG_PROCESS_RETURN (sock, hdr, ret);
  return ret;

 CLEANUP:
  /* Close socket. */
  hsl_sock_release (sock);

  return -1;
}

#ifdef HAVE_L3
/* HAL_MSG_IF_GET_ARP_AGEING_TIMEOUT message. */
int
hsl_msg_recv_if_get_arp_ageing_timeout (struct socket *sock, struct hal_nlmsghdr *hdr, char *msgbuf)
{
  int ret;
  u_int32_t cindex = 0;

  HSL_LOG (HSL_LOG_MSG, HSL_LEVEL_INFO, "Message: HAL Get interface ARP AGEING TIMEOUT\n");

  SET_CINDEX (cindex, HAL_MSG_CINDEX_IF_ARP_AGEING_TIMEOUT);
  ret = _hsl_msg_process_if_get (sock, hdr, msgbuf, cindex);

  return ret;
}

/* HAL_MSG_IF_SET_ARP_AGEING_TIMEOUT message. */
int
hsl_msg_recv_if_set_arp_ageing_timeout (struct socket *sock, struct hal_nlmsghdr *hdr, char *msgbuf)
{
  u_char *pnt;
  u_int32_t size;
  struct hal_msg_if msg;
  int ret;

  HSL_LOG (HSL_LOG_MSG, HSL_LEVEL_INFO, "Message: HAL Set interface ARP AGEING TIMEOUT\n");

  /* Get requests should have ACK flag set. */

  pnt = (u_char*)msgbuf;
  size = hdr->nlmsg_len - HAL_NLMSG_ALIGN(HAL_NLMSGHDR_SIZE);

  ret = hsl_msg_decode_if (&pnt, &size, &msg);
  if (ret < 0)
    goto CLEANUP;

  ret = hsl_ifmgr_set_arp_ageing_timeout (msg.ifindex, msg.arp_ageing_timeout);
  HSL_MSG_PROCESS_RETURN (sock, hdr, ret);
  return ret;

 CLEANUP:
  /* Close socket. */
  hsl_sock_release (sock);

  return -1;
}
#endif /* HAVE_L3 */

/* HAL_MSG_IF_GET_DUPLEX message. */
int
hsl_msg_recv_if_get_duplex (struct socket *sock, struct hal_nlmsghdr *hdr, char *msgbuf)
{
  int ret;
  u_int32_t cindex = 0;

  HSL_LOG (HSL_LOG_MSG, HSL_LEVEL_INFO, "Message: HAL Get interface DUPLEX\n");

  SET_CINDEX (cindex, HAL_MSG_CINDEX_IF_DUPLEX);
  ret = _hsl_msg_process_if_get (sock, hdr, msgbuf, cindex);

  return ret;
}

/* HAL_MSG_IF_SET_DUPLEX message. */
int
hsl_msg_recv_if_set_duplex (struct socket *sock, struct hal_nlmsghdr *hdr, char *msgbuf)
{
  u_char *pnt;
  u_int32_t size;
  struct hal_msg_if msg;
  int ret;

  HSL_LOG (HSL_LOG_MSG, HSL_LEVEL_INFO, "Message: HAL Set interface DUPLEX\n");

  /* Get requests should have ACK flag set. */

  pnt = (u_char*)msgbuf;
  size = hdr->nlmsg_len - HAL_NLMSG_ALIGN(HAL_NLMSGHDR_SIZE);

  ret = hsl_msg_decode_if (&pnt, &size, &msg);
  if (ret < 0)
    goto CLEANUP;

  ret = hsl_ifmgr_set_duplex (msg.ifindex, msg.duplex, HSL_FALSE);
  HSL_MSG_PROCESS_RETURN (sock, hdr, ret);
  return ret;

 CLEANUP:
  /* Close socket. */
  hsl_sock_release (sock);

  return -1;
}

/* HAL_MSG_IF_GET_HWADDR message. */
int
hsl_msg_recv_if_get_hwaddr (struct socket *sock, struct hal_nlmsghdr *hdr, char *msgbuf)
{
  int ret;
  u_int32_t cindex = 0;

  HSL_LOG (HSL_LOG_MSG, HSL_LEVEL_INFO, "Message: HAL Get interface hardware address\n");

  SET_CINDEX (cindex, HAL_MSG_CINDEX_IF_HW);
  ret = _hsl_msg_process_if_get (sock, hdr, msgbuf, cindex);

  return ret;
}


/* HAL_MSG_IF_SET_AUTONEGO message. */
int
hsl_msg_recv_if_set_autonego (struct socket *sock, struct hal_nlmsghdr *hdr, char *msgbuf)
{
  u_char *pnt;
  u_int32_t size;
  struct hal_msg_if msg;
  int ret;

  HSL_LOG (HSL_LOG_MSG, HSL_LEVEL_INFO, "Message: HAL Set interface AUTONEGO\n");

  /* Get requests should have ACK flag set. */

  pnt = (u_char*)msgbuf;
  size = hdr->nlmsg_len - HAL_NLMSG_ALIGN(HAL_NLMSGHDR_SIZE);

  ret = hsl_msg_decode_if (&pnt, &size, &msg);
  if (ret < 0)
    goto CLEANUP;

  ret = hsl_ifmgr_set_autonego (msg.ifindex, msg.autonego,HSL_FALSE);
  HSL_MSG_PROCESS_RETURN (sock, hdr, ret);
  return ret;

 CLEANUP:
  /* Close socket. */
  hsl_sock_release (sock);

  return -1;
}

/* HAL_MSG_IF_SET_HWADDR message. */
int
hsl_msg_recv_if_set_hwaddr (struct socket *sock, struct hal_nlmsghdr *hdr, char *msgbuf)
{
  u_char *pnt;
  u_int32_t size;
  struct hal_msg_if msg;
  int ret;

  HSL_LOG (HSL_LOG_MSG, HSL_LEVEL_INFO, "Message: HAL Set interface hardware address\n");

  /* Get requests should have ACK flag set. */

  pnt = (u_char*)msgbuf;
  size = hdr->nlmsg_len - HAL_NLMSG_ALIGN(HAL_NLMSGHDR_SIZE);
      
  ret = hsl_msg_decode_if (&pnt, &size, &msg);
  if (ret < 0)
    goto CLEANUP;

  ret = hsl_ifmgr_set_hwaddr (msg.ifindex, msg.hw_addr_len, msg.hw_addr, HSL_FALSE);
  HSL_MSG_PROCESS_RETURN (sock, hdr, ret);
  return ret;

 CLEANUP:
  /* Close socket. */
  hsl_sock_release (sock);

  return -1;
}

#ifdef HAVE_L3
static int
_hsl_msg_recv_if_sec_hwaddrs (struct hal_nlmsghdr *hdr, char *msgbuf,
                              int (*func) (hsl_ifIndex_t ifindex, int hwaddrlen, int num, u_char **hwaddr, HSL_BOOL send_notification))
{
  struct hal_msg_if msg;
  u_int32_t size;
  u_char *pnt;
  int ret;
  int i;

  ret = 0;
  
  pnt = (u_char*)msgbuf;
  size = hdr->nlmsg_len - HAL_NLMSG_ALIGN(HAL_NLMSGHDR_SIZE);

  /* Decode interface message. */     
  ret = hsl_msg_decode_if (&pnt, &size, &msg);
  if (ret < 0)
    goto CLEANUP;

  ret = (*func) (msg.ifindex, HSL_ETHER_ADDRLEN, msg.nAddrs, msg.addresses, HSL_FALSE);
  if (ret < 0)
    {
      HSL_LOG (HSL_LOG_MSG, HSL_LEVEL_ERROR, "Error setting secondary hardware addresses\n");
    }

 CLEANUP:
  if (msg.addresses)
    {
      for (i = 0; i < msg.nAddrs; i++)
        oss_free (msg.addresses[i], OSS_MEM_HEAP);

      oss_free (msg.addresses, OSS_MEM_HEAP);
    }

  return ret;
}

/* HAL_MSG_IF_SET_SEC_HWADDRS message. */
int
hsl_msg_recv_if_set_sec_hwaddrs (struct socket *sock, struct hal_nlmsghdr *hdr, char *msgbuf)
{
  int ret;
  
  HSL_LOG (HSL_LOG_MSG, HSL_LEVEL_INFO, "Message: HAL set interface secondary hardware address\n"); 

  ret = _hsl_msg_recv_if_sec_hwaddrs (hdr, msgbuf, hsl_ifmgr_set_secondary_hwaddrs);

  if (hdr->nlmsg_flags & HAL_NLM_F_ACK)
    {
      HSL_MSG_PROCESS_RETURN (sock, hdr, ret);
    }

  return ret;
}

/* HAL_MSG_IF_ADD_SEC_HWADDRS message. */
int
hsl_msg_recv_if_add_sec_hwaddrs (struct socket *sock, struct hal_nlmsghdr *hdr, char *msgbuf)
{
  int ret;

  HSL_LOG (HSL_LOG_MSG, HSL_LEVEL_INFO, "Message: HAL add interface secondary hardware address\n"); 

  ret = _hsl_msg_recv_if_sec_hwaddrs (hdr, msgbuf, hsl_ifmgr_add_secondary_hwaddrs);

  if (hdr->nlmsg_flags & HAL_NLM_F_ACK)
    {
      HSL_MSG_PROCESS_RETURN (sock, hdr, ret);
    }

  return ret;
}

/* HAL_MSG_IF_DELETE_SEC_HWADDRS message. */
int
hsl_msg_recv_if_delete_sec_hwaddrs (struct socket *sock, struct hal_nlmsghdr *hdr, char *msgbuf)
{
  int ret;

  HSL_LOG (HSL_LOG_MSG, HSL_LEVEL_INFO, "Message: HAL delete interface secondary hardware address\n"); 

  ret = _hsl_msg_recv_if_sec_hwaddrs (hdr, msgbuf, hsl_ifmgr_delete_secondary_hwaddrs);

  if (hdr->nlmsg_flags & HAL_NLM_F_ACK)
    {
      HSL_MSG_PROCESS_RETURN (sock, hdr, ret);
    }

  return ret;
}
#endif /* HAVE_L3 */

/* HAL_MSG_IF_FLAGS_GET message. */
int
hsl_msg_recv_if_flags_get (struct socket *sock, struct hal_nlmsghdr *hdr, char *msgbuf)
{
  int ret;
  u_int32_t cindex = 0;

  HSL_LOG (HSL_LOG_MSG, HSL_LEVEL_INFO, "Message: HAL Get interface flags\n");

  SET_CINDEX (cindex, HAL_MSG_CINDEX_IF_FLAGS);
  ret = _hsl_msg_process_if_get (sock, hdr, msgbuf, cindex);

  return ret;
}

/* HAL_MSG_IF_FLAGS_SET message. */
int
hsl_msg_recv_if_flags_set (struct socket *sock, struct hal_nlmsghdr *hdr, char *msgbuf)
{
  u_char *pnt;
  u_int32_t size;
  struct hal_msg_if msg;
  int ret;

  HSL_LOG (HSL_LOG_MSG, HSL_LEVEL_INFO, "Message: HAL Set interface flags\n");

  /* Get requests should have ACK flag set. */

  pnt = (u_char*)msgbuf;
  size = hdr->nlmsg_len - HAL_NLMSG_ALIGN(HAL_NLMSGHDR_SIZE);
      
  ret = hsl_msg_decode_if (&pnt, &size, &msg);
  if (ret < 0)
    goto CLEANUP;

  ret = hsl_ifmgr_set_flags (msg.name, msg.ifindex, msg.flags);
  HSL_MSG_PROCESS_RETURN (sock, hdr, ret);
  return ret;

 CLEANUP:
  HSL_LOG (HSL_LOG_MSG, HSL_LEVEL_INFO, "Errror Failed: HAL Set interface flags\n");
  /* Close socket. */
  hsl_sock_release (sock);

  return -1;
}

/* HAL_MSG_IF_FLAGS_UNSET message. */
int
hsl_msg_recv_if_flags_unset (struct socket *sock, struct hal_nlmsghdr *hdr, char *msgbuf)
{
  u_char *pnt;
  u_int32_t size;
  struct hal_msg_if msg;
  int ret;

  HSL_LOG (HSL_LOG_MSG, HSL_LEVEL_INFO, "Message: HAL Unset interface flags\n");

  /* Get requests should have ACK flag set. */

  pnt = (u_char*)msgbuf;
  size = hdr->nlmsg_len - HAL_NLMSG_ALIGN(HAL_NLMSGHDR_SIZE);
      
  ret = hsl_msg_decode_if (&pnt, &size, &msg);
  if (ret < 0)
    goto CLEANUP;

  ret = hsl_ifmgr_unset_flags (msg.name, msg.ifindex, msg.flags);
  HSL_MSG_PROCESS_RETURN (sock, hdr, ret);
  return ret;

 CLEANUP:
  /* Close socket. */
  hsl_sock_release (sock);

  return -1;
}

/* HAL_MSG_IF_GET_BW message. */
int
hsl_msg_recv_if_get_bw (struct socket *sock, struct hal_nlmsghdr *hdr, char *msgbuf)
{
  int ret;
  u_int32_t cindex = 0;

  HSL_LOG (HSL_LOG_MSG, HSL_LEVEL_INFO, "Message: HAL Get interface bandwidth\n");

  SET_CINDEX (cindex, HAL_MSG_CINDEX_IF_BANDWIDTH);
  ret = _hsl_msg_process_if_get (sock, hdr, msgbuf, cindex);

  return ret;
}


/* HAL_MSG_IF_SET_BW message. */
int
hsl_msg_recv_if_set_bw (struct socket *sock, struct hal_nlmsghdr *hdr, char *msgbuf)
{
  u_char *pnt;
  u_int32_t size;
  struct hal_msg_if msg;
  int ret;

  HSL_LOG (HSL_LOG_MSG, HSL_LEVEL_INFO, "Message: HAL Set interface BANDWIDTH\n");

  /* Get requests should have ACK flag set. */

  pnt = (u_char*)msgbuf;
  size = hdr->nlmsg_len - HAL_NLMSG_ALIGN(HAL_NLMSGHDR_SIZE);

  ret = hsl_msg_decode_if (&pnt, &size, &msg);
  if (ret < 0)
    goto CLEANUP;

  ret = hsl_ifmgr_set_bandwidth (msg.ifindex,msg.bandwidth, HSL_FALSE);
  HSL_MSG_PROCESS_RETURN (sock, hdr, ret);
  return ret;

 CLEANUP:
  /* Close socket. */
  hsl_sock_release (sock);

  return -1;
}

/* HAL_MSG_IF_COUNTERS_GET message. */
int 
hsl_msg_recv_if_get_counters (struct socket *sock, struct hal_nlmsghdr *hdr, char *msgbuf)
{
  struct hal_msg_if_stat *req;
  int ret = 0, respsz = 0;
  struct hal_msg_if_stat resp;

  HSL_FN_ENTER();
  HSL_LOG (HSL_LOG_MSG, HSL_LEVEL_INFO, "Message: HAL Get interface counters\n");

  memset (&resp, 0, sizeof (resp));

  req = (struct hal_msg_if_stat*) msgbuf;
  ret =  hsl_ifmgr_get_if_counters(req->ifindex, &resp.cntrs);
  if (ret == 0)
    {
      /* Total response size based on count. */
      respsz = sizeof (struct hal_if_counters);

      /* Post the message. */
      ret = hsl_sock_post_msg (sock, HAL_MSG_IF_COUNTERS_GET, 0, hdr->nlmsg_seq, (char *)&resp, respsz);
    }
  else
    ret = hsl_sock_post_ack (sock, hdr, 0, -1);

  HSL_FN_EXIT (ret);
}

/* HAL_MSG_IF_CLEAR_COUNTERS  message. */
int 
hsl_msg_recv_if_clear_counters (struct socket *sock, 
                                struct hal_nlmsghdr *hdr, char *msgbuf)
{
  int ret = 0;
  struct hal_msg_if_clear_stat *msg;
 
  HSL_FN_ENTER();
  HSL_LOG (HSL_LOG_MSG, HSL_LEVEL_INFO, "Message: HAL clear interface counters\n");
 
  if (!msgbuf)
    goto CLEANUP;
 
  msg = (struct hal_msg_if_clear_stat*) msgbuf;   
 
  ret =  hsl_ifmgr_if_clear_counters(msg->ifindex);
 
  HSL_MSG_PROCESS_RETURN (sock, hdr, ret);
 
  return ret;
 
  CLEANUP:
   /* Close socket. */
   hsl_sock_release (sock);
 
  HSL_FN_EXIT (ret);
}
 
/* HAL_MSG_IF_SET_PORT_TYPE message. */
int
hsl_msg_recv_if_set_port_type (struct socket *sock, struct hal_nlmsghdr *hdr, char *msgbuf)
{
  struct hal_msg_if_port_type *msg = (struct hal_msg_if_port_type *) msgbuf;
  struct hsl_if *ifp = NULL, *ifpp = NULL;
  int ret;
  u_int32_t cindex = 0;

  ifp = hsl_ifmgr_lookup_by_index (msg->ifindex);
  if (! ifp)
    goto ERR; 

  HSL_IFMGR_IF_REF_DEC (ifp);
#if defined(HAVE_L2)
  if (msg->type == HAL_MSG_SET_SWITCHPORT)
    {
      HSL_LOG (HSL_LOG_MSG, HSL_LEVEL_INFO, "Message: HAL Set port type to switchport\n");
      /* Lookup interface. It has to be of type IP. */
      ret = hsl_ifmgr_set_switch_port (ifp, &ifpp, HSL_FALSE);
      if (ret < 0)
        {
          HSL_LOG (HSL_LOG_MSG, HSL_LEVEL_ERROR, "Failed: HAL Set switchport\n");
          goto ERR;
        }
    }
  else 
#endif /* HAVE_L2 */
#if defined(HAVE_L3)
    if (msg->type == HAL_MSG_SET_ROUTERPORT)
      {
        HSL_LOG (HSL_LOG_MSG, HSL_LEVEL_INFO, "Message: HAL Set port type to router port\n");
        /* Lookup interface. It has to be of type L2_ETHERNET. */
        ret = hsl_ifmgr_set_router_port (ifp, NULL, &ifpp, HSL_FALSE);
        if (ret < 0)
          {
            HSL_LOG (HSL_LOG_MSG, HSL_LEVEL_ERROR, "Failed: HAL Set Router port\n");
            goto ERR;
          }
      }
    else
#endif /* HAVE_L3 */
      goto ERR;

  SET_CINDEX (cindex, HAL_MSG_CINDEX_IF_FLAGS);
  SET_CINDEX (cindex, HAL_MSG_CINDEX_IF_METRIC);
  SET_CINDEX (cindex, HAL_MSG_CINDEX_IF_MTU);
  SET_CINDEX (cindex, HAL_MSG_CINDEX_IF_DUPLEX);
  SET_CINDEX (cindex, HAL_MSG_CINDEX_IF_ARP_AGEING_TIMEOUT);
  SET_CINDEX (cindex, HAL_MSG_CINDEX_IF_AUTONEGO);
  SET_CINDEX (cindex, HAL_MSG_CINDEX_IF_HW);
  SET_CINDEX (cindex, HAL_MSG_CINDEX_IF_BANDWIDTH);
  
  ret = _hsl_msg_if_event (sock, ifpp, cindex, HAL_MSG_IF_NEWLINK);
  if (ret < 0)
    goto CLEANUP;

  return 0;  

 ERR:
  HSL_MSG_PROCESS_RETURN (sock, hdr, -1);

  return 0;
 CLEANUP:
  /* Close socket. */
  hsl_sock_release (sock);

  return -1;
}

#if defined(HAVE_L2) && defined(HAVE_L3)
/* HAL_MSG_IF_CREATE_SVI message. */
int
hsl_msg_recv_if_create_svi (struct socket *sock, struct hal_nlmsghdr *hdr, char *msgbuf)
{
  struct hal_msg_svi *msg = (struct hal_msg_svi *) msgbuf;
  struct hsl_if *ifpp = NULL;
  int br_id, vid;
  int ret;
  char *mac;

  HSL_LOG (HSL_LOG_MSG, HSL_LEVEL_INFO, "Message: HAL Create SVI\n");

  /* Get MAC for SVI. */
  mac = hsl_bcm_ifmap_svi_mac_get ();

  sscanf (msg->name, "vlan%d.%d", &br_id, &vid);

  /* Register with interface manager. */
  ret = hsl_ifmgr_L3_register (msg->name, (u_char*)mac, 6, NULL, &ifpp);
  if (ret < 0)
    goto ERR;

  /* Link the VLAN port members to this SVI. */
  hsl_vlan_svi_link_port_members (ifpp, vid);

  HSL_MSG_PROCESS_RETURN (sock, hdr, 0);
  return 0;

 ERR:
  HSL_MSG_PROCESS_RETURN (sock, hdr, -1);

  return 0;
}

/* HAL_MSG_IF_DELETE_SVI message. */
int
hsl_msg_recv_if_delete_svi (struct socket *sock, struct hal_nlmsghdr *hdr, char *msgbuf)
{
  struct hal_msg_svi *msg = (struct hal_msg_svi *) msgbuf;
  int ret;

  HSL_LOG (HSL_LOG_MSG, HSL_LEVEL_INFO, "Message: HAL Delete SVI\n");

  /* Unregister this SVI from the interface manager. */
  ret = hsl_ifmgr_L3_unregister (msg->name, msg->ifindex);
  if (ret < 0)
    {
      HSL_MSG_PROCESS_RETURN (sock, hdr, -1);
      return 0;
    } 

  HSL_MSG_PROCESS_RETURN (sock, hdr, 0);

  return 0;
}
#endif /* HAVE_L2 && HAVE_L3 */

#ifdef HAVE_L3
/*  HAL_MSG_IF_IPV4_NEWADDR message. */
int
hsl_msg_recv_if_ipv4_newaddr (struct socket *sock, struct hal_nlmsghdr *hdr, char *msgbuf)
{
  struct hal_msg_if_ipv4_addr *msg = (struct hal_msg_if_ipv4_addr *) msgbuf;
  hsl_prefix_t prefix;
  int ret;

  HSL_LOG (HSL_LOG_MSG, HSL_LEVEL_INFO, "Message: HAL IPv4 address add\n");

  memset (&prefix, 0, sizeof (hsl_prefix_t));
  prefix.family = AF_INET;
  prefix.prefixlen = msg->ipmask;
  prefix.u.prefix4 = msg->addr.s_addr;
  
  ret = hsl_ifmgr_ipv4_address_add (msg->name, msg->ifindex, &prefix, 0);
  HSL_MSG_PROCESS_RETURN (sock, hdr, ret);

  return ret;
}

/*  HAL_MSG_IF_IPV4_DELADDR message. */
int
hsl_msg_recv_if_ipv4_deladdr (struct socket *sock, struct hal_nlmsghdr *hdr, char *msgbuf)
{
  struct hal_msg_if_ipv4_addr *msg = (struct hal_msg_if_ipv4_addr *) msgbuf;
  hsl_prefix_t prefix;
  int ret;

  HSL_LOG (HSL_LOG_MSG, HSL_LEVEL_INFO, "Message: HAL IPv4 address delete\n");

  memset (&prefix, 0, sizeof (hsl_prefix_t));
  prefix.family = AF_INET;
  prefix.prefixlen = msg->ipmask;
  prefix.u.prefix4 = msg->addr.s_addr;
  
  ret = hsl_ifmgr_ipv4_address_delete (msg->name, msg->ifindex, &prefix);
  HSL_MSG_PROCESS_RETURN (sock, hdr, ret);

  return ret;
}

/*  HAL_MSG_IPV4_INIT message. */
int
hsl_msg_recv_ipv4_init (struct socket *sock, struct hal_nlmsghdr *hdr, char *msgbuf)
{

  HSL_LOG (HSL_LOG_MSG, HSL_LEVEL_INFO, "Message: HAL IPv4 initialization\n");
  HSL_MSG_PROCESS_RETURN (sock, hdr, 0);

  return 0;
}

/*  HAL_MSG_IPV4_DEINIT message. */
int
hsl_msg_recv_ipv4_deinit (struct socket *sock, struct hal_nlmsghdr *hdr, char *msgbuf)
{
  HSL_LOG (HSL_LOG_MSG, HSL_LEVEL_INFO, "Message: HAL IPv4 deinitialization\n");
  HSL_MSG_PROCESS_RETURN (sock, hdr, 0);
  return 0;
}

/*  HAL_MSG_GET_MAX_MULTIPATH message. */
int
hsl_msg_recv_get_max_multipath (struct socket *sock, struct hal_nlmsghdr *hdr, char *msgbuf)
{
  u_int32_t ecmp;

  HSL_FN_ENTER();
  HSL_LOG (HSL_LOG_MSG, HSL_LEVEL_INFO, "Message: HAL Get maximum number of multipaths.\n");

  hsl_fib_get_max_num_multipath(&ecmp);

  hsl_sock_post_msg (sock, HAL_MSG_GET_MAX_MULTIPATH, 0, hdr->nlmsg_seq, (char *)&ecmp, sizeof (u_int32_t));
  HSL_FN_EXIT(STATUS_OK);
}

/*  HAL_MSG_FIB_CREATE message. */
int
hsl_msg_recv_fib_create (struct socket *sock, struct hal_nlmsghdr *hdr, char *msgbuf)
{
  int ret;
  hsl_fib_id_t *fib_id;

  HSL_FN_ENTER();

  fib_id = (hsl_fib_id_t *)msgbuf;

  HSL_LOG (HSL_LOG_MSG, HSL_LEVEL_INFO, "Message: HAL FIB %d Create \n", *fib_id);

  ret = hsl_fib_create_tables (*fib_id);

  HSL_MSG_PROCESS_RETURN (sock, hdr, ret);

  HSL_FN_EXIT(STATUS_OK);
}

/*  HAL_MSG_FIB_DELETE message. */
int
hsl_msg_recv_fib_delete (struct socket *sock, struct hal_nlmsghdr *hdr, char *msgbuf)
{
  int ret;
  hsl_fib_id_t *fib_id;

  HSL_FN_ENTER();

  fib_id = (hsl_fib_id_t *)msgbuf;

  HSL_LOG (HSL_LOG_MSG, HSL_LEVEL_INFO, "Message: HAL FIB %d Delete\n", *fib_id);

  ret = hsl_fib_destroy_tables (*fib_id);

  HSL_MSG_PROCESS_RETURN (sock, hdr, ret);

  HSL_FN_EXIT(STATUS_OK);
}

/*  HAL_MSG_IPV4_UC_ADD message. */
int
hsl_msg_recv_ipv4_uc_add (struct socket *sock, struct hal_nlmsghdr *hdr, char *msgbuf)
{
  u_char *pnt; 
  u_int32_t size;
  struct hsl_route_table *prefix_table;
  struct hal_msg_ipv4uc_route msg;
  struct hsl_route_node *rnp;
  int ret;
  int i, j;
  hsl_prefix_t p;
  
  HSL_FN_ENTER();

  HSL_LOG (HSL_LOG_MSG, HSL_LEVEL_INFO, "Message: IPv4 Unicast Add.\n");

  rnp = NULL;
  prefix_table = NULL;
  pnt = (u_char*)msgbuf;
  size = hdr->nlmsg_len - HAL_NLMSG_ALIGN(HAL_NLMSGHDR_SIZE);
       
  ret =  hsl_msg_decode_ipv4_route (&pnt, &size, &msg);
  if (ret < 0)
    goto CLEANUP;
    
  /* Set prefix. */
  p.family = AF_INET;
  p.prefixlen = msg.masklen;
  p.u.prefix4 = msg.addr.s_addr;
  
  /* Update the ECMP flag */
  if (HSL_FIB_ID_VALID ((hsl_fib_id_t)msg.fib_id) &&
      HSL_FIB_TBL_VALID (prefix, (hsl_fib_id_t)msg.fib_id))
    {
      prefix_table = HSL_FIB_TBL (prefix, (hsl_fib_id_t)msg.fib_id);

      /* Find prefix entry. */
      rnp = hsl_fib_prefix_node_get (prefix_table, &p);
      if (! rnp)
        goto CLEANUP;

      if (msg.num > 1)
        rnp->is_ecmp = HSL_TRUE;
      else
        rnp->is_ecmp = HSL_FALSE;
    }

  for (i = 0; i < msg.num; i++)
    { 
      if (msg.nh[i].nexthopIP.s_addr == 0)
        ret = hsl_fib_add_ipv4uc_prefix ((hsl_fib_id_t)msg.fib_id,
            &p, msg.nh[i].type, msg.nh[i].egressIfindex,
                                         NULL);
      else
        ret = hsl_fib_add_ipv4uc_prefix ((hsl_fib_id_t)msg.fib_id,
            &p, msg.nh[i].type, msg.nh[i].egressIfindex,
                                         &msg.nh[i].nexthopIP.s_addr);
      if (ret < 0)
        goto ERR;
    }

  HSL_MSG_PROCESS_RETURN (sock, hdr, 0);
  return 0;
  
 ERR:
  /* Delete previous installed routes. */
  for (j = 0; j < i; j++)
    {
      if (msg.nh[j].nexthopIP.s_addr == 0)
        hsl_fib_delete_ipv4uc_prefix ((hsl_fib_id_t)msg.fib_id,
            &p, msg.nh[j].type, msg.nh[j].egressIfindex,
                                      NULL);
      else
        hsl_fib_delete_ipv4uc_prefix ((hsl_fib_id_t)msg.fib_id,
            &p, msg.nh[j].type, msg.nh[j].egressIfindex,
                                      &msg.nh[j].nexthopIP.s_addr);
    }

  HSL_MSG_PROCESS_RETURN (sock, hdr, -1);
  HSL_FN_EXIT (0);

 CLEANUP:
  /* Close socket. */
  hsl_sock_release (sock);
  
  return -1;
}

/*  HAL_MSG_IPV4_UC_DELETE message. */
int
hsl_msg_recv_ipv4_uc_delete (struct socket *sock, struct hal_nlmsghdr *hdr, char *msgbuf)
{
  u_char *pnt;
  u_int32_t size;
  struct hal_msg_ipv4uc_route msg;
  int ret;
  int i, j;
  hsl_prefix_t p;

  HSL_FN_ENTER ();

  HSL_LOG (HSL_LOG_MSG, HSL_LEVEL_INFO, "Message: IPv4 Unicast Delete.\n");

  pnt = (u_char*)msgbuf;
  size = hdr->nlmsg_len - HAL_NLMSG_ALIGN(HAL_NLMSGHDR_SIZE);

  ret =  hsl_msg_decode_ipv4_route (&pnt, &size, &msg);
  if (ret < 0)
    goto CLEANUP;

  /* Set prefix. */
  p.family = AF_INET;
  p.prefixlen = msg.masklen;
  p.u.prefix4 = msg.addr.s_addr;

  for (i = 0; i < msg.num; i++)
    {
      if (msg.nh[i].nexthopIP.s_addr == 0)
        ret = hsl_fib_delete_ipv4uc_prefix ((hsl_fib_id_t)msg.fib_id,
            &p, msg.nh[i].type, msg.nh[i].egressIfindex,
                                            NULL);
      else
        ret = hsl_fib_delete_ipv4uc_prefix ((hsl_fib_id_t)msg.fib_id,
            &p, msg.nh[i].type, msg.nh[i].egressIfindex,
                                            &msg.nh[i].nexthopIP.s_addr);
      if (ret < 0)
        goto ERR;
    }

  HSL_MSG_PROCESS_RETURN (sock, hdr, 0);
  HSL_FN_EXIT (0);

 ERR:
  /* Reinstall the deleted ones to send one response. */
  for (j = 0; j < i; j++)
    {
      if (msg.nh[j].nexthopIP.s_addr == 0)
        ret = hsl_fib_add_ipv4uc_prefix ((hsl_fib_id_t)msg.fib_id,
            &p, msg.nh[j].type, msg.nh[j].egressIfindex,
                                         NULL);
      else
        ret = hsl_fib_add_ipv4uc_prefix ((hsl_fib_id_t)msg.fib_id,
            &p, msg.nh[j].type, msg.nh[j].egressIfindex,
                                         &msg.nh[j].nexthopIP.s_addr);
    }

  HSL_MSG_PROCESS_RETURN (sock, hdr, -1);
  HSL_FN_EXIT (0);
 CLEANUP:
  /* Close socket. */
  hsl_sock_release (sock);

  return -1;
}

/*  HAL_MSG_IPV4_UC_UPDATE message. */
int
hsl_msg_recv_ipv4_uc_update (struct socket *sock, struct hal_nlmsghdr *hdr, char *msgbuf)
{
  return 0;
}

#ifdef HAVE_MCAST_IPV4

/* HAL_MSG_IPV4_MC_INIT */
int
hsl_msg_recv_ipv4_mc_init(struct socket *sock, struct hal_nlmsghdr *hdr, char *msgbuf)
{
  HSL_FN_ENTER();
  HSL_LOG (HSL_LOG_MSG, HSL_LEVEL_INFO, "Message: HAL multicast init.\n");

  hsl_ipv4_mc_init();

  HSL_MSG_PROCESS_RETURN (sock, hdr, 0);
  HSL_FN_EXIT(STATUS_OK);
}

/* HAL_MSG_IPV4_MC_DEINIT */
int
hsl_msg_recv_ipv4_mc_deinit(struct socket *sock, struct hal_nlmsghdr *hdr, char *msgbuf)
{
  HSL_FN_ENTER();
  HSL_LOG (HSL_LOG_MSG, HSL_LEVEL_INFO, "Message: HAL multicast deinit.\n");

  hsl_ipv4_mc_deinit();

  HSL_MSG_PROCESS_RETURN (sock, hdr, 0);
  HSL_FN_EXIT(STATUS_OK);
}
/* HAL_MSG_IPV4_MC_PIM_INIT */
int
hsl_msg_recv_ipv4_mc_pim_init(struct socket *sock, struct hal_nlmsghdr *hdr, char *msgbuf)
{
  HSL_FN_ENTER();
  HSL_LOG (HSL_LOG_MSG, HSL_LEVEL_INFO, "Message: HAL multicast pim init.\n");
  
  hsl_ipv4_mc_pim_init();
  
  HSL_MSG_PROCESS_RETURN (sock, hdr, 0);
  HSL_FN_EXIT(STATUS_OK);
}

/* HAL_MSG_IPV4_MC_PIM_DEINIT */
int 
hsl_msg_recv_ipv4_mc_pim_deinit(struct socket *sock, struct hal_nlmsghdr *hdr, char *msgbuf)
{
  HSL_FN_ENTER();
  HSL_LOG (HSL_LOG_MSG, HSL_LEVEL_INFO, "Message: HAL multicast pim deinit.\n");

  hsl_ipv4_mc_pim_deinit();
  
  HSL_MSG_PROCESS_RETURN (sock, hdr, 0);
  HSL_FN_EXIT(STATUS_OK);
}

/* HAL_MSG_IPV4_MC_VIF_ADD */
int 
hsl_msg_recv_ipv4_mc_vif_add(struct socket *sock, struct hal_nlmsghdr *hdr, char *msgbuf)
{
  u_char *pnt;
  u_int32_t size;
  struct hal_msg_ipv4mc_vif_add msg;
  int ret;

  HSL_FN_ENTER(); 
  HSL_LOG (HSL_LOG_MSG, HSL_LEVEL_INFO, "Message: HAL add mc interface\n");

  /* Get requests should have ACK flag set. */
  pnt = (u_char*)msgbuf;
  size = hdr->nlmsg_len - HAL_NLMSG_ALIGN(HAL_NLMSGHDR_SIZE);
      
  ret = hsl_msg_decode_ipv4_vif_add(&pnt, &size, &msg);
  if (ret < 0)
    goto CLEANUP;

  ret = hsl_ipv4_mc_vif_add(msg.vif_index, msg.ifindex, 
                            msg.loc_addr.s_addr,
                            msg.rmt_addr.s_addr,msg.flags);
  HSL_MSG_PROCESS_RETURN (sock, hdr, ret);
  HSL_FN_EXIT(ret);

 CLEANUP:
  /* Close socket. */
  hsl_sock_release (sock);

  HSL_FN_EXIT(STATUS_ERROR);
}

/* HAL_MSG_IPV4_MC_VIF_DEL */
int
hsl_msg_recv_ipv4_mc_vif_del(struct socket *sock, struct hal_nlmsghdr *hdr, char *msgbuf)
{
  u_int32_t vif_index = *((int *)msgbuf);
  int ret;
  
  HSL_FN_ENTER();
  HSL_LOG (HSL_LOG_MSG, HSL_LEVEL_INFO, "Message: HAL delete mc interface\n");

  ret = hsl_ipv4_mc_vif_del(vif_index);

  HSL_MSG_PROCESS_RETURN (sock, hdr, ret);
  HSL_FN_EXIT(ret);
}

/* HAL_MSG_IPV4_MC_MRT_ADD */
int 
hsl_msg_recv_ipv4_mc_route_add(struct socket *sock, struct hal_nlmsghdr *hdr, char *msgbuf)
{
  u_char *pnt;
  u_int32_t size;
  struct hal_msg_ipv4mc_mrt_add msg;
  int ret;
   
  HSL_FN_ENTER(); 
  HSL_LOG (HSL_LOG_MSG, HSL_LEVEL_INFO, "Message: HAL add mc route \n");

  /* Get requests should have ACK flag set. */
  pnt = (u_char*)msgbuf;
  size = hdr->nlmsg_len - HAL_NLMSG_ALIGN(HAL_NLMSGHDR_SIZE);
      
  memset (&msg, 0, sizeof (msg));

  ret = hsl_msg_decode_ipv4_mrt_add(&pnt, &size, &msg);
  if (ret < 0)
    goto CLEANUP;

  ret = hsl_mc_v4_route_add(msg.src.s_addr, msg.group.s_addr,
                              msg.iif_vif, msg.num_olist, msg.olist_ttls);

  /* Free memory allocated by decoding routine */
  if (msg.olist_ttls)
    oss_free (msg.olist_ttls, OSS_MEM_HEAP);

  HSL_MSG_PROCESS_RETURN (sock, hdr, ret);
  HSL_FN_EXIT(ret);

 CLEANUP:

  /* Free memory allocated by decoding routine */
  if (msg.olist_ttls)
    oss_free (msg.olist_ttls, OSS_MEM_HEAP);

  /* Close socket. */
  hsl_sock_release (sock);

  HSL_FN_EXIT(STATUS_ERROR);
}

/* HAL_MSG_IPV4_MC_MRT_DEL */ 
int
hsl_msg_recv_ipv4_mc_route_del(struct socket *sock, struct hal_nlmsghdr *hdr, char *msgbuf)
{
  u_char *pnt;
  u_int32_t size;
  struct hal_msg_ipv4mc_mrt_del msg;
  int ret;

  HSL_FN_ENTER(); 
  HSL_LOG (HSL_LOG_MSG, HSL_LEVEL_INFO, "Message: HAL delete mc route \n");

  /* Get requests should have ACK flag set. */
  pnt = (u_char*)msgbuf;
  size = hdr->nlmsg_len - HAL_NLMSG_ALIGN(HAL_NLMSGHDR_SIZE);
      
  ret = hsl_msg_decode_ipv4_mrt_del(&pnt, &size, &msg);
  if (ret < 0)
    goto CLEANUP;

  ret = hsl_mc_v4_route_del(msg.src.s_addr, msg.group.s_addr);
  HSL_MSG_PROCESS_RETURN (sock, hdr, ret);
  HSL_FN_EXIT(ret);

 CLEANUP:
  /* Close socket. */
  hsl_sock_release (sock);

  HSL_FN_EXIT(STATUS_ERROR);

}

/* HAL_MSG_IPV4_MC_SG_STAT_REQ */ 
int
hsl_msg_recv_ipv4_mc_stat_get(struct socket *sock, struct hal_nlmsghdr *hdr, char *msgbuf)
{
  u_char *pnt;
  u_int32_t size;
  struct hal_msg_ipv4mc_sg_stat msg;
  int ret; 
  int szsgstat = sizeof (struct hal_msg_ipv4mc_sg_stat);

  HSL_FN_ENTER(); 
  HSL_LOG (HSL_LOG_MSG, HSL_LEVEL_INFO, "Message: HAL get mc route statistics\n");

  /* Get requests should have ACK flag set. */
  pnt = (u_char*)msgbuf;
  size = hdr->nlmsg_len - HAL_NLMSG_ALIGN(HAL_NLMSGHDR_SIZE);
      
  ret = hsl_msg_decode_ipv4_sg_stat(&pnt, &size, &msg);
  if (ret < 0)
    goto CLEANUP;

  ret = hsl_mc_v4_sg_stat(msg.src.s_addr, msg.group.s_addr, 
                            msg.iif_vif, &msg.pktcnt, &msg.bytecnt, 
                            &msg.wrong_if);

  /* Post response. */
  hsl_sock_post_msg (sock, HAL_MSG_IPV4_MC_SG_STAT, hdr->nlmsg_seq, 0, (char *) &msg, szsgstat);

  HSL_FN_EXIT(ret);

 CLEANUP:
  /* Close socket. */
  hsl_sock_release (sock);

  HSL_FN_EXIT(STATUS_ERROR);
}
#endif /* HAVE_MCAST_IPV4 */

#ifdef HAVE_MCAST_IPV6

/* HAL_MSG_IPV6_MC_INIT */
int
hsl_msg_recv_ipv6_mc_init (struct socket *sock, struct hal_nlmsghdr *hdr, char *msgbuf)
{
  HSL_FN_ENTER();
  HSL_LOG (HSL_LOG_MSG, HSL_LEVEL_INFO, "Message: HAL IPv6 multicast init.\n");

  hsl_ipv6_mc_init();

  HSL_MSG_PROCESS_RETURN (sock, hdr, 0);
  HSL_FN_EXIT(STATUS_OK);
}

/* HAL_MSG_IPV6_MC_DEINIT */
int
hsl_msg_recv_ipv6_mc_deinit (struct socket *sock, struct hal_nlmsghdr *hdr, char *msgbuf)
{
  HSL_FN_ENTER();
  HSL_LOG (HSL_LOG_MSG, HSL_LEVEL_INFO, "Message: HAL IPv6 multicast deinit.\n");

  hsl_ipv6_mc_deinit();

  HSL_MSG_PROCESS_RETURN (sock, hdr, 0);
  HSL_FN_EXIT(STATUS_OK);
}

/* HAL_MSG_IPV6_MC_PIM_INIT */
int
hsl_msg_recv_ipv6_mc_pim_init (struct socket *sock, struct hal_nlmsghdr *hdr, char *msgbuf)
{
  HSL_FN_ENTER();
  HSL_LOG (HSL_LOG_MSG, HSL_LEVEL_INFO, "Message: HAL IPv6 multicast pim init.\n");
  
  hsl_ipv6_mc_pim_init();
  
  HSL_MSG_PROCESS_RETURN (sock, hdr, 0);
  HSL_FN_EXIT(STATUS_OK);
}

/* HAL_MSG_IPV6_MC_PIM_DEINIT */
int 
hsl_msg_recv_ipv6_mc_pim_deinit (struct socket *sock, struct hal_nlmsghdr *hdr, char *msgbuf)
{
  HSL_FN_ENTER();
  HSL_LOG (HSL_LOG_MSG, HSL_LEVEL_INFO, "Message: HAL IPv6 multicast pim deinit.\n");

  hsl_ipv6_mc_pim_deinit();
  
  HSL_MSG_PROCESS_RETURN (sock, hdr, 0);
  HSL_FN_EXIT(STATUS_OK);
}

/* HAL_MSG_IPV6_MC_VIF_ADD */
int 
hsl_msg_recv_ipv6_mc_vif_add(struct socket *sock, struct hal_nlmsghdr *hdr, char *msgbuf)
{
  u_char *pnt;
  u_int32_t size;
  struct hal_msg_ipv6mc_vif_add msg;
  int ret;

  HSL_FN_ENTER(); 
  HSL_LOG (HSL_LOG_MSG, HSL_LEVEL_INFO, "Message: HAL add IPv6 mc interface\n");

  /* Get requests should have ACK flag set. */
  pnt = msgbuf;
  size = hdr->nlmsg_len - HAL_NLMSG_ALIGN(HAL_NLMSGHDR_SIZE);
      
  ret = hsl_msg_decode_ipv6_vif_add (&pnt, &size, &msg);
  if (ret < 0)
    goto CLEANUP;

  ret = hsl_ipv6_mc_vif_add (msg.vif_index, msg.phy_ifindex, msg.flags);

  HSL_MSG_PROCESS_RETURN (sock, hdr, ret);
  HSL_FN_EXIT(ret);

 CLEANUP:
  /* Close socket. */
  hsl_sock_release (sock);

  HSL_FN_EXIT(STATUS_ERROR);
}

/* HAL_MSG_IPV6_MC_VIF_DEL */
int
hsl_msg_recv_ipv6_mc_vif_del (struct socket *sock, struct hal_nlmsghdr *hdr, char *msgbuf)
{
  u_int32_t vif_index = *((u_int32_t *)msgbuf);
  int ret;
  
  HSL_FN_ENTER();
  HSL_LOG (HSL_LOG_MSG, HSL_LEVEL_INFO, "Message: HAL delete IPv6 mc interface\n");

  ret = hsl_ipv6_mc_vif_del (vif_index);

  HSL_MSG_PROCESS_RETURN (sock, hdr, ret);
  HSL_FN_EXIT(ret);
}

/* HAL_MSG_IPV6_MC_MRT_ADD */
int 
hsl_msg_recv_ipv6_mc_route_add (struct socket *sock, struct hal_nlmsghdr *hdr, char *msgbuf)
{
  u_char *pnt;
  u_int32_t size;
  struct hal_msg_ipv6mc_mrt_add msg;
  int ret;
   
  HSL_FN_ENTER(); 
  HSL_LOG (HSL_LOG_MSG, HSL_LEVEL_INFO, "Message: HAL add IPv6 mc route \n");

  /* Get requests should have ACK flag set. */
  pnt = msgbuf;
  size = hdr->nlmsg_len - HAL_NLMSG_ALIGN(HAL_NLMSGHDR_SIZE);
      
  memset (&msg, 0, sizeof (struct hal_msg_ipv6mc_mrt_add));

  ret = hsl_msg_decode_ipv6_mrt_add (&pnt, &size, &msg);
  if (ret < 0)
    goto CLEANUP;

  ret = hsl_mc_v6_route_add((hsl_ipv6Address_t *)&msg.src, 
                              (hsl_ipv6Address_t *)&msg.group, msg.iif_vif, msg.num_olist, 
                              msg.olist);

  /* Free memory allocated by decoding routine */
  if (msg.olist)
    oss_free (msg.olist, OSS_MEM_HEAP);

  HSL_MSG_PROCESS_RETURN (sock, hdr, ret);
  HSL_FN_EXIT(ret);

 CLEANUP:

  /* Free memory allocated by decoding routine */
  if (msg.olist)
    oss_free (msg.olist, OSS_MEM_HEAP);

  /* Close socket. */
  hsl_sock_release (sock);

  HSL_FN_EXIT(STATUS_ERROR);
}

/* HAL_MSG_IPV6_MC_MRT_DEL */ 
int
hsl_msg_recv_ipv6_mc_route_del (struct socket *sock, struct hal_nlmsghdr *hdr, char *msgbuf)
{
  u_char *pnt;
  u_int32_t size;
  struct hal_msg_ipv6mc_mrt_del msg;
  int ret;

  HSL_FN_ENTER(); 
  HSL_LOG (HSL_LOG_MSG, HSL_LEVEL_INFO, "Message: HAL delete IPv6 mc route \n");

  /* Get requests should have ACK flag set. */
  pnt = msgbuf;
  size = hdr->nlmsg_len - HAL_NLMSG_ALIGN(HAL_NLMSGHDR_SIZE);
      
  ret = hsl_msg_decode_ipv6_mrt_del (&pnt, &size, &msg);
  if (ret < 0)
    goto CLEANUP;

  ret = hsl_mc_v6_route_del((hsl_ipv6Address_t *)&msg.src, (hsl_ipv6Address_t *)&msg.group);

  HSL_MSG_PROCESS_RETURN (sock, hdr, ret);
  HSL_FN_EXIT(ret);

 CLEANUP:
  /* Close socket. */
  hsl_sock_release (sock);

  HSL_FN_EXIT(STATUS_ERROR);

}

/* HAL_MSG_IPV6_MC_SG_STAT_REQ */ 
int
hsl_msg_recv_ipv6_mc_stat_get (struct socket *sock, struct hal_nlmsghdr *hdr, char *msgbuf)
{
  u_char *pnt;
  u_int32_t size;
  struct hal_msg_ipv6mc_sg_stat msg;
  int ret; 
  int szsgstat = sizeof (struct hal_msg_ipv6mc_sg_stat);

  HSL_FN_ENTER(); 
  HSL_LOG (HSL_LOG_MSG, HSL_LEVEL_INFO, "Message: HAL get IPv6 mc route statistics\n");

  /* Get requests should have ACK flag set. */
  pnt = msgbuf;
  size = hdr->nlmsg_len - HAL_NLMSG_ALIGN(HAL_NLMSGHDR_SIZE);
      
  ret = hsl_msg_decode_ipv6_sg_stat (&pnt, &size, &msg);
  if (ret < 0)
    goto CLEANUP;

  ret = hsl_mc_v6_sg_stat  ((hsl_ipv6Address_t *)&msg.src, (hsl_ipv6Address_t *)&msg.group, 
                             msg.iif_vif, &msg.pktcnt, &msg.bytecnt, 
                             &msg.wrong_if);

  /* Post response. */
  hsl_sock_post_msg (sock, HAL_MSG_IPV6_MC_SG_STAT, hdr->nlmsg_seq, 0, (char *) &msg, szsgstat);

  HSL_FN_EXIT(ret);

 CLEANUP:
  /* Close socket. */
  hsl_sock_release (sock);

  HSL_FN_EXIT(STATUS_ERROR);
}
#endif /* HAVE_MCAST_IPV6 */

#ifdef HAVE_IPV6
/*  HAL_MSG_IF_IPV6_ADDRESS_ADD message. */
int
hsl_msg_recv_if_ipv6_newaddr (struct socket *sock, struct hal_nlmsghdr *hdr, char *msgbuf)
{
  struct hal_msg_if_ipv6_addr *msg = (struct hal_msg_if_ipv6_addr *) msgbuf;
  hsl_prefix_t prefix;
  int ret;

  HSL_LOG (HSL_LOG_MSG, HSL_LEVEL_INFO, "Message: HAL IPv6 address add\n");

  memset (&prefix, 0, sizeof (hsl_prefix_t));
  prefix.family = AF_INET6;
  prefix.prefixlen = msg->ipmask;
  memcpy (&prefix.u.prefix6, &msg->addr, sizeof (struct hal_in6_addr));
  
  ret = hsl_ifmgr_ipv6_address_add (msg->name, msg->ifindex, &prefix, msg->flags);
  HSL_MSG_PROCESS_RETURN (sock, hdr, ret);

  return ret;
}

/*  HAL_MSG_IF_IPV6_ADDRESS_DELETE message. */
int
hsl_msg_recv_if_ipv6_deladdr (struct socket *sock, struct hal_nlmsghdr *hdr, char *msgbuf)
{
  struct hal_msg_if_ipv6_addr *msg = (struct hal_msg_if_ipv6_addr *) msgbuf;
  hsl_prefix_t prefix;
  int ret;

  HSL_LOG (HSL_LOG_MSG, HSL_LEVEL_INFO, "Message: HAL IPv6 address delete\n");

  memset (&prefix, 0, sizeof (hsl_prefix_t));
  prefix.family = AF_INET6;
  prefix.prefixlen = msg->ipmask;
  memcpy (&prefix.u.prefix6, &msg->addr, sizeof (struct hal_in6_addr));
  
  ret = hsl_ifmgr_ipv6_address_delete (msg->name, msg->ifindex, &prefix);
  HSL_MSG_PROCESS_RETURN (sock, hdr, ret);

  return ret;
}

/*  HAL_MSG_IPV6_INIT message. */
int
hsl_msg_recv_ipv6_init (struct socket *sock, struct hal_nlmsghdr *hdr, char *msgbuf)
{

  HSL_LOG (HSL_LOG_MSG, HSL_LEVEL_INFO, "Message: HAL IPv6 initialization\n");

#if 0
  /* Initialize L3. */
  bcmx_l3_init ();
#endif 
  HSL_MSG_PROCESS_RETURN (sock, hdr, 0);
  
  return 0;
}

/*  HAL_MSG_IPV6_DEINIT message. */
int
hsl_msg_recv_ipv6_deinit (struct socket *sock, struct hal_nlmsghdr *hdr, char *msgbuf)
{
  HSL_LOG (HSL_LOG_MSG, HSL_LEVEL_INFO, "Message: HAL IPv6 deinitialization\n");
  HSL_MSG_PROCESS_RETURN (sock, hdr, 0);
  return 0;
}

/*  HAL_MSG_IPV6_UC_INIT message. */
int
hsl_msg_recv_ipv6_uc_init (struct socket *sock, struct hal_nlmsghdr *hdr, char *msgbuf)
{
  return 0;
}


/*  HAL_MSG_IPV6_UC_DEINIT message. */
int
hsl_msg_recv_ipv6_uc_deinit (struct socket *sock, struct hal_nlmsghdr *hdr, char *msgbuf)
{
  return 0;
}



/*  HAL_MSG_IPV6_UC_ADD message. */
int
hsl_msg_recv_ipv6_uc_add (struct socket *sock, struct hal_nlmsghdr *hdr, char *msgbuf)
{
  u_char *pnt; 
  u_int32_t size;
  struct hal_msg_ipv6uc_route msg;
  struct hsl_route_table *prefix_table;
  struct hsl_route_node *rnp = NULL;
  int ret;
  int i, j;
  hsl_prefix_t p;
  
  pnt = (u_char*)msgbuf;
  size = hdr->nlmsg_len - HAL_NLMSG_ALIGN(HAL_NLMSGHDR_SIZE);
       
  ret =  hsl_msg_decode_ipv6_route (&pnt, &size, &msg);
  if (ret < 0)
    goto CLEANUP;
    
  /* Set prefix. */
  p.family = AF_INET6;
  p.prefixlen = msg.masklen;
  memcpy (&p.u.prefix6, &msg.addr, sizeof (struct hal_in6_addr));
  
  /* Update the ECMP flag */
  if (HSL_FIB_ID_VALID ((hsl_fib_id_t)msg.fib_id) &&
      HSL_FIB_TBL_VALID (prefix6, (hsl_fib_id_t)msg.fib_id))
    {
      prefix_table = HSL_FIB_TBL (prefix6, (hsl_fib_id_t)msg.fib_id);

      /* Find prefix entry. */
      rnp = hsl_fib_prefix_node_get (prefix_table, &p);
      if (! rnp)
        goto CLEANUP;

      if (msg.num > 1)
        rnp->is_ecmp = HSL_TRUE;
    }

  for (i = 0; i < msg.num; i++)
    { 
      if (IPV6_ADDR_ZERO (msg.nh[i].nexthopIP))
        ret = hsl_fib_add_ipv6uc_prefix ((hsl_fib_id_t)msg.fib_id,
            &p, msg.nh[i].type, msg.nh[i].egressIfindex,
                                         NULL);
      else
        ret = hsl_fib_add_ipv6uc_prefix ((hsl_fib_id_t)msg.fib_id,
            &p, msg.nh[i].type, msg.nh[i].egressIfindex,
                                         (hsl_ipv6Address_t *) &msg.nh[i].nexthopIP);
      if (ret < 0)
        goto ERR;
    }

  /* Reset the ECMP Flag */
  if (rnp)
    rnp->is_ecmp = HSL_FALSE;

  HSL_MSG_PROCESS_RETURN (sock, hdr, 0);
  return 0;
  
 ERR:
  /* Delete previous installed routes. */
  for (j = 0; j < i; j++)
    {
      if (IPV6_ADDR_ZERO (msg.nh[j].nexthopIP))
        hsl_fib_delete_ipv6uc_prefix ((hsl_fib_id_t)msg.fib_id,
            &p, msg.nh[j].type, msg.nh[j].egressIfindex,
                                      NULL);
      else
        hsl_fib_delete_ipv6uc_prefix ((hsl_fib_id_t)msg.fib_id,
            &p, msg.nh[j].type, msg.nh[j].egressIfindex,
                                      (hsl_ipv6Address_t *) &msg.nh[j].nexthopIP);
    }
  HSL_MSG_PROCESS_RETURN (sock, hdr, -1);
  return 0;

 CLEANUP:
  /* Close socket. */
  hsl_sock_release (sock);
  
  return -1;
}

/*  HAL_MSG_IPV6_UC_DELETE message. */
int
hsl_msg_recv_ipv6_uc_delete (struct socket *sock, struct hal_nlmsghdr *hdr, char *msgbuf)
{
  u_char *pnt;
  u_int32_t size;
  struct hal_msg_ipv6uc_route msg;
  int ret;
  int i, j;
  hsl_prefix_t p;

  HSL_FN_ENTER ();

  HSL_LOG (HSL_LOG_MSG, HSL_LEVEL_INFO, "Message: IPv4 Unicast Delete.\n");

  pnt = (u_char*)msgbuf;
  size = hdr->nlmsg_len - HAL_NLMSG_ALIGN(HAL_NLMSGHDR_SIZE);

  ret =  hsl_msg_decode_ipv6_route (&pnt, &size, &msg);
  if (ret < 0)
    goto CLEANUP;

  /* Set prefix. */
  p.family = AF_INET6;
  p.prefixlen = msg.masklen;
  memcpy (&p.u.prefix6, &msg.addr, sizeof (struct hal_in6_addr));

  for (i = 0; i < msg.num; i++)
    {
      if (IPV6_ADDR_ZERO (msg.nh[i].nexthopIP))
        ret = hsl_fib_delete_ipv6uc_prefix ((hsl_fib_id_t)msg.fib_id,
            &p, msg.nh[i].type, msg.nh[i].egressIfindex,
                                            NULL);
      else
        ret = hsl_fib_delete_ipv6uc_prefix ((hsl_fib_id_t)msg.fib_id,
            &p, msg.nh[i].type, msg.nh[i].egressIfindex,
                                            (hsl_ipv6Address_t *) &msg.nh[i].nexthopIP);
      if (ret < 0)
        goto ERR;
    }

  HSL_MSG_PROCESS_RETURN (sock, hdr, 0);
  HSL_FN_EXIT (0);

 ERR:
  /* Reinstall the deleted ones to send one response. */
  for (j = 0; j < i; j++)
    {
      if (IPV6_ADDR_ZERO (msg.nh[j].nexthopIP))
        ret = hsl_fib_add_ipv6uc_prefix ((hsl_fib_id_t)msg.fib_id,
            &p, msg.nh[j].type, msg.nh[j].egressIfindex,
                                         NULL);
      else
        ret = hsl_fib_add_ipv6uc_prefix ((hsl_fib_id_t)msg.fib_id,
            &p, msg.nh[j].type, msg.nh[j].egressIfindex,
                                         (hsl_ipv6Address_t *) &msg.nh[j].nexthopIP);
    }

  HSL_MSG_PROCESS_RETURN (sock, hdr, -1);
  HSL_FN_EXIT (0);
 CLEANUP:
  /* Close socket. */
  hsl_sock_release (sock);

  return -1;
}

/*  HAL_MSG_IPV6_UC_UPDATE message. */
int
hsl_msg_recv_ipv6_uc_update (struct socket *sock, struct hal_nlmsghdr *hdr, char *msgbuf)
{
  return 0;
}
#endif /* HAVE_IPV6 */

/*  HAL_MSG_IF_FIB_BIND message. */
int
hsl_msg_recv_if_bind_fib (struct socket *sock, struct hal_nlmsghdr *hdr, char *msgbuf)
{
  u_char *pnt; 
  u_int32_t size;
  struct hal_msg_if_fib_bind_unbind msg;
  int ret;
  
  pnt = (u_char*)msgbuf;
  size = hdr->nlmsg_len - HAL_NLMSG_ALIGN(HAL_NLMSGHDR_SIZE);
       
  ret =  hsl_msg_decode_if_fib_bind_unbind (&pnt, &size, &msg);
  if (ret < 0)
    goto CLEANUP;
    
  HSL_LOG (HSL_LOG_MSG, HSL_LEVEL_INFO, "Message: HAL IF %d Bind FIB %d\n",
      msg.ifindex, msg.fib_id);

  ret = hsl_ifmgr_if_bind_fib (msg.ifindex, (hsl_fib_id_t)msg.fib_id);

  HSL_MSG_PROCESS_RETURN (sock, hdr, ret);

  return 0;

 CLEANUP:
  /* Close socket. */
  hsl_sock_release (sock);
  
  return -1;
}

/*  HAL_MSG_IF_FIB_UNBIND message. */
int
hsl_msg_recv_if_unbind_fib (struct socket *sock, struct hal_nlmsghdr *hdr, char *msgbuf)
{
  u_char *pnt; 
  u_int32_t size;
  struct hal_msg_if_fib_bind_unbind msg;
  int ret;
  
  pnt = (u_char*)msgbuf;
  size = hdr->nlmsg_len - HAL_NLMSG_ALIGN(HAL_NLMSGHDR_SIZE);
       
  ret =  hsl_msg_decode_if_fib_bind_unbind (&pnt, &size, &msg);
  if (ret < 0)
    goto CLEANUP;
    
  HSL_LOG (HSL_LOG_MSG, HSL_LEVEL_INFO, "Message: HAL IF %d Unbind FIB %d\n",
      msg.ifindex, msg.fib_id);

  ret = hsl_ifmgr_if_unbind_fib (msg.ifindex, (hsl_fib_id_t)msg.fib_id);

  HSL_MSG_PROCESS_RETURN (sock, hdr, ret);

  return 0;

 CLEANUP:
  /* Close socket. */
  hsl_sock_release (sock);
  
  return -1;
}
#endif /* HAVE_L3 */



#ifdef HAVE_L2
/*
  HAL_MSG_IF_L2_INIT message.
*/
int
hsl_msg_recv_if_init_l2(struct socket *sock, struct hal_nlmsghdr *hdr, char *msgbuf)
{
  int ret =0;   
  //ret = hsl_ifmgr_init_policy_l2 ();
  HSL_MSG_PROCESS_RETURN (sock, hdr, ret);
  HSL_FN_EXIT(ret);
}

/* HAL_MSG_BRIDGE_INIT message. */
int
hsl_msg_recv_bridge_init (struct socket *sock, struct hal_nlmsghdr *hdr, char *msgbuf)
{
  HSL_LOG (HSL_LOG_MSG, HSL_LEVEL_INFO, "Message: HAL Bridging initialization\n");
  HSL_MSG_PROCESS_RETURN (sock, hdr, 0);
  return 0;
}

/*  HAL_MSG_BRIDGE_DEINIT message. */
int
hsl_msg_recv_bridge_deinit (struct socket *sock, struct hal_nlmsghdr *hdr, char *msgbuf)
{
  HSL_LOG (HSL_LOG_MSG, HSL_LEVEL_INFO, "Message: HAL Bridging deinitialization\n");
  HSL_MSG_PROCESS_RETURN (sock, hdr, 0);
  return 0;
}

/*  HAL_MSG_BRIDGE_ADD message */
int
hsl_msg_recv_bridge_add (struct socket *sock, struct hal_nlmsghdr *hdr, char *msgbuf)
{
  struct hal_msg_bridge *msg;
  int ret = 0;
  
  HSL_LOG (HSL_LOG_MSG, HSL_LEVEL_INFO, "Message: hsl_msg_recv_bridge_add\n");

  msg = (struct hal_msg_bridge *) msgbuf;
  
  //ret = hsl_bridge_add (msg->name, msg->is_vlan_aware, msg->type, msg->edge);
  HSL_MSG_PROCESS_RETURN (sock, hdr, ret);

  return ret;
}

/*  HAL_MSG_BRIDGE_DELETE message. */
int
hsl_msg_recv_bridge_delete (struct socket *sock, struct hal_nlmsghdr *hdr, char *msgbuf)
{
  struct hal_msg_bridge *msg;
  int ret = 0;
  
  HSL_LOG (HSL_LOG_MSG, HSL_LEVEL_INFO, "Message: hsl_msg_recv_bridge_delete\n");

  msg = (struct hal_msg_bridge *) msgbuf;

//  ret = hsl_bridge_delete (msg->name);
  HSL_MSG_PROCESS_RETURN (sock, hdr, ret);
  return ret;
}

/*  HAL_MSG_BRIDGE_CHANGE_VLAN_TYPE message. */
int
hsl_msg_recv_bridge_change_vlan_type (struct socket *sock,
                                      struct hal_nlmsghdr *hdr, char *msgbuf)
{
  struct hal_msg_bridge *msg;
  int ret=0;
  
  HSL_LOG (HSL_LOG_MSG, HSL_LEVEL_INFO, "Message: hsl_msg_recv_bridge_change_vlan_type\n");

  msg = (struct hal_msg_bridge *) msgbuf;

  //ret = hsl_bridge_change_vlan_type (msg->name, msg->is_vlan_aware, msg->type);
  HSL_MSG_PROCESS_RETURN (sock, hdr, ret);

  return ret;
}

/*  HAL_MSG_BRIDGE_SET_AGEING_TIME message. */
int
hsl_msg_recv_set_ageing_time (struct socket *sock, struct hal_nlmsghdr *hdr, char *msgbuf)
{
  struct hal_msg_bridge_ageing *msg;
  int ret=0;
  
  HSL_LOG (HSL_LOG_MSG, HSL_LEVEL_INFO, "Message: hsl_msg_recv_set_ageing_time\n");

  msg = (struct hal_msg_bridge_ageing *) msgbuf;

  //ret = hsl_bridge_age_timer_set (msg->name, msg->ageing_time);
  HSL_MSG_PROCESS_RETURN (sock, hdr, ret);
  return ret;
}

/* HAL_MSG_BRIDGE_SET_LEARNING message. */
int
hsl_msg_recv_set_learning (struct socket *sock, struct hal_nlmsghdr *hdr, char *msgbuf)
{
  struct hal_msg_bridge_learn *msg;
  int ret=0;
  
  HSL_LOG (HSL_LOG_MSG, HSL_LEVEL_INFO, "Message: hsl_msg_recv_set_learning\n");

  msg = (struct hal_msg_bridge_learn *) msgbuf;

  //ret = hsl_bridge_learning_set (msg->name, msg->learn);
  HSL_MSG_PROCESS_RETURN (sock, hdr, ret);
  return ret;
}

/*  HAL_MSG_BRIDGE_ADD_PORT message. */
int
hsl_msg_recv_bridge_add_port (struct socket *sock, struct hal_nlmsghdr *hdr, char *msgbuf)
{
  struct hal_msg_bridge_port *msg;
  int ret=0;
  
  HSL_LOG (HSL_LOG_MSG, HSL_LEVEL_INFO, "Message: hsl_msg_recv_bridge_add_port\n");

  msg = (struct hal_msg_bridge_port *) msgbuf;
  
  //ret = hsl_bridge_add_port (msg->name, msg->ifindex);
  HSL_MSG_PROCESS_RETURN (sock, hdr, ret);
  return ret;
}

/*  HAL_MSG_BRIDGE_DELETE_PORT message. */
int
hsl_msg_recv_bridge_delete_port (struct socket *sock, struct hal_nlmsghdr *hdr, char *msgbuf)
{
  struct hal_msg_bridge_port *msg;
  int ret=0;
  
  HSL_LOG (HSL_LOG_MSG, HSL_LEVEL_INFO, "Message: hsl_msg_recv_bridge_delete_port\n");

  msg = (struct hal_msg_bridge_port *) msgbuf;
  
  //ret = hsl_bridge_delete_port (msg->name, msg->ifindex);
  HSL_MSG_PROCESS_RETURN (sock, hdr, ret);
  return ret;
}

/*  HAL_MSG_BRIDGE_ADD_INSTANCE message. */
int
hsl_msg_recv_bridge_add_instance (struct socket *sock, struct hal_nlmsghdr *hdr, char *msgbuf)
{
  struct hal_msg_bridge_instance *msg;
  int ret=0;
  
  HSL_LOG (HSL_LOG_MSG, HSL_LEVEL_INFO, "Message: hsl_msg_recv_bridge_add_instance\n");

  msg = (struct hal_msg_bridge_instance *) msgbuf;
  
  //ret = hsl_bridge_add_instance (msg->name, msg->instance);
  HSL_MSG_PROCESS_RETURN (sock, hdr, ret);
  return ret;
}

/*  HAL_MSG_BRIDGE_DELETE_INSTANCE message. */
int
hsl_msg_recv_bridge_delete_instance (struct socket *sock, struct hal_nlmsghdr *hdr, char *msgbuf)
{
  struct hal_msg_bridge_instance *msg;
  int ret=0;
  
  HSL_LOG (HSL_LOG_MSG, HSL_LEVEL_INFO, "Message: hsl_msg_recv_bridge_delete_instance\n");

  msg = (struct hal_msg_bridge_instance *) msgbuf;
  //ret = hsl_bridge_delete_instance (msg->name, msg->instance);
  HSL_MSG_PROCESS_RETURN (sock, hdr, ret);
  return ret;
}

/*  HAL_MSG_BRIDGE_ADD_VLAN_TO_INSTANCE message. */
int
hsl_msg_recv_bridge_add_vlan_to_instance (struct socket *sock, struct hal_nlmsghdr *hdr, char *msgbuf)
{
  struct hal_msg_bridge_vid_instance *msg;
  int ret=0;
  
  HSL_LOG (HSL_LOG_MSG, HSL_LEVEL_INFO, "Message: hsl_msg_recv_bridge_add_vlan_to_instance\n");

  msg = (struct hal_msg_bridge_vid_instance *) msgbuf;
  //ret = hsl_bridge_add_vlan_to_inst (msg->name, msg->instance, msg->vid);
  HSL_MSG_PROCESS_RETURN (sock, hdr, ret);
  return ret;
}

/*  HAL_MSG_BRIDGE_DELETE_VLAN_FROM_INSTANCE message. */
int
hsl_msg_recv_bridge_delete_vlan_from_instance (struct socket *sock, struct hal_nlmsghdr *hdr, char *msgbuf)
{
  struct hal_msg_bridge_vid_instance *msg;
  int ret=0;
  
  HSL_LOG (HSL_LOG_MSG, HSL_LEVEL_INFO, "Message: hsl_msg_recv_bridge_delete_vlan_from_instance\n");

  msg = (struct hal_msg_bridge_vid_instance *) msgbuf;
  //ret = hsl_bridge_delete_vlan_from_inst(msg->name, msg->instance, msg->vid);
  HSL_MSG_PROCESS_RETURN (sock, hdr, ret);
  return ret;
}

#if defined (HAVE_STPD) || defined (HAVE_RSTPD) || defined (HAVE_MSTPD)
int hsl_msg_recv_set_port_state(struct socket *sock, struct hal_nlmsghdr *hdr, char *msgbuf)
{
  int ret=0;
  struct hal_msg_stp_port_state *msg =
    (struct hal_msg_stp_port_state *)msgbuf;
  
  HSL_LOG (HSL_LOG_MSG, HSL_LEVEL_INFO, "Message: hsl_msg_recv_set_port_state\n");
  
  //ret = hsl_bridge_set_stp_port_state(msg->name,msg->port_ifindex,msg->instance,
  //                                    msg->port_state);
  HSL_MSG_PROCESS_RETURN (sock, hdr, ret);
  return 0;
}
#endif  /* defined (HAVE_STPD) || defined (HAVE_RSTPD) || defined (HAVE_MSTPD) */

#ifdef HAVE_VLAN

/*  HAL_MSG_VLAN_INIT message. */
int
hsl_msg_recv_vlan_init (struct socket *sock, struct hal_nlmsghdr *hdr, char *msgbuf)
{
  HSL_LOG (HSL_LOG_MSG, HSL_LEVEL_INFO, "Message: HAL VLAN initialization\n");
  HSL_MSG_PROCESS_RETURN (sock, hdr, 0);
  return 0;
}

/*  HAL_MSG_VLAN_DEINIT message. */
int
hsl_msg_recv_vlan_deinit (struct socket *sock, struct hal_nlmsghdr *hdr, char *msgbuf)
{
  HSL_LOG (HSL_LOG_MSG, HSL_LEVEL_INFO, "Message: HAL VLAN deinitialization\n");
  HSL_MSG_PROCESS_RETURN (sock, hdr, 0);
  return 0;
}

/*  HAL_MSG_VLAN_ADD message. */
int
hsl_msg_recv_vlan_add (struct socket *sock, struct hal_nlmsghdr *hdr, char *msgbuf)
{
  struct hal_msg_vlan *msg;
  int ret=0;

  msg = (struct hal_msg_vlan *) msgbuf;
  //ret = hsl_vlan_add (msg->name, msg->type, msg->vid);

  if (ret == HSL_ERR_BRIDGE_VLAN_RESERVED_IN_HW)
    HSL_MSG_PROCESS_RETURN_RET (sock, hdr, ret);
  else
    HSL_MSG_PROCESS_RETURN (sock, hdr, ret);

  return ret;
}

/*  HAL_MSG_VLAN_DELETE message. */
int
hsl_msg_recv_vlan_delete (struct socket *sock, struct hal_nlmsghdr *hdr, char *msgbuf)
{
  struct hal_msg_vlan *msg;
  int ret=0;

  msg = (struct hal_msg_vlan *) msgbuf;
  //ret = hsl_vlan_delete (msg->name, msg->vid);
  HSL_MSG_PROCESS_RETURN (sock, hdr, ret);
  return ret;
}

/*  HAL_MSG_VLAN_SET_PORT_TYPE message. */
int
hsl_msg_recv_vlan_set_port_type (struct socket *sock, struct hal_nlmsghdr *hdr, char *msgbuf)
{
  struct hal_msg_vlan_port_type *msg;
  int ret=0;

  msg = (struct hal_msg_vlan_port_type *) msgbuf;
  //ret = hsl_vlan_set_port_type (msg->name, msg->ifindex, msg->port_type,
    //                            msg->sub_port_type, msg->acceptable_frame_type,
      //                          msg->enable_ingress_filter);
  HSL_MSG_PROCESS_RETURN (sock, hdr, ret);
  return ret;
}

/* HAL_MSG_VLAN_SET_DEFAULT_PVID message. */
int
hsl_msg_recv_vlan_set_default_pvid (struct socket *sock, struct hal_nlmsghdr *hdr, char *msgbuf)
{
  struct hal_msg_vlan_port *msg;
  int ret=0;

  msg = (struct hal_msg_vlan_port *) msgbuf;
  //ret = hsl_vlan_set_default_pvid (msg->name, msg->ifindex, msg->vid, msg->egress);
  HSL_MSG_PROCESS_RETURN (sock, hdr, ret);
  return ret;
}

/*  HAL_MSG_VLAN_ADD_VID_TO_PORT message. */
int
hsl_msg_recv_vlan_add_vid_to_port (struct socket *sock, struct hal_nlmsghdr *hdr, char *msgbuf)
{
  struct hal_msg_vlan_port *msg;
  int ret=0;

  msg = (struct hal_msg_vlan_port *) msgbuf;
  //ret = hsl_vlan_add_vid_to_port (msg->name, msg->ifindex, msg->vid, msg->egress);
  HSL_MSG_PROCESS_RETURN (sock, hdr, ret);
  return ret;
}

/*  HAL_MSG_DELETE_VID_FROM_PORT message. */
int
hsl_msg_recv_vlan_delete_vid_from_port (struct socket *sock, struct hal_nlmsghdr *hdr, char *msgbuf)
{
  struct hal_msg_vlan_port *msg;
  int ret=0;

  msg = (struct hal_msg_vlan_port *) msgbuf;
  //ret = hsl_vlan_delete_vid_from_port(msg->name, msg->ifindex, msg->vid);
  HSL_MSG_PROCESS_RETURN (sock, hdr, ret);
  return ret;
}

#endif /* HAVE_VLAN */

#ifdef HAVE_PVLAN
/* HAL_MSG_PVLAN_SET_VLAN_TYPE message */
int
hsl_msg_recv_pvlan_set_vlan_type (struct socket *sock, struct hal_nlmsghdr *hdr, char *msgbuf)
{
  struct hal_msg_pvlan *msg;
  int ret;

  msg = (struct hal_msg_pvlan *) msgbuf;
  ret = hsl_pvlan_set_vlan_type(msg->name, msg->vid, msg->vlan_type);

  HSL_MSG_PROCESS_RETURN (sock, hdr, ret);
  return ret;
}

/* HAL_MSG_PVLAN_VLAN_ASSOCIATE message */
int
hsl_msg_recv_pvlan_vlan_associate (struct socket *sock, struct hal_nlmsghdr *hdr, char *msgbuf)
{
  struct hal_msg_pvlan_association *msg;
  int ret;

  msg = (struct hal_msg_pvlan_association *) msgbuf;
  ret = hsl_pvlan_add_vlan_association (msg->name, msg->pvid, msg->svid);

  HSL_MSG_PROCESS_RETURN (sock, hdr, ret);
  return ret;
}

/* HAL_MSG_PVLAN_VLAN_DISSOCIATE message */
int
hsl_msg_recv_pvlan_vlan_dissociate (struct socket *sock, struct hal_nlmsghdr *hdr, char *msgbuf)
{
  struct hal_msg_pvlan_association *msg;
  int ret;

  msg = (struct hal_msg_pvlan_association *) msgbuf;
  ret = hsl_pvlan_remove_vlan_association (msg->name, msg->pvid, msg->svid);

  HSL_MSG_PROCESS_RETURN (sock, hdr, ret);
  return ret;
}

/* HAL_MSG_PVLAN_PORT_ADD message */
int
hsl_msg_recv_pvlan_port_add (struct socket *sock, struct hal_nlmsghdr *hdr, char *msgbuf)
{
  struct hal_msg_pvlan_port_set *msg;
  int ret;

  msg = (struct hal_msg_pvlan_port_set *) msgbuf;
  ret = hsl_pvlan_port_add_association (msg->name, msg->ifindex,
                                        msg->pvid, msg->svid);

  HSL_MSG_PROCESS_RETURN (sock, hdr, ret);
  return ret;
}

/* HAL_MSG_PVLAN_PORT_DELETE message */
int
hsl_msg_recv_pvlan_port_delete (struct socket *sock, struct hal_nlmsghdr *hdr, char *msgbuf)
{
  struct hal_msg_pvlan_port_set *msg;
  int ret;

  msg = (struct hal_msg_pvlan_port_set *) msgbuf;
  ret = hsl_pvlan_port_delete_association (msg->name, msg->ifindex,
                                           msg->pvid, msg->svid);

  HSL_MSG_PROCESS_RETURN (sock, hdr, ret);
  return ret;
}

/* HAL_MSG_PVLAN_SET_PORT_MODE message */
int
hsl_msg_recv_pvlan_set_port_mode (struct socket *sock, struct hal_nlmsghdr *hdr, char *msgbuf)
{
  struct hal_msg_pvlan_port_mode *msg;
  int ret;

  msg = (struct hal_msg_pvlan_port_mode *) msgbuf;
  ret = hsl_pvlan_set_port_mode (msg->name, msg->ifindex, msg->port_mode);

  HSL_MSG_PROCESS_RETURN (sock, hdr, ret);
  return ret;
}
#endif /* HAVE_PVLAN */

#ifdef HAVE_VLAN_CLASS
/*  HAL_MSG_VLAN_CLASSIFIER_ADD message. */
int
hsl_msg_recv_vlan_classifier_add (struct socket *sock, struct hal_nlmsghdr *hdr, 
                                  char *msgbuf)
{
  u_char *pnt;
  u_int32_t size;
  struct hal_msg_vlan_classifier_rule msg;
  int ret;

  HSL_FN_ENTER(); 
  HSL_LOG (HSL_LOG_MSG, HSL_LEVEL_INFO, "Message: HAL vlan classifier add\n");

  pnt = (u_char*)msgbuf;
  size = hdr->nlmsg_len - HAL_NLMSG_ALIGN(HAL_NLMSGHDR_SIZE);

  ret = hsl_msg_decode_vlan_classifier_rule(&pnt, &size, &msg);
  if (ret < 0)
    goto CLEANUP;

  ret = hsl_vlan_classifier_add (&msg);

  HSL_MSG_PROCESS_RETURN (sock, hdr, ret);
  HSL_FN_EXIT(ret);
  
 CLEANUP:
  /* Close socket. */
  hsl_sock_release(sock);
  HSL_FN_EXIT(STATUS_ERROR);
}


/*  HAL_MSG_VLAN_CLASSIFIER_DELETE message. */
int
hsl_msg_recv_vlan_classifier_delete (struct socket *sock, struct hal_nlmsghdr *hdr, 
                                     char *msgbuf)
{
  u_char *pnt;
  u_int32_t size;
  struct hal_msg_vlan_classifier_rule msg;
  int ret;

  HSL_FN_ENTER();
  HSL_LOG (HSL_LOG_MSG, HSL_LEVEL_INFO, "Message: HAL vlan classifier delete\n");

  HSL_FN_ENTER ();

  pnt = (u_char*)msgbuf;
  size = hdr->nlmsg_len - HAL_NLMSG_ALIGN(HAL_NLMSGHDR_SIZE);

  ret = hsl_msg_decode_vlan_classifier_rule(&pnt, &size, &msg);
  if (ret < 0)
    goto CLEANUP;

  ret = hsl_vlan_classifier_delete (&msg);

  HSL_MSG_PROCESS_RETURN (sock, hdr, ret);
  HSL_FN_EXIT(ret);

 CLEANUP:
  /* Close socket. */
  hsl_sock_release(sock);
  HSL_FN_EXIT(STATUS_ERROR);
}
#endif /* HAVE_VLAN_CLASS */

#ifdef HAVE_VLAN_STACK

int 
hsl_msg_recv_vlan_stacking_enable (struct socket *sock, struct hal_nlmsghdr *hdr, char *msgbuf)
{
  struct hal_msg_vlan_stack *msg = (struct hal_msg_vlan_stack *)msgbuf;
  int ret = -1;

  ret = hsl_bcm_vlan_stacking_enable (msg->ifindex, msg->ethtype,
                                      msg->stackmode);
  
  HSL_MSG_PROCESS_RETURN (sock, hdr, ret);

  return ret; 
}


int 
hsl_msg_recv_vlan_stacking_disable (struct socket *sock, struct hal_nlmsghdr *hdr, char *msgbuf)
{
  struct hal_msg_vlan_stack *msg = (struct hal_msg_vlan_stack *)msgbuf;
  int ret = -1;

  ret = hsl_bcm_provider_vlan_disable (msg->ifindex);
  
  HSL_MSG_PROCESS_RETURN (sock, hdr, ret);

  return ret; 
}

#endif /* HAVE_VLAN_STACK */

#ifdef HAVE_PROVIDER_BRIDGE

int
hsl_msg_recv_cvlan_reg_tab_add (struct socket *sock, struct hal_nlmsghdr *hdr, char *msgbuf)
{
  struct hal_msg_cvlan_reg_tab *msg = (struct hal_msg_cvlan_reg_tab *)msgbuf;
  int ret = 0;

  //ret = hsl_bcm_cvlan_reg_tab_add (msg->ifindex, msg->cvid, msg->svid);

  HSL_MSG_PROCESS_RETURN (sock, hdr, ret);

  return ret;
}

int
hsl_msg_recv_cvlan_reg_tab_del (struct socket *sock, struct hal_nlmsghdr *hdr, char *msgbuf)
{
  struct hal_msg_cvlan_reg_tab *msg = (struct hal_msg_cvlan_reg_tab *)msgbuf;
  int ret = -1;

  ret = hsl_bcm_cvlan_reg_tab_del (msg->ifindex, msg->cvid, msg->svid);

  HSL_MSG_PROCESS_RETURN (sock, hdr, ret);

  return ret;
}

int
hsl_msg_recv_protocol_process (struct socket *sock, struct hal_nlmsghdr *hdr, char *msgbuf)
{
  struct hal_msg_proto_process *msg = (struct hal_msg_proto_process *) msgbuf;
  int ret = -1;

  ret = hsl_bcm_protocol_process (msg->ifindex, msg->proto,
                                  msg->proto_process, msg->vid);

  HSL_MSG_PROCESS_RETURN (sock, hdr, ret);

  return ret;
}

#endif /* HAVE_PROVIDER_BRIDGE */

#ifdef HAVE_DCB
s_int32_t
hsl_msg_recv_dcb_init (struct socket *sock, struct hal_nlmsghdr *hdr,
                       char *msgbuf)
{
  HSL_MSG_PROCESS_RETURN (sock, hdr, 0);
}

s_int32_t
hsl_msg_recv_dcb_deinit (struct socket *sock, struct hal_nlmsghdr *hdr,
                         char *msgbuf)
{
  HSL_MSG_PROCESS_RETURN (sock, hdr, 0);
}

s_int32_t
hsl_msg_recv_dcb_enable (struct socket *sock, struct hal_nlmsghdr *hdr,
                         char *msgbuf)
{
  HSL_MSG_PROCESS_RETURN (sock, hdr, 0);
}

s_int32_t
hsl_msg_recv_dcb_disable (struct socket *sock, struct hal_nlmsghdr *hdr,
                          char *msgbuf)
{
  HSL_MSG_PROCESS_RETURN (sock, hdr, 0);
}

s_int32_t
hsl_msg_recv_dcb_ets_enable (struct socket *sock, struct hal_nlmsghdr *hdr,
                             char *msgbuf)
{
  HSL_MSG_PROCESS_RETURN (sock, hdr, 0);
}

s_int32_t
hsl_msg_recv_dcb_ets_disable (struct socket *sock, struct hal_nlmsghdr *hdr,
                              char *msgbuf)
{
  HSL_MSG_PROCESS_RETURN (sock, hdr, 0);
}

s_int32_t
hsl_msg_recv_dcb_if_enable (struct socket *sock, struct hal_nlmsghdr *hdr,
                            char *msgbuf)
{
  HSL_MSG_PROCESS_RETURN (sock, hdr, 0);
}

s_int32_t
hsl_msg_recv_dcb_if_disable (struct socket *sock, struct hal_nlmsghdr *hdr,
                            char *msgbuf)
{
  HSL_MSG_PROCESS_RETURN (sock, hdr, 0);
}

s_int32_t
hsl_msg_recv_dcb_ets_if_enable (struct socket *sock, struct hal_nlmsghdr *hdr,
                                char *msgbuf)
{
  HSL_MSG_PROCESS_RETURN (sock, hdr, 0);
}

s_int32_t
hsl_msg_recv_dcb_ets_if_disable (struct socket *sock, struct hal_nlmsghdr *hdr,
                                 char *msgbuf)
{
  HSL_MSG_PROCESS_RETURN (sock, hdr, 0);
}

s_int32_t
hsl_msg_recv_dcb_ets_select_mode (struct socket *sock, struct hal_nlmsghdr *hdr,
                                  char *msgbuf)
{
  HSL_MSG_PROCESS_RETURN (sock, hdr, 0);
}

s_int32_t
hsl_msg_recv_dcb_ets_add_pri_to_tcg (struct socket *sock, 
                                     struct hal_nlmsghdr *hdr,
                                     char *msgbuf)
{
  HSL_MSG_PROCESS_RETURN (sock, hdr, 0);
}

s_int32_t
hsl_msg_recv_dcb_ets_remove_pri_from_tcg (struct socket *sock,
                                          struct hal_nlmsghdr *hdr,
                                          char *msgbuf)
{
  HSL_MSG_PROCESS_RETURN (sock, hdr, 0);
}

s_int32_t
hsl_msg_recv_dcb_ets_tcg_bw_set (struct socket *sock, struct hal_nlmsghdr *hdr,
                                 char *msgbuf)
{
  HSL_MSG_PROCESS_RETURN (sock, hdr, 0);
}

s_int32_t
hsl_msg_recv_dcb_ets_app_pri_set (struct socket *sock, struct hal_nlmsghdr *hdr,
                                  char *msgbuf)
{
  HSL_MSG_PROCESS_RETURN (sock, hdr, 0);
}

s_int32_t
hsl_msg_recv_dcb_ets_app_pri_unset (struct socket *sock, 
                                    struct hal_nlmsghdr *hdr,
                                    char *msgbuf)
{
  HSL_MSG_PROCESS_RETURN (sock, hdr, 0);
}

s_int32_t
hsl_msg_recv_dcb_pfc_enable (struct socket *sock,
                             struct hal_nlmsghdr *hdr,
                             char *msgbuf)
{
  HSL_MSG_PROCESS_RETURN (sock, hdr, 0);
}

s_int32_t
hsl_msg_recv_dcb_pfc_disable (struct socket *sock,
                              struct hal_nlmsghdr *hdr,
                              char *msgbuf)
{
  HSL_MSG_PROCESS_RETURN (sock, hdr, 0);
}

s_int32_t
hsl_msg_recv_dcb_pfc_if_enable (struct socket *sock,
                                struct hal_nlmsghdr *hdr,
                                char *msgbuf)
{
  HSL_MSG_PROCESS_RETURN (sock, hdr, 0);
}

s_int32_t
hsl_msg_recv_dcb_pfc_select_mode (struct socket *sock,
                                  struct hal_nlmsghdr *hdr,
                                  char *msgbuf)
{
  HSL_MSG_PROCESS_RETURN (sock, hdr, 0);
}

s_int32_t
hsl_msg_recv_dcb_pfc_set_cap (struct socket *sock,
                              struct hal_nlmsghdr *hdr,
                              char *msgbuf)
{
  HSL_MSG_PROCESS_RETURN (sock, hdr, 0);
}

s_int32_t
hsl_msg_recv_dcb_pfc_set_lda (struct socket *sock,
                              struct hal_nlmsghdr *hdr,
                              char *msgbuf)
{
  HSL_MSG_PROCESS_RETURN (sock, hdr, 0);
}

#endif /* HAVE_DCB */

/*  HAL_MSG_FLOW_CONTROL_INIT message. */
int
hsl_msg_recv_flow_control_init (struct socket *sock, struct hal_nlmsghdr *hdr, char *msgbuf)
{
  int ret=0;

  HSL_LOG (HSL_LOG_MSG, HSL_LEVEL_INFO, "Message: HAL Flow Control initialization\n");

  //ret = hsl_bcm_flowcontrol_init ();
  HSL_MSG_PROCESS_RETURN (sock, hdr, ret);

  return 0;
}

/*  HAL_MSG_FLOW_CONTROL_DEINIT message. */
int
hsl_msg_recv_flow_control_deinit (struct socket *sock, struct hal_nlmsghdr *hdr, char *msgbuf)
{
  int ret;

  HSL_LOG (HSL_LOG_MSG, HSL_LEVEL_INFO, "Message: HAL Flow Control deinitialization\n");

  ret = hsl_bcm_flowcontrol_deinit ();
  HSL_MSG_PROCESS_RETURN (sock, hdr, ret);

  return 0;
}

/*  HAL_MSG_FLOW_CONTROL_SET message. */
int
hsl_msg_recv_flow_control_set (struct socket *sock, struct hal_nlmsghdr *hdr, char *msgbuf)
{
  struct hal_msg_flow_control *msg;
  int ret;

  msg = (struct hal_msg_flow_control *) msgbuf;
  ret = hsl_bridge_set_flowcontrol(msg->ifindex, msg->direction);
  HSL_MSG_PROCESS_RETURN (sock, hdr, ret);
  return ret;
}

/*  HAL_MSG_FLOW_CONTROL_STATISTICS message. */
int
hsl_msg_recv_flow_control_statistics (struct socket *sock, struct hal_nlmsghdr *hdr, char *msgbuf)
{
  u_int32_t *ifindex;
  struct hal_msg_flow_control_stats resp;
  int ret;

  ifindex = (u_int32_t *) msgbuf;
  ret = hsl_bridge_flowcontrol_statistics (*ifindex, &resp.direction, &resp.rxpause, &resp.txpause);
  if (ret < 0)
    {
      HSL_MSG_PROCESS_RETURN (sock, hdr, ret);
    }
  else
    hsl_sock_post_msg (sock, HAL_MSG_FLOW_CONTROL_STATISTICS, 0, hdr->nlmsg_seq, (char *)&resp, sizeof (struct hal_msg_flow_control_stats));

  return ret;
}


/*  HAL_MSG_L2_QOS_INIT message. */
int
hsl_msg_recv_l2_qos_init (struct socket *sock, struct hal_nlmsghdr * hdr, char *msgbuf)
{
  HSL_LOG (HSL_LOG_MSG, HSL_LEVEL_INFO, "Message: HAL L2 QoS initialization\n");

  if (hdr->nlmsg_flags & HAL_NLM_F_ACK)
    hsl_sock_post_ack (sock, hdr, 0, 0);

  return 0;
}

/*  HAL_MSG_L2_QOS_DEINIT message. */
int
hsl_msg_recv_l2_qos_deinit (struct socket *sock, struct hal_nlmsghdr *hdr, char *msgbuf)
{
  HSL_LOG (HSL_LOG_MSG, HSL_LEVEL_INFO, "Message: HAL L2 QoS initialization\n");

  if (hdr->nlmsg_flags & HAL_NLM_F_ACK)
    hsl_sock_post_ack (sock, hdr, 0, 0);

  return 0;
}

#ifdef HAVE_QOS

/*  HAL_MSG_L2_QOS_DEFAULT_USER_PRIORITY_SET message. */
int
hsl_msg_recv_l2_qos_default_user_priority_set (struct socket *sock, struct hal_nlmsghdr *hdr, char *msgbuf)
{
  struct hal_msg_qos_set_default_cos *msg =
    (struct hal_msg_qos_set_default_cos *)msgbuf;
  int ret;

  ret = hsl_bcm_qos_set_default_cos (msg->ifindex, msg->cos_value);

  HSL_MSG_PROCESS_RETURN (sock, hdr, ret);
  return ret;
}

#endif /* HAVE_QOS */
/*  HAL_MSG_L2_QOS_DEFAULT_USER_PRIORITY_GET message. */
int
hsl_msg_recv_l2_qos_default_user_priority_get (struct socket *sock, struct hal_nlmsghdr *hdr, char *msgbuf)
{
  return 0;
}

/*  HAL_MSG_L2_QOS_REGEN_USER_PRIORITY_SET message. */
int
hsl_msg_recv_l2_qos_regen_user_priority_set (struct socket *sock, 
                                             struct hal_nlmsghdr *hdr, char *msgbuf)
{
  struct hal_msg_l2_qos_regen_usr_pri *msg =
    (struct hal_msg_l2_qos_regen_usr_pri *)msgbuf;
  int ret;

  ret = hsl_bcm_qos_set_regen_user_priority (msg->ifindex, msg->recvd_user_priority,
                                             msg->regen_user_priority);

  HSL_MSG_PROCESS_RETURN (sock, hdr, ret);
  return ret;
}

/*  HAL_MSG_L2_QOS_REGEN_USER_PRIORITY_GET message. */
int
hsl_msg_recv_l2_qos_regen_user_priority_get (struct socket *sock, struct hal_nlmsghdr *hdr, char *msgbuf)
{
  return 0;
}

/*  HAL_MSG_L2_QOS_TRAFFIC_CLASS_SET message. */
int
hsl_msg_recv_l2_qos_traffic_class_set (struct socket *sock, 
                                       struct hal_nlmsghdr *hdr, char *msgbuf)
{
  struct hal_msg_l2_qos_traffic_class *msg = 
         (struct hal_msg_l2_qos_traffic_class *)msgbuf;
  int ret;
  
  ret = hsl_bcm_qos_port_traffic_class_set (msg->ifindex, msg->user_priority, 
                                       msg->traffic_class_value);

  HSL_MSG_PROCESS_RETURN (sock, hdr, ret);
  return ret;
}

/* HAL_MSG_L2_QOS_NUM_COSQ_GET */
int
hsl_msg_recv_l2_qos_num_cosq_get (struct socket *sock, struct hal_nlmsghdr *hdr, char *msgbuf)
{
  int ret=0;
  unsigned int num;

  //ret = hsl_bcm_l2_get_num_cosq (&num);
  if (ret < 0)
  {
      HSL_MSG_PROCESS_RETURN (sock, hdr, ret);
  }
  else
  {
     hsl_sock_post_msg (sock, hdr->nlmsg_type, hdr->nlmsg_seq, 0,
              (char *)&num, sizeof (u_int32_t));
  }

  return 0;
}

/* HAL_MSG_L2_QOS_NUM_COSQ_SET */
int
hsl_msg_recv_l2_qos_num_cosq_set (struct socket *sock, struct hal_nlmsghdr *hdr, char *msgbuf)
{
  int ret = -1;

  /* Currently the BCM does not support setting of the NUM_COSQ at runtime */
  HSL_MSG_PROCESS_RETURN (sock, hdr, ret);

  return ret;
}

/*  HAL_MSG_RATELIMIT_INIT message. */
int
hsl_msg_recv_ratelimit_init (struct socket *sock, struct hal_nlmsghdr *hdr, char *msgbuf)
{
  HSL_LOG (HSL_LOG_MSG, HSL_LEVEL_INFO, "Message: HAL Ratelimit initialization\n");
  HSL_MSG_PROCESS_RETURN (sock, hdr, 0);
  return 0;
}

/*  HAL_MSG_RATELIMIT_DEINIT message. */
int
hsl_msg_recv_ratelimit_deinit (struct socket *sock, struct hal_nlmsghdr *hdr, char *msgbuf)
{
  HSL_LOG (HSL_LOG_MSG, HSL_LEVEL_INFO, "Message: HAL Ratelimit deinitialization\n");
  HSL_MSG_PROCESS_RETURN (sock, hdr, 0);
  return 0;
}

/*  HAL_MSG_RATELIMIT_BCAST message. */
int
hsl_msg_recv_ratelimit_bcast (struct socket *sock, struct hal_nlmsghdr *hdr, char *msgbuf)
{
  struct hal_msg_ratelimit *msg;
  int ret;
  struct hsl_if * ifp;

  msg = (struct hal_msg_ratelimit *) msgbuf;
  ifp = hsl_ifmgr_lookup_by_index ( msg->ifindex );
  //ret = hsl_bridge_ratelimit_bcast (msg->ifindex, msg->level, msg->fraction);
  //ret = hsl_bcm_ratelimit_bcast( ifp, msg->level, msg->fraction);//djg20140101
  ret = hsl_bcm_ratebwlimit_set (ifp, BCM_RATE_BCAST, msg->level, msg->fraction);//djg20140101
  HSL_MSG_PROCESS_RETURN (sock, hdr, ret);
  return ret;  
}

/*  HAL_MSG_BCAST_DISCARDS_GET message. */
int
hsl_msg_recv_bcast_discards_get (struct socket *sock, struct hal_nlmsghdr *hdr, char *msgbuf)
{
  u_int32_t *ifindex;
  int discards = 0;
  int ret;

  ifindex = (u_int32_t *) msgbuf;
  ret = hsl_bridge_ratelimit_get_bcast_discards (*ifindex, &discards);
  if (ret < 0)
    {
      HSL_MSG_PROCESS_RETURN (sock, hdr, ret);
    }
  else
    hsl_sock_post_msg (sock, HAL_MSG_RATELIMIT_BCAST_DISCARDS_GET, 0, hdr->nlmsg_seq, (char *)&discards, sizeof (int));

  return 0;
}

/*  HAL_MSG_RATELIMIT_MCAST message. */
int
hsl_msg_recv_ratelimit_mcast (struct socket *sock, struct hal_nlmsghdr *hdr, char *msgbuf)
{
  struct hal_msg_ratelimit *msg;
  int ret;
  struct hsl_if * ifp;

  msg = (struct hal_msg_ratelimit *) msgbuf;
  ifp = hsl_ifmgr_lookup_by_index ( msg->ifindex );
  //ret = hsl_bridge_ratelimit_mcast (msg->ifindex, msg->level, msg->fraction);
  //ret = hsl_bcm_ratelimit_mcast( ifp, msg->level, msg->fraction);//djg20140101
  ret = hsl_bcm_ratebwlimit_set (ifp, BCM_RATE_MCAST, msg->level, msg->fraction);//djg20140101
  HSL_MSG_PROCESS_RETURN (sock, hdr, ret);
  return ret;  
}

/*  HAL_MSG_MCAST_DISCARDS_GET message. */
int
hsl_msg_recv_mcast_discards_get (struct socket *sock, struct hal_nlmsghdr *hdr, char *msgbuf)
{
  u_int32_t *ifindex;
  int discards = 0;
  int ret;

  ifindex = (u_int32_t *) msgbuf;
  ret = hsl_bridge_ratelimit_get_bcast_discards (*ifindex, &discards);
  if (ret < 0)
    {
      HSL_MSG_PROCESS_RETURN (sock, hdr, ret);
    }
  else
    hsl_sock_post_msg (sock, HAL_MSG_RATELIMIT_MCAST_DISCARDS_GET, 0, hdr->nlmsg_seq, (char *)&discards, sizeof (int));

  return 0;
}

/*  HAL_MSG_RATELIMIT_DLF_BCAST message. */
int
hsl_msg_recv_ratelimit_dlf_bcast (struct socket *sock, struct hal_nlmsghdr *hdr, char *msgbuf)
{
  struct hal_msg_ratelimit *msg;
  int ret;
  struct hsl_if * ifp;

  msg = (struct hal_msg_ratelimit *) msgbuf;
  ifp = hsl_ifmgr_lookup_by_index ( msg->ifindex );
  //ret = hsl_bridge_ratelimit_dlf_bcast (msg->ifindex, msg->level, msg->fraction);
  //ret = hsl_bcm_ratelimit_dlf_bcast( ifp, msg->level, msg->fraction);//djg20140101
  ret = hsl_bcm_ratebwlimit_set (ifp, BCM_RATE_DLF, msg->level, msg->fraction);//djg20140101
  HSL_MSG_PROCESS_RETURN (sock, hdr, ret);
  return ret;  
}

/*  HAL_MSG_RATELIMIT_DLF_BCAST_DISCARDS_GET message. */
int
hsl_msg_recv_dlf_bcast_discards_get (struct socket *sock, struct hal_nlmsghdr *hdr, char *msgbuf)
{
  u_int32_t *ifindex;
  int discards = 0;
  int ret;

  ifindex = (u_int32_t *) msgbuf;
  ret = hsl_bridge_ratelimit_get_dlf_bcast_discards (*ifindex, &discards);
  if (ret < 0)
    {
      HSL_MSG_PROCESS_RETURN (sock, hdr, ret);
    }
  else
    hsl_sock_post_msg (sock, HAL_MSG_RATELIMIT_DLF_BCAST_DISCARDS_GET, 0, hdr->nlmsg_seq, (char *)&discards, sizeof (int));

  return 0;
}

#ifdef HAVE_IGMP_SNOOP
/*  HAL_MSG_IGMP_SNOOPING_INIT message. */
int
hsl_recv_msg_igmp_snooping_init (struct socket *sock, struct hal_nlmsghdr *hdr, char *msgbuf)
{
  HSL_LOG (HSL_LOG_MSG, HSL_LEVEL_INFO, "Message: HAL IGMP Snooping initialization\n");
  HSL_MSG_PROCESS_RETURN (sock, hdr, 0);
  return 0;
}

/*  HAL_MSG_IGMP_SNOOPING_DEINIT message. */
int
hsl_recv_msg_igmp_snooping_deinit (struct socket *sock, struct hal_nlmsghdr *hdr, char *msgbuf)
{
  HSL_LOG (HSL_LOG_MSG, HSL_LEVEL_INFO, "Message: HAL IGMP Snooping deinitialization\n");
  HSL_MSG_PROCESS_RETURN (sock, hdr, 0);
  return 0;
}

/*  HAL_MSG_IGMP_SNOOPING_ENABLE message. */
int
hsl_msg_recv_igmp_snooping_enable (struct socket *sock, struct hal_nlmsghdr *hdr, char *msgbuf)
{
  u_char *pnt;
  u_int32_t size;
  struct hal_msg_igs_bridge msg;
  int ret;

  HSL_FN_ENTER(); 

  HSL_LOG (HSL_LOG_MSG, HSL_LEVEL_INFO, "Message: HAL message igmp snooping enable\n");

  pnt = (u_char*)msgbuf;
  size = hdr->nlmsg_len - HAL_NLMSG_ALIGN(HAL_NLMSGHDR_SIZE);
      
  ret = hsl_msg_decode_igs_bridge (&pnt, &size, &msg);
  if (ret < 0)
    goto CLEANUP;

  ret = hsl_bridge_enable_igmp_snooping(msg.bridge_name);

  HSL_MSG_PROCESS_RETURN (sock, hdr, ret);
  HSL_FN_EXIT(ret);

 CLEANUP:
  /* Close socket. */
  hsl_sock_release (sock);
  HSL_FN_EXIT(-1);
}

/*  HAL_MSG_IGMP_SNOOPING_DISABLE message. */
int
hsl_msg_igmp_snooping_disable (struct socket *sock, struct hal_nlmsghdr *hdr, char *msgbuf)
{
  u_char *pnt;
  u_int32_t size;
  struct hal_msg_igs_bridge msg;
  int ret;

  HSL_FN_ENTER(); 

  HSL_LOG (HSL_LOG_MSG, HSL_LEVEL_INFO, "Message: HAL message igmp snooping disable\n");

  pnt = (u_char*)msgbuf;
  size = hdr->nlmsg_len - HAL_NLMSG_ALIGN(HAL_NLMSGHDR_SIZE);
      
  ret = hsl_msg_decode_igs_bridge (&pnt, &size, &msg);
  if (ret < 0)
    goto CLEANUP;

  ret = hsl_bridge_disable_igmp_snooping(msg.bridge_name);

  HSL_MSG_PROCESS_RETURN (sock, hdr, ret);
  HSL_FN_EXIT(ret);

 CLEANUP:
  /* Close socket. */
  hsl_sock_release (sock);
  HSL_FN_EXIT(ret);
}

/*  HAL_MSG_IGMP_SNOOPING_ENTRY_ADD message. */
int
hsl_msg_igmp_snooping_add_entry(struct socket *sock, struct hal_nlmsghdr *hdr, char *msgbuf)
{
  u_char *pnt;
  u_int32_t size;
  struct hal_msg_igmp_snoop_entry msg;
  int ret;

  HSL_FN_ENTER(); 

  HSL_LOG (HSL_LOG_MSG, HSL_LEVEL_INFO, "Message: HAL message igmp snooping add entry\n");

  pnt = (u_char*)msgbuf;
  size = hdr->nlmsg_len - HAL_NLMSG_ALIGN(HAL_NLMSGHDR_SIZE);
      
  ret = hsl_msg_decode_igs_entry(&pnt, &size, &msg, hsl_malloc);
  if (ret < 0)
    goto CLEANUP;

  ret = hsl_mc_v4_igs_add_entry(msg.bridge_name, msg.src.s_addr,
                                msg.group.s_addr, msg.vid, msg.count,
                                msg.ifindexes, msg.is_exclude);

     
  if(msg.ifindexes) oss_free(msg.ifindexes,OSS_MEM_HEAP);

  HSL_MSG_PROCESS_RETURN (sock, hdr, ret);
  HSL_FN_EXIT(ret);

 CLEANUP:
  /* Close socket. */
  hsl_sock_release (sock);
  HSL_FN_EXIT(ret);
}

/*  HAL_MSG_IGMP_SNOOPING_ENTRY_DEL message. */
int
hsl_msg_igmp_snooping_del_entry(struct socket *sock, struct hal_nlmsghdr *hdr, char *msgbuf)
{
  u_char *pnt;
  u_int32_t size;
  struct hal_msg_igmp_snoop_entry msg;
  int ret;

  HSL_FN_ENTER(); 

  HSL_LOG (HSL_LOG_MSG, HSL_LEVEL_INFO, "Message: HAL message igmp snooping delete entry\n");

  pnt = (u_char*)msgbuf;
  size = hdr->nlmsg_len - HAL_NLMSG_ALIGN(HAL_NLMSGHDR_SIZE);
      
  ret = hsl_msg_decode_igs_entry(&pnt, &size, &msg, hsl_malloc);
  if (ret < 0)
    goto CLEANUP;

  ret = hsl_mc_v4_igs_del_entry(msg.bridge_name, msg.src.s_addr, msg.group.s_addr,
                                msg.vid, msg.count, msg.ifindexes, msg.is_exclude);

  if(msg.ifindexes) oss_free(msg.ifindexes,OSS_MEM_HEAP);

  HSL_MSG_PROCESS_RETURN (sock, hdr, ret);
  HSL_FN_EXIT(ret);

 CLEANUP:
  /* Close socket. */
  hsl_sock_release (sock);
  HSL_FN_EXIT(ret);
}
#endif /* HAVE_IGMP_SNOOP */

#if defined HAVE_IGMP_SNOOP || defined HAVE_MLD_SNOOP
/*  HAL_MSG_L2_UNKNOWN_MCAST_DISCARD message. */
int
hsl_msg_l2_unknown_mcast_discard_mode (struct socket *sock,
                                       struct hal_nlmsghdr *hdr,
                                       char *msgbuf)
{
    u_char *pnt;
    u_int32_t size;
    int ret;

    HSL_FN_ENTER();

    HSL_LOG (HSL_LOG_MSG, HSL_LEVEL_INFO,
             "Message: HAL message L2 Unknown Mcast discard mode \n");
  
    pnt = (u_char*)msgbuf;
    size = hdr->nlmsg_len - HAL_NLMSG_ALIGN(HAL_NLMSGHDR_SIZE);

    ret = hsl_bridge_l2_unknown_mcast_mode(HSL_BRIDGE_L2_UNKNOWN_MCAST_DISCARD);

    HSL_MSG_PROCESS_RETURN (sock, hdr, ret);
    HSL_FN_EXIT(ret);
}

/*  HAL_MSG_L2_UNKNOWN_MCAST_FLOOD message. */
int
hsl_msg_l2_unknown_mcast_flood_mode (struct socket *sock,
                                     struct hal_nlmsghdr *hdr,
                                     char *msgbuf)
{
    u_char *pnt;
    u_int32_t size;
    int ret;

    HSL_FN_ENTER();

    HSL_LOG (HSL_LOG_MSG, HSL_LEVEL_INFO,
             "Message: HAL message L2 Unknown mcast flood mode set\n");

    pnt = (u_char*)msgbuf;
    size = hdr->nlmsg_len - HAL_NLMSG_ALIGN(HAL_NLMSGHDR_SIZE);

    ret = hsl_bridge_l2_unknown_mcast_mode(HSL_BRIDGE_L2_UNKNOWN_MCAST_FLOOD);

    HSL_MSG_PROCESS_RETURN (sock, hdr, ret);
    HSL_FN_EXIT(ret);
}

#endif /* HAVE_IGMP_SNOOP || HAVE_MLD SNOOP */

#ifdef HAVE_MLD_SNOOP

#ifdef HAVE_MLD_SNOOP

/*  HAL_MSG_MLD_SNOOPING_INIT message. */
int
hsl_recv_msg_mld_snooping_init (struct socket *sock, struct hal_nlmsghdr *hdr, char *msgbuf)
{
  HSL_LOG (HSL_LOG_MSG, HSL_LEVEL_INFO, "Message: HAL MLD Snooping initialization\n");
  HSL_MSG_PROCESS_RETURN (sock, hdr, 0);
  return 0;
}

/*  HAL_MSG_MLD_SNOOPING_DEINIT message. */
int
hsl_recv_msg_mld_snooping_deinit (struct socket *sock, struct hal_nlmsghdr *hdr, char *msgbuf)
{
  HSL_LOG (HSL_LOG_MSG, HSL_LEVEL_INFO, "Message: HAL MLD Snooping deinitialization\n");
  HSL_MSG_PROCESS_RETURN (sock, hdr, 0);
  return 0;
}

/*  HAL_MSG_MLD_SNOOPING_ENABLE message. */
int
hsl_msg_recv_mld_snooping_enable (struct socket *sock, struct hal_nlmsghdr *hdr, char *msgbuf)
{
  char *name;
  int ret;

  name = (char *) msgbuf;
  ret = hsl_bridge_enable_mld_snooping (name);
  HSL_MSG_PROCESS_RETURN (sock, hdr, ret);
  return ret;
}

/*  HAL_MSG_MLD_SNOOPING_DISABLE message. */
int
hsl_msg_mld_snooping_disable (struct socket *sock, struct hal_nlmsghdr *hdr, char *msgbuf)
{
  char *name;
  int ret;

  name = (char *) msgbuf;
  ret = hsl_bridge_disable_mld_snooping (name);
  HSL_MSG_PROCESS_RETURN (sock, hdr, ret);
  return ret;
}

/*  HAL_MSG_MLD_SNOOPING_ENTRY_ADD message. */
int
hsl_msg_mld_snooping_add_entry(struct socket *sock, struct hal_nlmsghdr *hdr, char *msgbuf)
{
  u_char *pnt;
  u_int32_t size;
  struct hal_msg_mld_snoop_entry msg;
  int ret;

  HSL_FN_ENTER();

  HSL_LOG (HSL_LOG_MSG, HSL_LEVEL_INFO, "Message: HAL message MLD snooping add entry\n");

  pnt = (u_char*)msgbuf;
  size = hdr->nlmsg_len - HAL_NLMSG_ALIGN(HAL_NLMSGHDR_SIZE);

  ret = hsl_msg_decode_mlds_entry(&pnt, &size, &msg, hsl_malloc);

  if (ret < 0)
    goto CLEANUP;

  ret = hsl_mc_v6_mlds_add_entry (msg.bridge_name, (hsl_ipv6Address_t *) &msg.src,
                                  (hsl_ipv6Address_t *) &msg.group,
                                  msg.vid, msg.count, msg.ifindexes, msg.is_exclude);

  if(msg.ifindexes) oss_free(msg.ifindexes,OSS_MEM_HEAP);

  HSL_MSG_PROCESS_RETURN (sock, hdr, ret);
  HSL_FN_EXIT(ret);

 CLEANUP:

  HSL_LOG (HSL_LOG_MSG, HSL_LEVEL_INFO, "Message: HAL message MLD CLEANUP \n");
  /* Close socket. */
  hsl_sock_release (sock);
  HSL_FN_EXIT(ret);
}

/*  HAL_MSG_MLD_SNOOPING_ENTRY_DEL message. */
int
hsl_msg_mld_snooping_del_entry (struct socket *sock, struct hal_nlmsghdr *hdr, char *msgbuf)
{
  u_char *pnt;
  u_int32_t size;
  struct hal_msg_mld_snoop_entry msg;
  int ret;

  HSL_FN_ENTER(); 

  HSL_LOG (HSL_LOG_MSG, HSL_LEVEL_INFO, "Message: HAL message MLD snooping delete entry\n");

  pnt = (u_char*)msgbuf;
  size = hdr->nlmsg_len - HAL_NLMSG_ALIGN(HAL_NLMSGHDR_SIZE);

  ret = hsl_msg_decode_mlds_entry(&pnt, &size, &msg, hsl_malloc);

  if (ret < 0)
    goto CLEANUP;

  ret = hsl_mc_v6_mlds_del_entry (msg.bridge_name, (hsl_ipv6Address_t *) &msg.src,
                                  (hsl_ipv6Address_t *) &msg.group,
                                  msg.vid, msg.count, msg.ifindexes, msg.is_exclude);

  if(msg.ifindexes) oss_free(msg.ifindexes,OSS_MEM_HEAP);

  HSL_MSG_PROCESS_RETURN (sock, hdr, ret);
  HSL_FN_EXIT(ret);

 CLEANUP:
  /* Close socket. */
  hsl_sock_release (sock);
  HSL_FN_EXIT(ret);
}

#endif /* HAVE_MLD_SNOOP */

#endif /* HAVE_MLD_SNOOP */

/*  HAL_MSG_L2_FDB_INIT message. */
int
hsl_msg_recv_l2_fdb_init (struct socket *sock, struct hal_nlmsghdr *hdr, char *msgbuf)
{
  HSL_LOG (HSL_LOG_MSG, HSL_LEVEL_INFO, "Message: HAL L2 FDB initialization\n");
  HSL_MSG_PROCESS_RETURN (sock, hdr, 0);
  return 0;
}

/*  HAL_MSG_L2_FDB_DEINIT message. */
int
hsl_msg_recv_l2_fdb_deinit (struct socket *sock, struct hal_nlmsghdr *hdr, char *msgbuf)
{
  HSL_LOG (HSL_LOG_MSG, HSL_LEVEL_INFO, "Message: HAL L2 FDB deinitialization\n");
  HSL_MSG_PROCESS_RETURN (sock, hdr, 0);
  return 0;
}

/*  HAL_MSG_L2_FDB_ADD message. */
int
hsl_msg_recv_l2_fdb_add (struct socket *sock, struct hal_nlmsghdr *hdr, char *msgbuf)
{
  struct hal_msg_l2_fdb_entry *msg;
  int ret;

  msg = (struct hal_msg_l2_fdb_entry *) msgbuf;
  ret = hsl_bridge_add_fdb (msg->name, msg->ifindex, (char*)msg->mac, HSL_ETHER_ALEN, msg->vid, msg->flags, msg->is_forward);
  HSL_MSG_PROCESS_RETURN (sock, hdr, ret);

  return ret;
}

/*  HAL_MSG_L2_FDB_DELETE message. */
int
hsl_msg_recv_l2_fdb_delete (struct socket *sock, struct hal_nlmsghdr *hdr, char *msgbuf)
{
  struct hal_msg_l2_fdb_entry *msg;
  int ret;

  msg = (struct hal_msg_l2_fdb_entry *) msgbuf;
  ret = hsl_bridge_delete_fdb (msg->name, msg->ifindex, (char*)msg->mac, HSL_ETHER_ALEN, msg->vid, msg->flags);
  HSL_MSG_PROCESS_RETURN (sock, hdr, ret);

  return ret;
}

/*  HAL_MSG_L2_FDB_UNICAST_GET message. */
int
hsl_msg_recv_l2_fdb_unicast_get (struct socket *sock, struct hal_nlmsghdr *hdr, char *msgbuf)
{
  u_char *pnt;
  u_int32_t size;
  struct hal_msg_l2_fdb_entry_req req;
  struct hal_msg_l2_fdb_entry_resp resp;
  int ret = 0, respsz = 0;
  char *buf = NULL;

  HSL_FN_ENTER();
  HSL_LOG (HSL_LOG_MSG, HSL_LEVEL_INFO, "Message: HAL fdb get unicasts.\n");

  /* Get request. */
  pnt = (u_char*)msgbuf;
  size = hdr->nlmsg_len - HAL_NLMSG_ALIGN(HAL_NLMSGHDR_SIZE);

  /* Decode request message. */
  ret = hsl_msg_decode_l2_fdb_req(&pnt, &size, &req);
  if (ret < 0)
    goto CLEANUP;

  /* Prepare response structure. */
  resp.count = 0;

  /* Get buffer for mac entries. */
  size = HAL_MAX_L2_FDB_ENTRIES * sizeof (struct hal_fdb_entry);
  resp.fdb_entry = (struct hal_fdb_entry *)kmalloc (size, GFP_KERNEL);
  if(!resp.fdb_entry)
    goto CLEANUP;

  /* Zero the buffer. */ 
  memset (resp.fdb_entry, 0, size);

  /* Get unicast dynamic mac fdb. */
  ret = hsl_bridge_unicast_get_fdb (&req, &resp);

  if(ret != 0) 
    resp.count = 0;

  /* Allocate response buffer. */
  size = sizeof (struct hal_msg_l2_fdb_entry_resp) + resp.count * sizeof(struct hal_fdb_entry);
  buf = (char *) kmalloc(size,GFP_KERNEL);
  if (!buf)
    goto CLEANUP;

  pnt = (u_char*)buf;
  /* Encode message response. */
  respsz =  hsl_msg_encode_l2_fdb_resp (&pnt, &size, &resp);

  /* Post the message. */
  ret = hsl_sock_post_msg (sock, HAL_MSG_L2_FDB_UNICAST_GET, 0, hdr->nlmsg_seq, buf, respsz);

  /* Free the buffer. */
  kfree(buf);
  kfree(resp.fdb_entry);
  HSL_FN_EXIT(ret);

 CLEANUP:
  /* Close socket. */
  hsl_sock_release (sock);
  if(buf)            kfree(buf);
  if(resp.fdb_entry) kfree(resp.fdb_entry);
  HSL_FN_EXIT(STATUS_ERROR);
}

/*  HAL_MSG_L2_FDB_MULTICAST_GET message. */
int
hsl_msg_recv_l2_fdb_multicast_get (struct socket *sock, struct hal_nlmsghdr *hdr, char *msgbuf)
{
  return 0;
}

/* HAL_MSG_L2_FDB_FLUSH_PORT message. */
int
hsl_msg_recv_l2_fdb_flush (struct socket *sock, struct hal_nlmsghdr *hdr, char *msgbuf)
{
  struct hal_msg_l2_fdb_flush *msg;
  int ret = 0;

  msg = (struct hal_msg_l2_fdb_flush *) msgbuf;
  ret = hsl_bridge_flush_fdb (msg->name, msg->ifindex, msg->instance, msg->vid);
  HSL_MSG_PROCESS_RETURN (sock, hdr, ret);

  return ret;
}

/* HAL_MSG_L2_FDB_FLUSH_BY_MAC message. */
int
hsl_msg_recv_l2_fdb_flush_by_mac (struct socket *sock, struct hal_nlmsghdr *hdr, char *msgbuf)
{
  struct hal_msg_l2_fdb_entry *msg;
  int ret;

  msg = (struct hal_msg_l2_fdb_entry *) msgbuf;
  ret = hsl_bridge_flush_fdb_by_mac (msg->name, (char*)msg->mac, HSL_ETHER_ALEN, msg->flags);
  HSL_MSG_PROCESS_RETURN (sock, hdr, ret);

  return ret;
}
#endif /* HAVE_L2 */

#ifdef HAVE_ONMD
/* HAL_MSG_EFM_SET_FRAME_PERIOD_WINDOW */
int
hsl_msg_recv_set_frame_period_window(struct socket *sock, 
                                     struct hal_nlmsghdr *hdr,
                                     char* msgbuf)
{
  struct hal_msg_efm_set_frame_period_window* msg;
  int ret=0;
  
  HSL_FN_ENTER("+ hsl_msg_recv_set_frame_period_window \n");
  HSL_LOG (HSL_LOG_MSG, HSL_LEVEL_INFO,
                        "Message: %s \n", __FUNCTION__);
  
  msg = (struct hal_msg_efm_set_frame_period_window*) msgbuf;
  ret = hsl_ifmgr_efm_frame_period_window_set(msg->index, 
                                              msg->frame_period_window); 
  HSL_MSG_PROCESS_RETURN (sock, hdr, ret);

  HSL_FN_EXIT(ret);

  return ret;
}


/* HAL_MSG_EFM_SET_SYMBOL_PERIOD_WINDOW */
int
hsl_msg_recv_set_symbol_period_window(struct socket *sock, 
                                      struct hal_nlmsghdr *hdr,
                                      char* msgbuf)
{
  int ret = 0;
  
  HSL_LOG (HSL_LOG_MSG, HSL_LEVEL_INFO, 
           "Message: HSL Symbol Period Window set\n");
  
  return ret;
}

/* HAL_MSG_EFM_FRAME_ERROR_COUNT message*/
int
hsl_msg_recv_get_frame_error_count(struct socket *sock, 
                                   struct hal_nlmsghdr *hdr, 
                                   char* msgbuf)
{
  u_char *pnt;
  u_int32_t size;
  u_int32_t req_index;
  struct hal_msg_efm_err_frames resp;
  int ret = 0, respsz = 0;
  char *buf = NULL;
  int flag_cleanup = 0;
  struct hsl_if *ifp = NULL;

  HSL_FN_ENTER();
  HSL_LOG (HSL_LOG_MSG, HSL_LEVEL_INFO, 
      "Message: HSL get_frame_error_count\n");

  /* Get request. */
  pnt = (u_char*)msgbuf;
  size = hdr->nlmsg_len - HAL_NLMSG_ALIGN(HAL_NLMSGHDR_SIZE);

  /* Decode request message. */
  ret = hsl_msg_decode_err_frames_req(&pnt, &size, &req_index);
  if (ret < 0)
    flag_cleanup = 1;

  if(!flag_cleanup)
    {
      /* Prepare response structure. */
      resp.index = 0;
      memset(&resp.err_frames, 0, sizeof(ut_int64_t));

      /* Get interface pointer */
      ifp = hsl_ifmgr_lookup_by_index (req_index);
      if(!ifp)
        flag_cleanup = 1;
    }

  if(!flag_cleanup)
    {
      /*Get EFM error frames count*/
      ret = hsl_ifmgr_efm_get_frame_errors (ifp, NULL, &resp.err_frames.l[0]);
      if(ret!=0)
        {
          resp.index = 0;
        }
      HSL_LOG (HSL_LOG_MSG, HSL_LEVEL_INFO, "Message: index = %u, \
          counter = %u \n", req_index, resp.err_frames.l[0]);

      /* Allocate message buffer. */
      size = sizeof(struct hal_msg_efm_err_frames); 
      buf = (char *) kmalloc(size,GFP_KERNEL);
      pnt = (u_char*) buf;
      /* Encode message response. */
      respsz =  hsl_msg_encode_err_frames_resp (&pnt, &size, &resp);

      /* Post the message. */
      ret = hsl_sock_post_msg (sock, HAL_MSG_EFM_FRAME_ERROR_COUNT, 0, 
          hdr->nlmsg_seq, buf, respsz);

      /* Free the buffer. */
      kfree(buf);
      HSL_FN_EXIT(ret);
    }

  if(flag_cleanup)
    {
      /* Close socket. */
      hsl_sock_release (sock);
      if(buf)            kfree(buf);
      HSL_FN_EXIT(STATUS_ERROR);
    }

  return ret;
}

/* HAL_MSG_EFM_FRAME_ERROR_SECONDS_COUNT message*/
int
hsl_msg_recv_get_frame_error_seconds_count (struct socket *sock, 
                                            struct hal_nlmsghdr *hdr, 
                                            char* msgbuf)
{
  u_char *pnt;
  u_int32_t size;
  u_int32_t req_index;
  struct hal_msg_efm_err_frame_stat resp;
  struct hsl_if *ifp = NULL;
  int ret = 0, respsz = 0;
  char *buf = NULL;
  int flag_cleanup = 0;

  HSL_FN_ENTER();
  HSL_LOG (HSL_LOG_MSG, HSL_LEVEL_INFO, 
           "Message: HSL get_frame_error_secs_count\n");

  /* Get request. */
  pnt = (u_char*)msgbuf;
  size = hdr->nlmsg_len - HAL_NLMSG_ALIGN(HAL_NLMSGHDR_SIZE);

  /* Decode request message. */
  ret = hsl_msg_decode_err_frame_secs_req(&pnt, &size, &req_index);
  if (ret < 0)
    flag_cleanup = 1;

  if(!flag_cleanup)
    {
      /* Prepare response structure. */
      resp.index = 0;
      resp.err_frames = 0;

      /* Get interface pointer */
      ifp = hsl_ifmgr_lookup_by_index (req_index);
      if(!ifp)
        flag_cleanup = 1;
    }
  
  if(!flag_cleanup)
    {
      /* fill up the EFS count*/
      resp.err_frames = ifp->u.l2_ethernet.efm_err_frame.efm_efs_count;

      HSL_LOG (HSL_LOG_MSG, HSL_LEVEL_INFO, "Message: index =req_index, \
          counter = %u \n", req_index, resp.err_frames);

      /* Allocate response buffer. */
      size = sizeof(struct hal_msg_efm_err_frame_stat);
      buf = (char *) kmalloc(size,GFP_KERNEL);
      if (!buf)
        flag_cleanup = 1;
    }
  
  
  if(!flag_cleanup)
    {
      pnt = (u_char*)buf;
      /* Encode message response. */
      respsz =  hsl_msg_encode_err_frame_secs_resp (&pnt, &size, &resp);

      /* Post the message. */
      ret = hsl_sock_post_msg (sock, HAL_MSG_EFM_FRAME_ERROR_SECONDS_COUNT, 0, 
          hdr->nlmsg_seq, buf, respsz);

      /* Free the buffer. */
      kfree(buf);
      HSL_FN_EXIT(ret);
    }

  if(flag_cleanup)
    {
      /* Close socket. */
      hsl_sock_release (sock);
      if(buf)            kfree(buf);
      HSL_FN_EXIT(STATUS_ERROR);
    }

  return ret;
}
#endif /*HAVE_ONMD*/

/*  HAL_MSG_PMIRROR_INIT message. */
int
hsl_msg_recv_pmirror_init (struct socket *sock, struct hal_nlmsghdr *hdr, char *msgbuf)
{
  int ret =0;

  HSL_FN_ENTER();

  HSL_LOG (HSL_LOG_MSG, HSL_LEVEL_INFO, "Message: HAL Port Mirroring initialization\n");
  /* Init port mirroring. */
  //ret = hsl_ifmgr_init_portmirror ();

  HSL_MSG_PROCESS_RETURN (sock, hdr, ret);
  HSL_FN_EXIT(0);
}

/*  HAL_MSG_PMIRROR_DEINIT message. */
int
hsl_msg_recv_pmirror_deinit (struct socket *sock, struct hal_nlmsghdr *hdr, char *msgbuf)
{
  int ret;

  HSL_FN_ENTER();
  HSL_LOG (HSL_LOG_MSG, HSL_LEVEL_INFO, "Message: HAL Port Mirroring deinitialization\n");

  /* Deinit port mirroring. */
  ret = hsl_ifmgr_deinit_portmirror ();

  HSL_MSG_PROCESS_RETURN (sock, hdr, ret);
  HSL_FN_EXIT(0);
}

/*  HAL_MSG_PMIRROR_SET message. */
int
hsl_msg_recv_pmirror_set (struct socket *sock, struct hal_nlmsghdr *hdr, char *msgbuf)
{
  struct hal_msg_port_mirror *msg;
  int ret;

  msg = (struct hal_msg_port_mirror *) msgbuf;
  ret = hsl_ifmgr_set_portmirror(msg->to_ifindex, msg->from_ifindex, msg->direction);
  HSL_MSG_PROCESS_RETURN (sock, hdr, ret);
  return ret;
}

/*  HAL_MSG_PMIRROR_UNSET message. */
int
hsl_msg_recv_pmirror_unset (struct socket *sock, struct hal_nlmsghdr *hdr, char *msgbuf)
{
  struct hal_msg_port_mirror *msg;
  int ret;

  msg = (struct hal_msg_port_mirror *) msgbuf;
  ret = hsl_ifmgr_unset_portmirror(msg->to_ifindex, msg->from_ifindex, msg->direction);
  HSL_MSG_PROCESS_RETURN (sock, hdr, ret);
  return ret;
}

#ifdef HAVE_QOS
/*  HAL_MSG_QOS_INIT message. */
int
hsl_msg_recv_qos_init (struct socket *sock, struct hal_nlmsghdr *hdr, char *msgbuf)
{
  int ret =0;

  HSL_LOG (HSL_LOG_MSG, HSL_LEVEL_INFO, "Message: HAL QOS initialization\n");

 // ret = hsl_bcm_qos_init ();
  HSL_MSG_PROCESS_RETURN (sock, hdr, ret);
  return 0;
}

/*  HAL_MSG_QOS_DEINIT message. */
int
hsl_msg_recv_qos_deinit (struct socket *sock, struct hal_nlmsghdr *hdr, char *msgbuf)
{
  int ret;

  HSL_LOG (HSL_LOG_MSG, HSL_LEVEL_INFO, "Message: HAL QOS uninitialization\n");

  ret = hsl_bcm_qos_deinit ();
  HSL_MSG_PROCESS_RETURN (sock, hdr, ret);
  return 0;
}

/*  HAL_MSG_QOS_ENABLE message. */
int
hsl_msg_recv_qos_enable (struct socket *sock, struct hal_nlmsghdr *hdr, char *msgbuf)
{
  struct hal_msg_qos_enable *msg = (struct hal_msg_qos_enable *)msgbuf;
  int ret;
 /* wangjian@150108, enable qos */
 HSL_LOG (HSL_LOG_MSG, HSL_LEVEL_INFO, "Message: HAL QOS enable\n");
 
 ret = hsl_bcm_qos_enable (&msg->q0[0], &msg->q1[0], &msg->q2[0], &msg->q3[0],
                            &msg->q4[0], &msg->q5[0], &msg->q6[0], &msg->q7[0]);

  HSL_MSG_PROCESS_RETURN (sock, hdr, ret);
  return 0;
}

/*  HAL_MSG_QOS_DISABLE message. */
int
hsl_msg_recv_qos_disable (struct socket *sock, struct hal_nlmsghdr *hdr, char *msgbuf)
{
  struct hal_msg_qos_disable *msg = (struct hal_msg_qos_disable *)msgbuf;
  int ret;

  ret = hsl_bcm_qos_disable (msg->num_queue);
  HSL_MSG_PROCESS_RETURN (sock, hdr, ret);
  return 0;
}

/*  HAL_MSG_QOS_WRR_QUEUE_LIMIT message. */
int
hsl_msg_recv_qos_wrr_queue_limit (struct socket *sock, struct hal_nlmsghdr *hdr, char *msgbuf)
{
  struct hal_msg_qos_wrr_queue_limit *msg = (struct hal_msg_qos_wrr_queue_limit *)msgbuf;
  int ret;

  ret = hsl_bcm_qos_wrr_queue_limit (msg->ifindex, &msg->ratio[0]);
  


  HSL_MSG_PROCESS_RETURN (sock, hdr, ret);
  return 0;
}

/*  HAL_MSG_QOS_WRR_TAIL_DROP_THRESHOLD message. */
int
hsl_msg_recv_qos_wrr_tail_drop_threshold (struct socket *sock, struct hal_nlmsghdr *hdr, char *msgbuf)
{
  struct hal_msg_qos_wrr_tail_drop_threshold *msg = (struct hal_msg_qos_wrr_tail_drop_threshold *)msgbuf;
  int ret;
  
  ret = hsl_bcm_qos_wrr_tail_drop_threshold (msg->ifindex, msg->qid, msg->thres[0], msg->thres[1]);
  
  HSL_MSG_PROCESS_RETURN (sock, hdr, ret);
  return 0;
}

/*  HAL_MSG_QOS_WRR_THRESHOLD_DSCP_MAP message. */
int
hsl_msg_recv_qos_wrr_threshold_dscp_map (struct socket *sock, struct hal_nlmsghdr *hdr, char *msgbuf)
{
  HSL_MSG_PROCESS_RETURN (sock, hdr, 0);
  return 0;
}

/*  HAL_MSG_QOS_WRR_WRED_DROP_THRESHOLD message. */
int
hsl_msg_recv_qos_wrr_wred_drop_threshold (struct socket *sock, struct hal_nlmsghdr *hdr, char *msgbuf)
{
  struct hal_msg_qos_wrr_wred_drop_threshold *msg = (struct hal_msg_qos_wrr_wred_drop_threshold *)msgbuf;
  int ret;

  ret = hsl_bcm_qos_wrr_wred_drop_threshold (msg->ifindex, msg->qid, msg->thres[0], msg->thres[1]);
  
  HSL_MSG_PROCESS_RETURN (sock, hdr, ret);
  return 0;
}

/* zlh@150119,add weight masg */
/*  HAL_MSG_QOS_PORT_COSQ_WEIGHT_SET message. */
int
hsl_msg_recv_qos_port_cosq_weight_set (struct socket *sock, struct hal_nlmsghdr *hdr, char *msgbuf)
{
   
  int ret;
  
  hal_wrr_weight_info_t *ifp;
  
  ifp = (struct hal_wrr_weight_info_t *)msgbuf;

  ret = hsl_bcm_qos_port_cosq_weight_set ( *ifp );//zlh@150108 set weight


  HSL_MSG_PROCESS_RETURN (sock, hdr, ret);
  return 0;
}
/* zlh@150119,add bandwidth masg */
/*  HAL_MSG_QOS_PORT_COSQ_BANDWIDTH_SET message. */
int
hsl_msg_recv_qos_port_cosq_bandwidth_set (struct socket *sock, struct hal_nlmsghdr *hdr, char *msgbuf)
{
  hal_wrr_car_info_t *ifp;
  int ret;
  ifp = (struct hal_wrr_car_info_t *) msgbuf;
  ret = hsl_bcm_qos_port_cosq_bandwidth_set ( *ifp );//zlh@150108 set minbucket/maxbucket
  
  HSL_MSG_PROCESS_RETURN (sock, hdr, ret);
  return 0;
}
/* zlh@150119,add wred masg */
/*  HAL_MSG_QOS_PORT_COSQ_WRED_DROP_SET message. */
int
hsl_msg_recv_qos_port_cosq_wred_drop_set (struct socket *sock, struct hal_nlmsghdr *hdr, char *msgbuf)
{
  hal_wrr_wred_info_t *ifp;
  int ret;
  ifp = ( hal_wrr_wred_info_t *)msgbuf;

  ret = hsl_bcm_qos_port_cosq_wred_drop_set ( *ifp );
  
  HSL_MSG_PROCESS_RETURN (sock, hdr, ret);
  return 0;
}






/*  HAL_MSG_QOS_WRR_SET_BANDWIDTH message. */
int
hsl_msg_recv_qos_wrr_set_bandwidth (struct socket *sock, struct hal_nlmsghdr *hdr, char *msgbuf)
{
  struct hal_msg_qos_wrr_set_bandwidth *msg = (struct hal_msg_qos_wrr_set_bandwidth *)msgbuf;
  int ret;

  ret = hsl_bcm_qos_wrr_set_bandwidth (msg->ifindex, &msg->bw[0]);
  HSL_MSG_PROCESS_RETURN (sock, hdr, ret);
  return 0;
}

/*  HAL_MSG_QOS_WRR_QUEUE_COS_MAP_SET message. */
int
hsl_msg_recv_qos_wrr_queue_cos_map_set (struct socket *sock, struct hal_nlmsghdr *hdr, char *msgbuf)
{
  HSL_MSG_PROCESS_RETURN (sock, hdr, 0);
  return 0;
}

/*  HAL_MSG_QOS_WRR_QUEUE_COS_MAP_UNSET message. */
int
hsl_msg_recv_qos_wrr_queue_cos_map_unset (struct socket *sock, struct hal_nlmsghdr *hdr, char *msgbuf)
{
  HSL_MSG_PROCESS_RETURN (sock, hdr, 0);
  return 0;
}


/*  HAL_MSG_QOS_WRR_QUEUE_MIN_RESERVE message. */
int
hsl_msg_recv_qos_wrr_queue_min_reserve (struct socket *sock, struct hal_nlmsghdr *hdr, char *msgbuf)
{
  struct hal_msg_qos_wrr_queue_min_reserve *msg = (struct hal_msg_qos_wrr_queue_min_reserve *)msgbuf;
  int ret;

  ret = hsl_bcm_qos_wrr_queue_min_reserve (msg->ifindex, msg->qid, msg->packets);
  HSL_MSG_PROCESS_RETURN (sock, hdr, ret);
  return 0;
}

/*  HAL_MSG_QOS_SET_TRUST_STATE message. */
int
hsl_msg_recv_qos_set_trust_state (struct socket *sock, struct hal_nlmsghdr *hdr, char *msgbuf)
{
  struct hal_msg_qos_set_trust_state *msg = (struct hal_msg_qos_set_trust_state *)msgbuf;
  int ret;

  ret = hsl_bcm_qos_set_trust_state (msg->ifindex, msg->trust_state);
  HSL_MSG_PROCESS_RETURN (sock, hdr, ret);
  return 0;
}

/*  HAL_MSG_QOS_SET_DEFAULT_COS message. */
int
hsl_msg_recv_qos_set_default_cos (struct socket *sock, struct hal_nlmsghdr *hdr, char *msgbuf)
{
  struct hal_msg_qos_set_default_cos *msg = (struct hal_msg_qos_set_default_cos *)msgbuf;
  int ret;

  ret = hsl_bcm_qos_set_default_cos (msg->ifindex, msg->cos_value);
  HSL_MSG_PROCESS_RETURN (sock, hdr, ret);
  return 0;
}


/*  HAL_MSG_QOS_SET_DSCP_MAP_TBL message. */
int
hsl_msg_recv_qos_set_dscp_mapping_tbl (struct socket *sock, struct hal_nlmsghdr *hdr, char *msgbuf)
{
  struct hal_msg_qos_set_dscp_map_tbl *msg = (struct hal_msg_qos_set_dscp_map_tbl *)msgbuf;
  int ret;

  ret = hsl_bcm_qos_set_dscp_mapping_tbl (msg->ifindex, msg->flag, msg->map_table, msg->map_count);
  HSL_MSG_PROCESS_RETURN (sock, hdr, ret);
  return 0;
}


/*  HAL_MSG_QOS_SET_CLASS_MAP message. */
int
hsl_msg_recv_qos_set_class_map (struct socket *sock, struct hal_nlmsghdr *hdr, char *msgbuf)
{
  struct hal_msg_qos_set_class_map *msg = (struct hal_msg_qos_set_class_map *)msgbuf;
  int ret;

  ret = hsl_bcm_qos_set_class_map (msg);
  HSL_MSG_PROCESS_RETURN (sock, hdr, ret);
  return 0;
}

/*  HAL_MSG_QOS_SET_CMAP_COS_INNER message. */
int
hsl_msg_recv_qos_set_cmap_cos_inner (struct socket *sock, struct hal_nlmsghdr *hdr, char *msgbuf)
{
  struct hal_msg_qos_set_class_map *msg = (struct hal_msg_qos_set_class_map *)msgbuf;
  int ret;
                                                                                
  ret = hsl_bcm_qos_set_cmap_cos_inner (msg);
  HSL_MSG_PROCESS_RETURN (sock, hdr, ret);
  return 0;
}


/*  HAL_MSG_QOS_SET_POLICY_MAP message. */
int
hsl_msg_recv_qos_set_policy_map (struct socket *sock, struct hal_nlmsghdr *hdr, char *msgbuf)
{
  struct hal_msg_qos_set_policy_map *msg = (struct hal_msg_qos_set_policy_map *)msgbuf;
  int ret;

  ret = hsl_bcm_qos_set_policy_map (msg);
  HSL_MSG_PROCESS_RETURN (sock, hdr, ret);
  return 0;
}
#endif /* HAVE_QOS */

#ifdef HAVE_LACPD
/*  HAL_MSG_LACP_INIT message. */
int
hsl_msg_recv_lacp_init (struct socket *sock, struct hal_nlmsghdr *hdr, char *msgbuf)
{
  int ret;

  HSL_LOG (HSL_LOG_MSG, HSL_LEVEL_INFO, "Message: HAL LACP initialization\n");

  ret = hsl_bcm_lacp_init ();
  HSL_MSG_PROCESS_RETURN (sock, hdr, ret);
  return 0;
}

/*  HAL_MSG_LACP_DEINIT message. */
int
hsl_msg_recv_lacp_deinit (struct socket *sock, struct hal_nlmsghdr *hdr, char *msgbuf)
{
  int ret;

  HSL_LOG (HSL_LOG_MSG, HSL_LEVEL_INFO, "Message: HAL LACP uninitialization\n");

  ret = hsl_bcm_lacp_deinit ();
  HSL_MSG_PROCESS_RETURN (sock, hdr, ret);
  return 0;
}

/*  HAL_MSG_LACP_ADD_AGGREGATOR message. */
int
hsl_msg_recv_lacp_add_aggregator (struct socket *sock, struct hal_nlmsghdr *hdr, 
                                  char *msgbuf)
{
  u_char *pnt;
  u_int32_t size;
  struct hal_msg_lacp_agg_add msg;
  int ret;

  HSL_FN_ENTER();

    HSL_LOG (HSL_LOG_MSG, HSL_LEVEL_INFO, "Message: HAL message add aggregator\n");

  pnt = (u_char*)msgbuf;
  size = hdr->nlmsg_len - HAL_NLMSG_ALIGN(HAL_NLMSGHDR_SIZE);
      
  ret = hsl_msg_decode_lacp_agg_add(&pnt, &size, &msg);
  if (ret < 0)
    goto CLEANUP;

  ret = hsl_ifmgr_aggregator_add(msg.agg_name,msg.agg_mac,msg.agg_type);

  HSL_MSG_PROCESS_RETURN (sock, hdr, ret);
  HSL_FN_EXIT(ret);

 CLEANUP:
  /* Close socket. */
  hsl_sock_release (sock);
  HSL_FN_EXIT(-1);
}

/*  HAL_MSG_LACP_DELETE_AGGREGATOR message. */
int
hsl_msg_recv_lacp_delete_aggregator (struct socket *sock, struct hal_nlmsghdr *hdr, char *msgbuf)
{
  u_char *pnt;
  u_int32_t size;
  struct hal_msg_lacp_agg_identifier msg;
  int ret;

  HSL_FN_ENTER(); 

    HSL_LOG (HSL_LOG_MSG, HSL_LEVEL_INFO, "Message: HAL message delete aggregator\n");

  pnt = (u_char*)msgbuf;
  size = hdr->nlmsg_len - HAL_NLMSG_ALIGN(HAL_NLMSGHDR_SIZE);
      
  ret = hsl_msg_decode_lacp_id(&pnt, &size, &msg);
  if (ret < 0)
    goto CLEANUP;

  ret = hsl_ifmgr_aggregator_del(msg.agg_name,msg.agg_ifindex);

  HSL_MSG_PROCESS_RETURN (sock, hdr, ret);
  HSL_FN_EXIT(ret);

 CLEANUP:
  /* Close socket. */
  hsl_sock_release (sock);
  HSL_FN_EXIT(-1);
}


/*  HAL_MSG_LACP_ATTACH_MUX_TO_AGGREGATOR message. */
int
hsl_msg_recv_lacp_attach_mux_to_aggregator (struct socket *sock, struct hal_nlmsghdr *hdr,
                                            char *msgbuf)
{
  u_char *pnt;
  u_int32_t size;
  struct hal_msg_lacp_mux msg;
  int ret;

  HSL_FN_ENTER(); 

    HSL_LOG (HSL_LOG_MSG, HSL_LEVEL_INFO, "Message: HAL attach port to aggregator\n");

  pnt = (u_char*)msgbuf;
  size = hdr->nlmsg_len - HAL_NLMSG_ALIGN(HAL_NLMSGHDR_SIZE);
      
  ret = hsl_msg_decode_lacp_mux(&pnt, &size, &msg);
  if (ret < 0)
    goto CLEANUP;

  ret = hsl_ifmgr_aggregator_port_attach(msg.agg_name,msg.agg_ifindex,msg.port_ifindex);

  HSL_MSG_PROCESS_RETURN (sock, hdr, ret);
  HSL_FN_EXIT(ret);

 CLEANUP:
  /* Close socket. */
  hsl_sock_release (sock);
  HSL_FN_EXIT(-1);
}

/*  HAL_MSG_LACP_DETACH_MUX_TO_AGGREGATOR message. */
int
hsl_msg_recv_lacp_detach_mux_from_aggregator (struct socket *sock, struct hal_nlmsghdr *hdr,
                                              char *msgbuf)
{
  u_char *pnt;
  u_int32_t size;
  struct hal_msg_lacp_mux msg;
  int ret;

  HSL_FN_ENTER(); 

    HSL_LOG (HSL_LOG_MSG, HSL_LEVEL_INFO, "Message: HAL Detach port from aggregator\n");

  pnt = (u_char*)msgbuf;
  size = hdr->nlmsg_len - HAL_NLMSG_ALIGN(HAL_NLMSGHDR_SIZE);
      
  ret = hsl_msg_decode_lacp_mux(&pnt, &size, &msg);
  if (ret < 0)
    goto CLEANUP;

  ret = hsl_ifmgr_aggregator_port_detach(msg.agg_name,msg.agg_ifindex,msg.port_ifindex);

  HSL_MSG_PROCESS_RETURN (sock, hdr, ret);
  HSL_FN_EXIT(ret);

 CLEANUP:
  /* Close socket. */
  hsl_sock_release (sock);
  HSL_FN_EXIT(-1);
}


/*  HAL_MSG_LACP_PSC_SET message. */
int
hsl_msg_recv_lacp_psc_set (struct socket *sock, struct hal_nlmsghdr *hdr, 
                           char *msgbuf)
{
  u_char *pnt;
  u_int32_t size;
  struct hal_msg_lacp_psc_set msg;
  int ret;

  HSL_FN_ENTER(); 

    HSL_LOG (HSL_LOG_MSG, HSL_LEVEL_INFO, "Message: HAL Set lacp psc\n");

  pnt = (u_char*)msgbuf;
  size = hdr->nlmsg_len - HAL_NLMSG_ALIGN(HAL_NLMSGHDR_SIZE);
      
  ret = hsl_msg_decode_lacp_psc_set(&pnt, &size, &msg);
  if (ret < 0)
    goto CLEANUP;

  ret = hsl_ifmgr_lacp_psc_set (msg.ifindex,msg.psc);

  HSL_MSG_PROCESS_RETURN (sock, hdr, ret);
  HSL_FN_EXIT(ret);

 CLEANUP:
  /* Close socket. */
  hsl_sock_release (sock);
  HSL_FN_EXIT(-1);
}


/*  HAL_MSG_LACP_COLLECTING message. */
int
hsl_msg_recv_lacp_collecting (struct socket *sock, struct hal_nlmsghdr *hdr, char *msgbuf)
{
  HSL_FN_ENTER();
  HSL_FN_EXIT(0);
}

/* HAL_MSG_LACP_DISTRIBUTING message. */
int
hsl_msg_recv_lacp_distributing (struct socket *sock, struct hal_nlmsghdr *hdr, char *msgbuf)
{
  HSL_FN_ENTER();
  HSL_FN_EXIT(0);
}

/*  HAL_MSG_LACP_COLLECTING_DISTRIBUTING message. */
int
hsl_msg_recv_lacp_collecting_distributing (struct socket *sock, struct hal_nlmsghdr *hdr, char *msgbuf)
{
  HSL_FN_ENTER();
  HSL_FN_EXIT(0);
}
#endif /* HAVE_LACPD */

#ifdef HAVE_AUTHD

/*  HAL_MSG_8021x_INIT message. */
int
hsl_msg_recv_auth_init (struct socket *sock, struct hal_nlmsghdr *hdr, char *msgbuf)
{
  int ret;
  
  ret = hsl_bcm_auth_init ();
  HSL_MSG_PROCESS_RETURN (sock, hdr, ret);
  return 0;
}


/*  HAL_MSG_8021x_DEINIT message. */
int
hsl_msg_recv_auth_deinit (struct socket *sock, struct hal_nlmsghdr *hdr, char *msgbuf)
{
  int ret;
  
  ret = hsl_bcm_auth_deinit ();
  HSL_MSG_PROCESS_RETURN (sock, hdr, ret);
  return 0;
}


/*  HAL_MSG_8021x_PORT_STATE message. */
int
hsl_msg_recv_auth_set_port_state (struct socket *sock, struct hal_nlmsghdr *hdr, 
                                  char *msgbuf)
{
  int ret;
  struct hal_msg_auth_port_state *msg = 
    (struct hal_msg_auth_port_state *)msgbuf;
  
  ret = hsl_bcm_auth_set_port_state (msg->port_ifindex,
                                     msg->port_state);
  HSL_MSG_PROCESS_RETURN (sock, hdr, ret);
  return 0;
}

#ifdef HAVE_MAC_AUTH
/* Set port auth-mac state */
int
hsl_msg_recv_auth_mac_set_port_state (struct socket *sock,
                                      struct hal_nlmsghdr *hdr, char *msgbuf)
{
  int ret;
  struct hal_msg_auth_port_state *msg =
    (struct hal_msg_auth_port_state *)msgbuf;

  ret = hsl_bcm_auth_mac_set_port_state (msg->port_ifindex,
                                         msg->port_state);
  HSL_MSG_PROCESS_RETURN (sock, hdr, ret);
  return 0;
}
#endif

#endif /* HAVE_AUTHD */

#ifdef HAVE_L3

/* HAL_MSG_ARP_ADD message */
int
hsl_msg_recv_arp_add (struct socket *sock, struct hal_nlmsghdr *hdr, char *msgbuf)
{
  struct hal_msg_arp_update *msg = (struct hal_msg_arp_update *)msgbuf;
  int ret = 0;

  HSL_FN_ENTER();

  if (hsl_l3_enable_status) /*Update h/w iff L3 fwd is enabled*/
    {
      HSL_LOG (HSL_LOG_MSG, HSL_LEVEL_INFO, "Message: HAL ARP add message \n");
  if (msg->is_proxy_arp)
        ret = hsl_fib_arp_entry_add ((hsl_ipv4Address_t *)&msg->ip_addr,
                                      msg->ifindex, msg->mac_addr, 
            HSL_NH_ENTRY_STATIC|HSL_NH_ENTRY_VALID|HSL_NH_ENTRY_PROXY);
  else
        ret = hsl_fib_arp_entry_add ((hsl_ipv4Address_t *)&msg->ip_addr,
                                       msg->ifindex, msg->mac_addr, 
                               HSL_NH_ENTRY_STATIC|HSL_NH_ENTRY_VALID);
    }
  HSL_MSG_PROCESS_RETURN (sock, hdr, ret);
  HSL_FN_EXIT(ret);
}

/* HAL_MSG_ARP_DEL message */
int
hsl_msg_recv_arp_del (struct socket *sock, struct hal_nlmsghdr *hdr, char *msgbuf)
{
  struct hal_msg_arp_update *msg = (struct hal_msg_arp_update *)msgbuf;
  int ret = 0;

  HSL_FN_ENTER();
 if (hsl_l3_enable_status) /*L3 entries will be present iff L3 fwding is enabled*/
  {
  HSL_LOG (HSL_LOG_MSG, HSL_LEVEL_INFO, "Message: HAL ARP del message \n");

  ret = hsl_fib_arp_entry_del((hsl_ipv4Address_t *)&msg->ip_addr,
      msg->ifindex);
  }
  HSL_MSG_PROCESS_RETURN (sock, hdr, ret);
  HSL_FN_EXIT(ret);
}


/* HAL_MSG_ARP_DEL_ALL message */
int
hsl_msg_recv_arp_del_all (struct socket *sock, struct hal_nlmsghdr *hdr, char *msgbuf)
{
  int ret;
  struct hal_msg_arp_del_all *msg = (struct hal_msg_arp_del_all *)msgbuf;

  HSL_FN_ENTER();
  HSL_LOG (HSL_LOG_MSG, HSL_LEVEL_INFO, "Message: HAL arp del all message \n");

  ret = hsl_fib_arp_del_all ((hsl_fib_id_t)msg->fib_id, msg->clr_flag);

  HSL_MSG_PROCESS_RETURN (sock, hdr, ret);
  HSL_FN_EXIT(ret);
}

/*  HAL_MSG_ARP_CACHE_GET message. */
int
hsl_msg_recv_arp_cache_get (struct socket *sock, struct hal_nlmsghdr *hdr, char *msgbuf)
{
  u_char *pnt;
  u_int32_t size;
  struct hal_msg_arp_cache_req req;
  struct hal_msg_arp_cache_resp resp;
  int ret = 0, respsz = 0;
  char *buf = NULL; 

  HSL_FN_ENTER();
  HSL_LOG (HSL_LOG_MSG, HSL_LEVEL_INFO, "Message: HAL ARP cache get message.\n");

  memset (&resp, 0, sizeof (struct hal_msg_arp_cache_resp));

  /* Get requests should have ACK flag set. */
  pnt = (u_char*)msgbuf;
  size = hdr->nlmsg_len - HAL_NLMSG_ALIGN(HAL_NLMSGHDR_SIZE);
  /* Decode request. */
  ret = hsl_msg_decode_arp_cache_req(&pnt, &size, &req);
  if (ret < 0)
    goto CLEANUP;

  /* Get buffer for arp entries. */
  size = HAL_ARP_CACHE_GET_COUNT * sizeof (struct hal_arp_cache_entry);
  resp.cache = (struct hal_arp_cache_entry *)kmalloc (size, GFP_KERNEL);
  if(!resp.cache)
    goto CLEANUP;


  /*  Get Arp entries. */
  ret = hsl_fib_nh_get_bundle((hsl_fib_id_t)req.fib_id, (hsl_ipv4Address_t *)(req.ip_addr.s_addr == 0 ? NULL : &req.ip_addr), req.count, resp.cache);

  /* Allocate buffer to send a response. */
  if((ret < 0) || (ret > HAL_ARP_CACHE_GET_COUNT)) 
    ret = 0;

  size = sizeof (resp) + ret * sizeof(struct hal_arp_cache_entry);
  buf = (char *) kmalloc (size, GFP_KERNEL);
  if (!buf)
    goto CLEANUP;
     
  pnt = (u_char*)buf;
  if (ret > 0)
    resp.count = ret;
  else
    resp.count = 0;

  /* Encode arp-cache response. */
  respsz = hsl_msg_encode_arp_cache_resp (&pnt, &size, &resp);

  /* Post the message. */
  ret = hsl_sock_post_msg (sock, HAL_MSG_ARP_CACHE_GET, 0, hdr->nlmsg_seq, buf, respsz);

  /* Free the buffer. */ 
  kfree (buf);
  kfree (resp.cache);
  HSL_FN_EXIT(ret);

 CLEANUP:
  /* Close socket. */
  hsl_sock_release (sock);
  if(buf)        kfree (buf);
  if(resp.cache) kfree (resp.cache);
  HSL_FN_EXIT(STATUS_ERROR);
}


#ifdef HAVE_IPV6

/* HAL_MSG_IPV6_NBR_ADD message */
int
hsl_msg_recv_ipv6_nbr_add (struct socket *sock, struct hal_nlmsghdr *hdr, char *msgbuf)
{
  struct hal_msg_ipv6_nbr_update *msg = (struct hal_msg_ipv6_nbr_update *)msgbuf;
  int ret = 0;

  HSL_FN_ENTER();
  HSL_LOG (HSL_LOG_MSG, HSL_LEVEL_INFO, "Message: HAL IPV6 neighbor add message \n");

  if (hsl_ipv6_l3_enable_status) /*Update h/w iff L3 fwd is enabled*/
    {
      ret = hsl_fib_ipv6_nbr_add ((hsl_ipv6Address_t *)&msg->addr, 
                                msg->ifindex, msg->mac_addr, 
                                HSL_NH_ENTRY_STATIC|HSL_NH_ENTRY_VALID);
    }
  HSL_MSG_PROCESS_RETURN (sock, hdr, ret);
  HSL_FN_EXIT(ret);
}

/* HAL_MSG_IPV6_NBR_DEL message */
int
hsl_msg_recv_ipv6_nbr_del (struct socket *sock, struct hal_nlmsghdr *hdr, char *msgbuf)
{
  struct hal_msg_ipv6_nbr_update *msg = (struct hal_msg_ipv6_nbr_update *)msgbuf;
  int ret = 0;

  HSL_FN_ENTER();
  HSL_LOG (HSL_LOG_MSG, HSL_LEVEL_INFO, "Message: HAL IPV6 neighbor del message \n");

  if (hsl_ipv6_l3_enable_status) /*Update h/w iff L3 fwd is enabled*/
    {
  ret = hsl_fib_ipv6_nbr_del ((hsl_ipv6Address_t *)&msg->addr, 1,
      msg->ifindex);
    }
  HSL_MSG_PROCESS_RETURN (sock, hdr, ret);
  HSL_FN_EXIT(ret);
}

/* HAL_MSG_IPV6_NBR_DEL_ALL message */
int
hsl_msg_recv_ipv6_nbr_del_all (struct socket *sock, struct hal_nlmsghdr *hdr, char *msgbuf)
{
  int ret;
  struct hal_msg_ipv6_nbr_del_all *msg = (struct hal_msg_ipv6_nbr_del_all *)msgbuf;

  HSL_FN_ENTER();
  HSL_LOG (HSL_LOG_MSG, HSL_LEVEL_INFO, "Message: HAL IPV6 neighbor del all message \n");

  ret = hsl_fib_ipv6_nbr_del_all ((hsl_fib_id_t)msg->fib_id, 
  msg->clr_flag);

  HSL_MSG_PROCESS_RETURN (sock, hdr, ret);
  HSL_FN_EXIT(ret);
}


/*  HAL_MSG_IPV6_NBR_CACHE_GET message. */
int
hsl_msg_recv_ipv6_nbr_cache_get (struct socket *sock, struct hal_nlmsghdr *hdr, char *msgbuf)
{
  u_char *pnt;
  u_int32_t size;
  struct hal_msg_ipv6_nbr_cache_req req;
  struct hal_msg_ipv6_nbr_cache_resp resp;
  int ret = 0, respsz = 0;
  char *buf = NULL; 

  HSL_FN_ENTER();
  HSL_LOG (HSL_LOG_MSG, HSL_LEVEL_INFO, "Message: HAL IPV6 neighbor cache get message.\n");

  memset (&resp, 0, sizeof (struct hal_msg_ipv6_nbr_cache_resp));

  /* Get requests should have ACK flag set. */
  pnt = (u_char*)msgbuf;
  size = hdr->nlmsg_len - HAL_NLMSG_ALIGN(HAL_NLMSGHDR_SIZE);
  /* Decode request. */
  ret = hsl_msg_decode_ipv6_nbr_cache_req(&pnt, &size, &req);
  if (ret < 0)
    goto CLEANUP;

  /* Get buffer for  entries. */
  size = HAL_IPV6_NBR_CACHE_GET_COUNT * sizeof (struct hal_ipv6_nbr_cache_entry);
  resp.cache = (struct hal_ipv6_nbr_cache_entry *)kmalloc (size, GFP_KERNEL);
  if(! resp.cache)
    goto CLEANUP;

  /*  Get entries. */
  ret = hsl_fib_nh6_get_bundle((hsl_fib_id_t)req.fib_id, (hsl_ipv6Address_t *)(HSL_IPV6_ADDR_ZERO (req.addr) 
                                                     ? NULL : &req.addr),
                               req.count, resp.cache);

  /* Allocate buffer to send a response. */
  if((ret < 0) || (ret > HAL_IPV6_NBR_CACHE_GET_COUNT)) 
    ret = 0;

  size = sizeof (resp) + ret * sizeof(struct hal_ipv6_nbr_cache_entry);
  buf = (char *) kmalloc (size, GFP_KERNEL);
  if (!buf)
    goto CLEANUP;
     
  pnt = (u_char*)buf;
  if (ret > 0)
    resp.count = ret;
  else
    resp.count = 0;

  /* Encode arp-cache response. */
  respsz = hsl_msg_encode_ipv6_nbr_cache_resp (&pnt, &size, &resp);

  /* Post the message. */
  ret = hsl_sock_post_msg (sock, HAL_MSG_IPV6_NBR_CACHE_GET, 0, hdr->nlmsg_seq, buf, respsz);

  /* Free the buffer. */ 
  kfree (buf);
  kfree (resp.cache);
  HSL_FN_EXIT(ret);

 CLEANUP:
  /* Close socket. */
  hsl_sock_release (sock);
  if(buf)        kfree (buf);
  if(resp.cache) kfree (resp.cache);
  HSL_FN_EXIT(STATUS_ERROR);
}
#endif /* HAVE_IPV6 */
#endif /* HAVE_L3 */

#ifdef HAVE_MPLS
/*  HAL_MSG_MPLS_INIT message */
int
hsl_msg_recv_mpls_init (struct socket *sock, struct hal_nlmsghdr *hdr, 
                        char *msgbuf)
{
  int ret;

  HSL_LOG (HSL_LOG_MSG, HSL_LEVEL_INFO, "Message: HAL MPLS initialization\n");

  ret = hsl_mpls_init ();
  HSL_MSG_PROCESS_RETURN (sock, hdr, ret);
  return 0;
  
}

/*  HAL_MSG_MPLS_IF_INIT */
int
hsl_msg_recv_mpls_if_init (struct socket *sock, struct hal_nlmsghdr *hdr, 
                           char *msgbuf)
{
  HSL_MSG_PROCESS_RETURN (sock, hdr, 0);
  return 0;
}

/*  HAL_MSG_MPLS_IF_END */
int
hsl_msg_recv_mpls_if_deinit (struct socket *sock, struct hal_nlmsghdr *hdr, 
                             char *msgbuf)
{
  HSL_MSG_PROCESS_RETURN (sock, hdr, 0);
  return 0;
}

/*  HAL_MSG_MPLS_VRF_INIT */
int
hsl_msg_recv_mpls_vrf_init (struct socket *sock, struct hal_nlmsghdr *hdr, 
                           char *msgbuf)
{
  int ret;
  u_int32_t vrf_id;

  vrf_id = *(u_int32_t *)msgbuf;
  ret = hsl_mpls_vpn_add (vrf_id, HSL_MPLS_VPN_VRF);
  HSL_MSG_PROCESS_RETURN (sock, hdr, ret);
  HSL_FN_EXIT(ret);
}

/*  HAL_MSG_MPLS_VRF_END */
int
hsl_msg_recv_mpls_vrf_deinit (struct socket *sock, struct hal_nlmsghdr *hdr, 
                              char *msgbuf)
{
  int ret;
  u_int32_t vrf_id;

  vrf_id = *(u_int32_t *)msgbuf;
  ret = hsl_mpls_vpn_del (vrf_id, HSL_MPLS_VPN_VRF);
  HSL_MSG_PROCESS_RETURN (sock, hdr, ret);
  HSL_FN_EXIT(ret);
}

/*  HAL_MSG_MPLS_NEWILM */
int
hsl_msg_recv_mpls_ilm_add (struct socket *sock, struct hal_nlmsghdr *hdr, 
                           char *msgbuf)
{
  struct hal_msg_mpls_ilm_add *ia;
  int ret;
  u_char vc_type = HAL_MPLS_VC_STYLE_NONE;

  ia = (struct hal_msg_mpls_ilm_add *)msgbuf;

#ifdef HAVE_MPLS_VC
  if (ia->vc_peer && ia->vpn_id && ia->opcode == HAL_MPLS_POP_FOR_VC)
    vc_type = HAL_MPLS_VC_STYLE_MARTINI;
#endif /* HAVE_MPLS_VC */
  ret = hsl_mpls_ilm_add (ia, ia->vpn_id, vc_type);
  HSL_MSG_PROCESS_RETURN (sock, hdr, ret);
  HSL_FN_EXIT(ret);
}

/*  HAL_MSG_MPLS_DELILM */
int
hsl_msg_recv_mpls_ilm_del (struct socket *sock, struct hal_nlmsghdr *hdr, 
                           char *msgbuf)
{
  struct hal_msg_mpls_ilm_del *id;
  int ret;

  id = (struct hal_msg_mpls_ilm_del *)msgbuf;

  ret = hsl_mpls_ilm_del (id);
  HSL_MSG_PROCESS_RETURN (sock, hdr, ret);
  HSL_FN_EXIT(ret);
}


/*  HAL_MSG_MPLS_NEWFTN */
int
hsl_msg_recv_mpls_ftn_add (struct socket *sock, struct hal_nlmsghdr *hdr, 
                           char *msgbuf)
{
  struct hal_msg_mpls_ftn_add fa;
  int ret;
  int *size, len;
  u_char **pnt = (u_char **) &msgbuf;

  len = sizeof (struct hal_msg_mpls_ftn_add);
  size = &len;

  TLV_DECODE_GETC (fa.family);
  TLV_DECODE_GETL (fa.vrf);
  TLV_DECODE_GETL (fa.fec_addr);
  TLV_DECODE_GETC (fa.fec_prefixlen);
  TLV_DECODE_GETC (fa.dscp_in);
  TLV_DECODE_GETL (fa.tunnel_label);
  TLV_DECODE_GETL (fa.tunnel_nhop);
  TLV_DECODE_GETL (fa.tunnel_oif_ix);
  TLV_DECODE_GET (fa.tunnel_oif_name, HAL_IFNAME_LEN + 1);
  TLV_DECODE_GETC (fa.opcode);
  TLV_DECODE_GETL (fa.nhlfe_ix);
  TLV_DECODE_GETL (fa.ftn_ix);
  TLV_DECODE_GETC (fa.ftn_type);
  TLV_DECODE_GETL (fa.tunnel_id);
  TLV_DECODE_GETL (fa.qos_resource_id);
  TLV_DECODE_GETL (fa.bypass_ftn_ix);
  TLV_DECODE_GETC (fa.lsp_type);
  TLV_DECODE_GETL (fa.vpn_label);
  TLV_DECODE_GETL (fa.vpn_nhop);
  TLV_DECODE_GETL (fa.vpn_oif_ix);
  TLV_DECODE_GET (fa.tunnel_oif_name, HAL_IFNAME_LEN + 1);

  ret = hsl_mpls_ftn_add (&fa);
  HSL_MSG_PROCESS_RETURN (sock, hdr, ret);
  HSL_FN_EXIT(ret);
}


/*  HAL_MSG_MPLS_DELFTN */
int
hsl_msg_recv_mpls_ftn_del (struct socket *sock, struct hal_nlmsghdr *hdr, 
                           char *msgbuf)
{
  struct hal_msg_mpls_ftn_del *fd;
  int ret;

  fd = (struct hal_msg_mpls_ftn_del *)msgbuf;
  
  ret = hsl_mpls_ftn_del (fd);
  HSL_MSG_PROCESS_RETURN (sock, hdr, ret);
  HSL_FN_EXIT(ret);
}


#ifdef HAVE_MPLS_VC
/*  HAL_MSG_MPLS_VC_INIT */
int
hsl_msg_recv_mpls_vc_init (struct socket *sock, struct hal_nlmsghdr *hdr, 
                           char *msgbuf)
{
  int ret;
  struct hal_msg_mpls_vpn_if *vif_msg;

  vif_msg = (struct hal_msg_mpls_vpn_if *)msgbuf;
  ret = hsl_mpls_vpn_add (vif_msg->vpn_id, HSL_MPLS_VPN_MARTINI);
  if (ret < 0)
    {
      HSL_MSG_PROCESS_RETURN (sock, hdr, ret);
      HSL_FN_EXIT(ret);
    }

  ret = hsl_mpls_vpn_if_bind (vif_msg->vpn_id, vif_msg->ifindex, 
                              vif_msg->vlan_id, HSL_MPLS_VPN_MARTINI);
  HSL_MSG_PROCESS_RETURN (sock, hdr, ret);
  HSL_FN_EXIT(ret);
  
  return 0;
}

/*  HAL_MSG_MPLS_VC_END */
int
hsl_msg_recv_mpls_vc_deinit (struct socket *sock, struct hal_nlmsghdr *hdr, 
                             char *msgbuf)
{
  int ret;
  struct hal_msg_mpls_vpn_if *vif_msg;

  vif_msg = (struct hal_msg_mpls_vpn_if *)msgbuf;
  hsl_mpls_vpn_if_unbind (vif_msg->vpn_id, vif_msg->ifindex, 
                          vif_msg->vlan_id, HSL_MPLS_VPN_MARTINI);

  ret = hsl_mpls_vpn_del (vif_msg->vpn_id, HSL_MPLS_VPN_MARTINI);
  HSL_MSG_PROCESS_RETURN (sock, hdr, ret);
  HSL_FN_EXIT(ret);
}



int
hsl_msg_recv_mpls_vc_fib_add (struct socket *sock, struct hal_nlmsghdr *hdr,
                              char *msgbuf)
{
  int ret;
  struct hal_msg_mpls_vc_fib_add *vfib_msg;

  vfib_msg = (struct hal_msg_mpls_vc_fib_add *)msgbuf;

  ret = hsl_mpls_vc_fib_add (vfib_msg);

  HSL_MSG_PROCESS_RETURN (sock, hdr, ret);
  HSL_FN_EXIT(ret);
}


/* HAL_MSG_MPLS_VC_FIB_DEL */
int
hsl_msg_recv_mpls_vc_fib_del (struct socket *sock, struct hal_nlmsghdr *hdr,
                              char *msgbuf)
{
  int ret;
  struct hal_msg_mpls_vc_fib_del *vfib_msg;

  vfib_msg = (struct hal_msg_mpls_vc_fib_del *)msgbuf;

  ret = hsl_mpls_vc_fib_del (vfib_msg);
  HSL_MSG_PROCESS_RETURN (sock, hdr, ret);
  HSL_FN_EXIT(ret);
}
#endif /* HAVE_MPLS_VC */

#ifdef HAVE_VPLS
/* HAL_MSG_MPLS_VPLS_ADD */
int
hsl_msg_recv_mpls_vpls_add (struct socket *sock, struct hal_nlmsghdr *hdr,
                            char *msgbuf)
{
  int ret;
  u_int32_t vpls_id;

  vpls_id = *(u_int32_t *)msgbuf;
  ret = hsl_mpls_vpn_add (vpls_id, HSL_MPLS_VPN_VPLS);
  HSL_MSG_PROCESS_RETURN (sock, hdr, ret);
  HSL_FN_EXIT(ret);
}

/* HAL_MSG_MPLS_VPLS_DEL */
int
hsl_msg_recv_mpls_vpls_del (struct socket *sock, struct hal_nlmsghdr *hdr,
                            char *msgbuf)
{
  int ret;
  u_int32_t vpls_id;

  vpls_id = *(u_int32_t *)msgbuf;
  ret = hsl_mpls_vpn_del (vpls_id, HSL_MPLS_VPN_VPLS);
  HSL_MSG_PROCESS_RETURN (sock, hdr, ret);
  HSL_FN_EXIT(ret);
}

/* HAL_MSG_MPLS_VPLS_IF_BIND */
int
hsl_msg_recv_mpls_vpls_if_bind (struct socket *sock, struct hal_nlmsghdr *hdr,
                                char *msgbuf)
{
  int ret;
  struct hal_msg_mpls_vpn_if *vif_msg;

  vif_msg = (struct hal_msg_mpls_vpn_if *)msgbuf;

  ret = hsl_mpls_vpn_if_bind (vif_msg->vpn_id, vif_msg->ifindex, 
                              vif_msg->vlan_id, HSL_MPLS_VPN_VPLS);
  HSL_MSG_PROCESS_RETURN (sock, hdr, ret);
  HSL_FN_EXIT(ret);
}


/* HAL_MSG_MPLS_VPLS_IF_UNBIND */
int
hsl_msg_recv_mpls_vpls_if_unbind (struct socket *sock, struct hal_nlmsghdr *hdr,
                                  char *msgbuf)
{
  int ret;
  struct hal_msg_mpls_vpn_if *vif_msg;

  vif_msg = (struct hal_msg_mpls_vpn_if *)msgbuf;

  ret = hsl_mpls_vpn_if_unbind (vif_msg->vpn_id, vif_msg->ifindex, 
                                vif_msg->vlan_id, HSL_MPLS_VPN_VPLS);
  HSL_MSG_PROCESS_RETURN (sock, hdr, ret);
  HSL_FN_EXIT(ret);
}

#endif /* HAVE_VPLS */
#endif /* HAVE_MPLS */

#ifdef HAVE_L2LERN
int
hsl_msg_recv_mac_access_grp_set (struct socket *sock, struct hal_nlmsghdr *hdr, char *msgbuf)
{
  struct hal_msg_mac_set_access_grp *msg = (struct hal_msg_mac_set_access_grp *)msgbuf;
  int ret;

  ret = hsl_bcm_mac_set_access_grp (msg);
  HSL_MSG_PROCESS_RETURN (sock, hdr, ret);
  return 0;

}

int
hsl_msg_recv_vlan_access_map_set (struct socket *sock, struct hal_nlmsghdr *hdr, char *msgbuf)
{
  struct hal_msg_vlan_set_access_map *msg = (struct hal_msg_vlan_set_access_map *)msgbuf;
  int ret;

  ret = hsl_bcm_vlan_set_access_map (msg);
  HSL_MSG_PROCESS_RETURN (sock, hdr, ret);
  return 0;
}
#endif /* HAVE_L2LERN */

int
hsl_msg_recv_ip_set_acl_filter (struct socket *sock,
                                struct hal_nlmsghdr *hdr, char *msgbuf)
{
  struct hal_msg_ip_set_access_grp *msg =
                                   (struct hal_msg_ip_set_access_grp *) msgbuf;
  int ret;

  ret = hsl_bcm_set_ip_access_group (msg);
  HSL_MSG_PROCESS_RETURN (sock, hdr, ret);
  return 0;
}

int
hsl_msg_recv_ip_unset_acl_filter (struct socket *sock,
                                  struct hal_nlmsghdr *hdr, char *msgbuf)
{
  struct hal_msg_ip_set_access_grp *msg = (struct
                                       hal_msg_ip_set_access_grp *) msgbuf;
  int ret;

  ret = hsl_bcm_unset_ip_access_group (msg);
  HSL_MSG_PROCESS_RETURN (sock, hdr, ret);
  return 0;
}

int
hsl_msg_recv_ip_set_acl_filter_interface (struct socket *sock,
                                          struct hal_nlmsghdr *hdr,
                                          char *msgbuf)
{
  struct hal_msg_ip_set_access_grp_interface *msg =
                         (struct hal_msg_ip_set_access_grp_interface *) msgbuf;
  int ret;

  ret = hsl_bcm_set_ip_access_group_interface (msg);
  HSL_MSG_PROCESS_RETURN (sock, hdr, ret);
  return 0;
}

int
hsl_msg_recv_ip_unset_acl_filter_interface (struct socket *sock,
                                            struct hal_nlmsghdr *hdr,
                                            char *msgbuf)
{
  struct hal_msg_ip_set_access_grp_interface *msg =
                         (struct hal_msg_ip_set_access_grp_interface *) msgbuf;
  int ret;

  ret = hsl_bcm_unset_ip_access_group_interface (msg);
  HSL_MSG_PROCESS_RETURN (sock, hdr, ret);
  return 0;
}

/* CPU related HSL API's */
int
hsl_msg_recv_cpu_set_master (struct socket *sock, struct hal_nlmsghdr *hdr, 
                             char *msgbuf)
{
  int ret;
  unsigned char *msg = (unsigned char *) msgbuf;

  ret = hsl_bcm_set_master_cpu (msg);
  if (ret < 0)
  {
     HSL_MSG_PROCESS_RETURN (sock, hdr, ret);
  }
  else
  {
      hsl_sock_post_msg (sock, HAL_MSG_CPU_SET_MASTER, hdr->nlmsg_seq, 0,
         (char *)msg, sizeof (u_int32_t));
  }

  return 0;
}

int
hsl_msg_recv_cpu_get_info_index (struct socket *sock, struct hal_nlmsghdr *hdr, 
                                 char *msgbuf)
{
  int ret;
  unsigned int num;
  struct hal_cpu_info_entry cpu_info;

  memcpy(&num, msgbuf, sizeof(int)) ;

  ret = hsl_bcm_get_cpu_index (num, (char *)&cpu_info.mac_addr);
  if (ret < 0)
  {
      HSL_MSG_PROCESS_RETURN (sock, hdr, ret);
  }
  else
  {
      hsl_sock_post_msg (sock, HAL_MSG_CPU_GETDB_INDEX, 0, hdr->nlmsg_seq, 
       (char *)&cpu_info, sizeof (struct hal_cpu_info_entry));
  }
  return 0;
}

int
hsl_msg_recv_cpu_get_dump_index (struct socket *sock, struct hal_nlmsghdr *hdr, 
                                 char *msgbuf)
{
  int ret;
  unsigned int num;
  struct hal_cpu_dump_entry cpu_info;

  memcpy(&num, msgbuf, sizeof(int)) ;
  ret = hsl_bcm_get_dump_cpu_index (num, (char *)&cpu_info);
  if (ret < 0)
  {
      HSL_MSG_PROCESS_RETURN (sock, hdr, ret);
  }
  else
  {
     hsl_sock_post_msg (sock, hdr->nlmsg_type, hdr->nlmsg_seq, 0,
              (char *)&cpu_info, sizeof (struct hal_cpu_dump_entry));
  }
  return 0;
}

int
hsl_msg_recv_cpu_get_num (struct socket *sock, struct hal_nlmsghdr *hdr, char *msgbuf)
{
  int ret;
  unsigned int num;

  ret = hsl_bcm_get_num_cpu (&num);
  if (ret < 0)
  {
      HSL_MSG_PROCESS_RETURN (sock, hdr, ret);
  }
  else
  {
     hsl_sock_post_msg (sock, hdr->nlmsg_type, hdr->nlmsg_seq, 0,
              (char *)&num, sizeof (u_int32_t)); 
  }

  return 0;
}

int
hsl_msg_recv_cpu_getdb_info (struct socket *sock, struct hal_nlmsghdr *hdr, char *msgbuf)
{
  return 0;
}


int
hsl_msg_recv_cpu_get_master (struct socket *sock, struct hal_nlmsghdr *hdr, 
                             char *msgbuf)
{
  int ret;
  struct hal_cpu_info_entry cpu_info;

  ret = hsl_bcm_get_master_cpu ((char *)&cpu_info.mac_addr);
  if (ret < 0)
  {
     HSL_MSG_PROCESS_RETURN (sock, hdr, ret);
  }
  else
  {
      hsl_sock_post_msg (sock, HAL_MSG_CPU_GET_MASTER, 0, hdr->nlmsg_seq,
          (char *)&cpu_info, sizeof (struct hal_cpu_info_entry));
  }
  
  return 0;
}


int
hsl_msg_recv_cpu_get_local (struct socket *sock, struct hal_nlmsghdr *hdr, 
                            char *msgbuf)
{
  int ret;
  struct hal_cpu_info_entry cpu_info;

  ret = hsl_bcm_get_local_cpu ((char *)&cpu_info.mac_addr);
  if (ret < 0)
  {
     HSL_MSG_PROCESS_RETURN (sock, hdr, ret);
  }
  else
  {
      hsl_sock_post_msg (sock, HAL_MSG_CPU_GET_LOCAL, 0, hdr->nlmsg_seq,
          (char *)&cpu_info, sizeof (struct hal_cpu_info_entry));
  }

  return 0;
}

/* djg */
int
hsl_msg_recv_ptp_set_clock_type (struct socket *sock, struct hal_nlmsghdr *hdr, char *msgbuf)
{
  struct hal_msg_ptp_clk_type *msg = (struct hal_msg_ptp_clk_type *)msgbuf;

  HSL_LOG (HSL_LOG_MSG, HSL_LEVEL_INFO, "Message: PTP clock type set message \n");
  hsl_ptp_set_clock_type(msg->type);
  HSL_MSG_PROCESS_RETURN (sock, hdr, 0);
  return 0;
}

int 
hsl_msg_recv_ptp_create_domain(struct socket *sock, struct hal_nlmsghdr *hdr, char *msgbuf)
{
  unsigned int *domain = (unsigned int *)msgbuf;

  HSL_LOG (HSL_LOG_MSG, HSL_LEVEL_INFO, "Message: PTP domain create message \n");
  hsl_ptp_create_domain(*domain);
  HSL_MSG_PROCESS_RETURN (sock, hdr, 0);
  return 0;
}


int
hsl_msg_recv_ptp_domain_add_mport (struct socket *sock, struct hal_nlmsghdr *hdr, char *msgbuf)
{
  struct hal_msg_ptp_domain_port *msg = (struct hal_msg_ptp_domain_port *)msgbuf;

  HSL_LOG (HSL_LOG_MSG, HSL_LEVEL_INFO, "Message: PTP domain mport add message \n");
  hsl_ptp_set_domain_mport(msg->domain, msg->port);
  HSL_MSG_PROCESS_RETURN (sock, hdr, 0);
  return 0;
}

int
hsl_msg_recv_ptp_domain_add_sport (struct socket *sock, struct hal_nlmsghdr *hdr, char *msgbuf)
{
  struct hal_msg_ptp_domain_port *msg = (struct hal_msg_ptp_domain_port *)msgbuf;

  HSL_LOG (HSL_LOG_MSG, HSL_LEVEL_INFO, "Message: PTP domain sport add message \n");
  hsl_ptp_set_domain_sport(msg->domain, msg->port);
  HSL_MSG_PROCESS_RETURN (sock, hdr, 0);
  return 0;
}

int
hsl_msg_recv_ptp_set_delay_mech (struct socket *sock, struct hal_nlmsghdr *hdr, char *msgbuf)
{
  struct hal_msg_ptp_delay_mech *msg = (struct hal_msg_ptp_delay_mech *)msgbuf;

  HSL_LOG (HSL_LOG_MSG, HSL_LEVEL_INFO, "Message: PTP delay mech set message \n");
  hsl_ptp_set_delay_mech(msg->delay_mech);
  HSL_MSG_PROCESS_RETURN (sock, hdr, 0);
  return 0;
}

int
hsl_msg_recv_ptp_set_phyport (struct socket *sock, struct hal_nlmsghdr *hdr, char *msgbuf)
{
  struct hal_msg_ptp_phyport *msg = (struct hal_msg_ptp_phyport *)msgbuf;

  HSL_LOG (HSL_LOG_MSG, HSL_LEVEL_INFO, "Message: PTP port's phyport set message \n");
  hsl_ptp_set_phyport(msg->vport, msg->phyport);
  HSL_MSG_PROCESS_RETURN (sock, hdr, 0);
  return 0;
}

int
hsl_msg_recv_ptp_clear_phyport (struct socket *sock, struct hal_nlmsghdr *hdr, char *msgbuf)
{
  unsigned int *vport = (unsigned int *)msgbuf;

  HSL_LOG (HSL_LOG_MSG, HSL_LEVEL_INFO, "Message: PTP port's phyport clear message \n");
  hsl_ptp_clear_phyport(*vport);
  HSL_MSG_PROCESS_RETURN (sock, hdr, 0);	
  return 0;
}

int
hsl_msg_recv_ptp_enable_port (struct socket *sock, struct hal_nlmsghdr *hdr, char *msgbuf)
{
  unsigned int *vport = (unsigned int *)msgbuf;

  HSL_LOG (HSL_LOG_MSG, HSL_LEVEL_INFO, "Message: PTP port enable message \n");
  hsl_ptp_enable_port(*vport);
  HSL_MSG_PROCESS_RETURN (sock, hdr, 0);
  return 0;
}

int
hsl_msg_recv_ptp_disable_port (struct socket *sock, struct hal_nlmsghdr *hdr, char *msgbuf)
{
  unsigned int *vport = (unsigned int *)msgbuf;

  HSL_LOG (HSL_LOG_MSG, HSL_LEVEL_INFO, "Message: PTP port disable message \n");
  hsl_ptp_disable_port(*vport);
  HSL_MSG_PROCESS_RETURN (sock, hdr, 0);
  return 0;
}

int
hsl_msg_recv_ptp_port_delay_mech (struct socket *sock, struct hal_nlmsghdr *hdr, char *msgbuf)
{
  struct hal_msg_ptp_port_delay_mech *msg = (struct hal_msg_ptp_port_delay_mech *)msgbuf;

  HSL_LOG (HSL_LOG_MSG, HSL_LEVEL_INFO, "Message: PTP port delay mech set message \n");
  hsl_ptp_set_port_delay_mech(msg->vport, msg->delay_mech);
  HSL_MSG_PROCESS_RETURN (sock, hdr, 0);
  return 0;
}

int
hsl_msg_recv_ptp_set_req_interval (struct socket *sock, struct hal_nlmsghdr *hdr, char *msgbuf)
{
  struct hal_msg_ptp_req_interval *msg = (struct hal_msg_ptp_req_interval *)msgbuf;

  HSL_LOG (HSL_LOG_MSG, HSL_LEVEL_INFO, "Message: PTP port delay interval set message \n");
  hsl_ptp_set_req_delay_interval(msg->vport, msg->interval);
  HSL_MSG_PROCESS_RETURN (sock, hdr, 0);
  return 0;
}

int
hsl_msg_recv_ptp_set_p2p_interval (struct socket *sock, struct hal_nlmsghdr *hdr, char *msgbuf)
{
  struct hal_msg_p2p_delay *msg = (struct hal_msg_p2p_delay *)msgbuf;

  HSL_LOG (HSL_LOG_MSG, HSL_LEVEL_INFO, "Message: PTP port p2p delay interval set message \n");
  hsl_ptp_set_p2p_interval( msg->vport, msg->delay );
  HSL_MSG_PROCESS_RETURN (sock, hdr, 0);
  return 0;
}

int		  
hsl_msg_recv_ptp_set_p2p_meandelay (struct socket *sock, struct hal_nlmsghdr *hdr, char *msgbuf)
{
  struct hal_msg_p2p_delay *msg = (struct hal_msg_p2p_delay *)msgbuf;

  HSL_LOG (HSL_LOG_MSG, HSL_LEVEL_INFO, "Message: PTP port p2p mean delay set message \n");
  hsl_ptp_set_p2p_meandelay( msg->vport, msg->delay );
  HSL_MSG_PROCESS_RETURN (sock, hdr, 0);
  return 0;
}

int
hsl_msg_recv_ptp_set_asym_delay (struct socket *sock, struct hal_nlmsghdr *hdr, char *msgbuf)
{
  struct hal_msg_p2p_delay *msg = (struct hal_msg_p2p_delay *)msgbuf;

  HSL_LOG (HSL_LOG_MSG, HSL_LEVEL_INFO, "Message: PTP port p2p asymmetry delay  set message \n");
  hsl_ptp_set_asym_delay(msg->vport, msg->delay);
  HSL_MSG_PROCESS_RETURN (sock, hdr, 0);
  return 0;
}
#ifdef HAVE_TUNNEL
/*Tunnel*/
int
hsl_msg_recv_tunnel_add (struct socket *sock, struct hal_nlmsghdr *hdr, char *msgbuf)
{
  int ret;
  struct hal_msg_tunnel_add_if *l3_ifp;

  HSL_FN_ENTER();

  HSL_LOG (HSL_LOG_MSG, HSL_LEVEL_INFO,
           "Message: HAL message Tunnel Add \n");

  l3_ifp  = (struct hal_msg_tunnel_add_if *) msgbuf;
  ret = hsl_tunnel_if_add (l3_ifp);

  HSL_MSG_PROCESS_RETURN (sock, hdr, ret);

  HSL_FN_EXIT(ret);
}

int
hsl_msg_recv_tunnel_delete (struct socket *sock, struct hal_nlmsghdr *hdr, char *msgbuf)
{
  int ret;
  struct hal_msg_tunnel_if *ifp;

  HSL_FN_ENTER();

  HSL_LOG (HSL_LOG_MSG, HSL_LEVEL_INFO,
           "Message: HAL message Tunnel Delete \n");

  ifp = (struct hal_msg_tunnel_if *)msgbuf;

  ret = hsl_tunnel_if_delete (ifp);

  HSL_MSG_PROCESS_RETURN (sock, hdr, ret);

  HSL_FN_EXIT(ret);

}

int
hsl_msg_recv_tunnel_initiator_set (struct socket *sock, struct hal_nlmsghdr *hdr, char *msgbuf)
{
  int ret;
  struct hal_msg_tunnel_if *ifp;

  HSL_FN_ENTER();

  HSL_LOG (HSL_LOG_MSG, HSL_LEVEL_INFO,
           "Message: HAL message Tunnel Delete \n");

  ifp = (struct hal_msg_tunnel_if *)msgbuf;

  ret = hsl_tunnel_initiator_set (ifp);

  HSL_MSG_PROCESS_RETURN (sock, hdr, ret);

  HSL_FN_EXIT(ret);

}

int
hsl_msg_recv_tunnel_initiator_clear (struct socket *sock, struct hal_nlmsghdr *hdr, char *msgbuf)
{
  int ret;
  struct hal_msg_tunnel_if *ifp;

  HSL_FN_ENTER();

  HSL_LOG (HSL_LOG_MSG, HSL_LEVEL_INFO,
           "Message: HAL message Tunnel Delete \n");

  ifp = (struct hal_msg_tunnel_if *)msgbuf;

  ret = hsl_tunnel_initiator_clear (ifp);

  HSL_MSG_PROCESS_RETURN (sock, hdr, ret);

  HSL_FN_EXIT(ret);

}

int
hsl_msg_recv_tunnel_terminator_set (struct socket *sock, struct hal_nlmsghdr *hdr, char *msgbuf)
{
  int ret;
  struct hal_msg_tunnel_if *ifp;

  HSL_FN_ENTER();

  HSL_LOG (HSL_LOG_MSG, HSL_LEVEL_INFO,
           "Message: HAL message Tunnel Delete \n");

  ifp = (struct hal_msg_tunnel_if *)msgbuf;

  ret = hsl_tunnel_terminator_set (ifp);

  HSL_MSG_PROCESS_RETURN (sock, hdr, ret);

  HSL_FN_EXIT(ret);

}

int
hsl_msg_recv_tunnel_terminator_clear (struct socket *sock, struct hal_nlmsghdr *hdr, char *msgbuf)
{
  int ret;
  struct hal_msg_tunnel_if *ifp;

  HSL_FN_ENTER();

  HSL_LOG (HSL_LOG_MSG, HSL_LEVEL_INFO,
           "Message: HAL message Tunnel Delete \n");

  ifp = (struct hal_msg_tunnel_if *)msgbuf;

  ret = hsl_tunnel_terminator_clear (ifp);

  HSL_MSG_PROCESS_RETURN (sock, hdr, ret);

  HSL_FN_EXIT(ret);

}

#endif /* HAVE_TUNNEL*/

//#ifdef HAVE_VPWS   /*cai 2011-10 vpws module*/
#if 0
int
hsl_msg_vpws_pe_create (struct socket *sock, struct hal_nlmsghdr *hdr, char *msgbuf)
{
  int ret;
  struct hal_msg_vll_ingress *ifp;
  struct hsl_bcm_msg_vpws_s vpws;

  HSL_FN_ENTER();
  HSL_LOG (HSL_LOG_MSG, HSL_LEVEL_INFO,
           "Message: HAL message vpws add \n");

  ifp = (struct hal_msg_vll_ingress *)msgbuf;

  /*init needed input inoformation*/
  vpws.vpws_id = ifp->vllid;
  vpws.ac_port = ifp->inport;
  vpws.ac_vlan = ifp->vlan;	
	
  vpws.out_port = ifp->outport;
  vpws.out_lsp_label = ifp->outlabel;
  vpws.out_lsp_exp = ifp->tunexp;
  vpws.out_pw_label = ifp->pwlabel;
  vpws.out_pw_exp = ifp->pwexp;
  vpws.match_pw_label = ifp->remote_pw;
  vpws.match_lsp_label = ifp->remote_tunnel;

  ret = hsl_bcm_vpws_pe_create ( &vpws );

  HSL_MSG_PROCESS_RETURN (sock, hdr, ret);

  HSL_FN_EXIT(ret);

}

int
hsl_msg_vpws_p_create (struct socket *sock, struct hal_nlmsghdr *hdr, char *msgbuf)
{
  int ret;
  struct hal_msg_vll_egress *ifp;
  struct hsl_bcm_msg_vpws_s temp_vpws;

  HSL_FN_ENTER();

  HSL_LOG (HSL_LOG_MSG, HSL_LEVEL_INFO,
           "Message: HAL message vpws add \n");
  ifp = (struct hal_msg_vpws *)msgbuf;

	/*init needed input inoformation*/
	temp_vpws.ac_port = 14;
	temp_vpws.ac_vlan = 5;	
	
	//temp_vpws.in_tunnel = 100;
	//temp_vpws.in_pw = 10;
	temp_vpws.match_pw_label =20;

	temp_vpws.out_port = 15;
	temp_vpws.out_lsp_label= 100;
	temp_vpws.out_lsp_exp = 7;
	temp_vpws.out_pw_label  = 10;
	temp_vpws.out_pw_exp = 7;

  ret = hsl_bcm_vpws_p_create ( ifp );

  HSL_MSG_PROCESS_RETURN (sock, hdr, ret);



  HSL_FN_EXIT(ret);
}

int
hsl_msg_vpws_delete  (struct socket *sock, struct hal_nlmsghdr *hdr, char *msgbuf)
{
  int ret;
  int *ifp;

  HSL_FN_ENTER();

  HSL_LOG (HSL_LOG_MSG, HSL_LEVEL_INFO,
           "Message: HAL message vpws Delete \n");

  ifp = ( int *)msgbuf;
  ret = hsl_bcm_vpws_delete ( ifp );

  HSL_MSG_PROCESS_RETURN (sock, hdr, ret);

  HSL_FN_EXIT(ret);
}

//#endif /* Have_vpws*/
int
hsl_msg_vpls_vpn_add (struct socket *sock, struct hal_nlmsghdr *hdr, char *msgbuf)
{
  int ret;
  struct hal_msg_vpls_vpn_s *ifp;

  HSL_FN_ENTER();

  HSL_LOG (HSL_LOG_MSG, HSL_LEVEL_INFO,
           "Message: HAL message hsl_msg_vpls_vpn_add \n");

  ifp = ( struct hal_msg_vpls_vpn_s *)msgbuf;
  ret = hsl_bcm_vpls_vpn_add ( ifp );

/*
  int *ifp;
  
	HSL_FN_ENTER();
  
	HSL_LOG (HSL_LOG_MSG, HSL_LEVEL_INFO,
			 "Message: HAL message hsl_msg_vpls_vpn_add \n");
  
	ifp = (int *)msgbuf;
	ret = hsl_bcm_vpls_vpn_add ( ifp );
*/

  HSL_MSG_PROCESS_RETURN (sock, hdr, ret);

  HSL_FN_EXIT(ret);
}

int
hsl_msg_vpls_vpn_del (struct socket *sock, struct hal_nlmsghdr *hdr, char *msgbuf)		
{
   int ret;
   struct hal_msg_vpls_vpn_s *ifp;
 
   HSL_FN_ENTER();
 
   HSL_LOG (HSL_LOG_MSG, HSL_LEVEL_INFO,
			"Message: HAL message hsl_msg_vpls_vpn_del \n");
 
   ifp = ( struct hal_msg_vpls_vpn_s *)msgbuf;
   ret = hsl_bcm_vpls_vpn_del ( ifp );
 
   HSL_MSG_PROCESS_RETURN (sock, hdr, ret);
 
   HSL_FN_EXIT(ret);
 }

 
int
hsl_msg_vpls_port_add (struct socket *sock, struct hal_nlmsghdr *hdr, char *msgbuf)
{
   int ret;
   struct hal_msg_vpls_port_s *ifp;
   
   HSL_FN_ENTER();
   HSL_LOG (HSL_LOG_MSG, HSL_LEVEL_INFO,
			"Message: HAL message hsl_msg_vpls_port_add \n");
   
   ifp = (struct hal_msg_vpls_port_s *)msgbuf;

   ret = hsl_bcm_vpls_port_add ( ifp );
 
   HSL_MSG_PROCESS_RETURN (sock, hdr, ret);
 
   HSL_FN_EXIT(ret);
}

int
hsl_msg_vpls_port_del (struct socket *sock, struct hal_nlmsghdr *hdr, char *msgbuf)
{
   int ret;
   struct hal_msg_vpls_port_s *ifp;
 
   HSL_FN_ENTER();
 
   HSL_LOG (HSL_LOG_MSG, HSL_LEVEL_INFO,
			"Message: HAL message hsl_msg_vpls_port_del \n");
 
   ifp = ( struct hal_msg_vpls_port_s *)msgbuf;
   ret = hsl_bcm_vpls_port_del ( ifp );
 
   HSL_MSG_PROCESS_RETURN (sock, hdr, ret);
 
   HSL_FN_EXIT(ret);
}
#endif

int hsl_msg_vpn_add(struct socket *sock, struct hal_nlmsghdr *hdr, char *msgbuf)
{
      int ret = 0;
      hsl_bcm_msg_vpn_cfg *hsl_vpn_info = NULL;

      HSL_FN_ENTER();
 
      HSL_LOG (HSL_LOG_MSG, HSL_LEVEL_INFO,
                "Message: HAL message hsl_msg_vpn_add \n");

   

      hsl_vpn_info = (hsl_bcm_msg_vpn_cfg *)msgbuf;
      
      HSL_LOG (HSL_LOG_MSG, HSL_LEVEL_INFO,
               "hsl_msg_vpn_add:  \n");
      
      ret = hsl_bcm_vpn_create(hsl_vpn_info);

      HSL_LOG (HSL_LOG_MSG, HSL_LEVEL_INFO,
                      "Message: HAL message hsl_msg_vpn_add  rc = %x\n", ret);
  
      HSL_MSG_PROCESS_RETURN (sock, hdr, ret);
      HSL_FN_EXIT(ret);
}

int hsl_msg_vpn_del(struct socket *sock, struct hal_nlmsghdr *hdr, char *msgbuf)
{
      int ret = 0;
      hsl_bcm_msg_vpn_cfg *hsl_vpn_info = NULL;

      HSL_FN_ENTER();
 
      HSL_LOG (HSL_LOG_MSG, HSL_LEVEL_INFO,
			"Message: HAL message hsl_msg_vpn_del \n");

      hsl_vpn_info = (hsl_bcm_msg_vpn_cfg *)msgbuf;
      ret = hsl_bcm_vpn_del(hsl_vpn_info);

   HSL_LOG (HSL_LOG_MSG, HSL_LEVEL_INFO,
                   "Message: HAL message hsl_msg_vpn_del  rc = %x\n", ret);
   
      HSL_MSG_PROCESS_RETURN (sock, hdr, ret);
      HSL_FN_EXIT(ret);
}

/* add from hsl-0213 */
int hsl_msg_vpn_port_add(struct socket *sock, struct hal_nlmsghdr *hdr, char *msgbuf)
{
    int ret = 0;
    hsl_bcm_msg_vpn_port_cfg *hsl_vpn_port_info = NULL;
	
    HSL_FN_ENTER();

    HSL_LOG (HSL_LOG_MSG, HSL_LEVEL_INFO,
    "Message: HAL message hsl_msg_vpn_port_add \n");

    hsl_vpn_port_info = (hsl_bcm_msg_vpn_port_cfg *)msgbuf;
    
    HSL_LOG (HSL_LOG_MSG, HSL_LEVEL_INFO,
        "hsl_msg_vpn_port_add: hsl_vpn_id = %d\n",  hsl_vpn_port_info->hsl_vpn_id);
    HSL_LOG (HSL_LOG_MSG, HSL_LEVEL_INFO,
        "hsl_msg_vpn_port_add: svp_port_type = %d\n", hsl_vpn_port_info->svp_port_type);

    HSL_LOG (HSL_LOG_MSG, HSL_LEVEL_INFO,
        "hsl_msg_vpn_port_add: pannel_port = %d\n",  hsl_vpn_port_info->pannel_port);
    HSL_LOG (HSL_LOG_MSG, HSL_LEVEL_INFO,
        "hsl_msg_vpn_port_add: vpn_port_attr = %d\n",  hsl_vpn_port_info->vpn_port_attr);
    HSL_LOG (HSL_LOG_MSG, HSL_LEVEL_INFO,
        "hsl_msg_vpn_port_add: out_disable = %d\n",  hsl_vpn_port_info->out_disable);

    HSL_LOG (HSL_LOG_MSG, HSL_LEVEL_INFO,
        "hsl_msg_vpn_port_add: in_disable = %d\n",  hsl_vpn_port_info->in_disable);
    HSL_LOG (HSL_LOG_MSG, HSL_LEVEL_INFO,
        "hsl_msg_vpn_port_add: ac_vlan = %d\n",  hsl_vpn_port_info->ac_vlan);
    HSL_LOG (HSL_LOG_MSG, HSL_LEVEL_INFO,
        "hsl_msg_vpn_port_add: in_pw_label = %d\n",  hsl_vpn_port_info->in_pw_label);

    HSL_LOG (HSL_LOG_MSG, HSL_LEVEL_INFO,
        "hsl_msg_vpn_port_add: out_pw_exp = %d\n",  hsl_vpn_port_info->out_pw_exp);
    
    HSL_LOG (HSL_LOG_MSG, HSL_LEVEL_INFO,
        "hsl_msg_vpn_port_add: out_pw_label = %d\n",  hsl_vpn_port_info->out_pw_label);
	
	HSL_LOG (HSL_LOG_MSG, HSL_LEVEL_INFO,
            "hsl_msg_vpn_port_add: lsp_id = %d\n",  hsl_vpn_port_info->lsp_id);


    if(hsl_vpn_port_info->svp_port_type == 0)
    {
        hsl_bcm_ac_port_info  stHslVpnAcCfg;
		//hsl_bcm_ac_port_info_1307 stHslVpnAcCfg_1307;//ac port is sucess 
		stHslVpnAcCfg.hsl_vpn_id = hsl_vpn_port_info->hsl_vpn_id;
        stHslVpnAcCfg.ac_port = hsl_vpn_port_info->pannel_port;
        stHslVpnAcCfg.ac_vlan = hsl_vpn_port_info->ac_vlan;
        stHslVpnAcCfg.in_disable = hsl_vpn_port_info->in_disable;
        stHslVpnAcCfg.out_disable = hsl_vpn_port_info->out_disable;
        stHslVpnAcCfg.out_disable = hsl_vpn_port_info->out_disable;
        stHslVpnAcCfg.vpn_port_attr = hsl_vpn_port_info->vpn_port_attr;
		
        ret = hsl_bcm_vpn_ac_port_add( &stHslVpnAcCfg);
    }
    else
    {
        hsl_bcm_pw_port_info stHslVpnPwCfg;

        stHslVpnPwCfg.hsl_vpn_id = hsl_vpn_port_info->hsl_vpn_id;
        stHslVpnPwCfg.in_disable  = hsl_vpn_port_info->in_disable;
        stHslVpnPwCfg.match_pw_label = hsl_vpn_port_info->in_pw_label;
        stHslVpnPwCfg.out_disable = hsl_vpn_port_info->out_disable;
      
        stHslVpnPwCfg.out_port = hsl_vpn_port_info->pannel_port;
        stHslVpnPwCfg.out_pw_exp = hsl_vpn_port_info->out_pw_exp;
        stHslVpnPwCfg.out_pw_label = hsl_vpn_port_info->out_pw_label;
        stHslVpnPwCfg.vpn_port_attr = hsl_vpn_port_info->vpn_port_attr;
        stHslVpnPwCfg.lsp_id = hsl_vpn_port_info->lsp_id;
		
		hsl_bcm_pw_port_info_1307 stHslVpnPwCfg_1307;

		stHslVpnPwCfg_1307.hsl_vpn_id = stHslVpnPwCfg.hsl_vpn_id;
		stHslVpnPwCfg_1307.match_pw_label = stHslVpnPwCfg.match_pw_label;
		stHslVpnPwCfg_1307.in_disable = stHslVpnPwCfg.in_disable;
		stHslVpnPwCfg_1307.out_port = stHslVpnPwCfg.out_port;
		stHslVpnPwCfg_1307.out_pw_label = stHslVpnPwCfg.out_pw_label;
		stHslVpnPwCfg_1307.out_pw_exp =  stHslVpnPwCfg.out_pw_exp;
   		stHslVpnPwCfg_1307.vpn_port_attr = stHslVpnPwCfg.vpn_port_attr;
		stHslVpnPwCfg_1307.out_disable = stHslVpnPwCfg.out_disable;
		stHslVpnPwCfg_1307.lsp_id = stHslVpnPwCfg.lsp_id;
		
		ret = hsl_bcm_vpn_pw_port_add( &stHslVpnPwCfg_1307 );

		
    }

     HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO,
                   "Message: HAL message hsl_msg_vpn_port_add  rc = %x\n", ret);

    HSL_MSG_PROCESS_RETURN (sock, hdr, ret);
    HSL_FN_EXIT(ret);

}
/* add from hsl-0213 */

int hsl_msg_vpn_port_del(struct socket *sock, struct hal_nlmsghdr *hdr, char *msgbuf)
{
       int ret = 0;
      hsl_bcm_msg_vpn_port_cfg *hsl_vpn_port_info = NULL;
      
    HSL_FN_ENTER();

    HSL_LOG (HSL_LOG_MSG, HSL_LEVEL_INFO,
                "Message: HAL message hsl_msg_vpn_port_del \n");

    hsl_vpn_port_info = (hsl_bcm_msg_vpn_port_cfg *)msgbuf;

    if(hsl_vpn_port_info->svp_port_type == 0)
    {
        hsl_bcm_ac_port_info  stHslVpnAcCfg;

        stHslVpnAcCfg.ac_port = hsl_vpn_port_info->pannel_port;
        stHslVpnAcCfg.ac_vlan = hsl_vpn_port_info->ac_vlan;
        stHslVpnAcCfg.hsl_vpn_id = hsl_vpn_port_info->hsl_vpn_id;
        stHslVpnAcCfg.in_disable = hsl_vpn_port_info->in_disable;
        stHslVpnAcCfg.mpls_port_id = 0;
        stHslVpnAcCfg.out_disable = hsl_vpn_port_info->out_disable;
        stHslVpnAcCfg.vpn_port_attr = hsl_vpn_port_info->vpn_port_attr;
        ret = hsl_bcm_vpn_ac_port_del(&stHslVpnAcCfg);
    }
    else
    {
        hsl_bcm_pw_port_info stHslVpnPwCfg;

        stHslVpnPwCfg.hsl_vpn_id = hsl_vpn_port_info->hsl_vpn_id;
        stHslVpnPwCfg.in_disable  = hsl_vpn_port_info->in_disable;
        stHslVpnPwCfg.match_pw_label = hsl_vpn_port_info->in_pw_label;
        stHslVpnPwCfg.out_disable = hsl_vpn_port_info->out_disable;
        stHslVpnPwCfg.out_port = hsl_vpn_port_info->pannel_port;
        stHslVpnPwCfg.out_pw_exp = hsl_vpn_port_info->out_pw_exp;
        stHslVpnPwCfg.out_pw_label = hsl_vpn_port_info->out_pw_label;
        stHslVpnPwCfg.vpn_port_attr = hsl_vpn_port_info->vpn_port_attr;

		hsl_bcm_pw_port_info_1307 stHslVpnPwCfg_1307;

		stHslVpnPwCfg_1307.hsl_vpn_id = stHslVpnPwCfg.hsl_vpn_id;
		stHslVpnPwCfg_1307.match_pw_label = stHslVpnPwCfg.match_pw_label;
		//stHslVpnPwCfg_1307.match_pw_label = ;
		stHslVpnPwCfg_1307.out_port = stHslVpnPwCfg.out_port;
		//stHslVpnPwCfg_1307.out_lsp_label = ;
		//stHslVpnPwCfg_1307.out_lsp_exp = ;
		stHslVpnPwCfg_1307.out_pw_label = stHslVpnPwCfg.out_pw_exp;
		stHslVpnPwCfg_1307.out_pw_exp =  stHslVpnPwCfg.out_pw_exp;
		//stHslVpnPwCfg_1307.des_mac = ;
		//stHslVpnPwCfg_1307.mpls_out_vlan = ;
   		//stHslVpnPwCfg_1307.mpls_in_vlan = ;
   		stHslVpnPwCfg_1307.vpn_port_attr = stHslVpnPwCfg.vpn_port_attr;
		stHslVpnPwCfg_1307.out_disable = stHslVpnPwCfg.out_disable;
		//stHslVpnPwCfg_1307.in_disable = ;
		stHslVpnPwCfg_1307.mpls_port_id = 0;

		
		
        ret = hsl_bcm_vpn_pw_port_del(&stHslVpnPwCfg_1307);
    }

    HSL_LOG (HSL_LOG_MSG, HSL_LEVEL_INFO,
                            "Message: HAL message hsl_msg_vpn_port_del  rc = %x\n", ret);

    HSL_MSG_PROCESS_RETURN (sock, hdr, ret);
    HSL_FN_EXIT(ret);

}

/* add from hsl-0213 */

int hsl_msg_lsp_add(struct socket *sock, struct hal_nlmsghdr *hdr, char *msgbuf)
{
	
      int ret = 0;
      hsl_bcm_mpls_tunnel_switch_cfg *hsl_lsp_info = NULL;
	  hsl_bcm_mpls_tunnel_switch_cfg_1307 hsl_lsp_info_1307;
      HSL_FN_ENTER();

    hsl_lsp_info = (hsl_bcm_mpls_tunnel_switch_cfg *)msgbuf;

    HSL_LOG (HSL_LOG_MSG, HSL_LEVEL_INFO,
             "hsl_msg_lsp_add: lsp_id = %d\n",  hsl_lsp_info->lsp_id);
			 	
    HSL_LOG (HSL_LOG_MSG, HSL_LEVEL_INFO,
           "hsl_msg_lsp_add: lsp_type = %d\n", hsl_lsp_info->lsp_type);

    HSL_LOG (HSL_LOG_MSG, HSL_LEVEL_INFO,
             "hsl_msg_lsp_add: in_port = %d\n",  hsl_lsp_info->in_port);
		
    HSL_LOG (HSL_LOG_MSG, HSL_LEVEL_INFO,
             "hsl_msg_lsp_add: in_label = %d\n",  hsl_lsp_info->in_label);
	
    HSL_LOG (HSL_LOG_MSG, HSL_LEVEL_INFO,
             "hsl_msg_lsp_add: in_vlan = %d\n",  hsl_lsp_info->in_vlan);
		

    HSL_LOG (HSL_LOG_MSG, HSL_LEVEL_INFO,
             "hsl_msg_lsp_add: out_port = %d\n",  hsl_lsp_info->out_port);
		
		
    HSL_LOG (HSL_LOG_MSG, HSL_LEVEL_INFO,
            "hsl_msg_lsp_add: out_label = %d\n",  hsl_lsp_info->out_label);
		
		
    HSL_LOG (HSL_LOG_MSG, HSL_LEVEL_INFO,
            "hsl_msg_lsp_add: out_label_exp = %d\n",  hsl_lsp_info->out_label_exp);
		
    
    HSL_LOG (HSL_LOG_MSG, HSL_LEVEL_INFO,
            "hsl_msg_lsp_add: out_vlan = %d\n",  hsl_lsp_info->out_vlan);
		
		hsl_lsp_info_1307.lsp_id = hsl_lsp_info->lsp_id;//@zlh 0922
		//hsl_lsp_info_1307.lsp_type= HSL_MPLS_LABEL_SWITCH;//@zlh0923 
		hsl_lsp_info_1307.lsp_type = hsl_lsp_info->lsp_type;//@zlh0924 
		hsl_lsp_info_1307.in_port = hsl_lsp_info->in_port;
		hsl_lsp_info_1307.in_label = hsl_lsp_info->in_label;
		hsl_lsp_info_1307.mpls_in_vlan = hsl_lsp_info->in_vlan;
		hsl_lsp_info_1307.out_port = hsl_lsp_info->out_port;
		hsl_lsp_info_1307.out_label = hsl_lsp_info->out_label;
		hsl_lsp_info_1307.out_lsp_exp = hsl_lsp_info->out_label_exp;
		hsl_lsp_info_1307.mpls_out_vlan = hsl_lsp_info->out_vlan;
		

     HSL_LOG (HSL_LOG_MSG, HSL_LEVEL_INFO,
                "hsl_msg_lsp_add: des_mac = %x.%x.%x.%x.%x.%x\n",  
                hsl_lsp_info->des_mac[0],
                hsl_lsp_info->des_mac[1],
                hsl_lsp_info->des_mac[2],
                hsl_lsp_info->des_mac[3],
                hsl_lsp_info->des_mac[4],
                hsl_lsp_info->des_mac[5]);
	 
     			
		memcpy(hsl_lsp_info_1307.des_mac, hsl_lsp_info->des_mac, 6);
				
     ret = hsl_bcm_mpls_tunnel_add(& hsl_lsp_info_1307);
     HSL_LOG (HSL_LOG_MSG, HSL_LEVEL_INFO,
                         "Message: HAL message hsl_msg_lsp_add  rc = %x\n", ret);
  
      HSL_MSG_PROCESS_RETURN (sock, hdr, ret);
      HSL_FN_EXIT(ret);
}

/* add from hsl-0213 */

int hsl_msg_lsp_del(struct socket *sock, struct hal_nlmsghdr *hdr, char *msgbuf)
{
      int ret = 0;
      hsl_bcm_mpls_tunnel_switch_cfg *hsl_lsp_info = NULL;
	  hsl_bcm_mpls_tunnel_switch_cfg_1307 hsl_lsp_info_1307;
      HSL_FN_ENTER();
      
     hsl_lsp_info = (hsl_bcm_mpls_tunnel_switch_cfg *)msgbuf;

    HSL_LOG (HSL_LOG_MSG, HSL_LEVEL_INFO,
             "hsl_msg_lsp_del: lsp_id = %d\n",  hsl_lsp_info->lsp_id);
		hsl_lsp_info_1307.lsp_id = hsl_lsp_info->lsp_id;
		//hsl_lsp_info_1307.in_port = hsl_lsp_info->in_port;
		//hsl_lsp_info_1307.in_label = hsl_lsp_info->in_label;
     ret = hsl_bcm_mpls_tunnel_del( & hsl_lsp_info_1307 );
   
      HSL_LOG (HSL_LOG_MSG, HSL_LEVEL_INFO,
                    "Message: HAL message hsl_msg_lsp_del rc = 0x%x\n", ret);
      
      HSL_MSG_PROCESS_RETURN (sock, hdr, ret);
      HSL_FN_EXIT(ret);

}


int hsl_msg_get_lsp_status(struct socket *sock, struct hal_nlmsghdr *hdr, char *msgbuf)
{
    int ret = 0;
    HSL_FN_ENTER();
    ial_mpls_lsp_status lsp_status;
    ial_mpls_lsp_status  *p_lsp_status = 0;;

    HSL_LOG (HSL_LOG_MSG, HSL_LEVEL_INFO,
                    "Message: HAL message hsl_msg_get_lsp_status \n");
    memset(&lsp_status, 0, sizeof(ial_mpls_lsp_status));

    p_lsp_status = (ial_mpls_lsp_status *)msgbuf;
    lsp_status.lsp_id = p_lsp_status->lsp_id;
    ret = hsl_mpls_get_lsp_status(&lsp_status);
    if(ret == 0)
    {
          hsl_sock_post_msg (sock, HAL_MSG_LSP_GET_LINK_STATUS, 0, 
                      hdr->nlmsg_seq, (char *)&lsp_status, sizeof (ial_mpls_lsp_status));
    }
    else
    {
        HSL_MSG_PROCESS_RETURN (sock, hdr, ret);
    }

     HSL_FN_EXIT(ret);
}


int hsl_msg_lsp_protect_group_add(struct socket *sock, struct hal_nlmsghdr *hdr, char *msgbuf)
{
    int ret = 0;
    HSL_FN_ENTER();

    ret = hsl_mpls_protect_group_add((ial_mpls_protect_group_cfg *)msgbuf);
    
     HSL_LOG (HSL_LOG_MSG, HSL_LEVEL_INFO,
                    "Message: HAL message hsl_msg_lsp_protect_group_add rc = %x\n", ret);

    HSL_MSG_PROCESS_RETURN (sock, hdr, ret);
    HSL_FN_EXIT(ret);
}

int hsl_msg_lsp_protect_group_del(struct socket *sock, struct hal_nlmsghdr *hdr, char *msgbuf)
{
     int ret = 0;
    HSL_FN_ENTER();
    
    ret = hsl_mpls_protect_group_del((ial_mpls_protect_group_cfg *)msgbuf);
    
    HSL_LOG (HSL_LOG_MSG, HSL_LEVEL_INFO,
                    "Message: HAL message hsl_msg_lsp_protect_group_del rc = %x\n", ret);

    HSL_MSG_PROCESS_RETURN (sock, hdr, ret);
    HSL_FN_EXIT(ret);
}

int hsl_msg_lsp_protect_group_mod_param(struct socket *sock, struct hal_nlmsghdr *hdr, char *msgbuf)
{
   int ret = 0;
    HSL_FN_ENTER();

    HSL_LOG (HSL_LOG_MSG, HSL_LEVEL_INFO,
                    "Message: HAL message hsl_msg_lsp_protect_group_mod_param \n");
    
    ret = hsl_mpls_protect_group_mod_param((ial_mpls_protect_group_cfg *)msgbuf);

    HSL_MSG_PROCESS_RETURN (sock, hdr, ret);
    HSL_FN_EXIT(ret);
}

int hsl_msg_lsp_protect_group_switch(struct socket *sock, struct hal_nlmsghdr *hdr, char *msgbuf)
{
    int ret = 0;
    HSL_FN_ENTER();

    HSL_LOG (HSL_LOG_MSG, HSL_LEVEL_INFO,
                    "Message: HAL message hsl_msg_lsp_protect_group_switch \n");
    
    ret = hsl_mpls_protect_group_switch((ial_mpls_protect_switch_cfg *)msgbuf);

    HSL_MSG_PROCESS_RETURN (sock, hdr, ret);
    HSL_FN_EXIT(ret);
}

int hsl_msg_lsp_get_protect_grp_info(struct socket *sock, struct hal_nlmsghdr *hdr, char *msgbuf)
{
    int ret = 0;
    ial_mpls_protect_group_info lsp_grp_info;
    ial_mpls_protect_group_info *p_lsp_grp_infp = NULL;
    HSL_FN_ENTER();

    HSL_LOG (HSL_LOG_MSG, HSL_LEVEL_INFO,
               "Message: HAL message hsl_msg_lsp_get_protect_grp_info \n");

    memset(&lsp_grp_info, 0, sizeof(ial_mpls_protect_group_info));

    p_lsp_grp_infp = (ial_mpls_protect_group_info *)msgbuf;
    lsp_grp_info.grp_id = p_lsp_grp_infp->grp_id;
    ret = hsl_mpls_get_protect_grp_info(&lsp_grp_info);
    if(ret == 0)
    {
          ret = hsl_sock_post_msg (sock, HAL_MSG_LSP_GET_PROTECT_GRP_INFO, 0, 
                      hdr->nlmsg_seq, (char *)&lsp_grp_info, sizeof(ial_mpls_protect_group_info));
    }
    else
    {
        HSL_MSG_PROCESS_RETURN (sock, hdr, ret);
    }

    HSL_FN_EXIT(ret);

}


int hsl_msg_vpn_multicast_group_create(struct socket *sock, struct hal_nlmsghdr *hdr, char *msgbuf)
{
       int ret = 0;
      hsl_bcm_msg_mcast_grp *hsl_vpn_mc_grp_info = NULL;
      hsl_bcm_vpn_mcast_grp_cfg stMcastGrpCfg;

      HSL_FN_ENTER();
 
      HSL_LOG (HSL_LOG_MSG, HSL_LEVEL_INFO,
			"Message: HAL message hsl_msg_vpn_multicast_group_create \n");

      hsl_vpn_mc_grp_info = (hsl_bcm_msg_mcast_grp *)msgbuf;
      stMcastGrpCfg.hsl_vpn_id = hsl_vpn_mc_grp_info->hsl_vpn_id;
      stMcastGrpCfg.hsl_mc_group_id = hsl_vpn_mc_grp_info->hsl_mc_group_id;
     stMcastGrpCfg.bcm_mcast_id = 0;
     memcpy(stMcastGrpCfg.mc_mac,  hsl_vpn_mc_grp_info->mc_mac, 6);

	 HSL_LOG (HSL_LOG_MSG, HSL_LEVEL_INFO,
			"Message: HAL message hsl_msg_vpn_multicast_group_create: hsl_vpn_id = %d hsl_mc_group_id = %d\n",
												hsl_vpn_mc_grp_info->hsl_vpn_id,hsl_vpn_mc_grp_info->hsl_mc_group_id);
	 
     ret = hsl_bcm_mulitcast_group_create(&stMcastGrpCfg);
  
      HSL_MSG_PROCESS_RETURN (sock, hdr, ret);
      HSL_FN_EXIT(ret);
}

int hsl_msg_vpn_multicast_group_del(struct socket *sock, struct hal_nlmsghdr *hdr, char *msgbuf)
{
      int ret = 0;
      hsl_bcm_msg_mcast_grp *hsl_vpn_mc_grp_info = NULL;
      hsl_bcm_vpn_mcast_grp_cfg stMcastGrpCfg;

      HSL_FN_ENTER();
 
      HSL_LOG (HSL_LOG_MSG, HSL_LEVEL_INFO,
			"Message: HAL message hsl_msg_vpn_multicast_group_del \n");

      hsl_vpn_mc_grp_info = (hsl_bcm_msg_mcast_grp *)msgbuf;
      stMcastGrpCfg.hsl_vpn_id = hsl_vpn_mc_grp_info->hsl_vpn_id;
      stMcastGrpCfg.hsl_mc_group_id = hsl_vpn_mc_grp_info->hsl_mc_group_id;
     stMcastGrpCfg.bcm_mcast_id = 0;
     memcpy(stMcastGrpCfg.mc_mac,  hsl_vpn_mc_grp_info->mc_mac, 6);
     ret = hsl_bcm_mulitcast_group_del(&stMcastGrpCfg);
  
      HSL_MSG_PROCESS_RETURN (sock, hdr, ret);
      HSL_FN_EXIT(ret);

}

int hsl_msg_vpn_multicast_group_port_add(struct socket *sock, struct hal_nlmsghdr *hdr, char *msgbuf)
{
      int ret = 0;
      hsl_bcm_msg_mcast_grp_port *hsl_vpn_mc_grp_port_info = NULL;
      hsl_bcm_vpn_mcast_port_cfg  stMcastGrpPortCfg;

      HSL_FN_ENTER();
 
      HSL_LOG (HSL_LOG_MSG, HSL_LEVEL_INFO,
			"Message: HAL message hsl_msg_vpn_multicast_group_port_add \n");

      hsl_vpn_mc_grp_port_info = (hsl_bcm_msg_mcast_grp_port *)msgbuf;
	  
     stMcastGrpPortCfg.hsl_vpn_id = hsl_vpn_mc_grp_port_info->hsl_vpn_id;
     stMcastGrpPortCfg.hsl_mc_group_id = hsl_vpn_mc_grp_port_info->hsl_mc_group_id;
     stMcastGrpPortCfg.svp_type = hsl_vpn_mc_grp_port_info->svp_port_type;
     stMcastGrpPortCfg.ac_port = hsl_vpn_mc_grp_port_info->pannel_port;
     stMcastGrpPortCfg.ac_vlan = hsl_vpn_mc_grp_port_info->ac_vlan;
     stMcastGrpPortCfg.pw_in_port = hsl_vpn_mc_grp_port_info->pannel_port;
     stMcastGrpPortCfg.match_pw_label = hsl_vpn_mc_grp_port_info->pw_label;
	 
	 HSL_LOG (HSL_LOG_MSG, HSL_LEVEL_INFO,
			"Message: HAL message hsl_msg_vpn_multicast_group_port_add:hsl_vpn_id =%d  hsl_mc_group_id = %d\n",
											hsl_vpn_mc_grp_port_info->hsl_vpn_id,hsl_vpn_mc_grp_port_info->hsl_mc_group_id);
	  
      ret = hsl_bcm_mulitcast_group_port_set(&stMcastGrpPortCfg, 1);
	 
      HSL_MSG_PROCESS_RETURN (sock, hdr, ret);
      HSL_FN_EXIT(ret);
}


int  hsl_msg_vpn_multicast_group_port_del(struct socket *sock, struct hal_nlmsghdr *hdr, char *msgbuf)
{
      int ret = 0;
      hsl_bcm_msg_mcast_grp_port *hsl_vpn_mc_grp_port_info = NULL;
      hsl_bcm_vpn_mcast_port_cfg  stMcastGrpPortCfg;

      HSL_FN_ENTER();
 
      HSL_LOG (HSL_LOG_MSG, HSL_LEVEL_INFO,
			"Message: HAL message hsl_msg_vpn_multicast_group_port_del \n");

     hsl_vpn_mc_grp_port_info = (hsl_bcm_msg_mcast_grp_port *)msgbuf;
	  
     stMcastGrpPortCfg.hsl_vpn_id = hsl_vpn_mc_grp_port_info->hsl_vpn_id;
     stMcastGrpPortCfg.hsl_mc_group_id = hsl_vpn_mc_grp_port_info->hsl_mc_group_id;
     stMcastGrpPortCfg.svp_type = hsl_vpn_mc_grp_port_info->svp_port_type;
     stMcastGrpPortCfg.ac_port = hsl_vpn_mc_grp_port_info->pannel_port;
     stMcastGrpPortCfg.ac_vlan = hsl_vpn_mc_grp_port_info->ac_vlan;
     stMcastGrpPortCfg.pw_in_port = hsl_vpn_mc_grp_port_info->pannel_port;
     stMcastGrpPortCfg.match_pw_label = hsl_vpn_mc_grp_port_info->pw_label;
	 
      ret = hsl_bcm_mulitcast_group_port_set(&stMcastGrpPortCfg, 0);
  
      HSL_MSG_PROCESS_RETURN (sock, hdr, ret);
      HSL_FN_EXIT(ret);
}

int  hsl_msg_vpn_static_mac_add(struct socket *sock, struct hal_nlmsghdr *hdr, char *msgbuf)
{
      int ret = 0;
      hsl_bcm_msg_static_mac_cfg *hsl_vpn_static_mac_info = NULL;
      hsl_bcm_static_mac_cfg   stStaticMacCfg;

      HSL_FN_ENTER();
 
      HSL_LOG (HSL_LOG_MSG, HSL_LEVEL_INFO,
			"Message: HAL message hsl_msg_vpn_static_mac_add \n");

      hsl_vpn_static_mac_info = (hsl_bcm_msg_static_mac_cfg *)msgbuf;

      memset(&stStaticMacCfg, 0, sizeof(hsl_bcm_static_mac_cfg));
      stStaticMacCfg.ac_vlan      = hsl_vpn_static_mac_info->ac_vlan;
      stStaticMacCfg.dst_drop    = hsl_vpn_static_mac_info->dst_drop;
      stStaticMacCfg.hsl_vpn_id = hsl_vpn_static_mac_info->hsl_vpn_id;
      stStaticMacCfg.port            = hsl_vpn_static_mac_info->port;
      stStaticMacCfg.pw_label     = hsl_vpn_static_mac_info->pw_label;
      stStaticMacCfg.src_drop      = hsl_vpn_static_mac_info->src_drop;
      stStaticMacCfg.svp_type     = hsl_vpn_static_mac_info->svp_type;
      memcpy(stStaticMacCfg.mac_addr, hsl_vpn_static_mac_info->mac_addr, 6);

	   HSL_LOG (HSL_LOG_MSG, HSL_LEVEL_INFO,
			"Message: HAL message hsl_msg_vpn_static_mac_add: stStaticMacCfg.ac_vlan = %d stStaticMacCfg.hsl_vpn_id = %d stStaticMacCfg.dst_drop = %d stStaticMacCfg.port = %d\n",
			 stStaticMacCfg.ac_vlan,stStaticMacCfg.hsl_vpn_id,stStaticMacCfg.dst_drop,stStaticMacCfg.port);
 
      ret = hsl_bcm_vpn_static_macaddr_set(&stStaticMacCfg, 1);
	  
      HSL_MSG_PROCESS_RETURN (sock, hdr, ret);
      HSL_FN_EXIT(ret);
 
}

int hsl_msg_vpn_static_mac_del(struct socket *sock, struct hal_nlmsghdr *hdr, char *msgbuf)
{
      int ret = 0;
      hsl_bcm_msg_static_mac_cfg *hsl_vpn_static_mac_info = NULL;
      hsl_bcm_static_mac_cfg   stStaticMacCfg;

      HSL_FN_ENTER();
 
      HSL_LOG (HSL_LOG_MSG, HSL_LEVEL_INFO,
			"Message: HAL message hsl_msg_vpn_static_mac_del \n");

      hsl_vpn_static_mac_info = (hsl_bcm_msg_static_mac_cfg *)msgbuf;
	  
     memset(&stStaticMacCfg, 0, sizeof(hsl_bcm_static_mac_cfg));
     stStaticMacCfg.ac_vlan      = hsl_vpn_static_mac_info->ac_vlan;
     stStaticMacCfg.dst_drop    = hsl_vpn_static_mac_info->dst_drop;
     stStaticMacCfg.hsl_vpn_id = hsl_vpn_static_mac_info->hsl_vpn_id;
     stStaticMacCfg.port            = hsl_vpn_static_mac_info->port;
     stStaticMacCfg.pw_label     = hsl_vpn_static_mac_info->pw_label;
     stStaticMacCfg.src_drop      = hsl_vpn_static_mac_info->src_drop;
     stStaticMacCfg.svp_type     = hsl_vpn_static_mac_info->svp_type;
     memcpy(stStaticMacCfg.mac_addr, hsl_vpn_static_mac_info->mac_addr, 6);
	 
     ret = hsl_bcm_vpn_static_macaddr_set(&stStaticMacCfg, 0);
	  
     HSL_MSG_PROCESS_RETURN (sock, hdr, ret);
     HSL_FN_EXIT(ret);
}

/* add from hsl-0213 @zlh*/
/* not sch function */
int hsl_msg_oam_add(struct socket *sock, struct hal_nlmsghdr *hdr, char *msgbuf)
{
	/* @zlh0915 */
	HSL_LOG (HSL_LOG_MSG, HSL_LEVEL_INFO,
                  "Message: HAL message hsl_msg_oam_add function execution successfully @Z L H0915" );
    int ret = 0;
    ial_oam_cfg *ptr_oam_cfg = NULL;
        
    HSL_FN_ENTER();
    
    ptr_oam_cfg = (ial_oam_cfg *)msgbuf;
    
    HSL_LOG (HSL_LOG_MSG, HSL_LEVEL_INFO,
          "Message: HAL message hsl_msg_oam_add oam id = 0x%x\n", ptr_oam_cfg->oam_id);

    HSL_LOG (HSL_LOG_MSG, HSL_LEVEL_INFO,
             "Message: HAL message hsl_msg_oam_add in port = 0x%x\n", ptr_oam_cfg->in_port);

     HSL_LOG (HSL_LOG_MSG, HSL_LEVEL_INFO,
          "Message: HAL message hsl_msg_oam_add out port = 0x%x\n", ptr_oam_cfg->out_port);

    HSL_LOG (HSL_LOG_MSG, HSL_LEVEL_INFO,
          "Message: HAL message hsl_msg_oam_add  lsp_id  = 0x%x\n", ptr_oam_cfg->lsp_id);

     HSL_LOG (HSL_LOG_MSG, HSL_LEVEL_INFO,
          "Message: HAL message hsl_msg_oam_add cc  = 0x%x\n", ptr_oam_cfg->ccm_period);
     
    ret = hsl_oam_add(ptr_oam_cfg);

    HSL_LOG (HSL_LOG_MSG, HSL_LEVEL_INFO,
                  "Message: HAL message hsl_msg_oam_add rc = 0x%x\n", ret);

    
    HSL_MSG_PROCESS_RETURN (sock, hdr, ret);
    HSL_FN_EXIT(ret);
	
}

int hsl_msg_oam_del(struct socket *sock, struct hal_nlmsghdr *hdr, char *msgbuf)
{
    int ret = 0;
   ial_oam_cfg *ptr_oam_cfg = NULL;
          
    HSL_FN_ENTER();

    ptr_oam_cfg = (ial_oam_cfg *)msgbuf;

    HSL_LOG (HSL_LOG_MSG, HSL_LEVEL_INFO,
            "Message: HAL message hsl_msg_oam_del oam id = 0x%x\n", ptr_oam_cfg->oam_id);
     
    ret = hsl_oam_del(ptr_oam_cfg);
    
    HSL_LOG (HSL_LOG_MSG, HSL_LEVEL_INFO,"Message: HAL message hsl_msg_oam_del rc = %x\n",ret);
    
    HSL_MSG_PROCESS_RETURN (sock, hdr, ret);
    HSL_FN_EXIT(ret);
}


int hsl_msg_oam_get_status(struct socket *sock, struct hal_nlmsghdr *hdr, char *msgbuf)
{
    int ret = 0;
    ial_oam_status  oam_status_info;
    ial_oam_status *p_oam_status_info = NULL;

    HSL_FN_ENTER();
    HSL_LOG (HSL_LOG_MSG, HSL_LEVEL_INFO,"Message: HAL message hsl_msg_oam_get_status \n");

    memset(&oam_status_info, 0, sizeof(ial_oam_status));
    p_oam_status_info = (ial_oam_status *)msgbuf;
    oam_status_info.oam_id = p_oam_status_info->oam_id;

    ret = hsl_oam_get_status(&oam_status_info);
    if(ret == 0)
    {
          ret = hsl_sock_post_msg (sock, HAL_MSG_OAM_GET_STATUS, 0, 
                      hdr->nlmsg_seq, (char *)&oam_status_info, sizeof (ial_oam_status));
    }
    else
    {
        HSL_MSG_PROCESS_RETURN (sock, hdr, ret);
    }

    HSL_FN_EXIT(ret);
}


/*
int
hsl_msg_rate_limite_set (struct socket *sock, struct hal_nlmsghdr *hdr, char *msgbuf)
{
   int ret;
   struct hal_msg_ac_car_s *ifp;
 
   HSL_FN_ENTER();
 
   HSL_LOG (HSL_LOG_MSG, HSL_LEVEL_INFO,
			"Message: HAL message hsl_msg_rate_limite_set \n");
 
   ifp = ( struct hal_msg_ac_car_s *)msgbuf;
   ret = hsl_port_egr_shapping_set ( ifp );
 
   HSL_MSG_PROCESS_RETURN (sock, hdr, ret);
 
   HSL_FN_EXIT(ret);
}

int
hsl_msg_traffic_shapping_set (struct socket *sock, struct hal_nlmsghdr *hdr, char *msgbuf)
{
   int ret;
   struct hal_msg_ac_car_s *ifp;
 
   HSL_FN_ENTER();
 
   HSL_LOG (HSL_LOG_MSG, HSL_LEVEL_INFO,
			"Message: HAL message hsl_msg_traffic_shapping_set \n");
 
   ifp = ( struct hal_msg_ac_car_s *)msgbuf;
   ret = hsl_port_ing_rate_limite_set( ifp );
 
   HSL_MSG_PROCESS_RETURN (sock, hdr, ret);
 
   HSL_FN_EXIT(ret);
}
*/

int
hsl_msg_port_rate_limit (struct socket *sock, struct hal_nlmsghdr *hdr, char *msgbuf)
{
   int ret;
   hal_car_info_t *ifp;
 
   HSL_FN_ENTER();
 
   HSL_LOG (HSL_LOG_MSG, HSL_LEVEL_INFO,
			"Message: HAL message hsl_msg_ac_car_set \n");
   ifp = ( struct hal_car_info_t *)msgbuf;

   ret = hsl_port_ing_rate_limite_set(*ifp );


  HSL_LOG (HSL_LOG_MSG, HSL_LEVEL_INFO,"-----ret %d--\n",ret);
 
   HSL_MSG_PROCESS_RETURN (sock, hdr, ret);
 
   HSL_FN_EXIT(ret);
}


int
hsl_msg_port_shapping (struct socket *sock, struct hal_nlmsghdr *hdr, char *msgbuf)
{
   int ret;
   hal_car_info_t *ifp;
 
   HSL_FN_ENTER();
 
   HSL_LOG (HSL_LOG_MSG, HSL_LEVEL_INFO,
			"Message: HAL message hsl_msg_ac_car_set \n");
   ifp = ( struct hal_car_info_t *)msgbuf;
	
   ret = hsl_port_egr_shapping_set(*ifp );

 	HSL_LOG (HSL_LOG_MSG, HSL_LEVEL_INFO,"-----ret %d--\n",ret);
 
   HSL_MSG_PROCESS_RETURN (sock, hdr, ret);
 
   HSL_FN_EXIT(ret);
}

/* zlh@150116, add priority msg */
int
hsl_msg_ac_priority_set (struct socket *sock, struct hal_nlmsghdr *hdr, char *msgbuf)
{
   int ret;
   hal_priority_info_t *ifp;
   hal_car_info_t ifp1;//zlh@150123
   extern hsl_car_info_t g_ac_car_info[2048];//zlh@150125
   HSL_FN_ENTER();
 
   HSL_LOG (HSL_LOG_MSG, HSL_LEVEL_INFO,
			"Message: HAL message hsl_msg_ac_priority_set \n");
   ifp = ( struct hal_priority_info_t *)msgbuf;

   /* zlh@150125,下发上一次car's info*/
   
   ifp1.sev_index= g_ac_car_info[ifp->sev_index].index;
   ifp1.portId= g_ac_car_info[ifp->sev_index].port;
   ifp1.enable = g_ac_car_info[ifp->sev_index].enable;
   ifp1.cir = g_ac_car_info[ifp->sev_index].cir;
   ifp1.pir = g_ac_car_info[ifp->sev_index].pir;
   ifp1.cbs = g_ac_car_info[ifp->sev_index].cbs;
   ifp1.pbs = g_ac_car_info[ifp->sev_index].pbs;
   ifp1.o_tag_label = g_ac_car_info[ifp->sev_index].o_tag_label;
   
   HSL_LOG (HSL_LOG_MSG, HSL_LEVEL_INFO,"Message: HAL message hsl_msg_ac_priority_set:sev_index            = %d\n",ifp1.sev_index);//zlh@150125 teset
   HSL_LOG (HSL_LOG_MSG, HSL_LEVEL_INFO,"Message: HAL message hsl_msg_ac_priority_set:port                 = %d\n",ifp1.portId);//zlh@150125 teset
   HSL_LOG (HSL_LOG_MSG, HSL_LEVEL_INFO,"Message: HAL message hsl_msg_ac_priority_set:eable                = %d\n",ifp1.enable);//zlh@150125 teset
   HSL_LOG (HSL_LOG_MSG, HSL_LEVEL_INFO,"Message: HAL message hsl_msg_ac_priority_set:cir                  = %d\n",ifp1.cir);//zlh@150125 teset
   HSL_LOG (HSL_LOG_MSG, HSL_LEVEL_INFO,"Message: HAL message hsl_msg_ac_priority_set:pir                  = %d\n",ifp1.pir);//zlh@150125 teset
   HSL_LOG (HSL_LOG_MSG, HSL_LEVEL_INFO,"Message: HAL message hsl_msg_ac_priority_set:cbs                  = %d\n",ifp1.cbs);//zlh@150125 teset
   HSL_LOG (HSL_LOG_MSG, HSL_LEVEL_INFO,"Message: HAL message hsl_msg_ac_priority_set:pbs                  = %d\n",ifp1.pbs);//zlh@150125 teset
   HSL_LOG (HSL_LOG_MSG, HSL_LEVEL_INFO,"Message: HAL message hsl_msg_ac_priority_set:o_tag_label          = %d\n",ifp1.o_tag_label);//zlh@150125 teset

   //ret = hsl_ac_ing_priority_set ( 0, *ifp );
   ret = hsl_ac_ing_car_set( ifp1 ,*ifp);//zlh@150123,car and priority both return hsl_ac_ing_car_set function 


 HSL_LOG (HSL_LOG_MSG, HSL_LEVEL_INFO,"-----ret %d--\n",ret);
 
   HSL_MSG_PROCESS_RETURN (sock, hdr, ret);
 
   HSL_FN_EXIT(ret);
}


int
hsl_msg_ac_car_set (struct socket *sock, struct hal_nlmsghdr *hdr, char *msgbuf)
{
   int ret;
   hal_car_info_t *ifp;
   hal_priority_info_t ifp1;//zlh@150123
   
   extern hsl_priority_info_t g_ac_priority_info[2048];//zlh@150123
   
   HSL_FN_ENTER();
   HSL_LOG (HSL_LOG_MSG, HSL_LEVEL_INFO,
			"Message: HAL message hsl_msg_ac_car_set \n");
   ifp = ( struct hal_car_info_t *)msgbuf;
   ifp->drop_yellow = 1;
   ifp->fwd_red = 0;
   
    /* zlh@150125,下发上一次priority's info*/
	
   ifp1.sev_index= g_ac_priority_info[ifp->sev_index].index;
   ifp1.portId= g_ac_priority_info[ifp->sev_index].port;
   ifp1.enable = g_ac_priority_info[ifp->sev_index].enable;
   ifp1.priority = g_ac_priority_info[ifp->sev_index].priority;
   ifp1.ovid = g_ac_priority_info[ifp->sev_index].ovid;

   HSL_LOG (HSL_LOG_MSG, HSL_LEVEL_INFO,"Message: HAL message hsl_msg_ac_car_set:sev_index               = %d\n",ifp1.sev_index);//zlh@150125 teset
   HSL_LOG (HSL_LOG_MSG, HSL_LEVEL_INFO,"Message: HAL message hsl_msg_ac_car_set:port                    = %d\n",ifp1.portId);//zlh@150125 teset
   HSL_LOG (HSL_LOG_MSG, HSL_LEVEL_INFO,"Message: HAL message hsl_msg_ac_car_set:eable                   = %d\n",ifp1.enable);//zlh@150125 teset
   HSL_LOG (HSL_LOG_MSG, HSL_LEVEL_INFO,"Message: HAL message hsl_msg_ac_car_set:priority                = %d\n",ifp1.priority);//zlh@150125 teset
   HSL_LOG (HSL_LOG_MSG, HSL_LEVEL_INFO,"Message: HAL message hsl_msg_ac_car_set:ovid                    = %d\n",ifp1.ovid);//zlh@150125 teset
   
   ret = hsl_ac_ing_car_set ( *ifp ,ifp1 );//zlh@150123


 HSL_LOG (HSL_LOG_MSG, HSL_LEVEL_INFO,"-----ret %d--\n",ret);
 
   HSL_MSG_PROCESS_RETURN (sock, hdr, ret);
 
   HSL_FN_EXIT(ret);
}

int
hsl_msg_mpls_car_set (struct socket *sock, struct hal_nlmsghdr *hdr, char *msgbuf)
{
   int ret;
   hal_car_info_t *ifp;
 
   HSL_FN_ENTER();
 
   HSL_LOG (HSL_LOG_MSG, HSL_LEVEL_INFO,
			"Message: HAL message hsl_msg_mpls_car_set \n");
   ifp = ( struct hal_car_info_t *)msgbuf;

   ifp->drop_yellow = 1;
   ifp->fwd_red = 0;
   ret = hsl_mpls_ing_car_set ( *ifp );
 
   HSL_MSG_PROCESS_RETURN (sock, hdr, ret);
 
   HSL_FN_EXIT(ret);
}

int hsl_msg_port_pvid_set(struct socket *sock, struct hal_nlmsghdr *hdr, char *msgbuf)
{
       int ret = 0;
	   struct message
	    {
	        int port;
	        int pvid;
        }; 
	   struct message *msgdata = NULL;
      msgdata = (struct message *)msgbuf;
      
      HSL_FN_ENTER();
	  
      HSL_LOG (HSL_LOG_MSG, HSL_LEVEL_INFO,
			   "Message: HAL message hsl_msg_port_pvid_set:port %d, pvid %d.\n", msgdata->port, msgdata->pvid);
	  
       //ret = hsl_bcm_pvid_action_new_set(msgdata->port, 1, msgdata->pvid);
	   ret = hsl_bcm_pvid_action_set(msgdata->port, 1, msgdata->pvid);
      

    HSL_MSG_PROCESS_RETURN (sock, hdr, ret);
    HSL_FN_EXIT(ret);

}


/*  no function at hsl_bcm_vpls.c */
#if 0
int hsl_msg_vlan_new_create(struct socket *sock, struct hal_nlmsghdr *hdr, char *msgbuf)
{
      int ret = 0;
	   
      int msgdata = *(int *)msgbuf;
      
      HSL_FN_ENTER();
	  
      HSL_LOG (HSL_LOG_MSG, HSL_LEVEL_INFO,
			   "Message: HAL message hsl_msg_vlan_new_create:vlan id %d.\n", msgdata);
	  
       ret = hsl_bcm_vlan_new_create(msgdata);
      

    HSL_MSG_PROCESS_RETURN (sock, hdr, ret);
    HSL_FN_EXIT(ret);

}
int hsl_msg_vlan_new_delete(struct socket *sock, struct hal_nlmsghdr *hdr, char *msgbuf)
{
      int ret = 0;
	   
      int msgdata = *(int *)msgbuf;
      
      HSL_FN_ENTER();
	  
      HSL_LOG (HSL_LOG_MSG, HSL_LEVEL_INFO,
			   "Message: HAL message hsl_msg_vlan_new_delete:vlan id %d.\n", msgdata);
	  
       ret = hsl_bcm_vlan_new_delete(msgdata);
      

    HSL_MSG_PROCESS_RETURN (sock, hdr, ret);
    HSL_FN_EXIT(ret);

}

int hsl_msg_vlan_new_add_vid_to_port(struct socket *sock, struct hal_nlmsghdr *hdr, char *msgbuf)
{
      int ret = 0;

	  struct message
	    {
	        int port;
	        int vid;
			int untag_flag;
        };
      struct message *msgdata;
	  msgdata = (struct message *)msgbuf;
      
      HSL_FN_ENTER();
	  
      HSL_LOG (HSL_LOG_MSG, HSL_LEVEL_INFO,
			   "Message: HAL message hsl_msg_vlan_new_add_vid_to_port:port %d vlan id %d, untag flag %d.\n", 
			           msgdata->port, msgdata->vid, msgdata->untag_flag);
	  
       ret = hsl_bcm_vlan_new_add_vid_to_port(msgdata->port, msgdata->vid, msgdata->untag_flag);
      

    HSL_MSG_PROCESS_RETURN (sock, hdr, ret);
    HSL_FN_EXIT(ret);

}

int hsl_msg_vlan_new_rmv_vid_from_port(struct socket *sock, struct hal_nlmsghdr *hdr, char *msgbuf)
{
      int ret = 0;

	  struct message
	    {
	        int port;
	        int vid;
        };
      struct message *msgdata = (struct message *)msgbuf;
      
      HSL_FN_ENTER();
	  
      HSL_LOG (HSL_LOG_MSG, HSL_LEVEL_INFO,
			   "Message: HAL message hsl_msg_vlan_new_rmv_vid_from_port:port %d vlan id %d.\n", 
			           msgdata->port, msgdata->vid);
	  
       ret = hsl_bcm_vlan_new_rmv_vid_from_port(msgdata->port, msgdata->vid);
      

    HSL_MSG_PROCESS_RETURN (sock, hdr, ret);
    HSL_FN_EXIT(ret);

}

int hsl_msg_vlan_new_add_mcast_to_port(struct socket *sock, struct hal_nlmsghdr *hdr, char *msgbuf)
{
      int ret = 0;

	  struct message
	    {
	        int port;
	        int vid;
			char mac[6];
        };
      struct message *msgdata;
	  msgdata = (struct message *)msgbuf;
      
      HSL_FN_ENTER();
	  
      HSL_LOG (HSL_LOG_MSG, HSL_LEVEL_INFO,
			   "Message: HAL message hsl_msg_vlan_new_add_mcast_to_port:port %d vlan id %d, mac %x.%x.%x.\n", 
			           msgdata->port, msgdata->vid, msgdata->mac[0], msgdata->mac[2], msgdata->mac[4]);
	  
       ret = hsl_bcm_vlan_new_add_mcast_to_port(msgdata->port, msgdata->vid, msgdata->mac);
      

    HSL_MSG_PROCESS_RETURN (sock, hdr, ret);
    HSL_FN_EXIT(ret);

}

int hsl_msg_vlan_new_rmv_mcast_to_port(struct socket *sock, struct hal_nlmsghdr *hdr, char *msgbuf)
{
      int ret = 0;

	  struct message
	    {
	        int port;
	        int vid;
			char mac[6];
        };
      struct message *msgdata;
	  msgdata = (struct message *)msgbuf;
      
      HSL_FN_ENTER();
	  
      HSL_LOG (HSL_LOG_MSG, HSL_LEVEL_INFO,
			   "Message: HAL message hsl_msg_vlan_new_rmv_mcast_to_port:port %d vlan id %d, mac %x.%x.%x.\n", 
			           msgdata->port, msgdata->vid, msgdata->mac[0], msgdata->mac[2], msgdata->mac[4]);
	  
       ret = hsl_bcm_vlan_new_rmv_mcast_to_port(msgdata->port, msgdata->vid, msgdata->mac);
      

    HSL_MSG_PROCESS_RETURN (sock, hdr, ret);
    HSL_FN_EXIT(ret);

}
#endif



