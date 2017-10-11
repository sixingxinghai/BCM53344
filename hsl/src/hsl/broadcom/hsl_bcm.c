/* Copyright (C) 2003  All Rights Reserved. */

#include "config.h"
#include "hsl_os.h"

/* 
   Broadcom includes. 
*/
#include "bcm_incl.h"
/*
  HSL includes.
*/
#include "hsl_types.h"
#include "hsl_logger.h"
#include "hsl_ether.h"
#include "hsl.h"
#include "hsl_ifmgr.h"
#include "hsl_if_hw.h"
#include "hsl_if_os.h"

#include "hsl_bcm_ifmap.h"
#include "hsl_bcm_resv_vlan.h"
#include "hsl_bcm_pkt.h"
#include "hsl_bcm.h"

#include "sal/appl/config.h"
#include "appl/cputrans/ct_tun.h"
#include "hsl_bcm_cpu.h"

#include "ptp/constants.h"

/* Extern functions */
extern int cpudb_entry_count_get(cpudb_ref_t db_ref);
extern cpudb_ref_t volatile bcm_st_disc_db;
extern cpudb_ref_t volatile bcm_st_cur_db;


#define HSL_BCM_INVALID_FILTER          0xffffffff
#define HSL_BCM_INVALID_FIELD_GRP       0xffffffff
#define HSL_BCM_INVALID_FIELD_ENTRY     0xffffffff

static u_int32_t l2_age_timeout = HSL_DEFAULT_FDB_TIMEOUT; 
static u_int8_t base_mac[6] = {0,0,0,0,0,1};
static u_char _bcm_soc_type = HSL_BCM_SOC_TYPE_INVALID;
static u_char _bcm_chip_family = HSL_BCM_CHIP_FAMILY_UNKNOWN;
#ifdef HAVE_L3
static bcm_filterid_t _hsl_arp_filter = -1;
static bcm_filterid_t _hsl_llmc_filter = -1;
static bcm_filterid_t _hsl_ra_filter = HSL_BCM_INVALID_FILTER;
static bcm_filterid_t _hsl_bcast_icmp_filter = HSL_BCM_INVALID_FILTER;
static bcm_field_group_t _hsl_bcast_field_grp = HSL_BCM_INVALID_FIELD_GRP;
static bcm_field_entry_t _hsl_bcast_icmp_field_ent = HSL_BCM_INVALID_FIELD_ENTRY;

#ifdef HAVE_RIPD
static bcm_filterid_t _hsl_ripv1_filter = HSL_BCM_INVALID_FILTER;
static bcm_field_entry_t _hsl_ripv1_field_ent = HSL_BCM_INVALID_FIELD_ENTRY;
#endif /* HAVE_RIPD */
static u_int32_t _hsl_ra_udf_id = 0;
#endif /* HAVE_L3 */ 

static bcm_field_group_t _hsl_timesync_field_grp = HSL_BCM_INVALID_FIELD_GRP;/*djg*/
static bcm_field_entry_t _hsl_timesync_field_ent[PTP_NUMBER_PORTS] = { HSL_BCM_INVALID_FIELD_ENTRY,HSL_BCM_INVALID_FIELD_ENTRY,HSL_BCM_INVALID_FIELD_ENTRY,HSL_BCM_INVALID_FIELD_ENTRY,
	                                                   HSL_BCM_INVALID_FIELD_ENTRY,HSL_BCM_INVALID_FIELD_ENTRY,HSL_BCM_INVALID_FIELD_ENTRY,HSL_BCM_INVALID_FIELD_ENTRY,
	                                                   HSL_BCM_INVALID_FIELD_ENTRY,HSL_BCM_INVALID_FIELD_ENTRY,HSL_BCM_INVALID_FIELD_ENTRY,HSL_BCM_INVALID_FIELD_ENTRY,
	                                                   HSL_BCM_INVALID_FIELD_ENTRY,HSL_BCM_INVALID_FIELD_ENTRY,HSL_BCM_INVALID_FIELD_ENTRY,HSL_BCM_INVALID_FIELD_ENTRY,
	                                                   HSL_BCM_INVALID_FIELD_ENTRY,HSL_BCM_INVALID_FIELD_ENTRY,HSL_BCM_INVALID_FIELD_ENTRY,HSL_BCM_INVALID_FIELD_ENTRY};/*djg*/

#ifdef HAVE_ONMD
static bcm_filterid_t _hsl_cfm_filter = -1;
static bcm_field_group_t _hsl_cfm_field_grp = HSL_BCM_INVALID_FIELD_GRP;
static bcm_field_entry_t _hsl_cfm_field_ent = HSL_BCM_INVALID_FIELD_ENTRY;

int
hsl_bcm_cfm_filter_uninstall ();

int
hsl_bcm_cfm_filter_install ();

#endif /* HAVE_ONMD */

static int bcm_filter_feature_type = 0;

/*
  Forward declarations of callbacks. 
*/
int hsl_bcm_ifhdlr_init ();
int hsl_bcm_ifhdlr_deinit ();
int hsl_bcmx_init (void);
int hsl_bcmx_deinit (void);
extern bcmx_uport_t
_hsl_bcm_port_attach_cb (bcmx_lport_t lport, int unit, bcm_port_t port,
                         uint32 flags);

/*
  Broadcom initialization and deinitialization routines.
*/

/* 
  Get type of filtering supported on box.  
*/
int 
hsl_bcm_filter_type_get(void)
{
  HSL_FN_ENTER(); 
  
  if ((bcm_filter_feature_type != HSL_BCM_FEATURE_FIELD) && 
      (bcm_filter_feature_type != HSL_BCM_FEATURE_FILTER)) 
    {
      HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_ERROR, 
               "BCM Filter Type %d\n", bcm_filter_feature_type);
      HSL_FN_EXIT(-1);
    }
  
  HSL_FN_EXIT(bcm_filter_feature_type);
}

/* Init filtering on broadcom. */
static int 
_hsl_bcm_filter_init(void)
{
  int ret; 

  HSL_FN_ENTER();

  /* Attempt to initialize field filtering. */
  ret = bcmx_field_init();

  if (ret == BCM_E_NONE)
    {
      bcm_filter_feature_type = HSL_BCM_FEATURE_FIELD;
      HSL_FN_EXIT(0); 
    }

  HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO, "Error initialize field. Ignoring and trying filter\n");
 
  /* Initialize filtering */
  ret = bcmx_filter_init();
  if (ret == BCM_E_NONE)
    {
      bcm_filter_feature_type = HSL_BCM_FEATURE_FILTER;
      HSL_FN_EXIT(0); 
    }

  HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_FATAL, "Error initialize filter\n");
  HSL_FN_EXIT (-1);
}

/* Deinit filtering on broadcom. */
static int 
_hsl_bcm_filter_deinit(void)
{
  HSL_FN_ENTER(); 
  /* Can't find an api to deinit filters. */
  bcm_filter_feature_type = 0;
  HSL_FN_EXIT(0); 
}

#ifdef HAVE_ONMD

/*
  Install FFP filter for CFM packets.
*/
static int
_hsl_bcmx_ffp_cfm_filter_install (int offset)
{
  int ret;

  HSL_FN_ENTER ();

  /* Install filter to trap CFM to the CPU. */

  ret = bcmx_filter_create (&_hsl_cfm_filter);

  if (ret < 0)
    {
      HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_ERROR,
               "Can't create CFM filter\n");
      HSL_FN_EXIT (-1);
    }

  /* Qualify Non IP packets. */
  ret = bcmx_filter_qualify_format (_hsl_cfm_filter, BCM_FILTER_PKTFMT_NONIP);

  if (ret < 0)
    {
      HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_ERROR,
               "Can't qualify CFM filter\n");
      ret = -1;
      goto ERR;
    }

  /* Qualify CFM Packets */
  ret = bcmx_filter_qualify_data16 (_hsl_cfm_filter, offset, 
                                    HSL_ETHER_TYPE_CFM, 0xffff);
  if (ret < 0)
    {
      HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_ERROR,
               "Can't qualify CFM protocol filter\n");
      ret = -1;
      goto ERR;
    }

  /* Define processing rules. */
  ret = bcmx_filter_action_match (_hsl_cfm_filter, bcmActionCopyToCpu, 0);
  if (ret < 0)
    {
      HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_ERROR,
               "Can't set CFM filter Copy To CPU action\n");
      ret = -1;
      goto ERR;
    }

  ret = bcmx_filter_install (_hsl_cfm_filter);

  if (ret < 0)
    {
      HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_ERROR,
               "Can't install CFM filter\n");
      ret = -1;
      goto ERR;
    }

ERR:
  HSL_FN_EXIT (0);

}

/* Install CFM field entry */
static int
_hsl_bcmx_cfm_field_install ()
{
  int ret;
  bcm_field_qset_t qset;

  HSL_FN_ENTER();

  /* Initialize Qualifier */
  BCM_FIELD_QSET_INIT(qset);

  /* CFM Ethernet Type Qualifier */
  BCM_FIELD_QSET_ADD (qset, bcmFieldQualifyEtherType);
  
  ret = bcmx_field_group_create (qset, BCM_FIELD_GROUP_PRIO_ANY,
                                 &_hsl_cfm_field_grp);
  if (ret != BCM_E_NONE)
   {
     HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_ERROR,
           "Can't create CFM field group\n");
     goto ERR;
   }
  
  ret = bcmx_field_entry_create (_hsl_cfm_field_grp, &_hsl_cfm_field_ent);

  if (ret != BCM_E_NONE)
   {
     HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_ERROR,
             "Can't create CFM field entry\n");
     goto ERR;
   }

  /* Qualify CFM Packets */
  ret = bcmx_field_qualify_EtherType (_hsl_cfm_field_ent, HSL_ETHER_TYPE_CFM,
                                      0xffff);
  if (ret != BCM_E_NONE)
    {
      HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_ERROR,
               "Can't create CFM Ether Type qualifier\n");
      goto ERR;
    }

  ret = bcmx_field_action_add (_hsl_cfm_field_ent, 
                               bcmFieldActionCopyToCpu, 0, 0);
  if (ret != BCM_E_NONE)
    {
      HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_ERROR,
               "Can't set CFM field entry Copy To CPU action\n");
      goto ERR;
    }

  ret = bcmx_field_entry_install (_hsl_cfm_field_ent);

  if (ret != BCM_E_NONE)
    {
      HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_ERROR,
               "Can't install CFM field entry\n");
      goto ERR;
    }

  HSL_FN_EXIT (0);

