/* Copyright (C) 2001-2003  All Rights Reserved. */

#include "pal.h"
#include "lib.h"
#include "if.h"
#include "table.h"
#include "prefix.h"
#include "thread.h"
#include "nexthop.h"
#include "cli.h"
#include "host.h"
#include "snprintf.h"
#include "ptree.h"
#include "filter.h"
#include "lib_mtrace.h"
#include "hash.h"


#include "nsm_sec_cli.h"
#include "nsm_oam_1731_cli.h"

#ifdef HAVE_HAL
#include "hal_incl.h"
#endif /* HAVE_HAL */
#include "nsmd.h"

#ifdef HAVE_QOS
#include "nsm_qos.h"
#include "nsm_qos_cli.h"
#endif /* HAVE_QOS */

#ifdef HAVE_L2
#include "nsm_bridge.h"

#ifdef HAVE_VLAN
#include "nsm_vlan.h"

#ifdef HAVE_PVLAN
#include "nsm_pvlan.h"
#endif /* HAVE_PVLAN */

#ifdef HAVE_VLAN_CLASS
#include "nsm_vlanclassifier.h"
#endif /* HAVE_VLAN_CLASS */

#endif /* HAVE_VLAN */

#endif /* HAVE_L2 */

#include "nsm/nsm_interface.h"
#include "nsm_vrf.h"

#ifdef HAVE_L3
#include "nsm/rib/nsm_table.h"
#include "nsm/rib/rib.h"
#endif /* HAVE_L3 */

#ifdef HAVE_VRF
#include "nsm/rib/nsm_rib_vrf.h"
#endif /* HAVE_VRF */

#include "nsm/nsm_debug.h"
#include "nsm/nsm_api.h"
#include "nsm/nsm_router.h"
#include "nsm/nsm_fea.h"
#include "nsm/nsm_server.h"

#ifdef HAVE_L3
#include "nsm/rib/nsm_static_mroute.h"
#ifdef HAVE_RTADV
#include "nsm/nsm_rtadv.h"
#endif /* HAVE_RTADV */
#endif /* HAVE_L3 */

#ifdef HAVE_NSM_IF_UNNUMBERED
#include "nsm/nsm_if_unnumbered.h"
#endif /* HAVE_NSM_IF_UNNUMBERED */

#ifdef HAVE_TUNNEL
#include "nsm/tunnel/nsm_tunnel.h"
#endif /* HAVE_TUNNEL */

#ifdef HAVE_GMPLS
#include "nsm/gmpls/nsm_gmpls.h"
#endif /* HAVE_GMPLS */

#ifdef HAVE_VRRP
#include "nsm/vrrp/vrrp_cli.h"
#endif /* HAVE_VRRP */

#ifdef HAVE_QOS
#include "nsm_qos.h"
#include "nsm_qos_cli.h"
#endif /* HAVE_QOS */

#ifdef HAVE_MCAST_IPV4
#include "nsm_mcast4.h"
#endif /* HAVE_MCAST_IPV4 */

#ifdef HAVE_MCAST_IPV6
#include "nsm_mcast6.h"
#endif /* HAVE_MCAST_IPV6 */

#ifdef HAVE_LACPD
#include "nsm_static_aggregator_cli.h"
#include "nsm_lacp.h"
#endif /*HAVE_LACPD*/

#ifdef HAVE_L2
#include "L2/nsm_bridge_cli.h"
#include "L2/nsm_stats_api.h"
#endif /* HAVE_L2 */

#ifdef HAVE_QOS
#include "nsm_qos.h"
#include "nsm_qos_cli.h"
#endif /* HAVE_QOS */

#ifdef HAVE_USER_HSL
#include "nsm_hw_reg_api.h"
#endif /* HAVE_USER_HSL */
#ifdef HAVE_L2LERN
#include "nsm_mac_acl_cli.h"
#endif /* HAVE_L2LERN */

#ifdef HAVE_WMI
#include "wmi_server.h"
#include "nsm_l2_wmi.h"

#define  WMI_STR             "Web Management Interface (WMI)"

