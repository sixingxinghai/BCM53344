#include "meg.h"




void meg_master_init(struct meg_master *megg, struct lib_globals *zg)
{
  pal_mem_set (megg, 0, sizeof (struct meg_master));
  megg->zg = zg;
  megg->meg_list = list_new();
  init_stack( &megg->mstack );
}   //10.18 add by czh

int init_stack( struct stack_index * s)
{
 int iTmp = 1;

 s->base = (int*)malloc(STACK_INIT_SIZE*sizeof (int));
 if( NULL == s->base )
 	{
 	 //cli_out(cli, "malloc failed for stack\n");
	 return ERROR;
 	}

 s->top = s->base;
 s->stacksize = STACK_INIT_SIZE;
 while ( iTmp <= MAX_CAP_NUM )
 	{
 	 push_stack(s,iTmp);
	 iTmp++;
 	}
 s->top--;
 return SUCCESS;
}

struct stack_index * destory_stack( struct stack_index *s)
{
  if( NULL == s->base )
  	{
  	 return NULL;
  	}
  //free ( s->base );
  XFREE(MTYPE_NSM_STACK,s->base);
  s->top = s->base = NULL;
  s->stacksize = 0;
  return s;
}

struct stack_index * clear_stack( struct stack_index *s)
{
  if ( NULL == s->base )
  	{
 // 	 cli_out (cli, "stack does not exist.\n");
	 return NULL;
  	}
  s->top = s->base;
  return s;
}

int get_top( struct stack_index *s, int *e)
{
  if (s->top == s->base)
  	{
 // 	 cli_out (cli, "There is no element in the stack.\n");
	 return ERROR;
  	}
  *e = *(s->top);
  return SUCCESS;
}

int push_stack( struct stack_index *s, int e)
{
 if ( ( s->top - s->base ) >= s->stacksize )
 	{
 	 s->base = (int*)realloc(s->base,( STACK_INIT_SIZE + STACK_INCREMENT ) * sizeof(int));
	  if( NULL == s->base )
 	 {
 //	  cli_out(cli, "malloc failed for stack\n");
	  return NULL;
 	 }
	 s->top = s->base + s->stacksize;
	 s->stacksize += STACK_INCREMENT;
 	}
 *(s->top) = e;
 s->top++;
 return SUCCESS;
}

int pop_stack( struct stack_index *s )
{
   int iTmp;
   if (s->top == s->base)
  	{
 // 	 cli_out (cli, "There is no element in the stack.\n");
	 return ERROR;
  	}
   s->top--;
   iTmp = *(s->top);
   return iTmp;
}




















