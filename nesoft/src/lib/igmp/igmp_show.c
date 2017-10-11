/* Copyright (C) 2002-2005  All Rights Reserved. */

#include <igmp_incl.h>

/*
 * IGMP Show CLI Utility Routines
 */

/* IGMP Show Group Information in Summary */
void_t
igmp_if_show_group_summary (struct cli *cli,
                            struct igmp_group_rec *igr,
                            u_int32_t *disp_flags)
{
  struct pal_timeval timer_now;
  u_int8_t timebuf [TIME_BUF];
  struct igmp_if *igif;

  IGMP_FN_ENTER (Show Grp Summary);

  igif = NULL;
  igif = igr->igr_owning_igif;

  if (CHECK_FLAG ((*disp_flags), IGMP_SHOW_FLAG_DISP_HEADER))
    {
      cli_out (cli, "IGMP Connected Group Membership\n");
      cli_out (cli, "Group Address    Interface            Uptime"
               "   Expires  Last Reporter\n");

      UNSET_FLAG ((*disp_flags), IGMP_SHOW_FLAG_DISP_HEADER);
    }
  else if (CHECK_FLAG ((*disp_flags), IGMP_SHOW_FLAG_DISP_PROXY_HEADER))
    {
      cli_out (cli, "IGMP Connected Proxy Group Membership\n");
      cli_out (cli, "Group Address    Interface            Member state\n");

      UNSET_FLAG ((*disp_flags), IGMP_SHOW_FLAG_DISP_PROXY_HEADER);
    }

  if (CHECK_FLAG ((*disp_flags), IGMP_SHOW_FLAG_DISP_GROUP))
    {
      cli_out (cli, "%-15r  %-20s ",
               PTREE_NODE_KEY (igr->igr_owning_pn),
               igif->igif_owning_ifp->name);

      timeutil_uptime (timebuf, TIME_BUF,
                       pal_time_current (NULL) - igr->igr_uptime);

      cli_out (cli, "%8s ", timebuf);

      if (igr->t_igr_liveness)
        {
          if (! CHECK_FLAG (igr->igr_sflags, IGMP_IGR_SFLAG_STATIC))
            {
              pal_time_tzcurrent (&timer_now, NULL);

              timeval_uptime (timebuf, TIME_BUF,
                              timeval_subtract (THREAD_TIME_VAL
                                                (igr->t_igr_liveness),
                                                timer_now));

            }
          else
            pal_strcpy (timebuf, "static");
        }
      else
        pal_strcpy (timebuf, "stopped");

      cli_out (cli, "%8s ", timebuf);

      cli_out (cli, "%r\n", &igr->igr_last_reporter);
    }
  else if (CHECK_FLAG ((*disp_flags), IGMP_SHOW_FLAG_DISP_PROXY_GROUP))
    {
      cli_out (cli, "%-15r  %-20s ",
               PTREE_NODE_KEY (igr->igr_owning_pn),
               igif->igif_owning_ifp->name);

      if (igr->t_igr_rexmit_group || igr->t_igr_rexmit_group_source)
        cli_out(cli, "Idle\n");
      else
        cli_out(cli, "Delay\n");
    }

  IGMP_FN_EXIT ();
}

/* IGMP Show Group Information in Detail */
void_t
igmp_if_show_group_detail (struct cli *cli,
                           struct igmp_group_rec *igr,
                           u_int32_t *disp_flags)
{
  struct pal_timeval timer_now;
  u_int8_t timebuf [TIME_BUF];
  struct igmp_source_rec *isr;
  struct ptree_node *pn;
  struct igmp_if *igif;

  IGMP_FN_ENTER (Show Grp Detail);

  igif = NULL;
  isr = NULL;
  pn = NULL;

  igif = igr->igr_owning_igif;

  if (disp_flags
      && CHECK_FLAG ((*disp_flags), IGMP_SHOW_FLAG_DISP_HEADER))
    {
      /* No Flags info yet. Place-holder for later. */
      cli_out (cli, "\n");

      UNSET_FLAG ((*disp_flags), IGMP_SHOW_FLAG_DISP_HEADER);
    }

  if (CHECK_FLAG ((*disp_flags), IGMP_SHOW_FLAG_DISP_GROUP))
    {
      cli_out (cli, "%-15s %s\n", "Interface:",
               igif->igif_owning_ifp->name);

      cli_out (cli, "%-16s%r\n", "Group:",
               PTREE_NODE_KEY (igr->igr_owning_pn));

      timeutil_uptime (timebuf, TIME_BUF,
                       pal_time_current (NULL) - igr->igr_uptime);

      cli_out (cli, "%-16s%s\n", "Uptime:", timebuf);

      cli_out (cli, "%-16s%s", "Group mode:",
               IGMP_FMS_STR (igr->igr_filt_mode_state));

      if (CHECK_FLAG (igr->igr_sflags, IGMP_IGR_SFLAG_DYNAMIC)
          || CHECK_FLAG (igr->igr_sflags, IGMP_IGR_SFLAG_STATIC)
          || CHECK_FLAG (igr->igr_sflags, IGMP_IGR_SFLAG_JOIN))
        cli_out (cli, " (");
      if (igr->t_igr_liveness)
        {
          if (CHECK_FLAG (igr->igr_sflags, IGMP_IGR_SFLAG_DYNAMIC))
            {
              pal_time_tzcurrent (&timer_now, NULL);

              timeval_uptime (timebuf, TIME_BUF,
                              timeval_subtract (THREAD_TIME_VAL
                                                (igr->t_igr_liveness),
                                                timer_now));

              cli_out (cli, "Expires: %s", timebuf);
            }

          if (CHECK_FLAG (igr->igr_sflags, IGMP_IGR_SFLAG_DYNAMIC)
              && CHECK_FLAG (igr->igr_sflags, IGMP_IGR_SFLAG_STATIC))
            cli_out (cli, "%s", ", ");
        }
      if (CHECK_FLAG (igr->igr_sflags, IGMP_IGR_SFLAG_STATIC))
        cli_out (cli, "%s", "Static");
      if (CHECK_FLAG (igr->igr_sflags, IGMP_IGR_SFLAG_JOIN))
        cli_out (cli, "%s", "Join");
      if (CHECK_FLAG (igr->igr_sflags, IGMP_IGR_SFLAG_DYNAMIC)
          || CHECK_FLAG (igr->igr_sflags, IGMP_IGR_SFLAG_STATIC)
          || CHECK_FLAG (igr->igr_sflags, IGMP_IGR_SFLAG_JOIN))
        cli_out (cli, ")");
      cli_out (cli, "\n");
 
      cli_out (cli, "%-16s%r\n", "Last reporter:",
               &igr->igr_last_reporter);

#ifdef HAVE_IGMP_DEV_DEBUG
      cli_out (cli, "%-16s%d\n", "TIB-A Count:", igr->igr_src_a_tib_count);
      cli_out (cli, "%-16s%d\n", "TIB-B Count:", igr->igr_src_b_tib_count);
#endif /* HAVE_IGMP_DEV_DEBUG */

      if (igr->igr_src_a_tib_count || igr->igr_src_b_tib_count)
        {
              cli_out (cli, "Group source list: (R - Remote, M - SSM Mapping, "
                       "S - Static)\n");

              if (igr->igr_src_a_tib_count)
                {
                  cli_out (cli, "\n");
                  cli_out (cli, "Include Source List :\n");
                  cli_out (cli, "  Source Address  Uptime    v3 Exp    Fwd  "
                           "Flags\n");
                }

          for (pn = ptree_top (igr->igr_src_a_tib); pn; pn = ptree_next (pn))
            {
              if (! (isr = pn->info))
                continue;

              cli_out (cli, "  %-16r", PTREE_NODE_KEY (isr->isr_owning_pn));
 
              timeutil_uptime (timebuf, TIME_BUF,
                               pal_time_current (NULL) - isr->isr_uptime);

              cli_out (cli, "%-10s", timebuf);
  
              if (isr->t_isr_liveness)
                {
                  pal_time_tzcurrent (&timer_now, NULL);

                  timeval_uptime (timebuf, TIME_BUF,
                                  timeval_subtract (THREAD_TIME_VAL
                                                    (isr->t_isr_liveness),
                                                    timer_now));
                }
              else
                pal_strcpy (timebuf, "stopped");
              cli_out (cli, "%-10s", timebuf);

              cli_out (cli, "%-5s", "Yes");

              if (CHECK_FLAG (isr->isr_sflags, IGMP_ISR_SFLAG_DYNAMIC))
                cli_out (cli, "%s", "R");
              if (CHECK_FLAG (isr->isr_sflags, IGMP_ISR_SFLAG_STATIC))
                cli_out (cli, "%s", "S");
              cli_out (cli, "\n");
            }

          if (igr->igr_src_b_tib_count)
            {
              cli_out (cli, "\n");
              cli_out (cli, "Exclude Source List :\n");
              cli_out (cli, "  Source Address  Uptime    v3 Exp    Fwd  "
                       "Flags\n");
            }
    
          for (pn = ptree_top (igr->igr_src_b_tib); pn; pn = ptree_next (pn))
            {
              if (! (isr = pn->info))
                continue;

              cli_out (cli, "  %-16r", PTREE_NODE_KEY (isr->isr_owning_pn));
 
              timeutil_uptime (timebuf, TIME_BUF,
                               pal_time_current (NULL) - isr->isr_uptime);

              cli_out (cli, "%-10s", timebuf);

              if (isr->t_isr_liveness)
                {
                  pal_time_tzcurrent (&timer_now, NULL);

                  timeval_uptime (timebuf, TIME_BUF,
                                  timeval_subtract (THREAD_TIME_VAL
                                                    (isr->t_isr_liveness),
                                                    timer_now));
                }
              else
                pal_strcpy (timebuf, "stopped");
              cli_out (cli, "%-10s", timebuf);

              cli_out (cli, "%-5s", "No");

              if (CHECK_FLAG (isr->isr_sflags, IGMP_ISR_SFLAG_DYNAMIC))
                cli_out (cli, "%s", "R");
              if (CHECK_FLAG (isr->isr_sflags, IGMP_ISR_SFLAG_STATIC))
                cli_out (cli, "%s", "S");
              cli_out (cli, "\n");
            }
        }
      else
        cli_out (cli, "Source list is empty\n");

      cli_out (cli, "\n");
    }
  else if (CHECK_FLAG ((*disp_flags), IGMP_SHOW_FLAG_DISP_PROXY_GROUP))
    {
      cli_out (cli, "%-15s %s\n", "Interface:",
               igif->igif_owning_ifp->name);

      cli_out (cli, "%-16s%r\n", "Group:",
               PTREE_NODE_KEY (igr->igr_owning_pn));

      cli_out (cli, "%-16s%s\n", "Group mode:",
               IGMP_FMS_STR (igr->igr_filt_mode_state));

      cli_out(cli, "Member state:");

      if ( igr->t_igr_rexmit_group || igr->t_igr_rexmit_group_source)
        cli_out(cli, "   Idle\n");
      else
        cli_out(cli, "   Delay\n");

      if (igr->igr_src_a_tib_count || igr->igr_src_b_tib_count)
        {
          cli_out (cli, "Group source list: \n");

          cli_out (cli, "  Source Address  \n");

          for (pn = ptree_top (igr->igr_src_a_tib); pn; pn = ptree_next (pn))
            {
              if (! (isr = pn->info))
                continue;

              cli_out (cli, "  %-16r", PTREE_NODE_KEY (isr->isr_owning_pn));

              cli_out (cli, "\n");
            }

          for (pn = ptree_top (igr->igr_src_b_tib); pn; pn = ptree_next (pn))
            {
              if (! (isr = pn->info))
                continue;

              cli_out (cli, "  %-16r", PTREE_NODE_KEY (isr->isr_owning_pn));

              cli_out (cli, "\n");
            }
        }
      else
        cli_out (cli, "Source list is empty\n");
      cli_out (cli, "\n");
    }

  IGMP_FN_EXIT ();
}

