/* Copyright (C) 2004  All Rights Reserved */

#include "pal.h"
#include "lib.h"
#include "avl_tree.h"
#include "hash.h"
#include "hal_incl.h"
#if ! defined HAVE_MCAST_IPV4 && defined HAVE_IGMP_SNOOP
#include "igmp.h"
#endif /* ! HAVE_MCAST_IPV4 && HAVE_IGMP_SNOOP */
#if ((! defined HAVE_MCAST_IPV6) && (defined HAVE_MLD_SNOOP))
#include "mld.h"
#endif
#ifndef HAVE_CUSTOM1

#include "nsmd.h"
#include "nsm_message.h"
#include "nsm_server.h"
#include "nsm_api.h"

#ifdef HAVE_L2
#include "nsm_bridge.h"
#ifdef HAVE_VLAN
#include "nsm_vlan.h"
#include "nsm_vlan_cli.h"
#ifdef HAVE_PVLAN
#include "nsm_pvlan.h"
#include "nsm_pvlan_cli.h"
#endif /* HAVE_PVLAN */
#ifdef HAVE_VLAN_STACK
#include "nsm_vlan_stack.h"
#endif /* HAVE_VLAN_STACK */
#ifdef HAVE_VLAN_CLASS
#include "nsm_vlanclassifier.h"
#endif /* HAVE_VLAN_CLASS */
#endif /* HAVE_VLAN */
#ifdef HAVE_RATE_LIMIT
#include "nsm_ratelimit.h"
#endif /* HAVE_RATE_LIMIT */
#ifdef HAVE_DCB
#include "nsm_dcb_cli.h"
#endif /* HAVE_DCB */
#endif /* HAVE_L2 */
#include "nsm_interface.h"
#include "nsm_bridge_cli.h"
#include "nsm_flowctrl.h"
#ifdef HAVE_IGMP_SNOOP
#include "nsm_l2_mcast.h"
#endif
#ifdef HAVE_PROVIDER_BRIDGE
#include "nsm_pro_vlan.h"
#endif /* HAVE_PROVIDER_BRIDGE */

#if defined HAVE_I_BEB || defined HAVE_B_BEB
#include "nsm_pbb_cli.h"
#endif /* defined HAVE_I_BEB || defined HAVE_B_BEB */

#ifdef HAVE_G8032
#include "g8032_api.h"
#endif /*HAVE_G8032*/


#ifdef HAVE_PBB_TE
#include "nsm_pbb_te_cli.h"
#endif /* HAVE_PBB_TE */

static void
nsm_bridge_display_error_message (struct cli *cli, s_int32_t result)
{
  switch (result)
    {
#ifdef HAVE_PROVIDER_BRIDGE
      case NSM_ERR_BW_PROFILE_PER_UNI_CONFIGURED:
        cli_out (cli, "%% BW Profile per UNI Configured\n");
        break;
      case NSM_ERR_BW_PROFILE_PER_COS_CONFIGURED:
        cli_out (cli, "%% BW Profile per UNI Configured\n");
        break;
      case NSM_ERR_BW_PROFILE_PER_EVC_CONFIGURED:
        cli_out (cli, "%% BW Profile per EVC Configured\n");
        break;
      case NSM_ERR_COS_ALREADY_CONFIGURED:
        cli_out (cli, "%% BW Profile Already Configured\n");
        break;
      case NSM_ERR_BW_PROFILE_NOT_CONFIGURED:
        cli_out (cli, "%% BW Profile Not Configured\n");
        break;
      case NSM_ERR_BW_PROFILE_CBS_NOT_CONFIGURED:
        cli_out (cli, "%% BW Profile for CBS Not Configured\n");
        break;
      case NSM_ERR_BW_PROFILE_EBS_NOT_CONFIGURED:
        cli_out (cli, "%% BW Profile for EBS Not Configured\n");
        break;
      case NSM_ERR_BW_PROFILE_EIR_NOT_CONFIGURED:
        cli_out (cli, "%% BW Profile for EIR Not Configured\n");
        break;
      case NSM_ERR_BW_PROFILE_ACTIVE:
        cli_out (cli, "%% Bandwidth Profile is Active\n");
        break;
      case NSM_ERR_BW_PROFILE_NOT_ACTIVE:
        cli_out (cli, "%% Bandwidth Profile is not active\n");
        break;
      case NSM_ERR_CROSSED_MAX_EVC:
        cli_out (cli, "%% the EVC Count is more than the MAX EVC\n");
        break;
      case NSM_ERR_MAX_EVC_LESS:
        cli_out (cli, "%% MAX EVC count configured is less than the EVC count existing.\n");
        break;
      case NSM_ERR_MAX_EVC_CONFIGURED:
        cli_out (cli, "%% MAX EVC is already configured \n");
        break;
      case NSM_ERR_INSTANCE_ID_DO_NOT_MATCH:
        cli_out (cli, "%% Instance ID did not match the evc-id\n");
        break;
      case NSM_PRO_VLAN_ERR_INVALID_MODE:
        cli_out (cli, "%% Configured on invalid mode of provider edge bridge\n");
        break;
      case NSM_L2_ERR_INVALID_ARG:
        cli_out (cli, "%% Bridge port not found \n");
        break;
      case NSM_ERR_UNI_MODE_CONFIGURED:
        cli_out (cli, "%% UNI mode already configured\n");
        break;
      case NSM_L2_ERR_MEM:
        cli_out (cli, "%% memory not allocated\n");
        break;
#endif
      case NSM_BRIDGE_ERR_NOTFOUND:
        cli_out (cli, "%% Bridge not found\n");
        break;
      case NSM_BRIDGE_ERR_INVALID_ARG:
        cli_out (cli, "%% Bridge is a provider bridge. uni type mode cannot be configured\n");
        break;

      default:
        cli_out (cli, "%% Error in the configuration.\n");
        break;

    }
  return;
}
#ifdef HAVE_STPD
#ifdef HAVE_VLAN
CLI (nsm_bridge_protocol_ieee_vlan,
     nsm_bridge_protocol_ieee_vlan_cmd,
     "bridge <1-32> protocol ieee (vlan-bridge|)",
     NSM_BRIDGE_STR,
     NSM_BRIDGE_NAME_STR,
     "Spanning-tree protocol",
     "IEEE 801.1Q spanning-tree protocol",
     "VLAN aware bridge")
{
  int ret;
  struct nsm_master *nm = cli->vr->proto;
  struct nsm_bridge_master *master = nsm_bridge_get_master (nm);
  int brno;

  if (! master)
    {
      cli_out (cli, "%% Bridge master not configured\n");
      return CLI_ERROR;
    }

  CLI_GET_INTEGER_RANGE ("Max bridge groups", brno, argv[0], 1, 32);

  if (argc > 1)
    ret = nsm_bridge_add (master, NSM_BRIDGE_TYPE_STP_VLANAWARE,
                          argv[0], PAL_FALSE, TOPOLOGY_NONE, PAL_FALSE,
                          PAL_FALSE, NULL);
  else
    ret = nsm_bridge_add (master, NSM_BRIDGE_TYPE_STP, 
                          argv[0], PAL_FALSE, TOPOLOGY_NONE, PAL_FALSE,
                          PAL_FALSE, NULL);

  switch (ret)
    {
    case NSM_BRIDGE_ERR_EXISTS:
      cli_out (cli, "%% Bridge %s already exists\n", argv[0]);
      return CLI_ERROR;
    case NSM_BRIDGE_ERR_NOTFOUND:
    case NSM_BRIDGE_ERR_MEM:
      cli_out (cli, "%% Error adding bridge\n");
      return CLI_ERROR;
    default:
      return CLI_SUCCESS;
    }
}
#else
CLI (nsm_bridge_protocol_ieee,
     nsm_bridge_protocol_ieee_cmd,
     "bridge <1-32> protocol ieee",
     NSM_BRIDGE_STR,
     NSM_BRIDGE_NAME_STR,
     "Spanning-tree protocol",
     "IEEE 802.1D spanning-tree protocol")
{
  struct nsm_master *nm = cli->vr->proto;
  struct nsm_bridge_master *master = nsm_bridge_get_master (nm);
  int ret;
  int brno;

  if (! master)
    {
      cli_out (cli, "%% Bridge master not configured\n");
      return CLI_ERROR;
    }

  CLI_GET_INTEGER_RANGE ("Max bridge groups", brno, argv[0], 1, 32);

  /* Add bridge. */
  ret = nsm_bridge_add (master, NSM_BRIDGE_TYPE_STP, 
                        argv[0], PAL_FALSE, TOPOLOGY_NONE, PAL_FALSE);

  switch (ret)
    {
    case NSM_BRIDGE_ERR_EXISTS:
      cli_out (cli, "%% Bridge %s already exists\n", argv[0]);
      return CLI_ERROR;
    case NSM_BRIDGE_ERR_NOTFOUND:
    case NSM_BRIDGE_ERR_MEM:
      cli_out (cli, "%% Error adding bridge\n");
      return CLI_ERROR;
    default:
      return CLI_SUCCESS;
    }
}
#endif /* ! HAVE_VLAN. */
#endif /* HAVE_STPD. */

#ifdef HAVE_RSTPD
#ifdef HAVE_VLAN
CLI (nsm_bridge_protocol_rstp_vlan,
     nsm_bridge_protocol_rstp_vlan_cmd,
     "bridge <1-32> protocol rstp (vlan-bridge|)(ring|)",
     NSM_BRIDGE_STR,
     NSM_BRIDGE_NAME_STR,
     "Spanning-tree protocol",
     "IEEE 801.1w rapid spanning-tree protocol",
     "VLAN aware bridge",
     "Enable Rapid Ring spanning-tree")
{
  struct nsm_master *nm = cli->vr->proto;
  struct nsm_bridge_master *master = nsm_bridge_get_master (nm);
  int ret;
  int brno;

  if (! master)
    {
      cli_out (cli, "%% Bridge master not configured\n");
      return CLI_ERROR;
    }

  CLI_GET_INTEGER_RANGE ("Max bridge groups", brno, argv[0], 1, 32);

  ret = NSM_BRIDGE_ERR_GENERAL;

  if (argc > 2)
    ret = nsm_bridge_add (master, NSM_BRIDGE_TYPE_RSTP_VLANAWARE,
                          argv[0], PAL_FALSE, TOPOLOGY_RING, PAL_FALSE,
                          PAL_FALSE, NULL);
  else if (argc >1)
    {
      if (! pal_strncmp (argv[1], "v", 1))
        ret = nsm_bridge_add (master, NSM_BRIDGE_TYPE_RSTP_VLANAWARE,
                              argv[0], PAL_FALSE, TOPOLOGY_NONE, PAL_FALSE,
                              PAL_FALSE, NULL);
      else if (! pal_strncmp (argv[1], "r", 1))
        ret = nsm_bridge_add (master, NSM_BRIDGE_TYPE_RSTP,
                              argv[0], PAL_FALSE, TOPOLOGY_RING, PAL_FALSE,
                              PAL_FALSE, NULL);
    }
   else
     ret = nsm_bridge_add (master, NSM_BRIDGE_TYPE_RSTP,
                           argv[0], PAL_FALSE, TOPOLOGY_NONE, PAL_FALSE,
                           PAL_FALSE, NULL);

  switch (ret)
    {
    case NSM_BRIDGE_ERR_EXISTS:
      cli_out (cli, "%% Bridge %s already exists\n", argv[0]);
      return CLI_ERROR;
    case NSM_BRIDGE_ERR_NOTFOUND:
    case NSM_BRIDGE_ERR_MEM:
      cli_out (cli, "%% Error adding bridge\n");
      return CLI_ERROR;
    default:
      return CLI_SUCCESS;
    }
}
#else
CLI (nsm_bridge_protocol_rstp,
     nsm_bridge_protocol_rstp_cmd,
     "bridge <1-32> protocol rstp (ring|)",
     NSM_BRIDGE_STR,
     NSM_BRIDGE_NAME_STR,
     "Spanning-tree protocol",
     "IEEE 802.1w rapid spanning-tree protocol",
     "Enable Rapid Ring spanning-tree")
{
  struct nsm_master *nm = cli->vr->proto;
  struct nsm_bridge_master *master = nsm_bridge_get_master (nm);
  int ret;
  int brno;

  if (! master)
    {
      cli_out (cli, "%% Bridge master not configured\n");
      return CLI_ERROR;
    }

  CLI_GET_INTEGER_RANGE ("Max bridge groups", brno, argv[0], 1, 32);

  /* Add bridge. */
  if (argc > 1)
    ret = nsm_bridge_add (master, NSM_BRIDGE_TYPE_RSTP, argv[0],
                           PAL_FALSE, TOPOLOGY_RING, PAL_FALSE
                           PAL_FALSE, NULL);
  else
    ret = nsm_bridge_add (master, NSM_BRIDGE_TYPE_RSTP, argv[0],
                           PAL_FALSE, TOPOLOGY_NONE, PAL_FALSE);

  switch (ret)
    {
    case NSM_BRIDGE_ERR_EXISTS:
      cli_out (cli, "%% Bridge %s already exists\n", argv[0]);
      return CLI_ERROR;
    case NSM_BRIDGE_ERR_NOTFOUND:
    case NSM_BRIDGE_ERR_MEM:
      cli_out (cli, "%% Error adding bridge\n");
      return CLI_ERROR;
    default:
      return CLI_SUCCESS;
    }

  return CLI_SUCCESS;
}
#endif /* ! HAVE_VLAN. */
#endif /* HAVE_RSTPD. */

#if defined(HAVE_MSTPD) && defined(HAVE_VLAN)
CLI (nsm_bridge_protocol_mstp,
     nsm_bridge_protocol_mstp_cmd,
     "bridge <1-32> protocol mstp (ring|)",
     NSM_BRIDGE_STR,
     NSM_BRIDGE_NAME_STR,
     "Spanning-tree protocol",
     "IEEE 802.1s multiple spanning-tree protocol",
     "Enable Rapid Ring spanning-tree")
{
  struct nsm_master *nm = cli->vr->proto;
  struct nsm_bridge_master *master = nsm_bridge_get_master (nm);
  int ret;
  int brno;

  if (! master)
    {
      cli_out (cli, "%% Bridge master not configured\n");
      return CLI_ERROR;
    }

  CLI_GET_INTEGER_RANGE ("Max bridge groups", brno, argv[0], 1, 32);

  /* Add bridge. */
  if (argc > 1)
    ret = nsm_bridge_add (master, NSM_BRIDGE_TYPE_MSTP, 
                          argv[0], PAL_FALSE, TOPOLOGY_RING, PAL_FALSE,
                          PAL_FALSE, NULL);
  else
    ret = nsm_bridge_add (master, NSM_BRIDGE_TYPE_MSTP,
                          argv[0], PAL_FALSE, TOPOLOGY_NONE, PAL_FALSE,
                          PAL_FALSE, NULL);

  switch (ret)
    {
    case NSM_BRIDGE_ERR_EXISTS:
      cli_out (cli, "%% Bridge %s already exists\n", argv[0]);
      return CLI_ERROR;
    case NSM_BRIDGE_ERR_NOTFOUND:
    case NSM_BRIDGE_ERR_MEM:
      cli_out (cli, "%% Error adding bridge\n");
      return CLI_ERROR;
    case NSM_ERR_BRIDGE_LIMIT:
      cli_out (cli, "%% Cannot add bridge - cannot add more than 32 bridges\n");
      return CLI_ERROR;
    default:
      return CLI_SUCCESS;
    }
}

#endif /* HAVE_MSTPD && HAVE_VLAN */

#if defined (HAVE_RPVST_PLUS) && defined (HAVE_VLAN) && defined (HAVE_MSTPD)
CLI (nsm_bridge_protocol_rpvst_plus,
     nsm_bridge_protocol_rpvst_plus_cmd,
     "bridge <1-32> protocol rpvst+",
     NSM_BRIDGE_STR,
     NSM_BRIDGE_NAME_STR,
     "Spanning-tree protocol",
     "IEEE 801.1w rapid per vlan spanning-tree protocol")
{
  struct nsm_master *nm = cli->vr->proto;
  struct nsm_bridge_master *master = nsm_bridge_get_master (nm);
  int ret;
  int brno;

  if (! master)
    {
      cli_out (cli, "%% Bridge master not configured\n");
      return CLI_ERROR;
    }

  CLI_GET_INTEGER_RANGE ("Max bridge groups", brno, argv[0], 1, 32);

  ret = NSM_BRIDGE_ERR_GENERAL;

  ret = nsm_bridge_add (master, NSM_BRIDGE_TYPE_RPVST_PLUS,
                          argv[0], PAL_FALSE, TOPOLOGY_NONE, PAL_FALSE,
                          PAL_FALSE, NULL);

  switch (ret)
    {
    case NSM_BRIDGE_ERR_EXISTS:
      cli_out (cli, "%% Bridge %s already exists\n", argv[0]);
      return CLI_ERROR;
    case NSM_BRIDGE_ERR_NOTFOUND:
    case NSM_BRIDGE_ERR_MEM:
      cli_out (cli, "%% Error adding bridge\n");
      return CLI_ERROR;
    default:
      return CLI_SUCCESS;
    }
}
#endif /* HAVE_RPVST_PLUS && HAVE_VLAN && HAVE_MSTPD */

#ifdef HAVE_PROVIDER_BRIDGE

CLI (nsm_bridge_protocol_pro_rstp,
     nsm_bridge_protocol_pro_rstp_cmd,
     "bridge <1-32> protocol provider-rstp (edge|)",
     NSM_BRIDGE_STR,
     NSM_BRIDGE_NAME_STR,
     "Spanning-tree protocol",
     "Provider IEEE 802.1w rapid spanning-tree protocol",
     "Configure as Edge bridge")
{
  struct nsm_master *nm = cli->vr->proto;
  struct nsm_bridge_master *master = nsm_bridge_get_master (nm);
  int ret;
  int brno;

  if (! master)
    {
      cli_out (cli, "%% Bridge master not configured\n");
      return CLI_ERROR;
    }

  CLI_GET_INTEGER_RANGE ("Max bridge groups", brno, argv[0], 1, 32);

  /* Add bridge. */
  ret = nsm_bridge_add (master, NSM_BRIDGE_TYPE_PROVIDER_RSTP,
                        argv[0], PAL_FALSE, TOPOLOGY_NONE,
                        (argc == 2) ? PAL_TRUE:PAL_FALSE,
                        PAL_FALSE, NULL);

  switch (ret)
    {
    case NSM_BRIDGE_ERR_EXISTS:
      cli_out (cli, "%% Bridge %s already exists\n", argv[0]);
      return CLI_ERROR;
    case NSM_BRIDGE_ERR_NOTFOUND:
    case NSM_BRIDGE_ERR_MEM:
      cli_out (cli, "%% Error adding bridge\n");
      return CLI_ERROR;
    default:
      return CLI_SUCCESS;
    }
}

CLI (nsm_bridge_protocol_pro_mstp,
     nsm_bridge_protocol_pro_mstp_cmd,
     "bridge <1-32> protocol provider-mstp (edge|)",
     NSM_BRIDGE_STR,
     NSM_BRIDGE_NAME_STR,
     "Spanning-tree protocol",
     "Provider IEEE 802.1s multiple spanning-tree protocol",
     "Configure as Edge bridge")
{
  struct nsm_master *nm = cli->vr->proto;
  struct nsm_bridge_master *master = nsm_bridge_get_master (nm);
  int ret;
  int brno;

  if (! master)
    {
      cli_out (cli, "%% Bridge master not configured\n");
      return CLI_ERROR;
    }

  CLI_GET_INTEGER_RANGE ("Max bridge groups", brno, argv[0], 1, 32);

  /* Add bridge. */
  ret = nsm_bridge_add (master, NSM_BRIDGE_TYPE_PROVIDER_MSTP,
                        argv[0], PAL_FALSE, TOPOLOGY_NONE,
                        (argc == 2) ? PAL_TRUE:PAL_FALSE,
                        PAL_FALSE, NULL);

  switch (ret)
    {
    case NSM_BRIDGE_ERR_EXISTS:
      cli_out (cli, "%% Bridge %s already exists\n", argv[0]);
      return CLI_ERROR;
    case NSM_BRIDGE_ERR_NOTFOUND:
    case NSM_BRIDGE_ERR_MEM:
      cli_out (cli, "%% Error adding bridge\n");
      return CLI_ERROR;
    default:
      return CLI_SUCCESS;
    }
}

#endif /* HAVE_PROVIDER_BRIDGE */

#ifdef HAVE_I_BEB
CLI (nsm_beb_bridge_protocol_pro_rstp,
     nsm_beb_bridge_protocol_pro_rstp_cmd,
     "bridge beb mac MAC <1-32> "
     "protocol provider-rstp",
     NSM_BRIDGE_STR,
     "Backbone edge bridge",
     "Specifies the MAC address",
     "MAC address in HHHH.HHHH.HHHH format",
     NSM_BRIDGE_NAME_STR,
     "Spanning-tree protocol",
     "Provider IEEE 802.1w rapid spanning-tree protocol")
{
  struct nsm_master *nm = cli->vr->proto;
  struct nsm_bridge_master *master = nsm_bridge_get_master (nm);
  int ret;
  int brno;
  u_char type;
  u_int8_t mac[6];
  if (! master)
  {
    cli_out (cli, "%% Bridge master not configured\n");
    return CLI_ERROR;
  }

  CLI_GET_INTEGER_RANGE ("Max bridge groups", brno, argv[1], 1, 32);
  type = NSM_BRIDGE_TYPE_PROVIDER_RSTP;

/* MAC address changes */

  if (! nsm_api_mac_address_is_valid (argv[0]))
    {
      cli_out (cli, "%% Unable to translate mac address %s\n", argv[0]);
      return CLI_ERROR;
    }

   NSM_RESERVED_ADDRESS_CHECK (argv[0]);
   NSM_READ_MAC_ADDRESS (argv[0], mac);

  /* Convert to network order */
  *(unsigned short *)&mac[0] = pal_hton16(*(unsigned short *)&mac[0]);
  *(unsigned short *)&mac[2] = pal_hton16(*(unsigned short *)&mac[2]);
  *(unsigned short *)&mac[4] = pal_hton16(*(unsigned short *)&mac[4]);

/* MAC Address changes */
  ret = nsm_bridge_add (master, type,
                          argv[1], PAL_FALSE, TOPOLOGY_NONE,
                          PAL_FALSE, PAL_TRUE,(u_char*) mac);
  switch (ret)
  {
    case NSM_BRIDGE_ERR_EXISTS:
      cli_out (cli, "%% Bridge %s already exists\n", argv[1]);
      return CLI_ERROR;
    case NSM_BRIDGE_ERR_NOTFOUND:
    case NSM_BRIDGE_ERR_MEM:
      cli_out (cli, "%% Error adding bridge\n");
      return CLI_ERROR;
    default:
      return CLI_SUCCESS;
  }
}

CLI (nsm_beb_bridge_protocol_pro_mstp,
     nsm_beb_bridge_protocol_pro_mstp_cmd,
     "bridge beb mac MAC <1-32> "
     "protocol provider-mstp",
     NSM_BRIDGE_STR,
     "Backbone edge bridge",
     "Specifies the MAC address",
     "MAC address in HHHH.HHHH.HHHH format",
     NSM_BRIDGE_NAME_STR,
     "Spanning-tree protocol",
     "Provider IEEE 802.1s multiple spanning-tree protocol")
{
  struct nsm_master *nm = cli->vr->proto;
  struct nsm_bridge_master *master = nsm_bridge_get_master (nm);
  int ret;
  int brno;
  u_int8_t mac[6];
  u_char type;
  if (! master)
  {
    cli_out (cli, "%% Bridge master not configured\n");
    return CLI_ERROR;
  }

  CLI_GET_INTEGER_RANGE ("Max bridge groups", brno, argv[1], 1, 32);
  type = NSM_BRIDGE_TYPE_PROVIDER_MSTP;

/* MAC address changes */
  if (! nsm_api_mac_address_is_valid (argv[0]))
    {
      cli_out (cli, "%% Unable to translate mac address %s\n", argv[0]);
      return CLI_ERROR;
    }
 
   NSM_RESERVED_ADDRESS_CHECK (argv[0]);
   NSM_READ_MAC_ADDRESS (argv[0], mac);
 
  /* Convert to network order */
  *(unsigned short *)&mac[0] = pal_hton16(*(unsigned short *)&mac[0]);
  *(unsigned short *)&mac[2] = pal_hton16(*(unsigned short *)&mac[2]);
  *(unsigned short *)&mac[4] = pal_hton16(*(unsigned short *)&mac[4]);

/* MAC Address changes */

  ret = nsm_bridge_add (master, type,
                          argv[1], PAL_FALSE, TOPOLOGY_NONE,
                          PAL_FALSE, PAL_TRUE, (u_char *)mac);
 
  switch (ret)
  {
    case NSM_BRIDGE_ERR_EXISTS:
      cli_out (cli, "%% Bridge %s already exists\n", argv[1]);
      return CLI_ERROR;
    case NSM_BRIDGE_ERR_NOTFOUND:
    case NSM_BRIDGE_ERR_MEM:
      cli_out (cli, "%% No more memory: Error adding bridge\n");
       return CLI_ERROR;
    default:
      return CLI_SUCCESS;
  }
}
 
#endif /*HAVE_I_BEB */

#ifdef HAVE_B_BEB
CLI (nsm_beb_bridge_protocol_backbone_rstp,
     nsm_beb_bridge_protocol_backbone_rstp_cmd,
     "bridge beb mac MAC backbone "
     "protocol provider-rstp",
     NSM_BRIDGE_STR,
     "Backbone edge bridge",
     "Specifies the MAC address",
     "MAC address in HHHH.HHHH.HHHH format",
     "Specify the bridge as backbone B-BEB",
     "Spanning-tree protocol",
     "Provider IEEE 802.1w rapid spanning-tree protocol")
{
  struct nsm_master *nm = cli->vr->proto;
  struct nsm_bridge_master *master = nsm_bridge_get_master (nm);
  int ret;
  u_char type;
  u_int8_t mac[6];
  if (! master)
  {
    cli_out (cli, "%% Bridge master not configured\n");
    return CLI_ERROR;
  }
  type = NSM_BRIDGE_TYPE_BACKBONE_RSTP;

/* MAC address changes */
  if (! nsm_api_mac_address_is_valid (argv[0]))
    {
      cli_out (cli, "%% Unable to translate mac address %s\n", argv[0]);
      return CLI_ERROR;
    }

   NSM_RESERVED_ADDRESS_CHECK (argv[0]);
   NSM_READ_MAC_ADDRESS (argv[0], mac); 

  /* Convert to network order */
  *(unsigned short *)&mac[0] = pal_hton16(*(unsigned short *)&mac[0]);
  *(unsigned short *)&mac[2] = pal_hton16(*(unsigned short *)&mac[2]);
  *(unsigned short *)&mac[4] = pal_hton16(*(unsigned short *)&mac[4]);

/* MAC Address changes */
  ret = nsm_bridge_add (master, type,
                          "backbone", PAL_FALSE, TOPOLOGY_NONE,
                          PAL_FALSE, PAL_TRUE,(u_char*) mac);
  switch (ret)
  {
    case NSM_BRIDGE_ERR_NOTFOUND:
    case NSM_BRIDGE_ERR_MEM:
      cli_out (cli, "%% Error adding bridge\n");
      return CLI_ERROR;
    case NSM_BRIDGE_ERR_BACKBONE_EXISTS:
      cli_out (cli, "%% Backbone bridge bridge already exists\n,");
      return CLI_ERROR;
    default:
      return CLI_SUCCESS;
  }
}

CLI (nsm_beb_bridge_protocol_backbone_mstp,
     nsm_beb_bridge_protocol_backbone_mstp_cmd,
     "bridge beb mac MAC backbone "
     "protocol provider-mstp",
     NSM_BRIDGE_STR,
     "Backbone edge bridge",
     "Specifies the MAC address",
     "MAC address in HHHH.HHHH.HHHH format",
     "Specify the bridge as backbone B-BEB",
     "Spanning-tree protocol",
     "Provider IEEE 802.1s multiple spanning-tree protocol")
{
  struct nsm_master *nm = cli->vr->proto;
  struct nsm_bridge_master *master = nsm_bridge_get_master (nm);
  int ret;
  u_int8_t mac[6];
  u_char type;
  if (! master)
  {
    cli_out (cli, "%% Bridge master not configured\n");
    return CLI_ERROR;
  }
  type = NSM_BRIDGE_TYPE_BACKBONE_MSTP;

/* MAC address changes */
  if (! nsm_api_mac_address_is_valid (argv[0]))
    {
      cli_out (cli, "%% Unable to translate mac address %s\n", argv[0]);
      return CLI_ERROR;
    }
 
   NSM_RESERVED_ADDRESS_CHECK (argv[0]);
   NSM_READ_MAC_ADDRESS (argv[0], mac); 

  /* Convert to network order */
  *(unsigned short *)&mac[0] = pal_hton16(*(unsigned short *)&mac[0]);
  *(unsigned short *)&mac[2] = pal_hton16(*(unsigned short *)&mac[2]);
  *(unsigned short *)&mac[4] = pal_hton16(*(unsigned short *)&mac[4]);

/* MAC Address changes */

  ret = nsm_bridge_add (master, type,
                        "backbone", PAL_FALSE, TOPOLOGY_NONE,
                        PAL_FALSE, PAL_TRUE, (u_char *)mac);
 
  switch (ret)
  {
    case NSM_BRIDGE_ERR_NOTFOUND:
    case NSM_BRIDGE_ERR_MEM:
      cli_out (cli, "%% No more memory: Error adding bridge\n");
       return CLI_ERROR;
    case NSM_BRIDGE_ERR_BACKBONE_EXISTS:
      cli_out (cli, "%% Backbone bridge bridge already exists\n,");
      return CLI_ERROR;
    default:
      return CLI_SUCCESS;
  }
}
 
#endif /*HAVE_B_BEB */

#if 0

CLI (bridge_decouple_vlan,
     bridge_decouple_vlan_cmd,
     "bridge <1-32> decouple vlan (range <2-4094> <2-4094>|<2-4094>)",
     NSM_BRIDGE_STR,
     NSM_BRIDGE_NAME_STR,
     "Decouple the following vlans from xSTP",
     "vlanid (2-4094)",
     "vlan id to be decoupled")
{
  struct nsm_master *nm = cli->vr->proto;
  struct nsm_bridge_master *master = nsm_bridge_get_master (nm);
  int ret = 0;
  u_int16_t start_vid, end_vid;
  u_int8_t brid = 0;

  end_vid = 0;

  /* Get the bridge group number and see if its within range. */
  CLI_GET_INTEGER_RANGE ("Bridge group", brid, argv[0], 1, 32);

  if (argv[1][0] == 'r')
    {
      /* Get the vid and check if it is within range (2-4096).  */
      CLI_GET_INTEGER_RANGE ("VLAN id", start_vid, argv[2], NSM_VLAN_CLI_MIN, 
                             NSM_VLAN_CLI_MAX);
      CLI_GET_INTEGER_RANGE ("VLAN id", end_vid, argv[3], NSM_VLAN_CLI_MIN,
                             NSM_VLAN_CLI_MAX);
      if (start_vid >end_vid)
        {
          cli_out (cli, "inappropriate range");
          return CLI_ERROR;
        }
    }
  else
    {
      CLI_GET_INTEGER_RANGE ("VLAN id", start_vid, argv[1], NSM_VLAN_CLI_MIN,
                             NSM_VLAN_CLI_MAX);
    }

  ret = nsm_bridge_decouple_vlan (master, argv[0], start_vid, end_vid, 0);

  if (ret < 0)
    {
      cli_out (cli, "Unable to decouple vlan");
      return CLI_ERROR;
    }
  return CLI_SUCCESS; 
}

CLI (no_bridge_decouple_vlan,
     no_bridge_decouple_vlan_cmd,
     "no bridge <1-32> decouple vlan (range <2-4094> <2-4094>|<2-4094>)",
     NSM_BRIDGE_STR,
     NSM_BRIDGE_NAME_STR,
     "Decouple the following vlans from xSTP",
     "vlanid (2-4094)",
     "vlan id to be decoupled")
{
  struct nsm_master *nm = cli->vr->proto;
  struct nsm_bridge_master *master = nsm_bridge_get_master (nm);
  int ret = 0;
  u_int16_t start_vid, end_vid;
  u_int8_t brid = 0;

  end_vid = 0;

  /* Get the bridge group number and see if its within range. */
  CLI_GET_INTEGER_RANGE ("Bridge group", brid, argv[0], 1, 32);

  if (argv[1][0] == 'r')
    {
      /* Get the vid and check if it is within range (2-4096).  */
      CLI_GET_INTEGER_RANGE ("VLAN id", start_vid, argv[2], NSM_VLAN_CLI_MIN,
                             NSM_VLAN_CLI_MAX);
      CLI_GET_INTEGER_RANGE ("VLAN id", end_vid, argv[3], NSM_VLAN_CLI_MIN,
                             NSM_VLAN_CLI_MAX);
      if (start_vid >end_vid)
        {
          cli_out (cli, "inappropriate range");
          return CLI_ERROR;
        }
    }
  else
    {
      CLI_GET_INTEGER_RANGE ("VLAN id", start_vid, argv[1], NSM_VLAN_CLI_MIN,
                             NSM_VLAN_CLI_MAX);
    }

  ret = nsm_bridge_remove_decoupling (master, argv[0], start_vid, end_vid, 0);

  if (ret < 0)
    {
      cli_out (cli, "Unable to remove decoupling for this vlan" );
      return CLI_ERROR;
    }
  return CLI_SUCCESS;
}

#endif /* if 0*/

#ifdef HAVE_DEFAULT_BRIDGE 
#ifdef HAVE_RPVST_PLUS
CLI (spanning_tree_mode,
     spanning_tree_mode_cmd,
     "spanning-tree mode (stp|stp-vlan-bridge|"
     "rstp|rstp-vlan-bridge|rpvst+|mstp|provider-rstp|provider-mstp) (edge|)",
     NSM_SPANNING_STR,
     "Spanning tree mode",
     "STP mode",
     "VLAN aware STP mode",
     "RSTP mode",
     "VLAN aware RSTP mode",
     "VLAN aware RPVST+ mode",
     "MSTP mode",
     "Provider RSTP mode",
     "Provider MSTP mode",
     "Configure as Edge bridge")
{
  int ret;
  struct nsm_bridge *bridge;
  struct nsm_master *nm = cli->vr->proto;
  struct nsm_bridge_master *master = nsm_bridge_get_master (nm);

  if (! master)
    {
      cli_out (cli, "%% Bridge master not configured\n");
      return CLI_ERROR;
    }

  bridge = nsm_get_default_bridge (master);

  if (! bridge)
    {
      cli_out (cli, "%% Default Bridge not found\n");
      return CLI_ERROR;
    }

  /* Add bridge. */
  ret = nsm_bridge_add (master, nsm_bridge_str_to_type (argv [0]),
                        bridge->name, PAL_FALSE, TOPOLOGY_NONE,
                        (argc == 2) ? PAL_TRUE:PAL_FALSE,
                        PAL_FALSE, NULL);

  switch (ret)
    {
    case NSM_BRIDGE_ERR_EXISTS:
      cli_out (cli, "%% Bridge %s already exists\n", argv[0]);
      return CLI_ERROR;
    case NSM_BRIDGE_ERR_NOTFOUND:
    case NSM_BRIDGE_ERR_MEM:
      cli_out (cli, "%% Error adding bridge\n");
      return CLI_ERROR;
    default:
      return CLI_SUCCESS;
    }
}
#else
CLI (spanning_tree_mode,
     spanning_tree_mode_cmd,
     "spanning-tree mode (stp|stp-vlan-bridge|"
     "rstp|rstp-vlan-bridge|mstp|provider-rstp|provider-mstp) (edge|)",
     NSM_SPANNING_STR,
     "Spanning tree mode",
     "STP mode",
     "VLAN aware STP mode",
     "RSTP mode",
     "VLAN aware RSTP mode",
     "MSTP mode",
     "Provider RSTP mode",
     "Provider MSTP mode",
     "Configure as Edge bridge")
{
  int ret;
  struct nsm_bridge *bridge;
  struct nsm_master *nm = cli->vr->proto;
  struct nsm_bridge_master *master = nsm_bridge_get_master (nm);

  if (! master)
    {
      cli_out (cli, "%% Bridge master not configured\n");
      return CLI_ERROR;
    }

  bridge = nsm_get_default_bridge (master);

  if (! bridge)
    {
      cli_out (cli, "%% Default Bridge not found\n");
      return CLI_ERROR;
    }

  /* Add bridge. */
  ret = nsm_bridge_add (master, nsm_bridge_str_to_type (argv [0]),
                        bridge->name, PAL_FALSE, TOPOLOGY_NONE,
                        (argc == 2) ? PAL_TRUE:PAL_FALSE,
                        PAL_FALSE, NULL);

  switch (ret)
    {
    case NSM_BRIDGE_ERR_EXISTS:
      cli_out (cli, "%% Bridge %s already exists\n", argv[0]);
      return CLI_ERROR;
    case NSM_BRIDGE_ERR_NOTFOUND:
    case NSM_BRIDGE_ERR_MEM:
      cli_out (cli, "%% Error adding bridge\n");
      return CLI_ERROR;
    default:
      return CLI_SUCCESS;
    }
}
#endif /* HAVE_RPVST_PLUS */
#endif /* HAVE_DEFAULT_BRIDGE */

CLI (bridge_acquire,
     bridge_acquire_cmd,
     "bridge <1-32> acquire",
     NSM_BRIDGE_STR,
     NSM_BRIDGE_NAME_STR,
     "Enable dynamic learning of mac addresses")
{
  struct nsm_master *nm = cli->vr->proto;
  struct nsm_bridge_master *master = nsm_bridge_get_master (nm);
  int ret;
  int brno;

  if (! master)
    {
      cli_out (cli, "%% Bridge master not configured\n");
      return CLI_ERROR;
    }

  CLI_GET_INTEGER_RANGE ("Max bridge groups", brno, argv[0], 1, 32);

  /* Set bridge learning. */
  ret = nsm_bridge_set_learning (master, argv[0]);
  switch (ret)
    {
      case NSM_BRIDGE_ERR_NOTFOUND:
        cli_out (cli, "%% Bridge not found\n");
        return CLI_ERROR;
      default:
        return CLI_SUCCESS;
    }
}

CLI (no_bridge_acquire,
     no_bridge_acquire_cmd,
     "no bridge <1-32> acquire",
     CLI_NO_STR,
     NSM_BRIDGE_STR,
     NSM_BRIDGE_NAME_STR,
     "Disable dynamic learning of mac addresses")
{
  struct nsm_master *nm = cli->vr->proto;
  struct nsm_bridge_master *master = nsm_bridge_get_master (nm);
  int ret;
  int brno;

  if (! master)
    {
      cli_out (cli, "%% Bridge master not configured\n");
      return CLI_ERROR;
    }

  CLI_GET_INTEGER_RANGE ("Max bridge groups", brno, argv[0], 1, 32);

  /* Set bridge learning. */
  ret = nsm_bridge_unset_learning (master, argv[0]);
  switch (ret)
    {
      case NSM_BRIDGE_ERR_NOTFOUND:
        cli_out (cli, "%% Bridge not found\n");
        return CLI_ERROR;
      default:
        return CLI_SUCCESS;
    }
}

CLI (spanning_acquire,
     spanning_acquire_cmd,
     "spanning-tree acquire",
     NSM_SPANNING_STR,
     "Enable dynamic learning of mac addresses")
{
  struct nsm_bridge *bridge;
  struct nsm_master *nm = cli->vr->proto;
  struct nsm_bridge_master *master = nsm_bridge_get_master (nm);
  int ret;

  if (! master)
    {
      cli_out (cli, "%% Bridge master not configured\n");
      return CLI_ERROR;
    }

  bridge = nsm_get_default_bridge (master);

  if (! bridge)
    {
      cli_out (cli, "%% Default Bridge not found\n");
      return CLI_ERROR;
    }

  /* Set bridge learning. */
  ret = nsm_bridge_set_learning (master, bridge->name);

  switch (ret)
    {
      case NSM_BRIDGE_ERR_NOTFOUND:
        cli_out (cli, "%% Bridge not found\n");
        return CLI_ERROR;
      default:
        return CLI_SUCCESS;
    }
}

CLI (no_spanning_acquire,
     no_spanning_acquire_cmd,
     "no spanning-tree acquire",
     CLI_NO_STR,
     NSM_SPANNING_STR,
     "Disable dynamic learning of mac addresses")
{
  struct nsm_bridge *bridge;
  struct nsm_master *nm = cli->vr->proto;
  struct nsm_bridge_master *master = nsm_bridge_get_master (nm);
  int ret;

  if (! master)
    {
      cli_out (cli, "%% Bridge master not configured\n");
      return CLI_ERROR;
    }

  bridge = nsm_get_default_bridge (master);

  if (! bridge)
    {
      cli_out (cli, "%% Default Bridge not found\n");
      return CLI_ERROR;
    }

  /* Set bridge learning. */
  ret = nsm_bridge_unset_learning (master, bridge->name);

  switch (ret)
    {
      case NSM_BRIDGE_ERR_NOTFOUND:
        cli_out (cli, "%% Bridge not found\n");
        return CLI_ERROR;
      default:
        return CLI_SUCCESS;
    }
}
#ifdef HAVE_RPVST_PLUS
CLI (nsm_show_bridge,
     nsm_show_bridge_cmd,
     "show bridge (|ieee|rstp|rpvst+|mstp|provider-mstp|provider-rstp)",
     CLI_SHOW_STR,
     "Display forwarding information",
     "Forwarding information of STP Bridges",
     "Forwarding information of RSTP Bridges",
     "Forwarding information of RPVST Bridges",
     "Forwarding information of MSTP Bridges")
{
  int type = 0;

  if (argv[0])
    {
      if ( !pal_strncmp (argv[0], "i", 1) )
        type = NSM_BRIDGE_TYPE_STP;
      else if ( !pal_strncmp (argv[0], "rs", 2) )
        type = NSM_BRIDGE_TYPE_RSTP;
      else if ( !pal_strncmp (argv[0], "rp", 2) )
        type = NSM_BRIDGE_TYPE_RPVST_PLUS;
      else if ( !pal_strncmp (argv[0], "m", 1) )
        type = NSM_BRIDGE_TYPE_MSTP;
      else if ( !pal_strncmp (argv[0], "provider-m", 10) )
        type = NSM_BRIDGE_TYPE_PROVIDER_MSTP;
      else if ( !pal_strncmp (argv[0], "provider-r", 10) )
        type = NSM_BRIDGE_TYPE_PROVIDER_RSTP;
    }

  nsm_bridge_show(cli, type);

  return CLI_SUCCESS;
}
#else
CLI (nsm_show_bridge,
     nsm_show_bridge_cmd,
     "show bridge (|ieee|rstp|mstp|provider-mstp|provider-rstp)",
     CLI_SHOW_STR,
     "Display forwarding information",
     "Forwarding information of STP Bridges",
     "Forwarding information of RSTP Bridges",
     "Forwarding information of MSTP Bridges")
{
  int type = 0;
  
  if (argv[0])
    {
      if ( !pal_strncmp (argv[0], "i", 1) )
        type = NSM_BRIDGE_TYPE_STP;
      if ( !pal_strncmp (argv[0], "r", 1) )
        type = NSM_BRIDGE_TYPE_RSTP;
      if ( !pal_strncmp (argv[0], "m", 1) )
        type = NSM_BRIDGE_TYPE_MSTP;
      if ( !pal_strncmp (argv[0], "provider-m", 10) )
        type = NSM_BRIDGE_TYPE_PROVIDER_MSTP;
      if ( !pal_strncmp (argv[0], "provider-r", 10) )
        type = NSM_BRIDGE_TYPE_PROVIDER_RSTP;

    }
  
  nsm_bridge_show(cli, type);
  
  return CLI_SUCCESS; 
}
#endif /* HAVE_RPVST_PLUS */

CLI (bridge_add_static_forward,
     bridge_add_static_forward_cmd,
     "bridge <1-32> address MAC forward IFNAME",
     NSM_BRIDGE_STR,
     NSM_BRIDGE_NAME_STR,
     "address",
     "mac address in HHHH.HHHH.HHHH format",
     "forward",
     CLI_IFNAME_STR)
{
  int ret;
  int brno;
  u_int8_t mac[6];
  struct interface *ifp;
  u_int16_t vid = 0;
  u_int16_t svid = 0;
  struct nsm_master *nm = cli->vr->proto;
  struct nsm_bridge_master *master = nsm_bridge_get_master (nm);

  if (! master)
    {
      cli_out (cli, "%% Bridge master not configured\n");
      return CLI_ERROR;
    }

  CLI_GET_INTEGER_RANGE ("Max bridge groups", brno, argv[0], 1, 32);
  
  if ( (ifp = if_lookup_by_name(&cli->vr->ifm, argv[2])) == NULL )
    {
      cli_out (cli, "%% Interface not found\n");
      return CLI_ERROR;
    } 

  /* Validate the MAC address */
  if (! nsm_api_mac_address_is_valid (argv[1]))
    {
      cli_out (cli, "%% Unable to translate mac address %s\n",
               argv[1]);
      return CLI_ERROR;
    }

   NSM_RESERVED_ADDRESS_CHECK (argv[1]);

  /* Get the MAC address */
   NSM_READ_MAC_ADDRESS (argv[1], mac);

  /* Convert to network order */
  *(unsigned short *)&mac[0] = pal_hton16(*(unsigned short *)&mac[0]);
  *(unsigned short *)&mac[2] = pal_hton16(*(unsigned short *)&mac[2]);
  *(unsigned short *)&mac[4] = pal_hton16(*(unsigned short *)&mac[4]);

  vid = NSM_VLAN_NULL_VID;
  svid = NSM_VLAN_NULL_VID;

#ifdef HAVE_VLAN
#ifdef HAVE_PROVIDER_BRIDGE
    /* Get the vid and check if it is within range (2-4094) */
  if (argc > 3)
    CLI_GET_INTEGER_RANGE ("vlanid", vid, argv[3], NSM_VLAN_CLI_MIN,
                             NSM_VLAN_CLI_MAX);

  if (argc > 4)
    CLI_GET_INTEGER_RANGE ("svlanid", svid, argv[4], NSM_VLAN_CLI_MIN,
                             NSM_VLAN_CLI_MAX);
  else
    svid = vid;
#else
  /* Get the vlan id if passed by user */
  if (argc > 3)
    {
      /* Get the vid and check if it is within range (2-4094) */
      CLI_GET_INTEGER_RANGE ("vlanid", vid, argv[3], NSM_VLAN_CLI_MIN,
                             NSM_VLAN_CLI_MAX);
      svid = vid;
    }
#endif /* HAVE_PROVIDER_BRIDGE */
#endif /* HAVE_VLAN */
  
  /* Set bridge learning. */
  ret = nsm_bridge_add_mac (master, argv[0], ifp, mac, vid, svid,
                            HAL_L2_FDB_STATIC, PAL_TRUE, CLI_MAC);

  switch (ret)
    {
      case NSM_BRIDGE_ERR_NOTFOUND:
        cli_out (cli, "%% Bridge not found\n");
        return CLI_ERROR;
      case NSM_BRIDGE_ERR_NOT_BOUND:
        cli_out (cli, "%% Interface not bound to bridge or vlan\n");
        return CLI_ERROR;
      case NSM_HAL_FDB_ERR:
        cli_out (cli, "%% Failed to add fdb entry to bridge\n");
        return CLI_ERROR;
      case NSM_ERR_DUPLICATE_MAC:
        cli_out (cli, "%% Can't set duplicate MAC filter\n");
        return CLI_ERROR;
      default:
        return CLI_SUCCESS;
    }
}

#ifdef HAVE_VLAN
ALI (bridge_add_static_forward,
     vlan_add_static_forward_cmd,
     "bridge <1-32> address MAC forward IFNAME vlan "NSM_VLAN_CLI_RNG,
     NSM_BRIDGE_STR,
     NSM_BRIDGE_NAME_STR,
     "address",
     "mac address in HHHH.HHHH.HHHH format",
     "forward",
     CLI_IFNAME_STR,
     "VLAN",
     "vlan id");

CLI ( mac_prio_override,
      mac_prio_override_cmd,
      "bridge <1-32> mac-priority-override mac-address MAC " 
      "interface IFNAME vlan VLANID "
      "(static|static-priority-override|static-mgmt|"
       "static-mgmt-priority-overide) "
      "priority <0-7> ",
       NSM_BRIDGE_STR,
       NSM_BRIDGE_NAME_STR,
      "mac priority overide",
      "mac address",
      "mac address in HHHH.HHHH.HHHH format",
      CLI_INTERFACE_STR,
      CLI_IFNAME_STR,
      CLI_VLAN_STR,
      "VLAN Id", 
      "The MAC is a static entry",
      "The MAC is a static with priority override",
      "The MAC is a Static Management ",
      "The MAC is a Static Management with priority override",
      "priority",
      "priority value")
{ 
  int ret;
  int brno;
  char mac[6];
  u_int16_t vid = 0;
  struct interface *ifp;
  s_int32_t priority  = 0;
  enum nsm_bridge_pri_ovr_mac_type ovr_mac_type;
  
  struct nsm_master *nm = cli->vr->proto;
  struct nsm_bridge_master *master = nsm_bridge_get_master (nm);

  CLI_GET_INTEGER_RANGE ("Max bridge groups", brno, argv[0], 1, 32);

  if ( (ifp = if_lookup_by_name(&cli->vr->ifm, argv[2])) == NULL )
    {
      cli_out (cli, "%% Interface not found\n");
      return CLI_ERROR;
    }

  /* Validate the MAC address */
  if (! nsm_api_mac_address_is_valid (argv[1]))
    {
      cli_out (cli, "%% Unable to translate mac address %s\n",
               argv[1]);
      return CLI_ERROR;
    }

  NSM_RESERVED_ADDRESS_CHECK (argv[1]);

  /* Get the MAC address */
  NSM_READ_MAC_ADDRESS (argv[1], mac);

  /* Convert to network order */
  *(unsigned short *)&mac[0] = pal_hton16(*(unsigned short *)&mac[0]);
  *(unsigned short *)&mac[2] = pal_hton16(*(unsigned short *)&mac[2]);
  *(unsigned short *)&mac[4] = pal_hton16(*(unsigned short *)&mac[4]);

  CLI_GET_INTEGER_RANGE ("vlanid", vid, argv[3], NSM_VLAN_DEFAULT_VID,
                             NSM_VLAN_CLI_MAX);

  ovr_mac_type = nsm_bridge_str_to_ovr_mac_type (argv [4]);

  if (ovr_mac_type == NSM_BRIDGE_MAC_PRI_OVR_MAX)
    {
      cli_out (cli, "Invalid MAC Table type %s\n", argv [4]);
      return CLI_ERROR;
    }

  CLI_GET_INTEGER_RANGE ("priority", priority, argv[5], NSM_PRIO_CLI_MIN,
                          NSM_PRIO_CLI_MAX);

  ret = nsm_bridge_add_mac_prio_ovr (master, argv [0], ifp, mac, vid,
                                     ovr_mac_type, priority);

  if (ret != 0)
    {
      cli_out (cli, "Unable to add the entry into MAC Table\n");
      return CLI_ERROR;
    }
 
  return CLI_SUCCESS;

}

CLI ( no_mac_prio_override,
      no_mac_prio_override_cmd,
      "no bridge <1-32> mac-priority-override mac-address MAC " 
      "interface IFNAME vlan VLANID ",
       CLI_NO_STR,
       NSM_BRIDGE_STR,
       NSM_BRIDGE_NAME_STR,
      "mac priority overide",
      "mac address",
      "mac address in HHHH.HHHH.HHHH format",
      CLI_INTERFACE_STR,
      CLI_IFNAME_STR,
      CLI_VLAN_STR,
      "VLAN Id")
{ 
  int ret;
  int brno;
  char mac[6];
  u_int16_t vid = 0;
  struct interface *ifp;
  struct nsm_master *nm = cli->vr->proto;
  struct nsm_bridge_master *master = nsm_bridge_get_master (nm);

  CLI_GET_INTEGER_RANGE ("Max bridge groups", brno, argv[0], 1, 32);

  if ( (ifp = if_lookup_by_name(&cli->vr->ifm, argv[2])) == NULL )
    {
      cli_out (cli, "%% Interface not found\n");
      return CLI_ERROR;
    }

  /* Validate the MAC address */
  if (! nsm_api_mac_address_is_valid (argv[1]))
    {
      cli_out (cli, "%% Unable to translate mac address %s\n",
               argv[1]);
      return CLI_ERROR;
    }

  /* Get the MAC address */
  NSM_READ_MAC_ADDRESS (argv[1], mac);

  /* Convert to network order */
  *(unsigned short *)&mac[0] = pal_hton16(*(unsigned short *)&mac[0]);
  *(unsigned short *)&mac[2] = pal_hton16(*(unsigned short *)&mac[2]);
  *(unsigned short *)&mac[4] = pal_hton16(*(unsigned short *)&mac[4]);

  CLI_GET_INTEGER_RANGE ("vlanid", vid, argv[3], NSM_VLAN_DEFAULT_VID,
                             NSM_VLAN_CLI_MAX);

  ret = nsm_bridge_del_mac_prio_ovr (master, argv [0], ifp, mac, vid);

  if (ret != 0)
    {
      cli_out (cli, "Unable to add the entry into MAC Table\n");
      return CLI_ERROR;
    }
 
  return CLI_SUCCESS;

}
#endif

#ifdef HAVE_PROVIDER_BRIDGE
ALI (bridge_add_static_forward,
     svlan_add_static_forward_cmd,
     "bridge <1-32> address MAC forward IFNAME vlan "
     NSM_VLAN_CLI_RNG" svlan "NSM_VLAN_CLI_RNG,
     NSM_BRIDGE_STR,
     NSM_BRIDGE_NAME_STR,
     "address",
     "mac address in HHHH.HHHH.HHHH format",
     "forward",
     CLI_IFNAME_STR,
     "VLAN",
     "vlan id",
     "SVLAN",
     "svlan id");
#endif /* HAVE_PROVIDER_BRIDGE */

CLI (no_bridge_static,
     no_bridge_static_cmd,
     "no bridge <1-32> address MAC forward IFNAME",
     CLI_NO_STR,
     NSM_BRIDGE_STR,
     NSM_BRIDGE_NAME_STR,
     "address",
     "mac address in HHHH.HHHH.HHHH format",
     "forward",
     CLI_IFNAME_STR)
{
  int ret;
  int brno;
  u_int8_t mac[6];
  struct interface *ifp;
  u_int16_t vid = 0;
  u_int16_t svid = 0;
  struct nsm_master *nm = cli->vr->proto;
  struct nsm_bridge_master *master = nsm_bridge_get_master (nm);

  if (! master)
    {
      cli_out (cli, "%% Bridge master not configured\n");
      return CLI_ERROR;
    }

  CLI_GET_INTEGER_RANGE ("Max bridge groups", brno, argv[0], 1, 32);
  
  if ( (ifp = if_lookup_by_name(&cli->vr->ifm, argv[2])) == NULL )
    {
      cli_out (cli, "%% Interface not found\n");
      return CLI_ERROR;
    } 

  /* Validate the MAC address */
  if (! nsm_api_mac_address_is_valid (argv[1]))
    {
      cli_out (cli, "%% Unable to translate mac address %s\n",
               argv[1]);
      return CLI_ERROR;
    }

  NSM_RESERVED_ADDRESS_CHECK (argv[1]);

  /* Get the MAC address */
  NSM_READ_MAC_ADDRESS (argv[1], mac);

  /* Convert to network order */
  *(unsigned short *)&mac[0] = pal_hton16(*(unsigned short *)&mac[0]);
  *(unsigned short *)&mac[2] = pal_hton16(*(unsigned short *)&mac[2]);
  *(unsigned short *)&mac[4] = pal_hton16(*(unsigned short *)&mac[4]);

  vid = NSM_VLAN_NULL_VID;
  svid = NSM_VLAN_NULL_VID;

#ifdef HAVE_VLAN
#ifdef HAVE_PROVIDER_BRIDGE
    /* Get the vid and check if it is within range (2-4094) */
  if (argc > 3)
    CLI_GET_INTEGER_RANGE ("vlanid", vid, argv[3], NSM_VLAN_CLI_MIN,
                             NSM_VLAN_CLI_MAX);

  if (argc > 4)
    CLI_GET_INTEGER_RANGE ("svlanid", svid, argv[4], NSM_VLAN_CLI_MIN,
                             NSM_VLAN_CLI_MAX);
  else
    svid = vid;
#else
  /* Get the vlan id if passed by user */
  if (argc > 3)
    {
      /* Get the vid and check if it is within range (2-4094) */
      CLI_GET_INTEGER_RANGE ("vlanid", vid, argv[3], NSM_VLAN_CLI_MIN,
                             NSM_VLAN_CLI_MAX);

      svid = vid;
    }
#endif /* HAVE_PROVIDER_BRIDGE */
#endif /* HAVE_VLAN */

  /* Set bridge learning. */
  ret = nsm_bridge_delete_mac (master, argv[0], ifp, mac, vid, svid,
                               HAL_L2_FDB_STATIC, PAL_TRUE, CLI_MAC); 


  switch (ret)
    {
      case NSM_BRIDGE_ERR_NOTFOUND:
        cli_out (cli, "%% Bridge not found\n");
        return CLI_ERROR;
      case NSM_BRIDGE_ERR_NOT_BOUND:
        cli_out (cli, "%% Interface not bound to bridge or vlan\n");
        return CLI_ERROR;
      case NSM_HAL_FDB_ERR:
        cli_out (cli, "%% Failed to delete fdb entry from bridge\n");
        return CLI_ERROR;
      default:
        return CLI_SUCCESS;
    }
}

#ifdef HAVE_VLAN
ALI (no_bridge_static,
     no_vlan_static_cmd,
     "no bridge <1-32> address MAC forward IFNAME vlan "NSM_VLAN_CLI_RNG,
     CLI_NO_STR,
     NSM_BRIDGE_STR,
     NSM_BRIDGE_NAME_STR,
     "address",
     "mac address in HHHH.HHHH.HHHH format",
     "forward",
     CLI_IFNAME_STR,
     "VLAN",
     "vlan id");
#endif

#ifdef HAVE_PROVIDER_BRIDGE
ALI (no_bridge_static,
     no_svlan_static_cmd,
     "no bridge <1-32> address MAC forward IFNAME vlan ",
     NSM_VLAN_CLI_RNG" svlan "NSM_VLAN_CLI_RNG,
     CLI_NO_STR,
     NSM_BRIDGE_STR,
     NSM_BRIDGE_NAME_STR,
     "address",
     "mac address in HHHH.HHHH.HHHH format",
     "forward",
     CLI_IFNAME_STR,
     "VLAN",
     "vlan id",
     "SVLAN",
     "svlan id");
#endif /* HAVE_PROVIDER_BRIDGE */

CLI (spanning_add_static,
     spanning_add_static_cmd,
     "mac-address-table static MAC (forward|discard) IFNAME",
     NSM_SPANNING_STR,
     "Static address",
     "mac address in HHHH.HHHH.HHHH format",
     "forward",
     "discard",
     CLI_IFNAME_STR)
{
  int ret;
  char mac[6];
  char is_fwd;
  struct interface *ifp;
  u_int16_t vid = 0;
  u_int16_t svid = 0;
  struct nsm_bridge *bridge;
  struct nsm_master *nm = cli->vr->proto;
  struct nsm_bridge_master *master = nsm_bridge_get_master (nm);

  if (! master)
    {
      cli_out (cli, "%% Bridge master not configured\n");
      return CLI_ERROR;
    }

  bridge = nsm_get_default_bridge (master);

  if (! bridge)
    {
      cli_out (cli, "%% Default Bridge not found\n");
      return CLI_ERROR;
    }

  if (argv [1] [0] == 'f')
    is_fwd = PAL_TRUE;
  else
    is_fwd = PAL_FALSE;

  if ( (ifp = if_lookup_by_name(&cli->vr->ifm, argv[2])) == NULL )
    {
      cli_out (cli, "%% Interface not found\n");
      return CLI_ERROR;
    }

  /* Validate the MAC address */
  if (! nsm_api_mac_address_is_valid (argv[0]))
    {
      cli_out (cli, "%% Unable to translate mac address %s\n",
               argv[0]);
      return CLI_ERROR;
    }

  NSM_RESERVED_ADDRESS_CHECK (argv[0]);

  /* Get the MAC address */
  NSM_READ_MAC_ADDRESS (argv[0], mac);

  /* Convert to network order */
  *(unsigned short *)&mac[0] = pal_hton16(*(unsigned short *)&mac[0]);
  *(unsigned short *)&mac[2] = pal_hton16(*(unsigned short *)&mac[2]);
  *(unsigned short *)&mac[4] = pal_hton16(*(unsigned short *)&mac[4]);

  vid = NSM_VLAN_NULL_VID;
  svid = NSM_VLAN_NULL_VID;

#ifdef HAVE_VLAN
#ifdef HAVE_PROVIDER_BRIDGE
    /* Get the vid and check if it is within range (2-4094) */
  if (argc > 3)
    CLI_GET_INTEGER_RANGE ("vlanid", vid, argv[3], NSM_VLAN_CLI_MIN,
                             NSM_VLAN_CLI_MAX);

  if (argc > 4)
    CLI_GET_INTEGER_RANGE ("svlanid", svid, argv[4], NSM_VLAN_CLI_MIN,
                             NSM_VLAN_CLI_MAX);
  else
    svid = vid;
#else
  /* Get the vlan id if passed by user */
  if (argc > 3)
    {
      /* Get the vid and check if it is within range (2-4094) */
      CLI_GET_INTEGER_RANGE ("vlanid", vid, argv[3], NSM_VLAN_CLI_MIN,
                             NSM_VLAN_CLI_MAX);
      svid = vid;
    }
#endif /* HAVE_PROVIDER_BRIDGE */
#endif /* HAVE_VLAN */

  /* Set bridge learning. */
  ret = nsm_bridge_add_mac (master, bridge->name, ifp, mac, vid, svid,
                            HAL_L2_FDB_STATIC, is_fwd, CLI_MAC);

  switch (ret)
    {
      case NSM_BRIDGE_ERR_NOTFOUND:
        cli_out (cli, "%% Bridge not found\n");
        return CLI_ERROR;
      case NSM_BRIDGE_ERR_NOT_BOUND:
        cli_out (cli, "%% Interface not bound to bridge or vlan\n");
        return CLI_ERROR;
      case NSM_HAL_FDB_ERR:
        cli_out (cli, "%% Failed to add fdb entry to bridge\n");
        return CLI_ERROR;
      case NSM_ERR_DUPLICATE_MAC:
        cli_out (cli, "%% Can't set duplicate MAC filter\n");
        return CLI_ERROR;
      default:
        return CLI_SUCCESS;
    }
}

#ifdef HAVE_VLAN
ALI (spanning_add_static,
     spanning_vlan_add_static_cmd,
     "mac-address-table static MAC (forward|discard) IFNAME vlan "NSM_VLAN_CLI_RNG,
     NSM_SPANNING_STR,
     "Static address",
     "mac address in HHHH.HHHH.HHHH format",
     "forward",
     "discard",
     CLI_IFNAME_STR,
     "VLAN",
     "vlan id");
#endif

#ifdef HAVE_PROVIDER_BRIDGE
ALI (spanning_add_static,
     spanning_svlan_add_static_cmd,
     "mac-address-table static MAC (forward|discard) IFNAME "
     "vlan "NSM_VLAN_CLI_RNG" svlan "NSM_VLAN_CLI_RNG,
     NSM_SPANNING_STR,
     "Static address",
     "mac address in HHHH.HHHH.HHHH format",
     "forward",
     "discard",
     CLI_IFNAME_STR,
     "VLAN",
     "vlan id",
     "SVLAN",
     "svlan id");
#endif /* HAVE_PROVIDER_BRIDGE */

CLI (no_spanning_static,
     no_spanning_static_cmd,
     "no mac-address-table static MAC (forward|discard) IFNAME",
     CLI_NO_STR,
     NSM_SPANNING_STR,
     "Static address",
     "mac address in HHHH.HHHH.HHHH format",
     "forward",
     "discard",
     CLI_IFNAME_STR)
{
  int ret;
  char mac[6];
  char is_fwd;
  struct interface *ifp;
  u_int16_t vid = 0;
  u_int16_t svid = 0;
  struct nsm_bridge *bridge;
  struct nsm_master *nm = cli->vr->proto;
  struct nsm_bridge_master *master = nsm_bridge_get_master (nm);

  if (! master)
    {
      cli_out (cli, "%% Bridge master not configured\n");
      return CLI_ERROR;
    }

  bridge = nsm_get_default_bridge (master);

  if (! bridge)
    {
      cli_out (cli, "%% Default Bridge not found\n");
      return CLI_ERROR;
    }

  if (argv [1] [0] == 'f')
    is_fwd = PAL_TRUE;
  else
    is_fwd = PAL_FALSE;

  if ( (ifp = if_lookup_by_name(&cli->vr->ifm, argv[2])) == NULL )
    {
      cli_out (cli, "%% Interface not found\n");
      return CLI_ERROR;
    }

  /* Validate the MAC address */
  if (! nsm_api_mac_address_is_valid (argv[0]))
    {
      cli_out (cli, "%% Unable to translate mac address %s\n",
               argv[0]);
      return CLI_ERROR;
    }

  NSM_RESERVED_ADDRESS_CHECK (argv[0]);

  /* Get the MAC address */
  NSM_READ_MAC_ADDRESS (argv[0], mac);

  /* Convert to network order */
  *(unsigned short *)&mac[0] = pal_hton16(*(unsigned short *)&mac[0]);
  *(unsigned short *)&mac[2] = pal_hton16(*(unsigned short *)&mac[2]);
  *(unsigned short *)&mac[4] = pal_hton16(*(unsigned short *)&mac[4]);

  vid = NSM_VLAN_NULL_VID;
  svid = NSM_VLAN_NULL_VID;

#ifdef HAVE_VLAN
#ifdef HAVE_PROVIDER_BRIDGE
    /* Get the vid and check if it is within range (2-4094) */
  if (argc > 3)
    CLI_GET_INTEGER_RANGE ("vlanid", vid, argv[3], NSM_VLAN_CLI_MIN,
                             NSM_VLAN_CLI_MAX);

  if (argc > 4)
    CLI_GET_INTEGER_RANGE ("svlanid", svid, argv[4], NSM_VLAN_CLI_MIN,
                             NSM_VLAN_CLI_MAX);
  else
    svid = vid;

#else
  /* Get the vlan id if passed by user */
  if (argc > 3)
    {
      /* Get the vid and check if it is within range (2-4094) */
      CLI_GET_INTEGER_RANGE ("vlanid", vid, argv[3], NSM_VLAN_CLI_MIN,
                             NSM_VLAN_CLI_MAX);
      svid = vid;
    }
#endif /* HAVE_PROVIDER_BRIDGE */
#endif /* HAVE_VLAN */

  ret = nsm_bridge_delete_mac (master, bridge->name, ifp, mac, vid,
                               vid, HAL_L2_FDB_STATIC, is_fwd, CLI_MAC);


  switch (ret)
    {
      case NSM_BRIDGE_ERR_NOTFOUND:
        cli_out (cli, "%% Bridge not found\n");
        return CLI_ERROR;
      case NSM_BRIDGE_ERR_NOT_BOUND:
        cli_out (cli, "%% Interface not bound to bridge or vlan\n");
        return CLI_ERROR;
      case NSM_HAL_FDB_ERR:
        cli_out (cli, "%% Failed to delete fdb entry from bridge\n");
        return CLI_ERROR;
      default:
        return CLI_SUCCESS;
    }
}

#ifdef HAVE_VLAN
ALI (no_spanning_static,
     no_spanning_vlan_static_cmd,
     "no mac-address-table static MAC (forward|discard) IFNAME vlan "NSM_VLAN_CLI_RNG,
     CLI_NO_STR,
     NSM_SPANNING_STR,
     "Static address",
     "mac address in HHHH.HHHH.HHHH format",
     "forward",
     "discard",
     CLI_IFNAME_STR,
     "VLAN",
     "vlan id");
#endif

#ifdef HAVE_PROVIDER_BRIDGE
ALI (no_spanning_static,
     no_spanning_svlan_static_cmd,
     "no mac-address-table static MAC (forward|discard) IFNAME "
     "vlan "NSM_VLAN_CLI_RNG" svlan "NSM_VLAN_CLI_RNG,
     CLI_NO_STR,
     NSM_SPANNING_STR,
     "Static address",
     "mac address in HHHH.HHHH.HHHH format",
     "forward",
     "discard",
     CLI_IFNAME_STR,
     "VLAN",
     "vlan id",
     "SVLAN",
     "svlan id");
#endif /* HAVE_PROVIDER_BRIDGE */

CLI (bridge_add_static_filter,
     bridge_add_static_filter_cmd,
     "bridge <1-32> address MAC discard IFNAME",
     NSM_BRIDGE_STR,
     NSM_BRIDGE_NAME_STR,
     "address",
     "mac address in HHHH.HHHH.HHHH format",
     "discard",
     CLI_IFNAME_STR)
{
  int ret;
  int brno;
  u_int8_t mac[6];
  struct interface *ifp;
  u_int16_t vid = 0;
  u_int16_t svid = 0;
  struct nsm_master *nm = cli->vr->proto;
  struct nsm_bridge_master *master = nsm_bridge_get_master (nm);

  if (! master)
    {
      cli_out (cli, "%% Bridge master not configured\n");
      return CLI_ERROR;
    }

  CLI_GET_INTEGER_RANGE ("Max bridge groups", brno, argv[0], 1, 32);
  
  if ( (ifp = if_lookup_by_name(&cli->vr->ifm, argv[2])) == NULL )
    {
      cli_out (cli, "%% Interface not found\n");
      return CLI_ERROR;
    } 

  /* Validate the MAC address */
  if (! nsm_api_mac_address_is_valid (argv[1]))
    {
      cli_out (cli, "%% Unable to translate mac address %s\n",
               argv[1]);
      return CLI_ERROR;
    }

  NSM_RESERVED_ADDRESS_CHECK (argv[1]);

  /* Get the MAC address */
  NSM_READ_MAC_ADDRESS (argv[1], mac); 

  /* Convert to network order */
  *(unsigned short *)&mac[0] = pal_hton16(*(unsigned short *)&mac[0]);
  *(unsigned short *)&mac[2] = pal_hton16(*(unsigned short *)&mac[2]);
  *(unsigned short *)&mac[4] = pal_hton16(*(unsigned short *)&mac[4]);

  vid = NSM_VLAN_NULL_VID;
  svid = NSM_VLAN_NULL_VID;

#ifdef HAVE_VLAN
#ifdef HAVE_PROVIDER_BRIDGE
    /* Get the vid and check if it is within range (2-4094) */
  if (argc > 3)
    CLI_GET_INTEGER_RANGE ("vlanid", vid, argv[3], NSM_VLAN_CLI_MIN,
                             NSM_VLAN_CLI_MAX);

  if (argc > 4)
    CLI_GET_INTEGER_RANGE ("svlanid", svid, argv[4], NSM_VLAN_CLI_MIN,
                             NSM_VLAN_CLI_MAX);
  else
    svid = vid;
#else
  /* Get the vlan id if passed by user */
  if (argc > 3)
    {
      /* Get the vid and check if it is within range (2-4094) */
      CLI_GET_INTEGER_RANGE ("vlanid", vid, argv[3], NSM_VLAN_CLI_MIN,
                             NSM_VLAN_CLI_MAX);
      svid = vid;
    }
#endif /* HAVE_PROVIDER_BRIDGE */
#endif /* HAVE_VLAN */
  
  /* Set bridge learning. */
  ret = nsm_bridge_add_mac (master, argv[0], ifp, mac, vid, vid,
                            HAL_L2_FDB_STATIC, PAL_FALSE, CLI_MAC);
  switch (ret)
    {
      case NSM_BRIDGE_ERR_NOTFOUND:
        cli_out (cli, "%% Bridge not found\n");
        return CLI_ERROR;
      case NSM_BRIDGE_ERR_NOT_BOUND:
        cli_out (cli, "%% Interface not bound to bridge or vlan\n");
        return CLI_ERROR;
      case NSM_HAL_FDB_ERR:
        cli_out (cli, "%% Failed to add mac filter to bridge\n");
        return CLI_ERROR;
      case NSM_ERR_DUPLICATE_MAC:
        cli_out (cli, "%% Can't set duplicate MAC filter\n");
        return CLI_ERROR;
      default:
        return CLI_SUCCESS;
    }
}

#ifdef HAVE_VLAN
ALI (bridge_add_static_filter,
     vlan_add_static_filter_cmd,
     "bridge <1-32> address MAC discard IFNAME vlan "NSM_VLAN_CLI_RNG,
     NSM_BRIDGE_STR,
     NSM_BRIDGE_NAME_STR,
     "address",
     "mac address in HHHH.HHHH.HHHH format",
     "discard",
     CLI_IFNAME_STR,
     "VLAN",
     "vlan id");
#endif /* HAVE_VLAN */

#ifdef HAVE_PROVIDER_BRIDGE
ALI (bridge_add_static_filter,
     svlan_add_static_filter_cmd,
     "bridge <1-32> address MAC discard IFNAME "
     "vlan "NSM_VLAN_CLI_RNG" svlan "NSM_VLAN_CLI_RNG,
     NSM_BRIDGE_STR,
     NSM_BRIDGE_NAME_STR,
     "address",
     "mac address in HHHH.HHHH.HHHH format",
     "discard",
     CLI_IFNAME_STR,
     "VLAN",
     "vlan id",
     "SVLAN",
     "svlan id");
#endif /* HAVE_PROVIDER_BRIDGE */

CLI (no_bridge_static_filter,
     no_bridge_static_filter_cmd,
     "no bridge <1-32> address MAC discard IFNAME",
     CLI_NO_STR,
     NSM_BRIDGE_STR,
     NSM_BRIDGE_NAME_STR,
     "address",
     "mac address in HHHH.HHHH.HHHH format",
     "discard",
     CLI_IFNAME_STR)
{
  int ret;
  int brno;
  u_int8_t mac[6];
  struct interface *ifp;
  u_int16_t vid = 0;
  u_int16_t svid = 0;
  struct nsm_master *nm = cli->vr->proto;
  struct nsm_bridge_master *master = nsm_bridge_get_master (nm);

  if (! master)
    {
      cli_out (cli, "%% Bridge master not configured\n");
      return CLI_ERROR;
    }

  CLI_GET_INTEGER_RANGE ("Max bridge groups", brno, argv[0], 1, 32);
  
  if ( (ifp = if_lookup_by_name(&cli->vr->ifm, argv[2])) == NULL )
    {
      cli_out (cli, "%% Interface not found\n");
      return CLI_ERROR;
    } 

  /* Validate the MAC address */
  if (! nsm_api_mac_address_is_valid (argv[1]))
    {
      cli_out (cli, "%% Unable to translate mac address %s\n",
               argv[1]);
      return CLI_ERROR;
    }

  NSM_RESERVED_ADDRESS_CHECK (argv[1]);

  /* Get the MAC address */
  NSM_READ_MAC_ADDRESS (argv[1], mac);

  /* Convert to network order */
  *(unsigned short *)&mac[0] = pal_hton16(*(unsigned short *)&mac[0]);
  *(unsigned short *)&mac[2] = pal_hton16(*(unsigned short *)&mac[2]);
  *(unsigned short *)&mac[4] = pal_hton16(*(unsigned short *)&mac[4]);

  vid = NSM_VLAN_NULL_VID;
  svid = NSM_VLAN_NULL_VID;

#ifdef HAVE_VLAN
#ifdef HAVE_PROVIDER_BRIDGE
    /* Get the vid and check if it is within range (2-4094) */
  if (argc > 3)
    CLI_GET_INTEGER_RANGE ("vlanid", vid, argv[3], NSM_VLAN_CLI_MIN,
                             NSM_VLAN_CLI_MAX);

  if (argc > 4)
    CLI_GET_INTEGER_RANGE ("svlanid", svid, argv[4], NSM_VLAN_CLI_MIN,
                             NSM_VLAN_CLI_MAX);
  else
    svid = vid;
#else
  /* Get the vlan id if passed by user */
  if (argc > 3)
    {
      /* Get the vid and check if it is within range (2-4094) */
      CLI_GET_INTEGER_RANGE ("vlanid", vid, argv[3], NSM_VLAN_CLI_MIN,
                             NSM_VLAN_CLI_MAX);
      svid = vid;
    }
#endif /* HAVE_PROVIDER_BRIDGE */
#endif /* HAVE_VLAN */

  /* Set bridge learning. */
  ret = nsm_bridge_delete_mac (master, argv[0], ifp, mac, vid, vid,
                               HAL_L2_FDB_STATIC, PAL_FALSE, CLI_MAC); 
  switch (ret)
    {
      case NSM_BRIDGE_ERR_NOTFOUND:
        cli_out (cli, "%% Bridge not found\n");
        return CLI_ERROR;
      case NSM_BRIDGE_ERR_NOT_BOUND:
        cli_out (cli, "%% Interface not bound to bridge or vlan\n");
        return CLI_ERROR;
      case NSM_HAL_FDB_ERR:
        cli_out (cli, "%% Failed to delete fdb entry from bridge\n");
        return CLI_ERROR;
      default:
        return CLI_SUCCESS;
    }
}

#ifdef HAVE_VLAN
ALI (no_bridge_static_filter,
     no_vlan_static_filter_cmd,
     "no bridge <1-32> address MAC discard IFNAME vlan "NSM_VLAN_CLI_RNG,
     CLI_NO_STR,
     NSM_BRIDGE_STR,
     NSM_BRIDGE_NAME_STR,
     "address",
     "mac address in HHHH.HHHH.HHHH format",
     "discard",
     CLI_IFNAME_STR,
     "VLAN",
     "vlan id");
#endif

#ifdef HAVE_PROVIDER_BRIDGE
ALI (no_bridge_static_filter,
     no_svlan_static_filter_cmd,
     "no bridge <1-32> address MAC discard IFNAME "
     "vlan "NSM_VLAN_CLI_RNG" svlan "NSM_VLAN_CLI_RNG,
     CLI_NO_STR,
     NSM_BRIDGE_STR,
     NSM_BRIDGE_NAME_STR,
     "address",
     "mac address in HHHH.HHHH.HHHH format",
     "discard",
     CLI_IFNAME_STR,
     "VLAN",
     "vlan id",
     "SVLAN",
     "svlan id");
#endif /* HAVE_PROVIDER_BRIDGE */


CLI (nsm_bridge_clear_fdb,
     nsm_bridge_clear_br_fdb_cmd,
     "clear mac address-table (static|multicast) bridge <1-32>",
     "Clear the Forwarding database",
     "Clear layer 2 MAC entries",
     "Clear all Entries in the forwarding database",
     "Clear all entries configured through management",
     "Clear all multicast entries",
     NSM_BRIDGE_STR,
     NSM_BRIDGE_NAME_STR)
{
  int ret = 0;
  int brno = 0;
  struct nsm_bridge *bridge;
  struct nsm_master *nm = cli->vr->proto;
  struct nsm_bridge_master *master = nsm_bridge_get_master (nm);

  if (! master)
    {
      cli_out (cli, "%% Bridge master not configured\n");
      return CLI_ERROR;
    }

  if (argc == 2)
    {
      CLI_GET_INTEGER_RANGE ("Max bridge groups", brno, argv[1], 1, 32);
      bridge = nsm_lookup_bridge_by_name (master, argv[1]);
    }
  else
    bridge = nsm_get_default_bridge (master);

  if (! bridge)
    {
      cli_out (cli, "%% Bridge not found \n");
      return CLI_ERROR;
    }

  if (! pal_strncmp (argv[0], "s", 1))
    {
       ret = nsm_bridge_clear_static_mac (master, bridge->name);
    }
  else
    {
       ret = nsm_bridge_clear_multicast_mac (master, bridge->name);
    }

  if (ret < 0 )
    {
      switch (ret)
        {
          case NSM_BRIDGE_ERR_NOTFOUND:
            cli_out (cli, "%% Bridge not found\n");
            return CLI_ERROR;
          case NSM_HAL_FDB_ERR:
            cli_out (cli, "%% Failed to clear mac filter from bridge\n");
            return CLI_ERROR;
          default:
            cli_out (cli, "%% Unable to clear forwarding table\n" );
            return CLI_ERROR;
        }
    }

  return CLI_SUCCESS;
}

ALI (nsm_bridge_clear_fdb,
     nsm_bridge_clear_fdb_cmd,
     "clear mac address-table (static|multicast)",
     "Clear the Forwarding database",
     "Clear layer 2 MAC entries",
     "Clear all Entries in the forwarding database",
     "Clear all entries configured through management",
     "Clear all multicast entries");

CLI (nsm_bridge_clear_fdb_vlan_port,
     nsm_bridge_clear_br_fdb_vlan_port_cmd,
     "clear mac address-table (static|multicast) (address MACADDR | interface IFNAME | vlan VID) bridge <1-32>",
     "Clear the Forwarding database",
     "Clear layer 2 MAC entries",
     "Clear all Entries in the forwarding database",
     "Clear all entries configured through management",
     "Clear all multicast entries",
     "Clear the specified MAC Address",
     "Mac Address",  
     "Clear all mac address for the specified interface",
     "Interface Name",
     "Clear all mac address for the specified vlan",
     "Vlan Id (1-4094)",
     NSM_BRIDGE_STR,
     NSM_BRIDGE_NAME_STR)
{
  int ret = 0 ;
  int brno = 0;
  struct nsm_bridge *bridge;
  struct interface *ifp = NULL;
  struct nsm_master *nm = cli->vr->proto;
  struct nsm_bridge_master *master = nsm_bridge_get_master (nm);
#ifdef HAVE_VLAN
  u_int16_t vid = NSM_VLAN_NULL_VID;
#endif /* HAVE_VLAN */
  u_int8_t mac_addr [ETHER_ADDR_LEN];

  if (! master)
    {
      cli_out (cli, "%% Bridge master not configured\n");
      return CLI_ERROR;
    }

  if (argc == 4)
    {
      CLI_GET_INTEGER_RANGE ("Max bridge groups", brno, argv[3], 1, 32);
      bridge = nsm_lookup_bridge_by_name (master, argv[3]);
    }
  else
    bridge = nsm_get_default_bridge (master);

  if (! bridge)
    {
      cli_out (cli, "%% Bridge not found \n");
      return CLI_ERROR;
    }

  if (! pal_strncmp (argv[0], "s", 1))
    {
      if (! pal_strncmp (argv [1], "i", 1))
         {
           if ( (ifp = if_lookup_by_name(&cli->vr->ifm, argv[2])) == NULL )
             {
               cli_out (cli, "%% Interface not found\n");
               return CLI_ERROR;
             }
           ret = nsm_bridge_clear_static_mac_port (master, bridge->name, ifp);
         }

#ifdef HAVE_VLAN
      else if (! pal_strncmp (argv [1], "v",1))
         {
           /* Get the vid and check if it is within range (2-4094) */
           CLI_GET_INTEGER_RANGE ("vlanid", vid, argv[2], NSM_VLAN_DEFAULT_VID,
                                  NSM_VLAN_CLI_MAX);
           ret = nsm_bridge_clear_static_mac_vlan (master,  bridge->name, vid);
         }
#endif
     else if (! pal_strncmp (argv [1], "a",1))
       {
         NSM_READ_MAC_ADDRESS (argv[2], mac_addr);
 
         /* Convert to network order */
         *(unsigned short *)&mac_addr[0] = pal_hton16(*(unsigned short *)&mac_addr[0]);
         *(unsigned short *)&mac_addr[2] = pal_hton16(*(unsigned short *)&mac_addr[2]);
         *(unsigned short *)&mac_addr[4] = pal_hton16(*(unsigned short *)&mac_addr[4]);

         ret = nsm_bridge_clear_static_mac_addr (master,  bridge->name, mac_addr);

       }
      else
        {
          cli_out (cli, "%% Invalid Command \n");
          return CLI_ERROR;
        }  
    }
  else
    {

      if (! pal_strncmp (argv [1], "i", 1))
        {
          if ( (ifp = if_lookup_by_name(&cli->vr->ifm, argv[2])) == NULL )
            {
              cli_out (cli, "%% Interface not found\n");
              return CLI_ERROR;
            }
          ret = nsm_bridge_clear_multicast_mac_port (master, bridge->name, ifp);
        }

#ifdef HAVE_VLAN
      else if (! pal_strncmp (argv [1], "v",1))
        {
          /* Get the vid and check if it is within range (2-4094) */
          CLI_GET_INTEGER_RANGE ("vlanid", vid, argv[2], NSM_VLAN_DEFAULT_VID,
                                 NSM_VLAN_CLI_MAX);
          ret = nsm_bridge_clear_multicast_mac_vlan (master, bridge->name, vid);
        }
#endif
      else if (! pal_strncmp (argv [1], "a",1))
        {
          NSM_READ_MAC_ADDRESS (argv[2], mac_addr);

          /* Convert to network order */
          *(unsigned short *)&mac_addr[0] = pal_hton16(*(unsigned short *)&mac_addr[0]);
          *(unsigned short *)&mac_addr[2] = pal_hton16(*(unsigned short *)&mac_addr[2]);
          *(unsigned short *)&mac_addr[4] = pal_hton16(*(unsigned short *)&mac_addr[4]);

         if (!(mac_addr[0] & 1))
           {
             cli_out (cli, "%% Not a Multicast mac address \n");
             return CLI_ERROR;
           }

          ret = nsm_bridge_clear_multicast_mac_addr (master, bridge->name, mac_addr);
                                                                                                                             
        }
      else
        {
          cli_out (cli, "%% Invalid Command \n");
          return CLI_ERROR;
        }
    }

  if (ret < 0 )
    {
      switch (ret)
        {
          case NSM_BRIDGE_ERR_NOTFOUND:
            cli_out (cli, "%% Bridge not found\n");
            return CLI_ERROR;
          case NSM_HAL_FDB_ERR:
            cli_out (cli, "%% Failed to clear mac filter from bridge\n");
            return CLI_ERROR;
#ifdef HAVE_VLAN
          case NSM_VLAN_ERR_VLAN_NOT_FOUND:
            cli_out (cli, "%% VLAN Not a member of the bridge-group\n");
            return CLI_ERROR;
#endif
          default:
            cli_out (cli, "%% Unable to clear forwarding table\n" );
            return CLI_ERROR;
        }
    }

  return CLI_SUCCESS;

}

ALI (nsm_bridge_clear_fdb_vlan_port,
     nsm_bridge_clear_fdb_vlan_port_cmd,
     "clear mac address-table (static|multicast) (address MACADDR | interface IFNAME | vlan VID)",
     "Clear the Forwarding database",
     "Clear layer 2 MAC entries",
     "Clear all Entries in the forwarding database",
     "Clear all entries configured through management",
     "Clear all multicast entries",
     "Clear the specified MAC Address",
     "Mac Address",
     "Clear all mac address for the specified interface",
     "Interface Name",
     "Clear all mac address for the specified vlan",
     "Vlan Id (1-4094)");

CLI (nsm_bridge_clear_dynamic_fdb_bridge,
     nsm_bridge_clear_dynamic_br_fdb_bridge_cmd,
     "clear mac address-table dynamic bridge <1-32>",
     "Clear the Forwarding database",
     "Clear layer 2 MAC entries",
     "Clear all Entries in the forwarding database",
     "Clear all dynamic entries",
     NSM_BRIDGE_STR,
     NSM_BRIDGE_NAME_STR)
{
  int ret = 0;
  int brno = 0;
  struct nsm_bridge *bridge;
  struct nsm_master *nm = cli->vr->proto;
  struct nsm_bridge_master *master = nsm_bridge_get_master (nm);

  if (argc == 1)
    {
      CLI_GET_INTEGER_RANGE ("Max bridge groups", brno, argv[0], 1, 32);
      bridge = nsm_lookup_bridge_by_name (master, argv[0]);
    }
  else
    bridge = nsm_get_default_bridge (master);

  if (! bridge)
    {
      cli_out (cli, "%% Bridge not found \n");
      return CLI_ERROR;
    }

  ret = nsm_bridge_clear_dynamic_mac_bridge (master, bridge->name);

  if (ret < 0 )
    {
      switch (ret)
        {
          case NSM_BRIDGE_ERR_NOTFOUND:
            cli_out (cli, "%% Bridge not found\n");
            return CLI_ERROR;
          case NSM_HAL_FDB_ERR:
            cli_out (cli, "%% Failed to clear mac filter from bridge\n");
            return CLI_ERROR;
          default:
            cli_out (cli, "%% Unable to clear forwarding table\n" );
            return CLI_ERROR;
        }
    }

  return CLI_SUCCESS;

}

ALI (nsm_bridge_clear_dynamic_fdb_bridge,
     nsm_bridge_clear_dynamic_fdb_bridge_cmd,
     "clear mac address-table dynamic",
     "Clear the Forwarding database",
     "Clear layer 2 MAC entries",
     "Clear all Entries in the forwarding database",
     "Clear all dynamic entries");

CLI (nsm_bridge_clear_dynamic_fdb,
     nsm_bridge_clear_dynamic_br_fdb_cmd,
     "clear mac address-table dynamic (address MACADDR | interface IFNAME (instance INST|) | vlan VID) bridge <1-32>",
     "Clear the Forwarding database",
     "Clear layer 2 MAC entries",
     "Clear all Entries in the forwarding database",
     "Clear all dynamic entries",
     "Clear the specified MAC Address",
     "Mac Address",  
     "Clear all mac address for the specified interface",
     "Interface Name",
     "MSTP instance id",
     "<1-63>",
     "Clear all mac address for the specified vlan",
     "Vlan Id (1-4094)",
     NSM_BRIDGE_STR,
     NSM_BRIDGE_NAME_STR)
{

  int ret = 0;
  int brno = 0;
  struct nsm_bridge *bridge;
  struct nsm_master *nm = cli->vr->proto;
  struct nsm_bridge_master *master = nsm_bridge_get_master (nm);
  struct interface *ifp = NULL;
#ifdef HAVE_VLAN
  u_int16_t vid = NSM_VLAN_NULL_VID;
#endif /* HAVE_VLAN */
  u_int8_t mac_addr [ETHER_ADDR_LEN];

  if (! master)
    {
      cli_out (cli, "%% Bridge master not configured\n");
      return CLI_ERROR;
    }

  if (argc == 3)
    {
      CLI_GET_INTEGER_RANGE ("Max bridge groups", brno, argv[2], 1, 32);
      bridge = nsm_lookup_bridge_by_name (master, argv[2]);
    }
  else if (argc == 5)
    {
      CLI_GET_INTEGER_RANGE ("Max bridge groups", brno, argv[4], 1, 32);
      bridge = nsm_lookup_bridge_by_name (master, argv[4]);
    }
  else
    bridge = nsm_get_default_bridge (master);

  if (! bridge)
    {
      cli_out (cli, "%% Bridge not found \n");
      return CLI_ERROR;
    }
                                                                                                                             
  if (! pal_strncmp (argv[0], "int", 3))
    {
      int instance = 0;
      if ( (ifp = if_lookup_by_name(&cli->vr->ifm, argv[1])) == NULL )
        {
          cli_out (cli, "%% Interface not found\n");
          return CLI_ERROR;
        }
      if ((argc > 2) && (! pal_strncmp (argv[2], "ins", 3)))
        {
		CLI_GET_INTEGER_RANGE ("instance", instance, argv[3], 
                                NSM_BRIDGE_INSTANCE_MIN + 1, 
                                bridge->max_mst_instances - 1);
        }
      ret = nsm_bridge_clear_dynamic_mac_port (master, ifp, bridge->name, 
                                               instance);
    }
#ifdef HAVE_VLAN
  else if (! pal_strncmp (argv[0], "v", 1))
    {
      /* Get the vid and check if it is within range (2-4094) */
      CLI_GET_INTEGER_RANGE ("vlanid", vid, argv[1], NSM_VLAN_DEFAULT_VID,
                             NSM_VLAN_CLI_MAX);

      ret = nsm_bridge_clear_dynamic_mac_vlan (master, vid, bridge->name);
    }
#endif
  else if (!  pal_strncmp (argv[0], "a",1))
    {
      NSM_READ_MAC_ADDRESS (argv[1], mac_addr);

      /* Convert to network order */
      *(unsigned short *)&mac_addr[0] = pal_hton16(*(unsigned short *)&mac_addr[0]);
      *(unsigned short *)&mac_addr[2] = pal_hton16(*(unsigned short *)&mac_addr[2]);
      *(unsigned short *)&mac_addr[4] = pal_hton16(*(unsigned short *)&mac_addr[4]);


      ret = nsm_bridge_clear_dynamic_mac_addr (master, mac_addr, bridge->name);
    }
  else
    {
      cli_out (cli, "%% Invalid Command \n");
      return CLI_ERROR;
    }

  if (ret < 0 )
    {
      switch (ret)
        {
          case NSM_BRIDGE_ERR_NOTFOUND:
            cli_out (cli, "%% Bridge not found\n");
            return CLI_ERROR;
          case NSM_HAL_FDB_ERR:
            cli_out (cli, "%% Failed to delete mac filter from bridge\n");
            return CLI_ERROR;
#ifdef HAVE_VLAN 
          case NSM_VLAN_ERR_VLAN_NOT_FOUND:
            cli_out (cli, "%% VLAN Not a member of the bridge-group\n");
            return CLI_ERROR;
#endif
          default:
            cli_out (cli, "%% Unable to clear forwarding table\n" );
            return CLI_ERROR;
        }
    }
                                                                                                                             
  return CLI_SUCCESS;

}

ALI (nsm_bridge_clear_dynamic_fdb,
     nsm_bridge_clear_dynamic_fdb_cmd,
     "clear mac address-table dynamic (address MACADDR | interface IFNAME (instance INST|) | vlan VID)",
     "Clear the Forwarding database",
     "Clear layer 2 MAC entries",
     "Clear all Entries in the forwarding database",
     "Clear all dynamic entries",
     "Clear the specified MAC Address",
     "Mac Address",
     "Clear all mac address for the specified interface",
     "Interface Name",
     "Clear all mac address for the specified vlan",
     "Vlan Id (1-4094)");

CLI (no_nsm_bridge_protocol,
     no_nsm_bridge_protocol_cmd,
     "no bridge (<1-32> | backbone)",
     CLI_NO_STR,
     NSM_BRIDGE_STR,
     NSM_BRIDGE_NAME_STR,
     "De-associates Backbone bridge")
{
  struct nsm_master *nm = cli->vr->proto;
  int ret;
  struct nsm_bridge_master *master = nsm_bridge_get_master (nm);
  int brno;

  if (! master)
    {
      cli_out (cli, "%% Bridge master not configured\n");
      return CLI_ERROR;
    }

  if (argv[0][0]!='b')
  CLI_GET_INTEGER_RANGE ("Max bridge groups", brno, argv[0], 1, 32);

  ret = nsm_bridge_delete (master, argv[0]);
  if (ret < 0)
    {
      cli_out (cli, "%% Bridge %s not found. \n", argv[0]);
      return CLI_ERROR;
    }  

  return CLI_SUCCESS;
}

#ifdef HAVE_PROVIDER_BRIDGE

CLI (nsm_bridge_clear_fdb_pro_edge_vlan,
     nsm_bridge_clear_fdb_pro_edge_vlan_cmd,
     "clear mac address-table (dynamic|static|multicast) cvlan VID "
     "svlan VID bridge <1-32>",
     "Clear the Forwarding database",
     "Clear layer 2 MAC entries",
     "Clear all Entries in the forwarding database",
     "Clear all Learned entries",
     "Clear all entries configured through management",
     "Clear all multicast entries",
     "Clear all mac address for the specified cvlan",
     "CVlan Id (1-4094)",
     "Clear all mac address for the specified svlan",
     "SVlan Id (1-4094)",
     NSM_BRIDGE_STR,
     NSM_BRIDGE_NAME_STR)
{
  int ret = 0 ;
  int brno = 0;
  struct nsm_bridge *bridge;
  struct nsm_master *nm = cli->vr->proto;
  struct nsm_bridge_master *master = nsm_bridge_get_master (nm);
  u_int16_t vid = NSM_VLAN_NULL_VID;
  u_int16_t svid = NSM_VLAN_NULL_VID;

  if (! master)
    {
      cli_out (cli, "%% Bridge master not configured\n");
      return CLI_ERROR;
    }

  if (argc == 4)
    {
      CLI_GET_INTEGER_RANGE ("Max bridge groups", brno, argv[3], 1, 32);
      bridge = nsm_lookup_bridge_by_name (master, argv[3]);
    }
  else
    bridge = nsm_get_default_bridge (master);

  if (! bridge)
    {
      cli_out (cli, "%% Bridge not found \n");
      return CLI_ERROR;
    }

  /* Get the vid and check if it is within range (1-4094) */
  CLI_GET_INTEGER_RANGE ("cvlanid", vid, argv[1], NSM_VLAN_DEFAULT_VID,
                          NSM_VLAN_CLI_MAX);

  CLI_GET_INTEGER_RANGE ("svlanid", svid, argv[2], NSM_VLAN_DEFAULT_VID,
                          NSM_VLAN_CLI_MAX);

  if (! pal_strncmp (argv[0], "s", 1))
    ret = nsm_bridge_clear_static_mac_pro_edge_vlan
                                      (master,  bridge->name, vid, svid);
  else if (! pal_strncmp (argv[0], "m", 1))
    ret = nsm_bridge_clear_multicast_mac_pro_edge_vlan 
                                         (master,  bridge->name, vid, svid);
  else
    ret = nsm_bridge_clear_multicast_mac_pro_edge_vlan 
                                         (master,  bridge->name, vid, svid);

  if (ret < 0 )
    {
      switch (ret)
        {
          case NSM_BRIDGE_ERR_NOTFOUND:
            cli_out (cli, "%% Bridge not found\n");
            return CLI_ERROR;
          case NSM_HAL_FDB_ERR:
            cli_out (cli, "%% Failed to clear mac filter from bridge\n");
            return CLI_ERROR;
#ifdef HAVE_VLAN
          case NSM_VLAN_ERR_VLAN_NOT_FOUND:
            cli_out (cli, "%% VLAN Not a member of the bridge-group\n");
            return CLI_ERROR;
#endif
          default:
            cli_out (cli, "%% Unable to clear forwarding table\n" );
            return CLI_ERROR;
        }
    }

  return CLI_SUCCESS;

}

ALI (nsm_bridge_clear_fdb_pro_edge_vlan,
     nsm_bridge_clear_fdb_def_pro_edge_vlan_cmd,
     "clear mac address-table (dynamic|static|multicast) cvlan VID svlan VID",
     "Clear the Forwarding database",
     "Clear layer 2 MAC entries",
     "Clear all Entries in the forwarding database",
     "Clear all Learned entries",
     "Clear all entries configured through management",
     "Clear all multicast entries",
     "Clear all mac address for the specified cvlan",
     "CVlan Id (1-4094)",
     "Clear all mac address for the specified svlan",
     "SVlan Id (1-4094)");

#endif /* HAVE_PROVIDER_BRIDGE */

CLI (nsm_bridge_group,
     nsm_bridge_group_cmd,
     "bridge-group (<1-32> | backbone)",
     NSM_BRIDGE_STR,
     NSM_BRIDGE_NAME_STR,
     "Associates the interface with the B-component backbone bridge.")
{
  int ret;
  struct interface *ifp = cli->index;
  struct nsm_master *nm = cli->vr->proto;
  struct nsm_bridge_master *master = nsm_bridge_get_master (nm);
  int brno;

#ifdef HAVE_LACPD
  NSM_INTERFACE_CHECK_AGG_MEM_PROPERTY(cli, ifp);
#endif /* HAVE_LACPD */
  NSM_BRIDGE_CHECK_L2_PROPERTY(cli, ifp);
  
  if (!CHECK_FLAG(ifp->status, NSM_INTERFACE_ACTIVE))
    {
      cli_out (cli, "%% Interface %s is pseudo interface\n", ifp->name);
      return CLI_ERROR;
    }
     
  if (! master)
    {
      cli_out (cli, "%% Bridge master not configured\n");
      return CLI_ERROR;
    }
  if ( argv[0][0] != 'b')
    CLI_GET_INTEGER_RANGE ("Max bridge groups", brno, argv[0], 1, 32);

  /* Add the port to bridge and enable the spanning-tree */
  ret = nsm_bridge_api_port_spanning_tree_enable (master, argv[0], ifp, 
                                                  PAL_FALSE);
  switch (ret)
    {
    case NSM_BRIDGE_ERR_NOTFOUND:
      cli_out (cli, "%% Interface not bound\n");
      return CLI_ERROR;
    case NSM_BRIDGE_ERR_ALREADY_BOUND:
      cli_out (cli, "%% Interface already bound to a bridge\n");
      return CLI_ERROR;
    case NSM_BRIDGE_ERR_EXISTS:
      cli_out (cli, "%% Interface spanning-tree mode already exist\n");
      return CLI_ERROR;
    default:
      return CLI_SUCCESS;
    }
}

CLI (nsm_bridge_span_disable,
     nsm_bridge_span_disable_cmd,
     "spanning-tree (disable|enable)",
     "spannning tree command",
     "Disable spanning tree on the interface for default bridge",
     "Enable spanning tree on the interface for default bridge")
{
  int ret;
  struct interface *ifp = cli->index;
  struct nsm_master *nm = cli->vr->proto;
  struct nsm_bridge_master *master = nsm_bridge_get_master (nm);
  u_int8_t mode = 0;

  NSM_BRIDGE_CHECK_L2_PROPERTY(cli, ifp);

  if (!CHECK_FLAG(ifp->status, NSM_INTERFACE_ACTIVE))
    {
      cli_out (cli, "%% Interface %s is pseudo interface\n", ifp->name);
      return CLI_ERROR;
    }

  if (! master)
    {
      cli_out (cli, "%% Bridge master not configured\n");
      return CLI_ERROR;
    }
  
  if (argc == 1)
    {
      if ((pal_strncmp (argv[0], "e", 1) == 0))
        mode = PAL_FALSE;
      else if ((pal_strncmp (argv[0], "d", 1) == 0))
        mode = PAL_TRUE;
    }

  /* Enable/Disable the spanning-tree for the default bridge*/
  ret = nsm_bridge_api_port_spanning_tree_enable (master, "0",
                                                  ifp, mode);
  switch (ret)
    {
    case NSM_BRIDGE_ERR_NOTFOUND:
      cli_out (cli, "%% Interface not bound \n");
      return CLI_ERROR;
    case NSM_BRIDGE_ERR_ALREADY_BOUND:
      cli_out (cli, "%% Interface already bound to a bridge\n");
      return CLI_ERROR;
    case NSM_BRIDGE_ERR_EXISTS:
      cli_out (cli, "%% spanning-tree mode already exist\n");
      return CLI_ERROR;
    default:
      return CLI_SUCCESS;
    }

}

CLI (nsm_bridge_group_span,
     nsm_bridge_group_span_cmd,
     "bridge-group <1-32> spanning-tree (disable|enable)",
     NSM_BRIDGE_STR,
     NSM_BRIDGE_NAME_STR,
     "spannning tree commmand",
     "Disable spanning tree on the interface",
     "Enable spanning tree on the interface")
{
  int ret;
  struct interface *ifp = cli->index;
  struct nsm_master *nm = cli->vr->proto;
  struct nsm_bridge_master *master = nsm_bridge_get_master (nm);
  int brno;
  u_int8_t mode = 0;

#ifdef HAVE_LACPD
  NSM_INTERFACE_CHECK_AGG_MEM_PROPERTY(cli, ifp);
#endif /* HAVE_LACPD */
  NSM_BRIDGE_CHECK_L2_PROPERTY(cli, ifp);

  if (!CHECK_FLAG(ifp->status, NSM_INTERFACE_ACTIVE))
    {
      cli_out (cli, "%% Interface %s is pseudo interface\n", ifp->name);
      return CLI_ERROR;
    }

  if (! master)
    {
      cli_out (cli, "%% Bridge master not configured\n");
      return CLI_ERROR;
    }

  if (argc == 2)
    {
      if ((pal_strncmp (argv[1], "e", 1) == 0))
        mode = PAL_FALSE;
      else if ((pal_strncmp (argv[1], "d", 1) == 0))
        mode = PAL_TRUE;
    }

  CLI_GET_INTEGER_RANGE ("Max bridge groups", brno, argv[0], 1, 32);

  /* Enable/Disable the spanning-tree*/
  ret = nsm_bridge_api_port_spanning_tree_enable (master, argv[0], ifp, mode);

  switch (ret)
    {
    case NSM_BRIDGE_ERR_NOTFOUND:
      cli_out (cli, "%% Interface not bound\n");
      return CLI_ERROR;
    case NSM_BRIDGE_ERR_ALREADY_BOUND:
      cli_out (cli, "%% Interface already bound to a bridge\n");
      return CLI_ERROR;
    case NSM_BRIDGE_ERR_EXISTS:
      cli_out (cli, "%% spanning-tree mode already exist\n");
      return CLI_ERROR;
    default:
      return CLI_SUCCESS;
    }
}

CLI (no_nsm_bridge_group,
     no_nsm_bridge_group_cmd,
     "no bridge-group (<1-32> | backbone)",
     CLI_NO_STR,
     NSM_BRIDGE_STR,
     NSM_BRIDGE_NAME_STR
     "De-associates the interface with the B-component backbone bridge.")
{
  int ret;
  struct interface *ifp = cli->index;
  struct nsm_master *nm = cli->vr->proto;
  struct nsm_bridge_master *master = nsm_bridge_get_master (nm);
  int brno;

#ifdef HAVE_LACPD
  NSM_INTERFACE_CHECK_AGG_MEM_PROPERTY(cli, ifp);
#endif /* HAVE_LACPD */
  NSM_BRIDGE_CHECK_L2_PROPERTY(cli, ifp);

  if (! master)
    {
      cli_out (cli, "%% Bridge master not configured\n");
      return CLI_ERROR;
    }

  if ( argv[0][0] != 'b')
  CLI_GET_INTEGER_RANGE ("Max bridge groups", brno, argv[0], 1, 32);

  ret = nsm_bridge_port_delete (master, argv[0], ifp, PAL_TRUE, PAL_TRUE);

  if (ret < 0)
    {
      cli_out (cli, "%% Can't delete port from bridge. \n");
      return CLI_ERROR;
    }

  return CLI_SUCCESS;
}

CLI (switchport,
     switchport_cmd,
     "switchport",
     "Switchport")
{
  char *str = NULL;
  int ret = 0;
  struct interface *ifp = cli->index;
  bool_t allowed;

#ifdef HAVE_LACPD
  NSM_INTERFACE_CHECK_AGG_MEM_PROPERTY(cli, ifp);
#endif /* HAVE_LACPD */

  allowed = nsm_check_l2_commands_allowed (cli, ifp);
  if (allowed == PAL_FALSE)
    return CLI_ERROR;

#ifdef HAVE_GMPLS
  if ((ifp->gmpls_type != PHY_PORT_TYPE_GMPLS_DATA) &&
      (ifp->gmpls_type != PHY_PORT_TYPE_GMPLS_UNKNOWN))
    {
      /* Cannot allow a control link to be made into Layer-2 link */
      cli_out (cli, "%% Change GMPLS interface Type first\n");
      return CLI_ERROR;
    }
#endif /* HAVE_GMPLS */
#ifdef HAVE_VR
   if (ifp->vr && (ifp->vr->id != VRF_ID_UNSPEC))
    {
      cli_out (cli, "%% Interface is bound to virtual-router.\n");
      return CLI_ERROR;
    }
#endif /* HAVE_VR */
  ret = nsm_bridge_if_set_switchport (ifp);
  if ( ret < 0 )
    {
      switch (ret)
        {
          case NSM_INTERFACE_NOT_FOUND :
            str = "Interface not found";
            break;
          case NSM_INTERFACE_L2_MODE :
            str = "Interface already a switchport";
            break;
          case NSM_INTERFACE_IN_USE :
            str = "Please clear interface binding first";
            break;
          default :
            str = "Can't set port to switchport";
            break;
        }
    }

  if (str)
    {
      cli_out (cli, "%% %s\n", str);
      return CLI_ERROR;
    }

  return CLI_SUCCESS;
}

#ifdef HAVE_L3
CLI (no_switchport,
     no_switchport_cmd,
     "no switchport",
     CLI_NO_STR,
     "Switchport")
{
  char *str = NULL;
  int ret;
  struct interface *ifp = cli->index;
  bool_t allowed;

#ifdef HAVE_LACPD
  NSM_INTERFACE_CHECK_AGG_MEM_PROPERTY(cli, ifp);
#endif /* HAVE_LACPD */

  allowed = nsm_check_l2_commands_allowed (cli, ifp);
  if (allowed == PAL_FALSE)
    return CLI_ERROR;

  ret = nsm_bridge_if_unset_switchport (ifp);
  if ( ret < 0 )
  {
      switch (ret)
        {
          case NSM_INTERFACE_IN_USE :
            str = "Please clear interface binding first.";
            break;
          default:
            str = "Can't set port to router port.";
            break;
        }
  }

  if (str)
    {
      cli_out (cli, "%% %s\n", str);
      return CLI_ERROR;
    }

  return CLI_SUCCESS;
}
#endif /* HAVE_L3 */

#ifdef HAVE_PROVIDER_BRIDGE
/* CLI to enable UNI Type */
CLI (nsm_bridge_uni_type,
     nsm_bridge_uni_type_cmd,
     "ethernet uni type (enable | disable) (bridge <1-32>|)",
     "Configure Metro Ethernet Parameter",
     "Configure UNI Parameter",
     "configure UNI type enable/disable",
     "uni type enable will update the uni type",
     "uni type disable will set the default uni type",
     "bridge group commands",
     "Enter the bridge name on which the uni type",
     "need to enabled/disabled")
{
  s_int32_t ret;
  struct nsm_master *nm = cli->vr->proto;
  struct nsm_bridge_master *master = nsm_bridge_get_master (nm);
  struct nsm_bridge *bridge = NULL;
  u_int8_t brno = PAL_FALSE;
  u_int8_t flag = PAL_FALSE;

  if (! master)
    {
      cli_out (cli, "%% Bridge master not configured\n");
      return CLI_ERROR;
    }

  if (argv[0][0] == 'e')
    flag = NSM_UNI_TYPE_ENABLE;
  else
    flag = NSM_UNI_TYPE_DISABLE;

  if (argc == 3)
    {
      CLI_GET_INTEGER_RANGE ("Max bridge groups", brno, argv[2], 1, 32);
      /* API to set the uni type */
      ret = nsm_bridge_uni_type_detect (argv[2], flag, master); 
    }
  else
    {
      bridge = nsm_get_default_bridge (master);

      if (! bridge)
        {
          cli_out (cli, "%% Default Bridge not found\n");
          return CLI_ERROR;
        }
      /* API to set the uni type */
      ret = nsm_bridge_uni_type_detect (bridge->name, flag, master); 

    }

 
  if (ret == RESULT_OK)
    {
      return CLI_SUCCESS;
    }
  else
    {
      nsm_bridge_display_error_message (cli, ret); 
      return CLI_ERROR;
    }
  return CLI_SUCCESS;
}

/**@brief  - Function to display the uni type and mode for uni.
 * @param cli         - Pointer to imish terminal.
 * @param interface   - Interface for which the uni type need to be displayed.
 * @return            - return control to the CLI.
 * */
static void
nsm_show_uni_type (struct cli *cli, struct interface *ifp)
{
  struct nsm_if *zif = NULL;
  struct nsm_bridge_port *br_port = NULL;

  zif = (struct nsm_if *)ifp->info;

  if (zif)
    {
      if (zif->bridge && zif->switchport)
        {
          br_port = zif->switchport;
          cli_out (cli, "\n");
          
          cli_out (cli, "----------------------------------\n");
          cli_out (cli, "Interface Name         :  %s\n", ifp->name);
          cli_out (cli, "----------------------------------\n");

          cli_out (cli, "\nUNI Type               :  %s\n", 
              ((br_port->uni_type_status.uni_type == NSM_UNI_TYPE_2) ? 
               "UNI Type 2" : "UNI Type 1"));
          if (br_port->uni_type_status.uni_type == NSM_UNI_TYPE_2)
            {
              cli_out (cli, "UNI Type 2 mode        :  %s\n", 
                  ((br_port->uni_type_status.uni_type_2_status == 
                    NSM_UNI_TYPE_TWO_ONE) ? 
                   "UNI Type 2.1" : "UNI Type 2.2"));
            }
          cli_out (cli, "UNI TYPE Mode          :  %s\n",
              (br_port->uni_type_status.uni_type_mode == NSM_UNI_TYPE_ENABLE) ?
              "enable" : "disable");
          cli_out (cli, "CFM Operational Status :  %s\n",
              (br_port->uni_type_status.cfm_status == NSM_CFM_OPERATIONAL) ?
              "Operational" : "Not Operational");
          cli_out (cli, "E-LMI Operational Status : %s\n",
             (br_port->uni_type_status.elmi_status == NSM_ELMI_OPERATIONAL) ?
             "Operational" : "Not Operational");
         cli_out (cli,"\n"); 
        }

    }
  return;
}

/* SHOW Command for UNI Types */
CLI (nsm_show_ethernet_uni_type,
     nsm_show_ethernet_uni_type_cmd,
     "show ethernet uni type (interface IF_NAME | (bridge <1-32>|))",
     "Show running system information",
     "ethernet parameters",
     "User network interface",
     "uni type as per MEF 11",
     "interface for which the uni type need to be checked",
     "enter the interface name",
     "bridge-group command",
     "enter the bridge name")
{
  u_int8_t brno = PAL_FALSE;
  struct interface *ifp = NULL;
  struct nsm_master *nm = cli->vr->proto;
  struct nsm_bridge_master *master = nsm_bridge_get_master (nm);
  struct nsm_bridge                *bridge = NULL;
  struct nsm_if                    *zif = NULL;
  struct nsm_bridge_port           *br_port = NULL;
  struct nsm_vlan_port             *vlan_port = NULL;
  struct avl_node *avl_port;


  if ((argc == 2) && (argv[0][0] == 'i'))
    {
      ifp = if_lookup_by_name (&cli->vr->ifm, argv[1]);

      if (ifp == NULL) 
        {
          cli_out (cli, "%% Can't find interface %s\n", argv[1]);
          return CLI_ERROR;
        }

     zif = (struct nsm_if *)ifp->info;

     if (zif == NULL || zif->switchport == NULL)
       {
         cli_out (cli, "%% Interface not associated to bridge\n");
         return CLI_ERROR;
       }

    br_port = zif->switchport;

    vlan_port = &br_port->vlan_port;

    if ( !vlan_port )
      {
        cli_out (cli, "%% Interface not associated to bridge\n");
        return CLI_ERROR; 
      }

    if (NSM_BRIDGE_TYPE_PROVIDER (zif->bridge) &&
        (vlan_port->mode != NSM_VLAN_PORT_MODE_CE))
      {
        cli_out (cli, "%% Interface is not a customer edge port.\n");
        return CLI_ERROR;
      }

      nsm_show_uni_type (cli, ifp);
    }
  else
    {
      if (! master)
        {
          cli_out (cli, "%% Bridge master not configured\n");
          return CLI_ERROR;
        }
      if (argc == 2)
        {
          CLI_GET_INTEGER_RANGE ("Max bridge groups", brno, argv[1], 1, 32);

          bridge = nsm_lookup_bridge_by_name (master, argv[1]);
        }
      else
        {
          bridge = nsm_get_default_bridge (master);
        }

      if (bridge == NULL)
        {
          cli_out (cli, "bridge not found\n");
          return CLI_ERROR;
        }
      cli_out (cli, "\n");
      cli_out (cli, "Bridge Name         :  %s\n", bridge->name);
      cli_out (cli, "==================================\n");
     
      for (avl_port = avl_top (bridge->port_tree); avl_port;
          avl_port = avl_next (avl_port))
        {
          if ((br_port = (struct nsm_bridge_port *)
                AVL_NODE_INFO (avl_port)) == NULL)
            continue;

          if ((zif = br_port->nsmif) == NULL)
            continue;

          if ((ifp = zif->ifp) == NULL)
            continue;

          vlan_port = &br_port->vlan_port;

          if (! vlan_port)
            continue;

          if (NSM_BRIDGE_TYPE_PROVIDER (bridge) &&
              (vlan_port->mode != NSM_VLAN_PORT_MODE_CE))
            {
              continue;
            }

          nsm_show_uni_type (cli, ifp);

        }
    }
  
 return CLI_SUCCESS; 
  
}
/* CLI to enable UNI Type */
CLI (nsm_interface_uni_type,
     nsm_interface_uni_type_cmd,
     "ethernet uni type (enable | disable)",
     "Configure Metro Ethernet Parameter",
     "Configure UNI Parameter",
     "configure UNI type enable/disable",
     "uni type enable will update the uni type",
     "uni type disable will set the default uni type")
{
  s_int32_t ret;
  u_int8_t flag;
  struct interface *ifp;

  ifp = cli->index;

#ifdef HAVE_LACPD
  NSM_INTERFACE_CHECK_AGG_MEM_PROPERTY(cli, ifp);
#endif /* HAVE_LACPD */
  NSM_BRIDGE_CHECK_L2_PROPERTY(cli, ifp);

  if (!CHECK_FLAG(ifp->status, NSM_INTERFACE_ACTIVE))
    {
      cli_out (cli, "%% Interface %s is pseudo interface\n", ifp->name);
      return CLI_ERROR;
    }

  if (argv[0][0] == 'e')
    flag = NSM_UNI_TYPE_ENABLE;
  else
    flag = NSM_UNI_TYPE_DISABLE;

  ret = nsm_uni_type_detect (ifp, flag);

 
 
  if (ret < 0)
    {
      cli_out (cli, "%% UNI Type not %s\n", (flag == NSM_UNI_TYPE_ENABLE)?
                       "enabled" : "disabled");
      return CLI_ERROR;
    }
  return CLI_SUCCESS;

}


CLI (nsm_bridge_protocol_discard,
     nsm_bridge_protocol_discard_cmd,
     "l2protocol (stp|gmrp|mmrp|gvrp|mvrp|dot1x|lacp) discard",
     "Configure Layer2 Protocol Tunnelling",
     "Spanning Tree Protocols",
     "GARP Multicast Registration Protocol",
     "MRP Multicast Registration Protocol",
     "GARP VLAN Registration Protocol",
     "MRP VLAN Registration Protocol",
     "Port Authentication (802.1 X)",
     "Link Aggregation (LACP)",
     "Discard the protocol data unit")
{
  int ret;
  struct interface *ifp;

  ifp = cli->index;

#ifdef HAVE_LACPD
  NSM_INTERFACE_CHECK_AGG_MEM_PROPERTY(cli, ifp);
#endif /* HAVE_LACPD */
  NSM_BRIDGE_CHECK_L2_PROPERTY(cli, ifp);

  if (!CHECK_FLAG(ifp->status, NSM_INTERFACE_ACTIVE))
    {
      cli_out (cli, "%% Interface %s is pseudo interface\n", ifp->name);
      return CLI_ERROR;
    }

  ret = nsm_bridge_api_port_proto_process (ifp,
                            nsm_map_str_to_hal_protocol (argv [0]),
                            HAL_L2_PROTO_DISCARD,
                            PAL_TRUE, NSM_VLAN_DEFAULT_VID);

  switch (ret)
    {
    case NSM_BRIDGE_ERR_NOTFOUND:
      cli_out (cli, "%% Interface not bound\n");
      return CLI_ERROR;
    case NSM_BRIDGE_ERR_ALREADY_BOUND:
      cli_out (cli, "%% Interface already bound to a bridge\n");
      return CLI_ERROR;
    case NSM_BRIDGE_ERR_INVALID_MODE:
      cli_out (cli, "%% Invalid mode for the command\n");
      return CLI_ERROR;
    default:
      return CLI_SUCCESS;
    }
}

CLI (nsm_bridge_protocol_peer,
     nsm_bridge_protocol_peer_cmd,
     "l2protocol (stp|gmrp|mmrp|dot1x|lacp) peer",
     "Configure Layer2 Protocol Tunnelling",
     "Spanning Tree Protocols",
     "GARP Multicast Registration Protocol",
     "MRP Multicast Registration Protocol",
     "Port Authentication (802.1 X)",
     "Link Aggregation (LACP)",
     "Act as peer to the customer Device instance of the protocol")
{
  int ret;
  struct interface *ifp;

  ifp = cli->index;

#ifdef HAVE_LACPD
  NSM_INTERFACE_CHECK_AGG_MEM_PROPERTY(cli, ifp);
#endif /* HAVE_LACPD */
  NSM_BRIDGE_CHECK_L2_PROPERTY(cli, ifp);

  if (!CHECK_FLAG(ifp->status, NSM_INTERFACE_ACTIVE))
    {
      cli_out (cli, "%% Interface %s is pseudo interface\n", ifp->name);
      return CLI_ERROR;
    }

  ret = nsm_bridge_api_port_proto_process (ifp,
                            nsm_map_str_to_hal_protocol (argv [0]),
                            HAL_L2_PROTO_PEER,
                            PAL_TRUE, NSM_VLAN_NULL_VID);

  switch (ret)
    {
    case NSM_BRIDGE_ERR_NOTFOUND:
      cli_out (cli, "%% Interface not bound\n");
      return CLI_ERROR;
    case NSM_BRIDGE_ERR_ALREADY_BOUND:
      cli_out (cli, "%% Interface already bound to a bridge\n");
      return CLI_ERROR;
    case NSM_BRIDGE_ERR_INVALID_MODE:
      cli_out (cli, "%% Invalid mode for the command\n");
      return CLI_ERROR;
    default:
      return CLI_SUCCESS;
    }

  return CLI_SUCCESS;
}

CLI (nsm_bridge_protocol_tunnel,
     nsm_bridge_protocol_tunnel_cmd,
     "l2protocol (stp|gvrp|mvrp|dot1x|lacp) tunnel (VLAN_ID|)",
     "Configure Layer2 Protocol Tunnelling",
     "Spanning Tree Protocols",
     "GARP VLAN Registration Protocol",
     "MRP  VLAN Registration Protocol",
     "Port Authentication (802.1 X)",
     "Link Aggregation (LACP)",
     "Tunnel the Protocol data unit into the SVLAN",
     "SVLAN ID to be tunneled")
{
  int ret;
  u_int16_t svid;
  struct interface *ifp;

  ifp = cli->index;

#ifdef HAVE_LACPD
  NSM_INTERFACE_CHECK_AGG_MEM_PROPERTY(cli, ifp);
#endif /* HAVE_LACPD */
  NSM_BRIDGE_CHECK_L2_PROPERTY(cli, ifp);

  if (!CHECK_FLAG(ifp->status, NSM_INTERFACE_ACTIVE))
    {
      cli_out (cli, "%% Interface %s is pseudo interface\n", ifp->name);
      return CLI_ERROR;
    }

  if (argc == 2)
    CLI_GET_INTEGER_RANGE ("S-VLAN id", svid, argv[1], NSM_VLAN_DEFAULT_VID,
                           NSM_VLAN_CLI_MAX);
  else
    svid = NSM_VLAN_DEFAULT_VID;

  ret = nsm_bridge_api_port_proto_process (ifp,
                            nsm_map_str_to_hal_protocol (argv [0]),
                            HAL_L2_PROTO_TUNNEL,
                            PAL_TRUE, svid);

  switch (ret)
    {
    case NSM_BRIDGE_ERR_NOTFOUND:
      cli_out (cli, "%% Interface not bound\n");
      return CLI_ERROR;
    case NSM_BRIDGE_ERR_ALREADY_BOUND:
      cli_out (cli, "%% Interface already bound to a bridge\n");
      return CLI_ERROR;
    case NSM_BRIDGE_ERR_INVALID_MODE:
      cli_out (cli, "%% Invalid mode for the command\n");
    case NSM_VLAN_ERR_VLAN_NOT_FOUND:
      cli_out (cli, "%% S-VLAN %d not found\n", svid);		
      return CLI_ERROR;
    default:
      return CLI_SUCCESS;
    }

  return CLI_SUCCESS;
}

CLI (nsm_bridge_gmrp_tunnel,
     nsm_bridge_gmrp_tunnel_cmd,
     "l2protocol (gmrp|mmrp) tunnel",
     "Configure Layer2 Protocol Tunnelling",
     "GARP Multicast Registration Protocol",
     "MRP Multicast Registration Protocol",
     "Tunnel the Protocol Data Units into the provider network")
{
  int ret;
  struct interface *ifp;

  ifp = cli->index;

#ifdef HAVE_LACPD
  NSM_INTERFACE_CHECK_AGG_MEM_PROPERTY(cli, ifp);
#endif /* HAVE_LACPD */
  NSM_BRIDGE_CHECK_L2_PROPERTY(cli, ifp);

  if (!CHECK_FLAG(ifp->status, NSM_INTERFACE_ACTIVE))
    {
      cli_out (cli, "%% Interface %s is pseudo interface\n", ifp->name);
      return CLI_ERROR;
    }

  ret = nsm_bridge_api_port_proto_process (ifp,
                            nsm_map_str_to_hal_protocol (argv [0]),
                            HAL_L2_PROTO_TUNNEL,
                            PAL_TRUE, NSM_VLAN_NULL_VID);

  switch (ret)
    {
    case NSM_BRIDGE_ERR_NOTFOUND:
      cli_out (cli, "%% Interface not bound\n");
      return CLI_ERROR;
    case NSM_BRIDGE_ERR_ALREADY_BOUND:
      cli_out (cli, "%% Interface already bound to a bridge\n");
      return CLI_ERROR;
    case NSM_BRIDGE_ERR_INVALID_MODE:
      cli_out (cli, "%% Invalid mode for the command\n");
      return CLI_ERROR;
    default:
      return CLI_SUCCESS;
    }

  return CLI_SUCCESS;
}

void
nsm_bridge_l2protocol_show (struct cli *cli,
                            struct interface *ifp,
                            bool_t *print_header)
{
  struct nsm_if *zif;
  enum hal_l2_proto proto;
  struct nsm_bridge *bridge;
  struct nsm_bridge_port *br_port;

  if (!ifp || !print_header || !( zif = ifp->info)
      || !(bridge = zif->bridge) || !(br_port = zif->switchport))
    return;

  if (*print_header)
    {
      cli_out (cli, "%-10s%-32s%-10s%-18s%-10d\n", "Bridge", "Interface Name",
                    "Protocol", "Processing Status VLANID");
      cli_out (cli, "%-10s%-32s%-10s%-18s\n", "======", "==============",
                    "========", "================= ======");
      *print_header = PAL_FALSE;
    }

  for (proto = HAL_PROTO_STP; proto < HAL_PROTO_MAX; proto++)
     {
       if (proto == HAL_PROTO_RSTP || proto == HAL_PROTO_MSTP)
         continue;

       cli_out (cli, "%-10s%-32s%-10s%-18s%-10d\n", bridge->name, ifp->name,
                nsm_map_hal_proto_to_str (proto),
                nsm_bridge_get_process_str (ifp, proto),
                nsm_bridge_get_process_vid (ifp, proto));
     }

}

CLI (nsm_bridge_protocol_process_show,
     nsm_bridge_protocol_process_show_cmd,
     "show l2protocol processing (interface IFNAME|)",
     CLI_SHOW_STR,
     "Show Layer2 Protocol Tunnelling",
     CLI_INTERFACE_STR, CLI_IFNAME_STR)
{
  struct apn_vr *vr;
  struct nsm_if *zif;
  struct interface *ifp;
  struct listnode *node;
  bool_t print_header = PAL_TRUE;

  vr = cli->vr;

  if (argc != 0)
    {
      ifp = if_lookup_by_name (&vr->ifm, argv[1]);

      if (!ifp || !( zif = ifp->info))
        {
          cli_out (cli, "%% Can't find interface %s\n", argv[1]);
          return CLI_ERROR;
        }

      if (!zif->bridge)
        {
          cli_out (cli, "%% Interface Not bound to port %s\n", argv[1]);
          return CLI_ERROR;
        }
      nsm_bridge_l2protocol_show (cli, ifp, &print_header);
    }
  else
    {
      LIST_LOOP (vr->ifm.if_list, ifp, node)
       {
         if (!ifp || ! (zif = ifp->info)
            || !zif->bridge || ifp->ifindex < 0)
           continue;

         nsm_bridge_l2protocol_show (cli, ifp, &print_header);
       }
    }

  return CLI_SUCCESS;
}

CLI (nsm_bridge_service_attr,
     nsm_bridge_service_attr_cmd,
     "ethernet uni (bundle | all-to-one | multiplex)",
     "Configure Metro Ethernet Parameter",
     "Configure UNI Parameter",
     "UNI supports bundling without multiplexing",
     "UNI supports multiplexing without bundling",
     "UNI supports all to one bundling")
{
  int ret;
  struct interface *ifp;

  ifp = cli->index;

#ifdef HAVE_LACPD
  NSM_INTERFACE_CHECK_AGG_MEM_PROPERTY(cli, ifp);
#endif /* HAVE_LACPD */
  NSM_BRIDGE_CHECK_L2_PROPERTY(cli, ifp);

  if (!CHECK_FLAG(ifp->status, NSM_INTERFACE_ACTIVE))
    {
      cli_out (cli, "%% Interface %s is pseudo interface\n", ifp->name);
      return CLI_ERROR;
    }

  ret = nsm_bridge_uni_set_service_attr (ifp,
                            nsm_map_str_to_service_attr (argv [0]));

  switch (ret)
    {
    case NSM_BRIDGE_ERR_NOTFOUND:
      cli_out (cli, "%% Interface not bound\n");
      return CLI_ERROR;
    case NSM_PRO_VLAN_ERR_CVLAN_MAP_ERR:
      cli_out (cli, "%% CVLAN Map Contradicts the service attribute \n");
      return CLI_ERROR;
    default:
      return CLI_SUCCESS;
    }

  return CLI_SUCCESS;
}

CLI (no_nsm_bridge_service_attr,
     no_nsm_bridge_service_attr_cmd,
     "no ethernet uni (bundle | all-to-one | multiplex)",
     CLI_NO_STR,
     "Configure Metro Ethernet Parameter",
     "Configure UNI Parameter",
     "UNI supports bundling without multiplexing",
     "UNI supports multiplexing without bundling",
     "UNI supports all to one bundling")
{
  int ret;
  struct interface *ifp;

  ifp = cli->index;

#ifdef HAVE_LACPD
  NSM_INTERFACE_CHECK_AGG_MEM_PROPERTY(cli, ifp);
#endif /* HAVE_LACPD */
  NSM_BRIDGE_CHECK_L2_PROPERTY(cli, ifp);

  if (!CHECK_FLAG(ifp->status, NSM_INTERFACE_ACTIVE))
    {
      cli_out (cli, "%% Interface %s is pseudo interface\n", ifp->name);
      return CLI_ERROR;
    }

  ret = nsm_bridge_uni_unset_service_attr (ifp,
                            nsm_map_str_to_service_attr (argv [0]));

  switch (ret)
    {
    case NSM_BRIDGE_ERR_NOTFOUND:
      cli_out (cli, "%% Interface not bound\n");
      return CLI_ERROR;
    case NSM_PRO_VLAN_ERR_INVALID_SERVICE_ATTR:
      cli_out (cli, "%% Service Attribute of port is different from "
               "the given Service Attribute\n");
      return CLI_ERROR;
    default:
      return CLI_SUCCESS;
    }

  return CLI_SUCCESS;
}

CLI (nsm_bridge_uni_name,
     nsm_bridge_uni_name_cmd,
     "ethernet uni id NAME",
     "Configure Metro Ethernet Parameter",
     "Configure UNI Parameter",
     "Configure UNI Name",
     "UNI Name")
{
  int ret;
  struct interface *ifp;

  ifp = cli->index;

#ifdef HAVE_LACPD
  NSM_INTERFACE_CHECK_AGG_MEM_PROPERTY(cli, ifp);
#endif /* HAVE_LACPD */
  NSM_BRIDGE_CHECK_L2_PROPERTY(cli, ifp);

  if (!CHECK_FLAG(ifp->status, NSM_INTERFACE_ACTIVE))
    {
      cli_out (cli, "%% Interface %s is pseudo interface\n", ifp->name);
      return CLI_ERROR;
    }

  ret = nsm_bridge_uni_set_name (ifp, argv [0]);

  if (ret == RESULT_OK)
    {
      return CLI_SUCCESS;
    }
  else
    {
      nsm_bridge_display_error_message (cli, ret); 
      return CLI_ERROR;
    }

  return CLI_SUCCESS;
}

CLI (no_nsm_bridge_uni_name,
     no_nsm_bridge_uni_name_cmd,
     "no ethernet uni id",
     CLI_NO_STR,
     "Configure Metro Ethernet Parameter",
     "Configure UNI Parameter",
     "Configure UNI Name")
{
  int ret;
  struct interface *ifp;

  ifp = cli->index;

#ifdef HAVE_LACPD
  NSM_INTERFACE_CHECK_AGG_MEM_PROPERTY(cli, ifp);
#endif /* HAVE_LACPD */
  NSM_BRIDGE_CHECK_L2_PROPERTY(cli, ifp);

  if (!CHECK_FLAG(ifp->status, NSM_INTERFACE_ACTIVE))
    {
      cli_out (cli, "%% Interface %s is pseudo interface\n", ifp->name);
      return CLI_ERROR;
    }

  ret = nsm_bridge_uni_unset_name (ifp);

  if (ret == RESULT_OK)
    {
      return CLI_SUCCESS;
    }
  else
    {
      nsm_bridge_display_error_message (cli, ret); 
      return CLI_ERROR;
    }

  return CLI_SUCCESS;
}

/* CLI To configure MAX EVC per UNI as per Table 16 MEF 10.1*/
CLI (nsm_uni_max_evc_cli,
     nsm_uni_max_evc_cmd,
     "ethernet uni max-evc <1-4094>",
     "Configure Metro Ethernet Parameter",
     "Configure UNI Parameter",
     "configure maximum number of EVCs allowed",
     "enter the maximum number of EVCs for the UNI")
{
  int ret;
  struct interface *ifp;
  u_int16_t max_evc = PAL_FALSE;

  ifp = cli->index;

#ifdef HAVE_LACPD
  NSM_INTERFACE_CHECK_AGG_MEM_PROPERTY(cli, ifp);
#endif /* HAVE_LACPD */
  NSM_BRIDGE_CHECK_L2_PROPERTY(cli, ifp);

  if (!CHECK_FLAG(ifp->status, NSM_INTERFACE_ACTIVE))
    {
      cli_out (cli, "%% Interface %s is pseudo interface\n", ifp->name);
      return CLI_ERROR;
    }

  CLI_GET_INTEGER_RANGE ("MAx EVC", max_evc, argv[0], 1, 4094);
  
  /* API To set MAX EVC */
  ret = nsm_uni_set_max_evc (ifp, max_evc);

  if (ret == RESULT_OK)
    {
      return CLI_SUCCESS;
    }
  else
    {
      nsm_bridge_display_error_message (cli, ret); 
      return CLI_ERROR;
    }
  return CLI_SUCCESS;

}

 CLI (no_nsm_uni_max_evc_cli,
     no_nsm_uni_max_evc_cmd,
     "no ethernet uni max-evc",
     CLI_NO_STR,
     "Configure Metro Ethernet Parameter",
     "Configure UNI Parameter",
     "configure maximum number of EVCs allowed")
{
  int ret;
  struct interface *ifp;

  ifp = cli->index;

#ifdef HAVE_LACPD
  NSM_INTERFACE_CHECK_AGG_MEM_PROPERTY(cli, ifp);
#endif /* HAVE_LACPD */
  NSM_BRIDGE_CHECK_L2_PROPERTY(cli, ifp);

  if (!CHECK_FLAG(ifp->status, NSM_INTERFACE_ACTIVE))
    {
      cli_out (cli, "%% Interface %s is pseudo interface\n", ifp->name);
      return CLI_ERROR;
    }

  ret = nsm_uni_unset_max_evc (ifp);

  if (ret == RESULT_OK)
    {
      return CLI_SUCCESS;
    }
  else
    {
      nsm_bridge_display_error_message (cli, ret); 
      return CLI_ERROR;
    }
  return CLI_SUCCESS;

}

/**@brief  - Function to display the max evc for uni
 * @param cli         - Pointer to imish terminal.
 * @param bridge      - nsm_bridge interfaces for which the max evc
 *                         need to displayed.
 * @param interface   - Interface for which the max evc need to be
 *                         displayed.
 * @return       - return control to the CLI.
 * */
static void
nsm_show_uni_max_evc (struct cli *cli, struct nsm_bridge *bridge,
    struct interface *ifp)
{
  struct nsm_if *zif = NULL;
  struct nsm_bridge_port *br_port = NULL;
  struct nsm_vlan_port *vlan_port = NULL;
  struct avl_node *port_node = NULL;

  cli_out (cli, "% Bridge-Name    Interface-Name     MAX-EVCs \n");
  cli_out (cli, "% ------------------------------------------- \n");
  
  if (ifp != NULL)
    {
      zif = ifp->info;

      if (zif && zif->switchport)
        {
          br_port = zif->switchport;
        }
      else
        return;

      vlan_port = &br_port->vlan_port;

      if (!vlan_port)
        return;

      if (vlan_port->mode != NSM_VLAN_PORT_MODE_CE)
        return;
     
      cli_out (cli, "%% %s              %s                %d      \n",
          bridge->name, ifp->name,
          br_port->uni_max_evcs);
    }
  else
    {
      AVL_TREE_LOOP (bridge->port_tree, br_port, port_node)
        {

          if ((zif = br_port->nsmif) == NULL)
            continue;

          if ((ifp = zif->ifp) == NULL)
            continue;

          vlan_port = &br_port->vlan_port;

          if (vlan_port->mode != NSM_VLAN_PORT_MODE_CE)
            continue;

          cli_out (cli, "%% %s              %s                %d      \n",
              bridge->name, ifp->name,
              br_port->uni_max_evcs);

        }

    }
  return;
}

CLI (nsm_show_uni_max_evc_cli,
    nsm_show_uni_max_evc_cmd,
    "show ethernet uni max-evc (uni-id UNI_NAME | interface INTERFACE_NAME | "
    "( bridge <1-32>|))",
    CLI_SHOW_STR,
    "ethernet service parameters",
    "uni parameters",
    "max evc of the UNI",
    "uni id for which the max EVC need to be displayed",
    "enter the uni id",
    "interface for which the max EVC need to be displayed",
    "enter the interface name",
    "brief will display the max evcs of all the interfaces",
    "bridge for which the max evc need to be displayed",
    "enter the bridge name")
{
  struct nsm_if *zif = NULL;
  struct interface *ifp = NULL;
  struct nsm_bridge *bridge = NULL;
  struct nsm_vlan_port *vlan_port = NULL;
  struct nsm_bridge_port *br_port = NULL;
  struct nsm_master *nm = cli->vr->proto;
  struct nsm_bridge_master *master = nsm_bridge_get_master (nm);

  if (argc == 2)
    {
      if (argv[0][0] == 'i')
        {
          if (! (ifp = if_lookup_by_name (&cli->vr->ifm, argv[1]))
              || ! (zif = ifp->info)
              || ! (br_port = zif->switchport)
              || ! (vlan_port = &br_port->vlan_port))
            {
              cli_out (cli, "%% Interface does not exist: %s\n", argv[1]);
              return CLI_ERROR;
            }
          if (! (bridge = zif->bridge))
            {
              cli_out (cli, "%% Interface not bound to bridge: %s\n", argv[1]);
              return CLI_ERROR;
            }

          if (! NSM_BRIDGE_TYPE_PROVIDER (bridge))
            {
              cli_out (cli, "%% Interface not a bound to a provider bridge: %s\n",
                  argv[0]);
              return CLI_ERROR;
            }

          if (vlan_port->mode != NSM_VLAN_PORT_MODE_CE)
            {
              cli_out (cli, "%% Port is not Customer Edge Port\n");
              return CLI_ERROR;
            }
          nsm_show_uni_max_evc (cli, bridge, ifp);
        }
      else if (argv[0][0] == 'u')
        {
          ifp = nsm_if_lookup_by_uni_id (&cli->vr->ifm, argv[1]);

          if (ifp == NULL)
            {
              cli_out (cli, "%% Interface does not exist: %s\n", argv[1]);
              return CLI_ERROR;
            }

          if (! (zif = ifp->info) ||
              ! (br_port = zif->switchport) ||
              ! (vlan_port = &br_port->vlan_port))
            {
              cli_out (cli, "%% Interface does not exist: %s\n", argv[1]);
              return CLI_ERROR;
            }   
          if (! (bridge = zif->bridge))
            {
              cli_out (cli, "%% Interface not bound to bridge: %s\n", argv[1]);
              return CLI_ERROR;
            }

          if (! NSM_BRIDGE_TYPE_PROVIDER (bridge))
            {
              cli_out (cli, "%% Interface not a bound to a provider bridge: %s\n",
                  argv[0]);
              return CLI_ERROR;
            }
          if (vlan_port->mode != NSM_VLAN_PORT_MODE_CE)
            {
              cli_out (cli, "%% Port is not Customer Edge Port\n");
              return CLI_ERROR;
            }

          nsm_show_uni_max_evc (cli, bridge, ifp);
        }
    }

  if ((argc == 2) && (argv[0][0] == 'b'))
    {
      bridge = nsm_lookup_bridge_by_name(master, argv[1]);
      if (! bridge)
        {
          cli_out (cli, "%% Bridge not found\n");
          return CLI_ERROR;
        }
      if (! NSM_BRIDGE_TYPE_PROVIDER (bridge))
        {
          cli_out (cli, "%% bridge is not a provider edge bridge\n");
          return CLI_ERROR;
        }

      nsm_show_uni_max_evc (cli, bridge, NULL);

    }
  else if (argc == 0)
    {
      bridge = nsm_get_default_bridge (master);
      if (! bridge)
        {
          cli_out (cli, "%% Bridge not found\n");
          return CLI_ERROR;
        }
      if (! NSM_BRIDGE_TYPE_PROVIDER (bridge))
        {
          cli_out (cli, "%% bridge is not a provider edge bridge\n");
          return CLI_ERROR;
        }
      if (NSM_BRIDGE_TYPE_PROVIDER (bridge) &&
          (bridge->provider_edge == PAL_FALSE))
        {
          cli_out (cli, "%% bridge is not a provider edge bridge\n");
          return CLI_ERROR;
        }

      nsm_show_uni_max_evc (cli, bridge, NULL);

    }

  return CLI_SUCCESS;
}

    

CLI (nsm_bw_profile_uni_set_cir,
     nsm_bw_profile_uni_set_cir_cmd,
     "(ingress-policing | egress-shaping) uni cir <0-10000000>",
     "configure ingress policing parameters",
     "configure egress shaping parameters",
     "Configure UNI Parameter",
     "configure committed information rate (CIR)",
     "enter the cir ranging 0bps -10Mbps")
{
  s_int32_t ret = PAL_FALSE;
  struct interface *ifp = NULL;
  u_int32_t cir = PAL_FALSE;

  ifp = cli->index;

#ifdef HAVE_LACPD
  NSM_INTERFACE_CHECK_AGG_MEM_PROPERTY(cli, ifp);
#endif /* HAVE_LACPD */
  NSM_BRIDGE_CHECK_L2_PROPERTY(cli, ifp);

  if (!CHECK_FLAG(ifp->status, NSM_INTERFACE_ACTIVE))
    {
      cli_out (cli, "%% Interface %s is pseudo interface\n", ifp->name);
      return CLI_ERROR;
    }

  CLI_GET_UINT32_RANGE ("Committed Information Rate", cir, argv[1], 0, 
      10000000);

  if (argv[0][0] == 'i')
    {
      ret = nsm_uni_bw_profiling (ifp, cir, NSM_UNI_DEFAULT, NSM_UNI_DEFAULT,
          NSM_UNI_DEFAULT, NSM_UNI_DEFAULT, NSM_UNI_DEFAULT,
          NSM_INGRESS_POLICING, NSM_BW_PARAMETER_CIR);
    }
  else
    {
      ret = nsm_uni_bw_profiling (ifp, cir, NSM_UNI_DEFAULT, NSM_UNI_DEFAULT,
          NSM_UNI_DEFAULT, NSM_UNI_DEFAULT, NSM_UNI_DEFAULT,
          NSM_EGRESS_SHAPING, NSM_BW_PARAMETER_CIR);
    }
  if (ret == RESULT_OK)
    {
      return CLI_SUCCESS;
    }
  else
    {
      nsm_bridge_display_error_message (cli, ret); 
      return CLI_ERROR;
    }
  return CLI_SUCCESS;
}

CLI (nsm_bw_profile_uni_set_cbs,
     nsm_bw_profile_uni_set_cbs_cmd,
     "(ingress-policing | egress-shaping) uni cbs <0-20000>",
     "configure ingress policing parameters",
     "configure egress shaping parameters",
     "Configure UNI Parameter",
     "configure committed burst size (CBS)",
     "enter the cbs ranging 0-20000 bytes")
{
  s_int32_t ret = PAL_FALSE;
  struct interface *ifp;
  u_int32_t cbs;

  ifp = cli->index;

#ifdef HAVE_LACPD
  NSM_INTERFACE_CHECK_AGG_MEM_PROPERTY(cli, ifp);
#endif /* HAVE_LACPD */
  NSM_BRIDGE_CHECK_L2_PROPERTY(cli, ifp);

  if (!CHECK_FLAG(ifp->status, NSM_INTERFACE_ACTIVE))
    {
      cli_out (cli, "%% Interface %s is pseudo interface\n", ifp->name);
      return CLI_ERROR;
    }

  CLI_GET_UINT32_RANGE ("Committed Burst Size", cbs, argv[1], 0, 
      20000);
  if (argv[0][0] == 'i')
    {
      ret = nsm_uni_bw_profiling (ifp, NSM_UNI_DEFAULT, cbs, NSM_UNI_DEFAULT,
          NSM_UNI_DEFAULT, NSM_UNI_DEFAULT, 
          NSM_UNI_DEFAULT, NSM_INGRESS_POLICING, NSM_BW_PARAMETER_CBS);
    }
  else
    {
      ret = nsm_uni_bw_profiling (ifp, NSM_UNI_DEFAULT, cbs, NSM_UNI_DEFAULT,
          NSM_UNI_DEFAULT, NSM_UNI_DEFAULT, NSM_UNI_DEFAULT,
          NSM_EGRESS_SHAPING, NSM_BW_PARAMETER_CBS);
    }

  if (ret == RESULT_OK)
    {
      return CLI_SUCCESS;
    }
  else
    {
      nsm_bridge_display_error_message (cli, ret); 
      return CLI_ERROR;
    }
  return CLI_SUCCESS;

}

CLI (nsm_bw_profile_uni_set_eir,
     nsm_bw_profile_uni_set_eir_cmd,
     "(ingress-policing | egress-shaping) uni eir <0-20000000>",
     "configure ingress policing parameters",
     "configure egress shaping parameters",
     "Configure UNI Parameter",
     "configure excess information rate (EIR)",
     "enter the eir ranging 0bps -20Mbps")
{
  s_int32_t ret;
  struct interface *ifp;
  u_int32_t eir = PAL_FALSE;

  ifp = cli->index;

#ifdef HAVE_LACPD
  NSM_INTERFACE_CHECK_AGG_MEM_PROPERTY(cli, ifp);
#endif /* HAVE_LACPD */
  NSM_BRIDGE_CHECK_L2_PROPERTY(cli, ifp);

  if (!CHECK_FLAG(ifp->status, NSM_INTERFACE_ACTIVE))
    {
      cli_out (cli, "%% Interface %s is pseudo interface\n", ifp->name);
      return CLI_ERROR;
    }

  CLI_GET_UINT32_RANGE ("Excess Information Rate", eir, argv[1], 0, 
      20000000);

  if (argv[0][0] == 'i')
    {
      ret = nsm_uni_bw_profiling (ifp, NSM_UNI_DEFAULT, NSM_UNI_DEFAULT, eir,
          NSM_UNI_DEFAULT, NSM_UNI_DEFAULT, 
          NSM_UNI_DEFAULT, NSM_INGRESS_POLICING, NSM_BW_PARAMETER_EIR);

    }
  else
    {
      ret = nsm_uni_bw_profiling (ifp, 0, 0, eir, 0, 0, 
          0, NSM_EGRESS_SHAPING, NSM_BW_PARAMETER_EIR);

    }

  if (ret == RESULT_OK)
    {
      return CLI_SUCCESS;
    }
  else
    {
      nsm_bridge_display_error_message (cli, ret); 
      return CLI_ERROR;
    }
  return CLI_SUCCESS;

}

CLI (nsm_bw_profile_uni_set_ebs,
     nsm_bw_profile_uni_set_ebs_cmd,
     "(ingress-policing | egress-shaping) uni ebs <0-20000>",
     "configure ingress policing parameters",
     "configure egress shaping parameters",
     "Configure UNI Parameter",
     "configure excess burst size (EBS)",
     "enter the ebs ranging 0-20000")
{
  s_int32_t ret;
  struct interface *ifp;
  u_int32_t ebs = PAL_FALSE;

  ifp = cli->index;

#ifdef HAVE_LACPD
  NSM_INTERFACE_CHECK_AGG_MEM_PROPERTY(cli, ifp);
#endif /* HAVE_LACPD */
  NSM_BRIDGE_CHECK_L2_PROPERTY(cli, ifp);

  if (!CHECK_FLAG(ifp->status, NSM_INTERFACE_ACTIVE))
    {
      cli_out (cli, "%% Interface %s is pseudo interface\n", ifp->name);
      return CLI_ERROR;
    }

  CLI_GET_UINT32_RANGE ("Excess Burst Size", ebs, argv[1], 0, 
      20000);

  if (argv[0][0] == 'i')
    {
      ret = nsm_uni_bw_profiling (ifp, NSM_UNI_DEFAULT, NSM_UNI_DEFAULT,
          NSM_UNI_DEFAULT, ebs, NSM_UNI_DEFAULT, 
          NSM_UNI_DEFAULT, NSM_INGRESS_POLICING, NSM_BW_PARAMETER_EBS);

    }
  else
    {
      ret = nsm_uni_bw_profiling (ifp, NSM_UNI_DEFAULT, NSM_UNI_DEFAULT,
          NSM_UNI_DEFAULT, ebs, NSM_UNI_DEFAULT, 
          NSM_UNI_DEFAULT, NSM_EGRESS_SHAPING, NSM_BW_PARAMETER_EBS);
    }

  if (ret == RESULT_OK)
    {
      return CLI_SUCCESS;
    }
  else
    {
      nsm_bridge_display_error_message (cli, ret); 
      return CLI_ERROR;
    }
  return CLI_SUCCESS;

}

CLI (nsm_bw_profile_uni_set_coupling_flag,
     nsm_bw_profile_uni_set_coupling_flag_cmd,
     "(ingress-policing | egress-shaping) uni coupling-flag "
     "(enable | disable)",
     "configure ingress policing parameters",
     "configure egress shaping parameters",
     "Configure UNI Parameter",
     "configure coupling-flag per ingress uni",
     "enable the coupling-flag per ingress UNI",
     "disable  the coupling-flag per ingress UNI")
{
  s_int32_t ret;
  struct interface *ifp;
  u_int8_t coupling_flag = PAL_FALSE;

  ifp = cli->index;

#ifdef HAVE_LACPD
  NSM_INTERFACE_CHECK_AGG_MEM_PROPERTY(cli, ifp);
#endif /* HAVE_LACPD */
  NSM_BRIDGE_CHECK_L2_PROPERTY(cli, ifp);

  if (!CHECK_FLAG(ifp->status, NSM_INTERFACE_ACTIVE))
    {
      cli_out (cli, "%% Interface %s is pseudo interface\n", ifp->name);
      return CLI_ERROR;
    }

  if (argv[1][0] == 'e')
    {
      coupling_flag = PAL_TRUE;
    }
  if (argv[0][0] == 'i')
    {
      ret = nsm_uni_bw_profiling (ifp, NSM_UNI_DEFAULT, NSM_UNI_DEFAULT,
          NSM_UNI_DEFAULT, NSM_UNI_DEFAULT, coupling_flag, NSM_UNI_DEFAULT,
          NSM_INGRESS_POLICING, NSM_BW_PARAMETER_COUPLING_FLAG);
    }
  else
    {
       ret = nsm_uni_bw_profiling (ifp, NSM_UNI_DEFAULT, NSM_UNI_DEFAULT,
           NSM_UNI_DEFAULT, NSM_UNI_DEFAULT, coupling_flag,
           NSM_UNI_DEFAULT, NSM_EGRESS_SHAPING, NSM_BW_PARAMETER_COUPLING_FLAG);
    }

  if (ret == RESULT_OK)
    {
      return CLI_SUCCESS;
    }
  else
    {
      nsm_bridge_display_error_message (cli, ret); 
      return CLI_ERROR;
    }
  return CLI_SUCCESS;

}

CLI (nsm_bw_profile_uni_set_color_mode,
     nsm_bw_profile_uni_set_color_mode_cmd,
     "(ingress-policing | egress-shaping) uni color-mode "
     "(color-blind | color-aware)",
     "configure ingress policing parameters",
     "configure egress shaping parameters",
     "Configure UNI Parameter",
     "configure color-mode per uni",
     "configure color-blind mode per UNI",
     "configure color-aware mode per UNI")
{
  s_int32_t ret;
  struct interface *ifp;
  u_int8_t color_mode = PAL_FALSE;

  ifp = cli->index;

#ifdef HAVE_LACPD
  NSM_INTERFACE_CHECK_AGG_MEM_PROPERTY(cli, ifp);
#endif /* HAVE_LACPD */
  NSM_BRIDGE_CHECK_L2_PROPERTY(cli, ifp);

  if (!CHECK_FLAG(ifp->status, NSM_INTERFACE_ACTIVE))
    {
      cli_out (cli, "%% Interface %s is pseudo interface\n", ifp->name);
      return CLI_ERROR;
    }

  if (argv[1][6] == 'a')
    {
      color_mode = PAL_TRUE;
    }
  if (argv[0][0] == 'i')
    {
       ret = nsm_uni_bw_profiling (ifp, NSM_UNI_DEFAULT, NSM_UNI_DEFAULT,
           NSM_UNI_DEFAULT, NSM_UNI_DEFAULT, NSM_UNI_DEFAULT,
           color_mode, NSM_INGRESS_POLICING, NSM_BW_PARAMETER_COLOR_MODE);
    }
  else
    {
        ret = nsm_uni_bw_profiling (ifp, NSM_UNI_DEFAULT, NSM_UNI_DEFAULT,
            NSM_UNI_DEFAULT, NSM_UNI_DEFAULT, NSM_UNI_DEFAULT,
            color_mode, NSM_EGRESS_SHAPING, NSM_BW_PARAMETER_COLOR_MODE);
    }

  if (ret == RESULT_OK)
    {
      return CLI_SUCCESS;
    }
  else
    {
      nsm_bridge_display_error_message (cli, ret); 
      return CLI_ERROR;
    }
  return CLI_SUCCESS;

}

CLI (nsm_delete_uni_bw_profile,
     nsm_delete_uni_bw_profile_cmd,
     "no (ingress-policing | egress-shaping) uni",
     CLI_NO_STR,
     "unconfigure ingress policing parameters",
     "unconfigure egress shaping parameters",
     "unconfigure UNI Parameter")
{
  s_int32_t ret;
  struct interface *ifp;
  u_int8_t bw_profile_type = PAL_FALSE;

  ifp = cli->index;

#ifdef HAVE_LACPD
  NSM_INTERFACE_CHECK_AGG_MEM_PROPERTY(cli, ifp);
#endif /* HAVE_LACPD */
  NSM_BRIDGE_CHECK_L2_PROPERTY(cli, ifp);

  if (!CHECK_FLAG(ifp->status, NSM_INTERFACE_ACTIVE))
    {
      cli_out (cli, "%% Interface %s is pseudo interface\n", ifp->name);
      return CLI_ERROR;
    }

  if (argv[0][0] == 'i')
    {
      bw_profile_type = NSM_INGRESS_POLICING;
    }
  else
    {
      bw_profile_type = NSM_EGRESS_SHAPING;
    }

  ret = nsm_delete_uni_bw_profiling (ifp, bw_profile_type);
  
  if (ret == RESULT_OK)
    {
      return CLI_SUCCESS;
    }
  else
    {
      nsm_bridge_display_error_message (cli, ret); 
      return CLI_ERROR;
    }
  return CLI_SUCCESS;
}

CLI (nsm_activate_uni_bw_profile,
     nsm_activate_uni_bw_profile_cmd,
     "(ingress-policing | egress-shaping) uni (active | inactive)",
     CLI_NO_STR,
     "unconfigure ingress policing parameters",
     "unconfigure egress shaping parameters",
     "unconfigure UNI Parameter",
     "activate the bandwidth profiling comfigured",
     "inactivate the bandwidth profiling configured")
{
  s_int32_t ret;
  struct interface *ifp;
  u_int8_t bw_profile_type = PAL_FALSE;
  u_int8_t bw_profile_status = PAL_FALSE;

  ifp = cli->index;

#ifdef HAVE_LACPD
  NSM_INTERFACE_CHECK_AGG_MEM_PROPERTY(cli, ifp);
#endif /* HAVE_LACPD */
  NSM_BRIDGE_CHECK_L2_PROPERTY(cli, ifp);

  if (!CHECK_FLAG(ifp->status, NSM_INTERFACE_ACTIVE))
    {
      cli_out (cli, "%% Interface %s is pseudo interface\n", ifp->name);
      return CLI_ERROR;
    }

  if (argv[0][0] == 'i')
    {
      bw_profile_type = NSM_INGRESS_POLICING;
    }
  else
    {
      bw_profile_type = NSM_EGRESS_SHAPING;
    }

  if (argv[1][0] == 'a')
    {
      bw_profile_status = PAL_TRUE;
    }

  ret = nsm_set_uni_bw_profiling (ifp, bw_profile_type, bw_profile_status);
  
  if (ret == RESULT_OK)
    {
      return CLI_SUCCESS;
    }
  else
    {
      nsm_bridge_display_error_message (cli, ret); 
      return CLI_ERROR;
    }
  return CLI_SUCCESS;

}

/* CLI to return from EVC_SERVICE_INSTANCE_MODE
 */
CLI (nsm_exit_service_instance_bw_profile,
     nsm_exit_service_instance_bw_profile_cmd,
     "exit-service-instance-mode",
     "exit from EVC_SERVICE_INSTANCE_MODE")
{
  cli->mode = INTERFACE_MODE;

  return CLI_SUCCESS;
}

CLI (nsm_service_instance_bw_profile,
    nsm_service_instance_bw_profile_cmd,
    "service instance INSTANCE_ID evc-id EVC_ID",
    "evc service to which the bw profile parameters need to be configured",
    "instance id per evc",
    "enter the instance id that should be mapped for the evc",
    "evc-id of the svlan",
    "enter the EVC ID of the svlan")
{
  s_int32_t ret;
  u_int16_t instance_id;
  struct interface *ifp = NULL;
  struct nsm_uni_evc_bw_profile *nsm_uni_evc_bw_profile = NULL;
  
   ifp = cli->index;

#ifdef HAVE_LACPD
  NSM_INTERFACE_CHECK_AGG_MEM_PROPERTY(cli, ifp);
#endif /* HAVE_LACPD */
  NSM_BRIDGE_CHECK_L2_PROPERTY(cli, ifp);

  if (!CHECK_FLAG(ifp->status, NSM_INTERFACE_ACTIVE))
    {
      cli_out (cli, "%% Interface %s is pseudo interface\n", ifp->name);
      return CLI_ERROR;
    }

  CLI_GET_INTEGER_RANGE ("Instance id", instance_id, argv[0], 2, 
      4094);
  ret = nsm_uni_evc_set_service_instance (ifp, instance_id, argv[1], 
      &nsm_uni_evc_bw_profile);

  if (ret < 0)
    {
      cli_out (cli, "%% service instance not set for evc\n");
      return CLI_ERROR;
    }
  cli->index_sub = nsm_uni_evc_bw_profile;
  cli->mode = EVC_SERVICE_INSTANCE_MODE;
 return CLI_SUCCESS; 

}


CLI (nsm_unset_service_instance_bw_profile,
    nsm_unset_service_instance_bw_profile_cmd,
    "no service instance INSTANCE_ID evc-id EVC_ID",
    CLI_NO_STR,
    "evc service to which the bw profile parameters need to be configured",
    "instance id per evc",
    "enter the instance id that should be mapped for the evc",
    "evc-id of the svlan",
    "enter the EVC ID of the svlan")
{
  s_int32_t ret;
  u_int16_t instance_id;
  struct interface *ifp = NULL;
  
   ifp = cli->index;

#ifdef HAVE_LACPD
  NSM_INTERFACE_CHECK_AGG_MEM_PROPERTY(cli, ifp);
#endif /* HAVE_LACPD */
  NSM_BRIDGE_CHECK_L2_PROPERTY(cli, ifp);

  if (!CHECK_FLAG(ifp->status, NSM_INTERFACE_ACTIVE))
    {
      cli_out (cli, "%% Interface %s is pseudo interface\n", ifp->name);
      return CLI_ERROR;
    }

  CLI_GET_INTEGER_RANGE ("Instance id", instance_id, argv[0], 2, 
      4094);

  ret = nsm_unset_uni_evc_set_service_instance (ifp, instance_id, argv[1]);

  if (ret == RESULT_OK)
    {
      return CLI_SUCCESS;
    }
  else
    {
      nsm_bridge_display_error_message (cli, ret); 
      return CLI_ERROR;
    }
  return CLI_SUCCESS;

}

CLI (nsm_delete_evc_bw_profile,
     nsm_delete_evc_bw_profile_cmd,
     "no (ingress-policing | egress-shaping)",
     CLI_NO_STR,
     "unconfigure ingress policing parameters",
     "unconfigure egress shaping parameters")
{
  s_int32_t ret;
  struct interface *ifp = NULL;
  struct nsm_uni_evc_bw_profile *evc_bw_profile = NULL;
  u_int8_t bw_profile_type = PAL_FALSE;
  
  ifp = cli->index;
  
  evc_bw_profile = (struct nsm_uni_evc_bw_profile *)cli->index_sub;

#ifdef HAVE_LACPD
  NSM_INTERFACE_CHECK_AGG_MEM_PROPERTY(cli, ifp);
#endif /* HAVE_LACPD */
  NSM_BRIDGE_CHECK_L2_PROPERTY(cli, ifp);

  if (!CHECK_FLAG(ifp->status, NSM_INTERFACE_ACTIVE))
    {
      cli_out (cli, "%% Interface %s is pseudo interface\n", ifp->name);
      return CLI_ERROR;
    }

  if (argv[0][0] == 'i')
    bw_profile_type = NSM_INGRESS_POLICING;
  else
    bw_profile_type = NSM_EGRESS_SHAPING;

  ret = nsm_delete_evc_bw_profiling (ifp, evc_bw_profile, bw_profile_type);
 
  if (ret == RESULT_OK)
    {
      return CLI_SUCCESS;
    }
  else
    {
      nsm_bridge_display_error_message (cli, ret); 
      return CLI_ERROR;
    }
  return CLI_SUCCESS;
}

CLI (nsm_activate_evc_bw_profile,
     nsm_activate_evc_bw_profile_cmd,
     "(ingress-policing | egress-shaping) (active | inactive)",
     "ingress-policing per EVC",
     "egress-shaping per EVC",
     "activate bandwidth profile per EVC",
     "inactivate bandwidth profile per EVC")
{
  s_int32_t ret;
  struct interface *ifp = NULL;
  struct nsm_uni_evc_bw_profile *evc_bw_profile = NULL;
  u_int8_t bw_profile_type = PAL_FALSE;
  u_int8_t bw_profile_status = PAL_FALSE;
  
  ifp = cli->index;
  
  evc_bw_profile = (struct nsm_uni_evc_bw_profile *)cli->index_sub;

#ifdef HAVE_LACPD
  NSM_INTERFACE_CHECK_AGG_MEM_PROPERTY(cli, ifp);
#endif /* HAVE_LACPD */
  NSM_BRIDGE_CHECK_L2_PROPERTY(cli, ifp);

  if (!CHECK_FLAG(ifp->status, NSM_INTERFACE_ACTIVE))
    {
      cli_out (cli, "%% Interface %s is pseudo interface\n", ifp->name);
      return CLI_ERROR;
    }

  if (argv[0][0] == 'i')
    bw_profile_type = NSM_INGRESS_POLICING;
  else
    bw_profile_type = NSM_EGRESS_SHAPING;

  if (argv[1][0] == 'a')
    bw_profile_status = PAL_TRUE;

  ret = nsm_set_evc_bw_profiling (ifp, evc_bw_profile, bw_profile_type,
      bw_profile_status);
      
  if (ret == RESULT_OK)
    {
      return CLI_SUCCESS;
    }
  else
    {
      nsm_bridge_display_error_message (cli, ret); 
      return CLI_ERROR;
    }
  return CLI_SUCCESS;

}

CLI (nsm_bw_profile_evc_set_cir,
     nsm_bw_profile_evc_set_cir_cmd,
     "(ingress-policing | egress-shaping) cir <0-10000000>",
     "configure ingress policing parameters",
     "configure egress shaping parameters",
     "configure committed information rate (CIR) per EVC",
     "enter the cir ranging 0bps -10Mbps")
{
  s_int32_t ret;
  struct interface *ifp = NULL;
  struct nsm_uni_evc_bw_profile *evc_bw_profile = NULL;
  u_int32_t cir = PAL_FALSE;

  ifp = cli->index;
  
  evc_bw_profile = (struct nsm_uni_evc_bw_profile *)cli->index_sub;

#ifdef HAVE_LACPD
  NSM_INTERFACE_CHECK_AGG_MEM_PROPERTY(cli, ifp);
#endif /* HAVE_LACPD */
  NSM_BRIDGE_CHECK_L2_PROPERTY(cli, ifp);

  if (!CHECK_FLAG(ifp->status, NSM_INTERFACE_ACTIVE))
    {
      cli_out (cli, "%% Interface %s is pseudo interface\n", ifp->name);
      return CLI_ERROR;
    }

  CLI_GET_UINT32_RANGE ("Committed Information Rate", cir, argv[1], 0, 
      10000000);

  if (argv[0][0] == 'i')
    {
        ret = nsm_evc_bw_profiling (ifp, evc_bw_profile, cir, NSM_UNI_DEFAULT,
            NSM_UNI_DEFAULT, NSM_UNI_DEFAULT, NSM_UNI_DEFAULT, 
          NSM_UNI_DEFAULT, NSM_INGRESS_POLICING, NSM_BW_PARAMETER_CIR);
    }
  else
    {
      ret = nsm_evc_bw_profiling (ifp, evc_bw_profile, cir, NSM_UNI_DEFAULT,
          NSM_UNI_DEFAULT, NSM_UNI_DEFAULT, NSM_UNI_DEFAULT, 
          NSM_UNI_DEFAULT, NSM_EGRESS_SHAPING, NSM_BW_PARAMETER_CIR);

    }

  if (ret == RESULT_OK)
    {
      return CLI_SUCCESS;
    }
  else
    {
      nsm_bridge_display_error_message (cli, ret); 
      return CLI_ERROR;
    }
  return CLI_SUCCESS;

}

CLI (nsm_bw_profile_evc_set_cbs,
     nsm_bw_profile_evc_set_cbs_cmd,
     "(ingress-policing | egress-shaping) cbs <0-20000>",
     "configure ingress policing parameters",
     "configure egress shaping parameters",
     "configure committed burst size (CBS) per EVC",
     "enter the cbs ranging 0-20000 bytes")
{
  s_int32_t ret;
  struct interface *ifp;
  struct nsm_uni_evc_bw_profile *evc_bw_profile = NULL;
  u_int32_t cbs;

  ifp = cli->index;

  evc_bw_profile = cli->index_sub;

#ifdef HAVE_LACPD
  NSM_INTERFACE_CHECK_AGG_MEM_PROPERTY(cli, ifp);
#endif /* HAVE_LACPD */
  NSM_BRIDGE_CHECK_L2_PROPERTY(cli, ifp);

  if (!CHECK_FLAG(ifp->status, NSM_INTERFACE_ACTIVE))
    {
      cli_out (cli, "%% Interface %s is pseudo interface\n", ifp->name);
      return CLI_ERROR;
    }

  CLI_GET_UINT32_RANGE ("Committed Burst Size", cbs, argv[1], 0, 
      20000);

  if (argv[0][0] == 'i')
    {
      ret = nsm_evc_bw_profiling (ifp, evc_bw_profile, NSM_UNI_DEFAULT, cbs,
          NSM_UNI_DEFAULT, NSM_UNI_DEFAULT, NSM_UNI_DEFAULT,
          NSM_UNI_DEFAULT, NSM_INGRESS_POLICING, NSM_BW_PARAMETER_CBS);
    }
  else
    {
      ret = nsm_evc_bw_profiling (ifp, evc_bw_profile, NSM_UNI_DEFAULT, cbs,
          NSM_UNI_DEFAULT, NSM_UNI_DEFAULT, NSM_UNI_DEFAULT, 
          NSM_UNI_DEFAULT, NSM_EGRESS_SHAPING, NSM_BW_PARAMETER_CBS);
    }

  if (ret == RESULT_OK)
    {
      return CLI_SUCCESS;
    }
  else
    {
      nsm_bridge_display_error_message (cli, ret); 
      return CLI_ERROR;
    }
  return CLI_SUCCESS;

}

CLI (nsm_bw_profile_evc_set_eir,
     nsm_bw_profile_evc_set_eir_cmd,
     "(ingress-policing | egress-shaping) eir <0-20000000>",
     "configure ingress policing parameters",
     "configure egress shaping parameters",
     "configure excess information rate (EIR) per evc",
     "enter the eir ranging 0bps -20Mbps")
{
  s_int32_t ret;
  struct interface *ifp;
  struct nsm_uni_evc_bw_profile *evc_bw_profile = NULL;
  u_int32_t eir = PAL_FALSE;

  ifp = cli->index;

  evc_bw_profile = cli->index_sub;

#ifdef HAVE_LACPD
  NSM_INTERFACE_CHECK_AGG_MEM_PROPERTY(cli, ifp);
#endif /* HAVE_LACPD */
  NSM_BRIDGE_CHECK_L2_PROPERTY(cli, ifp);

  if (!CHECK_FLAG(ifp->status, NSM_INTERFACE_ACTIVE))
    {
      cli_out (cli, "%% Interface %s is pseudo interface\n", ifp->name);
      return CLI_ERROR;
    }

  CLI_GET_UINT32_RANGE ("Excess Information Rate", eir, argv[1], 0, 
      20000000);

  if (argv[0][0] == 'i')
    {
      ret = nsm_evc_bw_profiling (ifp, evc_bw_profile, NSM_UNI_DEFAULT,
          NSM_UNI_DEFAULT, eir, NSM_UNI_DEFAULT, NSM_UNI_DEFAULT, 
          NSM_UNI_DEFAULT, NSM_INGRESS_POLICING, NSM_BW_PARAMETER_EIR);
    }
  else
    {
      ret = nsm_evc_bw_profiling (ifp, evc_bw_profile, NSM_UNI_DEFAULT,
          NSM_UNI_DEFAULT, eir, NSM_UNI_DEFAULT, NSM_UNI_DEFAULT,
          NSM_UNI_DEFAULT, NSM_EGRESS_SHAPING, NSM_BW_PARAMETER_EIR);
    }

  if (ret == RESULT_OK)
    {
      return CLI_SUCCESS;
    }
  else
    {
      nsm_bridge_display_error_message (cli, ret); 
      return CLI_ERROR;
    }
  return CLI_SUCCESS;

}

CLI (nsm_bw_profile_evc_set_ebs,
     nsm_bw_profile_evc_set_ebs_cmd,
     "(ingress-policing | egress-shaping) ebs <0-20000>",
     "configure ingress policing parameters",
     "configure egress shaping parameters",
     "configure excess burst size (EBS) per EVC",
     "enter the ebs ranging 0-20000")
{
  s_int32_t ret;
  struct interface *ifp;
  struct nsm_uni_evc_bw_profile *evc_bw_profile = NULL;
  u_int32_t ebs = PAL_FALSE;

  ifp = cli->index;

  evc_bw_profile = cli->index_sub;

#ifdef HAVE_LACPD
  NSM_INTERFACE_CHECK_AGG_MEM_PROPERTY(cli, ifp);
#endif /* HAVE_LACPD */
  NSM_BRIDGE_CHECK_L2_PROPERTY(cli, ifp);

  if (!CHECK_FLAG(ifp->status, NSM_INTERFACE_ACTIVE))
    {
      cli_out (cli, "%% Interface %s is pseudo interface\n", ifp->name);
      return CLI_ERROR;
    }

  CLI_GET_UINT32_RANGE ("Excess Burst Size", ebs, argv[1], 0, 
      20000);

  if (argv[0][0] == 'i')
    {
      ret = nsm_evc_bw_profiling (ifp, evc_bw_profile, NSM_UNI_DEFAULT,
          NSM_UNI_DEFAULT, NSM_UNI_DEFAULT, ebs, NSM_UNI_DEFAULT,
          NSM_UNI_DEFAULT, NSM_INGRESS_POLICING, NSM_BW_PARAMETER_EBS);
    }
  else
    {
      ret = nsm_evc_bw_profiling (ifp, evc_bw_profile, NSM_UNI_DEFAULT,
          NSM_UNI_DEFAULT, NSM_UNI_DEFAULT, ebs, NSM_UNI_DEFAULT, 
          NSM_UNI_DEFAULT, NSM_EGRESS_SHAPING, NSM_BW_PARAMETER_EBS);
    }

  if (ret == RESULT_OK)
    {
      return CLI_SUCCESS;
    }
  else
    {
      nsm_bridge_display_error_message (cli, ret); 
      return CLI_ERROR;
    }
  return CLI_SUCCESS;

}

CLI (nsm_bw_profile_evc_set_coupling_flag,
     nsm_bw_profile_evc_set_coupling_flag_cmd,
     "(ingress-policing | egress-shaping) coupling-flag "
     "(enable | disable)",
     "configure ingress policing parameters",
     "configure egress shaping parameters",
     "configure coupling-flag per evc",
     "enable the coupling-flag per evc",
     "disable the coupling-flag per evc")
{
  s_int32_t ret;
  struct interface *ifp;
  struct nsm_uni_evc_bw_profile *evc_bw_profile = NULL;
  u_int8_t coupling_flag = PAL_FALSE;

  ifp = cli->index;

  evc_bw_profile = cli->index_sub;

#ifdef HAVE_LACPD
  NSM_INTERFACE_CHECK_AGG_MEM_PROPERTY(cli, ifp);
#endif /* HAVE_LACPD */
  NSM_BRIDGE_CHECK_L2_PROPERTY(cli, ifp);

  if (!CHECK_FLAG(ifp->status, NSM_INTERFACE_ACTIVE))
    {
      cli_out (cli, "%% Interface %s is pseudo interface\n", ifp->name);
      return CLI_ERROR;
    }

  if (argv[1][0] == 'e')
    {
      coupling_flag = PAL_TRUE;
    }

  if (argv[0][0] == 'i')
    {
      ret = nsm_evc_bw_profiling (ifp, evc_bw_profile, NSM_UNI_DEFAULT,
          NSM_UNI_DEFAULT, NSM_UNI_DEFAULT, NSM_UNI_DEFAULT, 
          coupling_flag, NSM_UNI_DEFAULT, NSM_INGRESS_POLICING, 
          NSM_BW_PARAMETER_COUPLING_FLAG);
    }
  else
    {
        ret = nsm_evc_bw_profiling (ifp, evc_bw_profile, NSM_UNI_DEFAULT,
            NSM_UNI_DEFAULT, NSM_UNI_DEFAULT, NSM_UNI_DEFAULT, 
            coupling_flag, NSM_UNI_DEFAULT, NSM_EGRESS_SHAPING, 
            NSM_BW_PARAMETER_COUPLING_FLAG);
    }

  if (ret == RESULT_OK)
    {
      return CLI_SUCCESS;
    }
  else
    {
      nsm_bridge_display_error_message (cli, ret); 
      return CLI_ERROR;
    }
  return CLI_SUCCESS;

}

CLI (nsm_bw_profile_evc_set_color_mode,
     nsm_bw_profile_evc_set_color_mode_cmd,
     "(ingress-policing | egress-shaping) color-mode "
     "(color-blind | color-aware)",
     "configure ingress policing parameters",
     "configure egress policing parameters",
     "configure color-mode per evc",
     "enter color-blind mode per evc",
     "enter color-aware mode per evc")
{
  s_int32_t ret;
  struct interface *ifp;
  struct nsm_uni_evc_bw_profile *evc_bw_profile = NULL;
  u_int8_t color_mode = PAL_FALSE;

  ifp = cli->index;

  evc_bw_profile = cli->index_sub;

#ifdef HAVE_LACPD
  NSM_INTERFACE_CHECK_AGG_MEM_PROPERTY(cli, ifp);
#endif /* HAVE_LACPD */
  NSM_BRIDGE_CHECK_L2_PROPERTY(cli, ifp);

  if (!CHECK_FLAG(ifp->status, NSM_INTERFACE_ACTIVE))
    {
      cli_out (cli, "%% Interface %s is pseudo interface\n", ifp->name);
      return CLI_ERROR;
    }

  if (argv[1][6] == 'a')
    {
      color_mode = PAL_TRUE;
    }
  if (argv[0][0] == 'i')
    {
      ret = nsm_evc_bw_profiling (ifp, evc_bw_profile, NSM_UNI_DEFAULT,
          NSM_UNI_DEFAULT, NSM_UNI_DEFAULT, NSM_UNI_DEFAULT, NSM_UNI_DEFAULT, 
          color_mode, NSM_INGRESS_POLICING, NSM_BW_PARAMETER_COLOR_MODE);
    }
  else
    {
      ret = nsm_evc_bw_profiling (ifp, evc_bw_profile, NSM_UNI_DEFAULT, NSM_UNI_DEFAULT,
          NSM_UNI_DEFAULT, NSM_UNI_DEFAULT, NSM_UNI_DEFAULT, 
          color_mode, NSM_EGRESS_SHAPING, NSM_BW_PARAMETER_COLOR_MODE);

    }

  if (ret == RESULT_OK)
    {
      return CLI_SUCCESS;
    }
  else
    {
      nsm_bridge_display_error_message (cli, ret); 
      return CLI_ERROR;
    }
  return CLI_SUCCESS;

}

CLI (nsm_delete_evc_cos_bw_profile,
     nsm_delete_evc_cos_bw_profile_cmd,
     "no (ingress-policing | egress-shaping) evc cos {(<0-7>) | (<0-7> <0-7>) | "
     "(<0-7> <0-7> <0-7>) | (<0-7> <0-7> <0-7> <0-7>) | "
     "(<0-7> <0-7> <0-7> <0-7> <0-7> ) | "
     "(<0-7> <0-7> <0-7> <0-7> <0-7> <0-7>) | "
     "(<0-7> <0-7> <0-7> <0-7> <0-7> <0-7> <0-7>) | "
     "(<0-7> <0-7> <0-7> <0-7> <0-7> <0-7> <0-7> <0-7>)}",
     CLI_NO_STR,
     "configure ingress policing parameters",
     "configure egress shaping parameters",
     "configure evc bandwidth profile parameters",
     "configure bandwidth profile parameters per cos",
     "enter the cos values to be configured for bandwidth profiling",
     "enter the cos value",
     "enter the cos value",
     "enter the cos value",
     "enter the cos value",
     "enter the cos value",
     "enter the cos value",
     "enter the cos value",
     "enter the cos value",
     "enter the cos value",
     "enter the cos value",
     "enter the cos value",
     "enter the cos value",
     "enter the cos value",
     "enter the cos value",
     "enter the cos value",
     "enter the cos value",
     "enter the cos value",
     "enter the cos value",
     "enter the cos value",
     "enter the cos value",
     "enter the cos value",
     "enter the cos value",
     "enter the cos value",
     "enter the cos value",
     "enter the cos value",
     "enter the cos value",
     "enter the cos value",
     "enter the cos value",
     "enter the cos value",
     "enter the cos value",
     "enter the cos value",
     "enter the cos value",
     "enter the cos value",
     "enter the cos value",
     "enter the cos value",
     "enter the cos value")
{
  s_int32_t ret;
  struct interface *ifp = NULL;
  struct nsm_uni_evc_bw_profile *evc_bw_profile = NULL;
  u_int8_t cos [NSM_MAX_COS_PER_EVC];
  u_int8_t cos_count = PAL_FALSE;
  u_int32_t temp_cos = PAL_FALSE;
  u_int8_t bw_profile_type = PAL_FALSE;

  ifp = cli->index;
  
  evc_bw_profile = cli->index_sub;

#ifdef HAVE_LACPD
  NSM_INTERFACE_CHECK_AGG_MEM_PROPERTY(cli, ifp);
#endif /* HAVE_LACPD */
  NSM_BRIDGE_CHECK_L2_PROPERTY(cli, ifp);

  if (!CHECK_FLAG(ifp->status, NSM_INTERFACE_ACTIVE))
    {
      cli_out (cli, "%% Interface %s is pseudo interface\n", ifp->name);
      return CLI_ERROR;
    }
  
  for (cos_count = 0; cos_count < NSM_MAX_COS_PER_EVC; cos_count++)
    {
      temp_cos = PAL_FALSE;
    }

  for (cos_count = 0; cos_count < argc - 1; cos_count++)
    {
      CLI_GET_UINT32_RANGE ("CoS value", temp_cos, argv[cos_count + 1], 0, 7);
      
      cos[temp_cos] = PAL_TRUE;

      temp_cos = PAL_FALSE;
    }

  if (argv[0][0] == 'i')
    {
      bw_profile_type = NSM_INGRESS_POLICING;
    }
  else
    {
      bw_profile_type = NSM_EGRESS_SHAPING;
    }

  ret = nsm_delete_evc_cos_bw_profiling (ifp, evc_bw_profile, cos,
      bw_profile_type);

  if (ret == RESULT_OK)
    {
      return CLI_SUCCESS;
    }
  else
    {
      nsm_bridge_display_error_message (cli, ret); 
      return CLI_ERROR;
    }
  return CLI_SUCCESS;

}

CLI (nsm_activate_evc_cos_bw_profile,
     nsm_activate_evc_cos_bw_profile_cmd,
     "(ingress-policing | egress-shaping) evc cos {(<0-7>) | (<0-7> <0-7>) | "
     "(<0-7> <0-7> <0-7>) | (<0-7> <0-7> <0-7> <0-7>) | "
     "(<0-7> <0-7> <0-7> <0-7> <0-7> ) | "
     "(<0-7> <0-7> <0-7> <0-7> <0-7> <0-7>) | "
     "(<0-7> <0-7> <0-7> <0-7> <0-7> <0-7> <0-7>) | "
     "(<0-7> <0-7> <0-7> <0-7> <0-7> <0-7> <0-7> <0-7>)} "
     "(active | inactive)",
     "configure ingress policing parameters",
     "configure egress shaping parameters",
     "configure evc bandwidth profile parameters",
     "configure bandwidth profile parameters per cos",
     "enter the cos values to be configured for bandwidth profiling",
     "enter the cos value",
     "enter the cos value",
     "enter the cos value",
     "enter the cos value",
     "enter the cos value",
     "enter the cos value",
     "enter the cos value",
     "enter the cos value",
     "enter the cos value",
     "enter the cos value",
     "enter the cos value",
     "enter the cos value",
     "enter the cos value",
     "enter the cos value",
     "enter the cos value",
     "enter the cos value",
     "enter the cos value",
     "enter the cos value",
     "enter the cos value",
     "enter the cos value",
     "enter the cos value",
     "enter the cos value",
     "enter the cos value",
     "enter the cos value",
     "enter the cos value",
     "enter the cos value",
     "enter the cos value",
     "enter the cos value",
     "enter the cos value",
     "enter the cos value",
     "enter the cos value",
     "enter the cos value",
     "enter the cos value",
     "enter the cos value",
     "enter the cos value",
     "enter the cos value",
     "activate bandwidth profile per COS",
     "inactivate bandwidth profile per COS")
{
  s_int32_t ret;
  struct interface *ifp = NULL;
  struct nsm_uni_evc_bw_profile *evc_bw_profile = NULL;
  u_int8_t cos [NSM_MAX_COS_PER_EVC];
  u_int8_t cos_count = PAL_FALSE;
  u_int32_t temp_cos = PAL_FALSE;
  u_int8_t bw_profile_type = PAL_FALSE;
  u_int8_t bw_profile_status = PAL_FALSE;

  ifp = cli->index;
  
  evc_bw_profile = cli->index_sub;

#ifdef HAVE_LACPD
  NSM_INTERFACE_CHECK_AGG_MEM_PROPERTY(cli, ifp);
#endif /* HAVE_LACPD */
  NSM_BRIDGE_CHECK_L2_PROPERTY(cli, ifp);

  if (!CHECK_FLAG(ifp->status, NSM_INTERFACE_ACTIVE))
    {
      cli_out (cli, "%% Interface %s is pseudo interface\n", ifp->name);
      return CLI_ERROR;
    }
  
  for (cos_count = 0; cos_count < NSM_MAX_COS_PER_EVC; cos_count++)
    {
      temp_cos = PAL_FALSE;
    }

  for (cos_count = 0; cos_count < argc - 2; cos_count++)
    {
      CLI_GET_UINT32_RANGE ("CoS value", temp_cos, argv[cos_count + 1], 0, 7);
      
      cos[temp_cos] = PAL_TRUE;

      temp_cos = PAL_FALSE;
    }

  if (argv[0][0] == 'i')
    {
      bw_profile_type = NSM_INGRESS_POLICING;
    }
  else
    {
      bw_profile_type = NSM_EGRESS_SHAPING;
    }

  if (argv[argc - 1][0] == 'a')
    {
      bw_profile_status = PAL_TRUE;
    }

  ret = nsm_set_evc_cos_bw_profiling (ifp, evc_bw_profile, cos,
      bw_profile_type, bw_profile_status);

  if (ret == RESULT_OK)
    {
      return CLI_SUCCESS;
    }
  else
    {
      nsm_bridge_display_error_message (cli, ret); 
      return CLI_ERROR;
    }
  return CLI_SUCCESS;

}

CLI (nsm_bw_profile_evc_cos_set_cir,
     nsm_bw_profile_evc_cos_set_cir_cmd,
     "(ingress-policing | egress-shaping) evc cos {(<0-7>) | (<0-7> <0-7>) | "
     "(<0-7> <0-7> <0-7>) | (<0-7> <0-7> <0-7> <0-7>) | "
     "(<0-7> <0-7> <0-7> <0-7> <0-7> ) | "
     "(<0-7> <0-7> <0-7> <0-7> <0-7> <0-7>) | "
     "(<0-7> <0-7> <0-7> <0-7> <0-7> <0-7> <0-7>) | "
     "(<0-7> <0-7> <0-7> <0-7> <0-7> <0-7> <0-7> <0-7>)} cir <0-10000000>",
     "configure ingress policing parameters",
     "configure egress shaping parameters",
     "configure evc bandwidth profile parameters",
     "configure bandwidth profile parameters per cos",
     "enter the cos values to be configured for bandwidth profiling",
     "enter the cos value",
     "enter the cos value",
     "enter the cos value",
     "enter the cos value",
     "enter the cos value",
     "enter the cos value",
     "enter the cos value",
     "enter the cos value",
     "enter the cos value",
     "enter the cos value",
     "enter the cos value",
     "enter the cos value",
     "enter the cos value",
     "enter the cos value",
     "enter the cos value",
     "enter the cos value",
     "enter the cos value",
     "enter the cos value",
     "enter the cos value",
     "enter the cos value",
     "enter the cos value",
     "enter the cos value",
     "enter the cos value",
     "enter the cos value",
     "enter the cos value",
     "enter the cos value",
     "enter the cos value",
     "enter the cos value",
     "enter the cos value",
     "enter the cos value",
     "enter the cos value",
     "enter the cos value",
     "enter the cos value",
     "enter the cos value",
     "enter the cos value",
     "enter the cos value",
     "configure committed information rate (CIR) per CoS",
     "enter the cir ranging 0bps -10Mbps")
{
  s_int32_t ret;
  struct interface *ifp = NULL;
  struct nsm_uni_evc_bw_profile *evc_bw_profile = NULL;
  u_int32_t cir = PAL_FALSE;
  u_int8_t cos [NSM_MAX_COS_PER_EVC];
  u_int8_t cos_count = PAL_FALSE;
  u_int32_t temp_cos = PAL_FALSE;

  ifp = cli->index;
  
  evc_bw_profile = cli->index_sub;

#ifdef HAVE_LACPD
  NSM_INTERFACE_CHECK_AGG_MEM_PROPERTY(cli, ifp);
#endif /* HAVE_LACPD */
  NSM_BRIDGE_CHECK_L2_PROPERTY(cli, ifp);

  if (!CHECK_FLAG(ifp->status, NSM_INTERFACE_ACTIVE))
    {
      cli_out (cli, "%% Interface %s is pseudo interface\n", ifp->name);
      return CLI_ERROR;
    }
  
  for (cos_count = 0; cos_count < NSM_MAX_COS_PER_EVC; cos_count++)
    {
      temp_cos = PAL_FALSE;
    }

  for (cos_count = 0; cos_count < argc-2; cos_count++)
    {
      CLI_GET_UINT32_RANGE ("CoS value", temp_cos, argv[cos_count+1], 0, 7);
      
      cos[temp_cos] = PAL_TRUE;

      temp_cos = PAL_FALSE;
    }

  CLI_GET_UINT32_RANGE ("Committed Information Rate", cir, argv[argc - 1], 0, 
      10000000);
  if (argv[0][0] == 'i')
    {
      ret = nsm_evc_cos_bw_profiling (ifp, evc_bw_profile, cos, cir,
          NSM_UNI_DEFAULT, NSM_UNI_DEFAULT, NSM_UNI_DEFAULT,
          NSM_UNI_DEFAULT, NSM_UNI_DEFAULT, NSM_INGRESS_POLICING,
          NSM_BW_PARAMETER_CIR);
    }
  else
    {
      ret = nsm_evc_cos_bw_profiling (ifp, evc_bw_profile, cos, cir,
          NSM_UNI_DEFAULT, NSM_UNI_DEFAULT, NSM_UNI_DEFAULT, NSM_UNI_DEFAULT, 
          NSM_UNI_DEFAULT, NSM_EGRESS_SHAPING, NSM_BW_PARAMETER_CIR);
    }

  if (ret == RESULT_OK)
    {
      return CLI_SUCCESS;
    }
  else
    {
      nsm_bridge_display_error_message (cli, ret); 
      return CLI_ERROR;
    }
  return CLI_SUCCESS;
}

CLI (nsm_bw_profile_evc_cos_set_cbs,
     nsm_bw_profile_evc_cos_set_cbs_cmd,
     "(ingress-policing | egress-shaping) evc cos {(<0-7>) | (<0-7> <0-7>) | "
     "(<0-7> <0-7> <0-7>) | (<0-7> <0-7> <0-7> <0-7>) | "
     "(<0-7> <0-7> <0-7> <0-7> <0-7> ) | "
     "(<0-7> <0-7> <0-7> <0-7> <0-7> <0-7>) | "
     "(<0-7> <0-7> <0-7> <0-7> <0-7> <0-7> <0-7>) | "
     "(<0-7> <0-7> <0-7> <0-7> <0-7> <0-7> <0-7> <0-7>)} cbs <0-20000>",
     "configure ingress policing parameters",
     "configure egress shaping parameters",
     "configure evc bandwidth profile parameters",
     "configure bandwidth profile parameters per cos",
     "enter the cos values to be configured for bandwidth profiling",
     "enter the cos value",
     "enter the cos value",
     "enter the cos value",
     "enter the cos value",
     "enter the cos value",
     "enter the cos value",
     "enter the cos value",
     "enter the cos value",
     "enter the cos value",
     "enter the cos value",
     "enter the cos value",
     "enter the cos value",
     "enter the cos value",
     "enter the cos value",
     "enter the cos value",
     "enter the cos value",
     "enter the cos value",
     "enter the cos value",
     "enter the cos value",
     "enter the cos value",
     "enter the cos value",
     "enter the cos value",
     "enter the cos value",
     "enter the cos value",
     "enter the cos value",
     "enter the cos value",
     "enter the cos value",
     "enter the cos value",
     "enter the cos value",
     "enter the cos value",
     "enter the cos value",
     "enter the cos value",
     "enter the cos value",
     "enter the cos value",
     "enter the cos value",
     "enter the cos value",
     "configure committed burst size (CBS) per CoS",
     "enter the cbs ranging 0-20000 bytes")
{
  s_int32_t ret;
  struct interface *ifp;
  struct nsm_uni_evc_bw_profile *evc_bw_profile = NULL;
  u_int32_t cbs = PAL_FALSE;
  u_int8_t cos [NSM_MAX_COS_PER_EVC];
  u_int8_t cos_count = PAL_FALSE;
  u_int32_t temp_cos = PAL_FALSE;

  ifp = cli->index;

  evc_bw_profile = cli->index_sub;

#ifdef HAVE_LACPD
  NSM_INTERFACE_CHECK_AGG_MEM_PROPERTY(cli, ifp);
#endif /* HAVE_LACPD */
  NSM_BRIDGE_CHECK_L2_PROPERTY(cli, ifp);

  if (!CHECK_FLAG(ifp->status, NSM_INTERFACE_ACTIVE))
    {
      cli_out (cli, "%% Interface %s is pseudo interface\n", ifp->name);
      return CLI_ERROR;
    }
  for (cos_count = 0; cos_count < argc-2; cos_count++)
    {
      CLI_GET_UINT32_RANGE ("CoS value", temp_cos, argv[cos_count+1], 0, 7);
      
      cos[temp_cos] = PAL_TRUE;

      temp_cos = PAL_FALSE;
    }

  CLI_GET_UINT32_RANGE ("Committed Burst Size", cbs, argv[argc - 1], 0, 
      20000);


  if (argv[0][0] == 'i')
    {
      ret = nsm_evc_cos_bw_profiling (ifp, evc_bw_profile, cos, NSM_UNI_DEFAULT
          , cbs, NSM_UNI_DEFAULT, NSM_UNI_DEFAULT, NSM_UNI_DEFAULT, 
          NSM_UNI_DEFAULT, NSM_INGRESS_POLICING, NSM_BW_PARAMETER_CBS);
    }
  else
    {
      ret = nsm_evc_cos_bw_profiling (ifp, evc_bw_profile, cos, NSM_UNI_DEFAULT,
          cbs, NSM_UNI_DEFAULT, NSM_UNI_DEFAULT, NSM_UNI_DEFAULT, 
          NSM_UNI_DEFAULT, NSM_EGRESS_SHAPING, NSM_BW_PARAMETER_CBS);
    }

  if (ret == RESULT_OK)
    {
      return CLI_SUCCESS;
    }
  else
    {
      nsm_bridge_display_error_message (cli, ret); 
      return CLI_ERROR;
    }
  return CLI_SUCCESS;

}

CLI (nsm_bw_profile_evc_cos_set_eir,
     nsm_bw_profile_evc_cos_set_eir_cmd,
     "(ingress-policing | egress-shaping) evc cos {(<0-7>) | (<0-7> <0-7>) | "
     "(<0-7> <0-7> <0-7>) | (<0-7> <0-7> <0-7> <0-7>) | "
     "(<0-7> <0-7> <0-7> <0-7> <0-7> ) | "
     "(<0-7> <0-7> <0-7> <0-7> <0-7> <0-7>) | "
     "(<0-7> <0-7> <0-7> <0-7> <0-7> <0-7> <0-7>) | "
     "(<0-7> <0-7> <0-7> <0-7> <0-7> <0-7> <0-7> <0-7>)} eir <0-20000000>",
     "configure ingress policing parameters",
     "configure egress shaping parameters",
     "configure evc bandwidth profile parameters",
     "configure bandwidth profile parameters per cos",
     "enter the cos values to be configured for bandwidth profiling",
     "enter the cos value",
     "enter the cos value",
     "enter the cos value",
     "enter the cos value",
     "enter the cos value",
     "enter the cos value",
     "enter the cos value",
     "enter the cos value",
     "enter the cos value",
     "enter the cos value",
     "enter the cos value",
     "enter the cos value",
     "enter the cos value",
     "enter the cos value",
     "enter the cos value",
     "enter the cos value",
     "enter the cos value",
     "enter the cos value",
     "enter the cos value",
     "enter the cos value",
     "enter the cos value",
     "enter the cos value",
     "enter the cos value",
     "enter the cos value",
     "enter the cos value",
     "enter the cos value",
     "enter the cos value",
     "enter the cos value",
     "enter the cos value",
     "enter the cos value",
     "enter the cos value",
     "enter the cos value",
     "enter the cos value",
     "enter the cos value",
     "enter the cos value",
     "enter the cos value",
     "configure excess information rate (EIR) per cos",
     "enter the eir ranging 0bps -20Mbps")
{
  s_int32_t ret;
  struct interface *ifp;
  struct nsm_uni_evc_bw_profile *evc_bw_profile = NULL;
  u_int32_t eir = PAL_FALSE;
  u_int8_t cos [NSM_MAX_COS_PER_EVC];
  u_int8_t cos_count = PAL_FALSE;
  u_int32_t temp_cos = PAL_FALSE;

  ifp = cli->index;

  evc_bw_profile = cli->index_sub;

#ifdef HAVE_LACPD
  NSM_INTERFACE_CHECK_AGG_MEM_PROPERTY(cli, ifp);
#endif /* HAVE_LACPD */
  NSM_BRIDGE_CHECK_L2_PROPERTY(cli, ifp);

  if (!CHECK_FLAG(ifp->status, NSM_INTERFACE_ACTIVE))
    {
      cli_out (cli, "%% Interface %s is pseudo interface\n", ifp->name);
      return CLI_ERROR;
    }

  for (cos_count = 0; cos_count < argc-2; cos_count++)
    {
      CLI_GET_UINT32_RANGE ("CoS value", temp_cos, argv[cos_count+1], 0, 7);
      
      cos[temp_cos] = PAL_TRUE;

      temp_cos = PAL_FALSE;
    }

  CLI_GET_UINT32_RANGE ("Excess Information Rate", eir, argv[argc - 1], 0, 
      20000000);


  if (argv[0][0] == 'i')
    {
      ret = nsm_evc_cos_bw_profiling (ifp, evc_bw_profile, cos, NSM_UNI_DEFAULT
          , NSM_UNI_DEFAULT, eir, NSM_UNI_DEFAULT, NSM_UNI_DEFAULT,
          NSM_UNI_DEFAULT, NSM_INGRESS_POLICING, NSM_BW_PARAMETER_EIR);
    }
  else
    {
      ret = nsm_evc_cos_bw_profiling (ifp, evc_bw_profile, cos, NSM_UNI_DEFAULT,
          NSM_UNI_DEFAULT, eir, NSM_UNI_DEFAULT, NSM_UNI_DEFAULT,
          NSM_UNI_DEFAULT, NSM_EGRESS_SHAPING, NSM_BW_PARAMETER_EIR);
    }

  if (ret == RESULT_OK)
    {
      return CLI_SUCCESS;
    }
  else
    {
      nsm_bridge_display_error_message (cli, ret); 
      return CLI_ERROR;
    }
  return CLI_SUCCESS;

}

CLI (nsm_bw_profile_evc_cos_set_ebs,
     nsm_bw_profile_evc_cos_set_ebs_cmd,
     "(ingress-policing | egress-shaping) evc cos {(<0-7>) | (<0-7> <0-7>) | "
     "(<0-7> <0-7> <0-7>) | (<0-7> <0-7> <0-7> <0-7>) | "
     "(<0-7> <0-7> <0-7> <0-7> <0-7> ) | "
     "(<0-7> <0-7> <0-7> <0-7> <0-7> <0-7>) | "
     "(<0-7> <0-7> <0-7> <0-7> <0-7> <0-7> <0-7>) | "
     "(<0-7> <0-7> <0-7> <0-7> <0-7> <0-7> <0-7> <0-7>)} ebs <0-20000>",
     "configure ingress policing parameters",
     "configure egress shaping parameters",
     "configure evc bandwidth profile parameters",
     "configure bandwidth profile parameters per cos",
     "enter the cos values to be configured for bandwidth profiling",
     "enter the cos value",
     "enter the cos value",
     "enter the cos value",
     "enter the cos value",
     "enter the cos value",
     "enter the cos value",
     "enter the cos value",
     "enter the cos value",
     "enter the cos value",
     "enter the cos value",
     "enter the cos value",
     "enter the cos value",
     "enter the cos value",
     "enter the cos value",
     "enter the cos value",
     "enter the cos value",
     "enter the cos value",
     "enter the cos value",
     "enter the cos value",
     "enter the cos value",
     "enter the cos value",
     "enter the cos value",
     "enter the cos value",
     "enter the cos value",
     "enter the cos value",
     "enter the cos value",
     "enter the cos value",
     "enter the cos value",
     "enter the cos value",
     "enter the cos value",
     "enter the cos value",
     "enter the cos value",
     "enter the cos value",
     "enter the cos value",
     "enter the cos value",
     "enter the cos value",
     "configure excess burst size (EBS) per CoS",
     "enter the ebs ranging 0-20000")
{
  s_int32_t ret;
  struct interface *ifp;
  struct nsm_uni_evc_bw_profile *evc_bw_profile = NULL;
  u_int32_t ebs = PAL_FALSE;
  u_int8_t cos [NSM_MAX_COS_PER_EVC];
  u_int8_t cos_count = PAL_FALSE;
  u_int32_t temp_cos = PAL_FALSE;

  ifp = cli->index;

  evc_bw_profile = cli->index_sub;

#ifdef HAVE_LACPD
  NSM_INTERFACE_CHECK_AGG_MEM_PROPERTY(cli, ifp);
#endif /* HAVE_LACPD */
  NSM_BRIDGE_CHECK_L2_PROPERTY(cli, ifp);

  if (!CHECK_FLAG(ifp->status, NSM_INTERFACE_ACTIVE))
    {
      cli_out (cli, "%% Interface %s is pseudo interface\n", ifp->name);
      return CLI_ERROR;
    }

  for (cos_count = 0; cos_count < argc-2; cos_count++)
    {
      CLI_GET_UINT32_RANGE ("CoS value", temp_cos, argv[cos_count+1], 0, 7);
      
      cos[temp_cos] = PAL_TRUE;

      temp_cos = PAL_FALSE;
    }
  CLI_GET_UINT32_RANGE ("Excess Information Rate", ebs, argv[argc - 1], 0, 
      20000);

  if (argv[0][0] == 'i')
    {
      ret = nsm_evc_cos_bw_profiling (ifp, evc_bw_profile, cos, NSM_UNI_DEFAULT,
          NSM_UNI_DEFAULT, NSM_UNI_DEFAULT, ebs, NSM_UNI_DEFAULT,
          NSM_UNI_DEFAULT, NSM_INGRESS_POLICING, NSM_BW_PARAMETER_EBS);
    }
  else
    {
      ret = nsm_evc_cos_bw_profiling (ifp, evc_bw_profile, cos, NSM_UNI_DEFAULT,
          NSM_UNI_DEFAULT, NSM_UNI_DEFAULT, ebs, NSM_UNI_DEFAULT,
          NSM_UNI_DEFAULT, NSM_EGRESS_SHAPING, NSM_BW_PARAMETER_EBS);
    }

  if (ret == RESULT_OK)
    {
      return CLI_SUCCESS;
    }
  else
    {
      nsm_bridge_display_error_message (cli, ret); 
      return CLI_ERROR;
    }
  return CLI_SUCCESS;

}

CLI (nsm_bw_profile_evc_cos_set_coupling_flag,
     nsm_bw_profile_evc_cos_set_coupling_flag_cmd,
     "(ingress-policing | egress-shaping) evc cos {(<0-7>) | (<0-7> <0-7>) | "
     "(<0-7> <0-7> <0-7>) | (<0-7> <0-7> <0-7> <0-7>) | "
     "(<0-7> <0-7> <0-7> <0-7> <0-7> ) | "
     "(<0-7> <0-7> <0-7> <0-7> <0-7> <0-7>) | "
     "(<0-7> <0-7> <0-7> <0-7> <0-7> <0-7> <0-7>) | "
     "(<0-7> <0-7> <0-7> <0-7> <0-7> <0-7> <0-7> <0-7>)} coupling-flag "
     "(enable | disable)",
     "configure ingress policing parameters",
     "configure egress shaping parameters",
     "configure evc bandwidth profile parameters",
     "configure bandwidth profile parameters per cos",
     "enter the cos values to be configured for bandwidth profiling",
     "enter the cos value",
     "enter the cos value",
     "enter the cos value",
     "enter the cos value",
     "enter the cos value",
     "enter the cos value",
     "enter the cos value",
     "enter the cos value",
     "enter the cos value",
     "enter the cos value",
     "enter the cos value",
     "enter the cos value",
     "enter the cos value",
     "enter the cos value",
     "enter the cos value",
     "enter the cos value",
     "enter the cos value",
     "enter the cos value",
     "enter the cos value",
     "enter the cos value",
     "enter the cos value",
     "enter the cos value",
     "enter the cos value",
     "enter the cos value",
     "enter the cos value",
     "enter the cos value",
     "enter the cos value",
     "enter the cos value",
     "enter the cos value",
     "enter the cos value",
     "enter the cos value",
     "enter the cos value",
     "enter the cos value",
     "enter the cos value",
     "enter the cos value",
     "enter the cos value",
     "configure coupling-flag per evc",
     "enable the coupling-flag per evc",
     "disable the coupling-flag per evc")
{
  s_int32_t ret;
  struct interface *ifp;
  struct nsm_uni_evc_bw_profile *evc_bw_profile = NULL;
  u_int8_t coupling_flag = PAL_FALSE;
  u_int8_t cos [NSM_MAX_COS_PER_EVC];
  u_int8_t cos_count = PAL_FALSE;
  u_int32_t temp_cos = PAL_FALSE;

  ifp = cli->index;

  evc_bw_profile = cli->index_sub;

#ifdef HAVE_LACPD
  NSM_INTERFACE_CHECK_AGG_MEM_PROPERTY(cli, ifp);
#endif /* HAVE_LACPD */
  NSM_BRIDGE_CHECK_L2_PROPERTY(cli, ifp);

  if (!CHECK_FLAG(ifp->status, NSM_INTERFACE_ACTIVE))
    {
      cli_out (cli, "%% Interface %s is pseudo interface\n", ifp->name);
      return CLI_ERROR;
    }

  if (argv[argc - 1][0] == 'e')
    {
      coupling_flag = PAL_TRUE;
    }

  for (cos_count = 0; cos_count < (argc - 2); cos_count++)
    {
      CLI_GET_UINT32_RANGE ("CoS value", temp_cos, argv[cos_count+1], 0, 7);
      
      cos[temp_cos] = PAL_TRUE;

      temp_cos = PAL_FALSE;
    }

  if (argv[0][0] == 'i')
    {
      ret = nsm_evc_cos_bw_profiling (ifp, evc_bw_profile, cos, NSM_UNI_DEFAULT,
          NSM_UNI_DEFAULT, NSM_UNI_DEFAULT, NSM_UNI_DEFAULT,
          coupling_flag, NSM_UNI_DEFAULT, NSM_INGRESS_POLICING, 
          NSM_BW_PARAMETER_COUPLING_FLAG);
    }
  else
    {
      ret = nsm_evc_cos_bw_profiling (ifp, evc_bw_profile, cos, NSM_UNI_DEFAULT,
          NSM_UNI_DEFAULT, NSM_UNI_DEFAULT, NSM_UNI_DEFAULT,
          coupling_flag, NSM_UNI_DEFAULT, NSM_EGRESS_SHAPING,
          NSM_BW_PARAMETER_COUPLING_FLAG);
    }

  if (ret == RESULT_OK)
    {
      return CLI_SUCCESS;
    }
  else
    {
      nsm_bridge_display_error_message (cli, ret); 
      return CLI_ERROR;
    }
  return CLI_SUCCESS;

}

CLI (nsm_bw_profile_evc_cos_set_color_mode,
     nsm_bw_profile_evc_cos_set_color_mode_cmd,
     "(ingress-policing | egress-shaping) evc cos {(<0-7>) | (<0-7> <0-7>) | "
     "(<0-7> <0-7> <0-7>) | (<0-7> <0-7> <0-7> <0-7>) | "
     "(<0-7> <0-7> <0-7> <0-7> <0-7> ) | "
     "(<0-7> <0-7> <0-7> <0-7> <0-7> <0-7>) | "
     "(<0-7> <0-7> <0-7> <0-7> <0-7> <0-7> <0-7>) | "
     "(<0-7> <0-7> <0-7> <0-7> <0-7> <0-7> <0-7> <0-7>)} color-mode "
     "(color-blind | color-aware) ",
     "configure ingress policing parameters",
     "configure egress shaping parameters",
     "configure evc bandwidth profile parameters",
     "configure bandwidth profile parameters per cos",
     "enter the cos values to be configured for bandwidth profiling",
     "enter the cos value",
     "enter the cos value",
     "enter the cos value",
     "enter the cos value",
     "enter the cos value",
     "enter the cos value",
     "enter the cos value",
     "enter the cos value",
     "enter the cos value",
     "enter the cos value",
     "enter the cos value",
     "enter the cos value",
     "enter the cos value",
     "enter the cos value",
     "enter the cos value",
     "enter the cos value",
     "enter the cos value",
     "enter the cos value",
     "enter the cos value",
     "enter the cos value",
     "enter the cos value",
     "enter the cos value",
     "enter the cos value",
     "enter the cos value",
     "enter the cos value",
     "enter the cos value",
     "enter the cos value",
     "enter the cos value",
     "enter the cos value",
     "enter the cos value",
     "enter the cos value",
     "enter the cos value",
     "enter the cos value",
     "enter the cos value",
     "enter the cos value",
     "enter the cos value",
     "configure ingress policing parameters",
     "configure egress policing parameters",
     "configure color-mode per cos",
     "enter color-blind mode per cos",
     "enter color-aware mode per cos")
{
  s_int32_t ret;
  struct interface *ifp;
  struct nsm_uni_evc_bw_profile *evc_bw_profile = NULL;
  u_int8_t color_mode = PAL_FALSE;
  u_int8_t cos [NSM_MAX_COS_PER_EVC];
  u_int8_t cos_count = PAL_FALSE;
  u_int32_t temp_cos = PAL_FALSE;

  ifp = cli->index;

  evc_bw_profile = cli->index_sub;

#ifdef HAVE_LACPD
  NSM_INTERFACE_CHECK_AGG_MEM_PROPERTY(cli, ifp);
#endif /* HAVE_LACPD */
  NSM_BRIDGE_CHECK_L2_PROPERTY(cli, ifp);

  if (!CHECK_FLAG(ifp->status, NSM_INTERFACE_ACTIVE))
    {
      cli_out (cli, "%% Interface %s is pseudo interface\n", ifp->name);
      return CLI_ERROR;
    }

  if (argv[argc - 1][6] == 'a')
    {
      color_mode = PAL_TRUE;
    }

  for (cos_count = 0; cos_count < (argc - 2); cos_count++)
    {
      CLI_GET_UINT32_RANGE ("CoS value", temp_cos, argv[cos_count+1], 0, 7);
      
      cos[temp_cos] = PAL_TRUE;

      temp_cos = PAL_FALSE;
    }

  if (argv[0][0] == 'i')
    {
      ret = nsm_evc_cos_bw_profiling (ifp, evc_bw_profile, cos,
          NSM_UNI_DEFAULT, NSM_UNI_DEFAULT, NSM_UNI_DEFAULT,
          NSM_UNI_DEFAULT, NSM_UNI_DEFAULT, color_mode,
          NSM_INGRESS_POLICING, NSM_BW_PARAMETER_COLOR_MODE);
    }
  else
    {
      ret = nsm_evc_cos_bw_profiling (ifp, evc_bw_profile, cos, NSM_UNI_DEFAULT
          , NSM_UNI_DEFAULT, NSM_UNI_DEFAULT, NSM_UNI_DEFAULT,
          NSM_UNI_DEFAULT, color_mode, NSM_EGRESS_SHAPING,
          NSM_BW_PARAMETER_COLOR_MODE);
    }

  if (ret == RESULT_OK)
    {
      return CLI_SUCCESS;
    }
  else
    {
      nsm_bridge_display_error_message (cli, ret); 
      return CLI_ERROR;
    }
  return CLI_SUCCESS;

}

/**@brief  - Function to display the bw profile parameters for uni or bw 
 *           profile parameters for all the interfaces associated to the bridge
 * @param cli         - Pointer to imish terminal.
 * @param bridge      - Bridge associated interfaces on which the bw profile
 *                        parameters need to be displayed.
 * @param zif         - nsm interface for which the bw profile parameters need
 *                        to be displayed.
 * @return            - return control to the CLI. 
 * */
static void 
nsm_show_bw_profiling (struct cli *cli, struct nsm_bridge *bridge,
    struct nsm_if *zif)
{
  struct nsm_band_width_profile *bw_profile = NULL;
  struct nsm_uni_evc_bw_profile *evc_bw_profile = NULL;
  struct listnode *evc_node = NULL;
  u_int8_t first = PAL_FALSE;
  u_int8_t i = PAL_FALSE;
  u_int8_t j = PAL_FALSE;

  if (! (zif->nsm_bw_profile))
    {
      cli_out (cli, " Bandwidth profile not configured\n");
      cli_out (cli, " \n");
      return;
    }

  bw_profile = zif->nsm_bw_profile;

  cli_out (cli, "%% Interface Name : %s\n", zif->ifp->name);
  cli_out (cli, " =========================\n");
  
  if (bw_profile->ingress_bw_profile_status == BW_PROFILE_CONFIGURED)
    {
      if (bw_profile->ingress_bw_profile_type == NSM_BW_PROFILE_PER_UNI)
        {
          cli_out (cli, "%% Ingress Bandwidth Profile : Per UNI\n");
          cli_out (cli, "%% Ingres Bandwidth Profile : %s\n",
              (bw_profile->ingress_uni_bw_profile->active == PAL_TRUE) ?
              "Active" : "Not Active");
          cli_out (cli, " %-8s%-8s%-8s%-8s%-16s%-12s\n", "CIR", "CBS", "EIR",
              "EBS", "Coupling-flag", "Color-mode");
          cli_out (cli, " ======= ======= ======= ======= =============== "
              "============ \n");
         cli_out (cli, " %-8d%-8d%-8d%-8d%-16s%-12s\n",
           bw_profile->ingress_uni_bw_profile->comm_info_rate,
           bw_profile->ingress_uni_bw_profile->comm_burst_size,
           bw_profile->ingress_uni_bw_profile->excess_info_rate,
           bw_profile->ingress_uni_bw_profile->excess_burst_size,
           (bw_profile->ingress_uni_bw_profile->coupling_flag == PAL_TRUE)?
           "enable" : "disable",
           (bw_profile->ingress_uni_bw_profile->color_mode == PAL_TRUE)?
           "color-aware" : "color-blind");
         cli_out (cli, " \n");
        }
      else if (bw_profile->ingress_bw_profile_type == NSM_BW_PROFILE_PER_EVC)
        {

          LIST_LOOP (bw_profile->uni_evc_bw_profile_list, evc_bw_profile,
              evc_node)
            {
              if (evc_bw_profile->ingress_bw_profile_type ==
                  NSM_EVC_BW_PROFILE_PER_EVC) 
                {
                  cli_out (cli, "%% Ingress Bandwidth Profile : Per EVC, "
                      "VLAN-ID :  %d \n", evc_bw_profile->instance_id);
                  cli_out (cli, "%% Ingres Bandwidth Profile : %s\n",
                      (evc_bw_profile->ingress_evc_bw_profile->active == PAL_TRUE) ?
                      "Active" : "Not Active");
                  cli_out (cli, " %-8s%-8s%-8s%-8s%-16s%-12s\n", "CIR", "CBS", "EIR",
                      "EBS", "Coupling-flag", "Color-mode");
                  cli_out (cli, " ======= ======= ======= ======= =============== "
                      "============ \n");

                  cli_out (cli, " %-8d%-8d%-8d%-8d%-16s%-12s\n",
                      evc_bw_profile->ingress_evc_bw_profile->comm_info_rate,
                      evc_bw_profile->ingress_evc_bw_profile->comm_burst_size,
                      evc_bw_profile->ingress_evc_bw_profile->excess_info_rate,
                      evc_bw_profile->ingress_evc_bw_profile->excess_burst_size,
                      (evc_bw_profile->ingress_evc_bw_profile->coupling_flag ==
                       PAL_TRUE)? "enable" : "disable",
                      (evc_bw_profile->ingress_evc_bw_profile->color_mode ==
                       PAL_TRUE)? "color-aware" : "color-blind");
                  cli_out (cli, " \n");
                }
              else if (evc_bw_profile->ingress_bw_profile_type ==
                  NSM_EVC_COS_BW_PROFILE)
                {
                  cli_out (cli, "%% Ingress Bandwidth Profile : Per COS"
                      ", VLAN-ID : %d \n", evc_bw_profile->instance_id);
                  for (i = 0; i < NSM_MAX_COS_PER_EVC; i++)
                    {
                      if (evc_bw_profile->num_of_ingress_cos_id [i] == PAL_TRUE)
                        {
                          cli_out (cli, "%% Ingres Bandwidth Profile COS ID : "
                              "%d \n", i + 1);

                          for (j = 0; j < NSM_MAX_COS_PER_EVC; j++)
                            {
                              if (evc_bw_profile->ingress_CoS_id[j]
                                  == (i + 1))
                                {
                                  if (first == PAL_FALSE)
                                    {
                                      cli_out (cli, "%% Ingress Bandwidth Profile"
                                          "per COS : %s\n", (evc_bw_profile->
                                           ingress_evc_cos_bw_profile[j]->active
                                           == PAL_TRUE)? "Active" : "Not active");

                                      cli_out (cli, " %-4s%-8s%-8s%-8s%-8s"
                                          "%-16s%-12s\n", "COS", "CIR",
                                          "CBS", "EIR", "EBS", "Coupling-flag",
                                          "Color-mode");
                                      cli_out (cli, " === ======= ======= ======= "
                                          "======= =============== "
                                          "============ \n");
                                      first = PAL_TRUE;
                                    }

                                  cli_out (cli, " %-4d%-8d%-8d%-8d%-8d%-16s%-12s\n",
                                      j,
                                      evc_bw_profile->
                                      ingress_evc_cos_bw_profile[j]->comm_info_rate,
                                      evc_bw_profile->
                                      ingress_evc_cos_bw_profile[j]->comm_burst_size,
                                      evc_bw_profile->
                                      ingress_evc_cos_bw_profile[j]->excess_info_rate,
                                      evc_bw_profile->
                                      ingress_evc_cos_bw_profile[j]->excess_burst_size,
                                      (evc_bw_profile->
                                       ingress_evc_cos_bw_profile[j]->coupling_flag ==
                                       PAL_TRUE)? "enable" : "disable",
                                      (evc_bw_profile->
                                       ingress_evc_cos_bw_profile[j]->color_mode ==
                                       PAL_TRUE)? "color-aware" : "color-blind");

                                }/* if (evc_bw_profile->ingress_CoS_id[j]
                                  == (i + 1))*/
                            }/* for (j = 0; j < NSM_MAX_COS_PER_EVC; j++)*/
                          first = PAL_FALSE;

                          cli_out (cli, " \n");
                        }/*if (evc_bw_profile->num_of_ingress_cos_id [i]
                            == PAL_TRUE)*/
                    }/* for (i = 0; i < NSM_MAX_COS_PER_EVC; i++)*/
                } /* else if (evc_bw_profile->ingress_bw_profile_type ==
                  NSM_EVC_COS_BW_PROFILE)*/

            }/* LIST_LOOP (bw_profile->uni_evc_bw_profile_list, evc_bw_profile,
              evc_node)*/

        }
    }
  else
    {
      cli_out (cli, "%% Ingress Bandwidth Profile : Not Configured \n");
    }


  /* Display Egress Bandwidth Profile */
  if (bw_profile->egress_bw_profile_status == BW_PROFILE_CONFIGURED)
    {
      if (bw_profile->egress_bw_profile_type == NSM_BW_PROFILE_PER_UNI)
        {
          cli_out (cli, "%% Egress Bandwidth Profile : Per UNI\n");
          cli_out (cli, "%% Egres Bandwidth Profile : %s\n",
              (bw_profile->egress_uni_bw_profile->active == PAL_TRUE) ?
              "Active" : "Not Active");
          cli_out (cli, " %-8s%-8s%-8s%-8s%-16s%-12s\n", "CIR", "CBS", "EIR",
              "EBS", "Coupling-flag", "Color-mode");
          cli_out (cli, " ======= ======= ======= ======= =============== "
              "============ \n");

          cli_out (cli, " %-8d%-8d%-8d%-8d%-16s%-12s\n",
              bw_profile->egress_uni_bw_profile->comm_info_rate,
           bw_profile->egress_uni_bw_profile->comm_burst_size,
           bw_profile->egress_uni_bw_profile->excess_info_rate,
           bw_profile->egress_uni_bw_profile->excess_burst_size,
           (bw_profile->egress_uni_bw_profile->coupling_flag == PAL_TRUE)?
           "enable" : "disable",
           (bw_profile->egress_uni_bw_profile->color_mode == PAL_TRUE)?
           "color-aware" : "color-blind");
          cli_out (cli, " \n");
        }
      else if (bw_profile->egress_bw_profile_type == NSM_BW_PROFILE_PER_EVC)
        {

          LIST_LOOP (bw_profile->uni_evc_bw_profile_list, evc_bw_profile,
              evc_node)
            {
              if (evc_bw_profile->egress_bw_profile_type ==
                  NSM_EVC_BW_PROFILE_PER_EVC) 
                {
                  cli_out (cli, "%% Egress Bandwidth Profile : Per EVC, "
                      "VLAN-ID :  %d \n", evc_bw_profile->instance_id);
                  cli_out (cli, "%% Egres Bandwidth Profile : %s\n",
                      (evc_bw_profile->egress_evc_bw_profile->active == PAL_TRUE) ?
                      "Active" : "Not Active");

                  cli_out (cli, " %-8s%-8s%-8s%-8s%-16s%-12s\n", "CIR", "CBS", "EIR",
                      "EBS", "Coupling-flag", "Color-mode");
                  cli_out (cli, " ======= ======= ======= ======= =============== "
                      "============ \n");

                  cli_out (cli, " %-8d%-8d%-8d%-8d%-16s%-12s\n",
                      evc_bw_profile->egress_evc_bw_profile->comm_info_rate,
                      evc_bw_profile->egress_evc_bw_profile->comm_burst_size,
                      evc_bw_profile->egress_evc_bw_profile->excess_info_rate,
                      evc_bw_profile->egress_evc_bw_profile->excess_burst_size,
                      (evc_bw_profile->egress_evc_bw_profile->coupling_flag ==
                       PAL_TRUE)? "enable" : "disable",
                      (evc_bw_profile->egress_evc_bw_profile->color_mode ==
                       PAL_TRUE)? "color-aware" : "color-blind");
                  cli_out (cli, " \n");
                }
              else if (evc_bw_profile->egress_bw_profile_type ==
                  NSM_EVC_COS_BW_PROFILE)
                {
                  cli_out (cli, "%% Egress Bandwidth Profile : Per COS"
                      ", VLAN-ID : %d \n", evc_bw_profile->instance_id);
                  for (i = 0; i < NSM_MAX_COS_PER_EVC; i++)
                    {
                      if (evc_bw_profile->num_of_egress_cos_id [i] == PAL_TRUE)
                        {
                          cli_out (cli, "%% Egress Bandwidth Profile COS ID : "
                              "%d \n", i + 1);

                          for (j = 0; j < NSM_MAX_COS_PER_EVC; j++)
                            {
                              if (evc_bw_profile->egress_CoS_id[j]
                                  == (i + 1))
                                {
                                  if (first == PAL_FALSE)
                                    {
                                      cli_out (cli, "%% Egress Bandwidth Profile"
                                          "per COS : %s\n", (evc_bw_profile->
                                           egress_evc_cos_bw_profile[j]->active
                                           == PAL_TRUE)? "Active" : "Not active");

                                      cli_out (cli, " %-4s%-8s%-8s%-8s%-8s"
                                          "%-16s%-12s\n", "COS", "CIR",
                                          "CBS", "EIR", "EBS", "Coupling-flag",
                                          "Color-mode");
                                      cli_out (cli, " === ======= ======= ======= "
                                          "======= =============== "
                                          "============ \n");
                                      first = PAL_TRUE;
                                    }

                                  cli_out (cli, " %-4d%-8d%-8d%-8d%-8d%-16s%-12s\n",
                                      j,
                                      evc_bw_profile->
                                      egress_evc_cos_bw_profile[j]->comm_info_rate,
                                      evc_bw_profile->
                                      egress_evc_cos_bw_profile[j]->comm_burst_size,
                                      evc_bw_profile->
                                      egress_evc_cos_bw_profile[j]->excess_info_rate,
                                      evc_bw_profile->
                                      egress_evc_cos_bw_profile[j]->excess_burst_size,
                                      (evc_bw_profile->
                                      egress_evc_cos_bw_profile[j]->coupling_flag ==
                                       PAL_TRUE)? "enable" : "disable",
                                      (evc_bw_profile->
                                       egress_evc_cos_bw_profile[j]->color_mode ==
                                       PAL_TRUE)? "color-aware" : "color-blind");

                                }/* if (evc_bw_profile->egress_CoS_id[j]
                                  == (i + 1))*/
                            }/* for (j = 0; j < NSM_MAX_COS_PER_EVC; j++)*/

                          first = PAL_FALSE;
                          cli_out (cli, " \n");
                        }/*if (evc_bw_profile->num_of_egress_cos_id [i]
                            == PAL_TRUE)*/
                    }/* for (i = 0; i < NSM_MAX_COS_PER_EVC; i++)*/
                } /* else if (evc_bw_profile->egress_bw_profile_type ==
                  NSM_EVC_COS_BW_PROFILE)*/

            }/* LIST_LOOP (bw_profile->uni_evc_bw_profile_list, evc_bw_profile,
              evc_node)*/

        }
    }
  else
    {
      cli_out (cli, "%% Egress Bandwidth Profile : Not Configured \n");
    }

  return;
}
    
CLI(nsm_show_bw_profile_per_uni_cli,
    nsm_show_bw_profile_per_uni_cmd,
    "show ethernet bandwidth-profile (interface INTERFACE_NAME | "
    "(bridge <1-32>|))",
    CLI_SHOW_STR,
    "ethernet service parameters",
    "bandwidth profile parameters",
    "interface for which the bw profiling need to be displayed",
    "enter the interface name",
    "bridge for which th bw profiling need to be displayed",
    "enter the bridge name")
{
  struct interface *ifp = NULL;
  struct nsm_if *zif = NULL;
  struct nsm_bridge_port *br_port = NULL;
  struct nsm_vlan_port *vlan_port = NULL;
  struct nsm_bridge *bridge = NULL;
  struct nsm_master *nm = cli->vr->proto;
  struct nsm_bridge_master *master = NULL;
  struct avl_node *br_port_node = NULL;
  u_int8_t brno = PAL_FALSE;
  
  if ((argc == 2) && (argv[0][0] == 'i'))
    {
      ifp = if_lookup_by_name (&cli->vr->ifm, argv[1]);

      if (!ifp)
        {
          cli_out (cli, "%% Interface not found\n");
          return CLI_ERROR;
        }

      if (! (zif = ifp->info)
          || ! (br_port = zif->switchport)
          || ! (vlan_port = &br_port->vlan_port))
        {
          cli_out (cli, "%% Interface does not exist: %s\n", argv[1]);
          return CLI_ERROR;
        }

      if (! (bridge = zif->bridge))
        {
          cli_out (cli, "%% Interface not bound to bridge: %s\n", argv[1]);
          return CLI_ERROR;
        }

      if (! NSM_BRIDGE_TYPE_PROVIDER (bridge))
        {
          cli_out (cli, "%% Interface not a bound to a provider bridge: %s\n",
              argv[1]);
          return CLI_ERROR;
        }
          
      if (vlan_port->mode != NSM_VLAN_PORT_MODE_CE)
        {
          cli_out (cli, "%% Port is not a Customer Edge Port\n");
          return CLI_ERROR;
        }
      nsm_show_bw_profiling (cli, bridge, zif);

    }
  else
    {
      master = nsm_bridge_get_master(nm);
      if (! master)
        {
          cli_out (cli, "%% Bridge master not configured\n");
          return CLI_ERROR;
        }
      if (argc == 2)
        {
          CLI_GET_INTEGER_RANGE ("Max bridge groups", brno, argv[1], 1, 32);

          bridge = nsm_lookup_bridge_by_name (master, argv[1]);
        }
      else
        {
          bridge = nsm_get_default_bridge (master);
        }

      if (bridge == NULL)
        {
          cli_out (cli, "bridge not found\n");
          return CLI_ERROR;
        }

      if (! NSM_BRIDGE_TYPE_PROVIDER (bridge))
        {
          cli_out (cli, "%% Bridge is not a provider edge bridge. \n");
          return CLI_ERROR;
        }

      if (NSM_BRIDGE_TYPE_PROVIDER (bridge) &&
          (bridge->provider_edge == PAL_FALSE))
        {
          cli_out (cli, "%% Bridge is not a provider edge bridge. \n");
          return CLI_ERROR;

        }

      cli_out (cli, "\n");
      cli_out (cli, " Bridge Name         :  %s\n", argv[1]);
      cli_out (cli, " ==================================\n");

      AVL_TREE_LOOP (bridge->port_tree, br_port, br_port_node)
        {
          if ((zif = br_port->nsmif) == NULL)
            continue;

          if ((ifp = zif->ifp) == NULL)
            continue;

           if (! (zif->nsm_bw_profile))
            {
              continue;
            }

         vlan_port = &br_port->vlan_port;

          if (! vlan_port)
            continue;

          if (NSM_BRIDGE_TYPE_PROVIDER (bridge) &&
              (vlan_port->mode != NSM_VLAN_PORT_MODE_CE))
            {
              continue;
            }
          nsm_show_bw_profiling (cli, bridge, zif);
        }
    }
  return CLI_SUCCESS;

}


/**@brief  - Function to display the bw profile p.arameters for uni or bw 
 *           profile parameters for all the interfaces associated to the bridge
 * @param cli         - Pointer to imish terminal.
 * @param bridge      - Bridge associated interfaces on which the bw profile
 *                        parameters need to be displayed.
 * @param zif         - nsm interface for which the bw profile parameters need
 *                        to be displayed.
 * @param instance_id - instance id for which the BW profiling need to be
 *                        displayed.
 * @param evc_id      - EVC-ID for which the BW profiling need to be displayed.
 * @return            - return control to the CLI 
 * */
static void 
nsm_show_evc_bw_profiling (struct cli *cli, struct nsm_bridge *bridge,
    struct nsm_if *zif, u_int16_t instance_id, u_int8_t *evc_id)
{
  struct nsm_band_width_profile *bw_profile = NULL;
  struct nsm_uni_evc_bw_profile *evc_bw_profile = NULL;
  struct listnode *evc_node = NULL;
  u_int8_t i = PAL_FALSE;
  u_int8_t j = PAL_FALSE;
  u_int8_t found = PAL_FALSE;

  if (! (zif->nsm_bw_profile))
    return;

  bw_profile = zif->nsm_bw_profile;

  cli_out (cli, "%% Interface Name : %s\n", zif->ifp->name);
  cli_out (cli, "=========================\n");
  
  if (bw_profile->ingress_bw_profile_status == BW_PROFILE_CONFIGURED)
    {
      if (bw_profile->ingress_bw_profile_type == NSM_BW_PROFILE_PER_UNI)
        {
          cli_out (cli, "%% Ingress Bandwidth Profile : Per UNI\n");
        }
      else if (bw_profile->ingress_bw_profile_type == NSM_BW_PROFILE_PER_EVC)
        {
          LIST_LOOP (bw_profile->uni_evc_bw_profile_list, evc_bw_profile,
              evc_node)
            {
              if (evc_id != NULL)
                {
                  if (pal_strcmp (evc_bw_profile->evc_id, evc_id) == 0)
                    {
                      found = PAL_TRUE;
                      break;
                    }
                }
              else
                {
                  if (evc_bw_profile->instance_id == instance_id)
                    {
                      found = PAL_TRUE;
                      break;
                    }
                }
            }/* LIST_LOOP (bw_profile->uni_evc_bw_profile_list, evc_bw_profile,
                              evc_node)*/
          if (found == PAL_FALSE)
            {
              cli_out (cli, "%% Ingress bandwidth profile not configured for the "
                  "evc\n");
            }
          else
            {

          if (evc_bw_profile->ingress_bw_profile_type ==
              NSM_EVC_BW_PROFILE_PER_EVC) 
            {
              cli_out (cli, "%% Ingress Bandwidth Profile : Per EVC, "
                  "VLAN-ID :  %d \n", evc_bw_profile->instance_id);
              cli_out (cli, "%% Ingres Bandwidth Profile : %s\n",
                  (evc_bw_profile->ingress_evc_bw_profile->active == PAL_TRUE) ?
                  "Active" : "Not Active");
              cli_out (cli, " %-8s%-8s%-8s%-8s%-16s%-12s\n", "CIR", "CBS", "EIR",
                  "EBS", "Coupling-flag", "Color-mode");
              cli_out (cli, " ======= ======= ======= ======= =============== "
                  "============ \n");
              cli_out (cli, " %-8d%-8d%-8d%-8d%-16s%-12s\n",
                  evc_bw_profile->ingress_evc_bw_profile->comm_info_rate,
                  evc_bw_profile->ingress_evc_bw_profile->comm_burst_size,
                  evc_bw_profile->ingress_evc_bw_profile->excess_info_rate,
                  evc_bw_profile->ingress_evc_bw_profile->excess_burst_size,
                  (evc_bw_profile->ingress_evc_bw_profile->coupling_flag ==
                   PAL_TRUE)? "enable" : "disable",
                  (evc_bw_profile->ingress_evc_bw_profile->color_mode ==
                   PAL_TRUE)? "color-aware" : "color-blind");
            }
          else if (evc_bw_profile->ingress_bw_profile_type ==
              NSM_EVC_COS_BW_PROFILE)
            {
              cli_out (cli, "%% Ingress Bandwidth Profile : Per COS"
                  ", VLAN-ID : %d \n", evc_bw_profile->instance_id);
              for (i = 0; i < NSM_MAX_COS_PER_EVC; i++)
                {
                  if (evc_bw_profile->num_of_ingress_cos_id [i] == PAL_TRUE)
                    {
                      cli_out (cli, " %-4s%-8s%-8s%-8s%-8s"
                          "%-16s%-12s%-16s\n", "COS", "CIR",
                          "CBS", "EIR", "EBS", "Coupling-flag",
                          "Color-mode", "Active/Inactive");
                      cli_out (cli, " === ======= ======= ======= "
                          "======= =============== "
                          "============ ================\n");

                      for (j = 0; j < NSM_MAX_COS_PER_EVC; j++)
                        {
                          if (evc_bw_profile->ingress_CoS_id[j]
                              == (i + 1))
                            {
                              cli_out (cli, "%% %-4d%-8d%-8d%-8d"
                                  "%-8d%-16s%-12s%-16s\n",
                                  j,
                                  evc_bw_profile->
                                  ingress_evc_cos_bw_profile[j]->comm_info_rate,
                                  evc_bw_profile->
                                  ingress_evc_cos_bw_profile[j]->comm_burst_size,
                                  evc_bw_profile->
                                  ingress_evc_cos_bw_profile[j]->excess_info_rate,
                                  evc_bw_profile->
                                  ingress_evc_cos_bw_profile[j]->excess_burst_size,
                                  (evc_bw_profile->
                                   ingress_evc_cos_bw_profile[j]->coupling_flag ==
                                   PAL_TRUE)? "enable" : "disable",
                                  (evc_bw_profile->
                                   ingress_evc_cos_bw_profile[j]->color_mode ==
                                   PAL_TRUE)? "color-aware" : "color-blind",
                                  (evc_bw_profile->
                                   ingress_evc_cos_bw_profile[j]->active ==
                                   PAL_TRUE)? "Active" : "Inactive");

                            }/* if (evc_bw_profile->ingress_CoS_id[j]
                                == (i + 1))*/
                        }/* for (j = 0; j < NSM_MAX_COS_PER_EVC; j++)*/
                    }/*if (evc_bw_profile->num_of_ingress_cos_id [i]
                       == PAL_TRUE)*/
                }/* for (i = 0; i < NSM_MAX_COS_PER_EVC; i++)*/
            } /* else if (evc_bw_profile->ingress_bw_profile_type ==
                 NSM_EVC_COS_BW_PROFILE)*/
            }
        }/* else if (bw_profile->ingress_bw_profile_type ==
            NSM_BW_PROFILE_PER_EVC)*/
    }/*if (bw_profile->ingress_bw_profile_status == BW_PROFILE_CONFIGURED)*/
  else
    {
      cli_out (cli, "%% Ingress Bandwidth Profile : Not Configured \n");
    }


  /* Display Egress Bandwidth Profile */
  if (bw_profile->egress_bw_profile_status == BW_PROFILE_CONFIGURED)
    {
      if (bw_profile->egress_bw_profile_type == NSM_BW_PROFILE_PER_UNI)
        {
          cli_out (cli, "%% Egress Bandwidth Profile : Per UNI\n");
        }
      else if (bw_profile->egress_bw_profile_type == NSM_BW_PROFILE_PER_EVC)
        {

          LIST_LOOP (bw_profile->uni_evc_bw_profile_list, evc_bw_profile,
              evc_node)
            {
              if (evc_id != NULL)
                {
                  if (pal_strcmp (evc_bw_profile->evc_id, evc_id) == 0)
                    {
                      break;
                    }
                }
              else
                {
                  if (evc_bw_profile->instance_id == instance_id)
                    break;
                }
            }

          if (!evc_bw_profile)
            {
              cli_out (cli, "%% Egress bandwidth profile not configured for the "
                  "evc\n");
              return;
            }

          if (evc_bw_profile->egress_bw_profile_type ==
              NSM_EVC_BW_PROFILE_PER_EVC) 
            {
              cli_out (cli, "%% Egress Bandwidth Profile : Per EVC, "
                  "VLAN-ID :  %d \n", evc_bw_profile->instance_id);
              cli_out (cli, "%% Egres Bandwidth Profile : %s\n",
                  (evc_bw_profile->egress_evc_bw_profile->active == PAL_TRUE) ?
                  "Active" : "Not Active");
              cli_out (cli, " %-8s%-8s%-8s%-8s%-16s%-12s\n", "CIR", "CBS", "EIR",
                  "EBS", "Coupling-flag", "Color-mode");
              cli_out (cli, " ======= ======= ======= ======= =============== "
                  "============ \n");
              cli_out (cli, " %-8d%-8d%-8d%-8d%-16s%-12s\n",
                  evc_bw_profile->egress_evc_bw_profile->comm_info_rate,
                  evc_bw_profile->egress_evc_bw_profile->comm_burst_size,
                  evc_bw_profile->egress_evc_bw_profile->excess_info_rate,
                  evc_bw_profile->egress_evc_bw_profile->excess_burst_size,
                  (evc_bw_profile->egress_evc_bw_profile->coupling_flag ==
                   PAL_TRUE)? "enable" : "disable",
                  (evc_bw_profile->egress_evc_bw_profile->color_mode ==
                   PAL_TRUE)? "color-aware" : "color-blind");
            }
          else if (evc_bw_profile->egress_bw_profile_type ==
              NSM_EVC_COS_BW_PROFILE)
            {
              cli_out (cli, "%% Egress Bandwidth Profile : Per COS"
                  ", VLAN-ID : %d \n", evc_bw_profile->instance_id);
              for (i = 0; i < NSM_MAX_COS_PER_EVC; i++)
                {
                  if (evc_bw_profile->num_of_egress_cos_id [i] == PAL_TRUE)
                    {
                      cli_out (cli, " %-4s%-8s%-8s%-8s%-8s"
                          "%-16s%-12s%-16s\n", "COS", "CIR",
                          "CBS", "EIR", "EBS", "Coupling-flag",
                          "Color-mode", "Active/Inactive");
                      cli_out (cli, " === ======= ======= ======= "
                          "======= =============== "
                          "============ ================\n");


                      for (j = 0; j < NSM_MAX_COS_PER_EVC; j++)
                        {
                          if (evc_bw_profile->egress_CoS_id[j]
                              == (i + 1))
                            {
                              cli_out (cli, "%% %-4d%-8d%-8d%-8d"
                                  "%-8d%-16s%-12s%-16s\n",
                                  j,
                                  evc_bw_profile->
                                  egress_evc_cos_bw_profile[j]->comm_info_rate,
                                  evc_bw_profile->
                                  egress_evc_cos_bw_profile[j]->comm_burst_size,
                                  evc_bw_profile->
                                  egress_evc_cos_bw_profile[j]->excess_info_rate,
                                  evc_bw_profile->
                                  egress_evc_cos_bw_profile[j]->excess_burst_size,
                                  (evc_bw_profile->
                                   egress_evc_cos_bw_profile[j]->coupling_flag ==
                                   PAL_TRUE)? "enable" : "disable",
                                  (evc_bw_profile->
                                   egress_evc_cos_bw_profile[j]->color_mode ==
                                   PAL_TRUE)? "color-aware" : "color-blind",
                                  (evc_bw_profile->
                                   egress_evc_cos_bw_profile[j]->active ==
                                   PAL_TRUE)? "Active" : "Inactive");

                            }/* if (evc_bw_profile->egress_CoS_id[j]
                                == (i + 1))*/
                        }/* for (j = 0; j < NSM_MAX_COS_PER_EVC; j++)*/
                    }/*if (evc_bw_profile->num_of_egress_cos_id [i]
                       == PAL_TRUE)*/
                }/* for (i = 0; i < NSM_MAX_COS_PER_EVC; i++)*/
            } /* else if (evc_bw_profile->egress_bw_profile_type ==
                 NSM_EVC_COS_BW_PROFILE)*/
        }/* else if (bw_profile->egress_bw_profile_type ==
            NSM_BW_PROFILE_PER_EVC)*/
    }/* if (bw_profile->egress_bw_profile_status == BW_PROFILE_CONFIGURED)*/
  else
    {
      cli_out (cli, "%% Egress Bandwidth Profile : Not Configured \n");
    }

  return;
}

CLI(nsm_show_bw_profile_per_evc_cli,
    nsm_show_bw_profile_per_evc_cmd,
    "show ethernet bandwidth-profile interface INTERFACE_NAME "
    "(evc-id EVC_ID | instance-id INSTANCE_ID)",
    CLI_SHOW_STR,
    "ethernet service parameters",
    "bandwidth profile parameters",
    "interface for which the bw profiling need to be displayed",
    "enter the interface name",
    "evc id for which the bandwidth profile need to be displayed",
    "enter the evc id",
    "instance id for which the bandwidth profile need to be displayed",
    "enter the instance id")
{
  struct interface *ifp = NULL;
  struct nsm_if *zif = NULL;
  struct nsm_bridge_port *br_port = NULL;
  struct nsm_vlan_port *vlan_port = NULL;
  struct nsm_bridge *bridge = NULL;
  u_int32_t instance_id = PAL_FALSE;
  
  ifp = if_lookup_by_name (&cli->vr->ifm, argv[1]);

  if (!ifp)
    {
      cli_out (cli, "%% Interface not found\n");
      return CLI_ERROR;
    }

  if (! (zif = ifp->info)
      || ! (br_port = zif->switchport)
      || ! (vlan_port = &br_port->vlan_port))
    {
      cli_out (cli, "%% Interface does not exist: %s\n", argv[1]);
      return CLI_ERROR;
    }

  if (! (bridge = zif->bridge))
    {
      cli_out (cli, "%% Interface not bound to bridge: %s\n", argv[1]);
      return CLI_ERROR;
    }

  if (! NSM_BRIDGE_TYPE_PROVIDER (bridge))
    {
      cli_out (cli, "%% Interface not a bound to a provider bridge: %s\n",
          argv[0]);
      return CLI_ERROR;
    }
  if (vlan_port->mode != NSM_VLAN_PORT_MODE_CE)
    {
      cli_out (cli, "%% Port is not a Customer Edge Port\n");
      return CLI_ERROR;
    }

  if (argv[2][0] == 'i')
    {
      CLI_GET_INTEGER_RANGE ("Max Instance id", instance_id, argv[3], 2, 4094);
      
      nsm_show_evc_bw_profiling (cli, bridge, zif, instance_id, NULL);
    }
  else
    {
      nsm_show_evc_bw_profiling (cli, bridge, zif, PAL_FALSE, argv[3]);
    }

  return CLI_SUCCESS;
}

#endif /* HAVE_PROVIDER_BRIDGE */

#ifdef HAVE_G8031
#ifdef HAVE_B_BEB
#ifdef HAVE_I_BEB
/* CLI to create protection group with working and protection ports for BEB*/
CLI (g8031_i_b_beb_eps_protection_group_create,
    g8031_i_b_beb_eps_protection_group_create_cmd,
    "bridge (<1-32> | backbone) g8031 eps-id EPS-ID working-port IFNAME"
    " protection-port IFNAME (instance <1-63> |)",
    NSM_BRIDGE_STR,
    NSM_BRIDGE_NAME_STR,
    "backbone bridge",
    "parameters are G.8031 related",
    "unique id to identify a EPS protection link",
    "EPS protection group ID",
    "port for the working EPS path",
    CLI_IFNAME_STR,
    "port for the protection EPS path",
    CLI_IFNAME_STR,
    "Instance",
    "Id")
{
    int epsid             = 0;
    u_int16_t instance    = 0;
    struct nsm_master *nm = cli->vr->proto;
    int ret;

    CLI_GET_INTEGER_RANGE ("Max protection groups", epsid, argv[1], 1, 4094);
    CLI_GET_INTEGER_RANGE ("instance", instance, argv[5], 1, 63);

    if (argc < 4)
      {
        ret = nsm_api_bridge_get_hw_instance (&instance);

       if (ret < 0)
         {
           cli_out(cli," %% Failed to get instance. Enter instance number\n");
           return CLI_ERROR;
         }
      } 

    switch(nsm_create_g8031_protection_group (argv[0], argv[2], 
                                              argv[3], instance, epsid, nm))
      {
      case RESULT_OK:
        break;
      case NSM_BRIDGE_ERR_NOTFOUND:
        cli_out(cli,"%% Bridge not found \n");
        return CLI_ERROR;
      case NSM_BRIDGE_PG_ALREADY_EXISTS:
        cli_out(cli,"%% Protection group with EPS-ID %d already exists\n",epsid);
        return CLI_ERROR;
      case NSM_INTERFACE_NOT_FOUND:
        cli_out(cli,"%% Interface not found \n");
        return CLI_ERROR;
      case NSM_INTERFACE_NOT_UP:
        cli_out(cli,"%% Interface not up\n");
        return CLI_ERROR;
      case NSM_G8031_INSTANCE_IN_USE:
        cli_out(cli, "%% Instance is in use\n");
        return CLI_ERROR;
      default:
        break;
      }
    return CLI_SUCCESS;
}

/* CLI to remove protection group for working and protection ports for BEB*/
CLI (g8031_i_b_beb_eps_protection_group_delete,
    g8031_i_b_beb_eps_protection_group_delete_cmd,
    "no bridge (<1-32> | backbone) g8031 eps-id EPS-ID",
    CLI_NO_STR,
    NSM_BRIDGE_STR,
    NSM_BRIDGE_NAME_STR,
    "backbone bridge",
    "parameters are G.8031 related",
    "unique id to identify a EPS protection link",
    "EPS protection group ID")

{
    int eps_id                       = 0 ;
    int brno                         = 0 ;
    struct nsm_master *nm            = cli->vr->proto;

    if (argv[0][0]!='b')
      CLI_GET_INTEGER_RANGE ("Max bridge groups", brno, argv[0], 1, 32);

    CLI_GET_INTEGER_RANGE ("Max Protection groups", eps_id, argv[1], 1, 4094);

    switch(nsm_delete_g8031_protection_group (argv[0],eps_id,nm))
      {
      case RESULT_ERROR:
        cli_out (cli,"%% Could not delete protection group");
        return (CLI_ERROR);
      case RESULT_OK:
        break;
      }

    return CLI_SUCCESS;
}
#else
/*CLI to create protection group with working and protection ports for BEB*/
CLI (g8031_beb_eps_protection_group_create,
    g8031_beb_eps_protection_group_create_cmd,
    "bridge backbone g8031 eps-id EPS-ID working-port IFNAME"
    " protection-port IFNAME (instance <1-63> |)",
    NSM_BRIDGE_STR,
    "backbone bridge",
    "parameters are G.8031 related",
    "unique id to identify a EPS protection link",
    "EPS protection group ID",
    "port for the working EPS path",
    CLI_IFNAME_STR,
    "port for the protection EPS path",
    CLI_IFNAME_STR,
    "Instance",
    "Id")
{
    int epsid             = 0;
    u_int16_t instance    = 0;
    struct nsm_master *nm = cli->vr->proto;
    int ret;

    CLI_GET_INTEGER_RANGE ("Max protection groups", epsid, argv[1], 1, 4094);
    CLI_GET_INTEGER_RANGE ("instance", instance, argv[5], 1,63); 

    if (argc < 4)
      {
        ret = nsm_api_bridge_get_hw_instance (&instance);

        if (ret < 0)
          {
            cli_out(cli," %% Failed to get instance. Enter instance number\n");
            return CLI_ERROR;
          }
      }
    
    switch(nsm_create_g8031_protection_group (argv[0], argv[2], argv[3], 
                                              instance, epsid, nm))
      {
      case RESULT_OK:
        break;
      case NSM_BRIDGE_ERR_NOTFOUND:
        cli_out(cli," bridge not found \n");
        return CLI_ERROR;
      case NSM_BRIDGE_PG_ALREADY_EXISTS:
        cli_out(cli,"Protection group with EPS-ID %d already exists\n",epsid);
        return CLI_ERROR;
      case NSM_INTERFACE_NOT_FOUND:
        cli_out(cli," interface not found \n");
        return CLI_ERROR;
      case NSM_INTERFACE_NOT_UP:
        cli_out(cli,"Interface not up\n");
        return CLI_ERROR;
      case NSM_G8031_INSTANCE_IN_USE:
        cli_out(cli, "Instance is in use\n");
        return CLI_ERROR;
      default:
        break;
      }
    return CLI_SUCCESS;
}

/* CLI to remove protection group for working and protection ports for BEB*/
CLI (g8031_beb_eps_protection_group_delete,
    g8031_beb_eps_protection_group_delete_cmd,
    "no bridge backbone g8031 eps-id EPS-ID",
    CLI_NO_STR,
    NSM_BRIDGE_STR,
    "backbone bridge",
    "parameters are G.8031 related",
    "unique id to identify a EPS protection link",
    "EPS protection group ID")

{
    int eps_id                       = 0 ;
    struct nsm_master *nm            = cli->vr->proto;

    CLI_GET_INTEGER_RANGE ("Max Protection groups", eps_id, argv[1], 1, 4094);

    switch(nsm_delete_g8031_protection_group (argv[0],eps_id,nm))
      {
      case RESULT_ERROR:
        cli_out (cli,"%% Could not delete protection group");
        return (CLI_ERROR);
      case RESULT_OK:
        break;
      }

    return CLI_SUCCESS;
}
#endif /* HAVE_I_BEB */

#else
/* CLI to create protection group with working and protection ports */
CLI (g8031_eps_protection_group_create,
     g8031_eps_protection_group_create_cmd,
     "bridge <1-32> g8031 eps-id EPS-ID working-port IFNAME"
      " protection-port IFNAME (instance <1-63> | )",
     NSM_BRIDGE_STR,
     NSM_BRIDGE_NAME_STR,
     "parameters are G.8031 related",
     "unique id to identify a EPS protection link",
     "EPS protection group ID",
     "port for the working EPS path",
     CLI_IFNAME_STR,
     "port for the protection EPS path",
     CLI_IFNAME_STR,
     "Instance",
     "Id")
{
  int epsid                 = 0;
  s_int16_t instance        = 0;
  struct nsm_master *nm     = cli->vr->proto;
  int ret = 0;

  CLI_GET_INTEGER_RANGE ("Max protection groups", epsid, argv[1], 1, 4094);
  CLI_GET_INTEGER_RANGE ("instance", instance, argv[5], 1,63); 

  if (argc < 4)
    {
      ret = nsm_api_bridge_get_hw_instance (&instance);

      if (ret < 0)
        {
          cli_out(cli," %% Failed to get instance. Enter instance number\n");
          return CLI_ERROR;
        }
    }
  
  switch(nsm_create_g8031_protection_group (argv[0], argv[2], argv[3],
                                            instance, epsid, nm))
    {
      case RESULT_OK:
        break;
      case NSM_BRIDGE_ERR_NOTFOUND:
        cli_out(cli,"%% Bridge %s not found \n", argv[0]);
        return CLI_ERROR;
      case NSM_BRIDGE_PG_ALREADY_EXISTS:
        cli_out(cli,"%% Protection group with EPS-ID %d already exists\n", epsid);
        return CLI_ERROR;
      case NSM_INTERFACE_NOT_FOUND:
        cli_out(cli,"%% Interface not found \n");
        return CLI_ERROR;
      case NSM_INTERFACE_NOT_UP:
        cli_out(cli,"%% Interface not up\n");
        return CLI_ERROR;
      case NSM_G8031_INSTANCE_IN_USE:
        cli_out(cli, "%% Instance is in use\n");
        return CLI_ERROR;
      default:
        break;
    }

  return CLI_SUCCESS;
}

/* CLI to remove protection group for working and protection ports */
CLI (g8031_eps_protection_group_delete,
     g8031_eps_protection_group_delete_cmd,
     "no bridge <1-32> g8031 eps-id EPS-ID",
     CLI_NO_STR,
     NSM_BRIDGE_STR,
     NSM_BRIDGE_NAME_STR,
     "parameters are G.8031 related",
     "unique id to identify a EPS protection link",
     "EPS protection group ID")

{
  int eps_id                       = 0 ;
  int brno                         = 0 ;
  struct nsm_master *nm            = cli->vr->proto;

  if (argv[0][0]!='b')
    CLI_GET_INTEGER_RANGE ("Max bridge groups", brno, argv[0], 1, 32);

  CLI_GET_INTEGER_RANGE ("Max Protection groups", eps_id, argv[1], 1, 4094);

  switch(nsm_delete_g8031_protection_group (argv[0],eps_id,nm))
    {
    case RESULT_ERROR:
      cli_out (cli,"%% Could not delete protection group");
      break;
    case RESULT_OK:
      break;
    }

  return CLI_SUCCESS;
}
#endif /*(defined HAVE_I_BEB || defined HAVE_B_BEB)*/
#endif /* HAVE_G8031 */

#ifdef HAVE_G8032

static s_int16_t
nsm_g8032_print_range_error (struct cli *cli, int result)
{
  switch (result)
    {
    case RESULT_ERROR:
      cli_out(cli, "%% G8032  Error");
      return CLI_ERROR;

    case NSM_BRIDGE_ERR_NOTFOUND:
      cli_out(cli, "%% Bridge does not exists \n");
      return CLI_ERROR;

    case NSM_BRIDGE_RING_ALREADY_EXISTS:
      cli_out(cli, "%% Ring with RING-ID already exists\n" );
      return CLI_ERROR;

    case NSM_INTERFACE_NOT_FOUND:
      cli_out(cli, "%% Interface not found \n");
      return CLI_ERROR;

    case NSM_G8032_INTERFACE_NAME_SAME_ERROR:
      cli_out(cli, "%% Same interface name not allowed\n");
      return CLI_ERROR;

    case NSM_G8032_RING_NODE_NOT_CREATED :
      cli_out (cli, "%% Ring creation Error\n");
      return CLI_ERROR;

   case G8032_RING_NOT_FOUND:
      cli_out (cli,"%%Ring  not found\n ");
      return (CLI_ERROR);

    case NSM_G8031_INSTANCE_IN_USE:
        cli_out(cli, "Instance is in use\n");
        return CLI_ERROR;
    }
  return CLI_ERROR;
}
  
#ifdef HAVE_B_BEB
CLI(nsm_bridge_beb_ring_add,
    nsm_bridge_beb_ring_br_add_cmd,
    "bridge (<1-32> | backbone) g8032 ring-id RINGID east-interface IFNAME" 
     " west-interface IFNAME (instance <1-63> |)",
    NSM_BRIDGE_STR,
    NSM_BRIDGE_NAME_STR,
    "backbone bridge",
    "g8032 configuration commands",
    "Unique ID to identify the Ether Ring Protection",
    "The ID assigned to an Ethernet Ring Protection group <1-65535>",
    "east interface",
    CLI_IFNAME_STR,
   "west interface",
    CLI_IFNAME_STR,
    "Instance",
    "Id")
{
  int ret;
  u_int16_t instance    = 0;
  unsigned int g8032_ring_id;
  struct nsm_master *nm = cli->vr->proto;

  CLI_GET_INTEGER_RANGE("Max Protection rings", g8032_ring_id, argv[1],
                        NSM_G8032_MIN_RING_VAL,
                        NSM_G8032_MAX_RING_VAL);
  CLI_GET_INTEGER_RANGE ("instance", instance, argv[5], 1, 63);

  if (argc < 4)
    {
      ret = nsm_api_bridge_get_hw_instance (&instance);
      if (ret < 0)
        {
          cli_out(cli," %% Failed to get instance. Enter instance number\n");
          return CLI_ERROR;
        }
    }

  ret = nsm_bridge_g8032_create_ring (argv[0], argv[2], argv[3], instance,
                                      g8032_ring_id, nm);
   if (ret != RESULT_OK)
    {
      nsm_g8032_print_range_error (cli, ret);
      return CLI_ERROR;
    }

  return CLI_SUCCESS;
}

CLI(no_nsm_bridge_beb_ring_delete,
    no_nsm_bridge_beb_ring_delete_cmd,
    "no bridge (<1-32> | backbone) g8032 ring-id RINGID ",
    CLI_NO_STR,
    NSM_BRIDGE_STR,
    NSM_BRIDGE_NAME_STR,
    "backbone bridge",
    "g8032 configuration commands",
    "Unique ID to identify the Ether Ring Protection",
    "The ID assigned to an Ethernet Ring Protection group <1-65535>")
{
  int ret = 0;
  int ring_id            = 0;
  int brno               = 0;
  struct nsm_master *nm  = cli->vr->proto;

  if (argv[0][0]!='b')
    CLI_GET_INTEGER_RANGE ("Max bridge groups", brno, argv[0], 1, 32);

  CLI_GET_INTEGER_RANGE ("Max Ring groups", ring_id, argv[1], 
                         NSM_G8032_MIN_RING_VAL, NSM_G8032_MAX_RING_VAL);

  ret = nsm_g8032_delete_ring (argv[0], ring_id, nm);
  if (ret != RESULT_OK)
    {
      nsm_g8032_print_range_error (cli, ret);
      return CLI_ERROR;
    }
  return CLI_SUCCESS;  
}

#else
CLI(nsm_bridge_ring_add,
    nsm_bridge_ring_br_add_cmd,
    "bridge <1-32> g8032 ring-id RINGID east-interface IFNAME"
    " west-interface IFNAME (instance <1-63> |)",
    NSM_BRIDGE_STR,
    NSM_BRIDGE_NAME_STR,
    "g8032 configuration commands",
    "Unique ID to identify the Ether Ring Protection",
    "The ID assigned to an Ethernet Ring Protection group <1-65535>",
    "east interface",
    CLI_IFNAME_STR,
   "west interface",
    CLI_IFNAME_STR,
   "Instance",
    "Id")
{
  int ret;
  s_int16_t instance    = 0;
  unsigned int g8032_ring_id;
  struct nsm_master *nm = cli->vr->proto;

  CLI_GET_INTEGER_RANGE("Max Protection rings", g8032_ring_id, argv[1],
                        NSM_G8032_MIN_RING_VAL, NSM_G8032_MAX_RING_VAL);
  CLI_GET_INTEGER_RANGE ("instance", instance, argv[5], NSM_BRIDGE_INSTANCE_MIN,
                         NSM_BRIDGE_INSTANCE_MAX);

  if (argc < 4)
    {
      ret = nsm_api_bridge_get_hw_instance (&instance);

      if (ret < 0)
        {
          cli_out(cli," %% Failed to get instance. Enter instance number\n");
          return CLI_ERROR;
        } 
    }

  ret = nsm_bridge_g8032_create_ring (argv[0], argv[2], argv[3], instance,
                                      g8032_ring_id, nm);
  if (ret != RESULT_OK)
    {
      nsm_g8032_print_range_error (cli, ret);
      return CLI_ERROR;
    }

  return CLI_SUCCESS;
}

CLI(nsm_bridge_g8032_ring_delete,
    nsm_bridge_g8032_ring_delete_cmd,
    "no bridge <1-32> g8032 ring-id RINGID ",
    CLI_NO_STR,
    NSM_BRIDGE_STR,
    NSM_BRIDGE_NAME_STR,
    "g8032 configuration commands",
    "Unique ID to identify the Ether Ring Protection",
    "The ID assigned to an Ethernet Ring Protection group <1-65535>")
{
  int brno;
  int g8032_ring_id;
  struct nsm_master *nm = cli->vr->proto;
  int ret;

  CLI_GET_INTEGER_RANGE ("Max Bridge groups", brno, argv[0], 1, 32); 

  CLI_GET_INTEGER_RANGE("Max Protection rings", g8032_ring_id, argv[1],
                        NSM_G8032_MIN_RING_VAL, NSM_G8032_MAX_RING_VAL);

  ret = nsm_g8032_delete_ring (argv[0], g8032_ring_id, nm);
  if (ret != RESULT_OK)
    {
      nsm_g8032_print_range_error (cli, ret);
      return CLI_ERROR;
    }

  return CLI_SUCCESS;
}
#endif 
#endif /* HAVE_G8032 */

/* NSM Bridge config write. */
int
nsm_bridge_config_write (struct cli *cli)
{
  int write = 0;
  struct nsm_bridge_master *master;
  struct nsm_bridge *bridge;
  struct nsm_master *nm = cli->vr->proto;
#ifdef HAVE_G8031
  struct g8031_protection_group    * pg = NULL;
  struct avl_node                  * pg_node = NULL;
  struct interface                 * ifp_w = NULL;
  struct interface                 *ifp_p = NULL;
#endif
#ifdef HAVE_G8032
  struct nsm_g8032_ring *ring = NULL;
  struct avl_node *node = NULL;
  struct interface *ifp_east = NULL;
  struct interface *ifp_west = NULL;
#endif

  master = nsm_bridge_get_master(nm);
  if (! master)
    return write;

  cli_out (cli, "!\n");
  write++;
  bridge = master->bridge_list;
  while (bridge)
    {
      if (bridge->is_default)
        {
#ifdef HAVE_PROVIDER_BRIDGE
          cli_out (cli, "spanning-tree mode %s %s\n",
                   nsm_bridge_type_to_str (bridge->type),
                   bridge->provider_edge ? "edge":"");
#else
          cli_out (cli, "spanning-tree mode %s\n",
                   nsm_bridge_type_to_str (bridge->type));
#endif /* HAVE_PROVIDER_BRIDGE */
        }
      else
        {
          switch (bridge->type)
            {
            case NSM_BRIDGE_TYPE_STP_VLANAWARE:
              cli_out (cli, "bridge %s protocol ieee vlan-bridge\n",
                       bridge->name);
              break;
            case NSM_BRIDGE_TYPE_STP:
              cli_out (cli, "bridge %s protocol ieee\n", bridge->name);
              break;
            case NSM_BRIDGE_TYPE_RSTP_VLANAWARE:
              cli_out (cli, "bridge %s protocol rstp vlan-bridge\n",
                       bridge->name);
              break;
            case NSM_BRIDGE_TYPE_RSTP:
              cli_out (cli, "bridge %s protocol rstp\n", bridge->name);
              break;
            case NSM_BRIDGE_TYPE_MSTP:
              cli_out (cli, "bridge %s protocol mstp\n", bridge->name);
              break;
#ifdef HAVE_RPVST_PLUS
            case NSM_BRIDGE_TYPE_RPVST_PLUS:
              cli_out (cli, "bridge %s protocol rpvst+\n", bridge->name);
              break;
#endif /* HAVE_RPVST_PLUS */
            case NSM_BRIDGE_TYPE_PROVIDER_RSTP:
#if defined (HAVE_I_BEB) || defined (HAVE_B_BEB)              
              if (bridge->backbone_edge != 1)
              {
#endif                
#ifdef HAVE_PROVIDER_BRIDGE
                cli_out (cli, "bridge %s protocol provider-rstp %s\n",
                         bridge->name,
                         bridge->provider_edge ? "edge":"");
#endif                
#if defined (HAVE_I_BEB) || defined (HAVE_B_BEB)                
              }
              else
              {
                cli_out (cli, "bridge beb mac %.04hx.%.04hx.%.04hx %s protocol "
                         "provider-rstp \n",
                          pal_ntoh16(((unsigned short *)bridge->bridge_mac)[0]),
                          pal_ntoh16(((unsigned short *)bridge->bridge_mac)[1]),
                          pal_ntoh16(((unsigned short *)bridge->bridge_mac)[2]),
                          bridge->name );
              }
#endif /* HAVE_I_BEB || HAVE_B_BEB */             
              break;
            case NSM_BRIDGE_TYPE_PROVIDER_MSTP:
#if defined (HAVE_I_BEB) || defined (HAVE_B_BEB)              
              if (bridge->backbone_edge != 1)
              { 
#endif                
#ifdef HAVE_PROVIDER_BRIDGE
                cli_out (cli, "bridge %s protocol provider-mstp %s\n",
                         bridge->name,
                         bridge->provider_edge ? "edge":"");
#endif /* HAVE_PROVIDER_BRIDGE */
#if defined (HAVE_I_BEB) || defined (HAVE_B_BEB)                
              }
              else
              {
                cli_out (cli, "bridge beb mac %.04hx.%.04hx.%.04hx %s protocol "
                         "provider-mstp \n",
                          pal_ntoh16(((unsigned short *)bridge->bridge_mac)[0]),
                          pal_ntoh16(((unsigned short *)bridge->bridge_mac)[1]),
                          pal_ntoh16(((unsigned short *)bridge->bridge_mac)[2]),
                          bridge->name );
              }
#endif /* HAVE_I_BEB || HAVE_B_BEB */
             break;
          default:
            break;
          }
        }

#ifdef HAVE_PROVIDER_BRIDGE
      if (bridge->uni_type_mode == NSM_UNI_TYPE_ENABLE)
        {
          cli_out (cli, "ethernet uni type enable bridge %s\n", bridge->name);
        }
#endif      
      if (bridge->learning == PAL_FALSE)
        cli_out (cli, "no bridge %s acquire\n", bridge->name);

      nsm_show_cp_fdb_entries (cli, bridge, nsm_show_running_display);
#ifdef HAVE_VLAN
      nsm_show_cp_fdb_entries (cli, bridge, nsm_pri_ovr_show_running_display);
#endif /* HAVE_VLAN */
#ifdef HAVE_G8031      
      if (bridge->eps_tree != NULL)
        AVL_TREE_LOOP (bridge->eps_tree, pg, pg_node)
          {
            ifp_w = ifg_lookup_by_index (&nm->zg->ifg, pg->ifindex_w);
            ifp_p = ifg_lookup_by_index (&nm->zg->ifg, pg->ifindex_p);

            cli_out (cli, "bridge %s g8031 eps-id %d working-port %s " 
                          "protection-port %s instance %d\n",
                           bridge->name, pg->eps_id,
                          ifp_w->name, ifp_p->name, pg->instance);
          }
#endif /* HAVE_G8031 */   
#ifdef HAVE_G8032
      if (bridge->raps_tree != NULL)
        AVL_TREE_LOOP (bridge->raps_tree, ring, node)
          {
            ifp_east = ifg_lookup_by_index (&nm->zg->ifg, ring->ifindex_e);
            ifp_west = ifg_lookup_by_index (&nm->zg->ifg, ring->ifindex_w);

            cli_out (cli, "bridge %s g8032 ring-id %d east-interface %s "
                          "west-interface %s instance %d\n",
                     bridge->name, ring->g8032_ring_id,
                     ifp_east->name, ifp_west->name, ring->instance);
          }
#endif  /*HAVE_G8032*/      
      write++;
      bridge = bridge->next;
    }
#ifdef HAVE_G8031  
#ifdef HAVE_B_BEB
  bridge = master->b_bridge;
  if (bridge)
    {
      if (bridge->eps_tree != NULL)
        AVL_TREE_LOOP (bridge->eps_tree, pg, pg_node)
          {
            ifp_w = ifg_lookup_by_index (&nm->zg->ifg, pg->ifindex_w);
            ifp_p = ifg_lookup_by_index (&nm->zg->ifg, pg->ifindex_p);

            cli_out (cli, "%s%s g8031 eps-id %d working-port %s protection-port %s\n",
                (bridge->name[0]=='b')?"" : "bridge ", bridge->name, pg->eps_id,
                ifp_w->name, ifp_p->name);
            write++;
          }
    }
#endif /* defined (HAVE_I_BEB) || defined (HAVE_B_BEB) */
#endif /* HAVE_G8031 */ 

#ifdef HAVE_G8032
 #ifdef HAVE_B_BEB
   bridge = master->b_bridge;
   if (bridge)
     {
       if (bridge->raps_tree != NULL)
         AVL_TREE_LOOP (bridge->raps_tree, ring, node)
           {
             ifp_east = ifg_lookup_by_index (&nm->zg->ifg, ring->ifindex_e);
             ifp_west = ifg_lookup_by_index (&nm->zg->ifg, ring->ifindex_w);

             cli_out (cli, "%s%s g8032 ring-id %d east-interface %s  west-interface %s\n",
                 (bridge->name[0]=='b')?"" : "bridge ", bridge->name, ring->g8032_ring_id,
                 ifp_east->name, ifp_west->name);
             write++;
           }
     }
 #endif /*  defined (HAVE_B_BEB) */
#endif /* HAVE_G8032 */
 
#ifdef HAVE_B_BEB
  bridge = master->b_bridge;
  if (bridge)
    {
      switch (bridge->type)
        {
        case NSM_BRIDGE_TYPE_BACKBONE_RSTP:
            cli_out (cli, "bridge beb mac %.04hx.%.04hx.%.04hx %s protocol "
                     "provider-rstp \n",
                     pal_ntoh16(((unsigned short *)bridge->bridge_mac)[0]),
                     pal_ntoh16(((unsigned short *)bridge->bridge_mac)[1]),
                     pal_ntoh16(((unsigned short *)bridge->bridge_mac)[2]),
                     bridge->name );

          break;
        case NSM_BRIDGE_TYPE_BACKBONE_MSTP:
            cli_out (cli, "bridge beb mac %.04hx.%.04hx.%.04hx %s protocol "
                    "provider-mstp \n",
                    pal_ntoh16(((unsigned short *)bridge->bridge_mac)[0]),
                    pal_ntoh16(((unsigned short *)bridge->bridge_mac)[1]),
                    pal_ntoh16(((unsigned short *)bridge->bridge_mac)[2]),
                    bridge->name );

          break;
        default:
          break;
        }
      write++;
    }
#endif /* HAVE_B_BEB */
#ifdef HAVE_VLAN_CLASS
  write += nsm_vlan_classifier_write(cli);
#endif /* HAVE_VLAN_CLASS */


#if defined HAVE_IGMP_SNOOP || defined HAVE_MLD_SNOOP
    if (master->l2mcast->nsm_l2_unknown_mcast == NSM_L2_UNKNOWN_MCAST_DISCARD)
          cli_out (cli,"l2 unknown mcast discard \n");
#endif /* HAVE_IGMP_SNOOP || HAVE_MLD_SNOOP */
    
  return write;
}

#ifdef HAVE_PROVIDER_BRIDGE

int
nsm_bridge_proto_process_config_write (struct cli *cli,
                                       struct nsm_bridge_port *br_port)
{
  int write;

  write = 0;

  if (br_port->vlan_port.mode == NSM_VLAN_PORT_MODE_CE)
    {

#if defined HAVE_STPD || defined HAVE_RSTPD || defined HAVE_MSTPD

      if (br_port->proto_process.stp_process != HAL_L2_PROTO_PEER)
        {
          if (br_port->proto_process.stp_process == HAL_L2_PROTO_DISCARD)
            cli_out (cli, " l2protocol stp discard\n");
          else
            {
              if (br_port->proto_process.stp_tun_vid)
                cli_out (cli, " l2protocol stp tunnel %d\n",
                         br_port->proto_process.stp_tun_vid);
              else
                cli_out (cli, " l2protocol stp tunnel\n");
            }

          write++;
        }
#endif /* HAVE_STPD || HAVE_RSTPD || HAVE_MSTPD */

#ifdef HAVE_GMRP

      if (br_port->proto_process.gmrp_process != HAL_L2_PROTO_TUNNEL)
        {
          if (br_port->proto_process.gmrp_process == HAL_L2_PROTO_DISCARD)
            cli_out (cli, " l2protocol gmrp discard\n");
          else
            cli_out (cli, " l2protocol gmrp peer\n");

          write++;
        }

#endif /* HAVE_GMRP */

#ifdef HAVE_MMRP

      if (br_port->proto_process.mmrp_process != HAL_L2_PROTO_TUNNEL)
        {
          if (br_port->proto_process.mmrp_process == HAL_L2_PROTO_DISCARD)
            cli_out (cli, " l2protocol mmrp discard\n");
          else
            cli_out (cli, " l2protocol mmrp peer\n");

          write++;
        }

#endif /* HAVE_MMRP */

#ifdef HAVE_GVRP

      if (br_port->proto_process.gvrp_process != HAL_L2_PROTO_DISCARD)
        {
          if (br_port->proto_process.gvrp_process == HAL_L2_PROTO_TUNNEL)
            {
              if (br_port->proto_process.gvrp_tun_vid)
                cli_out (cli, " l2protocol gvrp tunnel %d\n",
                         br_port->proto_process.gvrp_tun_vid);
              else
                cli_out (cli, " l2protocol gvrp tunnel\n");

              write++;
            }
        }

#endif /* HAVE_GVRP */

#ifdef HAVE_MVRP

      if (br_port->proto_process.mvrp_process != HAL_L2_PROTO_DISCARD)
        {
          if (br_port->proto_process.mvrp_process == HAL_L2_PROTO_TUNNEL)
            {
              cli_out (cli, " l2protocol mvrp tunnel %d\n",
                       br_port->proto_process.mvrp_tun_vid);
              write++;
            }
        }

#endif /* HAVE_MVRP */

#ifdef HAVE_LACPD

      if (br_port->proto_process.lacp_process != HAL_L2_PROTO_PEER)
        {
          if (br_port->proto_process.lacp_process == HAL_L2_PROTO_DISCARD)
            cli_out (cli, " l2protocol lacp discard\n");
          else
            {
              if (br_port->proto_process.lacp_tun_vid)
                cli_out (cli, " l2protocol lacp tunnel %d\n",
                         br_port->proto_process.lacp_tun_vid);
              else
                cli_out (cli, " l2protocol lacp tunnel\n");
            }

          write++;
        }

#endif /* HAVE_LACPD */

#ifdef HAVE_AUTHD

      if (br_port->proto_process.dot1x_process != HAL_L2_PROTO_PEER)
        {
          if (br_port->proto_process.dot1x_process == HAL_L2_PROTO_DISCARD)
            cli_out (cli, " l2protocol dot1x discard\n");
          else
            {
              if (br_port->proto_process.dot1x_tun_vid)
                cli_out (cli, " l2protocol dot1x tunnel %d\n",
                         br_port->proto_process.dot1x_tun_vid);
              else
                cli_out (cli, " l2protocol dot1x tunnel\n");
            }
        }

#endif /* HAVE_AUTHD */
   }
  else if (br_port->vlan_port.mode == NSM_VLAN_PORT_MODE_CN)
   {
#if defined HAVE_STPD || defined HAVE_RSTPD || defined HAVE_MSTPD
      if (br_port->proto_process.stp_process == HAL_L2_PROTO_TUNNEL)
        {
           if (br_port->proto_process.stp_tun_vid)
             {
               cli_out (cli, " l2protocol stp tunnel %d\n",
                        br_port->proto_process.stp_tun_vid);

               write++;
             }
        }
      else
        {
          if (br_port->proto_process.stp_process == HAL_L2_PROTO_DISCARD)
             {
               cli_out (cli, " l2protocol stp discard\n");
               write++;
             }
        }

#endif /* HAVE_STPD || HAVE_RSTPD || HAVE_MSTPD */

#ifdef HAVE_GMRP

      if (br_port->proto_process.gmrp_process != HAL_L2_PROTO_TUNNEL)
        {
          if (br_port->proto_process.gmrp_process == HAL_L2_PROTO_DISCARD)
            cli_out (cli, " l2protocol gmrp discard\n");
          else
            cli_out (cli, " l2protocol gmrp peer\n");

          write++;
        }

#endif /* HAVE_GMRP */

#ifdef HAVE_MMRP

      if (br_port->proto_process.mmrp_process != HAL_L2_PROTO_TUNNEL)
        {
          if (br_port->proto_process.mmrp_process == HAL_L2_PROTO_DISCARD)
            cli_out (cli, " l2protocol mmrp discard\n");
          else
            cli_out (cli, " l2protocol mmrp peer\n");

          write++;
        }

#endif /* HAVE_MMRP */

#ifdef HAVE_GVRP

      if (br_port->proto_process.gvrp_process == HAL_L2_PROTO_TUNNEL)
        {
           if (br_port->proto_process.gvrp_tun_vid)
             {
               cli_out (cli, " l2protocol gvrp tunnel %d\n",
                        br_port->proto_process.gvrp_tun_vid);
               write++;
             }
        }
      else
        {
          if (br_port->proto_process.gvrp_process == HAL_L2_PROTO_DISCARD)
             {
               cli_out (cli, " l2protocol gvrp discard\n");
               write++;
             }
        }

#endif /* HAVE_GVRP */

#ifdef HAVE_MVRP

      if (br_port->proto_process.mvrp_process != HAL_L2_PROTO_DISCARD)
        {
          if (br_port->proto_process.mvrp_process == HAL_L2_PROTO_TUNNEL)
            {
              cli_out (cli, " l2protocol mvrp tunnel %d\n",
                       br_port->proto_process.mvrp_tun_vid);
              write++;
            }
        }

#endif /* HAVE_MVRP */

#ifdef HAVE_LACPD

      if (br_port->proto_process.lacp_process != HAL_L2_PROTO_PEER)
        {
          if (br_port->proto_process.lacp_process == HAL_L2_PROTO_DISCARD)
            cli_out (cli, " l2protocol lacp discard\n");
          else
            {
              if (br_port->proto_process.lacp_tun_vid)
                cli_out (cli, " l2protocol lacp tunnel %d\n",
                         br_port->proto_process.lacp_tun_vid);
              else
                cli_out (cli, " l2protocol lacp tunnel\n");
            }

          write++;
        }

#endif /* HAVE_LACPD */

#ifdef HAVE_AUTHD

      if (br_port->proto_process.dot1x_process != HAL_L2_PROTO_PEER)
        {
          if (br_port->proto_process.dot1x_process == HAL_L2_PROTO_DISCARD)
            cli_out (cli, " l2protocol dot1x discard\n");
          else
            {
              if (br_port->proto_process.dot1x_tun_vid)
                cli_out (cli, " l2protocol dot1x tunnel %d\n",
                         br_port->proto_process.dot1x_tun_vid);
              else
                cli_out (cli, " l2protocol dot1x tunnel\n");
            }
        }

#endif /* HAVE_AUTHD */
   }
#ifdef HAVE_I_BEB
   else if (br_port->vlan_port.mode == NSM_VLAN_PORT_MODE_CNP)
    {
#if defined HAVE_STPD || defined HAVE_RSTPD || defined HAVE_MSTPD
      if (br_port->proto_process.stp_process == HAL_L2_PROTO_TUNNEL)
        {
           if (br_port->proto_process.stp_tun_vid)
             {
               cli_out (cli, " l2protocol stp tunnel\n");
               write++;
             }
        }
      else
        {
          if (br_port->proto_process.stp_process == HAL_L2_PROTO_DISCARD)
             {
               cli_out (cli, " l2protocol stp discard\n");
               write++;
             }
        }
#endif /* defined HAVE_STPD || defined HAVE_RSTPD || defined HAVE_MSTPD */
    }
#endif /* HAVE_I_BEB */
   
  return write;

}

#endif /* HAVE_PROVIDER_BRIDGE */

/* NSM Bridge interface configuration write. */
int
nsm_bridge_if_config_write (struct cli *cli, struct interface *ifp)
{
//  struct nsm_master *nm = cli->vr->proto;
  struct nsm_bridge_port *br_port;
#ifdef HAVE_PROVIDER_BRIDGE
  struct nsm_vlan_port *vlan_port;
  struct nsm_band_width_profile *bw_profile = NULL;
  struct nsm_uni_evc_bw_profile *evc_profile = NULL;
  struct nsm_bw_profile_parameters *ingress_evc = NULL;
  struct nsm_bw_profile_parameters *egress_evc = NULL;
  struct nsm_bw_profile_parameters *ingress_cos = NULL;
  struct nsm_bw_profile_parameters *egress_cos = NULL;
  struct listnode *evc_node = NULL;
  u_int8_t i = PAL_FALSE;
  u_int8_t j = PAL_FALSE;
  u_int8_t temp_cosid = PAL_FALSE;
  u_int8_t first = PAL_FALSE;
  u_int8_t evc_config_flag = PAL_FALSE;
  u_int8_t temp_cos[2];
  u_int8_t cos_string[NSM_MAX_COS_PER_EVC][16];
  s_int8_t temp_cos_val[NSM_MAX_COS_PER_EVC];
#endif /* HAVE_PROVIDER_BRIDGE */
  struct nsm_if *zif;
  int write = 0;

  zif = (struct nsm_if *)ifp->info;
  if (! zif)
    return write;

  if (zif->type == NSM_IF_TYPE_L2)
    {
      if(ifp->hw_type != IF_TYPE_PBB_LOGICAL)
        cli_out (cli, " switchport\n");

      write++;

      if (zif->bridge && zif->switchport)
        {
          br_port = zif->switchport;

            if(ifp->hw_type != IF_TYPE_PBB_LOGICAL)
              {
                if (!zif->bridge->is_default)
                  {
                    if (br_port->spanning_tree_disable == PAL_TRUE) 
                      cli_out (cli, " bridge-group %s spanning-tree disable\n",
                               zif->bridge->name);
                    else
                      cli_out (cli, " bridge-group %s\n", zif->bridge->name);
                    write++;
                  }
                else
                  {
                    /* check the default bridge spanning_tree mode */
                    if (br_port->spanning_tree_disable == PAL_TRUE)
                       cli_out (cli, " spanning-tree disable\n");
                    write++;   
                  }
              }

#ifdef HAVE_VLAN
          write += nsm_vlan_if_config_write (cli, ifp); 
#ifdef HAVE_PVLAN
          write += nsm_pvlan_if_config_write (cli, ifp);
#endif /* HAVE_PVLAN */

#ifdef HAVE_PROVIDER_BRIDGE
          write += nsm_bridge_proto_process_config_write (cli, br_port);

          vlan_port = &br_port->vlan_port;

          if (vlan_port && vlan_port->mode == NSM_VLAN_PORT_MODE_CE)
            {
              if (br_port->uni_name [0] != 0)
                {
                  cli_out (cli, " ethernet uni id %s\n", br_port->uni_name);
                  write++;
                }

              if (br_port->uni_ser_attr == NSM_UNI_MUX)
                {
                  cli_out (cli, " ethernet uni multiplex\n");
                  write++;
                }
              else if (br_port->uni_ser_attr == NSM_UNI_BNDL)
                {
                  cli_out (cli, " ethernet uni bundle\n");
                  write++;
                }
              else if (br_port->uni_ser_attr == NSM_UNI_AIO_BNDL)
                {
                  cli_out (cli, " ethernet uni all-to-one\n");
                  write++;
                }
            }
#endif /* HAVE_PROVIDER_BRIDGE */
          if (br_port->uni_type_status.uni_type_mode ==
              NSM_UNI_TYPE_ENABLE)
            {
              cli_out (cli, " ethernet uni type enable\n");
              write++;
            }

#ifdef HAVE_PROVIDER_BRIDGE
          if (vlan_port && vlan_port->mode == NSM_VLAN_PORT_MODE_CE)
            {
              if ((br_port->uni_max_evcs != PAL_FALSE)
                  && (br_port->uni_max_evcs != NSM_VLAN_MAX))
                {
                  cli_out (cli, " ethernet uni max-evc %d\n",
                      br_port->uni_max_evcs);
                  write++;
                }

              if (zif->nsm_bw_profile)
                bw_profile = zif->nsm_bw_profile;

              if (bw_profile)
                {
              /* Display Ingress Bandwidth Profile per UNI in show run*/
              if (bw_profile->ingress_uni_bw_profile)
                {
                  if (bw_profile->ingress_uni_bw_profile->comm_info_rate
                      != PAL_FALSE)
                    {
                      cli_out (cli, " ingress-policing uni cir %d\n",
                         bw_profile->ingress_uni_bw_profile->comm_info_rate);
                      write++;
                    }
                  if (bw_profile->ingress_uni_bw_profile->comm_burst_size
                      != PAL_FALSE)
                    {
                      cli_out (cli, " ingress-policing uni cbs %d\n",
                          bw_profile->ingress_uni_bw_profile->comm_burst_size);
                      write++;
                    }
                  if (bw_profile->ingress_uni_bw_profile->excess_info_rate
                      != PAL_FALSE)
                    {
                      cli_out (cli, " ingress-policing uni eir %d\n",
                         bw_profile->ingress_uni_bw_profile->excess_info_rate);
                      write++;
                    }
                  if (bw_profile->ingress_uni_bw_profile->excess_burst_size
                      != PAL_FALSE)
                    {
                      cli_out (cli, " ingress-policing uni ebs %d\n",
                        bw_profile->ingress_uni_bw_profile->excess_burst_size);
                      write++;
                    }
                  if (bw_profile->ingress_uni_bw_profile->coupling_flag
                      == PAL_TRUE)
                    {
                      cli_out (cli, " ingress-policing uni coupling-flag "
                          "enable\n");
                      write++;
                    }
                  if (bw_profile->ingress_uni_bw_profile->color_mode ==
                      PAL_TRUE)
                    {
                      cli_out (cli, " ingress-policing uni color-mode "
                          "color-aware\n");
                      write++;

                    }

                  if (bw_profile->ingress_uni_bw_profile->active == PAL_TRUE)
                    {
                      cli_out (cli, " ingress-policing uni active\n");
                      write++;
                    }
                }/*if (bw_profile->ingress_uni_bw_profile)*/

              /*Display Egress Bandiwdt Profile Per uni */
              if (bw_profile->egress_uni_bw_profile)
                {
                  if (bw_profile->egress_uni_bw_profile->comm_info_rate
                      != PAL_FALSE)
                    {
                      cli_out (cli, " egress-shaping uni cir %d\n",
                          bw_profile->egress_uni_bw_profile->comm_info_rate);
                      write++;
                    }
                  if (bw_profile->egress_uni_bw_profile->comm_burst_size
                      != PAL_FALSE)
                    {
                      cli_out (cli, " egress-shaping uni cbs %d\n",
                          bw_profile->egress_uni_bw_profile->comm_burst_size);
                      write++;
                    }
                  if (bw_profile->egress_uni_bw_profile->excess_info_rate
                      != PAL_FALSE)
                    {
                      cli_out (cli, " egress-shaping uni eir %d\n",
                          bw_profile->egress_uni_bw_profile->excess_info_rate);
                      write++;
                    }
                  if (bw_profile->egress_uni_bw_profile->excess_burst_size
                      != PAL_FALSE)
                    {
                      cli_out (cli, " egress-shaping uni ebs %d\n",
                        bw_profile->egress_uni_bw_profile->excess_burst_size);
                      write++;
                    }
                  if (bw_profile->egress_uni_bw_profile->coupling_flag
                      == PAL_TRUE)
                    {
                      cli_out (cli, " egress-shaping uni coupling-flag "
                          "enable\n");
                      write++;
                    }
                  if (bw_profile->egress_uni_bw_profile->color_mode ==
                      PAL_TRUE)
                    {
                      cli_out (cli, " egress-shaping uni color-mode "
                          "color-aware\n");
                      write++;

                    }

                  if (bw_profile->egress_uni_bw_profile->active == PAL_TRUE)
                    {
                      cli_out (cli, " egress-shaping uni active\n");
                      write++;
                    }
                }/*if (bw_profile->egress_uni_bw_profile)*/

              if (bw_profile->uni_evc_bw_profile_list)
                {
                  LIST_LOOP (bw_profile->uni_evc_bw_profile_list, evc_profile,
                      evc_node)
                    {
                      if ((evc_profile->evc_id) &&
                          (evc_profile->instance_id != PAL_FALSE))
                          {
                            cli_out (cli," service instance %d evc-id %s\n",
                                evc_profile->instance_id, evc_profile->evc_id);
                            
                            evc_config_flag = PAL_TRUE; 
                            write++;
                            
                            if (evc_profile->ingress_evc_bw_profile)
                              {
                                ingress_evc =
                                  evc_profile->ingress_evc_bw_profile;

                                if (ingress_evc->comm_info_rate != PAL_FALSE)
                                  {
                                    cli_out (cli, "  ingress-policing cir %d\n"
                                        , ingress_evc->comm_info_rate);
                                    write++;
                                  }
                                if (ingress_evc->comm_burst_size != PAL_FALSE)
                                  {
                                    cli_out (cli, "  ingress-policing cbs %d\n"
                                        , ingress_evc->comm_burst_size);
                                    write++;
                                  }
                                if (ingress_evc->excess_info_rate != PAL_FALSE)
                                  {
                                    cli_out (cli, "  ingress-policing eir %d\n"
                                        , ingress_evc->excess_info_rate);
                                    write++;
                                  }
                                if (ingress_evc->excess_burst_size != PAL_FALSE)
                                  {
                                    cli_out (cli, "  ingress-policing ebs %d\n"
                                        , ingress_evc->excess_burst_size);
                                    write++;
                                  }
                                if (ingress_evc->coupling_flag
                                    == PAL_TRUE)
                                  {
                                    cli_out (cli, "  ingress-policing "
                                        "coupling-flag enable\n");
                                    write++; 
                                  }
                                if (ingress_evc->color_mode ==
                                    PAL_TRUE)
                                  {
                                    cli_out (cli, "  ingress-policing "
                                        "color-mode color-aware\n");
                                    write++; 
                                  }

                                if (ingress_evc->active == PAL_TRUE)
                                  {
                                    cli_out (cli, "  ingress-policing active"
                                        "\n");
                                    write++;

                                  }
                              }/*if (evc_profile->ingress_evc_bw_profile)*/
                            else if(evc_profile->ingress_bw_profile_type == 
                                NSM_EVC_COS_BW_PROFILE)
                              {
                                for (i = 0; i < NSM_MAX_COS_PER_EVC; i++)
                                  {
                                    temp_cos_val[i] = -1;
                                    if (evc_profile->num_of_ingress_cos_id [i]
                                        == PAL_TRUE)
                                      {
                                        temp_cosid = i+1;
                                        for (j = 0; j < NSM_MAX_COS_PER_EVC;
                                            j++)
                                          {
                                            if (evc_profile->ingress_CoS_id[j]
                                                == temp_cosid)
                                              {
                                                pal_sprintf(temp_cos,"%d", j);
                                                
                                                if (first == PAL_FALSE)
                                                  {
                                                    pal_strcpy (cos_string[i],
                                                        temp_cos);
                                                    temp_cos_val[i] = j;
                                                    first = PAL_TRUE;
                                                  }
                                                else
                                                  {
                                                    pal_strcat (cos_string[i],
                                                        " ");
                                                    pal_strcat (cos_string[i],
                                                        temp_cos);
                                                  }
                                              }
                                          }
                                        first = PAL_FALSE;
                                      }
                                  }
                                for (i = 0; i < NSM_MAX_COS_PER_EVC; i++)
                                  {
                                    if ((temp_cos_val[i] >= 0) &&
                                      (temp_cos_val[i] < NSM_MAX_COS_PER_EVC))
                                      {
                                        j = temp_cos_val[i];
                                        if (evc_profile->
                                            ingress_evc_cos_bw_profile[j])
                                          {
                                            ingress_cos = evc_profile->
                                              ingress_evc_cos_bw_profile[j];

                                            if (ingress_cos->comm_info_rate !=
                                                PAL_FALSE)
                                              {
                                                cli_out (cli,
                                                    "  ingress-policing evc "
                                                    "cos %s cir %d\n",
                                                  cos_string[i],
                                                  ingress_cos->comm_info_rate);
                                                write++;
                                              }
                                            if (ingress_cos->comm_burst_size !=
                                                PAL_FALSE)
                                              {
                                                cli_out (cli,
                                                    "  ingress-policing evc "
                                                    "cos %s cbs %d\n",
                                                  cos_string[i],
                                                 ingress_cos->comm_burst_size);
                                                write++;
                                              }
                                            if (ingress_cos->excess_info_rate
                                                != PAL_FALSE)
                                              {
                                                cli_out (cli,
                                                    "  ingress-policing evc cos %s eir %d\n",
                                                cos_string[i], ingress_cos->
                                                excess_info_rate);
                                                write++;
                                              }
                                            if (ingress_cos->excess_burst_size
                                                != PAL_FALSE)
                                              {
                                                cli_out (cli,
                                                    "  ingress-policing evc "
                                                    "cos %s ebs %d\n",
                                                cos_string[i], ingress_cos->
                                                excess_burst_size);
                                                write++;
                                              }
                                            if (ingress_cos->coupling_flag
                                                == PAL_TRUE)
                                              {
                                                cli_out (cli,
                                                    "  ingress-policing evc "
                                                    "cos %s "
                                                   "coupling-flag enable\n",
                                                   cos_string[i]);
                                               write++;
                                              }
                                            if (ingress_cos->color_mode ==
                                                PAL_TRUE)
                                              {
                                                cli_out (cli,
                                                    "  ingress-policing evc "
                                                    "cos %s "
                                                   "color-mode color-aware\n",
                                                   cos_string[i]);
                                               write++;
                                              }
                                           if (ingress_cos->active ==
                                                PAL_TRUE)
                                             {
                                               cli_out (cli,
                                                   "  ingress-policing evc"
                                                  " cos %s active\n",
                                                cos_string[i]);
                                               write++;
                                             }

                                          }/* if (evc_profile->
                                              ingress_evc_cos_bw_profile[i])*/
                                        
                                      }/*if (temp_cos_val[i] == PAL_TRUE)*/
                                  }/*for (i = 0; i < NSM_MAX_COS_PER_EVC;i++)*/

                              }/*else if(evc_profile->bw_profile_type == 
                                NSM_EVC_COS_BW_PROFILE)*/

                            if (evc_profile->egress_evc_bw_profile)
                              {
                                egress_evc =
                                  evc_profile->egress_evc_bw_profile;

                                if (egress_evc->comm_info_rate != PAL_FALSE)
                                  {
                                    cli_out (cli, "  egress-shaping cir %d\n",
                                        egress_evc->comm_info_rate);
                                    write++;
                                  }
                                if (egress_evc->comm_burst_size != PAL_FALSE)
                                  {
                                    cli_out (cli, "  egress-shaping cbs %d\n",
                                        egress_evc->comm_burst_size);
                                    write++;
                                  }
                                if (egress_evc->excess_info_rate != PAL_FALSE)
                                  {
                                    cli_out (cli, "  egress-shaping eir %d\n",
                                        egress_evc->excess_info_rate);
                                    write++;
                                  }
                                if (egress_evc->excess_burst_size != PAL_FALSE)
                                  {
                                    cli_out (cli, "  egress-shaping ebs %d\n",
                                        egress_evc->excess_burst_size);
                                    write++;
                                  }
                                if (egress_evc->coupling_flag
                                    == PAL_TRUE)
                                  {
                                    cli_out (cli, "  egress-shaping "
                                        "coupling-flag "
                                        "enable\n");
                                  }
                                if (egress_evc->color_mode ==
                                    PAL_TRUE)
                                  {
                                    cli_out (cli, "  egress-shaping color-mode "
                                        "color-aware\n");

                                  }

                                if (egress_evc->active == PAL_TRUE)
                                  {
                                    cli_out (cli, "  egress-shaping active\n");
                                    write++;
                                  }
                              }/*if (evc_profile->egress_evc_bw_profile)*/
                            else if(evc_profile->egress_bw_profile_type == 
                                NSM_EVC_COS_BW_PROFILE)
                              {
                                for (i = 0; i < NSM_MAX_COS_PER_EVC; i++)
                                  {
                                    temp_cos_val[i] = -1;
                                    if (evc_profile->num_of_egress_cos_id [i]
                                        == PAL_TRUE)
                                      {
                                        temp_cosid = i+1;
                                        for (j = 0; j < NSM_MAX_COS_PER_EVC;
                                            j++)
                                          {
                                            if (evc_profile->egress_CoS_id[j]
                                                == temp_cosid)
                                              {
                                                pal_sprintf(temp_cos,"%d", j);
                                                
                                                if (first == PAL_FALSE)
                                                  {
                                                    pal_strcpy (cos_string[i],
                                                        temp_cos);
                                                    temp_cos_val[i] = j;
                                                    first = PAL_TRUE;
                                                  }
                                                else
                                                  {
                                                    pal_strcat (cos_string[i],
                                                        " ");
                                                    pal_strcat (cos_string[i],
                                                        temp_cos);
                                                  }
                                              }/*if(evc_profile->egress_CoS_id
                                                 [j] == temp_cosid)*/
                                          }/*for(j=0;j< NSM_MAX_COS_PER_EVC;
                                             j++)*/
                                        first = PAL_FALSE;
                                      }/*if (evc_profile->num_of_egress_cos_id [i]
                                         == PAL_TRUE)*/
                                  }/*for(i = 0; i<NSM_MAX_COS_PER_EVC; i++)*/
                                for (i = 0; i < NSM_MAX_COS_PER_EVC; i++)
                                  {
                                    if (((temp_cos_val[i] >= 0) &&
                                      (temp_cos_val[i] < NSM_MAX_COS_PER_EVC)))
                                      {
                                        j = temp_cos_val[i];
                                        
                                        if (evc_profile->egress_evc_cos_bw_profile[j])
                                          {
                                            egress_cos = evc_profile->
                                              egress_evc_cos_bw_profile[j];

                                            if (egress_cos->comm_info_rate !=
                                                PAL_FALSE)
                                              {
                                                cli_out (cli,
                                                    "  egress-shaping evc cos "
                                                    "%s cir %d\n",
                                                  cos_string[i],
                                                  egress_cos->comm_info_rate);
                                                write++;
                                              }
                                            if (egress_cos->comm_burst_size !=
                                                PAL_FALSE)
                                              {
                                                cli_out (cli,
                                                    "  egress-shaping evc cos "
                                                    "%s cbs %d\n",
                                                  cos_string[i],
                                                  egress_cos->comm_burst_size);
                                                write++;
                                              }
                                            if (egress_cos->excess_info_rate
                                                != PAL_FALSE)
                                              {
                                                cli_out (cli,
                                                    "  egress-shaping evc cos "
                                                    "%s eir %d\n",
                                                cos_string[i], egress_cos->
                                                excess_info_rate);
                                                write++;
                                              }
                                            if (egress_cos->excess_burst_size
                                                != PAL_FALSE)
                                              {
                                                cli_out (cli,
                                                    "  egress-shaping evc cos "
                                                    "%s ebs %d\n",
                                                cos_string[i], egress_cos->
                                                excess_burst_size);
                                                write++;
                                              }
                                            if (egress_cos->coupling_flag
                                                == PAL_TRUE)
                                              {
                                                cli_out (cli,
                                                    "  egress-shaping evc cos "
                                                    "%s "
                                                    "coupling-flag enable\n", 
                                                    cos_string[i]);
                                                write++;
                                              }
                                            if (egress_cos->color_mode ==
                                                PAL_TRUE)
                                              {
                                                cli_out (cli,
                                                    "  egress-shaping evc cos "
                                                    "%s "
                                                    "color-mode color-aware\n",
                                                    cos_string[i]);
                                                write++;

                                              }

                                            if (egress_cos->active ==
                                                PAL_TRUE)
                                             {
                                               cli_out (cli,
                                                   "  egress-shaping evc cos "
                                                   "%s active\n",
                                                cos_string[i]);
                                               write++;
                                             }

                                          }/* if (evc_profile->
                                              ingress_evc_cos_bw_profile[i])*/
                                        
                                      }/*if (temp_cos_val[i] == PAL_TRUE)*/
                                  }/*for (i = 0; i < NSM_MAX_COS_PER_EVC;i++)*/

                              }/*else if(evc_profile->bw_profile_type == 
                                NSM_EVC_COS_BW_PROFILE)*/

                          }/* if ((evc_profile->evc_id) &&
                          (evc_profile->instance_id != PAL_FALSE))*/

                    }/* LIST_LOOP (bw_profile->uni_evc_bw_profile_list,
                        evc_profile, evc_node)*/
                  if (evc_config_flag == PAL_TRUE)
                    {
                      cli_out (cli, "  exit-service-instance-mode\n");
                      cli_out (cli, " !\n");
                      write++;
                    }
                }/* if (bw_profile->uni_evc_bw_profile_list)*/
                } /* if (bw_profile)*/
            }/* if (vlan_port && vlan_port->mode == NSM_VLAN_PORT_MODE_CE)*/
          
#endif /* HAVE_PROVIDER_BRIDGE */

          

#endif /* HAVE_VLAN */
#if defined (HAVE_I_BEB) || defined(HAVE_B_BEB)
  write += nsm_pbb_if_config_write (cli, ifp);
#endif /* HAVE_I_BEB || HAVE_B_BEB */

#if defined (HAVE_I_BEB) && defined(HAVE_B_BEB) && defined(HAVE_PBB_TE)
  if (pal_strncmp (ifp->name, "cbp", 3) == 0)
    write += nsm_pbb_te_if_config_write (cli, ifp);
#endif /* (HAVE_I_BEB) && (HAVE_B_BEB) && (HAVE_PBB_TE) */
        }

#ifdef HAVE_VLAN_STACK
      write += nsm_vlan_stack_write (cli, ifp);
#endif /* HAVE_VLAN_STACK */
      write += flow_control_write (cli, ifp->ifindex);
#ifdef HAVE_RATE_LIMIT
      write += nsm_ratelimit_write(cli, ifp->ifindex);
#endif /* HAVE_RATE_LIMIT */
#ifdef HAVE_VLAN_CLASS
      write += nsm_vlan_class_write(cli, ifp);
#endif /* HAVE_VLAN_CLASS */
#ifdef HAVE_DCB
     write += nsm_dcb_if_config_write (cli, ifp->name);
#endif /* HAVE_DCB */
    }

  return write;
}

/* CLI Initialization. */
int
nsm_bridge_cli_init (struct lib_globals *zg)
{
  struct cli_tree *ctree;
#if (! defined HAVE_MCAST_IPV4 && defined HAVE_IGMP_SNOOP)            \
    || (! defined HAVE_MCAST_IPV6 && defined HAVE_MLD_SNOOP)
  s_int32_t ret;
#endif /*
        * (! HAVE_MCAST_IPV4 && HAVE_IGMP_SNOOP)
        * || (! HAVE_MCAST_IPV6 && HAVE_MLD_SNOOP)
        */

  ctree = zg->ctree;

#if ! defined HAVE_MCAST_IPV4 && defined HAVE_IGMP_SNOOP
  ret = igmp_cli_init (zg);

  if (ret < 0)
    return NSM_FAILURE;
#endif /* ! HAVE_MCAST_IPV4 && HAVE_IGMP_SNOOP */

#if ((! defined HAVE_MCAST_IPV6) && (defined HAVE_MLD_SNOOP))
  ret = mld_cli_init (zg);

  if (ret < 0)
    return NSM_FAILURE;
#endif /* ! HAVE_MCAST_IPV6 && HAVE_MLD_SNOOP */

  cli_install_config (ctree, BRIDGE_MODE, nsm_bridge_config_write);
  cli_install_default (ctree, BRIDGE_MODE);

  cli_install (ctree, EXEC_MODE, &nsm_show_bridge_cmd);

  cli_install (ctree, EXEC_MODE, &nsm_bridge_clear_fdb_cmd);
  cli_install (ctree, EXEC_MODE, &nsm_bridge_clear_fdb_vlan_port_cmd);
  cli_install (ctree, EXEC_MODE, &nsm_bridge_clear_dynamic_fdb_bridge_cmd);
  cli_install (ctree, EXEC_MODE, &nsm_bridge_clear_dynamic_fdb_cmd);
  cli_install (ctree, EXEC_MODE, &nsm_bridge_clear_br_fdb_cmd);
  cli_install (ctree, EXEC_MODE, &nsm_bridge_clear_br_fdb_vlan_port_cmd);
  cli_install (ctree, EXEC_MODE, &nsm_bridge_clear_dynamic_br_fdb_bridge_cmd);
  cli_install (ctree, EXEC_MODE, &nsm_bridge_clear_dynamic_br_fdb_cmd);
#ifdef HAVE_PROVIDER_BRIDGE
  /* CLI to display uni type */
  cli_install (ctree, EXEC_MODE, &nsm_show_ethernet_uni_type_cmd);
  cli_install (ctree, EXEC_MODE, &nsm_bridge_clear_dynamic_br_fdb_cmd);
  cli_install (ctree, EXEC_MODE, &nsm_bridge_clear_fdb_def_pro_edge_vlan_cmd);
  cli_install (ctree, EXEC_MODE, &nsm_bridge_protocol_process_show_cmd);
  /* CLI to display mac evc per uni */
  cli_install (ctree, EXEC_MODE, &nsm_show_uni_max_evc_cmd);

  /*CLI to display BW parameters per UNI */ 
  cli_install (ctree, EXEC_MODE, &nsm_show_bw_profile_per_uni_cmd); 
  /* CLI to display BW parameters per EVC */
  cli_install (ctree, EXEC_MODE, &nsm_show_bw_profile_per_evc_cmd);

#endif /* HAVE_PROVIDER_BRIDGE */

  /* CONFIG mode commands. */
  cli_install (ctree, CONFIG_MODE, &bridge_acquire_cmd);
  cli_install (ctree, CONFIG_MODE, &no_bridge_acquire_cmd);
  cli_install (ctree, CONFIG_MODE, &spanning_acquire_cmd);
  cli_install (ctree, CONFIG_MODE, &no_spanning_acquire_cmd);
#ifdef HAVE_STPD
#ifdef HAVE_VLAN
  cli_install (ctree, CONFIG_MODE, &nsm_bridge_protocol_ieee_vlan_cmd);
#else
  cli_install (ctree, CONFIG_MODE, &nsm_bridge_protocol_ieee_cmd);
#endif /* HAVE_VLAN */
#endif /* HAVE_STPD */

#ifdef HAVE_RSTPD
#ifdef HAVE_VLAN
  cli_install (ctree, CONFIG_MODE, &nsm_bridge_protocol_rstp_vlan_cmd);
#else
  cli_install (ctree, CONFIG_MODE, &nsm_bridge_protocol_rstp_cmd);
#endif /* HAVE_VLAN */
#endif /* HAVE_RSTPD */

#if defined(HAVE_MSTPD) && defined(HAVE_VLAN)
  cli_install (ctree, CONFIG_MODE, &nsm_bridge_protocol_mstp_cmd);
#endif /* HAVE_MSTPD */

#if defined(HAVE_RPVST_PLUS) && defined(HAVE_VLAN) && defined(HAVE_MSTPD)
  cli_install (ctree, CONFIG_MODE, &nsm_bridge_protocol_rpvst_plus_cmd);
#endif /* HAVE_RPVST_PLUS && HAVE_VLAN && HAVE_MSTPD*/

#ifdef HAVE_PROVIDER_BRIDGE
  cli_install (ctree, CONFIG_MODE, &nsm_bridge_protocol_pro_rstp_cmd);
  cli_install (ctree, CONFIG_MODE, &nsm_bridge_protocol_pro_mstp_cmd);
#endif /* HAVE_PROVIDER_BRIDGE */

#ifdef HAVE_I_BEB
  cli_install (ctree, CONFIG_MODE, &nsm_beb_bridge_protocol_pro_rstp_cmd);
  cli_install (ctree, CONFIG_MODE, &nsm_beb_bridge_protocol_pro_mstp_cmd);
#endif /* HAVE_I_BEB */

#ifdef HAVE_B_BEB
  cli_install (ctree, CONFIG_MODE, &nsm_beb_bridge_protocol_backbone_rstp_cmd);
  cli_install (ctree, CONFIG_MODE, &nsm_beb_bridge_protocol_backbone_mstp_cmd);
#endif /* HAVE_B_BEB */

#if 0
  cli_install (ctree, CONFIG_MODE, &bridge_decouple_vlan_cmd);
  cli_install (ctree, CONFIG_MODE, &no_bridge_decouple_vlan_cmd);
#endif /* if 0 */
  cli_install (ctree, CONFIG_MODE, &no_nsm_bridge_protocol_cmd);
 
#ifdef HAVE_DEFAULT_BRIDGE 
  cli_install (ctree, CONFIG_MODE, &spanning_tree_mode_cmd);
#endif /* HAVE_DEFAULT_BRIDGE */

  /* Bridge mac address table commands */
  cli_install (ctree, CONFIG_MODE, &bridge_add_static_forward_cmd);
  cli_install (ctree, CONFIG_MODE, &bridge_add_static_filter_cmd);
  cli_install (ctree, CONFIG_MODE, &no_bridge_static_cmd);
  cli_install (ctree, CONFIG_MODE, &no_bridge_static_filter_cmd);
  cli_install (ctree, CONFIG_MODE, &spanning_add_static_cmd);
  cli_install (ctree, CONFIG_MODE, &no_spanning_static_cmd);
#ifdef HAVE_VLAN
  cli_install (ctree, CONFIG_MODE, &mac_prio_override_cmd);
  cli_install (ctree, CONFIG_MODE, &no_mac_prio_override_cmd);
  cli_install (ctree, CONFIG_MODE, &vlan_add_static_forward_cmd);
  cli_install (ctree, CONFIG_MODE, &vlan_add_static_filter_cmd);
  cli_install (ctree, CONFIG_MODE, &no_vlan_static_cmd);
  cli_install (ctree, CONFIG_MODE, &no_vlan_static_filter_cmd);
#ifdef HAVE_PROVIDER_BRIDGE
  cli_install (ctree, CONFIG_MODE, &svlan_add_static_forward_cmd);
  cli_install (ctree, CONFIG_MODE, &svlan_add_static_filter_cmd);
  cli_install (ctree, CONFIG_MODE, &no_svlan_static_cmd);
  cli_install (ctree, CONFIG_MODE, &no_svlan_static_filter_cmd);
  cli_install (ctree, CONFIG_MODE, &spanning_svlan_add_static_cmd);
  cli_install (ctree, CONFIG_MODE, &no_spanning_svlan_static_cmd);
#endif /* HAVE_PROVIDER_BRIDGE */
#endif /* HAVE_VLAN */
  
  /* Interface mode commands. */
  cli_install (ctree, INTERFACE_MODE, &nsm_bridge_group_cmd);
  cli_install (ctree, INTERFACE_MODE, &no_nsm_bridge_group_cmd);
  cli_install (ctree, INTERFACE_MODE, &nsm_bridge_group_span_cmd);
  cli_install (ctree, INTERFACE_MODE, &nsm_bridge_span_disable_cmd);
  cli_install (ctree, INTERFACE_MODE, &switchport_cmd);

#ifdef HAVE_PROVIDER_BRIDGE
  cli_install (ctree, INTERFACE_MODE, &nsm_bridge_protocol_discard_cmd);
  cli_install (ctree, INTERFACE_MODE, &nsm_bridge_protocol_peer_cmd);
  cli_install (ctree, INTERFACE_MODE, &nsm_bridge_protocol_tunnel_cmd);
  cli_install (ctree, INTERFACE_MODE, &nsm_bridge_gmrp_tunnel_cmd);
  cli_install (ctree, INTERFACE_MODE, &nsm_bridge_service_attr_cmd);
  cli_install (ctree, INTERFACE_MODE, &no_nsm_bridge_service_attr_cmd);
  cli_install (ctree, INTERFACE_MODE, &nsm_bridge_uni_name_cmd);
  cli_install (ctree, INTERFACE_MODE, &no_nsm_bridge_uni_name_cmd);
  
  /* CLI to set MAX EVCs per UNI */
  cli_install (ctree, INTERFACE_MODE, &nsm_uni_max_evc_cmd);
  cli_install (ctree, INTERFACE_MODE, &no_nsm_uni_max_evc_cmd);
  
  /* CLIs for UNI TYPE */
  cli_install (ctree,  CONFIG_MODE, &nsm_bridge_uni_type_cmd);
  cli_install (ctree, INTERFACE_MODE, &nsm_interface_uni_type_cmd);

  /* CLIs for Bandwidth Profiling */
  cli_install_default (ctree, EVC_SERVICE_INSTANCE_MODE);
  cli_install (ctree, INTERFACE_MODE, &nsm_bw_profile_uni_set_cir_cmd);
  cli_install (ctree, INTERFACE_MODE, &nsm_bw_profile_uni_set_cbs_cmd);
  cli_install (ctree, INTERFACE_MODE, &nsm_bw_profile_uni_set_eir_cmd);
  cli_install (ctree, INTERFACE_MODE, &nsm_bw_profile_uni_set_ebs_cmd);
  cli_install (ctree, INTERFACE_MODE,
      &nsm_bw_profile_uni_set_coupling_flag_cmd);
  cli_install (ctree, INTERFACE_MODE,
      &nsm_bw_profile_uni_set_color_mode_cmd);

  cli_install (ctree, INTERFACE_MODE, &nsm_delete_uni_bw_profile_cmd);
  cli_install (ctree, INTERFACE_MODE, &nsm_activate_uni_bw_profile_cmd);
  
  cli_install (ctree, INTERFACE_MODE, &nsm_service_instance_bw_profile_cmd);
  cli_install (ctree, INTERFACE_MODE, 
      &nsm_unset_service_instance_bw_profile_cmd);
  
  cli_install (ctree, EVC_SERVICE_INSTANCE_MODE, 
      &nsm_exit_service_instance_bw_profile_cmd);

  cli_install (ctree, EVC_SERVICE_INSTANCE_MODE,
      &nsm_delete_evc_bw_profile_cmd);
  cli_install (ctree, EVC_SERVICE_INSTANCE_MODE,
      &nsm_activate_evc_bw_profile_cmd);

  cli_install (ctree, EVC_SERVICE_INSTANCE_MODE,
      &nsm_bw_profile_evc_set_cir_cmd);
  cli_install (ctree, EVC_SERVICE_INSTANCE_MODE,
      &nsm_bw_profile_evc_set_cbs_cmd);
  cli_install (ctree, EVC_SERVICE_INSTANCE_MODE,
      &nsm_bw_profile_evc_set_eir_cmd);
  cli_install (ctree, EVC_SERVICE_INSTANCE_MODE,
      &nsm_bw_profile_evc_set_ebs_cmd);
  cli_install (ctree, EVC_SERVICE_INSTANCE_MODE,
      &nsm_bw_profile_evc_set_coupling_flag_cmd);
  cli_install (ctree, EVC_SERVICE_INSTANCE_MODE,
      &nsm_bw_profile_evc_set_color_mode_cmd);


  cli_install (ctree, EVC_SERVICE_INSTANCE_MODE,
      &nsm_delete_evc_cos_bw_profile_cmd);

  cli_install (ctree, EVC_SERVICE_INSTANCE_MODE,
      &nsm_activate_evc_cos_bw_profile_cmd);

  cli_install (ctree, EVC_SERVICE_INSTANCE_MODE,
      &nsm_bw_profile_evc_cos_set_cir_cmd);
   cli_install (ctree, EVC_SERVICE_INSTANCE_MODE,
      &nsm_bw_profile_evc_cos_set_cbs_cmd);
   cli_install (ctree, EVC_SERVICE_INSTANCE_MODE,
      &nsm_bw_profile_evc_cos_set_eir_cmd);
  cli_install (ctree, EVC_SERVICE_INSTANCE_MODE,
      &nsm_bw_profile_evc_cos_set_ebs_cmd);

  cli_install (ctree, EVC_SERVICE_INSTANCE_MODE,
      &nsm_bw_profile_evc_cos_set_coupling_flag_cmd);

  cli_install (ctree, EVC_SERVICE_INSTANCE_MODE,
      &nsm_bw_profile_evc_cos_set_color_mode_cmd);

  /*Install service instance CLI in EVC_SERVICE_INSTANCE_MODE.
   *  so that the startup configuration
   * restores the configuration.
   */
  cli_install (ctree, EVC_SERVICE_INSTANCE_MODE,
      &nsm_service_instance_bw_profile_cmd);

#endif /* HAVE_PROVIDER_BRIDGE */

#ifdef HAVE_G8031
#ifdef HAVE_B_BEB
#ifdef HAVE_I_BEB
  cli_install (ctree, CONFIG_MODE, &g8031_i_b_beb_eps_protection_group_create_cmd);
  cli_install (ctree, CONFIG_MODE, &g8031_i_b_beb_eps_protection_group_delete_cmd);
#else
  cli_install (ctree, CONFIG_MODE, &g8031_beb_eps_protection_group_create_cmd);
  cli_install (ctree, CONFIG_MODE, &g8031_beb_eps_protection_group_delete_cmd);
#endif /* HAVE_I_BEB */  
  
#else
  cli_install (ctree, CONFIG_MODE, &g8031_eps_protection_group_create_cmd);
  cli_install (ctree, CONFIG_MODE, &g8031_eps_protection_group_delete_cmd);
#endif /*  defined HAVE_B_BEB */  
#endif /* HAVE_G8031 */

#ifdef HAVE_G8032
 #ifdef HAVE_B_BEB
   cli_install (ctree, CONFIG_MODE, &nsm_bridge_beb_ring_br_add_cmd);
   cli_install (ctree, CONFIG_MODE, &no_nsm_bridge_beb_ring_delete_cmd);
 #else
   cli_install (ctree, CONFIG_MODE, &nsm_bridge_ring_br_add_cmd);
   cli_install (ctree, CONFIG_MODE, &nsm_bridge_g8032_ring_delete_cmd);
 #endif
#endif  /*HAVE_G8032*/

#ifdef HAVE_L3
  cli_install (ctree, INTERFACE_MODE, &no_switchport_cmd);
#endif /* HAVE_L3 */

  return NSM_SUCCESS;
}

#endif /* HAVE_CUSTOM1 */