ERR:

  HSL_FN_EXIT (ret);
}

/*
  Install CFM filter
*/
int
hsl_bcm_cfm_filter_install ()
{
  int ret = -1;
  int filter_type;

  HSL_FN_ENTER ();

  filter_type = hsl_bcm_filter_type_get ();

  if (filter_type == HSL_BCM_FEATURE_FIELD)
    {
      ret = _hsl_bcmx_cfm_field_install ();
    }
  else
    {
      ret = _hsl_bcmx_ffp_cfm_filter_install (IP_PROTO_CFM_UNTAG_OFFSET);
      ret = _hsl_bcmx_ffp_cfm_filter_install (IP_PROTO_CFM_SINGLE_TAG_OFFSET);
      ret = _hsl_bcmx_ffp_cfm_filter_install (IP_PROTO_CFM_DOUBLE_TAG_OFFSET);
    }

  HSL_FN_EXIT (ret);
}

/*
  Uninstall FFP filter for CFM broadcast packets. 
*/
static int
_hsl_bcmx_ffp_cfm_filter_uninstall (void)
{
  HSL_FN_ENTER ();

  if (_hsl_cfm_filter != HSL_BCM_INVALID_FILTER)
    {
      bcmx_filter_remove (_hsl_cfm_filter);
      bcmx_filter_destroy (_hsl_cfm_filter);
      _hsl_cfm_filter = HSL_BCM_INVALID_FILTER;
    } 
  
  HSL_FN_EXIT (0);
}
  
static int
_hsl_bcmx_cfm_field_uninstall ()
{   
  int ret = 0;
    
  HSL_FN_ENTER();

  if (_hsl_cfm_field_ent != HSL_BCM_INVALID_FIELD_ENTRY)
    {
      bcmx_field_entry_destroy (_hsl_cfm_field_ent);
      _hsl_cfm_field_ent = HSL_BCM_INVALID_FIELD_ENTRY;
    }

  if (_hsl_cfm_field_grp != HSL_BCM_INVALID_FIELD_GRP)
    {
      bcmx_field_group_destroy (_hsl_cfm_field_grp);
      _hsl_cfm_field_grp = HSL_BCM_INVALID_FIELD_GRP;
    }

  HSL_FN_EXIT (ret);
}

/*
  Uninstall CFM filter
*/

int
hsl_bcm_cfm_filter_uninstall ()
{
  int ret = -1;
  int filter_type;
  HSL_FN_ENTER ();

  filter_type = hsl_bcm_filter_type_get ();

  if (filter_type == HSL_BCM_FEATURE_FIELD)
    {
      ret = _hsl_bcmx_cfm_field_uninstall ();
    }
  else
    {
      ret = _hsl_bcmx_ffp_cfm_filter_uninstall ();
    }

  HSL_FN_EXIT (ret);
}

#endif /* HAVE_ONMD */


#ifdef HAVE_L3
/*
  Install FFP filter for ARP packets.
*/
static int
_hsl_bcmx_ffp_arp_filter_install (void)
{
  int ret;

  HSL_FN_ENTER ();

  /* Install filter to trap ARP packets to the CPU. As XGSII don't support IPv6,
     we don't need a filter for ND packets. */
  ret = bcmx_filter_create (&_hsl_arp_filter);
  if (ret < 0)
    {
      HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_ERROR,
               "Can't create ARP filter\n");
      HSL_FN_EXIT (-1);
    }

  /* Qualify ARP packets. */
  ret = bcmx_filter_qualify_data16 (_hsl_arp_filter, TYPE_LENGTH_OFFSET, ARP_TYPE, 0xffff);
  if (ret < 0)
    {
      HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_ERROR,
               "Can't qualify ARP filter\n");
      ret = -1;
      goto ERR;
    }

  /* Define processing rules. */
  ret = bcmx_filter_action_match (_hsl_arp_filter, bcmActionCopyToCpu, 0);
  if (ret < 0)
    {
      HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_ERROR,
               "Can't qualify ARP filter\n");
      ret = -1;
      goto ERR;
    }

  ret = bcmx_filter_install (_hsl_arp_filter);
  if (ret < 0)
    {
      HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_ERROR,
               "Can't install ARP filter\n");
      ret = -1;
      goto ERR;
    }

  HSL_FN_EXIT (0);

 ERR:
  bcmx_filter_destroy (_hsl_arp_filter);
  HSL_FN_EXIT (-1);
}


/*
  Uninstall FFP filter for ARP packets. 
*/
static int
_hsl_bcmx_ffp_arp_filter_uninstall (void)
{
  HSL_FN_ENTER ();

  if (_hsl_arp_filter)
    {
      bcmx_filter_remove (_hsl_arp_filter);
      bcmx_filter_destroy (_hsl_arp_filter);
    }

  HSL_FN_EXIT (0);
}

/*
  Install FFP filter for linklocal multicast.
*/
static int
_hsl_bcmx_ffp_llmc_filter_install (void)
{
  int ret;

  HSL_FN_ENTER ();

  /* Install filter to trap linklocal multicast packets to the CPU. */
  ret = bcmx_filter_create (&_hsl_llmc_filter);
  if (ret < 0)
    {
      HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_ERROR,
               "Can't create ARP filter\n");
      HSL_FN_EXIT (-1);
    }

  /* Qualify ARP packets. */
  ret = bcmx_filter_qualify_data32 (_hsl_llmc_filter, IP_DST_OFFSET, 
                                    INADDR_MULTICAST_ADDRESS_BASE, 0xfffffff0);
  if (ret < 0)
    {
      HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_ERROR,
               "Can't qualify ARP filter\n");
      ret = -1;
      goto ERR;
    }

  /* Define processing rules. */
  ret = bcmx_filter_action_match (_hsl_llmc_filter, bcmActionCopyToCpu, 0);
  if (ret < 0)
    {
      HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_ERROR,
               "Can't qualify ARP filter\n");
      ret = -1;
      goto ERR;
    }

  ret = bcmx_filter_install (_hsl_llmc_filter);
  if (ret < 0)
    {
      HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_ERROR,
               "Can't install ARP filter\n");
      ret = -1;
      goto ERR;
    }

  HSL_FN_EXIT (0);

 ERR:
  bcmx_filter_destroy (_hsl_llmc_filter);
  HSL_FN_EXIT (-1);
}

/*
  Uninstall FFP filter for Linklocal multicast packets. 
*/
static int
_hsl_bcmx_ffp_llmc_filter_uninstall (void)
{
  HSL_FN_ENTER ();

  if (_hsl_llmc_filter)
    {
      bcmx_filter_remove (_hsl_llmc_filter);
      bcmx_filter_destroy (_hsl_llmc_filter);
    }

  HSL_FN_EXIT (0);
}

#ifdef HAVE_RIPD
/*
  Install FFP filter for RIPv1 broadcast packets.
*/
static int
_hsl_bcmx_ffp_ripv1_filter_install (void)
{
  int ret;

  HSL_FN_ENTER ();

  /* Install filter to trap linklocal multicast packets to the CPU. */
  ret = bcmx_filter_create (&_hsl_ripv1_filter);
  if (ret < 0)
    {
      HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_ERROR,
               "Can't create RIPv1 filter\n");
      HSL_FN_EXIT (-1);
    }

  /* Qualify Broadcast frames. */
  ret = bcmx_filter_qualify_broadcast (_hsl_ripv1_filter);
  if (ret < 0)
    {
      HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_ERROR,
               "Can't qualify RIPv1 broadcast filter\n");
      ret = -1;
      goto ERR;
    }

  /* Qualify IP packets. */
  ret = bcmx_filter_qualify_format (_hsl_ripv1_filter, BCM_FILTER_PKTFMT_IPV4);
  if (ret < 0)
    {
      HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_ERROR,
               "Can't qualify RIPv1 IPv4 filter\n");
      ret = -1;
      goto ERR;
    }

  /* Qualify UDP protocl type in IP packet. */
  ret = bcmx_filter_qualify_data8 (_hsl_ripv1_filter, IP_PROTO_OFFSET, 
                                    IP_PROTO_UDP_TYPE, 0xff);
  if (ret < 0)
    {
      HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_ERROR,
               "Can't qualify RIPv1 UDP protocol filter\n");
      ret = -1;
      goto ERR;
    }

  /* Note that we are not qualifying on RIP UDP destination port 520.
   * We will let the stack drop UDP packets bound to any other port
   */

  /* Define processing rules. */
  ret = bcmx_filter_action_match (_hsl_ripv1_filter, bcmActionCopyToCpu, 0);
  if (ret < 0)
    {
      HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_ERROR,
               "Can't set RIPv1 filter Copy To CPU action\n");
      ret = -1;
      goto ERR;
    }

  ret = bcmx_filter_install (_hsl_ripv1_filter);
  if (ret < 0)
    {
      HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_ERROR,
               "Can't install RIPv1 filter\n");
      ret = -1;
      goto ERR;
    }

  HSL_FN_EXIT (0);

 ERR:
  bcmx_filter_destroy (_hsl_ripv1_filter);
  HSL_FN_EXIT (-1);
}

