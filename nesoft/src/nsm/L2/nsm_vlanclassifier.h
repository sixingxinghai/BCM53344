/*   Copyright (C) 2003  All Rights Reserved.  
  
   This module declares the interface to the Layer 2 Vlan classification CLI
*/
#ifndef __NSM_VLANCLASSIFIER_H__
#define __NSM_VLANCLASSIFIER_H__

#define NSM_VLAN_INVALID_GROUP_ID   0
#define NSM_VLAN_INVALID_VLAN_ID    0
#define NSM_VLAN_GROUP_ID_MIN       1
#define NSM_VLAN_GROUP_ID_MAX       16
#define NSM_VLAN_RULE_ID_MIN        1
#define NSM_VLAN_RULE_ID_MAX        256 
#define NSM_VLAN_DEFAULT_GROUP_ID   1
#ifdef HAVE_SNMP
#define NSM_VLAN_ROW_STATUS_ACTIVE  1
#endif /* HAVE_SNMP */

 
#define    APN_CLASS_P_PUP       0x0200          /* Xerox PUP packet             */
#define    APN_CLASS_P_PUPAT     0x0201          /* Xerox PUP Addr Trans packet  */
#define    APN_CLASS_P_IP        0x0800          /* Internet Protocol packet     */
#define    APN_CLASS_P_X25       0x0805          /* CCITT X.25                   */
#define    APN_CLASS_P_ARP       0x0806          /* Address Resolution packet    */
#define    APN_CLASS_P_BPQ       0x08FF          /* G8BPQ AX.25 Ethernet Packet  [ NOT AN OFFICIALLY REGISTERED ID ] */
#define    APN_CLASS_P_IEEEPUP   0x0a00          /* Xerox IEEE802.3 PUP packet */
#define    APN_CLASS_P_IEEEPUPAT 0x0a01          /* Xerox IEEE802.3 PUP Addr Trans packet */
#define    APN_CLASS_P_DEC       0x6000          /* DEC Assigned proto           */
#define    APN_CLASS_P_DNA_DL    0x6001          /* DEC DNA Dump/Load            */
#define    APN_CLASS_P_DNA_RC    0x6002          /* DEC DNA Remote Console       */
#define    APN_CLASS_P_DNA_RT    0x6003          /* DEC DNA Routing              */
#define    APN_CLASS_P_LAT       0x6004          /* DEC LAT                      */
#define    APN_CLASS_P_DIAG      0x6005          /* DEC Diagnostics              */
#define    APN_CLASS_P_CUST      0x6006          /* DEC Customer use             */
#define    APN_CLASS_P_SCA       0x6007          /* DEC Systems Comms Arch       */
#define    APN_CLASS_P_RARP      0x8035          /* Reverse Addr Res packet      */
#define    APN_CLASS_P_ATALK     0x809B          /* Appletalk DDP                */
#define    APN_CLASS_P_AARP      0x80F3          /* Appletalk AARP               */
#define    APN_CLASS_P_8021Q     0x8100          /* 802.1Q VLAN Extended Header  */
#define    APN_CLASS_P_IPX       0x8137          /* IPX over DIX                 */
#define    APN_CLASS_P_IPV6      0x86DD          /* IPv6 over bluebook           */
#define    APN_CLASS_P_SPT       0x8809          /* Slow Protocol Types          */
#define    APN_CLASS_P_PPP_DISC  0x8863          /* PPPoE discovery messages     */
#define    APN_CLASS_P_PPP_SES   0x8864          /* PPPoE session messages       */
#define    APN_CLASS_P_ATMMPOA   0x884c          /* MultiProtocol Over ATM       */
#define    APN_CLASS_P_ATMFATE   0x8884          /* Frame-based ATM Transport    */

#define    APN_CLASS_NAME_PUP       "xeroxpup"            /* Xerox PUP packet             */
#define    APN_CLASS_NAME_PUPAT     "xeroxaddrtrans"      /* Xerox PUP Addr Trans packet  */
#define    APN_CLASS_NAME_IP        "ip"                  /* Internet Protocol packet     */
#define    APN_CLASS_NAME_X25       "x25"                 /* CCITT X.25                   */
#define    APN_CLASS_NAME_ARP       "arp"                 /* Address Resolution packet    */
#define    APN_CLASS_NAME_BPQ       "g8bpqx25"            /* G8BPQ AX.25 Ethernet Packet  */
#define    APN_CLASS_NAME_IEEEPUP   "ieeepup"             /* Xerox IEEE802.3 PUP packet   */
#define    APN_CLASS_NAME_IEEEPUPAT "ieeeaddrtrans"       /* Xerox IEEE802.3 PUP Addr Trans packet */
#define    APN_CLASS_NAME_DEC       "dec"                 /* DEC Assigned proto           */
#define    APN_CLASS_NAME_DNA_DL    "decdnadumpload"      /* DEC DNA Dump/Load            */
#define    APN_CLASS_NAME_DNA_RC    "decdnaremoteconsole" /* DEC DNA Remote Console       */
#define    APN_CLASS_NAME_DNA_RT    "decdnarouting"       /* DEC DNA Routing              */
#define    APN_CLASS_NAME_LAT       "declat"              /* DEC LAT                      */
#define    APN_CLASS_NAME_DIAG      "decdiagnostics"      /* DEC Diagnostics              */
#define    APN_CLASS_NAME_CUST      "deccustom"           /* DEC Customer use             */
#define    APN_CLASS_NAME_SCA       "decsyscomm"          /* DEC Systems Comms Arch       */
#define    APN_CLASS_NAME_RARP      "rarp"                /* Reverse Addr Res packet      */
#define    APN_CLASS_NAME_ATALK     "atalkddp"            /* Appletalk DDP                */
#define    APN_CLASS_NAME_AARP      "atalkaarp"           /* Appletalk AARP               */
#define    APN_CLASS_NAME_8021Q     "vlan8021q"           /* 802.1Q VLAN Extended Header  */
#define    APN_CLASS_NAME_IPX       "ipx"                 /* IPX over DIX                 */
#define    APN_CLASS_NAME_IPV6      "ipv6"                /* IPv6 over bluebook           */
#define    APN_CLASS_NAME_SPT       "spt"                 /* Slow Protocol Types          */
#define    APN_CLASS_NAME_PPP_DISC  "pppdiscovery"        /* PPPoE discovery messages     */
#define    APN_CLASS_NAME_PPP_SES   "pppsession"          /* PPPoE session messages       */
#define    APN_CLASS_NAME_ATMMPOA   "atmmulti"            /* MultiProtocol Over ATM       */
#define    APN_CLASS_NAME_ATMFATE   "atmtransport"        /* Frame-based ATM Transport    */


