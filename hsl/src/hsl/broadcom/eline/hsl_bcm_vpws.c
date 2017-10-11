#if 0

#include "config.h"
//#include "hsl_oss.h"
#include "hsl_os.h"
#include "hsl_types.h"
#include "hsl_logs.h"

#include "bcm_incl.h"
#include <soc/debug.h>
#include <bcm/error.h>
#include <bcm/debug.h>
#include "hsl_rsrc.h"
#include "hsl_linklist.h"
#include "hsl_bcm_vpws.h"
#include "hsl_bcm_vpls.h"



#define HSL_VPWS_ROE(op, arg)     \
    do {               \
        int __rv__;    \
        __rv__ = op arg;    \
        if (BCM_FAILURE(__rv__)) { \
	        HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO, \
			"HSL test VPWS: Error: " #op " failed, %s\n", \
                           bcm_errmsg(__rv__)); \
            return -1; \
        } \
    } while(0)
 
typedef void *vpws_sem_t;

//static struct hsl_bcm_tunnel_list *hsl_tunnel_list = NULL;
//sal_mutex_t hsl_tunnel_list_mutex; 

//ret = oss_sem_new ("hsl_vpws_list_sem", OSS_SEM_MUTEX, 0, NULL, &hsl_vpws_list_sem);
//oss_sem_delete (OSS_SEM_BINARY, hsl_vpws_list_sem );
#define VPWS_LOCK() \
    oss_sem_lock (OSS_SEM_BINARY, hsl_vpws_list_sem, OSS_WAIT_FOREVER);
#define VPWS_UNLOCK() \
    oss_sem_unlock (OSS_SEM_BINARY, hsl_vpws_list_sem );

#define VPWS_IS_INIT()                                             \
    if (hsl_vpws_list == NULL) {                                   \
        printk("Error: hsl_vpws_list not initialized\n");            \
        return -1;                                        \
    }

static struct hsl_bcm_vpws_list *hsl_vpws_list = NULL;
vpws_sem_t hsl_vpws_list_sem; 

bcm_mac_t  dmac_address_0 = {0x00, 0x4e, 0xa5, 0x00, 0x10, 0x01};
bcm_mac_t  smac_address_0 = {0x00, 0x4e, 0xa5, 0x00, 0x10, 0x00};
bcm_mac_t  dmac_address_1 = {0x00, 0x4e, 0xa5, 0x00, 0x40, 0x01};
bcm_mac_t  smac_address_1 = {0x00, 0x4e, 0xa5, 0x00, 0x40, 0x00};

extern TRsrMangrInfo_t g_bcm_vpn_id_pool;


int 
hsl_vpws_init(void)
{
	HSL_FN_ENTER();

	bcm_mpls_init(0);

    HSL_FN_EXIT( 0 );
}

/*
 * Function:
 *
 * Purpose:
 *
 * Parameters:
 *
 * Returns:
 *     
 */
STATIC struct hsl_bcm_vpws_list *
_hsl_bcm_vpws_alloc(void) {
    struct hsl_bcm_vpws_list    *vpws;
    //struct hsl_bcm_vpws_infor_t *vpws_infor;
	
	HSL_FN_ENTER();

    vpws = sal_alloc(sizeof(struct hsl_bcm_vpws_list), "_hsl_bcm_vpws_alloc");
    if (vpws == NULL) {
		HSL_ERR_MALLOC( sizeof(struct hsl_bcm_vpws_list) );
        return NULL;
    }
    sal_memset(vpws, 0, sizeof(struct hsl_bcm_vpws_list));
/*
    vpws->vpws_infor = sal_alloc(sizeof(hsl_bcm_vpws_infor_t), "hsl_bcm_vpws_infor_t");
    if (vpws == NULL) {
		HSL_ERR_MALLOC( sizeof(hsl_bcm_vpws_infor_t) );
        return NULL;
    }
    sal_memset(vpws_infor, 0, sizeof(hsl_bcm_vpws_infor_t));
*/
	vpws->next = NULL;

    HSL_FN_EXIT( vpws );
}


/*
 * Function:
 *
 * Purpose:
 *
 * Parameters:
 *
 * Returns:
 *     
 */
