/* Copyright (C) 2002-2003  All Rights Reserved. */

#include <igmp_incl.h>

/* Generate and send IGMP General Query Message */
s_int32_t
igmp_if_send_general_query (struct igmp_if *igif)
{
  struct igmp_instance *igi;
  u_int32_t max_rsp_time;
  u_int16_t msg_len;
  struct stream *s;
  s_int32_t ret;

  IGMP_FN_ENTER (Send Gen Query);

  ret = IGMP_ERR_NONE;
  max_rsp_time = 0;
  msg_len = 0;

  if (! igif
      || ! (igi = igif->igif_owning_igi)
      || ! (s = igi->igi_o_buf))
    {
      ret = IGMP_ERR_MALFORMED_ARG;
      goto EXIT;
    }

  /* Send Query only if a Querier on IFF */
  if (! CHECK_FLAG (igif->igif_sflags, IGMP_IF_SFLAG_QUERIER))
    {
      ret = IGMP_ERR_GENERIC;
      goto EXIT;
    }

  /* Reset the Stream Buffer */
  stream_reset (s);

  /* Forward Stream to accomodate IPv4 Msg Header and RA-Opt */
  STREAM_FORWARD_PUTP (s, sizeof (struct igmp_in4_header));

  /* Forward Stream by IGMP Msg Header size */
  STREAM_FORWARD_PUTP (s, IGMP_MSG_HEADER_SIZE);

  /* Encode an IGMP General Query Message */
  ret = igmp_enc_msg_query (igif, NULL, 0, NULL,
                            NULL, NULL, &msg_len);

  if (ret < IGMP_ERR_NONE)
    {
      IGMP_DBG_INFO (igi, ENCODE, "Query Msg Encode failed: %d", ret);

      goto EXIT;
    }

  /* Max-Rsp-Time would be Query-Resp-Interval */
  max_rsp_time = igif->igif_conf.igifc_qri * ONE_SEC_DECISECOND;

  /* Encode IGMP Msg Header */
  ret = igmp_enc_msg_query_hdr (igif, max_rsp_time, &msg_len);

  if (ret < IGMP_ERR_NONE)
    {
      IGMP_DBG_INFO (igi, ENCODE, "Msg Header Encoding failed: %d",
                     ret);

      goto EXIT;
    }

  /* Send IGMP message */
  ret = igmp_sock_write (igif,
                         igif->igif_paddr ? 
                         &igif->igif_paddr->u.prefix4:
                         &igi->igi_in4any_addr,
                         &igi->igi_allhosts,
                         msg_len);

  if (ret >= 0)
    {
      IGMP_DBG_INFO (igi, ENCODE, "Sent General Query on %s, ret=%d",
                     igif->igif_owning_ifp->name, ret);

      ret = IGMP_ERR_NONE;
    }

EXIT:

  IGMP_FN_EXIT (ret);
}

/* Generate and send IGMP Group-Specific Query Message */
s_int32_t
igmp_if_send_group_query (struct igmp_group_rec *igr)
{
  struct igmp_instance *igi;
  u_int32_t max_rsp_time;
  struct igmp_if *igif;
  u_int16_t msg_len;
  struct stream *s;
  s_int32_t ret;
  struct pal_timeval timeval;

  IGMP_FN_ENTER (Send Group Query);

  ret = IGMP_ERR_NONE;
  max_rsp_time = 0;
  msg_len = 0;

  if (! igr || ! (igif = igr->igr_owning_igif)
      || ! (igi = igif->igif_owning_igi)
      || ! (s = igi->igi_o_buf))
    {
      ret = IGMP_ERR_MALFORMED_ARG;
      goto EXIT;
    }

  /* Send Query only if a Querier on L3 IFF or a pure L2 Interface */
  if (! CHECK_FLAG (igif->igif_sflags, IGMP_IF_SFLAG_QUERIER)
      && CHECK_FLAG (igif->igif_sflags, IGMP_IF_SFLAG_SVC_TYPE_L3))
    {
      ret = IGMP_ERR_GENERIC;
      goto EXIT;
    }

  /* Reset the Stream Buffer */
  stream_reset (s);

  /* Forward Stream to accomodate IPv4 Msg Header and RA-Opt */
  STREAM_FORWARD_PUTP (s, sizeof (struct igmp_in4_header));

  /* Forward Stream by IGMP Msg Header size */
  STREAM_FORWARD_PUTP (s, IGMP_MSG_HEADER_SIZE);

  /* Encode an IGMP Grp-Specific Query Message */
  ret = igmp_enc_msg_query (igif, igr, 0, NULL,
                            NULL, NULL, &msg_len);

  if (ret < IGMP_ERR_NONE)
    {
      IGMP_DBG_INFO (igi, ENCODE, "Query Msg Encode failed: %d", ret);

      goto EXIT;
    }

  /* Max-Rsp-Time would be LMQI */
  max_rsp_time = igif->igif_conf.igifc_lmqi / MILLISEC_TO_DECISEC;

  /* Encode IGMP Msg Header */
  ret = igmp_enc_msg_query_hdr (igif, max_rsp_time, &msg_len);

  if (ret < IGMP_ERR_NONE)
    {
      IGMP_DBG_INFO (igi, ENCODE, "Msg Header Encoding failed: %d",
                     ret);

      goto EXIT;
    }

  /* Send IGMP message */
  ret = igmp_sock_write (igif,
                         (igif->igif_paddr
                          && CHECK_FLAG (igif->igif_sflags,
                                         IGMP_IF_SFLAG_QUERIER)) ?
                         &igif->igif_paddr->u.prefix4:
                         &igi->igi_in4any_addr,
                         (struct pal_in4_addr *)
                         PTREE_NODE_KEY (igr->igr_owning_pn),
                         msg_len);

  if (ret >= 0)
    {
      IGMP_DBG_INFO (igi, ENCODE, "Sent Grp Query on %s",
                     igif->igif_owning_ifp->name);
      ret = IGMP_ERR_NONE;
    }

  if (igr->igr_rexmit_group_lmqc)
    {
      timeval = MSEC2TV (igr->igr_owning_igif->igif_conf.igifc_lmqi);
      IGMP_TIMER_OFF (IGMP_LG (igi), igr->t_igr_rexmit_group);
      IGMP_TIMER_TIMEVAL_ON (IGMP_LG (igi), igr->t_igr_rexmit_group,
                     igr, igmp_if_grec_timer_rexmit_group,
                             timeval);
    }

EXIT:

  IGMP_FN_EXIT (ret);
}

/* Generate and send IGMP Group-Source Specific Query Message */
s_int32_t
igmp_if_send_group_source_query (struct igmp_group_rec *igr,
                                 struct ptree *psrcs_tib,
                                 u_int16_t num_srcs)
{
  enum igmp_gs_query_type query_type;
  struct igmp_instance *igi;
  u_int32_t max_rsp_time;
  u_int16_t num_enc_srcs;
  struct igmp_if *igif;
  u_int16_t msg_len;
  struct stream *s;
  s_int32_t ret;
  struct pal_timeval timeval;

  IGMP_FN_ENTER (Send Group-Source Query);

  ret = IGMP_ERR_NONE;
  max_rsp_time = 0;
  num_enc_srcs = 0;
  msg_len = 0;

  if (! igr
      || ! (igif = igr->igr_owning_igif)
      || ! (igi = igif->igif_owning_igi)
      || ! (s = igi->igi_o_buf))
    {
      ret = IGMP_ERR_MALFORMED_ARG;
      goto EXIT;
    }

  /* If Source-Tree is NULL, dont send Query */
  if ((! CHECK_FLAG (igif->igif_sflags, IGMP_IF_SFLAG_QUERIER)
       && CHECK_FLAG (igif->igif_sflags, IGMP_IF_SFLAG_SVC_TYPE_L3)) 
      || ! num_srcs)
    {
      ret = IGMP_ERR_NONE;
      goto EXIT;
    }

  /* Encode a Query with Timed-Sources and another without */
  query_type = IGMP_GS_QUERY_SFLAG_SET;
  do {
    /* Reset the Stream Buffer & Encoded Srcs Count */
    stream_reset (s);
    num_enc_srcs = 0;

    /* Forward Stream to accomodate IPv4 Msg Header and RA-Opt */
    STREAM_FORWARD_PUTP (s, sizeof (struct igmp_in4_header));

    /* Forward Stream by IGMP Msg Header size */
    STREAM_FORWARD_PUTP (s, IGMP_MSG_HEADER_SIZE);

    /* Encode an IGMP Group-Source Query Message */
    ret = igmp_enc_msg_query (igif, igr, num_srcs, psrcs_tib,
                              &query_type, &num_enc_srcs, &msg_len);

    if (ret < IGMP_ERR_NONE)
      {
        IGMP_DBG_INFO (igi, ENCODE, "Query Msg Encode failed: %d",
                       ret);

        goto EXIT;
      }

    /* If Source-List is 0, dont send Query */
    if (! num_enc_srcs)
      continue;

    /* Max-Rsp-Time would be LMQI */
    max_rsp_time = igif->igif_conf.igifc_lmqi / MILLISEC_TO_DECISEC;

    /* Encode IGMP Msg Header */
    ret = igmp_enc_msg_query_hdr (igif, max_rsp_time, &msg_len);

    if (ret < IGMP_ERR_NONE)
      {
        IGMP_DBG_INFO (igi, ENCODE, "Msg Header Encoding failed: %d",
                       ret);

        goto EXIT;
      }

    /* Send IGMP message */
    ret = igmp_sock_write (igif,
                           (igif->igif_paddr
                            && CHECK_FLAG (igif->igif_sflags,
                                          IGMP_IF_SFLAG_QUERIER)) ?
                           &igif->igif_paddr->u.prefix4:
                           &igi->igi_in4any_addr,
                           (struct pal_in4_addr *)
                           PTREE_NODE_KEY (igr->igr_owning_pn),
                           msg_len);

    if (ret >= 0)
      {
        IGMP_DBG_INFO (igi, ENCODE, "Sent G-S Query on %s",
                       igif->igif_owning_ifp->name);
        ret = IGMP_ERR_NONE;
      }
  } while (query_type != IGMP_GS_QUERY_DONE);

  /* (Re)Start the Re-Transmission Timer */
  if (igr->igr_rexmit_group_source_lmqc)
    {
      timeval = MSEC2TV (igr->igr_owning_igif->igif_conf.igifc_lmqi);
      IGMP_TIMER_OFF (IGMP_LG (igi), igr->t_igr_rexmit_group_source);
      IGMP_TIMER_TIMEVAL_ON (IGMP_LG (igi), igr->t_igr_rexmit_group_source,
                     igr, igmp_if_grec_timer_rexmit_group_source,
                             timeval);
    }

EXIT:

  IGMP_FN_EXIT (ret);
}