#define APN_ERR_SAME_RULE           (-100)
#define APN_ERR_RULE_ID_EXIST       (-101)

#define APN_ERR_VLAN_ON_RULES_FAIL  (-102)
#define APN_ERR_NO_VLAN_ON_RULES    (-103)
#define APN_ERR_NO_VLAN_ON_GROUP    (-104)
#define APN_ERR_NO_GROUP_EXIST      (-105)

struct nsm_vlan_class_group_list;

struct nsm_vlan_class_if_group
{
 u_int16_t group_id;
 u_int16_t group_vid;
 int row_status;
};


/* VLAN Classification group */
struct nsm_vlan_classifier_group
{
  u_int32_t group_id;              /* Group identification number. */
  u_int32_t group_vid;             /* Vlan Group identification number. */
  struct avl_tree  *if_tree;       /* Tree of interfaces group attached to. */
  struct avl_tree  *rule_tree;     /* Tree of rules in the group.  */
  char row_status;
};

void nsm_vlan_classifier_cli_init(struct cli_tree *ctree);
int  nsm_vlan_class_group_exists (struct nsm_bridge_master *master, u_int32_t group_id);
int  nsm_vlan_class_write(struct cli *cli, struct interface *ifp);


int nsm_vlan_classifier_rule_id_cmp (void *data1,void *data2);
int nsm_vlan_classifier_group_id_cmp (void *data1,void *data2);
int nsm_vlan_classifier_ifp_cmp (void *data1,void *data2);
int nsm_del_classifier_rule (struct nsm_bridge_master *master, struct hal_vlan_classifier_rule *rule_ptr);
int nsm_del_classifier_rule_by_id (struct nsm_bridge_master *master, u_int32_t rule_id);
int nsm_del_classifier_group (struct nsm_bridge_master *master, 
                              struct nsm_vlan_classifier_group *group_ptr);
int nsm_del_classifier_group_by_id (struct nsm_bridge_master *master, u_int32_t group_id);
int nsm_add_classifier_rule_to_if (struct interface *ifp, struct hal_vlan_classifier_rule *rule_ptr);
int nsm_del_classifier_rule_from_if (struct interface *ifp, struct hal_vlan_classifier_rule *rule_ptr);
int nsm_add_classifier_group_to_if (struct interface *ifp, 
                                    int group_id,
                                    int vlan_id);
int nsm_del_classifier_group_from_if (struct interface *ifp, 
                                      int group_id);
int nsm_add_classifier_rule (struct nsm_bridge_master *master, struct hal_vlan_classifier_rule *rule_ptr);
int nsm_bind_rule_to_group (struct nsm_bridge_master *master,
                       u_int32_t group_id, u_int32_t rule_id);
int nsm_unbind_rule_from_group (struct nsm_bridge_master *master,
                           u_int32_t group_id, u_int32_t rule_id);
int nsm_vlan_classifier_write (struct cli *cli);
int nsm_get_rule_reference_count (struct hal_vlan_classifier_rule *rule_ptr);

int nsm_del_all_classifier_rules(struct nsm_bridge_master *master);
int nsm_del_all_classifier_groups(struct nsm_bridge_master *master);
void nsm_vlan_classifier_del_group_tree(struct avl_tree **pp_group_tree);
void nsm_vlan_classifier_del_rules_tree(struct avl_tree **pp_rules_tree);

struct nsm_vlan_classifier_group *
nsm_add_new_classifier_group(struct nsm_bridge_master *master,
                             u_int32_t group_id);

struct nsm_vlan_classifier_group *
nsm_get_classifier_group_by_id(struct nsm_bridge_master *master,
                               u_int32_t group_id);

struct nsm_vlan_class_if_group *
nsm_get_classifier_group_by_id_from_if(struct interface *ifp,
                                       u_int32_t group_id);
int
nsm_vlan_classifier_rule_cmp(struct hal_vlan_classifier_rule *first,
                             struct hal_vlan_classifier_rule *second);

struct hal_vlan_classifier_rule *
nsm_add_new_classifier_rule(struct nsm_bridge_master *master,
                            struct hal_vlan_classifier_rule *rule);

struct hal_vlan_classifier_rule *
nsm_get_classifier_rule_by_id(struct nsm_bridge_master *master,
                              u_int32_t rule_id);

int
nsm_vlan_apply_vlan_on_rule(struct nsm_vlan_classifier_group *group,
                             int vlan_id);

int
nsm_del_all_classifier_groups_from_if (struct interface *ifp);

int
nsm_vlan_class_if_group_id_cmp(void *data1,void *data2);

void
nsm_vlan_classifier_free_if_group(void *group_ptr);

char *
nsm_classifier_proto_str (u_int16_t proto);

#endif /* __NSM_VLAN_CLASSIFIER_H__ */
