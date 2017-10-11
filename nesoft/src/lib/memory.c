/* Copyright (C) 2003  All Rights Reserved. */

#include "pal.h"

#include "modbmap.h"
#include "memory.h"

#ifdef MEMMGR
#include "memmgr/memmgr.h"
#include "memmgr/memmgr_config.h"
#endif /* MEMMGR */

#ifndef MEMMGR
static struct memory_global mem_global;
#define MEM_ZG mem_global.zg
#endif /* !MEMMGR */

#include "lib_fm_api.h"

/* set this flag to initialize memory manager only once */
static int memory_init_flag;

/* bit coded active PM's */
static modbmap_t active_pms;

/*
 *  memory_active_modules ()
 *
 *  Return a bit coded active protocol modules.
 */
modbmap_t
memory_active_modules (void)
{
    return active_pms;
}

int
memory_init (int pm)
{
    int  ret = 0;
#ifndef MEMMGR
    int  mtype;
#endif /* MEMMGR */

    /* keep note of active protocol modules - always enable lib/pal module */
    MODBMAP_SET (active_pms, pm);
    MODBMAP_SET (active_pms, APN_PROTO_LIB);

    /* return if already initialized */
    if (memory_init_flag != 0)
      return ret;

    /* set flag so as to call memmgr_init_memtable only once */
    memory_init_flag = 1;

#ifdef MEMMGR
    ret = memmgr_init_memtable();
    memmgr_qsort_mtype_array();
#else
   for (mtype = MTYPE_TMP; mtype < MTYPE_MAX; mtype++)
     pal_mem_type_init (mtype);
#endif /* MEMMGR */

    return ret;
}

int
memory_finish (void)
{
    int ret = 0;
#ifndef MEMMGR
    int mtype;
#endif /* MEMMGR */

#ifdef MEMMGR
    ret = memmgr_free_memtable ();
#else
    for (mtype = MTYPE_TMP; mtype < MTYPE_MAX; mtype++)
      pal_mem_type_deinit (mtype);
#endif /* MEMMGR */

    return ret;
}

void
memory_set_lg (void *lg)
{
#ifdef MEMMGR
  memmgr_set_lg (lg);
#endif /* MEMMGR */
  return;
}

void
memory_unset_lg (void *lg)
{
#ifdef MEMMGR
  memmgr_unset_lg (lg);
#endif /* MEMMGR */
  return;
}

/* Purpose of the following memory allocation function wrappers are */
/* to trap any memory allocation failure, and generate a core-file. */
/* This makes investigation of the failed process possible.         */

#ifdef MEMMGR

void *
mfh_malloc (int size, int mtype, char *filename, int line)
{
  void *p = memmgr_malloc (size, mtype, filename, line);
  if (p)
    return p;
  pal_assert (0);
  return NULL; /* Not Reached */
}

void *
mfh_calloc (int size, int mtype, char *filename, int line)
{
  void *p = memmgr_calloc (size, mtype, filename, line);
  if (p)
    return p;
  pal_assert (0);
  return NULL; /* Not Reached */
}

void *
mfh_realloc (void *ptr, int new_size, int mtype, char *filename, int line)
{
  void *p = memmgr_realloc (ptr, new_size, mtype, filename, line);
  if (p)
    return p;
  pal_assert (0);
  return NULL; /* Not Reached */
}

char *
mfh_strdup (const char *str, int mtype, char *filename, int line)
{
  char *p = memmgr_strdup (str, mtype, filename, line);
  if (p)
    return p;
  pal_assert (0);
  return NULL; /* Not Reached */
}

#else

void *
mfh_malloc (enum memory_type type, size_t size)
{
  void *p = pal_mem_malloc (type, size);
  if (p)
    return p;
  pal_assert (0);
  return NULL; /* Not Reached */
}

void *
mfh_calloc (enum memory_type type, size_t size)
{
  void *p = pal_mem_calloc (type, size);
  if (p)
    return p;
  /* Record the fault */
#ifdef MEMMGR
  RecordMemFailureFault(MEMMGR_LG);
#else
  RecordMemFailureFault(MEM_ZG);
#endif
  pal_assert (0);
  return NULL; /* Not Reached */
}

void *
mfh_realloc (enum memory_type type, void *ptr, size_t size)
{
  void *p = pal_mem_realloc (type, ptr, size);
  if (p)
    return p;
  pal_assert (0);
  return NULL; /* Not Reached */
}

char *
mfh_strdup (enum memory_type type, const char *str)
{
  char *p = pal_strdup (type, str);
  if (p)
    return p;
  pal_assert (0);
  return NULL; /* Not Reached */
}

void
memory_set_zg (void *zg)
{
  if (MEM_ZG == NULL)
    MEM_ZG = (struct lib_globals *)zg;
  return;
}

void
memory_unset_zg (void *zg)
{
  if (MEM_ZG == (struct lib_globals *)zg)
    MEM_ZG = NULL;
  return;
}
#endif /* MEMMGR */

/* Record memory failure */
void
RecordMemFailureFault (struct lib_globals *zg)
{
  u_int16_t groupId;

  xlate_groupid_from_proto(zg->protocol, &groupId);

  FM_REPORT (zg, groupId, FM_MEMORY_ALLOCATION_FAILURE);

  return;
}

void
xlate_groupid_from_proto (int protocol, u_int16_t *groupId)
{
  /* Get FM Group ID from protocol */
  switch(protocol)
  {
    case APN_PROTO_NSM:
      *groupId = FM_GID_NSM;
    break;
    case APN_PROTO_LACP:
      *groupId = FM_GID_LACP;
    break;
    case APN_PROTO_MSTP:
      *groupId = FM_GID_MSTP;
    break;
    case APN_PROTO_RMON:
      *groupId = FM_GID_RMON;
    break;
    case APN_PROTO_ONM:
      *groupId = FM_GID_ONM;
    break;
    case APN_PROTO_IMI:
      *groupId = FM_GID_IMI;
    break;
    default:
      return;
  }
  return;
}

