/* Copyright (C) 2002-2003  All Rights Reserved. */

#include <igmp_incl.h>

/*
 * IGMP Global CONFIG CLI APIs
 */

/* Set IGMP Limit at Instance Level */
s_int32_t
igmp_limit_set (struct igmp_instance *igi,
                u_int32_t limit,
                u_int8_t *except_alist)
{
  s_int32_t ret;

  IGMP_FN_ENTER (Limit Set);

  ret = IGMP_ERR_NONE;

  if (! igi)
    {
      ret = IGMP_ERR_INVALID_VALUE;
      goto EXIT;
    }

  if (except_alist)
    {
      if (! access_list_reference_validate (except_alist))
        {
          ret = IGMP_ERR_MALFORMED_ARG;
          goto EXIT;
        }

      igi->igi_limit_except_alist =
          XSTRDUP (MTYPE_ACCESS_LIST_STR, except_alist);

      if (! igi->igi_limit_except_alist)
        {
          ret = IGMP_ERR_OOM;
          goto EXIT;
        }
    }
  else if (igi->igi_limit_except_alist)
    {
      XFREE (MTYPE_ACCESS_LIST_STR,
             igi->igi_limit_except_alist);
      igi->igi_limit_except_alist = NULL;
    }

  SET_FLAG (igi->igi_cflags, IGMP_INST_CFLAG_LIMIT_GREC);

  igi->igi_limit = limit;

EXIT:

  IGMP_FN_EXIT (ret);
}

/* Unset IGMP Limit at Instance Level */
s_int32_t
igmp_limit_unset (struct igmp_instance *igi)
{
  s_int32_t ret;

  IGMP_FN_ENTER (Limit UnSet);

  ret = IGMP_ERR_NONE;

  if (! igi)
    {
      ret = IGMP_ERR_INVALID_VALUE;
      goto EXIT;
    }

  UNSET_FLAG (igi->igi_cflags, IGMP_INST_CFLAG_LIMIT_GREC);

  igi->igi_limit = 0;

  if (igi->igi_limit_except_alist)
    {
      XFREE (MTYPE_ACCESS_LIST_STR, igi->igi_limit_except_alist);
      igi->igi_limit_except_alist = NULL;
    }

EXIT:

  IGMP_FN_EXIT (ret);
}

/* Set IGMP Snooping at Instance Level */
s_int32_t
igmp_snooping_set (struct igmp_instance *igi)
{
  s_int32_t ret;

  IGMP_FN_ENTER (Snooping Set);

  ret = IGMP_ERR_NONE;

  if (! igi)
    {
      ret = IGMP_ERR_INVALID_VALUE;
      goto EXIT;
    }

  UNSET_FLAG (igi->igi_cflags, IGMP_INST_CFLAG_SNOOP_DISABLED);

  /* Enable Snooping on all L2 interfaces */
  avl_traverse (igi->igi_if_tree,
                igmp_if_snp_avl_traverse_update,
                (void_t *) IGMP_IF_UPDATE_CONFIG_ENABLE);

EXIT:

  IGMP_FN_EXIT (ret);
}

/* Unset IGMP Snooping at Instance Level */
s_int32_t
igmp_snooping_unset (struct igmp_instance *igi)
{
  s_int32_t ret;

  IGMP_FN_ENTER (Snooping UnSet);

  ret = IGMP_ERR_NONE;

  if (! igi)
    {
      ret = IGMP_ERR_INVALID_VALUE;
      goto EXIT;
    }

  SET_FLAG (igi->igi_cflags, IGMP_INST_CFLAG_SNOOP_DISABLED);

  /* Disable Snooping on all L2 interfaces */
  avl_traverse (igi->igi_if_tree,
                igmp_if_snp_avl_traverse_update,
                (void_t *) IGMP_IF_UPDATE_CONFIG_DISABLE);

EXIT:

  IGMP_FN_EXIT (ret);
}

/* Set IGMP SSM-Map Enable at Instance Level */
s_int32_t
igmp_ssm_map_enable_set (struct igmp_instance *igi)
{
  s_int32_t ret;

  IGMP_FN_ENTER (SSM-Map Enable Set);

  ret = IGMP_ERR_NONE;

  if (! igi)
    {
      ret = IGMP_ERR_INVALID_VALUE;
      goto EXIT;
    }

  /* Clear all IGMP L-Mem if status is to change */
  if (CHECK_FLAG (igi->igi_cflags, IGMP_INST_CFLAG_SSM_MAP_DISABLED))
    igmp_clear (igi, NULL, NULL, NULL);

  UNSET_FLAG (igi->igi_cflags, IGMP_INST_CFLAG_SSM_MAP_DISABLED);

EXIT:

  IGMP_FN_EXIT (ret);
}

/* Unset IGMP SSM-Map Enable at Instance Level */
s_int32_t
igmp_ssm_map_enable_unset (struct igmp_instance *igi)
{
  s_int32_t ret;

  IGMP_FN_ENTER (SSM-Map Enable UnSet);

  ret = IGMP_ERR_NONE;

  if (! igi)
    {
      ret = IGMP_ERR_INVALID_VALUE;
      goto EXIT;
    }

  /* Clear all IGMP L-Mem if status is to change */
  if (! CHECK_FLAG (igi->igi_cflags, IGMP_INST_CFLAG_SSM_MAP_DISABLED))
    igmp_clear (igi, NULL, NULL, NULL);

  SET_FLAG (igi->igi_cflags, IGMP_INST_CFLAG_SSM_MAP_DISABLED);

EXIT:

  IGMP_FN_EXIT (ret);
}

/* Set IGMP SSM-Map Static at Instance Level */
s_int32_t
igmp_ssm_map_static_set (struct igmp_instance *igi,
                         u_int8_t *alist,
                         u_int8_t *msrc_arg)
{
  struct igmp_ssm_map_static *igssms_tmp;
  struct igmp_ssm_map_static *igssms;
  struct pal_in4_addr msrc;
  struct listnode *nn;
  s_int32_t ret;

  IGMP_FN_ENTER (SSM-Map Static Set);

  ret = IGMP_ERR_NONE;
  igssms = NULL;

  if (! igi || ! alist || ! msrc_arg)
    {
      ret = IGMP_ERR_INVALID_VALUE;
      goto EXIT;
    }

  if (! access_list_reference_validate (alist))
    {
      ret = IGMP_ERR_MALFORMED_ARG;
      goto EXIT;
    }

  if (! pal_inet_pton (AF_INET, msrc_arg, &msrc))
    {
      ret = IGMP_ERR_MALFORMED_ARG;
      goto EXIT;
    }

  /* Search for existing entries */
  LIST_LOOP (&igi->igi_ssm_map_static_lst, igssms_tmp, nn)
    if (igssms_tmp->igssms_grp_alist
        && ! pal_strcmp (igssms_tmp->igssms_grp_alist, alist)
        && IPV4_ADDR_SAME (&igssms_tmp->igssms_msrc, &msrc))
      {
        igssms = igssms_tmp;
        break;
      }

  if (igssms)
    {
      ret = IGMP_ERR_NONE;
      goto EXIT;
    }

  igssms = XCALLOC (MTYPE_IGMP_SSM_MAP_STATIC,
                    sizeof (struct igmp_ssm_map_static));

  if (! igssms)
    {
      ret = IGMP_ERR_OOM;
      goto EXIT;
    }

  igssms->igssms_grp_alist = XSTRDUP (MTYPE_ACCESS_LIST_STR, alist);

  if (! igssms->igssms_grp_alist)
    {
      XFREE (MTYPE_IGMP_SSM_MAP_STATIC, igssms);

      ret = IGMP_ERR_OOM;
      goto EXIT;
    }

  igssms->igssms_msrc = msrc;

  listnode_add (&igi->igi_ssm_map_static_lst, igssms);

  SET_FLAG (igi->igi_cflags, IGMP_INST_CFLAG_SSM_MAP_STATIC);

EXIT:

  IGMP_FN_EXIT (ret);
}

/* Unset IGMP SSM-Map Static at Instance Level */
s_int32_t
igmp_ssm_map_static_unset (struct igmp_instance *igi,
                           u_int8_t *alist,
                           u_int8_t *msrc_arg)
{
  struct igmp_ssm_map_static *igssms_tmp;
  struct igmp_ssm_map_static *igssms;
  struct pal_in4_addr msrc;
  struct listnode *nn;
  s_int32_t ret;

  IGMP_FN_ENTER (SSM-Map Static UnSet);

  ret = IGMP_ERR_NONE;
  igssms = NULL;

  if (! igi || ! alist || ! msrc_arg)
    {
      ret = IGMP_ERR_INVALID_VALUE;
      goto EXIT;
    }

  if (! access_list_reference_validate (alist))
    {
      ret = IGMP_ERR_MALFORMED_ARG;
      goto EXIT;
    }

  if (! pal_inet_pton (AF_INET, msrc_arg, &msrc))
    {
      ret = IGMP_ERR_MALFORMED_ARG;
      goto EXIT;
    }

  /* Search for existing entries */
  LIST_LOOP (&igi->igi_ssm_map_static_lst, igssms_tmp, nn)
    if (igssms_tmp->igssms_grp_alist
        && ! pal_strcmp (igssms_tmp->igssms_grp_alist, alist)
        && IPV4_ADDR_SAME (&igssms_tmp->igssms_msrc, &msrc))
      {
        igssms = igssms_tmp;
        break;
      }

  if (! igssms)
    {
      ret = IGMP_ERR_NO_SUCH_VALUE;
      goto EXIT;
    }

  listnode_delete (&igi->igi_ssm_map_static_lst, igssms);

  if (igssms->igssms_grp_alist)
    XFREE (MTYPE_ACCESS_LIST_STR, igssms->igssms_grp_alist);

  XFREE (MTYPE_IGMP_SSM_MAP_STATIC, igssms);

  if (! LISTCOUNT (&igi->igi_ssm_map_static_lst))
    UNSET_FLAG (igi->igi_cflags, IGMP_INST_CFLAG_SSM_MAP_STATIC);

EXIT:

  IGMP_FN_EXIT (ret);
}

/*
 * IGMP IF CONFIG CLI APIs
 */

/* Set IGMP IF (Enable IGMP on Interface) */
s_int32_t
igmp_if_set (struct igmp_instance *igi,
             struct interface *ifp)
{
  struct igmp_if_idx igifidx;
  struct igmp_if *igif;
  s_int32_t ret;

  IGMP_FN_ENTER (IF Set);

  ret = IGMP_ERR_NONE;

  if (! igi || ! ifp)
    {
      ret = IGMP_ERR_INVALID_VALUE;
      goto EXIT;
    }

  if (INTF_TYPE_L2 (ifp))
    {
      ret = IGMP_ERR_L2_PHYSICAL_IF;
      goto EXIT;
    }

  igifidx.igifidx_ifp = ifp;
  igifidx.igifidx_sid = igmp_if_svc_reg_su_id (igi, ifp);

  if (if_is_vlan (ifp))
    igif = igmp_if_lookup (igi, &igifidx);
  else
    igif = igmp_if_get (igi, &igifidx);

  if (! igif)
    {
      ret = IGMP_ERR_NO_SUCH_IFF;
      goto EXIT;
    }

  SET_FLAG (igif->igif_conf.igifc_cflags,
            IGMP_IF_CFLAG_CONFIG_ENABLED);

  if (if_is_vlan (ifp))
    ret = igmp_if_update (igif, NULL, IGMP_IF_UPDATE_CONFIG_RESTART);
  else
    ret = igmp_if_update (igif, NULL, IGMP_IF_UPDATE_CONFIG_ENABLE);

EXIT:

  IGMP_FN_EXIT (ret);
}

/* UnSet IGMP IF (Disable IGMP on Interface) */
s_int32_t
igmp_if_unset (struct igmp_instance *igi,
               struct interface *ifp)
{
  struct igmp_if_idx igifidx;
  struct igmp_if *igif;
  s_int32_t ret;

  IGMP_FN_ENTER (IF UnSet);

  ret = IGMP_ERR_NONE;

  if (! igi || ! ifp)
    {
      ret = IGMP_ERR_INVALID_VALUE;
      goto EXIT;
    }

  if (INTF_TYPE_L2 (ifp))
    {
      ret = IGMP_ERR_L2_PHYSICAL_IF;
      goto EXIT;
    }

  /* Unset ALL configurable IF variables to default */
  ret = igmp_offlink_if_unset (igi, ifp);
  
  if (ret < IGMP_ERR_NONE)
    IGMP_DBG_WARN (igi, EVENTS, "Offlink Unset Failed on %s",
                   ifp->name);
	
  ret = igmp_if_access_list_unset (igi, ifp);

  if (ret < IGMP_ERR_NONE)
    IGMP_DBG_WARN (igi, EVENTS, "Access Grp Unset Failed on %s",
                   ifp->name);

  ret = igmp_if_immediate_leave_unset (igi, ifp);

  if (ret < IGMP_ERR_NONE)
    IGMP_DBG_WARN (igi, EVENTS, "Imm Leave Unset Failed on %s",
                   ifp->name);

  ret = igmp_if_lmqc_unset (igi, ifp);

  if (ret < IGMP_ERR_NONE)
    IGMP_DBG_WARN (igi, EVENTS, "LMQC Unset Failed on %s",
                   ifp->name);

  ret = igmp_if_lmqi_unset (igi, ifp);

  if (ret < IGMP_ERR_NONE)
    IGMP_DBG_WARN (igi, EVENTS, "LMQI Unset Failed on %s",
                   ifp->name);

  ret = igmp_if_limit_unset (igi, ifp);

  if (ret < IGMP_ERR_NONE)
    IGMP_DBG_WARN (igi, EVENTS, "Limit Unset Failed on %s",
                   ifp->name);

  ret = igmp_if_mroute_pxy_unset (igi, ifp);

  if (ret < IGMP_ERR_NONE)
    IGMP_DBG_WARN (igi, EVENTS, "Mroute-proxy Unset Failed on %s",
                   ifp->name);

  ret = igmp_if_pxy_service_unset (igi, ifp);

  if (ret < IGMP_ERR_NONE)
    IGMP_DBG_WARN (igi, EVENTS, "Proxy-service Unset Failed on %s",
                   ifp->name);

  ret = igmp_if_querier_timeout_unset (igi, ifp);

  if (ret < IGMP_ERR_NONE)
    IGMP_DBG_WARN (igi, EVENTS, "Querier Timeout Unset Failed on %s",
                   ifp->name);

  ret = igmp_if_query_interval_unset (igi, ifp);

  if (ret < IGMP_ERR_NONE)
    IGMP_DBG_WARN (igi, EVENTS, "Query Int Unset Failed on %s",
                   ifp->name);

  ret = igmp_if_startup_query_interval_unset (igi, ifp);

  if (ret < IGMP_ERR_NONE)
    IGMP_DBG_WARN (igi, EVENTS, "Startup Query Int Unset Failed on %s",
                   ifp->name);
  
  ret = igmp_if_startup_query_count_unset (igi, ifp);

  if (ret < IGMP_ERR_NONE)
    IGMP_DBG_WARN (igi, EVENTS, "Startup Query Count Unset Failed on %s",
                   ifp->name);

  ret = igmp_if_query_response_interval_unset (igi, ifp);

  if (ret < IGMP_ERR_NONE)
    IGMP_DBG_WARN (igi, EVENTS, "Query Resp Int Unset Failed on %s",
                   ifp->name);

  ret = igmp_if_robustness_var_unset (igi, ifp);

  if (ret < IGMP_ERR_NONE)
    IGMP_DBG_WARN (igi, EVENTS, "Rob Var Unset Failed on %s",
                   ifp->name);

  ret = igmp_if_ra_unset (igi, ifp);

  if (ret < IGMP_ERR_NONE)
    IGMP_DBG_WARN (igi, EVENTS, "RA Unset Failed on %s",
                   ifp->name);

  ret = igmp_if_version_unset (igi, ifp);

  if (ret < IGMP_ERR_NONE)
    IGMP_DBG_WARN (igi, EVENTS, "Version Unset Failed on %s",
                   ifp->name);

  igifidx.igifidx_ifp = ifp;
  igifidx.igifidx_sid = igmp_if_svc_reg_su_id (igi, ifp);

  igif = igmp_if_lookup (igi, &igifidx);

  if (! igif)
    {
      ret = IGMP_ERR_NO_SUCH_IFF;
      goto EXIT;
    }

  UNSET_FLAG (igif->igif_conf.igifc_cflags,
              IGMP_IF_CFLAG_CONFIG_ENABLED);

  /* Update/ Restart IGMP IF */
  if (if_is_vlan (ifp)
      && igmp_if_snp_start (igif))
    ret = igmp_if_update (igif, NULL, IGMP_IF_UPDATE_CONFIG_RESTART);
  else
    ret = igmp_if_update (igif, NULL, IGMP_IF_UPDATE_CONFIG_DISABLE);

EXIT:

  IGMP_FN_EXIT (ret);
}

s_int32_t
igmp_offlink_if_set (struct igmp_instance *igi,
             struct interface *ifp)
{
  struct igmp_if_idx igifidx;
  struct igmp_if *igif;
  s_int32_t ret;

  IGMP_FN_ENTER (IF Offlink Set);

  ret = IGMP_ERR_NONE;

  if (! igi || ! ifp)
    {
      ret = IGMP_ERR_INVALID_VALUE;
      goto EXIT;
    }

  if (INTF_TYPE_L2 (ifp))
    {
      ret = IGMP_ERR_L2_PHYSICAL_IF;
      goto EXIT;
    }

  igifidx.igifidx_ifp = ifp;
  igifidx.igifidx_sid = igmp_if_svc_reg_su_id (igi, ifp);

  if (if_is_vlan (ifp))
    igif = igmp_if_lookup (igi, &igifidx);
  else
    igif = igmp_if_get (igi, &igifidx);

  if (! igif)
    {
      ret = IGMP_ERR_NO_SUCH_IFF;
      goto EXIT;
    }

  SET_FLAG (igif->igif_conf.igifc_cflags,
            IGMP_OFFLINK_IF_CFLAG_CONFIG_ENABLED);

  if (if_is_vlan (ifp))
    ret = igmp_if_update (igif, NULL, IGMP_IF_UPDATE_CONFIG_RESTART);
  else
    ret = igmp_if_update (igif, NULL, IGMP_IF_UPDATE_CONFIG_ENABLE);

  IGMP_DBG_INFO (igi, EVENTS, "offlink igmp enable on %s",
                 igif->igif_owning_ifp->name);

EXIT:

  IGMP_FN_EXIT (ret);
}

s_int32_t
igmp_offlink_if_unset (struct igmp_instance *igi,
             struct interface *ifp)
{ 
  struct igmp_if_idx igifidx;
  struct igmp_if *igif;
  s_int32_t ret;

  IGMP_FN_ENTER (IF Offlink UnSet);

  ret = IGMP_ERR_NONE;

  if (! igi || ! ifp)
    {
      ret = IGMP_ERR_INVALID_VALUE;
      goto EXIT;
    }

  if (INTF_TYPE_L2 (ifp))
    {
      ret = IGMP_ERR_L2_PHYSICAL_IF;
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

  UNSET_FLAG (igif->igif_conf.igifc_cflags,
              IGMP_OFFLINK_IF_CFLAG_CONFIG_ENABLED);

  /* Update/ Restart IGMP IF */
  if (if_is_vlan (ifp)
      && igmp_if_snp_start (igif))
    ret = igmp_if_update (igif, NULL, IGMP_IF_UPDATE_CONFIG_RESTART);
  else
    ret = igmp_if_update (igif, NULL, IGMP_IF_UPDATE_CONFIG_DISABLE);

  IGMP_DBG_INFO (igi, EVENTS, "offlink igmp disable on %s",
                 igif->igif_owning_ifp->name);
  
EXIT:

  IGMP_FN_EXIT (ret);
}

/* Set IGMP Access-List */
s_int32_t
igmp_if_access_list_set (struct igmp_instance *igi,
                         struct interface *ifp,
                         u_int8_t *alist)
{
  struct igmp_if_idx igifidx;
  struct igmp_if *ds_igif;
  struct igmp_if *igif;
  struct listnode *nn;
  s_int32_t ret;

  IGMP_FN_ENTER (IF Access-List Set);

  ret = IGMP_ERR_NONE;

  if (! igi || ! ifp || ! alist)
    {
      ret = IGMP_ERR_INVALID_VALUE;
      goto EXIT;
    }

  if (INTF_TYPE_L2 (ifp))
    {
      ret = IGMP_ERR_L2_PHYSICAL_IF;
      goto EXIT;
    }

  igifidx.igifidx_ifp = ifp;
  igifidx.igifidx_sid = igmp_if_svc_reg_su_id (igi, ifp);

  if (if_is_vlan (ifp))
    igif = igmp_if_lookup (igi, &igifidx);
  else
    igif = igmp_if_get (igi, &igifidx);

  if (! igif)
    {
      ret = IGMP_ERR_NO_SUCH_IFF;
      goto EXIT;
    }

  if (! access_list_reference_validate (alist))
    {
      ret = IGMP_ERR_INVALID_VALUE;
      goto EXIT;
    }

  SET_FLAG (igif->igif_conf.igifc_cflags,
            IGMP_IF_CFLAG_ACCESS_LIST);

  if (igif->igif_conf.igifc_access_list)
    {
      XFREE (MTYPE_ACCESS_LIST_STR,
             igif->igif_conf.igifc_access_list);
      igif->igif_conf.igifc_access_list = NULL;
    }

  igif->igif_conf.igifc_access_list =
      XSTRDUP (MTYPE_ACCESS_LIST_STR, alist);

  if (! igif->igif_conf.igifc_access_list)
    {
      ret = IGMP_ERR_OOM;
      goto EXIT;
    }

  ret = igmp_if_access_list_update (igif);

  if (ret < IGMP_ERR_NONE)
    goto EXIT;

  if (if_is_vlan (ifp))
    {
      (void_t) igmp_if_snp_inherit_config (igif);

      LIST_LOOP (&igif->igif_hst_dsif_lst, ds_igif, nn)
        {
          (void_t) igmp_if_access_list_update (ds_igif);
        }
     }

  ret = igmp_if_update (igif, NULL, IGMP_IF_UPDATE_CONFIG_ENABLE);

EXIT:

  IGMP_FN_EXIT (ret);
}

/* Unset IGMP Access-List */
s_int32_t
igmp_if_access_list_unset (struct igmp_instance *igi,
                           struct interface *ifp)
{
  struct igmp_if_idx igifidx;
  struct igmp_if *ds_igif;
  struct igmp_if *igif;
  struct listnode *nn;
  s_int32_t ret;

  IGMP_FN_ENTER (IF Access-List UnSet);

  ret = IGMP_ERR_NONE;

  if (! igi || ! ifp)
    {
      ret = IGMP_ERR_INVALID_VALUE;
      goto EXIT;
    }

  if (INTF_TYPE_L2 (ifp))
    {
      ret = IGMP_ERR_L2_PHYSICAL_IF;
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

  UNSET_FLAG (igif->igif_conf.igifc_cflags,
              IGMP_IF_CFLAG_ACCESS_LIST);

  if (igif->igif_conf.igifc_access_list)
    {
      XFREE (MTYPE_ACCESS_LIST_STR,
             igif->igif_conf.igifc_access_list);
      igif->igif_conf.igifc_access_list = NULL;
    }

  ret = igmp_if_access_list_update (igif);

  if (ret < IGMP_ERR_NONE)
    goto EXIT;

  if (if_is_vlan (ifp))
    {
      (void_t) igmp_if_snp_inherit_config (igif);

      LIST_LOOP (&igif->igif_hst_dsif_lst, ds_igif, nn)
        {
          (void_t) igmp_if_access_list_update (ds_igif);
        }
    }

  ret = igmp_if_update (igif, NULL, IGMP_IF_UPDATE_CONFIG_DISABLE);

EXIT:

  IGMP_FN_EXIT (ret);
}

/* Set IGMP Immediate-Leave Access-List */
s_int32_t
igmp_if_immediate_leave_set (struct igmp_instance *igi,
                             struct interface *ifp,
                             u_int8_t *alist)
{
  struct igmp_if_idx igifidx;
  struct igmp_if *igif;
  s_int32_t ret;

  IGMP_FN_ENTER (IF Imm Leave Set);

  ret = IGMP_ERR_NONE;

  if (! igi || ! ifp || ! alist)
    {
      ret = IGMP_ERR_INVALID_VALUE;
      goto EXIT;
    }

  if (INTF_TYPE_L2 (ifp))
    {
      ret = IGMP_ERR_L2_PHYSICAL_IF;
      goto EXIT;
    }

  igifidx.igifidx_ifp = ifp;
  igifidx.igifidx_sid = igmp_if_svc_reg_su_id (igi, ifp);

  if (if_is_vlan (ifp))
    igif = igmp_if_lookup (igi, &igifidx);
  else
    igif = igmp_if_get (igi, &igifidx);

  if (! igif)
    {
      ret = IGMP_ERR_NO_SUCH_IFF;
      goto EXIT;
    }

  if (! access_list_reference_validate (alist))
    {
      ret = IGMP_ERR_INVALID_VALUE;
      goto EXIT;
    }

  SET_FLAG (igif->igif_conf.igifc_cflags,
            IGMP_IF_CFLAG_IMMEDIATE_LEAVE);

  if (igif->igif_conf.igifc_immediate_leave)
    {
      XFREE (MTYPE_ACCESS_LIST_STR,
             igif->igif_conf.igifc_immediate_leave);
      igif->igif_conf.igifc_immediate_leave = NULL;
    }

  igif->igif_conf.igifc_immediate_leave =
      XSTRDUP (MTYPE_ACCESS_LIST_STR, alist);

  if (if_is_vlan (ifp))
    (void_t) igmp_if_snp_inherit_config (igif);

  ret = igmp_if_update (igif, NULL, IGMP_IF_UPDATE_CONFIG_ENABLE);

EXIT:

  IGMP_FN_EXIT (ret);
}

/* Unset IGMP Immediate-Leave Access-List */
s_int32_t
igmp_if_immediate_leave_unset (struct igmp_instance *igi,
                               struct interface *ifp)
{
  struct igmp_if_idx igifidx;
  struct igmp_if *igif;
  s_int32_t ret;

  IGMP_FN_ENTER (IF Imm Leave Unset);

  ret = IGMP_ERR_NONE;

  if (! igi || ! ifp)
    {
      ret = IGMP_ERR_INVALID_VALUE;
      goto EXIT;
    }

  if (INTF_TYPE_L2 (ifp))
    {
      ret = IGMP_ERR_L2_PHYSICAL_IF;
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

  UNSET_FLAG (igif->igif_conf.igifc_cflags,
              IGMP_IF_CFLAG_IMMEDIATE_LEAVE);

  if (igif->igif_conf.igifc_immediate_leave)
    {
      XFREE (MTYPE_ACCESS_LIST_STR,
             igif->igif_conf.igifc_immediate_leave);
      igif->igif_conf.igifc_immediate_leave = NULL;
    }

  if (if_is_vlan (ifp))
    (void_t) igmp_if_snp_inherit_config (igif);

  ret = igmp_if_update (igif, NULL, IGMP_IF_UPDATE_CONFIG_DISABLE);

EXIT:

  IGMP_FN_EXIT (ret);
}

/* Set the Last Member Query Count value */
s_int32_t
igmp_if_lmqc_set (struct igmp_instance *igi,
                  struct interface *ifp,
                  u_int32_t lmqc)
{
  struct igmp_if_idx igifidx;
  struct igmp_if *igif;
  s_int32_t ret;

  IGMP_FN_ENTER (IF LMQC Set);

  ret = IGMP_ERR_NONE;
  igif = NULL;

  if (! igi || ! ifp)
    {
      ret = IGMP_ERR_INVALID_VALUE;
      goto EXIT;
    }

  if (INTF_TYPE_L2 (ifp))
    {
      ret = IGMP_ERR_L2_PHYSICAL_IF;
      goto EXIT;
    }

  igifidx.igifidx_ifp = ifp;
  igifidx.igifidx_sid = igmp_if_svc_reg_su_id (igi, ifp);

  if (if_is_vlan (ifp))
    igif = igmp_if_lookup (igi, &igifidx);
  else
    igif = igmp_if_get (igi, &igifidx);

  if (! igif)
    {
      ret = IGMP_ERR_NO_SUCH_IFF;
      goto EXIT;
    }

  SET_FLAG (igif->igif_conf.igifc_cflags,
            IGMP_IF_CFLAG_LAST_MEMBER_QUERY_COUNT);

  igif->igif_conf.igifc_lmqc = lmqc;

  if (if_is_vlan (ifp))
    (void_t) igmp_if_snp_inherit_config (igif);

  ret = igmp_if_update (igif, NULL, IGMP_IF_UPDATE_CONFIG_ENABLE);

EXIT:

  IGMP_FN_EXIT (ret);
}

/* Unset the Last Member Query Count value (to Default) */
s_int32_t
igmp_if_lmqc_unset (struct igmp_instance *igi,
                    struct interface *ifp)
{
  struct igmp_if_idx igifidx;
  struct igmp_if *igif;
  s_int32_t ret;

