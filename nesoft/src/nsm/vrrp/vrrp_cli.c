/* Copyright (C) 2001-2003  All Rights Reserved. */

#include "pal.h"

#include "lib.h"
#include "cli.h"
#include "prefix.h"
#include "hash.h"
#include "if.h"
#include "vty.h"

#include "nsm/nsmd.h"
#include "nsm/nsm_interface.h"
#include "nsm/nsm_debug.h"
#include "nsm/vrrp/vrrp.h"
#include "nsm/vrrp/vrrp_api.h"
#include "nsm/vrrp/vrrp_debug.h"

const char *vrrp_if_commands = "VRRP";

void vrrp_api_show (VRRP_SESSION *sess, struct cli *cli);

/*------------------------------------------------------------------------
 * vrrp_cli_return - Print error message for VRRP CLI-API.
 *-----------------------------------------------------------------------*/
int
vrrp_cli_return (struct cli *cli,
                 int ret)
{
  char *str = NULL;

  switch (ret)
    {
    case VRRP_API_SET_ERR_NO_EXIST:
      str = "VRID does not exist (cannot delete)";
      break;
    case VRRP_API_SET_ERR_ENABLED:
      str = "You must disable this session first";
      break;
    case VRRP_API_SET_ERR_DISABLED:
      str = "You must enable this session first";
      break;
    case VRRP_API_SET_ERR_BAD_IP_ADDR:
      str = "Malformed IP address";
      break;
    case VRRP_API_SET_ERR_BAD_INTF:
      str = "Can't find specified interface";
      break;
    case VRRP_API_SET_ERR_NO_INTF_IP:
      str = "VRRP interface has no addresses; can't enable";
      break;
    case VRRP_API_SET_ERR_VIP_UNSET:
      str = "MUST configure virtual IP address first";
      break;
    case VRRP_API_SET_ERR_PRIO_CANT_255:
      str = "MUST be virtual IP owner to have priority 255";
      break;
    case VRRP_API_SET_ERR_PRIO_MUST_255:
      str = "IP Address owner priority MUST be 255";
      break;
    case VRRP_API_SET_ERR_CONFIG_UNSET:
      str = "Virtual Router configuration incomplete (not enabled)";
      break;
    case VRRP_API_SET_ERR_ENABLE:
      str = "Unexpected error enabling";
      break;
    case VRRP_API_SET_ERR_DISABLE:
      str = "Unexpected error disabling";
      break;
    case VRRP_API_SET_ERR_PRIO_MISMATCH:
      str = "VRRP router priority mismatch (IP owner priority is 255)";
      break;
    case VRRP_API_SET_ERR_VIP_SET:
      str = "Must unset Virtual IP first";
      break;
    case VRRP_API_SET_WARN_DUPLICATE_IF:
      str = "Warning: This interface is used by another VrId";
      break;
    case VRRP_API_SET_ERR_CANT_BE_OWNER:
      str = "Virtual IP address not found; Cannot set default 'master'";
      break;
    case VRRP_API_SET_ERR_MUST_BE_OWNER:
      str = "Virtual IP address exists; Cannot set default 'backup'";
      break;
    case VRRP_API_SET_ERR_INTERFACE_CONFLICT_VIP_OWNER:
      str = "Interface must have VIP master address.";
      break;
    case VRRP_API_SET_ERR_NO_SUCH_SESSION:
      str = "The VRRP session does not exist";
      break;
    case VRRP_API_SET_ERR_DUPLICATE_SESSION:
      str = "Duplicate VRRP session";
      break;
    case VRRP_API_SET_ERR_CANNOT_ADD_VRRP_INSTANCE:
      str = "Cannot reefine the \"old style\" VRRP session";
      break;
    case VRRP_API_SET_ERR_CANNOT_ADD_TO_INTERFACE:
      str = "Cannot attach the session to the interface";
      break;
    case VRRP_API_SET_ERR_NO_SUCH_INTERFACE:
      str = "No such interface";
      break;
    case VRRP_API_SET_ERR_INVALID_LINKLOCAL_ADDRESS:
      str = "Virtual IPv6 address must be link local addres";
      break;
    case VRRP_API_SET_ERR_SEESION_GET_OR_CRE:
      str = "Session creation/retrieval failed";
      break;
    case VRRP_API_MASTER_FOUND:
      str = "Failed: A VRRP sessions is in Master state";
      break;
    case VRRP_API_SET_ERR_L2_INTERFACE:
      str = "A VRRP sessions can't be enabled on L2 interface";
      break;

    case VRRP_OK:
      return CLI_SUCCESS;
    default:
      str = "Unknown VRRP API error";
      break;
    }
  cli_out (cli, "%% %s\n", str);
  return CLI_ERROR;
}


void
vrrp_debug_all_on (struct cli *cli)
{
  if (cli->mode == CONFIG_MODE)
    {
      VRRP_CONF_DEBUG_ON (VRRP_DEBUG_EVENT_ALL);
      VRRP_CONF_DEBUG_ON (VRRP_DEBUG_PACKET_ALL);
    }
  VRRP_TERM_DEBUG_ON (VRRP_DEBUG_EVENT_ALL);
  VRRP_TERM_DEBUG_ON (VRRP_DEBUG_PACKET_ALL);
}

void
vrrp_debug_all_off (struct cli *cli)
{
  if (cli->mode == CONFIG_MODE)
    {
      VRRP_CONF_DEBUG_OFF (VRRP_DEBUG_EVENT_ALL);
      VRRP_CONF_DEBUG_OFF (VRRP_DEBUG_PACKET_ALL);
    }
  VRRP_TERM_DEBUG_OFF (VRRP_DEBUG_EVENT_ALL);
  VRRP_TERM_DEBUG_OFF (VRRP_DEBUG_PACKET_ALL);
}

CLI (debug_vrrp,
     debug_vrrp_cmd,
     "debug vrrp (all|)",
     CLI_DEBUG_STR,
     CLI_VRRP_STR,
     "Enable all debugging")
{
  vrrp_debug_all_on (cli);
  if (cli->mode != CONFIG_MODE)
    cli_out (cli, "All possible vrrp debugging has been turned on\n");

  return CLI_SUCCESS;
}

