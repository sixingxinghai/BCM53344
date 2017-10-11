/* Copyright (C) 2001-2005  All Rights Reserved. */

#include "igmp_incl.h"

/*
 * IGMP CLI Global Configuration Commands
 */

#ifdef HAVE_VRF
CLI (ip_igmp_limit,
     ip_igmp_limit_cmd,
     "ip igmp (vrf NAME|) limit"
     IGMP_CLI_RANGE_STR (IGMP_LIMIT_MIN,
                         IGMP_LIMIT_MAX)
     " (except (<1-99>|<1300-1999>|WORD) |)",
     CLI_IP_STR,
     CLI_IGMP_STR,
     CLI_VRF_STR,
     CLI_VRF_NAME_STR,
     "IGMP Limit",
     "Max Allowed State Globally"
     IGMP_CLI_DEF_VAL_STR (IGMP_LIMIT_DEF),
     "Exception Access List",
     "Access list number",
     "Access list number (expanded range)",
     "IP Named Standard Access list")
#else
CLI (ip_igmp_limit,
     ip_igmp_limit_cmd,
     "ip igmp limit"
     IGMP_CLI_RANGE_STR (IGMP_LIMIT_MIN,
                         IGMP_LIMIT_MAX)
     " (except (<1-99>|<1300-1999>|WORD) |)",
     CLI_IP_STR,
     CLI_IGMP_STR,
     "IGMP Limit",
     "Max Allowed State Globally"
     IGMP_CLI_DEF_VAL_STR (IGMP_LIMIT_DEF),
     "Exception Access List",
     "Access list number",
     "Access list number (expanded range)",
     "IP Named Standard Access list")
#endif /* HAVE_VRF */
{
  struct igmp_instance *igi;
  u_int32_t state_limit;
  struct apn_vrf *ivrf;
  u_int8_t *acl_arg;
  s_int32_t ret;

  IGMP_FN_ENTER ();

  ret = IGMP_ERR_NONE;
  acl_arg = NULL;

  /* Get the VRF Instance and Limit value */
  if (argc > 1
      && ! pal_strcmp (argv [0], "vrf"))
    {
      ivrf = apn_vrf_lookup_by_name (LIB_GLOB_GET_VR_CONTEXT (cli->zg), argv [1]);

      CLI_GET_UINT32_RANGE ("Limit",
                            state_limit, argv [2],
                            IGMP_LIMIT_MIN,
                            IGMP_LIMIT_MAX);

      acl_arg = argv [4];
    }
  else
    {
      ivrf = apn_vrf_lookup_by_name (LIB_GLOB_GET_VR_CONTEXT (cli->zg), NULL);

      CLI_GET_UINT32_RANGE ("Limit",
                            state_limit, argv [0],
                            IGMP_LIMIT_MIN,
                            IGMP_LIMIT_MAX);

      acl_arg = argv [2];
    }

  if (! ivrf)
    {
      ret = IGMP_ERR_NO_CONTEXT_INFO;
      goto EXIT;
    }

  /* Get the IGMP Instance */
  igi = LIB_VRF_GET_IGMP_INSTANCE (ivrf);

  if (! igi)
    {
      ret = IGMP_ERR_NO_CONTEXT_INFO;
      goto EXIT;
    }

  ret = igmp_limit_set (igi, state_limit, acl_arg);

EXIT:

  IGMP_FN_EXIT (igmp_cli_return (cli, ret));
}

#ifdef HAVE_VRF
CLI (no_ip_igmp_limit,
     no_ip_igmp_limit_cmd,
     "no ip igmp (vrf NAME|) limit",
     CLI_NO_STR,
     CLI_IP_STR,
     CLI_IGMP_STR,
     CLI_VRF_STR,
     CLI_VRF_NAME_STR,
     "IGMP Limit")
#else
CLI (no_ip_igmp_limit,
     no_ip_igmp_limit_cmd,
     "no ip igmp limit",
     CLI_NO_STR,
     CLI_IP_STR,
     CLI_IGMP_STR,
     "IGMP Limit")
#endif /* HAVE_VRF */
{
  struct igmp_instance *igi;
  struct apn_vrf *ivrf;
  s_int32_t ret;

  IGMP_FN_ENTER ();

  ret = IGMP_ERR_NONE;

  /* Get the VRF Instance */
  if (argc > 1
      && ! pal_strcmp (argv [0], "vrf"))
    ivrf = apn_vrf_lookup_by_name (LIB_GLOB_GET_VR_CONTEXT (cli->zg), argv [1]);
  else
    ivrf = apn_vrf_lookup_by_name (LIB_GLOB_GET_VR_CONTEXT (cli->zg), NULL);

  if (! ivrf)
    {
      ret = IGMP_ERR_NO_CONTEXT_INFO;
      goto EXIT;
    }

  /* Get the IGMP Instance */
  igi = LIB_VRF_GET_IGMP_INSTANCE (ivrf);

  if (! igi)
    {
      ret = IGMP_ERR_NO_CONTEXT_INFO;
      goto EXIT;
    }

  ret = igmp_limit_unset (igi);

EXIT:

  IGMP_FN_EXIT (igmp_cli_return (cli, ret));
}

#ifdef HAVE_VRF
CLI (ip_igmp_snooping,
     ip_igmp_snooping_cmd,
     "ip igmp (vrf NAME|) snooping",
     CLI_IP_STR,
     CLI_IGMP_STR,
     CLI_VRF_STR,
     CLI_VRF_NAME_STR,
     CLI_IGMP_SNOOP_STR)
#else
CLI (ip_igmp_snooping,
     ip_igmp_snooping_cmd,
     "ip igmp snooping",
     CLI_IP_STR,
     CLI_IGMP_STR,
     CLI_IGMP_SNOOP_STR)
#endif /* HAVE_VRF */
{
  struct igmp_instance *igi;
  struct apn_vrf *ivrf;
  s_int32_t ret;

  IGMP_FN_ENTER ();

  ret = IGMP_ERR_NONE;

  /* Get the VRF Instance */
  if (argc > 1
      && ! pal_strcmp (argv [0], "vrf"))
    ivrf = apn_vrf_lookup_by_name (LIB_GLOB_GET_VR_CONTEXT (cli->zg), argv [1]);
  else
    ivrf = apn_vrf_lookup_by_name (LIB_GLOB_GET_VR_CONTEXT (cli->zg), NULL);

  if (! ivrf)
    {
      ret = IGMP_ERR_NO_CONTEXT_INFO;
      goto EXIT;
    }

  /* Get the IGMP Instance */
  igi = LIB_VRF_GET_IGMP_INSTANCE (ivrf);

  if (! igi)
    {
      ret = IGMP_ERR_NO_CONTEXT_INFO;
      goto EXIT;
    }

  ret = igmp_snooping_set (igi);

EXIT:

  IGMP_FN_EXIT (igmp_cli_return (cli, ret));
}

#ifdef HAVE_VRF
CLI (no_ip_igmp_snooping,
     no_ip_igmp_snooping_cmd,
     "no ip igmp (vrf NAME|) snooping",
     CLI_NO_STR,
     CLI_IP_STR,
     CLI_IGMP_STR,
     CLI_VRF_STR,
     CLI_VRF_NAME_STR,
     CLI_IGMP_SNOOP_STR)
#else
CLI (no_ip_igmp_snooping,
     no_ip_igmp_snooping_cmd,
     "no ip igmp snooping",
     CLI_NO_STR,
     CLI_IP_STR,
     CLI_IGMP_STR,
     CLI_IGMP_SNOOP_STR)
#endif /* HAVE_VRF */
{
  struct igmp_instance *igi;
  struct apn_vrf *ivrf;
  s_int32_t ret;

  IGMP_FN_ENTER ();

  ret = IGMP_ERR_NONE;

  /* Get the VRF Instance */
  if (argc > 1
      && ! pal_strcmp (argv [0], "vrf"))
    ivrf = apn_vrf_lookup_by_name (LIB_GLOB_GET_VR_CONTEXT (cli->zg), argv [1]);
  else
    ivrf = apn_vrf_lookup_by_name (LIB_GLOB_GET_VR_CONTEXT (cli->zg), NULL);

  if (! ivrf)
    {
      ret = IGMP_ERR_NO_CONTEXT_INFO;
      goto EXIT;
    }

  /* Get the IGMP Instance */
  igi = LIB_VRF_GET_IGMP_INSTANCE (ivrf);

  if (! igi)
    {
      ret = IGMP_ERR_NO_CONTEXT_INFO;
      goto EXIT;
    }

  ret = igmp_snooping_unset (igi);

EXIT:

  IGMP_FN_EXIT (igmp_cli_return (cli, ret));
}

#ifdef HAVE_VRF
CLI (ip_igmp_ssm_map_enable,
     ip_igmp_ssm_map_enable_cmd,
     "ip igmp (vrf NAME|) ssm-map enable",
     CLI_IP_STR,
     CLI_IGMP_STR,
     CLI_VRF_STR,
     CLI_VRF_NAME_STR,
     "SSM Mapping for IGMPv1/v2 Groups",
     "Enable SSM Mapping")
#else
CLI (ip_igmp_ssm_map_enable,
     ip_igmp_ssm_map_enable_cmd,
     "ip igmp ssm-map enable",
     CLI_IP_STR,
     CLI_IGMP_STR,
     "SSM Mapping for IGMPv1/v2 Groups",
     "Enable SSM Mapping")
#endif /* HAVE_VRF */
{
  struct igmp_instance *igi;
  struct apn_vrf *ivrf;
  s_int32_t ret;

  IGMP_FN_ENTER ();

  ret = IGMP_ERR_NONE;

  /* Get the VRF Instance */
  if (argc > 1
      && ! pal_strcmp (argv [0], "vrf"))
    ivrf = apn_vrf_lookup_by_name (LIB_GLOB_GET_VR_CONTEXT (cli->zg), argv [1]);
  else
    ivrf = apn_vrf_lookup_by_name (LIB_GLOB_GET_VR_CONTEXT (cli->zg), NULL);

  if (! ivrf)
    {
      ret = IGMP_ERR_NO_CONTEXT_INFO;
      goto EXIT;
    }

  /* Get the IGMP Instance */
  igi = LIB_VRF_GET_IGMP_INSTANCE (ivrf);

  if (! igi)
    {
      ret = IGMP_ERR_NO_CONTEXT_INFO;
      goto EXIT;
    }

  ret = igmp_ssm_map_enable_set (igi);

EXIT:

  IGMP_FN_EXIT (igmp_cli_return (cli, ret));
}

#ifdef HAVE_VRF
CLI (no_ip_igmp_ssm_map_enable,
     no_ip_igmp_ssm_map_enable_cmd,
     "no ip igmp (vrf NAME|) ssm-map enable",
     CLI_NO_STR,
     CLI_IP_STR,
     CLI_IGMP_STR,
     CLI_VRF_STR,
     CLI_VRF_NAME_STR,
     "SSM Mapping for IGMPv1/v2 Groups",
     "Enable SSM Mapping")
#else
CLI (no_ip_igmp_ssm_map_enable,
     no_ip_igmp_ssm_map_enable_cmd,
     "no ip igmp ssm-map enable",
     CLI_NO_STR,
     CLI_IP_STR,
     CLI_IGMP_STR,
     "SSM Mapping for IGMPv1/v2 Groups",
     "Enable SSM Mapping")
#endif /* HAVE_VRF */
{
  struct igmp_instance *igi;
  struct apn_vrf *ivrf;
  s_int32_t ret;

  IGMP_FN_ENTER ();

  ret = IGMP_ERR_NONE;

  /* Get the VRF Instance */
  if (argc > 1
      && ! pal_strcmp (argv [0], "vrf"))
    ivrf = apn_vrf_lookup_by_name (LIB_GLOB_GET_VR_CONTEXT (cli->zg), argv [1]);
  else
    ivrf = apn_vrf_lookup_by_name (LIB_GLOB_GET_VR_CONTEXT (cli->zg), NULL);

  if (! ivrf)
    {
      ret = IGMP_ERR_NO_CONTEXT_INFO;
      goto EXIT;
    }

  /* Get the IGMP Instance */
  igi = LIB_VRF_GET_IGMP_INSTANCE (ivrf);

  if (! igi)
    {
      ret = IGMP_ERR_NO_CONTEXT_INFO;
      goto EXIT;
    }

  ret = igmp_ssm_map_enable_unset (igi);

EXIT:

  IGMP_FN_EXIT (igmp_cli_return (cli, ret));
}

#ifdef HAVE_VRF
CLI (ip_igmp_ssm_map_static,
     ip_igmp_ssm_map_static_cmd,
     "ip igmp (vrf NAME|) ssm-map static (<1-99>|<1300-1999>|WORD) A.B.C.D",
     CLI_IP_STR,
     CLI_IGMP_STR,
     CLI_VRF_STR,
     CLI_VRF_NAME_STR,
     "SSM Mapping for IGMPv1/v2 Groups",
     "Static SSM Mapping",
     "Access list number",
     "Access list number (expanded range)",
     "IP Named Standard Access list",
     "Source address to use for static map group")
#else
CLI (ip_igmp_ssm_map_static,
     ip_igmp_ssm_map_static_cmd,
     "ip igmp ssm-map static (<1-99>|<1300-1999>|WORD) A.B.C.D",
     CLI_IP_STR,
     CLI_IGMP_STR,
     "SSM Mapping for IGMPv1/v2 Groups",
     "Static SSM Mapping",
     "Access list number",
     "Access list number (expanded range)",
     "IP Named Standard Access list",
     "Source address to use for static map group")
#endif /* HAVE_VRF */
{
  struct igmp_instance *igi;
  struct apn_vrf *ivrf;
  s_int32_t ret;

  IGMP_FN_ENTER ();

  ret = IGMP_ERR_NONE;

  /* Get the VRF Instance */
  if (argc > 2
      && ! pal_strcmp (argv [0], "vrf"))
    ivrf = apn_vrf_lookup_by_name (LIB_GLOB_GET_VR_CONTEXT (cli->zg), argv [1]);
  else
    ivrf = apn_vrf_lookup_by_name (LIB_GLOB_GET_VR_CONTEXT (cli->zg), NULL);

  if (! ivrf)
    {
      ret = IGMP_ERR_NO_CONTEXT_INFO;
      goto EXIT;
    }

  /* Get the IGMP Instance */
  igi = LIB_VRF_GET_IGMP_INSTANCE (ivrf);

  if (! igi)
    {
      ret = IGMP_ERR_NO_CONTEXT_INFO;
      goto EXIT;
    }

  ret = igmp_ssm_map_static_set (igi, argc > 2 ? argv [2] : argv [0],
                                 argc > 2 ? argv [3] : argv [1]);

EXIT:

  IGMP_FN_EXIT (igmp_cli_return (cli, ret));
}

#ifdef HAVE_VRF
CLI (no_ip_igmp_ssm_map_static,
     no_ip_igmp_ssm_map_static_cmd,
     "no ip igmp (vrf NAME|) ssm-map static (<1-99>|<1300-1999>|WORD) A.B.C.D",
     CLI_NO_STR,
     CLI_IP_STR,
     CLI_IGMP_STR,
     CLI_VRF_STR,
     CLI_VRF_NAME_STR,
     "SSM Mapping for IGMPv1/v2 Groups",
     "Static SSM Mapping",
     "Access list number",
     "Access list number (expanded range)",
     "IP Named Standard Access list",
     "Source address to use for static map group")
#else
CLI (no_ip_igmp_ssm_map_static,
     no_ip_igmp_ssm_map_static_cmd,
     "no ip igmp ssm-map static (<1-99>|<1300-1999>|WORD) A.B.C.D",
     CLI_NO_STR,
     CLI_IP_STR,
     CLI_IGMP_STR,
     "SSM Mapping for IGMPv1/v2 Groups",
     "Static SSM Mapping",
     "Access list number",
     "Access list number (expanded range)",
     "IP Named Standard Access list",
     "Source address to use for static map group")
#endif /* HAVE_VRF */
{
  struct igmp_instance *igi;
  struct apn_vrf *ivrf;
  s_int32_t ret;

  IGMP_FN_ENTER ();

  ret = IGMP_ERR_NONE;

  /* Get the VRF Instance */
  if (argc > 2
      && ! pal_strcmp (argv [0], "vrf"))
    ivrf = apn_vrf_lookup_by_name (LIB_GLOB_GET_VR_CONTEXT (cli->zg), argv [1]);

  else
    ivrf = apn_vrf_lookup_by_name (LIB_GLOB_GET_VR_CONTEXT (cli->zg), NULL);

  if (! ivrf)
    {
      ret = IGMP_ERR_NO_CONTEXT_INFO;
      goto EXIT;
    }

  /* Get the IGMP Instance */
  igi = LIB_VRF_GET_IGMP_INSTANCE (ivrf);

  if (! igi)
    {
      ret = IGMP_ERR_NO_CONTEXT_INFO;
      goto EXIT;
    }

  ret = igmp_ssm_map_static_unset (igi, argc > 2 ? argv [2] : argv [0],
                                   argc > 2 ? argv [3] : argv [1]);

EXIT:

  IGMP_FN_EXIT (igmp_cli_return (cli, ret));
}

/*
 * IGMP CLI Interface Configuration Commands
 */

CLI (if_ip_igmp,
     if_ip_igmp_cmd,
     "ip igmp",
     CLI_IP_STR,
     CLI_IGMP_STR)
{
  struct igmp_instance *igi;
  struct interface *ifp;
  s_int32_t ret;

  IGMP_FN_ENTER ();

  ret = IGMP_ERR_NONE;

  ifp = (struct interface *) cli->index;

  if (! ifp)
    {
      ret = IGMP_ERR_NO_SUCH_IFF;
      goto EXIT;
    }

  if (! LIB_IF_GET_LIB_VR (ifp)
      || ! LIB_IF_GET_LIB_VRF (ifp))
    {
      ret = IGMP_ERR_NO_CONTEXT_INFO;
      goto EXIT;
    }

  /* Set Context */
  LIB_VR_SET_VRF_CONTEXT (LIB_IF_GET_LIB_VR (ifp),
                          LIB_IF_GET_LIB_VRF (ifp));

  igi = LIB_VRF_GET_IGMP_INSTANCE (LIB_IF_GET_LIB_VRF (ifp));

  if (! igi)
    {
      ret = IGMP_ERR_NO_CONTEXT_INFO;
      goto EXIT;
    }

  ret = igmp_if_set (igi, ifp);

EXIT:

  IGMP_FN_EXIT (igmp_cli_return (cli, ret));
}

CLI (no_if_ip_igmp,
     no_if_ip_igmp_cmd,
     "no ip igmp",
     CLI_NO_STR,
     CLI_IP_STR,
     CLI_IGMP_STR)
{
  struct igmp_instance *igi;
  struct interface *ifp;
  s_int32_t ret;

  IGMP_FN_ENTER ();

  ret = IGMP_ERR_NONE;

  ifp = (struct interface *) cli->index;

  if (! ifp)
    {
      ret = IGMP_ERR_NO_SUCH_IFF;
      goto EXIT;
    }

  if (! LIB_IF_GET_LIB_VR (ifp)
      || ! LIB_IF_GET_LIB_VRF (ifp))
    {
      ret = IGMP_ERR_NO_CONTEXT_INFO;
      goto EXIT;
    }

  /* Set Context */
  LIB_VR_SET_VRF_CONTEXT (LIB_IF_GET_LIB_VR (ifp),
                          LIB_IF_GET_LIB_VRF (ifp));

  igi = LIB_VRF_GET_IGMP_INSTANCE (LIB_IF_GET_LIB_VRF (ifp));

  if (! igi)
    {
      ret = IGMP_ERR_NO_CONTEXT_INFO;
      goto EXIT;
    }

  ret = igmp_if_unset (igi, ifp);

EXIT:

  IGMP_FN_EXIT (igmp_cli_return (cli, ret));
}