int
_hsl_bcm_vpws_add( struct hsl_bcm_vpws_list *vpws ) 
{
   
    struct hsl_bcm_vpws_list  *vpws_cur, *vpws_next;

    HSL_FN_ENTER();
	
    //VPWS_LOCK();

	if( hsl_vpws_list == NULL) //is head
	{
	    hsl_vpws_list = vpws;
	    HSL_FN_EXIT( 0 );
	} 

	if ( hsl_vpws_list->vpws_id > vpws->vpws_id )
	{  //insert to head 

		vpws->next = hsl_vpws_list;
		hsl_vpws_list = vpws;
		HSL_FN_EXIT( 0 );			
	} 

    vpws_cur= hsl_vpws_list;
	vpws_next = vpws_cur->next;

	while ( vpws_cur != NULL ) 
	{
		/*insert next to current node*/
		if( vpws_next != NULL)
		{
        	if ( ( vpws_cur->vpws_id < vpws->vpws_id )	&& ( vpws_next->vpws_id > vpws->vpws_id) )
        	{
			vpws->next = vpws_next;
			vpws_cur->next = vpws; 
			break;
		}
			} else{  //( vpws_next == NULL )
			vpws_cur->next = vpws; 
			break;
		} 

		vpws_cur = vpws_next;
		vpws_next = vpws_next->next;
		
    }

    HSL_FN_EXIT( 0 );
}

/*
 * Function:
 *
 * Purpose:
 *
 * Parameters:
 *
 * Returns:
 *     
 */
STATIC struct hsl_bcm_vpws_list *
_hsl_bcm_vpws_find( int vpws_id ) {
    struct hsl_bcm_vpws_list      *vpws = NULL;

	HSL_FN_ENTER();

    /* Traverse the lists. */
    for (vpws = hsl_vpws_list;
         vpws != NULL;
         vpws = vpws->next) {
                if (vpws->vpws_id == vpws_id ) {
                break;
            }
        }

    HSL_FN_EXIT( vpws );
}

/*
 * Function:
 *
 * Purpose:
 *
 * Parameters:
 *
 * Returns:
 *     
 */
int
_hsl_bcm_vpws_del( int vpws_id )
{
    struct hsl_bcm_vpws_list   *vpws, *vpws_pre;

	HSL_FN_ENTER();
	
    //VPWS_IS_INIT();
    //VPWS_LOCK();

	vpws = hsl_vpws_list;

	if ( vpws->vpws_id == vpws_id )  //if the head hit
	{
		hsl_vpws_list = vpws->next;
		oss_free ( vpws, OSS_MEM_HEAP);
		HSL_FN_EXIT( 0 );
	}

	do{
		vpws_pre = vpws;
		vpws = vpws->next;
		if ( (vpws != NULL) && (vpws->vpws_id == vpws_id) )
		{
			break;
		}

	} while (vpws != NULL);

        if( vpws == NULL ) {
		HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO, 
	              "----vpws_delete failed: there is no identified vpws!");
	        HSL_FN_EXIT( -1 );
        }else{
		vpws_pre->next = vpws->next;
		oss_free ( vpws, OSS_MEM_HEAP);
		}

    HSL_FN_EXIT( 0 );
}

void
_hsl_show_all_vpws()
{
	struct hsl_bcm_vpws_list   *vpws;
	int counter=0;
	vpws = hsl_vpws_list;
	HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO,"\n==========================\n" );	
	
	while ( vpws != NULL )
	{
		HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO, 
"\n vpws->index=%d\n vpws->type=%d\n vpws->mpls_port_ac.mpls_port_id=0x%x\n vpws->port=%d\n vpws->vlan=%d\n\n vpws->mpls_port_pw.mpls_port_id=0x%x\n vpws->l3_if_infor.l3a_vid=%d\n vpws->obj_id=%d\n vpws->vpn_id=0x%x\n",
vpws->vpws_id,vpws->type,vpws->mpls_port_ac.mpls_port_id,
vpws->port,vpws->vlan,vpws->mpls_port_pw.mpls_port_id,vpws->l3_if_infor.l3a_vid,vpws->obj_id, vpws->vpn_id );
		vpws = vpws->next;
		counter++;
	}

	HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO,
         "   * vpws counter = %d \n ==========================\n\n", counter );	

	return 0;
}


//==.c=============================

/*
 * Function:
 *
 * Purpose:
 *    create mpls tunnel-egress object: bcm_if_t egress_tunnel_if
 * Parameters:
 *
 * Returns:
 *     
 */
