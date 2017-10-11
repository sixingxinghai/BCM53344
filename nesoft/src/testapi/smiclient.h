/* Copyright (C) 2002-2003  All Rights Reserved. */

#include "smi_client.h"

#define DFLT_CONFIG_FILE    "/tmp/apiconfig.sample"
char config_filename[64];
FILE *fp_configread;

#ifdef TEST_AC

#define SMI_TRUE            1
#define IS_STR              0
#define IS_DIGIT            1
#define REGION_NAME     "RegionName"
#define MAC_ADDR1   "11:22:33:44:55:11"
#define SMI_ENABLE      1
#define SMI_DISABLE     0

#define CHK_N_COPY(s, d, type)                    \
          ptr = strchr(line, '=');                \
          ++ptr;                                  \
          if(type == IS_STR)                      \
            snprintf(s, strlen(ptr), ptr);        \
          else if(type == IS_DIGIT)               \
            d = atoi(ptr);                         
         
/* Interface params */
int vrid, mtuval, tmpval, dplxval, bwval, crossmode;
int ifAdminKey;
enum smi_lacp_mode ifLacpMode;
int if_port_conf_state, if_port_lrn_state, port_nonsw_state;
int if_egress_mode, if_ether_type;
char ifname[32], hwaddr[32];

/* VLAN params */
char brname[32], vlanname[32];
int vlanid, vlanstate, vlantype, vlanportmode, vlanportsubmode, vlanframetype;
int vlaningressfilter, vlanegresstype;
enum smi_vlan_add_opt vlan_add_opt;
struct smi_vlan_bmp vlanaddbmp, vlanegbmp, vlandelbmp;
int l2proto, protoprocess;

/* LACP params */
char linkname[32];
unsigned int priority;
unsigned int lacpAdminKey;
/* LLDP params */
char systemname[50], descriptor[50], localstring[50];
int admin_status;
int tx_hold, tx_interval, re_delay, tx_delay, neighb_interval;
int neighb_limit, lacpaggix;
u_int8_t lldpchassisidtype;
char lldpchassisip[SMI_IPADDRESS_SIZE];
 
/* CFM params */
char cfmethtype[32], cfmmdname[32], cfmmaname[32], cfmdoname[32];
int cfmlevel, cfmmepid, cfmrmepid, cfmentries, cfmseqno, cfmvid;

/* MSTP params */
int mstpinst, mstpbrpriority, mstpmstibrpr, mstpporthtime, mstpportp2p;
int mstpageing, mstphtime, mstpmaxage, mstpportedge, mstpforcever;
int mstppathcost, mstpcstipcost, mstpmaxhops;
int mstperrdistout, mstpmstiportpr, mstpuserpr;
int mstptopotype, mstpportfastfltr;
unsigned short  mstprevnu, mstpisforward;
char mstpregname[32];
bool_t mstpmstirsrtrole, mstprestricttcn, mstpportrsrtrole;
bool_t mstpporttcn, mstpportrtgrd, mstpbrforward, mstpportfastbpgrd;
bool_t mstperrdistoen, mstpautoedge, mstpmstirstrrole;
u_char mstpbrtype, mstpportbpgrd, mstptxholdcnt;
u_char mstpportbpfltr;
/* MSTP  params ends here */
s_int32_t mstpfwdelay;
int mstpportpr;
/* EFM Params start */
int efmlinktimer, efmoammode, efmpdutimer;
int efmsecthreslow, efmsecthreshigh;
int efmbuflen, efmnoframes;
u_int16_t efmthreslow, efmthreshigh, efmwindowth;
u_int8_t efmlinkmonen, efmmaxpdu , efmevent, efmeventenable, efmlbtimeout;
char efmtestbuf[30] ;
/* EFM params ends here */

/* GVRP params */
unsigned int gvrptimertype, gvrpregmode, gvrpgididx, gvrpdynvlanlrn;
unsigned short gvrpnoentries;
timer_t gvrptimervalue;
char gvrpregtype[32];
u_int8_t gvrpfirstcall;
s_int32_t timer_details[SMI_GARP_MAX_TIMERS];

/* RMON Params */
unsigned int rmonhistindex, rmonhistint, rmonalarmint, rmonalarmtype;
unsigned int rmonalarmstartup,rmonrisingindx, rmonfallingindx;
unsigned int rmonalarmstatus;
unsigned int  rmonalarmindx, rmonevtstatus, rmonhiststatus;
ut_int64_t rmonrisingval, rmonfallval;
unsigned int rmonethstindex, rmonifindex, rmonbucket, rmonevttype;
unsigned int rmonethstatus;
unsigned char rmonalarmown[15], rmonevetown[15], rmonhistown[15];
unsigned char rmonethstatsown[15];
unsigned char rmonalarmcomm[127], rmonevtcomm[127],rmonevtdesc[255];
unsigned char rmonalarmword[21], rmonalarmdesc[255];
unsigned int rmonsnmpevttype,rmoneventstatus;
unsigned char rmonsnmpevtcomm[127], rmonsnmpevtdesc[255], rmonsnmpevtown[15];
oid rmonalarmoid[10];
int rmonevtindx,eventstatus;
int rmoneventindx;
char mstpmacaddr[18];

/* QOS params */
char qoscmapname[32], qospmapname[32], qospolicyname[32];
int qoscommit, qoscos, qosdscp, qosratevalue, qosexcess, qosrateunit;
int qosmatchmode, qosfcmode, qosaction, qosqid, qostrust, qosforcetrust;
int qosdir, qosqweight, qosvid, qosframe, qosportmode, qosuserpr,
    qostraffictype, qosvlanpmode, qosregenpri;
unsigned char qospriority;
                                                                                
/* HA */
int act_stb;
enum smi_bridge_pri_ovr_mac_type ovr_mac_type;
char prio;
/* Fc params */
unsigned int fcifindex;
int fcdir;


typedef int (*mFunc)();

typedef struct menuItem {
  char *caption;
  char *description;
  mFunc func;
} menuItem;


/* Function prototypes */
int exit_menu();
int quit();
int dumpargs();
void read_config_from_file();
void show_module_menu(struct smiclient_globals *, menuItem *menu);
int smi_if_action(struct smiclient_globals *, char *);
int smi_client_action(struct smiclient_globals *, char *action);
int smi_lacp_action(struct smiclient_globals *azg, char *);
int smi_mstp_action(struct smiclient_globals *azg, char *);
int smi_vlan_action(struct smiclient_globals *azg, char *action);
int smi_rmon_action(struct smiclient_globals *azg, char *);
int smi_lldp_action(struct smiclient_globals *azg, char *);
int smi_cfm_action(struct smiclient_globals *azg, char *);
int smi_efm_action(struct smiclient_globals *azg, char *);
int smi_gvrp_action(struct smiclient_globals *azg, char *);
int smi_qos_action(struct smiclient_globals *azg, char *);
int smi_fc_action(struct smiclient_globals *azg, char *);
int smi_general_action(struct smiclient_globals *azg, char *);

#endif /* TEST_AC */