CLI (no_debug_vrrp,
     no_debug_vrrp_cmd,
     "no debug vrrp (all|)",
     CLI_NO_STR,
     CLI_DEBUG_STR,
     CLI_VRRP_STR,
     "Enable all debugging")
{
  vrrp_debug_all_off (cli);
  if (cli->mode != CONFIG_MODE)
    cli_out (cli, "All possible vrrp debugging has been turned off\n");

  return CLI_SUCCESS;
}

ALI (no_debug_vrrp,
     undebug_vrrp_cmd,
     "undebug vrrp (all|)",
     CLI_UNDEBUG_STR,
     CLI_VRRP_STR,
     "Enable all debugging");

CLI (debug_vrrp_event,
     debug_vrrp_events_cmd,
     "debug vrrp events",
     CLI_DEBUG_STR,
     CLI_VRRP_STR,
     "VRRP events")
{
  VRRP_DEBUG_ON (cli, EVENT);

  return CLI_SUCCESS;
}

CLI (no_debug_vrrp_events,
     no_debug_vrrp_events_cmd,
     "no debug vrrp events",
     CLI_NO_STR,
     CLI_DEBUG_STR,
     CLI_VRRP_STR,
     "VRRP events")
{
  VRRP_DEBUG_OFF (cli, EVENT);

  return CLI_SUCCESS;
}

ALI (no_debug_vrrp_events,
     undebug_vrrp_events_cmd,
     "undebug vrrp events",
     CLI_UNDEBUG_STR,
     CLI_VRRP_STR,
     "VRRP events");

CLI (debug_vrrp_packet,
     debug_vrrp_packet_cmd,
     "debug vrrp packet (recv|send|)",
     CLI_DEBUG_STR,
     CLI_VRRP_STR,
     "VRRP packets",
     "VRRP receive packets",
     "VRRP send packets")
{
  u_int32_t flags = 0;

  if (argc == 0)
    SET_FLAG (flags, VRRP_DEBUG_PACKET_ALL);
  else if (argc >= 1)
    {
      SET_FLAG (flags, VRRP_DEBUG_PACKET);
      if (pal_strncmp (argv[0], "s", 1) == 0)
        SET_FLAG (flags, VRRP_DEBUG_SEND);
      else if (pal_strncmp (argv[0], "r", 1) == 0)
        SET_FLAG (flags, VRRP_DEBUG_RECV);
    }

  VRRP_DEBUG_PACKET_ON (cli, flags);

  return CLI_SUCCESS;
}

CLI (no_debug_vrrp_packet,
     no_debug_vrrp_packet_cmd,
     "no debug vrrp packet (recv|send|)",
     CLI_NO_STR,
     CLI_DEBUG_STR,
     CLI_VRRP_STR,
     "VRRP packets",
     "VRRP receive packets",
     "VRRP send packets")
{
  u_int32_t flags = 0;

  if (argc == 0)
    SET_FLAG (flags, VRRP_DEBUG_PACKET_ALL);
  else if (argc >= 1)
    {
      if (pal_strncmp (argv[0], "s", 1) == 0)
        {
          if (VRRP_DEBUG (RECV))
            SET_FLAG (flags, VRRP_DEBUG_SEND);
          else
            SET_FLAG (flags, VRRP_DEBUG_PACKET_ALL);
        }
      else if (pal_strncmp (argv[0], "r", 1) == 0)
        {
          if (VRRP_DEBUG (SEND))
            SET_FLAG (flags, VRRP_DEBUG_RECV);
          else
            SET_FLAG (flags, VRRP_DEBUG_PACKET_ALL);
        }
    }

  VRRP_DEBUG_PACKET_OFF (cli, flags);

  return CLI_SUCCESS;
}

ALI (no_debug_vrrp_packet,
     undebug_vrrp_packet_cmd,
     "undebug vrrp packet (recv|send|)",
     CLI_UNDEBUG_STR,
     CLI_VRRP_STR,
     "VRRP packets",
     "VRRP receive packets",
     "VRRP send packets");

int
vrrp_debug_config_write (struct cli *cli)
{
  int write = 0;

  if (VRRP_CONF_DEBUG (EVENT))
    {
      cli_out (cli, "debug vrrp events\n");
      write++;
    }

  if (VRRP_CONF_DEBUG (PACKET))
    {
      if (VRRP_CONF_DEBUG (SEND)
          && VRRP_CONF_DEBUG (RECV))
        cli_out (cli, "debug vrrp packet\n");
      else if (VRRP_CONF_DEBUG (SEND))
        cli_out (cli, "debug vrrp packet send\n");
      else if (VRRP_CONF_DEBUG (RECV))
        cli_out (cli, "debug vrrp packet recv\n");
      write++;
    }

  return write;
}

CLI (show_debugging_vrrp,
     show_debugging_vrrp_cmd,
     "show debugging vrrp",
     CLI_SHOW_STR,
     CLI_DEBUG_STR,/* Setting multiple debug flags. */

     CLI_VRRP_STR)
{
  cli_out (cli, "VRRP debugging status:\n");

  if (IS_DEBUG_VRRP_EVENT)
    cli_out (cli, "  VRRP event debugging is on\n");

  if (IS_DEBUG_VRRP_PACKET)
    {
      if (IS_DEBUG_VRRP_SEND && IS_DEBUG_VRRP_RECV)
        cli_out (cli, "  VRRP packet debugging is on\n");
      else if (IS_DEBUG_VRRP_SEND)
        cli_out (cli, "  VRRP packet send debugging is on\n");
      else if (IS_DEBUG_VRRP_RECV)
        cli_out (cli, "  VRRP packet receive debugging is on\n");
    }

  cli_out (cli, "\n");

  return CLI_SUCCESS;
}