int 
hsl_bcm_mpls_ac_port_add( 
	int unit, 
	struct hsl_bcm_msg_vpws_s *vpws_infor,
	bcm_mpls_port_t *mport_ac )
{
	int is_untagged = 1;//TURE
	bcm_gport_t gport;

 	HSL_VPWS_ROE(bcm_port_gport_get, (unit, vpws_infor->ac_port, &gport));
 
	/* create ac interface*/
	bcm_mpls_port_t_init(mport_ac);	

    if(vpws_infor->ac_vlan != 0)
    {
        mport_ac->criteria = BCM_MPLS_PORT_MATCH_PORT_VLAN;
		mport_ac->match_vlan = vpws_infor->ac_vlan;
    }
	else
	{
        mport_ac->criteria = BCM_MPLS_PORT_MATCH_PORT;
	}
		
  	mport_ac->port = gport;

	//HSL_VPWS_ROE(bcm_vlan_create,(unit,vpws_infor->in_vlan));
	//HSL_VPWS_ROE(bcm_vlan_gport_add,(unit,vpws_infor->in_vlan,gport,is_untagged));	
	//bcm_switch_control_port_set(unit,vpws_infor->in_port,bcmSwitchL3EgressMode,1);

	return 0;
}


/*
 * Function:
 *
 * Purpose:
 *    create mpls tunnel-egress object: bcm_if_t egress_tunnel_if
 * Parameters:
 *
 * Returns:
 *     
 */
int 
hsl_bcm_mpls_ac_port_del( 
	int unit, 
	bcm_port_t port,
	bcm_vlan_t vid)
{
	bcm_gport_t gport;

 	HSL_VPWS_ROE(bcm_port_gport_get, (unit, port, &gport));

	bcm_vlan_destroy( unit, vid);
	bcm_vlan_gport_delete( unit, vid, gport);
	//bcm_switch_control_port_set(unit,vpws_infor->in_port,bcmSwitchL3EgressMode,1);

	return 0;
}

/*
 * Function:
 *
 * Purpose:
 *    create mpls tunnel-egress object: bcm_if_t egress_tunnel_if
 * Parameters:
 *
 * Returns:
 *     
 */
int 
hsl_bcm_mpls_pw_port_add( 
	int unit, 
	struct hsl_bcm_msg_vpws_s *vpws_infor,
	bcm_mpls_port_t *mport_pw,
	bcm_l3_intf_t *tunnel_intf_save )
{

	bcm_l3_intf_t tunnel_intf;
	bcm_mpls_egress_label_t tunnel_label;
	bcm_l3_egress_t   egress_object;
	
	bcm_if_t          l3_intf_id;
	bcm_if_t 	  object_id;

	bcm_gport_t gport;
	HSL_VPWS_ROE(bcm_port_gport_get, (unit, vpws_infor->out_port, &gport));

	/*create a mpls tunnel interface*/
	bcm_l3_intf_t_init(&tunnel_intf);
	tunnel_intf.l3a_vid = vpws_infor->ac_vlan;
	tunnel_intf.l3a_ttl = 127;
	sal_memcpy(tunnel_intf.l3a_mac_addr, smac_address_0, sizeof(smac_address_0));
	HSL_VPWS_ROE(bcm_l3_intf_create,(unit, &tunnel_intf));
	l3_intf_id = tunnel_intf.l3a_intf_id;

	bcm_mpls_egress_label_t_init(&tunnel_label);
	tunnel_label.flags = BCM_MPLS_EGRESS_LABEL_PRI_SET|
				BCM_MPLS_EGRESS_LABEL_EXP_SET|BCM_MPLS_EGRESS_LABEL_TTL_SET;
	tunnel_label.label = vpws_infor->out_lsp_label;/*tunnel label value 20*/
	tunnel_label.exp = vpws_infor->out_lsp_exp;
	tunnel_label.ttl = 127;
	//mport_pw.egress_label.pkt_pri= 7;
	//mport_pw.egress_label.pkt_cfi=0 ;

	HSL_VPWS_ROE(bcm_mpls_tunnel_initiator_set,(unit, l3_intf_id, 1,&tunnel_label));

	bcm_module_t femod;    
	bcm_stk_modid_get (unit, &femod);
	bcm_l3_egress_t_init(&egress_object);
	egress_object.vlan = vpws_infor->ac_vlan;
	egress_object.module = femod;
	egress_object.port = vpws_infor->out_port;
	egress_object.intf = l3_intf_id;
	sal_memcpy(egress_object.mac_addr,dmac_address_0,sizeof(dmac_address_0));
	HSL_VPWS_ROE(bcm_l3_egress_create,(unit, 0, &egress_object, &object_id)); //	uint32            flags = 0;

	/*add a mpls port to vpws*/
	bcm_mpls_port_t_init(mport_pw);

	mport_pw->egress_label.flags= BCM_MPLS_EGRESS_LABEL_EXP_SET|BCM_MPLS_EGRESS_LABEL_TTL_SET;
	mport_pw->egress_label.label = vpws_infor->out_pw_label;/*out vc label*/
	mport_pw->egress_label.exp = vpws_infor->out_pw_exp;
	mport_pw->egress_label.ttl = 127;
	mport_pw->egress_tunnel_if = object_id;

	mport_pw->criteria = BCM_MPLS_PORT_MATCH_LABEL;
	mport_pw->flags = BCM_MPLS_PORT_NETWORK|BCM_MPLS_PORT_EGRESS_TUNNEL;
			//|BCM_MPLS_PORT_CONTROL_WORD|BCM_MPLS_PORT_SEQUENCED;
	mport_pw->port = gport;
	mport_pw->match_label = vpws_infor->match_pw_label;  /*remote vc label*/
 	mport_pw->mtu = 1800;
	/* 
 	mport_pw.if_class = ;
        mport_pw.exp_map = ;
        mport_pw.int_pri = ;
        mport_pw.match_inner_vlan = ;
	mport_pw.service_tpid = ;
	mport_pw.egress_service_vlan = ;
	mport_pw.egress_label.flags = 
	mport_pw.egress_label.qos_map_id = ;
	*/
	
	/*for information saving*/
	memcpy(	tunnel_intf_save, 
		&tunnel_intf, sizeof( tunnel_intf ) );

	return 0;
}