  IGMP_FN_ENTER (IF LMQC Unset);

  ret = IGMP_ERR_NONE;
  igif = NULL;

  if (! igi || ! ifp)
    {
      ret = IGMP_ERR_INVALID_VALUE;
      goto EXIT;
    }

  if (INTF_TYPE_L2 (ifp))
    {
      ret = IGMP_ERR_L2_PHYSICAL_IF;
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

  UNSET_FLAG (igif->igif_conf.igifc_cflags,
              IGMP_IF_CFLAG_LAST_MEMBER_QUERY_COUNT);

  if (! CHECK_FLAG (igif->igif_conf.igifc_cflags,
                    IGMP_IF_CFLAG_ROBUSTNESS_VAR))
    igif->igif_conf.igifc_lmqc = IGMP_LAST_MEMBER_QUERY_COUNT_DEF;
  else
    igif->igif_conf.igifc_lmqc = igif->igif_conf.igifc_rv;

  if (if_is_vlan (ifp))
    (void_t) igmp_if_snp_inherit_config (igif);

  ret = igmp_if_update (igif, NULL, IGMP_IF_UPDATE_CONFIG_DISABLE);

EXIT:

  IGMP_FN_EXIT (ret);
}

/* Set the Last Member Query Interval value */
s_int32_t
igmp_if_lmqi_set (struct igmp_instance *igi,
                  struct interface *ifp,
                  u_int32_t lmqi)
{
  struct igmp_if_idx igifidx;
  struct igmp_if *igif;
  s_int32_t ret;

  IGMP_FN_ENTER (IF LMQI Set);

  ret = IGMP_ERR_NONE;
  igif = NULL;

  if (! igi || ! ifp)
    {
      ret = IGMP_ERR_INVALID_VALUE;
      goto EXIT;
    }

  if (INTF_TYPE_L2 (ifp))
    {
      ret = IGMP_ERR_L2_PHYSICAL_IF;
      goto EXIT;
    }

  igifidx.igifidx_ifp = ifp;
  igifidx.igifidx_sid = igmp_if_svc_reg_su_id (igi, ifp);

  if (if_is_vlan (ifp))
    igif = igmp_if_lookup (igi, &igifidx);
  else
    igif = igmp_if_get (igi, &igifidx);

  if (! igif)
    {
      ret = IGMP_ERR_NO_SUCH_IFF;
      goto EXIT;
    }

  SET_FLAG (igif->igif_conf.igifc_cflags,
            IGMP_IF_CFLAG_LAST_MEMBER_QUERY_INTERVAL);

  igif->igif_conf.igifc_lmqi = lmqi;

  if (if_is_vlan (ifp))
    (void_t) igmp_if_snp_inherit_config (igif);

  ret = igmp_if_update (igif, NULL, IGMP_IF_UPDATE_CONFIG_ENABLE);

EXIT:

  IGMP_FN_EXIT (ret);
}

/* Unset the Last Member Query Interval value (to Default) */
s_int32_t
igmp_if_lmqi_unset (struct igmp_instance *igi,
                    struct interface *ifp)
{
  struct igmp_if_idx igifidx;
  struct igmp_if *igif;
  s_int32_t ret;

  IGMP_FN_ENTER (IF LMQI Unset);

  ret = IGMP_ERR_NONE;
  igif = NULL;

  if (! igi || ! ifp)
    {
      ret = IGMP_ERR_INVALID_VALUE;
      goto EXIT;
    }

  if (INTF_TYPE_L2 (ifp))
    {
      ret = IGMP_ERR_L2_PHYSICAL_IF;
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

  UNSET_FLAG (igif->igif_conf.igifc_cflags,
              IGMP_IF_CFLAG_LAST_MEMBER_QUERY_INTERVAL);

  igif->igif_conf.igifc_lmqi = IGMP_LAST_MEMBER_QUERY_INTERVAL_DEF;

  if (if_is_vlan (ifp))
    (void_t) igmp_if_snp_inherit_config (igif);

  ret = igmp_if_update (igif, NULL, IGMP_IF_UPDATE_CONFIG_DISABLE);

EXIT:

  IGMP_FN_EXIT (ret);
}

/* Set the Interface Group-Membership Limit value */
s_int32_t
igmp_if_limit_set (struct igmp_instance *igi,
                   struct interface *ifp,
                   u_int32_t limit,
                   u_int8_t *except_alist)
{
  struct igmp_if_idx igifidx;
  struct igmp_if *igif;
  s_int32_t ret;

  IGMP_FN_ENTER (IF Limit Set);

  ret = IGMP_ERR_NONE;
  igif = NULL;

  if (! igi || ! ifp)
    {
      ret = IGMP_ERR_INVALID_VALUE;
      goto EXIT;
    }

  if (INTF_TYPE_L2 (ifp))
    {
      ret = IGMP_ERR_L2_PHYSICAL_IF;
      goto EXIT;
    }

  igifidx.igifidx_ifp = ifp;
  igifidx.igifidx_sid = igmp_if_svc_reg_su_id (igi, ifp);

  if (if_is_vlan (ifp))
    igif = igmp_if_lookup (igi, &igifidx);
  else
    igif = igmp_if_get (igi, &igifidx);

  if (! igif)
    {
      ret = IGMP_ERR_NO_SUCH_IFF;
      goto EXIT;
    }

  if (except_alist)
    {
      if (! access_list_reference_validate (except_alist))
        {
          ret = IGMP_ERR_MALFORMED_ARG;
          goto EXIT;
        }

      igif->igif_conf.igifc_limit_except_alist =
          XSTRDUP (MTYPE_ACCESS_LIST_STR, except_alist);

      if (! igif->igif_conf.igifc_limit_except_alist)
        {
          ret = IGMP_ERR_OOM;
          goto EXIT;
        }
    }
  else if (igif->igif_conf.igifc_limit_except_alist)
    {
      XFREE (MTYPE_ACCESS_LIST_STR,
             igif->igif_conf.igifc_limit_except_alist);
      igif->igif_conf.igifc_limit_except_alist = NULL;
    }

  SET_FLAG (igif->igif_conf.igifc_cflags,
            IGMP_IF_CFLAG_LIMIT_GREC);

  igif->igif_conf.igifc_limit = limit;

  if (if_is_vlan (ifp))
    (void_t) igmp_if_snp_inherit_config (igif);

  ret = igmp_if_update (igif, NULL, IGMP_IF_UPDATE_CONFIG_ENABLE);

EXIT:

  IGMP_FN_EXIT (ret);
}

/* Unset the Interface Group-Membership Limit value */
s_int32_t
igmp_if_limit_unset (struct igmp_instance *igi,
                     struct interface *ifp)
{
  struct igmp_if_idx igifidx;
  struct igmp_if *igif;
  s_int32_t ret;

  IGMP_FN_ENTER (IF Limit Unset);

  ret = IGMP_ERR_NONE;
  igif = NULL;

  if (! igi || ! ifp)
    {
      ret = IGMP_ERR_INVALID_VALUE;
      goto EXIT;
    }

  if (INTF_TYPE_L2 (ifp))
    {
      ret = IGMP_ERR_L2_PHYSICAL_IF;
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

  UNSET_FLAG (igif->igif_conf.igifc_cflags,
              IGMP_IF_CFLAG_LIMIT_GREC);

  igif->igif_conf.igifc_limit = IGMP_LIMIT_DEF;

  if (igif->igif_conf.igifc_limit_except_alist)
    {
      XFREE (MTYPE_ACCESS_LIST_STR,
             igif->igif_conf.igifc_limit_except_alist);
      igif->igif_conf.igifc_limit_except_alist = NULL;
    }

  if (if_is_vlan (ifp))
    (void_t) igmp_if_snp_inherit_config (igif);

  ret = igmp_if_update (igif, NULL, IGMP_IF_UPDATE_CONFIG_DISABLE);

EXIT:

  IGMP_FN_EXIT (ret);
}

/* Set IGMP Mroute-Proxy Interface */
s_int32_t
igmp_if_mroute_pxy_set (struct igmp_instance *igi,
                        struct interface *ifp,
                        u_int8_t *mrtr_pxy_ifname)
{
  struct igmp_if_idx *curr_mrtr_igifidx;
  struct interface *mrtr_pxy_ifp;
  struct igmp_if_idx igifidx;
  struct igmp_if *igif;
  struct listnode *next;
  struct listnode *nn;
  struct apn_vr *vr;
  s_int32_t ret;

  IGMP_FN_ENTER (IF Mroute Proxy Set);

  curr_mrtr_igifidx = NULL;
  ret = IGMP_ERR_NONE;
  igif = NULL;
  next = NULL;
  nn = NULL;
  vr = NULL;

  if (! igi || ! ifp || ! mrtr_pxy_ifname)
    {
      ret = IGMP_ERR_INVALID_VALUE;
      goto EXIT;
    }

  if (INTF_TYPE_L2 (ifp))
    {
      ret = IGMP_ERR_L2_PHYSICAL_IF;
      goto EXIT;
    }

  igifidx.igifidx_ifp = ifp;
  igifidx.igifidx_sid = igmp_if_svc_reg_su_id (igi, ifp);

  if (if_is_vlan (ifp))
    igif = igmp_if_lookup (igi, &igifidx);
  else
    igif = igmp_if_get (igi, &igifidx);

  if (! igif)
    {
      ret = IGMP_ERR_NO_SUCH_IFF;
      goto EXIT;
    }

  vr = LIB_IF_GET_LIB_VR (ifp);

  if (! vr)
    {
      ret = IGMP_ERR_MALFORMED_ARG;
      goto EXIT;
    }

  mrtr_pxy_ifp = ifv_lookup_by_name (LIB_VRF_GET_IF_MASTER
                                     (LIB_IF_GET_LIB_VRF (ifp)),
                                     mrtr_pxy_ifname);

  if (! mrtr_pxy_ifp)
    {
      if (igi->igi_cback_tunnel_get)
        {
          mrtr_pxy_ifp = igi->igi_cback_tunnel_get (vr->id,
                                                    mrtr_pxy_ifname);
        }
    }

  if (! mrtr_pxy_ifp)
    {
      ret = IGMP_ERR_NO_SUCH_IFF;
      goto EXIT;
    }

  if (CHECK_FLAG (igif->igif_conf.igifc_cflags,
                  IGMP_IF_CFLAG_PROXY_SERVICE))
    {
      ret = IGMP_ERR_CFG_FOR_PROXY_SERVICE;
      goto EXIT;
    }

  /* Mroute-Proxy IF should be in same VLAN */
  igifidx.igifidx_ifp = mrtr_pxy_ifp;

  SET_FLAG (igif->igif_conf.igifc_cflags,
            IGMP_IF_CFLAG_MROUTE_PROXY);

  /* Only one Interface SHOULD be configured as mroute-proxy */
  if (! LIST_ISEMPTY (&igif->igif_conf.igifc_mrouter_if_lst))
    curr_mrtr_igifidx = GETDATA (LISTHEAD
                        (&igif->igif_conf.igifc_mrouter_if_lst));

  if (! curr_mrtr_igifidx
      || curr_mrtr_igifidx->igifidx_ifp != igifidx.igifidx_ifp
      || curr_mrtr_igifidx->igifidx_sid != igifidx.igifidx_sid)
    {
      if (! curr_mrtr_igifidx)
        {
          curr_mrtr_igifidx = XCALLOC (MTYPE_IGMP_IF_IDX,
                                       sizeof (struct igmp_if_idx));

          if (! curr_mrtr_igifidx)
            {
              ret = IGMP_ERR_OOM;
              goto EXIT;
            }

          listnode_add (&igif->igif_conf.igifc_mrouter_if_lst,
                        curr_mrtr_igifidx);
        }

      *curr_mrtr_igifidx = igifidx;

      ret = igmp_if_update (igif, NULL, IGMP_IF_UPDATE_CONFIG_RESTART);
        
      if (ret < IGMP_ERR_NONE)
          {
            UNSET_FLAG (igif->igif_conf.igifc_cflags,
                        IGMP_IF_CFLAG_MROUTE_PROXY);
             
            for (nn = LISTHEAD (&igif->igif_conf.igifc_mrouter_if_lst);
                 nn; nn = next)
              {
                next = nn->next;
  
                if ((curr_mrtr_igifidx = GETDATA (nn)))
                  XFREE (MTYPE_IGMP_IF_IDX, curr_mrtr_igifidx);
              }
            list_delete_all_node (&igif->igif_conf.igifc_mrouter_if_lst);
       }
    }

EXIT:

  IGMP_FN_EXIT (ret);
}

/* UnSet IGMP Mroute-Proxy Interface */
s_int32_t
igmp_if_mroute_pxy_unset (struct igmp_instance *igi,
                          struct interface *ifp)
{
  struct igmp_if_idx igifidx;
  struct igmp_if *igif;
  s_int32_t ret;

  IGMP_FN_ENTER (IF Mroute Proxy UnSet);

  ret = IGMP_ERR_NONE;
  igif = NULL;

  if (! igi || ! ifp)
    {
      ret = IGMP_ERR_INVALID_VALUE;
      goto EXIT;
    }

  if (INTF_TYPE_L2 (ifp))
    {
      ret = IGMP_ERR_L2_PHYSICAL_IF;
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

  ret = igmp_if_update (igif, NULL, IGMP_IF_UPDATE_CONFIG_MROUTE_RESTART);
 
EXIT:

  IGMP_FN_EXIT (ret);
}

/* Set IGMP Interface for Proxy Service */
s_int32_t
igmp_if_pxy_service_set (struct igmp_instance *igi,
                         struct interface *ifp)
{
  struct igmp_if_idx igifidx;
  struct igmp_if *igif;
  s_int32_t ret;

  IGMP_FN_ENTER (IF Proxy Service Set);

  ret = IGMP_ERR_NONE;
  igif = NULL;

  if (! igi || ! ifp)
    {
      ret = IGMP_ERR_INVALID_VALUE;
      goto EXIT;
    }

  if (INTF_TYPE_L2 (ifp))
    {
      ret = IGMP_ERR_L2_PHYSICAL_IF;
      goto EXIT;
    }

  igifidx.igifidx_ifp = ifp;
  igifidx.igifidx_sid = igmp_if_svc_reg_su_id (igi, ifp);

  if (if_is_vlan (ifp))
    igif = igmp_if_lookup (igi, &igifidx);
  else
    igif = igmp_if_get (igi, &igifidx);

  if (! igif)
    {
      ret = IGMP_ERR_NO_SUCH_IFF;
      goto EXIT;
    }

  if (CHECK_FLAG (igif->igif_conf.igifc_cflags,
                  IGMP_IF_CFLAG_MROUTE_PROXY))
    {
      ret = IGMP_ERR_CFG_WITH_MROUTE_PROXY;
      goto EXIT;
    }

  if (! CHECK_FLAG (igif->igif_conf.igifc_cflags,
                    IGMP_IF_CFLAG_PROXY_SERVICE))
    {
      SET_FLAG (igif->igif_conf.igifc_cflags,
                IGMP_IF_CFLAG_PROXY_SERVICE);

      ret = igmp_if_update (igif, NULL, IGMP_IF_UPDATE_CONFIG_RESTART);
      if (ret < IGMP_ERR_NONE)
        UNSET_FLAG (igif->igif_conf.igifc_cflags,
                    IGMP_IF_CFLAG_PROXY_SERVICE);
    }

EXIT:

  IGMP_FN_EXIT (ret);
}

/* UnSet IGMP Interface for Proxy Service */
s_int32_t
igmp_if_pxy_service_unset (struct igmp_instance *igi,
                           struct interface *ifp)
{
  struct igmp_if_idx igifidx;
  struct igmp_if *igif;
  s_int32_t ret;

  IGMP_FN_ENTER (IF Proxy Service UnSet);

  ret = IGMP_ERR_NONE;
  igif = NULL;

  if (! igi || ! ifp)
    {
      ret = IGMP_ERR_INVALID_VALUE;
      goto EXIT;
    }

  if (INTF_TYPE_L2 (ifp))
    {
      ret = IGMP_ERR_L2_PHYSICAL_IF;
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

  ret = igmp_if_update (igif, NULL, IGMP_IF_UPDATE_CONFIG_PROXY_RESTART);

EXIT:

  IGMP_FN_EXIT (ret);
}

/* Set IGMP Other-Querier Timeout */
s_int32_t
igmp_if_querier_timeout_set (struct igmp_instance *igi,
                             struct interface *ifp,
                             u_int16_t other_querier_interval)
{
  struct igmp_if_idx igifidx;
  struct igmp_if *igif;
  s_int32_t ret;

  IGMP_FN_ENTER (IF Querier Timeout Set);

  ret = IGMP_ERR_NONE;
  igif = NULL;

  if (! igi || ! ifp)
    {
      ret = IGMP_ERR_INVALID_VALUE;
      goto EXIT;
    }

  if (INTF_TYPE_L2 (ifp))
    {
      ret = IGMP_ERR_L2_PHYSICAL_IF;
      goto EXIT;
    }

  igifidx.igifidx_ifp = ifp;
  igifidx.igifidx_sid = igmp_if_svc_reg_su_id (igi, ifp);

  if (if_is_vlan (ifp))
    igif = igmp_if_lookup (igi, &igifidx);
  else
    igif = igmp_if_get (igi, &igifidx);

  if (! igif)
    {
      ret = IGMP_ERR_NO_SUCH_IFF;
      goto EXIT;
    }

  SET_FLAG (igif->igif_conf.igifc_cflags,
            IGMP_IF_CFLAG_QUERIER_TIMEOUT);

  igif->igif_conf.igifc_oqi = other_querier_interval;

  igif->igif_oqi = igif->igif_conf.igifc_oqi;

  if (if_is_vlan (ifp))
    (void_t) igmp_if_snp_inherit_config (igif);

  ret = igmp_if_update (igif, NULL, IGMP_IF_UPDATE_CONFIG_ENABLE);

EXIT:

  IGMP_FN_EXIT (ret);
}

/* UnSet IGMP Other-Querier Timeout */
s_int32_t
igmp_if_querier_timeout_unset (struct igmp_instance *igi,
                               struct interface *ifp)
{
  struct igmp_if_idx igifidx;
  struct igmp_if *igif;
  s_int32_t ret;

  IGMP_FN_ENTER (IF Querier Timeout UnSet);

  ret = IGMP_ERR_NONE;
  igif = NULL;

  if (! igi || ! ifp)
    {
      ret = IGMP_ERR_INVALID_VALUE;
      goto EXIT;
    }

  if (INTF_TYPE_L2 (ifp))
    {
      ret = IGMP_ERR_L2_PHYSICAL_IF;
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

  UNSET_FLAG (igif->igif_conf.igifc_cflags,
              IGMP_IF_CFLAG_QUERIER_TIMEOUT);

  igif->igif_conf.igifc_oqi = IGMP_QUERIER_TIMEOUT_DEF;

  igif->igif_oqi = igif->igif_rv *
                   igif->igif_qi +
                   (igif->igif_conf.igifc_qri / 2);

  if (if_is_vlan (ifp))
    (void_t) igmp_if_snp_inherit_config (igif);

  ret = igmp_if_update (igif, NULL, IGMP_IF_UPDATE_CONFIG_DISABLE);

EXIT:

  IGMP_FN_EXIT (ret);
}

/* Set Query Interval value */
s_int32_t
igmp_if_query_interval_set (struct igmp_instance *igi,
                            struct interface *ifp,
                            u_int32_t query_interval)
{
  struct igmp_if_idx igifidx;
  struct igmp_if *igif;
  s_int32_t ret;

  IGMP_FN_ENTER (IF Query Int Set);

  ret = IGMP_ERR_NONE;
  igif = NULL;

  if (! igi || ! ifp)
    {
      ret = IGMP_ERR_INVALID_VALUE;
      goto EXIT;
    }

  if (INTF_TYPE_L2 (ifp))
    {
      ret = IGMP_ERR_L2_PHYSICAL_IF;
      goto EXIT;
    }

  igifidx.igifidx_ifp = ifp;
  igifidx.igifidx_sid = igmp_if_svc_reg_su_id (igi, ifp);

  if (if_is_vlan (ifp))
    igif = igmp_if_lookup (igi, &igifidx);
  else
    igif = igmp_if_get (igi, &igifidx);

  if (! igif)
    {
      ret = IGMP_ERR_NO_SUCH_IFF;
      goto EXIT;
    }

  /* Validate Query Interval to be > Query Response Interval */
  if (query_interval <= igif->igif_conf.igifc_qri)
    {
      ret = IGMP_ERR_QI_LE_QRI;
      goto EXIT;
    }

  SET_FLAG (igif->igif_conf.igifc_cflags,
            IGMP_IF_CFLAG_QUERY_INTERVAL);

  igif->igif_conf.igifc_qi = query_interval;

  /* Adopt the Configured value on a Querier */
  if (CHECK_FLAG (igif->igif_sflags, IGMP_IF_SFLAG_QUERIER)
      || igif->igif_conf.igifc_version == IGMP_VERSION_1
      || igif->igif_conf.igifc_version == IGMP_VERSION_2)
    igif->igif_qi = igif->igif_conf.igifc_qi;

  igif->igif_gmi = igif->igif_rv *
                   igif->igif_qi +
                   igif->igif_conf.igifc_qri;

  /* Recalculate OQI if not configured */
  if (! CHECK_FLAG (igif->igif_conf.igifc_cflags,
                    IGMP_IF_CFLAG_QUERIER_TIMEOUT))
    igif->igif_oqi = igif->igif_rv *
                     igif->igif_qi +
                     (igif->igif_conf.igifc_qri / 2);

  if (if_is_vlan (ifp))
    (void_t) igmp_if_snp_inherit_config (igif);

  ret = igmp_if_update (igif, NULL, IGMP_IF_UPDATE_CONFIG_ENABLE);

EXIT:

  IGMP_FN_EXIT (ret);
}

/* Unset Query Interval value */
s_int32_t
igmp_if_query_interval_unset (struct igmp_instance *igi,
                              struct interface *ifp)
{
  struct igmp_if_idx igifidx;
  struct igmp_if *igif;
  s_int32_t ret;

  IGMP_FN_ENTER (IF Query Int UnSet);

  ret = IGMP_ERR_NONE;
  igif = NULL;

  if (! igi || ! ifp)
    {
      ret = IGMP_ERR_INVALID_VALUE;
      goto EXIT;
    }

  if (INTF_TYPE_L2 (ifp))
    {
      ret = IGMP_ERR_L2_PHYSICAL_IF;
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

  /* Validate Query Interval to be > Query Response Interval */
  if (IGMP_QUERY_INTERVAL_DEF <= igif->igif_conf.igifc_qri)
    {
      ret = IGMP_ERR_QI_LE_QRI;
      goto EXIT;
    }

  UNSET_FLAG (igif->igif_conf.igifc_cflags,
              IGMP_IF_CFLAG_QUERY_INTERVAL);

  igif->igif_conf.igifc_qi = IGMP_QUERY_INTERVAL_DEF;

  /* Adopt the Configured value on a Querier */
  if (CHECK_FLAG (igif->igif_sflags, IGMP_IF_SFLAG_QUERIER)
      || igif->igif_conf.igifc_version == IGMP_VERSION_1
      || igif->igif_conf.igifc_version == IGMP_VERSION_2)
    igif->igif_qi = igif->igif_conf.igifc_qi;

  igif->igif_gmi = igif->igif_rv *
                   igif->igif_qi +
                   igif->igif_conf.igifc_qri;

  /* Recalculate OQI if not configured */
  if (! CHECK_FLAG (igif->igif_conf.igifc_cflags,
                    IGMP_IF_CFLAG_QUERIER_TIMEOUT))
    igif->igif_oqi = igif->igif_rv *
                     igif->igif_qi +
                     (igif->igif_conf.igifc_qri / 2);

  if (! CHECK_FLAG (igif->igif_conf.igifc_cflags,
                    IGMP_IF_CFLAG_STARTUP_QUERY_INTERVAL))
    igif->igif_conf.igifc_sqi = IGMP_STARTUP_QUERY_INTERVAL_DEF;

  if (if_is_vlan (ifp))
    (void_t) igmp_if_snp_inherit_config (igif);

  ret = igmp_if_update (igif, NULL, IGMP_IF_UPDATE_CONFIG_DISABLE);

EXIT:

  IGMP_FN_EXIT (ret);
}

/* Set IGMP Query Response Interval value */
s_int32_t
igmp_if_query_response_interval_set (struct igmp_instance *igi,
                                     struct interface *ifp,
                                     u_int32_t response_interval)
{
  struct igmp_if_idx igifidx;
  struct igmp_if *igif;
  s_int32_t ret;

  IGMP_FN_ENTER (IF Query Response Int Set);

  ret = IGMP_ERR_NONE;
  igif = NULL;

  if (! igi || ! ifp)
    {
      ret = IGMP_ERR_INVALID_VALUE;
      goto EXIT;
    }

  if (INTF_TYPE_L2 (ifp))
    {
      ret = IGMP_ERR_L2_PHYSICAL_IF;
      goto EXIT;
    }

  igifidx.igifidx_ifp = ifp;
  igifidx.igifidx_sid = igmp_if_svc_reg_su_id (igi, ifp);

  if (if_is_vlan (ifp))
    igif = igmp_if_lookup (igi, &igifidx);
  else
    igif = igmp_if_get (igi, &igifidx);

  if (! igif)
    {
      ret = IGMP_ERR_NO_SUCH_IFF;
      goto EXIT;
    }

  /* Validate Query Interval to be >= Query Response Interval */
  if (igif->igif_qi < response_interval)
    {
      ret = IGMP_ERR_QRI_GT_QI;
      goto EXIT;
    }

  SET_FLAG (igif->igif_conf.igifc_cflags,
            IGMP_IF_CFLAG_QUERY_RESPONSE_INTERVAL);

  igif->igif_conf.igifc_qri = response_interval;

  igif->igif_gmi = igif->igif_rv *
                   igif->igif_qi +
                   igif->igif_conf.igifc_qri;

  /* Recalculate OQI if not configured */
  if (! CHECK_FLAG (igif->igif_conf.igifc_cflags,
                    IGMP_IF_CFLAG_QUERIER_TIMEOUT))
    igif->igif_oqi = igif->igif_rv *
                     igif->igif_qi +
                     (igif->igif_conf.igifc_qri / 2);

  if (! CHECK_FLAG (igif->igif_conf.igifc_cflags,
                    IGMP_IF_CFLAG_STARTUP_QUERY_INTERVAL))
    igif->igif_conf.igifc_sqi = IGMP_STARTUP_QUERY_INTERVAL_DEF;

  if (if_is_vlan (ifp))
    (void_t) igmp_if_snp_inherit_config (igif);

  ret = igmp_if_update (igif, NULL, IGMP_IF_UPDATE_CONFIG_DISABLE);

EXIT:

  IGMP_FN_EXIT (ret);
}

/* Set the Startup Query Count */
s_int32_t
igmp_if_startup_query_count_set (struct igmp_instance *igi,
                                 struct interface *ifp,
                                 u_int32_t query_count)
{
  struct igmp_if_idx igifidx;
  struct igmp_if *igif;
  s_int32_t ret;

  IGMP_FN_ENTER (IF Startup Query Count Set);

  ret = IGMP_ERR_NONE;
  igif = NULL;

  if (! igi || ! ifp)
    {
      ret = IGMP_ERR_INVALID_VALUE;
      goto EXIT;
    }

  if (INTF_TYPE_L2 (ifp))
    {
      ret = IGMP_ERR_L2_PHYSICAL_IF;
      goto EXIT;
    }

  igifidx.igifidx_ifp = ifp;
  igifidx.igifidx_sid = igmp_if_svc_reg_su_id (igi, ifp);

  if (if_is_vlan (ifp))
    igif = igmp_if_lookup (igi, &igifidx);
  else
    igif = igmp_if_get (igi, &igifidx);

  if (! igif)
    {
      ret = IGMP_ERR_NO_SUCH_IFF;
      goto EXIT;
    }

  SET_FLAG (igif->igif_conf.igifc_cflags,
            IGMP_IF_CFLAG_STARTUP_QUERY_COUNT);

  igif->igif_conf.igifc_sqc = query_count;
  igif->igif_sqc = query_count;

  if (if_is_vlan (ifp))
    (void_t) igmp_if_snp_inherit_config (igif);

  ret = igmp_if_update (igif, NULL, IGMP_IF_UPDATE_CONFIG_ENABLE);

EXIT:

  IGMP_FN_EXIT (ret);
}

/* Unset the Startup Query Count */
s_int32_t
igmp_if_startup_query_count_unset (struct igmp_instance *igi,
                                   struct interface *ifp)
{
  struct igmp_if_idx igifidx;
  struct igmp_if *igif;
  s_int32_t ret;

