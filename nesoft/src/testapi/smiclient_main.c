/* Copyright (C) 2001-2003  All Rights Reserved. */

/* API testing program */

#include "smiclient.h"

#define START_ALL

void
smi_check_ret (int ret, enum smi_bridge_pri_ovr_mac_type ovr_mac_type,
               u_int8_t priority);

void testapi_process_alarm_message (smi_alarm alarm, smi_api_module module, 
                                    void *msg);
#ifdef TEST_AC
int hex2num(char c);
int hwaddr_aton(char *txt, unsigned char *addr);

/* Interface related settings */
menuItem if_menu[]=
  {
      {"setmtu", "\tSet MTU on the interface", smi_if_action},
      {"getmtu", "\tGet MTU on the interface", smi_if_action},
      {"setbw", "\tSet Bandwidth on the interface", smi_if_action},
      {"getbw", "\tGet Bandwidth on the interface", smi_if_action},
      {"setdplx", "\tSet Duplex on the interface", smi_if_action},
      {"getdplx", "\tGet Duplex on the interface", smi_if_action},
      {"unsetdp", "\tSet the interface to Auto-Negotiation mode", 
       smi_if_action},
      {"flgup", "\tEnable the Interface", smi_if_action},
      {"flgdn", "\tDisable the Interface", smi_if_action},
      {"setauto", "\tSet auto-negotiation on the Interface", smi_if_action},
      {"getauto", "\tGet auto-negotiation status of the Interface",
      smi_if_action},
      {"sethwa", "\tSet Hardware Address of the Interface", smi_if_action},
      {"gethwa", "\tGet Hardware Address of the Interface", smi_if_action},
      {"getipb", "\tGet Interface and Operational state information", 
      smi_if_action},
      {"setmcast", "Set Interface for multicasting", smi_if_action},
      {"getmcast", "Get Interface flag status on multicast", smi_if_action},
      {"unsetmt", "\tDisable Multicasting on the Interface", smi_if_action},
      {"getchange", "get the link status change of interface", smi_if_action},   
      {"getstat", "\tGet packet statistics for the interface", smi_if_action},
      {"setmdix", "\t Set the MDI/ MDIX crossover setting for the port", smi_if_action},
      {"getmdix", "\t Get the MDI/ MDIX crossover for the port", smi_if_action},
      {"clearstat", "\t Clear the packet statisctics on the interface ", smi_if_action},
      {"getbridge", "\tGet bridge config ", smi_if_action},
      {"addbridge", "Add a Bridge", smi_if_action},
      {"addbrport", "Add a Bridge Port", smi_if_action},
      {"mgmt", "Set the Management VLAN", smi_if_action},
      {"chagetometromgmt", "Changed to Metro Switch", smi_if_action},
      {"disabledot1q", "Changed to Metro Switch", smi_if_action},
      {"changetype", "Change Bridge & Topology Type", smi_if_action},
      {"delbridge", "\tDelete bridge ", smi_if_action},
      {"delbridgeport", "\tDelete port from bridge ", smi_if_action},
      {"addPortToChannel","\tAdd Port to the channel group ",smi_if_action},
      {"deleteFromChannel","\t Delete Port from Channel Group ",smi_if_action}, 
      {"flgget", "\tGet flags on interface", smi_if_action},
      {"dtagset", "\tSet Double Tag interface", smi_if_action},
      {"dump", "\tDump configuration args", dumpargs},
      {"exit", "\tExit menu", exit_menu},
      {"quit", "\tQuit program", quit},
      { NULL, NULL, NULL}
 };

/* VLAN related settings */
menuItem vlan_menu[]= {
      {"addvlan", "\t\tAdd VLAN", smi_vlan_action},
      {"delvlan", "\t\tDelete VLAN", smi_vlan_action},
      {"addvlanrange", "\t\tAdd Vlan Range", smi_vlan_action},
      {"delvlanrange", "\t\t Delete VlanRange", smi_vlan_action},
      {"setvlanportmode", "\tSet VLAN port mode", smi_vlan_action},
      {"getvlanportmode", "\tGet VLAN port mode", smi_vlan_action},
      {"setframetype", "\tSet acceptable frame type", smi_vlan_action},
      {"getframetype", "\tGet acceptable frame type", smi_vlan_action},
      {"setingfil", "\tSet ingress filter", smi_vlan_action},
      {"getingfil", "\tGet ingress filter", smi_vlan_action},
      {"setdvid", "\t\tSet default vlan id", smi_vlan_action},
      {"getdvid", "\t\tGet default vlan id", smi_vlan_action},
      {"addvlanport", "\tAdd VLAN to port", smi_vlan_action},
      {"delvlanport", "\tDelete VLAN from port", smi_vlan_action},
      {"clrport", "\t\tClear hybrid port", smi_vlan_action},
      {"addallexvid", "\tAll all except vid", smi_vlan_action},
      {"getallvlan", "\tGet all vlans", smi_vlan_action},
      {"getvlanbyid", "\tGet vlan config by id", smi_vlan_action},
      {"getvlanif", "\tGet vlan config ", smi_vlan_action},
      {"getbridge", "\tGet bridge config ", smi_vlan_action},
      {"protoprocess", "\tSet proto processing on cust port", smi_vlan_action},
      {"getprotoprocess", "\tGet proto processing on cust port", 
        smi_vlan_action},
      {"dump", "\t\tDump configuration args", dumpargs},
      {"exit", "\t\tExit menu", exit_menu},
      {"quit", "\t\tQuit program", quit},
      { NULL, NULL, NULL}
 };

/* MSTP related settings */
menuItem mstp_menu[]= {
      {"addins", "\tAdd MSTP Instance", smi_mstp_action},
      {"delins", "\tAdd MSTP Instance", smi_mstp_action},
      {"setage", "\tSet Ageingtime", smi_mstp_action},
      {"getage", "\tGet Ageingtime", smi_mstp_action},
      {"addport", "\tMSTP add port", smi_mstp_action},
      {"delport", "\tMSTP delete port", smi_mstp_action},
      {"sethtime", "Set Hello time", smi_mstp_action},
      {"gethtime", "Get Hello time", smi_mstp_action},
      {"setmage", "\tSet MSTP max age", smi_mstp_action},
      {"getmage", "\tGet MSTP max age", smi_mstp_action},
      {"setporteg", "Set MSTP port edge", smi_mstp_action},
      {"getporteg", "Get MSTP port edge", smi_mstp_action},
      {"setportver", "Set STP version", smi_mstp_action},
      {"getportver", "Get STP version", smi_mstp_action},
      {"setpr", "\tSet Bridge Priority", smi_mstp_action},
      {"getpr", "\tGet Bridge Priority", smi_mstp_action},
      {"getfwdd", "\tGet the bridge forward delay time", smi_mstp_action},
      {"setfwdd", "\tSet the bridge forward delay time", smi_mstp_action},
      {"setmpr", "\tSet the priority of the bridge & ports", smi_mstp_action},
      {"getmpr", "\tGet bridge instance priority & bridge priority", 
       smi_mstp_action},
      {"setmcost", "Set the MSTI path cost for a port", smi_mstp_action},
      {"getmcost", "Get the MSTI path cost for a port", smi_mstp_action},
      {"setrst", "\tSet the instance restricted role ", smi_mstp_action},
      {"getrst", "\tGet the instance restricted role ", smi_mstp_action},
      {"setrstcn", "Set/unset the instance restricted TCN ", smi_mstp_action},
      {"getrstcn", "Get the instance restricted TCN ", smi_mstp_action},
      {"sethello", "Set the port hellotime", smi_mstp_action},
      {"gethello", "Get the port hellotime", smi_mstp_action},
      {"setp2p", "\tSet the Link Type", smi_mstp_action},
      {"getp2p", "\tGet the Link Type", smi_mstp_action},
      {"setcost", "\tSet the CIST pathcost", smi_mstp_action},
      {"getcost", "\tGet the CIST pathcost", smi_mstp_action},
      {"setmhops", "Set MSTP max hops", smi_mstp_action},
      {"getmhops", "Get MSTP max hops", smi_mstp_action},
      {"setrev", "\tSet revision number", smi_mstp_action},
      {"getrev", "\tGet revision number", smi_mstp_action},
      {"setmstip", "Set MSTI priority", smi_mstp_action},
      {"getmstip", "Get MSTI priority", smi_mstp_action},
      {"seterrtime", "Set bridge erradisable timeoutint", smi_mstp_action},
      {"geterrtime", "Get bridge erradisable timeoutint", smi_mstp_action},
      {"seterrten", "Set bridge erradisable timeouten", smi_mstp_action},
      {"geterrten", "Get bridge erradisable timeouten", smi_mstp_action},
      {"setbrbpdug", "Set bridge portfast BPDUguard", smi_mstp_action},
      {"getbrbpdug", "Get bridge portfast BPDUguard", smi_mstp_action},
      {"settxcount", "Set bridge txholdcount", smi_mstp_action},
      {"gettxcount", "Get bridge txholdcount", smi_mstp_action},
      {"setpbpdug", "Set port BPDU guard", smi_mstp_action},
      {"getpbpdug", "Get port BPDU guard", smi_mstp_action},
      {"enablebr", "Enable bridge", smi_mstp_action},
      {"disablebr", "Disable bridge", smi_mstp_action},
      {"setpbpduf", "Set port BPDU filter", smi_mstp_action},
      {"getpbpduf", "Get port BPDU filter", smi_mstp_action},
      {"setprootg", "Set port root guard", smi_mstp_action},
      {"getprootg", "Get port root guard", smi_mstp_action},
      {"setprsttcn", "Set port restricted tcn", smi_mstp_action},
      {"getprsttcn", "Get port restricted tcn", smi_mstp_action},
      {"setprstrol", "Set port restricted role", smi_mstp_action},
      {"getprstrol", "Get port restricted role", smi_mstp_action},
      {"setppriority", "Set port priority", smi_mstp_action},
      {"getppriority", "Get port priority", smi_mstp_action},
      {"setautoeg", "Set MSTP Auto Edge", smi_mstp_action},
      {"getautoeg", "Get MSTP Auto Edge", smi_mstp_action},
      {"setregion", "Set MSTP region", smi_mstp_action},
      {"getregion", "Get MSTP region", smi_mstp_action},
      {"getspan", "Get spanning tree details", smi_mstp_action},
      {"getspanitf", "Get states of spanning tree", smi_mstp_action},
      {"getspanmst", "Get the filtering database value", smi_mstp_action},
      {"getspanconf", "Get the MSTP configuration info", smi_mstp_action},
      {"getstpmst", "Get stp details for each instance", smi_mstp_action},
      {"getstpmstpif", "Get stp details for each instance and if",
        smi_mstp_action},
      {"gettraffictbl", "Get data from traffic class table", smi_mstp_action},
      {"getusrpr", "Get User Priority Data", smi_mstp_action},
      {"addbridgemac", "Add a Static Mac Address", smi_mstp_action},
      {"delbridgemac", "Delete a Static Mac Address", smi_mstp_action},
      {"flushdynentry", "Flush all dynamic entries", smi_mstp_action},
      {"setbpdufltr", "Set MSTP BPDU Filter", smi_mstp_action},
      {"getbpdufltr", "Get MSTP BPDU Filter Status", smi_mstp_action},
      {"mcheck", "Clear the detected protocols", smi_mstp_action},
      {"brstatus", "Get bridge status", smi_mstp_action},
      {"dump", "\tDump configuration args", dumpargs},
      {"exit", "\tExit menu", exit_menu},
      {"quit", "\tQuit program", quit},
      { NULL, NULL, NULL}
 };

/* RMON related settings */
menuItem rmon_menu[]= {
      {"sethisintval","Set History Control Interval", smi_rmon_action},
      {"gethisintval","Get History Control Interval", smi_rmon_action},
      {"sethistowner","Set History Control Owner", smi_rmon_action},
      {"gethistowner","Get History Control Owner", smi_rmon_action},
      {"histindxrm", "Remove History Index", smi_rmon_action},
      {"setalarmint","Set Alarm Polling Interval", smi_rmon_action},
      {"getalarmint","Get Alarm Polling Interval", smi_rmon_action},
      {"setalarmvar", "Set Alarm Variable", smi_rmon_action},
      {"getalarmvar", "Get Alarm Variable", smi_rmon_action},
      {"setalarmtype","Set Alarm Type", smi_rmon_action},
      {"getalarmtype","Get Alarm Type", smi_rmon_action},
      {"setalarmstartup", "Set Alarm Startup", smi_rmon_action},
      {"getalarmstartup", "Get Alarm Startup", smi_rmon_action},
      {"setriseths", "Set Alarm Rising Threshold", smi_rmon_action},
      {"getriseths", "Get Alarm Rising Threshold", smi_rmon_action},
      {"setfallths", "Set Falling Threshold for Alarms", smi_rmon_action},
      {"getfallths", "Get Rising Threshold for Alarms", smi_rmon_action},
      {"seteventrisths", "Set Event for Rising Threshold Alarms", 
      smi_rmon_action},
      {"getevenrisths", "Get Event for Rising Threshold for Alarms", 
      smi_rmon_action},
      {"seteventfallths", "Set Event for Falling Threshold Alarms", 
      smi_rmon_action},
      {"geteventfallths", "Get Event for Rising Threshold Alarms", 
      smi_rmon_action},
      {"setalarmowner", "Set Alarm Owner", smi_rmon_action},
      {"getalarmowner", "Get Alarm Owner", smi_rmon_action},
      {"setalarmentry", "Set Alarm entry", smi_rmon_action},
      {"getalarmentry", "Get Alarm entry", smi_rmon_action},
      {"setalarmindxrm", "Alarm Indx Remove", smi_rmon_action},
      {"setalarmstatus", "Alarm Index Status Set",smi_rmon_action},
      {"seteventindxrm", "Event Index Remove", smi_rmon_action},
      {"seteventindx", "Set Event Index", smi_rmon_action},
      {"geteventindx", "Get Event Index", smi_rmon_action},
      {"seteventactive", "Set Event Active", smi_rmon_action},
      {"seteventstatus", "set the Status for Event", smi_rmon_action},
      {"geteventstatus", "Get Event Status", smi_rmon_action},
      {"seteventcomm", "Set Event Community", smi_rmon_action},
      {"geteventcomm", "Get Event community", smi_rmon_action},
      {"seteventdesc", "Set Event Description", smi_rmon_action},
      {"geteventdesc", "Get Event Description", smi_rmon_action},
      {"seteventowner", "Set Event Owner", smi_rmon_action},
      {"geteventowner", "Get Event Owner", smi_rmon_action},
      {"validstats", "Validate the collection on Interface", smi_rmon_action},
      {"addstats", "Add a collection stat entry", smi_rmon_action},
      {"removestat", "Remove collection stat entry", smi_rmon_action},
      {"validhist", "Check History Parameters", smi_rmon_action},
      {"sethistory", "Set the history control entry", smi_rmon_action},
      {"gethistory", "Get the history control entry", smi_rmon_action},
      {"setbucket", "Set bucket for history control entry", smi_rmon_action},
      {"getbucket", "Get bucket history control entry", smi_rmon_action},
      {"sethisctrl", "Set the history control entry to inactive state", 
      smi_rmon_action},
      {"addhisctrl", "Add the history control entry", smi_rmon_action},
      {"setdsource", "Set the Datasource", smi_rmon_action},
      {"sethisindex", "Set the history control entry", smi_rmon_action},
      {"gethisindex", "Get the history control entry", smi_rmon_action},
      {"setevent", "Set the event type", smi_rmon_action},
      {"getevent", "Get the event type", smi_rmon_action},
      {"setsevent", "Set the snmp event type", smi_rmon_action},
      {"getsevent", "Get the snmp event type", smi_rmon_action},
      {"setscomm", "Set the community name for the event", smi_rmon_action},
      {"getscomm", "Get the community name", smi_rmon_action},
      {"setsowner", "Set the owner name for the event", smi_rmon_action},
      {"getsowner", "Get the owner  name", smi_rmon_action},
      {"setether", "Set the ether stat status", smi_rmon_action},
      {"getether", "Get the ether stat status", smi_rmon_action},
      {"setdesc", "\tSet the Description", smi_rmon_action},
      {"getdesc", "\tGet the Description", smi_rmon_action},
      {"getifstats", "Get If Stats", smi_rmon_action},
      {"getifcount", "Get the counter detail", smi_rmon_action},
      {"flushport", "Flush ports", smi_rmon_action},
      {"flushallport", "Flush all ports", smi_rmon_action},
      {"dump", "\tDump configuration args", dumpargs},
      {"exit", "\tExit menu", exit_menu},
      {"quit", "\tQuit program", quit},
      { NULL, NULL, NULL}
 };

/* LACP related settings */
menuItem lacp_menu[]= {
      {"addlink", "\tAdd LACP link", smi_lacp_action},
      {"dellink", "\tDelete LACP link", smi_lacp_action},
      {"getchannelact", "\tGet link Aggregration status", smi_lacp_action},
      {"getchannelkey", "\tGet channel Key", smi_lacp_action},
      {"setchannelpr", "\tSet channel priority", smi_lacp_action},
      {"getchannelpr", "\tGet channel priority", smi_lacp_action},
      {"unsetchannelpr", "\tunset channel priority", smi_lacp_action},
      {"setchtimeout", "\tSet channel timeout", smi_lacp_action},
      {"getchtimeout", "\tGet channel timeout", smi_lacp_action},
      {"setsyspriority", "\tSet system priority", smi_lacp_action},
      {"getsyspriority", "\tGet system priority", smi_lacp_action},
      {"unsetsyspriority", "\tunset sysytem priority", smi_lacp_action},
      {"getchdetail", "\tGet ether channel detail", smi_lacp_action},
      {"getchsummary", "\tGet ether channel summary", smi_lacp_action},
      {"getcounter", "\tGet LACP counter", smi_lacp_action},
      {"getsysid", "\tGet system id", smi_lacp_action},
      {"dump", "\tDump configuration args", dumpargs},
      {"exit", "\tExit menu", exit_menu},
      {"quit", "\tQuit program", quit},
      { NULL, NULL, NULL}
 };

/* LLDP related settings */
menuItem lldp_menu[]= {
      {"disableport", "Diasable LLDP", smi_lldp_action},
      {"enableport", "enable LLDP", smi_lldp_action},
      {"setlstring", "set locally assigned string", smi_lldp_action},
      {"getlstring", "Get locally assigned string", smi_lldp_action},
      {"setporttlv", "Set TLVs to be enabled for transmission",smi_lldp_action},
      {"getporttlv", "Get TLVs to be enabled for transmission",smi_lldp_action},
      {"setptxhold", "Set Message Transmit Hold parameter ", smi_lldp_action},
      {"getptxhold", "Get Message Transmit Hold parameter ", smi_lldp_action},
      {"settxinterval", "Set message transmit interval ", smi_lldp_action},
      {"gettxinterval", "Get message transmit interval ", smi_lldp_action},
      {"setreinitdelay", "Set the delay time", smi_lldp_action},
      {"getreinitdelay", "Get the delay time", smi_lldp_action},
      {"setneighbr", "Set upper limit for Too Many Neighbors", smi_lldp_action},
      {"getneighbr", "Get upper limit for Too Many Neighbors", smi_lldp_action},
      {"settxdelay", "Set delay between successive tx of LLDP frames", 
       smi_lldp_action},
      {"gettxdelay", "Get delay between successive tx of LLDP frames", 
       smi_lldp_action},
      {"setsysdescr", "Set string that describes the system", smi_lldp_action},
      {"getsysdescr", "Get string that describes the system", smi_lldp_action},
      {"setsysname", "Set the system name", smi_lldp_action},
      {"getsysname", "Get the system name", smi_lldp_action},
      {"getremmac", "Get mac addr of remote nbrs on an int", smi_lldp_action},
      {"getport", "\tGet port information of remote port", smi_lldp_action},
      {"getportstat", "Get statistics on a given port", smi_lldp_action},
      {"sethwaddr", "Set reserved mac address for lldp ctrl packets", 
       smi_lldp_action},
      {"gethwaddr", "Get reserved mac address for lldp ctrl packets", 
       smi_lldp_action},
      {"setchassisidtype", "Set chassis id type", smi_lldp_action},
      {"getchassisidtype", "Get chassis id type", smi_lldp_action},
      {"setchassisip", "Set chassis ip ", smi_lldp_action},
      {"getchassisip", "Get chassis ip ", smi_lldp_action},
      {"dump", "\tDump configuration args", dumpargs},
      {"exit", "\tExit menu", exit_menu},
      {"quit", "\tQuit program", quit},
      { NULL, NULL, NULL}
 };

/* CFM related settings */
menuItem cfm_menu[]= {
      {"addma", "add a maintenance association (MA)", smi_cfm_action},
      {"addmd", "add a maintenance domain (MD)", smi_cfm_action},
      {"addmep", "create a maintenance end point (MEP)", smi_cfm_action},
      {"addmip", "add a maintenance intermediate point (MIP)", smi_cfm_action},
      {"addrmep", "add static configuration for remote MEP(RMEP)", smi_cfm_action},
      {"enablecc", "enable a continuity check (CC)", smi_cfm_action},
      {"getma", "Get specific maintenance association and VLAN ID", smi_cfm_action},
      {"getmd", "Get maintenance domain with particular level", smi_cfm_action},
      {"getmep", "Get specific MEP in particular level on selected VLAN", smi_cfm_action},
      {"removema", "remove a MA from a specific VLAN", smi_cfm_action},
      {"removemd", "remove a MD from a CFM-enabled bridge.", smi_cfm_action},
      {"removemep", "remove MEP from specified level and VLAN", smi_cfm_action},
      {"sendping", "send loopback messages (pings) to remote MEP", smi_cfm_action},
      {"sendtrace", "sending traceroute messages to a RMEP", smi_cfm_action},
      {"getitmep", "get all the MEPs configured on interface", smi_cfm_action},
      {"getitrmep", "get all RMEPs configured on interface", smi_cfm_action},
      {"getittrace", "get all entries in the traceroute cache", smi_cfm_action},
      {"geterror", "Get the  CFM errors for a doamin and a VID", smi_cfm_action},
      {"clearerror", "Get the Error Cleared for the Maintainence Domain ", smi_cfm_action},
      {"getclearrmep", "clear CFM errors related to remote MEPs", smi_cfm_action},
      {"sethwaddr", "Set the reserved mac address", smi_cfm_action},
      {"gethwaddr", "Get the reserved mac address", smi_cfm_action},
      {"setethtype", "Set ether type for cfm control packet", smi_cfm_action},
      {"getethtype", "Get ether type for cfm control packet", smi_cfm_action},
      {"getrmep", "Get state of specific RMEP on particular local mep", smi_cfm_action},
      {"dump", "\tDump configuration args", dumpargs},
      {"exit", "\tExit menu", exit_menu},
      {"quit", "\tQuit program", quit},
      { NULL, NULL, NULL}
 };

/* GvRP related settings */
menuItem gvrp_menu[]= {
      {"settimer", "Set timer", smi_gvrp_action},
      {"gettimer", "Get timer", smi_gvrp_action},
      {"enable", "\tEnable gvrp", smi_gvrp_action},
      {"disable", "\tDisable gvrp", smi_gvrp_action},
      {"enableport", "Enable gvrp on port", smi_gvrp_action},
      {"disableport", "Disable gvrp on port", smi_gvrp_action},
      {"setregmode", "Set registration mode", smi_gvrp_action},
      {"getregmode", "Get registration mode", smi_gvrp_action},
      {"getvlanstats", "Get Per Vlan Stats", smi_gvrp_action},
      {"clrallstats", "Get Per Vlan Stats", smi_gvrp_action},
      {"dynvlanlrn", "Set dynamic vlan learning", smi_gvrp_action},
      {"getbrconf", "Get bridge configuration", smi_gvrp_action},
      {"getviddetail", "Get VID details", smi_gvrp_action},
      {"getportstats", "Get summry of bridge", smi_gvrp_action},
      {"gettimerdetails", "Get all timer details", smi_gvrp_action},
      {"dump", "\tDump configuration args", dumpargs},
      {"exit", "\tExit menu", exit_menu},
      {"quit", "\tQuit program", quit},
      { NULL, NULL, NULL}
 };

/* EFM related settings */
menuItem efm_menu[]= {
      {"disableproto", "Diasable Ethernet OAM", smi_efm_action},
      {"enableproto", "Enable Ethernet OAM", smi_efm_action},
      {"setlinktimer", "Set OAM Ethernet Link Timer", smi_efm_action},
      {"getlinktimer", "Get OAM Ethernet Link Timer", smi_efm_action},
      {"startremotelb", "Start Remote Loop Back", smi_efm_action},
      {"stopremotelb", "Stop Remote Loop Back", smi_efm_action},
      {"setmodeactive", "Set Ethernet OAM mode to ActiveState", smi_efm_action},
      {"setmodepassive", "Set Ethernet OAM mode to Passive", smi_efm_action},
      {"getmode", "\tGet Ethernet OAM Mode ", smi_efm_action},
      {"setpdutimer", "Set Ethernet OAM PDU timer", smi_efm_action},
      {"getpdutimer", "Get Ethernet OAM PDU timer", smi_efm_action},
      {"setmaxrate", "Set Maximum PDU transmission rate", smi_efm_action},
      {"getmaxrate", "Get Maximum PDU transmission rate", smi_efm_action},
      {"setlinkmonitor", "Set Link Monitor Support", smi_efm_action},
      {"getlinkmonitor", "Get Link Monitor Support", smi_efm_action},
      {"setrmloopback", "Set Remote loopback", smi_efm_action},
      {"getrmloopback", "Get Remote loopback", smi_efm_action},
      {"setrmlbtimeout", "Set Remote loopback Timeout", smi_efm_action},
      {"getrmlbtimeout", "Get Remote loopback Timeout", smi_efm_action},
      {"setlowthres", "Set Low Threshold val for Err Frames",
        smi_efm_action},
      {"getlowthres", "Get Low Threshold value for error Frames",
        smi_efm_action},
      {"sethighthres", "Set High Threshold value for error Frames",
        smi_efm_action},
      {"gethighthres", "Get High Threshold value for error Frames",
        smi_efm_action},
      {"setwinthres", "Set Window Period for error Frames",
        smi_efm_action},
      {"getwinthres", "Get Window Period for error Frames",
        smi_efm_action},
      {"setseclowthres", "Set Second Low Threshold val for error Frames",
        smi_efm_action},
      {"getseclowthres", "Get Second Low Threshold value for error Frames",
        smi_efm_action},
      {"setsechighthres", "Set Second High Threshold value for error Frames",
        smi_efm_action},
      {"getsechighthres", "Get Second High Threshold value for error Frames",
        smi_efm_action},
      {"setdisableifevt", "Set events that disable if on errs", smi_efm_action},
      {"getifeventst", "Get EFM Interface Event State", smi_efm_action},
      {"getoamstats", "Show  OAM Statstistics", smi_efm_action},
      {"getifstatus", "Show  if Status", smi_efm_action},
      {"getdiscoverst", "Get Discovery State machine state", smi_efm_action},
      {"getethernet", "Get ethernet details", smi_efm_action},
      {"senddataframe", "Send a data frame on a interface", smi_efm_action},
      {"getlpbkstatus", "Get loopback status", smi_efm_action},
      {"dump", "\tDump configuration args", dumpargs},
      {"exit", "\tExit menu", exit_menu},
      {"quit", "\tQuit program", quit},
      { NULL, NULL, NULL}
 };

/* QOS related settings */
menuItem qos_menu[]= {
      {"englobal", "enable the qos globally", smi_qos_action},
      {"disglobal", "disable the qos globally", smi_qos_action},
      {"getglobal", "get the status of qos globally", smi_qos_action},
      {"setpmap", "insert a Policy in the Policy map", smi_qos_action},
      {"getpmap", "Get the all the Policy map names", smi_qos_action},
      {"getpolicy", "get the Policy map related information", smi_qos_action},
      {"delpmap", "delete Policy from the policy map table", smi_qos_action},
      {"setcmap", "create the Class Map ", smi_qos_action},
      {"getcmap", "check whether the Class Name exist", smi_qos_action},
      {"delcmap", "delete class Name from the class map list", smi_qos_action},
      {"settraffic", "set the Matching type for Class Map", smi_qos_action},
      {"gettraffic", "Get the Matching traffic type for The class name", smi_qos_action},  
      {"unsettraffic", "unset the Matching type for Class Map", smi_qos_action},
      {"setpmapc", "set the policing parameters in policy map", smi_qos_action},
      {"getpmapc", "Get the Policy related data for a class Map", smi_qos_action},
      {"delpmapc", "delete policing parameters in policy map", smi_qos_action},
      {"delcmapfrompmap", "Delete the class-map from policy-map", smi_qos_action},
      {"setportsched", "sets the port scheduling mode", smi_qos_action},
      {"getportsched", "gets the port scheduling mode", smi_qos_action},
      {"setuserp", "set default user priority with interface", smi_qos_action},
      {"getuserp", "get default user priority with interface", smi_qos_action},
      {"setregenp", "Set default user priority ", smi_qos_action},
      {"getregenp", "Get default user priority ", smi_qos_action},
      {"costoq", "configure the COS to queue mapping", smi_qos_action},
      {"dscptoq", " configure the IP DSCP to queue mapping", smi_qos_action},
      {"settrust", "configure the trust state of interface", smi_qos_action},
      {"setforcetrust", "configure behaviour when trust state is set ", smi_qos_action},
      {"setframep", "override the queue assignement", smi_qos_action},
      {"setvlanp", "set the priority associated with frames ", smi_qos_action},
      {"getvlanp", "get the priority associated with frames ", smi_qos_action},
      {"unsetvlanp", "unset priority associated with frames ", smi_qos_action},
      {"setpvlanp", "configure port’s vlan priority override mode ", smi_qos_action},
      {"setqweight", "set the weights for WRR", smi_qos_action},
      {"getqweight", "get the weights for WRR", smi_qos_action},
      {"setpservice", "attach a policy map to a interface", smi_qos_action},
      {"getpservice", "Get policy mapped to a interface", smi_qos_action},
      {"unsetpservice", "Get policy mapped to a interface", smi_qos_action},
      {"settrafficshape", "Set traffic shape on interface", smi_qos_action},
      {"gettrafficshape", "Set traffic shape on interface", smi_qos_action},
      {"unsettrafficshape", "Set traffic shape on interface", smi_qos_action},
      {"getcostoq", "Get COS to queue mapping", smi_qos_action},
      {"getdscptoq", "Get dscp to queue mapping", smi_qos_action},
      {"gettrust", "Get the trust state of interface", smi_qos_action},
      {"getpvlanp", "get port’s vlan priority override mode ", smi_qos_action},
      {"getforcetrust", "get behaviour when trust state is set ", smi_qos_action},
      {"dump", "\tDump configuration args", dumpargs},
      {"exit", "\tExit menu", exit_menu},
      {"quit", "\tQuit program", quit},
      { NULL, NULL, NULL}
 };

/* Fc related settings */
menuItem fc_menu[]= {
      {"addfc", "set the 802.3x flow control feature of port", smi_fc_action},
      {"delfc", "turns off the flow control on the interface", smi_fc_action},
      {"statfc", "gets the flow control statistics for a port", smi_fc_action},
      {"getif", "get flow control information", smi_fc_action},
      {"dump", "\tDump configuration args", dumpargs},
      {"exit", "\tExit menu", exit_menu},
      {"quit", "\tQuit program", quit},
      { NULL, NULL, NULL}
 };


menuItem client_menu[]= {
      /* Stop API clients */
      {"stopnsm", "\tStop NSM Client", smi_client_action},
      {"stoplacp", "Stop LACP Client", smi_client_action},
      {"stopmstp", "Stop MSTP Client", smi_client_action},
      {"stoprmon", "Stop RMON Client", smi_client_action},
      {"stoponm", "Stop ONM Client", smi_client_action},
      /* Start API clients */
      {"startnsm", "Start NSM Client", smi_client_action},
      {"startlacp", "Start LACP Client", smi_client_action},
      {"startmstp", "Start MSTP Client", smi_client_action},
      {"startrmon", "Start RMON Client", smi_client_action},
      {"startonm", "Start ONM Client", smi_client_action},
      {"exit", "\tExit menu", exit_menu},
      {"quit", "\tQuit program", quit},
      { NULL, NULL, NULL}
  };