void
vrrp_cli_init_debug (struct cli_tree *ctree)
{
  cli_install (ctree, EXEC_PRIV_MODE, &show_debugging_vrrp_cmd);

  /* "debug vrrp" commands. */
  cli_install (ctree, EXEC_PRIV_MODE, &debug_vrrp_cmd);
  cli_install (ctree, EXEC_PRIV_MODE, &no_debug_vrrp_cmd);
  cli_install (ctree, EXEC_PRIV_MODE, &undebug_vrrp_cmd);
  cli_install (ctree, CONFIG_MODE, &debug_vrrp_cmd);
  cli_install (ctree, CONFIG_MODE, &no_debug_vrrp_cmd);

  /* "debug vrrp events" commands. */
  cli_install (ctree, EXEC_PRIV_MODE, &debug_vrrp_events_cmd);
  cli_install (ctree, EXEC_PRIV_MODE, &no_debug_vrrp_events_cmd);
  cli_install (ctree, EXEC_PRIV_MODE, &undebug_vrrp_events_cmd);
  cli_install (ctree, CONFIG_MODE, &debug_vrrp_events_cmd);
  cli_install (ctree, CONFIG_MODE, &no_debug_vrrp_events_cmd);

  /* "debug vrrp packet" commands. */
  cli_install (ctree, EXEC_PRIV_MODE, &debug_vrrp_packet_cmd);
  cli_install (ctree, EXEC_PRIV_MODE, &no_debug_vrrp_packet_cmd);
  cli_install (ctree, EXEC_PRIV_MODE, &undebug_vrrp_packet_cmd);
  cli_install (ctree, CONFIG_MODE, &debug_vrrp_packet_cmd);
  cli_install (ctree, CONFIG_MODE, &no_debug_vrrp_packet_cmd);
}


CLI (vrrp_vmac,
     vrrp_vmac_enable_disable_cmd,
     "vrrp vmac (enable|disable)",
     "VRRP configuration",
     "Virtual-MAC feature",
     "Enabled",
     "Disabled")
{
  int ret = VRRP_OK;

  if (! IS_APN_VR_PRIVILEGED(cli->vr))
        {
    cli_out (cli, "%% Only privileged VR can alter VMAC status");
          return CLI_ERROR;
        }
  ret = vrrp_api_set_vmac_status((pal_strncmp(argv[0], "e", 1) == 0) ? 1 : 0);

  if (ret == VRRP_OK)
    return CLI_SUCCESS;
  else
    return vrrp_cli_return(cli, ret);
}

CLI (router_ipv4_vrrp,
     router_ipv4_vrrp_cmd,
     "router vrrp <1-255> IFNAME",
     "Enable routing process",
     "Start VRRP configuration",
     "Virtual router identifier",
     "Interface name")
{
  VRRP_SESSION *sess;
  int vrid = pal_atoi(argv[0]);
  int ret;

  ret = vrrp_api_get_session_by_ifname (cli->vr->id,
                                        AF_INET,
                                        vrid,
                                        argv[1],
                                        &sess);
  if (ret != VRRP_OK)
  {
    return vrrp_cli_return(cli, ret);
  }
  cli->index = sess;
  cli->mode  = VRRP_MODE;
  return CLI_SUCCESS;
}

CLI (no_router_ipv4_vrrp,
     no_router_ipv4_vrrp_cmd,
     "no router vrrp <1-255> IFNAME",
     CLI_NO_STR,
     "Destroy routing process",
     "Delete VRRP virtual router",
     "Virtual router identifier",
     "Interface name")
{
  int vrid = pal_atoi(argv[0]);
  int ret;

  ret = vrrp_api_del_session_by_ifname (cli->vr->id,
                                        AF_INET,
                                        vrid,
                                        argv[1]);
  return vrrp_cli_return(cli, ret);
}

#ifdef HAVE_CUSTOM1 /* <<< */

CLI (router_ipv4_vrrp_vlan,
     router_ipv4_vrrp_vlan_cmd,
     "router vrrp <1-255> vlan <1-4094>",
     "Enable routing process",
     "Start VRRP configuration",
     "Virtual router identifier",
     "VLAN interface",
     "VLAN id")
{
  VRRP_SESSION *sess;
  char ifname[32];
  int vrid = pal_atoi(argv[0]);
  int ret;

  sprintf(ifname, "vlan %d", pal_atoi(argv[1]));

  ret = vrrp_api_get_session_by_ifname (cli->vr->id,
                                        AF_INET,
                                        vrid,
                                        ifname,
                                        &sess);
  if (ret != VRRP_OK)
  {
    return vrrp_cli_return(cli, ret);
  }
  cli->index = sess;
  cli->mode  = VRRP_MODE;
  return CLI_SUCCESS;
}

CLI (no_router_ipv4_vrrp_vlan,
     no_router_ipv4_vrrp_vlan_cmd,
     "no router vrrp <1-255> vlan <1-4094>",
     CLI_NO_STR,
     "Destroy routing process",
     "Delete VRRP virtual router",
     "Virtual router identifier",
     "VLAN interface",
     "VLAN id")
{
  char ifname[32];
  int vrid = pal_atoi(argv[0]);
  int ret;

  sprintf(ifname, "vlan %d", pal_atoi(argv[1]));

  ret = vrrp_api_del_session_by_ifname (cli->vr->id,
                                        AF_INET,
                                        vrid,
                                        ifname);
  return vrrp_cli_return(cli, ret);
}

#endif /* HAVE_CUSTOM1 >>> */


#ifdef HAVE_IPV6

CLI (router_ipv6_vrrp,
     router_ipv6_vrrp_cmd,
     "router ipv6 vrrp <1-255> IFNAME",
     "Enable routing process",
     "Assume IPv6 address family",
     "Start VRRP configuration",
     "Virtual router identifier",
     "Interface name")
{
  VRRP_SESSION *sess;
  int ret;
  int vrid = pal_atoi(argv[0]);

  ret = vrrp_api_get_session_by_ifname (cli->vr->id,
                                        AF_INET6,
                                        vrid,
                                        argv[1],
                                        &sess);
  if (ret != VRRP_OK)
  {
    return vrrp_cli_return(cli, ret);
  }
  cli->index = sess;
  cli->mode  = VRRP6_MODE;
  return CLI_SUCCESS;
}

CLI (no_router_ipv6_vrrp,
     no_router_ipv6_vrrp_cmd,
     "no router ipv6 vrrp <1-255> IFNAME",
     CLI_NO_STR,
     "Destroy routing process",
     "Assume IPv6 address family",
     "Delete VRRP virtual router",
     "Virtual router identifier",
     "Interface name")
{
  int ret;
  int vrid = pal_atoi(argv[0]);

  ret = vrrp_api_del_session_by_ifname (cli->vr->id,
                                        AF_INET6,
                                        vrid,
                                        argv[1]);
  return vrrp_cli_return(cli, ret);
}

