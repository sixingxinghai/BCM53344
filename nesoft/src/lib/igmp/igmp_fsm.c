/* Copyright (C) 2002-2003  All Rights Reserved. */

#include <igmp_incl.h>

/*********************************************************************/
/*                   IGMP ROUTER-SIDE FSM                            */
/*********************************************************************/

/* IGMP G-Rec FSM Action Function Array indexed by current state */
igmp_igr_fsm_act_func_t igmp_igr_fsm_action_func [IGMP_FMS_MAX] =
{
  igmp_igr_fsm_action_invalid,
  igmp_igr_fsm_action_include,
  igmp_igr_fsm_action_exclude
};

/* IGMP Group Record FSM Event Handler */
s_int32_t
igmp_igr_fsm_process_event (struct igmp_group_rec *igr,
                            enum igmp_filter_mode_event igr_fme,
                            struct igmp_source_list *isl)
{
  struct igmp_instance *igi;
  s_int32_t ret;

  IGMP_FN_ENTER (Process Event);

  ret = IGMP_ERR_NONE;

  /* Sanity check */
  if (! igr)
    {
      ret = IGMP_ERR_MALFORMED_ARG;
      goto EXIT;
    }

  igi = igr->igr_owning_igif->igif_owning_igi;

  IGMP_DBG_INFO (igi, FSM, "I=%s, G=%r, State: %s, Event: %s",
                 igr->igr_owning_igif->igif_owning_ifp->name,
                 PTREE_NODE_KEY (igr->igr_owning_pn),
                 IGMP_FMS_STR (igr->igr_filt_mode_state),
                 IGMP_FME_STR (igr_fme));

  /* Invoke Action-Func defined for the current-state */
  ret = igmp_igr_fsm_action_func [igr->igr_filt_mode_state]
        (igr, igr_fme, isl);

EXIT:

  IGMP_FN_EXIT (ret);
}

/* Change IGMP Group Rec FSM State */
s_int32_t
igmp_igr_fsm_change_state (struct igmp_group_rec *igr,
                           enum igmp_filter_mode_state igr_fms)
{
  struct igmp_instance *igi;
  s_int32_t ret;

  IGMP_FN_ENTER (State Change);

  igi = igr->igr_owning_igif->igif_owning_igi;
  ret = IGMP_ERR_NONE;

  IGMP_DBG_INFO (igi, FSM, "%s(%d)->%s(%d)",
                IGMP_FMS_STR (igr->igr_filt_mode_state),
                igr->igr_filt_mode_state,
                IGMP_FMS_STR (igr_fms), igr_fms);

  /* Change to new state */
  igr->igr_filt_mode_state = igr_fms;

  IGMP_FN_EXIT (ret);
}

/* IGMP FSM action function for Invalid-state */
s_int32_t
igmp_igr_fsm_action_invalid (struct igmp_group_rec *igr,
                             enum igmp_filter_mode_event igr_fme,
                             struct igmp_source_list *isl)
{
  struct igmp_instance *igi;
  s_int32_t ret;

  IGMP_FN_ENTER (Act Invalid);

  igi = igr->igr_owning_igif->igif_owning_igi;
  ret = IGMP_ERR_DOOM;

  IGMP_DBG_ERR (igi, FSM, "!!! State: INVALID Event: %s(%d)",
                IGMP_FME_STR (igr_fme), igr_fme);

  pal_assert (0);

  IGMP_FN_EXIT (ret);
}

/* IGMP FSM action function for Include-state */
s_int32_t
igmp_igr_fsm_action_include (struct igmp_group_rec *igr,
                             enum igmp_filter_mode_event igr_fme,
                             struct igmp_source_list *isl)
{
  struct igmp_source_rec *isr_tmp;
  struct ptree *intersection_tib;
  struct igmp_source_rec *isr;
  struct ptree_node *pn_next;
  struct igmp_instance *igi;
  struct ptree_node *pn_sec;
  u_int8_t  del_rexmit_tib;
  struct ptree *minus_tib;
  bool_t send_indication;
  struct ptree_node *pn;
  u_int16_t num_srcs;
  u_int32_t idx;
  s_int32_t ret;

  IGMP_FN_ENTER (Act Include);

  igi = igr->igr_owning_igif->igif_owning_igi;
  send_indication = PAL_FALSE;
  del_rexmit_tib = PAL_FALSE;
  intersection_tib = NULL;
  ret = IGMP_ERR_NONE;
  minus_tib = NULL;
  isr_tmp = NULL;
  num_srcs = 0;
  isr = NULL;