/*
  Uninstall FFP filter for RIPv1 broadcast packets. 
*/
static int
_hsl_bcmx_ffp_ripv1_filter_uninstall (void)
{
  HSL_FN_ENTER ();

  if (_hsl_ripv1_filter != HSL_BCM_INVALID_FILTER)
    {
      bcmx_filter_remove (_hsl_ripv1_filter);
      bcmx_filter_destroy (_hsl_ripv1_filter);
      _hsl_ripv1_filter = HSL_BCM_INVALID_FILTER;
    }

  HSL_FN_EXIT (0);
}

static int
_hsl_bcmx_ripv1_field_uninstall ()
{
  int ret = 0;

  HSL_FN_ENTER();

  if (_hsl_ripv1_field_ent != HSL_BCM_INVALID_FIELD_ENTRY)
    {
      bcmx_field_entry_destroy (_hsl_ripv1_field_ent);
      _hsl_ripv1_field_ent = HSL_BCM_INVALID_FIELD_ENTRY;
    }

  /* Do not destroy the group. It will be done in icmp bcast 
   * uninstall.
   */
  HSL_FN_EXIT (ret);
}


/* Install RIPv1 field entry */
static int
_hsl_bcmx_ripv1_field_install ()
{
  int ret;
  bcm_field_qset_t qset;
  bcm_mac_t bcast_mac, bcast_mac_mask; 

  HSL_FN_ENTER();

  /* Initialize bcast mac & mask */
  memset (bcast_mac, 0xff, sizeof (bcm_mac_t));
  memset (bcast_mac_mask, 0xff, sizeof (bcm_mac_t));

  /* Initialize Qualifier */
  BCM_FIELD_QSET_INIT(qset);

  /* L2 DST Mac Qualifier */
  BCM_FIELD_QSET_ADD (qset, bcmFieldQualifyDstMac);
  /* Packet Format Qualifier */
  BCM_FIELD_QSET_ADD (qset, bcmFieldQualifyPacketFormat);
  /* IP Protocol Qualifier */
  BCM_FIELD_QSET_ADD (qset, bcmFieldQualifyIpProtocol);
  
  ret = bcmx_field_group_create (qset, BCM_FIELD_GROUP_PRIO_ANY,
                                 &_hsl_bcast_field_grp);
  if (ret != BCM_E_NONE)
   {
     HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_ERROR,
           "Can't create RIPv1 field group\n");
     goto ERR;
   }
  
  ret = bcmx_field_entry_create (_hsl_bcast_field_grp, &_hsl_ripv1_field_ent);
  if (ret != BCM_E_NONE)
   {
     HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_ERROR,
           "Can't create RIPv1 field entry\n");
     goto ERR;
   }

  /* Qualify L2 Dst Mac */
  ret = bcmx_field_qualify_DstMac (_hsl_ripv1_field_ent, bcast_mac,
                                    bcast_mac_mask);
  if (ret != BCM_E_NONE)
    {
      HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_ERROR,
            "Can't create RIPv1 destination MAC qualifier\n");
      goto ERR;
    }

  /* Qualify Packet Format */
  ret = bcmx_field_qualify_PacketFormat (_hsl_ripv1_field_ent, 
                                          BCM_FIELD_PKT_FMT_IPV4,
                                          0x1f);
  if (ret != BCM_E_NONE)
    {
      HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_ERROR,
            "Can't create RIPv1 IPv4 pkt format qualifier\n");
      goto ERR;
    }

  /* Qualify IP Protocol */
  ret = bcmx_field_qualify_IpProtocol (_hsl_ripv1_field_ent, 
                                          IP_PROTO_UDP_TYPE,
                                          0xff);
  if (ret != BCM_E_NONE)
    {
      HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_ERROR,
            "Can't create RIPv1 IPv4 UDP protocol qualifier\n");
      goto ERR;
    }

  ret = bcmx_field_action_add (_hsl_ripv1_field_ent, 
                               bcmFieldActionCopyToCpu, 0, 0);
  if (ret != BCM_E_NONE)
    {
      HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_ERROR,
            "Can't set RIPv1 field entry Copy To CPU action\n");
      goto ERR;
    }

  ret = bcmx_field_entry_install (_hsl_ripv1_field_ent);
  if (ret != BCM_E_NONE)
    {
      HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_ERROR,
            "Can't install RIPv1 field entry\n");
      goto ERR;
    }

  HSL_FN_EXIT (0);

ERR:
  _hsl_bcmx_ripv1_field_uninstall();
  if (_hsl_bcast_field_grp != HSL_BCM_INVALID_FIELD_GRP)
    bcmx_field_group_destroy (_hsl_bcast_field_grp);

  HSL_FN_EXIT (ret);
}

/*
  Install RIPv1 filter
*/
int
hsl_bcm_ripv1_filter_install ()
{
  int ret = -1;
  int filter_type;

  HSL_FN_ENTER ();
 
  filter_type = hsl_bcm_filter_type_get ();
  if (filter_type == HSL_BCM_FEATURE_FIELD)
    {
      ret = _hsl_bcmx_ripv1_field_install ();
    }
  else
    {
      ret = _hsl_bcmx_ffp_ripv1_filter_install ();
    }

  HSL_FN_EXIT (ret);
}

/*
  Uninstall RIPv1 filter
*/
int
hsl_bcm_ripv1_filter_uninstall ()
{
  int ret = -1;
  int filter_type;
  HSL_FN_ENTER ();

  filter_type = hsl_bcm_filter_type_get ();
  if (filter_type == HSL_BCM_FEATURE_FIELD)
    {
      ret = _hsl_bcmx_ripv1_field_uninstall ();
    }
  else
    {
      ret = _hsl_bcmx_ffp_ripv1_filter_uninstall ();
    }
  HSL_FN_EXIT (ret);
}
#endif /* HAVE_RIPD */

/*
  Install FFP filter for bcast icmp broadcast packets.
*/
static int
_hsl_bcmx_ffp_bcast_icmp_filter_install (void)
{
  int ret;

  HSL_FN_ENTER ();

  /* Install filter to trap linklocal multicast packets to the CPU. */
  ret = bcmx_filter_create (&_hsl_bcast_icmp_filter);
  if (ret < 0)
    {
      HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_ERROR,
               "Can't create bcast icmp filter\n");
      HSL_FN_EXIT (-1);
    }

  /* Qualify Broadcast frames. */
  ret = bcmx_filter_qualify_broadcast (_hsl_bcast_icmp_filter);
  if (ret < 0)
    {
      HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_ERROR,
               "Can't qualify bcast icmp broadcast filter\n");
      ret = -1;
      goto ERR;
    }

  /* Qualify IP packets. */
  ret = bcmx_filter_qualify_format (_hsl_bcast_icmp_filter, BCM_FILTER_PKTFMT_IPV4);
  if (ret < 0)
    {
      HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_ERROR,
               "Can't qualify bcast icmp IPv4 filter\n");
      ret = -1;
      goto ERR;
    }

  /* Qualify UDP protocl type in IP packet. */
  ret = bcmx_filter_qualify_data8 (_hsl_bcast_icmp_filter, IP_PROTO_OFFSET, 
                                    IP_PROTO_ICMP_TYPE, 0xff);
  if (ret < 0)
    {
      HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_ERROR,
               "Can't qualify bcast icmp ICMP protocol filter\n");
      ret = -1;
      goto ERR;
    }

  /* Note that we are not qualifying on ICMP type.
   */

  /* Define processing rules. */
  ret = bcmx_filter_action_match (_hsl_bcast_icmp_filter, bcmActionCopyToCpu, 0);
  if (ret < 0)
    {
      HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_ERROR,
               "Can't set bcast icmp filter Copy To CPU action\n");
      ret = -1;
      goto ERR;
    }

  ret = bcmx_filter_install (_hsl_bcast_icmp_filter);
  if (ret < 0)
    {
      HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_ERROR,
               "Can't install bcast icmp filter\n");
      ret = -1;
      goto ERR;
    }

  HSL_FN_EXIT (0);

 ERR:
  bcmx_filter_destroy (_hsl_bcast_icmp_filter);
  HSL_FN_EXIT (-1);
}

/*
  Uninstall FFP filter for bcast_icmp broadcast packets. 
*/
static int
_hsl_bcmx_ffp_bcast_icmp_filter_uninstall (void)
{
  HSL_FN_ENTER ();

  if (_hsl_bcast_icmp_filter != HSL_BCM_INVALID_FILTER)
    {
      bcmx_filter_remove (_hsl_bcast_icmp_filter);
      bcmx_filter_destroy (_hsl_bcast_icmp_filter);
      _hsl_bcast_icmp_filter = HSL_BCM_INVALID_FILTER;
    }

  HSL_FN_EXIT (0);
}

/* Unintall bcast icmp field entry */
static int
_hsl_bcmx_bcast_icmp_field_uninstall ()
{
  int ret = 0;

  HSL_FN_ENTER();

  if (_hsl_bcast_icmp_field_ent != HSL_BCM_INVALID_FIELD_ENTRY)
    {
      bcmx_field_entry_destroy (_hsl_bcast_icmp_field_ent);
      _hsl_bcast_icmp_field_ent = HSL_BCM_INVALID_FIELD_ENTRY;
    }

  if (_hsl_bcast_field_grp != HSL_BCM_INVALID_FIELD_GRP)
    {
      bcmx_field_group_destroy (_hsl_bcast_field_grp);
      _hsl_bcast_field_grp = HSL_BCM_INVALID_FIELD_GRP;
    }

  HSL_FN_EXIT (ret);
}