menuItem general_menu[]= {
      {"setportconf", "\tSet port conf", smi_general_action},
      {"getportconf", "\tGet port conf status", smi_general_action},
      {"setportlrn", "\tSet port learning", smi_general_action},
      {"getportlrn", "\tGet port learning", smi_general_action},
      {"setportnonsw", "\tSet port non-switch", smi_general_action},
      {"getportnonsw", "\tGet port non-switch status", smi_general_action},
      {"forcedfvlan", "\tForce default vlan", smi_general_action},
      {"setpreservececos", "Preserve Ce Cos", smi_general_action},
      {"setportbasedvlan", "Set port based vlan", smi_general_action},
      {"setegressmode", "\tSet port egress mode", smi_general_action},
      {"setcpudefvid", "\tSet CPU port default vlanid", smi_general_action},
      {"setwsdefvid", "\tSet WS port default vlanid", smi_general_action},
      {"setcpuportbasedvlan", "Set CPU port based vlan", smi_general_action},
      {"setportethertype", "Set Port Ethertype", smi_general_action},
      {"setwaysideethtype", "Set Wayside PortEthertype", smi_general_action}, 
      {"setswreset", "\tSet Sw Reset", smi_general_action}, 
      {"getversion", "\tGet PacOS version", smi_general_action},
      {"switchover", "\tHA: switchover", smi_general_action},
      {"priovr1", "\tprio override ", smi_general_action},
      {"priovr2", "\tprio override ", smi_general_action},
      {"priovr3", "\tprio override ", smi_general_action},
      {"priovr4", "\tprio override ", smi_general_action},
      {"priovr5", "\tprio override ", smi_general_action},
      {"priovr6", "\tprio override ", smi_general_action},
      {"priovr7", "\tprio override ", smi_general_action},
      {"priovr8", "\tprio override ", smi_general_action},
      {"priovr9", "\tprio override ", smi_general_action},
      {"priovr10", "\tprio override ", smi_general_action},
      {"priovr11", "\tprio override ", smi_general_action},
      {"priovr13", "\tprio override ", smi_general_action},
      {"priovr14", "\tprio override ", smi_general_action},
      {"priovr15", "\tprio override ", smi_general_action},
      {"priovr16", "\tprio override ", smi_general_action},
      {"priovr17", "\tprio override ", smi_general_action},
      {"priovr18", "\tprio override ", smi_general_action},
      {"priovr19", "\tprio override ", smi_general_action},
      {"dapri1", "\tprio override ", smi_general_action},
      {"dapri2", "\tprio override ", smi_general_action},
      {"dapri3", "\tprio override ", smi_general_action},
      {"exit", "\t\tExit menu", exit_menu},
      {"quit", "\t\tQuit program", quit},
      { NULL, NULL, NULL}
  };
#endif /* TEST_AC */

struct smiclient_globals *azg;

/* API client creation */
int
smiclient_create (int debug, char *filename)
{
  unsigned int errflag = 0;

#ifdef TEST_AC
  char buf[256];
#endif /* TEST_AC */

  SMI_FN_ENTER ();

  snprintf(config_filename, sizeof(config_filename), DFLT_CONFIG_FILE);
  if(filename)
    snprintf(config_filename, sizeof(config_filename), filename);

  printf("\n\tStarting API Client/s...\n\n");

  azg = PacOS_smi_client_lib_create(PAL_TRUE);
  if(azg == NULL)
    SMI_FN_EXIT(SMI_ERROR);

#ifdef START_ALL
  /* Start All API client */
  PacOS_smi_client_create (azg, SMI_AC_ALL, debug);
#else
  /* Create NSM API client */
  PacOS_smi_client_create (azg, SMI_AC_NSM_MODULE, debug);
 
  /* Create LACP API client */
  PacOS_smi_client_create (azg, SMI_AC_LACP_MODULE, debug);

  /* Create MSTP API client */
  PacOS_smi_client_create (azg, SMI_AC_MSTP_MODULE, debug);

  /* Create RMON API client */
  PacOS_smi_client_create (azg, SMI_AC_RMON_MODULE, debug);

  /* Create ONM API client */
  PacOS_smi_client_create (azg, SMI_AC_ONM_MODULE, debug);
#endif

  printf("\n\tRegister alarm callback ..\n\n");
  PacOS_smi_client_set_alarm_callback (testapi_process_alarm_message);

  /* Start API Clients */
  errflag = PacOS_smi_client_start(azg);


#ifdef TEST_AC
  printf("\n");
  if(errflag & SMI_AC_NSM_INITERR)
    printf("\tError in initializing NSM API client\n\n");
  else 
    printf("\tNSM API client started...\n\n");

  if(errflag & SMI_AC_LACP_INITERR)
    printf("\tError in initializing LACP API client\n\n");
  else 
    printf("\tLACP API client started...\n\n");

  if(errflag & SMI_AC_MSTP_INITERR)
    printf("\tError in initializing MSTP API client\n\n");
  else 
    printf("\tMSTP API client started...\n\n");

  if(errflag & SMI_AC_RMON_INITERR)
    printf("\tError in initializing RMON API client\n\n");
  else
    printf("\tRMON API client started...\n\n");

  if(errflag & SMI_AC_ONM_INITERR)
    printf("\tError in initializing ONM API client\n\n");
  else
    printf("\tONM API client started...\n\n");


  while (1) 
    {
      printf("\n\t******Press any key to continue...******\n");
      fgets(buf, 2, stdin);
      printf("\n");
      printf("\t********************************************\n");
      printf("\t**********Configuration API*****************\n");
      printf("\t********************************************\n");
      printf("\n");
      printf("\nModules: [if] [vlan] [mstp] [lacp] [rmon] [lldp] \n");
      printf("\n         [gvrp] [efm] [cfm] [qos] [fc] [apiclient]\n");
      printf("\n         [general] \n");
      printf("\nEnter module to be configured or q to quit: ");
        fflush(stdout);
      if(fgets(buf, 256, stdin))
        buf[strlen(buf)-1] = '\0';
      else
        buf[0] = '\0';

      if(!strcmp(buf, "if"))
        show_module_menu(azg, if_menu);
      else if(!strcmp(buf, "apiclient"))
        show_module_menu(azg, client_menu);
      else if(!strcmp(buf, "vlan"))
        show_module_menu(azg, vlan_menu);
      else if(!strcmp(buf, "mstp"))
        show_module_menu(azg, mstp_menu);
      else if(!strcmp(buf, "rmon"))
        show_module_menu(azg, rmon_menu);
      else if(!strcmp(buf, "lacp"))
        show_module_menu(azg, lacp_menu);
      else if(!strcmp(buf, "lldp"))
        show_module_menu(azg, lldp_menu);
      else if(!strcmp(buf, "cfm"))
        show_module_menu(azg, cfm_menu);
      else if(!strcmp(buf, "efm"))
        show_module_menu(azg, efm_menu);
      else if(!strcmp(buf, "gvrp"))
        show_module_menu(azg, gvrp_menu);
      else if(!strcmp(buf, "qos"))
        show_module_menu(azg, qos_menu);
      else if(!strcmp(buf, "fc"))
        show_module_menu(azg, fc_menu);
      else if(!strcmp(buf, "general"))
        show_module_menu(azg, general_menu);
      else if(!strcasecmp(buf, "q"))
        exit_menu();
  }
#endif /* TEST_AC */

  SMI_FN_EXIT(SMI_SUCEESS);
}

/* API client creation */
int
smiclient_create1 (int debug, char *filename)
{
  unsigned int errflag = 0;

#ifdef TEST_AC
  char buf[256];
#endif /* TEST_AC */

  SMI_FN_ENTER ();

  snprintf(config_filename, sizeof(config_filename), DFLT_CONFIG_FILE);
  if(filename)
    snprintf(config_filename, sizeof(config_filename), filename);

  printf("\n\tStarting API Client/s...\n\n");

  azg = PacOS_smi_client_lib_create(PAL_TRUE);
  if(azg == NULL)
    SMI_FN_EXIT(SMI_ERROR);

#ifdef START_ALL
  /* Start All API client */
  PacOS_smi_client_create (azg, SMI_AC_ALL, debug);
#else
  /* Create NSM API client */
  PacOS_smi_client_create (azg, SMI_AC_NSM_MODULE, debug);
 
  /* Create LACP API client */
  PacOS_smi_client_create (azg, SMI_AC_LACP_MODULE, debug);

  /* Create MSTP API client */
  PacOS_smi_client_create (azg, SMI_AC_MSTP_MODULE, debug);

  /* Create RMON API client */
  PacOS_smi_client_create (azg, SMI_AC_RMON_MODULE, debug);

  /* Create ONM API client */
  PacOS_smi_client_create (azg, SMI_AC_ONM_MODULE, debug);
#endif

  printf("\n\tRegister alarm callback ..\n\n");
  PacOS_smi_client_set_alarm_callback (testapi_process_alarm_message);

  /* Start API Clients */
  errflag = PacOS_smi_client_start(azg);


  printf("\n");
  if(errflag & SMI_AC_NSM_INITERR)
    printf("\tError in initializing NSM API client\n\n");
  else 
    printf("\tNSM API client started...\n\n");

  if(errflag & SMI_AC_LACP_INITERR)
    printf("\tError in initializing LACP API client\n\n");
  else 
    printf("\tLACP API client started...\n\n");

  if(errflag & SMI_AC_MSTP_INITERR)
    printf("\tError in initializing MSTP API client\n\n");
  else 
    printf("\tMSTP API client started...\n\n");

  if(errflag & SMI_AC_RMON_INITERR)
    printf("\tError in initializing RMON API client\n\n");
  else
    printf("\tRMON API client started...\n\n");

  if(errflag & SMI_AC_ONM_INITERR)
    printf("\tError in initializing ONM API client\n\n");
  else
    printf("\tONM API client started...\n\n");

  SMI_FN_EXIT(SMI_SUCEESS);
}

#ifdef TEST_AC
int
exit_menu()
{
    exit(0);
}

int
quit()
{
    exit(0);
}

/* Interface related APIs */
int
smi_if_action(struct smiclient_globals *azg, char *action)
{
  int ret=0;
  int val=0;
    
  if(!strcasecmp(action, "setmtu"))
    {
      ret = smi_if_set_mtu(azg, vrid, ifname, mtuval);
      if(ret >= 0)
        {
          printf("\n\tMTU Set Successfully\n");
        } else {
          printf("\n\tMTU Setting Failed\n");
        }
    } else if (!strcasecmp(action, "getmtu"))
    {
      int mtu;
      ret = smi_if_get_mtu(azg, vrid, ifname, &mtu);
      if(mtu > 0)
        {
          printf("\n\tMTU returned: %d \n", mtu);
        } else {
          printf("\n\tMTU Getting Failed\n");
        }
    } else if(!strcasecmp(action, "setbw"))
    {
      ret = smi_if_set_bandwidth(azg, ifname, bwval);
      if(ret == 0)
        {
          printf("\n\tBandwidth Set Successfully\n");
        } else {
          printf("\n\tBandwidth Setting Failed\n");
        }
    } else if (!strcasecmp(action, "getbw"))
    {
      float bw;
      ret = smi_if_get_bandwidth(azg, ifname, &bw);
      if(ret == 0)
        {
          printf("\n\tBandwidth returned: %f\n", bw);
        } else {
          printf("\n\tBandwidth Getting Failed\n");
        }
    } else if(!strcasecmp(action, "setdplx"))
    {
      ret = smi_if_set_duplex(azg, vrid, ifname, dplxval);
      if(ret == 0)
        {
          printf("\n\tDuplex Set Successfully\n");
        } else {
          printf("\n\tDuplex Setting Failed: %d\n", ret);
        }
    } else if (!strcasecmp(action, "getdplx"))
    {
      ret = smi_if_get_duplex(azg, vrid, ifname, &val);
      if(ret == 0)
        {
          printf("\n\tDuplex returned: %d\n", val);
        } else {
          printf("\n\tDuplex Getting Failed\n");
        }
    } else if(!strcasecmp(action, "flgup"))
    {
      ret=smi_if_set_flagup(azg, vrid, ifname);
      if(ret==0)
        {
          printf("\n\tInterface Enabled Successfully\n");
        } else {
          printf("\n\tInterface Enabling Failed\n");
        }
    } else if(!strcasecmp(action, "flgdn"))
    {
      ret=smi_if_unset_flagup(azg, vrid, ifname);
      if(ret==0)
        {
          printf("\n\tInterface Disabled Successsfully\n");
        } else {
          printf("\n\tInterface Disabling Failed\n");
        }
     } else if(!strcasecmp(action, "setauto"))
     {
       ret=smi_if_set_autoneg(azg, ifname);
       if(ret==0)
         {
          printf("\n\tAuto-Negotiation set Successfully\n");
         } else {
           printf("\n\tAuto-Negotiation setting Failed\n");
         }
     } else if (!strcasecmp(action, "getauto"))
     {
       ret = smi_if_get_autoneg(azg, ifname, &val);
      if(ret == 0)
        {
          printf("\n\tauto neg : %d\n", val);
        } else {
          printf("\n\tauto neg disabled\n");
        }
    } else if(!strcasecmp(action, "sethwa"))
    {
      ret = smi_if_set_hwaddr(azg, ifname, hwaddr);
      if(ret == 0)
        {
          printf("\n\tHardware Address Set Successfully\n");
        } else {
          printf("\n\tHardware Address Setting Failed\n");
        }
    } else if (!strcasecmp(action, "gethwa"))
    {
      unsigned char hw_addr[SMI_ETHER_ADDR_LEN];
      ret = smi_if_get_hwaddr(azg, ifname, hw_addr);
      if(ret == 0)
        {
          printf("\n\tHardware Address returned: %x%x",hw_addr[0],hw_addr[1]);
          printf(".%x%x",hw_addr[2],hw_addr[3]);
          printf(".%x%x\n",hw_addr[4],hw_addr[5]);
        } else {
          printf("\n\tHardware Address Getting Failed\n");
        }
    } else if(!strcasecmp(action, "getipb"))
    {
      struct smi_msg_if ipmsg;
      ret = smi_if_get_brief(azg, ifname, &ipmsg);
      if(ret == 0)
        {
          printf("\n\tInterface Name   : %s\n", ifname);
          printf("\n\tInterface status : %s\n", 
                 (ipmsg.flag == 0) ? "Down" : "Up");
          printf("\n\tProtocol         : %s\n", 
                 (ipmsg.if_status == 0) ? "Down" : "Up");
        } else {
          printf("\n\tInformation Getting Failed\n");
        }
    } else if(!strcasecmp(action, "setmcast"))
    {
      ret = smi_if_set_multicast(azg, vrid, ifname);
      if(ret == 0)
        {
          printf("\n\tInterface Set Successfully For Multicasting\n");
        } else {
          printf("\n\tInterface Setting Failed\n");
        }
    } else if(!strcasecmp(action, "getmcast"))
    {
      ret = smi_if_get_multicast(azg, vrid, ifname, &val);
      if(ret == 0)
        {
          printf("\n\tMCAST enabled-Flag Status returned: %d\n", val);
        } else {
          printf("\n\tInterface mcast getting Failed\n");
        }
    } else if(!strcasecmp(action, "unsetdp"))
    {
      ret = smi_if_unset_duplex(azg, vrid, ifname);
      if(ret == 0)
        {
         printf("\n\tDuplex: Auto-Negotiation set Successfully\n");
        } else {
          printf("\n\tDuplex: Auto-Negotiation setting Failed\n");
        }
     } else if(!strcasecmp(action, "unsetmt"))
     {
        ret = smi_if_unset_multicast(azg, vrid, ifname);
        if(ret==0)
         {
           printf("\n\tMulticast Disabled Successsfully\n");
         } else {
           printf("\n\tMulticast Disabling Failed\n");
         }
     } else if (!strcasecmp(action, "getchange"))
    {
      enum smi_if_link_changed flag;
      ret = smi_if_change_get(azg, ifname, &flag);
      if(ret >= 0)
        {
          if (flag == SMI_IF_LINK_CHANGED)
            printf("\n\t Change in link status since last poll\n");
          else
            printf("\n\t No change in link status since last poll\n");
        } else {
          printf("\n\tGetting Failed\n");
        }
     } else if(!strcasecmp(action, "getstat"))
    {
      struct smi_if_stats ipmsg;
      ret = smi_if_get_statistics(azg, ifname, &ipmsg);
      if(ret >= 0)
        {
          printf("\n\t successful\n");
        } else {
          printf("\n\tInformation Getting Failed\n");
        }
     } else if(!strcasecmp(action, "setmdix"))
     {
       ret = smi_if_set_mdix_crossover(azg, vrid, ifname, crossmode);
       if(ret == 0)
         {
          printf("\n\t set Successfully\n");
         } else {
           printf("\n\tsetting Failed\n");
         }
      } else if(!strcasecmp(action, "getmdix"))
      {
        enum smi_if_cross_over cross_mode;
        ret = smi_if_get_mdix_crossover(azg, vrid, ifname, &cross_mode);
        if(ret == 0)
          {
            printf("\n\t Returned MDIX Value: %d\n", cross_mode);
          } else {
            printf("\n\tgetting Failed\n");
          }
      }else if(!strcasecmp(action, "clearstat"))
      {
        ret = smi_if_clear_statistics (azg, ifname);
        if(ret == 0)
         {
          printf("\n\t statistics cleared Successfully\n");
         } else {
           printf("\n\tstatistics clearing Failed\n");
         }
      } else if (!strcasecmp (action, "getbridge"))
      {
        enum smi_bridge_type type;
        enum smi_bridge_topo_type topo_type;
        ret = smi_get_bridge_type (azg, brname, &type, &topo_type);
        if(ret >= 0)
        {
          printf("\n\t Bridge Get:\n");
          printf("\n\t\t Name: %s\n", brname);
          printf("\n\t\t Type: %d\n", type);
          printf("\n\t\t Topo type: %d\n", topo_type);
        } else {
          printf("\n\t Get bridge info Failed\n");
        }
      } else if(!strcasecmp(action, "addbridge"))
      {
        ret = smi_bridge_add(azg, brname, mstpbrtype, mstptopotype);
        if(ret == 0)
         {
           printf("\n\t added successfully\n");
         } else {
           printf("\n\tadding Failed\n");
         }
      } else if(!strcasecmp(action, "addbrport"))
      {
        ret = smi_bridge_port_add(azg, brname, ifname, PAL_TRUE);
        if(ret == 0)
          {
            printf("\n\t bridge port added Successfully\n");
          } else {
            printf("\n\t Setting Failed\n");
          }
      } else if(!strcasecmp(action, "mgmt"))
      {
        ret = smi_set_port_dot1q_state (azg, "fe6", PAL_FALSE);

         if ( ret < 0)
          printf ("\n\t Disabling dot1q on fe6 Failed \n");
         else
          printf ("\n\t Disabling dot1q on fe6 Success \n");

        ret = smi_set_port_dot1q_state (azg, "ge1", PAL_FALSE);

         if ( ret < 0)
          printf ("\n\t Disabling dot1q on ge1 Failed \n");
         else
          printf ("\n\t Disabling dot1q on ge1 Success \n");

        ret = smi_bridge_port_add (azg, brname, "fe6", PAL_TRUE);

        if(ret == 0)
          {
            printf("\n\t bridge port added Successfully\n");
          } else {
            printf("\n\t Setting Failed\n");
          }

        ret = smi_vlan_set_default_vid (azg, "fe6", 4);

        if(ret == 0)
        {
          printf("\n\t set default vlan id success\n");
        } else {
          printf("\n\t set default vlan id failed\n");
        }
         ret = smi_bridge_port_add(azg, brname, "ge1", PAL_TRUE);

        if(ret == 0)
          {
            printf("\n\t bridge port added Successfully\n");
          } else {
            printf("\n\t Setting Failed\n");
          }

        ret = smi_vlan_set_default_vid(azg, "ge1", 4);

        if(ret == 0)
        {
          printf("\n\t set default vlan id success\n");
        } else {
          printf("\n\t set default vlan id failed\n");
        }

        ret = smi_set_port_dot1q_state (azg, "fe6", PAL_TRUE);

         if ( ret < 0)
          printf ("\n\t Enabling dot1q on fe6 Failed \n");
         else
          printf ("\n\t Enabling dot1q on fe6 Success \n");

        ret = smi_set_port_dot1q_state (azg, "ge1", PAL_TRUE);

         if ( ret < 0)
          printf ("\n\t Disabling dot1q on ge1 Failed \n");
         else
          printf ("\n\t Disabling dot1q on ge1 Success \n");

       } else if(!strcasecmp(action, "changetype"))
      {
        ret = smi_bridge_change_type(azg, brname, mstpbrtype, mstptopotype);
        if(ret == 0)
          {
            printf("\n\t changed Successfully\n");
          } else {
            printf("\n\t changing Failed\n");
          }
      } else if (!strcasecmp (action, "delbridge"))
      {
        ret = smi_bridge_del (azg, brname);
        if (ret < 0)
          printf ("\n\t Delete bridge failed\n");
        else
          printf ("\n\t Delete bridge success\n");
      } else if (!strcasecmp (action, "delbridgeport"))
      {
        ret = smi_bridge_port_del (azg, brname, ifname);
        if (ret < 0)
          printf ("\n\t Delete bridge-port %s failed\n", ifname);
        else
          printf ("\n\t Delete bridge-port %s success\n", ifname);
      } else if(!strcasecmp(action, "chagetometromgmt"))
      {
        ret = smi_set_port_dot1q_state (azg, "fe6", PAL_FALSE);

         if ( ret < 0)
          printf ("\n\t Disabling dot1q on fe6 Failed \n");
         else
          printf ("\n\t Disabling dot1q on fe6 Success \n");

        ret = smi_set_port_dot1q_state (azg, "ge1", PAL_FALSE);

         if ( ret < 0)
          printf ("\n\t Disabling dot1q on ge1 Failed \n");
         else
          printf ("\n\t Disabling dot1q on ge1 Success \n");

        ret = smi_bridge_change_type (azg, brname, SMI_BRIDGE_PROVIDER_RSTP,
                                      SMI_TOPO_NONE);

        if(ret == 0)
          {
            printf("\n\t bridge change type Success \n");
          } else {
            printf("\n\t Setting Failed\n");
          }

        ret = smi_vlan_add (azg, brname, "svlan4", 4, SMI_VLAN_ACTIVE,
                            VLAN_SVLAN);

        if(ret == 0)
          {
            printf("\n\t VLAN Add type Success \n");
          } else {
            printf("\n\t Setting Failed\n");
          }

        ret = smi_vlan_api_set_port_mode (azg, "fe6", SMI_VLAN_PORT_MODE_CN,
                                         SMI_VLAN_PORT_MODE_CN);

        if(ret == 0)
        {
          printf("\n\t set Port Mode success\n");
        } else {
          printf("\n\t set Port Mode failed\n");
        }

        ret = smi_vlan_api_set_port_mode (azg, "ge1", SMI_VLAN_PORT_MODE_CN,
                                         SMI_VLAN_PORT_MODE_CN);

        if(ret == 0)
        {
          printf("\n\t set Port Mode success\n");
        } else {
          printf("\n\t set Port Mode failed\n");
        }

        ret = smi_vlan_set_default_vid (azg, "fe6", 4);

        if(ret == 0)
        {
          printf("\n\t set default vlan id success\n");
        } else {
          printf("\n\t set default vlan id failed\n");
        }

        ret = smi_vlan_set_default_vid(azg, "ge1", 4);

        if(ret == 0)
        {
          printf("\n\t set default vlan id success\n");
        } else {
          printf("\n\t set default vlan id failed\n");
        }

        ret = smi_set_port_dot1q_state (azg, "fe6", PAL_TRUE);

         if ( ret < 0)
          printf ("\n\t Enabling dot1q on fe6 Failed \n");
         else
          printf ("\n\t Enabling dot1q on fe6 Success \n");

        ret = smi_set_port_dot1q_state (azg, "ge1", PAL_TRUE);

         if ( ret < 0)
          printf ("\n\t Disabling dot1q on ge1 Failed \n");
         else
          printf ("\n\t Disabling dot1q on ge1 Success \n");

       } else if(!strcasecmp(action, "changetype"))
      {
        ret = smi_bridge_change_type(azg, brname, mstpbrtype, mstptopotype);
        if(ret == 0)
          {
            printf("\n\t changed Successfully\n");
          } else {
            printf("\n\t changing Failed\n");
          }
      } else if(!strcasecmp(action, "disabledot1q"))
      {
        struct smi_port_bmp port_bmp;

        SMI_BMP_INIT (port_bmp);
        SMI_BMP_SET (port_bmp, 1);
        SMI_BMP_SET (port_bmp, 2);

        ret = smi_set_port_based_vlan (azg, "fe3", &port_bmp);

        if (ret < 0)
          printf("\n\tError in setting port based vlan\n");
        else
          printf("\n\tSetting port based vlan success\n");

        ret = smi_set_port_dot1q_state (azg, "fe3", PAL_FALSE);

        if ( ret < 0)
          printf ("\n\t Disabling dot1q on fe3 Failed \n");
        else
          printf ("\n\t Disabling dot1q on fe3 Success \n");

      } else if(!strcasecmp(action, "flgget"))
      {
        u_int32_t flags = 0;
        ret=smi_if_get_flags(azg, ifname, &flags);
        if(ret==0)
        {
          printf("\n\tInterface Flags get success\n");
          if (SMI_CHECK_FLAG (flags, IFF_UP))
            printf("\n\tInterface %s: UP\n", ifname);
          else
            printf("\n\tInterface %s: DOWN\n", ifname);
          if (SMI_CHECK_FLAG (flags, IFF_RUNNING))
            printf("\n\tInterface %s: RUNNING\n", ifname);
          else
            printf("\n\tInterface %s: Not RUNNING\n", ifname);
        } else {
          printf("\n\tInterface Flag get Failed\n");
        }
      }else if (!strcasecmp (action, "addPortToChannel"))
      {
        ret = smi_lacp_add_link (azg, ifname, ifLacpMode, ifAdminKey);
         if ( ret < 0)
          printf ("\n\t Adding to port %s to Channel Group %d Failed \n",ifname,ifAdminKey);
         else
         printf ("\n\t Adding to port %s to Channel Group %d Success \n",ifname,ifAdminKey);
      } else if (!strcasecmp (action, "deleteFromChannel"))
         {
           ret = smi_lacp_delete_link (azg, ifname);
           if (ret < 0)
             printf ("\n\t Deleting the Interface %s from Channel Failed \n",ifname);
           else
             printf ("\n\t Deleting the Interface %s from Channel Successful \n",ifname);
         } 
      else if (!strcasecmp (action, "dtagset"))
      {
        ret = smi_set_port_dot1q_state (azg, "fe6", PAL_FALSE);
         if ( ret < 0)
          printf ("\n\t Adding to port %s to Channel Group %d Failed \n",ifname,ifAdminKey);
         else
         printf ("\n\t Adding to port %s to Channel Group %d Success \n",ifname,ifAdminKey);
      }

  return 0;
}

/* API client related APIs */
int
smi_client_action (struct smiclient_globals *azg, char *action)
{
  int ret = 0;

  if(!strcasecmp(action, "stopnsm"))
    {
      if(SMI_AC_NSM)
      {
        PacOS_smi_client_stop(azg, SMI_AC_NSM_MODULE);
        PacOS_smi_client_delete(&azg, SMI_AC_NSM_MODULE);
        printf("\n\tNSM Client stopped\n");
      }
      else
      {
        printf("\n\tNSM Client not running\n");
     }
   } else if(!strcasecmp(action, "startnsm"))
   {
      PacOS_smi_client_create(azg, SMI_AC_NSM_MODULE, SMI_AC_DEBUG);
      ret = PacOS_smi_client_start(azg);
      if(ret < 0)
        printf("\n\tError in initializing NSM Client\n");
      else
        printf("\n\tNSM Client started...\n");
   } else if(!strcasecmp(action, "stoplacp"))
   {
      if(SMI_AC_LACP)
      {
        PacOS_smi_client_stop(azg, SMI_AC_LACP_MODULE);
        PacOS_smi_client_delete(&azg, SMI_AC_LACP_MODULE);
        printf("\n\tLACP Client stopped\n");
      }
      else
      {
        printf("\n\tLACP Client not running\n");
      }
    } else if(!strcasecmp(action, "stopmstp"))
    {
      if(SMI_AC_MSTP)
      {
        PacOS_smi_client_stop(azg, SMI_AC_MSTP_MODULE);
        PacOS_smi_client_delete(&azg, SMI_AC_MSTP_MODULE);
        printf("\n\tMSTP Client stopped\n");
      }
      else
      {
        printf("\n\tMSTP Client not running\n");
      }
    } else if(!strcasecmp(action, "stoprmon"))
    {
      if(SMI_AC_RMON)
      {
        PacOS_smi_client_stop(azg, SMI_AC_RMON_MODULE);
        PacOS_smi_client_delete(&azg, SMI_AC_RMON_MODULE);
        printf("\n\tRMON Client stopped\n");
      }
      else
      {
        printf("\n\tRMON Client not running\n");
      }
    } else if(!strcasecmp(action, "stoponm"))
    {
      if(SMI_AC_ONM)
      {
        PacOS_smi_client_stop(azg, SMI_AC_ONM_MODULE);
        PacOS_smi_client_delete(&azg, SMI_AC_ONM_MODULE);
        printf("\n\tONM Client stopped\n");
      }
      else
      {
        printf("\n\tONM Client not running\n");
      }
    } else if(!strcasecmp(action, "startlacp"))
    {
      PacOS_smi_client_create(azg, SMI_AC_LACP_MODULE, SMI_AC_DEBUG);
      ret = PacOS_smi_client_start(azg);
      if(ret < 0)
        printf("\n\tError in initializing LACP Client\n");
      else
        printf("\n\tLACP Client started...\n");
    } else if(!strcasecmp(action, "startmstp"))
    {
      PacOS_smi_client_create(azg, SMI_AC_MSTP_MODULE, SMI_AC_DEBUG);
      ret = PacOS_smi_client_start(azg);
      if(ret < 0)
        printf("\n\tError in initializing MSTP Client\n");
      else
        printf("\n\tMSTP Client started...\n");
    } else if(!strcasecmp(action, "startrmon"))
    {
      PacOS_smi_client_create(azg, SMI_AC_RMON_MODULE, SMI_AC_DEBUG);
      ret = PacOS_smi_client_start(azg);
      if(ret < 0)
        printf("\n\tError in initializing RMON Client\n");
      else
        printf("\n\tRMON Client started...\n");
    } else if(!strcasecmp(action, "startonm"))
    {
      PacOS_smi_client_create(azg, SMI_AC_ONM_MODULE, SMI_AC_DEBUG);
      ret = PacOS_smi_client_start(azg);
      if(ret < 0)
        printf("\n\tError in initializing ONM Client\n");
      else
        printf("\n\tONM Client started...\n");
    }
  return 0;
}

/* LACP related APIs */
int
smi_lacp_action (struct smiclient_globals *azg, char *action)
{
  int ret=0;

  if(!strcasecmp(action, "addlink"))
    {
    } else if(!strcasecmp(action, "getchannelact"))
    {
       enum smi_lacp_mode mode;     
       ret = smi_lacp_get_channelactivity(azg, ifname, &mode);
      if(ret >= 0)
        {
          printf("\n\t  Successful: Link Aggregration status : %d\n", mode);
        } else {
          printf("\n\t Failed\n");
        }
    } else if(!strcasecmp(action, "getchannelkey"))
    {
      unsigned int key;
      ret = smi_lacp_get_channeladminkey(azg, linkname, &key);
      if(ret >= 0)
        {
          printf("\n\tSuccessful : Admin key %d\n", key);
        } else {
          printf("\n\t Failed\n");
        }
    } else if(!strcasecmp(action, "setchannelpr"))
    {
      ret = smi_lacp_set_channelpriority(azg, linkname, priority);
      if(ret == 0)
        {
          printf("\n\tchannel priority set Successfully \n");
        } else {
          printf("\n\t channel priority setting Failed\n");
        }
    } else if(!strcasecmp(action, "getchannelpr"))
    {
      unsigned int pr;
      ret = smi_lacp_get_channelpriority(azg, linkname, &pr);
      if(ret >= 0)
        {
          printf("\n\t Successfull: Channel priority %d\n", pr);
        } else {
          printf("\n\t Failed\n");
        }
     } else if(!strcasecmp(action, "unsetchannelpr"))
    {
      ret = smi_lacp_unset_channelpriority(azg, linkname);
      if(ret == 0)
        {
          printf("\n\t channel priority unset Successfully \n");
        } else {
          printf("\n\t Failed\n");
        }
     } else if(!strcasecmp(action, "setchtimeout"))
    {
      ret = smi_lacp_set_channeltimeout(azg, linkname, 3);
      if(ret == 0)
        {
          printf("\n\tChannel timeout set Successfully \n");
        } else {
          printf("\n\t Channel timeout setting Failed\n");
        }
    } else if(!strcasecmp(action, "getchtimeout"))
    {
      int time;
      ret = smi_lacp_get_channeltimeout(azg, linkname, &time);
      if(ret >= 0)
        {
          printf("\n\t Successful: Timeout %d\n", time);
        } else {
          printf("\n\t Failed\n");
        }
     } else if(!strcasecmp(action, "setsyspriority"))
    {
      ret = smi_lacp_set_systempriority(azg, priority);
      if(ret == 0)
        {
          printf("\n\t System priority set Successfully \n");
        } else {
          printf("\n\t System priority setting Failed\n");
        }
    } else if(!strcasecmp(action, "unsetsyspriority"))
    {
      ret = smi_lacp_unset_systempriority(azg);
      if(ret == 0)
        {
          printf("\n\t System priority unset Successfully \n");
        } else {
          printf("\n\t System priority unsetting Failed\n");
        }
   } else if(!strcasecmp(action, "getsysid"))
    {
      u_char lacp_sysid[6];
      int i = 0;
      ret = smi_lacp_get_sysid(azg, lacp_sysid);
      if(ret >= 0)
        {
        for (i=0; i < 6; i++)
          printf("\n\t Successful: system id %x\n", lacp_sysid[i]);
        } else {
          printf("\n\t Failed\n");
        }
    } else if(!strcasecmp(action, "getcounter"))
    {
      struct smi_lacp_link_counters link_counters;
      ret = smi_lacp_get_counter(azg, linkname, &link_counters);
      if(ret >= 0)
        {
          printf("\n\t Successful\n");
        } else {
          printf("\n\t Failed\n");
        }
    } else if(!strcasecmp(action, "getchsummary"))
    {
      struct smi_lacp_channel_summary ch_summ;
      ret = smi_lacp_get_etherchannelsummary(azg, lacpAdminKey, &ch_summ);
      if(ret >= 0)
        {
          printf("\n\t Successful\n");
        } else {
          printf("\n\t Failed\n");
        }
   } else if(!strcasecmp(action, "getchdetail"))
    {
      struct smi_lacp_channel ch_detail;
      ret = smi_lacp_get_etherchanneldetail(azg, lacpAdminKey, &ch_detail);
      if(ret >= 0)
        {
          printf("\n\t Successful\n");
        } else {
          printf("\n\t Failed\n");
        }
    } else if(!strcasecmp(action, "getsyspriority"))
    {
      unsigned int pr;
      ret = smi_lacp_get_systempriority(azg, &pr);
      if(ret >= 0)
        {
          printf("\n\t Successful: System Priority %d  \n", pr);
        } else {
          printf("\n\t Failed\n");
        }
     }
    return 0;
}