/*
 * Forward received IGMPv1/v2 Report Message
 * NOTE: igi->igi_i_buf SHOULD contain the received Report
 */
s_int32_t
igmp_if_hst_forward_report (struct igmp_if *iigif,
                            struct igmp_if *oigif)
{
  struct igmp_instance *igi;
  u_int16_t msg_len;
  s_int32_t ret;

  IGMP_FN_ENTER (Forward Report);

  ret = IGMP_ERR_NONE;

  /* Validate IF indices */
  if (! iigif
      || ! oigif
      || iigif == oigif
      || iigif->igif_su_id != oigif->igif_su_id
      || ! (igi = iigif->igif_owning_igi))
    {
      ret = IGMP_ERR_NONE;
      goto EXIT;
    }

  /* Validate IF modes */
  if (! CHECK_FLAG (iigif->igif_sflags, IGMP_IF_SFLAG_SNOOPING)
      || ! CHECK_FLAG (oigif->igif_sflags, IGMP_IF_SFLAG_SNOOPING)
      || (! CHECK_FLAG (oigif->igif_sflags,
                        IGMP_IF_SFLAG_SNOOP_MROUTER_IF)
          && ! CHECK_FLAG (oigif->igif_sflags,
                           IGMP_IF_SFLAG_SNOOP_MROUTER_IF_CONFIG)))
    {
      ret = IGMP_ERR_NONE;
      goto EXIT;
    }

  STREAM_REWIND_PUTP (igi->igi_i_buf, IGMP_MSG_HEADER_SIZE);

  /* Calculate Msg Length */
  msg_len = STREAM_GET_GETP (igi->igi_i_buf) -
            STREAM_GET_PUTP (igi->igi_i_buf);

  stream_copy (igi->igi_o_buf, igi->igi_i_buf);
 
  /* Send IGMP message */
  ret = igmp_sock_write (oigif, NULL, NULL, msg_len);

  if (ret >= 0)
    {
      IGMP_DBG_INFO (igi, ENCODE, "IGMP Msg Forwarded from %s onto %s",
                     iigif->igif_owning_ifp->name,
                     oigif->igif_owning_ifp->name);

      ret = IGMP_ERR_NONE;
    }

  STREAM_FORWARD_PUTP (igi->igi_i_buf, IGMP_MSG_HEADER_SIZE);

EXIT:

  IGMP_FN_EXIT (ret);
}

/*
 * Forward received IGMPv1/v2/v3 Query Message
 * NOTE: igi->igi_i_buf SHOULD contain the received Query.
 */
s_int32_t
igmp_if_hst_forward_query (struct igmp_if *iigif,
                           struct igmp_if *oigif)
{
  struct igmp_instance *igi;
  u_int16_t msg_len;
  s_int32_t ret;

  IGMP_FN_ENTER (Forward Query);

  ret = IGMP_ERR_NONE;

  /* Validate IF indices */
  if (! iigif
      || ! oigif
      || iigif == oigif
      || iigif->igif_su_id != oigif->igif_su_id
      || ! (igi = iigif->igif_owning_igi))
    {
      ret = IGMP_ERR_NONE;
      goto EXIT;
    }

  /* Validate IF modes */
  if (! CHECK_FLAG (iigif->igif_sflags, IGMP_IF_SFLAG_SNOOPING)
      || ! CHECK_FLAG (oigif->igif_sflags, IGMP_IF_SFLAG_SNOOPING))
    {
      ret = IGMP_ERR_NONE;
      goto EXIT;
    }

  STREAM_REWIND_PUTP (igi->igi_i_buf, IGMP_MSG_HEADER_SIZE);

  /* Calculate Msg Length */
  msg_len = STREAM_GET_GETP (igi->igi_i_buf) -
            STREAM_GET_PUTP (igi->igi_i_buf);

  stream_copy (igi->igi_o_buf, igi->igi_i_buf);

  /* Send IGMP message */
  ret = igmp_sock_write (oigif, NULL, NULL, msg_len);

  if (ret >= 0)
    {
      IGMP_DBG_INFO (igi, ENCODE, "IGMP Msg Forwarded from %s onto %s",
                     iigif->igif_owning_ifp->name,
                     oigif->igif_owning_ifp->name);

      ret = IGMP_ERR_NONE;
    }

  STREAM_FORWARD_PUTP (igi->igi_i_buf, IGMP_MSG_HEADER_SIZE);

EXIT:

  IGMP_FN_EXIT (ret);
}

/* Generate and send IGMP General-Query Report Message */
s_int32_t
igmp_if_hst_send_general_report (struct igmp_if *igif)
{
  struct ptree_node *last_grp_enc;
  struct ptree_node *last_src_enc;
  struct igmp_group_rec *hst_igr;
  struct igmp_instance *igi;
  struct igmp_if *hst_igif;
  struct ptree_node *pn;
  u_int32_t stream_size;
  u_int16_t msg_len;
  u_int8_t msg_type;
  struct stream *s;
  bool_t all_done;
  s_int32_t ret;
  s_int32_t mtu;

  IGMP_FN_ENTER (Send Gen Report);

  all_done = PAL_FALSE;
  last_grp_enc = NULL;
  last_src_enc = NULL;
  ret = IGMP_ERR_NONE;
  stream_size = 0;
  msg_len = 0;

  /* Sanity checks */
  if (! igif
      || ! (igi = igif->igif_owning_igi)
      || ! (s = igi->igi_o_buf))
    {
      ret = IGMP_ERR_MALFORMED_ARG;
      goto EXIT;
    }

  if (CHECK_FLAG (igif->igif_conf.igifc_cflags,
                  IGMP_IF_CFLAG_SNOOP_NO_REPORT_SUPPRESS)
      && ((igif->igif_conf.igifc_version == IGMP_VERSION_1)
          || (igif->igif_conf.igifc_version == IGMP_VERSION_2)))
    {
      ret = IGMP_ERR_NONE;
      goto EXIT;
    }

  /* Obtain the Host-side interface */
  if (! (hst_igif = igif->igif_hst_igif))
    {
      if (CHECK_FLAG (igif->igif_conf.igifc_cflags,
                      IGMP_IF_CFLAG_PROXY_SERVICE)
          || (CHECK_FLAG (igif->igif_sflags,
                          IGMP_IF_SFLAG_SNOOPING)
              && CHECK_FLAG (igif->igif_sflags,
                             IGMP_IF_SFLAG_SVC_TYPE_L2)
              && CHECK_FLAG (igif->igif_sflags,
                             IGMP_IF_SFLAG_SVC_TYPE_L3)))
        hst_igif = igif;
    }

  if (! hst_igif)
    {
      ret = IGMP_ERR_MALFORMED_ARG;
      goto EXIT;
    }

  /* Size of stream should not exceed MTU */
  if (igif->igif_owning_ifp)
    {
      mtu = igif->igif_owning_ifp->mtu;
      if (mtu < stream_get_size(s))
        {
          stream_size = stream_get_size (s);
    
          /* MTU also includes the ethernet header */
          stream_set_size (s, mtu - ETHER_HDR_LEN);
        }
    }

  /* Send Current-State General Report (CSR-General) */
  if (CHECK_FLAG (igif->igif_sflags,
                  IGMP_IF_SFLAG_HOST_COMPAT_V3))
    {
      do {
        /* Reset the Stream Buffer */
        stream_reset (s);

        /* Forward Stream to accomodate IPv4 Hdr & RA-Opt */
        STREAM_FORWARD_PUTP (s, sizeof (struct igmp_in4_header));

        ret = igmp_enc_msg_v3_report (hst_igif, NULL, NULL, &msg_len, 
                                     &last_grp_enc, &last_src_enc);

        /* Send IGMP message */
        if (! ret || ret == IGMP_ERR_BUF_TOO_SHORT)
          {
            if (! ret)
              all_done = PAL_TRUE;

            ret = igmp_sock_write (igif,
                                   igif->igif_paddr ?
                                   &igif->igif_paddr->u.prefix4:
                                   &igi->igi_in4any_addr,
                                   &igi->igi_igmp_v3routers,
                                   msg_len);

            if (ret >= IGMP_ERR_NONE)
              IGMP_DBG_INFO (igi, ENCODE, "Sent General Query"
                             " on %s, ret=%d",
                             igif->igif_owning_ifp->name, ret);

            /* Loose the Err Code */
            ret = IGMP_ERR_NONE;
          }
      } while (! all_done);

      /* Restore size of sream */
      if (stream_size)
        stream_set_size (s, stream_size);
    }

  /* IGMP Host-Compat Version 1/2 */
  else for (pn = ptree_top (hst_igif->igif_hst_gmr_tib); pn;
            pn = ptree_next (pn))
    {
      if (! (hst_igr = pn->info))
        continue;

      if ( hst_igr->igr_filt_mode_state == IGMP_FMS_INCLUDE &&
            ! hst_igr->igr_src_a_tib_count)
        continue;

      /* Reset the Stream Buffer and Msg Len */
      stream_reset (s);
      msg_len = 0;

      /* Forward Stream to accomodate IPv4 Hdr & RA-Opt */
      STREAM_FORWARD_PUTP (s, sizeof (struct igmp_in4_header));

      /* Encode IGMP v1/v2 Report Msg */
      msg_type = IGMP_MSG_V1_MEMBERSHIP_REPORT;

      if (CHECK_FLAG (igif->igif_sflags,
                      IGMP_IF_SFLAG_HOST_COMPAT_V2))
        msg_type = IGMP_MSG_V2_MEMBERSHIP_REPORT;

      ret = igmp_enc_msg_v1_v2_report_leave (hst_igr, msg_type, &msg_len);

      if (ret < IGMP_ERR_NONE)
        {
          IGMP_DBG_INFO (igi, ENCODE, "%r %s encoding Failed!",
                         PTREE_NODE_KEY (hst_igr->igr_owning_pn),
                         IGMP_MSG_TYPE_STR (msg_type));

          /* Loose the err-code */
          ret = IGMP_ERR_NONE;
          continue;
        }

      ret = igmp_sock_write (igif,
                             igif->igif_paddr ?
                             &igif->igif_paddr->u.prefix4:
                             &igi->igi_in4any_addr,
                             (struct pal_in4_addr *)
                             PTREE_NODE_KEY (hst_igr->igr_owning_pn),
                             msg_len);

      if (ret >= IGMP_ERR_NONE)
        IGMP_DBG_INFO (igi, ENCODE, "Sent General Report on %s, ret=%d",
                       igif->igif_owning_ifp->name, ret);
    }

EXIT:

  IGMP_FN_EXIT (ret);
}

