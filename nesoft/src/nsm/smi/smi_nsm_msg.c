/* Copyright (C) 2002-2003  All Rights Reserved. */

/* NSM SMI server implementation for Interface and VLAN  messages.  */

#include "lib_fm_api.h"
#include "smi_server.h"
#include "smi_nsm_server.h"
#include "nsm_qos.h"
#include "nsm_api.h"
#include "nsm_vlan.h"
#include "nsm_bridge.h"
#include "nsm_pro_vlan.h"
#include "gvrp_api.h"

#ifdef HAVE_SMI

/* Set/Get Interface params */
int
nsm_smi_server_recv_if(struct smi_msg_header *header,
                       void *arg, void *message)
{
  struct smi_server_entry *ase;
  struct smi_server *as;
  struct smi_msg_if *msg, replymsg;
  int status = 0, ret = 0;
  struct nsm_master *nm = nsm_master_lookup_by_id (nzg, SMI_DFLT_VRID);
  struct interface *ifp = NULL;
  struct nsm_if *zif = NULL;
  int msgtype = header->type;
  bool_t chan_activate = PAL_FALSE;
  #ifdef HAVE_PVLAN
  struct nsm_bridge_port *br_port = NULL;
  #endif /* HAVE_PVLAN */

  SMI_FN_ENTER ();

  ase = arg;
  as = ase->as;

  msg = (struct smi_msg_if *)message;

  if(IS_NSM_DEBUG_RECV)
    smi_interface_dump(NSM_ZG, msg);

  /* Init cindex */
  replymsg.cindex = 0;

  #ifdef HAVE_HA
  if ((HA_IS_STANDBY (nzg)))
  {
    ret = nsm_check_func_type (msgtype);
    if (ret == SMI_SET_FUNC)
      SMI_FN_EXIT (nsm_smi_server_send_sync_message (ase,
                                                     SMI_MSG_STATUS,
                                                     SMI_ERROR, NULL));
  }
  #endif /* HAVE_HA */

  switch(header->type)
    {
      case SMI_MSG_IF_SETMTU:
        status = nsm_if_mtu_set(SMI_DFLT_VRID, msg->ifname, msg->mtu);
        ret = nsm_smi_server_send_sync_message (ase, SMI_MSG_STATUS, status,
                                                NULL);
        break;

      case SMI_MSG_IF_GETMTU:
      {
         if (nm == NULL)
           {
             SMI_FN_EXIT (nsm_smi_server_send_sync_message (ase,
                                                            SMI_MSG_STATUS,
                                                            SMI_ERROR, NULL));
           }
         ifp = if_lookup_by_name (&nm->vr->ifm, msg->ifname);
         if (ifp == NULL)
           {
             SMI_FN_EXIT (nsm_smi_server_send_sync_message (ase,
                                                            SMI_MSG_STATUS,
                                                            SMI_ERROR, NULL));
           }
          /* Interface MTU TLV */
          SMI_SET_CTYPE (replymsg.cindex, SMI_IF_CTYPE_MTU);
          status = nsm_fea_if_get_mtu(ifp, &(replymsg.mtu));

          if(status < 0)
              msgtype = SMI_MSG_STATUS;

          ret = nsm_smi_server_send_sync_message (ase, msgtype, status,
                                                  &replymsg);
         }
       break;

      case SMI_MSG_IF_SETBW:
         if (nm == NULL)
           {
             SMI_FN_EXIT (nsm_smi_server_send_sync_message (ase,
                                                            SMI_MSG_STATUS,
                                                            SMI_ERROR, NULL));
           }
         ifp = if_lookup_by_name (&nm->vr->ifm, msg->ifname);
         if (ifp == NULL)
           {
             SMI_FN_EXIT (nsm_smi_server_send_sync_message (ase,
                                                            SMI_MSG_STATUS,
                                                            SMI_ERROR, NULL));
           }
         status = nsm_if_set_bandwidth (ifp, msg->bandwidth/BIT_BYTE_CONV);

         ret = nsm_smi_server_send_sync_message (ase, SMI_MSG_STATUS, status,
                                                 NULL);
      break;

      case SMI_MSG_IF_GETBW:
        /* Interface Bandwidth TLV */
        SMI_SET_CTYPE (replymsg.cindex, SMI_IF_CTYPE_BW);
         if (nm == NULL)
           {
             SMI_FN_EXIT (nsm_smi_server_send_sync_message (ase,
                                                            SMI_MSG_STATUS,
                                                            SMI_ERROR, NULL));
           }
         ifp = if_lookup_by_name (&nm->vr->ifm, msg->ifname);
         if (ifp == NULL)
           {
             SMI_FN_EXIT (nsm_smi_server_send_sync_message (ase,
                                                            SMI_MSG_STATUS,
                                                            SMI_ERROR, NULL));
           }

         replymsg.bandwidth = ifp->bandwidth * BIT_BYTE_CONV;

         if(replymsg.bandwidth < 0)
           {
             msgtype = SMI_MSG_STATUS;
             status = SMI_ERROR;
           }

         ret = nsm_smi_server_send_sync_message (ase, msgtype, status,
                                                 &replymsg);
       break;

       case SMI_MSG_IF_SETFLAG:
         if (nm == NULL)
           {
             SMI_FN_EXIT (nsm_smi_server_send_sync_message (ase,
                                                            SMI_MSG_STATUS,
                                                            SMI_ERROR, NULL));
           }
         ifp = if_lookup_by_name (&nm->vr->ifm, msg->ifname);
         if (ifp == NULL)
           {
             SMI_FN_EXIT (nsm_smi_server_send_sync_message (ase,
                                                            SMI_MSG_STATUS,
                                                            SMI_ERROR, NULL));
           }
         status = nsm_if_flag_up_set(SMI_DFLT_VRID, msg->ifname, PAL_TRUE);
         ret = nsm_smi_server_send_sync_message (ase, SMI_MSG_STATUS,
                                                 status, NULL);
       break;

       case SMI_MSG_IF_UNSETFLAG:
         if (nm == NULL)
           {
             SMI_FN_EXIT (nsm_smi_server_send_sync_message (ase,
                                                            SMI_MSG_STATUS,
                                                            SMI_ERROR, NULL));
           }
         ifp = if_lookup_by_name (&nm->vr->ifm, msg->ifname);
         if (ifp == NULL)
           {
             SMI_FN_EXIT (nsm_smi_server_send_sync_message (ase,
                                                            SMI_MSG_STATUS,
                                                            SMI_ERROR, NULL));
           }
         status = nsm_if_flag_up_unset(0, msg->ifname, PAL_TRUE);

         ret = nsm_smi_server_send_sync_message (ase, SMI_MSG_STATUS,
                                                 status, NULL);
        break;

       case SMI_MSG_IF_SETAUTO:

         if (nm == NULL)
           {
             SMI_FN_EXIT (nsm_smi_server_send_sync_message (ase,
                                                            SMI_MSG_STATUS,
                                                            SMI_ERROR, NULL));
           }
         ifp = if_lookup_by_name (&nm->vr->ifm, msg->ifname);
         if (ifp == NULL)
           {
             SMI_FN_EXIT (nsm_smi_server_send_sync_message (ase,
                                                            SMI_MSG_STATUS,
                                                            SMI_ERROR, NULL));
           }
         status = nsm_if_duplex_set(SMI_DFLT_VRID, msg->ifname,
                  NSM_IF_AUTO_NEGO );

         ret = nsm_smi_server_send_sync_message (ase, SMI_MSG_STATUS,
                                                 status, NULL);
        break;

       case SMI_MSG_IF_GETAUTO:
         if (nm == NULL)
           {
             SMI_FN_EXIT (nsm_smi_server_send_sync_message (ase,
                                                            SMI_MSG_STATUS,
                                                            SMI_ERROR, NULL));
           }
         ifp = if_lookup_by_name (&nm->vr->ifm, msg->ifname);
         if (ifp == NULL)
           {
             SMI_FN_EXIT (nsm_smi_server_send_sync_message (ase,
                                                            SMI_MSG_STATUS,
                                                            SMI_ERROR, NULL));
           }
         /* Interface Auto-negotiation TLV */
         SMI_SET_CTYPE (replymsg.cindex, SMI_IF_CTYPE_AUTONEG);

         status = nsm_fea_if_get_autonego (ifp, &replymsg.autoneg);

         if(status < 0)
           msgtype = SMI_MSG_STATUS;

         ret = nsm_smi_server_send_sync_message (ase, msgtype, status,
                                               &replymsg);
       break;

       case SMI_MSG_IF_SETHWADDR:
         if (nm == NULL)
           {
             SMI_FN_EXIT (nsm_smi_server_send_sync_message (ase,
                                                            SMI_MSG_STATUS,
                                                            SMI_ERROR, NULL));
           }
          ifp = if_lookup_by_name (&nm->vr->ifm, msg->ifname);
          if (ifp == NULL)
           {
             SMI_FN_EXIT (nsm_smi_server_send_sync_message (ase,
                                                            SMI_MSG_STATUS,
                                                            SMI_ERROR, NULL));
           }
          /* TODO:check in "hal_if_set_hwaddr" the hardware address*/
          status = hal_if_set_hwaddr(msg->ifname, ifp->ifindex,
                                     msg->hw_addr,
                                     SMI_ETHER_ADDR_LEN);

          ret = nsm_smi_server_send_sync_message (ase, SMI_MSG_STATUS,
                                                  status, NULL);
       break;

       case SMI_MSG_IF_GETHWADDR:
         {
          int hw_len;
           if (nm == NULL)
             {
               SMI_FN_EXIT (nsm_smi_server_send_sync_message (ase,
                                                              SMI_MSG_STATUS,
                                                              SMI_ERROR, NULL));
             }
           ifp = if_lookup_by_name (&nm->vr->ifm, msg->ifname);
           if (ifp == NULL)
           {
             SMI_FN_EXIT (nsm_smi_server_send_sync_message (ase,
                                                            SMI_MSG_STATUS,
                                                            SMI_ERROR, NULL));
           }

          /* Interface Hardware Address TLV */
          SMI_SET_CTYPE (replymsg.cindex, SMI_IF_CTYPE_HWADDR);
          hw_len = INTERFACE_HWADDR_MAX;
          /* Internal SMI to fetch Hardware Address */
          status = hal_if_get_hwaddr(msg->ifname, ifp->ifindex,
                                     replymsg.hw_addr, &hw_len);

          if(status < 0)
            msgtype = SMI_MSG_STATUS;

           ret = nsm_smi_server_send_sync_message (ase, msgtype, status,
                                               &replymsg);
         }
       break;

       case SMI_MSG_IF_SETDUPLEX:
         if (nm == NULL)
         {
           SMI_FN_EXIT (nsm_smi_server_send_sync_message (ase,
                                                          SMI_MSG_STATUS,
                                                          SMI_ERROR, NULL));
         }

         ifp = if_lookup_by_name (&nm->vr->ifm, msg->ifname);
         if (ifp == NULL)
         {
           SMI_FN_EXIT (nsm_smi_server_send_sync_message (ase,
                                                          SMI_MSG_STATUS,
                                                          SMI_ERROR, NULL));
         }

         status = nsm_if_duplex_set(SMI_DFLT_VRID, msg->ifname, msg->duplex);

         ret = nsm_smi_server_send_sync_message (ase, SMI_MSG_STATUS, status,
                                                 NULL);
       break;

       case SMI_MSG_IF_GETDUPLEX:
         /* Interface Duplex TLV */
         SMI_SET_CTYPE (replymsg.cindex, SMI_IF_CTYPE_DUPLEX);

         status = nsm_if_duplex_get(SMI_DFLT_VRID, msg->ifname,
                                    &replymsg.duplex);

         if(status < 0)
           msgtype = SMI_MSG_STATUS;

         ret = nsm_smi_server_send_sync_message (ase, msgtype, status,
                                                 &replymsg);
       break;

       case SMI_MSG_IF_GETBRIEF:
         if (nm == NULL)
           {
             SMI_FN_EXIT (nsm_smi_server_send_sync_message (ase,
                                                            SMI_MSG_STATUS,
                                                            SMI_ERROR, NULL));
           }

         ifp = if_lookup_by_name (&nm->vr->ifm, msg->ifname);
         if (ifp == NULL)
           {
             SMI_FN_EXIT (nsm_smi_server_send_sync_message (ase,
                                                            SMI_MSG_STATUS,
                                                            SMI_ERROR, NULL));
           }
         SMI_SET_CTYPE (replymsg.cindex, SMI_IF_CTYPE_FLAG);
         if (if_is_up (ifp))
           replymsg.flag = SMI_IF_UP;
         else
            replymsg.flag = SMI_IF_DOWN;

         SMI_SET_CTYPE (replymsg.cindex, SMI_IF_CTYPE_STATUS);
         if (if_is_running (ifp))
           replymsg.if_status = SMI_IF_UP;
         else
           replymsg.if_status = SMI_IF_DOWN;

         ret = nsm_smi_server_send_sync_message (ase, msgtype, status,
                                                 &replymsg);
       break;

       case SMI_MSG_IF_SETMCAST:
         if (nm == NULL)
           {
             SMI_FN_EXIT (nsm_smi_server_send_sync_message (ase,
                                                            SMI_MSG_STATUS,
                                                            SMI_ERROR, NULL));
           }
         ifp = if_lookup_by_name (&nm->vr->ifm, msg->ifname);
         if (ifp == NULL)
         {
           SMI_FN_EXIT (nsm_smi_server_send_sync_message (ase,
                                                          SMI_MSG_STATUS,
                                                          SMI_ERROR, NULL));
         }
         status = nsm_if_flag_multicast_set(SMI_DFLT_VRID, msg->ifname);
         ret = nsm_smi_server_send_sync_message (ase, SMI_MSG_STATUS, status,
                                                 NULL);
         break;

       case SMI_MSG_IF_GETMCAST:
         /* Interface MCAST TLV */
         SMI_SET_CTYPE (replymsg.cindex, SMI_IF_CTYPE_MCAST);

         status = nsm_if_flag_multicast_get(SMI_DFLT_VRID, msg->ifname,
                                            &replymsg.mcast);

         if(status < 0)
           msgtype = SMI_MSG_STATUS;

         ret = nsm_smi_server_send_sync_message (ase, msgtype, status,
                                                 &replymsg);
       break;

       case SMI_MSG_IF_UNSETMT:
         if (nm == NULL)
           {
             SMI_FN_EXIT (nsm_smi_server_send_sync_message (ase,
                                                            SMI_MSG_STATUS,
                                                            SMI_ERROR, NULL));
           }
         ifp = if_lookup_by_name (&nm->vr->ifm, msg->ifname);
         if (ifp == NULL)
         {
           SMI_FN_EXIT (nsm_smi_server_send_sync_message (ase,
                                                          SMI_MSG_STATUS,
                                                          SMI_ERROR, NULL));
         }
        status = nsm_if_flag_multicast_unset(SMI_DFLT_VRID, msg->ifname);
        ret = nsm_smi_server_send_sync_message (ase, SMI_MSG_STATUS, status,
                                                NULL);
        break;

       case SMI_MSG_IF_CHANGE_GET:
         if (nm == NULL)
           {
             SMI_FN_EXIT (nsm_smi_server_send_sync_message (ase,
                                                            SMI_MSG_STATUS,
                                                            SMI_ERROR, NULL));
           }

         ifp = if_lookup_by_name (&nm->vr->ifm, msg->ifname);
         if (ifp == NULL)
           {
             SMI_FN_EXIT (nsm_smi_server_send_sync_message (ase,
                                                            SMI_MSG_STATUS,
                                                            SMI_ERROR, NULL));
           }

        /* Interface Status change TLV */
        SMI_SET_CTYPE (replymsg.cindex, SMI_IF_CTYPE_IFSTATUS);
        zif = ifp->info;
        if (zif != NULL)
          {
            if (zif->nsm_if_link_changed == PAL_TRUE)
              replymsg.smi_if_link_change = SMI_IF_LINK_CHANGED;
            else
              replymsg.smi_if_link_change = SMI_IF_LINK_UNCHANGED;

            zif->nsm_if_link_changed = PAL_FALSE;
           }

        if(status < 0) /* failure */
            msgtype = SMI_MSG_STATUS;

        ret = nsm_smi_server_send_sync_message (ase, msgtype, status,
                                                &replymsg);
       break;

       case SMI_MSG_IF_GET_STATISTICS:
       {
         if (nm == NULL)
           {
             SMI_FN_EXIT (nsm_smi_server_send_sync_message (ase,
                                                            SMI_MSG_STATUS,
                                                            SMI_ERROR, NULL));
           }

         ifp = if_lookup_by_name (&nm->vr->ifm, msg->ifname);
         if (ifp == NULL)
           {
             SMI_FN_EXIT (nsm_smi_server_send_sync_message (ase,
                                                            SMI_MSG_STATUS,
                                                            SMI_ERROR, NULL));
           }
         /* Interface Statistics TLV */
         SMI_SET_CTYPE (replymsg.cindex, SMI_IF_CTYPE_STATISTICS);

         status = nsm_interface_get_counters(ifp, &(replymsg.ifstats));
         if(status < 0) /* failure */
             msgtype = SMI_MSG_STATUS;

         ret = nsm_smi_server_send_sync_message (ase, msgtype, status,
                                                 &replymsg);
       }
       break;

       case SMI_MSG_IF_CLEAR_STATISTICS:
       {
          nm = nsm_master_lookup_by_id (nzg, SMI_DFLT_VRID);
         if (nm == NULL)
           {
             SMI_FN_EXIT (nsm_smi_server_send_sync_message (ase,
                                                            SMI_MSG_STATUS,
                                                            SMI_ERROR, NULL));
           }

         ifp = if_lookup_by_name (&nm->vr->ifm, msg->ifname);
         if (ifp == NULL)
           {
             SMI_FN_EXIT (nsm_smi_server_send_sync_message (ase,
                                                            SMI_MSG_STATUS,
                                                            SMI_ERROR, NULL));
           }

         status = nsm_interface_clear_counters(ifp);

         ret = nsm_smi_server_send_sync_message (ase, SMI_MSG_STATUS, status,
                                                 NULL);
       }
       break;

       case SMI_MSG_IF_SET_MDIX_CROSSOVER:
       {
         if (nm == NULL)
           {
             SMI_FN_EXIT (nsm_smi_server_send_sync_message (ase,
                                                            SMI_MSG_STATUS,
                                                            SMI_ERROR, NULL));
           }

         ifp = if_lookup_by_name (&nm->vr->ifm, msg->ifname);
         if (ifp == NULL)
           {
             SMI_FN_EXIT (nsm_smi_server_send_sync_message (ase,
                                                            SMI_MSG_STATUS,
                                                            SMI_ERROR, NULL));
           }

         status = nsm_if_mdix_crossover_set(ifp, msg->cross_mode);
         ret = nsm_smi_server_send_sync_message (ase, SMI_MSG_STATUS, status,
                                                 NULL);
       }
       break;

       case SMI_MSG_IF_GET_MDIX_CROSSOVER:
       {
         if (nm == NULL)
         {
           SMI_FN_EXIT (nsm_smi_server_send_sync_message (ase,
                                                          SMI_MSG_STATUS,
                                                            SMI_ERROR, NULL));
         }

         ifp = if_lookup_by_name (&nm->vr->ifm, msg->ifname);
         if (ifp == NULL)
         {
           SMI_FN_EXIT (nsm_smi_server_send_sync_message (ase,
                                                          SMI_MSG_STATUS,
                                                          SMI_ERROR, NULL));
         }

         /* MDIX TLV */
         SMI_SET_CTYPE (replymsg.cindex, SMI_IF_CTYPE_MDIX_CROSSOVER);

         status = nsm_if_mdix_crossover_get(ifp, &(replymsg.cross_mode));
         if(status < 0)
           msgtype = SMI_MSG_STATUS;

         ret = nsm_smi_server_send_sync_message (ase, msgtype, status,
                                                 &replymsg);
       }
       break;

      case SMI_MSG_APN_GET_TRAFFIC_CLASSTBL:
      {
        if (nm == NULL)
         {
           SMI_FN_EXIT (nsm_smi_server_send_sync_message (ase,
                                                          SMI_MSG_STATUS,
                                                          SMI_ERROR, NULL));
         }

         ifp = if_lookup_by_name (&nm->vr->ifm, msg->ifname);
         if (ifp == NULL)
         {
           SMI_FN_EXIT (nsm_smi_server_send_sync_message (ase,
                                                          SMI_MSG_STATUS,
                                                          SMI_ERROR, NULL));
         }

        SMI_SET_CTYPE (replymsg.cindex, SMI_IF_CTYPE_TRAFFIC_CLASSTBL);

        status = nsm_apn_get_traffic_class_table(ifp,
                                                 replymsg.traffic_class_table);
        if(status < 0) /* failure */
            msgtype = SMI_MSG_STATUS;

        ret = nsm_smi_server_send_sync_message (ase, msgtype, status,
                                                &replymsg);
      }
      break;

      case SMI_MSG_IF_BRIDGE_ADDMAC:
      {
        struct nsm_bridge_master *master = nsm_bridge_get_master (nm);

        if (nm == NULL)
         {
           SMI_FN_EXIT (nsm_smi_server_send_sync_message (ase,
                                                          SMI_MSG_STATUS,
                                                            SMI_ERROR, NULL));
         }

        ifp = if_lookup_by_name (&nm->vr->ifm, msg->ifname);
        if (ifp == NULL)
        {
          SMI_FN_EXIT (nsm_smi_server_send_sync_message (ase,
                                                         SMI_MSG_STATUS,
                                                         SMI_ERROR, NULL));
        }

        master = nsm_bridge_get_master (nm);

        status = nsm_bridge_add_mac(master, msg->bridge_name, ifp,
                                    msg->hw_addr, msg->vid, NSM_VLAN_NULL_VID,
                                    HAL_L2_FDB_STATIC, msg->is_forward, 0);

        ret = nsm_smi_server_send_sync_message (ase, SMI_MSG_STATUS, status,
                                                NULL);
      }
      break;

      case SMI_MSG_IF_BRIDGE_DELMAC:
      {
        struct nsm_bridge_master *master = nsm_bridge_get_master (nm);

        if (nm == NULL)
        {
          SMI_FN_EXIT (nsm_smi_server_send_sync_message (ase,
                                                         SMI_MSG_STATUS,
                                                           SMI_ERROR, NULL));
        }

        ifp = if_lookup_by_name (&nm->vr->ifm, msg->ifname);
        if (ifp == NULL)
        {
          SMI_FN_EXIT (nsm_smi_server_send_sync_message (ase,
                                                         SMI_MSG_STATUS,
                                                         SMI_ERROR, NULL));
        }

        master = nsm_bridge_get_master (nm);

        status = nsm_bridge_delete_mac(master, msg->bridge_name, ifp,
                                       msg->hw_addr, msg->vid,
                                       NSM_VLAN_NULL_VID,
                                       HAL_L2_FDB_STATIC, msg->is_forward, 0);

        ret = nsm_smi_server_send_sync_message (ase, SMI_MSG_STATUS, status,
                                                NULL);
      }
      break;

      case SMI_MSG_IF_BRIDGE_MAC_ADD_PRIO_OVR:
      {
        struct nsm_bridge_master *master = nsm_bridge_get_master (nm);

        if (nm == NULL)
         {
           SMI_FN_EXIT (nsm_smi_server_send_sync_message (ase,
                                                          SMI_MSG_STATUS,
                                                          SMI_ERROR, NULL));
         }

        ifp = if_lookup_by_name (&nm->vr->ifm, msg->ifname);
        if (ifp == NULL)
        {
          SMI_FN_EXIT (nsm_smi_server_send_sync_message (ase,
                                                         SMI_MSG_STATUS,
                                                         SMI_ERROR, NULL));
        }

        master = nsm_bridge_get_master (nm);

        status = nsm_bridge_add_mac_prio_ovr (master, msg->bridge_name, ifp,
                                              msg->hw_addr, msg->vid,
                                              msg->ovr_mac_type,
                                              msg->priority);

        ret = nsm_smi_server_send_sync_message (ase, SMI_MSG_STATUS, status,
                                                NULL);
      }
      break;

      case SMI_MSG_IF_BRIDGE_MAC_DEL_PRIO_OVR:
      {
        struct nsm_bridge_master *master = nsm_bridge_get_master (nm);

        if (nm == NULL)
        {
          SMI_FN_EXIT (nsm_smi_server_send_sync_message (ase,
                                                         SMI_MSG_STATUS,
                                                         SMI_ERROR, NULL));
        }

        ifp = if_lookup_by_name (&nm->vr->ifm, msg->ifname);
        if (ifp == NULL)
        {
          SMI_FN_EXIT (nsm_smi_server_send_sync_message (ase,
                                                         SMI_MSG_STATUS,
                                                         SMI_ERROR, NULL));
        }

        master = nsm_bridge_get_master (nm);

        status = nsm_bridge_del_mac_prio_ovr (master, msg->bridge_name, ifp,
                                              msg->hw_addr, msg->vid);

        ret = nsm_smi_server_send_sync_message (ase, SMI_MSG_STATUS, status,
                                                NULL);
      }
      break;

      case SMI_MSG_IF_BRIDGE_FLUSH_DYNAMICENT:
       {
        struct nsm_bridge_master *master;
        if (nm == NULL)
        {
          SMI_FN_EXIT (nsm_smi_server_send_sync_message (ase,
                                                         SMI_MSG_STATUS,
                                                         SMI_ERROR, NULL));
        }
        master = nsm_bridge_get_master (nm);

        status = nsm_bridge_clear_dynamic_mac_bridge(master, msg->bridge_name);

        ret = nsm_smi_server_send_sync_message (ase, SMI_MSG_STATUS, status,
                                                NULL);
      }
      break;

      case SMI_MSG_ADD_BRIDGE:
      {
        struct nsm_bridge_master *master = nsm_bridge_get_master (nm);
        if (! master)
        {
          SMI_FN_EXIT (nsm_smi_server_send_sync_message (ase,
                                                         SMI_MSG_STATUS,
                                                         SMI_ERROR, NULL));
        }

        /* Translation from SMI to NSM */
        switch (msg->type)
        {
          case SMI_BRIDGE_STP:
            msg->type = NSM_BRIDGE_TYPE_STP_VLANAWARE;
            break;

          case SMI_BRIDGE_RSTP:
            msg->type = NSM_BRIDGE_TYPE_RSTP_VLANAWARE;
            break;

          case SMI_BRIDGE_MSTP:
            msg->type = NSM_BRIDGE_TYPE_MSTP;
            break;

          case SMI_BRIDGE_PROVIDER_RSTP:
            msg->type = NSM_BRIDGE_TYPE_PROVIDER_RSTP;
            break;

          case SMI_BRIDGE_PROVIDER_MSTP:
            msg->type = NSM_BRIDGE_TYPE_PROVIDER_MSTP;
            break;

          default:
            break;
        }

        status = nsm_bridge_add (master, msg->type, msg->bridge_name,
                                 PAL_FALSE, msg->topo_type, PAL_FALSE, 
                                 PAL_FALSE, NULL);

        ret = nsm_smi_server_send_sync_message (ase, SMI_MSG_STATUS, status,
                                                NULL);
      }
      break;

      case SMI_MSG_DEL_BRIDGE:
      {
        struct nsm_bridge_master *master = nsm_bridge_get_master (nm);
        if (! master)
        {
          SMI_FN_EXIT (nsm_smi_server_send_sync_message (ase,
                                                         SMI_MSG_STATUS,
                                                         SMI_ERROR, NULL));
        }

        status = nsm_bridge_delete (master, msg->bridge_name);

        ret = nsm_smi_server_send_sync_message (ase, SMI_MSG_STATUS, status,
                                                NULL);
      }
      break;

      case SMI_MSG_ADD_BRIDGE_PORT:
      {
        struct nsm_bridge_master *master = nsm_bridge_get_master (nm);

        if (! master)
        {
          SMI_FN_EXIT (nsm_smi_server_send_sync_message (ase,
                                                         SMI_MSG_STATUS,
                                                         SMI_ERROR, NULL));
        }

        ifp = if_lookup_by_name (&nm->vr->ifm, msg->ifname);
        if (ifp == NULL)
        {
          SMI_FN_EXIT (nsm_smi_server_send_sync_message (ase,
                                                         SMI_MSG_STATUS,
                                                         SMI_ERROR, NULL));
        }

        status = nsm_bridge_api_port_add (master, msg->bridge_name,
                                          ifp, PAL_TRUE, PAL_TRUE, PAL_TRUE,
                                          msg->spanning_tree_disable);

        ret = nsm_smi_server_send_sync_message (ase, SMI_MSG_STATUS, status,
                                                NULL);
      }
      break;

      case SMI_MSG_DEL_BRIDGE_PORT:
      {
        struct nsm_bridge_master *master = nsm_bridge_get_master (nm);
        if (! master)
        {
          SMI_FN_EXIT (nsm_smi_server_send_sync_message (ase,
                                                         SMI_MSG_STATUS,
                                                         SMI_ERROR, NULL));
        }

        ifp = if_lookup_by_name (&nm->vr->ifm, msg->ifname);
        if (ifp == NULL)
        {
          SMI_FN_EXIT (nsm_smi_server_send_sync_message (ase,
                                                         SMI_MSG_STATUS,
                                                         SMI_ERROR, NULL));
        }

        status = nsm_bridge_port_delete (master, msg->bridge_name,
                                         ifp, PAL_FALSE, PAL_TRUE);

        ret = nsm_smi_server_send_sync_message (ase, SMI_MSG_STATUS, status,
                                                NULL);
      }
      break;

     break;

      case SMI_MSG_CHANGE_TYPE:
      {
        struct nsm_bridge_master *master = nsm_bridge_get_master (nm);
        struct nsm_bridge *bridge = NULL;
        int bridge_exist=0;

        if (! master)
        {
          SMI_FN_EXIT (nsm_smi_server_send_sync_message (ase,
                                                         SMI_MSG_STATUS,
                                                         SMI_ERROR, NULL));
        }

        bridge = nsm_lookup_bridge_by_name (master, msg->bridge_name);
        if (! bridge)
        {
          SMI_FN_EXIT (nsm_smi_server_send_sync_message (ase,
                                                         SMI_MSG_STATUS,
                                                         SMI_ERROR, NULL));
        }

        if (msg->type == SMI_BRIDGE_TYPE_INVALID)
          msg->type = bridge->type;

        if (msg->topo_type == SMI_TOPO_INVALID)
          msg->topo_type = bridge->topology_type;

        /* Translation from SMI to NSM */
        switch (msg->type)
        {
          case SMI_BRIDGE_STP:
          case SMI_BRIDGE_TYPE_STP_VLANAWARE:
            if (bridge->type == NSM_BRIDGE_TYPE_STP_VLANAWARE)
              bridge_exist = 1;
            else
              msg->type = NSM_BRIDGE_TYPE_STP_VLANAWARE;
            break;

          case SMI_BRIDGE_RSTP:
          case SMI_BRIDGE_TYPE_RSTP_VLANAWARE:
            if (bridge->type == NSM_BRIDGE_TYPE_RSTP_VLANAWARE)
              bridge_exist = 1;
            else
              msg->type = NSM_BRIDGE_TYPE_RSTP_VLANAWARE;
            break;

          case SMI_BRIDGE_MSTP:
            if (bridge->type == NSM_BRIDGE_TYPE_MSTP)
              bridge_exist = 1;
            else
              msg->type = NSM_BRIDGE_TYPE_MSTP;
            break;

          case SMI_BRIDGE_PROVIDER_RSTP:
            if (bridge->type == NSM_BRIDGE_TYPE_PROVIDER_RSTP)
              bridge_exist = 1;
            else
              msg->type = NSM_BRIDGE_TYPE_PROVIDER_RSTP;
            break;

          case SMI_BRIDGE_PROVIDER_MSTP:
            if (bridge->type == NSM_BRIDGE_TYPE_PROVIDER_MSTP)
              bridge_exist = 1;
            else
              msg->type = NSM_BRIDGE_TYPE_PROVIDER_MSTP;
            break;

          default:
            break;
        }

       if (!bridge_exist)
        {
          status = nsm_bridge_change_type (master, msg->bridge_name,
                                           msg->type, msg->topo_type);
        }
       else
          status = 0;

        ret = nsm_smi_server_send_sync_message (ase, SMI_MSG_STATUS, status,
                                                NULL);
      }
      break;

      case SMI_MSG_GETBRIDGE_TYPE:
      {
        struct nsm_bridge_master *master = nsm_bridge_get_master (nm);
        if (! master)
        {
          SMI_FN_EXIT (nsm_smi_server_send_sync_message (ase,
                                                         SMI_MSG_STATUS,
                                                         SMI_ERROR, NULL));
        }

        /* Bridge type TLV */
        SMI_SET_CTYPE (replymsg.cindex, SMI_IF_CTYPE_BRIDGETYPE);
        /* Topology type TLV */
        SMI_SET_CTYPE (replymsg.cindex, SMI_IF_CTYPE_TOPOTYPE);

        status = nsm_api_bridge_get_type (master, msg->bridge_name,
                                          &(replymsg.type),
                                          &(replymsg.topo_type));
        switch (replymsg.type)
        {
          case NSM_BRIDGE_TYPE_STP_VLANAWARE:
            replymsg.type = SMI_BRIDGE_STP;
            break;

          case NSM_BRIDGE_TYPE_RSTP_VLANAWARE:
            replymsg.type = SMI_BRIDGE_RSTP;
            break;

          case NSM_BRIDGE_TYPE_MSTP:
            replymsg.type = SMI_BRIDGE_MSTP;
            break;

          case NSM_BRIDGE_TYPE_PROVIDER_RSTP:
            replymsg.type = SMI_BRIDGE_PROVIDER_RSTP;
            break;

          case NSM_BRIDGE_TYPE_PROVIDER_MSTP:
            replymsg.type = SMI_BRIDGE_PROVIDER_MSTP;
            break;

          default:
            break;
        }

        if(status < 0)
          msgtype = SMI_MSG_STATUS;

        ret = nsm_smi_server_send_sync_message (ase, msgtype, status,
                                                &replymsg);
      }
      break;

      case SMI_MSG_SET_PORT_NON_CONFIG:
      {
        ifp = if_lookup_by_name (&nm->vr->ifm, msg->ifname);
        if (ifp == NULL)
        {
          SMI_FN_EXIT (nsm_smi_server_send_sync_message (ase,
                                                         SMI_MSG_STATUS,
                                                         SMI_ERROR, NULL));
        }
        status = nsm_if_non_conf_set (ifp, msg->port_conf_state);
        ret = nsm_smi_server_send_sync_message (ase, SMI_MSG_STATUS, status,
                                                NULL);
      }
      break;

      case SMI_MSG_GET_PORT_NON_CONFIG:
      {
        ifp = if_lookup_by_name (&nm->vr->ifm, msg->ifname);
        if (ifp == NULL)
        {
          SMI_FN_EXIT (nsm_smi_server_send_sync_message (ase,
                                                         SMI_MSG_STATUS,
                                                         SMI_ERROR, NULL));
        }

        SMI_SET_CTYPE (replymsg.cindex, SMI_IF_CTYPE_PORT_STATE);

        status = nsm_if_non_conf_get (ifp, &(replymsg.port_conf_state));
        if (status < 0)
          msgtype = SMI_MSG_STATUS;

        ret = nsm_smi_server_send_sync_message (ase, msgtype, status,
                                                &replymsg);
      }
      break;

      case SMI_MSG_SET_PORT_LEARNING:
      {
        ifp = if_lookup_by_name (&nm->vr->ifm, msg->ifname);
        if (ifp == NULL)
        {
          SMI_FN_EXIT (nsm_smi_server_send_sync_message (ase,
                                                         SMI_MSG_STATUS,
                                                         SMI_ERROR, NULL));
        }

        status = nsm_set_learn_disable (ifp, msg->learn_state);

        ret = nsm_smi_server_send_sync_message (ase, SMI_MSG_STATUS, status,
                                                NULL);
      }
      break;

      case SMI_MSG_GET_PORT_LEARNING:
      {
        ifp = if_lookup_by_name (&nm->vr->ifm, msg->ifname);
        if (ifp == NULL)
        {
          SMI_FN_EXIT (nsm_smi_server_send_sync_message (ase,
                                                         SMI_MSG_STATUS,
                                                         SMI_ERROR, NULL));
        }

        SMI_SET_CTYPE (replymsg.cindex, SMI_IF_CTYPE_LEARN_STATE);

        status = nsm_get_learn_disable (ifp, &(replymsg.learn_state));
        if (status < 0)
          msgtype = SMI_MSG_STATUS;
        ret = nsm_smi_server_send_sync_message (ase, msgtype, status,
                                                &replymsg);
      }
      break;

      case SMI_IF_MSG_SET_DOT1Q_STATE:
      {
        ifp = if_lookup_by_name (&nm->vr->ifm, msg->ifname);

        if (ifp == NULL)
        {
          SMI_FN_EXIT (nsm_smi_server_send_sync_message (ase,
                                                         SMI_MSG_STATUS,
                                                         SMI_ERROR, NULL));
        }

        status = nsm_vlan_port_set_dot1q_state (ifp, msg->dot1q_state);

        ret = nsm_smi_server_send_sync_message (ase, SMI_MSG_STATUS, status,
                                                NULL);
      }
      break;

      case SMI_IF_MSG_GET_DOT1Q_STATE:
      {
        ifp = if_lookup_by_name (&nm->vr->ifm, msg->ifname);

        if (ifp == NULL)
        {
          SMI_FN_EXIT (nsm_smi_server_send_sync_message (ase,
                                                         SMI_MSG_STATUS,
                                                         SMI_ERROR, NULL));
        }

        SMI_SET_CTYPE (replymsg.cindex, SMI_IF_CTYPE_DOT1Q_STATE);

        status = nsm_vlan_port_get_dot1q_state (ifp, &(replymsg.dot1q_state));

        if (status < 0)
          msgtype = SMI_MSG_STATUS;

        ret = nsm_smi_server_send_sync_message (ase, msgtype, status,
                                                &replymsg);
      }
      break;

      case SMI_IF_MSG_SET_DTAG_MODE:
      {
        ifp = if_lookup_by_name (&nm->vr->ifm, msg->ifname);

        if (ifp == NULL)
        {
          SMI_FN_EXIT (nsm_smi_server_send_sync_message (ase,
                                                         SMI_MSG_STATUS,
                                                         SMI_ERROR, NULL));
        }

        status = nsm_pro_vlan_set_dtag_mode (ifp, msg->dtag_mode);

        ret = nsm_smi_server_send_sync_message (ase, SMI_MSG_STATUS, status,
                                                NULL);
      }
      break;

      case SMI_IF_MSG_GET_DTAG_MODE:
      {
        ifp = if_lookup_by_name (&nm->vr->ifm, msg->ifname);

        if (ifp == NULL)
        {
          SMI_FN_EXIT (nsm_smi_server_send_sync_message (ase,
                                                         SMI_MSG_STATUS,
                                                         SMI_ERROR, NULL));
        }

        SMI_SET_CTYPE (replymsg.cindex, SMI_IF_CTYPE_DTAG_MODE);

        status = nsm_pro_vlan_get_dtag_mode (ifp, &(replymsg.dtag_mode));

        if (status < 0)
          msgtype = SMI_MSG_STATUS;

        ret = nsm_smi_server_send_sync_message (ase, msgtype, status,
                                                &replymsg);
      }
      break;

      case SMI_MSG_SW_RESET:
      {
        status = nsm_set_sw_reset ();

        ret = nsm_smi_server_send_sync_message (ase, SMI_MSG_STATUS, status,
                                                NULL);
      }

      case SMI_MSG_SET_EGRESS_PORT_MODE:
      {
        ifp = if_lookup_by_name (&nm->vr->ifm, msg->ifname);
        if (ifp == NULL)
        {
          SMI_FN_EXIT (nsm_smi_server_send_sync_message (ase,
                                                         SMI_MSG_STATUS,
                                                         SMI_ERROR, NULL));
        }
        status = nsm_set_egress_port_mode (ifp, msg->egress_mode);
        ret = nsm_smi_server_send_sync_message (ase, SMI_MSG_STATUS, status,
                                                NULL);
      }
      break;

      case SMI_MSG_SET_PORT_NON_SWITCHING:
        /* TODO: Internal SMI
        status = nsm_smi_set_port_non_switch (msg->if_name,
                                               msg->port_switch_state); */
        ret = nsm_smi_server_send_sync_message (ase, SMI_MSG_STATUS, status,
                                                 NULL);
      break;

      case SMI_MSG_GET_PORT_NON_SWITCHING:
        ifp = if_lookup_by_name (&nm->vr->ifm, msg->ifname);
        if (ifp == NULL)
        {
          SMI_FN_EXIT (nsm_smi_server_send_sync_message (ase,
                                                         SMI_MSG_STATUS,
                                                         SMI_ERROR, NULL));
        }
        /* Non switch TLV */
        SMI_SET_CTYPE (replymsg.cindex, SMI_IF_CTYPE_PORT_SWITCH_STATE);

        status = nsm_api_get_port_non_switch (ifp,
                                              &(replymsg.port_switch_state));
        if(status < 0)
          msgtype = SMI_MSG_STATUS;

        ret = nsm_smi_server_send_sync_message (ase, msgtype, status,
                                                 &replymsg);
      break;

      case SMI_MSG_IF_GETFLAGS:
         if (nm == NULL)
           {
             SMI_FN_EXIT (nsm_smi_server_send_sync_message (ase,
                                                            SMI_MSG_STATUS,
                                                            SMI_ERROR, NULL));
           }
         ifp = if_lookup_by_name (&nm->vr->ifm, msg->ifname);
         if (ifp == NULL)
           {
             SMI_FN_EXIT (nsm_smi_server_send_sync_message (ase,
                                                            SMI_MSG_STATUS,
                                                            SMI_ERROR, NULL));
           }

         SMI_SET_CTYPE (replymsg.cindex, SMI_IF_CTYPE_FLAG);

         status = nsm_fea_if_flags_get (ifp);
         if (status < 0)
           msgtype = SMI_MSG_STATUS;

         replymsg.flag = ifp->flags;

         ret = nsm_smi_server_send_sync_message (ase, msgtype,
                                                 status, &replymsg);
       break;

      case SMI_MSG_IF_LACP_ADDLINK:
       {
         ifp = if_lookup_by_name (&nm->vr->ifm, msg->ifname);
         if (ifp == NULL)
         {
           SMI_FN_EXIT (nsm_smi_server_send_sync_message (ase,
                                                          SMI_MSG_STATUS,
                                                         SMI_ERROR, NULL));
         }

         zif = (struct nsm_if *)ifp->info;
         if (! zif)
         {
           /* Interface not found*/
           SMI_FN_EXIT (nsm_smi_server_send_sync_message (ase,
                                                        SMI_MSG_STATUS,
                                                        SMI_ERROR, NULL));
         }
         #ifdef HAVE_PVLAN
         br_port = zif->switchport;
         if (br_port && &br_port->vlan_port)
          {
            if ((&br_port->vlan_port)->pvlan_configured)
            {
               /* on private vlan port, agg can not be created */
               return CLI_ERROR;
            }
          }
         #endif /* HAVE_PVLAN */
         if (msg->if_lacp_mode == SMI_LACP_PASSIVE)
          {
            zif->agg_mode = PAL_FALSE;
          }
         else if (msg->if_lacp_mode == SMI_LACP_ACTIVE)
          {
            zif->agg_mode = PAL_TRUE;
            chan_activate = PAL_TRUE;
          }
         if ((zif->agg.type == NSM_IF_AGGREGATOR) ||
           (zif->agg_config_type == AGG_CONFIG_STATIC))
         {
            /* Interface not found*/
            SMI_FN_EXIT (nsm_smi_server_send_sync_message (ase,
                                                           SMI_MSG_STATUS,
                                                           SMI_ERROR, NULL));
         }
         zif->agg_config_type = AGG_CONFIG_LACP;
         status = nsm_lacp_api_add_aggregator_member(nm, ifp,
                                                     msg->if_lacp_admin_key,
                                                     chan_activate, PAL_TRUE,
                                                     AGG_CONFIG_LACP);
         ret = nsm_smi_server_send_sync_message (ase, SMI_MSG_STATUS, status,
                                                 NULL);
        break;
       }

       case SMI_IF_MSG_LACP_DELETELINK:
        {
          ifp = if_lookup_by_name (&nm->vr->ifm, msg->ifname);
          if (ifp == NULL)
           {
             SMI_FN_EXIT (nsm_smi_server_send_sync_message (ase,
                                                            SMI_MSG_STATUS,
                                                           SMI_ERROR, NULL));
           }

          zif = (struct nsm_if *)ifp->info;
          if (! zif)
           {
             /* Interface not found*/
             SMI_FN_EXIT (nsm_smi_server_send_sync_message (ase,
                                                           SMI_MSG_STATUS,
                                                           SMI_ERROR, NULL));
           }
           /* Check whether the port is itself an aggregator*/
           if ((zif->agg.type == NSM_IF_AGGREGATOR) ||
               (!ifp->lacp_agg_key))
            {
               SMI_FN_EXIT (nsm_smi_server_send_sync_message (ase,
                                                             SMI_MSG_STATUS,
                                                             SMI_ERROR, NULL));
            }
           status = nsm_lacp_api_delete_aggregator_member (nm, ifp, PAL_TRUE);
           ret = nsm_smi_server_send_sync_message (ase, SMI_MSG_STATUS, status,
                                                     NULL);
         break;
        }

        case SMI_IF_MSG_IF_EXIST:
        {
          struct nsm_if *zif;

          SMI_SET_CTYPE (replymsg.cindex, SMI_IF_CTYPE_IF_EXIST);
          replymsg.exist = SMI_EXIST_NO;

          if (nm == NULL)
          {
            SMI_FN_EXIT (nsm_smi_server_send_sync_message (ase,
                                                           SMI_MSG_STATUS,
                                                           SMI_ERROR, NULL));
          }

          ifp = if_lookup_by_name (&nm->vr->ifm, msg->ifname);
          if (ifp == NULL)
          {
            SMI_FN_EXIT (nsm_smi_server_send_sync_message (ase,
                                                           SMI_MSG_STATUS,
                                                           SMI_ERROR, NULL));
          }

          zif = (struct nsm_if *)ifp->info;
          if (zif->bridge != NULL)
            replymsg.exist = SMI_EXIST_YES;

          ret = nsm_smi_server_send_sync_message (ase, msgtype, status,
                                                  &replymsg);
         }
       break;

       case SMI_IF_MSG_BRIDGE_EXIST:
        {
          struct nsm_bridge_master *master = NULL;
          struct nsm_bridge *br = NULL;

          SMI_SET_CTYPE (replymsg.cindex, SMI_IF_CTYPE_IF_EXIST);
          replymsg.exist = SMI_EXIST_NO;

          if (nm == NULL)
          {
            SMI_FN_EXIT (nsm_smi_server_send_sync_message (ase,
                                                           SMI_MSG_STATUS,
                                                           SMI_ERROR, NULL));
          }

          master = nsm_bridge_get_master (nm);
          if (master == NULL)
          {
            SMI_FN_EXIT (nsm_smi_server_send_sync_message (ase,
                                                           SMI_MSG_STATUS,
                                                           SMI_ERROR, NULL));
          }

          br = nsm_lookup_bridge_by_name (master, msg->bridge_name);
          if (br != NULL)
           replymsg.exist = SMI_EXIST_YES;

          ret = nsm_smi_server_send_sync_message (ase, msgtype, status,
                                                  &replymsg);
         }
       break;

      default:
      break;
    }

  SMI_FN_EXIT(ret);
}