/* MSTP related APIs */
int
smi_mstp_action (struct smiclient_globals *azg, char *action)
{
  int ret = 0;
  int val = 0;

  if(!strcasecmp(action, "addins"))
    {
      int i;
      struct smi_vlan_bmp success_bmp;

      SMI_BMP_INIT (success_bmp);
      SMI_BMP_SET (vlanaddbmp, 1);
      SMI_BMP_SET (vlanaddbmp, 11);
      SMI_BMP_SET (vlanaddbmp, 12);
      SMI_BMP_SET (vlanaddbmp, 21);
      SMI_BMP_SET (vlanaddbmp, 31);

      ret = smi_mstp_add_instance (azg, brname, mstpinst, vlanaddbmp, 
                                   &success_bmp);
      if(ret == 0)
        {
          printf("\n\t Added MSTP Instance Successfully\n");
          for (i=0; i < SMI_BMP_WORD_MAX; i++)
          {
            if (SMI_BMP_IS_MEMBER(success_bmp, i))
               printf("\n\t vlan%d", i);
          }
        } else {
          printf("\n\tMSTP Instance Addition Failed\n");
          for (i=0; i < SMI_BMP_WORD_MAX; i++)
          {
            if (SMI_BMP_IS_MEMBER(success_bmp, i))
               printf("\n\t vlan%d", i);
          }
        }
    } else if (!strcasecmp(action, "delins"))
    {
      int i;
      struct smi_vlan_bmp success_bmp;

      SMI_BMP_INIT (success_bmp);
      SMI_BMP_SET (vlanaddbmp, 11);
      SMI_BMP_SET (vlanaddbmp, 1);
      SMI_BMP_SET (vlanaddbmp, 12);
      SMI_BMP_SET (vlanaddbmp, 21);
      SMI_BMP_SET (vlanaddbmp, 31);

      ret = smi_mstp_delete_instance(azg, brname, mstpinst, vlanaddbmp,
                                     &success_bmp);
      if(ret == 0)
        {
          printf("\n\t Deleted MSTP Instance Successfully\n");
          for (i=0; i < SMI_BMP_WORD_MAX; i++)
          {
            if (SMI_BMP_IS_MEMBER(success_bmp, i))
               printf("\n\t vlan%d", i);
          }
        } else {
          printf("\n\tMSTP Instance Deletion Failed\n");
          for (i=0; i < SMI_BMP_WORD_MAX; i++)
          {
            if (SMI_BMP_IS_MEMBER(success_bmp, i))
               printf("\n\t vlan%d", i);
          }
        }
    } else if(!strcasecmp(action, "setage"))
    {
      ret = smi_mstp_set_ageingtime(azg, brname, mstpageing);
      if(ret == 0)
        {
          printf("\n\t MSTP Ageingtime Set Successfully\n");
        } else {
          printf("\n\tMSTP Ageingtime Failed\n");
        }
    } else if(!strcasecmp(action, "getage"))
    {
      ret = smi_mstp_get_ageingtime(azg, brname, &val);
      if(val >= 0)
       {
         printf("\n\t MSTP Ageingtime Returned :%d\n",val);
       } else {
         printf("\n\tMSTP Ageingtime Getting Failed\n");
       }
    } else if (!strcasecmp(action, "addport"))
    {
      ret = smi_mstp_add_port(azg, brname, ifname, mstpinst);
      if(ret == 0)
        {
          printf("\n\t Added MSTP port Successfully\n");
        } else {
          printf("\n\tMSTP port addition Failed\n");
        }
    } else if (!strcasecmp(action, "delport"))
    {
      ret = smi_mstp_delete_port(azg, brname, ifname, mstpinst);
      if(ret == 0)
        {
          printf("\n\t Deleted MSTP port Successfully\n");
        } else {
          printf("\n\tMSTP port deletion Failed\n");
        }
    } else if(!strcasecmp(action, "sethtime"))
    {
      ret = smi_mstp_set_hellotime(azg, brname, mstphtime);
      if(ret == 0)
        {
          printf("\n\t MSTP Hello time Set Successfully\n");
        } else {
          printf("\n\tMSTP hello time setting Failed\n");
        }
    } else if(!strcasecmp(action, "gethtime"))
    {
      ret = smi_mstp_get_hellotime(azg, brname, &val);
      if(val >= 0)
       {
         printf("\n\t MSTP hello time Returned :%d\n",val);
       } else {
         printf("\n\tMSTP hello time getting Failed\n");
       }
    } else if(!strcasecmp(action, "setmage"))
    {
      ret = smi_mstp_set_maxage(azg, brname, mstpmaxage);
      if(ret == 0)
        {
          printf("\n\t MSTP max age Set Successfully\n");
        } else {
          printf("\n\tMSTP max age Setting Failed\n");
        }
    } else if(!strcasecmp(action, "getmage"))
    {
      ret = smi_mstp_get_maxage(azg, brname, &val);
      if(val >= 0)
       {
         printf("\n\t MSTP max age Returned :%d\n",val);
       } else {
         printf("\n\tMSTP max age Getting Failed\n");
       }
    } else if(!strcasecmp(action, "setporteg"))
    {
      ret = smi_mstp_set_portedge(azg, brname, ifname, mstpportedge);
      if(ret == 0)
        {
          printf("\n\t MSTP port edge Set Successfully\n");
        } else {
          printf("\n\tMSTP port edge Setting Failed\n");
        }
    } else if(!strcasecmp(action, "getporteg"))
    {
      ret = smi_mstp_get_portedge(azg, brname, ifname, &val);
      if(val >= 0)
       {
         printf("\n\t MSTP port edge :%s\n", 
                (val == 0) ? "Disabled":"Enabled");
       } else {
         printf("\n\tMSTP portedge Getting Failed\n");
       }
    } else if(!strcasecmp(action, "setportver"))
    {
      ret = smi_mstp_set_forceversion(azg, brname, ifname, mstpforcever);
      if(ret == 0)
        {
          printf("\n\t MSTP port version Set Successfully\n");
        } else {
          printf("\n\tMSTP port edge Setting Failed\n");
        }
    } else if(!strcasecmp(action, "getportver"))
    {
      ret = smi_mstp_get_forceversion(azg, brname, ifname, &val);
      if(val >= 0)
       {
         printf("\n\t MSTP port version: %d\n", val);
       } else {
         printf("\n\tMSTP port version Getting Failed\n");
       }
    } else if(!strcasecmp(action, "setpr"))
    {
      ret = smi_mstp_set_bridgepriority(azg, brname, mstpbrpriority);
      if(ret == 0)
        {
          printf("\n\t MSTP Bridge Priority Set Successfully\n");
        } else {
          printf("\n\tMSTP Bridge Priority setting Failed\n");
        }
    } else if(!strcasecmp(action, "getpr"))
    {
      ret = smi_mstp_get_bridgepriority(azg, brname, &val);
      if(val > 0)
       {
         printf("\n\t MSTP bridgepriority Returned :%d\n",val);
       } else {
         printf("\n\tMSTP  bridgepriority Getting Failed\n");
       }
    } else if(!strcasecmp(action, "setfwdd"))
    {
      ret = smi_mstp_set_forwarddelay(azg, brname, mstpfwdelay);
      if(ret == 0)
        {
          printf("\n\t MSTP Forward Delay Set Successfully\n");
        } else {
          printf("\n\tMSTP Forward Delay  setting Failed\n");
        }
     } else if(!strcasecmp(action, "getfwdd"))
    {
      ret = smi_mstp_get_forwarddelay(azg, brname, &val);
      if(val >= 0)
        {
          printf("\n\t MSTP Forward Delay Returned :%d\n", val);
        } else {
          printf("\n\t MSTP Forward Delay getting Failed\n");
        }
   } else if(!strcasecmp(action, "setmpr"))
    {
      ret = smi_mstp_set_msti_bridgepriority(azg, brname, mstpinst, 
                                             mstpmstibrpr);
      if(ret == 0)
        {
          printf("\n\t MSTP MSTI Bridge priority set successfully\n");
        } else {
          printf("\n\tMSTP setting Failed\n");
        }
    } else if(!strcasecmp(action, "getmpr"))
    {
      ret = smi_mstp_get_msti_bridgepriority(azg, brname, mstpinst, &val);
      if(val >= 0)
        {
          printf("\n\t MSTP MSTI Bridge priority Returned :%d\n", val);
        } else {
          printf("\n\t MSTP getting Failed\n");
        }
    } else if(!strcasecmp(action, "setmcost"))
    {
      ret = smi_mstp_set_msti_portpathcost(azg, brname, ifname, mstpinst, 
                                           mstppathcost);
      if(ret == 0)
        {
          printf("\n\tPath Cost Set Successfully\n");
        } else {
          printf("\n\tPath Cost setting Failed\n");
        }
     } else if(!strcasecmp(action, "getmcost"))
    {
      ret = smi_mstp_get_msti_portpathcost(azg, brname, ifname, mstpinst, &val);
      if(val >= 0)
        {
          printf("\n\t CIST Path Cost Returned :%d\n", val);
        } else {
          printf("\n\t CIST Path Cost getting Failed\n");
        }
    } else if(!strcasecmp(action, "setrst"))
    {
      ret = smi_mstp_set_msti_instance_restrictedrole(azg, brname, ifname, 
                                                      mstpinst,
                                                      mstpmstirstrrole);
      if(ret == 0)
        {
          printf("\n\tRistriction Set Successfully\n");
        } else {
          printf("\n\tRistriction setting Failed\n");
        }
     } else if(!strcasecmp(action, "getrst"))
    {
      ret = smi_mstp_get_msti_instance_restrictedrole(azg, brname, ifname, 
                                                      mstpinst,
                                                      &val);
      if(val >= 0)
        {
          printf("\n\t Ristriction  Returned :%d\n", val);
        } else {
          printf("\n\t Ristriction getting Failed\n");
        }
    } else if(!strcasecmp(action, "setrstcn"))
    {
      ret = smi_mstp_set_msti_instance_restrictedtcn(azg, brname, ifname, 
                                                     mstpinst, 
                                                     mstprestricttcn);
      if(ret == 0)
        {
          printf("\n\tRistriction Set Successfully\n");
        } else {
          printf("\n\tRistriction setting Failed\n");
        }
    } else if(!strcasecmp(action, "getrstcn"))
    {
      ret = smi_mstp_get_msti_instance_restrictedtcn(azg, brname, ifname, 
                                                     mstpinst, 
                                                     &val);
      if(val >= 0)
        {
          printf("\n\t Ristriction  Returned :%d\n", val);
        } else {
          printf("\n\t Ristriction getting Failed\n");
        }
    } else if(!strcasecmp(action, "sethello"))
    {
      ret = smi_mstp_set_porthellotime(azg, brname, ifname, mstpporthtime);
      if(ret == 0)
        {
          printf("\n\t MSTP port hellotime Set Successfully\n");
        } else {
          printf("\n\tMSTP port hellotime setting  Failed\n");
        }
    } else if(!strcasecmp(action, "gethello"))
    {
      ret = smi_mstp_get_porthellotime(azg, brname, ifname, &val);
      if(val > 0)
       {
         printf("\n\t MSTP port hellotime Returned :%d\n",val);
       } else {
         printf("\n\tMSTP port hellotime Getting Failed\n");
       }
    } else if(!strcasecmp(action, "setp2p"))
    {
      ret = smi_mstp_set_portp2p(azg, brname, ifname, mstpportp2p);
      if(ret == 0)
        {
          printf("\n\t MSTP Link Type Set Successfully\n");
        } else {
          printf("\n\tMSTP Link Type setting  Failed\n");
        }
    } else if(!strcasecmp(action, "getp2p"))
    {
      ret = smi_mstp_get_portp2p(azg, brname, ifname, &val);
      if(val >= 0)
       {
         printf("\n\t MSTP Link Type Returned :%d\n",val);
       } else {
         printf("\n\tMSTP Link Type Getting Failed\n");
       }
    } else if(!strcasecmp(action, "setcost"))
    {
      ret = smi_mstp_set_portpathcost(azg, brname, ifname, mstpcstipcost);
      if(ret == 0)
        {
          printf("\n\t CIST Port Pathcost Set Successfully\n");
        } else {
          printf("\n\tCIST Port Pathcost  setting  Failed\n");
        }
    } else if(!strcasecmp(action, "getcost"))
    {
      ret = smi_mstp_get_portpathcost(azg, brname,ifname, &val);
      if(val > 0)
       {
         printf("\n\t CIST Port Pathcost Returned :%d\n",val);
       } else {
         printf("\n\tCist Port Pathcost Getting Failed\n");
       }
    } else if(!strcasecmp(action, "setmhops"))
    {
      ret = smi_mstp_set_maxhops(azg, brname, mstpmaxhops);
      if(ret == 0)
        {
          printf("\n\t MSTP max hops Set Successfully\n");
        } else {
          printf("\n\tMSTP max hops setting  Failed\n");
        }
    } else if(!strcasecmp(action, "getmhops"))
    {
      ret = smi_mstp_get_maxhops(azg, brname, &val);
      if(val > 0)
       {
         printf("\n\t MSTP max hops Returned :%d\n",val);
       } else {
         printf("\n\tMSTP max hops Getting Failed\n");
       }
    } else if(!strcasecmp(action, "setppriority"))
    {
      ret = smi_mstp_set_portpriority(azg, brname, ifname, mstpportpr);
      if(ret == 0)
        {
          printf("\n\t MSTP Port Priority Set Successfully\n");
        } else {
          printf("\n\tMSTP Port Priority setting Failed\n");
        }
    } else if(!strcasecmp(action, "getppriority"))
    {
      s_int16_t pr = 0;
      ret = smi_mstp_get_portpriority(azg, brname, ifname, &pr);
      if(ret == 0)
       {
         printf("\n\t MSTP portpriority Returned :%d\n",pr);
       } else {
         printf("\n\tMSTP  portpriority Getting Failed\n");
       }
    } else if(!strcasecmp(action, "setprstrol"))
    {
      ret = smi_mstp_set_port_restrictedrole(azg, brname, ifname,
                                             mstpportrsrtrole);
      if(ret == 0)
        {
          printf("\n\t MSTP Port Restricted role Set Successfully\n");
        } else {
          printf("\n\tMSTP Port restricted role setting Failed\n");
        }
    } else if(!strcasecmp(action, "getprstrol"))
    {
      s_int16_t rst = 0;
      ret = smi_mstp_get_port_restrictedrole(azg, brname, ifname, &rst);
      if(ret == 0)
       {
         printf("\n\t MSTP port RST role Returned :%d\n", rst);
       } else {
         printf("\n\tMSTP  port RST role Getting Failed\n");
       }
    } else if(!strcasecmp(action, "setprsttcn"))
    {
      ret = smi_mstp_set_port_restrictedtcn(azg, brname, ifname, mstpporttcn);
      if(ret == 0)
        {
          printf("\n\t MSTP Port Restricted tcn Set Successfully\n");
        } else {
          printf("\n\tMSTP Port restricted tcn setting Failed\n");
        }
    } else if(!strcasecmp(action, "getprsttcn"))
    {
      s_int16_t tcn = 0;
      ret = smi_mstp_get_port_restrictedtcn(azg, brname, ifname, &tcn);
      if(ret == 0)
       {
         printf("\n\t MSTP port RST role Returned :%d\n", tcn);
       } else {
         printf("\n\tMSTP  port RST role Getting Failed\n");
       }
    } else if(!strcasecmp(action, "setprootg"))
    {
      ret = smi_mstp_set_portrootguard(azg, brname, ifname, mstpportrtgrd);
      if(ret == 0)
        {
          printf("\n\t MSTP Port Rootguard Set Successfully\n");
        } else {
          printf("\n\tMSTP Port Rootguard setting Failed\n");
        }
    } else if(!strcasecmp(action, "getprootg"))
    {
      s_int32_t root;
      ret = smi_mstp_get_portrootguard(azg, brname,ifname, &root);
      if(ret == 0)
       {
         printf("\n\t MSTP portroot guard Returned :%d\n", root);
       } else {
         printf("\n\tMSTP  portroot guard Getting Failed\n");
       }
    } else if(!strcasecmp(action, "setpbpduf"))
    {
      ret = smi_mstp_set_portbpdufilter(azg, brname, ifname, mstpportbpfltr);
      if(ret == 0)
        {
          printf("\n\t MSTP Port BPDU filter Set Successfully\n");
        } else {
          printf("\n\tMSTP Port BPDU filter setting Failed\n");
        }
    } else if(!strcasecmp(action, "getpbpduf"))
    {
      u_char bp;
      ret = smi_mstp_get_portbpdufilter(azg, brname, ifname, &bp);
      if(ret == 0)
       {
         printf("\n\t MSTP portbpdu filter Returned :%d\n", bp);
       } else {
         printf("\n\tMSTP  portbpdu filter Getting Failed\n");
       }
    } else if(!strcasecmp(action, "enablebr"))
    {
      ret = smi_mstp_enable_bridge(azg, brname);
      if(ret == 0)
        {
          printf("\n\t Enabled Successfully\n");
        } else {
          printf("\n\tEnabling Failed\n");
        }
    } else if(!strcasecmp(action, "disablebr"))
    {
      ret = smi_mstp_disable_bridge(azg, brname, mstpbrforward);
      if(ret == 0)
       {
         printf("\n\t Disabled successfully \n");
       } else {
         printf("\n\tDisabling Failed\n");
       }
    } else if(!strcasecmp(action, "setpbpdug"))
    {
      ret = smi_mstp_set_portbpduguard(azg, brname, ifname, mstpportbpgrd);
      if(ret == 0)
        {
          printf("\n\t MSTP Port BPDU guard Set Successfully\n");
        } else {
          printf("\n\tMSTP Port BPDU guard setting Failed\n");
        }
    } else if(!strcasecmp(action, "getpbpdug"))
    {
      u_char bp;
      ret = smi_mstp_get_portbpduguard(azg, brname,ifname, &bp);
      if(ret == 0)
       {
         printf("\n\t MSTP portbpdu guard Returned :%d\n", bp);
       } else {
         printf("\n\tMSTP  portbpdu guard Getting Failed\n");
       }
    } else if(!strcasecmp(action, "settxcount"))
    {
      ret = smi_mstp_set_transmit_holdcount(azg, brname, mstptxholdcnt);
      if(ret == 0)
        {
          printf("\n\t Txholdcount Set Successfully\n");
        } else {
          printf("\n\t txholdcount setting Failed\n");
        }
    } else if(!strcasecmp(action, "gettxcount"))
    {
      unsigned char tx;
      ret = smi_mstp_get_transmit_holdcount(azg, brname, &tx);
      if(ret == 0)
       {
         printf("\n\t Txholdcount Returned :%d\n", tx);
       } else {
         printf("\n\tTxholdcount Getting Failed\n");
       }
    } else if(!strcasecmp(action, "setbrbpdug"))
    {
      ret = smi_mstp_set_bridge_portfast_bpduguard(azg, brname, 
                                                   mstpportfastbpgrd);
      if(ret == 0)
        {
          printf("\n\t Set Successfully\n");
        } else {
          printf("\n\t setting Failed\n");
        }
    } else if(!strcasecmp(action, "getbrbpdug"))
    {
      ret = smi_mstp_get_bridge_portfast_bpduguard(azg, brname, &val);
      if(ret == 0)
       {
         printf("\n\t portfast bpduguard  Returned :%d\n", val);
       } else {
         printf("\n\t Getting Failed\n");
       }
    } else if(!strcasecmp(action, "seterrten"))
    {
      ret = smi_mstp_set_bridge_errdisable_timeoutenable(azg, brname, 
                                                         mstperrdistoen);
      if(ret == 0)
        {
          printf("\n\t Set Successfully\n");
        } else {
          printf("\n\t setting Failed\n");
        }
    } else if(!strcasecmp(action, "geterrten"))
    {
      ret = smi_mstp_get_bridge_errdisable_timeoutenable(azg, brname, &val);
      if(ret == 0)
       {
         printf("\n\t Errdisable timeout enable Returned :%d\n", val);
       } else {
         printf("\n\t Getting Failed\n");
       }
    } else if(!strcasecmp(action, "seterrtime"))
    {
      ret = smi_mstp_set_bridge_errdisable_timeoutinterval(azg, brname, 
                                                           mstperrdistout);
      if(ret == 0)
        {
          printf("\n\t Set Successfully\n");
        } else {
          printf("\n\t setting Failed\n");
        }
    } else if(!strcasecmp(action, "geterrtime"))
    {
      ret = smi_mstp_get_bridge_errdisable_timeoutinterval(azg, brname, &val);
      if(ret == 0)
       {
         printf("\n\t Errdisable timeout interval Returned :%d\n", val);
       } else {
         printf("\n\t Getting Failed\n");
       }
    } else if(!strcasecmp(action, "setmstip"))
    {
      ret = smi_mstp_set_msti_portpriority(azg, brname, ifname, mstpinst, 
                                           mstpmstiportpr);
      if(ret == 0)
        {
          printf("\n\t Set Successfully\n");
        } else {
          printf("\n\t setting Failed\n");
        }
    } else if(!strcasecmp(action, "getmstip"))
    {
      s_int16_t pr;
      ret = smi_mstp_get_msti_portpriority(azg, brname, ifname, mstpinst, &pr);
      if(ret == 0)
       {
         printf("\n\t MSTI portpriority Returned :%d\n", pr);
       } else {
         printf("\n\t Getting Failed\n");
       }
    } else if(!strcasecmp(action, "setrev"))
    {
      ret = smi_mstp_set_revision_number(azg, brname, mstprevnu);
      if(ret == 0)
        {
          printf("\n\t Revision Number Set Successfully\n");
        } else {
          printf("\n\t revision Number setting Failed\n");
        }
    } else if(!strcasecmp(action, "getrev"))
    {
      u_int16_t rev;
      ret = smi_mstp_get_revision_number(azg, brname, &rev);
      if(ret == 0)
       {
         printf("\n\t Revision number Returned :%d\n", rev);
       } else {
         printf("\n\t Revision number Getting Failed\n");
       }
     } else if(!strcasecmp(action, "setautoeg"))
    {
      ret = smi_mstp_set_autoedge(azg, brname, ifname, mstpautoedge);
      if(ret == 0)
        {
          printf("\n\t MSTP auto edge Set Successfully\n");
        } else {
          printf("\n\tMSTP auto edge Setting  Failed\n");
        }
    } else if(!strcasecmp(action, "getautoeg"))
    {
     int auto_edge = 0;
      ret = smi_mstp_get_autoedge(azg, brname, ifname, &auto_edge);
      if(auto_edge > 0)
       {
         printf("\n\t MSTP auto edge Returned :%d\n",auto_edge);
       } else {
         printf("\n\tMSTP auto edge Getting Failed\n");
       }
    } else if(!strcasecmp(action, "setregion"))
    {
      ret = smi_mstp_set_regionname(azg, brname, mstpregname);
      if(ret == 0)
        {
          printf("\n\t MSTP region name Set Successfully\n");
        } else {
          printf("\n\tMSTP region name Setting  Failed\n");
        }
 
   } else if(!strcasecmp(action, "getregion"))
    {
      char regname[MSTP_CONFIG_NAME_LEN];
      ret = smi_mstp_get_regionname(azg, brname, regname);
      if(ret == 0)
       {
         printf("\n\t MSTP region name Returned :%s\n", regname);
       } else {
         printf("\n\tMSTP region name Getting Failed\n");
       }
     } else if(!strcasecmp(action, "getspan"))
    {
      struct smi_mstp_spanning_tree_details details;
      ret = smi_mstp_get_spanningtree_details(azg, brname, &details);
      if(ret == 0)
        {
          printf("\n\t Details getting Successful\n");
        } else {
          printf("\n\t Details getting Failed\n");
        }
     } else if(!strcasecmp(action, "getspanitf"))
    {
      struct smi_mstp_port_details details;
      ret = smi_mstp_get_spanningtree_interface(azg, ifname, &details);
      if(ret == 0)
        {
          printf("\n\t Details getting Successful\n");
          switch (details.cist_state)
            {
              case SMI_STATE_DISCARDING:
                printf ("\n Interface %s Port State is Discarding\n",ifname);
              break;
              case SMI_STATE_LISTENING:
                printf ("\n Interface %s Port State is Listening\n",ifname);
              break;
              case SMI_STATE_LEARNING:
                printf ("\n Interface %s Port State is Learning\n",ifname);
              break;
              case SMI_STATE_FORWARDING:
                printf ("\n Interface %s Port State is Forwarding\n",ifname);
             break;
             case SMI_STATE_BLOCKING:
                printf ("\n Interface %s Port State is Blocking\n",ifname);
             break;
             case SMI_STATE_ERROR:
                printf ("\n Interface %s Port is in Invalid State\n",ifname);
             break;
              case SMI_STATE_DISABLED:
                printf ("\n Interface %s is in Disabled State \n",ifname);
             break;
             default:
                printf ("\n Interface %s is in %d State \n",ifname,details.cist_state);
             break;
           }
        } else {
          printf("\n\t Details getting Failed\n");
        }
     } else if(!strcasecmp(action, "getspanmst"))
   {
      struct smi_mstp_instance_details details;
      int i=0;
      ret = smi_mstp_get_spanningtree_mst(azg, brname, mstpinst, &details);
      if(ret == 0)
        {
          printf("\n\t Details getting Successful\n");
           for (i=0; i <= 4094; i++)
             if (SMI_BMP_IS_MEMBER (details.vlanMemberBmp, i))
                printf (" %d ",i);
        } else {
          printf("\n\t Details getting Failed\n");
        }
     } else if(!strcasecmp(action, "getspanconf"))
    {
      struct smi_mstp_config_details details;
      int index;
      ret = smi_mstp_get_spanningtree_mstconfig(azg, brname, &details);
      if(ret == 0)
        {
          printf("\n\t Details getting Successful:\n", details.cfg_digest);
          printf ("\n\tDigest:");
          for (index = 0; index < SMI_MSTP_CONFIG_DIGEST_LEN ; index++)
             {
               printf ("%.2X", details.cfg_digest[index]);
             }

        } else {
          printf("\n\t Details getting Failed\n");
        }
     } else if(!strcasecmp(action, "getstpmst"))
    {
      struct smi_mstp_instance_details details;
      ret = smi_mstp_get_spanningtree_mstdetail(azg, brname, mstpinst,&details);
      if(ret == 0)
       {
         printf("\n\t Got Instance details Successfully\n");
       } else {
         printf("\n\tFailed to get instance details\n");
       }
     } else if(!strcasecmp(action, "getstpmstpif"))
    {
     struct smi_mstp_instance_port_details details;
      ret = smi_mstp_get_spanningtree_mstdetail_interface(azg, ifname, mstpinst,
                                                          &details);
      if(ret == 0)
       {
         printf("\n\tGot Instance details and its if details successfully\n");
       } else {
         printf("\n\tFailed to get Instance details and its if details\n");
       }
     } else if(!strcasecmp(action, "gettraffictbl"))
    {
      u_char traffic_table[SMI_BRIDGE_MAX_USER_PRIO]
                          [SMI_BRIDGE_MAX_TRAFFIC_CLASS];
      int i = 0, j = 0;

      for(i = 0; i < SMI_BRIDGE_MAX_USER_PRIO; i++)
       for (j = 0; j< SMI_BRIDGE_MAX_TRAFFIC_CLASS; j++)
         traffic_table[i][j] = 0;

      ret = smi_apn_get_traffic_class_table(azg, ifname, traffic_table);
      if(ret == 0)
       {
         printf("\n\t Got the data from Traffic table Successfully\n");
         for (i = 0; i < SMI_BRIDGE_MAX_USER_PRIO; i++)
          {
           for(j = 0; j < SMI_BRIDGE_MAX_TRAFFIC_CLASS; j++)
            {            
              printf("\t%d",traffic_table[i][j]);
            }
           printf("\n");
          }
       } else {
         printf("\n\tFailed to get the data from the Traffic table\n");
        }
     } else if(!strcasecmp(action, "getusrpr"))
    {
      u_int8_t usrpriority = 0;
      //ret = smi_apn_get_user_priority(azg, ifname, &usrpriority);
      if(usrpriority >  0)
       {
         printf("\n\t Got User Priority successfully:%d\n", mstpuserpr);
       } else {
         printf("\n\tFailed to get User Priority\n");
       }
     } else if(!strcasecmp(action, "addbridgemac"))
     {
      char mac_addr[6];
      hwaddr_aton(hwaddr, mac_addr);

      ret = smi_bridge_add_mac(azg, brname, ifname, mac_addr, vlanid, 
                               mstpisforward);
      if(ret == 0)
        {
          printf("\n\t Added a static MAC Address Successfully\n");
        } else {
          printf("\n\tFailed to add a static MAC address\n");
        }
     } else if(!strcasecmp(action, "delbridgemac"))
    {
      char mac_addr[6];
      hwaddr_aton(hwaddr, mac_addr);

      ret = smi_bridge_delete_mac(azg, brname, ifname, mac_addr, vlanid, 
                                  mstpisforward);
      if(ret == 0)
       {
         printf("\n\t Deleted a static MAC Address Successfully\n");
       } else {
         printf("\n\tFailed to delete a static MAC address\n");
       }
     } else if(!strcasecmp(action, "flushdynentry"))
    {
      ret = smi_bridge_flush_dynamic_entries(azg, brname);
      if(ret == 0)
       {
         printf("\n\t Flushed all dynamic entries successfully\n");
       } else {
         printf("\n\tFailed to flush the dynamic entries\n");
       }
     } else if(!strcasecmp(action, "setbpdufltr"))
    {
      ret = smi_mstp_set_bridge_portfast_bpdufilter(azg, brname, 
                                                    mstpportfastfltr);
      if(ret == 0)
        {
          printf("\n\t MSTP BPDU Filter Set Successfully\n");
        } else {
          printf("\n\tMSTP BPDU Filter setting  Failed\n");
        }
    } else if(!strcasecmp(action, "getbpdufltr"))
    {
      ret = smi_mstp_get_bridge_portfast_bpdufilter(azg, brname, &val);
      if(ret >= 0)
       {
         printf("\n\t MSTP BPDU Filter Status Returned :%d\n",val);
       } else {
         printf("\n\tMSTP BPDU Filter Status Getting Failed\n");
       }
    }
    else if (!strcasecmp(action, "mcheck"))
    {
      ret = smi_mstp_check(azg, brname, ifname);
      if(ret == 0)
        {
          printf("\n\tCleared the protocols Successfully\n");
        } else {
          printf("\n\tFailed to clear protocols\n");
        }
    }
    else if (!strcasecmp(action, "brstatus"))
    {
      bool_t enabled, br_forward;
      ret = smi_mstp_get_bridge_status(azg, brname, &enabled, &br_forward);
      if(ret == 0)
        {
          printf("\n\tMSTP bridge status get Success: %d %d\n", 
                 enabled, br_forward);
        } else {
          printf("\n\tMSTP bridge status get failed\n");
        }
    }
  
  return 0;
}