/* Install bcast icmp field entry */
static int
_hsl_bcmx_bcast_icmp_field_install ()
{
  int ret;
  bcm_field_qset_t qset;
  bcm_mac_t bcast_mac, bcast_mac_mask; 

  HSL_FN_ENTER();

  /* Initialize bcast mac & mask */
  memset (bcast_mac, 0xff, sizeof (bcm_mac_t));
  memset (bcast_mac_mask, 0xff, sizeof (bcm_mac_t));

  /* Initialize group if not done by rip filter */

  if (_hsl_bcast_field_grp == HSL_BCM_INVALID_FIELD_GRP)
    {
      /* Initialize Qualifier */
      BCM_FIELD_QSET_INIT(qset);

      /* L2 DST Mac Qualifier */
      BCM_FIELD_QSET_ADD (qset, bcmFieldQualifyDstMac);
      /* Packet Format Qualifier */
      BCM_FIELD_QSET_ADD (qset, bcmFieldQualifyPacketFormat);
      /* IP Protocol Qualifier */
      BCM_FIELD_QSET_ADD (qset, bcmFieldQualifyIpProtocol);

      ret = bcmx_field_group_create (qset, BCM_FIELD_GROUP_PRIO_ANY,
                                     &_hsl_bcast_field_grp);
      if (ret != BCM_E_NONE)
      {
          HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_ERROR,
                   "Can't create bcast field group\n");
          goto ERR;
      }
    }
  
  ret = bcmx_field_entry_create (_hsl_bcast_field_grp, 
                                 &_hsl_bcast_icmp_field_ent);
  if (ret != BCM_E_NONE)
   {
     HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_ERROR,
           "Can't create bcast icmp field entry\n");
     goto ERR;
   }

  /* Qualify L2 Dst Mac */
  ret = bcmx_field_qualify_DstMac (_hsl_bcast_icmp_field_ent, bcast_mac,
                                    bcast_mac_mask);
  if (ret != BCM_E_NONE)
    {
      HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_ERROR,
            "Can't create bcast icmp destination MAC qualifier\n");
      goto ERR;
    }

  /* Qualify Packet Format */
  ret = bcmx_field_qualify_PacketFormat (_hsl_bcast_icmp_field_ent, 
                                          BCM_FIELD_PKT_FMT_IPV4,
                                          0x1f);
  if (ret != BCM_E_NONE)
    {
      HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_ERROR,
            "Can't create bcast icmp IPv4 pkt format qualifier\n");
      goto ERR;
    }

  /* Qualify IP Protocol */
  ret = bcmx_field_qualify_IpProtocol (_hsl_bcast_icmp_field_ent, 
                                          IP_PROTO_ICMP_TYPE,
                                          0xff);
  if (ret != BCM_E_NONE)
    {
      HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_ERROR,
            "Can't create bcast icmp IPv4 ICMP protocol qualifier\n");
      goto ERR;
    }

  ret = bcmx_field_action_add (_hsl_bcast_icmp_field_ent, 
                               bcmFieldActionCopyToCpu, 0, 0);
  if (ret != BCM_E_NONE)
    {
      HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_ERROR,
            "Can't set bcast icmp field entry Copy To CPU action\n");
      goto ERR;
    }

  ret = bcmx_field_entry_install (_hsl_bcast_icmp_field_ent);
  if (ret != BCM_E_NONE)
    {
      HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_ERROR,
            "Can't install bcast icmp field entry\n");
      goto ERR;
    }

  HSL_FN_EXIT (0);

ERR:
  _hsl_bcmx_bcast_icmp_field_uninstall();
  HSL_FN_EXIT (ret);
}

/*
  Install bcast_icmp filter
*/
int
hsl_bcm_bcast_icmp_filter_install ()
{
  int ret = -1;
  int filter_type;

  HSL_FN_ENTER ();
  
  filter_type = hsl_bcm_filter_type_get ();
  if (filter_type == HSL_BCM_FEATURE_FIELD)
    {
      ret = _hsl_bcmx_bcast_icmp_field_install ();
    }
  else
    {
      ret = _hsl_bcmx_ffp_bcast_icmp_filter_install ();
    }

  HSL_FN_EXIT (ret);
}

/*
  Uninstall broadcast ICMP filter
*/
int
hsl_bcm_bcast_icmp_filter_uninstall ()
{
  int ret = -1;
  int filter_type;
  HSL_FN_ENTER ();

  filter_type = hsl_bcm_filter_type_get ();
  if (filter_type == HSL_BCM_FEATURE_FIELD)
    {
      ret = _hsl_bcmx_bcast_icmp_field_uninstall ();
    }
  else
    {
      ret = _hsl_bcmx_ffp_bcast_icmp_filter_uninstall ();
    }
  HSL_FN_EXIT (ret);
}
#endif /* HAVE_L3 */

/* Install timesync field entry */
/*djg */
static int
_hsl_bcmx_timesync_field_install (unsigned int index, unsigned int port)
{
  int ret;
  bcm_field_qset_t qset;
  bcm_mac_t timesync_mac, timesync_mask; 

  HSL_FN_ENTER();
  HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO,
                   "test: inside function timesync field install\n");

  /* Initialize timesync mac & mask */
  memset (timesync_mac, 0x01, 1);
  memset (timesync_mac+1, 0x1b, 1);
  memset (timesync_mac+2, 0x19, 1);
  memset (timesync_mac+3, 0x00, 1);
  memset (timesync_mac+4, 0x00, 1);
  memset (timesync_mac+5, 0x00, 1);
  //memset (timesync_mac, 0xff, sizeof (bcm_mac_t));
  memset (timesync_mask, 0xff, sizeof (bcm_mac_t));

  /* Initialize group if not done by timesync filter */

  if (_hsl_timesync_field_grp == HSL_BCM_INVALID_FIELD_GRP)
    {
      /* Initialize Qualifier */
      BCM_FIELD_QSET_INIT(qset);

      /* L2 DST Mac Qualifier */
      //BCM_FIELD_QSET_ADD (qset, bcmFieldQualifyDstMac);
      /* Ethernet Type Qualifier */
      BCM_FIELD_QSET_ADD (qset, bcmFieldQualifyEtherType);
	  /*In Port Qualifyer*/
	   BCM_FIELD_QSET_ADD (qset, bcmFieldQualifyInPort);
      /* IP Protocol Qualifier */
      //BCM_FIELD_QSET_ADD (qset, bcmFieldQualifyIpProtocol);

      ret = bcmx_field_group_create (qset, BCM_FIELD_GROUP_PRIO_ANY,
                                     &_hsl_timesync_field_grp);
      if (ret != BCM_E_NONE)
      {
          HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_ERROR,
                   "Can't create timesync field group\n");
          goto ERR;
      }
    }
  
  ret = bcmx_field_entry_create (_hsl_timesync_field_grp, 
                                 &_hsl_timesync_field_ent[index]);
  if (ret != BCM_E_NONE)
   {
     HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_ERROR,
           "Can't create timesync field entry\n");
     goto ERR;
   }

  /* Qualify L2 Dst Mac */
 /* ret = bcmx_field_qualify_DstMac (_hsl_timesync_field_ent[index], timesync_mac,
                                    timesync_mask);
  if (ret != BCM_E_NONE)
    {
      HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_ERROR,
            "Can't create timesync destination MAC qualifier\n");
      goto ERR;
    }
    HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO,
                   "test: mac quality init ok!!\n");*/

  /* Qualify Ethernet Type */
  ret = bcmx_field_qualify_EtherType (_hsl_timesync_field_ent[index], 
                                          0x88f7,
                                          0xffff);
  if (ret != BCM_E_NONE)
    {
      HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_ERROR,
            "Can't create timesync pkt format qualifier\n");
      goto ERR;
    }

  /* Qualify In Port *///bcm_esw_port_gport_get  bcm_field_qualify_InPort
  bcm_gport_t gport;
  bcm_port_gport_get(0,port+2,&gport);
      //HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO,"test: the bcm_port_t 17 is mapped to gport %d\n!!\n",(bcmx_lport_t )gport);
  ret = bcmx_field_qualify_InPort (_hsl_timesync_field_ent[index], 
                                          (bcmx_lport_t )gport);
    //ret = bcm_field_qualify_InPort (0,_hsl_timesync_field_ent, 
                                        // 0x00000011,0xFFFFFFFF);
  if (ret != BCM_E_NONE)
    {
      HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_ERROR,
            "Can't create timesync pkt format qualifier\n");
      goto ERR;
    }


  /* Qualify IP Protocol */
  /*ret = bcmx_field_qualify_IpProtocol (_hsl_bcast_icmp_field_ent, 
                                          IP_PROTO_ICMP_TYPE,
                                         /0xff);
  if (ret != BCM_E_NONE)
    {
      HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_ERROR,
            "Can't create timesync  protocol qualifier\n");
      goto ERR;
    }*/

  ret = bcmx_field_action_add (_hsl_timesync_field_ent[index], 
                              bcmFieldActionTimeStampToCpu, 0, 0);/*  bcmFieldActionCopyToCpu with timestamp*/
  if (ret != BCM_E_NONE)
    {
      HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_ERROR,
            "Can't set timesync field entry Copy To CPU action\n");
      goto ERR;
    }
      HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO,
                   "test: timesync action init ok!!\n");

  //ret = bcmx_field_action_add (_hsl_timesync_field_ent, 
                              //bcmFieldActionTimeStampToCpuCancel, 0, 0);/*  bcmFieldActionCopyToCpu with timestamp*/

  //bcm_port_gport_get(0,0x00000010,&gport);
  //ret = bcmx_field_action_add (_hsl_timesync_field_ent, 
                              //bcmFieldActionRedirectPbmp, 0x1C000,0);//(bcmx_lport_t )gport);bcmFieldActionRedirectPbmp
 
  
  ret = bcmx_field_entry_install (_hsl_timesync_field_ent[index]);
  if (ret != BCM_E_NONE)
    {
      HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_ERROR,
            "Can't install timesync field entry\n");
      goto ERR;
    }
   HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO,
                   "test: timesync entry install ok!!\n");

  HSL_FN_EXIT (0);