CLI (if_ip_igmp_access_group,
     if_ip_igmp_access_group_cmd,
     "ip igmp access-group (<1-99>|WORD)",
     CLI_IP_STR,
     CLI_IGMP_STR,
     "IGMP group access group",
     "Access list number",
     "IP Named Standard Access list")
{
  struct igmp_instance *igi;
  struct interface *ifp;
  s_int32_t ret;

  IGMP_FN_ENTER ();

  ret = IGMP_ERR_NONE;

  ifp = (struct interface *) cli->index;

  if (! ifp)
    {
      ret = IGMP_ERR_NO_SUCH_IFF;
      goto EXIT;
    }

  if (! LIB_IF_GET_LIB_VR (ifp)
      || ! LIB_IF_GET_LIB_VRF (ifp))
    {
      ret = IGMP_ERR_NO_CONTEXT_INFO;
      goto EXIT;
    }

  /* Set Context */
  LIB_VR_SET_VRF_CONTEXT (LIB_IF_GET_LIB_VR (ifp),
                          LIB_IF_GET_LIB_VRF (ifp));

  igi = LIB_VRF_GET_IGMP_INSTANCE (LIB_IF_GET_LIB_VRF (ifp));

  if (! igi)
    {
      ret = IGMP_ERR_NO_CONTEXT_INFO;
      goto EXIT;
    }

  ret = igmp_if_access_list_set (igi, ifp, argv [0]);

EXIT:

  IGMP_FN_EXIT (igmp_cli_return (cli, ret));
}

CLI (no_if_ip_igmp_access_group,
     no_if_ip_igmp_access_group_cmd,
     "no ip igmp access-group",
     CLI_NO_STR,
     CLI_IP_STR,
     CLI_IGMP_STR,
     "IGMP group access group")
{
  struct igmp_instance *igi;
  struct interface *ifp;
  s_int32_t ret;

  IGMP_FN_ENTER ();

  ret = IGMP_ERR_NONE;

  ifp = (struct interface *) cli->index;

  if (! ifp)
    {
      ret = IGMP_ERR_NO_SUCH_IFF;
      goto EXIT;
    }

  if (! LIB_IF_GET_LIB_VR (ifp)
      || ! LIB_IF_GET_LIB_VRF (ifp))
    {
      ret = IGMP_ERR_NO_CONTEXT_INFO;
      goto EXIT;
    }

  /* Set Context */
  LIB_VR_SET_VRF_CONTEXT (LIB_IF_GET_LIB_VR (ifp),
                          LIB_IF_GET_LIB_VRF (ifp));

  igi = LIB_VRF_GET_IGMP_INSTANCE (LIB_IF_GET_LIB_VRF (ifp));

  if (! igi)
    {
      ret = IGMP_ERR_NO_CONTEXT_INFO;
      goto EXIT;
    }

  ret = igmp_if_access_list_unset (igi, ifp);

EXIT:

  IGMP_FN_EXIT (igmp_cli_return (cli, ret));
}

CLI (if_ip_igmp_immediate_leave,
     if_ip_igmp_immediate_leave_cmd,
     "ip igmp immediate-leave group-list (<1-99>|<1300-1999>|WORD)",
     CLI_IP_STR,
     CLI_IGMP_STR,
     "Leave groups immediately without sending last member query,"
     " use for one host network only",
     "Access list to specify groups",
     "Access list number",
     "Access list number (expanded range)",
     "IP Named Standard Access list")
{
  struct igmp_instance *igi;
  struct interface *ifp;
  s_int32_t ret;

  IGMP_FN_ENTER ();

  ret = IGMP_ERR_NONE;

  ifp = (struct interface *) cli->index;

  if (! ifp)
    {
      ret = IGMP_ERR_NO_SUCH_IFF;
      goto EXIT;
    }

  if (! LIB_IF_GET_LIB_VR (ifp)
      || ! LIB_IF_GET_LIB_VRF (ifp))
    {
      ret = IGMP_ERR_NO_CONTEXT_INFO;
      goto EXIT;
    }

  /* Set Context */
  LIB_VR_SET_VRF_CONTEXT (LIB_IF_GET_LIB_VR (ifp),
                          LIB_IF_GET_LIB_VRF (ifp));

  igi = LIB_VRF_GET_IGMP_INSTANCE (LIB_IF_GET_LIB_VRF (ifp));

  if (! igi)
    {
      ret = IGMP_ERR_NO_CONTEXT_INFO;
      goto EXIT;
    }

  ret = igmp_if_immediate_leave_set (igi, ifp, argv [0]);

EXIT:

  IGMP_FN_EXIT (igmp_cli_return (cli, ret));
}

CLI (no_if_ip_igmp_immediate_leave,
     no_if_ip_igmp_immediate_leave_cmd,
     "no ip igmp immediate-leave",
     CLI_NO_STR,
     CLI_IP_STR,
     CLI_IGMP_STR,
     "Leave groups immediately without sending last member query,"
     " use for one host network only")
{
  struct igmp_instance *igi;
  struct interface *ifp;
  s_int32_t ret;

  IGMP_FN_ENTER ();

  ret = IGMP_ERR_NONE;

  ifp = (struct interface *) cli->index;

  if (! ifp)
    {
      ret = IGMP_ERR_NO_SUCH_IFF;
      goto EXIT;
    }

  if (! LIB_IF_GET_LIB_VR (ifp)
      || ! LIB_IF_GET_LIB_VRF (ifp))
    {
      ret = IGMP_ERR_NO_CONTEXT_INFO;
      goto EXIT;
    }

  /* Set Context */
  LIB_VR_SET_VRF_CONTEXT (LIB_IF_GET_LIB_VR (ifp),
                          LIB_IF_GET_LIB_VRF (ifp));

  igi = LIB_VRF_GET_IGMP_INSTANCE (LIB_IF_GET_LIB_VRF (ifp));

  if (! igi)
    {
      ret = IGMP_ERR_NO_CONTEXT_INFO;
      goto EXIT;
    }

  ret = igmp_if_immediate_leave_unset (igi, ifp);

EXIT:

  IGMP_FN_EXIT (igmp_cli_return (cli, ret));
}

CLI (if_ip_igmp_last_member_query_count,
     if_ip_igmp_last_member_query_count_cmd,
     "ip igmp last-member-query-count"
     IGMP_CLI_RANGE_STR (IGMP_LAST_MEMBER_QUERY_COUNT_MIN,
                         IGMP_LAST_MEMBER_QUERY_COUNT_MAX),
     CLI_IP_STR,
     CLI_IGMP_STR,
     "Last Member Query Count",
     "Last Member Query Count value"
     IGMP_CLI_DEF_VAL_STR (IGMP_LAST_MEMBER_QUERY_COUNT_DEF))
{
  u_int32_t last_member_query_count;
  struct igmp_instance *igi;
  struct interface *ifp;
  s_int32_t ret;

  IGMP_FN_ENTER ();

  ret = IGMP_ERR_NONE;

  CLI_GET_UINT32_RANGE ("Last Member Query Count",
                        last_member_query_count, argv [0],
                        IGMP_LAST_MEMBER_QUERY_COUNT_MIN,
                        IGMP_LAST_MEMBER_QUERY_COUNT_MAX);

  ifp = (struct interface *) cli->index;

  if (! ifp)
    {
      ret = IGMP_ERR_NO_SUCH_IFF;
      goto EXIT;
    }

  if (! LIB_IF_GET_LIB_VR (ifp)
      || ! LIB_IF_GET_LIB_VRF (ifp))
    {
      ret = IGMP_ERR_NO_CONTEXT_INFO;
      goto EXIT;
    }

  /* Set Context */
  LIB_VR_SET_VRF_CONTEXT (LIB_IF_GET_LIB_VR (ifp),
                          LIB_IF_GET_LIB_VRF (ifp));

  igi = LIB_VRF_GET_IGMP_INSTANCE (LIB_IF_GET_LIB_VRF (ifp));

  if (! igi)
    {
      ret = IGMP_ERR_NO_CONTEXT_INFO;
      goto EXIT;
    }

  ret = igmp_if_lmqc_set (igi, ifp, last_member_query_count);

EXIT:

  IGMP_FN_EXIT (igmp_cli_return (cli, ret));
}

CLI (no_if_ip_igmp_last_member_query_count,
     no_if_ip_igmp_last_member_query_count_cmd,
     "no ip igmp last-member-query-count",
     CLI_NO_STR,
     CLI_IP_STR,
     CLI_IGMP_STR,
     "Last Member Query Count")
{
  struct igmp_instance *igi;
  struct interface *ifp;
  s_int32_t ret;

  IGMP_FN_ENTER ();

  ret = IGMP_ERR_NONE;

  ifp = (struct interface *) cli->index;

  if (! ifp)
    {
      ret = IGMP_ERR_NO_SUCH_IFF;
      goto EXIT;
    }

  if (! LIB_IF_GET_LIB_VR (ifp)
      || ! LIB_IF_GET_LIB_VRF (ifp))
    {
      ret = IGMP_ERR_NO_CONTEXT_INFO;
      goto EXIT;
    }

  /* Set Context */
  LIB_VR_SET_VRF_CONTEXT (LIB_IF_GET_LIB_VR (ifp),
                          LIB_IF_GET_LIB_VRF (ifp));

  igi = LIB_VRF_GET_IGMP_INSTANCE (LIB_IF_GET_LIB_VRF (ifp));

  if (! igi)
    {
      ret = IGMP_ERR_NO_CONTEXT_INFO;
      goto EXIT;
    }

  ret = igmp_if_lmqc_unset (igi, ifp);

EXIT:

  IGMP_FN_EXIT (igmp_cli_return (cli, ret));
}

CLI (if_ip_igmp_last_member_query_interval,
     if_ip_igmp_last_member_query_interval_cmd,
     "ip igmp last-member-query-interval"
     IGMP_CLI_RANGE_STR (IGMP_LAST_MEMBER_QUERY_INTERVAL_MIN,
                         IGMP_LAST_MEMBER_QUERY_INTERVAL_MAX),
     CLI_IP_STR,
     CLI_IGMP_STR,
     "Last Member Query Interval",
     "Last Member Query Interval value"
     IGMP_CLI_DEF_VAL_STR (IGMP_LAST_MEMBER_QUERY_INTERVAL_DEF ms))
{
  u_int32_t last_member_query_interval;
  struct igmp_instance *igi;
  struct interface *ifp;
  s_int32_t ret;

  IGMP_FN_ENTER ();

  ret = IGMP_ERR_NONE;

  CLI_GET_UINT32_RANGE ("Last Member Query Interval",
                        last_member_query_interval, argv [0],
                        IGMP_LAST_MEMBER_QUERY_INTERVAL_MIN,
                        IGMP_LAST_MEMBER_QUERY_INTERVAL_MAX);

  ifp = (struct interface *) cli->index;

  if (! ifp)
    {
      ret = IGMP_ERR_NO_SUCH_IFF;
      goto EXIT;
    }

  if (! LIB_IF_GET_LIB_VR (ifp)
      || ! LIB_IF_GET_LIB_VRF (ifp))
    {
      ret = IGMP_ERR_NO_CONTEXT_INFO;
      goto EXIT;
    }

  /* Set Context */
  LIB_VR_SET_VRF_CONTEXT (LIB_IF_GET_LIB_VR (ifp),
                          LIB_IF_GET_LIB_VRF (ifp));

  igi = LIB_VRF_GET_IGMP_INSTANCE (LIB_IF_GET_LIB_VRF (ifp));

  if (! igi)
    {
      ret = IGMP_ERR_NO_CONTEXT_INFO;
      goto EXIT;
    }

  ret = igmp_if_lmqi_set (igi, ifp,
                          last_member_query_interval);

EXIT:

  IGMP_FN_EXIT (igmp_cli_return (cli, ret));
}

CLI (no_if_ip_igmp_last_member_query_interval,
     no_if_ip_igmp_last_member_query_interval_cmd,
     "no ip igmp last-member-query-interval",
     CLI_NO_STR,
     CLI_IP_STR,
     CLI_IGMP_STR,
     "Last Member Query Interval")
{
  struct igmp_instance *igi;
  struct interface *ifp;
  s_int32_t ret;

  IGMP_FN_ENTER ();

  ret = IGMP_ERR_NONE;

  ifp = (struct interface *) cli->index;

  if (! ifp)
    {
      ret = IGMP_ERR_NO_SUCH_IFF;
      goto EXIT;
    }

  if (! LIB_IF_GET_LIB_VR (ifp)
      || ! LIB_IF_GET_LIB_VRF (ifp))
    {
      ret = IGMP_ERR_NO_CONTEXT_INFO;
      goto EXIT;
    }

  /* Set Context */
  LIB_VR_SET_VRF_CONTEXT (LIB_IF_GET_LIB_VR (ifp),
                          LIB_IF_GET_LIB_VRF (ifp));

  igi = LIB_VRF_GET_IGMP_INSTANCE (LIB_IF_GET_LIB_VRF (ifp));

  if (! igi)
    {
      ret = IGMP_ERR_NO_CONTEXT_INFO;
      goto EXIT;
    }

  ret = igmp_if_lmqi_unset (igi, ifp);

EXIT:

  IGMP_FN_EXIT (igmp_cli_return (cli, ret));
}

CLI (if_ip_igmp_limit,
     if_ip_igmp_limit_cmd,
     "ip igmp limit"
     IGMP_CLI_RANGE_STR (IGMP_LIMIT_MIN,
                         IGMP_LIMIT_MAX)
     " (except (<1-99>|<1300-1999>|WORD) |)",
     CLI_IP_STR,
     CLI_IGMP_STR,
     "IGMP Limit",
     "Max Allowed State on this interface"
     IGMP_CLI_DEF_VAL_STR (IGMP_LIMIT_DEF),
     "Exception Access List",
     "Access list number",
     "Access list number (expanded range)",
     "IP Named Standard Access list")
{
  struct igmp_instance *igi;
  struct interface *ifp;
  u_int32_t state_limit;
  s_int32_t ret;

  IGMP_FN_ENTER ();

  ret = IGMP_ERR_NONE;

  CLI_GET_UINT32_RANGE ("Limit",
                        state_limit, argv [0],
                        IGMP_LIMIT_MIN,
                        IGMP_LIMIT_MAX);

  ifp = (struct interface *) cli->index;

  if (! ifp)
    {
      ret = IGMP_ERR_NO_SUCH_IFF;
      goto EXIT;
    }

  if (! LIB_IF_GET_LIB_VR (ifp)
      || ! LIB_IF_GET_LIB_VRF (ifp))
    {
      ret = IGMP_ERR_NO_CONTEXT_INFO;
      goto EXIT;
    }

  /* Set Context */
  LIB_VR_SET_VRF_CONTEXT (LIB_IF_GET_LIB_VR (ifp),
                          LIB_IF_GET_LIB_VRF (ifp));

  igi = LIB_VRF_GET_IGMP_INSTANCE (LIB_IF_GET_LIB_VRF (ifp));

  if (! igi)
    {
      ret = IGMP_ERR_NO_CONTEXT_INFO;
      goto EXIT;
    }

  ret = igmp_if_limit_set (igi, ifp, state_limit, argv [2]);

EXIT:

  IGMP_FN_EXIT (igmp_cli_return (cli, ret));
}

CLI (no_if_ip_igmp_limit,
     no_if_ip_igmp_limit_cmd,
     "no ip igmp limit",
     CLI_NO_STR,
     CLI_IP_STR,
     CLI_IGMP_STR,
     "IGMP Limit")
{
  struct igmp_instance *igi;
  struct interface *ifp;
  s_int32_t ret;

  IGMP_FN_ENTER ();

  ret = IGMP_ERR_NONE;

  ifp = (struct interface *) cli->index;

  if (! ifp)
    {
      ret = IGMP_ERR_NO_SUCH_IFF;
      goto EXIT;
    }

  if (! LIB_IF_GET_LIB_VR (ifp)
      || ! LIB_IF_GET_LIB_VRF (ifp))
    {
      ret = IGMP_ERR_NO_CONTEXT_INFO;
      goto EXIT;
    }

  /* Set Context */
  LIB_VR_SET_VRF_CONTEXT (LIB_IF_GET_LIB_VR (ifp),
                          LIB_IF_GET_LIB_VRF (ifp));

  igi = LIB_VRF_GET_IGMP_INSTANCE (LIB_IF_GET_LIB_VRF (ifp));

  if (! igi)
    {
      ret = IGMP_ERR_NO_CONTEXT_INFO;
      goto EXIT;
    }

  ret = igmp_if_limit_unset (igi, ifp);

EXIT:

  IGMP_FN_EXIT (igmp_cli_return (cli, ret));
}

CLI (if_ip_igmp_mroute_proxy,
     if_ip_igmp_mroute_proxy_cmd,
     "ip igmp mroute-proxy IFNAME",
     CLI_IP_STR,
     CLI_IGMP_STR,
     "Mroute to IGMP proxy",
     CLI_IFNAME_STR)
{
  struct igmp_instance *igi;
  struct interface *ifp;
  s_int32_t ret;

  IGMP_FN_ENTER ();

  ret = IGMP_ERR_NONE;

  ifp = (struct interface *) cli->index;

  if (! ifp)
    {
      ret = IGMP_ERR_NO_SUCH_IFF;
      goto EXIT;
    }

  if (! LIB_IF_GET_LIB_VR (ifp)
      || ! LIB_IF_GET_LIB_VRF (ifp))
    {
      ret = IGMP_ERR_NO_CONTEXT_INFO;
      goto EXIT;
    }

  /* Set Context */
  LIB_VR_SET_VRF_CONTEXT (LIB_IF_GET_LIB_VR (ifp),
                          LIB_IF_GET_LIB_VRF (ifp));

  igi = LIB_VRF_GET_IGMP_INSTANCE (LIB_IF_GET_LIB_VRF (ifp));

  if (! igi)
    {
      ret = IGMP_ERR_NO_CONTEXT_INFO;
      goto EXIT;
    }

  ret = igmp_if_mroute_pxy_set (igi, ifp, argv [0]);

EXIT:

  IGMP_FN_EXIT (igmp_cli_return (cli, ret));
}

CLI (no_if_ip_igmp_mroute_proxy,
     no_if_ip_igmp_mroute_proxy_cmd,
     "no ip igmp mroute-proxy",
     CLI_NO_STR,
     CLI_IP_STR,
     CLI_IGMP_STR,
     "Mroute to IGMP proxy")
{
  struct igmp_instance *igi;
  struct interface *ifp;
  s_int32_t ret;

  IGMP_FN_ENTER ();

  ret = IGMP_ERR_NONE;

  ifp = (struct interface *) cli->index;

  if (! ifp)
    {
      ret = IGMP_ERR_NO_SUCH_IFF;
      goto EXIT;
    }

  if (! LIB_IF_GET_LIB_VR (ifp)
      || ! LIB_IF_GET_LIB_VRF (ifp))
    {
      ret = IGMP_ERR_NO_CONTEXT_INFO;
      goto EXIT;
    }

  /* Set Context */
  LIB_VR_SET_VRF_CONTEXT (LIB_IF_GET_LIB_VR (ifp),
                          LIB_IF_GET_LIB_VRF (ifp));

  igi = LIB_VRF_GET_IGMP_INSTANCE (LIB_IF_GET_LIB_VRF (ifp));

  if (! igi)
    {
      ret = IGMP_ERR_NO_CONTEXT_INFO;
      goto EXIT;
    }

  ret = igmp_if_mroute_pxy_unset (igi, ifp);

EXIT:

  IGMP_FN_EXIT (igmp_cli_return (cli, ret));
}

CLI (if_ip_igmp_proxy_service,
     if_ip_igmp_proxy_service_cmd,
     "ip igmp proxy-service",
     CLI_IP_STR,
     CLI_IGMP_STR,
     "Enable IGMP mroute proxy service")
{
  struct igmp_instance *igi;
  struct interface *ifp;
  s_int32_t ret;

  IGMP_FN_ENTER ();

  ret = IGMP_ERR_NONE;

  ifp = (struct interface *) cli->index;

  if (! ifp)
    {
      ret = IGMP_ERR_NO_SUCH_IFF;
      goto EXIT;
    }

  if (! LIB_IF_GET_LIB_VR (ifp)
      || ! LIB_IF_GET_LIB_VRF (ifp))
    {
      ret = IGMP_ERR_NO_CONTEXT_INFO;
      goto EXIT;
    }

  /* Set Context */
  LIB_VR_SET_VRF_CONTEXT (LIB_IF_GET_LIB_VR (ifp),
                          LIB_IF_GET_LIB_VRF (ifp));

  igi = LIB_VRF_GET_IGMP_INSTANCE (LIB_IF_GET_LIB_VRF (ifp));

  if (! igi)
    {
      ret = IGMP_ERR_NO_CONTEXT_INFO;
      goto EXIT;
    }

  ret = igmp_if_pxy_service_set (igi, ifp);

EXIT:

  IGMP_FN_EXIT (igmp_cli_return (cli, ret));
}

