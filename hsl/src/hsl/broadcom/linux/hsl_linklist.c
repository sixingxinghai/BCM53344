
#include "config.h"
#include "hsl_os.h"
#include "hsl_types.h"
#include "hal_if.h"
#include "hsl_oss.h"
#include "hsl_linklist.h"
/***********************************************************************
 *                            局部函数原型                             *
 **********************************************************************/

void linkListTakeSem(T_LINK_CONTAINER *pList);

void linkListGiveSem(T_LINK_CONTAINER *pList);

T_LIST_NODE *linkListNodeNew();

void linklistNodeFree (T_LIST_NODE *node);

/* 功能描述： 创建一个链表容器.用于存放链表**/
T_LINK_CONTAINER *linkContainerNew (unsigned char bNeedMutex)
{
    unsigned long  rcode = 0;
    T_LINK_CONTAINER *pList = NULL;
    
    pList = (T_LINK_CONTAINER *)(void *)sal_alloc((unsigned long)sizeof (T_LINK_CONTAINER), "hsl_link_list");

    if (pList == NULL)
    {
        return NULL;
    }

    memset ((char *)pList, 0, sizeof (T_LINK_CONTAINER));
    
    if (bNeedMutex == LINKLIST_NEED_SEM)
    {
        // rcode = oss_sem_new (LINKLIST_SEM_NAME, OSS_SEM_BINARY, 0, NULL, &(pList->sem));
        if (rcode != 0)
        {
            pList->sem = 0;
        }
    }

    return pList;
}

/* 输入参数： List --p 要释放的链表容器的指针*/
void linkListContainerFree(T_LINK_CONTAINER *pList)
{
    if (pList == NULL)
    {
        return;
    }
       
    linkListRemoveAllNode(pList);
    oss_free(pList, OSS_MEM_HEAP);
   
}


/* 功能描述： 把所给的内容加入到链表的尾*/
unsigned int linkListAddAtTail (T_LINK_CONTAINER *pList, void *pVal)
{
    T_LIST_NODE *pNode = NULL;

    if ((pList == NULL) || (pVal == NULL))
    {
        return LINK_ERR_INVALID_PARAM;
    }

    pNode = linkListNodeNew();    
    if (pNode == NULL)
    {
        return LINK_ERR_MEMORY_MALLOC;
    }
    
    pNode->data = pVal;

    if (pList->pTail == NULL)
    {
        pList->pHead = pNode;
        pList->pTail = pNode;
    } 
    else
    {
        pList->pTail->next = pNode;
        pNode->prev = pList->pTail;
        pList->pTail = pNode;
    }
    
    pList->count++;    

    return 0;
}

/*功能描述： 把所给的内容添加到所给链表的末尾*/
unsigned int  linkListAddAtFront(T_LINK_CONTAINER *pList, void *pData)
{
    T_LIST_NODE *pNode = NULL;

    if ((pList == NULL) || (pData == NULL))
    {
        return LINK_ERR_INVALID_PARAM;
    }

    pNode = linkListNodeNew();    
    if (pNode == NULL)
    {
        return LINK_ERR_MEMORY_MALLOC;
    }
    
    pNode->data = pData;
    

    if (pList->pHead == NULL)
    {
        pList->pHead = pNode;
        pList->pTail = pNode;
    } 
    else
    {
        pNode->next = pList->pHead;
        pList->pHead->prev = pNode;
        pList->pHead = pNode;
    }

    pList->count++;

    
    return 0;

}

/*LinkListRemove*/
unsigned int  linkListRemove(T_LINK_CONTAINER *pList, void *pData)
{
    T_LIST_NODE *pNode = NULL;
    
    if ((pList == NULL) || (pData == NULL))
    {
        return LINK_ERR_INVALID_PARAM;
    }

	pNode = pList->pHead;

	while(pNode != NULL)
	{
		if(pData == pNode->data)
        {
            break;
        }
		pNode = pNode->next;
	}
    
    /* 没有发现要删除的项*/
    if (pNode == NULL)
    {
      
        return LINK_ERR_NO_FIND;
    }

    if (pNode->prev != NULL)
    {
        pNode->prev->next = pNode->next;
    } 
    else
    {
        pList->pHead = pNode->next;
    }

    if (pNode->next != NULL)
    {
        pNode->next->prev = pNode->prev;
    } 
    else
    {
        pList->pTail = pNode->prev;
    }

    pList->count--;

  

    if (pList->del != NULL)
    {
        (*pList->del)(pNode->data);
    }

    linklistNodeFree(pNode);

    return 0;
}

/* 功能描述： 释放链表所有的节点*/

unsigned long linkListRemoveAllNode(T_LINK_CONTAINER *pList)
{
    T_LIST_NODE *pNode = NULL;
    T_LIST_NODE *pNext = NULL;

    if (pList == NULL)
    {
        return LINK_ERR_INVALID_PARAM;
    }

    for (pNode = pList->pHead; pNode; pNode = pNext)
    {
        pNext = pNode->next;
        if ((pList->del != NULL) && (pNode->data != NULL))
        {
            (*pList->del)(pNode->data);
        }
        linklistNodeFree(pNode);
    }

    pList->pHead = NULL;
    pList->pTail = NULL;
    pList->count = 0;

    return 0;
}