/* Generate and send IGMP Group-Specific Report Message */
s_int32_t
igmp_if_hst_send_group_report (struct igmp_group_rec *hst_igr)
{
  struct ptree_node *last_src_enc;
  struct pal_timeval timeval;
  struct igmp_instance *igi;
  struct igmp_if *hst_igif;
  struct igmp_if *ds_igif;
  u_int32_t stream_size;
  struct listnode *nn;
  u_int16_t msg_len;
  u_int8_t msg_type;
  struct stream *s;
  bool_t all_done;
  s_int32_t ret;
  u_int16_t mtu;

  IGMP_FN_ENTER (Send Group Report);

  all_done = PAL_FALSE;
  ret = IGMP_ERR_NONE;
  last_src_enc = NULL;
  stream_size = 0;
  hst_igif = NULL;
  ds_igif = NULL;
  msg_len = 0;
  igi = NULL;
  nn = NULL;

  /* Sanity checks */
  if (! hst_igr
      || ! (hst_igif = hst_igr->igr_owning_igif)
      || ! (igi = hst_igif->igif_owning_igi)
      || ! (s = igi->igi_o_buf))
    {
      ret = IGMP_ERR_MALFORMED_ARG;
      goto EXIT;
    }

  if (CHECK_FLAG (hst_igif->igif_conf.igifc_cflags,
                  IGMP_IF_CFLAG_SNOOP_NO_REPORT_SUPPRESS)
      && ((hst_igif->igif_conf.igifc_version == IGMP_VERSION_1)
          || (hst_igif->igif_conf.igifc_version == IGMP_VERSION_2)))
    {
      ret = IGMP_ERR_NONE;
      goto EXIT;
    }

  switch (hst_igr->igr_rexmit_hrt)
    {
    case IGMP_HRT_INVALID:
      /* Send Current-State Report Group */
      /* DO NOT BREAK HERE */
    case IGMP_HRT_CHG_TO_INCL:
    case IGMP_HRT_CHG_TO_EXCL:
      /* Get the first DS_IF */
      if (CHECK_FLAG (hst_igif->igif_sflags,
                      IGMP_IF_SFLAG_SNOOPING)
          && CHECK_FLAG (hst_igif->igif_sflags,
                         IGMP_IF_SFLAG_SVC_TYPE_L2)
          && CHECK_FLAG (hst_igif->igif_sflags,
                         IGMP_IF_SFLAG_SVC_TYPE_L3))
        {
          LIST_LOOP (&hst_igif->igif_hst_dsif_lst, ds_igif, nn)
            {
              if (CHECK_FLAG (ds_igif->igif_sflags,
                              IGMP_IF_SFLAG_SNOOP_MROUTER_IF)
                  || CHECK_FLAG (ds_igif->igif_sflags,
                                 IGMP_IF_SFLAG_SNOOP_MROUTER_IF_CONFIG))
                break;
            }

          if (nn)
            ds_igif = GETDATA (nn);
          else
            ds_igif = NULL;
        }
      else if (CHECK_FLAG (hst_igif->igif_conf.igifc_cflags,
                           IGMP_IF_CFLAG_PROXY_SERVICE))
        ds_igif = hst_igif;

      if (! ds_igif)
        {
          IGMP_DBG_INFO (igi, ENCODE, "HST-IF %s DS-IF NOT FOUND!",
                         hst_igif->igif_owning_ifp->name);

          ret = IGMP_ERR_NONE;
          goto EXIT;
        }

      /* Encode and send on all DS-IFs */
      do {

        /* Send State-Change Report Group */
        if (CHECK_FLAG (ds_igif->igif_sflags,
                        IGMP_IF_SFLAG_HOST_COMPAT_V3))
          {
            all_done = PAL_FALSE;
          
            /* Size of stream should not exceed MTU */
            mtu = ds_igif->igif_owning_ifp->mtu;
            if (mtu < stream_get_size (s))
              {
                stream_size = stream_get_size(s);

                /* MTU also includes the ethernet header */
                stream_set_size (s, mtu - ETHER_HDR_LEN);
              }

            do {
              /* Reset the Stream Buffer */
              stream_reset (s);
              msg_len = 0;

              /* Forward Stream to accomodate IPv4 Hdr & RA-Opt */
              STREAM_FORWARD_PUTP (s, sizeof (struct igmp_in4_header));

              ret = igmp_enc_msg_v3_report (hst_igif, hst_igr, NULL,
                                            &msg_len, NULL, &last_src_enc);

              /* Send IGMP message */
              if (ret == IGMP_ERR_NONE
                  || ret == IGMP_ERR_BUF_TOO_SHORT)
                {
                  if (ret == IGMP_ERR_NONE)
                    all_done = PAL_TRUE;

                  ret = igmp_sock_write (ds_igif,
                                         ds_igif->igif_paddr ?
                                         &ds_igif->igif_paddr->u.prefix4:
                                         &igi->igi_in4any_addr,
                                         &igi->igi_igmp_v3routers,
                                         msg_len);

                  if (ret >= IGMP_ERR_NONE)
                    IGMP_DBG_INFO (igi, ENCODE, "Sent Grp-Report"
                                   " on %s, ret=%d",
                                   ds_igif->igif_owning_ifp->name,
                                   ret);
                }

              /* Loose the Err Code */
              ret = IGMP_ERR_NONE;
            } while (! all_done);

            /* Restore size of stream */
            if (stream_size)
              stream_set_size(s, stream_size);

          }
        else /* ==> IGMP Host-Compat Version 1/2 */
          {
            /* Reset the Stream Buffer and Msg Len */
            stream_reset (s);
            msg_len = 0;

            /* Forward Stream to accomodate IPv4 Hdr & RA-Opt */
            STREAM_FORWARD_PUTP (s, sizeof (struct igmp_in4_header));

            /* Encode IGMP v1/v2 Report Msg */
            msg_type = IGMP_MSG_V2_MEMBERSHIP_REPORT;

            if (hst_igr->igr_rexmit_hrt == IGMP_HRT_CHG_TO_INCL
                && ! hst_igr->igr_src_a_tib_count)
              msg_type = IGMP_MSG_V2_LEAVE_GROUP;
            else if (CHECK_FLAG (ds_igif->igif_sflags,
                                 IGMP_IF_SFLAG_HOST_COMPAT_V1))
              msg_type = IGMP_MSG_V1_MEMBERSHIP_REPORT;

            if (msg_type != IGMP_MSG_V2_LEAVE_GROUP
                || ! CHECK_FLAG (ds_igif->igif_sflags,
                                 IGMP_IF_SFLAG_HOST_COMPAT_V1))
              {
                ret = igmp_enc_msg_v1_v2_report_leave (hst_igr,
                                                       msg_type,
                                                       &msg_len);

                if (ret == IGMP_ERR_NONE)
                  {
                    ret = igmp_sock_write
                          (ds_igif,
                           ds_igif->igif_paddr ?
                           &ds_igif->igif_paddr->u.prefix4:
                           &igi->igi_in4any_addr,
                           (struct pal_in4_addr *)
                           PTREE_NODE_KEY (hst_igr->igr_owning_pn),
                           msg_len);

                    if (ret >= IGMP_ERR_NONE)
                      IGMP_DBG_INFO (igi, ENCODE, "Sent Grp-Report"
                                     " on %s, ret=%d",
                                     ds_igif->igif_owning_ifp->name,
                                     ret);

                    /* Loose the err-code */
                    ret = IGMP_ERR_NONE;
                  }
                else
                  {
                    IGMP_DBG_INFO (igi, ENCODE, "%r %s encoding Failed!",
                                   PTREE_NODE_KEY (hst_igr->igr_owning_pn),
                                   IGMP_MSG_TYPE_STR (msg_type));

                    /* Loose the err-code */
                    ret = IGMP_ERR_NONE;
                  }
              }
          }

        /* Get the next DS_IF */
        ds_igif = NULL;

        if (CHECK_FLAG (hst_igif->igif_sflags,
                        IGMP_IF_SFLAG_SNOOPING)
            && CHECK_FLAG (hst_igif->igif_sflags,
                           IGMP_IF_SFLAG_SVC_TYPE_L2)
            && CHECK_FLAG (hst_igif->igif_sflags,
                           IGMP_IF_SFLAG_SVC_TYPE_L3))
          {
            for (NEXTNODE (nn); nn; NEXTNODE (nn))
              {
                if (! (ds_igif = GETDATA (nn)))
                  continue;

                if (CHECK_FLAG (ds_igif->igif_sflags,
                                IGMP_IF_SFLAG_SNOOP_MROUTER_IF)
                    || CHECK_FLAG (ds_igif->igif_sflags,
                                   IGMP_IF_SFLAG_SNOOP_MROUTER_IF_CONFIG))
                  break;
              }

            if (nn)
              ds_igif = GETDATA (nn);
            else
              ds_igif = NULL;
          }
      } while (ds_igif);
      break;

    case IGMP_HRT_MODE_IS_INCL:
    case IGMP_HRT_MODE_IS_EXCL:
    case IGMP_HRT_ALLOW_NEW_SRCS:
    case IGMP_HRT_BLOCK_OLD_SRCS:
      /* Invalid Values */
      IGMP_DBG_ERR (igi, ENCODE, "Invalid Rexmit HRT(%u)!",
                    hst_igr->igr_rexmit_hrt);
      break;

    default:
      ret = IGMP_ERR_DOOM;
      goto EXIT;
    }

EXIT:

  if (ret == IGMP_ERR_NONE
      && hst_igr->igr_rexmit_group_lmqc - 1)
    {
      timeval = MSEC2TV (hst_igr->igr_owning_igif->igif_conf.igifc_lmqi);
      IGMP_TIMER_OFF (IGMP_LG (igi), hst_igr->t_igr_rexmit_group);
      IGMP_TIMER_TIMEVAL_ON (IGMP_LG (igi), hst_igr->t_igr_rexmit_group,
                     hst_igr, igmp_if_hst_grec_timer_rexmit_group,
                             timeval);
    }

  IGMP_FN_EXIT (ret);
}