CLI (no_if_ip_igmp_proxy_service,
     no_if_ip_igmp_proxy_service_cmd,
     "no ip igmp proxy-service",
     CLI_NO_STR,
     CLI_IP_STR,
     CLI_IGMP_STR,
     "Enable IGMP mroute proxy service")
{
  struct igmp_instance *igi;
  struct interface *ifp;
  s_int32_t ret;

  IGMP_FN_ENTER ();

  ret = IGMP_ERR_NONE;

  ifp = (struct interface *) cli->index;

  if (! ifp)
    {
      ret = IGMP_ERR_NO_SUCH_IFF;
      goto EXIT;
    }

  if (! LIB_IF_GET_LIB_VR (ifp)
      || ! LIB_IF_GET_LIB_VRF (ifp))
    {
      ret = IGMP_ERR_NO_CONTEXT_INFO;
      goto EXIT;
    }

  /* Set Context */
  LIB_VR_SET_VRF_CONTEXT (LIB_IF_GET_LIB_VR (ifp),
                          LIB_IF_GET_LIB_VRF (ifp));

  igi = LIB_VRF_GET_IGMP_INSTANCE (LIB_IF_GET_LIB_VRF (ifp));

  if (! igi)
    {
      ret = IGMP_ERR_NO_CONTEXT_INFO;
      goto EXIT;
    }

  ret = igmp_if_pxy_service_unset (igi, ifp);

EXIT:

  IGMP_FN_EXIT (igmp_cli_return (cli, ret));
}

CLI (if_ip_igmp_querier_timeout,
     if_ip_igmp_querier_timeout_cmd,
     "ip igmp querier-timeout"
     IGMP_CLI_RANGE_STR (IGMP_QUERIER_TIMEOUT_MIN,
                         IGMP_QUERIER_TIMEOUT_MAX),
     CLI_IP_STR,
     CLI_IGMP_STR,
     "IGMP previous querier timeout",
     "IGMP previous querier timeout value"
     IGMP_CLI_DEF_VAL_STR (IGMP_QUERIER_TIMEOUT_DEF s))
{
  u_int32_t querier_timeout;
  struct igmp_instance *igi;
  struct interface *ifp;
  s_int32_t ret;

  IGMP_FN_ENTER ();

  ret = IGMP_ERR_NONE;

  CLI_GET_UINT32_RANGE ("Querier Timeout",
                        querier_timeout, argv [0],
                        IGMP_QUERIER_TIMEOUT_MIN,
                        IGMP_QUERIER_TIMEOUT_MAX);

  ifp = (struct interface *) cli->index;

  if (! ifp)
    {
      ret = IGMP_ERR_NO_SUCH_IFF;
      goto EXIT;
    }

  if (! LIB_IF_GET_LIB_VR (ifp)
      || ! LIB_IF_GET_LIB_VRF (ifp))
    {
      ret = IGMP_ERR_NO_CONTEXT_INFO;
      goto EXIT;
    }

  /* Set Context */
  LIB_VR_SET_VRF_CONTEXT (LIB_IF_GET_LIB_VR (ifp),
                          LIB_IF_GET_LIB_VRF (ifp));

  igi = LIB_VRF_GET_IGMP_INSTANCE (LIB_IF_GET_LIB_VRF (ifp));

  if (! igi)
    {
      ret = IGMP_ERR_NO_CONTEXT_INFO;
      goto EXIT;
    }

  ret = igmp_if_querier_timeout_set (igi, ifp, querier_timeout);

EXIT:

  IGMP_FN_EXIT (igmp_cli_return (cli, ret));
}

CLI (no_if_ip_igmp_querier_timeout,
     no_if_ip_igmp_querier_timeout_cmd,
     "no ip igmp querier-timeout",
     CLI_NO_STR,
     CLI_IP_STR,
     CLI_IGMP_STR,
     "IGMP previous querier timeout")
{
  struct igmp_instance *igi;
  struct interface *ifp;
  s_int32_t ret;

  IGMP_FN_ENTER ();

  ret = IGMP_ERR_NONE;

  ifp = (struct interface *) cli->index;

  if (! ifp)
    {
      ret = IGMP_ERR_NO_SUCH_IFF;
      goto EXIT;
    }

  if (! LIB_IF_GET_LIB_VR (ifp)
      || ! LIB_IF_GET_LIB_VRF (ifp))
    {
      ret = IGMP_ERR_NO_CONTEXT_INFO;
      goto EXIT;
    }

  /* Set Context */
  LIB_VR_SET_VRF_CONTEXT (LIB_IF_GET_LIB_VR (ifp),
                          LIB_IF_GET_LIB_VRF (ifp));

  igi = LIB_VRF_GET_IGMP_INSTANCE (LIB_IF_GET_LIB_VRF (ifp));

  if (! igi)
    {
      ret = IGMP_ERR_NO_CONTEXT_INFO;
      goto EXIT;
    }

  ret = igmp_if_querier_timeout_unset (igi, ifp);

EXIT:

  IGMP_FN_EXIT (igmp_cli_return (cli, ret));
}

CLI (if_ip_igmp_query_interval,
     if_ip_igmp_query_interval_cmd,
     "ip igmp query-interval"
     IGMP_CLI_RANGE_STR (IGMP_QUERY_INTERVAL_MIN,
                         IGMP_QUERY_INTERVAL_MAX),
     CLI_IP_STR,
     CLI_IGMP_STR,
     "Query Interval",
     "Query Interval value"
     IGMP_CLI_DEF_VAL_STR (IGMP_QUERY_INTERVAL_DEF s))
{
  struct igmp_instance *igi;
  u_int32_t query_interval;
  struct interface *ifp;
  s_int32_t ret;

  IGMP_FN_ENTER ();

  ret = IGMP_ERR_NONE;

  CLI_GET_UINT32_RANGE ("Query Interval",
                        query_interval, argv [0],
                        IGMP_QUERY_INTERVAL_MIN,
                        IGMP_QUERY_INTERVAL_MAX);

  ifp = (struct interface *) cli->index;

  if (! ifp)
    {
      ret = IGMP_ERR_NO_SUCH_IFF;
      goto EXIT;
    }

  if (! LIB_IF_GET_LIB_VR (ifp)
      || ! LIB_IF_GET_LIB_VRF (ifp))
    {
      ret = IGMP_ERR_NO_CONTEXT_INFO;
      goto EXIT;
    }

  /* Set Context */
  LIB_VR_SET_VRF_CONTEXT (LIB_IF_GET_LIB_VR (ifp),
                          LIB_IF_GET_LIB_VRF (ifp));

  igi = LIB_VRF_GET_IGMP_INSTANCE (LIB_IF_GET_LIB_VRF (ifp));

  if (! igi)
    {
      ret = IGMP_ERR_NO_CONTEXT_INFO;
      goto EXIT;
    }

  ret = igmp_if_query_interval_set (igi, ifp, query_interval);

EXIT:

  IGMP_FN_EXIT (igmp_cli_return (cli, ret));
}

CLI (no_if_ip_igmp_query_interval,
     no_if_ip_igmp_query_interval_cmd,
     "no ip igmp query-interval",
     CLI_NO_STR,
     CLI_IP_STR,
     CLI_IGMP_STR,
     "Query Interval")
{
  struct igmp_instance *igi;
  struct interface *ifp;
  s_int32_t ret;

  IGMP_FN_ENTER ();

  ret = IGMP_ERR_NONE;

  ifp = (struct interface *) cli->index;

  if (! ifp)
    {
      ret = IGMP_ERR_NO_SUCH_IFF;
      goto EXIT;
    }

  if (! LIB_IF_GET_LIB_VR (ifp)
      || ! LIB_IF_GET_LIB_VRF (ifp))
    {
      ret = IGMP_ERR_NO_CONTEXT_INFO;
      goto EXIT;
    }

  /* Set Context */
  LIB_VR_SET_VRF_CONTEXT (LIB_IF_GET_LIB_VR (ifp),
                          LIB_IF_GET_LIB_VRF (ifp));

  igi = LIB_VRF_GET_IGMP_INSTANCE (LIB_IF_GET_LIB_VRF (ifp));

  if (! igi)
    {
      ret = IGMP_ERR_NO_CONTEXT_INFO;
      goto EXIT;
    }

  ret = igmp_if_query_interval_unset (igi, ifp);

EXIT:

  IGMP_FN_EXIT (igmp_cli_return (cli, ret));
}

CLI (if_ip_igmp_startup_query_count,
     if_ip_igmp_startup_query_count_cmd,
     "ip igmp startup-query-count"
     IGMP_CLI_RANGE_STR (IGMP_STARTUP_QUERY_COUNT_MIN,
                         IGMP_STARTUP_QUERY_COUNT_MAX),
     CLI_IP_STR,
     CLI_IGMP_STR,
     "Startup Query count",
     "Startup Query count value"
     IGMP_CLI_DEF_VAL_STR (IGMP_STARTUP_QUERY_COUNT_DEF))
{
  struct igmp_instance *igi;
  u_int32_t query_count;
  struct interface *ifp;
  s_int32_t ret;

  IGMP_FN_ENTER ();

  ret = IGMP_ERR_NONE;

  CLI_GET_UINT32_RANGE ("Startup Query count",
                        query_count, argv [0],
                        IGMP_STARTUP_QUERY_COUNT_MIN,
                        IGMP_STARTUP_QUERY_COUNT_MAX);

  ifp = (struct interface *) cli->index;

  if (! ifp)
    {
      ret = IGMP_ERR_NO_SUCH_IFF;
      goto EXIT;
    }

  if (! LIB_IF_GET_LIB_VR (ifp)
      || ! LIB_IF_GET_LIB_VRF (ifp))
    {
      ret = IGMP_ERR_NO_CONTEXT_INFO;
      goto EXIT;
    }

  /* Set Context */
  LIB_VR_SET_VRF_CONTEXT (LIB_IF_GET_LIB_VR (ifp),
                          LIB_IF_GET_LIB_VRF (ifp));

  igi = LIB_VRF_GET_IGMP_INSTANCE (LIB_IF_GET_LIB_VRF (ifp));

  if (! igi)
    {
      ret = IGMP_ERR_NO_CONTEXT_INFO;
      goto EXIT;
    }

  ret = igmp_if_startup_query_count_set (igi, ifp, query_count);

EXIT:

  IGMP_FN_EXIT (igmp_cli_return (cli, ret));
}

CLI (no_if_ip_igmp_startup_query_count,
     no_if_ip_igmp_startup_query_count_cmd,
     "no ip igmp startup-query-count",
     CLI_NO_STR,
     CLI_IP_STR,
     CLI_IGMP_STR,
     "Startup Query Count")
{
  struct igmp_instance *igi;
  struct interface *ifp;
  s_int32_t ret;

  IGMP_FN_ENTER ();

  ret = IGMP_ERR_NONE;

  ifp = (struct interface *) cli->index;

  if (! ifp)
    {
      ret = IGMP_ERR_NO_SUCH_IFF;
      goto EXIT;
    }

  if (! LIB_IF_GET_LIB_VR (ifp)
      || ! LIB_IF_GET_LIB_VRF (ifp))
    {
      ret = IGMP_ERR_NO_CONTEXT_INFO;
      goto EXIT;
    }

  /* Set Context */
  LIB_VR_SET_VRF_CONTEXT (LIB_IF_GET_LIB_VR (ifp),
                          LIB_IF_GET_LIB_VRF (ifp));

  igi = LIB_VRF_GET_IGMP_INSTANCE (LIB_IF_GET_LIB_VRF (ifp));

  if (! igi)
    {
      ret = IGMP_ERR_NO_CONTEXT_INFO;
      goto EXIT;
    }

  ret = igmp_if_startup_query_count_unset (igi, ifp);

EXIT:

  IGMP_FN_EXIT (igmp_cli_return (cli, ret));
}

CLI (if_ip_igmp_startup_query_interval,
     if_ip_igmp_startup_query_interval_cmd,
     "ip igmp startup-query-interval"
     IGMP_CLI_RANGE_STR (IGMP_QUERY_INTERVAL_MIN,
                         IGMP_QUERY_INTERVAL_MAX),
     CLI_IP_STR,
     CLI_IGMP_STR,
     "Startup Query Interval",
     "Startup Query Interval value"
     IGMP_CLI_DEF_VAL_STR (IGMP_STARTUP_QUERY_INTERVAL_DEF s))
{
  struct igmp_instance *igi;
  u_int32_t query_interval;
  struct interface *ifp;
  s_int32_t ret;

  IGMP_FN_ENTER ();

  ret = IGMP_ERR_NONE;

  CLI_GET_UINT32_RANGE ("Startup Query Interval",
                        query_interval, argv [0],
                        IGMP_QUERY_INTERVAL_MIN,
                        IGMP_QUERY_INTERVAL_MAX);

  ifp = (struct interface *) cli->index;

  if (! ifp)
    {
      ret = IGMP_ERR_NO_SUCH_IFF;
      goto EXIT;
    }

  if (! LIB_IF_GET_LIB_VR (ifp)
      || ! LIB_IF_GET_LIB_VRF (ifp))
    {
      ret = IGMP_ERR_NO_CONTEXT_INFO;
      goto EXIT;
    }

  /* Set Context */
  LIB_VR_SET_VRF_CONTEXT (LIB_IF_GET_LIB_VR (ifp),
                          LIB_IF_GET_LIB_VRF (ifp));

  igi = LIB_VRF_GET_IGMP_INSTANCE (LIB_IF_GET_LIB_VRF (ifp));

  if (! igi)
    {
      ret = IGMP_ERR_NO_CONTEXT_INFO;
      goto EXIT;
    }

  ret = igmp_if_startup_query_interval_set (igi, ifp, query_interval);

EXIT:

  IGMP_FN_EXIT (igmp_cli_return (cli, ret));
}

CLI (no_if_ip_igmp_startup_query_interval,
     no_if_ip_igmp_startup_query_interval_cmd,
     "no ip igmp startup-query-interval",
     CLI_NO_STR,
     CLI_IP_STR,
     CLI_IGMP_STR,
     "Startup Query Interval")
{
  struct igmp_instance *igi;
  struct interface *ifp;
  s_int32_t ret;

  IGMP_FN_ENTER ();

  ret = IGMP_ERR_NONE;

  ifp = (struct interface *) cli->index;

  if (! ifp)
    {
      ret = IGMP_ERR_NO_SUCH_IFF;
      goto EXIT;
    }

  if (! LIB_IF_GET_LIB_VR (ifp)
      || ! LIB_IF_GET_LIB_VRF (ifp))
    {
      ret = IGMP_ERR_NO_CONTEXT_INFO;
      goto EXIT;
    }

  /* Set Context */
  LIB_VR_SET_VRF_CONTEXT (LIB_IF_GET_LIB_VR (ifp),
                          LIB_IF_GET_LIB_VRF (ifp));

  igi = LIB_VRF_GET_IGMP_INSTANCE (LIB_IF_GET_LIB_VRF (ifp));

  if (! igi)
    {
      ret = IGMP_ERR_NO_CONTEXT_INFO;
      goto EXIT;
    }

  ret = igmp_if_startup_query_interval_unset (igi, ifp);

EXIT:

  IGMP_FN_EXIT (igmp_cli_return (cli, ret));
}

CLI (if_ip_igmp_query_max_response_time,
     if_ip_igmp_query_max_response_time_cmd,
     "ip igmp query-max-response-time"
     IGMP_CLI_RANGE_STR (IGMP_QUERY_RESPONSE_INTERVAL_MIN,
                         IGMP_QUERY_RESPONSE_INTERVAL_MAX),
     CLI_IP_STR,
     CLI_IGMP_STR,
     "IGMP Max Query Response Time",
     "Query Reponse Time"
     IGMP_CLI_DEF_VAL_STR (IGMP_QUERY_RESPONSE_INTERVAL_DEF s))
{
  u_int32_t response_interval;
  struct igmp_instance *igi;
  struct interface *ifp;
  s_int32_t ret;

  IGMP_FN_ENTER ();

  ret = IGMP_ERR_NONE;

  CLI_GET_UINT32_RANGE ("Query Max-Response Time",
                        response_interval, argv [0],
                        IGMP_QUERY_RESPONSE_INTERVAL_MIN,
                        IGMP_QUERY_RESPONSE_INTERVAL_MAX);

  ifp = (struct interface *) cli->index;

  if (! ifp)
    {
      ret = IGMP_ERR_NO_SUCH_IFF;
      goto EXIT;
    }

  if (! LIB_IF_GET_LIB_VR (ifp)
      || ! LIB_IF_GET_LIB_VRF (ifp))
    {
      ret = IGMP_ERR_NO_CONTEXT_INFO;
      goto EXIT;
    }

  /* Set Context */
  LIB_VR_SET_VRF_CONTEXT (LIB_IF_GET_LIB_VR (ifp),
                          LIB_IF_GET_LIB_VRF (ifp));

  igi = LIB_VRF_GET_IGMP_INSTANCE (LIB_IF_GET_LIB_VRF (ifp));

  if (! igi)
    {
      ret = IGMP_ERR_NO_CONTEXT_INFO;
      goto EXIT;
    }

  ret = igmp_if_query_response_interval_set (igi, ifp,
                                             response_interval);

EXIT:

  IGMP_FN_EXIT (igmp_cli_return (cli, ret));
}

CLI (no_if_ip_igmp_query_max_response_time,
     no_if_ip_igmp_query_max_response_time_cmd,
     "no ip igmp query-max-response-time",
     CLI_NO_STR,
     CLI_IP_STR,
     CLI_IGMP_STR,
     "IGMP Max Query Response Time")
{
  struct igmp_instance *igi;
  struct interface *ifp;
  s_int32_t ret;

  IGMP_FN_ENTER ();

  ret = IGMP_ERR_NONE;

  ifp = (struct interface *) cli->index;

  if (! ifp)
    {
      ret = IGMP_ERR_NO_SUCH_IFF;
      goto EXIT;
    }

  if (! LIB_IF_GET_LIB_VR (ifp)
      || ! LIB_IF_GET_LIB_VRF (ifp))
    {
      ret = IGMP_ERR_NO_CONTEXT_INFO;
      goto EXIT;
    }

  /* Set Context */
  LIB_VR_SET_VRF_CONTEXT (LIB_IF_GET_LIB_VR (ifp),
                          LIB_IF_GET_LIB_VRF (ifp));

  igi = LIB_VRF_GET_IGMP_INSTANCE (LIB_IF_GET_LIB_VRF (ifp));

  if (! igi)
    {
      ret = IGMP_ERR_NO_CONTEXT_INFO;
      goto EXIT;
    }

  ret = igmp_if_query_response_interval_unset (igi, ifp);

EXIT:

  IGMP_FN_EXIT (igmp_cli_return (cli, ret));
}

CLI (if_ip_igmp_robustness_variable,
     if_ip_igmp_robustness_variable_cmd,
     "ip igmp robustness-variable"
     IGMP_CLI_RANGE_STR (IGMP_ROBUSTNESS_VAR_MIN,
                         IGMP_ROBUSTNESS_VAR_MAX),
     CLI_IP_STR,
     CLI_IGMP_STR,
     "Robustness Variable",
     "Robustness Variable value"
     IGMP_CLI_DEF_VAL_STR (IGMP_ROBUSTNESS_VAR_DEF))
{
  struct igmp_instance *igi;
  u_int32_t robustness_var;
  struct interface *ifp;
  s_int32_t ret;

  IGMP_FN_ENTER ();

  ret = IGMP_ERR_NONE;

  CLI_GET_UINT32_RANGE ("Robustness Variable",
                        robustness_var, argv [0],
                        IGMP_ROBUSTNESS_VAR_MIN,
                        IGMP_ROBUSTNESS_VAR_MAX);

  ifp = (struct interface *) cli->index;

  if (! ifp)
    {
      ret = IGMP_ERR_NO_SUCH_IFF;
      goto EXIT;
    }

  if (! LIB_IF_GET_LIB_VR (ifp)
      || ! LIB_IF_GET_LIB_VRF (ifp))
    {
      ret = IGMP_ERR_NO_CONTEXT_INFO;
      goto EXIT;
    }

  /* Set Context */
  LIB_VR_SET_VRF_CONTEXT (LIB_IF_GET_LIB_VR (ifp),
                          LIB_IF_GET_LIB_VRF (ifp));

  igi = LIB_VRF_GET_IGMP_INSTANCE (LIB_IF_GET_LIB_VRF (ifp));

  if (! igi)
    {
      ret = IGMP_ERR_NO_CONTEXT_INFO;
      goto EXIT;
    }

  ret = igmp_if_robustness_var_set (igi, ifp, robustness_var);

EXIT:

  IGMP_FN_EXIT (igmp_cli_return (cli, ret));
}