#ifdef HAVE_CUSTOM1 /* <<< */
CLI (router_ipv6_vrrp_vlan,
     router_ipv6_vrrp_vlan_cmd,
     "router ipv6 vrrp <1-255> vlan <1-4094>",
     "Enable routing process",
     "Assume IPv6 address family",
     "Start VRRP configuration",
     "Virtual router identifier",
     "VLAN interface",
     "VLAN id")
{
  VRRP_SESSION *sess;
  char ifname[32];
  int vrid = pal_atoi(argv[0]);
  int ret;

  sprintf(ifname, "vlan %d", pal_atoi(argv[1]));

  ret = vrrp_api_get_session_by_ifname (cli->vr->id,
                                        AF_INET6,
                                        vrid,
                                        ifname,
                                        &sess);
  if (ret != VRRP_OK)
  {
    return vrrp_cli_return(cli, ret);
  }
  cli->index = sess;
  cli->mode  = VRRP6_MODE;
  return CLI_SUCCESS;
}

CLI (no_router_ipv6_vrrp_vlan,
     no_router_ipv6_vrrp_vlan_cmd,
     "no router ipv6 vrrp <1-255> vlan <1-4094>",
     CLI_NO_STR,
     "Destroy routing process",
     "Assume IPv6 address family",
     "Delete VRRP virtual router",
     "Virtual router identifier",
     "VLAN interface",
     "VLAN id")
{
  char ifname[32];
  int vrid = pal_atoi(argv[0]);
  int ret;

  sprintf(ifname, "vlan %d", pal_atoi(argv[1]));

  ret = vrrp_api_del_session_by_ifname (cli->vr->id,
                                        AF_INET6,
                                        vrid,
                                        ifname);
  return vrrp_cli_return(cli, ret);
}

#endif /* HAVE_CUSTOM1 >>> */

#endif /* HAVE_IPV6 */

CLI (vrrp_virtual_ipv4,
     vrrp_virtual_ipv4_cmd,
     "virtual-ip A.B.C.D (master|backup)",
     "Set virtual IPv4 address",
     "Virtual IPv4 address (only IPv4 VRRP)",
     "Set default master (IP address owner)",
     "Set default backup (Not IP address owner)")
{
  VRRP_SESSION *sess = cli->index;
  int ret;
  /* define maximum possible buffer to store the ip address */
  struct pal_in4_addr buf;
  ret = pal_inet_pton(sess->af_type, argv[0], &buf);
  if (ret<=0) {
    cli_out (cli, "%% Invalid IPv4 address\n");
    return CLI_ERROR;
  }
  ret = vrrp_api_virtual_ip (cli->vr->id,
                             sess->af_type,
                             sess->vrid,
                             sess->ifindex,
                             (u_int8_t *)&buf,
                             *((char *)argv[1]) == 'm');
  return vrrp_cli_return (cli, ret);
}

CLI (vrrp_virtual_ipv4_cisco_style,
     vrrp_virtual_ipv4_cisco_style_cmd,
     "virtual-ip A.B.C.D owner",
     "Set virtual IPv4 address",
     "Virtual IPv4 address",
     "IP address owner")
{
  VRRP_SESSION *sess = cli->index;
  int ret;
   /* define maximum possible buffer to store the ip address */
  struct pal_in4_addr buf;
  ret = pal_inet_pton(sess->af_type, argv[0], &buf);
  if (ret<=0) {
    cli_out (cli, "%% Invalid IPv4 address\n");
    return CLI_ERROR;
  }
  ret = vrrp_api_virtual_ip (cli->vr->id,
                             sess->af_type,
                             sess->vrid,
                             sess->ifindex,
                             (u_int8_t *)&buf,
                             PAL_TRUE);
  return vrrp_cli_return (cli, ret);
}

CLI (no_vrrp_virtual_ip,
     no_vrrp_virtual_ip_cmd,
     "no virtual-ip",
     CLI_NO_STR,
     "delete virtual IP address for the virtual router")
{
  VRRP_SESSION *sess = cli->index;
  int ret;
  ret = vrrp_api_virtual_ip (cli->vr->id,
                             sess->af_type,
                             sess->vrid,
                             sess->ifindex,
                             NULL,
                             PAL_FALSE);

  return vrrp_cli_return (cli, ret);
}

#ifdef HAVE_IPV6

CLI (vrrp_virtual_ipv6,
     vrrp_virtual_ipv6_cmd,
     "virtual-ipv6 X:X::X:X (master|backup)",
     "Set virtual IPv6 address for the virtual router",
     "Virtual IPv6 address (only IPv6 VRRP)",
     "Set default master (IP address owner)",
     "Set default backup (Not IP address owner)")
{
  VRRP_SESSION *sess = cli->index;
  int ret;
  /* define maximum possible buffer to store the ip address */
  struct pal_in6_addr buf;
  ret = pal_inet_pton(sess->af_type, argv[0], &buf);
  if (ret<=0) {
    cli_out (cli, "%% Invalid IPv6 address\n");
    return CLI_ERROR;
  }
  ret = vrrp_api_virtual_ip (cli->vr->id,
                             sess->af_type,
                             sess->vrid,
                             sess->ifindex,
                             (u_int8_t *)&buf,
                             *((char *)argv[1]) == 'm');
  return vrrp_cli_return (cli, ret);
}

CLI (vrrp_virtual_ipv6_cisco_style,
     vrrp_virtual_ipv6_cisco_style_cmd,
     "virtual-ipv6 X:X::X:X owner",
     "Set virtual IPv6 address for the virtual router",
     "Virtual IPv6 address (only IPv6 VRRP)",
     "Set IPv6 address owner)")
{
  VRRP_SESSION *sess = cli->index;
  int ret;
  /* define maximum possible buffer to store the ip address */
  struct pal_in6_addr buf;
  ret = pal_inet_pton(sess->af_type, argv[0], &buf);
  if (ret<=0) {
    cli_out (cli, "%% Invalid IPv6 address\n");
    return CLI_ERROR;
  }
  ret = vrrp_api_virtual_ip (cli->vr->id,
                             sess->af_type,
                             sess->vrid,
                             sess->ifindex,
                             (u_int8_t *)&buf,
                             PAL_TRUE);
  return vrrp_cli_return (cli, ret);
}

CLI (no_vrrp_virtual_ipv6,
     no_vrrp_virtual_ipv6_cmd,
     "no virtual-ipv6",
     CLI_NO_STR,
     "delete virtual IPv6 address for the virtual router")
{
  VRRP_SESSION *sess = cli->index;
  int ret;
  ret = vrrp_api_virtual_ip (cli->vr->id,
                             sess->af_type,
                             sess->vrid,
                             sess->ifindex,
                             NULL,
                             PAL_FALSE);

  return vrrp_cli_return (cli, ret);
}
#endif /* HAVE_IPV6 */