/* Set/Get VLAN params */
int
nsm_smi_server_recv_vlan (struct smi_msg_header *header,
                          void *arg, void *message)
{
  struct smi_server_entry *ase;
  struct smi_server *as;
  struct smi_msg_vlan *msg = (struct smi_msg_vlan *)message;
  int status = 0, ret = 0;
  struct nsm_master *nm = nsm_master_lookup_by_id (nzg, 0);
  struct nsm_bridge_master *master = nsm_bridge_get_master (nm);
  struct nsm_bridge *bridge;
  struct smi_msg_vlan replymsg;
  struct interface *ifp = NULL;
  enum smi_vlan_type vlan_type = 0;
  enum nsm_vlan_port_mode mode, sub_mode;
  int ingress_filter;
  int msgtype = header->type;
  struct smi_vlan_info vlan_info;

  SMI_FN_ENTER ();

  ase = arg;
  as = ase->as;

  if(IS_NSM_DEBUG_RECV)
    smi_vlan_dump (NSM_ZG, msg);

  pal_mem_set (&replymsg, 0, sizeof(struct smi_msg_vlan));

  #ifdef HAVE_HA
  if ((HA_IS_STANDBY (nzg)))
  {
    ret = nsm_check_func_type (msgtype);
    if (ret == SMI_SET_FUNC)
      SMI_FN_EXIT (nsm_smi_server_send_sync_message (ase,
                                                     SMI_MSG_STATUS,
                                                     SMI_ERROR, NULL));
  }
  #endif /* HAVE_HA */

  switch(header->type)
  {
    case SMI_MSG_VLAN_ADD:
    {
      char *vlanname = NULL;

      if(msg->type == VLAN_CVLAN)
      {
        vlan_type = NSM_VLAN_STATIC | NSM_VLAN_CVLAN;
      }
      if(msg->type == VLAN_SVLAN)
      {
        vlan_type = NSM_VLAN_STATIC | NSM_VLAN_SVLAN;
      }

      if (strlen(msg->vlan_name) != 0)
        vlanname = msg->vlan_name;

      /* Probably changing the name of Vlan */
      if (msg->state == SMI_VLAN_INVALID)
      {
        status = nsm_get_vlan_by_id (msg->br_name, msg->vid, &vlan_info);
        if (status < 0)
          SMI_FN_EXIT (nsm_smi_server_send_sync_message (ase,
                                                         SMI_MSG_STATUS,
                                                         SMI_ERROR, NULL));
      }
      else
        vlan_info.vlan_state =  msg->state;

      status = nsm_vlan_add (master, msg->br_name, vlanname,
                             msg->vid, vlan_info.vlan_state, vlan_type);

      ret = nsm_smi_server_send_sync_message (ase, SMI_MSG_STATUS, status,
                                               NULL);
    }
    break;

    case SMI_MSG_VLAN_DEL:
    {
      if(msg->type == VLAN_CVLAN)
      {
        vlan_type = NSM_VLAN_STATIC | NSM_VLAN_CVLAN;
      }
      if(msg->type == VLAN_SVLAN)
      {
        vlan_type = NSM_VLAN_STATIC | NSM_VLAN_SVLAN;
      }
      status = nsm_vlan_delete(master, msg->br_name, msg->vid, vlan_type);
      ret = nsm_smi_server_send_sync_message (ase, SMI_MSG_STATUS, status,
                                              NULL);
    }
    break;
    case SMI_MSG_VLAN_RANGE_ADD:
    {
      int i, exec_status = 0;
      if(msg->type == VLAN_CVLAN)
      {
        vlan_type = NSM_VLAN_STATIC | NSM_VLAN_CVLAN;
      }
      if(msg->type == VLAN_SVLAN)
      {
        vlan_type = NSM_VLAN_STATIC | NSM_VLAN_SVLAN;
      }

      /* VLAN bitmap TLV */
      SMI_SET_CTYPE (replymsg.cindex, SMI_VLAN_CTYPE_BITMAP);
      SMI_BMP_INIT (replymsg.vlan_bmp);

      if (strlen(msg->vlan_name) > 0)
        {
          for (i = msg->lower_vid; i <= msg->higher_vid; i++)
             {
               status = nsm_vlan_add (master, msg->br_name, msg->vlan_name,
                                      i, msg->state, vlan_type);
               if (status < 0)
                 SMI_BMP_SET (replymsg.vlan_bmp, i);
             }
        }
      else
       {
          for (i = msg->lower_vid; i <= msg->higher_vid; i++)
             {
               status = nsm_vlan_add (master, msg->br_name, NULL,
                                      i, msg->state, vlan_type);
               if (status < 0)
                 SMI_BMP_SET (replymsg.vlan_bmp, i);
             }
       }

      ret = nsm_smi_server_send_sync_message (ase, SMI_MSG_STATUS, exec_status,
                                              &replymsg);
    }
    break;
    case SMI_MSG_VLAN_RANGE_DEL:
    {
      int i, exec_status=0;
      if(msg->type == VLAN_CVLAN)
      {
        vlan_type = NSM_VLAN_STATIC | NSM_VLAN_CVLAN;
      }
      if(msg->type == VLAN_SVLAN)
      {
        vlan_type = NSM_VLAN_STATIC | NSM_VLAN_SVLAN;
      }

      /* VLAN bitmap TLV */
      SMI_SET_CTYPE (replymsg.cindex, SMI_VLAN_CTYPE_BITMAP);
      SMI_BMP_INIT (replymsg.vlan_bmp);

      for (i = msg->lower_vid; i <= msg->higher_vid; i++)
        {
          status = nsm_vlan_delete(master, msg->br_name, i, vlan_type);
          if (status < 0)
            SMI_BMP_SET (replymsg.vlan_bmp, i);
        }

      ret = nsm_smi_server_send_sync_message (ase, SMI_MSG_STATUS, exec_status,
                                              &replymsg);
    }
    break;
    case SMI_MSG_VLAN_SET_PORT_MODE:
    {
      ifp = if_lookup_by_name (&nm->vr->ifm, msg->if_name);
      if (ifp == NULL)
      {
         SMI_FN_EXIT (nsm_smi_server_send_sync_message (ase,
                                                        SMI_MSG_STATUS,
                                                        SMI_ERROR, NULL));
      }

      msg->submode = msg->mode;
      status = nsm_vlan_api_set_port_mode (ifp, msg->mode,
                                           msg->submode,
                                           PAL_TRUE, PAL_TRUE);
      ret = nsm_smi_server_send_sync_message (ase, SMI_MSG_STATUS, status,
                                              NULL);
    }
    break;

    case SMI_MSG_VLAN_GET_PORT_MODE:
    {
      replymsg.cindex = 0;
      replymsg.mode = 0;
      replymsg.submode = 0;

      ifp = if_lookup_by_name (&nm->vr->ifm, msg->if_name);

      if(ifp == NULL)
      {
        SMI_FN_EXIT (nsm_smi_server_send_sync_message (ase,
                                                       SMI_MSG_STATUS,
                                                       SMI_ERROR, NULL));
      }

      SMI_SET_CTYPE (replymsg.cindex, SMI_VLAN_CTYPE_PORT_MODE);
      SMI_SET_CTYPE (replymsg.cindex, SMI_VLAN_CTYPE_PORT_SUBMODE);

      status = nsm_vlan_api_get_port_mode (ifp, &mode, &sub_mode);

      if(status < 0)
        msgtype = SMI_MSG_STATUS;

      replymsg.mode = mode;
      replymsg.submode = sub_mode;

      ret = nsm_smi_server_send_sync_message(ase, msgtype, status, &replymsg);
    }
    break;

    case SMI_MSG_VLAN_SET_ACC_FRAME_TYPE:
    {
      ifp = if_lookup_by_name (&nm->vr->ifm, msg->if_name);

      if(ifp == NULL)
      {
        SMI_FN_EXIT (nsm_smi_server_send_sync_message (ase,
                                                       SMI_MSG_STATUS,
                                                       SMI_ERROR, NULL));
      }

      status = nsm_vlan_set_acceptable_frame_type(ifp, msg->mode,
                                                  msg->frame_type);

      ret = nsm_smi_server_send_sync_message(ase, SMI_MSG_STATUS, status, NULL);
    }
    break;

    case SMI_MSG_VLAN_GET_ACC_FRAME_TYPE:
    {
      struct smi_msg_vlan replymsg;
      replymsg.cindex = 0;

      ifp = if_lookup_by_name (&nm->vr->ifm, msg->if_name);

      if(ifp == NULL)
      {
        SMI_FN_EXIT (nsm_smi_server_send_sync_message (ase,
                                                       SMI_MSG_STATUS,
                                                       SMI_ERROR, NULL));
      }

      SMI_SET_CTYPE (replymsg.cindex, SMI_VLAN_CTYPE_ACC_FRAME_TYPE);
      status = nsm_vlan_get_acceptable_frame_type(ifp, &(replymsg.frame_type));

      if(status < 0)
        msgtype = SMI_MSG_STATUS;

      ret = nsm_smi_server_send_sync_message(ase, msgtype, status, &replymsg);
    }
    break;

    case SMI_MSG_VLAN_SET_INGRESS_FILTER:
    {
      ifp = if_lookup_by_name (&nm->vr->ifm, msg->if_name);
      if (ifp == NULL)
      {
        SMI_FN_EXIT (nsm_smi_server_send_sync_message (ase,
                                                       SMI_MSG_STATUS,
                                                       SMI_ERROR, NULL));
      }

      status = nsm_vlan_api_get_port_mode (ifp, &mode, &sub_mode);

      if (status < 0)
      {
        SMI_FN_EXIT (nsm_smi_server_send_sync_message (ase,
                                                       SMI_MSG_STATUS,
                                                       SMI_ERROR, NULL));
      }

      status = nsm_vlan_set_ingress_filter (ifp, mode,
                                            sub_mode,
                                            msg->ingress_filter);

      ret = nsm_smi_server_send_sync_message (ase, SMI_MSG_STATUS, status,
                                              NULL);
    }
    break;

    case SMI_MSG_VLAN_GET_INGRESS_FILTER:
    {
      struct smi_msg_vlan replymsg;

      replymsg.cindex = 0;

      ifp = if_lookup_by_name (&nm->vr->ifm, msg->if_name);
      if(ifp == NULL)
      {
        SMI_FN_EXIT (nsm_smi_server_send_sync_message (ase,
                                                       SMI_MSG_STATUS,
                                                       SMI_ERROR, NULL));
      }

      SMI_SET_CTYPE (replymsg.cindex, SMI_VLAN_CTYPE_PORT_MODE);
      SMI_SET_CTYPE (replymsg.cindex, SMI_VLAN_CTYPE_PORT_SUBMODE);
      SMI_SET_CTYPE (replymsg.cindex, SMI_VLAN_CTYPE_INGRESS_FILTER);

      status = nsm_vlan_get_ingress_filter (ifp, &mode, &sub_mode,
                                            &ingress_filter);

      replymsg.mode = mode;
      replymsg.submode = sub_mode;
      replymsg.ingress_filter = ingress_filter;

      if(status < 0)
        msgtype = SMI_MSG_STATUS;

      ret = nsm_smi_server_send_sync_message(ase, msgtype, status, &replymsg);
    }
    break;

    case SMI_MSG_VLAN_SET_DEFAULT_VID:
    {
      ifp = if_lookup_by_name (&nm->vr->ifm, msg->if_name);

      if(ifp == NULL)
      {
        SMI_FN_EXIT (nsm_smi_server_send_sync_message (ase,
                                                       SMI_MSG_STATUS,
                                                       SMI_ERROR, NULL));
      }
      status = nsm_vlan_api_get_port_mode (ifp, &mode, &sub_mode);

      if (status < 0)
      {
        SMI_FN_EXIT (nsm_smi_server_send_sync_message (ase,
                                                       SMI_MSG_STATUS,
                                                       SMI_ERROR, NULL));
      }

      switch (mode)
      {
        case NSM_VLAN_PORT_MODE_ACCESS:
          status = nsm_vlan_set_access_port (ifp, msg->vid, PAL_TRUE, PAL_TRUE);
          break;
        case NSM_VLAN_PORT_MODE_HYBRID:
          status = nsm_vlan_set_hybrid_port (ifp, msg->vid, PAL_TRUE, PAL_TRUE);
          break;
        case NSM_VLAN_PORT_MODE_CN:
          status = nsm_vlan_set_provider_port (ifp, msg->vid, mode, sub_mode,
                                               PAL_TRUE, PAL_TRUE);
          break;
        default:
          status = SMI_ERROR;
          break;
      }
      ret = nsm_smi_server_send_sync_message(ase, SMI_MSG_STATUS, status, NULL);
    }
    break;

    case SMI_MSG_VLAN_GET_DEFAULT_VID:
    {
      struct smi_msg_vlan replymsg;
      replymsg.cindex = 0;

      ifp = if_lookup_by_name (&nm->vr->ifm, msg->if_name);

      if(ifp == NULL)
      {
        SMI_FN_EXIT (nsm_smi_server_send_sync_message (ase,
                                                       SMI_MSG_STATUS,
                                                       SMI_ERROR, NULL));
      }

      SMI_SET_CTYPE (replymsg.cindex, SMI_VLAN_CTYPE_VLAN_ID);
      status = nsm_vlan_get_pvid(ifp, &(replymsg.vid));

      if(status < 0)
        msgtype = SMI_MSG_STATUS;

      ret = nsm_smi_server_send_sync_message(ase, msgtype, status, &replymsg);
    }
    break;

    case SMI_MSG_VLAN_ADD_TO_PORT:
    {
      struct nsm_if *zif = NULL;
      replymsg.cindex = 0;
      int local_status = 0;
      int vid;
      int egr_typ = NSM_VLAN_EGRESS_UNTAGGED;
      struct smi_vlan_bmp egr_vlan_bmp;

      ifp = if_lookup_by_name (&nm->vr->ifm, msg->if_name);

      if(ifp == NULL)
      {
        SMI_FN_EXIT (nsm_smi_server_send_sync_message (ase,
                                                       SMI_MSG_STATUS,
                                                       SMI_ERROR, NULL));
      }

      if (! (zif = (struct nsm_if *)ifp->info))
      {
        SMI_FN_EXIT (nsm_smi_server_send_sync_message (ase,
                                                       SMI_MSG_STATUS,
                                                       SMI_ERROR, NULL));
      }

      if (!(bridge = zif->bridge))
      {
        SMI_FN_EXIT (nsm_smi_server_send_sync_message (ase,
                                                       SMI_MSG_STATUS,
                                                       SMI_ERROR, NULL));
      }

      status = nsm_vlan_api_get_port_mode (ifp, &mode, &sub_mode);

      if ((status < 0) || (mode == NSM_VLAN_PORT_MODE_ACCESS))
      {
        SMI_FN_EXIT (nsm_smi_server_send_sync_message (ase,
                                                       SMI_MSG_STATUS,
                                                       SMI_ERROR, NULL));
      }

      SMI_SET_CTYPE (replymsg.cindex, SMI_VLAN_CTYPE_BITMAP);
      SMI_BMP_INIT (replymsg.vlan_bmp);
      SMI_BMP_INIT (egr_vlan_bmp);

      for (vid = SMI_VLAN_ID_START + 1; vid <= SMI_VLAN_ID_END; vid++)
      {
        if (!SMI_BMP_IS_MEMBER (msg->vlan_bmp, vid))
          continue;

        switch (mode)
        {
          case NSM_VLAN_PORT_MODE_HYBRID:
          {
            if (SMI_BMP_IS_MEMBER (msg->egressTypeBmp, vid))
              egr_typ = NSM_VLAN_EGRESS_TAGGED;
            else
              egr_typ = NSM_VLAN_EGRESS_UNTAGGED;

            status = nsm_vlan_add_hybrid_port (ifp, vid, egr_typ,
                                               PAL_TRUE, PAL_TRUE);
          }
          break;

          case NSM_VLAN_PORT_MODE_TRUNK:
            status = nsm_vlan_add_trunk_port (ifp, vid, PAL_TRUE, PAL_TRUE);
            egr_typ = NSM_VLAN_EGRESS_TAGGED;
            break;

          case NSM_VLAN_PORT_MODE_CN:
          case NSM_VLAN_PORT_MODE_PN:
            status = nsm_vlan_add_provider_port (ifp, vid, mode, sub_mode,
                                                 NSM_VLAN_EGRESS_TAGGED,
                                                 PAL_TRUE, PAL_TRUE);
            egr_typ = NSM_VLAN_EGRESS_TAGGED;
            break;

          default:
            break;

        }
        if (status < 0)
        {
          local_status = SMI_ERROR;
          continue;
        }
        SMI_BMP_SET(replymsg.vlan_bmp, vid);
        if (egr_typ == NSM_VLAN_EGRESS_TAGGED)
          SMI_BMP_SET(egr_vlan_bmp, vid);
      }

      ret = nsm_smi_server_send_sync_message (ase, SMI_MSG_VLAN_ADD_TO_PORT,
                                              local_status, &replymsg);

      smi_record_vlan_addtoport_alarm (msg->if_name, replymsg.vlan_bmp,
                                       egr_vlan_bmp);
    }
    break;

    case SMI_MSG_VLAN_DEL_FROM_PORT:
    {
      struct nsm_if *zif = NULL;

      replymsg.cindex = 0;
      int local_status =0;
      int vid;

      ifp = if_lookup_by_name (&nm->vr->ifm, msg->if_name);

      if(ifp == NULL)
      {
        return (nsm_smi_server_send_sync_message (ase, SMI_MSG_STATUS,
                                                  SMI_ERROR, NULL));
      }

      if (! (zif = (struct nsm_if *)ifp->info))
      {
        SMI_FN_EXIT (nsm_smi_server_send_sync_message (ase,
                                                       SMI_MSG_STATUS,
                                                       SMI_ERROR, NULL));
      }

      if (!(bridge = zif->bridge))
      {
        SMI_FN_EXIT (nsm_smi_server_send_sync_message (ase,
                                                       SMI_MSG_STATUS,
                                                       SMI_ERROR, NULL));
      }

      SMI_SET_CTYPE (replymsg.cindex, SMI_VLAN_CTYPE_BITMAP);
      SMI_BMP_INIT (replymsg.vlan_bmp);

      status = nsm_api_clear_vlan_port (bridge, ifp, msg->vlan_bmp);

      for (vid=2; vid <= 4094; vid++)
      {
        if (!SMI_BMP_IS_MEMBER (msg->vlan_bmp, vid))
          continue;

        status = nsm_vlan_delete_port(bridge, ifp, vid, PAL_TRUE);

        if (status < 0)
        {
          local_status = SMI_ERROR;
          continue;
        }
        SMI_BMP_SET(replymsg.vlan_bmp, vid);
      }

      ret = nsm_smi_server_send_sync_message(ase, SMI_MSG_VLAN_DEL_FROM_PORT,
                                             local_status, &replymsg);

      smi_record_vlan_delfromport_alarm (msg->if_name, replymsg.vlan_bmp);
    }
    break;

    case SMI_MSG_VLAN_CLEAR_PORT:
    {
      ifp = if_lookup_by_name (&nm->vr->ifm, msg->if_name);

      if(ifp == NULL)
      {
        SMI_FN_EXIT (nsm_smi_server_send_sync_message (ase,
                                                       SMI_MSG_STATUS,
                                                       SMI_ERROR, NULL));
      }

      status = nsm_api_vlan_clear_port (ifp, PAL_TRUE, PAL_TRUE);

      ret = nsm_smi_server_send_sync_message(ase, SMI_MSG_STATUS, status, NULL);
    }
    break;

    case SMI_MSG_VLAN_ADD_ALL_EXCEPT_VID:
    {
      struct nsm_vlan_bmp excludeBmp;
      struct smi_vlan_bmp egr_vlan_bmp;

      SMI_SET_CTYPE (replymsg.cindex, SMI_VLAN_CTYPE_BITMAP);
      SMI_BMP_INIT (replymsg.vlan_bmp);
      SMI_BMP_INIT (egr_vlan_bmp);

      ifp = if_lookup_by_name (&nm->vr->ifm, msg->if_name);
      if (ifp == NULL)
      {
        SMI_FN_EXIT (nsm_smi_server_send_sync_message (ase,
                                                       SMI_MSG_STATUS,
                                                       SMI_ERROR, NULL));
      }

      pal_mem_cpy(excludeBmp.bitmap, msg->vlan_bmp.bitmap,
                  sizeof(struct nsm_vlan_bmp));

      msg->submode = msg->mode;

      if (msg->vlan_add_opt == SMI_VLAN_CONFIGURED_SPECIFIC)
       {
         status = nsm_vlan_add_all_except_vid (ifp, msg->mode, msg->submode,
                                               &excludeBmp, msg->egress_type,
                                               PAL_TRUE, PAL_TRUE);
       }
      else if (msg->vlan_add_opt == SMI_VLAN_CONFIGURED_ALL)
       {
         status = nsm_vlan_api_add_all_vid (ifp, msg->mode, msg->submode,
                                            &excludeBmp, msg->egress_type,
                                             PAL_TRUE, PAL_TRUE);

       }
      else if (msg->vlan_add_opt == SMI_VLAN_CONFIGURED_NONE)
       {
         status = nsm_vlan_remove_all_except_vid (ifp, msg->mode, msg->submode,
                                                  &excludeBmp, msg->egress_type,
                                                  msg->vlan_add_opt);
       }
      else
        ret = nsm_smi_server_send_sync_message (ase, SMI_MSG_STATUS, status,
                                                NULL);

      if (status == 0)
       {
         nsm_vlan_update_vid_bmp (ifp, msg->mode, &replymsg.vlan_bmp,
                                  &egr_vlan_bmp);
       }

      ret = nsm_smi_server_send_sync_message (ase, SMI_MSG_STATUS, status,
                                              NULL);

      smi_record_vlan_addtoport_alarm (msg->if_name, replymsg.vlan_bmp,
                                       egr_vlan_bmp);

    }
    break;

    case SMI_MSG_VLAN_GET_ALL_VLAN_CONFIG:
    {
      replymsg.cindex = 0;

      SMI_SET_CTYPE (replymsg.cindex, SMI_VLAN_CTYPE_BITMAP);

      status = nsm_get_all_vlan_config (msg->br_name,
                                        &replymsg.vlan_bmp);
      if(status < 0)
        msgtype = SMI_MSG_STATUS;

      ret = nsm_smi_server_send_sync_message(ase, msgtype, status, &replymsg);
    }
    break;

    case SMI_MSG_VLAN_GET_VLAN_BY_ID:
    {
      replymsg.cindex = 0;

      SMI_SET_CTYPE (replymsg.cindex, SMI_VLAN_CTYPE_VLAN_INFO);

      status = nsm_get_vlan_by_id (msg->br_name, msg->vid,
                                   &(replymsg.vlan_info));

      if(status < 0)
        msgtype = SMI_MSG_STATUS;

      ret = nsm_smi_server_send_sync_message(ase, msgtype, status, &replymsg);
    }
    break;

    case SMI_MSG_VLAN_GET_IF:
    {
      replymsg.cindex = 0;

      ifp = if_lookup_by_name (&nm->vr->ifm, msg->if_name);
      if (ifp == NULL)
      {
        SMI_FN_EXIT (nsm_smi_server_send_sync_message (ase,
                                                       SMI_MSG_STATUS,
                                                       SMI_ERROR, NULL));
      }

      SMI_SET_CTYPE (replymsg.cindex, SMI_VLAN_CTYPE_IF_VLAN_INFO);

      status = nsm_vlan_if_get (ifp, &(replymsg.if_vlan_info));

      if(status < 0)
        msgtype = SMI_MSG_STATUS;

      ret = nsm_smi_server_send_sync_message(ase, msgtype, status, &replymsg);
    }
    break;

    case SMI_MSG_VLAN_GET_BRIDGE:
    {
      replymsg.cindex = 0;

      SMI_SET_CTYPE (replymsg.cindex, SMI_VLAN_CTYPE_BR_INFO);

      status = nsm_get_bridge_config (msg->br_name, &(replymsg.bridge_info));

      if(status < 0)
        msgtype = SMI_MSG_STATUS;

      ret = nsm_smi_server_send_sync_message(ase, msgtype, status, &replymsg);
    }
    break;

    case SMI_MSG_VLAN_SET_PORT_PROTO_PROCESS:
    {
      ifp = if_lookup_by_name (&nm->vr->ifm, msg->if_name);
      if(ifp == NULL)
      {
        SMI_FN_EXIT (nsm_smi_server_send_sync_message (ase,
                                                       SMI_MSG_STATUS,
                                                       SMI_ERROR, NULL));
      }

#ifdef HAVE_PROVIDER_BRIDGE

      status = nsm_bridge_api_port_proto_process (ifp, msg->protocol,
                                                  msg->process, PAL_FALSE,
                                                  NSM_VLAN_NULL_VID);
#endif /* HAVE_PROVIDER_BRIDGE */

      ret = nsm_smi_server_send_sync_message(ase, SMI_MSG_STATUS, status, NULL);
    }
    break;

    case SMI_MSG_FORCE_DEFAULT_VLAN:
    {
      ifp = if_lookup_by_name (&nm->vr->ifm, msg->if_name);
      if (ifp == NULL)
      {
        SMI_FN_EXIT (nsm_smi_server_send_sync_message (ase,
                                                       SMI_MSG_STATUS,
                                                       SMI_ERROR, NULL));
      }

      status = nsm_force_default_vlan (ifp, msg->vid);
      ret = nsm_smi_server_send_sync_message (ase, SMI_MSG_STATUS, status,
                                              NULL);
    }
    break;

    case SMI_MSG_SET_PRESERVE_CE_COS:
    {
      ifp = if_lookup_by_name (&nm->vr->ifm, msg->if_name);
      if (ifp == NULL)
      {
        SMI_FN_EXIT (nsm_smi_server_send_sync_message (ase,
                                                       SMI_MSG_STATUS,
                                                       SMI_ERROR, NULL));
      }

      status = nsm_set_preserve_ce_cos (ifp);

      ret = nsm_smi_server_send_sync_message (ase, SMI_MSG_STATUS, status,
                                              NULL);
    }
    break;

    case SMI_MSG_SET_PORT_BASED_VLAN:
    {
      struct smi_port_bmp pbmp;

      ifp = if_lookup_by_name (&nm->vr->ifm, msg->if_name);
      if (ifp == NULL)
      {
        SMI_FN_EXIT (nsm_smi_server_send_sync_message (ase,
                                                       SMI_MSG_STATUS,
                                                       SMI_ERROR, NULL));
      }
      pal_mem_cpy(pbmp.bitmap, msg->port_list.bitmap,
                                            sizeof(struct smi_port_bmp));
      status =  nsm_set_portbased_vlan (ifp, pbmp);

      ret = nsm_smi_server_send_sync_message (ase, SMI_MSG_STATUS, status,
                                              NULL);
    }
    break;

    case SMI_MSG_SET_CPUPORT_DEFAULT_VLAN:
    {

      ifp = if_lookup_by_name (&nm->vr->ifm, SMI_CPU_PORT);

      if (ifp == NULL)
      {
        SMI_FN_EXIT (nsm_smi_server_send_sync_message (ase,
                                                       SMI_MSG_STATUS,
                                                       SMI_ERROR, NULL));
      }
      status = nsm_set_cpuport_default_vlan (ifp, msg->vid);

      ret = nsm_smi_server_send_sync_message (ase, SMI_MSG_STATUS, status,
                                                                 NULL);

    }
    break;

    case SMI_MSG_SET_WAYSIDEPORT_DEFAULT_VLAN:
    {


      ifp = if_lookup_by_name (&nm->vr->ifm, SMI_WAYSIDE_PORT);

      if (ifp == NULL)
      {
        SMI_FN_EXIT (nsm_smi_server_send_sync_message (ase,
                                                       SMI_MSG_STATUS,
                                                       SMI_ERROR, NULL));
      }
      status = nsm_set_waysideport_default_vlan (ifp, msg->vid);

      ret = nsm_smi_server_send_sync_message (ase, SMI_MSG_STATUS, status,
                                                                 NULL);

    }
    break;

    case SMI_MSG_SET_CPUPORT_BASED_VLAN:
    {
      struct smi_port_bmp pbmp;

      ifp = if_lookup_by_name (&nm->vr->ifm, SMI_CPU_PORT);

      if (ifp == NULL)
      {
        SMI_FN_EXIT (nsm_smi_server_send_sync_message (ase,
                                                       SMI_MSG_STATUS,
                                                       SMI_ERROR, NULL));
      }
      pal_mem_cpy(pbmp.bitmap, msg->port_list.bitmap,
                                             sizeof(struct smi_port_bmp));
      status = nsm_set_portbased_vlan (ifp, pbmp);
      ret = nsm_smi_server_send_sync_message (ase, SMI_MSG_STATUS, status,
                                                               NULL);
    }
    break;

    case SMI_MSG_SVLAN_SET_PORT_ETHER_TYPE:
    {
      ifp = if_lookup_by_name (&nm->vr->ifm, msg->if_name);
      if (ifp == NULL)
      {
        SMI_FN_EXIT (nsm_smi_server_send_sync_message (ase,
                                                       SMI_MSG_STATUS,
                                                       SMI_ERROR, NULL));
      }

      status = nsm_svlan_set_port_ether_type (ifp, msg->ether_type);

      ret = nsm_smi_server_send_sync_message (ase, SMI_MSG_STATUS, status,
                                              NULL);
    }
    break;

    case SMI_MSG_VLAN_GET_PORT_PROTO_PROCESS:
    {
      ifp = if_lookup_by_name (&nm->vr->ifm, msg->if_name);
      if(ifp == NULL)
      {
        SMI_FN_EXIT (nsm_smi_server_send_sync_message (ase,
                                                       SMI_MSG_STATUS,
                                                       SMI_ERROR, NULL));
      }

      /* Process TLV */
      SMI_SET_CTYPE (replymsg.cindex, SMI_VLAN_CTYPE_BR_PROTO_PROCESS);

#ifdef HAVE_PROVIDER_BRIDGE

      status = nsm_bridge_api_get_port_proto_process (ifp, msg->protocol,
                                                      &(replymsg.process));
#endif /* HAVE_PROVIDER_BRIDGE */
      if (status < 0)
        msgtype = SMI_MSG_STATUS;

      ret = nsm_smi_server_send_sync_message (ase, msgtype, status, &replymsg);
    }
    break;
    case SMI_MSG_NSM_HA_SWITCH:
#ifdef HAVE_HA
      status = fm_redun_state_set(HA_FM_PTR_GET(nzg), msg->switch_to_state);
      if (status != FM_RC_OK)
        status = SMI_NSM_SWITCH_ERR;
#else /*HAVE_HA*/
     status = SMI_NSM_SWITCH_ERR;
#endif /*HAVE_HA*/
      ret = nsm_smi_server_send_sync_message (ase, SMI_MSG_STATUS, status,
                                               NULL);
    break;

    default:
      break;
  }
  SMI_FN_EXIT(ret);
}