/* VLAN related APIs */
int
smi_vlan_action (struct smiclient_globals *azg, char *action)
{
  int ret=0;

  if(!strcasecmp(action, "addvlan"))
  {
    ret = smi_vlan_add(azg, brname, "vlan_new", vlanid, vlanstate, vlantype);
    if(ret == 0)
    {
      printf("\n\t Added vlan\n");
    } else {
      printf("\n\t Vlan Addition Failed\n");
    }
  }
  else if(!strcasecmp(action, "delvlan"))
  {
    ret = smi_vlan_delete(azg, brname, vlanid, vlantype);
    if(ret == 0)
    {
      printf("\n\t Deleted vlan\n");
    } else {
      printf("\n\t Vlan Del Failed\n");
    }
  }
  else if (!strcasecmp(action,"addvlanrange"))
   {
     struct smi_vlan_bmp vlanAddFailed;
     int i;
     u_int16_t lower_vlan = 2, higher_vlan = 4094;

     SMI_BMP_INIT (vlanAddFailed);
     ret = smi_vlan_range_add (azg, brname, vlanname, lower_vlan, higher_vlan,
                               vlanstate, vlantype, &vlanAddFailed);

     printf ("\n Following Vlan Id's were not added \n");
     for (i=0; i < SMI_BMP_WORD_MAX; i++)
      if (SMI_BMP_IS_MEMBER (vlanAddFailed, i))
         printf (" %d ", i);
   }
  else if (!strcasecmp(action,"delvlanrange"))
   {
     struct smi_vlan_bmp vlanDelFailed;
     int i;
     u_int16_t lower_vlan = 2, higher_vlan = 4094;

     SMI_BMP_INIT (vlanDelFailed);
     ret = smi_vlan_range_del (azg, brname, lower_vlan, higher_vlan,
                               vlantype, &vlanDelFailed);

     for (i=0; i < SMI_BMP_WORD_MAX; i++)
       if (SMI_BMP_IS_MEMBER (vlanDelFailed, i))
         printf (" %d ", i);
   }
  else if(!strcasecmp(action, "setvlanportmode"))
  {
    ret = smi_vlan_api_set_port_mode (azg, ifname, vlanportmode,
                                      vlanportsubmode);
    if(ret == 0)
    {
      printf("\n\t set vlan port mode \n");
    } else {
      printf("\n\t set vlan port mode Failed\n");
    }
  }
  else if(!strcasecmp(action, "getvlanportmode"))
  {
    enum smi_vlan_port_mode mode, submode;

    ret = smi_vlan_api_get_port_mode(azg, ifname, &mode, &submode);

    if(ret == 0)
    {
      printf("\n\t vlan port mode for intf: %d, %d \n", mode, submode);
    }
    else
    {
      printf("\n\t get vlan port mode Failed\n");
    }

  }
  else if(!strcasecmp(action, "setframetype"))
  {
      ret = smi_vlan_set_acceptable_frame_type(azg, ifname,
                                               vlanportmode,
                                               vlanframetype);
      if(ret == 0)
      {
        printf("\n\t set vlan frame type\n");
      } else {
        printf("\n\t set vlan frame type Failed\n");
      }
  } else if(!strcasecmp(action, "getframetype"))
  {
      int frame_type = 0;
      ret = smi_vlan_get_acceptable_frame_type(azg, ifname, &frame_type);
      if(ret >= 0)
      {
        printf("\n\t Vlan frame type: %d \n", frame_type);
      } else {
        printf("\n\t get vlan frame type Failed\n");
      }
  }
  else if(!strcasecmp(action, "setingfil"))
  {
      ret = smi_vlan_set_ingress_filter(azg, ifname,
                                        vlaningressfilter);
      if(ret == 0)
      {
        printf("\n\t set vlan ingress filter success\n");
      } else {
        printf("\n\t set vlan ingress filter failed\n");
      }
  } else if(!strcasecmp(action, "getingfil"))
  {
      enum smi_vlan_port_mode mode, submode;
      enum smi_vlan_port_ingress_filter enable;
      ret = smi_vlan_get_ingress_filter(azg, ifname, &mode, &submode, &enable);
      if(ret >= 0)
      {
        printf("\n\t VLAN ingress filter: mode: %d, submode: %d, enable: %d\n", 
               mode, submode, enable);
      } else {
        printf("\n\t get ingress filter  Failed\n");
      }
  }
  else if(!strcasecmp(action, "setdvid"))
  {
      ret = smi_vlan_set_default_vid(azg, ifname, vlanid);
      if(ret == 0)
      {
        printf("\n\t set default vlan id success\n");
      } else {
        printf("\n\t set default vlan id failed\n");
      }
  } else if(!strcasecmp(action, "getdvid"))
  {
      u_int16_t vid = 0;
      ret = smi_vlan_get_default_vid(azg, ifname, &vid);
      if(ret >= 0)
      {
        printf("\n\t Default VLAN id: %d\n", vid);
      } else {
        printf("\n\t get default VLAN id Failed\n");
      }
  }
  else if(!strcasecmp(action, "addvlanport"))
  {
    int i;
    struct smi_vlan_bmp success_bmp;
    SMI_BMP_INIT (success_bmp);

#if 0
    for (i = SMI_VLAN_ID_START + 1; i < 200; i++)
    {
      SMI_BMP_SET (vlanaddbmp, i);
    }
    for (i = SMI_VLAN_ID_START + 1; i < 100; i++)
    {
      SMI_BMP_SET (vlanegbmp, i);
    }
#else
    SMI_BMP_SET (vlanaddbmp, 11);
    SMI_BMP_SET (vlanaddbmp, 21);
    SMI_BMP_SET (vlanaddbmp, 31);
    SMI_BMP_SET (vlanegbmp, 31);
#endif
    ret = smi_vlan_add_vlan_to_port(azg, ifname, &vlanaddbmp, &vlanegbmp, 
                                    &success_bmp);
    if(ret == 0)
    {
      printf("\n\t Added vlans to port\n");
        printf("\n\t vlan bmp: ");
        for (i=0; i < 200; i++)
        {
          if (SMI_BMP_IS_MEMBER(success_bmp, i))
             printf("\n\t vlan%d", i);
        }
    } else {
      printf("\n\t Vlan Addition to port Failed\n");
        printf("\n\t vlan bmp: ");
        for (i=0; i < SMI_BMP_WORD_MAX; i++)
        {
          if (SMI_BMP_IS_MEMBER(success_bmp, i))
             printf("\n\t vlan%d", i);
        }
    }
  }
  else if(!strcasecmp(action, "delvlanport"))
  {
    int i;
    struct smi_vlan_bmp success_bmp;
    SMI_BMP_INIT (success_bmp);
    SMI_BMP_SET (vlandelbmp, 21);
    SMI_BMP_SET (vlandelbmp, 31);

    ret = smi_vlan_delete_vlan_from_port(azg, ifname, &vlandelbmp, 
                                         &success_bmp);
    if(ret == 0)
    {
      printf("\n\t Deleted vlan from port\n");
        printf("\n\t vlan bmp: ");
        for (i=0; i < SMI_BMP_WORD_MAX; i++)
        {
          if (SMI_BMP_IS_MEMBER(success_bmp, i))
             printf("\n\t vlan%d", i);
        }
    } else {
      printf("\n\t Vlan Delete from port Failed\n");
        printf("\n\t vlan bmp: ");
        for (i=0; i < SMI_BMP_WORD_MAX; i++)
        {
          if (SMI_BMP_IS_MEMBER(success_bmp, i))
             printf("\n\t vlan%d", i);
        }
    }
  }
  else if(!strcasecmp(action, "clrport"))
  {
    ret = smi_vlan_clear_port(azg, ifname);
    if(ret == 0)
    {
      printf("\n\t Cleared port \n");
    } else {
      printf("\n\t Clear port Failed\n");
    }
  }
  else if(!strcasecmp(action, "addallexvid"))
  {
    struct smi_vlan_bmp excludeBmp;

    SMI_BMP_INIT (excludeBmp);
    SMI_BMP_SET (excludeBmp, vlanid);

    ret = smi_vlan_add_all_except_vid (azg, ifname,
                                       vlanportmode,
                                       vlanportsubmode,
                                       &excludeBmp,
                                       vlanegresstype, vlan_add_opt);
    if(ret == 0)
    {
      printf("\n\t Add all except vid success\n");
    } else {
      printf("\n\t Add all except vid Failed\n");
    }
  } else if(!strcasecmp(action, "getallvlan"))
  {
      struct smi_vlan_bmp vlan_bmp;
      int i;
      ret = smi_get_all_vlan_config(azg, brname, &vlan_bmp);
      if(ret >= 0)
      {
        printf("\n\t vlan bmp: ");
        for (i=0; i < SMI_BMP_WORD_MAX; i++)
        {
          if (SMI_BMP_IS_MEMBER(vlan_bmp, i))
             printf("\n\t vlan%d", i);
        }
      } else {
        printf("\n\t Get all vlan config Failed\n");
      }
  } else if(!strcasecmp(action, "getvlanbyid"))
  {
      struct smi_vlan_info vlan_info;
      int i;
      ret = smi_get_vlan_by_id(azg, brname, vlanid, &vlan_info);
      if(ret >= 0)
      {
        printf("\n\t Vlan info:\n");
        printf("\n\t\t Vlan id: %d\n", vlan_info.vid);
        printf("\n\t\t Bridge id: %d\n", vlan_info.bridge_id);
        printf("\n\t\t Type: %d\n", vlan_info.type);
        printf("\n\t\t State: %d\n", vlan_info.vlan_state);
        printf("\n\t\t Instance: %d\n", vlan_info.instance);
        printf("\n\t Port list: ");
        for (i=0; i < SMI_BMP_WORD_MAX; i++)
          if (SMI_BMP_IS_MEMBER(vlan_info.port_list, i))
             printf("\n\t Port%d", i);
      } else {
        printf("\n\t Get vlan config by id Failed\n");
      }
  } else if(!strcasecmp(action, "getvlanif"))
  {
      struct smi_if_vlan_info if_vlan_info;
      ret = smi_vlan_if_get(azg, ifname, &if_vlan_info);
      if(ret >= 0)
      {
        printf("\n\t Interface info:\n");
        printf("\n\t\t Name: %s\n", if_vlan_info.name);
        printf("\n\t\t Vlan mode: %d\n", if_vlan_info.mode);
        printf("\n\t\t Vlan submode: %d\n", if_vlan_info.sub_mode);
        printf("\n\t\t Pvid: %d\n", if_vlan_info.pvid);
 
        if (if_vlan_info.config == SMI_VLAN_CONFIGURED_ALL)
          printf ("\n Config All\n");
        else if (if_vlan_info.config == SMI_VLAN_CONFIGURED_NONE)
          printf ("\n Config None\n");
        else if (if_vlan_info.config == SMI_VLAN_CONFIGURED_SPECIFIC)
          printf ("\n Config Specific\n");
        else
          printf ("\n Invalid \n");  
      } else {
        printf("\n\t Get vlan if Failed\n");
      }
  } else if(!strcasecmp(action, "getbridge"))
  {
      struct smi_bridge bridge_info;
      ret = smi_get_bridge(azg, brname, &bridge_info);
      if(ret >= 0)
      {
        printf("\n\t Bridge info:\n");
        printf("\n\t\t Name: %s\n", bridge_info.name);
        printf("\n\t\t Type: %d\n", bridge_info.type);
        printf("\n\t\t Bridge id: %d\n", bridge_info.bridge_id);
        printf("\n\t\t IsDefault: %d\n", bridge_info.is_default);
        printf("\n\t\t Ageing time: %d\n", bridge_info.ageing_time);
        printf("\n\t\t Learning: %d\n", bridge_info.learning);
      } else {
        printf("\n\t Get bridge info Failed\n");
      }
  }
  else if(!strcasecmp(action, "protoprocess"))
  {
      ret = smi_bridge_port_proto_process (azg, ifname, l2proto, protoprocess);
      if(ret == 0)
      {
        printf("\n\t set port process on customer port success\n");
      } else {
        printf("\n\t set port process on customer port failed\n");
      }
  }
  else if(!strcasecmp(action, "getprotoprocess"))
  {
      enum smi_bridge_proto_process process = 0; 
      ret = smi_bridge_get_port_proto_process (azg, ifname, l2proto, 
                                               &process);
      if(ret == 0)
      {
        printf("\n\t get port process on customer port success: %d\n", process);
      } else {
        printf("\n\t get port process on customer port failed\n");
      }
  }
  return 0;
}


/* RMON related APIs */
int
smi_rmon_action (struct smiclient_globals *azg, char *action)
{
  int ret = 0;
  int val = 0;
  oid temp_oidval[] = {1, 3, 6, 1, 2, 1, 16, 1, 1, 1};
  ut_int64_t val_int32;
  oid get_oid[100];
  char val_str[400];

  memset(val_str,0, sizeof(val_str));

  if(!strcasecmp(action, "getifstats"))
    {
     struct smi_rmon_ifstats rmon_ifstats;
      ret = smi_rmon_get_if_stats(azg, ifname, &rmon_ifstats);
      if(ret == 0)
        {
          printf("\n\t Got Interface Stats:\n");
        } else {
          printf("\n\tInterface Stats Get Failed\n");
        }
   }
   else if (!strcasecmp(action, "sethisintval"))
   {
      ret = smi_rmon_coll_history_interval_set(azg, rmonhistindex, rmonhistint,
                                               ifname);
      if(ret == 0)
        {
          printf("\n\t Set History Interval Successfully\n");
        } else {
          printf("\n\tFailed to set History Interval\n");
        }
   }
   else if (!strcasecmp(action, "gethisintval"))
   {
      ret = smi_get_rmon_coll_history_interval(azg, rmonhistindex, &val);
      if(ret == 0)
        {
          printf("\n\t Got History Interval Successfully:%d\n", val);
        } else {
          printf("\n\tFailed to get History Interval\n");
        }
   }
   else if (!strcasecmp(action, "sethistowner"))
   {
      ret = smi_rmon_coll_history_owner_set (azg, rmonhistindex, rmonhistown, ifname);
      if(ret == 0)
        {
          printf("\n\t Set History Control Owner Successfully\n");
        } else {
          printf("\n\tFailed to set History Control Owner\n");
        }
   }
   else if (!strcasecmp(action, "gethistowner"))
   {
      ret = smi_get_rmon_coll_history_owner(azg, rmonhistindex, val_str);
      if(ret == 0)
        {
          printf("\n\t Got History Owner Successfully:%s\n", val_str);
        } else {
          printf("\n\tFailed to get History Owner\n");
        }
   }
   else if (!strcasecmp(action, "histindxrm"))
   {
      ret = smi_rmon_coll_history_index_remove(azg, rmonhistindex);
      if(ret == 0)
        {
          printf("\n\t Removed Index Successfully\n");
        } else {
          printf("\n\tFailed to remove index\\n");
        }
   }
  else if (!strcasecmp(action, "setalarmint"))
   {
      ret = smi_rmon_set_alarm_interval(azg, rmonalarmindx, rmonalarmint);
      if(ret == 0)
        {
          printf("\n\t Set Alarm Polling Interval Successfully\n");
        } else {
          printf("\n\tFailed to Set Alarm Polling Interval\n");
        }
   }
   else if (!strcasecmp(action, "getalarmint"))
   {
      ret = smi_get_rmon_alarm_interval(azg, rmonalarmindx, &val);
      if(ret == 0)
        {
          printf("\n\t Got Alarm Polling Interval Successfully:%d\n", val);
        } else {
          printf("\n\tFailed to get Alarm Polling Interval\n");
        }
  }
  else if (!strcasecmp(action, "setalarmvar"))
  {
      ret = smi_rmon_set_alarm_variable(azg, rmonalarmindx, temp_oidval);
      if(ret == 0)
        {
          printf("\n\t Set Alarm Variable Successfully\n");
        } else {
          printf("\n\tFailed to Set Alarm Variable\n");
        }
  }
  else if (!strcasecmp(action, "getalarmvar"))
  {
      ret = smi_get_rmon_alarm_variable(azg, rmonalarmindx, get_oid);
      if(ret == 0)
        {
          printf("\n\t Got Alarm Variable Successfully\n");
        } else {
          printf("\n\tFailed to get Alarm Variable\n");
        }
  }
 else if (!strcasecmp(action, "setalarmtype"))
  {
      ret = smi_rmon_set_alarm_sample_type(azg, rmonalarmindx, rmonalarmtype);
      if(ret == 0)
        {
          printf("\n\t Set Alarm Type Successfully\n");
        } else {
          printf("\n\tFailed to Set Alarm Type\n");
        }
  }
  else if (!strcasecmp(action, "getalarmtype"))
  {
      ret = smi_get_rmon_alarm_sample_type(azg, rmonalarmindx, &val);
      if(ret == 0)
        {
          printf("\n\t Got Alarm Type Successfully:%d\n", val);
        } else {
          printf("\n\tFailed to get Alarm Type\n");
        }
  }

  else if (!strcasecmp(action, "setalarmstartup"))
  {
      ret = smi_rmon_set_alarm_start_up(azg, rmonalarmindx, rmonalarmstartup);
      if(ret == 0)
        {
          printf("\n\t Set Alarm Startup Type Successfully\n");
        } else {
          printf("\n\tFailed to Set Alarm Startup Type\n");
        }
  }
  else if (!strcasecmp(action, "getalarmstartup"))
  {
      ret = smi_get_rmon_alarm_start_up(azg, 
rmonalarmindx, &val);
      if(ret == 0)
        {
          printf("\n\t Got Alarm Startup Type Successfully:%d\n", val);
        } else {
          printf("\n\tFailed to get Alarm Startup Type\n");
        }
  }
  else if (!strcasecmp(action, "setriseths"))
  {
      ret = smi_rmon_set_alarm_rising_threshold(azg, rmonalarmindx, 
                                                rmonrisingval);
      if(ret == 0)
        {
          printf("\n\t Set Alarm Rising Threshold Successfully\n");
        } else {
          printf("\n\tFailed to Set Alarm Rising Threshold\n");
        }
  }
  else if (!strcasecmp(action, "getriseths"))
  {
      ret = smi_get_rmon_alarm_rising_threshold(azg, rmonalarmindx, &val_int32);
      if(ret == 0)
        {
         /*printf("\n\t Got Alarm Rising Threshold: %d\n", val_int32); */
          printf("\n\t Got Alarm Rising Threshold Successfully\n");
        } else {
          printf("\n\tFailed to get Alarm Rising Threshold\n");
        }
  }
  else if (!strcasecmp(action, "setfallths"))
  {
      ret = smi_rmon_set_alarm_falling_threshold(azg, rmonalarmindx, 
                                                 rmonfallval);
      if(ret == 0)
        {
          printf("\n\t Set Alarm Falling Threshold Successfully\n");
        } else {
          printf("\n\tFailed to Set Alarm Falling Threshold\n");
        }
  }
  else if (!strcasecmp(action, "getfallths"))
  {
      ret = smi_rmon_get_alarm_falling_threshold(azg, rmonalarmindx, 
                                                 &val_int32);
      if(ret == 0)
        {
          printf("\n\t Got Alarm Rising Threshold: %d\n", val_int32.l[0]); 
        } else {
          printf("\n\tFailed to get Alarm Rising Threshold\n");
        }
  }
 else if (!strcasecmp(action, "seteventrisths"))
  {
      ret = smi_rmon_set_alarm_rising_event_index(azg, rmonalarmindx, 
                                                  rmonrisingindx);
      if(ret == 0)
        {
          printf("\n\t Set Event for Rising Threshold Alarms Successfully\n");
        } else {
          printf("\n\tFailed to Set Event for Rising Threshold Alarms\n");
        }
  }
  else if (!strcasecmp(action, "getevenrisths"))
  {
      ret = smi_get_rmon_alarm_rising_event_index(azg, rmonalarmindx, &val);
      if(ret == 0)
        {
          printf("\n\t Got Event for Rising Threshold Alarms: %d\n", val);
        } else {
          printf("\n\tFailed to get Event for Rising Threshold Alarms\n");
        }
  }
  else if (!strcasecmp(action, "seteventfallths"))
  {
      ret = smi_rmon_set_alarm_falling_event_index(azg, rmonalarmindx, 
                                                   rmonfallingindx);
      if(ret == 0)
        {
          printf("\n\t Set Event for Falling Threshold Alarms Successfully\n");
        } else {
          printf("\n\tFailed to Set event for Falling Threshold Alarms\n");
        }
  }
  else if (!strcasecmp(action, "geteventfallths"))
  {
      ret = smi_rmon_get_alarm_falling_event_index(azg, rmonalarmindx, &val);
      if(ret == 0)
        {
          printf("\n\t Got Event for Falling Threshold Alarms :%d\n", val);
        } else {
          printf("\n\tFailed to get event for Falling Threshold Alarms\n");
        }
  }
 else if (!strcasecmp(action, "setalarmowner"))
  {
      ret = smi_rmon_set_alarm_owner(azg, rmonalarmindx, rmonalarmown);
      if(ret == 0)
        {
          printf("\n\t Set Alarm Owner Successfully\n");
        } else {
          printf("\n\tFailed to Set Owner\n");
        }
  }
  else if (!strcasecmp(action, "getalarmowner"))
  {
      char str_owner[SMI_RMON_OWNER_NAME_SIZE];
      memset(str_owner, 0, SMI_RMON_OWNER_NAME_SIZE);
      ret = smi_rmon_get_alarm_owner(azg, rmonalarmindx, str_owner);
      if(ret == 0)
        {
          printf("\n\t Got alarm owner Successfully:%s\n", str_owner);
        } else {
          printf("\n\tFailed to get owner\n");
        }
  }

  else if (!strcasecmp(action, "setalarmentry"))
  {
      ret = smi_rmon_set_alarm_entry(azg, rmonalarmindx, "etherStatsEntry.6.4", 
                                     rmonalarmint,
                                     rmonrisingval, rmonfallval, rmonrisingindx,                                                                                  rmonfallingindx, rmonalarmown);
      if(ret == 0)
        {
          printf("\n\t Added Alarm entry Successfully\n");
        } else {
          printf("\n\tFailed to add the selected Alarm entry\n");
        }
  }
  else if (!strcasecmp(action, "getalarmentry"))
  {
     struct smi_alarm_entry alarmentry;
      ret = smi_rmon_get_alarm_entry(azg, rmonalarmindx, &alarmentry);
      if(ret == 0)
        {
          printf("\n\t Got Selected alarm entry Successfully:%d\n", 
                  alarmentry.interval);
          printf("\n Alarm Interval = %d\n", alarmentry.interval);
          printf("\n Alarm Word = %s\n", alarmentry.alarmVariableWord);
          printf("\n Alarm Rising Threshold= %d\n", 
                    alarmentry.risingValue.l[0]);
          printf("\n Alarm Falling Threshold = %d\n", 
                     alarmentry.fallingValue.l[0]);
          printf("\n Alarm RisingEvent Indx = %d\n", alarmentry.rising_event);
          printf("\n Alarm FallingEvent Indx = %d\n",alarmentry.falling_event);
          printf("\n Alarm Owner = %s\n",alarmentry.ownername);
        } else {
          printf("\n\tFailed to get the selected Alarm entry\n");
        }
  }
  else if (!strcasecmp(action, "setalarmindxrm"))
  {
      ret = smi_rmon_alarm_index_remove(azg, rmonalarmindx);
      if(ret == 0)
        {
          printf("\n\t Selected Alarm entry removed Successfully\n");
        } else {
          printf("\n\tFailed to remove the Alarm Entry\n");
        }
  } else if (!strcasecmp(action, "setalarmstatus"))
  {
      ret = smi_rmon_set_alarm_status (azg, rmonalarmindx,
                                       rmonalarmstatus);
      if(ret == 0)
        {
          printf("\n\t Selected Alarm entry removed Successfully\n");
        } else {
          printf("\n\tFailed to remove the Alarm Entry\n");
        }
  }
  else if (!strcasecmp(action, "seteventindxrm"))
  {
      ret = smi_rmon_event_index_remove(azg, rmonevtindx);
      if(ret == 0)
        {
          printf("\n\tSelected entry removed Successfully:\n");
        } else {
          printf("\n\tFailed to remove the selected entry\n");
        }
  }
 else if (!strcasecmp(action, "seteventindx"))
  {
      ret = smi_rmon_set_event_index(azg, rmonevtindx);
      if(ret == 0)
        {
          printf("\n\t Set Event Index Successfully\n");
        } else {
          printf("\n\tFailed to Set event Index\n");
        }
  }
 else if (!strcasecmp(action, "geteventindx"))
  {
      int j=0;
      u_int32_t rmon_event_indices[30];
      memset(rmon_event_indices, 0, sizeof(rmon_event_indices));
      ret = smi_rmon_get_event_index(azg, rmon_event_indices);
      if(ret == 0)
        {
          printf("\n\t Got Event Index Successfully:%d\n", val);
          while (*(rmon_event_indices+j) != 0)
           printf (" %d ",*(rmon_event_indices + j++));
        } else {
          printf("\n\tFailed to get event Index\n");
        }
     
          printf("\n\t Got Event Index Successfully:%d\n", val);
  }

 else if (!strcasecmp(action, "seteventactive"))
  {
      ret = smi_rmon_set_event_active(azg, rmonevtindx);
      if(ret == 0)
        {
          printf("\n\t Set Event active Successfully\n");
        } else {
          printf("\n\tFailed to Set event to active state\n");
        }
  }

  else if (!strcasecmp(action, "seteventstatus"))
  {
      ret = smi_rmon_set_event_status(azg, rmonevtindx,rmoneventstatus);
      if(ret == 0)
        {
          printf("\n\t Event Status Set  Successfully:%d\n", rmoneventstatus);
        } else {
          printf("\n\tFailed to event status\n");
        }

  }
  else if (!strcasecmp(action, "geteventstatus"))
  {
      ret = smi_rmon_get_event_status(azg, rmonevtindx,&val);
      if(ret == 0)
        {
          printf("\n\t Got Event Status Successfully:%d\n", val);
        } else {
          printf("\n\tFailed to get event status\n");
        }
  }
 else if (!strcasecmp(action, "seteventcomm"))
  {
      ret = smi_rmon_set_event_comm(azg, rmonevtindx, rmonevtcomm);
      if(ret == 0)
        {
          printf("\n\t Set Event Community Successfully\n");
        } else {
          printf("\n\tFailed to Set event Community\n");
        }
  }
  else if (!strcasecmp(action, "geteventcomm"))
  {
      char str_comm[SMI_RMON_COMM_LENGTH+1];
      memset(str_comm, 0, SMI_RMON_COMM_LENGTH);
      ret = smi_rmon_get_event_comm(azg, rmonevtindx, str_comm);
      if(ret == 0)
        {
          printf("\n\t Got Event Community Successfully:%s\n", str_comm);
        } else {
          printf("\n\tFailed to get event community\n");
        }
  }
  else if (!strcasecmp(action, "seteventdesc"))
  {
      ret = smi_rmon_set_event_description(azg, rmonevtindx, rmonevtdesc);
      if(ret == 0)
        {
          printf("\n\t Set Event Description Successfully\n");
        } else {
          printf("\n\tFailed to Set event Description\n");
        }
  }
  else if (!strcasecmp(action, "geteventdesc"))
  {
      ret = smi_rmon_get_event_description(azg, rmonevtindx, val_str);
      if(ret == 0)
        {
          printf("\n\t Got Event Description Successfully:%s\n", val_str);
        } else {
          printf("\n\tFailed to get event Description\n");
        }
  }
 else if (!strcasecmp(action, "seteventowner"))
  {
      ret = smi_rmon_set_event_owner(azg, rmonevtindx, rmonevetown);
      if(ret == 0)
        {
          printf("\n\t Set Event Owner Successfully\n");
        } else {
          printf("\n\tFailed to Set event Owner\n");
        }
  }
  else if (!strcasecmp(action, "geteventowner"))
  {
      char str_owner[SMI_RMON_OWNER_NAME_SIZE+1];
      memset(str_owner, 0, SMI_RMON_OWNER_NAME_SIZE);
      ret = smi_rmon_get_event_owner(azg, rmonevtindx, str_owner);
      if(ret == 0)
        {
          printf("\n\t Got Event Owner Successfully: %s\n", str_owner);
        } else {
          printf("\n\tFailed to get event Owner\n");
        }
  } else if(!strcasecmp(action, "validstats"))
    {
      ret = smi_rmon_coll_stats_validate(azg, rmonethstindex, ifname);
      if(ret == 0)
        {
          printf("\n\t Collection is enabled:\n");
        } else {
          printf("\n\tCollection is not enabled\n");
        }
    } else if(!strcasecmp(action, "addstats"))
    {
      ret = smi_rmon_collection_stat_entry_add(azg, ifname, 
                                               rmonethstindex, 
                                               rmonethstatsown);
      if(ret == 0)
        {
          printf("\n\t Stat Entry Added Successfully:\n");
        } else {
          printf("\n\tAdding Failed:\n");
        }
    } else if(!strcasecmp(action, "removestat"))
    {
      ret = smi_rmon_collection_stat_entry_remove(azg, ifname, 
                                                  rmonethstindex);
      if(ret == 0)
        {
          printf("\n\t Stat Entry Removed Successfully:\n");
        } else {
          printf("\n\tRemoval Failed:\n");
        }
     } else if(!strcasecmp(action, "validhist"))
     {
       ret = smi_rmon_coll_history_validate(azg,rmonhistindex, ifname);
       if(ret == 0)
         {
           printf("\n\t History Call Parameter are set:\n");
         } else {
           printf("\n\t History Call Parameter are not set:\n");
         }
   } else if(!strcasecmp(action, "sethistory"))
     {
       ret = smi_rmon_coll_history_set_active(azg, rmonhistindex);
       if(ret == 0)
         {
           printf("\n\t History Control Entry set Successfully:\n");
         } else {
           printf("\n\t History Control Entry setting failed:\n");
         }
     } else if(!strcasecmp(action, "gethistory"))
     {
       ret = smi_get_rmon_coll_history_status(azg, rmonhistindex, &val);
       if(ret == 0)
         {
           printf("\n\t History Control Entry :%d\n", val);
         } else {
           printf("\n\t History Control Entry getting failed:\n");
         }
     } else if(!strcasecmp(action, "setbucket"))
     {
       ret = smi_rmon_coll_history_bucket_set(azg, rmonhistindex, rmonbucket, ifname);
       if(ret == 0)
         {
           printf("\n\t Bucket for History Control Entry set Successfully:\n");
         } else {
           printf("\n\t Bucket setting failed:\n");
         }
     } else if(!strcasecmp(action, "getbucket"))
     {
       ret = smi_get_rmon_coll_history_bucket(azg, rmonhistindex, &val);
       if(ret == 0)
         {
           printf("\n\t Bucket :%d\n", val);
         } else {
           printf("\n\t Bucket getting failed:\n");
         }

   } else if(!strcasecmp(action, "sethisctrl"))
     {
       ret = smi_rmon_coll_history_set_inactive(azg, rmonhistindex);
       if(ret == 0)
         {
           printf("\n\t History Control Entry set to inactive state\n");
         } else {
           printf("\n\t History Control Entry setting failed:\n");
         }
     } else if(!strcasecmp(action, "addhisctrl"))
     {
       ret = smi_rmon_coll_history_index_add_new(azg, rmonhistindex, ifname); 
       if(ret == 0)
         {
           printf("\n\t History Control Entry added Successfully:\n");
         } else {
           printf("\n\t History Control Entry adding failed:\n");
         }
      } else if(!strcasecmp(action, "setdsource"))
      {
        ret = smi_rmon_coll_history_datasource_set(azg, rmonhistindex, 
                                                   ifname);
        if(ret == 0)
         {
           printf("\n\t Data Source value set Successfully:\n");
         } else {
           printf("\n\t Setting Failed:\n");
         }
     } else if(!strcasecmp(action, "sethisindex"))
      {
        ret = smi_rmon_coll_history_index_set(azg, rmonhistindex, ifname);
        if(ret == 0)
         {
           printf("\n\tSet Successfully:\n");
         } else {
           printf("\n\t Setting Failed:\n");
         }

  } else if(!strcasecmp(action, "gethisindex"))
     {
       ret = smi_get_rmon_coll_history_index(azg, rmonhistindex, &val);
       if(ret == 0)
         {
           printf("\n\t History Index:%d\n", val);
         } else {
           printf("\n\t History Index getting failed:\n");
         }
     } else if(!strcasecmp(action, "setevent"))
      {
        ret = smi_rmon_event_type_set(azg, rmonevtindx, rmonevttype);
        if(ret == 0)
         {
           printf("\n\tEvent Type Set Successfully:\n");
         } else {
           printf("\n\t Setting Failed:\n");
         }
     } else if(!strcasecmp(action, "getevent"))
     {
       ret = smi_rmon_event_type_get(azg, rmonevtindx, &val);
       if(ret == 0)
         {
           printf("\n\t Event Type:%d\n", val);
         } else {
           printf("\n\t Event Type getting failed:\n");
         }
     } else if(!strcasecmp(action, "setsevent"))
      {
        ret = smi_rmon_snmp_set_event_type(azg, rmonevtindx, rmonsnmpevttype);
        if(ret == 0)
         {
           printf("\n\tSet Successfully:\n");
         } else {
           printf("\n\t Setting Failed:\n");
         }
   } else if(!strcasecmp(action, "getsevent"))
     {
       ret = smi_rmon_snmp_get_event_type(azg, rmonevtindx, &val);
       if(ret == 0)
         {
           printf("\n\t Event Type:%d\n", val);
         } else {
           printf("\n\t Event Type getting failed:\n");
         }
     }  else if(!strcasecmp(action, "setscomm"))
      {
        ret = smi_rmon_snmp_set_event_community(azg, rmonevtindx, 
                                                rmonsnmpevtcomm);
        if(ret == 0)
         {
           printf("\n\tSet Successfully:\n");
         } else {
           printf("\n\t Setting Failed:\n");
         }
     } else if(!strcasecmp(action, "getscomm"))
     {
       char com[SMI_RMON_COMM_LENGTH];
       ret = smi_rmon_snmp_get_event_community(azg, rmonevtindx, com);
       if(ret == 0)
         {
           printf("\n\t Community:%s\n", com);
         } else {
           printf("\n\t Community name getting failed:\n");
         }
     } else if(!strcasecmp(action, "setsowner"))
      {
        ret = smi_rmon_snmp_set_event_owner(azg, rmonevtindx, rmonsnmpevtown);
        if(ret == 0)
         {
           printf("\n\tSet Successfully:\n");
         } else {
           printf("\n\t Setting Failed:\n");
         }

    } else if(!strcasecmp(action, "getsowner"))
     {
       char owner[SMI_RMON_OWNER_NAME_SIZE];
       ret = smi_rmon_snmp_get_event_owner(azg, rmonevtindx, owner);
       if(ret == 0)
         {
           printf("\n\t Community:%s\n", owner);
         } else {
           printf("\n\t Community name getting failed:\n");
         }
     } else if(!strcasecmp(action, "setether"))
      {
        ret = smi_rmon_snmp_set_ether_stats_status(azg, rmonethstindex, 
                                                   rmonethstatus); 
        if(ret == 0)
         {
           printf("\n\tEther Stats Status set Successfully:\n");
         } else {
           printf("\n\t Setting Failed:\n");
         }
     } else if(!strcasecmp(action, "getether"))
     {
       ret = smi_rmon_snmp_get_ether_stats_status(azg, rmonethstindex, &val);
       if(ret == 0)
         {
           printf("\n\t Ether Stats status:%d\n", val);
         } else {
           printf("\n\t Ether status getting failed:\n");
         }
     } else if(!strcasecmp(action, "setdesc"))
      {
        ret = smi_rmon_snmp_set_event_description(azg, rmonevtindx, 
                                                  rmonsnmpevtdesc);
        if(ret == 0)
         {
           printf("\n\tSet Successfully:\n");
         } else {
           printf("\n\t Setting Failed:\n");
         }
     } else if(!strcasecmp(action, "getdesc"))
      {
       char desc[SMI_RMON_DESCR_LENGTH];
       ret = smi_rmon_snmp_get_event_description(azg, rmonevtindx, desc);
       if(ret == 0)
         {
           printf("\n\t Description:%s\n", desc);
         } else {
           printf("\n\t Description getting failed:\n");
         }
    } else if(!strcasecmp(action, "flushport"))
      {
        ret = smi_rmon_stats_flush_port(azg, ifname);
        if(ret == 0)
         {
           printf("\n\tSet Successfully:\n");
         } else {
           printf("\n\t Setting Failed:\n");
         }
    } else if(!strcasecmp(action, "flushallport"))
      {
        ret = smi_rmon_stats_flush_all_port(azg);
        if(ret == 0)
         {
           printf("\n\t flush all port : \n");
         } else {
           printf("\n\t failed: \n");
         }
      } else if(!strcasecmp(action, "getifcount"))
     {
       enum smi_rmon_stats_counter rmon_counter = 1;
        ret = smi_rmon_get_if_counter(azg, ifname,  rmon_counter, &val);
        if(ret == 0)
          {
            printf("\n\t Counter :%d \n",val);
          } else {
            printf("\n\t Counter getting failed:\n");
          }
       }

    return 0;

}