/* Generate and send IGMP Grp-Src Specific Report Message */
s_int32_t
igmp_if_hst_send_group_source_report (struct igmp_group_rec *hst_igr,
                                      struct ptree *psrcs_tib)
{
  struct ptree_node *last_src_enc;
  struct pal_timeval timeval;
  struct igmp_instance *igi;
  struct igmp_if *hst_igif;
  struct igmp_if *ds_igif;
  u_int32_t stream_size;
  struct listnode *nn;
  u_int16_t msg_len;
  u_int8_t msg_type;
  struct stream *s;
  bool_t all_done;
  s_int32_t ret;
  s_int32_t mtu;

  IGMP_FN_ENTER (Send Grp-Src Report);

  all_done = PAL_FALSE;
  ret = IGMP_ERR_NONE;
  last_src_enc = NULL;
  stream_size = 0;
  ds_igif = NULL;
  msg_len = 0;
  igi = NULL;
  nn = NULL;

  /* Sanity checks */
  if (! hst_igr
      || ! (hst_igif = hst_igr->igr_owning_igif)
      || ! (igi = hst_igif->igif_owning_igi)
      || ! (s = igi->igi_o_buf)
      || ! psrcs_tib)
    {
      ret = IGMP_ERR_MALFORMED_ARG;
      goto EXIT;
    }

  if (CHECK_FLAG (hst_igif->igif_conf.igifc_cflags,
                  IGMP_IF_CFLAG_SNOOP_NO_REPORT_SUPPRESS)
      && ((hst_igif->igif_conf.igifc_version == IGMP_VERSION_1)
          || (hst_igif->igif_conf.igifc_version == IGMP_VERSION_2)))
    {
      ret = IGMP_ERR_NONE;
      goto EXIT;
    }

  switch (hst_igr->igr_rexmit_hrt)
    {
    case IGMP_HRT_INVALID:
      /* Send Current-State Grp-Srcs Report (CSR-Sources) */
      /* DO NOT BREAK HERE */
    case IGMP_HRT_ALLOW_NEW_SRCS:
    case IGMP_HRT_BLOCK_OLD_SRCS:
      /* Send SrcListChange Report (SLR) */

      /* Get the first DS_IF */
      if (CHECK_FLAG (hst_igif->igif_sflags,
                      IGMP_IF_SFLAG_SNOOPING)
          && CHECK_FLAG (hst_igif->igif_sflags,
                         IGMP_IF_SFLAG_SVC_TYPE_L2)
          && CHECK_FLAG (hst_igif->igif_sflags,
                         IGMP_IF_SFLAG_SVC_TYPE_L3))
        {
          LIST_LOOP (&hst_igif->igif_hst_dsif_lst, ds_igif, nn)
            {
              if (CHECK_FLAG (ds_igif->igif_sflags,
                              IGMP_IF_SFLAG_SNOOP_MROUTER_IF)
                  || CHECK_FLAG (ds_igif->igif_sflags,
                                 IGMP_IF_SFLAG_SNOOP_MROUTER_IF_CONFIG))
                break;
            }

          if (nn)
            ds_igif = GETDATA (nn);
          else
            ds_igif = NULL;
        }
      else if (CHECK_FLAG (hst_igif->igif_conf.igifc_cflags,
                           IGMP_IF_CFLAG_PROXY_SERVICE))
        ds_igif = hst_igif;

      if (! ds_igif)
        {
          IGMP_DBG_INFO (igi, ENCODE, "HST-IF %s DS-IF NOT FOUND!",
                         hst_igif->igif_owning_ifp->name);

          ret = IGMP_ERR_NONE;
          goto EXIT;
        }

      /* Encode and send on all DS-IFs */
      do {

        /* Send State-Change Report Group */
        if (CHECK_FLAG (ds_igif->igif_sflags,
                        IGMP_IF_SFLAG_HOST_COMPAT_V3))
          {
            all_done = PAL_FALSE;

            /* Size of stream should not exceed MTU */
            if (ds_igif->igif_owning_ifp)
              {
                mtu = ds_igif->igif_owning_ifp->mtu;
                if (mtu < stream_get_size(s))
                  {
                    stream_size = stream_get_size (s);

                    /* MTU also includes the ethernet header */
                    stream_set_size (s, mtu - ETHER_HDR_LEN);
                  }
                }

            do {
              /* Reset the Stream Buffer */
              stream_reset (s);
              msg_len = 0;

              /* Forward Stream to accomodate IPv4 Hdr & RA-Opt */
              STREAM_FORWARD_PUTP (s, sizeof (struct igmp_in4_header));

              /* Encode IGMP V3 Report */
              ret = igmp_enc_msg_v3_report (hst_igif, hst_igr,
                                            psrcs_tib, &msg_len,
                                            NULL, &last_src_enc);

              /* Send IGMP message */
              if (ret == IGMP_ERR_NONE
                  || ret == IGMP_ERR_BUF_TOO_SHORT)
                {
                  if (ret == IGMP_ERR_NONE)
                    all_done = PAL_TRUE;

                  ret = igmp_sock_write (ds_igif,
                                         ds_igif->igif_paddr ?
                                         &ds_igif->igif_paddr->u.prefix4:
                                         &igi->igi_in4any_addr,
                                         &igi->igi_igmp_v3routers,
                                         msg_len);

                  if (ret >= IGMP_ERR_NONE)
                    IGMP_DBG_INFO (igi, ENCODE, "Sent Grp-Report"
                                   " on %s, ret=%d",
                                   ds_igif->igif_owning_ifp->name,
                                   ret);
                }

              /* Loose the Err Code */
              ret = IGMP_ERR_NONE;
            } while (! all_done);

            /* Restore size of sream */
            if (stream_size)
              stream_set_size (s, stream_size);
          }
        else /* ==> IGMP Host-Compat Version 1/2 */
          {
            /* Reset the Stream Buffer and Msg Len */
            stream_reset (s);
            msg_len = 0;

            /* Forward Stream to accomodate IPv4 Hdr & RA-Opt */
            STREAM_FORWARD_PUTP (s, sizeof (struct igmp_in4_header));

            /* Encode IGMP v1/v2 Report Msg */
            msg_type = IGMP_MSG_V1_MEMBERSHIP_REPORT;

            if (CHECK_FLAG (ds_igif->igif_sflags,
                            IGMP_IF_SFLAG_HOST_COMPAT_V2))
              msg_type = IGMP_MSG_V2_MEMBERSHIP_REPORT;

            ret = igmp_enc_msg_v1_v2_report_leave (hst_igr,
                                                   msg_type,
                                                   &msg_len);

            if (ret == IGMP_ERR_NONE)
              {
                ret = igmp_sock_write
                      (ds_igif,
                       ds_igif->igif_paddr ?
                       &ds_igif->igif_paddr->u.prefix4:
                       &igi->igi_in4any_addr,
                       (struct pal_in4_addr *)
                       PTREE_NODE_KEY (hst_igr->igr_owning_pn),
                       msg_len);

                if (ret >= IGMP_ERR_NONE)
                  IGMP_DBG_INFO (igi, ENCODE, "Sent Grp-Report"
                                 " on %s, ret=%d",
                                 ds_igif->igif_owning_ifp->name,
                                 ret);

                /* Loose the err-code */
                ret = IGMP_ERR_NONE;
              }
            else
              {
                IGMP_DBG_INFO (igi, ENCODE, "%r %s encoding Failed!",
                               PTREE_NODE_KEY (hst_igr->igr_owning_pn),
                               IGMP_MSG_TYPE_STR (msg_type));

                /* Loose the err-code */
                ret = IGMP_ERR_NONE;
              }
          }

        /* Get the next DS_IF */
        ds_igif = NULL;

        if (CHECK_FLAG (hst_igif->igif_sflags,
                        IGMP_IF_SFLAG_SNOOPING)
            && CHECK_FLAG (hst_igif->igif_sflags,
                           IGMP_IF_SFLAG_SVC_TYPE_L2)
            && CHECK_FLAG (hst_igif->igif_sflags,
                           IGMP_IF_SFLAG_SVC_TYPE_L3))
          {
            for (NEXTNODE (nn); nn; NEXTNODE (nn))
              {
                if (! (ds_igif = GETDATA (nn)))
                  continue;

                if (CHECK_FLAG (ds_igif->igif_sflags,
                                IGMP_IF_SFLAG_SNOOP_MROUTER_IF)
                    || CHECK_FLAG (ds_igif->igif_sflags,
                                   IGMP_IF_SFLAG_SNOOP_MROUTER_IF_CONFIG))
                  break;
              }

            if (nn)
              ds_igif = GETDATA (nn);
            else
              ds_igif = NULL;
          }
      } while (ds_igif);
      break;

    case IGMP_HRT_MODE_IS_INCL:
    case IGMP_HRT_MODE_IS_EXCL:
    case IGMP_HRT_CHG_TO_INCL:
    case IGMP_HRT_CHG_TO_EXCL:
      /* Invalid Values */
      IGMP_DBG_ERR (igi, ENCODE, "Invalid Rexmit HRT(%u)!",
                    hst_igr->igr_rexmit_hrt);
      break;

    default:
      ret = IGMP_ERR_DOOM;
      goto EXIT;
    }

EXIT:

  /* (Re)Start the Re-Transmission Timer */
  if (ret == IGMP_ERR_NONE
      && hst_igr->igr_rexmit_group_source_lmqc)
    {
      timeval = MSEC2TV (hst_igr->igr_owning_igif->igif_conf.igifc_lmqi);
      IGMP_TIMER_OFF (IGMP_LG (igi), hst_igr->t_igr_rexmit_group_source);
      IGMP_TIMER_TIMEVAL_ON (IGMP_LG (igi), hst_igr->t_igr_rexmit_group_source,
                     hst_igr, igmp_if_hst_grec_timer_rexmit_group_source,
                             timeval);
    }

  IGMP_FN_EXIT (ret);
}

