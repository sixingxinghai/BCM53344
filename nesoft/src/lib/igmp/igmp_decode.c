/* Copyright (C) 2002-2003  All Rights Reserved. */

#include <igmp_incl.h>

/*igmp offlink RPF check*/
extern struct rib *nsm_rib_match (struct nsm_vrf *, afi_t, struct prefix *,
                                  struct nsm_ptree_node **, u_int32_t, u_char);

/* IGMP Membership Query Message Decoder */
s_int32_t
igmp_dec_msg_query (struct igmp_if *igif,
                    u_int8_t max_rsp_code,
                    struct pal_in4_addr *psrc,
                    struct pal_in4_addr *pdst)
{
  struct igmp_source_rec *isr;
  struct igmp_group_rec *igr;
  struct igmp_instance *igi;
  struct pal_in4_addr pgrp;
  struct igmp_if *hst_igif;
  struct pal_in4_addr msrc;
  struct igmp_if *ds_igif; 
  struct igmp_svc_reg *igsr;
  u_int32_t max_rsp_time;
  bool_t reset_oqi_info;
  struct listnode *nn;
  bool_t was_querier;
  u_int32_t rsp_time;
  u_int32_t num_src;
  u_int8_t resv_qrv;
  bool_t warn_timer;
  struct stream *s;
  u_int32_t idx;
  s_int32_t ret;
  u_int8_t qqic;
  bool_t is_grp_query;

  IGMP_FN_ENTER (Dec Query);

  igi = igif->igif_owning_igi;
  reset_oqi_info = PAL_FALSE;
  was_querier = PAL_FALSE;
  warn_timer = PAL_FALSE;
  ret = IGMP_ERR_NONE;
  hst_igif = NULL;
  igr = NULL;
  isr = NULL;
  is_grp_query = PAL_FALSE;

  /* Do not process Self query */
  if (igif->igif_paddr
      && IPV4_ADDR_SAME (psrc, &igif->igif_paddr->u.prefix4))
    {
      ret = IGMP_ERR_NONE;
      goto EXIT;
    }

  /* Get stream buffer */
  s = igi->igi_i_buf;

  /* Decode Group Address */
  pgrp.s_addr = stream_get_ipv4 (s);

  /* Obtain the Router-side Group Rec */
  if (! IPV4_ADDR_SAME (&pgrp, &igi->igi_in4any_addr))
    {
      is_grp_query = PAL_TRUE;
      igr = igmp_if_grec_lookup (igif, &pgrp);

      if (! igr)
        IGMP_DBG_INFO (igi, DECODE, "Router-side G-Rec not found for"
                       " %r", &pgrp);
    }

  /* Host-side behaviour if in IGMP Proxy Service mode */
  if (CHECK_FLAG (igif->igif_conf.igifc_cflags,
                  IGMP_IF_CFLAG_PROXY_SERVICE))
    IGMP_DBG_INFO (igi, DECODE, "%s in Proxy-service mode",
                   igif->igif_owning_ifp->name);

  /* IGMP Other-Querier handling */
  if (! IPV4_ADDR_SAME (psrc, &igi->igi_in4any_addr)
      && igif->igif_paddr
      && (! CHECK_FLAG (igif->igif_sflags,
                        IGMP_IF_SFLAG_SVC_TYPE_L2)
          || CHECK_FLAG (igif->igif_sflags,
                         IGMP_IF_SFLAG_MCAST_ENABLED)
          || CHECK_FLAG (igif->igif_conf.igifc_cflags,
                         IGMP_IF_CFLAG_SNOOP_QUERIER)
          || CHECK_FLAG (igif->igif_conf.igifc_cflags,
                         IGMP_IF_CFLAG_CONFIG_ENABLED)))
    {

      if (IPV4_ADDR_CMP (psrc, &igif->igif_paddr->u.prefix4) < 0)
        {
          IGMP_DBG_INFO (igi, DECODE, "Lower IP %r on %s",
                         psrc, igif->igif_owning_ifp->name);

          if (CHECK_FLAG (igif->igif_sflags, IGMP_IF_SFLAG_QUERIER))
            {
              IGMP_DBG_INFO (igi, DECODE, "Querier->Non-querier on %s",
                             igif->igif_owning_ifp->name);

              UNSET_FLAG (igif->igif_sflags, IGMP_IF_SFLAG_QUERIER);

              reset_oqi_info = PAL_TRUE;
              was_querier = PAL_TRUE;
            }
          else if (IPV4_ADDR_SAME (&igif->igif_other_querier_addr,
                                   &igi->igi_in4any_addr)
                   || IPV4_ADDR_CMP (psrc, &igif->igif_other_querier_addr)
                      <= 0)
            reset_oqi_info = PAL_TRUE;
        }
    }
  else if (CHECK_FLAG (igif->igif_sflags,
                       IGMP_IF_SFLAG_SNOOPING))
    reset_oqi_info = PAL_TRUE;

  /* Extract Max-Reponse Time Interval */
  if (max_rsp_code < IGMP_MSG_TIME_INTERVAL_EXPONENT)
    max_rsp_time = max_rsp_code;
  else
    max_rsp_time = (IGMP_EXTRACT_MANT (max_rsp_code) | 0x10) <<
                   (IGMP_EXTRACT_EXPONENT (max_rsp_code) + 3);

  /* Convert into seconds */
  max_rsp_time = max_rsp_time / ONE_SEC_DECISECOND;

  /* IGMP Router-side OR Proxy-Host-side query handling */
  switch (igif->igif_conf.igifc_version)
    {
    case IGMP_VERSION_1:
      if (max_rsp_code || STREAM_DATA_REMAIN (s))
        {
          if (max_rsp_code < IGMP_MSG_TIME_INTERVAL_EXPONENT
              && ! STREAM_DATA_REMAIN (s))
            igif->igif_v2_querier_wcount += 1;
          else
            igif->igif_v3_querier_wcount += 1;

          IGMP_TIMER_ON (IGMP_LG (igi), igif->t_igif_warn_rlimit,
                         igif, igmp_if_timer_warn_rlimit,
                         IGMP_WARN_RATE_LIMIT_INTERVAL);
        }
      else /* ==> Version 1 Query */
        {
          /* Schedule response to the Query */
          rsp_time = igmp_if_timer_generate_jitter
                     (IGMP_QUERY_RESPONSE_INTERVAL_DEF);

          ret = igmp_if_hst_sched_report (igif, &pgrp,
                                          NULL, rsp_time);
        }
      break;

    case IGMP_VERSION_2:
      if (! max_rsp_code && ! STREAM_DATA_REMAIN (s))
        {
          igif->igif_v1_querier_wcount += 1;
          warn_timer = PAL_TRUE;

          /* Reset the V1-Querier-Present Timer */
          if (CHECK_FLAG (igif->igif_conf.igifc_cflags,
                          IGMP_IF_CFLAG_PROXY_SERVICE)
              || CHECK_FLAG (igif->igif_sflags,
                             IGMP_IF_SFLAG_SNOOPING))
            {
              IGMP_TIMER_OFF (IGMP_LG (igi),
                              igif->t_igif_hst_v1_querier_present);
              IGMP_TIMER_ON (IGMP_LG (igi),
                             igif->t_igif_hst_v1_querier_present,
                             igif, igmp_if_hst_timer_v1_querier_present,
                             igif->igif_gmi);

              /* Record V1 Host-Compat-Mode only if General Query */
              if (IPV4_ADDR_SAME (&pgrp, &igi->igi_in4any_addr))
                (void_t) igmp_if_hst_update_host_cmode (igif);
            }
        }
      else if (STREAM_DATA_REMAIN (s))
        {
          igif->igif_v3_querier_wcount += 1;
          warn_timer = PAL_TRUE;
        }

      if (warn_timer)
        IGMP_TIMER_ON (IGMP_LG (igi), igif->t_igif_warn_rlimit,
                       igif, igmp_if_timer_warn_rlimit,
                       IGMP_WARN_RATE_LIMIT_INTERVAL);

      /* Reduce the Group-Timer if necessary */
      if (igr
          && reset_oqi_info
          && IPV4_ADDR_SAME (pdst, &pgrp))
        ret = igmp_if_grec_update (igr, max_rsp_time);

      /* Schedule response to the Query */
      rsp_time = igmp_if_timer_generate_jitter (max_rsp_time);

      ret = igmp_if_hst_sched_report (igif, &pgrp, NULL, rsp_time);
      break;

    case IGMP_VERSION_3:
      if (! STREAM_DATA_REMAIN (s))
        {
          if (max_rsp_code)
            {
              igif->igif_v2_querier_wcount += 1;

              /* Reset the V2-Querier-Present Timer */
              if (CHECK_FLAG (igif->igif_conf.igifc_cflags,
                              IGMP_IF_CFLAG_PROXY_SERVICE)
                  || CHECK_FLAG (igif->igif_sflags,
                                 IGMP_IF_SFLAG_SNOOPING))
                {
                  IGMP_TIMER_OFF (IGMP_LG (igi),
                                  igif->t_igif_hst_v2_querier_present);
                  IGMP_TIMER_ON (IGMP_LG (igi),
                                 igif->t_igif_hst_v2_querier_present,
                                 igif, igmp_if_hst_timer_v2_querier_present,
                                 igif->igif_gmi);

                  /* Record V2 Host-Compat-Mode only if General Query */
                  if (IPV4_ADDR_SAME (&pgrp, &igi->igi_in4any_addr))
                    (void_t) igmp_if_hst_update_host_cmode (igif);
                }
            }
          else
            {
              igif->igif_v1_querier_wcount += 1;

              /* Reset the V1-Querier-Present Timer */
              if (CHECK_FLAG (igif->igif_conf.igifc_cflags,
                              IGMP_IF_CFLAG_PROXY_SERVICE)
                  || CHECK_FLAG (igif->igif_sflags,
                                 IGMP_IF_SFLAG_SNOOPING))
                {
                  IGMP_TIMER_OFF (IGMP_LG (igi),
                                  igif->t_igif_hst_v1_querier_present);
                  IGMP_TIMER_ON (IGMP_LG (igi),
                                 igif->t_igif_hst_v1_querier_present,
                                 igif, igmp_if_hst_timer_v1_querier_present,
                                 igif->igif_gmi);

                  /* Record V1 Host-Compat-Mode only if General Query */
                  if (IPV4_ADDR_SAME (&pgrp, &igi->igi_in4any_addr))
                    (void_t) igmp_if_hst_update_host_cmode (igif);
                }
            }

          IGMP_TIMER_ON (IGMP_LG (igi), igif->t_igif_warn_rlimit,
                         igif, igmp_if_timer_warn_rlimit,
                         IGMP_WARN_RATE_LIMIT_INTERVAL);

          /* Reduce the Group-Timer if necessary */
          if (igr
              && reset_oqi_info
              && IPV4_ADDR_SAME (pdst, &pgrp))
            ret = igmp_if_grec_update (igr, max_rsp_time);

          /* Schedule response to Query in Proxy mode */
          rsp_time = igmp_if_timer_generate_jitter (max_rsp_time);

          ret = igmp_if_hst_sched_report (igif, &pgrp, NULL, rsp_time);
        }
      else
        {
          /* No processing if no-reset-OQI, non-proxy, non-snoop */
          if (! reset_oqi_info
              && ! CHECK_FLAG (igif->igif_conf.igifc_cflags,
                               IGMP_IF_CFLAG_PROXY_SERVICE)
              && ! CHECK_FLAG (igif->igif_sflags,
                               IGMP_IF_SFLAG_SNOOPING))
            {
              ret = IGMP_ERR_NONE;
              goto EXIT;
            }

          /* Validate remaining length */
          if (STREAM_DATA_REMAIN (s) <
              IGMP_MSG_QUERY_V3_ADDITIONAL_MIN_SIZE)
            {
              IGMP_DBG_INFO (igi, DECODE, "Msg size < v3 Query Add. Hdr");

              ret = IGMP_ERR_MALFORMED_MSG;
              goto CLEANUP;
            }

          /* Decode Resv-QRV Field */
          resv_qrv = stream_getc (s);

          /* Decode Querier's Query Interval Code (QQIC) */
          qqic = stream_getc (s);

          /* Decode the Number of Sources */
          num_src = stream_getw (s);

          if (STREAM_DATA_REMAIN (s) < num_src * IPV4_MAX_BYTELEN)
            {
              IGMP_DBG_INFO (igi, ENCODE, "Msg size < v3 Query Srcs");

              ret = IGMP_ERR_MALFORMED_MSG;
              goto CLEANUP;
            }

          /* Group-Rec should be present if NumSrcs != 0 */
          if (num_src && ! igr)
            {
              IGMP_DBG_INFO (igi, DECODE, "G-Rec not present!");

              ret = IGMP_ERR_TEMP_INTERNAL;
              goto CLEANUP;
            }

          /* Update timers if 'S Flag' is unset */
          if (igr
              && ! num_src
              && IPV4_ADDR_SAME (pdst, &pgrp)
              && ! CHECK_FLAG (resv_qrv,
                               IGMP_MSG_QUERY_SUPPRESS_FLAG_MASK))
            {
              /* Reduce the Group-Timer if necessary */
              ret = igmp_if_grec_update (igr, max_rsp_time);

              if (ret < IGMP_ERR_NONE)
                {
                  IGMP_DBG_INFO (igi, DECODE, "G-Rec update failed!");

                  goto CLEANUP;
                }
            }

          /* Adopt QRV if non-zero */
          if (resv_qrv & IGMP_MSG_QUERY_QRV_MASK)
            igif->igif_rv = resv_qrv & IGMP_MSG_QUERY_QRV_MASK;
          else
            igif->igif_rv = igif->igif_conf.igifc_rv;

          /* Adopt QQIC if non-zero */
          if (! CHECK_FLAG (igif->igif_sflags,
                            IGMP_IF_SFLAG_QUERIER)
              && qqic)
            {
              if (qqic < IGMP_MSG_TIME_INTERVAL_EXPONENT)
                igif->igif_qi = qqic;
              else
                igif->igif_qi = (IGMP_EXTRACT_MANT (qqic) | 0x10) <<
                                (IGMP_EXTRACT_EXPONENT (qqic) + 3);
            }
          else
            igif->igif_qi = igif->igif_conf.igifc_qi;

          /* Update the Interface GMI */
          igif->igif_gmi = igif->igif_rv *
                           igif->igif_qi +
                           igif->igif_conf.igifc_qri;

          if (! CHECK_FLAG (igif->igif_conf.igifc_cflags,
                            IGMP_IF_CFLAG_QUERIER_TIMEOUT))
            igif->igif_oqi = igif->igif_rv *
                             igif->igif_qi +
                             (igif->igif_conf.igifc_qri / 2);

          /* Schedule response to Query in Proxy mode */
          rsp_time = igmp_if_timer_generate_jitter (max_rsp_time);

          if (! num_src)
            ret = igmp_if_hst_sched_report (igif, &pgrp,
                                            NULL, rsp_time);

          /* Decode the Sources List */
          for (idx = 0; idx < num_src; idx++)
            {
              msrc.s_addr = stream_get_ipv4 (s);

              if (IN_EXPERIMENTAL (pal_ntoh32 (msrc.s_addr))
                  || IN_MULTICAST (pal_ntoh32 (msrc.s_addr))
                  || IPV4_ADDR_MARTIAN (pal_ntoh32 (msrc.s_addr)))
                {
                  IGMP_DBG_WARN (igi, DECODE, "Invalid M-Src Addr %r",
                                 &msrc);

                  continue;
                }

              /* Only sources in Tib-A will have timers running */
              isr = igmp_if_srec_lookup (igr, PAL_TRUE, &msrc);

              /* Update Src-Rec Timers if necessary */
              if (! CHECK_FLAG (resv_qrv,
                                IGMP_MSG_QUERY_SUPPRESS_FLAG_MASK)
                  && isr)
                {
                  ret = igmp_if_srec_update (isr, max_rsp_time);

                  if (ret < IGMP_ERR_NONE)
                    {
                      IGMP_DBG_WARN (igi, DECODE, "Source Rec(%r)"
                                     " update Failed(%d)!",
                                     &msrc, ret);

                      continue;
                    }
                }

              /* Schedule response to Query in Proxy mode */
              ret = igmp_if_hst_sched_report
                    (igif, &pgrp, &msrc, rsp_time);
            }
        }
      break;
    }

