/* Copyright (C) 2009-10  All Rights Reserved. */

#ifndef _PACOS_NSM_MPLS_BFD_H
#define _PACOS_NSM_MPLS_BFD_H

#if defined (HAVE_MPLS_OAM) && defined (HAVE_BFD)

#include "bfd_message.h"

struct bfd_client_api_session;
struct ftn_entry;
struct nsm_mpls_circuit;

/**@brief Sturcture to store the BFD configuration for an LSP type
 */
struct nsm_mpls_bfd_lsp_conf
{
  u_int32_t lsp_ping_intvl; /** LSP Ping interval in seconds. */
  u_int32_t min_tx; /** Minimum BFD Tx interval in milli-seconds */ 
  u_int32_t min_rx; /** Minimum BFD Rx interval in milli-seconds */
  u_int32_t mult; /** Detection Multiplier value */
  bool_t force_explicit_null; /* Force Explicit NULL label */
};

/**@brief Sturcture to store the BFD configuration for FEC/Trunk 
 */
struct nsm_mpls_bfd_fec_conf
{
  u_int16_t lsp_type; /** LSP Type Values defined in lib/bfd_message.h */
  struct prefix fec; /** Prefix for LDP/Static FEC */
  u_char *tunnel_name; /** RSVP Tunnel Name */
  u_int32_t lsp_ping_intvl; /** LSP Ping interval in seconds. */
  u_int32_t min_tx; /** Minimum BFD Tx interval in milli-seconds */ 
  u_int32_t min_rx; /** Minimum BFD Rx interval in milli-seconds */
  u_int32_t mult; /** BFD Detection Multiplier value */
  bool_t force_explicit_null; /* Force Explicit NULL label */
  u_char cflag; /** BFD config flag */
#define NSM_MPLS_BFD_FEC_ENABLE    (1 << 0)
#define NSM_MPLS_BFD_FEC_DISABLE   (1 << 1)
}; 

/* Function prototypes. */

u_int16_t
nsm_mpls_bfd_lsp_name2type (char *lsp_name);

char *
nsm_mpls_bfd_lsp_type2name (u_int16_t lsp_type);

int
nsm_mpls_bfd_lsp_type2flag (u_int16_t lsp_type);

u_char
nsm_mpls_bfd_lsp_type2ftn_owner (u_int16_t lsp_type);

u_int16_t
nsm_mpls_bfd_ftn_owner2lsp_type (u_char owner);

int
nsm_mpls_bfd_send_ftn_down (struct nsm_master *nm, struct ftn_entry *ftn,
                             u_int32_t lsp_type);
void
nsm_mpls_bfd_session_down (struct bfd_client_api_session *s);

void
nsm_mpls_bfd_enable (struct nsm_master *nm);

void
nsm_mpls_bfd_disable (struct nsm_master *nm);

struct nsm_mpls_bfd_fec_conf *
nsm_mpls_bfd_fec_conf_lookup (struct list *bfd_fec_conf_list,
                              u_int16_t lsp_type, void *input);

int
nsm_mpls_bfd_fec_conf_update (struct nsm_master *nm,
                              struct nsm_mpls_bfd_fec_conf *bfd_conf,
                              struct nsm_mpls_bfd_fec_conf *input);

int
nsm_mpls_bfd_lsp_conf_update (struct nsm_master *nm,
                              struct nsm_mpls_bfd_lsp_conf *input,
                              u_int16_t lsp_type);

struct nsm_mpls_bfd_fec_conf *
nsm_mpls_bfd_fec_conf_new (struct list *mpls_bfd_fec_conf_list,
                           struct nsm_mpls_bfd_fec_conf *input);

int
nsm_mpls_bfd_fec_conf_session_add (struct nsm_master *nm,
                                   struct nsm_mpls_bfd_fec_conf *bfd_conf,
                                   bool_t is_update);

int
nsm_mpls_bfd_fec_disable (struct nsm_master *nm,
                          struct nsm_mpls_bfd_fec_conf *bfd_conf);

bool_t
nsm_mpls_bfd_lsp_fec_conf_attr_diff (struct nsm_master *nm,
                                     struct nsm_mpls_bfd_fec_conf *bfd_conf);

int
nsm_mpls_bfd_update_session_by_fec (struct nsm_master *nm,
                                    struct nsm_mpls_bfd_fec_conf *bfd_conf,
                                    struct ftn_entry *ftn,
                                    bool_t is_add,
                                    bool_t admin_flag);

int
nsm_mpls_bfd_fec_conf_session_delete (struct nsm_master *nm,
                                      struct nsm_mpls_bfd_fec_conf *bfd_conf);

int
nsm_mpls_bfd_lsp_session_add (struct nsm_master *nm, u_int16_t lsp_type);

int
nsm_mpls_bfd_lsp_update_session_by_ftn (struct nsm_master *nm, 
                                        struct ftn_entry *ftn,
                                        u_int16_t lsp_type,
                                        bool_t is_add,
                                        bool_t admin_flag);

int
nsm_mpls_bfd_lsp_session_delete (struct nsm_master *nm, u_int16_t lsp_type);

int
nsm_mpls_bfd_session_mpls_data_set (struct nsm_master *nm,
                                    struct bfd_client_api_session *s,
                                    struct ftn_entry *ftn,
                                    u_int16_t lsp_type);

void
nsm_mpls_ftn_bfd_update (struct nsm_master *nm, struct ftn_entry *ftn);

void
nsm_mpls_ftn_bfd_delete (struct nsm_master *nm, struct ftn_entry *ftn);

#ifdef HAVE_VCCV
void
nsm_vccv_bfd_enable (struct nsm_master *nm);

void
nsm_vccv_bfd_disable (struct nsm_master *nm);

int
nsm_mpls_bfd_session_vccv_data_set (struct nsm_master *nm,
                                     struct nsm_mpls_circuit *vc,
                                     struct bfd_client_api_session *s);

int
nsm_mpls_vc_bfd_session_add (struct nsm_master *nm, 
                              struct nsm_mpls_circuit *vc);

int
nsm_mpls_vc_bfd_session_delete (struct nsm_master *nm, 
                                 struct nsm_mpls_circuit *vc,
                                 bool_t admin_flag);
void
nsm_vccv_bfd_session_down (struct bfd_client_api_session *s);
#endif /* HAVE_VCCV */
#endif /* HAVE_BFD  && HAVE_MPLS_OAM */
#endif /* _PACOS_NSM_MPLS_BFD_H */
