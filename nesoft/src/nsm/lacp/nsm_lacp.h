/* Copyright (C) 2001-2003  All Rights Reserved. */

#ifndef _PACOS_NSM_LACP_H_
#define _PACOS_NSM_LACP_H_
#include "nsm_interface.h"
#include "nsm_message.h"
/* Error returns for API calls */
#define NSM_LACP_ERROR_BASE                   -1000 
#define NSM_LACP_ERROR_ADMIN_KEY_MISMATCH     (NSM_LACP_ERROR_BASE + 1)
#define NSM_LACP_ERROR_CONFIGURED             (NSM_LACP_ERROR_BASE + 2)
#define NSM_LACP_ERROR_MISMATCH               (NSM_LACP_ERROR_BASE + 3)
#define NSM_LACP_IF_CONSISTS_PROTECTION_GRP   (NSM_LACP_ERROR_BASE + 4)

/* Default aggregator port selection criteria */
#define NSM_LACP_PSC_DEFAULT  HAL_LACP_PSC_SRC_DST_MAC
#define NSM_MAX_AGGREGATOR_LINKS 6
#define NSM_LACP_LINK_ADMIN_KEY_MIN   1
#define NSM_LACP_LINK_ADMIN_KEY_MAX   65535

struct nsm_lacp_admin_key_element
{
  u_int16_t key;
  u_int16_t hw_type;
  float32_t bandwidth;
  int mtu;
  int duplex;
  u_int16_t refcount;
  u_char type;
#ifdef HAVE_VLAN
  u_char vlan_port_mode;
  /* For Checking Vlan All, None or Specific. */
  u_char vlan_port_config;
  u_int16_t vlan_id;
  struct nsm_vlan_bmp *vlan_bmp_static;
  struct nsm_vlan_bmp *vlan_bmp_dynamic;
#endif /* HAVE_VLAN */
#ifdef HAVE_L2
  int bridge_id;
#endif /* HAVE_L2 */
  
  struct nsm_lacp_admin_key_element *next;

#ifdef HAVE_HA
  HA_CDR_REF ake_lacp_cdr_ref;
#endif /* HAVE_HA */
};

/* Function prototype. */
void
nsm_lacp_cli_init (struct cli_tree *ctree);

int nsm_lacp_set_server_callback (struct nsm_server *ns);
void nsm_lacp_admin_key_element_free (struct nsm_master *,
                                      struct nsm_lacp_admin_key_element *);
int nsm_lacp_if_admin_key_set (struct interface *);
void nsm_lacp_if_admin_key_unset (struct interface *);
struct nsm_lacp_admin_key_element *
nsm_lacp_admin_key_element_lookup (struct nsm_master *nm, u_int16_t key);

int
nsm_lacp_admin_key_element_add (struct nsm_master *nm,
                                struct nsm_lacp_admin_key_element *ake);

struct nsm_lacp_admin_key_element *
nsm_lacp_admin_key_element_remove (struct nsm_master *nm, u_int16_t key);

void nsm_lacp_aggregator_bw_update (struct interface *);
int nsm_lacp_aggregator_psc_set (struct interface *ifp, int psc);
char *nsm_lacp_psc_string (int);

/* Cli API */
int nsm_lacp_api_add_aggregator_member (struct nsm_master *nm, struct interface *ifp, u_int32_t key, char activate, bool_t notify_lacp, u_char agg_type); 

int nsm_lacp_api_delete_aggregator_member (struct nsm_master *nm, struct interface *ifp, bool_t notify_lacp);


/* Functions to handle messages from lacp */
int nsm_lacp_aggregator_add_msg_process (struct nsm_master *nm, struct nsm_msg_lacp_agg_add *msg);

int nsm_lacp_recv_aggregator_del (struct nsm_msg_header *header, void *arg, void *message);

int nsm_lacp_recv_aggregator_add (struct nsm_msg_header *header, void *arg, void *message);

void nsm_lacp_cleanup_aggregator (struct nsm_master *nm, struct interface *agg_ifp);

int nsm_lacp_delete_aggregator (struct nsm_master *, struct interface *);

int nsm_update_aggregator_member_state (struct nsm_if *nif);

void nsm_lacp_delete_all_aggregators (struct nsm_master *nm);

int nsm_lacp_process_delete_aggregator (struct nsm_master *nm, struct interface *ifp);

int nsm_lacp_process_delete_aggregator_member (struct nsm_master *nm,
                                               char *name, u_int32_t ifindex);

int nsm_lacp_if_config_write(struct cli *cli, struct interface *ifp);

int nsm_lacp_send_aggregator_config (u_int32_t ifindex, u_int32_t key, char activate, int add_agg);

int
nsm_lacp_send_aggregate_member (char *br_name, struct interface *mem_ifp,
                                bool_t add);

#ifdef HAVE_ONMD
void
nsm_lacp_update_lldp_agg_port_id (struct interface *agg_ifp);
#endif /* HAVE_ONMD */
int nsm_lacp_check_link_mismatch (struct nsm_master *nm,
                                  struct interface *first_child,
                                  struct interface *candidate_ifp);

struct nsm_lacp_admin_key_element *
nsm_lacp_admin_key_element_lookup_by_ifp (struct nsm_master *nm,
                                              struct interface *ifp);

/* Update the Config Flag of the Child. */
int nsm_vlan_agg_update_config_flag (struct interface *agg_ifp);

#define NSM_AGG_IS_ONE_MEMBER_UP(ZIF)                                    \
        if (((ZIF)->agg.type == NSM_IF_AGGREGATED)&&                       \
              ((ZIF)->agg.info.parent))                                    \
          {                                                              \
            parent_if = (ZIF)->agg.info.parent->info;                      \
            if ((parent_if) && (parent_if->agg.info.memberlist)&&        \
                (listcount(parent_if->agg.info.memberlist) > 0))         \
               {                                                         \
                 struct interface *_memifp;                               \
                 struct listnode *_lsn;                                   \
                 AGGREGATOR_MEMBER_LOOP (parent_if, _memifp, _lsn)         \
                   {                                                     \
                     if (if_is_running (_memifp))                         \
                       {                                                 \
                         one_member_up = PAL_TRUE;                       \
                         break;                                          \
                       }                                                 \
                    }                                                    \
                }                                                        \
           } 

#define NSM_LACP_PSC_KEYWORD_GET(A)                                           \
    ((A) == HAL_LACP_PSC_DST_MAC               ? "dst-mac" :                  \
     (A) == HAL_LACP_PSC_SRC_MAC               ? "src-mac" :                  \
     (A) == HAL_LACP_PSC_SRC_DST_MAC           ? "src-dst-mac" :              \
     (A) == HAL_LACP_PSC_SRC_IP                ? "src-ip" :                   \
     (A) == HAL_LACP_PSC_DST_IP                ? "dst-ip" :                   \
     (A) == HAL_LACP_PSC_SRC_DST_IP            ? "src-dst-ip" :               \
     (A) == HAL_LACP_PSC_SRC_PORT              ? "src-port" :                 \
     (A) == HAL_LACP_PSC_DST_PORT              ? "dst-port" :                 \
     (A) == HAL_LACP_PSC_SRC_DST_PORT          ? "src-dst-port" : "Unknown")

#ifdef HAVE_L2
void nsm_bridge_if_send_state_sync_req_wrap ( struct interface *ifp);
#endif /* HAVE_L2 */

#endif /* _PACOS_NSM_LACP_H_ */