  /*join group handling*/
  if (is_grp_query && igr)
    {  /*Group-Specific Query*/     
      if (CHECK_FLAG (igr->igr_sflags, IGMP_IGR_SFLAG_JOIN))
        {
          IGMP_DBG_INFO (igi, DECODE, 
                         "Router-side: join group response Group-Specific Query(G %r) on %s", 
                         &pgrp, igif->igif_owning_ifp->name);
          
          /* Schedule response to the Query */
          rsp_time = igmp_if_timer_generate_jitter (max_rsp_time);
          ret = igmp_if_igr_group_report(igif, igr, rsp_time);
        }
    }
  else if (!is_grp_query)
    {  /*General Query*/
      IGMP_DBG_INFO (igi, DECODE, 
                     "Router-side: join group response General Query (G %r) on %s", 
                      &pgrp, igif->igif_owning_ifp->name);

      /* Schedule response to the Query */
      rsp_time = igmp_if_timer_generate_jitter (max_rsp_time);
      ret = igmp_if_igr_group_report(igif, NULL, rsp_time);
    }
  else
    {
      IGMP_DBG_INFO (igi, DECODE, "Group-Specific Query, But Router-side G-Rec not found for"
                          " %r", &pgrp);
    }
    
  if (reset_oqi_info)
    {
     /* Learn the presence of MRouter on this interface */
     if (CHECK_FLAG (igif->igif_sflags, IGMP_IF_SFLAG_SNOOPING))
       {
         /* Add mrouter interface to all group entries in forwarder */
         if ( ! CHECK_FLAG (igif->igif_sflags, IGMP_IF_SFLAG_SNOOP_MROUTER_IF)
              && ! CHECK_FLAG (igif->igif_sflags, 
                       IGMP_IF_SFLAG_SNOOP_MROUTER_IF_CONFIG) && igi)
            LIST_LOOP (&igi->igi_svc_reg_lst, igsr, nn)
             if (igsr->igsr_cback_mrt_if_update)
               {
                 igsr->igsr_cback_mrt_if_update (igsr->igsr_su_id,
                                                 igif->igif_owning_ifp,
                                                 igif->igif_su_id,
                                                 PAL_FALSE);
               }
         SET_FLAG (igif->igif_sflags, IGMP_IF_SFLAG_SNOOP_MROUTER_IF);
       }

      IPV4_ADDR_COPY (&igif->igif_other_querier_addr, psrc);

      if (CHECK_FLAG (igif->igif_sflags, IGMP_IF_SFLAG_SVC_TYPE_L2)
          && ! CHECK_FLAG (igif->igif_sflags, IGMP_IF_SFLAG_SVC_TYPE_L3)
          && (hst_igif = igif->igif_hst_igif))
        {
          LIST_LOOP (&hst_igif->igif_hst_dsif_lst, ds_igif, nn)
            {
              /* Stop the querier timer */
              if (was_querier)
                {
                  IGMP_TIMER_OFF (IGMP_LG (igi), ds_igif->t_igif_querier);

                  UNSET_FLAG (ds_igif->igif_sflags, IGMP_IF_SFLAG_QUERIER);
                }

              /* Reset Other-Querier-Present Timer */
              IGMP_TIMER_OFF (IGMP_LG (igi), ds_igif->t_igif_other_querier);
              IGMP_TIMER_ON (IGMP_LG (igi), ds_igif->t_igif_other_querier,
                             ds_igif, igmp_if_timer_other_querier,
                             ds_igif->igif_oqi);

              /* Forward the Query onto other DS-IFs */
              if (igif != ds_igif)
                igmp_if_hst_forward_query (igif, ds_igif);
            }

          /* Update the VLAN interface Querier Status */
          if (was_querier)
            UNSET_FLAG (hst_igif->igif_sflags, IGMP_IF_SFLAG_QUERIER);
        }
      else
        {
          /* Stop the querier timer */
          if (was_querier)
            IGMP_TIMER_OFF (IGMP_LG (igi), igif->t_igif_querier);

          /* Reset Other-Querier-Present Timer */
          if (! CHECK_FLAG (igif->igif_conf.igifc_cflags,
                            IGMP_IF_CFLAG_PROXY_SERVICE))
            {
              IGMP_TIMER_OFF (IGMP_LG (igi), igif->t_igif_other_querier);
              IGMP_TIMER_ON (IGMP_LG (igi), igif->t_igif_other_querier,
                             igif, igmp_if_timer_other_querier,
                             igif->igif_oqi);
            }
        }
    }

  goto EXIT;

CLEANUP:

  if (was_querier)
    SET_FLAG (igif->igif_sflags, IGMP_IF_SFLAG_QUERIER);

  /* Conditionally loose the ret code */
  if (ret == IGMP_ERR_TEMP_INTERNAL
      || ret == IGMP_ERR_MALFORMED_MSG)
    ret = IGMP_ERR_NONE;

EXIT:

  IGMP_FN_EXIT (ret);
}

/* IGMP V1 Membership Report Message Decoder */
s_int32_t
igmp_dec_msg_v1_report (struct igmp_if *igif,
                        struct pal_in4_addr *psrc,
                        struct pal_in4_addr *pdst)
{
  enum igmp_filter_mode_event rec_type;
  struct igmp_ssm_map_static *igssms;
  struct igmp_source_list *isl;
  struct igmp_source_rec *isr;
  struct igmp_group_rec *igr;
  struct ptree_node *pn_next;
  struct igmp_instance *igi;
  struct pal_in4_addr pgrp;
  bool_t apply_limit_check;
  u_int32_t igif_num_grecs;
  u_int32_t src_list_size;
  struct access_list *al;
  struct ptree_node *pn;
  struct listnode *nn;
  enum filter_type ft;
  struct stream *s;
  struct prefix p;
  s_int32_t ret;

  IGMP_FN_ENTER (Dec V1 Report);

  rec_type = IGMP_FME_MODE_IS_EXCL;
  igi = igif->igif_owning_igi;
  ret = IGMP_ERR_NONE;
  isl = NULL;

  /* Ignore Report if in IGMP Proxy Service mode */
  if (CHECK_FLAG (igif->igif_conf.igifc_cflags,
                  IGMP_IF_CFLAG_PROXY_SERVICE))
    {
      IGMP_DBG_INFO (igi, DECODE, "%s in Proxy-service mode, Ignoring",
                     igif->igif_owning_ifp->name);

      ret = IGMP_ERR_NONE;
      goto EXIT;
    }

  /* Get stream buffer */
  s = igi->igi_i_buf;

  /* Decode Group Address */
  pgrp.s_addr = stream_get_ipv4 (s);

  /* Do not process LAN groups */
  if (pal_ntoh32 (pgrp.s_addr) <= INADDR_MAX_LOCAL_GROUP
      || ! IN_MULTICAST (pal_ntoh32 (pgrp.s_addr)))
    {
      IGMP_DBG_WARN (igi, DECODE, "Invalid Grp %r on %s",
                     &pgrp, igif->igif_owning_ifp->name);

      ret = IGMP_ERR_NONE;
      goto EXIT;
    }

  IGMP_DBG_INFO (igi, DECODE, "Grp %r on %s", &pgrp,
                 igif->igif_owning_ifp->name);

  /* Apply the access list if present */
  if (igif->igif_conf.igifc_access_list
      && (al = access_list_lookup
               (LIB_VRF_GET_VR (igi->igi_owning_ivrf),
                AFI_IP, igif->igif_conf.igifc_access_list)))
    {
      p.family = AF_INET;
      p.prefixlen = IPV4_MAX_BITLEN;
      IPV4_ADDR_COPY (&p.u.prefix4, &pgrp);

      ft = access_list_apply (al, (void_t *) &p);

      switch (ft)
        {
        case FILTER_DENY:
          IGMP_DBG_INFO (igi, DECODE, "Grp %r denied on %s by alist %s",
                         &pgrp, igif->igif_owning_ifp->name,
                         igif->igif_conf.igifc_access_list);

          ret = IGMP_ERR_NONE;
          goto EXIT;
          break;

        case FILTER_PERMIT:
        case FILTER_DYNAMIC:
        case FILTER_NO_MATCH:
          break;
        }
    }

