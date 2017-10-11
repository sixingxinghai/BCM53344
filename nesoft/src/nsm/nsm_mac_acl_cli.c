#include "pal.h"
#ifdef HAVE_L2LERN
#include "lib.h"
#ifdef HAVE_HAL
#include "hal_incl.h"
#endif /* HAVE_HAL */



#include "nsmd.h"
#include "nsm_message.h"
#include "nsm_server.h"
#include "nsm_interface.h"
#include "nsm_acl.h"
#include "nsm_mac_acl_cli.h"
#include "nsm_mac_acl_api.h"

#define MAC_ACL_STR                             \
  "MAC access list",                            \
  "MAC access list",                            \
    "extended mac ACL <2000-2699>"

#define MAC_ACL_DENY_PERMIT_STR                 \
  "Specify packets to reject",                  \
    "Specify packets to permit"


#define CLI_GET_ACL_FILTER_TYPE(T,STR)          \
  do {                                          \
    /* Check of mac filter type. */             \
    if (pal_strncmp ((STR), "p", 1) == 0)       \
      (T) = ACL_FILTER_PERMIT;                  \
    else if (pal_strncmp ((STR), "d", 1) == 0)  \
      (T) = ACL_FILTER_DENY;                    \
    else                                        \
      return LIB_API_SET_ERR_INVALID_VALUE;     \
  } while (0)

CLI (mac_acl_host_host,
     mac_acl_host_host_cmd,
     "mac acl <2000-2699> (deny|permit) MAC MASK MAC MASK <1-8>",
     MAC_ACL_STR,
     MAC_ACL_DENY_PERMIT_STR,
     "Source host's MAC address in HHHH.HHHH.HHHH format",
     "Source wildcard in HHHH.HHHH.HHHH format",
     "Destination host's MAC address in HHHH.HHHH.HHHH format",
     "Destination wildcard in HHHH.HHHH.HHHH format",
     "Specify packet format, (1:Ethernet II/ 2:802.3/ 4:SNMP/ 8:LLC)")
{
  struct nsm_master *nm = cli->vr->proto;
  enum filter_type type;
  int acl_type = NSM_ACL_TYPE_MAC;
  int packet_format;
  int ret;


  CLI_GET_ACL_FILTER_TYPE (type, argv[1]);

  /* Check the range */
  CLI_GET_INTEGER_RANGE ("ether type", packet_format, argv[6], 1, 8);
  if (! ((packet_format == NSM_ACL_FILTER_FMT_ETH_II)  ||
         (packet_format == NSM_ACL_FILTER_FMT_802_3)   ||
         (packet_format == NSM_ACL_FILTER_FMT_SNAP)    ||
         (packet_format == NSM_ACL_FILTER_FMT_LLC)))
    {
      cli_out (cli, "%% The packet format is wrong\n");
      return CLI_ERROR;
    }

  ret = mac_acl_extended_set(nm->mac_acl, argv[0], type,
                             argv[2], argv[3], argv[4], argv[5],
                             1,1,acl_type, packet_format);

  if (ret < 0)
   {
     if (ret == LIB_API_SET_ERR_INVALID_VALUE)
       cli_out (cli, "%% Invalid Value\n");
     else if (ret == LIB_API_SET_ERR_MALFORMED_ADDRESS)
       cli_out (cli, "%% Invalid MAC Address\n");
     else if (ret == NSM_API_SET_ERR_MACACL_ATTACHED)
       cli_out (cli, "%% MAC ACL is attached to mac access group/vlan map\n");
     else 
       cli_out (cli, "%% Can't set mac acl\n");
     return CLI_ERROR;
   }
  return CLI_SUCCESS;
}

