/******************************************************************************
 *
 *  pac/pac.h
 *
 *****************************************************************************/

/*
 *  Copyright (C) 2012 Ku Ying <PacBridge Corp.>
 *  All rights reserved.
 *
 *  == ËµÃ÷ ==
 *  pacÄ£¿éÍ·ÎÄ¼ş£¬Ìá¹©½Ó¿Ú¹©Íâ²¿Ä£¿éµ÷ÓÃ£¬´´½¨ÓÚ2012.2.16
 *  Ô­ÔòÉÏÖ»ÄÜÌá¹©º¯Êı½Ó¿Ú¡¢Ã¶¾Ù¡¢ÏûÏ¢½Ó¿ *  ²»ÄÜ±©Â©½á¹¹Ìå¡¢Êı¾İ½á¹¹µÈÄÚ²¿ÊµÏÖ¸øÍâ²¿Ä£¿é
 *
 */

#ifndef _PAC_H_
#define _PAC_H_


/******************************************************************************
 *
 *  º¯Êı½Ó¿Ú
 *
 *****************************************************************************/
void vport_master_init();

void ac_cli_init(struct cli_tree *ctree);
void pw_cli_init(struct cli_tree *ctree);
void tun_cli_init(struct cli_tree *ctree);

void service_master_init();
void vpn_cli_init(struct cli_tree *ctree);
void lsp_cli_init(struct cli_tree *ctree);
void lsp_grp_cli_init(struct cli_tree *ctree);
void lsp_oam_cli_init(struct cli_tree *ctree);


void ptp_cli_init(struct cli_tree *ctree);
void ptp_port_cli_init(struct cli_tree *ctree);


#endif