  /* SSM-Map processing */
  if (! CHECK_FLAG (igi->igi_cflags, IGMP_INST_CFLAG_SSM_MAP_DISABLED)
      && (pal_ntoh32 (pgrp.s_addr) & IN_CLASSA_NET) ==
          INADDR_SSM_RANGE_ADDRESS)
    {
      /* Static SSM-Map processing */
      if (CHECK_FLAG (igi->igi_cflags, IGMP_INST_CFLAG_SSM_MAP_STATIC))
        LIST_LOOP (&igi->igi_ssm_map_static_lst, igssms, nn)
          {
            /* Apply the access list if present */
            if (! igssms->igssms_grp_alist
                || ! (al = access_list_lookup
                           (LIB_VRF_GET_VR (igi->igi_owning_ivrf),
                            AFI_IP, igssms->igssms_grp_alist)))
              continue;

            p.family = AF_INET;
            p.prefixlen = IPV4_MAX_BITLEN;
            IPV4_ADDR_COPY (&p.u.prefix4, &pgrp);

            ft = access_list_apply (al, (void_t *) &p);

            switch (ft)
              {
              case FILTER_PERMIT:
                IGMP_DBG_INFO (igi, DECODE, "Grp %r on %s matched"
                               " SSM-Map alist %s, MSrc %r",
                               &pgrp, igif->igif_owning_ifp->name,
                               igssms->igssms_grp_alist,
                               &igssms->igssms_msrc);

                /* Create the Source-List */
                if (! isl)
                  {
                    src_list_size = sizeof (struct igmp_source_list);

                    isl = XCALLOC (MTYPE_IGMP_SRC_LIST, src_list_size);

                    if (! isl)
                      {
                        IGMP_DBG_ERR (igi, DECODE, "Cannot allocate "
                                      "memory (%d) @ %s:%d",
                                      src_list_size,
                                      __FILE__, __LINE__);

                        ret = IGMP_ERR_OOM;
                        goto EXIT;
                      }
                  }
                else
                  {
                    src_list_size = sizeof (struct igmp_source_list)
                                    + sizeof (struct pal_in4_addr)
                                    * isl->isl_num;

                    isl = XREALLOC (MTYPE_IGMP_SRC_LIST,
                                    isl, src_list_size);

                    if (! isl)
                      {
                        IGMP_DBG_ERR (igi, DECODE, "Cannot allocate "
                                      "memory (%d) @ %s:%d",
                                      src_list_size,
                                      __FILE__, __LINE__);

                        ret = IGMP_ERR_OOM;
                        goto EXIT;
                      }
                  }

                /* Populate the Source-List */
                IPV4_ADDR_COPY (&isl->isl_saddr [isl->isl_num],
                                &igssms->igssms_msrc);
                isl->isl_num += 1;
                break;

              case FILTER_DENY:
              case FILTER_DYNAMIC:
              case FILTER_NO_MATCH:
                break;
              }
          }

      /* Change the rec_type if SSM-Map matched */
      if (isl)
        rec_type = IGMP_FME_MODE_IS_INCL;
    }

  /* Ignore v1_report for a group address in SSM range*/
  if (! isl
      && igif->igif_conf.igifc_version == IGMP_VERSION_3
      && (pal_ntoh32 (pgrp.s_addr) & IN_CLASSA_NET) ==
         INADDR_SSM_RANGE_ADDRESS)
    {
      IGMP_DBG_WARN (igi, DECODE, "SSM Grp %r on %s, Ignoring",
                     &pgrp, igif->igif_owning_ifp->name);

      ret = IGMP_ERR_NONE;
      goto EXIT;
    }

  /* Lookup the G-Rec for Limit processing */
  igr = igmp_if_grec_lookup (igif, &pgrp);

  /* Validate against Global and IF G-Rec Limit violation */
  if (! igr)
    {
      /* Validate against Global G-Rec Limit violation */
      if (CHECK_FLAG (igi->igi_cflags, IGMP_INST_CFLAG_LIMIT_GREC))
        {
          apply_limit_check = PAL_TRUE;

          if (igi->igi_limit_except_alist
              && (al = access_list_lookup
                       (LIB_VRF_GET_VR (igi->igi_owning_ivrf),
                        AFI_IP, igi->igi_limit_except_alist)))
            {
              p.family = AF_INET;
              p.prefixlen = IPV4_MAX_BITLEN;
              IPV4_ADDR_COPY (&p.u.prefix4, &pgrp);

              ft = access_list_apply (al, (void_t *) &p);

              switch (ft)
                {
                case FILTER_PERMIT:
                  IGMP_DBG_INFO (igi, DECODE, "%r on %s exempt from"
                                 " Global-Limit(%u), CurrNumGRecs(%u)",
                                 &pgrp, igif->igif_owning_ifp->name,
                                 igi->igi_limit, igi->igi_num_grecs);

                  apply_limit_check = PAL_FALSE;
                  break;

                case FILTER_DENY:
                case FILTER_DYNAMIC:
                case FILTER_NO_MATCH:
                  break;
                }
            }

          if (apply_limit_check
              && igi->igi_num_grecs >= igi->igi_limit)
            {
              IGMP_DBG_INFO (igi, DECODE, "%r on %s exceeds Global"
                             " Limit(%u)", &pgrp,
                             igif->igif_owning_ifp->name,
                             igi->igi_limit);

              ret = IGMP_ERR_IF_GREC_LIMIT_REACHED;
              goto EXIT;
            }
        }

      /* Validate against IF G-Rec Limit violation */
      if (CHECK_FLAG (igif->igif_conf.igifc_cflags,
                      IGMP_IF_CFLAG_LIMIT_GREC))
        {
          apply_limit_check = PAL_TRUE;

          igif_num_grecs = CHECK_FLAG (igif->igif_sflags,
                                       IGMP_IF_SFLAG_SVC_TYPE_L2)
                           && ! CHECK_FLAG (igif->igif_sflags,
                                            IGMP_IF_SFLAG_SVC_TYPE_L3) ?
                           igif->igif_hst_igif->igif_num_grecs:
                           igif->igif_num_grecs;

          if (igif->igif_conf.igifc_limit_except_alist
              && (al = access_list_lookup
                       (LIB_VRF_GET_VR (igi->igi_owning_ivrf),
                        AFI_IP,
                        igif->igif_conf.igifc_limit_except_alist)))
            {
              p.family = AF_INET;
              p.prefixlen = IPV4_MAX_BITLEN;
              IPV4_ADDR_COPY (&p.u.prefix4, &pgrp);

              ft = access_list_apply (al, (void_t *) &p);

              switch (ft)
                {
                case FILTER_PERMIT:
                  IGMP_DBG_INFO (igi, DECODE, "%r on %s exempt from"
                                 " IF-Limit(%u), CurrNumGRecs(%u)",
                                 &pgrp, igif->igif_owning_ifp->name,
                                 igif->igif_conf.igifc_limit,
                                 igif_num_grecs);

                  apply_limit_check = PAL_FALSE;
                  break;

                case FILTER_DENY:
                case FILTER_DYNAMIC:
                case FILTER_NO_MATCH:
                  break;
                }
            }

          if (apply_limit_check
              && igif_num_grecs >=
                 igif->igif_conf.igifc_limit)
            {
              IGMP_DBG_INFO (igi, DECODE, "%r exceeds Limit(%u) on %s",
                             &pgrp, igif->igif_conf.igifc_limit,
                             igif->igif_owning_ifp->name);

              ret = IGMP_ERR_IF_GREC_LIMIT_REACHED;
              goto EXIT;
            }
        }

      /* Obtain the Group Rec */
      igr = igmp_if_grec_get (igif, &pgrp);

      if (! igr)
        {
          IGMP_DBG_INFO (igi, DECODE, "Failed to 'get' Group-Rec!");

          ret = IGMP_ERR_OOM;
          goto EXIT;
        }
    }

  /* Stamp as a Dynamic Group */
  SET_FLAG (igr->igr_sflags, IGMP_IGR_SFLAG_DYNAMIC);

  /* Record last reporting host */
  igr->igr_last_reporter = *psrc;

  /* In case it is an L2 interface, update the igr_last_reporter field
   * in the host side group record also.
   */
  if (INTF_TYPE_L2 (igif->igif_owning_ifp)) 
    {
      ret = igmp_if_hst_grec_update(igif, &pgrp, igr);

      /* Updation will fail the first time, in which case the
       * igr_last_reporter field will be updated in the
       * igmp_if_hst_consolidate_mship(). From the next time the
       * igr_last_reporter field will be updated successfully
       */	
      
      if (ret < IGMP_ERR_NONE)
        IGMP_DBG_INFO (igi, DECODE, "G-Rec not found!"
                       " on %s for %r", 
                       igif->igif_owning_ifp->name, &pgrp);
    }
  
  /* Loose the Sources in TIB-A and TIB-B */
  for (pn = ptree_top (igr->igr_src_a_tib); pn; pn = pn_next)
    {
      pn_next = ptree_next (pn);

      if (! (isr = pn->info))
        continue;

      igmp_if_srec_delete (isr);

      igr->igr_src_a_tib_count -= 1;
    }

  for (pn = ptree_top (igr->igr_src_b_tib); pn; pn = pn_next)
    {
      pn_next = ptree_next (pn);

      if (! (isr = pn->info))
        continue;

      igmp_if_srec_delete (isr);

      igr->igr_src_b_tib_count -= 1;
    }

  /* Reset the V1-Host-Present Timer */
  IGMP_TIMER_OFF (IGMP_LG (igi), igr->t_igr_v1_host_present);
  IGMP_TIMER_ON (IGMP_LG (igi),
                 igr->t_igr_v1_host_present,
                 igr, igmp_if_grec_timer_v1_host_present,
                 igif->igif_gmi);

  /* Record V1 Compatibility Mode */
  igmp_if_grec_update_cmode (igr);

  /* Increment Grp-Joins Counter (G-Rec add activity) */
  igif->igif_num_joins += 1;

  /* Process FSM event */
  ret = igmp_igr_fsm_process_event (igr, rec_type, isl);

  if (ret < IGMP_ERR_NONE)
    {
      IGMP_DBG_INFO (igi, DECODE, "IGR FSM Failed(%d)! on %s for %r",
                     ret, igif->igif_owning_ifp->name, &pgrp);

      goto EXIT;
    }

EXIT:

  if (isl)
    XFREE (MTYPE_IGMP_SRC_LIST, isl);

  /* Error-code correction check-post */
  switch (ret)
    {
    case IGMP_ERR_IF_GREC_LIMIT_REACHED:
    case IGMP_ERR_OOM:
      ret = IGMP_ERR_NONE;
      break;

    default:
      break;
    }

  IGMP_FN_EXIT (ret);
}

/* IGMP V2 Membership Report Message Decoder */
s_int32_t
igmp_dec_msg_v2_report (struct igmp_if *igif,
                        struct pal_in4_addr *psrc,
                        struct pal_in4_addr *pdst)
{
  enum igmp_filter_mode_event rec_type;
  struct igmp_ssm_map_static *igssms;
  struct igmp_source_list *isl;
  struct igmp_source_rec *isr;
  struct igmp_group_rec *igr;
  struct ptree_node *pn_next;
  struct igmp_instance *igi;
  struct pal_in4_addr pgrp;
  bool_t apply_limit_check;
  u_int32_t igif_num_grecs;
  u_int32_t src_list_size;
  struct access_list *al;
  struct ptree_node *pn;
  struct listnode *nn;
  enum filter_type ft;
  struct stream *s;
  struct prefix p;
  s_int32_t ret;

  IGMP_FN_ENTER (Dec V2 Report);

  rec_type = IGMP_FME_MODE_IS_EXCL;
  igi = igif->igif_owning_igi;
  ret = IGMP_ERR_NONE;
  isl = NULL;

  /* Ignore Report if in IGMP Proxy Service mode */
  if (CHECK_FLAG (igif->igif_conf.igifc_cflags,
                  IGMP_IF_CFLAG_PROXY_SERVICE))
    {
      IGMP_DBG_INFO (igi, DECODE, "%s in Proxy-service mode, Ignoring",
                     igif->igif_owning_ifp->name);

      ret = IGMP_ERR_NONE;
      goto EXIT;
    }

  /* Ignoring higher version report*/
  switch (igif->igif_conf.igifc_version)
    {
    case IGMP_VERSION_1:
      IGMP_DBG_INFO (igi, DECODE, "V2 report on interface %s "
                     "configured for %d, ignored ",
                     igif->igif_owning_ifp->name,
                     igif->igif_conf.igifc_version);
      goto EXIT;
      break;

    default:
      break;
    }