/* Set/Get GvRP params */
int
nsm_smi_server_recv_gvrp (struct smi_msg_header *header,
                          void *arg, void *message)
{
  struct smi_server_entry *ase;
  struct smi_server *as;
  struct smi_msg_gvrp *msg = (struct smi_msg_gvrp *)message;
  struct nsm_master *nm = nsm_master_lookup_by_id (nzg, SMI_DFLT_VRID);
  struct nsm_bridge_master *master = nsm_bridge_get_master (nm);
  struct smi_msg_gvrp replymsg;
  struct interface *ifp = NULL;
  int status = 0, ret = 0;
  int msgtype = header->type;

  SMI_FN_ENTER ();

  ase = arg;
  as = ase->as;

  replymsg.cindex = 0;

  if(IS_NSM_DEBUG_RECV)
    smi_gvrp_dump(NSM_ZG, msg);

  #ifdef HAVE_HA
  if ((HA_IS_STANDBY (nzg)))
  {
    ret = nsm_check_func_type (msgtype);
    if (ret == SMI_SET_FUNC)
      SMI_FN_EXIT (nsm_smi_server_send_sync_message (ase,
                                                     SMI_MSG_STATUS,
                                                     SMI_ERROR, NULL));
  }
  #endif /* HAVE_HA */

  switch(header->type)
  {
    case SMI_MSG_GVRP_SET_TIMER:
      ifp = if_lookup_by_name (&nm->vr->ifm, msg->if_name);
      if (ifp == NULL)
      {
        SMI_FN_EXIT (nsm_smi_server_send_sync_message (ase,
                                                       SMI_MSG_STATUS,
                                                       SMI_ERROR, NULL));
      }

      status = gvrp_set_timer (master, ifp, msg->timer_type, msg->timer_value);

      ret = nsm_smi_server_send_sync_message (ase, SMI_MSG_STATUS, status,
                                              NULL);
    break;

    case SMI_MSG_GVRP_GET_TIMER:
      ifp = if_lookup_by_name (&nm->vr->ifm, msg->if_name);
      if (ifp == NULL)
      {
        SMI_FN_EXIT (nsm_smi_server_send_sync_message (ase,
                                                       SMI_MSG_STATUS,
                                                       SMI_ERROR, NULL));
      }

      SMI_SET_CTYPE (replymsg.cindex, SMI_GVRP_CTYPE_TIMER_VALUE);

      status = gvrp_get_timer (master, ifp, msg->timer_type,
                              &(replymsg.timer_value));

      if(status <= 0)
        msgtype = SMI_MSG_STATUS;

      ret = nsm_smi_server_send_sync_message (ase, msgtype, status,
                                              &replymsg);
    break;

    case SMI_MSG_GVRP_ENABLE:
      status = gvrp_enable (master, msg->reg_type, msg->bridge_name);

      if(status <= 0)
        msgtype = SMI_MSG_STATUS;

      ret = nsm_smi_server_send_sync_message (ase, msgtype, status,
                                              NULL);
    break;

    case SMI_MSG_GVRP_DISABLE:
      status = gvrp_disable (master, msg->bridge_name);

      if(status <= 0)
        msgtype = SMI_MSG_STATUS;

      ret = nsm_smi_server_send_sync_message (ase, msgtype, status,
                                              NULL);
    break;

    case SMI_MSG_GVRP_ENABLE_PORT:
      ifp = if_lookup_by_name (&nm->vr->ifm, msg->if_name);
      if (ifp == NULL)
      {
        SMI_FN_EXIT (nsm_smi_server_send_sync_message (ase,
                                                       SMI_MSG_STATUS,
                                                       SMI_ERROR, NULL));
      }

      status = gvrp_enable_port (master, ifp);

      if(status <= 0)
        msgtype = SMI_MSG_STATUS;

      ret = nsm_smi_server_send_sync_message (ase, msgtype, status,
                                              NULL);
    break;

    case SMI_MSG_GVRP_DISABLE_PORT:
      ifp = if_lookup_by_name (&nm->vr->ifm, msg->if_name);
      if (ifp == NULL)
      {
        SMI_FN_EXIT (nsm_smi_server_send_sync_message (ase,
                                                       SMI_MSG_STATUS,
                                                       SMI_ERROR, NULL));
      }

      status = gvrp_disable_port (master, ifp);

      if(status <= 0)
        msgtype = SMI_MSG_STATUS;

      ret = nsm_smi_server_send_sync_message (ase, msgtype, status,
                                              NULL);
    break;

    case SMI_MSG_GVRP_SET_REG_MODE:
      ifp = if_lookup_by_name (&nm->vr->ifm, msg->if_name);
      if (ifp == NULL)
      {
        SMI_FN_EXIT (nsm_smi_server_send_sync_message (ase,
                                                       SMI_MSG_STATUS,
                                                       SMI_ERROR, NULL));
      }

      status = gvrp_set_registration (master, ifp, msg->reg_mode);

      ret = nsm_smi_server_send_sync_message (ase, SMI_MSG_STATUS, status,
                                              NULL);
    break;

    case SMI_MSG_GVRP_GET_REG_MODE:
      ifp = if_lookup_by_name (&nm->vr->ifm, msg->if_name);
      if (ifp == NULL)
      {
        SMI_FN_EXIT (nsm_smi_server_send_sync_message (ase,
                                                       SMI_MSG_STATUS,
                                                       SMI_ERROR, NULL));
      }

      SMI_SET_CTYPE (replymsg.cindex, SMI_GVRP_CTYPE_REG_MODE);

      status = gvrp_get_registration (master, ifp, &(replymsg.reg_mode));

      if(status < 0)
        msgtype = SMI_MSG_STATUS;

      ret = nsm_smi_server_send_sync_message (ase, msgtype, status,
                                              &replymsg);
    break;

    case SMI_MSG_GVRP_GET_PER_VLAN_STATS:
    {
      SMI_SET_CTYPE (replymsg.cindex, SMI_GVRP_CTYPE_RECV_COUNTERS);
      SMI_SET_CTYPE (replymsg.cindex, SMI_GVRP_CTYPE_XMIT_COUNTERS);

      status = gvrp_get_per_vlan_statistics_details (master,
                                                 msg->bridge_name, msg->vid,
                                                 &(replymsg.receive_counters),
                                                 &(replymsg.transmit_counters));

      if(status < 0)
        msgtype = SMI_MSG_STATUS;

      ret = nsm_smi_server_send_sync_message (ase, msgtype, status,
                                              &replymsg);
    }
    break;

    case SMI_MSG_GVRP_CLEAR_ALL_STATS:
      status = gvrp_api_clear_all_statistics (master);

      ret = nsm_smi_server_send_sync_message (ase, SMI_MSG_STATUS, status,
                                              NULL);
    break;

    case SMI_MSG_GVRP_SET_DYNAMIC_VLAN_LEARNING:
      status = gvrp_dynamic_vlan_learning_set (master, msg->bridge_name,
                                               msg->dyn_vlan_learning);

      ret = nsm_smi_server_send_sync_message (ase, SMI_MSG_STATUS, status,
                                              NULL);
    break;

    case SMI_MSG_GVRP_GET_BRIDGE_CONFIG:
    {
      SMI_SET_CTYPE (replymsg.cindex, SMI_GVRP_CTYPE_BRIDGE_CONFIG);

      status = gvrp_get_configuration_bridge (master, msg->bridge_name,
                                              msg->reg_type,
                                              &(replymsg.gvrp_br_config));

      if(status < 0)
        msgtype = SMI_MSG_STATUS;

      ret = nsm_smi_server_send_sync_message (ase, msgtype, status,
                                              &replymsg);
    }
    break;

    case SMI_MSG_GVRP_GET_VID_DETAILS:
    {
      ifp = if_lookup_by_name (&nm->vr->ifm, msg->if_name);
      if (ifp == NULL)
      {
        SMI_FN_EXIT (nsm_smi_server_send_sync_message (ase,
                                                       SMI_MSG_STATUS,
                                                       SMI_ERROR, NULL));
      }

      SMI_SET_CTYPE (replymsg.cindex, SMI_GVRP_CTYPE_VID_DETAILS);

      status = gvrp_get_vid_details (master, ifp, msg->first_call,
                                      msg->gid_index,
                                      &(replymsg.gvrp_vid_detail));
      if(status < 0)
        msgtype = SMI_MSG_STATUS;

      ret = nsm_smi_server_send_sync_message (ase, msgtype, status,
                                              &replymsg);
    }
    break;

    case SMI_MSG_GVRP_GET_PORT_STATS:
    {
      ifp = if_lookup_by_name (&nm->vr->ifm, msg->if_name);
      if (ifp == NULL)
      {
        SMI_FN_EXIT (nsm_smi_server_send_sync_message (ase,
                                                       SMI_MSG_STATUS,
                                                       SMI_ERROR, NULL));
      }

      SMI_SET_CTYPE (replymsg.cindex, SMI_GVRP_CTYPE_PORT_STATS);

      status = gvrp_get_port_statistics (master, ifp,
                                         &(replymsg.gvrp_stats));
      if(status < 0)
        msgtype = SMI_MSG_STATUS;

      ret = nsm_smi_server_send_sync_message (ase, msgtype, status,
                                              &replymsg);
    }
    break;

    case SMI_MSG_GVRP_GET_TIMER_DETAILS:
      ifp = if_lookup_by_name (&nm->vr->ifm, msg->if_name);
      if (ifp == NULL)
      {
        SMI_FN_EXIT (nsm_smi_server_send_sync_message (ase,
                                                       SMI_MSG_STATUS,
                                                       SMI_ERROR, NULL));
      }

      SMI_SET_CTYPE (replymsg.cindex, SMI_GVRP_CTYPE_TIMER_DETAILS);

      status = gvrp_get_timer_details (master, ifp,
                        (pal_time_t *) &(replymsg.timer_details));

      if(status <= 0)
        msgtype = SMI_MSG_STATUS;

      ret = nsm_smi_server_send_sync_message (ase, msgtype, status,
                                              &replymsg);
    break;

    default:
    break;
  }
  SMI_FN_EXIT(ret);
}

