#include "pal.h"
#include "nsmd.h"
#include "vport.h"
#include "meg.h"
#include "meg_cli.h"

bool_t oam_meg_flag    = PAL_FALSE;

CLI(meg_index,
    meg_index_cmd,
    "meg INDEX",
    CLI_MEG_MODE_STR,
   "Indentifying MEG index"
)
{

	zlog_info (NSM_ZG, "enter the function for meg creat.");
	struct meg_master *megt = &cli->zg->megg;
	zlog_info (NSM_ZG, "call meg_master success.");
	struct MEG *megp; //test 010
	struct vport_pw *pwp ;
    struct vport_tun *tunp;
	int  iTmp = atoi(argv[0]);
	zlog_info (NSM_ZG, "the input is translate to %d \n.",iTmp);
	if ( iTmp <=0 || iTmp > MAX_MEG )
	{

	cli_out (cli, "the meg index is not validate.\n");
	return CLI_ERROR;

	}
	zlog_info (NSM_ZG, "meg index check passed.");


    megp = meg_lookup_by_id ( megt, iTmp);
	zlog_info (NSM_ZG, "meg lookup by id success.");



    if ( NULL == megp )
 	{
 	  cli_out (cli, "%% MEG %s does not exist. It will be created now.\n", argv[0]);
	  megp = XMALLOC (MTYPE_NSM_MEG, sizeof (struct MEG));
      pal_mem_set (megp, 0, sizeof (struct MEG));



	  get_top( &megt->mstack, &(megp->Meg_Index)); //test 005
	  //zlog_info (NSM_ZG, "the meg index index is now %d \n.",megp->Meg_Index); //test 005
	  if ( (megp->Meg_Index > MAX_MEG )| (megp->Meg_Index <= 0 )) //test 005
	  {
	  	cli_out ( cli, "the meg index distribute error.\n");
		return CLI_ERROR;
	  }
	  zlog_info (NSM_ZG, "memory apply success.");


	  pop_stack(&megt->mstack);
	  zlog_info (NSM_ZG, "pop the meg success.");

	  megp->Meg_Index = iTmp;//test 005
	  megp->Meg_Id = 0; //test 005

	  megp->iLocal_Mep_Class = No_Mep_Class;
	  megp->iLocal_Mep_State = Mep_Down;
	  megp->Local_Mep_Id = 0;
	  megp->iPeer_Mep_Class = No_Mep_Class;
	  megp->Peer_Mep_Id = 0;
	  megp->iMip_Enable= Mip_Disable;
	  megp->iMip_State= Mip_Down;
	  megp->Mip_Id= 0;

	  megp->iCc_Enable = Cc_Disable;
	  megp->iSend_Timer = Fast_Default;
	  megp->Loss_Pkt_Thr = 0;
	  megp->iPro_Active_Lm =Lm_Disable;
	  megp->Oam_Toolp=NULL;


	  zlog_info (NSM_ZG, "init meg success ");


	  zlog_info (NSM_ZG, "pointer operation success.");
      megg_list_add(megt,megp);
	  zlog_info (NSM_ZG, "add meg to list success.");
	  zlog_info (NSM_ZG, "new meg index %d has been created.",megp->Meg_Index);

    }


/*decide which mode that the meg belond to and config meg type and binded object_key      add by czh 2011.10.21*/


	 zlog_info(NSM_ZG, "MEG is under mode %d .\n",cli->mode);   //tset 030,decide which mode that the meg belond to.
	 if(cli->mode == 135)
	 	{
	 	zlog_info (NSM_ZG, "MEG is under pw mode %d .\n",cli->mode);
		megp->Meg_Type = PW;
		pwp = cli->index;
		zlog_info (NSM_ZG, "test 001 \n");  //test 2011-001 czh
		int ctmp = pwp->index;
		zlog_info (NSM_ZG, "test 002 \n");  //test 2011-002 czh
		megp->Object_Key = ctmp; //test 010

		zlog_info (NSM_ZG, "MEG is under pw  %d .\n",ctmp);

	 	}
	 if(cli->mode == 139)
	 	{
	 	zlog_info (NSM_ZG, "MEG is under tunnle mode %d .\n",cli->mode);
		megp->Meg_Type = LSP;
		tunp = cli->index;
		zlog_info (NSM_ZG, "test 003 \n");  //test 2011-003 czh
		int ctmp = pwp->index;
		zlog_info (NSM_ZG, "test 006 \n");  //test 2011-006 czh
		megp->Object_Key = ctmp; //test 010
		zlog_info (NSM_ZG, "MEG is under tunnle  %d .\n",tunp->index);

	 	}
	 zlog_info (NSM_ZG, "new meg index %d has been created.",megp->Meg_Index);


	 cli->index = megp;
	 cli->mode = MEG_MODE;

	  return CLI_SUCCESS;

}




CLI(no_meg_index,
    no_meg_index_cmd,
    "no meg INDEX",
    CLI_MEG_MODE_STR,
   "Indentifying MEG index"
   )
{

	struct meg_master *megt = &cli->zg->megg;
	struct MEG *megp;
	int  iTmp = atoi(argv[0]);
	zlog_info (NSM_ZG, "the input is translate to %d . \n",iTmp);


	if ( iTmp <=0 || iTmp > MAX_MEG )
	{

	cli_out (cli, "the meg index is not validate.\n");
	return CLI_ERROR;

	}
	zlog_info (NSM_ZG, "meg index check passed.");

	megp = meg_lookup_by_id ( megt, iTmp);
	zlog_info (NSM_ZG, "meg lookup by id success.");
 	if (megp == NULL)
 	{
    		cli_out (cli, "MEG index %s does not exist\n", argv[0]);
    		return CLI_ERROR;
  	}

	if ( NULL != megp->next || NULL != megp->prev )
  	{
  		if ( NULL != megp->next )
			megp->next->prev = megp->prev;
		if ( NULL != megp->prev )
			megp->prev->next = megp->next;

		return CLI_SUCCESS;
  	}
	zlog_info (NSM_ZG, "find the meg memory success.");


	  megp->Meg_Index = 0; //test 005
	  megp->Meg_Id = 0;
	  megp->Meg_Type = NOMEGTYPE;
	  megp->iLocal_Mep_Class = No_Mep_Class;
	  megp->iLocal_Mep_State = Mep_Down;
	  megp->Local_Mep_Id = 0;
	  megp->iPeer_Mep_Class = No_Mep_Class;
	  megp->Peer_Mep_Id = 0;
	  megp->iMip_Enable = Mip_Disable;
	  megp->iMip_State = Mip_Down;
	  megp->Mip_Id = 0;
	  megp->Object_Key = 0;
	  megp->iCc_Enable = Cc_Disable;
	  megp->iSend_Timer = Fast_Default;
	  megp->Loss_Pkt_Thr = 0;
	  megp->iPro_Active_Lm =Lm_Disable;
	  zlog_info (NSM_ZG, "config is set to init state");
	  megp->next = megp->prev = NULL;
	  megp->Oam_Toolp=NULL;
	  zlog_info (NSM_ZG, "pointer operation success.");

	push_stack(&megt->mstack,megp->Meg_Index);
	zlog_info (NSM_ZG, "push stack operation success.");
	listnode_delete (megt->meg_list, megp);
	zlog_info (NSM_ZG, "delete list success.");
	XFREE (MTYPE_NSM_MEG, megp);
    zlog_info (NSM_ZG, "release memory success");
    megt->megTblLastChange = pal_time_current (NULL);


	zlog_info (NSM_ZG, "the meg %d has been deleted.",iTmp);

	 cli->index = megp; //test 006
	 cli->mode = MEG_MODE;//test 006

   return CLI_SUCCESS;

}




CLI(meg_id,
    meg_id_cmd,
    "id ID",
    "configure meg id",
   "Indentifying MEG id"
)
{

  struct meg_master *megt = &cli->zg->megg; //test 010

  zlog_info (NSM_ZG, "call meg_master success \n"); //test 010
  struct MEG *megp;
  struct MEG *mtmp;
  mtmp = (struct MEG *)cli ->index;
  int iTmp = atoi(argv[0]);
  zlog_info (NSM_ZG, "the input is translated to %d \n",iTmp);

  if ( iTmp <=0 || iTmp > MAX_MEG )
	{

	cli_out (cli, "the meg id is not validate.\n");
	return CLI_ERROR;

	}
   zlog_info (NSM_ZG, "meg id check passed \n"); //test 010



   megp = meg_lookup_by_id ( megt, mtmp->Meg_Index);   //test 010
   zlog_info (NSM_ZG, "meg lookup by index success and id is now %d \n",megp->Meg_Id); //test 010
   zlog_info (NSM_ZG, "meg index is %d	\n",mtmp->Meg_Index); //test 010

  	if(0 != megp->Meg_Index)		 	 //test 010
  	{
  	megp->Meg_Id= iTmp;

	cli_out (cli, "the meg id is set to %d .\n",megp->Meg_Id);//test 005

	zlog_info (NSM_ZG, "the MEG  ID  is set  to %d \n.",megp->Meg_Id);	//test005

	return CLI_SUCCESS;   //test 010


	}

   else
      return CLI_SUCCESS;



} //config meg id  czh