/* IGMP IF Hash Table Iterate Show Groups */
s_int32_t
igmp_if_avl_traverse_show_groups (void_t *avl_node_info,
                                  void_t *arg1,
                                  void_t *arg2,
                                  void_t *arg3)
{
  struct igmp_group_rec *igr;
  struct pal_in4_addr *pgrp;
  u_int32_t *disp_flags;
  struct ptree_node *pn;
  struct ptree *gmr_tib;
  struct igmp_if *igif;
  struct cli *cli;
  s_int32_t ret;

  IGMP_FN_ENTER (IGMP IF Hash Iterate Show Groups);

  ret = IGMP_ERR_NONE;
  disp_flags = NULL;
  gmr_tib = NULL;
  pgrp = NULL;
  igif = NULL;
  igr = NULL;
  cli = NULL;
  pn = NULL;

  if (! (igif = (struct igmp_if *) avl_node_info)
      || ! (disp_flags = (u_int32_t *) arg1)
      || ! (cli = (struct cli *) arg3))
    goto EXIT;

  pgrp = (struct pal_in4_addr *) arg2;

  if (CHECK_FLAG ((*disp_flags), IGMP_SHOW_FLAG_DISP_PROXY_GROUP)
      && (!CHECK_FLAG (igif->igif_conf.igifc_cflags, 
                       IGMP_IF_CFLAG_PROXY_SERVICE)))
    goto EXIT;

  if (CHECK_FLAG ((*disp_flags), IGMP_SHOW_FLAG_DISP_GROUP))
    gmr_tib = igif->igif_gmr_tib;
  else 
    gmr_tib = igif->igif_hst_gmr_tib;

  for (pn = ptree_top (gmr_tib); pn; pn = ptree_next (pn))
    {
      if (! (igr = pn->info))
        continue;

      if (pgrp
          && ! IPV4_ADDR_SAME (PTREE_NODE_KEY (igr->igr_owning_pn), pgrp))
        continue;

      if (CHECK_FLAG ((*disp_flags), IGMP_SHOW_FLAG_DISP_DETAIL))
        igmp_if_show_group_detail (cli, igr, disp_flags);
      else
        igmp_if_show_group_summary (cli, igr, disp_flags);
    }

EXIT:

  IGMP_FN_EXIT (ret);
}

#ifdef HAVE_VRF
CLI (show_ip_igmp_groups,
     show_ip_igmp_groups_cmd,
     "show ip igmp (vrf NAME|) groups (detail|)",
     CLI_SHOW_STR,
     CLI_IP_STR,
     "IGMP information",
     CLI_VRF_STR,
     CLI_VRF_NAME_STR,
     "IGMP group membership information",
     "IGMPv3 source information")
#else
CLI (show_ip_igmp_groups,
     show_ip_igmp_groups_cmd,
     "show ip igmp groups (detail|)",
     CLI_SHOW_STR,
     CLI_IP_STR,
     "IGMP information",
     "IGMP group membership information",
     "IGMPv3 source information")
#endif /* HAVE_VRF */
{
  struct igmp_instance *igi;
  u_int32_t disp_flags;
  struct apn_vrf *ivrf;
  s_int32_t ret;

  IGMP_FN_ENTER (Show Groups);

  ret = IGMP_ERR_NONE;
  disp_flags = 0;
  ivrf = NULL;
  igi = NULL;

  SET_FLAG (disp_flags, IGMP_SHOW_FLAG_DISP_HEADER);
  SET_FLAG (disp_flags, IGMP_SHOW_FLAG_DISP_GROUP);

  if (argc == 1)
    SET_FLAG (disp_flags, IGMP_SHOW_FLAG_DISP_DETAIL);

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

  avl_traverse3 (igi->igi_if_tree, igmp_if_avl_traverse_show_groups,
                 (void_t *) &disp_flags, NULL, (void_t *) cli);

EXIT:

  IGMP_FN_EXIT (igmp_cli_return (cli, ret));
}

#ifdef HAVE_VRF
CLI (show_ip_igmp_groups_group,
     show_ip_igmp_groups_group_cmd,
     "show ip igmp (vrf NAME|) groups A.B.C.D (detail|)",
     CLI_SHOW_STR,
     CLI_IP_STR,
     "IGMP information",
     CLI_VRF_STR,
     CLI_VRF_NAME_STR,
     "IGMP group membership information",
     "Address of multicast group",
     "IGMPv3 source information")
#else
CLI (show_ip_igmp_groups_group,
     show_ip_igmp_groups_group_cmd,
     "show ip igmp groups A.B.C.D (detail|)",
     CLI_SHOW_STR,
     CLI_IP_STR,
     "IGMP information",
     "IGMP group membership information",
     "Address of multicast group",
     "IGMPv3 source information")