/* QOS params */
int
nsm_smi_server_recv_qos (struct smi_msg_header *header,
                          void *arg, void *message)
{
  struct smi_server_entry *ase;
  struct smi_server *as;
  int status = 0, ret = 0;
  struct smi_msg_qos *msg = (struct smi_msg_qos *)message;
  struct smi_msg_qos replymsg;
  struct nsm_master *nm = nsm_master_lookup_by_id (nzg, SMI_DFLT_VRID);
  struct interface *ifp = NULL;
  int msgtype = header->type;

  SMI_FN_ENTER ();

  ase = arg;
  as = ase->as;

  replymsg.cindex = 0;
  replymsg.extended_cindex_1 = 0;

  if(IS_NSM_DEBUG_RECV)
    smi_qos_dump (NSM_ZG, msg);

  if (nm == NULL)
  {
    SMI_FN_EXIT (nsm_smi_server_send_sync_message (ase,
                                                   SMI_MSG_STATUS,
                                                   SMI_ERROR, NULL));
  }

  /* Check QoS state */
  if(msgtype != SMI_MSG_QOS_GLOBAL_ENABLE &&
     msgtype != SMI_MSG_QOS_GET_GLOBAL_STATUS)
  {
    if (nsm_qos_chk_if_qos_enabled (nm->pmap))
    {
      SMI_FN_EXIT (nsm_smi_server_send_sync_message (ase,
                                                     SMI_MSG_STATUS,
                                                     SMI_ERROR_QOS_DISABLED, NULL));
    }
  }
  #ifdef HAVE_HA
  if ((HA_IS_STANDBY (nzg)))
  {
    ret = nsm_check_func_type (msgtype);
    if (ret == SMI_SET_FUNC)
      SMI_FN_EXIT (nsm_smi_server_send_sync_message (ase,
                                                     SMI_MSG_STATUS,
                                                     SMI_ERROR, NULL));
  }
  #endif /* HAVE_HA */


  switch(header->type)
  {
    case SMI_MSG_QOS_GLOBAL_ENABLE:
    {
      status = nsm_qos_global_enable (nm);
      ret = nsm_smi_server_send_sync_message (ase, SMI_MSG_STATUS,
                                              status, NULL);
    }
    break;
    case SMI_MSG_QOS_GLOBAL_DISABLE:
    {
      status = nsm_qos_global_disable (nm);
      ret = nsm_smi_server_send_sync_message (ase, SMI_MSG_STATUS,
                                              status, NULL);
    }
    break;
    case SMI_MSG_QOS_GET_GLOBAL_STATUS:
    {
      SMI_SET_CTYPE (replymsg.cindex, SMI_QOS_CTYPE_STATUS);

      status = nsm_qos_get_global_status (&(replymsg.global_stat));
      if(status < 0)
        msgtype = SMI_MSG_STATUS;

      ret = nsm_smi_server_send_sync_message (ase, msgtype, status,
                                              &replymsg);
    }
    break;
    case SMI_MSG_QOS_SET_PMAP_NAME:
    {
      status = nsm_qos_set_pmap_name(nm, msg->pmap_name);
      ret = nsm_smi_server_send_sync_message (ase, SMI_MSG_STATUS,
                                              status, NULL);
    }
    break;
    case SMI_MSG_QOS_GET_POLICY_MAP_NAMES:
    {

      SMI_SET_CTYPE (replymsg.cindex, SMI_QOS_CTYPE_PMAPNAMES);

      status = nsm_qos_get_policy_map_names (nm, replymsg.policy_map_name,
                                             msg->first_call, msg->pmap_name);
      if(status < 0)
        msgtype = SMI_MSG_STATUS;

      ret = nsm_smi_server_send_sync_message (ase, msgtype, status,
                                              &replymsg);
    }
    break;
    case SMI_MSG_QOS_GET_POLICY_MAP:
    {
      SMI_SET_CTYPE (replymsg.cindex, SMI_QOS_CTYPE_POLICYMAP);

      status = nsm_qos_get_policy_map_info (nm, msg->pmap_name,
                                            &(replymsg.smi_qos_pmap));
      if(status < 0)
        msgtype = SMI_MSG_STATUS;

      ret = nsm_smi_server_send_sync_message (ase, msgtype, status,
                                              &replymsg);
    }
    break;
    case SMI_MSG_QOS_PMAP_DELETE:
    {
      status = nsm_qos_delete_pmap (nm, msg->pmap_name);
      ret = nsm_smi_server_send_sync_message (ase, SMI_MSG_STATUS,
                                              status, NULL);
    }
    break;
    case SMI_MSG_QOS_SET_CMAP_NAME:
    {
      status = nsm_qos_set_cmap_name(nm, msg->cmap_name);
      ret = nsm_smi_server_send_sync_message (ase, SMI_MSG_STATUS,
                                              status, NULL);
    }
    break;
    case SMI_MSG_QOS_GET_CMAP_NAME:
    {
      SMI_SET_CTYPE (replymsg.cindex, SMI_QOS_CTYPE_CLASSMAP);

      status = nsm_qos_get_cmap_info (nm, msg->cmap_name,
                                      &(replymsg.smi_qos_cmap));
      if(status < 0)
        msgtype = SMI_MSG_STATUS;

      ret = nsm_smi_server_send_sync_message (ase, msgtype, status,
                                              &replymsg);
    }
    break;
    case SMI_MSG_QOS_DELETE_CMAP_NAME:
    {
      status = nsm_qos_delete_cmap (nm, msg->cmap_name);
      ret = nsm_smi_server_send_sync_message (ase, SMI_MSG_STATUS,
                                              status, NULL);
    }
    break;
    case SMI_MSG_QOS_CMAP_MATCH_TRAFFIC_SET:
    {
      status = nsm_qos_cmap_match_traffic_set(nm, msg->cmap_name,
                                              msg->traffic_type,
                                              msg->match_mode);
      ret = nsm_smi_server_send_sync_message (ase, SMI_MSG_STATUS,
                                              status, NULL);
    }
    break;
    case SMI_MSG_QOS_CMAP_MATCH_TRAFFIC_GET:
    {
      SMI_SET_CTYPE (replymsg.cindex, SMI_QOS_CTYPE_TRAFFICTYPE);
      SMI_SET_CTYPE (replymsg.cindex, SMI_QOS_CTYPE_MATCHMODE);

      replymsg.match_mode = msg->match_mode;
      status = nsm_qos_cmap_match_traffic_get (nm, msg->cmap_name,
                                              &(replymsg.match_mode),
                                              &(replymsg.traffic_type));
      if(status < 0)
        msgtype = SMI_MSG_STATUS;

      ret = nsm_smi_server_send_sync_message (ase, msgtype,
                                              status, &replymsg);
    }
    break;
    case SMI_MSG_QOS_CMAP_MATCH_TRAFFIC_UNSET:
    {
      status = nsm_qos_cmap_match_traffic_unset(nm, msg->cmap_name,
                                                msg->traffic_type);
      ret = nsm_smi_server_send_sync_message (ase, SMI_MSG_STATUS,
                                              status, NULL);
    }
    break;
    case SMI_MSG_QOS_PMAPC_POLICE:
    {
      status = nsm_qos_pmapc_set_police(nm, msg->pmap_name, msg->cmap_name,
                                        msg->rate_value,
                                        msg->commit_burst_size,
                                        msg->excess_burst_size,
                                        msg->exceed_action,
                                        msg->fc_mode);

      ret = nsm_smi_server_send_sync_message (ase, SMI_MSG_STATUS,
                                              status, NULL);
    }
    break;
    case SMI_MSG_QOS_PMAPC_POLICE_GET:
    {
      SMI_SET_CTYPE(replymsg.cindex, SMI_QOS_CTYPE_CMAPNAME);
      SMI_SET_CTYPE(replymsg.cindex, SMI_QOS_CTYPE_PMAPNAME);
      SMI_SET_CTYPE(replymsg.cindex, SMI_QOS_CTYPE_RATE_VALUE);
      SMI_SET_CTYPE(replymsg.cindex, SMI_QOS_CTYPE_ACTION);
      SMI_SET_CTYPE(replymsg.cindex, SMI_QOS_CTYPE_FCMODE);
      SMI_SET_CTYPE(replymsg.cindex, SMI_QOS_CTYPE_COMMITB);
      SMI_SET_CTYPE(replymsg.cindex, SMI_QOS_CTYPE_EXCESSB);
      
      status = nsm_qos_pmapc_get_police(nm, msg->pmap_name, msg->cmap_name,
                                        &replymsg.rate_value,
                                        &replymsg.commit_burst_size,
                                        &replymsg.excess_burst_size,
                                        &replymsg.exceed_action,
                                        &replymsg.fc_mode);

      if(status < 0)
        msgtype = SMI_MSG_STATUS;

      pal_mem_cpy (replymsg.pmap_name, msg->pmap_name, SMI_QOS_POLICY_LEN);
      pal_mem_cpy (replymsg.cmap_name, msg->cmap_name, SMI_QOS_POLICY_LEN); 
      
      ret = nsm_smi_server_send_sync_message (ase, msgtype,
                                              status, &replymsg);
    }
    break;
    case SMI_MSG_QOS_PMAPC_DELETE_CMAP:
    {
      status = nsm_qos_pmapc_delete_cmap (nm, msg->pmap_name, msg->cmap_name);
                   
      ret = nsm_smi_server_send_sync_message (ase, SMI_MSG_STATUS,
                                              status, NULL);
    }
    break;
    case SMI_MSG_QOS_PMAPC_POLICE_DELETE:
    {
      status = nsm_qos_pmapc_delete_police(nm, msg->pmap_name, msg->cmap_name);

      ret = nsm_smi_server_send_sync_message (ase, SMI_MSG_STATUS,
                                              status, NULL);
    }
    break;
    case SMI_MSG_QOS_SET_PORT_SCHEDULING:
    {
      ifp = if_lookup_by_name (&nm->vr->ifm, msg->if_name);
      if (ifp == NULL)
      {
        SMI_FN_EXIT (nsm_smi_server_send_sync_message (ase, SMI_MSG_STATUS,
                                                       SMI_ERROR, NULL));
      }

      status = nsm_qos_set_strict_queue (&qosg, ifp, msg->port_sched_mode);

      ret = nsm_smi_server_send_sync_message (ase, SMI_MSG_STATUS,
                                              status, NULL);
    }
    break;
    case SMI_MSG_QOS_GET_PORT_SCHEDULING:
    {
      ifp = if_lookup_by_name (&nm->vr->ifm, msg->if_name);
      if (ifp == NULL)
      {
        SMI_FN_EXIT (nsm_smi_server_send_sync_message (ase, SMI_MSG_STATUS,
                                                       SMI_ERROR, NULL));
      }

      SMI_SET_CTYPE (replymsg.cindex, SMI_QOS_CTYPE_PORTSCHED);

      status = nsm_qos_get_strict_queue (&qosg, ifp,
                                         &(replymsg.port_sched_mode));

      if(status < 0)
        msgtype = SMI_MSG_STATUS;

      ret = nsm_smi_server_send_sync_message (ase, msgtype, status,
                                              &replymsg);
    }
    break;
    case SMI_MSG_QOS_SET_DEFAULT_USER_PRIORITY:
    {
      ifp = if_lookup_by_name (&nm->vr->ifm, msg->if_name);
      if (ifp == NULL)
      {
        SMI_FN_EXIT (nsm_smi_server_send_sync_message (ase, SMI_MSG_STATUS,
                                                       SMI_ERROR, NULL));
      }

      status = nsm_vlan_port_set_default_user_priority(ifp,
                                                       msg->user_priority);
      ret = nsm_smi_server_send_sync_message (ase, SMI_MSG_STATUS,
                                              status, NULL);
    }
    break;
    case SMI_MSG_QOS_GET_DEFAULT_USER_PRIORITY:
    {
      ifp = if_lookup_by_name (&nm->vr->ifm, msg->if_name);
      if (ifp == NULL)
      {
        SMI_FN_EXIT (nsm_smi_server_send_sync_message (ase, SMI_MSG_STATUS,
                                                       SMI_ERROR, NULL));
      }

      SMI_SET_CTYPE (replymsg.cindex, SMI_QOS_CTYPE_PRIORITY);

      status = nsm_vlan_port_get_default_user_priority (ifp);
      if(status < 0)
      {
        status = SMI_ERROR;
        msgtype = SMI_MSG_STATUS;
      }
      else
        replymsg.user_priority = status;

      ret = nsm_smi_server_send_sync_message (ase, msgtype, status,
                                              &replymsg);
    }
    break;
    case SMI_MSG_QOS_PORT_SET_REGEN_USER_PRIORITY:
    {
      ifp = if_lookup_by_name (&nm->vr->ifm, msg->if_name);
      if (ifp == NULL)
      {
        SMI_FN_EXIT (nsm_smi_server_send_sync_message (ase, SMI_MSG_STATUS,
                                                       SMI_ERROR, NULL));
      }

      status = nsm_vlan_port_set_regen_user_priority(ifp, msg->user_priority,
                                                     msg->regen_user_priority);
      ret = nsm_smi_server_send_sync_message (ase, SMI_MSG_STATUS,
                                              status, NULL);
    }
    break;
    case SMI_MSG_QOS_PORT_GET_REGEN_USER_PRIORITY:
    {
      ifp = if_lookup_by_name (&nm->vr->ifm, msg->if_name);
      if (ifp == NULL)
      {
        SMI_FN_EXIT (nsm_smi_server_send_sync_message (ase, SMI_MSG_STATUS,
                                                       SMI_ERROR, NULL));
      }

      SMI_SET_CTYPE (replymsg.cindex, SMI_QOS_CTYPE_REGENPRIORITY);

      replymsg.regen_user_priority =
                nsm_vlan_port_get_regen_user_priority(ifp, msg->user_priority);

      if(replymsg.regen_user_priority < 0)
      {
        status = SMI_ERROR;
        msgtype = SMI_MSG_STATUS;
      }
      ret = nsm_smi_server_send_sync_message (ase, msgtype, status,
                                              &replymsg);
    }
    break;
    case SMI_MSG_QOS_GLOBAL_COS_TO_QUEUE:
    {
      status = nsm_mls_qos_set_cos_to_queue(&qosg, msg->cos, msg->queue_id);
      ret = nsm_smi_server_send_sync_message (ase, SMI_MSG_STATUS,
                                              status, NULL);
    }
    break;
    case SMI_MSG_QOS_GLOBAL_DSCP_TO_QUEUE:
    {
      status = nsm_mls_qos_set_dscp_to_queue (&qosg, msg->dscp, msg->queue_id);
      ret = nsm_smi_server_send_sync_message (ase, SMI_MSG_STATUS,
                                              status, NULL);
    }
    break;
    case SMI_MSG_QOS_SET_TRUST_STATE:
    {
      u_int32_t t_state = SMI_QOS_TRUST_NONE;

      ifp = if_lookup_by_name (&nm->vr->ifm, msg->if_name);
      if (ifp == NULL)
      {
        SMI_FN_EXIT (nsm_smi_server_send_sync_message (ase, SMI_MSG_STATUS,
                                                       SMI_ERROR, NULL));
      }

       switch (msg->trust_state)
       {
         case SMI_QOS_TRUST_NONE: 
           t_state = NSM_QOS_TRUST_NONE;
         break;
         case SMI_QOS_TRUST_8021P: 
           t_state = NSM_QOS_TRUST_COS;
         break;
         case SMI_QOS_TRUST_DSCP: 
           t_state = NSM_QOS_TRUST_DSCP;
         break;
         case SMI_QOS_TRUST_BOTH: 
           t_state = NSM_QOS_TRUST_DSCP_COS;
         break;
         default:
         break;
       }
 
      status = nsm_qos_set_trust_state (&qosg, ifp, t_state);
      ret = nsm_smi_server_send_sync_message (ase, SMI_MSG_STATUS,
                                              status, NULL);
    }
    break;
    case SMI_MSG_QOS_SET_FORCE_TRUST_COS:
    {
      ifp = if_lookup_by_name (&nm->vr->ifm, msg->if_name);
      if (ifp == NULL)
      {
        SMI_FN_EXIT (nsm_smi_server_send_sync_message (ase, SMI_MSG_STATUS,
                                                       SMI_ERROR, NULL));
      }

      status = nsm_qos_set_force_trust_cos (&qosg, ifp, msg->force_trust_cos);
      ret = nsm_smi_server_send_sync_message (ase, SMI_MSG_STATUS,
                                              status, NULL);
    }
    break;
    case SMI_MSG_QOS_SET_FRAME_TYPE_PRIORITY_OVERRIDE:
    {
      status = nsm_qos_set_frame_type_priority_override (&qosg, msg->frame_type,
                                                          msg->queue_id);
      ret = nsm_smi_server_send_sync_message (ase, SMI_MSG_STATUS,
                                              status, NULL);
    }
    break;

#ifdef HAVE_VLAN
    case SMI_MSG_QOS_SET_VLAN_PRIORITY:
    {
      status = nsm_mls_qos_vlan_priority_set (nm, msg->bridge_name, msg->vid,
                                              msg->priority);
      ret = nsm_smi_server_send_sync_message (ase, SMI_MSG_STATUS,
                                              status, NULL);
    }
    break;
    case SMI_MSG_QOS_GET_VLAN_PRIORITY:
    {
      SMI_SET_CTYPE(replymsg.cindex, SMI_QOS_CTYPE_VLANPRIORITY);

      status = nsm_mls_qos_vlan_priority_get (nm, msg->bridge_name, msg->vid,
                                              &replymsg.priority);
      if (status < 0)
        msgtype = SMI_MSG_STATUS;

      ret = nsm_smi_server_send_sync_message (ase, msgtype,
                                              status, &replymsg);
    }
    break;
    case SMI_MSG_QOS_UNSET_VLAN_PRIORITY:
    {
      status = nsm_mls_qos_vlan_priority_unset (nm, msg->bridge_name, msg->vid);
      ret = nsm_smi_server_send_sync_message (ase, SMI_MSG_STATUS,
                                              status, NULL);
    }
    break;
    case SMI_MSG_QOS_SET_PORT_VLAN_PRIORITY:
    {
      ifp = if_lookup_by_name (&nm->vr->ifm, msg->if_name);
      if (ifp == NULL)
      {
        SMI_FN_EXIT (nsm_smi_server_send_sync_message (ase, SMI_MSG_STATUS,
                                                       SMI_ERROR, NULL));
      }

      status = nsm_qos_set_port_vlan_priority_override (&qosg, ifp,
                                                        msg->vlan_port_mode);
      ret = nsm_smi_server_send_sync_message (ase, SMI_MSG_STATUS,
                                              status, NULL);
    }
    break;
#endif /* HAVE_VLAN */
    case SMI_MSG_QOS_SET_PORT_DA_PRIORITY:
    {
      ifp = if_lookup_by_name (&nm->vr->ifm, msg->if_name);
      if (ifp == NULL)
      {
        SMI_FN_EXIT (nsm_smi_server_send_sync_message (ase, SMI_MSG_STATUS,
                                                       SMI_ERROR, NULL));
      }

      status = nsm_qos_set_port_da_priority_override (&qosg, ifp,
                                                      msg->da_port_mode);

      ret = nsm_smi_server_send_sync_message (ase, SMI_MSG_STATUS,
                                              status, NULL);
    }
    break;

    case SMI_MSG_QOS_SET_QUEUE_WEIGHT:
    {
      status = nsm_mls_qos_set_queue_weight(&qosg, msg->queue_id, msg->weight);
      ret = nsm_smi_server_send_sync_message (ase, SMI_MSG_STATUS,
                                              status, NULL);
    }
    break;

    case SMI_MSG_QOS_GET_QUEUE_WEIGHT:
    {
      /* Extended ctype TLV */
      SMI_SET_CTYPE (replymsg.cindex, SMI_QOS_CTYPE_EXTENDED);
      /* Weight array TLV */
      SMI_SET_CTYPE(replymsg.extended_cindex_1, SMI_QOS_CTYPE_QWEIGHT_ARRAY);

      status = nsm_mls_qos_get_queue_weight (&qosg, replymsg.qweights);
      if(status < 0)
        msgtype = SMI_MSG_STATUS;

      ret = nsm_smi_server_send_sync_message (ase, msgtype,
                                              status, &replymsg);
    }
    break;

    case SMI_MSG_QOS_SET_PORT_SERVICE_POLICY:
    {
      ifp = if_lookup_by_name (&nm->vr->ifm, msg->if_name);
      if (ifp == NULL)
      {
        SMI_FN_EXIT (nsm_smi_server_send_sync_message (ase, SMI_MSG_STATUS,
                                                       SMI_ERROR, NULL));
      }

      status = nsm_qos_set_port_service_policy (nm, ifp, msg->pmap_name);
      ret = nsm_smi_server_send_sync_message (ase, SMI_MSG_STATUS,
                                              status, NULL);
    }
    break;

    case SMI_MSG_QOS_UNSET_PORT_SERVICE_POLICY:
    {
      ifp = if_lookup_by_name (&nm->vr->ifm, msg->if_name);
      if (ifp == NULL)
      {
        SMI_FN_EXIT (nsm_smi_server_send_sync_message (ase, SMI_MSG_STATUS,
                                                       SMI_ERROR, NULL));
      }

      status = nsm_qos_unset_port_service_policy (nm, ifp, msg->pmap_name);
      ret = nsm_smi_server_send_sync_message (ase, SMI_MSG_STATUS,
                                              status, NULL);
    }
    break;

    case SMI_MSG_QOS_GET_PORT_SERVICE_POLICY:
    {
      ifp = if_lookup_by_name (&nm->vr->ifm, msg->if_name);
      if (ifp == NULL)
      {
        SMI_FN_EXIT (nsm_smi_server_send_sync_message (ase, SMI_MSG_STATUS,
                                                       SMI_ERROR, NULL));
      }

      SMI_SET_CTYPE(replymsg.cindex, SMI_QOS_CTYPE_PMAPNAME);

      status = nsm_qos_get_port_service_policy (nm, ifp, replymsg.pmap_name);
      if (status < 0)
        msgtype = SMI_MSG_STATUS;

      ret = nsm_smi_server_send_sync_message (ase, msgtype,
                                              status, &replymsg);
    }
    break;

    case SMI_MSG_QOS_SET_TRAFFIC_SHAPE:
    {
      ifp = if_lookup_by_name (&nm->vr->ifm, msg->if_name);
      if (ifp == NULL)
      {
        SMI_FN_EXIT (nsm_smi_server_send_sync_message (ase, SMI_MSG_STATUS,
                                                       SMI_ERROR, NULL));
      }

      status = nsm_qos_set_egress_rate_shaping (&qosg, ifp, msg->rate_value,
                                                msg->rate_unit);
      ret = nsm_smi_server_send_sync_message (ase, SMI_MSG_STATUS,
                                              status, NULL);
    }
    break;

    case SMI_MSG_QOS_UNSET_TRAFFIC_SHAPE:
    {
      ifp = if_lookup_by_name (&nm->vr->ifm, msg->if_name);
      if (ifp == NULL)
      {
        SMI_FN_EXIT (nsm_smi_server_send_sync_message (ase, SMI_MSG_STATUS,
                                                       SMI_ERROR, NULL));
      }

      status = nsm_qos_set_egress_rate_shaping (&qosg, ifp,
                                                NSM_QOS_NO_TRAFFIC_SHAPE,
                                                NSM_QOS_RATE_KBPS);
      ret = nsm_smi_server_send_sync_message (ase, SMI_MSG_STATUS,
                                              status, NULL);
    }
    break;

    case SMI_MSG_QOS_GET_TRAFFIC_SHAPE:
    {
      ifp = if_lookup_by_name (&nm->vr->ifm, msg->if_name);
      if (ifp == NULL)
      {
        SMI_FN_EXIT (nsm_smi_server_send_sync_message (ase, SMI_MSG_STATUS,
                                                       SMI_ERROR, NULL));
      }

      SMI_SET_CTYPE(replymsg.cindex, SMI_QOS_CTYPE_RATE_VALUE);
      SMI_SET_CTYPE(replymsg.cindex, SMI_QOS_CTYPE_RATE);

      status = nsm_qos_get_egress_rate_shaping (&qosg, ifp,
                                                &replymsg.rate_value,
                                                &replymsg.rate_unit);
      if (status < 0)
        msgtype = SMI_MSG_STATUS;

      ret = nsm_smi_server_send_sync_message (ase, msgtype,
                                              status, &replymsg);
    }
    break;

    case SMI_MSG_QOS_GET_COS_TO_QUEUE:
    {
      /* Extended ctype TLV */
      SMI_SET_CTYPE (replymsg.cindex, SMI_QOS_CTYPE_EXTENDED);
      /* Cos to Queue map table TLV */
      SMI_SET_CTYPE(replymsg.extended_cindex_1, SMI_QOS_CTYPE_COS_QUEUE_MAPTBL);

      status = nsm_mls_qos_get_cos_to_queue (&qosg, replymsg.cos_queue_map_tbl);
      if (status < 0)
        msgtype = SMI_MSG_STATUS;

      ret = nsm_smi_server_send_sync_message (ase, msgtype,
                                              status, &replymsg);
    }
    break;

    case SMI_MSG_QOS_GET_DSCP_TO_QUEUE:
    {
      /* Extended ctype TLV */
      SMI_SET_CTYPE (replymsg.cindex, SMI_QOS_CTYPE_EXTENDED);
      /* Cos to Queue map table TLV */
      SMI_SET_CTYPE(replymsg.extended_cindex_1,
                    SMI_QOS_CTYPE_DSCP_QUEUE_MAPTBL);

      status = nsm_mls_qos_get_dscp_to_queue (&qosg,
                                              replymsg.dscp_queue_map_tbl);
      if (status < 0)
        msgtype = SMI_MSG_STATUS;

      ret = nsm_smi_server_send_sync_message (ase, msgtype,
                                              status, &replymsg);
    }
    break;

    case SMI_MSG_QOS_GET_TRUST_STATE:
    {
      ifp = if_lookup_by_name (&nm->vr->ifm, msg->if_name);
      if (ifp == NULL)
      {
        SMI_FN_EXIT (nsm_smi_server_send_sync_message (ase, SMI_MSG_STATUS,
                                                       SMI_ERROR, NULL));
      }

      /* Trust state TLV */
      SMI_SET_CTYPE(replymsg.cindex, SMI_QOS_CTYPE_TRUSTSTATE);

      status = nsm_qos_get_trust_state (&qosg, ifp, &replymsg.trust_state);
      if (status < 0)
        msgtype = SMI_MSG_STATUS;

      ret = nsm_smi_server_send_sync_message (ase, msgtype,
                                              status, &replymsg);
    }
    break;

    case SMI_MSG_QOS_GET_PORT_VLAN_PRIORITY:
    {
      ifp = if_lookup_by_name (&nm->vr->ifm, msg->if_name);
      if (ifp == NULL)
      {
        SMI_FN_EXIT (nsm_smi_server_send_sync_message (ase, SMI_MSG_STATUS,
                                                       SMI_ERROR, NULL));
      }

      /* Vlan priority override TLV */
      SMI_SET_CTYPE(replymsg.cindex, SMI_QOS_CTYPE_PORTMODE);

      status = nsm_qos_get_port_vlan_priority_override (&qosg, ifp,
                                                     &replymsg.vlan_port_mode);
      if (status < 0)
        msgtype = SMI_MSG_STATUS;

      ret = nsm_smi_server_send_sync_message (ase, msgtype,
                                              status, &replymsg);
    }

    break;

    case SMI_MSG_QOS_GET_FORCE_TRUST_COS:
    {
      ifp = if_lookup_by_name (&nm->vr->ifm, msg->if_name);
      if (ifp == NULL)
      {
        SMI_FN_EXIT (nsm_smi_server_send_sync_message (ase, SMI_MSG_STATUS,
                                                       SMI_ERROR, NULL));
      }

      /* Force trust cos TLV */
      SMI_SET_CTYPE(replymsg.cindex, SMI_QOS_CTYPE_FORCETRUSTCOS);

      status = nsm_qos_get_force_trust_cos (&qosg, ifp,
                                            &replymsg.force_trust_cos);
      if (status < 0)
        msgtype = SMI_MSG_STATUS;

      ret = nsm_smi_server_send_sync_message (ase, msgtype,
                                              status, &replymsg);
    }
    break;

    case SMI_MSG_QOS_GET_PORT_DA_PRIORITY:
    {
      ifp = if_lookup_by_name (&nm->vr->ifm, msg->if_name);

      if (ifp == NULL)
      {
        SMI_FN_EXIT (nsm_smi_server_send_sync_message (ase, SMI_MSG_STATUS,
                                                       SMI_ERROR, NULL));
      }

      /* DA priority override TLV */
      SMI_SET_CTYPE (replymsg.cindex, SMI_QOS_CTYPE_EXTENDED);
      SMI_SET_CTYPE (replymsg.extended_cindex_1, SMI_QOS_CTYPE_DA_PORTMODE);

      status = nsm_qos_get_port_da_priority_override (&qosg, ifp,
                                                     &replymsg.da_port_mode);
      if (status < 0)
        msgtype = SMI_MSG_STATUS;

      ret = nsm_smi_server_send_sync_message (ase, msgtype,
                                              status, &replymsg);
    }

    default:
    break;
  }
  SMI_FN_EXIT(ret);
}