CLI(no_meg_id,
    no_meg_id_cmd,
    "no id ID",
    "delete meg id",
   "Indentifying MEG id"
)
{
	  struct meg_master *megt = &cli->zg->megg;
	  zlog_info (NSM_ZG, "call meg_master success \n"); //test 010
	  struct MEG *megp;
	  struct MEG *mtmp;
	  mtmp = (struct MEG *)cli ->index;
	  int iTmp = atoi(argv[0]);
	  zlog_info (NSM_ZG, "the input is translated to %d.  \n",iTmp);
	  if ( iTmp <=0 || iTmp > MAX_MEG )
		{

		cli_out (cli, "the meg id is not validate.\n");
		return CLI_ERROR;

		}
	  zlog_info (NSM_ZG, "meg id check passed \n"); //test 010


	   megp = meg_lookup_by_id ( megt, mtmp->Meg_Index);	 //test 010
	   zlog_info (NSM_ZG, "meg lookup by index success and id is now %d \n",megp->Meg_Id); //test 010
       zlog_info (NSM_ZG, "meg index is %d	\n",mtmp->Meg_Index); //test 010
	   zlog_info (NSM_ZG, "meg lookup by index success"); //test 005
	  //megp = meg_lookup_by_id ( megt, iTmp); // test 010
	  if (megp->Meg_Id == 0)
	  	{
		  cli_out (cli, "the meg id does not exit.\n");
		  return CLI_ERROR;

	  	}


	   else
		    megp->Meg_Id = 0;


		cli_out (cli, "the meg id is deleted .\n");
		zlog_info (NSM_ZG, "delete meg id success.");	//test005
		return CLI_SUCCESS;


} // delete MEG ID


CLI(oam_enable,
    oam_enable_cmd,
    "oam {enable | disable }",
    "oam enable or disable ",
   "oam enable or disable"
)
{
	if (0 == strcmp("enable",argv[0]))
	{
	      oam_meg_flag = PAL_TRUE;
	  	  struct meg_master *megt = &cli->zg->megg;
		  zlog_info (NSM_ZG, "call meg master success  \n.");  //test 014
		  struct MEG *megp;
		  struct MEG *mtmp;
		  mtmp = (struct MEG *)cli->index;
		  megp = meg_lookup_by_id ( megt, mtmp->Meg_Index);//test 002

		  struct OAM_TOOL *oamp;
          megp->Oam_Toolp=oamp;


	  megp->Oam_Toolp= XMALLOC (MTYPE_NSM_OAM, sizeof (struct OAM_TOOL));   //test 2011.11.11
	  pal_mem_set (megp->Oam_Toolp, 0, sizeof (struct OAM_TOOL));


	  megp->Oam_Toolp->Oam_Tool_Ccp= XMALLOC (MTYPE_NSM_OAM, sizeof (struct OAM_TOOL_CC));   //test 2011.11.11
	  pal_mem_set (megp->Oam_Toolp->Oam_Tool_Ccp, 0, sizeof (struct OAM_TOOL_CC));
	  megp->Oam_Toolp->Oam_Tool_Lbp= XMALLOC (MTYPE_NSM_OAM, sizeof (struct OAM_TOOL_LB));   //test 2011.11.11
	  pal_mem_set (megp->Oam_Toolp->Oam_Tool_Lbp, 0, sizeof (struct OAM_TOOL_LB));
	  megp->Oam_Toolp->Oam_Tool_Ltp= XMALLOC (MTYPE_NSM_OAM, sizeof (struct OAM_TOOL_LT));   //test 2011.11.11
	  pal_mem_set (megp->Oam_Toolp->Oam_Tool_Ltp, 0, sizeof (struct OAM_TOOL_LT));
	  megp->Oam_Toolp->Oam_Tool_Lmp= XMALLOC (MTYPE_NSM_OAM, sizeof (struct OAM_TOOL_LM));   //test 2011.11.11
	  pal_mem_set (megp->Oam_Toolp->Oam_Tool_Lmp, 0, sizeof (struct OAM_TOOL_LM));
	  megp->Oam_Toolp->Oam_Tool_Dmp= XMALLOC (MTYPE_NSM_OAM, sizeof (struct OAM_TOOL_DM));   //test 2011.11.11
	  pal_mem_set (megp->Oam_Toolp->Oam_Tool_Dmp, 0, sizeof (struct OAM_TOOL_DM));




	  megp->Oam_Toolp->Oam_Tool_Lbp->Ttl=255;
	  megp->Oam_Toolp->Oam_Tool_Lbp->Repeat_Num=3;
	  megp->Oam_Toolp->Oam_Tool_Lbp->Packet_Size=0;
	  megp->Oam_Toolp->Oam_Tool_Lbp->Time_Out=5;


	  megp->Oam_Toolp->Oam_Tool_Ltp->Ttl=30;
	  megp->Oam_Toolp->Oam_Tool_Ltp->Packet_Size=0;
	  megp->Oam_Toolp->Oam_Tool_Ltp->Time_Out=5;



	  megp->Oam_Toolp->Oam_Tool_Lmp->Repeat_Num=3;
	  megp->Oam_Toolp->Oam_Tool_Lmp->Lm_Period=1;



	  megp->Oam_Toolp->Oam_Tool_Dmp->Repeat_Num=3;
	  megp->Oam_Toolp->Oam_Tool_Dmp->Dm_Period=1;
         return CLI_SUCCESS;
        }

	else if (0 == strcmp("disable",argv[0]))
	{
	oam_meg_flag = 	PAL_FALSE;
         cli_out (cli,"%% oam is disabled !!");
         return CLI_ERROR;
        }

}




CLI(creat_local_mep,
    creat_local_mep_cmd,
    "local-mep ID type {source | destinnation |bidirectional }",
    "local-mep id,type,source , destinnation or bidirectional ",
    "local mep id,and type:source , destinnation or bidirectional"
)
{
    zlog_info (NSM_ZG, "enter the function for local mep creat  \n.");  //test 014



    struct meg_master *megt = &cli->zg->megg;
	zlog_info (NSM_ZG, "call meg master success  \n.");  //test 014
    struct MEG *megp;
    struct MEG *mtmp;
    mtmp = (struct MEG *)cli->index;
	int iTmp = atoi(argv[0]); //test 011

	zlog_info (NSM_ZG, "input is translated to  %d. \n",iTmp);  //test 017

	if ( iTmp <=0 || iTmp > MAX_MEG )
		{

		cli_out (cli, "the mep id is not validate.\n");
		return CLI_ERROR;

		}   //test 021
	  zlog_info (NSM_ZG, "meg id check passed \n"); //test 010
	//need to make the input is valide first

    megp = meg_lookup_by_id ( megt, mtmp->Meg_Index);//test 002
    zlog_info (NSM_ZG, "meg look up success  and meg index is now %d. \n",megp->Meg_Index);  //test 014
    zlog_info (NSM_ZG, "meg look up success  \n.");  //test 014

   zlog_info (NSM_ZG, "oam flag check and flag is now %d.\n",oam_meg_flag); //test 015

	if (0 == oam_meg_flag)
    {
		cli_out (cli, "%% oam is disabled !! ");
 		return CLI_ERROR;
    }


 	else
	{
	cli_out (cli,"%% creat local mep!!\n");


    if( megp->Meg_Index > 0) //test 016
    {

 		megp->Local_Mep_Id = iTmp; //test 019
 		zlog_info (NSM_ZG, "the input iTmp is now %d . \n",iTmp);//test 020
		zlog_info (NSM_ZG, "local MEP ID is set to  %d \n.",megp->Local_Mep_Id);//test 014
		cli_out (cli, "local mep id is set to %d  !!\n ",iTmp); //test 018
		//zlog_info (NSM_ZG, "the meg %s local MEP ID is set to  %s.",megp->Meg_Id,argv[0]);//test 014

		megp->iLocal_Mep_State = Mep_Up;
		zlog_info (NSM_ZG, "the local MEP sate is up."); //test 014
		if ( 0 == pal_strcmp ( "source", argv[1] ))  //test 021
			{
				megp->iLocal_Mep_Class = SOURCE;
				cli_out (cli, "local mep class is  set to %s  !! \n ",argv[1]); //test 014
				zlog_info (NSM_ZG, "the local MEP class has been set to source  %d  . \n	",megp->iLocal_Mep_Class); //test 014
				return CLI_SUCCESS;
			}
			if ( 0 == pal_strcmp ( "destinnation", argv[1] ))  //test 021
			{
				megp->iLocal_Mep_Class = DEST;
				cli_out (cli, "local mep class is  set to %s  !! \n ",argv[1]); //test 014
				zlog_info (NSM_ZG, "the local MEP class has been set to destinnation %d	.\n",megp->iLocal_Mep_Class); //test 014
				return CLI_SUCCESS;
			}
		  if ( 0 == pal_strcmp ( "bidirectional", argv[1] ))  //test 021
			{
				megp->iLocal_Mep_Class = BIDI;
				cli_out (cli, "local mep class is  set to %s  !! \n ",argv[1]); //test 014
				zlog_info (NSM_ZG, "the local MEP class has been set to bidirectional %d. \n",megp->iLocal_Mep_Class); //test 014
				return CLI_SUCCESS;
			}


		       return CLI_SUCCESS;
    	}
}
}



CLI(no_creat_local_mep,
    no_creat_local_mep_cmd,
    "no local-mep ID",
    "delete local-mep ID ",
    "no creat local mep"
)
{
	   zlog_info (NSM_ZG, "enter the function for local mep delete	\n.");	//test 014



		struct meg_master *megt = &cli->zg->megg;
		zlog_info (NSM_ZG, "call meg master success  \n.");  //test 014
		struct MEG *megp;
		struct MEG *mtmp;
		mtmp = (struct MEG *)cli->index;
		int iTmp = atoi(argv[0]); //test 011

		zlog_info (NSM_ZG, "input is translated to	%d. \n",iTmp);	//test 017

		if ( iTmp <=0 || iTmp > MAX_MEG )
			{

			cli_out (cli, "the mep id is not validate.\n");
			return CLI_ERROR;

			}	//test 021
		  zlog_info (NSM_ZG, "meg id check passed \n"); //test 010
		//need to make the input is valide first

		megp = meg_lookup_by_id ( megt, mtmp->Meg_Index);//test 002
		zlog_info (NSM_ZG, "meg look up success  and meg index is now %d. \n",megp->Meg_Index);  //test 014
		zlog_info (NSM_ZG, "meg look up success  \n.");  //test 014


	if (0 == oam_meg_flag)
        {
		cli_out (cli, "%% oam is disabled !! ");
 		return CLI_ERROR;
	}
 	else
	{
	cli_out (cli,"%%no creat local mep id!!\n");


    if(megp ->Local_Mep_Id == 0)
    	{

		 cli_out (cli, "local mep id  does not exit.\n");
		  return CLI_ERROR;
    	}
	else
		{
		megp->Local_Mep_Id = 0;
		zlog_info (NSM_ZG, "local mep ID %d has  been delete  .\n",iTmp);
		megp->iLocal_Mep_Class = No_Mep_Class;
		zlog_info (NSM_ZG, "local mep class is set to no_mep_class  %d .\n",megp->iLocal_Mep_Class);

 	    megp->iLocal_Mep_State = Mep_Down;
 	    zlog_info (NSM_ZG, "the local MEP sate is down %d. \n",megp->iLocal_Mep_State);
		return CLI_SUCCESS;
		}
 	}


}