/*
 * Function:
 *
 * Purpose:
 *    create mpls tunnel-egress object: bcm_if_t egress_tunnel_if
 * Parameters:
 *
 * Returns:
 *     
 */
int 
hsl_bcm_mpls_pw_port_del( 
	int unit,
	bcm_l3_intf_t *tunnel_intf,
	bcm_mpls_port_t *mport_pw )
{
	/*create a mpls tunnel interface*/
	bcm_l3_intf_delete( unit,tunnel_intf);
	bcm_mpls_tunnel_initiator_clear( unit,tunnel_intf->l3a_intf_id );
	bcm_vlan_gport_delete( unit,mport_pw->match_vlan, mport_pw->port );
	//bcm_switch_control_port_set(unit,vpws_infor->out_port,bcmSwitchL3EgressMode,1);
	bcm_l3_egress_destroy( unit,mport_pw->egress_tunnel_if);

	return 0;
}

/*
 * Function:
 *
 * Purpose:
 *    create mpls tunnel-egress object: bcm_if_t egress_tunnel_if
 * Parameters:
 *
 * Returns:
 *     
 */
int 
hsl_bcm_vpws_pe_create(
	struct hsl_bcm_msg_vpws_s *vpws )
{
	int unit = 0;
	bcm_mpls_port_t mport_pw, mport_ac;
	struct hsl_bcm_msg_vpws_s *vpws_infor = vpws;
	bcm_mpls_egress_label_t bcm_mpls_egr_label = {0};
        bcm_mpls_vpn_config_t vpn_config;
	bcm_l3_intf_t tunnel_intf_save;
	bcm_port_t eth_port, mpls_port;
	bcm_gport_t eth_gport, mpls_gport;
        hsl_bcm_mpls_tunnel_egress_cfg bcm_mpls_egr_info = {0};
	bcm_pbmp_t  port_bmap_untag ;
	bcm_pbmp_t  port_bmap_tag;
        unsigned short vpn_id;
        int rec;

/**************************************************************
    HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO, 
              "-------- \nvpws_infor->in_port = %d\n vpws_infor->in_vlan = %d\n vpws_infor->remote_pw =%d\n\n vpws_infor->out_port =%d\n vpws_infor->out_tunnel =%d\n vpws_infor->out_tunnel_exp = %d\n vpws_infor->out_pw = %d\n vpws_infor->out_pw_exp =%d---------\n\n",
              vpws_infor->in_port, vpws_infor->in_vlan , vpws_infor->remote_pw ,vpws_infor->out_port,vpws_infor->out_tunnel ,vpws_infor->out_tunnel_exp , vpws_infor->out_pw , vpws_infor->out_pw_exp);
/**************************************************************/
  /*1创建vpn for vpws*/
    bcm_mpls_vpn_config_t_init(&vpn_config);


    memset(&port_bmap_untag, 0, sizeof(bcm_pbmp_t));
    memset(&port_bmap_tag, 0, sizeof(bcm_pbmp_t));

    #if 0
    bcm_multicast_t bcast_group;
    HSL_CHECK_RESULT(rec, bcm_multicast_create(unit, BCM_MULTICAST_TYPE_VPLS, &bcast_group));
    vpn_config.broadcast_group = bcast_group;
    vpn_config.unknown_unicast_group = bcast_group;
    vpn_config.unknown_multicast_group = bcast_group;
    vpn_config.flags = BCM_MPLS_VPN_VPLS;
   #endif