/* Join Group Generate and send IGMP Group-Specific Report Message */
s_int32_t
igmp_if_jg_send_group_report (struct igmp_group_rec *jg_igr)
{
  struct ptree_node *last_src_enc;
  struct igmp_instance *igi;
  struct igmp_if *igif;
  struct stream *s;
  u_int32_t stream_size;
  u_int16_t msg_len;
  u_int8_t msg_type;
  bool_t all_done;
  s_int32_t ret;
  u_int16_t mtu;

  IGMP_FN_ENTER (Join Group Send Group Report);

  all_done = PAL_FALSE;
  ret = IGMP_ERR_NONE;
  last_src_enc = NULL;
  stream_size = 0;
  igif = NULL;
  msg_len = 0;
  igi = NULL;

  /* Sanity checks */
  if (! jg_igr
      || ! (igif = jg_igr->igr_owning_igif)
      || ! (igi = igif->igif_owning_igi)
      || ! (s = igi->igi_o_buf))
    {
      ret = IGMP_ERR_MALFORMED_ARG;
      goto EXIT;
    }

  /* Encode and send on IF */
  /* Send Report Group */
  if (CHECK_FLAG (igif->igif_sflags,
                    IGMP_IF_SFLAG_HOST_COMPAT_V3))
    {
      all_done = PAL_FALSE;
    
      /* Size of stream should not exceed MTU */
      mtu = igif->igif_owning_ifp->mtu;
      if (mtu < stream_get_size (s))
        {
          stream_size = stream_get_size(s);
    
          /* MTU also includes the ethernet header */
          if((mtu - ETHER_HDR_LEN) > 0)
            stream_set_size (s, mtu - ETHER_HDR_LEN);
        }
       
      do 
      {
       /* Reset the Stream Buffer */
       stream_reset (s);
       msg_len = 0;
    
       /* Forward Stream to accomodate IPv4 Hdr & RA-Opt */
       STREAM_FORWARD_PUTP (s, sizeof (struct igmp_in4_header));
    
       ret = igmp_enc_msg_v3_report (igif, jg_igr, NULL,
                                     &msg_len, NULL, &last_src_enc);
    
       /* Send IGMP message */
       if (ret == IGMP_ERR_NONE
           || ret == IGMP_ERR_BUF_TOO_SHORT)
         {
           all_done = PAL_TRUE;

           if(ret == IGMP_ERR_BUF_TOO_SHORT)
             IGMP_DBG_INFO (igi, ENCODE, "Sent Grp-Report Failed! (Too Short Buffer)"
                            " on %s, ret=%d",
                            igif->igif_owning_ifp->name,
                            ret);
           
           if (ret == IGMP_ERR_NONE)
            {
              ret = igmp_sock_write (igif,
                                  igif->igif_paddr ?
                                  &igif->igif_paddr->u.prefix4:
                                  &igi->igi_in4any_addr,
                                  &igi->igi_igmp_v3routers,
                                  msg_len);
    
              IGMP_DBG_INFO (igi, ENCODE, "Sent Grp-Report"
                            " on %s, ret=%d",
                            igif->igif_owning_ifp->name,
                            ret);
            }           
         }
    
        /* Loose the Err Code */
        ret = IGMP_ERR_NONE;
      } while (! all_done);
    
      /* Restore size of stream */
      if (stream_size)
        stream_set_size(s, stream_size);
      
    }
  else /* ==> IGMP Host-Compat Version 1/2 */
    {
      /* Reset the Stream Buffer and Msg Len */
      stream_reset (s);
      msg_len = 0;
    
      /* Forward Stream to accomodate IPv4 Hdr & RA-Opt */
      STREAM_FORWARD_PUTP (s, sizeof (struct igmp_in4_header));
    
      /* Encode IGMP v1/v2 Report Msg */
      msg_type = IGMP_MSG_V2_MEMBERSHIP_REPORT;
    
      if (CHECK_FLAG (igif->igif_sflags,
                      IGMP_IF_SFLAG_HOST_COMPAT_V1))
        msg_type = IGMP_MSG_V1_MEMBERSHIP_REPORT;

      /*jg_igr is join group that is configured manually.
      It is impossible to be leave group (IGMP_MSG_V2_LEAVE_GROUP)*/  
      ret = igmp_enc_msg_v1_v2_report_leave (jg_igr,
                                             msg_type,
                                             &msg_len);

      if (ret == IGMP_ERR_NONE)
        {
          ret = igmp_sock_write
                (igif,
                 igif->igif_paddr ?
                 &igif->igif_paddr->u.prefix4:
                 &igi->igi_in4any_addr,
                 (struct pal_in4_addr *)
                 PTREE_NODE_KEY (jg_igr->igr_owning_pn),
                 msg_len);

          if (ret >= IGMP_ERR_NONE)
            IGMP_DBG_INFO (igi, ENCODE, "Sent Grp-Report"
                           " on %s, ret=%d",
                           igif->igif_owning_ifp->name,
                           ret);

          /* Loose the err-code */
          ret = IGMP_ERR_NONE;
        }
      else
        {
          IGMP_DBG_INFO (igi, ENCODE, "%r %s encoding Failed!",
                         PTREE_NODE_KEY (jg_igr->igr_owning_pn),
                         IGMP_MSG_TYPE_STR (msg_type));

          /* Loose the err-code */
          ret = IGMP_ERR_NONE;
        }
    }
        
EXIT:

  IGMP_FN_EXIT (ret);
}

/* Join Group Generate and send IGMP General-Query Report Message */
s_int32_t
igmp_if_jg_send_general_report (struct igmp_if *igif)
{
  struct ptree_node *last_grp_enc;
  struct ptree_node *last_src_enc;
  struct igmp_instance *igi;
  struct igmp_group_rec *jg_igr;
  u_int32_t stream_size;
  u_int16_t msg_len;
  u_int8_t msg_type;
  struct stream *s;
  bool_t all_done;
  s_int32_t ret;
  s_int32_t mtu;
  struct ptree_node *pn;

  IGMP_FN_ENTER (Join Group Send Gen Report);

  all_done = PAL_FALSE;
  last_grp_enc = NULL;
  last_src_enc = NULL;
  ret = IGMP_ERR_NONE;
  stream_size = 0;
  msg_len = 0;

  /* Sanity checks */
  if (! igif
      || ! (igi = igif->igif_owning_igi)
      || ! (s = igi->igi_o_buf))
    {
      ret = IGMP_ERR_MALFORMED_ARG;
      goto EXIT;
    }

  /* Size of stream should not exceed MTU */
  if (igif->igif_owning_ifp)
    {
      mtu = igif->igif_owning_ifp->mtu;
      if (mtu < stream_get_size(s))
        {
          stream_size = stream_get_size (s);
    
          /* MTU also includes the ethernet header */
          if((mtu - ETHER_HDR_LEN) > 0)
            stream_set_size (s, mtu - ETHER_HDR_LEN);
        }
    }

  /* Send Current-State General Report */
  if (CHECK_FLAG (igif->igif_sflags,
                  IGMP_IF_SFLAG_HOST_COMPAT_V3))
    {
      for (pn = ptree_top (igif->igif_gmr_tib); pn; pn = ptree_next (pn))
        {
          if (! (jg_igr = pn->info))
            continue;

          if(CHECK_FLAG (jg_igr->igr_sflags, IGMP_IGR_SFLAG_JOIN))
            {
              do {
                /* Reset the Stream Buffer */
                stream_reset (s);
        
                /* Forward Stream to accomodate IPv4 Hdr & RA-Opt */
                STREAM_FORWARD_PUTP (s, sizeof (struct igmp_in4_header));
        
                /*specific group record, jg_igr*/
                ret = igmp_enc_msg_v3_report (igif, jg_igr, NULL, 
                                              &msg_len, NULL, &last_src_enc);
        
                /* Send IGMP message */
                if (! ret || ret == IGMP_ERR_BUF_TOO_SHORT)
                  {
                    all_done = PAL_TRUE;
         
                    if(ret == IGMP_ERR_BUF_TOO_SHORT)
                      IGMP_DBG_INFO (igi, ENCODE, "Sent General Report Failed! (Too Short Buffer)"
                                     " on %s, ret=%d",
                                     igif->igif_owning_ifp->name,
                                     ret);
                   
                    if (ret == IGMP_ERR_NONE)
                      {
                        ret = igmp_sock_write (igif,
                                               igif->igif_paddr ?
                                               &igif->igif_paddr->u.prefix4:
                                               &igi->igi_in4any_addr,
                                               &igi->igi_igmp_v3routers,
                                               msg_len);
             
                       IGMP_DBG_INFO (igi, ENCODE, "Sent General Report"
                                     " on %s, ret=%d",
                                     igif->igif_owning_ifp->name,
                                     ret);
                      }           
                   }
                 ret = IGMP_ERR_NONE;
                 } while (! all_done);
            }
        }
    }
  /* IGMP Host-Compat Version 1/2 */
  else
    {
      for (pn = ptree_top (igif->igif_gmr_tib); pn; pn = ptree_next (pn))
        {
          if (! (jg_igr = pn->info))
            continue;
    
          if(CHECK_FLAG (jg_igr->igr_sflags, IGMP_IGR_SFLAG_JOIN))
            {
              /* Reset the Stream Buffer and Msg Len */
              stream_reset (s);
              msg_len = 0;
        
              /* Forward Stream to accomodate IPv4 Hdr & RA-Opt */
              STREAM_FORWARD_PUTP (s, sizeof (struct igmp_in4_header));
        
              /* Encode IGMP v1/v2 Report Msg */
              msg_type = IGMP_MSG_V1_MEMBERSHIP_REPORT;
        
              if (CHECK_FLAG (igif->igif_sflags,
                              IGMP_IF_SFLAG_HOST_COMPAT_V2))
                msg_type = IGMP_MSG_V2_MEMBERSHIP_REPORT;
        
              ret = igmp_enc_msg_v1_v2_report_leave (jg_igr, msg_type, &msg_len);
        
              if (ret < IGMP_ERR_NONE)
                {
                  IGMP_DBG_INFO (igi, ENCODE, "%r %s encoding Failed!",
                                 PTREE_NODE_KEY (jg_igr->igr_owning_pn),
                                 IGMP_MSG_TYPE_STR (msg_type));
        
                  /* Loose the err-code */
                  ret = IGMP_ERR_NONE;
                }
        
              ret = igmp_sock_write (igif,
                                     igif->igif_paddr ?
                                     &igif->igif_paddr->u.prefix4:
                                     &igi->igi_in4any_addr,
                                     (struct pal_in4_addr *)
                                     PTREE_NODE_KEY (jg_igr->igr_owning_pn),
                                     msg_len);
        
              if (ret >= IGMP_ERR_NONE)
                IGMP_DBG_INFO (igi, ENCODE, "Sent General Report on %s, ret=%d",
                               igif->igif_owning_ifp->name, ret);
            }
         }
    }
  
  /* Restore size of sream */
  if (stream_size)
    stream_set_size (s, stream_size);

EXIT:

  IGMP_FN_EXIT (ret);
}