ERR:
  bcmx_field_entry_destroy (_hsl_timesync_field_ent[index]);
  HSL_FN_EXIT (ret);
}

/* Uninstall timesync field entry */
static int
_hsl_bcmx_timesync_field_uninstall (unsigned int index)
{
  int ret = 0;
  int i,j = 0;

  HSL_FN_ENTER();

  if (_hsl_timesync_field_ent[index] != HSL_BCM_INVALID_FIELD_ENTRY)
    {
      bcmx_field_entry_destroy (_hsl_timesync_field_ent[index]);
      _hsl_timesync_field_ent[index] = HSL_BCM_INVALID_FIELD_ENTRY;
    }
  for( i = 0; i<PTP_NUMBER_PORTS; i++)
  {
  	if (_hsl_timesync_field_ent[index] != HSL_BCM_INVALID_FIELD_ENTRY)
    {
      j ++;
    }
  }
  if ((_hsl_timesync_field_grp != HSL_BCM_INVALID_FIELD_GRP) && (j == 0))
    {
      bcmx_field_group_destroy (_hsl_timesync_field_grp);
      _hsl_timesync_field_grp = HSL_BCM_INVALID_FIELD_GRP;
    }

  HSL_FN_EXIT (ret);
}

/*
  Install timesync filter
*/
int
hsl_bcm_timesync_filter_install (unsigned int index, unsigned int port)
{
  int ret = -1;
  int filter_type;

  HSL_FN_ENTER ();
  
  filter_type = hsl_bcm_filter_type_get ();
  if (filter_type == HSL_BCM_FEATURE_FIELD)
    {
      ret = _hsl_bcmx_timesync_field_install (index,port);
    }
  else
    {
      //ret = _hsl_bcmx_timesync_filter_install ();
    }

  HSL_FN_EXIT (ret);
}

/*
  Uninstall timesync filter
*/
int
hsl_bcm_timesync_filter_uninstall (unsigned int index)
{
  int ret = -1;
  int filter_type;
  HSL_FN_ENTER ();

  filter_type = hsl_bcm_filter_type_get ();
  if (filter_type == HSL_BCM_FEATURE_FIELD)
    {
      ret = _hsl_bcmx_timesync_field_uninstall (index);
    }
  else
    {
      //ret = _hsl_bcmx_timesync_filter_uninstall ();
    }
  HSL_FN_EXIT (ret);
}



/* 
   Initialize chip for protocol packets. 
*/
static int
_hsl_bcmx_switch_init (void)
{
#ifdef HAVE_L3
  hsl_bcm_soc_type chip_type;
#endif /* HAVE_L3 */
  int ret;

  HSL_FN_ENTER ();

#ifdef HAVE_L3
  /* Trap unknown L3 packets to CPU. */
  bcmx_switch_control_set (bcmSwitchUnknownL3DestToCpu, 1);
  bcmx_switch_control_set (bcmSwitchL3HeaderErrToCpu, 1);
  bcmx_switch_control_set (bcmSwitchL3SlowpathToCpu, 1);

  /* ARP request/replies to CPU. */
  bcmx_switch_control_set (bcmSwitchArpRequestToCpu, 1);
  bcmx_switch_control_set (bcmSwitchArpReplyToCpu, 1);
  bcmx_switch_control_set (bcmSwitchCpuProtoArpPriority, 7);

  /* Ttl fail to CPU */
  bcmx_switch_control_set (bcmSwitchL3UcTtlErrToCpu, 1);

#ifdef HAVE_IPV6
  bcmx_switch_control_set (bcmSwitchNdPktToCpu, 1);
#endif /* HAVE_IPV6. */

#if defined(HAVE_MCAST_IPV4) || defined(HAVE_MCAST_IPV6)
  /* Trap IPMC packets with port miss to CPU. */
  bcmx_switch_control_set (bcmSwitchIpmcPortMissToCpu, 1);
  bcmx_switch_control_set (bcmSwitchUnknownIpmcToCpu, 1);
  bcmx_switch_control_set (bcmSwitchIpmcTtlErrToCpu, 1);
  bcmx_switch_control_set (bcmSwitchIpmcErrorToCpu, 1);

#endif /* HAVE_MCAST_IPV4 || HAVE_MCAST_IPV6 */

  /* Directed mirroring. */
  bcmx_switch_control_set (bcmSwitchDirectedMirroring, 1);

#ifdef HAVE_MCAST_IPV6
  /* Trap MLD packets to CPU. */
  bcmx_switch_control_set (bcmSwitchMldPktToCpu, 1);
#endif /* HAVE_MCAST_IPV6 */
  
  /* Trap packets with L3 header problems. */
  bcmx_switch_control_set (bcmSwitchL3HeaderErrToCpu, 1);

#if 0
  /* Denial of Service checks. */
  bcmx_switch_control_set (bcmSwitchDosAttackToCpu, 1);
  bcmx_switch_control_set (bcmSwitchDosAttackSipEqualDip, 1);
  bcmx_switch_control_set (bcmSwitchDosAttackMinTcpHdrSize, 1);
  bcmx_switch_control_set (bcmSwitchDosAttackV4FirstFrag, 1);
  bcmx_switch_control_set (bcmSwitchDosAttackTcpFlags, 1);
  bcmx_switch_control_set (bcmSwitchDosAttackL4Port, 1);
  bcmx_switch_control_set (bcmSwitchDosAttackTcpFrag, 1);
  bcmx_switch_control_set (bcmSwitchDosAttackIcmp, 1);
  bcmx_switch_control_set (bcmSwitchDosAttackIcmpPktOversize, 1);
#ifdef HAVE_IPV6
  bcmx_switch_control_set (bcmSwitchDosAttackIcmpV6PingSize, 1);
#endif /* HAVE_IPV6. */
#endif

  /* V4, V6 reserved multicast to CPU. */
  bcmx_switch_control_set (bcmSwitchV4ResvdMcPktToCpu, 1);
#ifdef HAVE_IPV6
  bcmx_switch_control_set (bcmSwitchV6ResvdMcPktToCpu, 1);
#endif /* HAVE_IPV6. */

  /* MTU failure to CPU. */
  bcmx_switch_control_set (bcmSwitchL3MtuFailToCpu, 1);

#endif /* HAVE_L3 */

#if  ((defined(HAVE_MCAST_IPV4) && defined(HAVE_L3)) || \
      (defined(HAVE_IGMP_SNOOP) && defined(HAVE_L2)))
  /* Trap IGMP packets to CPU. */
  bcmx_switch_control_set (bcmSwitchIgmpPktToCpu, 1);
#endif /* HAVE_MCAST_IPV4  || HAVE_IGMP_SNOOP  */
  bcmx_switch_control_set (bcmSwitchCpuProtocolPrio, 7);
  /* Enable filtering */ 
  ret = _hsl_bcm_filter_init();
  if (ret < 0)
    {
       HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_FATAL, "Error initialize filtering\n");
       HSL_FN_EXIT (-1);
    }
#ifdef HAVE_L3
  /* Get chipset type. */
  chip_type =  hsl_hw_get_soc_type ();
  if (chip_type == HSL_BCM_SOC_TYPE_XGS2) 
    {
      /* Install linklocal multicast. */
      ret = _hsl_bcmx_ffp_llmc_filter_install ();
      if (ret < 0)
        HSL_FN_EXIT (-1);

      ret = _hsl_bcmx_ffp_arp_filter_install ();
      if (ret < 0)
        {
          _hsl_bcmx_ffp_llmc_filter_uninstall ();
          HSL_FN_EXIT (-1);
        }
    }
#ifdef HAVE_RIPD
  ret = hsl_bcm_ripv1_filter_install ();
  if (ret < 0)
    HSL_FN_EXIT (-1);
#endif /* HAVE_RIPD */

  ret = hsl_bcm_bcast_icmp_filter_install ();
  if (ret < 0)
    HSL_FN_EXIT (-1);

#endif /* HAVE_L3 */

#ifdef HAVE_ONMD
  ret = hsl_bcm_cfm_filter_install ();
  if (ret < 0)
    HSL_FN_EXIT (-1);
#endif /* HAVE_ONMD */

  HSL_FN_EXIT (0);
}


static void
_hsl_attach_ports (int unit)
{
#if 1 /* enabled for stacking */
  bcmx_lport_t lport;
  bcm_port_t   port;
  uint32       flags;
  int          *val, value;
  
  BCMX_FOREACH_LPORT(lport) {
      /* check if unit is the same */
      if (BCMX_LPORT_BCM_UNIT(lport) == unit)
      {
         /* Add port now */
         flags = BCMX_LPORT_FLAGS(lport);
         port = (bcm_port_t) BCMX_LPORT_BCM_PORT(lport);
         val = &value;
         if ((val = (int *)_hsl_bcm_port_attach_cb (lport, unit, port, flags)) 
               != NULL)
         {
             bcmx_uport_set(lport, (unsigned int *)val); 
         }
         else
         {
             bcmx_uport_set(lport, BCMX_UPORT_CAST(lport)); 
         }
         HSL_LOG(HSL_LOG_PLATFORM, HSL_LEVEL_INFO, "bcm_lport=%d (bcm_port_t port = %d) in this unit.\n", lport, port );
     }
     else {
        HSL_LOG(HSL_LOG_PLATFORM, HSL_LEVEL_INFO, "bcm_lport=%d NOT in the unit.\n", lport);
      }
  }
#endif
}