    vpn_config.flags  = BCM_MPLS_VPN_VPWS;
    rec = bcm_mpls_vpn_id_create (unit, &vpn_config);
    if (BCM_FAILURE(rec)) {
	  HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO,"bcm_mpls_vpn_id_create FAIL %d: %s\n", rec, bcm_errmsg(rec));
	  return -1;
    }

    HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO,"\n\rhsl_bcm_vpws_pe_create: vpn id = 0x%x\n", vpn_config.vpn);
   vpn_id = vpn_config.vpn;

    /*增加ac端口到vpn中*/
    eth_port = vpws_infor->ac_port;
    HSL_CHECK_RESULT(rec, bcm_port_gport_get (unit, eth_port, &eth_gport));
    HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO,"hsl_bcm_vpws_pe_create: add AC port: %d,  gport: 0x%x\n", eth_port, eth_gport);


    if(vpws->ac_vlan != 0)
   {
           bcm_port_ifilter_set(unit, eth_port, 0);  //关闭vlan过滤
   }


    bcm_mpls_port_t_init(&mport_ac);

   mport_ac.port = eth_gport;
   
   if(vpws->ac_vlan != 0)  /*端口+ vlan为ac, 使能vlan翻译功能*/
   {
        HSL_CHECK_RESULT(rec, bcm_vlan_control_port_set(unit, vpws_infor->ac_port, bcmVlanTranslateIngressEnable, 1));

       mport_ac.criteria = BCM_MPLS_PORT_MATCH_PORT_VLAN;
       mport_ac.match_vlan = vpws_infor->ac_vlan;
       mport_ac.flags |= BCM_MPLS_PORT_SERVICE_TAGGED;
       mport_ac.flags |= BCM_MPLS_PORT_SERVICE_VLAN_REPLACE;
       mport_ac.service_tpid = 0x8100;
       mport_ac.egress_service_vlan =  1024;	   
   }
   else   /*对于端口作为ac，关闭端口pvid功能*/
   {
        mport_ac.criteria = BCM_MPLS_PORT_MATCH_PORT;
        HSL_CHECK_RESULT(rec, hsl_bcm_pvid_action_set(vpws_infor->ac_port, 0));
    }
   
   mport_ac.mtu = 0x3fff;
   mport_ac.egress_tunnel_if = 0;
   HSL_CHECK_RESULT(rec, bcm_mpls_port_add (unit, vpn_id, &mport_ac));
   HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO,"vpws , ac mpls port id 0x%x\n", mport_ac.mpls_port_id);

    /*增加vp端口到vpn*/
    mpls_port = vpws_infor->out_port;
    HSL_CHECK_RESULT(rec, bcm_port_gport_get(unit, mpls_port, &mpls_gport));
    HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO,"hsl_bcm_vpws_pe_create: add vport: %d,  gport: 0x%x\n", mpls_port, mpls_gport);

    bcm_mpls_port_t_init(&mport_pw);

   if(vpws->match_pw_label <= 10240)
   {
        mport_pw.criteria = BCM_MPLS_PORT_MATCH_LABEL;
        mport_pw.port = mpls_gport;
   }
   else
   {
         mport_pw.criteria = BCM_MPLS_PORT_MATCH_LABEL_PORT;
         mport_pw.port = mpls_gport;
   }
   
   mport_pw.match_label = vpws_infor->match_pw_label;

   mport_pw.flags |= BCM_MPLS_PORT_EGRESS_TUNNEL;
   mport_pw.flags |= BCM_MPLS_PORT_COUNTED;
  
   mport_pw.egress_label.label = vpws_infor->out_pw_label;
   mport_pw.egress_label.qos_map_id = 0;
   mport_pw.egress_label.exp = vpws_infor->out_pw_exp;
   mport_pw.egress_label.ttl = 255;
   mport_pw.egress_label.pkt_pri = 0;
   mport_pw.egress_label.pkt_cfi = 0;
   mport_pw.mtu = 0xfff;
   mport_pw.egress_service_vlan = 200;
   mport_pw.service_tpid = 0x8100;
   mport_pw.flags |= BCM_MPLS_PORT_SERVICE_VLAN_REPLACE;

   /*nni 端口使能mpls look */
    HSL_CHECK_RESULT(rec, bcm_port_control_set(unit, mpls_port, bcmPortControlMpls, 1));

    sal_memset(&bcm_mpls_egr_info, 0, sizeof(hsl_bcm_mpls_tunnel_egress_cfg));
    sal_memcpy(bcm_mpls_egr_info.des_mac, vpws->des_mac, sizeof(bcm_mac_t));
    bcm_mpls_egr_info.l3_vid = vpws->mpls_out_vlan;
    bcm_mpls_egr_info.out_port = mpls_port;

    HSL_CHECK_RESULT(rec,  hsl_mpls_tunnel_egress_add(&bcm_mpls_egr_info));
	
    mport_pw.egress_tunnel_if = bcm_mpls_egr_info.egr_if;

    /*隧道绑定到L3 infc*/
    memset(&bcm_mpls_egr_label, 0, sizeof(bcm_mpls_egress_label_t));	
    bcm_mpls_egr_label.label = vpws->out_lsp_label;
    bcm_mpls_egr_label.flags = BCM_MPLS_EGRESS_LABEL_TTL_SET 
			                                   | BCM_MPLS_EGRESS_LABEL_EXP_SET;
	
    bcm_mpls_egr_label.qos_map_id = 0;
    bcm_mpls_egr_label.exp = vpws->out_lsp_exp;
    bcm_mpls_egr_label.ttl = 255;
    bcm_mpls_egr_label.pkt_cfi = 0;
    bcm_mpls_egr_label.pkt_pri = 0;
			
    /* 如果是3标签,对P设备动作为PHP;对PE设备不写驱动 */
    HSL_CHECK_RESULT(rec,bcm_mpls_tunnel_initiator_set(unit,bcm_mpls_egr_info.l3_if,1,&bcm_mpls_egr_label));

    
    HSL_CHECK_RESULT(rec, bcm_mpls_port_add(unit, vpn_id, &mport_pw));
    HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO,"hsl_bcm_vpws_pe_create:  pw mpls port id 0x%x\n", mport_pw.mpls_port_id);

    memset(&port_bmap_untag, 0, sizeof(bcm_pbmp_t));
    memset(&port_bmap_tag, 0, sizeof(bcm_pbmp_t));
	
    BCM_PBMP_PORT_SET(port_bmap_tag, mpls_port);
    BCM_PBMP_PORT_SET(port_bmap_untag, mpls_port);
    HSL_CHECK_RESULT(rec, bcm_vlan_port_add(unit, vpws->mpls_out_vlan, port_bmap_tag, port_bmap_untag));


   /*增加下环处理流程*/
    hsl_bcm_mpls_tunnel_switch_cfg  stTunnelCfg = {0};
   
   memset(&stTunnelCfg, 0, sizeof(hsl_bcm_mpls_tunnel_switch_cfg ));
   stTunnelCfg.in_label = vpws->match_lsp_label;
   stTunnelCfg.in_port = vpws->out_port;
   stTunnelCfg.label_action = HSL_MPLS_LABEL_POP;
   stTunnelCfg.mpls_in_vlan = vpws->mpls_out_vlan;
   HSL_CHECK_RESULT(rec, hsl_bcm_mpls_tunnel_add(&stTunnelCfg));