/* Encode IGMP Query Message */
s_int32_t
igmp_enc_msg_query (struct igmp_if *igif,
                    struct igmp_group_rec *igr,
                    u_int16_t num_srcs,
                    struct ptree *psrcs_tib,
                    enum igmp_gs_query_type *query_type,
                    u_int16_t *num_enc_srcs,
                    u_int16_t *msg_len)
{
  struct igmp_source_rec *isr;
  struct igmp_instance *igi;
  struct ptree_node *pn;
  u_int16_t count_srcs;
  u_int32_t putp_pos1;
  u_int32_t putp_pos2;
  u_int32_t start_pos;
  u_int32_t qqic_u32;
  u_int32_t rem_time;
  u_int32_t end_pos;
  u_int8_t resv_qrv;
  float32_t tmp_f32;
  struct stream *s;
  u_int32_t lmqt;
  s_int32_t ret;
  u_int8_t qqic;

  IGMP_FN_ENTER (Enc Query);

  ret = IGMP_ERR_NONE;
  count_srcs = 0;
  tmp_f32 = 0;
  lmqt = 0;

  /* Sanity checks */
  if (! igif
      || ! (igi = igif->igif_owning_igi)
      || ! (s = igi->igi_o_buf)
      || (num_srcs && (! num_enc_srcs || ! query_type)))
    {
      ret = IGMP_ERR_MALFORMED_ARG;
      goto EXIT;
    }

  /* Msg Length initilization */
  (*msg_len) = 0;
  start_pos = STREAM_GET_PUTP (s);
  end_pos = start_pos;

  /* Message size check */
  if (STREAM_REMAIN (s) < IGMP_MSG_QUERY_MIN_SIZE)
    {
      IGMP_DBG_INFO (igi, ENCODE, "Mesg size < Query Header");

      ret = IGMP_ERR_GENERIC;
      goto EXIT;
    }

  /* Update encoded number of sources */
  if (num_enc_srcs)
    *num_enc_srcs = 0;

  /* Encode Group Address */
  if (igr)
    {
      stream_put_in_addr (s, (struct pal_in4_addr *)
                              PTREE_NODE_KEY (igr->igr_owning_pn));

      /* Determine the LMQT */
      lmqt = (MSEC2SECROUND(igr->igr_owning_igif->igif_conf.igifc_lmqi)) *
             (igr->igr_owning_igif->igif_conf.igifc_lmqc);

      /* Lower Group Timer to LMQT for the first Grp-Specific Query */
      if (! num_srcs
          && thread_timer_remain_second (igr->t_igr_liveness) > lmqt
          && igr->igr_rexmit_group_lmqc ==
             igr->igr_owning_igif->igif_conf.igifc_lmqc)
        {
          IGMP_TIMER_OFF (IGMP_LG (igi), igr->t_igr_liveness);
          igr->v_igr_liveness = lmqt;
          IGMP_TIMER_ON (IGMP_LG (igi), igr->t_igr_liveness,
                           igr, igmp_if_grec_timer_liveness,
                           igr->v_igr_liveness);
        }
    }
  else
    stream_putl (s, 0);

  /* Encode IGMPv3 specific fields */
  if (igif->igif_conf.igifc_version == IGMP_VERSION_3)
    {
      /* Message size check */
      if (STREAM_REMAIN (s) < IGMP_MSG_QUERY_V3_ADDITIONAL_MIN_SIZE)
        {
          IGMP_DBG_INFO (igi, ENCODE, "Mesg size < v3 Query Add. Header");

          ret = IGMP_ERR_GENERIC;
          goto EXIT;
        }

      /* Encode Resvd & QRV Field */
      resv_qrv = 0;
      if (igif->igif_rv <= IGMP_ROBUSTNESS_VAR_MAX)
        resv_qrv |= (igif->igif_rv & IGMP_MSG_QUERY_QRV_MASK);
      if ((! num_srcs
           && igr
           && igr->igr_rexmit_group_lmqc <
              igr->igr_owning_igif->igif_conf.igifc_lmqc)
          || (num_srcs
              && *query_type == IGMP_GS_QUERY_SFLAG_SET
              && igr 
              && igr->igr_rexmit_group_source_lmqc <
                 igr->igr_owning_igif->igif_conf.igifc_lmqc))
        resv_qrv |= IGMP_MSG_QUERY_SUPPRESS_FLAG_MASK;
      stream_putc (s, resv_qrv);

      /* Encode QQIC Field */
      qqic_u32 = igif->igif_qi;
      if (qqic_u32 < IGMP_MSG_TIME_INTERVAL_EXPONENT)
        qqic = qqic_u32;
      else
        {
          tmp_f32 = (float32_t)qqic_u32;
          qqic = IGMP_GET_INT32_MANT (tmp_f32);
          qqic |= (IGMP_GET_INT32_EXPONENT (tmp_f32)
                   << IGMP_MSG_TIME_INTERVAL_MANTISSA_BITSIZE);
          qqic |= (u_int8_t) IGMP_MSG_TIME_INTERVAL_EXPONENT;
        }
      stream_putc (s, qqic);

      /* Encode Number of Sources Field */
      putp_pos1 = STREAM_GET_PUTP (s);
      stream_putw (s, 0);

      /* Encode List of Sources */
      for (pn = ptree_top (psrcs_tib), count_srcs = 0;
           pn; pn = ptree_next (pn))
        {
          if (! (isr = pn->info))
            continue;

          /* If a cloned Src-Rec, grab the real one from TIB-A */
          if (igr && igr->igr_src_a_tib != psrcs_tib)
            {
              isr = igmp_if_srec_lookup (igr, PAL_TRUE,
                                         (struct pal_in4_addr *)
                                         PTREE_NODE_KEY (pn));

              if (! isr)
                {
                  IGMP_DBG_WARN (igi, ENCODE, "Failed to find real ",
                                 "Src-Rec from a Clone!");

                  continue;
                }
            }
 
          if (CHECK_FLAG (isr->isr_sflags, IGMP_ISR_SFLAG_STATIC))
            continue;

          /* Get the remaining time for Src-Rec Timer */
          rem_time = thread_timer_remain_second (isr->t_isr_liveness);

          /* Group Sources with Timers active/inactive */
          if ((*query_type == IGMP_GS_QUERY_SFLAG_SET
               && rem_time <= lmqt)
              || (*query_type == IGMP_GS_QUERY_SFLAG_UNSET
                  && rem_time > lmqt))
            continue;

          /* Lower Source Timer to LMQT for the first Query */
          if (rem_time > lmqt && 
              (igr 
              && igr->igr_rexmit_group_source_lmqc ==
                 igr->igr_owning_igif->igif_conf.igifc_lmqc))
            {
              isr->v_isr_liveness = lmqt;
              IGMP_TIMER_OFF (IGMP_LG (igi), isr->t_isr_liveness);
              IGMP_TIMER_ON (IGMP_LG (igi), isr->t_isr_liveness,
                             isr, igmp_if_srec_timer_liveness,
                             isr->v_isr_liveness);
            }

          /* Message size check */
          if (STREAM_REMAIN (s) < IGMP_MSG_QUERY_MIN_SIZE)
            {
              IGMP_DBG_INFO (igi, ENCODE, "Mesg size < Min for Src Filed");

              ret = IGMP_ERR_GENERIC;
              goto EXIT;
            }

          /* Encode the Source Address */
          stream_put_in_addr (s, (struct pal_in4_addr *)
                                 PTREE_NODE_KEY (pn));

          count_srcs += 1;
        }

      /* Re-encode the num-sources */
      if (num_srcs)
        {
          putp_pos2 = STREAM_GET_PUTP (s);
          STREAM_GET_PUTP (s) = putp_pos1;
          stream_putw (s, count_srcs);
          STREAM_GET_PUTP (s) = putp_pos2;

          /* Update the Query Type for next query */
          if (num_srcs == count_srcs)
            *query_type = IGMP_GS_QUERY_DONE;
          else if (*query_type == IGMP_GS_QUERY_SFLAG_SET)
            *query_type = IGMP_GS_QUERY_SFLAG_UNSET;
          else if (*query_type == IGMP_GS_QUERY_SFLAG_UNSET)
            *query_type = IGMP_GS_QUERY_DONE;
        }
    }

  /* Update encoded number of sources */
  if (num_enc_srcs)
    *num_enc_srcs = count_srcs;

  /* Set Msg-Len and Reset Stream to Start */
  end_pos = STREAM_GET_PUTP (s);
  (*msg_len) += end_pos - start_pos;
  STREAM_REWIND_PUTP (s, (*msg_len));

EXIT:

  IGMP_FN_EXIT (ret);
}

