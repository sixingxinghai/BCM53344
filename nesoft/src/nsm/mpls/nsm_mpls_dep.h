/* Copyright (C) 2007  All Rights Reserved. */
                                                                               
#ifndef _NSM_MPLS_DEP_H
#define _NSM_MPLS_DEP_H

/* LSP Dependency types. */
typedef enum confirm_type {
  CONFIRM_MAPPED_ROUTE = 1,
  CONFIRM_RSVP_MAPPED_ROUTE,
  CONFIRM_RSVP_BYPASS_ILM,
  CONFIRM_RSVP_BYPASS_FTN,
  CONFIRM_IGP_SHORTCUT_ROUTE,
  CONFIRM_VRF,
  CONFIRM_VC,
  CONFIRM_VPLS_MESH_VC,
  CONFIRM_VPLS_SPOKE_VC,
  CONFIRM_MAPPED_LSP
} confirm_type_t;

/* Structure to store pointers for VRF/VC FTN entries that
   require an LSP originating on this node to work. */
struct lsp_dep_confirm
{
  confirm_type_t type;

  /* Pointer to VRF/VC entry. */
  void *data;

  /* Prev/Next entry. */
  struct lsp_dep_confirm *prev;
  struct lsp_dep_confirm *next;
};

struct confirm_list *mpls_confirm_list_new (u_char, struct prefix *);
void mpls_confirm_list_free (struct confirm_list *);
int mpls_confirm_list_add (struct confirm_list *, struct lsp_dep_confirm *);
int mpls_confirm_list_remove (struct confirm_list *, struct lsp_dep_confirm *);
struct lsp_dep_confirm *mpls_confirm_list_lookup (struct confirm_list *,
						  void *, u_char, bool_t);
int mpls_confirm_list_add_list (struct confirm_list *, struct confirm_list *);
int mpls_confirm_entry_move (struct lsp_dep_confirm *, struct confirm_list *,
                             struct route_table *, int, struct prefix *);
#endif /* _NSM_MPLS_DEP_H */