CLI(creat_peer_mep,
    creat_peer_mep_cmd,
    "peer-mep ID type{source | destinnation |bidirectional }",
    "configure peer-mep mep id ,and the type source ,destinnation or bidirectional }",
    "creat peer mep"
)
{
	 zlog_info (NSM_ZG, "enter the function for local mep creat  \n.");  //test 014



	 struct meg_master *megt = &cli->zg->megg;
	 zlog_info (NSM_ZG, "call meg master success  \n.");  //test 014
	 struct MEG *megp;
	 struct MEG *mtmp;
	 mtmp = (struct MEG *)cli->index;
	 int iTmp = atoi(argv[0]); //test 011

	 zlog_info (NSM_ZG, "input is translated to  %d. \n",iTmp);  //test 017

	 if ( iTmp <=0 || iTmp > MAX_MEG )
		 {

		 cli_out (cli, "the mep id is not validate.\n");
		 return CLI_ERROR;

		 }	 //test 021
	   zlog_info (NSM_ZG, "meg id check passed \n"); //test 010
	 //need to make the input is valide first

	 megp = meg_lookup_by_id ( megt, mtmp->Meg_Index);//test 002
	 zlog_info (NSM_ZG, "meg look up success  and meg index is now %d. \n",megp->Meg_Index);  //test 014
	 zlog_info (NSM_ZG, "meg look up success  \n.");  //test 014

	zlog_info (NSM_ZG, "oam flag check and flag is now %d.\n",oam_meg_flag); //test 015

	 if (0 == oam_meg_flag)
	 {
		 cli_out (cli, "%% oam is disabled !! ");
		 return CLI_ERROR;
	 }


	 else
	 {
	 cli_out (cli,"%% creat local mep!!\n");


	 if( megp->Meg_Index > 0) //test 016
	 {

		 megp->Peer_Mep_Id = iTmp; //test 019
		 zlog_info (NSM_ZG, "the input iTmp is now %d . \n",iTmp);//test 020
		 zlog_info (NSM_ZG, "Peer MEP ID is set to  %d \n.",megp->Peer_Mep_Id);//test 014
		 cli_out (cli, "Peer mep id is set to %d  !!\n ",iTmp); //test 018

		 if ( 0 == pal_strcmp ( "source", argv[1] ))  //test 021
			 {
				 megp->iPeer_Mep_Class = SOURCE;
				 cli_out (cli, "Peer mep class is	set to %s  !! \n ",argv[1]); //test 014
				 zlog_info (NSM_ZG, "the Peer MEP class has been set to source  %d  . \n	 ",megp->iPeer_Mep_Class); //test 014
				 return CLI_SUCCESS;
			 }
			 if ( 0 == pal_strcmp ( "destinnation", argv[1] ))	//test 021
			 {
				 megp->iPeer_Mep_Class = DEST;
				 cli_out (cli, "Peer mep class is	set to %s  !! \n ",argv[1]); //test 014
				 zlog_info (NSM_ZG, "the Peerl MEP class has been set to destinnation %d .\n",megp->iPeer_Mep_Class); //test 014
				 return CLI_SUCCESS;
			 }
		   if ( 0 == pal_strcmp ( "bidirectional", argv[1] ))  //test 021
			 {
				 megp->iPeer_Mep_Class = BIDI;
				 cli_out (cli, "Peer mep class is	set to %s  !! \n ",argv[1]); //test 014
				 zlog_info (NSM_ZG, "the local MEP class has been set to bidirectional %d. \n",megp->iPeer_Mep_Class); //test 014
				 return CLI_SUCCESS;
			 }


				return CLI_SUCCESS;
		 }


	 	}


} // creat peer MEP


CLI(no_creat_peer_mep,
    no_creat_peer_mep_cmd,
    "no peer-mep ID",
    "delete peer-mep ID ",
    "no creat peer mep"
)
{
	       zlog_info (NSM_ZG, "enter the function for local mep delete \n.");	//test 014



			struct meg_master *megt = &cli->zg->megg;
			zlog_info (NSM_ZG, "call meg master success  \n.");  //test 014
			struct MEG *megp;
			struct MEG *mtmp;
			mtmp = (struct MEG *)cli->index;
			int iTmp = atoi(argv[0]); //test 011

			zlog_info (NSM_ZG, "input is translated to	%d. \n",iTmp);	//test 017

			if ( iTmp <=0 || iTmp > MAX_MEG )
				{

				cli_out (cli, "the mep id is not validate.\n");
				return CLI_ERROR;

				}	//test 021
			  zlog_info (NSM_ZG, "meg id check passed \n"); //test 010
			//need to make the input is valide first

			megp = meg_lookup_by_id ( megt, mtmp->Meg_Index);//test 002
			zlog_info (NSM_ZG, "meg look up success  and meg index is now %d. \n",megp->Meg_Index);  //test 014
			zlog_info (NSM_ZG, "meg look up success  \n.");  //test 014


		if (0 == oam_meg_flag)
			{
			cli_out (cli, "%% oam is disabled !! ");
			return CLI_ERROR;
		}
		else
		{
		cli_out (cli,"%%no creat local mep id!!\n");




		if(megp ->Peer_Mep_Id == 0)
			{

			 cli_out (cli, "peer mep id  does not exit.\n");
			  return CLI_ERROR;
			}
		else
			{
			megp->Peer_Mep_Id = 0;
			zlog_info (NSM_ZG, "Peer mep ID %d  has  been delete  .\n",iTmp);
			megp->iPeer_Mep_Class = No_Mep_Class;
			zlog_info (NSM_ZG, "Peer mep class is set to no_mep_class	%d .\n",megp->iPeer_Mep_Class);


			return CLI_SUCCESS;
			}

		}


}  // create MIP



CLI(cc_detect,
    cc_detect_cmd,
    "cc {enable |disable}",
    "cc detect:enable or disable ",
    "cc dection configure"
)
{
	if (0 == oam_meg_flag)
        {
		cli_out (cli, "%% oam is disabled !! ");
 		return CLI_ERROR;
	}
 	else
	{
		   cli_out (cli,"%% cc dection!!\n");     //test 032

	       zlog_info (NSM_ZG, "enter the function for cc enable  \n.");	//test 014

		    struct meg_master *megt = &cli->zg->megg;
			zlog_info (NSM_ZG, "call meg master success  \n.");  //test 014
			struct MEG *megp;
			struct MEG *mtmp;
			mtmp = (struct MEG *)cli->index;
			int iTmp = atoi(argv[0]); //test 011



            megp = meg_lookup_by_id ( megt, mtmp->Meg_Index);//test 002
			zlog_info (NSM_ZG, "meg look up success  and meg index is now %d. \n",megp->Meg_Index);  //test 014
			zlog_info (NSM_ZG, "meg look up success  \n.");  //test 014
    if(megp->Meg_Index > 0)
    {

       if ( 0 == pal_strcmp ( "enable", argv[0] ))
 	    {
 		megp->iCc_Enable = Cc_Enable;
		zlog_info (NSM_ZG, "CC is enble %d . \n",megp->iCc_Enable);
		return CLI_SUCCESS;
 	    }
 	  if ( 0 == pal_strcmp ( "disable", argv[0] ))
 	   {
 		megp->iCc_Enable = Cc_Disable;
		zlog_info (NSM_ZG, "CC is disble %d . \n",megp->iCc_Enable);
		return CLI_SUCCESS;
 	   }

	}

 		}
}


CLI(creat_mip,
    creat_mip_cmd,
    "mip ID [interface NAME]",
    "configure mip ID and interface port-name ",
    "creat mip"
)
{
	if (0 == oam_meg_flag)
        {
		cli_out (cli, "%% oam is disabled !! ");
 		return CLI_ERROR;
	}
 	else
	{
		cli_out (cli,"%% creat mip!!\n");
		}

	        zlog_info (NSM_ZG, "enter the function for mip create \n.");	//test 014

		    struct meg_master *megt = &cli->zg->megg;
			zlog_info (NSM_ZG, "call meg master success  \n.");  //test 014
			struct MEG *megp;
			struct MEG *mtmp;
			mtmp = (struct MEG *)cli->index;
			int iTmp = atoi(argv[0]); //test 011


			if ( iTmp <=0 || iTmp > MAX_MEG )
				{

				cli_out (cli, "the mep id is not validate.\n");
				return CLI_ERROR;

				}	//test 021



            megp = meg_lookup_by_id ( megt, mtmp->Meg_Index);//test 002
			zlog_info (NSM_ZG, "meg look up success  and meg index is now %d. \n",megp->Meg_Index);  //test 014
			zlog_info (NSM_ZG, "meg look up success  \n.");  //test 014


	   if((megp->Local_Mep_Id >0) ||( megp->Peer_Mep_Id > 0))
	   	{
	   	cli_out (cli, "can't config to mip because mep config already exit .\n");
		zlog_info (NSM_ZG, "check mep config for mip config \n");
		return CLI_ERROR;
	   	}
        else
			{
    	   megp->Mip_Id = iTmp;
		   zlog_info (NSM_ZG, "the  MIP ID is set to  %d.  \n",megp->Mip_Id);

 		   megp->iMip_Enable = Mip_Enable;
		   zlog_info (NSM_ZG, "mip is enble %d . \n",megp->iMip_Enable);

 	       //megp->iCc_Enable =  Mip_Disable;
		   //zlog_info (NSM_ZG, "mip is disble %d . \n",megp->iMip_Enable);
		  return CLI_SUCCESS;
 	        }


}