/* Set/Get Fc params */
int
nsm_smi_server_recv_fc (struct smi_msg_header *header,
                          void *arg, void *message)
{
  struct smi_server_entry *ase;
  struct smi_server *as;
  struct smi_msg_flowctrl *msg = (struct smi_msg_flowctrl *)message;
  struct nsm_master *nm = nsm_master_lookup_by_id (nzg, SMI_DFLT_VRID);
  struct smi_msg_flowctrl replymsg;
  struct interface *ifp = NULL;
  int status = 0, ret = 0;
  int msgtype = header->type;

  SMI_FN_ENTER ();

  ase = arg;
  as = ase->as;

  replymsg.cindex = 0;

  if(IS_NSM_DEBUG_RECV)
    smi_fc_dump (NSM_ZG, msg);

  #ifdef HAVE_HA
  if ((HA_IS_STANDBY (nzg)))
  {
    ret = nsm_check_func_type(msgtype);
    if (ret == SMI_SET_FUNC)
      SMI_FN_EXIT (nsm_smi_server_send_sync_message (ase,
                                                     SMI_MSG_STATUS,
                                                     SMI_ERROR, NULL));
  }
  #endif /* HAVE_HA */

  switch(header->type)
  {
    case SMI_MSG_ADD_FC:
    {
      unsigned char dir;
      ifp = if_lookup_by_name (&nm->vr->ifm, msg->if_name);
      if (ifp == NULL)
      {
        return (nsm_smi_server_send_sync_message (ase, SMI_MSG_STATUS,
                                                  SMI_ERROR, NULL));
      }

      SMI_FLOWCTRL_DIR_CHECK;

      status = port_add_flow_control (ifp, dir);

      ret = nsm_smi_server_send_sync_message (ase, SMI_MSG_STATUS,
                                              status, NULL);
    }
    break;
    case SMI_MSG_DELETE_FC:
    {
      unsigned char dir;
      ifp = if_lookup_by_name (&nm->vr->ifm, msg->if_name);
      if (ifp == NULL)
      {
        SMI_FN_EXIT (nsm_smi_server_send_sync_message (ase, SMI_MSG_STATUS,
                                                       SMI_ERROR, NULL));
      }

      SMI_FLOWCTRL_DIR_CHECK;

      status = port_delete_flow_control (ifp, dir);

      ret = nsm_smi_server_send_sync_message (ase, SMI_MSG_STATUS,
                                              status, NULL);
    }
    break;
    case SMI_MSG_FC_STATISTICS:
    {
      ifp = if_lookup_by_name (&nm->vr->ifm, msg->if_name);
      if (ifp == NULL)
      {
        SMI_FN_EXIT (nsm_smi_server_send_sync_message (ase, SMI_MSG_STATUS,
                                                       SMI_ERROR, NULL));
      }

      SMI_SET_CTYPE(replymsg.cindex, SMI_FC_CTYPE_DIR);
      SMI_SET_CTYPE(replymsg.cindex, SMI_FC_CTYPE_RXPAUSE);
      SMI_SET_CTYPE(replymsg.cindex, SMI_FC_CTYPE_TXPAUSE);

      status = get_flow_control_statistics (ifp,
                                            &(replymsg.dir),
                                            &(replymsg.rxpause),
                                            &(replymsg.txpause));
      if(status < 0)
        msgtype = SMI_MSG_STATUS;

      ret = nsm_smi_server_send_sync_message (ase, msgtype, status,
                                              &replymsg);
    }
    break;
    case SMI_MSG_FC_GET_INTERFACE:
    {
      int rxpause, txpause;
      ifp = if_lookup_by_name (&nm->vr->ifm, msg->if_name);
      if (ifp == NULL)
      {
        SMI_FN_EXIT (nsm_smi_server_send_sync_message (ase, SMI_MSG_STATUS,
                                                       SMI_ERROR, NULL));
      }

      SMI_SET_CTYPE(replymsg.cindex, SMI_FC_CTYPE_DIR);

      status = get_flow_control_statistics (ifp, &(replymsg.dir),
                                            &rxpause, &txpause);
      if(status < 0)
        msgtype = SMI_MSG_STATUS;

      ret = nsm_smi_server_send_sync_message (ase, msgtype, status,
                                              &replymsg);
    }
    break;

    default:
    break;
  }
  SMI_FN_EXIT(ret);
}

