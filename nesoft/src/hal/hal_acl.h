#ifndef __HAL_ACL_H__
#define __HAL_ACL_H__

#define HAL_MAC_ACL_NAME_SIZE           20

#define HAL_ACL_ACTION_ATTACH           1
#define HAL_ACL_ACTION_DETACH           2

#ifdef HAVE_VLAN
#define HAL_VLAN_ACC_MAP_ADD            1
#define HAL_VLAN_ACC_MAP_DELETE         2
#endif

#define HAL_ACL_DIRECTION_INGRESS       1   /* Incoming */
#define HAL_ACL_DIRECTION_EGRESS        2   /* Outgoing */



struct hal_acl_mac_addr
{
    u_int8_t mac[6];                        /* MAC address */
};

/* Definition of the class map structure for communication with HSL */
struct hal_mac_access_grp
{
    char name[HAL_MAC_ACL_NAME_SIZE];          /* Class map name */

#ifdef HAVE_VLAN
    char vlan_map[HAL_MAC_ACL_NAME_SIZE];
#endif

    u_int8_t deny_permit;                   /* deny or permit */
    u_int8_t l2_type;                       /* The packet format */

    struct hal_acl_mac_addr a;                  /* Source MAC address */
    struct hal_acl_mac_addr a_mask;             /* Source mask (prefix) */
    struct hal_acl_mac_addr m;                  /* Destination MAC address */
    struct hal_acl_mac_addr m_mask;             /* Destination mask (prefix) */

};

#ifdef HAVE_L2LERN
int
hal_mac_set_access_grp( struct hal_mac_access_grp *hal_macc_grp,
                        int ifindex,
                        int action,
                        int dir);

#ifdef HAVE_VLAN
int
hal_vlan_set_access_map( struct hal_mac_access_grp *hal_macc_grp,
                        int vid,
                        int action);
#endif /* HAVE_VLAN */
#endif /* HAVE_L2LERN */

#endif