/*查找节点*/
T_LIST_NODE *linkListLookup (T_LINK_CONTAINER *pList, void *pData)
{

    T_LIST_NODE *pNode = NULL;
    unsigned long       bFind = 0;

    if ((pList == NULL) || (pData == NULL) || (pList->cmp == NULL))
    {
        return NULL;
    }


    for (pNode = pList->pHead; pNode != NULL; pNode = pNode->next)
    {
        if ((*pList->cmp)(pData,pNode->data) == Link_List_Compare_Match)
        {
            bFind = 1;
            break;
        }
    }


    if(bFind)
    {
        return pNode;
    }
    else
    {
        return NULL;
    }
}

/****************************************************************************
* 函数名称： linkListLookupStart
* 调    用：
* 被 调 用：
* 输入参数： 无
* 输出参数： 无
* 返 回 值： 0 -- 成功 ;其他 -- 错误码
* 功能描述： 开始查找链表,返回符合链表的第一个结构,在链表有信号量的情况下
             必须和linkListLookupEnd配合使用
***************************************************************************/
T_LIST_NODE *linkListProcesserStart(T_LINK_CONTAINER *pList,void *pData)
{
    T_LIST_NODE *pNode = NULL;
    
    if ((pList == NULL) || (pData == NULL))
    {
        return NULL;
    }

    for (pNode = pList->pHead; pNode != NULL; pNode = pNode->next)
    {
        if ((*pList->cmp)(pData,pNode->data) == Link_List_Compare_Match)
        {
            break;
        }
    }
    return pNode;
}

/****************************************************************************
* 函数名称： linkListProcesserNext
* 调    用：
* 被 调 用：
* 输入参数： 无
* 输出参数： 无
* 返 回 值： 0 -- 成功 ;其他 -- 错误码
* 功能描述： 得到要处理的下一个指针
***************************************************************************/
T_LIST_NODE *linkListProcesserNext(T_LINK_CONTAINER *pList,T_LIST_NODE *pNode,void *pData)
{
    T_LIST_NODE *pFindNode = NULL;

    if ((pNode == NULL) || (pData == NULL) || (pData == NULL))
    {
        return NULL;
    }
    for (pFindNode = pNode; pFindNode != NULL; pFindNode = pFindNode->next)
    {
        if ((*pList->cmp)(pData,pFindNode->data) == Link_List_Compare_Match)
        {
            break;
        }
    }
    return pFindNode;
}


/****************************************************************************
* 函数名称： LinkListCompare
* 调    用：
* 被 调 用：pList1--链表容器指针
* 输入参数： 
* 输出参数： 无
* 返 回 值： 无
* 功能描述： 比较两个链表是否完全一致
***************************************************************************/
unsigned long LinkListCompare(T_LINK_CONTAINER *pList1,T_LINK_CONTAINER *pList2)
{
	T_LIST_NODE *pNode = NULL;

	if((!pList1) || (!pList2))
	{
		return Link_List_Compare_Mismatch;
	}
	
	if(pList1->count != pList2->count)
	{
		return Link_List_Compare_Mismatch;
	}
;

	for (pNode = pList1->pHead; pNode != NULL; pNode = pNode->next)
    {
        if(linkListLookup(pList2,pNode->data) == NULL)
        {
        	
			return Link_List_Compare_Mismatch;
		}
    }
	return Link_List_Compare_Match;
}


/***********************************************************************
 *                            局部函数                                 *
 **********************************************************************/

 /****************************************************************************
* 函数名称： linkListNodeNew
* 调    用：
* 被 调 用：
* 输入参数： 无
* 输出参数： 无
* 返 回 值： 链表节点的指针 -- 成功;  空指针 -- 失败
* 功能描述： 为新的链表节点分配一块内存
***************************************************************************/
T_LIST_NODE *linkListNodeNew()
{
    T_LIST_NODE *pNode = NULL;

    pNode = (T_LIST_NODE *)(void *)sal_alloc(sizeof (T_LIST_NODE), "hsl_link_list");

    if ( pNode != NULL)
    {
        memset ((void *)pNode, 0, sizeof (T_LIST_NODE));
    }
    
    return pNode;
}

/****************************************************************************
* 函数名称： linklistNodeFree
* 调    用：
* 被 调 用：
* 输入参数： pNode -- 需要释放的链表节点的指针
* 输出参数： 无
* 返 回 值： 无
* 功能描述： 处理
***************************************************************************/
void linklistNodeFree (T_LIST_NODE *pNode)
{
    if (pNode != NULL)
    {
        oss_free(pNode, OSS_MEM_HEAP);
    }
}

void hsl_list_node_free(void *ptr)
{

     if (ptr != NULL)
    {
        oss_free(ptr, OSS_MEM_HEAP);
    }
}



#if 0
/* VPortFrd链表初始化 */
  g_PrjDrvVPortFrdList = linkContainerNew(LINKLIST_NEED_SEM);
  DDM_CHECK_PTR(rc, g_PrjDrvVPortFrdList);
  g_PrjDrvVPortFrdList->pHead = NULL;
  g_PrjDrvVPortFrdList->pTail = NULL;
  g_PrjDrvVPortFrdList->count = 0;
  g_PrjDrvVPortFrdList->cmp   = PrjDrvDDMVPortFrdCmp;
  g_PrjDrvVPortFrdList->del   = PrjDrvDDMDeleteNode;




   ptVPortFrdNode = linkListLookup(g_PrjDrvVPortFrdList, (void *)&VPortFrdInfo);

    linkListAddAtTail(g_PrjDrvVPortFrdList, (void *)ptVportFrdRecord);

	 linkListRemove(g_PrjDrvDDMForwardTbl, (void *)ptForwardNodeData);
#endif