  /* Get stream buffer */
  s = igi->igi_i_buf;

  /* Decode Group Address */
  pgrp.s_addr = stream_get_ipv4 (s);

  /* Do not process LAN groups */
  if (pal_ntoh32 (pgrp.s_addr) <= INADDR_MAX_LOCAL_GROUP
      || ! IN_MULTICAST (pal_ntoh32 (pgrp.s_addr)))
    {
      IGMP_DBG_INFO (igi, DECODE, "Invalid Grp %r on %s, Ignoring...",
                     &pgrp, igif->igif_owning_ifp->name);

      ret = IGMP_ERR_NONE;
      goto EXIT;
    }

  IGMP_DBG_INFO (igi, DECODE, "Grp %r on %s",
                 &pgrp, igif->igif_owning_ifp->name);

  /* Apply the access list if present */
  if ((igif->igif_conf.igifc_access_list)
      && (al = access_list_lookup
               (LIB_VRF_GET_VR (igi->igi_owning_ivrf),
                AFI_IP, igif->igif_conf.igifc_access_list)))
    {
      p.family = AF_INET;
      p.prefixlen = IPV4_MAX_BITLEN;
      IPV4_ADDR_COPY (&p.u.prefix4, &pgrp);

      ft = access_list_apply (al, (void_t *) &p);
      switch (ft)
        {
        case FILTER_DENY:
          IGMP_DBG_INFO (igi, DECODE, "Grp %r denied on %s by alist %s",
                         &pgrp, igif->igif_owning_ifp->name,
                         igif->igif_conf.igifc_access_list);

          ret = IGMP_ERR_NONE;
          goto EXIT;
          break;

        case FILTER_PERMIT:
        case FILTER_DYNAMIC:
        case FILTER_NO_MATCH:
          break;
        }
    }

  /* SSM-Map processing */
  if (! CHECK_FLAG (igi->igi_cflags, IGMP_INST_CFLAG_SSM_MAP_DISABLED)
      && (pal_ntoh32 (pgrp.s_addr) & IN_CLASSA_NET) ==
          INADDR_SSM_RANGE_ADDRESS)
    {
      /* Static SSM-Map processing */
      if (CHECK_FLAG (igi->igi_cflags, IGMP_INST_CFLAG_SSM_MAP_STATIC))
        LIST_LOOP (&igi->igi_ssm_map_static_lst, igssms, nn)
          {
            /* Apply the access list if present */
            if (! igssms->igssms_grp_alist
                || ! (al = access_list_lookup
                           (LIB_VRF_GET_VR (igi->igi_owning_ivrf),
                            AFI_IP, igssms->igssms_grp_alist)))
              continue;

            p.family = AF_INET;
            p.prefixlen = IPV4_MAX_BITLEN;
            IPV4_ADDR_COPY (&p.u.prefix4, &pgrp);

            ft = access_list_apply (al, (void_t *) &p);

            switch (ft)
              {
              case FILTER_PERMIT:
                IGMP_DBG_INFO (igi, DECODE, "Grp %r on %s matched"
                               " SSM-Map alist %s, MSrc %r",
                               &pgrp, igif->igif_owning_ifp->name,
                               igssms->igssms_grp_alist,
                               &igssms->igssms_msrc);

                /* Create the Source-List */
                if (! isl)
                  {
                    src_list_size = sizeof (struct igmp_source_list);

                    isl = XCALLOC (MTYPE_IGMP_SRC_LIST, src_list_size);

                    if (! isl)
                      {
                        IGMP_DBG_ERR (igi, DECODE, "Cannot allocate "
                                      "memory (%d) @ %s:%d",
                                      src_list_size,
                                      __FILE__, __LINE__);

                        ret = IGMP_ERR_OOM;
                        goto EXIT;
                      }
                  }
                else
                  {
                    src_list_size = sizeof (struct igmp_source_list)
                                    + sizeof (struct pal_in4_addr)
                                    * isl->isl_num;

                    isl = XREALLOC (MTYPE_IGMP_SRC_LIST,
                                    isl, src_list_size);

                    if (! isl)
                      {
                        IGMP_DBG_ERR (igi, DECODE, "Cannot allocate "
                                      "memory (%d) @ %s:%d",
                                      src_list_size,
                                      __FILE__, __LINE__);

                        ret = IGMP_ERR_OOM;
                        goto EXIT;
                      }
                  }

                /* Populate the Source-List */
                IPV4_ADDR_COPY (&isl->isl_saddr [isl->isl_num],
                                &igssms->igssms_msrc);
                isl->isl_num += 1;
                break;

              case FILTER_DENY:
              case FILTER_DYNAMIC:
              case FILTER_NO_MATCH:
                break;
              }
          }

      /* Change the rec_type if SSM-Map matched */
      if (isl)
        rec_type = IGMP_FME_MODE_IS_INCL;
    }

  /* Ignore V2-Report for a group address in SSM range*/
  if (! isl
      && igif->igif_conf.igifc_version == IGMP_VERSION_3
      && (pal_ntoh32 (pgrp.s_addr) & IN_CLASSA_NET) ==
          INADDR_SSM_RANGE_ADDRESS)
    {
      IGMP_DBG_WARN (igi, DECODE, "SSM Grp %r on %s, Ignoring",
                     &pgrp, igif->igif_owning_ifp->name);

      ret = IGMP_ERR_NONE;
      goto EXIT;
    }

  /* Lookup the G-Rec for Limit processing */
  igr = igmp_if_grec_lookup (igif, &pgrp);

  /* Validate against Global and IF G-Rec Limit violation */
  if (! igr)
    {
      /* Validate against Global G-Rec Limit violation */
      if (CHECK_FLAG (igi->igi_cflags, IGMP_INST_CFLAG_LIMIT_GREC))
        {
          apply_limit_check = PAL_TRUE;

          if (igi->igi_limit_except_alist
              && (al = access_list_lookup
                       (LIB_VRF_GET_VR (igi->igi_owning_ivrf),
                        AFI_IP, igi->igi_limit_except_alist)))
            {
              p.family = AF_INET;
              p.prefixlen = IPV4_MAX_BITLEN;
              IPV4_ADDR_COPY (&p.u.prefix4, &pgrp);

              ft = access_list_apply (al, (void_t *) &p);

              switch (ft)
                {
                case FILTER_PERMIT:
                  IGMP_DBG_INFO (igi, DECODE, "%r on %s exempt from"
                                 " Global-Limit(%u), CurrNumGRecs(%u)",
                                 &pgrp, igif->igif_owning_ifp->name,
                                 igi->igi_limit, igi->igi_num_grecs);

                  apply_limit_check = PAL_FALSE;
                  break;

                case FILTER_DENY:
                case FILTER_DYNAMIC:
                case FILTER_NO_MATCH:
                  break;
                }
            }

          if (apply_limit_check
              && igi->igi_num_grecs >= igi->igi_limit)
            {
              IGMP_DBG_INFO (igi, DECODE, "%r on %s exceeds Global"
                             " Limit(%u)", &pgrp,
                             igif->igif_owning_ifp->name,
                             igi->igi_limit);

              ret = IGMP_ERR_IF_GREC_LIMIT_REACHED;
              goto EXIT;
            }
        }

      /* Validate against IF G-Rec Limit violation */
      if (CHECK_FLAG (igif->igif_conf.igifc_cflags,
                      IGMP_IF_CFLAG_LIMIT_GREC))
        {
          apply_limit_check = PAL_TRUE;

          igif_num_grecs = CHECK_FLAG (igif->igif_sflags,
                                       IGMP_IF_SFLAG_SVC_TYPE_L2)
                           && ! CHECK_FLAG (igif->igif_sflags,
                                            IGMP_IF_SFLAG_SVC_TYPE_L3) ?
                           igif->igif_hst_igif->igif_num_grecs:
                           igif->igif_num_grecs;

          if (igif->igif_conf.igifc_limit_except_alist
              && (al = access_list_lookup
                       (LIB_VRF_GET_VR (igi->igi_owning_ivrf),
                        AFI_IP,
                        igif->igif_conf.igifc_limit_except_alist)))
            {
              p.family = AF_INET;
              p.prefixlen = IPV4_MAX_BITLEN;
              IPV4_ADDR_COPY (&p.u.prefix4, &pgrp);

              ft = access_list_apply (al, (void_t *) &p);

              switch (ft)
                {
                case FILTER_PERMIT:
                  IGMP_DBG_INFO (igi, DECODE, "%r on %s exempt from"
                                 " IF-Limit(%u), CurrNumGRecs(%u)",
                                 &pgrp, igif->igif_owning_ifp->name,
                                 igif->igif_conf.igifc_limit,
                                 igif_num_grecs);

                  apply_limit_check = PAL_FALSE;
                  break;

                case FILTER_DENY:
                case FILTER_DYNAMIC:
                case FILTER_NO_MATCH:
                  break;
                }
            }

          if (apply_limit_check
              && igif_num_grecs >=
                 igif->igif_conf.igifc_limit)
            {
              IGMP_DBG_INFO (igi, DECODE, "%r exceeds Limit(%u) on %s",
                             &pgrp, igif->igif_conf.igifc_limit,
                             igif->igif_owning_ifp->name);

              ret = IGMP_ERR_IF_GREC_LIMIT_REACHED;
              goto EXIT;
            }
        }

      /* Obtain the Group Rec */
      igr = igmp_if_grec_get (igif, &pgrp);

      if (! igr)
        {
          IGMP_DBG_INFO (igi, DECODE, "Failed to 'get' Group-Rec!");

          ret = IGMP_ERR_OOM;
          goto EXIT;
        }
    }

  /* Stamp as a Dynamic Group */
  SET_FLAG (igr->igr_sflags, IGMP_IGR_SFLAG_DYNAMIC);

  /* Record last reporting host */
  igr->igr_last_reporter = *psrc;

  /* In case it is an L2 interface, update the igr_last_reporter field
   * in the host side group record also.
   */
  if (INTF_TYPE_L2 (igif->igif_owning_ifp)) 
    {
      ret = igmp_if_hst_grec_update(igif, &pgrp, igr);

      /* Updation will fail the first time, in which case the
       * igr_last_reporter field will be updated in the
       * igmp_if_hst_consolidate_mship(). From the next time the
       * igr_last_reporter field will be updated successfully
       */

      if (ret < IGMP_ERR_NONE)
        IGMP_DBG_INFO (igi, DECODE, "G-Rec not found!"
                       " on %s for %r",
                       igif->igif_owning_ifp->name, &pgrp);
    }

  /* Loose the Sources in TIB-A and TIB-B */
  for (pn = ptree_top (igr->igr_src_a_tib); pn; pn = pn_next)
    {
      pn_next = ptree_next (pn);

      if (! (isr = pn->info))
        continue;

      if (CHECK_FLAG (isr->isr_sflags, IGMP_ISR_SFLAG_STATIC))
        {
          if (CHECK_FLAG (isr->isr_sflags, IGMP_ISR_SFLAG_DYNAMIC))
            UNSET_FLAG (isr->isr_sflags, IGMP_ISR_SFLAG_DYNAMIC);
          continue;
        }

      igmp_if_srec_delete (isr);

      igr->igr_src_a_tib_count -= 1;
    }

  for (pn = ptree_top (igr->igr_src_b_tib); pn; pn = pn_next)
    {
      pn_next = ptree_next (pn);

      if (! (isr = pn->info))
        continue;

      igmp_if_srec_delete (isr);

      igr->igr_src_b_tib_count -= 1;
    }

  /* Reset the V2-Host-Present Timer */
  IGMP_TIMER_OFF (IGMP_LG (igi), igr->t_igr_v2_host_present);
  IGMP_TIMER_ON (IGMP_LG (igi),
                 igr->t_igr_v2_host_present,
                 igr, igmp_if_grec_timer_v2_host_present,
                 igif->igif_gmi);

  /* Record V2 Compatibility Mode */
  igmp_if_grec_update_cmode (igr);

  /* Increment Grp-Joins Counter (G-Rec add activity) */
  igif->igif_num_joins += 1;

  /* Process FSM event */
  ret = igmp_igr_fsm_process_event (igr, rec_type, isl);

