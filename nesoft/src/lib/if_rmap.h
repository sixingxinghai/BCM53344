/* Copyright (C) 2001-2003  All Rights Reserved. */

#ifndef _PACOS_IF_RMAP_H
#define _PACOS_IF_RMAP_H

#include "vty.h"

enum if_rmap_type
{
  IF_RMAP_IN,
  IF_RMAP_OUT,
  IF_RMAP_MAX
};

struct if_rmap
{
  /* Name of the interface. */
  char *ifname;

  char *routemap[IF_RMAP_MAX];
};

struct if_rmap_master
{
  /* Hash. */
  struct hash *hash;

  /* Add hook. */
  void (*add_hook) (struct if_rmap *);

  /* Delete hook. */
  void (*delete_hook) (struct if_rmap *);
};

struct if_rmap *if_rmap_lookup (struct if_rmap_master *, char *);
struct if_rmap *if_rmap_set (struct if_rmap_master *, char *,
                             enum if_rmap_type, char *);
result_t if_rmap_unset (struct if_rmap_master *, char *,
                        enum if_rmap_type, char *);

void if_rmap_reset (struct if_rmap_master *);
void if_rmap_master_init (struct if_rmap_master *,
                          void (*) (struct if_rmap *),
                          void (*) (struct if_rmap *));
result_t config_write_if_rmap (struct cli *, struct if_rmap_master *);

#endif /* _PACOS_IF_RMAP_H */