#define WMI_NSM_DEBUG_ON(a) (nsm_wmi_server->debug |= WMI_DEBUG_FLAG_ ## a)
#define WMI_NSM_DEBUG_OFF(a) \
        (nsm_wmi_server->debug &= ~(WMI_DEBUG_FLAG_ ## a))
#define IS_DEBUG_WMI_NSM(a) (nsm_wmi_server->debug & WMI_EBUG_FLAG_ ## a)
#endif /* HAVE_WMI */

#ifdef HAVE_HA
#include "nsm_cal.h"
#endif /* HAVE_HA */

#ifdef HAVE_VR
void nsm_vr_cli_init (struct cli_tree *);
#endif /* HAVE_VR */

#if defined HAVE_MCAST_IPV4 || defined HAVE_IGMP_SNOOP
#include "igmp.h"
#endif /* HAVE_MCAST_IPV4 || HAVE_IGMP_SNOOP */
#if defined HAVE_MCAST_IPV6 || defined HAVE_MLD_SNOOP
#include "mld.h"
#endif /* HAVE_MCAST_IPV6 || HAVE_MLD_SNOOP */

#if defined HAVE_I_BEB || defined HAVE_B_BEB
#include "nsm_pbb_cli.h"
#endif /* defined HAVE_I_BEB || defined HAVE_B_BEB */

#ifdef HAVE_BFD
#include "nsm/nsm_bfd_cli.h"
#endif /* HAVE_BFD */

#ifdef HAVE_PBB_TE
#include "nsm_pbb_te_cli.h"
#endif /* HAVE_PBB_TE */

#ifdef HAVE_BFD
#include "nsm_bfd.h"
#endif /* HAVE_BFD */

#ifdef HAVE_DCB
#include "nsm/L2/nsm_dcb_cli.h"
#endif /* HAVE_DCB */

int
nsm_cli_return (struct cli *cli, int ret)
{
  char *str = NULL;

  switch (ret)
    {
    case NSM_API_SET_SUCCESS:
      return CLI_SUCCESS;

    case NSM_API_SET_ERROR:
      return CLI_ERROR;

    case NSM_API_SET_ERR_VR_CANT_CREATE:
      str = "Cannot create new VR";
      break;
    case NSM_API_SET_ERR_VR_NOT_EXIST:
      str = "No such VR";
      break;
    case NSM_API_SET_ERR_VR_BOUND:
      str = "VR is already associated";
      break;
    case NSM_API_SET_ERR_VR_NOT_BOUND:
      str = "Interface is not bound to VR";
      break;
    case NSM_API_SET_ERR_DIFFERENT_VR_BOUND:
      str = "Different VR bound";
      break;
    case NSM_API_SET_ERR_IPV4_FORWARDING_CANT_CHANGE:
      str = "Can't change IP forwarding state";
      break;
    case NSM_API_SET_ERR_IPV6_FORWARDING_CANT_CHANGE:
      str = "Can't change IPv6 forwarding state";
      break;
    case NSM_API_SET_ERR_VR_INVALID_NAME:
      str = "Invalid VR name, cant create VR";
      break;
    case NSM_API_SET_ERR_VR_INVALID_NAMELEN:
      str = "VR name exceeds " VAR_SUPERSTR (LIB_VR_MAX_NAMELEN) " characters, cant create VR";
      break;
    case NSM_API_SET_ERR_VRF_NOT_EXIST:
      str = "No such VRF";
      break;
    case NSM_API_SET_ERR_VRF_NOT_BOUND:
      str = "Interface is not bound to VRF";
      break;
    case NSM_API_SET_ERR_VRF_BOUND:
      str = "VRF is already associated";
      break;
    case NSM_API_SET_ERR_VPLS_BOUND:
      str = "Interface is bound to VPLS";
      break;
    case NSM_API_SET_ERR_DIFFERENT_VRF_BOUND:
      str = "Different VRF bound";
      break;
    case NSM_API_SET_ERR_VRF_NAME_TOO_LONG:
      str = "VRF name is too long";
      break;
    case NSM_API_SET_ERR_VRF_CANT_CREATE:
      str = "Cannot create new VRF: System resource limit exceeded";
      break;

    case NSM_API_SET_ERR_NO_PERMISSION:
      str = "No permission to do this";
      break;
    case NSM_API_SET_ERR_NO_IPV4_SUPPORT:
      str = "No IPv4 support";
      break;
    case NSM_API_SET_ERR_NO_IPV6_SUPPORT:
      str = "No IPv6 support";
      break;
    case NSM_API_SET_ERR_MAX_STATIC_ROUTE_LIMIT:
      str = "Exceeding maximum static routes limit";
      break;
    case NSM_API_SET_ERR_MAX_FIB_ROUTE_LIMIT:
      str = "Exceeding maximum fib routes limit";
      break;
    case NSM_API_SET_ERR_MAX_ADDRESS_LIMIT:
      str = "Exceeding maximum address limit";
      break;

    case NSM_API_SET_ERR_IF_NOT_EXIST:
      str = "Interface does not exist";
      break;
    case NSM_API_SET_ERR_NO_SUCH_INTERFACE:
      str = "No such interface";
      break;
    case NSM_API_SET_ERR_IF_NOT_ACTIVE:
      str = "Interface is not active";
      break;
    case NSM_API_SET_ERR_INVALID_IF:
      str = "Invalid interface";
      break;
    case NSM_API_SET_ERR_INVALID_IF_TYPE:
      str = "Invalid interface type";
      break;
    case NSM_API_SET_ERR_FLAG_UP_CANT_SET:
      str = "Can't up interface";
      break;
    case NSM_API_SET_ERR_FLAG_UP_CANT_UNSET:
      str = "Can't shutdown interface";
      break;
    case NSM_API_SET_ERR_FLAG_MULTICAST_CANT_SET:
      str = "Can't set multicast flag";
      break;
    case NSM_API_SET_ERR_FLAG_MULTICAST_CANT_UNSET:
      str = "Can't unset multicast flag";
      break;
    case NSM_API_SET_ERR_INVALID_MTU_SIZE:
      str = "Given MTU is not allowed on this interface";
      break;
    case NSM_API_SET_ERR_DUPLEX:
      str = "Duplex set failed";
      break;
    case NSM_API_SET_ERR_ARP_AGEING_TIMEOUT:
      str = "ARP Ageing Timeout set failed";
      break;
    case NSM_API_SET_ARP_GENERAL_ERR:
      str = "NSM ARP General Error";
      break;

    case NSM_API_SET_ERR_ADDRESS_NOT_EXIST:
      str = "Can't find address";
      break;
    case NSM_API_SET_ERR_INVALID_PREFIX_LENGTH:
      str = "Invalid prefix length";
      break;
    case NSM_API_SET_ERR_PREFIX_NOT_EXIST:
      str = "Non-exist prefix";
      break;
    case NSM_API_SET_ERR_CANT_SET_ADDRESS:
      str = "Can't set interface IP address";
      break;
    case NSM_API_SET_ERR_CANT_UNSET_ADDRESS:
      str = "Can't unset interface IP address";
      break;
    case NSM_API_SET_ERR_CANT_UNSET_ADDRESS_VRRP:
      str = "Can't unset interface IP address: VRRP vip address owner.";
      break;
    case NSM_API_SET_ERR_CANT_SET_ADDRESS_ON_P2P:
      str = "Can't set IP address on PPP interfaces";
      break;
    case NSM_API_SET_ERR_INVALID_IPV4_ADDRESS:
      str = "Invalid IPv4 address";
      break;
    case NSM_API_SET_ERR_INVALID_IPV4_ADDRESS_VRRP:
      str = "Conflicting VRRP address found (cannot set)";
      break;
    case NSM_API_SET_ERR_INVALID_IPV4_NEXTHOP:
      str = "Invalid IPv4 nexthop";
      break;
    case NSM_API_SET_ERR_INVALID_IPV6_ADDRESS:
      str = "Invalid IPv6 address";
      break;
    case NSM_API_SET_ERR_INVALID_IPV6_NEXTHOP:
      str = "Invalid IPv6 nexthop";
      break;
    case NSM_API_SET_ERR_INVALID_IPV6_NEXTHOP_LINKLOCAL:
      str = "Interface has to be specified for a link-local nexthop";
      break;
    case NSM_API_SET_ERR_CANT_SET_SECONDARY_FIRST:
      str = "Configure primary first";
      break;
    case NSM_API_SET_ERR_CANT_CHANGE_PRIMARY:
      str = "Secondary can't be same as primary";
      break;
    case NSM_API_SET_ERR_CANT_CHANGE_SECONDARY:
      str = "Primary can't be same as secondary";
      break;
    case NSM_API_SET_ERR_MUST_DELETE_SECONDARY_FIRST:
      str = "Must delete secondary before deleting primary";
      break;
    case NSM_API_SET_ERR_MALFORMED_ADDRESS:
      str = "Malformed address";
      break;
    case NSM_API_SET_ERR_INCONSISTENT_ADDRESS_MASK:
      str = "Inconsistent address and mask";
      break;
    case NSM_API_SET_ERR_MALFORMED_GATEWAY:
      str = "Malformed gateway or interface not found";
      break;
    case NSM_API_SET_ERR_NO_MATCHING_ROUTE:
      str = "No matching route to delete";
      break;
#ifdef HAVE_BFD
    case NSM_API_SET_ERR_NO_STATIC_ROUTE:
      str = "Can't find static route";
      break;
    case NSM_API_SET_ERR_STATIC_BFD_SET:
      str = "Static BFD already enabled";
      break;
    case NSM_API_SET_ERR_STATIC_BFD_UNSET:
      str = "Static BFD not enabled";
      break;
    case NSM_API_SET_ERR_STATIC_BFD_DISABLE_SET:
      str = "Static BFD already disabled";
      break;
    case NSM_API_SET_ERR_STATIC_BFD_DISABLE_UNSET:
      str = "Static BFD not disabled";
      break;
#endif /* HAVE_BFD */

#ifdef HAVE_TUNNEL
    case NSM_API_SET_ERR_TUNNEL_INVALID_MODE:
      str = "Invalid tunnel mode";
      break;
    case NSM_API_SET_ERR_TUNNEL_INVALID_SUBMODE:
      str = "Invalid tunnel mode";
      break;
    case NSM_API_SET_ERR_TUNNEL_IF_NOT_EXIST:
      str = "No tunnel interface found";
      break;
    case NSM_API_SET_ERR_TUNNEL_IF_ADD_FAIL:
      str = "Tunnel could not be configured due to missing parameters";
      break;
    case NSM_API_SET_ERR_TUNNEL_IF_DELETE_FAIL:
      str = "Tunnel could not be deleted";
      break;
    case NSM_API_SET_ERR_TUNNEL_IF_UPDATE_FAIL:
      str = "Tunnel interface update fail";
      break;
    case NSM_API_SET_ERR_TUNNEL_IF_UPDATE_FAIL_TTL:
      str = "TTL configuration requires path MTU discovery";
      break;
    case NSM_API_SET_ERR_TUNNEL_DEST_MUST_UNCONFIGURED:
      str = "The tunnel destination must be unconfigured "
        "before setting this mode";
      break;
    case NSM_API_SET_ERR_TUNNEL_DEST_CANT_CONFIGURED:
      str = "The tunnel destination can not be configured "
        "under the existing mode";
      break;
#endif /* HAVE_TUNNEL */

#ifdef HAVE_NSM_IF_UNNUMBERED
    case NSM_API_SET_ERR_UNNUMBERED_NOT_P2P:
      str = "Point-to-point (non-multi-access) interface only";
      break;
    case NSM_API_SET_ERR_UNNUMBERED_SOURCE:
      str = "Cannot use an unnumbered interface as a source";
      break;
    case NSM_API_SET_ERR_UNNUMBERED_RECURSIVE:
      str = "The interface is refered by other unnumbered interface(s)";
      break;
#endif /* HAVE_NSM_IF_UNNUMBERED */

#ifdef HAVE_LACPD
    case NSM_API_SET_ERR_EMPTY_AGGREGATOR_DOWN:
      str = "Can't up an empty aggregator";
      break;
#endif /* HAVE_LACPD */

    case NSM_API_SET_ERR_ENTRY_EXISTS:
      str = "Prefix and MAC are from same interface";
      break;
    case NSM_API_SET_ERR_IF_SAME:
      str = "Duplicate Entry";
      break;
    case NSM_API_SET_ERR_IF_NOT_PROXY_ARP:
      str = "Proxy Arp is not enabled in the interface";
      break;

    case NSM_API_SET_ERR_ACL_INVALID_VALUE:
      str = "Invalid Access list Name";
      break;
    case NSM_API_SET_ERR_NEXTHOP_OWN_ADDR:
      str = "Invalid nexthop address. (It's this router).";
      break;
    case NSM_API_ACL_ERR_NOT_SET:
      str = "Access-group not configured";
      break;

#ifdef HAVE_GMPLS
    case NSM_API_SET_ERR_IF_TYPE_NOT_MATCH:
      str = " Interface Type not match";
      break;
    case NSM_API_SET_ERR_IF_NOT_FOUND:
      str = " Interface not found";
      break;
    case NSM_API_SET_ERR_INTERFACE_GMPLS_TYPE_EXIST:
      str = " Unconfigure gmpls interface-type first";
      break;
    case NSM_API_SET_ERR_IF_TYPE_NOT_DATA:
      str = " Interface Type is not Data/Data-Control";
      break;
    case NSM_API_SET_ERR_INVALID_IF_ID:
      str = " Invalid Interface ID";
      break;
    case NSM_API_SET_ERR_INVALID_SWITCH_CAP:
      str = " Invalid Switching Capability";
      break;
    case NSM_API_SET_ERR_SWITCH_CAP_ALREADY_CONFIGURED:
      str = " Same Switching Capability is already configured";
      break;
    case NSM_API_SET_ERR_SWITCH_CAP_NOT_MATCH:
      str = " Switching Capability not match";
      break;
    case NSM_API_SET_ERR_INVALID_BW:
      str = " Invalid Bandwidth";
      break;
    case NSM_API_SET_ERR_BW_NOT_MATCH:
      str = " Bandwidth not match";
      break;
    case NSM_API_SET_ERR_TE_LINK_NOT_EXIST:
      str = "TE Link does not exist";
      break;
    case NSM_API_SET_ERR_DATA_LINK_NOT_EXIST:
      str = "Data Link does not exist";
      break;
    case NSM_API_SET_ERR_CC_NOT_EXIST:
      str = "Control Channel does not exist";
      break;
    case NSM_API_SET_ERR_CA_NOT_EXIST:
      str = "Control Adjacency does not exist";
      break;
    case NSM_API_SET_ERR_INVALID_DATA_LINK:
      str = "Invalid Data Link";
      break;
    case NSM_API_SET_ERR_INVALID_TE_LINK:
      str = "Invalid TE Link";
      break;
    case NSM_API_SET_ERR_INVALID_CLASS_TYPE:
      str = "Invalid Class Type";
      break;
    case NSM_API_SET_ERR_INVALID_DESC:
      str = "Invalid Description";
      break;
    case NSM_API_SET_ERR_INVALID_ADMIN_GROUP:
      str = "Invalid administrative group specified";
      break;
    case NSM_API_SET_ERR_ADMIN_GROUP_EXIST:
      str = "TE LINK is already part of the specified administrative group";
      break;
    case NSM_API_SET_ERR_ADMIN_GROUP_NOT_EXIST:
      str = "TE Link does not belong to the specified administrative group";
      break;
    case NSM_API_SET_ERR_CA_RNODE_ID_MISMATCH:
      str = "Control adjacnecy Remote Node ID Mismatch, Delete existing adjacency and create new with modified Remote Node ID";
      break;
    case NSM_API_SET_ERR_CA_LMP_MISMATCH:
      str = "Control adjacnecy LMP/Static Mismatch";
      break;
    case NSM_API_SET_ERR_CA_UNIQUE_NODE_ID:
      str = "Another Control adjacnecy with Same Remote Node ID exists";
      break;
    case NSM_API_SET_ERR_CC_CA_EXIST:
      str = "Control Channel has binded to a control adjacency";
      break;
    case NSM_API_SET_ERR_TE_CA_EXIST:
      str = "TE Link has binded to a control adjacency";
      break;
    case NSM_API_SET_ERR_CC_DUP_ADDR:
      str = "Control Channel has duplicate Local+Remote address";
      break;
    case NSM_API_SET_ERR_CA_DUP_PRIMARY_CC:
      str = "Control Adjacency has duplicate primary control-channel";
      break;
    case NSM_API_SET_ERR_DL_ANOTHER_TL:
      str = "Same Data Link can not be part of multiple TE links";
      break;
    case NSM_API_SET_ERR_DL_TL_EXISTS:
      str = "Data Link already part of this TE link";
      break;
    case NSM_API_SET_ERR_ONE_DL_ALLOWED_ON_TL:
      str = "Multiple data links not allowed on a TE link";
      break;
    case NSM_API_SET_ERR_DL_SW_CAP_MISMATCH:
      str = "Data link with different switching capability not allowed on a TE link";
      break;
    case NSM_API_SET_ERR_TL_EXISTS:
      str = "TE Link already exists";
      break;
    case NSM_API_SET_ERR_LINKID_EXIST:
      str = " Link Creation Failed: link-id should be unique within datalink local-link-id, telink local-link-id and cc-id";
      break;
    case NSM_API_SET_ERR_LLID_MISMATCH:
      str = "TE Link Local Link ID Mismatch, Delete existing TE link and create a new TE Link with modified Local Link ID";
      break;
    case NSM_API_SET_ERR_TE_LINK_UNNUMBERED:
      str = "IP Address can not be configured on unnumbered TE Link";
      break;
    case NSM_API_SET_ERR_TE_LABEL_SPACE:
      str = "Cannot enable/disable label-switching on TE Link";
      break;
    case NSM_API_SET_ERR_DL_UNNUMBERED_TL_NUMBERED:
      str = "Unnumbered Data Links not allowed on "
            "Numbered TE links";
      break;
    case NSM_API_SET_ERR_DL_NUMBERED_TL_UNNUMBERED:
      str = "Numbered Data Links not allowed on "
            "Unnumbered TE links";
      break;
    case NSM_API_SET_ERR_CC_NOT_BINDED_TO_THIS_CA:
      str = "Control Channel is not binded to this Control Adjacency";
      break;
    case NSM_API_SET_ERR_DL_NOT_BINDED_TO_THIS_TL:
      str = "Data Link is not binded to this TE Link";
      break;
#endif /*HAVE_GMPLS */
    case NSM_API_SET_ERR_BW_IN_USE:
      str = "Bandwidth in use. Cannot modify ";
      break;
    case NSM_API_SET_ERR_INVALID_BW_CONSTRAINT:
      str = "Bandwidth constraints exceeds Maximum Reservable Bandwidth";
      break;
#ifdef HAVE_RTADV
    case NSM_API_SET_ERR_IPV6_RA_MIN_INTERVAL_VALUE:
      str = " Min-ra-interval value is greater than 0.75*ra-interval.";
      break;
    case NSM_API_SET_ERR_IPV6_RA_PREFIX_PREFERRED_LIFETIME:
      str = "Preferred lifetime value must not be larger than valid"
            " lifetime value.";
      break;
#endif /* HAVE_RTADV */
#ifdef HAVE_VPLS
    case NSM_ERR_VPLS_NOT_FOUND:
      str = "No VPLS found with given name";
      break;
    case NSM_ERR_VPLS_PEER_NOT_FOUND:
      str = "VPLS Peer not configured";
      break;
    case NSM_ERR_INVALID_VPLS_ID:
      str = "INVALID VPLS ID";
      break;
    case NSM_ERR_INVALID_VPLS_NAME:
      str = "INVALID VPLS Name";
      break;
    case NSM_ERR_VPLS_PEER_PW_MANUAL_NOT_CONFIGURED:
      str = "VPLS Peer not configured as Manual";
      break;
    case NSM_ERR_VPLS_SPOKE_VC_MANUAL_NOT_CONFIGURED:
      str = "VPLS Spoke VC not configured as Manual";
      break;
    case NSM_ERR_VPLS_INVALID_INCOMING_LABEL:
      str = "Invalid Incoming Label";
      break;
    case NSM_ERR_VPLS_INVALID_OUTGOING_LABEL:
      str = "Invalid Outgoing Label";
      break;
    case NSM_ERR_VPLS_INVALID_INCOMING_INTERFACE:
      str = "Invalid Incoming Interface";
      break;
    case NSM_ERR_VPLS_INVALID_OUTGOING_INTERFACE:
      str = "Invalid Outgoing Interface";
      break;
    case NSM_ERR_VPLS_INCOMING_LABEL_MISMATCH:
      str = "Incoming Label mismatch with the configured value";
      break;
    case NSM_ERR_VPLS_OUTGOING_LABEL_MISMATCH:
      str = "Outgoing Label mismatch with the configured value";
      break;
    case NSM_ERR_VPLS_INCOMING_INTERFACE_MISMATCH:
      str = "Incoming Interface mismatch with the configured value";
      break;
    case NSM_ERR_VPLS_OUTGOING_INTERFACE_MISMATCH:
      str = "Outgoing Interface mismatch with the configured value";
      break;
    case NSM_ERR_VPLS_FIB_ADD_FAILED:
      str = "VPLS Fib entry add process failed";
      break;
    case NSM_ERR_VPLS_FIB_ENTRY_NOT_FOUND:
      str = "VPLS Fib entry not present";
      break;
    case NSM_ERR_VPLS_TUNNEL_ID_MISMATCH:
      str = "Tunnel-id mismatch with the configured value";
      break;
    case NSM_ERR_VPLS_TUNNEL_NAME_MISMATCH:
      str = "Tunnel name mismatch with the configured value";
      break;
    case NSM_ERR_VPLS_TUNNEL_DIR_MISMATCH:
      str = "Tunnel Direction mismatch with the configured value";
      break;
    case NSM_ERR_VPLS_VC_TYPE_MISMATCH:
      str = "VPLS vc type mismatch with the configured value";
      break;
    case NSM_ERR_VPLS_SPOKE_CONF_PW_TYPE_MISMATCH:
      str = "VPLS Spoke configuration mismatch with VC configuration.";
      break;
    case NSM_ERR_VPLS_SPOKE_VC_IN_USE:
      str = "VC in use. Unbind VC from VPLS and update VC. ";
      break;
    case NSM_ERR_SPOKE_VC_VIRTUAL_CIRCUIT_NOT_FOUND:
      str = "VPLS Spoke Virtual Circuit not configured.";
      break;
    case NSM_ERR_SPOKE_VC_NOT_FOUND:
      str = "VPLS Spoke not configured.";
      break;
    case NSM_ERR_VPLS_MESH_OWNER_MISMATCH:
      str = "Operation Failed. Owner cannot be changed when peer is active/UP.";
      break;
#endif /* HAVE_VPLS */
#if defined (HAVE_MPLS_OAM) && defined (HAVE_BFD)
    case NSM_MPLS_BFD_ERR_INVALID_INTVL:
      str = "MPLS BFD LSP Ping Interval should be atleast 10 times "
        "greater than BFD min TX & min Rx.";
      break;
    case NSM_MPLS_BFD_ERR_LSP_UNKNOWN:
      str = "BFD MPLS not supported for this LSP-Type";
      break;
    case NSM_MPLS_BFD_ERR_ENTRY_EXISTS:
      str = "MPLS BFD Entry exists";
      break;
    case NSM_MPLS_BFD_ERR_ENTRY_NOT_FOUND:
      str = "MPLS BFD not configured for this entry";
      break;
    case NSM_MPLS_BFD_ERR_INVALID_PREFIX:
      str = "Invalid FEC Prefix";
      break;
#endif /* HAVE_MPLS_OAM && HAVE_BFD */
#ifdef HAVE_MPLS_VC
#ifdef HAVE_MS_PW
    case NSM_ERR_MS_PW_NOT_FOUND:
      str = "MS PW not found.";
      break;
    case NSM_ERR_MS_PW_ALREADY_STITCHED:
      str = "VC is already stitched.";
      break;
    case NSM_ERR_MS_PW_EXISTS:
      str = "MS PW already exists.";
      break;
    case NSM_ERR_SAME_VC_NAME:
      str = "Cannot stitch a VC to itself.";
      break;
    case NSM_ERR_VC_ID_NOT_FOUND:
      str = "VC Not found.";
      break;
    case NSM_ERR_VC_ALREADY_BOUND:
      str = "VC bound to another interface";
      break;
#endif /* HAVE_MS_PW */
    case NSM_ERR_INVALID_VC_NAME:
      str = "Invalid VC-Name Specified.";
      break;
    case NSM_ERR_PRIMARY_VC_INACTIVE:
      str = "Primary VC Inactive.";
      break;
    case NSM_ERR_SECONDARY_VC_INACTIVE:
      str = "Secondary VC inactive";
      break;
    case NSM_VC_ERR_NO_SIBLING:
      str = "No Secondary VC configured to primary VC.";
      break;
    case NSM_VC_ERR_SIBLING_MISMATCH:
      str = "Secondary VC mismatch.";
      break;
    case NSM_ERR_VC_SWITCHOVER_MODE_CONFLICT:
      str = "Could not SwitchOver VC if mode is revertive.";
      break;
    case NSM_ERR_SEC_VC_STATUS_DOWN:
      str = "Could not SwitchOver. Secondary VC operationally down.";
      break;
    case NSM_ERR_SEC_VC_ALREADY_INSTALLED:
      str = "Could not SwitchOver. Secondary VC already Installed.";
      break;
    case NSM_ERR_VC_MODE_NO_SECONDARY:
      str = "Switchover can be performed only in Primary/Secondary case.";
      break;
#endif /* HAVE_MPLS_VC */
#ifdef HAVE_PBR
    case NSM_API_SET_ERR_IF_MAP_ENABLED:
      str = "Interface associated with single policy"
            ", policy already configured on this interface";
      break;
    case NSM_API_SET_ERR_MAP_NOT_CONFIGURED:
      str = "Given policy not configured in system.";
      break;
    case NSM_API_SET_ERR_MAP_NOT_CONFIGURED_ON_IF:
      str = "Given policy not configured on this interface.";
      break;
#endif /* HAVE_PBR */
    default:
      return ret;
    }

  cli_out (cli, "%% %s\n", str);

  return CLI_ERROR;
}


void
nsm_debug_all_on (struct cli *cli)
{
  if (cli->mode == CONFIG_MODE)
    {
      CONF_DEBUG_ON (event, EVENT_ALL);
      CONF_DEBUG_ON (packet, PACKET_ALL);
      CONF_DEBUG_ON (kernel, KERNEL_ALL);
      CONF_DEBUG_ON (ha, HA_ALL);
#ifdef HAVE_BFD
      CONF_DEBUG_ON (bfd, BFD_ALL);
#endif /* HAVE_BFD */
    }
  TERM_DEBUG_ON (event, EVENT_ALL);
  TERM_DEBUG_ON (packet, PACKET_ALL);
  TERM_DEBUG_ON (kernel, KERNEL_ALL);
  TERM_DEBUG_ON (ha, HA_ALL);
#ifdef HAVE_BFD
  TERM_DEBUG_ON (bfd, BFD_ALL);
#endif /* HAVE_BFD */
}

void
nsm_debug_all_off (struct cli *cli)
{
  if (cli->mode == CONFIG_MODE)
    {
      CONF_DEBUG_OFF (event, EVENT_ALL);
      CONF_DEBUG_OFF (packet, PACKET_ALL);
      CONF_DEBUG_OFF (kernel, KERNEL_ALL);
      CONF_DEBUG_OFF (ha, HA_ALL);
#ifdef HAVE_BFD
      CONF_DEBUG_OFF (bfd, BFD_ALL);
#endif /* HAVE_BFD */
    }
  TERM_DEBUG_OFF (event, EVENT_ALL);
  TERM_DEBUG_OFF (packet, PACKET_ALL);
  TERM_DEBUG_OFF (kernel, KERNEL_ALL);
  TERM_DEBUG_OFF (ha, HA_ALL);
#ifdef HAVE_BFD
  TERM_DEBUG_OFF (bfd, BFD_ALL);
#endif /* HAVE_BFD */
}

int
nsm_dump_cpu_entry (struct cli *cli,
                   struct nsm_cpu_info_entry *stkEntry)
{
   cli_out(cli, "   %02x:%02x.%02x.%02x.%02x.%02x \n", stkEntry->mac_addr[0],
      stkEntry->mac_addr[1], stkEntry->mac_addr[2], stkEntry->mac_addr[3],
      stkEntry->mac_addr[4], stkEntry->mac_addr[5]);
   return 0;
}

int
nsm_dump_cpu_dump_entry (struct cli *cli,
                         int    cnt,
                         struct nsm_cpu_dump_entry *dumpEntry)
{
  struct nsm_stk_port_info *stkPort;
  int ii;

  cli_out(cli, " SYSTEM[%d]  KEY:  %02x:%02x.%02x.%02x.%02x.%02x \n", cnt,
      dumpEntry->mac_addr[0], dumpEntry->mac_addr[1], dumpEntry->mac_addr[2],
      dumpEntry->mac_addr[3], dumpEntry->mac_addr[4], dumpEntry->mac_addr[5]);

  cli_out (cli, "  Num of units: %d ", dumpEntry->num_units);
  cli_out (cli, " Master Pri: %d  Slot: %d\n", dumpEntry->master_prio, dumpEntry->slot_id);
  for (ii = 1; ((ii <= dumpEntry->num_stk_ports) && (ii <= NSM_STK_PORTS_ON_DEV)); ii++)
  {
      stkPort = &dumpEntry->stk[ii - 1];
      cli_out (cli, "  StackPort[%d]->  Unit: %d  Port: %d  Weight:%d \n", ii,
          stkPort->unit, stkPort->port, stkPort->weight);
      cli_out (cli, "  Flags: %x  Info: ", stkPort->flags);

      if (stkPort->flags)
          cli_out (cli, "%s%s%s%s%s%s\n",
              ((stkPort->flags & NSM_STK_CPUDB_SPF_NO_LINK)? "NoLink ": ""),
              ((stkPort->flags & NSM_STK_CPUDB_SPF_DUPLEX)? "Duplex ": ""),
              ((stkPort->flags & NSM_STK_CPUDB_SPF_INACTIVE)? "Inactive ": ""),
              ((stkPort->flags & NSM_STK_CPUDB_SPF_TX_RESOLVED)? "TxResolved ": ""),
              ((stkPort->flags & NSM_STK_CPUDB_SPF_RX_RESOLVED) ? "RxResolved ": ""),
              ((stkPort->flags & NSM_STK_CPUDB_SPF_CUT_PORT )? "CutPort": ""));

      cli_out(cli, "\n");
  }

  return 0;
}

#ifdef HAVE_BFD
CLI (debug_nsm_bfd,
     debug_nsm_bfd_cmd,
     "debug nsm bfd",
     CLI_DEBUG_STR,
     CLI_NSM_STR,
     CLI_BFD_CMD_STR)
{
  struct nsm_master *nm = cli->vr->proto;

  DEBUG_ON (cli, bfd, BFD, "NSM BFD");
  nsm_bfd_debug_set (nm);

  return CLI_SUCCESS;
}

CLI (no_debug_nsm_bfd,
     no_debug_nsm_bfd_cmd,
     "no debug nsm bfd",
     CLI_NO_STR,
     CLI_DEBUG_STR,
     CLI_NSM_STR,
     CLI_BFD_CMD_STR)
{
  struct nsm_master *nm = cli->vr->proto;

  DEBUG_OFF (cli, bfd, BFD, "NSM BFD");
  nsm_bfd_debug_unset (nm);

  return CLI_SUCCESS;
}

ALI (no_debug_nsm_bfd,
     undebug_nsm_bfd_cmd,
     "undebug nsm bfd",
     CLI_UNDEBUG_STR,
     CLI_NSM_STR,
     CLI_BFD_CMD_STR);
#endif /* HAVE_BFD */

CLI (debug_nsm,
     debug_nsm_cmd,
     "debug nsm (all|)",
     CLI_DEBUG_STR,
     CLI_NSM_STR,
     "Enable all debugging")
{
  nsm_debug_all_on (cli);
  if (cli->mode != CONFIG_MODE)
    cli_out (cli, "All possible debugging has been turned on\n");

  return CLI_SUCCESS;
}

CLI (no_debug_nsm,
     no_debug_nsm_cmd,
     "no debug nsm (all|)",
     CLI_NO_STR,
     CLI_DEBUG_STR,
     CLI_NSM_STR,
     CLI_UNDEBUG_ALL_STR)
{
  nsm_debug_all_off (cli);
  if (cli->mode != CONFIG_MODE)
    cli_out (cli, "All possible debugging has been turned off\n");

  return CLI_SUCCESS;
}

ALI (no_debug_nsm,
     undebug_nsm_cmd,
     "undebug nsm (all|)",
     CLI_UNDEBUG_STR,
     CLI_NSM_STR,
     "Enable all debugging");

ALI (no_debug_nsm,
     no_debug_nsm_all_cmd,
     "no debug all nsm",
     CLI_NO_STR,
     CLI_DEBUG_STR,
     CLI_UNDEBUG_ALL_STR,
     CLI_NSM_STR);

ALI (no_debug_nsm,
     undebug_nsm_all_cmd,
     "undebug all nsm",
     CLI_UNDEBUG_STR,
     CLI_UNDEBUG_ALL_STR,
     CLI_NSM_STR);

CLI (no_debug_all_nsm,
     no_debug_all_nsm_cmd,
     "no debug all",
     CLI_NO_STR,
     CLI_DEBUG_STR,
     CLI_UNDEBUG_ALL_STR)
{
  s_int32_t ret = CLI_SUCCESS;

  nsm_debug_all_off (cli);

#ifdef HAVE_MCAST_IPV4
  ret = igmp_debug_all_off(cli, argc, argv);
#endif /*HAVE_MCAST_IPV4 */

#ifdef HAVE_MCAST_IPV6
  ret = mld_debug_all_off(cli, argc, argv);
#endif /*HAVE_MCAST_IPV6 */

#ifdef HAVE_VRRP
  IF_NSM_CAP_HAVE_VRRP
    {
      vrrp_debug_all_off (cli);
    }
#endif /* HAVE_VRRP */
  if(ret < 0)
    return CLI_ERROR;
  else
    return CLI_SUCCESS;
}

ALI (no_debug_all_nsm,
     undebug_all_nsm_cmd,
     "undebug all",
     CLI_UNDEBUG_STR,
     CLI_UNDEBUG_ALL_STR);

CLI (debug_nsm_events,
     debug_nsm_events_cmd,
     "debug nsm events",
     CLI_DEBUG_STR,
     CLI_NSM_STR,
     "NSM events")
{
  DEBUG_ON (cli, event, EVENT, "NSM event");

  return CLI_SUCCESS;
}

CLI (no_debug_nsm_events,
     no_debug_nsm_events_cmd,
     "no debug nsm events",
     CLI_NO_STR,
     CLI_DEBUG_STR,
     CLI_NSM_STR,
     "NSM events")
{
  DEBUG_OFF (cli, event, EVENT, "NSM events");

  return CLI_SUCCESS;
}

ALI (no_debug_nsm_events,
     undebug_nsm_events_cmd,
     "undebug nsm events",
     CLI_UNDEBUG_STR,
     CLI_NSM_STR,
     "NSM events");

CLI (debug_nsm_packet,
     debug_nsm_packet_cmd,
     "debug nsm packet (recv|send|) (detail|)",
     CLI_DEBUG_STR,
     CLI_NSM_STR,
     "NSM packets",
     "NSM receive packets",
     "NSM send packets",
     "Detailed information display")
{
  u_int32_t flags = 0;

  SET_FLAG (flags, NSM_DEBUG_PACKET);
  if (argc == 0)
    {
      SET_FLAG (flags, NSM_DEBUG_SEND);
      SET_FLAG (flags, NSM_DEBUG_RECV);
    }
  else if (argc >= 1)
    {
      if (pal_strncmp (argv[0], "s", 1) == 0)
        SET_FLAG (flags, NSM_DEBUG_SEND);
      else if (pal_strncmp (argv[0], "r", 1) == 0)
        SET_FLAG (flags, NSM_DEBUG_RECV);
      else if (pal_strncmp (argv[0], "d", 1) == 0)
        {
          SET_FLAG (flags, NSM_DEBUG_SEND);
          SET_FLAG (flags, NSM_DEBUG_RECV);
          SET_FLAG (flags, NSM_DEBUG_DETAIL);
        }

      if (argc == 2)
        if (pal_strncmp (argv[1], "d", 1) == 0)
          SET_FLAG (flags, NSM_DEBUG_DETAIL);
    }

  DEBUG_PACKET_ON (cli, flags);

  return CLI_SUCCESS;
}

CLI (no_debug_nsm_packet,
     no_debug_nsm_packet_cmd,
     "no debug nsm packet (recv|send|) (detail|)",
     CLI_NO_STR,
     CLI_DEBUG_STR,
     CLI_NSM_STR,
     "NSM packets",
     "NSM receive packets",
     "NSM send packets",
     "Detailed information display")
{
  u_int32_t flags = 0;

  if (argc == 0)
    SET_FLAG (flags, NSM_DEBUG_PACKET_ALL);
  else if (argc >= 1)
    {
      if (pal_strncmp (argv[0], "s", 1) == 0)
        {
          if (NSM_DEBUG (packet, RECV))
            SET_FLAG (flags, NSM_DEBUG_SEND);
          else
            SET_FLAG (flags, NSM_DEBUG_PACKET_ALL);
        }
      else if (pal_strncmp (argv[0], "r", 1) == 0)
        {
          if (NSM_DEBUG (packet, SEND))
            SET_FLAG (flags, NSM_DEBUG_RECV);
          else
            SET_FLAG (flags, NSM_DEBUG_PACKET_ALL);
        }
      else if (pal_strncmp (argv[0], "d", 1) == 0)
        SET_FLAG (flags, NSM_DEBUG_DETAIL);

      if (argc == 2)
        if (pal_strncmp (argv[1], "d", 1) == 0)
          SET_FLAG (flags, NSM_DEBUG_DETAIL);
    }

  DEBUG_PACKET_OFF (cli, flags);

  return CLI_SUCCESS;
}

ALI (no_debug_nsm_packet,
     undebug_nsm_packet_cmd,
     "undebug nsm packet (recv|send|) (detail|)",
     CLI_UNDEBUG_STR,
     CLI_NSM_STR,
     "NSM packets",
     "NSM receive packets",
     "NSM send packets",
     "Detailed information display");

CLI (debug_nsm_kernel,
     debug_nsm_kernel_cmd,
     "debug nsm kernel",
     CLI_DEBUG_STR,
     CLI_NSM_STR,
     "NSM kernel")
{
  DEBUG_ON (cli, kernel, KERNEL, "NSM kernel");

  return CLI_SUCCESS;
}

CLI (no_debug_nsm_kernel,
     no_debug_nsm_kernel_cmd,
     "no debug nsm kernel",
     CLI_NO_STR,
     CLI_DEBUG_STR,
     CLI_NSM_STR,
     "NSM kernel")
{
  DEBUG_OFF (cli, kernel, KERNEL, "NSM kernel");

  return CLI_SUCCESS;
}

ALI (no_debug_nsm_kernel,
     undebug_nsm_kernel_cmd,
     "undebug nsm kernel",
     CLI_UNDEBUG_STR,
     CLI_NSM_STR,
     "NSM kernel");

CLI (debug_nsm_ha,
     debug_nsm_ha_cmd,
     "debug nsm ha",
     CLI_DEBUG_STR,
     CLI_NSM_STR,
     "NSM High Availability")
{
  DEBUG_ON (cli, ha, HA, "NSM HA");

  return CLI_SUCCESS;
}

CLI (no_debug_nsm_ha,
     no_debug_nsm_ha_cmd,
     "no debug nsm ha",
     CLI_NO_STR,
     CLI_DEBUG_STR,
     CLI_NSM_STR,
     "NSM High Availability")
{
  DEBUG_OFF (cli, ha, HA, "NSM HA");

  return CLI_SUCCESS;
}

ALI (no_debug_nsm_ha,
     undebug_nsm_ha_cmd,
     "undebug nsm ha",
     CLI_UNDEBUG_STR,
     CLI_NSM_STR,
     "NSM High Availability");

CLI (debug_nsm_ha_all,
     debug_nsm_ha_all_cmd,
     "debug nsm ha all",
     CLI_DEBUG_STR,
     CLI_NSM_STR,
     "NSM High Availability",
     "All NSM High Availability events")
{
  DEBUG_ON (cli, ha, HA_ALL, "All NSM HA");

  return CLI_SUCCESS;
}

CLI (no_debug_nsm_ha_all,
     no_debug_nsm_ha_all_cmd,
     "no debug nsm ha all",
     CLI_NO_STR,
     CLI_DEBUG_STR,
     CLI_NSM_STR,
     "NSM High Availability",
     "All NSM High Availability events")
{
  DEBUG_OFF (cli, ha, HA_ALL, "All NSM HA");

  return CLI_SUCCESS;
}

ALI (no_debug_nsm_ha_all,
     undebug_nsm_ha_all_cmd,
     "undebug nsm ha all",
     CLI_UNDEBUG_STR,
     CLI_NSM_STR,
     "NSM High Availability",
     "All NSM High Availability events");

#ifdef HAVE_WMI
/* WMI Debug command. */
CLI (debug_nsm_wmi,
     debug_nsm_wmi_cmd,
     "debug nsm wmi",
     CLI_DEBUG_STR,
     CLI_NSM_STR,
     WMI_STR)
{
  WMI_NSM_DEBUG_ON (EVENT);
  WMI_NSM_DEBUG_ON (RECV);
  WMI_NSM_DEBUG_ON (SEND);
  return CLI_SUCCESS;
}

void
nsm_wmi_debug_init (struct lib_globals *zg)
{
  struct cli_tree *ctree = zg->ctree;

  /* Install debug commands. */
  cli_install_gen (ctree, EXEC_MODE, PRIVILEGE_MAX, 0,
                   &debug_nsm_wmi_cmd);
}
#endif /* HAVE_WMI */

int
nsm_debug_config_write (struct cli *cli)
{
  int write = 0;

  if (CONF_NSM_DEBUG (event, EVENT))
    {
      cli_out (cli, "debug nsm events\n");
      write++;
    }
  if (CONF_NSM_DEBUG (packet, PACKET))
    {
      if (CONF_NSM_DEBUG (packet, SEND) && CONF_NSM_DEBUG (packet, RECV))
        {
          cli_out (cli, "debug nsm packet%s\n",
                   CONF_NSM_DEBUG (packet, DETAIL) ? " detail" : "");
          write++;
        }
      else
        {
          if (CONF_NSM_DEBUG (packet, SEND))
            cli_out (cli, "debug nsm packet send%s\n",
                     CONF_NSM_DEBUG (packet, DETAIL) ? " detail" : "");
          else if (CONF_NSM_DEBUG (packet, RECV))
            cli_out (cli, "debug nsm packet recv%s\n",
                     CONF_NSM_DEBUG (packet, DETAIL) ? " detail" : "");
          write++;
        }
    }
  if (CONF_NSM_DEBUG (kernel, KERNEL))
    {
      cli_out (cli, "debug nsm kernel\n");
      write++;
    }
  if (CONF_NSM_DEBUG (ha, HA_ALL))
    {
      cli_out (cli, "debug nsm ha all\n");
      write++;
    }
  else if (CONF_NSM_DEBUG (ha, HA))
    {
      cli_out (cli, "debug nsm ha\n");
      write++;
    }

#ifdef HAVE_BFD
  if (CONF_NSM_DEBUG (bfd, BFD))
    {
      cli_out (cli, "debug nsm bfd\n");
      write++;
    }
#endif /* HAVE_BFD */

#ifdef HAVE_VRRP
  if (NSM_CAP_HAVE_VRRP)
    write += vrrp_debug_config_write (cli);
#endif /* HAVE_VRRP */

#if defined HAVE_MCAST_IPV4 || defined HAVE_IGMP_SNOOP
  write += igmp_debug_config_write (cli);
#endif /* ! HAVE_MCAST_IPV4 && HAVE_IGMP_SNOOP */

#if defined HAVE_MCAST_IPV6 || defined HAVE_MLD_SNOOP
  write += mld_debug_config_write (cli);
#endif /* ! HAVE_MCAST_IPV6 && HAVE_MLD_SNOOP */

#ifdef HAVE_MCAST_IPV4
  /* NSM MCAST debug-mode config-write function */
  write += nsm_mcast_debug_config_write (cli);
#endif /* HAVE_MCAST_IPV4 */

#ifdef HAVE_MCAST_IPV6
  /* NSM MCAST debug-mode config-write function */
  write += nsm_mcast6_debug_config_write (cli);
#endif /* HAVE_MCAST_IPV6 */

  return write;
}

/* NSM debug commands.  */
CLI (show_debugging_nsm,
     show_debugging_nsm_cmd,
     "show debugging nsm",
     CLI_SHOW_STR,
     CLI_DEBUG_STR,
     CLI_NSM_STR)
{
  cli_out (cli, "NSM debugging status:\n");

  if (NSM_DEBUG (event, EVENT))
    cli_out (cli, "  NSM event debugging is on\n");

  if (NSM_DEBUG (packet, PACKET))
    {
      if (NSM_DEBUG (packet, SEND) && NSM_DEBUG (packet, RECV))
        {
          cli_out (cli, "  NSM packet%s debugging is on\n",
                   NSM_DEBUG (packet, DETAIL) ? " detail" : "");
        }
      else
        {
          if (NSM_DEBUG (packet, SEND))
            cli_out (cli, "  NSM packet send%s debugging is on\n",
                     NSM_DEBUG (packet, DETAIL) ? " detail" : "");
          else
            cli_out (cli, "  NSM packet receive%s debugging is on\n",
                     NSM_DEBUG (packet, DETAIL) ? " detail" : "");
        }
    }

  if (NSM_DEBUG (kernel, KERNEL))
    cli_out (cli, "  NSM kernel debugging is on\n");

  if (NSM_DEBUG (ha, HA_ALL))
    cli_out (cli, "  NSM HA all debugging is on\n");
  else if (NSM_DEBUG (ha, HA))
    cli_out (cli, "  NSM HA debugging is on\n");

#ifdef HAVE_BFD
  if (NSM_DEBUG (bfd, BFD))
    cli_out (cli, "  NSM BFD debugging is on\n");
#endif /* HAVE_BFD */

  cli_out (cli, "\n");

  return CLI_SUCCESS;
}

void
nsm_cli_init_debug (struct cli_tree *ctree)
{
  cli_install_config (ctree, DEBUG_MODE, nsm_debug_config_write);

  cli_install_gen (ctree, EXEC_MODE, PRIVILEGE_NORMAL, 0,
                   &show_debugging_nsm_cmd);

  /* "debug nsm" commands. */
  cli_install_gen (ctree, EXEC_MODE, PRIVILEGE_NORMAL, 0,
                   &debug_nsm_cmd);
  cli_install_gen (ctree, EXEC_MODE, PRIVILEGE_NORMAL, 0,
                   &no_debug_nsm_cmd);
  cli_install_gen (ctree, EXEC_MODE, PRIVILEGE_NORMAL, 0,
                   &undebug_nsm_cmd);
  cli_install_gen (ctree, CONFIG_MODE, PRIVILEGE_NORMAL, 0,
                   &debug_nsm_cmd);
  cli_install_gen (ctree, CONFIG_MODE, PRIVILEGE_NORMAL, 0,
                   &no_debug_nsm_cmd);

  /* "no debug all nsm" commands. */
  cli_install_gen (ctree, EXEC_MODE, PRIVILEGE_NORMAL, 0,
                   &no_debug_nsm_all_cmd);
  cli_install_gen (ctree, EXEC_MODE, PRIVILEGE_NORMAL, 0,
                   &undebug_nsm_all_cmd);
  cli_install_gen (ctree, CONFIG_MODE, PRIVILEGE_NORMAL, 0,
                   &no_debug_nsm_all_cmd);

  /* "no debug all" commands. */
  cli_install_gen (ctree, EXEC_MODE, PRIVILEGE_NORMAL, 0,
                   &no_debug_all_nsm_cmd);
  cli_install_gen (ctree, EXEC_MODE, PRIVILEGE_NORMAL, 0,
                   &undebug_all_nsm_cmd);
  cli_install_gen (ctree, CONFIG_MODE, PRIVILEGE_NORMAL, 0,
                   &no_debug_all_nsm_cmd);

#ifdef HAVE_BFD
  /* "debug nsm bfd commands. "*/
  cli_install_gen (ctree, EXEC_MODE, PRIVILEGE_NORMAL, 0,
                   &debug_nsm_bfd_cmd);
  cli_install_gen (ctree, EXEC_MODE, PRIVILEGE_NORMAL, 0,
                   &no_debug_nsm_bfd_cmd);
  cli_install_gen (ctree, EXEC_MODE, PRIVILEGE_NORMAL, 0,
                   &undebug_nsm_bfd_cmd);
  cli_install_gen (ctree, CONFIG_MODE, PRIVILEGE_NORMAL, 0,
                   &debug_nsm_bfd_cmd);
  cli_install_gen (ctree, CONFIG_MODE, PRIVILEGE_NORMAL, 0,
                   &no_debug_nsm_bfd_cmd);
#endif /* HAVE_BFD */

  /* "debug nsm events" commands. */
  cli_install_gen (ctree, EXEC_MODE, PRIVILEGE_NORMAL, 0,
                   &debug_nsm_events_cmd);
  cli_install_gen (ctree, EXEC_MODE, PRIVILEGE_NORMAL, 0,
                   &no_debug_nsm_events_cmd);
  cli_install_gen (ctree, EXEC_MODE, PRIVILEGE_NORMAL, 0,
                   &undebug_nsm_events_cmd);
  cli_install_gen (ctree, CONFIG_MODE, PRIVILEGE_NORMAL, 0,
                   &debug_nsm_events_cmd);
  cli_install_gen (ctree, CONFIG_MODE, PRIVILEGE_NORMAL, 0,
                   &no_debug_nsm_events_cmd);

  /* "debug nsm packet" commands. */
  cli_install_gen (ctree, EXEC_MODE, PRIVILEGE_NORMAL, 0,
                   &debug_nsm_packet_cmd);
  cli_install_gen (ctree, EXEC_MODE, PRIVILEGE_NORMAL, 0,
                   &no_debug_nsm_packet_cmd);
  cli_install_gen (ctree, EXEC_MODE, PRIVILEGE_NORMAL, 0,
                   &undebug_nsm_packet_cmd);
  cli_install_gen (ctree, CONFIG_MODE, PRIVILEGE_NORMAL, 0,
                   &debug_nsm_packet_cmd);
  cli_install_gen (ctree, CONFIG_MODE, PRIVILEGE_NORMAL, 0,
                   &no_debug_nsm_packet_cmd);

  /* "debug nsm kernel" commands. */
  cli_install_gen (ctree, EXEC_MODE, PRIVILEGE_NORMAL, 0,
                   &debug_nsm_kernel_cmd);
  cli_install_gen (ctree, EXEC_MODE, PRIVILEGE_NORMAL, 0,
                   &no_debug_nsm_kernel_cmd);
  cli_install_gen (ctree, EXEC_MODE, PRIVILEGE_NORMAL, 0,
                   &undebug_nsm_kernel_cmd);
  cli_install_gen (ctree, CONFIG_MODE, PRIVILEGE_NORMAL, 0,
                   &debug_nsm_kernel_cmd);
  cli_install_gen (ctree, CONFIG_MODE, PRIVILEGE_NORMAL, 0,
                   &no_debug_nsm_kernel_cmd);

  /* "debug nsm ha" commands. */
  cli_install_gen (ctree, EXEC_MODE, PRIVILEGE_NORMAL, 0,
                   &debug_nsm_ha_cmd);
  cli_install_gen (ctree, EXEC_MODE, PRIVILEGE_NORMAL, 0,
                   &no_debug_nsm_ha_cmd);
  cli_install_gen (ctree, EXEC_MODE, PRIVILEGE_NORMAL, 0,
                   &undebug_nsm_ha_cmd);
  cli_install_gen (ctree, CONFIG_MODE, PRIVILEGE_NORMAL, 0,
                   &debug_nsm_ha_cmd);
  cli_install_gen (ctree, CONFIG_MODE, PRIVILEGE_NORMAL, 0,
                   &no_debug_nsm_ha_cmd);

  /* "debug nsm ha all" commands. */
  cli_install_gen (ctree, EXEC_MODE, PRIVILEGE_NORMAL, 0,
                   &debug_nsm_ha_all_cmd);
  cli_install_gen (ctree, EXEC_MODE, PRIVILEGE_NORMAL, 0,
                   &no_debug_nsm_ha_all_cmd);
  cli_install_gen (ctree, EXEC_MODE, PRIVILEGE_NORMAL, 0,
                   &undebug_nsm_ha_all_cmd);
  cli_install_gen (ctree, CONFIG_MODE, PRIVILEGE_NORMAL, 0,
                   &debug_nsm_ha_all_cmd);
  cli_install_gen (ctree, CONFIG_MODE, PRIVILEGE_NORMAL, 0,
                   &no_debug_nsm_ha_all_cmd);
}


CLI (no_nsm_if_shutdown,
     no_nsm_if_shutdown_cmd,
     "no shutdown",
     CLI_NO_STR,
     "Shutdown the selected interface")
{
  int ret = 0;
  struct interface *ifp = cli->index;

#ifdef HAVE_LACPD
  NSM_INTERFACE_CHECK_AGG_MEM_PROPERTY(cli, ifp);
#endif /* HAVE_LACPD */
  if (!CHECK_FLAG (ifp->flags, IFF_UP))
  {
    ret = nsm_if_flag_up_set (cli->vr->id, ifp->name, PAL_TRUE);
  }

  return nsm_cli_return (cli, ret);
}

CLI (nsm_if_shutdown,
     nsm_if_shutdown_cmd,
     "shutdown",
     "Shutdown the selected interface")
{
  int ret = 0;
  struct interface *ifp = cli->index;

#ifdef HAVE_LACPD
  NSM_INTERFACE_CHECK_AGG_MEM_PROPERTY(cli, ifp);
#endif /* HAVE_LACPD */
  if (CHECK_FLAG (ifp->flags, IFF_UP))
  {
    ret = nsm_if_flag_up_unset (cli->vr->id, ifp->name, PAL_TRUE);
  }

  return nsm_cli_return (cli, ret);
}

#ifdef HAVE_L3
CLI (nsm_if_multicast,
     nsm_if_multicast_cmd,
     "multicast",
     "Set multicast flag to interface")
{
  int ret;
  struct interface *ifp = cli->index;

#ifdef HAVE_LACPD
  NSM_INTERFACE_CHECK_AGG_MEM_PROPERTY(cli, ifp);
#endif /* HAVE_LACPD */
  ret = nsm_if_flag_multicast_set (cli->vr->id, ifp->name);

  return nsm_cli_return (cli, ret);
}

CLI (no_nsm_if_multicast,
     no_nsm_if_multicast_cmd,
     "no multicast",
     CLI_NO_STR,
     "Unset multicast flag to interface")
{
  int ret;
  struct interface *ifp = cli->index;

#ifdef HAVE_LACPD
  NSM_INTERFACE_CHECK_AGG_MEM_PROPERTY(cli, ifp);
#endif /* HAVE_LACPD */
  ret = nsm_if_flag_multicast_unset (cli->vr->id, ifp->name);

  return nsm_cli_return (cli, ret);
}

/* The maximum MTU size is GE Jumbo Frame Size (9216) -
 * 4 bytes for VLAN tag - 4 bytes CRC
 */
CLI (nsm_if_mtu,
     nsm_if_mtu_cmd,
     "mtu <68-9216>",
     "Set mtu value to interface",
     "MTU in bytes")
{
  int ret;
  int mtu;
  struct interface *ifp = cli->index;

#ifdef HAVE_LACPD
  NSM_INTERFACE_CHECK_AGG_MEM_PROPERTY(cli, ifp);
#endif /* HAVE_LACPD */

  /* MTU setting is not allowed on VLAN interfaces */
  if (if_is_vlan (ifp))
    {
      cli_out (cli, "%% MTU change not allowed on VLAN interface.\n");
      return CLI_ERROR;
    }

  CLI_GET_INTEGER_RANGE ("MTU", mtu, argv[0], 68, 9216);

  ret = nsm_if_mtu_set (cli->vr->id, ifp->name, mtu);

  return nsm_cli_return (cli, ret);
}

CLI (no_nsm_if_mtu,
     no_nsm_if_mtu_cmd,
     "no mtu",
     CLI_NO_STR,
     "Set default mtu value to interface")
{
  int ret;
  struct interface *ifp = cli->index;

#ifdef HAVE_LACPD
  NSM_INTERFACE_CHECK_AGG_MEM_PROPERTY(cli, ifp);
#endif /* HAVE_LACPD */

  /* MTU setting is not allowed on VLAN interfaces */
  if (if_is_vlan (ifp))
    {
      cli_out (cli, "%% MTU change not allowed on VLAN interface.\n");
      return CLI_ERROR;
    }

  ret = nsm_if_mtu_unset (cli->vr->id, ifp->name);

  return nsm_cli_return (cli, ret);
}
#endif /* HAVE_L3 */

CLI (nsm_if_duplex,
     nsm_if_duplex_cmd,
     "duplex (half|full|auto)",
     "Set duplex to interface",
     "set half-duplex",
     "set full-duplex",
     "set auto-negotiate")
{
  int ret;
  int duplex = NSM_IF_AUTO_NEGO;
  struct interface *ifp = cli->index;

  if ( !pal_strncmp (argv[0], "h", 1) )
    duplex = NSM_IF_HALF_DUPLEX;
  if ( !pal_strncmp (argv[0], "f", 1) )
    duplex = NSM_IF_FULL_DUPLEX;
  if ( !pal_strncmp (argv[0], "a", 1) )
    duplex = NSM_IF_AUTO_NEGO;

  if (duplex == NSM_IF_AUTO_NEGO)
    ifp->autonego = NSM_IF_AUTONEGO_ENABLE;
  else
    ifp->autonego = NSM_IF_AUTONEGO_DISABLE;

  ret = nsm_if_duplex_set (cli->vr->id, ifp->name, duplex);

  return nsm_cli_return (cli, ret);
}

CLI (no_nsm_if_duplex,
     no_nsm_if_duplex_cmd,
     "no duplex",
     CLI_NO_STR,
     "Set default duplex(auto-negotiate) to interface")
{
  int ret;
  struct interface *ifp = cli->index;

  ret = nsm_if_duplex_unset (cli->vr->id, ifp->name);

  return nsm_cli_return (cli, ret);
}


CLI (nsm_access_group_any,
     nsm_access_group_any_cmd,
     "ip-access-group (<1-199>|<1300-2699>|WORD) (in|out)",
     "Setting IP access group",
     "IP access list (Standard or Extended)",
     "IP Expanded access list (Standard or Extended)",
     "IP PacOS access-list name",
     "inbound packets",
     "outbound packets")
{
  int ret = 0;
  struct interface *ifp = cli->index;
  ret = nsm_access_group_set (cli->vr, argv[0], argv[1], ifp->name, PAL_TRUE);

  return nsm_cli_return (cli, ret);
}

CLI (no_nsm_access_group_any,
     no_nsm_access_group_any_cmd,
     "no ip-access-group (<1-199>|<1300-2699>|WORD) (in|out)",
     CLI_NO_STR,
     "IP access group",
     "IP access list (Standard or Extended)",
     "IP Expanded access list (Standard or Extended)",
     "IP PacOS access-list name",
     "inbound packets",
     "outbound packets")
{
  int ret =0;
  struct interface *ifp = cli->index;
  ret = nsm_access_group_set (cli->vr, argv[0], argv[1], ifp->name, PAL_FALSE);

  return nsm_cli_return (cli,ret);
}

void
nsm_cli_init_interface (struct cli_tree *ctree)
{

#ifdef HAVE_L3
  /* "multicast" CLI.  */
  cli_install_gen (ctree, INTERFACE_MODE, PRIVILEGE_NORMAL, 0,
                   &nsm_if_multicast_cmd);
  cli_install_gen (ctree, INTERFACE_MODE, PRIVILEGE_NORMAL, 0,
                   &no_nsm_if_multicast_cmd);

  /* "mtu" CLI.  */
  cli_install_gen (ctree, INTERFACE_MODE, PRIVILEGE_NORMAL, 0,
                   &nsm_if_mtu_cmd);
  cli_install_gen (ctree, INTERFACE_MODE, PRIVILEGE_NORMAL, 0,
                   &no_nsm_if_mtu_cmd);
#endif /* HAVE_L3 */

  /* "ip access-group CLI */
  cli_install_gen (ctree, INTERFACE_MODE, PRIVILEGE_NORMAL, 0,
                   &nsm_access_group_any_cmd);
  cli_install_gen (ctree, INTERFACE_MODE, PRIVILEGE_NORMAL, 0,
                   &no_nsm_access_group_any_cmd);

}

void
nsm_cli_init_all_interface (struct cli_tree *ctree)
{

  /* "shutdown" CLI.  */
  cli_install_gen (ctree, INTERFACE_MODE, PRIVILEGE_NORMAL, 0,
                   &nsm_if_shutdown_cmd);
  cli_install_gen (ctree, INTERFACE_MODE, PRIVILEGE_NORMAL, 0,
                   &no_nsm_if_shutdown_cmd);

 /* "duplex" CLI. */
  cli_install_gen (ctree, INTERFACE_MODE, PRIVILEGE_NORMAL, 0,
                   &nsm_if_duplex_cmd);
  cli_install_gen (ctree, INTERFACE_MODE, PRIVILEGE_NORMAL, 0,
                   &no_nsm_if_duplex_cmd);

}


#ifdef HAVE_L3
CLI (nsm_fib_retain,
     nsm_fib_retain_cmd,
     "fib retain (forever|time <1-65535>|)",
     CLI_FIB_STR,
     "Retain FIB during NSM restart",
     "Retain FIB forever",
     "Retain FIB for a specific time after NSM restarts",
     "Seconds")
{
  int retain_time;

  if (argc > 1)
    CLI_GET_INTEGER_RANGE ("retain time", retain_time, argv[1], 1, 65535);
  else if (argc > 0)
    retain_time = NSM_FIB_RETAIN_TIME_FOREVER;
  else
    retain_time = NSM_FIB_RETAIN_TIME_DEFAULT;

  nsm_fib_retain_set (cli->vr->id, retain_time);

  return CLI_SUCCESS;
}

CLI (no_nsm_fib_retain,
     no_nsm_fib_retain_cmd,
     "no fib retain (forever|time <1-65535>|)",
     CLI_NO_STR,
     CLI_FIB_STR,
     "Retain FIB when NSM restarting",
     "Retain FIB forever",
     "Retain FIB for specific time after NSM restarts",
     "Retain time value")
{
  nsm_fib_retain_unset (cli->vr->id);

  return CLI_SUCCESS;
}

CLI (clear_ip_route_kernel,
     clear_ip_route_kernel_cmd,
     "clear ip route kernel",
     CLI_CLEAR_STR,
     CLI_IP_STR,
     CLI_ROUTE_STR,
     "Stale kernel route")
{
  int ret;

  ret = nsm_ipv4_route_stale_clear (cli->vr->id);

  return nsm_cli_return (cli, ret);
}

ALI (clear_ip_route_kernel,
     clear_ip_kernel_route_cmd,
     "clear ip kernel route",
     CLI_CLEAR_STR,
     CLI_IP_STR,
     "Stale kernel route",
     CLI_ROUTE_STR);

#ifdef HAVE_IPV6
CLI (clear_ipv6_route_kernel,
     clear_ipv6_route_kernel_cmd,
     "clear ipv6 route kernel",
     CLI_CLEAR_STR,
     CLI_IPV6_STR,
     CLI_ROUTE_STR,
     "Stale kernel route")
{
  int ret;

  ret = nsm_ipv6_route_stale_clear (cli->vr->id);

  return nsm_cli_return (cli, ret);
}
#endif /* HAVE_IPV6 */

int
nsm_multipath_num_write (struct cli *cli)
{
  struct nsm_master *nm = cli->vr->proto;
  int write = 0;

  if (nm->multipath_num != DEFAULT_MULTIPATH_NUM)
    {
      cli_out (cli, "maximum-paths %d\n", nm->multipath_num);
      write = 1;
    }

  return write;
}

/* NSM MULTIPATH-NUM configure func. */
int
nsm_multipath_num_func (struct cli *cli, int set, char *num_str)
{
  struct nsm_master *nm = cli->vr->proto;
  int multipath = 0;

  /* Check argument valid or not. */
  if (num_str)
    CLI_GET_INTEGER_RANGE ("multipath number", multipath, num_str, 1, 64);

  /* If multipath number is unchanged through configuration,
     just return CLI_SUCCESS. */
  if ((set && nm->multipath_num == multipath)
      || (! set && nm->multipath_num == DEFAULT_MULTIPATH_NUM))
    return CLI_SUCCESS;

  /* Set the correct information. */
  SET_FLAG (nm->flags, NSM_MULTIPATH_REFRESH);
  nm->multipath_num = (set == 0) ? DEFAULT_MULTIPATH_NUM : multipath;

  /* XXX: Make sure the change in multipath number is checkpoionted
   * before any RIB modifications get checkpointed in
   * nsm_rib_multipath_process(). This is required because multipath
   * number affects the number of nexthops added to FIB on the Standby.
   */
#ifdef HAVE_HA
  nsm_cal_modify_nsm_master (nm);
#endif /* HAVE_HA */

  /* Refresh rib routes into FIB. */
  nsm_rib_multipath_process (nm);

  /* Reset nm's status. */
  UNSET_FLAG (nm->flags, NSM_MULTIPATH_REFRESH);

  return CLI_SUCCESS;
}

#ifdef HAVE_MULTIPATH
/* MULTIPATH_NUM configuration. */
CLI (maximum_paths,
     maximum_paths_cmd,
     "maximum-paths <1-64>",
     "Set multipath numbers installed to FIB",
     "supported multipath numbers")
{
  /* Currently only support maximum 10 multipaths. */
  return nsm_multipath_num_func (cli, 1, argv[0]);
}

CLI (no_maximum_paths,
     no_maximum_paths_cmd,
     "no maximum-paths",
     CLI_NO_STR,
     "Set multipath number of route which can be installed into FIB")
{
  if (argc > 1)
    return nsm_multipath_num_func (cli, 0, argv[0]);

  return nsm_multipath_num_func (cli, 0, NULL);
}

ALI (no_maximum_paths,
     no_maximum_paths_num_cmd,
     "no maximum-paths <1-64>",
     CLI_NO_STR,
     "Set multipath number of route which can be installed into FIB",
     "Supported multipath numbers");
#endif /* HAVE_MULTIPATH */

int
nsm_max_static_routes_write (struct cli *cli)
{
  struct nsm_master *nm = cli->vr->proto;
  int write = 0;

  if (nm->max_static_routes != MAX_STATIC_ROUTE_DEFAULT)
    {
      cli_out (cli, "max-static-routes %lu\n", nm->max_static_routes);
      write = 1;
    }

  return write;
}
void
nsm_set_maximum_static_routes (struct nsm_master *nm, int num)
{
  nm->max_static_routes = num;

#ifdef HAVE_HA
  nsm_cal_modify_nsm_master (nm);
#endif /* HAVE_HA */
}

void
nsm_unset_maximum_static_routes (struct nsm_master *nm)
{
  nm->max_static_routes = MAX_STATIC_ROUTE_DEFAULT;

#ifdef HAVE_HA
  nsm_cal_modify_nsm_master (nm);
#endif /* HAVE_HA */
}

CLI (maximum_static_routes,
     maximum_static_routes_cmd,
     "max-static-routes <1-4294967294>",
     "Set maximum static routes number",
     "Allowed number of static routes")
{
  struct nsm_master *nm = cli->vr->proto;
  struct nsm_vrf *nv;
  u_int32_t routes;

  vrf_id_t vrf_id = VRF_ID_UNSPEC;

  CLI_GET_INTEGER_RANGE ("Routes", routes, argv[0], 1,
                          MAX_STATIC_ROUTE_DEFAULT);

  nv = nsm_vrf_lookup_by_id (nm, vrf_id);

  /* Sanity check */
  if (nv == NULL)
    return CLI_ERROR;

  if (nsm_static_ipv4_count (nv) > routes)
  {
    cli_out (cli, "%% Number of configured static routes exceeds"
             " max-static-routes limit.\n");
    return CLI_ERROR;
  }

  /* Set the max-static-routes limit */
  nsm_set_maximum_static_routes (nm, routes);

  return CLI_SUCCESS;
}

CLI (no_maximum_static_routes,
     no_maximum_static_routes_cmd,
     "no max-static-routes",
     CLI_NO_STR,
     "Set maximum static routes number")
{
  struct nsm_master *nm = cli->vr->proto;

  nsm_unset_maximum_static_routes (nm);

  return CLI_SUCCESS;
}
 int
 nsm_max_fib_routes_write (struct cli *cli)
 {
   struct nsm_master *nm = cli->vr->proto;
   int write = 0;

   if (nm->max_fib_routes != MAX_FIB_ROUTE_DEFAULT)
     {
       cli_out (cli, "max-fib-routes %lu\n", nm->max_fib_routes);
       write = 1;
     }

   return write;
 }

void
nsm_set_maximum_fib_routes (struct nsm_master *nm, int num)
{
  nm->max_fib_routes = num;

#ifdef HAVE_HA
  nsm_cal_modify_nsm_master (nm);
#endif /* HAVE_HA */
}

void
nsm_unset_maximum_fib_routes (struct nsm_master *nm)
{
  nm->max_fib_routes = MAX_FIB_ROUTE_DEFAULT;

#ifdef HAVE_HA
  nsm_cal_modify_nsm_master (nm);
#endif /* HAVE_HA */
}

 CLI (maximum_fib_routes,
      maximum_fib_routes_cmd,
      "max-fib-routes <1-4294967294>",
      "Set maximum fib routes number",
      "Allowed number of fib routes excluding Kernel, Connect and Static")
 {
   struct nsm_master *nm = cli->vr->proto;
   u_int32_t routes;

   CLI_GET_INTEGER_RANGE ("Routes", routes, argv[0], 1, MAX_FIB_ROUTE_DEFAULT);

   nsm_set_maximum_fib_routes (nm, routes);

   return CLI_SUCCESS;
 }

 CLI (no_maximum_fib_routes,
      no_maximum_fib_routes_cmd,
      "no max-fib-routes",
      CLI_NO_STR,
      "Set maximum fib routes number")
 {
   struct nsm_master *nm = cli->vr->proto;

   nsm_unset_maximum_fib_routes (nm);

   return CLI_SUCCESS;
 }

#define NSM_CLI_GATEWAY_INVALID         0
#define NSM_CLI_GATEWAY_IFNAME          1
#define NSM_CLI_GATEWAY_ADDRESS         2

/* Check if prefix is not network nor broadcast. */
int
nsm_cli_ipv4_addr_check (struct prefix_ipv4 *p, struct interface *ifp)
{
  struct pal_in4_addr addr;
  struct pal_in4_addr mask;

  /* ifp can be NULL */

  /* 32 bit address prefix is disallowed only on broadcast interfaces */
  if (ifp && if_is_broadcast (ifp) && p->prefixlen == IPV4_MAX_BITLEN)
    return 0;

  /* For 32-bit prefixes no need to check for network or broadcast addresses */
  if (p->prefixlen != IPV4_MAX_BITLEN)
    {
      masklen2ip (p->prefixlen, &mask);

      /* This is network. */
      addr.s_addr = p->prefix.s_addr & mask.s_addr;
      if (addr.s_addr == p->prefix.s_addr)
        return 0;

      /* This is broadcast. */
      addr.s_addr = p->prefix.s_addr | (~mask.s_addr);
      if (addr.s_addr == p->prefix.s_addr)
        return 0;
    }

  return 1;
}

/* When gateway is A.B.C.D format, gate is treated as nexthop
   address other case gate is treated as interface name. */
int
nsm_cli_ipv4_gateway_get (struct nsm_master *nm, char *str, char **ifname,
                          union nsm_nexthop *gate, vrf_id_t vrf_id)
{
  struct interface *ifp;
  int ret;

  ret = pal_inet_pton (AF_INET, str, &gate->ipv4);
  if (!ret)
    {
      if (IS_NULL_INTERFACE_STR (str))
        *ifname = NULL_INTERFACE;
      else
        {
          /* Check if it is valid ifname. */    /* XXX */
          ifp = if_lookup_by_name (&nm->vr->ifm, str);
          if (ifp == NULL || ifp->vrf->id != vrf_id)
            return NSM_CLI_GATEWAY_INVALID;

          *ifname = str;
        }
      return NSM_CLI_GATEWAY_IFNAME;
    }

  return NSM_CLI_GATEWAY_ADDRESS;
}

int
nsm_cli_ip_route_prefix (struct nsm_master *nm,
                         vrf_id_t vrf_id, struct prefix_ipv4 *p,
                         char *gate_str, int distance,
                         u_int32_t tag, char *desc)
{
  union nsm_nexthop gate;
  char *ifname = NULL;
  struct nsm_vrf *nv;
  int ret;
  struct prefix_ipv4 np, ptemp;

  /* Check IP address */
  if (IN_EXPERIMENTAL (pal_ntoh32 (p->prefix.s_addr)))
    return NSM_API_SET_ERR_INVALID_IPV4_ADDRESS;

  pal_mem_cpy(&ptemp, p, sizeof (struct prefix_ipv4));
  apply_mask_ipv4(&ptemp);
  if (! IPV4_ADDR_SAME(&p->prefix, &ptemp.prefix))
    return NSM_API_SET_ERR_INCONSISTENT_ADDRESS_MASK;

#ifdef HAVE_CRX
  apply_mask_ipv4 (p);
  if (crx_vip_exists ((struct prefix *)p))
    return CLI_SUCCESS;
#endif /* HAVE_CRX. */

  ret = nsm_cli_ipv4_gateway_get (nm, gate_str, &ifname, &gate, vrf_id);
  if (ret == NSM_CLI_GATEWAY_INVALID)
    return NSM_API_SET_ERR_MALFORMED_GATEWAY;

  nv = nsm_vrf_lookup_by_id (nm, vrf_id);
  if (! nv)
    return NSM_API_SET_ERR_VRF_NOT_EXIST;

  pal_mem_set (&np, 0, sizeof (struct prefix_ipv4));
  np.family = AF_INET;
  np.prefixlen = IPV4_MAX_PREFIXLEN;
  np.prefix = gate.ipv4;
  if (nsm_ipv4_connected_table_lookup (nv, (struct prefix *)&np))
    return NSM_API_SET_ERR_NEXTHOP_OWN_ADDR;

  if (nsm_static_ipv4_count (nv) >= nm->max_static_routes)
    return NSM_API_SET_ERR_MAX_STATIC_ROUTE_LIMIT;

  if (ret == NSM_CLI_GATEWAY_ADDRESS)
    ret = nsm_ipv4_route_set (nm, vrf_id, p, &gate, ifname,
                              distance, 0, ROUTE_TYPE_OTHER,
                              tag, desc);
  else
    ret = nsm_ipv4_route_set (nm, vrf_id, p, NULL, ifname,
                              distance, 0, ROUTE_TYPE_OTHER,
                              tag, desc);

  return ret;
}

int
nsm_cli_no_ip_route_prefix (struct nsm_master *nm,
                            vrf_id_t vrf_id, struct prefix_ipv4 *p,
                            char *gate_str, int distance, u_int32_t tag,
                            char *desc)
{
  union nsm_nexthop gate;
  char *ifname = NULL;
  int ret = NSM_API_SET_ERROR;

  /* Check IP address */
  if (IN_EXPERIMENTAL (pal_ntoh32 (p->prefix.s_addr)))
    return NSM_API_SET_ERR_INVALID_IPV4_ADDRESS;

#ifdef HAVE_CRX
  apply_mask_ipv4 (p);
  if (crx_vip_exists ((struct prefix *)p))
    return CLI_SUCCESS;
#endif /* HAVE_CRX. */

  ret = nsm_cli_ipv4_gateway_get (nm, gate_str, &ifname, &gate, vrf_id);
  if (ret == NSM_CLI_GATEWAY_INVALID)
    return NSM_API_SET_ERR_MALFORMED_GATEWAY;

  if (ret == NSM_CLI_GATEWAY_ADDRESS)
    ret = nsm_ipv4_route_unset (nm, vrf_id, p, &gate, ifname, distance,
                                tag, desc);
  else
    ret = nsm_ipv4_route_unset (nm, vrf_id, p, NULL, ifname, distance,
                                tag, desc);

  return ret;
}

int
nsm_cli_no_ip_route (struct nsm_master *nm, vrf_id_t vrf_id,
                     struct prefix_ipv4 *p)
{
  int ret = NSM_API_SET_ERROR;

#ifdef HAVE_CRX
  apply_mask_ipv4 (p);
  if (crx_vip_exists ((struct prefix *)p))
    return CLI_SUCCESS;
#endif /* HAVE_CRX. */

  ret = nsm_ipv4_route_unset_all (nm, vrf_id, p);

  return ret;
}

/* Static route configuration. */
CLI (ip_route_prefix,
     ip_route_prefix_cmd,
     "ip route A.B.C.D/M (A.B.C.D|INTERFACE)",
     CLI_IP_STR,
     "Establish static routes",
     "IP destination prefix (e.g. 10.0.0.0/8)",
     "IP gateway address",
     "IP gateway interface name or pseudo interface Null")
{
  struct nsm_master *nm = cli->vr->proto;
  struct prefix_ipv4 p;
  vrf_id_t vrf_id = VRF_ID_UNSPEC;
  int distance = APN_DISTANCE_STATIC;
  int ret;
  /* Tag field */
  u_int32_t tag = APN_TAG_DEFAULT;
  char *desc = NULL;
  u_int8_t i;

  ret = str2prefix_ipv4 (argv[0], &p);
  if (ret <= 0)
    return nsm_cli_return (cli, NSM_API_SET_ERR_MALFORMED_ADDRESS);

  /* If argc greater than 2 then user must have configured
     tag/distance/description */
  if (argc > 2)
    for (i = 2; i < argc; i++)
      {
        /* check for Tag keyword */
        if (pal_strncmp (argv[i], "t", 1) == 0)
          {
            /* Get the tag value */
            CLI_GET_UINT32_RANGE ("Tag", tag, argv[i+1], 1, 4294967295U);
            i++;
          }
        /* Check for keyword Description */
        else if (pal_strncmp (argv[i], "d", 1) == 0)
          {
            desc = argv[i+1];
            i++;
          }
        /* If argv[i] is neither Tag nor Description then
           that should be Distance  */
        else
          CLI_GET_INTEGER_RANGE ("Distance", distance, argv[i], 1, 255);
      }

  ret = nsm_cli_ip_route_prefix (nm, vrf_id, &p, argv[1], distance, tag, desc);

  return nsm_cli_return (cli, ret);
}

CLI (ip_route_addr_mask,
     ip_route_addr_mask_cmd,
     "ip route A.B.C.D A.B.C.D (A.B.C.D|INTERFACE)",
     CLI_IP_STR,
     "Establish static routes",
     "IP destination prefix",
     "IP destination prefix mask",
     "IP gateway address",
     "IP gateway interface name or pseudo interface Null")
{
  struct nsm_master *nm = cli->vr->proto;
  struct prefix_ipv4 p;
  struct pal_in4_addr mask;
  vrf_id_t vrf_id = VRF_ID_UNSPEC;
  int distance = APN_DISTANCE_STATIC;
  int ret;
  /* Tag field */
  u_int32_t tag = APN_TAG_DEFAULT;
  char *desc = NULL;
  u_int8_t i;

  pal_mem_set (&p, 0, sizeof (struct prefix_ipv4));

  ret = pal_inet_pton (AF_INET, argv[0], &p.prefix);
  if (ret == 0)
    return nsm_cli_return (cli, NSM_API_SET_ERR_MALFORMED_ADDRESS);

  ret = pal_inet_pton (AF_INET, argv[1], &mask);
  if (ret == 0)
    return nsm_cli_return (cli, NSM_API_SET_ERR_MALFORMED_ADDRESS);

  if (!cli_mask_check (mask))
    return nsm_cli_return (cli, NSM_API_SET_ERR_INCONSISTENT_ADDRESS_MASK);

  p.family = AF_INET;
  p.prefixlen = ip_masklen (mask);

  /* If argc greater than 3 then user must have configured
     tag/distance/description */
  if (argc > 3)
    for (i = 3; i < argc; i++)
      {
        if (pal_strncmp (argv[i], "t", 1) == 0)
          {
            CLI_GET_UINT32_RANGE ("Tag", tag, argv[i+1], 1, 4294967295U);
            i++;
          }
        else if (pal_strncmp (argv[i], "d", 1) == 0)
          {
            desc = argv[i+1];
            i++;
          }
        else
          CLI_GET_INTEGER_RANGE ("Distance", distance, argv[i], 1, 255);
      }

  ret = nsm_cli_ip_route_prefix (nm, vrf_id, &p, argv[2], distance, tag, desc);

  return nsm_cli_return (cli, ret);
}

ALI (ip_route_prefix,
     ip_route_prefix_distance_cmd,
     "ip route A.B.C.D/M (A.B.C.D|INTERFACE)"
     "{<1-255>|tag <1-4294967295>|description WORD}",
     CLI_IP_STR,
     "Establish static routes",
     "IP destination prefix (e.g. 10.0.0.0/8)",
     "IP gateway address",
     "IP gateway interface name or pseudo interface Null",
     "Distance value for this route",
     "Set tag for this route",
     "Tag value",
     "Set description of the static route",
     "Description");

ALI (ip_route_addr_mask,
     ip_route_addr_mask_distance_cmd,
     "ip route A.B.C.D A.B.C.D (A.B.C.D|INTERFACE)"
     "{<1-255>|tag <1-4294967295>|description WORD}",
     CLI_IP_STR,
     "Establish static routes",
     "IP destination prefix",
     "IP destination prefix mask",
     "IP gateway address",
     "IP gateway interface name or pseudo interface Null",
     "Distance value for this route",
     "Set tag for this route",
     "Tag value",
     "Set description of the static route",
     "Description");

CLI (no_ip_route_prefix,
     no_ip_route_prefix_cmd,
     "no ip route A.B.C.D/M (A.B.C.D|INTERFACE|)",
     CLI_NO_STR,
     CLI_IP_STR,
     "Establish static routes",
     "IP destination prefix (e.g. 10.0.0.0/8)",
     "IP gateway address",
     "IP gateway interface name or pseudo interface Null")
{
  struct nsm_master *nm = cli->vr->proto;
  struct prefix_ipv4 p;
  vrf_id_t vrf_id = VRF_ID_UNSPEC;
  int distance = APN_DISTANCE_STATIC;
  int ret = CLI_ERROR;
  /* Tag field */
  u_int32_t tag = APN_TAG_DEFAULT;
  char *desc = NULL;
  u_int8_t i;

  ret = str2prefix_ipv4 (argv[0], &p);
  if (ret <= 0)
    {
      cli_out (cli, "%% Malformed address\n");
      return CLI_ERROR;
    }

  /* If argc greater than 2 then user must have configured
     tag/distance/description */
  if (argc > 2)
    for (i = 2; i < argc; i++)
      {
        if (pal_strncmp (argv[i], "t", 1) == 0)
          {
            CLI_GET_UINT32_RANGE ("Tag", tag, argv[i+1], 1, 4294967295U);
            i++;
          }
        else if (pal_strncmp (argv[i], "d", 1) == 0)
          {
            desc = argv[i+1];
            i++;
          }
        else
          CLI_GET_INTEGER_RANGE ("Distance", distance, argv[i], 1, 255);
      }

  if (argc > 1)
    ret = nsm_cli_no_ip_route_prefix (nm, vrf_id, &p, argv[1], distance,
                                tag, desc);
  else
    ret = nsm_cli_no_ip_route (nm, vrf_id, &p);

  return nsm_cli_return (cli, ret);
}

CLI (no_ip_route_addr_mask,
     no_ip_route_addr_mask_cmd,
     "no ip route A.B.C.D A.B.C.D (A.B.C.D|INTERFACE|)",
     CLI_NO_STR,
     CLI_IP_STR,
     "Establish static routes",
     "IP destination prefix",
     "IP destination prefix mask",
     "IP gateway address",
     "IP gateway interface name or pseudo interface Null")
{
  struct nsm_master *nm = cli->vr->proto;
  struct prefix_ipv4 p;
  struct pal_in4_addr mask;
  vrf_id_t vrf_id = VRF_ID_UNSPEC;
  int distance = APN_DISTANCE_STATIC;
  int ret = CLI_ERROR;
  /* Tag field */
  u_int32_t tag = APN_TAG_DEFAULT;
  char *desc = NULL;
  u_int8_t i;

  pal_mem_set (&p, 0, sizeof (struct prefix_ipv4));

  ret = pal_inet_pton (AF_INET, argv[0], &p.prefix);
  if (ret == 0)
    return nsm_cli_return (cli, NSM_API_SET_ERR_MALFORMED_ADDRESS);

  ret = pal_inet_pton (AF_INET, argv[1], &mask);
  if (ret == 0)
    return nsm_cli_return (cli, NSM_API_SET_ERR_MALFORMED_ADDRESS);

  if (!cli_mask_check (mask))
    return nsm_cli_return (cli, NSM_API_SET_ERR_INCONSISTENT_ADDRESS_MASK);

  p.family = AF_INET;
  p.prefixlen = ip_masklen (mask);

  /* If argc greater than 3 then user must have configured
     tag/distance/description */
  if (argc > 3)
    for (i = 3; i < argc; i++)
      {
        if (pal_strncmp (argv[i], "t", 1) == 0)
          {
            CLI_GET_UINT32_RANGE ("Tag", tag, argv[i+1], 1, 4294967295U);
            i++;
          }
        else if (pal_strncmp (argv[i], "d", 1) == 0)
          {
            desc = argv[i+1];
            i++;
          }
        else
          CLI_GET_INTEGER_RANGE ("Distance", distance, argv[i], 1, 255);
      }

  if (argc > 2)
    ret = nsm_cli_no_ip_route_prefix (nm, vrf_id, &p, argv[2], distance, tag, desc);
  else
    ret = nsm_cli_no_ip_route (nm, vrf_id, &p);

  return nsm_cli_return (cli, ret);
}

ALI (no_ip_route_prefix,
     no_ip_route_prefix_distance_cmd,
     "no ip route A.B.C.D/M (A.B.C.D|INTERFACE)"
     "{<1-255>|tag <1-4294967295>|description WORD}",
     CLI_NO_STR,
     CLI_IP_STR,
     "Establish static routes",
     "IP destination prefix (e.g. 10.0.0.0/8)",
     "IP gateway address",
     "IP gateway interface name or pseudo interface Null",
     "Distance value for this route",
     "Set tag for this route",
     "Tag value",
     "Set description of the static route",
     "Description");

ALI (no_ip_route_addr_mask,
     no_ip_route_addr_mask_distance_cmd,
     "no ip route A.B.C.D A.B.C.D (A.B.C.D|INTERFACE)"
     "{<1-255>|tag <1-4294967295>|description WORD}",
     CLI_NO_STR,
     CLI_IP_STR,
     "Establish static routes",
     "IP destination prefix",
     "IP destination prefix mask",
     "IP gateway address",
     "IP gateway interface name or pseudo interface Null",
     "Distance value for this route",
     "Set tag for this route",
     "Tag value",
     "Set description of the static route",
     "Description");

#ifdef HAVE_VRF
CLI (ip_route_vrf_ifname,
     ip_route_vrf_ifname_cmd,
     "ip route vrf NAME A.B.C.D/M INTERFACE",
     CLI_IP_STR,
     "Establish static routes into VRF",
     CLI_VRF_STR,
     CLI_VRF_NAME_STR,
     "IP destination prefix (e.g. 10.0.0.0/8)",
     "IP gateway interface name")
{
  struct nsm_master *nm = cli->vr->proto;
  struct apn_vr *vr = cli->vr;
  struct apn_vrf *vrf;
  struct interface *ifp;
  struct prefix_ipv4 p;
  int distance = APN_DISTANCE_STATIC;
  int ret;
  /* Tag field */
  u_int32_t tag = APN_TAG_DEFAULT;
  char *desc = NULL;
  u_int8_t i;

  vrf = apn_vrf_lookup_by_name (vr, argv[0]);
  if (vrf == NULL)
    return nsm_cli_return (cli, NSM_API_SET_ERR_VRF_NOT_EXIST);

  ifp = if_lookup_by_name (&vr->ifm, argv[2]);
  if (ifp == NULL)
    return nsm_cli_return (cli, NSM_API_SET_ERR_NO_SUCH_INTERFACE);

  if (ifp->vrf != vrf || ifp->vrf->id == 0)
    return nsm_cli_return (cli, NSM_API_SET_ERR_VRF_NOT_BOUND);

  /* Get prefix. */
  ret = str2prefix_ipv4 (argv[1], &p);
  if (ret <= 0)
    return nsm_cli_return (cli, NSM_API_SET_ERR_MALFORMED_ADDRESS);

  /* If argc greater than 3 then user must have configured
     tag/distance/description */
  if (argc > 3)
   for (i = 3; i < argc; i++)
     {
       /* Get the corresponding tag and description values */
       if (pal_strncmp(argv[i], "t", 1) == 0)
         {
           CLI_GET_UINT32_RANGE ("Tag", tag, argv[i+1], 1, 4294967295U);
           i++;
         }
       else if (pal_strncmp (argv[i], "d", 1) == 0)
         {
           desc = argv[i+1];
           i++;
         }
     }

  ret = nsm_ipv4_route_set (nm, vrf->id, &p, NULL, argv[2],
                            distance, 0, ROUTE_TYPE_OTHER,
                            tag, desc);

  return nsm_cli_return (cli, ret);
}

ALI (ip_route_vrf_ifname,
     ip_route_vrf_ifname_tag_desc_cmd,
     "ip route vrf NAME A.B.C.D/M INTERFACE"
     "{tag <1-4294967295>|description WORD}",
     CLI_IP_STR,
     "Establish static routes into VRF",
     CLI_VRF_STR,
     CLI_VRF_NAME_STR,
     "IP destination prefix (e.g. 10.0.0.0/8)",
     "IP gateway interface name",
     "Set tag for this route",
     "Tag value",
     "Set description of the static route",
     "Description");

CLI (ip_route_vrf,
     ip_route_vrf_cmd,
     "ip route vrf NAME A.B.C.D/M A.B.C.D INTERFACE",
     CLI_IP_STR,
     "Establish static routes into VRF",
     CLI_VRF_STR,
     CLI_VRF_NAME_STR,
     "IP destination prefix (e.g. 10.0.0.0/8)",
     "IP gateway address",
     "IP gateway interface name")
{
  struct nsm_master *nm = cli->vr->proto;
  struct apn_vr *vr = cli->vr;
  struct apn_vrf *vrf;
  struct prefix_ipv4 p;
  struct interface *ifp;
  union nsm_nexthop gate;
  int distance = APN_DISTANCE_STATIC;
  int ret;
  /* Tag field */
  u_int32_t tag = APN_TAG_DEFAULT;
  char *desc = NULL;
  u_int8_t i;

  vrf = apn_vrf_lookup_by_name (vr, argv[0]);
  if (vrf == NULL)
    return nsm_cli_return (cli, NSM_API_SET_ERR_VRF_NOT_EXIST);

  ifp = if_lookup_by_name (&vr->ifm, argv[3]);
  if (ifp == NULL)
    return nsm_cli_return (cli, NSM_API_SET_ERR_NO_SUCH_INTERFACE);

  if (ifp->vrf != vrf || ifp->vrf->id == 0)
    return nsm_cli_return (cli, NSM_API_SET_ERR_VRF_NOT_BOUND);

  /* Get prefix. */
  ret = str2prefix_ipv4 (argv[1], &p);
  if (ret <= 0)
    return nsm_cli_return (cli, NSM_API_SET_ERR_MALFORMED_ADDRESS);

  /* Get gateway address. */
  CLI_GET_IPV4_ADDRESS ("Gateway address", gate.ipv4, argv[2]);

  /* If argc greater than 4 then user must have configured
     tag/distance/description */
  if (argc > 4)
    for (i = 4; i < argc; i++)
      {
        if (pal_strncmp (argv[i], "t", 1) == 0)
          {
            CLI_GET_INTEGER_RANGE ("Tag", tag, argv[i+1], 1, 4294967295U);
            i++;
          }
        else if (pal_strncmp (argv[i], "d", 1) == 0)
          {
            desc = argv[i+1];
            i++;
          }
      }
  ret = nsm_ipv4_route_set (nm, vrf->id, &p, &gate, argv[3],
                            distance, 0, ROUTE_TYPE_OTHER,
                            tag, desc);

  return nsm_cli_return (cli, ret);
}

ALI (ip_route_vrf,
     ip_route_vrf_tag_desc_cmd,
     "ip route vrf NAME A.B.C.D/M A.B.C.D INTERFACE"
     "{tag <1-4294967295>|description WORD}",
     CLI_IP_STR,
     "Establish static routes into VRF",
     CLI_VRF_STR,
     CLI_VRF_NAME_STR,
     "IP destination prefix (e.g. 10.0.0.0/8)",
     "IP gateway address",
     "IP gateway interface name",
     "Set tag for this route",
     "Tag value",
     "Set description of the static route",
     "Description");

CLI (no_ip_route_vrf_ifname,
     no_ip_route_vrf_ifname_cmd,
     "no ip route vrf NAME A.B.C.D/M INTERFACE",
     CLI_NO_STR,
     CLI_IP_STR,
     "Establish static routes into VRF",
     CLI_VRF_STR,
     CLI_VRF_NAME_STR,
     "IP destination prefix (e.g. 10.0.0.0/8)",
     "IP gateway interface name")
{
  struct nsm_master *nm = cli->vr->proto;
  struct apn_vr *vr = cli->vr;
  struct apn_vrf *vrf;
  struct interface *ifp;
  struct prefix_ipv4 p;
  int distance = APN_DISTANCE_STATIC;
  int ret = CLI_ERROR;
  /* Tag field */
  u_int32_t tag = APN_TAG_DEFAULT;
  char *desc = NULL;
  u_int8_t i;

  vrf = apn_vrf_lookup_by_name (vr, argv[0]);
  if (vrf == NULL)
    return nsm_cli_return (cli, NSM_API_SET_ERR_VRF_NOT_EXIST);

  ifp = if_lookup_by_name (&vr->ifm, argv[2]);
  if (ifp == NULL)
    return nsm_cli_return (cli, NSM_API_SET_ERR_NO_SUCH_INTERFACE);

  if (ifp->vrf != vrf || ifp->vrf->id == 0)
    return nsm_cli_return (cli, NSM_API_SET_ERR_VRF_NOT_BOUND);

  /* Get prefix. */
  ret = str2prefix_ipv4 (argv[1], &p);
  if (ret <= 0)
    return nsm_cli_return (cli, NSM_API_SET_ERR_MALFORMED_ADDRESS);

  /* If argc greater than 3 then user must have configured
     tag/distance/description */
  if (argc > 3)
   for (i = 3; i < argc; i++)
     {
       /* Get the corresponding tag and description values */
       if (pal_strncmp(argv[i], "t", 1) == 0)
         {
           CLI_GET_UINT32_RANGE ("Tag", tag, argv[i+1], 1, 4294967295U);
           i++;
         }
       else if (pal_strncmp (argv[i], "d", 1) == 0)
         {
           desc = argv[i+1];
           i++;
         }
     }

  ret = nsm_ipv4_route_unset (nm, vrf->id, &p, NULL, argv[2], distance,
                              tag, desc);

  return nsm_cli_return (cli, ret);
}

ALI (no_ip_route_vrf_ifname,
     no_ip_route_vrf_ifname_tag_desc_cmd,
     "no ip route vrf NAME A.B.C.D/M INTERFACE"
     "{tag <1-4294967295>|description WORD}",
     CLI_NO_STR,
     CLI_IP_STR,
     "Establish static routes into VRF",
     CLI_VRF_STR,
     CLI_VRF_NAME_STR,
     "IP destination prefix (e.g. 10.0.0.0/8)",
     "IP gateway interface name",
     "Set tag for this route",
     "Tag value",
     "Set description of the static route",
     "Description");

CLI (no_ip_route_vrf,
     no_ip_route_vrf_cmd,
     "no ip route vrf NAME A.B.C.D/M A.B.C.D INTERFACE",
     CLI_NO_STR,
     CLI_IP_STR,
     "Establish static routes into VRF",
     CLI_VRF_STR,
     CLI_VRF_NAME_STR,
     "IP destination prefix (e.g. 10.0.0.0/8)",
     "IP gateway address",
     "IP gateway interface name")
{
  struct nsm_master *nm = cli->vr->proto;
  struct apn_vr *vr = cli->vr;
  struct apn_vrf *vrf;
  struct prefix_ipv4 p;
  struct interface *ifp;
  union nsm_nexthop gate;
  int distance = APN_DISTANCE_STATIC;
  int ret;
  /* Tag field */
  u_int32_t tag = APN_TAG_DEFAULT;
  char *desc = NULL;
  u_int8_t i;

  vrf = apn_vrf_lookup_by_name (vr, argv[0]);
  if (vrf == NULL)
    return nsm_cli_return (cli, NSM_API_SET_ERR_VRF_NOT_EXIST);

  ifp = if_lookup_by_name (&vr->ifm, argv[3]);
  if (ifp == NULL)
    return nsm_cli_return (cli, NSM_API_SET_ERR_NO_SUCH_INTERFACE);

  if (ifp->vrf != vrf || ifp->vrf->id == 0)
    return nsm_cli_return (cli, NSM_API_SET_ERR_VRF_NOT_BOUND);

  /* Get prefix. */
  ret = str2prefix_ipv4 (argv[1], &p);
  if (ret <= 0)
    return nsm_cli_return (cli, NSM_API_SET_ERR_MALFORMED_ADDRESS);

  /* Get gateway address. */
  CLI_GET_IPV4_ADDRESS ("Gateway address", gate.ipv4, argv[2]);

  /* If argc greater than 4 then user must have configured
     tag/distance/description */
  if (argc > 4)
    for (i = 4; i < argc; i++)
      {
        if (pal_strncmp (argv[i], "t", 1) == 0)
          {
            CLI_GET_INTEGER_RANGE ("Tag", tag, argv[i+1], 1, 4294967295U);
            i++;
          }
        else if (pal_strncmp (argv[i], "d", 1) == 0)
          {
            desc = argv[i+1];
            i++;
          }
      }

  ret = nsm_ipv4_route_unset (nm, vrf->id, &p, &gate, argv[3], distance,
                              tag, desc);

  return nsm_cli_return (cli, ret);
}

ALI (no_ip_route_vrf,
     no_ip_route_vrf_tag_desc_cmd,
     "no ip route vrf NAME A.B.C.D/M A.B.C.D INTERFACE"
     "{tag <1-4294967295>|description WORD}",
     CLI_NO_STR,
     CLI_IP_STR,
     "Establish static routes into VRF",
     CLI_VRF_STR,
     CLI_VRF_NAME_STR,
     "IP destination prefix (e.g. 10.0.0.0/8)",
     "IP gateway address",
     "IP gateway interface name",
     "Set tag for this route",
     "Tag value",
     "Set description of the static route",
     "Description");
#endif /* HAVE_VRF */


#ifdef HAVE_IPV6
int
nsm_cli_ipv6_addr_check (struct prefix_ipv6 *p, struct interface *ifp)
{
  /* 128 bit address prefix is disallowed only on broadcast interface */
  if (ifp && if_is_broadcast (ifp) && p->prefixlen == IPV6_MAX_PREFIXLEN)
    return 0;

  if (IN6_IS_ADDR_MULTICAST (&p->prefix))
    return 0;

  if (IN6_IS_ADDR_LOOPBACK (&p->prefix))
    return 0;

  if (IN6_IS_ADDR_UNSPECIFIED (&p->prefix))
    return 0;

  return 1;
}

int
nsm_cli_ipv6_gateway_get (struct nsm_master *nm, char *str, char **ifname,
                          union nsm_nexthop *gate, vrf_id_t vrf_id)
{
  struct interface *ifp;
  int ret;

  ret = pal_inet_pton (AF_INET6, str, &gate->ipv6);
  if (!ret)
    {
      if (IS_NULL_INTERFACE_STR (str))
        *ifname = NULL_INTERFACE;
      else
        {
          /* Check if it is valid ifname. */
          ifp = if_lookup_by_name (&nm->vr->ifm, str);
          if (ifp == NULL || ifp->vrf->id != vrf_id)
            return NSM_CLI_GATEWAY_INVALID;

          *ifname = str;
        }
      return NSM_CLI_GATEWAY_IFNAME;
    }
  else if (IN6_IS_ADDR_MULTICAST (&gate->ipv6)
           || IN6_IS_ADDR_UNSPECIFIED (&gate->ipv6))
    return NSM_CLI_GATEWAY_INVALID;

  return NSM_CLI_GATEWAY_ADDRESS;
}

int
nsm_cli_ipv6_route_prefix (struct nsm_master *nm, vrf_id_t vrf_id,
                           struct prefix_ipv6 *p,
                           char *gate_str, int distance)
{
  union nsm_nexthop gate;
  char *ifname = NULL;
  int ret;

  ret = nsm_cli_ipv6_gateway_get (nm, gate_str, &ifname, &gate, vrf_id);
  if (ret == NSM_CLI_GATEWAY_INVALID)
    return NSM_API_SET_ERR_MALFORMED_GATEWAY;

  if (ret == NSM_CLI_GATEWAY_ADDRESS)
    ret = nsm_ipv6_route_set (nm, vrf_id, p, &gate, ifname, distance);
  else
    ret = nsm_ipv6_route_set (nm, vrf_id, p, NULL, ifname, distance);

  return ret;
}

int
nsm_cli_ipv6_route_prefix_ifname (struct nsm_master *nm, vrf_id_t vrf_id,
                                  struct prefix_ipv6 *p, union nsm_nexthop *gate,
                                  char *ifname, int distance)
{
  struct interface *ifp;
  int ret;

  ifp = if_lookup_by_name (&nm->vr->ifm, ifname);
  if (ifp == NULL || ifp->vrf->id != vrf_id)
    return NSM_API_SET_ERR_NO_SUCH_INTERFACE;

  ret = nsm_ipv6_route_set (nm, vrf_id, p, gate, ifname, distance);

  return ret;
}

int
nsm_cli_no_ipv6_route_prefix (struct nsm_master *nm,
                              vrf_id_t vrf_id, struct prefix_ipv6 *p,
                              char *gate_str, int distance)
{
  union nsm_nexthop gate;
  char *ifname = NULL;
  int ret;

  ret = nsm_cli_ipv6_gateway_get (nm, gate_str, &ifname, &gate, vrf_id);
  if (ret == NSM_CLI_GATEWAY_INVALID)
    return NSM_API_SET_ERR_MALFORMED_GATEWAY;

  if (ret == NSM_CLI_GATEWAY_ADDRESS)
    ret = nsm_ipv6_route_unset (nm, vrf_id, p, &gate, ifname, distance);
  else
    ret = nsm_ipv6_route_unset (nm, vrf_id, p, NULL, ifname, distance);

  return NSM_API_SET_SUCCESS;
}

CLI (ipv6_route_prefix,
     ipv6_route_prefix_cmd,
     "ipv6 route X:X::X:X/M (X:X::X:X|INTERFACE)",
     CLI_IPV6_STR,
     "Establish static routes",
     "IPv6 destination prefix (e.g. 3ffe:506::/32)",
     "IPv6 gateway address",
     "IPv6 gateway interface name or pseudo interface Null")
{
  struct nsm_master *nm = cli->vr->proto;
  struct prefix_ipv6 p;
  vrf_id_t vrf_id = VRF_ID_UNSPEC;
  int distance = APN_DISTANCE_STATIC;
  int ret;

  ret = str2prefix_ipv6 (argv[0], &p);
  if (ret <= 0)
    return nsm_cli_return (cli, NSM_API_SET_ERR_MALFORMED_ADDRESS);

  if (argc > 2)
    CLI_GET_INTEGER_RANGE ("Distance", distance, argv[2], 1, 255);

  ret = nsm_cli_ipv6_route_prefix (nm, vrf_id, &p, argv[1], distance);

  return nsm_cli_return (cli, ret);
}

ALI (ipv6_route_prefix,
     ipv6_route_prefix_distance_cmd,
     "ipv6 route X:X::X:X/M (X:X::X:X|INTERFACE) <1-255>",
     CLI_IPV6_STR,
     "Establish static routes",
     "IPv6 destination prefix (e.g. 3ffe:506::/32)",
     "IPv6 gateway address",
     "IPv6 gateway interface name",
     "Distance value for this prefix");

CLI (ipv6_route_prefix_ifname,
     ipv6_route_prefix_ifname_cmd,
     "ipv6 route X:X::X:X/M X:X::X:X INTERFACE",
     CLI_IPV6_STR,
     "Establish static routes",
     "IPv6 destination prefix (e.g. 3ffe:506::/32)",
     "IPv6 gateway address",
     "IPv6 gateway interface name or pseudo interface Null")
{
  struct nsm_master *nm = cli->vr->proto;
  struct prefix_ipv6 p;
  union nsm_nexthop gate;
  vrf_id_t vrf_id = VRF_ID_UNSPEC;
  int distance = APN_DISTANCE_STATIC;
  int ret;

  ret = str2prefix_ipv6 (argv[0], &p);
  if (ret <= 0)
    return nsm_cli_return (cli, NSM_API_SET_ERR_MALFORMED_ADDRESS);

  CLI_GET_IPV6_ADDRESS ("Nexthop address", gate.ipv6, argv[1]);

  if (argc > 3)
    CLI_GET_INTEGER_RANGE ("Distance", distance, argv[3], 1, 255);

  ret = nsm_cli_ipv6_route_prefix_ifname (nm, vrf_id, &p, &gate, argv[2], distance);

  return nsm_cli_return (cli, ret);
}

ALI (ipv6_route_prefix_ifname,
     ipv6_route_prefix_ifname_distance_cmd,
     "ipv6 route X:X::X:X/M X:X::X:X INTERFACE <1-255>",
     CLI_IPV6_STR,
     "Establish static routes",
     "IPv6 destination prefix (e.g. 3ffe:506::/32)",
     "IPv6 gateway address",
     "IPv6 gateway interface name",
     "Distance value for this prefix");

CLI (no_ipv6_route_prefix,
     no_ipv6_route_prefix_cmd,
     "no ipv6 route X:X::X:X/M (X:X::X:X|INTERFACE)",
     CLI_NO_STR,
     CLI_IP_STR,
     "Establish static routes",
     "IPv6 destination prefix (e.g. 3ffe:506::/32)",
     "IPv6 gateway address",
     "IPv6 gateway interface name or pseudo interface Null")
{
  struct nsm_master *nm = cli->vr->proto;
  struct prefix_ipv6 p;
  vrf_id_t vrf_id = VRF_ID_UNSPEC;
  int distance = APN_DISTANCE_STATIC;
  int ret;

  ret = str2prefix_ipv6 (argv[0], &p);
  if (ret <= 0)
    return nsm_cli_return (cli, NSM_API_SET_ERR_MALFORMED_ADDRESS);

  if (argc > 2)
    CLI_GET_INTEGER_RANGE ("Distance", distance, argv[2], 1, 255);

  ret = nsm_cli_no_ipv6_route_prefix (nm, vrf_id, &p, argv[1], distance);

  return nsm_cli_return (cli, ret);
}

ALI (no_ipv6_route_prefix,
     no_ipv6_route_prefix_distance_cmd,
     "no ipv6 route X:X::X:X/M (X:X::X:X|INTERFACE) <1-255>",
     CLI_NO_STR,
     CLI_IP_STR,
     "Establish static routes",
     "IPv6 destination prefix (e.g. 3ffe:506::/32)",
     "IPv6 gateway address",
     "IPv6 gateway interface name",
     "Distance value for this prefix");

CLI (no_ipv6_route_prefix_ifname,
     no_ipv6_route_prefix_ifname_cmd,
     "no ipv6 route X:X::X:X/M X:X::X:X INTERFACE",
     CLI_NO_STR,
     CLI_IP_STR,
     "Establish static routes",
     "IPv6 destination prefix (e.g. 3ffe:506::/32)",
     "IPv6 gateway address",
     "IPv6 gateway interface name or pseudo interface Null")
{
  struct nsm_master *nm = cli->vr->proto;
  struct prefix_ipv6 p;
  union nsm_nexthop gate;
  vrf_id_t vrf_id = VRF_ID_UNSPEC;
  int distance = APN_DISTANCE_STATIC;
  int ret;

  ret = str2prefix_ipv6 (argv[0], &p);
  if (ret <= 0)
    return nsm_cli_return (cli, NSM_API_SET_ERR_MALFORMED_ADDRESS);

  CLI_GET_IPV6_ADDRESS ("Nexthop address", gate.ipv6, argv[1]);

  if (argc > 3)
    CLI_GET_INTEGER_RANGE ("Distance", distance, argv[3], 1, 255);

  ret = nsm_ipv6_route_unset (nm, vrf_id, &p, &gate, argv[2], distance);

  return nsm_cli_return (cli, ret);
}

ALI (no_ipv6_route_prefix_ifname,
     no_ipv6_route_prefix_ifname_distance_cmd,
     "no ipv6 route X:X::X:X/M X:X::X:X INTERFACE <1-255>",
     CLI_NO_STR,
     CLI_IP_STR,
     "Establish static routes",
     "IPv6 destination prefix (e.g. 3ffe:506::/32)",
     "IPv6 gateway address",
     "IPv6 gateway interface name",
     "Distance value for this prefix");

CLI (no_ipv6_route,
     no_ipv6_route_cmd,
     "no ipv6 route X:X::X:X/M",
     CLI_NO_STR,
     CLI_IP_STR,
     "Establish static routes",
     "IPv6 destination prefix (e.g. 3ffe:506::/32)")
{
  struct nsm_master *nm = cli->vr->proto;
  struct prefix_ipv6 p;
  vrf_id_t vrf_id = VRF_ID_UNSPEC;
  int ret;

  ret = str2prefix_ipv6 (argv[0], &p);
  if (ret <= 0)
    return nsm_cli_return (cli, NSM_API_SET_ERR_MALFORMED_ADDRESS);

  ret = nsm_ipv6_route_unset_all (nm, vrf_id, &p);

  return nsm_cli_return (cli, ret);
}
#ifdef HAVE_VRF /*6VPE*/
CLI (ipv6_route_vrf_ifname,
     ipv6_route_vrf_ifname_cmd,
     "ipv6 route vrf NAME X:X::X:X/M X:X::X:X INTERFACE",
     CLI_IPV6_STR,
     "Establish IPV6 static routes into VRF",
     CLI_VRF_STR,
     CLI_VRF_NAME_STR,
     "IPV6 destination prefix (e.g. 3ffe:506::1/32)",
     "IPv6 gateway address",
     "IPV6 gateway interface name")
{
  struct nsm_master *nm = cli->vr->proto;
  struct apn_vr *vr = cli->vr;
  struct apn_vrf *vrf;
  struct interface *ifp;
  struct prefix_ipv6 p;
  int distance = APN_DISTANCE_STATIC;
  int ret;

  vrf = apn_vrf_lookup_by_name (vr, argv[0]);
  if (vrf == NULL)
    return nsm_cli_return (cli, NSM_API_SET_ERR_VRF_NOT_EXIST);

  ifp = if_lookup_by_name (&vr->ifm, argv[3]);
  if (ifp == NULL)
    return nsm_cli_return (cli, NSM_API_SET_ERR_NO_SUCH_INTERFACE);

  if (ifp->vrf != vrf || ifp->vrf->id == 0)
    return nsm_cli_return (cli, NSM_API_SET_ERR_VRF_NOT_BOUND);

  /* Get prefix. */
  ret = str2prefix_ipv6 (argv[1], &p);
  if (ret <= 0)
    return nsm_cli_return (cli, NSM_API_SET_ERR_MALFORMED_ADDRESS);

   ret = nsm_cli_ipv6_route_prefix (nm, vrf->id, &p, argv[2], distance);

  return nsm_cli_return (cli, ret);
}

CLI (no_ipv6_route_vrf_ifname,
     no_ipv6_route_vrf_ifname_cmd,
     "no ipv6 route vrf NAME X:X::X:X/M X:X::X:X INTERFACE",
     CLI_NO_STR,
     CLI_IPV6_STR,
     "Delete IPV6 static route from VRF",
     CLI_VRF_STR,
     CLI_VRF_NAME_STR,
     "IPV6 destination prefix (e.g. 3ffe:506::/32)",
     "IPv6 gateway address",
     "IPV6 gateway interface name")
{
  struct nsm_master *nm = cli->vr->proto;
  struct apn_vr *vr = cli->vr;
  struct apn_vrf *vrf;
  struct interface *ifp;
  struct prefix_ipv6 p;
  int distance = APN_DISTANCE_STATIC;
  int ret;

  vrf = apn_vrf_lookup_by_name (vr, argv[0]);
  if (vrf == NULL)
    return nsm_cli_return (cli, NSM_API_SET_ERR_VRF_NOT_EXIST);

  ifp = if_lookup_by_name (&vr->ifm, argv[3]);
  if (ifp == NULL)
    return nsm_cli_return (cli, NSM_API_SET_ERR_NO_SUCH_INTERFACE);

  if (ifp->vrf != vrf || ifp->vrf->id == 0)
    return nsm_cli_return (cli, NSM_API_SET_ERR_VRF_NOT_BOUND);

  /* Get prefix. */
  ret = str2prefix_ipv6 (argv[1], &p);
  if (ret <= 0)
    return nsm_cli_return (cli, NSM_API_SET_ERR_MALFORMED_ADDRESS);

  ret = nsm_cli_no_ipv6_route_prefix (nm, vrf->id, &p, argv[2], distance);

  return nsm_cli_return (cli, ret);
}
#endif /*HAVE_VRF*/
#endif /* HAVE_IPV6 */
#endif /* HAVE_L3 */

#ifdef HAVE_VRF
/* Create VRF instance.  */
CLI (ip_vrf_name,
     ip_vrf_name_cmd,
     "ip vrf WORD",
     CLI_IP_STR,
     CLI_VRF_STR,
     CLI_VRF_NAME_STR)
{
  struct nsm_master *nm = cli->vr->proto;
  struct apn_vr *vr = cli->vr;
  int ret;

  ret = nsm_ip_vrf_set (vr, argv[0]);
  if (ret != CLI_SUCCESS)
    return nsm_cli_return (cli, ret);

  cli->mode = VRF_MODE;
  cli->index = nsm_vrf_lookup_by_name (nm, argv[0]);

  return CLI_SUCCESS;
}

CLI (no_ip_vrf_name,
     no_ip_vrf_name_cmd,
     "no ip vrf WORD",
     CLI_NO_STR,
     CLI_IP_STR,
     CLI_VRF_STR,
     CLI_VRF_NAME_STR)
{
  struct apn_vr *vr = cli->vr;
  int ret;

  ret = nsm_ip_vrf_unset (vr, argv[0]);

  return nsm_cli_return (cli, ret);
}

CLI (vrf_desc,
     vrf_desc_cmd,
     "description LINE",
     "VRF specific description",
     "Characters describing this VRF")
{
  struct nsm_vrf *vrf = cli->index;

  if (vrf->desc)
    XFREE (MTYPE_IF_DESC, vrf->desc);

  vrf->desc = XSTRDUP (MTYPE_IF_DESC, argv[0]);

  return CLI_SUCCESS;
}

CLI (no_vrf_desc,
     no_vrf_desc_cmd,
     "no description",
     CLI_NO_STR,
     "VRF specific description")
{
  struct nsm_vrf *vrf = cli->index;

  if (vrf->desc)
    {
      XFREE (MTYPE_IF_DESC, vrf->desc);
      vrf->desc = NULL;
    }

  return CLI_SUCCESS;
}
#endif /* HAVE_VRF */

#ifdef HAVE_L3
int
nsm_config_write_router_id (struct cli *cli, struct nsm_vrf *nv)
{
  int write = 0;

  if (CHECK_FLAG (nv->config, NSM_VRF_CONFIG_ROUTER_ID))
    {
      cli_out (cli, "%srouter-id %r\n",
               nv->vrf->id == VRF_ID_MAIN ? "" : " ", &nv->router_id_config);
      write++;
    }
  return write;
}

/* Write FIB configuration.  */
int
nsm_config_write_fib (struct cli *cli)
{
  struct nsm_master *nm = cli->vr->proto;
  int write = 0;

  if (!CHECK_FLAG (nm->flags, NSM_IPV4_FORWARDING))
    cli_out (cli, "no ip forwarding\n");

#ifdef HAVE_IPV6
  if (!CHECK_FLAG (nm->flags, NSM_IPV6_FORWARDING))
    cli_out (cli, "no ipv6 forwarding\n");
#endif /*HAVE_IPV6*/

  if (CHECK_FLAG (nm->flags, NSM_FIB_RETAIN_RESTART))
    {
      if (nm->fib_retain_time == NSM_FIB_RETAIN_TIME_DEFAULT)
        cli_out (cli, "fib retain\n");
      else if (nm->fib_retain_time == NSM_FIB_RETAIN_TIME_FOREVER)
        cli_out (cli, "fib retain forever\n");
      else
        cli_out (cli, "fib retain time %d\n", nm->fib_retain_time);
      write++;
    }
  return write;
}

#define NSM_VRF_NAMELEN_MAX       (64 + 5)

/* Write static route configuration. */
int
nsm_config_write_ip_route (struct cli *cli, struct nsm_vrf *nv)
{
  struct ptree_node *rn;
  struct nsm_static *ns;
  char buf1[NSM_VRF_NAMELEN_MAX];
  char buf2[19];
  int write = 0;
  struct prefix p;

  if (!NSM_CAP_HAVE_VRF && nv->vrf->id != 0)
    return 0;

  if (nv->vrf->id != 0)
    zsnprintf (buf1, NSM_VRF_NAMELEN_MAX, " vrf %s", nv->vrf->name);
  else
    buf1[0] = '\0';

  for (rn = ptree_top (nv->IPV4_STATIC); rn; rn = ptree_next (rn))
    if (rn->info)
      {
        pal_mem_set (&p, 0, sizeof (struct prefix));
        p.prefixlen = rn->key_len;
        NSM_PTREE_KEY_COPY (&p.u.prefix4, rn);

        zsnprintf (buf2, 19, "%P", &p);
        for (ns = rn->info; ns; ns = ns->next)
          {
            cli_out (cli, "ip route%s %s", buf1, buf2);

            switch (ns->type)
              {
              case NEXTHOP_TYPE_IPV4:
                cli_out (cli, " %r", &ns->gate.ipv4);
                break;
              case NEXTHOP_TYPE_IFNAME:
                cli_out (cli, " %s", ns->ifname);
                break;
              case NEXTHOP_TYPE_IPV4_IFNAME:
                cli_out (cli, " %r %s", &ns->gate.ipv4, ns->ifname);
                break;
              }

            if (ns->distance != APN_DISTANCE_STATIC)
              cli_out (cli, " %d", ns->distance);

           /* Display tag value */
           if (ns->tag != APN_TAG_DEFAULT)
             cli_out (cli, " tag %d", ns->tag);

          /* Display description of the route */
          if (ns->desc)
            cli_out (cli, " description %s", ns->desc);

            cli_out (cli, "\n");

            write++;
          }
      }

#ifdef HAVE_BFD
      if (CHECK_FLAG (nv->config, NSM_VRF_CONFIG_BFD_IPV4))
        {
          cli_out (cli, "ip bfd static all-interfaces\n");
          write++;
        }
#endif /* HAVE_BFD */

  return write;
}

#ifdef HAVE_IPV6
/* Write IPv6 static route configuration. */
int
nsm_config_write_ipv6_route (struct cli *cli, struct nsm_vrf *nv)
{
  struct ptree_node *rn;
  struct nsm_static *ns;
  char buf1[NSM_VRF_NAMELEN_MAX];
  char buf2[45];
  int write = 0;
  struct prefix p;

  if (!NSM_CAP_HAVE_VRF && nv->vrf->id != 0)
    return 0;

  if (nv->vrf->id != 0)
    zsnprintf (buf1, NSM_VRF_NAMELEN_MAX, " vrf %s", nv->vrf->name);
  else
    buf1[0] = '\0';

  for (rn = ptree_top (nv->IPV6_STATIC); rn; rn = ptree_next (rn))
    if (rn->info)
      {
        pal_mem_set (&p, 0, sizeof (struct prefix));
        p.prefixlen = rn->key_len;
        NSM_PTREE_KEY_COPY (&p.u.prefix6, rn);

        zsnprintf (buf2, 45, "%Q", &p);
        for (ns = rn->info; ns; ns = ns->next)
          {
            cli_out (cli, "ipv6 route%s %s", buf1, buf2);

            switch (ns->type)
              {
              case NEXTHOP_TYPE_IPV6:
                cli_out (cli, " %R", &ns->gate.ipv6);
                break;
              case NEXTHOP_TYPE_IFNAME:
                cli_out (cli, " %s", ns->ifname);
                break;
              case NEXTHOP_TYPE_IPV6_IFNAME:
                cli_out (cli, " %R %s", &ns->gate.ipv6, ns->ifname);
                break;
              }

            if (ns->distance != APN_DISTANCE_STATIC)
              cli_out (cli, " %d", ns->distance);

            cli_out (cli, "\n");

            write++;
          }
      }
  return write;
}
#endif /* HAVE_IPV6 */

#ifdef HAVE_BFD
/* Write BFD static route configuration. */
int
nsm_config_write_bfd_static (struct cli *cli, struct nsm_vrf *nv)
{
  struct ptree_node *rn;
  struct nsm_static *ns;
  char buf1[NSM_VRF_NAMELEN_MAX];
  char buf2[19];
  int write = 0;
  struct prefix p;

  if (!NSM_CAP_HAVE_VRF && nv->vrf->id != 0)
    return 0;

  if (nv->vrf->id != 0)
    zsnprintf (buf1, NSM_VRF_NAMELEN_MAX, " vrf %s", nv->vrf->name);
  else
    buf1[0] = '\0';

  for (rn = ptree_top (nv->IPV4_STATIC); rn; rn = ptree_next (rn))
    if (rn->info)
      {
        for (ns = rn->info; ns; ns = ns->next)
          {
            if (!CHECK_FLAG (ns->flags, NSM_STATIC_FLAG_BFD_SET) &&
                !CHECK_FLAG (ns->flags, NSM_STATIC_FLAG_BFD_DISABLE_SET))
              continue;
             pal_mem_set (&p, 0, sizeof (struct prefix));
             p.prefixlen = rn->key_len;
             NSM_PTREE_KEY_COPY (&p.u.prefix4, rn);
             zsnprintf (buf2, 19, "%P", &p);
             cli_out (cli, "ip static%s %s", buf1, buf2);
             cli_out (cli, " %r", &ns->gate.ipv4);
             cli_out (cli, " fall-over bfd");
             if (CHECK_FLAG (ns->flags, NSM_STATIC_FLAG_BFD_DISABLE_SET))
               cli_out (cli, " disable");
             cli_out (cli, "\n");
             write++;

           }
       }
  return write;
}
#endif /* HAVE_BFD */

/* Static IP route configuration write function. */
int
nsm_ip_config_write (struct cli *cli)
{
  struct apn_vrf *iv = apn_vrf_lookup_default (cli->vr);
  struct nsm_vrf *nv = NULL;
  int write = 0;
  int i;

  if (!iv)
   return write;
  nv =  iv->proto;
  write += nsm_config_write_router_id (cli, nv);
  write += nsm_multipath_num_write (cli);
  write += nsm_config_write_fib (cli);
  write += nsm_max_static_routes_write (cli);
  write += nsm_max_fib_routes_write (cli);

  if (write)
    cli_out (cli, "!\n");

  /* Write IPv4 static routes.  */
  for (i = 0; i < vector_max (cli->vr->vrf_vec); i++)
    if ((iv = vector_slot (cli->vr->vrf_vec, i)))
      if (IS_APN_VRF_DEFAULT (iv) || NSM_CAP_HAVE_VRF)
        if ((nv = iv->proto))
          write += nsm_config_write_ip_route (cli, nv);

  if (write)
    cli_out (cli, "!\n");

#ifdef HAVE_BFD
  /* Write IPv4 BFD static routes.  */
  for (i = 0; i < vector_max (cli->vr->vrf_vec); i++)
    if ((iv = vector_slot (cli->vr->vrf_vec, i)))
      if (IS_APN_VRF_DEFAULT (iv) || NSM_CAP_HAVE_VRF)
        if ((nv = iv->proto))
          write += nsm_config_write_bfd_static (cli, nv);

  if (write)
    cli_out (cli, "!\n");
#endif /* HVE_BFD */

  /* Write IPv4 static mroutes.  */
  for (i = 0; i < vector_max (cli->vr->vrf_vec); i++)
    if ((iv = vector_slot (cli->vr->vrf_vec, i)))
      if (IS_APN_VRF_DEFAULT (iv) || NSM_CAP_HAVE_VRF)
        if ((nv = iv->proto))
          write += nsm_config_write_ip_mroute (cli, nv, AFI_IP);

  if (write)
    cli_out (cli, "!\n");

#ifdef HAVE_IPV6
  if (NSM_CAP_HAVE_IPV6)
    {
      /* Write IPv6 static mroutes.  */
      for (i = 0; i < vector_max (cli->vr->vrf_vec); i++)
        if ((iv = vector_slot (cli->vr->vrf_vec, i)))
          if (IS_APN_VRF_DEFAULT (iv) || NSM_CAP_HAVE_VRF)
            if ((nv = iv->proto))
              write += nsm_config_write_ipv6_route (cli, nv);

      if (write)
        cli_out (cli, "!\n");

      /* Write IPv6 static mroutes.  */
      for (i = 0; i < vector_max (cli->vr->vrf_vec); i++)
        if ((iv = vector_slot (cli->vr->vrf_vec, i)))
          if (IS_APN_VRF_DEFAULT (iv) || NSM_CAP_HAVE_VRF)
            if ((nv = iv->proto))
              write += nsm_config_write_ip_mroute (cli, nv, AFI_IP6);
    }
#endif /* HAVE_IPV6 */

  return write;
}
#endif /* HAVE_L3 */

#ifdef HAVE_INTEL
CLI (create_ports,
     create_ports_cmd,
     "create-ports BLADE_ID  PORTCOUNT",
     "Create ports",
     "Blade Identifier",
     "Number of ports")
{
  int blade_id;
  int port_count;

  CLI_GET_INTEGER ("blade-id", blade_id, argv[0]);
  CLI_GET_INTEGER ("number of ports", port_count, argv[1]);
  hal_create_ports (blade_id, port_count);
  return CLI_SUCCESS;
}
#endif /* HAVE_INTEL */

#ifdef HAVE_L3
void
nsm_cli_init_route (struct cli_tree *ctree)
{
  cli_install_config (ctree, IP_MODE, nsm_ip_config_write);

#ifdef HAVE_MULTIPATH
  /* "maximum-paths" commands. */
  cli_install_gen (ctree, CONFIG_MODE, PRIVILEGE_NORMAL, 0,
                   &maximum_paths_cmd);
  cli_install_gen (ctree, CONFIG_MODE, PRIVILEGE_NORMAL, 0,
                   &no_maximum_paths_cmd);
  cli_install_gen (ctree, CONFIG_MODE, PRIVILEGE_NORMAL, 0,
                   &no_maximum_paths_num_cmd);
#endif /* HAVE_MULTIPATH */

  /* Maximum static routes commands */
  cli_install_gen (ctree, CONFIG_MODE, PRIVILEGE_MAX, 0,
                   &maximum_static_routes_cmd);
  cli_install_gen (ctree, CONFIG_MODE, PRIVILEGE_MAX, 0,
                   &no_maximum_static_routes_cmd);

   /* Maximum fib routes commands */
   cli_install_gen (ctree, CONFIG_MODE, PRIVILEGE_MAX, 0,
                    &maximum_fib_routes_cmd);
   cli_install_gen (ctree, CONFIG_MODE, PRIVILEGE_MAX, 0,
                    &no_maximum_fib_routes_cmd);

  /* "fib retain" commands. */
  cli_install_gen (ctree, CONFIG_MODE, PRIVILEGE_NORMAL, 0,
                   &nsm_fib_retain_cmd);
  cli_install_gen (ctree, CONFIG_MODE, PRIVILEGE_NORMAL, 0,
                   &no_nsm_fib_retain_cmd);

  /* "clear ip route kernel"  commands. */
  cli_install_gen (ctree, EXEC_MODE, PRIVILEGE_MAX, 0,
                   &clear_ip_route_kernel_cmd);
  cli_install_gen (ctree, EXEC_MODE, PRIVILEGE_MAX, CLI_FLAG_HIDDEN,
                   &clear_ip_kernel_route_cmd);

#ifdef HAVE_IPV6
  /* "clear ipv6 route kernel"  commands. */
  cli_install_gen (ctree, EXEC_MODE, PRIVILEGE_MAX, 0,
                   &clear_ipv6_route_kernel_cmd);
#endif /* HAVE_IPV6 */

  /* "ip route" commands. */
  cli_install_gen (ctree, CONFIG_MODE, PRIVILEGE_NORMAL, 0,
                   &ip_route_prefix_cmd);
  cli_install_gen (ctree, CONFIG_MODE, PRIVILEGE_NORMAL, 0,
                   &ip_route_addr_mask_cmd);
  cli_install_gen (ctree, CONFIG_MODE, PRIVILEGE_NORMAL, 0,
                   &no_ip_route_prefix_cmd);
  cli_install_gen (ctree, CONFIG_MODE, PRIVILEGE_NORMAL, 0,
                   &no_ip_route_addr_mask_cmd);
  cli_install_gen (ctree, CONFIG_MODE, PRIVILEGE_NORMAL, 0,
                   &ip_route_prefix_distance_cmd);
  cli_install_gen (ctree, CONFIG_MODE, PRIVILEGE_NORMAL, 0,
                   &ip_route_addr_mask_distance_cmd);
  cli_install_gen (ctree, CONFIG_MODE, PRIVILEGE_NORMAL, 0,
                   &no_ip_route_prefix_distance_cmd);
  cli_install_gen (ctree, CONFIG_MODE, PRIVILEGE_NORMAL, 0,
                   &no_ip_route_addr_mask_distance_cmd);
#ifdef HAVE_INTEL
  cli_install_gen (ctree, CONFIG_MODE, PRIVILEGE_NORMAL, 0,
                   &create_ports_cmd);
#endif /* HAVE_INTEL */

  /* IPv6 related CLIs are not available in VR-CLI yet. */
#ifdef HAVE_IPV6
  if (NSM_CAP_HAVE_IPV6)
    {
      /* "ipv6 route" commands. */
      cli_install_gen (ctree, CONFIG_MODE, PRIVILEGE_NORMAL, 0,
                       &ipv6_route_prefix_cmd);
      cli_install_gen (ctree, CONFIG_MODE, PRIVILEGE_NORMAL, 0,
                       &ipv6_route_prefix_distance_cmd);
      cli_install_gen (ctree, CONFIG_MODE, PRIVILEGE_NORMAL, 0,
                       &ipv6_route_prefix_ifname_cmd);
      cli_install_gen (ctree, CONFIG_MODE, PRIVILEGE_NORMAL, 0,
                       &ipv6_route_prefix_ifname_distance_cmd);
      cli_install_gen (ctree, CONFIG_MODE, PRIVILEGE_NORMAL, 0,
                       &no_ipv6_route_prefix_cmd);
      cli_install_gen (ctree, CONFIG_MODE, PRIVILEGE_NORMAL, 0,
                       &no_ipv6_route_prefix_distance_cmd);
      cli_install_gen (ctree, CONFIG_MODE, PRIVILEGE_NORMAL, 0,
                       &no_ipv6_route_prefix_ifname_cmd);
      cli_install_gen (ctree, CONFIG_MODE, PRIVILEGE_NORMAL, 0,
                       &no_ipv6_route_prefix_ifname_distance_cmd);
      cli_install_gen (ctree, CONFIG_MODE, PRIVILEGE_NORMAL, 0,
                       &no_ipv6_route_cmd);
    }
#endif /* HAVE_IPV6 */

  /* VRF. */
#ifdef HAVE_VRF
  cli_install_gen (ctree, CONFIG_MODE, PRIVILEGE_NORMAL, 0,
                   &ip_vrf_name_cmd);
  cli_install_gen (ctree, CONFIG_MODE, PRIVILEGE_NORMAL, 0,
                   &no_ip_vrf_name_cmd);

  cli_install_gen (ctree, CONFIG_MODE, PRIVILEGE_NORMAL, 0,
                   &ip_route_vrf_ifname_cmd);
  cli_install_gen (ctree, CONFIG_MODE, PRIVILEGE_NORMAL, 0,
                   &ip_route_vrf_ifname_tag_desc_cmd);
  cli_install_gen (ctree, CONFIG_MODE, PRIVILEGE_NORMAL, 0,
                   &no_ip_route_vrf_ifname_cmd);
  cli_install_gen (ctree, CONFIG_MODE, PRIVILEGE_NORMAL, 0,
                   &no_ip_route_vrf_ifname_tag_desc_cmd);
  cli_install_gen (ctree, CONFIG_MODE, PRIVILEGE_NORMAL, 0,
                   &ip_route_vrf_cmd);
  cli_install_gen (ctree, CONFIG_MODE, PRIVILEGE_NORMAL, 0,
                   &ip_route_vrf_tag_desc_cmd);
  cli_install_gen (ctree, CONFIG_MODE, PRIVILEGE_NORMAL, 0,
                   &no_ip_route_vrf_tag_desc_cmd);
  cli_install_gen (ctree, CONFIG_MODE, PRIVILEGE_NORMAL, 0,
                   &no_ip_route_vrf_cmd);
#ifdef HAVE_IPV6 /*6VPE*/
  cli_install_gen (ctree, CONFIG_MODE, PRIVILEGE_NORMAL, 0,
                   &ipv6_route_vrf_ifname_cmd);
  cli_install_gen (ctree, CONFIG_MODE, PRIVILEGE_NORMAL, 0,
                   &no_ipv6_route_vrf_ifname_cmd);
#endif /*HAVE_IPV6*/
#endif /* HAVE_VRF */
}


CLI (nsm_router_id,
     nsm_router_id_cmd,
     "router-id A.B.C.D",
     "Router identifier for this system",
     "Router identifier in IP address format")
{
  int ret;
  struct pal_in4_addr router_id;

  CLI_GET_IPV4_ADDRESS ("router ID", router_id, argv[0]);

  if (cli->mode == CONFIG_MODE)
    ret = nsm_router_id_set (cli->vr->id, VRF_ID_MAIN, &router_id);
  else
    {
      struct nsm_vrf *nv = cli->index;

      ret = nsm_router_id_set (cli->vr->id, nv->vrf->id, &router_id);
    }

  return nsm_cli_return (cli, ret);
}

CLI (no_nsm_router_id,
     no_nsm_router_id_cmd,
     "no router-id (A.B.C.D|)",
     CLI_NO_STR,
     "Router identifier for this system",
     "Router identifier in IP address format")
{
  int ret;

  if (cli->mode == CONFIG_MODE)
    ret = nsm_router_id_unset (cli->vr->id, VRF_ID_MAIN);
  else
    {
      struct nsm_vrf *nv = cli->index;

      ret = nsm_router_id_unset (cli->vr->id, nv->vrf->id);
    }

  return nsm_cli_return (cli, ret);
}

CLI (ip_forwarding,
     ip_forwarding_cmd,
     "ip forwarding",
     CLI_IP_STR,
     "Turn on IP forwarding")
{
  int ret;

  ret = nsm_ipv4_forwarding_set (cli->vr->id);

  return nsm_cli_return (cli, ret);
}

CLI (no_ip_forwarding,
     no_ip_forwarding_cmd,
     "no ip forwarding",
     CLI_NO_STR,
     CLI_IP_STR,
     "Turn off IP forwarding")
{
  int ret;

  ret = nsm_ipv4_forwarding_unset (cli->vr->id);

  return nsm_cli_return (cli, ret);
}


#ifdef HAVE_IPV6
CLI (ipv6_forwarding,
     ipv6_forwarding_cmd,
     "ipv6 forwarding",
     CLI_IPV6_STR,
     "Forward IPv6 protocol packet")
{
  int ret;

  ret = nsm_ipv6_forwarding_set (cli->vr->id);

  return nsm_cli_return (cli, ret);
}

CLI (no_ipv6_forwarding,
     no_ipv6_forwarding_cmd,
     "no ipv6 forwarding",
     CLI_NO_STR,
     CLI_IPV6_STR,
     "Doesn't forward IPv6 protocol packet")
{
  int ret;

  ret = nsm_ipv6_forwarding_unset (cli->vr->id);

  return nsm_cli_return (cli, ret);
}
#endif /* HAVE_IPV6 */

void
nsm_cli_init_router (struct cli_tree *ctree)
{
  /* "router-id" CLIs.  */
  cli_install_gen (ctree, CONFIG_MODE, PRIVILEGE_VR_MAX, 0,
                   &nsm_router_id_cmd);
  cli_install_gen (ctree, CONFIG_MODE, PRIVILEGE_VR_MAX, 0,
                   &no_nsm_router_id_cmd);
  cli_install_gen (ctree, CONFIG_MODE, PRIVILEGE_NORMAL, 0,
                   &ip_forwarding_cmd);
  cli_install_gen (ctree, CONFIG_MODE, PRIVILEGE_NORMAL, 0,
                   &no_ip_forwarding_cmd);

#ifdef HAVE_IPV6
  if (NSM_CAP_HAVE_IPV6)
    {
      /* Not available for VR-CLI yet. */
      cli_install_gen (ctree, CONFIG_MODE, PRIVILEGE_NORMAL, 0,
                       &ipv6_forwarding_cmd);
      cli_install_gen (ctree, CONFIG_MODE, PRIVILEGE_NORMAL, 0,
                       &no_ipv6_forwarding_cmd);
    }
#endif /* HAVE_IPV6 */
}
#endif /* HAVE_L3 */


#ifdef HAVE_VRF
int
nsm_config_write_vrf (struct cli *cli)
{
  struct apn_vr *vr = cli->vr;
  struct apn_vrf *iv;
  struct nsm_vrf *nv;
  int write = 0;
  int i;

  for (i = 1; i < vector_max (vr->vrf_vec); i++)
    if ((iv = vector_slot (vr->vrf_vec, i)))
      if ((nv = iv->proto))
        {
          write++;

          cli_out(cli, "ip vrf %s\n", iv->name);

          if (nv->desc)
            cli_out (cli, " description %s\n", nv->desc);

          /* router-id.  */
          nsm_config_write_router_id (cli, nv);
        }

  if (write)
    cli_out(cli, "!\n");

  return write;
}

/* Initialize VRF CLIs.  */
void
nsm_cli_init_vrf (struct cli_tree *ctree)
{
  cli_install_default (ctree, VRF_MODE);
  cli_install_config (ctree, VRF_MODE, nsm_config_write_vrf);

  cli_install_gen (ctree, VRF_MODE, PRIVILEGE_VR_MAX, 0, &vrf_desc_cmd);
  cli_install_gen (ctree, VRF_MODE, PRIVILEGE_VR_MAX, 0, &no_vrf_desc_cmd);

  /* "router-id" CLIs.  */
  cli_install_gen (ctree, VRF_MODE, PRIVILEGE_VR_MAX, 0,
                   &nsm_router_id_cmd);
  cli_install_gen (ctree, VRF_MODE, PRIVILEGE_VR_MAX, 0,
                   &no_nsm_router_id_cmd);

  return;
}
#endif /* HAVE_VRF */

/* Router Advertisement CLIs. */
#ifdef HAVE_RTADV
CLI (ipv6_nd_suppress_ra,
     ipv6_nd_suppress_ra_cmd,
     "ipv6 nd suppress-ra",
     CLI_IPV6_STR,
     CLI_ND_STR,
     "Suppress IPv6 Router Advertisements")
{
  int ret;
  struct interface *ifp = cli->index;

#ifdef HAVE_LACPD
  NSM_INTERFACE_CHECK_AGG_MEM_PROPERTY(cli, ifp);
#endif /* HAVE_LACPD */
  ret = nsm_rtadv_ra_unset (cli->vr->id, ifp->name);

  return nsm_cli_return (cli, ret);
}

CLI (no_ipv6_nd_suppress_ra,
     no_ipv6_nd_suppress_ra_cmd,
     "no ipv6 nd suppress-ra",
     CLI_NO_STR,
     CLI_IPV6_STR,
     CLI_ND_STR,
     "Suppress IPv6 Router Advertisements")
{
  int ret;
  struct interface *ifp = cli->index;

#ifdef HAVE_LACPD
  NSM_INTERFACE_CHECK_AGG_MEM_PROPERTY(cli, ifp);
#endif /* HAVE_LACPD */
  ret = nsm_rtadv_ra_set (cli->vr->id, ifp->name);

  return nsm_cli_return (cli, ret);
}

CLI (ipv6_nd_ra_interval,
     ipv6_nd_ra_interval_cmd,
     "ipv6 nd ra-interval <4-1800>",
     CLI_IPV6_STR,
     CLI_ND_STR,
     "Set IPv6 Router Advertisement Interval",
     "RA interval (sec)")
{
  int ret;
  u_int32_t interval;
  struct interface *ifp = cli->index;

#ifdef HAVE_LACPD
  NSM_INTERFACE_CHECK_AGG_MEM_PROPERTY(cli, ifp);
#endif /* HAVE_LACPD */
  CLI_GET_UINT32_RANGE ("RA interval", interval, argv[0], 4, 1800);

  ret = nsm_rtadv_ra_interval_set (cli->vr->id, ifp->name, interval);

  return nsm_cli_return (cli, ret);
}

CLI (no_ipv6_nd_ra_interval,
     no_ipv6_nd_ra_interval_cmd,
     "no ipv6 nd ra-interval",
     CLI_NO_STR,
     CLI_IPV6_STR,
     CLI_ND_STR,
     "Set IPv6 Router Advertisement Interval")
{
  int ret;
  struct interface *ifp = cli->index;

#ifdef HAVE_LACPD
  NSM_INTERFACE_CHECK_AGG_MEM_PROPERTY(cli, ifp);
#endif /* HAVE_LACPD */
  ret = nsm_rtadv_ra_interval_unset (cli->vr->id, ifp->name);

  return nsm_cli_return (cli, ret);
}

CLI (ipv6_nd_ra_lifetime,
     ipv6_nd_ra_lifetime_cmd,
     "ipv6 nd ra-lifetime <0-9000>",
     CLI_IPV6_STR,
     CLI_ND_STR,
     "Set IPv6 Router Advertisement Lifetime",
     "RA Lifetime (seconds)")
{
  int ret;
  int lifetime;
  struct interface *ifp = cli->index;

#ifdef HAVE_LACPD
  NSM_INTERFACE_CHECK_AGG_MEM_PROPERTY(cli, ifp);
#endif /* HAVE_LACPD */
  CLI_GET_INTEGER_RANGE ("router lifetime", lifetime, argv[0], 0, 9000);

  ret = nsm_rtadv_router_lifetime_set (cli->vr->id, ifp->name, lifetime);

  return nsm_cli_return (cli, ret);
}

CLI (no_ipv6_nd_ra_lifetime,
     no_ipv6_nd_ra_lifetime_cmd,
     "no ipv6 nd ra-lifetime",
     CLI_NO_STR,
     CLI_IPV6_STR,
     CLI_ND_STR,
     "Set IPv6 Router Advertisement Lifetime")
{
  int ret;
  struct interface *ifp = cli->index;

#ifdef HAVE_LACPD
  NSM_INTERFACE_CHECK_AGG_MEM_PROPERTY(cli, ifp);
#endif /* HAVE_LACPD */
  ret = nsm_rtadv_router_lifetime_unset (cli->vr->id, ifp->name);

  return nsm_cli_return (cli, ret);
}

CLI (ipv6_nd_reachable_time,
     ipv6_nd_reachable_time_cmd,
     "ipv6 nd reachable-time <0-3600000>",
     CLI_IPV6_STR,
     CLI_ND_STR,
     "Set advertised reachability time",
     "Reachablity time in milliseconds")
{
  int ret;
  s_int32_t reachtime;
  struct interface *ifp = cli->index;

#ifdef HAVE_LACPD
  NSM_INTERFACE_CHECK_AGG_MEM_PROPERTY(cli, ifp);
#endif /* HAVE_LACPD */
  CLI_GET_INTEGER_RANGE ("router reachable time", reachtime, argv[0],
                         0, RTADV_MAX_REACHABLE_TIME);

  ret = nsm_rtadv_router_reachable_time_set (cli->vr->id, ifp->name,
                                             reachtime);

  return nsm_cli_return (cli, ret);
}

CLI (no_ipv6_nd_reachable_time,
     no_ipv6_nd_reachable_time_cmd,
     "no ipv6 nd reachable-time",
     CLI_NO_STR,
     CLI_IPV6_STR,
     CLI_ND_STR,
     "Set advertised reachability time")
{
  int ret;
  struct interface *ifp = cli->index;

#ifdef HAVE_LACPD
  NSM_INTERFACE_CHECK_AGG_MEM_PROPERTY(cli, ifp);
#endif /* HAVE_LACPD */
  ret = nsm_rtadv_router_reachable_time_unset (cli->vr->id, ifp->name);

  return nsm_cli_return (cli, ret);
}

CLI (ipv6_nd_managed_config_flag,
     ipv6_nd_managed_config_flag_cmd,
     "ipv6 nd managed-config-flag",
     CLI_IPV6_STR,
     CLI_ND_STR,
     "Hosts should use DHCP for address config")
{
  int ret;
  struct interface *ifp = cli->index;

#ifdef HAVE_LACPD
  NSM_INTERFACE_CHECK_AGG_MEM_PROPERTY(cli, ifp);
#endif /* HAVE_LACPD */
  ret = nsm_rtadv_ra_managed_config_flag_set (cli->vr->id, ifp->name);

  return nsm_cli_return (cli, ret);
}

CLI (no_ipv6_nd_managed_config_flag,
     no_ipv6_nd_managed_config_flag_cmd,
     "no ipv6 nd managed-config-flag",
     CLI_NO_STR,
     CLI_IPV6_STR,
     CLI_ND_STR,
     "Hosts should use DHCP for address config")
{
  int ret;
  struct interface *ifp = cli->index;

#ifdef HAVE_LACPD
  NSM_INTERFACE_CHECK_AGG_MEM_PROPERTY(cli, ifp);
#endif /* HAVE_LACPD */
  ret = nsm_rtadv_ra_managed_config_flag_unset (cli->vr->id, ifp->name);

  return nsm_cli_return (cli, ret);
}

CLI (ipv6_nd_other_config_flag,
     ipv6_nd_other_config_flag_cmd,
     "ipv6 nd other-config-flag",
     CLI_IPV6_STR,
     CLI_ND_STR,
     "Hosts should use DHCP for non-address config")
{
  int ret;
  struct interface *ifp = cli->index;

#ifdef HAVE_LACPD
  NSM_INTERFACE_CHECK_AGG_MEM_PROPERTY(cli, ifp);
#endif /* HAVE_LACPD */
  ret = nsm_rtadv_ra_other_config_flag_set (cli->vr->id, ifp->name);

  return nsm_cli_return (cli, ret);
}

CLI (no_ipv6_nd_other_config_flag,
     no_ipv6_nd_other_config_flag_cmd,
     "no ipv6 nd other-config-flag",
     CLI_NO_STR,
     CLI_IPV6_STR,
     CLI_ND_STR,
     "Hosts should use DHCP for non-address config")
{
  int ret;
  struct interface *ifp = cli->index;

#ifdef HAVE_LACPD
  NSM_INTERFACE_CHECK_AGG_MEM_PROPERTY(cli, ifp);
#endif /* HAVE_LACPD */
  ret = nsm_rtadv_ra_other_config_flag_unset (cli->vr->id, ifp->name);

  return nsm_cli_return (cli, ret);
}

CLI (ipv6_nd_prefix,
     ipv6_nd_prefix_val_cmd,
     "ipv6 nd prefix X:X::X:X/M <0-4294967295> <0-4294967295> (off-link|) (no-autoconfig|)",
     CLI_IPV6_STR,
     CLI_ND_STR,
     "Configure IPv6 Routing Prefix Advertisement",
     "IPv6 prefix",
     "Valid lifetime (secs)",
     "Preferred lifetime (secs)",
     "Do not use prefix for onlink determination",
     "Do not use prefix for autoconfiguration")
{
  int i;
  int ret;
  struct prefix_ipv6 p;
  struct interface *ifp = cli->index;
  u_int32_t vlifetime = 0;
  u_int32_t plifetime = 0;
  u_char flags = (PAL_ND_OPT_PI_FLAG_ONLINK|PAL_ND_OPT_PI_FLAG_AUTO);

#ifdef HAVE_LACPD
  NSM_INTERFACE_CHECK_AGG_MEM_PROPERTY(cli, ifp);
#endif /* HAVE_LACPD */
  CLI_GET_IPV6_PREFIX ("IPv6 prefix", p, argv[0]);

  if (argc > 1)
    CLI_GET_UINT32 ("IPv6 prefix valid lifetime", vlifetime, argv[1]);

  if (argc > 2)
    CLI_GET_UINT32_RANGE ("IPv6 prefix preferred lifetime",
                           plifetime, argv[2], 0, vlifetime);

  if (argc > 3)
    for (i = 3; i < argc; i++)
      {
        if (pal_strncmp ("o", argv[i], 1) == 0)
          UNSET_FLAG (flags, PAL_ND_OPT_PI_FLAG_ONLINK);
        else if (pal_strncmp ("n", argv[i], 1) == 0)
          UNSET_FLAG (flags, PAL_ND_OPT_PI_FLAG_AUTO);
      }

  ret = nsm_rtadv_ra_prefix_set (cli->vr->id, ifp->name, &p.prefix,
                                 p.prefixlen, vlifetime, plifetime, flags);

  return nsm_cli_return (cli, ret);
}

ALI (ipv6_nd_prefix,
     ipv6_nd_prefix_cmd,
     "ipv6 nd prefix X:X::X:X/M",
     CLI_IPV6_STR,
     CLI_ND_STR,
     "Configure IPv6 Routing Prefix Advertisement",
     "IPv6 prefix");

CLI (no_ipv6_nd_prefix,
     no_ipv6_nd_prefix_cmd,
     "no ipv6 nd prefix X:X::X:X/M",
     CLI_NO_STR,
     CLI_IPV6_STR,
     CLI_ND_STR,
     "Prefix information",
     "IPv6 prefix")
{
  int ret;
  struct prefix_ipv6 p;
  struct interface *ifp = cli->index;

#ifdef HAVE_LACPD
  NSM_INTERFACE_CHECK_AGG_MEM_PROPERTY(cli, ifp);
#endif /* HAVE_LACPD */
  CLI_GET_IPV6_PREFIX ("IPv6 prefix", p, argv[0]);

  ret = nsm_rtadv_ra_prefix_unset (cli->vr->id, ifp->name,
                                   &p.prefix, p.prefixlen);

  return nsm_cli_return (cli, ret);
}

CLI (ipv6_nd_retrans_timer,
     ipv6_nd_retrans_timer_cmd,
     "ipv6 nd retransmission-time <1000-3600000>",
     CLI_IPV6_STR,
     CLI_ND_STR,
     "Set advertised retransmission timer",
     "Retransmission time in milliseconds")
{
  u_int32_t retrans_timer;
  struct interface *ifp;
  int ret;
  /* Set ifp */
 ifp = cli->index;
#ifdef HAVE_LACPD
  NSM_INTERFACE_CHECK_AGG_MEM_PROPERTY (cli, ifp);
#endif /* HAVE_LACPD */
 CLI_GET_UINT32_RANGE ("Router advertisement retransmission time",
                       retrans_timer, argv[0],
                       RTADV_MIN_RETRANS_TIMER, RTADV_MAX_RETRANS_TIMER);
  ret = nsm_rtadv_router_retrans_timer_set (cli->vr->id, ifp->name, retrans_timer);
  return nsm_cli_return (cli, ret);
}

CLI (no_ipv6_nd_retrans_timer,
     no_ipv6_nd_retrans_timer_cmd,
     "no ipv6 nd retransmission-time (<1000-3600000>|)",
     CLI_NO_STR,
     CLI_IPV6_STR,
     CLI_ND_STR,
     "Set advertised retransmission time",
     "Retransmission time in milliseconds")
{
  struct interface *ifp;
  int ret;
  /* Set ifp */
  ifp = cli->index;
#ifdef HAVE_LACPD
  NSM_INTERFACE_CHECK_AGG_MEM_PROPERTY(cli, ifp);
#endif /* HAVE_LACPD */
  ret = nsm_rtadv_router_retrans_timer_unset (cli->vr->id, ifp->name);
  return nsm_cli_return (cli, ret);
}

CLI (ipv6_nd_curr_hoplimit,
     ipv6_nd_curr_hoplimit_cmd,
     "ipv6 nd current-hoplimit <0-255>",
     CLI_IPV6_STR,
     CLI_ND_STR,
     "Set advertised current hop limit",
     "Current hop limit value")
{
  int ret, curr_hoplimit;
  struct interface *ifp;
  /* Set ifp */
  ifp = cli->index;
#ifdef HAVE_LACPD
  NSM_INTERFACE_CHECK_AGG_MEM_PROPERTY(cli, ifp);
#endif /* HAVE_LACPD */
  CLI_GET_INTEGER_RANGE ("Router advertisement current hop limit", curr_hoplimit, argv[0],
                RTADV_MIN_CUR_HOPLIMIT, RTADV_MAX_CUR_HOPLIMIT);
  ret = nsm_rtadv_curr_hoplimit_set (cli->vr->id, ifp->name, curr_hoplimit);
  return nsm_cli_return (cli, ret);
}

CLI (no_ipv6_nd_curr_hoplimit,
     no_ipv6_nd_curr_hoplimit_cmd,
     "no ipv6 nd current-hoplimit (<0-255>|)",
     CLI_NO_STR,
     CLI_IPV6_STR,
     CLI_ND_STR,
     "Set advertised current hop limit",
     "Current hop limit value")
{
  struct interface *ifp;
  int ret;
  /* Set ifp */
  ifp = cli->index;
#ifdef HAVE_LACPD
  NSM_INTERFACE_CHECK_AGG_MEM_PROPERTY(cli, ifp);
#endif /* HAVE_LACPD */
  ret = nsm_rtadv_curr_hoplimit_unset (cli->vr->id, ifp->name);
  return nsm_cli_return (cli, ret);
}

CLI (ipv6_nd_linkmtu,
     ipv6_nd_linkmtu_cmd,
     "ipv6 nd link-mtu",
     CLI_IPV6_STR,
     CLI_ND_STR,
     "Set advertised link-mtu option")
{
  struct interface *ifp;
  int ret;
  /* Set ifp */
  ifp = cli->index;
#ifdef HAVE_LACPD
  NSM_INTERFACE_CHECK_AGG_MEM_PROPERTY(cli, ifp);
#endif /* HAVE_LACPD */
  ret = nsm_rtadv_linkmtu_set (cli->vr->id, ifp->name);
  return nsm_cli_return (cli, ret);
}

CLI (no_ipv6_nd_linkmtu,
     no_ipv6_nd_linkmtu_cmd,
     "no ipv6 nd link-mtu (default|)",
     CLI_NO_STR,
     CLI_IPV6_STR,
     CLI_ND_STR,
     "Set advertised link-mtu option",
     "Default option parameter")
{
  struct interface *ifp;
  int ret;
  /* Set ifp */
  ifp = cli->index;
#ifdef HAVE_LACPD
  NSM_INTERFACE_CHECK_AGG_MEM_PROPERTY(cli, ifp);
#endif /* HAVE_LACPD */
  ret = nsm_rtadv_linkmtu_unset (cli->vr->id, ifp->name);
  return nsm_cli_return (cli, ret);
}

CLI (ipv6_nd_min_ra_interval,
     ipv6_nd_min_ra_interval_cmd,
     "ipv6 nd minimum-ra-interval <3-1350>",
     CLI_IPV6_STR,
     CLI_ND_STR,
     "Set IPv6 minimum router advertisement interval",
     "Minimum router advertisement interval (sec)")
{
  u_int32_t min_interval;
  struct interface *ifp;
  int ret;
  /* Set ifp */
  ifp = cli->index;
#ifdef HAVE_LACPD
  NSM_INTERFACE_CHECK_AGG_MEM_PROPERTY(cli, ifp);
#endif /* HAVE_LACPD */
  CLI_GET_INTEGER_RANGE ("Minimum router advertisement interval", min_interval, argv[0], 3, 1350);
  ret = nsm_rtadv_min_ra_interval_set (cli->vr->id, ifp->name, min_interval);
  return nsm_cli_return (cli, ret);
}

CLI (no_ipv6_nd_min_ra_interval,
     no_ipv6_nd_min_ra_interval_cmd,
     "no ipv6 nd minimum-ra-interval (<3-1350>|)",
     CLI_NO_STR,
     CLI_IPV6_STR,
     CLI_ND_STR,
     "Set IPv6 minimum router advertisement interval",
     "Minimum router advertisement interval (sec)")
{
  struct interface *ifp;
  int ret;
  /* Set ifp */
  ifp = cli->index;
#ifdef HAVE_LACPD
  NSM_INTERFACE_CHECK_AGG_MEM_PROPERTY(cli, ifp);
#endif /* HAVE_LACPD */
  ret = nsm_rtadv_min_ra_interval_unset (cli->vr->id, ifp->name);
  return nsm_cli_return (cli, ret);
}

CLI (ipv6_nd_prefix_valid_lifetime,
     ipv6_nd_prefix_valid_lifetime_cmd,
     "ipv6 nd prefix valid-lifetime <0-4294967295>",
     CLI_IPV6_STR,
     CLI_ND_STR,
     "Set IPv6 prefix list option",
     "Set IPv6 prefix valid lifetime",
     "Valid lifetime value (sec)")
{
  u_int32_t valid_lifetime;
  struct interface *ifp;
  int ret;
  /* Set ifp */
  ifp = cli->index;
#ifdef HAVE_LACPD
  NSM_INTERFACE_CHECK_AGG_MEM_PROPERTY(cli, ifp);
#endif /* HAVE_LACPD */
  CLI_GET_UINT32("Prefix valid lifetime value", valid_lifetime, argv[0]);
  ret = nsm_rtadv_prefix_valid_lifetime_set (cli->vr->id, ifp->name, valid_lifetime);
  return nsm_cli_return (cli, ret);
}

CLI (no_ipv6_nd_prefix_valid_lifetime,
     no_ipv6_nd_prefix_valid_lifetime_cmd,
     "no ipv6 nd prefix valid-lifetime (<0-4294967295>|)",
     CLI_NO_STR,
     CLI_IPV6_STR,
     CLI_ND_STR,
     "Set IPv6 Prefix List Option",
     "Set IPv6 Prefix Valid Lifetime",
     "Valid Lifetime value (sec)")
{
  struct interface *ifp;
  int ret;
  /* Set ifp */
  ifp = cli->index;
#ifdef HAVE_LACPD
  NSM_INTERFACE_CHECK_AGG_MEM_PROPERTY(cli, ifp);
#endif /* HAVE_LACPD */
  ret = nsm_rtadv_prefix_valid_lifetime_unset (cli->vr->id, ifp->name);
  return nsm_cli_return (cli, ret);
}

CLI (ipv6_nd_prefix_preferred_lifetime,
     ipv6_nd_prefix_preferred_lifetime_cmd,
     "ipv6 nd prefix preferred-lifetime <0-4294967295>",
     CLI_IPV6_STR,
     CLI_ND_STR,
     "Set IPv6 prefix list option",
     "Set IPv6 prefix preferred lifetime",
     "Preferred lifetime value (sec)")
{
  u_int32_t preferred_lifetime;
  struct interface *ifp;
  int ret;
  /* Set ifp */
  ifp = cli->index;
#ifdef HAVE_LACPD
  NSM_INTERFACE_CHECK_AGG_MEM_PROPERTY(cli, ifp);
#endif /* HAVE_LACPD */
  CLI_GET_UINT32 ("Prefix valid lifetime value", preferred_lifetime, argv[0]);
  ret = nsm_rtadv_prefix_preferred_lifetime_set (cli->vr->id, ifp->name, preferred_lifetime);
  return nsm_cli_return (cli, ret);
}

CLI (no_ipv6_nd_prefix_preferred_lifetime,
     no_ipv6_nd_prefix_preferred_lifetime_cmd,
     "no ipv6 nd prefix preferred-lifetime (<0-4294967295>|)",
     CLI_NO_STR,
     CLI_IPV6_STR,
     CLI_ND_STR,
     "Set IPv6 prefix list option",
     "Set IPv6 prefix preferred lifetime",
     "Valid lifetime value (sec)")
{
  struct interface *ifp;
  int ret;
  /* Set ifp */
  ifp = cli->index;
#ifdef HAVE_LACPD
  NSM_INTERFACE_CHECK_AGG_MEM_PROPERTY(cli, ifp);
#endif /* HAVE_LACPD */
  ret = nsm_rtadv_prefix_preferred_lifetime_unset (cli->vr->id, ifp->name);
  return nsm_cli_return (cli, ret);
}

CLI (ipv6_nd_prefix_flag_offlink,
     ipv6_nd_prefix_flag_offlink_cmd,
     "ipv6 nd prefix offlink",
     CLI_IPV6_STR,
     CLI_ND_STR,
     "Set IPv6 prefix list option",
     "Prefix offlink flag")
{
  struct interface *ifp;
  int ret;
  /* Set ifp */
  ifp = cli->index;
#ifdef HAVE_LACPD
  NSM_INTERFACE_CHECK_AGG_MEM_PROPERTY(cli, ifp);
#endif /* HAVE_LACPD */
  ret = nsm_rtadv_prefix_offlink_set (cli->vr->id, ifp->name);
  return nsm_cli_return (cli, ret);
}

CLI (no_ipv6_nd_prefix_flag_offlink,
     no_ipv6_nd_prefix_flag_offlink_cmd,
     "no ipv6 nd prefix offlink",
     CLI_NO_STR,
     CLI_IPV6_STR,
     CLI_ND_STR,
     "Set IPv6 prefix list option",
     "Prefix offlink flag")
{
  struct interface *ifp;
  int ret;
  /* Set ifp */
  ifp = cli->index;
#ifdef HAVE_LACPD
  NSM_INTERFACE_CHECK_AGG_MEM_PROPERTY(cli, ifp);
#endif /* HAVE_LACPD */
  ret = nsm_rtadv_prefix_offlink_unset (cli->vr->id, ifp->name);
  return nsm_cli_return (cli, ret);
}

CLI (ipv6_nd_prefix_flag_noautoconf,
     ipv6_nd_prefix_flag_noautoconf_cmd,
     "ipv6 nd prefix no-autoconf",
     CLI_IPV6_STR,
     CLI_ND_STR,
     "Set IPv6 prefix list option",
     "Prefix no autoconf flag")
{
  struct interface *ifp;
  int ret;
  /* Set ifp */
  ifp = cli->index;
#ifdef HAVE_LACPD
  NSM_INTERFACE_CHECK_AGG_MEM_PROPERTY(cli, ifp);
#endif /* HAVE_LACPD */
  ret = nsm_rtadv_prefix_noautoconf_set (cli->vr->id, ifp->name);
  return nsm_cli_return (cli, ret);
}

CLI (no_ipv6_nd_prefix_flag_noautoconf,
     no_ipv6_nd_prefix_flag_noautoconf_cmd,
     "no ipv6 nd prefix no-autoconf",
     CLI_NO_STR,
     CLI_IPV6_STR,
     CLI_ND_STR,
     "Set IPv6 prefix list option",
     "Prefix no autoconf flag")
{
  struct interface *ifp;
  int ret;
  /* Set ifp */
  ifp = cli->index;
#ifdef HAVE_LACPD
  NSM_INTERFACE_CHECK_AGG_MEM_PROPERTY(cli, ifp);
#endif
  ret = nsm_rtadv_prefix_noautoconf_unset (cli->vr->id, ifp->name);
  return nsm_cli_return (cli, ret);
}

#ifdef HAVE_MIP6
CLI (ipv6_mobile_home_agent,
     ipv6_mobile_home_agent_cmd,
     "ipv6 mobile home-agent",
     CLI_IPV6_STR,
     CLI_MIP_STR,
     "Home agent flag")
{
  int ret;
  struct interface *ifp = cli->index;

#ifdef HAVE_LACPD
  NSM_INTERFACE_CHECK_AGG_MEM_PROPERTY(cli, ifp);
#endif /* HAVE_LACPD */
  ret = nsm_rtadv_mip6_ha_set (cli->vr->id, ifp->name);

  return nsm_cli_return (cli, ret);
}

CLI (no_ipv6_mobile_home_agent,
     no_ipv6_mobile_home_agent_cmd,
     "no ipv6 mobile home-agent",
     CLI_NO_STR,
     CLI_IPV6_STR,
     CLI_MIP_STR,
     "Home agent flag")
{
  int ret;
  struct interface *ifp = cli->index;

#ifdef HAVE_LACPD
  NSM_INTERFACE_CHECK_AGG_MEM_PROPERTY(cli, ifp);
#endif /* HAVE_LACPD */
  ret = nsm_rtadv_mip6_ha_unset (cli->vr->id, ifp->name);

  return nsm_cli_return (cli, ret);
}

CLI (ipv6_mobile_home_agent_info,
     ipv6_mobile_home_agent_info_cmd,
     "ipv6 mobile home-agent-info <0-65535> <0-65535>",
     CLI_IPV6_STR,
     CLI_MIP_STR,
     "Home agent information option",
     "Home agent preference",
     "Home agent lifetime (secs)")
{
  int ret;
  int pref;
  int lifetime;
  struct interface *ifp = cli->index;

#ifdef HAVE_LACPD
  NSM_INTERFACE_CHECK_AGG_MEM_PROPERTY(cli, ifp);
#endif /* HAVE_LACPD */
  /* Get home agent preference. */
  CLI_GET_INTEGER_RANGE ("home agent preference", pref, argv[0], 0, 65535);

  /* Get home agent lifetime. */
  CLI_GET_INTEGER_RANGE ("home agent lifetime", lifetime, argv[1], 0, 65535);

  ret = nsm_rtadv_mip6_ha_info_set (cli->vr->id, ifp->name, pref, lifetime);

  return nsm_cli_return (cli, ret);
}

CLI (no_ipv6_mobile_home_agent_info,
     no_ipv6_mobile_home_agent_info_cmd,
     "no ipv6 mobile home-agent-info",
     CLI_NO_STR,
     CLI_IPV6_STR,
     CLI_MIP_STR,
     "Home agent information option")
{
  int ret;
  struct interface *ifp = cli->index;

#ifdef HAVE_LACPD
  NSM_INTERFACE_CHECK_AGG_MEM_PROPERTY(cli, ifp);
#endif /* HAVE_LACPD */
  ret = nsm_rtadv_mip6_ha_info_unset (cli->vr->id, ifp->name);

  return nsm_cli_return (cli, ret);
}

CLI (ipv6_mobile_adv_interval,
     ipv6_mobile_adv_interval_cmd,
     "ipv6 mobile adv-interval",
     CLI_IPV6_STR,
     CLI_MIP_STR,
     "Advertisement interval option")
{
  int ret;
  struct interface *ifp = cli->index;

#ifdef HAVE_LACPD
  NSM_INTERFACE_CHECK_AGG_MEM_PROPERTY(cli, ifp);
#endif /* HAVE_LACPD */
  ret = nsm_rtadv_mip6_adv_interval_set (cli->vr->id, ifp->name);

  return nsm_cli_return (cli, ret);
}

CLI (no_ipv6_mobile_adv_interval,
     no_ipv6_mobile_adv_interval_cmd,
     "no ipv6 mobile adv-interval",
     CLI_NO_STR,
     CLI_IPV6_STR,
     CLI_MIP_STR,
     "Advertisement interval option")
{
  int ret;
  struct interface *ifp = cli->index;

#ifdef HAVE_LACPD
  NSM_INTERFACE_CHECK_AGG_MEM_PROPERTY(cli, ifp);
#endif /* HAVE_LACPD */
  ret = nsm_rtadv_mip6_adv_interval_unset (cli->vr->id, ifp->name);

  return nsm_cli_return (cli, ret);
}

CLI (ipv6_mobile_home_agent_addr,
     ipv6_mobile_home_agent_addr_val_cmd,
     "ipv6 mobile home-agent-address X:X::X:X/M <0-4294967295> <0-4294967295>",
     CLI_IPV6_STR,
     CLI_MIP_STR,
     "Home agent address",
     "IPv6 prefix",
     "Valid lifetime (secs)",
     "Preferred lifetime (secs)")
{
  int ret;
  struct prefix_ipv6 p;
  struct interface *ifp = cli->index;
  u_int32_t vlifetime = RTADV_DEFAULT_VALID_LIFETIME;
  u_int32_t plifetime = RTADV_DEFAULT_PREFERRED_LIFETIME;
  u_char flags = PAL_ND_OPT_PI_FLAG_ROUTER;

#ifdef HAVE_LACPD
  NSM_INTERFACE_CHECK_AGG_MEM_PROPERTY(cli, ifp);
#endif /* HAVE_LACPD */
  CLI_GET_IPV6_PREFIX ("home agent address", p, argv[0]);

  if (argc > 1)
    CLI_GET_UINT32 ("home agent address valid lifetime", vlifetime, argv[1]);

  if (argc > 2)
    CLI_GET_INTEGER_RANGE ("home agent address preferred lifetime",
                           plifetime, argv[2], 0, vlifetime);

  ret = nsm_rtadv_mip6_prefix_set (cli->vr->id, ifp->name, &p.prefix,
                                   p.prefixlen, vlifetime, plifetime, flags);

  return nsm_cli_return (cli, ret);
}

ALI (ipv6_mobile_home_agent_addr,
     ipv6_mobile_home_agent_addr_cmd,
     "ipv6 mobile home-agent-address X:X::X:X/M",
     CLI_IPV6_STR,
     CLI_MIP_STR,
     "Home agent address",
     "IPv6 prefix");

CLI (no_ipv6_mobile_home_agent_addr,
     no_ipv6_mobile_home_agent_addr_cmd,
     "no ipv6 mobile home-agent-address X:X::X:X/M",
     CLI_NO_STR,
     CLI_IPV6_STR,
     CLI_MIP_STR,
     "Home agent address",
     "IPv6 prefix")
{
  int ret;
  struct prefix_ipv6 p;
  struct interface *ifp = cli->index;

#ifdef HAVE_LACPD
  NSM_INTERFACE_CHECK_AGG_MEM_PROPERTY(cli, ifp);
#endif /* HAVE_LACPD */
  CLI_GET_IPV6_PREFIX ("IPv6 prefix", p, argv[0]);

  ret = nsm_rtadv_mip6_prefix_unset (cli->vr->id, ifp->name,
                                     &p.prefix, p.prefixlen);

  return nsm_cli_return (cli, ret);
}

void
nsm_rtadv_ha_dump (struct cli *cli, struct rtadv_home_agent *ha,
                   struct interface *ifp)
{
  struct route_node *rn;
  struct rtadv_prefix *rp;

  cli_out (cli, "%-34R%%%%-s11d%14d\n",
           &ha->lladdr, ifp->name, ha->preference, ha->lifetime);

  cli_out (cli, "  %-46s%11s%14s%5s\n", "Global/Site-local Address",
           "Valid(sec)", "Prefered(sec)", "Flag");

  for (rn = route_top (ha->rt_address); rn; rn = route_next (rn))
    if ((rp = rn->info))
      cli_out (cli, "  %-43R/%-3u%11lu%14lu%3s%s%s\n",
               &rn->p.u.prefix6, rp->prefixlen, rp->vlifetime, rp->plifetime,
               CHECK_FLAG (rp->flags, PAL_ND_OPT_PI_FLAG_ONLINK) ? "O" : " ",
               CHECK_FLAG (rp->flags, PAL_ND_OPT_PI_FLAG_AUTO) ? "A" : " ",
               CHECK_FLAG (rp->flags, PAL_ND_OPT_PI_FLAG_ROUTER) ? "R" : " ");
}

void
nsm_rtadv_ha_dump_if (struct cli *cli, struct interface *ifp)
{
  struct listnode *n;
  struct nsm_if *nif;
  struct rtadv_if *rif;
  struct rtadv_home_agent *ha;

  nif = ifp->info;
  rif = nif->rtadv_if;

  if (! NSM_RTADV_CONFIG_CHECK (rif, MIP6_HOME_AGENT))
    return;

  if (rif->li_homeagent->count > 0)
    {
      cli_out (cli, "Mobile IPv6 home agent list on %s\n\n",
               ifp->name);

      cli_out (cli, "%-40s%11s%14s\n",
               "Link-local address", "Preference", "Lifetime(sec)");

      LIST_LOOP (rif->li_homeagent, ha, n)
        nsm_rtadv_ha_dump (cli, ha, ifp);
    }

  cli_out (cli, "\n");
}

void
nsm_rtadv_ha_dump_all (struct cli *cli)
{
  struct interface *ifp;
  struct route_node *rn;

  for (rn = route_top (cli->vr->ifm.if_table); rn; rn = route_next (rn))
    if ((ifp = rn->info))
      nsm_rtadv_ha_dump_if (cli, ifp);
}

CLI (show_ipv6_mobile_home_agent,
     show_ipv6_mobile_home_agent_cmd,
     "show ipv6 mobile home-agent (IFNAME|)",
     CLI_SHOW_STR,
     CLI_IPV6_STR,
     CLI_MIP_STR,
     CLI_HA_STR,
     "Interface's name")
{
  struct rtadv_if *rif;

  if (argc == 0)
    nsm_rtadv_ha_dump_all (cli);
  else
    {
      if ((rif = nsm_rtadv_if_lookup_by_name (cli->vr->proto, argv[0])))
        nsm_rtadv_ha_dump_if (cli, rif->ifp);
      else
        {
          cli_out (cli, "Invalid interface\n");
          return CLI_ERROR;
        }
    }

  return CLI_SUCCESS;
}
#endif /* HAVE_MIP6 */

void
nsm_rtadv_if_show (struct cli *cli, struct interface *ifp)
{
  struct nsm_master *nm = cli->vr->proto;
  struct nsm_if *nif;
  struct rtadv_if *rif;

  if (nm->rtadv == NULL)
    return;

  if ((nif = ifp->info) == NULL || (rif = nif->rtadv_if) == NULL)
    return;

  if (NSM_RTADV_CONFIG_CHECK (rif, RA_ADVERTISE))
    {
      cli_out (cli, "  ND router advertisements are sent every %lu seconds\n",
               rif->ra_interval);
      cli_out (cli, "  ND next router advertisement due in %lu seconds.\n",
               thread_timer_remain_second (rif->t_ra_unsolicited));
      cli_out (cli, "  ND router advertisements live for %lu seconds\n",
               rif->router_lifetime);
      if (NSM_RTADV_CONFIG_CHECK (rif, ROUTER_REACHABLE_TIME))
        cli_out (cli, "  ND advertised reachable time is %lu milliseconds\n",
                 rif->reachable_time);
      if (NSM_RTADV_CONFIG_CHECK(rif, ROUTER_RETRANS_TIMER))
        cli_out(cli, "  ND retransmission-time is %lu milliseconds\n", rif->retrans_timer);
      if (NSM_RTADV_CONFIG_CHECK(rif, CUR_HOPLIMIT))
        cli_out(cli, "  ND current hop limit is %hu bytes\n", rif->curr_hoplimit);
      if (NSM_RTADV_CONFIG_CHECK(rif, LINKMTU))
        cli_out(cli, "  ND link-mtu option is enabled\n");
      if (NSM_RTADV_CONFIG_CHECK(rif, MIN_RA_INTERVAL))
        cli_out(cli, "  ND minimum-ra-interval is %lu\n", rif->min_ra_interval);
      if (NSM_RTADV_CONFIG_CHECK(rif, PREFIX_VALID_LIFETIME))
        cli_out(cli, "  ND prefix valid-lifetime is %lu\n", rif->prefix_valid_lifetime);
      if (NSM_RTADV_CONFIG_CHECK(rif, PREFIX_PREFERRED_LIFETIME))
        cli_out(cli, "  ND prefix preferred-lifetime is %lu\n", rif->prefix_preferred_lifetime);
      if (NSM_RTADV_CONFIG_CHECK(rif, PREFIX_OFFLINK))
        cli_out(cli, "  ND prefix offlink option enable\n");
      if (NSM_RTADV_CONFIG_CHECK(rif, PREFIX_NO_AUTOCONFIG))
        cli_out(cli, "  ND prefix no-autoconf option enable\n");
      if (NSM_RTADV_CONFIG_CHECK (rif, RA_FLAG_MANAGED))
        cli_out (cli, "  Hosts use DHCP to obtain routable addresses.\n");
      else
        cli_out (cli, "  Hosts use stateless autoconfig for addresses.\n");
    }
}

void
nsm_rtadv_if_config_write (struct cli *cli, struct interface *ifp)
{
  struct nsm_master *nm = cli->vr->proto;
  struct nsm_if *nif;
  struct rtadv_if *rif;
  struct route_node *rn;
  struct rtadv_prefix *rp;
#ifdef HAVE_MIP6
  struct listnode *n;
  struct pal_in6_addr allzero;
  struct rtadv_home_agent *ha;
#endif /* HAVE_MIP6 */

  if (nm->rtadv == NULL)
    return;

  if ((nif = ifp->info) == NULL || (rif = nif->rtadv_if) == NULL)
    return;

  if (NSM_RTADV_CONFIG_CHECK (rif, RA_ADVERTISE))
    cli_out (cli, " no ipv6 nd suppress-ra\n");

  if (NSM_RTADV_CONFIG_CHECK (rif, RA_INTERVAL)
      && (rif->ra_interval != RTADV_MAX_RA_INTERVAL))
    cli_out (cli, " ipv6 nd ra-interval %lu\n", rif->ra_interval);

  if (NSM_RTADV_CONFIG_CHECK (rif, ROUTER_LIFETIME)
     && (rif->router_lifetime != ( 3 * rif->ra_interval)))
    cli_out (cli, " ipv6 nd ra-lifetime %hu\n", rif->router_lifetime);

  if (NSM_RTADV_CONFIG_CHECK (rif, ROUTER_REACHABLE_TIME)
     && (rif->reachable_time != RTADV_DEFAULT_REACHABLE_TIME))
    cli_out (cli, " ipv6 nd reachable-time %lu\n", rif->reachable_time);

  if (NSM_RTADV_CONFIG_CHECK (rif, ROUTER_RETRANS_TIMER)
     && (rif->retrans_timer != RTADV_DEFAULT_RETRANS_TIMER))
    cli_out(cli, " ipv6 nd retransmission-time %lu\n", rif->retrans_timer);
  if (NSM_RTADV_CONFIG_CHECK (rif, CUR_HOPLIMIT)
     && (rif->curr_hoplimit != RTADV_DEFAULT_CUR_HOPLIMIT))
    cli_out(cli, " ipv6 nd current-hoplimit %lu\n", rif->curr_hoplimit);
  if (NSM_RTADV_CONFIG_CHECK (rif, LINKMTU))
    cli_out(cli, " ipv6 nd link-mtu\n", rif->curr_hoplimit);
  if (NSM_RTADV_CONFIG_CHECK (rif, MIN_RA_INTERVAL)
      && (rif->min_ra_interval !=
         (int)(rif->ra_interval * RTADV_MIN_RA_INTERVAL_FACTOR)))
    cli_out(cli, " ipv6 nd minimum-ra-interval %lu\n", rif->min_ra_interval);
  if (NSM_RTADV_CONFIG_CHECK (rif, PREFIX_VALID_LIFETIME)
      && (rif->prefix_valid_lifetime != RTADV_DEFAULT_PREFIX_VALID_LIFETIME))
    cli_out(cli, " ipv6 nd prefix valid-lifetime %lu\n", rif->prefix_valid_lifetime);
  if (NSM_RTADV_CONFIG_CHECK (rif, PREFIX_PREFERRED_LIFETIME)
     && (rif->prefix_preferred_lifetime != RTADV_DEFAULT_PREFIX_PREFERRED_LIFETIME))
    cli_out(cli, " ipv6 nd prefix preferred-lifetime %lu\n", rif->prefix_preferred_lifetime);
  if (NSM_RTADV_CONFIG_CHECK (rif, PREFIX_OFFLINK))
    cli_out(cli, " ipv6 nd prefix offlink\n");
  if (NSM_RTADV_CONFIG_CHECK (rif, PREFIX_NO_AUTOCONFIG))
    cli_out(cli, " ipv6 nd prefix no-autoconf\n");

  if (NSM_RTADV_CONFIG_CHECK (rif, RA_FLAG_MANAGED))
    cli_out (cli, " ipv6 nd managed-config-flag\n");

  if (NSM_RTADV_CONFIG_CHECK (rif, RA_FLAG_OTHER))
    cli_out (cli, " ipv6 nd other-config-flag\n");

  for (rn = route_top (rif->rt_prefix); rn; rn = route_next (rn))
    if ((rp = rn->info))
      {
        cli_out (cli, " ipv6 nd prefix %Q", &rn->p);
        if ((rp->vlifetime && rp->vlifetime != RTADV_DEFAULT_VALID_LIFETIME)
            || (rp->plifetime && rp->plifetime != RTADV_DEFAULT_PREFERRED_LIFETIME)
            || ! CHECK_FLAG (rp->flags, PAL_ND_OPT_PI_FLAG_ONLINK)
            || ! CHECK_FLAG (rp->flags, PAL_ND_OPT_PI_FLAG_AUTO))
          {
            cli_out (cli, " %lu %lu", rp->vlifetime, rp->plifetime);
            if (! CHECK_FLAG (rp->flags, PAL_ND_OPT_PI_FLAG_ONLINK))
              cli_out (cli, " off-link");
            if (! CHECK_FLAG (rp->flags, PAL_ND_OPT_PI_FLAG_AUTO))
              cli_out (cli, " no-autoconfig");
          }
        cli_out (cli, "\n");
      }

#ifdef HAVE_MIP6
  if (NSM_RTADV_CONFIG_CHECK (rif, MIP6_HOME_AGENT))
    cli_out (cli, " ipv6 mobile home-agent\n");

  if (NSM_RTADV_CONFIG_CHECK (rif, MIP6_HA_INFO))
    cli_out (cli, " ipv6 mobile home-agent-info %hu %hu\n",
             rif->ha_preference, rif->ha_lifetime);

  if (NSM_RTADV_CONFIG_CHECK (rif, MIP6_ADV_INTERVAL))
    cli_out (cli, " ipv6 mobile adv-interval\n");

  pal_mem_set (&allzero, 0, sizeof (struct pal_in6_addr));
  LIST_LOOP (rif->li_homeagent, ha, n)
    for (rn = route_top (ha->rt_address); rn; rn = route_next (rn))
      if ((rp = rn->info))
        {
          cli_out (cli, " ipv6 mobile home-agent-address %R/%u",
                   &rn->p.u.prefix6, rp->prefixlen);
          if (rp->vlifetime != RTADV_DEFAULT_VALID_LIFETIME
              || rp->plifetime != RTADV_DEFAULT_PREFERRED_LIFETIME)
            cli_out (cli, " %lu %lu", rp->vlifetime, rp->plifetime);
          cli_out (cli, "\n");
        }
#endif /* HAVE_MIP6 */
}


/* Router advertisement CLIs (MIPv6 related RA is also supported).  */
void
nsm_cli_init_rtadv (struct cli_tree *ctree)
{
  cli_install_gen (ctree, INTERFACE_MODE, PRIVILEGE_NORMAL, 0,
                   &ipv6_nd_suppress_ra_cmd);
  cli_install_gen (ctree, INTERFACE_MODE, PRIVILEGE_NORMAL, 0,
                   &no_ipv6_nd_suppress_ra_cmd);
  cli_install_gen (ctree, INTERFACE_MODE, PRIVILEGE_NORMAL, 0,
                   &ipv6_nd_ra_interval_cmd);
  cli_install_gen (ctree, INTERFACE_MODE, PRIVILEGE_NORMAL, 0,
                   &no_ipv6_nd_ra_interval_cmd);
  cli_install_gen (ctree, INTERFACE_MODE, PRIVILEGE_NORMAL, 0,
                   &ipv6_nd_ra_lifetime_cmd);
  cli_install_gen (ctree, INTERFACE_MODE, PRIVILEGE_NORMAL, 0,
                   &no_ipv6_nd_ra_lifetime_cmd);
  cli_install_gen (ctree, INTERFACE_MODE, PRIVILEGE_NORMAL, 0,
                   &ipv6_nd_reachable_time_cmd);
  cli_install_gen (ctree, INTERFACE_MODE, PRIVILEGE_NORMAL, 0,
                   &no_ipv6_nd_reachable_time_cmd);
  cli_install_gen (ctree, INTERFACE_MODE, PRIVILEGE_NORMAL, 0,
                   &ipv6_nd_managed_config_flag_cmd);
  cli_install_gen (ctree, INTERFACE_MODE, PRIVILEGE_NORMAL, 0,
                   &no_ipv6_nd_managed_config_flag_cmd);
  cli_install_gen (ctree, INTERFACE_MODE, PRIVILEGE_NORMAL, 0,
                   &ipv6_nd_other_config_flag_cmd);
  cli_install_gen (ctree, INTERFACE_MODE, PRIVILEGE_NORMAL, 0,
                   &no_ipv6_nd_other_config_flag_cmd);
  cli_install_gen (ctree, INTERFACE_MODE, PRIVILEGE_NORMAL, 0,
                   &ipv6_nd_prefix_val_cmd);
  cli_install_gen (ctree, INTERFACE_MODE, PRIVILEGE_NORMAL, 0,
                   &ipv6_nd_prefix_cmd);
  cli_install_gen (ctree, INTERFACE_MODE, PRIVILEGE_NORMAL, 0,
                   &no_ipv6_nd_prefix_cmd);
  cli_install_gen(ctree, INTERFACE_MODE, PRIVILEGE_NORMAL, 0,
                  &ipv6_nd_retrans_timer_cmd);
  cli_install_gen(ctree, INTERFACE_MODE, PRIVILEGE_NORMAL, 0,
                  &no_ipv6_nd_retrans_timer_cmd);
  cli_install_gen(ctree, INTERFACE_MODE, PRIVILEGE_NORMAL, 0,
                  &ipv6_nd_curr_hoplimit_cmd);
  cli_install_gen(ctree, INTERFACE_MODE, PRIVILEGE_NORMAL, 0,
                  &no_ipv6_nd_curr_hoplimit_cmd);
  cli_install_gen(ctree, INTERFACE_MODE, PRIVILEGE_NORMAL, 0,
                  &ipv6_nd_linkmtu_cmd);
  cli_install_gen(ctree, INTERFACE_MODE, PRIVILEGE_NORMAL, 0,
                  &no_ipv6_nd_linkmtu_cmd);
  cli_install_gen(ctree, INTERFACE_MODE, PRIVILEGE_NORMAL, 0,
                  &ipv6_nd_min_ra_interval_cmd);
  cli_install_gen(ctree, INTERFACE_MODE, PRIVILEGE_NORMAL, 0,
                  &no_ipv6_nd_min_ra_interval_cmd);
  cli_install_gen(ctree, INTERFACE_MODE, PRIVILEGE_NORMAL, 0,
                  &ipv6_nd_prefix_valid_lifetime_cmd);
  cli_install_gen(ctree, INTERFACE_MODE, PRIVILEGE_NORMAL, 0,
                  &no_ipv6_nd_prefix_valid_lifetime_cmd);
  cli_install_gen(ctree, INTERFACE_MODE, PRIVILEGE_NORMAL, 0,
                  &ipv6_nd_prefix_preferred_lifetime_cmd);
  cli_install_gen(ctree, INTERFACE_MODE, PRIVILEGE_NORMAL, 0,
                  &no_ipv6_nd_prefix_preferred_lifetime_cmd);
  cli_install_gen(ctree, INTERFACE_MODE, PRIVILEGE_NORMAL, 0,
                  &ipv6_nd_prefix_flag_offlink_cmd);
  cli_install_gen(ctree, INTERFACE_MODE, PRIVILEGE_NORMAL, 0,
                  &no_ipv6_nd_prefix_flag_offlink_cmd);
  cli_install_gen(ctree, INTERFACE_MODE, PRIVILEGE_NORMAL, 0,
                  &ipv6_nd_prefix_flag_noautoconf_cmd);
  cli_install_gen(ctree, INTERFACE_MODE, PRIVILEGE_NORMAL, 0,
                  &no_ipv6_nd_prefix_flag_noautoconf_cmd);

#ifdef HAVE_MIP6
  cli_install_gen (ctree, INTERFACE_MODE, PRIVILEGE_NORMAL, 0,
                   &ipv6_mobile_home_agent_cmd);
  cli_install_gen (ctree, INTERFACE_MODE, PRIVILEGE_NORMAL, 0,
                   &no_ipv6_mobile_home_agent_cmd);
  cli_install_gen (ctree, INTERFACE_MODE, PRIVILEGE_NORMAL, 0,
                   &ipv6_mobile_home_agent_info_cmd);
  cli_install_gen (ctree, INTERFACE_MODE, PRIVILEGE_NORMAL, 0,
                   &no_ipv6_mobile_home_agent_info_cmd);
  cli_install_gen (ctree, INTERFACE_MODE, PRIVILEGE_NORMAL, 0,
                   &ipv6_mobile_adv_interval_cmd);
  cli_install_gen (ctree, INTERFACE_MODE, PRIVILEGE_NORMAL, 0,
                   &no_ipv6_mobile_adv_interval_cmd);
  cli_install_gen (ctree, INTERFACE_MODE, PRIVILEGE_NORMAL, 0,
                   &ipv6_mobile_home_agent_addr_val_cmd);
  cli_install_gen (ctree, INTERFACE_MODE, PRIVILEGE_NORMAL, 0,
                   &ipv6_mobile_home_agent_addr_cmd);
  cli_install_gen (ctree, INTERFACE_MODE, PRIVILEGE_NORMAL, 0,
                   &no_ipv6_mobile_home_agent_addr_cmd);

  cli_install_gen (ctree, EXEC_MODE, PRIVILEGE_NORMAL, 0,
                   &show_ipv6_mobile_home_agent_cmd);
#endif /* HAVE_MIP6 */
}
#endif /* HAVE_RTADV */

#ifdef HAVE_HWSTACK
int
nsm_stacking_config_write (struct cli *cli)
{
  int write = 0;

  cli_out (cli, "stacking masterdev 01:02:03:04:05:06\n");
  write++;

  return write;
}

#if 0
CLI (stacking_nsm,
     debug_nsm_cmd,
     "debug nsm (all|)",
     CLI_DEBUG_STR,
     CLI_NSM_STR,
     "Enable all debugging")
{
  nsm_debug_all_on (cli);
  if (cli->mode != CONFIG_MODE)
    cli_out (cli, "All possible debugging has been turned on\n");

  return CLI_SUCCESS;
}
#endif


CLI (show_stacking_num_cpu_nsm,
     show_stacking_num_cpu_nsm_cmd,
     "show stacking numCPU",
     CLI_SHOW_STR,
     CLI_STACKING_STR,
     CLI_STACK_NUM_CPU_STR)
{
  unsigned int num_entry, ret;

  ret = nsm_get_num_cpu_entries ((u_int32_t *)&num_entry);

  if (ret)
  {
     cli_out(cli, "ERROR: No CPU entries found\n");
  }
  else
  {
     cli_out (cli, "Number of CPU entries in the system %d\n", num_entry);
  }

  return CLI_SUCCESS;
}



CLI (nsm_stacking_set_master,
     nsm_stacking_set_master_cmd,
     "stacking masterdev MAC",
     CLI_STACKING_STR,
     CLI_STACK_MASTER_DEV_STR,
     "Mac (hardware) address of the CPU in HHHH.HHHH.HHHH format")
{
  unsigned int    ret;
  unsigned char mac_addr [ETHER_ADDR_LEN];
  u_int32_t mac1;
  u_int16_t mac2;

  pal_mem_set (mac_addr, 0, sizeof(mac_addr));
  if (pal_sscanf (argv[0], "%4hx.%4hx.%4hx",
                  (unsigned short *)&mac_addr[0],
                  (unsigned short *)&mac_addr[2],
                  (unsigned short *)&mac_addr[4]) != 3)
    {
      cli_out (cli, "%% Unable to translate mac address %s\n", argv[0]);
      return CLI_ERROR;
    }

  if (! nsm_api_mac_address_is_valid ((unsigned char *)argv[0]))
    {
      cli_out (cli, "%% Unable to translate mac address %s\n",
               argv[1]);
      return CLI_ERROR;
    }

  mac1 = *(u_int32_t *)&mac_addr[0];
  mac2 = *(u_int16_t *)&mac_addr[4];
  if ((mac1 == 0 && mac2 == 0) ||
      (mac1 == 0xffffffff && mac2 == 0xffff))
    {
      cli_out (cli, "%% Unable to translate mac address %s\n", argv[0]);
      return CLI_ERROR;
    }

  ret = nsm_stacking_set_entry_master (mac_addr);

  if (ret)
  {
     cli_out (cli, "CPU has been made the master CPU \n");
  }
  else
  {
     cli_out(cli, "ERROR: Entry could not be set as master\n");
  }

  return CLI_SUCCESS;
}



CLI (show_stacking_db_local_nsm,
     show_stacking_db_local_nsm_cmd,
     "show stacking local",
     CLI_SHOW_STR,
     CLI_STACKING_STR,
     "Local device(key)",
     "Show all Entries in the stacking database")
{
  struct nsm_cpu_info_entry stk_entry;

  memset(&stk_entry, '\0', NSM_CPU_KEY_SIZE);
  nsm_get_local_cpu_entry (&stk_entry);

  cli_out (cli, " Local CPU : %02x:%02x:%02x:%02x:%02x:%02x\n",
      stk_entry.mac_addr[0], stk_entry.mac_addr[1], stk_entry.mac_addr[2],
      stk_entry.mac_addr[3], stk_entry.mac_addr[4], stk_entry.mac_addr[5]);

  return CLI_SUCCESS;
}

CLI (show_stacking_db_master_nsm,
     show_stacking_db_master_nsm_cmd,
     "show stacking master",
     CLI_SHOW_STR,
     CLI_STACKING_STR,
     "Master device(key)",
     "Show all Entries in the stacking database")
{
  struct nsm_cpu_info_entry stk_entry;

  memset(&stk_entry, '\0', NSM_CPU_KEY_SIZE);
  nsm_get_master_cpu_entry (&stk_entry);

  cli_out (cli, " Master CPU : %02x:%02x:%02x:%02x:%02x:%02x\n",
      stk_entry.mac_addr[0], stk_entry.mac_addr[1], stk_entry.mac_addr[2],
      stk_entry.mac_addr[3], stk_entry.mac_addr[4], stk_entry.mac_addr[5]);

  return CLI_SUCCESS;
}

CLI (show_stacking_dump_db_nsm,
     show_stacking_dump_db_nsm_cmd,
     "show stacking dump db",
     CLI_SHOW_STR,
     CLI_STACKING_STR,
     "Dump all information",
     CLI_STACK_DB_STR,
     "Show all information about entries in the stacking database")
{
  u_char                    ii = 1;
  struct nsm_cpu_dump_entry dump_entry;
  unsigned int              num_entry;
  struct nsm_cpu_info_entry stk_entry;

  memset(&dump_entry, '\0', sizeof(struct nsm_cpu_dump_entry));
  cli_out (cli, "--------------------------------------------\n");
  cli_out (cli, "   DETAILED STACKING DATABASE\n");
  cli_out (cli, "--------------------------------------------\n");
  nsm_get_num_cpu_entries ((u_int32_t *)&num_entry);

  cli_out (cli, "  Total Number of CPU's = %d\n", num_entry);

  memset(&stk_entry, '\0', NSM_CPU_KEY_SIZE);
  nsm_get_master_cpu_entry (&stk_entry);
  cli_out (cli, "  Master CPU : %02x:%02x:%02x:%02x:%02x:%02x\n",
      stk_entry.mac_addr[0], stk_entry.mac_addr[1], stk_entry.mac_addr[2],
      stk_entry.mac_addr[3], stk_entry.mac_addr[4], stk_entry.mac_addr[5]);

  memset(&stk_entry, '\0', NSM_CPU_KEY_SIZE);
  nsm_get_local_cpu_entry (&stk_entry);
  cli_out (cli, "  Local CPU : %02x:%02x:%02x:%02x:%02x:%02x\n",
      stk_entry.mac_addr[0], stk_entry.mac_addr[1], stk_entry.mac_addr[2],
      stk_entry.mac_addr[3], stk_entry.mac_addr[4], stk_entry.mac_addr[5]);

  cli_out (cli, "  MAC ADDRESSES (KEY)\n\n\n");

  while (ii <= num_entry)
  {
     if (nsm_get_index_dump_cpu_entry (ii, &dump_entry))
     {
        nsm_dump_cpu_dump_entry (cli, ii, &dump_entry);
        cli_out(cli, "\n");
     }
     ii ++;
  }

  return CLI_SUCCESS;
}

CLI (show_stacking_db_nsm,
     show_stacking_db_nsm_cmd,
     "show stacking db (all|)",
     CLI_SHOW_STR,
     CLI_STACKING_STR,
     CLI_STACK_DB_STR,
     "Show all Entries in the stacking database")
{
  u_char                    ii = 1;
  struct nsm_cpu_info_entry stk_entry;
  unsigned int              num_entry;

  memset(&stk_entry, '\0', NSM_CPU_KEY_SIZE);
  cli_out (cli, "--------------------------------------------\n");
  cli_out (cli, "          STACKING DATABASE\n");
  cli_out (cli, "--------------------------------------------\n");
  nsm_get_num_cpu_entries ((u_int32_t *)&num_entry);

  cli_out (cli, "  Total Number of CPU's = %d\n", num_entry);
  cli_out (cli, "  MAC ADDRESSES (KEY)\n");

  while (ii <= num_entry)
  {
     if (nsm_get_index_cpu_entry (ii, &stk_entry))
     {
        nsm_dump_cpu_entry (cli, &stk_entry);
     }
     ii ++;
  }

  return CLI_SUCCESS;
}


void
nsm_stacking_init (struct cli_tree *ctree)
{
  cli_install_config (ctree, STACKING_MODE, nsm_stacking_config_write);

  cli_install_gen (ctree, CONFIG_MODE, PRIVILEGE_MAX, 0,
                           &nsm_stacking_set_master_cmd);

  cli_install_gen (ctree, EXEC_MODE, PRIVILEGE_MAX, 0,
                           &show_stacking_db_master_nsm_cmd);

  cli_install_gen (ctree, EXEC_MODE, PRIVILEGE_MAX, 0,
                           &show_stacking_db_local_nsm_cmd);

  cli_install_gen (ctree, EXEC_MODE, PRIVILEGE_MAX, 0,
                           &show_stacking_dump_db_nsm_cmd);

  cli_install_gen (ctree, EXEC_MODE, PRIVILEGE_MAX, 0,
                           &show_stacking_db_nsm_cmd);

  cli_install_gen (ctree, EXEC_MODE, PRIVILEGE_MAX, 0,
                           &show_stacking_num_cpu_nsm_cmd);

  /* "stacking" commands. */
#if 0
   cli_install_gen (ctree, EXEC_MODE, PRIVILEGE_MAX, 0,
                 &debug_nsm_cmd);

#endif
}
#endif /* HAVE_HWSTACK */



/* NSM CLI initialization.  */
void
nsm_cli_init (struct lib_globals *zg)
{
  struct cli_tree *ctree = zg->ctree;

  /* Initialize debug CLIs. */
  nsm_cli_init_debug (ctree);

 nsm_cli_init_all_interface(ctree);

#ifdef HAVE_L3
  /* Initialize interface CLIs.  */
  nsm_cli_init_interface (ctree);

  /* Initialize route related CLIs. */
  nsm_cli_init_route (ctree);

  /* Initialize static mroute related CLIs. */
  nsm_cli_init_static_mroute (ctree);

  /* Initialize router CLIs. */
  nsm_cli_init_router (ctree);
#endif /* HAVE_L3 */

  /* CPU related CLI init */
#ifdef HAVE_HWSTACK
  nsm_stacking_init (ctree);
#endif /* HAVE_HWSTACK */
#ifdef HAVE_VR
  /* Initialize VR CLIs. */
  nsm_vr_cli_init (ctree);
#endif /* HAVE_VR */

#ifdef HAVE_VRF
  /* Initialize VRF CLIs. */
  nsm_cli_init_vrf (ctree);
#endif /* HAVE_VRF */

#ifdef HAVE_RTADV
  /* Initialize Router Advertisement CLIs. */
  if (NSM_CAP_HAVE_IPV6)
    nsm_cli_init_rtadv (ctree);
#endif /* HAVE_RTADV */

#ifdef HAVE_TUNNEL
  /* Initialize tunnel interface CLIs. */
  nsm_tunnel_cli_init (ctree);
#endif /* HAVE_TUNNEL */

#ifdef HAVE_NSM_IF_UNNUMBERED
  /* Initialize unnumbered interface CLIs.  */
  nsm_if_unnumbered_cli_init (ctree);
#endif /* HAVE_NSM_IF_UNNUMBERED */

#ifdef HAVE_GMPLS
  /* Initialize GMPLS interface CLIs. */
  nsm_gmpls_cli_init (ctree);
#endif /* HAVE_GMPLS */

#ifdef HAVE_LACPD
  /* Initialize the Static Aggregator CLIs*/
  nsm_static_aggregator_cli_init (ctree);
  nsm_lacp_cli_init (ctree);
#endif /*HAVE_LACPD*/

#ifdef HAVE_L2
  nsm_bridge_cli_init (zg);
#if defined (HAVE_I_BEB)||defined (HAVE_B_BEB)
  nsm_pbb_cli_init (zg);
#endif
/* Initialize the pbb-te commands */
#ifdef HAVE_PBB_TE
  nsm_pbb_te_cli_init (zg);
#endif /* HAVE_PBB_TE */

#endif /* HAVE_L2 */

#ifdef HAVE_QOS
  nsm_qos_cli_init (ctree);
#endif /* HAVE_QOS */

#ifdef HAVE_DCB
  nsm_dcb_cli_init (ctree);
#endif /* HAVE_DCB */

#ifdef HAVE_VLAN
  nsm_vlan_cli_init (ctree);
#ifdef HAVE_L2LERN
  nsm_vlan_access_cli_init (ctree);
#endif /* HAVE_L2LERN */
#ifdef HAVE_PVLAN
  nsm_pvlan_cli_init (ctree);
#endif /* HAVE_PVLAN */
#ifdef HAVE_VLAN_CLASS
  nsm_vlan_classifier_cli_init(ctree);
#endif /* HAVE_VLAN_CLASS */
#ifdef HAVE_VLAN_STACK
  nsm_vlan_stack_cli_init(ctree);
#endif /* HAVE_VLAN_STACK */
#endif /* HAVE_VLAN */

#ifdef MEMMGR
  /* Initialize memory manager CLIs. */
  memmgr_cli_init (zg);
#endif /* MEMMGR */

#ifdef HAVE_MCAST_IPV4
  /* Initialize IPv4 multicast commands */
  nsm_mcast_cli_init (zg);
#endif /* HAVE_MCAST_IPV4 */

#ifdef HAVE_MCAST_IPV6
  /* Initialize IPv4 multicast commands */
  nsm_mcast6_cli_init (zg);
#endif /* HAVE_MCAST_IPV6 */

#ifdef HAVE_L3
  nsm_cli_init_arp (zg);
#endif /* HAVE_L3 */

#ifdef HAVE_L2LERN
  nsm_mac_acl_cli_init(ctree);
#endif /* HAVE_L2LERN */

#ifdef HAVE_L2
#ifdef HAVE_USER_HSL
  nsm_hw_reg_init (ctree);
  nsm_stats_show_init (ctree);
#endif
#endif /* HAVE_L2 */

#ifdef HAVE_BFD
  nsm_bfd_cli_init (ctree);
#endif /* HAVE_BFD */
  nsm_show_init (zg);


tun_cli_init(ctree);
ac_cli_init(ctree);
pw_cli_init(ctree);
port_cli_init(ctree);

vpn_cli_init(ctree);
lsp_cli_init(ctree);
lsp_grp_cli_init(ctree);
lsp_oam_cli_init(ctree);


ptp_cli_init(ctree);
ptp_port_cli_init(ctree);

nsm_oam_1731_cli_init(ctree);
nsm_meg_cli_init(ctree);
nsm_sec_cli_init(ctree);
}