static void
_bcm_unit_cpu_ratelimit_set (int unit)
{
  int ret;
  bcmx_lport_t lport;
  u_int32_t kbps = 1200;
  u_int32_t kb_burst = 1000;

  ret = bcmx_lport_local_cpu_get(unit,&lport);
  if (ret == BCM_E_NOT_FOUND)
    {
      HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_ERROR, "Can't  get CPU port for ret = %d\n", ret);
    }

  ret = bcmx_port_rate_egress_set (lport, kbps, kb_burst);
  if (ret < 0)
    {
      HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_ERROR, "Can't set egress rate for CPU port ret = %d\n", ret);
    }
  ret = bcmx_port_rate_ingress_set (lport, kbps, kb_burst);
  if (ret < 0)
    {
      HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_ERROR, "Can't set ingress rate for CPU port ret = %d\n", ret);
    }
      
  HSL_FN_EXIT (ret);
}

/* 
   Initialize units. 
*/
static int
_hsl_bcmx_unit_init (void)
{
  int max_unit;
  int bcm_unit=0;
  int count;
  int first;
  int ret;
  int i;

#if 0
  char master_dev[6] = {'\0'} , local_dev[6] = {'\0'}, cmp = {'\0'};

  
  /* Only if local device is the master device then attach else continue */
  hsl_bcm_get_master_cpu (master_dev);
  hsl_bcm_get_local_cpu (local_dev);
           
  if (memcmp (master_dev, local_dev, 6))
       return -33;

#endif

  first = 0;
  /* Find max units attached. */
  bcm_attach_max (&max_unit);
  ct_tx_tunnel_setup (); 

  /* Attach units to BCMX. 
     NOTE: No hybrid chip configurations supported at this time. */
  for (bcm_unit = 0; bcm_unit <= max_unit; bcm_unit++)
    {
      if (! first && (SOC_IS_XGS_SWITCH(bcm_unit)))
        {
          if (SOC_IS_XGS3_SWITCH(bcm_unit))
            {
              hsl_hw_set_soc_type (HSL_BCM_SOC_TYPE_XGS3);
              HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO, "bcm_soc_type is XGS3!\n");
              if (SOC_IS_EASYRIDER(bcm_unit))
                hsl_hw_set_chip_family (HSL_BCM_CHIP_FAMILY_EASYRIDER);
              if (SOC_IS_TRIUMPH(bcm_unit))
                hsl_hw_set_chip_family (HSL_BCM_CHIP_FAMILY_TRIUMPH);
              if (SOC_IS_ENDURO(bcm_unit)) /* 56334 */
              {
                 hsl_hw_set_chip_family (HSL_BCM_CHIP_FAMILY_TRIUMPH);
                 HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO, "bcm_chip_family is ENDURO, but we set it as TRIUMPH!\n");
              }
            }
          else
            {
              HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO, "bcm_soc_type is XGS2!\n");
              hsl_hw_set_soc_type (HSL_BCM_SOC_TYPE_XGS2);
            }

          first = 1;
        }

      if (bcm_attach_check (bcm_unit) < 0)
        continue;
      ret = bcmx_device_attach (bcm_unit);
      HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO, "bcmx_device_attach (bcm_unit=%d), ret=%d\n", bcm_unit, ret);

      if (ret < 0)
        {
          if (ret == BCM_E_EXISTS)
            {
               /* Get all ports from the BCMX layer for that unit and register */
               _hsl_attach_ports (bcm_unit);
                continue;
            }
          else
            {
              HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_FATAL, "Error attaching unit %d to BCMX layer\n", bcm_unit);
              return -1;
            }
        }

      _hsl_attach_ports (bcm_unit);
      _bcm_unit_cpu_ratelimit_set (bcm_unit);

    }

  /* Initialize statistics module. */
  ret =  bcmx_stat_init();
  if (ret < 0)
    {
      HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_FATAL, "Error %d to initialize statistics module\n", ret);
    }

    
  /* Set default ageing timer value */
  ret = bcmx_l2_age_timer_set(l2_age_timeout);
  if (ret != BCM_E_NONE)
    HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_ERROR, 
             "Error setting default ageing timer value (%d) to BCMX layer\n", l2_age_timeout);

  /* Set default VLAN. */
  bcmx_vlan_default_set (HSL_DEFAULT_VID);

  count = bcmx_l2_bpdu_count ();
  if (count < 0)
    return 0;

  /* Set switch pkt control. */
  ret = _hsl_bcmx_switch_init ();  
  if (ret < 0)
    HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_ERROR,
             "Error initializing switch units\n");
  
  i = 0;
  if (i < count)
    bcmx_l2_bpdu_set (i, bpdu_addr);
  else
    return 0;
  i++;
  if (i < count)
    bcmx_l2_bpdu_set (i, gmrp_addr);
  else
    return 0;
  i++;
  if (i < count)
    bcmx_l2_bpdu_set (i, gvrp_addr);

  if (++i < count)
    bcmx_l2_bpdu_set (i, lacp_addr);

  if (++i < count)
    bcmx_l2_bpdu_set (i, eapol_addr);

  if (++i < count)
    bcmx_l2_bpdu_set (i, pro_bpdu_addr);

#ifdef HAVE_L3
  /* Enable IPMC for now. */
  bcmx_ipmc_enable (1);
#else
  /* Disable IPMC for now. */
  bcmx_ipmc_enable (0);
  bcmx_ipmc_detach(); 
#endif /* HAVE_L3*/

  return 0;
}

/*
  Deinitialize units. 
*/
static int
_hsl_bcmx_unit_deinit (void)
{
  int ret;
  int max_unit;
  int bcm_unit;

#ifdef HAVE_L3
  /* Uninstall llmc filter. */
  _hsl_bcmx_ffp_llmc_filter_uninstall ();
  
  /* Uninstall arp filter. */
  _hsl_bcmx_ffp_arp_filter_uninstall ();

#ifdef HAVE_RIPD
  /* Uninstall RIPv1 filter. */
  hsl_bcm_ripv1_filter_uninstall ();
#endif /* HAVE_RIPD */

  /* Uninstall bcast ICMP filter. */
  hsl_bcm_bcast_icmp_filter_uninstall ();
#endif /* HAVE_L3 */

#ifdef HAVE_ONMD
  /* Uninstall CFM filter */
  hsl_bcm_cfm_filter_uninstall ();
#endif /* HAVE_ONMD */

  /* Disable filtering. */
  _hsl_bcm_filter_deinit();

  bcm_attach_max (&max_unit);

  /* Attach units to BCMX. */

  if (max_unit >= 0)
    {
      for (bcm_unit = 0; bcm_unit <= max_unit && bcm_unit < BCM_CONTROL_MAX;
           bcm_unit++)
        {
          if (bcm_attach_check (bcm_unit) < 0)
            continue;

          ret = bcmx_device_detach (bcm_unit);
          if (ret < 0)
            {
              HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_FATAL, "Error detaching unit"
                       " %d to BCMX layer\n", bcm_unit);
              return -1;
            }
        }
    }

  ct_tx_tunnel_stop ();

  /* Find max units attached. */
  bcmx_ipmc_enable (0);
  bcmx_ipmc_detach ();

  return 0;
}

/*
  Initialize BCMX layer.
*/
int
hsl_bcmx_init (void)
{
  int ret;

  HSL_FN_ENTER ();

  /* Initialize units. */
  ret = _hsl_bcmx_unit_init ();

  if (ret == -33)
    {
      hsl_bcm_pkt_init ();
      HSL_FN_EXIT (0);
    }

  /* If the CPU is rebooted but the ASIC is intact this will happen. So instead 
     just get information from the CPU */
  if (ret < 0)
    {
      hsl_bcm_pkt_init ();
      HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_FATAL, "Error attaching units to BCMX. Exiting!!!\n");
      goto ERR;
    }

  /* Initialize Packet driver, BCMX Tx/Rx. */
  ret = hsl_bcm_pkt_init ();
  if (ret < 0)
    {
      HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_FATAL, "Error initializing packet driver. \n");
      goto ERR;
    }

  HSL_FN_EXIT (0);

 ERR:
#if 0
  /* Deinitialize. */
  hsl_bcmx_deinit ();
#endif
  HSL_FN_EXIT (-1);
}

/*
  Deinitialize BCMX layer.
*/
int
hsl_bcmx_deinit (void)
{
  int ret;

  /* Deinitialize units. */
  ret = _hsl_bcmx_unit_deinit ();
  if (ret < 0)
    {
      HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_FATAL, "Error deinitializing BCMX units\n");
    }

  /* Deinitialize packet driver. */
  ret = hsl_bcm_pkt_deinit ();
  if (ret < 0)
    {
      HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_FATAL, "Error deinitializing packet driver\n");
    }

  return 0;
}

/* 
  Attach/detach callback (tailored for stack task)
*/
void
hsl_attach_callback (int unit, int attach, cpudb_entry_t *cpuent, 
                     int cpuunit)
{
  HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO, 
           "HSL: Attach callback, unit %d. %s\n", unit,
           attach ? "attach" : "detach");

  if (attach)
    (void) bcmx_device_attach (unit);
  else
    (void) bcmx_device_detach (unit);
}