  IGMP_FN_ENTER (IF Startup Query Count Unset);

  ret = IGMP_ERR_NONE;
  igif = NULL;

  if (! igi || ! ifp)
    {
      ret = IGMP_ERR_INVALID_VALUE;
      goto EXIT;
    }

  if (INTF_TYPE_L2 (ifp))
    {
      ret = IGMP_ERR_L2_PHYSICAL_IF;
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

  UNSET_FLAG (igif->igif_conf.igifc_cflags,
              IGMP_IF_CFLAG_STARTUP_QUERY_COUNT);

  if (CHECK_FLAG (igif->igif_conf.igifc_cflags,
                  IGMP_IF_CFLAG_ROBUSTNESS_VAR))
    {
      igif->igif_conf.igifc_sqc = igif->igif_conf.igifc_rv;
      igif->igif_sqc = igif->igif_rv;
    }
  else
    {
      igif->igif_conf.igifc_sqc = IGMP_STARTUP_QUERY_COUNT_DEF;
      igif->igif_sqc = IGMP_STARTUP_QUERY_COUNT_DEF;
    }

  if (if_is_vlan (ifp))
    (void_t) igmp_if_snp_inherit_config (igif);

  ret = igmp_if_update (igif, NULL, IGMP_IF_UPDATE_CONFIG_DISABLE);

EXIT:

  IGMP_FN_EXIT (ret);
}

/* Set Startup Query Interval value */
s_int32_t
igmp_if_startup_query_interval_set (struct igmp_instance *igi,
                                    struct interface *ifp,
                                    u_int32_t query_interval)
{
  struct igmp_if_idx igifidx;
  struct igmp_if *igif;
  s_int32_t ret;

  IGMP_FN_ENTER (IF Startup Query Int Set);

  ret = IGMP_ERR_NONE;
  igif = NULL;

  if (! igi || ! ifp)
    {
      ret = IGMP_ERR_INVALID_VALUE;
      goto EXIT;
    }

  if (INTF_TYPE_L2 (ifp))
    {
      ret = IGMP_ERR_L2_PHYSICAL_IF;
      goto EXIT;
    }

  igifidx.igifidx_ifp = ifp;
  igifidx.igifidx_sid = igmp_if_svc_reg_su_id (igi, ifp);

  if (if_is_vlan (ifp))
    igif = igmp_if_lookup (igi, &igifidx);
  else
    igif = igmp_if_get (igi, &igifidx);

  if (! igif)
    {
      ret = IGMP_ERR_NO_SUCH_IFF;
      goto EXIT;
    }

  SET_FLAG (igif->igif_conf.igifc_cflags,
            IGMP_IF_CFLAG_STARTUP_QUERY_INTERVAL);

  igif->igif_conf.igifc_sqi = query_interval;

  if (if_is_vlan (ifp))
    (void_t) igmp_if_snp_inherit_config (igif);

  ret = igmp_if_update (igif, NULL, IGMP_IF_UPDATE_CONFIG_ENABLE);

EXIT:

  IGMP_FN_EXIT (ret);
}

/* Unset Startup Query Interval value */
s_int32_t
igmp_if_startup_query_interval_unset (struct igmp_instance *igi,
                                      struct interface *ifp)
{
  struct igmp_if_idx igifidx;
  struct igmp_if *igif;
  s_int32_t ret;

  IGMP_FN_ENTER (IF Startup Query Int Unset);

  ret = IGMP_ERR_NONE;
  igif = NULL;

  if (! igi || ! ifp)
    {
      ret = IGMP_ERR_INVALID_VALUE;
      goto EXIT;
    }

  if (INTF_TYPE_L2 (ifp))
    {
      ret = IGMP_ERR_L2_PHYSICAL_IF;
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

  UNSET_FLAG (igif->igif_conf.igifc_cflags,
              IGMP_IF_CFLAG_STARTUP_QUERY_INTERVAL);

  if (CHECK_FLAG(igif->igif_conf.igifc_cflags,
                 IGMP_IF_CFLAG_QUERY_INTERVAL))
    igif->igif_conf.igifc_sqi = igif->igif_conf.igifc_qi >> 2;
  else
    igif->igif_conf.igifc_sqi = IGMP_STARTUP_QUERY_INTERVAL_DEF;

  if (if_is_vlan (ifp))
    (void_t) igmp_if_snp_inherit_config (igif);

  ret = igmp_if_update (igif, NULL, IGMP_IF_UPDATE_CONFIG_ENABLE);

EXIT:

  IGMP_FN_EXIT (ret);
}

/* Unset IGMP Query Response Interval value */
s_int32_t
igmp_if_query_response_interval_unset (struct igmp_instance *igi,
                                       struct interface *ifp)
{
  struct igmp_if_idx igifidx;
  struct igmp_if *igif;
  s_int32_t ret;

  IGMP_FN_ENTER (IF Query Response Int UnSet);

  ret = IGMP_ERR_NONE;
  igif = NULL;

  if (! igi || ! ifp)
    {
      ret = IGMP_ERR_INVALID_VALUE;
      goto EXIT;
    }

  if (INTF_TYPE_L2 (ifp))
    {
      ret = IGMP_ERR_L2_PHYSICAL_IF;
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

  /* Validate Query Interval to be >= Query Response Interval */
  if (igif->igif_qi < IGMP_QUERY_RESPONSE_INTERVAL_DEF)
    {
      ret = IGMP_ERR_QRI_GT_QI;
      goto EXIT;
    }

  UNSET_FLAG (igif->igif_conf.igifc_cflags,
              IGMP_IF_CFLAG_QUERY_RESPONSE_INTERVAL);

  igif->igif_conf.igifc_qri = IGMP_QUERY_RESPONSE_INTERVAL_DEF;

  igif->igif_gmi = igif->igif_rv *
                   igif->igif_qi +
                   igif->igif_conf.igifc_qri;

  /* Recalculate OQI if not configured */
  if (! CHECK_FLAG (igif->igif_conf.igifc_cflags,
                    IGMP_IF_CFLAG_QUERIER_TIMEOUT))
    igif->igif_oqi = igif->igif_rv *
                     igif->igif_qi +
                     (igif->igif_conf.igifc_qri / 2);

  if (if_is_vlan (ifp))
    (void_t) igmp_if_snp_inherit_config (igif);

  ret = igmp_if_update (igif, NULL, IGMP_IF_UPDATE_CONFIG_DISABLE);

EXIT:

  IGMP_FN_EXIT (ret);
}

/* Set the Robustness Variable */
s_int32_t
igmp_if_robustness_var_set (struct igmp_instance *igi,
                            struct interface *ifp,
                            u_int32_t robustness_var)
{
  struct igmp_if_idx igifidx;
  struct igmp_if *igif;
  s_int32_t ret;

  IGMP_FN_ENTER (IF Rob-Var Set);

  ret = IGMP_ERR_NONE;
  igif = NULL;

  if (! igi || ! ifp)
    {
      ret = IGMP_ERR_INVALID_VALUE;
      goto EXIT;
    }

  if (INTF_TYPE_L2 (ifp))
    {
      ret = IGMP_ERR_L2_PHYSICAL_IF;
      goto EXIT;
    }

  igifidx.igifidx_ifp = ifp;
  igifidx.igifidx_sid = igmp_if_svc_reg_su_id (igi, ifp);

  if (if_is_vlan (ifp))
    igif = igmp_if_lookup (igi, &igifidx);
  else
    igif = igmp_if_get (igi, &igifidx);

  if (! igif)
    {
      ret = IGMP_ERR_NO_SUCH_IFF;
      goto EXIT;
    }

  SET_FLAG (igif->igif_conf.igifc_cflags,
            IGMP_IF_CFLAG_ROBUSTNESS_VAR);

  igif->igif_conf.igifc_rv = robustness_var;
  igif->igif_rv = robustness_var;

  /* Update the Interface GMI */
  igif->igif_gmi = igif->igif_rv *
                   igif->igif_qi +
                   igif->igif_conf.igifc_qri;

  /* Recalculate OQI if not configured */
  if (! CHECK_FLAG (igif->igif_conf.igifc_cflags,
                    IGMP_IF_CFLAG_QUERIER_TIMEOUT))
    igif->igif_oqi = igif->igif_rv *
                     igif->igif_qi +
                     (igif->igif_conf.igifc_qri / 2);
  
  /* Update the Interface SQC */
  if (! CHECK_FLAG (igif->igif_conf.igifc_cflags,
                    IGMP_IF_CFLAG_STARTUP_QUERY_COUNT))
    igif->igif_conf.igifc_sqc = robustness_var;

  if (! CHECK_FLAG (igif->igif_conf.igifc_cflags,
                    IGMP_IF_CFLAG_LAST_MEMBER_QUERY_COUNT))
    igif->igif_conf.igifc_lmqc = robustness_var;


  if (if_is_vlan (ifp))
    (void_t) igmp_if_snp_inherit_config (igif);

  ret = igmp_if_update (igif, NULL, IGMP_IF_UPDATE_CONFIG_ENABLE);

EXIT:

  IGMP_FN_EXIT (ret);
}

/* UnSet the Robustness Variable */
s_int32_t
igmp_if_robustness_var_unset (struct igmp_instance *igi,
                              struct interface *ifp)
{
  struct igmp_if_idx igifidx;
  struct igmp_if *igif;
  s_int32_t ret;

  IGMP_FN_ENTER (IF Rob-Var UnSet);

  ret = IGMP_ERR_NONE;
  igif = NULL;

  if (! igi || ! ifp)
    {
      ret = IGMP_ERR_INVALID_VALUE;
      goto EXIT;
    }

  if (INTF_TYPE_L2 (ifp))
    {
      ret = IGMP_ERR_L2_PHYSICAL_IF;
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

  UNSET_FLAG (igif->igif_conf.igifc_cflags,
              IGMP_IF_CFLAG_ROBUSTNESS_VAR);

  igif->igif_conf.igifc_rv = IGMP_ROBUSTNESS_VAR_DEF;
  igif->igif_rv = IGMP_ROBUSTNESS_VAR_DEF;

  /* Update the Interface GMI */
  igif->igif_gmi = igif->igif_rv *
                   igif->igif_qi +
                   igif->igif_conf.igifc_qri;

  /* Recalculate OQI if not configured */
  if (! CHECK_FLAG (igif->igif_conf.igifc_cflags,
                    IGMP_IF_CFLAG_QUERIER_TIMEOUT))
    igif->igif_oqi = igif->igif_rv *
                     igif->igif_qi +
                     (igif->igif_conf.igifc_qri / 2);
   
  /* Update the Interface SQC */
  if (! CHECK_FLAG (igif->igif_conf.igifc_cflags,
                    IGMP_IF_CFLAG_STARTUP_QUERY_COUNT))
    igif->igif_conf.igifc_sqc = IGMP_STARTUP_QUERY_COUNT_DEF;

  if (! CHECK_FLAG (igif->igif_conf.igifc_cflags,
                    IGMP_IF_CFLAG_LAST_MEMBER_QUERY_COUNT))
    igif->igif_conf.igifc_lmqc = IGMP_LAST_MEMBER_QUERY_COUNT_DEF;

  if (if_is_vlan (ifp))
    (void_t) igmp_if_snp_inherit_config (igif);

  ret = igmp_if_update (igif, NULL, IGMP_IF_UPDATE_CONFIG_DISABLE);

EXIT:

  IGMP_FN_EXIT (ret);
}

/* Set the Strict RA Option Validation */
s_int32_t
igmp_if_ra_set (struct igmp_instance *igi,
                struct interface *ifp)
{
  struct igmp_if_idx igifidx;
  struct igmp_if *igif;
  s_int32_t ret;

  IGMP_FN_ENTER (IF RA Set);

  ret = IGMP_ERR_NONE;
  igif = NULL;

  if (! igi || ! ifp)
    {
      ret = IGMP_ERR_INVALID_VALUE;
      goto EXIT;
    }

  if (INTF_TYPE_L2 (ifp))
    {
      ret = IGMP_ERR_L2_PHYSICAL_IF;
      goto EXIT;
    }

  igifidx.igifidx_ifp = ifp;
  igifidx.igifidx_sid = igmp_if_svc_reg_su_id (igi, ifp);

  if (if_is_vlan (ifp))
    igif = igmp_if_lookup (igi, &igifidx);
  else
    igif = igmp_if_get (igi, &igifidx);

  if (! igif)
    {
      ret = IGMP_ERR_NO_SUCH_IFF;
      goto EXIT;
    }

  SET_FLAG (igif->igif_conf.igifc_cflags,
            IGMP_IF_CFLAG_RA_OPT);

  if (if_is_vlan (ifp))
    (void_t) igmp_if_snp_inherit_config (igif);

  ret = igmp_if_update (igif, NULL, IGMP_IF_UPDATE_CONFIG_ENABLE);

EXIT:

  IGMP_FN_EXIT (ret);
}

/* Unset the Strict RA Option validation*/
s_int32_t
igmp_if_ra_unset (struct igmp_instance *igi,
                  struct interface *ifp)
{
  struct igmp_if_idx igifidx;
  struct igmp_if *igif;
  s_int32_t ret;

  IGMP_FN_ENTER (IF RA Unset);

  ret = IGMP_ERR_NONE;
  igif = NULL;

  if (! igi || ! ifp)
    {
      ret = IGMP_ERR_INVALID_VALUE;
      goto EXIT;
    }

  if (INTF_TYPE_L2 (ifp))
    {
      ret = IGMP_ERR_L2_PHYSICAL_IF;
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

  UNSET_FLAG (igif->igif_conf.igifc_cflags,
              IGMP_IF_CFLAG_RA_OPT);

  if (if_is_vlan (ifp))
    (void_t) igmp_if_snp_inherit_config (igif);

  ret = igmp_if_update (igif, NULL, IGMP_IF_UPDATE_CONFIG_DISABLE);

EXIT:

  IGMP_FN_EXIT (ret);
}

/* Set IGMP IF Snooping */
s_int32_t
igmp_if_snooping_set (struct igmp_instance *igi,
                      struct interface *ifp)
{
  struct igmp_if_idx igifidx;
  struct igmp_if *igif;
  s_int32_t ret;

  IGMP_FN_ENTER (IF Snooping Set);

  ret = IGMP_ERR_NONE;
  igif = NULL;

  if (! igi || ! ifp)
    {
      ret = IGMP_ERR_INVALID_VALUE;
      goto EXIT;
    }

  if (INTF_TYPE_L2 (ifp))
    {
      ret = IGMP_ERR_L2_PHYSICAL_IF;
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

  if (CHECK_FLAG (igif->igif_sflags, IGMP_IF_SFLAG_SVC_TYPE_L3)
      && ! CHECK_FLAG (igif->igif_sflags, IGMP_IF_SFLAG_SVC_TYPE_L2))
    {
      ret = IGMP_ERR_L3_NON_VLAN_IF;
      goto EXIT;
    }

  SET_FLAG (igif->igif_conf.igifc_cflags,
            IGMP_IF_CFLAG_SNOOP_ENABLED);
  UNSET_FLAG (igif->igif_conf.igifc_cflags,
              IGMP_IF_CFLAG_SNOOP_DISABLED);

  ret = igmp_if_update (igif, NULL, IGMP_IF_UPDATE_CONFIG_ENABLE);

EXIT:

  IGMP_FN_EXIT (ret);
}

/* UnSet IGMP IF Snooping */
s_int32_t
igmp_if_snooping_unset (struct igmp_instance *igi,
                        struct interface *ifp)
{
  struct igmp_if_idx *mrtr_igifidx;
  struct igmp_if_idx igifidx;
  struct listnode *nnext;
  struct igmp_if *igif;
  struct listnode *nn;
  s_int32_t ret;

  IGMP_FN_ENTER (IF Snooping UnSet);

  ret = IGMP_ERR_NONE;

  if (! igi || ! ifp)
    {
      ret = IGMP_ERR_INVALID_VALUE;
      goto EXIT;
    }

  if (INTF_TYPE_L2 (ifp))
    {
      ret = IGMP_ERR_L2_PHYSICAL_IF;
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

  if (CHECK_FLAG (igif->igif_sflags, IGMP_IF_SFLAG_SVC_TYPE_L3)
      && ! CHECK_FLAG (igif->igif_sflags, IGMP_IF_SFLAG_SVC_TYPE_L2))
    {
      ret = IGMP_ERR_L3_NON_VLAN_IF;
      goto EXIT;
    }

  /* Unset ALL configurable IF variables to default */
  ret = igmp_if_snoop_fast_leave_unset (igi, ifp);

  if (ret < IGMP_ERR_NONE)
    IGMP_DBG_WARN (igi, EVENTS, "Snoop Fast-Leave Unset Failed on %s",
                   ifp->name);

  if (CHECK_FLAG (igif->igif_conf.igifc_cflags,
                  IGMP_IF_CFLAG_SNOOP_MROUTER_IF))
    for (nn = LISTHEAD (&igif->igif_conf.igifc_mrouter_if_lst);
         nn; nn = nnext)
      {
        nnext = nn->next;

        mrtr_igifidx = GETDATA (nn);

        if (mrtr_igifidx
            && mrtr_igifidx->igifidx_ifp)
          {
            ret = igmp_if_snoop_mrouter_if_unset
                  (igi, ifp, mrtr_igifidx->igifidx_ifp->name);

            if (ret < IGMP_ERR_NONE)
              IGMP_DBG_WARN (igi, EVENTS, "Snoop Mrtr-IF Unset Failed"
                             " on %s for Mrtr-IF %s", ifp->name,
                             mrtr_igifidx->igifidx_ifp->name);
          }
      }

  ret = igmp_if_snoop_querier_unset (igi, ifp);

  if (ret < IGMP_ERR_NONE)
    IGMP_DBG_WARN (igi, EVENTS, "Snoop Querier Unset Failed on %s",
                   ifp->name);

  ret = igmp_if_snoop_report_suppress_set (igi, ifp);

  if (ret < IGMP_ERR_NONE)
    IGMP_DBG_WARN (igi, EVENTS, "Snoop Rep-suppress Set Failed on %s",
                   ifp->name);

  UNSET_FLAG (igif->igif_conf.igifc_cflags,
              IGMP_IF_CFLAG_SNOOP_ENABLED);

  if (CHECK_FLAG (igi->igi_cflags,
                  IGMP_INST_CFLAG_SNOOP_DISABLED))
    UNSET_FLAG (igif->igif_conf.igifc_cflags,
                IGMP_IF_CFLAG_SNOOP_DISABLED);
  else
    SET_FLAG (igif->igif_conf.igifc_cflags,
              IGMP_IF_CFLAG_SNOOP_DISABLED);

  ret = igmp_if_update (igif, NULL, IGMP_IF_UPDATE_CONFIG_DISABLE);

  if (ret < IGMP_ERR_NONE)
    IGMP_DBG_WARN (igi, EVENTS, "Snoop Config Disable Failed on %s",
                   ifp->name);

EXIT:

  IGMP_FN_EXIT (ret);
}

/* Set IGMP IF Snooping Fast-Leave */
s_int32_t
igmp_if_snoop_fast_leave_set (struct igmp_instance *igi,
                              struct interface *ifp)
{
  struct igmp_if_idx igifidx;
  struct igmp_if *igif;
  s_int32_t ret;

  IGMP_FN_ENTER (IF Snoop Fast-Leave Set);

  ret = IGMP_ERR_NONE;
  igif = NULL;

  if (! igi || ! ifp)
    {
      ret = IGMP_ERR_INVALID_VALUE;
      goto EXIT;
    }

  if (INTF_TYPE_L2 (ifp))
    {
      ret = IGMP_ERR_L2_PHYSICAL_IF;
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

  if (CHECK_FLAG (igif->igif_sflags, IGMP_IF_SFLAG_SVC_TYPE_L3)
      && ! CHECK_FLAG (igif->igif_sflags, IGMP_IF_SFLAG_SVC_TYPE_L2))
    {
      ret = IGMP_ERR_L3_NON_VLAN_IF;
      goto EXIT;
    }

  SET_FLAG (igif->igif_conf.igifc_cflags,
            IGMP_IF_CFLAG_SNOOP_FAST_LEAVE);

  if (if_is_vlan (ifp))
    (void_t) igmp_if_snp_inherit_config (igif);

  ret = igmp_if_update (igif, NULL, IGMP_IF_UPDATE_CONFIG_ENABLE);

EXIT:

  IGMP_FN_EXIT (ret);
}

/* UnSet IGMP IF Snooping Fast-Leave */
s_int32_t
igmp_if_snoop_fast_leave_unset (struct igmp_instance *igi,
                                struct interface *ifp)
{
  struct igmp_if_idx igifidx;
  struct igmp_if *igif;
  s_int32_t ret;

  IGMP_FN_ENTER (IF Snoop Fast-Leave UnSet);

  ret = IGMP_ERR_NONE;

  if (! igi || ! ifp)
    {
      ret = IGMP_ERR_INVALID_VALUE;
      goto EXIT;
    }

  if (INTF_TYPE_L2 (ifp))
    {
      ret = IGMP_ERR_L2_PHYSICAL_IF;
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

  if (CHECK_FLAG (igif->igif_sflags, IGMP_IF_SFLAG_SVC_TYPE_L3)
      && ! CHECK_FLAG (igif->igif_sflags, IGMP_IF_SFLAG_SVC_TYPE_L2))
    {
      ret = IGMP_ERR_L3_NON_VLAN_IF;
      goto EXIT;
    }

  UNSET_FLAG (igif->igif_conf.igifc_cflags,
              IGMP_IF_CFLAG_SNOOP_FAST_LEAVE);

  if (if_is_vlan (ifp))
    (void_t) igmp_if_snp_inherit_config (igif);

  ret = igmp_if_update (igif, NULL, IGMP_IF_UPDATE_CONFIG_DISABLE);

EXIT:

  IGMP_FN_EXIT (ret);
}

/* Set IGMP IF Snooping Mrouter Interface */
s_int32_t
igmp_if_snoop_mrouter_if_set (struct igmp_instance *igi,
                              struct interface *ifp,
                              u_int8_t *mrtr_ifname)
{
  struct igmp_if_idx *curr_mrtr_igifidx;
  struct igmp_if_idx *mrtr_igifidx_tmp;
  struct igmp_if_idx igifidx;
  struct interface *mrtr_ifp;
  struct igmp_if *igif;
  struct listnode *nn;
  struct mrtr_ifp;
  s_int32_t ret;

  IGMP_FN_ENTER (IF Snoop Mrouter-IF Set);

  curr_mrtr_igifidx = NULL;
  mrtr_igifidx_tmp = NULL;
  ret = IGMP_ERR_NONE;
  igif = NULL;

  if (! igi || ! ifp || ! mrtr_ifname)
    {
      ret = IGMP_ERR_INVALID_VALUE;
      goto EXIT;
    }

  if (INTF_TYPE_L2 (ifp))
    {
      ret = IGMP_ERR_L2_PHYSICAL_IF;
      goto EXIT;
    }

  mrtr_ifp = ifv_lookup_by_name (LIB_VRF_GET_IF_MASTER
                                 (LIB_IF_GET_LIB_VRF (ifp)),
                                  mrtr_ifname);

  if (! mrtr_ifp)
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

  if (CHECK_FLAG (igif->igif_sflags, IGMP_IF_SFLAG_SVC_TYPE_L3)
      && ! CHECK_FLAG (igif->igif_sflags, IGMP_IF_SFLAG_SVC_TYPE_L2))
    {
      ret = IGMP_ERR_L3_NON_VLAN_IF;
      goto EXIT;
    }

  /* The Mrouter IF will be in the same VLAN */
  igifidx.igifidx_ifp = mrtr_ifp;

  /* Look for same existing configuration */
  LIST_LOOP (&igif->igif_conf.igifc_mrouter_if_lst,
             mrtr_igifidx_tmp, nn)
    if (mrtr_igifidx_tmp->igifidx_ifp == mrtr_ifp)
      {
        curr_mrtr_igifidx = mrtr_igifidx_tmp;
        break;
      }

  if (! curr_mrtr_igifidx)
    {
      curr_mrtr_igifidx = XCALLOC (MTYPE_IGMP_IF_IDX,
                                   sizeof (struct igmp_if_idx));

      if (! curr_mrtr_igifidx)
        {
          ret = IGMP_ERR_OOM;
          goto EXIT;
        }

      *curr_mrtr_igifidx = igifidx;

      listnode_add (&igif->igif_conf.igifc_mrouter_if_lst,
                    curr_mrtr_igifidx);

      SET_FLAG (igif->igif_conf.igifc_cflags,
                IGMP_IF_CFLAG_SNOOP_MROUTER_IF);

      ret = igmp_if_update (igif, NULL, IGMP_IF_UPDATE_CONFIG_RESTART);
    }

EXIT:

  IGMP_FN_EXIT (ret);
}

/* UnSet IGMP IF Snooping Mrouter Interface */
s_int32_t
igmp_if_snoop_mrouter_if_unset (struct igmp_instance *igi,
                                struct interface *ifp,
                                u_int8_t *mrtr_ifname)
{
  struct igmp_if_idx *curr_mrtr_igifidx;
  struct igmp_if_idx igifidx;
  struct interface *mrtr_ifp;
  struct igmp_if *igif;
  struct listnode *nn;
  struct mrtr_ifp;
  s_int32_t ret;

  IGMP_FN_ENTER (IF Snoop Mrouter-IF UnSet);

  curr_mrtr_igifidx = NULL;
  ret = IGMP_ERR_NONE;
  igif = NULL;

  if (! igi || ! ifp || ! mrtr_ifname)
    {
      ret = IGMP_ERR_INVALID_VALUE;
      goto EXIT;
    }

  if (INTF_TYPE_L2 (ifp))
    {
      ret = IGMP_ERR_L2_PHYSICAL_IF;
      goto EXIT;
    }

  mrtr_ifp = ifv_lookup_by_name (LIB_VRF_GET_IF_MASTER
                                 (LIB_IF_GET_LIB_VRF (ifp)),
                                  mrtr_ifname);

  if (! mrtr_ifp)
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

  if (CHECK_FLAG (igif->igif_sflags, IGMP_IF_SFLAG_SVC_TYPE_L3)
      && ! CHECK_FLAG (igif->igif_sflags, IGMP_IF_SFLAG_SVC_TYPE_L2))
    {
      ret = IGMP_ERR_L3_NON_VLAN_IF;
      goto EXIT;
    }

  /* Look for same existing configuration */
  LIST_LOOP (&igif->igif_conf.igifc_mrouter_if_lst,
             curr_mrtr_igifidx, nn)
    if (curr_mrtr_igifidx->igifidx_ifp == mrtr_ifp)
      break;

  if (curr_mrtr_igifidx)
    {
      listnode_delete (&igif->igif_conf.igifc_mrouter_if_lst,
                       curr_mrtr_igifidx);

      XFREE (MTYPE_IGMP_IF_IDX, curr_mrtr_igifidx);

      if (LIST_ISEMPTY (&igif->igif_conf.igifc_mrouter_if_lst))
        {
          UNSET_FLAG (igif->igif_conf.igifc_cflags,
                      IGMP_IF_CFLAG_SNOOP_MROUTER_IF);
          list_delete_all_node (&igif->igif_conf.igifc_mrouter_if_lst);
        }

      ret = igmp_if_update (igif, NULL, IGMP_IF_UPDATE_CONFIG_RESTART);
    }

EXIT:

  IGMP_FN_EXIT (ret);
}

/* Set IGMP IF Snooping Querier */
s_int32_t
igmp_if_snoop_querier_set (struct igmp_instance *igi,
                           struct interface *ifp)
{
  struct igmp_if_idx igifidx;
  struct igmp_if *igif;
  s_int32_t ret;