#if 0
  rec= bcm_vlan_create(unit,vpws_infor->in_vlan);
  if (rec == BCM_E_EXISTS)
  {
    HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO,
		"bcm_vlan_create() vlan:%d already exist !\n", vpws_infor->in_vlan);
  }else if (BCM_FAILURE(rec)) {
    HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO,"BCM FAIL %d: %s\n", rec, bcm_errmsg(rec));
    return -1;
  }

 
  HSL_VPWS_ROE(bcm_port_gport_get, (unit, mpls_port, &mpls_gport));

  HSL_VPWS_ROE(bcm_vlan_gport_add,(unit,vpws_infor->in_vlan,eth_gport,0));	
  bcm_switch_control_port_set(unit,eth_port,bcmSwitchL3EgressMode,1);

  //HSL_VPWS_ROE(bcm_vlan_gport_add,(unit,vpws_infor->in_vlan,mpls_gport,1));
  bcm_switch_control_port_set(unit,mpls_port,bcmSwitchL3EgressMode,1);
  bcm_switch_control_port_set(unit,mpls_port,bcmSwitchHashMPLS,BCM_HASH_LSB);
  bcm_port_control_set(unit, mpls_port,bcmPortControlMpls,1);

/**************************************************************/
	
	
	/*create a pw port*/
	HSL_VPWS_ROE( hsl_bcm_mpls_pw_port_add,( 
			unit, vpws_infor, &mport_pw, &tunnel_intf_save));

	/*create a ac port*/
	HSL_VPWS_ROE( hsl_bcm_mpls_ac_port_add,( unit, vpws_infor,&mport_ac));



	/*bind the ac port and pw port to vpn*/
	HSL_VPWS_ROE( bcm_mpls_port_add, (unit, vpn_id, &mport_ac));
	HSL_VPWS_ROE( bcm_mpls_port_add, (unit, vpn_id, &mport_pw));

  
  bcm_switch_control_port_set(unit,mpls_port,bcmSwitchMplsPortIndependentLowerRange1,0);
  bcm_switch_control_port_set(unit,mpls_port,bcmSwitchMplsPortIndependentLowerRange2,0);
  bcm_switch_control_port_set(unit,mpls_port,bcmSwitchMplsPortIndependentUpperRange1,1048575);
  bcm_switch_control_port_set(unit,mpls_port,bcmSwitchMplsPortIndependentUpperRange2,1048575);
  #endif
