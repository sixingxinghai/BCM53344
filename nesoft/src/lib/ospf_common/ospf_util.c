/* Copyright (C) 2002  All Rights Reserved. */

#include "pal.h"
#include "lib.h"

#include "ospf_const.h"
#include "ospf_util.h"


/* CLI utility functions. */
int
ospf_str2area_id (char *str, struct pal_in4_addr *area_id, int *format)
{
  char *endptr = NULL;
  unsigned long ret;

  if (str[0] == '-')
    return -1;
  /* match "A.B.C.D". */
  else if (pal_strchr (str, '.') != NULL)
    {
      ret = pal_inet_pton (AF_INET, str, area_id);
      if (!ret)
        return -1;
      *format = OSPF_AREA_ID_FORMAT_ADDRESS;
    }
  /* match "<0-4294967295>". */
  else
    {
      ret = pal_strtou32 (str, &endptr, 10);
      if (*endptr != '\0' || (ret == ULONG_MAX && errno == ERANGE))
        return -1;

      area_id->s_addr = pal_hton32 (ret);
      *format = OSPF_AREA_ID_FORMAT_DECIMAL;
    }

  return 0;
}

int
ospf_str2metric (char *str, int *metric)
{
  /* Sanity check. */
  if (str == NULL)
    return 0;

  *metric = pal_strtos32 (str, NULL, 10);
  if (*metric < 0 && *metric > 16777214)
    return 0;

  return 1;
}

int
ospf_str2metric_type (char *str, int *metric_type)
{
  /* Sanity check. */
  if (str == NULL)
    return 0;

  if (pal_strncmp (str, "1", 1) == 0)
    *metric_type = EXTERNAL_METRIC_TYPE_1;
  else if (pal_strncmp (str, "2", 1) == 0)
    *metric_type = EXTERNAL_METRIC_TYPE_2;
  else
    return 0;

  return 1;
}

int
ospf_str2network_type (char *str, int *network_type)
{
  /* Sanity check.  */
  if (str == NULL)
    return 0;

  if (pal_strncmp (str, "b", 1) == 0)
    *network_type = OSPF_IFTYPE_BROADCAST;
  else if (pal_strncmp (str, "n", 1) == 0)
    *network_type = OSPF_IFTYPE_NBMA;
  else if (pal_strncmp (str, "point-to-m", 10) == 0)
    *network_type = OSPF_IFTYPE_POINTOMULTIPOINT;
  else if (pal_strncmp (str, "point-to-p", 10) == 0)
    *network_type = OSPF_IFTYPE_POINTOPOINT;
  else
    return 0;

  return 1;
}