  switch (igr_fme)
    {
    case IGMP_FME_MODE_IS_INCL:
      if (CHECK_FLAG (igr->igr_sflags, IGMP_IGR_SFLAG_COMPAT_V1)
          || CHECK_FLAG (igr->igr_sflags, IGMP_IGR_SFLAG_COMPAT_V2)
          || CHECK_FLAG (igr->igr_sflags, IGMP_IGR_SFLAG_COMPAT_V3))
        {
          /* Merge Source-List into TIB-A */
          for (idx = 0; isl && idx < isl->isl_num; idx++)
            {
              if ((isr = igmp_if_srec_lookup (igr, PAL_TRUE,
                                              &isl->isl_saddr [idx])))
                {
                  /* Reset Src-Timer to GMI */
                  isr->v_isr_liveness = igr->v_igr_liveness;
                  IGMP_TIMER_OFF (IGMP_LG (igi), isr->t_isr_liveness);
                  IGMP_TIMER_ON (IGMP_LG (igi), isr->t_isr_liveness,
                                 isr, igmp_if_srec_timer_liveness,
                                 isr->v_isr_liveness);
                }
              else
                {
                  if ((isr = igmp_if_srec_get (igr, PAL_TRUE,
                                               &isl->isl_saddr [idx])))
                    {
                      /* Reset Src-Timer to GMI */
                      isr->v_isr_liveness = igr->v_igr_liveness;
                      IGMP_TIMER_OFF (IGMP_LG (igi), isr->t_isr_liveness);
                      IGMP_TIMER_ON (IGMP_LG (igi), isr->t_isr_liveness,
                                     isr, igmp_if_srec_timer_liveness,
                                     isr->v_isr_liveness);

                      send_indication = PAL_TRUE;
                    }
                  else
                    {
                      IGMP_DBG_ERR (igi, TIB, "Failed to get Src-Rec for"
                                    " %r @ %s:%d", &isl->isl_saddr [idx],
                                    __FILE__, __LINE__);

                      continue;
                    }
                }
              if (isr)
                {
                  if (! isl->isl_static)
                    SET_FLAG (isr->isr_sflags, IGMP_ISR_SFLAG_DYNAMIC);
                  else
                    SET_FLAG (isr->isr_sflags, IGMP_ISR_SFLAG_STATIC);
                }
            }

          ret = igmp_igr_fsm_change_state (igr, IGMP_FMS_INCLUDE);

          if (! send_indication)
            send_indication = (CHECK_FLAG (igr->igr_sflags,
                                           IGMP_IGR_SFLAG_STATE_REFRESH));

          /* Send INCL <NULL> Indication (-> G-rec Del) */
          if (! igr->igr_src_a_tib_count)
            {
              ret = igmp_igr_fsm_send_lmem_update (igr);

              ret = igmp_if_grec_delete (igr);
            }
          else if (send_indication)
            ret = igmp_igr_fsm_send_lmem_update (igr);
        }
      break;

    case IGMP_FME_MODE_IS_EXCL:
      if (CHECK_FLAG (igr->igr_sflags, IGMP_IGR_SFLAG_COMPAT_V1)
          || CHECK_FLAG (igr->igr_sflags, IGMP_IGR_SFLAG_COMPAT_V2)
          || CHECK_FLAG (igr->igr_sflags, IGMP_IGR_SFLAG_COMPAT_V3))
        {
          intersection_tib = ptree_init (IPV4_MAX_BITLEN);

          if (! intersection_tib)
            {
              IGMP_DBG_ERR (igi, TIB, "P-Trie Init Failed %s:%d",
                            __FILE__, __LINE__);

              ret = IGMP_ERR_OOM;
              goto EXIT;
            }

          minus_tib = ptree_init (IPV4_MAX_BITLEN);

          if (! minus_tib)
            {
              IGMP_DBG_ERR (igi, TIB, "P-Trie Init Failed %s:%d",
                            __FILE__, __LINE__);

              ret = IGMP_ERR_OOM;
              goto EXIT;
            }

          for (idx = 0; isl && idx < isl->isl_num; idx++)
            {
              if ((isr = igmp_if_srec_lookup (igr, PAL_TRUE,
                                              &isl->isl_saddr [idx])))
                {
                  pn = ptree_node_get (intersection_tib,
                                       (u_int8_t *) &isl->isl_saddr [idx],
                                       IPV4_MAX_PREFIXLEN);

                  if (! pn)
                    {
                      IGMP_DBG_ERR (igi, TIB, "Failed to get P-Trie "
                                    "node for %r @ %s:%d",
                                    &isl->isl_saddr [idx],
                                    __FILE__, __LINE__);

                      continue;
                    }

                  /* Switch over the Src-Rec to Xtion TIB */
                  if (isr->isr_owning_pn)
                    isr->isr_owning_pn->info = NULL;
                  pn->info = isr;
                  isr->isr_owning_pn = pn;
                }
              else
                {
                  /* Create a new Src-Rec */
                  ret = igmp_if_srec_create (igr, &isr);

                  if (ret < IGMP_ERR_NONE)
                    {
                      IGMP_DBG_ERR (igi, TIB, "Failed to create Src node"
                                    " for %r @ %s:%d", &isl->isl_saddr [idx],
                                    __FILE__, __LINE__);

                      continue;
                    }

                  pn = ptree_node_get (minus_tib,
                                       (u_int8_t *) &isl->isl_saddr [idx],
                                       IPV4_MAX_PREFIXLEN);

                  if (! pn)
                    {
                      IGMP_DBG_ERR (igi, TIB, "Failed to get P-Trie node"
                                    " for %r @ %s:%d",
                                     &isl->isl_saddr [idx],
                                     __FILE__, __LINE__);

                      igmp_if_srec_delete (isr);

                      continue;
                    }

                  /* Install the Src-Rec into Minus TIB */
                  pn->info = isr;
                  isr->isr_owning_pn = pn;

                  /* Count into TIB-B */
                  igr->igr_src_b_tib_count += 1;

                  if (isl->isl_static)
                    SET_FLAG (isr->isr_sflags, IGMP_ISR_SFLAG_STATIC);
                  else
                    SET_FLAG (isr->isr_sflags, IGMP_ISR_SFLAG_DYNAMIC);

                  /* Set the Source Timer to ZERO */
                  isr->v_isr_liveness = 0;
                  IGMP_TIMER_OFF (IGMP_LG (igi), isr->t_isr_liveness);
                  IGMP_TIMER_ON (IGMP_LG (igi), isr->t_isr_liveness,
                                 isr, igmp_if_srec_timer_liveness,
                                 isr->v_isr_liveness);
                }
            }

          /* Loose the Sources in TIB-A and TIB-B */
          for (pn = ptree_top (igr->igr_src_a_tib); pn; pn = pn_next)
            {
              pn_next = ptree_next (pn);

              if (! (isr = pn->info))
                continue;

              /* Move static src to Xtion TIB */
              if (CHECK_FLAG (isr->isr_sflags, IGMP_ISR_SFLAG_STATIC))
                {
                  UNSET_FLAG (isr->isr_sflags, IGMP_ISR_SFLAG_DYNAMIC);

                  /* Reset the Src-Timer to 0 */
                  isr->v_isr_liveness = 0;
                  IGMP_TIMER_OFF (IGMP_LG (igi), isr->t_isr_liveness);

                  pn_sec = ptree_node_get (intersection_tib,
                                           (u_int8_t *) PTREE_NODE_KEY (
                                           isr->isr_owning_pn),
                                           IPV4_MAX_PREFIXLEN);

                  if (pn_sec)
                    {
                      /* Switch over the Src-Rec to Xtion TIB */
                      if (isr->isr_owning_pn)
                        isr->isr_owning_pn->info = NULL;
                      pn_sec->info = isr;
                      isr->isr_owning_pn = pn_sec;

                      continue;
                    }
                  else
                    IGMP_DBG_ERR (igi, TIB, "Failed to get P-Trie "
                                  "node for %r @ %s:%d",
                                  PTREE_NODE_KEY (isr->isr_owning_pn),
                                  __FILE__, __LINE__);
                }

              igmp_if_srec_delete (isr);

              igr->igr_src_a_tib_count -= 1;
            }
          ptree_finish (igr->igr_src_a_tib);

          for (pn = ptree_top (igr->igr_src_b_tib); pn; pn = pn_next)
            {
              pn_next = ptree_next (pn);

              if (! (isr = pn->info))
                continue;

              igmp_if_srec_delete (isr);

              igr->igr_src_b_tib_count -= 1;
            }
          ptree_finish (igr->igr_src_b_tib);

          /* Install the new TIBs */
          igr->igr_src_a_tib = intersection_tib;
          igr->igr_src_b_tib = minus_tib;

          /* Reset Group Timer to GMI for Dynamic events */
          if (! isl || ! isl->isl_static)
            {
              igr->v_igr_liveness = igr->igr_owning_igif->igif_gmi;
              IGMP_TIMER_OFF (IGMP_LG (igi), igr->t_igr_liveness);
              IGMP_TIMER_ON (IGMP_LG (igi), igr->t_igr_liveness,
                             igr, igmp_if_grec_timer_liveness,
                             igr->v_igr_liveness);
            }

          ret = igmp_igr_fsm_change_state (igr, IGMP_FMS_EXCLUDE);

          ret = igmp_igr_fsm_send_lmem_update (igr);
        }
      break;

    case IGMP_FME_CHG_TO_INCL:
      if (CHECK_FLAG (igr->igr_sflags, IGMP_IGR_SFLAG_COMPAT_V1)
          || CHECK_FLAG (igr->igr_sflags, IGMP_IGR_SFLAG_COMPAT_V2)
          || CHECK_FLAG (igr->igr_sflags, IGMP_IGR_SFLAG_COMPAT_V3))
        {
          minus_tib = ptree_init (IPV4_MAX_BITLEN);

          if (! minus_tib)
            {
              IGMP_DBG_ERR (igi, TIB, "P-Trie Init Failed %s:%d",
                            __FILE__, __LINE__);

              ret = IGMP_ERR_OOM;
              goto EXIT;
            }

          /* First duplicate TIB-A for (A-B) */
          for (num_srcs = 0, pn = ptree_top (igr->igr_src_a_tib);
               pn; pn = ptree_next (pn))
            {
              if (! (isr_tmp = pn->info))
                continue;

              /* If the src is not dynamic, dont duplicate onto MINUS TIB */
              if (! CHECK_FLAG (isr_tmp->isr_sflags, IGMP_ISR_SFLAG_DYNAMIC))
                continue;

              /* Create a new Src-Rec */
              ret = igmp_if_srec_create (igr, &isr);

              if (ret < IGMP_ERR_NONE)
                {
                  IGMP_DBG_ERR (igi, TIB, "Failed to create Src node"
                                " for %r @ %s:%d", PTREE_NODE_KEY (pn),
                                __FILE__, __LINE__);

                  continue;
                }

              if (CHECK_FLAG (isr_tmp->isr_sflags, IGMP_ISR_SFLAG_STATIC))
                SET_FLAG (isr->isr_sflags, IGMP_ISR_SFLAG_STATIC);

              if (CHECK_FLAG (isr_tmp->isr_sflags, IGMP_ISR_SFLAG_DYNAMIC))
                SET_FLAG (isr->isr_sflags, IGMP_ISR_SFLAG_DYNAMIC);

              pn_sec = ptree_node_get (minus_tib,
                                       PTREE_NODE_KEY (pn),
                                       IPV4_MAX_PREFIXLEN);

              if (! pn_sec)
                {
                  IGMP_DBG_ERR (igi, TIB, "Failed to get P-Trie node"
                                " for %r @ %s:%d", PTREE_NODE_KEY (pn),
                                __FILE__, __LINE__);

                  /* Delete the Src-Rec created above */
                  igmp_if_srec_delete (isr);

                  continue;
                }

              /* Couple the Src-Rec to P-Trie node */
              pn_sec->info = isr;
              isr->isr_owning_pn = pn_sec;

              /* Run-up the Srcs Counter */
              num_srcs += 1;
            }

          /* Merge Source-List into TIB-A */
          for (idx = 0; isl && idx < isl->isl_num; idx++)
            {
              if ((isr = igmp_if_srec_lookup (igr, PAL_TRUE,
                                              &isl->isl_saddr [idx])))
                {
                  if (! isl->isl_static)
                    {
                      SET_FLAG (isr->isr_sflags, IGMP_ISR_SFLAG_DYNAMIC);

                      /* Reset Src-Timer to GMI */
                      isr->v_isr_liveness = igr->v_igr_liveness;
                      IGMP_TIMER_OFF (IGMP_LG (igi), isr->t_isr_liveness);
                      IGMP_TIMER_ON (IGMP_LG (igi), isr->t_isr_liveness,
                                     isr, igmp_if_srec_timer_liveness,
                                     isr->v_isr_liveness);
                    }
                  else
                    SET_FLAG (isr->isr_sflags, IGMP_ISR_SFLAG_STATIC);

                  isr = NULL;

                  /* Delete Src from Minus TIB (A-B) */
                  pn = ptree_node_lookup (minus_tib,
                                          (u_int8_t *)
                                          &isl->isl_saddr [idx],
                                          IPV4_MAX_PREFIXLEN);

                  if (pn)
                    {
                      isr = pn->info;
                      ptree_unlock_node (pn);
                    }

                  if (isr)
                    igmp_if_srec_delete (isr);

                  /* Decrement the Srcs Counter */
                  num_srcs -= 1;
                }
              else
                {
                  /* Merge New Src into TIB-A */
                  isr = igmp_if_srec_get (igr, PAL_TRUE,
                                          &isl->isl_saddr [idx]);

                  if (isr)
                    {
                      if (isl->isl_static)
                        SET_FLAG (isr->isr_sflags, IGMP_ISR_SFLAG_STATIC);
                      else
                        SET_FLAG (isr->isr_sflags, IGMP_ISR_SFLAG_DYNAMIC);

                      if (CHECK_FLAG (isr->isr_sflags, IGMP_ISR_SFLAG_DYNAMIC))
                        {
                          /* Reset Src-Timer to GMI */
                          isr->v_isr_liveness = igr->v_igr_liveness;
                          IGMP_TIMER_OFF (IGMP_LG (igi), isr->t_isr_liveness);
                          IGMP_TIMER_ON (IGMP_LG (igi), isr->t_isr_liveness,
                                         isr, igmp_if_srec_timer_liveness,
                                         isr->v_isr_liveness);
                        }

                      send_indication = PAL_TRUE;
                    }
                  else
                    IGMP_DBG_WARN (igi, TIB, "Failed to get Src-Rec for"
                                   " %r @ %s:%d", &isl->isl_saddr [idx],
                                   __FILE__, __LINE__);
                }
            }

          /* Reset error-code */
          ret = IGMP_ERR_NONE;

          /* Send Q(G,A-B) */
          if (num_srcs)
            {
              igr->igr_rexmit_group_source_lmqc =
                   igr->igr_owning_igif->igif_conf.igifc_lmqc;

              ret = igmp_if_send_group_source_query (igr, minus_tib, num_srcs);

              if (ret < IGMP_ERR_NONE)
                IGMP_DBG_INFO (igi, FSM, "Grp-Src Specific Query Send Failed!");
            }

          /* Store the Minus-TIB into G-Rec for Re-Transmissions */
          if (num_srcs && ! ret)
            {
              if (igr->igr_rexmit_srcs_tib)
                (void_t) igmp_if_grec_rexmit_tib_delete (igr);

              igr->igr_rexmit_srcs_tib = minus_tib;
              igr->igr_rexmit_srcs_tib_count = num_srcs;
            }
          else if (minus_tib)
            {
              /* Discard the Minus-TIB */
              for (pn = ptree_top (minus_tib); pn; pn = pn_next)
                {
                  pn_next = ptree_next (pn);

                  if (! (isr = pn->info))
                    continue;

                  igmp_if_srec_delete (isr);
                }
              ptree_finish (minus_tib);

              igr->igr_rexmit_srcs_tib = NULL;
              igr->igr_rexmit_srcs_tib_count = 0;
            }

          if (isl
              && isl->isl_static
              && ! isl->isl_num)
            {
              UNSET_FLAG (igr->igr_sflags, IGMP_IGR_SFLAG_STATIC);

              for (pn = ptree_top (igr->igr_src_a_tib); pn; pn = pn_next)
                {
                  pn_next = ptree_next (pn);

                  if (! (isr = pn->info))
                    continue;

                  UNSET_FLAG (isr->isr_sflags, IGMP_ISR_SFLAG_STATIC);

                  if (CHECK_FLAG (isr->isr_sflags, IGMP_ISR_SFLAG_DYNAMIC))
                    continue;

                  igmp_if_srec_delete (isr);

                  igr->igr_src_a_tib_count -= 1;
                }
            }

          if (! send_indication)
            send_indication = (CHECK_FLAG (igr->igr_sflags,
                                           IGMP_IGR_SFLAG_STATE_REFRESH));

          ret = igmp_igr_fsm_change_state (igr, IGMP_FMS_INCLUDE);

          if (send_indication)
            ret = igmp_igr_fsm_send_lmem_update (igr);

          if (! igr->igr_src_a_tib_count)
            igmp_if_grec_delete (igr);
        }
      break;

    case IGMP_FME_CHG_TO_EXCL:
      if (CHECK_FLAG (igr->igr_sflags, IGMP_IGR_SFLAG_COMPAT_V1)
          || CHECK_FLAG (igr->igr_sflags, IGMP_IGR_SFLAG_COMPAT_V2)
          || CHECK_FLAG (igr->igr_sflags, IGMP_IGR_SFLAG_COMPAT_V3))
        {
          intersection_tib = ptree_init (IPV4_MAX_BITLEN);

          if (! intersection_tib)
            {
              IGMP_DBG_ERR (igi, TIB, "P-Trie Init Failed %s:%d",
                            __FILE__, __LINE__);

              ret = IGMP_ERR_OOM;
              goto EXIT;
            }

          minus_tib = ptree_init (IPV4_MAX_BITLEN);

          if (! minus_tib)
            {
              IGMP_DBG_ERR (igi, TIB, "P-Trie Init Failed %s:%d",
                            __FILE__, __LINE__);

              ret = IGMP_ERR_OOM;
              goto EXIT;
            }

          num_srcs = 0;

          for (idx = 0; isl && idx < isl->isl_num; idx++)
            {
              if ((isr = igmp_if_srec_lookup (igr, PAL_TRUE,
                                              &isl->isl_saddr [idx])))
                {
                  pn = ptree_node_get (intersection_tib,
                                       (u_int8_t *) &isl->isl_saddr [idx],
                                       IPV4_MAX_PREFIXLEN);

                  if (! pn)
                    {
                      IGMP_DBG_ERR (igi, TIB, "Failed to get P-Trie node"
                                    " for %r @ %s:%d",
                                    &isl->isl_saddr [idx],
                                    __FILE__, __LINE__);

                      continue;
                    }

                  /* Switch over the Src-Rec to Xtion TIB */
                  if (isr->isr_owning_pn)
                    isr->isr_owning_pn->info = NULL;
                  pn->info = isr;
                  isr->isr_owning_pn = pn;
                }
              else
                {
                  /* Create a new Src-Rec */
                  ret = igmp_if_srec_create (igr, &isr);

                  if (ret < IGMP_ERR_NONE)
                    {
                      IGMP_DBG_ERR (igi, TIB, "Failed to create Src node "
                                    "for %r @ %s:%d", &isl->isl_saddr [idx],
                                    __FILE__, __LINE__);

                      ret = IGMP_ERR_NONE;
                      continue;
                    }

                  pn = ptree_node_get (minus_tib,
                                       (u_int8_t *) &isl->isl_saddr [idx],
                                       IPV4_MAX_PREFIXLEN);

                  if (! pn)
                    {
                      IGMP_DBG_ERR (igi, TIB, "Failed to get P-Trie node "
                                    "for %r @ %s:%d", &isl->isl_saddr [idx],
                                     __FILE__, __LINE__);

                      /* Delete the Src-Rec created above */
                      igmp_if_srec_delete (isr);

                      continue;
                    }

                  /* Install the Src-Rec into Minus TIB */
                  pn->info = isr;
                  isr->isr_owning_pn = pn;

                  /* Count into TIB-B */
                  igr->igr_src_b_tib_count += 1;

                  if (isl->isl_static)
                    SET_FLAG (isr->isr_sflags, IGMP_ISR_SFLAG_STATIC);
                  else
                    SET_FLAG (isr->isr_sflags, IGMP_ISR_SFLAG_DYNAMIC);

                  if (! isl->isl_static)
                    {
                      /* Set the Source Timer to 0 */
                      isr->v_isr_liveness = 0;
                      IGMP_TIMER_OFF (IGMP_LG (igi), isr->t_isr_liveness);
                      IGMP_TIMER_ON (IGMP_LG (igi), isr->t_isr_liveness,
                                     isr, igmp_if_srec_timer_liveness,
                                     isr->v_isr_liveness);
                    }
                }
            }

          /* Loose the Sources in TIB-A and TIB-B */
          for (pn = ptree_top (igr->igr_src_a_tib); pn; pn = pn_next)
            {
              pn_next = ptree_next (pn);

              if (! (isr = pn->info))
                continue;

              /* Move static src to Xtion TIB */
              if (CHECK_FLAG (isr->isr_sflags, IGMP_ISR_SFLAG_STATIC)
                  && ! (isl
                        && isl->isl_static
                        && ! isl->isl_num))
                {
                  pn_sec = ptree_node_get (intersection_tib,
                                           (u_int8_t *) PTREE_NODE_KEY (
                                           isr->isr_owning_pn),
                                           IPV4_MAX_PREFIXLEN);

                  if (pn_sec)
                    {
                      /* Switch over the Src-Rec to Xtion TIB */
                      if (isr->isr_owning_pn)
                        isr->isr_owning_pn->info = NULL;
                      pn_sec->info = isr;
                      isr->isr_owning_pn = pn_sec;

                      continue;
                    }
                  else
                    IGMP_DBG_ERR (igi, TIB, "Failed to get P-Trie "
                                  "node for %r @ %s:%d",
                                  PTREE_NODE_KEY (isr->isr_owning_pn),
                                  __FILE__, __LINE__);
                }

              igmp_if_srec_delete (isr);

              igr->igr_src_a_tib_count -= 1;
            }
          ptree_finish (igr->igr_src_a_tib);

          for (pn = ptree_top (igr->igr_src_b_tib); pn; pn = pn_next)
            {
              pn_next = ptree_next (pn);

              if (! (isr = pn->info))
                continue;

              igmp_if_srec_delete (isr);

              igr->igr_src_b_tib_count -= 1;
            }
          ptree_finish (igr->igr_src_b_tib);

          if (igr->igr_rexmit_srcs_tib != igr->igr_src_a_tib
              && igr->igr_rexmit_srcs_tib != igr->igr_src_b_tib)
            del_rexmit_tib = PAL_TRUE;
          else
            del_rexmit_tib = PAL_FALSE;

          /* Install the new TIBs */
          igr->igr_src_a_tib = intersection_tib;
          igr->igr_src_b_tib = minus_tib;

          /* Send Q (G, A*B) */
          igr->igr_rexmit_group_source_lmqc =
              igr->igr_owning_igif->igif_conf.igifc_lmqc;

          ret = igmp_if_send_group_source_query (igr, intersection_tib,
                                                 igr->igr_src_a_tib_count);

          /* Store the Intersection-TIB for Re-Transmissions */
          if (ret < IGMP_ERR_NONE)
            IGMP_DBG_INFO (igi, FSM, "Grp-Src Specific Query Send Failed!");
          else if (igr->igr_src_a_tib_count)
            {
              if (igr->igr_rexmit_srcs_tib
                  && del_rexmit_tib == PAL_TRUE)
                {
                  (void_t) igmp_if_grec_rexmit_tib_delete (igr);
                }
              else
                {
                  igr->igr_rexmit_srcs_tib_count = 0;
                  igr->igr_rexmit_srcs_tib = NULL;
                }

              igr->igr_rexmit_srcs_tib = intersection_tib;
              igr->igr_rexmit_srcs_tib_count =
                  igr->igr_src_a_tib_count;
            }

          send_indication = PAL_TRUE;
        }

      /* Reset Group Timer to GMI for Dynamic events */
      if (! isl || ! isl->isl_static)
        {
          igr->v_igr_liveness = igr->igr_owning_igif->igif_gmi;
          IGMP_TIMER_OFF (IGMP_LG (igi), igr->t_igr_liveness);
          IGMP_TIMER_ON (IGMP_LG (igi), igr->t_igr_liveness,
                         igr, igmp_if_grec_timer_liveness,
                         igr->v_igr_liveness);
        }

      ret = igmp_igr_fsm_change_state (igr, IGMP_FMS_EXCLUDE);

      if (send_indication)
        ret = igmp_igr_fsm_send_lmem_update (igr);
      break;

    case IGMP_FME_ALLOW_NEW_SRCS:
      if (CHECK_FLAG (igr->igr_sflags, IGMP_IGR_SFLAG_COMPAT_V1)
          || CHECK_FLAG (igr->igr_sflags, IGMP_IGR_SFLAG_COMPAT_V2)
          || CHECK_FLAG (igr->igr_sflags, IGMP_IGR_SFLAG_COMPAT_V3))
        {
          /* Merge Source-List into TIB-A */
          for (idx = 0; isl && idx < isl->isl_num; idx++)
            {
              if (! (isr = igmp_if_srec_lookup (igr, PAL_TRUE,
                                              &isl->isl_saddr [idx]))
                  && (isr = igmp_if_srec_get (igr, PAL_TRUE,
                                              &isl->isl_saddr [idx])))
                send_indication = PAL_TRUE;

              if (isr)
                {
                  if (isl->isl_static)
                    SET_FLAG (isr->isr_sflags, IGMP_ISR_SFLAG_STATIC);
                  else
                    SET_FLAG (isr->isr_sflags, IGMP_ISR_SFLAG_DYNAMIC);

                  if (! isl->isl_static)
                    {
                      /* Reset Src-Timer to GMI */
                      isr->v_isr_liveness = igr->v_igr_liveness;
                      IGMP_TIMER_OFF (IGMP_LG (igi), isr->t_isr_liveness);
                      IGMP_TIMER_ON (IGMP_LG (igi), isr->t_isr_liveness,
                                     isr, igmp_if_srec_timer_liveness,
                                     isr->v_isr_liveness);
                    }

                }
              else
                {
                  IGMP_DBG_ERR (igr->igr_owning_igif->igif_owning_igi,
                                TIB, "Failed to get Src-Rec for"
                                " %r @ %s:%d", &isl->isl_saddr [idx],
                                __FILE__, __LINE__);

                  continue;
                }
            }

          if (! send_indication)
            send_indication = (CHECK_FLAG (igr->igr_sflags,
                                           IGMP_IGR_SFLAG_STATE_REFRESH));

          ret = igmp_igr_fsm_change_state (igr, IGMP_FMS_INCLUDE);

          if (send_indication)
            ret = igmp_igr_fsm_send_lmem_update (igr);

          if (! igr->igr_src_a_tib_count)
            igmp_if_grec_delete (igr);
        }
      break;

    case IGMP_FME_BLOCK_OLD_SRCS:
      if (CHECK_FLAG (igr->igr_sflags, IGMP_IGR_SFLAG_COMPAT_V1)
          || CHECK_FLAG (igr->igr_sflags, IGMP_IGR_SFLAG_COMPAT_V2)
          || CHECK_FLAG (igr->igr_sflags, IGMP_IGR_SFLAG_COMPAT_V3))
        {
          intersection_tib = ptree_init (IPV4_MAX_BITLEN);

          if (! intersection_tib)
            {
              IGMP_DBG_ERR (igi, TIB, "P-Trie Init Failed %s:%d",
                            __FILE__, __LINE__);

              ret = IGMP_ERR_OOM;
              goto EXIT;
            }

          num_srcs = 0;

          /* Obtain Intersection Source-List TIB */
          for (idx = 0; isl && idx < isl->isl_num; idx++)
            {
              if ((isr_tmp = igmp_if_srec_lookup (igr, PAL_TRUE,
                                                  &isl->isl_saddr [idx])))
                {
                  if (isl->isl_static)
                    {
                      if (CHECK_FLAG (isr_tmp->isr_sflags, IGMP_ISR_SFLAG_DYNAMIC))
                        UNSET_FLAG (isr_tmp->isr_sflags, IGMP_ISR_SFLAG_STATIC);
                      else
                        {
                          igmp_if_srec_delete (isr_tmp);
                          igr->igr_src_a_tib_count -= 1;

                          send_indication = PAL_TRUE;
                        }
                    }
                  else
                    {
                      if (! CHECK_FLAG (isr_tmp->isr_sflags,
                                        IGMP_ISR_SFLAG_DYNAMIC))
                        continue;

                      /* Create a new Src-Rec */
                      ret = igmp_if_srec_create (igr, &isr);

                      if (ret < IGMP_ERR_NONE)
                        {
                          IGMP_DBG_ERR (igi, TIB, "Failed to create Src node"
                                        " for %r @ %s:%d",
                                        &isl->isl_saddr [idx],
                                        __FILE__, __LINE__);

                          continue;
                        }

                      pn = ptree_node_get (intersection_tib,
                                           (u_int8_t *) &isl->isl_saddr [idx],
                                           IPV4_MAX_PREFIXLEN);
                      if (! pn)
                        {
                          IGMP_DBG_ERR (igi, TIB, "Failed to get P-Trie node"
                                        " for %r @ %s:%d",
                                        &isl->isl_saddr [idx],
                                        __FILE__, __LINE__);

                          /* Delete the Src-Rec created above */
                          igmp_if_srec_delete (isr);

                          continue;
                        }

                      /* Couple the Src-Rec to P-Trie node */
                      pn->info = isr;
                      isr->isr_owning_pn = pn;

                      /* Run-up the Srcs Counter */
                      num_srcs += 1;
                    }
                }
            }

          /* Reset error-code */
          ret = IGMP_ERR_NONE;

          /* Send Q(G,A*B) */
          if (num_srcs)
            {
              igr->igr_rexmit_group_source_lmqc =
                   igr->igr_owning_igif->igif_conf.igifc_lmqc;

              ret = igmp_if_send_group_source_query (igr, intersection_tib,
                                                     num_srcs);

              if (ret < IGMP_ERR_NONE)
                IGMP_DBG_INFO (igi, FSM, "Grp-Src Specific Query Send Failed!");
            }

          /* Store the Intersection-TIB for Re-Transmissions */
          if (num_srcs && ! ret)
            {
              if (igr->igr_rexmit_srcs_tib)
                (void_t) igmp_if_grec_rexmit_tib_delete (igr);

              igr->igr_rexmit_srcs_tib = intersection_tib;
              igr->igr_rexmit_srcs_tib_count = num_srcs;
            }
          else if (intersection_tib)
            {
              /* Discard the Intersection-TIB */
              for (pn = ptree_top (intersection_tib); pn; pn = pn_next)
                {
                  pn_next = ptree_next (pn);

                  if (! (isr = pn->info))
                    continue;

                  igmp_if_srec_delete (isr);
                }
              ptree_finish (intersection_tib);

              igr->igr_rexmit_srcs_tib = NULL;
              igr->igr_rexmit_srcs_tib_count = 0;
            }

           if (! send_indication)
             send_indication = (CHECK_FLAG (igr->igr_sflags,
                                            IGMP_IGR_SFLAG_STATE_REFRESH));

          ret = igmp_igr_fsm_change_state (igr, IGMP_FMS_INCLUDE);

          if (send_indication)
            ret = igmp_igr_fsm_send_lmem_update (igr);

          if (! igr->igr_src_a_tib_count)
            igmp_if_grec_delete (igr);
        }
      break;

    case IGMP_FME_GROUP_TIMER_EXPIRY:
      if (CHECK_FLAG (igr->igr_sflags, IGMP_IGR_SFLAG_STATIC))
        {
          UNSET_FLAG (igr->igr_sflags, IGMP_IGR_SFLAG_DYNAMIC);
          if (igr->igr_src_a_tib_count)
            ret = igmp_igr_fsm_change_state (igr, IGMP_FMS_INCLUDE);
          else
            ret = igmp_igr_fsm_change_state (igr, IGMP_FMS_EXCLUDE);
        }
      else
        ret = igmp_igr_fsm_change_state (igr, IGMP_FMS_INCLUDE);
      break;

    case IGMP_FME_SOURCE_TIMER_EXPIRY:
      if ((isr = igmp_if_srec_lookup (igr, PAL_TRUE,
                                      &isl->isl_saddr [0])))
        {
          if (! CHECK_FLAG (isr->isr_sflags, IGMP_ISR_SFLAG_STATIC))
            {
              igmp_if_srec_delete (isr);

              igr->igr_src_a_tib_count -= 1;

              if (! igr->igr_src_a_tib_count)
                {
                  /* Send INCL <NULL> Indication (-> G-rec Del) */
                  ret = igmp_igr_fsm_send_lmem_update (igr);

                  ret = igmp_if_grec_delete (igr);
                }
              else
                /* Send INCL <Src-List> Indication */
                ret = igmp_igr_fsm_send_lmem_update (igr);
            }
          else
            UNSET_FLAG (isr->isr_sflags, IGMP_ISR_SFLAG_DYNAMIC);
        }
      break;

    case IGMP_FME_MANUAL_CLEAR:
    case IGMP_FME_IMMEDIATE_LEAVE:
      if (! isl)
        {
          /* Release the Source P-Trie 'A' */
          for (pn = ptree_top (igr->igr_src_a_tib); pn; pn = pn_next)
            {
              pn_next = ptree_next (pn);

              if (! (isr = pn->info))
                continue;

              if (CHECK_FLAG (isr->isr_sflags, IGMP_ISR_SFLAG_STATIC)
                  && CHECK_FLAG (isr->isr_sflags, IGMP_ISR_SFLAG_DYNAMIC))
                {
                  UNSET_FLAG (isr->isr_sflags, IGMP_ISR_SFLAG_DYNAMIC);
                  continue;
                }

              igmp_if_srec_delete (isr);

              igr->igr_src_a_tib_count -= 1;
            }

          /* Release the Source P-Trie 'B' */
          for (pn = ptree_top (igr->igr_src_b_tib); pn; pn = pn_next)
            {
              pn_next = ptree_next (pn);

              if (! (isr = pn->info))
                continue;

              igmp_if_srec_delete (isr);

              igr->igr_src_b_tib_count -= 1;
            }

          ret = igmp_igr_fsm_change_state (igr, IGMP_FMS_INCLUDE);

          ret = igmp_igr_fsm_send_lmem_update (igr);

          ret = igmp_if_grec_delete (igr);
        }
      else if ((isr = igmp_if_srec_lookup (igr, PAL_TRUE,
                                           &isl->isl_saddr [0])))
        {
          igmp_if_srec_delete (isr);

          igr->igr_src_a_tib_count -= 1;

          ret = igmp_igr_fsm_change_state (igr, IGMP_FMS_INCLUDE);

          if (! igr->igr_src_a_tib_count)
            {
              ret = igmp_igr_fsm_send_lmem_update (igr);

              ret = igmp_if_grec_delete (igr);
            }
          else
            ret = igmp_igr_fsm_send_lmem_update (igr);
        }
      break;

    case IGMP_FME_MANUAL_LMEM_UPDATE:
      ret = igmp_igr_fsm_send_lmem_update (igr);
      break;

    case IGMP_FME_INVALID:
      ret = IGMP_ERR_DOOM;
      break;
    }

EXIT:

  IGMP_FN_EXIT (ret);
}

/* IGMP FSM action function for Exclude-state */
s_int32_t
igmp_igr_fsm_action_exclude (struct igmp_group_rec *igr,
                             enum igmp_filter_mode_event igr_fme,
                             struct igmp_source_list *isl)
{
  struct igmp_source_rec *isr_tmp;
  struct ptree *intersection_tib;
  struct igmp_source_rec *isr;
  struct ptree_node *pn_next;
  struct ptree_node *pn_sec;
  struct igmp_instance *igi;
  u_int8_t  del_rexmit_tib;
  struct ptree *minus_tib;
  bool_t send_indication;
  struct ptree_node *pn;
  u_int16_t num_srcs;
  u_int32_t rem_time;
  u_int32_t idx;
  s_int32_t ret;

  IGMP_FN_ENTER (Act Exclude);

  igi = igr->igr_owning_igif->igif_owning_igi;
  send_indication = PAL_FALSE;
  del_rexmit_tib = PAL_FALSE;
  intersection_tib = NULL;
  ret = IGMP_ERR_NONE;
  minus_tib = NULL;
  isr_tmp = NULL;
  isr = NULL;

  switch (igr_fme)
    {
    case IGMP_FME_MODE_IS_INCL:
      if (CHECK_FLAG (igr->igr_sflags, IGMP_IGR_SFLAG_COMPAT_V1)
          || CHECK_FLAG (igr->igr_sflags, IGMP_IGR_SFLAG_COMPAT_V2)
          || CHECK_FLAG (igr->igr_sflags, IGMP_IGR_SFLAG_COMPAT_V3))
        {
          /* Merge Source-List into TIB-A */
          for (idx = 0; isl && idx < isl->isl_num; idx++)
            {
              /* Remove the Source from TIB-B ==> (Y-A) */
              isr = igmp_if_srec_lookup (igr, PAL_FALSE,
                                         &isl->isl_saddr [idx]);

              if (isr)
                {
                  igmp_if_srec_delete (isr);

                  igr->igr_src_b_tib_count -= 1;

                  send_indication = PAL_TRUE;
                }

              /* Add the Source to TIB-A ==> (X+A) */
              if ((isr = igmp_if_srec_get (igr, PAL_TRUE,
                                           &isl->isl_saddr [idx])))
                {
                  if (! isl->isl_static)
                    SET_FLAG (isr->isr_sflags, IGMP_ISR_SFLAG_DYNAMIC);
                  else
                    SET_FLAG (isr->isr_sflags, IGMP_ISR_SFLAG_STATIC);

                  /* Reset Src-Timer to GMI */
                  isr->v_isr_liveness = igr->v_igr_liveness;
                  IGMP_TIMER_OFF (IGMP_LG (igi), isr->t_isr_liveness);
                  IGMP_TIMER_ON (IGMP_LG (igi), isr->t_isr_liveness,
                                 isr, igmp_if_srec_timer_liveness,
                                 isr->v_isr_liveness);
                }
            }
        }

       if (! send_indication)
         send_indication = (CHECK_FLAG (igr->igr_sflags,
                                        IGMP_IGR_SFLAG_STATE_REFRESH));

      ret = igmp_igr_fsm_change_state (igr, IGMP_FMS_EXCLUDE);

      if (send_indication)
        ret = igmp_igr_fsm_send_lmem_update (igr);

      break;

    case IGMP_FME_MODE_IS_EXCL:
      if (CHECK_FLAG (igr->igr_sflags, IGMP_IGR_SFLAG_COMPAT_V1)
          || CHECK_FLAG (igr->igr_sflags, IGMP_IGR_SFLAG_COMPAT_V2)
          || CHECK_FLAG (igr->igr_sflags, IGMP_IGR_SFLAG_COMPAT_V3))
        {
          intersection_tib = ptree_init (IPV4_MAX_BITLEN);

          if (! intersection_tib)
            {
              IGMP_DBG_ERR (igi, TIB, "P-Trie Init Failed %s:%d",
                            __FILE__, __LINE__);

              ret = IGMP_ERR_OOM;
              goto EXIT;
            }

          minus_tib = ptree_init (IPV4_MAX_BITLEN);

          if (! minus_tib)
            {
              IGMP_DBG_ERR (igi, TIB, "P-Trie Init Failed %s:%d",
                            __FILE__, __LINE__);

              ret = IGMP_ERR_OOM;
              goto EXIT;
            }

          /* Construct A-Y and Y*A TIBs */
          for (idx = 0; isl && idx < isl->isl_num; idx++)
            {
              if ((isr = igmp_if_srec_lookup (igr, PAL_FALSE,
                                              &isl->isl_saddr [idx])))
                {
                  pn = ptree_node_get (intersection_tib,
                                       (u_int8_t *) &isl->isl_saddr [idx],
                                       IPV4_MAX_PREFIXLEN);

                  if (! pn)
                    {
                      IGMP_DBG_ERR (igi, TIB, "Failed to get P-Trie node"
                                    " for %r @ %s:%d",
                                     &isl->isl_saddr [idx],
                                     __FILE__, __LINE__);

                      continue;
                    }

                  /* Switch over the Src-Rec to Xtion TIB */
                  if (isr->isr_owning_pn)
                    isr->isr_owning_pn->info = NULL;
                  pn->info = isr;
                  isr->isr_owning_pn = pn;
                }
              /* Overlapping Static Sources will be handled during deletion */
              else if ((isr = igmp_if_srec_lookup (igr, PAL_TRUE,
                                                   &isl->isl_saddr [idx]))
                       && CHECK_FLAG (isr->isr_sflags, IGMP_ISR_SFLAG_STATIC))
                continue;
              else
                {
                  /* Create a new Src-Rec */
                  ret = igmp_if_srec_create (igr, &isr);

                  if (ret < IGMP_ERR_NONE)
                    {
                      IGMP_DBG_ERR (igi, TIB, "Failed to create Src node "
                                    "for %r @ %s:%d", &isl->isl_saddr [idx],
                                    __FILE__, __LINE__);

                      ret = IGMP_ERR_NONE;
                      continue;
                    }

                  pn = ptree_node_get (minus_tib,
                                       (u_int8_t *) &isl->isl_saddr [idx],
                                       IPV4_MAX_PREFIXLEN);

                  if (! pn)
                    {
                      IGMP_DBG_ERR (igi, TIB, "Failed to get P-Trie node "
                                    "for %r @ %s:%d",
                                    &isl->isl_saddr [idx],
                                    __FILE__, __LINE__);

                      /* Delete the Src-Rec created above */
                      igmp_if_srec_delete (isr);

                      continue;
                    }

                  /* Install the Src-Rec into Minus TIB */
                  pn->info = isr;
                  isr->isr_owning_pn = pn;

                  /* Count into TIB-A */
                  igr->igr_src_a_tib_count += 1;

                  if (isl->isl_static)
                    SET_FLAG (isr->isr_sflags, IGMP_ISR_SFLAG_STATIC);
                  else
                    SET_FLAG (isr->isr_sflags, IGMP_ISR_SFLAG_DYNAMIC);

                  /* Inherit Source Timer or Reset it to GMI */
                  if ((isr_tmp = igmp_if_srec_lookup (igr, PAL_TRUE,
                                                      &isl->isl_saddr [idx])))
                    {
                      isr->isr_uptime = isr_tmp->isr_uptime;
                      isr->v_isr_liveness = isr_tmp->v_isr_liveness;
                      rem_time = thread_timer_remain_second (
                                 isr_tmp->t_isr_liveness);
                      IGMP_TIMER_OFF (IGMP_LG (igi), isr_tmp->t_isr_liveness);
                      IGMP_TIMER_OFF (IGMP_LG (igi), isr->t_isr_liveness);
                      IGMP_TIMER_ON (IGMP_LG (igi), isr->t_isr_liveness,
                                     isr, igmp_if_srec_timer_liveness,
                                     rem_time);
                    }
                  else
                    {
                      isr->v_isr_liveness = igr->v_igr_liveness;
                      IGMP_TIMER_OFF (IGMP_LG (igi), isr->t_isr_liveness);
                      IGMP_TIMER_ON (IGMP_LG (igi), isr->t_isr_liveness,
                                     isr, igmp_if_srec_timer_liveness,
                                     isr->v_isr_liveness);
                    }

                  send_indication = PAL_TRUE;
                }
            }

          /* Loose the Sources in TIB-A and TIB-B */
          for (pn = ptree_top (igr->igr_src_a_tib); pn; pn = pn_next)
            {
              pn_next = ptree_next (pn);

              if (! (isr = pn->info))
                continue;

              /* Move static source to minus TIB */
              if (CHECK_FLAG (isr->isr_sflags, IGMP_ISR_SFLAG_STATIC))
                {
                  UNSET_FLAG (isr->isr_sflags, IGMP_ISR_SFLAG_DYNAMIC);

                  /* Reset the Src-Timer to 0 */
                  isr->v_isr_liveness = 0;
                  IGMP_TIMER_OFF (IGMP_LG (igi), isr->t_isr_liveness);

                  pn_sec = ptree_node_get (minus_tib,
                                           (u_int8_t *) PTREE_NODE_KEY (
                                           isr->isr_owning_pn),
                                           IPV4_MAX_PREFIXLEN);

                  if (pn_sec)
                    {
                      /* Switch over the Src-Rec to Xtion TIB */
                      if (isr->isr_owning_pn)
                        isr->isr_owning_pn->info = NULL;
                      pn_sec->info = isr;
                      isr->isr_owning_pn = pn_sec;

                      continue;
                    }
                  else
                    IGMP_DBG_ERR (igi, TIB, "Failed to get P-Trie "
                                  "node for %r @ %s:%d",
                                  PTREE_NODE_KEY (isr->isr_owning_pn),
                                  __FILE__, __LINE__);
                }

              igmp_if_srec_delete (isr);

              igr->igr_src_a_tib_count -= 1;
            }
          ptree_finish (igr->igr_src_a_tib);

          for (pn = ptree_top (igr->igr_src_b_tib); pn; pn = pn_next)
            {
              pn_next = ptree_next (pn);

              if (! (isr = pn->info))
                continue;

              igmp_if_srec_delete (isr);

              igr->igr_src_b_tib_count -= 1;

              send_indication = PAL_TRUE;
            }
          ptree_finish (igr->igr_src_b_tib);

          if (igr->igr_rexmit_srcs_tib == igr->igr_src_a_tib
              || igr->igr_rexmit_srcs_tib == igr->igr_src_b_tib)
            {
               igr->igr_rexmit_srcs_tib = NULL;
               igr->igr_rexmit_srcs_tib_count = 0;
            }

          /* Install the new TIBs */
          igr->igr_src_a_tib = minus_tib;
          igr->igr_src_b_tib = intersection_tib;
        }

      /* Reset Group Timer to GMI for Dynamic events */
      if (! isl || ! isl->isl_static)
        {
          igr->v_igr_liveness = igr->igr_owning_igif->igif_gmi;
          IGMP_TIMER_OFF (IGMP_LG (igi), igr->t_igr_liveness);
          IGMP_TIMER_ON (IGMP_LG (igi), igr->t_igr_liveness,
                         igr, igmp_if_grec_timer_liveness,
                         igr->v_igr_liveness);

          if (CHECK_FLAG (igr->igr_sflags, IGMP_IGR_SFLAG_COMPAT_V2))
            IGMP_TIMER_OFF (IGMP_LG (igi), igr->t_igr_rexmit_group);
        }

      if (! send_indication)
         send_indication = (CHECK_FLAG (igr->igr_sflags,
                                        IGMP_IGR_SFLAG_STATE_REFRESH));

      ret = igmp_igr_fsm_change_state (igr, IGMP_FMS_EXCLUDE);

      if (send_indication)
        ret = igmp_igr_fsm_send_lmem_update (igr);

      break;

    case IGMP_FME_CHG_TO_INCL:
      if (CHECK_FLAG (igr->igr_sflags, IGMP_IGR_SFLAG_COMPAT_V1)
          || CHECK_FLAG (igr->igr_sflags, IGMP_IGR_SFLAG_COMPAT_V2)
          || CHECK_FLAG (igr->igr_sflags, IGMP_IGR_SFLAG_COMPAT_V3))
        {
          minus_tib = ptree_init (IPV4_MAX_BITLEN);

          if (! minus_tib)
            {
              IGMP_DBG_ERR (igi, TIB, "P-Trie Init Failed %s:%d",
                            __FILE__, __LINE__);

              ret = IGMP_ERR_OOM;
              goto EXIT;
            }

          /* First duplicate TIB-A for (X-A) */
          for (num_srcs = 0, pn = ptree_top (igr->igr_src_a_tib);
               pn; pn = ptree_next (pn))
            {
              if (! (isr_tmp = pn->info))
                continue;

              /* If the src is static, dont move it to MINUS TIB */
              if (! CHECK_FLAG (isr_tmp->isr_sflags, IGMP_ISR_SFLAG_DYNAMIC))
                continue;

              /* Create a new Src-Rec */
              ret = igmp_if_srec_create (igr, &isr);

              if (ret < IGMP_ERR_NONE)
                {
                  IGMP_DBG_ERR (igi, TIB, "Failed to create Src node"
                                " for %r @ %s:%d", PTREE_NODE_KEY (pn),
                                __FILE__, __LINE__);

                  continue;
                }

              if (CHECK_FLAG (isr_tmp->isr_sflags, IGMP_ISR_SFLAG_STATIC))
                SET_FLAG (isr->isr_sflags, IGMP_ISR_SFLAG_STATIC);

              if (CHECK_FLAG (isr_tmp->isr_sflags, IGMP_ISR_SFLAG_DYNAMIC))
                SET_FLAG (isr->isr_sflags, IGMP_ISR_SFLAG_DYNAMIC);

              pn_sec = ptree_node_get (minus_tib,
                                       PTREE_NODE_KEY (pn),
                                       IPV4_MAX_PREFIXLEN);

              if (! pn_sec)
                {
                  IGMP_DBG_ERR (igi, TIB, "Failed to get P-Trie node"
                                " for %r @ %s:%d", PTREE_NODE_KEY (pn),
                                __FILE__, __LINE__);

                  /* Delete the Src-Rec created above */
                  igmp_if_srec_delete (isr);

                  continue;
                }

              /* Couple the Src-Rec to P-Trie node */
              pn_sec->info = isr;
              isr->isr_owning_pn = pn_sec;

              /* Run-up the Srcs Counter */
              num_srcs += 1;
            }

          /* Merge Source-List into TIB-A */
          for (idx = 0; isl && idx < isl->isl_num; idx++)
            {
              if ((isr = igmp_if_srec_lookup (igr, PAL_TRUE,
                                              &isl->isl_saddr [idx])))
                {
                  if (! isl->isl_static)
                    {
                      SET_FLAG (isr->isr_sflags, IGMP_ISR_SFLAG_DYNAMIC);

                      /* Reset Src-Timer to GMI */
                      isr->v_isr_liveness = igr->v_igr_liveness;
                      IGMP_TIMER_OFF (IGMP_LG (igi), isr->t_isr_liveness);
                      IGMP_TIMER_ON (IGMP_LG (igi), isr->t_isr_liveness,
                                     isr, igmp_if_srec_timer_liveness,
                                     isr->v_isr_liveness);
                    }
                  else
                    SET_FLAG (isr->isr_sflags, IGMP_ISR_SFLAG_STATIC);

                  isr = NULL;

                  /* Delete Src from Minus TIB (X-A) */
                  pn = ptree_node_lookup (minus_tib,
                                          (u_int8_t *)
                                          &isl->isl_saddr [idx],
                                          IPV4_MAX_PREFIXLEN);

                  if (pn)
                    {
                      isr = pn->info;
                      ptree_unlock_node (pn);
                    }

                  if (isr)
                    igmp_if_srec_delete (isr);

                  /* Decrement the Srcs Counter */
                  num_srcs -= 1;
                }
              else
                {
                  /* Merge New Src into TIB-A */
                  isr = igmp_if_srec_get (igr, PAL_TRUE,
                                          &isl->isl_saddr [idx]);

                  if (isr)
                    {
                      if (isl->isl_static)
                        SET_FLAG (isr->isr_sflags, IGMP_ISR_SFLAG_STATIC);
                      else
                        SET_FLAG (isr->isr_sflags, IGMP_ISR_SFLAG_DYNAMIC);

                      if (CHECK_FLAG (isr->isr_sflags, IGMP_ISR_SFLAG_DYNAMIC))
                        {
                          /* Reset Src-Timer to GMI */
                          isr->v_isr_liveness = igr->v_igr_liveness;
                          IGMP_TIMER_OFF (IGMP_LG (igi), isr->t_isr_liveness);
                          IGMP_TIMER_ON (IGMP_LG (igi), isr->t_isr_liveness,
                                         isr, igmp_if_srec_timer_liveness,
                                         isr->v_isr_liveness);
                        }

                      send_indication = PAL_TRUE;
                    }
                  else
                    IGMP_DBG_WARN (igi, TIB, "Failed to get Src-Rec for"
                                   " %r @ %s:%d", &isl->isl_saddr [idx],
                                   __FILE__, __LINE__);
                }

              /* Delete Src from TIB-B (Y-A) */
              if ((isr = igmp_if_srec_lookup (igr, PAL_FALSE,
                                              &isl->isl_saddr [idx])))
                {
                  ret = igmp_if_srec_delete (isr);

                  if (ret < IGMP_ERR_NONE)
                    {
                      IGMP_DBG_WARN (igi, FSM, "Src %r Delete from TIB-B"
                                     " Failed!", &isl->isl_saddr [idx]);
                      ret = IGMP_ERR_NONE;
                      continue;
                    }

                  igr->igr_src_b_tib_count -= 1;

                  send_indication = PAL_TRUE;
                }
            }

          /* Reset error-code */
          ret = IGMP_ERR_NONE;

          /* Send Q(G,X-A) */
          if (num_srcs)
            {
              igr->igr_rexmit_group_source_lmqc =
                   igr->igr_owning_igif->igif_conf.igifc_lmqc;

              ret = igmp_if_send_group_source_query (igr, minus_tib, num_srcs);

              if (ret < IGMP_ERR_NONE)
                IGMP_DBG_INFO (igi, FSM, "Grp-Src Specific Query Send Failed!");
            }

          /* Store the minus-TIB into G-Rec for Re-Transmissions */
          if (num_srcs && ! ret)
            {
              if (igr->igr_rexmit_srcs_tib)
                (void_t) igmp_if_grec_rexmit_tib_delete (igr);

              igr->igr_rexmit_srcs_tib = minus_tib;
              igr->igr_rexmit_srcs_tib_count = num_srcs;
            }
          else if (minus_tib)
            {
              /* Discard the Minus-TIB */
              for (pn = ptree_top (minus_tib); pn; pn = pn_next)
                {
                  pn_next = ptree_next (pn);

                  if (! (isr = pn->info))
                    continue;

                  igmp_if_srec_delete (isr);
                }
              ptree_finish (minus_tib);

              igr->igr_rexmit_srcs_tib = NULL;
              igr->igr_rexmit_srcs_tib_count = 0;
            }

          if (isl
              && isl->isl_static
              && ! isl->isl_num)
            {
              UNSET_FLAG (igr->igr_sflags, IGMP_IGR_SFLAG_STATIC);

              for (pn = ptree_top (igr->igr_src_a_tib); pn; pn = pn_next)
                {
                  pn_next = ptree_next (pn);

                  if (! (isr = pn->info))
                    continue;

                  UNSET_FLAG (isr->isr_sflags, IGMP_ISR_SFLAG_STATIC);

                  if (CHECK_FLAG (isr->isr_sflags, IGMP_ISR_SFLAG_DYNAMIC))
                    continue;

                  igmp_if_srec_delete (isr);

                  igr->igr_src_a_tib_count -= 1;

                  send_indication = PAL_TRUE;
                }

              if (! CHECK_FLAG (igr->igr_sflags, IGMP_IGR_SFLAG_DYNAMIC)
                  && ! igr->igr_src_a_tib_count)
                {
                  ret = igmp_igr_fsm_change_state (igr, IGMP_FMS_INCLUDE);

                  ret = igmp_igr_fsm_send_lmem_update (igr);

                  ret = igmp_if_grec_delete (igr);

                  send_indication = PAL_FALSE;
                }
            }
          else
            {
              /* Switch over to INCL mode */
              if (isl
                  && isl->isl_static
                  && ! CHECK_FLAG (igr->igr_sflags, IGMP_IGR_SFLAG_DYNAMIC)
                  && igr->igr_src_a_tib_count)
                {
                  /* Delete all records in TIB-B */
                  for (pn = ptree_top (igr->igr_src_b_tib); pn; pn = pn_next)
                    {
                      pn_next = ptree_next (pn);

                      if (! (isr = pn->info))
                        continue;

                      igmp_if_srec_delete (isr);

                      igr->igr_src_b_tib_count -= 1;
                    }

                  ret = igmp_igr_fsm_change_state (igr, IGMP_FMS_INCLUDE);

                  send_indication = PAL_TRUE;
                }
              else /* Send Q(G) */
                {
                  igr->igr_rexmit_group_lmqc =
                      igr->igr_owning_igif->igif_conf.igifc_lmqc;

                  ret = igmp_if_send_group_query (igr);

                  if (ret < IGMP_ERR_NONE)
                    IGMP_DBG_INFO (igi, FSM, "Group Specific Query Send Failed!");

                  if (! send_indication)
                    send_indication = (CHECK_FLAG (igr->igr_sflags,
                                                   IGMP_IGR_SFLAG_STATE_REFRESH));

                  ret = igmp_igr_fsm_change_state (igr, IGMP_FMS_EXCLUDE);
                }
            }

          if (send_indication)
            ret = igmp_igr_fsm_send_lmem_update (igr);
        }
      break;

    case IGMP_FME_CHG_TO_EXCL:
      if (CHECK_FLAG (igr->igr_sflags, IGMP_IGR_SFLAG_COMPAT_V1)
          || CHECK_FLAG (igr->igr_sflags, IGMP_IGR_SFLAG_COMPAT_V2)
          || CHECK_FLAG (igr->igr_sflags, IGMP_IGR_SFLAG_COMPAT_V3))
        {
          intersection_tib = ptree_init (IPV4_MAX_BITLEN);

          if (! intersection_tib)
            {
              IGMP_DBG_ERR (igi, TIB, "P-Trie Init Failed %s:%d",
                            __FILE__, __LINE__);

              ret = IGMP_ERR_OOM;
              goto EXIT;
            }

          minus_tib = ptree_init (IPV4_MAX_BITLEN);

          if (! minus_tib)
            {
              IGMP_DBG_ERR (igi, TIB, "P-Trie Init Failed %s:%d",
                            __FILE__, __LINE__);

              ret = IGMP_ERR_OOM;
              goto EXIT;
            }

          for (idx = 0; isl && idx < isl->isl_num; idx++)
            {
              if ((isr = igmp_if_srec_lookup (igr, PAL_FALSE,
                                              &isl->isl_saddr [idx])))
                {
                  pn = ptree_node_get (intersection_tib,
                                       (u_int8_t *) &isl->isl_saddr [idx],
                                       IPV4_MAX_PREFIXLEN);

                  if (! pn)
                    {
                      IGMP_DBG_ERR (igi, TIB, "Failed to get P-Trie node"
                                    " for %r @ %s:%d",
                                     &isl->isl_saddr [idx],
                                     __FILE__, __LINE__);

                      continue;
                    }

                  /* Switch over the Src-Rec to Xtion TIB (Y*A) */
                  if (isr->isr_owning_pn)
                    isr->isr_owning_pn->info = NULL;
                  pn->info = isr;
                  isr->isr_owning_pn = pn;
                }
              else
                {
                  pn = ptree_node_get (minus_tib,
                                       (u_int8_t *) &isl->isl_saddr [idx],
                                       IPV4_MAX_PREFIXLEN);

                  if (! pn)
                    {
                      IGMP_DBG_ERR (igi, TIB, "Failed to get P-Trie node"
                                    " for %r @ %s:%d", &isl->isl_saddr [idx],
                                     __FILE__, __LINE__);

                      continue;
                    }

                  /* If Src-Rec is in X just Switch it over */
                  if ((isr = igmp_if_srec_lookup (igr, PAL_TRUE,
                                                  &isl->isl_saddr [idx])))
                    {
                      if (isr->isr_owning_pn)
                        isr->isr_owning_pn->info = NULL;
                      pn->info = isr;
                      isr->isr_owning_pn = pn;

                      if (isl->isl_static)
                        SET_FLAG (isr->isr_sflags, IGMP_ISR_SFLAG_STATIC);
                      else
                        SET_FLAG (isr->isr_sflags, IGMP_ISR_SFLAG_DYNAMIC);
                    }
                  else
                    {
                      /* Create a new Src-Rec */
                      ret = igmp_if_srec_create (igr, &isr);

                      if (ret < IGMP_ERR_NONE)
                        {
                          IGMP_DBG_ERR (igi, TIB, "Failed to create Src node"
                                        " for %r @ %s:%d",
                                        &isl->isl_saddr [idx],
                                        __FILE__, __LINE__);

                          /* Release the P-Trie Node */
                          ptree_unlock_node (pn);

                          continue;
                        }

                      /* Couple the Src-Rec to P-Trie node */
                      pn->info = isr;
                      isr->isr_owning_pn = pn;

                      /* Run-up the Srcs Counter */
                      igr->igr_src_a_tib_count += 1;

                      if (isl->isl_static)
                        SET_FLAG (isr->isr_sflags, IGMP_ISR_SFLAG_STATIC);
                      else
                        {
                          SET_FLAG (isr->isr_sflags, IGMP_ISR_SFLAG_DYNAMIC);

                          /* Reset the Src-Timer to GMI */
                          isr->v_isr_liveness = igr->v_igr_liveness;
                          IGMP_TIMER_OFF (IGMP_LG (igi), isr->t_isr_liveness);
                          IGMP_TIMER_ON (IGMP_LG (igi), isr->t_isr_liveness,
                                         isr, igmp_if_srec_timer_liveness,
                                         isr->v_isr_liveness);
                        }
                    }
                }
            }

          /* Loose the Sources in TIB-A and TIB-B */
          for (pn = ptree_top (igr->igr_src_a_tib); pn; pn = pn_next)
            {
              pn_next = ptree_next (pn);

              if (! (isr = pn->info))
                continue;

              /* Move static source to minus TIB */
              if (CHECK_FLAG (isr->isr_sflags, IGMP_ISR_SFLAG_STATIC))
                {
                  UNSET_FLAG (isr->isr_sflags, IGMP_ISR_SFLAG_DYNAMIC);

                  /* Reset the Src-Timer to 0 */
                  isr->v_isr_liveness = 0;
                  IGMP_TIMER_OFF (IGMP_LG (igi), isr->t_isr_liveness);
                  IGMP_TIMER_ON (IGMP_LG (igi), isr->t_isr_liveness,
                                 isr, igmp_if_srec_timer_liveness,
                                 isr->v_isr_liveness);

                  pn_sec = ptree_node_get (minus_tib,
                                           (u_int8_t *) PTREE_NODE_KEY (
                                           isr->isr_owning_pn),
                                           IPV4_MAX_PREFIXLEN);

                  if (pn_sec)
                    {
                      /* Switch over the Src-Rec to Xtion TIB */
                      if (isr->isr_owning_pn)
                        isr->isr_owning_pn->info = NULL;
                      pn_sec->info = isr;
                      isr->isr_owning_pn = pn_sec;

                      continue;
                    }
                  else
                    IGMP_DBG_ERR (igi, TIB, "Failed to get P-Trie "
                                  "node for %r @ %s:%d",
                                  PTREE_NODE_KEY (isr->isr_owning_pn),
                                  __FILE__, __LINE__);
                }

              igmp_if_srec_delete (isr);

              igr->igr_src_a_tib_count -= 1;
            }
          ptree_finish (igr->igr_src_a_tib);

          for (pn = ptree_top (igr->igr_src_b_tib); pn; pn = pn_next)
            {
              pn_next = ptree_next (pn);

              if (! (isr = pn->info))
                continue;

              igmp_if_srec_delete (isr);

              igr->igr_src_b_tib_count -= 1;
            }
          ptree_finish (igr->igr_src_b_tib);

          if (igr->igr_rexmit_srcs_tib != igr->igr_src_a_tib
              && igr->igr_rexmit_srcs_tib != igr->igr_src_b_tib)
            del_rexmit_tib = PAL_TRUE;
          else
            del_rexmit_tib = PAL_FALSE;

          /* Install the new TIBs */
          igr->igr_src_a_tib = minus_tib;
          igr->igr_src_b_tib = intersection_tib;

          /* Send Q(G, A-Y) */
          igr->igr_rexmit_group_source_lmqc =
              igr->igr_owning_igif->igif_conf.igifc_lmqc;

          ret = igmp_if_send_group_source_query (igr, minus_tib,
                                                 igr->igr_src_a_tib_count);

          if (ret < IGMP_ERR_NONE)
            IGMP_DBG_INFO (igi, FSM, "Grp-Src Specific Query Send Failed!");

          /* Store the minus-TIB into G-Rec for Re-Transmissions */
          if (igr->igr_src_a_tib_count && ! ret)
            {
              if (igr->igr_rexmit_srcs_tib
                  && del_rexmit_tib == PAL_TRUE)
                {
                  (void_t) igmp_if_grec_rexmit_tib_delete (igr);
                }
              else
                {
                  igr->igr_rexmit_srcs_tib_count = 0;
                  igr->igr_rexmit_srcs_tib = NULL;
                }

              igr->igr_rexmit_srcs_tib = minus_tib;
              igr->igr_rexmit_srcs_tib_count = igr->igr_src_a_tib_count;
            }

          send_indication = PAL_TRUE;
        }

      /* Reset Group Timer to GMI for Dynamic events */
      if (! isl || ! isl->isl_static)
        {
          igr->v_igr_liveness = igr->igr_owning_igif->igif_gmi;
          IGMP_TIMER_OFF (IGMP_LG (igi), igr->t_igr_liveness);
          IGMP_TIMER_ON (IGMP_LG (igi), igr->t_igr_liveness,
                         igr, igmp_if_grec_timer_liveness,
                         igr->v_igr_liveness);
        }

      if (! send_indication)
        send_indication = (CHECK_FLAG (igr->igr_sflags,
                                       IGMP_IGR_SFLAG_STATE_REFRESH));

      ret = igmp_igr_fsm_change_state (igr, IGMP_FMS_EXCLUDE);

      if (send_indication)
        ret = igmp_igr_fsm_send_lmem_update (igr);
      break;

    case IGMP_FME_ALLOW_NEW_SRCS:
      if (CHECK_FLAG (igr->igr_sflags, IGMP_IGR_SFLAG_COMPAT_V1)
          || CHECK_FLAG (igr->igr_sflags, IGMP_IGR_SFLAG_COMPAT_V2)
          || CHECK_FLAG (igr->igr_sflags, IGMP_IGR_SFLAG_COMPAT_V3))
        {
          /* Merge Source-List into TIB-A */
          for (idx = 0; isl && idx < isl->isl_num; idx++)
            {
              if ((isr = igmp_if_srec_get (igr, PAL_TRUE,
                                           &isl->isl_saddr [idx])))
                {
                  if (! isl->isl_static)
                    SET_FLAG (isr->isr_sflags, IGMP_ISR_SFLAG_DYNAMIC);
                  else
                    SET_FLAG (isr->isr_sflags, IGMP_ISR_SFLAG_STATIC);

                  /* Reset Src-Timer to GMI */
                  isr->v_isr_liveness = igr->v_igr_liveness;
                  IGMP_TIMER_OFF (IGMP_LG (igi), isr->t_isr_liveness);
                  IGMP_TIMER_ON (IGMP_LG (igi), isr->t_isr_liveness,
                                 isr, igmp_if_srec_timer_liveness,
                                 isr->v_isr_liveness);
                }
              else
                IGMP_DBG_ERR (igr->igr_owning_igif->igif_owning_igi,
                              TIB, "Failed to get Src-Rec for"
                              " %r @ %s:%d", &isl->isl_saddr [idx],
                              __FILE__, __LINE__);

              /* Delete the Src-Rec for the Minus-TIB (Y-A) */
              if ((isr = igmp_if_srec_lookup (igr, PAL_FALSE,
                                              &isl->isl_saddr [idx])))
                {
                  ret = igmp_if_srec_delete (isr);

                  if (ret < IGMP_ERR_NONE)
                    {
                      IGMP_DBG_ERR (igi, TIB, "Failed to delete Src node"
                                    " for %r @ %s:%d", &isl->isl_saddr [idx],
                                    __FILE__, __LINE__);

                      ret = IGMP_ERR_NONE;
                      continue;
                    }

                  igr->igr_src_b_tib_count -= 1;

                  send_indication = PAL_TRUE;
                }
            }

          if (! send_indication)
            send_indication = (CHECK_FLAG (igr->igr_sflags,
                                           IGMP_IGR_SFLAG_STATE_REFRESH));

          ret = igmp_igr_fsm_change_state (igr, IGMP_FMS_EXCLUDE);

          if (send_indication)
            ret = igmp_igr_fsm_send_lmem_update (igr);
        }
      break;

    case IGMP_FME_BLOCK_OLD_SRCS:
      if (CHECK_FLAG (igr->igr_sflags, IGMP_IGR_SFLAG_COMPAT_V1)
          || CHECK_FLAG (igr->igr_sflags, IGMP_IGR_SFLAG_COMPAT_V2)
          || CHECK_FLAG (igr->igr_sflags, IGMP_IGR_SFLAG_COMPAT_V3))
        {
          minus_tib = ptree_init (IPV4_MAX_BITLEN);

          if (! minus_tib)
            {
              IGMP_DBG_ERR (igi, TIB, "P-Trie Init Failed %s:%d",
                            __FILE__, __LINE__);

              ret = IGMP_ERR_OOM;
              goto EXIT;
            }

          num_srcs = 0;

          for (idx = 0; isl && idx < isl->isl_num; idx++)
            {
              if (isl->isl_static)
                {
                  /* Check if A is in X, viz., TIB-A */
                  isr_tmp = igmp_if_srec_lookup (igr, PAL_TRUE,
                                                 &isl->isl_saddr [idx]);

                  if (isr_tmp)
                    {
                      if (CHECK_FLAG (isr_tmp->isr_sflags, IGMP_ISR_SFLAG_DYNAMIC))
                        UNSET_FLAG (isr_tmp->isr_sflags, IGMP_ISR_SFLAG_STATIC);
                      else
                        {
                          igmp_if_srec_delete (isr_tmp);
                          igr->igr_src_a_tib_count -= 1;
                          if (! igmp_igr_srec_count (igr, PAL_TRUE, IGMP_ISR_SFLAG_STATIC))
                            UNSET_FLAG (igr->igr_sflags, IGMP_IGR_SFLAG_STATIC);

                          send_indication = PAL_TRUE;
                        }
                    }
                }
              /* Check if A is in Y */
              else if (! igmp_if_srec_lookup (igr, PAL_FALSE,
                                              &isl->isl_saddr [idx]))
                {
                  /* Create a new Src-Rec */
                  ret = igmp_if_srec_create (igr, &isr);

                  if (ret < IGMP_ERR_NONE)
                    {
                      IGMP_DBG_ERR (igi, TIB, "Failed to create Src node"
                                    " for %r @ %s:%d", &isl->isl_saddr [idx],
                                    __FILE__, __LINE__);

                      ret = IGMP_ERR_NONE;
                      continue;
                    }

                  pn = ptree_node_get (minus_tib,
                                       (u_int8_t *) &isl->isl_saddr [idx],
                                       IPV4_MAX_PREFIXLEN);

                  if (! pn)
                    {
                      IGMP_DBG_ERR (igi, TIB, "Failed to get P-Trie node"
                                    " for %r @ %s:%d",
                                     &isl->isl_saddr [idx],
                                     __FILE__, __LINE__);

                      continue;
                    }

                  /* Insert Src into Minus-TIB (A-Y) */
                  pn->info = isr;
                  isr->isr_owning_pn = pn;
                  num_srcs += 1;

                  /* Check if A is in X, viz., TIB-A */
                  isr_tmp = igmp_if_srec_lookup (igr, PAL_TRUE,
                                                 &isl->isl_saddr [idx]);

                  if (isr_tmp)
                    continue;

                  /* Insert Src into TIB-A */
                  isr = igmp_if_srec_get (igr, PAL_TRUE,
                                          &isl->isl_saddr [idx]);

                  if (! isr)
                    {
                      IGMP_DBG_ERR (igi, TIB, "Failed to get Src-Rec for"
                                    " %r @ %s:%d", &isl->isl_saddr [idx],
                                    __FILE__, __LINE__);

                      continue;
                    }

                  SET_FLAG (isr->isr_sflags, IGMP_ISR_SFLAG_DYNAMIC);
                  /* Reset Source-Timer for (A-X-Y) */
                  isr->v_isr_liveness = igr->v_igr_liveness;
                  IGMP_TIMER_OFF (IGMP_LG (igi), isr->t_isr_liveness);
                  IGMP_TIMER_ON (IGMP_LG (igi), isr->t_isr_liveness,
                                 isr, igmp_if_srec_timer_liveness,
                                 isr->v_isr_liveness);
                }
            }

          igr->igr_rexmit_group_source_lmqc =
              igr->igr_owning_igif->igif_conf.igifc_lmqc;

          ret = igmp_if_send_group_source_query (igr, minus_tib, num_srcs);

          /* Store the Minus-TIB for Re-Transmissions */
          if (ret < IGMP_ERR_NONE)
            IGMP_DBG_INFO (igi, FSM, "Grp-Src Specific Query Send Failed!");
          else if (num_srcs)
            {
              if (igr->igr_rexmit_srcs_tib)
                (void_t) igmp_if_grec_rexmit_tib_delete (igr);

              igr->igr_rexmit_srcs_tib = minus_tib;
              igr->igr_rexmit_srcs_tib_count = num_srcs;
            }
          else if (minus_tib)
            {
              /* Discard the Minus-TIB */
              for (pn = ptree_top (minus_tib); pn; pn = pn_next)
                {
                  pn_next = ptree_next (pn);

                  if (! (isr = pn->info))
                    continue;

                  igmp_if_srec_delete (isr);
                }
              ptree_finish (minus_tib);

              igr->igr_rexmit_srcs_tib = NULL;
              igr->igr_rexmit_srcs_tib_count = 0;
            }

          ret = igmp_igr_fsm_change_state (igr, IGMP_FMS_EXCLUDE);

          if (send_indication)
            ret = igmp_igr_fsm_send_lmem_update (igr);
        }
      break;

    case IGMP_FME_GROUP_TIMER_EXPIRY:
      /* Delete all records in TIB-B */
      for (pn = ptree_top (igr->igr_src_b_tib); pn; pn = pn_next)
        {
          pn_next = ptree_next (pn);

          if (! (isr = pn->info))
            continue;

          igmp_if_srec_delete (isr);

          igr->igr_src_b_tib_count -= 1;

          send_indication = PAL_TRUE;
        }

      UNSET_FLAG (igr->igr_sflags, IGMP_IGR_SFLAG_DYNAMIC);

      if (CHECK_FLAG (igr->igr_sflags, IGMP_IGR_SFLAG_STATIC)
          && ! igr->igr_src_a_tib_count)
        {
          ret = igmp_igr_fsm_change_state (igr, IGMP_FMS_EXCLUDE);
  
          if (send_indication) 
              ret = igmp_igr_fsm_send_lmem_update (igr);
        }
      else
        {
          /* Switch to INCL mode */
          ret = igmp_igr_fsm_change_state (igr, IGMP_FMS_INCLUDE);

          /* Send INCL <Src-List> Indication */
          ret = igmp_igr_fsm_send_lmem_update (igr);

          if (! igr->igr_src_a_tib_count)
            igmp_if_grec_delete (igr);
        }
      break;

    case IGMP_FME_SOURCE_TIMER_EXPIRY:
      if ((isr = igmp_if_srec_lookup (igr, PAL_TRUE,
                                      &isl->isl_saddr [0])))
        {
          if (CHECK_FLAG (isr->isr_sflags, IGMP_ISR_SFLAG_STATIC))
            {
              UNSET_FLAG (isr->isr_sflags, IGMP_ISR_SFLAG_DYNAMIC);
              send_indication = PAL_FALSE;
            }
          else
            {
              /* Move the Src-Rec from TIB-A to TIB-B */
              pn = ptree_node_get (igr->igr_src_b_tib,
                                   PTREE_NODE_KEY (isr->isr_owning_pn),
                                   IPV4_MAX_PREFIXLEN);

              if (! pn)
                {
                  ret = IGMP_ERR_OOM;
                  goto EXIT;
                }

              isr->isr_owning_pn->info = NULL;
              ptree_unlock_node (isr->isr_owning_pn);

              pn->info = isr;
              isr->isr_owning_pn = pn;

              igr->igr_src_a_tib_count -= 1;
              igr->igr_src_b_tib_count += 1;

              send_indication = PAL_TRUE;
          }
        }

      if (! send_indication)
        send_indication = (CHECK_FLAG (igr->igr_sflags,
                                       IGMP_IGR_SFLAG_STATE_REFRESH));

      ret = igmp_igr_fsm_change_state (igr, IGMP_FMS_EXCLUDE);

      if (send_indication)
        ret = igmp_igr_fsm_send_lmem_update (igr);

      break;

    case IGMP_FME_MANUAL_CLEAR:
    case IGMP_FME_IMMEDIATE_LEAVE:
      if (! isl)
        {
          /* Release the Source P-Trie 'A' */
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

          /* Release the Source P-Trie 'B' */
          for (pn = ptree_top (igr->igr_src_b_tib); pn; pn = pn_next)
            {
              pn_next = ptree_next (pn);

              if (! (isr = pn->info))
                continue;

              igmp_if_srec_delete (isr);

              igr->igr_src_b_tib_count -= 1;
            }

          UNSET_FLAG (igr->igr_sflags, IGMP_IGR_SFLAG_DYNAMIC);
          IGMP_TIMER_OFF (IGMP_LG (igi), igr->t_igr_liveness);
          pal_mem_set (&igr->igr_last_reporter, 0, 
                       sizeof (struct pal_in4_addr));

          if (CHECK_FLAG (igr->igr_sflags, IGMP_IGR_SFLAG_STATIC)
              && ! igr->igr_src_a_tib_count)
            {
              ret = igmp_igr_fsm_change_state (igr, IGMP_FMS_EXCLUDE);
          
              ret = igmp_igr_fsm_send_lmem_update (igr);
            }
          else
            {
              /* Switch to INCL mode */
              ret = igmp_igr_fsm_change_state (igr, IGMP_FMS_INCLUDE);

              /* Send INCL <Src-List> Indication */
              ret = igmp_igr_fsm_send_lmem_update (igr);
 
              if (! igr->igr_src_a_tib_count)
                igmp_if_grec_delete (igr);
            }
        }
      else if ((isr = igmp_if_srec_lookup (igr, PAL_TRUE,
                                           &isl->isl_saddr [0]))
               || (isr_tmp = igmp_if_srec_lookup (igr, PAL_FALSE,
                                                  &isl->isl_saddr [0])))
        {
          if (! isr)
            isr = isr_tmp;

          igmp_if_srec_delete (isr);

          if (! isr_tmp)
            igr->igr_src_a_tib_count -= 1;
          else
            igr->igr_src_b_tib_count -= 1;

          ret = igmp_igr_fsm_change_state (igr, IGMP_FMS_INCLUDE);

          ret = igmp_igr_fsm_send_lmem_update (igr);
        }
      break;

    case IGMP_FME_MANUAL_LMEM_UPDATE:
      ret = igmp_igr_fsm_send_lmem_update (igr);
      break;

    case IGMP_FME_INVALID:
      ret = IGMP_ERR_DOOM;
      break;
    }

EXIT:

  IGMP_FN_EXIT (ret);
}

/* IGMP Membership Status Message Indication */
s_int32_t
igmp_igr_fsm_send_lmem_update (struct igmp_group_rec *igr)
{
  struct igmp_source_list *isl;
  struct igmp_source_rec *isr;
  struct igmp_instance *igi;
  struct igmp_svc_reg *igsr;
  u_int32_t src_list_size;
  bool_t send_lmem_update;
  struct igmp_if *ds_igif;
  struct ptree_node *pn;
  struct igmp_if *igif;
  struct listnode *mm;
  struct listnode *nn;
  bool_t consol_mship;
  u_int16_t idx;
  s_int32_t ret;

  IGMP_FN_ENTER (Send G-Rec LMEM Update);

  igi = igr->igr_owning_igif->igif_owning_igi;
  igif = igr->igr_owning_igif;
  ret = IGMP_ERR_NONE;
  igsr = NULL;
  isl = NULL;
  idx = 0;

  if (igr->igr_src_a_tib_count || igr->igr_src_b_tib_count)
    switch (igr->igr_filt_mode_state)
      {
      case IGMP_FMS_INCLUDE:
        src_list_size = sizeof (struct igmp_source_list)
                        + sizeof (struct pal_in4_addr) *
                          igr->igr_src_a_tib_count;

        isl = XCALLOC (MTYPE_IGMP_SRC_LIST, src_list_size);

        if (! isl)
          {
            IGMP_DBG_ERR (igi, FSM, "Cannot allocate memory"
                          " (%d) @ %s:%d", src_list_size,
                          __FILE__, __LINE__);

            ret = IGMP_ERR_OOM;
            goto EXIT;
          }

        isl->isl_num = igr->igr_src_a_tib_count;

        for (pn = ptree_top (igr->igr_src_a_tib), idx = 0;
             pn; pn = ptree_next (pn))
          {
            if (! (isr = pn->info))
              continue;

            IPV4_ADDR_COPY (&isl->isl_saddr [idx],
                            PTREE_NODE_KEY (pn));

            idx += 1;
          }
        break;

      case IGMP_FMS_EXCLUDE:
        src_list_size = sizeof (struct igmp_source_list)
                        + sizeof (struct pal_in4_addr) *
                          igr->igr_src_b_tib_count;

        isl = XCALLOC (MTYPE_IGMP_SRC_LIST, src_list_size);

        if (! isl)
          {
            IGMP_DBG_ERR (igi, FSM, "Cannot allocate memory"
                          " (%d) @ %s:%d", src_list_size,
                          __FILE__, __LINE__);

            ret = IGMP_ERR_OOM;
            goto EXIT;
          }

        isl->isl_num = igr->igr_src_b_tib_count;

        for (pn = ptree_top (igr->igr_src_b_tib), idx = 0;
             pn; pn = ptree_next (pn))
          {
            if (! (isr = pn->info))
              continue;

            IPV4_ADDR_COPY (&isl->isl_saddr [idx],
                            PTREE_NODE_KEY (pn));

            idx += 1;
          }
        break;

      case IGMP_FMS_INVALID:
      case IGMP_FMS_MAX:
        ret = IGMP_ERR_DOOM;
        goto EXIT;
        break;
      }

  /* Decide on sending L-mem Ind or M-ship Consolidation */
  send_lmem_update = PAL_FALSE;
  consol_mship = PAL_FALSE;

  if (CHECK_FLAG (igif->igif_sflags, IGMP_IF_SFLAG_SNOOPING))
    {
      UNSET_FLAG (igr->igr_sflags, IGMP_IGR_SFLAG_STATE_REFRESH);

      /* No_report_suppression should be done only for L2 interfaces */
      if (INTF_TYPE_L2 (igif->igif_owning_ifp)
          && CHECK_FLAG (igif->igif_conf.igifc_cflags,
                         IGMP_IF_CFLAG_SNOOP_NO_REPORT_SUPPRESS)
          && (igif->igif_conf.igifc_version == IGMP_VERSION_1
              || igif->igif_conf.igifc_version == IGMP_VERSION_2))
        {
          if (! igif->igif_hst_igif)
            {
              IGMP_DBG_WARN (igi, FSM, "Host-IF NOT present "
                             "for L2 Phy-IF %s",
                             igif->igif_owning_ifp->name);

              ret = IGMP_ERR_DOOM;
              goto EXIT;
            }

          LIST_LOOP (&igif->igif_hst_igif->igif_hst_dsif_lst,
                     ds_igif, nn)
            if (ds_igif != igif)
              {
                ret = igmp_if_hst_forward_report (igif, ds_igif);

                if (ret < IGMP_ERR_NONE)
                  {
                    IGMP_DBG_INFO (igi, FSM, "Forwarding Msg "
                                   "from %s onto %s Failed!",
                                   igif->igif_owning_ifp->name,
                                   ds_igif->igif_owning_ifp->name);

                    /* Just loose err-code */
                    ret = IGMP_ERR_NONE;
                  }
              }
        }

      /* Report-Suppression === Snooping Proxy */
      if (! CHECK_FLAG (igif->igif_sflags,
                        IGMP_IF_SFLAG_SVC_TYPE_L3))
        {
          send_lmem_update = PAL_TRUE;
          consol_mship = PAL_TRUE;
        }
      else if (CHECK_FLAG (igif->igif_conf.igifc_cflags,
                           IGMP_IF_CFLAG_MROUTE_PROXY))
        consol_mship = PAL_TRUE;
      else if (! CHECK_FLAG (igif->igif_conf.igifc_cflags,
                             IGMP_IF_CFLAG_PROXY_SERVICE)
               && CHECK_FLAG (igif->igif_sflags,
                              IGMP_IF_SFLAG_MCAST_ENABLED)
               && igif->igif_paddr)
        send_lmem_update = PAL_TRUE;
    }
  /* Consolidate into Proxy Membership Database */
  else if (CHECK_FLAG (igif->igif_conf.igifc_cflags,
                       IGMP_IF_CFLAG_MROUTE_PROXY))
    consol_mship = PAL_TRUE;
  /* Update MRouting-Protos only if IFF in non-proxy mode */
  else if (! CHECK_FLAG (igif->igif_conf.igifc_cflags,
                         IGMP_IF_CFLAG_PROXY_SERVICE))
    send_lmem_update = PAL_TRUE;

  /* Consolidate onto Host-side Membership TIB */
  if (consol_mship)
    {
      ret = igmp_if_hst_consolidate_mship (igr, isl);

      if (ret < IGMP_ERR_NONE)
        {
          IGMP_DBG_INFO (igi, FSM, "Host Consol MShip Failed"
                         " for %r on %s",
                         PTREE_NODE_KEY (igr->igr_owning_pn),
                         igif->igif_owning_ifp->name);

          ret = IGMP_ERR_NONE;
        }
    }

  /* Obtain the Svc Registrant(s) for this Interface */
  LIST_LOOP (&igi->igi_svc_reg_lst, igsr, mm)
    {
      if (! igmp_if_svc_reg_match (igif, igsr))
        continue;

      /* Send Local-Mem Indication */
      if (send_lmem_update
          && igsr->igsr_cback_lmem_update)
        {
          ret = igsr->igsr_cback_lmem_update
                (igsr->igsr_su_id,
                 igif->igif_owning_ifp,
                 igif->igif_su_id,
                 igr->igr_filt_mode_state,
                 (struct pal_in4_addr *)
                 PTREE_NODE_KEY (igr->igr_owning_pn),
                 isl ? isl->isl_num : 0,
                 isl ? &isl->isl_saddr [0] : NULL);

          if (ret < IGMP_ERR_NONE)
            {
              IGMP_DBG_INFO (igi, FSM, "Lmem-Update callback failed"
                             " for %r on %s",
                             PTREE_NODE_KEY (igr->igr_owning_pn),
                             igif->igif_owning_ifp->name);

              ret = IGMP_ERR_NONE;
            }
        }
    }

EXIT:
  if (isl)
    XFREE (MTYPE_IGMP_SRC_LIST, isl);

  IGMP_FN_EXIT (ret);
}