  if (ret < IGMP_ERR_NONE)
    {
      IGMP_DBG_INFO (igi, DECODE, "IGR FSM Failed(%d)! on %s for %r",
                     ret, igif->igif_owning_ifp->name, &pgrp);

      goto EXIT;
    }

EXIT:

  if (isl)
    XFREE (MTYPE_IGMP_SRC_LIST, isl);

  /* Error-code correction check-post */
  switch (ret)
    {
    case IGMP_ERR_IF_GREC_LIMIT_REACHED:
    case IGMP_ERR_OOM:
      ret = IGMP_ERR_NONE;
      break;

    default:
      break;
    }

  IGMP_FN_EXIT (ret);
}

/* IGMP V3 Membership Report Group Record Decoder */
s_int32_t
igmp_dec_msg_v3_group_rec (struct igmp_if *igif,
                           struct pal_in4_addr *psrc,
                           struct pal_in4_addr *pdst)
{
  enum igmp_filter_mode_event rec_type;
  struct igmp_source_list *isl;
  struct igmp_group_rec *igr;
  struct igmp_instance *igi;
  struct pal_in4_addr pgrp;
  bool_t apply_limit_check;
  struct pal_in4_addr msrc;
  u_int32_t igif_num_grecs;
  u_int32_t src_list_size;
  struct access_list *al;
  u_int8_t aux_data_len;
  enum filter_type ft;
  u_int16_t num_srcs;
  u_int16_t src_num;
  struct stream *s;
  struct prefix p;
  u_int32_t idx;
  s_int32_t ret;

  IGMP_FN_ENTER (Dec V3 Grp Rec);

  igi = igif->igif_owning_igi;
  ft = FILTER_NO_MATCH;
  ret = IGMP_ERR_NONE;
  isl = NULL;

  /* Get stream buffer */
  s = igi->igi_i_buf;

  /* Decode Record Type */
  rec_type = stream_getc (s);

  switch (rec_type)
    {
    case IGMP_FME_MODE_IS_INCL:
    case IGMP_FME_MODE_IS_EXCL:
    case IGMP_FME_CHG_TO_INCL:
    case IGMP_FME_CHG_TO_EXCL:
    case IGMP_FME_ALLOW_NEW_SRCS:
    case IGMP_FME_BLOCK_OLD_SRCS:
      break;

    case IGMP_FME_GROUP_TIMER_EXPIRY:
    case IGMP_FME_SOURCE_TIMER_EXPIRY:
    case IGMP_FME_INVALID:
    default:
      IGMP_DBG_WARN (igi, DECODE, "Invalid Grp-Record-Type %d!");
      ret = IGMP_ERR_NONE;
      goto EXIT;
    }

  /* Decode Aux Data Length */
  aux_data_len = stream_getc (s);

  /* Decode Num Source Records */
  num_srcs = stream_getw (s);

  /* Decode Group Address */
  pgrp.s_addr = stream_get_ipv4 (s);

  /* Do not process LAN groups */
  if (pal_ntoh32 (pgrp.s_addr) <= INADDR_MAX_LOCAL_GROUP
      || ! IN_MULTICAST (pal_ntoh32 (pgrp.s_addr)))
    {
      IGMP_DBG_INFO (igi, DECODE, "Invalid Grp %r on %s",
                     &pgrp, igif->igif_owning_ifp->name);

      ret = IGMP_ERR_NONE;
      goto EXIT;
    }

  IGMP_DBG_INFO (igi, DECODE, "Grp %r on %s",
                 &pgrp, igif->igif_owning_ifp->name);

  /* Apply the access list if present */
  if (igif->igif_conf.igifc_access_list
      && (al = access_list_lookup
               (LIB_VRF_GET_VR (igi->igi_owning_ivrf),
                AFI_IP, igif->igif_conf.igifc_access_list)))
    {
      p.family = AF_INET;
      p.prefixlen = IPV4_MAX_BITLEN;
      IPV4_ADDR_COPY (&p.u.prefix4, &pgrp);

      ft = access_list_apply (al, (void_t *) &p);

      switch (ft)
        {
        case FILTER_DENY:
          IGMP_DBG_INFO (igi, DECODE, "Grp %r denied on %s by alist %s",
                         &pgrp, igif->igif_owning_ifp->name,
                         igif->igif_conf.igifc_access_list);

          /* Jump over to the next Grp-Rec */
          for (idx = 0; idx < num_srcs; idx += 1)
            stream_get_ipv4 (s);

          if (aux_data_len)
            STREAM_FORWARD_GETP (s, aux_data_len * 4);

          ret = IGMP_ERR_NONE;
          goto EXIT;
          break;

        case FILTER_PERMIT:
        case FILTER_DYNAMIC:
        case FILTER_NO_MATCH:
          break;
        }
    }

  /* Check for SSM-Range Addresses */
  if ((pal_ntoh32 (pgrp.s_addr) & IN_CLASSA_NET) ==
      INADDR_SSM_RANGE_ADDRESS)
    switch (rec_type)
      {
      case IGMP_FME_MODE_IS_EXCL:
      case IGMP_FME_CHG_TO_EXCL:
        IGMP_DBG_INFO (igi, DECODE, "SSM Grp %r on %s, Ignoring"
                       " Grp-Rec-Type %s(%d)", &pgrp,
                       igif->igif_owning_ifp->name,
                       IGMP_FME_STR (rec_type), rec_type);

        /* Jump over to the next Grp-Rec */
        for (idx = 0; idx < num_srcs; idx += 1)
          stream_get_ipv4 (s);

        if (aux_data_len)
          STREAM_FORWARD_GETP (s, aux_data_len * 4);

        ret = IGMP_ERR_NONE;
        goto EXIT;
        break;

      default:
        break;
      }

  /* Lookup the G-Rec for Imm-Leave and Limit processing */
  igr = igmp_if_grec_lookup (igif, &pgrp);
   
  if (igr != NULL)
    {
      /*According to RFC 3376 section 7.3.2 ignoring BLOCK, TO_EX and TO_IN
       * messages in v1 compatability mode
       */ 
      if (CHECK_FLAG (igr->igr_sflags, IGMP_IGR_SFLAG_COMPAT_V1) 
         && (rec_type == IGMP_FME_BLOCK_OLD_SRCS 
            || rec_type == IGMP_FME_CHG_TO_EXCL 
            || rec_type == IGMP_FME_CHG_TO_INCL 
            || rec_type == IGMP_FME_IMMEDIATE_LEAVE))
        {
          IGMP_DBG_INFO (igi, DECODE, "Ignoring BLOCK, TO_EX and TO_IN "
                                      "in v1 compatability mode %r", &pgrp);
          ret = IGMP_ERR_NONE;
          goto EXIT;
        }
      
      /*According to RFC 3376 section 7.3.2 ignoring BLOCK and  TO_EX
       * messages in v2 compatability mode
       */  
      if (CHECK_FLAG (igr->igr_sflags, IGMP_IGR_SFLAG_COMPAT_V2) 
         && ( rec_type == IGMP_FME_BLOCK_OLD_SRCS 
            || rec_type == IGMP_FME_CHG_TO_EXCL ))
        {
          IGMP_DBG_INFO (igi, DECODE, "Ignoring the BLOCK and "
                         "TO_EX in v2 compatability mode %r ", &pgrp);
          ret = IGMP_ERR_NONE;
          goto EXIT;
        }
    }

  /* Apply the Fast Leave/ immediate-leave access list if present */
  if (! num_srcs
      && rec_type == IGMP_FME_CHG_TO_INCL)
    {
      if (! igr)
        {
          IGMP_DBG_INFO (igi, DECODE, "No such Group-Rec(%r)!", &pgrp);

          ret = IGMP_ERR_NONE;
          goto EXIT;
        }

      if (CHECK_FLAG (igr->igr_sflags, IGMP_IGR_SFLAG_COMPAT_V2))
        {
        if (CHECK_FLAG (igif->igif_conf.igifc_cflags,
                        IGMP_IF_CFLAG_SNOOP_FAST_LEAVE))
          {
            IGMP_DBG_INFO (igi, DECODE, "Fast Leave on %s for %r",
                           igif->igif_owning_ifp->name, &pgrp);

            ret = igmp_igr_fsm_process_event (igr,
                                              IGMP_FME_IMMEDIATE_LEAVE, NULL);
            if (ret < IGMP_ERR_NONE)
              IGMP_DBG_INFO (igi, DECODE, "IGR FSM Failed(%d)!"
                             " on %s for %r", ret,
                             igif->igif_owning_ifp->name, &pgrp);
            goto EXIT;
          }
        else if (igif->igif_conf.igifc_immediate_leave
                 && (al = access_list_lookup
                          (LIB_VRF_GET_VR (igi->igi_owning_ivrf),
                           AFI_IP, igif->igif_conf.igifc_immediate_leave)))
          {
            pal_mem_set (&p, 0, sizeof (struct prefix));
            p.family = AF_INET;
            p.prefixlen = IPV4_MAX_BITLEN;
            IPV4_ADDR_COPY (&p.u.prefix4, &pgrp);

            ft = access_list_apply (al, (void_t *) &p);

            switch (ft)
              {
              case FILTER_PERMIT:
                IGMP_DBG_INFO (igi, DECODE, "Imm-leave alist %s on %s for %r",
                               igif->igif_conf.igifc_immediate_leave,
                               igif->igif_owning_ifp->name, &pgrp);

                ret = igmp_igr_fsm_process_event
                      (igr, IGMP_FME_IMMEDIATE_LEAVE, NULL);

                if (ret < IGMP_ERR_NONE)
                  IGMP_DBG_INFO (igi, DECODE, "IGR FSM Failed(%d)!"
                                 " on %s for %r", ret,
                                 igif->igif_owning_ifp->name, &pgrp);

                goto EXIT;
                break;

              case FILTER_DENY:
              case FILTER_DYNAMIC:
              case FILTER_NO_MATCH:
                break;
              }
          }
      }
    }
  /* Validate against Global G-Rec Limit violation */
  if (! igr)
    {
      if (CHECK_FLAG (igi->igi_cflags, IGMP_INST_CFLAG_LIMIT_GREC))
        {
          apply_limit_check = PAL_TRUE;

          if (igi->igi_limit_except_alist
              && (al = access_list_lookup
                       (LIB_VRF_GET_VR (igi->igi_owning_ivrf),
                        AFI_IP, igi->igi_limit_except_alist)))
            {
              p.family = AF_INET;
              p.prefixlen = IPV4_MAX_BITLEN;
              IPV4_ADDR_COPY (&p.u.prefix4, &pgrp);

              ft = access_list_apply (al, (void_t *) &p);

              switch (ft)
                {
                case FILTER_PERMIT:
                  IGMP_DBG_INFO (igi, DECODE, "%r on %s exempt from"
                                 " Global-Limit(%u), CurrNumGRecs(%u)",
                                 &pgrp, igif->igif_owning_ifp->name,
                                 igi->igi_limit, igi->igi_num_grecs);

                  apply_limit_check = PAL_FALSE;
                  break;

                case FILTER_DENY:
                case FILTER_DYNAMIC:
                case FILTER_NO_MATCH:
                  break;
                }
            }

          if (apply_limit_check
              && igi->igi_num_grecs >= igi->igi_limit)
            {
              IGMP_DBG_INFO (igi, DECODE, "%r on %s exceeds Global"
                             " Limit(%u)", &pgrp,
                             igif->igif_owning_ifp->name,
                             igi->igi_limit);

              ret = IGMP_ERR_IF_GREC_LIMIT_REACHED;
              goto EXIT;
            }
        }

      /* Validate against IF G-Rec Limit violation */
      if (CHECK_FLAG (igif->igif_conf.igifc_cflags,
                      IGMP_IF_CFLAG_LIMIT_GREC))
        {
          apply_limit_check = PAL_TRUE;

          igif_num_grecs = CHECK_FLAG (igif->igif_sflags,
                                       IGMP_IF_SFLAG_SVC_TYPE_L2)
                           && ! CHECK_FLAG (igif->igif_sflags,
                                            IGMP_IF_SFLAG_SVC_TYPE_L3) ?
                           igif->igif_hst_igif->igif_num_grecs:
                           igif->igif_num_grecs;

          if (igif->igif_conf.igifc_limit_except_alist
              && (al = access_list_lookup
                       (LIB_VRF_GET_VR (igi->igi_owning_ivrf),
                        AFI_IP,
                        igif->igif_conf.igifc_limit_except_alist)))
            {
              p.family = AF_INET;
              p.prefixlen = IPV4_MAX_BITLEN;
              IPV4_ADDR_COPY (&p.u.prefix4, &pgrp);

              ft = access_list_apply (al, (void_t *) &p);

              switch (ft)
                {
                case FILTER_PERMIT:
                  IGMP_DBG_INFO (igi, DECODE, "%r on %s exempt from"
                                 " IF-Limit(%u), CurrNumGRecs(%u)",
                                 &pgrp, igif->igif_owning_ifp->name,
                                 igif->igif_conf.igifc_limit,
                                 igif_num_grecs);

                  apply_limit_check = PAL_FALSE;
                  break;

                case FILTER_DENY:
                case FILTER_DYNAMIC:
                case FILTER_NO_MATCH:
                  break;
                }
            }

          if (apply_limit_check
              && igif_num_grecs >=
                 igif->igif_conf.igifc_limit)
            {
              IGMP_DBG_INFO (igi, DECODE, "%r exceeds Limit(%u) on %s",
                             &pgrp, igif->igif_conf.igifc_limit,
                             igif->igif_owning_ifp->name);

              ret = IGMP_ERR_IF_GREC_LIMIT_REACHED;
              goto EXIT;
            }
        }

      /* Obtain the Group Rec */
      igr = igmp_if_grec_get (igif, &pgrp);

      if (! igr)
        {
          IGMP_DBG_INFO (igi, DECODE, "Failed to 'get' Group-Rec!");

          ret = IGMP_ERR_OOM;
          goto EXIT;
        }
    }