/*
  Initialize Broadcom specific data.
*/
int
hsl_hw_init (void)
{
  char *str;
  int max_fe, max_ge, max_xe;
  static int hsl_bcmx_registered = FALSE;
  int rv;

#if 0
  HSL_FN_ENTER ();
  {
      unsigned char mac[6] = { 0x02, 0x02, 0x03, 0x04, 0x05, 0x05 };
      hsl_bcm_port_list  port_struct;

      port_struct.num_ports = 1;
      port_struct.unit[0] = 0;
      port_struct.port[0] = 25;
      hsl_bcm_start_stacking (mac, 0, 28, &port_struct, 0, 0, -1, 1, 0, -1, 2,
          -1, -1);

  }

  taskDelay(150);
#endif

  HSL_FN_ENTER ();

  if (!hsl_bcmx_registered)
  {
     if ((rv = bcm_stack_attach_register(hsl_attach_callback)) < 0) { 
          HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_ERROR,
              "Could not register BCMX with stack attach: %s\n", bcm_errmsg(rv));
    } else {
        hsl_bcmx_registered = TRUE;
    }
  }

  /* Get max FE ports. */
  str = (char *) sal_config_get ("max_fe_ports");
  if (! str)
    {
      max_fe = BCM_CONFIG_MAX_FE;
    }
  else
    {
      max_fe = hsl_atoi (str);
    }

  /* Get max GE ports. */
  str = (char *) sal_config_get ("max_ge_ports");
  if (! str)
    {
      max_ge = BCM_CONFIG_MAX_GE;
    }
  else
    {
      max_ge = hsl_atoi (str);
    }

  /* Get max XE ports. */
  str = (char *) sal_config_get ("max_xe_ports");
  if (! str)
    {
      max_xe = BCM_CONFIG_MAX_XE;
    }
  else
    {
      max_xe = hsl_atoi (str);
    }

  /* Get base mac address. */
  str = (char *) sal_config_get ("station_mac_base");
  if (str)
    {
      hsl_etherStrToAddr (str, (char*)base_mac);
    }
  else
    {
      /* Just generate a random number for the 2nd, 3rd and 4th octet. */
      base_mac[1] = oss_rand () % 255;
      base_mac[2] = oss_rand () % 255;
      base_mac[3] = oss_rand () % 255;
    }

  /* Initialize interface map. */
  hsl_bcm_ifmap_init (max_fe, max_ge, max_xe, base_mac);

  /* Initialize interface handler. */
  hsl_bcm_ifhdlr_init ();

  /* Reserved VLAN database init. */
  hsl_bcm_resv_vlan_db_init (HSL_MVL_RESV_VLAN_FIRST_DEFAULT, HSL_MVL_RESV_VLAN_NUM_DEFAULT);

  /* Initialize BCMX layer. */
  hsl_bcmx_init ();


  HSL_FN_EXIT (0);
}

/*
  Deinitialize Broadcom specific data.
*/
int
hsl_hw_deinit (void)
{

  /* Deinitialize interface handler. */
  hsl_bcm_ifhdlr_deinit ();

  /* Deinitialize reserved VLAN database. */
  hsl_bcm_resv_vlan_db_deinit ();

  bcm_stack_attach_unregister (hsl_attach_callback);

  /* Deinitialize BCMX layer. */
  hsl_bcmx_deinit ();

  /* Deinitialize interface map. */
  hsl_bcm_ifmap_deinit ();

  return 0;
}

/*
  Initialize the MAC base address.
*/

int hsl_soft_init(void)
{
    int rc = 0;

    rc = hsl_bcm_vpn_init();
   //rc = hsl_bcm_qos_init();
     //hsl_bcm_port_init();

    return rc;
}



int
hsl_hw_mac_base_set (bcm_mac_t mac)
{
  memcpy (base_mac, mac, HSL_ETHER_ALEN);

  return 0;
}

/*
  Set SOC type.
*/
void
hsl_hw_set_soc_type (hsl_bcm_soc_type type)
{
  _bcm_soc_type = type;   
}

/* 
   Get SOC type. 
*/
hsl_bcm_soc_type
hsl_hw_get_soc_type (void)
{
  return _bcm_soc_type;
}


void
hsl_hw_set_chip_family (hsl_bcm_chip_family_t family)
{
  _bcm_chip_family = family;
}

hsl_bcm_chip_family_t
hsl_hw_get_chip_family (void)
{
  return _bcm_chip_family;
}

int
hsl_bcm_set_master_cpu (unsigned char *key)
{
  int         ret;
  cpudb_key_t cpKey; 
 
  HSL_FN_ENTER ();
  memcpy(&cpKey.key, key, 6);

  /* Should we call the functions directly or should we wrote API's for the 
     same. We may have to write our own API for the same */
  ret = cpudb_master_set (bcm_st_disc_db, cpKey);

  if (ret == BCM_E_NONE)
  {
      HSL_FN_EXIT (0);
  }
  else
  {
    HSL_FN_EXIT (-1);
  }
}

int
hsl_bcm_get_num_cpu (u_int32_t *num)
{
  HSL_FN_ENTER ();

  /* Check if num is being returned properly  */
  *num = 0;

  if (bcm_st_disc_db)
      *num += cpudb_entry_count_get (bcm_st_disc_db);
   
  if (bcm_st_cur_db) 
      *num += cpudb_entry_count_get (bcm_st_cur_db);

  if (*num != 0)
  {
      HSL_FN_EXIT (0);
  }
  else
  {
    HSL_FN_EXIT (-1);
  }
}

int 
hsl_bcm_get_master_cpu (char *buf)
{
  if (bcm_st_cur_db && cpudb_valid (bcm_st_cur_db))
  {
      if (bcm_st_cur_db->master_entry)
      {
          /* Non-NULL Master Entry */
          memcpy(buf, (void *)&bcm_st_cur_db->master_entry->base.key, 
              HSL_ETHER_ALEN);
      }
      return 0;
  }
  else
  {

      /* look in the discovery DB */
      if (bcm_st_cur_db && cpudb_valid (bcm_st_cur_db))
      {
          if (bcm_st_cur_db->master_entry)
          {
              /* Non-NULL Master Entry */
              memcpy(buf, (void *)&bcm_st_cur_db->master_entry->base.key,
                  HSL_ETHER_ALEN);
          }
      }
      return 0;
  } 
  return -1;
}

int 
hsl_bcm_get_local_cpu (char *buf)
{
  if (bcm_st_cur_db && cpudb_valid (bcm_st_cur_db))
  {
      if (bcm_st_cur_db->local_entry)
      {
          /* Non-NULL Local Entry */
          memcpy(buf, (void *)&bcm_st_cur_db->local_entry->base.key, 
              HSL_ETHER_ALEN);
      }
      return 0;
  }
  else
  {

      /* look in the discovery DB */
      if (bcm_st_cur_db && cpudb_valid (bcm_st_cur_db))
      {
          if (bcm_st_cur_db->local_entry)
          {
              /* Non-NULL Master Entry */
              memcpy(buf, (void *)&bcm_st_cur_db->local_entry->base.key,
                  HSL_ETHER_ALEN);
          }
      }
      return 0;
  } 
  return -1;
}

int 
hsl_bcm_get_cpu_index (unsigned int num, char *buf)
{
  cpudb_entry_t *entry = NULL;
  cpudb_base_t *base;
  int           ctr = 0;

  HSL_FN_ENTER ();

  /* Check if num is being returned properly  */

  if (bcm_st_disc_db && cpudb_valid (bcm_st_disc_db))
  {
      CPUDB_FOREACH_ENTRY(bcm_st_disc_db, entry) 
      {
          base = &entry->base;
          ctr++;
          if (ctr < num)
             continue;

          if (ctr == num) 
          {
              memcpy(buf, base->mac, HSL_ETHER_ALEN);
              return 0;  /* Found it */
          }
      }
  }

  if (bcm_st_cur_db && cpudb_valid (bcm_st_cur_db))
  {
      CPUDB_FOREACH_ENTRY(bcm_st_cur_db, entry) 
      {
          base = &entry->base;
          ctr++;
          if (ctr < num)
              continue;

          if (ctr == num) 
          {
              memcpy(buf, base->mac, HSL_ETHER_ALEN);
              return 0;  /* Found it */
          }
      }
  }

  if (entry)
  {
      HSL_FN_EXIT (0);
  }
  else
  {
    HSL_FN_EXIT (-1);
  }
}

struct hsl_stk_port_info
{
  int unit;
  int port;
  int flags;
  int weight;
};

struct hsl_cpu_dump_entry
{
  char mac_addr [6];
  char pack_1[2]; /* Packing bytes */
  int  num_units;
  int  master_prio;
  int  slot_id;

  /* Stack port info */
  int num_stk_ports;
  struct hsl_stk_port_info stk[5]; /* Allow a maximum of 5 stack ports */
};