CLI (vrrp_monitored_circuit,
     vrrp_monitored_circuit_cmd,
     "circuit-failover IFNAME <1-253>",
     "Circuit failover for this VRRP session",
     "Interface name for Monitored Circuit",
     "Priority Delta")
{
  VRRP_SESSION *sess = cli->index;
  int priority_delta;
  int ret;

  CLI_GET_INTEGER ("Priority Delta", priority_delta, argv[1]);

  /* NOTE: We do not follow changes in the monitored circuit config
   *        - we need to keep its name.
   */
  ret = vrrp_api_monitored_circuit (cli->vr->id,
                                    sess->af_type,
                                    sess->vrid,
                                    sess->ifindex,
                                    argv[0],
                                    priority_delta);
  return vrrp_cli_return (cli, ret);
}

CLI (no_vrrp_monitored_circuit,
     no_vrrp_monitored_circuit_cmd,
     "no circuit-failover (IFNAME|)",
     CLI_NO_STR,
     "Circuit failover for this VRRP session",
     "Interface name for Monitored Circuit")
{
  VRRP_SESSION *sess = cli->index;
  int ret;

  ret = vrrp_api_monitored_circuit (cli->vr->id,
                                    sess->af_type,
                                    sess->vrid,
                                    sess->ifindex,
                                    NULL, 0);
  return vrrp_cli_return (cli, ret);
}

ALI (no_vrrp_monitored_circuit,
     no_vrrp_monitored_circuit_arg_cmd,
     "no circuit-failover IFNAME <1-253>",
     CLI_NO_STR,
     "Circuit failover for this VRRP session",
     "Interface name for Monitored Circuit",
     "Priority Delta");

CLI (vrrp_priority,
     vrrp_priority_cmd,
     "priority <1-255>",
     "Set router priority within virtual router",
     "Priority (Must be 255 if own virtual IP address)")
{
  int ret, prio;
  VRRP_SESSION *sess = cli->index;

  /* Convert vrid & prio. */
  prio = pal_atoi (argv[0]);

  ret = vrrp_api_priority (cli->vr->id,
                           sess->af_type,
                           sess->vrid,
                           sess->ifindex,
                           prio);
  return vrrp_cli_return (cli, ret);
}

CLI (no_vrrp_priority,
     no_vrrp_priority_cmd,
     "no priority",
     CLI_NO_STR,
     "Set router priority within virtual router")
{
  int ret;
  VRRP_SESSION *sess = cli->index;

  ret = vrrp_api_unset_priority (cli->vr->id,
                                 sess->af_type,
                                 sess->vrid,
                                 sess->ifindex);
  return vrrp_cli_return (cli, ret);
}

CLI (vrrp_advt_int,
     vrrp_advt_int_cmd,
     "advertisement-interval <1-10>",
     "Set advertisement interval",
     "Interval (in seconds)")
{
  int ret, interval;
  VRRP_SESSION *sess = cli->index;

  /* Convert vrid & interval. */
  interval = pal_atoi (argv[0]);

  /* Call API. */
  ret = vrrp_api_advt_interval (cli->vr->id,
                                sess->af_type,
                                sess->vrid,
                                sess->ifindex,
                                interval);
  return vrrp_cli_return (cli, ret);
}

CLI (no_vrrp_advt_int,
     no_vrrp_advt_int_cmd,
     "no advertisement-interval",
     CLI_NO_STR,
     "Set advertisement interval")
{
  int ret;
  VRRP_SESSION *sess = cli->index;

  /* Call API. */
  ret = vrrp_api_unset_advt_interval (cli->vr->id,
                                      sess->af_type,
                                      sess->vrid,
                                      sess->ifindex);
  return vrrp_cli_return (cli, ret);
}

CLI (vrrp_preempt_true,
     vrrp_preempt_true_cmd,
     "preempt-mode true",
     "Set preempt mode for the session",
     "Preemption enabled")
{
  int ret;
  VRRP_SESSION *sess = cli->index;

  /* Call API. */
  ret = vrrp_api_preempt_mode (cli->vr->id,
                               sess->af_type,
                               sess->vrid,
                               sess->ifindex, 1);
  return vrrp_cli_return (cli, ret);
}

CLI (vrrp_preempt_false,
     vrrp_preempt_false_cmd,
     "preempt-mode false",
     "Set preempt mode for the session",
     "Preemption disabled")
{
  int ret;
  VRRP_SESSION *sess = cli->index;

  /* Call API. */
  ret = vrrp_api_preempt_mode (cli->vr->id,
                               sess->af_type,
                               sess->vrid,
                               sess->ifindex,
                               0);
  return vrrp_cli_return (cli, ret);
}

CLI (vrrp_enable,
     vrrp_enable_cmd,
     "enable",
     "Enable VRRP session")
{
  int ret;
  VRRP_SESSION *sess = cli->index;

  /* Call API. */
  ret = vrrp_api_enable_session (cli->vr->id,
                                 sess->af_type,
                                 sess->vrid,
                                 sess->ifindex);
  return vrrp_cli_return (cli, ret);
}

CLI (vrrp_disable,
     vrrp_disable_cmd,
     "disable",
     "Disable VRRP session")
{
  int ret;
  VRRP_SESSION *sess = cli->index;

  /* Call API. */
  ret = vrrp_api_disable_session (cli->vr->id,
                                  sess->af_type,
                                  sess->vrid,
                                  sess->ifindex);
  return vrrp_cli_return (cli, ret);
}

/******************************************************************
 *                        SHOW COMMANDS
 ******************************************************************
 */

static void
_show_vmac (struct cli *cli)
{
  cli_out (cli, "VMAC ");

  if (vrrp_fe_get_vmac_status())
    cli_out (cli, "enabled\n");
  else
    cli_out (cli, "disabled\n");
}

