/* Copyright (C) 2001-2003  All Rights Reserved. */

#include "pal.h"

#include "lib.h"
#include "distribute.h"
#include "hash.h"


struct distribute *
distribute_new (void)
{
  struct distribute *new;

  new = XCALLOC (MTYPE_DISTRIBUTE, sizeof (struct distribute));
  return new;
}

/* XFREE distribute object. */
void
distribute_free (struct distribute *dist)
{
  if (dist == NULL)
    return;

  if (dist->ifname)
    XFREE (MTYPE_DISTSTR, dist->ifname);

  if (dist->list[DISTRIBUTE_IN])
    XFREE (MTYPE_DISTSTR, dist->list[DISTRIBUTE_IN]);
  if (dist->list[DISTRIBUTE_OUT])
    XFREE (MTYPE_DISTSTR, dist->list[DISTRIBUTE_OUT]);

  if (dist->prefix[DISTRIBUTE_IN])
    XFREE (MTYPE_DISTSTR, dist->prefix[DISTRIBUTE_IN]);
  if (dist->prefix[DISTRIBUTE_OUT])
    XFREE (MTYPE_DISTSTR, dist->prefix[DISTRIBUTE_OUT]);

  XFREE (MTYPE_DISTRIBUTE, dist);
}

/* Lookup interface's distribute list. */
struct distribute *
distribute_lookup (struct distribute_master *dm, char *ifname)
{
  struct distribute key;
  struct distribute *dist;

  key.ifname = ifname;

  dist = hash_lookup (dm->hash, &key);

  return dist;
}

void *
distribute_hash_alloc (struct distribute *arg)
{
  struct distribute *dist;

  dist = distribute_new ();
  if (arg->ifname)
    dist->ifname = XSTRDUP (MTYPE_DISTSTR, arg->ifname);
  else
    dist->ifname = NULL;

  return dist;
}

/* Make new distribute list and push into hash. */
struct distribute *
distribute_get (struct distribute_master *dm, char *ifname)
{
  struct distribute key;

  key.ifname = ifname;

  return hash_get (dm->hash, &key, distribute_hash_alloc);
}

u_int32_t
distribute_hash_make (struct distribute *dist)
{
  u_int32_t key;
  int i;

  key = 0;
  if (dist->ifname)
    for (i = 0; i < pal_strlen (dist->ifname); i++)
      key += dist->ifname[i];

  return key;
}

/* If two distribute-list have same value then return 1 else return
   0. This function is used by hash package. */
bool_t
distribute_cmp (struct distribute *dist1, struct distribute *dist2)
{
  if (dist1->ifname && dist2->ifname)
    if (pal_strcmp (dist1->ifname, dist2->ifname) == 0)
      return 1;
  if (! dist1->ifname && ! dist2->ifname)
    return 1;
  return 0;
}

/* Set access-list name to the distribute list. */
struct distribute *
distribute_list_set (struct distribute_master *dm, char *ifname,
                     enum distribute_type type, char *alist_name)
{
  struct distribute *dist;

  dist = distribute_get (dm, ifname);

  if (dist == NULL)
    return NULL;

  if (type == DISTRIBUTE_IN)
    {
      if (dist->list[DISTRIBUTE_IN])
        XFREE (MTYPE_DISTSTR, dist->list[DISTRIBUTE_IN]);
      dist->list[DISTRIBUTE_IN] = XSTRDUP (MTYPE_DISTSTR,alist_name);
    }
  if (type == DISTRIBUTE_OUT)
    {
      if (dist->list[DISTRIBUTE_OUT])
        XFREE (MTYPE_DISTSTR, dist->list[DISTRIBUTE_OUT]);
      dist->list[DISTRIBUTE_OUT] = XSTRDUP (MTYPE_DISTSTR,alist_name);
    }

  /* Apply this distribute-list to the interface. */
  if (dm->add_hook)
    (*dm->add_hook) (dist);

  return dist;
}

/* Unset distribute-list.  If matched distribute-list exist then
   return 1. */
result_t
distribute_list_unset (struct distribute_master *dm, char *ifname,
                       enum distribute_type type, char *alist_name)
{
  struct distribute *dist;

  dist = distribute_lookup (dm, ifname);
  if (!dist)
    return 0;

  if (type == DISTRIBUTE_IN)
    {
      if (!dist->list[DISTRIBUTE_IN])
        return 0;
      if (pal_strcmp (dist->list[DISTRIBUTE_IN], alist_name) != 0)
        return 0;

      XFREE (MTYPE_DISTSTR, dist->list[DISTRIBUTE_IN]);
      dist->list[DISTRIBUTE_IN] = NULL;
    }

