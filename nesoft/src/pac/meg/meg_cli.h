#ifndef _NSM_MEG_CLI_H_
#define _NSM_MEG_CLI_H_





void nsm_meg_cli_init(struct cli_tree *ctree);


int  nsm_meg_config_write (struct cli *cli);
//int nsm_meg_config_write (struct cli *cli);

//void meg_master_init(struct meg_master *megg, struct lib_globals *zg);
//int  pwname_cmp (void *v1, void *v2);

struct MEG *meg_lookup_by_id (struct meg_master *megm, int  megid);


void megg_list_add (struct meg_master *megg, struct MEG* megp);


/*Element defined for OAM tools CC/RDI/LB/LT/DM/LM  2011-11-08 by czh*/










#endif


