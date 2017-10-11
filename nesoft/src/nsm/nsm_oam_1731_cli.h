#ifndef _NSM_OAM_1731_CLI_H_
#define _NSM_OAM_1731_CLI_H_

void nsm_oam_1731_cli_init(struct cli_tree *ctree);


int
nsm_oam_1731_config_write (struct cli *cli);
//int nsm_meg_config_write (struct cli *cli);

//void vport_master_init(struct vport_master *pwg, struct lib_globals *zg);
//int  pwname_cmp (void *v1, void *v2);
//struct Pseudo_Wire *pw_lookup_by_name (struct vport_master *ppwm, char *name);
//void pwg_list_add (struct vport_master *pwg, struct Pseudo_Wire* pwp);



#endif