CLI (no_if_ip_igmp_robustness_variable,
     no_if_ip_igmp_robustness_variable_cmd,
     "no ip igmp robustness-variable",
     CLI_NO_STR,
     CLI_IP_STR,
     CLI_IGMP_STR,
     "Robustness Variable")
{
  struct igmp_instance *igi;
  struct interface *ifp;
  s_int32_t ret;

  IGMP_FN_ENTER ();

  ret = IGMP_ERR_NONE;

  ifp = (struct interface *) cli->index;

  if (! ifp)
    {
      ret = IGMP_ERR_NO_SUCH_IFF;
      goto EXIT;
    }

  if (! LIB_IF_GET_LIB_VR (ifp)
      || ! LIB_IF_GET_LIB_VRF (ifp))
    {
      ret = IGMP_ERR_NO_CONTEXT_INFO;
      goto EXIT;
    }

  /* Set Context */
  LIB_VR_SET_VRF_CONTEXT (LIB_IF_GET_LIB_VR (ifp),
                          LIB_IF_GET_LIB_VRF (ifp));

  igi = LIB_VRF_GET_IGMP_INSTANCE (LIB_IF_GET_LIB_VRF (ifp));

  if (! igi)
    {
      ret = IGMP_ERR_NO_CONTEXT_INFO;
      goto EXIT;
    }

  ret = igmp_if_robustness_var_unset (igi, ifp);

EXIT:

  IGMP_FN_EXIT (igmp_cli_return (cli, ret));
}

CLI (if_ip_igmp_ra_option,
     if_ip_igmp_ra_option_cmd,
     "ip igmp ra-option",
     CLI_IP_STR,
     CLI_IGMP_STR,
     "IGMP Strict RA Option Validation")
{
  struct igmp_instance *igi;
  struct interface *ifp;
  s_int32_t ret;

  IGMP_FN_ENTER ();

  ret = IGMP_ERR_NONE;

  ifp = (struct interface *) cli->index;

  if (! ifp)
    {
      ret = IGMP_ERR_NO_SUCH_IFF;
      goto EXIT;
    }

  if (! LIB_IF_GET_LIB_VR (ifp)
      || ! LIB_IF_GET_LIB_VRF (ifp))
    {
      ret = IGMP_ERR_NO_CONTEXT_INFO;
      goto EXIT;
    }

  /* Set Context */
  LIB_VR_SET_VRF_CONTEXT (LIB_IF_GET_LIB_VR (ifp),
                          LIB_IF_GET_LIB_VRF (ifp));
  igi = LIB_VRF_GET_IGMP_INSTANCE (LIB_IF_GET_LIB_VRF (ifp));

  if (! igi)
    {
      ret = IGMP_ERR_NO_CONTEXT_INFO;
      goto EXIT;
    }

  ret = igmp_if_ra_set (igi, ifp);

  EXIT:

  IGMP_FN_EXIT (igmp_cli_return (cli, ret));
}

CLI (no_if_ip_igmp_ra_option,
     no_if_ip_igmp_ra_option_cmd,
     "no ip igmp ra-option",
     CLI_NO_STR,
     CLI_IP_STR,
     CLI_IGMP_STR,
     "IGMP Strict RA Option Validation")
{
  struct igmp_instance *igi;
  struct interface *ifp;
  s_int32_t ret;

  IGMP_FN_ENTER ();

  ret = IGMP_ERR_NONE;

  ifp = (struct interface *) cli->index;

  if (! ifp)
    {
      ret = IGMP_ERR_NO_SUCH_IFF;
      goto EXIT;
    }

  if (! LIB_IF_GET_LIB_VR (ifp)
      || ! LIB_IF_GET_LIB_VRF (ifp))
    {
      ret = IGMP_ERR_NO_CONTEXT_INFO;
      goto EXIT;
    }

  /* Set Context */
  LIB_VR_SET_VRF_CONTEXT (LIB_IF_GET_LIB_VR (ifp),
                          LIB_IF_GET_LIB_VRF (ifp));

  igi = LIB_VRF_GET_IGMP_INSTANCE (LIB_IF_GET_LIB_VRF (ifp));

  if (! igi)
    {
      ret = IGMP_ERR_NO_CONTEXT_INFO;
      goto EXIT;
    }

  ret = igmp_if_ra_unset (igi, ifp);

EXIT:

  IGMP_FN_EXIT (igmp_cli_return (cli, ret));
}

CLI (if_ip_igmp_snooping,
     if_ip_igmp_snooping_cmd,
     "ip igmp snooping",
     CLI_IP_STR,
     CLI_IGMP_STR,
     CLI_IGMP_SNOOP_STR)
{
  struct igmp_instance *igi;
  struct interface *ifp;
  s_int32_t ret;

  IGMP_FN_ENTER ();

  ret = IGMP_ERR_NONE;

  ifp = (struct interface *) cli->index;

  if (! ifp)
    {
      ret = IGMP_ERR_NO_SUCH_IFF;
      goto EXIT;
    }

  if (! LIB_IF_GET_LIB_VR (ifp)
      || ! LIB_IF_GET_LIB_VRF (ifp))
    {
      ret = IGMP_ERR_NO_CONTEXT_INFO;
      goto EXIT;
    }

  /* Set Context */
  LIB_VR_SET_VRF_CONTEXT (LIB_IF_GET_LIB_VR (ifp),
                          LIB_IF_GET_LIB_VRF (ifp));

  igi = LIB_VRF_GET_IGMP_INSTANCE (LIB_IF_GET_LIB_VRF (ifp));

  if (! igi)
    {
      ret = IGMP_ERR_NO_CONTEXT_INFO;
      goto EXIT;
    }

  ret = igmp_if_snooping_set (igi, ifp);

EXIT:

  IGMP_FN_EXIT (igmp_cli_return (cli, ret));
}

CLI (no_if_ip_igmp_snooping,
     no_if_ip_igmp_snooping_cmd,
     "no ip igmp snooping",
     CLI_NO_STR,
     CLI_IP_STR,
     CLI_IGMP_STR,
     CLI_IGMP_SNOOP_STR)
{
  struct igmp_instance *igi;
  struct interface *ifp;
  s_int32_t ret;

  IGMP_FN_ENTER ();

  ret = IGMP_ERR_NONE;

  ifp = (struct interface *) cli->index;

  if (! ifp)
    {
      ret = IGMP_ERR_NO_SUCH_IFF;
      goto EXIT;
    }

  if (! LIB_IF_GET_LIB_VR (ifp)
      || ! LIB_IF_GET_LIB_VRF (ifp))
    {
      ret = IGMP_ERR_NO_CONTEXT_INFO;
      goto EXIT;
    }

  /* Set Context */
  LIB_VR_SET_VRF_CONTEXT (LIB_IF_GET_LIB_VR (ifp),
                          LIB_IF_GET_LIB_VRF (ifp));

  igi = LIB_VRF_GET_IGMP_INSTANCE (LIB_IF_GET_LIB_VRF (ifp));

  if (! igi)
    {
      ret = IGMP_ERR_NO_CONTEXT_INFO;
      goto EXIT;
    }

  ret = igmp_if_snooping_unset (igi, ifp);

EXIT:

  IGMP_FN_EXIT (igmp_cli_return (cli, ret));
}

CLI (if_ip_igmp_snooping_fast_leave,
     if_ip_igmp_snooping_fast_leave_cmd,
     "ip igmp snooping fast-leave",
     CLI_IP_STR,
     CLI_IGMP_STR,
     CLI_IGMP_SNOOP_STR,
     "Fast Leave Processing")
{
  struct igmp_instance *igi;
  struct interface *ifp;
  s_int32_t ret;

  IGMP_FN_ENTER ();

  ret = IGMP_ERR_NONE;

  ifp = (struct interface *) cli->index;

  if (! ifp)
    {
      ret = IGMP_ERR_NO_SUCH_IFF;
      goto EXIT;
    }

  if (! LIB_IF_GET_LIB_VR (ifp)
      || ! LIB_IF_GET_LIB_VRF (ifp))
    {
      ret = IGMP_ERR_NO_CONTEXT_INFO;
      goto EXIT;
    }

  /* Set Context */
  LIB_VR_SET_VRF_CONTEXT (LIB_IF_GET_LIB_VR (ifp),
                          LIB_IF_GET_LIB_VRF (ifp));

  igi = LIB_VRF_GET_IGMP_INSTANCE (LIB_IF_GET_LIB_VRF (ifp));

  if (! igi)
    {
      ret = IGMP_ERR_NO_CONTEXT_INFO;
      goto EXIT;
    }

  ret = igmp_if_snoop_fast_leave_set (igi, ifp);

EXIT:

  IGMP_FN_EXIT (igmp_cli_return (cli, ret));
}

CLI (no_if_ip_igmp_snooping_fast_leave,
     no_if_ip_igmp_snooping_fast_leave_cmd,
     "no ip igmp snooping fast-leave",
     CLI_NO_STR,
     CLI_IP_STR,
     CLI_IGMP_STR,
     CLI_IGMP_SNOOP_STR,
     "Fast Leave Processing")
{
  struct igmp_instance *igi;
  struct interface *ifp;
  s_int32_t ret;

  IGMP_FN_ENTER ();

  ret = IGMP_ERR_NONE;

  ifp = (struct interface *) cli->index;

  if (! ifp)
    {
      ret = IGMP_ERR_NO_SUCH_IFF;
      goto EXIT;
    }

  if (! LIB_IF_GET_LIB_VR (ifp)
      || ! LIB_IF_GET_LIB_VRF (ifp))
    {
      ret = IGMP_ERR_NO_CONTEXT_INFO;
      goto EXIT;
    }

  /* Set Context */
  LIB_VR_SET_VRF_CONTEXT (LIB_IF_GET_LIB_VR (ifp),
                          LIB_IF_GET_LIB_VRF (ifp));

  igi = LIB_VRF_GET_IGMP_INSTANCE (LIB_IF_GET_LIB_VRF (ifp));

  if (! igi)
    {
      ret = IGMP_ERR_NO_CONTEXT_INFO;
      goto EXIT;
    }

  ret = igmp_if_snoop_fast_leave_unset (igi, ifp);

EXIT:

  IGMP_FN_EXIT (igmp_cli_return (cli, ret));
}

CLI (if_ip_igmp_snooping_mrouter_if,
     if_ip_igmp_snooping_mrouter_if_cmd,
     "ip igmp snooping mrouter interface IFNAME",
     CLI_IP_STR,
     CLI_IGMP_STR,
     CLI_IGMP_SNOOP_STR,
     "Multicast Router",
     "Interface to use",
     CLI_IFNAME_STR)
{
  struct igmp_instance *igi;
  struct interface *ifp;
  s_int32_t ret;

  IGMP_FN_ENTER ();

  ret = IGMP_ERR_NONE;

  ifp = (struct interface *) cli->index;

  if (! ifp)
    {
      ret = IGMP_ERR_NO_SUCH_IFF;
      goto EXIT;
    }

  if (! LIB_IF_GET_LIB_VR (ifp)
      || ! LIB_IF_GET_LIB_VRF (ifp))
    {
      ret = IGMP_ERR_NO_CONTEXT_INFO;
      goto EXIT;
    }

  /* Set Context */
  LIB_VR_SET_VRF_CONTEXT (LIB_IF_GET_LIB_VR (ifp),
                          LIB_IF_GET_LIB_VRF (ifp));

  igi = LIB_VRF_GET_IGMP_INSTANCE (LIB_IF_GET_LIB_VRF (ifp));

  if (! igi)
    {
      ret = IGMP_ERR_NO_CONTEXT_INFO;
      goto EXIT;
    }

  ret = igmp_if_snoop_mrouter_if_set (igi, ifp, argv [0]);

EXIT:

  IGMP_FN_EXIT (igmp_cli_return (cli, ret));
}

CLI (no_if_ip_igmp_snooping_mrouter_if,
     no_if_ip_igmp_snooping_mrouter_if_cmd,
     "no ip igmp snooping mrouter interface IFNAME",
     CLI_NO_STR,
     CLI_IP_STR,
     CLI_IGMP_STR,
     CLI_IGMP_SNOOP_STR,
     "Multicast Router",
     "Interface to use",
     CLI_IFNAME_STR)
{
  struct igmp_instance *igi;
  struct interface *ifp;
  s_int32_t ret;

  IGMP_FN_ENTER ();

  ret = IGMP_ERR_NONE;

  ifp = (struct interface *) cli->index;

  if (! ifp)
    {
      ret = IGMP_ERR_NO_SUCH_IFF;
      goto EXIT;
    }

  if (! LIB_IF_GET_LIB_VR (ifp)
      || ! LIB_IF_GET_LIB_VRF (ifp))
    {
      ret = IGMP_ERR_NO_CONTEXT_INFO;
      goto EXIT;
    }

  /* Set Context */
  LIB_VR_SET_VRF_CONTEXT (LIB_IF_GET_LIB_VR (ifp),
                          LIB_IF_GET_LIB_VRF (ifp));

  igi = LIB_VRF_GET_IGMP_INSTANCE (LIB_IF_GET_LIB_VRF (ifp));

  if (! igi)
    {
      ret = IGMP_ERR_NO_CONTEXT_INFO;
      goto EXIT;
    }

  ret = igmp_if_snoop_mrouter_if_unset (igi, ifp, argv [0]);

EXIT:

  IGMP_FN_EXIT (igmp_cli_return (cli, ret));
}

CLI (if_ip_igmp_snooping_querier,
     if_ip_igmp_snooping_querier_cmd,
     "ip igmp snooping querier",
     CLI_IP_STR,
     CLI_IGMP_STR,
     CLI_IGMP_SNOOP_STR,
     "Querier")
{
  struct igmp_instance *igi;
  struct interface *ifp;
  s_int32_t ret;

  IGMP_FN_ENTER ();

  ret = IGMP_ERR_NONE;

  ifp = (struct interface *) cli->index;

  if (! ifp)
    {
      ret = IGMP_ERR_NO_SUCH_IFF;
      goto EXIT;
    }

  if (! LIB_IF_GET_LIB_VR (ifp)
      || ! LIB_IF_GET_LIB_VRF (ifp))
    {
      ret = IGMP_ERR_NO_CONTEXT_INFO;
      goto EXIT;
    }

  /* Set Context */
  LIB_VR_SET_VRF_CONTEXT (LIB_IF_GET_LIB_VR (ifp),
                          LIB_IF_GET_LIB_VRF (ifp));

  igi = LIB_VRF_GET_IGMP_INSTANCE (LIB_IF_GET_LIB_VRF (ifp));

  if (! igi)
    {
      ret = IGMP_ERR_NO_CONTEXT_INFO;
      goto EXIT;
    }

  ret = igmp_if_snoop_querier_set (igi, ifp);

EXIT:

  IGMP_FN_EXIT (igmp_cli_return (cli, ret));
}

CLI (no_if_ip_igmp_snooping_querier,
     no_if_ip_igmp_snooping_querier_cmd,
     "no ip igmp snooping querier",
     CLI_NO_STR,
     CLI_IP_STR,
     CLI_IGMP_STR,
     CLI_IGMP_SNOOP_STR,
     "Querier")
{
  struct igmp_instance *igi;
  struct interface *ifp;
  s_int32_t ret;

  IGMP_FN_ENTER ();

  ret = IGMP_ERR_NONE;

  ifp = (struct interface *) cli->index;

  if (! ifp)
    {
      ret = IGMP_ERR_NO_SUCH_IFF;
      goto EXIT;
    }

  if (! LIB_IF_GET_LIB_VR (ifp)
      || ! LIB_IF_GET_LIB_VRF (ifp))
    {
      ret = IGMP_ERR_NO_CONTEXT_INFO;
      goto EXIT;
    }

  /* Set Context */
  LIB_VR_SET_VRF_CONTEXT (LIB_IF_GET_LIB_VR (ifp),
                          LIB_IF_GET_LIB_VRF (ifp));

  igi = LIB_VRF_GET_IGMP_INSTANCE (LIB_IF_GET_LIB_VRF (ifp));

  if (! igi)
    {
      ret = IGMP_ERR_NO_CONTEXT_INFO;
      goto EXIT;
    }

  ret = igmp_if_snoop_querier_unset (igi, ifp);

EXIT:

  IGMP_FN_EXIT (igmp_cli_return (cli, ret));
}

CLI (if_ip_igmp_snooping_report_suppress,
     if_ip_igmp_snooping_report_suppress_cmd,
     "ip igmp snooping report-suppression",
     CLI_IP_STR,
     CLI_IGMP_STR,
     CLI_IGMP_SNOOP_STR,
     "IGMPv1/V2 Report Suppression")
{
  struct igmp_instance *igi;
  struct interface *ifp;
  s_int32_t ret;

  IGMP_FN_ENTER ();

  ret = IGMP_ERR_NONE;

  ifp = (struct interface *) cli->index;

  if (! ifp)
    {
      ret = IGMP_ERR_NO_SUCH_IFF;
      goto EXIT;
    }

  if (! LIB_IF_GET_LIB_VR (ifp)
      || ! LIB_IF_GET_LIB_VRF (ifp))
    {
      ret = IGMP_ERR_NO_CONTEXT_INFO;
      goto EXIT;
    }

  /* Set Context */
  LIB_VR_SET_VRF_CONTEXT (LIB_IF_GET_LIB_VR (ifp),
                          LIB_IF_GET_LIB_VRF (ifp));

  igi = LIB_VRF_GET_IGMP_INSTANCE (LIB_IF_GET_LIB_VRF (ifp));

  if (! igi)
    {
      ret = IGMP_ERR_NO_CONTEXT_INFO;
      goto EXIT;
    }

  ret = igmp_if_snoop_report_suppress_set (igi, ifp);

EXIT:

  IGMP_FN_EXIT (igmp_cli_return (cli, ret));
}

CLI (no_if_ip_igmp_snooping_report_suppress,
     no_if_ip_igmp_snooping_report_suppress_cmd,
     "no ip igmp snooping report-suppression",
     CLI_NO_STR,
     CLI_IP_STR,
     CLI_IGMP_STR,
     CLI_IGMP_SNOOP_STR,
     "IGMPv1/V2 Report Suppression")
{
  struct igmp_instance *igi;
  struct interface *ifp;
  s_int32_t ret;

  IGMP_FN_ENTER ();

  ret = IGMP_ERR_NONE;

  ifp = (struct interface *) cli->index;

  if (! ifp)
    {
      ret = IGMP_ERR_NO_SUCH_IFF;
      goto EXIT;
    }

  if (! LIB_IF_GET_LIB_VR (ifp)
      || ! LIB_IF_GET_LIB_VRF (ifp))
    {
      ret = IGMP_ERR_NO_CONTEXT_INFO;
      goto EXIT;
    }

  /* Set Context */
  LIB_VR_SET_VRF_CONTEXT (LIB_IF_GET_LIB_VR (ifp),
                          LIB_IF_GET_LIB_VRF (ifp));

  igi = LIB_VRF_GET_IGMP_INSTANCE (LIB_IF_GET_LIB_VRF (ifp));

  if (! igi)
    {
      ret = IGMP_ERR_NO_CONTEXT_INFO;
      goto EXIT;
    }

  ret = igmp_if_snoop_report_suppress_unset (igi, ifp);

EXIT:

  IGMP_FN_EXIT (igmp_cli_return (cli, ret));
}

CLI (if_ip_igmp_version,
     if_ip_igmp_version_cmd,
     "ip igmp version"
     IGMP_CLI_RANGE_STR (IGMP_VERSION_MIN, IGMP_VERSION_MAX),
     CLI_IP_STR,
     CLI_IGMP_STR,
     "IGMP Version",
     "Version Number"
     IGMP_CLI_DEF_VAL_STR (IGMP_VERSION_DEF))
{
  struct igmp_instance *igi;
  struct interface *ifp;
  u_int32_t version;
  s_int32_t ret;

  IGMP_FN_ENTER ();

  ret = IGMP_ERR_NONE;

  CLI_GET_UINT32_RANGE ("Version",
                        version, argv [0],
                        IGMP_VERSION_MIN,
                        IGMP_VERSION_MAX);

  ifp = (struct interface *) cli->index;

  if (! ifp)
    {
      ret = IGMP_ERR_NO_SUCH_IFF;
      goto EXIT;
    }

  if (! LIB_IF_GET_LIB_VR (ifp)
      || ! LIB_IF_GET_LIB_VRF (ifp))
    {
      ret = IGMP_ERR_NO_CONTEXT_INFO;
      goto EXIT;
    }

  /* Set Context */
  LIB_VR_SET_VRF_CONTEXT (LIB_IF_GET_LIB_VR (ifp),
                          LIB_IF_GET_LIB_VRF (ifp));

  igi = LIB_VRF_GET_IGMP_INSTANCE (LIB_IF_GET_LIB_VRF (ifp));

  if (! igi)
    {
      ret = IGMP_ERR_NO_CONTEXT_INFO;
      goto EXIT;
    }

  ret = igmp_if_version_set (igi, ifp, version);

EXIT:

  IGMP_FN_EXIT (igmp_cli_return (cli, ret));
}