static int
_show_sess (VRRP_SESSION *sess, struct cli *cli)
{
  char *state, *owner = NULL;
  const char *vip=NULL;
  char vip_buf[BUFSIZ];
  u_char vif_flags;
  char *init_msg = "";

  cli_out (cli, "Address family ");
  if (sess->af_type == AF_INET) {
    cli_out (cli, "IPv4\n");
  }
#ifdef HAVE_IPV6
  else if (sess->af_type == AF_INET6) {
    cli_out (cli, "IPv6\n");
  }
#endif /* HAVE_IPV6 */
  else {
    cli_out (cli, " **** Invalid address family: %d\n", sess->af_type);
  }
  cli_out (cli, "VRRP Id: %d on interface: %s\n", sess->vrid, sess->ifp->name);

  cli_out (cli, " State: ");
  if (sess->admin_state == VRRP_ADMIN_STATE_UP)
    cli_out (cli, "AdminUp   - ");
  else
    cli_out (cli, "AdminDown - ");

  if (sess->state == VRRP_STATE_INIT)
    {
      state = "Init";
      switch (sess->init_msg_code)
        {
        case VRRP_INIT_MSG_ADMIN_DOWN:
          init_msg = " (admin state down)";
          break;
        case VRRP_INIT_MSG_IF_NOT_RUNNING:
          init_msg = " (interface is not running)";
          break;
        case VRRP_INIT_MSG_CIRCUIT_DOWN:
          init_msg = " (monitored circuit is down)";
          break;
        case VRRP_INIT_MSG_NO_SUBNET:
          init_msg = " (no matching subnet)";
          break;
        case VRRP_INIT_MSG_VIP_UNSET:
          init_msg = " (virtual IP address not set)";
          break;
        default:
          init_msg = " (unexpected)";
          break;
        }
    }
  else if (sess->state == VRRP_STATE_MASTER)
    state = "Master";
  else
    state = "Backup";
  cli_out (cli, "%s%s\n", state, init_msg);

  /* Virtual IP address */
  if (sess->vip_status == VRRP_UNSET) {
    vip = "unset";
    owner = NULL;
  }
  else if (sess->af_type == AF_INET) {
    vip = pal_inet_ntop (AF_INET, &sess->vip_v4, vip_buf, BUFSIZ);
  }
#ifdef HAVE_IPV6
  else { /* IPv6 */
    vip = pal_inet_ntop (AF_INET6, &sess->vip_v6, vip_buf, BUFSIZ);
  }
#endif /* HAVE_IPV6 */
  if (*vip != 'u') {
    if (sess->owner == PAL_TRUE)
      owner = "Owner";
    else
      owner = "Not-owner";
  }
  if (owner)
    cli_out (cli, " Virtual IP address: %s (%s)\n", vip, owner);
  else
    cli_out (cli, " Virtual IP address: %s\n", vip);

  /* Priority */
  if (sess->prio == -1)
    cli_out (cli, " Priority: UNSET\n");
  else
  {
    if (sess->monitor_if)
    {
      if (sess->conf_prio == -1)
        cli_out (cli, " Priority not configured; Current priority: %d\n",
                 vrrp_adjust_priority(sess));
      else
        cli_out (cli, " Configured priority: %d, Current priority: %d\n",
                 sess->conf_prio,
                 vrrp_adjust_priority(sess));
    }
    else
      cli_out (cli, " Priority is %d\n", sess->prio);
  }
  /* Advertisement interval */
  if (sess->adv_int == -1)
    cli_out (cli, " Advertisement interval: UNSET\n");
  else
    cli_out (cli, " Advertisement interval: %d sec\n", sess->adv_int);

  /* Preempt-mode */
  cli_out (cli, " Preempt mode: %s\n", (sess->preempt == PAL_TRUE) ? "TRUE" : "FALSE");

  /* Monitored interface. */
  if (sess->monitor_if)
    {
      cli_out (cli, " Monitored circuit: %s, Priority Delta: %d, Status: %s\n",
           sess->monitor_if, sess->priority_delta,
               vrrp_if_monitored_circuit_state(sess) ? "UP":"DOWN");
    }

  cli_out (cli, " Multicast membership on ");
  vif_flags = ((struct nsm_if *)sess->ifp->info)->vrrp_if.vrrp_flags;
  if (sess->af_type == AF_INET)
    {
     cli_out(cli, "IPv4 interface %s: ", sess->ifp->name);
     if (CHECK_FLAG(vif_flags, NSM_VRRP_IPV4_MCAST_JOINED))
       cli_out (cli, "JOINED\n");
     else
       cli_out (cli, "LEFT\n");
    }
#ifdef HAVE_IPV6
  else if (sess->af_type == AF_INET6)
    {
      cli_out(cli, "IPv6 interface  %s: ", sess->ifp->name);
      if (CHECK_FLAG(vif_flags, NSM_VRRP_IPV6_MCAST_JOINED))
        cli_out (cli, "JOINED\n");
      else
        cli_out (cli, "LEFT\n");
    }
#endif /* HAVE_IPV6 */
  cli_out(cli, "\n");
  return VRRP_OK;
}

/*-----------------------------------------------------------------------
 *  show_vrrp_all() - show a list of VRIDs that are configured on
 *                    the router.
 *----------------------------------------------------------------------*/

CLI (show_vrrp_all,
     show_vrrp_all_cmd,
     "show vrrp",
     CLI_SHOW_STR,
     "VRRP information")
{
  _show_vmac (cli);
  vrrp_api_walk_sessions(cli->vr->id, cli, (vrrp_api_sess_walk_fun_t)_show_sess, NULL);

  return CLI_SUCCESS;
}

/*-----------------------------------------------------------------------
 *  show_vrrp_ipv6 - show the information for a VRRP session.
 *----------------------------------------------------------------------*/
CLI (show_router_vrrp_ipv6,
     show_router_vrrp_ipv6_cmd,
     "show vrrp ipv6 <1-255> IFNAME",
     CLI_SHOW_STR,
     "VRRP information",
     "VRRP IPv6 session",
     "Router identifier",
     "Interface name")
{
  struct nsm_master *nm = cli->vr->proto;
  struct interface *ifp;
  VRRP_SESSION *sess;
  int vrid;

  vrid = pal_atoi (argv[0]);
  ifp  = if_lookup_by_name (&nm->vr->ifm, argv[1]);
  if (! ifp) {
    return vrrp_cli_return(cli, VRRP_API_SET_ERR_NO_SUCH_INTERFACE);
  }
  sess = vrrp_api_lookup_session(cli->vr->id,
                                 AF_INET6,
                                 vrid,
                                 ifp->ifindex);
  if (sess == NULL) {
    return vrrp_cli_return(cli, VRRP_API_SET_ERR_NO_SUCH_SESSION);
  }
  _show_vmac (cli);
  _show_sess (sess, cli);
  return CLI_SUCCESS;
}