  if (type == DISTRIBUTE_OUT)
    {
      if (!dist->list[DISTRIBUTE_OUT])
        return 0;
      if (pal_strcmp (dist->list[DISTRIBUTE_OUT], alist_name) != 0)
        return 0;

      XFREE(MTYPE_DISTSTR, dist->list[DISTRIBUTE_OUT]);
      dist->list[DISTRIBUTE_OUT] = NULL;
    }

  /* Apply this distribute-list to the interface. */
  if (dm->delete_hook)
    (*dm->delete_hook) (dist);

  /* If both out and in is NULL then XFREEdistribute list. */
  if (dist->list[DISTRIBUTE_IN] == NULL
      && dist->list[DISTRIBUTE_OUT] == NULL
      && dist->prefix[DISTRIBUTE_IN] == NULL
      && dist->prefix[DISTRIBUTE_OUT] == NULL)
    {
      hash_release (dm->hash, dist);
      distribute_free (dist);
    }

  return 1;
}

/* Set access-list name to the distribute list. */
struct distribute *
distribute_list_prefix_set (struct distribute_master *dm, char *ifname,
                            enum distribute_type type, char *plist_name)
{
  struct distribute *dist;

  dist = distribute_get (dm, ifname);

  if (dist == NULL)
    return NULL;

  if (type == DISTRIBUTE_IN)
    {
      if (dist->prefix[DISTRIBUTE_IN])
        XFREE (MTYPE_DISTSTR, dist->prefix[DISTRIBUTE_IN]);
      dist->prefix[DISTRIBUTE_IN] = XSTRDUP (MTYPE_DISTSTR,plist_name);
    }
  if (type == DISTRIBUTE_OUT)
    {
      if (dist->prefix[DISTRIBUTE_OUT])
        XFREE (MTYPE_DISTSTR, dist->prefix[DISTRIBUTE_OUT]);
      dist->prefix[DISTRIBUTE_OUT] = XSTRDUP (MTYPE_DISTSTR,plist_name);
    }

  /* Apply this distribute-list to the interface. */
  if (dm->add_hook)
    (*dm->add_hook) (dist);

  return dist;
}

/* Unset distribute-list.  If matched distribute-list exist then
   return 1. */
result_t
distribute_list_prefix_unset (struct distribute_master *dm, char *ifname,
                              enum distribute_type type, char *plist_name)
{
  struct distribute *dist;

  dist = distribute_lookup (dm, ifname);
  if (!dist)
    return 0;

  if (type == DISTRIBUTE_IN)
    {
      if (!dist->prefix[DISTRIBUTE_IN])
        return 0;
      if (pal_strcmp (dist->prefix[DISTRIBUTE_IN], plist_name) != 0)
        return 0;

      XFREE (MTYPE_DISTSTR, dist->prefix[DISTRIBUTE_IN]);
      dist->prefix[DISTRIBUTE_IN] = NULL;
    }

  if (type == DISTRIBUTE_OUT)
    {
      if (!dist->prefix[DISTRIBUTE_OUT])
        return 0;
      if (pal_strcmp (dist->prefix[DISTRIBUTE_OUT], plist_name) != 0)
        return 0;

      XFREE (MTYPE_DISTSTR, dist->prefix[DISTRIBUTE_OUT]);
      dist->prefix[DISTRIBUTE_OUT] = NULL;
    }

  /* Apply this distribute-list to the interface. */
  if (dm->delete_hook)
    (*dm->delete_hook) (dist);

  /* If both out and in is NULL then XFREEdistribute list. */
  if (dist->list[DISTRIBUTE_IN] == NULL
      && dist->list[DISTRIBUTE_OUT] == NULL
      && dist->prefix[DISTRIBUTE_IN] == NULL
      && dist->prefix[DISTRIBUTE_OUT] == NULL)
    {
      hash_release (dm->hash, dist);
      distribute_free (dist);
    }

  return 1;
}