CLI (no_if_ip_igmp_version,
     no_if_ip_igmp_version_cmd,
     "no ip igmp version",
     CLI_NO_STR,
     CLI_IP_STR,
     CLI_IGMP_STR,
     "Version")
{
  struct igmp_instance *igi;
  struct interface *ifp;
  s_int32_t ret;

  IGMP_FN_ENTER ();

  ret = IGMP_ERR_NONE;

  ifp = (struct interface *) cli->index;

  if (! ifp)
    {
      ret = IGMP_ERR_NO_SUCH_IFF;
      goto EXIT;
    }

  if (! LIB_IF_GET_LIB_VR (ifp)
      || ! LIB_IF_GET_LIB_VRF (ifp))
    {
      ret = IGMP_ERR_NO_CONTEXT_INFO;
      goto EXIT;
    }

  /* Set Context */
  LIB_VR_SET_VRF_CONTEXT (LIB_IF_GET_LIB_VR (ifp),
                          LIB_IF_GET_LIB_VRF (ifp));

  igi = LIB_VRF_GET_IGMP_INSTANCE (LIB_IF_GET_LIB_VRF (ifp));

  if (! igi)
    {
      ret = IGMP_ERR_NO_CONTEXT_INFO;
      goto EXIT;
    }

  ret = igmp_if_version_unset (igi, ifp);

EXIT:

  IGMP_FN_EXIT (igmp_cli_return (cli, ret));
}

CLI (if_ip_igmp_static_group,
     if_ip_igmp_static_group_cmd,
     "ip igmp static-group A.B.C.D"
     "{(source (A.B.C.D|ssm-map)|)"
     "(interface IFNAME|)}",
     CLI_IP_STR,
     CLI_IGMP_STR,
     "Static Group to be Joined",
     "Multicast Address to be Joined",
     "Static Source to be Joined",
     "Source Address to be Joined",
     "ssm mapping",
     "Interface to use",
     CLI_IFNAME_STR)
{
  struct igmp_instance *igi;
  struct pal_in4_addr grp;
  struct pal_in4_addr src;
  struct interface *ifp;
  bool_t is_ssm_mapped;
  u_int8_t *ifname;
  s_int32_t ret;

  IGMP_FN_ENTER (Static-Group-Source Add);

  pal_mem_set (&src, 0, sizeof (struct pal_in4_addr));
  is_ssm_mapped = PAL_FALSE;
  ret = IGMP_ERR_NONE;
  ifname = NULL;

  CLI_GET_IPV4_ADDRESS ("Group address", grp, argv [0]);

  if (argc == 5
      || argc == 3)
    {
      if (! pal_strncmp (argv [1], "source", 6))
        {
          if (pal_strncmp (argv [2], "ssm-map", 9))
            CLI_GET_IPV4_ADDRESS ("Source address", src, argv [2]);
          else
            is_ssm_mapped = PAL_TRUE;

          if (argc == 5)
            ifname = argv [4];
        }
      else
        ifname = argv [2];
    }

  ifp = (struct interface *) cli->index;

  if (! ifp)
    {
      ret = IGMP_ERR_NO_SUCH_IFF;
      goto EXIT;
    }

  if (! LIB_IF_GET_LIB_VR (ifp)
      || ! LIB_IF_GET_LIB_VRF (ifp))
    {
      ret = IGMP_ERR_NO_CONTEXT_INFO;
      goto EXIT;
    }

  /* Set Context */
  LIB_VR_SET_VRF_CONTEXT (LIB_IF_GET_LIB_VR (ifp),
                          LIB_IF_GET_LIB_VRF (ifp));

  igi = LIB_VRF_GET_IGMP_INSTANCE (LIB_IF_GET_LIB_VRF (ifp));

  if (! igi)
    {
      ret = IGMP_ERR_NO_CONTEXT_INFO;
      goto EXIT;
    }

  ret = igmp_if_static_group_source_set (igi, ifp, &grp, &src,
                                         ifname, is_ssm_mapped);

EXIT:

  IGMP_FN_EXIT (igmp_cli_return (cli, ret));
}

CLI (no_if_ip_igmp_static_group,
     no_if_ip_igmp_static_group_cmd,
     "no ip igmp static-group A.B.C.D"
     "{(source (A.B.C.D|ssm-map)|)"
     "(interface IFNAME|)}",
     CLI_NO_STR,
     CLI_IP_STR,
     CLI_IGMP_STR,
     "Static Group to be Joined",
     "Multicast Address to be Joined",
     "Static Source to be Joined",
     "Source Address to be Joined",
     "ssm mapping",
     "Interface to use",
     CLI_IFNAME_STR)
{
  struct igmp_instance *igi;
  struct pal_in4_addr grp;
  struct pal_in4_addr src;
  struct interface *ifp;
  bool_t is_ssm_mapped;
  u_int8_t *ifname;
  s_int32_t ret;

  IGMP_FN_ENTER (Static-Group-Source Del);

  pal_mem_set (&src, 0, sizeof (struct pal_in4_addr));
  is_ssm_mapped = PAL_FALSE;
  ret = IGMP_ERR_NONE;
  ifname = NULL;

  CLI_GET_IPV4_ADDRESS ("Group address", grp, argv [0]);

  if (argc == 5
      || argc == 3)
    {
      if (! pal_strncmp (argv [1], "source", 6))
        {
          if (pal_strncmp (argv [2], "ssm-map", 9))
            CLI_GET_IPV4_ADDRESS ("Source address", src, argv [2]);
          else
            is_ssm_mapped = PAL_TRUE;

          if (argc == 5)
            ifname = argv [4];
        }
      else
        ifname = argv [2];
    }

  ifp = (struct interface *) cli->index;

  if (! ifp)
    {
      ret = IGMP_ERR_NO_SUCH_IFF;
      goto EXIT;
    }

  if (! LIB_IF_GET_LIB_VR (ifp)
      || ! LIB_IF_GET_LIB_VRF (ifp))
    {
      ret = IGMP_ERR_NO_CONTEXT_INFO;
      goto EXIT;
    }

  /* Set Context */
  LIB_VR_SET_VRF_CONTEXT (LIB_IF_GET_LIB_VR (ifp),
                          LIB_IF_GET_LIB_VRF (ifp));

  igi = LIB_VRF_GET_IGMP_INSTANCE (LIB_IF_GET_LIB_VRF (ifp));

  if (! igi)
    {
      ret = IGMP_ERR_NO_CONTEXT_INFO;
      goto EXIT;
    }

  ret = igmp_if_static_group_source_unset (igi, ifp, &grp, &src,
                                           ifname, is_ssm_mapped);

EXIT:

  IGMP_FN_EXIT (igmp_cli_return (cli, ret));
}

/*
 * IGMP CLI Debug Configuration Commands
 */

#ifdef HAVE_VRF
CLI (show_debugging_igmp,
     show_debugging_igmp_cmd,
     "show debugging igmp (vrf NAME|)",
     CLI_SHOW_STR,
     CLI_DEBUG_STR,
     CLI_IGMP_STR,
     CLI_VRF_STR,
     CLI_VRF_NAME_STR)
#else
CLI (show_debugging_igmp,
     show_debugging_igmp_cmd,
     "show debugging igmp",
     CLI_SHOW_STR,
     CLI_DEBUG_STR,
     CLI_IGMP_STR)
#endif /* HAVE_VRF */
{
  struct igmp_instance *igi;
  struct apn_vrf *ivrf;
  s_int32_t ret;

  IGMP_FN_ENTER (Show Debugging status);

  ret = IGMP_ERR_NONE;

  /* Get the VRF Instance */
  if (argc > 1
      && ! pal_strcmp (argv [0], "vrf"))
    ivrf = apn_vrf_lookup_by_name (LIB_GLOB_GET_VR_CONTEXT (cli->zg), argv [1]);
  else
    ivrf = apn_vrf_lookup_by_name (LIB_GLOB_GET_VR_CONTEXT (cli->zg), NULL);

  if (! ivrf)
    {
      ret = IGMP_ERR_NO_CONTEXT_INFO;
      goto EXIT;
    }

  /* Get the IGMP Instance */
  igi = LIB_VRF_GET_IGMP_INSTANCE (ivrf);

  if (! igi)
    {
      ret = IGMP_ERR_NO_CONTEXT_INFO;
      goto EXIT;
    }

  cli_out (cli, "IGMP Debugging status");
#ifdef HAVE_VRF
  if (LIB_VRF_GET_VRF_NAME (ivrf))
    cli_out (cli, " for vrf %s");
#endif /* HAVE_VRF */
  cli_out (cli, ":\n");

  if (IGMP_DEBUG (igi, DECODE))
    cli_out (cli, "  IGMP Decoder debugging is on\n");
  else
    cli_out (cli, "  IGMP Decoder debugging is off\n");

  if (IGMP_DEBUG (igi, ENCODE))
    cli_out (cli, "  IGMP Encoder debugging is on\n");
  else
    cli_out (cli, "  IGMP Encoder debugging is off\n");

  if (IGMP_DEBUG (igi, EVENTS))
    cli_out (cli, "  IGMP Events debugging is on\n");
  else
    cli_out (cli, "  IGMP Events debugging is off\n");

  if (IGMP_DEBUG (igi, FSM))
    cli_out (cli, "  IGMP FSM debugging is on\n");
  else
    cli_out (cli, "  IGMP FSM debugging is off\n");

  if (IGMP_DEBUG (igi, TIB))
    cli_out (cli, "  IGMP Tree-Info-Base (TIB) debugging is on\n");
  else
    cli_out (cli, "  IGMP Tree-Info-Base (TIB) debugging is off\n");

EXIT:

  IGMP_FN_EXIT (ret);
}

#ifdef HAVE_VRF
CLI (debug_igmp_decode,
     debug_igmp_decode_cmd,
     "debug igmp (vrf NAME|) decode",
     CLI_DEBUG_STR,
     CLI_IGMP_STR,
     CLI_VRF_STR,
     CLI_VRF_NAME_STR,
     "IGMP decode")
#else
CLI (debug_igmp_decode,
     debug_igmp_decode_cmd,
     "debug igmp decode",
     CLI_DEBUG_STR,
     CLI_IGMP_STR,
     "IGMP decode")
#endif /* HAVE_VRF */
{
  struct igmp_instance *igi;
  struct apn_vrf *ivrf;
  s_int32_t ret;

  IGMP_FN_ENTER (Debug IGMP Decode);

  ret = IGMP_ERR_NONE;

  /* Get the VRF Instance */
  if (argc > 1
      && ! pal_strcmp (argv [0], "vrf"))
    ivrf = apn_vrf_lookup_by_name (LIB_GLOB_GET_VR_CONTEXT (cli->zg), argv [1]);
  else
    ivrf = apn_vrf_lookup_by_name (LIB_GLOB_GET_VR_CONTEXT (cli->zg), NULL);

  if (! ivrf)
    {
      ret = IGMP_ERR_NO_CONTEXT_INFO;
      goto EXIT;
    }

  /* Get the IGMP Instance */
  igi = LIB_VRF_GET_IGMP_INSTANCE (ivrf);

  if (! igi)
    {
      ret = IGMP_ERR_NO_CONTEXT_INFO;
      goto EXIT;
    }

  if (cli->mode == CONFIG_MODE)
    IGMP_DEBUG_CONF_ON (igi, DECODE);

  IGMP_DEBUG_TERM_ON (igi, DECODE);

EXIT:

  IGMP_FN_EXIT (igmp_cli_return (cli, ret));
}

#ifdef HAVE_VRF
CLI (no_debug_igmp_decode,
     no_debug_igmp_decode_cmd,
     "no debug igmp (vrf NAME|) decode",
     CLI_NO_STR,
     CLI_DEBUG_STR,
     CLI_IGMP_STR,
     CLI_VRF_STR,
     CLI_VRF_NAME_STR,
     "IGMP decode")
#else
CLI (no_debug_igmp_decode,
     no_debug_igmp_decode_cmd,
     "no debug igmp decode",
     CLI_NO_STR,
     CLI_DEBUG_STR,
     CLI_IGMP_STR,
     "IGMP decode")
#endif /* HAVE_VRF */
{
  struct igmp_instance *igi;
  struct apn_vrf *ivrf;
  s_int32_t ret;

  IGMP_FN_ENTER (No Debug IGMP Decode);

  ret = IGMP_ERR_NONE;

  /* Get the VRF Instance */
  if (argc > 1
      && ! pal_strcmp (argv [0], "vrf"))
    ivrf = apn_vrf_lookup_by_name (LIB_GLOB_GET_VR_CONTEXT (cli->zg), argv [1]);
  else
    ivrf = apn_vrf_lookup_by_name (LIB_GLOB_GET_VR_CONTEXT (cli->zg), NULL);

  if (! ivrf)
    {
      ret = IGMP_ERR_NO_CONTEXT_INFO;
      goto EXIT;
    }

  /* Get the IGMP Instance */
  igi = LIB_VRF_GET_IGMP_INSTANCE (ivrf);

  if (! igi)
    {
      ret = IGMP_ERR_NO_CONTEXT_INFO;
      goto EXIT;
    }

  if (cli->mode == CONFIG_MODE)
    IGMP_DEBUG_CONF_OFF (igi, DECODE);

  IGMP_DEBUG_TERM_OFF (igi, DECODE);

EXIT:

  IGMP_FN_EXIT (igmp_cli_return (cli, ret));
}

#ifdef HAVE_VRF
CLI (debug_igmp_encode,
     debug_igmp_encode_cmd,
     "debug igmp (vrf NAME|) encode",
     CLI_DEBUG_STR,
     CLI_IGMP_STR,
     CLI_VRF_STR,
     CLI_VRF_NAME_STR,
     "IGMP encode")
#else
CLI (debug_igmp_encode,
     debug_igmp_encode_cmd,
     "debug igmp encode",
     CLI_DEBUG_STR,
     CLI_IGMP_STR,
     "IGMP encode")
#endif /* HAVE_VRF */
{
  struct igmp_instance *igi;
  struct apn_vrf *ivrf;
  s_int32_t ret;

  IGMP_FN_ENTER (Debug IGMP Encode);

  ret = IGMP_ERR_NONE;

  /* Get the VRF Instance */
  if (argc > 1
      && ! pal_strcmp (argv [0], "vrf"))
    ivrf = apn_vrf_lookup_by_name (LIB_GLOB_GET_VR_CONTEXT (cli->zg), argv [1]);
  else
    ivrf = apn_vrf_lookup_by_name (LIB_GLOB_GET_VR_CONTEXT (cli->zg), NULL);

  if (! ivrf)
    {
      ret = IGMP_ERR_NO_CONTEXT_INFO;
      goto EXIT;
    }

  /* Get the IGMP Instance */
  igi = LIB_VRF_GET_IGMP_INSTANCE (ivrf);

  if (! igi)
    {
      ret = IGMP_ERR_NO_CONTEXT_INFO;
      goto EXIT;
    }

  if (cli->mode == CONFIG_MODE)
    IGMP_DEBUG_CONF_ON (igi, ENCODE);

  IGMP_DEBUG_TERM_ON (igi, ENCODE);

EXIT:

  IGMP_FN_EXIT (igmp_cli_return (cli, ret));
}

#ifdef HAVE_VRF
CLI (no_debug_igmp_encode,
     no_debug_igmp_encode_cmd,
     "no debug igmp (vrf NAME|) encode",
     CLI_NO_STR,
     CLI_DEBUG_STR,
     CLI_IGMP_STR,
     CLI_VRF_STR,
     CLI_VRF_NAME_STR,
     "IGMP encode")
#else
CLI (no_debug_igmp_encode,
     no_debug_igmp_encode_cmd,
     "no debug igmp encode",
     CLI_NO_STR,
     CLI_DEBUG_STR,
     CLI_IGMP_STR,
     "IGMP encode")
#endif /* HAVE_VRF */
{
  struct igmp_instance *igi;
  struct apn_vrf *ivrf;
  s_int32_t ret;

  IGMP_FN_ENTER (No Debug IGMP Encode);

  ret = IGMP_ERR_NONE;

  /* Get the VRF Instance */
  if (argc > 1
      && ! pal_strcmp (argv [0], "vrf"))
    ivrf = apn_vrf_lookup_by_name (LIB_GLOB_GET_VR_CONTEXT (cli->zg), argv [1]);
  else
    ivrf = apn_vrf_lookup_by_name (LIB_GLOB_GET_VR_CONTEXT (cli->zg), NULL);

  if (! ivrf)
    {
      ret = IGMP_ERR_NO_CONTEXT_INFO;
      goto EXIT;
    }

  /* Get the IGMP Instance */
  igi = LIB_VRF_GET_IGMP_INSTANCE (ivrf);

  if (! igi)
    {
      ret = IGMP_ERR_NO_CONTEXT_INFO;
      goto EXIT;
    }

  if (cli->mode == CONFIG_MODE)
    IGMP_DEBUG_CONF_OFF (igi, ENCODE);

  IGMP_DEBUG_TERM_OFF (igi, ENCODE);

EXIT:

  IGMP_FN_EXIT (igmp_cli_return (cli, ret));
}

#ifdef HAVE_VRF
CLI (debug_igmp_events,
     debug_igmp_events_cmd,
     "debug igmp (vrf NAME|) events",
     CLI_DEBUG_STR,
     CLI_IGMP_STR,
     CLI_VRF_STR,
     CLI_VRF_NAME_STR,
     "IGMP events")
#else
CLI (debug_igmp_events,
     debug_igmp_events_cmd,
     "debug igmp events",
     CLI_DEBUG_STR,
     CLI_IGMP_STR,
     "IGMP events")
#endif /* HAVE_VRF */
{
  struct igmp_instance *igi;
  struct apn_vrf *ivrf;
  s_int32_t ret;

  IGMP_FN_ENTER (Debug IGMP Events);

  ret = IGMP_ERR_NONE;

  /* Get the VRF Instance */
  if (argc > 1
      && ! pal_strcmp (argv [0], "vrf"))
    ivrf = apn_vrf_lookup_by_name (LIB_GLOB_GET_VR_CONTEXT (cli->zg), argv [1]);
  else
    ivrf = apn_vrf_lookup_by_name (LIB_GLOB_GET_VR_CONTEXT (cli->zg), NULL);

  if (! ivrf)
    {
      ret = IGMP_ERR_NO_CONTEXT_INFO;
      goto EXIT;
    }

  /* Get the IGMP Instance */
  igi = LIB_VRF_GET_IGMP_INSTANCE (ivrf);

  if (! igi)
    {
      ret = IGMP_ERR_NO_CONTEXT_INFO;
      goto EXIT;
    }

  if (cli->mode == CONFIG_MODE)
    IGMP_DEBUG_CONF_ON (igi, EVENTS);

  IGMP_DEBUG_TERM_ON (igi, EVENTS);

EXIT:

  IGMP_FN_EXIT (igmp_cli_return (cli, ret));
}

#ifdef HAVE_VRF
CLI (no_debug_igmp_events,
     no_debug_igmp_events_cmd,
     "no debug igmp (vrf NAME|) events",
     CLI_NO_STR,
     CLI_DEBUG_STR,
     CLI_IGMP_STR,
     CLI_VRF_STR,
     CLI_VRF_NAME_STR,
     "IGMP events")
#else
CLI (no_debug_igmp_events,
     no_debug_igmp_events_cmd,
     "no debug igmp events",
     CLI_NO_STR,
     CLI_DEBUG_STR,
     CLI_IGMP_STR,
     "IGMP events")
