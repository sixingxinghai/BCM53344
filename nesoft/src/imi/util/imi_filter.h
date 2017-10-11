/* Copyright (C) 2001-2003  All Rights Reserved. */

#ifndef _PACOS_IMI_FILTER_H
#define _PACOS_IMI_FILTER_H

/* Representation of a single chain. */
struct imi_filter_chain
{
  struct list acl_num_list;
};

/* Indexed with "rule->path" :
 * IMI_FILTER_INPUT, IMI_FILTER_OUTPUT, IMI_FILTER_FORWARD.
 */
struct imi_filter_chain filter_chain_tbl [3];

int imi_acl_if_config_write (struct cli *, struct interface *);
result_t imi_filter_acl_ntf_cb (struct access_list *access,
                                struct filter_list *filter,
                                filter_opcode_t     opcode);
void imi_acl_init (struct lib_globals *);
void imi_acl_finish (struct lib_globals *);

void imi_filter_process_if (struct interface *ifp, int opcode);

#endif /* _PACOS_IMI_FILTER_H */