  IGMP_FN_ENTER (IF Snoop Querier Set);

  ret = IGMP_ERR_NONE;
  igif = NULL;

  if (! igi || ! ifp)
    {
      ret = IGMP_ERR_INVALID_VALUE;
      goto EXIT;
    }

  if (INTF_TYPE_L2 (ifp))
    {
      ret = IGMP_ERR_L2_PHYSICAL_IF;
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

  if (CHECK_FLAG (igif->igif_sflags, IGMP_IF_SFLAG_SVC_TYPE_L3)
      && ! CHECK_FLAG (igif->igif_sflags, IGMP_IF_SFLAG_SVC_TYPE_L2))
    {
      ret = IGMP_ERR_L3_NON_VLAN_IF;
      goto EXIT;
    }

  SET_FLAG (igif->igif_conf.igifc_cflags,
            IGMP_IF_CFLAG_SNOOP_QUERIER);

  ret = igmp_if_update (igif, NULL, IGMP_IF_UPDATE_CONFIG_RESTART);

EXIT:

  IGMP_FN_EXIT (ret);
}

/* UnSet IGMP IF Snooping Querier */
s_int32_t
igmp_if_snoop_querier_unset (struct igmp_instance *igi,
                             struct interface *ifp)
{
  struct igmp_if_idx igifidx;
  struct igmp_if *igif;
  s_int32_t ret;

  IGMP_FN_ENTER (IF Snoop Querier UnSet);

  ret = IGMP_ERR_NONE;

  if (! igi || ! ifp)
    {
      ret = IGMP_ERR_INVALID_VALUE;
      goto EXIT;
    }

  if (INTF_TYPE_L2 (ifp))
    {
      ret = IGMP_ERR_L2_PHYSICAL_IF;
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

  if (CHECK_FLAG (igif->igif_sflags, IGMP_IF_SFLAG_SVC_TYPE_L3)
      && ! CHECK_FLAG (igif->igif_sflags, IGMP_IF_SFLAG_SVC_TYPE_L2))
    {
      ret = IGMP_ERR_L3_NON_VLAN_IF;
      goto EXIT;
    }

  UNSET_FLAG (igif->igif_conf.igifc_cflags,
              IGMP_IF_CFLAG_SNOOP_QUERIER);

  ret = igmp_if_update (igif, NULL, IGMP_IF_UPDATE_CONFIG_RESTART);

EXIT:

  IGMP_FN_EXIT (ret);
}

/* Set IGMP IF Snooping Report-Suppress */
s_int32_t
igmp_if_snoop_report_suppress_set (struct igmp_instance *igi,
                                   struct interface *ifp)
{
  struct igmp_if_idx igifidx;
  struct igmp_if *igif;
  s_int32_t ret;

  IGMP_FN_ENTER (IF Snoop Report-Suppress Set);

  ret = IGMP_ERR_NONE;
  igif = NULL;

  if (! igi || ! ifp)
    {
      ret = IGMP_ERR_INVALID_VALUE;
      goto EXIT;
    }

  if (INTF_TYPE_L2 (ifp))
    {
      ret = IGMP_ERR_L2_PHYSICAL_IF;
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

  if (CHECK_FLAG (igif->igif_sflags, IGMP_IF_SFLAG_SVC_TYPE_L3)
      && ! CHECK_FLAG (igif->igif_sflags, IGMP_IF_SFLAG_SVC_TYPE_L2))
    {
      ret = IGMP_ERR_L3_NON_VLAN_IF;
      goto EXIT;
    }

  UNSET_FLAG (igif->igif_conf.igifc_cflags,
              IGMP_IF_CFLAG_SNOOP_NO_REPORT_SUPPRESS);

  if (if_is_vlan (ifp))
    (void_t) igmp_if_snp_inherit_config (igif);

  ret = igmp_if_update (igif, NULL, IGMP_IF_UPDATE_CONFIG_DISABLE);

EXIT:

  IGMP_FN_EXIT (ret);
}

/* UnSet IGMP IF Snooping Report-Suppression */
s_int32_t
igmp_if_snoop_report_suppress_unset (struct igmp_instance *igi,
                                     struct interface *ifp)
{
  struct igmp_if_idx igifidx;
  struct igmp_if *igif;
  s_int32_t ret;

  IGMP_FN_ENTER (IF Snoop Report-Suppress UnSet);

  ret = IGMP_ERR_NONE;

  if (! igi || ! ifp)
    {
      ret = IGMP_ERR_INVALID_VALUE;
      goto EXIT;
    }

  if (INTF_TYPE_L2 (ifp))
    {
      ret = IGMP_ERR_L2_PHYSICAL_IF;
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

  if (CHECK_FLAG (igif->igif_sflags, IGMP_IF_SFLAG_SVC_TYPE_L3)
      && ! CHECK_FLAG (igif->igif_sflags, IGMP_IF_SFLAG_SVC_TYPE_L2))
    {
      ret = IGMP_ERR_L3_NON_VLAN_IF;
      goto EXIT;
    }

  SET_FLAG (igif->igif_conf.igifc_cflags,
            IGMP_IF_CFLAG_SNOOP_NO_REPORT_SUPPRESS);

  if (if_is_vlan (ifp))
    (void_t) igmp_if_snp_inherit_config (igif);

  ret = igmp_if_update (igif, NULL, IGMP_IF_UPDATE_CONFIG_ENABLE);

EXIT:

  IGMP_FN_EXIT (ret);
}

/* Set the IGMP Version */
s_int32_t
igmp_if_version_set (struct igmp_instance *igi,
                     struct interface *ifp,
                     u_int16_t version)
{
  struct igmp_if_idx igifidx;
  struct igmp_if *igif;
  s_int32_t ret;

  IGMP_FN_ENTER (IF Version Set);

  ret = IGMP_ERR_NONE;
  igif = NULL;

  if (! igi || ! ifp)
    {
      ret = IGMP_ERR_INVALID_VALUE;
      goto EXIT;
    }

  if (INTF_TYPE_L2 (ifp))
    {
      ret = IGMP_ERR_L2_PHYSICAL_IF;
      goto EXIT;
    }

  igifidx.igifidx_ifp = ifp;
  igifidx.igifidx_sid = igmp_if_svc_reg_su_id (igi, ifp);

  if (if_is_vlan (ifp))
    igif = igmp_if_lookup (igi, &igifidx);
  else
    igif = igmp_if_get (igi, &igifidx);

  if (! igif)
    {
      ret = IGMP_ERR_NO_SUCH_IFF;
      goto EXIT;
    }

  SET_FLAG (igif->igif_conf.igifc_cflags, IGMP_IF_CFLAG_VERSION);

  if (igif->igif_conf.igifc_version != version)
    {
      igif->igif_conf.igifc_version = version;

      ret = igmp_if_update (igif, NULL, IGMP_IF_UPDATE_CONFIG_RESTART);
    }

EXIT:

  IGMP_FN_EXIT (ret);
}

/* UnSet the IGMP Version */
s_int32_t
igmp_if_version_unset (struct igmp_instance *igi,
                       struct interface *ifp)
{
  struct igmp_if_idx igifidx;
  struct igmp_if *igif;
  s_int32_t ret;

  IGMP_FN_ENTER (IF Version UnSet);

  ret = IGMP_ERR_NONE;
  igif = NULL;

  if (! igi || ! ifp)
    {
      ret = IGMP_ERR_INVALID_VALUE;
      goto EXIT;
    }

  if (INTF_TYPE_L2 (ifp))
    {
      ret = IGMP_ERR_L2_PHYSICAL_IF;
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

  UNSET_FLAG (igif->igif_conf.igifc_cflags,
              IGMP_IF_CFLAG_VERSION);

  igif->igif_conf.igifc_version = IGMP_VERSION_DEF;

  ret = igmp_if_update (igif, NULL, IGMP_IF_UPDATE_CONFIG_DISABLE);

EXIT:

  IGMP_FN_EXIT (ret);
}

/* Set IGMP Static Group*/
s_int32_t
igmp_if_static_group_source_set (struct igmp_instance *igi,
                                 struct interface *ifp,
                                 struct pal_in4_addr *pgrp,
                                 struct pal_in4_addr *psrc,
                                 u_int8_t *ifname,
                                 bool_t is_ssm_mapped)
{
  struct igmp_source_list *isl;
  struct igmp_if_idx igifidx;
  struct igmp_group_rec *igr;
  struct interface *cons_ifp;
  struct igmp_if *cons_igif;
  struct igmp_if *ds_igif;
  u_int32_t src_list_size;
  struct igmp_if *igif;
  struct listnode *nn;
  bool_t is_igr_add;
  u_int32_t idx;
  s_int32_t ret;

  IGMP_FN_ENTER (IF Static Group Set);

  ret = IGMP_ERR_NONE;
  cons_igif = NULL;
  ds_igif = NULL;
  igif = NULL;
  isl = NULL;
  igr = NULL;

  if (! igi || ! ifp || ! pgrp)
    {
      ret = IGMP_ERR_INVALID_VALUE;
      goto EXIT;
    }

  if (INTF_TYPE_L2 (ifp))
    {
      ret = IGMP_ERR_L2_PHYSICAL_IF;
      goto EXIT;
    }

  if (! IN_MULTICAST ( pal_ntoh32 (pgrp->s_addr)))
    {
      ret = IGMP_ERR_INVALID_VALUE;
      goto EXIT;
    }

  igifidx.igifidx_ifp = ifp;
  igifidx.igifidx_sid = igmp_if_svc_reg_su_id (igi, ifp);

  if (if_is_vlan (ifp))
    {
      igif = igmp_if_lookup (igi, &igifidx);
    }
  else
    {
      if (ifname)
        {
          ret = IGMP_ERR_L3_NON_VLAN_IF;
          goto EXIT;
        }

      igif = igmp_if_get (igi, &igifidx);
    }

  if (! igif)
    {
      ret = IGMP_ERR_NO_SUCH_IFF;
      goto EXIT;
    }

  if (ifname)
    {
      cons_ifp = ifv_lookup_by_name (LIB_VRF_GET_IF_MASTER
                                     (LIB_IF_GET_LIB_VRF (ifp)),
                                     ifname);
      if (! cons_ifp)
        {
          ret = IGMP_ERR_NO_SUCH_IFF;
          goto EXIT;
        }

      igifidx.igifidx_ifp = cons_ifp;
      igifidx.igifidx_sid = igmp_if_svc_reg_su_id (igi, ifp);

      cons_igif = igmp_if_lookup (igi, &igifidx);

      if (! cons_igif)
        {
          ret = IGMP_ERR_NO_SUCH_IFF;
          goto EXIT;
        }
    }

  /* Decode the Sources List */
  src_list_size = sizeof (struct igmp_source_list) +
                  sizeof (struct pal_in4_addr);

  isl = XCALLOC (MTYPE_IGMP_SRC_LIST, src_list_size);

  if (! isl)
    {
      IGMP_DBG_ERR (igi, EVENTS, "Cannot allocate memory"
                    " (%d) @ %s:%d", src_list_size,
                    __FILE__, __LINE__);

      ret = IGMP_ERR_OOM;
      goto EXIT;
    }

  isl->isl_num = 0;
  idx = 0;
  isl->isl_static = PAL_TRUE;

  if (psrc
      && ! IPV4_ADDR_SAME (psrc, &igi->igi_in4any_addr))
    {
      if (IN_EXPERIMENTAL (pal_ntoh32 (psrc->s_addr))
          || IN_MULTICAST (pal_ntoh32 (psrc->s_addr))
          || IPV4_ADDR_MARTIAN (pal_ntoh32 (psrc->s_addr)))
        {
          IGMP_DBG_WARN (igi, EVENTS, "Invalid Src Addr %r", psrc);
          ret = IGMP_ERR_NONE;
          goto EXIT;
        }

      IPV4_ADDR_COPY (&isl->isl_saddr [idx], psrc);
      isl->isl_num += 1;
      idx += 1;
    }

  is_igr_add = PAL_TRUE;

  if (if_is_vlan (ifp))
    {
      if (ifname)
        {
          LIST_LOOP (&igif->igif_hst_chld_lst, ds_igif, nn)
            {
              if (ds_igif == cons_igif)
                {
                  igr = igmp_if_grec_get (cons_igif, pgrp);
                  if (! igr)
                    {
                      ret = IGMP_ERR_NO_SUCH_GROUP_REC;
                      goto EXIT;
                    }

                  if(CHECK_FLAG (igr->igr_sflags, IGMP_IGR_SFLAG_JOIN))
                    {
                      IGMP_DBG_WARN (igi, EVENTS, "The Group is already join group");
                      goto EXIT;
                    }
									
                  if (is_ssm_mapped)
                    {
                      ret = igmp_if_ssm_map_process (cons_igif, pgrp, &isl);
                      if (ret < IGMP_ERR_NONE)
                        {
                          IGMP_DBG_WARN (igi, EVENTS, "SSM Process failed!");
                          goto EXIT;
                        }

                      SET_FLAG (igr->igr_cflags,
                                IGMP_IGR_CFLAG_STATIC_SOURCE_SSM_MAP);
                      isl->isl_static = PAL_TRUE;
                    }

                  if (isl->isl_num)
                    SET_FLAG (igr->igr_cflags,
                              IGMP_IGR_CFLAG_STATIC_GROUP_SOURCE);

                  SET_FLAG (igr->igr_sflags, IGMP_IGR_SFLAG_STATIC);
                  SET_FLAG (igr->igr_cflags, IGMP_IGR_CFLAG_STATIC_GROUP);
                  SET_FLAG (igr->igr_cflags,
                            IGMP_IGR_CFLAG_STATIC_GROUP_IF_NAME);
                  ret = igmp_if_grec_update_static (igr, isl, is_igr_add);
                }
            }
        }
      else
        {
          LIST_LOOP (&igif->igif_hst_chld_lst, ds_igif, nn)
            {
              igr = igmp_if_grec_get (ds_igif, pgrp);

              if (! igr)
                {
                  ret = IGMP_ERR_NO_SUCH_GROUP_REC;
                  goto EXIT;
                }

              if(CHECK_FLAG (igr->igr_sflags, IGMP_IGR_SFLAG_JOIN))
                {
                  IGMP_DBG_WARN (igi, EVENTS, "The Group is already join group");
                  goto EXIT;
                }

              if (is_ssm_mapped)
                {
                  ret = igmp_if_ssm_map_process (ds_igif, pgrp, &isl);

                  if (ret < IGMP_ERR_NONE)
                    {
                      IGMP_DBG_WARN (igi, EVENTS, "SSM Process failed!");
                      goto EXIT;
                    }

                   SET_FLAG (igr->igr_cflags,
                             IGMP_IGR_CFLAG_STATIC_SOURCE_SSM_MAP);
                  isl->isl_static = PAL_TRUE;
                }

              if (isl->isl_num)
                SET_FLAG (igr->igr_cflags,
                          IGMP_IGR_CFLAG_STATIC_GROUP_SOURCE);

              SET_FLAG (igr->igr_sflags, IGMP_IGR_SFLAG_STATIC);
              SET_FLAG (igr->igr_cflags, IGMP_IGR_CFLAG_STATIC_GROUP);
              ret = igmp_if_grec_update_static (igr, isl, is_igr_add);
            }
        }
    }
  else
    {
      igr = igmp_if_grec_get (igif, pgrp);

      if (! igr)
        {
          ret = IGMP_ERR_NO_SUCH_GROUP_REC;
          goto EXIT;
        }

      if(CHECK_FLAG (igr->igr_sflags, IGMP_IGR_SFLAG_JOIN))
        {
          IGMP_DBG_WARN (igi, EVENTS, "The Group is already join group");
          goto EXIT;
        }

      if (is_ssm_mapped)
        {
          ret = igmp_if_ssm_map_process (igif, pgrp, &isl);

          if (ret < IGMP_ERR_NONE)
            {
              IGMP_DBG_WARN (igi, EVENTS, "SSM Process failed!");
              goto EXIT;
            }

          SET_FLAG (igr->igr_cflags,
                    IGMP_IGR_CFLAG_STATIC_SOURCE_SSM_MAP);
          isl->isl_static = PAL_TRUE;
        }

      if (isl->isl_num)
        SET_FLAG (igr->igr_cflags,
                  IGMP_IGR_CFLAG_STATIC_GROUP_SOURCE);

      SET_FLAG (igr->igr_sflags, IGMP_IGR_SFLAG_STATIC);
      SET_FLAG (igr->igr_cflags, IGMP_IGR_CFLAG_STATIC_GROUP);
      ret = igmp_if_grec_update_static (igr, isl, is_igr_add);
    }

EXIT:

  if (isl)
    XFREE (MTYPE_IGMP_SRC_LIST, isl);

  /* Error-code correction check-post */
  switch (ret)
    {
      case IGMP_ERR_OOM:
        ret = IGMP_ERR_NONE;
        break;

      default:
        break;
    }

  IGMP_FN_EXIT (ret);
}

/* Unset IGMP Static Group*/
s_int32_t
igmp_if_static_group_source_unset (struct igmp_instance *igi,
                                   struct interface *ifp,
                                   struct pal_in4_addr *pgrp,
                                   struct pal_in4_addr *psrc,
                                   u_int8_t *ifname,
                                   bool_t is_ssm_mapped)
{
  struct igmp_source_list *isl;
  struct igmp_if_idx igifidx;
  struct igmp_group_rec *igr;
  struct interface *cons_ifp;
  struct igmp_if *cons_igif;
  struct igmp_if *ds_igif;
  u_int32_t src_list_size;
  struct igmp_if *igif;
  struct listnode *nn;
  bool_t is_igr_add;
  u_int32_t idx;
  s_int32_t ret;

  IGMP_FN_ENTER (IF Static Group Unset);

  ret = IGMP_ERR_NONE;
  isl = NULL;
  igr = NULL;
  igif = NULL;
  cons_igif = NULL;

  if (! igi || ! ifp || ! pgrp)
    {
      ret = IGMP_ERR_INVALID_VALUE;
      goto EXIT;
    }

  if (INTF_TYPE_L2 (ifp))
    {
      ret = IGMP_ERR_L2_PHYSICAL_IF;
      goto EXIT;
    }

  if (! IN_MULTICAST ( pal_ntoh32 (pgrp->s_addr)))
    {
      ret = IGMP_ERR_INVALID_VALUE;
      goto EXIT;
    }

  igifidx.igifidx_ifp = ifp;
  igifidx.igifidx_sid = igmp_if_svc_reg_su_id (igi, ifp);

  if (if_is_vlan (ifp))
    {
      igif = igmp_if_lookup (igi, &igifidx);
    }
  else
    {
      if (ifname)
        {
          ret = IGMP_ERR_L3_NON_VLAN_IF;
          goto EXIT;
        }

      igif = igmp_if_get (igi, &igifidx);
    }

  if (! igif)
    {
      ret = IGMP_ERR_NO_SUCH_IFF;
      goto EXIT;
    }

  if (ifname)
    {
      cons_ifp = ifv_lookup_by_name (LIB_VRF_GET_IF_MASTER
                                     (LIB_IF_GET_LIB_VRF (ifp)),
                                     ifname);
      if (! cons_ifp)
        {
          ret = IGMP_ERR_NO_SUCH_IFF;
          goto EXIT;
        }

      igifidx.igifidx_ifp = cons_ifp;
      igifidx.igifidx_sid = igmp_if_svc_reg_su_id (igi, ifp);

      cons_igif = igmp_if_lookup (igi, &igifidx);

      if (! cons_igif)
        {
          ret = IGMP_ERR_NO_SUCH_IFF;
          goto EXIT;
        }
    }

  /* Decode the Sources List */
  src_list_size = sizeof (struct igmp_source_list) +
                  sizeof (struct pal_in4_addr);

  isl = XCALLOC (MTYPE_IGMP_SRC_LIST, src_list_size);

  if (! isl)
    {
      IGMP_DBG_ERR (igi, EVENTS, "Cannot allocate memory"
                    " (%d) @ %s:%d", src_list_size,
                    __FILE__, __LINE__);

      ret = IGMP_ERR_OOM;
      goto EXIT;
    }

  isl->isl_num = 0;
  idx = 0;
  isl->isl_static = PAL_TRUE;

  if (psrc
      && ! IPV4_ADDR_SAME (psrc, &igi->igi_in4any_addr))
    {
      if (IN_EXPERIMENTAL (pal_ntoh32 (psrc->s_addr))
          || IN_MULTICAST (pal_ntoh32 (psrc->s_addr))
          || IPV4_ADDR_MARTIAN (pal_ntoh32 (psrc->s_addr)))
        {
          IGMP_DBG_WARN (igi, EVENTS, "Invalid Src Addr %r", psrc);
          ret = IGMP_ERR_NONE;
          goto EXIT;
        }

      IPV4_ADDR_COPY (&isl->isl_saddr [idx], psrc);
      isl->isl_num += 1;
      idx += 1;
    }

  is_igr_add = PAL_FALSE;

  if (if_is_vlan (ifp))
    {
      if (ifname)
        {
          LIST_LOOP (&igif->igif_hst_dsif_lst, ds_igif, nn)
            {
              if (ds_igif == cons_igif)
                {
                  igr = igmp_if_grec_get (cons_igif, pgrp);
                  if (! igr)
                    {
                      ret = IGMP_ERR_NO_SUCH_GROUP_REC;
                      goto EXIT;
                    }

                  if (is_ssm_mapped)
                    {
                      ret = igmp_if_ssm_map_process (igif, pgrp, &isl);

                      if (ret < IGMP_ERR_NONE)
                        {
                          IGMP_DBG_WARN (igi, EVENTS, "SSM Process failed!");
                          goto EXIT;
                        }

                      UNSET_FLAG (igr->igr_cflags,
                                  IGMP_IGR_CFLAG_STATIC_SOURCE_SSM_MAP);
                    }

                  igmp_static_group_source_flag_unset (igr);

                  SET_FLAG (igr->igr_sflags, IGMP_IGR_SFLAG_STATIC);
                  UNSET_FLAG (igr->igr_cflags,
                              IGMP_IGR_CFLAG_STATIC_GROUP_IF_NAME);

                  if (! is_ssm_mapped &&
                      ! igr->igr_src_a_tib_count)
                    UNSET_FLAG (igr->igr_cflags, IGMP_IGR_CFLAG_STATIC_GROUP);

                  ret = igmp_if_grec_update_static (igr, isl, is_igr_add);
                }
            }
        }
      else
        {
          LIST_LOOP (&igif->igif_hst_dsif_lst, ds_igif, nn)
            {
              igr = igmp_if_grec_get (ds_igif, pgrp);

              if (! igr)
                {
                  ret = IGMP_ERR_NO_SUCH_GROUP_REC;
                  goto EXIT;
                }

              if (is_ssm_mapped)
                {
                  ret = igmp_if_ssm_map_process (ds_igif, pgrp, &isl);

                  if (ret < IGMP_ERR_NONE)
                    {
                      IGMP_DBG_WARN (igi, EVENTS, "SSM Process failed!");
                      goto EXIT;
                    }

                  UNSET_FLAG (igr->igr_cflags,
                              IGMP_IGR_CFLAG_STATIC_SOURCE_SSM_MAP);
                }

              igmp_static_group_source_flag_unset (igr);

              if (! is_ssm_mapped &&
                  ! igr->igr_src_a_tib_count)
                UNSET_FLAG (igr->igr_cflags, IGMP_IGR_CFLAG_STATIC_GROUP);

              SET_FLAG (igr->igr_sflags, IGMP_IGR_SFLAG_STATIC);
              ret = igmp_if_grec_update_static (igr, isl, is_igr_add);

            }
        }
    }
  else
    {
      igr = igmp_if_grec_get (igif, pgrp);
      if (! igr)
        {
          ret = IGMP_ERR_NO_SUCH_GROUP_REC;
          goto EXIT;
        }

      if (is_ssm_mapped)
        {
          ret = igmp_if_ssm_map_process (igif, pgrp, &isl);
          if (ret < IGMP_ERR_NONE)
            {
              IGMP_DBG_WARN (igi, EVENTS, "SSM Process failed!");
              goto EXIT;
            }

          UNSET_FLAG (igr->igr_cflags,
                      IGMP_IGR_CFLAG_STATIC_SOURCE_SSM_MAP);
        }

      igmp_static_group_source_flag_unset (igr);

      if (! is_ssm_mapped &&
          ! igr->igr_src_a_tib_count)
        UNSET_FLAG (igr->igr_cflags, IGMP_IGR_CFLAG_STATIC_GROUP);

      SET_FLAG (igr->igr_sflags, IGMP_IGR_SFLAG_STATIC);
      ret = igmp_if_grec_update_static (igr, isl, is_igr_add);
    }

EXIT:

  if (isl)
    XFREE (MTYPE_IGMP_SRC_LIST, isl);

  /* Error-code correction check-post */
  switch (ret)
    {
      case IGMP_ERR_OOM:
        ret = IGMP_ERR_NONE;
        break;

      default:
        break;
    }

  IGMP_FN_EXIT (ret);
}

void
igmp_static_group_source_flag_unset (struct igmp_group_rec *igr)
{
  struct igmp_source_rec *isr;
  struct ptree_node *pn_next;
  struct ptree_node *pn;
  s_int32_t ret;

  IGMP_FN_ENTER (Unset StaticGroupSourceFlag);

  ret = IGMP_ERR_NONE;

  for (pn = ptree_top (igr->igr_src_a_tib); pn; pn = pn_next)
    {
      pn_next = ptree_next (pn);

      if (! (isr = pn->info))
        continue;

      if (CHECK_FLAG (isr->isr_sflags, IGMP_ISR_SFLAG_STATIC))
        goto EXIT;
    }

  UNSET_FLAG (igr->igr_cflags,
              IGMP_IGR_CFLAG_STATIC_GROUP_SOURCE);

EXIT:

    IGMP_FN_EXIT ();
}

/* Set IGMP Join Group*/
s_int32_t
igmp_if_join_group_set (struct igmp_instance *igi,
                        struct interface *ifp,
                        struct pal_in4_addr *pgrp)
{/*just store "IGMP_IGR_SFLAG_JOIN" group records*/
  struct igmp_if_idx igifidx;
  struct igmp_group_rec *igr;
  struct igmp_if *igif;
  s_int32_t ret;

  IGMP_FN_ENTER (IF Join Group Set);

  ret = IGMP_ERR_NONE;
  igif = NULL;
  igr = NULL;

  if (! igi || ! ifp || ! pgrp)
    {
      ret = IGMP_ERR_INVALID_VALUE;
      goto EXIT;
    }

  if (INTF_TYPE_L2 (ifp))
    {
      ret = IGMP_ERR_L2_PHYSICAL_IF;
      goto EXIT;
    }

  if (! IN_MULTICAST ( pal_ntoh32 (pgrp->s_addr)))
    {
      ret = IGMP_ERR_INVALID_VALUE;
      goto EXIT;
    }

  igifidx.igifidx_ifp = ifp;
  igifidx.igifidx_sid = igmp_if_svc_reg_su_id (igi, ifp);

  if (if_is_vlan (ifp))
    {
      igif = igmp_if_lookup (igi, &igifidx);
    }
  else
    {
      igif = igmp_if_get (igi, &igifidx);
    }

  if (! igif)
    {
      ret = IGMP_ERR_NO_SUCH_IFF;
      goto EXIT;
    }
  
  igr = igmp_if_grec_get (igif, pgrp);

  if (! igr)
    {
      ret = IGMP_ERR_NO_SUCH_GROUP_REC;
      goto EXIT;
    }

  if(CHECK_FLAG (igr->igr_sflags, IGMP_IGR_SFLAG_STATIC))
    IGMP_DBG_WARN (igi, EVENTS, "Join Group %r add unsuccessfully on %s because the Group is already static", pgrp,
                               igif->igif_owning_ifp->name);
  else
    {
      SET_FLAG (igr->igr_sflags, IGMP_IGR_SFLAG_JOIN);

      IGMP_DBG_WARN (igi, EVENTS, "Join Group %r add successfully on %s", pgrp,
                               igif->igif_owning_ifp->name);

      ret = igmp_igr_fsm_change_state (igr, IGMP_FMS_EXCLUDE);
    }

EXIT:
  
  /* Error-code correction check-post */
  switch (ret)
    {
      case IGMP_ERR_OOM:
        ret = IGMP_ERR_NONE;
        break;

      default:
        break;
    }

