/* Copyright (C) 2001-2003  All Rights Reserved. */

#include "pal.h"

#include "lib.h"

#ifdef HAVE_TE

#include "admin_grp.h"

/*
 * Initialize administrative group handling.
 */
void
admin_group_array_init (struct admin_group admin_group_array[])
{
  int i;

  /* Set the whole array to NULL */
  for (i = 0; i < ADMIN_GROUP_MAX; i++)
    {
      pal_mem_set (admin_group_array[i].name, '\0', ADMIN_GROUP_NAMSIZ);
      admin_group_array[i].val = ADMIN_GROUP_UNDEF;
    }
}

/*
 * Add a new name/value pair.
 *
 * Please note that val cannot be greater than 31 and no less than 0.
 *
 * All names are added at the (val) index in the admin group array.
 *
 * Neither names or values can be repeated.
 */
result_t
admin_group_add (struct admin_group admin_group_array[], char *name,
                 int val)
{
  int i;

  /* First check whether this name already exists */
  if (pal_strcasecmp (admin_group_array[val].name, name) == 0)
    return ADMIN_GROUP_ERR_DUPLICATE;

  /* Check whether the name is already in use. */
  for (i = 0; i < ADMIN_GROUP_MAX; i++)
    if (pal_strcasecmp (admin_group_array[i].name, name) == 0)
      return ADMIN_GROUP_ERR_DUPLICATE;
  
  /* Check whether some other name already exists at this point */
  if ((admin_group_array[val]).val != ADMIN_GROUP_UNDEF)
    return ADMIN_GROUP_ERR_EEXIST;

  /* Now add this name */
  pal_strcpy (admin_group_array[val].name, name);
  admin_group_array[val].val = val;

  return 0;
}

/*
 * Delete a name/val pair.
 */
result_t
admin_group_del (struct admin_group admin_group_array[], char *name,
                 int val)
{
  /* First check whether this name actually exists */
  if (pal_strcasecmp (admin_group_array[val].name, name) != 0)
    return ADMIN_GROUP_ERR_EEXIST;

  /* Remove this name */
  pal_mem_set (admin_group_array[val].name, '\0', ADMIN_GROUP_NAMSIZ);
  admin_group_array[val].val = ADMIN_GROUP_UNDEF;

  return 0;
}

/*
 * Find the value for the specified name.
 */
int
admin_group_get_val (struct admin_group admin_group_array[], char *name)
{
  int i;

  /* Go through entire array */
  for (i = 0; i < ADMIN_GROUP_MAX; i++)
    {
      if (admin_group_array[i].val != ADMIN_GROUP_UNDEF)
        {
          if (pal_strcmp (name, admin_group_array[i].name) == 0)
            return admin_group_array[i].val;
        }
    }

  return ADMIN_GROUP_UNDEF;
}

/*
 * Find the name for the specified value.
 */
char *
admin_group_get_name (struct admin_group admin_group_array[],
                      int val)
{
  if ((val == ADMIN_GROUP_UNDEF) ||
      (admin_group_array[val].val == ADMIN_GROUP_UNDEF))
    return NULL;
  else
    return admin_group_array[val].name;
}

/* Helper function */
void
admin_group_pad_with_spaces (char str[], char *_strchar, int max_size)
{
  int i;
  int len_diff;

  len_diff = _strchar - str;

  for (i = len_diff; i < max_size - 1; i++)
    str[i] = ' ';
  str[i] = '\0';
}

/* Dump admin group info. */
void
admin_group_dump (struct cli *cli, struct admin_group admin_group_array[])
{
  int i;
  u_char found = 0;

  cli_out (cli, " Admin group detail:");

  for (i = 0; i < ADMIN_GROUP_MAX; i++)
    {
      if (admin_group_array[i].val != ADMIN_GROUP_UNDEF)
        {
          if (found == 0)
            found = 1;

          cli_out (cli, "\n  Value of %d associated with admin group "
                   "\'%s\'", admin_group_array[i].val,
                   admin_group_array[i].name);
        }
    }

  if (found == 0)
    cli_out (cli, " none\n");
  else
    cli_out (cli, "\n");
}

#endif /* #ifdef HAVE_TE */
