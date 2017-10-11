/* Copyright (C) 2001-2003  All Rights Reserved. */

#include "pal.h"
#include "lib.h"

#include "thread.h"
#include "if.h"
#include "prefix.h"
#include "linklist.h"
#include "hash.h"
#include "filter.h"
#include "vty.h"
#include "log.h"
#include "snprintf.h"
#ifdef HAVE_OSPF_CSPF
#include "cspf_common.h"
#endif /* HAVE_OSPF_CSPF */

#include "ospfd/ospfd.h"
#include "ospfd/ospf_interface.h"
#include "ospfd/ospf_vlink.h"
#ifdef HAVE_OSPF_MULTI_AREA
#include "ospfd/ospf_multi_area_link.h"
#endif /* HAVE_OSPF_MULTI_AREA */
#include "ospfd/ospf_packet.h"
#include "ospfd/ospf_neighbor.h"
#include "ospfd/ospf_lsa.h"
#include "ospfd/ospf_lsdb.h"
#include "ospfd/ospf_flood.h"
#include "ospfd/ospf_abr.h"
#include "ospfd/ospf_asbr.h"
#include "ospfd/ospf_route.h"
#include "ospfd/ospf_nsm.h"
#include "ospfd/ospf_api.h"
#include "ospfd/ospf_cli.h"
#include "ospfd/ospf_vrf.h"
#ifdef HAVE_RESTART
#include "ospfd/ospf_restart.h"
#endif /* HAVE_RESTART */
#ifdef HAVE_OSPF_TE
#include "ospfd/ospf_te.h"
#endif /* HAVE_OSPF_TE */
#include "ospfd/ospf_debug.h"
#ifdef HAVE_OSPF_CSPF 
#include "ospfd/cspf/ospf_cspf.h"
#endif /* HAVE_OSPF_CSPF */
#ifdef HAVE_BFD
#include "ospfd/ospf_bfd.h"
#include "ospfd/ospf_bfd_api.h"
#include "ospfd/ospf_bfd_cli.h"
#endif /* HAVE_BFD */

int
ospf_cli_str2distribute_source (char *str, int *source)
{
  /* Sanity check. */
  if (str == NULL)
    return 0;

  if (pal_strncmp (str, "k", 1) == 0)
    *source = APN_ROUTE_KERNEL;
  else if (pal_strncmp (str, "c", 1) == 0)
    *source = APN_ROUTE_CONNECT;
  else if (pal_strncmp (str, "s", 1) == 0)
    *source = APN_ROUTE_STATIC;
  else if (pal_strncmp (str, "r", 1) == 0)
    *source = APN_ROUTE_RIP;
  else if (pal_strncmp (str, "b", 1) == 0)
    *source = APN_ROUTE_BGP;
  else if (pal_strncmp (str, "i", 1) == 0)
    *source = APN_ROUTE_ISIS;
  else if (pal_strncmp (str, "o", 1) == 0)
    *source = APN_ROUTE_OSPF;
  else
    return 0;

  return 1;
}

int
ospf_cli_str2filter_type (char *str, int *type)
{
  if (str == NULL)
    return 0;

  if (pal_strncmp (str, "i", 1) == 0)
    *type = FILTER_IN;
  else if (pal_strncmp (str, "o", 1) == 0)
    *type = FILTER_OUT;
  else
    return 0;

  return 1;
}

int 
ospf_printable_str (char *str, int len)
{
  int i = 0;
  char *ptr;

  ptr = str;
  for (i = 0; i < len; i++)
    {
      if (!pal_char_isprint(*ptr))
        return OSPF_NONPRINTABLE_STR;
      else
        ptr ++; 
    }
  return OSPF_PRINTABLE_STR;
}

int
ospf_cli_mask_check (struct pal_in4_addr mask)
{
  int count = 0;
  int i;
  int bit = 0;
  u_int32_t val;

  val = pal_ntoh32 (mask.s_addr);
  bit = (val >> 0) & 1;

  for (i = 1; i < IPV4_MAX_BITLEN; i++)
    if (((val >> i) & 1) != bit)
      {
        if (count > 0)
          return 0;

        bit = (val >> i) & 1;
        count++;
      }

  return 1;
}

int
ospf_cli_masklen_get (struct pal_in4_addr mask)
{
  /* Special case for 255.255.255.255 wild card.  */
  if (mask.s_addr == 0xFFFFFFFF)
    return 32;

  if (!(pal_ntoh32 (mask.s_addr) >> 31))
    mask.s_addr = ~mask.s_addr;

  return ip_masklen (mask);
}

int
ospf_cli_return (struct cli *cli, int ret)
{
  const char *str;

  switch (ret)
    {
    case OSPF_API_SET_ERR_WRONG_VALUE:
      str = "Invalid input value";
      break;
    case OSPF_API_SET_ERR_INCONSISTENT_VALUE:
      str = "Inconsistent Value";
      break;

    case OSPF_API_SET_ERR_INVALID_VALUE:
      str = "Invalid Value";
      break;
    case OSPF_API_SET_ERR_INVALID_NETWORK:
      str = "No valid network present for this neighbor";
      break;
    case OSPF_API_SET_ERR_INVALID_HELPER_POLICY:
      str = "Invalid Helper Policy";
      break;
    case OSPF_API_SET_ERR_INVALID_REASON:
      str = "Invalid Reason";
      break;
    case OSPF_API_SET_ERR_INVALID_FILTER_TYPE:
      str = "Invalid Filter Type";
      break;
    case OSPF_API_SET_ERR_EXT_MULTI_INST_NOT_ENABLED:
      str = "'Extension to Multi instance' feature is not enabled";
     break;

    case OSPF_API_SET_ERR_NETWORK_TYPE_INVALID:
      str = "Invalid type for interface";
      break;
    case OSPF_API_SET_ERR_ABR_TYPE_INVALID:
      str = "Invalid ABR type";
      break;
    case OSPF_API_SET_ERR_TIMER_VALUE_INVALID:
      str = "Invalid time value";
      break;
    case OSPF_API_SET_ERR_COST_INVALID:
      str = "Invalid cost value";
      break;
    case OSPF_API_SET_ERR_AUTH_TYPE_INVALID:
      str = "Invalid Auth-Type";
      break;
    case OSPF_API_SET_ERR_METRIC_INVALID:
      str = "Invalid Metric Value";
      break;
    case OSPF_API_SET_ERR_METRIC_TYPE_INVALID:
      str = "Invalid Metric-Type";
      break;
    case OSPF_API_SET_ERR_EXTERNAL_LSDB_LIMIT_INVALID:
      str = "Invalid External LSDB Limit";
      break;
    case OSPF_API_SET_ERR_OVERFLOW_INTERVAL_INVALID:
      str = "Invalid Overflow Interval";
      break;
    case OSPF_API_SET_ERR_REDISTRIBUTE_PROTO_INVALID:
      str = "Invalid Redistribute Proto-type";
      break;
    case OSPF_API_SET_ERR_DEFAULT_ORIGIN_INVALID:
      str = "Invalid Default Origin";
      break;
    case OSPF_API_SET_ERR_REFERENCE_BANDWIDTH_INVALID:
      str = "Invalid Reference Bandwidth";
      break;
    case OSPF_API_SET_ERR_RESTART_METHOD_INVALID:
      str = "Invalid Restart Method";
      break;
    case OSPF_API_SET_ERR_GRACE_PERIOD_INVALID:
      str = "Invalid Grace Period";
      break;
    case OSPF_API_SET_ERR_DEPRECATED_COMMAND:
      str = "Command has been deprecated";
      break;
    case OSPF_API_SET_ERR_INVALID_DOMAIN_ID:
      str = "This domain Id does not exists";
      break;
    case OSPF_API_SET_ERR_DUPLICATE_DOMAIN_ID:
      str = "Duplicate domain Id entry";
      break;
    case OSPF_API_SET_ERR_SEC_DOMAIN_ID:
     str = "configure primary domain ID first";
      break;
    case OSPF_API_SET_ERROR_REMOVE_SEC:
      str = "remove secondary domain ID first ";
      break;
    case OSPF_API_SET_ERR_NON_ZERO_DOMAIN_ID:
      str = "non-zero domain-id value exists";
     break;
    case OSPF_API_SET_ERR_INVALID_HEX_VALUE:
      str = "Invalid hex value ";
     break;

    case OSPF_API_SET_ERR_VR_NOT_EXIST:
      str = "No VR";
      break;
    case OSPF_API_SET_ERR_VRF_NOT_EXIST:
      str = "No such VRF";
      break;
    case OSPF_API_SET_ERR_VRF_ALREADY_BOUND:
      str = "Instance is already bound to other VRF";
      break;
    case OSPF_API_SET_ERR_NETWORK_EXIST:
      str = "There is already the same network statement";
      break;
    case OSPF_API_SET_ERR_NETWORK_NOT_EXIST:
      str = "Can't find specified network statement";
      break;
    case OSPF_API_SET_ERR_NETWORK_WITH_ANOTHER_INST_ID_EXIST:
      str = "Network enabled in same process with different instance ID";
      break;
    case OSPF_API_SET_ERR_NETWORK_OWNED_BY_ANOTHER_AREA:
      str = "Network enabled in another area";
      break;
    case OSPF_API_SET_ERR_VLINK_NOT_EXIST:
      str = "Can't find virtual link";
      break;
    case OSPF_API_SET_ERR_VLINK_CANT_GET:
      str = "Can't configure virtual link";
      break;
    case OSPF_API_SET_ERR_SUMMARY_ADDRESS_EXIST:
      str = "There is same summary address exists";
      break;
    case OSPF_API_SET_ERR_SUMMARY_ADDRESS_NOT_EXIST:
      str = "Can't find the specified summary address";
      break;
    case OSPF_API_SET_ERR_DISTANCE_NOT_EXIST:
      str = "Can't find specified distance";
      break;
    case OSPF_API_SET_ERR_HOST_ENTRY_EXIST:
      str = "There is already the same host statement";
      break;
    case OSPF_API_SET_ERR_HOST_ENTRY_NOT_EXIST:
      str = "Can't find the specified host entry";
      break;
    case OSPF_API_SET_ERR_REDISTRIBUTE_NOT_SET:
      str = "Redistribute is not set";
      break;
    case OSPF_API_SET_ERR_MD5_KEY_EXIST:
      str = "MD5 key is present";
      break;
    case OSPF_API_SET_ERR_MD5_KEY_NOT_EXIST:
      str = "Can't find the MD5 key";
      break;
    case OSPF_API_SET_ERR_INVALID_IPV4_ADDRESS:
      str = "Not a valid IPv4 Unicast Address";
      break;

    case OSPF_API_SET_ERR_PROCESS_ID_INVALID:
      str = "Invalid Process ID";
      break;
    case OSPF_API_SET_ERR_PROCESS_NOT_EXIST:
      str = "There isn't active OSPF instance with such ID";
      break;
    case OSPF_API_SET_ERR_SELF_REDIST:
      str = "Redistribution to a instance of OSPF from same instance not allowed";
      break;

    case OSPF_API_SET_ERR_IF_NOT_EXIST:
      str = "Can't find specified interface";
      break;
    case OSPF_API_SET_ERR_IF_PARAM_NOT_CONFIGURED:
      str = "Parameter is not configured";
      break;
    case OSPF_API_SET_ERR_IF_COST_INVALID:
      str = "Invalid cost value";
      break;
    case OSPF_API_SET_ERR_IF_TRANSMIT_DELAY_INVALID:
      str = "Invalid transmit-delay";
      break;
    case OSPF_API_SET_ERR_IF_RETRANSMIT_INTERVAL_INVALID:
      str = "Invalid re-transmit value";
      break;
    case OSPF_API_SET_ERR_IF_HELLO_INTERVAL_INVALID:
      str = "Invalid hello interval";
      break;
    case OSPF_API_SET_ERR_IF_DEAD_INTERVAL_INVALID:
      str = "Invalid dead interval";
      break;
    case OSPF_API_SET_ERR_IF_RESYNC_TIMEOUT_INVALID:
      str = "Invalid Resync Timeout value";
      break;
    case OSPF_API_SET_ERR_IF_INST_ID_NOT_MATCH:
      str = "Instance ID not matched";
      break; 

    case OSPF_API_SET_ERR_AREA_NOT_EXIST:
      str = "Specified area is not configured";
      break;
    case OSPF_API_SET_ERR_AREA_NOT_DEFAULT:
      str = "Area is a stub or nssa so virtual links are not allowed";
      break;
    case OSPF_API_SET_ERR_AREA_NOT_NSSA:
      str = "Area is not NSSA area";
      break;
    case OSPF_API_SET_ERR_AREA_IS_BACKBONE:
      str = "Can't configure it to backbone area";
      break;
    case OSPF_API_SET_ERR_AREA_IS_DEFAULT:
      str = "The area is neither stub, nor NSSA";
      break;
    case OSPF_API_SET_ERR_AREA_IS_STUB:
      str = "The area is configured as stub area already";
      break;
    case OSPF_API_SET_ERR_AREA_IS_NSSA:
      str = "The area is configured as NSSA area already";
      break;
    case OSPF_API_SET_ERR_AREA_ID_FORMAT_INVALID:
      str = "Invalid Area ID format";
      break;
    case OSPF_API_SET_ERR_AREA_ID_NOT_MATCH:
      str = "Area ID not match";
      break;
    case OSPF_API_SET_ERR_AREA_TYPE_INVALID:
      str = "Invalid Area Type";
      break;
    case OSPF_API_SET_ERR_AREA_RANGE_NOT_EXIST:
      str = "Can't find specified area range";
      break;
    case OSPF_API_SET_ERR_AREA_HAS_VLINK:
      str = "First deconfigure all virtual link through this area";
      break;
    case OSPF_API_SET_ERR_AREA_LIMIT:
      str = "Too many OSPF areas";
      break;

    case OSPF_API_SET_ERR_NBR_CONFIG_INVALID:
      str = "Neighbor command is allowed only on "
        "NBMA and point-to-multipoint networks";
      break;
    case OSPF_API_SET_ERR_NBR_P2MP_CONFIG_REQUIRED:
      str = "Cost option is required for "
        "point-to-multipoint broadcast network";
      break;
    case OSPF_API_SET_ERR_NBR_P2MP_CONFIG_INVALID:
      str = "Poll and priority option are allowed only for NBMA network";
      break;
    case OSPF_API_SET_ERR_NBR_NBMA_CONFIG_INVALID:
      str = "Cost option is allowed only for point-to-multipoint network";
      break;
    case OSPF_API_SET_ERR_NBR_STATIC_EXIST:
      str = "Neighbor address is already configured";
      break;
    case OSPF_API_SET_ERR_NBR_STATIC_NOT_EXIST:
      str = "Neighbor address does not map to an interface";
      break;

#ifdef HAVE_GMPLS
    case OSPF_API_SET_ERR_TELINK_FLOOD_SCOPE_ENABLED:
      str = "TE-link flood scope already enabled";
      break;
    case OSPF_API_SET_ERR_TELINK_FLOOD_SCOPE_NOT_ENABLED:
      str = "TE-link flood scope not enabled";
      break;
    case OSPF_API_SET_ERR_TELINK_METRIC_EXIST:
      str = "TE-link metric already configured";
      break;
    case OSPF_API_SET_ERR_TELINK_METRIC_NOT_EXIST:
      str = "TE-link metric not configured";
      break;
    case OSPF_API_SET_ERR_TELINK_LOCAL_LSA_ENABLED:
      str = "TE link local LSA already enabled";
      break; 
    case OSPF_API_SET_ERR_TELINK_LOCAL_LSA_NOT_ENABLED:
      str = "TE link local LSA not enabled";
      break;
#endif /* HAVE_GMPLS */

    case OSPF_API_SET_ERR_CSPF_INSTANCE_EXIST:
      str = "CSPF is already enabled for other OSPF process";
      break;
    case OSPF_API_SET_ERR_CSPF_INSTANCE_NOT_EXIST:
      str = "CSPF is not enabled for this OSPF process";
      break;
    case OSPF_API_SET_ERR_CSPF_CANT_START:
      str = "CSPF can not start";
      break;
    case OSPF_API_SET_ERR_SAME_ROUTER_ID_EXIST:
      str = "This router-id is already enabled on other ospf instance";
      break;
    case OSPF_API_SET_ERR_MALLOC_FAIL_FOR_ROUTERID:
      str = "Cannot allocate memory for storing router-id in the list";
      break;
    case OSPF_API_SET_ERR_IP_ADDR_IN_USE:
      str = "IP address is already added to the list";
      break;
#ifdef HAVE_OSPF_MULTI_AREA
    case OSPF_API_SET_ERR_MULTI_AREA_ADJ_NOT_SET:
      str = "Multi area adjacency not set";
      break;
    case OSPF_API_SET_ERR_MULTI_AREA_LINK_CANT_GET:
      str = "Can't configure multi-area-adjacency link";
      break;
#endif /* HAVE_OSPF_MULTI_AREA */
    case OSPF_API_SET_ERR_PASSIVE_IF_ENTRY_NOT_EXIST:
      str = "This interface is already non-passive";
      break; 
    case OSPF_API_SET_ERR_PASSIVE_IF_ENTRY_EXIST:
      str = "This interface is already passive";
      break;
    case OSPF_API_SET_ERR_NO_PASSIVE_IF_ENTRY_EXIST:
      str = "This interface is already present in no-passive interface list";
      break;
    case OSPF_API_SET_ERR_EMPTY_NEVER_RTR_ID:
      str = "Policy to never act as helper not configured for any router";
      break;
    case OSPF_API_SET_ERR_NEVER_RTR_ID_NOT_EXIST:
      str = "Policy to never act as helper not configured for this router";
      break;
    default:
      return CLI_SUCCESS;
    }

  cli_out (cli, "%% %s\n", str);

  return CLI_ERROR;
}

/* "router ospf" command */
CLI (router_ospf,
     router_ospf_cmd,
     "router ospf",
     "Enable a routing process",
     "Start OSPF configuration")
{
  int ospf_id = 0;
  int ret = 0;

  if (argc > 0)
    CLI_GET_INTEGER_RANGE ("OSPF process ID", ospf_id, argv[0], 1, 65535);

  ret = ospf_process_set (cli->vr->id, ospf_id);
  if (ret != OSPF_API_SET_SUCCESS)
    return ospf_cli_return (cli, ret); 

  cli->mode = OSPF_MODE;
  cli->index = ospf_proc_lookup (cli->vr->proto, ospf_id);

  return CLI_SUCCESS;
}

ALI (router_ospf,
     router_ospf_id_cmd,
     "router ospf <1-65535>",
     "Enable a routing process",
     "Start OSPF configuration",
     "OSPF process ID");

CLI (no_router_ospf,
     no_router_ospf_cmd,
     "no router ospf",
     CLI_NO_STR,
     "Enable a routing process",
     "Start OSPF configuration")
{
  int ospf_id = 0;
  int ret;

  if (argc > 0)
    CLI_GET_INTEGER_RANGE ("OSPF process ID", ospf_id, argv[0], 1, 65535);

  ret = ospf_process_unset (cli->vr->id, ospf_id);
  if (ret != OSPF_API_SET_SUCCESS)
    return ospf_cli_return (cli, ret);

  return CLI_SUCCESS;
}

ALI (no_router_ospf,
     no_router_ospf_id_cmd,
     "no router ospf <1-65535>",
     CLI_NO_STR,
     "Enable a routing process",
     "Start OSPF configuration",
     "OSPF process ID");

#ifdef HAVE_OSPF_MULTI_INST
CLI (enable_ospf_ext_multi_inst,
     enable_ext_ospf_multi_inst_cmd,
     "enable ext-ospf-multi-inst",
     "Enable",
     "Extension to ospf multi instance support")
{
   s_int8_t ret;

   ret = ospf_enable_ext_multi_inst (cli->vr->id); 
   if (ret != OSPF_API_SET_SUCCESS)
     ospf_cli_return (cli, ret);
   
   return CLI_SUCCESS;
}

CLI (no_enable_ospf_ext_multi_inst,
     no_enable_ext_ospf_multi_inst_cmd,
     "no enable ext-ospf-multi-inst",
     CLI_NO_STR,
     "Enable",
     "Extension to ospf multi instance support")
{
   s_int8_t ret;

   ret = ospf_disable_ext_multi_inst (cli->vr->id); 
   if (ret != OSPF_API_SET_SUCCESS)
     return ospf_cli_return (cli, ret);

   return CLI_SUCCESS;
}
#endif /* HAVE_OSPF_MULTI_INST */

#ifdef HAVE_OSPF_MULTI_INST
CLI (network_area,
     network_area_cmd,
     "network A.B.C.D/M area (A.B.C.D|<0-4294967295>) (instance-id <0-255>|)",
     "Enable routing on an IP network",
     "OSPF network prefix",
     "Set the OSPF area ID",
     "OSPF area ID in IP address format",
     "OSPF area ID as a decimal value",
     "Set the instance ID",
     "Instance ID")
#else
CLI (network_area,
     network_area_cmd,
     "network A.B.C.D/M area (A.B.C.D|<0-4294967295>)",
     "Enable routing on an IP network",
     "OSPF network prefix",
     "Set the OSPF area ID",
     "OSPF area ID in IP address format",
     "OSPF area ID as a decimal value")
#endif /* HAVE_OSPF_MULTI_INST */
{
  struct ospf *top = cli->index;
  struct pal_in4_addr area_id;
  struct prefix_ipv4 p;
  int ret = OSPF_API_SET_SUCCESS, format;
#ifdef HAVE_OSPF_MULTI_INST
  struct ospf_master *om = top->om;
  s_int16_t inst_id = OSPF_IF_INSTANCE_ID_DEFAULT;
#else
  s_int16_t inst_id = OSPF_IF_INSTANCE_ID_UNUSE;
#endif /* HAVE_OSPF_MULTI_INST */

  /* Get network prefix. */
  CLI_GET_IPV4_PREFIX ("network prefix", p, argv[0]);

  /* Get Area ID. */
  CLI_GET_OSPF_AREA_ID (area_id, format, argv[1]);
 
#ifdef HAVE_OSPF_MULTI_INST
  if (argc > 2)
    {
      /* Get Instance ID */
      CLI_GET_INTEGER_RANGE ("Instance ID", inst_id, argv[3], 0, 255);

      /* Throw an error if user has given non-zero instance-id
         without enabling the feature through CLI */
      if (inst_id !=  OSPF_IF_INSTANCE_ID_DEFAULT 
          && !CHECK_FLAG (om->flags, OSPF_EXT_MULTI_INST))
        {
          cli_out (cli, "%%Multiple instances can't be configured"
                        " as Ext-to-Multi-instance feature not enabled\n");
          return CLI_ERROR;
        }
    }
#endif /* HAVE_OSPF_MULTI_INST */

  ret = ospf_network_set (cli->vr->id, top->ospf_id,
                          p.prefix, p.prefixlen, area_id, inst_id);

  if (ret != OSPF_API_SET_SUCCESS)
    return ospf_cli_return (cli, ret);

  ret = ospf_area_format_set (cli->vr->id, top->ospf_id, area_id, format);
  if (ret != OSPF_API_SET_SUCCESS)
    return ospf_cli_return (cli, ret);

  ret = ospf_network_format_set (cli->vr->id, top->ospf_id,
                                 p.prefix, p.prefixlen, area_id, format);
  if (ret != OSPF_API_SET_SUCCESS)
    return ospf_cli_return (cli, ret);

  ret = ospf_network_wildmask_set (cli->vr->id, top->ospf_id, p.prefix,
                                   p.prefixlen, area_id,
                                   OSPF_NETWORK_MASK_DECIMAL);
  if (ret != OSPF_API_SET_SUCCESS)
    return ospf_cli_return (cli, ret);

  return CLI_SUCCESS;
}

#ifdef HAVE_OSPF_MULTI_INST
CLI (network_mask_area,
     network_mask_area_cmd,
     "network A.B.C.D A.B.C.D area (A.B.C.D|<0-4294967295>) (instance-id <0-255>|)",
     "Enable routing on an IP network",
     "Network number",
     "OSPF wild card bits",
     "Set the OSPF area ID",
     "OSPF area ID in IP address format",
     "OSPF area ID as a decimal value",
     "Set the instance ID",
     "Instance ID")
#else
CLI (network_mask_area,
     network_mask_area_cmd,
     "network A.B.C.D A.B.C.D area (A.B.C.D|<0-4294967295>))",
     "Enable routing on an IP network",
     "Network number",
     "OSPF wild card bits",
     "Set the OSPF area ID",
     "OSPF area ID in IP address format",
     "OSPF area ID as a decimal value")
#endif /* HAVE_OSPF_MULTI_INST */ 
{
  struct ospf *top = cli->index;
  struct pal_in4_addr area_id;
  struct pal_in4_addr net, mask;
#ifdef HAVE_OSPF_MULTI_INST
  struct ospf_master *om = top->om;
  s_int16_t inst_id = OSPF_IF_INSTANCE_ID_DEFAULT;
#else
  s_int16_t inst_id = OSPF_IF_INSTANCE_ID_UNUSE;
#endif /* HAVE_OSPF_MULTI_INST */
  u_char masklen;
  int ret = OSPF_API_SET_SUCCESS, format;

  /* Get network prefix. */
  CLI_GET_IPV4_ADDRESS ("network address", net, argv[0]);

  /* Get mask. */
  CLI_GET_IPV4_ADDRESS ("wildcard mask", mask, argv[1]);

  if (!ospf_cli_mask_check (mask))
    {
      cli_out (cli, "Invalid address/mask combination (discontiguous mask)\n");
      return CLI_ERROR;
    }

  masklen = ospf_cli_masklen_get (mask);

  /* Get Area ID. */
  CLI_GET_OSPF_AREA_ID (area_id, format, argv[2]);

#ifdef HAVE_OSPF_MULTI_INST
  if (argc > 3)
    {
      /* Get Instance ID */
      CLI_GET_INTEGER_RANGE ("Instance ID", inst_id, argv[4], 0, 255);

      /* Check if this feature is not enabled through CLI */
      if (inst_id != OSPF_IF_INSTANCE_ID_DEFAULT 
          && !CHECK_FLAG (om->flags, OSPF_EXT_MULTI_INST))
        {
          cli_out (cli, "%%Multiple instances can't be configured"
                        " as Ext-to-Multi-instance feature not enabled\n");
          return CLI_ERROR;
        }
    }
#endif /* HAVE_OSPF_MULTI_INST */

  ret = ospf_network_set (cli->vr->id, top->ospf_id, net, masklen, 
                          area_id, inst_id);
  if (ret != OSPF_API_SET_SUCCESS && ret != OSPF_API_SET_ERR_NETWORK_NOT_ACTIVE)
    return ospf_cli_return (cli, ret);

  ret = ospf_area_format_set (cli->vr->id, top->ospf_id, area_id, format);
  if (ret != OSPF_API_SET_SUCCESS)
    return ospf_cli_return (cli, ret);

  ret = ospf_network_format_set (cli->vr->id, top->ospf_id, net,
                                 masklen, area_id, format);
  if (ret != OSPF_API_SET_SUCCESS)
    return ospf_cli_return (cli, ret);

  ret = ospf_network_wildmask_set (cli->vr->id, top->ospf_id, net, masklen,
                                   area_id, OSPF_NETWORK_MASK_WILD);
  if (ret != OSPF_API_SET_SUCCESS)
    return ospf_cli_return (cli, ret);

  return CLI_SUCCESS;
}
#ifdef HAVE_OSPF_MULTI_INST
CLI (no_network_area,
     no_network_area_cmd,
     "no network A.B.C.D/M area (A.B.C.D|<0-4294967295>) (instance-id <0-255>|)",
     CLI_NO_STR,
     "Enable routing on an IP network",
     "OSPF network prefix",
     "Set the OSPF area ID",
     "OSPF area ID in IP address format",
     "OSPF area ID as a decimal value",
     "Set the instance ID",
     "Instance ID")
#else
CLI (no_network_area,
     no_network_area_cmd,
     "no network A.B.C.D/M area (A.B.C.D|<0-4294967295>)",
     CLI_NO_STR,
     "Enable routing on an IP network",
     "OSPF network prefix",
     "Set the OSPF area ID",
     "OSPF area ID in IP address format",
     "OSPF area ID as a decimal value")
#endif /* HAVE_OSPF_MULTI_INST */
{
  struct ospf *top = cli->index;
  struct prefix_ipv4 p;
  struct pal_in4_addr area_id;
  int ret, format;
#ifdef HAVE_OSPF_MULTI_INST
  struct ospf_master *om = top->om;
  s_int16_t inst_id = OSPF_IF_INSTANCE_ID_DEFAULT;
#else
  s_int16_t inst_id = OSPF_IF_INSTANCE_ID_UNUSE;
#endif /* HAVE_OSPF_MULTI_INST */

  CLI_GET_IPV4_PREFIX ("network prefix", p, argv[0]);
  CLI_GET_OSPF_AREA_ID (area_id, format, argv[1]);

#ifdef HAVE_OSPF_MULTI_INST
  if (argc > 2)
    {
      /* Check if instance-id not set */
      if (!CHECK_FLAG (om->flags, OSPF_EXT_MULTI_INST))
        {
          cli_out (cli, "%%Instance ID not set");
          return CLI_ERROR;
        }
      CLI_GET_INTEGER_RANGE ("Instance ID", inst_id, argv[3], 0, 255);
    }
#endif /* HAVE_OSPF_MULTI_INST */
   
  ret = ospf_network_unset (cli->vr->id, top->ospf_id,
                            p.prefix, p.prefixlen, area_id, inst_id);
  if (ret != OSPF_API_SET_SUCCESS)
    return ospf_cli_return (cli, ret);
  
  return CLI_SUCCESS;
}

#ifdef HAVE_OSPF_MULTI_INST
CLI (no_network_mask_area,
     no_network_mask_area_cmd,
     "no network A.B.C.D A.B.C.D area (A.B.C.D|<0-4294967295>) (instance-id <0-255>|)",
     CLI_NO_STR,
     "Enable routing on an IP network",
     "Network number",
     "OSPF wild card bits",
     "Set the OSPF area ID",
     "OSPF area ID in IP address format",
     "OSPF area ID as a decimal value",
     "Set the instance ID",
     "Instance ID")
#else
CLI (no_network_mask_area,
     no_network_mask_area_cmd,
     "no network A.B.C.D A.B.C.D area (A.B.C.D|<0-4294967295>)",
     CLI_NO_STR,
     "Enable routing on an IP network",
     "Network number",
     "OSPF wild card bits",
     "Set the OSPF area ID",
     "OSPF area ID in IP address format",
     "OSPF area ID as a decimal value")
#endif /* HAVE_OSPF_MULTI_INST */
{
  struct ospf *top = cli->index;
  struct pal_in4_addr area_id;
  struct pal_in4_addr net, mask;
  u_char masklen;
  int ret, format;
#ifdef HAVE_OSPF_MULTI_INST
  struct ospf_master *om = top->om;
  s_int16_t inst_id = OSPF_IF_INSTANCE_ID_DEFAULT;
#else
  s_int16_t inst_id = OSPF_IF_INSTANCE_ID_UNUSE;
#endif /* HAVE_OSPF_MULTI_INST */

  /* Get network address. */
  CLI_GET_IPV4_ADDRESS ("network address", net, argv[0]);

  /* Get mask. */
  CLI_GET_IPV4_ADDRESS ("wildcard mask", mask, argv[1]);

  if (!ospf_cli_mask_check (mask))
    {
      cli_out (cli, "Invalid address/mask combination (discontiguous mask)\n");
      return CLI_ERROR;
    }

  masklen = ospf_cli_masklen_get (mask);

  /* Get OSPF Area ID. */
  CLI_GET_OSPF_AREA_ID (area_id, format, argv[2]);

#ifdef HAVE_OSPF_MULTI_INST
  if (argc > 3)
    {
      /* Check if instance-id not set */
      if (!CHECK_FLAG (om->flags, OSPF_EXT_MULTI_INST))
        {
          cli_out (cli, "%%Instance ID not set");
          return CLI_ERROR;
        }
      CLI_GET_INTEGER_RANGE ("Instance ID", inst_id, argv[4], 0, 255);
    }
#endif /* HAVE_OSPF_MULTI_INST */

  ret = ospf_network_unset (cli->vr->id, top->ospf_id, net, masklen, 
                            area_id, inst_id);
  if (ret != OSPF_API_SET_SUCCESS)
    return ospf_cli_return (cli, ret);

  return CLI_SUCCESS;
}

CLI (ospf_router_id,
     ospf_router_id_cmd,
     "ospf router-id A.B.C.D",
     "OSPF specific commands",
     "router-id for the OSPF process",
     "OSPF router-id in IP address format")
{
  struct ospf *top = cli->index;
  struct pal_in4_addr router_id;
  int ret = 0;

  CLI_GET_IPV4_ADDRESS ("Router ID", router_id, argv[0]);

  ret = ospf_router_id_set (cli->vr->id, top->ospf_id, router_id);
  if (ret != OSPF_API_SET_SUCCESS)
    return ospf_cli_return (cli, ret);

  if (!CHECK_FLAG (top->config, OSPF_CONFIG_ROUTER_ID_USE))
    {
      cli_out (cli, "Use \"clear ip ospf process\" command to take effect\n");
      return CLI_ERROR;
    }
  else
    {
      return CLI_SUCCESS;
    }
}

ALI (ospf_router_id,
     router_id_cmd,
     "router-id A.B.C.D",
     "router-id for the OSPF process",
     "OSPF router-id in IP address format");

CLI (no_ospf_router_id,
     no_ospf_router_id_cmd,
     "no ospf router-id",
     CLI_NO_STR,
     "OSPF specific commands",
     "router-id for the OSPF process")
{
  struct ospf *top = cli->index;
  int ret = 0;
  
  if (!CHECK_FLAG (top->config, OSPF_CONFIG_ROUTER_ID))
    return CLI_SUCCESS;

  if (argc > 0) 
    {
      struct pal_in4_addr router_id;
      
      CLI_GET_IPV4_ADDRESS ("Router ID", router_id, argv[0]);    
      if (!IPV4_ADDR_SAME (&router_id, &top->router_id_config))
        {
          cli_out (cli, "%% Incorrect router id\n");
          return CLI_ERROR;
        }
    }
  
  ret = ospf_router_id_unset (cli->vr->id, top->ospf_id);
  if (ret != OSPF_API_SET_SUCCESS)
    return ospf_cli_return (cli, ret);
  
  if (!CHECK_FLAG (top->config, OSPF_CONFIG_ROUTER_ID_USE))
    {
      cli_out (cli, "Use \"clear ip ospf process\" command to take effect\n");
      return CLI_ERROR;
    }
  else
    { 
      return CLI_SUCCESS;
    }
}

ALI (no_ospf_router_id,
     no_router_id_cmd,
     "no router-id (A.B.C.D|)",
     CLI_NO_STR,
     "router-id for the OSPF process",
     "OSPF router-id in IP address format");

CLI (passive_interface,
     passive_interface_addr_cmd,
     "passive-interface (IFNAME A.B.C.D |)",
     "Suppress routing updates on an interface or on all interfaces",
     "Interface's name",
     "Address of interface")
{
  struct ospf *top = cli->index;
  struct pal_in4_addr addr;
  int ret = 0;

  if (argc == 0)
    { 
      /* Make all the interfaces into passive */
      ret = ospf_passive_interface_default_set(cli->vr->id, top->ospf_id);
    }
  else
    {
      if (argc == 1)
        {
          ret = ospf_passive_interface_set (cli->vr->id, top->ospf_id,
                                            argv[0]);
        }
      else
        {
          CLI_GET_IPV4_ADDRESS ("interface address", addr, argv[1]);
          ret = ospf_passive_interface_set_by_addr (cli->vr->id, top->ospf_id,
                                                   argv[0], addr);
        }
    }


  if (ret != OSPF_API_SET_SUCCESS)
    return ospf_cli_return (cli, ret);

  return CLI_SUCCESS;
}

ALI (passive_interface,
     passive_interface_cmd,
     "passive-interface IFNAME",
     "Suppress routing updates on an interface",
     "Interface's name");

CLI (no_passive_interface,
     no_passive_interface_addr_cmd,
     "no passive-interface (IFNAME A.B.C.D |)",
     CLI_NO_STR,
     "Allow routing updates on an interface or on all interfaces",
     "Interface's name",
     "Address of interface")
{
  struct ospf *top = cli->index;
  struct pal_in4_addr addr;
  int ret = 0;
  if (argc == 0)
    {
      /* Remove all the interfaces from passive interface list */
      ret = ospf_passive_interface_default_unset(cli->vr->id, top->ospf_id);
    }
  else
    {
      if (argc == 1)
        ret = ospf_passive_interface_unset (cli->vr->id, top->ospf_id,
                                            argv[0]);
      else
        {
          CLI_GET_IPV4_ADDRESS ("interface address", addr, argv[1]);
          ret = ospf_passive_interface_unset_by_addr (cli->vr->id, top->ospf_id,
                                                      argv[0], addr);
        }
    }

  if (ret != OSPF_API_SET_SUCCESS)
    return ospf_cli_return (cli, ret);

  return CLI_SUCCESS;
}

ALI (no_passive_interface,
     no_passive_interface_cmd,
     "no passive-interface IFNAME",
     CLI_NO_STR,
     "Allow routing updates on an interface",
     "Interface's name");

CLI (ospf_host_area,
     ospf_host_area_cmd,
     "host A.B.C.D area (A.B.C.D|<0-4294967295>)",
     "OSPF stub host entry",
     "Host address",
     "Set the OSPF area ID",
     "OSPF area ID in IP address format",
     "OSPF area ID as a decimal value")
{
  struct ospf *top = cli->index;
  struct pal_in4_addr area_id, addr;
  int format;
  int ret = 0;

  /* Get host address. */
  CLI_GET_IPV4_ADDRESS ("host address", addr, argv[0]);

  /* Get Area ID. */
  CLI_GET_OSPF_AREA_ID (area_id, format, argv[1]);

  ret = ospf_host_entry_set (cli->vr->id, top->ospf_id, addr, area_id);
  if (ret != OSPF_API_SET_SUCCESS)
    return ospf_cli_return (cli, ret);

  ret = ospf_host_entry_format_set (cli->vr->id, top->ospf_id,
                                    addr, area_id, format);
  if (ret != OSPF_API_SET_SUCCESS)
    return ospf_cli_return (cli, ret);

  return CLI_SUCCESS;
}

CLI (ospf_host_area_cost,
     ospf_host_area_cost_cmd,
     "host A.B.C.D area (A.B.C.D|<0-4294967295>) cost <0-65535>",
     "OSPF stub host entry",
     "Host address",
     "Set the OSPF area ID",
     "OSPF area ID in IP address format",
     "OSPF area ID as a decimal value",
     "Cost of host",
     "Cost")
{
  struct ospf *top = cli->index;
  struct pal_in4_addr area_id, addr;
  int format;
  int cost = 0;
  int ret = 0;

  /* Get host address. */
  CLI_GET_IPV4_ADDRESS ("host address", addr, argv[0]);

  /* Get Area ID. */
  CLI_GET_OSPF_AREA_ID (area_id, format, argv[1]);

  /* Get cost. */
  CLI_GET_INTEGER_RANGE ("Stub cost", cost, argv[2], 0, 65535);

  ret = ospf_host_entry_set (cli->vr->id, top->ospf_id, addr, area_id);
  if (ret != OSPF_API_SET_SUCCESS)
    return ospf_cli_return (cli, ret);

  ret = ospf_host_entry_format_set (cli->vr->id, top->ospf_id,
                                    addr, area_id, format);
  if (ret != OSPF_API_SET_SUCCESS)
    return ospf_cli_return (cli, ret);

  ret = ospf_host_entry_cost_set (cli->vr->id, top->ospf_id,
                                  addr, area_id, cost);
  if (ret != OSPF_API_SET_SUCCESS)
    return ospf_cli_return (cli, ret);

  return CLI_SUCCESS;
}

CLI (no_ospf_host_area,
     no_ospf_host_area_cmd,
     "no host A.B.C.D area (A.B.C.D|<0-4294967295>)",
     CLI_NO_STR,
     "OSPF stub host entry",
     "Host address",
     "Set the OSPF area ID",
     "OSPF area ID in IP address format",
     "OSPF area ID as a decimal value")
{
  struct ospf *top = cli->index;
  struct pal_in4_addr area_id, addr;
  int ret, format;

  /* Get host address. */
  CLI_GET_IPV4_ADDRESS ("host address", addr, argv[0]);

  /* Get Area ID. */
  CLI_GET_OSPF_AREA_ID (area_id, format, argv[1]);

  ret = ospf_host_entry_unset (cli->vr->id, top->ospf_id, addr, area_id);
  if (ret != OSPF_API_SET_SUCCESS)
    return ospf_cli_return (cli, ret);

  return CLI_SUCCESS;
}

CLI (no_ospf_host_area_cost,
     no_ospf_host_area_cost_cmd,
     "no host A.B.C.D area (A.B.C.D|<0-4294967295>) cost (<0-65535>|)",
     CLI_NO_STR,
     "OSPF stub host entry",
     "Host address",
     "Set the OSPF area ID",
     "OSPF area ID in IP address format",
     "OSPF area ID as a decimal value",
     "Cost of host",
     "Cost")
{
  struct ospf *top = cli->index;
  struct pal_in4_addr area_id, addr;
  int ret, format;

  /* Get host address. */
  CLI_GET_IPV4_ADDRESS ("host address", addr, argv[0]);

  /* Get Area ID. */
  CLI_GET_OSPF_AREA_ID (area_id, format, argv[1]);

  ret = ospf_host_entry_cost_unset (cli->vr->id, top->ospf_id, addr, area_id);
  if (ret == OSPF_API_SET_ERR_AREA_ID_NOT_MATCH)
    return ospf_cli_return (cli, ret);

  return CLI_SUCCESS;
}

CLI (ospf_abr_type,
     ospf_abr_type_cmd,
     "ospf abr-type (cisco|ibm|standard|shortcut)",
     "OSPF specific commands",
     "Set OSPF ABR type",
     "Alternative ABR, Cisco implementation (RFC3509)",
     "Alternative ABR, IBM implementation (RFC3509)",
     "Standard behavior (RFC2328)",
     "Shortcut ABR")
{
  struct ospf *top = cli->index;
  u_char type;
  int ret;

  if (pal_strncmp (argv[0], "c", 1) == 0)
    type = OSPF_ABR_TYPE_CISCO;
  else if (pal_strncmp (argv[0], "i", 1) == 0)
    type = OSPF_ABR_TYPE_IBM;
  else if (pal_strncmp (argv[0], "sh", 2) == 0)
    type = OSPF_ABR_TYPE_SHORTCUT;
  else if (pal_strncmp (argv[0], "st", 2) == 0)
    type = OSPF_ABR_TYPE_STANDARD;
  else
    return ospf_cli_return (cli, OSPF_API_SET_ERR_ABR_TYPE_INVALID);

  ret = ospf_abr_type_set (cli->vr->id, top->ospf_id, type);

  if (ret != OSPF_API_SET_SUCCESS)
    return ospf_cli_return (cli, ret);

  return CLI_SUCCESS;
}

CLI (no_ospf_abr_type,
     no_ospf_abr_type_cmd,
     "no ospf abr-type (cisco|ibm|standard|shortcut|)",
     CLI_NO_STR,
     "OSPF specific commands",
     "Set OSPF ABR type",
     "Alternative ABR, Cisco implementation (RFC3509)"
     "Alternative ABR, IBM implementation (RFC3509)",
     "Standard behaviour (RFC2328)",
     "Shortcut ABR")
{
  struct ospf *top = cli->index;
  int ret;

  ret = ospf_abr_type_unset (cli->vr->id, top->ospf_id);
  if (ret != OSPF_API_SET_SUCCESS)
    return ospf_cli_return (cli, ret);

  return CLI_SUCCESS;
}

CLI (ospf_compatible_rfc1583,
     ospf_compatible_rfc1583_cmd,
     "compatible rfc1583",
     "OSPF compatibility list",
     "compatible with RFC 1583")
{
  struct ospf *top = cli->index;
  int ret = 0;

  ret = ospf_compatible_rfc1583_set (cli->vr->id, top->ospf_id);
  if (ret != OSPF_API_SET_SUCCESS)
    return ospf_cli_return (cli, ret);

  return CLI_SUCCESS;
}

CLI (no_ospf_compatible_rfc1583,
     no_ospf_compatible_rfc1583_cmd,
     "no compatible rfc1583",
     CLI_NO_STR,
     "OSPF compatibility list",
     "compatible with RFC 1583")
{
  struct ospf *top = cli->index;
  int ret = 0;

  ret = ospf_compatible_rfc1583_unset (cli->vr->id, top->ospf_id);
  if (ret != OSPF_API_SET_SUCCESS)
    return ospf_cli_return (cli, ret);

  return CLI_SUCCESS;
}

ALI (ospf_compatible_rfc1583,
     ospf_rfc1583_compatibility_cmd,
     "ospf rfc1583compatibility",
     "OSPF specific commands",
     "Enable the RFC1583Compatibility flag");

ALI (no_ospf_compatible_rfc1583,
     no_ospf_rfc1583_compatibility_cmd,
     "no ospf rfc1583compatibility",
     CLI_NO_STR,
     "OSPF specific commands",
     "Disable the RFC1583Compatibility flag");

CLI (summary_address,
     summary_address_cmd,
     "summary-address A.B.C.D/M (not-advertise|tag <0-4294967295>|)",
     "Configure IP address summaries",
     "IP summary prefix",
     "Suppress routes that match the prefix",
     "Set tag",
     "32-bit tag value")
{
  struct ospf *top = cli->index;
  struct prefix_ipv4 p;
  u_int32_t tag;
  int ret = 0;

  CLI_GET_IPV4_PREFIX ("summary address prefix", p, argv[0]);

  ret = ospf_summary_address_set (cli->vr->id, top->ospf_id,
                                  p.prefix, p.prefixlen);
  if (ret != OSPF_API_SET_SUCCESS)
    return ospf_cli_return (cli, ret);

  if (argc > 1)
    {
      if (pal_strncmp (argv[1], "n", 1) == 0)
        ret = ospf_summary_address_not_advertise_set (cli->vr->id, top->ospf_id,
                                                      p.prefix, p.prefixlen);
      else if (pal_strncmp (argv[1], "t", 1) == 0)
        {
          CLI_GET_UINT32 ("Route Tag", tag, argv[2]);
          ret = ospf_summary_address_tag_set (cli->vr->id, top->ospf_id,
                                              p.prefix, p.prefixlen, tag);
        }
    }
  if (ret != OSPF_API_SET_SUCCESS)
    return ospf_cli_return (cli, ret);

  return CLI_SUCCESS;
}

CLI (no_summary_address,
     no_summary_address_cmd,
     "no summary-address A.B.C.D/M",
     CLI_NO_STR,
     "Aggregate addresses",
     "Summary prefix")
{
  struct ospf *top = cli->index;
  struct prefix_ipv4 p;
  int ret = 0;

  CLI_GET_IPV4_PREFIX ("summary address prefix", p, argv[0]);

  ret = ospf_summary_address_unset (cli->vr->id, top->ospf_id,
                                    p.prefix, p.prefixlen);
  if (ret != OSPF_API_SET_SUCCESS)
    return ospf_cli_return (cli, ret);

  return CLI_SUCCESS;
}

CLI (no_summary_address_options,
     no_summary_address_options_cmd,
     "no summary-address A.B.C.D/M (not-advertise|tag (<0-4294967295>|))",
     CLI_NO_STR,
     "Configure IP address summaries",
     "IP summary prefix",
     "Suppress routes that match the prefix",
     "Set tag",
     "32-bit tag value")
{
  struct ospf *top = cli->index;
  struct prefix_ipv4 p;
  int ret = 0;

  CLI_GET_IPV4_PREFIX ("summary address prefix", p, argv[0]);

  if (pal_strncmp (argv[1], "n", 1) == 0)
    ret = ospf_summary_address_not_advertise_unset (cli->vr->id, top->ospf_id,
                                                    p.prefix, p.prefixlen);
  else if (pal_strncmp (argv[1], "t", 1) == 0)
    ret = ospf_summary_address_tag_unset (cli->vr->id, top->ospf_id,
                                          p.prefix, p.prefixlen);

  if (ret !=  OSPF_API_SET_SUCCESS)
    return ospf_cli_return (cli, ret);

  return CLI_SUCCESS;
}


CLI (timers_spf,
     timers_spf_cmd,
     "timers spf <0-2147483647> <0-2147483647>",
     "Adjust routing timers",
     "OSPF SPF timers",
     "Delay between receiving a change to SPF calculation",
     "Hold time between consecutive SPF calculations")
{
  return ospf_cli_return (cli, OSPF_API_SET_ERR_DEPRECATED_COMMAND);
}

CLI (timers_spf_exp,
     timers_spf_exp_cmd,
     "timers spf exp <0-2147483647> <0-2147483647>",
     "Adjust routing timers",
     "OSPF SPF timers",
     "Use exponential backoff delays",
     "Minimum Delay between receiving a change to SPF calculation in milliseconds",
     "Maximum Delay between receiving a change to SPF calculation in milliseconds ")
{
  struct ospf *top = cli->index;
  u_int32_t max_delay, min_delay;
  int ret;

  CLI_GET_UINT32_RANGE ("SPF min delay timer", min_delay, argv[0], 0, 2147483647);
  CLI_GET_UINT32_RANGE ("SPF max delay timer", max_delay, argv[1], 0, 2147483647);

  ret = ospf_timers_spf_set (cli->vr->id, top->ospf_id, min_delay, max_delay);
  if (ret != OSPF_API_SET_SUCCESS)
    return ospf_cli_return (cli, ret);

  return CLI_SUCCESS;
}

CLI (no_timers_spf,
     no_timers_spf_cmd,
     "no timers spf",
     CLI_NO_STR,
     "Adjust routing timers",
     "OSPF SPF timers")
{
  return ospf_cli_return (cli, OSPF_API_SET_ERR_DEPRECATED_COMMAND);
}

CLI (no_timers_spf_exp,
     no_timers_spf_exp_cmd,
     "no timers spf exp",
     CLI_NO_STR,
     "Adjust routing timers",
     "OSPF SPF timers",
     "Use exponential backoff delays")
{
  struct ospf *top = cli->index;
  int ret;

  ret = ospf_timers_spf_unset (cli->vr->id, top->ospf_id);
  if (ret != OSPF_API_SET_SUCCESS)
    return ospf_cli_return (cli, ret);

  return CLI_SUCCESS;
}

CLI (refresh_timer,
     refresh_timer_cmd,
     "refresh timer <10-1800>",
     "Adjust refresh parameters",
     "Set refresh timer",
     "Timer value in seconds")
{
  struct ospf *top = cli->index;
  int interval;
  int ret;

  CLI_GET_INTEGER_RANGE ("refresh timer", interval, argv[0], 10, 1800);

  ret = ospf_timers_refresh_set (cli->vr->id, top->ospf_id, interval);
  if (ret != OSPF_API_SET_SUCCESS)
    return ospf_cli_return (cli, ret);

  return CLI_SUCCESS;
}

CLI (no_refresh_timer,
     no_refresh_timer_cmd,
     "no refresh timer (<10-1800>|)",
     CLI_NO_STR,
     "Adjust refresh parameters",
     "Unset refresh timer",
     "Timer value in seconds")
{
  struct ospf *top = cli->index;
  int ret;

  ret = ospf_timers_refresh_unset (cli->vr->id, top->ospf_id);
  if (ret != OSPF_API_SET_SUCCESS)
    return ospf_cli_return (cli, ret);

  return CLI_SUCCESS;
}

CLI (auto_cost_reference_bandwidth,
     auto_cost_reference_bandwidth_cmd,
     "auto-cost reference-bandwidth <1-4294967>",
     "Calculate OSPF interface cost according to bandwidth",
     "Use reference bandwidth method to assign OSPF cost",
     "The reference bandwidth in terms of Mbits per second")
{
  struct ospf *top = cli->index;
  u_int32_t refbw;
  int ret;

  CLI_GET_INTEGER_RANGE ("reference bandwidth", refbw, argv[0], 1, 4294967);

  ret = ospf_auto_cost_reference_bandwidth_set (cli->vr->id,
                                                top->ospf_id, refbw);
  if (ret == OSPF_API_SET_SUCCESS)
    {
      cli_out (cli, "%% OSPF: Reference bandwidth is changed.\n");
      cli_out (cli, "        Please ensure reference bandwidth"
               "is consistent across all routers\n");
    }

  if (ret != OSPF_API_SET_SUCCESS)
    return ospf_cli_return (cli, ret);

  return CLI_SUCCESS;
}

CLI (no_auto_cost_reference_bandwidth,
     no_auto_cost_reference_bandwidth_cmd,
     "no auto-cost reference-bandwidth",
     CLI_NO_STR,
     "Calculate OSPF interface cost according to bandwidth",
     "Use reference bandwidth method to assign OSPF cost")
{
  struct ospf *top = cli->index;
  int ret;

  ret = ospf_auto_cost_reference_bandwidth_unset (cli->vr->id, top->ospf_id);
  if (ret == OSPF_API_SET_SUCCESS)
    {
      cli_out (cli, "%% OSPF: Reference bandwidth is changed.\n");
      cli_out (cli, "        Please ensure reference bandwidth"
               "is consistent across all routers\n");
    }

  if (ret != OSPF_API_SET_SUCCESS)
    return ospf_cli_return (cli, ret);

  return CLI_SUCCESS;
}

CLI (ospf_max_concurrent_dd,
     ospf_max_concurrent_dd_cmd,
     "max-concurrent-dd <1-65535>",
     "Maximum number allowed to process DD concurrently",
     "Number of DD process")
{
  struct ospf *top = cli->index;
  u_int32_t max_dd;
  int ret;

  CLI_GET_INTEGER_RANGE ("Max concurrent DD", max_dd, argv[0], 1, 65535);

  ret = ospf_max_concurrent_dd_set (cli->vr->id, top->ospf_id, 
      (u_int16_t) max_dd);
  if (ret != OSPF_API_SET_SUCCESS)
    return ospf_cli_return (cli, ret);

  return CLI_SUCCESS;
}
     
CLI (no_ospf_max_concurrent_dd,
     no_ospf_max_concurrent_dd_cmd,
     "no max-concurrent-dd",
     CLI_NO_STR,
     "Maximum number allowed to process DD concurrently")
{
  struct ospf *top = cli->index;
  int ret;

  ret = ospf_max_concurrent_dd_unset (cli->vr->id, top->ospf_id);
  if (ret != OSPF_API_SET_SUCCESS)
    return ospf_cli_return (cli, ret);

  return CLI_SUCCESS;
}

CLI (ospf_max_unuse_packet,
     ospf_max_unuse_packet_cmd,
     "max-unuse-packet <0-65535>",
     "Maximum unused OSPF packets",
     "Maximum number of unused OSPF packets")
{
  struct ospf *top = cli->index;
  u_int32_t max;
  int ret;

  CLI_GET_UINT32_RANGE ("Max unused OSPF packet", max, argv[0], 0, 65535);

  ret = ospf_max_unuse_packet_set (cli->vr->id, top->ospf_id, max);
  if (ret != OSPF_API_SET_SUCCESS)
    return ospf_cli_return (cli, ret);

  return CLI_SUCCESS;
}

CLI (no_ospf_max_unuse_packet,
     no_ospf_max_unuse_packet_cmd,
     "no max-unuse-packet (<0-65535>|)",
     CLI_NO_STR,
     "Maximum unused OSPF packets",
     "Maximum number of unused OSPF packets")
{
  struct ospf *top = cli->index;
  int ret;

  ret = ospf_max_unuse_packet_unset (cli->vr->id, top->ospf_id);
  if (ret != OSPF_API_SET_SUCCESS)
    return ospf_cli_return (cli, ret);

  return CLI_SUCCESS;
}

CLI (ospf_max_unuse_lsa,
     ospf_max_unuse_lsa_cmd,
     "max-unuse-lsa <0-65535>",
     "Maximum unused LSAs",
     "Maximum number of unused LSAs")
{
  struct ospf *top = cli->index;
  u_int32_t max;
  int ret;

  CLI_GET_UINT32_RANGE ("Max unused LSA", max, argv[0], 0, 65535);

  ret = ospf_max_unuse_lsa_set (cli->vr->id, top->ospf_id, max);
  if (ret != OSPF_API_SET_SUCCESS)
    return ospf_cli_return (cli, ret);

  return CLI_SUCCESS;
}

CLI (no_ospf_max_unuse_lsa,
     no_ospf_max_unuse_lsa_cmd,
     "no max-unuse-lsa (<0-65535>|)",
     CLI_NO_STR,
     "Maximum unused LSAs",
     "Maximum number of unused LSAs")
{
  struct ospf *top = cli->index;
  int ret;

  ret = ospf_max_unuse_lsa_unset (cli->vr->id, top->ospf_id);
  if (ret != OSPF_API_SET_SUCCESS)
    return ospf_cli_return (cli, ret);

  return CLI_SUCCESS;
}

CLI (ospf_neighbor,
     ospf_neighbor_cmd,
     "neighbor A.B.C.D",
     CLI_NEIGHBOR_STR,
     "Neighbor address")
{
  struct ospf *top = cli->index;
  struct pal_in4_addr nbr_addr;
  int priority = 0;
  int interval = 0;
  int cost = 0;
  int config = 0;
  int ret;
  int i;

  CLI_GET_IPV4_ADDRESS ("neighbor address", nbr_addr, argv[0]);

  for (i = 1; i < argc; i += 2)
    {
      if (pal_strncmp (argv[i], "pr", 2) == 0)
        {
          CLI_GET_INTEGER_RANGE ("neighbor priority",
                                 priority, argv[i + 1], 0, 255);
          SET_FLAG (config, OSPF_API_NBR_CONFIG_PRIORITY);
        }
      else if (pal_strncmp (argv[i], "po", 2) == 0)
        {
          CLI_GET_INTEGER_RANGE ("poll interval",
                                 interval, argv[i + 1], 1, 65535);
          SET_FLAG (config, OSPF_API_NBR_CONFIG_POLL_INTERVAL);
        }
      else if (pal_strncmp (argv[i], "c", 1) == 0)
        {
          CLI_GET_INTEGER_RANGE ("cost", cost, argv[i + 1], 1, 65535);
          SET_FLAG (config, OSPF_API_NBR_CONFIG_COST);
        }
    }

  ret = ospf_nbr_static_config_check (cli->vr->id, top->ospf_id,
                                      nbr_addr, config);
  if (ret != OSPF_API_SET_SUCCESS && ret != OSPF_API_WARN_INVALID_NETWORK)
    return ospf_cli_return (cli, ret);

  ret = ospf_nbr_static_set (cli->vr->id, top->ospf_id, nbr_addr);
  if (ret != OSPF_API_SET_SUCCESS)
    return ospf_cli_return (cli, ret);

  if (CHECK_FLAG (config, OSPF_API_NBR_CONFIG_PRIORITY))
    {
      ret = ospf_nbr_static_priority_set (cli->vr->id, top->ospf_id,
                                          nbr_addr, priority);
      if (ret != OSPF_API_SET_SUCCESS)
        return ospf_cli_return (cli, ret);
    }

  if (CHECK_FLAG (config, OSPF_API_NBR_CONFIG_POLL_INTERVAL))
    {
      ret = ospf_nbr_static_poll_interval_set (cli->vr->id, top->ospf_id,
                                               nbr_addr, interval);
      if (ret != OSPF_API_SET_SUCCESS)
        return ospf_cli_return (cli, ret);
    }

  if (CHECK_FLAG (config, OSPF_API_NBR_CONFIG_COST))
    {
      ret = ospf_nbr_static_cost_set (cli->vr->id, top->ospf_id, nbr_addr,
                                      cost);
      if (ret != OSPF_API_SET_SUCCESS)
        return ospf_cli_return (cli, ret);
    }

  if (ret == OSPF_API_WARN_INVALID_NETWORK)
    {
      cli_out (cli, "%% Warning: There is no corresponding ip address"
                        " for this neighbor \n");
      return CLI_ERROR;
    }

  return CLI_SUCCESS;
}

ALI (ospf_neighbor,
     ospf_neighbor_priority_poll_interval_cmd,
     "neighbor A.B.C.D {priority <0-255>|poll-interval <1-65535>}",
     CLI_NEIGHBOR_STR,
     "Neighbor address",
     "OSPF priority of non-broadcast neighbor",
     "Priority",
     "OSPF dead-router polling interval",
     "Seconds");

ALI (ospf_neighbor,
     ospf_neighbor_cost_cmd,
     "neighbor A.B.C.D (cost <1-65535>)",
     CLI_NEIGHBOR_STR,
     "Neighbor address",
     "OSPF cost for point-to-multipoint neighbor",
     "metric");

CLI (no_ospf_neighbor,
     no_ospf_neighbor_cmd,
     "no neighbor A.B.C.D",
     CLI_NO_STR,
     CLI_NEIGHBOR_STR,
     "Neighbor IP address")
{
  struct ospf *top = cli->index;
  struct pal_in4_addr nbr_addr;
  int ret;

  CLI_GET_IPV4_ADDRESS ("neighbor address", nbr_addr, argv[0]);

  ret = ospf_nbr_static_unset (cli->vr->id, top->ospf_id, nbr_addr);
  if (ret != OSPF_API_SET_SUCCESS)
    return ospf_cli_return (cli, ret);

  return CLI_SUCCESS;
}

CLI (no_ospf_neighbor_priority_poll_interval,
     no_ospf_neighbor_priority_poll_interval_cmd,
     "no neighbor A.B.C.D {priority (<0-255>|)|poll-interval (<1-65535>|)}",
     CLI_NO_STR,
     CLI_NEIGHBOR_STR,
     "Neighbor IP address",
     "Neighbor Priority",
     "Priority",
     "OSPF dead-router polling interval",
     "Seconds")
{
  struct ospf *top = cli->index;
  struct pal_in4_addr nbr_addr;
  int ret = OSPF_API_SET_SUCCESS;
  int i;

  CLI_GET_IPV4_ADDRESS ("neighbor address", nbr_addr, argv[0]);

  for (i = 1; i < argc; i++)
    {
      if (pal_char_isdigit (argv[i][0]))
        continue;

      if (pal_strncmp (argv[i], "pr", 2) == 0)
        ret = ospf_nbr_static_priority_unset (cli->vr->id, top->ospf_id,
                                              nbr_addr);
      else if (pal_strncmp (argv[i], "po", 2) == 0)
        ret = ospf_nbr_static_poll_interval_unset (cli->vr->id, top->ospf_id,
                                                   nbr_addr);
      else if (pal_strncmp (argv[i], "c", 1) == 0)
        ret = ospf_nbr_static_cost_unset (cli->vr->id, top->ospf_id, nbr_addr);

      if (ret != OSPF_API_SET_SUCCESS)
        return ospf_cli_return (cli, ret);
    }

  return CLI_SUCCESS;
}

ALI (no_ospf_neighbor_priority_poll_interval,
     no_ospf_neighbor_cost_cmd,
     "no neighbor A.B.C.D (cost (<1-65535>|))",
     CLI_NO_STR,
     CLI_NEIGHBOR_STR,
     "Neighbor address",
     "OSPF cost for point-to-multipoint neighbor",
     "metric");

CLI (ospf_overflow_lsdb_limit,
     ospf_overflow_lsdb_limit_cmd,
     "overflow database <0-4294967294> (hard|soft|)",
     "Control overflow",
     "Database",
     "Maximum number of LSAs",
     "Hard limit; Instance will be shutdown if exceed",
     "Soft limit; Warning will be given if exceed")
{
  struct ospf *top = cli->index;
  int type = OSPF_LSDB_OVERFLOW_HARD_LIMIT;
  u_int32_t limit;
  int ret = 0;

  CLI_GET_UINT32_RANGE ("LSDB limit", limit, argv[0], 0, 4294967294UL);

  if (argc > 1)
    if (pal_strncmp (argv[1], "s", 1) == 0)
      type = OSPF_LSDB_OVERFLOW_SOFT_LIMIT;

  ret = ospf_set_lsdb_limit (top, limit, type, 1);
  if (ret != OSPF_API_SET_SUCCESS)
    return ospf_cli_return (cli, ret);

  return CLI_SUCCESS;
}

CLI (no_ospf_overflow_lsdb_limit,
     no_ospf_overflow_lsdb_limit_cmd,
     "no overflow database",
     CLI_NO_STR,
     "Control overflow",
     "Database")
{
  struct ospf *top = cli->index;
  int ret = 0;

  ret = ospf_set_lsdb_limit (top, 0, 0, 0);
  if (ret != OSPF_API_SET_SUCCESS)
    return ospf_cli_return (cli, ret);

  return CLI_SUCCESS;
}

#ifdef HAVE_OSPF_DB_OVERFLOW
CLI (overflow_database_external,
     overflow_database_external_cmd,
     "overflow database external <0-2147483647> <0-65535>",
     "Control overflow",
     "Database",
     "External link states",
     "Maximum number of LSAs",
     "Time to recover (0 not recover)")
{
  struct ospf *top = cli->index;
  u_int32_t limit, interval;
  int ret = 0;

  CLI_GET_UINT32_RANGE ("external LSDB limit", limit, argv[0], 0,
                        OSPF_DEFAULT_LSDB_LIMIT);
  CLI_GET_UINT32_RANGE ("exit overflow interval",
                        interval, argv[1], 0, 65535);

  ret = ospf_overflow_database_external_interval_set (cli->vr->id,
                                                      top->ospf_id, interval);
  if (ret != OSPF_API_SET_SUCCESS)
    return ospf_cli_return (cli, ret);

  ret = ospf_overflow_database_external_limit_set (cli->vr->id, top->ospf_id,
                                                   limit);
  if (ret != OSPF_API_SET_SUCCESS)
    return ospf_cli_return (cli, ret);

  return CLI_SUCCESS;
}

CLI (no_overflow_database_external,
     no_overflow_database_external_cmd,
     "no overflow database external",
     CLI_NO_STR,
     "Control overflow",
     "Database",
     "External link states")
{
  struct ospf *top = cli->index;
  int ret = 0;

  ret = ospf_overflow_database_external_interval_unset (cli->vr->id,
                                                        top->ospf_id);
  if (ret != OSPF_API_SET_SUCCESS)
    return ospf_cli_return (cli, ret);

  ret = ospf_overflow_database_external_limit_unset (cli->vr->id,
                                                     top->ospf_id);
  if (ret != OSPF_API_SET_SUCCESS)
    return ospf_cli_return (cli, ret);

  return CLI_SUCCESS;
}
#endif /* HAVE_OSPF_DB_OVERFLOW */

CLI (ospf_max_area_limit,
     ospf_max_area_limit_cmd,
     "maximum-area <1-4294967294>",
     "Maximum number of ospf area",
     "area limit")
{
  struct ospf *top = cli->index;
  u_int32_t num;
  int ret =0;

  CLI_GET_INTEGER_RANGE ("max area limit", num, argv[0], 1, 4294967294UL);
  ret = ospf_max_area_limit_set (top, num);
  if (ret != OSPF_API_SET_SUCCESS)
    return ospf_cli_return (cli, ret);

  if ((AREA_COUNT_NON_BACKBONE (top)) > num)
    {
      cli_out (cli, "%% Warning: The number of ospf areas currently configured is higher"
                        " than the maximum-area limit. \n");
      return CLI_ERROR;
    }
  
  return CLI_SUCCESS;
}

CLI (no_ospf_max_area_limit,
     no_ospf_max_area_limit_cmd,
     "no maximum-area",
     CLI_NO_STR,
     "Maximum number of ospf area")
{
  struct ospf *top = cli->index;
  int ret = 0;

  ret =  ospf_max_area_limit_unset (top);
  if (ret != OSPF_API_SET_SUCCESS)
    return ospf_cli_return (cli, ret);

  return CLI_SUCCESS;
}


/* OSPF interface related CLIs. */
CLI (interface_ospf,
     interface_ospf_cmd,
     "interface IFNAME",
     "Select an interface to configure",
     "Interface's name")
{
  struct interface *ifp;

  ifp = ifg_get_by_name (&ZG->ifg, argv[0]);

  cli->index = ifp;
  cli->mode = INTERFACE_MODE;

  return CLI_SUCCESS;
}
#if defined HAVE_GMPLS && defined HAVE_OSPF_TE
CLI (te_link_ospf,
     te_link_ospf_cmd,
     "te-link TLNAME",
     CLI_TE_LINK_STR,
     "TE Link Name")
{
  struct telink *tl;
  struct gmpls_if *gmif;

  gmif = gmpls_if_get (ZG, cli->vr->id);
  tl = te_link_lookup_by_name (gmif, argv[0]);

  if (tl)
    {
      cli->mode = TELINK_MODE;
      cli->index = tl;
      return CLI_SUCCESS;
    }
  else
    {
      cli_out (cli, "%% TE Link %s does NOT exist, please "
               "create first. \n", argv[0]);
      return CLI_ERROR;
    }
}

CLI (te_link_ospf_detail,
     te_link_ospf_detail_cmd,
     "te-link TLNAME local-link-id A.B.C.D (numbered |)",
     CLI_TE_LINK_STR,
     "TE Link Name",
     CLI_LOCAL_LINK_ID_STR,
     "Local Link-ID in IPV4 Format",
     "Link Type Numbered")
{
  struct telink *tl;
  bool_t new = PAL_FALSE;
  struct ospf_telink *os_telink;
  struct gmpls_if *gmif;
  struct gmpls_link_id lid;

  gmif = gmpls_if_get (ZG, cli->vr->id);

  pal_mem_set (&lid, 0, sizeof (struct gmpls_link_id));
  if (!pal_inet_pton (AF_INET, argv[1], &lid.linkid.ipv4))
    return CLI_ERROR;
  
  tl = te_link_lookup_by_name (gmif, argv[0]);

  if (tl == NULL)
    {
      tl = te_link_add (cli->vr->id, cli->zg, 0, argv[0], &new); 
      SET_FLAG (tl->flags, GMPLS_TE_LLID_NOT_SET);
      GMPLS_LINK_ID_COPY (&tl->l_linkid, &lid);
      if (argc == 3)
        SET_FLAG (tl->flags, GMPLS_TE_LINK_NUMBERED); 
      os_telink = ospf_te_link_new (tl->gmif->vr->id, tl);
      tl->info = (void *)os_telink;
    } 

  cli->mode = TELINK_MODE;
  cli->index = tl;

  return CLI_SUCCESS;
}
#endif /*defined HAVE_GMPLS && defined HAVE_OSPF_TE */

#ifdef HAVE_TUNNEL
CLI (interface_tunnel_ospf,
     interface_tunnel_ospf_cmd,
     "interface tunnel <0-2147483647>",
     "Select an interface to configure",
     "Tunnel interface",
     "Tunnel interface number")
{
  u_int8_t ifname [INTERFACE_NAMSIZ + 1];
  struct interface *ifp;
  u_int32_t tid;

  CLI_GET_UINT32_RANGE ("tunnel interface number",
                        tid, argv[0], 0, 0x7FFFFFFF);

  pal_snprintf (ifname, INTERFACE_NAMSIZ, "%s%d", PAL_TUNNEL_PREFIX, tid);

  ifp = ifg_get_by_name (&ZG->ifg, ifname);

  cli->index = ifp;
  cli->mode = INTERFACE_MODE;

  return CLI_SUCCESS;
}
#endif /* HAVE_TUNNEL */

CLI (ip_ospf_network,
     ip_ospf_network_cmd,
     "ip ospf network (broadcast|non-broadcast|point-to-multipoint|point-to-point)",
     OSPF_CLI_IF_STR,
     "Network type",
     "Specify OSPF broadcast multi-access network",
     "Specify OSPF NBMA network",
     "Specify OSPF point-to-multipoint network",
     "Specify OSPF point-to-point network")
{
  struct interface *ifp = cli->index;
  int type;
  int ret;

  if (!ospf_str2network_type (argv[0], &type))
    ret = OSPF_API_SET_ERR_NETWORK_TYPE_INVALID;
  else
    ret = ospf_if_network_set (cli->vr->id, ifp->name, type);

  if (ret != OSPF_API_SET_SUCCESS)
    return ospf_cli_return (cli, ret);

  return CLI_SUCCESS;
}

CLI (ip_ospf_network_p2mp_nbma,
     ip_ospf_network_p2mp_nbma_cmd,
     "ip ospf network point-to-multipoint non-broadcast",
     OSPF_CLI_IF_STR,
     "Network type",
     "Specify OSPF point-to-multipoint network",
     "Specify non-broadcast point-to-multipoint network")
{
  struct interface *ifp = cli->index;
  int ret = 0;

  ret = ospf_if_network_p2mp_nbma_set (cli->vr->id, ifp->name);
  if (ret != OSPF_API_SET_SUCCESS)
    return ospf_cli_return (cli, ret);

  return CLI_SUCCESS;
}

CLI (no_ip_ospf_network,
     no_ip_ospf_network_cmd,
     "no ip ospf network",
     CLI_NO_STR,
     OSPF_CLI_IF_STR,
     "Network type")
{
  struct interface *ifp = cli->index;
  int ret = 0;

  ret = ospf_if_network_unset (cli->vr->id, ifp->name);
  if (ret != OSPF_API_SET_SUCCESS)
    return ospf_cli_return (cli, ret);

  return CLI_SUCCESS;
}

CLI (ip_ospf_authentication,
     ip_ospf_authentication_cmd,
     "ip ospf authentication (null|message-digest|)",
     OSPF_CLI_IF_STR,
     "Enable authentication",
     "Use no authentication",
     "Use message-digest authentication")
{
  struct interface *ifp = cli->index;
  u_char type = OSPF_AUTH_SIMPLE;
  int ret = 0;

  if (argc > 0)
    {
      if (pal_strncmp (argv[0], "n", 1) == 0)
        type = OSPF_AUTH_NULL;
      else if (pal_strncmp (argv[0], "m", 1) == 0)
        type = OSPF_AUTH_CRYPTOGRAPHIC;
      else
        {
          cli_out (cli, "%% No such authentication method\n");
          return CLI_ERROR;
        }
    }

  ret = ospf_if_authentication_type_set (cli->vr->id, ifp->name, type);
  if (ret != OSPF_API_SET_SUCCESS)
    return ospf_cli_return (cli, ret);

  return CLI_SUCCESS;
}

CLI (ip_ospf_authentication_addr,
     ip_ospf_authentication_addr_cmd,
     "ip ospf A.B.C.D authentication (null|message-digest|)",
     OSPF_CLI_IF_STR,
     OSPF_CLI_IF_ADDR_STR,
     "Enable authentication on this interface",
     "Use null authentication",
     "Use message-digest authentication")
{
  struct interface *ifp = cli->index;
  struct pal_in4_addr addr;
  u_char type = OSPF_AUTH_SIMPLE;
  int ret = 0;

  if (argc > 1)
    {
      if (pal_strncmp (argv[1], "n", 1) == 0)
        type = OSPF_AUTH_NULL;
      else if (pal_strncmp (argv[1], "m", 1) == 0)
        type = OSPF_AUTH_CRYPTOGRAPHIC;
      else
        {
          cli_out (cli, "%% No such authentication method\n");
          return CLI_ERROR;
        }
    }

  CLI_GET_IPV4_ADDRESS ("interface address", addr, argv[0]);
  ret = ospf_if_authentication_type_set_by_addr (cli->vr->id,
                                                 ifp->name, addr, type);
  if (ret != OSPF_API_SET_SUCCESS)
    return ospf_cli_return (cli, ret);

  return CLI_SUCCESS;
}

CLI (no_ip_ospf_authentication,
     no_ip_ospf_authentication_cmd,
     "no ip ospf (A.B.C.D|) authentication",
     CLI_NO_STR,
     OSPF_CLI_IF_STR,
     OSPF_CLI_IF_ADDR_STR,
     "Enable authentication on this interface")
{
  struct interface *ifp = cli->index;
  struct pal_in4_addr addr;
  int ret = 0;
  
  if (argc == 0)
    ret = ospf_if_authentication_type_unset (cli->vr->id, ifp->name);
  else
    {
      CLI_GET_IPV4_ADDRESS ("interface address", addr, argv[0]);
      ret = ospf_if_authentication_type_unset_by_addr (cli->vr->id,
                                                       ifp->name, addr);
    }

  if (ret != OSPF_API_SET_SUCCESS)
    return ospf_cli_return (cli, ret);

  return CLI_SUCCESS;
}

CLI (ip_ospf_authentication_key,
     ip_ospf_authentication_key_cmd,
     "ip ospf (A.B.C.D|) authentication-key LINE",
     OSPF_CLI_IF_STR,
     OSPF_CLI_IF_ADDR_STR,
     "Authentication password (key)",
     "The OSPF password (key)")
{
  struct interface *ifp = cli->index;
  struct pal_in4_addr addr;
  char *key;
  int key_len = 0;
  int ret = 0;
  
  if (argc == 1)
    key = argv[0]; 
  else 
    key = argv[1];

  key_len = pal_strlen (key);
  if (!ospf_printable_str (key, key_len))
    return ospf_cli_return (cli, OSPF_API_SET_ERR_WRONG_VALUE);

  if (argc == 1)
    ret = ospf_if_authentication_key_set (cli->vr->id, ifp->name, argv[0]); 
  else
    {
      CLI_GET_IPV4_ADDRESS ("interface address", addr, argv[0]);
      ret = ospf_if_authentication_key_set_by_addr (cli->vr->id, ifp->name,
                                                    addr, argv[1]);
    }

  if (ret != OSPF_API_SET_SUCCESS)
    return ospf_cli_return (cli, ret);

  return CLI_SUCCESS;
}

CLI (no_ip_ospf_authentication_key,
     no_ip_ospf_authentication_key_cmd,
     "no ip ospf (A.B.C.D|) authentication-key",
     CLI_NO_STR,
     OSPF_CLI_IF_STR,
     OSPF_CLI_IF_ADDR_STR,
     "Authentication password (key)")
{
  struct interface *ifp = cli->index;
  struct pal_in4_addr addr;
  int ret = 0;

  if (argc == 0)
    ret = ospf_if_authentication_key_unset (cli->vr->id, ifp->name);
  else
    {
      CLI_GET_IPV4_ADDRESS ("interface address", addr, argv[0]);
      ret = ospf_if_authentication_key_unset_by_addr (cli->vr->id,
                                                      ifp->name, addr);
    }

  if (ret != OSPF_API_SET_SUCCESS)
    return ospf_cli_return (cli, ret);

  return CLI_SUCCESS;
}

CLI (ip_ospf_message_digest_key,
     ip_ospf_message_digest_key_cmd,
     "ip ospf (A.B.C.D|) message-digest-key <1-255> md5 LINE",
     OSPF_CLI_IF_STR,
     OSPF_CLI_IF_ADDR_STR,
     "Message digest authentication password (key)",
     "Key ID",
     "Use MD5 algorithm",
     "The OSPF password (key)")
{
  struct interface *ifp = cli->index;
  struct pal_in4_addr addr;
  u_int32_t key_id;
  int ret;

  CLI_GET_INTEGER_RANGE ("key ID", key_id,
                         argc == 2 ? argv[0] : argv[1], 1, 255);

  if (argc == 2)
    ret = ospf_if_message_digest_key_set (cli->vr->id, ifp->name,
                                          key_id, argv[1]);
  else
    {
      CLI_GET_IPV4_ADDRESS ("interface address", addr, argv[0]);
      ret = ospf_if_message_digest_key_set_by_addr (cli->vr->id, ifp->name,
                                                    addr, key_id, argv[2]);
    }

  if (ret == OSPF_API_SET_ERR_MD5_KEY_EXIST)
    {
      cli_out (cli, "OSPF: Key %d already exists\n", key_id);
      return CLI_ERROR;
    }

  if (ret != OSPF_API_SET_SUCCESS)
    return ospf_cli_return (cli, ret);

  return CLI_SUCCESS;
}

CLI (no_ip_ospf_message_digest_key,
     no_ip_ospf_message_digest_key_cmd,
     "no ip ospf (A.B.C.D|) message-digest-key <1-255>",
     CLI_NO_STR,
     OSPF_CLI_IF_STR,
     OSPF_CLI_IF_ADDR_STR,
     "Message digest authentication password (key)",
     "Key ID")
{
  struct interface *ifp = cli->index;
  struct pal_in4_addr addr;
  u_int32_t key_id;
  int ret;
  
  CLI_GET_INTEGER_RANGE ("key ID", key_id,
                         argc == 1 ? argv[0] : argv[1], 1, 255);
  
  if (argc == 1)
    ret = ospf_if_message_digest_key_unset (cli->vr->id, ifp->name, key_id);
  else
    {
      CLI_GET_IPV4_ADDRESS ("interface address", addr, argv[0]);
      ret = ospf_if_message_digest_key_unset_by_addr (cli->vr->id, ifp->name,
                                                      addr, key_id);
    }

  if (ret == OSPF_API_SET_ERR_MD5_KEY_NOT_EXIST)
    {
      cli_out (cli, "OSPF: Key %d does not exist\n", key_id);
      return CLI_ERROR;
    }

  if (ret != OSPF_API_SET_SUCCESS)
    return ospf_cli_return (cli, ret);

  return CLI_SUCCESS;
}

CLI (ip_ospf_priority,
     ip_ospf_priority_cmd,
     "ip ospf (A.B.C.D|) priority <0-255>",
     OSPF_CLI_IF_STR,
     OSPF_CLI_IF_ADDR_STR,
     "Router priority",
     "Priority")
{
  struct interface *ifp = cli->index;
  struct pal_in4_addr addr;
  int priority;
  int ret = 0;
      
  CLI_GET_INTEGER_RANGE ("router priority", priority,
                         argc == 1 ? argv[0] : argv[1], 0, 255);

  if (argc == 1)
    ret = ospf_if_priority_set (cli->vr->id, ifp->name, priority);
  else
    {
      CLI_GET_IPV4_ADDRESS ("interface address", addr, argv[0]);
      ret = ospf_if_priority_set_by_addr (cli->vr->id, ifp->name,
                                          addr, priority);
    }

  if (ret != OSPF_API_SET_SUCCESS)
    return ospf_cli_return (cli, ret);

  return CLI_SUCCESS;
}

CLI (no_ip_ospf_priority,
     no_ip_ospf_priority_cmd,
     "no ip ospf (A.B.C.D|) priority",
     CLI_NO_STR,
     OSPF_CLI_IF_STR,
     OSPF_CLI_IF_ADDR_STR,
     "Router priority")
{
  struct interface *ifp = cli->index;
  struct pal_in4_addr addr;
  int ret = 0;
  
  if (argc == 0)
    ret = ospf_if_priority_unset (cli->vr->id, ifp->name);
  else
    {
      CLI_GET_IPV4_ADDRESS ("interface address", addr, argv[0]);
      ret = ospf_if_priority_unset_by_addr (cli->vr->id, ifp->name, addr);
    }

  if (ret != OSPF_API_SET_SUCCESS)
    return ospf_cli_return (cli, ret);

  return CLI_SUCCESS;
}

CLI (ip_ospf_cost,
     ip_ospf_cost_cmd,
     "ip ospf (A.B.C.D|) cost <1-65535>",
     OSPF_CLI_IF_STR,
     OSPF_CLI_IF_ADDR_STR,
     "Interface cost",
     "Cost")
{
  struct interface *ifp = cli->index;
  struct pal_in4_addr addr;
  u_int32_t cost;
  int ret;

  CLI_GET_INTEGER_RANGE ("interface cost", cost,
                         argc == 1 ? argv[0] : argv[1],
                         OSPF_OUTPUT_COST_MIN, OSPF_OUTPUT_COST_MAX);

  if (argc == 1)
    ret = ospf_if_cost_set (cli->vr->id, ifp->name, cost);
  else
    {
      CLI_GET_IPV4_ADDRESS ("interface address", addr, argv[0]);
      ret = ospf_if_cost_set_by_addr (cli->vr->id, ifp->name, addr, cost);
    }

  if (ret != OSPF_API_SET_SUCCESS)
    return ospf_cli_return (cli, ret);

  return CLI_SUCCESS;
}

CLI (no_ip_ospf_cost,
     no_ip_ospf_cost_cmd,
     "no ip ospf (A.B.C.D|) cost",
     CLI_NO_STR,
     OSPF_CLI_IF_STR,
     OSPF_CLI_IF_ADDR_STR,
     "Interface cost")
{
  struct interface *ifp = cli->index;
  struct pal_in4_addr addr;
  int ret;

  if (argc == 0)
    ret = ospf_if_cost_unset (cli->vr->id, ifp->name);
  else
    {
      CLI_GET_IPV4_ADDRESS ("interface address", addr, argv[0]);
      ret = ospf_if_cost_unset_by_addr (cli->vr->id, ifp->name, addr);
    }

  if (ret != OSPF_API_SET_SUCCESS)
    return ospf_cli_return (cli, ret);

  return CLI_SUCCESS;
}

CLI (ip_ospf_dead_interval,
     ip_ospf_dead_interval_cmd,
     "ip ospf (A.B.C.D|) dead-interval <1-65535>",
     OSPF_CLI_IF_STR,
     OSPF_CLI_IF_ADDR_STR,
     "Interval after which a neighbor is declared dead",
     "Seconds")
{
  struct interface *ifp = cli->index;
  struct pal_in4_addr addr;
  u_int32_t seconds;
  int ret = 0;
      
  CLI_GET_INTEGER_RANGE ("router dead interval", seconds,
                         argc == 1 ? argv[0] : argv[1], 1, 65535);

  if (argc == 1)
    ret = ospf_if_dead_interval_set (cli->vr->id, ifp->name, seconds);
  else
    {
      CLI_GET_IPV4_ADDRESS ("interface address", addr, argv[0]);
      ret = ospf_if_dead_interval_set_by_addr (cli->vr->id, ifp->name,
                                               addr, seconds);
    }

  if (ret != OSPF_API_SET_SUCCESS)
    return ospf_cli_return (cli, ret);

  return CLI_SUCCESS;
}

CLI (no_ip_ospf_dead_interval,
     no_ip_ospf_dead_interval_cmd,
     "no ip ospf (A.B.C.D|) dead-interval",
     CLI_NO_STR,
     OSPF_CLI_IF_STR,
     OSPF_CLI_IF_ADDR_STR,
     "Interval after which a neighbor is declared dead")
{
  struct interface *ifp = cli->index;
  struct pal_in4_addr addr;
  int ret = 0;

  if (argc == 0)
    ret = ospf_if_dead_interval_unset (cli->vr->id, ifp->name);
  else
    {
      CLI_GET_IPV4_ADDRESS ("interface address", addr, argv[0]);
      ret = ospf_if_dead_interval_unset_by_addr (cli->vr->id, ifp->name,
                                                 addr);
    }

  if (ret != OSPF_API_SET_SUCCESS)
    return ospf_cli_return (cli, ret);

  return CLI_SUCCESS;
}

CLI (ip_ospf_hello_interval,
     ip_ospf_hello_interval_cmd,
     "ip ospf (A.B.C.D|) hello-interval <1-65535>",
     OSPF_CLI_IF_STR,
     OSPF_CLI_IF_ADDR_STR,
     "Time between HELLO packets",
     "Seconds")
{
  struct interface *ifp = cli->index;
  struct pal_in4_addr addr;
  u_int32_t seconds;
  int ret = 0;
      
  CLI_GET_INTEGER_RANGE ("Hello interval", seconds,
                         argc == 1 ? argv[0] : argv[1], 1, 65535);

  if (argc == 1)
    ret = ospf_if_hello_interval_set (cli->vr->id, ifp->name, seconds);
  else
    {
      CLI_GET_IPV4_ADDRESS ("interface address", addr, argv[0]);
      ret = ospf_if_hello_interval_set_by_addr (cli->vr->id, ifp->name,
                                                addr, seconds);
    }

  if (ret != OSPF_API_SET_SUCCESS)
    return ospf_cli_return (cli, ret);

  return CLI_SUCCESS;
}

CLI (no_ip_ospf_hello_interval,
     no_ip_ospf_hello_interval_cmd,
     "no ip ospf (A.B.C.D|) hello-interval",
     CLI_NO_STR,
     OSPF_CLI_IF_STR,
     OSPF_CLI_IF_ADDR_STR,
     "Time between HELLO packets")
{
  struct interface *ifp = cli->index;
  struct pal_in4_addr addr;
  int ret = 0;
  
  if (argc == 0)
    ret = ospf_if_hello_interval_unset (cli->vr->id, ifp->name);
  else
    {
      CLI_GET_IPV4_ADDRESS ("interface address", addr, argv[0]);
      ret = ospf_if_hello_interval_unset_by_addr (cli->vr->id, ifp->name,
                                                  addr);
    }

  if (ret != OSPF_API_SET_SUCCESS)
    return ospf_cli_return (cli, ret);

  return CLI_SUCCESS;
}

CLI (ip_ospf_retransmit_interval,
     ip_ospf_retransmit_interval_cmd,
     "ip ospf (A.B.C.D|) retransmit-interval <5-65535>",
     OSPF_CLI_IF_STR,
     OSPF_CLI_IF_ADDR_STR,
     "Time between retransmitting lost link state advertisements",
     "Seconds")
{
  struct interface *ifp = cli->index;
  struct pal_in4_addr addr;
  u_int32_t seconds;
  int ret = 0;
      
  CLI_GET_INTEGER_RANGE ("retransmit interval", seconds,
                         argc == 1 ? argv[0] : argv[1], 5, 65535);

  if (argc == 1)
    ret = ospf_if_retransmit_interval_set (cli->vr->id, ifp->name, seconds);
  else
    {
      CLI_GET_IPV4_ADDRESS ("interface address", addr, argv[0]);
      ret = ospf_if_retransmit_interval_set_by_addr (cli->vr->id, ifp->name,
                                                     addr, seconds);
    }

  if (ret != OSPF_API_SET_SUCCESS)
    return ospf_cli_return (cli, ret);

  return CLI_SUCCESS;
}

CLI (no_ip_ospf_retransmit_interval,
     no_ip_ospf_retransmit_interval_cmd,
     "no ip ospf (A.B.C.D|) retransmit-interval",
     CLI_NO_STR,
     OSPF_CLI_IF_STR,
     OSPF_CLI_IF_ADDR_STR,
     "Time between retransmitting lost link state advertisements")
{
  struct interface *ifp = cli->index;
  struct pal_in4_addr addr;
  int ret = 0;
  
  if (argc == 0)
    ret = ospf_if_retransmit_interval_unset (cli->vr->id, ifp->name);
  else
    {
      CLI_GET_IPV4_ADDRESS ("interface address", addr, argv[0]);
      ret = ospf_if_retransmit_interval_unset_by_addr (cli->vr->id, ifp->name,
                                                       addr);
    }

  if (ret != OSPF_API_SET_SUCCESS)
    return ospf_cli_return (cli, ret);

  return CLI_SUCCESS;
}

CLI (ip_ospf_transmit_delay,
     ip_ospf_transmit_delay_cmd,
     "ip ospf (A.B.C.D|) transmit-delay <1-65535>",
     OSPF_CLI_IF_STR,
     OSPF_CLI_IF_ADDR_STR,
     "Link state transmit delay",
     "Seconds")
{
  struct interface *ifp = cli->index;
  struct pal_in4_addr addr;
  u_int32_t seconds;
  int ret = 0;
      
  CLI_GET_INTEGER_RANGE ("transmit delay", seconds,
                         argc == 1 ? argv[0] : argv[1], 1, 65535);

  if (argc == 1)
    ret = ospf_if_transmit_delay_set (cli->vr->id, ifp->name, seconds);
  else
    {
      CLI_GET_IPV4_ADDRESS ("interface address", addr, argv[0]);
      ret = ospf_if_transmit_delay_set_by_addr (cli->vr->id, ifp->name,
                                                addr, seconds);
    }

  if (ret != OSPF_API_SET_SUCCESS)
    return ospf_cli_return (cli, ret);

  return CLI_SUCCESS;
}

CLI (no_ip_ospf_transmit_delay,
     no_ip_ospf_transmit_delay_cmd,
     "no ip ospf (A.B.C.D|) transmit-delay",
     CLI_NO_STR,
     OSPF_CLI_IF_STR,
     OSPF_CLI_IF_ADDR_STR,
     "Link state transmit delay")
{
  struct interface *ifp = cli->index;
  struct pal_in4_addr addr;
  int ret = 0;
  
  if (argc == 0)
    ret = ospf_if_transmit_delay_unset (cli->vr->id, ifp->name);
  else
    {
      CLI_GET_IPV4_ADDRESS ("interface address", addr, argv[0]);
      ret = ospf_if_transmit_delay_unset_by_addr (cli->vr->id, ifp->name, addr);
    }

  if (ret != OSPF_API_SET_SUCCESS)
    return ospf_cli_return (cli, ret);

  return CLI_SUCCESS;
}

CLI (ip_ospf_database_filter_all_out,
     ip_ospf_database_filter_all_out_cmd,
     "ip ospf (A.B.C.D|) database-filter all out",
     OSPF_CLI_IF_STR,
     OSPF_CLI_IF_ADDR_STR,
     "Filter OSPF LSA during synchronization and flooding",
     "Filter all LSA",
     "Outgoing LSA")
{
  struct interface *ifp = cli->index;
  struct pal_in4_addr addr;
  int ret = 0;

  if (argc == 0)
    ret = ospf_if_database_filter_set (cli->vr->id, ifp->name);
  else
    {
      CLI_GET_IPV4_ADDRESS ("interface address", addr, argv[0]);
      ret = ospf_if_database_filter_set_by_addr (cli->vr->id, ifp->name,
                                                 addr);
    }

  if (ret != OSPF_API_SET_SUCCESS)
    return ospf_cli_return (cli, ret);

  return CLI_SUCCESS;
}

CLI (no_ip_ospf_database_filter,
     no_ip_ospf_database_filter_cmd,
     "no ip ospf (A.B.C.D|) database-filter",
     CLI_NO_STR,
     OSPF_CLI_IF_STR,
     OSPF_CLI_IF_ADDR_STR,
     "Filter OSPF LSA during synchronization and flooding")
{
  struct interface *ifp = cli->index;
  struct pal_in4_addr addr;
  int ret = 0;

  if (argc == 0)
    ret = ospf_if_database_filter_unset (cli->vr->id, ifp->name);
  else
    {
      CLI_GET_IPV4_ADDRESS ("interface address", addr, argv[0]);
      ret = ospf_if_database_filter_unset_by_addr (cli->vr->id, ifp->name,
                                                   addr);
    }

  if (ret != OSPF_API_SET_SUCCESS)
    return ospf_cli_return (cli, ret);

  return CLI_SUCCESS;
}

CLI (ip_ospf_mtu_ignore,
     ip_ospf_mtu_ignore_cmd,
     "ip ospf (A.B.C.D|) mtu-ignore",
     OSPF_CLI_IF_STR,
     OSPF_CLI_IF_ADDR_STR,
     "Ignores the MTU in DBD packets")
{
  struct interface *ifp = cli->index;
  struct pal_in4_addr addr;
  int ret = 0;

  if (argc == 0)
    ret = ospf_if_mtu_ignore_set (cli->vr->id, ifp->name);
  else
    {
      CLI_GET_IPV4_ADDRESS ("interface address", addr, argv[0]);
      ret = ospf_if_mtu_ignore_set_by_addr (cli->vr->id, ifp->name, addr);
    }

  if (ret != OSPF_API_SET_SUCCESS)
    return ospf_cli_return (cli, ret);

  return CLI_SUCCESS;
}

CLI (no_ip_ospf_mtu_ignore,
     no_ip_ospf_mtu_ignore_cmd,
     "no ip ospf (A.B.C.D|) mtu-ignore",
     CLI_NO_STR,
     OSPF_CLI_IF_STR,
     OSPF_CLI_IF_ADDR_STR,
     "Ignores the MTU in DBD packets")
{
  struct interface *ifp = cli->index;
  struct pal_in4_addr addr;
  int ret = 0;

  if (argc == 0)
    ret = ospf_if_mtu_ignore_unset (cli->vr->id, ifp->name);
  else
    {
      CLI_GET_IPV4_ADDRESS ("interface address", addr, argv[0]);
      ret = ospf_if_mtu_ignore_unset_by_addr (cli->vr->id, ifp->name, addr);
    }

  if (ret != OSPF_API_SET_SUCCESS)
    return ospf_cli_return (cli, ret);

  return CLI_SUCCESS;
}

CLI (ip_ospf_mtu,
     ip_ospf_mtu_cmd,
     "ip ospf mtu <576-65535>",
     OSPF_CLI_IF_STR,
     "OSPF interface MTU",
     "MTU size")
{
  struct interface *ifp = cli->index;
  u_int32_t mtu;
  int ret = 0;

  CLI_GET_INTEGER_RANGE ("MTU size", mtu, argv[0], 576, 65535);

  ret = ospf_if_mtu_set (cli->vr->id, ifp->name, (u_int16_t) mtu);
  if (ret != OSPF_API_SET_SUCCESS)
    return ospf_cli_return (cli, ret);

  return CLI_SUCCESS;
}

CLI (no_ip_ospf_mtu,
     no_ip_ospf_mtu_cmd,
     "no ip ospf mtu",
     CLI_NO_STR,
     OSPF_CLI_IF_STR,
     "OSPF interface MTU")
{
  struct interface *ifp = cli->index;
  int ret = 0;

  ret = ospf_if_mtu_unset (cli->vr->id, ifp->name);
  if (ret != OSPF_API_SET_SUCCESS)
    return ospf_cli_return (cli, ret);

  return CLI_SUCCESS;
}

CLI (ip_ospf_disable_all,
     ip_ospf_disable_all_cmd,
     "ip ospf disable all",
     OSPF_CLI_IF_STR,
     "Disable OSPF",
     "All functionality")
{
  struct interface *ifp = cli->index;
  int ret = 0;

  ret = ospf_if_disable_all_set (cli->vr->id, ifp->name);
  if (ret != OSPF_API_SET_SUCCESS)
    return ospf_cli_return (cli, ret);

  return CLI_SUCCESS;
}

CLI (no_ip_ospf_disable_all,
     no_ip_ospf_disable_all_cmd,
     "no ip ospf disable all",
     CLI_NO_STR,
     OSPF_CLI_IF_STR,
     "Disable OSPF",
     "All functionality")
{
  struct interface *ifp = cli->index;
  int ret = 0;

  ret = ospf_if_disable_all_unset (cli->vr->id, ifp->name);
  if (ret != OSPF_API_SET_SUCCESS)
    return ospf_cli_return (cli, ret);

  return CLI_SUCCESS;
}

#ifdef HAVE_RESTART
CLI (ip_ospf_resync_timeout,
     ip_ospf_resync_timeout_cmd,
     "ip ospf (A.B.C.D|) resync-timeout <1-65535>",
     OSPF_CLI_IF_STR,
     OSPF_CLI_IF_ADDR_STR,
     "Interval after which adjacency is reset if oob-resync is not started",
     "Seconds")
{
  struct interface *ifp = cli->index;
  struct pal_in4_addr addr;
  u_int32_t seconds;
  int ret = 0;

  CLI_GET_INTEGER_RANGE ("Resync timeout", seconds,
                         argc == 1 ? argv[0] : argv[1], 1, 65535);

  if (argc == 1)
    ret = ospf_if_resync_timeout_set (cli->vr->id, ifp->name, seconds);
  else
    {
      CLI_GET_IPV4_ADDRESS ("interface address", addr, argv[0]);
      ret = ospf_if_resync_timeout_set_by_addr (cli->vr->id, ifp->name,
                                                addr, seconds);
    }

  if (ret != OSPF_API_SET_SUCCESS)
    return ospf_cli_return (cli, ret);

  return CLI_SUCCESS;
}

CLI (no_ip_ospf_resync_timeout,
     no_ip_ospf_resync_timeout_cmd,
     "no ip ospf (A.B.C.D|) resync-timeout",
     CLI_NO_STR,
     OSPF_CLI_IF_STR,
     OSPF_CLI_IF_ADDR_STR,
     "Interval after which adjacency is reset if oob-resync is not started")
{
  struct interface *ifp = cli->index;
  struct pal_in4_addr addr;
  int ret = 0;

  if (argc == 0)
    ret = ospf_if_resync_timeout_unset (cli->vr->id, ifp->name);
  else
    {
      CLI_GET_IPV4_ADDRESS ("interface address", addr, argv[0]);
      ret = ospf_if_resync_timeout_unset_by_addr (cli->vr->id, ifp->name,
                                                  addr);
    }

  if (ret != OSPF_API_SET_SUCCESS)
    return ospf_cli_return (cli, ret);

  return CLI_SUCCESS;
}

#endif /* HAVE_RESTART */

#ifdef HAVE_OSPF_TE
CLI (te_metric,
     te_metric_cmd,
     "te-metric <1-65535>",
     "OSPF TE metric information",
     "Set TE Metric value")
{
  u_int32_t metric;
  int ret = 0;

  /* Metric range is <1-65535>. */
  CLI_GET_INTEGER_RANGE ("TE metric cost",
                         metric, argv[0], 1, 65535);

  if (cli->mode == INTERFACE_MODE)
    {
      struct interface *ifp = cli->index;

      ret = ospf_if_te_metric_set (cli->vr->id, ifp->name, metric);
      if (ret != OSPF_API_SET_SUCCESS)
        return ospf_cli_return (cli, ret);
    }
#ifdef HAVE_GMPLS
  else if (cli->mode == TELINK_MODE)
    {
      struct telink *tl = cli->index;

      ret = ospf_telink_te_metric_set (cli->vr->id, tl->name,
                                       metric);

      if (ret != OSPF_API_SET_SUCCESS)
        return ospf_cli_return (cli, ret);      
    }
#endif /* HAVE_GMPLS */

  return CLI_SUCCESS;
}

CLI (no_te_metric,
     no_te_metric_cmd,
     "no te-metric",
     CLI_NO_STR,
     "Unset TE Metric for the interface")
{
  int ret = 0;

  if (cli->mode == INTERFACE_MODE)
    {
      struct interface *ifp = cli->index;

      ret = ospf_if_te_metric_unset (cli->vr->id, ifp->name);
      if (ret != OSPF_API_SET_SUCCESS)
        return ospf_cli_return (cli, ret);
    }
#ifdef HAVE_GMPLS
  else if (cli->mode == TELINK_MODE)
    {
      struct telink *tl = cli->index;

      ret = ospf_telink_te_metric_unset (cli->vr->id, tl->name);

      if (ret != OSPF_API_SET_SUCCESS)
        return ospf_cli_return (cli, ret);      
    }
#endif /* HAVE_GMPLS */

  return CLI_SUCCESS;
}
#endif /* HAVE_OSPF_TE */

/* OSPF virtual-link related CLIs. */
CLI (area_virtual_link,
     area_virtual_link_cmd,
     "area (A.B.C.D|<0-4294967295>) virtual-link A.B.C.D",
     OSPF_CLI_AREA_STR,
     OSPF_CLI_VLINK_STR)
{
  struct ospf *top = cli->index;
  struct pal_in4_addr area_id, peer_id;
  char *auth_key = NULL;
#ifdef HAVE_MD5
  char *md5_key = NULL;
#endif /* HAVE_MD5 */
  int dead_interval = -1;
  int hello_interval = -1;
  int retransmit_interval = -1;
  int transmit_delay = -1;
  int auth_type = -1;
#ifdef HAVE_MD5
  int key_id = -1;
#endif /* HAVE_MD5 */
#ifdef HAVE_BFD
  int bfd_option = 0;
#endif /* HAVE_BFD */
  int fmt, i, ret;

  /* Get Area ID and Peer ID for virtual-link. */
  CLI_GET_OSPF_AREA_ID_NO_BB ("virtual-link transit", area_id, fmt, argv[0]);
  CLI_GET_IPV4_ADDRESS ("neighbor ID", peer_id, argv[1]);

  for (i = 2; i < argc;)
    {
      if (pal_strncmp (argv[i], "d", 1) == 0)
        {
          CLI_GET_INTEGER_RANGE ("Dead Interval", dead_interval,
                                 argv[i + 1], 1, 65535);
          i += 2;
        }
      else if (pal_strncmp (argv[i], "h", 1) == 0)
        {
          CLI_GET_INTEGER_RANGE ("Hello Interval", hello_interval,
                                 argv[i + 1], 1, 65535);
          i += 2;
        }
      else if (pal_strncmp (argv[i], "r", 1) == 0)
        {
          CLI_GET_INTEGER_RANGE ("Retransmit Interval", retransmit_interval,
                                 argv[i + 1], 1, OSPF_API_UpToMaxAge_MAX);
          i += 2;
        }
      else if (pal_strncmp (argv[i], "t", 1) == 0)
        {
          CLI_GET_INTEGER_RANGE ("Transmit Delay", transmit_delay,
                                 argv[i + 1], 1, OSPF_API_UpToMaxAge_MAX);
          i += 2;
        }
      else if (pal_strncmp (argv[i], "authentication-", 15) == 0)
        {
          auth_key = argv[i + 1];
          i += 2;
        }
      else if (pal_strncmp (argv[i], "m", 1) == 0)
        {
#ifdef HAVE_MD5
          CLI_GET_INTEGER_RANGE ("key ID", key_id, argv[i + 1], 1, 255);
          md5_key = argv[i + 3];
          i += 4;
#else /* HAVE_MD5 */
          cli_out (cli, "%% MD5 authentication is not supported "
                   "on this platform\n", argv[i]);
          return CLI_ERROR;
#endif /* HAVE_MD5 */
        }
      else if (pal_strncmp (argv[i], "authentication", 14) == 0)
        {
          if (argc > i + 1)
            {
              if (pal_strncmp (argv[i + 1], "message-digest", 14) == 0
                  && pal_strlen (argv[i + 1]) == 14)
                {
#ifdef HAVE_MD5
                  auth_type = OSPF_AUTH_CRYPTOGRAPHIC;
                  i += 2;
#else /* HAVE_MD5 */
                  cli_out (cli, "%% MD5 authentication is not supported "
                           "on this platform\n", argv[i]);
                  return CLI_ERROR;
#endif /* HAVE_MD5 */
                }
              else if (pal_strncmp (argv[i + 1], "n", 1) == 0)
                {
                  auth_type = OSPF_AUTH_NULL;
                  i += 2;
                }
              else
                {
                  auth_type = OSPF_AUTH_SIMPLE;
                  i++;
                }
            }
          else
            {
              auth_type = OSPF_AUTH_SIMPLE;
              i++;
            }
        }
#ifdef HAVE_BFD
      else if (pal_strncmp (argv[i], "fall-over", 9) == 0)
        {
          bfd_option = 1;
          i += 2;
        }
#endif /* HAVE_BFD */
    }

  ret = ospf_vlink_set (cli->vr->id, top->ospf_id, area_id, peer_id);
  if (ret != OSPF_API_SET_SUCCESS)
    return ospf_cli_return (cli, ret);

  ret = ospf_vlink_format_set (cli->vr->id, top->ospf_id, area_id, peer_id,
                               fmt);
  if (ret != OSPF_API_SET_SUCCESS)
    return ospf_cli_return (cli, ret);

  if (dead_interval > 0)
    {
      ret = ospf_vlink_dead_interval_set (cli->vr->id, top->ospf_id, area_id,
                                          peer_id, dead_interval);
      if (ret != OSPF_API_SET_SUCCESS)
        return ospf_cli_return (cli, ret);
    }

  if (hello_interval > 0)
    {
      ret = ospf_vlink_hello_interval_set (cli->vr->id, top->ospf_id, area_id,
                                           peer_id, hello_interval);
      if (ret != OSPF_API_SET_SUCCESS)
        return ospf_cli_return (cli, ret);
    }

  if (retransmit_interval > 0)
    {
      ret = ospf_vlink_retransmit_interval_set (cli->vr->id, top->ospf_id,
                                                area_id, peer_id,
                                                retransmit_interval);
      if (ret != OSPF_API_SET_SUCCESS)
        return ospf_cli_return (cli, ret);
    }

  if (transmit_delay > 0)
    {
      ret = ospf_vlink_transmit_delay_set (cli->vr->id, top->ospf_id,
                                           area_id, peer_id,
                                           transmit_delay);
      if (ret != OSPF_API_SET_SUCCESS)
        return ospf_cli_return (cli, ret);
    }

  if (auth_type >= 0)
    {
      ret = ospf_vlink_authentication_type_set (cli->vr->id, top->ospf_id,
                                                area_id, peer_id,
                                                auth_type);
      if (ret != OSPF_API_SET_SUCCESS)
        return ospf_cli_return (cli, ret);
    }

  if (auth_key > 0)
    {
      ret = ospf_vlink_authentication_key_set (cli->vr->id, top->ospf_id,
                                               area_id, peer_id,
                                               auth_key);
      if (ret != OSPF_API_SET_SUCCESS)
        return ospf_cli_return (cli, ret);
    }

#ifdef HAVE_MD5
  if (md5_key)
    {
      ret = ospf_vlink_message_digest_key_set (cli->vr->id, top->ospf_id,
                                               area_id, peer_id,
                                               key_id, md5_key);
      if (ret == OSPF_API_SET_ERR_MD5_KEY_EXIST)
        {
          cli_out (cli, "OSPF: Key %d already exists\n", key_id);
          return CLI_ERROR;
        }
    }
#endif /* HAVE_MD5 */

#ifdef HAVE_BFD
  if (bfd_option)
    {
      ret = ospf_vlink_bfd_set (cli->vr->id, top->ospf_id,
                                area_id, peer_id);
      if (ret == OSPF_API_SET_SUCCESS)
        return ospf_cli_return (cli, ret);
    }
#endif /* HAVE_BFD */

  return CLI_SUCCESS;
}

ALI (area_virtual_link,
     area_virtual_link_options_cmd,
     "area (A.B.C.D|<0-4294967295>) virtual-link A.B.C.D "
     "{authentication (message-digest|null|)"
     "|authentication-key LINE"
     "|message-digest-key <1-255> md5 LINE"
     "|dead-interval <1-65535>|hello-interval <1-65535>"
     "|retransmit-interval <1-3600>|transmit-delay <1-3600>}",
     OSPF_CLI_AREA_STR,
     OSPF_CLI_VLINK_STR,
     "Enable authentication",
     "Use message-digest authentication",
     "Use null authentication",
     "Set authentication key",
     "Authentication key (8 chars)",
     "Set message digest key",
     "Key ID",
     "Use MD5 algorithm",
     "Authentication key (16 chars)",
     "Dead router detection time",
     "Seconds",
     "Hello packet interval",
     "Seconds",
     "LSA retransmit interval",
     "Seconds",
     "LSA transmission delay",
     "Seconds");

CLI (no_area_virtual_link,
     no_area_virtual_link_cmd,
     "no area (A.B.C.D|<0-4294967295>) virtual-link A.B.C.D ",
     CLI_NO_STR,
     OSPF_CLI_AREA_STR,
     OSPF_CLI_VLINK_STR)
{
  struct ospf *top = cli->index;
  struct pal_in4_addr area_id, peer_id;
  int fmt;
  int ret = 0;

  /* Get Area ID and Peer ID for virtual-link. */
  CLI_GET_OSPF_AREA_ID_NO_BB ("virtual-link transit", area_id, fmt, argv[0]);
  CLI_GET_IPV4_ADDRESS ("neighbor ID", peer_id, argv[1]);

  ret = ospf_vlink_unset (cli->vr->id, top->ospf_id, area_id, peer_id);
  if ( ret != OSPF_API_SET_SUCCESS)
    return ospf_cli_return (cli, ret);

  return CLI_SUCCESS;
}

CLI (no_area_virtual_link_options,
     no_area_virtual_link_options_cmd,
     "no area (A.B.C.D|<0-4294967295>) virtual-link A.B.C.D "
     "{dead-interval|hello-interval|retransmit-interval|transmit-delay"
     "|authentication|authentication-key|message-digest-key <1-255>}",
     CLI_NO_STR,
     OSPF_CLI_AREA_STR,
     OSPF_CLI_VLINK_STR,
     "Dead router detection time",
     "Hello packet interval",
     "LSA retransmit interval",
     "LSA transmission delay",
     "Enable authentication",
     "Set authentication key",
     "Set message digest key",
     "Key ID")
{
  struct ospf *top = cli->index;
  struct pal_in4_addr area_id, peer_id;
#ifdef HAVE_MD5
  int key_id;
#endif /* HAVE_MD5 */
  int fmt, i;
  int ret = 0;

  /* Get Area ID and Peer ID for virtual-link. */
  CLI_GET_OSPF_AREA_ID_NO_BB ("virtual-link transit", area_id, fmt, argv[0]);
  CLI_GET_IPV4_ADDRESS ("neighbor ID", peer_id, argv[1]);

  for (i = 2; i < argc;)
    {
      if (pal_strncmp (argv[i], "d", 1) == 0)
        {
          ret = ospf_vlink_dead_interval_unset (cli->vr->id, top->ospf_id,
                                                area_id, peer_id);
          i++;
        }
      else if (pal_strncmp (argv[i], "h", 1) == 0)
        {
          ret = ospf_vlink_hello_interval_unset (cli->vr->id, top->ospf_id,
                                                 area_id, peer_id);
          i++;
        }
      else if (pal_strncmp (argv[i], "r", 1) == 0)
        {
          ret = ospf_vlink_retransmit_interval_unset (cli->vr->id, top->ospf_id,
                                                      area_id, peer_id);
          i++;
        }
      else if (pal_strncmp (argv[i], "t", 1) == 0)
        {
          ret = ospf_vlink_transmit_delay_unset (cli->vr->id, top->ospf_id,
                                                 area_id, peer_id);
          i++;
        }
      else if (pal_strncmp (argv[i], "authentication-", 15) == 0)
        {
          ret = ospf_vlink_authentication_key_unset (cli->vr->id, top->ospf_id,
                                                     area_id, peer_id);
          i++;
        }
      else if (pal_strncmp (argv[i], "a", 1) == 0)
        {
          ret = ospf_vlink_authentication_type_unset (cli->vr->id, top->ospf_id,
                                                      area_id, peer_id);
          i++;
        }
      else if (pal_strncmp (argv[i], "m", 1) == 0)
        {
#ifdef HAVE_MD5
          int ret;

          /* Get Key ID. */
          CLI_GET_INTEGER_RANGE ("key ID", key_id, argv[i + 1], 1, 255);
          ret = ospf_vlink_message_digest_key_unset (cli->vr->id, top->ospf_id,
                                                     area_id, peer_id, key_id);
          if (ret == OSPF_API_SET_ERR_MD5_KEY_NOT_EXIST)
            {
              cli_out (cli, "OSPF: Key %d does not exist\n", key_id);
              return CLI_ERROR;
            }
#endif /* HAVE_MD5 */
          i += 2;
        }
#ifdef HAVE_BFD
      else if (pal_strncmp (argv[i], "fall-over", 9) == 0)
        {
          ret = ospf_vlink_bfd_unset (cli->vr->id, top->ospf_id,
                                      area_id, peer_id);
          i += 2;
        }
#endif /* HAVE_BFD */

      if (ret != OSPF_API_SET_SUCCESS)
        return ospf_cli_return (cli, ret);
    }

  return CLI_SUCCESS;
}

/* OSPF area range relatede CLIs. */
CLI (area_range,
     area_range_cmd,
     "area (A.B.C.D|<0-4294967295>) range A.B.C.D/M",
     OSPF_CLI_AREA_STR,
     "Summarize routes matching address/mask (border routers only)",
     "area range prefix")
{
  struct ospf *top = cli->index;
  struct pal_in4_addr area_id;
  struct prefix_ipv4 p;
  int format;
  int ret;

  CLI_GET_OSPF_AREA_ID (area_id, format, argv[0]);
  CLI_GET_IPV4_PREFIX ("area range", p, argv[1]);

  ret = ospf_area_range_set (cli->vr->id, top->ospf_id,
                             area_id, p.prefix, p.prefixlen);
  if (ret != OSPF_API_SET_SUCCESS)
    return ospf_cli_return (cli, ret);

  ret = ospf_area_format_set (cli->vr->id, top->ospf_id, area_id, format);

  if (ret != OSPF_API_SET_SUCCESS)
    return ospf_cli_return (cli, ret);

  return CLI_SUCCESS;
}

ALI (area_range,
     area_range_advertise_cmd,
     "area (A.B.C.D|<0-4294967295>) range A.B.C.D/M advertise",
     OSPF_CLI_AREA_STR,
     "Summarize routes matching address/mask (border routers only)",
     "area range prefix",
     "Advertise this range (default)");

CLI (area_range_not_advertise,
     area_range_not_advertise_cmd,
     "area (A.B.C.D|<0-4294967295>) range A.B.C.D/M not-advertise",
     OSPF_CLI_AREA_STR,
     "Summarize routes matching address/mask (border routers only)",
     "area range prefix",
     "DoNotAdvertise this range")
{
  struct ospf *top = cli->index;
  struct pal_in4_addr area_id;
  struct prefix_ipv4 p;
  int format;
  int ret;

  CLI_GET_OSPF_AREA_ID (area_id, format, argv[0]);
  CLI_GET_IPV4_PREFIX ("area range", p, argv[1]);

  ret = ospf_area_range_set (cli->vr->id, top->ospf_id,
                             area_id, p.prefix, p.prefixlen);
  if (ret != OSPF_API_SET_SUCCESS)
    return ospf_cli_return (cli, ret);

  ret = ospf_area_range_not_advertise_set (cli->vr->id, top->ospf_id, area_id,
                                           p.prefix, p.prefixlen);
  if (ret != OSPF_API_SET_SUCCESS)
    return ospf_cli_return (cli, ret);

  ret = ospf_area_format_set (cli->vr->id, top->ospf_id, area_id, format);
  if (ret != OSPF_API_SET_SUCCESS)
    return ospf_cli_return (cli, ret);

  return CLI_SUCCESS;
}

CLI (no_area_range,
     no_area_range_cmd,
     "no area (A.B.C.D|<0-4294967295>) range A.B.C.D/M",
     CLI_NO_STR,
     OSPF_CLI_AREA_STR,
     "Summarize routes matching address/mask (border routers only)",
     "area range prefix")
{
  struct ospf *top = cli->index;
  struct pal_in4_addr area_id;
  struct prefix_ipv4 p;
  int ret, format;

  CLI_GET_OSPF_AREA_ID (area_id, format, argv[0]);
  CLI_GET_IPV4_PREFIX ("area range", p, argv[1]);

  ret = ospf_area_range_unset (cli->vr->id, top->ospf_id, area_id,
                               p.prefix, p.prefixlen);

  if (ret != OSPF_API_SET_SUCCESS)
    return ospf_cli_return (cli, ret);

  return ospf_cli_return (cli, ret);
}

ALI (no_area_range,
     no_area_range_advertise_cmd,
     "no area (A.B.C.D|<0-4294967295>) range A.B.C.D/M (advertise|not-advertise)",
     CLI_NO_STR,
     OSPF_CLI_AREA_STR,
     "Summarize routes matching address/mask (border routers only)",
     "area range prefix",
     "Advertise this range (default)",
     "DoNotAdvertise this range");

CLI (area_range_substitute,
     area_range_substitute_cmd,
     "area (A.B.C.D|<0-4294967295>) range A.B.C.D/M substitute A.B.C.D/M",
     OSPF_CLI_AREA_STR,
     "Summarize routes matching address/mask (border routers only)",
     "area range prefix",
     "Announce area range as another prefix",
     "Network prefix to be announced instead of range")
{
  struct ospf *top = cli->index;
  struct pal_in4_addr area_id;
  struct prefix_ipv4 p, s;
  int format;
  int ret = 0;

  CLI_GET_OSPF_AREA_ID (area_id, format, argv[0]);
  CLI_GET_IPV4_PREFIX ("area range", p, argv[1]);
  CLI_GET_IPV4_PREFIX ("substituted network prefix", s, argv[2]);

  ret = ospf_area_range_substitute_set (cli->vr->id, top->ospf_id, area_id,
                                        p.prefix, p.prefixlen,
                                        s.prefix, s.prefixlen);
  if (ret != OSPF_API_SET_SUCCESS)
    return ospf_cli_return (cli, ret);

  ret = ospf_area_format_set (cli->vr->id, top->ospf_id, area_id, format);
  if (ret != OSPF_API_SET_SUCCESS)
    return ospf_cli_return (cli, ret);

  return CLI_SUCCESS;
}
  
CLI (no_area_range_substitute,
     no_area_range_substitute_cmd,
     "no area (A.B.C.D|<0-4294967295>) range A.B.C.D/M substitute",
     CLI_NO_STR,
     OSPF_CLI_AREA_STR,
     "Summarize routes matching address/mask (border routers only)",
     "area range prefix",
     "Announce area range as another prefix")
{
  struct ospf *top = cli->index;
  struct pal_in4_addr area_id;
  struct prefix_ipv4 p;
  int ret, format;

  CLI_GET_OSPF_AREA_ID (area_id, format, argv[0]);
  CLI_GET_IPV4_PREFIX ("area range", p, argv[1]);

  ret = ospf_area_range_substitute_unset (cli->vr->id, top->ospf_id, area_id,
                                          p.prefix, p.prefixlen);
  if (ret != OSPF_API_SET_SUCCESS)
    return ospf_cli_return (cli, ret);

  return ospf_cli_return (cli, ret);
}

CLI (area_authentication,
     area_authentication_cmd,
     "area (A.B.C.D|<0-4294967295>) authentication",
     OSPF_CLI_AREA_STR,
     "Enable authentication")
{
  struct ospf *top = cli->index;
  struct pal_in4_addr area_id;
  int format;
  int ret;

  CLI_GET_OSPF_AREA_ID (area_id, format, argv[0]);

  ret = ospf_area_auth_type_set (cli->vr->id, top->ospf_id,
                                 area_id, OSPF_AREA_AUTH_SIMPLE);
  if (ret != OSPF_API_SET_SUCCESS)
    return ospf_cli_return (cli, ret);

  ret = ospf_area_format_set (cli->vr->id, top->ospf_id, area_id, format);
  if (ret != OSPF_API_SET_SUCCESS)
    return ospf_cli_return (cli, ret);

  return CLI_SUCCESS;
}

CLI (area_authentication_message_digest,
     area_authentication_message_digest_cmd,
     "area (A.B.C.D|<0-4294967295>) authentication message-digest",
     OSPF_CLI_AREA_STR,
     "Enable authentication",
     "Use message-digest authentication")
{
  struct ospf *top = cli->index;
  struct pal_in4_addr area_id;
  int format;
  int ret;

  CLI_GET_OSPF_AREA_ID (area_id, format, argv[0]);

  ret = ospf_area_auth_type_set (cli->vr->id, top->ospf_id, area_id,
                                 OSPF_AREA_AUTH_CRYPTOGRAPHIC);
  if (ret != OSPF_API_SET_SUCCESS)
    return ospf_cli_return (cli, ret);

  ret = ospf_area_format_set (cli->vr->id, top->ospf_id, area_id, format);
  if (ret != OSPF_API_SET_SUCCESS)
    return ospf_cli_return (cli, ret);

  return CLI_SUCCESS;
}

CLI (no_area_authentication,
     no_area_authentication_cmd,
     "no area (A.B.C.D|<0-4294967295>) authentication",
     CLI_NO_STR,
     OSPF_CLI_AREA_STR,
     "Enable authentication")
{
  struct ospf *top = cli->index;
  struct pal_in4_addr area_id;
  int ret, format;

  CLI_GET_OSPF_AREA_ID (area_id, format, argv[0]);

  ret = ospf_area_auth_type_unset (cli->vr->id, top->ospf_id, area_id);

  if (ret != OSPF_API_SET_SUCCESS)
    return ospf_cli_return (cli, ret);

  return ospf_cli_return (cli, ret);
}


CLI (area_default_cost,
     area_default_cost_cmd,
     "area (A.B.C.D|<0-4294967295>) default-cost <0-16777215>",
     OSPF_CLI_AREA_STR,
     "Set the summary-default cost of a NSSA or stub area",
     "Stub's advertised default summary cost")
{
  struct ospf *top = cli->index;
  struct pal_in4_addr area_id;
  int ret, format;
  u_int32_t cost;

  CLI_GET_OSPF_AREA_ID_NO_BB ("default-cost", area_id, format, argv[0]);
  CLI_GET_UINT32_RANGE ("stub default cost", cost, argv[1],
                        OSPF_STUB_DEFAULT_COST_MIN,
                        OSPF_STUB_DEFAULT_COST_MAX);

  ret = ospf_area_default_cost_set (cli->vr->id, top->ospf_id, area_id, cost);
  if (ret != OSPF_API_SET_SUCCESS)
    return ospf_cli_return (cli, ret);

  ret = ospf_area_format_set (cli->vr->id, top->ospf_id, area_id, format);
  if (ret != OSPF_API_SET_SUCCESS)
    return ospf_cli_return (cli, ret);

  return CLI_SUCCESS;
}

CLI (no_area_default_cost,
     no_area_default_cost_cmd,
     "no area (A.B.C.D|<0-4294967295>) default-cost",
     CLI_NO_STR,
     OSPF_CLI_AREA_STR,
     "Set the summary-default cost of a NSSA or stub area")
{
  struct ospf *top = cli->index;
  struct pal_in4_addr area_id;
  int ret, format;

  CLI_GET_OSPF_AREA_ID_NO_BB ("default-cost", area_id, format, argv[0]);

  ret = ospf_area_default_cost_unset (cli->vr->id, top->ospf_id, area_id);
  if (ret != OSPF_API_SET_SUCCESS)
    return ospf_cli_return (cli, ret);

  return ospf_cli_return (cli, ret);
}


CLI (area_filter_list_prefix,
     area_filter_list_prefix_cmd,
     "area (A.B.C.D|<0-4294967295>) filter-list prefix WORD (in|out)",
     OSPF_CLI_AREA_STR,
     "Filter networks between OSPF areas",
     "Filter networks by prefix-list",
     "Name of an IP prefix-list",
     "Filter networks sent to this area",
     "Filter networks sent from this area")
{
  struct ospf *top = cli->index;
  struct pal_in4_addr area_id;
  int format;
  int type;
  int ret;

  CLI_GET_OSPF_AREA_ID (area_id, format, argv[0]);

  if (!ospf_cli_str2filter_type (argv[2], &type))
    return CLI_ERROR;

  ret = ospf_area_filter_list_prefix_set (cli->vr->id, top->ospf_id,
                                          area_id, type, argv[1]);
  if (ret != OSPF_API_SET_SUCCESS)
    return ospf_cli_return (cli, ret);

  ret = ospf_area_format_set (cli->vr->id, top->ospf_id, area_id, format);
  if (ret != OSPF_API_SET_SUCCESS)
    return ospf_cli_return (cli, ret);

  return CLI_SUCCESS;
}

CLI (no_area_filter_list_prefix,
     no_area_filter_list_prefix_cmd,
     "no area (A.B.C.D|<0-4294967295>) filter-list prefix WORD (in|out)",
     CLI_NO_STR,
     OSPF_CLI_AREA_STR,
     "Filter networks between OSPF areas",
     "Filter networks by prefix-list",
     "Name of an IP prefix-list",
     "Filter networks sent to this area",
     "Filter networks sent from this area")
{
  struct ospf *top = cli->index;
  struct pal_in4_addr area_id;
  int format;
  int type;
  int ret;

  CLI_GET_OSPF_AREA_ID (area_id, format, argv[0]);

  if (!ospf_cli_str2filter_type (argv[2], &type))
    return CLI_ERROR;

  ret = ospf_area_filter_list_prefix_unset (cli->vr->id, top->ospf_id,
                                            area_id, type);
  if (ret != OSPF_API_SET_SUCCESS)
    return ospf_cli_return (cli, ret);

  return ospf_cli_return (cli, ret);
}

CLI (area_filter_list_access,
     area_filter_list_access_cmd,
     "area (A.B.C.D|<0-4294967295>) filter-list access WORD (in|out)",
     OSPF_CLI_AREA_STR,
     "Filter networks between OSPF areas",
     "Filter networks by access-list",
     "Name of an access-list",
     "Filter networks sent to this area",
     "Filter networks sent from this area")
{
  struct ospf *top = cli->index;
  struct pal_in4_addr area_id;
  int format;
  int type;
  int ret;

  CLI_GET_OSPF_AREA_ID (area_id, format, argv[0]);

  if (!ospf_cli_str2filter_type (argv[2], &type))
    return CLI_ERROR;

  ret = ospf_area_filter_list_access_set (cli->vr->id, top->ospf_id,
                                          area_id, type, argv[1]);
  if (ret != OSPF_API_SET_SUCCESS)
    return ospf_cli_return (cli, ret);

  ret = ospf_area_format_set (cli->vr->id, top->ospf_id, area_id, format);
  if (ret != OSPF_API_SET_SUCCESS)
    return ospf_cli_return (cli, ret);

  return CLI_SUCCESS;
}

CLI (no_area_filter_list_access,
     no_area_filter_list_access_cmd,
     "no area (A.B.C.D|<0-4294967295>) filter-list access WORD (in|out)",
     CLI_NO_STR,
     OSPF_CLI_AREA_STR,
     "Filter networks between OSPF areas",
     "Filter networks by access-list",
     "Name of an access-list",
     "Filter networks sent to this area",
     "Filter networks sent from this area")
{
  struct ospf *top = cli->index;
  struct pal_in4_addr area_id;
  int format;
  int type;
  int ret;

  CLI_GET_OSPF_AREA_ID (area_id, format, argv[0]);

  if (!ospf_cli_str2filter_type (argv[2], &type))
    return CLI_ERROR;

  ret = ospf_area_filter_list_access_unset (cli->vr->id, top->ospf_id,
                                            area_id, type);
  if (ret != OSPF_API_SET_SUCCESS)
    return ospf_cli_return (cli, ret);

  return ospf_cli_return (cli, ret);
}

CLI (area_export_list,
     area_export_list_cmd,
     "area (A.B.C.D|<0-4294967295>) export-list NAME",
     OSPF_CLI_AREA_STR,
     "Set the filter for networks announced to other areas",
     "Name of the access-list")
{
  struct ospf *top = cli->index;
  struct pal_in4_addr area_id;
  int format;
  int ret = 0;

  CLI_GET_OSPF_AREA_ID (area_id, format, argv[0]);

  ret = ospf_area_export_list_set (cli->vr->id, top->ospf_id, area_id,
                                   argv[1]);
  if (ret != OSPF_API_SET_SUCCESS)
    return ospf_cli_return (cli, ret);

  ret = ospf_area_format_set (cli->vr->id, top->ospf_id, area_id, format);
  if (ret != OSPF_API_SET_SUCCESS)
    return ospf_cli_return (cli, ret);

  return CLI_SUCCESS;
}


CLI (no_area_export_list,
     no_area_export_list_cmd,
     "no area (A.B.C.D|<0-4294967295>) export-list",
     CLI_NO_STR,
     OSPF_CLI_AREA_STR,
     "Unset the filter for networks announced to other areas")
{
  struct ospf *top = cli->index;
  struct pal_in4_addr area_id;
  int format;
  int ret = 0;

  CLI_GET_OSPF_AREA_ID (area_id, format, argv[0]);

  ret = ospf_area_export_list_unset (cli->vr->id, top->ospf_id, area_id);
  if (ret != OSPF_API_SET_SUCCESS)
    return ospf_cli_return (cli, ret);

  return CLI_SUCCESS;
}

CLI (area_import_list,
     area_import_list_cmd,
     "area (A.B.C.D|<0-4294967295>) import-list NAME",
     OSPF_CLI_AREA_STR,
     "Set the filter for networks from other areas announced to the specified one",
     "Name of the access-list")
{
  struct ospf *top = cli->index;
  struct pal_in4_addr area_id;
  int format;
  int ret = 0;

  CLI_GET_OSPF_AREA_ID (area_id, format, argv[0]);

  ret = ospf_area_import_list_set (cli->vr->id, top->ospf_id, area_id,
                                   argv[1]);
  if (ret != OSPF_API_SET_SUCCESS)
    return ospf_cli_return (cli, ret);

  ret =ospf_area_format_set (cli->vr->id, top->ospf_id, area_id, format);
  if (ret != OSPF_API_SET_SUCCESS)
    return ospf_cli_return (cli, ret);

  return CLI_SUCCESS;
}

CLI (no_area_import_list,
     no_area_import_list_cmd,
     "no area (A.B.C.D|<0-4294967295>) import-list",
     CLI_NO_STR,
     OSPF_CLI_AREA_STR,
     "Unset the filter for networks announced to other areas")
{
  struct ospf *top = cli->index;
  struct pal_in4_addr area_id;
  int format;
  int ret = 0;

  CLI_GET_OSPF_AREA_ID (area_id, format, argv[0]);
  ret = ospf_area_import_list_unset (cli->vr->id, top->ospf_id, area_id);

  if (ret != OSPF_API_SET_SUCCESS)
    return ospf_cli_return (cli, ret);

  return CLI_SUCCESS;
}


CLI (area_shortcut,
     area_shortcut_cmd,
     "area (A.B.C.D|<0-4294967295>) shortcut (default|enable|disable)",
     OSPF_CLI_AREA_STR,
     "Configure the area's shortcutting mode",
     "Set default shortcutting behavior",
     "Enable shortcutting through the area",
     "Disable shortcutting through the area")
{
  struct ospf *top = cli->index;
  struct pal_in4_addr area_id;
  int mode, format;
  int ret = 0;

  CLI_GET_OSPF_AREA_ID_NO_BB ("shortcut", area_id, format, argv[0]);

  if (pal_strncmp (argv[1], "de", 2) == 0)
    mode = OSPF_SHORTCUT_DEFAULT;
  else if (pal_strncmp (argv[1], "di", 2) == 0)
    mode = OSPF_SHORTCUT_DISABLE;
  else if (pal_strncmp (argv[1], "e", 1) == 0)
    mode = OSPF_SHORTCUT_ENABLE;
  else
    return CLI_ERROR;

  ret = ospf_area_shortcut_set (cli->vr->id, top->ospf_id, area_id, mode);
  if (ret != OSPF_API_SET_SUCCESS)
    return ospf_cli_return (cli, ret);

  ret = ospf_area_format_set (cli->vr->id, top->ospf_id, area_id, format);
  if (ret != OSPF_API_SET_SUCCESS)
    return ospf_cli_return (cli, ret);

  if (top->abr_type != OSPF_ABR_TYPE_SHORTCUT)
    cli_out (cli, "Shortcut area setting will take effect only "
             "when the router is configured as Shortcut ABR\n");

  return CLI_SUCCESS;
}

CLI (no_area_shortcut,
     no_area_shortcut_args_cmd,
     "no area (A.B.C.D|<0-4294967295>) shortcut (enable|disable)",
     CLI_NO_STR,
     OSPF_CLI_AREA_STR,
     "Deconfigure the area's shortcutting mode",
     "Deconfigure enabled shortcutting through the area",
     "Deconfigure disabled shortcutting through the area")
{
  struct ospf *top = cli->index;
  struct pal_in4_addr area_id;
  int format;
  int ret = 0;

  CLI_GET_OSPF_AREA_ID_NO_BB ("shortcut", area_id, format, argv[0]);
  ret = ospf_area_shortcut_unset (cli->vr->id, top->ospf_id, area_id);
  if (ret != OSPF_API_SET_SUCCESS)
    return ospf_cli_return (cli, ret);

  return CLI_SUCCESS;
}

ALI (no_area_shortcut,
     no_area_shortcut_cmd,
     "no area (A.B.C.D|<0-4294967295>) shortcut",
     CLI_NO_STR,
     OSPF_CLI_AREA_STR,
     "Deconfigure the area's shortcutting mode");


CLI (area_stub,
     area_stub_cmd,
     "area (A.B.C.D|<0-4294967295>) stub",
     OSPF_CLI_AREA_STR,
     "Configure OSPF area as stub")
{
  struct ospf *top = cli->index;
  struct pal_in4_addr area_id;
  int ret, format;

  CLI_GET_OSPF_AREA_ID_NO_BB ("stub", area_id, format, argv[0]);

  ret = ospf_area_stub_set (cli->vr->id, top->ospf_id, area_id);
  if (ret != OSPF_API_SET_SUCCESS)
    return ospf_cli_return (cli, ret);

  ret = ospf_area_format_set (cli->vr->id, top->ospf_id, area_id, format);
  if (ret != OSPF_API_SET_SUCCESS)
    return ospf_cli_return (cli, ret);

  return CLI_SUCCESS;
}

CLI (area_stub_no_summary,
     area_stub_no_summary_cmd,
     "area (A.B.C.D|<0-4294967295>) stub no-summary",
     OSPF_CLI_AREA_STR,
     "Configure OSPF area as stub",
     "Do not inject inter-area routes into stub")
{
  struct ospf *top = cli->index;
  struct pal_in4_addr area_id;
  int ret, format;

  CLI_GET_OSPF_AREA_ID_NO_BB ("stub", area_id, format, argv[0]);

  ret = ospf_area_stub_set (cli->vr->id, top->ospf_id, area_id);
  if (ret != OSPF_API_SET_SUCCESS)
    return ospf_cli_return (cli, ret);

  ret = ospf_area_format_set (cli->vr->id, top->ospf_id, area_id, format);
  if (ret != OSPF_API_SET_SUCCESS)
    return ospf_cli_return (cli, ret);

  ret = ospf_area_no_summary_set (cli->vr->id, top->ospf_id, area_id);
  if (ret != OSPF_API_SET_SUCCESS)
    return ospf_cli_return (cli, ret);

  return CLI_SUCCESS;
}

CLI (no_area_stub,
     no_area_stub_cmd,
     "no area (A.B.C.D|<0-4294967295>) stub",
     CLI_NO_STR,
     OSPF_CLI_AREA_STR,
     "Configure OSPF area as stub")
{
  struct ospf *top = cli->index;
  struct pal_in4_addr area_id;
  int format;
  int ret;

  CLI_GET_OSPF_AREA_ID_NO_BB ("stub", area_id, format, argv[0]);

  ret = ospf_area_stub_unset (cli->vr->id, top->ospf_id, area_id);
  if (ret != OSPF_API_SET_SUCCESS)
    return ospf_cli_return (cli, ret);

  return ospf_cli_return (cli, ret);
}

CLI (no_area_stub_no_summary,
     no_area_stub_no_summary_cmd,
     "no area (A.B.C.D|<0-4294967295>) stub no-summary",
     CLI_NO_STR,
     OSPF_CLI_AREA_STR,
     "Configure OSPF area as stub",
     "Do not inject inter-area routes into area")
{
  struct ospf *top = cli->index;
  struct pal_in4_addr area_id;
  int format;
  int ret;

  CLI_GET_OSPF_AREA_ID_NO_BB ("stub", area_id, format, argv[0]);

  ret = ospf_area_no_summary_unset (cli->vr->id, top->ospf_id, area_id);

  return ospf_cli_return (cli, ret);
}

#ifdef HAVE_NSSA
CLI (area_nssa,
     area_nssa_cmd,
     "area (A.B.C.D|<0-4294967295>) nssa",
     OSPF_CLI_AREA_STR,
     OSPF_CLI_NSSA_STR)
{
  struct ospf *top = cli->index;
  struct pal_in4_addr area_id;
  int mtype;
  int metric;
  int ret, format;
  int i;

  CLI_GET_OSPF_AREA_ID_NO_BB ("NSSA", area_id, format, argv[0]);

  ret = ospf_area_nssa_set (cli->vr->id, top->ospf_id, area_id);
  if (ret != OSPF_API_SET_SUCCESS)
    return ospf_cli_return (cli, ret);

  ret = ospf_area_format_set (cli->vr->id, top->ospf_id, area_id, format);
  if (ret != OSPF_API_SET_SUCCESS)
    return ospf_cli_return (cli, ret);

  for (i = 1; i < argc;)
    {
      if (pal_strncmp (argv[i], "t", 1) == 0)
        {
          if (pal_strncmp (argv[i + 1], "c", 1) == 0)
            ret = ospf_area_nssa_translator_role_set (cli->vr->id,
                                              top->ospf_id, area_id,
                                              OSPF_NSSA_TRANSLATE_CANDIDATE);
          else if (pal_strncmp (argv[i + 1], "n", 1) == 0)
            ret = ospf_area_nssa_translator_role_set (cli->vr->id,
                                              top->ospf_id, area_id,
                                              OSPF_NSSA_TRANSLATE_NEVER);
          else if (pal_strncmp (argv[i + 1], "a", 1) == 0)
            ret = ospf_area_nssa_translator_role_set (cli->vr->id,
                                              top->ospf_id, area_id,
                                              OSPF_NSSA_TRANSLATE_ALWAYS);
          i += 2;
        }
      else if (pal_strncmp (argv[i], "no-r", 4) == 0)
        {
          ret = ospf_area_nssa_no_redistribution_set (cli->vr->id,
                                                      top->ospf_id,
                                                      area_id);
          i++;
        }
      else if (pal_strncmp (argv[i], "d", 1) == 0)
        {
          ret = ospf_area_nssa_default_originate_set (cli->vr->id,
                                                      top->ospf_id, area_id);
          i++;

          for (; i < argc;)
            {
              if (pal_strncmp (argv[i], "m", 1) != 0)
                break;

              if (pal_strlen (argv[i]) == 6)
                {
                  CLI_GET_INTEGER_RANGE ("Metric", metric,
                                         argv[i + 1], 0, 16777214);
                  ret = ospf_area_nssa_default_originate_metric_set (
                                                               cli->vr->id,
                                                               top->ospf_id,
                                                               area_id,
                                                               metric);
                  i += 2;
                }
              else
                {
                  CLI_GET_INTEGER_RANGE ("Metric Type", mtype,
                                         argv[i + 1], 1, 2);
                  ret = ospf_area_nssa_default_originate_metric_type_set (
                                                               cli->vr->id,
                                                               top->ospf_id,
                                                               area_id,
                                                               mtype);
                  i += 2;
                }

              if (ret != OSPF_API_SET_SUCCESS)
                return ospf_cli_return (cli, ret);
            }
        }
      else if (pal_strncmp (argv[i], "no-s", 4) == 0)
        {
          ret = ospf_area_no_summary_set (cli->vr->id, top->ospf_id, area_id);
          i++;
        }

     if (ret != OSPF_API_SET_SUCCESS)
       return ospf_cli_return (cli, ret);
    }

  return CLI_SUCCESS;
}

ALI (area_nssa,
     area_nssa_options_cmd,
     "area (A.B.C.D|<0-4294967295>) nssa "
     "{translator-role (candidate|never|always)"
     "|no-redistribution"
     "|default-information-originate (metric <0-16777214>|metric-type <1-2>|metric <0-16777214> metric-type <1-2>|metric-type <1-2> metric <0-16777214>|)"
     "|no-summary}",
     OSPF_CLI_AREA_STR,
     OSPF_CLI_NSSA_STR,
     "NSSA-ABR Translator role",
     "Candidate for translator (default)",
     "Do not translate",
     "Translate always",
     "No redistribution into this NSSA area",
     "Originate Type 7 default into NSSA area",
     "OSPF default metric",
     "OSPF metric",
     "OSPF metric type for default routes",
     "OSPF Link State type",
     "OSPF default metric",
     "OSPF metric",
     "OSPF metric type for default routes",
     "OSPF Link State type",
     "OSPF metric type for default routes",
     "OSPF Link State type",
     "OSPF default metric",
     "OSPF metric",
     "Do not send summary LSA into NSSA");

#if 0
ALI (area_nssa,
     area_nssa_options_cmd,
     "area (A.B.C.D|<0-4294967295>) nssa "
     "{translator-role (candidate|never|always)"
     "|no-redistribution"
     "|default-information-originate {metric <0-16777214>|metric-type <1-2>}"
     "|no-summary}",
     OSPF_CLI_AREA_STR,
     OSPF_CLI_NSSA_STR,
     "NSSA-ABR Translator role",
     "Candidate for translator (default)",
     "Do not translate",
     "Translate always",
     "No redistribution into this NSSA area",
     "Originate Type 7 default into NSSA area",
     "OSPF default metric",
     "OSPF metric",
     "OSPF metric type for default routes",
     "OSPF Link State type",
     "Do not send summary LSA into NSSA");
#endif

CLI (area_nssa_translator_role,
     area_nssa_translator_role_cmd,
     "area (A.B.C.D|<0-4294967295>) nssa "
     "(translate-candidate|translate-never|translate-always)",
     OSPF_CLI_AREA_STR,
     OSPF_CLI_NSSA_STR,
     "Configure NSSA-ABR for translate election (default)",
     "Configure NSSA-ABR to never translate",
     "Configure NSSA-ABR to always translate")
{
  struct ospf *top = cli->index;
  struct pal_in4_addr area_id;
  int ret, format;
  int role;

  CLI_GET_OSPF_AREA_ID_NO_BB ("NSSA", area_id, format, argv[0]);

  ret = ospf_area_nssa_set (cli->vr->id, top->ospf_id, area_id);
  if (ret != OSPF_API_SET_SUCCESS)
    return ospf_cli_return (cli, ret);

  ret = ospf_area_format_set (cli->vr->id, top->ospf_id, area_id, format);
  if (ret != OSPF_API_SET_SUCCESS)
    return ospf_cli_return (cli, ret);

  if (pal_strncmp (argv[1], "translate-n", 11) == 0)
    role = OSPF_NSSA_TRANSLATE_NEVER;
  else if (pal_strncmp (argv[1], "translate-a", 11) == 0)
    role = OSPF_NSSA_TRANSLATE_ALWAYS;
  else
    role = OSPF_NSSA_TRANSLATE_CANDIDATE;

  ret = ospf_area_nssa_translator_role_set (cli->vr->id, top->ospf_id,
                                            area_id, role);
  if (ret != OSPF_API_SET_SUCCESS)
    return ospf_cli_return (cli, ret);

  return CLI_SUCCESS;
}

CLI (no_area_nssa,
     no_area_nssa_cmd,
     "no area (A.B.C.D|<0-4294967295>) nssa",
     CLI_NO_STR,
     OSPF_CLI_AREA_STR,
     OSPF_CLI_NSSA_STR)
{
  struct ospf *top = cli->index;
  struct pal_in4_addr area_id;
  int ret, format;

  CLI_GET_OSPF_AREA_ID_NO_BB ("NSSA", area_id, format, argv[0]);

  ret = ospf_area_nssa_unset (cli->vr->id, top->ospf_id, area_id);
  if (ret != OSPF_API_SET_SUCCESS)
    return ospf_cli_return (cli, ret);

  return CLI_SUCCESS;
}

CLI (no_area_nssa_options,
     no_area_nssa_options_cmd,
     "no area (A.B.C.D|<0-4294967295>) nssa "
     "{translator-role|no-redistribution"
     "|default-information-originate|no-summary}",
     CLI_NO_STR,
     OSPF_CLI_AREA_STR,
     OSPF_CLI_NSSA_STR,
     "NSSA-ABR Translator role",
     "No redistribution into this NSSA area",
     "Originate Type 7 default into NSSA area",
     "Do not send summary LSA into NSSA")
{
  struct ospf *top = cli->index;
  struct pal_in4_addr area_id;
  int format;
  int i;
  int ret = 0;

  CLI_GET_OSPF_AREA_ID_NO_BB ("NSSA", area_id, format, argv[0]);

  for (i = 1; i < argc; i++)
    {
      if (pal_strncmp (argv[i], "t", 1) == 0)
        ret = ospf_area_nssa_translator_role_unset (cli->vr->id, top->ospf_id,
                                                    area_id);
      else if (pal_strncmp (argv[i], "no-r", 4) == 0)
        ret = ospf_area_nssa_no_redistribution_unset (cli->vr->id,
                                                      top->ospf_id, area_id);
      else if (pal_strncmp (argv[i], "d", 1) == 0)
        ret = ospf_area_nssa_default_originate_unset (cli->vr->id,
                                                      top->ospf_id, area_id);
      else if (pal_strncmp (argv[i], "no-s", 4) == 0)
        ret = ospf_area_no_summary_unset (cli->vr->id, top->ospf_id, area_id);

      if (ret != OSPF_API_SET_SUCCESS)
        return ospf_cli_return (cli, ret);
    }

  return CLI_SUCCESS;
}
#endif /* HAVE_NSSA */
#ifdef HAVE_OSPF_MULTI_AREA
CLI (area_multi_area_adjacency,
     area_multi_area_adjacency_cmd,
     "area (A.B.C.D|<0-4294967295>)"
     "multi-area-adjacency IFNAME neighbor A.B.C.D",
     OSPF_CLI_AREA_STR,
     "Set the multi-area-adjacency",
     "Interface name",
     "Set the neighbor",
     "Neighbor ip address")
{
  struct ospf *top = cli->index;
  struct pal_in4_addr area_id;
  s_int32_t format;
  s_int32_t ret;
  u_int8_t *ifname = NULL;
  struct pal_in4_addr nbr_addr;
  
  /* Get the area id */
  CLI_GET_OSPF_AREA_ID (area_id, format, argv[0]);

  ifname = argv[1];

  /* Get neighbor ipv4 address */
  CLI_GET_IPV4_ADDRESS ("neighbor address", nbr_addr, argv[2]);
  
  /* Create multi area link for the given area to the specified neighbor */
  ret = ospf_multi_area_adjacency_set (cli->vr->id, top->ospf_id, area_id,
                                       ifname, nbr_addr, format);
  if (ret != OSPF_API_SET_SUCCESS)
    return ospf_cli_return (cli, ret);

  return CLI_SUCCESS;
}
CLI (no_area_multi_area_adjacency,
     no_area_multi_area_adjacency_cmd,
     "no area (A.B.C.D|<0-4294967295>)"
     "multi-area-adjacency IFNAME (neighbor A.B.C.D|)",
     OSPF_CLI_AREA_STR,
     "Set the multi-area-adjacency",
     "Interface name",
     "Set the neighbor",
     "Neighbor ip address")
{
  struct ospf *top = cli->index;
  struct pal_in4_addr area_id;
  s_int32_t format;
  s_int32_t ret;
  struct pal_in4_addr nbr_addr;
  u_int8_t *ifname = NULL;
  
  /* Get the area id */
  CLI_GET_OSPF_AREA_ID (area_id, format, argv[0]);

  ifname = argv[1];

  /* Get neighbor ipv4 address */ 
  if (argc == 2)
    nbr_addr = IPv4AddressUnspec;
  else
    CLI_GET_IPV4_ADDRESS ("neighbor address", nbr_addr, argv[3]);

  /* Un set multi area adjacency */
  ret = ospf_multi_area_adjacency_unset (cli->vr->id, top->ospf_id, area_id,
                                         ifname, nbr_addr);
  if (ret != OSPF_API_SET_SUCCESS)
    return ospf_cli_return (cli, ret);

  return CLI_SUCCESS;
}
#endif /* HAVE_OSPF_MULTI_AREA */

#ifdef HAVE_OPAQUE_LSA
#ifdef HAVE_DEV_TEST
/* This command is only for debugging purpose. */
CLI (opaque_lsa_gen,
     opaque_lsa_gen_cmd,
     "opaque (link|area|as) WORD <0-256> <0-16777216> [WORD]",
     "Opaque",
     "Scope link",
     "Scope area",
     "Scope as",
     "Scope ext",
     "Type",
     "ID",
     "Data")
{
  struct ospf *top = cli->index;
  struct pal_in4_addr ls_id, addr, area_id;
  u_int32_t id, len = 0;
  u_char type;
  u_char *data = NULL;
  int ret = OSPF_API_SET_SUCCESS;
  int format;

  CLI_GET_INTEGER ("Opaque type", type, argv[2]);
  CLI_GET_INTEGER ("Opaque ID", id, argv[3]);

  ls_id.s_addr = MAKE_OPAQUE_ID (type, id);
  cli_out (cli, "Id is %x\n", ls_id.s_addr);

  /* Get Opaque Data. */
  if (argc == 5)
    {
      data = (u_char *)argv [4];
      len = pal_strlen ((char *)data);
    }

  if (pal_strncmp (argv[0], "link", 4) == 0)
    {
      CLI_GET_IPV4_ADDRESS ("Interface Address", addr, argv[1]);
      ret = ospf_opaque_link_lsa_set (cli->vr->id, top->ospf_id,
                                      addr, type, id, data, len);
    }
  else if (pal_strncmp (argv[0], "area", 4) == 0)
    {
      CLI_GET_OSPF_AREA_ID (area_id, format, argv[1]);
      ret = ospf_opaque_area_lsa_set (cli->vr->id, top->ospf_id, area_id,
                                      type, id, data, len);
    }
  else
    ret = ospf_opaque_as_lsa_set (cli->vr->id, top->ospf_id, type, id,
                                  data, len);
 
  if (ret != OSPF_API_SET_SUCCESS)
    return ospf_cli_return (cli, ret);

  return CLI_SUCCESS;
}
#endif /* HAVE_DEV_TEST */

CLI (ospf_capability_opaque,
     ospf_capability_opaque_cmd,
     "capability opaque",
     "Enable specific OSPF feature",
     "Opaque LSA")
{
  struct ospf *top = cli->index;
  int ret;

  ret = ospf_capability_opaque_lsa_set (cli->vr->id, top->ospf_id);
  if (ret != OSPF_API_SET_SUCCESS)
    return ospf_cli_return (cli, ret);

  return CLI_SUCCESS;
}

ALI (ospf_capability_opaque,
     ospf_opaque_lsa_capable_cmd,
     "opaque-lsa-capable",
     "Enable Opaque-LSA capability");

CLI (no_ospf_capability_opaque,
     no_ospf_capability_opaque_cmd,
     "no capability opaque",
     CLI_NO_STR,
     "Enable specific OSPF feature",
     "Opaque LSA")
{
  struct ospf *top = cli->index;
  int ret;

  ret = ospf_capability_opaque_lsa_unset (cli->vr->id, top->ospf_id);
  if (ret != OSPF_API_SET_SUCCESS)
    return ospf_cli_return (cli, ret);

  return CLI_SUCCESS;
}

ALI (no_ospf_capability_opaque,
     no_ospf_opaque_lsa_capable_cmd,
     "no opaque-lsa-capable",
     CLI_NO_STR,
     "Enable Opaque-LSA capability");
#endif /* HAVE_OPAQUE_LSA */

#ifdef HAVE_OSPF_TE
CLI (ospf_capability_te,
     ospf_capability_te_cmd,
     "capability (te|traffic-engineering)",
     "Enable specific OSPF feature",
     "OSPF Traffic Engineering extension",
     "OSPF Traffic Engineering extension")
{
  struct ospf *top = cli->index;
  int ret;

  ret = ospf_capability_traffic_engineering_set (cli->vr->id, top->ospf_id);
  if (ret != OSPF_API_SET_SUCCESS)
    return ospf_cli_return (cli, ret);

  return CLI_SUCCESS;
}

ALI (ospf_capability_te,
     ospf_enable_te_cmd,
     "enable-te",
     "Enable OSPF-TE extension");

CLI (no_ospf_capability_te,
     no_ospf_capability_te_cmd,
     "no capability (te|traffic-engineering)",
     CLI_NO_STR,
     "Enable specific OSPF feature",
     "OSPF Traffic Engineering extension",
     "OSPF Traffic Engineering extension")
{
  struct ospf *top = cli->index;
  int ret;

  ret = ospf_capability_traffic_engineering_unset (cli->vr->id, top->ospf_id);
  if (ret != OSPF_API_SET_SUCCESS)
    return ospf_cli_return (cli, ret);

  return CLI_SUCCESS;
}

ALI (no_ospf_capability_te,
     no_ospf_enable_te_cmd,
     "no enable-te",
     CLI_NO_STR,
     "Enable OSPF-TE extension");

#ifdef HAVE_GMPLS

CLI (ospf_te_flooding,
     ospf_te_flooding_cmd,
     "te-flooding ospf <0-65535> area (A.B.C.D|<0-4294967295>)",
     "Enable te-flooding",
     "OSPF instance",
     "OSPF instance ID",
     "OSPF area",
     "OSPF area ID in IP address format",
     "OSPF area ID as a decimal value")
{
  struct telink *tl = cli->index;
  struct pal_in4_addr area_id;
  int format;
  int ospf_id = 0;
  int ret = 0;

  if (argc > 0)
    CLI_GET_INTEGER_RANGE ("OSPF process ID", ospf_id, argv[0], 0, 65535);

  /* Get Area ID. */
  CLI_GET_OSPF_AREA_ID (area_id, format, argv[1]);

  ret = ospf_te_link_flood_scope_set (cli->vr->id, tl->name, 
                                      ospf_id, area_id, format);

  if (ret != OSPF_API_SET_SUCCESS)
    return ospf_cli_return (cli, ret);

  return CLI_SUCCESS;
}

CLI (no_ospf_te_flooding,
     no_ospf_te_flooding_cmd,
     "no te-flooding ospf <0-65535> area (A.B.C.D|<0-4294967295>)",
     "Disable te-flooding",
     "OSPF instance",
     "OSPF Process ID"
     "OSPF area",
     "OSPF area ID in IP address format",
     "OSPF area ID as a decimal value")
{
  struct telink *tl = cli->index;
  struct pal_in4_addr area_id;
  int format;
  int ospf_id = 0;
  int ret = 0;

  if (argc > 0)
    CLI_GET_INTEGER_RANGE ("OSPF process ID", ospf_id, argv[0], 0, 65535);

  /* Get Area ID. */
  CLI_GET_OSPF_AREA_ID (area_id, format, argv[1]);

  ret = ospf_te_link_flood_scope_unset (cli->vr->id, tl->name,
                                        ospf_id, area_id);

  if (ret != OSPF_API_SET_SUCCESS)
    return ospf_cli_return (cli, ret);

  return CLI_SUCCESS;
}

CLI (ospf_enable_te_link_local,
     ospf_enable_te_link_local_cmd,
     "enable-te-link-local",
     "Enable TE Link local exchange")
{
  struct telink *tl = cli->index;
  int ret = 0;  

  ret = ospf_opaque_te_link_local_lsa_enable (cli->vr->id, 
                                              tl->name);

  if (ret != OSPF_API_SET_SUCCESS)
    return ospf_cli_return (cli, ret);

  return CLI_SUCCESS;
}

CLI (ospf_disable_te_link_local,
     ospf_disable_te_link_local_cmd,
     "disable-te-link-local",
     "Disable TE Link local exchange")
{
  struct telink *tl = cli->index;
  int ret = 0;  

  ret = ospf_opaque_te_link_local_lsa_disable (cli->vr->id, 
                                               tl->name);

  if (ret != OSPF_API_SET_SUCCESS)
    return ospf_cli_return (cli, ret);

  return CLI_SUCCESS;
}
#endif /* HAVE_GMPLS */
#endif /* HAVE_OSPF_TE */

#ifdef HAVE_OSPF_CSPF
CLI (ospf_capability_cspf,
     ospf_capability_cspf_cmd, 
     "capability cspf",
     "Enable specific OSPF feature",
     "Constrained Shortest Path First")
{
  struct ospf *top = cli->index;
  int ret;

  ret = ospf_capability_cspf_set (cli->vr->id, top->ospf_id);
  if (ret != OSPF_API_SET_SUCCESS)
    return ospf_cli_return (cli, ret);

  return CLI_SUCCESS;
}

ALI (ospf_capability_cspf,
     ospf_enable_cspf_cmd, 
     "enable-cspf",
     "Enable CSPF feature");

CLI (no_ospf_capability_cspf,
     no_ospf_capability_cspf_cmd, 
     "no capability cspf",
     CLI_NO_STR,
     "Enable specific OSPF feature",
     "Constrained Shortest Path First")
{
  struct ospf *top = cli->index;
  int ret;
  
  ret = ospf_capability_cspf_unset (cli->vr->id, top->ospf_id);
  if (ret != OSPF_API_SET_SUCCESS)
    return ospf_cli_return (cli, ret);

  return CLI_SUCCESS;
}
 
ALI (no_ospf_capability_cspf,
     no_ospf_enable_cspf_cmd, 
     "no enable-cspf",
     CLI_NO_STR,
     "Enable CSPF feature");

#ifdef HAVE_GMPLS

CLI (cspf_enable_better_protection,
     cspf_enable_better_protection_cmd,
     "cspf enable-better-protection",
     "Constrained Shortest Path First",
     "Enable default CSPF protection type")
{
  struct ospf *top = cli->index;
  int ret;

  ret = ospf_cspf_better_protection_type (cli->vr->id, top->ospf_id, PAL_TRUE);
  if (ret != OSPF_API_SET_SUCCESS)
    return ospf_cli_return (cli, ret);

  return CLI_SUCCESS;
}

CLI (cspf_disable_better_protection,
     cspf_disable_better_protection_cmd,
     "cspf disable-better-protection",
     "Constrained Shortest Path First",
     "Disable default CSPF protection type")
{
  struct ospf *top = cli->index;
  int ret;

  ret = ospf_cspf_better_protection_type (cli->vr->id, top->ospf_id, PAL_FALSE);
  if (ret != OSPF_API_SET_SUCCESS)
    return ospf_cli_return (cli, ret);

  return CLI_SUCCESS;
}
#endif /* HAVE_GMPLS */

CLI (cspf_tie_break,
     cspf_tie_break_cmd,
     "cspf tie-break (random|least-fill|most-fill)",
     "Constrained Shortest Path First",
     "Set CSPF tie break method",
     "Random",
     "Least fill",
     "Most fill")
{
  struct ospf *top = cli->index;
  struct cspf *cspf;
  u_char tie_break;

  if (! top->cspf) 
    {
      cli_out (cli, "CSPF not enabled\n");
      return CLI_ERROR;
    }

  cspf = top->cspf;

  if (pal_strncmp ("r", argv[0], 1) == 0)
    tie_break = CSPF_TIE_BREAK_RANDOM;
  else if (pal_strncmp ("l", argv[0], 1) == 0)
    tie_break = CSPF_TIE_BREAK_LEAST_FILL;
  else if (pal_strncmp ("m", argv[0], 1) == 0)
    tie_break = CSPF_TIE_BREAK_MOST_FILL;
  else
    {
      cli_out (cli, "Incorrect input\n");
      return CLI_ERROR;
    }

  if ((! CHECK_FLAG (cspf->config, CSPF_CONFIG_TIE_BREAK)) || 
      (tie_break != cspf->tie_break))
    {
      cspf->tie_break = tie_break;
      SET_FLAG (cspf->config, CSPF_CONFIG_TIE_BREAK);
    }

  return CLI_SUCCESS;
}

ALI (cspf_tie_break,
     cspf_tie_break_old_cmd,
     "cspf-tie-break (random|least-fill|most-fill)",
     "Set CSPF tie break method",
     "Random",
     "Least fill",
     "Most fill");

CLI (no_cspf_tie_break,
     no_cspf_tie_break_cmd,
     "no cspf tie-break",
     CLI_NO_STR,
     "Constrained Shortest Path First",
     "Set CSPF tie break method")
{
  struct ospf *top = cli->index;
  struct cspf *cspf;

  if (! top->cspf)
    {
      cli_out (cli, "CSPF not enabled\n");
      return CLI_ERROR;
    }

  cspf = top->cspf;

  if (CHECK_FLAG (cspf->config, CSPF_CONFIG_TIE_BREAK))
    {
      cspf->tie_break = CSPF_TIE_BREAK_RANDOM;
      UNSET_FLAG (cspf->config, CSPF_CONFIG_TIE_BREAK);
    }
  
  return CLI_SUCCESS;
}

ALI (no_cspf_tie_break,
     no_cspf_tie_break_old_cmd,
     "no cspf-tie-break",
     CLI_NO_STR,
     "Set CSPF tie break method");

CLI (cspf_default_retry_interval,
     cspf_default_retry_interval_cmd,
     "cspf default-retry-interval <1-3600>",
     "Constrained Shortest Path First",
     "Set CSPF default computation retry interval",
     "Interval")
{
  struct ospf *top = cli->index;
  u_int16_t interval;

  if (! top->cspf) 
    {
      cli_out (cli, "CSPF not enabled\n");
      return CLI_ERROR;
    }

  pal_sscanf (argv[0], "%hu", &interval);
  if ((interval < 1) || (interval > 3600))
    {
      cli_out (cli, "Invalid value specified\n");
      return CLI_ERROR;
    }

  top->cspf->def_retry_interval = interval;
  SET_FLAG (top->cspf->config, CSPF_CONFIG_DEF_RETRY_INTERVAL);
 
  return CLI_SUCCESS;
}

ALI (cspf_default_retry_interval,
     cspf_default_retry_interval_old_cmd,
     "cspf-default-retry-interval <1-3600>",
     "Set CSPF default computation retry interval",
     "Interval");

CLI (no_cspf_default_retry_interval,
     no_cspf_default_retry_interval_cmd,
     "no cspf default-retry-interval",
     CLI_NO_STR,
     "Constrained Shortest Path First",
     "Set CSPF default computation retry interval")
{
  struct ospf *top = cli->index;

  if (! top->cspf) 
    {
      cli_out (cli, "CSPF not enabled\n");
      return CLI_ERROR;
    }

  if (CHECK_FLAG (top->cspf->config, CSPF_CONFIG_DEF_RETRY_INTERVAL))
    {
      top->cspf->def_retry_interval = CSPF_DEFAULT_RETRY_INTERVAL;
      UNSET_FLAG (top->cspf->config, CSPF_CONFIG_DEF_RETRY_INTERVAL);
    }
 
  return CLI_SUCCESS;
}

ALI (no_cspf_default_retry_interval,
     no_cspf_default_retry_interval_old_cmd,
     "no cspf-default-retry-interval",
     CLI_NO_STR,
     "Set CSPF default computation retry interval");
#endif /* HAVE_OSPF_CSPF */

#define OSPF_CLI_REDIST_PROTOS_CMD                                            \
    "kernel|connected|static|rip|bgp|isis|ospf (<1-65535>|)"
#define OSPF_CLI_REDIST_OPTIONS_CMD                                           \
    "metric <0-16777214>|metric-type (1|2)|?route-map WORD"
#define OSPF_CLI_NO_REDIST_OPTIONS_CMD                                        \
    "metric|metric-type|?route-map"

#define OSPF_CLI_REDIST_STR                                                   \
    "Redistribute information from another routing protocol"
    
#define OSPF_CLI_REDIST_PROTOS_STR                                            \
    "Kernel routes",                                                          \
    "Connected",                                                              \
    "Static routes",                                                          \
    "Routing Information Protocol (RIP)",                                     \
    "Border Gateway Protocol (BGP)",                                          \
    "ISO IS-IS",                                                              \
    "Open Shortest Path First (OSPF)",                                        \
    "OSPF Process ID"

#define OSPF_CLI_REDIST_OPTIONS_STR                                           \
    "OSPF default metric",                                                    \
    "OSPF metric",                                                            \
    "OSPF metric type for default routes",                                    \
    "Set OSPF External Type 1 metrics",                                       \
    "Set OSPF External Type 2 metrics",                                       \
    "Route map reference",                                                    \
    "Pointer to route-map entries"

CLI (ospf_redistribute,
     ospf_redistribute_cmd,
     "redistribute (" OSPF_CLI_REDIST_PROTOS_CMD ")",
     OSPF_CLI_REDIST_STR,
     OSPF_CLI_REDIST_PROTOS_STR)
{
  struct ospf *top = cli->index;
  int source;
  int ret = 0;
  int ospf_id = 0;

  if (!ospf_cli_str2distribute_source (argv[0], &source))
    return CLI_ERROR;

  /* If the process-id is specified then check
     its  validity */
  if (source == APN_ROUTE_OSPF &&  argv[1])
    CLI_GET_INTEGER_RANGE ("OSPF process ID", ospf_id, argv[1], 1, 65535);
  
  ret = ospf_redist_proto_set (cli->vr->id, top->ospf_id, source, ospf_id);

  if (ret != OSPF_API_SET_SUCCESS)
    return ospf_cli_return (cli, ret);

  ret = ospf_redist_metric_unset (cli->vr->id, top->ospf_id, source, ospf_id);
  if (ret != OSPF_API_SET_SUCCESS)
    return ospf_cli_return (cli, ret);

  return CLI_SUCCESS;
}

CLI (ospf_redistribute_options,
     ospf_redistribute_options_cmd,
     "redistribute (" OSPF_CLI_REDIST_PROTOS_CMD ") "
     "{" OSPF_CLI_REDIST_OPTIONS_CMD
     "|tag <0-4294967295>}",
     OSPF_CLI_REDIST_STR,
     OSPF_CLI_REDIST_PROTOS_STR,
     OSPF_CLI_REDIST_OPTIONS_STR,
     "Set tag for routes redistributed into OSPF",
     "32-bit tag value")
{
  struct ospf *top = cli->index;
  int source;
  int mtype;
  int metric;
  u_int32_t tag;
  int i = 1;
  int ret = 0;
  int ospf_id = 0;

  if (!ospf_cli_str2distribute_source (argv[0], &source))
    return CLI_ERROR;

  /* Checking whether the process-id is specified while redistributing OSPF  */
  if ((source == APN_ROUTE_OSPF) && pal_strncmp (argv[1], "metric", 6) != 0)
    {
      /* check PID's validity */
      CLI_GET_INTEGER_RANGE ("OSPF process ID", ospf_id, argv[1], 1, 65535);
      i++;
    }
    
  ret = ospf_redist_proto_set (cli->vr->id, top->ospf_id, source, ospf_id);
  
  if (ret != OSPF_API_SET_SUCCESS)
    return ospf_cli_return (cli, ret);

  /*  If instance id is given then argv[2] will be the metric; i will be 2 
   *  As i value is updated in above if-check, i value not initialised in for()
  */
  for (; i < argc; i += 2)
    {
      if (pal_strncmp (argv[i], "metric-", 7) == 0)
        {
          if (!ospf_str2metric_type (argv[i + 1], &mtype))
            return CLI_ERROR;

          ret = ospf_redist_metric_type_set (cli->vr->id, top->ospf_id,
                                             source, mtype, ospf_id);
        }
      else if (pal_strncmp (argv[i], "metric", 6) == 0)
        {
          if (!ospf_str2metric (argv[i + 1], &metric))
            return CLI_ERROR;
          
          ret = ospf_redist_metric_set (cli->vr->id, top->ospf_id,
                                        source, metric, ospf_id);
        }
      else if (pal_strncmp (argv[i], "t", 1) == 0)
        {
          CLI_GET_UINT32 ("Route Tag", tag, argv[i + 1]);
          ret = ospf_redist_tag_set (cli->vr->id, top->ospf_id, source, tag, ospf_id);
        }
      else if (pal_strncmp (argv[i], "r", 1) == 0)
        ret = ospf_routemap_set (cli->vr->id, top->ospf_id, source,
                                 argv[i + 1], ospf_id);

      if (ret != OSPF_API_SET_SUCCESS)
        return ospf_cli_return (cli, ret);
    }

  return CLI_SUCCESS;
}

CLI (no_ospf_redistribute,
     no_ospf_redistribute_cmd,
     "no redistribute (" OSPF_CLI_REDIST_PROTOS_CMD ")",
     CLI_NO_STR,
     OSPF_CLI_REDIST_STR,
     OSPF_CLI_REDIST_PROTOS_STR)
{
  struct ospf *top = cli->index;
  int source;
  int ret = 0;
  int ospf_id = 0;

  if (!ospf_cli_str2distribute_source (argv[0], &source))
    return CLI_ERROR;

  if (source == APN_ROUTE_OSPF && argv[1])
    CLI_GET_INTEGER_RANGE ("OSPF process ID", ospf_id, argv[1], 1, 65535);
  
  ret = ospf_redist_proto_unset (cli->vr->id, top->ospf_id, source, ospf_id);

  if (ret != OSPF_API_SET_SUCCESS)
    return ospf_cli_return (cli, ret);

  ret = ospf_routemap_unset (cli->vr->id, top->ospf_id, source, ospf_id);
  if (ret != OSPF_API_SET_SUCCESS)
    return ospf_cli_return (cli, ret);

  return CLI_SUCCESS;
}

CLI (no_ospf_redistribute_options,
     no_ospf_redistribute_options_cmd,
     "no redistribute (" OSPF_CLI_REDIST_PROTOS_CMD ") "
     "{" OSPF_CLI_NO_REDIST_OPTIONS_CMD
     "|tag }",
     CLI_NO_STR,
     OSPF_CLI_REDIST_STR,
     OSPF_CLI_REDIST_PROTOS_STR,
     OSPF_CLI_REDIST_OPTIONS_STR,
     "Set tag for routes redistributed into OSPF")
{
  struct ospf *top = cli->index;
  int source;
  int i;
  int ret = 0;
  int ospf_id = 0;

  if (!ospf_cli_str2distribute_source (argv[0], &source))
    return CLI_ERROR;

  if (source == APN_ROUTE_OSPF && argv[1])
    CLI_GET_INTEGER_RANGE ("OSPF process ID", ospf_id, argv[1], 1, 65535);

  for (i = 1; i < argc; i++)
    {
      if (pal_char_isdigit (argv[i][0]))
        continue;

      if (pal_strncmp (argv[i], "metric-", 7) == 0)
        ret = ospf_redist_metric_type_unset (cli->vr->id, top->ospf_id, source, ospf_id);
      else if (pal_strncmp (argv[i], "metric", 6) == 0)
        ret = ospf_redist_metric_unset (cli->vr->id, top->ospf_id, source, ospf_id);
      else if (pal_strncmp (argv[i], "t", 1) == 0)
        ret = ospf_redist_tag_unset (cli->vr->id, top->ospf_id, source, ospf_id);
      else if (pal_strncmp (argv[i], "r", 1) == 0)
        {
          ret = ospf_routemap_unset (cli->vr->id, top->ospf_id, source, ospf_id);
          i++;
        }

      if (ret != OSPF_API_SET_SUCCESS)
        return ospf_cli_return (cli, ret);
    }

  return CLI_SUCCESS;
}

CLI (ospf_distribute_list_out,
     ospf_distribute_list_out_cmd,
     "distribute-list WORD out (" OSPF_CLI_REDIST_PROTOS_CMD ")",
     "Filter networks in routing updates",
     "Access-list name",
     CLI_OUT_STR,
     OSPF_CLI_REDIST_PROTOS_STR,
     "Process-ID")
{
  struct ospf *top = cli->index;
  int source;
  int ret = 0;
  int ospf_id = 0;

  if (!ospf_cli_str2distribute_source (argv[1], &source))
    return CLI_ERROR;

  /* Checking the range of OSPF process ID */
  if (source == APN_ROUTE_OSPF &&  argv[2])
    CLI_GET_INTEGER_RANGE ("OSPF process ID", ospf_id, argv[2], 1, 65535);
  

  ret = ospf_distribute_list_out_set (cli->vr->id, top->ospf_id,
                                      source, ospf_id, argv[0]);
  if (ret != OSPF_API_SET_SUCCESS)
    return ospf_cli_return (cli, ret);

  return CLI_SUCCESS;
}

CLI (no_ospf_distribute_list_out,
     no_ospf_distribute_list_out_cmd,
     "no distribute-list WORD out (" OSPF_CLI_REDIST_PROTOS_CMD ")",
     CLI_NO_STR,
     "Filter networks in routing updates",
     "Access-list name",
     CLI_OUT_STR,
     OSPF_CLI_REDIST_PROTOS_STR,
     "OSPF Process-ID")
{
  struct ospf *top = cli->index;
  int source;
  int ret = 0;
  int ospf_id = 0;

  if (!ospf_cli_str2distribute_source (argv[1], &source))
    return CLI_ERROR;

  if (source == APN_ROUTE_OSPF &&  argv[2])
    CLI_GET_INTEGER_RANGE ("OSPF process ID", ospf_id, argv[2], 1, 65535);

  ret = ospf_distribute_list_out_unset (cli->vr->id, top->ospf_id,
                                        source, ospf_id, argv[0]);

  if (ret != OSPF_API_SET_SUCCESS)
    return ospf_cli_return (cli, ret);

  return CLI_SUCCESS;
}

CLI (ospf_distribute_list_in,
     ospf_distribute_list_in_cmd,
     "distribute-list WORD in",
     "Filter networks in routing updates",
     "Access-list name",CLI_IN_STR)
{
  struct ospf *top = cli->index;
  int ret = 0;
  
  ret = ospf_distribute_list_in_set (cli->vr->id, top->ospf_id, argv[0]);

  if (ret != OSPF_API_SET_SUCCESS)
    return ospf_cli_return (cli, ret);

  return CLI_SUCCESS;
}     

CLI (no_ospf_distribute_list_in,
     no_ospf_distribute_list_in_cmd,
     "no distribute-list WORD in",
     CLI_NO_STR,
     "Filter networks in routing updates",
     "Access-list name",CLI_IN_STR)
{
  struct ospf *top = cli->index;
  int ret = 0;
 
  ret = ospf_distribute_list_in_unset (cli->vr->id, top->ospf_id, argv[0]);

  if (ret != OSPF_API_SET_SUCCESS)
    return ospf_cli_return (cli, ret);

  return CLI_SUCCESS;
}

CLI (ospf_default_information_originate,
     ospf_default_information_originate_cmd,
     "default-information originate",
     "Control distribution of default information",
     "Distribute a default route")
{
  struct ospf *top = cli->index;
  int origin = OSPF_DEFAULT_ORIGINATE_NSM;
  int mtype;
  int metric;
  int i;
  int ret = 0;

  for (i = 0; i < argc; i++)
    {
      if (pal_strncmp (argv[i], "r", 1) == 0)
        i++;
      else if (pal_strncmp (argv[i], "a", 1) == 0)
        origin = OSPF_DEFAULT_ORIGINATE_ALWAYS;
    }

  for (i = 0; i < argc; i += 2)
    {
      if (pal_strncmp (argv[i], "metric-", 7) == 0)
        {
          if (!ospf_str2metric_type (argv[i + 1], &mtype))
            return CLI_ERROR;

          ret = ospf_redist_metric_type_set (cli->vr->id, top->ospf_id,
                                             APN_ROUTE_DEFAULT, mtype, 0);
        }
      else if (pal_strncmp (argv[i], "metric", 6) == 0)
        {
          if (!ospf_str2metric (argv[i + 1], &metric))
            return CLI_ERROR;

          ret = ospf_redist_metric_set (cli->vr->id, top->ospf_id,
                                        APN_ROUTE_DEFAULT, metric, 0);
        }
      else if (pal_strncmp (argv[i], "r", 1) == 0)
        ret = ospf_routemap_default_set (cli->vr->id, top->ospf_id,
                                         argv[i + 1]);
      else if (pal_strncmp (argv[i], "a", 1) == 0)
        i--;
      else
        return CLI_ERROR;

      if (ret != OSPF_API_SET_SUCCESS)
        return ospf_cli_return (cli, ret);
    }

  ret = ospf_redist_default_set (cli->vr->id, top->ospf_id, origin);
  if (ret != OSPF_API_SET_SUCCESS)
    return ospf_cli_return (cli, ret);

  return CLI_SUCCESS;
}

ALI (ospf_default_information_originate,
     ospf_default_information_originate_options_cmd,
     "default-information originate "
     "{" OSPF_CLI_REDIST_OPTIONS_CMD "|always}",
     "Control distribution of default information",
     "Distribute a default route",
     OSPF_CLI_REDIST_OPTIONS_STR,
     "Always advertise default route");

CLI (no_ospf_default_information_originate,
     no_ospf_default_information_originate_cmd,
     "no default-information originate",
     CLI_NO_STR,
     "Control distribution of default information",
     "Distribute a default route")
{
  struct ospf *top = cli->index;
  int ret = 0;
  
  ret = ospf_routemap_default_unset (cli->vr->id, top->ospf_id);
  if (ret != OSPF_API_SET_SUCCESS)
    return ospf_cli_return (cli, ret);

  ret = ospf_redist_default_unset (cli->vr->id, top->ospf_id);
  if (ret != OSPF_API_SET_SUCCESS)
    return ospf_cli_return (cli, ret);

  return CLI_SUCCESS;
}

CLI (no_ospf_default_information_originate_options,
     no_ospf_default_information_originate_options_cmd,
     "no default-information originate "
     "{" OSPF_CLI_NO_REDIST_OPTIONS_CMD "|always}",
     CLI_NO_STR,
     "Control distribution of default information",
     "Distribute a default route",
     OSPF_CLI_REDIST_OPTIONS_STR,
     "Always advertise default route")
{
  struct ospf *top = cli->index;
  int i;
  int ret = 0;

  for (i = 0; i < argc; i++)
    {
      if (pal_char_isdigit (argv[i][0]))
        continue;

      if (pal_strncmp (argv[i], "metric-", 7) == 0)
        ret = ospf_redist_metric_type_unset (cli->vr->id, top->ospf_id,
                                             APN_ROUTE_DEFAULT, 0);
      else if (pal_strncmp (argv[i], "metric", 6) == 0)
        ret = ospf_redist_metric_unset (cli->vr->id, top->ospf_id,
                                        APN_ROUTE_DEFAULT, 0);
      else if (pal_strncmp (argv[i], "r", 1) == 0)
        {
          ret = ospf_routemap_unset (cli->vr->id, top->ospf_id,
                                     APN_ROUTE_DEFAULT, 0);
          i++;
        }
      else if (pal_strncmp (argv[i], "a", 1) == 0)
        ret = ospf_redist_default_set (cli->vr->id, top->ospf_id,
                                       OSPF_DEFAULT_ORIGINATE_NSM);

      if (ret != OSPF_API_SET_SUCCESS)
        return ospf_cli_return (cli, ret);
    }

  return CLI_SUCCESS;
}

CLI (ospf_default_metric,
     ospf_default_metric_cmd,
     "default-metric <1-16777214>",
     "Set metric of redistributed routes",
     "Default metric")
{
  struct ospf *top = cli->index;
  int metric = -1;
  int ret = 0;

  if (!ospf_str2metric (argv[0], &metric))
    return CLI_ERROR;

  ret = ospf_default_metric_set (cli->vr->id, top->ospf_id, metric);
  if (ret != OSPF_API_SET_SUCCESS)
    return ospf_cli_return (cli, ret);

  return CLI_SUCCESS;
}

CLI (no_ospf_default_metric,
     no_ospf_default_metric_cmd,
     "no default-metric",
     CLI_NO_STR,
     "Set metric of redistributed routes")
{
  struct ospf *top = cli->index;
  int ret = 0;

  ret = ospf_default_metric_unset (cli->vr->id, top->ospf_id);
  if (ret != OSPF_API_SET_SUCCESS)
    return ospf_cli_return (cli, ret);

  return CLI_SUCCESS;
}

ALI (no_ospf_default_metric,
     no_ospf_default_metric_val_cmd,
     "no default-metric <1-16777214>",
     CLI_NO_STR,
     "Set metric of redistributed routes",
     "Default metric");

CLI (ospf_distance,
     ospf_distance_cmd,
     "distance <1-255>",
     "Define an administrative distance",
     "OSPF Administrative distance")
{
  struct ospf *top = cli->index;
  u_int16_t distance;
  int ret = 0;
  
  CLI_GET_INTEGER_RANGE ("Distance", distance, argv[0], 1, 255);

  ret = ospf_distance_all_set (cli->vr->id, top->ospf_id, (u_char) distance);
  if (ret != OSPF_API_SET_SUCCESS)
    return ospf_cli_return (cli, ret);

  return CLI_SUCCESS;
}

CLI (no_ospf_distance,
     no_ospf_distance_cmd,
     "no distance <1-255>",
     CLI_NO_STR,
     "Define an administrative distance",
     "OSPF Administrative distance")
{
  struct ospf *top = cli->index;
  int ret = 0;
  
  ret = ospf_distance_all_unset (cli->vr->id, top->ospf_id);
  if (ret != OSPF_API_SET_SUCCESS)
    return ospf_cli_return (cli, ret);

  return CLI_SUCCESS;
}

CLI (ospf_distance_ospf,
     ospf_distance_ospf_cmd,
     "distance ospf {intra-area <1-255>|inter-area <1-255>|external <1-255>}",
     "Define an administrative distance",
     "OSPF routes Administrative distance",
     "Intra-area routes",
     "Distance for intra-area routes",
     "Inter-area routes",
     "Distance for inter-area routes",
     "External routes",
     "Distance for external routes")
{
  struct ospf *top = cli->index;
  int intra = 0;
  int inter = 0;
  int external = 0;
  int i;
  int ret = 0;

  for (i = 0; i < argc; i += 2)
    {
      if (pal_strncmp (argv[i], "intr", 4) == 0)
        CLI_GET_INTEGER_RANGE ("Distance", intra, argv[i + 1], 1, 255);
      else if (pal_strncmp (argv[i], "inte", 4) == 0)
        CLI_GET_INTEGER_RANGE ("Distance", inter, argv[i + 1], 1, 255);
      else if (pal_strncmp (argv[i], "e", 1) == 0)
        CLI_GET_INTEGER_RANGE ("Distance", external, argv[i + 1], 1, 255);
    }

  if (intra)
    {
      ret = ospf_distance_intra_area_set (cli->vr->id, top->ospf_id, intra);
      if (ret != OSPF_API_SET_SUCCESS)
        return ospf_cli_return (cli, ret);
    }

  if (inter)
    {
      ret = ospf_distance_inter_area_set (cli->vr->id, top->ospf_id, inter);
      if (ret != OSPF_API_SET_SUCCESS)
        return ospf_cli_return (cli, ret);
    }

  if (external)
    {
      ret = ospf_distance_external_set (cli->vr->id, top->ospf_id, external);
      if (ret != OSPF_API_SET_SUCCESS)
        return ospf_cli_return (cli, ret);
    }   

  return CLI_SUCCESS;
}

CLI (no_ospf_distance_ospf,
     no_ospf_distance_ospf_cmd,
     "no distance ospf",
     CLI_NO_STR
     "Define an administrative distance",
     "OSPF Administrative distance",
     "OSPF Distance")
{
  struct ospf *top = cli->index;
  int ret = 0;
  
  ret = ospf_distance_intra_area_unset (cli->vr->id, top->ospf_id);
  if (ret != OSPF_API_SET_SUCCESS)
    return ospf_cli_return (cli, ret);

  ret = ospf_distance_inter_area_unset (cli->vr->id, top->ospf_id);
  if (ret != OSPF_API_SET_SUCCESS)
    return ospf_cli_return (cli, ret);

  ret = ospf_distance_external_unset (cli->vr->id, top->ospf_id);
  if (ret != OSPF_API_SET_SUCCESS)
    return ospf_cli_return (cli, ret);

  return CLI_SUCCESS;
}

#ifdef HAVE_RESTART
CLI (ospf_restart_grace_period,
     ospf_restart_grace_period_cmd,
     "ospf restart grace-period <1-1800>",
     "Open Shortest Path First (OSPF)",
     "Graceful Restart",
     "Grace Period",
     "Seconds")
{
  struct ospf_master *om = cli->vr->proto;
  int reason = OSPF_RESTART_REASON_UNKNOWN;
  u_int16_t seconds;
  int ret;

  CLI_GET_INTEGER_RANGE ("grace period", seconds, argv[0],
                         OSPF_GRACE_PERIOD_MIN, OSPF_GRACE_PERIOD_MAX);

  ret = ospf_graceful_restart_set (om, seconds, reason);
  if (ret != OSPF_API_SET_SUCCESS)
    return ospf_cli_return (cli, ret);

  return CLI_SUCCESS;
}

ALI (ospf_restart_grace_period,
     ospf_hitless_restart_grace_period_cmd,
     "ospf hitless-restart grace-period <1-1800>",
     "Open Shortest Path First (OSPF)",
     "Hitless Restart",
     "Grace Period",
     "Seconds");

CLI (no_ospf_restart_grace_period,
     no_ospf_restart_grace_period_cmd,
     "no ospf restart grace-period",
     CLI_NO_STR,
     "Open Shortest Path First (OSPF)",
     "Graceful Restart",
     "Grace Period")
{
  struct ospf_master *om = cli->vr->proto;
  int ret;

  ret = ospf_graceful_restart_unset (om);
  if (ret != OSPF_API_SET_SUCCESS)
    return ospf_cli_return (cli, ret);

  return CLI_SUCCESS;
}

ALI (no_ospf_restart_grace_period,
     no_ospf_hitless_restart_grace_period_cmd,
     "no ospf hitless-restart grace-period",
     CLI_NO_STR,
     "Open Shortest Path First (OSPF)",
     "Hitless Restart",
     "Grace Period");

CLI (ospf_restart_helper_never,
     ospf_restart_helper_never_cmd,
     "ospf restart helper never (router-id A.B.C.D|)",
     "Open Shortest Path First (OSPF)",
     "Graceful Restart",
     "Local policy as helper mode",
     "Never act as helper",
     "Router-id for which never act as helper",
     "OSPF Router Id")
{
  int ret = OSPF_API_SET_SUCCESS;
  struct pal_in4_addr router_id;

  if (argc > 0)
    {
      CLI_GET_IPV4_ADDRESS ("Router ID", router_id, argv[1]);
      ret = ospf_restart_helper_never_router_set (cli->vr->id, router_id);
    }
  else
    { 
      ret = ospf_restart_helper_policy_set (cli->vr->id, OSPF_RESTART_HELPER_NEVER);
      if (ret != OSPF_API_SET_SUCCESS)
        return ospf_cli_return (cli, ret);
      
      ret = ospf_restart_helper_grace_period_unset (cli->vr->id);
    }

  if (ret != OSPF_API_SET_SUCCESS)
      return ospf_cli_return (cli, ret);

  return CLI_SUCCESS;
}

ALI (ospf_restart_helper_never,
     ospf_hitless_restart_helper_never_cmd,
     "ospf hitless-restart helper never",
     "Open Shortest Path First (OSPF)",
     "Hitless Restart",
     "Local policy as helper mode",
     "Never act as helper");

CLI (ospf_restart_helper_policy,
     ospf_restart_helper_policy_cmd,
     "ospf restart helper {only-reload|only-upgrade|max-grace-period <1-1800>}",
     "Open Shortest Path First (OSPF)",
     "Graceful Restart",
     "Local policy as helper mode",
     "Only help on software reloads",
     "Only help on software upgrades",
     "Maximum grace period to accept",
     "Seconds")
{
  u_int16_t seconds;
  int i;
  int ret = 0;

  for (i = 0; i < argc; i++)
    {
      if (pal_strncmp (argv[i], "only-r", 6) == 0)
        ret = ospf_restart_helper_policy_set (cli->vr->id,
                                              OSPF_RESTART_HELPER_ONLY_RELOAD);
      else if (pal_strncmp (argv[i], "only-u", 6) == 0)
        ret = ospf_restart_helper_policy_set (cli->vr->id,
                                              OSPF_RESTART_HELPER_ONLY_UPGRADE);
      else if (pal_strncmp (argv[i], "m", 1) == 0)
        {
          CLI_GET_INTEGER_RANGE ("grace period", seconds, argv[i + 1],
                                 OSPF_GRACE_PERIOD_MIN, OSPF_GRACE_PERIOD_MAX);
          ret = ospf_restart_helper_grace_period_set (cli->vr->id, seconds);
          i++;
        }
    }

  if (ret != OSPF_API_SET_SUCCESS)
    return ospf_cli_return (cli, ret);

  return CLI_SUCCESS;
}

ALI (ospf_restart_helper_policy,
     ospf_hitless_restart_helper_policy_cmd,
     "ospf hitless-restart helper {only-reload|only-upgrade|max-grace-period <1-1800>}",
     "Open Shortest Path First (OSPF)",
     "Hitless Restart",
     "Local policy as helper mode",
     "Only help on software reloads",
     "Only help on software upgrades",
     "Maximum grace period to accept",
     "Seconds");

CLI (no_ospf_restart_helper_policy,
     no_ospf_restart_helper_policy_cmd,
     "no ospf restart helper (never router-id (A.B.C.D | all) | max-grace-period|)",
     CLI_NO_STR,
     "Open Shortest Path First (OSPF)",
     "Graceful Restart",
     "Local policy as helper mode",
     "never act as helper for a specific Router Id",
     "Router Id for which to act as a helper",
     "IP Address of Router",
     "All Router-Ids",
     "Limit grace period")
{
  int ret = 0;
  struct pal_in4_addr router_id;
  u_int8_t i;

   /* Commands and their implementation
     1. no ospf restart helper --Only unset the helper policy.
     2. no ospf restart helper never router-id A.B.C.D -- Remove the router
        for which helper policy is disabled.
     3. no ospf restart helper max-grace-period -- Resets the max grace period.
  */
  if (argc == 0)
    ret = ospf_restart_helper_policy_unset (cli->vr->id);
  else
    {
      for (i = 0; i < argc; i++)
        {
          if (pal_strncmp (argv[i], "r", 1) == 0)
            {
              if (pal_strncmp (argv[i+1], "a", 1) == 0)
                ret = ospf_restart_helper_never_router_unset_all (cli->vr->id); 
              else
                {  
                  CLI_GET_IPV4_ADDRESS ("Router ID", router_id, argv[i+1]);
                  ret = ospf_restart_helper_never_router_unset (cli->vr->id, router_id); 
                }
            }
          else if (pal_strncmp (argv[i], "m", 1) == 0)
            ret = ospf_restart_helper_grace_period_unset (cli->vr->id);
        }
     }

  if (ret != OSPF_API_SET_SUCCESS)
    return ospf_cli_return (cli, ret);

  return CLI_SUCCESS;
}

ALI (no_ospf_restart_helper_policy,
     no_ospf_hitless_restart_helper_policy_cmd,
     "no ospf hitless-restart helper (never router-id (A.B.C.D | all) |max-grace-period|)",
     CLI_NO_STR,
     "Open Shortest Path First (OSPF)",
     "Hitless Restart",
     "Local policy as helper mode",
     "never act as helper for a specific Router Id",
     "Router Id for which to act as a helper",
     "IP Address of Router",
     "All Router-Ids",
     "Limit grace period");

CLI (restart_ospf_graceful,
     restart_ospf_graceful_cmd,
     "restart ospf graceful (grace-period <1-1800>|)",
     "Restart routing protocol",
     "Open Shortest Path First (OSPF)",
     "Graceful Restart",
     "Grace period",
     "Seconds")
{
  int reason = OSPF_RESTART_REASON_RESTART;
  u_int16_t seconds = 0;
  int ret = 0;

  if (argc > 0)
    {
      CLI_GET_INTEGER_RANGE ("grace period", seconds, argv[1],
                             OSPF_GRACE_PERIOD_MIN, OSPF_GRACE_PERIOD_MAX);
    }
 ret = ospf_restart_graceful (cli->zg, seconds, reason);
 if (OSPF_API_SET_ERR_PROCESS_NOT_EXIST == ret)
   {
     cli_out (cli, "%% There is no OSPF instance\n");
     return CLI_ERROR;
   }
 else if (OSPF_API_SET_SUCCESS != ret)
   return ospf_cli_return (cli, ret);

  return CLI_SUCCESS;
}

ALI (restart_ospf_graceful,
     restart_ospf_hitless_cmd,
     "restart ospf hitless (grace-period <1-1800>|)",
     "Restart routing protocol",
     "Open Shortest Path First (OSPF)",
     "Hitless Restart",
     "Grace period",
     "Seconds");

CLI (ospf_capability_restart,
     ospf_capability_restart_cmd,
     "capability restart (graceful|signaling)",
     "Enable specific OSPF feature",
     "Restarting support",
     "Graceful OSPF Restart (default)",
     "OSPF Restart Signaling")
{
  struct ospf *top = cli->index;
  int method = OSPF_RESTART_METHOD_DEFAULT;
  int ret;

  if (pal_strncmp (argv[0], "g", 1) == 0)
    method = OSPF_RESTART_METHOD_GRACEFUL;
  else if (pal_strncmp (argv[0], "s", 1) == 0)
    method = OSPF_RESTART_METHOD_SIGNALING;

  ret = ospf_capability_restart_set (cli->vr->id, top->ospf_id, method);
  if (ret != OSPF_API_SET_SUCCESS)
    return ospf_cli_return (cli, ret);

  return CLI_SUCCESS;
}

CLI (no_ospf_capability_restart,
     no_ospf_capability_restart_cmd,
     "no capability restart",
     CLI_NO_STR,
     "Enable specific OSPF feature",
     "Restarting support")
{
  struct ospf *top = cli->index;
  int ret;

  ret = ospf_capability_restart_unset (cli->vr->id, top->ospf_id);
  if (ret != OSPF_API_SET_SUCCESS)
    return ospf_cli_return (cli, ret);

  return CLI_SUCCESS;
}
#endif /* HAVE_RESTART */

void
ospf_debug_all_on (struct cli *cli)
{
  int flags = OSPF_DEBUG_SEND|OSPF_DEBUG_RECV|OSPF_DEBUG_DETAIL;
  struct apn_vr *vr = cli->vr;
  struct ospf_master *om = vr->proto;
  int i;

  for (i = OSPF_PACKET_HELLO; i <= OSPF_PACKET_LS_ACK; i++)
    DEBUG_PACKET_ON (cli, i, flags);

  DEBUG_ON (cli, ifsm, IFSM, "OSPF all IFSM");
  DEBUG_ON (cli, nfsm, NFSM, "OSPF all NFSM");
  DEBUG_ON (cli, lsa, LSA, "OSPF all LSA");
  DEBUG_ON (cli, nsm, NSM, "OSPF all NSM");
  DEBUG_ON (cli, event, EVENT, "OSPF all events");
  DEBUG_ON (cli, rt_calc, RT_ALL, "OSPF all route calculation");
#ifdef HAVE_BFD
  DEBUG_ON (cli, bfd, BFD, "OSPF all BFD");
#endif /* HAVE_BFD */
#ifdef HAVE_OSPF_CSPF
  if (cli->mode == CONFIG_MODE)
    CSPF_CONF_DEBUG_ON (&om->debug_cspf, EVENT);
  CSPF_TERM_DEBUG_ON (&om->debug_cspf, EVENT);

  if (cli->mode == CONFIG_MODE)
    CSPF_CONF_DEBUG_ON (&om->debug_cspf, HEXDUMP);
  CSPF_TERM_DEBUG_ON (&om->debug_cspf, HEXDUMP);
#endif /* HAVE_OSPF_CSPF */

  ospf_nsm_debug_set (om);
#ifdef HAVE_BFD
  ospf_bfd_debug_set (om);
#endif /* HAVE_BFD */
}

void
ospf_debug_all_off (struct cli *cli)
{
  int flags = OSPF_DEBUG_SEND|OSPF_DEBUG_RECV|OSPF_DEBUG_DETAIL;
  struct apn_vr *vr = cli->vr;
  struct ospf_master *om = vr->proto;
  int i;

  for (i = OSPF_PACKET_HELLO; i <= OSPF_PACKET_LS_ACK; i++)
    DEBUG_PACKET_OFF (cli, i, flags);

  DEBUG_OFF (cli, ifsm, IFSM, "OSPF all IFSM");
  DEBUG_OFF (cli, nfsm, NFSM, "OSPF all NFSM");
  DEBUG_OFF (cli, lsa, LSA, "OSPF all LSA");
  DEBUG_OFF (cli, nsm, NSM, "OSPF all NSM");
  DEBUG_OFF (cli, event, EVENT, "OSPF all events");
  DEBUG_OFF (cli, rt_calc, RT_ALL, "OSPF all route calculation");
#ifdef HAVE_BFD
  DEBUG_OFF (cli, bfd, BFD, "OSPF all BFD");
#endif /* HAVE_BFD */
#ifdef HAVE_OSPF_CSPF
  if (cli->mode == CONFIG_MODE)
    CSPF_CONF_DEBUG_OFF (&om->debug_cspf, EVENT);
  CSPF_TERM_DEBUG_OFF (&om->debug_cspf, EVENT);

  if (cli->mode == CONFIG_MODE)
    CSPF_CONF_DEBUG_OFF (&om->debug_cspf, HEXDUMP);
  CSPF_TERM_DEBUG_OFF (&om->debug_cspf, HEXDUMP);
#endif /* HAVE_OSPF_CSPF */

  ospf_nsm_debug_unset (om);
#ifdef HAVE_BFD
  ospf_bfd_debug_unset (om);
#endif /* HAVE_BFD */
}

CLI (debug_ospf,
     debug_ospf_cmd,
     "debug ospf (all|)",
     CLI_DEBUG_STR,
     CLI_OSPF_STR,
     "Enable all debugging")
{
  ospf_debug_all_on (cli);
  if (cli->mode != CONFIG_MODE)
    cli_out (cli, "All possible debugging has been turned on\n");

  return CLI_SUCCESS;
}

CLI (no_debug_ospf,
     no_debug_ospf_cmd,
     "no debug ospf (all|)",
     CLI_NO_STR,
     CLI_DEBUG_STR,
     CLI_OSPF_STR,
     CLI_UNDEBUG_ALL_STR)
{
  ospf_debug_all_off (cli);
  if (cli->mode != CONFIG_MODE)
    cli_out (cli, "All possible debugging has been turned off\n");

  return CLI_SUCCESS;
}

ALI (no_debug_ospf,
     undebug_ospf_cmd,
     "undebug ospf (all|)",
     CLI_UNDEBUG_STR,
     CLI_OSPF_STR,
     CLI_UNDEBUG_ALL_STR);

ALI (no_debug_ospf,
     no_debug_ospf_all_cmd,
     "no debug all ospf",
     CLI_NO_STR,
     CLI_DEBUG_STR,
     CLI_UNDEBUG_ALL_STR,
     CLI_OSPF_STR);

ALI (no_debug_ospf,
     undebug_ospf_all_cmd,
     "undebug all ospf",
     CLI_UNDEBUG_STR,
     CLI_UNDEBUG_ALL_STR,
     CLI_OSPF_STR);

CLI (no_debug_all_ospf,
     no_debug_all_ospf_cmd,
     "no debug all",
     CLI_NO_STR,
     CLI_DEBUG_STR,
     CLI_UNDEBUG_ALL_STR)
{
  ospf_debug_all_off (cli);

  return CLI_SUCCESS;
}

ALI (no_debug_all_ospf,
     undebug_all_ospf_cmd,
     "undebug all",
     CLI_UNDEBUG_STR,
     CLI_UNDEBUG_ALL_STR);

CLI (debug_ospf_packet,
     debug_ospf_packet_cmd,
     "debug ospf packet ({hello|dd|ls-request|ls-update|ls-ack|send|recv|detail}|)",
     CLI_DEBUG_STR,
     CLI_OSPF_STR,
     "OSPF packets",
     "OSPF Hello",
     "OSPF Database Description",
     "OSPF Link State Request",
     "OSPF Link State Update",
     "OSPF Link State Acknowledgment",
     "Packet sent",
     "Packet received",
     "Detail information")
{
  struct ospf_master *om = cli->vr->proto;
  int types = 0;
  int flags = 0;
  int i;

  for (i = 0; i < argc; i ++)
    {
      if (pal_strncmp (argv[i], "h", 1) == 0)
        types |= OSPF_DEBUG_HELLO;
      else if (pal_strncmp (argv[i], "dd", 2) == 0)
        types |= OSPF_DEBUG_DB_DESC;
      else if (pal_strncmp (argv[i], "ls-r", 4) == 0)
        types |= OSPF_DEBUG_LS_REQ;
      else if (pal_strncmp (argv[i], "ls-u", 4) == 0)
        types |= OSPF_DEBUG_LS_UPD;
      else if (pal_strncmp (argv[i], "ls-a", 4) == 0)
        types |= OSPF_DEBUG_LS_ACK;
      else if (pal_strncmp (argv[i], "r", 1) == 0)
        flags |= OSPF_DEBUG_RECV;
      else if (pal_strncmp (argv[i], "s", 1) == 0)
        flags |= OSPF_DEBUG_SEND;
      else if (pal_strncmp (argv[i], "de", 2) == 0)
        flags |= OSPF_DEBUG_DETAIL;
    }

  if (types == 0)
    types = OSPF_DEBUG_PACKET_ALL;

  if ((flags & (OSPF_DEBUG_SEND|OSPF_DEBUG_RECV)) == 0)
    flags |= OSPF_DEBUG_SEND|OSPF_DEBUG_RECV;

  for (i = OSPF_PACKET_HELLO; i <= OSPF_PACKET_LS_ACK; i++)
    if (types & (1 << (i - 1)))
      DEBUG_PACKET_ON (cli, i, flags);

  return CLI_SUCCESS;
}

CLI (no_debug_ospf_packet,
     no_debug_ospf_packet_cmd,
     "no debug ospf packet ({hello|dd|ls-request|ls-update|ls-ack|send|recv|detail}|)",
     CLI_NO_STR,
     CLI_DEBUG_STR,
     CLI_OSPF_STR,
     "OSPF packets",
     "OSPF Hello",
     "OSPF Database Description",
     "OSPF Link State Request",
     "OSPF Link State Update",
     "OSPF Link State Acknowledgment",
     "Packet sent",
     "Packet received",
     "Detail information")
{
  struct ospf_master *om = cli->vr->proto;
  int types = 0;
  int flags = 0;
  int i;

  for (i = 0; i < argc; i ++)
    {
      if (pal_strncmp (argv[i], "h", 1) == 0)
        types |= OSPF_DEBUG_HELLO;
      else if (pal_strncmp (argv[i], "dd", 2) == 0)
        types |= OSPF_DEBUG_DB_DESC;
      else if (pal_strncmp (argv[i], "ls-r", 4) == 0)
        types |= OSPF_DEBUG_LS_REQ;
      else if (pal_strncmp (argv[i], "ls-u", 4) == 0)
        types |= OSPF_DEBUG_LS_UPD;
      else if (pal_strncmp (argv[i], "ls-a", 4) == 0)
        types |= OSPF_DEBUG_LS_ACK;
      else if (pal_strncmp (argv[i], "r", 1) == 0)
        flags |= OSPF_DEBUG_RECV;
      else if (pal_strncmp (argv[i], "s", 1) == 0)
        flags |= OSPF_DEBUG_SEND;
      else if (pal_strncmp (argv[i], "de", 2) == 0)
        flags |= OSPF_DEBUG_DETAIL;
    }

  if (types == 0)
    types = OSPF_DEBUG_PACKET_ALL;

  if (flags == 0
      || flags == (OSPF_DEBUG_SEND|OSPF_DEBUG_RECV))
    flags = OSPF_DEBUG_SEND|OSPF_DEBUG_RECV|OSPF_DEBUG_DETAIL;
  else if (flags & OSPF_DEBUG_DETAIL)
    flags = OSPF_DEBUG_DETAIL;

  for (i = OSPF_PACKET_HELLO; i <= OSPF_PACKET_LS_ACK; i++)
    if (types & (1 << (i - 1)))
      DEBUG_PACKET_OFF (cli, i, flags);

  return CLI_SUCCESS;
}

ALI (no_debug_ospf_packet,
     undebug_ospf_packet_cmd,
     "undebug ospf packet ({hello|dd|ls-request|ls-update|ls-ack|send|recv|detail}|)",
     CLI_UNDEBUG_STR,
     CLI_OSPF_STR,
     "OSPF packets",
     "OSPF Hello",
     "OSPF Database Description",
     "OSPF Link State Request",
     "OSPF Link State Update",
     "OSPF Link State Acknowledgment",
     "Packet sent",
     "Packet received",
     "Detail information");

CLI (debug_ospf_ifsm,
     debug_ospf_ifsm_cmd,
     "debug ospf ifsm ({events|status|timers}|)",
     CLI_DEBUG_STR,
     CLI_OSPF_STR,
     "OSPF Interface State Machine",
     "IFSM Event Information",
     "IFSM Status Information",
     "IFSM Timer Information")
{
  struct ospf_master *om = cli->vr->proto;
  int i;

  if (argc == 0)
    DEBUG_ON (cli, ifsm, IFSM, "OSPF all IFSM");
  else
    for (i = 0; i < argc; i++)
      {
        if (pal_strncmp (argv[i], "e", 1) == 0)
          DEBUG_ON (cli, ifsm, IFSM_EVENTS, "OSPF IFSM event");
        else if (pal_strncmp (argv[i], "s", 1) == 0)
          DEBUG_ON (cli, ifsm, IFSM_STATUS, "OSPF IFSM status");
        else if (pal_strncmp (argv[i], "t", 1) == 0)
          DEBUG_ON (cli, ifsm, IFSM_TIMERS, "OSPF IFSM timer");
      }
  
  return CLI_SUCCESS;
}

CLI (no_debug_ospf_ifsm,
     no_debug_ospf_ifsm_cmd,
     "no debug ospf ifsm ({events|status|timers}|)",
     CLI_NO_STR,
     CLI_DEBUG_STR,
     CLI_OSPF_STR,
     "OSPF Interface State Machine",
     "IFSM Event Information",
     "IFSM Status Information",
     "IFSM Timer Information")
{
  struct ospf_master *om = cli->vr->proto;
  int i;

  if (argc == 0)
    DEBUG_OFF (cli, ifsm, IFSM, "OSPF all IFSM");
  else
    for (i = 0; i < argc; i++)
      {
        if (pal_strncmp (argv[i], "e", 1) == 0)
          DEBUG_OFF (cli, ifsm, IFSM_EVENTS, "OSPF IFSM event");
        else if (pal_strncmp (argv[i], "s", 1) == 0)
          DEBUG_OFF (cli, ifsm, IFSM_STATUS, "OSPF IFSM status");
        else if (pal_strncmp (argv[i], "t", 1) == 0)
          DEBUG_OFF (cli, ifsm, IFSM_TIMERS, "OSPF IFSM timer");
      }

  return CLI_SUCCESS;
}

ALI (no_debug_ospf_ifsm,
     undebug_ospf_ifsm_cmd,
     "undebug ospf ifsm ({events|status|timers}|)",
     CLI_UNDEBUG_STR,
     CLI_OSPF_STR,
     "OSPF Interface State Machine",
     "IFSM Event Information",
     "IFSM Status Information",
     "IFSM Timer Information");

CLI (debug_ospf_nfsm,
     debug_ospf_nfsm_cmd,
     "debug ospf nfsm ({events|status|timers}|)",
     CLI_DEBUG_STR,
     CLI_OSPF_STR,
     "OSPF Neighbor State Machine",
     "NFSM Event Information",
     "NFSM Status Information",
     "NFSM Timer Information")
{
  struct ospf_master *om = cli->vr->proto;
  int i;

  if (argc == 0)
    DEBUG_ON (cli, nfsm, NFSM, "OSPF all NFSM");
  else
    for (i = 0; i < argc; i++)
      {
        if (pal_strncmp (argv[i], "e", 1) == 0)
          DEBUG_ON (cli, nfsm, NFSM_EVENTS, "OSPF NFSM event");
        else if (pal_strncmp (argv[i], "s", 1) == 0)
          DEBUG_ON (cli, nfsm, NFSM_STATUS, "OSPF NFSM status");
        else if (pal_strncmp (argv[i], "t", 1) == 0)
          DEBUG_ON (cli, nfsm, NFSM_TIMERS, "OSPF NFSM timer");
      }
  
  return CLI_SUCCESS;
}

CLI (no_debug_ospf_nfsm,
     no_debug_ospf_nfsm_cmd,
     "no debug ospf nfsm ({events|status|timers}|)",
     CLI_NO_STR,
     CLI_DEBUG_STR,
     CLI_OSPF_STR,
     "OSPF Neighbor State Machine",
     "NFSM Event Information",
     "NFSM Status Information",
     "NFSM Timer Information")
{
  struct ospf_master *om = cli->vr->proto;
  int i;

  if (argc == 0)
    DEBUG_OFF (cli, nfsm, NFSM, "OSPF all NFSM");
  else
    for (i = 0; i < argc; i++)
      {
        if (pal_strncmp (argv[i], "e", 1) == 0)
          DEBUG_OFF (cli, nfsm, NFSM_EVENTS, "OSPF NFSM event");
        else if (pal_strncmp (argv[i], "s", 1) == 0)
          DEBUG_OFF (cli, nfsm, NFSM_STATUS, "OSPF NFSM status");
        else if (pal_strncmp (argv[i], "t", 1) == 0)
          DEBUG_OFF (cli, nfsm, NFSM_TIMERS, "OSPF NFSM timer");
      }
  
  return CLI_SUCCESS;
}

ALI (no_debug_ospf_nfsm,
     undebug_ospf_nfsm_cmd,
     "undebug ospf nfsm ({events|status|timers}|)",
     CLI_UNDEBUG_STR,
     CLI_OSPF_STR,
     "OSPF Neighbor State Machine",
     "NFSM Event Information",
     "NFSM Status Information",
     "NFSM Timer Information");

CLI (debug_ospf_lsa,
     debug_ospf_lsa_cmd,
     "debug ospf lsa ({flooding|generate|install|maxage|refresh}|)",
     CLI_DEBUG_STR,
     CLI_OSPF_STR,
     "OSPF Link State Advertisement",
     "LSA Flooding",
     "LSA Generation",
     "LSA Installation",
     "LSA MaxAge processing",
     "LSA Refreshment")
{
  struct ospf_master *om = cli->vr->proto;
  int i;

  if (argc == 0)
    DEBUG_ON (cli, lsa, LSA, "OSPF all LSA");
  else
    for (i = 0; i < argc; i++)
      {
        if (pal_strncmp (argv[i], "f", 1) == 0)
          DEBUG_ON (cli, lsa, LSA_FLOODING, "OSPF LSA flooding");
        else if (pal_strncmp (argv[i], "g", 1) == 0)
          DEBUG_ON (cli, lsa, LSA_GENERATE, "OSPF LSA generation");
        else if (pal_strncmp (argv[i], "i", 1) == 0)
          DEBUG_ON (cli, lsa, LSA_INSTALL, "OSPF LSA installation");
        else if (pal_strncmp (argv[i], "m", 1) == 0)
          DEBUG_ON (cli, lsa, LSA_MAXAGE, "OSPF LSA MaxAge processing");
        else if (pal_strncmp (argv[i], "r", 1) == 0)
          DEBUG_ON (cli, lsa, LSA_REFRESH, "OSPF LSA refreshment");
      }

  return CLI_SUCCESS;
}

CLI (no_debug_ospf_lsa,
     no_debug_ospf_lsa_cmd,
     "no debug ospf lsa ({flooding|generate|install|maxage|refresh}|)",
     CLI_NO_STR,
     CLI_DEBUG_STR,
     CLI_OSPF_STR,
     "OSPF Link State Advertisement",
     "LSA Flooding",
     "LSA Generation",
     "LSA Installation",
     "LSA MaxAge processing",
     "LSA Refreshment")
{
  struct ospf_master *om = cli->vr->proto;
  int i;

  if (argc == 0)
    DEBUG_OFF (cli, lsa, LSA, "OSPF all LSA");
  else
    for (i = 0; i < argc; i++)
      {
        if (pal_strncmp (argv[i], "f", 1) == 0)
          DEBUG_OFF (cli, lsa, LSA_FLOODING, "OSPF LSA flooding");
        else if (pal_strncmp (argv[i], "g", 1) == 0)
          DEBUG_OFF (cli, lsa, LSA_GENERATE, "OSPF LSA generation");
        else if (pal_strncmp (argv[i], "i", 1) == 0)
          DEBUG_OFF (cli, lsa, LSA_INSTALL, "OSPF LSA installation");
        else if (pal_strncmp (argv[i], "m", 1) == 0)
          DEBUG_OFF (cli, lsa, LSA_MAXAGE, "OSPF LSA MaxAge processing");
        else if (pal_strncmp (argv[i], "r", 1) == 0)
          DEBUG_OFF (cli, lsa, LSA_REFRESH, "OSPF LSA refreshment");
      }

  return CLI_SUCCESS;
}

ALI (no_debug_ospf_lsa,
     undebug_ospf_lsa_cmd,
     "undebug ospf lsa ({flooding|generate|install|maxage|refresh}|)",
     CLI_UNDEBUG_STR,
     CLI_OSPF_STR,
     "OSPF Link State Advertisement",
     "LSA Flooding",
     "LSA Generation",
     "LSA Installation",
     "LSA MaxAge processing",
     "LSA Refreshment");

CLI (debug_ospf_nsm,
     debug_ospf_nsm_cmd,
     "debug ospf nsm ({interface|redistribute}|)",
     CLI_DEBUG_STR,
     CLI_OSPF_STR,
     "OSPF NSM information",
     "NSM interface",
     "NSM redistribute")
{
  struct ospf_master *om = cli->vr->proto;
  int i;

  if (argc == 0)
    {
      DEBUG_ON (cli, nsm, NSM, "OSPF all NSM");
      ospf_nsm_debug_set (om);
    }
  else
    for (i = 0; i < argc; i++)
      {
        if (pal_strncmp (argv[i], "i", 1) == 0)
          DEBUG_ON (cli, nsm, NSM_INTERFACE, "OSPF NSM interface");
        else if (pal_strncmp (argv[i], "r", 1) == 0)
          DEBUG_ON (cli, nsm, NSM_REDISTRIBUTE, "OSPF NSM redistribute");
      }

  return CLI_SUCCESS;
}

CLI (no_debug_ospf_nsm,
     no_debug_ospf_nsm_cmd,
     "no debug ospf nsm ({interface|redistribute}|)",
     CLI_NO_STR,
     CLI_DEBUG_STR,
     CLI_OSPF_STR,
     "OSPF NSM information",
     "NSM interface",
     "NSM redistribute")
{
  struct ospf_master *om = cli->vr->proto;
  int i;

  if (argc == 0)
    {
      DEBUG_OFF (cli, nsm, NSM, "OSPF all NSM");
      ospf_nsm_debug_unset (om);
    }
  else
    for (i = 0; i < argc; i++)
      {
        if (pal_strncmp (argv[i], "i", 1) == 0)
          DEBUG_OFF (cli, nsm, NSM_INTERFACE, "OSPF NSM interface");
        else if (pal_strncmp (argv[i], "r", 1) == 0)
          DEBUG_OFF (cli, nsm, NSM_REDISTRIBUTE, "OSPF NSM redistribute");
      }

  return CLI_SUCCESS;
}

ALI (no_debug_ospf_nsm,
     undebug_ospf_nsm_cmd,
     "undebug ospf nsm ({interface|redistribute}|)",
     CLI_UNDEBUG_STR,
     CLI_OSPF_STR,
     "OSPF NSM information",
     "NSM interface",
     "NSM redistribute");

CLI (debug_ospf_events,
     debug_ospf_events_cmd,
     "debug ospf events ({abr|asbr|lsa|nssa|os|router|vlink}|)",
     CLI_DEBUG_STR,
     CLI_OSPF_STR,
     "OSPF events information",
     "OSPF ABR events",
     "OSPF ASBR events",
     "OSPF LSA events",
     "OSPF NSSA events",
     "OSPF OS interaction events",
     "Other router events",
     "OSPF Virtual-Link events")
{
  struct ospf_master *om = cli->vr->proto;
  int i;

  if (argc == 0)
    DEBUG_ON (cli, event, EVENT, "OSPF all events");
  else
    for (i = 0; i < argc; i++)
      {
        if (pal_strncmp (argv[i], "ab", 2) == 0)
          DEBUG_ON (cli, event, EVENT_ABR, "OSPF ABR events");
        else if (pal_strncmp (argv[i], "as", 2) == 0)
          DEBUG_ON (cli, event, EVENT_ASBR, "OSPF ASBR events");
        else if (pal_strncmp (argv[i], "l", 1) == 0)
          DEBUG_ON (cli, event, EVENT_LSA, "OSPF LSA events");
        else if (pal_strncmp (argv[i], "n", 1) == 0)
          DEBUG_ON (cli, event, EVENT_NSSA, "OSPF NSSA events");
        else if (pal_strncmp (argv[i], "o", 1) == 0)
          DEBUG_ON (cli, event, EVENT_OS, "OSPF OS interaction events");
        else if (pal_strncmp (argv[i], "r", 1) == 0)
          DEBUG_ON (cli, event, EVENT_ROUTER, "Other router events");
        else if (pal_strncmp (argv[i], "v", 1) == 0)
          DEBUG_ON (cli, event, EVENT_VLINK, "OSPF Virtual-Link events");
      }
  
  return CLI_SUCCESS;
}

CLI (no_debug_ospf_events,
     no_debug_ospf_events_cmd,
     "no debug ospf events ({abr|asbr|lsa|nssa|os|router|vlink}|)",
     CLI_NO_STR,
     CLI_DEBUG_STR,
     CLI_OSPF_STR,
     "OSPF event information",
     "OSPF ABR events",
     "OSPF ASBR events",
     "OSPF LSA events",
     "OSPF NSSA events",
     "OSPF OS interaction events",
     "Other router events",
     "OSPF Virtual-Link events")
{
  struct ospf_master *om = cli->vr->proto;
  int i;

  if (argc == 0)
    DEBUG_OFF (cli, event, EVENT, "OSPF all events");
  else
    for (i = 0; i < argc; i++)
      {
        if (pal_strncmp (argv[i], "ab", 2) == 0)
          DEBUG_OFF (cli, event, EVENT_ABR, "OSPF ABR events");
        else if (pal_strncmp (argv[i], "as", 2) == 0)
          DEBUG_OFF (cli, event, EVENT_ASBR, "OSPF ASBR events");
        else if (pal_strncmp (argv[i], "l", 1) == 0)
          DEBUG_OFF (cli, event, EVENT_LSA, "OSPF LSA events");
        else if (pal_strncmp (argv[i], "n", 1) == 0)
          DEBUG_OFF (cli, event, EVENT_NSSA, "OSPF NSSA events");
        else if (pal_strncmp (argv[i], "o", 1) == 0)
          DEBUG_OFF (cli, event, EVENT_OS, "OSPF OS interaction events");
        else if (pal_strncmp (argv[i], "r", 1) == 0)
          DEBUG_OFF (cli, event, EVENT_ROUTER, "Other router events");
        else if (pal_strncmp (argv[i], "v", 1) == 0)
          DEBUG_OFF (cli, event, EVENT_VLINK, "OSPF Virtual-Link events");
      }
  
  return CLI_SUCCESS;
}

ALI (no_debug_ospf_events,
     undebug_ospf_events_cmd,
     "undebug ospf events ({abr|asbr|lsa|nssa|os|router|vlink}|)",
     CLI_UNDEBUG_STR,
     CLI_OSPF_STR,
     "OSPF event information",
     "OSPF ABR events",
     "OSPF ASBR events",
     "OSPF LSA events",
     "OSPF NSSA events",
     "OSPF OS interaction events",
     "Other router events",
     "OSPF Virtual-Link events");

CLI (debug_ospf_route,
     debug_ospf_route_cmd,
     "debug ospf route ({ase|ia|install|spf}|)",
     CLI_DEBUG_STR,
     CLI_OSPF_STR,
     "OSPF route information",
     "External route calculation information",
     "Inter-Area route calculation information",
     "Route installation information",
     "SPF calculation information")
{
  struct ospf_master *om = cli->vr->proto;
  int i;

  if (argc == 0)
    DEBUG_ON (cli, rt_calc, RT_ALL, "OSPF all route calculation");
  else
    for (i = 0; i < argc; i++)
      {
        if (pal_strncmp (argv[i], "a", 1) == 0)
          DEBUG_ON (cli, rt_calc, RT_CALC_ASE,
                    "OSPF External route calculation");
        else if (pal_strncmp (argv[i], "ia", 2) == 0)
          DEBUG_ON (cli, rt_calc, RT_CALC_IA,
                    "OSPF Inter-Area route calculation");
        else if (pal_strncmp (argv[i], "in", 2) == 0)
          DEBUG_ON (cli, rt_calc, RT_INSTALL, "OSPF route installation");
        else if (pal_strncmp (argv[i], "s", 1) == 0)
          DEBUG_ON (cli, rt_calc, RT_CALC_SPF, "OSPF SPF calculation");
      }
  
  return CLI_SUCCESS;
}

CLI (no_debug_ospf_route,
     no_debug_ospf_route_cmd,
     "no debug ospf route ({ase|ia|install|spf}|)",
     CLI_NO_STR,
     CLI_DEBUG_STR,
     CLI_OSPF_STR,
     "OSPF route information",
     "External route calculation information",
     "Inter-Area route calculation information",
     "Route installation information",
     "SPF calculation information")
{
  struct ospf_master *om = cli->vr->proto;
  int i;

  if (argc == 0)
    DEBUG_OFF (cli, rt_calc, RT_ALL, "OSPF all route calculation");
  else
    for (i = 0; i < argc; i++)
      {
        if (pal_strncmp (argv[i], "a", 1) == 0)
          DEBUG_OFF (cli, rt_calc, RT_CALC_ASE,
                     "OSPF External route calculation");
        else if (pal_strncmp (argv[i], "ia", 2) == 0)
          DEBUG_OFF (cli, rt_calc, RT_CALC_IA,
                     "OSPF Inter-Area route calculation");
        else if (pal_strncmp (argv[i], "in", 2) == 0)
          DEBUG_OFF (cli, rt_calc, RT_INSTALL, "OSPF route installation");
        else if (pal_strncmp (argv[i], "s", 1) == 0)
          DEBUG_OFF (cli, rt_calc, RT_CALC_SPF, "OSPF SPF calculation");
      }
  
  return CLI_SUCCESS;
}

ALI (no_debug_ospf_route,
     undebug_ospf_route_cmd,
     "undebug ospf route ({ase|ia|install|spf}|)",
     CLI_UNDEBUG_STR,
     CLI_OSPF_STR,
     "OSPF route information",
     "External route calculation information",
     "Inter-Area route calculation information",
     "Route installation information",
     "SPF calculation information");

#ifdef HAVE_BFD
CLI (debug_ospf_bfd,
     debug_ospf_bfd_cmd,
     "debug ospf bfd",
     CLI_DEBUG_STR,
     CLI_OSPF_STR,
     CLI_BFD_CMD_STR)
{
  struct ospf_master *om = cli->vr->proto;

  DEBUG_ON (cli, bfd, BFD, "OSPF BFD");
  ospf_bfd_debug_set (om);

  return CLI_SUCCESS;
}

CLI (no_debug_ospf_bfd,
     no_debug_ospf_bfd_cmd,
     "no debug ospf bfd",
     CLI_NO_STR,
     CLI_DEBUG_STR,
     CLI_OSPF_STR,
     CLI_BFD_CMD_STR)
{
  struct ospf_master *om = cli->vr->proto;

  DEBUG_OFF (cli, bfd, BFD, "OSPF BFD");
  ospf_bfd_debug_unset (om);

  return CLI_SUCCESS;
}

ALI (no_debug_ospf_bfd,
     undebug_ospf_bfd_cmd,
     "undebug ospf bfd",
     CLI_UNDEBUG_STR,
     CLI_OSPF_STR,
     CLI_BFD_CMD_STR);
#endif /* HAVE_BFD */ 

#ifdef HAVE_OSPF_CSPF
CLI (debug_cspf_event,
     debug_cspf_event_cmd,
     "debug cspf events",
     CLI_DEBUG_STR,
     "CSPF Information",
     "CSPF event information")
{
  struct apn_vr *vr = cli->vr;
  struct ospf_master *om = vr->proto;

  if (cli->mode == CONFIG_MODE)
    CSPF_CONF_DEBUG_ON (&om->debug_cspf, EVENT);
  CSPF_TERM_DEBUG_ON (&om->debug_cspf, EVENT);

  return CLI_SUCCESS;
}

CLI (debug_cspf_hexdump,
     debug_cspf_hexdump_cmd,
     "debug cspf hexdump",
     CLI_DEBUG_STR,
     "CSPF Information",
     "CSPF message hexdump")
{
  struct apn_vr *vr = cli->vr;
  struct ospf_master *om = vr->proto;

  if (cli->mode == CONFIG_MODE)
    CSPF_CONF_DEBUG_ON (&om->debug_cspf, HEXDUMP);
  CSPF_TERM_DEBUG_ON (&om->debug_cspf, HEXDUMP);

  return CLI_SUCCESS;
}

CLI (no_debug_cspf_event,
     no_debug_cspf_event_cmd,
     "no debug cspf events",
     CLI_NO_STR,
     CLI_DEBUG_STR,
     "Disable CSPF debugging",
     "Disable CSPF debugging for events")
{
  struct apn_vr *vr = cli->vr;
  struct ospf_master *om = vr->proto;

  if (cli->mode == CONFIG_MODE)
    CSPF_CONF_DEBUG_OFF (&om->debug_cspf, EVENT);
  CSPF_TERM_DEBUG_OFF (&om->debug_cspf, EVENT);

  return CLI_SUCCESS;
}

ALI (no_debug_cspf_event,
     undebug_cspf_event_cmd,
     "undebug cspf events",
     CLI_UNDEBUG_STR,
     "Disable CSPF debugging",
     "Disable CSPF debugging for events");

CLI (no_debug_cspf_hexdump,
     no_debug_cspf_hexdump_cmd,
     "no debug cspf hexdump",
     CLI_NO_STR,
     CLI_DEBUG_STR,
     "Disable CSPF debugging",
     "Disable CSPF message hexdump debugging")
{
  struct apn_vr *vr = cli->vr;
  struct ospf_master *om = vr->proto;

  if (cli->mode == CONFIG_MODE)
    CSPF_CONF_DEBUG_OFF (&om->debug_cspf, HEXDUMP);
  CSPF_TERM_DEBUG_OFF (&om->debug_cspf, HEXDUMP);

  return CLI_SUCCESS;
}

ALI (no_debug_cspf_hexdump,
     undebug_cspf_hexdump_cmd,
     "undebug cspf hexdump",
     CLI_UNDEBUG_STR,
     "Disable CSPF debugging",
     "Disable CSPF message hexdump debugging");
#endif /* HAVE_OSPF_CSPF */

CLI (ospf_db_summary_opt,
     ospf_enable_db_summary_opt_cmd,
     "enable db-summary-opt",
     "Enable specific OSPF feature",
     "Enable OSPF Database Summary Optimization")
{
  struct ospf *top = cli->index;
  int ret;

  ret = ospf_enable_db_summary_opt (cli->vr->id, top->ospf_id);
  if (ret != OSPF_API_SET_SUCCESS)
    return ospf_cli_return (cli, ret);

  return CLI_SUCCESS;
}

CLI (no_ospf_db_summary_opt,
     no_ospf_enable_db_summary_opt_cmd,
     "no enable db-summary-opt",
     "Disable specific OSPF feature",
     "Disable OSPF Database Summary Optimization")
{
  struct ospf *top = cli->index;
  int ret;

  ret = ospf_disable_db_summary_opt (cli->vr->id, top->ospf_id);
  if (ret != OSPF_API_SET_SUCCESS)
    return ospf_cli_return (cli, ret);
  
  return CLI_SUCCESS;
} 

/* Configuration write related functions and definition. */

char *ospf_auth_type_str[] =
{
  " null",
  "",
  " message-digest",
};

char *ospf_shortcut_mode_str[] = 
{
  "default",
  "enable",
  "disable"
};

int
ospf_config_write_network_area (struct ls_node *rn, struct cli *cli)
{
  char area_str[OSPF_AREA_STRING_MAXLEN];
  struct ospf_network *nw;
  struct pal_in4_addr mask;

  if (rn != NULL)
    {
      if (rn->l_left)
        ospf_config_write_network_area (rn->l_left, cli);

      if (rn->l_right)
        ospf_config_write_network_area (rn->l_right, cli);

      if ((nw = RN_INFO (rn, RNI_DEFAULT)))
        {
          if (nw->wildcard == OSPF_NETWORK_MASK_WILD)
            {
              masklen2ip (rn->p->prefixlen, &mask);
              mask.s_addr = ~mask.s_addr;
              ospf_area_fmt_string (nw->area_id, nw->format, area_str);

#ifdef HAVE_OSPF_MULTI_INST
              /* Display instance-id if it is non-zero value */
              if (nw->inst_id != OSPF_IF_INSTANCE_ID_DEFAULT)
                cli_out (cli, " network %r %r area %s instance-id %d\n",
                         rn->p->prefix, &mask, area_str, nw->inst_id);
              else
#endif /* HAVE_OSPF_MULTI_INST */
                cli_out (cli, " network %r %r area %s\n",
                         rn->p->prefix, &mask, area_str);
            }
          else
            {
#ifdef HAVE_OSPF_MULTI_INST
              if (nw->inst_id != OSPF_IF_INSTANCE_ID_DEFAULT)
                cli_out (cli, " network %P area %s instance-id %d\n", rn->p,
                         ospf_area_fmt_string (nw->area_id, nw->format, 
                                               area_str), nw->inst_id);
              else
#endif /* HAVE_OSPF_MULTI_INST */
                cli_out (cli, " network %P area %s\n", rn->p,
                         ospf_area_fmt_string (nw->area_id, nw->format, area_str));
            }
        }
    }

  return 0;
}

#ifdef HAVE_VRF_OSPF
int
ospf_config_write_domain_info (struct ospf *top, struct cli *cli)
{
  u_char domain_id[16];
  struct ospf_vrf_domain_id *sdomain_id;
  struct listnode *nn;
  bool_t no_match;
  int i;

  /* Primary domain-id */
  if (CHECK_FLAG (top->config, OSPF_CONFIG_DOMAIN_ID_PRIMARY))
    {
      no_match = PAL_FALSE;
      switch (top->pdomain_id->type)
       {
         case TYPE_IP_DID:
           pal_inet_ntop (AF_INET, top->pdomain_id->value, domain_id, 32);
           cli_out (cli, " domain-id %s\n",domain_id );
           break;
         case TYPE_AS_DID:
           cli_out (cli, " domain-id type type-as value ");
           break;
         case TYPE_AS4_DID:
           cli_out (cli, " domain-id type  type-as4 value ");
           break;
         case TYPE_BACK_COMP_DID:
           cli_out (cli, " domain-id type type-back-comp value ");
           break;
         default:
           no_match = PAL_TRUE;
           break;
       }
      if (!no_match && (top->pdomain_id->type != TYPE_IP_DID))
        {
          for (i =0;i<6; i++)
            cli_out (cli,"%x",top->pdomain_id->value[i]);
          cli_out (cli, "\n");
        }
    }

  /* Secondary domain-Id */
  if (CHECK_FLAG (top->config, OSPF_CONFIG_DOMAIN_ID_SEC))
    {
      no_match = PAL_FALSE;
      LIST_LOOP (top->domainid_list, sdomain_id, nn)
        {
          switch (sdomain_id->type)
           {
             case TYPE_IP_DID:
               pal_inet_ntop (AF_INET, sdomain_id->value, domain_id, 32);
               cli_out (cli, " domain-id %s secondary \n",domain_id );
               break;
             case TYPE_AS_DID:
               cli_out (cli, " domain-id type type-as value ");
               break;
             case TYPE_AS4_DID:
               cli_out (cli, " domain-id type  type-as4 value ");
               break;
             case TYPE_BACK_COMP_DID:
               cli_out (cli, " domain-id type type-back-comp value ");
               break;
             default:
               no_match = PAL_TRUE;
               break;
           }

         if (!no_match && (sdomain_id->type != TYPE_IP_DID))
           {
             for (i=0;i<6; i++)
               cli_out (cli,"%x",sdomain_id->value[i]);
             cli_out (cli, " secondary\n");
           }
        }
    }
  /* NULL domain-Id */
  if (CHECK_FLAG (top->config, OSPF_CONFIG_NULL_DOMAIN_ID))
      cli_out (cli, " domain-id NULL \n");

  return 0;
}
#endif /*HAVE_VRF_OSPF */

int
ospf_config_write_host_area (struct ospf *top, struct cli *cli)
{
  char area_str[OSPF_AREA_STRING_MAXLEN];
  struct ospf_host_entry *host;
  struct ls_node *rn;

  /* `host area' print. */
  for (rn = ls_table_top (top->host_table); rn; rn = ls_route_next (rn))
    if ((host = RN_INFO (rn, RNI_DEFAULT)))
      {
        cli_out (cli, " host %r area %s", &host->addr,
                 ospf_area_fmt_string (host->area_id, host->format, area_str));
        if (host->metric > 0)
          cli_out (cli, " cost %d", host->metric);
        cli_out (cli, "\n");
      }
  return 0;
}

int
ospf_config_write_area (struct ospf *top, struct cli *cli)
{
  char area_id_str[OSPF_AREA_STRING_MAXLEN];
  struct ospf_area_range *range;
  struct ospf_area *area;
  struct ls_node *r1, *r2;
  u_char auth_type, mode;
#ifdef HAVE_NSSA
  u_char role;
#endif /* HAVE_NSSA */

  /* Area configuration print. */
  for (r1 = ls_table_top (top->area_table); r1; r1 = ls_route_next (r1))
    if ((area = RN_INFO (r1, RNI_DEFAULT)))
      {
        auth_type = ospf_area_auth_type_get (area);
        mode = ospf_area_shortcut_get (area);
        ospf_area_fmt_string (area->area_id, area->conf.format, area_id_str);

        /* `area authentication'. */
        switch (auth_type)
          {
          case OSPF_AREA_AUTH_NULL:
            break;
          case OSPF_AREA_AUTH_SIMPLE:
            cli_out (cli, " area %s authentication\n", area_id_str);
            break;
          case OSPF_AREA_AUTH_CRYPTOGRAPHIC:
            cli_out (cli, " area %s authentication message-digest\n",
                     area_id_str);
            break;
          }

        if (OSPF_AREA_CONF_CHECK (area, SHORTCUT_ABR))
          cli_out (cli, " area %s shortcut %s\n", area_id_str,
                   ospf_shortcut_mode_str[mode]);

        if (!IS_AREA_DEFAULT (area))
          {
            if (IS_AREA_STUB (area))
              cli_out (cli, " area %s stub", area_id_str);

#ifdef HAVE_NSSA
            else if (IS_AREA_NSSA (area))
              {
                role = ospf_nssa_translator_role_get (area);

                cli_out (cli, " area %s nssa", area_id_str);
                if (role == OSPF_NSSA_TRANSLATE_ALWAYS)
                  cli_out (cli, " translator-role always");
                else if (role == OSPF_NSSA_TRANSLATE_NEVER)
                  cli_out (cli, " translator-role never");

                if (OSPF_AREA_CONF_CHECK (area, NO_REDISTRIBUTION))
                  cli_out (cli, " no-redistribution");

                if (OSPF_AREA_CONF_CHECK (area, DEFAULT_ORIGINATE))
                  {
                    cli_out (cli, " default-information-originate");
                    if (OSPF_AREA_CONF_CHECK (area, METRIC))
                      cli_out (cli, " metric %lu", area->conf.metric);
                    if (OSPF_AREA_CONF_CHECK (area, METRIC_TYPE))
                      cli_out (cli, " metric-type %d", area->conf.metric_type);
                  }
              }
#endif /* HAVE_NSSA */

            if (OSPF_AREA_CONF_CHECK (area, NO_SUMMARY))
              cli_out (cli, " no-summary");

            cli_out (cli, "\n");

            if (OSPF_AREA_CONF_CHECK (area, DEFAULT_COST))
              cli_out (cli, " area %s default-cost %d\n", area_id_str,
                       OSPF_AREA_DEFAULT_COST (area));
          }

        for (r2 = ls_table_top (area->ranges); r2; r2 = ls_route_next (r2))
          if ((range = RN_INFO (r2, RNI_DEFAULT)))
            {
              cli_out (cli, " area %s range %P", area_id_str, range->lp);

              if (!CHECK_FLAG (range->flags, OSPF_AREA_RANGE_ADVERTISE))
                cli_out (cli, " not-advertise");

              if (CHECK_FLAG (range->flags, OSPF_AREA_RANGE_SUBSTITUTE))
                cli_out (cli, " substitute %P", range->subst);

              cli_out (cli, "\n");
            }

        if (ALIST_NAME (area, FILTER_IN))
          cli_out (cli, " area %s filter-list access %s in\n", area_id_str,
                   ALIST_NAME (area, FILTER_IN));

        if (ALIST_NAME (area, FILTER_OUT))
          cli_out (cli, " area %s filter-list access %s out\n", area_id_str,
                   ALIST_NAME (area, FILTER_OUT));

        if (PLIST_NAME (area, FILTER_IN))
          cli_out (cli, " area %s filter-list prefix %s in\n", area_id_str,
                   PLIST_NAME (area, FILTER_IN));

        if (PLIST_NAME (area, FILTER_OUT))
          cli_out (cli, " area %s filter-list prefix %s out\n", area_id_str,
                   PLIST_NAME (area, FILTER_OUT));

        ospf_call_notifiers (top->om, OSPF_NOTIFY_WRITE_AREA_CONFIG,
                             cli, area);
      }

  return 0;
}

int
ospf_is_neighbor_in_nbma (struct ospf* top, struct pal_in4_addr addr)
{
  struct ospf_interface *oi;
  struct ls_node *rn;
  struct ls_prefix p;
  int flag = 0;

  ls_prefix_ipv4_set (&p, IPV4_MAX_BITLEN, addr);

  for (rn = ls_table_top (top->if_table); rn; rn = ls_route_next (rn))
    if ((oi = RN_INFO (rn, RNI_DEFAULT)))
    {
      flag = 1;
      if (IS_OSPF_NETWORK_NBMA (oi))
        if (prefix_match (oi->ident.address, (struct prefix *)&p))
          return PAL_TRUE;
    }

  if (!flag)
    return PAL_TRUE;

  return PAL_FALSE;
}

int
ospf_config_write_nbr_static (struct ospf *top, struct cli *cli)
{
  struct ospf_nbr_static *nbr_static;
  struct ls_node *rn;

  /* Static Neighbor configuration print. */
  for (rn = ls_table_top (top->nbr_static); rn; rn = ls_route_next (rn))
    if ((nbr_static = RN_INFO (rn, RNI_DEFAULT)))
      {
        if (nbr_static->flags == 0)
          cli_out (cli, " neighbor %r\n", &nbr_static->addr);
        else
          {
            if (CHECK_FLAG (nbr_static->flags, OSPF_NBR_STATIC_PRIORITY)
                || CHECK_FLAG (nbr_static->flags,
                               OSPF_NBR_STATIC_POLL_INTERVAL))
              {
                cli_out (cli, " neighbor %r", &nbr_static->addr);
                if (CHECK_FLAG (nbr_static->flags, OSPF_NBR_STATIC_PRIORITY))
                  cli_out (cli, " priority %d", nbr_static->priority);

                if (CHECK_FLAG (nbr_static->flags,
                                OSPF_NBR_STATIC_POLL_INTERVAL))
                  cli_out (cli, " poll-interval %d",
                           nbr_static->poll_interval);

                cli_out (cli, "\n");
              }

            if (CHECK_FLAG (nbr_static->flags, OSPF_NBR_STATIC_COST))
              cli_out (cli, " neighbor %r cost %d\n",
                       &nbr_static->addr, nbr_static->cost);
          }
      }

  return 0;
}

#define OSPF_AREA_VLINK_STRING_MAXLEN 50
int
ospf_config_write_vlink (struct ospf *top, struct cli *cli)
{
  char area_str[OSPF_AREA_STRING_MAXLEN];
  char buf[OSPF_AREA_VLINK_STRING_MAXLEN];
  struct ospf_if_params *oip;
  struct ospf_crypt_key *ck;
  struct ospf_vlink *vlink;
  struct ls_node *rn;
  struct listnode *node;

  /* Virtual-Link print. */
  for (rn = ls_table_top (top->vlink_table); rn; rn = ls_route_next (rn))
    if ((vlink = RN_INFO (rn, RNI_DEFAULT)))
      {
        oip = vlink->oi->params_default;

        zsnprintf (buf, OSPF_AREA_VLINK_STRING_MAXLEN,
                   "area %s virtual-link %r",
                   ospf_area_fmt_string (vlink->area_id, vlink->format,
                                         area_str),
                   &vlink->peer_id);

        if (oip == NULL || oip->config == 0)
          cli_out (cli, " %s\n", buf);
        else
          {
            if (OSPF_IF_PARAM_CHECK (oip, HELLO_INTERVAL)
                || OSPF_IF_PARAM_CHECK (oip, DEAD_INTERVAL)
                || OSPF_IF_PARAM_CHECK (oip, RETRANSMIT_INTERVAL)
                || OSPF_IF_PARAM_CHECK (oip, TRANSMIT_DELAY)
#ifdef HAVE_BFD
                || OSPF_IF_PARAM_CHECK (oip, BFD)
                || OSPF_IF_PARAM_CHECK (oip, BFD_DISABLE)
#endif /* HAVE_BFD */
                || OSPF_IF_PARAM_CHECK (oip, AUTH_TYPE)
                || OSPF_IF_PARAM_CHECK (oip, AUTH_SIMPLE))
              {
                cli_out (cli, " %s", buf);

                /* Hello interval. */
                if (OSPF_IF_PARAM_CHECK (oip, HELLO_INTERVAL))
                  cli_out (cli, " hello-interval %d", oip->hello_interval);
                /* Dead interval. */
                if (OSPF_IF_PARAM_CHECK (oip, DEAD_INTERVAL))
                  cli_out (cli, " dead-interval %d", oip->dead_interval);
                /* Retransmit interval. */
                if (OSPF_IF_PARAM_CHECK (oip, RETRANSMIT_INTERVAL))
                  cli_out (cli, " retransmit-interval %d",
                           oip->retransmit_interval);
                /* Transmit delay. */
                if (OSPF_IF_PARAM_CHECK (oip, TRANSMIT_DELAY))
                  cli_out (cli, " transmit-delay %d", oip->transmit_delay);
                /* Authentication type. */
                if (OSPF_IF_PARAM_CHECK (oip, AUTH_TYPE))
                  cli_out (cli, " authentication%s",
                           ospf_auth_type_str[oip->auth_type]);
                /* Authentication key. */
                if (OSPF_IF_PARAM_CHECK (oip, AUTH_SIMPLE))
                  cli_out (cli, " authentication-key %s", oip->auth_simple);
#ifdef HAVE_BFD
                /* BFD.  */
                if (OSPF_IF_PARAM_CHECK (oip, BFD_DISABLE))
                  cli_out (cli, " bfd disable");
                /* BFD.  */
                else if (OSPF_IF_PARAM_CHECK (oip, BFD))
                  cli_out (cli, " fall-over bfd");
#endif /* HAVE_BFD */

                cli_out (cli, "\n");
              }

            /* MD5 keys. */
            if (OSPF_IF_PARAM_CHECK (oip, AUTH_CRYPT))
              LIST_LOOP (oip->auth_crypt, ck, node)
                cli_out (cli, " %s message-digest-key %d md5 %s\n", buf,
                         ck->key_id, ck->auth_key);
          }
      }

  return 0;
}
#ifdef HAVE_OSPF_MULTI_AREA
s_int32_t
ospf_config_write_multi_area_adj_link (struct ospf *top, struct cli *cli)
{
  struct ospf_multi_area_link *mlink = NULL;
  char area_str[OSPF_AREA_STRING_MAXLEN];
  struct ls_node *rn = NULL;

  for (rn = ls_table_top (top->multi_area_link_table); rn;
                               rn = ls_route_next (rn))
    if ((mlink = RN_INFO (rn, RNI_DEFAULT)))
      {
        ospf_area_fmt_string (mlink->area_id, mlink->format,
                              area_str);
        cli_out (cli, " area %s multi-area-adjacency %s neighbor %-15r\n",
                 area_str, mlink->ifname, &mlink->nbr_addr);
      }
  return 0;
}
#endif /* HAVE_OSPF_MULTI_AREA */

int
ospf_config_write_redistribute (struct ospf *top, struct cli *cli)
{
  int type;
  struct ospf_redist_conf *rc;

  /* redistribute print. */
  for (type = 1; type < APN_ROUTE_MAX; type++)
   /* Printing redist conf info for every Instance */
   for (rc = &top->redist[type]; rc ;rc = rc->next)
    if (REDIST_PROTO_CHECK (rc))
      {
        if (rc->pid)
          cli_out (cli, " redistribute %s %d",
                 LOOKUP (ospf_nsm_route_type_msg, type),INST_VALUE (rc));
        else
          cli_out (cli, " redistribute %s",
                 LOOKUP (ospf_nsm_route_type_msg, type));
        if (CHECK_FLAG (rc->flags, OSPF_REDIST_METRIC))
          cli_out (cli, " metric %d", METRIC_VALUE (rc));
        
        if (CHECK_FLAG (rc->flags, OSPF_REDIST_METRIC_TYPE))
          cli_out (cli, " metric-type %d", METRIC_TYPE (rc));

        if (CHECK_FLAG (rc->flags, OSPF_REDIST_TAG) && REDIST_TAG (rc) != 0)
          cli_out (cli, " tag %lu", REDIST_TAG (rc));

        if (CHECK_FLAG (rc->flags, OSPF_REDIST_ROUTE_MAP))
          cli_out (cli, " route-map %s", RMAP_NAME (rc));

        cli_out (cli, "\n");
      }

  return 0;
}

int
ospf_config_write_default_metric (struct ospf *top, struct cli *cli)
{
  if (CHECK_FLAG (top->config, OSPF_CONFIG_DEFAULT_METRIC))
    cli_out (cli, " default-metric %lu\n", top->default_metric);

  return 0;
}

int
ospf_config_write_distribute (struct ospf *top, struct cli *cli)
{
  int type;
  struct ospf_redist_conf *rc; 

  /* distribute-list print. */
  for (type = 0; type < APN_ROUTE_MAX; type++)
   for (rc = &top->redist[type]; rc; rc = rc->next)
    if (CHECK_FLAG (rc->flags, OSPF_REDIST_FILTER_LIST))
      {
        if (type == APN_ROUTE_OSPF)
        {
   	    if (rc->pid != 0)
               {
                 cli_out (cli, " distribute-list %s out %s %d\n", 
               		    DIST_NAME (rc),
                    	    LOOKUP (ospf_nsm_route_type_msg, type), rc->pid);
 	       }
 	    else
 	       {
 	         cli_out (cli, " distribute-list %s out %s\n",
                             DIST_NAME (rc),
                             LOOKUP (ospf_nsm_route_type_msg, type));
 	       }
        }
	else 
          cli_out (cli, " distribute-list %s out %s\n", 
                   DIST_NAME (rc), LOOKUP (ospf_nsm_route_type_msg, type));
      }

  if (CHECK_FLAG (top->dist_in.flags, OSPF_DIST_LIST_IN))
    {
      cli_out (cli, " distribute-list %s in \n",
               DIST_NAME (&top->dist_in));
    }

      /* default-information print. */
  if (REDIST_PROTO_CHECK (&top->redist[APN_ROUTE_DEFAULT]))
    {
      cli_out (cli, " default-information originate");
      if (top->default_origin == OSPF_DEFAULT_ORIGINATE_ALWAYS)
        cli_out (cli, " always");

      if (CHECK_FLAG (top->redist[APN_ROUTE_DEFAULT].flags,OSPF_REDIST_METRIC))
        cli_out (cli, " metric %d",
                 METRIC_VALUE (&top->redist[APN_ROUTE_DEFAULT]));

      if (CHECK_FLAG (top->redist[APN_ROUTE_DEFAULT].flags,
                      OSPF_REDIST_METRIC_TYPE))
        cli_out (cli, " metric-type %d",
                 METRIC_TYPE (&top->redist[APN_ROUTE_DEFAULT]));

      if (CHECK_FLAG (top->redist[APN_ROUTE_DEFAULT].flags,
                      OSPF_REDIST_ROUTE_MAP))
        cli_out (cli, " route-map %s",
                 RMAP_NAME (&top->redist[APN_ROUTE_DEFAULT]));
          
      cli_out (cli, "\n");
    }
  
  return 0;
}

int
ospf_config_write_distance (struct ospf *top, struct cli *cli)
{
  struct ls_node *rn;
  struct ospf_distance *odistance;
  bool_t intra = PAL_FALSE;
  bool_t inter = PAL_FALSE;
  bool_t external = PAL_FALSE;

  if (top->distance_all && top->distance_all != APN_DISTANCE_OSPF)
    cli_out (cli, " distance %d\n", top->distance_all);

  if (top->distance_intra && (top->distance_intra != APN_DISTANCE_OSPF))
    intra = PAL_TRUE;
      
  if (top->distance_inter && (top->distance_inter != APN_DISTANCE_OSPF))
    inter =PAL_TRUE;

  if (top->distance_external && (top->distance_external != APN_DISTANCE_OSPF))
    external = PAL_TRUE;
  
  if (intra || inter || external) 
      {
        cli_out (cli, " distance ospf");
  
        if (intra)
      cli_out (cli, " intra-area %d", top->distance_intra);
        if (inter)
      cli_out (cli, " inter-area %d", top->distance_inter);
        if (external)
      cli_out (cli, " external %d", top->distance_external);
  
        cli_out (cli, "\n");
      }
  
  for (rn = ls_table_top (top->distance_table); rn; rn = ls_route_next (rn))
    if ((odistance = RN_INFO (rn, RNI_DEFAULT)))
      {
        cli_out (cli, " distance %d %s/%d %s\n", odistance->distance,
                 rn->p->prefix[0], rn->p->prefix[1],
                 rn->p->prefix[2], rn->p->prefix[3], rn->p->prefixlen,
                 odistance->access_list ? odistance->access_list : "");
      }
  return 0;
}

int
ospf_config_write_summary_address (struct ospf *top, struct cli *cli)
{
  struct ospf_summary *summary;
  struct ls_node *rn;
  
  for (rn = ls_table_top (top->summary); rn; rn = ls_route_next (rn))
    if ((summary = RN_INFO (rn, RNI_DEFAULT)))
      {
        cli_out (cli, " summary-address %P", summary->lp);
      
        if (!CHECK_FLAG (summary->flags, OSPF_AREA_RANGE_ADVERTISE))
          cli_out (cli, " not-advertise");
      
        if (summary->tag)
          cli_out (cli, " tag %lu", summary->tag);

        cli_out (cli, "\n");
      }

  return 0;
}

/* OSPF configuration write function. */
int
ospf_config_write_instance (struct cli *cli, struct ospf *top)
{
  struct listnode *node;
  
  int write = 0;

  /* `router ospf' print. */
  if (top->ospf_id == 0)
    cli_out (cli, "router ospf\n");
  else if (IS_APN_VRF_DEFAULT (top->ov->iv))
    cli_out (cli, "router ospf %d\n", top->ospf_id);
#ifdef HAVE_VRF_OSPF
  else if (OSPF_CAP_HAVE_VRF_OSPF)
    cli_out (cli, "router ospf %d %s\n", top->ospf_id, top->ov->iv->name);
#endif /* HAVE_VRF_OSPF */
  write++;

  /* Router ID print. */
  if (CHECK_FLAG (top->config, OSPF_CONFIG_ROUTER_ID))
    cli_out (cli, " ospf router-id %r\n", &top->router_id_config);

  /* ABR type print. */
  if (top->abr_type != OSPF_ABR_TYPE_DEFAULT)
    cli_out (cli, " ospf abr-type %s\n", ospf_abr_type_string (top->abr_type));

  /* RFC1583 compatibility flag print. */
  if (IS_RFC1583_COMPATIBLE (top))
    cli_out (cli, " compatible rfc1583\n");

  /* auto-cost reference-bandwidth configuration.  */
  if (top->ref_bandwidth != OSPF_DEFAULT_REF_BANDWIDTH)
    cli_out (cli, " auto-cost reference-bandwidth %d\n",
             top->ref_bandwidth / 1000);

  if (top->max_dd != OSPF_MAX_CONCURRENT_DD_DEFAULT)
    cli_out (cli, " max-concurrent-dd %hu\n", top->max_dd);

  if (top->op_unuse_max != OSPF_PACKET_UNUSE_MAX_DEFAULT)
    cli_out (cli, " max-unuse-packet %lu\n", top->op_unuse_max);

  if (top->lsa_unuse_max != OSPF_LSA_UNUSE_MAX_DEFAULT)
    cli_out (cli, " max-unuse-lsa %lu\n", top->lsa_unuse_max);

#ifdef HAVE_OPAQUE_LSA
  if (!IS_OPAQUE_CAPABLE (top))
    cli_out (cli, " no capability opaque\n");
#endif /* HAVE_OPAQUE_LSA */

#ifdef HAVE_RESTART
  if (CHECK_FLAG (top->config, OSPF_CONFIG_RESTART_METHOD))
    {
      if (top->restart_method == OSPF_RESTART_METHOD_NEVER)
        cli_out (cli, " no capability restart\n");
      else if (top->restart_method == OSPF_RESTART_METHOD_SIGNALING)
        cli_out (cli, " capability restart signaling\n");
    }
#endif /* HAVE_RESTART */

  if (CHECK_FLAG (top->config, OSPF_CONFIG_DB_SUMMARY_OPT))
    cli_out (cli, " enable db-summary-opt\n");

#ifdef HAVE_BFD
  if (CHECK_FLAG (top->config, OSPF_CONFIG_BFD))
      cli_out (cli, " bfd all-interfaces\n");
#endif /* HAVE_BFD */

  /* SPF timers print. */
  if ((top->spf_min_delay.tv_sec != OSPF_SPF_MIN_DELAY_DEFAULT) ||
      (top->spf_max_delay.tv_sec != OSPF_SPF_MAX_DELAY_DEFAULT))
    cli_out (cli, " timers spf exp %lu %lu\n",
             (top->spf_min_delay.tv_sec * ONE_SEC_MILLISECOND) + 
             (top->spf_min_delay.tv_usec / MILLISEC_TO_MICROSEC),
             (top->spf_max_delay.tv_sec * ONE_SEC_MILLISECOND) + 
             (top->spf_max_delay.tv_usec / MILLISEC_TO_MICROSEC));

  /* SPF refresh parameters print. */
  if (top->lsa_refresh.interval != OSPF_LSA_REFRESH_INTERVAL_DEFAULT)
    cli_out (cli, " refresh timer %d\n",
             top->lsa_refresh.interval);

  /* Redistribute information print. */
  ospf_config_write_redistribute (top, cli);

  /* Passive-interface print. */
  if (top->passive_if_default == PAL_TRUE)
    {
      cli_out(cli, " passive-interface\n");
      for (node = LISTHEAD (top->no_passive_if); node; NEXTNODE (node))
        {
          struct ospf_passive_if *opi = GETDATA (node);
          if (!opi)
          continue;
          if (opi->addr.s_addr == 0)
            cli_out (cli, " no passive-interface %s\n", opi->name);
          else
            cli_out (cli, " no passive-interface %s %r\n", opi->name, &opi->addr);
        } 
    }

  else
    {
      for (node = LISTHEAD (top->passive_if); node; NEXTNODE (node))
        {
          struct ospf_passive_if *opi = GETDATA (node);
          if (!opi)
          continue;

          if (opi->addr.s_addr == 0)
            cli_out (cli, " passive-interface %s\n", opi->name);
          else
            cli_out (cli, " passive-interface %s %r\n", opi->name, &opi->addr);
        }
    }

  /* LSA limit print. */
  if (CHECK_FLAG (top->config, OSPF_CONFIG_OVERFLOW_LSDB_LIMIT))
    cli_out (cli, " overflow database %lu%s\n", top->lsdb_overflow_limit,
             top->lsdb_overflow_limit_type == OSPF_LSDB_OVERFLOW_SOFT_LIMIT
             ? " soft" : "");

#ifdef HAVE_OSPF_DB_OVERFLOW
  if ((top->ext_lsdb_limit != OSPF_DEFAULT_LSDB_LIMIT) ||
      (top->exit_overflow_interval != OSPF_DEFAULT_EXIT_OVERFLOW_INTERVAL))
    cli_out (cli, " overflow database external %u %d\n",
             top->ext_lsdb_limit, top->exit_overflow_interval);
#endif /* HAVE_OSPF_DB_OVERFLOW */

  /* Maximum area limit */
  if (top->max_area_limit != OSPF_AREA_LIMIT_DEFAULT)
    cli_out (cli, " maximum-area %lu\n", top->max_area_limit);

#ifdef HAVE_VRF_OSPF
  /* Domain ID config print */
  ospf_config_write_domain_info (top, cli);

#endif /* HAVE_VRF_OSPF */

  /* Host area print. */
  ospf_config_write_host_area (top, cli);

  /* Area config print. */
  ospf_config_write_area (top, cli);

  /* Network area print. */
  ospf_config_write_network_area (top->networks->top, cli);

  /* static neighbor print. */
  ospf_config_write_nbr_static (top, cli);

  /* Virtual-Link print. */
  ospf_config_write_vlink (top, cli);
#ifdef HAVE_OSPF_MULTI_AREA
  /* Multi area adj links print. */
  ospf_config_write_multi_area_adj_link (top, cli); 
#endif /* HAVE_OSPF_MULTI_AREA */

  /* Default metric configuration.  */
  ospf_config_write_default_metric (top, cli);

  /* Distribute-list and default-information print. */
  ospf_config_write_distribute (top, cli);

  /* Distance configuration. */
  ospf_config_write_distance (top, cli);

  /* Summary address configuration. */
  ospf_config_write_summary_address (top, cli);

  ospf_call_notifiers (top->om, OSPF_NOTIFY_WRITE_PROCESS_CONFIG, cli, top);

  cli_out (cli, "!\n");

  return write;
}

int
ospf_config_write (struct cli *cli)
{
  struct apn_vr *vr = cli->vr;
  struct ospf_master *om = vr->proto;
  struct listnode *node;
  int write = 0;
#ifdef HAVE_RESTART
  struct pal_in4_addr *router_id = NULL;
  struct listnode *nn = NULL;
#endif /* HAVE_RESTART */

#ifdef HAVE_OSPF_MULTI_INST
  if (CHECK_FLAG (om->flags, OSPF_EXT_MULTI_INST))
    cli_out (cli, "enable ext-ospf-multi-inst\n");
#endif /* HAVE_OSPF_MULTI_INST */

  for (node = LISTHEAD (om->ospf); node; NEXTNODE (node))
    write += ospf_config_write_instance (cli, node->data);

  cli_out (cli, "!\n");
 
#ifdef HAVE_RESTART
  if (CHECK_FLAG (om->config, OSPF_GLOBAL_CONFIG_RESTART_GRACE_PERIOD))
    cli_out (cli, "ospf restart grace-period %d\n", om->grace_period);

  if (CHECK_FLAG (om->config, OSPF_GLOBAL_CONFIG_RESTART_HELPER_POLICY)
      || CHECK_FLAG (om->config, OSPF_GLOBAL_CONFIG_RESTART_HELPER_GRACE_PERIOD))
    {
      if (CHECK_FLAG (om->config, 
                      OSPF_GLOBAL_CONFIG_RESTART_HELPER_GRACE_PERIOD))
        {
          cli_out (cli, "ospf restart helper");
          cli_out (cli, " max-grace-period %d\n", om->max_grace_period);
        }
      if (CHECK_FLAG (om->config, OSPF_GLOBAL_CONFIG_RESTART_HELPER_POLICY))
        {
          if (om->helper_policy == OSPF_RESTART_HELPER_NEVER)
            { 
              cli_out (cli, "ospf restart helper");
              cli_out (cli, " never\n");
            }
          else 
            {
              if (om->helper_policy == OSPF_RESTART_HELPER_ONLY_RELOAD)
                {
                  cli_out (cli, "ospf restart helper");
                  cli_out (cli, " only-reload\n");
                }
              else if (om->helper_policy == OSPF_RESTART_HELPER_ONLY_UPGRADE)
                {
                  cli_out (cli, "ospf restart helper");
                  cli_out (cli, " only-upgrade\n");
                }

              if (listcount (om->never_restart_helper_list) > 0)
                LIST_LOOP(om->never_restart_helper_list, router_id, nn)
                  {
                    cli_out (cli, "ospf restart helper");
                    cli_out (cli, " never router-id %r \n", router_id);
                  }
            }
        }
    } 
#endif /* HAVE_RESTART*/

  return write;
}

/* Interface configuration write function. */
int
ospf_config_write_interface (struct cli *cli)
{
  struct ospf_master *om = cli->vr->proto;
  struct ospf_if_params *oip;
  struct interface *ifp;
  struct listnode *n1, *n2;
  struct ls_table *rt;
  struct ls_node *rn;
  int write = 0;

#ifdef HAVE_CUSTOM1
  LIST_LOOP (cli->zg->ifg.if_list, ifp, n1)
#else /* HAVE_CUSTOM1 */
  LIST_LOOP (cli->vr->ifm.if_list, ifp, n1)
#endif /* HAVE_CUSTOM1 */
    {
      if (CHECK_FLAG (ifp->status, IF_HIDDEN_FLAG))
        continue;

      if (write)
        cli_out (cli, "!\n");

      cli_out (cli, "interface %s\n", ifp->name);
      if (ifp->desc)
        cli_out (cli, " description %s\n", ifp->desc);

      write++;

      rt = ospf_if_params_table_lookup_by_name (om, ifp->name);
      if (rt == NULL)
        continue;

      for (rn = ls_table_top (rt); rn; rn = ls_route_next (rn))
        if ((oip = RN_INFO (rn, RNI_DEFAULT)))
          {
            struct ospf_crypt_key *ck;
            char buf[25];

            if (rn->p->prefixlen != 0)
              zsnprintf (buf, 25, " ip ospf %r", rn->p->prefix);
            else
              zsnprintf (buf, 25, " ip ospf");

            /* Interface disable all print.  */
            if (OSPF_IF_PARAM_CHECK (oip, DISABLE_ALL))
              cli_out (cli, "%s disable all\n", buf);

            /* Interface Network print.  */
            if (OSPF_IF_PARAM_CHECK (oip, NETWORK_TYPE))
              cli_out (cli, "%s network %s\n", buf,
                       ospf_if_type_string (oip->type));

            /* OSPF interface authentication print. */
            if (OSPF_IF_PARAM_CHECK (oip, AUTH_TYPE))
              cli_out (cli, "%s authentication%s\n",
                       buf, ospf_auth_type_str[oip->auth_type]);

            /* Simple Authentication Password print. */
            if (OSPF_IF_PARAM_CHECK (oip, AUTH_SIMPLE))
              cli_out (cli, "%s authentication-key %s\n",
                       buf, oip->auth_simple);

            /* Cryptographic Authentication Key print. */
            if (OSPF_IF_PARAM_CHECK (oip, AUTH_CRYPT))
              LIST_LOOP (oip->auth_crypt, ck, n2)
                cli_out (cli, "%s message-digest-key %d md5 %s\n",
                         buf, ck->key_id, ck->auth_key);
        
            /* Interface Output Cost print. */
            if (OSPF_IF_PARAM_CHECK (oip, OUTPUT_COST))
              cli_out (cli, "%s cost %lu\n", buf, oip->output_cost);
#ifdef HAVE_OSPF_TE
            /* Interafce TE Metric print */
            if (OSPF_IF_PARAM_CHECK (oip, TE_METRIC))
              cli_out (cli, " te-metric %lu\n", oip->te_metric);
#endif /* HAVE_OSPF_TE */

            /* Hello Interval print. */
            if (OSPF_IF_PARAM_CHECK (oip, HELLO_INTERVAL))
              cli_out (cli, "%s hello-interval %lu\n",
                       buf, oip->hello_interval);

            /* Router Dead Interval print. */
            if (OSPF_IF_PARAM_CHECK (oip, DEAD_INTERVAL))
              cli_out (cli, "%s dead-interval %lu\n", buf, oip->dead_interval);

            /* Router Priority print. */
            if (OSPF_IF_PARAM_CHECK (oip, PRIORITY))
              cli_out (cli, "%s priority %u\n", buf, oip->priority);
        
#ifdef HAVE_RESTART
            /* Resync timeout. */
            if (OSPF_IF_PARAM_CHECK (oip, RESYNC_TIMEOUT))
              cli_out (cli, "%s resync-timeout %lu\n",
                       buf, oip->resync_timeout);
#endif /* HAVE_RESTART */

#ifdef HAVE_BFD
            /* BFD. */
            if (OSPF_IF_PARAM_CHECK (oip, BFD_DISABLE))
              cli_out (cli, "%s bfd disable\n", buf);
            else if (OSPF_IF_PARAM_CHECK (oip, BFD))
              cli_out (cli, "%s bfd\n", buf);
#endif /* HAVE_BFD */

            /* MTU print. */
            if (OSPF_IF_PARAM_CHECK (oip, MTU))
              cli_out (cli, "%s mtu %u\n", buf, oip->mtu);

            /* MTU ignore print. */
            if (OSPF_IF_PARAM_CHECK (oip, MTU_IGNORE))
              cli_out (cli, "%s mtu-ignore\n", buf);

            /* Retransmit Interval print. */
            if (OSPF_IF_PARAM_CHECK (oip, RETRANSMIT_INTERVAL))
              cli_out (cli, "%s retransmit-interval %lu\n",
                       buf, oip->retransmit_interval);

            /* Transmit Delay print. */
            if (OSPF_IF_PARAM_CHECK (oip, TRANSMIT_DELAY))
              cli_out (cli, "%s transmit-delay %u\n",
                       buf, oip->transmit_delay);

            /* Database Filter print. */
            if (OSPF_IF_PARAM_CHECK (oip, DATABASE_FILTER))
              cli_out (cli, "%s database-filter all out\n", buf);
          }

      ospf_call_notifiers (om, OSPF_NOTIFY_WRITE_LINK_CONFIG, cli, ifp);
    }

  return write;
}
#ifdef HAVE_GMPLS
void
ospf_te_link_hash_interator_write (struct hash_backet *hb, struct cli *cli)
{
  struct ospf_telink *os_tel = NULL;
  struct telink *tl = NULL;
  char area_str[OSPF_AREA_STRING_MAXLEN];
  
  tl = (struct telink *) hb->data;
  if (! tl)
    return;
    
  if (tl->info == NULL)
    return;

  os_tel = (struct ospf_telink *)tl->info;
  if (! os_tel)
    return;

  cli_out (cli, "te-link %s local-link-id %r", tl->name,
             &tl->l_linkid.linkid.ipv4);

  if (CHECK_FLAG (tl->flags, GMPLS_TE_LINK_NUMBERED))
    cli_out (cli, " numbered\n");
  else
    cli_out (cli, "\n");

  if (os_tel->params)
    {
      if (CHECK_FLAG (os_tel->params->config, OSPF_TLINK_PARAM_AREA_ID))
        {
          ospf_area_fmt_string (os_tel->params->area_id, 
                                os_tel->params->format, area_str);
          cli_out (cli, " te-flooding ospf %d area %s\n",
                   os_tel->params->proc_id, area_str);
        }

      if (CHECK_FLAG (os_tel->params->config, OSPF_TLINK_PARAM_TE_METRIC))
        cli_out (cli, " te-metric %lu\n", os_tel->params->te_metric);

      if (CHECK_FLAG (os_tel->params->config, OSPF_TLINK_PARAM_TE_LINK_LOCAL))
        cli_out (cli, " enable-te-link-local\n");
    }
}

int
ospf_config_write_telink (struct cli *cli)
{
  struct ospf_master *om = cli->vr->proto;

  struct gmpls_if *gmif;

  if ((! om) || (! om->zg) || (! om->vr))
      return CLI_SUCCESS;

  /* Get the GMPLS interface structure */
  gmif = gmpls_if_get (om->zg, om->vr->id);
  if (gmif == NULL)
    return CLI_SUCCESS;

  /* Go through CA tree */
  hash_iterate (gmif->telhash,
                (void (*) (struct hash_backet*, void*))
                ospf_te_link_hash_interator_write, cli);

  return CLI_SUCCESS;
}
#endif /* HAVE_GMPLS */

static char *type_str[] = {"hello", "dd", "ls-request", "ls-update", "ls-ack"};

int
ospf_config_write_debug_packet (struct cli *cli, int flags, char *str)
{
  struct apn_vr *vr = cli->vr;
  struct ospf_master *om = vr->proto;
  int types = 0;
  int i;

  for (i = 0; i < 5; i++)
    if (om->debug.conf.packet[i] == flags)
      types |= (1 << i);

  if (types != 0)
    {
      cli_out (cli, "debug ospf packet");

      if (types != OSPF_DEBUG_PACKET_ALL)
        for (i = 0; i < 5; i++)
          if (types & (1 << i))
            cli_out (cli, " %s", type_str[i]);

      cli_out (cli, " %s\n", str);
    }

  return 1;
}

int
ospf_config_write_debug (struct cli *cli)
{
  struct apn_vr *vr = cli->vr;
  struct ospf_master *om = vr->proto;
  int write = 0;

#ifdef HAVE_OSPF_CSPF
  if (IS_DEBUG_CSPF (&om->debug_cspf, EVENT))
    cli_out (cli, "debug cspf events\n");
  if (IS_DEBUG_CSPF (&om->debug_cspf, HEXDUMP))
    cli_out (cli, "debug cspf hexdump\n");
#endif /* HAVE_OSPF_CSPF */

  /* debug ospf ifsm (status|events|timers). */
  if (IS_CONF_DEBUG_OSPF (ifsm, IFSM))
    {
      cli_out (cli, "debug ospf ifsm");
      if (IS_CONF_DEBUG_OSPF (ifsm, IFSM) != OSPF_DEBUG_IFSM)
        {
          if (IS_CONF_DEBUG_OSPF (ifsm, IFSM_STATUS))
            cli_out (cli, " status");
          if (IS_CONF_DEBUG_OSPF (ifsm, IFSM_EVENTS))
            cli_out (cli, " events");
          if (IS_CONF_DEBUG_OSPF (ifsm, IFSM_TIMERS))
            cli_out (cli, " timers");
        }
      cli_out (cli, "\n");
      write++;
    }

  /* debug ospf nfsm (status|events|timers). */
  if (IS_CONF_DEBUG_OSPF (nfsm, NFSM))
    {
      cli_out (cli, "debug ospf nfsm");
      if (IS_CONF_DEBUG_OSPF (nfsm, NFSM) != OSPF_DEBUG_NFSM)
        {
          if (IS_CONF_DEBUG_OSPF (nfsm, NFSM_STATUS))
            cli_out (cli, " status");
          if (IS_CONF_DEBUG_OSPF (nfsm, NFSM_EVENTS))
            cli_out (cli, " events");
          if (IS_CONF_DEBUG_OSPF (nfsm, NFSM_TIMERS))
            cli_out (cli, " timers");
        }
      cli_out (cli, "\n");
      write++;
    }

  /* debug ospf lsa (generate|flooding|refresh). */
  if (IS_CONF_DEBUG_OSPF (lsa, LSA))
    {
      cli_out (cli, "debug ospf lsa");
      if (IS_CONF_DEBUG_OSPF (lsa, LSA) != OSPF_DEBUG_LSA)
        {
          if (IS_CONF_DEBUG_OSPF (lsa, LSA_GENERATE))
            cli_out (cli, " generate");
          if (IS_CONF_DEBUG_OSPF (lsa, LSA_FLOODING))
            cli_out (cli, " flooding");
          if (IS_CONF_DEBUG_OSPF (lsa, LSA_REFRESH))
            cli_out (cli, " refresh");
          if (IS_CONF_DEBUG_OSPF (lsa, LSA_INSTALL))
            cli_out (cli, " install");
          if (IS_CONF_DEBUG_OSPF (lsa, LSA_MAXAGE))
            cli_out (cli, " maxage");
        }
      cli_out (cli, "\n");
      write++;
    }

  /* debug ospf nsm (interface|redistribute). */
  if (IS_CONF_DEBUG_OSPF (nsm, NSM))
    {
      cli_out (cli, "debug ospf nsm");
      if (IS_CONF_DEBUG_OSPF (nsm, NSM) != OSPF_DEBUG_NSM)
        {
          if (IS_CONF_DEBUG_OSPF (nsm, NSM_INTERFACE))
            cli_out (cli, " interface");
          if (IS_CONF_DEBUG_OSPF (nsm, NSM_REDISTRIBUTE))
            cli_out (cli, " redistribute");
        }
      cli_out (cli,"\n");
      write++;
    }

#ifdef HAVE_BFD
  /* debug ospf bfd.  */
  if (IS_CONF_DEBUG_OSPF (bfd, BFD))
    {
      cli_out (cli, "debug ospf bfd\n");
      write++;
    }
#endif /* HAVE_BFD */

  /* debug ospf events (abr|asbr|nssa|vl|lsa|os|router) */
  if (IS_CONF_DEBUG_OSPF (event, EVENT))
    {
      cli_out (cli, "debug ospf events");
      if (IS_CONF_DEBUG_OSPF (event, EVENT) != OSPF_DEBUG_EVENT)
        {
          if (IS_CONF_DEBUG_OSPF (event, EVENT_ABR))
            cli_out (cli, " abr");
          if (IS_CONF_DEBUG_OSPF (event, EVENT_ASBR))
            cli_out (cli, " asbr");
          if (IS_CONF_DEBUG_OSPF (event, EVENT_NSSA))
            cli_out (cli, " nssa");
          if (IS_CONF_DEBUG_OSPF (event, EVENT_VLINK))
            cli_out (cli, " vlink");
          if (IS_CONF_DEBUG_OSPF (event, EVENT_LSA))
            cli_out (cli, " lsa");
          if (IS_CONF_DEBUG_OSPF (event, EVENT_OS))
            cli_out (cli, " os");
          if (IS_CONF_DEBUG_OSPF (event, EVENT_ROUTER))
            cli_out (cli, " router");
        }
      cli_out (cli, "\n");
      write++;
    }

  /* debug ospf route (spf|ia|ase|install) */
  if (IS_CONF_DEBUG_OSPF (rt_calc, RT_ALL))
    {
      cli_out (cli, "debug ospf route");
      if (IS_CONF_DEBUG_OSPF (rt_calc, RT_ALL) != OSPF_DEBUG_RT_ALL)
        {
          if (IS_CONF_DEBUG_OSPF (rt_calc, RT_CALC_SPF))
            cli_out (cli, " spf");
          if (IS_CONF_DEBUG_OSPF (rt_calc, RT_CALC_IA))
            cli_out (cli, " ia");
          if (IS_CONF_DEBUG_OSPF (rt_calc, RT_CALC_ASE))
            cli_out (cli, " ase");
          if (IS_CONF_DEBUG_OSPF (rt_calc, RT_INSTALL))
            cli_out (cli, " install");
        }      
      cli_out (cli, "\n");
      write++;
    }

  /* debug ospf packet. */
  ospf_config_write_debug_packet (cli, OSPF_DEBUG_SEND_RECV, "");
  ospf_config_write_debug_packet (cli, OSPF_DEBUG_SEND_RECV|OSPF_DEBUG_DETAIL,
                                  "detail");
  ospf_config_write_debug_packet (cli, OSPF_DEBUG_RECV, "recv");
  ospf_config_write_debug_packet (cli, OSPF_DEBUG_RECV|OSPF_DEBUG_DETAIL,
                                  "recv detail");
  ospf_config_write_debug_packet (cli, OSPF_DEBUG_SEND, "send");
  ospf_config_write_debug_packet (cli, OSPF_DEBUG_SEND|OSPF_DEBUG_DETAIL,
                                  "send detail");

  return write;
}

void
ospf_cli_router_init (struct cli_tree *ctree)
{
  /* Install ospf top node. */
  cli_install_config (ctree, OSPF_MODE, ospf_config_write);
  cli_install_default (ctree, OSPF_MODE);

  /* "router ospf" commands. */
  cli_install_gen (ctree, CONFIG_MODE, PRIVILEGE_NORMAL, 0,
                   &router_ospf_cmd);
  cli_install_gen (ctree, CONFIG_MODE, PRIVILEGE_NORMAL, 0,
                   &router_ospf_id_cmd);
  cli_install_gen (ctree, CONFIG_MODE, PRIVILEGE_NORMAL, 0,
                   &no_router_ospf_cmd);
  cli_install_gen (ctree, CONFIG_MODE, PRIVILEGE_NORMAL, 0,
                   &no_router_ospf_id_cmd);
}

/* CLI init functions. */
void
ospf_cli_ospf_init (struct cli_tree *ctree)
{
  /* "network area" commands. */
  cli_install_gen (ctree, OSPF_MODE, PRIVILEGE_NORMAL, 0,
                   &network_area_cmd);
  cli_install_gen (ctree, OSPF_MODE, PRIVILEGE_NORMAL, 0,
                   &network_mask_area_cmd);
  cli_install_gen (ctree, OSPF_MODE, PRIVILEGE_NORMAL, 0,
                   &no_network_area_cmd);
  cli_install_gen (ctree, OSPF_MODE, PRIVILEGE_NORMAL, 0,
                   &no_network_mask_area_cmd);

  /* "ospf router-id" commands. */
  cli_install_gen (ctree, OSPF_MODE, PRIVILEGE_NORMAL, 0,
                   &ospf_router_id_cmd);
  cli_install_gen (ctree, OSPF_MODE, PRIVILEGE_NORMAL, 0,
                   &no_ospf_router_id_cmd);
  cli_install_gen (ctree, OSPF_MODE, PRIVILEGE_NORMAL, 0,
                   &router_id_cmd);
  cli_install_gen (ctree, OSPF_MODE, PRIVILEGE_NORMAL, 0,
                   &no_router_id_cmd);

  /* "passive-interface" commands. */
  cli_install_gen (ctree, OSPF_MODE, PRIVILEGE_NORMAL, 0,
                   &passive_interface_addr_cmd);
  cli_install_gen (ctree, OSPF_MODE, PRIVILEGE_NORMAL, 0,
                   &passive_interface_cmd);
  cli_install_gen (ctree, OSPF_MODE, PRIVILEGE_NORMAL, 0,
                   &no_passive_interface_addr_cmd);
  cli_install_gen (ctree, OSPF_MODE, PRIVILEGE_NORMAL, 0,
                   &no_passive_interface_cmd);

  /* "host area" commands. */
  cli_install_gen (ctree, OSPF_MODE, PRIVILEGE_NORMAL, 0,
                   &ospf_host_area_cmd);
  cli_install_gen (ctree, OSPF_MODE, PRIVILEGE_NORMAL, 0,
                   &ospf_host_area_cost_cmd);
  cli_install_gen (ctree, OSPF_MODE, PRIVILEGE_NORMAL, 0,
                   &no_ospf_host_area_cmd);
  cli_install_gen (ctree, OSPF_MODE, PRIVILEGE_NORMAL, 0,
                   &no_ospf_host_area_cost_cmd);

  /* "ospf abr-type" commands. */
  cli_install_gen (ctree, OSPF_MODE, PRIVILEGE_NORMAL, 0,
                   &ospf_abr_type_cmd);
  cli_install_gen (ctree, OSPF_MODE, PRIVILEGE_NORMAL, 0,
                   &no_ospf_abr_type_cmd);

  /* "compatible rfc1583" commands. */
  cli_install_gen (ctree, OSPF_MODE, PRIVILEGE_NORMAL, 0,
                   &ospf_compatible_rfc1583_cmd);
  cli_install_gen (ctree, OSPF_MODE, PRIVILEGE_NORMAL, 0,
                   &no_ospf_compatible_rfc1583_cmd);
  cli_install_gen (ctree, OSPF_MODE, PRIVILEGE_NORMAL, CLI_FLAG_HIDDEN,
                   &ospf_rfc1583_compatibility_cmd);
  cli_install_gen (ctree, OSPF_MODE, PRIVILEGE_NORMAL, CLI_FLAG_HIDDEN,
                   &no_ospf_rfc1583_compatibility_cmd);

  /* "summary-address" commands. */
  cli_install_gen (ctree, OSPF_MODE, PRIVILEGE_NORMAL, 0,
                   &summary_address_cmd);
  cli_install_gen (ctree, OSPF_MODE, PRIVILEGE_NORMAL, 0,
                   &no_summary_address_cmd);
  cli_install_gen (ctree, OSPF_MODE, PRIVILEGE_NORMAL, 0,
                   &no_summary_address_options_cmd);

  /* "timers spf" commands. */
  cli_install_gen (ctree, OSPF_MODE, PRIVILEGE_NORMAL, 0,
                   &timers_spf_cmd);
  cli_install_gen (ctree, OSPF_MODE, PRIVILEGE_NORMAL, 0,
                   &timers_spf_exp_cmd);
  cli_install_gen (ctree, OSPF_MODE, PRIVILEGE_NORMAL, 0,
                   &no_timers_spf_cmd);
  cli_install_gen (ctree, OSPF_MODE, PRIVILEGE_NORMAL, 0,
                   &no_timers_spf_exp_cmd);

  /* "refresh timer" commands. */
  cli_install_gen (ctree, OSPF_MODE, PRIVILEGE_NORMAL, CLI_FLAG_HIDDEN,
                   &refresh_timer_cmd);
  cli_install_gen (ctree, OSPF_MODE, PRIVILEGE_NORMAL, CLI_FLAG_HIDDEN,
                   &no_refresh_timer_cmd);
  
  /* "auto-cost reference-bandwidth" commands. */
  cli_install_gen (ctree, OSPF_MODE, PRIVILEGE_NORMAL, 0,
                   &auto_cost_reference_bandwidth_cmd);
  cli_install_gen (ctree, OSPF_MODE, PRIVILEGE_NORMAL, 0,
                   &no_auto_cost_reference_bandwidth_cmd);

  /* "max-concurrent-dd" command. */
  cli_install_gen (ctree, OSPF_MODE, PRIVILEGE_NORMAL, 0,
                   &ospf_max_concurrent_dd_cmd);
  cli_install_gen (ctree, OSPF_MODE, PRIVILEGE_NORMAL, 0,
                   &no_ospf_max_concurrent_dd_cmd);

  /* "max-unuse-packet" command.  */
  cli_install_gen (ctree, OSPF_MODE, PRIVILEGE_NORMAL, CLI_FLAG_HIDDEN,
                   &ospf_max_unuse_packet_cmd);
  cli_install_gen (ctree, OSPF_MODE, PRIVILEGE_NORMAL, CLI_FLAG_HIDDEN,
                   &no_ospf_max_unuse_packet_cmd);

  /* "max-unuse-lsa" command.  */
  cli_install_gen (ctree, OSPF_MODE, PRIVILEGE_NORMAL, CLI_FLAG_HIDDEN,
                   &ospf_max_unuse_lsa_cmd);
  cli_install_gen (ctree, OSPF_MODE, PRIVILEGE_NORMAL, CLI_FLAG_HIDDEN,
                   &no_ospf_max_unuse_lsa_cmd);

  /* "neighbor" commands. */
  cli_install_gen (ctree, OSPF_MODE, PRIVILEGE_NORMAL, 0,
                   &ospf_neighbor_cmd);
  cli_install_gen (ctree, OSPF_MODE, PRIVILEGE_NORMAL, 0,
                   &ospf_neighbor_priority_poll_interval_cmd);
  cli_install_gen (ctree, OSPF_MODE, PRIVILEGE_NORMAL, 0,
                   &ospf_neighbor_cost_cmd);
  cli_install_gen (ctree, OSPF_MODE, PRIVILEGE_NORMAL, 0,
                   &no_ospf_neighbor_cmd);
  cli_install_gen (ctree, OSPF_MODE, PRIVILEGE_NORMAL, 0,
                   &no_ospf_neighbor_priority_poll_interval_cmd);
  cli_install_gen (ctree, OSPF_MODE, PRIVILEGE_NORMAL, 0,
                   &no_ospf_neighbor_cost_cmd);

  /* lsdb_limit" commands. */
  cli_install_gen (ctree, OSPF_MODE, PRIVILEGE_NORMAL, 0,
                   &ospf_overflow_lsdb_limit_cmd);
  cli_install_gen (ctree, OSPF_MODE, PRIVILEGE_NORMAL, 0,
                   &no_ospf_overflow_lsdb_limit_cmd);

#ifdef HAVE_OSPF_DB_OVERFLOW
  /* "overflow database external" commands. */
  cli_install_gen (ctree, OSPF_MODE, PRIVILEGE_NORMAL, 0,
                   &overflow_database_external_cmd);
  cli_install_gen (ctree, OSPF_MODE, PRIVILEGE_NORMAL, 0,
                   &no_overflow_database_external_cmd);
#endif /* HAVE_OSPF_DB_OVERFLOW */

  /* "maximum-area" command */
  cli_install_gen (ctree, OSPF_MODE, PRIVILEGE_NORMAL, 0,
                   &ospf_max_area_limit_cmd);
  cli_install_gen (ctree, OSPF_MODE, PRIVILEGE_NORMAL, 0,
                   &no_ospf_max_area_limit_cmd);

#ifdef HAVE_OPAQUE_LSA
  /* "capability opaque" commands. */
  cli_install_gen (ctree, OSPF_MODE, PRIVILEGE_NORMAL, 0,
                   &ospf_capability_opaque_cmd);
  cli_install_gen (ctree, OSPF_MODE, PRIVILEGE_NORMAL, 0,
                   &no_ospf_capability_opaque_cmd);
  cli_install_gen (ctree, OSPF_MODE, PRIVILEGE_NORMAL, CLI_FLAG_HIDDEN,
                   &ospf_opaque_lsa_capable_cmd);
  cli_install_gen (ctree, OSPF_MODE, PRIVILEGE_NORMAL, CLI_FLAG_HIDDEN,
                   &no_ospf_opaque_lsa_capable_cmd);
#ifdef HAVE_DEV_TEST
  cli_install_gen (ctree, OSPF_MODE, PRIVILEGE_NORMAL, 0,
                   &opaque_lsa_gen_cmd);
#endif /* HAVE_DEV_TEST */
#endif /* HAVE_OPAQUE_LSA */

#ifdef HAVE_OSPF_TE
  cli_install_gen (ctree, OSPF_MODE, PRIVILEGE_NORMAL, 0,
                   &ospf_capability_te_cmd);
  cli_install_gen (ctree, OSPF_MODE, PRIVILEGE_NORMAL, 0,
                   &no_ospf_capability_te_cmd);
  cli_install_gen (ctree, OSPF_MODE, PRIVILEGE_NORMAL, CLI_FLAG_HIDDEN,
                   &ospf_enable_te_cmd);
  cli_install_gen (ctree, OSPF_MODE, PRIVILEGE_NORMAL, CLI_FLAG_HIDDEN,
                   &no_ospf_enable_te_cmd);

#endif /* HAVE_OSPF_TE */

#ifdef HAVE_OSPF_CSPF
  cli_install_gen (ctree, OSPF_MODE, PRIVILEGE_NORMAL, 0,
                   &ospf_capability_cspf_cmd);
#ifdef HAVE_GMPLS
  cli_install_gen (ctree, OSPF_MODE, PRIVILEGE_NORMAL, 0,
                   &cspf_enable_better_protection_cmd);
  cli_install_gen (ctree, OSPF_MODE, PRIVILEGE_NORMAL, 0,
                   &cspf_disable_better_protection_cmd);
#endif /* HAVE_GMPLS */
  cli_install_gen (ctree, OSPF_MODE, PRIVILEGE_NORMAL, 0,
                   &cspf_tie_break_cmd);
  cli_install_gen (ctree, OSPF_MODE, PRIVILEGE_NORMAL, 0,
                   &cspf_default_retry_interval_cmd);
  cli_install_gen (ctree, OSPF_MODE, PRIVILEGE_NORMAL, 0,
                   &no_ospf_capability_cspf_cmd);
  cli_install_gen (ctree, OSPF_MODE, PRIVILEGE_NORMAL, 0,
                   &no_cspf_tie_break_cmd);
  cli_install_gen (ctree, OSPF_MODE, PRIVILEGE_NORMAL, 0,
                   &no_cspf_default_retry_interval_cmd);

  cli_install_gen (ctree, OSPF_MODE, PRIVILEGE_NORMAL, CLI_FLAG_HIDDEN,
                   &ospf_enable_cspf_cmd);
  cli_install_gen (ctree, OSPF_MODE, PRIVILEGE_NORMAL, CLI_FLAG_HIDDEN,
                   &cspf_tie_break_old_cmd);
  cli_install_gen (ctree, OSPF_MODE, PRIVILEGE_NORMAL, CLI_FLAG_HIDDEN,
                   &cspf_default_retry_interval_old_cmd);
  cli_install_gen (ctree, OSPF_MODE, PRIVILEGE_NORMAL, CLI_FLAG_HIDDEN,
                   &no_ospf_enable_cspf_cmd);
  cli_install_gen (ctree, OSPF_MODE, PRIVILEGE_NORMAL, CLI_FLAG_HIDDEN,
                   &no_cspf_tie_break_old_cmd);
  cli_install_gen (ctree, OSPF_MODE, PRIVILEGE_NORMAL, CLI_FLAG_HIDDEN,
                   &no_cspf_default_retry_interval_old_cmd);
#endif /* HAVE_OSPF_CSPF */

#ifdef HAVE_OSPF_MULTI_INST
  cli_install_gen (ctree, CONFIG_MODE, PRIVILEGE_NORMAL, 0,
                   &enable_ext_ospf_multi_inst_cmd);
  cli_install_gen (ctree, CONFIG_MODE, PRIVILEGE_NORMAL, 0,
                   &no_enable_ext_ospf_multi_inst_cmd);
#endif /* HAVE_OSPF_MULTI_INST */

#ifdef HAVE_RESTART
  /* For Graceful Restart. */
  cli_install_gen (ctree, CONFIG_MODE, PRIVILEGE_NORMAL, 0,
                   &ospf_restart_grace_period_cmd);
  cli_install_gen (ctree, CONFIG_MODE, PRIVILEGE_NORMAL, 0,
                   &ospf_restart_helper_never_cmd);
  cli_install_gen (ctree, CONFIG_MODE, PRIVILEGE_NORMAL, 0,
                   &ospf_restart_helper_policy_cmd);
  cli_install_gen (ctree, CONFIG_MODE, PRIVILEGE_NORMAL, 0,
                   &no_ospf_restart_grace_period_cmd);
  cli_install_gen (ctree, CONFIG_MODE, PRIVILEGE_NORMAL, 0,
                   &no_ospf_restart_helper_policy_cmd);
  cli_install_gen (ctree, OSPF_MODE, PRIVILEGE_NORMAL, 0,
                   &ospf_capability_restart_cmd);
  cli_install_gen (ctree, OSPF_MODE, PRIVILEGE_NORMAL, 0,
                   &no_ospf_capability_restart_cmd);
  cli_install_gen (ctree, INTERFACE_MODE, PRIVILEGE_NORMAL, 0,
                   &ip_ospf_resync_timeout_cmd);
  cli_install_gen (ctree, INTERFACE_MODE, PRIVILEGE_NORMAL, 0,
                   &no_ip_ospf_resync_timeout_cmd);
  cli_install_gen (ctree, EXEC_MODE, PRIVILEGE_MAX, 0,
                   &restart_ospf_graceful_cmd);

  cli_install_gen (ctree, CONFIG_MODE, PRIVILEGE_NORMAL, CLI_FLAG_HIDDEN,
                   &ospf_hitless_restart_grace_period_cmd);
  cli_install_gen (ctree, CONFIG_MODE, PRIVILEGE_NORMAL, CLI_FLAG_HIDDEN,
                   &no_ospf_hitless_restart_grace_period_cmd);
  cli_install_gen (ctree, CONFIG_MODE, PRIVILEGE_NORMAL, CLI_FLAG_HIDDEN,
                   &ospf_hitless_restart_helper_never_cmd);
  cli_install_gen (ctree, CONFIG_MODE, PRIVILEGE_NORMAL, CLI_FLAG_HIDDEN,
                   &ospf_hitless_restart_helper_policy_cmd);
  cli_install_gen (ctree, CONFIG_MODE, PRIVILEGE_NORMAL, CLI_FLAG_HIDDEN,
                   &no_ospf_hitless_restart_helper_policy_cmd);
  cli_install_gen (ctree, EXEC_MODE, PRIVILEGE_MAX, CLI_FLAG_HIDDEN,
                   &restart_ospf_hitless_cmd);

#endif /* HAVE_RESTART */
  cli_install_gen (ctree, OSPF_MODE, PRIVILEGE_NORMAL, 0,
                   &ospf_enable_db_summary_opt_cmd);
  cli_install_gen (ctree, OSPF_MODE, PRIVILEGE_NORMAL, 0,
                   &no_ospf_enable_db_summary_opt_cmd);
}

void
ospf_cli_if_init (struct cli_tree *ctree)
{
  /* Install interface node. */
  cli_install_config (ctree, INTERFACE_MODE, ospf_config_write_interface);
  cli_install_gen (ctree, CONFIG_MODE, PRIVILEGE_NORMAL, 0,
                   &interface_ospf_cmd);
#ifdef HAVE_TUNNEL
  cli_install_gen (ctree, CONFIG_MODE, PRIVILEGE_NORMAL, 0,
                   &interface_tunnel_ospf_cmd);
#endif /* HAVE_TUNNEL */

  cli_install_default (ctree, INTERFACE_MODE);

  /* "description" commands. */
  cli_install_gen (ctree, INTERFACE_MODE, PRIVILEGE_NORMAL, 0,
                   &interface_desc_cli);
  cli_install_gen (ctree, INTERFACE_MODE, PRIVILEGE_NORMAL, 0,
                   &no_interface_desc_cli);

  /* "ip ospf network" commands. */
  cli_install_gen (ctree, INTERFACE_MODE, PRIVILEGE_NORMAL, 0,
                   &ip_ospf_network_cmd);
  cli_install_gen (ctree, INTERFACE_MODE, PRIVILEGE_NORMAL, 0,
                   &ip_ospf_network_p2mp_nbma_cmd);
  cli_install_gen (ctree, INTERFACE_MODE, PRIVILEGE_NORMAL, 0,
                   &no_ip_ospf_network_cmd);

  /* "ip ospf authentication" commands. */
  cli_install_gen (ctree, INTERFACE_MODE, PRIVILEGE_NORMAL, 0,
                   &ip_ospf_authentication_cmd);
  cli_install_gen (ctree, INTERFACE_MODE, PRIVILEGE_NORMAL, 0,
                   &ip_ospf_authentication_addr_cmd);
  cli_install_gen (ctree, INTERFACE_MODE, PRIVILEGE_NORMAL, 0,
                   &no_ip_ospf_authentication_cmd);

  /* "ip ospf authentication-key" commands. */
  cli_install_gen (ctree, INTERFACE_MODE, PRIVILEGE_NORMAL, 0,
                   &ip_ospf_authentication_key_cmd);
  cli_install_gen (ctree, INTERFACE_MODE, PRIVILEGE_NORMAL, 0,
                   &no_ip_ospf_authentication_key_cmd);

  /* "ip ospf message-digest-key" commands. */
  cli_install_gen (ctree, INTERFACE_MODE, PRIVILEGE_NORMAL, 0,
                   &ip_ospf_message_digest_key_cmd);
  cli_install_gen (ctree, INTERFACE_MODE, PRIVILEGE_NORMAL, 0,
                   &no_ip_ospf_message_digest_key_cmd);

  /* "ip ospf priority" commands. */
  cli_install_gen (ctree, INTERFACE_MODE, PRIVILEGE_NORMAL, 0,
                   &ip_ospf_priority_cmd);
  cli_install_gen (ctree, INTERFACE_MODE, PRIVILEGE_NORMAL, 0,
                   &no_ip_ospf_priority_cmd);

  /* "ip ospf cost" commands. */
  cli_install_gen (ctree, INTERFACE_MODE, PRIVILEGE_NORMAL, 0,
                   &ip_ospf_cost_cmd);
  cli_install_gen (ctree, INTERFACE_MODE, PRIVILEGE_NORMAL, 0,
                   &no_ip_ospf_cost_cmd);

  /* "ip ospf dead-interval" commands. */
  cli_install_gen (ctree, INTERFACE_MODE, PRIVILEGE_NORMAL, 0,
                   &ip_ospf_dead_interval_cmd);
  cli_install_gen (ctree, INTERFACE_MODE, PRIVILEGE_NORMAL, 0,
                   &no_ip_ospf_dead_interval_cmd);

  /* "ip ospf hello-interval" commands. */
  cli_install_gen (ctree, INTERFACE_MODE, PRIVILEGE_NORMAL, 0,
                   &ip_ospf_hello_interval_cmd);
  cli_install_gen (ctree, INTERFACE_MODE, PRIVILEGE_NORMAL, 0,
                   &no_ip_ospf_hello_interval_cmd);

  /* "ip ospf retransmit-interval" commands. */
  cli_install_gen (ctree, INTERFACE_MODE, PRIVILEGE_NORMAL, 0,
                   &ip_ospf_retransmit_interval_cmd);
  cli_install_gen (ctree, INTERFACE_MODE, PRIVILEGE_NORMAL, 0,
                   &no_ip_ospf_retransmit_interval_cmd);

  /* "ip ospf transmit-delay" commands. */
  cli_install_gen (ctree, INTERFACE_MODE, PRIVILEGE_NORMAL, 0,
                   &ip_ospf_transmit_delay_cmd);
  cli_install_gen (ctree, INTERFACE_MODE, PRIVILEGE_NORMAL, 0,
                   &no_ip_ospf_transmit_delay_cmd);

  /* "ip ospf database-filter all out" commands. */
  cli_install_gen (ctree, INTERFACE_MODE, PRIVILEGE_NORMAL, 0,
                   &ip_ospf_database_filter_all_out_cmd);
  cli_install_gen (ctree, INTERFACE_MODE, PRIVILEGE_NORMAL, 0,
                   &no_ip_ospf_database_filter_cmd);

  /* "ip ospf mtu-ignore" commands. */
  cli_install_gen (ctree, INTERFACE_MODE, PRIVILEGE_NORMAL, 0,
                   &ip_ospf_mtu_ignore_cmd);
  cli_install_gen (ctree, INTERFACE_MODE, PRIVILEGE_NORMAL, 0,
                   &no_ip_ospf_mtu_ignore_cmd);

  /* "ip ospf mtu" commands. */
  cli_install_gen (ctree, INTERFACE_MODE, PRIVILEGE_NORMAL, 0,
                   &ip_ospf_mtu_cmd);
  cli_install_gen (ctree, INTERFACE_MODE, PRIVILEGE_NORMAL, 0,
                   &no_ip_ospf_mtu_cmd);

  /* "ip ospf disable all" commands. */
  cli_install_gen (ctree, INTERFACE_MODE, PRIVILEGE_NORMAL, 0,
                   &ip_ospf_disable_all_cmd);
  cli_install_gen (ctree, INTERFACE_MODE, PRIVILEGE_NORMAL, 0,
                   &no_ip_ospf_disable_all_cmd);

#ifdef HAVE_OSPF_TE 
  /* "te-metric" commands. */
  cli_install_gen (ctree, INTERFACE_MODE, PRIVILEGE_NORMAL, 0,
                   &te_metric_cmd);
  cli_install_gen (ctree, INTERFACE_MODE, PRIVILEGE_NORMAL, 0,
                   &no_te_metric_cmd);
#endif /* HAVE_OSPF_TE */
}

#if defined HAVE_GMPLS && defined HAVE_OSPF_TE
void
ospf_cli_telink_init (struct cli_tree *ctree)
{
  cli_install_config (ctree, TELINK_MODE, ospf_config_write_telink);
  cli_install_gen (ctree, CONFIG_MODE, PRIVILEGE_NORMAL, 0,
                   &te_link_ospf_cmd);
  cli_install_gen (ctree, CONFIG_MODE, PRIVILEGE_NORMAL, 0,
                   &te_link_ospf_detail_cmd);
  cli_install_default (ctree, TELINK_MODE);
  cli_install_gen (ctree, TELINK_MODE, PRIVILEGE_NORMAL, 0,
                   &ospf_te_flooding_cmd);
  cli_install_gen (ctree, TELINK_MODE, PRIVILEGE_NORMAL, 0,
                   &no_ospf_te_flooding_cmd);
  cli_install_gen (ctree, TELINK_MODE, PRIVILEGE_NORMAL, 0,
                   &te_metric_cmd);
  cli_install_gen (ctree, TELINK_MODE, PRIVILEGE_NORMAL, 0,
                   &no_te_metric_cmd);
  cli_install_gen (ctree, TELINK_MODE, PRIVILEGE_NORMAL, 0,
                   &ospf_enable_te_link_local_cmd);
  cli_install_gen (ctree, TELINK_MODE, PRIVILEGE_NORMAL, 0,
                   &ospf_disable_te_link_local_cmd);
}
#endif /* HAVE_GMPLS && HAVE_OSPF_TE */

void
ospf_cli_vlink_init (struct cli_tree *ctree)
{
  /* "area virtual-link" commands. */
  cli_install_gen (ctree, OSPF_MODE, PRIVILEGE_NORMAL, 0,
                   &area_virtual_link_cmd);
  cli_install_gen (ctree, OSPF_MODE, PRIVILEGE_NORMAL, 0,
                   &area_virtual_link_options_cmd);
  cli_install_gen (ctree, OSPF_MODE, PRIVILEGE_NORMAL, 0,
                   &no_area_virtual_link_cmd);
  cli_install_gen (ctree, OSPF_MODE, PRIVILEGE_NORMAL, 0,
                   &no_area_virtual_link_options_cmd);
}
#ifdef HAVE_OSPF_MULTI_AREA
void
ospf_cli_multi_area_link_init (struct cli_tree *ctree)
{
  cli_install_gen (ctree, OSPF_MODE, PRIVILEGE_NORMAL, 0,
                   &area_multi_area_adjacency_cmd);
  cli_install_gen (ctree, OSPF_MODE, PRIVILEGE_NORMAL, 0,
                   &no_area_multi_area_adjacency_cmd);
}
#endif /* HAVE_OSPF_MULTI_AREA */
void
ospf_cli_abr_init (struct cli_tree *ctree)
{
  /* "area authentication" commands. */
  cli_install_gen (ctree, OSPF_MODE, PRIVILEGE_NORMAL, 0,
                   &area_authentication_cmd);
#ifdef HAVE_MD5
  cli_install_gen (ctree, OSPF_MODE, PRIVILEGE_NORMAL, 0,
                   &area_authentication_message_digest_cmd);
#endif /* HAVE_MD5 */
  cli_install_gen (ctree, OSPF_MODE, PRIVILEGE_NORMAL, 0,
                   &no_area_authentication_cmd);

  /* "area stub" commands. */
  cli_install_gen (ctree, OSPF_MODE, PRIVILEGE_NORMAL, 0,
                   &area_stub_no_summary_cmd);
  cli_install_gen (ctree, OSPF_MODE, PRIVILEGE_NORMAL, 0,
                   &area_stub_cmd);
  cli_install_gen (ctree, OSPF_MODE, PRIVILEGE_NORMAL, 0,
                   &no_area_stub_cmd);
  cli_install_gen (ctree, OSPF_MODE, PRIVILEGE_NORMAL, 0,
                   &no_area_stub_no_summary_cmd);

  /* "area default-cost" commands. */
  cli_install_gen (ctree, OSPF_MODE, PRIVILEGE_NORMAL, 0,
                   &area_default_cost_cmd);
  cli_install_gen (ctree, OSPF_MODE, PRIVILEGE_NORMAL, 0,
                   &no_area_default_cost_cmd);
  
  /* "area range" commands. */
  cli_install_gen (ctree, OSPF_MODE, PRIVILEGE_NORMAL, 0,
                   &area_range_cmd);
  cli_install_gen (ctree, OSPF_MODE, PRIVILEGE_NORMAL, 0,
                   &area_range_advertise_cmd);
  cli_install_gen (ctree, OSPF_MODE, PRIVILEGE_NORMAL, 0,
                   &area_range_not_advertise_cmd);
  cli_install_gen (ctree, OSPF_MODE, PRIVILEGE_NORMAL, 0,
                   &no_area_range_cmd);
  cli_install_gen (ctree, OSPF_MODE, PRIVILEGE_NORMAL, 0,
                   &no_area_range_advertise_cmd);
  cli_install_gen (ctree, OSPF_MODE, PRIVILEGE_NORMAL, CLI_FLAG_HIDDEN,
                   &area_range_substitute_cmd);
  cli_install_gen (ctree, OSPF_MODE, PRIVILEGE_NORMAL, CLI_FLAG_HIDDEN,
                   &no_area_range_substitute_cmd);

  /* "area filter-list prefix" commands. */
  cli_install_gen (ctree, OSPF_MODE, PRIVILEGE_NORMAL, 0,
                   &area_filter_list_prefix_cmd);
  cli_install_gen (ctree, OSPF_MODE, PRIVILEGE_NORMAL, 0,
                   &no_area_filter_list_prefix_cmd);

  /* "area filter-list access" commands. */
  cli_install_gen (ctree, OSPF_MODE, PRIVILEGE_NORMAL, 0,
                   &area_filter_list_access_cmd);
  cli_install_gen (ctree, OSPF_MODE, PRIVILEGE_NORMAL, 0,
                   &no_area_filter_list_access_cmd);

  /* "area export-list" commands. */
  cli_install_gen (ctree, OSPF_MODE, PRIVILEGE_NORMAL, CLI_FLAG_HIDDEN,
                   &area_export_list_cmd);
  cli_install_gen (ctree, OSPF_MODE, PRIVILEGE_NORMAL, CLI_FLAG_HIDDEN,
                   &no_area_export_list_cmd);

  /* "area import-list" commands. */
  cli_install_gen (ctree, OSPF_MODE, PRIVILEGE_NORMAL, CLI_FLAG_HIDDEN,
                   &area_import_list_cmd);
  cli_install_gen (ctree, OSPF_MODE, PRIVILEGE_NORMAL, CLI_FLAG_HIDDEN,
                   &no_area_import_list_cmd);
  
  /* "area shortcut" commands. */
  cli_install_gen (ctree, OSPF_MODE, PRIVILEGE_NORMAL, 0,
                   &area_shortcut_cmd);
  cli_install_gen (ctree, OSPF_MODE, PRIVILEGE_NORMAL, 0,
                   &no_area_shortcut_args_cmd);
  cli_install_gen (ctree, OSPF_MODE, PRIVILEGE_NORMAL, 0,
                   &no_area_shortcut_cmd);
}

#ifdef HAVE_NSSA
void
ospf_cli_nssa_init (struct cli_tree *ctree)
{
  /* "area nssa" commdnds. */
  cli_install_gen (ctree, OSPF_MODE, PRIVILEGE_NORMAL, 0,
                   &area_nssa_cmd);
  cli_install_gen (ctree, OSPF_MODE, PRIVILEGE_NORMAL, 0,
                   &area_nssa_options_cmd);
  cli_install_gen (ctree, OSPF_MODE, PRIVILEGE_NORMAL, 0,
                   &no_area_nssa_cmd);
  cli_install_gen (ctree, OSPF_MODE, PRIVILEGE_NORMAL, 0,
                   &no_area_nssa_options_cmd);

  cli_install_gen (ctree, OSPF_MODE, PRIVILEGE_NORMAL, CLI_FLAG_HIDDEN,
                   &area_nssa_translator_role_cmd);
}
#endif /* HAVE_NSSA */

void
ospf_cli_nsm_init (struct cli_tree *ctree)
{
  /* "redistribute" commands. */
  cli_install_gen (ctree, OSPF_MODE, PRIVILEGE_NORMAL, 0,
                   &ospf_redistribute_cmd);
  cli_install_gen (ctree, OSPF_MODE, PRIVILEGE_NORMAL, 0,
                   &ospf_redistribute_options_cmd);
  cli_install_gen (ctree, OSPF_MODE, PRIVILEGE_NORMAL, 0,
                   &no_ospf_redistribute_cmd);
  cli_install_gen (ctree, OSPF_MODE, PRIVILEGE_NORMAL, 0,
                   &no_ospf_redistribute_options_cmd);
                   
  /* "distribute-list" commands. */
  cli_install_gen (ctree, OSPF_MODE, PRIVILEGE_NORMAL, 0,
                   &ospf_distribute_list_out_cmd);
  cli_install_gen (ctree, OSPF_MODE, PRIVILEGE_NORMAL, 0,
                   &no_ospf_distribute_list_out_cmd);
  cli_install_gen (ctree, OSPF_MODE, PRIVILEGE_NORMAL, 0,
                   &ospf_distribute_list_in_cmd);
  cli_install_gen (ctree, OSPF_MODE, PRIVILEGE_NORMAL, 0,
                   &no_ospf_distribute_list_in_cmd);
  
  /* "default-information" commands. */
  cli_install_gen (ctree, OSPF_MODE, PRIVILEGE_NORMAL, 0,
                   &ospf_default_information_originate_cmd);
  cli_install_gen (ctree, OSPF_MODE, PRIVILEGE_NORMAL, 0,
                   &ospf_default_information_originate_options_cmd);
  cli_install_gen (ctree, OSPF_MODE, PRIVILEGE_NORMAL, 0,
                   &no_ospf_default_information_originate_cmd);
  cli_install_gen (ctree, OSPF_MODE, PRIVILEGE_NORMAL, 0,
                   &no_ospf_default_information_originate_options_cmd);

  /* "default-metric" commands. */
  cli_install_gen (ctree, OSPF_MODE, PRIVILEGE_NORMAL, CLI_PLUS_SIGN_FORBIDDEN,
                   &ospf_default_metric_cmd);
  cli_install_gen (ctree, OSPF_MODE, PRIVILEGE_NORMAL, CLI_PLUS_SIGN_FORBIDDEN,
                   &no_ospf_default_metric_cmd);
  cli_install_gen (ctree, OSPF_MODE, PRIVILEGE_NORMAL, CLI_PLUS_SIGN_FORBIDDEN,
                   &no_ospf_default_metric_val_cmd);
  
  /* "distance" commands. */
  cli_install_gen (ctree, OSPF_MODE, PRIVILEGE_NORMAL, 0,
                   &ospf_distance_cmd);
  cli_install_gen (ctree, OSPF_MODE, PRIVILEGE_NORMAL, 0,
                   &ospf_distance_ospf_cmd);
  cli_install_gen (ctree, OSPF_MODE, PRIVILEGE_NORMAL, 0,
                   &no_ospf_distance_cmd);
  cli_install_gen (ctree, OSPF_MODE, PRIVILEGE_NORMAL, 0,
                   &no_ospf_distance_ospf_cmd);
}

void
ospf_cli_debug_init (struct cli_tree *ctree)
{
  cli_install_config (ctree, DEBUG_MODE, ospf_config_write_debug);

  /* "debug ospf" commands.  */
  cli_install_gen (ctree, EXEC_MODE, PRIVILEGE_NORMAL, 0,
                   &debug_ospf_cmd);
  cli_install_gen (ctree, EXEC_MODE, PRIVILEGE_NORMAL, 0,
                   &no_debug_ospf_cmd);
  cli_install_gen (ctree, EXEC_MODE, PRIVILEGE_NORMAL, 0,
                   &undebug_ospf_cmd);
  cli_install_gen (ctree, CONFIG_MODE, PRIVILEGE_NORMAL, 0,
                   &debug_ospf_cmd);
  cli_install_gen (ctree, CONFIG_MODE, PRIVILEGE_NORMAL, 0,
                   &no_debug_ospf_cmd);

  /* "no debug all ospf" commands.  */
  cli_install_gen (ctree, EXEC_MODE, PRIVILEGE_NORMAL, 0,
                   &no_debug_ospf_all_cmd);
  cli_install_gen (ctree, EXEC_MODE, PRIVILEGE_NORMAL, 0,
                   &undebug_ospf_all_cmd);
  cli_install_gen (ctree, CONFIG_MODE, PRIVILEGE_NORMAL, 0,
                   &no_debug_ospf_all_cmd);

  /* "no debug all" commands.  */
  cli_install_gen (ctree, EXEC_MODE, PRIVILEGE_NORMAL, 0,
                   &no_debug_all_ospf_cmd);
  cli_install_gen (ctree, EXEC_MODE, PRIVILEGE_NORMAL, 0,
                   &undebug_all_ospf_cmd);
  cli_install_gen (ctree, CONFIG_MODE, PRIVILEGE_NORMAL, 0,
                   &no_debug_all_ospf_cmd);

  /* "debug ospf packet" commands.  */
  cli_install_gen (ctree, EXEC_MODE, PRIVILEGE_NORMAL, 0,
                   &debug_ospf_packet_cmd);
  cli_install_gen (ctree, EXEC_MODE, PRIVILEGE_NORMAL, 0,
                   &no_debug_ospf_packet_cmd);
  cli_install_gen (ctree, EXEC_MODE, PRIVILEGE_NORMAL, 0,
                   &undebug_ospf_packet_cmd);
  cli_install_gen (ctree, CONFIG_MODE, PRIVILEGE_NORMAL, 0,
                   &debug_ospf_packet_cmd);
  cli_install_gen (ctree, CONFIG_MODE, PRIVILEGE_NORMAL, 0,
                   &no_debug_ospf_packet_cmd);

  /* "debug ospf ifsm" commands.  */
  cli_install_gen (ctree, EXEC_MODE, PRIVILEGE_NORMAL, 0,
                   &debug_ospf_ifsm_cmd);
  cli_install_gen (ctree, EXEC_MODE, PRIVILEGE_NORMAL, 0,
                   &no_debug_ospf_ifsm_cmd);
  cli_install_gen (ctree, EXEC_MODE, PRIVILEGE_NORMAL, 0,
                   &undebug_ospf_ifsm_cmd);
  cli_install_gen (ctree, CONFIG_MODE, PRIVILEGE_NORMAL, 0,
                   &debug_ospf_ifsm_cmd);
  cli_install_gen (ctree, CONFIG_MODE, PRIVILEGE_NORMAL, 0,
                   &no_debug_ospf_ifsm_cmd);

  /* "debug ospf nfsm" commands.  */
  cli_install_gen (ctree, EXEC_MODE, PRIVILEGE_NORMAL, 0,
                   &debug_ospf_nfsm_cmd);
  cli_install_gen (ctree, EXEC_MODE, PRIVILEGE_NORMAL, 0,
                   &no_debug_ospf_nfsm_cmd);
  cli_install_gen (ctree, EXEC_MODE, PRIVILEGE_NORMAL, 0,
                   &undebug_ospf_nfsm_cmd);
  cli_install_gen (ctree, CONFIG_MODE, PRIVILEGE_NORMAL, 0,
                   &debug_ospf_nfsm_cmd);
  cli_install_gen (ctree, CONFIG_MODE, PRIVILEGE_NORMAL, 0,
                   &no_debug_ospf_nfsm_cmd);

  /* "debug ospf lsa" commands.  */
  cli_install_gen (ctree, EXEC_MODE, PRIVILEGE_NORMAL, 0,
                   &debug_ospf_lsa_cmd);
  cli_install_gen (ctree, EXEC_MODE, PRIVILEGE_NORMAL, 0,
                   &no_debug_ospf_lsa_cmd);
  cli_install_gen (ctree, EXEC_MODE, PRIVILEGE_NORMAL, 0,
                   &undebug_ospf_lsa_cmd);
  cli_install_gen (ctree, CONFIG_MODE, PRIVILEGE_NORMAL, 0,
                   &debug_ospf_lsa_cmd);
  cli_install_gen (ctree, CONFIG_MODE, PRIVILEGE_NORMAL, 0,
                   &no_debug_ospf_lsa_cmd);

  /* "debug ospf nsm" commands.  */
  cli_install_gen (ctree, EXEC_MODE, PRIVILEGE_NORMAL, 0,
                   &debug_ospf_nsm_cmd);
  cli_install_gen (ctree, EXEC_MODE, PRIVILEGE_NORMAL, 0,
                   &no_debug_ospf_nsm_cmd);
  cli_install_gen (ctree, EXEC_MODE, PRIVILEGE_NORMAL, 0,
                   &undebug_ospf_nsm_cmd);
  cli_install_gen (ctree, CONFIG_MODE, PRIVILEGE_NORMAL, 0,
                   &debug_ospf_nsm_cmd);
  cli_install_gen (ctree, CONFIG_MODE, PRIVILEGE_NORMAL, 0,
                   &no_debug_ospf_nsm_cmd);

  /* "debug ospf events" commands.  */
  cli_install_gen (ctree, EXEC_MODE, PRIVILEGE_NORMAL, 0,
                   &debug_ospf_events_cmd);
  cli_install_gen (ctree, EXEC_MODE, PRIVILEGE_NORMAL, 0,
                   &no_debug_ospf_events_cmd);
  cli_install_gen (ctree, EXEC_MODE, PRIVILEGE_NORMAL, 0,
                   &undebug_ospf_events_cmd);
  cli_install_gen (ctree, CONFIG_MODE, PRIVILEGE_NORMAL, 0,
                   &debug_ospf_events_cmd);
  cli_install_gen (ctree, CONFIG_MODE, PRIVILEGE_NORMAL, 0,
                   &no_debug_ospf_events_cmd);

  /* "debug ospf route" commands.  */
  cli_install_gen (ctree, EXEC_MODE, PRIVILEGE_NORMAL, 0,
                   &debug_ospf_route_cmd);
  cli_install_gen (ctree, EXEC_MODE, PRIVILEGE_NORMAL, 0,
                   &no_debug_ospf_route_cmd);
  cli_install_gen (ctree, EXEC_MODE, PRIVILEGE_NORMAL, 0,
                   &undebug_ospf_route_cmd);
  cli_install_gen (ctree, CONFIG_MODE, PRIVILEGE_NORMAL, 0,
                   &debug_ospf_route_cmd);
  cli_install_gen (ctree, CONFIG_MODE, PRIVILEGE_NORMAL, 0,
                   &no_debug_ospf_route_cmd);

#ifdef HAVE_BFD
  /* "debug ospf bfd" commands.  */
  cli_install_gen (ctree, EXEC_MODE, PRIVILEGE_MAX, 0,
                   &debug_ospf_bfd_cmd);
  cli_install_gen (ctree, EXEC_MODE, PRIVILEGE_MAX, 0,
                   &no_debug_ospf_bfd_cmd);
  cli_install_gen (ctree, EXEC_MODE, PRIVILEGE_MAX, 0,
                   &undebug_ospf_bfd_cmd);
  cli_install_gen (ctree, CONFIG_MODE, PRIVILEGE_MAX, 0,
                   &debug_ospf_bfd_cmd);
  cli_install_gen (ctree, CONFIG_MODE, PRIVILEGE_MAX, 0,
                   &no_debug_ospf_bfd_cmd);
#endif /* HAVE_BFD */

#ifdef HAVE_OSPF_CSPF
  /* "debug cspf event" commands.  */
  cli_install_gen (ctree, EXEC_MODE, PRIVILEGE_NORMAL, 0,
                   &debug_cspf_event_cmd);
  cli_install_gen (ctree, EXEC_MODE, PRIVILEGE_NORMAL, 0,
                   &no_debug_cspf_event_cmd);
  cli_install_gen (ctree, EXEC_MODE, PRIVILEGE_NORMAL, 0,
                   &undebug_cspf_event_cmd);
  cli_install_gen (ctree, CONFIG_MODE, PRIVILEGE_NORMAL, 0,
                   &debug_cspf_event_cmd);
  cli_install_gen (ctree, CONFIG_MODE, PRIVILEGE_NORMAL, 0,
                   &no_debug_cspf_event_cmd);

  /* "debug cspf hexdump" commands.  */
  cli_install_gen (ctree, EXEC_MODE, PRIVILEGE_NORMAL, 0,
                   &debug_cspf_hexdump_cmd);
  cli_install_gen (ctree, EXEC_MODE, PRIVILEGE_NORMAL, 0,
                   &no_debug_cspf_hexdump_cmd);
  cli_install_gen (ctree, EXEC_MODE, PRIVILEGE_NORMAL, 0,
                   &undebug_cspf_hexdump_cmd);
  cli_install_gen (ctree, CONFIG_MODE, PRIVILEGE_NORMAL, 0,
                   &debug_cspf_hexdump_cmd);
  cli_install_gen (ctree, CONFIG_MODE, PRIVILEGE_NORMAL, 0,
                   &no_debug_cspf_hexdump_cmd);
#endif /* HAVE_OSPF_CSPF */
}

#ifdef HAVE_SNMP_RESTART
CLI (snmp_restart_ospf,
     snmp_restart_ospf_cli,
     "snmp restart ospf",
     "snmp",
     "restart",
     "ospf")
{
  ospf_snmp_restart ();
  return CLI_SUCCESS;
}

void
ospf_cli_snmp_init (struct cli_tree *ctree)
{
  cli_install_gen (ctree, CONFIG_MODE, PRIVILEGE_MAX, CLI_FLAG_HIDDEN,
                   &snmp_restart_ospf_cli);
}
#endif /* HAVE_SNMP_RESTART */

void
ospf_cli_init (struct lib_globals *zg)
{
  struct cli_tree *ctree = zg->ctree;

  /* Install `router ospf' related commands. */
  ospf_cli_router_init (ctree);

  /* Install `ospf router-id' related commnds. */
  ospf_cli_ospf_init (ctree);

  /* Install `ip ospf' related commands. */
  ospf_cli_if_init (ctree);

#if defined HAVE_GMPLS && defined HAVE_OSPF_TE
  /* Install te-link related commands */
  ospf_cli_telink_init (ctree);
#endif /* HAVE_GMPLS */

  /* Install `area vlink' related commands. */
  ospf_cli_vlink_init (ctree);
#ifdef HAVE_OSPF_MULTI_AREA
  ospf_cli_multi_area_link_init (ctree);
#endif /* HAVE_OSPF_MULTI_AREA */
  /* Install `area' related commands. */
  ospf_cli_abr_init (ctree);
#ifdef HAVE_NSSA
  /* Install `area nssa' related commands. */
  ospf_cli_nssa_init (ctree);
#endif /* HAVE_NSSA */

  /* Install pacos related commands. */
  ospf_cli_nsm_init (ctree);

  /* Install `debug' related commands. */
  ospf_cli_debug_init (ctree);

#ifdef HAVE_BFD
  ospf_bfd_cli_init (ctree);
#endif /* HAVE_BFD */

#ifdef HAVE_SNMP_RESTART
  ospf_cli_snmp_init (ctree);
#endif /* HAVE_SNMP_RESTART */
}
