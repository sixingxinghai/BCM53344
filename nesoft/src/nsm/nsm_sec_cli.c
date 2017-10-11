#include "pal.h"
#include "lib.h"

#include "avl_tree.h"
#include "nsmd.h"
#include "nsm_message.h"
#include "nsm_server.h"
#include "nsm_interface.h"

#include "nsm_sec_cli.h"
#include "log.h"
#include "cli.h"


CLI(sec_id,
    sec_id_cmd,
    "section ID",
    "configure section id",
   "Indentifying section id"
)
{
	cli_out (cli,"%% mode_sec_by_id received!!\n");
       	return CLI_SUCCESS;

}


CLI(no_sec_id,
    no_sec_id_cmd,
    "no section ID",
    "delete section id",
   "Indentifying section id"
)
{
	cli_out (cli,"%% no_mode_sec_by_id received!!\n");
       	return CLI_SUCCESS;

}




void
nsm_sec_config_write (struct cli *cli)
{
       cli_out (cli,"sec%\n","sec-example");
       cli_out (cli,"sec id %\n","sec-example");


}



void nsm_sec_cli_init(struct cli_tree *ctree)
{
  cli_install_default(ctree, SEC_MODE);

  cli_install(ctree,CONFIG_MODE,&sec_id_cmd);
  cli_install(ctree,CONFIG_MODE,&no_sec_id_cmd);

cli_install(ctree,OAM_1731_MODE,&sec_id_cmd);
cli_install(ctree,OAM_1731_MODE,&no_sec_id_cmd);

  cli_install_config(ctree,MEG_MODE,&nsm_sec_config_write);
}