/**********************************************************/
  
	/*save important information for managment*/
	struct hsl_bcm_vpws_list *vpws_new;
	vpws_new = _hsl_bcm_vpws_alloc();

	HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO,
				"_hsl_bcm_vpws_alloc a new vpws finished------\n");

	vpws_new->type = 1; //VPWS_PE	
	vpws_new->vpws_id = vpws->vpws_id;
	
	vpws_new->port = vpws_infor->ac_port;
	vpws_new->vlan = vpws_infor->ac_vlan;

	memcpy( &vpws_new->mpls_port_ac, &mport_ac, sizeof(mport_ac) );
	memcpy( &vpws_new->mpls_port_pw, &mport_pw, sizeof(mport_pw) );
	//vpws_new->mpls_port_ac = mport_ac;
	//vpws_new->mpls_port_pw = mport_pw;

	memcpy( &vpws_new->l3_if_infor, &tunnel_intf_save, sizeof(tunnel_intf_save) );
	//vpws_new->l3_if_infor = tunnel_intf_save;
	vpws_new->vpn_id = vpn_id;

	_hsl_bcm_vpws_add(vpws_new);	

	HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO,
				"----hsl_bcm_vpws_pe_create finished------\n");

	_hsl_show_all_vpws();

	return 0;

}


/*
 * Function:
 *
 * Purpose:
 * 
 * Parameters:
 *
 * Returns:
 *     
 */
/*************************************************************
    HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO, 
              "vpws->index=%d\n vpws->type=%d\n vpws->mpls_port_ac.mpls_port_id=%d\n vpws->port=%d\n vpws->vlan=%d\n\n vpws->mpls_port_pw.mpls_port_id=%d\n vpws->l3_if_infor=%d\n vpws->obj_id=%d\n vpws->vpn_id=%d\n---\n\n",
vpws->index,vpws->type,vpws->mpls_port_ac.mpls_port_id,
vpws->port,vpws->vlan,vpws->mpls_port_pw.mpls_port_id,vpws->l3_if_infor,vpws->obj_id, vpws->vpn_id );
*************************************************************/
int
hsl_bcm_vpws_pe_del( 
	int unit,
	struct hsl_bcm_vpws_list *vpws_del )
{

	struct hsl_bcm_vpws_list *vpws;
	vpws = vpws_del;

	/*create a ac port*/
	if (0 > hsl_bcm_mpls_ac_port_del( 
			unit, vpws->port,vpws->vlan ))
	{
		return -1;
	}	

	/*create a pw port*/
	if (0 > hsl_bcm_mpls_pw_port_del( 
		unit, &vpws->l3_if_infor, &vpws->mpls_port_pw))
	{
		return -1;
	}

	bcm_mpls_port_delete(
		unit,vpws->vpn_id,vpws->mpls_port_ac.mpls_port_id);
	bcm_mpls_port_delete(
			unit,vpws->vpn_id,vpws->mpls_port_pw.mpls_port_id);
	bcm_mpls_vpn_id_destroy( unit,vpws->vpn_id);

	_hsl_bcm_vpws_del( vpws->vpws_id);

	_hsl_show_all_vpws();
	HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO,
				"----hsl_bcm_vpws_pe_del OK!------\n");	
	return 0;

}


/*
 * Function:
 *
 * Purpose:
 *    create mpls tunnel-egress object: bcm_if_t egress_tunnel_if
 * Parameters:
 *
 * Returns:
 *     
 */


int 
hsl_bcm_vpws_p_create( 
	struct hsl_bcm_msg_vpws_s *vpws_infor )
{
	int unit = 0;
	int rc;

	bcm_port_t in_port = 15, out_port = 14;
	bcm_mpls_label_t in_tunnel = 0x1000, out_tunnel = 0x2000;
	bcm_gport_t in_gport, out_gport;

	//vlan

	bcm_vlan_t vlan=1;