CLI(cc_packet_send_period,
    cc_packet_send_period_cmd,
    "cc period {default |10ms |100ms |1s |10s |1min |10min }",
    "cc period configure ",
    "cc period : 3.33ms ,10ms ,100ms ,1s ,10s ,1min ,10min "
)
{
	if (PAL_FALSE== oam_meg_flag)
        {
		cli_out (cli, "%% oam is disabled !! ");
 		return CLI_ERROR;
	}
 	else
	{

	zlog_info (NSM_ZG, "enter the function for cc perriod config  \n.");	//test 014

	struct meg_master *megt = &cli->zg->megg;
	zlog_info (NSM_ZG, "call meg master success  \n.");  //test 014
	struct MEG *megp;
	struct MEG *mtmp;
	mtmp = (struct MEG *)cli->index;
	//int iTmp = atoi(argv[0]); //test 011

	megp = meg_lookup_by_id ( megt, mtmp->Meg_Index);//test 002
	zlog_info (NSM_ZG, "meg look up success  and meg index is now %d. \n",megp->Meg_Index);  //test 014
	zlog_info (NSM_ZG, "meg look up success  \n.");  //test 014   test 036

   if(megp->Local_Mep_Id == 0 || megp->Peer_Mep_Id == 0)
   	{
   	    cli_out (cli, "%% mep is not config complete,can't config cc period !!");
        zlog_info (NSM_ZG, "mep is not config complete,can't config cc period \n");

				   return CLI_ERROR;

   	}
    else

	     if(megp->Local_Mep_Id >0 && megp->Peer_Mep_Id > 0)

	{

      if ( 0 == pal_strcmp ( "default", argv[0] ))
 	{
 		megp->iSend_Timer = Fast_Default;
		zlog_info (NSM_ZG, " cc period is configured to 3.33ms %d. \n",megp->iSend_Timer);
		return CLI_SUCCESS;
 	}
 	   if ( 0 == pal_strcmp ( "10ms", argv[0] ))
 	{
 		megp->iSend_Timer = Fast_10_Ms;
		zlog_info (NSM_ZG, " cc period is configured to 10ms %d. \n",megp->iSend_Timer);
		return CLI_SUCCESS;
 	}
      if ( 0 == pal_strcmp ( "100ms", argv[0] ))
 	{
 		megp->iSend_Timer = Fast_100_Ms;
		zlog_info (NSM_ZG, " cc period is configured to 100ms %d. \n",megp->iSend_Timer);
		return CLI_SUCCESS;
 	}
 	if ( 0 == pal_strcmp ( "1s", argv[0] ))
 	{
 		megp->iSend_Timer = Fast_1_S;
		zlog_info (NSM_ZG, " cc period is configured to 1s %d. \n",megp->iSend_Timer);
		return CLI_SUCCESS;
 	}
 	if ( 0 == pal_strcmp ( "10s", argv[0] ))
 	{
 		megp->iSend_Timer = Fast_10_S;
		zlog_info (NSM_ZG, " cc period is configured to 10s %d. \n",megp->iSend_Timer);
		return CLI_SUCCESS;
 	}
  if ( 0 == pal_strcmp ( "1min", argv[0] ))
 	{
 		megp->iSend_Timer = Fast_1_M;
		zlog_info (NSM_ZG, " cc period is configured to 1min %d. \n",megp->iSend_Timer);
		return CLI_SUCCESS;
 	}
  if ( 0 == pal_strcmp ( "10min", argv[0] ))
 	{
 		megp->iSend_Timer = Fast_10_M;
		zlog_info (NSM_ZG, " cc period is configured to 10min %d. \n",megp->iSend_Timer);
		return CLI_SUCCESS;
 	}
    }

	return CLI_SUCCESS;	   //test 035


    }
}

CLI(lb_detect_enable,
    lb_detect_enable_cmd,
    "lb {enable|disable} ",
    "lb configure",
    "lb enable configure:enable or disable"
)
{
			 	if (0 == oam_meg_flag)
        {
		cli_out (cli, "%% oam is disabled !! ");
 		return CLI_ERROR;
	}
 	else
	{
		          struct meg_master *megt = &cli->zg->megg;
			   zlog_info (NSM_ZG, "call meg master success	\n.");	//test 014
			   struct MEG *megp;
			   struct MEG *mtmp;
			   mtmp = (struct MEG *)cli->index;
			   int iTtl= atoi(argv[0]);

			   megp = meg_lookup_by_id ( megt, mtmp->Meg_Index);//test 002

			   if ( 0 == pal_strcmp ( "enable", argv[0] ))
			   	{

/****************************************************************************************

					  call for oam driver  for oam message delive

*****************************************************************************************/
	                 return CLI_SUCCESS;

			    }


			   if ( 0 == pal_strcmp ( "disable", argv[0] ))
			   	{

/****************************************************************************************

					  call for oam driver  for oam message delive

*****************************************************************************************/

                 return CLI_SUCCESS;

			   }

 	}


}






CLI(lb_detect_ttl,
    lb_detect_ttl_cmd,
    "lb ttl <1-255> ",
    "lb configure",
    "lb ttl configure:ttl 1-255,default is 255"
)
{
			   struct meg_master *megt = &cli->zg->megg;
			   zlog_info (NSM_ZG, "call meg master success	\n.");	//test 014
			   struct MEG *megp;
			   struct MEG *mtmp;
			   mtmp = (struct MEG *)cli->index;
			   int iTtl= atoi(argv[0]);

			   megp = meg_lookup_by_id ( megt, mtmp->Meg_Index);//test 002

			   if((iTtl<1)||(iTtl>255))
			   {
				cli_out (cli, "%% Invalid value !! TTL should be 1-255 !!");
				return CLI_ERROR;
			   }
			   else
				   {
				   		megp->Oam_Toolp->Oam_Tool_Lbp->Ttl=iTtl;
				   		cli_out (cli, "%% ttl %d \n !!",iTtl);
				   		return CLI_SUCCESS;
				   }


}


CLI(lb_detect_repeat,
    lb_detect_repeat_cmd,
    "lb repeat <1-200> ",
    "lb configure ",
    "lb repeat configure:repeat 1-200,default is 3 ."
)
{
	struct meg_master *megt = &cli->zg->megg;
	zlog_info (NSM_ZG, "call meg master success  \n.");  //test 014
	struct MEG *megp;
	struct MEG *mtmp;
	mtmp = (struct MEG *)cli->index;
	int iRepeat= atoi(argv[0]);

	megp = meg_lookup_by_id ( megt, mtmp->Meg_Index);//test 002



	if((iRepeat>=1)&&(iRepeat<=200))
	{
		megp->Oam_Toolp->Oam_Tool_Lbp->Repeat_Num=iRepeat;
		cli_out (cli, "%% repeat %d \n !!",iRepeat);

	return CLI_SUCCESS;
	}




}


CLI(lb_detect_size,
    lb_detect_size_cmd,
    "lb size <1-400> ",
    "lb configure",
    "lb packet size configure:repeat 1-400,default is 0"
)
{
	struct meg_master *megt = &cli->zg->megg;
	zlog_info (NSM_ZG, "call meg master success  \n.");  //test 014
	struct MEG *megp;
	struct MEG *mtmp;
	mtmp = (struct MEG *)cli->index;
	int iSize= atoi(argv[0]);
	megp = meg_lookup_by_id ( megt, mtmp->Meg_Index);//test 002




	if((iSize>=1)&&(iSize<=400))
	{
	megp->Oam_Toolp->Oam_Tool_Lbp->Packet_Size=iSize;
	cli_out (cli, "%% repeat %d \n !!",iSize);

		return CLI_SUCCESS;
	}


}


CLI(lb_detect_timeout,
    lb_detect_timeout_cmd,
    "lb timeout <1-10> ",
    "lb configure ",
    "lb timeout configure:timeout 1-10,default is 5s"
)
{
	struct meg_master *megt = &cli->zg->megg;
	zlog_info (NSM_ZG, "call meg master success  \n.");  //test 014
	struct MEG *megp;
	struct MEG *mtmp;
	mtmp = (struct MEG *)cli->index;
	int iTimeout =atoi(argv[0]);
	megp = meg_lookup_by_id ( megt, mtmp->Meg_Index);//test 002



	if((iTimeout>=1)&&(iTimeout<=10))
	{
	megp->Oam_Toolp->Oam_Tool_Lbp->Time_Out=iTimeout;
	cli_out (cli, "%% repeat %d \n !!",iTimeout);

	return CLI_SUCCESS;
	}

}