/* LLDP related APIs */
int
smi_lldp_action (struct smiclient_globals *azg, char *action)
{
  int ret = 0;
  int val = 0;

  if(!strcasecmp(action, "disableport"))
    {
      ret = smi_lldp_api_port_disable(azg, ifname);
      if(ret == 0)
        {
          printf("\n\t Port Disabled Successfully:\n");
        } else {
          printf("\n\tPort Disabling Failed\n");
        }
   } else if(!strcasecmp(action, "enableport"))
   {
      ret = smi_lldp_api_port_enable(azg, ifname, admin_status);
      if(ret == 0)
        {
          printf("\n\t Port enabled Successfully:\n");
        } else {
          printf("\n\tPort enabling Failed\n");
        }
    } else if(!strcasecmp(action, "setlstring"))
    {
      ret = smi_lldp_port_set_locally_assigned(azg, ifname, localstring);
      if(ret == 0)
        {
          printf("\n\tString Set Successfully:\n");
        } else {
          printf("\n\tString setting Failed\n");
        }
    } else if(!strcasecmp(action, "getlstring"))
    {
      u_char name[LLDP_LOCAL_MAX_LEN];
      ret = smi_lldp_port_get_locally_assigned(azg, ifname, name);
      if(ret >= 0)
        {
          printf("\n\tString Returned: %s ", name);
        } else {
          printf("\n\tString getting Failed\n");
        }
    } else if(!strcasecmp(action, "setporttlv"))
    { /* 
         flags - IEEE_8021_ORG_SPECIFIC_TLV_TX_ENABLE,
                 IEEE_8023_ORG_SPECIFIC_TLV_TX_ENABLE
      */
                 
      ret = smi_lldp_set_port_basic_tlvs_enable(azg, ifname, 
                              IEEE_8021_ORG_SPECIFIC_TLV_TX_ENABLE);
      if(ret == 0)
        {
          printf("\n\tTLVs enabled Successfully:\n");
        } else {
          printf("\n\tTLVs enabling Failed\n");
        }
    } else if(!strcasecmp(action, "getporttlv"))
    {
      u_int16_t flag = 0;
      ret = smi_lldp_get_port_basic_tlvs_enable(azg, ifname, &flag);
      if(ret >= 0)
        {
          printf("\n\tTlv flag Returned: %d\n", flag);
        } else {
          printf("\n\tTlv flag getting Failed\n");
        }
    } else if(!strcasecmp(action, "setptxhold"))
    {
      ret = smi_lldp_set_port_msg_tx_hold(azg, ifname, tx_hold);
      if(ret == 0)
        {
          printf("\n\tTx hold set Successfully:\n");
        } else {
          printf("\n\tTx hold setting Failed\n");
        }
    } else if(!strcasecmp(action, "getptxhold"))
    {
      ret = smi_lldp_get_port_msg_tx_hold(azg, ifname, &val);
      if(ret >= 0)
        {
          printf("\n\tTx hold Returned: %d\n", val);
        } else {
          printf("\n\tTxhold getting Failed\n");
        }
    } else if(!strcasecmp(action, "settxinterval"))
    {
      ret = smi_lldp_set_port_msg_tx_interval(azg, ifname, tx_interval);
      if(ret == 0)
        {
          printf("\n\tTime interval set Successfully:\n");
        } else {
          printf("\n\tTime interval setting Failed\n");
        }
    } else if(!strcasecmp(action, "gettxinterval"))
    {
      ret = smi_lldp_get_port_msg_tx_interval(azg, ifname, &val);
      if(ret >= 0)
        {
          printf("\n\tTx interval Returned: %d\n", val);
        } else {
          printf("\n\tTx interval getting Failed\n");
        }
    } else if(!strcasecmp(action, "setreinitdelay"))
    {
      ret = smi_lldp_set_port_reinit_delay(azg, ifname, re_delay);
      if(ret == 0)
        {
          printf("\n\tDelay time set Successfully:\n");
        } else {
          printf("\n\tDelay time setting Failed\n");
        }
    } else if(!strcasecmp(action, "getreinitdelay"))
    {
      ret = smi_lldp_get_port_reinit_delay(azg, ifname, &val);
      if(ret >= 0)
        {
          printf("\n\tDelay time Returned: %d\n", val);
        } else {
          printf("\n\tdelay time getting Failed\n");
        }
    } else if(!strcasecmp(action, "setneighbr"))
    {
      ret = smi_lldp_set_port_too_many_neighbors (azg, ifname, neighb_limit,
                                             TOO_MANY_NEIGHBOURS_RECIEVED_INFO,
                                             NULL, neighb_interval);
      if(ret == 0)
        {
          printf("\n\tNeighbours set Successfully:\n");
        } else {
          printf("\n\tNeighbours setting Failed\n");
        }
    } else if(!strcasecmp(action, "getneighbr"))
    {
      u_int32_t inter = 0;
      ret = smi_lldp_get_port_too_many_neighbors (azg, ifname, &val,
                                              TOO_MANY_NEIGHBOURS_RECIEVED_INFO,
                                              NULL, &inter);
      if(ret >= 0)
        {
          printf("\n\tMax Limit: %d\n", val);
          printf("\n\tInterval : %d\n", inter);
        } else {
          printf("\n\tNeighbour getting Failed\n");
        }
    } else if(!strcasecmp(action, "settxdelay"))
    {
      ret = smi_lldp_set_port_tx_delay(azg, ifname, tx_delay);
      if(ret == 0)
        {
          printf("\n\tTxDelay set Successfully:\n");
        } else {
          printf("\n\tTxDelay setting Failed\n");
        }
    } else if(!strcasecmp(action, "gettxdelay"))
    {
      ret = smi_lldp_get_port_tx_delay(azg, ifname, &val);
      if(ret >= 0)
        {
          printf("\n\tTxDelay Returned: %d\n", val);
        } else {
          printf("\n\tTxdelay getting Failed\n");
        }
    } else if(!strcasecmp(action, "setsysdescr"))
    {
      ret = smi_lldp_set_system_description(azg, descriptor);
      if(ret == 0)
        {
          printf("\n\tSystem descriptor set Successfully:\n");
        } else {
          printf("\n\tSystem Descriptor setting Failed\n");
        }
    } else if(!strcasecmp(action, "getsysdescr"))
    {
      u_char descr[LLDP_DESCR_MAX_LEN];
      ret = smi_lldp_get_system_description(azg, descr);
      if(ret >= 0)
        {
          printf("\n\tsystem descriptor : %s\n", descr);
        } else {
          printf("\n\tsystem descriptor getting Failed\n");
        }
    } else if(!strcasecmp(action, "setsysname"))
    {
      ret = smi_lldp_set_system_name(azg, systemname);
      if(ret == 0)
        {
          printf("\n\tSystem name set Successfully:\n");
        } else {
          printf("\n\tSystem name setting Failed\n");
        }
    } else if(!strcasecmp(action, "getsysname"))
    {
      u_char sys_name[LLDP_NAME_MAX_LEN];
      ret = smi_lldp_get_system_name(azg, sys_name);
      if(ret >= 0)
        {
          printf("\n\tsystem name : %s\n", sys_name);
        } else {
          printf("\n\tsystem name getting Failed\n");
        }
    } else if(!strcasecmp(action, "getremmac"))
    {
      char mac_addr[6];
      int i,j;
      char rem_mac_arr[SMI_NUM_REC][SMI_ETHER_ADDR_LEN];
      pal_mem_set (rem_mac_arr, 0, sizeof(char)*SMI_NUM_REC*SMI_ETHER_ADDR_LEN);
      hwaddr_aton(hwaddr, mac_addr);
      printf("Input mac:\n");
      ret = smi_lldp_get_rem_macs_on_port (azg, ifname, rem_mac_arr, 1, 
                                           mac_addr);
      if(ret >= 0)
      {
        printf("\n\tSuccessfull\n");
        for(j = 0; j < 10; j++)
        {
          for(i = 0; i < SMI_ETHER_ADDR_LEN; i++)
          {
            printf("\t%d - %2x\n", i, (char)rem_mac_arr[j][i]);
          }
          printf("\n");
        } 
      }else {
          printf("\n\tgetting Failed\n");
        }
    } else if(!strcasecmp(action, "getport"))
    {
      struct smi_remote_lldp lpmsg;
      char mac_addr[6];
      int i;
      hwaddr_aton(hwaddr, mac_addr); 
      printf("Input mac:\n");
      for(i = 0; i < 6; i++)
      {
        printf("\t%d - %2x\n", i, (char)mac_addr[i]);
      }
      printf("\n");

      /* Note: Need to pass the remote (neighbour) MAC address to get the
               port details 3rd parameter below*/
      ret = smi_lldp_get_port(azg, ifname, mac_addr, &lpmsg);
      if(ret >= 0)
        {
          printf("\n\tSuccessfull\n");
          printf("\n\tremote_mac_addr: %x%x.%x%x.%x%x", lpmsg.remote_mac_addr[0],
                      lpmsg.remote_mac_addr[1], lpmsg.remote_mac_addr[2],
                      lpmsg.remote_mac_addr[3], lpmsg.remote_mac_addr[4],
                      lpmsg.remote_mac_addr[5]);
         printf("\n\tTTL: %d", lpmsg.rx_ttl);
         printf("\n\tInterface Number: %d", lpmsg.remote_if_number);
         printf("\n\tPort Vlan ID: %d", lpmsg.remote_port_vlan_id);
         printf("\n\tAutoNego Support: ");

         if (lpmsg.remote_autonego_support == IF_MAU_AUTONEG_SUPPORTED)
           printf("Supported");
         else if(lpmsg.remote_autonego_support == IF_MAU_AUTONEG_ENABLED)
           printf("Enabled");
         else
           printf("Not Supported");

         printf("\n\tAutoNego Capability: %d", lpmsg.remote_autonego_cap);
         printf("\n\tOperational MAU Type: %d", lpmsg.remote_oper_mau_type);
         printf("\n\tLink Aggregation Capability: ");

         if (lpmsg.remote_link_aggr_status == ONM_AGGREGATION_CAPABLE)
           printf("Capable");
         else
           printf("Not Capable");

         printf("\n\tLink Aggregation Status: ");

         if (lpmsg.remote_link_aggr_status == ONM_AGGREGATION_ENABLE)  
           printf("Enabled");
         else
           printf("Disabled");
         printf("\n\tLink Aggregation Port ID: %d", lpmsg.remote_link_aggr_id);
         printf("\n\tMax Frame Size: %d", lpmsg.remote_max_frame_size);
         printf("\n\tSystem Capabilities: %d", lpmsg.remote_sys_cap);
         printf("\n\tSystem Capabilities Enabled: %d", lpmsg.remote_sys_cap_enabled);
        } else {
          printf("\n\tgetting Failed\n");
        }
    } else if(!strcasecmp(action, "getportstat"))
    {
      struct smi_port_lldp_statistics llpmsg;
      ret = smi_lldp_get_port_statistics(azg, ifname, &llpmsg);
      if(ret >= 0)
        {
          printf("\n\tSuccessfull\n");
          printf("\n\tLLDP frames Transmitted : %d", llpmsg.frames_out_total);
          printf("\n\tLLDP frames aged out : %d", llpmsg.ageouts_total);
          printf("\n\tLLDP frames discarded : %d", llpmsg.frames_discarded_total);
          printf("\n\tLLDP frames with error:%d", llpmsg.frames_in_errors_total);
          printf("\n\tLLDP frames received : %d", llpmsg.frames_in_total);
          printf("\n\tLLDP TLVs discarded : %d", llpmsg.tlvs_discarded_total);
          printf("\n\tunrecognized TLVs : %d", llpmsg.tlvs_unrecognized_total);
        } else {
          printf("\n\tgetting Failed\n");
        }
    } else if(!strcasecmp(action, "sethwaddr"))
    {
      char mac_addr[SMI_ETHER_ADDR_LEN];
      int i;
      hwaddr_aton(hwaddr, mac_addr);
      printf("Input mac:\n");
      for(i = 0; i < SMI_ETHER_ADDR_LEN; i++)
      {
        printf("\t%d - %2x\n", i, (char)mac_addr[i]);
      }
      printf("\n");

      ret = smi_lldp_set_hwaddr(azg, mac_addr);

      if(ret == 0)
        {
          printf("\n\tHwaddr addr set Successfully:\n");
        } else {
          printf("\n\tHwaddr addr setting Failed\n");
        }
    } else if(!strcasecmp(action, "gethwaddr"))
    {
      char addr[SMI_ETHER_ADDR_LEN];

      ret = smi_lldp_get_hwaddr(azg, addr);

      if(ret >= 0)
        {
          printf("\n\tHardware addr : %x%x.%x%x.%x%x\n", addr[0],
                     addr[1], addr[2], addr[3], addr[4], addr[5]);
        } else {
          printf("\n\t Hardware addr getting Failed\n");
        }
    } else if(!strcasecmp(action, "setchassisidtype"))
    {
      ret = smi_lldp_set_chassis_id_type (azg, ifname, lldpchassisidtype);
      if(ret == 0)
        {
          printf("\n\tChassis id type set Successfully:\n");
        } else {
          printf("\n\tChassis id type setting Failed\n");
        }
    } else if(!strcasecmp(action, "getchassisidtype"))
    {
      enum smi_lldp_chassis_id_type type = 0;
      ret = smi_lldp_get_chassis_id_type (azg, ifname, &type);
      if(ret == 0)
        {
          printf("\n\tChassis id type get Successfully: %d\n", type);
        } else {
          printf("\n\tChassis id type getting Failed\n");
        }
    } else if(!strcasecmp(action, "setchassisip"))
    {
      ret = smi_lldp_set_chassis_ip_address (azg, lldpchassisip);
      if(ret == 0)
        {
          printf("\n\tChassis ip set Successfully:\n");
        } else {
          printf("\n\tChassis ip setting Failed\n");
        }
    } else if(!strcasecmp(action, "getchassisip"))
    {
      char ipaddr[SMI_IPADDRESS_SIZE];
      ret = smi_lldp_get_chassis_ip_address (azg, ipaddr);
      if(ret == 0)
        {
          printf("\n\tChassis ip get Successfully: %s\n", ipaddr);
        } else {
          printf("\n\tChassis ip getting Failed\n");
        }
    }
  return 0;
}

/* CFM */
int
smi_cfm_action (struct smiclient_globals *azg, char *action)
{
  int ret = 0;

  if(!strcasecmp(action, "addma"))
    {
      ret = smi_cfm_add_ma (azg, brname, cfmmdname,cfmmaname, cfmvid);
      if(ret == 0)
        {
          printf("\n\t MA added Successfully:\n");
        } else {
          printf("\n\tMA adding Failed\n");
        }
    } else if(!strcasecmp(action, "addmd"))
    {
      ret = smi_cfm_add_md (azg, brname, cfmmdname, cfmlevel);
      if(ret == 0)
        {
          printf("\n\t MD added Successfully:\n");
        } else {
          printf("\n\tMd adding Failed\n");
        }
    } else if(!strcasecmp(action, "addmep"))
    {
      ret = smi_cfm_add_mep (azg, brname, cfmmepid, cfmlevel, cfmvid,
                             "UP", ifname);
      if(ret == 0)
        {
          printf("\n\t MEP added Successfully:\n");
        } else {
          printf("\n\tMEP adding Failed\n");
        }
     } else if(!strcasecmp(action, "addmip"))
     {
      ret = smi_cfm_add_mip(azg, brname, cfmlevel, ifname);
      if(ret == 0)
        {
          printf("\n\t MIP added Successfully:\n");
        } else {
          printf("\n\t MIP adding Failed\n");
        }
     } else if(!strcasecmp(action, "addrmep"))
     {
      ret = smi_cfm_add_rmep (azg, brname, cfmmdname, cfmrmepid, cfmvid, 
                              hwaddr);
      if(ret == 0)
        {
          printf("\n\t RMEP added Successfully:\n");
        } else {
          printf("\n\t RMEP adding Failed\n");
        }
     } else if(!strcasecmp(action, "enablecc"))
     {
       ret = smi_cfm_cc_enable(azg, brname, cfmlevel, cfmvid);
       if(ret == 0)
        {
          printf("\n\t enabled Successfully:\n");
        } else {
          printf("\n\t enabling Failed\n");
        }
     } else if(!strcasecmp(action, "getma"))
     {
       struct smi_cfm_ma ma;
       ret = smi_cfm_ma_get(azg, brname, cfmmdname, cfmmaname, cfmvid, &ma);
       if(ret >= 0)
        {
          printf("\n\t Returned : md_name %s\n level %d /n",
                  ma.md_name, ma.level);
        } else {
          printf("\n\t Getting Failed\n");
        }
     } else if(!strcasecmp(action, "getmd"))
     {
       struct smi_cfm_md md;
       ret = smi_cfm_md_get(azg, brname, cfmmdname, cfmlevel, &md);
       if(ret >= 0)
        {
          printf("\n\t Returned : md_name %s\n level %d /n",
                  md.md_name, md.level);
        } else {
          printf("\n\t Getting Failed\n");
        }
     } else if(!strcasecmp(action, "getmep"))
     {
       struct smi_cfm_mep mep;
       ret = smi_cfm_mep_get(azg, brname, cfmmepid, cfmlevel, cfmvid, &mep);
       if(ret >= 0)
        {
          printf("\n\t Returned : md_name %s\n level %d /n",
                  mep.md_name, mep.level);
        } else {
          printf("\n\t Getting Failed\n");
        }
     } else if(!strcasecmp(action, "removema"))
     {
       ret = smi_cfm_remove_ma(azg, brname, cfmmdname, cfmmaname, cfmvid);
       if(ret == 0)
        {
          printf("\n\t Removed Successful ");
        } else {
          printf("\n\t Removal Failed \n");
        }
     } else if(!strcasecmp(action, "removemd"))
     {
       ret = smi_cfm_remove_md(azg, brname, cfmmdname, cfmlevel);
       if(ret == 0)
        {
          printf("\n\t Removed Successful ");
        } else {
          printf("\n\t Removal Failed \n");
        }
     } else if(!strcasecmp(action, "removemep"))
     {
       ret = smi_cfm_remove_mep(azg, brname, cfmmepid, cfmlevel, 
                                cfmvid, "UP", ifname);
       if(ret == 0)
        {
          printf("\n\t Removed Successful ");
        } else {
          printf("\n\t Removal Failed \n");
        }
     } else if(!strcasecmp(action, "sendping"))
     {
       struct smi_cfm_lb lb;
       ret = smi_cfm_send_ping(azg, brname, cfmmepid, 
                               cfmmdname, cfmlevel, cfmvid, &lb);
       if(ret >= 0)
        {
          printf("\n\t Returned :level %d md_name %s ", 
                 lb.level, lb.md_name);
        } else {
          printf("\n\t getting Failed \n");
        }
     } else if(!strcasecmp(action, "sendtrace"))
     {
       struct smi_cfm_lt lt;
       ret = smi_cfm_send_traceroute(azg, brname, hwaddr, cfmmdname, 
                                     cfmlevel, cfmvid, &lt);
       if(ret >= 0)
        {
          printf("\n\t Returned :mepid %d \n interface name %s ",
                 lt.mepid, lt.name);
        } else {
          printf("\n\t getting Failed \n");
        }
     } else if(!strcasecmp(action, "getitmep"))
     {
       int mep_up_list[40];
       int mep_down_list[40];
       struct smi_cfm_mep mep; 
       int i=0;
  
       memset (&mep_up_list, 0, sizeof(int)*40);
       memset (&mep_down_list, 0, sizeof(int)*40);
       memset (&mep, 0, sizeof( struct smi_cfm_mep));
       ret = smi_ethernet_cfm_if_mep_list (azg, brname, ifname, 
                                           &mep_up_list,&mep_down_list);

      printf (" The Up List \n");
       while (mep_up_list[i])
        {
          printf (" %d ",mep_up_list[i]);
          i++;
        }
       i = 0;
       printf (" The Down List \n");
       while (mep_down_list[i])
        {
          printf (" %d ",mep_down_list[i]);
          i++;
        }
       i = 0; 
     
       memset (&mep, 0, sizeof( struct smi_cfm_mep));
       printf ("\n Up list Details \n");
       while  (mep_up_list[i])
        {
         printf ("\n mep_up_list[%d] %d\n",i,mep_up_list[i]);       
          ret = smi_cfm_get_mep_info (azg, ifname, brname, mep_up_list[i], &mep);
          printf (" Md Name     %s\n",mep.md_name);
          printf (" Level       %d\n",mep.level);
          printf (" Ma Name     %s\n",mep.ma_name);
          printf (" vid         %d\n",mep.vid);
          printf (" mep_id      %d\n",mep.mep_id);
          printf (" mep_dir     %d\n",mep.mep_dir);
          i++;
        }
       
       memset (&mep, 0, sizeof( struct smi_cfm_mep));
       i=0; 
       printf ("\n Down Mep list Details \n");
       while  (mep_down_list[i])
        {
         printf ("\n mep_down_list[%d] %d\n",i,mep_down_list[i]);
          ret = smi_cfm_get_mep_info (azg, ifname, brname, mep_down_list[i], &mep);
          printf (" Md Name     %s\n",mep.md_name);
          printf (" Level       %d\n",mep.level);
          printf (" Ma Name     %s\n",mep.ma_name);
          printf (" vid         %d\n",mep.vid);
          printf (" mep_id      %d\n",mep.mep_id);
          printf (" mep_dir     %d\n",mep.mep_dir);
          i++;
        }
  
 
       if (ret >=0 )
        {
          printf ("\n Good \n");
         }
       else printf ("\n Error in If Mep List \n");

        #if 0
         ret = smi_ethernet_cfm_iterate_mep(azg, ifname, cfmdoname, cfmvid,
                                            cfmmepid, cfmentries, &mep);
         if(ret >= 0)
          {
            printf("\n\t Returned :level %d \n md_name %s ",
                   mep.level, mep.md_name);
          } else {
          printf("\n\t getting Failed \n");
        }
      #endif
     } else if(!strcasecmp(action, "getitrmep"))
     {
       struct smi_cfm_rmep rmep_info;
       u_int32_t i= 0,rmep_idlist[30];
       memset (&rmep_idlist, 0 , sizeof (u_int32_t) *30); 
       ret = smi_cfm_get_rmep_list (azg, brname, cfmmdname, cfmlevel, cfmvid, 
                                    &rmep_idlist);
       while (rmep_idlist[i])     
        {
          printf (" %d ",rmep_idlist[i]);
          i++;
        }
       i = 0; 
       while (rmep_idlist[i])
        {
          printf ("\n rmep_idlist[%d]\n",i,rmep_idlist[i]); 
          ret = smi_cfm_get_rmep_info (azg, brname, rmep_idlist[i],&rmep_info); 
          if (ret < 0 ) break;
          printf ("\n\t Got the List of rmeps \n");
          printf (" Level     %d \n",rmep_info.level);
          printf (" rmep id   %d \n",rmep_info.rmep_id);
          printf (" Vid       %d \n",rmep_info.vid);
          printf (" Mac Addrs %x %x %x %x %x %x \n",rmep_info.mac_add);
          printf ("rdi        %d \n",rmep_info.rdi);
          i++;
        }

       if(ret >= 0)
        {
          printf ("\n Got all the Rmep Info Successfully \n");
        } else {
          printf("\n\t getting RMEP LIST Failed \n");
        }
     } else if(!strcasecmp(action, "getittrace"))
     {
       struct smi_cfm_lt lt;
       ret = smi_cfm_iterate_traceroute_cache(azg, brname, cfmdoname, 
                                              cfmmepid,
                                              cfmvid, cfmseqno, 
                                              cfmentries, &lt);
       if(ret >= 0)
        {
          printf("\n\t Returned :mepid %d \n Interface name %s ",
                 lt.mepid, lt.name);
        } else {
          printf("\n\t getting Failed \n");
        }
     } else if(!strcasecmp(action, "geterror"))
     {
       unsigned int no_of_entries=0;
       struct smi_cfm_errors cfm_errors;
       ret = smi_cfm_error_entries (azg, brname, cfmmdname, cfmlevel, cfmvid, 
                                    0, &no_of_entries);
       if(ret == 0)
        {
          printf("\n\t Number of Entries for the MD %s and VID %d is %d \n",cfmmdname,cfmvid,no_of_entries);
        } else {
          printf("\n\t Getting numer of  error entries Failed \n");
        }
       memset (&cfm_errors, 0, sizeof (struct smi_cfm_errors));
       ret = smi_cfm_get_error_entry (azg, brname, cfmmdname, cfmlevel, cfmvid, 0, &cfm_errors);
        if ( ret >=0 )
         {
           printf ("\n First Error Entry \n");
           printf ("\n Level %d \n",cfm_errors.level);
           printf ("\n Vid %d\n",cfm_errors.vid);
           printf ("\n mep_id %d\n",cfm_errors.mep_id);
           printf ("\n mep_mac %x %x %x %x %x %x\n",
                               cfm_errors.mep_mac[5],  cfm_errors.mep_mac[4], cfm_errors.mep_mac[3],
                               cfm_errors.mep_mac[2],  cfm_errors.mep_mac[1],  cfm_errors.mep_mac[0]);  
          printf ("\n Error Reason %d\n",cfm_errors.err_reason);
          printf ("\n ma_name  %s\n",cfm_errors.ma_name);
         }
        else
         {
           printf ("\n Retrieval of First Error Entry Failed \n");  
         } 
         

     } else if (!strcasecmp (action, "clearerror"))
        {
          ret = smi_cfm_get_errors_clear (azg, brname, cfmmdname, cfmlevel);
           if (ret == 0)
            {
              printf ("\n The Error related to MD %s is cleared successfully \n",cfmmdname);
            }
           else
            {
              printf ("\n Failed to clear the Error for the Maintainence domain \n");
            }
        }  else if(!strcasecmp(action, "getclearrmep"))
     {
       ret = smi_cfm_get_rmep_clear(azg, cfmmdname, cfmlevel, brname);
       if(ret == 0)
        {
          printf("\n\t Cleared successful \n");
        } else {
          printf("\n\t errors clearing Failed \n");
        }
     } else if(!strcasecmp(action, "sethwaddr"))
     {
       ret = smi_cfm_set_hwaddr(azg, hwaddr);
       if(ret == 0)
        {
          printf("\n\t Set successfully \n");
        } else {
          printf("\n\t Hardware setting Failed \n");
        }
     } else if(!strcasecmp(action, "gethwaddr"))
     {
       char addr[32];
       ret = smi_cfm_get_hwaddr(azg, addr);
       if(ret >= 0)
        {
          printf("\n\t Returned :Hwaddr %x %x %x %x %x %x \n", 
                                      addr[5],addr[4],addr[3],addr[2],addr[1],addr[0]);
        } else {
          printf("\n\t Hardware getting Failed \n");
        }
     } else if(!strcasecmp(action, "setethtype"))
     {
       ret = smi_cfm_set_ether_type(azg, cfmethtype);
       if(ret == 0)
        {
          printf("\n\t Set successfully \n");
        } else {
          printf("\n\t setting Failed \n");
        }
     } else if(!strcasecmp(action, "getethtype"))
     {
       char eth[32];
       ret = smi_cfm_get_ether_type(azg, eth);
       if(ret >= 0)
        {
          printf("\n\t Returned :ethtype %s \n", eth);
        } else {
          printf("\n\t getting Failed \n");
        }
     } else if(!strcasecmp(action, "getrmep"))
     {
       struct smi_cfm_rmep rmep;
       ret = smi_cfm_rmep_get(azg, brname, cfmrmepid,
                              cfmlevel, cfmvid, &rmep);
       if(ret >= 0)
        {
          printf("\n\t Returned :level %d \n Remote MID %d ",
                 rmep.level, rmep.rmep_id);
        } else {
          printf("\n\t getting Failed \n");
        }
     }
return 0;
}