	//port seting
	HSL_VPWS_ROE(bcm_port_gport_get, (unit, in_port, &in_gport));
	HSL_VPWS_ROE(bcm_port_gport_get, (unit, out_port, &out_gport));
	

	bcm_switch_control_port_set(unit,in_port,bcmSwitchL3EgressMode,1);
	bcm_switch_control_port_set(unit,out_port,bcmSwitchL3EgressMode,1);

	//set mpls_station_tcam
	bcm_port_control_set(unit, in_port,bcmPortControlMpls,1);
	bcm_l2_tunnel_add( unit,dmac_address_0, 1);

	/*create a mpls tunnel interface*/
	bcm_l3_intf_t  pwe_l3if;
	bcm_l3_intf_t_init(&pwe_l3if);
	pwe_l3if.l3a_vid = vlan;
	pwe_l3if.l3a_ttl = 127;
	sal_memcpy(pwe_l3if.l3a_mac_addr, smac_address_0, sizeof(smac_address_0));
	HSL_VPWS_ROE(bcm_l3_intf_create,(unit, &pwe_l3if));
	int pweIntfId;
	pweIntfId = pwe_l3if.l3a_intf_id;
	
	/* Configure the egress MPLS Port */
	bcm_if_t pwe_egid;
	bcm_l3_egress_t pwe_l3eg;
	bcm_l3_egress_t_init(&pwe_l3eg);

	bcm_module_t femod;	 
	bcm_stk_modid_get (unit, &femod);
	pwe_l3eg.intf = pweIntfId;
	sal_memcpy( pwe_l3eg.mac_addr, dmac_address_0, sizeof(dmac_address_0));  
	pwe_l3eg.vlan = vlan; //vlan;
	pwe_l3eg.module = femod;
	pwe_l3eg.port = out_port;

	//for swap
	//pwe_l3eg.mpls_flags = BCM_MPLS_EGRESS_LABEL_EXP_SET;
	//pwe_l3eg.mpls_label = out_tunnel;
	//pwe_l3eg.mpls_exp = 1;
	
	HSL_VPWS_ROE(bcm_l3_egress_create, (unit, 0, &pwe_l3eg, &pwe_egid));
	HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO,"Created egress_l3_intf: 0x%x \n", pwe_egid);


	/**************************************************/
	/**************************************************/
	/**************************************************/
	bcm_mpls_egress_label_t egress_lable;
	bcm_mpls_egress_label_t_init(&egress_lable);
	//egress_lable.flags = BCM_MPLS_EGRESS_LABEL_PRI_SET|	BCM_MPLS_EGRESS_LABEL_EXP_SET|BCM_MPLS_EGRESS_LABEL_TTL_SET;
	egress_lable.label = out_tunnel;/*tunnel label value 20*/
	//egress_lable.exp = vpws_infor->out_tunnel_exp;
	//egress_lable.ttl = 127;
	//HSL_VPWS_ROE(bcm_mpls_tunnel_initiator_set,(unit, pweIntfId, 1,&egress_lable));
 
	bcm_mpls_tunnel_switch_t ler_switch;
	bcm_mpls_tunnel_switch_t_init(&ler_switch);
	ler_switch.flags  = BCM_MPLS_SWITCH_OUTER_TTL; //BCM_MPLS_SWITCH_LOOKUP_INNER_LABEL;  
	ler_switch.label  = in_tunnel;
	ler_switch.action = BCM_MPLS_SWITCH_ACTION_SWAP;//BCM_MPLS_SWITCH_ACTION_POP_DIRECT; 

	ler_switch.egress_label = egress_lable;
	ler_switch.egress_if = pwe_egid;
	
	ler_switch.port = -1;
	ler_switch.mtu = 1800;

	HSL_VPWS_ROE(bcm_mpls_tunnel_switch_add, (unit, &ler_switch));
	HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO,
	  "port %d, incoming label 0x%x action is POP with vpn=0x%x\n", in_port,ler_switch.label, 0);  
	
	return 0;
}

/*
 * Function:
 *
 * Purpose:
 *
 * Parameters:
 *
 * Returns:
 *     
 */
int hsl_bcm_vpws_delete( int *ifp )
{
	int unit = 0;
	int vpws_id = *ifp;
	int ret = 0;
	
	struct hsl_bcm_vpws_list *vpws_del;
		
	HSL_FN_ENTER();

	vpws_del = _hsl_bcm_vpws_find(vpws_id);
	
    if( vpws_del->type == 1)  //PE
    {
		hsl_bcm_vpws_pe_del( unit,vpws_del );
	} 
	else if( vpws_del->type == 2)  //P
	{
		return 0;
	}

	HSL_FN_EXIT(ret);
}
#endif

