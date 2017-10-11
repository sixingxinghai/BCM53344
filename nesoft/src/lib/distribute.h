/* Copyright (C) 2001-2003  All Rights Reserved. */

#ifndef _PACOS_DISTRIBUTE_H
#define _PACOS_DISTRIBUTE_H

#include "vty.h"
#include "prefix.h"
#include "if.h"
#include "filter.h"

/* Distribute list types. */
enum distribute_type
{
  DISTRIBUTE_IN,
  DISTRIBUTE_OUT,
  DISTRIBUTE_MAX
};

/* Distribute list. */
struct distribute
{
  /* Name of the interface. */
  char *ifname;

  /* Filter name of `in' and `out' */
  char *list[DISTRIBUTE_MAX];

  /* prefix-list name of `in' and `out' */
  char *prefix[DISTRIBUTE_MAX];
};

/* Distribute list master. */
struct distribute_master
{
  /* Hash. */
  struct hash *hash;

  /* Add hook. */
  void (*add_hook) (struct distribute *);

  /* Delete hook. */
  void (*delete_hook) (struct distribute *);
};

/* Prototypes for distribute-list. */
void distribute_free (struct distribute *);
struct distribute *distribute_lookup (struct distribute_master *, char *);
void distribute_list_clean (struct distribute_master *);
struct distribute *distribute_list_set (struct distribute_master *, char *,
                                        enum distribute_type, char *);
result_t distribute_list_unset (struct distribute_master *, char *,
                                enum distribute_type, char *);
struct distribute *distribute_list_prefix_set (struct distribute_master *,
                                               char *, enum distribute_type,
                                               char *);
result_t distribute_list_prefix_unset (struct distribute_master *, char *,
                                       enum distribute_type, char *);

result_t config_write_distribute (struct cli *, struct distribute_master *);
result_t config_show_distribute (struct cli *, struct distribute_master *);
void distribute_list_reset (struct distribute_master *);
void distribute_master_init (struct distribute_master *,
                             void (*) (struct distribute *),
                             void (*) (struct distribute *));

#endif /* _PACOS_DISTRIBUTE_H */