  /* Stamp as a Dynamic Group */
  SET_FLAG (igr->igr_sflags, IGMP_IGR_SFLAG_DYNAMIC);

  /* Record last reporting host */
  igr->igr_last_reporter = *psrc;

  /* In case it is an L2 interface, update the igr_last_reporter field 
   * in the host side group record also.
   */
  if (INTF_TYPE_L2 (igif->igif_owning_ifp)) 
    {
      ret = igmp_if_hst_grec_update(igif, &pgrp, igr);

      /* Updation will fail the first time, in which case the
       * igr_last_reporter field will be updated in the
       * igmp_if_hst_consolidate_mship(). From the next time the
       * igr_last_reporter field will be updated successfully
       */

      if (ret < IGMP_ERR_NONE)
        IGMP_DBG_INFO (igi, DECODE, "G-Rec not found!"
                       " on %s for %r",
                       igif->igif_owning_ifp->name, &pgrp);
    }

  /* Record V3 Compatibility Mode */
  igmp_if_grec_update_cmode (igr);

  /* Decode the Sources List */
  src_list_size = sizeof (struct igmp_source_list) +
                  sizeof (struct pal_in4_addr) * num_srcs;

  isl = XCALLOC (MTYPE_IGMP_SRC_LIST, src_list_size);

  if (! isl)
    {
      IGMP_DBG_ERR (igi, DECODE, "Cannot allocate memory"
                    " (%d) @ %s:%d", src_list_size,
                    __FILE__, __LINE__);

      ret = IGMP_ERR_OOM;
      goto EXIT;
    }

  isl->isl_num = 0;
  idx = 0;

  for (src_num = 0; src_num < num_srcs; src_num++)
    {
      msrc.s_addr = stream_get_ipv4 (s);

      if (IN_EXPERIMENTAL (pal_ntoh32 (msrc.s_addr))
          || IN_MULTICAST (pal_ntoh32 (msrc.s_addr))
          || IPV4_ADDR_MARTIAN (pal_ntoh32 (msrc.s_addr)))
        {
          IGMP_DBG_WARN (igi, DECODE, "Invalid M-Src Addr %r", &msrc);

          continue;
        }

      IPV4_ADDR_COPY (&isl->isl_saddr [idx], &msrc);
      isl->isl_num += 1;
      idx += 1;
    }

  SET_FLAG (igr->igr_sflags, IGMP_IGR_SFLAG_DYNAMIC);

  if (aux_data_len)
    {
      IGMP_DBG_INFO (igi, DECODE, "Ignoring Aux Data of Len %d 32-bit"
                     " words", aux_data_len);

      STREAM_FORWARD_GETP (s, aux_data_len * IPV4_MAX_BYTELEN);
    }

  /* Update Joins/Leaves Counters */
  switch (rec_type)
    {
    case IGMP_FME_MODE_IS_INCL:
    case IGMP_FME_CHG_TO_INCL:
      if (! isl->isl_num)
        igif->igif_num_leaves += 1;
      else
        igif->igif_num_joins += 1;
      break;

    case IGMP_FME_MODE_IS_EXCL:
    case IGMP_FME_CHG_TO_EXCL:
      if (! isl->isl_num)
        igif->igif_num_joins += 1;
      else
        igif->igif_num_leaves += 1;
      break;

    case IGMP_FME_ALLOW_NEW_SRCS:
      igif->igif_num_joins += 1;
      break;

    case IGMP_FME_BLOCK_OLD_SRCS:
      igif->igif_num_leaves += 1;
      break;

    case IGMP_FME_GROUP_TIMER_EXPIRY:
    case IGMP_FME_SOURCE_TIMER_EXPIRY:
    case IGMP_FME_INVALID:
    default:
      IGMP_DBG_WARN (igi, DECODE, "Invalid Grp-Record-Type %d!", rec_type);
      ret = IGMP_ERR_NONE;
      goto EXIT;
    }

  /* Process FSM event */
  ret = igmp_igr_fsm_process_event (igr, rec_type, isl);

  if (ret < IGMP_ERR_NONE)
    {
      IGMP_DBG_INFO (igi, DECODE, "IGR FSM Failed(%d)! on %s for %r",
                     ret, igif->igif_owning_ifp->name, &pgrp);

      goto EXIT;
    }

EXIT:

  if (isl)
    XFREE (MTYPE_IGMP_SRC_LIST, isl);

  /* Error-code correction check-post */
  switch (ret)
    {
    case IGMP_ERR_IF_GREC_LIMIT_REACHED:
    case IGMP_ERR_OOM:
      ret = IGMP_ERR_NONE;
      break;

    default:
      break;
    }

  IGMP_FN_EXIT (ret);
}

/* IGMP V3 Membership Report Message Decoder */
s_int32_t
igmp_dec_msg_v3_report (struct igmp_if *igif,
                        struct pal_in4_addr *psrc,
                        struct pal_in4_addr *pdst)
{
  struct igmp_instance *igi;
  u_int16_t num_grps;
  struct stream *s;
  s_int32_t ret;

  IGMP_FN_ENTER (Dec V3 Report);

  igi = igif->igif_owning_igi;
  ret = IGMP_ERR_NONE;
  num_grps = 0;

  /* Ignore Report if in IGMP Proxy Service mode */
  if (CHECK_FLAG (igif->igif_conf.igifc_cflags,
                  IGMP_IF_CFLAG_PROXY_SERVICE))
    {
      IGMP_DBG_INFO (igi, DECODE, "%s in Proxy-service mode, Ignoring",
                     igif->igif_owning_ifp->name);

      ret = IGMP_ERR_NONE;
      goto EXIT;
    }

  /* Ignoring higher version reports*/
  switch (igif->igif_conf.igifc_version)
    {
    case IGMP_VERSION_1:
    case IGMP_VERSION_2:
      IGMP_DBG_INFO (igi, DECODE, "V3 report on interface %s configured"
                     " for %d, ignored ", igif->igif_owning_ifp->name,
                     igif->igif_conf.igifc_version);
      goto EXIT;
      break;

    default:
      break;
    }

  /* Get stream buffer */
  s = igi->igi_i_buf;

  /* Decode and ignore Resvd word */
  stream_getw (s);

  /* Decode Num Group Records */
  num_grps = stream_getw (s);

  /* Message Length Check */
  if (STREAM_DATA_REMAIN (s) < num_grps *
      IGMP_MSG_V3_REPORT_GRP_REC_MIN_SIZE)
    {
      IGMP_DBG_INFO (igi, DECODE, "Data size < Min size for %d Grps",
                     num_grps);

      ret = IGMP_ERR_NONE;
      goto EXIT;
    }

  while (num_grps--)
    {
      ret = igmp_dec_msg_v3_group_rec (igif, psrc, pdst);

      if (ret < IGMP_ERR_NONE)
        IGMP_DBG_INFO (igi, DECODE, "Grp-Rec Decode Failed(%d) on %s",
                       ret, igif->igif_owning_ifp->name);
    }

EXIT:

  IGMP_FN_EXIT (ret);
}

/* IGMP V2 Leave Group Message Decoder */
s_int32_t
igmp_dec_msg_v2_leave_group (struct igmp_if *igif,
                             struct pal_in4_addr *psrc,
                             struct pal_in4_addr *pdst)
{
  struct igmp_ssm_map_static *igssms;
  struct igmp_group_rec *igr;
  struct igmp_instance *igi;
  struct pal_in4_addr pgrp;
  struct access_list *al;
  struct listnode *nn;
  enum filter_type ft;
  bool_t ssm_mapped;
  struct stream *s;
  struct prefix p;
  s_int32_t ret;

  IGMP_FN_ENTER (Dec V2 Leave);

  igi = igif->igif_owning_igi;
  ssm_mapped = PAL_FALSE;
  ret = IGMP_ERR_NONE;
  al = NULL;

  /* Ignore Leave Msg if in IGMP Proxy Service mode */
  if (CHECK_FLAG (igif->igif_conf.igifc_cflags,
                  IGMP_IF_CFLAG_PROXY_SERVICE))
    {
      IGMP_DBG_INFO (igi, DECODE, "%s in Proxy-service mode, Ignoring",
                     igif->igif_owning_ifp->name);

      ret = IGMP_ERR_NONE;
      goto EXIT;
    }

  /* Ignoring higher version leave */
  switch (igif->igif_conf.igifc_version)
    {
    case IGMP_VERSION_1:
      IGMP_DBG_INFO (igi, DECODE, "V2 leave on interface %s configured"
                     " for %d, ignored", igif->igif_owning_ifp->name,
                     igif->igif_conf.igifc_version);

      goto EXIT;
      break;

    default:
      break;
    }

  /* Get stream buffer */
  s = igi->igi_i_buf;

  /* Decode Group Address */
  pgrp.s_addr = stream_get_ipv4 (s);

  /* Do not process LAN groups */
  if (pal_ntoh32 (pgrp.s_addr) <= INADDR_MAX_LOCAL_GROUP
      || ! IN_MULTICAST (pal_ntoh32 (pgrp.s_addr)))
    {
      IGMP_DBG_WARN (igi, DECODE, "Invalid Grp %r on %s",
                     &pgrp, igif->igif_owning_ifp->name);

      ret = IGMP_ERR_NONE;
      goto EXIT;
    }

  /* SSM-Map processing */
  if (! CHECK_FLAG (igi->igi_cflags, IGMP_INST_CFLAG_SSM_MAP_DISABLED)
      && (pal_ntoh32 (pgrp.s_addr) & IN_CLASSA_NET) ==
          INADDR_SSM_RANGE_ADDRESS)
    {
      /* Static SSM-Map processing */
      if (CHECK_FLAG (igi->igi_cflags, IGMP_INST_CFLAG_SSM_MAP_STATIC))
        LIST_LOOP (&igi->igi_ssm_map_static_lst, igssms, nn)
          {
            /* Apply the access list if present */
            if (! igssms->igssms_grp_alist
                || ! (al = access_list_lookup
                           (LIB_VRF_GET_VR (igi->igi_owning_ivrf),
                            AFI_IP, igssms->igssms_grp_alist)))
              continue;

            p.family = AF_INET;
            p.prefixlen = IPV4_MAX_BITLEN;
            IPV4_ADDR_COPY (&p.u.prefix4, &pgrp);

            ft = access_list_apply (al, (void_t *) &p);

            switch (ft)
              {
              case FILTER_PERMIT:
                IGMP_DBG_INFO (igi, DECODE, "Grp %r on %s matched"
                               " SSM-Map alist %s, MSrc %r",
                               &pgrp, igif->igif_owning_ifp->name,
                               igssms->igssms_grp_alist,
                               &igssms->igssms_msrc);

                ssm_mapped = PAL_TRUE;
                break;

              case FILTER_DENY:
              case FILTER_DYNAMIC:
              case FILTER_NO_MATCH:
                break;
              }

            if (ssm_mapped)
              break;
          }
    }