/*********************************************************************/
/*                   IGMP HOST-SIDE FSM                              */
/*********************************************************************/

/*
 * IGMP G-Rec Host-side FSM Action Function Array
 * indexed on current state
 */
igmp_hst_igr_fsm_act_func_t igmp_hst_igr_fsm_action_func
                            [IGMP_HFMS_MAX] =
{
  igmp_hst_igr_fsm_action_invalid,
  igmp_hst_igr_fsm_action_include,
  igmp_hst_igr_fsm_action_exclude
};

/* IGMP Proxy Group Record FSM Event Handler */
s_int32_t
igmp_hst_igr_fsm_process_event (struct igmp_group_rec *hst_igr,
                                enum igmp_hst_filter_mode_event igr_hfme,
                                struct igmp_source_list *isl)
{
  struct igmp_instance *igi;
  s_int32_t ret;

  IGMP_FN_ENTER (Host Process Event);

  ret = IGMP_ERR_NONE;

  /* Sanity check */
  if (! hst_igr)
    {
      ret = IGMP_ERR_MALFORMED_ARG;
      goto EXIT;
    }

  igi = hst_igr->igr_owning_igif->igif_owning_igi;

  IGMP_DBG_INFO (igi, FSM, "I=%s, G=%r, State: %s, Event: %s",
                 hst_igr->igr_owning_igif->igif_owning_ifp->name,
                 PTREE_NODE_KEY (hst_igr->igr_owning_pn),
                 IGMP_HFMS_STR (hst_igr->igr_filt_mode_state),
                 IGMP_HFME_STR (igr_hfme));

  /* Invoke Action-Func defined for the current-state */
  ret = igmp_hst_igr_fsm_action_func [hst_igr->igr_filt_mode_state]
        (hst_igr, igr_hfme, isl);

EXIT:

  IGMP_FN_EXIT (ret);
}

/* Change IGMP Proxy Group Rec FSM State */
s_int32_t
igmp_hst_igr_fsm_change_state (struct igmp_group_rec *hst_igr,
                               enum igmp_hst_filter_mode_state igr_hfms)
{
  struct igmp_instance *igi;
  s_int32_t ret;

  IGMP_FN_ENTER (Host State Change);

  igi = hst_igr->igr_owning_igif->igif_owning_igi;
  ret = IGMP_ERR_NONE;

  IGMP_DBG_INFO (igi, FSM, "%s(%d)->%s(%d)",
                IGMP_HFMS_STR (hst_igr->igr_filt_mode_state),
                hst_igr->igr_filt_mode_state,
                IGMP_HFMS_STR (igr_hfms), igr_hfms);

  /* Change to new state */
  hst_igr->igr_filt_mode_state = igr_hfms;

  IGMP_FN_EXIT (ret);
}

/* IGMP Proxy FSM action function for Invalid-state */
s_int32_t
igmp_hst_igr_fsm_action_invalid (struct igmp_group_rec *hst_igr,
                                 enum igmp_hst_filter_mode_event igr_hfme,
                                 struct igmp_source_list *isl)
{
  struct igmp_instance *igi;
  s_int32_t ret;

  IGMP_FN_ENTER (Host Act Invalid);

  igi = hst_igr->igr_owning_igif->igif_owning_igi;
  ret = IGMP_ERR_DOOM;

  IGMP_DBG_ERR (igi, FSM, "!!! State: INVALID Event: %s(%d)",
                IGMP_HFME_STR (igr_hfme), igr_hfme);

  pal_assert (0);

  IGMP_FN_EXIT (ret);
}

/* IGMP Proxy FSM action function for Include-state */
s_int32_t
igmp_hst_igr_fsm_action_include (struct igmp_group_rec *hst_igr,
                                 enum igmp_hst_filter_mode_event igr_hfme,
                                 struct igmp_source_list *isl)
{
  struct ptree *intersection_tib;
  struct igmp_source_rec *isr;
  struct ptree_node *pn_next;
  struct igmp_instance *igi;
  struct igmp_if *hst_igif;
  bool_t send_lmem_update;
  struct ptree *minus_tib;
  struct ptree_node *pn;
  u_int16_t num_srcs;
  bool_t send_report;
  bool_t chg_to_incl;
  bool_t grec_del;
  u_int32_t idx;
  s_int32_t ret;

  IGMP_FN_ENTER (Host Act Include);

  igi = hst_igr->igr_owning_igif->igif_owning_igi;
  hst_igif = hst_igr->igr_owning_igif;
  send_lmem_update = PAL_FALSE;
  intersection_tib = NULL;
  send_report = PAL_FALSE;
  chg_to_incl = PAL_FALSE;
  grec_del = PAL_FALSE;
  ret = IGMP_ERR_NONE;
  minus_tib = NULL;
  num_srcs = 0;
  isr = NULL;