CLI (show_router_vrrp_ipv4,
     show_router_vrrp_ipv4_cmd,
     "show vrrp <1-255> IFNAME",
     CLI_SHOW_STR,
     "VRRP information",
     "VRRP IPv4 router identifier",
     "Interface name")
{
  struct nsm_master *nm = cli->vr->proto;
  struct interface *ifp;
  VRRP_SESSION *sess;
  int vrid;

  vrid = pal_atoi (argv[0]);
  ifp  = if_lookup_by_name (&nm->vr->ifm, argv[1]);
  if (! ifp) {
    return vrrp_cli_return(cli, VRRP_API_SET_ERR_NO_SUCH_INTERFACE);
  }
  sess = vrrp_api_lookup_session(cli->vr->id,
                                 AF_INET,
                                 vrid,
                                 ifp->ifindex);
  if (sess == NULL) {
    return vrrp_cli_return(cli, VRRP_API_SET_ERR_NO_SUCH_SESSION);
  }
  _show_vmac (cli);
  _show_sess (sess, cli);
  return CLI_SUCCESS;
}

/*----------------------------------------------------------------------------
 * vrrp_config_write - write VRRP configuration to config-file.
 *----------------------------------------------------------------------------
 */
static int
_write_config (VRRP_SESSION *sess, struct cli *cli)
{
  char buf[BUFSIZ];

  /* Process only for requested address family. */
  if ((cli->afi==AFI_IP  && sess->af_type != AF_INET) ||
      (cli->afi==AFI_IP6 && sess->af_type != AF_INET6)) {
    return VRRP_IGNORE;
  }
  /* Write line to create session without if name. */
  cli_out (cli, "router%s vrrp %d",
           (sess->af_type==AF_INET6) ? " ipv6":"",
           sess->vrid);
  /* Add interface name. */
#ifdef HAVE_CUSTOM1
  if (pal_strncmp(sess->ifp->name, "vlan", 4) == 0)
    cli_out (cli, " vlan %s\n", sess->ifp->name + 4);
  else
#endif /* HAVE_CUSTOM1 */
    cli_out (cli, " %s\n", sess->ifp->name);

  /* Write virtualip. */
  if (sess->vip_status == VRRP_SET)
  {
    if (sess->af_type == AF_INET)
      pal_inet_ntop (AF_INET, &sess->vip_v4, buf, sizeof(buf));
#ifdef HAVE_IPV6
    else
      pal_inet_ntop (AF_INET6, &sess->vip_v6, buf, sizeof(buf));
#endif /* HAVE_iPV6 */

    if (sess->owner == PAL_TRUE)
      cli_out (cli, " virtual-ip %s master\n", buf);
    else
      cli_out (cli, " virtual-ip %s backup\n", buf);
  }
  /* Monitored Circuit.  */
  if (sess->monitor_if) {
    cli_out (cli, " circuit-failover %s %d\n",
             sess->monitor_if, sess->priority_delta);
  }
  /* Write session priority. */
  if (sess->prio >= 0) {
    if (sess->owner == PAL_FALSE && sess->prio != 100)
    {
      if (sess->conf_prio == VRRP_UNSET)
        cli_out (cli, " priority %d\n", sess->prio);
      else
        cli_out (cli, " priority %d\n", sess->conf_prio);
    }
  }
  /* Write advertisement interval. */
  if (sess->adv_int > 1) {
    cli_out (cli, " advertisement-interval %d\n", sess->adv_int);
  }
  /* Default is preempt true, so we don't show it. in the
     configuration.  */
  if (sess->preempt == PAL_FALSE) {
    cli_out (cli, " preempt-mode false\n");
  }
  /* Enable the session. */
  if (sess->admin_state == VRRP_ADMIN_STATE_UP) {
    cli_out (cli, " enable\n");
  }
  cli_out (cli, "!\n");
  return (VRRP_OK);
}

int
vrrp_config_ipv4_write (struct cli *cli)
{
  struct nsm_master *nm = cli->vr->proto;
  int num_sess=0;

  /* Write line to inform VMAC status */
  if (vrrp_fe_get_vmac_status ())
    cli_out (cli, "vrrp vmac enable\n");
  else
    cli_out (cli, "vrrp vmac disable\n");

  cli->afi = AFI_IP;
  vrrp_api_walk_sessions(nm->vr->id, cli, (vrrp_api_sess_walk_fun_t)_write_config, &num_sess);

  return num_sess;
}

int
vrrp_config_ipv6_write (struct cli *cli)
{
  struct nsm_master *nm = cli->vr->proto;
  int num_sess=0;

  cli->afi = AFI_IP6;
  vrrp_api_walk_sessions(nm->vr->id, cli, (vrrp_api_sess_walk_fun_t)_write_config, &num_sess);

  return num_sess;
}

/*----------------------------------------------------------------------------
 * vrrp_if_master_dump - Dump all write VRRP configuration to config-file.
 *----------------------------------------------------------------------------
 */
void
vrrp_if_master_dump (struct cli *cli, struct interface *ifp)
{
  struct nsm_if *nif = ifp->info;
  VRRP_IF *vif = &nif->vrrp_if;
  VRRP_SESSION *sess;
  const char *owner=NULL, *vip=NULL;
  char vip_buf[BUFSIZ];
  int num_masters = 0;
  int num_sess = 0;
  struct listnode *node;

  cli_out (cli, "  VRRP Master of : ");

  /* Disable VRRP VRs on this inetrface */
  LIST_LOOP(vif->lh_sess_ipv4, sess, node)
  {
    num_sess++;

    if (sess->state == VRRP_STATE_MASTER
        && sess->vip_status == VRRP_SET)
      {
        if (num_masters == 0)
          cli_out (cli, "\n");

        vip = pal_inet_ntop (AF_INET, &sess->vip_v4, vip_buf, sizeof(vip_buf));
        owner = (sess->owner == PAL_TRUE) ? "Owner" : "Not Owner";
        cli_out (cli, "    Virtual IP addr %s (%s)\n", vip, owner);
        num_masters++;
    }
  }

#ifdef HAVE_IPV6
  LIST_LOOP(vif->lh_sess_ipv6, sess, node)
  {
    num_sess++;

    if (sess->state == VRRP_STATE_MASTER
        && sess->vip_status == VRRP_SET)
      {
        if (num_masters == 0)
          cli_out (cli, "\n");

        vip = pal_inet_ntop (AF_INET6, &sess->vip_v6, vip_buf, sizeof(vip_buf));
        owner = (sess->owner == PAL_TRUE) ? "Owner" : "Not Owner";
        cli_out (cli, "    Virtual IPv6 addr %s (%s)\n", vip, owner);
        num_masters++;
    }
  }
#endif /* HAVE_IPV6 */

  if (num_sess == 0) {
    cli_out (cli, " VRRP is not configured on this interface.\n");
    return;
  }
  /* there are sessions runnng - is there any master among them? */
  if (num_masters == 0)
    cli_out (cli, "    None\n");
}

