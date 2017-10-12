/* Copyright (C) 2002-2004  All Rights Reserved. */


/*资源池管理*/
#include "config.h"
#include "hsl_os.h"
#include "bcm_incl.h"
#include "hsl_types.h"
#include "hsl_oss.h"

#include "hsl_Rsrc.h"


//创建资源池
unsigned int RsrcMgrCreate(TRsrMangrInfo_t *ptRsr)
{
	unsigned short usLen = 0;
	unsigned int i = 0;
	unsigned char  aTmpString[8];
	unsigned int rc = 0;
	
	if(ptRsr->usMaxValue < ptRsr->usMinValue)
	{
		return RSRC_INVALID_PARAM;	
	}

	usLen = ptRsr->usMaxValue - ptRsr->usMinValue + 1;

	ptRsr->pRsrLink = (unsigned short *)sal_alloc (usLen * sizeof(unsigned short), "Hsl_RsrcMgr");

	if(!ptRsr->pRsrLink)
	{
		return RSRC_NO_AVAIL_MEMORY;
	}

	for(i = 0;i < usLen - 1;i ++)
	{
		ptRsr->pRsrLink[i] = i + 1;
	}

	strcpy((char *)&aTmpString[0],(const char *)&(ptRsr->aRsrName[0]));
	strcat((char *)&aTmpString[0],"sem");

	ptRsr->usFreePos = 0;
	ptRsr->usAlloced = 0;
	rc = oss_sem_new (aTmpString, OSS_SEM_BINARY, 0, NULL, &(ptRsr->ulSim));
    if (rc != 0)
    {
     
    } 
		
	return RSRC_OK;
}


//获取资源池
unsigned int RsrcMgrGet(TRsrMangrInfo_t *ptRsr,unsigned short *pValue)
{
	unsigned short i = 0;

	
	if((ptRsr == NULL)||(ptRsr->pRsrLink == NULL)||(pValue == NULL))
	{	
		return RSRC_INVALID_POINTER;
	}

	
    oss_sem_lock (OSS_SEM_BINARY, ptRsr->ulSim, OSS_WAIT_FOREVER);
	
	
	if(ptRsr->usAlloced >= (ptRsr->usMaxValue - ptRsr->usMinValue + 1))
	{
		oss_sem_unlock(OSS_SEM_BINARY, ptRsr->ulSim);    //信号量操作，可以暂时屏蔽。
		return RSRC_NO_AVAIL_RESOURCE;
	}
	i =  ptRsr->usFreePos;	
	
	ptRsr->usFreePos 	= ptRsr->pRsrLink[i];
	ptRsr->pRsrLink[i] 	= RSRC_RESERV_VALUE;
	ptRsr->usAlloced ++;

	(*pValue) = i + ptRsr->usMinValue;

	oss_sem_unlock(OSS_SEM_BINARY, ptRsr->ulSim); //信号量操作，可以暂时屏蔽。
	return RSRC_OK;
}

//释放资源池
unsigned int RsrcMgrRet(TRsrMangrInfo_t *ptRsr,unsigned short usValue)
{
	unsigned short i;

	if((ptRsr == NULL)||(ptRsr->pRsrLink == NULL))
	{
		return RSRC_INVALID_POINTER;
	}
	
	 oss_sem_lock (OSS_SEM_BINARY, ptRsr->ulSim, OSS_WAIT_FOREVER);  //信号量操作，可以暂时屏蔽。
	
	if((usValue < ptRsr->usMinValue) || (usValue > ptRsr->usMaxValue))
	{
		oss_sem_unlock(OSS_SEM_BINARY, ptRsr->ulSim);    //信号量操作，可以暂时屏蔽。
		return RSRC_INVALID_PARAM;
	}

	i = usValue - ptRsr->usMinValue;

	if(ptRsr->pRsrLink[i] != RSRC_RESERV_VALUE)
	{
		oss_sem_unlock(OSS_SEM_BINARY, ptRsr->ulSim);    //信号量操作，可以暂时屏蔽。
		return RSRC_RELEASE_AGAIN;
	}
	ptRsr->pRsrLink[i] = ptRsr->usFreePos;
	ptRsr->usFreePos = i;
	ptRsr->usAlloced --;

	oss_sem_unlock(OSS_SEM_BINARY, ptRsr->ulSim);    //信号量操作，可以暂时屏蔽。

	return RSRC_OK;
}


#if 0
//举例说明
TRsrMangrInfo_t g_SourcePool;
    strcpy(g_SourcePool.aRsrName,"pool");   //资源池名字
    g_PrjDrvMgidRsrPool.usMaxValue = 2047;  //范围为0~2047	
    g_PrjDrvMgidRsrPool.usMinValue = 0;
    if(RsrcMgrCreate(&g_PrjDrvMgidRsrPool) != RSRC_OK)
   {
    // 错误处理。
   }

     //获取资源池资源
    if(RsrcMgrGet(&g_SourcePool,(UINT16 *)&Index) != RSRC_OK)
    {
       //错误处理
    }

    //释放资源池资源
    RsrcMgrRet(&g_PrjDrvDidRsrPool,(UINT16)Index);
    
 #endif