/* GvRP related APIs */
int
smi_gvrp_action (struct smiclient_globals *azg, char *action)
{
  int ret = 0;

  if(!strcasecmp(action, "settimer"))
   {
      ret = smi_gvrp_set_timer (azg, ifname, gvrptimertype, gvrptimervalue);
      if(ret >= 0)
        {
          printf("\n\t GvRP timer set successfully \n");
        } else {
          printf("\n\tGvRP timer set Failed\n");
        }
   } else if(!strcasecmp(action, "gettimer"))
   {
      smi_time_t timer_value;
      ret = smi_gvrp_get_timer (azg, ifname, gvrptimertype, &timer_value);
      if(ret >= 0)
        {
          printf("\n\t GvRP timer values\n");
          if (gvrptimertype == SMI_GARP_JOIN_TIMER)
            printf("Join                 %u\n", timer_value);
          else if (gvrptimertype == SMI_GARP_LEAVE_TIMER)
            printf ("Leave                %u\n", timer_value);
          else if (gvrptimertype == SMI_GARP_LEAVE_ALL_TIMER)
            printf ("Leave All            %u\n", timer_value);
        } else {
          printf ("\n\tGvRP timer get Failed\n");
        }
   }
  else if(!strcasecmp(action, "enable"))
   {
      ret = smi_gvrp_enable (azg, gvrpregtype, brname);
      if(ret >= 0)
        {
          printf("\n\t GvRP enable successfully \n");
        } else {
          printf("\n\tGvRP enable Failed\n");
        }
   }
  else if(!strcasecmp(action, "disable"))
   {
      ret = smi_gvrp_disable (azg, brname);
      if(ret >= 0)
        {
          printf("\n\t GvRP disable successfully \n");
        } else {
          printf("\n\tGvRP disable Failed\n");
        }
   }
  else if(!strcasecmp(action, "enableport"))
   {
      ret = smi_gvrp_enable_port (azg, ifname);
      if(ret >= 0)
        {
          printf("\n\t GvRP enabled on port %s successfully \n", ifname);
        } else {
          printf("\n\tGvRP enable on port %s Failed\n", ifname);
        }
   }
  else if(!strcasecmp(action, "disableport"))
   {
      ret = smi_gvrp_disable_port (azg, ifname);
      if(ret >= 0)
        {
          printf("\n\t GvRP disable on port %s successfully \n", ifname);
        } else {
          printf("\n\tGvRP disable on port %s Failed\n", ifname);

        }
   }
  else if(!strcasecmp(action, "setregmode"))
   {
      ret = smi_gvrp_set_registration (azg, ifname, gvrpregmode);
      if(ret >= 0)
        {
          printf("\n\t GvRP registration mode set successfully \n");
        } else {
          printf("\n\tGvRP registration mode setting Failed\n");
        }
   } else if(!strcasecmp(action, "getregmode"))
   {
      u_int32_t reg_mode;
      ret = smi_gvrp_get_registration (azg, ifname, &reg_mode);
      if(ret >= 0)
        {
          printf("\n\t GvRP registration mode: %d\n", reg_mode);
        } else {
          printf("\n\tGvRP registration mode get Failed\n");
        }
   } else if(!strcasecmp(action, "getvlanstats"))
   {
      u_int32_t recv_counters = 0, xmit_counters = 0;
      ret = smi_gvrp_get_per_vlan_statistics_details (azg, brname, vlanid, 
                                                      &recv_counters, 
                                                      &xmit_counters);
      if(ret >= 0)
        {
          printf("\n\t GvRP vlan %d stats:\n", vlanid);
          printf("\n\t\t Receive counters: %d\n", recv_counters);
          printf("\n\t\t Transmit counters: %d\n", xmit_counters);
        } else {
          printf("\n\tGvRP per vlan stats get Failed\n");
        }
   } else if(!strcasecmp(action, "clrallstats"))
   {
      ret = smi_gvrp_clear_all_statistics (azg, brname);
      if(ret >= 0)
        {
          printf("\n\t GvRP all stats cleared successfully\n");
        } else {
          printf("\n\tGvRP clear all stats Failed\n");
        }
   }
  else if(!strcasecmp(action, "dynvlanlrn"))
   {
      ret = smi_gvrp_dynamic_vlan_learning_set (azg, brname, gvrpdynvlanlrn);
      if(ret >= 0)
        {
          printf("\n\t GvRP dynamic vlan learning set successfully\n");
        } else {
          printf("\n\tGvRP dynamic vlan learning set Failed\n");
        }
   } 
   else if(!strcasecmp(action, "getbrconf"))
   {
      struct gvrp_bridge_configuration brconfig;
      ret = smi_gvrp_get_configuration_bridge (azg, gvrpregtype, brname, &brconfig);
      if(ret >= 0)
        {
          printf("\n\t GvRP bridge %s config:\n", brname);
          printf("\n\t\t Dynamic VLAN learning: %s\n", 
                (brconfig.dynamic_vlan_enabled == 0 ? "Disabled" : "Enabled"));
        } else {
          printf("\n\tGvRP bridge config get Failed\n");
        }
   }
   else if(!strcasecmp(action, "getviddetail"))
   {
      struct smi_gvrp_vid_detail vid_detail;
      ret = smi_gvrp_get_vid_details  (azg, ifname, gvrpfirstcall, 
                                               gvrpgididx, gvrpnoentries, 
                                               &vid_detail);
      if(ret >= 0)
        {
          printf("\n\t GvRP Vid config get successful\n");
        } else {
          printf("\n\tGvRP Vid config get Failed\n");
        }
   }
   else if(!strcasecmp(action, "getportstats"))
   {
      struct smi_gvrp_statistics gvrp_stats;
      ret = smi_gvrp_get_port_statistics (azg, ifname, &gvrp_stats);
      if(ret >= 0)
        {
          printf("\n\t GvRP port stats get successful\n");
        } else {
          printf("\n\tGvRP  port stats get Failed\n");
        }
   }
   else if(!strcasecmp(action, "gettimerdetails"))
   {
     smi_time_t timer_details[SMI_GARP_MAX_TIMERS];

     ret = smi_gvrp_get_timer_details (azg, ifname, timer_details);  
     if(ret >= 0)
        {
          printf("\n\t Timer Details\n");
          printf("\n\t -------------\n");
          printf("\n\t Join Timer:      %u", timer_details[SMI_GARP_JOIN_TIMER]);
          printf("\n\t Leave Timer:     %u", timer_details[SMI_GARP_JOIN_TIMER]);
          printf("\n\t Leave all Timer: %u", timer_details[SMI_GARP_JOIN_TIMER]);
        } else {
          printf("\n\tGvRP get timer details Failed\n");
        }

   }
  return 0;
}


/* EFM related APIs */
int
smi_efm_action (struct smiclient_globals *azg, char *action)
{
  int ret=0;
  int val = 0;

  if(!strcasecmp(action, "disableproto"))
  {
    ret = smi_efm_oam_protocol_disable(azg, ifname);
    if(ret == 0)
    {
      printf("\n\t Disabled Ethernet OAM Successfully\n");
    } else {
      printf("\n\t Failed to disable Ethernet OAM\n");
    }
  }
  else if(!strcasecmp(action, "enableproto"))
  {
    ret = smi_efm_oam_protocol_enable(azg, ifname);
    if(ret == 0)
    {
      printf("\n\t Enabled Ethernet OAM Successfully\n");
    } else {
      printf("\n\t Failed to enable Ethernet OAM\n");
    }
  }
  if(!strcasecmp(action, "setlinktimer"))
  {
    ret = smi_efm_oam_set_link_timer (azg, ifname, efmlinktimer);
    if(ret == 0)
    {
      printf("\n\t Set Link timer Successfully\n");
    } else {
      printf("\n\t Failed to Set Link timer\n");
    }
  }
 else if(!strcasecmp(action, "getlinktimer"))
  {
    ret = smi_efm_oam_get_link_timer (azg, ifname, &val);
    if(ret == 0)
    {
      printf("\n\t Got Link timer Successfully : %d\n", val);
    } else {
      printf("\n\t Failed to enable Ethernet OAM\n");
    }
  }
  if(!strcasecmp(action, "startremotelb"))
  {
    ret = smi_efm_oam_remoteloopback_start (azg, ifname);
    if(ret == 0)
    {
      printf("\n\t Started Remote loopback Successfully\n");
    } else {
      printf("\n\t Failed to Start Remote loopback\n");
    }
  }
  else if(!strcasecmp(action, "stopremotelb"))
  {
    ret = smi_efm_oam_remoteloopback_stop (azg, ifname);
    if(ret == 0)
    {
      printf("\n\t Stopped Remote loopback Successfully \n");
    } else {
      printf("\n\t Failed to Stop Remote loopback\n");
    }
  }
  if(!strcasecmp(action, "setmodeactive"))
  {
    ret = smi_efm_oam_set_mode_active (azg, ifname);
    if(ret == 0)
    {
      printf("\n\t Set OAM Mode to Active state\n");
    } else {
      printf("\n\t Failed to Set OAM mode to Active state\n");
    }
  }
else if(!strcasecmp(action, "setmodepassive"))
  {
    ret = smi_efm_oam_set_mode_passive (azg, ifname);
    if(ret == 0)
    {
      printf("\n\t Set OAM Mode to passive state \n");
    } else {
      printf("\n\t Failed to Set OAM Mode to passive state\n");
    }
  }
  if(!strcasecmp(action, "getmode"))
  {
    ret = smi_efm_oam_get_mode (azg, ifname, &val);
    if(ret == 0)
    {
      printf("\n\t Got OAM Mode Successfully :%d\n", val);
    } else {
      printf("\n\t Failed to get OAM mode \n");
    }
  }
  else if(!strcasecmp(action, "setpdutimer"))
  {
    ret = smi_efm_oam_set_pdu_timer (azg, ifname, efmpdutimer);
    if(ret == 0)
    {
      printf("\n\t Set OAM PDU Timer\n");
    } else {
      printf("\n\t Failed to Set OAM PDU Timer\n");
    }
  }
  else if(!strcasecmp(action, "getpdutimer"))
  {
    ret = smi_efm_oam_get_pdu_timer (azg, ifname, &val);
    if(val > 0)
    {
      printf("\n\t Got OAM PDU Timer Successfully :%c\n", val);
    } else {
      printf("\n\t Failed to get OAM PDU Timer\n");
    }
  }
else if(!strcasecmp(action, "setmaxrate"))
  {
    ret = smi_efm_oam_set_max_rate (azg, ifname, efmmaxpdu);
    if(ret == 0)
    {
      printf("\n\t Set Maximum PDU Transmission rate Successfully\n");
    } else {
      printf("\n\t Failed to Set Maximum PDU Transmission rate\n");
    }
  }
  else if(!strcasecmp(action, "getmaxrate"))
  {
    u_int8_t max_pdu = 0;
    ret = smi_efm_oam_get_max_rate (azg, ifname, &max_pdu);
    if(ret == 0)
    {
      printf("\n\t Got Maximum PDU Transmission rate : %d\n", max_pdu);
    } else {
      printf("\n\t Failed to get OAM PDU MAX Rate\n");
    }
  }
  else if(!strcasecmp(action, "setlinkmonitor"))
  {
    ret = smi_efm_oam_set_link_monitor (azg, ifname, efmlinkmonen);
    if(ret == 0)
    {
      printf("\n\t Enable ethernet oam link-monitor support\n");
    } else {
      printf("\n\t Failed to Set Link Monitor\n");
    }
  }
  else if(!strcasecmp(action, "getlinkmonitor"))
  {
    u_int8_t link_val = 0;
    ret = smi_efm_oam_get_link_monitor (azg, ifname, &link_val);
    if(ret == 0)
    {
      printf("\n\t Got Link Monitor Support state: %d\n", link_val);
    } else {
      printf("\n\t Failed to get Link Monitor state\n");
    }
  }
  else if(!strcasecmp(action, "setrmloopback"))
  {
    ret = smi_efm_oam_set_remote_loopback (azg, ifname, efmlinkmonen);
    if(ret == 0)
    {
      printf("\n\t Enabled OAM Remote Loopback Successfully\n");
    } else {
      printf("\n\t Failed to enable OAM Remote Loopback\n");
    }
  }
  else if(!strcasecmp(action, "getrmloopback"))
  {
    u_int8_t remotelb = 0;
    ret = smi_efm_oam_get_remote_loopback (azg, ifname, &remotelb);
    if(ret == 0)
    {
      printf("\n\t Got Remote Loopback Support state: %d\n", remotelb);
    } else {
      printf("\n\t Failed to get Loopback Support state\n");
    }
  }
  else if(!strcasecmp(action, "setrmlbtimeout"))
  {
    ret = smi_efm_oam_set_remote_loopback_timeout (azg, ifname, efmlbtimeout);
    if(ret == 0)
    {
      printf("\n\t Set OAM Remote Loopback timeout Period Successfully\n");
    } else {
      printf("\n\t Failed to set OAM Remote Loopback Timeout\n");
    }
  }
  else if(!strcasecmp(action, "getrmlbtimeout"))
  {
    u_int8_t remotelbto = 0;
    ret = smi_efm_oam_get_remote_loopback_timeout (azg, ifname, &remotelbto);
    if(ret == 0)
    {
      printf("\n\t Got Remote Loopback Timeout Period Successfully: %d\n", 
             remotelbto);
    } else {
      printf("\n\t Failed to get Loopback Timeout Period\n");
    }
  }
   else if(!strcasecmp(action, "setlowthres"))
  {
    ret = smi_efm_oam_set_err_frame_low_thres (azg, ifname, efmthreslow);
    if(ret == 0)
    {
      printf("\n\t Set Low threshold val for error frames Successfully\n");
    } else {
      printf("\n\t Failed to Set Low threshold val\n");
    }
  }
  else if(!strcasecmp(action, "getlowthres"))
  {
    ret = smi_efm_oam_get_err_frame_low_thres (azg, ifname, &val);
    if(ret == 0)
    {
      printf("\n\t Got Low threshold val Successfully: %d\n", val);
    } else {
      printf("\n\t Failed to get Low threshold val\n");
    }
  }
  else if(!strcasecmp(action, "sethighthres"))
  {
    ret = smi_efm_oam_set_err_frame_high_thres (azg, ifname, efmthreshigh);
    if(ret == 0)
    {
      printf("\n\t Set high threshold val for error frames Successfully\n");
    } else {
      printf("\n\t Failed to Set high threshold val\n");
    }
  }
  else if(!strcasecmp(action, "gethighthres"))
  {
    ret = smi_efm_oam_get_err_frame_high_thres (azg, ifname, &val);
    if(ret == 0)
    {
      printf("\n\t Got high threshold val Successfully: %d\n", val);
    } else {
      printf("\n\t Failed to get high threshold val\n");
    }
  }
  else if(!strcasecmp(action, "setwinthres"))
  {
    ret = smi_efm_oam_set_err_frame_period_window_thres (azg, ifname, 
                                                         efmwindowth);
    if(ret == 0)
    {
      printf("\n\t Set Winow threshold val for error frames Successfully\n");
    } else {
      printf("\n\t Failed to Set Window threshold val\n");
    }
  }
  else if(!strcasecmp(action, "getwinthres"))
  {
    ret = smi_efm_oam_get_err_frame_period_window_thres (azg, ifname, &val);
    if(ret == 0)
    {
      printf("\n\t Got Window threshold val Successfully: %d\n", val);
    } else {
      printf("\n\t Failed to get Window threshold val\n");
    }
  }

  else if(!strcasecmp(action, "setseclowthres"))
  {
    ret = smi_efm_oam_set_err_frame_second_low_thres(azg, ifname, 
                                                     efmsecthreslow);
    if(ret == 0)
    {
      printf("\n\t Set Second Low threshold val Successfully\n");
    } else {
      printf("\n\t Failed to Set Second Low threshold val\n");
    }
  }
  else if(!strcasecmp(action, "getseclowthres"))
  {
   u_int16_t low_thres;
    ret = smi_efm_oam_get_err_frame_second_low_thres (azg, ifname, &low_thres);
    if(ret == 0)
    {
      printf("\n\t Got Low second threshold val Successfully: %d\n", low_thres);
    } else {
      printf("\n\t Failed to get second low threshold val\n");
    }
  }
   else if(!strcasecmp(action, "setsechighthres"))
  {
    ret = smi_efm_oam_set_err_frame_second_high_thres (azg, ifname, 
                                                       efmsecthreslow);
    if(ret == 0)
    {
      printf("\n\t Set Second High threshold value Successfully\n");
    } else {
      printf("\n\t Failed to Set Second High threshold val\n");
    }
  }
  else if(!strcasecmp(action, "getsechighthres"))
  {
    u_int16_t high_thres;
    ret = smi_efm_oam_get_err_frame_second_high_thres (azg, ifname, 
                                                       &high_thres);
    if(ret == 0)
    {
      printf("\n\t Got High second threshold val Successfully: %d\n", 
             high_thres);
    } else {
      printf("\n\t Failed to get High second threshold val\n");
    }
  }
  else if(!strcasecmp(action, "setdisableifevt"))
  {
//    ret = smi_efm_oam_disable_if_event_set (azg, ifname, efmevent, 
  //                                          efmeventenable);
    ret = smi_efm_oam_disable_if_event_set (azg, ifname, 2, 
                                            1);
    if(ret == 0)
    {
      printf("\n\t Set disable if event Successfully\n");
    } else {
      printf("\n\t Failed to Set disbale if event\n");
    }
  }
  else if(!strcasecmp(action, "getifeventst"))
  {
    ret = smi_efm_oam_disable_if_event_get (azg, ifname, efmevent, &val);
    if(ret == 0)
    {
      printf("\n\t Got disable if event Successfully: %d\n", val);
    } else {
      printf("\n\t Failed to get disable if event st\n");
    }
  }
   else if(!strcasecmp(action, "getoamstats"))
  {
    struct smi_efm_oam_stats oam_stats;
    ret = smi_efm_oam_show_statistics (azg, ifname, &oam_stats);
    if(ret == 0)
    {
      printf("\n\t Got OAM Status Successfully\n");
    } else {
      printf("\n\t Failed to get OAM Status Successfully\n");
    }
  }
  else if(!strcasecmp(action, "getifstatus"))
  {
   struct smi_efm_oam_if_status if_info;
    ret = smi_efm_oam_show_interface_status (azg, ifname, &if_info);
    if(ret == 0)
    {
      printf("\n\t Loop bak St= %d",if_info.local_info.loopback);
      printf("\n\t oAM Version = %d",if_info.local_info.oam_version);
      printf("\n\t Info State  = %d",if_info.local_info.state );
      printf("\n\t Got Interface Status Successfully\n");
    } else {
      printf("\n\t Failed to get Interface Status\n");
    }
  }
  else if(!strcasecmp(action, "getdiscoverst"))
  {
    struct smi_efm_oam_discovery_status discover_st;
    ret = smi_efm_oam_get_discovery (azg, ifname, &discover_st);
    if(ret == 0)
    {
      printf("\n\t Got Discovery State Machine St Successfully\n");
    } else {
      printf("\n\t Failed to get Discovery State Machine St\n");
    }
  }
  else if(!strcasecmp(action, "getethernet"))
  {
   struct smi_efm_oam_details_info ether_detail;
    ret = smi_efm_oam_get_ethernet (azg, ifname, &ether_detail);
    if(ret == 0)
    {
      printf("\n\t Got Ethernet Details Successfully \n");
    } else {
      printf("\n\t Failed to get Ethernet details\n");
    }
  }
  else if(!strcasecmp(action, "senddataframe"))
  {
    char mac[6];
    hwaddr_aton(hwaddr, mac);
    int len = strlen(efmtestbuf);
    ret = smi_efm_send_data_frame (azg, ifname, mac, NULL, len, efmnoframes);
    if(ret ==  0)
    {
      printf("\n\t Sent data frames Successfully\n");
    } else {
      printf("\n\t Failed to send data frames\n");
    }
  }
  else if (!strcasecmp(action, "getlpbkstatus"))
  {
    enum smi_efm_loopback_status status = 0;
    ret = smi_efm_get_local_loopback_status (azg, ifname, &status);
    if(ret == 0)
      {
        printf("\n\t EFM loopback status %d\n", status);
      } else {
          printf("\n\t Failed to get loopback status \n");
      }
  }
  return 0; 
}

/* QOS related APIs */
int
smi_qos_action (struct smiclient_globals *azg, char *action)
{
  int ret = 0;
  int val = 0;

  if(!strcasecmp(action, "englobal"))
  {
    ret = smi_qos_global_enable(azg);
    if(ret == 0)
    {
      printf("\n\t global enable successful \n");
    } else {
      printf("\n\t global enabling failed \n");
    }
  } else if(!strcasecmp(action, "disglobal"))
  {
    ret = smi_qos_global_disabled(azg);
    if(ret == 0)
    {
      printf("\n\t global disable successful \n");
    } else {
      printf("\n\t global disabling failed \n");
    } 
  } else if(!strcasecmp(action, "getglobal"))
  {
    unsigned char status;
    ret = smi_qos_get_global_status(azg, &status);
    if(ret >= 0)
    {
      printf("\n\t global get successful: %u ", status);
    } else {
      printf("\n\t global get failed \n");
    }
  } else if(!strcasecmp(action, "setpmap"))
  {
    ret = smi_qos_set_pmap_name(azg, qospmapname);
    if(ret == 0)
    {
      printf("\n\t Pmap name set successfully ");
    } else {
      printf("\n\t pmap name setting failed \n");
    }
  } else if(!strcasecmp(action, "getpmap"))
  {
    char policy_name[SMI_NUM_REC][SMI_QOS_POLICY_LEN];
    char pmap_name[SMI_QOS_POLICY_LEN];
    memset(pmap_name, '\0', SMI_QOS_POLICY_LEN);
    ret = smi_qos_get_policy_map_names(azg, policy_name, 1, pmap_name);
    if(ret >= 0)
    {
      int i = 0;
      printf("\n\t Pmap name get successfully ");
      for ( i = 0; i < SMI_NUM_REC ; i++)
        printf("\t  %s\n", policy_name[i]);
    } else {
      printf("\n\t pmap name getting failed \n");
    }
  } else if(!strcasecmp(action, "getpolicy"))
  {
    struct smi_qos_pmap qos_pmap;
    ret = smi_qos_get_policy_map(azg, qospolicyname, &qos_pmap);
    if(ret == 0)
    {
      int i = 0;
      printf("\n\t Policy name get successfully ");
      printf("\n\t  Name: %s", qos_pmap.name);
      printf("\n\t  ACL Name: %s", qos_pmap.acl_name);
      printf("\n\t  Attached : %d", qos_pmap.attached);
      for (i = 0; i < SMI_MAX_NUM_OF_CLASS_IN_PMAPL; i++)
        printf("\n\t  Cl_name: %s", qos_pmap.cl_name[i]);
    } else {
      printf("\n\t polivy name getting failed \n");
    }
  } else if(!strcasecmp(action, "delpmap"))
  {
    ret = smi_qos_pmap_delete(azg, qospmapname);
    if(ret == 0)
    {
      printf("\n\t Policy name deleted successfully \n");
    } else {
      printf("\n\t polivy name deleting failed \n");
    }
  } else if(!strcasecmp(action, "setcmap"))
  {
    ret = smi_qos_set_cmap_name(azg, qoscmapname);
    if(ret == 0)
    {
      printf("\n\t class name set successfully \n");
    } else {
      printf("\n\t class name setting failed \n");
    }
  } else if(!strcasecmp(action, "getcmap"))
  {
    struct smi_qos_cmap qos_cmap;
    ret = smi_qos_get_cmap_name(azg, qoscmapname, &qos_cmap);
    if(ret >= 0)
    {
      printf("\n\t class name get successful \n");
    } else {
      printf("\n\t class name getting failed \n");
    }
  } else if(!strcasecmp(action, "delcmap"))
  {
    ret = smi_qos_delete_cmap_name(azg, qoscmapname);
    if(ret == 0)
    {
      printf("\n\t class name deletion successful \n");
    } else {
      printf("\n\t class name deletion failed \n");
    }
  } else if(!strcasecmp(action, "settraffic"))
  {
    ret = smi_qos_cmap_match_traffic_set(azg, qoscmapname, qostraffictype ,
                                          qosmatchmode);
    if(ret == 0)
    {
      printf("\n\t cmap match traffic set successfully \n");
    } else {
      printf("\n\t cmap match traffic setting failed \n");
    }
  } else if(!strcasecmp(action, "gettraffic"))
  {
    enum smi_qos_traffic_match_mode match_mode = qosmatchmode;
    u_int32_t traffic_type;

    ret = smi_qos_cmap_match_traffic_get(azg, qoscmapname, &match_mode, &traffic_type);
    if(ret == 0)
    {
      printf("\n\t cmap match traffic get successfully \n");
      printf ("\n The values are Match Mode %d , Traffic Type %d\n",match_mode,traffic_type);
    } else {
      printf("\n\t cmap match traffic setting failed \n");
    }
  } else if(!strcasecmp(action, "unsettraffic"))
  {
    ret = smi_qos_cmap_match_traffic_unset(azg, qoscmapname, qostraffictype);
    if(ret == 0)
    {
      printf("\n\t cmap match traffic unset successfully \n");
    } else {
      printf("\n\t cmap match traffic unsetting failed \n");
    }
  } else if(!strcasecmp(action, "setpmapc"))
  {
    ret = smi_mls_qos_pmapc_police(azg, qoscmapname, qospmapname, qosratevalue,
                                qosrateunit, qoscommit, qosexcess, qosaction,
                                qosfcmode);
    if(ret == 0)
    {
      printf("\n\t Pmapc set successfully \n");
    } else {
      printf("\n\t pmap setting failed \n");
    }
  } else if(!strcasecmp(action, "getpmapc"))
    {
      u_int32_t rate_value, commit_burst_size, excess_burst_size;
      enum smi_qos_rate_unit rate_unit=0;
      enum smi_exceed_action exceed_action;
      enum smi_qos_flow_control_mode fc_mode;
       
      ret = smi_mls_qos_pmapc_police_get (azg, qoscmapname, qospmapname,
                                          &rate_value, &rate_unit,
                                          &commit_burst_size, &excess_burst_size,
                                          &exceed_action, &fc_mode);
      if (ret >= 0)
        {
          printf ("\n qoscmapname %s inside qospmapname %s \n",qoscmapname, qospmapname);
          printf ("\n rate_value %d\n",rate_value);
          printf ("\n rate_unit %d\n",rate_unit);
          printf ("\n commit_burst_size %d\n",commit_burst_size);
          printf ("\n excess_burst_size %d\n",excess_burst_size);
          printf ("\n exceed_action %d\n",exceed_action);
          printf ("\n fc_mode %d\n",fc_mode);
        }
      else
          printf ("\n Failed to execute the smi_mls_qos_pmapc_police_get API \n"); 
    } else if(!strcasecmp(action, "delpmapc"))
   {
    ret = smi_mls_qos_pmapc_police_delete(azg, qoscmapname, qospmapname);
    if(ret == 0)
    {
      printf("\n\t pmapc deletion successful \n");
    } else {
      printf("\n\t pmapc deletion failed \n");
    }
  } else if (!strcasecmp(action, "delcmapfrompmap"))
  {
    ret = smi_mls_qos_pmapc_del_cmap (azg, qoscmapname, qospmapname);
       
    if (ret == 0)
      printf ("\n Class-Map %s successfully deleted from Policy-Map %s\n",
               qoscmapname, qospmapname);
    else
      printf ("\n Deletion of Class-Map failed from Policy-Map\n");
  
  } else if(!strcasecmp(action, "setportsched"))
  {
    ret = smi_qos_set_port_scheduling(azg, ifname, qosportmode);
    if(ret == 0)
    {
      printf("\n\t port scheduling set successfully \n");
    } else {
      printf("\n\t port scheduling setting failed \n");
    }
  } else if(!strcasecmp(action, "getportsched"))
  {
    u_int8_t mode;
    ret = smi_qos_get_port_scheduling(azg, ifname, &mode);
    if(ret >= 0)
    {
      printf("\n\t port scheduling get successful\n");
      if (mode == SMI_QOS_STRICT_QUEUE_ALL)
        printf("\n\tmls qos strict queue: all");
      else if (mode == SMI_QOS_STRICT_QUEUE0)
        printf("\n\tmls qos strict queue: 0");
      else if (mode == SMI_QOS_STRICT_QUEUE1)
        printf("\n\tmls qos strict queue: 1");
      else if (mode == SMI_QOS_STRICT_QUEUE2)
        printf("\n\tmls qos strict queue: 2");
      else if (mode == SMI_QOS_STRICT_QUEUE3)
        printf("\n\tmls qos strict queue: 3");
    } else {
      printf("\n\t port scheduling get failed \n");
    }
  } else if(!strcasecmp(action, "setuserp"))
  {
    //ret = smi_qos_set_default_user_priority(azg, ifname, qospriority);
    ret = smi_qos_set_default_user_priority(azg, ifname, 3);
    if(ret == 0)
    {
      printf("\n\t priority set successfully \n");
    } else {
      printf("\n\t priority setting failed \n");
    }
  } else if(!strcasecmp(action, "getuserp"))
  {
    int prio;
    ret = smi_qos_get_default_user_priority(azg, ifname, &prio);
    if(ret >= 0)
    {
      printf("\n\t priority get successful %d\n", prio);
    } else {
      printf("\n\t priority get failed \n");
    }
  } else if(!strcasecmp(action, "setregenp"))
  {
    ret = smi_vlan_port_set_regen_user_priority(azg, ifname, qosuserpr, 
                                                qosregenpri);
    if(ret == 0)
    {
      printf("\n\t priority set successfully \n");
    } else {
      printf("\n\t priority setting failed \n");
    }
  } else if(!strcasecmp(action, "getregenp"))
  {
    ret = smi_vlan_port_get_regen_user_priority(azg, ifname, qosuserpr,
                                                 &val);
    if(ret >= 0)
    {
      printf("\n\t priority set successfully %d\n", val);
    } else {
      printf("\n\t priority setting failed \n");
    }
  } else if(!strcasecmp(action, "costoq"))
  {
    ret = smi_qos_global_cos_to_queue(azg, qoscos, qosqid);
    if(ret == 0)
    {
      printf("\n\t configuration successful \n");
    } else {
      printf("\n\t configuration failed \n");
    }
  } else if(!strcasecmp(action, "dscptoq"))
  {
    ret = smi_qos_global_dscp_to_queue(azg, qosdscp, qosqid);
    if(ret == 0)
    {
      printf("\n\t configuration successful \n");
    } else {
      printf("\n\t configuration failed \n");
    }
  } else if(!strcasecmp(action, "settrust"))
  {
    ret = smi_qos_set_trust_state(azg, ifname, qostrust);
    if(ret == 0)
    {
      printf("\n\t trust state set successful \n");
    } else {
      printf("\n\t trust state set failed \n");
    }
  } else if(!strcasecmp(action, "setforcetrust"))
  {
    ret = smi_qos_set_force_trust_cos(azg, ifname, qosforcetrust);
    if(ret == 0)
    {
      printf("\n\t force trust cos set successful \n");
    } else {
      printf("\n\t force trust cos set failed \n");
    }
  } else if(!strcasecmp(action, "setframep"))
  {
    ret = smi_qos_set_frame_type_priority_override(azg, qosframe, qosqid);
    if(ret == 0)
    {
      printf("\n\t frame type priority set successful \n");
    } else {
      printf("\n\t frame type priority set failed \n");
    }
  } else if(!strcasecmp(action, "setvlanp"))
  {
    ret = smi_qos_set_vlan_priority(azg, brname, qosvid, 2);
    if(ret == 0)
    {
      printf("\n\t vlan priority set successful \n");
    } else {
      printf("\n\t vlan priority set failed \n");
    }
   } else if(!strcasecmp(action, "getvlanp"))
  {
    u_int8_t prio = 0;
    ret = smi_qos_get_vlan_priority(azg, brname, qosvid, &prio);
    if(ret == 0)
    {
      printf("\n\t vlan priority get successful %d\n", prio);
    } else {
      printf("\n\t vlan priority get failed \n");
    }
  } else if(!strcasecmp(action, "unsetvlanp"))
  {
    ret = smi_qos_unset_vlan_priority(azg, brname, qosvid);
    if(ret == 0)
    {
      printf("\n\t vlan priority unset successful \n");
    } else {
      printf("\n\t vlan priority unset failed \n");
    }
  } else if(!strcasecmp(action, "setpvlanp"))
  {
    ret = smi_qos_set_port_vlan_priority_override(azg, ifname, qosvlanpmode);
    if(ret == 0)
    {
      printf("\n\t vlan port priority set successful \n");
    } else {
      printf("\n\t vlan port priority set failed \n");
    }
  } else if(!strcasecmp(action, "setqweight"))
  {
    ret = smi_mls_qos_set_queue_weight(azg, qosqid, qosqweight);
    if(ret == 0)
    {
      printf("\n\t queue weight set successful \n");
    } else {
      printf("\n\t queue weight set failed \n");
    }
  } else if(!strcasecmp(action, "getqweight"))
  {
    int i = 0;
    int qweights[SMI_QOS_MAX_QUEUE_SIZE];
    ret = smi_mls_qos_get_queue_weight(azg, qweights);
    if(ret >= 0)
    {
      printf("\n\t queue weight get successful \n");
      for ( i = 0; i < 8; i++)
        printf("\n\t  %d", qweights[i]);
    } else {
      printf("\n\t queue weight get failed \n");
    }
  } else if(!strcasecmp(action, "setpservice"))
  {
    ret = smi_qos_set_port_service_policy (azg, ifname, qospmapname);
    if(ret == 0)
    {
      printf("\n\t port service set successful \n");
    } else {
      printf("\n\t port service set failed \n");
    }
  } else if(!strcasecmp(action, "unsetpservice"))
  {
    ret = smi_qos_unset_port_service_policy(azg, ifname, qospmapname);
    if(ret == 0)
    {
      printf("\n\t port service unset successful \n");
    } else {
      printf("\n\t port service unset failed \n");
    }
  } else if(!strcasecmp(action, "getpservice"))
  {
    char pmap_name[SMI_QOS_POLICY_LEN];
    ret = smi_qos_get_port_service_policy (azg, ifname, pmap_name);
    if(ret == 0)
    {
      printf("\n\t port service get successful: %s<->%s \n", ifname, pmap_name);
    } else {
      printf("\n\t port service get failed \n");
    }
  } else if(!strcasecmp(action, "settrafficshape"))
  {
    ret = smi_qos_set_traffic_shape (azg, ifname, qosratevalue, qosrateunit);
    if(ret == 0)
    {
      printf("\n\t traffic shape set successful \n");
    } else {
      printf("\n\t traffic shape set failed \n");
    }
  } else if(!strcasecmp(action, "unsettrafficshape"))
  {
    ret = smi_qos_unset_traffic_shape (azg, ifname);
    if(ret == 0)
    {
      printf("\n\t traffic shape unset successful \n");
    } else {
      printf("\n\t traffic shape unset failed \n");
    }
  } else if(!strcasecmp(action, "gettrafficshape"))
  {
    int ratevalue = 0;
    enum smi_qos_rate_unit rateunit = 0;
    ret = smi_qos_get_traffic_shape (azg, ifname, &ratevalue, &rateunit);
    if(ret == 0)
    {
      printf("\n\t traffic shape get successful: %d %d \n", 
             ratevalue, rateunit);
    } else {
      printf("\n\t traffic shape get failed \n");
    }
  } else if(!strcasecmp(action, "getcostoq"))
  {
    u_int8_t cos_to_queue [SMI_COS_TBL_SIZE];
    int i;
    ret = smi_qos_get_cos_to_queue(azg, cos_to_queue);
    if(ret == 0)
    {
      printf("\n\tCos to Queue get success\n");
      for (i = 0; i < SMI_COS_TBL_SIZE; i++)
      {
        if (SMI_CHECK_FLAG (cos_to_queue[i], SMI_QOS_COS_QUEUE_CONF))
          printf ("\n\tqos cos-queue %d %d\n", i,
                  cos_to_queue [i] & SMI_QOS_QUEUE_ID_MASK);
      }
    } else {
      printf("\n\tCos to Queue get failed \n");
    }
  } else if(!strcasecmp(action, "getdscptoq"))
  {
    u_int8_t dscp_to_queue [SMI_DSCP_TBL_SIZE];
    int i;
    ret = smi_qos_get_dscp_to_queue(azg, dscp_to_queue);
    if(ret == 0)
    {
      printf("\n\tDscp to Queue get success\n");
      for (i = 0; i < SMI_DSCP_TBL_SIZE; i++)
      {
        if (SMI_CHECK_FLAG (dscp_to_queue[i], SMI_QOS_COS_QUEUE_CONF))
          printf ("\n\tqos dscp-queue %d %d\n", i,
                  dscp_to_queue [i] & SMI_QOS_QUEUE_ID_MASK);
      }
    } else {
      printf("\n\t DSCP to Queue get failed \n");
    }
  } else if(!strcasecmp(action, "gettrust"))
  {
    enum smi_qos_trust_state qtrust = 0;
    ret = smi_qos_get_trust_state (azg, ifname, &qtrust);
    if(ret == 0)
    {
      printf("\n\t trust state get successful: %d \n", qtrust);
    } else {
      printf("\n\t trust state get failed \n");
    }
  } else if(!strcasecmp(action, "getpvlanp"))
  {
    enum smi_qos_vlan_pri_override prio = 0;
    ret = smi_qos_get_port_vlan_priority_override(azg, ifname, &prio);
    if(ret == 0)
    {
      printf("\n\t vlan port priority override get successful:%d \n", prio);
    } else {
      printf("\n\t vlan port priority override get failed \n");
    }
  } else if(!strcasecmp(action, "getforcetrust"))
  {
    u_int8_t force_trust;
    ret = smi_qos_get_force_trust_cos(azg, ifname, &force_trust);
    if(ret == 0)
    {
      printf("\n\t force trust cos get successful: %d \n", force_trust);
    } else {
      printf("\n\t force trust cos get failed \n");
    }
  }
  return 0;
}