  IGMP_FN_EXIT (ret);
}

/* Unset IGMP Join Group*/
s_int32_t
igmp_if_join_group_unset (struct igmp_instance *igi,
                          struct interface *ifp,
                          struct pal_in4_addr *pgrp)
{/*just remove "IGMP_IGR_SFLAG_JOIN" group records*/
  struct igmp_if_idx igifidx;
  struct igmp_group_rec *igr;
  struct igmp_if *igif;
  s_int32_t ret;

  IGMP_FN_ENTER (IF Join Group Unset);

  ret = IGMP_ERR_NONE;
  igif = NULL;
  igr = NULL;

  if (! igi || ! ifp || ! pgrp)
    {
      ret = IGMP_ERR_INVALID_VALUE;
      goto EXIT;
    }

  if (INTF_TYPE_L2 (ifp))
    {
      ret = IGMP_ERR_L2_PHYSICAL_IF;
      goto EXIT;
    }

  if (! IN_MULTICAST ( pal_ntoh32 (pgrp->s_addr)))
    {
      ret = IGMP_ERR_INVALID_VALUE;
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

  /*find the igr*/
  igr = igmp_if_grec_lookup(igif, pgrp);  
  
  /*UNSET_FLAG (igr->igr_sflags, IGMP_IGR_SFLAG_JOIN);*/
  /*Kill this group*/
  ret = igmp_if_grec_delete (igr);
  
  IGMP_DBG_WARN (igi, EVENTS, "Join Group %r delete successfully on %s", pgrp,
                              igif->igif_owning_ifp->name);
  
EXIT:
  
  /* Error-code correction check-post */
  switch (ret)
    {
      case IGMP_ERR_OOM:
        ret = IGMP_ERR_NONE;
        break;

      default:
        break;
    }

  IGMP_FN_EXIT (ret);
}

#ifdef HAVE_SNMP
s_int32_t
igmp_if_status_set (struct igmp_instance *igi,
                    struct interface *ifp,
                    u_int32_t statusval)
{
  struct igmp_if_idx igifidx;
  struct igmp_if *igif;
  s_int32_t ret;

  IGMP_FN_ENTER (Set IfStatus);

  ret = IGMP_ERR_NONE;

  if (! igi || ! ifp)
    {
      ret = IGMP_ERR_INVALID_VALUE;
      goto EXIT;
    }

  if (INTF_TYPE_L2 (ifp))
    {
      ret = IGMP_ERR_L2_PHYSICAL_IF;
      goto EXIT;
    }

  igifidx.igifidx_ifp = ifp;
  igifidx.igifidx_sid = igmp_if_svc_reg_su_id (igi, ifp);

  igif = igmp_if_lookup (igi, &igifidx);

  switch (statusval)
    {
      case IGMP_SNMP_ROW_STATUS_ACTIVE:

        if (! igif)
          {
            ret = IGMP_ERR_NO_SUCH_IFF;
            goto EXIT;
          }

        if (CHECK_FLAG (igif->igif_sflags, IGMP_IF_SFLAG_ACTIVE))
          ret = IGMP_SNMP_ROW_STATUS_ACTIVE;
        else
          ret = igmp_if_update (igif, NULL, IGMP_IF_UPDATE_CONFIG_ENABLE);
        break;

      case IGMP_SNMP_ROW_STATUS_CREATEANDGO:

        if (igif)
          {
            ret = IGMP_ERR_GENERIC;
            goto EXIT;
          }

        ret = igmp_if_set (igi, igifidx.igifidx_ifp);
        break;

      case IGMP_SNMP_ROW_STATUS_CREATEANDWAIT:
        ret = igmp_if_set (igi, igifidx.igifidx_ifp);

        if (ret >= 0)
          ret = igmp_if_update (igif, NULL, IGMP_IF_UPDATE_CONFIG_DISABLE);
        break;

      case IGMP_SNMP_ROW_STATUS_NOTINSERVICE:
        ret = IGMP_ERR_INVALID_VALUE;
        break;

      case IGMP_SNMP_ROW_STATUS_DESTROY:
        if (! igif)
          {
            ret = IGMP_ERR_NO_SUCH_IFF;
            goto EXIT;
          }

        ret = igmp_if_unset (igi, igifidx.igifidx_ifp);
        break;

      case IGMP_SNMP_ROW_STATUS_NOTREADY:
        ret = IGMP_ERR_INVALID_VALUE;
        break;

      default:
        ret = IGMP_ERR_API_GET;
        break;
    }

EXIT:

  IGMP_FN_EXIT (ret);
}

/* IGMP Get/Get Next */
s_int32_t
igmp_if_querier_get (struct igmp_instance *igi,
                     struct interface **pifp,
                     u_int8_t **ret_var,
                     pal_size_t *ret_var_len)
{
  struct igmp_if_idx igifidx;
  struct igmp_if *igif;
  s_int32_t ret;

  IGMP_FN_ENTER (IF Querier Get);

  ret = IGMP_ERR_NONE;
  *ret_var_len = 0;
  *ret_var = NULL;
  igif = NULL;

  if (! igi || ! *pifp)
    {
      ret = IGMP_ERR_INVALID_VALUE;
      goto EXIT;
    }

  if (INTF_TYPE_L2 (*pifp))
    {
      ret = IGMP_ERR_L2_PHYSICAL_IF;
      goto EXIT;
    }

  igifidx.igifidx_ifp = *pifp;
  igifidx.igifidx_sid = igmp_if_svc_reg_su_id (igi, *pifp);

  igif = igmp_if_lookup (igi, &igifidx);

  if (! igif)
    {
      ret = IGMP_ERR_NO_SUCH_IFF;
      goto EXIT;
    }

  if (! CHECK_FLAG (igif->igif_sflags, IGMP_IF_SFLAG_ACTIVE))
    {
      ret = IGMP_ERR_NO_SUCH_IFF;
      goto EXIT;
    }

  if (CHECK_FLAG (igif->igif_sflags, IGMP_IF_SFLAG_QUERIER))
    *ret_var = (u_int8_t *) &(igif->igif_paddr->u.prefix4);
  else
    *ret_var = (u_int8_t *) &(igif->igif_other_querier_addr);

  *ret_var_len = IN_ADDR_SIZE;

EXIT:

  IGMP_FN_EXIT (ret);
}

s_int32_t
igmp_if_querier_get_next (struct igmp_instance *igi,
                          struct interface **pifp,
                          u_int8_t **ret_var,
                          pal_size_t *ret_var_len)
{
  struct igmp_if_idx igifidx;
  struct igmp_if *igif;
  s_int32_t ret;

  IGMP_FN_ENTER (IF Querier Get Next);

  ret = IGMP_ERR_NONE;

  *ret_var_len = 0;
  *ret_var = NULL;
  igif = NULL;

  if (! igi)
    {
      ret = IGMP_ERR_INVALID_VALUE;
      *pifp = NULL;
      goto EXIT;
    }

  if (*pifp)
    {
      if (INTF_TYPE_L2 (*pifp))
        {
          ret = IGMP_ERR_L2_PHYSICAL_IF;
          goto EXIT;
        }
    }

  igifidx.igifidx_ifp = *pifp;
  igifidx.igifidx_sid = igmp_if_svc_reg_su_id (igi, *pifp);

  igif = igmp_if_lookup_next (igi, &igifidx);

  if (! igif)
    {
      ret = IGMP_ERR_NO_SUCH_IFF;
      *pifp = NULL;
      goto EXIT;
    }

  if (! CHECK_FLAG (igif->igif_sflags, IGMP_IF_SFLAG_ACTIVE))
    {
      ret = IGMP_ERR_NO_SUCH_IFF;
      *pifp = NULL;
      goto EXIT;
    }

  if (CHECK_FLAG (igif->igif_sflags, IGMP_IF_SFLAG_QUERIER))
    *ret_var = (u_int8_t *) &(igif->igif_paddr->u.prefix4);
  else
    *ret_var = (u_int8_t *) &(igif->igif_other_querier_addr);

  *ret_var_len = IN_ADDR_SIZE;

  *pifp = igifidx.igifidx_ifp;

EXIT:
  IGMP_FN_EXIT (ret);
}

s_int32_t
igmp_if_query_interval_get (struct igmp_instance *igi,
                            struct interface **pifp,
                            u_int8_t **ret_var,
                            pal_size_t *ret_var_len)
{
  struct igmp_if_idx igifidx;
  struct igmp_if *igif;
  s_int32_t ret;

  IGMP_FN_ENTER (IF Query Int Get);

  ret = IGMP_ERR_NONE;
  *ret_var_len = 0;
  *ret_var = NULL;
  igif = NULL;

  if (! igi || ! *pifp)
    {
      ret = IGMP_ERR_INVALID_VALUE;
      goto EXIT;
    }

  if (INTF_TYPE_L2 (*pifp))
    {
      ret = IGMP_ERR_L2_PHYSICAL_IF;
      goto EXIT;
    }

  igifidx.igifidx_ifp = *pifp;
  igifidx.igifidx_sid = igmp_if_svc_reg_su_id (igi, *pifp);

  igif = igmp_if_lookup (igi, &igifidx);

  if (! igif)
    {
      ret = IGMP_ERR_NO_SUCH_IFF;
      goto EXIT;
    }

  if (CHECK_FLAG (igif->igif_sflags, IGMP_IF_SFLAG_QUERIER))
    igif->igif_snmp_ret_var = igif->igif_qi; 
  else
    igif->igif_snmp_ret_var = igif->igif_conf.igifc_qi;
   
  *ret_var = (u_int8_t *) &(igif->igif_snmp_ret_var);
  *ret_var_len = sizeof (u_int32_t);

EXIT:

  IGMP_FN_EXIT (ret);
}

s_int32_t
igmp_if_query_interval_get_next (struct igmp_instance *igi,
                                 struct interface **pifp,
                                 u_int8_t **ret_var,
                                 pal_size_t *ret_var_len)
{
  struct igmp_if_idx igifidx;
  struct igmp_if *igif;
  s_int32_t ret;

  IGMP_FN_ENTER (IF Query Int GetNext);

  ret = IGMP_ERR_NONE;
  *ret_var_len = 0;
  *ret_var = NULL;
  igif = NULL;

  if (! igi)
    {
      ret = IGMP_ERR_INVALID_VALUE;
      *pifp = NULL;
      goto EXIT;
    }

  if (*pifp)
    {
      if (INTF_TYPE_L2 (*pifp))
        {
          ret = IGMP_ERR_L2_PHYSICAL_IF;
          goto EXIT;
        }
    }

  igifidx.igifidx_ifp = *pifp;
  igifidx.igifidx_sid = igmp_if_svc_reg_su_id (igi, *pifp);

  igif = igmp_if_lookup_next (igi, &igifidx);

  if (! igif)
    {
      ret = IGMP_ERR_NO_SUCH_IFF;
      *pifp = NULL;
      goto EXIT;
    }

  if (CHECK_FLAG (igif->igif_sflags, IGMP_IF_SFLAG_QUERIER))
    igif->igif_snmp_ret_var = igif->igif_qi;
  else
    igif->igif_snmp_ret_var = igif->igif_conf.igifc_qi;

  *ret_var = (u_int8_t *) &(igif->igif_snmp_ret_var);
  *ret_var_len = sizeof (u_int32_t);

  *pifp = igifidx.igifidx_ifp;

EXIT:

  IGMP_FN_EXIT (ret);
}

s_int32_t
igmp_if_status_get (struct igmp_instance *igi,
                    struct interface **pifp,
                    u_int8_t **ret_var,
                    pal_size_t *ret_var_len)
{
  struct igmp_if_idx igifidx;
  struct igmp_if *igif;
  s_int32_t ret;

  IGMP_FN_ENTER (IF Querier Status Get);

  ret = IGMP_ERR_NONE;
  *ret_var_len = 0;
  *ret_var = NULL;
  igif = NULL;

  if (! igi || ! *pifp)
    {
      ret = IGMP_ERR_INVALID_VALUE;
      goto EXIT;
    }

  if (INTF_TYPE_L2 (*pifp))
    {
      ret = IGMP_ERR_L2_PHYSICAL_IF;
      goto EXIT;
    }

  igifidx.igifidx_ifp = *pifp;
  igifidx.igifidx_sid = igmp_if_svc_reg_su_id (igi, *pifp);

  igif = igmp_if_lookup (igi, &igifidx);

  if (! igif)
    {
      ret = IGMP_ERR_NO_SUCH_IFF;
      goto EXIT;
    }

  if (CHECK_FLAG (igif->igif_sflags, IGMP_IF_SFLAG_ACTIVE))
    igif->igif_snmp_ret_var = IGMP_SNMP_ROW_STATUS_ACTIVE;
  else
    igif->igif_snmp_ret_var = IGMP_SNMP_ROW_STATUS_NOTINSERVICE;

  *ret_var = (u_int8_t *) &(igif->igif_snmp_ret_var);
  *ret_var_len = sizeof (u_int32_t);

EXIT:

  IGMP_FN_EXIT (ret);
}

s_int32_t
igmp_if_status_get_next (struct igmp_instance *igi,
                         struct interface **pifp,
                         u_int8_t **ret_var,
                         pal_size_t *ret_var_len)
{
  struct igmp_if_idx igifidx;
  struct igmp_if *igif;
  s_int32_t ret;

  IGMP_FN_ENTER (IF Status Get Next);

  ret = IGMP_ERR_NONE;
  *ret_var_len = 0;
  *ret_var = NULL;
  igif = NULL;

  if (! igi)
    {
      ret = IGMP_ERR_INVALID_VALUE;
      *pifp = NULL;
      goto EXIT;
    }

  if (*pifp)
    {
      if (INTF_TYPE_L2 (*pifp))
        {
          ret = IGMP_ERR_L2_PHYSICAL_IF;
          goto EXIT;
        }
    }

  igifidx.igifidx_ifp = *pifp;
  igifidx.igifidx_sid = igmp_if_svc_reg_su_id (igi, *pifp);

  igif = igmp_if_lookup_next (igi, &igifidx);

  if (! igif)
    {
      ret = IGMP_ERR_NO_SUCH_IFF;
      *pifp = NULL;
      goto EXIT;
    }

  if (CHECK_FLAG (igif->igif_sflags, IGMP_IF_SFLAG_ACTIVE))
    igif->igif_snmp_ret_var = IGMP_SNMP_ROW_STATUS_ACTIVE;
  else
    igif->igif_snmp_ret_var = IGMP_SNMP_ROW_STATUS_NOTINSERVICE;

  *ret_var = (u_int8_t *) &(igif->igif_snmp_ret_var);
  *ret_var_len = sizeof (u_int32_t);

  *pifp = igifidx.igifidx_ifp;

EXIT:

  IGMP_FN_EXIT (ret);
}

s_int32_t
igmp_if_version_get (struct igmp_instance *igi,
                     struct interface **pifp,
                     u_int8_t **ret_var,
                     pal_size_t *ret_var_len)
{
  struct igmp_if_idx igifidx;
  struct igmp_if *igif;
  s_int32_t ret;

  IGMP_FN_ENTER (IF Querier Version Get);

  ret = IGMP_ERR_NONE;
  *ret_var_len = 0;
  *ret_var = NULL;
  igif = NULL;

  if (! igi || ! *pifp)
    {
      ret = IGMP_ERR_INVALID_VALUE;
      goto EXIT;
    }

  if (INTF_TYPE_L2 (*pifp))
    {
      ret = IGMP_ERR_L2_PHYSICAL_IF;
      goto EXIT;
    }

  igifidx.igifidx_ifp = *pifp;
  igifidx.igifidx_sid = igmp_if_svc_reg_su_id (igi, *pifp);

  igif = igmp_if_lookup (igi, &igifidx);

  if (! igif)
    {
      ret = IGMP_ERR_NO_SUCH_IFF;
      goto EXIT;
    }

  igif->igif_snmp_ret_var = igif->igif_conf.igifc_version;

  *ret_var = (u_int8_t *) &(igif->igif_snmp_ret_var);
  *ret_var_len = sizeof (u_int32_t);

EXIT:

  IGMP_FN_EXIT (ret);
}

s_int32_t
igmp_if_version_get_next (struct igmp_instance *igi,
                          struct interface **pifp,
                          u_int8_t **ret_var,
                          pal_size_t *ret_var_len)
{
  struct igmp_if_idx igifidx;
  struct igmp_if *igif;
  s_int32_t ret;

  IGMP_FN_ENTER (IF Version Get Next);

  ret = IGMP_ERR_NONE;
  *ret_var_len = 0;
  *ret_var = NULL;
  igif = NULL;

  if (! igi)
    {
      ret = IGMP_ERR_INVALID_VALUE;
      *pifp = NULL;
      goto EXIT;
    }

  if (*pifp)
    {
      if (INTF_TYPE_L2 (*pifp))
        {
          ret = IGMP_ERR_L2_PHYSICAL_IF;
          goto EXIT;
        }
    }

  igifidx.igifidx_ifp = *pifp;
  igifidx.igifidx_sid = igmp_if_svc_reg_su_id (igi, *pifp);

  igif = igmp_if_lookup_next (igi, &igifidx);

  if (! igif)
    {
      ret = IGMP_ERR_NO_SUCH_IFF;
      *pifp = NULL;
      goto EXIT;
    }

  igif->igif_snmp_ret_var = igif->igif_conf.igifc_version;

  *ret_var = (u_int8_t *) &(igif->igif_snmp_ret_var);
  *ret_var_len = sizeof (u_int32_t);

  *pifp = igifidx.igifidx_ifp;

EXIT:

  IGMP_FN_EXIT (ret);
}

s_int32_t
igmp_if_query_response_interval_get (struct igmp_instance *igi,
                                     struct interface **pifp,
                                     u_int8_t **ret_var,
                                     pal_size_t *ret_var_len)
{
  struct igmp_if_idx igifidx;
  struct igmp_if *igif;
  s_int32_t ret;

  IGMP_FN_ENTER (IF Query Response interval Get);

  ret = IGMP_ERR_NONE;
  *ret_var_len = 0;
  *ret_var = NULL;
  igif = NULL;

  if (! igi || ! *pifp)
    {
      ret = IGMP_ERR_INVALID_VALUE;
      goto EXIT;
    }

  if (INTF_TYPE_L2 (*pifp))
    {
      ret = IGMP_ERR_L2_PHYSICAL_IF;
      goto EXIT;
    }

  igifidx.igifidx_ifp = *pifp;
  igifidx.igifidx_sid = igmp_if_svc_reg_su_id (igi, *pifp);

  igif = igmp_if_lookup (igi, &igifidx);

  if (! igif)
    {
      ret = IGMP_ERR_NO_SUCH_IFF;
      goto EXIT;
    }

  igif->igif_snmp_ret_var =
       (igif->igif_conf.igifc_qri)* ONE_SEC_TENTHS_OF_SECOND;

  *ret_var = (u_int8_t *) &(igif->igif_snmp_ret_var);
  *ret_var_len = sizeof (u_int32_t);

EXIT:

  IGMP_FN_EXIT (ret);
}

s_int32_t
igmp_if_query_response_interval_get_next (struct igmp_instance *igi,
                                          struct interface **pifp,
                                          u_int8_t **ret_var,
                                          pal_size_t *ret_var_len)
{
  struct igmp_if_idx igifidx;
  struct igmp_if *igif;
  s_int32_t ret;

  IGMP_FN_ENTER (IF Query Response Interval Get Next);

  ret = IGMP_ERR_NONE;
  *ret_var_len = 0;
  *ret_var = NULL;
  igif = NULL;

  if (! igi)
    {
      ret = IGMP_ERR_INVALID_VALUE;
      *pifp = NULL;
      goto EXIT;
    }

  if (*pifp)
    {
      if (INTF_TYPE_L2 (*pifp))
        {
          ret = IGMP_ERR_L2_PHYSICAL_IF;
          goto EXIT;
        }
    }

  igifidx.igifidx_ifp = *pifp;
  igifidx.igifidx_sid = igmp_if_svc_reg_su_id (igi, *pifp);

  igif = igmp_if_lookup_next (igi, &igifidx);

  if (! igif)
    {
      ret = IGMP_ERR_NO_SUCH_IFF;
      *pifp = NULL;
      goto EXIT;
    }

  igif->igif_snmp_ret_var =
       (igif->igif_conf.igifc_qri)* ONE_SEC_TENTHS_OF_SECOND;

  *ret_var = (u_int8_t *) &(igif->igif_snmp_ret_var);
  *ret_var_len = sizeof (u_int32_t);

  *pifp = igifidx.igifidx_ifp;

EXIT:

  IGMP_FN_EXIT (ret);
}

s_int32_t
igmp_if_querier_uptime_get (struct igmp_instance *igi,
                            struct interface **pifp,
                            u_int8_t **ret_var,
                            pal_size_t *ret_var_len)
{
  struct igmp_if_idx igifidx;
  struct igmp_if *igif;
  s_int32_t ret;

  IGMP_FN_ENTER (IF Querier Uptime Get);

  ret = IGMP_ERR_NONE;
  *ret_var_len = 0;
  *ret_var = NULL;
  igif = NULL;

  if (! igi || ! *pifp)
    {
      ret = IGMP_ERR_INVALID_VALUE;
      goto EXIT;
    }

  if (INTF_TYPE_L2 (*pifp))
    {
      ret = IGMP_ERR_L2_PHYSICAL_IF;
      goto EXIT;
    }

  igifidx.igifidx_ifp = *pifp;
  igifidx.igifidx_sid = igmp_if_svc_reg_su_id (igi, *pifp);

  igif = igmp_if_lookup (igi, &igifidx);

  if (! igif)
    {
      ret = IGMP_ERR_NO_SUCH_IFF;
      goto EXIT;
    }

  if (CHECK_FLAG (igif->igif_sflags, IGMP_IF_SFLAG_QUERIER))
    igif->igif_snmp_ret_var = (pal_time_current (NULL) - igif->igif_uptime)
                               * ONE_SEC_CENTISECOND;
  else
    igif->igif_snmp_ret_var = 0;

  *ret_var = (u_int8_t *) &(igif->igif_snmp_ret_var);
  *ret_var_len = sizeof (pal_time_t);

EXIT:

  IGMP_FN_EXIT (ret);
}

s_int32_t
igmp_if_querier_uptime_get_next (struct igmp_instance *igi,
                                 struct interface **pifp,
                                 u_int8_t **ret_var,
                                 pal_size_t *ret_var_len)
{
  struct igmp_if_idx igifidx;
  struct igmp_if *igif;
  s_int32_t ret;

  IGMP_FN_ENTER (IF Querier Uptime Get Next);

  ret = IGMP_ERR_NONE;
  *ret_var_len = 0;
  *ret_var = NULL;
  igif = NULL;

  if (! igi)
    {
      ret = IGMP_ERR_INVALID_VALUE;
      *pifp = NULL;
      goto EXIT;
    }

  if (*pifp)
    {
      if (INTF_TYPE_L2 (*pifp))
        {
          ret = IGMP_ERR_L2_PHYSICAL_IF;
          goto EXIT;
        }
    }

  igifidx.igifidx_ifp = *pifp;
  igifidx.igifidx_sid = igmp_if_svc_reg_su_id (igi, *pifp);

  igif = igmp_if_lookup_next (igi, &igifidx);

  if (! igif)
    {
      ret = IGMP_ERR_NO_SUCH_IFF;
      *pifp = NULL;
      goto EXIT;
    }

  if (CHECK_FLAG (igif->igif_sflags, IGMP_IF_SFLAG_QUERIER))
    igif->igif_snmp_ret_var = (pal_time_current (NULL) - igif->igif_uptime)
                               * ONE_SEC_CENTISECOND;
  else
    igif->igif_snmp_ret_var = 0;

  *ret_var = (u_int8_t *) &(igif->igif_snmp_ret_var);
  *ret_var_len = sizeof (pal_time_t);

  *pifp = igifidx.igifidx_ifp;

EXIT:

  IGMP_FN_EXIT (ret);
}

s_int32_t
igmp_if_querier_expiry_time_get (struct igmp_instance *igi,
                                 struct interface **pifp,
                                 u_int8_t **ret_var,
                                 pal_size_t *ret_var_len)
{
  struct igmp_if_idx igifidx;
  struct igmp_if *igif;
  s_int32_t ret;

  IGMP_FN_ENTER (IF Querier Expiry time Get);

  ret = IGMP_ERR_NONE;
  *ret_var_len = 0;
  *ret_var = NULL;
  igif = NULL;

  if (! igi || ! *pifp)
    {
      ret = IGMP_ERR_INVALID_VALUE;
      goto EXIT;
    }

  if (INTF_TYPE_L2 (*pifp))
    {
      ret = IGMP_ERR_L2_PHYSICAL_IF;
      goto EXIT;
    }

  igifidx.igifidx_ifp = *pifp;
  igifidx.igifidx_sid = igmp_if_svc_reg_su_id (igi, *pifp);

  igif = igmp_if_lookup (igi, &igifidx);

  if (! igif)
    {
      ret = IGMP_ERR_NO_SUCH_IFF;
      goto EXIT;
    }

  if (! CHECK_FLAG (igif->igif_sflags, IGMP_IF_SFLAG_QUERIER))
    igif->igif_snmp_ret_var = thread_timer_remain_second (
                              igif->t_igif_other_querier);
  else
    igif->igif_snmp_ret_var = 0;

  *ret_var = (u_int8_t *) &(igif->igif_snmp_ret_var);
  *ret_var_len = sizeof (u_int32_t);

EXIT:

  IGMP_FN_EXIT (ret);
}

s_int32_t
igmp_if_querier_expiry_time_get_next (struct igmp_instance *igi,
                                      struct interface **pifp,
                                      u_int8_t **ret_var,
                                      pal_size_t *ret_var_len)
{
  struct igmp_if_idx igifidx;
  struct igmp_if *igif;
  s_int32_t ret;

  IGMP_FN_ENTER (IF Querier Expiry time Get Next);

  ret = IGMP_ERR_NONE;
  *ret_var_len = 0;
  *ret_var = NULL;
  igif = NULL;

  if (! igi)
    {
      ret = IGMP_ERR_INVALID_VALUE;
      *pifp = NULL;
      goto EXIT;
    }

  if (*pifp)
    {
      if (INTF_TYPE_L2 (*pifp))
        {
          ret = IGMP_ERR_L2_PHYSICAL_IF;
          goto EXIT;
        }
    }

  igifidx.igifidx_ifp = *pifp;
  igifidx.igifidx_sid = igmp_if_svc_reg_su_id (igi, *pifp);

  igif = igmp_if_lookup_next (igi, &igifidx);

  if (! igif)
    {
      ret = IGMP_ERR_NO_SUCH_IFF;
      *pifp = NULL;
      goto EXIT;
    }

  if (! CHECK_FLAG (igif->igif_sflags, IGMP_IF_SFLAG_QUERIER))
    igif->igif_snmp_ret_var = thread_timer_remain_second (
                              igif->t_igif_other_querier);
  else
    igif->igif_snmp_ret_var = 0;

  *ret_var = (u_int8_t *) &(igif->igif_snmp_ret_var);
  *ret_var_len = sizeof (u_int32_t);

  *pifp = igifidx.igifidx_ifp;

EXIT:

  IGMP_FN_EXIT (ret);
}

s_int32_t
igmp_if_wrong_version_queries_get (struct igmp_instance *igi,
                                   struct interface **pifp,
                                   u_int8_t **ret_var,
                                   pal_size_t *ret_var_len)
{
  struct igmp_if_idx igifidx;
  struct igmp_if *igif;
  s_int32_t ret;

  IGMP_FN_ENTER (IF Wrong version Queries Get);

  ret = IGMP_ERR_NONE;
  *ret_var_len = 0;
  *ret_var = NULL;
  igif = NULL;

  if (! igi || ! *pifp)
    {
      ret = IGMP_ERR_INVALID_VALUE;
      goto EXIT;
    }

  if (INTF_TYPE_L2 (*pifp))
    {
      ret = IGMP_ERR_L2_PHYSICAL_IF;
      goto EXIT;
    }

  igifidx.igifidx_ifp = *pifp;
  igifidx.igifidx_sid = igmp_if_svc_reg_su_id (igi, *pifp);

  igif = igmp_if_lookup (igi, &igifidx);

  if (! igif)
    {
      ret = IGMP_ERR_NO_SUCH_IFF;
      goto EXIT;
    }

  if (! CHECK_FLAG (igif->igif_sflags, IGMP_IF_SFLAG_ACTIVE))
    {
      ret = IGMP_ERR_NO_SUCH_IFF;
      goto EXIT;
    }

  igif->igif_snmp_ret_var = igif->igif_v1_querier_wcount +
                            igif->igif_v2_querier_wcount +
                            igif->igif_v3_querier_wcount;

  *ret_var = (u_int8_t *) &(igif->igif_snmp_ret_var);
  *ret_var_len = sizeof (u_int32_t);

EXIT:

  IGMP_FN_EXIT (ret);
}

s_int32_t
igmp_if_wrong_version_queries_get_next (struct igmp_instance *igi,
                                        struct interface **pifp,
                                        u_int8_t **ret_var,
                                        pal_size_t *ret_var_len)
{
  struct igmp_if_idx igifidx;
  struct igmp_if *igif;
  s_int32_t ret;

  IGMP_FN_ENTER (IF Wrong version queries Get Next);

  ret = IGMP_ERR_NONE;
  *ret_var_len = 0;
  *ret_var = NULL;
  igif = NULL;

  if (! igi)
    {
      ret = IGMP_ERR_INVALID_VALUE;
      *pifp = NULL;
      goto EXIT;
    }

  if (*pifp)
    {
      if (INTF_TYPE_L2 (*pifp))
        {
          ret = IGMP_ERR_L2_PHYSICAL_IF;
          goto EXIT;
        }
    }

  igifidx.igifidx_ifp = *pifp;
  igifidx.igifidx_sid = igmp_if_svc_reg_su_id (igi, *pifp);

  igif = igmp_if_lookup_next (igi, &igifidx);

  if (! igif)
    {
      ret = IGMP_ERR_NO_SUCH_IFF;
      *pifp = NULL;
      goto EXIT;
    }

  if (! CHECK_FLAG (igif->igif_sflags, IGMP_IF_SFLAG_ACTIVE))
    {
      ret = IGMP_ERR_NO_SUCH_IFF;
      *pifp = NULL;
      goto EXIT;
    }

  igif->igif_snmp_ret_var = igif->igif_v1_querier_wcount +
                            igif->igif_v2_querier_wcount +
                            igif->igif_v3_querier_wcount;

  *ret_var = (u_int8_t *) &(igif->igif_snmp_ret_var);
  *ret_var_len = sizeof (u_int32_t);

  *pifp = igifidx.igifidx_ifp;

EXIT:

  IGMP_FN_EXIT (ret);
}

s_int32_t
igmp_if_joins_get (struct igmp_instance *igi,
                   struct interface **pifp,
                   u_int8_t **ret_var,
                   pal_size_t *ret_var_len)
{
  struct igmp_if_idx igifidx;
  struct igmp_if *igif;
  s_int32_t ret;

  IGMP_FN_ENTER (IF Joins Get);

  ret = IGMP_ERR_NONE;
  *ret_var_len = 0;
  *ret_var = NULL;
  igif = NULL;

  if (! igi || ! *pifp)
    {
      ret = IGMP_ERR_INVALID_VALUE;
      goto EXIT;
    }

  if (INTF_TYPE_L2 (*pifp))
    {
      ret = IGMP_ERR_L2_PHYSICAL_IF;
      goto EXIT;
    }

  igifidx.igifidx_ifp = *pifp;
  igifidx.igifidx_sid = igmp_if_svc_reg_su_id (igi, *pifp);

  igif = igmp_if_lookup (igi, &igifidx);

  if (! igif)
    {
      ret = IGMP_ERR_NO_SUCH_IFF;
      goto EXIT;
    }

  if (! CHECK_FLAG (igif->igif_sflags, IGMP_IF_SFLAG_ACTIVE))
    {
      ret = IGMP_ERR_NO_SUCH_IFF;
      goto EXIT;
    }

  igif->igif_snmp_ret_var = igif->igif_num_joins;

  *ret_var = (u_int8_t *) &(igif->igif_snmp_ret_var);
  *ret_var_len = sizeof (u_int32_t);

EXIT:

  IGMP_FN_EXIT (ret);
}

s_int32_t
igmp_if_joins_get_next (struct igmp_instance *igi,
                        struct interface **pifp,
                        u_int8_t **ret_var,
                        pal_size_t *ret_var_len)
{
  struct igmp_if_idx igifidx;
  struct igmp_if *igif;
  s_int32_t ret;

  IGMP_FN_ENTER (IF Joins Get Next);

  ret = IGMP_ERR_NONE;
  *ret_var_len = 0;
  *ret_var = NULL;
  igif = NULL;

  if (! igi)
    {
      ret = IGMP_ERR_INVALID_VALUE;
      *pifp = NULL;
      goto EXIT;
    }

  if (*pifp
      && INTF_TYPE_L2 (*pifp))
    {
      ret = IGMP_ERR_L2_PHYSICAL_IF;
      goto EXIT;
    }

  igifidx.igifidx_ifp = *pifp;
  igifidx.igifidx_sid = igmp_if_svc_reg_su_id (igi, *pifp);

  igif = igmp_if_lookup_next (igi, &igifidx);

  if (! igif)
    {
      ret = IGMP_ERR_NO_SUCH_IFF;
      *pifp = NULL;
      goto EXIT;
    }

  if (! CHECK_FLAG (igif->igif_sflags, IGMP_IF_SFLAG_ACTIVE))
    {
      ret = IGMP_ERR_NO_SUCH_IFF;
      *pifp = NULL;
      goto EXIT;
    }

  igif->igif_snmp_ret_var = igif->igif_num_joins;

  *ret_var = (u_int8_t *) &(igif->igif_snmp_ret_var);
  *ret_var_len = sizeof (u_int32_t);


  *pifp = igifidx.igifidx_ifp;

EXIT:

  IGMP_FN_EXIT (ret);
}

s_int32_t
igmp_if_mroute_pxy_get (struct igmp_instance *igi,
                        struct interface **pifp,
                        u_int8_t **ret_var,
                        pal_size_t *ret_var_len)
{
  struct igmp_if_idx *igifidx_tmp;
  struct igmp_if_idx igifidx;
  struct igmp_if *igif;
  s_int32_t ret;

  IGMP_FN_ENTER (IF Proxy If Get);

  ret = IGMP_ERR_NONE;
  igifidx_tmp = NULL;
  *ret_var_len = 0;
  *ret_var = NULL;
  igif = NULL;

  if (! igi || ! *pifp)
    {
      ret = IGMP_ERR_INVALID_VALUE;
      goto EXIT;
    }

  if (INTF_TYPE_L2 (*pifp))
    {
      ret = IGMP_ERR_L2_PHYSICAL_IF;
      goto EXIT;
    }

  igifidx.igifidx_ifp = *pifp;
  igifidx.igifidx_sid = igmp_if_svc_reg_su_id (igi, *pifp);

  igif = igmp_if_lookup (igi, &igifidx);

  if (! igif)
    {
      ret = IGMP_ERR_NO_SUCH_IFF;
      goto EXIT;
    }

  if (CHECK_FLAG (igif->igif_conf.igifc_cflags,
                  IGMP_IF_CFLAG_MROUTE_PROXY)
      && ! LIST_ISEMPTY (&igif->igif_conf.igifc_mrouter_if_lst))
    {
      igifidx_tmp = GETDATA (LISTHEAD
                            (&igif->igif_conf.igifc_mrouter_if_lst));

      if (igifidx_tmp)
        igif->igif_snmp_ret_var = igifidx_tmp->igifidx_ifp->ifindex;
      else 
        igif->igif_snmp_ret_var = 0;
    }
  else
    igif->igif_snmp_ret_var = 0;

  *ret_var = (u_int8_t *) &(igif->igif_snmp_ret_var);
  *ret_var_len = sizeof (u_int32_t);

EXIT:

  IGMP_FN_EXIT (ret);
}

s_int32_t
igmp_if_mroute_pxy_get_next (struct igmp_instance *igi,
                             struct interface **pifp,
                             u_int8_t **ret_var,
                             pal_size_t *ret_var_len)
{
  struct igmp_if_idx *igifidx_tmp;
  struct igmp_if_idx igifidx;
  struct igmp_if *igif;
  s_int32_t ret;

  IGMP_FN_ENTER (IF Proxy Interface Get Next);

  ret = IGMP_ERR_NONE;
  igifidx_tmp = NULL;
  *ret_var_len = 0;
  *ret_var = NULL;
  igif = NULL;

  if (! igi)
    {
      ret = IGMP_ERR_INVALID_VALUE;
      *pifp = NULL;
      goto EXIT;
    }

  if (*pifp
      && INTF_TYPE_L2 (*pifp))
    {
      ret = IGMP_ERR_L2_PHYSICAL_IF;
      goto EXIT;
    }

  igifidx.igifidx_ifp = *pifp;
  igifidx.igifidx_sid = igmp_if_svc_reg_su_id (igi, *pifp);

  igif = igmp_if_lookup_next (igi, &igifidx);

  if (! igif)
    {
      ret = IGMP_ERR_NO_SUCH_IFF;
      *pifp = NULL;
      goto EXIT;
    }

  if (CHECK_FLAG (igif->igif_conf.igifc_cflags,
                  IGMP_IF_CFLAG_MROUTE_PROXY)
      && ! LIST_ISEMPTY (&igif->igif_conf.igifc_mrouter_if_lst))
    {
      igifidx_tmp = GETDATA (LISTHEAD
                             (&igif->igif_conf.igifc_mrouter_if_lst));

      if (igifidx_tmp)
        igif->igif_snmp_ret_var = igifidx_tmp->igifidx_ifp->ifindex;
      else
        igif->igif_snmp_ret_var = 0;
    }
  else
    igif->igif_snmp_ret_var = 0;

  *ret_var = (u_int8_t *) &(igif->igif_snmp_ret_var);
  *ret_var_len = sizeof (u_int32_t);

  *pifp = igifidx.igifidx_ifp;

EXIT:

  IGMP_FN_EXIT (ret);
}

s_int32_t
igmp_if_groups_get (struct igmp_instance *igi,
                    struct interface **pifp,
                    u_int8_t **ret_var,
                    pal_size_t *ret_var_len)
{
  struct igmp_if_idx igifidx;
  struct igmp_if *igif;
  s_int32_t ret;

  IGMP_FN_ENTER (IF Groups Get);

  ret = IGMP_ERR_NONE;
  *ret_var_len = 0;
  *ret_var = NULL;
  igif = NULL;

  if (! igi || ! *pifp)
    {
      ret = IGMP_ERR_INVALID_VALUE;
      goto EXIT;
    }

  if (INTF_TYPE_L2 (*pifp))
    {
      ret = IGMP_ERR_L2_PHYSICAL_IF;
      goto EXIT;
    }

  igifidx.igifidx_ifp = *pifp;
  igifidx.igifidx_sid = igmp_if_svc_reg_su_id (igi, *pifp);

  igif = igmp_if_lookup (igi, &igifidx);

  if (! igif)
    {
      ret = IGMP_ERR_NO_SUCH_IFF;
      goto EXIT;
    }

  if (! CHECK_FLAG (igif->igif_sflags, IGMP_IF_SFLAG_ACTIVE))
    {
      ret = IGMP_ERR_NO_SUCH_IFF;
      goto EXIT;
    }

  igif->igif_snmp_ret_var = igif->igif_num_grecs;

  *ret_var = (u_int8_t *) &(igif->igif_snmp_ret_var);
  *ret_var_len = sizeof (u_int32_t);

EXIT:

  IGMP_FN_EXIT (ret);
}

s_int32_t
igmp_if_groups_get_next (struct igmp_instance *igi,
                         struct interface **pifp,
                         u_int8_t **ret_var,
                         pal_size_t *ret_var_len)
{
  struct igmp_if_idx igifidx;
  struct igmp_if *igif;
  s_int32_t ret;

  IGMP_FN_ENTER (IF Groups Get Next);

  ret = IGMP_ERR_NONE;
  *ret_var_len = 0;
  *ret_var = NULL;
  igif = NULL;

  if (! igi)
    {
      ret = IGMP_ERR_INVALID_VALUE;
      *pifp = NULL;
      goto EXIT;
    }

  if (*pifp
      && INTF_TYPE_L2 (*pifp))
    {
      ret = IGMP_ERR_L2_PHYSICAL_IF;
      goto EXIT;
    }

  igifidx.igifidx_ifp = *pifp;
  igifidx.igifidx_sid = igmp_if_svc_reg_su_id (igi, *pifp);

  igif = igmp_if_lookup_next (igi, &igifidx);

  if (! igif)
    {
      ret = IGMP_ERR_NO_SUCH_IFF;
      *pifp = NULL;
      goto EXIT;
    }

  if (! CHECK_FLAG (igif->igif_sflags, IGMP_IF_SFLAG_ACTIVE))
    {
      ret = IGMP_ERR_NO_SUCH_IFF;
      *pifp = NULL;
      goto EXIT;
    }

  igif->igif_snmp_ret_var = igif->igif_num_grecs;

  *ret_var = (u_int8_t *) &(igif->igif_snmp_ret_var);
  *ret_var_len = sizeof (u_int32_t);

  *pifp = igifidx.igifidx_ifp;

EXIT:

  IGMP_FN_EXIT (ret);
}

s_int32_t
igmp_if_robustness_var_get (struct igmp_instance *igi,
                            struct interface **pifp,
                            u_int8_t **ret_var,
                            pal_size_t *ret_var_len)
{
  struct igmp_if_idx igifidx;
  struct igmp_if *igif;
  s_int32_t ret;

  IGMP_FN_ENTER (IF Robustness Get);

  ret = IGMP_ERR_NONE;
  *ret_var_len = 0;
  *ret_var = NULL;
  igif = NULL;

  if (! igi || ! *pifp)
    {
      ret = IGMP_ERR_INVALID_VALUE;
      goto EXIT;
    }

  if (INTF_TYPE_L2 (*pifp))
    {
      ret = IGMP_ERR_L2_PHYSICAL_IF;
      goto EXIT;
    }

  igifidx.igifidx_ifp = *pifp;
  igifidx.igifidx_sid = igmp_if_svc_reg_su_id (igi, *pifp);

  igif = igmp_if_lookup (igi, &igifidx);

  if (! igif)
    {
      ret = IGMP_ERR_NO_SUCH_IFF;
      goto EXIT;
    }

  igif->igif_snmp_ret_var = igif->igif_rv;

  *ret_var = (u_int8_t *) &(igif->igif_snmp_ret_var);
  *ret_var_len = sizeof (u_int32_t);

EXIT:

  IGMP_FN_EXIT (ret);
}

s_int32_t
igmp_if_robustness_var_get_next (struct igmp_instance *igi,
                                 struct interface **pifp,
                                 u_int8_t **ret_var,
                                 pal_size_t *ret_var_len)
{
  struct igmp_if_idx igifidx;
  struct igmp_if *igif;
  s_int32_t ret;

  IGMP_FN_ENTER (IF Robustness Get Next);

  ret = IGMP_ERR_NONE;
  *ret_var_len = 0;
  *ret_var = NULL;
  igif = NULL;

  if (! igi)
    {
      ret = IGMP_ERR_INVALID_VALUE;
      *pifp = NULL;
      goto EXIT;
    }

  if (*pifp)
    {
      if (INTF_TYPE_L2 (*pifp))
        {
          ret = IGMP_ERR_L2_PHYSICAL_IF;
          goto EXIT;
        }
    }

  igifidx.igifidx_ifp = *pifp;
  igifidx.igifidx_sid = igmp_if_svc_reg_su_id (igi, *pifp);

  igif = igmp_if_lookup_next (igi, &igifidx);

  if (! igif)
    {
      ret = IGMP_ERR_NO_SUCH_IFF;
      *pifp = NULL;
      goto EXIT;
    }

  igif->igif_snmp_ret_var = igif->igif_rv;

  *ret_var = (u_int8_t *) &(igif->igif_snmp_ret_var);
  *ret_var_len = sizeof (u_int32_t);

  *pifp = igifidx.igifidx_ifp;

EXIT:

  IGMP_FN_EXIT (ret);
}

s_int32_t
igmp_if_lmqi_get (struct igmp_instance *igi,
                  struct interface **pifp,
                  u_int8_t **ret_var,
                  pal_size_t *ret_var_len)
{
  struct igmp_if_idx igifidx;
  struct igmp_if *igif;
  s_int32_t ret;

  IGMP_FN_ENTER (IF LMQI Get);

  ret = IGMP_ERR_NONE;
  *ret_var_len = 0;
  *ret_var = NULL;
  igif = NULL;

  if (! igi || ! *pifp)
    {
      ret = IGMP_ERR_INVALID_VALUE;
      goto EXIT;
    }

  if (INTF_TYPE_L2 (*pifp))
    {
      ret = IGMP_ERR_L2_PHYSICAL_IF;
      goto EXIT;
    }

  igifidx.igifidx_ifp = *pifp;
  igifidx.igifidx_sid = igmp_if_svc_reg_su_id (igi, *pifp);

  igif = igmp_if_lookup (igi, &igifidx);

  if (! igif)
    {
      ret = IGMP_ERR_NO_SUCH_IFF;
      goto EXIT;
    }

  igif->igif_snmp_ret_var =
       (igif->igif_conf.igifc_lmqi) / MILLISEC_TO_DECISEC;

  *ret_var = (u_int8_t *) &(igif->igif_snmp_ret_var);
  *ret_var_len = sizeof (u_int32_t);

EXIT:

  IGMP_FN_EXIT (ret);
}

s_int32_t
igmp_if_lmqi_get_next (struct igmp_instance *igi,
                       struct interface **pifp,
                       u_int8_t **ret_var,
                       pal_size_t *ret_var_len)
{
  struct igmp_if_idx igifidx;
  struct igmp_if *igif;
  s_int32_t ret;

  IGMP_FN_ENTER (IF LMQI Get Next);

  ret = IGMP_ERR_NONE;
  *ret_var_len = 0;
  *ret_var = NULL;
  igif = NULL;

  if (! igi)
    {
      ret = IGMP_ERR_INVALID_VALUE;
      *pifp = NULL;
      goto EXIT;
    }

  if (*pifp)
    {
      if (INTF_TYPE_L2 (*pifp))
        {
          ret = IGMP_ERR_L2_PHYSICAL_IF;
          goto EXIT;
        }
    }

  igifidx.igifidx_ifp = *pifp;
  igifidx.igifidx_sid = igmp_if_svc_reg_su_id (igi, *pifp);

  igif = igmp_if_lookup_next (igi, &igifidx);

  if (! igif)
    {
      ret = IGMP_ERR_NO_SUCH_IFF;
      *pifp = NULL;
      goto EXIT;
    }

  igif->igif_snmp_ret_var =
       (igif->igif_conf.igifc_lmqi)/ MILLISEC_TO_DECISEC;

  *ret_var = (u_int8_t *) &(igif->igif_snmp_ret_var);
  *ret_var_len = sizeof (u_int32_t);

  *pifp = igifidx.igifidx_ifp;

EXIT:

  IGMP_FN_EXIT (ret);
}

s_int32_t
igmp_if_lmqc_get (struct igmp_instance *igi,
                  struct interface **pifp,
                  u_int8_t **ret_var,
                  pal_size_t *ret_var_len)
{
  struct igmp_if_idx igifidx;
  struct igmp_if *igif;
  s_int32_t ret;

  IGMP_FN_ENTER (IF LMQC Get);

  ret = IGMP_ERR_NONE;
  *ret_var_len = 0;
  *ret_var = NULL;
  igif = NULL;

  if (! igi || ! *pifp)
    {
      ret = IGMP_ERR_INVALID_VALUE;
      goto EXIT;
    }

  if (INTF_TYPE_L2 (*pifp))
    {
      ret = IGMP_ERR_L2_PHYSICAL_IF;
      goto EXIT;
    }

  igifidx.igifidx_ifp = *pifp;
  igifidx.igifidx_sid = igmp_if_svc_reg_su_id (igi, *pifp);

  igif = igmp_if_lookup (igi, &igifidx);

  if (! igif)
    {
      ret = IGMP_ERR_NO_SUCH_IFF;
      goto EXIT;
    }

  igif->igif_snmp_ret_var = igif->igif_conf.igifc_lmqc;

  *ret_var = (u_int8_t *) &(igif->igif_snmp_ret_var);
  *ret_var_len = sizeof (u_int32_t);

EXIT:

  IGMP_FN_EXIT (ret);
}

s_int32_t
igmp_if_lmqc_get_next (struct igmp_instance *igi,
                       struct interface **pifp,
                       u_int8_t **ret_var,
                       pal_size_t *ret_var_len)
{
  struct igmp_if_idx igifidx;
  struct igmp_if *igif;
  s_int32_t ret;

  IGMP_FN_ENTER (IF LMQC Get Next);

  ret = IGMP_ERR_NONE;
  *ret_var_len = 0;
  *ret_var = NULL;
  igif = NULL;

   if (! igi)
    {
      ret = IGMP_ERR_INVALID_VALUE;
      *pifp = NULL;
      goto EXIT;
    }

  if (*pifp)
    {
      if (INTF_TYPE_L2 (*pifp))
        {
          ret = IGMP_ERR_L2_PHYSICAL_IF;
          goto EXIT;
        }
    }

  igifidx.igifidx_ifp = *pifp;
  igifidx.igifidx_sid = igmp_if_svc_reg_su_id (igi, *pifp);

  igif = igmp_if_lookup_next (igi, &igifidx);

  if (! igif)
    {
      ret = IGMP_ERR_NO_SUCH_IFF;
      *pifp = NULL;
      goto EXIT;
    }

  igif->igif_snmp_ret_var = igif->igif_conf.igifc_lmqc;

  *ret_var = (u_int8_t *) &(igif->igif_snmp_ret_var);
  *ret_var_len = sizeof (u_int32_t);

  *pifp = igifidx.igifidx_ifp;

EXIT:

  IGMP_FN_EXIT (ret);
}

s_int32_t
igmp_if_sqc_get (struct igmp_instance *igi,
                 struct interface **pifp,
                 u_int8_t **ret_var,
                 pal_size_t *ret_var_len)
{
  struct igmp_if_idx igifidx;
  struct igmp_if *igif;
  s_int32_t ret;

  IGMP_FN_ENTER (IF SQC Get);

  ret = IGMP_ERR_NONE;
  *ret_var_len = 0;
  *ret_var = NULL;
  igif = NULL;

  if (! igi || ! *pifp)
    {
      ret = IGMP_ERR_INVALID_VALUE;
      goto EXIT;
    }

  if (INTF_TYPE_L2 (*pifp))
    {
      ret = IGMP_ERR_L2_PHYSICAL_IF;
      goto EXIT;
    }

  igifidx.igifidx_ifp = *pifp;
  igifidx.igifidx_sid = igmp_if_svc_reg_su_id (igi, *pifp);

  igif = igmp_if_lookup (igi, &igifidx);

  if (! igif)
    {
      ret = IGMP_ERR_NO_SUCH_IFF;
      goto EXIT;
    }

  igif->igif_snmp_ret_var = igif->igif_conf.igifc_sqc;

  *ret_var = (u_int8_t *) &(igif->igif_snmp_ret_var);
  *ret_var_len = sizeof (u_int32_t);

EXIT:

  IGMP_FN_EXIT (ret);
}

s_int32_t
igmp_if_sqc_get_next (struct igmp_instance *igi,
                      struct interface **pifp,
                      u_int8_t **ret_var,
                      pal_size_t *ret_var_len)
{
  struct igmp_if_idx igifidx;
  struct igmp_if *igif;
  s_int32_t ret;

  IGMP_FN_ENTER (IF SQC Get Next);

  ret = IGMP_ERR_NONE;

  *ret_var_len = 0;
  *ret_var = NULL;
  igif = NULL;

  if (! igi)
    {
      ret = IGMP_ERR_INVALID_VALUE;
      *pifp = NULL;
      goto EXIT;
    }

  if (*pifp)
    {
      if (INTF_TYPE_L2 (*pifp))
        {
          ret = IGMP_ERR_L2_PHYSICAL_IF;
          goto EXIT;
        }
    }

  igifidx.igifidx_ifp = *pifp;
  igifidx.igifidx_sid = igmp_if_svc_reg_su_id (igi, *pifp);

  igif = igmp_if_lookup_next (igi, &igifidx);

  if (! igif)
    {
      ret = IGMP_ERR_NO_SUCH_IFF;
      *pifp = NULL;
      goto EXIT;
    }

  igif->igif_snmp_ret_var = igif->igif_conf.igifc_sqc;

  *ret_var = (u_int8_t *) &(igif->igif_snmp_ret_var);
  *ret_var_len = sizeof (u_int32_t);

  *pifp = igifidx.igifidx_ifp;

EXIT:

  IGMP_FN_EXIT (ret);
}

s_int32_t
igmp_if_sqi_get (struct igmp_instance *igi,
                 struct interface **pifp,
                 u_int8_t **ret_var,
                 pal_size_t *ret_var_len)
{
  struct igmp_if_idx igifidx;
  struct igmp_if *igif;
  s_int32_t ret;

  IGMP_FN_ENTER (IF SQI Get);

  ret = IGMP_ERR_NONE;
  *ret_var_len = 0;
  *ret_var = NULL;
  igif = NULL;

  if (! igi || ! *pifp)
    {
      ret = IGMP_ERR_INVALID_VALUE;
      goto EXIT;
    }

  if (INTF_TYPE_L2 (*pifp))
    {
      ret = IGMP_ERR_L2_PHYSICAL_IF;
      goto EXIT;
    }

  igifidx.igifidx_ifp = *pifp;
  igifidx.igifidx_sid = igmp_if_svc_reg_su_id (igi, *pifp);

  igif = igmp_if_lookup (igi, &igifidx);

  if (! igif)
    {
      ret = IGMP_ERR_NO_SUCH_IFF;
      goto EXIT;
    }

  igif->igif_snmp_ret_var = igif->igif_conf.igifc_sqi;

  *ret_var = (u_int8_t *) &(igif->igif_snmp_ret_var);
  *ret_var_len = sizeof (u_int32_t);

EXIT:

  IGMP_FN_EXIT (ret);
}

s_int32_t
igmp_if_sqi_get_next (struct igmp_instance *igi,
                      struct interface **pifp,
                      u_int8_t **ret_var,
                      pal_size_t *ret_var_len)
{
  struct igmp_if_idx igifidx;
  struct igmp_if *igif;
  s_int32_t ret;

  IGMP_FN_ENTER (IF SQI Get Next);

  ret = IGMP_ERR_NONE;

  *ret_var_len = 0;
  *ret_var = NULL;
  igif = NULL;

  if (! igi)
    {
      ret = IGMP_ERR_INVALID_VALUE;
      *pifp = NULL;
      goto EXIT;
    }

  if (*pifp)
    {
      if (INTF_TYPE_L2 (*pifp))
        {
          ret = IGMP_ERR_L2_PHYSICAL_IF;
          goto EXIT;
        }
    }

  igifidx.igifidx_ifp = *pifp;
  igifidx.igifidx_sid = igmp_if_svc_reg_su_id (igi, *pifp);

  igif = igmp_if_lookup_next (igi, &igifidx);

  if (! igif)
    {
      ret = IGMP_ERR_NO_SUCH_IFF;
      *pifp = NULL;
      goto EXIT;
    }

  igif->igif_snmp_ret_var = igif->igif_conf.igifc_sqi;

  *ret_var = (u_int8_t *) &(igif->igif_snmp_ret_var);
  *ret_var_len = sizeof (u_int32_t);

  *pifp = igifidx.igifidx_ifp;

EXIT:

  IGMP_FN_EXIT (ret);
}

s_int32_t
igmp_if_cache_last_reporter_get (struct igmp_instance *igi,
                                 struct interface **pifp,
                                 struct igmp_snmp_rtr_cache_index *index,
                                 u_int8_t **ret_var,
                                 pal_size_t *ret_var_len)
{
  struct igmp_if_idx igifidx;
  struct igmp_group_rec *igr;
  struct igmp_if *igif;
  s_int32_t ret;

  IGMP_FN_ENTER (CacheLastReporter Get);

  ret = IGMP_ERR_NONE;
  *ret_var_len = 0;
  *ret_var = NULL;
  igif = NULL;

  if (! igi || ! *pifp)
    {
      ret = IGMP_ERR_INVALID_VALUE;
      goto EXIT;
    }

  if (INTF_TYPE_L2 (*pifp))
    {
      ret = IGMP_ERR_L2_PHYSICAL_IF;
      goto EXIT;
    }

  igifidx.igifidx_ifp = *pifp;
  igifidx.igifidx_sid = igmp_if_svc_reg_su_id (igi, *pifp);

  igif = igmp_if_lookup (igi, &igifidx);

  if (! igif)
    {
      ret = IGMP_ERR_NO_SUCH_IFF;
      goto EXIT;
    }

  igr = igmp_if_grec_lookup (igif, &index->cache_addr);

  if (! igr)
    {
      ret = IGMP_ERR_API_GET;
      goto EXIT;
    }

  *ret_var = (u_int8_t *) &igr->igr_last_reporter;
  *ret_var_len = sizeof (struct pal_in4_addr);
  index->ifindex = igr->igr_owning_igif->igif_idx.igifidx_ifp->ifindex;
  pal_mem_cpy(&index->cache_addr, igr->igr_owning_pn->key, sizeof (igr->igr_owning_pn->key_len));

EXIT:

  IGMP_FN_EXIT (ret);
}

struct igmp_group_rec *
igmp_if_find_next_highest_group (struct igmp_instance *igi,
                                 u_int32_t *passed_ifindex,
                                 struct pal_in4_addr *passed_group_addr,
                                 struct igmp_if **igif)
{

