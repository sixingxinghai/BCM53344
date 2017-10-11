#include "pal.h"
#include "lib.h"

#include "avl_tree.h"
#include "nsmd.h"
#include "nsm_message.h"
#include "nsm_server.h"
#include "nsm_interface.h"

#include "nsm_oam_1731_cli.h"
#include "log.h"
#include "cli.h"

bool_t oam_flag    = PAL_FALSE;


CLI(oam_1731_mode,
    oam_1731_mode_cmd,
    "mpls-tp oam {y1731-mode |bfd-mode}",
    CLI_OAM_1731_MODE_STR,
   "set oam_1731 mode:y1731-mode or bfd-mode"
)
{
        cli->mode = OAM_1731_MODE;
        cli_out (cli,"%% set oam_1731 mode!!\n");
        return CLI_SUCCESS;

}

CLI(gloal_oam_1731_enable,
    gloal_oam_1731_enable_cmd,
    "oam {enable |disable}",
    "gloal oam enable or disable",
   "set oam enable or disable"
)
{

	if (0 == strcmp("enable",argv[0]))
	{
	oam_flag = 1;
         return CLI_SUCCESS;
        }

	else if (0 == strcmp("disable",argv[0]))
	{
	oam_flag = 0;
         cli_out (cli,"%% oam is disabled !!");
         return CLI_ERROR;
        }


}


CLI(ACH_channel_type_and_mel,
    ACH_channel_type_and_mel_cmd,
    "y1731-channel-type TYPE mel MEL",
    "configure y1731-channel-type:0x0000-0xFFFF",
    "set mel:0-7"
)
{
       if (0 == oam_flag)
        {
		cli_out (cli, "%% oam is disabled !!\n");
 		return CLI_ERROR;
	}
 	else
	{
		char ACH_TYPE = 0x0000;
   		char MEL = 0;
                if((argv[0]<0x0000 || argv[0]> 0xffff )&&(argv[1]<0 || argv[1]> 7))
                 {  ACH_TYPE = 0x0000;
                    MEL = 0;
		    cli_out (cli, "%% input is invalid \n");
		 }
                else
		{
    		ACH_TYPE = argv[0];
                MEL = argv[1];
       		return CLI_SUCCESS;
		}
	}

}


CLI(mac_in_ppp_section,
    mac_in_ppp_section_cmd,
    "section dmac <XXXX.XXXX.XXXX> ",
    "configure section dmac:XXXX.XXXX.XXXX"

)
{
        if (0 == oam_flag)
        {
		cli_out (cli, "%% oam is disabled !! ");
 		return CLI_ERROR;
	}
 	else
	{
		cli_out (cli,"%% XXXX.XXXX.XXXX!!\n");
       		return CLI_SUCCESS;
	}

}


CLI(config_section_mode,
    config_section_mode_cmd,
    "section ID",
   "configure section id"
)
{
 if (0 == oam_flag)
        {
		cli_out (cli, "%% oam is disabled !!\n");
 		return CLI_ERROR;
	}
 	else
	{

       		//if( ( '0' <= argv[0] && argv[0] <= '9') || ( 'a' <= argv[0] && argv[0] <= 'z') || ( 'A' <= argv[0] && argv[0] <= 'Z'))
                 //{
		return CLI_SUCCESS;

		 //}
               // else
		//{
    		//cli_out (cli, "%% input is invalid \n");
		//return CLI_ERROR;
		//}

	}
}


CLI(no_config_section_mode,
    no_config_section_mode_cmd,
    "no section ID",
   "delete section id"
)
{
if (0 == oam_flag)
        {
		cli_out (cli, "%% oam is disabled !!\n");
 		return CLI_ERROR;
	}
 	else
	{
       		return CLI_SUCCESS;
	}
}



CLI(bind_section_tol3p_and_peerip,
    bind_section_tol3p_and_peerip_cmd,
    "local-port NAME [peer-port IP-ADDRESS]",
   "bind section to local-port",
    "set local-port name and peer-port ip-address"
)
{
if (0 == oam_flag)
        {
		cli_out (cli, "%% oam is disabled !!\n");
 		return CLI_ERROR;
	}
 	else
	{
		return CLI_SUCCESS;

	}
}


