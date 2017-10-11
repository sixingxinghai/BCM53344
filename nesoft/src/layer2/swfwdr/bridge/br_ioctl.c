/*
  Ioctl handler
  Linux ethernet bridge
  
  Authors:
  Lennert Buytenhek   <buytenh@gnu.org>
  
  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version
  2 of the License, or (at your option) any later version.
*/
#include <linux/autoconf.h>
#include <linux/kernel.h>
#include "if_ipifwd.h"
#include <linux/inetdevice.h>
#include <net/sock.h>
#include <asm/uaccess.h>
#include <net/net_namespace.h>
#include <linux/nsproxy.h>
#include "br_types.h"
#include "br_ioctl.h"
#include "br_api.h"
#include "br_fdb.h"
#include "br_vlan_api.h"
#include "br_vlan.h"
#include "bdebug.h"
#include "br_gmrp_api.h"

const char * const cmdString [] =
  {
    "Invalid command",/* 0*/
    "APNBR_GET_VERSION",/* 1*/
    "APNBR_GET_BRIDGES",/* 2*/
    "APNBR_ADD_BRIDGE",/* 3*/
    "APNBR_DEL_BRIDGE",/* 4*/
    "APNBR_ADD_IF",/* 5*/
    "APNBR_DEL_IF",/* 6*/
    "APNBR_GET_BRIDGE_INFO",/* 7*/
    "APNBR_GET_PORT_LIST",/* 8*/
    "APNBR_SET_AGEING_TIME",/* 9*/
    "APNBR_SET_DYNAMIC_AGEING_INTERVAL",/* 10*/
    "APNBR_GET_PORT_INFO",/* 11*/
    "APNBR_SET_BRIDGE_LEARNING",/* 12*/
    "APNBR_GET_DYNFDB_ENTRIES",/* 13*/
    "APNBR_GET_STATFDB_ENTRIES",/* 14*/
    "APNBR_ADD_STATFDB_ENTRY",/* 15*/
    "APNBR_DEL_STATFDB_ENTRY",/* 16*/
    "APNBR_GET_DEVADDR",/* 17*/
    "APNBR_GET_PORT_STATE",/* 18*/
    "APNBR_SET_PORT_STATE",/* 19*/
    "APNBR_SET_PORT_FWDER_FLAGS",/* 20*/
    "APN_VLAN_ADD",/* 21*/
    "APN_VLAN_DEL",/* 22*/
    "APN_VLAN_SET_PORT_TYPE",/* 23*/
    "APN_VLAN_SET_DEFAULT_PVID",/* 24*/
    "APN_VLAN_ADD_VID_TO_PORT",/* 25*/
    "APN_VLAN_DEL_VID_FROM_PORT",/* 26*/
    "APNBR_FLUSH_FDB_BY_PORT",/* 27 */
    "APNBR_ADD_DYNAMIC_FDB_ENTRY",/* 28 */
    "APNBR_DEL_DYNAMIC_FDB_ENTRY",/* 29*/
    "APNBR_ADD_VLAN_TO_INST",/* 30 */
    "APNBR_ENABLE_IGMP_SNOOPING",/* 31 */
    "APNBR_DISABLE_IGMP_SNOOPING",/* 32 */
    "APN_VLAN_SET_NATIVE_VID",/* 33 */
    "APN_VLAN_SET_MTU",/* 34 */
    "APNBR_GET_UNICAST_ENTRIES",/* 35 */ 
    "APNBR_GET_MULTICAST_ENTRIES",/* 36 */ 
    "APNBR_CLEAR_FDB_BY_MAC", /* 37 */
    "APNBR_GARP_SET_BRIDGE_TYPE", /* 38 */
    "APNBR_ADD_GMRP_SERVICE_REQ", /* 39 */
    "APNBR_SET_EXT_FILTER", /* 40 */
    "APNBR_SET_PVLAN_TYPE", /* 41 */
    "APNBR_SET_PVLAN_ASSOCIATE", /* 42 */
    "APNBR_SET_PVLAN_PORT_MODE", /* 43 */
    "APNBR_SET_PVLAN_HOST_ASSOCIATION", /* 44 */
    "APNBR_ADD_CVLAN_REG_ENTRY",  /* 45 */
    "APNBR_DEL_CVLAN_REG_ENTRY",  /* 46 */
    "APNBR_ADD_VLAN_TRANS_ENTRY", /* 47 */
    "APNBR_DEL_VLAN_TRANS_ENTRY", /* 48 */
    "APNBR_SET_PROTO_PROCESS",     /* 49 */
    "APNBR_CHANGE_VLAN_TYPE", /* 50 */
    "APNBR_GET_IFINDEX_BY_MAC_VID", /* 55 */
  };


#define GET_CMD()\
  if(copy_from_user ((&cmd), ((void *)(arg)), (1 * sizeof(unsigned long))))\
  {\
    BDEBUG("Couldn't get command\n");\
    return -EFAULT;\
  }

#define GET_ARGS(num)\
  if (copy_from_user ((arglist), ((void *)(arg)), ((num) * sizeof(unsigned long))))\
    {\
      BDEBUG("Couldn't get %d arguments\n", (num));\
      return -EFAULT;\
    } 

#define GET_STRING(arg, str, len)\
  if (copy_from_user ((str), ((void *)(arg)), (len)))\
    {\
      BDEBUG("Couldn't get string argument\n");\
      return -EFAULT;\
    } \
   (str)[(len) - 1] = '\0';


#define GET_BRIDGE()\
  br = br_find_bridge(bridge_name);\
  if (br == NULL)\
    {\
      BDEBUG("Bridge %s not found\n", (bridge_name));\
      return -EINVAL;\
    }


/* This function handles ioctls which have a bridge argument. */