int
nsm_check_func_type(int functype)
{
  int ret=SMI_GET_FUNC;

  SMI_FN_ENTER ();

  switch(functype)
  {
    case SMI_MSG_IF_SETMTU:
      ret=SMI_SET_FUNC;
    break;
    case SMI_MSG_IF_SETBW:
      ret=SMI_SET_FUNC;
    break;
    case SMI_MSG_IF_SETFLAG:
      ret=SMI_SET_FUNC;
    break;
    case SMI_MSG_IF_UNSETFLAG:
      ret=SMI_SET_FUNC;
    break;
    case SMI_MSG_IF_SETAUTO:
      ret=SMI_SET_FUNC;
    break;
    case SMI_MSG_IF_SETHWADDR:
      ret=SMI_SET_FUNC;
    break;
    case SMI_MSG_IF_SETDUPLEX:
      ret=SMI_SET_FUNC;
    break;
    case SMI_MSG_IF_UNSETDUPLEX:
      ret=SMI_SET_FUNC;
    break;
    case SMI_MSG_IF_SETMCAST:
      ret=SMI_SET_FUNC;
    break;
    case SMI_MSG_IF_UNSETMT:
      ret=SMI_SET_FUNC;
    break;
    case SMI_MSG_IF_CLEAR_STATISTICS:
      ret=SMI_SET_FUNC;
    break;
    case SMI_MSG_IF_SET_MDIX_CROSSOVER:
      ret=SMI_SET_FUNC;
    break;
    case SMI_MSG_IF_BRIDGE_ADDMAC:
      ret=SMI_SET_FUNC;
    break;
    case SMI_MSG_IF_BRIDGE_DELMAC:
      ret=SMI_SET_FUNC;
    break;
    case SMI_MSG_IF_BRIDGE_FLUSH_DYNAMICENT:
      ret=SMI_SET_FUNC;
    break;
    case SMI_MSG_ADD_BRIDGE:
      ret=SMI_SET_FUNC;
    break;
    case SMI_MSG_ADD_BRIDGE_PORT:
      ret=SMI_SET_FUNC;
    break;
    case SMI_MSG_CHANGE_TYPE:
      ret=SMI_SET_FUNC;
    break;
    case SMI_MSG_DEL_BRIDGE:
      ret=SMI_SET_FUNC;
    break;
    case SMI_MSG_DEL_BRIDGE_PORT:
      ret=SMI_SET_FUNC;
    break;
    case SMI_MSG_SET_PORT_NON_CONFIG:
      ret=SMI_SET_FUNC;
    break;
    case SMI_MSG_SET_PORT_LEARNING:
      ret=SMI_SET_FUNC;
    break;
    case SMI_MSG_SET_EGRESS_PORT_MODE:
      ret=SMI_SET_FUNC;
    break;
    case SMI_MSG_SET_PORT_NON_SWITCHING:
      ret=SMI_SET_FUNC;
    break;
    case SMI_MSG_IF_LACP_ADDLINK:
      ret=SMI_SET_FUNC;
    break;
    case SMI_IF_MSG_LACP_DELETELINK:
      ret=SMI_SET_FUNC;
    break;
    case SMI_MSG_SW_RESET:
      ret=SMI_SET_FUNC;
    break;
   /* If Message */

   /* Vlan Message */
    case SMI_MSG_VLAN_ADD:
      ret=SMI_SET_FUNC;
    break;
    case SMI_MSG_VLAN_DEL:
      ret=SMI_SET_FUNC;
    break;
    case SMI_MSG_VLAN_SET_PORT_MODE:
      ret=SMI_SET_FUNC;
    break;
    case SMI_MSG_VLAN_SET_ACC_FRAME_TYPE:
      ret=SMI_SET_FUNC;
    break;
    case SMI_MSG_VLAN_SET_INGRESS_FILTER:
      ret=SMI_SET_FUNC;
    break;
    case SMI_MSG_VLAN_SET_DEFAULT_VID:
      ret=SMI_SET_FUNC;
    break;
    case SMI_MSG_VLAN_ADD_TO_PORT:
      ret=SMI_SET_FUNC;
    break;
    case SMI_MSG_VLAN_DEL_FROM_PORT:
      ret=SMI_SET_FUNC;
    break;
    case SMI_MSG_VLAN_CLEAR_PORT:
      ret=SMI_SET_FUNC;
    break;
    case SMI_MSG_VLAN_ADD_ALL_EXCEPT_VID:
      ret=SMI_SET_FUNC;
    break;
    case SMI_MSG_VLAN_SET_PORT_PROTO_PROCESS:
      ret=SMI_SET_FUNC;
    break;
    case SMI_MSG_FORCE_DEFAULT_VLAN:
      ret=SMI_SET_FUNC;
    break;
    case SMI_MSG_SET_PRESERVE_CE_COS:
      ret=SMI_SET_FUNC;
    break;
    case SMI_MSG_SET_PORT_BASED_VLAN:
      ret=SMI_SET_FUNC;
    break;
    case SMI_MSG_SET_CPUPORT_DEFAULT_VLAN:
      ret=SMI_SET_FUNC;
    break;
    case SMI_MSG_SET_CPUPORT_BASED_VLAN:
      ret=SMI_SET_FUNC;
    break;
    case SMI_MSG_SVLAN_SET_PORT_ETHER_TYPE:
      ret=SMI_SET_FUNC;
    break;
    /* Vlan Messages */

    /* Qos Messages */
    case SMI_MSG_QOS_GLOBAL_ENABLE:
      ret=SMI_SET_FUNC;
    break;
    case SMI_MSG_QOS_GLOBAL_DISABLE:
      ret=SMI_SET_FUNC;
    break;
    case SMI_MSG_QOS_SET_PMAP_NAME:
      ret=SMI_SET_FUNC;
    break;
    case SMI_MSG_QOS_PMAP_DELETE:
      ret=SMI_SET_FUNC;
    break;
    case SMI_MSG_QOS_SET_CMAP_NAME:
      ret=SMI_SET_FUNC;
    break;
    case SMI_MSG_QOS_DELETE_CMAP_NAME:
      ret=SMI_SET_FUNC;
    break;
    case SMI_MSG_QOS_CMAP_MATCH_TRAFFIC_SET:
      ret=SMI_SET_FUNC;
    break;
    case SMI_MSG_QOS_CMAP_MATCH_TRAFFIC_UNSET:
      ret=SMI_SET_FUNC;
    break;
    case SMI_MSG_QOS_PMAPC_POLICE:
      ret=SMI_SET_FUNC;
    break;
    case SMI_MSG_QOS_PMAPC_POLICE_DELETE:
      ret=SMI_SET_FUNC;
    break;
    case SMI_MSG_QOS_PMAPC_DELETE_CMAP:
       ret = SMI_SET_FUNC;
     break; 
    case SMI_MSG_QOS_SET_PORT_SCHEDULING:
      ret=SMI_SET_FUNC;
    break;
    case SMI_MSG_QOS_SET_DEFAULT_USER_PRIORITY:
      ret=SMI_SET_FUNC;
    break;
    case SMI_MSG_QOS_PORT_SET_REGEN_USER_PRIORITY:
      ret=SMI_SET_FUNC;
    break;
    case SMI_MSG_QOS_GLOBAL_COS_TO_QUEUE:
      ret=SMI_SET_FUNC;
    break;
    case SMI_MSG_QOS_GLOBAL_DSCP_TO_QUEUE:
      ret=SMI_SET_FUNC;
    break;
    case SMI_MSG_QOS_SET_TRUST_STATE:
      ret=SMI_SET_FUNC;
    break;
    case SMI_MSG_QOS_SET_FORCE_TRUST_COS:
      ret=SMI_SET_FUNC;
    break;
    case SMI_MSG_QOS_SET_FRAME_TYPE_PRIORITY_OVERRIDE:
      ret=SMI_SET_FUNC;
    break;
    case SMI_MSG_QOS_SET_VLAN_PRIORITY:
      ret=SMI_SET_FUNC;
    break;
    case SMI_MSG_QOS_UNSET_VLAN_PRIORITY:
      ret=SMI_SET_FUNC;
    break;
    case SMI_MSG_QOS_SET_PORT_VLAN_PRIORITY:
      ret=SMI_SET_FUNC;
    break;
    case SMI_MSG_QOS_SET_QUEUE_WEIGHT:
      ret=SMI_SET_FUNC;
    break;
    case SMI_MSG_QOS_SET_PORT_SERVICE_POLICY:
      ret=SMI_SET_FUNC;
    break;
    case SMI_MSG_QOS_UNSET_PORT_SERVICE_POLICY:
      ret=SMI_SET_FUNC;
    break;
    case SMI_MSG_QOS_SET_TRAFFIC_SHAPE:
      ret=SMI_SET_FUNC;
    break;
    case SMI_MSG_QOS_UNSET_TRAFFIC_SHAPE:
      ret=SMI_SET_FUNC;
    break;
    /* Qos Messages */

    /* Flow Contrl Messages */
    case SMI_MSG_ADD_FC:
      ret=SMI_SET_FUNC;
    break;
    case SMI_MSG_DELETE_FC:
      ret=SMI_SET_FUNC;
    break;
    case SMI_MSG_FC_STATISTICS:
      ret=SMI_SET_FUNC;
    break;
    /* Flow Control Messages */

    /* GVRP Messages */
    case SMI_MSG_GVRP_SET_TIMER:
      ret=SMI_SET_FUNC;
    break;
    case SMI_MSG_GVRP_ENABLE:
      ret=SMI_SET_FUNC;
    break;
    case SMI_MSG_GVRP_DISABLE:
      ret=SMI_SET_FUNC;
    break;
    case SMI_MSG_GVRP_ENABLE_PORT:
      ret=SMI_SET_FUNC;
    break;
    case SMI_MSG_GVRP_DISABLE_PORT:
      ret=SMI_SET_FUNC;
    break;
    case SMI_MSG_GVRP_SET_REG_MODE:
      ret=SMI_SET_FUNC;
    break;
    case SMI_MSG_GVRP_CLEAR_ALL_STATS:
      ret=SMI_SET_FUNC;
    break;
    case SMI_MSG_GVRP_SET_DYNAMIC_VLAN_LEARNING:
      ret=SMI_SET_FUNC;
    break;
    default:
    break;
 }
  SMI_FN_EXIT (ret);
}