  switch (igr_hfme)
    {
    case IGMP_HFME_INCL:

      /* Get the Sources to be deleted */
      ret = igmp_hst_igr_fsm_get_block_tib (hst_igr);

      if (ret < IGMP_ERR_NONE)
        {
          IGMP_DBG_INFO (igi, FSM, "Get Block TIB FAILED(%d)!", ret);
          goto EXIT;
        }

      if (hst_igr->igr_rexmit_srcs_block_tib_count)
        {
          ret = igmp_hst_igr_fsm_mfc_reprog (hst_igr,hst_igr->igr_rexmit_block_tib);
          if (ret < IGMP_ERR_NONE)
            IGMP_DBG_INFO (igi, FSM, "%r MFC Reprog Failed",
                          (struct pal_in4_addr *)
                          PTREE_NODE_KEY (hst_igr->igr_owning_pn));

          hst_igr->igr_rexmit_hrt = IGMP_HRT_BLOCK_OLD_SRCS;

          hst_igr->igr_rexmit_group_source_lmqc =
              hst_igif->igif_conf.igifc_rv;

          ret = igmp_if_hst_send_group_source_report
                (hst_igr, hst_igr->igr_rexmit_block_tib);

          /* Store the Block-TIB for Re-Transmissions */
          if (ret < IGMP_ERR_NONE)
            {
              IGMP_DBG_INFO (igi, FSM, "Grp-Src Report Send Failed(%d)!",
                             ret);

              if (hst_igr->igr_rexmit_block_tib)
                ptree_finish (hst_igr->igr_rexmit_block_tib);

              hst_igr->igr_rexmit_block_tib = NULL;
            }
         }

      /* Preprocess INCL Event if necessary */
      if (! isl
          || ! isl->isl_num)
        {
          ret = igmp_hst_igr_fsm_preprocess_incl_event
                (hst_igr, &grec_del, &chg_to_incl, PAL_FALSE);

          if (ret < IGMP_ERR_NONE)
            {
              IGMP_DBG_INFO (igi, FSM, "INCL Preproc FAILED(%d)!", ret);

              goto EXIT;
            }
        }

      /* GRec may need be deleted */
      if (grec_del)
        {
          /* Loose the Sources in TIB-A and TIB-B */
          for (pn = ptree_top (hst_igr->igr_src_a_tib); pn; pn = pn_next)
            {
              pn_next = ptree_next (pn);

              if (! (isr = pn->info))
                continue;

              igmp_if_srec_delete (isr);

              hst_igr->igr_src_a_tib_count -= 1;
            }

          for (pn = ptree_top (hst_igr->igr_src_b_tib); pn; pn = pn_next)
            {
              pn_next = ptree_next (pn);

              if (! (isr = pn->info))
                continue;

              igmp_if_srec_delete (isr);

              hst_igr->igr_src_b_tib_count -= 1;
            }

          if (CHECK_FLAG (hst_igif->igif_sflags, IGMP_IF_SFLAG_SVC_TYPE_L2)
              && CHECK_FLAG (hst_igif->igif_sflags, IGMP_IF_SFLAG_SVC_TYPE_L3))
            send_lmem_update = PAL_TRUE;
        }
      else /* ==> non-NULL incoming Src-List */
        {
          minus_tib = ptree_init (IPV4_MAX_BITLEN);

          if (! minus_tib)
            {
              IGMP_DBG_ERR (igi, TIB, "P-Trie Init Failed %s:%d",
                            __FILE__, __LINE__);

              ret = IGMP_ERR_OOM;
              goto EXIT;
            }

          /* Merge Source-List into TIB-A */
          for (idx = 0; isl && idx < isl->isl_num; idx++)
            {
              /* Prepare the Src-List for ALLOW (B - (A*B)) */
              if (! (igmp_if_srec_lookup (hst_igr, PAL_TRUE,
                                          &isl->isl_saddr [idx])))
                {
                  /* Create a new Src-Rec */
                  ret = igmp_if_srec_create (hst_igr, &isr);

                  if (ret < IGMP_ERR_NONE)
                    {
                      IGMP_DBG_ERR (igi, TIB, "Failed to create Src node"
                                    " for %r @ %s:%d", &isl->isl_saddr [idx],
                                    __FILE__, __LINE__);

                      continue;
                    }

                  pn = ptree_node_get (minus_tib,
                                       (u_int8_t *) &isl->isl_saddr [idx],
                                       IPV4_MAX_PREFIXLEN);

                  if (! pn)
                    {
                      IGMP_DBG_ERR (igi, TIB, "Failed to get P-Trie node"
                                    " for %r @ %s:%d",
                                     &isl->isl_saddr [idx],
                                     __FILE__, __LINE__);

                      igmp_if_srec_delete (isr);
                      continue;
                    }

                  /* Install the Src-Rec into Minus TIB */
                  pn->info = isr;
                  isr->isr_owning_pn = pn;

                  /* Run-up the Srcs Counter */
                  num_srcs += 1;

                  send_report = PAL_TRUE;

                  if (CHECK_FLAG (hst_igif->igif_sflags,
                                  IGMP_IF_SFLAG_SVC_TYPE_L2)
                      && CHECK_FLAG (hst_igif->igif_sflags,
                                     IGMP_IF_SFLAG_SVC_TYPE_L3))
                    send_lmem_update = PAL_TRUE;

                  /* Merge INCL Src-List into TIB-A */
                  if (! (igmp_if_srec_get (hst_igr, PAL_TRUE,
                                           (struct pal_in4_addr *)
                                           PTREE_NODE_KEY (pn))))
                    {
                      IGMP_DBG_ERR (igi, TIB, "Failed to create "
                                    "Src node for %r @ %s:%d",
                                    PTREE_NODE_KEY (pn),
                                    __FILE__, __LINE__);

                      /* Hopeless situation, too bad */
                      continue;
                    }
                }
            }
        }

      ret = igmp_hst_igr_fsm_change_state (hst_igr, IGMP_HFMS_INCLUDE);

      if (send_lmem_update)
        {
           ret = igmp_igr_fsm_send_lmem_update (hst_igr);

           if (ret < IGMP_ERR_NONE)
             IGMP_DBG_INFO (igi, FSM, "Lmem-Update failed for %r on %s",
                            PTREE_NODE_KEY (hst_igr->igr_owning_pn),
                            hst_igif->igif_owning_ifp->name);
        }

      /* Grp-Rec Del (==> Send INCL <NULL> Indication) */
      if (grec_del)
        {
          ret = igmp_hst_igr_fsm_mfc_reprog (hst_igr, NULL);

          if (ret < IGMP_ERR_NONE)
            IGMP_DBG_INFO (igi, FSM, "%r MFC Reprog Failed",
                           (struct pal_in4_addr *)
                           PTREE_NODE_KEY (hst_igr->igr_owning_pn));

          hst_igr->igr_rexmit_group_lmqc =
              hst_igif->igif_conf.igifc_rv;

          hst_igr->igr_rexmit_hrt = IGMP_HRT_CHG_TO_INCL;

          ret = igmp_if_hst_send_group_report (hst_igr);

          if (ret < IGMP_ERR_NONE)
            {
              IGMP_DBG_INFO (igi, FSM, "Grp Report Send Failed(%d)!",
                             ret);

              igmp_if_grec_delete (hst_igr);
            }
        }
      /* Send SCR (G, (B-(A*B))) */
      else if (send_report)
        {
          ret = igmp_hst_igr_fsm_mfc_reprog (hst_igr, minus_tib);

          if (ret < IGMP_ERR_NONE)
            IGMP_DBG_INFO (igi, FSM, "%r MFC Reprog Failed",
                           (struct pal_in4_addr *)
                           PTREE_NODE_KEY (hst_igr->igr_owning_pn));

          hst_igr->igr_rexmit_group_source_lmqc =
              hst_igif->igif_conf.igifc_rv;

          hst_igr->igr_rexmit_hrt = IGMP_HRT_ALLOW_NEW_SRCS;

          ret = igmp_if_hst_send_group_source_report
                (hst_igr, minus_tib);

          /* Store the Minus-TIB for Re-Transmissions */
          if (ret < IGMP_ERR_NONE)
            {
              IGMP_DBG_INFO (igi, FSM, "Grp-Src Report Send Failed(%d)!",
                             ret);

              ptree_finish (minus_tib);
            }
          else
            {
              if (hst_igr->igr_rexmit_allow_tib)
                ptree_finish (hst_igr->igr_rexmit_allow_tib);

              hst_igr->igr_rexmit_allow_tib = minus_tib;
              hst_igr->igr_rexmit_srcs_allow_tib_count = num_srcs;
            }
        }
      /* No resultant change */
        else
          {
            ret = igmp_hst_igr_fsm_mfc_reprog (hst_igr, NULL);
            
             if (ret < IGMP_ERR_NONE)
                IGMP_DBG_INFO (igi, FSM, "%r MFC Reprog Failed",
                              (struct pal_in4_addr *)
                               PTREE_NODE_KEY (hst_igr->igr_owning_pn));
  
             if (minus_tib)
               ptree_finish (minus_tib);
           }
  
      break;

    case IGMP_HFME_EXCL:
      for (idx = 0; isl && idx < isl->isl_num; idx++)
        {
          if (! (igmp_if_srec_lookup (hst_igr, PAL_TRUE,
                                      &isl->isl_saddr [idx])))
            {
              /* Merge New Src into TIB-B */
              isr = igmp_if_srec_get (hst_igr, PAL_FALSE,
                                      &isl->isl_saddr [idx]);

              if (! isr)
                {
                  IGMP_DBG_WARN (igi, TIB, "Failed to get Src-Rec for"
                                 " %r @ %s:%d", &isl->isl_saddr [idx],
                                 __FILE__, __LINE__);

                  continue;
                }
            }
        }

      /* Loose the Sources in TIB-A */
      for (pn = ptree_top (hst_igr->igr_src_a_tib); pn; pn = pn_next)
        {
          pn_next = ptree_next (pn);

          if (! (isr = pn->info))
            continue;

          igmp_if_srec_delete (isr);

          hst_igr->igr_src_a_tib_count -= 1;
        }

      ret = igmp_hst_igr_fsm_change_state (hst_igr, IGMP_HFMS_EXCLUDE);

      ret = igmp_hst_igr_fsm_mfc_reprog (hst_igr, NULL);

      if (ret < IGMP_ERR_NONE)
        IGMP_DBG_INFO (igi, FSM, "%r MFC Reprog Failed",
                       (struct pal_in4_addr *)
                       PTREE_NODE_KEY (hst_igr->igr_owning_pn));

      if (CHECK_FLAG (hst_igif->igif_sflags, IGMP_IF_SFLAG_SVC_TYPE_L2)
          && CHECK_FLAG (hst_igif->igif_sflags, IGMP_IF_SFLAG_SVC_TYPE_L3))
        {

           ret = igmp_igr_fsm_send_lmem_update (hst_igr);

           if (ret < IGMP_ERR_NONE)
             IGMP_DBG_INFO (igi, FSM, "Lmem-Update failed for %r on %s",
                            PTREE_NODE_KEY (hst_igr->igr_owning_pn),
                            hst_igif->igif_owning_ifp->name);
         }

      hst_igr->igr_rexmit_group_lmqc = hst_igif->igif_conf.igifc_rv;

      hst_igr->igr_rexmit_hrt = IGMP_HRT_CHG_TO_EXCL;

      ret = igmp_if_hst_send_group_report (hst_igr);

      if (ret < IGMP_ERR_NONE)
        IGMP_DBG_INFO (igi, FSM, "Grp Report Send Failed(%d)!", ret);
      break;

    case IGMP_HFME_MFC_MSG:
      if (isl
          && isl->isl_num)
        {
          isr = igmp_if_srec_lookup (hst_igr, PAL_TRUE,
                                     &isl->isl_saddr [0]);

          /* Prepare content and programme MFC */
          ret = igmp_hst_igr_fsm_mfc_prog (hst_igr,
                                           &isl->isl_saddr [0],
                                           ! hst_igr->igr_src_a_tib_count ?
                                           PAL_TRUE : PAL_FALSE,
                                           isr ? PAL_FALSE : PAL_TRUE);

          if (ret < IGMP_ERR_NONE)
            {
              IGMP_DBG_INFO (igi, FSM, "MFC Prog Failed!");

              goto EXIT;
            }
        }
      break;

    case IGMP_HFME_MANUAL_CLEAR:
      if (! isl)
        {
          /* Release the Source P-Trie 'A' */
          for (pn = ptree_top (hst_igr->igr_src_a_tib); pn; pn = pn_next)
            {
              pn_next = ptree_next (pn);

              if (! (isr = pn->info))
                continue;

              igmp_if_srec_delete (isr);

              hst_igr->igr_src_a_tib_count -= 1;
            }

          /* Release the Source P-Trie 'B' */
          for (pn = ptree_top (hst_igr->igr_src_b_tib); pn; pn = pn_next)
            {
              pn_next = ptree_next (pn);

              if (! (isr = pn->info))
                continue;

              igmp_if_srec_delete (isr);

              hst_igr->igr_src_b_tib_count -= 1;
            }

          ret = igmp_igr_fsm_change_state (hst_igr, IGMP_HFMS_INCLUDE);

          ret = igmp_hst_igr_fsm_mfc_reprog (hst_igr, NULL);

          if (ret < IGMP_ERR_NONE)
            IGMP_DBG_INFO (igi, FSM, "%r MFC Reprog Failed",
                           (struct pal_in4_addr *)
                           PTREE_NODE_KEY (hst_igr->igr_owning_pn));

          hst_igr->igr_rexmit_group_lmqc =
              hst_igif->igif_conf.igifc_rv;

          hst_igr->igr_rexmit_hrt = IGMP_HRT_CHG_TO_INCL;

          ret = igmp_if_hst_send_group_report (hst_igr);

          if (ret < IGMP_ERR_NONE)
            {
              IGMP_DBG_INFO (igi, FSM, "Grp Report Send Failed(%d)!",
                             ret);

              igmp_if_grec_delete (hst_igr);
            }
        }
      else if ((isr = igmp_if_srec_lookup (hst_igr, PAL_TRUE,
                                           &isl->isl_saddr [0])))
        {
          igmp_if_srec_delete (isr);

          hst_igr->igr_src_a_tib_count -= 1;

          ret = igmp_igr_fsm_change_state (hst_igr, IGMP_FMS_INCLUDE);

          if (! hst_igr->igr_src_a_tib_count)
            {
              ret = igmp_hst_igr_fsm_mfc_reprog (hst_igr, NULL);

              if (ret < IGMP_ERR_NONE)
                IGMP_DBG_INFO (igi, FSM, "%r MFC Reprog Failed",
                               (struct pal_in4_addr *)
                               PTREE_NODE_KEY (hst_igr->igr_owning_pn));

              hst_igr->igr_rexmit_group_lmqc =
                  hst_igif->igif_conf.igifc_rv;

              hst_igr->igr_rexmit_hrt = IGMP_HRT_CHG_TO_INCL;

              ret = igmp_if_hst_send_group_report (hst_igr);

              if (ret < IGMP_ERR_NONE)
                {
                  IGMP_DBG_INFO (igi, FSM, "Grp Report Send Failed(%d)!",
                                 ret);

                  igmp_if_grec_delete (hst_igr);
                }
            }
          else
            {
              minus_tib = ptree_init (IPV4_MAX_BITLEN);

              if (! minus_tib)
                {
                  IGMP_DBG_ERR (igi, FSM, "P-Trie Init Failed %s:%d",
                                __FILE__, __LINE__);

                  ret = IGMP_ERR_OOM;
                  goto EXIT;
                }

              /* Create a new Src-Rec */
              ret = igmp_if_srec_create (hst_igr, &isr);

              if (ret < IGMP_ERR_NONE)
                {
                  IGMP_DBG_ERR (igi, FSM, "Failed to create Src node "
                                "@ %s:%d", __FILE__, __LINE__);

                  ptree_finish (minus_tib);

                  ret = IGMP_ERR_OOM;
                  goto EXIT;
                }

              pn = ptree_node_get (minus_tib,
                                   (u_int8_t *) &isl->isl_saddr [0],
                                   IPV4_MAX_PREFIXLEN);

              if (! pn)
                {
                  IGMP_DBG_ERR (igi, TIB, "Failed to get P-Trie node"
                                " @ %s:%d", __FILE__, __LINE__);

                  /* Delete the Src-Rec created above */
                  igmp_if_srec_delete (isr);

                  ptree_finish (minus_tib);

                  ret = IGMP_ERR_OOM;
                  goto EXIT;
                }

              /* Install the Src-Rec into Minus TIB */
              pn->info = isr;
              isr->isr_owning_pn = pn;

              /* Run-up the Srcs Counter */
              num_srcs += 1;

              ret = igmp_hst_igr_fsm_mfc_reprog (hst_igr, minus_tib);

              if (ret < IGMP_ERR_NONE)
                IGMP_DBG_INFO (igi, FSM, "%r MFC Reprog Failed",
                               (struct pal_in4_addr *)
                               PTREE_NODE_KEY (hst_igr->igr_owning_pn));

              /* Send SLC Record Report (G, BLOCK (Src))  */
              hst_igr->igr_rexmit_group_source_lmqc =
                  hst_igif->igif_conf.igifc_rv;

              hst_igr->igr_rexmit_hrt = IGMP_HRT_BLOCK_OLD_SRCS;

              ret = igmp_if_hst_send_group_source_report
                    (hst_igr, minus_tib);

              /* Store the Minus-TIB for Re-Transmissions */
              if (ret < IGMP_ERR_NONE)
                {
                  IGMP_DBG_INFO (igi, FSM, "Grp-Src Report Send Failed(%d)!",
                                 ret);

                  /* Delete the Src-Rec created above */
                  igmp_if_srec_delete (isr);

                  ptree_finish (minus_tib);
                }
              else
                {
                  if (hst_igr->igr_rexmit_srcs_tib)
                    ptree_finish (hst_igr->igr_rexmit_srcs_tib);

                  hst_igr->igr_rexmit_srcs_tib = minus_tib;
                  hst_igr->igr_rexmit_srcs_tib_count = num_srcs;
                }
            }
        }
      break;

    case IGMP_HFME_INVALID:
      ret = IGMP_ERR_DOOM;
      break;
    }

EXIT:

  IGMP_FN_EXIT (ret);
}

/* IGMP Proxy FSM action function for Exclude-state */
s_int32_t
igmp_hst_igr_fsm_action_exclude (struct igmp_group_rec *hst_igr,
                                 enum igmp_hst_filter_mode_event igr_hfme,
                                 struct igmp_source_list *isl)
{
  struct ptree *intersection_tib;
  u_int16_t num_srcs_to_allow;
  u_int16_t num_srcs_to_block;
  struct ptree *old_state_tib;
  struct igmp_source_rec *isr;
  struct igmp_group_rec *igr;
  struct ptree_node *pn_next;
  struct igmp_instance *igi;
  struct ptree_node *pn_sec;
  struct igmp_if *hst_igif;
  struct ptree *minus_tib;
  struct ptree *allow_tib;
  struct ptree *block_tib;
  bool_t send_lmem_update;
  struct igmp_if *ds_igif;
  struct ptree_node *pn;
  struct listnode *nn;
  u_int16_t num_srcs;
  bool_t send_report;
  bool_t chg_to_incl;
  bool_t grec_del;
  u_int32_t idx;
  s_int32_t ret;

  IGMP_FN_ENTER (Host Act Exclude);

  hst_igif = hst_igr->igr_owning_igif;
  igi = hst_igif->igif_owning_igi;
  send_lmem_update = PAL_FALSE;
  intersection_tib = NULL;
  send_report = PAL_FALSE;
  chg_to_incl = PAL_FALSE;
  num_srcs_to_allow = 0;
  num_srcs_to_block = 0;
  grec_del = PAL_FALSE;
  ret = IGMP_ERR_NONE;
  allow_tib = NULL;
  block_tib = NULL;
  minus_tib = NULL;
  num_srcs = 0;
  isr = NULL;