/*CLI(lb_detect,
    lb_detect_cmd,
    "lb [ttl<1-255>  repeat<1-200>  size<1-400>  timeout<1-10>]{enable|disable}",
    "lb configure:ttl 1-255 ",
    "lb configure:repeat 1-200 ",
    "lb configure:size 1-400 ",
    "lb configure:timeout 1-10 "

)
{
	if (0 == oam_meg_flag)
        {
		cli_out (cli, "%% oam is disabled !! ");
 		return CLI_ERROR;
	}
 	else
	{
		    struct meg_master *megt = &cli->zg->megg;
			zlog_info (NSM_ZG, "call meg master success  \n.");  //test 014
			struct MEG *megp;
			struct MEG *mtmp;
			mtmp = (struct MEG *)cli->index;
			int iTtl= atoi(argv[0]);
			int iRepeat= atoi(argv[1]);
			int iSize= atoi(argv[2]);
			int iTimeout =atoi(argv[3]);
			megp = meg_lookup_by_id ( megt, mtmp->Meg_Index);//test 002

			megp->Oam_Toolp->Oam_Tool_Lbp->Ttl=255;
			megp->Oam_Toolp->Oam_Tool_Lbp->Repeat_Num=3;
			megp->Oam_Toolp->Oam_Tool_Lbp->Packet_Size=0;
			megp->Oam_Toolp->Oam_Tool_Lbp->Time_Out=5;

			if((iTtl<1)||(iTtl>255))
			{
             cli_out (cli, "%% Invalid value !! TTL should be 1-255 !!");
			 return CLI_ERROR;
			}
			else
				{
				megp->Oam_Toolp->Oam_Tool_Lbp->Ttl=iTtl;
				cli_out (cli, "%% ttl %d \n !!",iTtl);
				}
			if((iRepeat>=1)&&(iRepeat<=200))
			{
			    megp->Oam_Toolp->Oam_Tool_Lbp->Repeat_Num=iRepeat;
				cli_out (cli, "%% repeat %d \n !!",iRepeat);


			}

			if((iSize>=1)&&(iSize<=400))
			{
			megp->Oam_Toolp->Oam_Tool_Lbp->Packet_Size=iSize;
			cli_out (cli, "%% repeat %d \n !!",iSize);

			}

			if((iTimeout>=1)&&(iTimeout<=10))
			{
			megp->Oam_Toolp->Oam_Tool_Lbp->Time_Out=iTimeout;
			cli_out (cli, "%% repeat %d \n !!",iTimeout);

			}


	return CLI_SUCCESS;



	}
}*/


CLI(lt_detect_enable,
	lt_detect_enable_cmd,
	"lt {enable|disable}",
	"lt configure",
	"lt enable configure:lb enable or disable "

	)
	{
		if (0 == oam_meg_flag)
				{
				cli_out (cli, "%% oam is disabled !! ");
				return CLI_ERROR;
			}
			else
			{
			   struct meg_master *megt = &cli->zg->megg;
			   zlog_info (NSM_ZG, "call meg master success	\n.");	//test 014
			   struct MEG *megp;
			   struct MEG *mtmp;
			   mtmp = (struct MEG *)cli->index;
			   int iTtl= atoi(argv[0]);

			   megp = meg_lookup_by_id ( megt, mtmp->Meg_Index);//test 002

			   if ( 0 == pal_strcmp ( "enable", argv[0] ))
			   	{

/****************************************************************************************

					  call for oam driver  for oam message delive

*****************************************************************************************/
	                 return CLI_SUCCESS;

			    }


			   if ( 0 == pal_strcmp ( "disable", argv[0] ))
			   	{

/****************************************************************************************

					  call for oam driver  for oam message delive

*****************************************************************************************/

                 return CLI_SUCCESS;

			   }

			}



}






CLI(lt_detect_ttl,
    lt_detect_ttl_cmd,
    "lt ttl  <1-255>",
    "lt configure",
    "lt ttl configure:ttl 1-255,default is 30. "

)
{
	if (0 == oam_meg_flag)
			{
			cli_out (cli, "%% oam is disabled !! ");
			return CLI_ERROR;
		}
		else
		{
					  struct meg_master *megt = &cli->zg->megg;
					  zlog_info (NSM_ZG, "call meg master success  \n.");  //test 014
					  struct MEG *megp;
					  struct MEG *mtmp;
					  mtmp = (struct MEG *)cli->index;
					  int iTtl= atoi(argv[0]);

					  megp = meg_lookup_by_id ( megt, mtmp->Meg_Index);//test 002






					  if((iTtl<1)||(iTtl>255))
					  {

					   cli_out (cli, "%% Invalid value !! TTL should be 1-255 !!");
					   return CLI_ERROR;
					  }
					  else
						  {
						  megp->Oam_Toolp->Oam_Tool_Ltp->Ttl=iTtl;

						  cli_out (cli, "%% ttl %d \n !!",iTtl);
						  }



		return CLI_SUCCESS;

		}



}



CLI(lt_detect_size,
    lt_detect_size_cmd,
    "lt size  <1-400>",
    "lt configure ",
    "lt size configure:size 1-400,default is 0. "

)
{
	if (0 == oam_meg_flag)
			{
			cli_out (cli, "%% oam is disabled !! ");
			return CLI_ERROR;
		}
		else
		{
					  struct meg_master *megt = &cli->zg->megg;
					  zlog_info (NSM_ZG, "call meg master success  \n.");  //test 014
					  struct MEG *megp;
					  struct MEG *mtmp;
					  mtmp = (struct MEG *)cli->index;
					  int iSize= atoi(argv[0]);
					  megp = meg_lookup_by_id ( megt, mtmp->Meg_Index);//test 002




					  if((iSize>=1)&&(iSize<=400))
					  {
					  megp->Oam_Toolp->Oam_Tool_Ltp->Packet_Size=iSize;
					  cli_out (cli, "%% repeat %d \n !!",iSize);

					  }



		return CLI_SUCCESS;

		}



}



CLI(lt_detect_timeout,
    lt_detect_timeout_cmd,
    "lt timeout  <1-10>",
    "lt configure ",
    "lt timeout configure:timeout 1-10,default is 5s. "

)
{

if (0 == oam_meg_flag)
        {
		cli_out (cli, "%% oam is disabled !! ");
 		return CLI_ERROR;
	}
 	else
	{
		          struct meg_master *megt = &cli->zg->megg;
				  zlog_info (NSM_ZG, "call meg master success  \n.");  //test 014
				  struct MEG *megp;
				  struct MEG *mtmp;
				  mtmp = (struct MEG *)cli->index;

				  int iTimeout =atoi(argv[0]);
				  megp = meg_lookup_by_id ( megt, mtmp->Meg_Index);//test 002





				  if((iTimeout>=1)&&(iTimeout<=10))
				  {
				  megp->Oam_Toolp->Oam_Tool_Ltp->Time_Out=iTimeout;
				  cli_out (cli, "%% repeat %d \n !!",iTimeout);

				  }

	return CLI_SUCCESS;

	}


}





/*CLI(lt_detect,
    lt_detect_cmd,
    "lt [ttl<1-255>][size<1-400>][timeout<1-10>]{enable|disable}",
    "lt configure:ttl 1-255 ",
    "lt configure:size 1-400 ",
    "lt configure:timeout 1-10 "

)
{
	if (0 == oam_meg_flag)
        {
		cli_out (cli, "%% oam is disabled !! ");
 		return CLI_ERROR;
	}
 	else
	{
		          struct meg_master *megt = &cli->zg->megg;
				  zlog_info (NSM_ZG, "call meg master success  \n.");  //test 014
				  struct MEG *megp;
				  struct MEG *mtmp;
				  mtmp = (struct MEG *)cli->index;
				  int iTtl= atoi(argv[0]);
				  int iSize= atoi(argv[1]);
				  int iTimeout =atoi(argv[2]);
				  megp = meg_lookup_by_id ( megt, mtmp->Meg_Index);//test 002

				  megp->Oam_Toolp->Oam_Tool_Ltp->Ttl=30;
				  megp->Oam_Toolp->Oam_Tool_Ltp->Packet_Size=0;
				  megp->Oam_Toolp->Oam_Tool_Ltp->Time_Out=5;




				  if((iTtl<1)||(iTtl>255))
				  {

				   cli_out (cli, "%% Invalid value !! TTL should be 1-255 !!");
				   return CLI_ERROR;
				  }
				  else
					  {
					  megp->Oam_Toolp->Oam_Tool_Ltp->Ttl=iTtl;

					  cli_out (cli, "%% ttl %d \n !!",iTtl);
					  }


				  if((iSize>=1)&&(iSize<=400))
				  {
				  megp->Oam_Toolp->Oam_Tool_Ltp->Packet_Size=iSize;
				  cli_out (cli, "%% repeat %d \n !!",iSize);

				  }

				  if((iTimeout>=1)&&(iTimeout<=10))
				  {
				  megp->Oam_Toolp->Oam_Tool_Ltp->Time_Out=iTimeout;
				  cli_out (cli, "%% repeat %d \n !!",iTimeout);

				  }

	return CLI_SUCCESS;

	}
}*/



CLI(lm_proactive_detect,
    lm_proactive_detect_cmd,
    "lm proactive {enable |disable}",
    "lm proactive detect:enable or disable ",
    "lm proactive dection configure"
)
{
	if (0 == oam_meg_flag)
        {
		cli_out (cli, "%% oam is disabled !! ");
 		return CLI_ERROR;
	}
 	else
	{
		    zlog_info (NSM_ZG, "enter the function for LM enable  \n.");	//test 014

		    struct meg_master *megt = &cli->zg->megg;
			zlog_info (NSM_ZG, "call meg master success  \n.");  //test 014
			struct MEG *megp;
			struct MEG *mtmp;
			mtmp = (struct MEG *)cli->index;
            megp = meg_lookup_by_id ( megt, mtmp->Meg_Index);//test 002


			zlog_info (NSM_ZG, "meg look up success  and meg index is now %d. \n",megp->Meg_Index);  //test 014
			zlog_info (NSM_ZG, "meg look up success  \n.");  //test 014
    if(megp->Meg_Index > 0)
    {

       if ( 0 == pal_strcmp ( "enable", argv[0] ))
 	    {

		//megp->Oam_Toolp->Oam_Tool_Lmp->Lm_State=Lm_On;  //test czh
 		megp->iPro_Active_Lm = Lm_Enable;
		zlog_info (NSM_ZG, "proactive LM is enble %d . \n",megp->iPro_Active_Lm);
		return CLI_SUCCESS;
 	    }
 	  if ( 0 == pal_strcmp ( "disable", argv[0] ))
 	   {
		//megp->Oam_Toolp->Oam_Tool_Lmp->Lm_State=Lm_Off;    //test czh
 		megp->iPro_Active_Lm= Lm_Disable;
		zlog_info (NSM_ZG, "proactive LM is disble %d . \n",megp->iPro_Active_Lm);
		return CLI_SUCCESS;
 	   }
	}
}
}