int 
br_ioctl_bridge (struct sock * sk, unsigned long arg)
{
  unsigned long cmd;
  GET_CMD();
  
  BDEBUG("br_ioctl_bridge:Executing cmd %ld (%s)\n", cmd, 
         (cmd > 0 && cmd <= APNBR_MAX_CMD) ? cmdString[cmd] : "Unknown command");

  switch (cmd)
    {
    case APNBR_GET_VERSION:
      {
        int version = PACOS_LAYER2_VERSION ;
        unsigned long arglist[2];
        GET_ARGS(2);
      
        if (copy_to_user ((void *)arglist[1], &version, sizeof(int)))
          {
            return -EFAULT;
          }

        return 0;
      }

    case APNBR_ADD_IF:
    case APNBR_DEL_IF:
      {
        /* arglist contains (cmd, bridge_name, ifindex) */
        unsigned long arglist[3];
        char bridge_name[IFNAMSIZ + 1];
        struct apn_bridge *br = 0;
        struct net_device *dev = 0;
        int ret;
      
        GET_ARGS(3);
        memset (bridge_name, 0, IFNAMSIZ + 1);
        GET_STRING(arglist[1], bridge_name, IFNAMSIZ);
        GET_BRIDGE();

        BDEBUG("Bridge %s selected\n", bridge_name);

        dev = dev_get_by_index (current->nsproxy->net_ns, arglist[2]);
        if (dev == NULL)
          {
            return -EINVAL;
          }

        BDEBUG("Executing add interface %ld\n", arglist[2]);
      
        if (cmd == APNBR_ADD_IF)
          {
            ret = br_add_if (br, dev);
          }
        else
          {
            ret = br_del_if (br, dev);
          }

        dev_put (dev);
        return ret;
      }

    case APNBR_GET_BRIDGE_INFO:
      {
        /* arglist contains (cmd, bridge_name, bridge_info ptr) */
        unsigned long arglist[3];
        char bridge_name[IFNAMSIZ + 1];
        struct apn_bridge *br = 0;
        struct bridge_info b;

        GET_ARGS(3);
        memset (bridge_name, 0, IFNAMSIZ + 1);
        GET_STRING(arglist[1], bridge_name, IFNAMSIZ);
        GET_BRIDGE();

        BDEBUG("Bridge %s selected, executing get bridge info\n", bridge_name);

        memset (&b, '\0', sizeof(struct bridge_info));
        b.up = (br->flags & APNBR_UP);
        b.learning_enabled = (br->flags & APNBR_LEARNING_ENABLED);
        b.ageing_time = br->ageing_time / HZ;
        b.dynamic_ageing_period = br->dynamic_ageing_interval / HZ;
        b.dynamic_ageing_timer_value = (br->dynamic_ageing_interval - 
                                        br_timer_get_residue (&br->dynamic_aging_timer)) / HZ;

        if (copy_to_user ((void *)arglist[2], &b, sizeof(b)))
          {
            return -EFAULT;
          }

        return 0;
      }

    case APNBR_GET_PORT_LIST:
      {
        /* arglist contains (cmd, bridge_name, if_indices ptr, max_ports) */
        unsigned long arglist[4];
        char bridge_name[IFNAMSIZ + 1];
        struct apn_bridge *br = 0;
        int i;
        int num_ports = -1;
        int indices[BR_MAX_PORTS];

        GET_ARGS(4);
        memset (bridge_name, 0, IFNAMSIZ + 1);
        GET_STRING(arglist[1], bridge_name, IFNAMSIZ);
        GET_BRIDGE();
      
        BDEBUG("Bridge %s selected, executing get portlist\n", bridge_name);

        if (arglist[3] > BR_MAX_PORTS)
          {
            arglist[3] = BR_MAX_PORTS;
          }

        for (i = 0; i < arglist[3]; i++)
          {
            indices[i] = 0;
          }

        num_ports = br_get_port_ifindices (br, indices,  arglist[3]);

        if (copy_to_user ((void *)arglist[2], indices, arglist[3] * sizeof(int)))
          {
            return -EFAULT;
          }

        return num_ports;
      }

    case APNBR_SET_PORT_STATE:
      {
        /* arglist contains (cmd, bridge_name, if_index, instance, port_state) */
        unsigned long arglist[5];
        char bridge_name[IFNAMSIZ + 1];
        struct apn_bridge *br = 0;
        struct apn_bridge_port *port = 0; 
      
        GET_ARGS(5);
        memset (bridge_name, 0, IFNAMSIZ + 1);
        GET_STRING(arglist[1], bridge_name, IFNAMSIZ);
        GET_BRIDGE();
      
        BDEBUG("Bridge %s selected, executing set port state to %ld for "
               "if_index %ld instance %d \n" , bridge_name, arglist[4], 
               arglist[2], arglist[3]);

        if ((port = br_get_port (br, arglist[2])) == NULL)
          {
            BDEBUG("Invalid port  %ld\n", arglist[2]);
            return -EINVAL;
          }
      
        if (arglist[4] >= BR_STATE_MAX)
          {
          
            BDEBUG("Invalid  port State %ld\n", arglist[4]);
            return -EINVAL;
          }
      
        port->state[arglist[3]] = arglist[4];

        return 0;
      
      }

    case APNBR_GET_PORT_STATE:
      {
        /* arglist contains (cmd, bridge_name, if_index, instance, port_state) */
        unsigned long arglist[5];
        char bridge_name[IFNAMSIZ + 1];
        struct apn_bridge *br = 0;
        struct apn_bridge_port *port = 0;
      
        GET_ARGS(5);
        memset (bridge_name, 0, IFNAMSIZ + 1);
        GET_STRING(arglist[1], bridge_name, IFNAMSIZ);
        GET_BRIDGE();
      
        BDEBUG("Bridge %s selected, executing get port state for if_index %ld "
               "instance %ld\n", bridge_name, arglist[2],arglist[3]);
      
        if ((port = br_get_port (br, arglist[2])) == NULL)
          {
            return -EINVAL;
          }
      
        if (copy_to_user ((void *)arglist[4], &(port->state[arglist[3]]),
                          sizeof(int)))
          {
            return -EFAULT;
          }
      
        return 0;
      
      }
    
    case APNBR_SET_PORT_FWDER_FLAGS:
      {
        /* arglist contains (cmd, bridge_name, if_index, 
         *                                instance, learn, forward) */
        unsigned long arglist[6];
        char bridge_name[IFNAMSIZ + 1];
        struct apn_bridge *br = 0;
        struct apn_bridge_port *port = 0; 
      
        GET_ARGS(6);
        memset (bridge_name, 0, IFNAMSIZ + 1);
        GET_STRING(arglist[1], bridge_name, IFNAMSIZ);
        GET_BRIDGE();
      
        BDEBUG("Bridge %s selected, executing set port flags"
               "learn  %ld forward %ld for if_index %ld i instance %d \n",
               bridge_name, arglist[5], arglist[4],arglist[2],arglist[3]);

        if ((port = br_get_port (br, arglist[2])) == NULL)
          {
            BDEBUG("Invalid port  %ld\n", arglist[2]);
            return -EINVAL;
          }
      
        if (arglist[4]) 
          port->state_flags[arglist[3]] |= APNFLAG_LEARN;
        else
          port->state_flags[arglist[3]] &= ~(APNFLAG_LEARN);

        if (arglist[5]) 
          port->state_flags[arglist[3]] |= APNFLAG_FORWARD ;
        else
          port->state_flags[arglist[3]] &= ~(APNFLAG_FORWARD);

        if (br->is_vlan_aware)
          br_vlan_update_vlan_dev (br, port, arglist[3]);

        return 0;
      
      }

    
    case APNBR_FLUSH_FDB_BY_PORT:
      {
        /* arglist contains (cmd, bridge_name, if_index, instance, vid, svid) */

        unsigned long arglist[6];
        char bridge_name[IFNAMSIZ + 1];
        struct apn_bridge *br = 0;
        struct apn_bridge_port *port = 0; 
        unsigned int instance = 0;
        unsigned short vid;
        unsigned short svid;
        int port_no;
      
        GET_ARGS(6);
        memset (bridge_name, 0, IFNAMSIZ + 1);
        GET_STRING(arglist[1], bridge_name, IFNAMSIZ);
        GET_BRIDGE();
        vid = arglist[4];
        svid = arglist[5];
        port_no = arglist[2];
        instance = arglist[3];

        BDEBUG("Bridge %s selected, executing flush fdb  for if_index %ld"
               " instance %d vid %d svid %d\n", bridge_name, arglist[2], 
               instance, vid, svid);

        if ( port_no && ((port = br_get_port (br, arglist[2])) == NULL))
          {
            BDEBUG("Invalid port  %ld\n", arglist[2]);
            return -EINVAL;
          }

        br_fdb_delete_dynamic_by_port (br, port, instance, vid, svid);

        return 0;
      
      }

    case APNBR_CHANGE_VLAN_TYPE:
      {
        /* arglist contains (cmd, bridge_name, is_vlan_aware, type) */
        char bridge_name[IFNAMSIZ + 1];
        struct apn_bridge *br = NULL;
        unsigned long arglist[4];
        int is_vlan_aware;
        int type;

        GET_ARGS(4);
        memset (bridge_name, 0, IFNAMSIZ + 1);
        GET_STRING(arglist[1], bridge_name, IFNAMSIZ);
        GET_BRIDGE();
        is_vlan_aware = arglist [2];
        type = arglist [3];

        if (br->is_vlan_aware == BR_TRUE
            && is_vlan_aware == BR_FALSE)
          br_vlan_reset_port_type (br);
        else if (BR_IS_PROVIDER_BRIDGE (br->type)
                 && !BR_IS_PROVIDER_BRIDGE (type))
          br_vlan_reset_port_type (br);

        br->is_vlan_aware = is_vlan_aware;
        br->type = type;
        BDEBUG ("Bridge %s, changing vlan type\n", bridge_name);
        return 0; 
      }

    case APNBR_SET_AGEING_TIME:
      {
        /* arglist contains (cmd, bridge_name, ageing_time, vid) */
        unsigned long arglist[4];
        char bridge_name[IFNAMSIZ + 1];
        struct apn_bridge *br = 0;

        GET_ARGS(4);
        memset (bridge_name, 0, IFNAMSIZ + 1);
        GET_STRING(arglist[1], bridge_name, IFNAMSIZ);
        GET_BRIDGE();
       
        BDEBUG("Bridge %s selected, executing set ageing time %ld\n", 
               bridge_name, arglist[2]);

        if (arglist[2] >= 10 && arglist[2] <= 1000000)
          {
            BDEBUG("Set ageing_time to %ld\n", arglist[2]);
            /* Set ageing time of the FID */
            br->ageing_time = arglist[2] * HZ;
            return 0;
          }
        else
          {
            return -EINVAL;
          }
      }

    case APNBR_SET_DYNAMIC_AGEING_INTERVAL:
      {
        /* arglist contains (cmd, bridge_name, instance, ageing_interval,) */
        unsigned long arglist[3];
        char bridge_name[IFNAMSIZ + 1];
        struct apn_bridge *br = 0;
      
        GET_ARGS(3);
        memset (bridge_name, 0, IFNAMSIZ + 1);
        GET_STRING(arglist[1], bridge_name, IFNAMSIZ);
        GET_BRIDGE();

        BDEBUG("Bridge %s selected, executing set dynamic ageing interval %ld\n", 
               bridge_name, arglist[2]);

        if (arglist[2] >= 1 && arglist[2] <= 10)
          {
            BDEBUG("Set dynamic ageing interval to %ld\n", arglist[2]);
            br->dynamic_ageing_interval = arglist[2] * HZ;
            return 0;
          }
        else
          {
            return -EINVAL;
          }
      }

    case APNBR_GET_PORT_INFO:
      {
        /* arglist contains (cmd, bridge_name, port_info ptr, if_index) */
        unsigned long arglist[4];
        char bridge_name[IFNAMSIZ + 1];
        struct apn_bridge *br = 0;
        struct apn_bridge_port *pt = 0;
        struct port_info p;
      
        GET_ARGS(4);
        memset (bridge_name, 0, IFNAMSIZ + 1);
        GET_STRING(arglist[1], bridge_name, IFNAMSIZ);
        GET_BRIDGE();

        BDEBUG("Bridge %s selected, executing set port info for if_index %ld\n", 
               bridge_name, arglist[3]);

        if ((pt = br_get_port (br, arglist[3])) == NULL)
          {
            return -EINVAL;
          }

        memset (&p, '\0', sizeof(struct port_info));
        p.port_id = pt->port_no;

        if (copy_to_user ((void *)arglist[2], &p, sizeof(p)))
          {
            return -EFAULT;
          }

        return 0;
      }

    case APNBR_SET_BRIDGE_LEARNING:
      {
        /* arglist contains (cmd, bridge_name, learning, instance) */
        unsigned long arglist[3];
        char bridge_name[IFNAMSIZ + 1];
        struct apn_bridge *br = 0;

        GET_ARGS(3);
        memset (bridge_name, 0, IFNAMSIZ + 1);
        GET_STRING(arglist[1], bridge_name, IFNAMSIZ);
        GET_BRIDGE();

        BDEBUG("Bridge %s selected, executing set bridge learning %ld\n", 
               bridge_name, arglist[2]);
      
        if (arglist[2])
          {
            /* Set learn flag of the vid */
            br->flags |= APNBR_LEARNING_ENABLED;
          }
        else
          {
            /* Set learn flag of the vid */
            br->flags &= ~(APNBR_LEARNING_ENABLED);
          }
        return 0;
      } 

    case APNBR_GET_DYNFDB_ENTRIES:
      {
        /* arglist contains (cmd, bridge_name, fdb_entries ptr, max_entries,
           offset) */
        unsigned long arglist[5];
        char bridge_name[IFNAMSIZ + 1];
        struct apn_bridge *br = 0;

        GET_ARGS(5);
        memset (bridge_name, 0, IFNAMSIZ + 1);
        GET_STRING(arglist[1], bridge_name, IFNAMSIZ);
        GET_BRIDGE();

        BDEBUG("Bridge %s selected, executing get dynamic fdb entries\n", 
               bridge_name);
      
        return br_fdb_get_entries (br, (void *)arglist[2], 
                                   arglist[3], arglist[4]);
      }

    case APNBR_ADD_STATFDB_ENTRY:
      {
        /* arglist contains (cmd, bridge_name, if_index, mac ptr, is_fwd, vid) */
        unsigned long arglist[7];
        char bridge_name[IFNAMSIZ + 1];
        struct apn_bridge *br = 0;
        struct apn_bridge_port *p = 0;
        unsigned short vid;
        unsigned short svid;
        char buf[IFHWADDRLEN];

        GET_ARGS(7);
        memset (bridge_name, 0, IFNAMSIZ + 1);
        GET_STRING(arglist[1], bridge_name, IFNAMSIZ);
        GET_BRIDGE();
        vid = arglist[5];
        svid = arglist[6];

        BDEBUG("Bridge %s selected, executing add static fdb entry, if_index(%ld)"
               " is_fwd(%ld) vid %d\n", bridge_name, arglist[2], arglist[4], 
               arglist[5]);
     
        if ((p = br_get_port (br, arglist[2])) == NULL)
          {
            return -EINVAL;
          }

        BDEBUG("found port (%ld)\n",arglist[2]);    

        if (p->port_type == PRO_NET_PORT
            || p->port_type == CUST_NET_PORT)
          {
            /* Validate FID */
            if (!br_vlan_validate_vid (br, SERVICE_VLAN, svid))
              return -EINVAL;
          }
        else
          {
            /* Validate FID */
            if (!br_vlan_validate_vid (br, CUSTOMER_VLAN, vid))
              return -EINVAL;
          }
        
        if (copy_from_user (buf, (void *)arglist[3], IFHWADDRLEN)) 
          {
            return -EFAULT;
          }

        BDEBUG("Adding static mac (%.02x:%.02x:%.02x:%.02x:%.02x:%.02x)\n",
               buf[0], buf[1], buf[2], buf[3], buf[4], buf[5]);    

        return br_fdb_insert_static (br, p, buf, arglist[4] ? 1 : 0, vid, svid);
      }

    case APNBR_DEL_STATFDB_ENTRY:
      {
        /* arglist contains (cmd, bridge_name, if_index, mac ptr, vid) */
        unsigned long arglist[6];
        char bridge_name[IFNAMSIZ + 1];
        struct apn_bridge *br = 0;
        struct apn_bridge_port *p = 0;
        unsigned short vid;
        unsigned short svid;
        char buf[IFHWADDRLEN];

        GET_ARGS(6);
        memset (bridge_name, 0, IFNAMSIZ + 1);
        GET_STRING(arglist[1], bridge_name, IFNAMSIZ);
        GET_BRIDGE();
        vid = arglist[4];
        svid = arglist[5];

        BDEBUG ("Bridge %s selected, executing delete static fdb entry, "
                "if_index(%ld) vid %d \n", bridge_name, arglist[2],arglist[4]);
      
        if ((p = br_get_port (br, arglist[2])) == NULL)
          {
            return -EINVAL;
          }

        if (p->port_type == PRO_NET_PORT
            || p->port_type == CUST_NET_PORT)
          {
            /* Validate FID */ 
            if (!br_vlan_validate_vid (br, SERVICE_VLAN, svid))
              return -EINVAL;
          }
        else
          {
            /* Validate FID */ 
            if (!br_vlan_validate_vid (br, CUSTOMER_VLAN, vid))
              return -EINVAL;
          }
        
        if (copy_from_user (buf, (void *)arglist[3], IFHWADDRLEN))
          {
            return -EFAULT;
          }

        BDEBUG("Deleting static mac (%.02x:%.02x:%.02x:%.02x:%.02x:%.02x)\n",
               buf[0], buf[1], buf[2], buf[3], buf[4], buf[5]);    
      
        return br_fdb_rem_static (br, p, buf, vid, svid);
      }

    case APNBR_ADD_DYNAMIC_FDB_ENTRY :
      {
        /* arglist contains (cmd, bridge_name, if_index, vid, mac ptr, vid) */
        unsigned long arglist[6];
        char bridge_name[IFNAMSIZ + 1];
        struct apn_bridge *br = 0;
        struct apn_bridge_port *p = 0;
        char buf[IFHWADDRLEN];
        unsigned short vid;
        unsigned short svid;
        int ret;

        GET_ARGS(6);
        memset (bridge_name, 0, IFNAMSIZ + 1);
        GET_STRING(arglist[1], bridge_name, IFNAMSIZ);
        GET_BRIDGE();
        vid = arglist[4];
        svid = arglist[5];

        BDEBUG("Bridge %s selected, executing add dynamic fdb entry, "
               "if_index(%ld)\n", bridge_name, arglist[2]);

        if ((p = br_get_port (br, arglist[2])) == NULL)
          {
            return -EINVAL;
          }
        
        BDEBUG("found port (%ld)\n",arglist[2]);

        if (p->port_type == PRO_NET_PORT
            || p->port_type == CUST_NET_PORT)
          {
            /* Validate FID */
            if (!br_vlan_validate_vid (br, SERVICE_VLAN, svid))
              return -EINVAL;
          }
        else
          {
            /* Validate FID */
            if (!br_vlan_validate_vid (br, CUSTOMER_VLAN, vid))
              return -EINVAL;
          }

        if (copy_from_user (buf, (void *)arglist[3], IFHWADDRLEN)) 
          {
            return -EFAULT;
          }

        BDEBUG("Adding dynamic mac (%.02x:%.02x:%.02x:%.02x:%.02x:%.02x)\n",
               buf[0], buf[1], buf[2], buf[3], buf[4], buf[5]);    

        ret = br_fdb_insert (br, p, buf, vid, svid, 0, arglist[4]);

        return (ret == BR_NORESOURCE) ? -ENOMEM : 0;
      }

    case APNBR_DEL_DYNAMIC_FDB_ENTRY :
      {
        /* arglist contains (cmd, bridge_name, if_index, vid, mac ptr, is_forward) */
        unsigned long arglist[6];
        char bridge_name[IFNAMSIZ + 1];
        struct apn_bridge *br = 0;
        struct apn_bridge_port *p = 0;
        char buf[IFHWADDRLEN];
        unsigned short vid;
        unsigned short svid;
        int ret;

        GET_ARGS(6);
        memset (bridge_name, 0, IFNAMSIZ + 1);
        GET_STRING(arglist[1], bridge_name, IFNAMSIZ);
        GET_BRIDGE();
        vid = arglist[4];
        svid = arglist[5];

        BDEBUG("Bridge %s selected, executing add dynamic fdb entry, "
               "if_index(%ld)\n", bridge_name, arglist[2]);
     
        if ((p = br_get_port (br, arglist[2])) == NULL)
          {
            return -EINVAL;
          }
        
        BDEBUG("found port (%ld)\n",arglist[2]);    

        if (p->port_type == PRO_NET_PORT
            || p->port_type == CUST_NET_PORT)
          {
            /* Validate FID */
            if (!br_vlan_validate_vid (br, SERVICE_VLAN, svid))
              return -EINVAL;
          }
        else
          {
            /* Validate FID */
            if (!br_vlan_validate_vid (br, CUSTOMER_VLAN, vid))
              return -EINVAL;
          }

        if (copy_from_user (buf, (void *)arglist[3], IFHWADDRLEN)) 
          {
            return -EFAULT;
          }

        BDEBUG("Deleting dynamic mac (%.02x:%.02x:%.02x:%.02x:%.02x:%.02x)\n",
               buf[0], buf[1], buf[2], buf[3], buf[4], buf[5]);    

        ret = br_fdb_delete (br, p, buf, vid, svid);

        return (ret == BR_NORESOURCE) ? -ENOMEM : 0;
      }

    case APNBR_GET_STATFDB_ENTRIES:
      {
        /* arglist contains (cmd, bridge_name, fdb_entries ptr, max_entries,
           offset) */
        unsigned long arglist[5];
        char bridge_name[IFNAMSIZ + 1];
        struct apn_bridge *br = 0;

        GET_ARGS(5);
        memset (bridge_name, 0, IFNAMSIZ + 1);
        GET_STRING(arglist[1], bridge_name, IFNAMSIZ);
        GET_BRIDGE();

        BDEBUG("Bridge %s selected, executing get static fdb entries\n", 
               bridge_name);
     
        /* The copy_to_user is done inside the br_fdb_get_static_entries */
        return br_fdb_get_static_entries (br, (void *)arglist[2], 
                                          arglist[3], arglist[4]);
      }

    case APNBR_GET_BRIDGES:
      {
        /* arglist contains (cmd, bridge_name ptr, max_bridge_entries) */
        unsigned long arglist[3];
        char *name_list = 0;

        BDEBUG("In APNBR_GET_BRIDGES \n");

        GET_ARGS(3);

        if (arglist[2] > BR_MAX_BRIDGES)
          {
            arglist[2] = BR_MAX_BRIDGES;
          }

        name_list = kmalloc (IFNAMSIZ * arglist[2], GFP_KERNEL);

        if (name_list == 0)
          {
            return -EFAULT;
          }
        memset(name_list, '\0', sizeof(name_list));

        /* Add socket argument to "owned" get_bridge_names. */
        arglist[2] = br_get_bridge_names (name_list, arglist[2]);
        if (copy_to_user ((void *)arglist[1], name_list, arglist[2] * IFNAMSIZ))
          {
            BDEBUG(" mem errr \n");
            kfree (name_list);
            return -EFAULT;
          }

        kfree (name_list);
        BDEBUG(" returning from br ioctl %d \n", arglist[2]);
        return arglist[2];
      }

    case APNBR_ADD_BRIDGE:
      {
        /* arglist contains (cmd, bridge_name, is_vlan_aware, protocol, edge) */
        unsigned long arglist[6];
        char bridge_name[IFNAMSIZ + 1];

        GET_ARGS(6);
        memset (bridge_name, 0, IFNAMSIZ + 1);
        GET_STRING(arglist[1], bridge_name, IFNAMSIZ);

        return br_add_bridge (bridge_name, sk, arglist[2], arglist[3], arglist[4],
                              arglist[5]);
      }

    case APNBR_DEL_BRIDGE:
      {
        /* arglist contains (cmd, bridge_name */
        unsigned long arglist[2];
        char bridge_name[IFNAMSIZ + 1];

        GET_ARGS(2);
        memset (bridge_name, 0, IFNAMSIZ + 1);
        GET_STRING(arglist[1], bridge_name, IFNAMSIZ);

        return br_del_bridge (bridge_name, sk);
      }

    case APNBR_GET_DEVADDR:
      {      
        /* arglist contains (cmd, mac ptr, if_index) */
        unsigned long arglist[3];
        struct net_device *dev = 0;
      
        GET_ARGS(3);

        dev = dev_get_by_index (current->nsproxy->net_ns, arglist[2]);

        if(dev == NULL)
          {
            return -EINVAL;
          }
      
        if (copy_to_user ((void *)arglist[1], dev->dev_addr, ETH_ALEN))
          {
            return -EFAULT;
          }

        dev_put(dev);
        return 0;
      }

    case APNBR_ADD_VLAN_TO_INST :
      {
        /* arglist contains (cmd, bridge_name, instance_id, vlanid) */
        unsigned long arglist[4];
        char bridge_name[IFNAMSIZ + 1];
        unsigned short instance_id;
        unsigned short vlanid;

        GET_ARGS(4);
        memset (bridge_name, 0, IFNAMSIZ + 1);
        GET_STRING(arglist[1], bridge_name, IFNAMSIZ);
      
        instance_id = arglist[2];
        vlanid = arglist[3];      

        return br_vlan_add_to_inst (bridge_name, vlanid, instance_id);
      }

    case APN_VLAN_ADD:
      {
        /* arglist contains (cmd, bridge_name, vid) */
        unsigned char num_args = 5;
        unsigned long arglist[num_args];
        char bridge_name[IFNAMSIZ + 1];

        GET_ARGS(num_args);
        memset (bridge_name, 0, IFNAMSIZ + 1);
        GET_STRING(arglist[1], bridge_name, IFNAMSIZ);
	
        BDEBUG("Bridge %s, vid(%u) executing vlan add \n",
               bridge_name, arglist[2]);

        return br_vlan_add (bridge_name, arglist[2], arglist[3]);
      }

    case APN_VLAN_DEL:
      {
        /* arglist contains (cmd, bridge_name, type, vid) */
        unsigned char num_args = 4;
        unsigned long arglist[num_args];
        char bridge_name[IFNAMSIZ + 1];

        GET_ARGS(num_args);
        memset (bridge_name, 0, IFNAMSIZ + 1);
        GET_STRING(arglist[1], bridge_name, IFNAMSIZ);

        BDEBUG("Bridge %s, executing vlan delete vid(%ld)\n", bridge_name,
               arglist[2]);
      
        return br_vlan_delete (bridge_name, arglist[2], arglist[3]);
      }

    case APN_VLAN_SET_PORT_TYPE:
      {
        /* arglist contains (cmd, bridge_name, if_index, port_type, 
           acceptable_frame_types, enable_ingress_filter */
        unsigned num_args = 7;
        unsigned long arglist[num_args];
        char bridge_name[IFNAMSIZ + 1];

        GET_ARGS(num_args);
        memset (bridge_name, 0, IFNAMSIZ + 1);
        GET_STRING(arglist[1], bridge_name, IFNAMSIZ);
      
        return br_vlan_set_port_type (bridge_name, arglist[2], arglist[3],
                                      arglist[4], arglist[5], arglist[6]);
      }

    case APN_VLAN_SET_DEFAULT_PVID:
      {
        /* arglist contains (cmd, bridge_name, if_index, pvid, egress_tagged) */
        unsigned num_args = 5;
        unsigned long arglist[num_args];
        char bridge_name[IFNAMSIZ + 1];

        GET_ARGS(num_args);
        memset (bridge_name, 0, IFNAMSIZ + 1);
        GET_STRING(arglist[1], bridge_name, IFNAMSIZ);
      
        return br_vlan_set_default_pvid (bridge_name, arglist[2], arglist[3],
                                         arglist[4]);
      }

    case APN_VLAN_SET_NATIVE_VID:
      {
        /* arglist contains (cmd, bridge_name, if_index, native_vid ) */
        unsigned num_args = 4;
        unsigned long arglist[num_args];
        char bridge_name[IFNAMSIZ + 1];

        GET_ARGS(num_args);
        memset (bridge_name, 0, IFNAMSIZ + 1);
        GET_STRING(arglist[1], bridge_name, IFNAMSIZ);

        return br_vlan_set_native_vid (bridge_name, arglist[2], arglist[3]);
      }

    case APN_VLAN_SET_MTU:
      {
        /* arglist contains (cmd, bridge_name, vid, mtu_val) */
        unsigned num_args = 4;
        unsigned long arglist[num_args];
        char bridge_name[IFNAMSIZ + 1];

        GET_ARGS(num_args);
        memset (bridge_name, 0, IFNAMSIZ + 1);
        GET_STRING(arglist[1], bridge_name, IFNAMSIZ);

        return br_vlan_set_mtu (bridge_name, arglist[2], arglist[3]);
      }

    case APN_VLAN_ADD_VID_TO_PORT:
      {
        /* arglist contains (cmd, bridge_name, if_index, vid, egress_tagged) */
        unsigned char num_args = 5;
        unsigned long arglist[num_args];
        char bridge_name[IFNAMSIZ + 1];

        GET_ARGS(num_args);
        memset (bridge_name, 0, IFNAMSIZ + 1);
        GET_STRING(arglist[1], bridge_name, IFNAMSIZ);
      
        return br_vlan_add_vid_to_port (bridge_name, arglist[2], arglist[3],
                                        arglist[4]);
      }

    case APN_VLAN_DEL_VID_FROM_PORT:
      {
        /* arglist contains (cmd, bridge_name, if_index, vid) */ 
        unsigned char num_args = 4;
        unsigned long arglist[num_args];
        char bridge_name[IFNAMSIZ + 1];

        GET_ARGS(num_args);
        memset (bridge_name, 0, IFNAMSIZ + 1);
        GET_STRING(arglist[1], bridge_name, IFNAMSIZ);
      
        return br_vlan_delete_vid_from_port (bridge_name, arglist[2], 
                                             arglist[3]);
      }

    case APNBR_ENABLE_IGMP_SNOOPING:
      {
        /* arglist contains (cmd, bridge_name) */
        unsigned long arglist[2];
        char bridge_name[IFNAMSIZ + 1];
        struct apn_bridge *br = 0;

        GET_ARGS(2);
        memset (bridge_name, 0, IFNAMSIZ + 1);
        GET_STRING(arglist[1], bridge_name, IFNAMSIZ);
        GET_BRIDGE();

        BDEBUG("Bridge %s selected, executing enable igmp snooping\n", bridge_name);

        br->flags |= APNBR_IGMP_SNOOP_ENABLED;

        return 0;
      }
    
    case APNBR_DISABLE_IGMP_SNOOPING:
      {
        /* arglist contains (cmd, bridge_name) */
        unsigned long arglist[2];
        char bridge_name[IFNAMSIZ + 1];
        struct apn_bridge *br = 0;

        GET_ARGS(2);
        memset (bridge_name, 0, IFNAMSIZ + 1);
        GET_STRING(arglist[1], bridge_name, IFNAMSIZ);
        GET_BRIDGE();

        BDEBUG("Bridge %s selected, executing disable igmp snooping\n", bridge_name);

        br->flags &= ~APNBR_IGMP_SNOOP_ENABLED;

        return 0;
      }

    case APNBR_GET_UNICAST_ENTRIES:
      {
        /* arglist contains (cmd, bridge_name, mac_addr, vid, max_entries, fdb_entries ptr,
           offset) */
        unsigned long arglist[6];
        char bridge_name[IFNAMSIZ + 1];
        struct apn_bridge *br = 0;

        GET_ARGS(6);
        memset (bridge_name, 0, IFNAMSIZ + 1);
        GET_STRING(arglist[1], bridge_name, IFNAMSIZ);
        GET_BRIDGE();

        BDEBUG("Bridge %s selected, executing get Unicast fdb entries\n",
               bridge_name);

        return br_get_fdb_range (br, (void *)arglist[2], arglist[3],
                                 arglist[4], (void *) arglist[5], BR_UNICAST_FDB);
      }

    case APNBR_GET_MULTICAST_ENTRIES:
      {
        /* arglist contains (cmd, bridge_name, mac_addr, vid, max_entries, fdb_entries ptr,
           offset) */
        unsigned long arglist[6];
        char bridge_name[IFNAMSIZ + 1];
        struct apn_bridge *br = 0;

        GET_ARGS(6);
        memset (bridge_name, 0, IFNAMSIZ + 1);
        GET_STRING(arglist[1], bridge_name, IFNAMSIZ);
        GET_BRIDGE();

        BDEBUG("Bridge %s selected, executing get Unicast fdb entries\n",
               bridge_name);

        return br_get_fdb_range (br, (void *)arglist[2], arglist[3],
                                 arglist[4], (void *) arglist[5], BR_MULTICAST_FDB);

      }

    case APNBR_CLEAR_FDB_BY_MAC:
      {
        /* arglist contains (cmd, bridge_name, mac address) */
        unsigned long arglist[3];
        char bridge_name[IFNAMSIZ + 1];
        unsigned char buf[IFHWADDRLEN];
        struct apn_bridge *br = 0;
        int    ret;

        GET_ARGS(3);
        memset (bridge_name, 0, IFNAMSIZ + 1);
        GET_STRING(arglist[1], bridge_name, IFNAMSIZ);
        GET_BRIDGE();
        BDEBUG("Deleting dynamic mac (%.02x:%.02x:%.02x:%.02x:%.02x:%.02x)\n",
               buf[0], buf[1], buf[2], buf[3], buf[4], buf[5]);

        if (copy_from_user (buf, (void *)arglist[2], IFHWADDRLEN))
          {
            return -EFAULT;
          }

        br_clear_dynamic_fdb_by_mac (br, buf);

        return 0;
      }
    case APNBR_GARP_SET_BRIDGE_TYPE:
      {
        /* arglist contains (cmd, bridge_name, type, enable) */
        unsigned long arglist[4];
        char bridge_name[IFNAMSIZ + 1];
        struct apn_bridge *br = 0;
        int ret;
 
        GET_ARGS(4);
        memset (bridge_name, 0, IFNAMSIZ + 1);
        GET_STRING(arglist[1], bridge_name, IFNAMSIZ);
        GET_BRIDGE();

        BDEBUG("Bridge %s selected\n", bridge_name);
        BDEBUG("Executing garp set port state on interface%ld\n", arglist[1]);
        ret = br_garp_set_bridge_type (br, arglist[2], arglist[3]);

        return ret;
      }

    case APNBR_ADD_GMRP_SERVICE_REQ:
      {
        /* arg list contatins (cmd, bridge_name, vlanid, ifindex, bool_t fwd_all | fwd_unreg, bool_t activate)*/
        unsigned long arglist[6];
        char bridge_name[IFNAMSIZ + 1];
        struct apn_bridge *br = 0;
        int ret;

        GET_ARGS(6);
        memset (bridge_name, 0, IFNAMSIZ + 1);
        GET_STRING(arglist[1], bridge_name, IFNAMSIZ);
        GET_BRIDGE();
        BDEBUG("Bridge %s selected\n", bridge_name);
        BDEBUG("Executing gmrp set service requirement on interface%ld\n", arglist[2]);

        ret = br_gmrp_set_service_requirement (br, arglist[2], arglist[3], arglist[4], arglist[5]);

        return ret;
      }
    
    case APNBR_SET_EXT_FILTER:
      {
        /* arg list contains (cmd, bridge_name, bool_t enable) */
        unsigned long arglist [3];
        char bridge_name[IFNAMSIZ + 1];
        struct apn_bridge *br = 0;
        int ret;

        GET_ARGS(3);
        memset (bridge_name, 0, IFNAMSIZ + 1);
        GET_STRING(arglist[1], bridge_name, IFNAMSIZ);
        GET_BRIDGE();
        BDEBUG("Bridge %s selected\n", bridge_name);

        BDEBUG("Executing gmrp set Extended filtering service on bridge %ld\n", arglist[1]);

        ret = br_gmrp_set_ext_filter (br, arglist[2]); 

        return ret;
      }
  
    case APNBR_SET_PVLAN_TYPE:
      {
        /* cmd list contains (cmd, bridge_name, vid_t vid, 
         * enum pvlan_type type)
         */
        unsigned char num_args = 4;
        unsigned long arglist[num_args];
        char bridge_name[IFNAMSIZ];

        GET_ARGS(num_args);
        memset (bridge_name, 0, IFNAMSIZ);
        GET_STRING(arglist[1], bridge_name, IFNAMSIZ);

        BDEBUG("Bridge %s, vid(%u) executing pvlan set type \n",
               bridge_name, arglist[2]);

        return br_pvlan_set_type (bridge_name, arglist[2], arglist[3]);
      }

    case APNBR_SET_PVLAN_ASSOCIATE:
      {
        /* cmd list contains (cmd, bridge_name, vid_t vid, vid_t pvid,
         * bool_t associate)
         */
        unsigned char num_args = 5;
        unsigned long arglist[num_args];
        char bridge_name[IFNAMSIZ];

        GET_ARGS(num_args);
        memset (bridge_name, 0, IFNAMSIZ);
        GET_STRING(arglist[1], bridge_name, IFNAMSIZ);

        BDEBUG("Bridge %s, vid(%u) pvid(%u) executing pvlan associate %d\n",
               bridge_name, arglist[2], arglist[3], arglist[4]);

        return br_pvlan_set_associate (bridge_name, arglist[2], arglist[3], arglist[4]);

      }
    case APNBR_SET_PVLAN_PORT_MODE:
      {
        /* arglist contains (cmd, bridge_name, if_index, port_mode ) */
        unsigned num_args = 4;
        unsigned long arglist[num_args];
        char bridge_name[IFNAMSIZ];

        GET_ARGS(num_args);
        memset (bridge_name, 0, IFNAMSIZ);
        GET_STRING(arglist[1], bridge_name, IFNAMSIZ);

        return br_pvlan_set_port_mode (bridge_name, arglist[2], arglist[3]);

      }

    case APNBR_SET_PVLAN_HOST_ASSOCIATION: 
      {
        /* arglist contains (cmd, bridge_name, if_index,
           primary_vid, secondary_vid, associate or not */
        unsigned num_args = 6;
        unsigned long arglist[num_args];
        char bridge_name[IFNAMSIZ];

        GET_ARGS(num_args);
        memset (bridge_name, 0, IFNAMSIZ);
        GET_STRING(arglist[1], bridge_name, IFNAMSIZ);

        return br_pvlan_set_port_host_associate
          (bridge_name, arglist[2], arglist[3], arglist[4], arglist[5]);
    
      }

    case APNBR_ADD_CVLAN_REG_ENTRY:
      {
       /* arglist contains (cmd, bridge_name, if_index, cvid, svid) */
        unsigned char num_args = 5;
        unsigned long arglist[num_args];
        char bridge_name[IFNAMSIZ + 1];
        struct apn_bridge *br = 0;
        struct apn_bridge_port *port = 0;
        int port_no = 0;

        GET_ARGS(num_args);
        port_no = arglist[2];
        memset (bridge_name, 0, IFNAMSIZ + 1);
        GET_STRING(arglist[1], bridge_name, IFNAMSIZ);
        GET_BRIDGE();

        if ( ((port = br_get_port (br, port_no)) == NULL))
          {
            BDEBUG("Invalid port  %ld\n", port_no);
            return -EINVAL;
          }

        if (port->port_type != CUST_EDGE_PORT)
          {
            BDEBUG("Invalid port type %ld\n", port_no);
            return -EINVAL;
          }

        return br_vlan_reg_tab_entry_add (bridge_name, port_no, arglist[3],
                                          arglist[4]);
      }

    case APNBR_DEL_CVLAN_REG_ENTRY:
      {
       /* arglist contains (cmd, bridge_name, if_index, cvid, svid) */
        unsigned char num_args = 5;
        unsigned long arglist[num_args];
        char bridge_name[IFNAMSIZ + 1];
        struct apn_bridge *br = 0;
        struct apn_bridge_port *port = 0;
        int port_no = 0;

        GET_ARGS(num_args);
        port_no = arglist[2];
        memset (bridge_name, 0, IFNAMSIZ + 1);
        GET_STRING(arglist[1], bridge_name, IFNAMSIZ);
        GET_BRIDGE();

        if ( ((port = br_get_port (br, port_no)) == NULL))
          {
            BDEBUG("Invalid port  %ld\n", port_no);
            return -EINVAL;
          }

        if (port->port_type != CUST_EDGE_PORT)
          {
            BDEBUG("Invalid port type %ld\n", port_no);
            return -EINVAL;
          }

        return br_vlan_reg_tab_entry_delete (bridge_name, port_no, arglist[3],
                                            arglist[4]);
      }

    case APNBR_ADD_VLAN_TRANS_ENTRY:
      {
       /* arglist contains (cmd, bridge_name, if_index, svid, trans_svid) */
        unsigned char num_args = 5;
        unsigned long arglist[num_args];
        char bridge_name[IFNAMSIZ + 1];
        struct apn_bridge *br = 0;
        struct apn_bridge_port *port = 0;
        int port_no = 0;

        GET_ARGS(num_args);
        port_no = arglist[2];
        memset (bridge_name, 0, IFNAMSIZ + 1);
        GET_STRING(arglist[1], bridge_name, IFNAMSIZ);
        GET_BRIDGE();

        if ( (br_get_port (br, port_no)) == NULL)
          {
            BDEBUG("Invalid port  %ld\n", port_no);
            return -EINVAL;
          }

        return br_vlan_trans_tab_entry_add (bridge_name, port_no, arglist[3],
                                            arglist[4]);
      }

    case APNBR_DEL_VLAN_TRANS_ENTRY:
      {
       /* arglist contains (cmd, bridge_name, if_index, cvid, svid) */
        unsigned char num_args = 5;
        unsigned long arglist[num_args];
        char bridge_name[IFNAMSIZ + 1];
        struct apn_bridge *br = 0;
        struct apn_bridge_port *port = 0;
        int port_no = 0;

        GET_ARGS(num_args);
        port_no = arglist[2];
        memset (bridge_name, 0, IFNAMSIZ + 1);
        GET_STRING(arglist[1], bridge_name, IFNAMSIZ);
        GET_BRIDGE();

        if ( (br_get_port (br, port_no)) == NULL)
          {
            BDEBUG("Invalid port  %ld\n", port_no);
            return -EINVAL;
          }
        return br_vlan_trans_tab_entry_delete (bridge_name, port_no, arglist[3],
                                               arglist[4]);
      }

    case APNBR_SET_PROTO_PROCESS:
      {
       /* arglist contains (cmd, bridge_name, if_index, proto, process, vid) */
        unsigned char num_args = 6;
        unsigned long arglist[num_args];
        char bridge_name[IFNAMSIZ + 1];
        struct apn_bridge *br = 0;
        struct apn_bridge_port *port = 0;
        int port_no = 0;

        GET_ARGS(num_args);
        port_no = arglist[2];
        memset (bridge_name, 0, IFNAMSIZ + 1);
        GET_STRING(arglist[1], bridge_name, IFNAMSIZ);
        GET_BRIDGE();

        if ( ((port = br_get_port (br, port_no)) == NULL))
          {
            BDEBUG("Invalid port  %ld\n", port_no);
            return -EINVAL;
          }

        if (port->port_type != CUST_EDGE_PORT)
          {
            BDEBUG("Invalid port type %ld\n", port_no);
            return -EINVAL;
          }

        return br_port_protocol_process (bridge_name, port_no, arglist[3],
                                         arglist[4], arglist[5]);
      }

    case APN_VLAN_ADD_PRO_EDGE_PORT:
      {
       /* arglist contains (cmd, bridge_name, if_index, svid) */
        unsigned char num_args = 4;
        unsigned long arglist[num_args];
        char bridge_name[IFNAMSIZ + 1];
        struct apn_bridge *br = 0;
        struct apn_bridge_port *port = 0;
        int port_no = 0;

        GET_ARGS(num_args);
        port_no = arglist[2];
        memset (bridge_name, 0, IFNAMSIZ + 1);
        GET_STRING(arglist[1], bridge_name, IFNAMSIZ);
        GET_BRIDGE();

        if ( ((port = br_get_port (br, port_no)) == NULL))
          {
            BDEBUG("Invalid port  %ld\n", port_no);
            return -EINVAL;
          }

        if (port->port_type != CUST_EDGE_PORT)
          {
            BDEBUG("Invalid port type %ld\n", port_no);
            return -EINVAL;
          }

        return br_vlan_add_pro_edge_port (bridge_name, port_no, arglist[3]);

      }

    case APN_VLAN_DEL_PRO_EDGE_PORT:
      {
       /* arglist contains (cmd, bridge_name, if_index, svid) */
        unsigned char num_args = 4;
        unsigned long arglist[num_args];
        char bridge_name[IFNAMSIZ + 1];
        struct apn_bridge *br = 0;
        struct apn_bridge_port *port = 0;
        int port_no = 0;

        GET_ARGS(num_args);
        port_no = arglist[2];
        memset (bridge_name, 0, IFNAMSIZ + 1);
        GET_STRING(arglist[1], bridge_name, IFNAMSIZ);
        GET_BRIDGE();

        if ( ((port = br_get_port (br, port_no)) == NULL))
          {
            BDEBUG("Invalid port  %ld\n", port_no);
            return -EINVAL;
          }

        if (port->port_type != CUST_EDGE_PORT)
          {
            BDEBUG("Invalid port type %ld\n", port_no);
            return -EINVAL;
          }

        return br_vlan_del_pro_edge_port (bridge_name, port_no, arglist[3]);
      }

    case APN_VLAN_SET_PRO_EDGE_DEFAULT_PVID:
      {
       /* arglist contains (cmd, bridge_name, if_index, svid, pvid) */
        unsigned char num_args = 5;
        unsigned long arglist[num_args];
        char bridge_name[IFNAMSIZ + 1];
        struct apn_bridge *br = 0;
        struct apn_bridge_port *port = 0;
        int port_no = 0;

        GET_ARGS(num_args);
        port_no = arglist[2];
        memset (bridge_name, 0, IFNAMSIZ + 1);
        GET_STRING(arglist[1], bridge_name, IFNAMSIZ);
        GET_BRIDGE();

        if ( ((port = br_get_port (br, port_no)) == NULL))
          {
            BDEBUG("Invalid port  %ld\n", port_no);
            return -EINVAL;
          }

        if (port->port_type != CUST_EDGE_PORT)
          {
            BDEBUG("Invalid port type %ld\n", port_no);
            return -EINVAL;
          }

        return br_vlan_set_pro_edge_pvid (bridge_name, port_no, arglist[3],
                                          arglist[4]);
      }

    case APN_VLAN_SET_PRO_EDGE_UNTAGGED_VID:
      {
       /* arglist contains (cmd, bridge_name, if_index, svid, untagged_vid) */
        unsigned char num_args = 5;
        unsigned long arglist[num_args];
        char bridge_name[IFNAMSIZ + 1];
        struct apn_bridge *br = 0;
        struct apn_bridge_port *port = 0;
        int port_no = 0;

        GET_ARGS(num_args);
        port_no = arglist[2];
        memset (bridge_name, 0, IFNAMSIZ + 1);
        GET_STRING(arglist[1], bridge_name, IFNAMSIZ);
        GET_BRIDGE();

        if ( ((port = br_get_port (br, port_no)) == NULL))
          {
            BDEBUG("Invalid port  %ld\n", port_no);
            return -EINVAL;
          }

        if (port->port_type != CUST_EDGE_PORT)
          {
            BDEBUG("Invalid port type %ld\n", port_no);
            return -EINVAL;
          }

        return br_vlan_set_pro_edge_untagged_vid (bridge_name, port_no,
                                                  arglist[3], arglist[4]);
      }

    case APNBR_GET_IFINDEX_BY_MAC_VID:
      {
        /* arglist contains (cmd, bridge_name, if_index, mac ptr, */
        /*                   vid)                         */

        unsigned long arglist[6];
        char bridge_name[IFNAMSIZ];
        struct apn_bridge *br = 0;
        struct apn_bridge_port *port = 0;
        unsigned short vid;
        unsigned short svid;
        int *ifindex;
        char buf[IFHWADDRLEN];

        GET_ARGS(6);
        GET_STRING(arglist[1], bridge_name, IFNAMSIZ);
        GET_BRIDGE();
        vid = arglist[4];
        svid = arglist[5];

        BDEBUG("Bridge %s selected, executing get ifindex by mac and vid , "
               "vid %d\n", bridge_name, arglist[4]);

        if (svid == 0)
          svid = vid;

        if (copy_from_user (buf, (void *)arglist[3], IFHWADDRLEN))
          {
            return -EFAULT;
          }

        BDEBUG("Adding static mac (%.02x:%.02x:%.02x:%.02x:%.02x:%.02x)\n",
               buf[0], buf[1], buf[2], buf[3], buf[4], buf[5]);


        return br_fdb_get_index_by_mac_vid (br, buf, vid, svid, 
                                            (int *)arglist[2]);
      }

    default:
      BDEBUG("Error: Executing default condition\n");
      break;
    }

  BDEBUG("Unknown command encountered?\n");
  return -EOPNOTSUPP;
}