#endif /* HAVE_VRF */
{
  struct igmp_instance *igi;
  struct apn_vrf *ivrf;
  s_int32_t ret;

  IGMP_FN_ENTER (No Debug IGMP Events);

  ret = IGMP_ERR_NONE;

  /* Get the VRF Instance */
  if (argc > 1
      && ! pal_strcmp (argv [0], "vrf"))
    ivrf = apn_vrf_lookup_by_name (LIB_GLOB_GET_VR_CONTEXT (cli->zg), argv [1]);
  else
    ivrf = apn_vrf_lookup_by_name (LIB_GLOB_GET_VR_CONTEXT (cli->zg), NULL);

  if (! ivrf)
    {
      ret = IGMP_ERR_NO_CONTEXT_INFO;
      goto EXIT;
    }

  /* Get the IGMP Instance */
  igi = LIB_VRF_GET_IGMP_INSTANCE (ivrf);

  if (! igi)
    {
      ret = IGMP_ERR_NO_CONTEXT_INFO;
      goto EXIT;
    }

  if (cli->mode == CONFIG_MODE)
    IGMP_DEBUG_CONF_OFF (igi, EVENTS);

  IGMP_DEBUG_TERM_OFF (igi, EVENTS);

EXIT:

  IGMP_FN_EXIT (igmp_cli_return (cli, ret));
}

#ifdef HAVE_VRF
CLI (debug_igmp_fsm,
     debug_igmp_fsm_cmd,
     "debug igmp (vrf NAME|) fsm",
     CLI_DEBUG_STR,
     CLI_IGMP_STR,
     CLI_VRF_STR,
     CLI_VRF_NAME_STR,
     "IGMP FSM")
#else
CLI (debug_igmp_fsm,
     debug_igmp_fsm_cmd,
     "debug igmp fsm",
     CLI_DEBUG_STR,
     CLI_IGMP_STR,
     "IGMP FSM")
#endif /* HAVE_VRF */
{
  struct igmp_instance *igi;
  struct apn_vrf *ivrf;
  s_int32_t ret;

  IGMP_FN_ENTER (Debug IGMP FSM);

  ret = IGMP_ERR_NONE;

  /* Get the VRF Instance */
  if (argc > 1
      && ! pal_strcmp (argv [0], "vrf"))
    ivrf = apn_vrf_lookup_by_name (LIB_GLOB_GET_VR_CONTEXT (cli->zg), argv [1]);
  else
    ivrf = apn_vrf_lookup_by_name (LIB_GLOB_GET_VR_CONTEXT (cli->zg), NULL);

  if (! ivrf)
    {
      ret = IGMP_ERR_NO_CONTEXT_INFO;
      goto EXIT;
    }

  /* Get the IGMP Instance */
  igi = LIB_VRF_GET_IGMP_INSTANCE (ivrf);

  if (! igi)
    {
      ret = IGMP_ERR_NO_CONTEXT_INFO;
      goto EXIT;
    }

  if (cli->mode == CONFIG_MODE)
    IGMP_DEBUG_CONF_ON (igi, FSM);

  IGMP_DEBUG_TERM_ON (igi, FSM);

EXIT:

  IGMP_FN_EXIT (igmp_cli_return (cli, ret));
}

#ifdef HAVE_VRF
CLI (no_debug_igmp_fsm,
     no_debug_igmp_fsm_cmd,
     "no debug igmp (vrf NAME|) fsm",
     CLI_NO_STR,
     CLI_DEBUG_STR,
     CLI_IGMP_STR,
     CLI_VRF_STR,
     CLI_VRF_NAME_STR,
     "IGMP FSM")
#else
CLI (no_debug_igmp_fsm,
     no_debug_igmp_fsm_cmd,
     "no debug igmp fsm",
     CLI_NO_STR,
     CLI_DEBUG_STR,
     CLI_IGMP_STR,
     "IGMP FSM")
#endif /* HAVE_VRF */
{
  struct igmp_instance *igi;
  struct apn_vrf *ivrf;
  s_int32_t ret;

  IGMP_FN_ENTER (No Debug IGMP FSM);

  ret = IGMP_ERR_NONE;

  /* Get the VRF Instance */
  if (argc > 1
      && ! pal_strcmp (argv [0], "vrf"))
    ivrf = apn_vrf_lookup_by_name (LIB_GLOB_GET_VR_CONTEXT (cli->zg), argv [1]);
  else
    ivrf = apn_vrf_lookup_by_name (LIB_GLOB_GET_VR_CONTEXT (cli->zg), NULL);

  if (! ivrf)
    {
      ret = IGMP_ERR_NO_CONTEXT_INFO;
      goto EXIT;
    }

  /* Get the IGMP Instance */
  igi = LIB_VRF_GET_IGMP_INSTANCE (ivrf);

  if (! igi)
    {
      ret = IGMP_ERR_NO_CONTEXT_INFO;
      goto EXIT;
    }

  if (cli->mode == CONFIG_MODE)
    IGMP_DEBUG_CONF_OFF (igi, FSM);

  IGMP_DEBUG_TERM_OFF (igi, FSM);

EXIT:

  IGMP_FN_EXIT (igmp_cli_return (cli, ret));
}

#ifdef HAVE_VRF
CLI (debug_igmp_tib,
     debug_igmp_tib_cmd,
     "debug igmp (vrf NAME|) tib",
     CLI_DEBUG_STR,
     CLI_IGMP_STR,
     CLI_VRF_STR,
     CLI_VRF_NAME_STR,
     "IGMP Tree-Info-Base (TIB)")
#else
CLI (debug_igmp_tib,
     debug_igmp_tib_cmd,
     "debug igmp tib",
     CLI_DEBUG_STR,
     CLI_IGMP_STR,
     "IGMP Tree-Info-Base (TIB)")
#endif /* HAVE_VRF */
{
  struct igmp_instance *igi;
  struct apn_vrf *ivrf;
  s_int32_t ret;

  IGMP_FN_ENTER (Debug IGMP TIB);

  ret = IGMP_ERR_NONE;

  /* Get the VRF Instance */
  if (argc > 1
      && ! pal_strcmp (argv [0], "vrf"))
    ivrf = apn_vrf_lookup_by_name (LIB_GLOB_GET_VR_CONTEXT (cli->zg), argv [1]);
  else
    ivrf = apn_vrf_lookup_by_name (LIB_GLOB_GET_VR_CONTEXT (cli->zg), NULL);

  if (! ivrf)
    {
      ret = IGMP_ERR_NO_CONTEXT_INFO;
      goto EXIT;
    }

  /* Get the IGMP Instance */
  igi = LIB_VRF_GET_IGMP_INSTANCE (ivrf);

  if (! igi)
    {
      ret = IGMP_ERR_NO_CONTEXT_INFO;
      goto EXIT;
    }

  if (cli->mode == CONFIG_MODE)
    IGMP_DEBUG_CONF_ON (igi, TIB);

  IGMP_DEBUG_TERM_ON (igi, TIB);

EXIT:

  IGMP_FN_EXIT (igmp_cli_return (cli, ret));
}

#ifdef HAVE_VRF
CLI (no_debug_igmp_tib,
     no_debug_igmp_tib_cmd,
     "no debug igmp (vrf NAME|) tib",
     CLI_NO_STR,
     CLI_DEBUG_STR,
     CLI_IGMP_STR,
     CLI_VRF_STR,
     CLI_VRF_NAME_STR,
     "IGMP Tree-Info-Base (TIB)")
#else
CLI (no_debug_igmp_tib,
     no_debug_igmp_tib_cmd,
     "no debug igmp tib",
     CLI_NO_STR,
     CLI_DEBUG_STR,
     CLI_IGMP_STR,
     "IGMP Tree-Info-Base (TIB)")
#endif /* HAVE_VRF */
{
  struct igmp_instance *igi;
  struct apn_vrf *ivrf;
  s_int32_t ret;

  IGMP_FN_ENTER (No Debug IGMP TIB);

  ret = IGMP_ERR_NONE;

  /* Get the VRF Instance */
  if (argc > 1
      && ! pal_strcmp (argv [0], "vrf"))
    ivrf = apn_vrf_lookup_by_name (LIB_GLOB_GET_VR_CONTEXT (cli->zg), argv [1]);
  else
    ivrf = apn_vrf_lookup_by_name (LIB_GLOB_GET_VR_CONTEXT (cli->zg), NULL);

  if (! ivrf)
    {
      ret = IGMP_ERR_NO_CONTEXT_INFO;
      goto EXIT;
    }

  /* Get the IGMP Instance */
  igi = LIB_VRF_GET_IGMP_INSTANCE (ivrf);

  if (! igi)
    {
      ret = IGMP_ERR_NO_CONTEXT_INFO;
      goto EXIT;
    }

  if (cli->mode == CONFIG_MODE)
    IGMP_DEBUG_CONF_OFF (igi, TIB);

  IGMP_DEBUG_TERM_OFF (igi, TIB);

EXIT:

  IGMP_FN_EXIT (igmp_cli_return (cli, ret));
}

#ifdef HAVE_VRF
CLI (debug_igmp_all,
     debug_igmp_all_cmd,
     "debug igmp (vrf NAME|) all",
     CLI_DEBUG_STR,
     CLI_IGMP_STR,
     CLI_VRF_STR,
     CLI_VRF_NAME_STR,
     "All IGMP debugging")
#else
CLI (debug_igmp_all,
     debug_igmp_all_cmd,
     "debug igmp all",
     CLI_DEBUG_STR,
     CLI_IGMP_STR,
     "All IGMP debugging")
#endif /* HAVE_VRF */
{
  struct igmp_instance *igi;
  struct apn_vrf *ivrf;
  s_int32_t ret;

  IGMP_FN_ENTER (Debug IGMP All);

  ret = IGMP_ERR_NONE;

  /* Get the VRF Instance */
  if (argc > 1
      && ! pal_strcmp (argv [0], "vrf"))
    ivrf = apn_vrf_lookup_by_name (LIB_GLOB_GET_VR_CONTEXT (cli->zg), argv [1]);
  else
    ivrf = apn_vrf_lookup_by_name (LIB_GLOB_GET_VR_CONTEXT (cli->zg), NULL);

  if (! ivrf)
    {
      ret = IGMP_ERR_NO_CONTEXT_INFO;
      goto EXIT;
    }

  /* Get the IGMP Instance */
  igi = LIB_VRF_GET_IGMP_INSTANCE (ivrf);

  if (! igi)
    {
      ret = IGMP_ERR_NO_CONTEXT_INFO;
      goto EXIT;
    }

  if (cli->mode == CONFIG_MODE)
    {
      IGMP_DEBUG_CONF_ON (igi, DECODE);
      IGMP_DEBUG_CONF_ON (igi, ENCODE);
      IGMP_DEBUG_CONF_ON (igi, EVENTS);
      IGMP_DEBUG_CONF_ON (igi, FSM);
      IGMP_DEBUG_CONF_ON (igi, TIB);
    }

  IGMP_DEBUG_TERM_ON (igi, DECODE);
  IGMP_DEBUG_TERM_ON (igi, ENCODE);
  IGMP_DEBUG_TERM_ON (igi, EVENTS);
  IGMP_DEBUG_TERM_ON (igi, FSM);
  IGMP_DEBUG_TERM_ON (igi, TIB);

EXIT:

  IGMP_FN_EXIT (igmp_cli_return (cli, ret));
}
s_int32_t
igmp_debug_all_off(struct cli *cli, int argc, char **argv)
{
  struct igmp_instance *igi;
  struct apn_vrf *ivrf;
  s_int32_t ret;

  IGMP_FN_ENTER (No Debug IGMP All);

  ret = IGMP_ERR_NONE;

  /* Get the VRF Instance */
  if (argc > 1
      && ! pal_strcmp (argv [0], "vrf"))
    ivrf = apn_vrf_lookup_by_name (LIB_GLOB_GET_VR_CONTEXT (cli->zg), argv [1]);
  else
    ivrf = apn_vrf_lookup_by_name (LIB_GLOB_GET_VR_CONTEXT (cli->zg), NULL);

  if (! ivrf)
    {
      ret = IGMP_ERR_NO_CONTEXT_INFO;
      goto EXIT;
    }

  /* Get the associated IGMP Instance */
  igi = LIB_VRF_GET_IGMP_INSTANCE (ivrf);

  if (! igi)
    {
      ret = IGMP_ERR_NO_CONTEXT_INFO;
      goto EXIT;
    }

  if (cli->mode == CONFIG_MODE)
    {
      IGMP_DEBUG_CONF_OFF (igi, DECODE);
      IGMP_DEBUG_CONF_OFF (igi, ENCODE);
      IGMP_DEBUG_CONF_OFF (igi, EVENTS);
      IGMP_DEBUG_CONF_OFF (igi, FSM);
      IGMP_DEBUG_CONF_OFF (igi, TIB);
    }

  IGMP_DEBUG_TERM_OFF (igi, DECODE);
  IGMP_DEBUG_TERM_OFF (igi, ENCODE);
  IGMP_DEBUG_TERM_OFF (igi, EVENTS);
  IGMP_DEBUG_TERM_OFF (igi, FSM);
  IGMP_DEBUG_TERM_OFF (igi, TIB);

EXIT:

  IGMP_FN_EXIT (igmp_cli_return (cli, ret));
}

#ifdef HAVE_VRF
CLI (no_debug_igmp_all,
     no_debug_igmp_all_cmd,
     "no debug igmp (vrf NAME|) all",
     CLI_NO_STR,
     CLI_DEBUG_STR,
     CLI_IGMP_STR,
     CLI_VRF_STR,
     CLI_VRF_NAME_STR,
     "All IGMP debugging")
#else
CLI (no_debug_igmp_all,
     no_debug_igmp_all_cmd,
     "no debug igmp all",
     CLI_NO_STR,
     CLI_DEBUG_STR,
     CLI_IGMP_STR,
     "All IGMP debugging")
#endif /* HAVE_VRF */
{
  s_int32_t ret = IGMP_ERR_NONE;
  ret = igmp_debug_all_off(cli, argc, argv);

  if (ret < 0)
    return CLI_ERROR;
  else
    return CLI_SUCCESS;
}
#ifdef HAVE_VRF
ALI (no_debug_igmp_all,
     no_debug_igmp_all_dbg_cmd,
     "no debug vrf NAME all",
     CLI_NO_STR,
     CLI_DEBUG_STR,
     CLI_VRF_STR,
     CLI_VRF_NAME_STR,
     "Disable all debugging");
#endif /* HAVE_VRF */

/*
 * IGMP Clear Configuration Commands
 */

#ifdef HAVE_VRF
CLI (clear_igmp,
     clear_igmp_cmd,
     "clear ip igmp (vrf NAME|)",
     CLI_CLEAR_STR,
     CLI_IP_STR,
     CLI_IGMP_STR,
     CLI_VRF_STR,
     CLI_VRF_NAME_STR)
#else
CLI (clear_igmp,
     clear_igmp_cmd,
     "clear ip igmp",
     CLI_CLEAR_STR,
     CLI_IP_STR,
     CLI_IGMP_STR)
#endif /* HAVE_VRF */
{
  struct igmp_instance *igi;
  struct apn_vrf *ivrf;
  s_int32_t ret;

  IGMP_FN_ENTER (Clear All Groups);

  ret = IGMP_ERR_NONE;

  /* Get the VRF Instance */
  if (argc > 1
      && ! pal_strcmp (argv [0], "vrf"))
    ivrf = apn_vrf_lookup_by_name (LIB_GLOB_GET_VR_CONTEXT (cli->zg), argv [1]);
  else
    ivrf = apn_vrf_lookup_by_name (LIB_GLOB_GET_VR_CONTEXT (cli->zg), NULL);

  if (! ivrf)
    {
      ret = IGMP_ERR_NO_CONTEXT_INFO;
      goto EXIT;
    }

  /* Get the associated IGMP Instance */
  igi = LIB_VRF_GET_IGMP_INSTANCE (ivrf);

  if (! igi)
    {
      ret = IGMP_ERR_NO_CONTEXT_INFO;
      goto EXIT;
    }

  ret = igmp_clear (igi, NULL, NULL, NULL);

EXIT:

  IGMP_FN_EXIT (igmp_cli_return (cli, ret));
}

#ifdef HAVE_VRF
ALI (clear_igmp,
     clear_igmp_group_x_cmd,
     "clear ip igmp (vrf NAME|) group *",
     CLI_CLEAR_STR,
     CLI_IP_STR,
     CLI_IGMP_STR,
     CLI_VRF_STR,
     CLI_VRF_NAME_STR,
     "Delete IGMP group cache entries",
     "All groups");
#else
ALI (clear_igmp,
     clear_igmp_group_x_cmd,
     "clear ip igmp group *",
     CLI_CLEAR_STR,
     CLI_IP_STR,
     CLI_IGMP_STR,
     "Delete IGMP group cache entries",
     "All groups");
#endif /* HAVE_VRF */

#ifdef HAVE_VRF
CLI (clear_igmp_group_address,
     clear_igmp_group_address_cmd,
     "clear ip igmp (vrf NAME|) group A.B.C.D",
     CLI_CLEAR_STR,
     CLI_IP_STR,
     CLI_IGMP_STR,
     CLI_VRF_STR,
     CLI_VRF_NAME_STR,
     "Delete IGMP group cache entries",
     "Multicast group address")
#else
CLI (clear_igmp_group_address,
     clear_igmp_group_address_cmd,
     "clear ip igmp group A.B.C.D",
     CLI_CLEAR_STR,
     CLI_IP_STR,
     CLI_IGMP_STR,
     "Delete IGMP group cache entries",
     "Multicast group address")
#endif /* HAVE_VRF */
{
  struct igmp_instance *igi;
  struct pal_in4_addr grp;
  struct apn_vrf *ivrf;
  s_int32_t ret;

  IGMP_FN_ENTER (Clear Groups);

  ret = IGMP_ERR_NONE;

  /* Get the VRF Instance and arguments */
  if (argc > 2
      && ! pal_strcmp (argv [0], "vrf"))
    {
      ivrf = apn_vrf_lookup_by_name (LIB_GLOB_GET_VR_CONTEXT (cli->zg), argv [1]);

      CLI_GET_IPV4_ADDRESS ("Group address", grp, argv [2]);
    }
  else
    {
      ivrf = apn_vrf_lookup_by_name (LIB_GLOB_GET_VR_CONTEXT (cli->zg), NULL);

      CLI_GET_IPV4_ADDRESS ("Group address", grp, argv [0]);
    }

  if (! ivrf)
    {
      ret = IGMP_ERR_NO_CONTEXT_INFO;
      goto EXIT;
    }

  /* Get the associated IGMP Instance */
  igi = LIB_VRF_GET_IGMP_INSTANCE (ivrf);

  if (! igi)
    {
      ret = IGMP_ERR_NO_CONTEXT_INFO;
      goto EXIT;
    }

  ret = igmp_clear (igi, NULL, &grp, NULL);

EXIT:

  IGMP_FN_EXIT (igmp_cli_return (cli, ret));
}

#ifdef HAVE_VRF
CLI (clear_igmp_group_address_interface,
     clear_igmp_group_address_interface_cmd,
     "clear ip igmp (vrf NAME|) group A.B.C.D IFNAME",
     CLI_CLEAR_STR,
     CLI_IP_STR,
     CLI_IGMP_STR,
     CLI_VRF_STR,
     CLI_VRF_NAME_STR,
     "Delete IGMP group cache entries",
     "Multicast group Address",
     CLI_IFNAME_STR)
#else
CLI (clear_igmp_group_address_interface,
     clear_igmp_group_address_interface_cmd,
     "clear ip igmp group A.B.C.D IFNAME",
     CLI_CLEAR_STR,
     CLI_IP_STR,
     CLI_IGMP_STR,
     "Delete IGMP group cache entries",
     "Multicast group Address",
     CLI_IFNAME_STR)
#endif /* HAVE_VRF */
{
  struct igmp_instance *igi;
  struct pal_in4_addr grp;
  struct interface *ifp;
  struct apn_vrf *ivrf;
  s_int32_t ret;

  IGMP_FN_ENTER (Clear Groups IFNAME);

  ret = IGMP_ERR_NONE;

  /* Get the VRF Instance and arguments */
  if (argc > 2
      && ! pal_strcmp (argv [0], "vrf"))
    {
      ivrf = apn_vrf_lookup_by_name (LIB_GLOB_GET_VR_CONTEXT (cli->zg), argv [1]);

      if (! ivrf)
        {
          ret = IGMP_ERR_NO_CONTEXT_INFO;
          goto EXIT;
        }

      CLI_GET_IPV4_ADDRESS ("Group address", grp, argv [2]);

      ifp = ifv_lookup_by_name (&ivrf->ifv, argv [3]);
    }
  else
    {
      ivrf = apn_vrf_lookup_by_name (LIB_GLOB_GET_VR_CONTEXT (cli->zg), NULL);

      if (! ivrf)
        {
          ret = IGMP_ERR_NO_CONTEXT_INFO;
          goto EXIT;
        }

      CLI_GET_IPV4_ADDRESS ("Group address", grp, argv [0]);

      ifp = ifv_lookup_by_name (&ivrf->ifv, argv [1]);
    }

  /* Get the IGMP Instance */
  igi = LIB_VRF_GET_IGMP_INSTANCE (ivrf);

  if (! igi)
    {
      ret = IGMP_ERR_NO_CONTEXT_INFO;
      goto EXIT;
    }

  if (! ifp)
    {
      ret = IGMP_ERR_NO_SUCH_IFF;
      goto EXIT;
    }

  ret = igmp_clear (igi, ifp, &grp, NULL);

EXIT:

  IGMP_FN_EXIT (igmp_cli_return (cli, ret));
}