int
smi_record_vlan_addtoport_alarm (char *ifname, struct smi_vlan_bmp vlan_bmp,
                                 struct smi_vlan_bmp egr_bmp)
{
  struct smi_vlan_port_alarm vlaninfo;

  pal_mem_set(&vlaninfo, 0, sizeof(struct smi_vlan_port_alarm));

  pal_snprintf(vlaninfo.ifname, SMI_INTERFACE_NAMSIZ, ifname);
  pal_mem_cpy(&vlaninfo.vlan_bmp, &vlan_bmp, sizeof(struct smi_vlan_bmp));
  pal_mem_cpy(&vlaninfo.egr_bmp, &egr_bmp, sizeof(struct smi_vlan_bmp));

  smi_record_fault (nzg, FM_GID_NSM, NSM_SMI_ALARM_NSM_VLAN_ADD_TO_PORT,
                    __LINE__, __FILE__, &(vlaninfo));

  return SMI_SUCEESS;
}

int
smi_record_vlan_delfromport_alarm (char *ifname, struct smi_vlan_bmp vlan_bmp)
{
  struct smi_vlan_port_alarm vlaninfo;

  pal_mem_set(&vlaninfo, 0, sizeof(struct smi_vlan_port_alarm));

  pal_snprintf(vlaninfo.ifname, SMI_INTERFACE_NAMSIZ, ifname);
  pal_mem_cpy(&vlaninfo.vlan_bmp, &vlan_bmp, sizeof(struct smi_vlan_bmp));

  smi_record_fault (nzg, FM_GID_NSM, NSM_SMI_ALARM_NSM_VLAN_DEL_FROM_PORT,
                    __LINE__, __FILE__, &(vlaninfo));

  return SMI_SUCEESS;
}


#endif /* HAVE_SMI_CLIENT_SERVER */