int
smi_fc_action (struct smiclient_globals *azg, char *action)
{
  int ret=0;
                                                                                
  if(!strcasecmp(action, "addfc"))
    {
      ret = smi_port_add_flow_control(azg, ifname, fcdir);
      if(ret == 0)
        {
          printf("\n\tFc added Successfully\n");
        } else {
          printf("\n\tFC adding Failed\n");
        }
     } else if(!strcasecmp(action, "delfc"))
     {
       ret = smi_port_delete_flow_control (azg, ifname, fcdir);
       if(ret == 0)
        {
          printf("\n\tFc deleted Successfully\n");
        } else {
          printf("\n\tFC deleting Failed\n");
        }
     } else if(!strcasecmp(action, "statfc"))
     {
       int rxpause, txpause;
       enum smi_flow_control_dir dir;
       ret = smi_port_get_flow_control_statistics (azg, ifname, &dir, 
                                                   &rxpause, &txpause);
       if(ret >= 0)
        {
          printf("\n\tSuccessful %d\t%d\t%d  \n", dir, rxpause, txpause);
        } else {
          printf("\n\tFC stat Failed\n");
        }
     } else if(!strcasecmp(action, "getif"))
     {
       enum smi_flow_control_dir flow_ctl;
       ret = smi_port_get_flow_control_status (azg, ifname, &flow_ctl);
       if(ret >= 0)
        {
          printf("\n\tFc get Successful: %d\n", flow_ctl);
        } else {
          printf("\n\tFC get Failed\n");
        }
      }
   return 0;
}

int
smi_general_action (struct smiclient_globals *azg, char *action)
{
  int ret = 0;
  int i=0;
  enum smi_qos_da_pri_override da_port_mode;
  enum smi_if_exist exist = 0;
  unsigned char  hwaddr[6] = {0x0,0x23,0x45,0x67,0x89,0xab};

  if (!strcasecmp (action, "setportconf"))
  {
    ret = smi_set_port_non_configure_state (azg, ifname, 
                                            if_port_conf_state);
    if (ret < 0)
      printf ("\n\t set port non-conf %s failed\n", ifname);
    else
      printf ("\n\t set port non-conf %s success\n", ifname);
  } else if (!strcasecmp (action, "getportconf"))
  {
    enum smi_port_conf_state state = 0;
    ret = smi_get_port_non_configure_status (azg, ifname, 
                                             &state);
    if (ret == 0)
      printf ("\n\t Get port non-conf success: %d\n", state);
    else
      printf ("\n\t Get port non-conf port %s failed\n", ifname);
  } else if (!strcasecmp (action, "setportlrn"))
  {
    ret = smi_set_port_learning_state (azg, ifname, if_port_lrn_state);
    if (ret < 0)
      printf ("\n\t set port learning %s failed\n", ifname);
    else
      printf ("\n\t set port learning %s success\n", ifname);
  } else if (!strcasecmp (action, "getportlrn"))
  {
    enum smi_port_conf_state state;
    ret = smi_get_port_learning_status (azg, ifname, &state);
    if (ret < 0)
      printf ("\n\t get port learning %s failed\n", ifname);
    else
      printf ("\n\t get port learning %s success: %d\n", ifname, state);
  }
  else if (!strcasecmp(action, "setportnonsw"))
  {
    ret = smi_set_port_non_switching_state (azg, ifname, port_nonsw_state);
    if(ret == 0)
      {
        printf("\n\tSet port non-switch success\n");
      } else {
        printf("\n\tSet port non-switch failed\n");
      }
  }
  else if (!strcasecmp(action, "getportnonsw"))
  {
    enum smi_port_conf_state state;
    ret = smi_get_port_non_switching_status (azg, ifname, &state);
    if(ret == 0)
      {
        printf("\n\tGet port non-switch success: %d\n", state);
      } else {
        printf("\n\tGet port non-switch failed\n");
      }
  }
  else if(!strcasecmp(action, "forcedfvlan"))
  {
      ret = smi_port_force_default_vlan (azg, ifname, vlanid);
      if(ret == 0)
      {
        printf("\n\t set port force default vlan success\n");
      } else {
        printf("\n\t set port force default vlan failed\n");
      }
   } else if(!strcasecmp(action, "getversion"))
   {
     char version[PACOS_VERSION_LEN];
     ret = smi_get_pacos_version (azg, version);
     if(ret < 0)
       printf("\n\tError in getting PacOS version\n");
     else
       printf("\n\tPacOS Version: %s\n", version);
   } else if(!strcasecmp(action, "setpreservececos"))
   {
     ret = smi_set_preserve_ce_cos (azg, ifname);
     if(ret < 0)
       printf("\n\tError in setting preserve ce cos\n");
     else
       printf("\n\tSetting preserve ce cos success\n");
   } else if(!strcasecmp(action, "setportbasedvlan"))
   {
    struct smi_port_bmp port_bmp;
    SMI_BMP_INIT (port_bmp);
    SMI_BMP_SET (port_bmp, 1);
    SMI_BMP_SET (port_bmp, 2);
     ret = smi_set_port_based_vlan (azg, ifname, &port_bmp);
     if(ret < 0)
       printf("\n\tError in setting port based vlan\n");
     else
       printf("\n\tSetting port based vlan success\n");
   } else if(!strcasecmp(action, "setegressmode"))
   {
     ret = smi_set_port_egress_mode (azg, ifname, if_egress_mode);
     if(ret < 0)
       printf("\n\tError in setting port egress mode\n");
     else
       printf("\n\tSetting port egress mode success\n");
   } else if(!strcasecmp(action, "setcpudefvid"))
   {
     ret = smi_vlan_set_cpu_port_default_vid (azg, vlanid);
     if(ret < 0)
       printf("\n\tError in setting cpu port default vlan id\n");
     else
       printf("\n\tSetting cpu port default vlan id success\n");
   } else if(!strcasecmp(action, "setwsdefvid"))
   {
     ret = smi_vlan_set_wayside_port_default_vid (azg, vlanid);
     if(ret < 0)
       printf("\n\tError in setting ws port default vlan id\n");
     else
       printf("\n\tSetting ws port default vlan id success\n");
   } else if(!strcasecmp(action, "setcpuportbasedvlan"))
   {
     struct smi_port_bmp port_bmp;
     SMI_BMP_INIT (port_bmp);
     SMI_BMP_SET (port_bmp, 1);
     SMI_BMP_SET (port_bmp, 2);
     ret = smi_vlan_set_cpu_port_based_vlan (azg, &port_bmp);
     if(ret < 0)
       printf("\n\tError in setting cpu port based vlan\n");
     else
       printf("\n\tSetting cpu port based vlan success\n");
   } else if(!strcasecmp(action, "setportethertype"))
   {
     ret = smi_svlan_set_port_ether_type (azg, ifname, if_ether_type);
     if(ret < 0)
       printf("\n\tError in setting port ether type\n");
     else
       printf("\n\tSetting port ether type success\n");
   } else if(!strcasecmp(action, "setswreset")) 
   {
     ret = smi_if_set_sw_reset(azg);     
     if(ret < 0)
       printf("Error in sw resetting \n"); 
     else   
       printf("SW Resetting success \n");
   } else if(!strcasecmp(action, "setwaysideethtype"))
   {
     ret = smi_port_set_wayside_ether_type (azg, if_ether_type);
     if(ret < 0)
       printf("\n\tError in setting wayside port ether type\n");
     else
       printf("\n\tSetting wayside port ether type success\n");
   } else if(!strcasecmp(action, "switchover"))
   {
      int failed;
      ret = smi_ha_switchover (azg, act_stb, &failed);
      if(ret < 0)
        printf("\n\tError in Switchover, Failed module: %d\n", failed);
      else
        printf("\n\tAtleast one Switchover success, Failed: %d\n", failed);
   } else if(!strcasecmp(action, "ifexist"))
   {
      enum smi_if_exist exist = 0;
      ret = smi_if_bridge_port_exist (azg, ifname, &exist);
      if(ret < 0)
        printf("\n\tError in getting ifinfo\n");
      else
        printf("\n\tifinfo get success: %d\n", exist);
   } else if(!strcasecmp(action, "brexist"))
   {
      enum smi_if_exist exist = 0;
      ret = smi_bridge_exist (azg, brname, &exist);
      if(ret < 0)
        printf("\n\tError in getting brinfo\n");
      else
        printf("\n\tbrinfo get success: %d\n", exist);
   }else if(!strcasecmp(action, "priovr1"))
   {

      ret = smi_bridge_add_mac_prio_ovr(azg, brname, ifname,hwaddr,
                 vlanid, ovr_mac_type, prio);

      smi_check_ret (ret,ovr_mac_type, prio);
   }else if(!strcasecmp(action, "priovr2"))
   {

      ret = smi_bridge_add_mac_prio_ovr(azg, brname, ifname,hwaddr,
                  vlanid, SMI_BRIDGE_MAC_PRI_OVR_NONE, 0);

      smi_check_ret (ret, SMI_BRIDGE_MAC_PRI_OVR_NONE, 0);
   }else if(!strcasecmp(action, "priovr3"))
   {

      ret = smi_bridge_add_mac_prio_ovr(azg, brname, ifname,hwaddr,
                  vlanid, SMI_BRIDGE_MAC_STATIC, 0);
 
      smi_check_ret (ret,SMI_BRIDGE_MAC_STATIC, 0);
   }else if(!strcasecmp(action, "priovr4"))
   {
  
      ret = smi_bridge_add_mac_prio_ovr(azg, brname, ifname,hwaddr,
                  vlanid, SMI_BRIDGE_MAC_STATIC_PRI_OVR, 0);

      smi_check_ret (ret,SMI_BRIDGE_MAC_STATIC_PRI_OVR, 0);


   }else if(!strcasecmp(action, "priovr5"))
   {
      ret = smi_bridge_add_mac_prio_ovr(azg, brname, ifname,hwaddr,
                  vlanid, SMI_BRIDGE_MAC_STATIC_MGMT, 0);

      smi_check_ret (ret, SMI_BRIDGE_MAC_STATIC_MGMT, 0);

   }else if(!strcasecmp(action, "priovr6"))
   {
      ret = smi_bridge_add_mac_prio_ovr(azg, brname, ifname,hwaddr,
                  vlanid, SMI_BRIDGE_MAC_STATIC_MGMT_PRI_OVR, 0);

      smi_check_ret (ret, SMI_BRIDGE_MAC_STATIC_MGMT_PRI_OVR, 0);

   }else if(!strcasecmp(action, "priovr7"))
   {
      ret = smi_bridge_add_mac_prio_ovr(azg, brname, ifname,hwaddr,
                  vlanid, SMI_BRIDGE_MAC_PRI_OVR_MAX, 0);

      smi_check_ret (ret, SMI_BRIDGE_MAC_PRI_OVR_MAX, 0);

   }else if(!strcasecmp(action, "priovr8"))
   {

      ret = smi_bridge_add_mac_prio_ovr(azg, brname, ifname,hwaddr,
                vlanid, SMI_BRIDGE_MAC_STATIC_MGMT, 1);
    
      smi_check_ret (ret, SMI_BRIDGE_MAC_STATIC_MGMT, 1);
   }else if(!strcasecmp(action, "priovr9"))
   {

      ret = smi_bridge_add_mac_prio_ovr(azg, brname, ifname,hwaddr,
                vlanid, SMI_BRIDGE_MAC_STATIC_MGMT, 2);
    
      smi_check_ret (ret, SMI_BRIDGE_MAC_STATIC_MGMT, 2);
   }else if(!strcasecmp(action, "priovr10"))
   {

      ret = smi_bridge_add_mac_prio_ovr(azg, brname, ifname,hwaddr,
                vlanid, SMI_BRIDGE_MAC_STATIC_MGMT, 3);
    
      smi_check_ret (ret, SMI_BRIDGE_MAC_STATIC_MGMT, 3);
   }else if(!strcasecmp(action, "priovr11"))
   {

      ret = smi_bridge_add_mac_prio_ovr(azg, brname, ifname,hwaddr,
                vlanid, SMI_BRIDGE_MAC_STATIC_MGMT, 4);
    
      smi_check_ret (ret, SMI_BRIDGE_MAC_STATIC_MGMT, 4);
   }else if(!strcasecmp(action, "priovr12"))
   {

      ret = smi_bridge_add_mac_prio_ovr(azg, brname, ifname,hwaddr,
                vlanid, SMI_BRIDGE_MAC_STATIC_MGMT, 5);
    
      smi_check_ret (ret, SMI_BRIDGE_MAC_STATIC_MGMT, 5);
   }else if(!strcasecmp(action, "priovr13"))
   {

      ret = smi_bridge_add_mac_prio_ovr(azg, brname, ifname,hwaddr,
                vlanid, SMI_BRIDGE_MAC_STATIC_MGMT, 6);
    
      smi_check_ret (ret, SMI_BRIDGE_MAC_STATIC_MGMT, 6);
   }else if(!strcasecmp(action, "priovr14"))
   {

      ret = smi_bridge_add_mac_prio_ovr(azg, brname, ifname,hwaddr,
                vlanid, SMI_BRIDGE_MAC_STATIC_MGMT, 7);
    
      smi_check_ret (ret, SMI_BRIDGE_MAC_STATIC_MGMT, 7);
   }else if(!strcasecmp(action, "priovr15"))
   {

       ret = smi_bridge_add_mac_prio_ovr(azg, brname, ifname,hwaddr,
                 vlanid, SMI_BRIDGE_MAC_STATIC_MGMT, 1);

       smi_check_ret (ret, SMI_BRIDGE_MAC_STATIC_MGMT, 1);
   }else if(!strcasecmp(action, "priovr16"))
   {

       ret = smi_bridge_add_mac_prio_ovr(azg, brname, ifname,hwaddr,
                    vlanid, SMI_BRIDGE_MAC_STATIC_MGMT, 1);

       smi_check_ret (ret, SMI_BRIDGE_MAC_STATIC_MGMT, 1);
   }else if(!strcasecmp(action, "priovr17"))
   {

       ret = smi_bridge_add_mac_prio_ovr(azg, brname, ifname,hwaddr,
                     vlanid, SMI_BRIDGE_MAC_STATIC_MGMT, 1);

       smi_check_ret (ret, SMI_BRIDGE_MAC_STATIC_MGMT, 1);
   }else if(!strcasecmp(action, "priovr18"))
   {
   
       ret = smi_bridge_add_mac_prio_ovr(azg, brname, ifname,hwaddr,
                     vlanid, SMI_BRIDGE_MAC_STATIC_MGMT, 1);

       smi_check_ret (ret, SMI_BRIDGE_MAC_STATIC_MGMT, 1);
   }else if(!strcasecmp(action, "priovr19"))
   {

       ret = smi_bridge_add_mac_prio_ovr(azg, brname, ifname,hwaddr,
                     vlanid, SMI_BRIDGE_MAC_STATIC_MGMT, 1);

       smi_check_ret (ret, SMI_BRIDGE_MAC_STATIC_MGMT, 1);
   }else if(!strcasecmp(action, "dapri1"))
   {
       smi_qos_set_port_da_priority_override (azg, ifname,
                                              SMI_QOS_DA_PRI_OVERRIDE_COS);
       
       smi_qos_get_port_da_priority_override (azg, ifname,
                                              &da_port_mode);
   printf ("\n Mac Priority Values over_type: %d, \n",
                       da_port_mode);
   }else if(!strcasecmp(action, "dapri2"))
   {
       smi_qos_set_port_da_priority_override (azg, ifname,
                                              SMI_QOS_DA_PRI_OVERRIDE_QUEUE);
       smi_qos_get_port_da_priority_override (azg, ifname,
                                              &da_port_mode);
   printf ("\n Mac Priority Values over_type: %d, \n",
                       da_port_mode);
       
   }else if(!strcasecmp(action, "dapri3"))
   {
       smi_qos_set_port_da_priority_override (azg, ifname,
                                              SMI_QOS_DA_PRI_OVERRIDE_BOTH);
       smi_qos_get_port_da_priority_override (azg, ifname,
                                              &da_port_mode);
   printf ("\n Mac Priority Values over_type: %d, \n",
                       da_port_mode);
       
   }

   return 0;
}


void
smi_check_ret (int ret, enum smi_bridge_pri_ovr_mac_type ovr_mac_type, 
               u_int8_t priority )

{
   printf ("\n Mac Priority Values over_mac_type: %d, priority: %d, \n",
                       ovr_mac_type,priority);

   if (ret < 0 ) printf ("\n Api Failed with the above values \n");
    else printf ("\n The Api Passed using the above values \n");

}

/* Show module menu */
void
show_module_menu(struct smiclient_globals *azg, menuItem *menu)
{
  int i;
  char buf[256];

   while(1) {
      printf("\n\t*****Configuration options*****\n\n");
      for( i = 0; menu[i].caption;i++)
        printf("\t\t%s\t%s\n", menu[i].caption, menu[i].description);
      printf("\nEnter your option: ");
        fflush(stdout);
      if(fgets(buf, 256, stdin))
        buf[strlen(buf)-1] = '\0';
      else
        buf[0] = '\0';
  
      for(i = 0;menu[i].caption;i++) 
        {
        if(!strcmp(buf, menu[i].caption)) 
          {
           if(!strcasecmp(buf, "exit"))
             return;
            read_config_from_file();
            menu[i].func(azg, menu[i].caption);
            break;
          }
        }
        if(!menu[i].caption) 
          printf("\n\tInvalid option...\n");
        printf("\n\t******Press any key to continue...******\n");
        fgets(buf, 2, stdin);
  }
  return;
}