CLI (mac_acl_host_any,
     mac_acl_host_any_cmd,
     "mac acl <2000-2699> (deny|permit) MAC MASK any <1-8>",
     MAC_ACL_STR,
     MAC_ACL_DENY_PERMIT_STR,
     "Source host's MAC address in HHHH.HHHH.HHHH format",
     "Source wildcard in HHHH.HHHH.HHHH format",
     "Destination any",
     "Specify packet format, (1:Ethernet II/ 2:802.3/ 4:SNMP/ 8:LLC)")
{
  struct nsm_master *nm = cli->vr->proto;
  enum filter_type type;
  int acl_type = NSM_ACL_TYPE_MAC;
  int packet_format;
  int ret;


  CLI_GET_ACL_FILTER_TYPE (type, argv[1]);

  /* Check the range */
  CLI_GET_INTEGER_RANGE ("ether type", packet_format, argv[4], 1, 8);

  if (! ((packet_format == NSM_ACL_FILTER_FMT_ETH_II)  ||
         (packet_format == NSM_ACL_FILTER_FMT_802_3)   ||
         (packet_format == NSM_ACL_FILTER_FMT_SNAP)    ||
         (packet_format == NSM_ACL_FILTER_FMT_LLC)))
    {
      cli_out (cli, "%% The packet format is wrong\n");
      return CLI_ERROR;
    }

  if (!nm || argv[0] == NULL)
    {
      cli_out (cli, "%% address error\n");
      return CLI_ERROR;
    }

  ret = mac_acl_extended_set(nm->mac_acl, argv[0], type,
                             argv[2], argv[3], "0.0.0", "FFFF.FFFF.FFFF",
                             1,1,acl_type, packet_format);

  if (ret < 0)
   {
     if (ret == LIB_API_SET_ERR_INVALID_VALUE)
       cli_out (cli, "%% Invalid Value\n");
     else if (ret == LIB_API_SET_ERR_MALFORMED_ADDRESS)
       cli_out (cli, "%% Invalid MAC Address\n");
     else if (ret == NSM_API_SET_ERR_MACACL_ATTACHED)
       cli_out (cli, "%% MAC ACL is attached to mac access group/vlan map\n");
     else
       cli_out (cli, "%% Can't set mac acl\n");
     return CLI_ERROR;
   }


  return CLI_SUCCESS;

}

CLI (mac_acl_any_host,
     mac_acl_any_host_cmd,
     "mac acl <2000-2699> (deny|permit) any MAC MASK <1-8>",
     MAC_ACL_STR,
     MAC_ACL_DENY_PERMIT_STR,
     "Source any",
     "Destination host's MAC address in HHHH.HHHH.HHHH format",
     "Destination wildcard in HHHH.HHHH.HHHH format",
     "Specify packet format, (1:Ethernet II/ 2:802.3/ 4:SNMP/ 8:LLC)")
{
  struct nsm_master *nm = cli->vr->proto;
  enum filter_type type;
  int acl_type = NSM_ACL_TYPE_MAC;
  int packet_format;
  int ret;


  CLI_GET_ACL_FILTER_TYPE (type, argv[1]);

  /* Check the range */
  CLI_GET_INTEGER_RANGE ("ether type", packet_format, argv[4], 1, 8);

  if (! ((packet_format == NSM_ACL_FILTER_FMT_ETH_II)  ||
         (packet_format == NSM_ACL_FILTER_FMT_802_3)   ||
         (packet_format == NSM_ACL_FILTER_FMT_SNAP)    ||
         (packet_format == NSM_ACL_FILTER_FMT_LLC)))
    {
      cli_out (cli, "%% The packet format is wrong\n");
      return CLI_ERROR;
    }

  if (!nm || argv[0] == NULL)
    {
      cli_out (cli, "%% address error\n");
      return CLI_ERROR;
    }

  ret = mac_acl_extended_set(nm->mac_acl, argv[0], type,
                             "0.0.0", "FFFF.FFFF.FFFF", argv[2], argv[3],
                             1,1,acl_type, packet_format);

  if (ret < 0)
   {
     if (ret == LIB_API_SET_ERR_INVALID_VALUE)
       cli_out (cli, "%% Invalid Value\n");
     else if (ret == LIB_API_SET_ERR_MALFORMED_ADDRESS)
       cli_out (cli, "%% Invalid MAC Address\n");
     else if (ret == NSM_API_SET_ERR_MACACL_ATTACHED)
       cli_out (cli, "%% MAC ACL is attached to mac access group/vlan map\n");
     else
       cli_out (cli, "%% Can't set mac acl\n");
     return CLI_ERROR;
   }


  return CLI_SUCCESS;

}