#ifndef HAVE_CUSTOM1
#ifdef HAVE_SNMP_RESTART
CLI (snmp_restart_vrrp,
     snmp_restart_vrrp_cli,
     "snmp restart vrrp",
     "snmp",
     "restart",
     "vrrp")
{
  vrrp_snmp_restart ();
  return CLI_SUCCESS;
}
#endif /* HAVE_SNMP_RESTART */
#endif /* HAVE_CUSTOM1 */
/*-----------------------------------------------------------------------
 *  vrrp_cli_init() - perform any configuration initialziation, such as
 *                    registering the configuration commands.
 *----------------------------------------------------------------------*/

void
vrrp_cli_init (struct lib_globals *zg)
{
  struct cli_tree *ctree;

  ctree = zg->ctree;

  /* Install VRRP mode. */
  cli_install_config (ctree, VRRP_MODE, vrrp_config_ipv4_write);
  cli_install_default (ctree, VRRP_MODE);

  /* Install VRRP commands. */
  cli_install (ctree, CONFIG_MODE, &vrrp_vmac_enable_disable_cmd);

  cli_install (ctree, CONFIG_MODE, &router_ipv4_vrrp_cmd);
  cli_install (ctree, CONFIG_MODE, &no_router_ipv4_vrrp_cmd);
  cli_install (ctree, VRRP_MODE, &vrrp_virtual_ipv4_cmd);

  cli_install (ctree, VRRP_MODE, &vrrp_virtual_ipv4_cisco_style_cmd);
  cli_install (ctree, VRRP_MODE, &no_vrrp_virtual_ip_cmd);

#ifdef HAVE_CUSTOM1
  cli_install (ctree, CONFIG_MODE, &router_ipv4_vrrp_vlan_cmd);
  cli_install (ctree, CONFIG_MODE, &no_router_ipv4_vrrp_vlan_cmd);
#endif /* HAVE_CUSTOM1 */

  cli_install (ctree, VRRP_MODE, &vrrp_monitored_circuit_cmd);
  cli_install (ctree, VRRP_MODE, &no_vrrp_monitored_circuit_cmd);
  cli_install (ctree, VRRP_MODE, &no_vrrp_monitored_circuit_arg_cmd);

  cli_install (ctree, VRRP_MODE, &vrrp_priority_cmd);
  cli_install (ctree, VRRP_MODE, &no_vrrp_priority_cmd);
  cli_install (ctree, VRRP_MODE, &vrrp_advt_int_cmd);
  cli_install (ctree, VRRP_MODE, &no_vrrp_advt_int_cmd);
  cli_install (ctree, VRRP_MODE, &vrrp_preempt_true_cmd);
  cli_install (ctree, VRRP_MODE, &vrrp_preempt_false_cmd);
  cli_install (ctree, VRRP_MODE, &vrrp_enable_cmd);
  cli_install (ctree, VRRP_MODE, &vrrp_disable_cmd);

#ifdef HAVE_IPV6
    /* Install VRRP6 mode. */
  cli_install_config (ctree, VRRP6_MODE, vrrp_config_ipv6_write);
  cli_install_default (ctree, VRRP6_MODE);

  cli_install (ctree, CONFIG_MODE, &router_ipv6_vrrp_cmd);
  cli_install (ctree, CONFIG_MODE, &no_router_ipv6_vrrp_cmd);

#ifdef HAVE_CUSTOM1
  cli_install (ctree, CONFIG_MODE, &router_ipv6_vrrp_vlan_cmd);
  cli_install (ctree, CONFIG_MODE, &no_router_ipv6_vrrp_vlan_cmd);
#endif /* HAVE_CUSTOM1 */

  cli_install (ctree, VRRP6_MODE, &vrrp_virtual_ipv6_cmd);
  cli_install (ctree, VRRP6_MODE, &no_vrrp_virtual_ipv6_cmd);
  cli_install (ctree, VRRP6_MODE, &vrrp_virtual_ipv6_cisco_style_cmd);
  cli_install (ctree, VRRP6_MODE, &vrrp_monitored_circuit_cmd);
  cli_install (ctree, VRRP6_MODE, &no_vrrp_monitored_circuit_cmd);
  cli_install (ctree, VRRP6_MODE, &no_vrrp_monitored_circuit_arg_cmd);

  cli_install (ctree, VRRP6_MODE, &vrrp_priority_cmd);
  cli_install (ctree, VRRP6_MODE, &no_vrrp_priority_cmd);
  cli_install (ctree, VRRP6_MODE, &vrrp_advt_int_cmd);
  cli_install (ctree, VRRP6_MODE, &no_vrrp_advt_int_cmd);
  cli_install (ctree, VRRP6_MODE, &vrrp_preempt_true_cmd);
  cli_install (ctree, VRRP6_MODE, &vrrp_preempt_false_cmd);
  cli_install (ctree, VRRP6_MODE, &vrrp_enable_cmd);
  cli_install (ctree, VRRP6_MODE, &vrrp_disable_cmd);
#endif /* HAVE_IPV6 */

  cli_install (ctree, EXEC_MODE, &show_vrrp_all_cmd);
  cli_install (ctree, EXEC_MODE, &show_router_vrrp_ipv4_cmd);
  cli_install (ctree, EXEC_MODE, &show_router_vrrp_ipv6_cmd);

  vrrp_cli_init_debug (ctree);
#ifndef HAVE_CUSTOM1
#ifdef HAVE_SNMP_RESTART
  cli_install_gen (ctree, CONFIG_MODE, PRIVILEGE_MAX, CLI_FLAG_HIDDEN,
                   &snmp_restart_vrrp_cli);
#endif /* HAVE_SNMP_RESTART */
#endif /* HAVE_CUSTOM1 */
}
