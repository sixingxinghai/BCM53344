#include "pal.h"
#include "lib.h"
#include "cli.h"

int generic_show_func (struct cli *, int, char **);

IMI_ALI (NULL,
    domain_create_cmd_imi,
    "domain-create <0-255>",
    "create domain",
    "domain value, range 0 - 255");

IMI_ALI (NULL,
     mstp_spanning_max_age_cmd_imi,
     "spanning-tree max-age <6-40>",
     "Spanning Tree Commands",
     "max-age",
     "seconds <6-40> - Maximum time to listen for root bridge in seconds");

IMI_ALI (NULL,
     no_spanning_instance_vlan_cmd_imi,
     "no instance <1-63> vlan VLANID",
     "Negate a command or set its defaults",
     "Instance",
     "ID",
     "Delete the association of vlan with this instance",
     "vlanid associated with instance");

IMI_ALI (NULL,
     no_bridge_group_vlan_cmd_imi,
     "no bridge-group <1-32> vlan <2-4094>",
     "Negate a command or set its defaults",
     "Bridge group commands",
     "Bridge Group name for bridging",
     "Rapid pervlan spanning tree vlan instance",
     "Vlan identifier");

IMI_ALI (NULL,
     no_qos_pmapc_set_cos_cmd_imi,
     "no set cos",
     "Negate a command or set its defaults",
     "Setting a new value in the packet",
     "CoS");

IMI_ALI (generic_show_func,
    show_spanning_tree_instance_vlan_cmd_imi,
    "show spanning-tree  vlan range-index",
    "Show running system information",
    "spanning-tree Display spanning tree information",
    "vlan information",
    "range-index value");

IMI_ALI (NULL,
     no_service_policy_output_cmd_imi,
     "no service-policy output NAME",
     "Negate a command or set its defaults",
     "Service policy",
     "Egress service policy",
     "Specify policy output name");

IMI_ALI (NULL,
     no_qos_pmapc_set_ip_dscp_cmd_imi,
     "no set ip-dscp",
     "Negate a command or set its defaults",
     "Setting a new value in the packet",
     "IP-DSCP");

IMI_ALI (NULL,
     spanning_revision_cmd_imi,
     "revision REVISION_NUM",
     "Revision  Number for configuration information",
     "Number 0-255");

IMI_ALI (NULL,
     no_mls_qos_trust_dscp_path_through_cos_cmd_imi,
     "no mls cos trust dscp path-through cos",
     "Negate a command or set its defaults",
     "Multi-Layer Switch(L2/L3).",
     "Quality of Service.",
     "Enable pass-through mode, not modify CoS value",
     "DSCP",
     "Path-through CoS",
     "CoS");

IMI_ALI (NULL,
    port_rate_limit_cmd_imi,
    "rate-limit (disable|enable cir <0-1000000> cbs <0-4095> )",
    "config port rate limit paras",
    "disable rate limit",
    "enable rate limit",
    "committed information rate",
    "rate value, kbps",
    "committed brust size",
    "brust size value, bytes");

IMI_ALI (NULL,
     nsm_br_vlan_name_cmd_imi,
     "vlan ""<2-4094>"" bridge <1-32> name WORD (state (enable|disable)|)",
     "Add, delete, or modify values associated with a single VLAN",
     "VLAN id",
     "Bridge group commands.",
     "Bridge group for bridging.",
     "Ascii name of the VLAN",
     "The ascii name of the VLAN",
     "Operational state of the VLAN",
     "Enable",
     "Disable");

IMI_ALI (NULL,
     nsm_br_cus_vlan_name_cmd_imi,
     "vlan ""<2-4094>"" type customer bridge <1-32> "
     "name WORD (state (enable|disable)|)",
     "Add, delete, or modify values associated with a single VLAN",
     "VLAN id",
     "VLAN Type",
     "Customer VLAN",
     "Bridge group commands.",
     "Bridge group for bridging.",
     "Ascii name of the VLAN",
     "The ascii name of the VLAN",
     "Operational state of the VLAN",
     "Enable",
     "Disable");

IMI_ALI (NULL,
    ptp_port_bind_config_cmd_imi,
    "bind (fe <1-24>|ge <1-4>)",
    "bind ethernet interface",
    "interface type",
    "fe interface id",
    "interface type",
    "ge interface id");

IMI_ALI (NULL,
     nsm_br_cus_vlan_no_name_cmd_imi,
     "vlan ""<2-4094>"" type customer bridge <1-32>",
     "Add, delete, or modify values associated with a single VLAN",
     "VLAN id",
     "VLAN Type",
     "Customer VLAN",
     "Service VLAN",
     "Bridge group commands.",
     "Bridge group for bridging.");

IMI_ALI (NULL,
    asymmetrydelay_config_cmd_imi,
    "set-asymdelay INTERVAL",
    "set asymmetry delay",
    "asymmetry delay value, range -500000000 - 500000000");

IMI_ALI (NULL,
     flowcontrol_both_on_cmd_imi,
     "flowcontrol both",
     "IEEE 802.3x Flow Control",
     "Flow control on send and receive",
     "Turn on flow control");

IMI_ALI (generic_show_func,
     show_gmrp_machine_br_cmd_imi,
     "show (gmrp|mmrp) machine bridge BRIDGE_NAME",
     "CLI_SHOW_STR",
     "GARP Multicast Registration Protocol",
     "MRP Multicast Registration Protocol",
     "Finite State Machine",
     "Bridge",
     "Bridge name");

IMI_ALI (NULL,
     no_ip_access_list_standard_host_cmd_imi,
     "no ip-access-list (<1-99>|<1300-1999>) (deny|permit) host A.B.C.D",
     "Negate a command or set its defaults",
     "QoS's IP access list",
     "IP standard access list", "IP standard access list (expanded range)", "Specify packets to reject", "Specify pactets to permit",
     "A single host address",
     "Address to match");

IMI_ALI (NULL,
     qos_pmapc_police_aggregate_cmd_imi,
     "police-aggregate NAME",
     "Specify a aggregate policer to multiple classes",
     "Specify a aggregate policer name");

IMI_ALI (generic_show_func,
     show_qos_access_list_name_cmd_imi,
     "show qos-access-list (<1-99>|<100-199>|<1300-1999>|<2000-2699>|WORD)",
     "Show running system information",
     "List QoS (MAC & IP) access lists",
     "Standard access list",
     "Extended access list",
     "Standard access list (expanded range)",
     "Extended access list (expanded range)",
     "QoS access-list name");

IMI_ALI (generic_show_func,
     show_gvrp_statistics_cmd_imi,
     "show (gvrp|mvrp) statistics",
     "Show running system information",
     "GARP VLAN Registration Protocol",
     "MRP Vlan Registration Protocol",
     "Statistics");

IMI_ALI (NULL,
    pd_alarm_threshold_cmd_imi,
    "packet-delay alarm-threshold <1-100000>",
    "packet-delay alarm-threshold:1-100000 ",
    "packet-delay alarm-threshold configure"
);

IMI_ALI (NULL,
     mls_qos_min_reserve_cmd_imi,
     "mls qos min-reserve <1-8> <10-170>",
     "Multi-Layer Switch(L2/L3).",
     "Quality of Service.",
     "Configure the buffer size of min-reserve level",
     "Specify level ID in Fast Ethernet",
     "Specify holding packets in Fast Ethernet");

IMI_ALI (NULL,
    no_creat_local_mep_cmd_imi,
    "no local-mep ID",
    "delete local-mep ID ",
    "no creat local mep"
);

IMI_ALI (NULL,
     no_nsm_br_pro_vlan_mtu_cmd_imi,
     "no vlan ""<2-4094>"" type (customer|service) mtu bridge <1-32>",
     "Negate a command or set its defaults",
     "Add, delete, or modify values associated with a single VLAN",
     "VLAN id",
     "VLAN Type",
     "Customer VLAN",
     "Service VLAN",
     "Reset the MMaximum Transmission Unit value for the vlan",
     "Bridge group commands.",
     "Bridge group for bridging.");

IMI_ALI (NULL,
     set_port_gvrp2_cmd_imi,
     "set port (gvrp|mvrp) disable (IF_NAME|all)",
     "Disable GVRP on interface",
     "Layer2 Interface",
     "GARP Vlan Registration Protocol",
     "MRP Vlan Registration Protocol",
     "Disable",
     "Ifname",
     "All the ports");

IMI_ALI (NULL,
     no_mls_qos_force_trust_cos_cmd_imi,
     "no mls qos force-trust cos",
     "Negate a command or set its defaults",
     "Multi-Layer Switch(L2/L3).",
     "Quality of Service.",
     "Confiure force trust state",
     "Classifies ingress packets with the packet cos values");

IMI_ALI (NULL,
     no_mls_qos_map_policed_dscp_cmd_imi,
     "no mls qos map policed-dscp",
     "Negate a command or set its defaults",
     "Multi-Layer Switch(L2/L3).",
     "Quality of Service.",
     "Specify maps",
     "Modify the policed-DSCP map");

IMI_ALI (NULL,
     ip_access_list_extended_host_host_cmd_imi,
     "ip-access-list (<100-199>|<2000-2699>) (deny|permit) ip host A.B.C.D host A.B.C.D",
     "QoS's IP access list",
     "IP extended access list", "IP extended access list (expanded range)", "Specify packets to reject", "Specify pactets to permit",
     "Any Internet Protocol",
     "A single source host",
     "Source address",
     "A single destination host",
     "Destination address");

IMI_ALI (NULL,
     undebug_hal_cmd_imi,
     "undebug hal (all|)",
     "Disable debugging functions (see also 'debug')",
     "Network Service Module (NSM)",
     "Disable all debugging");

IMI_ALI (NULL,
     bridge_acquire_cmd_imi,
     "bridge <1-32> acquire",
     "Bridge group commands.",
     "Bridge group for bridging.",
     "Enable dynamic learning of mac addresses");

IMI_ALI (NULL,
     no_qos_pmapc_set_ip_precedence_cmd_imi,
     "no set ip-precedence",
     "Negate a command or set its defaults",
     "Setting a new value in the packet",
     "IP-Precedence");

IMI_ALI (NULL,
     mls_qos_map_ip_prec_dscp_cmd_imi,
     "mls qos map ip-prec-dscp <0-63> <0-63> <0-63> <0-63> <0-63> <0-63> <0-63> <0-63>",
     "Multi-Layer Switch(L2/L3).",
     "Quality of Service.",
     "Specify maps",
     "Modify the IP-PREC-to-DSCP map",
     "DSCP value, mapping to IP Precedence 0",
     "DSCP value, mapping to IP Precedence 1",
     "DSCP value, mapping to IP Precedence 2",
     "DSCP value, mapping to IP Precedence 3",
     "DSCP value, mapping to IP Precedence 4",
     "DSCP value, mapping to IP Precedence 5",
     "DSCP value, mapping to IP Precedence 6",
     "DSCP value, mapping to IP Precedence 7");

IMI_ALI (NULL,
     no_debug_nsm_all_cmd_imi,
     "no debug all nsm",
     "Negate a command or set its defaults",
     "Debugging functions (see also 'undebug')",
     "Turn off all Debugging",
     "Network Service Module (NSM)");

IMI_ALI (NULL,
  domain_sport_add_cmd_imi,
  "domain-add <0-255> sport <2-19>",
  "add vport to domain",
  "domain number, range 0 - 255",
  "add port to be connected with slave clock",
  "virtual port, range 2-19" );

IMI_ALI (NULL,
     mstp_bridge_cisco_interop_cmd_imi,
     "bridge <1-32> cisco-interoperability ( enable | disable)",
     "Bridge group commands",
     "Bridge Group name for bridging",
     "Configure CISCO Interoperability",
     "Enable CISCO Interoperability",
     "Disable CISCO Interoperability");

IMI_ALI (NULL,
     no_mls_qos_vlan_priority_cmd_imi,
     "no mls qos vlan-priority-override (bridge <1-32> |) VLANID <0-7>",
     "Negate a command or set its defaults",
     "Multi-Layer Switch(L2/L3).",
     "Quality of Service.",
     "Vlan Priority Override",
     "Bridge group commands",
     "Bridge group name",
     "Select vlan id <1-4094>",
     "Select priority <0-7>");

IMI_ALI (NULL,
      mstp_bridge_portfast_bpdufilter_cmd_imi,
      "bridge <1-32> spanning-tree portfast bpdu-filter",
      "Bridge group commands",
      "Bridge Group name for bridging",
      "spanning-tree",
      "portfast",
      "Filter the BPDUs on portfast enabled ports");

IMI_ALI (NULL,
     clear_gvrp_statistics_port_cmd_imi,
     "clear (gvrp|mvrp) statistics IFNAME",
     "Reset functions",
     "GARP Vlan Registration Protocol",
     "MRP Vlan Registration Protocol",
     "GVRP Statistics",
     "Port Name");

IMI_ALI (NULL,
    ACH_channel_type_and_mel_cmd_imi,
    "y1731-channel-type TYPE mel MEL",
    "configure y1731-channel-type:0x0000-0xFFFF",
    "set mel:0-7"
);

IMI_ALI (NULL,
     mstp_bridge_priority_cmd_imi,
     "bridge <1-32> priority <0-61440>",
     "Bridge group commands",
     "Bridge Group name for bridging",
     "priority - bridge priority for the common instance",
     "bridge priority in increments of 4096 (Lower priority indicates greater likelihood of becoming root)");

IMI_ALI (NULL,
     mls_qos_trust_dscp_cos_cmd_imi,
     "mls qos trust (dscp | cos | both)",
     "Multi-Layer Switch(L2/L3).",
     "Quality of Service.",
     "Confiure port trust state",
     "Classifies ingress packets with the packet DSCP values",
     "Classifies ingress packets with the packet COS values",
     "Classifies ingress packets with the packet BOTH values");

IMI_ALI (NULL,
     qos_cmap_match_vlan_range_cmd_imi,
     "match vlan-range <1-4094> to <1-4094>",
     "Define the match creteria",
     "List VLAN ID's range",
     "Specify starting vlan ID",
     "Specify range",
     "Specify ending VLAN ID");

IMI_ALI (NULL,
     no_mls_qos_min_reserve_cmd_imi,
     "no mls qos min-reserve <0-7>",
     "Negate a command or set its defaults",
     "Multi-Layer Switch(L2/L3).",
     "Quality of Service.",
     "Minimum reservation",
     "Specify level ID");

IMI_ALI (NULL,
     no_gvrp_debug_event_cmd_imi,
     "no debug gvrp event",
     "Negate a command or set its defaults",
     "debug - enable debug output",
     "gvrp - GVRP commands",
     "event - echo events to console");

IMI_ALI (NULL,
    lt_detect_timeout_cmd_imi,
    "lt timeout  <1-10>",
    "lt configure ",
    "lt timeout configure:timeout 1-10,default is 5s. "

);

IMI_ALI (NULL,
     undebug_hal_events_cmd_imi,
     "undebug nsm hal events",
     "Disable debugging functions (see also 'debug')",
     "Network Service Module (NSM)",
     "Hardware Abstraction Layer",
     "NSM events");

IMI_ALI (NULL,
     ip_access_list_extended_any_any_cmd_imi,
     "ip-access-list (<100-199>|<2000-2699>) (deny|permit) ip any any",
     "QoS's IP access list",
     "IP extended access list", "IP extended access list (expanded range)", "Specify packets to reject", "Specify pactets to permit",
     "Any Internet Protocol",
     "Any source host",
     "Any destination host");

IMI_ALI (NULL,
     ip_mroute_prefix_distance_cmd_imi,
     "ip mroute A.B.C.D/M (" "static|rip|ospf|bgp|isis|" ") (A.B.C.D|INTERFACE) <1-255>",
     "Internet Protocol (IP)",
     "Configure static multicast routes",
     "Source prefix",
     "Static routes", "Routing Information Protocol (RIP)", "Open Shortest Patch First (OSPF)", "Border Gateway Protocol (BGP)", "ISO IS-IS",
     "RPF neighbor address or route",
     "RPF interface or pseudo interface Null",
     "Administrative distance for mroute");

IMI_ALI (NULL,
    mstp_clear_spanning_tree_interface_cmd_imi,
    "clear spanning-tree statistics (interface IFNAME| (instance <1-63>| vlan <2-4094>)) bridge <1-32>",
    "Reset functions",
    "Spanning-tree Information",
    "Statistics of the BPDUs",
    "Interface",
    "Interface name",
    "Instance Information",
    "Instance ID",
    "Vlan",
    "VLAN ID Associated with the Instance",
    "Bridge",
    "Bridge ID");

IMI_ALI (NULL,
     no_bridge_shutdown_multiple_spanning_tree_cmd_imi,
     "no bridge shutdown <1-32>",
     "Negate a command or set its defaults",
     "Bridge group commands",
     "reset",
     "Bridge Group name for bridging");

IMI_ALI (NULL,
     no_spanning_vlan_priority_cmd_imi,
     "no spanning-tree vlan <2-4094> priority",
     "Negate a command or set its defaults",
     "Spanning Tree Commands" ,
     "Change priority for a particular vlan",
     "vlan id",
     "priority - Reset bridge priority to default for the vlan");

IMI_ALI (NULL,
     no_ip_access_list_extended_any_any_cmd_imi,
     "no ip-access-list (<100-199>|<2000-2699>) (deny|permit) ip any any",
     "Negate a command or set its defaults",
     "QoS's IP access list",
     "IP extended access list", "IP extended access list (expanded range)", "Specify packets to reject", "Specify pactets to permit",
     "Any Internet Protocol",
     "Any source host",
     "Any destination host");

IMI_ALI (NULL,
     no_debug_nsm_events_cmd_imi,
     "no debug nsm events",
     "Negate a command or set its defaults",
     "Debugging functions (see also 'undebug')",
     "Network Service Module (NSM)",
     "NSM events");

IMI_ALI (NULL,
     hw_register_get_cmd_imi,
     "hardware register get ADDR ",
      "hardware"
     "register",
     "get the value from the Register",
     "Register address in 0xhhhh format");

IMI_ALI (NULL,
     mac_access_list_any_host_cmd_imi,
     "mac-access-list <2000-2699> (deny|permit) any MAC MASK <1-8>",
     "QOS's MAC access list", "extended mac ACL <2000-2699>",
     "Specify packets to reject", "Specify packets to permit",
     "Source any",
     "Destination host's MAC address in HHHH.HHHH.HHHH format",
     "Destination wildcard in HHHH.HHHH.HHHH format",
     "Specify packet format, (1:Ethernet II/ 2:802.3/ 4:SNMP/ 8:LLC)");

IMI_ALI (NULL,
     no_bridge_multiple_spanning_tree_cmd_imi,
     "no bridge <1-32> "
     "(multiple-spanning-tree|rapid-pervlan-spanning-tree|rapid-spanning-tree|spanning-tree) enable "
     "(|bridge-forward)",
     "Negate a command or set its defaults",
     "Bridge group commands",
     "Bridge Group name for bridging",
     "multiple-spanning-tree - MSTP commands",
     "rapid pervlan-spanning-tree - RPVST+ commands",
     "rapid-spanning-tree - RSTP commands",
     "spanning-tree - STP commands",
     "enable(disable) spanning tree protocol",
     "put all ports of the bridge into forwarding state");

IMI_ALI (generic_show_func,
     nsm_show_vlan_filter_cmd_imi,
     "show vlan filter",
     "Show running system information",
     "VLAN parameters",
     "Show VLAN Access Map Filter");

IMI_ALI (NULL,
     no_bridge_group_inst_path_cost_cmd_imi,
     "no bridge-group ( <1-32> | backbone) instance <1-63> path-cost",
     "Negate a command or set its defaults",
     "Bridge group commands",
     "Bridge Group name for bridging",
     "Specifies the backbone bridge group name",
     "Multiple Spanning Tree Instance",
     "identifier",
     "path-cost - path cost for a port");

IMI_ALI (NULL,
     no_mls_qos_enable_global_cmd_imi,
     "no mls qos",
     "Negate a command or set its defaults",
     "Multi-Layer Switch(L2/L3).",
     "Quality of Service.");

IMI_ALI (NULL,
     undebug_nsm_events_cmd_imi,
     "undebug nsm events",
     "Disable debugging functions (see also 'debug')",
     "Network Service Module (NSM)",
     "NSM events");

IMI_ALI (NULL,
     debug_nsm_cmd_imi,
     "debug nsm (all|)",
     "Debugging functions (see also 'undebug')",
     "Network Service Module (NSM)",
     "Enable all debugging");

IMI_ALI (NULL,
      no_mstp_bridge_portfast_bpduguard_cmd_imi,
      "no bridge <1-32> spanning-tree portfast bpdu-guard",
      "Negate a command or set its defaults",
      "Bridge group commands",
      "Bridge Group name for bridging",
      "spanning-tree",
      "portfast",
      "Disable Guard of portfast port against bpdu receive");

IMI_ALI (generic_show_func,
     show_gvrp_configuration_cmd_imi,
     "show (gvrp|mvrp) configuration bridge BRIDGE_NAME",
     "CLI_SHOW_STR",
     "GARP Vlan Registration Protocol",
     "MRP Vlan Registration Protocol",
     "Display GVRP configuration",
     "Bridge instance ",
     "Bridge instance name");

IMI_ALI (NULL,
     nsm_vlan_classifier_activate_cmd_imi,
     "vlan classifier activate <1-16> (vlan NSM_VLAN_CLI_RNG |)",
     "Vlan commands",
     "Vlan classification commands",
     "Vlan classifier activation commands",
     "Vlan classifier group id",
     "Vlan",
     "Vlan Identifier");

IMI_ALI (generic_show_func,
     nsm_show_vlan_access_map_cmd_imi,
     "show vlan access-map",
     "Show running system information",
     "VLAN parameters",
     "Show VLAN Access Map");

IMI_ALI (NULL,
    lb_detect_timeout_cmd_imi,
    "lb timeout <1-10> ",
    "lb configure ",
    "lb timeout configure:timeout 1-10,default is 5s"
);

IMI_ALI (NULL,
     mstp_spanning_tree_guard_root_cmd_imi,
     "spanning-tree guard root",
     "spanning-tree",
     "guard",
     "disable reception of superior BPDUs");

IMI_ALI (NULL,
     qos_pmapc_set_cos_cmd_imi,
     "set cos (<0-7>|cos-inner)",
     "Setting a new value in the packet",
     "CoS",
     "Specify a new CoS value",
     "Specify CoS value to be copied from inner tag");

IMI_ALI (NULL,
     gvrp_debug_timer_cmd_imi,
     "debug gvrp timer",
     "gvrp - GVRP commands",
     "debug - enable debug output",
     "timer - echo timer start to console");

IMI_ALI (NULL,
      mstp_bridge_portfast_bpduguard_cmd_imi,
      "bridge <1-32> spanning-tree portfast bpdu-guard",
      "Bridge group commands",
      "Bridge Group name for bridging",
      "spanning-tree",
      "portfast",
      "guard the portfast ports against bpdu receive");

IMI_ALI (NULL,
     no_spanning_acquire_cmd_imi,
     "no spanning-tree acquire",
     "Negate a command or set its defaults",
     "Spanning Tree group commands.",
     "Disable dynamic learning of mac addresses");

IMI_ALI (NULL,
     bridge_inst_priority_cmd_imi,
     "bridge (<1-32>|backbone) instance <1-63> priority <0-61440>",
     "Bridge group commands",
     "Bridge Group name for bridging",
     "Specifies the backbone bridge group name",
     "Change priority for a particular instance",
     "instance id",
     "priority - bridge priority for the common instance",
     "bridge priority in increments of 4096 (Lower priority indicates greater likelihood of becoming root)");

IMI_ALI (generic_show_func,
     show_spanning_tree_rpvst_vlan_interface_cmd_imi,
     "show spanning-tree rpvst+ vlan <1-4094> interface IFNAME",
     "Show running system information",
     "spanning-tree Display spanning tree information",
     "Display RPVST+ information",
     "Display vlan information",
     "instance_id",
     "Interface information",
     "Interface name");

IMI_ALI (NULL,
     no_debug_hal_cmd_imi,
     "no debug nsm hal (all|)",
     "Negate a command or set its defaults",
     "Debugging functions (see also 'undebug')",
     "Network Service Module (NSM)",
     "Hardware Abstraction Layer",
     "Disable all debugging");

IMI_ALI (generic_show_func,
     show_gvrp_statistics_port_cmd_imi,
     "show (gvrp|mvrp) statistics IFNAME",
     "Show running system information",
     "GARP VLAN Registration Protocol",
     "MRP Vlan Registration Protocol",
     "Statistics",
     "Port Name");

IMI_ALI (NULL,
     qos_cmap_match_traffic_type_cmd_imi,
     "match traffic-type (unknown-unicast | "
     "unknown-multicast | broadcast | multicast "
     "| unicast | management | arp | tcp-data | "
     "tcp-control | udp | non-tcp-udp | queue0 | "
     "queue1 | queue2 | queue3 | all |) "
     "(traffic-type-and-queue | traffic-type-or-queue|)",
     "Define the match criteria",
     "Specify Traffic Type",
     "Specify unknown unicast frames",
     "Specify unknown multicast frames",
     "Specify broadcast frames",
     "Specify multicast frames",
     "Specify unicast frames",
     "Specify management frames",
     "Specify ARP frames",
     "Specify TCP data frames",
     "Specify TCP control frames",
     "Specify UDP frames",
     "Specify non TCP/UDP frames",
     "Specify queue0",
     "Specify queue1",
     "Specify queue2",
     "Specify queue3",
     "Specify all",
     "Specify traffic-type-and-queue",
     "Specify traffic-type-or-queue (default)");

IMI_ALI (NULL,
    lsp_group_modify_cmd_imi,
    "config  sw-back-flag (enable|disable) back-time <1-1000>",
    "lsp protect group config command",
    "switch back flag",
    "enable switch",
    "disable switch",
    "back time",
    "back time value");

IMI_ALI (NULL,
     nsm_bridge_protocol_rpvst_plus_cmd_imi,
     "bridge <1-32> protocol rpvst+",
     "Bridge group commands.",
     "Bridge group for bridging.",
     "Spanning-tree protocol",
     "IEEE 801.1w rapid per vlan spanning-tree protocol");

IMI_ALI (NULL,
     mls_qos_dscp_mutation_cmd_imi,
     "mls qos dscp-mutation NAME",
     "Multi-Layer Switch(L2/L3).",
     "Quality of Service.",
     "Specify DSCP-to-DSCP mutation",
     "Specify the name of DSCP-to-DSCP mutaion map");

IMI_ALI (generic_show_func,
     show_nsm_vlan_classifier_group_cmd_imi,
     "show vlan classifier group (<1-16>|)",
     "Show running system information",
     "Vlan commands",
     "Vlan classification commands",
     "Vlan classifier group id",
     "Group Id");

IMI_ALI (NULL,
     vlan_add_static_forward_cmd_imi,
     "bridge <1-32> address MAC forward IFNAME vlan ""<2-4094>",
     "Bridge group commands.",
     "Bridge group for bridging.",
     "address",
     "mac address in HHHH.HHHH.HHHH format",
     "forward",
     "Interface name",
     "VLAN",
     "vlan id");

IMI_ALI (NULL,
     no_priority_queue_out_cmd_imi,
     "no priority-queue out",
     "Negate a command or set its defaults",
     "Enable the egress expedite queue",
     "Enable the egress expedite queue");

IMI_ALI (NULL,
     no_mac_access_list_address_cmd_imi,
     "no mac-access-list <2000-2699> (source|destination) MAC priority <0-7>",
     "Negate a command or set its defaults",
     "QOS's MAC access list", "extended mac ACL <2000-2699>",
     "Specify packets with source MAC address",
     "Specify packets with destination MAC address",
     "Source host's MAC address in HHHH.HHHH.HHHH format",
     "Priority Class",
     "Priority Value");

IMI_ALI (NULL,
     no_qos_cmap_match_vlan_cmd_imi,
     "no match vlan",
     "Negate a command or set its defaults",
     "Define the match creteria",
     "List VLAN ID");

IMI_ALI (generic_show_func,
     show_gmrp_timer_cmd_imi,
     "show (gmrp|mmrp) timer IF_NAME",
     "CLI_SHOW_STR",
     "GARP Multicast Registration Protocol",
     "MRP Multicast Registration Protocol",
     "GARP timer",
     "Ifname");

IMI_ALI (NULL,
     spanning_region_cmd_imi,
     "region REGION_NAME",
     "MST Region",
     "Name of the region");

IMI_ALI (NULL,
     no_mstp_spanning_tree_guard_root_cmd_imi,
     "no spanning-tree guard root",
     "Negate a command or set its defaults",
     "spanning-tree",
     "guard",
     "disable root guard feature for this port");

IMI_ALI (NULL,
     no_mstp_spanning_tree_inst_restricted_role_cmd_imi,
     "no spanning-tree instance <1-63> restricted-role",
     "Negate a command or set its defaults",
     "spanning tree commands",
     "remove restrictions for the port of particular instance",
     "instance id",
     "remove restriction on the role of the port");

IMI_ALI (NULL,
     no_qos_cmap_match_ip_dscp_cmd_imi,
     "no match ip-dscp",
     "Negate a command or set its defaults",
     "Define the match creteria",
     "IP DSCP");

IMI_ALI (NULL,
 lt_detect_enable_cmd_imi,
 "lt {enable|disable}",
 "lt configure",
 "lt enable configure:lb enable or disable "

 );

IMI_ALI (NULL,
     no_mstp_spanning_tree_inst_restricted_tcn_cmd_imi,
     "no spanning-tree instance <1-63> restricted_tcn",
     "Negate a command or set its defaults",
     "spanning tree commands",
     "remove restrictions for the port of particular instance",
     "instance id",
     "remove restriction on propagation of topology change notifications from port");

IMI_ALI (NULL,
     bridge_instance_cmd_imi,
     "bridge (<1-32> | backbone) instance <1-63>",
     "Bridge group commands",
     "Bridge Group name for bridging",
     "Specifies the backbone bridge group name",
     "Instance",
     "ID");

IMI_ALI (NULL,
     nsm_vlan_switchport_allowed_vlan_none_cmd_imi,
     "switchport hybrid allowed vlan none",
     "Reset the switching characteristics of the Layer2 interface",
     "Set the Layer2 interface as hybrid",
     "Set the VLANs that will Xmit/Rx through the Layer2 interface",
     "VLANs that will be removed",
     "Allow no VLANs to Xmit/Rx through the Layer2 interface");

IMI_ALI (NULL,
     wrr_queue_threshold_dscp_map_cmd_imi,
     "wrr-queue dscp-map <1-2> (<0-63>|<0-63> <0-63>|<0-63> <0-63> <0-63>|<0-63> <0-63> <0-63> <0-63>|<0-63> <0-63> <0-63> <0-63> <0-63>|<0-63> <0-63> <0-63> <0-63> <0-63> <0-63>|<0-63> <0-63> <0-63> <0-63> <0-63> <0-63> <0-63>|<0-63> <0-63> <0-63> <0-63> <0-63> <0-63> <0-63> <0-63>)",
     "WRR queue",
     "Specify map DSCP values to the wred-drop thresholds of egress queue",
     "Specify the threshold ID",
     "DSCP value",
     "DSCP value",
     "DSCP value",
     "DSCP value",
     "DSCP value",
     "DSCP value",
     "DSCP value",
     "DSCP value",
     "DSCP value",
     "DSCP value",
     "DSCP value",
     "DSCP value",
     "DSCP value",
     "DSCP value",
     "DSCP value",
     "DSCP value",
     "DSCP value",
     "DSCP value",
     "DSCP value",
     "DSCP value",
     "DSCP value",
     "DSCP value",
     "DSCP value",
     "DSCP value",
     "DSCP value",
     "DSCP value",
     "DSCP value",
     "DSCP value",
     "DSCP value",
     "DSCP value",
     "DSCP value",
     "DSCP value",
     "DSCP value",
     "DSCP value",
     "DSCP value",
     "DSCP value");

IMI_ALI (NULL,
     mls_qos_map_policed_dscp_cmd_imi,
     "mls qos map policed-dscp <0-63> to <0-63>",
     "Multi-Layer Switch(L2/L3).",
     "Quality of Service.",
     "Specify maps",
     "Modify the policed-DSCP map",
     "Ingress DSCP value",
     "To",
     "Egress DSCP vlaue (marked down DSCP");

IMI_ALI (NULL,
     qos_pmapc_set_ip_precedence_cmd_imi,
     "set ip-precedence <0-7>",
     "Setting a new value in the packet",
     "IP-Precedence",
     "Specify a new IP-Precedence value");

IMI_ALI (NULL,
     spanning_shutdown_multiple_spanning_tree_cmd_imi,
     "spanning-tree shutdown",
     "Spanning Tree Commands",
     "reset");

IMI_ALI (NULL,
     gvrp_debug_packet_cmd_imi,
     "debug gvrp packet",
     "debug - enable debug output",
     "gvrp - GVRP commands",
     "packet - echo packet contents to console");

IMI_ALI (NULL,
     no_nsm_vlan_switchport_trunk_native_cmd_imi,
     "no switchport trunk native vlan",
     "Negate a command or set its defaults",
     "Reset the switching characteristics of the Layer2 interface",
     "Reset the switching characteristics of the Layer2 trunk interface",
     "Reset the native vlan of the Layer2 trunk interface",
     "Reset the native vlan of the Layer2 trunk interface to default vlan id");

IMI_ALI (NULL,
     no_wrr_queue_cos_map_cmd_imi,
     "no wrr-queue cos-map <0-7>",
     "Negate a command or set its defaults",
     "WRR queue",
     "CoS map",
     "Queue ID");

IMI_ALI (NULL,
     no_ip_access_list_extended_mask_host_cmd_imi,
     "no ip-access-list (<100-199>|<2000-2699>) (deny|permit) ip A.B.C.D A.B.C.D host A.B.C.D",
     "Negate a command or set its defaults",
     "QoS's IP access list",
     "IP extended access list", "IP extended access list (expanded range)", "Specify packets to reject", "Specify pactets to permit",
     "Any Internet Protocol",
     "Source address",
     "Source wildcard bits",
     "A single destination host",
     "Destination address");

IMI_ALI (NULL,
     bridge_group_inst_path_cost_cmd_imi,
     "bridge-group (<1-32> | backbone) instance <1-63> path-cost <1-200000000>",
     "Bridge group commands",
     "Bridge Group name for bridging",
     "Specifies the backbone bridge group name",
     "Multiple Spanning Tree Instance",
     "identifier",
     "path cost for a port",
     "path cost in range <1-200000000> (lower path cost indicates greater likelihood of becoming root)");

IMI_ALI (NULL,
     mstp_bridge_transmit_hold_count_cmd_imi,
     "bridge <1-32> transmit-holdcount <1-10>",
     "Bridge group commands",
     "Bridge Group name for bridging",
     "Transmit hold count of the bridge",
     "range of the transmitholdcount");

IMI_ALI (generic_show_func,
     show_traffic_class_table_cmd_imi,
     "show traffic-class-table interface IFNAME",
     "Show running system information",
     "Display traffic class table",
     "Interface information",
     "Interface name");

IMI_ALI (NULL,
     ip_mroute_prefix_cmd_imi,
     "ip mroute A.B.C.D/M (" "static|rip|ospf|bgp|isis|" ") (A.B.C.D|INTERFACE)",
     "Internet Protocol (IP)",
     "Configure static multicast routes",
     "Source prefix",
     "Static routes", "Routing Information Protocol (RIP)", "Open Shortest Patch First (OSPF)", "Border Gateway Protocol (BGP)", "ISO IS-IS",
     "RPF neighbor address or route",
     "RPF interface or pseudo interface Null");

IMI_ALI (NULL,
     no_mls_qos_da_priority_cos_queue_cmd_imi,
     "no mls qos da-priority-override",
     "Negate a command or set its defaults",
     "Multi-Layer Switch(L2/L3).",
     "Quality of Service.",
     "Destination Address Priority Override");

IMI_ALI (NULL,
     no_mstp_bridge_spanning_tree_forceversion_cmd_imi,
     "no bridge <1-32> spanning-tree force-version",
     "Negate a command or set its defaults",
     "Bridge group commands",
     "Bridge Group name for bridging",
     "spanning-tree" ,
     "force-version - Version of the protocol");

IMI_ALI (NULL,
     vlan_action_cmd_imi,
     "action (forward|discard)",
     "Configure VLAN Access Map Action",
     "Forward",
     "Discard");

IMI_ALI (NULL,
    clockclass_config_cmd_imi,
    "set-clock-class <0 - 255>",
    "set clock class",
    "clock class, range <0 - 255>");

IMI_ALI (NULL,
     nsm_br_cus_vlan_enable_disable_cmd_imi,
     "vlan ""<2-4094>"" type customer bridge <1-32> "
     "state (enable|disable)",
     "Add, delete, or modify values associated with a single VLAN",
     "VLAN id",
     "VLAN Type",
     "Customer VLAN",
     "Bridge group commands.",
     "Bridge group for bridging.",
     "Operational state of the VLAN",
     "Enable",
     "Disable");

IMI_ALI (NULL,
     no_gvrp_debug_all_cmd_imi,
     "no debug gvrp all",
     "Negate a command or set its defaults",
     "debug - enable debug output",
     "gvrp - GVRP commands",
     "all - turn off all debugging");

IMI_ALI (NULL,
     no_mstp_spanning_tree_restricted_tcn_cmd_imi,
     "no spanning-tree restricted-tcn",
     "Negate a command or set its defaults",
     "spanning tree commands",
     "remove restriction on propagation of topology change notifications");

IMI_ALI (NULL,
     set_gvrp_timer1_cmd_imi,
     "set (gvrp|mvrp) timer join TIMER_VALUE IF_NAME",
     "Set GVRP join timer for the specified interface",
     "GARP Vlan Registration Protocol",
     "MRP Vlan Registration Protocol",
     "GARP timer",
     "Join Timer",
     "Timervalue in centiseconds",
     "Ifname");

IMI_ALI (NULL,
     nsm_if_duplex_cmd_imi,
     "duplex (half|full|auto)",
     "Set duplex to interface",
     "set half-duplex",
     "set full-duplex",
     "set auto-negotiate");

IMI_ALI (NULL,
     mls_qos_map_dscp_cos_cmd_imi,
     "mls qos map dscp-cos NAME (<0-63>|<0-63> <0-63>|<0-63> <0-63> <0-63>|<0-63> <0-63> <0-63> <0-63>|<0-63> <0-63> <0-63> <0-63> <0-63>|<0-63> <0-63> <0-63> <0-63> <0-63> <0-63>|<0-63> <0-63> <0-63> <0-63> <0-63> <0-63> <0-63>|<0-63> <0-63> <0-63> <0-63> <0-63> <0-63> <0-63> <0-63>) to <0-7>",
     "Multi-Layer Switch(L2/L3).",
     "Quality of Service.",
     "Specify maps",
     "Modify DSCP-to-COS map",
     "Specify map name",
     "Incoming DSCP value",
     "Incoming DSCP value",
     "Incoming DSCP value",
     "Incoming DSCP value",
     "Incoming DSCP value",
     "Incoming DSCP value",
     "Incoming DSCP value",
     "Incoming DSCP value",
     "Incoming DSCP value",
     "Incoming DSCP value",
     "Incoming DSCP value",
     "Incoming DSCP value",
     "Incoming DSCP value",
     "Incoming DSCP value",
     "Incoming DSCP value",
     "Incoming DSCP value",
     "Incoming DSCP value",
     "Incoming DSCP value",
     "Incoming DSCP value",
     "Incoming DSCP value",
     "Incoming DSCP value",
     "Incoming DSCP value",
     "Incoming DSCP value",
     "Incoming DSCP value",
     "Incoming DSCP value",
     "Incoming DSCP value",
     "Incoming DSCP value",
     "Incoming DSCP value",
     "Incoming DSCP value",
     "Incoming DSCP value",
     "Incoming DSCP value",
     "Incoming DSCP value",
     "Incoming DSCP value",
     "Incoming DSCP value",
     "Incoming DSCP value",
     "Incoming DSCP value",
     "To",
     "Outgoing COS value");

IMI_ALI (NULL,
     no_nsm_interface_cmd_imi,
     "no interface IFNAME",
     "Negate a command or set its defaults",
     "Delete a pseudo interface's configuration",
     "Interface's name");

IMI_ALI (NULL,
     nsm_br_vlan_no_name_cmd_imi,
     "vlan ""<2-4094>"" bridge <1-32>",
     "Add, delete, or modify values associated with a single VLAN",
     "VLAN id",
     "VLAN",
     "Bridge group commands.",
     "Bridge group for bridging.");

IMI_ALI (NULL,
     no_nsm_br_vlan_mtu_cmd_imi,
     "no vlan ""<2-4094>"" mtu bridge <1-32>",
     "Negate a command or set its defaults",
     "Add, delete, or modify values associated with a single VLAN",
     "VLAN id",
     "Reset the MMaximum Transmission Unit value for the vlan",
     "Bridge group commands.",
     "Bridge group for bridging.");

IMI_ALI (NULL,
     mls_qos_trust_dscp_path_through_cos_cmd_imi,
     "mls cos trust dscp path-through cos",
     "Multi-Layer Switch(L2/L3).",
     "Quality of Service.",
     "Enable pass-through mode, not modify CoS value",
     "DSCP",
     "Path-through CoS",
     "CoS");

IMI_ALI (NULL,
     mstp_clear_spanning_tree_bridge_cmd_imi,
     "clear spanning-tree statistics bridge <1-32>",
     "Reset functions",
     "Spanning-tree Information",
     "Statistics of the BPDUs",
     "Bridge",
     "Bridge ID");

IMI_ALI (NULL,
     bridge_instance_vlan_cmd_imi,
     "bridge (<1-32> | backbone) instance <1-63> vlan VLANID",
     "Bridge group commands",
     "Bridge Group name for bridging",
     "Specifies the backbone bridge group name",
     "Instance",
     "ID",
     "vlan",
     "vlan id to be associated to instance");

IMI_ALI (NULL,
     mstp_spanning_tree_portfast_bpduguard_cmd_imi,
     "spanning-tree portfast bpdu-guard (enable|disable|default)",
     "spanning-tree",
     "portfast",
     "guard the port against reception of BPDUs",
     "enable",
     "disable",
     "default");

IMI_ALI (NULL,
     undebug_nsm_cmd_imi,
     "undebug nsm (all|)",
     "Disable debugging functions (see also 'debug')",
     "Network Service Module (NSM)",
     "Enable all debugging");

IMI_ALI (generic_show_func,
     show_spanning_tree_mst_instance_interface_cmd_imi,
     "show spanning-tree mst instance <1-63> interface IFNAME",
     "Show running system information",
     "spanning-tree Display spanning tree information",
     "Display MST information",
     "Display instance information",
     "instance_id",
     "Interface information",
     "Interface name");

IMI_ALI (NULL,
     set_gmrp_ext_filtering2_br_cmd_imi,
     "set (gmrp|mmrp) extended-filtering disable bridge BRIDGE_NAME ",
     "Enable Extended filtering Option",
     "GARP Multicast Registration Protocol",
     "MRP Multicast Registration Protocol",
     "Extended filtering Option",
     "disable",
     "Bridge",
     "BRIDGE_NAME");

IMI_ALI (NULL,
     spanning_vlan_cmd_imi,
     "vlan <2-4094>",
     "Vlan",
     "Vlan ID");

IMI_ALI (generic_show_func,
     show_gmrp_configuration_cmd_imi,
     "show (gmrp|mmrp) configuration",
     "Show running system information",
     "GARP Multicast Registration Protocol",
     "MRP Multicast Registration Protocol",
     "Display GMRP configuration");

IMI_ALI (NULL,
     no_mstp_spanning_tree_portfast_bpdufilter_cmd_imi,
     "no spanning-tree portfast bpdu-filter ",
     "Negate a command or set its defaults",
     "spanning-tree",
     "portfast",
     "disable the bpdu-filter feature for the port");

IMI_ALI (generic_show_func,
     show_interface_switchport_bridge_cmd_imi,
     "show interface switchport bridge <1-32>",
     "Display the characteristics of the Layer2 interface",
     "The layer2 interfaces",
     "Display the modes of the Layer2 interfaces",
     "The bridge to use with this VLAN",
     "The Bridge Group number");

IMI_ALI (NULL,
     no_lsp_oam_cmd_imi,
     "no lsp-oam <0-63>",
     "Negate a command or set its defaults",
     "delete a lsp oam's configuration",
     "lsp oam id, range 0-63");

IMI_ALI (NULL,
     no_nsm_access_group_any_cmd_imi,
     "no ip-access-group (<1-199>|<1300-2699>|WORD) (in|out)",
     "Negate a command or set its defaults",
     "IP access group",
     "IP access list (Standard or Extended)",
     "IP Expanded access list (Standard or Extended)",
     "IP PacOS access-list name",
     "inbound packets",
     "outbound packets");

IMI_ALI (NULL,
     ip_access_list_extended_mask_any_cmd_imi,
     "ip-access-list (<100-199>|<2000-2699>) (deny|permit) ip A.B.C.D A.B.C.D any",
     "QoS's IP access list",
     "IP extended access list", "IP extended access list (expanded range)", "Specify packets to reject", "Specify pactets to permit",
     "Any Internet Protocol",
     "Source address",
     "Source wildcard bits",
     "Any destination host");

IMI_ALI (NULL,
     qos_cmap_match_exp_cmd_imi,
     "match mpls exp-bit topmost (<0-7>|<0-7> <0-7>|<0-7> <0-7> <0-7>|<0-7> <0-7> <0-7> <0-7>|<0-7> <0-7> <0-7> <0-7> <0-7>|<0-7> <0-7> <0-7> <0-7> <0-7> <0-7>|<0-7> <0-7> <0-7> <0-7> <0-7> <0-7> <0-7>|<0-7> <0-7> <0-7> <0-7> <0-7> <0-7> <0-7> <0-7>)",
     "Define the match criteria",
     "Specify Multi Protocol Label Switch specific values",
     "Specify MPLS experimental bits",
     "Match MPLS experimental value on topmost label",
     "Specify experimental value",
     "Specify experimental value",
     "Specify experimental value",
     "Specify experimental value",
     "Specify experimental value",
     "Specify experimental value",
     "Specify experimental value",
     "Specify experimental value",
     "Specify experimental value",
     "Specify experimental value",
     "Specify experimental value",
     "Specify experimental value",
     "Specify experimental value",
     "Specify experimental value",
     "Specify experimental value",
     "Specify experimental value",
     "Specify experimental value",
     "Specify experimental value",
     "Specify experimental value",
     "Specify experimental value",
     "Specify experimental value",
     "Specify experimental value",
     "Specify experimental value",
     "Specify experimental value",
     "Specify experimental value",
     "Specify experimental value",
     "Specify experimental value",
     "Specify experimental value",
     "Specify experimental value",
     "Specify experimental value",
     "Specify experimental value",
     "Specify experimental value",
     "Specify experimental value",
     "Specify experimental value",
     "Specify experimental value",
     "Specify experimental value");

IMI_ALI (NULL,
 portstatemech_config_cmd_imi,
 "set-state-mech  STATEMECH",
 "config port state mechanism",
 "port state mechanism, range <bcm manual>");

IMI_ALI (generic_show_func,
     show_class_map_cmd_imi,
     "show class-map",
     "Show running system information",
     "Class map entry");

IMI_ALI (NULL,
    ptp_port_disable_config_cmd_imi,
    "no-enable",
    "disable port");

IMI_ALI (NULL,
     no_gvrp_debug_timer_cmd_imi,
     "no debug gvrp timer",
     "Negate a command or set its defaults",
     "gvrp - GVRP commands",
     "debug - disable debug output",
     "timer - do not echo timer start to console");

IMI_ALI (NULL,
     user_prio_regen_table_cmd_imi,
     "user-priority-regen-table user-priority <0-7> regenerated-user-priority <0-7>",
     "Set the value for the mapping of user-priority to regenerated user priority",
     "User priority associated with the regeneration table",
     "User priority value",
     "Regenerated values to be used for the user priority ",
     "Regenerated user priority value");

IMI_ALI (NULL,
     bridge_group_inst_cmd_imi,
     "bridge-group (<1-32> | backbone) instance (<1-63> | te-msti)",
     "Bridge group commands",
     "Bridge Group name for bridging",
     "Specifies the backbone bridge group name",
     "Multiple Spanning tree instance",
     "ID",
     "MSTI to be the traffic engineering MSTI instance");

IMI_ALI (NULL,
     no_mstp_spanning_tree_forceversion_cmd_imi,
     "no spanning-tree force-version",
     "Negate a command or set its defaults",
     "spanning-tree" ,
     "force-version - Version of the protocol");

IMI_ALI (NULL,
     no_bridge_group_vlan_path_cost_cmd_imi,
     "no bridge-group <1-32> vlan <2-4094> path-cost",
     "Negate a command or set its defaults",
     "Bridge group commands",
     "Bridge Group name for bridging",
     "Rapid Pervlan Spanning Tree Instance",
     "identifier",
     "path-cost - path cost for a port");

IMI_ALI (NULL,
     clear_gvrp_statistics_bridge_cmd_imi,
     "clear (gvrp|mvrp) statistics bridge BRIDGE_NAME",
     "Reset functions",
     "GARP Vlan Registration Protocol",
     "MRP Vlan Registration Protocol",
     "GVRP Statistics",
     "Bridge instance",
     "Bridge instance name");

IMI_ALI (NULL,
     set_gmrp1_cmd_imi,
     "set (gmrp|mmrp) enable",
     "Enable GMRP globally for the bridge instance",
     "GARP Multicast Registration Protocol",
     "MRP Multicast Registration Protocol",
     "Enable");

IMI_ALI (NULL,
    vpn_rmv_mutigrp_cmd_imi,
    "rmv-mutigrp id <1-8>",
    "rmv an muticast group in vpn",
    "muticast group index",
    "id value");

IMI_ALI (NULL,
     no_ip_access_list_extended_host_mask_cmd_imi,
     "no ip-access-list (<100-199>|<2000-2699>) (deny|permit) ip host A.B.C.D A.B.C.D A.B.C.D",
     "Negate a command or set its defaults",
     "QoS's IP access list",
     "IP extended access list", "IP extended access list (expanded range)", "Specify packets to reject", "Specify pactets to permit",
     "Any Internet Protocol",
     "A single source host",
     "Source address",
     "Destination address",
     "Destination Wildcard bits");

IMI_ALI (NULL,
     debug_hal_cmd_imi,
     "debug nsm hal (all|)",
     "Debugging functions (see also 'undebug')",
     "Network Service Module (NSM)",
     "Hardware Abstraction Layer",
     "Enable all debugging");

IMI_ALI (generic_show_func,
      show_nsm_vlan_classifier_interface_group_cmd_imi,
      "show vlan classifier interface group (<1-16>|)",
      "Show running system information",
      "Vlan commands",
      "Vlan classification commands",
      "Interface group activated on",
      "Vlan classifier group id",
      "Group Id");

IMI_ALI (NULL,
     no_debug_mstp_packet_rx_cmd_imi,
     "no debug mstp packet rx",
     "Negate a command or set its defaults",
     "Debugging functions (see also 'undebug')",
     "Multiple Spanning Tree Protocol (MSTP)",
     "MSTP Packets",
     "Receive");

IMI_ALI (generic_show_func,
     show_spanning_tree_cmd_imi,
     "show spanning-tree",
     "Show running system information",
     "Display spanning-tree information");

IMI_ALI (NULL,
    fpga_read_cmd_imi,
    "debug fpga-read ADDR",
    "Debugging functions (see also 'undebug')",
    "read fpag value",
    "fpga address, in HHHH format");

IMI_ALI (NULL,
     set_gvrp2_br_cmd_imi,
     "set (gvrp|mvrp) disable bridge BRIDGE_NAME",
     "Disable GVRP globally for the bridge instance",
     "GARP Vlan Registration Protocol",
     "MRP Vlan Registration Protocol",
     "Disable",
     "Bridge instance ",
     "Bridge instance name");

IMI_ALI (NULL,
     debug_mstp_packet_tx_cmd_imi,
     "debug mstp packet tx",
     "Debugging functions (see also 'undebug')",
     "Multiple Spanning Tree Protocol (MSTP)",
     "MSTP Packets",
     "Transmit");

IMI_ALI (NULL,
    lsp_cfg_switch_cmd_imi,
    "config ingress (fe <1-24>|ge <1-4>) label <0-1048575> (vlan <0-4095>|) egress (fe <1-24>|ge <1-4>) label <0-1048575> (vlan <0-4095>|) exp <0-8> peer-mac MAC",
    "lsp config",
    "ingress tunnel",
    "ingress-tun interface type",
    "fe interface id",
    "ingress-tun interface type",
    "ge interface id",
    "ingress-tun lable",
    "lable value",
    "ingress-tun vlan",
    "vlan id, 0 means vlan diable",
    "egress tunnel",
    "egress-tun interface type",
    "fe interface id",
    "egress-tun interface type",
    "ge interface id",
    "egress-tun lable",
    "lable value",
    "egress-tun vlan",
    "vlan id, 0 means vlan diable",
    "egress-tun exp",
    "exp value,<0-7> outgoing lsp label exp uses the number specified; <8> outgoing lsp label exp uses internal priority mapping",
    "egress-tun peer mac address",
    "mac (hardware) address, in HHHH.HHHH.HHHH format");

IMI_ALI (NULL,
     mls_qos_wrr_weight_queue_cmd_imi,
     "mls qos wrr-weight <0-3> <1-32>",
      "Multi-Layer Switch(L2/L3).",
      "Quality of Service.",
     "Wrr weight",
     "Configure the queue id",
     "Specify a weight");

IMI_ALI (NULL,
     no_mac_access_list_host_host_cmd_imi,
     "no mac-access-list <2000-2699> (deny|permit) MAC MASK MAC MASK <1-8>",
     "Negate a command or set its defaults",
     "QOS's MAC access list", "extended mac ACL <2000-2699>",
     "Specify packets to reject", "Specify packets to permit",
     "Source host's MAC address in HHHH.HHHH.HHHH format",
     "Source wildcard in HHHH.HHHH.HHHH format",
     "Destination host's MAC address in HHHH.HHHH.HHHH format",
     "Destination wildcard in HHHH.HHHH.HHHH format",
     "Specify packet format, (1:Ethernet II/ 2:802.3/ 4:SNMP/ 8:LLC)");

IMI_ALI (NULL,
     set_port_gmrp_enable_vlan_cmd_imi,
     "set port (gmrp|mmrp) enable IF_NAME vlan VLANID",
     "Enable GMRP on a port",
     "Layer2 Interface",
     "GARP Multicast Registration Protocol",
     "MRP Multicast Registration Protocol",
     "Enable",
     "Ifname",
     "Identify the VLAN to use",
     "The VLAN ID to use");

IMI_ALI (NULL,
     ip_access_list_standard_host_cmd_imi,
     "ip-access-list (<1-99>|<1300-1999>) (deny|permit) host A.B.C.D",
     "QoS's IP access list",
     "IP standard access list", "IP standard access list (expanded range)", "Specify packets to reject", "Specify pactets to permit",
     "A single host address",
     "Address to match");

IMI_ALI (NULL,
     qos_pmapc_trust_cmd_imi,
     "trust dscp",
     "Specify trust state for policy-map",
     "Trust DSCP");

IMI_ALI (generic_show_func,
     show_nsm_vlan_bridge_cmd_imi,
     "show vlan (all|static|dynamic|auto) bridge <1-32>",
     "Show running system information",
     "Display VLAN information",
     "All VLANs(static and dynamic)",
     "Static VLANs",
     "Dynamic VLANS",
     "Auto configured VLANS",
     "The Bridge Group to use with this VLAN",
     "The Bridge Group number");

IMI_ALI (NULL,
      no_mstp_spanning_hello_time_cmd_imi,
      "no spanning-tree hello-time",
      "Negate a command or set its defaults",
      "Spanning Tree Commands",
      "hello-time - hello BDPU interval");

IMI_ALI (NULL,
     spanning_tree_ageing_time_cmd_imi,
     "spanning-tree ageing-time <10-1000000>",
     "Spanning Tree Commands",
     "time a learned mac address will persist after last update",
     "ageing time in seconds");

IMI_ALI (NULL,
     mirror_interface_transmit_cmd_imi,
     "mirror interface IFNAME direction transmit",
     "Port mirroring command",
     "Interface to use",
     "Source Interface to use",
     "Mirroring direction",
     "Mirror transmit traffic");

IMI_ALI (NULL,
     nsm_vlan_switchport_hybrid_acceptable_frame_cmd_imi,
     "switchport mode (hybrid) acceptable-frame-type (all|vlan-tagged)",
     "Set the switching characteristics of the Layer2 interface",
     "Set the mode of the Layer2 interface",
     "Set the Layer2 interface as hybrid",
     "Set the Layer2 interface acceptable frames types",
     "Set all frames can be received",
     "Set vlan-tagged frames can only be received");

IMI_ALI (NULL,
     flowcontrol_send_on_cmd_imi,
     "flowcontrol send on",
     "IEEE 802.3x Flow Control",
     "Flow control on send",
     "Turn on flow control");

IMI_ALI (NULL,
     mstp_port_hello_time_cmd_imi,
     "spanning-tree hello-time <1-10>",
     "spanning tree commands",
     "hello-time - hello BDPU interval",
     "seconds <1-10> - Hello BPDU interval");

IMI_ALI (NULL,
     bridge_vlan_cmd_imi,
     "bridge <1-32> vlan <2-4094>",
     "Bridge group commands",
     "Bridge Group name for bridging",
     "Vlan",
     "Vlan Id");

IMI_ALI (NULL,
     no_debug_mstp_all_cmd_imi,
     "no debug mstp all",
     "Negate a command or set its defaults",
     "Debugging functions (see also 'undebug')",
     "Multiple Spanning Tree Protocol (MSTP)",
     "all");

IMI_ALI (NULL,
     wrr_queue_wred_cmd_imi,
     "wred (disable qid <0-7>|enable qid <0-7> color <0-7> dropstart <0-100> slope <0-90> time <0-255>)",
     "Configure WRED drop threshold and weight on egress queue.",
     "disable to restore the queue to default wred configuration. ",
     "set queue ID.",
     "ID number 0-7.",
     "enable wred configuration.",
     "set queue ID",
     "ID number 0-7",
     "Specify the bit map of color combination",
     "Set color combinations, bit0=1->red; bit1=1->yellow; bit2=1->green. if set 0,restore default setting",
     "Specify the percentage of average queue size to start dropping packets",
     "Set start dropping packets paras",
     "Specify the the probality slope of droping packets",
     "Set slope paras",
     "Specify the average time in us, average_time/4 = 2**WRED_weight",
     "Set average_time paras");

IMI_ALI (NULL,
     no_ip_access_list_standard_cmd_imi,
     "no ip-access-list (<1-99>|<1300-1999>) (deny|permit) A.B.C.D A.B.C.D",
     "Negate a command or set its defaults",
     "QoS's IP access list",
     "IP standard access list", "IP standard access list (expanded range)", "Specify packets to reject", "Specify pactets to permit",
     "Address to match",
     "Wildcard bits");

IMI_ALI (NULL,
      no_mstp_spanning_tree_errdisable_timeout_enable_cmd1_imi,
      "no spanning-tree errdisable-timeout enable",
      "Negate a command or set its defaults",
      "spanning-tree",
      "errdisable-timeout",
      "enable the timeout mechanism for the port to be enabled back");

IMI_ALI (NULL,
    p2p_meanpath_delay_config_cmd_imi,
    "set-p2p-mean-delay  <0-1000000000>",
    "set p2p meanpath delay",
    "p2p meanpath delay value, range 0 - 1000000000");

IMI_ALI (NULL,
     bridge_vlan_priority_cmd_imi,
     "bridge <1-32> vlan <2-4094> priority <0-61440>",
     "Bridge group commands",
     "Bridge Group name for bridging",
     "Change priority for a particular instance",
     "instance id",
     "priority - bridge priority for the common instance",
     "bridge priority in increments of 4096 (Lower priority indicates greater likelihood of becoming root)");

IMI_ALI (NULL,
     mls_qos_strict_queue_cmd_imi,
     "mls qos strict queue <0-3>",
     "Multi-Layer Switch(L2/L3).",
     "Quality of Service.",
     "Configure the Queue as Strict Queue",
     "The Queue which is configured as Strict Queue",
     "The Queue ID of Strict Queue");

IMI_ALI (NULL,
     no_debug_mstp_imish_cmd_imi,
     "no debug mstp cli",
     "Negate a command or set its defaults",
     "Debugging functions (see also 'undebug')",
     "Multiple Spanning Tree Protocol (MSTP)",
     "CLI Commands");

IMI_ALI (NULL,
     no_lsp_group_cmd_imi,
     "no lsp-protect-group <0-63>",
     "Negate a command or set its defaults",
     "delete a lsp group's configuration",
     "lsp group id, range 0-63");

IMI_ALI (NULL,
     nsm_vlan_switchport_trunk_pn_cn_except_cmd_imi,
     "switchport (trunk|provider-network|customer-network) "
     "allowed vlan except VLAN_ID",
     "Set the switching characteristics of the Layer2 interface",
     "Set the Layer2 interface as trunk",
     "Set the Layer2 interface as provider network",
     "Set the Layer2 interface as Customer Network",
     "Set the VLANs that will Xmit/Rx through the Layer2 interface",
     "VLAN that will be removed",
     "Allow all VLANs except VID to Xmit/Rx through the Layer2 interface",
     "The List of VIDs that will be removed from the Layer2 interface");

IMI_ALI (NULL,
     nsm_vlan_filter_cmd_imi,
     "vlan filter WORD <2-4094>",
     "Configure VLAN parameters",
     "Configure VLAN Access Map Filter",
     "VLAN Access Map Name",
     "VLAN id");

IMI_ALI (NULL,
     set_gvrp_app_state_active_cmd_imi,
     "set (gvrp|mvrp) applicant state active IF_NAME",
     "Set values in destination routing protocol",
     "GARP Vlan Registration Protocol",
     "MRP Vlan Registration Protocol",
     "GID Applicant",
     "State of the Applicant",
     "Active State",
     "Interface name");

IMI_ALI (generic_show_func,
     show_mls_qos_maps_dscp_mutation_cmd_imi,
     "show mls qos maps dscp-mutation NAME",
     "Show running system information",
     "Multi-Layer Switch(L2/L3).",
     "Quality of Service.",
     "Select QoS map",
     "DSCP mutation map",
     "DSCP mutation map name");

IMI_ALI (NULL,
     mac_acl_host_host_cmd_imi,
     "mac acl <2000-2699> (deny|permit) MAC MASK MAC MASK <1-8>",
     "MAC access list", "MAC access list", "extended mac ACL <2000-2699>",
     "Specify packets to reject", "Specify packets to permit",
     "Source host's MAC address in HHHH.HHHH.HHHH format",
     "Source wildcard in HHHH.HHHH.HHHH format",
     "Destination host's MAC address in HHHH.HHHH.HHHH format",
     "Destination wildcard in HHHH.HHHH.HHHH format",
     "Specify packet format, (1:Ethernet II/ 2:802.3/ 4:SNMP/ 8:LLC)");

IMI_ALI (NULL,
     no_qos_pmapc_police_aggregate_cmd_imi,
     "no police-aggregate NAME",
     "Negate a command or set its defaults",
     "Specify a aggregate policer to multiple classes",
     "Specify a aggregate policer name");

IMI_ALI (NULL,
     fm_faults_test_cmd_imi,
     "faults test (raise|clear) WORD (WORD|)",
     "Fault management command",
     "Fault test command",
     "Raise a fault",
     "Clear fault",
     "Fault identifier",
     "Fault instance");

IMI_ALI (NULL,
     no_gvrp_debug_packet_cmd_imi,
     "no debug gvrp packet",
     "Negate a command or set its defaults",
     "debug - disable debug output",
     "gvrp - GVRP commands",
     "packet - do not echo packet contents to console");

IMI_ALI (NULL,
    no_ac_id_cmd_imi,
    "no ac <1-2000>",
    "Negate a command or set its defaults",
    "delete a attachment-circuit's configuration",
    "attachment-circuit id, range 1-2000");

IMI_ALI (NULL,
      no_mstp_bridge_portfast_bpdufilter_cmd_imi,
      "no bridge <1-32> spanning-tree portfast bpdu-filter",
      "Negate a command or set its defaults",
      "Bridge group commands",
      "Bridge Group name for bridging",
      "spanning-tree",
      "portfast",
      "disable bpdu filter ");

IMI_ALI (NULL,
     vlan_match_cmd_imi,
     "match mac WORD",
     "Configure VLAN Access Map Match",
     "Configure VLAN Access Map Match",
     "Mac access-list name");

IMI_ALI (NULL,
     nsm_br_ser_vlan_name_cmd_imi,
     "vlan ""<2-4094>"" type service "
     "(point-point|multipoint-multipoint) bridge <1-32> "
     "name WORD (state (enable|disable)|)",
     "Add, delete, or modify values associated with a single VLAN",
     "VLAN id",
     "VLAN Type",
     "Service VLAN",
     "Point to Point Service VLAN",
     "Multi Point to Multi Point Service VLAN",
     "Bridge group commands.",
     "Bridge group for bridging.",
     "Ascii name of the VLAN",
     "The ascii name of the VLAN",
     "Operational state of the VLAN",
     "Enable",
     "Disable");

IMI_ALI (NULL,
    lsp_group_cfg_cmd_imi,
    "config protect-type (1plus1|1plus1snc|1by1|1by1snc) work-lsp-id <1-2000> backup-lsp-id <1-2000>",
    "lsp protect group config command"
    "protect type",
    "1PLUS1 1 + 1",
    "1PLUS1 SNC",
    "1BY1 1:1",
    "1BY1SNC",
    "work lsp id",
    "work lsp id value",
    "backup lsp id",
    "backup lsp id value");

IMI_ALI (generic_show_func,
     show_gvrp_timer_cmd_imi,
     "show (gvrp|mvrp) timer IF_NAME",
     "CLI_SHOW_STR",
     "GARP Vlan Registration Protocol",
     "MRP Vlan Registration Protocol",
     "GARP timer",
     "Ifname");

IMI_ALI (NULL,
     set_gmrp_ext_filtering2_cmd_imi,
     "set (gmrp|mmrp) extended-filtering disable",
     "Enable Extended filtering Option",
     "GARP Multicast Registration Protocol",
     "MRP Multicast Registration Protocol",
     "Extended filtering Option",
     "disable");

IMI_ALI (NULL,
    tun_config_cmd_imi,
    "config (ingress (fe <1-24>|ge <1-4>) lable <0-1048575> vlan <0-4095>|egress (fe <1-24>|ge <1-4>) lable <0-1048575> vlan <0-4095> exp <0-7> peer-mac MAC)",
    "config tunnel paras: direction, interface type, interface id, lable, vlan, exp, peer-mac",
    "tun direction",
    "ingress-tun interface type",
    "fe interface id",
    "ingress-tun interface type",
    "ge interface id",
    "ingress-tun lable",
    "lable value",
    "ingress-tun vlan",
    "vlan id, 0 means vlan diable",
    "tun direction",
    "egress-tun interface type",
    "fe interface id",
    "egress-tun interface type",
    "ge interface id",
    "egress-tun lable",
    "lable value",
    "egress-tun vlan",
    "vlan id, 0 means vlan diable",
    "egress-tun exp",
    "exp value",
    "egress-tun peer mac address",
    "mac (hardware) address, in HHHH.HHHH.HHHH format");

IMI_ALI (NULL,
     no_nsm_pro_vlan_no_name_cmd_imi,
     "no vlan ""<2-4094>"" type (customer|service|backbone)",
     "Negate a command or set its defaults",
     "Add, delete, or modify values associated with a single VLAN",
     "VLAN id",
     "VLAN Type",
     "Customer VLAN",
     "Service VLAN",
     "Identifies the backbone bridge instance of the B-component.");

IMI_ALI (NULL,
     no_vlan_static_filter_cmd_imi,
     "no bridge <1-32> address MAC discard IFNAME vlan ""<2-4094>",
     "Negate a command or set its defaults",
     "Bridge group commands.",
     "Bridge group for bridging.",
     "address",
     "mac address in HHHH.HHHH.HHHH format",
     "discard",
     "Interface name",
     "VLAN",
     "vlan id");

IMI_ALI (NULL,
     mac_acl_any_host_cmd_imi,
     "mac acl <2000-2699> (deny|permit) any MAC MASK <1-8>",
     "MAC access list", "MAC access list", "extended mac ACL <2000-2699>",
     "Specify packets to reject", "Specify packets to permit",
     "Source any",
     "Destination host's MAC address in HHHH.HHHH.HHHH format",
     "Destination wildcard in HHHH.HHHH.HHHH format",
     "Specify packet format, (1:Ethernet II/ 2:802.3/ 4:SNMP/ 8:LLC)");

IMI_ALI (NULL,
     mstp_spanning_tree_linktype_cmd_imi,
     "spanning-tree link-type point-to-point",
     "spanning tree commands",
     "link-type - point-to-point or shared",
     "point-to-point - enable rapid transition");

IMI_ALI (NULL,
     no_bridge_inst_priority_cmd_imi,
     "no bridge (<1-32> | backbone) instance <1-63> priority",
     "Negate a command or set its defaults",
     "Bridge group commands",
     "Bridge Group name for bridging",
     "Specifies the backbone bridge group name",
     "Change priority for a particular instance",
     "instance id",
     "priority - Reset bridge priority to default for the instance");

IMI_ALI (NULL,
     nsm_bridge_clear_dynamic_br_fdb_cmd_imi,
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
     "Bridge group commands.",
     "Bridge group for bridging.");

IMI_ALI (generic_show_func,
     show_mac_access_grp_cmd_imi,
     "show mac-access-group",
     "Show running system information",
     "List MAC access groups");

IMI_ALI (NULL,
     ip_access_list_extended_any_host_cmd_imi,
     "ip-access-list (<100-199>|<2000-2699>) (deny|permit) ip any host A.B.C.D",
     "QoS's IP access list",
     "IP extended access list", "IP extended access list (expanded range)", "Specify packets to reject", "Specify pactets to permit",
     "Any Internet Protocol",
     "Any source host",
     "A single destination host",
     "Destination address");

IMI_ALI (NULL,
     no_mls_qos_map_dscp_mutation_cmd_imi,
     "no mls qos map dscp-mutation NAME",
     "Negate a command or set its defaults",
     "Multi-Layer Switch(L2/L3).",
     "Quality of Service.",
     "Specify maps",
     "Modify the DSCP mutation map",
     "Specify DSCP mutation name or ID");

IMI_ALI (NULL,
     no_bridge_static_cmd_imi,
     "no bridge <1-32> address MAC forward IFNAME",
     "Negate a command or set its defaults",
     "Bridge group commands.",
     "Bridge group for bridging.",
     "address",
     "mac address in HHHH.HHHH.HHHH format",
     "forward",
     "Interface name");

IMI_ALI (NULL,
     nsm_no_vlan_filter_cmd_imi,
     "no vlan filter WORD <2-4094>",
     "Negate a command or set its defaults",
     "Configure VLAN parameters",
     "Configure VLAN Access Map Filter",
     "VLAN Access Map Name",
     "VLAN id");

IMI_ALI (NULL,
 priority_one_config_cmd_imi,
 "set-priority1  <0 - 255>",
 "set priority1",
 "priority1, range <0 - 255>");

IMI_ALI (NULL,
     spanning_tree_mode_cmd_imi,
     "spanning-tree mode (stp|stp-vlan-bridge|"
     "rstp|rstp-vlan-bridge|rpvst+|mstp|provider-rstp|provider-mstp) (edge|)",
     "Spanning Tree group commands.",
     "Spanning tree mode",
     "STP mode",
     "VLAN aware STP mode",
     "RSTP mode",
     "VLAN aware RSTP mode",
     "VLAN aware RPVST+ mode",
     "MSTP mode",
     "Provider RSTP mode",
     "Provider MSTP mode",
     "Configure as Edge bridge");

IMI_ALI (NULL,
    vpn_add_mutigrp_vport_cmd_imi,
    "add-muti-vport grp-id <1-8> (ac|pw) <1-2000>",
    "add an muticast port to muticast group in vpn",
    "muticast group index",
    "id value",
    "attachment circuit",
    "pseudo wire",
    "vport id value");

IMI_ALI (NULL,
     mstp_add_l2gp_rx_statuscmd_imi,
     "switchport l2gp  psuedoRootId ROOTID:MAC (enableBPDUrx|disableBPDUrx)",
     "Set the l2gp characteristics of the Layer2 interface",
     "l2gp",
     "puedoInfo bridge/bridge mac address",
     "priority/mac-address 12345/0000.0000.0000",
     "enableBPDUrx",
     "disableBPDUrx");

IMI_ALI (generic_show_func,
     show_qos_access_list_cmd_imi,
     "show qos-access-list",
     "Show running system information",
     "List QoS (MAC & IP) access lists");

IMI_ALI (NULL,
     no_nsm_interface_alias_cmd_imi,
     "no alias",
     "Negate a command or set its defaults",
     "Alias name for the interface");

IMI_ALI (NULL,
     no_nsm_pro_vlan_mtu_cmd_imi,
     "no vlan ""<2-4094>"" type (customer|service) mtu",
     "Negate a command or set its defaults",
     "Add, delete, or modify values associated with a single VLAN",
     "VLAN id",
     "VLAN Type",
     "Customer VLAN",
     "Service VLAN",
     "Reset the MMaximum Transmission Unit value for the vlan");

IMI_ALI (NULL,
     no_qos_pmapc_police_cmd_imi,
     "no police <1-1000000> <0-20000000> exceed-action drop",
     "Negate a command or set its defaults",
     "Specify a policer for the classified traffic",
     "Specify an average traffic rate (kbps)",
     "Specify a normal burst size (bytes)",
     "Specify the action if exceed-action",
     "Drop the packet");

IMI_ALI (NULL,
     nsm_vlan_classifier_proto_cmd_imi,
     "vlan classifier rule <1-256> proto (ip|ipv6|ipx|x25|arp|rarp|atalkddp|atalkaarp|atmmulti|atmtransport|pppdiscovery|pppsession|xeroxpup|xeroxaddrtrans|g8bpqx25|ieeepup|ieeeaddrtrans|dec|decdnadumpload|decdnaremoteconsole|decdnarouting|declat|decdiagnostics|deccustom|decsyscomm|<0-65535>) encap (ethv2|snapllc|nosnapllc|) (vlan ""<2-4094>""|)",
     "Vlan commands",
     "Vlan classification commands",
     "Vlan classification rules commands",
     "Vlan classifier rule id",
     "proto - specify an ethernet protocol classification",
     "protocol - IP",
     "protocol - IPv6",
     "protocol - IPX",
     "protocol - CCITT X.25",
     "protocol - Address Resolution",
     "protocol - Reverse Address Resolution",
     "protocol - Appletalk DDP",
     "protocol - Appletalk AARP",
     "protocol - MultiProtocol Over ATM",
     "protocol - Frame-based ATM Transport",
     "protocol - PPPoE discovery",
     "protocol - PPPoE session",
     "protocol - Xerox PUP",
     "protocol - Xerox PUP Address Translation",
     "protocol - G8BPQ AX.25",
     "protocol - Xerox IEEE802.3 PUP",
     "protocol - Xerox IEEE802.3 PUP Address Translation",
     "protocol - DEC Assigned",
     "protocol - DEC DNA Dump/Load",
     "protocol - DEC DNA Remote Console",
     "protocol - DEC DNA Routing",
     "protocol - DEC LAT",
     "protocol - DEC Diagnostics",
     "protocol - DEC Customer use",
     "protocol - DEC Systems Comms Arch",
     "ethernet decimal",
     "encap - specifify packet encapsulation",
     "ethv2 - ethernet v2",
     "llc_snap - llc snap encapsulation",
     "llc_nosnap - llc without snap encapsulation",
     "Vlan",
     "Vlan Identifier");

IMI_ALI (NULL,
     no_service_policy_input_cmd_imi,
     "no service-policy input NAME",
     "Negate a command or set its defaults",
     "Service policy",
     "Ingress service policy",
     "Specify policy input name");

IMI_ALI (NULL,
     mstp_spanning_tree_autoedge_cmd_imi,
     "spanning-tree autoedge",
     "spanning-tree",
     "autoedge - enable automatic edge detection");

IMI_ALI (NULL,
     wrr_queue_weight_config_cmd_imi,
     "weight (disable|enable <0-1016> <0-1016> <0-1016> <0-1016> <0-1016> <0-1016> <0-1016> <0-1016>)",
     "WDRR queue weight configuration",
     "disable to restore the default wrr configuration",
     "eenable wrr configuration",
     "Weight value of Queue 0",
     "Weight value of Queue 1",
     "Weight value of Queue 2",
     "Weight value of Queue 3",
     "Weight value of Queue 4",
     "Weight value of Queue 5",
     "Weight value of Queue 6",
     "Weight value of Queue 7");

IMI_ALI (generic_show_func,
     show_nsm_vlan_classifier_group_interface_cmd_imi,
     "show vlan classifier group interface IFNAME",
     "Show running system information",
     "Vlan commands",
     "Vlan classification commands",
     "group activated",
     "Interface information",
     "Interface name");

IMI_ALI (NULL,
     no_nsm_bridge_group_cmd_imi,
     "no bridge-group (<1-32> | backbone)",
     "Negate a command or set its defaults",
     "Bridge group commands.",
     "Bridge group for bridging."
     "De-associates the interface with the B-component backbone bridge.");

IMI_ALI (NULL,
     no_gmrp_debug_imish_cmd_imi,
     "no debug gmrp cli",
     "Negate a command or set its defaults",
     "debug - disable debug output",
     "gmrp - GMRP commands",
     "cli - do not echo commands to console");

IMI_ALI (generic_show_func,
     show_spanning_tree_mst_detail_interface_cmd_imi,
     "show spanning-tree mst detail interface IFNAME",
     "Show running system information",
     "spanning-tree Display spanning tree information",
     "Display MST information",
     "Display detailed information",
     "Interface information",
     "Interface name");

IMI_ALI (NULL,
     nsm_vlan_switchport_hybrid_allowed_all_cmd_imi,
     "switchport hybrid allowed vlan all",
     "Set the switching characteristics of the Layer2 interface",
     "Set the Layer2 interface as hybrid",
     "Set the VLANs that will Xmit/Rx through the Layer2 interface",
     "VLANs that will be added",
     "Allow all VLANs to Xmit/Rx through the Layer2 interface");

IMI_ALI (NULL,
     qos_pmapc_set_ip_dscp_cmd_imi,
     "set ip-dscp <0-63>",
     "Setting a new value in the packet",
     "IP-DSCP",
     "Specify a new IP-DSCP value");

IMI_ALI (NULL,
     mstp_spanning_tree_vlan_restricted_tcn_cmd_imi,
     "spanning-tree vlan <2-4094> restricted-tcn",
     "spanning tree commands",
     "Set restrictions for the port of particular vlan",
     "vlan id",
     "restrict propagation of topology change notifications from port");

IMI_ALI (NULL,
     no_gmrp_debug_event_cmd_imi,
     "no debug gmrp event",
     "Negate a command or set its defaults",
     "debug - enable debug output",
     "gmrp - GMRP commands",
     "event - echo events to console");

IMI_ALI (NULL,
     hw_register_set_cmd_imi,
     "hardware register set ADDR VALUE",
      "hardware"
     "register",
     "set the value to the Register",
     "Register address in 0xhhhh format",
     "Value in hex format");

IMI_ALI (NULL,
     fm_faults_delete_cmd_imi,
     "faults delete (all|active|cleared) (WORD|)",
     "Fault management command",
     "Fault delete command",
     "Delete all faults - including cleared",
     "Delete active only",
     "Delete cleared only",
     "Process name");

IMI_ALI (NULL,
    no_meg_id_cmd_imi,
    "no id ID",
    "delete meg id",
   "Indentifying MEG id"
);

IMI_ALI (NULL,
     no_bandwidth_if_cmd_imi,
     "no bandwidth",
     "Negate a command or set its defaults",
     "Unset bandwidth informational parameter");

IMI_ALI (NULL,
     qos_pmapc_police_cmd_imi,
     "police <1-1000000> <0-20000000> exceed-action drop",
     "Specify a policer for the classified traffic",
     "Specify an average traffic rate (kbps)",
     "Specify a normal burst size (bytes)",
     "Specify the action if exceed-action",
     "Drop the packet");

IMI_ALI (generic_show_func,
     fm_show_faults_cmd_imi,
     "show faults (all|active|cleared) (WORD|)",
     "Show command",
     "Show recorded faults",
     "All faults - including cleared",
     "Active only",
     "Cleared only",
     "Process name");

IMI_ALI (generic_show_func,
     show_nsm_imishent_cmd_imi,
     "show nsm client",
     "Show running system information",
     "NSM",
     "client");

IMI_ALI (NULL,
     bridge_shutdown_multiple_spanning_tree_cmd_imi,
     "bridge shutdown <1-32>",
     "Bridge group commands",
     "reset",
     "Bridge Group name for bridging");

IMI_ALI (NULL,
     no_mstp_spanning_priority_cmd_imi,
     "no spanning-tree priority",
     "Negate a command or set its defaults",
     "Spanning Tree Commands",
     "priority - Reset bridge priority to default value for the common instance");

IMI_ALI (NULL,
    set_port_gmrp1_cmd_imi,
    "set port (gmrp|mmrp) enable (IF_NAME|all)",
    "Enable GMRP on port",
    "Layer2 Interface",
    "GARP Multicast Registration Protocol",
    "MRP Multicast Registration Protocol",
    "Enable",
    "Ifname",
    "All the ports");

IMI_ALI (NULL,
     undebug_all_nsm_cmd_imi,
     "undebug all",
     "Disable debugging functions (see also 'debug')",
     "Turn off all Debugging");

IMI_ALI (NULL,
     service_policy_input_cmd_imi,
     "service-policy input NAME",
     "Service policy",
     "Ingress service policy",
     "Specify policy input name");

IMI_ALI (NULL,
     set_gvrp_app_state_normal_cmd_imi,
     "set (gvrp|mvrp) applicant state normal IF_NAME",
     "Set values in destination routing protocol",
     "GARP Vlan Registration Protocol",
     "MRP Vlan Registration Protocol",
     "GID Applicant",
     "State of the Applicant",
     "Normal State",
     "Interface name");

IMI_ALI (NULL,
     no_bridge_vlan_cmd_imi,
     "no bridge <1-32> vlan <2-4094>",
     "Negate a command or set its defaults",
     "Bridge group commands",
     "Bridge Group name for bridging",
     "Vlan",
     "Vlan Id");

IMI_ALI (NULL,
     debug_nsm_kernel_cmd_imi,
     "debug nsm kernel",
     "Debugging functions (see also 'undebug')",
     "Network Service Module (NSM)",
     "NSM kernel");

IMI_ALI (NULL,
     debug_mstp_timer_detail_cmd_imi,
     "debug mstp timer detail",
     "Debugging functions (see also 'undebug')",
     "Multiple Spanning Tree Protocol (MSTP)",
     "MSTP Timers",
     "Detailed output");

IMI_ALI (NULL,
     no_bridge_region_cmd_imi,
     "no bridge <1-32> region",
     "Negate a command or set its defaults",
     "Bridge group commands",
     "Bridge Group name for bridging",
     "MST Region");

IMI_ALI (NULL,
     no_mstp_spanning_tree_portfast_bpduguard_cmd_imi,
     "no spanning-tree portfast bpdu-guard ",
     "Negate a command or set its defaults",
     "spanning-tree",
     "portfast",
     "disable guarding the port against reception of BPDUs");

IMI_ALI (NULL,
     mls_qos_enable_global_new_cmd_imi,
     "mls qos enable",
     "Multi-Layer Switch(L2/L3).",
     "Quality of Service.",
     "Enable QoS globally\n");

IMI_ALI (NULL,
     no_qos_pmapc_set_exp_cmd_imi,
     "no set mpls exp-bit topmost",
     "Negate a command or set its defaults",
     "Setting a new value in the packet",
     "Multi Protocol Label Switch",
     "MPLS experimental bit",
     "Set MPLS experimental value on topmost label");

IMI_ALI (NULL,
     mstp_spanning_tree_vlan_restricted_role_cmd_imi,
     "spanning-tree vlan <2-4094> restricted-role",
     "spanning tree commands",
     "Set restrictions for the port of particular vlan",
     "vlan id",
     "restrict the role of the port");

IMI_ALI (NULL,
     no_spanning_max_hops_cmd_imi,
     "no spanning-tree max-hops <1-40>",
     "Negate a command or set its defaults",
     "Spanning Tree Commands",
     "max-hops",
     "hops <1-40> - Maximum hops the BPDU will be valid");

IMI_ALI (NULL,
     bridge_group_inst_priority_cmd_imi,
     "bridge-group (<1-32> | backbone) instance <1-63> priority <0-240>",
     "Bridge group commands",
     "Bridge Group name for bridging",
     "Specifies the backbone bridge group name",
     "multiple spanning tree instance",
     "identifier",
     "port priority for a bridge",
     "port priority in increments of 16 (lower priority indicates greater likelihood of becoming root)");

IMI_ALI (NULL,
    meg_id_cmd_imi,
    "id ID",
    "configure meg id",
   "Indentifying MEG id"
);

IMI_ALI (NULL,
     no_qos_pmapc_set_vlanpriority_cmd_imi,
     "no set vlan-priority",
     "Negate a command or set its defaults",
     "Set Qos Parameters",
     "Set the priority for the queues");

IMI_ALI (NULL,
    lsp_oam_cfg_cmd_imi,
    "config lsp-id <1-2000> ccm-period <1-10000> ma-name MANAME mep-name MAPNAME ingress (fe <1-24>|ge <1-4>) egress (fe <1-24>|ge <1-4>)",
    "config lsp oam command",
    "lsp id",
    "lsp id value",
    "ccm period",
    "ccm period value",
    "ma name",
    "ma name character",
    "mep name",
    "mep name character",
    "loopback inport",
    "loopback interface type",
    "fe interface id",
    "loopback interface type",
    "ge interface id",
    "loopback outport",
    "loopback interface type",
    "fe interface id",
    "loopback interface type",
    "ge interface id");

IMI_ALI (NULL,
    no_sec_id_cmd_imi,
    "no section ID",
    "delete section id",
   "Indentifying section id"
);

IMI_ALI (NULL,
     no_nsm_vlan_switchport_trunk_vlan_cmd_imi,
     "no switchport trunk",
     "Negate a command or set its defaults",
     "Reset the switching characteristics of the Layer2 interface",
     "Reset the switching characteristics of the Layer2 interface to access");

IMI_ALI (NULL,
     mstp_spanning_tree_forceversion_cmd_imi,
     "spanning-tree force-version <0-3>",
     "Spanning Tree Commands",
     "force-version - Version of the protocol",
     "Version identifier - 0-  STP ,1- Not supported ,2 -RSTP, 3- MSTP");

IMI_ALI (NULL,
     no_mls_qos_trust_cos_path_through_dscp_cmd_imi,
     "no mls cos trust cos path-through dscp",
     "Negate a command or set its defaults",
     "Multi-Layer Switch(L2/L3).",
     "Quality of Service.",
     "Enable pass-through mode, not modify DSCP value",
     "CoS",
     "Path-through DSCP",
     "DSCP");

IMI_ALI (NULL,
     no_wrr_queue_bandwidth_cmd_imi,
     "no wrr-queue bandwidth <0-7>",
     "Negate a command or set its defaults",
     "WRR queue",
     "Specify bandwidth of QoS queue",
     "Specify Qos queue ID");

IMI_ALI (NULL,
     no_mls_qos_map_cos_queue_cmd_imi,
     "no mls qos cos-queue <0-7> <0-3>",
     "Multi-Layer Switch(L2/L3).",
     "Quality of Service.",
     "Specify cos-queue",
     "Specify Cos value with priority <0-7>",
     "Specify queue id");

IMI_ALI (NULL,
    lt_detect_size_cmd_imi,
    "lt size  <1-400>",
    "lt configure ",
    "lt size configure:size 1-400,default is 0. "

);

IMI_ALI (NULL,
     spanning_acquire_cmd_imi,
     "spanning-tree acquire",
     "Spanning Tree group commands.",
     "Enable dynamic learning of mac addresses");

IMI_ALI (NULL,
     no_mstp_spanning_max_age_cmd_imi,
     "no spanning-tree max-age ",
     "Negate a command or set its defaults",
     "Spanning Tree Commands",
     "max-age");

IMI_ALI (NULL,
     no_spanning_inst_cmd_imi,
     "no spanning-tree instance <1-63>",
     "Negate a command or set its defaults",
     "Spanning Tree Commands",
     "Multiple Spanning Tree Instance",
     "identifier");

IMI_ALI (NULL,
     nsm_bridge_protocol_rstp_vlan_cmd_imi,
     "bridge <1-32> protocol rstp (vlan-bridge|)(ring|)",
     "Bridge group commands.",
     "Bridge group for bridging.",
     "Spanning-tree protocol",
     "IEEE 801.1w rapid spanning-tree protocol",
     "VLAN aware bridge",
     "Enable Rapid Ring spanning-tree");

IMI_ALI (NULL,
     no_ip_access_list_extended_cmd_imi,
     "no ip-access-list (<100-199>|<2000-2699>) (deny|permit) ip A.B.C.D A.B.C.D A.B.C.D A.B.C.D",
     "Negate a command or set its defaults",
     "QoS's IP access list",
     "IP extended access list", "IP extended access list (expanded range)", "Specify packets to reject", "Specify pactets to permit",
     "Any Internet Protocol",
     "Source address",
     "Source wildcard bits",
     "Destination address",
     "Destination Wildcard bits");

IMI_ALI (generic_show_func,
     show_mls_qos_maps_cos_dscp_cmd_imi,
     "show mls qos maps cos-dscp",
     "Show running system information",
     "Multi-Layer Switch(L2/L3).",
     "Quality of Service.",
     "Select QoS map",
     "COS-to-DSCP map");

IMI_ALI (NULL,
     no_spanning_vlan_static_cmd_imi,
     "no mac-address-table static MAC (forward|discard) IFNAME vlan ""<2-4094>",
     "Negate a command or set its defaults",
     "Spanning Tree group commands.",
     "Static address",
     "mac address in HHHH.HHHH.HHHH format",
     "forward",
     "discard",
     "Interface name",
     "VLAN",
     "vlan id");

IMI_ALI (NULL,
     nsm_bridge_clear_dynamic_fdb_cmd_imi,
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

IMI_ALI (NULL,
     set_port_gvrp1_cmd_imi,
     "set port (gvrp|mvrp) enable (IF_NAME|all)",
     "Enable GVRP on port",
     "Layer2 Interface",
     "GARP Vlan Registration Protocol",
     "MRP Vlan Registration Protocol",
     "Enable",
     "Ifname",
     "All the ports");

IMI_ALI (generic_show_func,
     show_default_priority_cmd_imi,
     "show user-priority interface IFNAME",
     "Show running system information",
     "Display the default user priority associated with the layer2 interface",
     "Interface information",
     "Interface name");

IMI_ALI (NULL,
     bridge_multiple_spanning_tree_cmd_imi,
     "bridge <1-32> "
     "(multiple-spanning-tree|rapid-pervlan-spanning-tree|rapid-spanning-tree|spanning-tree) enable",
     "Bridge group commands",
     "Bridge Group name for bridging",
     "multiple-spanning-tree - MSTP commands",
     "rapid-pervlan-spanning-tree - RPVST+ commands",
     "rapid-spanning-tree - RSTP commands",
     "spanning-tree - STP commands",
     "enable spanning tree protocol");

IMI_ALI (NULL,
    no_pw_id_cmd_imi,
    "no pw <1-2000>",
    "Negate a command or set its defaults",
    "delete a pseudo-wire's configuration",
    "pseudo-wire id, range 1-2000");

IMI_ALI (NULL,
     nsm_vlan_switchport_trunk_pn_cn_none_cmd_imi,
     "switchport (trunk|provider-network|customer-network) allowed vlan none",
     "Set the switching characteristics of the Layer2 interface",
     "Set the Layer2 interface as trunk",
     "Set the Layer2 interface as provider network",
     "Set the Layer2 interface as Customer Network",
     "Set the VLANs that will Xmit/Rx through the Layer2 interface",
     "VLAN(s) that will be removed",
     "Allow no VLANs to Xmit/Rx through the Layer2 interface");

IMI_ALI (NULL,
     nsm_br_ser_vlan_enable_disable_cmd_imi,
     "vlan ""<2-4094>"" type service (point-point|multipoint-multipoint)"
     "bridge <1-32> state (enable|disable)",
     "Add, delete, or modify values associated with a single VLAN",
     "VLAN id",
     "VLAN Type",
     "Service VLAN",
     "Point to Point Service VLAN",
     "Multi Point to Multi Point Service VLAN",
     "Bridge group commands.",
     "Bridge group for bridging.",
     "Operational state of the VLAN",
     "Enable",
     "Disable");

IMI_ALI (NULL,
      mstp_spanning_tree_errdisable_timeout_enable_cmd_imi,
      "bridge <1-32> spanning-tree errdisable-timeout enable",
      "Bridge group commands",
      "Bridge Group name for bridging",
      "spanning-tree",
      "errdisable-timeout",
      "enable the timeout mechanism for the port to be enabled back");

IMI_ALI (NULL,
     no_wrr_queue_min_reserve_cmd_imi,
     "no wrr-queue min-reserve <0-7>",
     "Negate a command or set its defaults",
     "Wrr queue",
     "Configure the buffer size of the minimum-reserve level",
     "Specify a queue ID");

IMI_ALI (generic_show_func,
     show_nsm_vlan_brief_cmd_imi,
     "show vlan (brief | ""<2-4094>"")",
     "Show running system information",
     "Display VLAN information",
     "VLAN information for all bridges (static and dynamic)",
     "VLAN id");

IMI_ALI (NULL,
     no_qos_policy_map_cmd_imi,
     "no policy-map NAME",
     "Negate a command or set its defaults",
     "Policy map command",
     "Specify a policy-map name");

IMI_ALI (NULL,
     no_qos_pmapc_set_algorithm_cmd_imi,
     "no set algorithm",
     "Negate a command or set its defaults",
     "Set Qos Parameters",
     "Set the algorithm for egress scheduling");

IMI_ALI (NULL,
     nsm_cus_vlan_no_name_cmd_imi,
     "vlan ""<2-4094>"" type customer",
     "Add, delete, or modify values associated with a single VLAN",
     "VLAN id",
     "VLAN Type",
     "Customer VLAN");

IMI_ALI (NULL,
     wrr_queue_cos_map_cmd_imi,
     "wrr-queue cos-map <0-7> (<0-7>|<0-7> <0-7>|<0-7> <0-7> <0-7>|<0-7> <0-7> <0-7> <0-7>|<0-7> <0-7> <0-7> <0-7> <0-7>|<0-7> <0-7> <0-7> <0-7> <0-7> <0-7>|<0-7> <0-7> <0-7> <0-7> <0-7> <0-7> <0-7>|<0-7> <0-7> <0-7> <0-7> <0-7> <0-7> <0-7> <0-7>)",
     "WRR queue",
     "CoS map",
     "Specify queue ID",
     "Specify CoS value",
     "Specify CoS value",
     "Specify CoS value",
     "Specify CoS value",
     "Specify CoS value",
     "Specify CoS value",
     "Specify CoS value",
     "Specify CoS value",
     "Specify CoS value",
     "Specify CoS value",
     "Specify CoS value",
     "Specify CoS value",
     "Specify CoS value",
     "Specify CoS value",
     "Specify CoS value",
     "Specify CoS value",
     "Specify CoS value",
     "Specify CoS value",
     "Specify CoS value",
     "Specify CoS value",
     "Specify CoS value",
     "Specify CoS value",
     "Specify CoS value",
     "Specify CoS value",
     "Specify CoS value",
     "Specify CoS value",
     "Specify CoS value",
     "Specify CoS value",
     "Specify CoS value",
     "Specify CoS value",
     "Specify CoS value",
     "Specify CoS value",
     "Specify CoS value",
     "Specify CoS value",
     "Specify CoS value",
     "Specify CoS value");

IMI_ALI (NULL,
     no_vlan_switchport_pvid_cmd_imi,
     "no switchport (access | hybrid) vlan",
     "Negate a command or set its defaults",
     "Reset the switching characteristics of the Layer2 interface",
     "Set the Layer2 interface as Access",
     "Set the Layer2 interface as hybrid",
     "Reset the default VLAN for the interface");

IMI_ALI (NULL,
     spanning_multiple_spanning_tree_cmd_imi,
     "spanning-tree enable",
     "Spanning Tree Commands",
     "enable multiple spanning tree protocol");

IMI_ALI (NULL,
     bridge_group_vlan_cmd_imi,
     "bridge-group <1-32> vlan <2-4094>",
     "Bridge group commands",
     "Bridge Group name for bridging",
     "Rapid Pervlan Spanning tree instance",
     "ID");

IMI_ALI (NULL,
     set_gvrp_timer3_cmd_imi,
     "set (gvrp|mvrp) timer leaveall TIMER_VALUE IF_NAME",
     "Set GVRP leaveall timer for the specified interface",
     "GARP Vlan Registration Protocol",
     "MRP Vlan Registration Protocol",
     "GARP timer",
     "Leaveall Timer",
     "Timervalue in centiseconds",
     "Ifname");

IMI_ALI (NULL,
     debug_mstp_imish_cmd_imi,
     "debug mstp cli",
     "Debugging functions (see also 'undebug')",
     "Multiple Spanning Tree Protocol (MSTP)",
     "CLI Commands");

IMI_ALI (NULL,
     nsm_ser_vlan_name_cmd_imi,
     "vlan ""<2-4094>"" type service "
     "(point-point|multipoint-multipoint) name WORD "
     "(state (enable|disable)|)",
     "Add, delete, or modify values associated with a single VLAN",
     "VLAN id",
     "VLAN Type",
     "Service VLAN",
     "Point to Point Service VLAN",
     "Multi Point to Multi Point Service VLAN",
     "Ascii name of the VLAN",
     "The ascii name of the VLAN",
     "Operational state of the VLAN",
     "Enable",
     "Disable");

IMI_ALI (NULL,
     switchport_cmd_imi,
     "switchport",
     "Switchport");

IMI_ALI (NULL,
     mls_qos_aggregate_police_cmd_imi,
     "mls qos aggregate-police NAME <1-1000000> <1-20000> exceed-action drop",
     "Multi-Layer Switch(L2/L3).",
     "Quality of Service.",
     "Specify a policer for the classified traffic",
     "Specify aggregate-policer name",
     "Specify an average traffic rate (kbps)",
     "Specify a normal burst size (bytes)",
     "Specify the action if exceed-action",
     "Drop the packet");

IMI_ALI (NULL,
     qos_cmap_match_ip_dscp_cmd_imi,
     "match ip-dscp (<0-63>|<0-63> <0-63>|<0-63> <0-63> <0-63>|<0-63> <0-63> <0-63> <0-63>|<0-63> <0-63> <0-63> <0-63> <0-63>|<0-63> <0-63> <0-63> <0-63> <0-63> <0-63>|<0-63> <0-63> <0-63> <0-63> <0-63> <0-63> <0-63>|<0-63> <0-63> <0-63> <0-63> <0-63> <0-63> <0-63> <0-63>)",
     "Define the match creteria",
     "List IP DSCP values",
     "Specify DSCP value",
     "Specify DSCP value",
     "Specify DSCP value",
     "Specify DSCP value",
     "Specify DSCP value",
     "Specify DSCP value",
     "Specify DSCP value",
     "Specify DSCP value",
     "Specify DSCP value",
     "Specify DSCP value",
     "Specify DSCP value",
     "Specify DSCP value",
     "Specify DSCP value",
     "Specify DSCP value",
     "Specify DSCP value",
     "Specify DSCP value",
     "Specify DSCP value",
     "Specify DSCP value",
     "Specify DSCP value",
     "Specify DSCP value",
     "Specify DSCP value",
     "Specify DSCP value",
     "Specify DSCP value",
     "Specify DSCP value",
     "Specify DSCP value",
     "Specify DSCP value",
     "Specify DSCP value",
     "Specify DSCP value",
     "Specify DSCP value",
     "Specify DSCP value",
     "Specify DSCP value",
     "Specify DSCP value",
     "Specify DSCP value",
     "Specify DSCP value",
     "Specify DSCP value",
     "Specify DSCP value");

IMI_ALI (NULL,
     bridge_add_static_forward_cmd_imi,
     "bridge <1-32> address MAC forward IFNAME",
     "Bridge group commands.",
     "Bridge group for bridging.",
     "address",
     "mac address in HHHH.HHHH.HHHH format",
     "forward",
     "Interface name");

IMI_ALI (NULL,
     set_gmrp_instance_disable_cmd_imi,
     "set (gmrp|mmrp) disable vlan VLANID",
     "Disable GMRP globally for the bridge",
     "GARP Multicast Registration Protocol",
     "MRP Multicast Registration Protocol",
     "Disable",
     "The Vlan of the instance",
     "VLAN ID of the instance");

IMI_ALI (NULL,
     no_interface_arbiter_cmd_imi,
     "no if-arbiter",
     "Negate a command or set its defaults",
     "Stop arbiter to check interface information periodically");

IMI_ALI (NULL,
     no_gmrp_debug_packet_cmd_imi,
     "no debug gmrp packet",
     "Negate a command or set its defaults",
     "debug - disable debug output",
     "gmrp - GMRP commands",
     "packet - do not echo packet contents to console");

IMI_ALI (NULL,
    oam_enable_cmd_imi,
    "oam {enable | disable }",
    "oam enable or disable ",
   "oam enable or disable"
);

IMI_ALI (NULL,
     mls_qos_map_cos_dscp_cmd_imi,
     "mls qos map cos-dscp <0-63> <0-63> <0-63> <0-63> <0-63> <0-63> <0-63> <0-63>",
     "Multi-Layer Switch(L2/L3).",
     "Quality of Service.",
     "Specify maps",
     "Modify the CoS-to-DSCP map",
     "DSCP value, mapping to CoS 0",
     "DSCP value, mapping to CoS 1",
     "DSCP value, mapping to CoS 2",
     "DSCP value, mapping to CoS 3",
     "DSCP value, mapping to CoS 4",
     "DSCP value, mapping to CoS 5",
     "DSCP value, mapping to CoS 6",
     "DSCP value, mapping to CoS 7");

IMI_ALI (NULL,
    dm_way_select_enable_cmd_imi,
    "dm {enable|disable}",
    "dm  configure ",
    "lt enable configure:enable or disable . "
);

IMI_ALI (NULL,
     no_mstp_spanning_tree_vlan_restricted_tcn_cmd_imi,
     "no spanning-tree vlan <2-4094> restricted_tcn",
     "Negate a command or set its defaults",
     "spanning tree commands",
     "remove restrictions for the port of particular vlan",
     "vlan id",
     "remove restriction on propagation of topology change notifications from port");

IMI_ALI (NULL,
    clocktype_config_cmd_imi,
    "set-clock-type (tc|oc|bc)",
    "set clock type",
    "transparent clock",
    "ordinary clock",
    "boundary clock");

IMI_ALI (NULL,
     no_nsm_vlan_no_name_cmd_imi,
     "no vlan <2-4094>",
     "Negate a command or set its defaults",
     "Add, delete, or modify values associated with a single VLAN",
     "VLAN id");

IMI_ALI (NULL,
      mstp_span_portfast_bpduguard_cmd_imi,
      "spanning-tree portfast bpdu-guard",
      "spanning-tree",
      "portfast",
      "guard the portfast ports against bpdu receive");

IMI_ALI (NULL,
     no_vpn_id_cmd_imi,
     "no vpn <1-2000>",
     "Negate a command or set its defaults",
    "delete a vpn's configuration",
    "vpn id, range 1-2000");

IMI_ALI (NULL,
     gmrp_debug_imish_cmd_imi,
     "debug gmrp cli",
     "debug - enable debug output",
     "gmrp - GMRP commands",
     "cli - echo commands to console");

IMI_ALI (NULL,
     mstp_clear_spanning_tree_interface_bridge_cmd_imi,
     "clear spanning-tree statistics interface IFNAME (instance <1-63>| vlan <1-4094>) bridge <1-32>",
     "Reset functions",
     "Display spanning-tree information",
     "statistics of the BPDUs",
     "interface",
     "Interface name",
     "Display instance information",
     "instance_id",
     "vlan",
     "vlan id to be associated to instance",
     "bridge",
     "bridge id");

IMI_ALI (NULL,
     set_gmrp_timer3_cmd_imi,
     "set (gmrp|mmrp) timer leaveall TIMER_VALUE IF_NAME",
     "Set GMRP leaveall timer for the specified interface",
     "GARP Multicast Registration Protocol",
     "MRP Multicast Registration Protocol",
     "GARP timer",
     "Leaveall Timer",
     "Timervalue in centiseconds",
     "Ifname");

IMI_ALI (NULL,
     no_gmrp_debug_all_cmd_imi,
     "no debug gmrp all",
     "Negate a command or set its defaults",
     "debug - enable debug output",
     "gmrp - GMRP commands",
     "all - turn off all debugging");

IMI_ALI (NULL,
     gvrp_debug_event_cmd_imi,
     "debug gvrp event",
     "debug - enable debug output",
     "gvrp - GVRP commands",
     "event - echo events to console");

IMI_ALI (NULL,
     nsm_vlan_switchport_trunk_pn_cn_all_cmd_imi,
     "switchport (trunk|provider-network|customer-network) allowed vlan all",
     "Set the switching characteristics of the Layer2 interface",
     "Set the Layer2 interface as trunk",
     "Set the Layer2 interface as provider network",
     "Set the Layer2 interface as Customer Network",
     "Set the VLANs that will Xmit/Rx through the Layer2 interface",
     "VLAN(s) that will be added",
     "Allow all VLANs to Xmit/Rx through the Layer2 interface");

IMI_ALI (generic_show_func,
     show_gvrp_configuration_all_cmd_imi,
     "show (gvrp|mvrp) configuration",
     "CLI_SHOW_STR",
     "GARP Vlan Registration Protocol",
     "MRP Vlan Registration Protocol",
     "Display GVRP configuration");

IMI_ALI (NULL,
     no_ip_mroute_prefix_cmd_imi,
     "no ip mroute A.B.C.D/M (" "static|rip|ospf|bgp|isis|" ")",
     "Negate a command or set its defaults",
     "Internet Protocol (IP)",
     "Configure static multicast routes",
     "Source prefix",
     "Static routes", "Routing Information Protocol (RIP)", "Open Shortest Patch First (OSPF)", "Border Gateway Protocol (BGP)", "ISO IS-IS");

IMI_ALI (generic_show_func,
     show_mls_qos_maps_ip_prec_dscp_cmd_imi,
     "show mls qos maps ip-prec-dscp",
     "Show running system information",
     "Multi-Layer Switch(L2/L3).",
     "Quality of Service.",
     "Select QoS map",
     "IP-Prec-to-DSCP map");

IMI_ALI (NULL,
     no_flowcontrol_cmd_imi,
     "no flowcontrol",
     "Negate a command or set its defaults",
     "IEEE 802.3x Flow Control");

IMI_ALI (NULL,
     spanning_vlan_priority_cmd_imi,
     "spanning-tree vlan <2-4094> priority <0-61440>",
     "Spanning Tree Commands",
     "Change priority for a particular vlan",
     "vlan id",
     "priority - bridge priority for the common instance",
     "bridge priority in increments of 4096 (Lower priority indicates greater likelihood of becoming root)");

IMI_ALI (generic_show_func,
     show_spanning_tree_rpvst_vlan_cmd_imi,
     "show spanning-tree rpvst+ vlan <1-4094>",
     "Show running system information",
     "spanning-tree Display spanning tree information",
     "Display RPVST+ information",
     "Display vlan information",
     "vlan-id");

IMI_ALI (NULL,
      mstp_spanning_tree_errdisable_timeout_enable_cmd1_imi,
      "spanning-tree errdisable-timeout enable",
      "spanning-tree",
      "errdisable-timeout",
      "enable the timeout mechanism for the port to be enabled back");

IMI_ALI (NULL,
     gmrp_debug_all_cmd_imi,
     "debug gmrp all",
     "debug - enable debug output",
     "gmrp - GMRP commands",
     "all - turn on all debugging");

IMI_ALI (NULL,
    lsp_group_sw_cmd_imi,
    "config current-work-lsp <1-2000> sw-status (clear|lock|force) lock-lsp <1-2000>",
    "lsp protect group config command",
    "switch lsp",
    "switch lsp id",
    "switch status",
    "protect switch clear",
    "protect switch lock",
    "protect switch force",
    "lock lsp",
    "lock lsp id");

IMI_ALI (NULL,
     no_mls_qos_map_dscp_cos_cmd_imi,
     "no mls qos map dscp-cos NAME",
     "Negate a command or set its defaults",
     "Multi-Layer Switch(L2/L3).",
     "Quality of Service.",
     "Specify maps",
     "Modify DSCP-to-COS map",
     "Specify DSCP-to-COS map name");

IMI_ALI (NULL,
     no_debug_nsm_ha_cmd_imi,
     "no debug nsm ha",
     "Negate a command or set its defaults",
     "Debugging functions (see also 'undebug')",
     "Network Service Module (NSM)",
     "NSM High Availability");

IMI_ALI (NULL,
     clear_gmrp_br_statistics2_cmd_imi,
     "clear (gmrp|mmrp) statistics vlanid <1-4094> bridge <1-32>",
     "Clear GMRP statistics for given vlan",
     "GARP Multicast Registration Protocol",
     "MRP Multicast Registration Protocol",
     "GMRP Statistics",
     "Vlanid",
     "Vlanid value <1-4094>",
     "Bridge instance",
     "Bridge instance name");

IMI_ALI (NULL,
     no_switchport_ratelimit_cmd_imi,
     "no storm-control (broadcast|multicast|dlf) level",
     "Negate a command or set its defaults",
     "Reset the switching characteristics of Layer2 interface",
     "Reset Broadcast Rate Limiting of layer2 Interface",
     "Reset Multicast Rate Limiting of layer2 Interface",
     "Reset DLF Broadcast Rate Limiting of layer2 Interface",
     "Threshhold Level");

IMI_ALI (NULL,
     set_gmrp_instance_enable_cmd_imi,
     "set (gmrp|mmrp) enable vlan VLANID",
     "Enable GMRP globally for the bridge",
     "GARP Multicast Registration Protocol",
     "MRP Multicast Registration Protocol",
     "Enable",
     "The Vlan of the instance",
     "VLAN ID of the instance");

IMI_ALI (NULL,
     no_bridge_group_inst_cmd_imi,
     "no bridge-group (<1-32> | backbone) instance (<1-63> | te-msti)",
     "Negate a command or set its defaults",
     "Bridge group commands",
     "Bridge Group name for bridging",
     "Specifies the backbone bridge group name",
     "multiple spanning tree instance",
     "ID",
     "MSTI to be the traffic engineering MSTI instance");

IMI_ALI (NULL,
     nsm_br_pro_vlan_mtu_cmd_imi,
     "vlan ""<2-4094>"" type (customer|service) mtu MTU_VAL bridge <1-32>",
     "Add, delete, or modify values associated with a single VLAN",
     "VLAN id",
     "VLAN Type",
     "Customer VLAN",
     "Service VLAN",
     "MTU Value",
     "The Maximum Transmission Unit value for the vlan",
     "Bridge group commands.",
     "Bridge group for bridging.");

IMI_ALI (NULL,
     mac_acl_host_any_cmd_imi,
     "mac acl <2000-2699> (deny|permit) MAC MASK any <1-8>",
     "MAC access list", "MAC access list", "extended mac ACL <2000-2699>",
     "Specify packets to reject", "Specify packets to permit",
     "Source host's MAC address in HHHH.HHHH.HHHH format",
     "Source wildcard in HHHH.HHHH.HHHH format",
     "Destination any",
     "Specify packet format, (1:Ethernet II/ 2:802.3/ 4:SNMP/ 8:LLC)");

IMI_ALI (NULL,
     nsm_bridge_span_disable_cmd_imi,
     "spanning-tree (disable|enable)",
     "spannning tree command",
     "Disable spanning tree on the interface for default bridge",
     "Enable spanning tree on the interface for default bridge");

IMI_ALI (NULL,
     bridge_max_hops_cmd_imi,
     "bridge <1-32> max-hops <1-40>",
     "Bridge group commands",
     "Bridge Group name for bridging",
     "max-hops",
     "hops <1-40> - Maximum hops the BPDU will be valid");

IMI_ALI (NULL,
     nsm_vlan_enable_disable_cmd_imi,
     "vlan ""<2-4094>"" state (enable|disable)",
     "Add, delete, or modify values associated with a single VLAN",
     "VLAN id",
     "Operational state of the VLAN",
     "Enable",
     "Disable");

IMI_ALI (NULL,
     no_debug_hal_events_cmd_imi,
     "no debug nsm hal events",
     "Negate a command or set its defaults",
     "Debugging functions (see also 'undebug')",
     "Network Service Module (NSM)",
     "Hardware Abstraction Layer",
     "NSM events");

IMI_ALI (NULL,
     no_qos_class_map_cmd_imi,
     "no class-map NAME",
     "Negate a command or set its defaults",
     "Class map command",
     "Specify a class-map name");

IMI_ALI (NULL,
     spanning_vlan_path_cost_cmd_imi,
     "spanning-tree vlan <2-4094> path-cost <1-200000000>",
     "Spanning Tree Commands",
     "rapid Pervlan Spanning Tree Instance",
     "identifier",
     "path cost for a port",
     "path cost in range <1-200000000>"
     "(lower path cost indicates greater likelihood of becoming root)");

IMI_ALI (NULL,
     mls_qos_da_priority_cos_queue_cmd_imi,
     "mls qos da-priority-override (cos | queue | both)",
     "Multi-Layer Switch(L2/L3).",
     "Quality of Service.",
     "Destination Address Priority Override",
     "Select COS",
     "Select QUEUE",
     "Select both");

IMI_ALI (NULL,
     nsm_vlan_switchport_mode_ce_cmd_imi,
     "switchport mode customer-edge (access|hybrid|trunk)",
     "Set the switching characteristics of the Layer2 interface",
     "Set the mode of the Layer2 interface",
     "Set the Layer2 interface as Customer Edge",
     "Set the Layer2 interface as Access",
     "Set the Layer2 interface as hybrid",
     "Set the Layer2 interface as trunk");

IMI_ALI (NULL,
     debug_nsm_events_cmd_imi,
     "debug nsm events",
     "Debugging functions (see also 'undebug')",
     "Network Service Module (NSM)",
     "NSM events");

IMI_ALI (NULL,
      mac_prio_override_cmd_imi,
      "bridge <1-32> mac-priority-override mac-address MAC "
      "interface IFNAME vlan VLANID "
      "(static|static-priority-override|static-mgmt|"
       "static-mgmt-priority-overide) "
      "priority <0-7> ",
       "Bridge group commands.",
       "Bridge group for bridging.",
      "mac priority overide",
      "mac address",
      "mac address in HHHH.HHHH.HHHH format",
      "Interface information",
      "Interface name",
      "Add, delete, or modify values associated with a single VLAN",
      "VLAN Id",
      "The MAC is a static entry",
      "The MAC is a static with priority override",
      "The MAC is a Static Management ",
      "The MAC is a Static Management with priority override",
      "priority",
      "priority value");

IMI_ALI (NULL,
     ip_access_list_extended_any_mask_cmd_imi,
     "ip-access-list (<100-199>|<2000-2699>) (deny|permit) ip any A.B.C.D A.B.C.D",
     "QoS's IP access list",
     "IP extended access list", "IP extended access list (expanded range)", "Specify packets to reject", "Specify pactets to permit",
     "Any Internet Protocol",
     "Any source host",
     "Destination address",
     "Destination Wildcard bits");

IMI_ALI (NULL,
     debug_mstp_proto_detail_cmd_imi,
     "debug mstp protocol detail",
     "Debugging functions (see also 'undebug')",
     "Multiple Spanning Tree Protocol (MSTP)",
     "Protocol",
     "detail");

IMI_ALI (NULL,
     debug_mstp_proto_cmd_imi,
     "debug mstp protocol",
     "Debugging functions (see also 'undebug')",
     "Multiple Spanning Tree Protocol (MSTP)",
     "protocol");

IMI_ALI (generic_show_func,
     show_mls_qos_cmd_imi,
     "show mls qos",
     "Show running system information",
     "Multi-Layer Switch(L2/L3).",
     "Quality of Service.");

IMI_ALI (generic_show_func,
     show_nsm_vlan_classifier_rule_cmd_imi,
     "show vlan classifier rule(<1-256>|)",
     "Show running system information",
     "Vlan commands",
     "Vlan classification commands",
     "Vlan classifier rule id",
     "Rule Id");

IMI_ALI (NULL,
      no_mstp_span_portfast_bpduguard_cmd_imi,
      "no spanning-tree portfast bpdu-guard",
      "Negate a command or set its defaults",
      "spanning-tree",
      "portfast",
      "guard the portfast ports against bpdu receive");

IMI_ALI (NULL,
     ip_access_list_standard_any_cmd_imi,
     "ip-access-list (<1-99>|<1300-1999>) (deny|permit) any",
     "QoS's IP access list",
     "IP standard access list", "IP standard access list (expanded range)", "Specify packets to reject", "Specify pactets to permit",
     "Any source host");

IMI_ALI (NULL,
     undebug_nsm_ha_all_cmd_imi,
     "undebug nsm ha all",
     "Disable debugging functions (see also 'debug')",
     "Network Service Module (NSM)",
     "NSM High Availability",
     "All NSM High Availability events");

IMI_ALI (NULL,
     undebug_nsm_packet_cmd_imi,
     "undebug nsm packet (recv|send|) (detail|)",
     "Disable debugging functions (see also 'debug')",
     "Network Service Module (NSM)",
     "NSM packets",
     "NSM receive packets",
     "NSM send packets",
     "Detailed information display");

IMI_ALI (NULL,
     mstp_spanning_forward_time_cmd_imi,
     "spanning-tree forward-time <4-30>",
     "Spanning Tree Commands",
     "forward-time - forwarding delay time",
     "forward delay time in seconds");

IMI_ALI (generic_show_func,
     show_policy_map_cmd_imi,
     "show policy-map",
     "Show running system information",
     "Policy map entry");

IMI_ALI (NULL,
     set_gmrp_registration4_cmd_imi,
     "set (gmrp|mmrp) registration restricted IF_NAME",
     "Set GMRP Registration to Restricted Registration",
     "GARP Multicast Registration Protocol",
     "MRP Multicast Registration Protocol",
     "GMRP Registration",
     "Restricted Registration",
     "Interface name");

IMI_ALI (NULL,
    lm_on_demand_detect_enable_cmd_imi,
    "lm-on-demand {enable|disable}",
    "lm on-demand configure ",
    "lm on-demand enable configure:enable or disable ."
);

IMI_ALI (NULL,
    creat_peer_mep_cmd_imi,
    "peer-mep ID type{source | destinnation |bidirectional }",
    "configure peer-mep mep id ,and the type source ,destinnation or bidirectional }",
    "creat peer mep"
);

IMI_ALI (NULL,
     no_mstp_add_l2gp_cmd_imi,
     "no switchport l2gp",
     "clear the l2gp characteristics of the Layer2 interface",
     "l2gp",
     "set mode as switch port");

IMI_ALI (NULL,
     qos_pmapc_set_exp_cmd_imi,
     "set mpls exp-bit topmost <0-7>",
     "Setting a new value in the packet",
     "Multi Protocol Label Switch",
     "MPLS experimental bit",
     "Set MPLS experimental value on topmost label",
     "Specify experimental value");

IMI_ALI (NULL,
     mstp_bridge_spanning_tree_forceversion_cmd_imi,
     "bridge <1-32> spanning-tree force-version <0-3>",
     "Bridge group commands",
     "Bridge Group name for bridging",
     "spanning-tree" ,
     "force-version - Version of the protocol",
     "Version identifier - 0-  STP ,1- Not supported ,2 -RSTP, 3- MSTP");

IMI_ALI (NULL,
     clear_gmrp_statistics2_cmd_imi,
     "clear (gmrp|mmrp) statistics vlanid <1-4094>",
     "Clear GMRP statistics for given vlan",
     "GARP Multicast Registration Protocol",
     "MRP Multicast Registration Protocol",
     "GMRP Statistics",
     "Vlanid",
     "Vlanid value <1-4094>");

IMI_ALI (NULL,
     no_mstp_bridge_priority_cmd_imi,
     "no bridge <1-32> priority",
     "Negate a command or set its defaults",
     "Bridge group commands",
     "Bridge Group name for bridging",
     "priority - Reset bridge priority to default value for the common instance");

IMI_ALI (NULL,
      mstp_spanning_tree_errdisable_timeout_interval_cmd_imi,
      "bridge <1-32> spanning-tree errdisable-timeout interval <10-1000000>",
      "Bridge group commands",
      "Bridge Group name for bridging",
      "spanning-tree",
      "errdisable-timeout",
      "interval after which port shall be enabled",
      "errdisable-timeout interval in seconds");

IMI_ALI (NULL,
     debug_nsm_ha_cmd_imi,
     "debug nsm ha",
     "Debugging functions (see also 'undebug')",
     "Network Service Module (NSM)",
     "NSM High Availability");

IMI_ALI (NULL,
     no_nsm_vlan_mtu_cmd_imi,
     "no vlan ""<2-4094>"" mtu",
     "Negate a command or set its defaults",
     "Add, delete, or modify values associated with a single VLAN",
     "VLAN id",
     "Reset the MMaximum Transmission Unit value for the vlan");

IMI_ALI (NULL,
     mstp_spanning_tree_restricted_tcn_cmd_imi,
     "spanning-tree restricted-tcn",
     "spanning tree commands",
     "restrict propagation of topology change and received topology change notifications");

IMI_ALI (NULL,
     qos_cmap_match_access_group_cmd_imi,
     "match access-group NAME",
     "Define the match creteria",
     "List IP or MAC access contol lists",
     "Specify ACL list name");

IMI_ALI (NULL,
     spanning_max_hops_cmd_imi,
     "spanning-tree max-hops <1-40>",
     "Spanning Tree Commands",
     "max-hops",
     "hops <1-40> - Maximum hops the BPDU will be valid");

IMI_ALI (NULL,
     no_nsm_vlan_switchport_hybrid_vlan_cmd_imi,
     "no switchport hybrid",
     "Negate a command or set its defaults",
     "Reset the switching characteristics of the Layer2 interface",
     "Reset the switching characteristics of the Layer2 interface to access");

IMI_ALI (NULL,
     undebug_nsm_ha_cmd_imi,
     "undebug nsm ha",
     "Disable debugging functions (see also 'debug')",
     "Network Service Module (NSM)",
     "NSM High Availability");

IMI_ALI (NULL,
    vpn_add_mutigrp_cmd_imi,
    "add-mutigrp id <1-8> muti-mac MAC",
    "add an muticast group in vpn",
    "muticast group index",
    "id value",
    "muticast mac address",
    "mac (hardware) address, in HHHH.HHHH.HHHH format");

IMI_ALI (NULL,
     nsm_vlan_switchport_hybrid_delete_cmd_imi,
     "switchport (hybrid) allowed vlan remove VLAN_ID",
     "Set the switching characteristics of the Layer2 interface",
     "Set the Layer2 interface as hybrid",
     "Set the VLANs that will Xmit/Rx through the Layer2 interface",
     "VLAN that will be removed",
     "Remove a VLAN to Xmit/Rx through the Layer2 interface",
     "The List of the VLAN IDs that will be removed from the Layer2 interface");

IMI_ALI (NULL,
     set_gvrp_registration_fixed_cmd_imi,
     "set (gvrp|mvrp) registration fixed IF_NAME",
     "Set GVRP Registration to Fixed Registration",
     "GARP Vlan Registration Protocol",
     "MRP Vlan Registration Protocol",
     "GVRP Registration",
     "Fixed Registration",
     "Ifname");

IMI_ALI (NULL,
     nsm_vlan_classifier_group_add_del_rule_cmd_imi,
     "vlan classifier group <1-16> (add | delete) rule <1-256>",
     "Vlan commands",
     "Vlan classification commands",
     "Vlan classifier groups commands",
     "Vlan classifier group id",
     "Command add rule to a group",
     "Command delete rule from a group",
     "Vlan classifier rule",
     "Vlan classifier rule id");

IMI_ALI (generic_show_func,
     show_spanning_tree_rpvst_config_cmd_imi,
     "show spanning-tree rpvst+ config",
     "Show running system information",
     "spanning-tree Display spanning tree information",
     "Display RPVST information",
     "Display Configuration information");

IMI_ALI (NULL,
     mstp_spanning_tree_restricted_role_cmd_imi,
     "spanning-tree restricted-role",
     "spanning tree commands",
     "restrict the role of the port");

IMI_ALI (NULL,
     mstp_clear_spanning_tree_detected_protocols_cmd_imi,
     "clear spanning-tree detected protocols bridge <1-32>",
     "clear",
     "spanning-tree",
     "detected",
     "protocols",
     "bridge",
     "NAME - bridge name");

IMI_ALI (NULL,
     mstp_bridge_group_priority_cmd_imi,
     "bridge-group <1-32> priority <0-240>",
     "Bridge group commands",
     "Bridge Group name for bridging",
     "port priority for a bridge",
     "port priority in increments of 16 (lower priority indicates greater "
     "likelihood of becoming root)");

IMI_ALI (NULL,
     clear_gvrp_statistics_all_cmd_imi,
     "clear (gvrp|mvrp) statistics all",
     "Reset functions",
     "GARP Vlan Registration Protocol",
     "MRP Vlan Registration Protocol",
     "GVRP Statistics",
     "All bridges");

IMI_ALI (NULL,
 ac_priority_cmd_imi,
 "priority (disable|enable <0-7>)",
 "config priority paras",
 "disable car",
 "enable car",
 "priority paras");

IMI_ALI (generic_show_func,
     show_policy_map_name_cmd_imi,
     "show policy-map NAME",
     "Show running system information",
     "Policy map entry",
     "Specify policy map name");

IMI_ALI (NULL,
     no_mls_qos_map_ip_prec_dscp_cmd_imi,
     "no mls qos map ip-prec-dscp",
     "Negate a command or set its defaults",
     "Multi-Layer Switch(L2/L3).",
     "Quality of Service.",
     "Specify maps",
     "Modify the IP-precedence-to-DSCP map");

IMI_ALI (NULL,
     mac_access_list_host_host_cmd_imi,
     "mac-access-list <2000-2699> (deny|permit) MAC MASK MAC MASK <1-8>",
     "QOS's MAC access list", "extended mac ACL <2000-2699>",
     "Specify packets to reject", "Specify packets to permit",
     "Source host's MAC address in HHHH.HHHH.HHHH format",
     "Source wildcard in HHHH.HHHH.HHHH format",
     "Destination host's MAC address in HHHH.HHHH.HHHH format",
     "Destination wildcard in HHHH.HHHH.HHHH format",
     "Specify packet format, (1:Ethernet II/ 2:802.3/ 4:SNMP/ 8:LLC)");

IMI_ALI (NULL,
     no_nsm_vlan_classifier_activate_cmd_imi,
     "no vlan classifier activate <1-16>",
     "Negate a command or set its defaults",
     "Vlan commands",
     "Vlan classification commands",
     "Vlan classifier activation commands",
     "Vlan classification groups commands",
     "Vlan classifier group id");

IMI_ALI (NULL,
     nsm_bridge_clear_br_fdb_cmd_imi,
     "clear mac address-table (static|multicast) bridge <1-32>",
     "Clear the Forwarding database",
     "Clear layer 2 MAC entries",
     "Clear all Entries in the forwarding database",
     "Clear all entries configured through management",
     "Clear all multicast entries",
     "Bridge group commands.",
     "Bridge group for bridging.");

IMI_ALI (NULL,
     set_gmrp_registration1_cmd_imi,
     "set (gmrp|mmrp) registration normal IF_NAME",
     "Set GMRP Registration to Normal Registration",
     "GARP Multicast Registration Protocol",
     "MRP Multicast Registration Protocol",
     "GMRP Registration",
     "Normal Registration",
     "Ifname");

IMI_ALI (NULL,
 priority_two_config_cmd_imi,
 "set-priority2  <0 - 255>",
 "set priority2",
 "priority2, range <0 - 255>");

IMI_ALI (NULL,
     set_gvrp_timer2_cmd_imi,
     "set (gvrp|mvrp) timer leave TIMER_VALUE IF_NAME",
     "Set GVRP leave timer for the specified interface",
     "GARP Vlan Registration Protocol",
     "MRP Vlan Registration Protocol",
     "GARP timer",
     "Leave Timer",
     "Timervalue in centiseconds",
     "Ifname");

IMI_ALI (NULL,
     mls_qos_dscp_cos_cmd_imi,
     "mls qos dscp-cos NAME",
     "Multi-Layer Switch(L2/L3).",
     "Quality of Service.",
     "Specify DSCP-to-COS",
     "Specify DSCP-to-COS map name");

IMI_ALI (generic_show_func,
     show_ip_rpf_cmd_imi,
     "show ip rpf A.B.C.D",
     "Show running system information",
     "Internet Protocol (IP)",
     "Display RPF information for multicast source",
     "IP address of multicast source");

IMI_ALI (NULL,
     qos_cmap_match_l4_port_cmd_imi,
     "match layer4 (source-port|destination-port) <1-65535>",
     "Define the match criteria",
     "Specify TCP/UDP port",
     "Specify source TCP/UDP port",
     "Specify destination TCP/UDP port",
     "TCP/UDP port value");

IMI_ALI (generic_show_func,
     show_user_prio_regen_table_cmd_imi,
     "show user-priority-regen-table interface IFNAME",
     "Show running system information",
     "Display the User priority to regenerated user priority mapping associated with the layer2 interface",
     "Interface information", "Interface name");

IMI_ALI (NULL,
     ip_access_list_extended_mask_host_cmd_imi,
     "ip-access-list (<100-199>|<2000-2699>) (deny|permit) ip A.B.C.D A.B.C.D host A.B.C.D",
     "QoS's IP access list",
     "IP extended access list", "IP extended access list (expanded range)", "Specify packets to reject", "Specify pactets to permit",
     "Any Internet Protocol",
     "Source address",
     "Source wildcard bits",
     "A single destination host",
     "Destination address");

IMI_ALI (NULL,
     mstp_spanning_cisco_interop_cmd_imi,
     "spanning-tree cisco-interoperability ( enable | disable)",
     "Spanning Tree Commands",
     "Configure CISCO Interoperability",
     "Enable CISCO Interoperability",
     "Disable CISCO Interoperability");

IMI_ALI (NULL,
      mstp_spanning_tree_pathcost_method_cmd_imi,
      "bridge <1-32> spanning-tree pathcost method (short|long)",
      "Bridge group commands",
      "Bridge Group name for bridging",
      "spanning-tree",
      "Spanning tree pathcost options",
      "Method to calculate default port path cost",
      "short - Use 16 bit based values for default port path costs",
      "long  - Use 32 bit based values for default port path costs");

IMI_ALI (NULL,
     service_policy_output_cmd_imi,
     "service-policy output NAME",
     "Service policy",
     "Egress service policy",
     "Specify policy output name");

IMI_ALI (NULL,
     wrr_queue_bandwidth_cmd_imi,
     "wrr-queue bandwidth <1-65535> <1-65535> <1-65535> <1-65535> <1-65535> <1-65535> <1-65535> <1-65535>",
     "WRR queue",
     "Specify bandwidth ratios",
     "Specify the weight of queue 0",
     "Specify the weight of queue 1",
     "Specify the weight of queue 2",
     "Specify the weight of queue 3",
     "Specify the weight of queue 4",
     "Specify the weight of queue 5",
     "Specify the weight of queue 6",
     "Specify the weight of queue 7");

IMI_ALI (NULL,
     no_mls_qos_wrr_weight_queue_cmd_imi,
     "no mls qos wrr-weight <0-3>",
      "Multi-Layer Switch(L2/L3).",
      "Quality of Service.",
     "Wrr weight",
     "Configure the queue id",
     "Specify a weight");

IMI_ALI (NULL,
     bridge_region_cmd_imi,
     "bridge <1-32> region REGION_NAME",
     "Bridge group commands",
     "Bridge Group name for bridging",
     "MST Region",
     "Name of the region");

IMI_ALI (generic_show_func,
     show_spanning_tree_pathcost_method_cmd_imi,
     "show bridge <1-32> spanning-tree pathcost method",
     "Show running system information",
     "Bridge group commands",
     "Bridge Group name for bridging",
     "Spanning tree topology",
     "Show Spanning pathcost options",
     "Default pathcost calculation method");

IMI_ALI (NULL,
    ac_config_cmd_imi,
    "config (fe <1-24>|ge <1-4>) (vlan <1-4095>|)",
    "config ac paras: interface type, interface id, vlan",
    "interface type",
    "fe interface id",
    "interface type",
    "ge interface id",
    "config vlan info",
    "vlan id, 0-4095");

IMI_ALI (NULL,
     clear_gmrp_statistics1_cmd_imi,
     "clear (gmrp|mmrp) statistics all",
     "Clear GMRP statistics for all vlans",
     "GARP Multicast Registration Protocol",
     "MRP Multicast Registration Protocol",
     "GMRP Statistics",
     "All vlans");

IMI_ALI (NULL,
     no_mls_qos_vlan_priority_cos_queue_cmd_imi,
     "no mls qos vlan-priority-override",
     "Negate a command or set its defaults",
     "Multi-Layer Switch(L2/L3).",
     "Quality of Service.",
     "Vlan Priority Override");

IMI_ALI (NULL,
     no_bridge_instance_vlan_cmd_imi,
     "no bridge (<1-32> | backbone) instance <1-63> vlan VLANID",
     "Negate a command or set its defaults",
     "Bridge group commands",
     "Bridge Group name for bridging",
     "Specifies the backbone bridge group name",
     "Instance",
     "ID",
     "Delete the association of vlan with this instance",
     "vlanid associated with instance");

IMI_ALI (NULL,
     no_mstp_bridge_transmit_hold_count_cmd_imi,
     "no bridge <1-32> transmit-holdcount",
     "Negate a command or set its defaults",
     "Bridge group commands",
     "Bridge Group name for bridging",
     "transmit hold count - set count to default");

IMI_ALI (NULL,
     clear_gvrp_statistics_cmd_imi,
     "clear (gvrp|mvrp) statistics",
     "Reset functions",
     "GARP Vlan Registration Protocol",
     "MRP Vlan Registration Protocol",
     "GVRP Statistics");

IMI_ALI (NULL,
     no_mac_acl_host_any_cmd_imi,
     "no mac acl <2000-2699> (deny|permit) MAC MASK any <1-8>",
     "Negate a command or set its defaults",
     "MAC access list", "MAC access list", "extended mac ACL <2000-2699>",
     "Specify packets to reject", "Specify packets to permit",
     "Source host's MAC address in HHHH.HHHH.HHHH format",
     "Source wildcard in HHHH.HHHH.HHHH format",
     "Destination any",
     "Specify packet format, (1:Ethernet II/ 2:802.3/ 4:SNMP/ 8:LLC)");

IMI_ALI (NULL,
     nsm_pro_vlan_mtu_cmd_imi,
     "vlan ""<2-4094>"" type (customer|service) mtu MTU_VAL",
     "Add, delete, or modify values associated with a single VLAN",
     "VLAN id",
     "VLAN Type",
     "Customer VLAN",
     "Service VLAN",
     "MTU Value",
     "The Maximum Transmission Unit value for the vlan");

IMI_ALI (NULL,
     no_mstp_bridge_max_age_cmd_imi,
     "no bridge <1-32> max-age",
     "Negate a command or set its defaults",
     "Bridge group commands",
     "Bridge Group name for bridging",
     "max-age - set time to listen for root bridge to default");

IMI_ALI (NULL,
    lb_detect_size_cmd_imi,
    "lb size <1-400> ",
    "lb configure",
    "lb packet size configure:repeat 1-400,default is 0"
);

IMI_ALI (generic_show_func,
     show_spanning_tree_rpvst_interface_cmd_imi,
     "show spanning-tree rpvst+ interface IFNAME",
     "Show running system information",
     "spanning-tree Display spanning tree information",
     "Display RPVST information",
     "Interface information",
     "Interface name");

IMI_ALI (NULL,
     nsm_br_vlan_enable_disable_cmd_imi,
     "vlan ""<2-4094>"" bridge <1-32> state (enable|disable)",
     "Add, delete, or modify values associated with a single VLAN",
     "VLAN id",
     "Bridge group commands.",
     "Bridge group for bridging.",
     "Operational state of the VLAN",
     "Enable",
     "Disable");

IMI_ALI (NULL,
     nsm_if_shutdown_cmd_imi,
     "shutdown",
     "Shutdown the selected interface");

IMI_ALI (NULL,
     mstp_add_l2gp_cmd_imi,
     "switchport l2gp  psuedoRootId ROOTID:MAC",
     "Set the l2gp characteristics of the Layer2 interface",
     "l2gp",
     "puedoInfo priority/bridge mac address",
     "priority/mac-address 12345/0000.0000.0000");

IMI_ALI (NULL,
     no_bridge_max_hops_cmd_imi,
     "no bridge <1-32> max-hops",
     "Negate a command or set its defaults",
     "Bridge group commands",
     "Bridge Group name for bridging",
     "max-hops - set time to listen for root bridge to default");

IMI_ALI (generic_show_func,
     show_mirror_interface_cmd_imi,
     "show mirror interface IFNAME",
     "Show",
     "Port Mirroring",
     "Source Interface to use",
     "Source Interface to use");

IMI_ALI (generic_show_func,
     show_mls_qos_interface_cmd_imi,
     "show mls qos interface IFNAME",
     "Show running system information",
     "Multi-Layer Switch(L2/L3).",
     "Quality of Service.",
     "Select an interface",
     "Interface name");

IMI_ALI (generic_show_func,
     show_lsp_status_cmd_imi,
     "show lsp-status <1-2000>",
     "show command",
     "lsp egress tunnel status and ingress tunnel status",
     "lsp id");

IMI_ALI (NULL,
     nsm_bridge_clear_dynamic_fdb_bridge_cmd_imi,
     "clear mac address-table dynamic",
     "Clear the Forwarding database",
     "Clear layer 2 MAC entries",
     "Clear all Entries in the forwarding database",
     "Clear all dynamic entries");

IMI_ALI (generic_show_func,
     show_nsm_vlan_spanningtree_bridge_cmd_imi,
     "show vlan (all|static|dynamic|auto)",
     "Show running system information",
     "Display VLAN information",
     "All VLANs(static and dynamic)",
     "Static VLANs",
     "Dynamic VLANS",
     "Auto configured VLANS");

IMI_ALI (NULL,
     no_debug_mstp_timer_cmd_imi,
     "no debug mstp timer",
     "Negate a command or set its defaults",
     "Debugging functions (see also 'undebug')",
     "Multiple Spanning Tree Protocol (MSTP)",
     "MSTP Timers");

IMI_ALI (NULL,
     gmrp_debug_event_cmd_imi,
     "debug gmrp event",
     "debug - enable debug output",
     "gmrp - GMRP commands",
     "event - echo events to console");

IMI_ALI (NULL,
    config_lsp_head_entity_cmd_imi,
    "tun ID {primary|backup}",
   "configure lsp_head_entity :tun ID and primary or backup"
);

IMI_ALI (NULL,
     bridge_revision_cmd_imi,
     "bridge <1-32> revision REVISION_NUM",
     "Bridge group commands",
     "Bridge Group name for bridging",
     "Revision  Number for configuration information",
     "Number 0-255");

IMI_ALI (NULL,
    ptp_port_unbind_config_cmd_imi,
    "unbind",
    "unbind ethernet interface");

IMI_ALI (NULL,
    vpn_rmv_vport_mac_cmd_imi,
    "rmv-vport-mac (ac|pw) <1-2000> mac MAC",
    "rmv an mac from vport",
    "vport type, attachment circuit",
    "vport type, pseudo wire",
    "vport id value",
    "virtual mac address",
    "mac (hardware) address, in HHHH.HHHH.HHHH format");

IMI_ALI (NULL,
 lsp_car_cmd_imi,
    "car (disable|enable cir <0-1000000> cbs <0-4095> pir <0-1000000> pbs <0-4095>)",
    "config car paras",
    "disable car",
    "enable car",
 "committed information rate",
    "rate value, kbps",
    "committed brust size",
 "brust size value, bytes",
 "peak information rate",
 "rate value, kbps",
 "peak brust size",
 "brust size value, bytes");

IMI_ALI (NULL,
     no_ip_access_list_extended_host_host_cmd_imi,
     "no ip-access-list (<100-199>|<2000-2699>) (deny|permit) ip host A.B.C.D host A.B.C.D",
     "Negate a command or set its defaults",
     "QoS's IP access list",
     "IP extended access list", "IP extended access list (expanded range)", "Specify packets to reject", "Specify pactets to permit",
     "Any Internet Protocol",
     "A single source host",
     "Source address",
     "A single destination host",
     "Destination address");

IMI_ALI (NULL,
     no_wrr_queue_threshold_dscp_map_cmd_imi,
     "no wrr-queue dscp-map <1-2>",
     "Negate a command or set its defaults",
     "WRR queue",
     "DSCP map",
     "Threshold ID of the queue, range is 1 to 2");

IMI_ALI (NULL,
     nsm_vlan_switchport_trunk_pn_cn_delete_cmd_imi,
     "switchport (trunk|provider-network|customer-network) allowed "
     "vlan remove VLAN_ID",
     "Set the switching characteristics of the Layer2 interface",
     "Set the Layer2 interface as trunk",
     "Set the Layer2 interface as provider network",
     "Set the Layer2 interface as Customer Network",
     "Set the VLANs that will Xmit/Rx through the Layer2 interface",
     "VLAN that will be removed",
     "Remove a VLAN that Xmit/Tx through the Layer2 interface",
     "The List of the VLAN IDs that will be removed from the Layer2 interface");

IMI_ALI (NULL,
    lm_proactive_detect_cmd_imi,
    "lm proactive {enable |disable}",
    "lm proactive detect:enable or disable ",
    "lm proactive dection configure"
);

IMI_ALI (NULL,
    no_tun_id_cmd_imi,
    "no tun <1-2000>",
    "Negate a command or set its defaults",
    "delete a tunnel's configuration",
    "tunnel id, range 1-2000");

IMI_ALI (NULL,
     no_qos_cmap_match_traffic_type_cmd1_imi,
     "no match traffic-type (unknown-unicast | "
     "unknown-multicast | broadcast | multicast "
     "| unicast | management | arp | tcp-data | "
     "tcp-control | udp | non-tcp-udp | queue0 | "
     "queue1 | queue2 | queue3 | all) ",
     "Negate a command or set its defaults",
     "Define the match criteria",
     "Specify Traffic Type",
     "Specify unknown unicast frames",
     "Specify unknown multicast frames",
     "Specify broadcast frames",
     "Specify multicast frames",
     "Specify unicast frames",
     "Specify management frames",
     "Specify ARP frames",
     "Specify TCP data frames",
     "Specify TCP control frames",
     "Specify UDP frames",
     "Specify non TCP/UDP frames",
     "Specify queue0",
     "Specify queue1",
     "Specify queue2",
     "Specify queue3",
     "Specify all");

IMI_ALI (NULL,
    gloal_oam_1731_enable_cmd_imi,
    "oam {enable |disable}",
    "gloal oam enable or disable",
   "set oam enable or disable"
);

IMI_ALI (NULL,
     mls_qos_map_dscp_mutation_cmd_imi,
     "mls qos map dscp-mutation NAME (<0-63>|<0-63> <0-63>|<0-63> <0-63> <0-63>|<0-63> <0-63> <0-63> <0-63>|<0-63> <0-63> <0-63> <0-63> <0-63>|<0-63> <0-63> <0-63> <0-63> <0-63> <0-63>|<0-63> <0-63> <0-63> <0-63> <0-63> <0-63> <0-63>|<0-63> <0-63> <0-63> <0-63> <0-63> <0-63> <0-63> <0-63>) to <0-63>",
     "Multi-Layer Switch(L2/L3).",
     "Quality of Service.",
     "Specify maps",
     "Modify the DSCP mutation map",
     "Specify DSCP mutation name or ID",
     "Incoming DSCP value",
     "Incoming DSCP value",
     "Incoming DSCP value",
     "Incoming DSCP value",
     "Incoming DSCP value",
     "Incoming DSCP value",
     "Incoming DSCP value",
     "Incoming DSCP value",
     "Incoming DSCP value",
     "Incoming DSCP value",
     "Incoming DSCP value",
     "Incoming DSCP value",
     "Incoming DSCP value",
     "Incoming DSCP value",
     "Incoming DSCP value",
     "Incoming DSCP value",
     "Incoming DSCP value",
     "Incoming DSCP value",
     "Incoming DSCP value",
     "Incoming DSCP value",
     "Incoming DSCP value",
     "Incoming DSCP value",
     "Incoming DSCP value",
     "Incoming DSCP value",
     "Incoming DSCP value",
     "Incoming DSCP value",
     "Incoming DSCP value",
     "Incoming DSCP value",
     "Incoming DSCP value",
     "Incoming DSCP value",
     "Incoming DSCP value",
     "Incoming DSCP value",
     "Incoming DSCP value",
     "Incoming DSCP value",
     "Incoming DSCP value",
     "Incoming DSCP value",
     "To",
     "Outgoing DSCP value");

IMI_ALI (NULL,
     nsm_bridge_clear_dynamic_br_fdb_bridge_cmd_imi,
     "clear mac address-table dynamic bridge <1-32>",
     "Clear the Forwarding database",
     "Clear layer 2 MAC entries",
     "Clear all Entries in the forwarding database",
     "Clear all dynamic entries",
     "Bridge group commands.",
     "Bridge group for bridging.");

IMI_ALI (generic_show_func,
     show_gvrp_machine_br_cmd_imi,
     "show (gvrp|mvrp) machine bridge BRIDGE_NAME",
     "CLI_SHOW_STR",
     "Generic Attribute Registration Protocol",
     "MRP Vlan Registration Protocol",
     "Finite State Machine",
     "Bridge",
     "Bridge name");

IMI_ALI (NULL,
     no_spanning_instance_cmd_imi,
     "no instance <1-63>",
     "Negate a command or set its defaults",
     "Instance",
     "ID");

IMI_ALI (NULL,
     nsm_vlan_switchport_trunk_native_cmd_imi,
     "switchport trunk native vlan VLAN_ID",
     "Set the switching characteristics of the Layer2 interface",
     "Set the switching characteristics of the Layer2 interface to trunk",
     "Set the native VLAN for classifying untagged traffic through the Layer2 interface",
     "VLAN that will be added",
     "The native VLAN id");

IMI_ALI (NULL,
     mls_qos_egress_rate_shape_cmd_imi,
     "traffic-shape rate RATE (kbps|fps)",
     "Configure Traffic Shaping",
     "Configure the Traffic rate shape",
     "Traffic rate shape",
     "Kilo Bits Per Second",
     "Frames Per Second");

IMI_ALI (NULL,
    lt_detect_ttl_cmd_imi,
    "lt ttl  <1-255>",
    "lt configure",
    "lt ttl configure:ttl 1-255,default is 30. "

);

IMI_ALI (NULL,
     bridge_add_static_filter_cmd_imi,
     "bridge <1-32> address MAC discard IFNAME",
     "Bridge group commands.",
     "Bridge group for bridging.",
     "address",
     "mac address in HHHH.HHHH.HHHH format",
     "discard",
     "Interface name");

IMI_ALI (NULL,
     mstp_spanning_tree_portfast_cmd_imi,
     "spanning-tree (portfast | edgeport)",
     "spanning-tree",
     "portfast - enable fast transitions",
     "edgeport - enable it as edgeport");

IMI_ALI (NULL,
     no_debug_mstp_proto_detail_cmd_imi,
     "no debug mstp protocol detail",
     "Negate a command or set its defaults",
     "Debugging functions (see also 'undebug')",
     "Multiple Spanning Tree Protocol (MSTP)",
     "MSTP Protocol",
     "Detailed output");

IMI_ALI (NULL,
     mac_access_list_host_any_cmd_imi,
     "mac-access-list <2000-2699> (deny|permit) MAC MASK any <1-8>",
     "QOS's MAC access list", "extended mac ACL <2000-2699>",
     "Specify packets to reject", "Specify packets to permit",
     "Source host's MAC address in HHHH.HHHH.HHHH format",
     "Source wildcard in HHHH.HHHH.HHHH format",
     "Destination any",
     "Specify packet format, (1:Ethernet II/ 2:802.3/ 4:SNAP/ 8:LLC)");

IMI_ALI (NULL,
     mls_qos_strict_queue_all_cmd_imi,
     "mls qos strict queue (all|none)",
     "Multi-Layer Switch(L2/L3).",
     "Quality of Service.",
     "Configure the Queue as Strict Queue",
     "The Queue which is configured as Strict Queue",
     "Configure All Queues to be Strict",
     "Configure All Queues to be WRR");

IMI_ALI (NULL,
     qos_pmapc_policing_meter_cmd_imi,
     "policing meter <1-255>",
     "Specify a policer meter for the classified traffic",
     "Value is average traffic rate by burst rate",
     "Policing ratio");

IMI_ALI (NULL,
     nsm_vlan_no_name_cmd_imi,
     "vlan <2-4094>",
     "Add, delete, or modify values associated with a single VLAN",
     "VLAN id");

IMI_ALI (NULL,
     nsm_bridge_protocol_mstp_cmd_imi,
     "bridge <1-32> protocol mstp (ring|)",
     "Bridge group commands.",
     "Bridge group for bridging.",
     "Spanning-tree protocol",
     "IEEE 802.1s multiple spanning-tree protocol",
     "Enable Rapid Ring spanning-tree");

IMI_ALI (NULL,
     no_gvrp_debug_imish_cmd_imi,
     "no debug gvrp cli",
     "Negate a command or set its defaults",
     "debug - disable debug output",
     "gvrp - GVRP commands",
     "cli - do not echo commands to console");

IMI_ALI (NULL,
    vpn_config_cmd_imi,
    "config (vpws|vpls (enable-muticast|disable-muticast) (enable-mac-learn|disable-mac-learn)) ",
    "config vpn paras: type, muticast flag, mac learn flag",
    "vpn type",
    "vpn type",
    "muticast flag",
    "muticast flag",
    "mac learn flag",
    "mac learn flag");

IMI_ALI (NULL,
     no_spanning_port_vlan_priority_cmd_imi,
     "no spanning-tree vlan <2-4094> priority",
     "Negate a command or set its defaults",
     "Spanning Tree Commands",
     "Rapid Pervlan Spanning Tree vlan",
     "identifier",
     "Port priority for bridge");

IMI_ALI (generic_show_func,
     show_flowcontrol_interface_cmd_imi,
     "show flowcontrol interface IFNAME",
     "Show running system information",
     "IEEE 802.3x Flow Control",
     "Interface",
     "Interface to display");

IMI_ALI (NULL,
     mls_qos_default_cos_cmd_imi,
     "mls qos cos <0-7>",
     "Multi-Layer Switch(L2/L3).",
     "Quality of Service.",
     "Configure default CoS values",
     "CoS value");

IMI_ALI (NULL,
     qos_pmapc_set_algorithm_cmd_imi,
     "set algorithm (strict|drr|drr-strict)",
     "Set Qos Parameters",
     "Set the algorithm for egress scheduling",
     "Strict algorithm",
     "drr algorithm",
     "drr-strict algorithm");

IMI_ALI (NULL,
     mstp_spanning_tree_inst_restricted_tcn_cmd_imi,
     "spanning-tree instance <1-63> restricted-tcn",
     "spanning tree commands",
     "Set restrictions for the port of particular instance",
     "instance id",
     "restrict propagation of topology change notifications from port");

IMI_ALI (NULL,
     mls_qos_monitor_dscp_cmd_imi,
     "mls qos monitor dscp",
     "Multi-Layer Switch(L2/L3).",
     "Quality of Service.",
     "Monitor",
     "DSCP");

IMI_ALI (NULL,
     set_mvrp_point_to_point_cmd_imi,
     "set mvrp pointtopoint (enable|disable) interface IF_NAME",
     "set MVRP globally for the bridge",
     "MRP Vlan Registration Protocol",
     "point to point mode of interface",
     "enable",
     "disale",
     "Identify the name of the bridge to use",
     "The text string to use for the name of the bridge");

IMI_ALI (NULL,
     no_gmrp_debug_timer_cmd_imi,
     "no debug gmrp timer",
     "Negate a command or set its defaults",
     "gmrp - GMRP commands",
     "debug - disable debug output",
     "timer - do not echo timer start to console");

IMI_ALI (NULL,
     nsm_vlan_switchport_mode_cmd_imi,
     "switchport mode (access|hybrid|trunk|provider-network|customer-network)",
     "Set the switching characteristics of the Layer2 interface",
     "Set the mode of the Layer2 interface",
     "Set the Layer2 interface as Access",
     "Set the Layer2 interface as hybrid",
     "Set the Layer2 interface as trunk",
     "Set the Layer2 interface as provider network",
     "Set the Layer2 interface as Customer Network"
     );

IMI_ALI (NULL,
     mstp_clear_spanning_tree_detected_protocols_interface_cmd_imi,
     "clear spanning-tree detected protocols interface INTERFACE",
     "clear",
     "spanning-tree",
     "detected",
     "protocols",
     "interface",
     "INTERFACE - interface name");

IMI_ALI (NULL,
    dm_way_select_period_cmd_imi,
    "dm period  {1s|10s}",
    "dm  configure ",
    "lt period configure:1s or 10s,default is 1s  . "
);

IMI_ALI (NULL,
    ac_car_cmd_imi,
    "car (disable|enable cir <0-1000000> cbs <0-4095> pir <0-1000000> pbs <0-4095>)",
    "config car paras",
    "disable car",
    "enable car",
    "committed information rate",
    "rate value, kbps",
    "committed brust size",
    "brust size value, bytes",
    "peak information rate",
    "rate value, kbps",
    "peak brust size",
    "brust size value, bytes");

IMI_ALI (NULL,
     no_mstp_bridge_forward_time_cmd_imi,
     "no bridge <1-32> forward-time",
     "Negate a command or set its defaults",
     "Bridge group commands",
     "Bridge Group name for bridging",
     "forward-time - set forward delay time to default value");

IMI_ALI (NULL,
     bandwidth_if_cmd_imi,
     "bandwidth BANDWIDTH",
     "Set maximum bandwidth parameter",
     "<1-999> k|m for 1 to 999 kilo bits or mega bits                 <1-10> g for 1 to 10 giga bits");

IMI_ALI (NULL,
    lm_on_demand_detect_repeat_cmd_imi,
    "lm-on-demand repeat <1-200>",
    "lm on-demand configure ",
    "lm on-demand repeat configure:repeat 1-200,default is 3 ."
);

IMI_ALI (generic_show_func,
     show_lspgrp_status_cmd_imi,
     "show lsp-grp-status <1-2000>",
     "show command",
     "lsp protect group status",
     "lsp protect group id");

IMI_ALI (generic_show_func,
     show_gmrp_br_statistics_cmd_imi,
     "show (gmrp|mmrp) statistics vlanid <1-4094> (bridge <1-32>|)",
     "CLI_SHOW_STR",
     "gmrp",
     "MRP Multicast Registration Protocol",
     "GMRP Statistics for the given vlanid",
     "Vlanid",
     "Vlanid value <1-4094>",
     "Bridge instance",
     "Bridge instance name");

IMI_ALI (NULL,
     no_mls_qos_map_cos_dscp_cmd_imi,
     "no mls qos map cos-dscp",
     "Negate a command or set its defaults",
     "Multi-Layer Switch(L2/L3).",
     "Quality of Service.",
     "Specify maps",
     "Modify the CoS-to_DSCP map");

IMI_ALI (NULL,
    lb_detect_enable_cmd_imi,
    "lb {enable|disable} ",
    "lb configure",
    "lb enable configure:enable or disable"
);

IMI_ALI (NULL,
     mstp_bridge_ageing_time_cmd_imi,
     "bridge <1-32> ageing-time <10-1000000>",
     "Bridge group commands",
     "Bridge Group name for bridging",
     "time a learned mac address will persist after last update",
     "ageing time in seconds");

IMI_ALI (NULL,
    sync_interval_config_cmd_imi,
    "set-sync-interval INTERVAL",
    "set sync interval",
    "sync interval value, range -128 - 127");

IMI_ALI (NULL,
     qos_pmapc_set_vlanpriority_cmd_imi,
     "set vlan-priority <0-7> <0-7> <0-7> <0-7> <0-7> <0-7> <0-7> <0-7>",
     "Set Qos Parameters",
     "Set the priority for the queues",
     "Specify priority for queue 0",
     "Specify priority for queue 1",
     "Specify priority for queue 2",
     "Specify priority for queue 3",
     "Specify priority for queue 4",
     "Specify priority for queue 5",
     "Specify priority for queue 6",
     "Specify priority for queue 7");

IMI_ALI (NULL,
    config_pw_entity_cmd_imi,
    "vcid ID peer-ip IPADDR pwtype {vpls|vpws}",
   "configure vcid",
   "configure peer-ip",
    "configure pwtype:vpls or vpws"
);

IMI_ALI (NULL,
     nsm_cus_vlan_enable_disable_cmd_imi,
     "vlan ""<2-4094>"" type customer state (enable|disable)",
     "Add, delete, or modify values associated with a single VLAN",
     "VLAN id",
     "VLAN Type",
     "Customer VLAN",
     "Operational state of the VLAN",
     "Enable",
     "Disable");

IMI_ALI (NULL,
     nsm_mac_access_group_cmd_imi,
     "mac access-group WORD in",
     "Configure MAC parameters",
     "Configure MAC Access group",
     "Name for the MAC Access List",
     "ingress direction");

IMI_ALI (NULL,
     spanning_vlan_add_static_cmd_imi,
     "mac-address-table static MAC (forward|discard) IFNAME vlan ""<2-4094>",
     "Spanning Tree group commands.",
     "Static address",
     "mac address in HHHH.HHHH.HHHH format",
     "forward",
     "discard",
     "Interface name",
     "VLAN",
     "vlan id");

IMI_ALI (NULL,
      mstp_spanning_tree_errdisable_timeout_interval_cmd1_imi,
      "spanning-tree errdisable-timeout interval <10-1000000>",
      "spanning-tree",
      "errdisable-timeout",
      "interval after which port shall be enabled",
      "errdisable-timeout interval in seconds");

IMI_ALI (NULL,
    lm_on_demand_detect_period_cmd_imi,
    "lm-on-demand period {1s|10s}",
    "lm on-demand configure ",
    "lm on-demand period configure:period  1s or 10s ,default is 1s  ."
);

IMI_ALI (NULL,
     no_spanning_static_cmd_imi,
     "no mac-address-table static MAC (forward|discard) IFNAME",
     "Negate a command or set its defaults",
     "Spanning Tree group commands.",
     "Static address",
     "mac address in HHHH.HHHH.HHHH format",
     "forward",
     "discard",
     "Interface name");

IMI_ALI (generic_show_func,
     show_mirror_cmd_imi,
     "show mirror",
     "Show",
     "Port Mirroring");

IMI_ALI (NULL,
    mac_in_ppp_section_cmd_imi,
    "section dmac <XXXX.XXXX.XXXX> ",
    "configure section dmac:XXXX.XXXX.XXXX"

);

IMI_ALI (NULL,
     set_port_gmrp2_cmd_imi,
     "set port (gmrp|mmrp) disable (IF_NAME|all)",
     "Disable GMRP on interface",
     "Layer2 Interface",
     "GARP Multicast Registration Protocol",
     "MRP Multicast Registration Protocol",
     "Disable",
     "Ifname",
     "All the ports");

IMI_ALI (NULL,
    vpn_add_vport_mac_cmd_imi,
    "add-vport-mac (ac|pw) <1-2000> mac MAC (enable-src-drop|disable-src-drop) (enable-dst-drop|disable-dst-drop)",
    "add an mac to vport",
    "vport type, attachment circuit",
    "vport type, pseudo wire",
    "vport id value",
    "virtual mac address",
    "mac (hardware) address, in HHHH.HHHH.HHHH format",
    "drop when source mac address matched",
    "retain when source mac address matched",
    "drop when destination mac address matched",
    "retain when destination mac address matched");

IMI_ALI (NULL,
    domain_mport_add_cmd_imi,
    "domain-add <0-255> mport <0-1>",
    "add vport to domain",
    "domain number, range 0 - 255",
    "add port to be connected with master clock",
    "virtual port, range 0-1" );

IMI_ALI (NULL,
     mstp_bridge_hello_time_cmd_imi,
     "bridge <1-32> hello-time <1-10>",
     "Bridge group commands",
     "Bridge Group name for bridging",
     "hello-time - hello BDPU interval",
     "seconds <1-10> - Hello BPDU interval");

IMI_ALI (NULL,
     no_qos_cmap_match_access_group_cmd_imi,
     "no match access-group NAME",
     "Negate a command or set its defaults",
     "Define the match creteria",
     "List IP or MAC access contol lists",
     "Specify ACL list name");

IMI_ALI (NULL,
     mstp_spanning_transmit_hold_count_cmd_imi,
     "spanning-tree transmit-holdcount <1-10>",
     "Spanning Tree Commands",
     "Transmit hold count of the bridge",
     "range of the transmitholdcount");

IMI_ALI (NULL,
     nsm_bridge_clear_br_fdb_vlan_port_cmd_imi,
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
     "Bridge group commands.",
     "Bridge group for bridging.");

IMI_ALI (NULL,
    pw_config_cmd_imi,
    "config ingress lable <0-1048575> egress lable <0-1048575> exp <0-7> lsp-id <1-2000>",
    "config pw paras: direction, lable, exp, lsp",
    "pw direction",
    "ingress-pw lable",
    "lable value",
    "pw direction",
    "egress-pw lable",
    "lable value",
    "egress-pw exp",
    "exp value",
    "bind lsp to tunnel",
    "lsp id");

IMI_ALI (generic_show_func,
     show_nsm_vlan_static_ports_cmd_imi,
     "show vlan (<2-4094> static-ports | static-ports)",
     "Show running system information",
     "Display VLAN information",
     "VLAN id",
     "Display static egress/forbidden ports",
     "Display static egress/forbidden ports");

IMI_ALI (generic_show_func,
     show_debugging_gvrp_cmd_imi,
     "show debugging gvrp",
     "Show running system information",
     "Debugging functions (see also 'undebug')",
     "GARP VLAN Registration Protocol (GVRP)");

IMI_ALI (NULL,
     flowcontrol_receive_off_cmd_imi,
     "flowcontrol receive off",
     "IEEE 802.3x Flow Control",
     "Flow control on receive",
     "Turn off flow control");

IMI_ALI (NULL,
     no_mac_acl_any_host_cmd_imi,
     "no mac acl <2000-2699> (deny|permit) any MAC MASK <1-8>",
     "Negate a command or set its defaults",
     "MAC access list", "MAC access list", "extended mac ACL <2000-2699>",
     "Specify packets to reject", "Specify packets to permit",
     "Source any",
     "Destination host's MAC address in HHHH.HHHH.HHHH format",
     "Destination wildcard in HHHH.HHHH.HHHH format",
     "Specify packet format, (1:Ethernet II/ 2:802.3/ 4:SNMP/ 8:LLC)");

IMI_ALI (NULL,
     no_nsm_if_duplex_cmd_imi,
     "no duplex",
     "Negate a command or set its defaults",
     "Set default duplex(auto-negotiate) to interface");

IMI_ALI (NULL,
     no_mls_qos_egress_rate_shape_cmd_imi,
     "no traffic-shape",
     "Negate a command or set its defaults",
     "Configure Traffic Shaping");

IMI_ALI (NULL,
      no_mstp_span_portfast_bpdufilter_cmd_imi,
      "no spanning-tree portfast bpdu-filter",
      "Negate a command or set its defaults",
      "spanning-tree",
      "portfast",
      "Filter the BPDUs on portfast enabled ports");

IMI_ALI (NULL,
     no_nsm_vlan_classifier_group_cmd_imi,
     "no vlan classifier group <1-16>",
     "Negate a command or set its defaults",
     "Vlan commands",
     "Vlan classification commands",
     "Vlan classification groups commands",
     "Vlan classifier group id");

IMI_ALI (NULL,
     no_vlan_static_cmd_imi,
     "no bridge <1-32> address MAC forward IFNAME vlan ""<2-4094>",
     "Negate a command or set its defaults",
     "Bridge group commands.",
     "Bridge group for bridging.",
     "address",
     "mac address in HHHH.HHHH.HHHH format",
     "forward",
     "Interface name",
     "VLAN",
     "vlan id");

IMI_ALI (NULL,
    vpn_rmv_vport_cmd_imi,
    "rmv-vport (ac|pw) <1-2000>",
    "remove an attachment circuit or pseudo wire from vpn",
    "attachment circuit",
    "pseudo wire",
    "vport id value, range 1-2000");

IMI_ALI (generic_show_func,
     show_l2_ratelimit_cmd_imi,
     "show storm-control (IFNAME|)",
     "Show running system information",
     "The layer2 interface",
     "Display Rate Limit",
     "Interface name");

IMI_ALI (NULL,
     mstp_spanning_priority_cmd_imi,
     "spanning-tree priority <0-61440>",
     "Spanning Tree Commands",
     "priority - bridge priority for the common instance",
     "bridge priority in increments of 4096 (Lower priority indicates greater likelihood of becoming root)");

IMI_ALI (NULL,
     undebug_nsm_all_cmd_imi,
     "undebug all nsm",
     "Disable debugging functions (see also 'debug')",
     "Turn off all Debugging",
     "Network Service Module (NSM)");

IMI_ALI (NULL,
     no_qos_pmapc_trust_cmd_imi,
     "no trust",
     "Negate a command or set its defaults",
     "Specify trust mode for policy-map");

IMI_ALI (NULL,
    pdc_alarm_threshold_cmd_imi,
    "packet-delay-change alarm-threshold <1-100000>",
    "packet-delay-change alarm-threshold:1-100000 ",
    "packet-delay-change alarm-threshold configure"
);

IMI_ALI (NULL,
      no_mstp_spanning_tree_pathcost_method_cmd_imi,
      "no bridge <1-32> spanning-tree pathcost method",
      "Bridge group commands",
      "Bridge Group name for bridging",
      "Spanning Tree Subsystem",
      "Spanning tree pathcost options",
      "Method to calculate default port path cost");

IMI_ALI (NULL,
     set_port_gmrp_disable_vlan_cmd_imi,
     "set port (gmrp|mmrp) disable IF_NAME vlan VLANID",
     "Enable GMRP on a port",
     "Layer2 Interface",
     "GARP Multicast Registration Protocol",
     "MRP Multicast Registration Protocol",
     "Disable",
     "Ifname",
     "Identify the VLAN to use",
     "The VLAN ID to use");

IMI_ALI (NULL,
     mls_qos_trust_cos_path_through_dscp_cmd_imi,
     "mls cos trust cos path-through dscp",
     "Multi-Layer Switch(L2/L3).",
     "Quality of Service.",
     "Enable pass-through mode, not modify DSCP value",
     "CoS",
     "Path-through DSCP",
     "DSCP");

IMI_ALI (NULL,
      no_mstp_spanning_tree_errdisable_timeout_enable_cmd_imi,
      "no bridge <1-32> spanning-tree errdisable-timeout enable",
      "Negate a command or set its defaults",
      "Bridge group commands",
      "Bridge Group name for bridging",
      "spanning-tree",
      "errdisable-timeout",
      "disable the timeout mechanism for the port to be enbaled back");

IMI_ALI (NULL,
     no_debug_nsm_cmd_imi,
     "no debug nsm (all|)",
     "Negate a command or set its defaults",
     "Debugging functions (see also 'undebug')",
     "Network Service Module (NSM)",
     "Turn off all Debugging");

IMI_ALI (NULL,
     gvrp_debug_all_cmd_imi,
     "debug gvrp all",
     "debug - enable debug output",
     "gvrp - GVRP commands",
     "all - turn on all debugging");

IMI_ALI (NULL,
     no_debug_mstp_timer_detail_cmd_imi,
     "no debug mstp timer detail",
     "Negate a command or set its defaults",
     "Debugging functions (see also 'undebug')",
     "Multiple Spanning Tree Protocol (MSTP)",
     "MSTP Timers",
     "Detailed output");

IMI_ALI (NULL,
     no_mls_qos_trust_cmd_imi,
     "no mls qos trust dscp)",
     "Negate a command or set its defaults",
     "Multi-Layer Switch(L2/L3).",
     "Quality of Service.",
     "Confiure port trust state",
     "Classifies ingress packets with the packet DSCP values");

IMI_ALI (NULL,
     no_qos_cmap_match_ip_precedence_cmd_imi,
     "no match ip-precedence <0-7>",
     "Negate a command or set its defaults",
     "Define the match creteria",
     "List IP-Precedence list",
     "Specify IP precedence value");

IMI_ALI (NULL,
     no_qos_pmapc_set_quantumpriority_cmd_imi,
     "no set drr-priority",
     "Negate a command or set its defaults",
     "set Qos parameters",
     "Setting the Priority and Quantum for DRR");

IMI_ALI (NULL,
     set_gmrp2_cmd_imi,
     "set (gmrp|mmrp) disable",
     "Disable GMRP globally for the bridge instance",
     "GARP Multicast Registration Protocol",
     "MRP Multicast Registration Protocol",
     "Disable");

IMI_ALI (NULL,
    dm_way_select_repeat_cmd_imi,
    "dm repeat  <1-200>",
    "dm  configure ",
    "lt period configure:repeat 1-200,default is 3 . "
);

IMI_ALI (NULL,
     spanning_inst_priority_cmd_imi,
     "spanning-tree instance <1-63> priority <0-61440>",
     "Spanning Tree Commands",
     "Change priority for a particular instance",
     "instance id",
     "priority - bridge priority for the common instance",
     "bridge priority in increments of 4096 (Lower priority indicates greater likelihood of becoming root)");

IMI_ALI (NULL,
     mls_qos_map_cos_queue_cmd_imi,
     "mls qos cos-queue <0-7> <0-3>",
     "Multi-Layer Switch(L2/L3).",
     "Quality of Service.",
     "Specify cos-queue",
     "Specify Cos value with priority <0-7>",
     "Specify queue id <0-3>");

IMI_ALI (NULL,
     nsm_interface_alias_cmd_imi,
     "alias WORD ",
     "Alias name for the interface",
     "Name ");

IMI_ALI (NULL,
     mac_access_list_address_cmd_imi,
     "mac-access-list <2000-2699> (source|destination) MAC priority <0-7>",
     "QOS's MAC access list", "extended mac ACL <2000-2699>",
     "Specify packets with source MAC address",
     "Specify packets with destination MAC address",
     "MAC address in HHHH.HHHH.HHHH format",
     "Priority Class",
     "Priority Value");

IMI_ALI (NULL,
     ip_access_list_extended_cmd_imi,
     "ip-access-list (<100-199>|<2000-2699>) (deny|permit) ip A.B.C.D A.B.C.D A.B.C.D A.B.C.D",
     "QoS's IP access list",
     "IP extended access list", "IP extended access list (expanded range)", "Specify packets to reject", "Specify pactets to permit",
     "Any Internet Protocol",
     "Source address",
     "Source wildcard bits",
     "Destination address",
     "Destination Wildcard bits");

IMI_ALI (NULL,
     set_gvrp_br_dyn_vlan_enable_cmd_imi,
     "set (gvrp|mvrp) dynamic-vlan-creation enable bridge BRIDGE_NAME",
     "Set values in destination routing protocol",
     "GARP Vlan Registration Protocol",
     "MRP Vlan Registration Protocol",
     "Dynamic Vlan creation",
     "Enable",
     "Bridge instance ",
     "Bridge instance name");

IMI_ALI (NULL,
     switchport_ratelimit_cmd_imi,
     "storm-control (broadcast|multicast|dlf) level LEVEL",
     "Set the switching characteristics of Layer2 interface",
     "Set Broadcast Rate Limiting",
     "Set Multicast Rate Limiting",
     "Set DLF Broadcast Rate Limiting",
     "Threshold Level",
     "Threshold Percentage (0.0-100.0)");

IMI_ALI (NULL,
     spanning_inst_path_cost_cmd_imi,
     "spanning-tree instance <1-63> path-cost <1-200000000>",
     "Spanning Tree Commands",
     "Multiple Spanning Tree Instance",
     "identifier",
     "path cost for a port",
     "path cost in range <1-200000000>"
     " (lower path cost indicates greater likelihood of becoming root)");

IMI_ALI (NULL,
     no_spanning_inst_path_cost_cmd_imi,
     "no spanning-tree instance <1-63> path-cost",
     "Negate a command or set its defaults",
     "Spanning Tree Commands",
     "Multiple Spanning Tree Instance",
     "identifier",
     "path-cost - path cost for a port");

IMI_ALI (NULL,
     mls_qos_vlan_priority_cos_queue_cmd_imi,
     "mls qos vlan-priority-override (cos | queue | both)",
     "Multi-Layer Switch(L2/L3).",
     "Quality of Service.",
     "Vlan Priority Override",
     "Select COS",
     "Select QUEUE",
     "Select both");

IMI_ALI (NULL,
     mls_qos_force_trust_cos_cmd_imi,
     "mls qos force-trust cos",
     "Multi-Layer Switch(L2/L3).",
     "Quality of Service.",
     "Confiure port trust state",
     "Classifies ingress packets with the packet cos values");

IMI_ALI (NULL,
     priority_queue_out_cmd_imi,
     "priority-queue out",
     "Enable the egress expedite queue",
     "Enable the egress expedite queue");

IMI_ALI (NULL,
     no_mls_qos_strict_queue_cmd_imi,
     "no mls qos strict queue <0-3>",
     "Negate a command or set its defaults",
     "Multi-Layer Switch(L2/L3).",
     "Quality of Service.",
     "Configure the Queue as WRR",
     "The Queue which is configured as WRR Queue",
     "The Queue ID of WRR Queue");

IMI_ALI (NULL,
     spanning_inst_cmd_imi,
     "spanning-tree instance <1-63>",
     "Spanning Tree Commands",
     "Multiple Spanning Tree Instance",
     "identifier");

IMI_ALI (NULL,
     wrr_queue_egress_car_cmd_imi,
     "egress-car (disable qid <0-7>|enable qid <0-7> cir <0-1000000> pir <0-1000000>)",
     "WRR queue car",
     "disable to restore default egress car configuration",
     "Specify queue ID",
     "Queue ID 0-7",
     "enable egress car ",
     "Specify queue ID",
     "Queue ID 0-7",
     "Set cir paras",
     "Specify the cir in term of kbps",
     "Set pir paras",
     "Specify the pir in term of kbps");

IMI_ALI (NULL,
     mls_qos_map_dscp_queue_cmd_imi,
     "mls qos dscp-queue <0-64> <0-3>",
     "Multi-Layer Switch(L2/L3).",
     "Quality of Service.",
     "DSCP-to-QUEUE",
     "Select dscp_value <0-63>",
     "Select queue id");

IMI_ALI (NULL,
     spanning_instance_vlan_cmd_imi,
     "instance <1-63> vlan VLANID",
     "Instance",
     "ID",
     "vlan",
     "vlan id to be associated to instance");

IMI_ALI (NULL,
     set_gvrp_br_dyn_vlan_disable_cmd_imi,
     "set (gvrp|mvrp) dynamic-vlan-creation disable bridge BRIDGE_NAME",
     "Set values in destination routing protocol",
     "GARP Vlan Registration Protocol",
     "MRP Vlan Registration Protocol",
     "Dynamic Vlan creation",
     "Disable",
     "Bridge instance ",
     "Bridge instance name");

IMI_ALI (NULL,
     interface_arbiter_cmd_imi,
     "if-arbiter (interval <1-65535>|)",
     "Start arbiter to check interface information periodically",
     "Poll interval",
     "Seconds");

IMI_ALI (NULL,
    lp_alarm_threshold_cmd_imi,
    "loss-packet alarm-threshold <1-100000>",
    "loss-packet alarm-threshold:1-100000 ",
    "loss-packet alarm-threshold configure"
);

IMI_ALI (NULL,
    lsp_cfg_up_cmd_imi,
    "config egress (fe <1-24>|ge <1-4>) label <0-1048575> (vlan <0-4095>|) exp <0-8> peer-mac MAC",
    "lsp config",
    "egress tunnel",
    "egress-tun interface type",
    "fe interface id",
    "egress-tun interface type",
    "ge interface id",
    "egress-tun lable",
    "lable value",
    "egress-tun vlan",
    "vlan id, 0 means vlan diable",
    "egress-tun exp",
    "exp value,<0-7> outgoing lsp label exp uses the number specified; <8> outgoing lsp label exp uses internal priority mapping",
    "egress-tun peer mac address",
    "mac (hardware) address, in HHHH.HHHH.HHHH format");

IMI_ALI (NULL,
     debug_nsm_ha_all_cmd_imi,
     "debug nsm ha all",
     "Debugging functions (see also 'undebug')",
     "Network Service Module (NSM)",
     "NSM High Availability",
     "All NSM High Availability events");

IMI_ALI (NULL,
     no_mstp_spanning_tree_vlan_restricted_role_cmd_imi,
     "no spanning-tree vlan <2-4094> restricted-role",
     "Negate a command or set its defaults",
     "spanning tree commands",
     "remove restrictions for the port of particular vlan",
     "vlan id",
     "remove restriction on the role of the port");

IMI_ALI (NULL,
     no_spanning_vlanif_cmd_imi,
     "no spanning-tree vlan <2-4094>",
     "Negate a command or set its defaults",
     "Spanning Tree Commands",
     "Rapid Pervlan Spanning Tree Instance",
     "Vlan identifier");

IMI_ALI (NULL,
     mstp_spanning_tree_linktype_auto_cmd_imi,
     "spanning-tree link-type auto",
     "spanning tree commands",
     "link-type - point-to-point shared or auto",
     "auto - will be set to either p2p or shared based on duplex state");

IMI_ALI (NULL,
     no_spanning_shutdown_multiple_spanning_tree_cmd_imi,
     "no spanning-tree shutdown",
     "Negate a command or set its defaults",
     "Spanning Tree Commands",
     "reset");

IMI_ALI (NULL,
     bridge_shutdown_multiple_spanning_tree_for_cmd_imi,
     "bridge shutdown <1-32> bridge-forward",
     "Bridge group commands",
     "reset",
     "Bridge Group name for bridging",
     "put all ports of the bridge into forwarding state");

IMI_ALI (NULL,
     no_mstp_spanning_tree_autoedge_cmd_imi,
     "no spanning-tree autoedge",
     "Negate a command or set its defaults",
     "spanning-tree",
     "autoedge - enable automatic edge detection");

IMI_ALI (NULL,
     megshow_information_cmd_imi,
     "megshow INDEX",
     "Show meg id",
     "current meg information" );

IMI_ALI (NULL,
     switchport_ratelimit_cmd1_imi,
     "storm-control  ({broadcast|multicast}|)",
     "Set the switching characteristics of Layer2 interface",
     "Set Broadcast Rate Limiting",
     "Set Multicast Rate Limiting");

IMI_ALI (generic_show_func,
     show_mls_qos_aggregator_policer_cmd_imi,
     "show mls qos aggregator-policer NAME",
     "Show running system information",
     "Multi-Layer Switch(L2/L3).",
     "Quality of Service.",
     "Aggregator policer entry",
     "Aggregator policer name");

IMI_ALI (NULL,
     no_mls_qos_trust_dscp_cos_cmd_imi,
     "no mls qos trust (dscp | cos | both)",
     "Negate a command or set its defaults",
     "Multi-Layer Switch(L2/L3).",
     "Quality of Service.",
     "Confiure port trust state",
     "Classifies ingress packets with the packet DSCP values",
     "Classifies ingress packets with the packet COS values",
     "Classifies ingress packets with the packet BOTH values");

IMI_ALI (NULL,
     set_gmrp1_br_cmd_imi,
     "set (gmrp|mmrp) enable bridge BRIDGE_NAME",
     "Enable GMRP globally for the bridge instance",
     "GARP Multicast Registration Protocol",
     "MRP Multicast Registration Protocol",
     "Enable",
     "Bridge instance ",
     "Bridge instance name");

IMI_ALI (generic_show_func,
     show_gmrp_configuration_br_cmd_imi,
     "show (gmrp|mmrp) configuration bridge BRIDGE_NAME",
     "CLI_SHOW_STR",
     "GARP Multicast Registration Protocol",
     "MRP Multicast Registration Protocol",
     "Display GMRP configuration",
     "Bridge instance ",
     "Bridge instance name");

IMI_ALI (NULL,
     nsm_vlan_mtu_cmd_imi,
     "vlan ""<2-4094>"" mtu MTU_VAL",
     "Add, delete, or modify values associated with a single VLAN",
     "VLAN id",
     "MTU Value",
     "The Maximum Transmission Unit value for the vlan");

IMI_ALI (generic_show_func,
     show_spanning_tree_rpvst_cmd_imi,
     "show spanning-tree rpvst+",
     "Show running system information",
     "spanning-tree Display spanning tree information",
     "Display RPVST+ information" );

IMI_ALI (NULL,
     mstp_spanning_tree_portfast_bpdufilter_cmd_imi,
     "spanning-tree portfast bpdu-filter (enable|disable|default)",
     "spanning-tree",
     "portfast",
     "set the portfast bpdu-filter for the port",
     "enable",
     "disable",
     "default");

IMI_ALI (generic_show_func,
     show_spanning_tree_stats_interface_bridge_cmd_imi,
     "show spanning-tree statistics interface IFNAME (instance <1-63>| vlan <2-4094>) bridge <1-32>",
     "Show running system information",
     "Display Spanning-tree Information",
     "Statistics of the BPDUs",
     "Interface",
     "Interface name",
     "Display Instance Information",
     "Instance ID",
     "Vlan",
     "VLAN ID Associated with the Instance",
     "Bridge",
     "Bridge ID");

IMI_ALI (NULL,
     spanning_instance_cmd_imi,
     "instance <1-63>",
     "Instance",
     "ID");

IMI_ALI (NULL,
     mls_qos_enable_global_cmd_imi,
     "mls qos <0-32> <0-7> <0-32> <0-7> <0-32> <0-7> <0-32> <0-7> <0-32> <0-7> <0-32> <0-7> <0-32> <0-7> <0-32> <0-7>",
     "Multi-Layer Switch(L2/L3).",
     "Quality of Service.",
     "Specify weight for queue 0, 0 means Strict Priority",
     "Specify priority for queue 0",
     "Specify weight for queue 1, 0 means Strict Priority",
     "Specify priority for queue 1",
     "Specify weight for queue 2, 0 means Strict Priority",
     "Specify priority for queue 2",
     "Specify weight for queue 3, 0 means Strict Priority",
     "Specify priority for queue 3",
     "Specify weight for queue 4, 0 means Strict Priority",
     "Specify priority for queue 4",
     "Specify weight for queue 5, 0 means Strict Priority",
     "Specify priority for queue 5",
     "Specify weight for queue 6, 0 means Strict Priority",
     "Specify priority for queue 6",
     "Specify weight for queue 7, 0 means Strict Priority",
     "Specify priority for queue 7");

IMI_ALI (generic_show_func,
     show_spanning_tree_mst_interface_cmd_imi,
     "show spanning-tree mst interface IFNAME",
     "Show running system information",
     "spanning-tree Display spanning tree information",
     "Display MST information",
     "Interface information",
     "Interface name");

IMI_ALI (NULL,
     mirror_interface_receive_cmd_imi,
     "mirror interface IFNAME direction receive",
     "Port mirroring command",
     "Interface to use",
     "Source Interface to use",
     "Mirroring direction",
     "Mirror received traffic");

IMI_ALI (NULL,
     bridge_group_vlan_path_cost_cmd_imi,
     "bridge-group <1-32> vlan <2-4094> path-cost <1-200000000>",
     "Bridge group commands",
     "Bridge Group name for bridging",
     "rapid Pervlan Spanning Tree Instance",
     "vlan identifier",
     "path cost for a port",
     "path cost in range <1-200000000> (lower path cost indicates greater likelihood of becoming root)");

IMI_ALI (NULL,
     no_mirror_interface_receive_cmd_imi,
     "no mirror interface IFNAME direction receive",
     "Negate a command or set its defaults",
     "Port mirroring command",
     "Interface to use",
     "Source Interface to use",
     "Mirroring direction",
     "Mirror received traffic");

IMI_ALI (NULL,
     no_nsm_br_vlan_no_name_cmd_imi,
     "no vlan ""<2-4094>"" bridge <1-32>",
     "Negate a command or set its defaults",
     "Add, delete, or modify values associated with a single VLAN",
     "VLAN id",
     "Bridge group commands.",
     "Bridge group for bridging.");

IMI_ALI (generic_show_func,
     show_class_map_name_cmd_imi,
     "show class-map NAME",
     "Show running system information",
     "Class map entry",
     "Specify class map name");

IMI_ALI (NULL,
     gmrp_debug_timer_cmd_imi,
     "debug gmrp timer",
     "gmrp - GMRP commands",
     "debug - enable debug output",
     "timer - echo timer start to console");

IMI_ALI (NULL,
    creat_local_mep_cmd_imi,
    "local-mep ID type {source | destinnation |bidirectional }",
    "local-mep id,type,source , destinnation or bidirectional ",
    "local mep id,and type:source , destinnation or bidirectional"
);

IMI_ALI (NULL,
     qos_cmap_match_vlan_cmd_imi,
     "match vlan <1-4094>",
     "Define the match creteria",
     "List VLAN ID",
     "Specify VLAN ID");

IMI_ALI (NULL,
    no_meg_index_cmd_imi,
    "no meg INDEX",
    "MEG mode configure",
   "Indentifying MEG index"
   );

IMI_ALI (NULL,
     nsm_bridge_protocol_ieee_vlan_cmd_imi,
     "bridge <1-32> protocol ieee (vlan-bridge|)",
     "Bridge group commands.",
     "Bridge group for bridging.",
     "Spanning-tree protocol",
     "IEEE 801.1Q spanning-tree protocol",
     "VLAN aware bridge");

IMI_ALI (NULL,
     default_user_priority_cmd_imi,
     "user-priority <0-7>",
     "Set the default user priority associated with the layer2 interface",
     "User priority value");

IMI_ALI (NULL,
     spanning_add_static_cmd_imi,
     "mac-address-table static MAC (forward|discard) IFNAME",
     "Spanning Tree group commands.",
     "Static address",
     "mac address in HHHH.HHHH.HHHH format",
     "forward",
     "discard",
     "Interface name");

IMI_ALI (generic_show_func,
     show_flowcontrol_cmd_imi,
     "show flowcontrol",
     "Show running system information",
     "IEEE 802.3x Flow Control");

IMI_ALI (NULL,
      mstp_spanning_hello_time_cmd_imi,
      "spanning-tree hello-time <1-10>",
      "Spanning Tree Commands",
      "hello-time - hello BDPU interval",
      "seconds <1-10> - Hello BPDU interval");

IMI_ALI (NULL,
     gmrp_debug_packet_cmd_imi,
     "debug gmrp packet",
     "debug - enable debug output",
     "gmrp - GMRP commands",
     "packet - echo packet contents to console");

IMI_ALI (NULL,
     gvrp_debug_imish_cmd_imi,
     "debug gvrp cli",
     "debug - enable debug output",
     "gvrp - GVRP commands",
     "cli - echo commands to console");

IMI_ALI (NULL,
     no_qos_pmapc_police_rate_cmd_imi,
     "no police <1-1000000> <0-20000000> <1-20000000> exceed-action "
     "(drop | flow-control) reset-flow-control-mode available-bucket-room "
     "(full | cbs)",
     "Negate a command or set its defaults",
     "Specify a policer for the classified rate",
     "Specify an average traffic rate (kbps)",
     "Specify a normal burst size (bytes) CBS <0-20000000>",
     "Specify an exceed burst size in (bytes) EBS <1-20000000> ",
     "Specify action if exceed-action",
     "Drop the frame",
     "Send a pause frame and pass packet",
     "Specify to generate flow control",
     "De-assert flow when bucket room is full",
     "De-assert flow when bucket has enough room");

IMI_ALI (NULL,
     no_mls_qos_frame_type_priority_cmd_imi,
     "no mls qos frame-type-priority-override (bpdu-to-cpu |"
     "ucast-mgmt-to-cpu | from-cpu | port-etype-match)",
     "Negate a command or set its defaults",
     "Multi-Layer Switch(L2/L3).",
     "Quality of Service.",
     "Specify frame type priority",
     "Specify bpdu-to-cpu",
     "Specify ucast-mgmt-to-cpu",
     "Specify from-cpu",
     "Specify port-etype-match",
     "Specify QUEUE_ID");

IMI_ALI (NULL,
      mstp_span_portfast_bpdufilter_cmd_imi,
      "spanning-tree portfast bpdu-filter",
      "spanning-tree",
      "portfast",
      "Filter the BPDUs on portfast enabled ports");

IMI_ALI (NULL,
     no_spanning_vlan_cmd_imi,
     "no vlan <2-4094>",
     "Negate a command or set its defaults",
     "Vlan",
     "Vlan Id");

IMI_ALI (NULL,
     mirror_interface_both_cmd_imi,
     "mirror interface IFNAME direction both",
     "Port mirroring command",
     "Interface to use",
     "Source Interface to use",
     "Mirroring direction",
     "Mirror traffic in both directions");

IMI_ALI (generic_show_func,
     show_debugging_mstp_cmd_imi,
     "show debugging mstp",
     "Show running system information",
     "Debugging information outputs",
     "Multiple Spanning Tree Protocol (MSTP)");

IMI_ALI (NULL,
 timedelmech_config_cmd_imi,
 "set-delay-mech  (p2p|e2e)",
 "set time delay mech",
 "peer to peer",
 "end to end");

IMI_ALI (NULL,
     flow_ctrl_set_cmd_imi,
     "flowcontrol wmpause <0-255> wmcancel <0-255>",
     "IEEE 802.3x Flow Control",
     "Watermark pause command",
     "Watermark pause value",
     "Watermark cancel command",
     "Watermark cancel value"
    );

IMI_ALI (NULL,
     mls_qos_frame_type_priority_cmd_imi,
     "mls qos frame-type-priority-override "
     "(bpdu-to-cpu | ucast-mgmt-to-cpu |  from-cpu | port-etype-match) <0-3>",
     "Multi-Layer Switch(L2/L3).",
     "Quality of Service.",
     "Specify frame type priority",
     "Specify bpdu-to-cpu",
     "Specify ucast-mgmt-to-cpu",
     "Specify from-cpu",
     "Specify port-etype-match",
     "Specify QUEUE_ID");

IMI_ALI (NULL,
     ip_access_list_standard_nomask_cmd_imi,
     "ip-access-list (<1-99>|<1300-1999>) (deny|permit) A.B.C.D",
     "QoS's IP access list",
     "IP standard access list", "IP standard access list (expanded range)", "Specify packets to reject", "Specify pactets to permit",
     "Address to match");

IMI_ALI (NULL,
    cc_detect_cmd_imi,
    "cc {enable |disable}",
    "cc detect:enable or disable ",
    "cc dection configure"
);

IMI_ALI (NULL,
     set_gmrp_br_instance_enable_cmd_imi,
     "set (gmrp|mmrp) enable bridge BRIDGE_NAME vlan VLANID",
     "Enable GMRP globally for the bridge",
     "GARP Multicast Registration Protocol",
     "MRP Multicast Registration Protocol",
     "Enable",
     "Identify the name of the bridge to use",
     "The text string to use for the name of the bridge",
     "The Vlan of the instance",
     "VLAN ID of the instance");

IMI_ALI (NULL,
    bind_section_tol3p_and_peerip_cmd_imi,
    "local-port NAME [peer-port IP-ADDRESS]",
   "bind section to local-port",
    "set local-port name and peer-port ip-address"
);

IMI_ALI (generic_show_func,
     show_spanning_tree_rpvst_detail_interface_cmd_imi,
     "show spanning-tree rpvst+ detail interface IFNAME",
     "Show running system information",
     "spanning-tree Display spanning tree information",
     "Display RPVST+ information",
     "Display detailed information",
     "Interface information",
     "Interface name");

IMI_ALI (NULL,
     spanning_port_inst_priority_cmd_imi,
     "spanning-tree instance <1-63> priority <0-240>",
     "Spanning Tree Commands",
     "multiple spanning tree instance",
     "identifier",
     "port priority for a bridge",
     "port priority in increments of 16 (lower priority indicates greater likelihood of becoming root)");

IMI_ALI (NULL,
     spanning_port_vlan_priority_cmd_imi,
     "spanning-tree vlan <2-4094> priority <0-240>",
     "Spanning Tree Commands",
     "rapid Pervlan spanning tree instance",
     "identifier",
     "port priority for a bridge",
     "port priority in increments of 16 (lower priority indicates greater likelihood of becoming root)");

IMI_ALI (NULL,
     wrr_queue_min_reserve_cmd_imi,
     "wrr-queue min-reserve <0-7> <1-8>",
     "Wrr queue",
     "Configure the buffer size of the minimum-reserve level",
     "Specify a queue ID",
     "Specify a min-reserve level ID");

IMI_ALI (NULL,
     no_spanning_vlan_path_cost_cmd_imi,
     "no spanning-tree vlan <2-4094> path-cost",
     "Negate a command or set its defaults",
     "Spanning Tree Commands",
     "Rapid Pervlan Spanning Tree Instance",
     "identifier",
     "path-cost - path cost for a port");

IMI_ALI (NULL,
     no_mstp_spanning_tree_portfast_cmd_imi,
     "no spanning-tree (portfast | edgeport)",
     "Negate a command or set its defaults",
     "spanning-tree",
     "portfast - disable fast transitions",
     "edgeport - disable it as edgeport");

IMI_ALI (NULL,
     mstp_spanning_tree_inst_restricted_role_cmd_imi,
     "spanning-tree instance <1-63> restricted-role",
     "spanning tree commands",
     "Set restrictions for the port of particular instance",
     "instance id",
     "restrict the role of the port");

IMI_ALI (generic_show_func,
     fm_show_proc_names_cmd_imi,
     "show proc-names",
     "Show command",
     "Show process names");

IMI_ALI (NULL,
    announce_interval_config_cmd_imi,
    "set-announce-interval INTERVAL",
    "set announce interval",
    "announce interval, range <-128 - 127>");

IMI_ALI (generic_show_func,
     show_spanning_tree_mst_detail_cmd_imi,
     "show spanning-tree mst detail",
     "Show running system information",
     "spanning-tree Display spanning tree information",
     "Display MST information",
     "Display detailed information" );

IMI_ALI (NULL,
     mls_qos_trust_cmd_imi,
     "mls qos trust dscp)",
     "Multi-Layer Switch(L2/L3).",
     "Quality of Service.",
     "Confiure port trust state",
     "Classifies ingress packets with the packet DSCP values");

IMI_ALI (NULL,
     set_gmrp_fwd_all1_cmd_imi,
     "set (gmrp|mmrp) fwdall enable IF_NAME",
     "Set GMRP Forward All Option",
     "GARP Multicast Registration Protocol",
     "MRP Multicast Registration Protocol",
     "Forward All Option",
     "Enable",
     "Ifname");

IMI_ALI (NULL,
     no_mls_qos_map_dscp_queue_cmd_imi,
     "no mls qos dscp-queue <0-63> <0-3>",
     "Negate a command or set its defaults",
     "Multi-Layer Switch(L2/L3).",
     "Quality of Service.",
     "DSCP-to-QUEUE",
     "Select dscp_value <0-64>",
     "Select queue id");

IMI_ALI (NULL,
     no_mls_qos_default_cos_cmd_imi,
     "no mls qos cos",
     "Negate a command or set its defaults",
     "Multi-Layer Switch(L2/L3).",
     "Quality of Service.",
     "Configure the default CoS values ");

IMI_ALI (NULL,
     ip_access_list_extended_host_any_cmd_imi,
     "ip-access-list (<100-199>|<2000-2699>) (deny|permit) ip host A.B.C.D any",
     "QoS's IP access list",
     "IP extended access list", "IP extended access list (expanded range)", "Specify packets to reject", "Specify pactets to permit",
     "Any Internet Protocol",
     "A single source host",
     "Source address",
     "Any destination host");

IMI_ALI (NULL,
    config_lsp_mip_or_tail_entity_cmd_imi,
     "tun ID lsp ID Ingress LSRID egress LSRID",
     "configure tun ID",
     "configure lsp ID",
     "configure Ingress LSRID",
     "configure egress LSRID"
);

IMI_ALI (NULL,
    port_status_config_cmd_imi,
    "set-port-status STATUS",
    "set port status",
    "port status, range <master slave passive>");

IMI_ALI (NULL,
     debug_mstp_packet_rx_cmd_imi,
     "debug mstp packet rx",
     "Debugging functions (see also 'undebug')",
     "Multiple Spanning Tree Protocol (MSTP)",
     "MSTP Packets",
     "Receive");

IMI_ALI (NULL,
     set_gmrp2_br_cmd_imi,
     "set (gmrp|mmrp) disable bridge BRIDGE_NAME",
     "Disable GMRP globally for the bridge instance",
     "GARP Multicast Registration Protocol",
     "MRP Multicast Registration Protocol",
     "Disable",
     "Bridge instance ",
     "Bridge instance name");

IMI_ALI (NULL,
     no_qos_cmap_match_exp_cmd_imi,
     "no match mpls exp-bit topmost",
     "Negate a command or set its defaults",
     "Define the match criteria",
     "Specify Multi Protocol Label Switch specific values",
     "Specify MPLS experimental bits",
     "Match MPLS experimental value on topmost label");

IMI_ALI (NULL,
    no_config_section_mode_cmd_imi,
    "no section ID",
   "delete section id"
);

IMI_ALI (NULL,
    fpga_write_cmd_imi,
    "debug fpga-write ADDR VALUE",
    "Debugging functions (see also 'undebug')",
    "write value to fpag",
    "fpga address, in HHHH format",
    "fpga value, in HHHH format");

IMI_ALI (NULL,
    p2p_request_interval_config_cmd_imi,
    "set-p2p-req-interval INTERVAL",
    "set p2p request interval",
    "log p2p request interval, range -4 - 4");

IMI_ALI (NULL,
     no_ip_access_list_extended_any_mask_cmd_imi,
     "no ip-access-list (<100-199>|<2000-2699>) (deny|permit) ip any A.B.C.D A.B.C.D",
     "Negate a command or set its defaults",
     "QoS's IP access list",
     "IP extended access list", "IP extended access list (expanded range)", "Specify packets to reject", "Specify pactets to permit",
     "Any Internet Protocol",
     "Any source host",
     "Destination address",
     "Destination Wildcard bits");

IMI_ALI (NULL,
     nsm_bridge_clear_fdb_vlan_port_cmd_imi,
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

IMI_ALI (NULL,
     no_mstp_spanning_forward_time_cmd_imi,
     "no spanning-tree forward-time ",
     "Negate a command or set its defaults",
     "Spanning Tree Commands",
     "forward-time - forwarding delay time");

IMI_ALI (NULL,
     qos_cmap_match_ip_precedence_cmd_imi,
     "match ip-precedence (<0-7>|<0-7> <0-7>|<0-7> <0-7> <0-7>|<0-7> <0-7> <0-7> <0-7>|<0-7> <0-7> <0-7> <0-7> <0-7>|<0-7> <0-7> <0-7> <0-7> <0-7> <0-7>|<0-7> <0-7> <0-7> <0-7> <0-7> <0-7> <0-7>|<0-7> <0-7> <0-7> <0-7> <0-7> <0-7> <0-7> <0-7>)",
     "Define the match creteria",
     "List IP precedence vlaues",
     "Specify IP precedence value",
     "Specify IP precedence value",
     "Specify IP precedence value",
     "Specify IP precedence value",
     "Specify IP precedence value",
     "Specify IP precedence value",
     "Specify IP precedence value",
     "Specify IP precedence value",
     "Specify IP precedence value",
     "Specify IP precedence value",
     "Specify IP precedence value",
     "Specify IP precedence value",
     "Specify IP precedence value",
     "Specify IP precedence value",
     "Specify IP precedence value",
     "Specify IP precedence value",
     "Specify IP precedence value",
     "Specify IP precedence value",
     "Specify IP precedence value",
     "Specify IP precedence value",
     "Specify IP precedence value",
     "Specify IP precedence value",
     "Specify IP precedence value",
     "Specify IP precedence value",
     "Specify IP precedence value",
     "Specify IP precedence value",
     "Specify IP precedence value",
     "Specify IP precedence value",
     "Specify IP precedence value",
     "Specify IP precedence value",
     "Specify IP precedence value",
     "Specify IP precedence value",
     "Specify IP precedence value",
     "Specify IP precedence value",
     "Specify IP precedence value",
     "Specify IP precedence value");

IMI_ALI (generic_show_func,
     show_gmrp_statistics_cmd_imi,
     "show (gmrp|mmrp) statistics vlanid <1-4094>",
     "CLI_SHOW_STR",
     "gmrp",
     "MRP Multicast Registration Protocol",
     "GMRP Statistics for the given vlanid",
     "Vlanid",
     "Vlanid value <1-4094>");

IMI_ALI (NULL,
     set_gvrp_dyn_vlan_disable_cmd_imi,
     "set (gvrp|mvrp) dynamic-vlan-creation disable",
     "Set values in destination routing protocol",
     "GARP Vlan Registration Protocol",
     "MRP Vlan Registration Protocol",
     "Dynamic Vlan creation",
     "Disable");

IMI_ALI (generic_show_func,
     show_mls_qos_maps_policed_dscp_cmd_imi,
     "show mls qos maps policed-dscp",
     "Show running system information",
     "Multi-Layer Switch(L2/L3).",
     "Quality of Service.",
     "Select QoS map",
     "Policed-DSCP map");

IMI_ALI (NULL,
     no_qos_pmap_class_cmd_imi,
     "no class NAME",
     "Negate a command or set its defaults",
     "Specify class map",
     "Specify class map name");

IMI_ALI (NULL,
     set_gvrp_registration_normal_cmd_imi,
     "set (gvrp|mvrp) registration normal IF_NAME",
     "Set GVRP Registration to Normal Registration",
     "GARP Vlan Registration Protocol",
     "MRP Vlan Registration Protocol",
     "GVRP Registration",
     "Normal Registration",
     "Ifname");

IMI_ALI (NULL,
     no_nsm_if_shutdown_cmd_imi,
     "no shutdown",
     "Negate a command or set its defaults",
     "Shutdown the selected interface");

IMI_ALI (NULL,
     nsm_vlan_switchport_pvid_cmd_imi,
     "switchport (access|hybrid) vlan ""<2-4094>",
     "Set the switching characteristics of the Layer2 interface",
     "Set the Layer2 interface as Access",
     "Set the Layer2 interface as hybrid",
     "Set the default VLAN for the interface",
     "The default VID for the interface");

IMI_ALI (NULL,
     nsm_cus_vlan_name_cmd_imi,
     "vlan ""<2-4094>"" type customer name WORD "
     "(state (enable|disable)|)",
     "Add, delete, or modify values associated with a single VLAN",
     "VLAN id",
     "VLAN Type",
     "Customer VLAN",
     "Ascii name of the VLAN",
     "The ascii name of the VLAN",
     "Operational state of the VLAN",
     "Enable",
     "Disable");

IMI_ALI (generic_show_func,
     show_mac_acl_name_cmd_imi,
     "show mac-acl <2000-2699>",
     "Show running system information",
     "List MAC access lists",
     "MAC ACL NAME");

IMI_ALI (NULL,
    vpn_rmv_mutigrp_vport_cmd_imi,
    "rmv-muti-vport grp-id <1-8> (ac|pw) <1-2000>",
    "rmv an muticast port from muticast group in vpn",
    "muticast group index",
    "id value",
    "attachment circuit",
    "pseudo wire",
    "vport id value");

IMI_ALI (NULL,
     nsm_ser_vlan_no_name_cmd_imi,
     "vlan ""<2-4094>"" type service (point-point|multipoint-multipoint)",
     "Add, delete, or modify values associated with a single VLAN",
     "Point to Point Service VLAN",
     "Multi Point to Multi Point Service VLAN",
     "VLAN id",
     "VLAN Type",
     "Customer VLAN");

IMI_ALI (NULL,
     no_spanning_tree_ageing_time_cmd_imi,
     "no spanning-tree ageing-time",
     "Negate a command or set its defaults",
     "Spanning Tree Commands",
     "ageing-time");

IMI_ALI (NULL,
     qos_pmapc_set_quantumpriority_cmd_imi,
     "set drr-priority <0-7> quantum <1-255>",
     "Set Qos Parameters",
     "DRR Priority",
     "DRR Priority Value",
     "DRR Quantum",
     "DRR Quantum Value");

IMI_ALI (NULL,
     set_gmrp_registration3_cmd_imi,
     "set (gmrp|mmrp) registration forbidden IF_NAME",
     "Set GMRP Registration to Forbidden Registration",
     "GARP Multicast Registration Protocol",
     "MRP Multicast Registration Protocol",
     "GMRP Registration",
     "Forbidden Registration",
     "Interface name");

IMI_ALI (NULL,
     mstp_bridge_group_path_cost_cmd_imi,
     "bridge-group <1-32> path-cost <1-200000000>",
     "Bridge group commands",
     "Bridge Group name for bridging",
     "path cost for a port",
     "path cost in range <1-200000000> (lower path cost indicates greater "
     "likelihood of becoming root)");

IMI_ALI (NULL,
    lsp_cfg_down_cmd_imi,
    "config ingress (fe <1-24>|ge <1-4>) label <0-1048575> (vlan <0-4095>|)",
    "lsp config",
    "ingress tunnel",
    "ingress-tun interface type",
    "fe interface id",
    "ingress-tun interface type",
    "ge interface id",
    "ingress-tun lable",
    "lable value",
    "ingress-tun vlan",
    "vlan id, 0 means vlan diable");

IMI_ALI (NULL,
     spanning_vlanif_cmd_imi,
     "spanning-tree vlan <2-4094>",
     "Spanning Tree Commands",
     "Rapid Pervlan Spanning Tree Vlan Instance",
     "Vlan identifier");

IMI_ALI (NULL,
     no_debug_nsm_ha_all_cmd_imi,
     "no debug nsm ha all",
     "Negate a command or set its defaults",
     "Debugging functions (see also 'undebug')",
     "Network Service Module (NSM)",
     "NSM High Availability",
     "All NSM High Availability events");

IMI_ALI (NULL,
     no_spanning_inst_priority_cmd_imi,
     "no spanning-tree instance <1-63> priority",
     "Negate a command or set its defaults",
     "Spanning Tree Commands",
     "Change priority for a particular instance",
     "instance id",
     "priority - Reset bridge priority to default for the instance");

IMI_ALI (NULL,
     set_gvrp_dyn_vlan_enable_cmd_imi,
     "set (gvrp|mvrp) dynamic-vlan-creation enable",
     "Set values in destination routing protocol",
     "GARP Vlan Registration Protocol",
     "MRP Vlan Registration Protocol",
     "Dynamic Vlan creation",
     "Enable");

IMI_ALI (NULL,
     nsm_br_vlan_mtu_cmd_imi,
     "vlan ""<2-4094>"" mtu MTU_VAL bridge <1-32>",
     "Add, delete, or modify values associated with a single VLAN",
     "VLAN id",
     "MTU Value",
     "The Maximum Transmission Unit value for the vlan",
     "Bridge group commands.",
     "Bridge group for bridging.");

IMI_ALI (NULL,
     ip_access_list_extended_host_mask_cmd_imi,
     "ip-access-list (<100-199>|<2000-2699>) (deny|permit) ip host A.B.C.D A.B.C.D A.B.C.D",
     "QoS's IP access list",
     "IP extended access list", "IP extended access list (expanded range)", "Specify packets to reject", "Specify pactets to permit",
     "Any Internet Protocol",
     "A single source host",
     "Source address",
     "Destination address",
     "Destination Wildcard bits");

IMI_ALI (generic_show_func,
     show_debugging_nsm_cmd_imi,
     "show debugging nsm",
     "Show running system information",
     "Debugging functions (see also 'undebug')",
     "Network Service Module (NSM)");

IMI_ALI (generic_show_func,
     show_ip_interface_if_brief_cmd_imi,
     "show ip interface IFNAME brief",
     "Show running system information",
     "Internet Protocol (IP)",
     "IP interface status and configuration",
     "Interface name",
     "Brief summary of IP status and configuration");

IMI_ALI (NULL,
     nsm_vlan_classifier_ipv4_cmd_imi,
     "vlan classifier rule <1-256> ipv4 A.B.C.D/M vlan ""<2-4094>",
     "Vlan commands",
     "Vlan classification commands",
     "Vlan classification rules commands",
     "Vlan classifier rule id",
     "ipv4 address classification",
     "ipv4 address in A.B.C.D/M format",
     "Vlan",
     "Vlan Identifier");

IMI_ALI (NULL,
     nsm_br_ser_vlan_no_name_cmd_imi,
     "vlan ""<2-4094>"" type service (point-point|multipoint-multipoint) "
     "bridge <1-32>",
     "Add, delete, or modify values associated with a single VLAN",
     "VLAN id",
     "VLAN Type",
     "Service VLAN",
     "Point to Point Service VLAN",
     "Multi Point to Multi Point Service VLAN",
     "Bridge group commands.",
     "Bridge group for bridging.");

IMI_ALI (generic_show_func,
     show_lspoam_status_cmd_imi,
     "show lsp-oam-status <0-63>",
     "show command",
     "lsp oam status",
     "lsp oam id");

IMI_ALI (NULL,
     nsm_bridge_group_span_cmd_imi,
     "bridge-group <1-32> spanning-tree (disable|enable)",
     "Bridge group commands.",
     "Bridge group for bridging.",
     "spannning tree commmand",
     "Disable spanning tree on the interface",
     "Enable spanning tree on the interface");

IMI_ALI (NULL,
     set_gvrp_registration_forbidden_cmd_imi,
     "set (gvrp|mvrp) registration forbidden IF_NAME",
     "Set GVRP Registration to Forbidden Registration",
     "GARP Vlan Registration Protocol",
     "MRP Vlan Registration Protocol",
     "GVRP Registration",
     "Forbidden Registration",
     "Interface Name");

IMI_ALI (generic_show_func,
     nsm_show_bridge_cmd_imi,
     "show bridge (|ieee|rstp|rpvst+|mstp|provider-mstp|provider-rstp)",
     "Show running system information",
     "Display forwarding information",
     "Forwarding information of STP Bridges",
     "Forwarding information of RSTP Bridges",
     "Forwarding information of RPVST Bridges",
     "Forwarding information of MSTP Bridges");

IMI_ALI (NULL,
     no_debug_mstp_proto_cmd_imi,
     "no debug mstp protocol",
     "Negate a command or set its defaults",
     "Debugging functions (see also 'undebug')",
     "Multiple Spanning Tree Protocol (MSTP)",
     "MSTP Protocol");

IMI_ALI (NULL,
     debug_mstp_all_cmd_imi,
     "debug mstp all",
     "Debugging functions (see also 'undebug')",
     "Multiple Spanning Tree Protocol (MSTP)",
     "all");

IMI_ALI (NULL,
     no_debug_all_nsm_cmd_imi,
     "no debug all",
     "Negate a command or set its defaults",
     "Debugging functions (see also 'undebug')",
     "Turn off all Debugging");

IMI_ALI (generic_show_func,
     show_gvrp_machine_cmd_imi,
     "show (gvrp|mvrp) machine",
     "CLI_SHOW_STR",
     "Generic Attribute Registration Protocol",
     "MRP Vlan Registration Protocol",
     "Finite State Machine");

IMI_ALI (NULL,
     no_nsm_bridge_protocol_cmd_imi,
     "no bridge (<1-32> | backbone)",
     "Negate a command or set its defaults",
     "Bridge group commands.",
     "Bridge group for bridging.",
     "De-associates Backbone bridge");

IMI_ALI (NULL,
     no_bridge_instance_cmd_imi,
     "no bridge (<1-32> | backbone) instance <1-63>",
     "Negate a command or set its defaults",
     "Bridge group commands",
     "Bridge Group name for bridging",
     "Specifies the backbone bridge group name",
     "Instance",
     "ID");

IMI_ALI (NULL,
     set_gmrp_registration2_cmd_imi,
     "set (gmrp|mmrp) registration fixed IF_NAME",
     "Set GMRP Registration to Fixed Registration",
     "GARP Multicast Registration Protocol",
     "MRP Multicast Registration Protocol",
     "GMRP Registration",
     "Fixed Registration",
     "Ifname");

IMI_ALI (NULL,
     no_mstp_bridge_group_path_cost_cmd_imi,
     "no bridge-group <1-32> path-cost",
     "Negate a command or set its defaults",
     "Bridge group commands",
     "Bridge Group name for bridging",
     "path-cost - path cost for a port");

IMI_ALI (NULL,
     nsm_vlan_switchport_hybrid_add_egress_cmd_imi,
     "switchport (hybrid) allowed vlan add VLAN_ID egress-tagged (enable|disable)",
     "Set the switching characteristics of the Layer2 interface",
     "Set the Layer2 interface as hybrid",
     "Set the VLANs that will Xmit/Rx through the Layer2 interface",
     "VLAN that will be added ",
     "Add a VLAN to Xmit/Rx through the Layer2 interface",
     "The List of the VLAN IDs that will be added to the Layer2 interface",
     "Set the egress tagging for the outgoing frames",
     "Set the egress tagging on the outgoing frames to enabled",
     "Set the egress tagging on the outgoing frames to disabled");

IMI_ALI (NULL,
    announce_receipt_config_cmd_imi,
    "set-announce-receipt <0 - 255>",
    "set announce receipt timeout",
    "announce receipt timeout, range <0 - 255>");

IMI_ALI (generic_show_func,
     show_mac_acl_cmd_imi,
     "show mac-acl",
     "Show running system information",
     "List MAC access lists");

IMI_ALI (NULL,
    vpn_add_vport_cmd_imi,
    "add-vport (ac|pw) (<1-2000> | <1-2000> (hub-member|spoke-member))",
    "add an attachment circuit or pseudo wire to vpn",
    "vport type, attachment circuit",
    "vport type, pseudo wire",
    "vport id value",
    "vport id value",
    "hub, not valid in vpws",
    "spoke, not valid in vpws");

IMI_ALI (NULL,
     no_ip_access_list_standard_any_cmd_imi,
     "no ip-access-list (<1-99>|<1300-1999>) (deny|permit) any",
     "Negate a command or set its defaults",
     "QoS's IP access list",
     "IP standard access list", "IP standard access list (expanded range)", "Specify packets to reject", "Specify pactets to permit",
     "Any source host");

IMI_ALI (NULL,
     undebug_nsm_kernel_cmd_imi,
     "undebug nsm kernel",
     "Disable debugging functions (see also 'debug')",
     "Network Service Module (NSM)",
     "NSM kernel");

IMI_ALI (NULL,
     set_gmrp_br_instance_disable_cmd_imi,
     "set (gmrp|mmrp) disable bridge BRIDGE_NAME vlan VLANID",
     "Disable GMRP globally for the bridge",
     "GARP Multicast Registration Protocol",
     "MRP Multicast Registration Protocol",
     "Disable",
     "Identify the name of the bridge to use",
     "The text string to use for the name of the bridge",
     "The Vlan of the instance",
     "VLAN ID of the instance");

IMI_ALI (NULL,
    pw_car_cmd_imi,
    "car (disable|enable cir <0-1000000> cbs <0-4095> pir <0-1000000> pbs <0-4095>)",
    "config car paras",
    "disable car",
    "enable car",
    "committed information rate",
    "rate value, kbps",
    "committed brust size",
    "brust size value, bytes",
    "peak information rate",
    "rate value, kbps",
    "peak brust size",
    "brust size value, bytes");

IMI_ALI (NULL,
     qos_pmapc_police_rate_cmd_imi,
     "police <1-1000000> <0-20000000> <1-20000000> exceed-action "
     "(drop | flow-control) reset-flow-control-mode available-bucket-room "
     "(full | cbs)",
     "Specify a policer for the classified rate",
     "Specify an average traffic rate (kbps)",
     "Specify a burst size in (bytes) EBS <0-20000000> ",
     "Specify an exceed burst size in (bytes) EBS <1-20000000> ",
     "Specify action if exceed-action",
     "Drop the frame",
     "Send a pause frame and pass packet",
     "Specify to generate flow control",
     "Specify when to deassert flow control",
     "De-assert flow when bucket room is full",
     "De-assert flow when bucket has enough room");

IMI_ALI (NULL,
     no_ip_access_list_extended_mask_any_cmd_imi,
     "no ip-access-list (<100-199>|<2000-2699>) (deny|permit) ip A.B.C.D A.B.C.D any",
     "Negate a command or set its defaults",
     "QoS's IP access list",
     "IP extended access list", "IP extended access list (expanded range)", "Specify packets to reject", "Specify pactets to permit",
     "Any Internet Protocol",
     "Source address",
     "Source wildcard bits",
     "Any destination host");

IMI_ALI (generic_show_func,
     show_spanning_tree_stats_bridge_cmd_imi,
     "show spanning-tree statistics bridge <1-32>",
     "Show running system information",
     "Display Spanning-tree Information",
     "Statistics of the BPDUs",
     "Bridge",
     "Bridge ID");

IMI_ALI (NULL,
    delaymech_config_cmd_imi,
    "set-delay-mechanism (e2e|p2p)",
    "set delay mechanism",
    "end to end",
    "peer to peer");

IMI_ALI (NULL,
     no_mirror_interface_cmd_imi,
     "no mirror interface IFNAME",
     "Negate a command or set its defaults",
     "Port mirroring command",
     "Interface to use",
     "Source Interface to use");

IMI_ALI (NULL,
 scalelogvar_config_cmd_imi,
 "set-scale-log-var  <-2147483648 - 2147483647>",
 "set scalelogvar,used for variance algorithm",
 "scale log variance, range <-2147483648 - 2147483647>");

IMI_ALI (NULL,
    no_bind_section_tol3p_and_peerip_cmd_imi,
    "no local-port",
   "no bind section to local-port",
    "delete local-port name and peer-port ip-address"
);

IMI_ALI (NULL,
     mstp_spanning_path_cost_cmd_imi,
     "spanning-tree path-cost <1-200000000>",
     "Spanning Tree Commands",
     "path cost for a port",
     "path cost in range <1-200000000> (lower path cost indicates greater "
     "likelihood of becoming root)");

IMI_ALI (NULL,
     no_debug_nsm_packet_cmd_imi,
     "no debug nsm packet (recv|send|) (detail|)",
     "Negate a command or set its defaults",
     "Debugging functions (see also 'undebug')",
     "Network Service Module (NSM)",
     "NSM packets",
     "NSM receive packets",
     "NSM send packets",
     "Detailed information display");

IMI_ALI (NULL,
    cc_packet_send_period_cmd_imi,
    "cc period {default |10ms |100ms |1s |10s |1min |10min }",
    "cc period configure ",
    "cc period : 3.33ms ,10ms ,100ms ,1s ,10s ,1min ,10min "
);

IMI_ALI (NULL,
     set_gvrp1_cmd_imi,
     "set (gvrp|mvrp) enable",
     "Enable GVRP globally for the bridge instance",
     "GARP Vlan Registration Protocol",
     "MRP Vlan Registration Protocol",
     "Enable");

IMI_ALI (NULL,
     no_mls_qos_aggregate_police_cmd_imi,
     "no mls qos aggregate-police NAME",
     "Negate a command or set its defaults",
     "Multi-Layer Switch(L2/L3).",
     "Quality of Service.",
     "Aggregate police",
     "Specify aggregate policer name");

IMI_ALI (generic_show_func,
     show_gmrp_machine_cmd_imi,
     "show (gmrp|mmrp) machine",
     "CLI_SHOW_STR",
     "Generic Attribute Registration Protocol",
     "MRP Multicast Registration Protocol",
     "Finite State Machine");

IMI_ALI (NULL,
     clear_gmrp_br_statistics1_cmd_imi,
     "clear (gmrp|mmrp) statistics all bridge BRIDGE_NAME",
     "Clear GMRP statistics for all vlans",
     "GARP Multicast Registration Protocol",
     "MRP Multicast Registration Protocol",
     "GMRP Statistics",
     "All vlans",
     "Bridge instance",
     "Bridge instance name");

IMI_ALI (NULL,
     nsm_bridge_clear_fdb_cmd_imi,
     "clear mac address-table (static|multicast)",
     "Clear the Forwarding database",
     "Clear layer 2 MAC entries",
     "Clear all Entries in the forwarding database",
     "Clear all entries configured through management",
     "Clear all multicast entries");

IMI_ALI (NULL,
      no_mstp_spanning_tree_errdisable_timeout_interval_cmd_imi,
      "no bridge <1-32> spanning-tree errdisable-timeout interval ",
      "Negate a command or set its defaults",
      "Bridge group commands",
      "Bridge Group name for bridging",
      "spanning-tree",
      "errdisable-timeout",
      "disable the errdisable-timeout interval setting");

IMI_ALI (NULL,
    tun_car_cmd_imi,
    "car (disable|enable cir <0-1000000> cbs <0-4095> pir <0-1000000> pbs <0-4095>)",
    "config car paras",
    "disable car",
    "enable car",
    "committed information rate",
    "rate value, kbps",
    "committed brust size",
    "brust size value, bytes",
    "peak information rate",
    "rate value, kbps",
    "peak brust size",
    "brust size value, bytes");

IMI_ALI (NULL,
     nsm_vlan_classifier_mac_cmd_imi,
     "vlan classifier rule <1-256> mac WORD vlan ""<2-4094>",
     "Vlan commands",
     "Vlan classification commands",
     "Vlan classifier rules commands",
     "Vlan classifier rule id",
     "Mac address classification",
     "MAC - mac address in HHHH.HHHH.HHHH format",
     "Vlan",
     "Vlan Identifier");

IMI_ALI (NULL,
     nsm_bridge_group_cmd_imi,
     "bridge-group (<1-32> | backbone)",
     "Bridge group commands.",
     "Bridge group for bridging.",
     "Associates the interface with the B-component backbone bridge.");

IMI_ALI (NULL,
     no_nsm_br_pro_vlan_no_name_cmd_imi,
     "no vlan ""<2-4094>"" type (customer|service) bridge <1-32>",
     "Negate a command or set its defaults",
     "Add, delete, or modify values associated with a single VLAN",
     "VLAN id",
     "VLAN Type",
     "Customer VLAN",
     "Service VLAN",
     "Bridge group commands.",
     "Bridge group for bridging.");

IMI_ALI (NULL,
     mstp_spanning_tree_linktype_shared_cmd_imi,
     "spanning-tree link-type shared",
     "spanning tree commands",
     "link-type - point-to-point or shared",
     "shared - disable rapid transition");

IMI_ALI (NULL,
     nsm_vlan_name_cmd_imi,
     "vlan ""<2-4094>"" name WORD (state (enable|disable)|)",
     "Add, delete, or modify values associated with a single VLAN",
     "VLAN id",
     "Ascii name of the VLAN",
     "The ascii name of the VLAN",
     "Operational state of the VLAN",
     "Enable",
     "Disable");

IMI_ALI (NULL,
    no_creat_peer_mep_cmd_imi,
    "no peer-mep ID",
    "delete peer-mep ID ",
    "no creat peer mep"
);

IMI_ALI (generic_show_func,
     show_spanning_tree_mst_config_cmd_imi,
     "show spanning-tree mst config",
     "Show running system information",
     "spanning-tree Display spanning tree information",
     "Display MST information",
     "Display Configuration information");

IMI_ALI (NULL,
     no_ip_access_list_standard_nomask_cmd_imi,
     "no ip-access-list (<1-99>|<1300-1999>) (deny|permit) A.B.C.D",
     "Negate a command or set its defaults",
     "QoS's IP access list",
     "IP standard access list", "IP standard access list (expanded range)", "Specify packets to reject", "Specify pactets to permit",
     "Address to match");

IMI_ALI (NULL,
    delay_request_interval_config_cmd_imi,
    "set-delay-req-interval INTERVAL",
    "set delay request interval",
    "log interval value, range -128 - 127");

IMI_ALI (NULL,
    creat_mip_cmd_imi,
    "mip ID [interface NAME]",
    "configure mip ID and interface port-name ",
    "creat mip"
);

IMI_ALI (NULL,
     set_gvrp2_cmd_imi,
     "set (gvrp|mvrp) disable",
     "Disable GVRP globally for the bridge instance",
     "GARP Vlan Registration Protocol",
     "MRP Vlan Registration Protocol",
     "Disable");

IMI_ALI (NULL,
     no_bridge_vlan_priority_cmd_imi,
     "no bridge <1-32> vlan <2-4094> priority",
     "Negate a command or set its defaults",
     "Bridge group commands",
     "Bridge Group name for bridging",
     "Change priority for a particular vlan",
     "instance id",
     "priority - Reset bridge priority to default for the vlan");

IMI_ALI (NULL,
     no_lsp_id_cmd_imi,
     "no lsp <1-2000>",
     "Negate a command or set its defaults",
     "delete a lsp's configuration",
     "lsp id, range 1-2000");

IMI_ALI (NULL,
     bridge_group_vlan_priority_cmd_imi,
     "bridge-group <1-32> vlan <2-4094> priority <0-240>",
     "Bridge group commands",
     "Bridge Group name for bridging",
     "Rapid pervlan spanning tree vlan instance",
     "Vlan identifier",
     "port priority for a bridge",
     "port priority in increments of 16 (lower priority indicates greater likelihood of becoming root)");

IMI_ALI (NULL,
     no_mac_acl_host_host_cmd_imi,
     "no mac acl <2000-2699> (deny|permit) MAC MASK MAC MASK <1-8>",
     "Negate a command or set its defaults",
     "MAC access list", "MAC access list", "extended mac ACL <2000-2699>",
     "Specify packets to reject", "Specify packets to permit",
     "Source host's MAC address in HHHH.HHHH.HHHH format",
     "Source wildcard in HHHH.HHHH.HHHH format",
     "Destination host's MAC address in HHHH.HHHH.HHHH format",
     "Destination wildcard in HHHH.HHHH.HHHH format",
     "Specify packet format, (1:Ethernet II/ 2:802.3/ 4:SNMP/ 8:LLC)");

IMI_ALI (NULL,
     traffic_class_table_cmd_imi,
     "traffic-class-table user-priority <0-7> num-traffic-classes <1-8> value <0-7>",
     "Set the traffic class tables values",
     "User priority associated with the traffic class table",
     "value <0-7>",
     "Number of traffic classes that are supported",
     "Number of traffic classes value <1-8>",
     "Value to be used for the given user priority/num traffic classes",
     "Value <0-7>");

IMI_ALI (generic_show_func,
     show_mls_qos_maps_dscp_cos_cmd_imi,
     "show mls qos maps dscp-cos NAME",
     "Show running system information",
     "Multi-Layer Switch(L2/L3).",
     "Quality of Service.",
     "Select QoS map",
     "DSCP-to-COS map",
     "DSCP-to-COS map name");

IMI_ALI (NULL,
     no_bridge_acquire_cmd_imi,
     "no bridge <1-32> acquire",
     "Negate a command or set its defaults",
     "Bridge group commands.",
     "Bridge group for bridging.",
     "Disable dynamic learning of mac addresses");

IMI_ALI (NULL,
     no_mac_access_list_any_host_cmd_imi,
     "no mac-access-list <2000-2699> (deny|permit) any MAC MASK <1-8>",
     "Negate a command or set its defaults",
     "QOS's MAC access list", "extended mac ACL <2000-2699>",
     "Specify packets to reject", "Specify packets to permit",
     "Source any",
     "Destination host's MAC address in HHHH.HHHH.HHHH format",
     "Destination wildcard in HHHH.HHHH.HHHH format",
     "Specify packet format, (1:Ethernet II/ 2:802.3/ 4:SNMP/ 8:LLC)");

IMI_ALI (NULL,
     set_gmrp_timer1_cmd_imi,
     "set (gmrp|mmrp) timer join TIMER_VALUE IF_NAME",
     "Set GMRP join timer for the specified interface",
     "GARP Multicast Registration Protocol",
     "MRP Multicast Registration Protocol",
     "GARP timer",
     "Join Timer",
     "Timervalue in centiseconds",
     "Ifname");

IMI_ALI (generic_show_func,
     show_spanning_tree_rpvst_detail_cmd_imi,
     "show spanning-tree rpvst+ detail",
     "Show running system information",
     "spanning-tree Display spanning tree information",
     "Display RPVST+ information",
     "Display detailed information" );

IMI_ALI (NULL,
     no_bridge_static_filter_cmd_imi,
     "no bridge <1-32> address MAC discard IFNAME",
     "Negate a command or set its defaults",
     "Bridge group commands.",
     "Bridge group for bridging.",
     "address",
     "mac address in HHHH.HHHH.HHHH format",
     "discard",
     "Interface name");

IMI_ALI (NULL,
     no_mls_qos_dscp_mutation_cmd_imi,
     "no mls qos dscp-mutation NAME",
     "Negate a command or set its defaults",
     "Multi-Layer Switch(L2/L3).",
     "Quality of Service.",
     "Specify DSCP-to-DSCP mutation",
     "Specify the name of DSCP-to-DSCP mutaion map");

IMI_ALI (NULL,
     no_debug_nsm_kernel_cmd_imi,
     "no debug nsm kernel",
     "Negate a command or set its defaults",
     "Debugging functions (see also 'undebug')",
     "Network Service Module (NSM)",
     "NSM kernel");

IMI_ALI (NULL,
     no_mirror_interface_transmit_cmd_imi,
     "no mirror interface IFNAME direction transmit",
     "Negate a command or set its defaults",
     "Port mirroring command",
     "Interface to use",
     "Source Interface to use",
     "Mirroring direction",
     "Mirror received traffic");

IMI_ALI (NULL,
    ptp_port_enable_config_cmd_imi,
    "enable-ptp",
    "enable port's ptp function");

IMI_ALI (NULL,
     set_gmrp_ext_filtering_cmd_imi,
     "set (gmrp|mmrp) extended-filtering enable",
     "Enable Extended filtering Option",
     "GARP Multicast Registration Protocol",
     "MRP Multicast Registration Protocol",
     "Extended filtering Option",
     "Enable");

IMI_ALI (NULL,
     no_mstp_spanning_path_cost_cmd_imi,
     "no spanning-tree path-cost",
     "Negate a command or set its defaults",
     "Spanning Tree Commands",
     "path-cost - path cost for a port");

IMI_ALI (NULL,
     set_gmrp_timer2_cmd_imi,
     "set (gmrp|mmrp) timer leave TIMER_VALUE IF_NAME",
     "Set GMRP leave timer for the specified interface",
     "GARP Multicast Registration Protocol",
     "MRP Multicast Registration Protocol",
     "GARP timer",
     "Leave Timer",
     "Timervalue in centiseconds",
     "Ifname");

IMI_ALI (generic_show_func,
     show_ip_interface_brief_cmd_imi,
     "show ip interface brief",
     "Show running system information",
     "Internet Protocol (IP)",
     "IP interface status and configuration",
     "Brief summary of IP status and configuration");

IMI_ALI (NULL,
     mls_qos_vlan_priority_cmd_imi,
     "mls qos vlan-priority-override (bridge <1-32> |) VLANID <0-7>",
     "Multi-Layer Switch(L2/L3).",
     "Quality of Service.",
     "Vlan Priority Override",
     "Bridge group commands",
     "Bridge group name",
     "Select vlan id <1-4094>",
     "Select priority <0-7>");

IMI_ALI (NULL,
 port_shapping_cmd_imi,
 "shapping (disable|enable cir <0-1000000> cbs <0-4095>)",
    "config shapping paras",
    "disable shapping",
    "enable shapping",
    "committed information rate",
    "rate value, kbps",
    "committed brust size",
    "brust size value, bytes");

IMI_ALI (NULL,
     ip_access_list_standard_cmd_imi,
     "ip-access-list (<1-99>|<1300-1999>) (deny|permit) A.B.C.D A.B.C.D",
     "QoS's IP access list",
     "IP standard access list", "IP standard access list (expanded range)", "Specify packets to reject", "Specify pactets to permit",
     "Address to match",
     "Wildcard bits");

IMI_ALI (generic_show_func,
     show_spanning_tree_mst_instance_cmd_imi,
     "show spanning-tree mst instance (<1-63> | te-msti)",
     "Show running system information",
     "spanning-tree Display spanning tree information",
     "Display MST information",
     "Display instance information",
     "instance_id",
     "MSTI to be the traffic engineering MSTI instance");

IMI_ALI (NULL,
     mstp_bridge_max_age_cmd_imi,
     "bridge <1-32> max-age <6-40>",
     "Bridge group commands",
     "Bridge Group name for bridging",
     "max-age",
     "seconds <6-40> - Maximum time to listen for root bridge in seconds");

IMI_ALI (NULL,
     mstp_bridge_forward_time_cmd_imi,
     "bridge <1-32> forward-time <4-30>",
     "Bridge group commands",
     "Bridge Group name for bridging",
     "forward-time - forwarding delay time",
     "forward delay time in seconds");

IMI_ALI (NULL,
     set_gvrp1_br_cmd_imi,
     "set (gvrp|mvrp) enable bridge BRIDGE_NAME",
     "Enable GVRP globally for the bridge instance",
     "GARP Vlan Registration Protocol",
     "MRP Vlan Registration Protocol",
     "Enable",
     "Bridge instance ",
     "Bridge instance name");

IMI_ALI (NULL,
     debug_nsm_packet_cmd_imi,
     "debug nsm packet (recv|send|) (detail|)",
     "Debugging functions (see also 'undebug')",
     "Network Service Module (NSM)",
     "NSM packets",
     "NSM receive packets",
     "NSM send packets",
     "Detailed information display");

IMI_ALI (NULL,
     flowcontrol_receive_on_cmd_imi,
     "flowcontrol receive on",
     "IEEE 802.3x Flow Control",
     "Flow control on receive",
     "Turn on flow control");

IMI_ALI (NULL,
     no_mstp_port_hello_time_cmd_imi,
     "no spanning-tree hello-time",
     "Negate a command or set its defaults",
     "spanning tree commands",
     "hello-time - hello BDPU interval");

IMI_ALI (NULL,
    lb_detect_repeat_cmd_imi,
    "lb repeat <1-200> ",
    "lb configure ",
    "lb repeat configure:repeat 1-200,default is 3 ."
);

IMI_ALI (NULL,
     no_qos_cmap_match_traffic_type_cmd_imi,
     "no match traffic-type",
     "Negate a command or set its defaults",
     "Define the match criteria",
     "Specify Traffic Type");

IMI_ALI (NULL,
     no_mstp_spanning_tree_restricted_role_cmd_imi,
     "no spanning-tree restricted-role",
     "Negate a command or set its defaults",
     "spanning tree commands",
     "remove restriction on the role of the port");

IMI_ALI (NULL,
     debug_mstp_timer_cmd_imi,
     "debug mstp timer",
     "Debugging functions (see also 'undebug')",
     "Multiple Spanning Tree Protocol (MSTP)",
     "MSTP Timers");

IMI_ALI (generic_show_func,
     show_debugging_gmrp_cmd_imi,
     "show debugging gmrp",
     "Show running system information",
     "Debugging functions (see also 'undebug')",
     "GARP Multicast Registration Protocol (GMRP)");

IMI_ALI (NULL,
     vlan_add_static_filter_cmd_imi,
     "bridge <1-32> address MAC discard IFNAME vlan ""<2-4094>",
     "Bridge group commands.",
     "Bridge group for bridging.",
     "address",
     "mac address in HHHH.HHHH.HHHH format",
     "discard",
     "Interface name",
     "VLAN",
     "vlan id");

IMI_ALI (NULL,
     no_nsm_vlan_classifier_rule_cmd_imi,
     "no vlan classifier rule <1-256>",
     "Negate a command or set its defaults",
     "Vlan commands",
     "Vlan classification commands",
     "Vlan classification rules commands",
     "Vlan classifier rule id");

IMI_ALI (NULL,
      no_mstp_spanning_tree_errdisable_timeout_interval_cmd1_imi,
      "no spanning-tree errdisable-timeout interval ",
      "Negate a command or set its defaults",
      "spanning-tree",
      "errdisable-timeout",
      "interval after which port shall be enabled");

IMI_ALI (NULL,
    lb_detect_ttl_cmd_imi,
    "lb ttl <1-255> ",
    "lb configure",
    "lb ttl configure:ttl 1-255,default is 255"
);

IMI_ALI (NULL,
     nsm_access_group_any_cmd_imi,
     "ip-access-group (<1-199>|<1300-2699>|WORD) (in|out)",
     "Setting IP access group",
     "IP access list (Standard or Extended)",
     "IP Expanded access list (Standard or Extended)",
     "IP PacOS access-list name",
     "inbound packets",
     "outbound packets");

IMI_ALI (NULL,
     no_mstp_spanning_tree_linktype_cmd_imi,
     "no spanning-tree link-type",
     "Negate a command or set its defaults",
     "spanning-tree",
     "default (point-to-point) link type enables rapid transitions");

IMI_ALI (NULL,
     no_mstp_bridge_hello_time_cmd_imi,
     "no bridge <1-32> hello-time",
     "Negate a command or set its defaults",
     "Bridge group commands",
     "Bridge Group name for bridging",
     "hello-time - set Hello BPDU interval to default");

IMI_ALI (NULL,
     no_ip_access_list_extended_host_any_cmd_imi,
     "no ip-access-list (<100-199>|<2000-2699>) (deny|permit) ip host A.B.C.D any",
     "Negate a command or set its defaults",
     "QoS's IP access list",
     "IP extended access list", "IP extended access list (expanded range)", "Specify packets to reject", "Specify pactets to permit",
     "Any Internet Protocol",
     "A single source host",
     "Source address",
     "Any destination host");

IMI_ALI (NULL,
     no_mstp_spanning_transmit_hold_count_cmd_imi,
     "no spanning-tree transmit-holdcount <1-10>",
     "Negate a command or set its defaults",
     "Spanning Tree Commands",
     "Transmit hold count of the bridge",
     "range of the transmitholdcount");

IMI_ALI (NULL,
     set_gmrp_fwd_all2_cmd_imi,
     "set (gmrp|mmrp) fwdall disable IF_NAME",
     "Reset GMRP Forward All Option",
     "GARP Multicast Registration Protocol",
     "MRP Multicast Registration Protocol",
     "Forward All Option",
     "Disable",
     "Ifname");

IMI_ALI (NULL,
     nsm_ser_vlan_enable_disable_cmd_imi,
     "vlan ""<2-4094>"" type service (point-point|multipoint-multipoint)"
     "state (enable|disable)",
     "Add, delete, or modify values associated with a single VLAN",
     "VLAN id",
     "VLAN Type",
     "Service VLAN",
     "Point to Point Service VLAN",
     "Multi Point to Multi Point Service VLAN",
     "Operational state of the VLAN",
     "Enable",
     "Disable");

IMI_ALI (NULL,
     nsm_no_mac_access_group_cmd_imi,
     "no mac access-group WORD",
     "Negate a command or set its defaults",
     "Configure MAC parameters",
     "Configure MAC Access group",
     "Name for the MAC Access List");

IMI_ALI (generic_show_func,
     show_spanning_tree_stats_cmd_imi,
     "show spanning-tree statistics (interface IFNAME| "
                    "(instance <1-63>| vlan <1-4094>)) bridge <1-32> ",
     "Show running system information",
     "Display Spanning-tree Information",
     "Statistics of the BPDUs",
     "Interface",
     "Interface name",
     "Display Instance Information",
     "Instance ID",
     "Vlan",
     "VLAN ID Associated with the Instance",
     "Bridge",
     "Bridge ID");

IMI_ALI (NULL,
     debug_hal_events_cmd_imi,
     "debug nsm hal events",
     "Debugging functions (see also 'undebug')",
     "Network Service Module (NSM)",
     "Hardware Abstraction Layer",
     "NSM events");

IMI_ALI (NULL,
     set_gmrp_ext_filtering_br_cmd_imi,
     "set (gmrp|mmrp) extended-filtering enable bridge BRIDGE_NAME ",
     "Enable Extended filtering Option",
     "GARP Multicast Registration Protocol",
     "MRP Multicast Registration Protocol",
     "Extended filtering Option",
     "Enable",
     "Bridge",
     "BRIDGE_NAME");

IMI_ALI (NULL,
     flowcontrol_send_off_cmd_imi,
     "flowcontrol send off",
     "IEEE 802.3x Flow Control",
     "Flow control on send",
     "Turn off flow control");

IMI_ALI (generic_show_func,
     show_spanning_tree_interface_cmd_imi,
     "show spanning-tree interface IFNAME",
     "Show running system information",
     "Display spanning-tree information",
     "Interface information",
     "Interface name");

IMI_ALI (NULL,
     no_ip_access_list_extended_any_host_cmd_imi,
     "no ip-access-list (<100-199>|<2000-2699>) (deny|permit) ip any host A.B.C.D",
     "Negate a command or set its defaults",
     "QoS's IP access list",
     "IP extended access list", "IP extended access list (expanded range)", "Specify packets to reject", "Specify pactets to permit",
     "Any Internet Protocol",
     "Any source host",
     "A single destination host",
     "Destination address");

IMI_ALI (NULL,
     no_bridge_group_vlan_priority_cmd_imi,
     "no bridge-group <1-32> vlan <2-4094> priority",
     "Negate a command or set its defaults",
     "Bridge group commands",
     "Bridge Group name for bridging",
     "Rapid Pervlan Spanning Tree Instance",
     "identifier",
     "port priority for a bridge");

IMI_ALI (NULL,
     no_spanning_region_cmd_imi,
     "no region REGION_NAME",
     "Negate a command or set its defaults",
     "MST Region",
     "Name of the region");

IMI_ALI (NULL,
     no_mac_access_list_host_any_cmd_imi,
     "no mac-access-list <2000-2699> (deny|permit) MAC MASK any <1-8>",
     "Negate a command or set its defaults",
     "QOS's MAC access list", "extended mac ACL <2000-2699>",
     "Specify packets to reject", "Specify packets to permit",
     "Source host's MAC address in HHHH.HHHH.HHHH format",
     "Source wildcard in HHHH.HHHH.HHHH format",
     "Destination any",
     "Specify packet format, (1:Ethernet II/ 2:802.3/ 4:SNMP/ 8:LLC)");

IMI_ALI (NULL,
     nsm_vlan_switchport_trunk_pn_cn_add_cmd_imi,
     "switchport (trunk|provider-network|customer-network) allowed "
     "vlan add VLAN_ID",
     "Set the switching characteristics of the Layer2 interface",
     "Set the Layer2 interface as trunk",
     "Set the Layer2 interface as provider network",
     "Set the Layer2 interface as Customer Network",
     "Set the VLANs that will Xmit/Rx through the Layer2 interface",
     "VLAN that will be added",
     "Add a VLAN to Xmit/Tx through the Layer2 interface",
     "The List of the VLAN IDs that will be added to the Layer2 interface");

IMI_ALI (generic_show_func,
     show_spanning_tree_mst_cmd_imi,
     "show spanning-tree mst",
     "Show running system information",
     "spanning-tree Display spanning tree information",
     "Display MST information" );

IMI_ALI (NULL,
    portstatus_config_cmd_imi,
    "set-port-status  NUMBER    STATUS",
    "set port status",
    "port number, range <1-8>",
    "status, range <master slave passive>");

IMI_ALI (NULL,
     vlan_no_match_cmd_imi,
     "no match mac WORD",
     "Negate a command or set its defaults",
     "Configure VLAN Access Map Match",
     "Configure VLAN Access Map Match",
     "Mac access-list name");

IMI_ALI (NULL,
     nsm_vlan_switchport_ingress_filter_enable_cmd_imi,
     "switchport mode (access|hybrid|trunk|provider-network|customer-network) "
     "ingress-filter (enable|disable)",
     "Set the switching characteristics of the Layer2 interface",
     "Set the mode of the Layer2 interface",
     "Set the Layer2 interface as Access",
     "Set the Layer2 interface as hybrid",
     "Set the Layer2 interface as trunk",
     "Set the Layer2 interface as provider network",
     "Set the Layer2 interface as Customer Network",
     "Set the ingress filtering of the frames received",
     "Enable ingress filtering",
     "Disable ingress filtering");

IMI_ALI (NULL,
      no_mac_prio_override_cmd_imi,
      "no bridge <1-32> mac-priority-override mac-address MAC "
      "interface IFNAME vlan VLANID ",
       "Negate a command or set its defaults",
       "Bridge group commands.",
       "Bridge group for bridging.",
      "mac priority overide",
      "mac address",
      "mac address in HHHH.HHHH.HHHH format",
      "Interface information",
      "Interface name",
      "Add, delete, or modify values associated with a single VLAN",
      "VLAN Id");

IMI_ALI (NULL,
     no_mls_qos_dscp_cos_cmd_imi,
     "no mls qos dscp-cos NAME",
     "Negate a command or set its defaults",
     "Multi-Layer Switch(L2/L3).",
     "Quality of Service.",
     "Specify DSCP-to-COS",
     "Specify DSCP-to-DSCP map name");

IMI_ALI (NULL,
     no_qos_pmapc_policing_meter_cmd_imi,
     "no policing meter",
     "Negate a command or set its defaults",
     "Specify a policer for the classified traffic",
     "Value is average traffic by burst size");

IMI_ALI (NULL,
     no_debug_mstp_packet_tx_cmd_imi,
     "no debug mstp packet tx",
     "Negate a command or set its defaults",
     "Debugging functions (see also 'undebug')",
     "Multiple Spanning Tree Protocol (MSTP)",
     "MSTP Packets",
     "Transmit");

IMI_ALI (NULL,
     set_mmrp_point_to_point_cmd_imi,
     "set mmrp pointtopoint (enable|disable) interface IF_NAME",
     "set MMRP globally for the bridge",
     "MRP Multicast Registration Protocol",
     "point to point mode of interface",
     "enable",
     "disale",
     "Identify the name of the bridge to use",
     "The text string to use for the name of the bridge");

IMI_ALI (NULL,
     no_qos_cmap_match_l4_port_cmd_imi,
     "no match layer4 (source-port|destination-port) <1-65535>",
     "Negate a command or set its defaults",
     "Define the match criteria",
     "Specify TCP/UDP port",
     "Specify source TCP/UDP port",
     "Specify destination TCP/UDP port",
     "TCP/UDP port value");

IMI_ALI (NULL,
     no_mstp_bridge_ageing_time_cmd_imi,
     "no bridge <1-32> ageing-time",
     "Negate a command or set its defaults",
     "Bridge group commands",
     "Bridge Group name for bridging",
     "ageing-time");


void
imi_extracted_cmd_init (struct cli_tree *ctree)
{
  cli_install_imi (ctree, 16, PM_MSTP, PRIVILEGE_NORMAL, 0, &bridge_group_vlan_priority_cmd_imi);
  cli_install_imi (ctree, 5, modbmap_vor (2, &PM_NSM, &PM_NSM), PRIVILEGE_NORMAL, 0, &no_ip_access_list_standard_cmd_imi);
  cli_install_imi (ctree, 16, PM_NSM, PRIVILEGE_NORMAL, 0, &flowcontrol_receive_off_cmd_imi);
  cli_install_imi (ctree, 4, PM_NSM,  1,  0, &no_debug_nsm_ha_cmd_imi);
  cli_install_imi (ctree, 16, PM_MSTP, PRIVILEGE_NORMAL, 0, &bridge_group_inst_priority_cmd_imi);
  cli_install_imi (ctree, 71, PM_MSTP, PRIVILEGE_NORMAL, 0, &spanning_instance_cmd_imi);
  cli_install_imi (ctree, 4, PM_NSM,  1,  0, &no_debug_nsm_all_cmd_imi);
  cli_install_imi (ctree, 147, PM_NSM, PRIVILEGE_NORMAL, 0, &lsp_group_sw_cmd_imi);
  cli_install_imi (ctree, 4, PM_NSM,  1,  0, &debug_nsm_cmd_imi);
  cli_install_imi (ctree, 5, modbmap_vor (2, &PM_NSM, &PM_NSM), PRIVILEGE_NORMAL, 0, &mac_access_list_any_host_cmd_imi);
  cli_install_imi (ctree, 16, PM_NSM,  1,  0, &nsm_access_group_any_cmd_imi);
  cli_install_imi (ctree, 67, PM_NSM, PRIVILEGE_NORMAL, 0, &nsm_br_vlan_name_cmd_imi);
  cli_install_imi (ctree, 5, PM_NSM,  1,  0, &no_debug_nsm_cmd_imi);
  cli_install_imi (ctree, 5, modbmap_vor (2, &PM_NSM, &PM_NSM), PRIVILEGE_NORMAL, 0, &mac_access_list_host_any_cmd_imi);
  cli_install_imi (ctree, 4, PM_MSTP, PRIVILEGE_NORMAL, 0, &show_spanning_tree_rpvst_interface_cmd_imi);
  cli_install_imi (ctree, 16, modbmap_vor (2, &PM_NSM, &PM_NSM), PRIVILEGE_NORMAL, 0, &service_policy_input_cmd_imi);
  cli_install_imi (ctree, 4, PM_NSM,  1,  0, &no_debug_nsm_cmd_imi);
  cli_install_imi (ctree, 16, PM_NSM, PRIVILEGE_NORMAL, 0, &nsm_vlan_switchport_hybrid_delete_cmd_imi);
  cli_install_imi (ctree, 5, modbmap_vor (2, &PM_NSM, &PM_NSM), PRIVILEGE_NORMAL, 0, &mac_acl_host_host_cmd_imi);
  cli_install_imi (ctree, 145, PM_NSM, PRIVILEGE_NORMAL, 0, &domain_mport_add_cmd_imi);
  cli_install_imi (ctree, 146, PM_NSM, PRIVILEGE_NORMAL, 0, &ptp_port_disable_config_cmd_imi);
  cli_install_imi (ctree, 82, modbmap_vor (2, &PM_NSM, &PM_NSM), PRIVILEGE_NORMAL, 0, &no_qos_cmap_match_ip_dscp_cmd_imi);
  cli_install_imi (ctree, 5, PM_NSM, PRIVILEGE_NORMAL, 0, &gmrp_debug_all_cmd_imi);
  cli_install_imi (ctree, 5, PM_NSM, PRIVILEGE_NORMAL, 0, &show_lsp_status_cmd_imi);
  cli_install_imi (ctree, 5, PM_NSM, PRIVILEGE_NORMAL, 0, &set_mmrp_point_to_point_cmd_imi);
  cli_install_imi (ctree, 16, PM_MSTP, PRIVILEGE_NORMAL, 0, &mstp_spanning_tree_autoedge_cmd_imi);
  cli_install_imi (ctree, 141, PM_NSM, PRIVILEGE_NORMAL, 0, &gloal_oam_1731_enable_cmd_imi);
  cli_install_imi (ctree, 16, PM_MSTP, PRIVILEGE_NORMAL, 0, &no_mstp_spanning_path_cost_cmd_imi);
  cli_install_imi (ctree, 136, PM_NSM, PRIVILEGE_NORMAL, 0, &vpn_add_mutigrp_vport_cmd_imi);
  cli_install_imi (ctree, 16, PM_MSTP, PRIVILEGE_NORMAL, 0, &mstp_spanning_tree_inst_restricted_tcn_cmd_imi);
  cli_install_imi (ctree, 147, PM_NSM, PRIVILEGE_NORMAL, 0, &lsp_group_modify_cmd_imi);
  cli_install_imi (ctree, 5, PM_MSTP, PRIVILEGE_NORMAL, 0, &no_debug_mstp_packet_tx_cmd_imi);
  cli_install_imi (ctree, 82, modbmap_vor (2, &PM_NSM, &PM_NSM), PRIVILEGE_NORMAL, 0, &no_qos_cmap_match_traffic_type_cmd1_imi);
  cli_install_imi (ctree, 84, modbmap_vor (2, &PM_NSM, &PM_NSM), PRIVILEGE_NORMAL, 0, &no_qos_pmapc_police_aggregate_cmd_imi);
  cli_install_imi (ctree, 84, modbmap_vor (2, &PM_NSM, &PM_NSM), PRIVILEGE_NORMAL, 0, &qos_pmapc_policing_meter_cmd_imi);
  cli_install_imi (ctree, 5, PM_NSM, PRIVILEGE_NORMAL, 0, &spanning_acquire_cmd_imi);
  cli_install_imi (ctree, 5, PM_MSTP, PRIVILEGE_NORMAL, 0, &no_mstp_span_portfast_bpdufilter_cmd_imi);
  cli_install_imi (ctree, 16, PM_NSM, PRIVILEGE_NORMAL, 0, &nsm_vlan_switchport_trunk_pn_cn_delete_cmd_imi);
  cli_install_imi (ctree, 4, PM_MSTP, PRIVILEGE_NORMAL, 0, &show_spanning_tree_rpvst_detail_interface_cmd_imi);
  cli_install_imi (ctree, 143, PM_NSM, PRIVILEGE_NORMAL, 0, &lsp_cfg_down_cmd_imi);
  cli_install_imi (ctree, 4, PM_NSM, PRIVILEGE_NORMAL, 0, &show_gvrp_configuration_all_cmd_imi);
  cli_install_imi (ctree, 5, PM_NSM, PRIVILEGE_NORMAL, 0, &no_vpn_id_cmd_imi);
  cli_install_imi (ctree, 16, PM_MSTP, PRIVILEGE_NORMAL, 0, &no_spanning_vlanif_cmd_imi);
  cli_install_imi (ctree, 5, PM_MSTP, PRIVILEGE_NORMAL, 0, &no_debug_mstp_imish_cmd_imi);
  cli_install_imi (ctree, 5, PM_NSM, PRIVILEGE_NORMAL, 0, &set_port_gvrp1_cmd_imi);
  cli_install_imi (ctree, 5, PM_NSM, PRIVILEGE_NORMAL, 0, &set_gvrp_dyn_vlan_disable_cmd_imi);
  cli_install_imi (ctree, 5, modbmap_vor (2, &PM_NSM, &PM_NSM), PRIVILEGE_NORMAL, 0, &no_ip_access_list_extended_host_host_cmd_imi);
  cli_install_imi (ctree, 16, PM_NSM, PRIVILEGE_NORMAL, 0, &traffic_class_table_cmd_imi);
  cli_install_imi (ctree, 4, PM_NSM,  1,  0, &undebug_nsm_all_cmd_imi);
  cli_install_imi (ctree, 4, PM_MSTP, PRIVILEGE_NORMAL, 0, &mstp_clear_spanning_tree_detected_protocols_cmd_imi);
  cli_install_imi (ctree, 4, PM_NSM, PRIVILEGE_NORMAL, 0, &show_nsm_vlan_classifier_group_cmd_imi);
  cli_install_imi (ctree, 4, PM_NSM,  1,  0, &fpga_write_cmd_imi);
  cli_install_imi (ctree, 5, PM_NSM,  (15),  0, &no_debug_hal_cmd_imi);
  cli_install_imi (ctree, 84, modbmap_vor (2, &PM_NSM, &PM_NSM), PRIVILEGE_NORMAL, 0, &no_qos_pmapc_policing_meter_cmd_imi);
  cli_install_imi (ctree, 146, PM_NSM, PRIVILEGE_NORMAL, 0, &p2p_meanpath_delay_config_cmd_imi);
  cli_install_imi (ctree, 5, PM_NSM, PRIVILEGE_NORMAL, 0, &no_ac_id_cmd_imi);
  cli_install_imi (ctree, 16, modbmap_vor (2, &PM_NSM, &PM_NSM), PRIVILEGE_NORMAL, 0, &no_wrr_queue_min_reserve_cmd_imi);
  cli_install_imi (ctree, 5, PM_MSTP, PRIVILEGE_NORMAL, 0, &no_mstp_bridge_max_age_cmd_imi);
  cli_install_imi (ctree, 5, modbmap_vor (2, &PM_NSM, &PM_NSM), PRIVILEGE_NORMAL, 0, &no_ip_access_list_extended_any_any_cmd_imi);
  cli_install_imi (ctree, 110, PM_MSTP, PRIVILEGE_NORMAL, 0, &spanning_vlan_cmd_imi);
  cli_install_imi (ctree, 5, PM_NSM, PRIVILEGE_NORMAL, 0, &no_gmrp_debug_imish_cmd_imi);
  cli_install_imi (ctree, 5, PM_NSM,  1,  0, &no_debug_nsm_kernel_cmd_imi);
  cli_install_imi (ctree, 16, PM_NSM, PRIVILEGE_NORMAL, 0, &no_nsm_vlan_classifier_activate_cmd_imi);
  cli_install_imi (ctree, 5, PM_NSM,  (15),  0, &fpga_read_cmd_imi);
  cli_install_imi (ctree, 16, PM_NSM, PRIVILEGE_NORMAL, 0, &no_mirror_interface_cmd_imi);
  cli_install_imi (ctree, 16, PM_MSTP, PRIVILEGE_NORMAL, 0, &mstp_spanning_tree_guard_root_cmd_imi);
  cli_install_imi (ctree, 4, PM_MSTP, PRIVILEGE_NORMAL, 0, &no_debug_mstp_proto_detail_cmd_imi);
  cli_install_imi (ctree, 5, PM_NSM, PRIVILEGE_NORMAL, 0, &set_gvrp_dyn_vlan_enable_cmd_imi);
  cli_install_imi (ctree, 146, PM_NSM, PRIVILEGE_NORMAL, 0, &ptp_port_unbind_config_cmd_imi);
  cli_install_imi (ctree, 67, PM_NSM, PRIVILEGE_NORMAL, 0, &nsm_br_vlan_enable_disable_cmd_imi);
  cli_install_imi (ctree, 5, PM_NSM, PRIVILEGE_NORMAL, 0, &set_gvrp_registration_forbidden_cmd_imi);
  cli_install_imi (ctree, 5, PM_MSTP, PRIVILEGE_NORMAL, 0, &debug_mstp_all_cmd_imi);
  cli_install_imi (ctree, 4, PM_MSTP, PRIVILEGE_NORMAL, 0, &debug_mstp_packet_rx_cmd_imi);
  cli_install_imi (ctree, 140, PM_NSM, PRIVILEGE_NORMAL, 0, &lm_on_demand_detect_period_cmd_imi);
  cli_install_imi (ctree, 4, PM_NSM, PRIVILEGE_NORMAL, 0, &show_nsm_vlan_static_ports_cmd_imi);
  cli_install_imi (ctree, 4, PM_MSTP, PRIVILEGE_NORMAL, 0, &show_spanning_tree_stats_interface_bridge_cmd_imi);
  cli_install_imi (ctree, 5, PM_NSM, PRIVILEGE_NORMAL, 0, &no_spanning_static_cmd_imi);
  cli_install_imi (ctree, 4, PM_NSM, PRIVILEGE_NORMAL, 0, &hw_register_set_cmd_imi);
  cli_install_imi (ctree, 89, PM_NSM,  1,  0, &vlan_match_cmd_imi);
  cli_install_imi (ctree, 16, PM_NSM, PRIVILEGE_NORMAL, 0, &nsm_vlan_switchport_ingress_filter_enable_cmd_imi);
  cli_install_imi (ctree, 71, PM_MSTP, PRIVILEGE_NORMAL, 0, &no_bridge_instance_vlan_cmd_imi);
  cli_install_imi (ctree, 4, PM_NSM, PRIVILEGE_NORMAL, 0, &show_interface_switchport_bridge_cmd_imi);
  cli_install_imi (ctree, 5, PM_MSTP, PRIVILEGE_NORMAL, 0, &no_mstp_bridge_portfast_bpduguard_cmd_imi);
  cli_install_imi (ctree, 4, PM_NSM, PRIVILEGE_NORMAL, 0, &show_gvrp_statistics_port_cmd_imi);
  cli_install_imi (ctree, 4, PM_NSM, PRIVILEGE_NORMAL, 0, &show_nsm_vlan_bridge_cmd_imi);
  cli_install_imi (ctree, 5, PM_NSM,  (15),  0, &interface_arbiter_cmd_imi);
  cli_install_imi (ctree, 16, PM_NSM, PRIVILEGE_NORMAL, 0, &nsm_vlan_switchport_trunk_native_cmd_imi);
  cli_install_imi (ctree, 4, PM_NSM, PRIVILEGE_NORMAL, 0, &clear_gvrp_statistics_cmd_imi);
  cli_install_imi (ctree, 5, PM_MSTP, PRIVILEGE_NORMAL, 0, &no_mstp_spanning_tree_forceversion_cmd_imi);
  cli_install_imi (ctree, 5, PM_NSM,  1,  0, &no_debug_all_nsm_cmd_imi);
  cli_install_imi (ctree, 5, PM_NSM, PRIVILEGE_NORMAL, 0, &set_port_gmrp1_cmd_imi);
  cli_install_imi (ctree, 4, PM_MSTP, PRIVILEGE_NORMAL, 0, &show_spanning_tree_mst_instance_cmd_imi);
  cli_install_imi (ctree, 5, PM_MSTP, PRIVILEGE_NORMAL, 0, &no_mstp_spanning_forward_time_cmd_imi);
  cli_install_imi (ctree, 5, PM_MSTP, PRIVILEGE_NORMAL, 0, &mstp_spanning_tree_errdisable_timeout_enable_cmd_imi);
  cli_install_imi (ctree, 5, PM_MSTP, PRIVILEGE_NORMAL, 0, &no_mstp_spanning_hello_time_cmd_imi);
  cli_install_imi (ctree, 16, PM_MSTP, PRIVILEGE_NORMAL, 0, &mstp_spanning_tree_linktype_cmd_imi);
  cli_install_imi (ctree, 16, PM_MSTP, PRIVILEGE_NORMAL, 0, &spanning_port_inst_priority_cmd_imi);
  cli_install_imi (ctree, 16, modbmap_vor (2, &PM_NSM, &PM_NSM),  1,  0, &nsm_no_mac_access_group_cmd_imi);
  cli_install_imi (ctree, 84, modbmap_vor (2, &PM_NSM, &PM_NSM), PRIVILEGE_NORMAL, 0, &no_qos_pmapc_set_exp_cmd_imi);
  cli_install_imi (ctree, 84, modbmap_vor (2, &PM_NSM, &PM_NSM), PRIVILEGE_NORMAL, 0, &no_qos_pmapc_set_cos_cmd_imi);
  cli_install_imi (ctree, 5, PM_NSM, PRIVILEGE_NORMAL, 0, &set_gmrp_ext_filtering_cmd_imi);
  cli_install_imi (ctree, 16, PM_MSTP, PRIVILEGE_NORMAL, 0, &mstp_spanning_tree_restricted_role_cmd_imi);
  cli_install_imi (ctree, 84, modbmap_vor (2, &PM_NSM, &PM_NSM), PRIVILEGE_NORMAL, 0, &qos_pmapc_set_algorithm_cmd_imi);
  cli_install_imi (ctree, 145, PM_NSM, PRIVILEGE_NORMAL, 0, &timedelmech_config_cmd_imi);
  cli_install_imi (ctree, 4, modbmap_vor (2, &PM_NSM, &PM_NSM), PRIVILEGE_NORMAL, 0, &show_qos_access_list_cmd_imi);
  cli_install_imi (ctree, 140, PM_NSM, PRIVILEGE_NORMAL, 0, &lt_detect_timeout_cmd_imi);
  cli_install_imi (ctree, 16, PM_MSTP, PRIVILEGE_NORMAL, 0, &bridge_group_inst_path_cost_cmd_imi);
  cli_install_imi (ctree, 4, modbmap_vor (2, &PM_NSM, &PM_NSM), PRIVILEGE_NORMAL, 0, &show_policy_map_cmd_imi);
  cli_install_imi (ctree, 16, PM_NSM, PRIVILEGE_NORMAL, 0, &nsm_vlan_switchport_trunk_pn_cn_except_cmd_imi);
  cli_install_imi (ctree, 16, PM_MSTP, PRIVILEGE_NORMAL, 0, &mstp_spanning_tree_linktype_auto_cmd_imi);
  cli_install_imi (ctree, 5, modbmap_vor (2, &PM_NSM, &PM_NSM), PRIVILEGE_NORMAL, 0, &no_mac_acl_host_host_cmd_imi);
  cli_install_imi (ctree, 5, PM_NSM, PRIVILEGE_NORMAL, 0, &no_bridge_static_filter_cmd_imi);
  cli_install_imi (ctree, 16, PM_NSM,  1,  0, &no_nsm_interface_alias_cmd_imi);
  cli_install_imi (ctree, 82, modbmap_vor (2, &PM_NSM, &PM_NSM), PRIVILEGE_NORMAL, 0, &qos_cmap_match_access_group_cmd_imi);
  cli_install_imi (ctree, 4, PM_MSTP, PRIVILEGE_NORMAL, 0, &mstp_clear_spanning_tree_interface_cmd_imi);
  cli_install_imi (ctree, 5, PM_NSM, PRIVILEGE_NORMAL, 0, &no_gmrp_debug_timer_cmd_imi);
  cli_install_imi (ctree, 5, PM_NSM, PRIVILEGE_NORMAL, 0, &set_gvrp2_br_cmd_imi);
  cli_install_imi (ctree, 5, PM_NSM, PRIVILEGE_NORMAL, 0, &no_spanning_acquire_cmd_imi);
  cli_install_imi (ctree, 5, PM_MSTP, PRIVILEGE_NORMAL, 0, &no_spanning_shutdown_multiple_spanning_tree_cmd_imi);
  cli_install_imi (ctree, 5, PM_MSTP, PRIVILEGE_NORMAL, 0, &no_mstp_bridge_forward_time_cmd_imi);
  cli_install_imi (ctree, 16, PM_MSTP, PRIVILEGE_NORMAL, 0, &no_bridge_group_vlan_cmd_imi);
  cli_install_imi (ctree, 144, PM_NSM, PRIVILEGE_NORMAL, 0, &wrr_queue_weight_config_cmd_imi);
  cli_install_imi (ctree, 5, PM_NSM,  1,  0, &debug_nsm_cmd_imi);
  cli_install_imi (ctree, 5, modbmap_vor (2, &PM_NSM, &PM_NSM), PRIVILEGE_NORMAL, 0, &no_mac_access_list_address_cmd_imi);
  cli_install_imi (ctree, 16, PM_NSM, PRIVILEGE_NORMAL, 0, &nsm_vlan_classifier_activate_cmd_imi);
  cli_install_imi (ctree, 5, PM_MSTP, PRIVILEGE_NORMAL, 0, &no_debug_mstp_proto_detail_cmd_imi);
  cli_install_imi (ctree, 140, PM_NSM, PRIVILEGE_NORMAL, 0, &lb_detect_repeat_cmd_imi);
  cli_install_imi (ctree, 16, PM_MSTP, PRIVILEGE_NORMAL, 0, &mstp_port_hello_time_cmd_imi);
  cli_install_imi (ctree, 5, PM_MSTP, PRIVILEGE_NORMAL, 0, &debug_mstp_packet_rx_cmd_imi);
  cli_install_imi (ctree, 4, PM_NSM, PRIVILEGE_NORMAL, 0, &show_nsm_vlan_classifier_group_interface_cmd_imi);
  cli_install_imi (ctree, 16, PM_MSTP, PRIVILEGE_NORMAL, 0, &spanning_inst_cmd_imi);
  cli_install_imi (ctree, 4, PM_NSM, PRIVILEGE_NORMAL, 0, &clear_gvrp_statistics_bridge_cmd_imi);
  cli_install_imi (ctree, 4, PM_NSM, PRIVILEGE_NORMAL, 0, &nsm_bridge_clear_dynamic_fdb_bridge_cmd_imi);
  cli_install_imi (ctree, 84, modbmap_vor (2, &PM_NSM, &PM_NSM), PRIVILEGE_NORMAL, 0, &no_qos_pmapc_set_algorithm_cmd_imi);
  cli_install_imi (ctree, 5, PM_NSM, PRIVILEGE_NORMAL, 0, &gvrp_debug_packet_cmd_imi);
  cli_install_imi (ctree, 16, PM_MSTP, PRIVILEGE_NORMAL, 0, &spanning_vlan_path_cost_cmd_imi);
  cli_install_imi (ctree, 144, PM_NSM, PRIVILEGE_NORMAL, 0, &wrr_queue_wred_cmd_imi);
  cli_install_imi (ctree, 71, PM_MSTP, PRIVILEGE_NORMAL, 0, &bridge_revision_cmd_imi);
  cli_install_imi (ctree, 144, PM_NSM, PRIVILEGE_NORMAL, 0, &wrr_queue_egress_car_cmd_imi);
  cli_install_imi (ctree, 5, PM_MSTP, PRIVILEGE_NORMAL, 0, &no_mstp_spanning_tree_errdisable_timeout_enable_cmd_imi);
  cli_install_imi (ctree, 5, PM_MSTP, PRIVILEGE_NORMAL, 0, &no_spanning_tree_ageing_time_cmd_imi);
  cli_install_imi (ctree, 16, PM_NSM,  1,  0, &nsm_if_duplex_cmd_imi);
  cli_install_imi (ctree, 5, PM_MSTP, PRIVILEGE_NORMAL, 0, &spanning_shutdown_multiple_spanning_tree_cmd_imi);
  cli_install_imi (ctree, 4, PM_NSM,  (15),  0, &no_debug_hal_cmd_imi);
  cli_install_imi (ctree, 84, modbmap_vor (2, &PM_NSM, &PM_NSM), PRIVILEGE_NORMAL, 0, &qos_pmapc_police_rate_cmd_imi);
  cli_install_imi (ctree, 4, PM_NSM,  1,  0, &undebug_nsm_packet_cmd_imi);
  cli_install_imi (ctree, 136, PM_NSM, PRIVILEGE_NORMAL, 0, &vpn_add_vport_mac_cmd_imi);
  cli_install_imi (ctree, 140, PM_NSM, PRIVILEGE_NORMAL, 0, &no_meg_id_cmd_imi);
  cli_install_imi (ctree, 5, PM_NSM, PRIVILEGE_NORMAL, 0, &set_gmrp_instance_disable_cmd_imi);
  cli_install_imi (ctree, 146, PM_NSM, PRIVILEGE_NORMAL, 0, &delaymech_config_cmd_imi);
  cli_install_imi (ctree, 16, PM_NSM, PRIVILEGE_NORMAL, 0, &switchport_cmd_imi);
  cli_install_imi (ctree, 140, PM_NSM, PRIVILEGE_NORMAL, 0, &dm_way_select_repeat_cmd_imi);
  cli_install_imi (ctree, 140, PM_NSM, PRIVILEGE_NORMAL, 0, &pdc_alarm_threshold_cmd_imi);
  cli_install_imi (ctree, 5, PM_NSM,  1,  0, &debug_nsm_ha_cmd_imi);
  cli_install_imi (ctree, 5, PM_NSM, PRIVILEGE_NORMAL, 0, &set_gmrp_ext_filtering2_br_cmd_imi);
  cli_install_imi (ctree, 5, PM_NSM, PRIVILEGE_NORMAL, 0, &no_gvrp_debug_timer_cmd_imi);
  cli_install_imi (ctree, 5, PM_MSTP, PRIVILEGE_NORMAL, 0, &no_spanning_inst_priority_cmd_imi);
  cli_install_imi (ctree, 140, PM_NSM, PRIVILEGE_NORMAL, 0, &lt_detect_ttl_cmd_imi);
  cli_install_imi (ctree, 16, PM_MSTP, PRIVILEGE_NORMAL, 0, &show_spanning_tree_mst_config_cmd_imi);
  cli_install_imi (ctree, 4, PM_NSM,  1,  0, &no_debug_nsm_events_cmd_imi);
  cli_install_imi (ctree, 4, PM_NSM, PRIVILEGE_NORMAL, 0, &clear_gmrp_statistics1_cmd_imi);
  cli_install_imi (ctree, 71, PM_MSTP, PRIVILEGE_NORMAL, 0, &spanning_revision_cmd_imi);
  cli_install_imi (ctree, 5, PM_NSM, PRIVILEGE_NORMAL, 0, &set_gvrp1_br_cmd_imi);
  cli_install_imi (ctree, 16, PM_NSM, PRIVILEGE_NORMAL, 0, &nsm_vlan_switchport_trunk_pn_cn_add_cmd_imi);
  cli_install_imi (ctree, 140, PM_NSM, PRIVILEGE_NORMAL, 0, &lb_detect_size_cmd_imi);
  cli_install_imi (ctree, 5, PM_NSM, PRIVILEGE_NORMAL, 0, &gvrp_debug_imish_cmd_imi);
  cli_install_imi (ctree, 4, PM_MSTP, PRIVILEGE_NORMAL, 0, &show_spanning_tree_rpvst_detail_cmd_imi);
  cli_install_imi (ctree, 5, PM_NSM, PRIVILEGE_NORMAL, 0, &set_gvrp_timer1_cmd_imi);
  cli_install_imi (ctree, 4, PM_NSM,  (15),  0, &show_nsm_imishent_cmd_imi);
  cli_install_imi (ctree, 82, modbmap_vor (2, &PM_NSM, &PM_NSM), PRIVILEGE_NORMAL, 0, &no_qos_cmap_match_access_group_cmd_imi);
  cli_install_imi (ctree, 16, PM_MSTP, PRIVILEGE_NORMAL, 0, &mstp_spanning_tree_inst_restricted_role_cmd_imi);
  cli_install_imi (ctree, 5, PM_NSM, PRIVILEGE_NORMAL, 0, &nsm_bridge_protocol_mstp_cmd_imi);
  cli_install_imi (ctree, 5, PM_MSTP, PRIVILEGE_NORMAL, 0, &no_mstp_bridge_priority_cmd_imi);
  cli_install_imi (ctree, 16, PM_MSTP, PRIVILEGE_NORMAL, 0, &no_bridge_group_inst_path_cost_cmd_imi);
  cli_install_imi (ctree, 67, PM_NSM, PRIVILEGE_NORMAL, 0, &nsm_vlan_no_name_cmd_imi);
  cli_install_imi (ctree, 16, PM_MSTP, PRIVILEGE_NORMAL, 0, &no_mstp_spanning_tree_autoedge_cmd_imi);
  cli_install_imi (ctree, 4, PM_NSM, PRIVILEGE_NORMAL, 0, &show_gvrp_machine_cmd_imi);
  cli_install_imi (ctree, 4, PM_NSM,  (15),  0, &debug_hal_cmd_imi);
  cli_install_imi (ctree, 16, PM_NSM, PRIVILEGE_NORMAL, 0, &no_nsm_vlan_switchport_trunk_native_cmd_imi);
  cli_install_imi (ctree, 4, PM_NSM,  (15),  0, &debug_hal_events_cmd_imi);
  cli_install_imi (ctree, 4, PM_MSTP, PRIVILEGE_NORMAL, 0, &show_spanning_tree_rpvst_vlan_interface_cmd_imi);
  cli_install_imi (ctree, 16, PM_MSTP, PRIVILEGE_NORMAL, 0, &mstp_bridge_group_path_cost_cmd_imi);
  cli_install_imi (ctree, 82, modbmap_vor (2, &PM_NSM, &PM_NSM), PRIVILEGE_NORMAL, 0, &no_qos_cmap_match_traffic_type_cmd_imi);
  cli_install_imi (ctree, 5, PM_MSTP, PRIVILEGE_NORMAL, 0, &mstp_spanning_transmit_hold_count_cmd_imi);
  cli_install_imi (ctree, 140, PM_NSM, PRIVILEGE_NORMAL, 0, &lb_detect_enable_cmd_imi);
  cli_install_imi (ctree, 4, PM_NSM,  1,  0, &undebug_nsm_ha_all_cmd_imi);
  cli_install_imi (ctree, 16, PM_MSTP, PRIVILEGE_NORMAL, 0, &no_bridge_group_inst_cmd_imi);
  cli_install_imi (ctree, 5, PM_NSM, PRIVILEGE_NORMAL, 0, &set_port_gmrp2_cmd_imi);
  cli_install_imi (ctree, 4, PM_NSM,  (15),  0, &no_debug_hal_events_cmd_imi);
  cli_install_imi (ctree, 16, PM_MSTP, PRIVILEGE_NORMAL, 0, &no_mstp_spanning_tree_portfast_cmd_imi);
  cli_install_imi (ctree, 16, PM_NSM, PRIVILEGE_NORMAL, 0, &flowcontrol_receive_on_cmd_imi);
  cli_install_imi (ctree, 82, modbmap_vor (2, &PM_NSM, &PM_NSM), PRIVILEGE_NORMAL, 0, &no_qos_cmap_match_l4_port_cmd_imi);
  cli_install_imi (ctree, 5, PM_MSTP, PRIVILEGE_NORMAL, 0, &mstp_bridge_transmit_hold_count_cmd_imi);
  cli_install_imi (ctree, 4, PM_NSM,  1,  0, &fpga_read_cmd_imi);
  cli_install_imi (ctree, 4, PM_NSM, PRIVILEGE_NORMAL, 0, &show_gmrp_configuration_cmd_imi);
  cli_install_imi (ctree, 71, PM_MSTP, PRIVILEGE_NORMAL, 0, &spanning_region_cmd_imi);
  cli_install_imi (ctree, 67, PM_NSM, PRIVILEGE_NORMAL, 0, &no_nsm_br_vlan_no_name_cmd_imi);
  cli_install_imi (ctree, 4, PM_NSM, PRIVILEGE_NORMAL, 0, &show_nsm_vlan_spanningtree_bridge_cmd_imi);
  cli_install_imi (ctree, 16, modbmap_vor (2, &PM_NSM, &PM_NSM), PRIVILEGE_NORMAL, 0, &no_wrr_queue_cos_map_cmd_imi);
  cli_install_imi (ctree, 16, PM_MSTP, PRIVILEGE_NORMAL, 0, &no_mstp_spanning_tree_portfast_bpduguard_cmd_imi);
  cli_install_imi (ctree, 4, PM_NSM, PRIVILEGE_NORMAL, 0, &show_gvrp_configuration_cmd_imi);
  cli_install_imi (ctree, 84, modbmap_vor (2, &PM_NSM, &PM_NSM), PRIVILEGE_NORMAL, 0, &qos_pmapc_police_aggregate_cmd_imi);
  cli_install_imi (ctree, 16, PM_NSM, PRIVILEGE_NORMAL, 0, &nsm_vlan_switchport_pvid_cmd_imi);
  cli_install_imi (ctree, 5, PM_MSTP, PRIVILEGE_NORMAL, 0, &no_mstp_spanning_transmit_hold_count_cmd_imi);
  cli_install_imi (ctree, 84, modbmap_vor (2, &PM_NSM, &PM_NSM), PRIVILEGE_NORMAL, 0, &qos_pmapc_set_ip_precedence_cmd_imi);
  cli_install_imi (ctree, 5, PM_MSTP, PRIVILEGE_NORMAL, 0, &debug_mstp_proto_cmd_imi);
  cli_install_imi (ctree, 16, PM_MSTP, PRIVILEGE_NORMAL, 0, &mstp_spanning_tree_vlan_restricted_role_cmd_imi);
  cli_install_imi (ctree, 136, PM_NSM, PRIVILEGE_NORMAL, 0, &vpn_add_mutigrp_cmd_imi);
  cli_install_imi (ctree, 140, PM_NSM, PRIVILEGE_NORMAL, 0, &lb_detect_timeout_cmd_imi);
  cli_install_imi (ctree, 140, PM_NSM, PRIVILEGE_NORMAL, 0, &lm_proactive_detect_cmd_imi);
  cli_install_imi (ctree, 5, PM_MSTP, PRIVILEGE_NORMAL, 0, &debug_mstp_imish_cmd_imi);
  cli_install_imi (ctree, 5, PM_NSM, PRIVILEGE_NORMAL, 0, &no_gvrp_debug_packet_cmd_imi);
  cli_install_imi (ctree, 16, PM_NSM,  1,  0, &no_nsm_if_duplex_cmd_imi);
  cli_install_imi (ctree, 4, PM_NSM,  1,  0, &debug_nsm_ha_cmd_imi);
  cli_install_imi (ctree, 16, PM_MSTP, PRIVILEGE_NORMAL, 0, &show_spanning_tree_mst_detail_cmd_imi);
  cli_install_imi (ctree, 5, PM_NSM, PRIVILEGE_NORMAL, 0, &set_gmrp_timer2_cmd_imi);
  cli_install_imi (ctree, 5, PM_MSTP, PRIVILEGE_NORMAL, 0, &bridge_shutdown_multiple_spanning_tree_cmd_imi);
  cli_install_imi (ctree, 5, PM_NSM, PRIVILEGE_NORMAL, 0, &no_gmrp_debug_event_cmd_imi);
  cli_install_imi (ctree, 82, modbmap_vor (2, &PM_NSM, &PM_NSM), PRIVILEGE_NORMAL, 0, &qos_cmap_match_vlan_cmd_imi);
  cli_install_imi (ctree, 136, PM_NSM, PRIVILEGE_NORMAL, 0, &vpn_config_cmd_imi);
  cli_install_imi (ctree, 5, PM_NSM, PRIVILEGE_NORMAL, 0, &gmrp_debug_timer_cmd_imi);
  cli_install_imi (ctree, 4, modbmap_vor (2, &PM_NSM, &PM_NSM), PRIVILEGE_NORMAL, 0, &show_mac_access_grp_cmd_imi);
  cli_install_imi (ctree, 5, modbmap_vor (2, &PM_NSM, &PM_NSM), PRIVILEGE_NORMAL, 0, &ip_access_list_extended_mask_host_cmd_imi);
  cli_install_imi (ctree, 5, PM_MSTP, PRIVILEGE_NORMAL, 0, &debug_mstp_timer_cmd_imi);
  cli_install_imi (ctree, 139, PM_NSM, PRIVILEGE_NORMAL, 0, &config_lsp_mip_or_tail_entity_cmd_imi);
  cli_install_imi (ctree, 4, PM_NSM, PRIVILEGE_NORMAL, 0, &nsm_bridge_clear_fdb_vlan_port_cmd_imi);
  cli_install_imi (ctree, 5, PM_MSTP, PRIVILEGE_NORMAL, 0, &spanning_max_hops_cmd_imi);
  cli_install_imi (ctree, 71, PM_MSTP, PRIVILEGE_NORMAL, 0, &no_bridge_region_cmd_imi);
  cli_install_imi (ctree, 5, PM_MSTP, PRIVILEGE_NORMAL, 0, &bridge_inst_priority_cmd_imi);
  cli_install_imi (ctree, 89, PM_NSM,  1,  0, &vlan_no_match_cmd_imi);
  cli_install_imi (ctree, 4, PM_NSM, PRIVILEGE_NORMAL, 0, &nsm_bridge_clear_br_fdb_cmd_imi);
  cli_install_imi (ctree, 67, PM_NSM, PRIVILEGE_NORMAL, 0, &nsm_br_vlan_mtu_cmd_imi);
  cli_install_imi (ctree, 16, PM_MSTP, PRIVILEGE_NORMAL, 0, &bridge_group_vlan_path_cost_cmd_imi);
  cli_install_imi (ctree, 83, modbmap_vor (2, &PM_NSM, &PM_NSM), PRIVILEGE_NORMAL, 0, &no_qos_pmap_class_cmd_imi);
  cli_install_imi (ctree, 16, modbmap_vor (2, &PM_NSM, &PM_NSM),  1,  0, &nsm_mac_access_group_cmd_imi);
  cli_install_imi (ctree, 5, PM_MSTP, PRIVILEGE_NORMAL, 0, &no_bridge_vlan_priority_cmd_imi);
  cli_install_imi (ctree, 5, modbmap_vor (2, &PM_NSM, &PM_NSM), PRIVILEGE_NORMAL, 0, &no_ip_access_list_extended_cmd_imi);
  cli_install_imi (ctree, 5, PM_NSM, PRIVILEGE_NORMAL, 0, &set_gmrp_br_instance_enable_cmd_imi);
  cli_install_imi (ctree, 4, modbmap_vor (2, &PM_NSM, &PM_NSM), PRIVILEGE_NORMAL, 0, &show_policy_map_name_cmd_imi);
  cli_install_imi (ctree, 5, PM_MSTP, PRIVILEGE_NORMAL, 0, &no_mstp_spanning_tree_pathcost_method_cmd_imi);
  cli_install_imi (ctree, 84, modbmap_vor (2, &PM_NSM, &PM_NSM), PRIVILEGE_NORMAL, 0, &qos_pmapc_set_cos_cmd_imi);
  cli_install_imi (ctree, 5, modbmap_vor (2, &PM_NSM, &PM_NSM), PRIVILEGE_NORMAL, 0, &no_mac_access_list_host_host_cmd_imi);
  cli_install_imi (ctree, 4, modbmap_vor (2, &PM_NSM, &PM_NSM), PRIVILEGE_NORMAL, 0, &show_qos_access_list_name_cmd_imi);
  cli_install_imi (ctree, 5, PM_MSTP, PRIVILEGE_NORMAL, 0, &mstp_spanning_tree_errdisable_timeout_enable_cmd1_imi);
  cli_install_imi (ctree, 16, PM_NSM, PRIVILEGE_NORMAL, 0, &no_switchport_ratelimit_cmd_imi);
  cli_install_imi (ctree, 4, PM_NSM, PRIVILEGE_NORMAL, 0, &show_debugging_gvrp_cmd_imi);
  cli_install_imi (ctree, 4, PM_FM,  (15),  (1 << 11), &fm_show_faults_cmd_imi);
  cli_install_imi (ctree, 5, PM_MSTP, PRIVILEGE_NORMAL, 0, &mstp_bridge_spanning_tree_forceversion_cmd_imi);
  cli_install_imi (ctree, 16, PM_NSM, PRIVILEGE_NORMAL, 0, &switchport_ratelimit_cmd_imi);
  cli_install_imi (ctree, 16, PM_NSM, PRIVILEGE_NORMAL, 0, &nsm_bridge_group_span_cmd_imi);
  cli_install_imi (ctree, 4, PM_MSTP, PRIVILEGE_NORMAL, 0, &no_debug_mstp_imish_cmd_imi);
  cli_install_imi (ctree, 5, modbmap_vor (2, &PM_NSM, &PM_NSM), PRIVILEGE_NORMAL, 0, &ip_access_list_extended_any_any_cmd_imi);
  cli_install_imi (ctree, 5, modbmap_vor (2, &PM_NSM, &PM_NSM), PRIVILEGE_NORMAL, 0, &no_ip_access_list_extended_any_mask_cmd_imi);
  cli_install_imi (ctree, 4, PM_MSTP, PRIVILEGE_NORMAL, 0, &no_debug_mstp_packet_rx_cmd_imi);
  cli_install_imi (ctree, 5, modbmap_vor (2, &PM_NSM, &PM_NSM), PRIVILEGE_NORMAL, 0, &no_mac_acl_host_any_cmd_imi);
  cli_install_imi (ctree, 5, PM_NSM, PRIVILEGE_NORMAL, 0, &mac_prio_override_cmd_imi);
  cli_install_imi (ctree, 4, PM_MSTP, PRIVILEGE_NORMAL, 0, &mstp_clear_spanning_tree_bridge_cmd_imi);
  cli_install_imi (ctree, 67, PM_NSM, PRIVILEGE_NORMAL, 0, &nsm_br_vlan_no_name_cmd_imi);
  cli_install_imi (ctree, 5, PM_MSTP, PRIVILEGE_NORMAL, 0, &debug_mstp_timer_detail_cmd_imi);
  cli_install_imi (ctree, 5, modbmap_vor (2, &PM_NSM, &PM_NSM), PRIVILEGE_NORMAL, 0, &no_ip_access_list_extended_any_host_cmd_imi);
  cli_install_imi (ctree, 4, PM_NSM, PRIVILEGE_NORMAL, 0, &hw_register_get_cmd_imi);
  cli_install_imi (ctree, 4, PM_NSM, PRIVILEGE_NORMAL, 0, &show_nsm_vlan_classifier_rule_cmd_imi);
  cli_install_imi (ctree, 5, PM_MSTP, PRIVILEGE_NORMAL, 0, &mstp_spanning_cisco_interop_cmd_imi);
  cli_install_imi (ctree, 16, modbmap_vor (2, &PM_NSM, &PM_NSM), PRIVILEGE_NORMAL, 0, &no_service_policy_input_cmd_imi);
  cli_install_imi (ctree, 5, PM_MSTP, PRIVILEGE_NORMAL, 0, &spanning_inst_priority_cmd_imi);
  cli_install_imi (ctree, 5, PM_NSM,  1,  0, &no_debug_nsm_events_cmd_imi);
  cli_install_imi (ctree, 140, PM_NSM, PRIVILEGE_NORMAL, 0, &cc_detect_cmd_imi);
  cli_install_imi (ctree, 5, PM_NSM, PRIVILEGE_NORMAL, 0, &spanning_add_static_cmd_imi);
  cli_install_imi (ctree, 141, modbmap_vor (2, &PM_NSM, &PM_NSM), PRIVILEGE_NORMAL, 0, &mac_in_ppp_section_cmd_imi);
  cli_install_imi (ctree, 140, PM_NSM, PRIVILEGE_NORMAL, 0, &lb_detect_ttl_cmd_imi);
  cli_install_imi (ctree, 141, modbmap_vor (2, &PM_NSM, &PM_NSM), PRIVILEGE_NORMAL, 0, &no_sec_id_cmd_imi);
  cli_install_imi (ctree, 16, PM_NSM, PRIVILEGE_NORMAL, 0, &mirror_interface_transmit_cmd_imi);
  cli_install_imi (ctree, 5, PM_NSM, PRIVILEGE_NORMAL, 0, &nsm_bridge_protocol_ieee_vlan_cmd_imi);
  cli_install_imi (ctree, 5, PM_MSTP, PRIVILEGE_NORMAL, 0, &mstp_bridge_priority_cmd_imi);
  cli_install_imi (ctree, 4, PM_NSM,  (15),  0, &undebug_hal_cmd_imi);
  cli_install_imi (ctree, 16, PM_NSM, PRIVILEGE_NORMAL, 0, &nsm_vlan_switchport_allowed_vlan_none_cmd_imi);
  cli_install_imi (ctree, 16, PM_NSM, PRIVILEGE_NORMAL, 0, &nsm_vlan_switchport_trunk_pn_cn_all_cmd_imi);
  cli_install_imi (ctree, 16, PM_MSTP, PRIVILEGE_NORMAL, 0, &no_mstp_spanning_tree_linktype_cmd_imi);
  cli_install_imi (ctree, 4, PM_NSM,  1,  0, &undebug_nsm_ha_cmd_imi);
  cli_install_imi (ctree, 16, PM_MSTP, PRIVILEGE_NORMAL, 0, &mstp_spanning_tree_vlan_restricted_tcn_cmd_imi);
  cli_install_imi (ctree, 4, PM_MSTP, PRIVILEGE_NORMAL, 0, &debug_mstp_imish_cmd_imi);
  cli_install_imi (ctree, 16, PM_NSM, PRIVILEGE_NORMAL, 0, &flowcontrol_send_off_cmd_imi);
  cli_install_imi (ctree, 4, PM_NSM, PRIVILEGE_NORMAL, 0, &show_gvrp_statistics_cmd_imi);
  cli_install_imi (ctree, 16, modbmap_vor (2, &PM_NSM, &PM_NSM), PRIVILEGE_NORMAL, 0, &mls_qos_dscp_mutation_cmd_imi);
  cli_install_imi (ctree, 16, PM_MSTP, PRIVILEGE_NORMAL, 0, &no_spanning_vlan_path_cost_cmd_imi);
  cli_install_imi (ctree, 5, PM_NSM, PRIVILEGE_NORMAL, 0, &no_tun_id_cmd_imi);
  cli_install_imi (ctree, 5, PM_NSM, PRIVILEGE_NORMAL, 0, &set_port_gmrp_enable_vlan_cmd_imi);
  cli_install_imi (ctree, 147, PM_NSM, PRIVILEGE_NORMAL, 0, &lsp_group_cfg_cmd_imi);
  cli_install_imi (ctree, 5, modbmap_vor (2, &PM_NSM, &PM_NSM), PRIVILEGE_NORMAL, 0, &mac_access_list_address_cmd_imi);
  cli_install_imi (ctree, 5, PM_MSTP, PRIVILEGE_NORMAL, 0, &no_mstp_span_portfast_bpduguard_cmd_imi);
  cli_install_imi (ctree, 5, PM_NSM,  (15),  0, &debug_hal_events_cmd_imi);
  cli_install_imi (ctree, 136, PM_NSM, PRIVILEGE_NORMAL, 0, &vpn_rmv_mutigrp_vport_cmd_imi);
  cli_install_imi (ctree, 5, PM_NSM, PRIVILEGE_NORMAL, 0, &set_gvrp2_cmd_imi);
  cli_install_imi (ctree, 82, modbmap_vor (2, &PM_NSM, &PM_NSM), PRIVILEGE_NORMAL, 0, &qos_cmap_match_vlan_range_cmd_imi);
  cli_install_imi (ctree, 110, PM_MSTP, PRIVILEGE_NORMAL, 0, &bridge_vlan_cmd_imi);
  cli_install_imi (ctree, 136, PM_NSM, PRIVILEGE_NORMAL, 0, &vpn_rmv_vport_mac_cmd_imi);
  cli_install_imi (ctree, 4, PM_NSM, PRIVILEGE_NORMAL, 0, &show_gvrp_timer_cmd_imi);
  cli_install_imi (ctree, 5, PM_MSTP, PRIVILEGE_NORMAL, 0, &spanning_vlan_priority_cmd_imi);
  cli_install_imi (ctree, 4, PM_MSTP, PRIVILEGE_NORMAL, 0, &show_spanning_tree_stats_bridge_cmd_imi);
  cli_install_imi (ctree, 5, PM_NSM, PRIVILEGE_NORMAL, 0, &set_port_gvrp2_cmd_imi);
  cli_install_imi (ctree, 139, PM_NSM, PRIVILEGE_NORMAL, 0, &tun_config_cmd_imi);
  cli_install_imi (ctree, 5, PM_NSM,  (15),  0, &debug_hal_cmd_imi);
  cli_install_imi (ctree, 16, PM_NSM, PRIVILEGE_NORMAL, 0, &no_mirror_interface_transmit_cmd_imi);
  cli_install_imi (ctree, 140, PM_NSM, PRIVILEGE_NORMAL, 0, &dm_way_select_enable_cmd_imi);
  cli_install_imi (ctree, 5, PM_NSM, PRIVILEGE_NORMAL, 0, &set_gmrp_timer1_cmd_imi);
  cli_install_imi (ctree, 140, PM_NSM, PRIVILEGE_NORMAL, 0, &lm_on_demand_detect_repeat_cmd_imi);
  cli_install_imi (ctree, 5, PM_MSTP, PRIVILEGE_NORMAL, 0, &no_mstp_bridge_spanning_tree_forceversion_cmd_imi);
  cli_install_imi (ctree, 5, PM_MSTP, PRIVILEGE_NORMAL, 0, &no_bridge_inst_priority_cmd_imi);
  cli_install_imi (ctree, 4, PM_MSTP, PRIVILEGE_NORMAL, 0, &debug_mstp_timer_cmd_imi);
  cli_install_imi (ctree, 16, PM_NSM, PRIVILEGE_NORMAL, 0, &flowcontrol_both_on_cmd_imi);
  cli_install_imi (ctree, 4, PM_NSM, PRIVILEGE_NORMAL, 0, &clear_gvrp_statistics_port_cmd_imi);
  cli_install_imi (ctree, 4, PM_MSTP, PRIVILEGE_NORMAL, 0, &show_spanning_tree_cmd_imi);
  cli_install_imi (ctree, 16, PM_NSM, PRIVILEGE_NORMAL, 0, &nsm_vlan_switchport_mode_cmd_imi);
  cli_install_imi (ctree, 16, PM_NSM, PRIVILEGE_NORMAL, 0, &no_mirror_interface_receive_cmd_imi);
  cli_install_imi (ctree, 5, PM_NSM, PRIVILEGE_NORMAL, 0, &set_gmrp2_br_cmd_imi);
  cli_install_imi (ctree, 16, PM_NSM, PRIVILEGE_NORMAL, 0, &no_nsm_bridge_group_cmd_imi);
  cli_install_imi (ctree, 135, PM_NSM, PRIVILEGE_NORMAL, 0, &no_meg_index_cmd_imi);
  cli_install_imi (ctree, 67, PM_NSM, PRIVILEGE_NORMAL, 0, &nsm_vlan_name_cmd_imi);
  cli_install_imi (ctree, 5, PM_NSM, PRIVILEGE_NORMAL, 0, &set_gmrp_fwd_all1_cmd_imi);
  cli_install_imi (ctree, 16, modbmap_vor (2, &PM_NSM, &PM_NSM), PRIVILEGE_NORMAL, 0, &wrr_queue_cos_map_cmd_imi);
  cli_install_imi (ctree, 16, PM_NSM, PRIVILEGE_NORMAL, 0, &flowcontrol_send_on_cmd_imi);
  cli_install_imi (ctree, 4, PM_NSM, PRIVILEGE_NORMAL, 0, &show_gmrp_machine_cmd_imi);
  cli_install_imi (ctree, 4, PM_NSM,  1,  0, &no_debug_nsm_kernel_cmd_imi);
  cli_install_imi (ctree, 5, PM_MSTP, PRIVILEGE_NORMAL, 0, &mstp_bridge_portfast_bpduguard_cmd_imi);
  cli_install_imi (ctree, 5, PM_MSTP, PRIVILEGE_NORMAL, 0, &no_bridge_multiple_spanning_tree_cmd_imi);
  cli_install_imi (ctree, 16, PM_MSTP, PRIVILEGE_NORMAL, 0, &no_mstp_spanning_tree_portfast_bpdufilter_cmd_imi);
  cli_install_imi (ctree, 71, PM_MSTP, PRIVILEGE_NORMAL, 0, &no_spanning_instance_vlan_cmd_imi);
  cli_install_imi (ctree, 5, PM_MSTP, PRIVILEGE_NORMAL, 0, &no_mstp_spanning_tree_errdisable_timeout_interval_cmd_imi);
  cli_install_imi (ctree, 5, modbmap_vor (2, &PM_NSM, &PM_NSM), PRIVILEGE_NORMAL, 0, &ip_access_list_extended_mask_any_cmd_imi);
  cli_install_imi (ctree, 16, PM_NSM, PRIVILEGE_NORMAL, 0, &no_flowcontrol_cmd_imi);
  cli_install_imi (ctree, 4, PM_NSM, PRIVILEGE_NORMAL, 0, &show_mirror_interface_cmd_imi);
  cli_install_imi (ctree, 16, modbmap_vor (2, &PM_NSM, &PM_NSM), PRIVILEGE_NORMAL, 0, &no_wrr_queue_threshold_dscp_map_cmd_imi);
  cli_install_imi (ctree, 135, PM_NSM, PRIVILEGE_NORMAL, 0, &pw_car_cmd_imi);
  cli_install_imi (ctree, 139, PM_NSM, PRIVILEGE_NORMAL, 0, &no_meg_index_cmd_imi);
  cli_install_imi (ctree, 84, modbmap_vor (2, &PM_NSM, &PM_NSM), PRIVILEGE_NORMAL, 0, &qos_pmapc_set_exp_cmd_imi);
  cli_install_imi (ctree, 5, PM_NSM, PRIVILEGE_NORMAL, 0, &nsm_bridge_protocol_rstp_vlan_cmd_imi);
  cli_install_imi (ctree, 140, PM_NSM, PRIVILEGE_NORMAL, 0, &lt_detect_size_cmd_imi);
  cli_install_imi (ctree, 136, PM_NSM, PRIVILEGE_NORMAL, 0, &vpn_rmv_vport_cmd_imi);
  cli_install_imi (ctree, 5, PM_NSM, PRIVILEGE_NORMAL, 0, &bridge_acquire_cmd_imi);
  cli_install_imi (ctree, 146, PM_NSM, PRIVILEGE_NORMAL, 0, &p2p_request_interval_config_cmd_imi);
  cli_install_imi (ctree, 5, PM_NSM, PRIVILEGE_NORMAL, 0, &show_lspgrp_status_cmd_imi);
  cli_install_imi (ctree, 5, PM_MSTP, PRIVILEGE_NORMAL, 0, &mstp_span_portfast_bpduguard_cmd_imi);
  cli_install_imi (ctree, 16, PM_MSTP, PRIVILEGE_NORMAL, 0, &no_mstp_spanning_tree_vlan_restricted_role_cmd_imi);
  cli_install_imi (ctree, 143, PM_NSM, PRIVILEGE_NORMAL, 0, &lsp_cfg_up_cmd_imi);
  cli_install_imi (ctree, 135, PM_NSM, PRIVILEGE_NORMAL, 0, &config_pw_entity_cmd_imi);
  cli_install_imi (ctree, 5, PM_NSM, PRIVILEGE_NORMAL, 0, &bridge_add_static_filter_cmd_imi);
  cli_install_imi (ctree, 16, PM_NSM,  1,  0, &no_nsm_if_shutdown_cmd_imi);
  cli_install_imi (ctree, 140, PM_NSM, PRIVILEGE_NORMAL, 0, &pd_alarm_threshold_cmd_imi);
  cli_install_imi (ctree, 16, PM_MSTP, PRIVILEGE_NORMAL, 0, &spanning_port_vlan_priority_cmd_imi);
  cli_install_imi (ctree, 5, PM_NSM, PRIVILEGE_NORMAL, 0, &set_gvrp_br_dyn_vlan_disable_cmd_imi);
  cli_install_imi (ctree, 4, PM_MSTP, PRIVILEGE_NORMAL, 0, &no_debug_mstp_timer_cmd_imi);
  cli_install_imi (ctree, 5, PM_NSM, PRIVILEGE_NORMAL, 0, &no_nsm_bridge_protocol_cmd_imi);
  cli_install_imi (ctree, 5, PM_NSM, PRIVILEGE_NORMAL, 0, &nsm_vlan_filter_cmd_imi);
  cli_install_imi (ctree, 5, PM_MSTP, PRIVILEGE_NORMAL, 0, &debug_mstp_packet_tx_cmd_imi);
  cli_install_imi (ctree, 5, PM_MSTP, PRIVILEGE_NORMAL, 0, &no_debug_mstp_packet_rx_cmd_imi);
  cli_install_imi (ctree, 5, PM_NSM, PRIVILEGE_NORMAL, 0, &set_gmrp2_cmd_imi);
  cli_install_imi (ctree, 140, PM_NSM, PRIVILEGE_NORMAL, 0, &lt_detect_enable_cmd_imi);
  cli_install_imi (ctree, 67, PM_NSM, PRIVILEGE_NORMAL, 0, &no_nsm_br_vlan_mtu_cmd_imi);
  cli_install_imi (ctree, 138, PM_NSM, PRIVILEGE_NORMAL, 0, &ac_config_cmd_imi);
  cli_install_imi (ctree, 82, modbmap_vor (2, &PM_NSM, &PM_NSM), PRIVILEGE_NORMAL, 0, &qos_cmap_match_ip_dscp_cmd_imi);
  cli_install_imi (ctree, 16, PM_MSTP, PRIVILEGE_NORMAL, 0, &no_bridge_group_vlan_priority_cmd_imi);
  cli_install_imi (ctree, 5, PM_NSM, PRIVILEGE_NORMAL, 0, &set_gmrp1_cmd_imi);
  cli_install_imi (ctree, 16, PM_MSTP, PRIVILEGE_NORMAL, 0, &mstp_bridge_group_priority_cmd_imi);
  cli_install_imi (ctree, 140, PM_NSM, PRIVILEGE_NORMAL, 0, &meg_id_cmd_imi);
  cli_install_imi (ctree, 5, PM_NSM, PRIVILEGE_NORMAL, 0, &set_gvrp1_cmd_imi);
  cli_install_imi (ctree, 4, PM_NSM,  1,  0, &debug_nsm_packet_cmd_imi);
  cli_install_imi (ctree, 5, modbmap_vor (2, &PM_NSM, &PM_NSM), PRIVILEGE_NORMAL, 0, &no_sec_id_cmd_imi);
  cli_install_imi (ctree, 16, PM_NSM, PRIVILEGE_NORMAL, 0, &nsm_vlan_switchport_hybrid_allowed_all_cmd_imi);
  cli_install_imi (ctree, 4, PM_MSTP, PRIVILEGE_NORMAL, 0, &debug_mstp_timer_detail_cmd_imi);
  cli_install_imi (ctree, 16, PM_MSTP, PRIVILEGE_NORMAL, 0, &no_mstp_bridge_group_path_cost_cmd_imi);
  cli_install_imi (ctree, 5, PM_NSM,  (15),  0, &fpga_write_cmd_imi);
  cli_install_imi (ctree, 16, PM_NSM, PRIVILEGE_NORMAL, 0, &switchport_ratelimit_cmd1_imi);
  cli_install_imi (ctree, 5, PM_NSM, PRIVILEGE_NORMAL, 0, &set_gmrp_registration3_cmd_imi);
  cli_install_imi (ctree, 5, PM_MSTP, PRIVILEGE_NORMAL, 0, &mstp_bridge_portfast_bpdufilter_cmd_imi);
  cli_install_imi (ctree, 5, PM_NSM, PRIVILEGE_NORMAL, 0, &set_gmrp_instance_enable_cmd_imi);
  cli_install_imi (ctree, 5, PM_NSM, PRIVILEGE_NORMAL, 0, &spanning_tree_mode_cmd_imi);
  cli_install_imi (ctree, 5, PM_MSTP, PRIVILEGE_NORMAL, 0, &mstp_spanning_hello_time_cmd_imi);
  cli_install_imi (ctree, 16, PM_MSTP, PRIVILEGE_NORMAL, 0, &spanning_vlanif_cmd_imi);
  cli_install_imi (ctree, 16, PM_MSTP, PRIVILEGE_NORMAL, 0, &mstp_spanning_tree_portfast_cmd_imi);
  cli_install_imi (ctree, 5, PM_NSM, PRIVILEGE_NORMAL, 0, &set_gvrp_app_state_active_cmd_imi);
  cli_install_imi (ctree, 4, modbmap_vor (2, &PM_NSM, &PM_NSM), PRIVILEGE_NORMAL, 0, &show_mac_acl_name_cmd_imi);
  cli_install_imi (ctree, 4, PM_NSM, PRIVILEGE_NORMAL, 0, &show_mirror_cmd_imi);
  cli_install_imi (ctree, 4, PM_NSM, PRIVILEGE_NORMAL, 0, &show_debugging_gmrp_cmd_imi);
  cli_install_imi (ctree, 16, PM_NSM, PRIVILEGE_NORMAL, 0, &no_nsm_vlan_switchport_trunk_vlan_cmd_imi);
  cli_install_imi (ctree, 16, modbmap_vor (2, &PM_NSM, &PM_NSM), PRIVILEGE_NORMAL, 0, &wrr_queue_threshold_dscp_map_cmd_imi);
  cli_install_imi (ctree, 16, PM_MSTP, PRIVILEGE_NORMAL, 0, &no_mstp_spanning_tree_guard_root_cmd_imi);
  cli_install_imi (ctree, 16, PM_NSM,  1,  0, &no_bandwidth_if_cmd_imi);
  cli_install_imi (ctree, 4, PM_NSM, PRIVILEGE_NORMAL, 0, &clear_gmrp_statistics2_cmd_imi);
  cli_install_imi (ctree, 84, modbmap_vor (2, &PM_NSM, &PM_NSM), PRIVILEGE_NORMAL, 0, &no_qos_pmapc_set_vlanpriority_cmd_imi);
  cli_install_imi (ctree, 5, PM_MSTP, PRIVILEGE_NORMAL, 0, &mstp_spanning_tree_forceversion_cmd_imi);
  cli_install_imi (ctree, 5, PM_NSM, PRIVILEGE_NORMAL, 0, &set_gmrp_registration1_cmd_imi);
  cli_install_imi (ctree, 5, PM_NSM,  1,  0, &no_debug_nsm_ha_all_cmd_imi);
  cli_install_imi (ctree, 16, PM_MSTP, PRIVILEGE_NORMAL, 0, &show_spanning_tree_rpvst_detail_cmd_imi);
  cli_install_imi (ctree, 140, PM_NSM, PRIVILEGE_NORMAL, 0, &lm_on_demand_detect_enable_cmd_imi);
  cli_install_imi (ctree, 5, PM_NSM, PRIVILEGE_NORMAL, 0, &no_gmrp_debug_all_cmd_imi);
  cli_install_imi (ctree, 16, PM_MSTP, PRIVILEGE_NORMAL, 0, &show_spanning_tree_rpvst_config_cmd_imi);
  cli_install_imi (ctree, 4, PM_NSM,  1,  0, &undebug_nsm_events_cmd_imi);
  cli_install_imi (ctree, 4, PM_MSTP, PRIVILEGE_NORMAL, 0, &show_spanning_tree_mst_cmd_imi);
  cli_install_imi (ctree, 5, PM_NSM, PRIVILEGE_NORMAL, 0, &set_gmrp_ext_filtering2_cmd_imi);
  cli_install_imi (ctree, 5, PM_NSM,  1,  0, &no_debug_nsm_ha_cmd_imi);
  cli_install_imi (ctree, 5, PM_NSM, PRIVILEGE_NORMAL, 0, &gvrp_debug_timer_cmd_imi);
  cli_install_imi (ctree, 4, PM_MSTP, PRIVILEGE_NORMAL, 0, &show_spanning_tree_mst_config_cmd_imi);
  cli_install_imi (ctree, 16, modbmap_vor (2, &PM_NSM, &PM_NSM), PRIVILEGE_NORMAL, 0, &no_mls_qos_dscp_mutation_cmd_imi);
  cli_install_imi (ctree, 4, PM_NSM, PRIVILEGE_NORMAL, 0, &show_default_priority_cmd_imi);
  cli_install_imi (ctree, 84, modbmap_vor (2, &PM_NSM, &PM_NSM), PRIVILEGE_NORMAL, 0, &no_qos_pmapc_set_ip_precedence_cmd_imi);
  cli_install_imi (ctree, 5, PM_MSTP, PRIVILEGE_NORMAL, 0, &mstp_bridge_cisco_interop_cmd_imi);
  cli_install_imi (ctree, 16, PM_MSTP, PRIVILEGE_NORMAL, 0, &no_mstp_port_hello_time_cmd_imi);
  cli_install_imi (ctree, 4, PM_NSM,  (15),  0, &undebug_hal_events_cmd_imi);
  cli_install_imi (ctree, 16, PM_MSTP, PRIVILEGE_NORMAL, 0, &mstp_add_l2gp_cmd_imi);
  cli_install_imi (ctree, 84, modbmap_vor (2, &PM_NSM, &PM_NSM), PRIVILEGE_NORMAL, 0, &qos_pmapc_set_ip_dscp_cmd_imi);
  cli_install_imi (ctree, 4, PM_MSTP, PRIVILEGE_NORMAL, 0, &show_spanning_tree_mst_interface_cmd_imi);
  cli_install_imi (ctree, 16, PM_MSTP, PRIVILEGE_NORMAL, 0, &show_spanning_tree_rpvst_cmd_imi);
  cli_install_imi (ctree, 5, PM_NSM, PRIVILEGE_NORMAL, 0, &no_gvrp_debug_imish_cmd_imi);
  cli_install_imi (ctree, 5, PM_MSTP, PRIVILEGE_NORMAL, 0, &mstp_spanning_max_age_cmd_imi);
  cli_install_imi (ctree, 110, PM_MSTP, PRIVILEGE_NORMAL, 0, &no_spanning_vlan_cmd_imi);
  cli_install_imi (ctree, 16, PM_NSM, PRIVILEGE_NORMAL, 0, &flow_ctrl_set_cmd_imi);
  cli_install_imi (ctree, 4, PM_NSM,  1,  0, &undebug_nsm_cmd_imi);
  cli_install_imi (ctree, 5, PM_MSTP, PRIVILEGE_NORMAL, 0, &no_debug_mstp_timer_cmd_imi);
  cli_install_imi (ctree, 16, PM_NSM, PRIVILEGE_NORMAL, 0, &nsm_vlan_switchport_trunk_pn_cn_none_cmd_imi);
  cli_install_imi (ctree, 4, PM_MSTP, PRIVILEGE_NORMAL, 0, &no_debug_mstp_proto_cmd_imi);
  cli_install_imi (ctree, 5, PM_MSTP, PRIVILEGE_NORMAL, 0, &no_spanning_max_hops_cmd_imi);
  cli_install_imi (ctree, 16, PM_NSM, PRIVILEGE_NORMAL, 0, &nsm_bridge_span_disable_cmd_imi);
  cli_install_imi (ctree, 5, PM_MSTP, PRIVILEGE_NORMAL, 0, &mstp_bridge_forward_time_cmd_imi);
  cli_install_imi (ctree, 82, modbmap_vor (2, &PM_NSM, &PM_NSM), PRIVILEGE_NORMAL, 0, &no_qos_cmap_match_exp_cmd_imi);
  cli_install_imi (ctree, 5, modbmap_vor (2, &PM_NSM, &PM_NSM), PRIVILEGE_NORMAL, 0, &ip_access_list_extended_host_any_cmd_imi);
  cli_install_imi (ctree, 67, PM_NSM, PRIVILEGE_NORMAL, 0, &no_nsm_vlan_no_name_cmd_imi);
  cli_install_imi (ctree, 5, PM_NSM, PRIVILEGE_NORMAL, 0, &set_gvrp_timer2_cmd_imi);
  cli_install_imi (ctree, 71, PM_MSTP, PRIVILEGE_NORMAL, 0, &no_spanning_region_cmd_imi);
  cli_install_imi (ctree, 5, PM_MSTP, PRIVILEGE_NORMAL, 0, &no_mstp_spanning_max_age_cmd_imi);
  cli_install_imi (ctree, 136, PM_NSM, PRIVILEGE_NORMAL, 0, &vpn_add_vport_cmd_imi);
  cli_install_imi (ctree, 16, PM_NSM, PRIVILEGE_NORMAL, 0, &nsm_vlan_switchport_hybrid_add_egress_cmd_imi);
  cli_install_imi (ctree, 4, PM_MSTP, PRIVILEGE_NORMAL, 0, &no_debug_mstp_all_cmd_imi);
  cli_install_imi (ctree, 5, PM_MSTP, PRIVILEGE_NORMAL, 0, &debug_mstp_proto_detail_cmd_imi);
  cli_install_imi (ctree, 16, PM_MSTP, PRIVILEGE_NORMAL, 0, &spanning_inst_path_cost_cmd_imi);
  cli_install_imi (ctree, 71, PM_MSTP, PRIVILEGE_NORMAL, 0, &bridge_region_cmd_imi);
  cli_install_imi (ctree, 16, PM_NSM, PRIVILEGE_NORMAL, 0, &default_user_priority_cmd_imi);
  cli_install_imi (ctree, 16, PM_MSTP, PRIVILEGE_NORMAL, 0, &bridge_group_inst_cmd_imi);
  cli_install_imi (ctree, 16, PM_MSTP, PRIVILEGE_NORMAL, 0, &no_spanning_port_vlan_priority_cmd_imi);
  cli_install_imi (ctree, 4, PM_NSM, PRIVILEGE_NORMAL, 0, &nsm_bridge_clear_fdb_cmd_imi);
  cli_install_imi (ctree, 5, PM_MSTP, PRIVILEGE_NORMAL, 0, &mstp_bridge_max_age_cmd_imi);
  cli_install_imi (ctree, 5, PM_MSTP, PRIVILEGE_NORMAL, 0, &mstp_spanning_tree_pathcost_method_cmd_imi);
  cli_install_imi (ctree, 84, modbmap_vor (2, &PM_NSM, &PM_NSM), PRIVILEGE_NORMAL, 0, &qos_pmapc_set_vlanpriority_cmd_imi);
  cli_install_imi (ctree, 4, PM_MSTP, PRIVILEGE_NORMAL, 0, &debug_mstp_all_cmd_imi);
  cli_install_imi (ctree, 4, PM_MSTP, PRIVILEGE_NORMAL, 0, &show_spanning_tree_interface_cmd_imi);
  cli_install_imi (ctree, 5, PM_MSTP, PRIVILEGE_NORMAL, 0, &mstp_span_portfast_bpdufilter_cmd_imi);
  cli_install_imi (ctree, 5, modbmap_vor (2, &PM_NSM, &PM_NSM), PRIVILEGE_NORMAL, 0, &no_qos_class_map_cmd_imi);
  cli_install_imi (ctree, 16, modbmap_vor (2, &PM_NSM, &PM_NSM), PRIVILEGE_NORMAL, 0, &priority_queue_out_cmd_imi);
  cli_install_imi (ctree, 5, PM_MSTP, PRIVILEGE_NORMAL, 0, &no_mstp_bridge_transmit_hold_count_cmd_imi);
  cli_install_imi (ctree, 4, PM_NSM, PRIVILEGE_NORMAL, 0, &show_gvrp_machine_br_cmd_imi);
  cli_install_imi (ctree, 5, PM_NSM, PRIVILEGE_NORMAL, 0, &set_mvrp_point_to_point_cmd_imi);
  cli_install_imi (ctree, 5, PM_MSTP, PRIVILEGE_NORMAL, 0, &mstp_spanning_tree_errdisable_timeout_interval_cmd_imi);
  cli_install_imi (ctree, 140, PM_NSM, PRIVILEGE_NORMAL, 0, &creat_peer_mep_cmd_imi);
  cli_install_imi (ctree, 4, PM_MSTP, PRIVILEGE_NORMAL, 0, &mstp_clear_spanning_tree_interface_bridge_cmd_imi);
  cli_install_imi (ctree, 4, PM_MSTP, PRIVILEGE_NORMAL, 0, &show_spanning_tree_mst_instance_interface_cmd_imi);
  cli_install_imi (ctree, 5, PM_NSM,  1,  0, &debug_nsm_events_cmd_imi);
  cli_install_imi (ctree, 138, PM_NSM, PRIVILEGE_NORMAL, 0, &ac_priority_cmd_imi);
  cli_install_imi (ctree, 82, modbmap_vor (2, &PM_NSM, &PM_NSM), PRIVILEGE_NORMAL, 0, &qos_cmap_match_exp_cmd_imi);
  cli_install_imi (ctree, 4, PM_NSM,  1,  0, &undebug_all_nsm_cmd_imi);
  cli_install_imi (ctree, 5, PM_MSTP, PRIVILEGE_NORMAL, 0, &mstp_bridge_ageing_time_cmd_imi);
  cli_install_imi (ctree, 67, PM_NSM, PRIVILEGE_NORMAL, 0, &no_nsm_vlan_mtu_cmd_imi);
  cli_install_imi (ctree, 146, PM_NSM, PRIVILEGE_NORMAL, 0, &ptp_port_enable_config_cmd_imi);
  cli_install_imi (ctree, 5, PM_NSM, PRIVILEGE_NORMAL, 0, &no_nsm_vlan_classifier_group_cmd_imi);
  cli_install_imi (ctree, 5, PM_MSTP, PRIVILEGE_NORMAL, 0, &no_mstp_spanning_priority_cmd_imi);
  cli_install_imi (ctree, 16, PM_MSTP, PRIVILEGE_NORMAL, 0, &no_spanning_inst_path_cost_cmd_imi);
  cli_install_imi (ctree, 16, PM_MSTP, PRIVILEGE_NORMAL, 0, &mstp_spanning_tree_portfast_bpduguard_cmd_imi);
  cli_install_imi (ctree, 5, modbmap_vor (2, &PM_NSM, &PM_NSM), PRIVILEGE_NORMAL, 0, &no_mac_access_list_any_host_cmd_imi);
  cli_install_imi (ctree, 5, PM_MSTP, PRIVILEGE_NORMAL, 0, &mstp_spanning_tree_errdisable_timeout_interval_cmd1_imi);
  cli_install_imi (ctree, 5, PM_NSM, PRIVILEGE_NORMAL, 0, &no_bridge_static_cmd_imi);
  cli_install_imi (ctree, 16, PM_NSM, PRIVILEGE_NORMAL, 0, &nsm_bridge_group_cmd_imi);
  cli_install_imi (ctree, 141, PM_NSM, PRIVILEGE_NORMAL, 0, &no_meg_index_cmd_imi);
  cli_install_imi (ctree, 5, PM_NSM, PRIVILEGE_NORMAL, 0, &gmrp_debug_event_cmd_imi);
  cli_install_imi (ctree, 4, PM_MSTP, PRIVILEGE_NORMAL, 0, &show_spanning_tree_rpvst_cmd_imi);
  cli_install_imi (ctree, 141, PM_NSM, PRIVILEGE_NORMAL, 0, &ACH_channel_type_and_mel_cmd_imi);
  cli_install_imi (ctree, 5, PM_NSM, PRIVILEGE_NORMAL, 0, &nsm_vlan_classifier_group_add_del_rule_cmd_imi);
  cli_install_imi (ctree, 5, PM_MSTP, PRIVILEGE_NORMAL, 0, &no_bridge_shutdown_multiple_spanning_tree_cmd_imi);
  cli_install_imi (ctree, 4, PM_NSM,  1,  0, &debug_nsm_ha_all_cmd_imi);
  cli_install_imi (ctree, 5, PM_NSM,  1,  0, &no_nsm_interface_cmd_imi);
  cli_install_imi (ctree, 5, modbmap_vor (2, &PM_NSM, &PM_NSM), PRIVILEGE_NORMAL, 0, &ip_access_list_extended_cmd_imi);
  cli_install_imi (ctree, 16, PM_MSTP, PRIVILEGE_NORMAL, 0, &mstp_spanning_tree_restricted_tcn_cmd_imi);
  cli_install_imi (ctree, 5, PM_MSTP, PRIVILEGE_NORMAL, 0, &mstp_spanning_priority_cmd_imi);
  cli_install_imi (ctree, 4, PM_NSM, PRIVILEGE_NORMAL, 0, &nsm_show_vlan_filter_cmd_imi);
  cli_install_imi (ctree, 5, modbmap_vor (2, &PM_NSM, &PM_NSM), PRIVILEGE_NORMAL, 0, &no_ip_access_list_extended_mask_any_cmd_imi);
  cli_install_imi (ctree, 136, PM_NSM, PRIVILEGE_NORMAL, 0, &vpn_rmv_mutigrp_cmd_imi);
  cli_install_imi (ctree, 82, modbmap_vor (2, &PM_NSM, &PM_NSM), PRIVILEGE_NORMAL, 0, &qos_cmap_match_traffic_type_cmd_imi);
  cli_install_imi (ctree, 16, PM_NSM,  1,  0, &nsm_interface_alias_cmd_imi);
  cli_install_imi (ctree, 145, PM_NSM, PRIVILEGE_NORMAL, 0, &domain_sport_add_cmd_imi);
  cli_install_imi (ctree, 16, PM_MSTP, PRIVILEGE_NORMAL, 0, &show_spanning_tree_mst_instance_cmd_imi);
  cli_install_imi (ctree, 16, PM_NSM, PRIVILEGE_NORMAL, 0, &user_prio_regen_table_cmd_imi);
  cli_install_imi (ctree, 5, PM_NSM, PRIVILEGE_NORMAL, 0, &set_gmrp_registration4_cmd_imi);
  cli_install_imi (ctree, 5, PM_NSM, PRIVILEGE_NORMAL, 0, &gvrp_debug_all_cmd_imi);
  cli_install_imi (ctree, 4, PM_NSM,  1,  0, &show_debugging_nsm_cmd_imi);
  cli_install_imi (ctree, 71, PM_MSTP, PRIVILEGE_NORMAL, 0, &no_spanning_instance_cmd_imi);
  cli_install_imi (ctree, 5, PM_NSM, PRIVILEGE_NORMAL, 0, &no_bridge_acquire_cmd_imi);
  cli_install_imi (ctree, 139, PM_NSM, PRIVILEGE_NORMAL, 0, &config_lsp_head_entity_cmd_imi);
  cli_install_imi (ctree, 4, PM_NSM, PRIVILEGE_NORMAL, 0, &show_gmrp_machine_br_cmd_imi);
  cli_install_imi (ctree, 5, PM_NSM, PRIVILEGE_NORMAL, 0, &gmrp_debug_imish_cmd_imi);
  cli_install_imi (ctree, 16, PM_MSTP, PRIVILEGE_NORMAL, 0, &no_mstp_spanning_tree_vlan_restricted_tcn_cmd_imi);
  cli_install_imi (ctree, 5, modbmap_vor (2, &PM_NSM, &PM_NSM), PRIVILEGE_NORMAL, 0, &ip_access_list_standard_any_cmd_imi);
  cli_install_imi (ctree, 16, modbmap_vor (2, &PM_NSM, &PM_NSM), PRIVILEGE_NORMAL, 0, &no_mls_qos_dscp_cos_cmd_imi);
  cli_install_imi (ctree, 140, PM_NSM, PRIVILEGE_NORMAL, 0, &oam_enable_cmd_imi);
  cli_install_imi (ctree, 5, PM_MSTP, PRIVILEGE_NORMAL, 0, &bridge_multiple_spanning_tree_cmd_imi);
  cli_install_imi (ctree, 140, PM_NSM, PRIVILEGE_NORMAL, 0, &no_creat_local_mep_cmd_imi);
  cli_install_imi (ctree, 5, modbmap_vor (2, &PM_NSM, &PM_NSM), PRIVILEGE_NORMAL, 0, &ip_access_list_extended_host_host_cmd_imi);
  cli_install_imi (ctree, 4, modbmap_vor (2, &PM_NSM, &PM_NSM), PRIVILEGE_NORMAL, 0, &show_class_map_cmd_imi);
  cli_install_imi (ctree, 5, PM_NSM, PRIVILEGE_NORMAL, 0, &set_port_gmrp_disable_vlan_cmd_imi);
  cli_install_imi (ctree, 5, PM_NSM, PRIVILEGE_NORMAL, 0, &set_gvrp_timer3_cmd_imi);
  cli_install_imi (ctree, 5, PM_MSTP, PRIVILEGE_NORMAL, 0, &bridge_max_hops_cmd_imi);
  cli_install_imi (ctree, 5, PM_MSTP, PRIVILEGE_NORMAL, 0, &no_bridge_max_hops_cmd_imi);
  cli_install_imi (ctree, 142, PM_NSM, PRIVILEGE_NORMAL, 0, &bind_section_tol3p_and_peerip_cmd_imi);
  cli_install_imi (ctree, 5, PM_NSM, PRIVILEGE_NORMAL, 0, &vlan_add_static_filter_cmd_imi);
  cli_install_imi (ctree, 5, PM_NSM, PRIVILEGE_NORMAL, 0, &no_pw_id_cmd_imi);
  cli_install_imi (ctree, 16, PM_NSM, PRIVILEGE_NORMAL, 0, &mirror_interface_both_cmd_imi);
  cli_install_imi (ctree, 4, PM_MSTP, PRIVILEGE_NORMAL, 0, &debug_mstp_proto_cmd_imi);
  cli_install_imi (ctree, 139, PM_NSM, PRIVILEGE_NORMAL, 0, &tun_car_cmd_imi);
  cli_install_imi (ctree, 5, PM_NSM, PRIVILEGE_NORMAL, 0, &no_nsm_vlan_classifier_rule_cmd_imi);
  cli_install_imi (ctree, 5, PM_NSM,  1,  0, &no_debug_nsm_packet_cmd_imi);
  cli_install_imi (ctree, 5, PM_NSM,  (15),  0, &no_interface_arbiter_cmd_imi);
  cli_install_imi (ctree, 4, PM_NSM, PRIVILEGE_NORMAL, 0, &show_gmrp_configuration_br_cmd_imi);
  cli_install_imi (ctree, 4, PM_NSM, PRIVILEGE_NORMAL, 0, &nsm_show_bridge_cmd_imi);
  cli_install_imi (ctree, 142, PM_NSM, PRIVILEGE_NORMAL, 0, &no_bind_section_tol3p_and_peerip_cmd_imi);
  cli_install_imi (ctree, 16, PM_MSTP, PRIVILEGE_NORMAL, 0, &mstp_spanning_tree_linktype_shared_cmd_imi);
  cli_install_imi (ctree, 5, PM_NSM, PRIVILEGE_NORMAL, 0, &no_vlan_static_filter_cmd_imi);
  cli_install_imi (ctree, 16, modbmap_vor (2, &PM_NSM, &PM_NSM), PRIVILEGE_NORMAL, 0, &mls_qos_dscp_cos_cmd_imi);
  cli_install_imi (ctree, 5, PM_NSM, PRIVILEGE_NORMAL, 0, &nsm_bridge_protocol_rpvst_plus_cmd_imi);
  cli_install_imi (ctree, 5, PM_NSM, PRIVILEGE_NORMAL, 0, &nsm_vlan_classifier_proto_cmd_imi);
  cli_install_imi (ctree, 144, PM_NSM, PRIVILEGE_NORMAL, 0, &port_rate_limit_cmd_imi);
  cli_install_imi (ctree, 5, PM_MSTP, PRIVILEGE_NORMAL, 0, &mstp_bridge_hello_time_cmd_imi);
  cli_install_imi (ctree, 5, PM_MSTP, PRIVILEGE_NORMAL, 0, &mstp_spanning_forward_time_cmd_imi);
  cli_install_imi (ctree, 16, PM_MSTP, PRIVILEGE_NORMAL, 0, &show_spanning_tree_rpvst_vlan_cmd_imi);
  cli_install_imi (ctree, 5, PM_NSM, PRIVILEGE_NORMAL, 0, &set_gmrp1_br_cmd_imi);
  cli_install_imi (ctree, 82, modbmap_vor (2, &PM_NSM, &PM_NSM), PRIVILEGE_NORMAL, 0, &qos_cmap_match_l4_port_cmd_imi);
  cli_install_imi (ctree, 145, PM_NSM, PRIVILEGE_NORMAL, 0, &clocktype_config_cmd_imi);
  cli_install_imi (ctree, 4, PM_NSM,  1,  0, &debug_nsm_events_cmd_imi);
  cli_install_imi (ctree, 4, PM_NSM, PRIVILEGE_NORMAL, 0, &megshow_information_cmd_imi);
  cli_install_imi (ctree, 4, PM_NSM, PRIVILEGE_NORMAL, 0, &show_flowcontrol_cmd_imi);
  cli_install_imi (ctree, 5, PM_MSTP, PRIVILEGE_NORMAL, 0, &no_debug_mstp_timer_detail_cmd_imi);
  cli_install_imi (ctree, 16, PM_MSTP, PRIVILEGE_NORMAL, 0, &mstp_add_l2gp_rx_statuscmd_imi);
  cli_install_imi (ctree, 4, PM_NSM, PRIVILEGE_NORMAL, 0, &show_gmrp_timer_cmd_imi);
  cli_install_imi (ctree, 5, PM_MSTP, PRIVILEGE_NORMAL, 0, &no_debug_mstp_all_cmd_imi);
  cli_install_imi (ctree, 4, PM_NSM, PRIVILEGE_NORMAL, 0, &nsm_bridge_clear_br_fdb_vlan_port_cmd_imi);
  cli_install_imi (ctree, 4, PM_MSTP, PRIVILEGE_NORMAL, 0, &show_debugging_mstp_cmd_imi);
  cli_install_imi (ctree, 5, modbmap_vor (2, &PM_NSM, &PM_NSM), PRIVILEGE_NORMAL, 0, &ip_access_list_extended_host_mask_cmd_imi);
  cli_install_imi (ctree, 84, modbmap_vor (2, &PM_NSM, &PM_NSM), PRIVILEGE_NORMAL, 0, &qos_pmapc_police_cmd_imi);
  cli_install_imi (ctree, 5, PM_MSTP, PRIVILEGE_NORMAL, 0, &no_mstp_spanning_tree_errdisable_timeout_enable_cmd1_imi);
  cli_install_imi (ctree, 16, PM_MSTP, PRIVILEGE_NORMAL, 0, &no_spanning_inst_cmd_imi);
  cli_install_imi (ctree, 16, modbmap_vor (2, &PM_NSM, &PM_NSM), PRIVILEGE_NORMAL, 0, &wrr_queue_bandwidth_cmd_imi);
  cli_install_imi (ctree, 5, PM_MSTP, PRIVILEGE_NORMAL, 0, &no_spanning_vlan_priority_cmd_imi);
  cli_install_imi (ctree, 5, PM_NSM, PRIVILEGE_NORMAL, 0, &set_gvrp_registration_fixed_cmd_imi);
  cli_install_imi (ctree, 5, PM_NSM, PRIVILEGE_NORMAL, 0, &nsm_no_vlan_filter_cmd_imi);
  cli_install_imi (ctree, 16, modbmap_vor (2, &PM_NSM, &PM_NSM), PRIVILEGE_NORMAL, 0, &no_wrr_queue_bandwidth_cmd_imi);
  cli_install_imi (ctree, 5, modbmap_vor (2, &PM_NSM, &PM_NSM), PRIVILEGE_NORMAL, 0, &mac_access_list_host_host_cmd_imi);
  cli_install_imi (ctree, 5, PM_NSM, PRIVILEGE_NORMAL, 0, &no_gvrp_debug_event_cmd_imi);
  cli_install_imi (ctree, 4, PM_FM,  (15),  (1 << 11), &fm_show_proc_names_cmd_imi);
  cli_install_imi (ctree, 84, modbmap_vor (2, &PM_NSM, &PM_NSM), PRIVILEGE_NORMAL, 0, &no_qos_pmapc_police_rate_cmd_imi);
  cli_install_imi (ctree, 4, PM_NSM, PRIVILEGE_NORMAL, 0, &show_traffic_class_table_cmd_imi);
  cli_install_imi (ctree, 5, PM_MSTP, PRIVILEGE_NORMAL, 0, &spanning_tree_ageing_time_cmd_imi);
  cli_install_imi (ctree, 4, PM_NSM, PRIVILEGE_NORMAL, 0, &clear_gmrp_br_statistics2_cmd_imi);
  cli_install_imi (ctree, 5, modbmap_vor (2, &PM_NSM, &PM_NSM), PRIVILEGE_NORMAL, 0, &mac_acl_any_host_cmd_imi);
  cli_install_imi (ctree, 4, PM_MSTP, PRIVILEGE_NORMAL, 0, &mstp_clear_spanning_tree_detected_protocols_interface_cmd_imi);
  cli_install_imi (ctree, 5, modbmap_vor (2, &PM_NSM, &PM_NSM), PRIVILEGE_NORMAL, 0, &ip_access_list_standard_cmd_imi);
  cli_install_imi (ctree, 16, PM_MSTP, PRIVILEGE_NORMAL, 0, &no_mstp_spanning_tree_inst_restricted_tcn_cmd_imi);
  cli_install_imi (ctree, 5, modbmap_vor (2, &PM_NSM, &PM_NSM), PRIVILEGE_NORMAL, 0, &no_ip_access_list_standard_nomask_cmd_imi);
  cli_install_imi (ctree, 16, PM_NSM,  1,  0, &no_nsm_access_group_any_cmd_imi);
  cli_install_imi (ctree, 5, PM_NSM, PRIVILEGE_NORMAL, 0, &no_lsp_id_cmd_imi);
  cli_install_imi (ctree, 5, PM_NSM,  1,  0, &debug_nsm_ha_all_cmd_imi);
  cli_install_imi (ctree, 71, PM_MSTP, PRIVILEGE_NORMAL, 0, &spanning_instance_vlan_cmd_imi);
  cli_install_imi (ctree, 4, modbmap_vor (2, &PM_NSM, &PM_NSM), PRIVILEGE_NORMAL, 0, &show_mac_acl_cmd_imi);
  cli_install_imi (ctree, 142, PM_NSM, PRIVILEGE_NORMAL, 0, &no_meg_index_cmd_imi);
  cli_install_imi (ctree, 5, PM_MSTP, PRIVILEGE_NORMAL, 0, &bridge_shutdown_multiple_spanning_tree_for_cmd_imi);
  cli_install_imi (ctree, 82, modbmap_vor (2, &PM_NSM, &PM_NSM), PRIVILEGE_NORMAL, 0, &no_qos_cmap_match_ip_precedence_cmd_imi);
  cli_install_imi (ctree, 4, PM_NSM,  1,  0, &debug_nsm_kernel_cmd_imi);
  cli_install_imi (ctree, 16, PM_NSM, PRIVILEGE_NORMAL, 0, &no_vlan_switchport_pvid_cmd_imi);
  cli_install_imi (ctree, 146, PM_NSM, PRIVILEGE_NORMAL, 0, &ptp_port_bind_config_cmd_imi);
  cli_install_imi (ctree, 140, PM_NSM, PRIVILEGE_NORMAL, 0, &dm_way_select_period_cmd_imi);
  cli_install_imi (ctree, 4, PM_MSTP, PRIVILEGE_NORMAL, 0, &no_debug_mstp_packet_tx_cmd_imi);
  cli_install_imi (ctree, 4, PM_MSTP, PRIVILEGE_NORMAL, 0, &show_spanning_tree_mst_detail_interface_cmd_imi);
  cli_install_imi (ctree, 4, PM_MSTP, PRIVILEGE_NORMAL, 0, &debug_mstp_proto_detail_cmd_imi);
  cli_install_imi (ctree, 4, PM_NSM, PRIVILEGE_NORMAL, 0, &nsm_bridge_clear_dynamic_fdb_cmd_imi);
  cli_install_imi (ctree, 16, PM_MSTP, PRIVILEGE_NORMAL, 0, &no_mstp_spanning_tree_inst_restricted_role_cmd_imi);
  cli_install_imi (ctree, 145, PM_NSM, PRIVILEGE_NORMAL, 0, &domain_create_cmd_imi);
  cli_install_imi (ctree, 16, PM_MSTP, PRIVILEGE_NORMAL, 0, &mstp_spanning_transmit_hold_count_cmd_imi);
  cli_install_imi (ctree, 5, PM_MSTP, PRIVILEGE_NORMAL, 0, &no_mstp_bridge_hello_time_cmd_imi);
  cli_install_imi (ctree, 5, modbmap_vor (2, &PM_NSM, &PM_NSM), PRIVILEGE_NORMAL, 0, &no_ip_access_list_extended_host_mask_cmd_imi);
  cli_install_imi (ctree, 16, PM_MSTP, PRIVILEGE_NORMAL, 0, &no_mstp_spanning_transmit_hold_count_cmd_imi);
  cli_install_imi (ctree, 5, modbmap_vor (2, &PM_NSM, &PM_NSM), PRIVILEGE_NORMAL, 0, &ip_access_list_standard_nomask_cmd_imi);
  cli_install_imi (ctree, 4, PM_MSTP, PRIVILEGE_NORMAL, 0, &debug_mstp_packet_tx_cmd_imi);
  cli_install_imi (ctree, 5, PM_NSM, PRIVILEGE_NORMAL, 0, &no_lsp_group_cmd_imi);
  cli_install_imi (ctree, 5, modbmap_vor (2, &PM_NSM, &PM_NSM), PRIVILEGE_NORMAL, 0, &no_ip_access_list_extended_host_any_cmd_imi);
  cli_install_imi (ctree, 5, PM_NSM, PRIVILEGE_NORMAL, 0, &gvrp_debug_event_cmd_imi);
  cli_install_imi (ctree, 5, PM_NSM, PRIVILEGE_NORMAL, 0, &nsm_vlan_classifier_ipv4_cmd_imi);
  cli_install_imi (ctree, 5, PM_NSM, PRIVILEGE_NORMAL, 0, &set_gmrp_br_instance_disable_cmd_imi);
  cli_install_imi (ctree, 71, PM_MSTP, PRIVILEGE_NORMAL, 0, &bridge_instance_cmd_imi);
  cli_install_imi (ctree, 84, modbmap_vor (2, &PM_NSM, &PM_NSM), PRIVILEGE_NORMAL, 0, &no_qos_pmapc_set_ip_dscp_cmd_imi);
  cli_install_imi (ctree, 4, modbmap_vor (2, &PM_NSM, &PM_NSM), PRIVILEGE_NORMAL, 0, &show_class_map_name_cmd_imi);
  cli_install_imi (ctree, 16, PM_MSTP, PRIVILEGE_NORMAL, 0, &bridge_group_vlan_cmd_imi);
  cli_install_imi (ctree, 16, modbmap_vor (2, &PM_NSM, &PM_NSM), PRIVILEGE_NORMAL, 0, &wrr_queue_min_reserve_cmd_imi);
  cli_install_imi (ctree, 16, PM_MSTP, PRIVILEGE_NORMAL, 0, &mstp_spanning_tree_portfast_bpdufilter_cmd_imi);
  cli_install_imi (ctree, 4, PM_NSM, PRIVILEGE_NORMAL, 0, &nsm_bridge_clear_dynamic_br_fdb_cmd_imi);
  cli_install_imi (ctree, 4, PM_NSM, PRIVILEGE_NORMAL, 0, &show_flowcontrol_interface_cmd_imi);
  cli_install_imi (ctree, 16, PM_MSTP, PRIVILEGE_NORMAL, 0, &no_mstp_spanning_tree_restricted_role_cmd_imi);
  cli_install_imi (ctree, 16, PM_MSTP, PRIVILEGE_NORMAL, 0, &no_mstp_bridge_transmit_hold_count_cmd_imi);
  cli_install_imi (ctree, 5, PM_NSM, PRIVILEGE_NORMAL, 0, &no_gvrp_debug_all_cmd_imi);
  cli_install_imi (ctree, 16, PM_MSTP, PRIVILEGE_NORMAL, 0, &mstp_spanning_path_cost_cmd_imi);
  cli_install_imi (ctree, 140, PM_NSM, PRIVILEGE_NORMAL, 0, &no_creat_peer_mep_cmd_imi);
  cli_install_imi (ctree, 5, PM_NSM, PRIVILEGE_NORMAL, 0, &set_gmrp_fwd_all2_cmd_imi);
  cli_install_imi (ctree, 4, PM_NSM,  1,  0, &no_debug_nsm_ha_all_cmd_imi);
  cli_install_imi (ctree, 16, PM_NSM,  1,  0, &bandwidth_if_cmd_imi);
  cli_install_imi (ctree, 4, PM_NSM, PRIVILEGE_NORMAL, 0, &show_nsm_vlan_classifier_interface_group_cmd_imi);
  cli_install_imi (ctree, 5, modbmap_vor (2, &PM_NSM, &PM_NSM), PRIVILEGE_NORMAL, 0, &ip_access_list_extended_any_mask_cmd_imi);
  cli_install_imi (ctree, 5, PM_NSM, PRIVILEGE_NORMAL, 0, &set_gmrp_registration2_cmd_imi);
  cli_install_imi (ctree, 16, PM_MSTP, PRIVILEGE_NORMAL, 0, &show_spanning_tree_mst_cmd_imi);
  cli_install_imi (ctree, 4, PM_NSM, PRIVILEGE_NORMAL, 0, &nsm_bridge_clear_dynamic_br_fdb_bridge_cmd_imi);
  cli_install_imi (ctree, 89, PM_NSM,  1,  0, &vlan_action_cmd_imi);
  cli_install_imi (ctree, 16, PM_NSM,  1,  0, &nsm_if_shutdown_cmd_imi);
  cli_install_imi (ctree, 5, modbmap_vor (2, &PM_NSM, &PM_NSM), PRIVILEGE_NORMAL, 0, &mac_acl_host_any_cmd_imi);
  cli_install_imi (ctree, 5, PM_NSM, PRIVILEGE_NORMAL, 0, &gmrp_debug_packet_cmd_imi);
  cli_install_imi (ctree, 138, PM_NSM, PRIVILEGE_NORMAL, 0, &ac_car_cmd_imi);
  cli_install_imi (ctree, 4, PM_NSM, PRIVILEGE_NORMAL, 0, &show_user_prio_regen_table_cmd_imi);
  cli_install_imi (ctree, 5, PM_NSM,  1,  0, &debug_nsm_packet_cmd_imi);
  cli_install_imi (ctree, 4, PM_MSTP, PRIVILEGE_NORMAL, 0, &show_spanning_tree_rpvst_vlan_cmd_imi);
  cli_install_imi (ctree, 16, PM_MSTP, PRIVILEGE_NORMAL, 0, &no_mstp_add_l2gp_cmd_imi);
  cli_install_imi (ctree, 140, PM_NSM, PRIVILEGE_NORMAL, 0, &creat_mip_cmd_imi);
  cli_install_imi (ctree, 4, PM_NSM, PRIVILEGE_NORMAL, 0, &show_nsm_vlan_brief_cmd_imi);
  cli_install_imi (ctree, 5, PM_NSM,  1,  0, &no_debug_nsm_all_cmd_imi);
  cli_install_imi (ctree, 16, PM_MSTP, PRIVILEGE_NORMAL, 0, &no_mstp_spanning_tree_restricted_tcn_cmd_imi);
  cli_install_imi (ctree, 84, modbmap_vor (2, &PM_NSM, &PM_NSM), PRIVILEGE_NORMAL, 0, &no_qos_pmapc_police_cmd_imi);
  cli_install_imi (ctree, 82, modbmap_vor (2, &PM_NSM, &PM_NSM), PRIVILEGE_NORMAL, 0, &qos_cmap_match_ip_precedence_cmd_imi);
  cli_install_imi (ctree, 71, PM_MSTP, PRIVILEGE_NORMAL, 0, &bridge_instance_vlan_cmd_imi);
  cli_install_imi (ctree, 4, PM_NSM,  1,  0, &no_debug_nsm_packet_cmd_imi);
  cli_install_imi (ctree, 5, PM_NSM, PRIVILEGE_NORMAL, 0, &show_lspoam_status_cmd_imi);
  cli_install_imi (ctree, 4, PM_MSTP, PRIVILEGE_NORMAL, 0, &show_spanning_tree_pathcost_method_cmd_imi);
  cli_install_imi (ctree, 4, PM_NSM, PRIVILEGE_NORMAL, 0, &clear_gmrp_br_statistics1_cmd_imi);
  cli_install_imi (ctree, 135, PM_NSM, PRIVILEGE_NORMAL, 0, &pw_config_cmd_imi);
  cli_install_imi (ctree, 5, modbmap_vor (2, &PM_NSM, &PM_NSM), PRIVILEGE_NORMAL, 0, &no_ip_access_list_extended_mask_host_cmd_imi);
  cli_install_imi (ctree, 140, PM_NSM, PRIVILEGE_NORMAL, 0, &lp_alarm_threshold_cmd_imi);
  cli_install_imi (ctree, 5, modbmap_vor (2, &PM_NSM, &PM_NSM), PRIVILEGE_NORMAL, 0, &no_qos_policy_map_cmd_imi);
  cli_install_imi (ctree, 140, PM_NSM, PRIVILEGE_NORMAL, 0, &creat_local_mep_cmd_imi);
  cli_install_imi (ctree, 5, PM_NSM, PRIVILEGE_NORMAL, 0, &vlan_add_static_forward_cmd_imi);
  cli_install_imi (ctree, 5, modbmap_vor (2, &PM_NSM, &PM_NSM), PRIVILEGE_NORMAL, 0, &ip_access_list_extended_any_host_cmd_imi);
  cli_install_imi (ctree, 4, PM_NSM,  1,  0, &undebug_nsm_kernel_cmd_imi);
  cli_install_imi (ctree, 4, PM_NSM, PRIVILEGE_NORMAL, 0, &nsm_show_vlan_access_map_cmd_imi);
  cli_install_imi (ctree, 4, PM_FM,  (15),  (1 << 11), &fm_faults_delete_cmd_imi);
  cli_install_imi (ctree, 4, PM_MSTP, PRIVILEGE_NORMAL, 0, &show_spanning_tree_stats_cmd_imi);
  cli_install_imi (ctree, 16, PM_MSTP, PRIVILEGE_NORMAL, 0, &no_bridge_group_vlan_path_cost_cmd_imi);
  cli_install_imi (ctree, 4, PM_NSM, PRIVILEGE_NORMAL, 0, &show_l2_ratelimit_cmd_imi);
  cli_install_imi (ctree, 5, PM_NSM, PRIVILEGE_NORMAL, 0, &set_gmrp_ext_filtering_br_cmd_imi);
  cli_install_imi (ctree, 5, PM_MSTP, PRIVILEGE_NORMAL, 0, &no_mstp_bridge_ageing_time_cmd_imi);
  cli_install_imi (ctree, 5, PM_NSM, PRIVILEGE_NORMAL, 0, &no_gmrp_debug_packet_cmd_imi);
  cli_install_imi (ctree, 16, PM_NSM, PRIVILEGE_NORMAL, 0, &show_mirror_interface_cmd_imi);
  cli_install_imi (ctree, 4, PM_FM,  (15),  (1 << 11), &fm_faults_test_cmd_imi);
  cli_install_imi (ctree, 5, PM_NSM, PRIVILEGE_NORMAL, 0, &set_gmrp_timer3_cmd_imi);
  cli_install_imi (ctree, 84, modbmap_vor (2, &PM_NSM, &PM_NSM), PRIVILEGE_NORMAL, 0, &qos_pmapc_set_quantumpriority_cmd_imi);
  cli_install_imi (ctree, 5, PM_NSM, PRIVILEGE_NORMAL, 0, &no_vlan_static_cmd_imi);
  cli_install_imi (ctree, 140, PM_NSM, PRIVILEGE_NORMAL, 0, &megshow_information_cmd_imi);
  cli_install_imi (ctree, 4, PM_MSTP, PRIVILEGE_NORMAL, 0, &no_debug_mstp_timer_detail_cmd_imi);
  cli_install_imi (ctree, 71, PM_MSTP, PRIVILEGE_NORMAL, 0, &no_bridge_instance_cmd_imi);
  cli_install_imi (ctree, 5, PM_NSM, PRIVILEGE_NORMAL, 0, &bridge_add_static_forward_cmd_imi);
  cli_install_imi (ctree, 110, PM_MSTP, PRIVILEGE_NORMAL, 0, &no_bridge_vlan_cmd_imi);
  cli_install_imi (ctree, 16, PM_MSTP, PRIVILEGE_NORMAL, 0, &mstp_bridge_transmit_hold_count_cmd_imi);
  cli_install_imi (ctree, 148, PM_NSM, PRIVILEGE_NORMAL, 0, &lsp_oam_cfg_cmd_imi);
  cli_install_imi (ctree, 16, modbmap_vor (2, &PM_NSM, &PM_NSM), PRIVILEGE_NORMAL, 0, &no_priority_queue_out_cmd_imi);
  cli_install_imi (ctree, 5, PM_NSM, PRIVILEGE_NORMAL, 0, &set_gvrp_br_dyn_vlan_enable_cmd_imi);
  cli_install_imi (ctree, 144, PM_NSM, PRIVILEGE_NORMAL, 0, &port_shapping_cmd_imi);
  cli_install_imi (ctree, 84, modbmap_vor (2, &PM_NSM, &PM_NSM), PRIVILEGE_NORMAL, 0, &no_qos_pmapc_set_quantumpriority_cmd_imi);
  cli_install_imi (ctree, 5, modbmap_vor (2, &PM_NSM, &PM_NSM), PRIVILEGE_NORMAL, 0, &no_mac_access_list_host_any_cmd_imi);
  cli_install_imi (ctree, 4, PM_MSTP, PRIVILEGE_NORMAL, 0, &show_spanning_tree_mst_detail_cmd_imi);
  cli_install_imi (ctree, 67, PM_NSM, PRIVILEGE_NORMAL, 0, &nsm_vlan_mtu_cmd_imi);
  cli_install_imi (ctree, 4, PM_MSTP, PRIVILEGE_NORMAL, 0, &show_spanning_tree_rpvst_config_cmd_imi);
  cli_install_imi (ctree, 5, PM_MSTP, PRIVILEGE_NORMAL, 0, &bridge_vlan_priority_cmd_imi);
  cli_install_imi (ctree, 5, modbmap_vor (2, &PM_NSM, &PM_NSM), PRIVILEGE_NORMAL, 0, &no_ip_access_list_standard_any_cmd_imi);
  cli_install_imi (ctree, 5, PM_NSM, PRIVILEGE_NORMAL, 0, &set_gvrp_registration_normal_cmd_imi);
  cli_install_imi (ctree, 5, PM_NSM, PRIVILEGE_NORMAL, 0, &set_gvrp_app_state_normal_cmd_imi);
  cli_install_imi (ctree, 4, PM_NSM, PRIVILEGE_NORMAL, 0, &show_gmrp_statistics_cmd_imi);
  cli_install_imi (ctree, 5, PM_NSM, PRIVILEGE_NORMAL, 0, &nsm_vlan_classifier_mac_cmd_imi);
  cli_install_imi (ctree, 5, PM_NSM,  (15),  0, &no_debug_hal_events_cmd_imi);
  cli_install_imi (ctree, 16, PM_NSM, PRIVILEGE_NORMAL, 0, &mirror_interface_receive_cmd_imi);
  cli_install_imi (ctree, 5, PM_NSM, PRIVILEGE_NORMAL, 0, &no_mac_prio_override_cmd_imi);
  cli_install_imi (ctree, 5, PM_MSTP, PRIVILEGE_NORMAL, 0, &no_debug_mstp_proto_cmd_imi);
  cli_install_imi (ctree, 143, PM_NSM, PRIVILEGE_NORMAL, 0, &lsp_car_cmd_imi);
  cli_install_imi (ctree, 5, PM_NSM,  1,  0, &debug_nsm_kernel_cmd_imi);
  cli_install_imi (ctree, 4, PM_NSM, PRIVILEGE_NORMAL, 0, &show_gmrp_br_statistics_cmd_imi);
  cli_install_imi (ctree, 143, PM_NSM, PRIVILEGE_NORMAL, 0, &lsp_cfg_switch_cmd_imi);
  cli_install_imi (ctree, 5, PM_MSTP, PRIVILEGE_NORMAL, 0, &spanning_multiple_spanning_tree_cmd_imi);
  cli_install_imi (ctree, 67, PM_NSM, PRIVILEGE_NORMAL, 0, &nsm_vlan_enable_disable_cmd_imi);
  cli_install_imi (ctree, 4, PM_NSM, PRIVILEGE_NORMAL, 0, &clear_gvrp_statistics_all_cmd_imi);
  cli_install_imi (ctree, 4, PM_MSTP, PRIVILEGE_NORMAL, 0, &show_spanning_tree_instance_vlan_cmd_imi);
  cli_install_imi (ctree, 140, PM_NSM, PRIVILEGE_NORMAL, 0, &cc_packet_send_period_cmd_imi);
  cli_install_imi (ctree, 5, PM_MSTP, PRIVILEGE_NORMAL, 0, &no_mstp_bridge_portfast_bpdufilter_cmd_imi);
  cli_install_imi (ctree, 16, PM_NSM, PRIVILEGE_NORMAL, 0, &nsm_vlan_switchport_hybrid_acceptable_frame_cmd_imi);
  cli_install_imi (ctree, 82, modbmap_vor (2, &PM_NSM, &PM_NSM), PRIVILEGE_NORMAL, 0, &no_qos_cmap_match_vlan_cmd_imi);
  cli_install_imi (ctree, 4, PM_NSM,  1,  0, &no_debug_all_nsm_cmd_imi);
  cli_install_imi (ctree, 5, PM_MSTP, PRIVILEGE_NORMAL, 0, &no_mstp_spanning_tree_errdisable_timeout_interval_cmd1_imi);
  cli_install_imi (ctree, 5, modbmap_vor (2, &PM_NSM, &PM_NSM), PRIVILEGE_NORMAL, 0, &no_mac_acl_any_host_cmd_imi);
  cli_install_imi (ctree, 5, PM_NSM, PRIVILEGE_NORMAL, 0, &no_lsp_oam_cmd_imi);
}