#ifdef HAVE_VRF
CLI (clear_igmp_interface,
     clear_igmp_interface_cmd,
     "clear ip igmp (vrf NAME|) interface IFNAME",
     CLI_CLEAR_STR,
     CLI_IP_STR,
     CLI_IGMP_STR,
     CLI_VRF_STR,
     CLI_VRF_NAME_STR,
     "Clear Interface IGMP entries",
     CLI_IFNAME_STR)
#else
CLI (clear_igmp_interface,
     clear_igmp_interface_cmd,
     "clear ip igmp interface IFNAME",
     CLI_CLEAR_STR,
     CLI_IP_STR,
     CLI_IGMP_STR,
     "Clear Interface IGMP entries",
     CLI_IFNAME_STR)
#endif /* HAVE_VRF */
{
  struct igmp_instance *igi;
  struct interface *ifp;
  struct apn_vrf *ivrf;
  s_int32_t ret;

  IGMP_FN_ENTER (Clear IntF IFNAME);

  ret = IGMP_ERR_NONE;

  /* Get the VRF Instance and arguments */
  if (argc > 2
      && ! pal_strcmp (argv [0], "vrf"))
    {
      ivrf = apn_vrf_lookup_by_name (LIB_GLOB_GET_VR_CONTEXT (cli->zg), argv [1]);

      if (! ivrf)
        {
          ret = IGMP_ERR_NO_CONTEXT_INFO;
          goto EXIT;
        }

      ifp = ifv_lookup_by_name (&ivrf->ifv, argv [2]);
    }
  else
    {
      ivrf = apn_vrf_lookup_by_name (LIB_GLOB_GET_VR_CONTEXT (cli->zg), NULL);

      if (! ivrf)
        {
          ret = IGMP_ERR_NO_CONTEXT_INFO;
          goto EXIT;
        }

      ifp = ifv_lookup_by_name (&ivrf->ifv, argv [0]);
    }

  /* Get the associated IGMP Instance */
  igi = LIB_VRF_GET_IGMP_INSTANCE (ivrf);

  if (! igi)
    {
      ret = IGMP_ERR_NO_CONTEXT_INFO;
      goto EXIT;
    }

  if (! ifp)
    {
      ret = IGMP_ERR_NO_SUCH_IFF;
      goto EXIT;
    }

  ret = igmp_clear (igi, ifp, NULL, NULL);

EXIT:

  IGMP_FN_EXIT (igmp_cli_return (cli, ret));
}

/* offlink igmp */
CLI (if_ip_igmp_offlink,
     if_ip_igmp_offlink_cmd,
     "ip igmp offlink",
     CLI_IP_STR,
     CLI_IGMP_STR,
     "Offlink")
{
  struct igmp_instance *igi;
  struct interface *ifp;
  s_int32_t ret;

  IGMP_FN_ENTER ();

  ret = IGMP_ERR_NONE;

  ifp = (struct interface *) cli->index;

  if (! ifp)
    {
      ret = IGMP_ERR_NO_SUCH_IFF;
      goto EXIT;
    }

  if (! LIB_IF_GET_LIB_VR (ifp)
      || ! LIB_IF_GET_LIB_VRF (ifp))
    {
      ret = IGMP_ERR_NO_CONTEXT_INFO;
      goto EXIT;
    }

  /* Set Context */
  LIB_VR_SET_VRF_CONTEXT (LIB_IF_GET_LIB_VR (ifp),
                          LIB_IF_GET_LIB_VRF (ifp));

  igi = LIB_VRF_GET_IGMP_INSTANCE (LIB_IF_GET_LIB_VRF (ifp));

  if (! igi)
    {
      ret = IGMP_ERR_NO_CONTEXT_INFO;
      goto EXIT;
    }

  ret = igmp_offlink_if_set (igi, ifp);

EXIT:

  IGMP_FN_EXIT (igmp_cli_return (cli, ret));
}

CLI (no_if_ip_igmp_offlink,
     no_if_ip_igmp_offlink_cmd,
     "no ip igmp offlink",
     CLI_NO_STR,
     CLI_IP_STR,
     CLI_IGMP_STR,
     "Offlink")
{
  struct igmp_instance *igi;
  struct interface *ifp;
  s_int32_t ret;

  IGMP_FN_ENTER ();

  ret = IGMP_ERR_NONE;

  ifp = (struct interface *) cli->index;

  if (! ifp)
    {
      ret = IGMP_ERR_NO_SUCH_IFF;
      goto EXIT;
    }

  if (! LIB_IF_GET_LIB_VR (ifp)
      || ! LIB_IF_GET_LIB_VRF (ifp))
    {
      ret = IGMP_ERR_NO_CONTEXT_INFO;
      goto EXIT;
    }

  /* Set Context */
  LIB_VR_SET_VRF_CONTEXT (LIB_IF_GET_LIB_VR (ifp),
                          LIB_IF_GET_LIB_VRF (ifp));

  igi = LIB_VRF_GET_IGMP_INSTANCE (LIB_IF_GET_LIB_VRF (ifp));

  if (! igi)
    {
      ret = IGMP_ERR_NO_CONTEXT_INFO;
      goto EXIT;
    }

  ret = igmp_offlink_if_unset (igi, ifp);

EXIT:

  IGMP_FN_EXIT (igmp_cli_return (cli, ret));
}

/*join igmp*/
CLI (if_ip_igmp_join_group,
     if_ip_igmp_join_group_cmd,
     "ip igmp join-group A.B.C.D",
     CLI_IP_STR,
     CLI_IGMP_STR,
     "Joined Group",
     "Multicast Address to be Joined")
{
  struct igmp_instance *igi;
  struct pal_in4_addr grp;
  struct interface *ifp;
  s_int32_t ret;

  IGMP_FN_ENTER (Joined-Group Add);

  ret = IGMP_ERR_NONE;

  CLI_GET_IPV4_ADDRESS ("Group address", grp, argv [0]);
  
  ifp = (struct interface *) cli->index;

  if (! ifp)
    {
      ret = IGMP_ERR_NO_SUCH_IFF;
      goto EXIT;
    }

  if (! LIB_IF_GET_LIB_VR (ifp)
      || ! LIB_IF_GET_LIB_VRF (ifp))
    {
      ret = IGMP_ERR_NO_CONTEXT_INFO;
      goto EXIT;
    }

  /* Set Context */
  LIB_VR_SET_VRF_CONTEXT (LIB_IF_GET_LIB_VR (ifp),
                          LIB_IF_GET_LIB_VRF (ifp));

  igi = LIB_VRF_GET_IGMP_INSTANCE (LIB_IF_GET_LIB_VRF (ifp));

  if (! igi)
    {
      ret = IGMP_ERR_NO_CONTEXT_INFO;
      goto EXIT;
    }

  ret = igmp_if_join_group_set (igi, ifp, &grp);

EXIT:

  IGMP_FN_EXIT (igmp_cli_return (cli, ret));
}

CLI (no_if_ip_igmp_join_group,
     no_if_ip_igmp_join_group_cmd,
     "no ip igmp join-group A.B.C.D",
     CLI_NO_STR,
     CLI_IP_STR,
     CLI_IGMP_STR,
     "Joined Group",
     "Multicast Address to be Joined")
{
  struct igmp_instance *igi;
  struct pal_in4_addr grp;
  struct interface *ifp;
  s_int32_t ret;

  IGMP_FN_ENTER (Joined-Group Del);

  ret = IGMP_ERR_NONE;

  CLI_GET_IPV4_ADDRESS ("Group address", grp, argv [0]);
  
  ifp = (struct interface *) cli->index;

  if (! ifp)
    {
      ret = IGMP_ERR_NO_SUCH_IFF;
      goto EXIT;
    }

  if (! LIB_IF_GET_LIB_VR (ifp)
      || ! LIB_IF_GET_LIB_VRF (ifp))
    {
      ret = IGMP_ERR_NO_CONTEXT_INFO;
      goto EXIT;
    }

  /* Set Context */
  LIB_VR_SET_VRF_CONTEXT (LIB_IF_GET_LIB_VR (ifp),
                          LIB_IF_GET_LIB_VRF (ifp));

  igi = LIB_VRF_GET_IGMP_INSTANCE (LIB_IF_GET_LIB_VRF (ifp));

  if (! igi)
    {
      ret = IGMP_ERR_NO_CONTEXT_INFO;
      goto EXIT;
    }

  ret = igmp_if_join_group_unset (igi, ifp, &grp);

EXIT:

  IGMP_FN_EXIT (igmp_cli_return (cli, ret));
}

/*
 * IGMP CLI Utility Routines
 */

/* IGMP Error Codes Interpretation Strings */
s_int32_t
igmp_cli_return (struct cli *cli,
                 s_int32_t iret)
{
  u_int8_t *str;
  s_int32_t ret;

  IGMP_FN_ENTER (CLI Return);

  ret = CLI_SUCCESS;

  str = igmp_strerror (iret);

  if (str)
    cli_out (cli, "%% %s\n", str);

  if (iret < IGMP_ERR_NONE)
    ret = CLI_ERROR;

  IGMP_FN_EXIT (ret);
}

/* IGMP Debug-mode Instance Config-write Function */
s_int32_t
igmp_debug_config_write_instance (struct cli *cli,
                                  struct igmp_instance *igi)
{
  u_int8_t debug_str [80];
  s_int32_t write;

  IGMP_FN_ENTER (Debug Config Write Inst);

  debug_str [0] = '\0';
  write = 0;

  if (igi->igi_conf_dbg_flags)
    {
      pal_snprintf (debug_str, 80, "debug igmp");

#ifdef HAVE_VRF
      if (LIB_VRF_GET_VRF_NAME (igi->igi_owning_ivrf))
        {
          pal_strcat (debug_str, " vrf ");
          pal_strcat (debug_str, LIB_VRF_GET_VRF_NAME (igi->igi_owning_ivrf));
        }
#endif /* HAVE_VRF */

      write += 1;
    }

  if (IGMP_DEBUG_CONF (igi, DECODE))
    cli_out (cli, "%s decode\n", debug_str);

  if (IGMP_DEBUG_CONF (igi, ENCODE))
    cli_out (cli, "%s encode\n", debug_str);

  if (IGMP_DEBUG_CONF (igi, EVENTS))
    cli_out (cli, "%s events\n", debug_str);

  if (IGMP_DEBUG_CONF (igi, FSM))
    cli_out (cli, "%s fsm\n", debug_str);

  if (IGMP_DEBUG_CONF (igi, TIB))
    cli_out (cli, "%s tib\n", debug_str);

  cli_out (cli, "!\n");

  IGMP_FN_EXIT (write);
}

/* IGMP Debug-mode Config-write Function */
s_int32_t
igmp_debug_config_write (struct cli *cli)
{
  struct igmp_instance *igi;
  struct apn_vrf *ivrf;
  struct apn_vr *ivr;
  s_int32_t write;
  u_int32_t idx;
  s_int32_t ret;

  IGMP_FN_ENTER (Debug Config Write);

  ret = IGMP_ERR_NONE;

  ivr = LIB_GLOB_GET_VR_CONTEXT (cli->zg);
  write = 0;

  if (! ivr)
    {
      ret = IGMP_ERR_MALFORMED_ARG;
      goto EXIT;
    }

  /* Get all associated IGMP Instances */
  VECTOR_LOOP (ivr->vrf_vec, ivrf, idx)
    if ((igi = LIB_VRF_GET_IGMP_INSTANCE (ivrf)))
      write += igmp_debug_config_write_instance (cli, igi);

  if (write)
    cli_out (cli, "!\n");

EXIT:

  IGMP_FN_EXIT (write);
}

/* IGMP Global Config-Mode Config Write Instance Function */
s_int32_t
igmp_config_write_instance (struct cli *cli,
                            struct igmp_instance *igi)
{
  struct igmp_ssm_map_static *igssms;
  struct listnode *nn;
  s_int32_t write;

  IGMP_FN_ENTER (Global Config Write Instance);

  write = 0;

  if (CHECK_FLAG (igi->igi_cflags,
                  IGMP_INST_CFLAG_LIMIT_GREC))
    {
      cli_out (cli, "ip igmp");

#ifdef HAVE_VRF
      if (LIB_VRF_GET_VRF_NAME (igi->igi_owning_ivrf))
        cli_out (cli, " vrf %s", LIB_VRF_GET_VRF_NAME (igi->igi_owning_ivrf));
#endif /* HAVE_VRF */

      cli_out (cli, " limit %u", igi->igi_limit);

      if (igi->igi_limit_except_alist)
        cli_out (cli, " except %s", igi->igi_limit_except_alist);

      cli_out (cli, "\n");

      write += 1;
    }

  if (CHECK_FLAG (igi->igi_cflags,
                  IGMP_INST_CFLAG_SNOOP_DISABLED))
    {
      cli_out (cli, "no ip igmp");

#ifdef HAVE_VRF
      if (LIB_VRF_GET_VRF_NAME (igi->igi_owning_ivrf))
        cli_out (cli, " vrf %s", LIB_VRF_GET_VRF_NAME (igi->igi_owning_ivrf));
#endif /* HAVE_VRF */

      cli_out (cli, " snooping\n");

      write += 1;
    }

  if (CHECK_FLAG (igi->igi_cflags,
                  IGMP_INST_CFLAG_SSM_MAP_DISABLED))
    {
      cli_out (cli, "no ip igmp");

#ifdef HAVE_VRF
      if (LIB_VRF_GET_VRF_NAME (igi->igi_owning_ivrf))
        cli_out (cli, " vrf %s", LIB_VRF_GET_VRF_NAME (igi->igi_owning_ivrf));
#endif /* HAVE_VRF */

      cli_out (cli, " ssm-map enable\n");

      write += 1;
    }

  if (CHECK_FLAG (igi->igi_cflags,
                  IGMP_INST_CFLAG_SSM_MAP_STATIC))
    LIST_LOOP (&igi->igi_ssm_map_static_lst, igssms, nn)
      {
        cli_out (cli, "ip igmp");

#ifdef HAVE_VRF
        if (LIB_VRF_GET_VRF_NAME (igi->igi_owning_ivrf))
          cli_out (cli, " vrf %s", LIB_VRF_GET_VRF_NAME (igi->igi_owning_ivrf));
#endif /* HAVE_VRF */

        cli_out (cli, " ssm-map static %s %r\n",
                 igssms->igssms_grp_alist, &igssms->igssms_msrc);

        write += 1;
      }

  if (write)
    cli_out (cli, "!\n");

  IGMP_FN_EXIT (write);
}

/* IGMP Global Config-Mode Config Write Function */
s_int32_t
igmp_config_write (struct cli *cli)
{
  struct igmp_instance *igi;
  struct apn_vrf *ivrf;
  struct apn_vr *ivr;
  s_int32_t write;
  u_int32_t idx;
  s_int32_t ret;

  IGMP_FN_ENTER (Global Config Write);

  ret = IGMP_ERR_NONE;

  ivr = LIB_GLOB_GET_VR_CONTEXT (cli->zg);
  write = 0;

  if (! ivr)
    {
      ret = IGMP_ERR_MALFORMED_ARG;
      goto EXIT;
    }

  /* Get all associated IGMP Instances */
  VECTOR_LOOP (ivr->vrf_vec, ivrf, idx)
    if ((igi = LIB_VRF_GET_IGMP_INSTANCE (ivrf)))
      write += igmp_config_write_instance (cli, igi);

  if (write)
    cli_out (cli, "!\n");

EXIT:

  IGMP_FN_EXIT (ret);
}

/* IGMP Interface Config-Mode Config-Write Iterator */
s_int32_t
igmp_if_avl_traverse_config_write (void_t *avl_node_info,
                                   void_t *arg1,
                                   void_t *arg2)
{
  struct interface *ifp = NULL;
  struct igmp_if *igif = NULL;
  struct cli *cli = NULL;
  s_int32_t ret = 0;

  igif = (struct igmp_if *) avl_node_info;
  cli = arg2;
  ifp = (struct interface *)cli->index;
  
  ret = igmp_if_config_write_all (igif, arg1, arg2, ifp);

  return ret;
}

/* Displays the IGMP interface related config-information */
s_int32_t
igmp_if_config_write_all (struct igmp_if *igif, s_int32_t *write, 
                          struct cli *cli, struct interface *ifp)
{

  struct igmp_if_idx *mrtr_igifidx = NULL;
  struct igmp_source_rec *isr = NULL;
  struct igmp_group_rec *igr = NULL;
  struct ptree_node *pn_src = NULL;
  struct igmp_if *ds_igif = NULL;
  struct ptree_node *pn = NULL;
  struct listnode *nn = NULL;
  s_int32_t ret = 0;

  IGMP_FN_ENTER (IF Hash Iterate Config Write);

  ds_igif = NULL;

  /* Do not display the configuration information for pure L2 interfaces */

