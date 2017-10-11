/* Copyright (C) 2004  All Rights Reserved */

#ifndef __NSM_QOS_CLI_H__
#define __NSM_QOS_CLI_H__

#ifdef HAVE_QOS

#define NSM_MLS_STR                 "Multi-Layer Switch(L2/L3)."
#define NSM_QOS_STR                 "Quality of Service."


extern struct map_tbls              default_map_tbls;
extern struct map_tbls              work_map_tbls;
extern struct min_res               min_reserve[MAX_NUM_OF_QUEUE];
extern struct min_res               queue_min_reserve[MAX_NUM_OF_QUEUE];
extern struct nsm_qos_global        qosg;


/* Error codes */
#define NSM_CMAP_QOS_ALGORITHM_STRICT 1
#define NSM_CMAP_QOS_ALGORITHM_DRR 2
#define NSM_CMAP_QOS_ALGORITHM_DRR_STRICT 3

/* Function prototypes. */
void nsm_qos_init(struct nsm_master *nm);
void nsm_qos_deinit(struct nsm_master *nm);
int nsm_qos_config_write (struct cli *cli);
int nsm_qos_cmap_config_write (struct cli *cli);
int nsm_qos_pmap_config_write (struct cli *cli);
int nsm_qos_pmapc_config_write (struct cli *cli);
void nsm_qos_cli_init (struct cli_tree *ctree);
int nsm_qos_if_config_write (struct cli *cli, struct interface *ifp);

#endif /* HAVE_QOS */
#endif /* __NSM_QOS_CLI_H__ */