result_t
config_show_distribute (struct cli *cli, struct distribute_master *dm)
{
  struct distribute *dist;
  struct hash_backet *mp;
  int i;

  /* Output filter configuration. */
  dist = distribute_lookup (dm, NULL);
  if (dist && (dist->list[DISTRIBUTE_OUT] || dist->prefix[DISTRIBUTE_OUT]))
    {
      cli_out (cli, "  Outgoing update filter list for all interface is");
      if (dist->list[DISTRIBUTE_OUT])
        cli_out (cli, " %s", dist->list[DISTRIBUTE_OUT]);
      if (dist->prefix[DISTRIBUTE_OUT])
        cli_out (cli, "%s (prefix-list) %s",
                 dist->list[DISTRIBUTE_OUT] ? "," : "",
                 dist->prefix[DISTRIBUTE_OUT]);
      cli_out (cli, "\n");
    }
  else
    cli_out (cli, "  Outgoing update filter list for all interface is not set\n");

  for (i = 0; i < dm->hash->size; i++)
    for (mp = dm->hash->index[i]; mp; mp = mp->next)
      {
        dist = mp->data;
        if (dist->ifname)
          if (dist->list[DISTRIBUTE_OUT] || dist->prefix[DISTRIBUTE_OUT])
            {
              cli_out (cli, "    %s filtered by", dist->ifname);
              if (dist->list[DISTRIBUTE_OUT])
                cli_out (cli, " %s", dist->list[DISTRIBUTE_OUT]);
              if (dist->prefix[DISTRIBUTE_OUT])
                cli_out (cli, "%s (prefix-list) %s",
                         dist->list[DISTRIBUTE_OUT] ? "," : "",
                         dist->prefix[DISTRIBUTE_OUT]);
              cli_out (cli, "\n");
            }
      }


  /* Input filter configuration. */
  dist = distribute_lookup (dm, NULL);
  if (dist && (dist->list[DISTRIBUTE_IN] || dist->prefix[DISTRIBUTE_IN]))
    {
      cli_out (cli, "  Incoming update filter list for all interface is");
      if (dist->list[DISTRIBUTE_IN])
        cli_out (cli, " %s", dist->list[DISTRIBUTE_IN]);
      if (dist->prefix[DISTRIBUTE_IN])
        cli_out (cli, "%s (prefix-list) %s",
                 dist->list[DISTRIBUTE_IN] ? "," : "",
                 dist->prefix[DISTRIBUTE_IN]);
      cli_out (cli, "\n");
    }
  else
    cli_out (cli, "  Incoming update filter list for all interface is not set\n");

  for (i = 0; i < dm->hash->size; i++)
    for (mp = dm->hash->index[i]; mp; mp = mp->next)
      {
        dist = mp->data;
        if (dist->ifname)
          if (dist->list[DISTRIBUTE_IN] || dist->prefix[DISTRIBUTE_IN])
            {
              cli_out (cli, "    %s filtered by", dist->ifname);
              if (dist->list[DISTRIBUTE_IN])
                cli_out (cli, " %s", dist->list[DISTRIBUTE_IN]);
              if (dist->prefix[DISTRIBUTE_IN])
                cli_out (cli, "%s (prefix-list) %s",
                         dist->list[DISTRIBUTE_IN] ? "," : "",
                         dist->prefix[DISTRIBUTE_IN]);
              cli_out (cli, "\n");
            }
      }
  return 0;
}

/* Configuration write function. */
result_t
config_write_distribute (struct cli *cli, struct distribute_master *dm)
{
  int i;
  struct hash_backet *mp;
  int write = 0;

  for (i = 0; i < dm->hash->size; i++)
    for (mp = dm->hash->index[i]; mp; mp = mp->next)
      {
        struct distribute *dist;

        dist = mp->data;

        if (dist->list[DISTRIBUTE_IN])
          {
            cli_out (cli, " distribute-list %s in %s\n",
                     dist->list[DISTRIBUTE_IN],
                     dist->ifname ? dist->ifname : "");
            write++;
          }

        if (dist->list[DISTRIBUTE_OUT])
          {
            cli_out (cli, " distribute-list %s out %s\n",

                     dist->list[DISTRIBUTE_OUT],
                     dist->ifname ? dist->ifname : "");
            write++;
          }

        if (dist->prefix[DISTRIBUTE_IN])
          {
            cli_out (cli, " distribute-list prefix %s in %s\n",
                     dist->prefix[DISTRIBUTE_IN],
                     dist->ifname ? dist->ifname : "");
            write++;
          }

        if (dist->prefix[DISTRIBUTE_OUT])
          {
            cli_out (cli, " distribute-list prefix %s out %s\n",
                     dist->prefix[DISTRIBUTE_OUT],
                     dist->ifname ? dist->ifname : "");
            write++;
          }
      }
  return write;
}

/* Clear all distribute list. */
void
distribute_list_reset (struct distribute_master *dm)
{
  if (dm->hash != NULL)
    {
      hash_clean (dm->hash, (void (*) (void *)) distribute_free);
      hash_free (dm->hash);
      dm->hash = NULL;
    }
}

void
distribute_master_init (struct distribute_master *dm,
                        void (*add_hook) (struct distribute *),
                        void (*delete_hook) (struct distribute *))
{
  if (dm->hash == NULL)
    dm->hash = hash_create (distribute_hash_make, distribute_cmp);
  dm->add_hook = add_hook;
  dm->delete_hook = delete_hook;
}