CLI (no_mac_acl_host_host,
     no_mac_acl_host_host_cmd,
     "no mac acl <2000-2699> (deny|permit) MAC MASK MAC MASK <1-8>",
     CLI_NO_STR,
     MAC_ACL_STR,
     MAC_ACL_DENY_PERMIT_STR,
     "Source host's MAC address in HHHH.HHHH.HHHH format",
     "Source wildcard in HHHH.HHHH.HHHH format",
     "Destination host's MAC address in HHHH.HHHH.HHHH format",
     "Destination wildcard in HHHH.HHHH.HHHH format",
     "Specify packet format, (1:Ethernet II/ 2:802.3/ 4:SNMP/ 8:LLC)")
{
  struct nsm_master *nm = cli->vr->proto;
  enum filter_type type;
  int acl_type = NSM_ACL_TYPE_MAC;
  int packet_format;
  int ret;


  CLI_GET_ACL_FILTER_TYPE (type, argv[1]);

  /* Check the range */
  CLI_GET_INTEGER_RANGE ("ether type", packet_format, argv[6], 1, 8);

  if (! ((packet_format == NSM_ACL_FILTER_FMT_ETH_II)  ||
         (packet_format == NSM_ACL_FILTER_FMT_802_3)   ||
         (packet_format == NSM_ACL_FILTER_FMT_SNAP)    ||
         (packet_format == NSM_ACL_FILTER_FMT_LLC)))
    {
      cli_out (cli, "%% The packet format is wrong\n");
      return CLI_ERROR;
    }

  if (!nm || argv[0] == NULL)
    {
      cli_out (cli, "%% address error\n");
      return CLI_ERROR;
    }

  ret = mac_acl_extended_set(nm->mac_acl, argv[0], type,
                             argv[2], argv[3], argv[4], argv[5],
                             1,0,acl_type, packet_format);

  if (ret < 0)
   {
     if (ret == LIB_API_SET_ERR_INVALID_VALUE)
       cli_out (cli, "%% Invalid Value\n");
     else if (ret == LIB_API_SET_ERR_MALFORMED_ADDRESS)
       cli_out (cli, "%% Invalid MAC Address\n");
     else if (ret == NSM_API_SET_ERR_MACACL_ATTACHED)
       cli_out (cli, "%% MAC ACL is attached to mac access group/vlan map\n");
     else
       cli_out (cli, "%% Can't set mac acl\n");
     return CLI_ERROR;
   }


  return CLI_SUCCESS;

}
CLI (no_mac_acl_host_any,
     no_mac_acl_host_any_cmd,
     "no mac acl <2000-2699> (deny|permit) MAC MASK any <1-8>",
     CLI_NO_STR,
     MAC_ACL_STR,
     MAC_ACL_DENY_PERMIT_STR,
     "Source host's MAC address in HHHH.HHHH.HHHH format",
     "Source wildcard in HHHH.HHHH.HHHH format",
     "Destination any",
     "Specify packet format, (1:Ethernet II/ 2:802.3/ 4:SNMP/ 8:LLC)")
{
  struct nsm_master *nm = cli->vr->proto;
  enum filter_type type;
  int acl_type = NSM_ACL_TYPE_MAC;
  int packet_format;
  int ret;


  CLI_GET_ACL_FILTER_TYPE (type, argv[1]);

  /* Check the range */
  CLI_GET_INTEGER_RANGE ("ether type", packet_format, argv[4], 1, 8);

  if (! ((packet_format == NSM_ACL_FILTER_FMT_ETH_II)  ||
         (packet_format == NSM_ACL_FILTER_FMT_802_3)   ||
         (packet_format == NSM_ACL_FILTER_FMT_SNAP)    ||
         (packet_format == NSM_ACL_FILTER_FMT_LLC)))
    {
      cli_out (cli, "%% The packet format is wrong\n");
      return CLI_ERROR;
    }

  if (!nm || argv[0] == NULL)
    {
      cli_out (cli, "%% address error\n");
      return CLI_ERROR;
    }

  ret = mac_acl_extended_set(nm->mac_acl, argv[0], type,
                             argv[2], argv[3], "0.0.0", "FFFF.FFFF.FFFF",
                             1,0,acl_type, packet_format);

  if (ret < 0)
   {
     if (ret == LIB_API_SET_ERR_INVALID_VALUE)
       cli_out (cli, "%% Invalid Value\n");
     else if (ret == LIB_API_SET_ERR_MALFORMED_ADDRESS)
       cli_out (cli, "%% Invalid MAC Address\n");
     else if (ret == NSM_API_SET_ERR_MACACL_ATTACHED)
       cli_out (cli, "%% MAC ACL is attached to mac access group/vlan map\n");
     else
       cli_out (cli, "%% Can't set mac acl\n");
     return CLI_ERROR;
   }


  return CLI_SUCCESS;

}
CLI (no_mac_acl_any_host,
     no_mac_acl_any_host_cmd,
     "no mac acl <2000-2699> (deny|permit) any MAC MASK <1-8>",
     CLI_NO_STR,
     MAC_ACL_STR,
     MAC_ACL_DENY_PERMIT_STR,
     "Source any",
     "Destination host's MAC address in HHHH.HHHH.HHHH format",
     "Destination wildcard in HHHH.HHHH.HHHH format",
     "Specify packet format, (1:Ethernet II/ 2:802.3/ 4:SNMP/ 8:LLC)")
{
  struct nsm_master *nm = cli->vr->proto;
  enum filter_type type;
  int acl_type = NSM_ACL_TYPE_MAC;
  int packet_format;
  int ret;


  CLI_GET_ACL_FILTER_TYPE (type, argv[1]);

  /* Check the range */
  CLI_GET_INTEGER_RANGE ("ether type", packet_format, argv[4], 1, 8);

  if (! ((packet_format == NSM_ACL_FILTER_FMT_ETH_II)  ||
         (packet_format == NSM_ACL_FILTER_FMT_802_3)   ||
         (packet_format == NSM_ACL_FILTER_FMT_SNAP)    ||
         (packet_format == NSM_ACL_FILTER_FMT_LLC)))
    {
      cli_out (cli, "%% The packet format is wrong\n");
      return CLI_ERROR;
    }

  if (!nm || argv[0] == NULL)
    {
      cli_out (cli, "%% address error\n");
      return CLI_ERROR;
    }

  ret = mac_acl_extended_set(nm->mac_acl, argv[0], type,
                             "0.0.0", "FFFF.FFFF.FFFF", argv[2], argv[3],
                             1,0,acl_type, packet_format);

  if (ret < 0)
   {
     if (ret == LIB_API_SET_ERR_INVALID_VALUE)
       cli_out (cli, "%% Invalid Value\n");
     else if (ret == LIB_API_SET_ERR_MALFORMED_ADDRESS)
       cli_out (cli, "%% Invalid MAC Address\n");
     else if (ret == NSM_API_SET_ERR_MACACL_ATTACHED)
       cli_out (cli, "%% MAC ACL is attached to mac access group/vlan map\n");
     else
       cli_out (cli, "%% Can't set mac acl\n");
     return CLI_ERROR;
   }


  return CLI_SUCCESS;

}