  struct pal_in4_addr next_highest_group;
  struct igmp_group_rec *igr_found;
  struct pal_in4_addr grp_addr;
  struct pal_in4_addr z_addr;
  struct igmp_group_rec *igr;
  struct igmp_if_idx igifidx;
  struct igmp_if *igif_found;
  struct interface *igi_ifp;
  struct ptree_node *pn;

  IGMP_FN_ENTER (Find Next Highest Group Record);

  pal_mem_set(&next_highest_group, 0xFF, IN_ADDR_SIZE);
  pal_mem_set(&z_addr, 0, IN_ADDR_SIZE);
  igif_found = NULL;
  igr_found = NULL;
  igi_ifp = NULL;
  pn = NULL;  

  IPV4_ADDR_COPY(&grp_addr,&z_addr);

  *igif = NULL;
  *passed_ifindex = 0;

  /* Look through all groups on ifindexes to find the next highest group*/
  do {
    igi_ifp = ifv_lookup_next_by_index (&igi->igi_owning_ivrf->ifv,
                                        *passed_ifindex);
    if (!igi_ifp)
      break;

    igifidx.igifidx_ifp = igi_ifp;
    igifidx.igifidx_sid = igmp_if_svc_reg_su_id (igi, igi_ifp);
    *igif = igmp_if_lookup(igi, &igifidx);

    (*passed_ifindex)++;

    if (*igif)
      pn = ptree_top ((*igif)->igif_gmr_tib);

    for (igr = NULL; pn; pn = ptree_next (pn))
      {
        if (!(igr = (pn->info)))
          continue;

        pal_mem_cpy(&grp_addr, igr->igr_owning_pn->key, IN_ADDR_SIZE); 

        if (IPV4_ADDR_CMP(&grp_addr,passed_group_addr) > 0
            && IPV4_ADDR_CMP(&grp_addr,&next_highest_group) < 0)
          {
            IPV4_ADDR_COPY(&next_highest_group,&grp_addr);
            igr_found = igr;
            igif_found = *igif;
          }  
      }
  } while (igi_ifp);