  /* Ignore V2-Leave for a group address in SSM range*/
  if (! ssm_mapped
      && igif->igif_conf.igifc_version == IGMP_VERSION_3
      && (pal_ntoh32 (pgrp.s_addr) & IN_CLASSA_NET) ==
         INADDR_SSM_RANGE_ADDRESS)
    {
      IGMP_DBG_WARN (igi, DECODE, "SSM Grp %r on %s, Ignoring",
                     &pgrp, igif->igif_owning_ifp->name);

      ret = IGMP_ERR_NONE;
      goto EXIT;
    }

  IGMP_DBG_INFO (igi, DECODE, "Grp %r on %s",
                 &pgrp, igif->igif_owning_ifp->name);

  /* Lookup the Group Rec */
  igr = igmp_if_grec_lookup (igif, &pgrp);

  if (! igr)
    {
      IGMP_DBG_INFO (igi, DECODE, "No such Group-Rec(%r)!", &pgrp);

      ret = IGMP_ERR_NONE;
      goto EXIT;
    }

  /* Apply the Fast Leave/ immediate-leave access list if present */
  if (CHECK_FLAG (igr->igr_sflags, IGMP_IGR_SFLAG_COMPAT_V2))
    {
    if (CHECK_FLAG (igif->igif_conf.igifc_cflags,
                    IGMP_IF_CFLAG_SNOOP_FAST_LEAVE))
      {
        IGMP_DBG_INFO (igi, DECODE, "Fast Leave on %s for %r",
                       igif->igif_owning_ifp->name, &pgrp);

        ret = igmp_igr_fsm_process_event (igr,
                                          IGMP_FME_IMMEDIATE_LEAVE, NULL);
        if (ret < IGMP_ERR_NONE)
          IGMP_DBG_INFO (igi, DECODE, "IGR FSM Failed(%d)!"
                         " on %s for %r", ret,
                         igif->igif_owning_ifp->name, &pgrp);
        goto EXIT;
      }
    else if ((igif->igif_conf.igifc_immediate_leave)
              && (al = access_list_lookup
                       (LIB_VRF_GET_VR (igi->igi_owning_ivrf),
                        AFI_IP, igif->igif_conf.igifc_immediate_leave)))
      {
        pal_mem_set (&p, 0, sizeof (struct prefix));
        p.family = AF_INET;
        p.prefixlen = IPV4_MAX_BITLEN;
        IPV4_ADDR_COPY (&p.u.prefix4, &pgrp);

        ft = access_list_apply (al, (void_t *) &p);

        switch (ft)
          {
          case FILTER_PERMIT:
            IGMP_DBG_INFO (igi, DECODE, "Imm-leave alist %s on %s for %r",
                           igif->igif_conf.igifc_immediate_leave,
                           igif->igif_owning_ifp->name, &pgrp);

            ret = igmp_igr_fsm_process_event
                  (igr, IGMP_FME_IMMEDIATE_LEAVE, NULL);

            if (ret < IGMP_ERR_NONE)
              IGMP_DBG_INFO (igi, DECODE, "IGR FSM Failed(%d)! on %s for %r",
                             ret, igif->igif_owning_ifp->name, &pgrp);

            goto EXIT;
            break;

          case FILTER_DENY:
          case FILTER_DYNAMIC:
          case FILTER_NO_MATCH:
            break;
          }
      }
    }
  /* Discard the leave messages in the Version1 Members Present state */
  if (CHECK_FLAG (igr->igr_sflags, IGMP_IGR_SFLAG_COMPAT_V1))
    goto EXIT;

  /* Increment Grp-Leaves Counter (G-Rec leave activity) */
  igif->igif_num_leaves += 1;

  /* Process FSM event */
  ret = igmp_igr_fsm_process_event (igr, IGMP_FME_CHG_TO_INCL, NULL);

  if (ret < IGMP_ERR_NONE)
    {
      IGMP_DBG_INFO (igi, DECODE, "IGR FSM Failed(%d)! on %s for %r",
                     ret, igif->igif_owning_ifp->name, &pgrp);

      goto EXIT;
    }

EXIT:

  IGMP_FN_EXIT (ret);
}

/* IGMP Message Decoder */
s_int32_t
igmp_dec_msg (struct igmp_if *igif,
              struct pal_in4_addr *psrc,
              struct pal_in4_addr *pdst,
              u_int32_t msg_len)
{
  struct igmp_instance *igi;
  u_int8_t max_rsp_code;
  u_int16_t calc_csum;
  u_int16_t rcvd_csum;
  u_int8_t msg_type;
  struct stream *s;
  struct prefix p;
  struct rib *rib;
  struct nsm_ptree_node *rn = NULL;
  s_int32_t ret;

  IGMP_FN_ENTER (Dec Msg);

  ret = IGMP_ERR_NONE;

  igi = igif->igif_owning_igi;

  /* Get stream buffer */
  s = igi->igi_i_buf;

  /* Decode Msg Type */
  msg_type = stream_getc (s);

  /* Msg length must be bigger than IGMP Header size */
  if (msg_len < IGMP_MSG_MIN_LEN)
    {
      IGMP_DBG_INFO (igi, DECODE, "Msg len(%d) < IGMP min len(%d)",
                     msg_len, IGMP_MSG_MIN_LEN);

      ret = IGMP_ERR_NONE;
      goto EXIT;
    }

  p.family = AF_INET;
  p.prefixlen = IPV4_MAX_PREFIXLEN;
  IPV4_ADDR_COPY (&p.u.prefix4, psrc);

  if (CHECK_FLAG (igif->igif_conf.igifc_cflags, IGMP_OFFLINK_IF_CFLAG_CONFIG_ENABLED)
      && (CHECK_FLAG (igif->igif_sflags, IGMP_IF_SFLAG_SVC_TYPE_L3)
      && ! if_match_ifc_ipv4_direct (igif->igif_owning_ifp, &p)))
    {/*if enabled igmp offlink, even if the source 
     is not part of the local subnet,as long as it passes an RPF check,
     then IGMP need accept and record this subscriber.*/
     
     if(msg_type == IGMP_MSG_MEMBERSHIP_QUERY
       || msg_type == IGMP_MSG_V2_LEAVE_GROUP)
      {
       IGMP_DBG_WARN (igi, DECODE, "Non-local Source %r on %s, msg Type=0x%x, Ignoring",
                     psrc, igif->igif_owning_ifp->name, msg_type);

       ret = IGMP_ERR_NONE;
       goto EXIT;
      }

     /*RPF check the source*/
     rib = nsm_rib_match (igi->igi_owning_ivrf->proto, AFI_IP, &p, &rn, 0, 0);
     
     IGMP_DBG_WARN (igi, DECODE,"route: %r/%u", PTREE_NODE_KEY(rn), rn->key_len);
     
     if(!rib || 0 == rn->key_len)
      {
       IGMP_DBG_WARN (igi, DECODE, "Non-local Source %r on %s, msg Type=0x%x, RPF check failed, Ignoring",
                     psrc, igif->igif_owning_ifp->name, msg_type);

       ret = IGMP_ERR_NONE;
       goto EXIT;
      }
     
     IGMP_DBG_WARN (igi, DECODE, "Non-local Source %r on %s, msg Type=0x%x, Don't ignore for offlink igmp",
                     psrc, igif->igif_owning_ifp->name, msg_type);

    }
  else if (CHECK_FLAG (igif->igif_sflags, IGMP_IF_SFLAG_SVC_TYPE_L3)
      && ! if_match_ifc_ipv4_direct (igif->igif_owning_ifp, &p))
    {/* Ignore packets not belonging to local subnet */
      IGMP_DBG_WARN (igi, DECODE, "Non-local Source %r on %s, Ignoring",
                     psrc, igif->igif_owning_ifp->name);

      ret = IGMP_ERR_NONE;
      goto EXIT;
    }

  /* Ignore all unknown messages */
  switch (msg_type)
    {
    case IGMP_MSG_MEMBERSHIP_QUERY:
    case IGMP_MSG_V1_MEMBERSHIP_REPORT:
    case IGMP_MSG_V2_MEMBERSHIP_REPORT:
    case IGMP_MSG_V3_MEMBERSHIP_REPORT:
    case IGMP_MSG_V2_LEAVE_GROUP:
      break;

    default:
      IGMP_DBG_INFO (igi, DECODE, "Unknown msg Type=%d, Ignoring Msg",
                     msg_type);
      ret = IGMP_ERR_NONE;
      goto EXIT;
      break;
    }

  /* Decode Max. Response Code */
  max_rsp_code = stream_getc (s);

  /* Decode and Reset Checksum field */
  stream_get_only (&rcvd_csum, s, sizeof (rcvd_csum));
  stream_putw_at (s, STREAM_GET_GETP (s), 0);
  STREAM_FORWARD_GETP (s, sizeof (u_int16_t));

  /* Rewind back to start of IGMP Mesg */
  STREAM_REWIND_GETP (s, IGMP_MSG_HEADER_SIZE);

  /* Calculate Checksum */
  calc_csum = in_checksum ((u_int16_t *) STREAM_PNT (s), msg_len);

  /* Validate Checksum */
  if (calc_csum != rcvd_csum)
    {
      IGMP_DBG_INFO (igi, DECODE, "Checksum error. Rcvd=%u Calc=%u,"
                    " Ignoring %s message", rcvd_csum,
                    calc_csum, IGMP_MSG_TYPE_STR (msg_type));

      ret = IGMP_ERR_NONE;
      goto EXIT;
    }

  /* Forward 'getp' to Checksum field.*/
  STREAM_FORWARD_GETP (s, sizeof (u_int8_t));
  STREAM_FORWARD_GETP (s, sizeof (u_int8_t));

  /* Put the Checksum back */
  stream_putw_at (s, STREAM_GET_GETP (s), pal_ntoh16 (rcvd_csum));

  /* Forward 'getp' and 'putp' to start of IGMP Msg PDU */
  STREAM_FORWARD_GETP (s, sizeof (u_int16_t));
  STREAM_FORWARD_PUTP (s, IGMP_MSG_HEADER_SIZE);

  IGMP_DBG_INFO (igi, DECODE, "%s, Max. Rsp. Code %d",
                 IGMP_MSG_TYPE_STR (msg_type), max_rsp_code);

  /* Decode the remaining portion of the messages */
  switch (msg_type)
    {
    case IGMP_MSG_MEMBERSHIP_QUERY:
      ret = igmp_dec_msg_query (igif, max_rsp_code, psrc, pdst);
      break;

    case IGMP_MSG_V1_MEMBERSHIP_REPORT:
      ret = igmp_dec_msg_v1_report (igif, psrc, pdst);
      break;

    case IGMP_MSG_V2_MEMBERSHIP_REPORT:
      ret = igmp_dec_msg_v2_report (igif, psrc, pdst);
      break;

    case IGMP_MSG_V3_MEMBERSHIP_REPORT:
      ret = igmp_dec_msg_v3_report (igif, psrc, pdst);
      break;

    case IGMP_MSG_V2_LEAVE_GROUP:
      ret = igmp_dec_msg_v2_leave_group (igif, psrc, pdst);
      break;
    }

EXIT:

  IGMP_FN_EXIT (ret);
}