CLI (show_mac_acl,
     show_mac_acl_cmd,
     "show mac-acl",
     CLI_SHOW_STR,
     "List MAC access lists")
{
  struct nsm_master *nm = cli->vr->proto;

  mac_acl_show (cli, nm->mac_acl);
  return CLI_SUCCESS;

}

CLI (show_mac_acl_name,
     show_mac_acl_name_cmd,
     "show mac-acl <2000-2699>",
     CLI_SHOW_STR,
     "List MAC access lists",
     "MAC ACL NAME")
{
  struct nsm_master *nm = cli->vr->proto;

  mac_acl_show_name (cli, nm->mac_acl, argv[0]);
  return CLI_SUCCESS;

}


CLI (nsm_mac_access_group,
     nsm_mac_access_group_cmd,
     "mac access-group WORD in",
     "Configure MAC parameters",
     "Configure MAC Access group",
     "Name for the MAC Access List",
     "ingress direction")
{
  int ret;
  struct nsm_master *nm = cli->vr->proto;
  struct nsm_if *zif;
  struct interface *ifp;
  struct nsm_vlan_port *vlan_port;
  struct nsm_bridge_port *br_port;
  struct nsm_bridge *bridge;
  struct mac_acl *access = NULL;

  int action,dir;

  ifp = cli->index;
  zif = (struct nsm_if *)ifp->info;

  if (! zif)
    return CLI_ERROR;
 
  br_port = zif->switchport;

  if (! br_port)
    return CLI_ERROR;

  vlan_port = &br_port->vlan_port;

  bridge = br_port->bridge;

  if (! NSM_INTF_TYPE_L2(ifp) || ! bridge)
    {
      cli_out (cli, "%% Interface is not a switchport or not attached to the bridge\n");
      return CLI_ERROR;
    }

  access = mac_acl_lookup (nm->mac_acl, argv[0]);

  if (access == NULL)
    {
      cli_out (cli, "%% Specified MAC ACL not found\n");
      return CLI_ERROR;
    }
#ifdef HAVE_VLAN
  ret = check_interface_vlan_filter(ifp);
#endif
  if (ret == 0)
    {
      dir = NSM_ACL_DIRECTION_INGRESS;  
      action = NSM_ACL_ACTION_ADD;
      ret = nsm_mac_hal_set_access_grp(access, ifp->ifindex, action, dir);
      if (ret < 0)
        {
          cli_out (cli, "%% Failed to set the MAC access group\n");
          return CLI_ERROR;
        }
      zif->mac_acl = access;
      mac_acl_lock (access);
    }
  else
    {
      cli_out (cli, "%%Vlan access map is applied. Cant set MAC access group\n");
      return CLI_ERROR;
    }

  return CLI_SUCCESS;
}