  *igif = igif_found;

  IGMP_FN_EXIT (igr_found);
}


struct igmp_group_rec *
igmp_if_table_get_next (struct igmp_instance *igi, u_int32_t *passed_ifindex,
                        struct pal_in4_addr *passed_group_addr,        
                        struct igmp_if **igif)

{
  struct pal_in4_addr cache_addr;
  struct igmp_group_rec *igr;
  struct igmp_if_idx igifidx;
  struct pal_in4_addr z_addr;
  struct interface *igi_ifp;

  IGMP_FN_ENTER (GroupRecord GetNext);  

  pal_mem_cpy(&cache_addr, passed_group_addr, IN_ADDR_SIZE);
  pal_mem_set(&z_addr, 0, IN_ADDR_SIZE);
  igr = NULL;

  /* If a group has been passed ,see if any higher ifindexes has the same group.
    If not,find the next highest group in all the ifindexes */
  if (!(IPV4_ADDR_SAME(&cache_addr,&z_addr)))
    {
      igi_ifp = ifv_lookup_next_by_index (&igi->igi_owning_ivrf->ifv,
                                          *passed_ifindex);
      while (igi_ifp)
        {
          (*passed_ifindex)++;

          igifidx.igifidx_ifp = igi_ifp;
          igifidx.igifidx_sid = igmp_if_svc_reg_su_id (igi, igi_ifp);
            
          *igif = igmp_if_lookup(igi, &igifidx);
          if (*igif)
            {
              igr = igmp_if_grec_lookup(*igif, &cache_addr);

              if (igr != NULL)
                break;
            }
          igi_ifp = ifv_lookup_by_index (&igi->igi_owning_ivrf->ifv,
              *passed_ifindex);
        }
        
      if (!igr)
        {
          *passed_ifindex = 0;
          igr = igmp_if_find_next_highest_group (igi,passed_ifindex,
                                                 &cache_addr, igif);
        }
    }
  else if (IPV4_ADDR_SAME(&cache_addr,&z_addr))
    { /*No group has been passed*/
      /*Since cache address is 0,this function will find the smallest group*/ 
      igr = igmp_if_find_next_highest_group (igi, passed_ifindex,
                                             &cache_addr, igif);
    }

  IGMP_FN_EXIT (igr);
}


s_int32_t
igmp_if_cache_last_reporter_get_next (struct igmp_instance *igi,
                                      struct interface **pifp,
                                      struct igmp_snmp_rtr_cache_index *index,
                                      u_int8_t **ret_var,
                                      pal_size_t *ret_var_len)
{
  struct pal_in4_addr cache_addr;
  struct igmp_group_rec *igr;
  struct igmp_if_idx igifidx;
  struct igmp_if *igif;
  s_int32_t ret;

  IGMP_FN_ENTER (CacheLastReporter GetNext);

  ret = IGMP_ERR_NONE;
  *ret_var_len = 0;
  *ret_var = NULL;
  igif = NULL;
  igr = NULL;

  index->qtype = 1;

  if (! igi)
    {
      ret = IGMP_ERR_INVALID_VALUE;
      *pifp = NULL;
      goto EXIT;
    }

  if (*pifp)
    {
      if (INTF_TYPE_L2 (*pifp))
        {
          ret = IGMP_ERR_L2_PHYSICAL_IF;
          goto EXIT;
        }
    }

  igifidx.igifidx_ifp = *pifp;
  igifidx.igifidx_sid = igmp_if_svc_reg_su_id (igi, *pifp);

  if ( index->ifindex != 0 )
    igif = igmp_if_get (igi, &igifidx);
  else if ( index->ifindex == 0 )
    igif = igmp_if_lookup_next (igi, &igifidx);

  if (! igif)
    {
      ret = IGMP_ERR_NO_SUCH_IFF;
      *pifp = NULL;
      goto EXIT;
    }

  igr = igmp_if_table_get_next(igi,&index->ifindex, &index->cache_addr , &igif);

  if (igr)
    {
      pal_mem_cpy (&cache_addr, igr->igr_owning_pn->key,
                   sizeof (igr->igr_owning_pn->key_len));
      pal_mem_cpy(&index->cache_addr, &cache_addr,IN_ADDR_SIZE);

      *ret_var = (u_int8_t *) &(igr->igr_last_reporter);
      *ret_var_len = sizeof (struct pal_in4_addr);
      index->ifindex = igr->igr_owning_igif->igif_owning_ifp->ifindex;
      *pifp  = ifv_lookup_by_index (LIB_VRF_GET_IF_MASTER (igi->igi_owning_ivrf),
                                   index->ifindex);
    }
  else
    {
      ret = IGMP_ERR_NO_SUCH_GROUP_REC;
      *pifp = NULL;
      goto EXIT;
    }

EXIT:

  IGMP_FN_EXIT (ret);
}

s_int32_t
igmp_if_cache_uptime_get (struct igmp_instance *igi,
                          struct interface **pifp,
                          struct igmp_snmp_rtr_cache_index *index,
                          u_int8_t **ret_var,
                          pal_size_t *ret_var_len)
{
  struct igmp_if_idx igifidx;
  struct igmp_group_rec *igr;
  struct igmp_if *igif;
  s_int32_t ret;

  IGMP_FN_ENTER (CacheUptime Get);

  ret = IGMP_ERR_NONE;
  *ret_var_len = 0;
  *ret_var = NULL;
  igif = NULL;

  if (! igi || ! *pifp)
    {
      ret = IGMP_ERR_INVALID_VALUE;
      goto EXIT;
    }

  if (INTF_TYPE_L2 (*pifp))
    {
      ret = IGMP_ERR_L2_PHYSICAL_IF;
      goto EXIT;
    }

  igifidx.igifidx_ifp = *pifp;
  igifidx.igifidx_sid = igmp_if_svc_reg_su_id (igi, *pifp);

  igif = igmp_if_lookup (igi, &igifidx);

  if (! igif)
    {
      ret = IGMP_ERR_NO_SUCH_IFF;
      goto EXIT;
    }

  igr = igmp_if_grec_lookup (igif, &index->cache_addr);

  if (! igr)
    {
      ret = IGMP_ERR_API_GET;
      goto EXIT;
    }

  igif->igif_snmp_ret_var = pal_time_current(NULL) - igr->igr_uptime;
  *ret_var = (u_int8_t *) &(igif->igif_snmp_ret_var);
  *ret_var_len = sizeof (pal_time_t);
  pal_mem_cpy(&index->cache_addr, igr->igr_owning_pn->key,
                                  sizeof (igr->igr_owning_pn->key_len));

EXIT:

  IGMP_FN_EXIT (ret);
}

s_int32_t
igmp_if_cache_uptime_get_next (struct igmp_instance *igi,
                               struct interface **pifp,
                               struct igmp_snmp_rtr_cache_index *index,
                               u_int8_t **ret_var,
                               pal_size_t *ret_var_len)
{
  struct pal_in4_addr cache_addr;
  struct igmp_if_idx igifidx;
  struct igmp_group_rec *igr;
  struct igmp_if *igif;
  s_int32_t ret;

  IGMP_FN_ENTER (CacheUptime GetNext);

  ret = IGMP_ERR_NONE;
  index->qtype = 1;
  *ret_var_len = 0;
  *ret_var = NULL;
  igif = NULL;
  igr = NULL;

  if (! igi)
    {
      ret = IGMP_ERR_INVALID_VALUE;
      *pifp = NULL;
      goto EXIT;
    }

  if (*pifp)
    {
      if (INTF_TYPE_L2 (*pifp))
        {
          ret = IGMP_ERR_L2_PHYSICAL_IF;
          goto EXIT;
        }
    }

  igifidx.igifidx_ifp = *pifp;
  igifidx.igifidx_sid = igmp_if_svc_reg_su_id (igi, *pifp);

  if ( index->ifindex != 0 )
    igif = igmp_if_get (igi, &igifidx);
  else if ( index->ifindex == 0 )
    igif = igmp_if_lookup_next (igi, &igifidx);

  if (! igif)
    {
      ret = IGMP_ERR_NO_SUCH_IFF;
      *pifp = NULL;
      goto EXIT;
    }

  igr = igmp_if_table_get_next(igi,&index->ifindex, &index->cache_addr, &igif);

  if (igr)
  {
    igif->igif_snmp_ret_var = pal_time_current(NULL) - igr->igr_uptime;
    *ret_var = (u_int8_t *) &(igif->igif_snmp_ret_var);
    *ret_var_len = sizeof (pal_time_t);
    index->ifindex = igr->igr_owning_igif->igif_owning_ifp->ifindex;
    *pifp  = ifv_lookup_by_index (LIB_VRF_GET_IF_MASTER (igi->igi_owning_ivrf),
                                   index->ifindex);
    pal_mem_cpy(&cache_addr, igr->igr_owning_pn->key, 
                sizeof (igr->igr_owning_pn->key_len));
    pal_mem_cpy(&index->cache_addr, &cache_addr, IN_ADDR_SIZE);
  }
  else
  {
    ret = IGMP_ERR_NO_SUCH_GROUP_REC;
    *pifp = NULL;
    goto EXIT;
  }
EXIT:

  IGMP_FN_EXIT (ret);
}

s_int32_t
igmp_if_cache_expiry_time_get (struct igmp_instance *igi,
                               struct interface **pifp,
                               struct igmp_snmp_rtr_cache_index *index,
                               u_int8_t **ret_var,
                               pal_size_t *ret_var_len)
{
  struct igmp_if_idx igifidx;
  struct igmp_group_rec *igr;
  struct igmp_if *igif;
  s_int32_t ret;

  IGMP_FN_ENTER (CacheExpiryTime Get);

  ret = IGMP_ERR_NONE;
  *ret_var_len = 0;
  *ret_var = NULL;
  igif = NULL;

  if (! igi || ! *pifp)
    {
      ret = IGMP_ERR_INVALID_VALUE;
      goto EXIT;
    }

  if (INTF_TYPE_L2 (*pifp))
    {
      ret = IGMP_ERR_L2_PHYSICAL_IF;
      goto EXIT;
    }

  igifidx.igifidx_ifp = *pifp;
  igifidx.igifidx_sid = igmp_if_svc_reg_su_id (igi, *pifp);

  igif = igmp_if_lookup (igi, &igifidx);

  if (! igif)
    {
      ret = IGMP_ERR_NO_SUCH_IFF;
      goto EXIT;
    }

  igr = igmp_if_grec_lookup (igif, &index->cache_addr);

  if (! igr)
    {
      ret = IGMP_ERR_API_GET;
      goto EXIT;
    }

  igif->igif_snmp_ret_var = thread_timer_remain_second (igr->t_igr_liveness);
  *ret_var = (u_int8_t *) &(igif->igif_snmp_ret_var);
  *ret_var_len = sizeof (pal_time_t);
  pal_mem_cpy(&index->cache_addr, igr->igr_owning_pn->key, sizeof (igr->igr_owning_pn->key_len));


EXIT:

  IGMP_FN_EXIT (ret);

}

s_int32_t
igmp_if_cache_expiry_time_get_next (struct igmp_instance *igi,
                                    struct interface **pifp,
                                    struct igmp_snmp_rtr_cache_index *index,
                                    u_int8_t **ret_var,
                                    pal_size_t *ret_var_len)
{
  struct pal_in4_addr cache_addr;
  struct igmp_if_idx igifidx;
  struct igmp_group_rec *igr;
  struct igmp_if *igif;
  s_int32_t ret;

  IGMP_FN_ENTER (CacheExpiryTime GetNext);

  ret = IGMP_ERR_NONE;
  index->qtype = 1;
  *ret_var_len = 0;
  *ret_var = NULL;
  igif = NULL;
  igr = NULL;

  if (! igi)
    {
      ret = IGMP_ERR_INVALID_VALUE;
      *pifp = NULL;
      goto EXIT;
    }

  if (*pifp)
    {
      if (INTF_TYPE_L2 (*pifp))
        {
          ret = IGMP_ERR_L2_PHYSICAL_IF;
          goto EXIT;
        }
    }

  igifidx.igifidx_ifp = *pifp;
  igifidx.igifidx_sid = igmp_if_svc_reg_su_id (igi, *pifp);

  if ( index->ifindex != 0 )
    igif = igmp_if_get (igi, &igifidx);
  else if ( index->ifindex == 0 )


    igif = igmp_if_lookup_next (igi, &igifidx);

  if (! igif)
    {
      ret = IGMP_ERR_NO_SUCH_IFF;
      *pifp = NULL;
      goto EXIT;
    }

  igr = igmp_if_table_get_next(igi,&index->ifindex, &index->cache_addr, &igif);

  if (igr)
   {
     pal_mem_cpy (&cache_addr, igr->igr_owning_pn->key, 
                  sizeof (igr->igr_owning_pn->key_len));
     pal_mem_cpy (&index->cache_addr, &cache_addr, IN_ADDR_SIZE);

     igif->igif_snmp_ret_var = thread_timer_remain_second (igr->t_igr_liveness);
     *ret_var = (u_int8_t *) &(igif->igif_snmp_ret_var);
     *ret_var_len = sizeof (pal_time_t);
     index->ifindex = (igr->igr_owning_igif->igif_idx.igifidx_ifp->ifindex);
     *pifp  = ifv_lookup_by_index (LIB_VRF_GET_IF_MASTER (igi->igi_owning_ivrf),
                                   index->ifindex);
   }
  else 
   {
     ret = IGMP_ERR_NO_SUCH_GROUP_REC;
     *pifp = NULL;
     goto EXIT;
   }

EXIT:

  IGMP_FN_EXIT (ret);
}

s_int32_t
igmp_if_cache_exclmode_exp_timer_get (struct igmp_instance *igi,
                                      struct interface **pifp,
                                      struct igmp_snmp_rtr_cache_index *index,
                                      u_int8_t **ret_var,
                                      pal_size_t *ret_var_len)
{
  struct igmp_if_idx igifidx;
  struct igmp_group_rec *igr;
  struct igmp_if *igif;
  s_int32_t ret;

  IGMP_FN_ENTER (CacheExclModeExpiryTimer Get);

  ret = IGMP_ERR_NONE;
  *ret_var_len = 0;
  *ret_var = NULL;
  igif = NULL;

  if (! igi || ! *pifp)
    {
      ret = IGMP_ERR_INVALID_VALUE;
      goto EXIT;
    }

  if (INTF_TYPE_L2 (*pifp))
    {
      ret = IGMP_ERR_L2_PHYSICAL_IF;
      goto EXIT;
    }

  igifidx.igifidx_ifp = *pifp;
  igifidx.igifidx_sid = igmp_if_svc_reg_su_id (igi, *pifp);

  igif = igmp_if_lookup (igi, &igifidx);

  if (! igif)
    {
      ret = IGMP_ERR_NO_SUCH_IFF;
      goto EXIT;
    }

  igr = igmp_if_grec_lookup (igif, &index->cache_addr);

  if (! igr)
    {
      ret = IGMP_ERR_API_GET;
      goto EXIT;
    }

  igif->igif_snmp_ret_var = thread_timer_remain_second (igr->t_igr_liveness);
  *ret_var = (u_int8_t *) &(igif->igif_snmp_ret_var);
  *ret_var_len = sizeof (pal_time_t);
  pal_mem_cpy(&index->cache_addr, igr->igr_owning_pn->key, sizeof (igr->igr_owning_pn->key_len));

EXIT:

  IGMP_FN_EXIT (ret);
}

s_int32_t
igmp_if_cache_exclmode_exp_timer_get_next (struct igmp_instance *igi,
                                           struct interface **pifp,
                                           struct igmp_snmp_rtr_cache_index *index,
                                           u_int8_t **ret_var,
                                           pal_size_t *ret_var_len)
{
  struct pal_in4_addr cache_addr;
  struct igmp_group_rec *igr;
  struct igmp_if_idx igifidx;
  struct igmp_if *igif;
  s_int32_t ret;

  IGMP_FN_ENTER (CacheExclModeExpiryTimer GetNext);

  ret = IGMP_ERR_NONE;
  index->qtype = 1;
  *ret_var_len = 0;
  *ret_var = NULL;
  igif = NULL;
  igr = NULL;

  if (! igi)
    {
      ret = IGMP_ERR_INVALID_VALUE;
      *pifp = NULL;
      goto EXIT;
    }

  if (*pifp)
    {
      if (INTF_TYPE_L2 (*pifp))
        {
          ret = IGMP_ERR_L2_PHYSICAL_IF;
          goto EXIT;
        }
    }

  igifidx.igifidx_ifp = *pifp;
  igifidx.igifidx_sid = igmp_if_svc_reg_su_id (igi, *pifp);

  if ( index->ifindex != 0 )
    igif = igmp_if_get (igi, &igifidx);
  else if ( index->ifindex == 0 )

    igif = igmp_if_lookup_next (igi, &igifidx);

  if (! igif)
    {
      ret = IGMP_ERR_NO_SUCH_IFF;
      *pifp = NULL;
      goto EXIT;
    }

  igr = igmp_if_table_get_next(igi,&index->ifindex, &index->cache_addr, &igif);

 if (igr)
   {
     igif->igif_snmp_ret_var = thread_timer_remain_second (igr->t_igr_liveness);
     *ret_var = (u_int8_t *) &(igif->igif_snmp_ret_var);
     *ret_var_len = sizeof (pal_time_t);
     pal_mem_cpy(&cache_addr, igr->igr_owning_pn->key, sizeof (igr->igr_owning_pn->key_len));
     index->cache_addr = cache_addr;
     index->ifindex = igr->igr_owning_igif->igif_idx.igifidx_ifp->ifindex;
     *pifp  = ifv_lookup_by_index (LIB_VRF_GET_IF_MASTER (igi->igi_owning_ivrf),
                                   index->ifindex);
     pal_mem_cpy(&index->cache_addr, &cache_addr, sizeof (struct pal_in4_addr));
   }
  else
   {
      ret = IGMP_ERR_NO_SUCH_GROUP_REC;
      *pifp = NULL;
      goto EXIT;
   }

EXIT:

  IGMP_FN_EXIT (ret);
}

s_int32_t
igmp_if_cache_ver1_host_timer_get (struct igmp_instance *igi,
                                   struct interface **pifp,
                                   struct igmp_snmp_rtr_cache_index *index,
                                   u_int8_t **ret_var,
                                   pal_size_t *ret_var_len)
{
  struct igmp_if_idx igifidx;
  struct igmp_group_rec *igr;
  struct igmp_if *igif;
  s_int32_t ret;

  IGMP_FN_ENTER (CacheVer1HostTimer Get);

  ret = IGMP_ERR_NONE;
  *ret_var_len = 0;
  *ret_var = NULL;
  igif = NULL;

  if (! igi || ! *pifp)
    {
      ret = IGMP_ERR_INVALID_VALUE;
      goto EXIT;
    }

  if (INTF_TYPE_L2 (*pifp))
    {
      ret = IGMP_ERR_L2_PHYSICAL_IF;
      goto EXIT;
    }

  igifidx.igifidx_ifp = *pifp;
  igifidx.igifidx_sid = igmp_if_svc_reg_su_id (igi, *pifp);

  igif = igmp_if_lookup (igi, &igifidx);

  if (! igif)
    {
      ret = IGMP_ERR_NO_SUCH_IFF;
      goto EXIT;
    }

  igr = igmp_if_grec_lookup (igif, &index->cache_addr);

  if (! igr)
    {
      ret = IGMP_ERR_API_GET;
      goto EXIT;
    }

  igif->igif_snmp_ret_var = thread_timer_remain_second (
                            igr->t_igr_v1_host_present);
  *ret_var = (u_int8_t *) &(igif->igif_snmp_ret_var);
  *ret_var_len = sizeof (pal_time_t);

EXIT:

  IGMP_FN_EXIT (ret);
}

s_int32_t
igmp_if_cache_ver1_host_timer_get_next (struct igmp_instance *igi,
                                        struct interface **pifp,
                                        struct igmp_snmp_rtr_cache_index *index,
                                        u_int8_t **ret_var,
                                        pal_size_t *ret_var_len)
{
  struct pal_in4_addr cache_addr;
  struct igmp_if_idx igifidx;
  struct igmp_group_rec *igr;
  struct igmp_if *igif;
  s_int32_t ret;

  IGMP_FN_ENTER (CacheVer1HostTimer GetNext);

  ret = IGMP_ERR_NONE;
  index->qtype = 1;
  *ret_var_len = 0;
  *ret_var = NULL;
  igif = NULL;
  igr = NULL;

  if (! igi)
    {
      ret = IGMP_ERR_INVALID_VALUE;
      *pifp = NULL;
      goto EXIT;
    }

  if (*pifp)
    {
      if (INTF_TYPE_L2 (*pifp))
        {
          ret = IGMP_ERR_L2_PHYSICAL_IF;
          goto EXIT;
        }
    }

  igifidx.igifidx_ifp = *pifp;
  igifidx.igifidx_sid = igmp_if_svc_reg_su_id (igi, *pifp);

  if ( index->ifindex != 0 )
    igif = igmp_if_get (igi, &igifidx);
  else if ( index->ifindex == 0 )

    igif = igmp_if_lookup_next (igi, &igifidx);

  if (! igif)
    {
      ret = IGMP_ERR_NO_SUCH_IFF;
      *pifp = NULL;
      goto EXIT;
    }

  igr = igmp_if_table_get_next(igi,&index->ifindex, &index->cache_addr, &igif);

 if (igr)
  {
    igif->igif_snmp_ret_var = thread_timer_remain_second (
                            igr->t_igr_v1_host_present);
    *ret_var = (u_int8_t *) &(igif->igif_snmp_ret_var);
    *ret_var_len = sizeof (pal_time_t);

   pal_mem_cpy(&cache_addr, igr->igr_owning_pn->key, sizeof (igr->igr_owning_pn->key_len));
   index->cache_addr = cache_addr;
   index->ifindex = igr->igr_owning_igif->igif_idx.igifidx_ifp->ifindex;
   *pifp = ifv_lookup_next_by_index (&igi->igi_owning_ivrf->ifv,
                                     index->ifindex);
   pal_mem_cpy(&index->cache_addr, &cache_addr, sizeof (struct pal_in4_addr));
  }
  else
   {
     ret = IGMP_ERR_NO_SUCH_GROUP_REC;
     *pifp = NULL;
      goto EXIT;
   }

EXIT:

  IGMP_FN_EXIT (ret);
}

s_int32_t
igmp_if_cache_ver2_host_timer_get (struct igmp_instance *igi,
                                   struct interface **pifp,
                                   struct igmp_snmp_rtr_cache_index *index,
                                   u_int8_t **ret_var,
                                   pal_size_t *ret_var_len)
{
  struct igmp_if_idx igifidx;
  struct igmp_group_rec *igr;
  struct igmp_if *igif;
  s_int32_t ret;

  IGMP_FN_ENTER (CacheVer2HostTimer Get);

  ret = IGMP_ERR_NONE;
  *ret_var_len = 0;
  *ret_var = NULL;
  igif = NULL;

  if (! igi || ! *pifp)
    {
      ret = IGMP_ERR_INVALID_VALUE;
      goto EXIT;
    }

  if (INTF_TYPE_L2 (*pifp))
    {
      ret = IGMP_ERR_L2_PHYSICAL_IF;
      goto EXIT;
    }

  igifidx.igifidx_ifp = *pifp;
  igifidx.igifidx_sid = igmp_if_svc_reg_su_id (igi, *pifp);

  igif = igmp_if_lookup (igi, &igifidx);

  if (! igif)
    {
      ret = IGMP_ERR_NO_SUCH_IFF;
      goto EXIT;
    }

  igr = igmp_if_grec_lookup (igif, &index->cache_addr);

  if (! igr)
    {
      ret = IGMP_ERR_API_GET;
      goto EXIT;
    }

  igif->igif_snmp_ret_var = thread_timer_remain_second (
                            igr->t_igr_v2_host_present);
  *ret_var = (u_int8_t *) &(igif->igif_snmp_ret_var);
  *ret_var_len = sizeof (pal_time_t);

EXIT:

  IGMP_FN_EXIT (ret);
}

s_int32_t
igmp_if_cache_ver2_host_timer_get_next (struct igmp_instance *igi,
                                        struct interface **pifp,
                                        struct igmp_snmp_rtr_cache_index *index,
                                        u_int8_t **ret_var,
                                        pal_size_t *ret_var_len)
{
  struct pal_in4_addr cache_addr;
  struct igmp_if_idx igifidx;
  struct igmp_group_rec *igr;
  struct igmp_if *igif;
  s_int32_t ret;

  IGMP_FN_ENTER (CacheVer2HostTimer GetNext);

  ret = IGMP_ERR_NONE;
  index->qtype = 1;
  *ret_var_len = 0;
  *ret_var = NULL;
  igif = NULL;
  igr = NULL;

  if (! igi)
    {
      ret = IGMP_ERR_INVALID_VALUE;
      *pifp = NULL;
      goto EXIT;
    }

  if (*pifp)
    {
      if (INTF_TYPE_L2 (*pifp))
        {
          ret = IGMP_ERR_L2_PHYSICAL_IF;
          goto EXIT;
        }
    }

  igifidx.igifidx_ifp = *pifp;
  igifidx.igifidx_sid = igmp_if_svc_reg_su_id (igi, *pifp);

  if ( index->ifindex != 0 )
    igif = igmp_if_get (igi, &igifidx);
  else if ( index->ifindex == 0 )

    igif = igmp_if_lookup_next (igi, &igifidx);

  if (! igif)
    {
      ret = IGMP_ERR_NO_SUCH_IFF;
      *pifp = NULL;
      goto EXIT;
    }

  igr = igmp_if_table_get_next(igi,&index->ifindex, &index->cache_addr, &igif);

 if (igr)
  {
    igif->igif_snmp_ret_var = thread_timer_remain_second (
                            igr->t_igr_v2_host_present);
    *ret_var = (u_int8_t *) &(igif->igif_snmp_ret_var);
    *ret_var_len = sizeof (pal_time_t);

    pal_mem_cpy(&cache_addr, igr->igr_owning_pn->key, sizeof (igr->igr_owning_pn->key_len));
    index->cache_addr = cache_addr;
    index->ifindex = igr->igr_owning_igif->igif_idx.igifidx_ifp->ifindex;
    *pifp  = ifv_lookup_by_index (LIB_VRF_GET_IF_MASTER (igi->igi_owning_ivrf),
                                  index->ifindex);
    pal_mem_cpy(&index->cache_addr, &cache_addr, sizeof (struct pal_in4_addr));
  }
  else
  {
    ret = IGMP_ERR_NO_SUCH_GROUP_REC;
    *pifp = NULL;
    goto EXIT;
  }


EXIT:

  IGMP_FN_EXIT (ret);
}

s_int32_t
igmp_if_cache_src_filter_mode_get (struct igmp_instance *igi,
                                   struct interface **pifp,
                                   struct igmp_snmp_rtr_cache_index *index,
                                   u_int8_t **ret_var,
                                   pal_size_t *ret_var_len)
{
  struct igmp_if_idx igifidx;
  struct igmp_group_rec *igr;
  struct igmp_if *igif;
  s_int32_t ret;

  IGMP_FN_ENTER (CacheSrcFilterMode Get);

  ret = IGMP_ERR_NONE;
  *ret_var_len = 0;
  *ret_var = NULL;
  igif = NULL;

  if (! igi || ! *pifp)
    {
      ret = IGMP_ERR_INVALID_VALUE;
      goto EXIT;
    }

  if (INTF_TYPE_L2 (*pifp))
    {
      ret = IGMP_ERR_L2_PHYSICAL_IF;
      goto EXIT;
    }

  igifidx.igifidx_ifp = *pifp;
  igifidx.igifidx_sid = igmp_if_svc_reg_su_id (igi, *pifp);

    igif = igmp_if_lookup (igi, &igifidx);

  if (! igif)
    {
      ret = IGMP_ERR_NO_SUCH_IFF;
      goto EXIT;
    }

  igr = igmp_if_grec_lookup (igif, &index->cache_addr);

  if (! igr)
    {
      ret = IGMP_ERR_API_GET;
      goto EXIT;
    }

  *ret_var = (u_int8_t *) &(igr->igr_filt_mode_state);
  *ret_var_len = sizeof (pal_time_t);

EXIT:

  IGMP_FN_EXIT (ret);
}

s_int32_t
igmp_if_cache_src_filter_mode_get_next (struct igmp_instance *igi,
                                        struct interface **pifp,
                                        struct igmp_snmp_rtr_cache_index *index,
                                        u_int8_t **ret_var,
                                        pal_size_t *ret_var_len)
{
  struct pal_in4_addr cache_addr;
  struct igmp_if_idx igifidx;
  struct igmp_group_rec *igr;
  struct igmp_if *igif;
  s_int32_t ret;

  IGMP_FN_ENTER (CacheSrcFilterMode GetNext);

  ret = IGMP_ERR_NONE;
  index->qtype = 1;
  *ret_var_len = 0;
  *ret_var = NULL;
  igif = NULL;
  igr = NULL;

  if (! igi)
    {
      ret = IGMP_ERR_INVALID_VALUE;
      *pifp = NULL;
      goto EXIT;
    }

  if (*pifp)
    {
      if (INTF_TYPE_L2 (*pifp))
        {
          ret = IGMP_ERR_L2_PHYSICAL_IF;
          goto EXIT;
        }
    }

  igifidx.igifidx_ifp = *pifp;
  igifidx.igifidx_sid = igmp_if_svc_reg_su_id (igi, *pifp);

  if ( index->ifindex != 0 )
    igif = igmp_if_get (igi, &igifidx);
  else if ( index->ifindex == 0 )

    igif = igmp_if_lookup_next (igi, &igifidx);

  if (! igif)
    {
      ret = IGMP_ERR_NO_SUCH_IFF;
      *pifp = NULL;
      goto EXIT;
    }

  igr = igmp_if_table_get_next(igi,&index->ifindex, &index->cache_addr, &igif);

 if (igr)
  {
    *ret_var = (u_int8_t *) &(igr->igr_filt_mode_state);
    *ret_var_len = sizeof (pal_time_t);

   pal_mem_cpy(&cache_addr, igr->igr_owning_pn->key, sizeof (igr->igr_owning_pn->key_len));
   index->cache_addr = cache_addr;
   index->ifindex = igr->igr_owning_igif->igif_idx.igifidx_ifp->ifindex;
   *pifp  = ifv_lookup_by_index (LIB_VRF_GET_IF_MASTER (igi->igi_owning_ivrf),
                                 index->ifindex);
   pal_mem_cpy(&index->cache_addr, &cache_addr, sizeof (struct pal_in4_addr));
  }
  else
   {
      ret = IGMP_ERR_NO_SUCH_GROUP_REC;
      *pifp = NULL;
       goto EXIT;
   }

EXIT:

  IGMP_FN_EXIT (ret);
}

s_int32_t
igmp_if_inv_cache_address_get (struct igmp_instance *igi,
                               struct interface **pifp,
                               struct igmp_snmp_inv_rtr_index *index,
                               u_int8_t **ret_var,
                               pal_size_t *ret_var_len)
{
  struct igmp_if_idx igifidx;
  struct igmp_group_rec *igr;
  struct igmp_if *igif;
  s_int32_t ret;

  IGMP_FN_ENTER (InvCacheAddress Get);

  ret = IGMP_ERR_NONE;
  *ret_var_len = 0;
  *ret_var = NULL;
  igif = NULL;

  if (! igi || ! *pifp)
    {
      ret = IGMP_ERR_INVALID_VALUE;
      goto EXIT;
    }

  if (INTF_TYPE_L2 (*pifp))
    {
      ret = IGMP_ERR_L2_PHYSICAL_IF;
      goto EXIT;
    }

  igifidx.igifidx_ifp = *pifp;
  igifidx.igifidx_sid = igmp_if_svc_reg_su_id (igi, *pifp);

  igif = igmp_if_lookup (igi, &igifidx);

  if (! igif)
    {
      ret = IGMP_ERR_NO_SUCH_IFF;
      goto EXIT;
    }

  igr = igmp_if_grec_lookup (igif, &index->cache_addr);
  if (! igr)
    {
      ret = IGMP_ERR_API_GET;
      goto EXIT;
    }

  *ret_var = (u_int8_t *) &index->cache_addr;
  *ret_var_len = sizeof (struct pal_in4_addr);

EXIT:

  IGMP_FN_EXIT (ret);
}

s_int32_t
igmp_if_inv_cache_address_get_next (struct igmp_instance *igi,
                                    struct interface **pifp,
                                    struct igmp_snmp_inv_rtr_index *index,
                                    u_int8_t **ret_var,
                                    pal_size_t *ret_var_len)
{
  struct igmp_group_rec *igr;
  struct igmp_if_idx igifidx;
  struct igmp_if *igif;
  s_int32_t ret;
  struct interface *igi_ifp;
  struct pal_in4_addr cache_addr;

  IGMP_FN_ENTER (InvCacheAddress GetNext);

  pal_mem_set(&cache_addr, 0, sizeof(struct pal_in4_addr));
  ret = IGMP_ERR_NONE;
  *ret_var_len = 0;
  *ret_var = NULL;
  igif = NULL;

  if (! igi)
    {
      ret = IGMP_ERR_INVALID_VALUE;
      *pifp = NULL;
      goto EXIT;
    }

  if (*pifp)
    {
      if (INTF_TYPE_L2 (*pifp))
        {
          ret = IGMP_ERR_L2_PHYSICAL_IF;
          goto EXIT;
        }
    }

  igifidx.igifidx_ifp = *pifp;
  igifidx.igifidx_sid = igmp_if_svc_reg_su_id (igi, *pifp);

  if ( index->ifindex != 0 )
    igif = igmp_if_get (igi, &igifidx);
  else if ( index->ifindex == 0 )


    igif = igmp_if_lookup_next (igi, &igifidx);

  if (!igif)
    {
     ret = IGMP_ERR_INVALID_VALUE;
     *pifp = NULL;
     goto EXIT;
    }

  cache_addr = index->cache_addr;
  while(!(igr = igmp_if_grec_lookup_next (igif, &cache_addr)))
   {
      igi_ifp = ifv_lookup_by_index (LIB_VRF_GET_IF_MASTER (igi->igi_owning_ivrf),
                                     ++(index->ifindex));
      if (!igi_ifp)
        break;

      igifidx.igifidx_ifp = igi_ifp;
      igifidx.igifidx_sid = igmp_if_svc_reg_su_id (igi, igi_ifp);
      igif = igmp_if_lookup(igi, &igifidx);
      pal_mem_set(&cache_addr, 0, sizeof(struct pal_in4_addr));
   }

 if (igr)
  {
   pal_mem_cpy(&cache_addr, igr->igr_owning_pn->key, sizeof (igr->igr_owning_pn->key_len)); 
   *ret_var = (u_int8_t *) &igr->igr_owning_pn->key;
   *ret_var_len = sizeof (igr->igr_owning_pn->key_len);
   *pifp = igifidx.igifidx_ifp;

   index->ifindex = igr->igr_owning_igif->igif_idx.igifidx_ifp->ifindex ;
   pal_mem_cpy(&index->cache_addr, &cache_addr, sizeof (struct pal_in4_addr));
  }
 else
  {
    ret = IGMP_ERR_INVALID_VALUE;
    *pifp = NULL; 
  }

EXIT:

  IGMP_FN_EXIT (ret);
}

s_int32_t
igmp_if_srclist_host_address_get (struct igmp_instance *igi,
                                  struct interface **pifp,
                                  struct pal_in4_addr *group_addr,
                                  struct pal_in4_addr *src_addr,
                                  struct igmp_snmp_rtr_src_list_index *index,
                                  u_int8_t **ret_var,
                                  pal_size_t *ret_var_len)

{
  struct igmp_source_rec *isr;
  struct igmp_if_idx igifidx;
  struct igmp_group_rec *igr;
  struct igmp_if *igif;
  s_int32_t ret;

  IGMP_FN_ENTER (SrcListHostAddress Get);

  ret = IGMP_ERR_NONE;
  *ret_var_len = 0;
  *ret_var = NULL;
  igif = NULL;

  if (! igi || ! *pifp)
    {
      ret = IGMP_ERR_INVALID_VALUE;
      goto EXIT;
    }

  if (INTF_TYPE_L2 (*pifp))
    {
      ret = IGMP_ERR_L2_PHYSICAL_IF;
      goto EXIT;
    }

  igifidx.igifidx_ifp = *pifp;
  igifidx.igifidx_sid = igmp_if_svc_reg_su_id (igi, *pifp);

  igif = igmp_if_lookup (igi, &igifidx);

  if (! igif)
    {
      ret = IGMP_ERR_NO_SUCH_IFF;
      goto EXIT;
    }

  igr = igmp_if_grec_lookup (igif, group_addr);

  if (! igr)
    {
      ret = IGMP_ERR_API_GET;
      goto EXIT;
    }

  isr = igmp_if_srec_lookup (igr, PAL_TRUE, src_addr);
  if (! isr)
    isr = igmp_if_srec_lookup (igr, PAL_FALSE, src_addr);
  if (! isr)
    {
      ret = IGMP_ERR_API_GET;
      goto EXIT;
    }

  *ret_var = (u_int8_t *) PTREE_NODE_KEY (isr->isr_owning_pn);
  *ret_var_len = sizeof (struct pal_in4_addr);

EXIT:

  IGMP_FN_EXIT (ret);
}





s_int32_t
igmp_if_srclist_host_address_get_next (struct igmp_instance *igi,
                                       struct interface **pifp,
                                       struct pal_in4_addr *group_addr,
                                       struct pal_in4_addr *src_addr,
                                       struct igmp_snmp_rtr_src_list_index *index,
                                       u_int8_t **ret_var,
                                       pal_size_t *ret_var_len)
{
  struct igmp_source_rec *isr;
  struct igmp_group_rec *igr;
  struct igmp_if_idx igifidx;
  struct pal_in4_addr z_addr;
  struct pal_in4_addr *saddr;
  struct igmp_if *igif;
  s_int32_t ret;

  IGMP_FN_ENTER (SrcListHostAddress GetNext);

  pal_mem_set(&z_addr, 0, IN_ADDR_SIZE);
  ret = IGMP_ERR_NONE;
  *ret_var_len = 0;
  *ret_var = NULL;
  saddr = NULL;
  igif = NULL;
  isr = NULL;
  igr = NULL;

  if (! igi)
    {
      ret = IGMP_ERR_INVALID_VALUE;
      *pifp = NULL;
      pal_mem_set (group_addr, 0, sizeof (struct pal_in4_addr));
      pal_mem_set (src_addr, 0, sizeof (struct pal_in4_addr));
      goto EXIT;
    }
  igifidx.igifidx_ifp = *pifp;
  igifidx.igifidx_sid = igmp_if_svc_reg_su_id (igi, *pifp);

  if ( index->ifindex != 0 )
    igif = igmp_if_get (igi, &igifidx);
  else if ( index->ifindex == 0 || ( !igif ) )
    igif = igmp_if_lookup_next (igi, &igifidx);

  if (! igif)
    {
      ret = IGMP_ERR_NO_SUCH_IFF;
      *pifp = NULL;
      pal_mem_set (group_addr, 0, sizeof (struct pal_in4_addr));
      pal_mem_set (src_addr, 0, sizeof (struct pal_in4_addr));
      goto EXIT;
    }

  saddr = src_addr;

/***************************************
1.If there is a source address,then find thee next source address for the same group
If you cannot find this,then look for the next group/index combo calling a function 
After finding the next group/index find the first source on that and return,
or keep parsing through group index combos till u find a source

2.If there is no source address present initially,
  Check if there  is a valid group/index combo,and find a source on that.
  Else parse through next group/index combos,till you find a source
         
****************************************/
  if (!(IPV4_ADDR_SAME(saddr,&z_addr)))
    {      
      igr = igmp_if_grec_lookup(igif, group_addr);

      do {        
        if (igr)  
          {
            isr = igmp_if_srec_lookup_next (igr, saddr); 
            if (!isr)
              {
                saddr = NULL ;
                IPV4_ADDR_COPY (group_addr, 
                                PTREE_NODE_KEY (igr->igr_owning_pn));
              }
             else 
              break;
          }
        igr = igmp_if_table_get_next(igi,&index->ifindex,group_addr,&igif);
 
      } while(igr);     
    }
  else  if (IPV4_ADDR_SAME(saddr,&z_addr))
    {
      igr = igmp_if_grec_lookup (igif, group_addr);
       do {
         saddr = NULL ;
         if (igr)
           {
             isr = igmp_if_srec_lookup_next (igr, saddr);
             if (isr)
               break;
           }

         igr = igmp_if_table_get_next (igi,&index->ifindex,group_addr,&igif);
         if (igr)
           IPV4_ADDR_COPY (group_addr, PTREE_NODE_KEY (igr->igr_owning_pn));
       } while(igr);
   }

 if (isr)
   {
      *ret_var = (u_int8_t *) PTREE_NODE_KEY (isr->isr_owning_pn);
      *ret_var_len = IN_ADDR_SIZE;
      index->ifindex = igr->igr_owning_igif->igif_owning_ifp->ifindex;
      *pifp = ifv_lookup_by_index (LIB_VRF_GET_IF_MASTER (igi->igi_owning_ivrf),
                                   (index->ifindex));

      IPV4_ADDR_COPY (group_addr, PTREE_NODE_KEY (igr->igr_owning_pn));
      IPV4_ADDR_COPY (src_addr, PTREE_NODE_KEY (isr->isr_owning_pn));
   }
  else
   {
     ret = IGMP_ERR_API_GET;
     *pifp = NULL;
     pal_mem_set (group_addr, 0, sizeof (struct pal_in4_addr));
     pal_mem_set (src_addr, 0, sizeof (struct pal_in4_addr));
   }

EXIT:

  IGMP_FN_EXIT (ret);

}

s_int32_t
igmp_if_srclist_expiry_time_get (struct igmp_instance *igi,
                                 struct interface **pifp,
                                 struct pal_in4_addr *group_addr,
                                 struct pal_in4_addr *src_addr,
                                 struct igmp_snmp_rtr_src_list_index *index,
                                 u_int8_t **ret_var,
                                 pal_size_t *ret_var_len)
{
  struct igmp_source_rec *isr;
  struct igmp_if_idx igifidx;
  struct igmp_group_rec *igr;
  struct igmp_if *igif;
  s_int32_t ret;

  IGMP_FN_ENTER (SrcListExpTime Get);

  ret = IGMP_ERR_NONE;
  *ret_var_len = 0;
  *ret_var = NULL;
  igif = NULL;

  if (! igi || ! *pifp)
    {
      ret = IGMP_ERR_INVALID_VALUE;
      goto EXIT;
    }

  if (INTF_TYPE_L2 (*pifp))
    {
      ret = IGMP_ERR_L2_PHYSICAL_IF;
      goto EXIT;
    }

  igifidx.igifidx_ifp = *pifp;
  igifidx.igifidx_sid = igmp_if_svc_reg_su_id (igi, *pifp);

  igif = igmp_if_lookup (igi, &igifidx);

  if (! igif)
    {
      ret = IGMP_ERR_NO_SUCH_IFF;
      goto EXIT;
    }

  igr = igmp_if_grec_lookup (igif, group_addr);

  if (! igr)
    {
      ret = IGMP_ERR_API_GET;
      goto EXIT;
    }

  isr = igmp_if_srec_lookup (igr, PAL_TRUE, src_addr);
  if (isr)
    igif->igif_snmp_ret_var = thread_timer_remain_second (
                              isr->t_isr_liveness);
  else
    {
      isr = igmp_if_srec_lookup (igr, PAL_FALSE, src_addr);
      if (isr)
        igif->igif_snmp_ret_var = 0;
      else
        {
          ret = IGMP_ERR_API_GET;
          goto EXIT;
        }
    }

  *ret_var = (u_int8_t *) &igif->igif_snmp_ret_var;
  *ret_var_len = sizeof (pal_time_t);

EXIT:

  IGMP_FN_EXIT (ret);
}

s_int32_t
igmp_if_srclist_expiry_time_get_next (struct igmp_instance *igi,
                                      struct interface **pifp,
                                      struct pal_in4_addr *group_addr,
                                      struct pal_in4_addr *src_addr,
                                      struct igmp_snmp_rtr_src_list_index *index,
                                      u_int8_t **ret_var,
                                      pal_size_t *ret_var_len)
{
  struct igmp_source_rec *isr;
  struct igmp_group_rec *igr;
  struct igmp_if_idx igifidx;
  struct pal_in4_addr *saddr;
  struct pal_in4_addr z_addr;
  struct igmp_if *igif;
  s_int32_t ret;

  IGMP_FN_ENTER (SrcListExpTime GetNext);

  pal_mem_set(&z_addr, 0, IN_ADDR_SIZE);
  ret = IGMP_ERR_NONE;
  *ret_var_len = 0;
  *ret_var = NULL;
  saddr = NULL;
  igif = NULL;
  isr = NULL;
  igr = NULL;

  if (! igi)
    {
      ret = IGMP_ERR_INVALID_VALUE;
      *pifp = NULL;
      pal_mem_set (group_addr, 0, sizeof (struct pal_in4_addr));
      pal_mem_set (src_addr, 0, sizeof (struct pal_in4_addr));
      goto EXIT;
    }
  igifidx.igifidx_ifp = *pifp;
  igifidx.igifidx_sid = igmp_if_svc_reg_su_id (igi, *pifp);

  if ( index->ifindex != 0 )
    igif = igmp_if_get (igi, &igifidx);
  else if ( index->ifindex == 0 || ( !igif ) )
    igif = igmp_if_lookup_next (igi, &igifidx);

  if (! igif)
    {
      ret = IGMP_ERR_NO_SUCH_IFF;
      *pifp = NULL;
      pal_mem_set (group_addr, 0, sizeof (struct pal_in4_addr));
      pal_mem_set (src_addr, 0, sizeof (struct pal_in4_addr));
      goto EXIT;
    }

  saddr = src_addr;

  if (!(IPV4_ADDR_SAME(saddr,&z_addr)))
    {
      igr = igmp_if_grec_lookup (igif, group_addr);
      do {
        if (igr)
          {
            isr = igmp_if_srec_lookup_next (igr, saddr);
            if (!isr)
              {
                saddr = NULL ;
                IPV4_ADDR_COPY (group_addr, PTREE_NODE_KEY (igr->igr_owning_pn));
              }
            else 
              break;
          }

        igr = igmp_if_table_get_next (igi,&index->ifindex,group_addr,&igif);
      } while(igr);
    }
  else if (IPV4_ADDR_SAME(saddr,&z_addr))
    {
      igr = igmp_if_grec_lookup(igif, group_addr);
      do {
        saddr = NULL;
        if (igr)
          {
            isr = igmp_if_srec_lookup_next (igr, saddr);
            if (isr)
              break;
          }

        igr = igmp_if_table_get_next (igi,&index->ifindex,group_addr,&igif);

        if (igr)
          IPV4_ADDR_COPY (group_addr, PTREE_NODE_KEY (igr->igr_owning_pn));
      } while(igr);
   }

  if (isr)
    {
      igif->igif_snmp_ret_var = 
                  thread_timer_remain_second (isr->t_isr_liveness);
      *ret_var = (u_int8_t *) &igif->igif_snmp_ret_var;
      *ret_var_len = sizeof (pal_time_t);
      index->ifindex = igr->igr_owning_igif->igif_owning_ifp->ifindex;
      *pifp = ifv_lookup_by_index (LIB_VRF_GET_IF_MASTER (igi->igi_owning_ivrf),
                                   (index->ifindex));
      IPV4_ADDR_COPY (group_addr, PTREE_NODE_KEY (igr->igr_owning_pn));
      IPV4_ADDR_COPY (src_addr, PTREE_NODE_KEY (isr->isr_owning_pn));
    }
  else
    {
      ret = IGMP_ERR_API_GET;
      *pifp = NULL;
      pal_mem_set (group_addr, 0, sizeof (struct pal_in4_addr));
      pal_mem_set (src_addr, 0, sizeof (struct pal_in4_addr));
    }

EXIT:

  IGMP_FN_EXIT (ret);
}
#endif /* HAVE_SNMP */

/*
 * IGMP CLEAR CLI API
 */

/* IGMP Instance/IF Clear */
s_int32_t
igmp_clear (struct igmp_instance *igi,
            struct interface *ifp,
            struct pal_in4_addr *pgrp,
            struct pal_in4_addr *psrc)
{
  struct igmp_if_idx igifidx;
  struct igmp_if *igif;
  struct igmp_if *ds_igif;
  struct listnode *nn;
  s_int32_t ret;