  switch (igr_hfme)
    {
    case IGMP_HFME_INCL:
      ret = igmp_hst_igr_fsm_preprocess_incl_event
            (hst_igr, &grec_del, &chg_to_incl, PAL_TRUE);

      if (ret < IGMP_ERR_NONE)
        {
          IGMP_DBG_INFO (igi, FSM, "INCL Preproc FAILED(%d)!", ret);

          goto EXIT;
        }

      /* Check if GRec needs to be deleted */
      if (grec_del)
        {
          /* Loose the Sources in TIB-A and TIB-B */
          for (pn = ptree_top (hst_igr->igr_src_a_tib); pn; pn = pn_next)
            {
              pn_next = ptree_next (pn);

              if (! (isr = pn->info))
                continue;

              igmp_if_srec_delete (isr);

              hst_igr->igr_src_a_tib_count -= 1;
            }

          for (pn = ptree_top (hst_igr->igr_src_b_tib); pn; pn = pn_next)
            {
              pn_next = ptree_next (pn);

              if (! (isr = pn->info))
                continue;

              igmp_if_srec_delete (isr);

              hst_igr->igr_src_b_tib_count -= 1;
            }

          ret = igmp_hst_igr_fsm_change_state
                (hst_igr, IGMP_HFMS_INCLUDE);

          if (CHECK_FLAG (hst_igif->igif_sflags, IGMP_IF_SFLAG_SVC_TYPE_L2)
              && CHECK_FLAG (hst_igif->igif_sflags, IGMP_IF_SFLAG_SVC_TYPE_L3))
            send_lmem_update = PAL_TRUE;
        }
      else if (chg_to_incl)
        {
          /* Loose the Sources in TIB-B */
          for (pn = ptree_top (hst_igr->igr_src_b_tib);
               pn; pn = pn_next)
            {
              pn_next = ptree_next (pn);

              if (! (isr = pn->info))
                continue;

              igmp_if_srec_delete (isr);

              hst_igr->igr_src_b_tib_count -= 1;
            }

          ret = igmp_hst_igr_fsm_change_state
                (hst_igr, IGMP_HFMS_INCLUDE);

          if (CHECK_FLAG (hst_igif->igif_sflags, IGMP_IF_SFLAG_SVC_TYPE_L2)
              && CHECK_FLAG (hst_igif->igif_sflags, IGMP_IF_SFLAG_SVC_TYPE_L3))
            send_lmem_update = PAL_TRUE;
        }
      else /* Staying in EXCL, modify TIB-B as needed */
        {
          if (isl
              && isl->isl_num)
            {
              intersection_tib = ptree_init (IPV4_MAX_BITLEN);

              if (! intersection_tib)
                {
                  IGMP_DBG_ERR (igi, TIB, "P-Trie Init Failed %s:%d",
                                __FILE__, __LINE__);

                  ret = IGMP_ERR_OOM;
                  goto EXIT;
                }
            }

          /* Generate (A-B) Src-List into TIB-B */
          for (idx = 0; isl && idx < isl->isl_num; idx++)
            {
              /* Prepare the Src-List for ALLOW (A*B) */
              if ((isr = igmp_if_srec_lookup (hst_igr, PAL_FALSE,
                                              &isl->isl_saddr [idx])))
                {
                  pn = ptree_node_get (intersection_tib,
                                       (u_int8_t *) &isl->isl_saddr [idx],
                                       IPV4_MAX_PREFIXLEN);

                  if (! pn)
                    {
                      IGMP_DBG_ERR (igi, TIB, "Failed to get P-Trie"
                                    " node for %r @ %s:%d",
                                    &isl->isl_saddr [idx],
                                    __FILE__, __LINE__);

                      continue;
                    }

                  /* Switch over the Src-Rec to Xtion TIB */
                  if (isr->isr_owning_pn)
                    isr->isr_owning_pn->info = NULL;
                  pn->info = isr;
                  isr->isr_owning_pn = pn;

                  /* Count Src-Rec out off TIB-B */
                  hst_igr->igr_src_b_tib_count += 1;

                  /* Run-up the Srcs Counter */
                  num_srcs += 1;

                  send_report = PAL_TRUE;

                  if (CHECK_FLAG (hst_igif->igif_sflags,
                                  IGMP_IF_SFLAG_SVC_TYPE_L2)
                      && CHECK_FLAG (hst_igif->igif_sflags,
                                     IGMP_IF_SFLAG_SVC_TYPE_L3))
                    send_lmem_update = PAL_TRUE;
                }
            }

          ret = igmp_hst_igr_fsm_change_state
                (hst_igr, IGMP_HFMS_EXCLUDE);
        }

      if (send_lmem_update)
        {
           ret = igmp_igr_fsm_send_lmem_update (hst_igr);

           if (ret < IGMP_ERR_NONE)
             IGMP_DBG_INFO (igi, FSM, "Lmem-Update failed for %r on %s",
                            PTREE_NODE_KEY (hst_igr->igr_owning_pn),
                            hst_igif->igif_owning_ifp->name);
        }

      /* Grp-Rec Del (==> Send INCL <NULL> Indication) */
      if (grec_del)
        {
          ret = igmp_hst_igr_fsm_mfc_reprog (hst_igr, NULL);

          if (ret < IGMP_ERR_NONE)
            IGMP_DBG_INFO (igi, FSM, "%r MFC Reprog Failed",
                           (struct pal_in4_addr *)
                           PTREE_NODE_KEY (hst_igr->igr_owning_pn));

          hst_igr->igr_rexmit_group_lmqc =
              hst_igif->igif_conf.igifc_rv;

          hst_igr->igr_rexmit_hrt = IGMP_HRT_CHG_TO_INCL;

          ret = igmp_if_hst_send_group_report (hst_igr);

          if (ret < IGMP_ERR_NONE)
            {
              IGMP_DBG_INFO (igi, FSM, "Grp Report Send Failed(%d)!",
                             ret);

              igmp_if_grec_delete (hst_igr);
            }
        }
      /* Send FMC Record in Report (G, TO_INCL (B)) */
      else if (chg_to_incl)
        {
          ret = igmp_hst_igr_fsm_mfc_reprog (hst_igr, NULL);

          if (ret < IGMP_ERR_NONE)
            IGMP_DBG_INFO (igi, FSM, "%r MFC Reprog Failed",
                           (struct pal_in4_addr *)
                           PTREE_NODE_KEY (hst_igr->igr_owning_pn));

          hst_igr->igr_rexmit_group_lmqc =
              hst_igif->igif_conf.igifc_rv;

          hst_igr->igr_rexmit_hrt = IGMP_HRT_CHG_TO_INCL;

          ret = igmp_if_hst_send_group_report (hst_igr);

          if (ret < IGMP_ERR_NONE)
            IGMP_DBG_INFO (igi, FSM, "Grp Report Send Failed(%d)!",
                           ret);
        }
      /* Send SLC Record Report (G, ALLOW (A*B)) */
      else if (send_report)
        {
          ret = igmp_hst_igr_fsm_mfc_reprog
                (hst_igr, intersection_tib);

          if (ret < IGMP_ERR_NONE)
            IGMP_DBG_INFO (igi, FSM, "%r MFC Reprog Failed",
                           (struct pal_in4_addr *)
                           PTREE_NODE_KEY (hst_igr->igr_owning_pn));

          hst_igr->igr_rexmit_group_source_lmqc =
              hst_igif->igif_conf.igifc_rv;

          hst_igr->igr_rexmit_hrt = IGMP_HRT_ALLOW_NEW_SRCS;

          ret = igmp_if_hst_send_group_source_report
                (hst_igr, intersection_tib);

          /* Store the Xtion-TIB for Re-Transmissions */
          if (ret < IGMP_ERR_NONE)
            {
              IGMP_DBG_INFO (igi, FSM, "Grp-Src Report Send Failed(%d)!",
                             ret);

              ptree_finish (intersection_tib);
            }
          else
            {
              if (hst_igr->igr_rexmit_srcs_tib)
                ptree_finish (hst_igr->igr_rexmit_srcs_tib);

              hst_igr->igr_rexmit_srcs_tib = intersection_tib;
              hst_igr->igr_rexmit_srcs_tib_count = num_srcs;
            }
        }
      /* No resultant change */
         else
           {    
              ret = igmp_hst_igr_fsm_mfc_reprog (hst_igr, NULL);
              
              if (ret < IGMP_ERR_NONE)
                 IGMP_DBG_INFO (igi, FSM, "%r MFC Reprog Failed",
                                (struct pal_in4_addr *)
                                  PTREE_NODE_KEY (hst_igr->igr_owning_pn));
                  if (intersection_tib)
                    ptree_finish (intersection_tib);
            }
      break;

    case IGMP_HFME_EXCL:
      old_state_tib = ptree_init (IPV4_MAX_BITLEN);

      if (! old_state_tib)
        {
          IGMP_DBG_ERR (igi, TIB, "P-Trie Init Failed %s:%d",
                        __FILE__, __LINE__);

          ret = IGMP_ERR_OOM;
          goto EXIT;
        }

      allow_tib = ptree_init (IPV4_MAX_BITLEN);

      if (! allow_tib)
        {
          IGMP_DBG_ERR (igi, TIB, "P-Trie Init Failed %s:%d",
                        __FILE__, __LINE__);

          ret = IGMP_ERR_OOM;
          goto EXIT;
        }

      block_tib = ptree_init (IPV4_MAX_BITLEN);

      if (! block_tib)
        {
          IGMP_DBG_ERR (igi, TIB, "P-Trie Init Failed %s:%d",
                        __FILE__, __LINE__);

          ret = IGMP_ERR_OOM;
          goto EXIT;
        }

      intersection_tib = ptree_init (IPV4_MAX_BITLEN);
    
      if (! intersection_tib)
        {
          IGMP_DBG_ERR (igi, TIB, "P-Trie Init Failed %s:%d",
                        __FILE__, __LINE__);

          ret = IGMP_ERR_OOM;
          goto EXIT;
        }

      /* Duplicate the old state to another tib for later use */
      for (pn = ptree_top (hst_igr->igr_src_b_tib); pn; pn = ptree_next (pn))
        {
          if (pn->info == NULL)
            continue;

          ret = igmp_if_srec_create (hst_igr, &isr);

          if (ret < IGMP_ERR_NONE)
            {
              IGMP_DBG_ERR (igi, TIB, "Failed to create Src node"
                            " for %r @ %s:%d", PTREE_NODE_KEY (pn),
                            __FILE__, __LINE__);
              continue;
            }

          pn_sec = ptree_node_get (old_state_tib,
                                   (u_int8_t *) PTREE_NODE_KEY (pn),
                                   IPV4_MAX_PREFIXLEN);

          if (pn_sec == NULL)
            {
              IGMP_DBG_ERR (igi, TIB, "Failed to get P-Trie node"
                            "@ %s:%d",__FILE__, __LINE__);
             
              /* Delete the Src-Rec created above */
              igmp_if_srec_delete (isr);

              continue;
            }
            
          pn_sec->info = isr;;
          isr->isr_owning_pn = pn_sec;
        }

      num_srcs = 0;

      /* Wade through all members to build the new state */
      LIST_LOOP (&hst_igif->igif_hst_dsif_lst, ds_igif, nn)
        {
          if (CHECK_FLAG (ds_igif->igif_sflags, IGMP_IF_SFLAG_SVC_TYPE_L2)
              && CHECK_FLAG (ds_igif->igif_sflags, IGMP_IF_SFLAG_SVC_TYPE_L3))
            igr = igmp_if_hst_grec_lookup (ds_igif,
                                           (struct pal_in4_addr *)
                                           PTREE_NODE_KEY
                                           (hst_igr->igr_owning_pn));
          else
            igr = igmp_if_grec_lookup (ds_igif,
                                       (struct pal_in4_addr *)
                                       PTREE_NODE_KEY
                                       (hst_igr->igr_owning_pn));
          if (igr == NULL)
            continue;

          if (igr->igr_filt_mode_state == IGMP_FMS_EXCLUDE
              && igr->igr_src_b_tib_count)
            {
              num_srcs = 0;

              for (pn = ptree_top (hst_igr->igr_src_b_tib);
                   pn; pn = ptree_next (pn))
                {
                  if (pn->info == NULL)
                    continue;

                  if ((igmp_if_srec_lookup (igr, PAL_FALSE,
                                            (struct pal_in4_addr *)
                                            PTREE_NODE_KEY (pn))) == NULL)
                    continue;

                  pn_sec = ptree_node_lookup (intersection_tib,
                                              PTREE_NODE_KEY (pn),
                                              IPV4_MAX_PREFIXLEN);

                  if (pn_sec != NULL)
                    continue;

                  ret = igmp_if_srec_create (hst_igr, &isr);

                  if (ret < IGMP_ERR_NONE)
                    {
                      IGMP_DBG_ERR (igi, TIB, "Failed to create Src node"
                                    " for %r @ %s:%d", PTREE_NODE_KEY (pn_sec),
                                    __FILE__, __LINE__);
                      continue;
                    }

                  pn_sec = ptree_node_get (intersection_tib,
                                           PTREE_NODE_KEY (pn),
                                           IPV4_MAX_PREFIXLEN);

                  pn_sec->info = isr;

                  isr->isr_owning_pn = pn_sec;
                }

              /* Free all the nodes in HST_IGR B TIB */
              for (pn = ptree_top (hst_igr->igr_src_b_tib); pn; pn = pn_next)
                {
                  pn_next = ptree_next (pn);

                  if (! (isr = pn->info))
                    continue;

                  igmp_if_srec_delete (isr);

                  hst_igr->igr_src_b_tib_count -= 1;
                }
              ptree_node_delete_all (hst_igr->igr_src_b_tib);

              num_srcs = 0;

              /* Duplicate intersection tib to hst_igr B TIB */
              for (pn = ptree_top (intersection_tib); pn;
                   pn = ptree_next (pn))
                {
                  if (pn->info == NULL)
                    continue;

                  ret = igmp_if_srec_create (hst_igr, &isr);

                  if (ret < IGMP_ERR_NONE)
                    {
                      IGMP_DBG_ERR (igi, TIB, "Failed to create Src node"
                                    " for %r @ %s:%d", PTREE_NODE_KEY (pn),
                                    __FILE__, __LINE__);
                      continue;
                    }

                  pn_sec = ptree_node_get (hst_igr->igr_src_b_tib,
                                           (u_int8_t *) PTREE_NODE_KEY (pn),
                                           IPV4_MAX_PREFIXLEN);

                  if (pn_sec == NULL)
                    {
                      IGMP_DBG_ERR (igi, TIB, "Failed to get P-Trie node"
                                    "@ %s:%d",__FILE__, __LINE__);

                      /* Delete the Src-Rec created above */
                      igmp_if_srec_delete (isr);

                      continue;
                    }

                  if (isr->isr_owning_pn)
                    isr->isr_owning_pn->info = NULL;

                  pn_sec->info = isr;;
                  isr->isr_owning_pn = pn_sec;

                  num_srcs += 1;
                }

              /* Free all the nodes in intersection_tib */
              for (pn = ptree_top (intersection_tib); pn; pn = pn_next)
                {
                  pn_next = ptree_next (pn);

                  if (! (isr = pn->info))
                    continue;

                  igmp_if_srec_delete (isr);
                }
              ptree_node_delete_all (intersection_tib);
            }
        }
      /* Free "intersection_tib" since it is not used
         in the rest of this "case" statement
       */
      ptree_finish (intersection_tib);

      hst_igr->igr_src_b_tib_count = num_srcs;

      /* Create the ALLOW TIB (A-B) */
      for (pn = ptree_top (old_state_tib); pn; pn = ptree_next (pn))
        {
          if (pn->info == NULL)
            continue;

          if ((igmp_if_srec_lookup (hst_igr, PAL_FALSE,
                                    (struct pal_in4_addr *)PTREE_NODE_KEY
                                    (pn))) == NULL)
            {
              /* Create a new Src-Rec */
              ret = igmp_if_srec_create (hst_igr, &isr);

              if (ret < IGMP_ERR_NONE)
                {
                  IGMP_DBG_ERR (igi, TIB, "Failed to create Src node"
                                " for %r @ %s:%d", PTREE_NODE_KEY(
                                pn), __FILE__, __LINE__);

                  continue;
                }

              pn_sec = ptree_node_get (allow_tib, 
                                       PTREE_NODE_KEY (pn),
                                       IPV4_MAX_PREFIXLEN);

              if (pn_sec == NULL)
                {
                  IGMP_DBG_ERR (igi, TIB, "Failed to get P-Trie node"
                                "@ %s:%d",__FILE__, __LINE__);

                  igmp_if_srec_delete (isr);

                  continue;
                }

              if (isr->isr_owning_pn)
                isr->isr_owning_pn->info = NULL;

              pn_sec->info = isr;
              isr->isr_owning_pn = pn_sec;

              num_srcs_to_allow += 1;
            } 
        }

      /* Create the BLOCK TIB (B-A) */
      for (pn = ptree_top (hst_igr->igr_src_b_tib); pn; pn = ptree_next (pn))
        {
          if (pn->info == NULL)
            continue;

          if ((ptree_node_lookup (old_state_tib,
                                  PTREE_NODE_KEY (
                                  pn),IPV4_MAX_PREFIXLEN)) == NULL)
            {
              /* Create a new Src-Rec */
              ret = igmp_if_srec_create (hst_igr, &isr);

              if (ret < IGMP_ERR_NONE)
                {
                  IGMP_DBG_ERR (igi, TIB, "Failed to create Src node"
                                " for %r @ %s:%d", (u_int8_t *) PTREE_NODE_KEY(
                                pn), __FILE__, __LINE__);

                  continue;
                }

              pn_sec = ptree_node_get (block_tib,
                                       (u_int8_t *) PTREE_NODE_KEY (pn),
                                       IPV4_MAX_PREFIXLEN);

              if (pn_sec == NULL)
                {
                  IGMP_DBG_ERR (igi, TIB, "Failed to get P-Trie node"
                                "@ %s:%d",__FILE__, __LINE__);

                  igmp_if_srec_delete (isr);

                  continue;
                }

              if (isr->isr_owning_pn)
                isr->isr_owning_pn->info = NULL;

              pn_sec->info = isr;
              isr->isr_owning_pn = pn_sec;

              num_srcs_to_block += 1;
            }
        }

      ret = igmp_hst_igr_fsm_change_state
            (hst_igr, IGMP_HFMS_EXCLUDE);

      ret = igmp_hst_igr_fsm_mfc_reprog (hst_igr, NULL);

      if (ret < IGMP_ERR_NONE)
        IGMP_DBG_INFO (igi, FSM, "%r MFC Reprog Failed",
                       (struct pal_in4_addr *)
                       PTREE_NODE_KEY (hst_igr->igr_owning_pn));

      if (CHECK_FLAG (hst_igif->igif_sflags, IGMP_IF_SFLAG_SVC_TYPE_L2)
          && CHECK_FLAG (hst_igif->igif_sflags, IGMP_IF_SFLAG_SVC_TYPE_L3))
        {

           ret = igmp_igr_fsm_send_lmem_update (hst_igr);

           if (ret < IGMP_ERR_NONE)
             IGMP_DBG_INFO (igi, FSM, "Lmem-Update failed for %r on %s",
                            PTREE_NODE_KEY (hst_igr->igr_owning_pn),
                            hst_igif->igif_owning_ifp->name);
        }

      hst_igr->igr_rexmit_group_source_lmqc =
          hst_igif->igif_conf.igifc_rv;

      if (num_srcs_to_allow)
        {
          /* Send report for ALLOW_NEW_SRCS*/
          hst_igr->igr_rexmit_hrt = IGMP_HRT_ALLOW_NEW_SRCS;

          ret = igmp_if_hst_send_group_source_report
                 (hst_igr, allow_tib);

          /* Store the ALLOW_TIB for Re-Transmissions */
          if (ret < IGMP_ERR_NONE)
            {
              IGMP_DBG_INFO (igi, FSM, "Grp-Src Report Send Failed(%d)!",
                             ret);

              ptree_finish (allow_tib);
           }
         else
           {
             if (hst_igr->igr_rexmit_allow_tib)
               ptree_finish (hst_igr->igr_rexmit_allow_tib);

             hst_igr->igr_rexmit_allow_tib = allow_tib;
             hst_igr->igr_rexmit_srcs_allow_tib_count = num_srcs_to_allow;
           }
        }
      else
        {
          if (allow_tib)
            ptree_finish (allow_tib);
        }

      if (num_srcs_to_block)
        {
          /* Send report for BLOCK_OLD_SRCS */
          hst_igr->igr_rexmit_hrt = IGMP_HRT_BLOCK_OLD_SRCS;

          ret = igmp_if_hst_send_group_source_report
                (hst_igr, block_tib);

          /* Store the BLOCK_TIB for Re-Transmissions */
          if (ret < IGMP_ERR_NONE)
            {
              IGMP_DBG_INFO (igi, FSM, "Grp-Src Report Send Failed(%d)!",
                             ret);

              ptree_finish (block_tib);
            }
          else
            {
              if (hst_igr->igr_rexmit_block_tib)
                ptree_finish (hst_igr->igr_rexmit_block_tib);

              hst_igr->igr_rexmit_block_tib = block_tib;
              hst_igr->igr_rexmit_srcs_block_tib_count = num_srcs_to_block;
            }
         }
       else
         {
           if (block_tib)
             ptree_finish (block_tib);
         }

       /* Free all the nodes in old_state_tib */
       for (pn = ptree_top (old_state_tib); pn; pn = pn_next)
         {
           pn_next = ptree_next (pn);

           if (! (isr = pn->info))
             continue;

           igmp_if_srec_delete (isr);
         }
       ptree_finish (old_state_tib);
       break;

    case IGMP_HFME_MFC_MSG:
      if (isl
          && isl->isl_num)
        {
          isr = igmp_if_srec_lookup (hst_igr, PAL_FALSE,
              &isl->isl_saddr [0]);

          /* Prepare content and programme MFC */
          ret = igmp_hst_igr_fsm_mfc_prog (hst_igr,
              &isl->isl_saddr [0],
              PAL_FALSE,
              isr ? PAL_TRUE : PAL_FALSE);

          if (ret < IGMP_ERR_NONE)
            {
              IGMP_DBG_INFO (igi, FSM, "MFC Prog Failed!");

              goto EXIT;
            }
        }
      break;

    case IGMP_HFME_MANUAL_CLEAR:
      if (! isl)
        {
          /* Release the Source P-Trie 'A' */
          for (pn = ptree_top (hst_igr->igr_src_a_tib); pn; pn = pn_next)
            {
              pn_next = ptree_next (pn);

              if (! (isr = pn->info))
                continue;

              igmp_if_srec_delete (isr);

              hst_igr->igr_src_a_tib_count -= 1;
            }

          /* Release the Source P-Trie 'B' */
          for (pn = ptree_top (hst_igr->igr_src_b_tib); pn; pn = pn_next)
            {
              pn_next = ptree_next (pn);

              if (! (isr = pn->info))
                continue;

              igmp_if_srec_delete (isr);

              hst_igr->igr_src_b_tib_count -= 1;
            }

          ret = igmp_igr_fsm_change_state (hst_igr, IGMP_HFMS_INCLUDE);

          ret = igmp_hst_igr_fsm_mfc_reprog (hst_igr, NULL);

          if (ret < IGMP_ERR_NONE)
            IGMP_DBG_INFO (igi, FSM, "%r MFC Reprog Failed",
                           (struct pal_in4_addr *)
                           PTREE_NODE_KEY (hst_igr->igr_owning_pn));

          hst_igr->igr_rexmit_group_lmqc =
              hst_igif->igif_conf.igifc_rv;

          hst_igr->igr_rexmit_hrt = IGMP_HRT_CHG_TO_INCL;

          ret = igmp_if_hst_send_group_report (hst_igr);

          if (ret < IGMP_ERR_NONE)
            {
              IGMP_DBG_INFO (igi, FSM, "Grp Report Send Failed(%d)!",
                             ret);

              igmp_if_grec_delete (hst_igr);
            }
        }
      else if ((isr = igmp_if_srec_lookup (hst_igr, PAL_FALSE,
                                           &isl->isl_saddr [0])))
        {
          igmp_if_srec_delete (isr);

          hst_igr->igr_src_b_tib_count -= 1;

          ret = igmp_hst_igr_fsm_change_state (hst_igr,
                                               IGMP_HFMS_EXCLUDE);

          minus_tib = ptree_init (IPV4_MAX_BITLEN);

          if (! minus_tib)
            {
              IGMP_DBG_ERR (igi, FSM, "P-Trie Init Failed %s:%d",
                            __FILE__, __LINE__);

              ret = IGMP_ERR_OOM;
              goto EXIT;
            }

          /* Create a new Src-Rec */
          ret = igmp_if_srec_create (hst_igr, &isr);

          if (ret < IGMP_ERR_NONE)
            {
              IGMP_DBG_ERR (igi, FSM, "Failed to create Src node "
                            "for %r @ %s:%d", &isl->isl_saddr [0],
                            __FILE__, __LINE__);

              ret = IGMP_ERR_OOM;
              goto EXIT;
            }

          pn = ptree_node_get (minus_tib,
                               (u_int8_t *) &isl->isl_saddr [0],
                               IPV4_MAX_PREFIXLEN);

          if (! pn)
            {
              IGMP_DBG_ERR (igi, TIB, "Failed to get P-Trie node"
                            " for %r @ %s:%d",
                             &isl->isl_saddr [0],
                             __FILE__, __LINE__);

              /* Delete the Src-Rec created above */
              igmp_if_srec_delete (isr);

              ret = IGMP_ERR_OOM;
              goto EXIT;
            }

          /* Install the Src-Rec into Minus TIB */
          pn->info = isr;
          isr->isr_owning_pn = pn;

          /* Run-up the Srcs Counter */
          num_srcs += 1;

          /* Re-program MFC if necessary */
          ret = igmp_hst_igr_fsm_mfc_reprog (hst_igr, minus_tib);

          if (ret < IGMP_ERR_NONE)
            IGMP_DBG_INFO (igi, FSM, "%r MFC Reprog Failed",
                           (struct pal_in4_addr *)
                           PTREE_NODE_KEY (hst_igr->igr_owning_pn));

          /* Send SLC Record Report (G, ALLOW (Src))  */
          hst_igr->igr_rexmit_group_source_lmqc =
              hst_igif->igif_conf.igifc_rv;

          hst_igr->igr_rexmit_hrt = IGMP_HRT_ALLOW_NEW_SRCS;

          ret = igmp_if_hst_send_group_source_report
                (hst_igr, minus_tib);

          /* Store the Minus-TIB for Re-Transmissions */
          if (ret < IGMP_ERR_NONE)
            {
              IGMP_DBG_INFO (igi, FSM, "Grp-Src Report Send Failed(%d)!",
                             ret);

              /* Delete the Src-Rec created above */
              igmp_if_srec_delete (isr);

              ptree_finish (minus_tib);
            }
          else
            {
              if (hst_igr->igr_rexmit_srcs_tib)
                ptree_finish (hst_igr->igr_rexmit_srcs_tib);

              hst_igr->igr_rexmit_srcs_tib = minus_tib;
              hst_igr->igr_rexmit_srcs_tib_count = num_srcs;
            }
        }

    case IGMP_HFME_INVALID:
      ret = IGMP_ERR_DOOM;
      break;
    }

EXIT:

  IGMP_FN_EXIT (ret);
}

/* IGMP Proxy FSM Get the Block TIB Event*/
s_int32_t
igmp_hst_igr_fsm_get_block_tib (struct igmp_group_rec *hst_igr)
{
  struct igmp_source_rec *isr_sec;
  struct igmp_source_rec *isr;
  struct igmp_group_rec *igr;
  struct ptree_node *pn_next;
  struct ptree_node *pn_sec;
  struct igmp_instance *igi;
  struct igmp_if *hst_igif;
  struct igmp_if *ds_igif;
  struct ptree_node *pn;
  struct listnode *nn;
  s_int32_t ret;

  IGMP_FN_ENTER (Host Get Block TIB Event);

  ret = IGMP_ERR_NONE;
  isr_sec = NULL;
  igr = NULL;
  isr = NULL;
  nn = NULL;

  /* Sanity Checks */
  if (! hst_igr
      || ! (hst_igif = hst_igr->igr_owning_igif)
      || ! (igi = hst_igif->igif_owning_igi))
    {
      ret = IGMP_ERR_MALFORMED_ARG;
      goto EXIT;
    }

  /* Wade through all DS-IFs, Check for sources to be deleted */

  for (pn = ptree_top (hst_igr->igr_src_a_tib);
       pn; pn = pn_next)
    {

      pn_next = ptree_next (pn);

      if (! (isr = pn->info))
        continue;

      LIST_LOOP (&hst_igif->igif_hst_dsif_lst, ds_igif, nn)
        {
          if (CHECK_FLAG (ds_igif->igif_sflags, IGMP_IF_SFLAG_SVC_TYPE_L2)
              && CHECK_FLAG (ds_igif->igif_sflags, IGMP_IF_SFLAG_SVC_TYPE_L3))
            igr = igmp_if_hst_grec_lookup (ds_igif,
                                           (struct pal_in4_addr *)
                                            PTREE_NODE_KEY
                                            (hst_igr->igr_owning_pn));
          else
            igr = igmp_if_grec_lookup (ds_igif,
                                       (struct pal_in4_addr *)
                                        PTREE_NODE_KEY
                                        (hst_igr->igr_owning_pn));

          if (! igr)
            continue;

          if (igmp_if_srec_lookup (igr, PAL_TRUE, (struct pal_in4_addr *)
                                   PTREE_NODE_KEY (pn)) != NULL)
            break;
        }

      if (nn == NULL)
        {
          if (! hst_igr->igr_rexmit_block_tib)
            {
              hst_igr->igr_rexmit_block_tib = ptree_init (IPV4_MAX_BITLEN);

              if (! hst_igr->igr_rexmit_block_tib)
                {
                  IGMP_DBG_ERR (igi, TIB, "P-Trie Init Failed %s:%d",
                        __FILE__, __LINE__);

                  ret = IGMP_ERR_OOM;
                  goto EXIT;
                }
            }

          ret = igmp_if_srec_create (hst_igr, &isr_sec);

          if (ret < IGMP_ERR_NONE)
            {
              IGMP_DBG_ERR (igi, TIB, "Failed to create Src node"
                            " for %r @ %s:%d", PTREE_NODE_KEY (pn),
                            __FILE__, __LINE__);
              continue;
            }

          pn_sec = ptree_node_get (hst_igr->igr_rexmit_block_tib,
                                   (u_int8_t *) PTREE_NODE_KEY (pn),
                                   IPV4_MAX_PREFIXLEN);

          if (pn_sec == NULL)
            {
              IGMP_DBG_ERR (igi, TIB, "Failed to get P-Trie node"
                            "@ %s:%d",__FILE__, __LINE__);

              /* Delete the Src-Rec created above */
              igmp_if_srec_delete (isr_sec);

              continue;
            }

          pn_sec->info = isr_sec;;
          isr_sec->isr_owning_pn = pn_sec;

          hst_igr->igr_rexmit_srcs_block_tib_count += 1;

          igmp_if_srec_delete (isr);
          
          hst_igr->igr_src_a_tib_count -= 1;
        }
    }

EXIT:

  IGMP_FN_EXIT (ret);
}

/* IGMP Proxy FSM Preprocess an INCL event */
s_int32_t
igmp_hst_igr_fsm_preprocess_incl_event (struct igmp_group_rec *hst_igr,
                                        bool_t *grec_del,
                                        bool_t *chg_to_incl,
                                        bool_t agg_incl_srcs)
{
  struct igmp_source_rec *isr;
  struct igmp_group_rec *igr;
  struct ptree_node *pn_next;
  struct igmp_instance *igi;
  struct igmp_if *hst_igif;
  struct igmp_if *ds_igif;
  struct ptree_node *pn;
  struct listnode *nn;
  s_int32_t ret;

  IGMP_FN_ENTER (Host Preproc INCL Event);

  ret = IGMP_ERR_NONE;
  igr = NULL;
  isr = NULL;

  /* Sanity Checks */
  if (! hst_igr
      || ! (hst_igif = hst_igr->igr_owning_igif)
      || ! (igi = hst_igif->igif_owning_igi)
      || ! grec_del
      || ! chg_to_incl)
    {
      ret = IGMP_ERR_MALFORMED_ARG;
      goto EXIT;
    }

  *grec_del = PAL_TRUE;
  *chg_to_incl = PAL_TRUE;

  /* Wade through all DS-IFs, process for Chg2INCL state */
  LIST_LOOP (&hst_igif->igif_hst_dsif_lst, ds_igif, nn)
    {
      if (CHECK_FLAG (ds_igif->igif_sflags, IGMP_IF_SFLAG_SVC_TYPE_L2)
          && CHECK_FLAG (ds_igif->igif_sflags, IGMP_IF_SFLAG_SVC_TYPE_L3))
        igr = igmp_if_hst_grec_lookup (ds_igif,
                                       (struct pal_in4_addr *)
                                       PTREE_NODE_KEY
                                       (hst_igr->igr_owning_pn));
      else
        igr = igmp_if_grec_lookup (ds_igif,
                                   (struct pal_in4_addr *)
                                   PTREE_NODE_KEY
                                   (hst_igr->igr_owning_pn));

      if (! igr)
        continue;

      if (igr->igr_filt_mode_state == IGMP_FMS_EXCLUDE)
        {
          *grec_del = PAL_FALSE;
          *chg_to_incl = PAL_FALSE;

          /* Delete any merged Srcs in TIB-A */
          if (agg_incl_srcs)
            for (pn = ptree_top (hst_igr->igr_src_a_tib);
                 pn; pn = pn_next)
              {
                pn_next = ptree_next (pn);

                if (! (isr = pn->info))
                  continue;

                igmp_if_srec_delete (isr);

                igr->igr_src_a_tib_count -= 1;
              }

          break;
        }

      if (igr->igr_filt_mode_state == IGMP_FMS_INCLUDE
          && igr->igr_src_a_tib_count)
        {
          *grec_del = PAL_FALSE;

          /* Merge INCL Src-List into TIB-A */
          if (agg_incl_srcs)
            for (pn = ptree_top (igr->igr_src_a_tib);
                 pn; pn = ptree_next (pn))
              {
                if (! pn->info)
                  continue;

                if (! (igmp_if_srec_get (hst_igr, PAL_TRUE,
                                         (struct pal_in4_addr *)
                                         PTREE_NODE_KEY (pn))))
                  {
                    IGMP_DBG_ERR (igi, TIB, "Failed to create "
                                  "Src node for %r @ %s:%d",
                                  PTREE_NODE_KEY (pn),
                                  __FILE__, __LINE__);

                    continue;
                  }
              }
        }
    }

EXIT:

  IGMP_FN_EXIT (ret);
}

/* Prepare content and programme MFC */
s_int32_t
igmp_hst_igr_fsm_mfc_prog (struct igmp_group_rec *hst_igr,
                           struct pal_in4_addr *msrc,
                           bool_t mfe_del,
                           bool_t null_olst)
{
  struct igmp_source_rec *isr;
  struct igmp_group_rec *igr;
  struct igmp_instance *igi;
  struct igmp_svc_reg *igsr;
  struct list *mfc_dsif_lst;
  struct igmp_if *hst_igif;
  struct igmp_if *ds_igif;
  struct igmp_if *iigif;
  struct listnode *nn;
  s_int32_t ret;

  IGMP_FN_ENTER (Host MFC Prog);

  ret = IGMP_ERR_NONE;
  mfc_dsif_lst = NULL;
  iigif = NULL;

  if (! hst_igr
      || ! msrc
      || ! (hst_igif = hst_igr->igr_owning_igif)
      || ! (igi = hst_igif->igif_owning_igi))
    {
      ret = IGMP_ERR_MALFORMED_ARG;
      goto EXIT;
    }

  if (! mfe_del)
    {
      if (CHECK_FLAG (hst_igif->igif_sflags,
                      IGMP_IF_SFLAG_MFC_MSG_IN_IF))
        iigif = hst_igif;

      mfc_dsif_lst = list_new ();

      if (! mfc_dsif_lst)
        {
          ret = IGMP_ERR_OOM;
          goto EXIT;
        }

      LIST_LOOP (&hst_igif->igif_hst_dsif_lst, ds_igif, nn)
        {
          if (CHECK_FLAG (ds_igif->igif_sflags,
                          IGMP_IF_SFLAG_MFC_MSG_IN_IF))
            {
              iigif = ds_igif;
              continue;
            }

          if (null_olst
              || iigif == ds_igif
              || ! CHECK_FLAG (ds_igif->igif_sflags,
                               IGMP_IF_SFLAG_QUERIER))
            continue;

          igr = igmp_if_grec_lookup (ds_igif,
                                     (struct pal_in4_addr *)
                                     PTREE_NODE_KEY
                                     (hst_igr->igr_owning_pn));

          if (! igr)
            continue;

          switch (igr->igr_filt_mode_state)
            {
            case IGMP_FMS_INCLUDE:
              isr = igmp_if_srec_lookup (igr, PAL_TRUE, msrc);

              if (! isr)
                continue;

              listnode_add (mfc_dsif_lst, &ds_igif->igif_idx);
              break;

            case IGMP_FMS_EXCLUDE:
              isr = igmp_if_srec_lookup (igr, PAL_FALSE, msrc);

              if (isr)
                continue;

              listnode_add (mfc_dsif_lst, &ds_igif->igif_idx);
              break;

            default:
              ret = IGMP_ERR_DOOM;
              goto EXIT;
            }
        }

      if (! iigif)
        {
          ret = IGMP_ERR_NO_VALID_CONFIG;
          goto EXIT;
        }

      if (iigif != hst_igif)
        listnode_add (mfc_dsif_lst, &hst_igif->igif_idx);

      UNSET_FLAG (iigif->igif_sflags, IGMP_IF_SFLAG_MFC_MSG_IN_IF);
    }
  else
    UNSET_FLAG (hst_igif->igif_sflags, IGMP_IF_SFLAG_MFC_MSG_IN_IF);

  /* Obtain the Svc Registrant(s) for this Interface */
  LIST_LOOP (&igi->igi_svc_reg_lst, igsr, nn)
    {
      if (! igmp_if_svc_reg_match (hst_igif, igsr))
        continue;

      if (igsr->igsr_cback_mfc_prog)
        (void_t) igsr->igsr_cback_mfc_prog (igsr->igsr_su_id,
                                            PAL_FALSE,
                                            mfe_del ?
                                            PAL_FALSE : PAL_TRUE,
                                            (struct pal_in4_addr *)
                                            PTREE_NODE_KEY
                                            (hst_igr->igr_owning_pn),
                                            msrc,
                                            mfe_del ? NULL :
                                            iigif->igif_owning_ifp,
                                            mfe_del ?
                                            IGMP_IF_SVC_USER_ID_DEF :
                                            iigif->igif_su_id,
                                            mfe_del ? NULL :
                                            mfc_dsif_lst);
    }

  /* Remember that the MFC programming was done */
  UNSET_FLAG (hst_igr->igr_sflags, IGMP_IGR_SFLAG_MFC_PROGMED);

  if (! mfe_del)
    SET_FLAG (hst_igr->igr_sflags, IGMP_IGR_SFLAG_MFC_PROGMED);

EXIT:

  if (mfc_dsif_lst)
    list_free (mfc_dsif_lst);

  IGMP_FN_EXIT (ret);
}

/* Prepare content and programme MFC */
s_int32_t
igmp_hst_igr_fsm_mfc_reprog (struct igmp_group_rec *hst_igr,
                             struct ptree *psrcs_tib)
{
  struct igmp_source_rec *isr;
  struct igmp_instance *igi;
  struct igmp_svc_reg *igsr;
  struct igmp_if *hst_igif;
  struct ptree_node *pn;
  struct listnode *nn;
  s_int32_t ret;

  IGMP_FN_ENTER (Host MFC Re-Prog);

  ret = IGMP_ERR_NONE;

  if (! hst_igr
      || ! (hst_igif = hst_igr->igr_owning_igif)
      || ! (igi = hst_igif->igif_owning_igi))
    {
      ret = IGMP_ERR_MALFORMED_ARG;
      goto EXIT;
    }

  if (! CHECK_FLAG (hst_igr->igr_sflags, IGMP_IGR_SFLAG_MFC_PROGMED))
    {
      ret = IGMP_ERR_NONE;
      goto EXIT;
    }

  /* Obtain the Svc Registrant(s) for this Interface */
  LIST_LOOP (&igi->igi_svc_reg_lst, igsr, nn)
    {
      if (! igmp_if_svc_reg_match (hst_igif, igsr)
          || ! igsr->igsr_cback_mfc_prog)
        continue;

      if (psrcs_tib)
        for (pn = ptree_top (psrcs_tib); pn; pn = ptree_next (pn))
          {
            if (! (isr = pn->info))
              continue;

            (void_t) igsr->igsr_cback_mfc_prog (igsr->igsr_su_id,
                                                PAL_TRUE,
                                                PAL_FALSE,
                                                (struct pal_in4_addr *)
                                                PTREE_NODE_KEY
                                                (hst_igr->igr_owning_pn),
                                                (struct pal_in4_addr *)
                                                PTREE_NODE_KEY
                                                (isr->isr_owning_pn),
                                                NULL,
                                                IGMP_IF_SVC_USER_ID_DEF,
                                                NULL);
          }
      else
        (void_t) igsr->igsr_cback_mfc_prog (igsr->igsr_su_id,
                                            PAL_TRUE,
                                            PAL_FALSE,
                                            (struct pal_in4_addr *)
                                            PTREE_NODE_KEY
                                            (hst_igr->igr_owning_pn),
                                            NULL,
                                            NULL,
                                            IGMP_IF_SVC_USER_ID_DEF,
                                            NULL);
    }

EXIT:

  IGMP_FN_EXIT (ret);
}