/* Encode IGMP Message Header */
s_int32_t
igmp_enc_msg_query_hdr (struct igmp_if *igif,
                        u_int32_t max_rsp_time,
                        u_int16_t *msg_len)
{
  struct igmp_instance *igi;
  u_int8_t max_rsp_code;
  u_int16_t calc_csum;
  float32_t tmp_f32;
  struct stream *s;
  s_int32_t ret;

  IGMP_FN_ENTER (Enc Query Hdr);

  ret = IGMP_ERR_NONE;
  max_rsp_code = 0;
  tmp_f32 = 0;

  if (! igif
      || ! (igi = igif->igif_owning_igi)
      || ! (s = igi->igi_o_buf))
    {
      ret = IGMP_ERR_MALFORMED_ARG;
      goto EXIT;
    }

  /* Rewind to point to start of IGMP Mesg Header */
  STREAM_REWIND_PUTP (s, IGMP_MSG_HEADER_SIZE);

  if (igif->igif_conf.igifc_version == IGMP_VERSION_2)
    {
      max_rsp_code = (u_int8_t) max_rsp_time;
    }
  /* Max Response Time field units is 1/10 second.  */
  else if (igif->igif_conf.igifc_version == IGMP_VERSION_3)
    {
      if (max_rsp_time < IGMP_MSG_TIME_INTERVAL_EXPONENT)
        max_rsp_code = (u_int8_t) max_rsp_time;
      else
        {
          tmp_f32 = (float32_t)max_rsp_time;
          max_rsp_code = IGMP_GET_INT32_MANT (tmp_f32);
          max_rsp_code |= (IGMP_GET_INT32_EXPONENT (tmp_f32)
                           << IGMP_MSG_TIME_INTERVAL_MANTISSA_BITSIZE);
          max_rsp_code |= (u_int8_t) IGMP_MSG_TIME_INTERVAL_EXPONENT;
        }
    }

  /* Encode IGMP Msg Type */
  stream_putc (s, IGMP_MSG_MEMBERSHIP_QUERY);

  /* Encode Max. Response Code */
  stream_putc (s, max_rsp_code);

  /* Zero the Checksum field */
  stream_putw (s, 0);

  /* Set Message Length */
  (*msg_len) += IGMP_MSG_HEADER_SIZE;

  /* Rewind to start of IGMP Mesg Header */
  STREAM_REWIND_PUTP (s, IGMP_MSG_HEADER_SIZE);

  /* Calculate Checksum */
  calc_csum = in_checksum ((u_int16_t *) STREAM_PUT_PNT (s), (*msg_len));

  /* Encode the Checksum */
  STREAM_FORWARD_PUTP (s, IGMP_MSG_HEADER_SIZE - sizeof (calc_csum));
  stream_put (s, &calc_csum, sizeof (calc_csum));

  /* Again Rewind to start of IGMP Mesg Header */
  STREAM_REWIND_PUTP (s, IGMP_MSG_HEADER_SIZE);

  IGMP_DBG_INFO (igi, ENCODE, "%s Checksum=%u, MsgLen=%u",
                 IGMP_MSG_TYPE_STR (IGMP_MSG_MEMBERSHIP_QUERY),
                 pal_ntoh16 (calc_csum), (*msg_len));

EXIT:

  IGMP_FN_EXIT (ret);
}

/* Encode IGMP V3 Report Message */
s_int32_t
igmp_enc_msg_v3_report (struct igmp_if *hst_igif,
                        struct igmp_group_rec *hst_igr,
                        struct ptree *psrcs_tib,
                        u_int16_t *msg_len,
                        struct ptree_node **last_grp_enc,
                        struct ptree_node **last_src_enc)
{
  enum igmp_hst_rec_type msg_type;
  struct igmp_instance *igi;
  u_int32_t num_enc_grps;
  struct ptree_node *pn;
  u_int16_t calc_csum;
  u_int32_t start_pos;
  u_int32_t end_pos;
  struct stream *s;
  s_int32_t ret;

  IGMP_FN_ENTER (Enc V3 Report);

  msg_type = IGMP_HRT_INVALID;
  ret = IGMP_ERR_NONE;
  num_enc_grps = 0;

  /* Sanity checks */
  if (! hst_igif
      || ! msg_len
      || ! (igi = hst_igif->igif_owning_igi)
      || ! (s = igi->igi_o_buf)
      || (psrcs_tib
          && ! hst_igr)
      || ! (hst_igr || last_grp_enc)
      || ! last_src_enc)
    {
      ret = IGMP_ERR_MALFORMED_ARG;
      goto EXIT;
    }

  /* Msg Length initilization */
  (*msg_len) = 0;
  start_pos = STREAM_GET_PUTP (s);
  end_pos = start_pos;

  /* Message size check */
  if (STREAM_REMAIN (s) < IGMP_MSG_V3_REPORT_MIN_SIZE)
    {
      IGMP_DBG_INFO (igi, ENCODE, "Msg size < v3 Report Hdr");

      ret = IGMP_ERR_BUF_TOO_SHORT;
      goto EXIT;
    }

  /* Forward by v3 Report Header */
  STREAM_FORWARD_PUTP (s, IGMP_MSG_V3_REPORT_MIN_SIZE);

  /* Determine Group(s) to encode */
  if (hst_igr)
    {
       ret = igmp_enc_msg_v3_report_grec
             (hst_igr,
              hst_igr->igr_rexmit_hrt == IGMP_HRT_INVALID ?
              hst_igr->igr_filt_mode_state :
              hst_igr->igr_rexmit_hrt,
              psrcs_tib, msg_len, last_src_enc);

      if (ret == IGMP_ERR_BUF_TOO_SHORT)
        num_enc_grps += 1;

      if (ret < IGMP_ERR_NONE)
        {
          IGMP_DBG_INFO (igi, ENCODE, "%r G-Rec Encode Failed",
                         PTREE_NODE_KEY (hst_igr->igr_owning_pn));
        }
      else
        num_enc_grps += 1;
    }
    else for (pn = (last_grp_enc && *last_grp_enc) ?
              *last_grp_enc : ptree_top (hst_igif->igif_hst_gmr_tib);
              pn; pn = ptree_next (pn))

    {
      if (! (hst_igr = pn->info))
        continue;

      if ( hst_igr->igr_filt_mode_state == IGMP_FMS_INCLUDE &&
            ! hst_igr->igr_src_a_tib_count)
        continue;

      /* Set Report-Pending Flag to ensure GRec Encoding */
      SET_FLAG (hst_igr->igr_sflags,
                IGMP_IGR_SFLAG_REPORT_PENDING);

      ret = igmp_enc_msg_v3_report_grec
            (hst_igr, hst_igr->igr_filt_mode_state,
             NULL, msg_len, last_src_enc);

      /* UnSet Report-Pending Flag */
      UNSET_FLAG (hst_igr->igr_sflags,
                  IGMP_IGR_SFLAG_REPORT_PENDING);

      if (ret == IGMP_ERR_NO_SRC_ENC)
        {
          /* Save current group for the next report */
          *last_grp_enc = pn;
          ret = IGMP_ERR_BUF_TOO_SHORT;
          break;
        }

      if (ret == IGMP_ERR_BUF_TOO_SHORT)
        {
          /* Save current group for the next report */
          *last_grp_enc = pn;
          num_enc_grps += 1;
          break;
        }

      if (ret < IGMP_ERR_NONE)
        {
          IGMP_DBG_INFO (igi, ENCODE, "%r G-Rec Encode Failed",
                         PTREE_NODE_KEY (hst_igr->igr_owning_pn));
          /* Loose the error code */
          ret = IGMP_ERR_NONE;
          break;
        }

      num_enc_grps += 1;
    }

  if (! num_enc_grps)
    {
      IGMP_DBG_INFO (igi, ENCODE, "ZERO G-Recs were Encoded!");

      ret = IGMP_ERR_GENERIC;
      goto EXIT;
    }

  /* Set Msg-Len and Reset Stream to Start */
  end_pos = STREAM_GET_PUTP (s);
  STREAM_REWIND_PUTP (s, (*msg_len) +
                         IGMP_MSG_V3_REPORT_MIN_SIZE);

  /* Encode IGMP Msg Type */
  stream_putc (s, IGMP_MSG_V3_MEMBERSHIP_REPORT);

  /* Encode Reserved Field */
  stream_putc (s, 0);

  /* Zero the Checksum field */
  stream_putw (s, 0);

  /* Encode Reserved Field */
  stream_putw (s, 0);

  /* Encode Num G-Recs Encoded */
  stream_putw (s, num_enc_grps);

  /* Set Message Length */
  (*msg_len) += IGMP_MSG_V3_REPORT_MIN_SIZE;

  /* Rewind to start of IGMP Mesg Header */
  STREAM_REWIND_PUTP (s, IGMP_MSG_V3_REPORT_MIN_SIZE);

  /* Calculate Checksum */
  calc_csum = in_checksum ((u_int16_t *) STREAM_PUT_PNT (s),
                           (*msg_len));

  /* Encode the Checksum */
  STREAM_FORWARD_PUTP (s, IGMP_MSG_HEADER_SIZE - sizeof (calc_csum));
  stream_put (s, &calc_csum, sizeof (calc_csum));

  /* Again Rewind to start of IGMP Mesg Header */
  STREAM_REWIND_PUTP (s, IGMP_MSG_HEADER_SIZE);

  IGMP_DBG_INFO (igi, ENCODE, "%s Checksum=%u, MsgLen=%u",
                 IGMP_MSG_TYPE_STR (IGMP_MSG_V3_MEMBERSHIP_REPORT),
                 pal_ntoh16 (calc_csum), (*msg_len));

EXIT:

  IGMP_FN_EXIT (ret);
}

/* Encode IGMP V3 Report Group-Record Field */
s_int32_t
igmp_enc_msg_v3_report_grec (struct igmp_group_rec *hst_igr,
                             enum igmp_hst_rec_type igr_hrt,
                             struct ptree *psrcs_tib,
                             u_int16_t *msg_len,
                             struct ptree_node **last_src_enc)
{
  struct igmp_source_rec *isr;
  struct igmp_instance *igi;
  u_int16_t num_enc_srcs;
  struct ptree_node *pn;
  bool_t pend_srcs_only;
  u_int32_t start_pos;
  u_int32_t put_pos;
  u_int32_t end_pos;
  struct stream *s;
  s_int32_t ret;

  IGMP_FN_ENTER (Enc V3 Report GRec);

  pend_srcs_only = PAL_FALSE;
  ret = IGMP_ERR_NONE;
  num_enc_srcs = 0;

  /* Sanity checks */
  if (! hst_igr
      || ! (igi = hst_igr->igr_owning_igif->igif_owning_igi)
      || ! (s = igi->igi_o_buf)
      || ! msg_len
      || ! last_src_enc)
    {
      ret = IGMP_ERR_MALFORMED_ARG;
      goto EXIT;
    }

  /* Msg Encoding stamps */
  start_pos = STREAM_GET_PUTP (s);
  end_pos = start_pos;

  /* Message size check - Header and atleast one source should fit in */
  if (STREAM_REMAIN (s) < IGMP_MSG_V3_REPORT_GRP_REC_MIN_SIZE
                          + IGMP_MSG_V3_REPORT_SRC_REC_MIN_SIZE)
    {
      IGMP_DBG_INFO (igi, ENCODE, "Msg size < Grp-Rec Header");

      ret = IGMP_ERR_NO_SRC_ENC;
      goto EXIT;
    }

  /* Forward by Grp-Rec Header */
  STREAM_FORWARD_PUTP (s, IGMP_MSG_V3_REPORT_GRP_REC_MIN_SIZE);

