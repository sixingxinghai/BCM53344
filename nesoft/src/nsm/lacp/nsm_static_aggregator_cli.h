/* Copyright (C) 2005-2006  All Rights Reserved.  */

#ifndef _NSM_STATIC_AGGREGATOR_CLI_H_
#define _NSM_STATIC_AGGREGATOR_CLI_H

#define NSM_STATIC_AGG_COUNT_MAX       8
#define NSM_STATIC_AGG_KEY_MIN         1
#define NSM_STATIC_AGG_KEY_MAX         12

/*Prototype*/
void
nsm_static_aggregator_cli_init (struct cli_tree *ctree);

int
nsm_static_aggregator_if_config_write (struct cli *cli, struct interface *ifp);

void
nsm_send_lacp_agg_count_update ();
#endif /* _NSM_STATIC_AGGREGATOR_CLI_H*/