CLI(lm_on_demand_detect_enable,
    lm_on_demand_detect_enable_cmd,
    "lm-on-demand {enable|disable}",
    "lm on-demand configure ",
    "lm on-demand enable configure:enable or disable ."
)
{

if (0 == oam_meg_flag)
        {
		cli_out (cli, "%% oam is disabled !! ");
 		return CLI_ERROR;
	}
 	else
	{
	           struct meg_master *megt = &cli->zg->megg;
			   zlog_info (NSM_ZG, "call meg master success	\n.");	//test 014
			   struct MEG *megp;
			   struct MEG *mtmp;
			   mtmp = (struct MEG *)cli->index;
			   int iTtl= atoi(argv[0]);

			   megp = meg_lookup_by_id ( megt, mtmp->Meg_Index);//test 002

			   if ( 0 == pal_strcmp ( "enable", argv[0] ))
			   	{

/****************************************************************************************

					  call for oam driver  for oam message delive

*****************************************************************************************/
	                 return CLI_SUCCESS;

			    }


			   if ( 0 == pal_strcmp ( "disable", argv[0] ))
			   	{

/****************************************************************************************

					  call for oam driver  for oam message delive

*****************************************************************************************/

                 return CLI_SUCCESS;

			   }

	}


}






CLI(lm_on_demand_detect_repeat,
    lm_on_demand_detect_repeat_cmd,
    "lm-on-demand repeat <1-200>",
    "lm on-demand configure ",
    "lm on-demand repeat configure:repeat 1-200,default is 3 ."
)
{

if (0 == oam_meg_flag)
        {
		cli_out (cli, "%% oam is disabled !! ");
 		return CLI_ERROR;
	}
 	else
	{
		          struct meg_master *megt = &cli->zg->megg;
				  zlog_info (NSM_ZG, "call meg master success  \n.");  //test 014
				  struct MEG *megp;
				  struct MEG *mtmp;
				  mtmp = (struct MEG *)cli->index;
				  int iRepeat= atoi(argv[0]);

				  megp = meg_lookup_by_id ( megt, mtmp->Meg_Index);//test 002



				  if((iRepeat>=1)&&(iRepeat<=200))
				  {
				  megp->Oam_Toolp->Oam_Tool_Lmp->Repeat_Num=iRepeat;
				  cli_out (cli, "%% repeat %d \n !!",iRepeat);

				  }


						  return CLI_SUCCESS;

	}


}



CLI(lm_on_demand_detect_period,
    lm_on_demand_detect_period_cmd,
    "lm-on-demand period {1s|10s}",
    "lm on-demand configure ",
    "lm on-demand period configure:period  1s or 10s ,default is 1s  ."
)
{
	if (0 == oam_meg_flag)
			{
			cli_out (cli, "%% oam is disabled !! ");
			return CLI_ERROR;
		}
		else
		{
					  struct meg_master *megt = &cli->zg->megg;
					  zlog_info (NSM_ZG, "call meg master success  \n.");  //test 014
					  struct MEG *megp;
					  struct MEG *mtmp;
					  mtmp = (struct MEG *)cli->index;
					  int iPeriod ;
					  megp = meg_lookup_by_id ( megt, mtmp->Meg_Index);//test 002


					  megp->Oam_Toolp->Oam_Tool_Lmp->Repeat_Num=3;
					  megp->Oam_Toolp->Oam_Tool_Lmp->Lm_Period=1;



					  if ( 0 == pal_strcmp ( "1s", argv[1] ))
							  {
							  iPeriod =1;
							  megp->Oam_Toolp->Oam_Tool_Lmp->Lm_Period=iPeriod ;
							  zlog_info (NSM_ZG, "period is 1s	\n");
							  return CLI_SUCCESS;
							  }
							if ( 0 == pal_strcmp ( "10s", argv[1] ))
							 {
							  iPeriod =10;
							  megp->Oam_Toolp->Oam_Tool_Lmp->Lm_Period=iPeriod ;
							  zlog_info (NSM_ZG, "period is 10s  \n");
							  return CLI_SUCCESS;
							 }

		}



}





/*CLI(lm_on_demand_detect,
    lm_on_demand_detect_cmd,
    "lm on-demand[repeat<1-200>][period{1s|10s}]",
    "lm on-demand detect:repeat 1-200 ",
    "lm on-demand detect:period 1s or 10s",
    "lm on-demand dection configure"
)
{
	if (0 == oam_meg_flag)
        {
		cli_out (cli, "%% oam is disabled !! ");
 		return CLI_ERROR;
	}
 	else
	{
		          struct meg_master *megt = &cli->zg->megg;
				  zlog_info (NSM_ZG, "call meg master success  \n.");  //test 014
				  struct MEG *megp;
				  struct MEG *mtmp;
				  mtmp = (struct MEG *)cli->index;
				  int iRepeat= atoi(argv[0]);
				  int iPeriod ;
				  megp = meg_lookup_by_id ( megt, mtmp->Meg_Index);//test 002


				  megp->Oam_Toolp->Oam_Tool_Lmp->Repeat_Num=3;
				  megp->Oam_Toolp->Oam_Tool_Lmp->Lm_Period=1;


				  if((iRepeat>=1)&&(iRepeat<=200))
				  {
				  megp->Oam_Toolp->Oam_Tool_Lmp->Repeat_Num=iRepeat;
				  cli_out (cli, "%% repeat %d \n !!",iRepeat);

				  }

				  if ( 0 == pal_strcmp ( "1s", argv[1] ))
						  {
						  iPeriod =1;
						  megp->Oam_Toolp->Oam_Tool_Lmp->Lm_Period=iPeriod ;
						  zlog_info (NSM_ZG, "period is 1s  \n");
						  return CLI_SUCCESS;
						  }
						if ( 0 == pal_strcmp ( "10s", argv[1] ))
						 {
						  iPeriod =10;
						  megp->Oam_Toolp->Oam_Tool_Lmp->Lm_Period=iPeriod ;
						  zlog_info (NSM_ZG, "period is 10s  \n");
						  return CLI_SUCCESS;
						 }

	}
}*/



CLI(lp_alarm_threshold,
    lp_alarm_threshold_cmd,
    "loss-packet alarm-threshold <1-100000>",
    "loss-packet alarm-threshold:1-100000 ",
    "loss-packet alarm-threshold configure"
)
{
	if (0 == oam_meg_flag)
        {
		cli_out (cli, "%% oam is disabled !! ");
 		return CLI_ERROR;
	}
 	else
	{
		    zlog_info (NSM_ZG, "enter the function for cc enable  \n.");	//test 014

		    struct meg_master *megt = &cli->zg->megg;
			zlog_info (NSM_ZG, "call meg master success  \n.");  //test 014
			struct MEG *megp;
			struct MEG *mtmp;
			mtmp = (struct MEG *)cli->index;
			int iTmp = atoi(argv[0]); //test 011



            megp = meg_lookup_by_id ( megt, mtmp->Meg_Index);//test 002
			zlog_info (NSM_ZG, "meg look up success  and meg index is now %d. \n",megp->Meg_Index);  //test 014
			zlog_info (NSM_ZG, "meg look up success  \n.");  //test 014
    if(megp->Meg_Index > 0)
    {

       if ((iTmp>=1)&&(iTmp<=100000))
 	    {
 	       megp->Oam_Toolp->Oam_Tool_Lmp->Result=iTmp ;
		   megp->Loss_Pkt_Thr=iTmp;

		   zlog_info (NSM_ZG, "loss packet threshold is  now %d. \n",megp->Loss_Pkt_Thr);

 	    }
	}
	}
}

/*support two way DM only now,one way DM will be added at the later version  2011.11.09 by czh*/


CLI(dm_way_select_enable,
    dm_way_select_enable_cmd,
    "dm {enable|disable}",
    "dm  configure ",
    "lt enable configure:enable or disable . "
)
{
	if (0 == oam_meg_flag)
        {
		cli_out (cli, "%% oam is disabled !! ");
 		return CLI_ERROR;
	}
 	else
	{
		          struct meg_master *megt = &cli->zg->megg;
			   zlog_info (NSM_ZG, "call meg master success	\n.");	//test 014
			   struct MEG *megp;
			   struct MEG *mtmp;
			   mtmp = (struct MEG *)cli->index;
			   int iTtl= atoi(argv[0]);

			   megp = meg_lookup_by_id ( megt, mtmp->Meg_Index);//test 002

			   if ( 0 == pal_strcmp ( "enable", argv[0] ))
			   	{

/****************************************************************************************

					  call for oam driver  for oam message delive

*****************************************************************************************/
	                 return CLI_SUCCESS;

			    }


			   if ( 0 == pal_strcmp ( "disable", argv[0] ))
			   	{

/****************************************************************************************

					  call for oam driver  for oam message delive

*****************************************************************************************/

                 return CLI_SUCCESS;

			   }

 	}
}







CLI(dm_way_select_repeat,
    dm_way_select_repeat_cmd,
    "dm repeat  <1-200>",
    "dm  configure ",
    "lt period configure:repeat 1-200,default is 3 . "
)
{
	if (0 == oam_meg_flag)
        {
		cli_out (cli, "%% oam is disabled !! ");
 		return CLI_ERROR;
	}
 	else
	{
		          struct meg_master *megt = &cli->zg->megg;
				  zlog_info (NSM_ZG, "call meg master success  \n.");  //test 014
				  struct MEG *megp;
				  struct MEG *mtmp;
				  mtmp = (struct MEG *)cli->index;
				  int iRepeat= atoi(argv[0]);

				  megp = meg_lookup_by_id ( megt, mtmp->Meg_Index);//test 002



				  if((iRepeat>=1)&&(iRepeat<=200))
				  {
				  megp->Oam_Toolp->Oam_Tool_Dmp->Repeat_Num=iRepeat ;
				  cli_out (cli, "%% repeat %d \n !!",iRepeat);

				  }


	return CLI_SUCCESS;


 	}
}