#endif /* HAVE_VRF */
{
  struct igmp_instance *igi;
  struct pal_in4_addr grp;
  u_int32_t disp_flags;
  struct apn_vrf *ivrf;
  s_int32_t ret;

  IGMP_FN_ENTER (Show Group);

  ret = IGMP_ERR_NONE;
  disp_flags = 0;
  ivrf = NULL;
  igi = NULL;

  SET_FLAG (disp_flags, IGMP_SHOW_FLAG_DISP_HEADER);
  SET_FLAG (disp_flags, IGMP_SHOW_FLAG_DISP_GROUP);

  /* Get the VRF Instance and arguments */
  if (argc > 1
      && ! pal_strcmp (argv [0], "vrf"))
    {
      ivrf = apn_vrf_lookup_by_name (LIB_GLOB_GET_VR_CONTEXT (cli->zg), 
                                     argv [1]);

      CLI_GET_IPV4_ADDRESS ("Group address", grp, argv[2]);

      if (argc == 4)
        SET_FLAG (disp_flags, IGMP_SHOW_FLAG_DISP_DETAIL);
    }
  else
    {
      ivrf = apn_vrf_lookup_by_name (LIB_GLOB_GET_VR_CONTEXT (cli->zg), NULL);

      CLI_GET_IPV4_ADDRESS ("Group address", grp, argv[0]);

      if (argc == 2)
        SET_FLAG (disp_flags, IGMP_SHOW_FLAG_DISP_DETAIL);
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

  avl_traverse3 (igi->igi_if_tree, igmp_if_avl_traverse_show_groups,
                 (void_t *) &disp_flags, (void_t *) &grp, (void_t *) cli);

EXIT:

  IGMP_FN_EXIT (igmp_cli_return (cli, ret));
}

#ifdef HAVE_VRF
CLI (show_ip_igmp_groups_interface,
     show_ip_igmp_groups_interface_cmd,
     "show ip igmp (vrf NAME|) groups IFNAME (detail|)",
     CLI_SHOW_STR,
     CLI_IP_STR,
     "IGMP information",
     CLI_VRF_STR,
     CLI_VRF_NAME_STR,
     "IGMP group membership information",
     "Interface name",
     "IGMPv3 source information")
#else
CLI (show_ip_igmp_groups_interface,
     show_ip_igmp_groups_interface_cmd,
     "show ip igmp groups IFNAME (detail|)",
     CLI_SHOW_STR,
     CLI_IP_STR,
     "IGMP information",
     "IGMP group membership information",
     "Interface name",
     "IGMPv3 source information")
#endif /* HAVE_VRF */
{
  struct igmp_group_rec *igr;
  struct igmp_if_idx igifidx;
  struct igmp_instance *igi;
  struct ptree_node *pn;
  struct interface *ifp;
  struct ptree *gmr_tib;
  struct igmp_if *igif;
  struct apn_vrf *ivrf;
  u_int32_t disp_flags;
  s_int32_t ret;

  IGMP_FN_ENTER (Show IF Groups);

  ret = IGMP_ERR_NONE;
  disp_flags = 0;
  gmr_tib = NULL;
  igif = NULL;
  ivrf = NULL;
  igr = NULL;
  igi = NULL;
  ifp = NULL;
  pn = NULL;

  SET_FLAG (disp_flags, IGMP_SHOW_FLAG_DISP_HEADER);
  SET_FLAG (disp_flags, IGMP_SHOW_FLAG_DISP_GROUP);

  if (argc == 2)
    SET_FLAG (disp_flags, IGMP_SHOW_FLAG_DISP_DETAIL);

  /* Get the VRF Instance and arguments */
  if (argc > 1
      && ! pal_strcmp (argv [0], "vrf"))
    {
      ivrf = apn_vrf_lookup_by_name (LIB_GLOB_GET_VR_CONTEXT (cli->zg), 
                                     argv [1]);

      if (! ivrf)
        {
          ret = IGMP_ERR_NO_CONTEXT_INFO;
          goto EXIT;
        }

      ifp = ifv_lookup_by_name (LIB_VRF_GET_IF_MASTER (ivrf), argv [2]);

      if (argc == 4)
        SET_FLAG (disp_flags, IGMP_SHOW_FLAG_DISP_DETAIL);
    }
  else
    {
      ivrf = apn_vrf_lookup_by_name (LIB_GLOB_GET_VR_CONTEXT (cli->zg), NULL);

      if (! ivrf)
        {
          ret = IGMP_ERR_NO_CONTEXT_INFO;
          goto EXIT;
        }

      ifp = ifv_lookup_by_name (LIB_VRF_GET_IF_MASTER (ivrf), argv [0]);

      if (argc == 2)
        SET_FLAG (disp_flags, IGMP_SHOW_FLAG_DISP_DETAIL);
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

  igifidx.igifidx_ifp = ifp;
  igifidx.igifidx_sid = igmp_if_svc_reg_su_id (igi, ifp);

  igif = igmp_if_lookup (igi, &igifidx);

  if (! igif)
    {
      ret = IGMP_ERR_NO_SUCH_IFF;
      goto EXIT;
    }

  if (if_is_vlan (ifp))
    gmr_tib = igif->igif_hst_gmr_tib;
  else
    gmr_tib = igif->igif_gmr_tib;    
    
  for (pn = ptree_top (gmr_tib); pn; pn = ptree_next (pn))
    {
      if (! (igr = pn->info))
        continue;

      if (CHECK_FLAG (disp_flags, IGMP_SHOW_FLAG_DISP_DETAIL))
        igmp_if_show_group_detail (cli, igr, &disp_flags);
      else
        igmp_if_show_group_summary (cli, igr, &disp_flags);
    }

EXIT:

  IGMP_FN_EXIT (igmp_cli_return (cli, ret));
}

#ifdef HAVE_VRF
CLI (show_ip_igmp_groups_interface_group,
     show_ip_igmp_groups_interface_group_cmd,
     "show ip igmp (vrf NAME|) groups IFNAME A.B.C.D (detail|)",
     CLI_SHOW_STR,
     CLI_IP_STR,
     "IGMP information",
     CLI_VRF_STR,
     CLI_VRF_NAME_STR,
     "IGMP group membership information",
     "Interface name",
     "Multicast Group Address",
     "IGMPv3 source information")
#else
CLI (show_ip_igmp_groups_interface_group,
     show_ip_igmp_groups_interface_group_cmd,
     "show ip igmp groups IFNAME A.B.C.D (detail|)",
     CLI_SHOW_STR,
     CLI_IP_STR,
     "IGMP information",
     "IGMP group membership information",
     "Interface name",
     "Multicast Group Address",
     "IGMPv3 source information")
#endif /* HAVE_VRF */
{
  struct igmp_group_rec *igr;
  struct igmp_if_idx igifidx;
  struct igmp_instance *igi;
  struct pal_in4_addr grp;
  struct interface *ifp;
  struct igmp_if *igif;
  struct apn_vrf *ivrf;
  u_int32_t disp_flags;
  s_int32_t ret;

  IGMP_FN_ENTER (Show IF Group);

  ret = IGMP_ERR_NONE;
  disp_flags = 0;
  igif = NULL;
  ivrf = NULL;
  igr = NULL;
  igi = NULL;
  ifp = NULL;

  SET_FLAG (disp_flags, IGMP_SHOW_FLAG_DISP_HEADER);
  SET_FLAG (disp_flags, IGMP_SHOW_FLAG_DISP_GROUP);

  if (argc == 3)
    SET_FLAG (disp_flags, IGMP_SHOW_FLAG_DISP_DETAIL);

  /* Get the VRF Instance and arguments */
  if (argc > 1
      && ! pal_strcmp (argv [0], "vrf"))
    {
      ivrf = apn_vrf_lookup_by_name (LIB_GLOB_GET_VR_CONTEXT (cli->zg), 
                                     argv [1]);

      if (! ivrf)
        {
          ret = IGMP_ERR_NO_CONTEXT_INFO;
          goto EXIT;
        }

      ifp = ifv_lookup_by_name (LIB_VRF_GET_IF_MASTER (ivrf), argv [2]);

      CLI_GET_IPV4_ADDRESS ("Group address", grp, argv[3]);

      if (argc == 5)
        SET_FLAG (disp_flags, IGMP_SHOW_FLAG_DISP_DETAIL);
    }
  else
    {
      ivrf = apn_vrf_lookup_by_name (LIB_GLOB_GET_VR_CONTEXT (cli->zg), NULL);
  
      if (! ivrf)
        {
          ret = IGMP_ERR_NO_CONTEXT_INFO;
          goto EXIT;
        }

      ifp = ifv_lookup_by_name (LIB_VRF_GET_IF_MASTER (ivrf), argv [0]);

      CLI_GET_IPV4_ADDRESS ("Group address", grp, argv[1]);

      if (argc == 3)
        SET_FLAG (disp_flags, IGMP_SHOW_FLAG_DISP_DETAIL);
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

  igifidx.igifidx_ifp = ifp;
  igifidx.igifidx_sid = igmp_if_svc_reg_su_id (igi, ifp);

  igif = igmp_if_lookup (igi, &igifidx);

  if (! igif)
    {
      ret = IGMP_ERR_NO_SUCH_IFF;
      goto EXIT;
    }

  igr = igmp_if_grec_lookup (igif, &grp);

  if (! igr)
    {
      ret = IGMP_ERR_NO_SUCH_GROUP_REC;
      goto EXIT;
    }

  if (CHECK_FLAG (disp_flags, IGMP_SHOW_FLAG_DISP_DETAIL))
    igmp_if_show_group_detail (cli, igr, &disp_flags);
  else
    igmp_if_show_group_summary (cli, igr, &disp_flags);

EXIT:

  IGMP_FN_EXIT (igmp_cli_return (cli, ret));
}

#ifdef HAVE_VRF
CLI (show_ip_igmp_proxy_groups,
     show_ip_igmp_proxy_groups_cmd,
     "show ip igmp (vrf NAME|) proxy groups (detail|)",
     CLI_SHOW_STR,
     CLI_IP_STR,
     "IGMP information",
     CLI_VRF_STR,
     CLI_VRF_NAME_STR,
     "Proxy information",
     "IGMP proxy group membership information",
     "IGMPv3 source information")
#else
CLI (show_ip_igmp_proxy_groups,
     show_ip_igmp_proxy_groups_cmd,
     "show ip igmp proxy groups (detail|)",
     CLI_SHOW_STR,
     CLI_IP_STR,
     "IGMP information",
     "Proxy information",
     "IGMP proxy group membership information",
     "IGMPv3 source information")
#endif /* HAVE_VRF */
{
  struct igmp_instance *igi;
  u_int32_t disp_flags;
  struct apn_vrf *ivrf;
  s_int32_t ret;

  IGMP_FN_ENTER (Show Proxy Groups);
  
  ret = IGMP_ERR_NONE;
  disp_flags = 0;
  ivrf = NULL;
  igi = NULL;

  SET_FLAG (disp_flags, IGMP_SHOW_FLAG_DISP_PROXY_HEADER);
  SET_FLAG (disp_flags, IGMP_SHOW_FLAG_DISP_PROXY_GROUP);

  if (argc == 1)
    SET_FLAG (disp_flags, IGMP_SHOW_FLAG_DISP_DETAIL);

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

  avl_traverse3 (igi->igi_if_tree, igmp_if_avl_traverse_show_groups,
                 (void_t *) &disp_flags, NULL, (void_t *) cli);

EXIT:

  IGMP_FN_EXIT (igmp_cli_return (cli, ret));
}

#ifdef HAVE_VRF
CLI (show_ip_igmp_proxy_groups_group,
     show_ip_igmp_proxy_groups_group_cmd,
     "show ip igmp (vrf NAME|) proxy groups A.B.C.D (detail|)",
     CLI_SHOW_STR,
     CLI_IP_STR,
     "IGMP information",
     CLI_VRF_STR,
     CLI_VRF_NAME_STR,
     "Proxy information",
     "IGMP proxy group membership information",
     "Address of multicast group",
     "IGMPv3 source information")
#else
CLI (show_ip_igmp_proxy_groups_group,
     show_ip_igmp_proxy_groups_group_cmd,
     "show ip igmp proxy groups A.B.C.D (detail|)",
     CLI_SHOW_STR,
     CLI_IP_STR,
     "IGMP information",
     "Proxy information",
     "IGMP proxy group membership information",
     "Address of multicast group",
     "IGMPv3 source information")
#endif /* HAVE_VRF */
{
  struct igmp_instance *igi;
  struct pal_in4_addr grp;
  u_int32_t disp_flags;
  struct apn_vrf *ivrf;
  s_int32_t ret;

  IGMP_FN_ENTER (Show Proxy Group);

  ret = IGMP_ERR_NONE;
  disp_flags = 0;
  ivrf = NULL;
  igi = NULL;

  SET_FLAG (disp_flags, IGMP_SHOW_FLAG_DISP_PROXY_HEADER);
  SET_FLAG (disp_flags, IGMP_SHOW_FLAG_DISP_PROXY_GROUP);

  /* Get the VRF Instance and arguments */
  if (argc > 1
      && ! pal_strcmp (argv [0], "vrf"))
    {
      ivrf = apn_vrf_lookup_by_name (LIB_GLOB_GET_VR_CONTEXT (cli->zg), 
                                     argv [1]);

      CLI_GET_IPV4_ADDRESS ("Group address", grp, argv[2]);

      if (argc == 4)
        SET_FLAG (disp_flags, IGMP_SHOW_FLAG_DISP_DETAIL);
    }
  else
    {
      ivrf = apn_vrf_lookup_by_name (LIB_GLOB_GET_VR_CONTEXT (cli->zg), NULL);

      CLI_GET_IPV4_ADDRESS ("Group address", grp, argv[0]);

      if (argc == 2)
        SET_FLAG (disp_flags, IGMP_SHOW_FLAG_DISP_DETAIL);
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

  avl_traverse3 (igi->igi_if_tree, igmp_if_avl_traverse_show_groups,
                 (void_t *) &disp_flags, (void_t *) &grp, (void_t *) cli);

EXIT:

  IGMP_FN_EXIT (igmp_cli_return (cli, ret));
}

#ifdef HAVE_VRF
CLI (show_ip_igmp_proxy_groups_interface,
     show_ip_igmp_proxy_groups_interface_cmd,
     "show ip igmp (vrf NAME|) proxy groups IFNAME (detail|)",
     CLI_SHOW_STR,
     CLI_IP_STR,
     "IGMP information",
     CLI_VRF_STR,
     CLI_VRF_NAME_STR,
     "Proxy information",
     "IGMP proxy group membership information",
     "Interface name",
     "IGMPv3 source information")
#else
CLI (show_ip_igmp_proxy_groups_interface,
     show_ip_igmp_proxy_groups_interface_cmd,
     "show ip igmp proxy groups IFNAME (detail|)",
     CLI_SHOW_STR,
     CLI_IP_STR,
     "IGMP information",
     "Proxy information",
     "IGMP proxy group membership information",
     "Interface name",
     "IGMPv3 source information")
#endif /* HAVE_VRF */
{
  struct igmp_group_rec *igr;
  struct igmp_if_idx igifidx;
  struct igmp_instance *igi;
  struct ptree_node *pn;
  struct interface *ifp;
  struct ptree *gmr_tib;
  struct igmp_if *igif;
  struct apn_vrf *ivrf;
  u_int32_t disp_flags;
  s_int32_t ret;

  IGMP_FN_ENTER (Show Proxy IF Groups);

  ret = IGMP_ERR_NONE;
  disp_flags = 0;
  gmr_tib = NULL;
  igif = NULL;
  ivrf = NULL;
  igr = NULL;
  igi = NULL;
  ifp = NULL;
  pn = NULL;

  SET_FLAG (disp_flags, IGMP_SHOW_FLAG_DISP_PROXY_HEADER);
  SET_FLAG (disp_flags, IGMP_SHOW_FLAG_DISP_PROXY_GROUP);

  if (argc == 2)
    SET_FLAG (disp_flags, IGMP_SHOW_FLAG_DISP_DETAIL);

  /* Get the VRF Instance and arguments */
  if (argc > 1
      && ! pal_strcmp (argv [0], "vrf"))
    {
      ivrf = apn_vrf_lookup_by_name (LIB_GLOB_GET_VR_CONTEXT (cli->zg), 
                                     argv [1]);

      if (! ivrf)
        {
          ret = IGMP_ERR_NO_CONTEXT_INFO;
          goto EXIT;
        }

      ifp = ifv_lookup_by_name (LIB_VRF_GET_IF_MASTER (ivrf), argv [2]);
      if (! ifp)
       {
         ret = IGMP_ERR_NO_SUCH_IFF;
         goto EXIT;
       }
      
      if (argc == 4)
        SET_FLAG (disp_flags, IGMP_SHOW_FLAG_DISP_DETAIL);
    }
  else
    {
      ivrf = apn_vrf_lookup_by_name (LIB_GLOB_GET_VR_CONTEXT (cli->zg), NULL);

      if (! ivrf)
        {
          ret = IGMP_ERR_NO_CONTEXT_INFO;
          goto EXIT;
        }

      ifp = ifv_lookup_by_name (LIB_VRF_GET_IF_MASTER (ivrf), argv [0]);

      if (argc == 2)
        SET_FLAG (disp_flags, IGMP_SHOW_FLAG_DISP_DETAIL);
    }

  /* Get the IGMP Instance */
  igi = LIB_VRF_GET_IGMP_INSTANCE (ivrf);

  if (! igi)
    {
      ret = IGMP_ERR_NO_CONTEXT_INFO;
      goto EXIT;
    }

  ifp = ifv_lookup_by_name (LIB_VRF_GET_IF_MASTER (ivrf), argv [0]);

  if (! ifp)
    {
      ret = IGMP_ERR_NO_SUCH_IFF;
      goto EXIT;
    }

  igifidx.igifidx_ifp = ifp;
  igifidx.igifidx_sid = igmp_if_svc_reg_su_id (igi, ifp);

  igif = igmp_if_lookup (igi, &igifidx);

  if (! igif)
    {
      ret = IGMP_ERR_NO_SUCH_IFF;
      goto EXIT;
    }
  
  if (! CHECK_FLAG (igif->igif_conf.igifc_cflags, IGMP_IF_CFLAG_PROXY_SERVICE))
    {
      ret = IGMP_ERR_NO_PROXY_SERVICE;
      goto EXIT;
    }

  for (pn = ptree_top (igif->igif_hst_gmr_tib); pn; pn = ptree_next (pn))
    {
      if (! (igr = pn->info))
        continue;

      if (CHECK_FLAG (disp_flags, IGMP_SHOW_FLAG_DISP_DETAIL))
        igmp_if_show_group_detail (cli, igr, &disp_flags);
      else
        igmp_if_show_group_summary (cli, igr, &disp_flags);
    }

EXIT:

  IGMP_FN_EXIT (igmp_cli_return (cli, ret));
}

#ifdef HAVE_VRF
CLI (show_ip_igmp_proxy_groups_interface_group,
     show_ip_igmp_proxy_groups_interface_group_cmd,
     "show ip igmp (vrf NAME|) proxy groups IFNAME A.B.C.D (detail|)",
     CLI_SHOW_STR,
     CLI_IP_STR,
     "IGMP information",
     CLI_VRF_STR,
     CLI_VRF_NAME_STR,
     "Proxy information",
     "IGMP proxy group membership information",
     "Interface name",
     "Multicast Group Address",
     "IGMPv3 source information")
#else
CLI (show_ip_igmp_proxy_groups_interface_group,
     show_ip_igmp_proxy_groups_interface_group_cmd,
     "show ip igmp proxy groups IFNAME A.B.C.D (detail|)",
     CLI_SHOW_STR,
     CLI_IP_STR,
     "IGMP information",
     "Proxy information",
     "IGMP proxy group membership information",
     "Interface name",
     "Multicast Group Address",
     "IGMPv3 source information")
#endif /* HAVE_VRF */
{
  struct igmp_group_rec *igr;
  struct igmp_if_idx igifidx;
  struct igmp_instance *igi;
  struct pal_in4_addr grp;
  struct interface *ifp;
  struct igmp_if *igif;
  struct apn_vrf *ivrf;
  u_int32_t disp_flags;
  s_int32_t ret;

  IGMP_FN_ENTER (Show Proxy IF Group);

  ret = IGMP_ERR_NONE;
  disp_flags = 0;
  igif = NULL;
  ivrf = NULL;
  igr = NULL;
  igi = NULL;
  ifp = NULL;

  SET_FLAG (disp_flags, IGMP_SHOW_FLAG_DISP_PROXY_HEADER);
  SET_FLAG (disp_flags, IGMP_SHOW_FLAG_DISP_PROXY_GROUP);

  if (argc == 3)
    SET_FLAG (disp_flags, IGMP_SHOW_FLAG_DISP_DETAIL);

  /* Get the VRF Instance and arguments */
  if (argc > 1
      && ! pal_strcmp (argv [0], "vrf"))
    {
      ivrf = apn_vrf_lookup_by_name (LIB_GLOB_GET_VR_CONTEXT (cli->zg), 
                                     argv [1]);

      if (! ivrf)
        {
          ret = IGMP_ERR_NO_CONTEXT_INFO;
          goto EXIT;
        }

      ifp = ifv_lookup_by_name (LIB_VRF_GET_IF_MASTER (ivrf), argv [2]);


      CLI_GET_IPV4_ADDRESS ("Group address", grp, argv[3]);

      if (argc == 5)
        SET_FLAG (disp_flags, IGMP_SHOW_FLAG_DISP_DETAIL);
    }
  else
    {
      ivrf = apn_vrf_lookup_by_name (LIB_GLOB_GET_VR_CONTEXT (cli->zg), NULL);

      if (! ivrf)
        {
          ret = IGMP_ERR_NO_CONTEXT_INFO;
          goto EXIT;
        }

      ifp = ifv_lookup_by_name (LIB_VRF_GET_IF_MASTER (ivrf), argv [0]);

      CLI_GET_IPV4_ADDRESS ("Group address", grp, argv[1]);

      if (argc == 3)
        SET_FLAG (disp_flags, IGMP_SHOW_FLAG_DISP_DETAIL);
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

  igifidx.igifidx_ifp = ifp;
  igifidx.igifidx_sid = igmp_if_svc_reg_su_id (igi, ifp);

  igif = igmp_if_lookup (igi, &igifidx);

  if (! igif)
    {
      ret = IGMP_ERR_NO_SUCH_IFF;
      goto EXIT;
    }

  if (! CHECK_FLAG (igif->igif_conf.igifc_cflags, IGMP_IF_CFLAG_PROXY_SERVICE))
    {
      ret = IGMP_ERR_NO_PROXY_SERVICE;
      goto EXIT;
    }
  
  igr = igmp_if_hst_grec_lookup (igif, &grp);

  if (! igr)
    {
      ret = IGMP_ERR_NO_SUCH_GROUP_REC;
      goto EXIT;
    }

  if (CHECK_FLAG (disp_flags, IGMP_SHOW_FLAG_DISP_DETAIL))
    igmp_if_show_group_detail (cli, igr, &disp_flags);
  else
    igmp_if_show_group_summary (cli, igr, &disp_flags);

EXIT:

  IGMP_FN_EXIT (igmp_cli_return (cli, ret));
}

s_int32_t
igmp_if_show_interface (struct igmp_if *igif,
                        struct cli *cli,
                        u_int32_t *disp_flags)
{
  struct igmp_if_idx *mrtr_igifidx;
  struct igmp_instance *igi;
  struct ptree_node *p_node;
  struct igmp_if *hst_igif;
  struct avl_node *node;
  s_int32_t ret;

  IGMP_FN_ENTER (IF Info);

  mrtr_igifidx = NULL;
  ret = IGMP_ERR_NONE;
  hst_igif = NULL;
  p_node = NULL;
  node = NULL;
  igi = NULL;

  if (! igif
      || ! (igi = igif->igif_owning_igi))
    {
      ret = IGMP_ERR_NO_SUCH_IFF;
      goto EXIT;
    }
  if (CHECK_FLAG ((*disp_flags), IGMP_SHOW_FLAG_DISP_PROXY))
    {
      if (CHECK_FLAG (igif->igif_conf.igifc_cflags,
                      IGMP_IF_CFLAG_MROUTE_PROXY))
        {
          cli_out(cli, "\n");
          if (! CHECK_FLAG (igif->igif_sflags, IGMP_IF_SFLAG_SVC_TYPE_L3))
            cli_out (cli, "Interface %s (Index %u) (Vlan %u)\n",
                     igif->igif_owning_ifp->name,
                     igif->igif_owning_ifp->ifindex,
                     IGMP_IF_GET_VID (igif->igif_su_id));
          else
            cli_out (cli, "Interface %s (Index %u)\n",
                     igif->igif_owning_ifp->name,
                     igif->igif_owning_ifp->ifindex);
          cli_out (cli, "Administrative status: enabled");
          cli_out (cli, "\n");
          cli_out (cli, "Operational status: ");
          if (LIST_ISEMPTY (&igif->igif_conf.igifc_mrouter_if_lst))
            {
              ret = IGMP_ERR_NONE;
              goto EXIT;
            }
          else
            {
              mrtr_igifidx = GETDATA (LISTHEAD
                             (&igif->igif_conf.igifc_mrouter_if_lst));
              if (! mrtr_igifidx)
                {
                  IGMP_DBG_INFO (igi, EVENTS, "%s Mrouter-proxy IF INVALID!",
                                 igif->igif_owning_ifp->name);
                  ret = IGMP_ERR_NONE;
                  goto EXIT;
                }
              hst_igif = igmp_if_lookup (igi, mrtr_igifidx);

              if (!hst_igif )
                {
                  IGMP_DBG_WARN (igi, EVENTS,
                                 "Host-side interface for %s NOT FOUND!",
                                 igif->igif_owning_ifp->name);
                  ret = IGMP_ERR_NONE;
                  cli_out(cli, "down\n");
                }
              else
                {
                  if (CHECK_FLAG (hst_igif->igif_conf.igifc_cflags,
                                  IGMP_IF_CFLAG_PROXY_SERVICE))
                    cli_out(cli, "up\n");
                  else
                    cli_out(cli, "down\n");
                }
              cli_out (cli, "Upstream interface is %s\n",
                       mrtr_igifidx->igifidx_ifp->name);
            }
          cli_out(cli, "Number of multicast groups: %d\n",
                  igif->igif_num_grecs );
        }
    }
  else if (CHECK_FLAG ((*disp_flags), IGMP_SHOW_FLAG_DISP_INTERFACE))
    {
      cli_out(cli, "\n");
      if (! CHECK_FLAG (igif->igif_sflags, IGMP_IF_SFLAG_SVC_TYPE_L3))
        cli_out (cli, "Interface %s (Index %u) (Vlan %u)\n",
                 igif->igif_owning_ifp->name,
                 igif->igif_owning_ifp->ifindex,
                 IGMP_IF_GET_VID (igif->igif_su_id));
      else
        cli_out (cli, "Interface %s (Index %u)\n",
                 igif->igif_owning_ifp->name,
                 igif->igif_owning_ifp->ifindex);
      if (igif->igif_conf.igifc_cflags)
        cli_out (cli, " IGMP Enabled,");
      else
        cli_out (cli, " IGMP");
      if (CHECK_FLAG (igif->igif_sflags, IGMP_IF_SFLAG_ACTIVE))
        {
          cli_out (cli, " Active,");
          if (CHECK_FLAG (igif->igif_sflags, IGMP_IF_SFLAG_QUERIER))
            cli_out (cli, " Querier,");
          else
            cli_out (cli, " Non-Querier,");
        }
      else
        cli_out (cli, " Inactive,");
      if (CHECK_FLAG (igif->igif_conf.igifc_cflags,
                      IGMP_IF_CFLAG_VERSION))
        cli_out (cli, " Configured for version %u",
                 igif->igif_conf.igifc_version);
      else
        cli_out (cli, " Version %u (default)",
                 igif->igif_conf.igifc_version);
      if (CHECK_FLAG (igif->igif_conf.igifc_cflags,
                      IGMP_IF_CFLAG_PROXY_SERVICE))
        cli_out (cli, " proxy-service");
      cli_out (cli, "\n");
      if (CHECK_FLAG (igif->igif_conf.igifc_cflags,
                      IGMP_IF_CFLAG_PROXY_SERVICE)
          && CHECK_FLAG (igif->igif_sflags,
                         (IGMP_IF_SFLAG_HOST_COMPAT_V1
                          | IGMP_IF_SFLAG_HOST_COMPAT_V2
                          | IGMP_IF_SFLAG_HOST_COMPAT_V3)))
        cli_out (cli, " IGMP host version %u\n",
                 (CHECK_FLAG (igif->igif_sflags,
                              IGMP_IF_SFLAG_HOST_COMPAT_V1) ?
                  IGMP_VERSION_1 :
                  CHECK_FLAG (igif->igif_sflags,
                              IGMP_IF_SFLAG_HOST_COMPAT_V2) ?
                  IGMP_VERSION_2 : IGMP_VERSION_3));
      if (CHECK_FLAG (igif->igif_conf.igifc_cflags,
                      IGMP_IF_CFLAG_MROUTE_PROXY)
          && ! LIST_ISEMPTY (&igif->igif_conf.igifc_mrouter_if_lst))
        {
          mrtr_igifidx = GETDATA (LISTHEAD
                         (&igif->igif_conf.igifc_mrouter_if_lst));
          if (mrtr_igifidx)
          cli_out (cli, " IGMP mroute-proxy interface is %s\n",
                   mrtr_igifidx->igifidx_ifp->name);
        }
      if (igif->igif_paddr)
        cli_out (cli, " Internet address is %r\n",
                 &igif->igif_paddr->u.prefix4);
      if (CHECK_FLAG (igi->igi_cflags, IGMP_INST_CFLAG_LIMIT_GREC))
        {
          cli_out (cli, " IGMP global limit is %u", igi->igi_limit);

          if (igi->igi_limit_except_alist)
            cli_out (cli, ", exception access-list %s",
                     igi->igi_limit_except_alist);

          cli_out (cli, "\n");

          cli_out (cli, " IGMP global limit states count is"
                   " currently %u\n", igi->igi_num_grecs);
        }
      if (CHECK_FLAG (igif->igif_conf.igifc_cflags,
                      IGMP_IF_CFLAG_LIMIT_GREC))
        {
          cli_out (cli, " IGMP interface limit is %u",
                   igif->igif_conf.igifc_limit);

          if (igif->igif_conf.igifc_limit_except_alist)
            cli_out (cli, ", exception access-list %s",
                     igif->igif_conf.igifc_limit_except_alist);

          cli_out (cli, "\n");
        }
      cli_out (cli, " IGMP interface has %u group-record states\n",
               igif->igif_num_grecs);
      cli_out (cli, " IGMP activity: %u joins, %u leaves\n",
               igif->igif_num_joins, igif->igif_num_leaves);
      if (CHECK_FLAG (igif->igif_sflags, IGMP_IF_SFLAG_ACTIVE)
          && ! CHECK_FLAG (igif->igif_sflags, IGMP_IF_SFLAG_QUERIER))
        cli_out (cli, " IGMP querying router is %r\n",
                 &igif->igif_other_querier_addr);
      if (CHECK_FLAG (igif->igif_sflags, IGMP_IF_SFLAG_ACTIVE))
        cli_out (cli, " IGMP query interval is %d seconds\n",
                 igif->igif_qi);
      else
        cli_out (cli, " IGMP query interval is %d seconds\n",
                 igif->igif_conf.igifc_qi);
      cli_out (cli, " IGMP Startup query interval is %d seconds\n",
               igif->igif_conf.igifc_sqi);
      cli_out (cli, " IGMP Startup query count is %d \n",
               igif->igif_conf.igifc_sqc);

      cli_out (cli, " IGMP querier timeout is %d seconds\n",
               igif->igif_oqi);
      cli_out (cli, " IGMP max query response time is %d seconds\n",
               igif->igif_conf.igifc_qri);
      cli_out (cli, " Last member query response interval is %d milliseconds\n",
               igif->igif_conf.igifc_lmqi);
      cli_out (cli, " Group Membership interval is %d seconds\n",
               igif->igif_gmi);
      cli_out (cli, " IGMP Last member query count is %d \n",
               igif->igif_conf.igifc_lmqc);

      if (CHECK_FLAG (igif->igif_sflags, IGMP_IF_SFLAG_SVC_TYPE_L2))
        {
          cli_out (cli, " IGMP Snooping is globally %s\n",
                   CHECK_FLAG (igif->igif_owning_igi->igi_cflags,
                               IGMP_INST_CFLAG_SNOOP_DISABLED) ?
                   "disabled" : "enabled");

          cli_out (cli, " IGMP Snooping is %senabled on this interface\n",
                   (CHECK_FLAG (igif->igif_sflags,
                                IGMP_IF_SFLAG_ACTIVE) ? "" : "not "));

          cli_out (cli, " IGMP Snooping fast-leave is %senabled\n",
                   CHECK_FLAG (igif->igif_conf.igifc_cflags,
                               IGMP_IF_CFLAG_SNOOP_FAST_LEAVE) ?
                   "" : "not ");

          cli_out (cli, " IGMP Snooping querier is %senabled\n",
                   CHECK_FLAG (igif->igif_conf.igifc_cflags,
                               IGMP_IF_CFLAG_SNOOP_QUERIER) ?
                   "" : "not ");

          cli_out (cli, " IGMP Snooping report suppression is %s\n",
                   CHECK_FLAG (igif->igif_conf.igifc_cflags,
                               IGMP_IF_CFLAG_SNOOP_NO_REPORT_SUPPRESS) ?
                   "disabled" : "enabled");
        }
    }

EXIT:

  IGMP_FN_EXIT (ret);
}

static s_int32_t
igmp_if_avl_traverse_show_interface (void_t *avl_node_info, 
                                     void_t *arg1,
                                     void_t *arg2)
{
  u_int32_t *disp_flags;
  struct igmp_if *igif;
  struct cli *cli;
  s_int32_t ret;

  IGMP_FN_ENTER (IF Hash Iterate Show);

  disp_flags = NULL;
  igif = NULL;
  cli = NULL;
  ret = -1;

  if (! (igif = (struct igmp_if *) avl_node_info)
      || ! (disp_flags = (u_int32_t *) arg1)
      || ! (cli = (struct cli *) arg2))
    goto EXIT;

  igmp_if_show_interface (igif, cli, disp_flags);

  ret = 0;

EXIT:

  IGMP_FN_EXIT (ret);
}

#ifdef HAVE_VRF
CLI (show_ip_igmp_interface_name,
     show_ip_igmp_interface_name_cmd,
     "show ip igmp (vrf NAME|) interface (IFNAME|)",
     CLI_SHOW_STR,
     CLI_IP_STR,
     "IGMP information",
     CLI_VRF_STR,
     CLI_VRF_NAME_STR,
     "IGMP interface information",
     "Interface Name")
#else
CLI (show_ip_igmp_interface_name,
     show_ip_igmp_interface_name_cmd,
     "show ip igmp interface (IFNAME|)",
     CLI_SHOW_STR,
     CLI_IP_STR,
     "IGMP information",
     "IGMP interface information",
     "Interface Name")
#endif /* HAVE_VRF */
{
  struct igmp_if_idx igifidx;
  struct igmp_instance *igi;
  struct interface *ifp;
  s_int32_t disp_flags;  
  struct igmp_if *igif;
  struct apn_vrf *ivrf;
  u_int8_t *ifname;
  s_int32_t ret;

  IGMP_FN_ENTER (Show IF);

  ret = IGMP_ERR_NONE;
  disp_flags = 0;
  ifname = NULL;
  igif = NULL;
  ivrf = NULL;
  igi = NULL;
  ifp = NULL;

  SET_FLAG (disp_flags, IGMP_SHOW_FLAG_DISP_INTERFACE);

  /* Get the VRF Instance */
  if (argc > 1
      && ! pal_strcmp (argv [0], "vrf"))
    {
      ivrf = apn_vrf_lookup_by_name (LIB_GLOB_GET_VR_CONTEXT (cli->zg), 
                                     argv [1]);

      if (argc > 2)
        ifname = argv [2];
    }
  else
    {
      ivrf = apn_vrf_lookup_by_name (LIB_GLOB_GET_VR_CONTEXT (cli->zg), NULL);

      if (argc > 0)
        ifname = argv [0];
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

  if (ifname)
    {
      ifp = ifv_lookup_by_name (LIB_VRF_GET_IF_MASTER (ivrf), ifname);

      if (! ifp)
        {
          ret = IGMP_ERR_NO_SUCH_IFF;
          goto EXIT;
        }

      igifidx.igifidx_ifp = ifp;
      igifidx.igifidx_sid = igmp_if_svc_reg_su_id (igi, ifp);

      igif = igmp_if_lookup (igi, &igifidx);

      if (! igif)
        {
          ret = IGMP_ERR_NO_SUCH_IFF;
          goto EXIT;
        }

      ret = igmp_if_show_interface (igif, cli, &disp_flags);
    }
  else
    {
      avl_traverse2 (igi->igi_if_tree, igmp_if_avl_traverse_show_interface, 
                     (void_t *) &disp_flags, cli);
    }

EXIT:

  IGMP_FN_EXIT (igmp_cli_return (cli, ret));
}

#ifdef HAVE_VRF
CLI (show_ip_igmp_proxy,
     show_ip_igmp_proxy_cmd,
     "show ip igmp (vrf NAME|)proxy ",
     CLI_SHOW_STR,
     CLI_IP_STR,
     "IGMP information",
     CLI_VRF_STR,
     CLI_VRF_NAME_STR,
     "Proxy information")
#else
CLI (show_ip_igmp_proxy,
     show_ip_igmp_proxy_cmd,
     "show ip igmp proxy ",
     CLI_SHOW_STR,
     CLI_IP_STR,
     "IGMP information",
     "Proxy information")
#endif /* HAVE_VRF */
{
  struct igmp_instance *igi;
  struct apn_vrf *ivrf;
  s_int32_t disp_flags;
  u_int8_t *ifname;
  s_int32_t ret;

  IGMP_FN_ENTER (Show IF);

  ret = IGMP_ERR_NONE;
  disp_flags = 0;
  ifname = NULL;
  ivrf = NULL;
  igi = NULL;

  SET_FLAG (disp_flags, IGMP_SHOW_FLAG_DISP_PROXY);

  /* Get the VRF Instance */
  if (argc > 1
      && ! pal_strcmp (argv [0], "vrf"))
    {
      ivrf = apn_vrf_lookup_by_name (LIB_GLOB_GET_VR_CONTEXT (cli->zg), 
                                     argv [1]);

    }
  else
    {
      ivrf = apn_vrf_lookup_by_name (LIB_GLOB_GET_VR_CONTEXT (cli->zg), NULL);

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

  avl_traverse2 (igi->igi_if_tree, igmp_if_avl_traverse_show_interface, 
                 (void_t *) &disp_flags, cli);

EXIT:

  IGMP_FN_EXIT (igmp_cli_return (cli, ret));
}

#ifdef HAVE_VRF
CLI (show_ip_igmp_snoop_mrouter,
     show_ip_igmp_snoop_mrouter_cmd,
     "show ip igmp (vrf NAME|) snooping mrouter IFNAME",
     CLI_SHOW_STR,
     CLI_IP_STR,
     "IGMP information",
     CLI_VRF_STR,
     CLI_VRF_NAME_STR,
     "IGMP Snooping information",
     "IGMP Snooping Mrouter information",
     "VLAN Interface Name")
#else
CLI (show_ip_igmp_snoop_mrouter,
     show_ip_igmp_snoop_mrouter_cmd,
     "show ip igmp snooping mrouter IFNAME",
     CLI_SHOW_STR,
     CLI_IP_STR,
     "IGMP information",
     "IGMP Snooping information",
     "IGMP Snooping Mrouter information",
     "VLAN Interface Name")
#endif /* HAVE_VRF */
{
  struct igmp_if_idx igifidx;
  struct igmp_instance *igi;
  struct igmp_if *ds_igif;
  struct interface *ifp;
  struct igmp_if *igif;
  u_int32_t disp_flags;
  struct apn_vrf *ivrf;
  struct listnode *nn;
  s_int32_t ret;

  IGMP_FN_ENTER (Show IF Snooping Mrouter);

  ret = IGMP_ERR_NONE;
  disp_flags = 0;
  ds_igif = NULL;
  igif = NULL;
  ivrf = NULL;
  igi = NULL;
  ifp = NULL;
  nn = NULL;

  SET_FLAG (disp_flags, IGMP_SHOW_FLAG_DISP_HEADER);

  /* Get the VRF Instance */
  if (argc > 2
      && ! pal_strcmp (argv [0], "vrf"))
    {
      ivrf = apn_vrf_lookup_by_name (LIB_GLOB_GET_VR_CONTEXT (cli->zg), 
                                     argv [1]);

      if (! ivrf)
        {
          ret = IGMP_ERR_NO_CONTEXT_INFO;
          goto EXIT;
        }

      ifp = ifv_lookup_by_name (LIB_VRF_GET_IF_MASTER (ivrf), argv [2]);
    }
  else
    {
      ivrf = apn_vrf_lookup_by_name (LIB_GLOB_GET_VR_CONTEXT (cli->zg), NULL);

      if (! ivrf)
        {
          ret = IGMP_ERR_NO_CONTEXT_INFO;
          goto EXIT;
        }

      ifp = ifv_lookup_by_name (LIB_VRF_GET_IF_MASTER (ivrf), argv [0]);
    }

  /* Get the IGMP Instance */
  igi = LIB_VRF_GET_IGMP_INSTANCE (ivrf);

  if (! igi)
    {
      ret = IGMP_ERR_NO_CONTEXT_INFO;
      goto EXIT;
    }

  if (! ifp || ! if_is_vlan (ifp))
    {
      ret = IGMP_ERR_NO_SUCH_IFF;
      goto EXIT;
    }

  igifidx.igifidx_ifp = ifp;
  igifidx.igifidx_sid = igmp_if_svc_reg_su_id (igi, ifp);

  igif = igmp_if_lookup (igi, &igifidx);

  if (! igif)
    {
      ret = IGMP_ERR_NO_SUCH_IFF;
      goto EXIT;
    }

  cli_out (cli, "VLAN    Interface\n");

  LIST_LOOP (&igif->igif_hst_dsif_lst, ds_igif, nn)
    if (CHECK_FLAG (ds_igif->igif_sflags,
                    IGMP_IF_SFLAG_SNOOP_MROUTER_IF_CONFIG)
        || CHECK_FLAG (ds_igif->igif_sflags,
                       IGMP_IF_SFLAG_SNOOP_MROUTER_IF))
      cli_out (cli, "%-8d%-20s\n", IGMP_IF_GET_VID (igif->igif_su_id),
               ds_igif->igif_owning_ifp->name);

EXIT:

  IGMP_FN_EXIT (igmp_cli_return (cli, ret));
}

s_int32_t
igmp_if_snp_show_statistics (struct cli *cli,
                             struct igmp_if *igif)
{
  struct igmp_group_rec *igr;
  struct ptree_node *pn;
  struct ptree *gmr_tib;
  u_int32_t disp_flags;
  s_int32_t ret;

  IGMP_FN_ENTER (IGMP IF Show Stats);
  
  ret = IGMP_ERR_NONE;
  disp_flags = 0; 
  gmr_tib = NULL;
  igr = NULL;
  pn = NULL;
  
  SET_FLAG (disp_flags, IGMP_SHOW_FLAG_DISP_GROUP);

  if (! cli
      || ! igif)
    {
      ret = IGMP_ERR_MALFORMED_ARG;
      goto EXIT;
    }

  cli_out (cli, "IGMP Snooping statistics for %s\n",
           igif->igif_owning_ifp->name);
 
  /* Check if the interface is of type vlan */ 
  if (if_is_vlan (igif->igif_owning_ifp))
    gmr_tib = igif->igif_hst_gmr_tib;
  else
    gmr_tib = igif->igif_gmr_tib;

  for (pn = ptree_top (gmr_tib); pn; pn = ptree_next (pn))
    {
      if (! (igr = pn->info))
        continue;

      igmp_if_show_group_detail (cli, igr, &disp_flags);
    }

EXIT:

  IGMP_FN_EXIT (ret);
}

#ifdef HAVE_VRF
CLI (show_ip_igmp_snoop_statistics,
     show_ip_igmp_snoop_statistics_cmd,
     "show ip igmp (vrf NAME|) snooping statistics interface IFNAME",
     CLI_SHOW_STR,
     CLI_IP_STR,
     "IGMP information",
     CLI_VRF_STR,
     CLI_VRF_NAME_STR,
     "IGMP Snooping information",
     "IGMP Snooping Statistics",
     "VLAN Interface",
     "VLAN Interface Name")
#else
CLI (show_ip_igmp_snoop_statistics,
     show_ip_igmp_snoop_statistics_cmd,
     "show ip igmp snooping statistics interface IFNAME",
     CLI_SHOW_STR,
     CLI_IP_STR,
     "IGMP information",
     "IGMP Snooping information",
     "IGMP Snooping Statistics",
     "VLAN Interface",
     "VLAN Interface Name")
#endif /* HAVE_VRF */
{
  struct igmp_if_idx igifidx;
  struct igmp_instance *igi;
  struct interface *ifp;
  struct igmp_if *igif;
  struct apn_vrf *ivrf;
  s_int32_t ret;

  IGMP_FN_ENTER (Show IF Snooping Statistics);

  ret = IGMP_ERR_NONE;

  /* Get the VRF Instance */
  if (argc > 2
      && ! pal_strcmp (argv [0], "vrf"))
    {
      ivrf = apn_vrf_lookup_by_name (LIB_GLOB_GET_VR_CONTEXT (cli->zg), 
                                     argv [1]);
       
      if (! ivrf)
        {
          ret = IGMP_ERR_NO_CONTEXT_INFO;
          goto EXIT;
        }

      ifp = ifv_lookup_by_name (LIB_VRF_GET_IF_MASTER (ivrf), argv [2]);
    }
  else
    {
      ivrf = apn_vrf_lookup_by_name (LIB_GLOB_GET_VR_CONTEXT (cli->zg), NULL);

      if (! ivrf)
        {
          ret = IGMP_ERR_NO_CONTEXT_INFO;
          goto EXIT;
        }

      ifp = ifv_lookup_by_name (LIB_VRF_GET_IF_MASTER (ivrf), argv [0]);
    }

  /* Get the IGMP Instance */
  igi = LIB_VRF_GET_IGMP_INSTANCE (ivrf);

  if (! igi)
    {
      ret = IGMP_ERR_NO_CONTEXT_INFO;
      goto EXIT;
    }

  if (! ifp || ! if_is_vlan(ifp))
    {
      ret = IGMP_ERR_NO_SUCH_IFF;
      goto EXIT;
    }

  igifidx.igifidx_ifp = ifp;
  igifidx.igifidx_sid = igmp_if_svc_reg_su_id (igi, ifp);

  igif = igmp_if_lookup (igi, &igifidx);

  if (! igif)
    {
      ret = IGMP_ERR_NO_SUCH_IFF;
      goto EXIT;
    }

  ret = igmp_if_snp_show_statistics (cli, igif);

EXIT:

  IGMP_FN_EXIT (igmp_cli_return (cli, ret));
}

#ifdef HAVE_VRF
CLI (show_ip_igmp_ssm_map,
     show_ip_igmp_ssm_map_cmd,
     "show ip igmp (vrf NAME|) ssm-map",
     CLI_SHOW_STR,
     CLI_IP_STR,
     "IGMP information",
     CLI_VRF_STR,
     CLI_VRF_NAME_STR,
     "Source-Specific-Multicast Mapping")
#else
CLI (show_ip_igmp_ssm_map,
     show_ip_igmp_ssm_map_cmd,
     "show ip igmp ssm-map",
     CLI_SHOW_STR,
     CLI_IP_STR,
     "IGMP information",
     "Source-Specific-Multicast Mapping")
#endif /* HAVE_VRF */
{
  struct igmp_instance *igi;
  struct apn_vrf *ivrf;
  s_int32_t ret;

  IGMP_FN_ENTER (Show SSM-Map);

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

  cli_out (cli, "SSM Mapping : %s\n",
           CHECK_FLAG (igi->igi_cflags,
                       IGMP_INST_CFLAG_SSM_MAP_DISABLED) ?
           "Disabled" : "Enabled");

  cli_out (cli, "Database    : %s\n",
           CHECK_FLAG (igi->igi_cflags,
                       IGMP_INST_CFLAG_SSM_MAP_STATIC) ?
           "Static mappings configured" : "None configured");

EXIT:

  IGMP_FN_EXIT (igmp_cli_return (cli, ret));
}

#ifdef HAVE_VRF
CLI (show_ip_igmp_ssm_map_group,
     show_ip_igmp_ssm_map_group_cmd,
     "show ip igmp (vrf NAME|) ssm-map A.B.C.D",
     CLI_SHOW_STR,
     CLI_IP_STR,
     "IGMP information",
     CLI_VRF_STR,
     CLI_VRF_NAME_STR,
     "Source-Specific-Multicast Mapping",
     "Multicast Group Address")
#else
CLI (show_ip_igmp_ssm_map_group,
     show_ip_igmp_ssm_map_group_cmd,
     "show ip igmp ssm-map A.B.C.D",
     CLI_SHOW_STR,
     CLI_IP_STR,
     "IGMP information",
     "Source-Specific-Multicast Mapping",
     "Multicast Group Address")
#endif /* HAVE_VRF */
{
  struct igmp_ssm_map_static *igssms;
  struct igmp_instance *igi;
  struct pal_in4_addr grp;
  struct access_list *al;
  struct apn_vrf *ivrf;
  bool_t ssm_map_found;
  struct listnode *nn;
  enum filter_type ft;
  struct prefix p;
  s_int32_t ret;

  IGMP_FN_ENTER (Show SSM-Map Group);

  ssm_map_found = PAL_FALSE;
  ret = IGMP_ERR_NONE;

  /* Get the VRF Instance */
  if (argc > 1
      && ! pal_strcmp (argv [0], "vrf"))
    {
      ivrf = apn_vrf_lookup_by_name (LIB_GLOB_GET_VR_CONTEXT (cli->zg), 
                                     argv [1]);

      CLI_GET_IPV4_ADDRESS ("Group address", grp, argv[2]);
    }
  else
    {
      ivrf = apn_vrf_lookup_by_name (LIB_GLOB_GET_VR_CONTEXT (cli->zg), NULL);

      CLI_GET_IPV4_ADDRESS ("Group address", grp, argv[0]);
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

  if (! CHECK_FLAG (igi->igi_cflags,
                  IGMP_INST_CFLAG_SSM_MAP_DISABLED)
      && CHECK_FLAG (igi->igi_cflags,
                     IGMP_INST_CFLAG_SSM_MAP_STATIC))
    LIST_LOOP (&igi->igi_ssm_map_static_lst, igssms, nn)
      {
        /* Apply the access list if present */
        if (! igssms->igssms_grp_alist
            || ! (al = access_list_lookup
                       (LIB_VRF_GET_VR (igi->igi_owning_ivrf),
                        AFI_IP,
                        igssms->igssms_grp_alist)))
          continue;

        p.family = AF_INET;
        p.prefixlen = IPV4_MAX_BITLEN;
        IPV4_ADDR_COPY (&p.u.prefix4, &grp);

        ft = access_list_apply (al, (void_t *) &p);

        switch (ft)
          {
          case FILTER_PERMIT:
            if (! ssm_map_found)
              {
                cli_out (cli, "Group address: %r\n", &grp);
                cli_out (cli, "Database     : Static\n");
                cli_out (cli, "Source list  : ");

                ssm_map_found = PAL_TRUE;
              }
            else
              cli_out (cli, "             : ");

            cli_out (cli, "%r\n", &igssms->igssms_msrc);
            break;

          case FILTER_DENY:
          case FILTER_DYNAMIC:
          case FILTER_NO_MATCH:
            break;
          }
      }

  if (! ssm_map_found)
    cli_out (cli, "\nCan't resolve %r to source-mapping\n\n", &grp);

EXIT:

  IGMP_FN_EXIT (igmp_cli_return (cli, ret));
}

/* IGMP Show CLI Commands Initialization */
s_int32_t
igmp_cli_show_init (struct lib_globals *zlg)
{
  struct cli_tree *ctree;
  s_int32_t ret;

  IGMP_FN_ENTER (CLI Show Init);

  ret = IGMP_ERR_NONE;
  ctree = zlg->ctree;

  /* IGMP show commands */
  cli_install (ctree, EXEC_MODE, &show_ip_igmp_groups_cmd);
  cli_install (ctree, EXEC_MODE, &show_ip_igmp_groups_group_cmd);
  cli_install (ctree, EXEC_MODE, &show_ip_igmp_groups_interface_cmd);
  cli_install (ctree, EXEC_MODE, &show_ip_igmp_groups_interface_group_cmd);
  cli_install (ctree, EXEC_MODE, &show_ip_igmp_proxy_groups_cmd);
  cli_install (ctree, EXEC_MODE, &show_ip_igmp_proxy_groups_group_cmd);
  cli_install (ctree, EXEC_MODE, &show_ip_igmp_proxy_groups_interface_cmd);
  cli_install (ctree, EXEC_MODE, 
               &show_ip_igmp_proxy_groups_interface_group_cmd);
  cli_install (ctree, EXEC_MODE, &show_ip_igmp_proxy_cmd);
  cli_install (ctree, EXEC_MODE, &show_ip_igmp_interface_name_cmd);
  cli_install (ctree, EXEC_MODE, &show_ip_igmp_snoop_mrouter_cmd);
  cli_install (ctree, EXEC_MODE, &show_ip_igmp_snoop_statistics_cmd);
  cli_install (ctree, EXEC_MODE, &show_ip_igmp_ssm_map_cmd);
  cli_install (ctree, EXEC_MODE, &show_ip_igmp_ssm_map_group_cmd);

  IGMP_FN_EXIT (ret);
}

