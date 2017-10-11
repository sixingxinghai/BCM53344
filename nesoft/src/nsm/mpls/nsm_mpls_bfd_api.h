/* Copyright (C) 2009-10  All Rights Reserved. */

#ifndef _PACOS_NSM_MPLS_BFD_API_H
#define _PACOS_NSM_MPLS_BFD_API_H

enum bfd_vccv_ops
{
  BFD_VCCV_OP_NONE,
  BFD_VCCV_OP_START,
  BFD_VCCV_OP_STOP
};

int
nsm_mpls_bfd_api_fec_set (u_int32_t vr_id, char *lsp_name, char *input,
                          u_int32_t lsp_ping_intvl, u_int32_t min_tx,
                          u_int32_t min_rx, u_int32_t mult,
                          bool_t force_explicit_null);

int
nsm_mpls_bfd_api_fec_disable_set (u_int32_t vr_id, char *lsp_name, char *input);

int
nsm_mpls_bfd_api_lsp_all_set (u_int32_t vr_id, char *lsp_name,
                              u_int32_t lsp_ping_intvl, u_int32_t min_tx,
                              u_int32_t min_rx, u_int32_t mult,
                              bool_t force_explicit_null);

int
nsm_mpls_bfd_api_fec_unset (u_int32_t vr_id, char *lsp_name,  char *input);

int
nsm_mpls_bfd_api_lsp_all_unset (u_int32_t vr_id, char *lsp_name);

#ifdef HAVE_VCCV
int
nsm_mpls_bfd_api_vccv_trigger (struct nsm_master *nm, u_int32_t vc_id, u_char op);
#endif /* HAVE_VCCV */

#endif /* _PACOS_NSM_MPLS_BFD_API_H */