CLI(dm_way_select_period,
    dm_way_select_period_cmd,
    "dm period  {1s|10s}",
    "dm  configure ",
    "lt period configure:1s or 10s,default is 1s  . "
)
{
	if (0 == oam_meg_flag)
        {
		cli_out (cli, "%% oam is disabled !! ");
 		return CLI_ERROR;
	}
 	else
	{
		          struct meg_master *megt = &cli->zg->megg;
				  zlog_info (NSM_ZG, "call meg master success  \n.");  //test 014
				  struct MEG *megp;
				  struct MEG *mtmp;
				  mtmp = (struct MEG *)cli->index;
				  int iPeriod ;
				  megp = meg_lookup_by_id ( megt, mtmp->Meg_Index);//test 002





				  if ( 0 == pal_strcmp ( "1s", argv[1] ))
						  {
						  iPeriod =1;
						  megp->Oam_Toolp->Oam_Tool_Dmp->Dm_Period=iPeriod ;
						  zlog_info (NSM_ZG, "period is 1s  \n");
						  return CLI_SUCCESS;
						  }
				if ( 0 == pal_strcmp ( "10s", argv[1] ))
						 {
						  iPeriod =10;
						  megp->Oam_Toolp->Oam_Tool_Dmp->Dm_Period=iPeriod ;
						  zlog_info (NSM_ZG, "period is 10s  \n");
						  return CLI_SUCCESS;
						}
	}



}






/*CLI(dm_way_select,
    dm_way_select_cmd,
    "dm  [repeat<1-200>][period{1s|10s}]",
    "dm way configure:one-way or two-way ",
    "dm repeat configure:repeat 1-200 ",
    "lt period configure:1s or 10s "
)
{
	if (0 == oam_meg_flag)
        {
		cli_out (cli, "%% oam is disabled !! ");
 		return CLI_ERROR;
	}
 	else
	{
		          struct meg_master *megt = &cli->zg->megg;
				  zlog_info (NSM_ZG, "call meg master success  \n.");  //test 014
				  struct MEG *megp;
				  struct MEG *mtmp;
				  mtmp = (struct MEG *)cli->index;
				  int iRepeat= atoi(argv[0]);
				  int iPeriod ;
				  megp = meg_lookup_by_id ( megt, mtmp->Meg_Index);//test 002

				   megp->Oam_Toolp->Oam_Tool_Dmp->Repeat_Num=3;
				   megp->Oam_Toolp->Oam_Tool_Dmp->Dm_Period=1;


				  if((iRepeat>=1)&&(iRepeat<=200))
				  {
				  megp->Oam_Toolp->Oam_Tool_Dmp->Repeat_Num=iRepeat ;
				  cli_out (cli, "%% repeat %d \n !!",iRepeat);

				  }

				  if ( 0 == pal_strcmp ( "1s", argv[1] ))
						  {
						  iPeriod =1;
						  megp->Oam_Toolp->Oam_Tool_Dmp->Dm_Period=iPeriod ;
						  zlog_info (NSM_ZG, "period is 1s  \n");
						  return CLI_SUCCESS;
						  }
						if ( 0 == pal_strcmp ( "10s", argv[1] ))
						 {
						  iPeriod =10;
						  megp->Oam_Toolp->Oam_Tool_Dmp->Dm_Period=iPeriod ;
						  zlog_info (NSM_ZG, "period is 10s  \n");
						  return CLI_SUCCESS;
						 }
	}
}*/



CLI(pd_alarm_threshold,
    pd_alarm_threshold_cmd,
    "packet-delay alarm-threshold <1-100000>",
    "packet-delay alarm-threshold:1-100000 ",
    "packet-delay alarm-threshold configure"
)
{
	if (PAL_FALSE== oam_meg_flag)
        {
		cli_out (cli, "%% oam is disabled !! ");
 		return CLI_ERROR;
	}
 	else
	{
		 zlog_info (NSM_ZG, "enter the function for cc enable  \n.");	//test 014

		    struct meg_master *megt = &cli->zg->megg;
			zlog_info (NSM_ZG, "call meg master success  \n.");  //test 014
			struct MEG *megp;
			struct MEG *mtmp;
			mtmp = (struct MEG *)cli->index;
			int iTmp = atoi(argv[0]); //test 011



            megp = meg_lookup_by_id ( megt, mtmp->Meg_Index);//test 002
			zlog_info (NSM_ZG, "meg look up success  and meg index is now %d. \n",megp->Meg_Index);  //test 014
			zlog_info (NSM_ZG, "meg look up success  \n.");  //test 014
    if(megp->Meg_Index > 0)
    {

       if ((iTmp>=1)&&(iTmp<=100000))
 	    {
 	       megp->Oam_Toolp->Oam_Tool_Dmp->Result=iTmp ;

		   cli_out (cli,"%%packet delay alarm threshold is %d .\n",iTmp);



 	    }
	}
	return CLI_SUCCESS;

	}
}


CLI(pdc_alarm_threshold,
    pdc_alarm_threshold_cmd,
    "packet-delay-change alarm-threshold <1-100000>",
    "packet-delay-change alarm-threshold:1-100000 ",
    "packet-delay-change alarm-threshold configure"
)
{
	if (0 == oam_meg_flag)
        {
		cli_out (cli, "%% oam is disabled !! ");
 		return CLI_ERROR;
	}
 	else
	{
		zlog_info (NSM_ZG, "enter the function for cc enable  \n.");	//test 014

		    struct meg_master *megt = &cli->zg->megg;
			zlog_info (NSM_ZG, "call meg master success  \n.");  //test 014
			struct MEG *megp;
			struct MEG *mtmp;
			mtmp = (struct MEG *)cli->index;
			int iTmp = atoi(argv[0]); //test 011



            megp = meg_lookup_by_id ( megt, mtmp->Meg_Index);//test 002
			zlog_info (NSM_ZG, "meg look up success  and meg index is now %d. \n",megp->Meg_Index);  //test 014
			zlog_info (NSM_ZG, "meg look up success  \n.");  //test 014
    if(megp->Meg_Index > 0)
    {

       if ((iTmp>=1)&&(iTmp<=100000))
 	    {
		   megp->Oam_Toolp->Oam_Tool_Dmp->Result=iTmp ;

		   cli_out (cli,"%%packet delay  alarm threshold is changed to %d .\n",iTmp);



 	    }
	}
	return CLI_SUCCESS;

	}
}

CLI (megshow_information,
     megshow_information_cmd,
     "megshow INDEX",
     "Show meg id",
     "current meg information" )
{

     zlog_info (NSM_ZG, "enter the function for meg information show . \n.");  //test 014

	 struct meg_master *megt = &cli->zg->megg;
	 zlog_info (NSM_ZG, "call meg master success  \n.");  //test 014
	 struct MEG *megp;
	 struct MEG *mtmp;
	 mtmp = (struct MEG *)cli->index;
	 int iTmp = atoi(argv[0]); //test 011

	 megp = meg_lookup_by_id ( megt, mtmp->Meg_Index);//test 002
	 zlog_info (NSM_ZG, "meg look up success  and meg index is now %d. \n",megp->Meg_Index);  //test 014
	 zlog_info (NSM_ZG, "meg look up success  \n.");  //test 014

 if ( NULL == megp )
      {
        cli_out (cli, "%% MEG %s does not exist. ");
        return CLI_ERROR;
       }
  else
    {
        cli_out (cli, "MEG index: %d  \n  MEG ID: %d \n  CC INFO:%d \n  Bind to : %d \n", megp->Meg_Index,megp->Meg_Id,megp->iCc_Enable,megp->Object_Key); //test   027
         zlog_info (NSM_ZG, "MEG index: %d  \n  MEG ID: %d \n  CC INFO:%d \n  Bind to : %d \n", megp->Meg_Index,megp->Meg_Id,megp->iCc_Enable,megp->Object_Key); //test 028

	return CLI_SUCCESS;
    }

}

#if 0
struct MEG *megp;
struct listnode *node;
LIST_LOOP (nzg->megg.meg_list, megp, node)
  {
       if(iTmp == megp->Object_Key)
  {
       if( 0 != megp->Meg_Index)
       {
              cli_out (cli, " meg  %d \n", megp->Meg_Index);
      if(megp->Meg_Index >0)
          {
          cli_out (cli, " id  %d \n", megp->Meg_Id);
          }


     if( No_Mep_Class != megp->iLocal_Mep_Class)
      {

        if(SOURCE == megp->iLocal_Mep_Class)
          {
              cli_out (cli, " type source\n");

          }
          if(DEST == megp->iLocal_Mep_Class)
          {
              cli_out (cli, " type destinnation\n");

          }
          if(BIDI == megp->iLocal_Mep_Class)
          {
              cli_out (cli, " type bidirectional\n");

          }
    }


    if(Mep_Down != megp->iLocal_Mep_State)
              {
                  cli_out (cli, " Local mep is up\n");
              }
          if( 0 != megp->Local_Mep_Id)
          {
              cli_out (cli, " local-mep  %d \n", megp->Local_Mep_Id);
          }

          if( 0 != megp->Peer_Mep_Id)
          {
              cli_out (cli, " peer-mep  %d \n", megp->Peer_Mep_Id);
          }
          if( No_Mep_Class != megp->iPeer_Mep_Class)
      {

        if(SOURCE == megp->iPeer_Mep_Class)
          {
              cli_out (cli, " type  source\n");

          }
          if(DEST == megp->iPeer_Mep_Class)
          {
              cli_out (cli, " type destinnation\n");

          }
          if(BIDI == megp->iPeer_Mep_Class)
          {
              cli_out (cli, " type bidirectional\n");

          }
    }


    if(Mip_Disable != megp->iMip_Enable)
          {
              cli_out (cli, " mip enble \n");

          }
          if(Mip_Down != megp->iMip_State)
          {
              cli_out (cli, " mip up \n");

          }
          if( 0 != megp->Mip_Id)
          {
              cli_out (cli, " mip %d\n", megp->Mip_Id);
          }
          if( 0!= megp->Object_Key)
          {
              cli_out (cli, " Bind to PW/LSP/SECTION %d\n", megp->Object_Key);
          }
          if(Cc_Disable != megp->iCc_Enable)
          {
              cli_out (cli, " cc enable \n");

          }



          if(Fast_Default != megp->iSend_Timer)
          {
          if(Fast_10_Ms == megp->iSend_Timer)
          {
              cli_out (cli, " cc period \n",megp->iSend_Timer);
          }
          if(Fast_100_Ms == megp->iSend_Timer)
          {
              cli_out (cli, " cc period \n",megp->iSend_Timer);
          }
          if(Fast_1_S== megp->iSend_Timer)
          {
              cli_out (cli, " cc period \n",megp->iSend_Timer);
          }
          if(Fast_10_S == megp->iSend_Timer)
          {
              cli_out (cli, "  cc period \n",megp->iSend_Timer);  //test 033
          }
          if(Fast_1_M == megp->iSend_Timer)
          {
              cli_out (cli, " cc period \n",megp->iSend_Timer);
          }
          if(Fast_10_M == megp->iSend_Timer)
          {
              cli_out (cli, " cc period \n",megp->iSend_Timer);
          }

          }



           if(Lm_Disable != megp->iPro_Active_Lm)
          {
              cli_out (cli, " lm proactive enable  \n");

          }
          if( 0!= megp->Loss_Pkt_Thr)
          {
              cli_out (cli, " loss-packet alarm-threshold %d\n", megp->Loss_Pkt_Thr);
          }

          }
       }
   }
