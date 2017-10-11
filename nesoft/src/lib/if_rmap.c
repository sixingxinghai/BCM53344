/* Copyright (C) 2001-2003  All Rights Reserved. */

#include "pal.h"

#include "lib.h"
#include "if_rmap.h"
#include "hash.h"


struct if_rmap *
if_rmap_new (void)
{
  struct if_rmap *new;

  new = XCALLOC (MTYPE_IF_RMAP, sizeof (struct if_rmap));

  return new;
}

void
if_rmap_free (struct if_rmap *if_rmap)
{
  if (if_rmap->ifname)
    XFREE (MTYPE_IF_RMAP_NAME, if_rmap->ifname);

  if (if_rmap->routemap[IF_RMAP_IN])
    XFREE (MTYPE_IF_RMAP_NAME, if_rmap->routemap[IF_RMAP_IN]);
  if (if_rmap->routemap[IF_RMAP_OUT])
    XFREE (MTYPE_IF_RMAP_NAME, if_rmap->routemap[IF_RMAP_OUT]);

  XFREE(MTYPE_IF_RMAP, if_rmap);
}

struct if_rmap *
if_rmap_lookup (struct if_rmap_master *ifrm, char *ifname)
{
  struct if_rmap key;
  struct if_rmap *if_rmap;

  key.ifname = ifname;

  if_rmap = hash_lookup (ifrm->hash, &key);
  
  return if_rmap;
}

void *
if_rmap_hash_alloc (struct if_rmap *arg)
{
  struct if_rmap *if_rmap;

  if_rmap = if_rmap_new ();
  if_rmap->ifname = XSTRDUP (MTYPE_IF_RMAP_NAME,arg->ifname);

  return if_rmap;
}

struct if_rmap *
if_rmap_get (struct if_rmap_master *ifrm, char *ifname)
{
  struct if_rmap key;

  key.ifname = ifname;

  return (struct if_rmap *) hash_get (ifrm->hash, &key, if_rmap_hash_alloc);
}

u_int32_t
if_rmap_hash_make (struct if_rmap *if_rmap)
{
  u_int32_t key;
  s_int32_t i;

  key = 0;
  for (i = 0; i < pal_strlen (if_rmap->ifname); i++)
    key += if_rmap->ifname[i];

  return key;
}

bool_t
if_rmap_hash_cmp (struct if_rmap *if_rmap1, struct if_rmap *if_rmap2)
{
  if (pal_strcmp (if_rmap1->ifname, if_rmap2->ifname) == 0)
    return 1;
  return 0;
}

struct if_rmap *
if_rmap_set (struct if_rmap_master *ifrm, char *ifname,
             enum if_rmap_type type, char *routemap_name)
{
  struct if_rmap *if_rmap;

  if_rmap = if_rmap_get (ifrm, ifname);

  if (if_rmap == NULL)
    return NULL;

  if (type == IF_RMAP_IN)
    {
      if (if_rmap->routemap[IF_RMAP_IN])
        XFREE (MTYPE_IF_RMAP_NAME, if_rmap->routemap[IF_RMAP_IN]);
      if_rmap->routemap[IF_RMAP_IN] =
        XSTRDUP (MTYPE_IF_RMAP_NAME, routemap_name);
    }
  if (type == IF_RMAP_OUT)
    {
      if (if_rmap->routemap[IF_RMAP_OUT])
        XFREE (MTYPE_IF_RMAP_NAME, if_rmap->routemap[IF_RMAP_OUT]);
      if_rmap->routemap[IF_RMAP_OUT] =
        XSTRDUP (MTYPE_IF_RMAP_NAME, routemap_name);
    }

  if (ifrm->add_hook)
    (*(ifrm->add_hook)) (if_rmap);
  
  return if_rmap;
}

result_t
if_rmap_unset (struct if_rmap_master *ifrm, char *ifname,
               enum if_rmap_type type, char *routemap_name)
{
  struct if_rmap *if_rmap;

  if_rmap = if_rmap_lookup (ifrm, ifname);
  if (!if_rmap)
    return 0;

  if (type == IF_RMAP_IN)
    {
      if (!if_rmap->routemap[IF_RMAP_IN])
        return 0;
      if (pal_strcmp (if_rmap->routemap[IF_RMAP_IN], routemap_name) != 0)
        return 0;

      XFREE (MTYPE_IF_RMAP_NAME, if_rmap->routemap[IF_RMAP_IN]);
      if_rmap->routemap[IF_RMAP_IN] = NULL;      
    }

  if (type == IF_RMAP_OUT)
    {
      if (!if_rmap->routemap[IF_RMAP_OUT])
        return 0;
      if (pal_strcmp (if_rmap->routemap[IF_RMAP_OUT], routemap_name) != 0)
        return 0;

      XFREE (MTYPE_IF_RMAP_NAME, if_rmap->routemap[IF_RMAP_OUT]);
      if_rmap->routemap[IF_RMAP_OUT] = NULL;      
    }

  if (ifrm->delete_hook)
    (*ifrm->delete_hook) (if_rmap);

  if (if_rmap->routemap[IF_RMAP_IN] == NULL
      && if_rmap->routemap[IF_RMAP_OUT] == NULL)
    {
      hash_release (ifrm->hash, if_rmap);
      if_rmap_free (if_rmap);
    }

  return 1;
}



/* Configuration write function. */
result_t
config_write_if_rmap (struct cli *cli, struct if_rmap_master *ifrm)
{
  struct if_rmap *if_rmapx;
  struct hash_backet *mp;
  s_int32_t i;
  s_int32_t write = 0;

  for (i = 0; i < ifrm->hash->size; i++)
    for (mp = ifrm->hash->index[i]; mp; mp = mp->next)
      {

        if_rmapx = mp->data;

        if (if_rmapx->routemap[IF_RMAP_IN])
          {
            cli_out (cli, " route-map %s in %s\n", 
                     if_rmapx->routemap[IF_RMAP_IN],
                     if_rmapx->ifname);
            write++;
          }

        if (if_rmapx->routemap[IF_RMAP_OUT])
          {
            cli_out (cli, " route-map %s out %s\n", 
                     if_rmapx->routemap[IF_RMAP_OUT],
                     if_rmapx->ifname);
            write++;
          }
      }
  return write;
}

void
if_rmap_reset (struct if_rmap_master *ifrm)
{
  hash_clean (ifrm->hash, (void (*) (void *)) if_rmap_free);
}

void
if_rmap_master_init (struct if_rmap_master *ifrm,
                     void (*add_hook) (struct if_rmap *),
                     void (*delete_hook) (struct if_rmap *))
{
  ifrm->hash = hash_create (if_rmap_hash_make, if_rmap_hash_cmp);
  ifrm->add_hook = add_hook;
  ifrm->delete_hook = delete_hook;
}