CLI (nsm_no_mac_access_group,
     nsm_no_mac_access_group_cmd,
     "no mac access-group WORD",
     CLI_NO_STR,
     "Configure MAC parameters",
     "Configure MAC Access group",
     "Name for the MAC Access List")
{
  struct nsm_master *nm = cli->vr->proto;
  struct nsm_if *zif;
  struct interface *ifp;
  struct mac_acl *access = NULL;
  int action;
  int dir;

  ifp = cli->index;
  zif = (struct nsm_if *)ifp->info;

  if (! zif)
    return CLI_ERROR;

  if (! NSM_INTF_TYPE_L2(ifp))
    {
      cli_out (cli, " %%Interface is not a switchport\n");
      return CLI_ERROR;
    }

  access = mac_acl_lookup (nm->mac_acl, argv[0]);

  if (access == NULL)
    { 
      cli_out (cli, " %%Specified MAC ACL is not present\n");
      return CLI_ERROR;
    }

  if (pal_strcmp (zif->mac_acl->name, argv[0]) == 0)
    {
      zif->mac_acl = NULL;
    }
  else
    {
      cli_out (cli, " %%Wrong MAC ACL specified\n");
      return CLI_ERROR;
    }

  action = NSM_ACL_ACTION_DELETE;
  dir = NSM_ACL_DIRECTION_INGRESS;

  /* Attach code to send the information to hsl*/
  nsm_mac_hal_set_access_grp(access, ifp->ifindex, action, dir);
  
  mac_acl_unlock (access);
  return CLI_SUCCESS;
}

CLI (show_mac_access_grp,
     show_mac_access_grp_cmd,
     "show mac-access-group",
     CLI_SHOW_STR,
     "List MAC access groups")
{
  struct nsm_master *nm = cli->vr->proto;

  mac_access_grp_show (cli, nm);
  return CLI_SUCCESS;

}