#endif


int nsm_meg_config_write (struct cli *cli)    //test 033
{

	  zlog_info (NSM_ZG, "enter the function for meg_config_write \n");   //test 035

	  struct MEG *megp;
	  struct listnode *node;
	  LIST_LOOP (nzg->megg.meg_list, megp, node)
	  	{

			 if( 0 != megp->Meg_Index)
			 {
			  		cli_out (cli, " meg index %d \n", megp->Meg_Index);
			if(megp->Meg_Index >0)
				{
				cli_out (cli, " meg index %d \n", megp->Meg_Id);
				}


           if( No_Mep_Class != megp->iLocal_Mep_Class)
        	{

			  if(SOURCE == megp->iLocal_Mep_Class)
			  	{
			  		cli_out (cli, " type source\n");

			  	}
			  	if(DEST == megp->iLocal_Mep_Class)
			  	{
			  		cli_out (cli, " type destinnation\n");

			  	}
			  	if(BIDI == megp->iLocal_Mep_Class)
			  	{
			  		cli_out (cli, " type bidirectional\n");

			  	}
          }


		  if(Mep_Down != megp->iLocal_Mep_State)
			  		{
			  			cli_out (cli, " Local mep is up\n");
			  		}
			  	if( 0 != megp->Local_Mep_Id)
			  	{
			  		cli_out (cli, " local-mep  %d \n", megp->Local_Mep_Id);
			  	}

			  	if( 0 != megp->Peer_Mep_Id)
			  	{
			  		cli_out (cli, " peer-mep  %d \n", megp->Peer_Mep_Id);
			  	}
			  	if( No_Mep_Class != megp->iPeer_Mep_Class)
        	{

			  if(SOURCE == megp->iPeer_Mep_Class)
			  	{
			  		cli_out (cli, " type  source\n");

			  	}
			  	if(DEST == megp->iPeer_Mep_Class)
			  	{
			  		cli_out (cli, " type destinnation\n");

			  	}
			  	if(BIDI == megp->iPeer_Mep_Class)
			  	{
			  		cli_out (cli, " type bidirectional\n");

			  	}
          }


		  if(Mip_Disable != megp->iMip_Enable)
			  	{
			  		cli_out (cli, " mip enble \n");

			  	}
			  	if(Mip_Down != megp->iMip_State)
			  	{
			  		cli_out (cli, " mip up \n");

			  	}
			  	if( 0 != megp->Mip_Id)
			  	{
			  		cli_out (cli, " mip %d\n", megp->Mip_Id);
			  	}
			  	if( 0!= megp->Object_Key)
			  	{
			  		cli_out (cli, " Bind to PW/LSP/SECTION %d\n", megp->Object_Key);
			  	}
			    if(Cc_Disable != megp->iCc_Enable)
			  	{
			  		cli_out (cli, " cc enable \n");

			  	}



				if(Fast_Default != megp->iSend_Timer)
			  	{
			  	if(Fast_10_Ms == megp->iSend_Timer)
			  	{
			  		cli_out (cli, " cc period \n",megp->iSend_Timer);
			  	}
			  	if(Fast_100_Ms == megp->iSend_Timer)
			  	{
			  		cli_out (cli, " cc period \n",megp->iSend_Timer);
			  	}
			  	if(Fast_1_S== megp->iSend_Timer)
			  	{
			  		cli_out (cli, " cc period \n",megp->iSend_Timer);
			  	}
			  	if(Fast_10_S == megp->iSend_Timer)
			  	{
			  		cli_out (cli, "  cc period \n",megp->iSend_Timer);  //test 033
			  	}
			  	if(Fast_1_M == megp->iSend_Timer)
			  	{
			  		cli_out (cli, " cc period \n",megp->iSend_Timer);
			  	}
			  	if(Fast_10_M == megp->iSend_Timer)
			  	{
			  		cli_out (cli, " cc period \n",megp->iSend_Timer);
			  	}

			  	}



				 if(Lm_Disable != megp->iPro_Active_Lm)
			  	{
			  		cli_out (cli, " lm proactive enable  \n");

			  	}
			  	if( 0!= megp->Loss_Pkt_Thr)
			  	{
			  		cli_out (cli, " loss-packet alarm-threshold %d\n", megp->Loss_Pkt_Thr);
			  	}
			  	}
}

   cli_out (cli, "!\n");  //test 002

}



void nsm_meg_cli_init(struct cli_tree *ctree)
{
  cli_install_default(ctree, MEG_MODE);
  cli_install_config(ctree,MEG_MODE,nsm_meg_config_write);  //test 032
  cli_install(ctree,TUN_MODE,&meg_index_cmd);
  cli_install(ctree,TUN_MODE,&no_meg_index_cmd);

  cli_install(ctree,PW_MODE,&meg_index_cmd);
  cli_install(ctree,PW_MODE,&no_meg_index_cmd);

  cli_install(ctree,SEC_MODE,&meg_index_cmd);
  cli_install(ctree,SEC_MODE,&no_meg_index_cmd);

  cli_install(ctree,OAM_1731_MODE,&meg_index_cmd);
  cli_install(ctree,OAM_1731_MODE,&no_meg_index_cmd);

  cli_install(ctree,MEG_MODE,&meg_id_cmd);
  cli_install(ctree,MEG_MODE,&no_meg_id_cmd);

  cli_install(ctree,MEG_MODE,&oam_enable_cmd);
  cli_install(ctree,MEG_MODE,&creat_local_mep_cmd);
  cli_install(ctree,MEG_MODE,&no_creat_local_mep_cmd);
  cli_install(ctree,MEG_MODE,&creat_peer_mep_cmd);
  cli_install(ctree,MEG_MODE,&no_creat_peer_mep_cmd);
  cli_install(ctree,MEG_MODE,&creat_mip_cmd);
  cli_install(ctree,MEG_MODE,&cc_detect_cmd);
  cli_install(ctree,MEG_MODE,&cc_packet_send_period_cmd);


  cli_install(ctree,MEG_MODE,&lb_detect_ttl_cmd);
  cli_install(ctree,MEG_MODE,&lb_detect_repeat_cmd);
  cli_install(ctree,MEG_MODE,&lb_detect_size_cmd);
  cli_install(ctree,MEG_MODE,&lb_detect_timeout_cmd);
  cli_install(ctree,MEG_MODE,&lb_detect_enable_cmd);


  cli_install(ctree,MEG_MODE,&lt_detect_ttl_cmd);
  cli_install(ctree,MEG_MODE,&lt_detect_size_cmd);
  cli_install(ctree,MEG_MODE,&lt_detect_timeout_cmd);
  cli_install(ctree,MEG_MODE,&lt_detect_enable_cmd);




  cli_install(ctree,MEG_MODE,&lm_proactive_detect_cmd);


  cli_install(ctree,MEG_MODE,&lm_on_demand_detect_repeat_cmd);
  cli_install(ctree,MEG_MODE,&lm_on_demand_detect_period_cmd);
  cli_install(ctree,MEG_MODE,&lm_on_demand_detect_enable_cmd);



  cli_install(ctree,MEG_MODE,&dm_way_select_repeat_cmd);
  cli_install(ctree,MEG_MODE,&dm_way_select_period_cmd);
  cli_install(ctree,MEG_MODE,&dm_way_select_enable_cmd);


  cli_install(ctree,MEG_MODE,&lp_alarm_threshold_cmd);

  cli_install(ctree,MEG_MODE,&pd_alarm_threshold_cmd);
  cli_install(ctree,MEG_MODE,&pdc_alarm_threshold_cmd);

  cli_install(ctree,MEG_MODE,&megshow_information_cmd);
  cli_install(ctree,EXEC_MODE,&megshow_information_cmd);


}


struct MEG *meg_lookup_by_id (struct meg_master *megm, int  megid)  //test 002
{

  zlog_info (NSM_ZG, "enter the function for meg look up  \n.");
  struct MEG *megp;
  struct listnode *node;

  zlog_info (NSM_ZG, "list loop for meg  look up  \n.");

  LIST_LOOP (megm->meg_list, megp, node)

     if (megp->Meg_Index == megid)    //test 002    test 008

     {
      zlog_info (NSM_ZG, "look for meg index  %d \n.",megp->Meg_Index);//test 008
      return megp;
	  zlog_info (NSM_ZG, "leave  if branch of list_loop   \n"); //test 010
     }

	zlog_info (NSM_ZG, "leave meg_lookup_by_id function and return null \n"); //test 010

	return NULL;

                                                      //test 002

}


void megg_list_add (struct meg_master *megg, struct MEG* megp)
{

  listnode_add (megg->meg_list,megp);

  megg->megTblLastChange = pal_time_current (NULL);

}