/* Read configuration from the file */
void
read_config_from_file()
{
  char line[32];
  char *ptr = NULL;

  if ((fp_configread = fopen(config_filename, "r")) == NULL)
    {
      printf("File open error: %s\n", config_filename);
      exit(1);
    }
  while(fgets(line, 32, fp_configread) != NULL)
    {
      if((ptr = strstr(line, "ifname")))
        {
          CHK_N_COPY(ifname, tmpval, IS_STR);
        }
      else if((ptr = strstr(line, "vrid")))
        {
          CHK_N_COPY(NULL, vrid, IS_DIGIT);
        }
      else if((ptr = strstr(line, "mtu")))
        {
          CHK_N_COPY(NULL, mtuval, IS_DIGIT);
        }
      else if((ptr = strstr(line, "bw")))
        {
          CHK_N_COPY(NULL, bwval, IS_DIGIT);
        }
      else if((ptr = strstr(line, "duplex")))
        {
          CHK_N_COPY(NULL, dplxval, IS_DIGIT);
        }
      else if((ptr = strstr(line, "crossmode")))
        {
          CHK_N_COPY(NULL, crossmode, IS_DIGIT);
        }
      else if((ptr = strstr(line, "hwaddr")))
        {
          CHK_N_COPY(hwaddr, tmpval, IS_STR);
        }
      else if((ptr = strstr(line, "brname")))
        {
          CHK_N_COPY(brname, tmpval, IS_STR);
        }
      else if((ptr = strstr(line, "ifLacpMode")))
        {
          CHK_N_COPY(NULL, ifLacpMode, IS_DIGIT);
        }
      else if((ptr = strstr(line, "ifAdminKey")))
        {
          CHK_N_COPY(NULL, ifAdminKey, IS_DIGIT);
        }
      else if((ptr = strstr(line, "lacpAdminKey")))
        {
          CHK_N_COPY(NULL, lacpAdminKey, IS_DIGIT);
        }
      else if((ptr = strstr(line, "vlanname")))
        {
          CHK_N_COPY(vlanname, tmpval, IS_STR);
        }
      else if((ptr = strstr(line, "vlanid")))
        {
          CHK_N_COPY(NULL, vlanid, IS_DIGIT);
        }
      else if((ptr = strstr(line, "vlanstate")))
        {
          CHK_N_COPY(NULL, vlanstate, IS_DIGIT);
        }
      else if((ptr = strstr(line, "vlantype")))
        {
          CHK_N_COPY(NULL, vlantype, IS_DIGIT);
        }
      else if((ptr = strstr(line, "vlanportmode")))
        {
          CHK_N_COPY(NULL, vlanportmode, IS_DIGIT);
        }
      else if((ptr = strstr(line, "vlanportsubmode")))
        {
          CHK_N_COPY(NULL, vlanportsubmode, IS_DIGIT);
        }
      else if((ptr = strstr(line, "vlanframetype")))
        {
          CHK_N_COPY(NULL, vlanframetype, IS_DIGIT);
        }
      else if((ptr = strstr(line, "vlaningressfilter")))
        {
          CHK_N_COPY(NULL, vlaningressfilter, IS_DIGIT);
        }
      else if((ptr = strstr(line, "linkname")))
        {
          CHK_N_COPY(linkname, tmpval, IS_STR);
        }
     else if((ptr = strstr(line, "priority")))
        {
          CHK_N_COPY(NULL, priority, IS_DIGIT);
        }
     else if((ptr = strstr(line, "vlanegresstype")))
        {
          CHK_N_COPY(NULL, vlanegresstype, IS_DIGIT);
        }
     else if ((ptr = strstr(line, "vlan_add_opt")))
       {
         CHK_N_COPY(NULL, vlan_add_opt, IS_DIGIT);
       } 
     /* LLDP */
     else if((ptr = strstr(line, "admin_status")))
        {
          CHK_N_COPY(NULL, admin_status, IS_DIGIT);
        }
     else if((ptr = strstr(line, "tx_hold")))
        {
          CHK_N_COPY(NULL, tx_hold, IS_DIGIT);
        }
     else if((ptr = strstr(line, "tx_interval")))
        {
          CHK_N_COPY(NULL, tx_interval, IS_DIGIT);
        }
     else if((ptr = strstr(line, "re_delay")))
        {
          CHK_N_COPY(NULL, re_delay, IS_DIGIT);
        }
     else if((ptr = strstr(line, "tx_delay")))
        {
          CHK_N_COPY(NULL, tx_delay, IS_DIGIT);
        }
     else if((ptr = strstr(line, "neighb_limit")))
        {
          CHK_N_COPY(NULL, neighb_limit, IS_DIGIT);
        }
     else if((ptr = strstr(line, "neighb_interval")))
        {
          CHK_N_COPY(NULL, neighb_interval, IS_DIGIT);
        }
     else if((ptr = strstr(line, "neighb_interval")))
        {
          CHK_N_COPY(NULL, neighb_interval, IS_DIGIT);
        }
     else if((ptr = strstr(line, "localstring")))
        {
          CHK_N_COPY(localstring, tmpval, IS_STR);
        }
     else if((ptr = strstr(line, "descriptor")))
        {
          CHK_N_COPY(descriptor, tmpval, IS_STR);
        }
     else if((ptr = strstr(line, "systemname")))
        {
          CHK_N_COPY(systemname, tmpval, IS_STR);
        }
     else if((ptr = strstr(line, "cfmmaname")))
        {
          CHK_N_COPY(cfmmaname, tmpval, IS_STR);
        }
    else if((ptr = strstr(line, "cfmmdname")))
        {
          CHK_N_COPY(cfmmdname, tmpval, IS_STR);
        }
    else if((ptr = strstr(line, "cfmdoname")))
        {
          CHK_N_COPY(cfmdoname, tmpval, IS_STR);
        }
    else if((ptr = strstr(line, "cfmmaname")))
        {
          CHK_N_COPY(cfmmaname, tmpval, IS_STR);
        }
    else if((ptr = strstr(line, "cfmethtype")))
        {
          CHK_N_COPY(cfmethtype, tmpval, IS_STR);
        }
    else if((ptr = strstr(line, "cfmvid")))
        {
          CHK_N_COPY(NULL, cfmvid, IS_DIGIT);
        }
    else if((ptr = strstr(line, "cfmmepid")))
        {
          CHK_N_COPY(NULL, cfmmepid, IS_DIGIT);
        }
    else if((ptr = strstr(line, "cfmrmepid")))
        {
          CHK_N_COPY(NULL, cfmrmepid, IS_DIGIT);
        }
    else if((ptr = strstr(line, "cfmentries")))
        {
          CHK_N_COPY(NULL, cfmentries, IS_DIGIT);
        }
    else if((ptr = strstr(line, "cfmseqno")))
        {
          CHK_N_COPY(NULL, cfmseqno, IS_DIGIT);
        }
    else if((ptr = strstr(line, "cfmlevel")))
        {
          CHK_N_COPY(NULL, cfmlevel, IS_DIGIT);
        }
    else if((ptr = strstr(line, "mstpinst")))
        {
          CHK_N_COPY(NULL, mstpinst, IS_DIGIT);
        }
    else if((ptr = strstr(line, "mstprevnu")))
        {
          CHK_N_COPY(NULL, mstprevnu, IS_DIGIT);
        }
    else if((ptr = strstr(line, "mstpbrpriority")))
        {
          CHK_N_COPY(NULL, mstpbrpriority, IS_DIGIT);
        }
    else if((ptr = strstr(line, "mstpportpr")))
        {
          CHK_N_COPY(NULL, mstpportpr, IS_DIGIT);
        }
    else if((ptr = strstr(line, "mstpmstiportpr")))
        {
          CHK_N_COPY(NULL, mstpmstiportpr, IS_DIGIT);
        }
    else if((ptr = strstr(line, "mstpmstirstrrole")))
        {
          CHK_N_COPY(NULL, mstpmstirstrrole, IS_DIGIT);
        }
    else if((ptr = strstr(line, "mstpmstibrpr")))
        {
          CHK_N_COPY(NULL, mstpmstibrpr, IS_DIGIT);
        }
    else if((ptr = strstr(line, "mstpporthtime")))
        {
          CHK_N_COPY(NULL, mstpporthtime, IS_DIGIT);
        }
    else if((ptr = strstr(line, "mstpportp2p")))
        {
          CHK_N_COPY(NULL, mstpportp2p, IS_DIGIT);
        }
    else if((ptr = strstr(line, "mstpageing")))
        {
          CHK_N_COPY(NULL, mstpageing, IS_DIGIT);
        }
    else if((ptr = strstr(line, "mstphtime")))
        {
          CHK_N_COPY(NULL, mstphtime, IS_DIGIT);
        }
    else if((ptr = strstr(line, "mstpmaxage")))
        {
          CHK_N_COPY(NULL, mstpmaxage, IS_DIGIT);
        }
    else if((ptr = strstr(line, "mstpportedge")))
        {
          CHK_N_COPY(NULL, mstpportedge, IS_DIGIT);
        }
    else if((ptr = strstr(line, "mstpforcever")))
        {
          CHK_N_COPY(NULL, mstpforcever, IS_DIGIT);
        }
    else if((ptr = strstr(line, "mstpfwdelay")))
        {
          CHK_N_COPY(NULL, mstpfwdelay, IS_DIGIT);
        }
    else if((ptr = strstr(line, "mstppathcost")))
        {
          CHK_N_COPY(NULL, mstppathcost, IS_DIGIT);
        }
    else if((ptr = strstr(line, "mstpcstipcost")))
        {
          CHK_N_COPY(NULL, mstpcstipcost, IS_DIGIT);
        }
    else if((ptr = strstr(line, "mstpmaxhops")))
        {
          CHK_N_COPY(NULL, mstpmaxhops, IS_DIGIT);
        }
    else if((ptr = strstr(line, "mstpbrtype")))
        {
          CHK_N_COPY(NULL, mstpbrtype, IS_DIGIT);
        }
    else if((ptr = strstr(line, "mstperrdistout")))
        {
          CHK_N_COPY(NULL, mstperrdistout, IS_DIGIT);
        }
    else if((ptr = strstr(line, "mstpportpr")))
        {
          CHK_N_COPY(NULL, mstpportpr, IS_DIGIT);
        }
    else if((ptr = strstr(line, "mstpisforward")))
        {
          CHK_N_COPY(NULL, mstpisforward, IS_DIGIT);
        }
    else if((ptr = strstr(line, "mstpuserpr")))
        {
          CHK_N_COPY(NULL, mstpuserpr, IS_DIGIT);
        }
    else if((ptr = strstr(line, "mstpmstirsrtrole")))
        {
          CHK_N_COPY(NULL, mstpmstirsrtrole, IS_DIGIT);
        }
    else if((ptr = strstr(line, "mstprestricttcn")))
        {
          CHK_N_COPY(NULL, mstprestricttcn, IS_DIGIT);
        }
    else if((ptr = strstr(line, "mstpportrsrtrole")))
        {
          CHK_N_COPY(NULL, mstpportrsrtrole, IS_DIGIT);
        }
    else if((ptr = strstr(line, "mstpporttcn")))
        {
          CHK_N_COPY(NULL, mstpporttcn, IS_DIGIT);
        }
    else if((ptr = strstr(line, "mstpportrtgrd")))
        {
          CHK_N_COPY(NULL, mstpportrtgrd, IS_DIGIT);
        }
    else if((ptr = strstr(line, "mstpbrforward")))
        {
          CHK_N_COPY(NULL, mstpbrforward, IS_DIGIT);
        }
     else if((ptr = strstr(line, "mstpportfastbpgrd")))
        {
          CHK_N_COPY(NULL, mstpportfastbpgrd, IS_DIGIT);
        }
     else if((ptr = strstr(line, "mstperrdistoen")))
        {
          CHK_N_COPY(NULL, mstperrdistoen, IS_DIGIT);
        }
     else if((ptr = strstr(line, "mstpautoedge")))
        {
          CHK_N_COPY(NULL, mstpautoedge, IS_DIGIT);
        }
     else if((ptr = strstr(line, "mstpportbpfltr")))
        {
          CHK_N_COPY(NULL, mstpportbpfltr, IS_DIGIT);
        }
     else if((ptr = strstr(line, "mstpportbpgrd")))
        {
          CHK_N_COPY(NULL, mstpportbpgrd, IS_DIGIT);
        }
     else if((ptr = strstr(line, "mstptxholdcnt")))
        {
          CHK_N_COPY(NULL, mstptxholdcnt, IS_DIGIT);
        }
     else if((ptr = strstr(line, "mstpregname")))
        {
          CHK_N_COPY(mstpregname, tmpval, IS_STR);
        }
     else if((ptr = strstr(line, "mstpportfastfltr")))
        {
          CHK_N_COPY(NULL, mstpportfastfltr, IS_DIGIT);
        }
     else if((ptr = strstr(line, "mstptopotype")))
        {
          CHK_N_COPY(NULL, mstptopotype, IS_DIGIT);
        }
     else if((ptr = strstr(line, "gvrptimertype")))
        {
          CHK_N_COPY(NULL, gvrptimertype, IS_DIGIT);
        }
     else if((ptr = strstr(line, "gvrptimervalue")))
        {
          CHK_N_COPY(NULL, gvrptimervalue, IS_DIGIT);
        }
     else if((ptr = strstr(line, "gvrpregtype")))
        {
          CHK_N_COPY(gvrpregtype, tmpval, IS_STR);
        }
     else if((ptr = strstr(line, "gvrpregmode")))
        {
          CHK_N_COPY(NULL, gvrpregmode, IS_DIGIT);
        }
     else if((ptr = strstr(line, "gvrpdynvlanlrn")))
        {
          CHK_N_COPY(NULL, gvrpdynvlanlrn, IS_DIGIT);
        }
     else if((ptr = strstr(line, "gvrpfirstcall")))
        {
          CHK_N_COPY(NULL, gvrpfirstcall, IS_DIGIT);
        }
     else if((ptr = strstr(line, "gvrpgididx")))
        {
          CHK_N_COPY(NULL, gvrpgididx, IS_DIGIT);
        }
     else if((ptr = strstr(line, "gvrpnoentries")))
        {
          CHK_N_COPY(NULL, gvrpnoentries, IS_DIGIT);
        }
     /* EFM Params */
     else if((ptr = strstr(line, "efmlinktimer")))
        {
          CHK_N_COPY(NULL, efmlinktimer, IS_DIGIT);
        }
     else if((ptr = strstr(line, "efmpdutimer")))
        {
          CHK_N_COPY(NULL, efmpdutimer, IS_DIGIT);
        }
     else if((ptr = strstr(line, "efmoammode")))
        {
          CHK_N_COPY(NULL, efmoammode, IS_DIGIT);
        }
     else if((ptr = strstr(line, "efmthreslow")))
        {
          CHK_N_COPY(NULL, efmthreslow, IS_DIGIT);
        }
     else if((ptr = strstr(line, "efmthreshigh")))
        {
          CHK_N_COPY(NULL, efmthreshigh, IS_DIGIT);
        }
     else if((ptr = strstr(line, "efmsecthreslow")))
        {
          CHK_N_COPY(NULL, efmsecthreslow, IS_DIGIT);
        }
     else if((ptr = strstr(line, "efmsecthreshigh")))
        {
          CHK_N_COPY(NULL, efmsecthreshigh, IS_DIGIT);
        }
     else if((ptr = strstr(line, "efmwindowth")))
        {
          CHK_N_COPY(NULL, efmwindowth, IS_DIGIT);
        }
     else if((ptr = strstr(line, "efmlbtimeout")))
        {
          CHK_N_COPY(NULL, efmlbtimeout, IS_DIGIT);
        }
     else if((ptr = strstr(line, "efmnoframes")))
        {
          CHK_N_COPY(NULL, efmnoframes, IS_DIGIT);
        }
     else if((ptr = strstr(line, "efmlinkmonen")))
        {
          CHK_N_COPY(NULL, efmlinkmonen, IS_DIGIT);
        }
     else if((ptr = strstr(line, "efmmaxpdu")))
        {
          CHK_N_COPY(NULL, efmmaxpdu, IS_DIGIT);
        }
     else if((ptr = strstr(line, "efmevent")))
        {
          CHK_N_COPY(NULL, efmevent, IS_DIGIT);
        }
     else if((ptr = strstr(line, "efmeventenable")))
        {
          CHK_N_COPY(NULL, efmeventenable, IS_DIGIT);
        }
     else if((ptr = strstr(line, "efmtestbuf")))
        {
          CHK_N_COPY(efmtestbuf, tmpval, IS_STR);
        }
     else if((ptr = strstr(line, "rmonhistindex")))
        {
          CHK_N_COPY(NULL, rmonhistindex, IS_DIGIT);
        }
     else if((ptr = strstr(line, "rmonevtindx")))
        {
          CHK_N_COPY(NULL, rmonevtindx, IS_DIGIT);
        }
     else if((ptr = strstr(line, "rmonhistint")))
        {
          CHK_N_COPY(NULL, rmonhistint, IS_DIGIT);
        }
     else if((ptr = strstr(line, "rmonalarmint")))
        {
          CHK_N_COPY(NULL, rmonalarmint, IS_DIGIT);
        }
     else if((ptr = strstr(line, "rmonalarmtype")))
        {
          CHK_N_COPY(NULL, rmonalarmtype, IS_DIGIT);
        }
     else if((ptr = strstr(line, "rmonalarmstartup")))
        {
          CHK_N_COPY(NULL, rmonalarmstartup, IS_DIGIT);
        }
     else if((ptr = strstr(line, "rmonrisingindx")))
        {
          CHK_N_COPY(NULL, rmonrisingindx, IS_DIGIT);
        }
     else if((ptr = strstr(line, "rmonfallingindx")))
        {
          CHK_N_COPY(NULL, rmonfallingindx, IS_DIGIT);
        }
     else if((ptr = strstr(line, "rmonalarmindx")))
        {
          CHK_N_COPY(NULL, rmonalarmindx, IS_DIGIT);
        }
     else if((ptr = strstr(line, "rmonalarmstatus")))
        {
          CHK_N_COPY(NULL, rmonalarmstatus, IS_DIGIT);
        }
     else if((ptr = strstr(line, "rmonevtstatus")))
        {
          CHK_N_COPY(NULL, rmonevtstatus, IS_DIGIT);
        }
     else if((ptr = strstr(line, "rmonhiststatus")))
        {
          CHK_N_COPY(NULL, rmonhiststatus, IS_DIGIT);
        }
     else if((ptr = strstr(line, "rmonethstindex")))
        {
          CHK_N_COPY(NULL, rmonethstindex, IS_DIGIT);
        }
     else if((ptr = strstr(line, "rmonifindex")))
        {
          CHK_N_COPY(NULL, rmonifindex, IS_DIGIT);
        }
     else if((ptr = strstr(line, "rmonbucket")))
        {
          CHK_N_COPY(NULL, rmonbucket, IS_DIGIT);
        }
     else if((ptr = strstr(line, "rmonevttype")))
        {
          CHK_N_COPY(NULL, rmonevttype, IS_DIGIT);
        }
     else if((ptr = strstr(line, "rmonethstatus")))
        {
          CHK_N_COPY(NULL, rmonethstatus, IS_DIGIT);
        }
     else if((ptr = strstr(line, "rmonsnmpevttype")))
        {
          CHK_N_COPY(NULL, rmonsnmpevttype, IS_DIGIT);
        }
     else if((ptr = strstr(line, "rmonalarmown")))
        {
          CHK_N_COPY(rmonalarmown, tmpval, IS_STR);
        }
     else  if((ptr = strstr(line, "rmonevetown")))
        {
          CHK_N_COPY(rmonevetown, tmpval, IS_STR);
        }
     else if((ptr = strstr(line, "rmonhistown")))
        {
          CHK_N_COPY(rmonhistown, tmpval, IS_STR);
        }
     else if((ptr = strstr(line, "rmonethstatsown")))
        {
          CHK_N_COPY(rmonethstatsown, tmpval, IS_STR);
        }
     else if((ptr = strstr(line, "rmonalarmcomm")))
        {
          CHK_N_COPY(rmonalarmcomm, tmpval, IS_STR);
        }
     else if((ptr = strstr(line, "rmoneventstatus")))
        {
          CHK_N_COPY(rmoneventstatus, rmoneventstatus,
                     IS_DIGIT);
        }

     else if((ptr = strstr(line, "rmonevtcomm")))
        {
          CHK_N_COPY(rmonevtcomm, tmpval, IS_STR);
        }
     else if((ptr = strstr(line, "rmonevtdesc")))
        {
          CHK_N_COPY(rmonevtdesc, tmpval, IS_STR);
        }
     else if((ptr = strstr(line, "rmonalarmword")))
        {
          CHK_N_COPY(rmonalarmword, tmpval, IS_STR);
        }
     else if((ptr = strstr(line, "rmonalarmdesc")))
        {
          CHK_N_COPY(rmonalarmdesc, tmpval, IS_STR);
        }
     else if((ptr = strstr(line, "rmonsnmpevtcomm")))
        {
          CHK_N_COPY(rmonsnmpevtcomm, tmpval, IS_STR);
        }
     else if((ptr = strstr(line, "rmonsnmpevtdesc")))
        {
          CHK_N_COPY(rmonsnmpevtdesc, tmpval, IS_STR);
        }
     else if((ptr = strstr(line, "rmonsnmpevtown")))
        {
          CHK_N_COPY(rmonsnmpevtown, tmpval, IS_STR);
        }
     else if((ptr = strstr(line, "rmonoevtindx")))
        {
          CHK_N_COPY(NULL, rmonevtindx, IS_DIGIT);
        }
     else if((ptr = strstr(line, "rmonrisingval")))
        {
          CHK_N_COPY(NULL, rmonrisingval.l[0], IS_DIGIT);
        }
     else if((ptr = strstr(line, "rmonfallval")))
        {
          CHK_N_COPY(NULL, rmonfallval.l[0], IS_DIGIT);
        }
     else if((ptr = strstr(line, "l2proto")))
        {
          CHK_N_COPY(NULL, l2proto, IS_DIGIT);
        }
     else if((ptr = strstr(line, "protoprocess")))
        {
          CHK_N_COPY(NULL, protoprocess, IS_DIGIT);
        }
     /* QOS */
     else if((ptr = strstr(line, "qostraffictype")))
        {
          CHK_N_COPY(NULL, qostraffictype, IS_DIGIT);
        }
     else if((ptr = strstr(line, "qosratevalue")))
        {
          CHK_N_COPY(NULL, qosratevalue, IS_DIGIT);
        }
     else if((ptr = strstr(line, "qoscommit")))
        {
          CHK_N_COPY(NULL, qoscommit, IS_DIGIT);
        }
     else if((ptr = strstr(line, "qosexcess")))
        {
          CHK_N_COPY(NULL, qosexcess, IS_DIGIT);
        }
     else if((ptr = strstr(line, "qosmatchmode")))
        {
          CHK_N_COPY(NULL, qosmatchmode, IS_DIGIT);
        }
     else if((ptr = strstr(line, "qosrateunit")))
        {
          CHK_N_COPY(NULL, qosrateunit, IS_DIGIT);
        }
     else if((ptr = strstr(line, "qosfcmode")))
        {
          CHK_N_COPY(NULL, qosfcmode, IS_DIGIT);
        }
     else if((ptr = strstr(line, "qosaction")))
        {
          CHK_N_COPY(NULL, qosaction, IS_DIGIT);
        }
     else if((ptr = strstr(line, "qoscos")))
        {
          CHK_N_COPY(NULL, qoscos, IS_DIGIT);
        }
     else if((ptr = strstr(line, "qosportmode")))
        {
          CHK_N_COPY(NULL, qosportmode, IS_DIGIT);
        }
     else if((ptr = strstr(line, "qospriority")))
        {
          CHK_N_COPY(NULL, qospriority, IS_DIGIT);
        }
     else if((ptr = strstr(line, "qosuserpr")))
        {
          CHK_N_COPY(NULL, qosuserpr, IS_DIGIT);
        }
     else if ((ptr= strstr(line, "qosregenpri")))
        {
          CHK_N_COPY(NULL, qosregenpri, IS_DIGIT);
        }
     else if((ptr = strstr(line, "qosqid")))
        {
          CHK_N_COPY(NULL, qosqid, IS_DIGIT);
        }
     else if((ptr = strstr(line, "qosdscp")))
        {
          CHK_N_COPY(NULL, qosdscp, IS_DIGIT);
        }
     else if((ptr = strstr(line, "qostrust")))
        {
          CHK_N_COPY(NULL, qostrust, IS_DIGIT);
        }
     else if((ptr = strstr(line, "qosforcetrust")))
        {
          CHK_N_COPY(NULL, qosforcetrust, IS_DIGIT);
        }
     else if((ptr = strstr(line, "qosframe")))
        {
          CHK_N_COPY(NULL, qosframe, IS_DIGIT);
        }
     else if((ptr = strstr(line, "qosvid")))
        {
          CHK_N_COPY(NULL, qosvid, IS_DIGIT);
        }
     else if((ptr = strstr(line, "qosvlanpmode")))
        {
          CHK_N_COPY(NULL, qosvlanpmode, IS_DIGIT);
        }
     else if((ptr = strstr(line, "qosqweight")))
        {
          CHK_N_COPY(NULL, qosqweight, IS_DIGIT);
        }
     else if((ptr = strstr(line, "qosdir")))
        {
          CHK_N_COPY(NULL, qosdir, IS_DIGIT);
        }
     else if((ptr = strstr(line, "qoscmapname")))
        {
          CHK_N_COPY(qoscmapname, tmpval, IS_STR);
        }
     else if((ptr = strstr(line, "qospmapname")))
        {
          CHK_N_COPY(qospmapname, tmpval, IS_STR);
        }
     else if((ptr = strstr(line, "qospolicyname")))
        {
          CHK_N_COPY(qospolicyname, tmpval, IS_STR);
        }
     /* FC */
     else if((ptr = strstr(line, "fcdir")))
        {
          CHK_N_COPY(NULL, fcdir, IS_DIGIT);
        }
    else if((ptr = strstr(line, "fcifindex")))
        {
          CHK_N_COPY(NULL, fcifindex, IS_DIGIT);
        }
     else if((ptr = strstr(line, "lacpaggix")))
        {
          CHK_N_COPY(NULL, lacpaggix, IS_DIGIT);
        }
     else if((ptr = strstr(line, "if_port_conf_state")))
        {
          CHK_N_COPY(NULL, if_port_conf_state, IS_DIGIT);
        }
     else if((ptr = strstr(line, "if_port_lrn_state")))
        {
          CHK_N_COPY(NULL, if_port_lrn_state, IS_DIGIT);
        }
     else if((ptr = strstr(line, "port_nonsw_state")))
        {
          CHK_N_COPY(NULL, port_nonsw_state, IS_DIGIT);
        }
     else if((ptr = strstr(line, "if_egress_mode")))
        {
          CHK_N_COPY(NULL, if_egress_mode, IS_DIGIT);
        }
     else if((ptr = strstr(line, "if_ether_type")))
        {
          CHK_N_COPY(NULL, if_ether_type, IS_DIGIT);
        }
     else if((ptr = strstr(line, "lldpchassisidtype")))
        {
          CHK_N_COPY(NULL, lldpchassisidtype, IS_DIGIT);
        }
     else if((ptr = strstr(line, "lldpchassisip")))
        {
          CHK_N_COPY(lldpchassisip, tmpval, IS_STR);
        }
     else if((ptr = strstr(line, "act_stb")))
        {
          CHK_N_COPY(NULL, act_stb, IS_DIGIT);
        }
     else if((ptr = strstr(line, "ovr_mac_type")))
        {
          CHK_N_COPY(NULL, ovr_mac_type, IS_DIGIT);
        }
     else if((ptr = strstr(line, "prio")))
        {
          CHK_N_COPY(NULL, prio, IS_DIGIT);
        }
    }
  fclose(fp_configread);
  return;
}

/* Dump arguments from the configuration file */
int
dumpargs()
{
  int i;
  printf("\n******Configuration param dump*******\n\n");
  printf("Interface name:\t\t%s\n", ifname);
  printf("MTU:\t\t\t%d\n", mtuval);
  printf("Interface Bandwidth:\t\t%d\n", bwval);
  printf("Duplex:\t\t\t%d\n", dplxval);
  printf("crossmode:\t\t\t%d\n", crossmode);
  printf("HW Address:\t\t%x%x.%x%x.%x%x\n", hwaddr[0],hwaddr[1],
                                                  hwaddr[2],hwaddr[3],
                                                  hwaddr[4],hwaddr[5]);
  printf("Bridge Name:\t\t%s\n", brname);
  printf("Vlan Name:\t\t%s\n", vlanname);
  printf("Vlan id:\t\t%d\n", vlanid);
  printf("Vlan state:\t\t%d\n", vlanstate);
  printf("Vlan type:\t\t%d\n", vlantype);
  printf("Vlan port mode:\t\t%d\n", vlanportmode);
  printf("Vlan port submode:\t%d\n", vlanportsubmode);
  printf("Vlan frame type:\t%d\n", vlanframetype);
  printf("Vlan ingress filter:\t%d\n", vlaningressfilter);
  printf("Vlan ADD BMP\n" );
        for (i=0; i < SMI_BMP_WORD_MAX; i++)
        {
          if (SMI_BMP_IS_MEMBER(vlanaddbmp, i))
             printf("\n\t vlan%d", i);
        }
  printf("LACP Link name:\t\t%s\n", linkname);
  printf("LACP priority:\t\t%d\n", priority);
  printf("LLDP admin_status:\t%d\n", admin_status);
  printf("LLDP neighb_limit:\t%d\n", neighb_limit);
  printf("LLDP neighb_interval:\t%d\n", neighb_interval);
  printf("LLDP tx_hold:\t\t%d\n", tx_hold);
  printf("LLDP tx_interval:\t%d\n", tx_interval);
  printf("LLDP tx_delay:\t\t%d\n", tx_delay);
  printf("LLDP re_delay:\t\t%d\n", re_delay);
  printf("LLDP systemname:\t%s\n", systemname);
  printf("LLDP systemdescriptor:\t%s\n", descriptor);
  printf("LLDP localstring:\t%s\n", localstring);
  /* MSTP */
  printf("MSTP Instance:\t\t%d\n", mstpinst);
  printf("MSTP Bridge priority:\t%d\n", mstpbrpriority);
  printf("MSTP Msti Bridge Pri:\t%d\n", mstpmstibrpr);
  printf("MSTP Port Hellotime:\t%d\n", mstpporthtime);
  printf("MSTP Portp2p:\t\t%d\n", mstpportp2p);
  printf("MSTP Ageing Time:\t%d\n", mstpageing);
  printf("MSTP Hello Time:\t%d\n", mstphtime);
  printf("MSTP Max Age:\t\t%d\n", mstpmaxage);
  printf("MSTP Port Edge:\t\t%d\n", mstpportedge);
  printf("MSTP Force Version:\t%d\n", mstpforcever);
  printf("MSTP Forward Delay:\t%d\n", mstpfwdelay);
  printf("MSTP Path Cost:\t\t%d\n", mstppathcost);
  printf("MSTP Csti PathCost:\t%d\n", mstpcstipcost);
  printf("MSTP Errdisable TO:\t%d\n", mstperrdistout);
  printf("MSTP Msti Port Pr:\t%d\n", mstpmstiportpr);
  printf("MSTP Revision Num:\t%d\n", mstprevnu);
  printf("MSTP Port Priority:\t%d\n", mstpportpr);
  printf("MSTP Is Foward:\t\t%d\n", mstpisforward);
  printf("MSTP Region Name:\t%s\n", mstpregname);
  printf("MSTP User Priority:\t%d\n", mstpuserpr);
  printf("MSTP Msti RestirctedRol:%d\n", mstpmstirsrtrole);
  printf("MSTP Msti RestrictedTCN:%d\n", mstprestricttcn);
  printf("MSTP Port RestrictedRol:%d\n", mstpportrsrtrole);
  printf("MSTP Port RestrictedTCN:%d\n", mstpporttcn);
  printf("MSTP Port root Guard:\t%d\n", mstpportrtgrd);
  printf("MSTP Bridge Forward:\t%d\n", mstpbrforward);
  printf("MSTP Port Fast Bpdugrd:\t%d\n", mstpportfastbpgrd);
  printf("MSTP Errdisabif TO En :\t%d\n", mstperrdistoen);
  printf("MSTP Auto Edge:\t\t%d\n", mstpautoedge);
  printf("MSTP Bpdu Filter:\t%d\n", mstpportbpfltr);
  printf("MSTP Br Type:\t\t%d\n", mstpbrtype);
  printf("MSTP Max Hops :\t\t%d\n", mstpmaxhops);
  printf("MSTP Port BpduGuard:\t%d\n", mstpportbpgrd);
  printf("MSTP TX HoldCount:\t%d\n", mstptxholdcnt);
  printf("MSTP Topology Type:\t%d\n", mstptopotype);
  printf("MSTP Portfastbpdufltr:\t%d\n", mstpportfastfltr);
  printf("GvRP timer type:\t%d\n", gvrptimertype);
  printf("GvRP timer value:\t%d\n", gvrptimervalue);
  printf("GvRP reg type:\t\t%s\n", gvrpregtype);
  printf("GvRP reg mode:\t\t%d\n", gvrpregmode);

   /* EFM */
  printf("EFM Link timer:\t\t%d\n", efmlinktimer);
  printf("EFM Pdu timer:\t\t%d\n", efmpdutimer);
  printf("EFM Oam Mode:\t\t%d\n", efmoammode);
  printf("EFM Threshold Low:\t%d\n", efmthreslow);
  printf("EFM Threshold high:\t%d\n", efmthreshigh);
  printf("EFM Sec Threslow:\t%d\n", efmsecthreslow);
  printf("EFM Sec Threshigh:\t%d\n", efmsecthreshigh);
  printf("EFM Window Thres:\t%d\n", efmwindowth);
  printf("EFM Loopback Timeout:\t%d\n", efmlbtimeout);
  printf("EFM Link Monitor en:\t%d\n", efmlinkmonen);
  printf("EFM Event:\t\t%d\n", efmevent);
  printf("EFM Event enable:\t%d\n", efmeventenable);
  printf("EFM Testbuf:\t\t%s\n", efmtestbuf);
  printf("EFM Noframes:\t\t%d\n", efmnoframes);
  printf("EFM Max Pdu:\t\t%d\n", efmmaxpdu);

  /* RMON Params */
  printf("RMON History Index:\t%d\n", rmonhistindex);
  printf("RMON Event Index:\t%d\n", rmonevtindx);
  printf("RMON History Interval:\t%d\n", rmonhistint);
  printf("RMON History Status:\t%d\n", rmonhiststatus);
  printf("RMON History Owner:\t%s\n", rmonhistown);
  printf("RMON Alarm PollIntval:\t%d\n", rmonalarmint);
  printf("RMON Alarm Type:\t%d\n", rmonalarmtype);
  printf("RMON Alarm Startup:\t%d\n", rmonalarmstartup);
  printf("RMON Risingindx:\t%d\n", rmonrisingindx);
  printf("RMON Fallingindx:\t%d\n", rmonfallingindx);
  printf("RMON Alarm Index:\t%d\n", rmonalarmindx);
  printf("RMON Alarm Owner:\t%s\n", rmonalarmown);
  printf("RMON Alarm Comm:\t%s\n", rmonalarmcomm);
  printf("RMON Alarm varword:\t%s\n", rmonalarmword);
  printf("RMON Alarm Descr:\t%s\n", rmonalarmdesc);
  printf("RMON Event Status:\t%d\n", rmonevtstatus);
  printf("RMON Event Owner:\t%s\n", rmonevetown);
  printf("RMON Event Comm:\t%s\n", rmonevtcomm);
  printf("RMON Event Descr:\t%s\n", rmonevtdesc);
  printf("RMON Snmp Event Type:\t%d\n", rmonsnmpevttype);
  printf("RMON Snmp Event Comm:\t%s\n", rmonsnmpevtcomm);
  printf("RMON Smnp Event Descr:\t%s\n", rmonsnmpevtdesc);
  printf("RMON Snmp Event Owner:\t%s\n", rmonsnmpevtown);
  printf("RMON Ether Status:\t%d\n", rmonethstatus);
  printf("RMON Ethstats Owner:\t%s\n", rmonethstatsown);
  printf("RMON Rising Thres:\t%d\n", rmonrisingval.l[0]);
  printf("RMON Falling Thres:\t%d\n", rmonfallval.l[0]);
  /* CFM */
  printf("cfmlevel:\t\t%d\n", cfmlevel);
  printf("cfmvid:\t\t%d\n", cfmvid);
  printf("cfm md name:\t\t%s\n", cfmmdname);
  printf("cfm ma name:\t\t%s\n", cfmmaname);
  printf("cfm mepid:\t\t%d\n", cfmmepid);
  printf("cfm rmepid:\t\t%d\n", cfmrmepid);
  printf("cfm entries:\t\t%d\n", cfmentries);
  printf("cfm seq no:\t\t%d\n", cfmseqno);
  printf("cfm ethtype:\t\t%s\n", cfmethtype);
  printf("cfm domain name:\t\t%s\n", cfmdoname);
  /* QOS */ 
  printf("QOS cmap name:\t\t%s\n", qoscmapname);
  printf("QOS pmap name:\t\t%s\n", qospmapname);
  printf("QOS policy name:\t\t%s\n", qospolicyname);
  printf("QOS traffic type:\t\t%d\n", qostraffictype);
  printf("QOS rate value:\t\t%d\n", qosratevalue);
  printf("QOS commit burst size:\t\t%d\n", qoscommit);
  printf("QOS excees burst size:\t\t%d\n", qosexcess);
  printf("QOS match mode:\t\t%d\n", qosmatchmode);
  printf("QOS rate unit:\t\t%d\n", qosrateunit);
  printf("QOS portmode:\t\t%d\n", qosportmode);
  printf("QOS priority:\t\t%d\n", qospriority);
  printf("QOS priority:\t\t%d\n", qosuserpr);
  printf("QOS fc mode:\t\t%d\n", qosfcmode);
  printf("QOS exceed action:\t\t%d\n", qosaction);
  printf("QOS cos:\t\t%d\n", qoscos);
  printf("QOS dscp:\t\t%d\n", qosdscp);
  printf("QOS queue id:\t\t%d\n", qosqid);
  printf("QOS trust:\t\t%d\n", qostrust);
  printf("QOS force trust:\t\t%d\n", qosforcetrust);
  printf("QOS frame:\t\t%d\n", qosframe);
  printf("QOS vid:\t\t%d\n", qosvid);
  printf("QOS vlan port mode:\t\t%d\n", qosvlanpmode);
  printf("QOS queue weight:\t\t%d\n", qosqweight);
  printf("QOS direction:\t\t%d\n", qosdir);
  /* FC */
  printf("FC direction:\t\t%d\n", fcdir);
  printf("FC ifindex:\t\t%d\n", fcifindex);

  return 0;
}

int hwaddr_aton(char *txt, unsigned char *addr)
{
int i;
int a, b;

  for (i = 0; i < 6; i++) {
    a = hex2num(*txt++);
    if (a < 0)
        return -1;
    b = hex2num(*txt++);
    if (b < 0)
        return -1;
    *addr++ = (a << 4) | b;
    if (i < 5 && *txt++ != ':')
        return -1;
   }
  return 0;
}

int hex2num(char c)
{
  if (c >= '0' && c <= '9')
     return c - '0';
  if (c >= 'a' && c <= 'f')
     return c - 'a' + 10;
  if (c >= 'A' && c <= 'F')
     return c - 'A' + 10;
 return -1;
}
#endif /* TEST_AC */

void 
testapi_process_alarm_message (smi_alarm alarm, smi_api_module module, 
                           void * message)
{
  struct smi_msg_alarm *msg = (struct smi_msg_alarm *)message;

  printf ("\nModule causing alarm = %d\n", msg->smi_module);
  printf("Alarm type = %d\n", msg->alarm_type);

  switch (alarm)
  {
    case SMI_ALARM_MEMORY_FAILURE:
      printf("Memory Failure Alarm\n");
    break;
    case SMI_ALARM_NSM_CLIENT_SOCKET_DISCONNECT:
      printf("Client socket disconnect Alarm\n");
      printf("NSM client= %d\n", msg->nsm_client);
    break;
    case SMI_ALARM_NSM_SERVER_SOCKET_DISCONNECT:
      printf("NSM server socket disconnect Alarm\n");
    break;
    case SMI_ALARM_RMON:
    {
      printf("RMON Alarm:\n");
      printf("\tInterface: %s\n", msg->rmon_alarm_info.ifname);
      printf("\tEtherStatObj: %s\n", msg->rmon_alarm_info.etherStatObjName);
      printf("\tSampleType: %d\n", msg->rmon_alarm_info.alarmSampleType);
      printf("\tAlarmType: %d\n", msg->rmon_alarm_info.alarm_type);
      printf("\tThreshold: %d\n", msg->rmon_alarm_info.thresHold.l[0]);
    }
    break;
    case SMI_ALARM_EFM:
    {
      printf("EFM Alarm:\n");
      printf("\tInterface: %s\n", msg->efm_alarm_info.ifname);
      printf("\tFlags: %d\n", msg->efm_alarm_info.flags);
    }
    break;
    case SMI_ALARM_CFM:
    {
      printf("CFM Alarm:\n");
      printf("\tMD Name: %s\n", msg->cfm_alarm_info.md_name);
      printf("\tMA Name: %s\n", msg->cfm_alarm_info.ma_name);
      printf("\tMD Level: %d\n", msg->cfm_alarm_info.level);
      printf("\tVID: %d\n", msg->cfm_alarm_info.vid);
      printf("\tMed id: %d\n", msg->cfm_alarm_info.mep_id);
      printf("\tMed dir: %d\n", msg->cfm_alarm_info.mep_dir);
      printf("\tFlags: %d\n", msg->cfm_alarm_info.flags);
    }
    break;
    case SMI_ALARM_STP:
    {
      printf("STP Alarm:\n");
      printf("\tInterface: %s\n", msg->stp_alarm_info.ifname);
      printf("\tFlags: %d\n", msg->stp_alarm_info.flags);
    }
    break;
    case SMI_ALARM_TRANSPORT_FAILURE:
    {
      printf("Transport Failure Alarm:\n");
      printf("\tDescription: %s\n", msg->description);
    }
    case SMI_ALARM_SMI_SERVER_CONNECT:
      printf("API server connect Alarm\n");
      printf("\tAPI client: %d\n", msg->nsm_client);
    break;
    case SMI_ALARM_SMI_SERVER_DISCONNECT:
      printf("API server disconnect Alarm\n");
      printf("\tAPI client: %d\n", msg->nsm_client);
    break;
    case SMI_ALARM_LOC:
      printf("LOC alarm\n");
      printf("\tInterface: %s\n", msg->loc_alarm_info.ifname);
    break;
    case SMI_ALARM_NSM_VLAN_ADD_TO_PORT:
    {
      int i;
      printf("\nAdd to port alarm\n");
      printf("\tInterface: %s\n", msg->vlan_alarm_info.ifname);
      printf("\tVLANs added:");
      for (i=0; i < SMI_BMP_WORD_MAX; i++)
      {
        if (SMI_BMP_IS_MEMBER(msg->vlan_alarm_info.vlan_bmp, i))
           printf("\n\t vlan %d", i);
      }
      printf("\n");
    }
    break;
    case SMI_ALARM_NSM_VLAN_DEL_FROM_PORT:
    {
      int i;
      printf("\nDelete from port alarm\n");
      printf("\tInterface: %s\n", msg->vlan_alarm_info.ifname);
      printf("\tVLANs deleted:");
      for (i=0; i < SMI_BMP_WORD_MAX; i++)
      {
        if (SMI_BMP_IS_MEMBER(msg->vlan_alarm_info.vlan_bmp, i))
           printf("\n\t vlan %d", i);
      }
      printf("\n");
    }
    break;
    case SMI_ALARM_NSM_VLAN_PORT_MODE:
    printf(" VLAN port mode change alarm\n");
    printf("\tInterface: %s\n", msg->vlan_port_mode_alarm_info.ifname);
    printf("\tMode: %d\n", msg->vlan_port_mode_alarm_info.mode);
    printf("\tSubMode: %d\n", msg->vlan_port_mode_alarm_info.sub_mode);
    break;

    case SMI_ALARM_NSM_BRIDGE_PROTO_CHANGE:
    printf(" Bridge proto change alarm\n");
    printf("\tBridge: %s\n", msg->bridge_proto_change_alarm_info.brname);
    printf("\tType: %d\n", msg->bridge_proto_change_alarm_info.type);
    printf("\tTopo type: %d\n", msg->bridge_proto_change_alarm_info.topo_type);
    break;

    default:
    break;
  }
  return;
}
