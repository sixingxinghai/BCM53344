
#ifndef _HSL_LINK_LIST_H_
#define _HSL_LINK_LIST_H_


/* 定义模块通用错误码 */
#ifndef NULL
#define NULL   (0)
#endif

#define LINK_ERR_MEMORY_MALLOC  (1)
#define LINK_ERR_INVALID_PARAM  (2)

/* 定义模块特有错误码 */
#define LINK_ERR_NO_FIND        (3)

#define LINKLIST_NEED_SEM       (4)

#define LINKLIST_SEM_NAME       "LinkSem"


typedef struct T_LIST_NODE
{
  struct T_LIST_NODE *next;
  struct T_LIST_NODE *prev;
  void *data;
}T_LIST_NODE;


/* 链表容器数据结构*/
typedef struct 
{
  T_LIST_NODE *pHead;
  T_LIST_NODE *pTail;
  unsigned int count;
  unsigned int (*cmp) (void *val1, void *val2);  
  void (*del) (void *val);
  unsigned long sem;
} T_LINK_CONTAINER;

/* 链表节点比较结果 */
typedef enum
{
	Link_List_Compare_Match,
	Link_List_Compare_Mismatch
}TCompareResult_e;




T_LINK_CONTAINER *linkContainerNew (unsigned char  bNeedMutex);

void linkListContainerFree(T_LINK_CONTAINER *pList);

unsigned int linkListAddAtFront(T_LINK_CONTAINER *pList, void *pData);

unsigned int linkListAddAtTail (T_LINK_CONTAINER *pList, void *pVal);

unsigned int linkListRemove(T_LINK_CONTAINER *pList, void *pData);

unsigned long linkListRemoveAllNode(T_LINK_CONTAINER *pList);

T_LIST_NODE *linkListLookup (T_LINK_CONTAINER *pList, void *pData);

T_LIST_NODE *linkListProcesserStart(T_LINK_CONTAINER *pList,void *pData);

T_LIST_NODE *linkListProcesserNext(T_LINK_CONTAINER *pList,T_LIST_NODE *pNode,void *pData);

void linkListProcesserEnd(T_LINK_CONTAINER *pList);

unsigned long LinkListCompare(T_LINK_CONTAINER *pList1,T_LINK_CONTAINER *pList2);

void linkListTakeSem(T_LINK_CONTAINER *pList);

void linkListGiveSem(T_LINK_CONTAINER *pList);
void hsl_list_node_free(void *ptr);


#endif