int 
hsl_bcm_get_dump_cpu_index (unsigned int num, char *buf)
{
  cpudb_entry_t *entry = NULL;
  cpudb_base_t *base;
  struct hsl_cpu_dump_entry *cpu_entry, info;
  int           ctr = 0, ii;

  HSL_FN_ENTER ();

  /* Check if num is being returned properly  */
  cpu_entry = &info;
  memset (cpu_entry, '\0', sizeof(struct hsl_cpu_dump_entry));
  if (bcm_st_disc_db && cpudb_valid (bcm_st_disc_db))
  {
      CPUDB_FOREACH_ENTRY(bcm_st_disc_db, entry) 
      {
          base = &entry->base;
          ctr++;
          if (ctr < num)
             continue;

          if (ctr == num) 
          {
              memcpy (&cpu_entry->mac_addr, base->mac, HSL_ETHER_ALEN);
              cpu_entry->num_units = base->num_units;
              cpu_entry->master_prio = base->master_pri;
              cpu_entry->slot_id = base->slot_id;
              cpu_entry->num_stk_ports = base->num_stk_ports;
              for (ii = 0; ii < base->num_stk_ports; ii++)
              {
                  /* Copy the information */
                  cpu_entry->stk[ii].unit = base->stk_ports[ii].unit;
                  cpu_entry->stk[ii].port = base->stk_ports[ii].port;
                  cpu_entry->stk[ii].weight = base->stk_ports[ii].weight;

                  /* Copy flags */
                  cpu_entry->stk[ii].flags = entry->sp_info[ii].flags;
              }
              memcpy (buf, cpu_entry, sizeof(struct hsl_cpu_dump_entry));
              return 0;  /* Found it */
          }
      }
  }

  if (bcm_st_cur_db && cpudb_valid (bcm_st_cur_db))
  {
      CPUDB_FOREACH_ENTRY(bcm_st_cur_db, entry) 
      {
          base = &entry->base;
          ctr++;
          if (ctr < num)
              continue;

          if (ctr == num) 
          {
              memcpy (&cpu_entry->mac_addr, base->mac, HSL_ETHER_ALEN);
              cpu_entry->num_units = base->num_units;
              cpu_entry->master_prio = base->master_pri;
              cpu_entry->slot_id = base->slot_id;
              cpu_entry->num_stk_ports = base->num_stk_ports;
              for (ii = 0; ii < base->num_stk_ports; ii++)
              {
                  /* Copy the information */
                  cpu_entry->stk[ii].unit = base->stk_ports[ii].unit;
                  cpu_entry->stk[ii].port = base->stk_ports[ii].port;
                  cpu_entry->stk[ii].weight = base->stk_ports[ii].weight;

                  /* Copy flags */
                  cpu_entry->stk[ii].flags = entry->sp_info[ii].flags;
              }
              memcpy (buf, cpu_entry, sizeof(struct hsl_cpu_dump_entry));
              return 0;  /* Found it */
          }
      }
  }

  if (entry)
  {
      HSL_FN_EXIT (0);
  }
  else
  {
    HSL_FN_EXIT (-1);
  }
}


#ifdef HAVE_L3
int
_hsl_bcm_ra_filter_install ()
{
  int ret;

  ret = bcmx_filter_create (&_hsl_ra_filter);
  if (ret < 0)
    {
      HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_ERROR,
               "Can't create router alert filter\n");
      HSL_FN_EXIT (-1);
    }

  /* Setup mask and option header for router alerts */
  bcmx_filter_qualify_data32 (_hsl_ra_filter,
                              IP_OPTIONS_OFFSET,
                              (ROUTER_ALERT1 << 24 |
                               ROUTER_ALERT2 << 16 |
                               ROUTER_ALERT3 << 8  |
                               ROUTER_ALERT4), 
                              0xffffffff);
  if (ret < 0)
    {
      HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_ERROR,
               "Can't qualify router alert filter\n");
      ret = -1;
      goto ERR;
    }

  /* Define processing rules. */
  ret = bcmx_filter_action_match (_hsl_ra_filter, bcmActionCopyToCpu, 0);
  if (ret < 0)
    {
      HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_ERROR,
               "Can't qualify router alert filter\n");
      ret = -1;
      goto ERR;
    }

  ret = bcmx_filter_action_match (_hsl_ra_filter, bcmActionDoNotSwitch, 0);
  if (ret < 0)
    {
      HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_ERROR,
               "Can't qualify router alert filter\n");
      ret = -1;
      goto ERR;
    }

  ret = bcmx_filter_install (_hsl_ra_filter);
  if (ret < 0)
    {
      HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_ERROR,
               "Can't install router alert filter\n");
      ret = -1;
      goto ERR;
    }

  HSL_FN_EXIT (0);

 ERR:
  bcmx_filter_destroy (_hsl_ra_filter);
  _hsl_ra_filter = HSL_BCM_INVALID_FILTER;
  HSL_FN_EXIT (-1);
}


int
hsl_bcm_udf_ra_create (u_int32_t *udf_id)
{
  bcm_field_udf_spec_t udfspec;
  int ret;
  
  memset(&udfspec, 0, sizeof(bcm_field_udf_spec_t));
  ret = bcmx_field_udf_spec_set (&udfspec, BCM_FIELD_USER_L2_ETHERNET2|
                                 BCM_FIELD_USER_VLAN_NOTAG|BCM_FIELD_USER_IP4_HDR_ONLY,
                                 IP_OPTIONS_OFFSET/4);
  if (ret < 0)
  {
    return -1;
  }

  ret = bcmx_field_udf_create (&udfspec, udf_id);
  if (ret < 0)
  {
    return -1;
  }

  return 0;
}


int
_hsl_bcm_ra_field_install ()
{
  int ret;
  u_int32_t udf_id;
  bcm_field_qset_t qset;
  
  bcm_field_group_t group;
  bcm_field_entry_t entry;
  u_char ra_opt[BCM_FIELD_USER_FIELD_SIZE] = {0x00, 0x00, 0x94, 0x04};
  u_char ra_opt_mask[BCM_FIELD_USER_FIELD_SIZE] = {0x00, 0x00, 0xff, 0xff};
#if 0
  bcmx_lplist_t plist;
#endif /* 0 */
  if (_hsl_ra_udf_id == 0)
    {
      udf_id = 1;
      ret = hsl_bcm_udf_ra_create (&udf_id);
      if (ret < 0)
        return -1;
      
      _hsl_ra_udf_id = udf_id;
    }

  BCM_FIELD_QSET_INIT(qset);

  ret = bcmx_field_qset_add_udf (&qset, _hsl_ra_udf_id);
  if (ret < 0)
    {
      return -1;
    } 
  
  ret = bcmx_field_group_create (qset, BCM_FIELD_GROUP_PRIO_ANY,
                                 &group);
  if (ret != BCM_E_NONE)
    return ret;
  
  BCM_FIELD_QSET_ADD (qset, bcmFieldQualifyInPorts);
  
  ret = bcmx_field_group_set (group, qset);
  if (ret < 0)
    {
      bcmx_field_group_destroy (group);
      return -1;
    }
  
  ret = bcmx_field_entry_create (group, &entry);
  if (ret < 0)
    {
      bcmx_field_group_destroy (group);
      return -1;
    }

  ret = bcmx_field_qualify_UserDefined (entry, _hsl_ra_udf_id, 
                                        ra_opt, ra_opt_mask);
  if (ret < 0)
    {
      bcmx_field_group_destroy (group);
      bcmx_field_entry_remove (entry);
      bcmx_field_entry_destroy (entry);
      return -1;
    }

#if 0
  plist.lp_ports = NULL;

  ret = bcmx_lplist_init (&plist, 0, 0);
  if (ret != BCM_E_NONE)
    {
      bcmx_field_group_destroy (group);
      bcmx_field_entry_remove (entry);
      bcmx_field_entry_destroy (entry);
      return -1;
    }


  l2_ifp = hsl_ifmgr_get_first_L2_port (ifp);
  if (! l2_ifp)
    {
      bcmx_field_group_destroy (group);
      bcmx_field_entry_remove (entry);
      bcmx_field_entry_destroy (entry);
      return -1;
    }

  bcmif = l2_ifp->system_info;
  HSL_IFMGR_IF_REF_DEC (l2_ifp);

  ret = bcmx_lplist_add (&plist, bcmif->u.l2.lport);
  if (ret != BCM_E_NONE)
    {
      bcmx_field_group_destroy (group);
      bcmx_field_entry_remove (entry);
      bcmx_field_entry_destroy (entry);
      return -1;
    }

  ret = bcmx_field_qualify_InPorts (entry, plist);
  if (ret != BCM_E_NONE)
    {
      bcmx_field_group_destroy (group);
      bcmx_field_entry_remove (entry);
      bcmx_field_entry_destroy (entry);
      bcmx_lplist_free (&plist);
      return -1;
    }

#endif
  
  ret = bcmx_field_action_add (entry, bcmFieldActionCopyToCpu, 0, 0);
  if (ret != BCM_E_NONE)
    {
      bcmx_field_group_destroy (group);
      bcmx_field_entry_remove (entry);
      bcmx_field_entry_destroy (entry);
      return -1;
    }

  ret = bcmx_field_entry_install (entry);
  if (ret != BCM_E_NONE)
    {
      bcmx_field_group_destroy (group);
      bcmx_field_entry_remove (entry);
      bcmx_field_entry_destroy (entry);
      return -1;
    }

  return 0;
}


/*
  Install router alert filter
*/
int
hsl_bcm_router_alert_filter_install ()
{
  int ret = -1;
  int filter_type;

  HSL_FN_ENTER ();
  
  filter_type = hsl_bcm_filter_type_get ();
  if (filter_type == HSL_BCM_FEATURE_FIELD)
    {
      ret = _hsl_bcm_ra_field_install ();
    }
  else
    {
      ret = _hsl_bcm_ra_filter_install ();
    }

  return ret;
}


/*
  Uninstall router alert filter
*/
int
hsl_bcm_router_alert_filter_uninstall ()
{

  HSL_FN_ENTER ();
  if ((_hsl_ra_filter != 0) && 
      (_hsl_ra_filter != HSL_BCM_INVALID_FILTER))
    {
      bcmx_filter_remove (_hsl_ra_filter);
      bcmx_filter_destroy (_hsl_ra_filter);
      _hsl_ra_filter = HSL_BCM_INVALID_FILTER;
    }

  HSL_FN_EXIT (0);

}
#endif /* HAVE_L3 */