  /* Select the Srcs-TIB to encode */
  if (! psrcs_tib)
    {
      if (hst_igr->igr_rexmit_hrt == IGMP_HRT_INVALID)
        pend_srcs_only = PAL_TRUE;

      switch (hst_igr->igr_filt_mode_state)
        {
        case IGMP_HFMS_INCLUDE:
          psrcs_tib = hst_igr->igr_src_a_tib;
          break;

        case IGMP_HFMS_EXCLUDE:
          psrcs_tib = hst_igr->igr_src_b_tib;
          break;

        default:
          ret = IGMP_ERR_DOOM;
          goto EXIT;
        }
    }

  /* Record Encoding stamp */
  put_pos = STREAM_GET_PUTP (s);
 
  /* Start from last encoded source */ 
  if (last_src_enc && *last_src_enc)
    {
      pn = *last_src_enc;
      *last_src_enc = NULL;
    }
  else /* Start from beginnng */
    pn = ptree_top (psrcs_tib);

  /* Encode the Srcs-List */
  for (; pn; pn = ptree_next (pn))
    {
      if (! (isr = pn->info))
        continue;

      if (pend_srcs_only
          && ! CHECK_FLAG (isr->isr_sflags,
                           IGMP_ISR_SFLAG_REPORT_PENDING)
          && ! CHECK_FLAG (hst_igr->igr_sflags,
                           IGMP_IGR_SFLAG_REPORT_PENDING))
        continue;

      if (STREAM_REMAIN (s) < IGMP_MSG_V3_REPORT_SRC_REC_MIN_SIZE)
        {
          if (hst_igr->igr_rexmit_hrt == IGMP_HRT_MODE_IS_EXCL
              || hst_igr->igr_rexmit_hrt == IGMP_HRT_CHG_TO_EXCL)
            {
              /* Discard the remaining sources */
              *last_src_enc = NULL;
              ret = IGMP_ERR_NONE;
            }
          else
            {
              /* Save current source for the next report */
              *last_src_enc = pn;
              IGMP_DBG_INFO (igi, ENCODE, "Msg size < Src-Rec Field");
              ret = IGMP_ERR_BUF_TOO_SHORT;
            }
          break;
        }

      num_enc_srcs += 1;
      stream_put_in_addr (s, (struct pal_in4_addr *)
                              PTREE_NODE_KEY (isr->isr_owning_pn));
      UNSET_FLAG (isr->isr_sflags, IGMP_ISR_SFLAG_REPORT_PENDING);
    }

  /* Set Msg-Len and Reset Stream to Start */
  end_pos = STREAM_GET_PUTP (s);
  (*msg_len) += end_pos - start_pos;
  STREAM_REWIND_PUTP (s, end_pos - start_pos);

  /* Encode Grp-Rec Header */
  stream_putc (s, (u_int8_t) igr_hrt);
  stream_putc (s, (u_int8_t) 0);
  stream_putw (s, num_enc_srcs);
  stream_put_in_addr (s, (struct pal_in4_addr *)
                          PTREE_NODE_KEY (hst_igr->igr_owning_pn));

  /* Reposition stream */
  STREAM_FORWARD_PUTP (s, end_pos - put_pos);

EXIT:

  IGMP_FN_EXIT (ret);
}

/* Encode IGMP V2 Report/ Leave Message */
s_int32_t
igmp_enc_msg_v1_v2_report_leave (struct igmp_group_rec *hst_igr,
                                 u_int8_t msg_type,
                                 u_int16_t *msg_len)
{
  struct igmp_instance *igi;
  u_int16_t calc_csum;
  u_int32_t start_pos;
  u_int32_t end_pos;
  struct stream *s;
  s_int32_t ret;

  IGMP_FN_ENTER (Enc V1/V2 Report);

  ret = IGMP_ERR_NONE;

  if (! hst_igr
      || ! msg_len
      || ! (igi = hst_igr->igr_owning_igif->igif_owning_igi)
      || ! (s = igi->igi_o_buf))
    {
      ret = IGMP_ERR_MALFORMED_ARG;
      goto EXIT;
    }

  /* Msg Encoding stamps */
  start_pos = STREAM_GET_PUTP (s);
  end_pos = start_pos;
  (*msg_len) = 0;

  /* Encode IGMP Msg Type */
  stream_putc (s, msg_type);

  /* Encode Reserved Field */
  stream_putc (s, 0);

  /* Zero the Checksum field */
  stream_putw (s, 0);

  /* Encode the Group Address */
  stream_put_in_addr (s, (struct pal_in4_addr *)
                         PTREE_NODE_KEY (hst_igr->igr_owning_pn));

  /* Set Msg-Len and Reset Stream to Start */
  end_pos = STREAM_GET_PUTP (s);
  (*msg_len) += end_pos - start_pos;
  STREAM_REWIND_PUTP (s, (*msg_len));

  /* Calculate Checksum */
  calc_csum = in_checksum ((u_int16_t *) STREAM_PUT_PNT (s),
                           (*msg_len));


  /* Encode the Checksum */
  STREAM_FORWARD_PUTP (s, IGMP_MSG_HEADER_SIZE - sizeof (calc_csum));
  stream_put (s, &calc_csum, sizeof (calc_csum));

  /* Again Rewind to start of IGMP Mesg Header */
  STREAM_REWIND_PUTP (s, IGMP_MSG_HEADER_SIZE);

  IGMP_DBG_INFO (igi, ENCODE, "%s Checksum=%u, MsgLen=%u",
                 IGMP_MSG_TYPE_STR (msg_type),
                 pal_ntoh16 (calc_csum), (*msg_len));

EXIT:

  IGMP_FN_EXIT (ret);
}

/* IGMP Socket Write */
s_int32_t
igmp_sock_write (struct igmp_if *igif,
                 struct pal_in4_addr *psrc,
                 struct pal_in4_addr *pdst,
                 u_int16_t msg_len)
{
  struct igmp_in4_header *iph_ra;
  union igmp_in4_cmsg igcmsg;
  struct igmp_svc_reg *igsr;
  struct igmp_instance *igi;
  struct listnode *nn;
  u_int32_t igcmsglen;
  struct stream *s;
  s_int32_t ret;

  IGMP_FN_ENTER (IGMP Write);

  ret = IGMP_ERR_NONE;

  igi = igif->igif_owning_igi;
  s = igi->igi_o_buf;

  /* Prepare the IPv4 header */
  STREAM_REWIND_PUTP (s, sizeof (struct igmp_in4_header));
  iph_ra = (struct igmp_in4_header *) STREAM_PUT_PNT (s);
  if (psrc && pdst)
    pal_mem_set (iph_ra, 0, sizeof (struct igmp_in4_header));
  pal_in4_ip_hdr_len_set (&iph_ra->iph, IGMP_RA_SIZE);
  iph_ra->iph.ip_v = IPVERSION;
  pal_in4_ip_hdr_tos_set (&iph_ra->iph,
                          PAL_IPTOS_PREC_INTERNETCONTROL);
  pal_in4_ip_packet_len_set (&iph_ra->iph, msg_len);
  iph_ra->iph.ip_id = 0;
  iph_ra->iph.ip_off = 0;
  iph_ra->iph.ip_ttl = IGMP_MSG_TTL_DEF;
  iph_ra->iph.ip_p = IPPROTO_IGMP;
  iph_ra->iph.ip_sum = 0;
  if (psrc)
    iph_ra->iph.ip_src = *psrc;
  if (pdst)
    iph_ra->iph.ip_dst = *pdst;
  iph_ra->ra_opt [0] = IGMP_IPOPT_RA;
  iph_ra->ra_opt [1] = IGMP_RA_SIZE;

  /* Update IP Header checksum */
  iph_ra->iph.ip_sum = in_checksum ((u_int16_t *) iph_ra,
                                    sizeof (struct igmp_in4_header));

  /* Update Msg-Len to include IPv4 Hdr + RA Opt */
  msg_len += sizeof (struct igmp_in4_header);

  /* Send Message on IGMP Socket */
  LIST_LOOP (&igi->igi_svc_reg_lst, igsr, nn)
    {
      if (! igmp_if_svc_reg_match (igif, igsr))
        continue;

      switch (igsr->igsr_svc_type)
        {
        case IGMP_SVC_TYPE_L2:
          iph_ra->iph.ip_len = pal_hton16 (iph_ra->iph.ip_len);
          pal_mem_set (&igcmsg, 0, sizeof (union igmp_in4_cmsg));
          IGMP_CONVERT_IPV4MCADDR_TO_MAC (&iph_ra->iph.ip_dst,
                                          igcmsg.vaddr.dest_mac);
          igcmsg.vaddr.port = igif->igif_owning_ifp->ifindex;
          igcmsg.vaddr.vlanid = IGMP_IF_GET_VID (igif->igif_su_id);
          pal_mem_cpy (igcmsg.vaddr.src_mac,igif->igif_owning_ifp->hw_addr,
                       ETHER_ADDR_LEN);
          igcmsglen = sizeof (igcmsg.vaddr);
          break;

        case IGMP_SVC_TYPE_L3:
          pal_mem_set (&igcmsg, 0, sizeof (union igmp_in4_cmsg));
          igcmsg.sin4.sin_family = AF_INET;
          igcmsg.sin4.sin_port = igif->igif_owning_ifp->ifindex;
#ifdef HAVE_SIN_LEN
          igcmsg.sin4.sin_len = sizeof (struct pal_sockaddr_in4);
#endif /* HAVE_SIN_LEN */
          igcmsg.sin4.sin_addr = *pdst;
          igcmsglen = sizeof (igcmsg.sin4);

          /* Set Multicast-IF Socket Option */
          ret = pal_sock_set_ipv4_multicast_if 
                (igsr->igsr_sock,
                *psrc,
                igif->igif_owning_ifp->ifindex);

          if (ret < 0)
            IGMP_DBG_WARN (igi, ENCODE, "Setsockopt IP_MCAST_IF"
                           " failed: %s(%d)!",
                           pal_strerror (errno), errno);
          break;

        default:
          ret = IGMP_ERR_DOOM;
          goto EXIT;
        }

      ret = pal_sock_sendto (igsr->igsr_sock,
                             STREAM_PUT_PNT (s),
                             msg_len,
                             MSG_DONTWAIT,
                             &igcmsg.sa,
                             igcmsglen);

      if (ret < 0)
        IGMP_DBG_ERR (igi, ENCODE, "sendto() failed: %s(%d)",
                      pal_strerror (errno), errno);
    }

EXIT:

  STREAM_FORWARD_PUTP (s, sizeof (struct igmp_in4_header));

  IGMP_FN_EXIT (ret);
}

