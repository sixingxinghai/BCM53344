#ifndef __NSM_ACL_H__
#define __NSM_ACL_H__

#define NSM_ACL_TYPE_MAC        1       /* MAC acl type */
#define NSM_ACL_TYPE_IP         2       /* IP acl type */

#define NSM_ACL_FILTER_FMT_ETH_II   1       /* type/len >= 0x600 */
#define NSM_ACL_FILTER_FMT_802_3    2       /* type/len < 0x600 */
#define NSM_ACL_FILTER_FMT_SNAP     4       /* SNAP packet */
#define NSM_ACL_FILTER_FMT_LLC      8       /* LLC packet */

/* Define the action of service-policy map */
#define NSM_ACL_ACTION_ADD          1
#define NSM_ACL_ACTION_DELETE       2

#ifdef HAVE_VLAN
#define NSM_VLAN_ACC_MAP_ADD        1
#define NSM_VLAN_ACC_MAP_DELETE     2
#endif

/* Define the direction of service-policy map */
#define NSM_ACL_DIRECTION_INGRESS   1
#define NSM_ACL_DIRECTION_EGRESS    2

enum acl_filter_type
  {
    ACL_FILTER_DENY,
    ACL_FILTER_PERMIT,
  };

struct acl_mac_addr
{
  u_int8_t mac[6];
};

struct mac_acl
{
  char *name;

  /* Acl type information - MAC or IP */
  int  acl_type;

  /* Reference count. */
  u_int32_t ref_cnt;

  enum acl_filter_type type;

  int extended;
  int packet_format;
  struct acl_mac_addr a;
  struct acl_mac_addr a_mask;
  struct acl_mac_addr m;
  struct acl_mac_addr m_mask;

  struct nsm_mac_acl_master *master;

  struct mac_acl *next;
  struct mac_acl *prev;
};

struct mac_acl_list
{
  struct mac_acl *head;
  struct mac_acl *tail;
};

struct nsm_mac_acl_master
{
  struct mac_acl_list num;
};

int mac_acl_list_master_init (struct nsm_master *nm);
struct mac_acl * mac_acl_new ();
void mac_acl_free (struct mac_acl *access);
struct mac_acl * mac_acl_lock (struct mac_acl *access);
void mac_acl_unlock (struct mac_acl *access);
struct mac_acl * mac_acl_insert (struct nsm_mac_acl_master *master, const char *name);
void mac_acl_delete (struct mac_acl *access);
struct mac_acl * mac_acl_lookup (struct nsm_mac_acl_master *master, const char *name);
struct mac_acl * mac_acl_get (struct nsm_mac_acl_master *master, const char *name);

#endif /* __NSM_ACL_H__ */