CLI(no_bind_section_tol3p_and_peerip,
    no_bind_section_tol3p_and_peerip_cmd,
    "no local-port",
   "no bind section to local-port",
    "delete local-port name and peer-port ip-address"
)
{
         if (0 == oam_flag)
        {
		cli_out (cli, "%% oam is disabled !! ");
 		return CLI_ERROR;
	}
 	else
	{
		cli->mode = AC_MODE;
        	cli_out (cli,"%% delete local-port name and peer-port ip-address!!\n");
        	return CLI_SUCCESS;
	}

}


CLI(config_lsp_head_entity,
    config_lsp_head_entity_cmd,
    "tun ID {primary|backup}",
   "configure lsp_head_entity :tun ID and primary or backup"
)
{

	if (0 == oam_flag)
        {
		cli_out (cli, "%% oam is disabled !! ");
 		return CLI_ERROR;
	}
 	else
	{
		cli->mode = TUN_MODE;
        	cli_out (cli,"%% configure section id!!\n");
        	return CLI_SUCCESS;
	}

}


CLI(config_lsp_mip_or_tail_entity,
    config_lsp_mip_or_tail_entity_cmd,
     "tun ID lsp ID Ingress LSRID egress LSRID",
     "configure tun ID",
     "configure lsp ID",
     "configure Ingress LSRID",
     "configure egress LSRID"
)
{

	if (0 == oam_flag)
        {
		cli_out (cli, "%% oam is disabled !! ");
 		return CLI_ERROR;
	}
 	else
	{
		cli->mode = TUN_MODE;
        	cli_out (cli,"%% delete section id!!\n");
        	return CLI_SUCCESS;
	}

}


CLI(config_pw_entity,
    config_pw_entity_cmd,
    "vcid ID peer-ip IPADDR pwtype {vpls|vpws}",
   "configure vcid",
   "configure peer-ip",
    "configure pwtype:vpls or vpws"
)
{
	if (0 == oam_flag)
        {
		cli_out (cli, "%% oam is disabled !! ");
 		return CLI_ERROR;
	}
 	else
	{
		cli->mode = PW_MODE;
        	cli_out (cli,"%% configure pw_entity!!\n");
        	return CLI_SUCCESS;
	}

}



int
nsm_oam_1731_config_write (struct cli *cli)
{
       cli_out (cli,"oam_1731%\n","oam_1731-example");
       cli_out (cli,"oam_1731 MODE %\n","oam_1731-example");


}



void nsm_oam_1731_cli_init(struct cli_tree *ctree)
{
  cli_install_default(ctree, OAM_1731_MODE);
  cli_install(ctree,CONFIG_MODE,&oam_1731_mode_cmd);

cli_install(ctree,OAM_1731_MODE,&gloal_oam_1731_enable_cmd);
cli_install(ctree,OAM_1731_MODE,&ACH_channel_type_and_mel_cmd);
cli_install(ctree,OAM_1731_MODE,&mac_in_ppp_section_cmd);
cli_install(ctree,OAM_1731_MODE,&config_section_mode_cmd);
cli_install(ctree,OAM_1731_MODE,&no_config_section_mode_cmd);

cli_install(ctree,OAM_1731_MODE,&mac_in_ppp_section_cmd);
cli_install(ctree,CONFIG_MODE,&config_section_mode_cmd);
cli_install(ctree,CONFIG_MODE,&no_config_section_mode_cmd);
cli_install(ctree,SEC_MODE,&bind_section_tol3p_and_peerip_cmd);
cli_install(ctree,SEC_MODE,&no_bind_section_tol3p_and_peerip_cmd);

cli_install(ctree,TUN_MODE,&config_lsp_head_entity_cmd);
cli_install(ctree,TUN_MODE,&config_lsp_mip_or_tail_entity_cmd);
cli_install(ctree,PW_MODE,&config_pw_entity_cmd);

cli_install_config(ctree,MEG_MODE,&nsm_oam_1731_config_write);

}