  if (INTF_TYPE_L2 (igif->igif_owning_ifp))
    goto EXIT;

#ifdef HAVE_IMI
  cli_out (cli, "interface %s\n",
           igif->igif_owning_ifp->name);
#else
  if (!pal_strcmp (ifp->name, igif->igif_idx.igifidx_ifp->name))
    {
#endif /* HAVE_IMI */

  if (CHECK_FLAG (igif->igif_conf.igifc_cflags,
                  IGMP_IF_CFLAG_CONFIG_ENABLED))
    cli_out (cli, " ip igmp\n");

  if (CHECK_FLAG (igif->igif_conf.igifc_cflags,
                  IGMP_OFFLINK_IF_CFLAG_CONFIG_ENABLED))
    cli_out (cli, " ip igmp offlink\n");

  if (CHECK_FLAG (igif->igif_conf.igifc_cflags,
                  IGMP_IF_CFLAG_ACCESS_LIST))
    cli_out (cli, " ip igmp access-group %s\n",
             igif->igif_conf.igifc_access_list);

  if (CHECK_FLAG (igif->igif_conf.igifc_cflags,
                  IGMP_IF_CFLAG_IMMEDIATE_LEAVE))
    cli_out (cli, " ip igmp immediate-leave group-list %s\n",
             igif->igif_conf.igifc_immediate_leave);

  if (CHECK_FLAG (igif->igif_conf.igifc_cflags,
                  IGMP_IF_CFLAG_LAST_MEMBER_QUERY_COUNT)
      && igif->igif_conf.igifc_lmqc != IGMP_LAST_MEMBER_QUERY_COUNT_DEF)
    cli_out (cli, " ip igmp last-member-query-count %u\n",
             igif->igif_conf.igifc_lmqc);

  if (igif->igif_conf.igifc_lmqi != IGMP_LAST_MEMBER_QUERY_INTERVAL_DEF)
    cli_out (cli, " ip igmp last-member-query-interval %u\n",
             igif->igif_conf.igifc_lmqi);

  if (CHECK_FLAG (igif->igif_conf.igifc_cflags,
                  IGMP_IF_CFLAG_LIMIT_GREC))
    {
      cli_out (cli, " ip igmp limit %u",
               igif->igif_conf.igifc_limit);

      if (igif->igif_conf.igifc_limit_except_alist)
        cli_out (cli, " except %s",
                 igif->igif_conf.igifc_limit_except_alist);

      cli_out (cli, "\n");
    }

  if (if_is_vlan (igif->igif_owning_ifp))
    {
      LIST_LOOP (&igif->igif_hst_chld_lst, ds_igif, nn)
        {
          for (pn = ptree_top (ds_igif->igif_gmr_tib); pn;
               pn = ptree_next (pn))
            {
              if (! (igr = pn->info))
                continue;

              if (CHECK_FLAG (igr->igr_cflags, IGMP_IGR_CFLAG_STATIC_GROUP))
                {
                  if ( ! igr->igr_src_a_tib_count &&
                       ! (CHECK_FLAG (igr->igr_cflags,
                                      IGMP_IGR_CFLAG_STATIC_SOURCE_SSM_MAP)))
                    {
                      cli_out (cli, " ip igmp static-group %r",
                               PTREE_NODE_KEY (igr->igr_owning_pn));

                      if (CHECK_FLAG (igr->igr_cflags,
                                      IGMP_IGR_CFLAG_STATIC_GROUP_IF_NAME))
                        cli_out (cli, " interface %s",
                                 ds_igif->igif_owning_ifp->name);

                      cli_out (cli, "\n");
                    }

                  if (CHECK_FLAG (igr->igr_cflags,
                                  IGMP_IGR_CFLAG_STATIC_GROUP_SOURCE))
                    {
                      for (pn_src = ptree_top (igr->igr_src_a_tib);
                           pn_src; pn_src = ptree_next (pn_src))
                        {
                          if (! (isr = pn_src->info))
                            continue;

                          cli_out (cli, " ip igmp static-group %r",
                                   PTREE_NODE_KEY (igr->igr_owning_pn));

                          if (CHECK_FLAG (isr->isr_sflags,
                                          IGMP_ISR_SFLAG_STATIC))
                            {
                              cli_out (cli, " source %r",
                                       PTREE_NODE_KEY (isr->isr_owning_pn));

                              if (CHECK_FLAG (igr->igr_cflags,
                                              IGMP_IGR_CFLAG_STATIC_GROUP_IF_NAME))
                                cli_out (cli, " interface %s",
                                         ds_igif->igif_owning_ifp->name);

                              cli_out (cli, "\n");
                            }
                        }
                    }

                  if (CHECK_FLAG (igr->igr_cflags,
                                  IGMP_IGR_CFLAG_STATIC_SOURCE_SSM_MAP))
                    {
                      cli_out (cli, " ip igmp static-group %r",
                               PTREE_NODE_KEY (igr->igr_owning_pn));

                      cli_out (cli, " source ssm-map");

                      if (CHECK_FLAG (igr->igr_cflags,
                                      IGMP_IGR_CFLAG_STATIC_GROUP_IF_NAME))
                        cli_out (cli, " interface %s",
                                 ds_igif->igif_owning_ifp->name);

                      cli_out (cli, "\n");
                    }
                }
            }
        }
    }
  else
    {
      for (pn = ptree_top (igif->igif_gmr_tib); pn; pn = ptree_next (pn))
        {
          if (! (igr = pn->info))
            continue;

          if (CHECK_FLAG (igr->igr_cflags, IGMP_IGR_CFLAG_STATIC_GROUP))
            {
              if ( ! igr->igr_src_a_tib_count &&
                   ! (CHECK_FLAG (igr->igr_cflags,
                                  IGMP_IGR_CFLAG_STATIC_SOURCE_SSM_MAP)))
                cli_out (cli, " ip igmp static-group %r\n",
                         PTREE_NODE_KEY (igr->igr_owning_pn));

              if (CHECK_FLAG (igr->igr_cflags,
                              IGMP_IGR_CFLAG_STATIC_GROUP_SOURCE))
                {
                  for (pn_src = ptree_top (igr->igr_src_a_tib); pn_src;
                       pn_src = ptree_next (pn_src))
                    {
                      if (! (isr = pn_src->info))
                        continue;

                      if (CHECK_FLAG (isr->isr_sflags,
                                      IGMP_ISR_SFLAG_STATIC))
                        {
                          cli_out (cli, " ip igmp static-group %r",
                                   PTREE_NODE_KEY (igr->igr_owning_pn));

                          cli_out (cli, " source %r\n",
                                   PTREE_NODE_KEY (isr->isr_owning_pn));
                        }
                    }
                }

              if (CHECK_FLAG (igr->igr_cflags,
                              IGMP_IGR_CFLAG_STATIC_SOURCE_SSM_MAP))
                {
                  cli_out (cli, " ip igmp static-group %r",
                           PTREE_NODE_KEY (igr->igr_owning_pn));

                  cli_out (cli, " source ssm-map \n");
                }
            }
          if (CHECK_FLAG (igr->igr_sflags, IGMP_IGR_SFLAG_JOIN))
            {
              cli_out (cli, " ip igmp join-group %r\n",
                         PTREE_NODE_KEY (igr->igr_owning_pn));
            }
        }
    }

  if (CHECK_FLAG (igif->igif_conf.igifc_cflags,
                  IGMP_IF_CFLAG_MROUTE_PROXY)
      && ! LIST_ISEMPTY (&igif->igif_conf.igifc_mrouter_if_lst))
    {
      mrtr_igifidx = GETDATA (LISTHEAD
                     (&igif->igif_conf.igifc_mrouter_if_lst));
      if (mrtr_igifidx)
        cli_out (cli, " ip igmp mroute-proxy %s\n",
                 mrtr_igifidx->igifidx_ifp->name);
    }

  if (CHECK_FLAG (igif->igif_conf.igifc_cflags,
                  IGMP_IF_CFLAG_PROXY_SERVICE))
    cli_out (cli, " ip igmp proxy-service\n");

  if (igif->igif_conf.igifc_oqi != IGMP_QUERIER_TIMEOUT_DEF)
    cli_out (cli, " ip igmp querier-timeout %u\n",
             igif->igif_conf.igifc_oqi);

  if (igif->igif_conf.igifc_qri != IGMP_QUERY_RESPONSE_INTERVAL_DEF)
    cli_out (cli, " ip igmp query-max-response-time %u\n",
             igif->igif_conf.igifc_qri);

  if (igif->igif_conf.igifc_qi != IGMP_QUERY_INTERVAL_DEF)
    cli_out (cli, " ip igmp query-interval %u\n",
             igif->igif_conf.igifc_qi);
  
  if (CHECK_FLAG (igif->igif_conf.igifc_cflags,
                  IGMP_IF_CFLAG_STARTUP_QUERY_INTERVAL)
      && igif->igif_conf.igifc_sqi != IGMP_STARTUP_QUERY_INTERVAL_DEF)
    cli_out (cli, " ip igmp startup-query-interval %u\n",
             igif->igif_conf.igifc_sqi);

  if (CHECK_FLAG (igif->igif_conf.igifc_cflags,
                  IGMP_IF_CFLAG_STARTUP_QUERY_COUNT)
      && igif->igif_conf.igifc_sqc != IGMP_STARTUP_QUERY_COUNT_DEF)
    cli_out (cli, " ip igmp startup-query-count %u\n",
             igif->igif_conf.igifc_sqc);

  if (igif->igif_conf.igifc_rv != IGMP_ROBUSTNESS_VAR_DEF)
    cli_out (cli, " ip igmp robustness-variable %u\n",
             igif->igif_conf.igifc_rv);

  if (CHECK_FLAG (igif->igif_conf.igifc_cflags,
                  IGMP_IF_CFLAG_RA_OPT))
    cli_out (cli, " ip igmp ra-option\n");

  if (CHECK_FLAG (igif->igif_conf.igifc_cflags,
                  IGMP_IF_CFLAG_SNOOP_ENABLED))
    cli_out (cli, " ip igmp snooping\n");
  else if (CHECK_FLAG (igif->igif_conf.igifc_cflags,
                       IGMP_IF_CFLAG_SNOOP_DISABLED))
    cli_out (cli, " no ip igmp snooping\n");

  if (CHECK_FLAG (igif->igif_conf.igifc_cflags,
                  IGMP_IF_CFLAG_SNOOP_FAST_LEAVE))
    cli_out (cli, " ip igmp snooping fast-leave\n");

  if (CHECK_FLAG (igif->igif_conf.igifc_cflags,
                  IGMP_IF_CFLAG_SNOOP_MROUTER_IF))
    LIST_LOOP (&igif->igif_conf.igifc_mrouter_if_lst,
               mrtr_igifidx, nn)
      if (mrtr_igifidx->igifidx_ifp)
        cli_out (cli, " ip igmp snooping mrouter interface %s\n",
                 mrtr_igifidx->igifidx_ifp->name);

  if (CHECK_FLAG (igif->igif_conf.igifc_cflags,
                  IGMP_IF_CFLAG_SNOOP_QUERIER))
    cli_out (cli, " ip igmp snooping querier\n");

  if (CHECK_FLAG (igif->igif_conf.igifc_cflags,
                  IGMP_IF_CFLAG_SNOOP_NO_REPORT_SUPPRESS))
    cli_out (cli, " no ip igmp snooping report-suppression\n");

  if (igif->igif_conf.igifc_version != IGMP_VERSION_DEF)
    cli_out (cli, " ip igmp version %u\n",
             igif->igif_conf.igifc_version);

  (*write) += 1;

#ifndef HAVE_IMI
  }
#endif /* !HAVE_IMI */

EXIT:

  IGMP_FN_EXIT (ret);
  return ret;
}

/* IGMP Interface Config-Mode Config Write Function - 
   Invoked when imi is disabled */
#ifndef HAVE_IMI
s_int32_t
igmp_config_write_interface (struct cli *cli, struct interface *ifp)
{
  struct igmp_instance *igi = NULL;
  struct igmp_if *igif = NULL;
  struct igmp_if_idx igifidx;
  s_int32_t write;

  IGMP_FN_ENTER (IF Config Write);

  write = 0;

  if ((igi = LIB_VRF_GET_IGMP_INSTANCE (LIB_IF_GET_LIB_VRF (ifp))))
    {
      igifidx.igifidx_ifp = ifp;
      igifidx.igifidx_sid = igmp_if_svc_reg_su_id (igi, ifp);
      igif = igmp_if_lookup (igi, &igifidx);

      if (igif)
        igmp_if_config_write_all (igif, &write, cli, ifp);
      else 
        goto EXIT;
    }

  if (write)
    cli_out (cli, "!\n");

EXIT:

  IGMP_FN_EXIT (write);
}
#endif /* !HAVE_IMI */

/* IGMP Interface Config-Mode Config Write Function */
s_int32_t
igmp_if_config_write (struct cli *cli)
{
  struct igmp_instance *igi;
  struct apn_vrf *ivrf;
  struct apn_vr *ivr;
  s_int32_t write;
  u_int32_t idx;

  IGMP_FN_ENTER (IF Config Write);

  write = 0;

  ivr = LIB_GLOB_GET_VR_CONTEXT (cli->zg);

  if (! ivr)
    goto EXIT;

  /* Get all associated MLD Instances */
  VECTOR_LOOP (ivr->vrf_vec, ivrf, idx)
    if ((igi = LIB_VRF_GET_IGMP_INSTANCE (ivrf)))
      avl_traverse2 (igi->igi_if_tree,
                     igmp_if_avl_traverse_config_write, &write, cli);

  if (write)
    cli_out (cli, "!\n");

EXIT:

  IGMP_FN_EXIT (write);
  return 0;
}

/* IGMP CLI Commands Initilization */
s_int32_t
igmp_cli_init (struct lib_globals *zlg)
{
  struct cli_tree *ctree;
  s_int32_t ret;

  IGMP_FN_ENTER (IGMP CLI Init);

  ret = IGMP_ERR_NONE;
  ctree = zlg->ctree;

  ret = igmp_cli_show_init (zlg);

  if (ret < IGMP_ERR_NONE)
    goto EXIT;

  /* IGMP Global Config-write function */
  cli_install_config (ctree, IGMP_MODE, igmp_config_write);

  /* IGMP Interface Config-write function */
#ifdef HAVE_IMI
  cli_install_config (ctree, IGMP_IF_MODE, igmp_if_config_write);
#endif /* HAVE_IMI */

  /* IGMP Global Configuration Commands */
  cli_install (ctree, CONFIG_MODE, &ip_igmp_limit_cmd);
  cli_install (ctree, CONFIG_MODE, &no_ip_igmp_limit_cmd);

  cli_install (ctree, CONFIG_MODE, &ip_igmp_snooping_cmd);
  cli_install (ctree, CONFIG_MODE, &no_ip_igmp_snooping_cmd);

  cli_install (ctree, CONFIG_MODE, &ip_igmp_ssm_map_enable_cmd);
  cli_install (ctree, CONFIG_MODE, &no_ip_igmp_ssm_map_enable_cmd);

  cli_install (ctree, CONFIG_MODE, &ip_igmp_ssm_map_static_cmd);
  cli_install (ctree, CONFIG_MODE, &no_ip_igmp_ssm_map_static_cmd);

  /* IGMP Interface Commands */
  cli_install (ctree, INTERFACE_MODE, &if_ip_igmp_cmd);
  cli_install (ctree, INTERFACE_MODE, &no_if_ip_igmp_cmd);

  cli_install (ctree, INTERFACE_MODE, &if_ip_igmp_access_group_cmd);
  cli_install (ctree, INTERFACE_MODE, &no_if_ip_igmp_access_group_cmd);

  cli_install (ctree, INTERFACE_MODE, &if_ip_igmp_immediate_leave_cmd);
  cli_install (ctree, INTERFACE_MODE, &no_if_ip_igmp_immediate_leave_cmd);

  cli_install (ctree, INTERFACE_MODE, &if_ip_igmp_last_member_query_count_cmd);
  cli_install (ctree, INTERFACE_MODE, &no_if_ip_igmp_last_member_query_count_cmd);

  cli_install (ctree, INTERFACE_MODE, &if_ip_igmp_last_member_query_interval_cmd);
  cli_install (ctree, INTERFACE_MODE, &no_if_ip_igmp_last_member_query_interval_cmd);

  cli_install (ctree, INTERFACE_MODE, &if_ip_igmp_limit_cmd);
  cli_install (ctree, INTERFACE_MODE, &no_if_ip_igmp_limit_cmd);

  cli_install (ctree, INTERFACE_MODE, &if_ip_igmp_mroute_proxy_cmd);
  cli_install (ctree, INTERFACE_MODE, &no_if_ip_igmp_mroute_proxy_cmd);

  cli_install (ctree, INTERFACE_MODE, &if_ip_igmp_proxy_service_cmd);
  cli_install (ctree, INTERFACE_MODE, &no_if_ip_igmp_proxy_service_cmd);

  cli_install (ctree, INTERFACE_MODE, &if_ip_igmp_querier_timeout_cmd);
  cli_install (ctree, INTERFACE_MODE, &no_if_ip_igmp_querier_timeout_cmd);

  cli_install (ctree, INTERFACE_MODE, &if_ip_igmp_query_interval_cmd);
  cli_install (ctree, INTERFACE_MODE, &no_if_ip_igmp_query_interval_cmd);
  
  cli_install (ctree, INTERFACE_MODE, &if_ip_igmp_startup_query_interval_cmd);
  cli_install (ctree, INTERFACE_MODE, &no_if_ip_igmp_startup_query_interval_cmd);

  cli_install (ctree, INTERFACE_MODE, &if_ip_igmp_startup_query_count_cmd);
  cli_install (ctree, INTERFACE_MODE, &no_if_ip_igmp_startup_query_count_cmd);

  cli_install (ctree, INTERFACE_MODE, &if_ip_igmp_query_max_response_time_cmd);
  cli_install (ctree, INTERFACE_MODE, &no_if_ip_igmp_query_max_response_time_cmd);

  cli_install (ctree, INTERFACE_MODE, &if_ip_igmp_robustness_variable_cmd);
  cli_install (ctree, INTERFACE_MODE, &no_if_ip_igmp_robustness_variable_cmd);

  cli_install (ctree, INTERFACE_MODE, &if_ip_igmp_ra_option_cmd);
  cli_install (ctree, INTERFACE_MODE, &no_if_ip_igmp_ra_option_cmd);

  cli_install (ctree, INTERFACE_MODE, &if_ip_igmp_snooping_cmd);
  cli_install (ctree, INTERFACE_MODE, &no_if_ip_igmp_snooping_cmd);

  cli_install (ctree, INTERFACE_MODE, &if_ip_igmp_snooping_fast_leave_cmd);
  cli_install (ctree, INTERFACE_MODE, &no_if_ip_igmp_snooping_fast_leave_cmd);

  cli_install (ctree, INTERFACE_MODE, &if_ip_igmp_snooping_mrouter_if_cmd);
  cli_install (ctree, INTERFACE_MODE, &no_if_ip_igmp_snooping_mrouter_if_cmd);

  cli_install (ctree, INTERFACE_MODE, &if_ip_igmp_snooping_querier_cmd);
  cli_install (ctree, INTERFACE_MODE, &no_if_ip_igmp_snooping_querier_cmd);

  cli_install (ctree, INTERFACE_MODE, &if_ip_igmp_snooping_report_suppress_cmd);
  cli_install (ctree, INTERFACE_MODE, &no_if_ip_igmp_snooping_report_suppress_cmd);

  cli_install (ctree, INTERFACE_MODE, &if_ip_igmp_version_cmd);
  cli_install (ctree, INTERFACE_MODE, &no_if_ip_igmp_version_cmd);

  cli_install (ctree, INTERFACE_MODE, &if_ip_igmp_static_group_cmd);
  cli_install (ctree, INTERFACE_MODE, &no_if_ip_igmp_static_group_cmd);

  cli_install (ctree, INTERFACE_MODE, &if_ip_igmp_offlink_cmd);
  cli_install (ctree, INTERFACE_MODE, &no_if_ip_igmp_offlink_cmd);
  
  cli_install (ctree, INTERFACE_MODE, &if_ip_igmp_join_group_cmd);
  cli_install (ctree, INTERFACE_MODE, &no_if_ip_igmp_join_group_cmd);

  /* Debug Commands (EXEC-MODE) */
  cli_install (ctree, EXEC_PRIV_MODE, &show_debugging_igmp_cmd);
  cli_install (ctree, EXEC_PRIV_MODE, &debug_igmp_decode_cmd);
  cli_install (ctree, EXEC_PRIV_MODE, &no_debug_igmp_decode_cmd);
  cli_install (ctree, EXEC_PRIV_MODE, &debug_igmp_encode_cmd);
  cli_install (ctree, EXEC_PRIV_MODE, &no_debug_igmp_encode_cmd);
  cli_install (ctree, EXEC_PRIV_MODE, &debug_igmp_events_cmd);
  cli_install (ctree, EXEC_PRIV_MODE, &no_debug_igmp_events_cmd);
  cli_install (ctree, EXEC_PRIV_MODE, &debug_igmp_fsm_cmd);
  cli_install (ctree, EXEC_PRIV_MODE, &no_debug_igmp_fsm_cmd);
  cli_install (ctree, EXEC_PRIV_MODE, &debug_igmp_tib_cmd);
  cli_install (ctree, EXEC_PRIV_MODE, &no_debug_igmp_tib_cmd);
  cli_install (ctree, EXEC_PRIV_MODE, &debug_igmp_all_cmd);
  cli_install (ctree, EXEC_PRIV_MODE, &no_debug_igmp_all_cmd);
#ifdef HAVE_VRF
  cli_install (ctree, EXEC_PRIV_MODE, &no_debug_igmp_all_dbg_cmd);
#endif /* HAVE_VRF */

  /* Debug Commands (CONFIG-MODE) */
  cli_install (ctree, CONFIG_MODE, &debug_igmp_decode_cmd);
  cli_install (ctree, CONFIG_MODE, &no_debug_igmp_decode_cmd);
  cli_install (ctree, CONFIG_MODE, &debug_igmp_encode_cmd);
  cli_install (ctree, CONFIG_MODE, &no_debug_igmp_encode_cmd);
  cli_install (ctree, CONFIG_MODE, &debug_igmp_events_cmd);
  cli_install (ctree, CONFIG_MODE, &no_debug_igmp_events_cmd);
  cli_install (ctree, CONFIG_MODE, &debug_igmp_fsm_cmd);
  cli_install (ctree, CONFIG_MODE, &no_debug_igmp_fsm_cmd);
  cli_install (ctree, CONFIG_MODE, &debug_igmp_tib_cmd);
  cli_install (ctree, CONFIG_MODE, &no_debug_igmp_tib_cmd);
  cli_install (ctree, CONFIG_MODE, &debug_igmp_all_cmd);
  cli_install (ctree, CONFIG_MODE, &no_debug_igmp_all_cmd);
#ifdef HAVE_VRF
  cli_install (ctree, CONFIG_MODE, &no_debug_igmp_all_dbg_cmd);
#endif /* HAVE_VRF */

  /* Clear commands */
  cli_install (ctree, EXEC_PRIV_MODE, &clear_igmp_cmd);
  cli_install (ctree, EXEC_PRIV_MODE, &clear_igmp_group_x_cmd);
  cli_install (ctree, EXEC_PRIV_MODE, &clear_igmp_group_address_cmd);
  cli_install (ctree, EXEC_PRIV_MODE, &clear_igmp_group_address_interface_cmd);
  cli_install (ctree, EXEC_PRIV_MODE, &clear_igmp_interface_cmd);

EXIT:

  IGMP_FN_EXIT (ret);
}

