
#ifndef _HSL_VPWS_H_
#define _HSL_VPWS_H_

/*
  HSL structure.
*/
struct hsl_bcm_msg_vpws_s{

int vpws_id; 
//ac port
bcm_port_t ac_port;
int ac_vlan;  //int16 

//mpls port
bcm_mpls_label_t match_pw_label;
bcm_mpls_label_t match_lsp_label;
bcm_port_t out_port;
bcm_mpls_label_t out_lsp_label;
int8 out_lsp_exp;
bcm_mpls_label_t out_pw_label;
int8 out_pw_exp;
bcm_mac_t    des_mac;
int mpls_out_vlan;
};

struct hsl_bcm_vpws_list
{
int vpws_id;
int type;

bcm_mpls_port_t   	mpls_port_ac;
bcm_port_t			port;
bcm_vlan_t 			vlan;
	
bcm_mpls_port_t     mpls_port_pw;
bcm_l3_intf_t 		l3_if_infor;
bcm_if_t 		    obj_id;

bcm_vpn_t 				vpn_id;
//bcm_mpls_vpn_config_t 	vpws_vpn;

struct hsl_bcm_vpws_list *next;
};

/* 
  Function prototypes.
*/
int hsl_bcm_vpws_pe_create( struct hsl_bcm_msg_vpws_s *vpws );
int hsl_bcm_vpws_p_create( struct hsl_bcm_msg_vpws_s *vpws );
int hsl_bcm_vpws_delete( int *ifp );

#endif /* _HSL_VPWS_H_ */
