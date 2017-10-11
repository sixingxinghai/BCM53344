/*  Copyright (C) 2002-2003   All Rights Reserved. */

/*
**
** pal_string.c -- PacOS PAL definitions for string management
*/

#include "pal.h"
#include "pal_string.h"

pal_handle_t 
pal_strstart(struct lib_globals *lib_node) 
{
  return (pal_handle_t) 1;
}

result_t 
pal_strstop(struct lib_globals *lib_node) 
{
  return RESULT_OK;
}