/* IGMP Socket Read thread */
s_int32_t
igmp_sock_read (struct thread *t_read)
{
  struct mtrace_hdr_igmp *mtrigh;
  struct igmp_in4_header *igiph;
  struct igmp_if_idx igifidx;
  struct igmp_instance *igi;
  struct igmp_svc_reg *igsr;
  union igmp_in4_cmsg cmsg;
  pal_sock_handle_t sock;
  pal_sock_len_t cmsglen;
  struct interface *ifp;
  struct igmp_if *igif;
  s_int32_t msg_type;
  u_int16_t iph_len;
  s_int32_t ip_len;
  s_int32_t msg_len = 0;
  s_int32_t ret;

  IGMP_FN_ENTER (Socket Read);

  ret = IGMP_ERR_NONE;
  igsr = NULL;
  igi = NULL;

  /* Extract Socket FD */
  sock = THREAD_FD (t_read);

  if (sock <= IGMP_SOCK_FD_INVALID)
    {
      ret = IGMP_ERR_MALFORMED_ARG;
      goto EXIT;
    }

  /* Obtain the IGMP Svc Registration */
  igsr = THREAD_ARG (t_read);

  if (! igsr)
    {
      ret = IGMP_ERR_MALFORMED_ARG;
      goto EXIT;
    }

  /* Obtain the IGMP Instance */
  igi = igsr->igsr_owning_igi;

  if (! igi)
    {
      ret = IGMP_ERR_MALFORMED_ARG;
      goto EXIT;
    }

  /* Sanity Check */
  if (sock != igsr->igsr_sock)
    {
      IGMP_DBG_WARN (igi, DECODE, "Sock(%u) != Svc-Reg Sock(%u)",
                     sock, igsr->igsr_sock);

      ret = IGMP_ERR_NONE;
      goto EXIT;
    }

  /* Reset the Read thread */
  igsr->t_igsr_read = NULL;

  /* Reset input buffer */
  stream_reset (igi->igi_i_buf);

  /* Obtain header and packet lengths */
  igiph = (struct igmp_in4_header *) STREAM_PUT_PNT (igi->igi_i_buf);

  /* Peek at least IPv4 Header + IGMP header */
  ret = pal_sock_recvfrom (sock, (void *)igiph,
                           sizeof (struct igmp_in4_header) +
                           sizeof (struct mtrace_hdr_igmp),
                           MSG_PEEK, NULL, NULL);

  if (ret < 0)
    {
      ret = IGMP_ERR_NONE;
      goto EXIT;
    }

  msg_len = ret;

  if (ret < sizeof (struct pal_in4_header))
    {
      IGMP_DBG_INFO (igi, DECODE, "Msg Len(%d) < IP Hdr(%u)",
                     ret, sizeof (struct pal_in4_header));

      ret = IGMP_ERR_SOCK_MSG_PEEK;
      goto EXIT;
    }

  /* Derive header and packet lengths */
  pal_in4_ip_hdr_len_get (&igiph->iph, &iph_len);
  if (igsr->igsr_svc_type == IGMP_SVC_TYPE_L3)
    pal_in4_ip_packet_len_get (&igiph->iph, (u_int32_t *)&ip_len);
  else
    ip_len = pal_ntoh16 (igiph->iph.ip_len);
  
  msg_len = ip_len;

  /* Ignore Msg If IP_PROTO != IGMP */
  if (igiph->iph.ip_p != IPPROTO_IGMP)
    {
      IGMP_DBG_INFO (igi, DECODE, "Msg protocol (%d) not IGMP",
                     igiph->iph.ip_p);

      ret = IGMP_ERR_NONE;
      goto EXIT;
    }
  
  /*Ignore the message if ttl field is not 1 */
  if (igiph->iph.ip_ttl != 1)
    {
      IGMP_DBG_INFO (igi, DECODE, "ttl field (%d)",
                     igiph->iph.ip_ttl);
      
      ret = IGMP_ERR_SOCK_MSG_PEEK;
      goto EXIT;
    }

  /* Ignore MTrace Frames */
  if (ret > PAL_IPLEN_WORDS2BYTES (iph_len))
    {
      mtrigh = (struct mtrace_hdr_igmp *)
               ((u_int8_t *) igiph +
                PAL_IPLEN_WORDS2BYTES (iph_len));

      if (mtrigh->igmp_type == MTRACE_QUERY_REQUEST
          || mtrigh->igmp_type == MTRACE_RESPONSE
          || mtrigh->igmp_type == IGMP_MSG_DVMRP)
        {
          IGMP_DBG_INFO (igi, DECODE, "Ignoring MRACE Frame Type(%d)",
                         mtrigh->igmp_type);

          ret = IGMP_ERR_NONE;
          goto EXIT;
        }
    }

  /* Validate Frame Length to be within limits */
  if (ip_len > STREAM_SIZE (igi->igi_i_buf))
    {
      IGMP_DBG_INFO (igi, DECODE, "Tot. IP Msg Len(%d) too huge!",
                     ip_len);

      ret = IGMP_ERR_SOCK_MSG_PEEK;/*Read data from socket as we cant handle*/
      goto EXIT;
    }

  /* Read the IGMP Message from Socket */
  pal_mem_set (&cmsg, 0, sizeof (union igmp_in4_cmsg));
  cmsglen = sizeof (union igmp_in4_cmsg);

  switch (igsr->igsr_svc_type)
    {
    case IGMP_SVC_TYPE_L2:
      ip_len = pal_sock_recvfrom (sock, STREAM_PNT (igi->igi_i_buf),
                                  ip_len, 0, &cmsg.sa,
                                  (pal_socklen_t *) &cmsglen);

      if (ip_len < 0)
        {
          IGMP_DBG_INFO (igi, DECODE, "recvfrom() error %s(%d)",
                         pal_strerror (errno), errno);

          ret = IGMP_ERR_NONE;
          goto EXIT;
        }

      if (cmsglen < sizeof (cmsg.vaddr))
        {
          IGMP_DBG_INFO (igi, DECODE, "Invalid cmsglen(%u)", cmsglen);

          ret = IGMP_ERR_NONE;
          goto EXIT;
        }

      ifp = ifv_lookup_by_index (&igi->igi_owning_ivrf->ifv,
                                 cmsg.vaddr.port);

      if (! ifp)
        {
          IGMP_DBG_INFO (igi, DECODE, "No interface for ifindex %d",
                         cmsg.vaddr.port);

          ret = IGMP_ERR_NONE;
          goto EXIT;
        }

      igifidx.igifidx_ifp = ifp;
      igifidx.igifidx_sid = igmp_if_snp_svc_reg_su_id (igi, ifp,
                                                       cmsg.vaddr.vlanid);
      break;

    case IGMP_SVC_TYPE_L3:
      ret = pal_in4_recv_packet_len (sock, &igiph->iph,
                                     STREAM_PNT (igi->igi_i_buf),
                                     ip_len, &cmsg.cdm);

      if (ret < 0)
        {
          IGMP_DBG_INFO (igi, DECODE, "recvmsg() error %s(%d)",
                         pal_strerror (errno), errno);

          ret = IGMP_ERR_NONE;
          goto EXIT;
        }

      if (cmsg.cdm.cinfo == PAL_RECVIF_INDEX)
        {
          ifp = ifv_lookup_by_index (&igi->igi_owning_ivrf->ifv,
                                     cmsg.cdm.data.ifindex);

          if (! ifp)
            {
              IGMP_DBG_INFO (igi, DECODE, "No interface for ifindex %d",
                             cmsg.cdm.data.ifindex);

              ret = IGMP_ERR_NONE;
              goto EXIT;
            }
        }
      else
        {
          ifp = if_match_by_ipv4_address (LIB_VR_GET_IF_MASTER
                                          (LIB_VRF_GET_VR
                                           (igi->igi_owning_ivrf)),
                                          &cmsg.cdm.data.addr,
                                          LIB_VRF_GET_VRF_ID
                                          (igi->igi_owning_ivrf));

          if (! ifp)
            {
              IGMP_DBG_INFO (igi, DECODE, "No interface for address %r",
                             &cmsg.cdm.data.addr);

              ret = IGMP_ERR_NONE;
              goto EXIT;
            }
        }

      igifidx.igifidx_ifp = ifp;
      igifidx.igifidx_sid = igmp_if_svc_reg_su_id (igi, ifp);
      break;

    default:
      ret = IGMP_ERR_DOOM;
      goto EXIT;
    }

  /* Sanity Check */
  if (if_is_loopback (ifp))
    {
      IGMP_DBG_INFO (igi, DECODE, "Interface %s is loopback",
                     ifp->name);

      ret = IGMP_ERR_NONE;
      goto EXIT;
    }

  /* Obtain the IGMP Interface */
  igif = igmp_if_lookup (igi, &igifidx);

  if (! igif)
    {
      IGMP_DBG_INFO (igi, DECODE, "No IGMP-IF for interface %s",
                     ifp->name);

      ret = IGMP_ERR_NONE;
      goto EXIT;
    }

  /* Ignore message if IGMP Interface is inactive */
  if (! CHECK_FLAG (igif->igif_sflags, IGMP_IF_SFLAG_ACTIVE))
    {
      IGMP_DBG_INFO (igi, DECODE, "IGMP-IF for interface %s inactive",
                     ifp->name);

      ret = IGMP_ERR_NONE;
      goto EXIT;
    }

  if (igsr->igsr_svc_type == IGMP_SVC_TYPE_L3
      && CHECK_FLAG (igif->igif_sflags, IGMP_IF_SFLAG_SNOOPING))
    {
      IGMP_DBG_INFO (igi, DECODE, "Ignoring IGMP Message on L3 socket"
                                  "since Snooping is enabled on %s",
                     ifp->name);

      ret = IGMP_ERR_NONE;
      goto EXIT;
    }

  /* Forward to just beyond IP header */
  STREAM_FORWARD_GETP (igi->igi_i_buf,
                       PAL_IPLEN_WORDS2BYTES (iph_len));

  /* Set Stream 'putp' and 'endp' appropriately */
  STREAM_FORWARD_PUTP (igi->igi_i_buf,
                       PAL_IPLEN_WORDS2BYTES (iph_len));
  STREAM_FORWARD_ENDP (igi->igi_i_buf, ip_len);

  /* Get Message type */
  msg_type= stream_getc (igi->igi_i_buf);

  /* Rewind back to just beyond IP header */
  STREAM_REWIND_GETP (igi->igi_i_buf,
                      sizeof (u_int8_t));

  /* Validate Router Alert option in IP header, if present */
  if (PAL_IPLEN_WORDS2BYTES (iph_len) ==
     sizeof (struct pal_in4_header) + IGMP_RA_SIZE)
    {
      if (igiph->ra_opt [0] != IGMP_IPOPT_RA
          || igiph->ra_opt [1] != IGMP_RA_SIZE
          || igiph->ra_opt [2]
          || igiph->ra_opt [3])
        {
          IGMP_DBG_INFO (igi, DECODE, "Invalid RA-Opt, Ignoring Msg...");

          /* Read and discard the frame */
          pal_sock_recvfrom (sock, STREAM_PNT (igi->igi_i_buf),
                             (ip_len - PAL_IPLEN_WORDS2BYTES (iph_len)),
                             0, NULL, NULL);

          ret = IGMP_ERR_NONE;
          goto EXIT;
        }
    }
  /* Warn about message without RA-opt field in IP header */
  else if (PAL_IPLEN_WORDS2BYTES (iph_len) >=
           sizeof (struct pal_in4_header))
    {
      IGMP_DBG_WARN (igi, DECODE, "IGMP Msg (IPHdrLen=%d) without RA-OPT",
                     PAL_IPLEN_WORDS2BYTES (iph_len));

      if (CHECK_FLAG (igif->igif_conf.igifc_cflags,IGMP_IF_CFLAG_RA_OPT))
        {
          IGMP_DBG_ERR (igi, DECODE,
                        "Ignoring IGMP Msg without RA Option due "
                        "to strict RA-Opt Validation");

          /* Read and discard the frame */
          pal_sock_recvfrom (sock, STREAM_PNT (igi->igi_i_buf),
                             (ip_len - PAL_IPLEN_WORDS2BYTES (iph_len)),
                             0, NULL, NULL);

          ret = IGMP_ERR_NONE;
          goto EXIT;
        }
    }

  /* Invoke message decoder */
  ret = igmp_dec_msg (igif, &igiph->iph.ip_src,
                      &igiph->iph.ip_dst,
                      ip_len - PAL_IPLEN_WORDS2BYTES (iph_len));

EXIT:

  /* Re-spawn the Read thread, if no errors */
  if (ret >= IGMP_ERR_NONE)
    {
      if (ret == IGMP_ERR_SOCK_MSG_PEEK)
        {
          /* Since PEEK will not actually read the data,
           * Read the message from the socket before returning */
          IGMP_DBG_INFO (igi, DECODE, "Clean-up socket unread data, Len(%d)",
                                      msg_len);
          pal_sock_recvfrom (sock, STREAM_PNT (igi->igi_i_buf),
                             msg_len ,
                             0, NULL, NULL);
         }

       IGMP_READ_ON (IGMP_LG (igi), igsr->t_igsr_read, igsr,
                  igmp_sock_read, sock);
    }

  IGMP_FN_EXIT (ret);
}