  IGMP_FN_ENTER (IGMP Clear);

  ret = IGMP_ERR_NONE;
  igif = NULL;
  ds_igif = NULL;

  if (! igi)
    {
      ret = IGMP_ERR_INVALID_VALUE;
      goto EXIT;
    }

  if (ifp)
    {
      igifidx.igifidx_ifp = ifp;
      igifidx.igifidx_sid = igmp_if_svc_reg_su_id (igi, ifp);

      igif = igmp_if_lookup (igi, &igifidx);

      if (! igif)
        {
          ret = IGMP_ERR_NO_SUCH_IFF;
          goto EXIT;
        }
      if (if_is_vlan (ifp))
        {
          LIST_LOOP (&igif->igif_hst_chld_lst, ds_igif, nn)
            ret = igmp_if_clear (ds_igif, pgrp, psrc, PAL_FALSE);
        }
      else
        ret = igmp_if_clear (igif, pgrp, psrc, PAL_FALSE);
    }
  else
    {
      avl_traverse2 (igi->igi_if_tree,
                     igmp_if_avl_traverse_clear, pgrp, psrc);
    }

EXIT:

  IGMP_FN_EXIT (ret);
}

/*
 * IGMP Utility API
 */

/* IGMP Error Code Interpretation */
u_int8_t *
igmp_strerror (s_int32_t iret)
{
  u_int8_t *str;

  IGMP_FN_ENTER (Err to string);

  str = NULL;

  switch (iret)
    {
    case IGMP_ERR_NONE:
      str = NULL;
      break;
    case IGMP_ERR_GENERIC:
      str = "Operation failed";
      break;
    case IGMP_ERR_INVALID_COMMAND:
      str = "Invalid command";
      break;
    case IGMP_ERR_INVALID_VALUE:
      str = "Invalid value";
      break;
    case IGMP_ERR_INVALID_FLAG:
      str = "Invalid flag";
      break;
    case IGMP_ERR_INVALID_AF:
      str = "Invalid address-family";
      break;
    case IGMP_ERR_NO_SUCH_IFF:
      str = "No such interface configured";
      break;
    case IGMP_ERR_NO_SUCH_GROUP_REC:
      str = "No such Group-Rec found";
      break;
    case IGMP_ERR_NO_SUCH_SOURCE_REC:
      str = "No such Source-Rec found";
      break;
    case IGMP_ERR_NO_SUCH_SVC_REG:
      str = "No such Service-Registration found";
      break;
    case IGMP_ERR_NO_CONTEXT_INFO:
      str = "Failed to get VR/VRF Context Information";
      break;
    case IGMP_ERR_NO_VALID_CONFIG:
      str = "Invalid configuration for this operation";
      break;
    case IGMP_ERR_NO_SUCH_VALUE:
      str = "No such configured value found";
      break;
    case IGMP_ERR_DOOM:
      str = "IGMP is DOOMED!";
      break;
    case IGMP_ERR_OOM:
      str = "Out of memory";
      break;
    case IGMP_ERR_SOCK_JOIN_FAIL:
      str = "Failed to Join ALL-PIM-ROUTERS MCast Group";
      break;
    case IGMP_ERR_MALFORMED_ARG:
      str = "Malformed argument";
      break;
    case IGMP_ERR_QI_LE_QRI:
      str = "Query interval should be greater than Query Response Interval";
      break;
    case IGMP_ERR_QRI_GT_QI:
      str = "Query Response Interval should be less than Query Interval";
      break;
    case IGMP_ERR_MALFORMED_MSG:
      str = "Malformed message received";
      break;
    case IGMP_ERR_TEMP_INTERNAL:
      str = "Temporary internal run-time failure";
      break;
    case IGMP_ERR_CFG_WITH_MROUTE_PROXY:
      str = "Interface configured with mroute-proxy, undo first";
      break;
    case IGMP_ERR_CFG_FOR_PROXY_SERVICE:
      str = "Interface configured for proxy-service, undo first";
      break;
    case IGMP_ERR_UNINIT_WITHOUT_DEREG:
      str = "Cannot un-initialize without de-registration";
      break;
    case IGMP_ERR_IF_GREC_LIMIT_REACHED:
      str = "G-Rec Limit Reached on IF";
      break;
    case IGMP_ERR_BUF_TOO_SHORT:
      str = "Processing exceeded buffer space";
      break;
    case IGMP_ERR_L2_PHYSICAL_IF:
       str = "Command is invalid on VLAN constituent Interface";
       break;
    case IGMP_ERR_L3_NON_VLAN_IF:
       str = "Command is valid only on VLAN Interfaces";
       break;
    case IGMP_ERR_UNKNOWN_MSG:
       str = "Unknown Message";
       break;
    case IGMP_ERR_L2_SOCK_FAIL:
       str = "L2 socket initialization failed";
       break;
    case IGMP_ERR_L3_SOCK_FAIL:
       str = "L3 socket initialization failed";
       break;
    case IGMP_ERR_SNOOP_ENABLED:
       str = "IGMP Snooping is enabled on VLAN Interface";
       break;
    case IGMP_ERR_IGMP_ENABLED:
       str = "IGMP is enabled on VLAN Interface";
       break;
    case IGMP_ERR_NO_PROXY_SERVICE:
       str = "Interface not configured for proxy service";
       break;
    default:
       str = "Undefined Error Code!";
       break;
    }

  IGMP_FN_EXIT (str);
}