int
nsm_mac_acl_config_write (struct cli *cli)
{
  struct nsm_master *nm = cli->vr->proto;
  struct nsm_mac_acl_master *master = nm->mac_acl;
  struct mac_acl *access = NULL;
  char src_str[100];
  char src_mask_str[100];
  char dst_str[100];
  char dst_mask_str[100];


  if (master == NULL)
    return 0;

  for (access = master->num.head; access; access = access->next)
    {
      if (! access->name)
        continue;

      if (access->acl_type == NSM_ACL_TYPE_MAC)
        {
          pal_snprintf (src_str, 100,
                        "%02x%02x.%02x%02x.%02x%02x",
                        access->a.mac[0], access->a.mac[1],
                        access->a.mac[2], access->a.mac[3],
                        access->a.mac[4], access->a.mac[5]);
          pal_snprintf (src_mask_str, 100,
                        "%02x%02x.%02x%02x.%02x%02x",
                        access->a_mask.mac[0], access->a_mask.mac[1],
                        access->a_mask.mac[2], access->a_mask.mac[3],
                        access->a_mask.mac[4], access->a_mask.mac[5]);
          pal_snprintf (dst_str, 100,
                        "%02x%02x.%02x%02x.%02x%02x",
                        access->m.mac[0], access->m.mac[1],
                        access->m.mac[2], access->m.mac[3],
                        access->m.mac[4], access->m.mac[5]);
          pal_snprintf (dst_mask_str, 100,
                        "%02x%02x.%02x%02x.%02x%02x",
                        access->m_mask.mac[0], access->m_mask.mac[1],
                        access->m_mask.mac[2], access->m_mask.mac[3],
                        access->m_mask.mac[4], access->m_mask.mac[5]);

          if (compare_mac_filter (&access->a.mac[0], 0) == 0 &&
              compare_mac_filter (&access->a_mask.mac[0], 0xff) == 0)
            {
              cli_out (cli, "mac acl %s %s any %s %s %d\n",
                       access->name, access->type == ACL_FILTER_DENY ? "deny" : "permit",
                       dst_str, dst_mask_str, access->packet_format);
            }
          else if (compare_mac_filter (&access->m.mac[0], 0) == 0 &&
                   compare_mac_filter (&access->m_mask.mac[0], 0xff) == 0)
            {
              cli_out (cli, "mac acl %s %s %s %s any %d\n",
                       access->name, access->type == ACL_FILTER_DENY ? "deny" : "permit",
                       src_str, src_mask_str, access->packet_format);
            }
          else
            {
              cli_out (cli, "mac acl %s %s %s %s %s %s %d\n",
                       access->name, access->type == ACL_FILTER_DENY ? "deny" : "permit",
                       src_str, src_mask_str, dst_str, dst_mask_str, access->packet_format);
            }
        }
    }
  cli_out (cli, "!\n");
  return 0;
}



void
nsm_mac_acl_cli_init (struct cli_tree *ctree)
{
  cli_install_config (ctree, MAC_ACL_MODE, nsm_mac_acl_config_write);
  cli_install (ctree, CONFIG_MODE, &mac_acl_host_host_cmd);
  cli_install (ctree, CONFIG_MODE, &no_mac_acl_host_host_cmd);
  cli_install (ctree, CONFIG_MODE, &mac_acl_host_any_cmd);
  cli_install (ctree, CONFIG_MODE, &no_mac_acl_host_any_cmd);
  cli_install (ctree, CONFIG_MODE, &mac_acl_any_host_cmd);
  cli_install (ctree, CONFIG_MODE, &no_mac_acl_any_host_cmd);
  cli_install (ctree, EXEC_MODE, &show_mac_acl_cmd);
  cli_install (ctree, EXEC_MODE, &show_mac_acl_name_cmd);
  cli_install (ctree, EXEC_MODE, &show_mac_access_grp_cmd);
  cli_install_imi (ctree, INTERFACE_MODE, PM_NSM, PRIVILEGE_NORMAL, 0, &nsm_mac_access_group_cmd);
  cli_install_imi (ctree, INTERFACE_MODE, PM_NSM, PRIVILEGE_NORMAL, 0, &nsm_no_mac_access_group_cmd);
}
#endif /* HAVE_L2LERN */
