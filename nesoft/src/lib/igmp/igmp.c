/* Copyright (C) 2002-2003  All Rights Reserved. */

#include <igmp_incl.h>

int igmp_ifindex_cmp (void *data1, void *data2);

/* Initialize IGMP Instance */
s_int32_t
igmp_initialize (struct igmp_init_params *igin,
                 struct igmp_instance **pp_igi)
{
  struct igmp_instance *igi;
  s_int32_t ret;

  IGMP_FN_ENTER (IGMP Init);

  ret = IGMP_ERR_NONE;
  (*pp_igi) = NULL;
  igi = NULL;

  if (! igin
      || ! igin->igin_owning_ivrf)
    {
      ret = IGMP_ERR_MALFORMED_ARG;
      goto EXIT;
    }

  /* Lookout for already initialized instance */
  igi = LIB_VRF_GET_IGMP_INSTANCE (igin->igin_owning_ivrf);

  if (igi)
    {
      ret = IGMP_ERR_NONE;
      goto EXIT;
    }

  if (! igin->igin_i_buf)
    {
      ret = IGMP_ERR_MALFORMED_ARG;
      goto EXIT;
    }

  if (! igin->igin_o_buf)
    {
      ret = IGMP_ERR_MALFORMED_ARG;
      goto EXIT;
    }

  igi = XCALLOC (MTYPE_IGMP_INST, sizeof (struct igmp_instance));

  if (! igi)
    {
      ret = IGMP_ERR_OOM;
      goto EXIT;
    }

  /* Initialize with input parameters */
  igi->igi_lg = igin->igin_lg;
  igi->igi_owning_ivrf = igin->igin_owning_ivrf;
  igi->igi_i_buf = igin->igin_i_buf;
  igi->igi_o_buf = igin->igin_o_buf;
  igi->igi_cback_tunnel_get = igin->igin_cback_tunnel_get;

  /* Initialize IGMP Interface AVL Tree */
  ret = avl_create (&igi->igi_if_tree,0, igmp_ifindex_cmp);

  if (ret < 0)
    {
      ret = IGMP_ERR_OOM;
      goto CLEANUP;
    }

  /* Initialize common group addresses */
  igi->igi_allhosts.s_addr = pal_hton32 (INADDR_ALLHOSTS_GROUP);
  igi->igi_allrouters.s_addr = pal_hton32 (INADDR_ALLRTRS_GROUP);
  igi->igi_igmp_v3routers.s_addr = pal_hton32 (INADDR_IGMPv3_RTRS_GROUP);
  igi->igi_in4any_addr.s_addr = INADDR_ANY;

#ifdef HAVE_SNMP
  if (IS_APN_VR_PRIVILEGED (igi->igi_owning_ivrf->vr) && IS_APN_VRF_DEFAULT (igi->igi_owning_ivrf))
    ret = igmp_snmp_init (igi);
#endif /*HAVE_SNMP */

  goto EXIT;

CLEANUP:

  if (igi)
    {
      list_delete_all_node (&igi->igi_ssm_map_static_lst);

      list_delete_all_node (&igi->igi_svc_reg_lst);

      if (igi->igi_if_tree)
        avl_tree_free (&igi->igi_if_tree,
                       (void_t (*) (void_t *)) igmp_if_free);

      XFREE (MTYPE_IGMP_INST, igi);
      igi = NULL;
    }

EXIT:

  (*pp_igi) = igi;

  IGMP_FN_EXIT (ret);
}

/* Uninitialize IGMP Instance */
s_int32_t
igmp_uninitialize (struct igmp_instance *igi,
                   bool_t forced)
{
  struct igmp_ssm_map_static *igssms;
  struct igmp_svc_reg_params igsrp;
  struct igmp_svc_reg *igsr;
  struct listnode *next;
  struct listnode *nn;
  s_int32_t ret;

  IGMP_FN_ENTER (IGMP UnInit);

  ret = IGMP_ERR_NONE;

  if (! igi)
    {
      ret = IGMP_ERR_NONE;
      goto EXIT;
    }

  /* Refuse uninit till all Svc Regs are de-registered */
  if (! forced
      && LISTCOUNT (&igi->igi_svc_reg_lst))
    {
      ret = IGMP_ERR_NONE;
      goto EXIT;
    }

  /* Clean up SSM-Map Static List */
  for (nn = LISTHEAD (&igi->igi_ssm_map_static_lst); nn; nn = next)
    {
      next = nn->next;

      if (! (igssms = GETDATA (nn)))
        continue;

      if (igssms->igssms_grp_alist)
        XFREE (MTYPE_ACCESS_LIST_STR, igssms->igssms_grp_alist);

      XFREE (MTYPE_IGMP_SSM_MAP_STATIC, igssms);
    }
  list_delete_all_node (&igi->igi_ssm_map_static_lst);

  /* De-register all Svc-Registrations */
  for (nn = LISTHEAD (&igi->igi_svc_reg_lst); nn; nn = next)
    {
      next = nn->next;

      if (! (igsr = GETDATA (nn)))
        continue;

      igsrp.igsrp_su_id = igsr->igsr_su_id;
      igsrp.igsrp_sp_id = igsr->igsr_sp_id;
      igsrp.igsrp_svc_type = igsr->igsr_svc_type;

      (void_t) igmp_svc_deregister (igi, &igsrp);
    }
  list_delete_all_node (&igi->igi_svc_reg_lst);

  /* Stop all IGMP Interfaces & Free IGMP Interface AVL Tree*/
  if (igi->igi_if_tree)
    avl_tree_free (&igi->igi_if_tree,
                   (void_t (*) (void_t *)) igmp_if_free);

#ifdef HAVE_SNMP
  if (IS_APN_VR_PRIVILEGED (igi->igi_owning_ivrf->vr) && IS_APN_VRF_DEFAULT (igi->igi_owning_ivrf))
    ret = igmp_snmp_deinit (igi);
#endif /* HAVE_SNMP */

  /* Free IGMP Instance */
  XFREE (MTYPE_IGMP_INST, igi);

EXIT:

  IGMP_FN_EXIT (ret);
}

/* IGMP Service Registration */
s_int32_t
igmp_svc_register (struct igmp_instance *igi,
                   struct igmp_svc_reg_params *igsrp)
{
  struct igmp_svc_reg *igsr_tmp;
  struct igmp_svc_reg *igsr;
  struct listnode *nn;
  s_int32_t ret;

  IGMP_FN_ENTER (Svc Reg);

  ret = IGMP_ERR_NONE;
  igsr = NULL;

  /* Sanity checks */
  if (! igi
      || ! igsrp)
    {
      ret = IGMP_ERR_MALFORMED_ARG;
      goto EXIT;
    }

  switch (igsrp->igsrp_svc_type)
    {
    case IGMP_SVC_TYPE_L2:
      if (! igsrp->igsrp_cback_lmem_update)
        {
          ret = IGMP_ERR_MALFORMED_ARG;
          goto EXIT;
        }
      break;

    case IGMP_SVC_TYPE_L3:
      if (! igsrp->igsrp_cback_vif_ctl
          || ! igsrp->igsrp_cback_lmem_update
          || ! igsrp->igsrp_cback_mfc_prog)
        {
          ret = IGMP_ERR_MALFORMED_ARG;
          goto EXIT;
        }
      break;

    default:
      ret = IGMP_ERR_MALFORMED_ARG;
      goto EXIT;
    }

  /* Search for existing Reg based on SvcType & SuId */
  LIST_LOOP (&igi->igi_svc_reg_lst, igsr_tmp, nn)
    if (igsr_tmp->igsr_svc_type == igsrp->igsrp_svc_type
        && igsr_tmp->igsr_sp_id == igsrp->igsrp_su_id)
      {
        igsr = igsr_tmp;
        break;
      }

  if (igsr)
    {
      ret = IGMP_ERR_NONE;
      goto EXIT;
    }

  /* Create a new Registration */
  igsr = XCALLOC (MTYPE_IGMP_SVC_REG, sizeof (struct igmp_svc_reg));

  if (! igsr)
    {
      /* Clear the SpId returned to the Registrant */
      igsrp->igsrp_sp_id = (u_int32_t) NULL;

      ret = IGMP_ERR_OOM;
      goto EXIT;
    }

  /* Initialize with input parameters */
  igsr->igsr_owning_igi = igi;
  igsr->igsr_su_id = igsrp->igsrp_su_id;
  igsr->igsr_sp_id = (u_int32_t) igsr;
  igsr->igsr_svc_type = igsrp->igsrp_svc_type;
  igsr->igsr_sock = igsrp->igsrp_sock;
  igsr->igsr_cback_if_su_id = igsrp->igsrp_cback_if_su_id;
  igsr->igsr_cback_vif_ctl = igsrp->igsrp_cback_vif_ctl;
  igsr->igsr_cback_lmem_update = igsrp->igsrp_cback_lmem_update;
  igsr->igsr_cback_mfc_prog = igsrp->igsrp_cback_mfc_prog;
  igsr->igsr_cback_mrt_if_update = igsrp->igsrp_cback_mrt_if_update;

  switch (igsr->igsr_svc_type)
    {
    case IGMP_SVC_TYPE_L2:
      /* Obtain the socket if not provided */
      if (igsr->igsr_sock <= IGMP_SOCK_FD_INVALID)
        {
          igsr->igsr_sock = pal_sock (IGMP_LG (igi), AF_IGMP_SNOOP,
                                      SOCK_RAW, IPPROTO_IGMP);

          if (igsr->igsr_sock <= IGMP_SOCK_FD_INVALID)
            {
              IGMP_DBG_INFO (igi, EVENTS, "AF_IGMP_SNOOP socket ()"
                             " Failed!");

              ret = IGMP_ERR_L2_SOCK_FAIL;
              goto CLEANUP;
            }

          SET_FLAG (igsr->igsr_sflags, IGMP_SVC_REG_SFLAG_SOCK_OWNER);
        }

      /* Set IGMP L2 socket options */
      ret = igmp_svc_l2_sock_setopt (igsr);

      if (ret < IGMP_ERR_NONE)
        {
          ret = IGMP_ERR_L2_SOCK_FAIL;
          goto CLEANUP;
        }

      /* Start the Socket Read thread */
      IGMP_READ_ON (IGMP_LG (igi), igsr->t_igsr_read, igsr,
                    igmp_sock_read, igsr->igsr_sock);

      /* Determine IGMP-Snooping Operational Status */
      if (! CHECK_FLAG (igi->igi_cflags,
                        IGMP_INST_CFLAG_SNOOP_DISABLED))
        {
          SET_FLAG (igi->igi_sflags, IGMP_INST_SFLAG_SNOOP_ENABLED);

          /* Enable L2 filters on the socket */
          ret = pal_sock_set_l2_igmp_filter (igsr->igsr_sock, PAL_TRUE);

          if (ret < 0)
            {
              IGMP_DBG_WARN (igi, EVENTS, "Sock SetOpt Filters "
                             "Enable Failed: %s(%d)",
                             pal_strerror (errno), errno);

              /* Just loose err-code */
              ret = IGMP_ERR_NONE;
            }
        }
      break;

    case IGMP_SVC_TYPE_L3:
      /* Obtain the socket if not provided */
      if (igsr->igsr_sock <= IGMP_SOCK_FD_INVALID)
        {
          igsr->igsr_sock = pal_sock (IGMP_LG (igi), AF_INET,
                                      SOCK_RAW, IPPROTO_IGMP);

          if (igsr->igsr_sock <= IGMP_SOCK_FD_INVALID)
            {
              IGMP_DBG_INFO (igi, EVENTS, "AF_INET socket () Failed!");

              ret = IGMP_ERR_L3_SOCK_FAIL;
              goto CLEANUP;
            }

          SET_FLAG (igsr->igsr_sflags, IGMP_SVC_REG_SFLAG_SOCK_OWNER);
        }

      /* Set IGMP L3 socket options */
      ret = igmp_svc_l3_sock_setopt (igsr);

      if (ret < IGMP_ERR_NONE)
        {
          ret = IGMP_ERR_L3_SOCK_FAIL;
          goto CLEANUP;
        }

      /* Start the Socket Read thread */
      IGMP_READ_ON (IGMP_LG (igi), igsr->t_igsr_read, igsr,
                    igmp_sock_read, igsr->igsr_sock);

      /* Determine IGMP Operational Status */
      SET_FLAG (igi->igi_sflags, IGMP_INST_SFLAG_L3_ENABLED);
      break;

    default:
      ret = IGMP_ERR_DOOM;
      goto EXIT;
    }

  listnode_add (&igi->igi_svc_reg_lst, igsr);

  /* Registrant needs SpId for de-registration */
  igsrp->igsrp_sp_id = igsr->igsr_sp_id;

  goto EXIT;

CLEANUP:

  if (CHECK_FLAG (igsr->igsr_sflags,
                  IGMP_SVC_REG_SFLAG_SOCK_OWNER))
    pal_sock_close (IGMP_LG (igi), igsr->igsr_sock);

  if (igsr)
    XFREE (MTYPE_IGMP_SVC_REG, igsr);

  igsrp->igsrp_sp_id = (u_int32_t) NULL;

EXIT:

  IGMP_FN_EXIT (ret);
}

/* IGMP Service De-Registration */
s_int32_t
igmp_svc_deregister (struct igmp_instance *igi,
                     struct igmp_svc_reg_params *igsrp)
{
  struct igmp_svc_reg *igsr_tmp;
  struct igmp_svc_reg *igsr;
  struct listnode *nn;
  s_int32_t ret;

  IGMP_FN_ENTER (Svc DeReg);

  ret = IGMP_ERR_NONE;
  igsr = NULL;

  if (! igi
      || ! igsrp
      || ! igsrp->igsrp_sp_id)
    {
      ret = IGMP_ERR_MALFORMED_ARG;
      goto EXIT;
    }

  switch (igsrp->igsrp_svc_type)
    {
    case IGMP_SVC_TYPE_L2:
    case IGMP_SVC_TYPE_L3:
      break;

    default:
      ret = IGMP_ERR_MALFORMED_ARG;
      goto EXIT;
    }

  /* Search for existing Reg based on SvcType & SpId */
  LIST_LOOP (&igi->igi_svc_reg_lst, igsr_tmp, nn)
    if (igsr_tmp->igsr_svc_type == igsrp->igsrp_svc_type
        && igsr_tmp->igsr_sp_id == igsrp->igsrp_sp_id)
      {
        igsr = igsr_tmp;
        break;
      }

  if (! igsr)
    {
      ret = IGMP_ERR_NO_SUCH_SVC_REG;
      goto EXIT;
    }

  switch (igsr->igsr_svc_type)
    {
    case IGMP_SVC_TYPE_L2:
      /* Stop the IGMP read thread */
      IGMP_READ_OFF (IGMP_LG (igi), igsr->t_igsr_read);

      /* Determine IGMP-Snooping Operational Status */
      UNSET_FLAG (igi->igi_sflags, IGMP_INST_SFLAG_SNOOP_ENABLED);

      /* Disable L2 filters on the socket */
      ret = pal_sock_set_l2_igmp_filter (igsr->igsr_sock, PAL_FALSE);

      if (ret < 0)
        {
          IGMP_DBG_WARN (igi, EVENTS, "Sock SetOpt Filters "
                         "Disable Failed: %s(%d)",
                         pal_strerror (errno), errno);

          /* Just loose err-code */
          ret = IGMP_ERR_NONE;
        }

      /* Close the socket if Self-owned */
      if (CHECK_FLAG (igsr->igsr_sflags,
                      IGMP_SVC_REG_SFLAG_SOCK_OWNER))
        {
          pal_sock_close (IGMP_LG (igi), igsr->igsr_sock);

          UNSET_FLAG (igsr->igsr_sflags,
                      IGMP_SVC_REG_SFLAG_SOCK_OWNER);
        }
      break;

    case IGMP_SVC_TYPE_L3:
      /* Stop the IGMP read thread */
      IGMP_READ_OFF (IGMP_LG (igi), igsr->t_igsr_read);

      /* Determine IGMP Operational Status */
      UNSET_FLAG (igi->igi_sflags, IGMP_INST_SFLAG_L3_ENABLED);

      /* Close the socket if Self-owned */
      if (CHECK_FLAG (igsr->igsr_sflags,
                      IGMP_SVC_REG_SFLAG_SOCK_OWNER))
        {
          pal_sock_close (IGMP_LG (igi), igsr->igsr_sock);

          UNSET_FLAG (igsr->igsr_sflags,
                      IGMP_SVC_REG_SFLAG_SOCK_OWNER);
        }
      break;

    default:
      ret = IGMP_ERR_DOOM;
      goto EXIT;
    }

  listnode_delete (&igi->igi_svc_reg_lst, igsr);

  XFREE (MTYPE_IGMP_SVC_REG, igsr);

  igsrp->igsrp_sp_id = (u_int32_t) NULL;

EXIT:

  IGMP_FN_EXIT (ret);
}

/* IGMP Layer-3 Service Raw-Socket Set Options */
s_int32_t
igmp_svc_l3_sock_setopt (struct igmp_svc_reg *igsr)
{
  s_int32_t ret;

  IGMP_FN_ENTER (L3 Svc Sock SetOpt);

  ret = IGMP_ERR_NONE;

  if (! igsr
      || igsr->igsr_svc_type != IGMP_SVC_TYPE_L3
      || igsr->igsr_sock <= IGMP_SOCK_FD_INVALID)
    {
      ret = IGMP_ERR_MALFORMED_ARG;
      goto EXIT;
    }

  /* Set socket as non-blocking */
  ret = pal_sock_set_nonblocking (igsr->igsr_sock, PAL_TRUE);

  if (ret < 0)
    {
      IGMP_DBG_WARN (igsr->igsr_owning_igi, EVENTS,
                     "Can't set Non-blocking: %s(%d)",
                     pal_strerror (errno), errno);

      ret = IGMP_ERR_NONE;
      goto EXIT;
    }

  /* Turn on header include IPHDR if available */
  ret = pal_sock_set_ip_hdr_incl (igsr->igsr_sock, PAL_TRUE);

  if (ret < 0)
    {
      IGMP_DBG_WARN (igsr->igsr_owning_igi, EVENTS,
                     "Can't set IP Hdr Incl to TRUE: %s(%d)",
                     pal_strerror (errno), errno);

      ret = IGMP_ERR_NONE;
      goto EXIT;
    }

  /* Set IPv4 multicast TTL to 1 */
  ret = pal_sock_set_ipv4_unicast_hops (igsr->igsr_sock,
                                        IGMP_MSG_TTL_DEF);

  if (ret < 0)
    {
      IGMP_DBG_WARN (igsr->igsr_owning_igi, EVENTS,
                     "Can't set IPv4 Multicast TTL to %d: %s(%d)",
                     IGMP_MSG_TTL_DEF, pal_strerror (errno), errno);

      ret = IGMP_ERR_NONE;
      goto EXIT;
    }

EXIT:

  IGMP_FN_EXIT (ret);
}

/* IGMP Layer-2 Service Raw-Socket Set Options */
s_int32_t
igmp_svc_l2_sock_setopt (struct igmp_svc_reg *igsr)
{
  s_int32_t ret;

  IGMP_FN_ENTER (L2 Svc Sock SetOpt);

  ret = IGMP_ERR_NONE;

  if (! igsr
      || igsr->igsr_svc_type != IGMP_SVC_TYPE_L2
      || igsr->igsr_sock <= IGMP_SOCK_FD_INVALID)
    {
      ret = IGMP_ERR_MALFORMED_ARG;
      goto EXIT;
    }

  /* Set socket as non-blocking */
  ret = pal_sock_set_nonblocking (igsr->igsr_sock, PAL_TRUE);

  if (ret < 0)
    {
      IGMP_DBG_WARN (igsr->igsr_owning_igi, EVENTS,
                     "Can't set Non-blocking: %s(%d)",
                     pal_strerror (errno), errno);

      ret = IGMP_ERR_NONE;
      goto EXIT;
    }

EXIT:

  IGMP_FN_EXIT (ret);
}

/* Match IGMP IF with Service Registration */
bool_t
igmp_if_svc_reg_match (struct igmp_if *igif,
                       struct igmp_svc_reg *igsr)
{
  bool_t bret;

  IGMP_FN_ENTER (IF Match Svc Reg);

  bret = PAL_FALSE;

  switch (igsr->igsr_svc_type)
    {
    case IGMP_SVC_TYPE_L2:
      if (CHECK_FLAG (igif->igif_sflags, IGMP_IF_SFLAG_SVC_TYPE_L2)
          && ! CHECK_FLAG (igif->igif_sflags, IGMP_IF_SFLAG_SVC_TYPE_L3))
        bret = PAL_TRUE;
      break;

    case IGMP_SVC_TYPE_L3:
      if ((CHECK_FLAG (igif->igif_sflags, IGMP_IF_SFLAG_SVC_TYPE_L3)
           && ! CHECK_FLAG (igif->igif_sflags, IGMP_IF_SFLAG_SVC_TYPE_L2))
          || (CHECK_FLAG (igif->igif_sflags, IGMP_IF_SFLAG_SVC_TYPE_L3)
              && CHECK_FLAG (igif->igif_sflags, IGMP_IF_SFLAG_SVC_TYPE_L2)
              && (CHECK_FLAG (igif->igif_conf.igifc_cflags,
                              IGMP_IF_CFLAG_CONFIG_ENABLED)
                  ||  CHECK_FLAG (igif->igif_sflags,
                                  IGMP_IF_SFLAG_MCAST_ENABLED))))
        bret = PAL_TRUE;
      break;

    default:
      break;
    }

  IGMP_FN_EXIT (bret);
}

/* Determine Svc-User-ID for the Interface */
u_int32_t
igmp_if_svc_reg_su_id (struct igmp_instance *igi,
                       struct interface *ifp)
{
  struct igmp_svc_reg *igsr;
  struct listnode *nn;
  u_int32_t if_su_id;
  s_int32_t ret;

  IGMP_FN_ENTER (Svc Reg Get SID);

  if_su_id = IGMP_IF_SVC_USER_ID_DEF;

  LIST_LOOP (&igi->igi_svc_reg_lst, igsr, nn)
    if (igsr->igsr_cback_if_su_id)
      {
        ret = igsr->igsr_cback_if_su_id (igsr->igsr_su_id,
                                         ifp,
                                         IGMP_IF_SVC_USER_ID_DEF,
                                         &if_su_id);

        if (ret < 0)
          if_su_id = IGMP_IF_SVC_USER_ID_DEF;
        else
          break;
      }

  IGMP_FN_EXIT (if_su_id);
}

u_int32_t
igmp_if_snp_svc_reg_su_id (struct igmp_instance *igi,
                           struct interface *ifp,
                           u_int16_t sec_key)
{
  struct igmp_svc_reg *igsr;
  struct listnode *nn;
  u_int32_t if_su_id;
  s_int32_t ret;

  IGMP_FN_ENTER (Snoop Svc Reg Get SID);

  if_su_id = IGMP_IF_SVC_USER_ID_DEF;

  LIST_LOOP (&igi->igi_svc_reg_lst, igsr, nn)
    if (igsr->igsr_cback_if_su_id
        && igsr->igsr_svc_type == IGMP_SVC_TYPE_L2)
      {
        ret = igsr->igsr_cback_if_su_id (igsr->igsr_su_id,
                                         ifp,
                                         sec_key,
                                         &if_su_id);

        if (ret < 0)
          if_su_id = IGMP_IF_SVC_USER_ID_DEF;
        else
          break;
      }

  IGMP_FN_EXIT (if_su_id);

}

/* IGMP IFF Hash Key Preparation */
u_int32_t
igmp_if_hkey_make (void_t *arg)
{
  struct igmp_if_idx *igifidx;
  u_int32_t hkey;

  IGMP_FN_ENTER (IGMP HKey Make);

  igifidx = arg;

  hkey = ((u_int32_t) igifidx->igifidx_ifp)
          + ((u_int32_t) igifidx->igifidx_sid);

  IGMP_FN_EXIT (hkey);
}

/* IGMP IFF Hash Key Comparison */
bool_t
igmp_if_hkey_cmp (void_t *arg1,
                  void_t *arg2)
{
  struct igmp_if_idx *igifidx;
  struct igmp_if *igif;
  bool_t bret;

  IGMP_FN_ENTER (IGMP HKey Cmp);

  igifidx = arg2;
  igif = arg1;

  bret = (igif->igif_idx.igifidx_ifp == igifidx->igifidx_ifp
          && igif->igif_idx.igifidx_sid == igifidx->igifidx_sid);

  IGMP_FN_EXIT (bret);
}

/* IGMP IFF association with VIF */
s_int32_t
igmp_if_get_vif (struct igmp_if *igif,
                 struct igmp_svc_reg *igsr)
{
  struct igmp_instance *igi;
  s_int32_t ret;

  IGMP_FN_ENTER (IF Get VIF);

  ret = IGMP_ERR_NONE;

  if (! igif
      || ! igsr
      || ! (igi = igif->igif_owning_igi))
    {
      ret = IGMP_ERR_MALFORMED_ARG;
      goto EXIT;
    }

  if (igsr->igsr_cback_vif_ctl
      && igif->igif_paddr != NULL)
    {
      ret = igsr->igsr_cback_vif_ctl (igsr->igsr_su_id,
                                      igif->igif_owning_ifp,
                                      igif->igif_su_id,
                                      igif->igif_paddr,
                                      PAL_TRUE);

      if (ret < IGMP_ERR_NONE)
        {
          IGMP_DBG_INFO (igi, EVENTS, "VIF-Ctl Callback failed on %s",
                         igif->igif_owning_ifp->name);

          goto EXIT;
        }
    }

EXIT:

  IGMP_FN_EXIT (ret);
}

/* IGMP IFF dissociation with VIF */
s_int32_t
igmp_if_release_vif (struct igmp_if *igif,
                     struct igmp_svc_reg *igsr)
{
  struct igmp_instance *igi;
  s_int32_t ret;

  IGMP_FN_ENTER (IF Rel VIF);

  ret = IGMP_ERR_NONE;

  if (! igif || ! (igi = igif->igif_owning_igi))
    {
      ret = IGMP_ERR_MALFORMED_ARG;
      goto EXIT;
    }

  if (igsr->igsr_cback_vif_ctl)
    {
      ret = igsr->igsr_cback_vif_ctl (igsr->igsr_su_id,
                                      igif->igif_owning_ifp,
                                      igif->igif_su_id,
                                      igif->igif_paddr,
                                      PAL_FALSE);

      if (ret < IGMP_ERR_NONE)
        {
          IGMP_DBG_INFO (igi, EVENTS, "VIF-Ctl Callback failed on %s",
                         igif->igif_owning_ifp->name);

          goto EXIT;
        }
    }

EXIT:

  IGMP_FN_EXIT (ret);
}

/* Lookup existing VIF/Get new VIF */
struct igmp_if *
igmp_if_get (struct igmp_instance *igi,
             struct igmp_if_idx *igifidx)
{
  struct igmp_if avl_igif;
  struct avl_node *node;
  struct igmp_if *igif;
  s_int32_t ret;

  IGMP_FN_ENTER (IF Get);

  igif = NULL;

  if (! igi
      || ! igifidx)
    goto EXIT;

  avl_igif.igif_idx = *igifidx;

  if ((node = avl_search (igi->igi_if_tree, (void_t *) &avl_igif)) != NULL)
    {
      igif = (struct igmp_if *) node->info;

      if (igif == NULL)
        IGMP_DBG_INFO (igi, EVENTS, "IGIF info not found in AVL NODE for %s",
                       igifidx->igifidx_ifp->name);
      goto EXIT;
    }
  else
    {
      igif = igmp_if_create (igifidx);

      if (igif == NULL)
        {
          IGMP_DBG_INFO (igi, EVENTS, "IF creation failed for %s",
                         igifidx->igifidx_ifp->name);
          goto EXIT;
        }
      else
        {
          ret = avl_insert (igi->igi_if_tree, (void_t *) igif);

          if (ret < 0)
            {
              IGMP_DBG_INFO (igi, EVENTS, "AVL insertion failed for %s",
                             igifidx->igifidx_ifp->name);

              /* Free allocated node */
              XFREE (MTYPE_IGMP_IF, igif);
              igif = NULL;

              goto EXIT;
            }
        }
    } 

EXIT:

  IGMP_FN_EXIT (igif);
}

/* Lookup existing VIF */
struct igmp_if *
igmp_if_lookup (struct igmp_instance *igi,
                struct igmp_if_idx *igifidx)
{
  struct igmp_if avl_igif;
  struct avl_node *node;
  struct igmp_if *igif;

  IGMP_FN_ENTER (IF Lookup);

  igif = NULL;

  if (! igi
      || ! igifidx)
    goto EXIT;

  avl_igif.igif_idx = *igifidx;

  node = avl_search (igi->igi_if_tree, (void_t *) &avl_igif);

  if (node == NULL)
    {
      IGMP_DBG_INFO (igi, EVENTS, "IGIF not found in AVL Tree for %s",
                     avl_igif.igif_idx.igifidx_ifp->name);
      goto EXIT;
    }

  igif = (struct igmp_if *) node->info;

  if (igif == NULL)
    {
      IGMP_DBG_INFO (igi, EVENTS, "IGIF not found in AVL Node for %s",
                     avl_igif.igif_idx.igifidx_ifp->name);
      goto EXIT;
    }
  else
    {
      return igif;
    }

EXIT:

  IGMP_FN_EXIT (igif);
}

/* Lookup next VIF */
struct igmp_if *
igmp_if_lookup_next (struct igmp_instance *igi,
                     struct igmp_if_idx *igifidx)
{
  struct interface *ifp;
  struct igmp_if *igif;
  u_int32_t index;

  IGMP_FN_ENTER (IF Lookup Next);

  ifp = NULL;
  igif = NULL;
  index = 0;

  if (! igi || ! igifidx)
    goto EXIT;

  if (igifidx->igifidx_ifp)
    index = igifidx->igifidx_ifp->ifindex;

  ifp = ifv_lookup_next_by_index (&igi->igi_owning_ivrf->ifv, index);

  for (igif = NULL; ifp;
       ifp = ifv_lookup_next_by_index (&igi->igi_owning_ivrf->ifv,
                                       igifidx->igifidx_ifp->ifindex))
    {
      igifidx->igifidx_ifp = ifp;
      igifidx->igifidx_sid = igmp_if_svc_reg_su_id (igi, ifp);

      igif = igmp_if_lookup (igi, igifidx);

      if (igif)
        break;
    }

EXIT:

  IGMP_FN_EXIT (igif);
}

/* IGMP Interface Delete */
s_int32_t
igmp_if_delete (struct igmp_if *igif)
{
  struct igmp_instance *igi;
  s_int32_t ret;

  IGMP_FN_ENTER (IF Del);

  ret = IGMP_ERR_NONE;

  if (! igif || ! (igi = igif->igif_owning_igi))
    {
      ret = IGMP_ERR_MALFORMED_ARG;
      goto EXIT;
    }

  ret = avl_remove (igi->igi_if_tree, (void_t *) igif);

  if (ret < 0)
    IGMP_DBG_INFO (igi, EVENTS, "AVL Node Remove failed,"
                   " continuing...");

  ret = igmp_if_clear (igif, NULL, NULL, PAL_TRUE);
  ret = igmp_if_free (igif);

EXIT:

  IGMP_FN_EXIT (ret);
}

/* IGMP Interface Create */
struct igmp_if *
igmp_if_create (struct igmp_if_idx *igifidx)
{
  struct igmp_instance *igi;
  struct interface *ifp;
  struct igmp_if *igif;

  IGMP_FN_ENTER (IF Create);

  igif = NULL;

  if (! igifidx)
    goto EXIT;

  ifp = igifidx->igifidx_ifp;

  if (! ifp)
    goto EXIT;
  
  if (!ifp->vrf)
    goto EXIT;

  igi = LIB_VRF_GET_IGMP_INSTANCE (LIB_IF_GET_LIB_VRF (ifp));

  if (! igi)
    goto EXIT;

  igif = XCALLOC (MTYPE_IGMP_IF, sizeof (struct igmp_if));

  if (! igif)
    goto EXIT;

  /* Initialize IGMP interface */
  igif->igif_owning_igi = igi;
  igif->igif_idx = *igifidx;

  /* Initialize the Group-Membership-Records TIB */
  igif->igif_gmr_tib = ptree_init (IPV4_MAX_PREFIXLEN);

  if (! igif->igif_gmr_tib)
    goto CLEANUP;

  /* Initialize the Host-side Group-Membership-Records TIB */
  igif->igif_hst_gmr_tib = ptree_init (IPV4_MAX_PREFIXLEN);

  if (! igif->igif_hst_gmr_tib)
    goto CLEANUP;

  /* Initialize the Configurable values to Defaults */
  igif->igif_conf.igifc_lmqc = IGMP_LAST_MEMBER_QUERY_COUNT_DEF;
  igif->igif_conf.igifc_lmqi = IGMP_LAST_MEMBER_QUERY_INTERVAL_DEF;
  igif->igif_conf.igifc_limit = IGMP_LIMIT_DEF;
  igif->igif_conf.igifc_oqi = IGMP_QUERIER_TIMEOUT_DEF;
  igif->igif_conf.igifc_qi = IGMP_QUERY_INTERVAL_DEF;
  igif->igif_conf.igifc_sqi = IGMP_STARTUP_QUERY_INTERVAL_DEF;
  igif->igif_conf.igifc_sqc = IGMP_STARTUP_QUERY_COUNT_DEF;
  igif->igif_conf.igifc_qri = IGMP_QUERY_RESPONSE_INTERVAL_DEF;
  igif->igif_conf.igifc_rv = IGMP_ROBUSTNESS_VAR_DEF;
  igif->igif_conf.igifc_version = IGMP_VERSION_DEF;

  /* Inherit the Config-Defaults for run-time */
  igif->igif_qi = igif->igif_conf.igifc_qi;
  igif->igif_sqc = igif->igif_conf.igifc_sqc;
  igif->igif_oqi = igif->igif_conf.igifc_oqi;
  igif->igif_gmi = igif->igif_conf.igifc_rv *
                   igif->igif_conf.igifc_qi +
                   igif->igif_conf.igifc_qri;
  igif->igif_rv = igif->igif_conf.igifc_rv;

  /* Determine possible Service-Types for the interface */
  if (INTF_TYPE_L3 (ifp))
    {
      if (if_is_vlan (ifp))
        SET_FLAG (igif->igif_sflags, IGMP_IF_SFLAG_SVC_TYPE_L2);

      SET_FLAG (igif->igif_sflags, IGMP_IF_SFLAG_SVC_TYPE_L3);
    }
  else if (INTF_TYPE_L2 (ifp))
    SET_FLAG (igif->igif_sflags, IGMP_IF_SFLAG_SVC_TYPE_L2);
  else
    {
      IGMP_DBG_ERR (igi, EVENTS, "%s IF-TYPE !L2 && !L3, Aborting...",
                    ifp->name);

      goto CLEANUP;
    }

  goto EXIT;

CLEANUP:

  if (igif)
    {
      list_delete_all_node (&igif->igif_hst_dsif_lst);

      if (igif->igif_hst_gmr_tib)
        ptree_finish (igif->igif_hst_gmr_tib);

      if (igif->igif_gmr_tib)
        ptree_finish (igif->igif_gmr_tib);

      XFREE (MTYPE_IGMP_IF, igif);

      igif = NULL;
    }

EXIT:

  IGMP_FN_EXIT (igif);
}

/* IGMP Interface Free */
s_int32_t
igmp_if_free (struct igmp_if *igif)
{
  struct igmp_if_idx *igifidx;
  struct igmp_instance *igi;
  struct listnode *next;
  struct listnode *nn;
  s_int32_t ret;

  IGMP_FN_ENTER (IF Free);

  ret = IGMP_ERR_NONE;

  if (! igif || ! (igi = igif->igif_owning_igi))
    {
      ret = IGMP_ERR_MALFORMED_ARG;
      goto EXIT;
    }

  ret = igmp_if_stop (igif);

  if (ret < IGMP_ERR_NONE)
    IGMP_DBG_WARN (igi, EVENTS, "IF Stop failed(%d), continuing...",
                   ret);

  IGMP_DBG_INFO (igi, EVENTS, "Deleting %s IGMP IF",
                 igif->igif_owning_ifp->name);

  if (! CHECK_FLAG (igif->igif_sflags,
                    IGMP_IF_SFLAG_IF_CONF_INHERITED))
    {
      if (igif->igif_conf.igifc_access_list)
        XFREE (MTYPE_ACCESS_LIST_STR,
               igif->igif_conf.igifc_access_list);

      if (igif->igif_conf.igifc_immediate_leave)
        XFREE (MTYPE_ACCESS_LIST_STR,
               igif->igif_conf.igifc_immediate_leave);

      for (nn = LISTHEAD (&igif->igif_conf.igifc_mrouter_if_lst);
           nn; nn = next)
        {
          next = nn->next;

          if ((igifidx = GETDATA (nn)))
            XFREE (MTYPE_IGMP_IF_IDX, igifidx);
        }

      list_delete_all_node (&igif->igif_conf.igifc_mrouter_if_lst);
    }

  list_delete_all_node (&igif->igif_hst_dsif_lst);

  list_delete_all_node (&igif->igif_hst_chld_lst);

  if (!CHECK_FLAG (igif->igif_sflags, IGMP_IF_SFLAG_SVC_TYPE_L3)
      && CHECK_FLAG (igif->igif_sflags, IGMP_IF_SFLAG_SVC_TYPE_L2)
      && igif->igmp_parent_igif)
    listnode_delete (&igif->igmp_parent_igif->igif_hst_chld_lst, igif);

  if (igif->igif_hst_gmr_tib)
    ptree_finish (igif->igif_hst_gmr_tib);

  if (igif->igif_gmr_tib)
    ptree_finish (igif->igif_gmr_tib);

  XFREE (MTYPE_IGMP_IF, igif);

EXIT:

  IGMP_FN_EXIT (ret);
}

/* IGMP Interface Start */
s_int32_t
igmp_if_start (struct igmp_if *igif, struct igmp_if *p_igif)
{
  struct igmp_if_idx *mrtr_igifidx;
  struct connected *conn_addr;
  u_int32_t querier_interval;
  struct igmp_if *temp_igif;
  struct igmp_svc_reg *igsr;
  struct igmp_instance *igi;
  struct igmp_if *hst_igif;
  struct igmp_if *ds_igif;
  bool_t assoc_svc_reg;
  struct listnode *nn;
  s_int32_t ret;

  IGMP_FN_ENTER (IF Start);

  assoc_svc_reg = PAL_FALSE;
  mrtr_igifidx = NULL;
  ret = IGMP_ERR_NONE;

  if (! igif || ! (igi = igif->igif_owning_igi))
    {
      ret = IGMP_ERR_MALFORMED_ARG;
      goto EXIT;
    }

  if (CHECK_FLAG (igif->igif_sflags, IGMP_IF_SFLAG_ACTIVE))
    {
      IGMP_DBG_INFO (igi, EVENTS, "IGMP IF %s already active",
                     igif->igif_owning_ifp->name);

      ret = IGMP_ERR_NONE;
      goto EXIT;
    }

  /* L2 Physical IF initialization */
  if (CHECK_FLAG (igif->igif_sflags, IGMP_IF_SFLAG_SVC_TYPE_L2)
      && ! CHECK_FLAG (igif->igif_sflags, IGMP_IF_SFLAG_SVC_TYPE_L3))
    {
      /* Try to get an association with VLAN-IF */
      if (! igif->igif_hst_igif)
        {
          ret = igmp_if_snp_service_association (p_igif, igif, PAL_TRUE);
          if (p_igif
              && (p_igif->igif_su_id == igif->igif_su_id)
              && ! igif->igif_hst_igif) 
            {
              IGMP_DBG_INFO (igi, EVENTS, "L2/L3 IF %s is not active"
                             "Adding L2 Physical %s to L2/L3 IF for"
                             "future use", igif->igif_owning_ifp->name,
                             p_igif->igif_owning_ifp->name);
            }

          if (! igif->igmp_parent_igif && p_igif)
            {
              listnode_add (&p_igif->igif_hst_chld_lst, igif);
              igif->igmp_parent_igif = p_igif;
            }
        }

      /* L2 Phy IFs cannot start before the VLAN Interface */
      if (! igif->igif_hst_igif
          || ! CHECK_FLAG (igif->igif_hst_igif->igif_sflags,
                           IGMP_IF_SFLAG_ACTIVE))
        {
          IGMP_DBG_INFO (igi, EVENTS, "L2 Physical IF %s cannot start"
                         " before owning VLAN (%u) IF, Aborting...",
                         igif->igif_owning_ifp->name,
                         igif->igif_su_id);

          ret = IGMP_ERR_NONE;
          goto EXIT;
        }

      /* Inherit configs and address from VLAN Interface */
      igif->igif_conf = igif->igif_hst_igif->igif_conf;
      igif->igif_paddr = igif->igif_hst_igif->igif_paddr;

      /* Inherit layer3 multicast status flag from VLAN Interface */
      if (CHECK_FLAG (igif->igif_hst_igif->igif_sflags,
                      IGMP_IF_SFLAG_MCAST_ENABLED))
        SET_FLAG (igif->igif_sflags, IGMP_IF_SFLAG_MCAST_ENABLED);

      /* Note that IF Configs were inherited */
      SET_FLAG (igif->igif_sflags, IGMP_IF_SFLAG_IF_CONF_INHERITED);
    }

  /* Enable snooping if necessary */
  if (igmp_if_snp_start (igif))
    SET_FLAG (igif->igif_sflags, IGMP_IF_SFLAG_SNOOPING);

  /* Globally enable IGMP Snooping if not already enabled */
  if (CHECK_FLAG (igif->igif_sflags, IGMP_IF_SFLAG_SVC_TYPE_L2)
      && CHECK_FLAG (igif->igif_sflags, IGMP_IF_SFLAG_SVC_TYPE_L3)
      && ! CHECK_FLAG (igi->igi_sflags, IGMP_INST_SFLAG_SNOOP_ENABLED))
    {
      SET_FLAG (igi->igi_sflags, IGMP_INST_SFLAG_SNOOP_ENABLED);

      /* Enable L2 filters on the socket */
      LIST_LOOP (&igi->igi_svc_reg_lst, igsr, nn)
        switch (igsr->igsr_svc_type)
          {
          case IGMP_SVC_TYPE_L2:
            ret = pal_sock_set_l2_igmp_filter (igsr->igsr_sock,
                                               PAL_TRUE);

            if (ret < 0)
              {
                IGMP_DBG_WARN (igi, EVENTS, "Sock SetOpt Filters"
                               " Enable Failed: %s(%d)",
                               pal_strerror (errno), errno);

                /* Just loose err-code */
                ret = IGMP_ERR_NONE;
              }
            break;

          case IGMP_SVC_TYPE_L3:
          default:
            break;
          }
    }

  /* Prioritize L2 Snooping on Dual Mode L2/L3 interfaces  */
  if (CHECK_FLAG (igif->igif_sflags, IGMP_IF_SFLAG_SVC_TYPE_L3)
      && ! igif->igif_paddr)
    {
      /* First address will be the Primary Address */
      for (conn_addr = igif->igif_owning_ifp->ifc_ipv4; conn_addr;
           conn_addr = conn_addr->next)
        if (conn_addr->address
            && ! CHECK_FLAG (conn_addr->flags, NSM_IFA_SECONDARY))
          igif->igif_paddr = conn_addr->address;

      if (! igif->igif_paddr)
        {
          if (! CHECK_FLAG (igif->igif_sflags,
                            IGMP_IF_SFLAG_SVC_TYPE_L2))
            {
              IGMP_DBG_INFO (igi, EVENTS, "No valid IPv4 address on %s",
                             igif->igif_owning_ifp->name);

              ret = IGMP_ERR_GENERIC;
              goto EXIT;
            }
        }
      else
        IGMP_DBG_INFO (igi, EVENTS, "Address %O added to IGMP IF %s",
                       igif->igif_paddr, igif->igif_owning_ifp->name);
    }

  IGMP_DBG_INFO (igi, EVENTS, "Starting IGMP%s on %s",
                 CHECK_FLAG (igif->igif_sflags,
                             IGMP_IF_SFLAG_SNOOPING) ?
                 "-Snooping" : "",
                 igif->igif_owning_ifp->name);

  /* Perform Service Specific Initialization */
  LIST_LOOP (&igi->igi_svc_reg_lst, igsr, nn)
    {
      if (! igmp_if_svc_reg_match (igif, igsr))
        continue;

      switch (igsr->igsr_svc_type)
        {
        case IGMP_SVC_TYPE_L3:
          /* Set Multicast-IF Socket Option */
          if (igif->igif_paddr)
            {
              ret = pal_sock_set_ipv4_multicast_if
                    (igsr->igsr_sock,
                     igif->igif_paddr->u.prefix4,
                     igif->igif_owning_ifp->ifindex);

              if (ret < IGMP_ERR_NONE)
                IGMP_DBG_WARN (igi, EVENTS, "Setsockopt IP_MCAST_IF"
                               " failed: %s(%d)!",
                               pal_strerror (errno), errno);
            }

          /* Join required IGMP Groups in the Interface */
          ret = igmp_if_group_join (igif, igsr,
                                    &igi->igi_allhosts);

          if (ret < IGMP_ERR_NONE)
            {
              IGMP_DBG_WARN (igi, EVENTS, "Join ALLHOSTS Grp Failed!"
                             " Aborting...");
            }

          if (! CHECK_FLAG (igif->igif_conf.igifc_cflags,
                            IGMP_IF_CFLAG_PROXY_SERVICE))
            {
              ret = igmp_if_group_join (igif, igsr,
                                        &igi->igi_allrouters);

              if (ret < IGMP_ERR_NONE)
                {
                  IGMP_DBG_WARN (igi, EVENTS, "Join ALLRTRS Grp "
                                 "Failed! Aborting...");
                }
            }

          if (igif->igif_conf.igifc_version == IGMP_VERSION_3)
            {
              ret = igmp_if_group_join (igif, igsr,
                                        &igi->igi_igmp_v3routers);

              if (ret < IGMP_ERR_NONE)
                {
                  IGMP_DBG_WARN (igi, EVENTS, "Join IGMPv3RTRS Grp "
                                 "Failed! Aborting...");
                }
            }

          /* DO NOT BREAK HERE */

        case IGMP_SVC_TYPE_L2:
          /* Obtain the VIF from Multicast Forwarder */
          ret = igmp_if_get_vif (igif, igsr);

          if (ret < IGMP_ERR_NONE)
            {
              IGMP_DBG_INFO (igi, EVENTS, "VIF Get Failed!");

              goto EXIT;
            }

          assoc_svc_reg = PAL_TRUE;
          break;

        default:
          break;
        }

      if (assoc_svc_reg)
        break;
    }

  /* Initialize runtime variables from config values */
  (void_t) igmp_if_update_rtp_vars (igif);

  /* Initialize Host-Compatibility Mode */
  (void_t) igmp_if_hst_update_host_cmode (igif);

  /* Loop through chldlist(L2 IF) and associate them with L2/L3 IF */
  if (CHECK_FLAG (igif->igif_sflags, IGMP_IF_SFLAG_SVC_TYPE_L3)
      && CHECK_FLAG (igif->igif_sflags, IGMP_IF_SFLAG_SVC_TYPE_L2)
      && CHECK_FLAG (igif->igif_sflags, IGMP_IF_SFLAG_SNOOPING))
    LIST_LOOP (&igif->igif_hst_chld_lst, temp_igif, nn)
      if (CHECK_FLAG (igif->igif_sflags,
                      IGMP_IF_SFLAG_SNOOPING))
        igmp_if_snp_service_association (igif, temp_igif, PAL_TRUE); 

  if (CHECK_FLAG (igif->igif_conf.igifc_cflags,
                  IGMP_IF_CFLAG_PROXY_SERVICE))
    avl_traverse (igi->igi_if_tree,
                  igmp_if_hst_avl_traverse_associate, igif);
 
  /* Associate with Proxy-Service Interface */
  if (CHECK_FLAG (igif->igif_conf.igifc_cflags,
                  IGMP_IF_CFLAG_MROUTE_PROXY)
      && ! CHECK_FLAG (igif->igif_conf.igifc_cflags,
                       IGMP_IF_CFLAG_PROXY_SERVICE))
    {
      /* Only one Interface SHOULD be present in mrouter-list */
      if (LIST_ISEMPTY (&igif->igif_conf.igifc_mrouter_if_lst))
        {
          ret = IGMP_ERR_NONE;
          goto EXIT;
        }

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

      if (! hst_igif
          || ! CHECK_FLAG (hst_igif->igif_conf.igifc_cflags,
                           IGMP_IF_CFLAG_PROXY_SERVICE))
        {
          ret = IGMP_ERR_NONE;
          goto EXIT;
        }

      ret = igmp_if_pxy_service_association (hst_igif, igif, PAL_TRUE);
    }

  /* Set the Interface to be Active */
  SET_FLAG (igif->igif_sflags, IGMP_IF_SFLAG_ACTIVE);

  /* Record the start time */
  igif->igif_uptime = pal_time_current (NULL);

  /* Start out as the Querier on the IFF if necessary */
  if (((CHECK_FLAG (igif->igif_sflags, IGMP_IF_SFLAG_SVC_TYPE_L3)
        && ! CHECK_FLAG (igif->igif_sflags, IGMP_IF_SFLAG_SNOOPING)
        && ! CHECK_FLAG (igif->igif_conf.igifc_cflags,
                         IGMP_IF_CFLAG_PROXY_SERVICE))
       || (CHECK_FLAG (igif->igif_sflags, IGMP_IF_SFLAG_SNOOPING)
           && ! CHECK_FLAG (igif->igif_sflags,
                            IGMP_IF_SFLAG_SVC_TYPE_L3)
           && (CHECK_FLAG (igif->igif_conf.igifc_cflags,
                           IGMP_IF_CFLAG_SNOOP_QUERIER)
               || CHECK_FLAG (igif->igif_conf.igifc_cflags,
                              IGMP_IF_CFLAG_CONFIG_ENABLED)
               || CHECK_FLAG (igif->igif_hst_igif->igif_sflags,
                              IGMP_IF_SFLAG_MCAST_ENABLED))))
      && igif->igif_paddr)
    {
      querier_interval = 0;

      if (CHECK_FLAG (igif->igif_sflags, IGMP_IF_SFLAG_SNOOPING)
          && CHECK_FLAG (igif->igif_conf.igifc_cflags,
                         IGMP_IF_CFLAG_SNOOP_QUERIER))
        querier_interval = igif->igif_conf.igifc_qi;

      if (! CHECK_FLAG (igif->igif_sflags, IGMP_IF_SFLAG_SVC_TYPE_L3))
        SET_FLAG (igif->igif_hst_igif->igif_sflags, IGMP_IF_SFLAG_QUERIER);

      SET_FLAG (igif->igif_sflags, IGMP_IF_SFLAG_QUERIER);

      if (CHECK_FLAG(igif->igif_conf.igifc_cflags,
                     IGMP_IF_CFLAG_STARTUP_QUERY_COUNT))
        igif->igif_sqc = igif->igif_conf.igifc_sqc;
      else
        igif->igif_sqc = igif->igif_conf.igifc_rv;

      IGMP_TIMER_ON (IGMP_LG (igi), igif->t_igif_querier, igif,
                     igmp_if_timer_querier, querier_interval);

      IGMP_DBG_INFO (igi, EVENTS, "IGMP ::->Querier on %s",
                     igif->igif_owning_ifp->name);
    }

  /* Set the Mrouter IF flag if necessary */
  if (CHECK_FLAG (igif->igif_sflags, IGMP_IF_SFLAG_SVC_TYPE_L2)
      && ! CHECK_FLAG (igif->igif_sflags, IGMP_IF_SFLAG_SVC_TYPE_L3)
      && CHECK_FLAG (igif->igif_hst_igif->igif_conf.igifc_cflags,
                     IGMP_IF_CFLAG_SNOOP_MROUTER_IF))
    LIST_LOOP (&igif->igif_hst_igif->igif_conf.igifc_mrouter_if_lst,
               mrtr_igifidx, nn)
      if (mrtr_igifidx->igifidx_ifp == igif->igif_owning_ifp)
        {
          /* Add the mrouter interface to all group entries in forwarder */
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

          SET_FLAG (igif->igif_sflags,
                    IGMP_IF_SFLAG_SNOOP_MROUTER_IF_CONFIG);
          break;
        }

  /* Start Operation on all the associated Downstream-IFs */
  if ((CHECK_FLAG (igif->igif_sflags, IGMP_IF_SFLAG_SNOOPING)
       && CHECK_FLAG (igif->igif_sflags, IGMP_IF_SFLAG_SVC_TYPE_L3))
      || CHECK_FLAG (igif->igif_conf.igifc_cflags,
                     IGMP_IF_CFLAG_PROXY_SERVICE))
    LIST_LOOP (&igif->igif_hst_dsif_lst, ds_igif, nn)
      {
        ret = igmp_if_start (ds_igif, NULL);

        if (ret < IGMP_ERR_NONE)
          {
            IGMP_DBG_INFO (igi, EVENTS, "Failed to Start DS-IF %s",
                           ds_igif->igif_owning_ifp->name);

            continue;
          }
      }

EXIT:

  IGMP_FN_EXIT (ret);
}

/* IGMP Interface Stop */
s_int32_t
igmp_if_stop (struct igmp_if *igif)
{
  struct igmp_group_rec *hst_igr;
  struct ptree_node *pn_next;
  struct igmp_instance *igi;
  struct igmp_svc_reg *igsr;
  struct igmp_if *ds_igif;
  struct listnode *next;
  struct ptree_node *pn;
  struct listnode *nn;
  bool_t inp_enabled;
  bool_t oup_status;
  s_int32_t ret;

  IGMP_FN_ENTER (IF Stop);

  ret = IGMP_ERR_NONE;

  if (! igif || ! (igi = igif->igif_owning_igi))
    {
      ret = IGMP_ERR_MALFORMED_ARG;
      goto EXIT;
    }

  IGMP_DBG_INFO (igi, EVENTS, "Stoping IGMP IF %s",
                 igif->igif_owning_ifp->name);

  /* Clear the Group Membership TIB */
  ret = igmp_if_clear (igif, NULL, NULL, PAL_FALSE);

  if (ret < IGMP_ERR_NONE)
    IGMP_DBG_WARN (igi, EVENTS, "Clear TIB Failed(%d)!", ret);

  /* Set the Interface to be Inactive */
  UNSET_FLAG (igif->igif_sflags, IGMP_IF_SFLAG_ACTIVE);

  /* Unset uptime */
  igif->igif_uptime = 0;

  /* Stop Operation of all the DS-IFs IF */
  if ((CHECK_FLAG (igif->igif_sflags, IGMP_IF_SFLAG_SNOOPING)
       && CHECK_FLAG (igif->igif_sflags, IGMP_IF_SFLAG_SVC_TYPE_L3))
      || CHECK_FLAG (igif->igif_conf.igifc_cflags,
                     IGMP_IF_CFLAG_PROXY_SERVICE))
    for (nn = LISTHEAD (&igif->igif_hst_dsif_lst); nn; nn = next)
      {
        next = nn->next;

        if (! (ds_igif = GETDATA (nn)))
          continue;

        ret = igmp_if_stop (ds_igif);

        if (ret < IGMP_ERR_NONE)
          {
            IGMP_DBG_INFO (igi, EVENTS, "Failed to Stop DS-IF %s",
                           ds_igif->igif_owning_ifp->name);

            continue;
          }
      }

  /* Implode Downstream IFs list if in Snoop/Proxy-Svc Mode */
  if (CHECK_FLAG (igif->igif_sflags, IGMP_IF_SFLAG_SNOOPING)
      || CHECK_FLAG (igif->igif_conf.igifc_cflags,
                     IGMP_IF_CFLAG_PROXY_SERVICE))
    avl_traverse (igi->igi_if_tree,
                  igmp_if_hst_avl_traverse_dissociate, igif);
  /* Dissociate with Proxy-Service Interface */
  else if (CHECK_FLAG (igif->igif_conf.igifc_cflags,
                       IGMP_IF_CFLAG_MROUTE_PROXY)
           && igif->igif_hst_igif)
    igmp_if_pxy_service_association (igif->igif_hst_igif,
                                     igif, PAL_FALSE);

  /* Clear the Host Group Membership TIB */
  if ((CHECK_FLAG (igif->igif_sflags, IGMP_IF_SFLAG_SNOOPING)
       && CHECK_FLAG (igif->igif_sflags, IGMP_IF_SFLAG_SVC_TYPE_L3))
      || CHECK_FLAG (igif->igif_conf.igifc_cflags,
                     IGMP_IF_CFLAG_PROXY_SERVICE))
    {
      for (pn = ptree_top (igif->igif_hst_gmr_tib); pn; pn = pn_next)
        {
          pn_next = ptree_next (pn);

          if (! (hst_igr = pn->info))
            continue;

          igmp_if_grec_delete (hst_igr);
        }
    }
  /* Default Configuration if Inherited */
  if (CHECK_FLAG (igif->igif_sflags, IGMP_IF_SFLAG_IF_CONF_INHERITED))
    {
      /* Loose inherited values */
      pal_mem_set (&igif->igif_conf, 0, sizeof (struct igmp_if_conf));

      /* Initialize the Configurable values to Defaults */
      igif->igif_conf.igifc_lmqc = IGMP_LAST_MEMBER_QUERY_COUNT_DEF;
      igif->igif_conf.igifc_lmqi = IGMP_LAST_MEMBER_QUERY_INTERVAL_DEF; 
      igif->igif_conf.igifc_limit = IGMP_LIMIT_DEF;
      igif->igif_conf.igifc_oqi = IGMP_QUERIER_TIMEOUT_DEF;
      igif->igif_conf.igifc_qi = IGMP_QUERY_INTERVAL_DEF;
      igif->igif_conf.igifc_sqi = IGMP_STARTUP_QUERY_INTERVAL_DEF;
      igif->igif_conf.igifc_sqc = IGMP_STARTUP_QUERY_COUNT_DEF;
      igif->igif_conf.igifc_qri = IGMP_QUERY_RESPONSE_INTERVAL_DEF;
      igif->igif_conf.igifc_rv = IGMP_ROBUSTNESS_VAR_DEF;
      igif->igif_conf.igifc_version = IGMP_VERSION_DEF;

      /* Inherit the Config-Defaults for run-time */
      igif->igif_qi = igif->igif_conf.igifc_qi;
      igif->igif_sqc = igif->igif_conf.igifc_sqc;
      igif->igif_oqi = igif->igif_conf.igifc_oqi;
      igif->igif_gmi = igif->igif_conf.igifc_rv *
                       igif->igif_conf.igifc_qi +
                       igif->igif_conf.igifc_qri;
      igif->igif_rv = igif->igif_conf.igifc_rv;
    }

  /* Reset Counters */
  igif->igif_v1_querier_wcount = 0;
  igif->igif_v2_querier_wcount = 0;
  igif->igif_v3_querier_wcount = 0;
  igif->igif_num_leaves = 0;
  igif->igif_num_joins = 0;

  /* Unset derived Flags */
  UNSET_FLAG (igif->igif_sflags, IGMP_IF_SFLAG_HOST_COMPAT_V1);
  UNSET_FLAG (igif->igif_sflags, IGMP_IF_SFLAG_HOST_COMPAT_V2);
  UNSET_FLAG (igif->igif_sflags, IGMP_IF_SFLAG_HOST_COMPAT_V3);
  UNSET_FLAG (igif->igif_sflags, IGMP_IF_SFLAG_IF_CONF_INHERITED);
  UNSET_FLAG (igif->igif_sflags, IGMP_IF_SFLAG_SNOOPING);
  UNSET_FLAG (igif->igif_sflags, IGMP_IF_SFLAG_SNOOP_MROUTER_IF);
  UNSET_FLAG (igif->igif_sflags, IGMP_IF_SFLAG_SNOOP_MROUTER_IF_CONFIG);
  UNSET_FLAG (igif->igif_sflags, IGMP_IF_SFLAG_QUERIER);
  UNSET_FLAG (igif->igif_sflags, IGMP_IF_SFLAG_MFC_MSG_IN_IF);

  /* Stop all threads on IFF */
  IGMP_TIMER_OFF (IGMP_LG (igi), igif->t_igif_querier);
  IGMP_TIMER_OFF (IGMP_LG (igi), igif->t_igif_other_querier);
  IGMP_TIMER_OFF (IGMP_LG (igi), igif->t_igif_warn_rlimit);
  IGMP_TIMER_OFF (IGMP_LG (igi), igif->t_igif_hst_v1_querier_present);
  IGMP_TIMER_OFF (IGMP_LG (igi), igif->t_igif_hst_v2_querier_present);
  IGMP_TIMER_OFF (IGMP_LG (igi), igif->t_igif_hst_send_general_report);
  IGMP_TIMER_OFF (IGMP_LG (igi), igif->t_igif_jg_send_general_report);

  /* Perform Service Specific Initialization */
  LIST_LOOP (&igi->igi_svc_reg_lst, igsr, nn)
    {
      if (! igmp_if_svc_reg_match (igif, igsr))
        continue;

      switch (igsr->igsr_svc_type)
        {
        case IGMP_SVC_TYPE_L3:
          /* Leave required IGMP Groups in the Interface */
          if (igif->igif_conf.igifc_version == IGMP_VERSION_3)
            {
              ret = igmp_if_group_leave (igif, igsr,
                                         &igi->igi_igmp_v3routers);

              if (ret < IGMP_ERR_NONE)
                IGMP_DBG_WARN (igi, EVENTS, "Leave IGMPv3RTRS Grp "
                               "Failed! Continuing...");
            }

          if (! CHECK_FLAG (igif->igif_conf.igifc_cflags,
                            IGMP_IF_CFLAG_PROXY_SERVICE))
            {
              ret = igmp_if_group_leave (igif, igsr,
                                         &igi->igi_allrouters);

              if (ret < IGMP_ERR_NONE)
                IGMP_DBG_WARN (igi, EVENTS, "Leave ALLRTRS Grp "
                               "Failed! Continuing...");
            }

          ret = igmp_if_group_leave (igif, igsr,
                                     &igi->igi_allhosts);

          if (ret < IGMP_ERR_NONE)
            IGMP_DBG_WARN (igi, EVENTS, "Leave ALLHOSTS Grp Failed!"
                           " Continuing...");

          /* DO NOT BREAK HERE */

        case IGMP_SVC_TYPE_L2:
          /* Release the Multicast Forwarder VIF */
          ret = igmp_if_release_vif (igif, igsr);

          if (ret < IGMP_ERR_NONE)
            {
              IGMP_DBG_WARN (igi, EVENTS, "VIF release Failed!");

              /* Loose the err-code */
              ret = IGMP_ERR_NONE;
            }
          break;

        default:
          break;
        }
    }

  /* Loose the Primary-Address */
  igif->igif_paddr = NULL;


  /* Check if Snooping is disabled on all IFs */
  if (CHECK_FLAG (igif->igif_sflags, IGMP_IF_SFLAG_SVC_TYPE_L3)
      && CHECK_FLAG (igif->igif_sflags, IGMP_IF_SFLAG_SVC_TYPE_L2))
    {
      inp_enabled = PAL_FALSE;
      oup_status = PAL_FALSE;
      avl_traverse2 (igi->igi_if_tree,
                     igmp_if_snp_avl_traverse_match_status,
                     (void_t *) &inp_enabled,
                     (void_t *) &oup_status);

      /* Unset Snooping Socket Filters if disabled on all IFs */
      if (! oup_status
          && CHECK_FLAG (igi->igi_sflags, IGMP_INST_SFLAG_SNOOP_ENABLED))
        {
          UNSET_FLAG (igi->igi_sflags, IGMP_INST_SFLAG_SNOOP_ENABLED);

          LIST_LOOP (&igi->igi_svc_reg_lst, igsr, nn)
            switch (igsr->igsr_svc_type)
              {
              case IGMP_SVC_TYPE_L2:
                ret = pal_sock_set_l2_igmp_filter (igsr->igsr_sock,
                                                   PAL_FALSE);

                if (ret < 0)
                  {
                    IGMP_DBG_WARN (igi, EVENTS, "Sock SetOpt Filters"
                                   " Disable Failed: %s(%d)",
                                   pal_strerror (errno), errno);

                    /* Just loose err-code */
                    ret = IGMP_ERR_NONE;
                  }
              break;

              case IGMP_SVC_TYPE_L3:
              default:
              break;
              }
          }
    }

EXIT:

  IGMP_FN_EXIT (ret);
}

/* IGMP IF Clear */
s_int32_t
igmp_if_clear (struct igmp_if *igif,
               struct pal_in4_addr *pgrp,
               struct pal_in4_addr *msrc,
               bool_t delstatgrp)
{
  struct igmp_source_list *isl;
  struct igmp_source_rec *isr;
  struct igmp_group_rec *igr;
  struct ptree_node *pn_next;
  struct ptree_node *pn;
  s_int32_t ret;
  bool_t delgrec = PAL_FALSE;

  IGMP_FN_ENTER (IGMP IF Clear);

  ret = IGMP_ERR_NONE;

  if (! igif)
    {
      ret = IGMP_ERR_INVALID_VALUE;
      goto EXIT;
    }

  if (pgrp)
    {
      igr = igmp_if_grec_lookup (igif, pgrp);

      if (! igr)
        {
          ret = IGMP_ERR_NO_SUCH_GROUP_REC;
          goto EXIT;
        }

      if (CHECK_FLAG (igr->igr_sflags, IGMP_IGR_SFLAG_STATIC)
          || CHECK_FLAG (igr->igr_sflags, IGMP_IGR_SFLAG_JOIN))
        {
          if (delstatgrp == PAL_TRUE
            || CHECK_FLAG (igr->igr_sflags, IGMP_IGR_SFLAG_DYNAMIC))
            delgrec = PAL_TRUE;
        }
      else
        delgrec = PAL_TRUE;

      if (msrc)
        {
          isr = igmp_if_srec_lookup (igr, PAL_TRUE, msrc);

          if (! isr)
            isr = igmp_if_srec_lookup (igr, PAL_FALSE, msrc);

          if (! isr)
            {
              ret = IGMP_ERR_NO_SUCH_SOURCE_REC;
              goto EXIT;
            }

          isl = XCALLOC (MTYPE_IGMP_SRC_LIST,
                         sizeof (struct igmp_source_list));

          if (! isl)
            {
              IGMP_DBG_INFO (igif->igif_owning_igi, EVENTS,
                             "Cannot allocate memory (%d) @ %s:%d",
                             sizeof (struct igmp_source_list),
                             __FILE__, __LINE__);

              ret = IGMP_ERR_OOM;
              goto EXIT;
            }

          isl->isl_num = 1;

          IPV4_ADDR_COPY (&isl->isl_saddr [0],
                          PTREE_NODE_KEY (isr->isr_owning_pn));
          
          if (PAL_TRUE == delgrec)
            {
              ret = igmp_igr_fsm_process_event
                             (igr, IGMP_FME_MANUAL_CLEAR, isl);

              XFREE (MTYPE_IGMP_SRC_LIST, isl);
            }
        }
      else
        {
          if (PAL_TRUE == delgrec)
            {   
              ret = igmp_igr_fsm_process_event
                            (igr, IGMP_FME_MANUAL_CLEAR, NULL);
            }
        }
    }
  else
    {
      for (pn = ptree_top (igif->igif_gmr_tib); pn; pn = pn_next)
        {
          pn_next = ptree_next (pn);

          if (! (igr = pn->info))
            continue;

          if (CHECK_FLAG (igr->igr_sflags, IGMP_IGR_SFLAG_STATIC)
              || CHECK_FLAG (igr->igr_sflags, IGMP_IGR_SFLAG_JOIN))
            {
              if (PAL_TRUE == delstatgrp
                 || CHECK_FLAG (igr->igr_sflags, IGMP_IGR_SFLAG_DYNAMIC))
                {
                  ret = igmp_igr_fsm_process_event
                              (igr, IGMP_FME_MANUAL_CLEAR, NULL);
                }
            }   
          else
            {                   
              ret = igmp_igr_fsm_process_event
                              (igr, IGMP_FME_MANUAL_CLEAR, NULL);
            }
        }
    }

EXIT:

  IGMP_FN_EXIT (ret);
}

/* Update IGMP IF as instructed */
s_int32_t
igmp_if_update (struct igmp_if *igif,
                struct igmp_if *p_igif,
                enum igmp_if_update_mode umode)
{
  struct igmp_if_idx *mrtr_igifidx;
  struct igmp_instance *igi;
  struct listnode *next;
  struct listnode *nn;
  s_int32_t ret;

  IGMP_FN_ENTER (IF Update);

  mrtr_igifidx = NULL;
  ret = IGMP_ERR_NONE;
  next = NULL;
  igi = NULL;
  nn = NULL;

  if (! igif
      || ! (igi = igif->igif_owning_igi))
    {
      ret = IGMP_ERR_MALFORMED_ARG;
      goto EXIT;
    }

  IGMP_DBG_INFO (igi, EVENTS, "IGMP IF %s Update Mode %d",
                 igif->igif_owning_ifp->name, umode);

  switch (umode)
    {
    case IGMP_IF_UPDATE_MCAST_ENABLE:
      if (if_is_vlan (igif->igif_owning_ifp))
        {
          if (CHECK_FLAG (igif->igif_sflags, IGMP_IF_SFLAG_ACTIVE))
            ret = igmp_if_stop (igif);

          SET_FLAG (igif->igif_sflags, IGMP_IF_SFLAG_MCAST_ENABLED);

          if (ret == IGMP_ERR_NONE
              && if_is_up (igif->igif_owning_ifp)
              && if_is_running (igif->igif_owning_ifp)
              && igmp_if_snp_start (igif))
            ret = igmp_if_start (igif, NULL);
        }
      else
        {
          SET_FLAG (igif->igif_sflags, IGMP_IF_SFLAG_MCAST_ENABLED);

          if (if_is_up (igif->igif_owning_ifp)
              && if_is_running (igif->igif_owning_ifp))
            ret = igmp_if_start (igif, NULL);
        }
      break;

    case IGMP_IF_UPDATE_MCAST_DISABLE:
      if (if_is_vlan (igif->igif_owning_ifp))
        {
           if (CHECK_FLAG (igif->igif_sflags, IGMP_IF_SFLAG_ACTIVE))
             ret = igmp_if_stop (igif);

           UNSET_FLAG (igif->igif_sflags, IGMP_IF_SFLAG_MCAST_ENABLED);

           if (ret == IGMP_ERR_NONE
               && if_is_up (igif->igif_owning_ifp)
               && if_is_running (igif->igif_owning_ifp)
               && igmp_if_snp_start (igif))
             ret = igmp_if_start (igif, NULL);
        }
      else if (CHECK_FLAG (igif->igif_sflags, IGMP_IF_SFLAG_ACTIVE))
        {
          UNSET_FLAG (igif->igif_sflags, IGMP_IF_SFLAG_MCAST_ENABLED);

          if (! igif->igif_conf.igifc_cflags)
            ret = igmp_if_stop (igif);
        }
      break;

    case IGMP_IF_UPDATE_MCAST_LMEM_MANUAL:
      if (CHECK_FLAG (igif->igif_sflags, IGMP_IF_SFLAG_MCAST_ENABLED)
          && CHECK_FLAG (igif->igif_sflags, IGMP_IF_SFLAG_ACTIVE))
        ret = igmp_if_send_lmem_update (igif);
      break;

    case IGMP_IF_UPDATE_L2_MCAST_ENABLE:
      SET_FLAG (igif->igif_sflags, IGMP_IF_SFLAG_L2_MCAST_ENABLED);

      if (igmp_if_snp_start (igif)
          && if_is_up (igif->igif_owning_ifp)
          && if_is_running (igif->igif_owning_ifp))
        ret = igmp_if_start (igif, p_igif);
      break;

    case IGMP_IF_UPDATE_L2_MCAST_DISABLE:
      UNSET_FLAG (igif->igif_sflags, IGMP_IF_SFLAG_L2_MCAST_ENABLED);

      if (CHECK_FLAG (igif->igif_sflags, IGMP_IF_SFLAG_ACTIVE))
        ret = igmp_if_stop (igif);
      break;

    case IGMP_IF_UPDATE_CONFIG_ENABLE:
      if (if_is_up (igif->igif_owning_ifp)
          && if_is_running (igif->igif_owning_ifp)
          && (! CHECK_FLAG (igif->igif_sflags,
                            IGMP_IF_SFLAG_SVC_TYPE_L2)
              || CHECK_FLAG (igif->igif_sflags,
                             IGMP_IF_SFLAG_L2_MCAST_ENABLED)))
        ret = igmp_if_start (igif, NULL);
      break;

    case IGMP_IF_UPDATE_CONFIG_DISABLE:
      if (CHECK_FLAG (igif->igif_sflags, IGMP_IF_SFLAG_ACTIVE)
          && ! CHECK_FLAG (igif->igif_sflags,
                           IGMP_IF_SFLAG_MCAST_ENABLED)
          && ! CHECK_FLAG (igif->igif_sflags,
                           IGMP_IF_SFLAG_SVC_TYPE_L2)
          && ! igif->igif_conf.igifc_cflags)
        ret = igmp_if_delete (igif);
      else if (igmp_if_snp_stop (igif))
        ret = igmp_if_stop (igif);
      break;

    case IGMP_IF_UPDATE_CONFIG_RESTART:
      if (CHECK_FLAG (igif->igif_sflags, IGMP_IF_SFLAG_ACTIVE))
        ret = igmp_if_stop (igif);

      if (ret == IGMP_ERR_NONE
          && if_is_up (igif->igif_owning_ifp)
          && if_is_running (igif->igif_owning_ifp))
        {
          if (if_is_vlan (igif->igif_owning_ifp)
              && igmp_if_snp_start (igif))
            ret = igmp_if_start (igif, NULL);
          else if (! CHECK_FLAG (igif->igif_sflags,
                                 IGMP_IF_SFLAG_SVC_TYPE_L2)
                   && (igif->igif_conf.igifc_cflags
                       || CHECK_FLAG (igif->igif_sflags,
                                      IGMP_IF_SFLAG_MCAST_ENABLED)))
            ret = igmp_if_start (igif, NULL);
        }
      break;

    case IGMP_IF_UPDATE_CONFIG_MROUTE_RESTART:
      if (CHECK_FLAG (igif->igif_sflags, IGMP_IF_SFLAG_ACTIVE))
        ret = igmp_if_stop (igif);

      UNSET_FLAG (igif->igif_conf.igifc_cflags,
                  IGMP_IF_CFLAG_MROUTE_PROXY);

      for (nn = LISTHEAD (&igif->igif_conf.igifc_mrouter_if_lst);
           nn; nn = next)
        {
          next = nn->next;

          if ((mrtr_igifidx = GETDATA (nn)))
            XFREE (MTYPE_IGMP_IF_IDX, mrtr_igifidx);
        }
      list_delete_all_node (&igif->igif_conf.igifc_mrouter_if_lst);

      if (ret == IGMP_ERR_NONE
          && if_is_up (igif->igif_owning_ifp)
          && if_is_running (igif->igif_owning_ifp))
        {
          if (if_is_vlan (igif->igif_owning_ifp)
              && igmp_if_snp_start (igif))
            ret = igmp_if_start (igif, NULL);
          else if (! CHECK_FLAG (igif->igif_sflags,
                            IGMP_IF_SFLAG_SVC_TYPE_L2))
            {
              if (igif->igif_conf.igifc_cflags
                  || CHECK_FLAG (igif->igif_sflags,
                                 IGMP_IF_SFLAG_MCAST_ENABLED))
                ret = igmp_if_start (igif, NULL);
              else if (! igif->igif_conf.igifc_cflags
                       && ! CHECK_FLAG (igif->igif_sflags,
                                        IGMP_IF_SFLAG_MCAST_ENABLED))
                ret = igmp_if_delete (igif);
            }
        }
      break;

    case IGMP_IF_UPDATE_CONFIG_PROXY_RESTART:
      if (CHECK_FLAG (igif->igif_sflags, IGMP_IF_SFLAG_ACTIVE))
        ret = igmp_if_stop (igif);

      UNSET_FLAG (igif->igif_conf.igifc_cflags,
                  IGMP_IF_CFLAG_PROXY_SERVICE);
   
      if (ret == IGMP_ERR_NONE
          && if_is_up (igif->igif_owning_ifp)
          && if_is_running (igif->igif_owning_ifp))
        {
          if (if_is_vlan (igif->igif_owning_ifp)
              && igmp_if_snp_start (igif))
            ret = igmp_if_start (igif, NULL);
          else if (! CHECK_FLAG (igif->igif_sflags,
                            IGMP_IF_SFLAG_SVC_TYPE_L2))
            {
              if (igif->igif_conf.igifc_cflags
                  || CHECK_FLAG (igif->igif_sflags,
                                 IGMP_IF_SFLAG_MCAST_ENABLED))
                ret = igmp_if_start (igif, NULL);
              else if (! igif->igif_conf.igifc_cflags
                       && ! CHECK_FLAG (igif->igif_sflags,
                                        IGMP_IF_SFLAG_MCAST_ENABLED))
                ret = igmp_if_delete(igif);
            }
        }
      break;

    case IGMP_IF_UPDATE_IFF_STATE:
      if (! if_is_up (igif->igif_owning_ifp)
          || ! if_is_running (igif->igif_owning_ifp))
        ret = igmp_if_stop (igif);
      else if (igmp_if_snp_start (igif)
               || ((igif->igif_conf.igifc_cflags
                    || CHECK_FLAG (igif->igif_sflags,
                                   IGMP_IF_SFLAG_MCAST_ENABLED))
                   && CHECK_FLAG (igif->igif_sflags,
                                  IGMP_IF_SFLAG_SVC_TYPE_L3)
                   && ! CHECK_FLAG (igif->igif_sflags,
                                    IGMP_IF_SFLAG_SVC_TYPE_L2)))
        ret = igmp_if_start (igif, NULL);
      break;

    case IGMP_IF_UPDATE_IFF_ADDR_ADD:
      if (if_is_vlan (igif->igif_owning_ifp))
        {
          if (CHECK_FLAG (igif->igif_sflags, IGMP_IF_SFLAG_ACTIVE))
            ret = igmp_if_stop (igif);

          if (ret == IGMP_ERR_NONE
              && igmp_if_snp_start (igif)
              && if_is_up (igif->igif_owning_ifp)
              && if_is_running (igif->igif_owning_ifp))
            ret = igmp_if_start (igif, NULL);
        }
      else if (! CHECK_FLAG (igif->igif_sflags, IGMP_IF_SFLAG_ACTIVE)
               && if_is_up (igif->igif_owning_ifp)
               && if_is_running (igif->igif_owning_ifp)
               && (CHECK_FLAG (igif->igif_sflags,
                               IGMP_IF_SFLAG_MCAST_ENABLED)
                   || igif->igif_conf.igifc_cflags))
        ret = igmp_if_start (igif, NULL);
      break;

    case IGMP_IF_UPDATE_IFF_ADDR_DEL:
      if (if_is_vlan (igif->igif_owning_ifp))
        {
          if (CHECK_FLAG (igif->igif_sflags, IGMP_IF_SFLAG_ACTIVE))
            ret = igmp_if_stop (igif);

          if (ret == IGMP_ERR_NONE
              && igmp_if_snp_start (igif)
              && if_is_up (igif->igif_owning_ifp)
              && if_is_running (igif->igif_owning_ifp))
            ret = igmp_if_start (igif, NULL);
        }
      else if (CHECK_FLAG (igif->igif_sflags, IGMP_IF_SFLAG_ACTIVE))
        ret = igmp_if_stop (igif);
      break;
    }

EXIT:

  IGMP_FN_EXIT (ret);
}

/* IGMP IF Address Add */
s_int32_t
igmp_if_addr_add (struct igmp_if *igif,
                  struct connected *ifc)
{
  struct igmp_instance *igi;
  s_int32_t ret;

  IGMP_FN_ENTER (IF Addr Add);

  ret = IGMP_ERR_NONE;

  if (! igif
      || ! ifc
      || ! ifc->address
      || ! (igi = igif->igif_owning_igi))
    {
      ret = IGMP_ERR_MALFORMED_ARG;
      goto EXIT;
    }

  if (CHECK_FLAG (ifc->flags, NSM_IFA_SECONDARY))
    {
      ret = IGMP_ERR_INVALID_VALUE;
      goto EXIT;
    }

  ret = igmp_if_update (igif, NULL, IGMP_IF_UPDATE_IFF_ADDR_ADD);

EXIT:

  IGMP_FN_EXIT (ret);
}

/* IGMP IF Address Delete */
s_int32_t
igmp_if_addr_del (struct igmp_if *igif,
                  struct connected *ifc)
{
  struct igmp_instance *igi;
  s_int32_t ret;

  IGMP_FN_ENTER (IF Addr Del);

  ret = IGMP_ERR_NONE;

  if (! igif
      || ! ifc
      || ! ifc->address
      || ! (igi = igif->igif_owning_igi))
    {
      ret = IGMP_ERR_MALFORMED_ARG;
      goto EXIT;
    }

  if (CHECK_FLAG (ifc->flags, NSM_IFA_SECONDARY))
    {
      ret = IGMP_ERR_INVALID_VALUE;
      goto EXIT;
    }

  if (igif->igif_paddr != ifc->address)
    {
      IGMP_DBG_INFO (igi, EVENTS, "Addr(%O) != Curr-Addr",
                     ifc->address);

      ret = IGMP_ERR_GENERIC;
      goto EXIT;
    }

  ret = igmp_if_update (igif, NULL, IGMP_IF_UPDATE_IFF_ADDR_DEL);

EXIT:

  IGMP_FN_EXIT (ret);
}

/* IGMP IF Join Group */
s_int32_t
igmp_if_group_join (struct igmp_if *igif,
                    struct igmp_svc_reg *igsr,
                    struct pal_in4_addr *group)
{
  s_int32_t ret;

  IGMP_FN_ENTER (IF Join Grp);

  ret = IGMP_ERR_NONE;

  if (! igif
      || ! igsr
      || ! igif->igif_paddr)
    {
      ret = IGMP_ERR_MALFORMED_ARG;
      goto EXIT;
    }

  ret = pal_sock_set_ipv4_multicast_join
        (igsr->igsr_sock, *group,
         igif->igif_paddr->u.prefix4,
         igif->igif_owning_ifp->ifindex);

EXIT:

  IGMP_FN_EXIT (ret);
}

/* IGMP IF Leave Group */
s_int32_t
igmp_if_group_leave (struct igmp_if *igif,
                     struct igmp_svc_reg *igsr,
                     struct pal_in4_addr *group)
{
  s_int32_t ret;

  IGMP_FN_ENTER (IF Leave Grp);

  ret = IGMP_ERR_NONE;

  if (! igif
      || ! igsr
      || ! igif->igif_paddr)
    {
      ret = IGMP_ERR_MALFORMED_ARG;
      goto EXIT;
    }

  ret = pal_sock_set_ipv4_multicast_leave
        (igsr->igsr_sock, *group,
         igif->igif_paddr->u.prefix4,
         igif->igif_owning_ifp->ifindex);

EXIT:

  IGMP_FN_EXIT (ret);
}

/* IGMP IF Group Record Create */
s_int32_t
igmp_if_grec_create (struct igmp_if *igif,
                     struct igmp_group_rec **pp_igr)
{
  struct igmp_instance *igi;
  s_int32_t ret;

  IGMP_FN_ENTER (Group Rec Create);

  ret = IGMP_ERR_NONE;

  if (! igif
      || ! pp_igr
      || ! (igi = igif->igif_owning_igi))
    {
      ret = IGMP_ERR_MALFORMED_ARG;
      goto EXIT;
    }

  (*pp_igr) = XCALLOC (MTYPE_IGMP_GRP_REC,
                       sizeof (struct igmp_group_rec));
  if (! (*pp_igr))
    {
      IGMP_DBG_INFO (igi, TIB,
                     "Cannot allocate memory (%d) @ %s:%d",
                     sizeof (struct igmp_group_rec),
                     __FILE__, __LINE__);

      ret = IGMP_ERR_OOM;
      goto EXIT;
    }

  /* Initialize the IGMP Group Record */
  (*pp_igr)->igr_owning_igif = igif;
  (*pp_igr)->igr_uptime = pal_time_current (NULL);
  (*pp_igr)->igr_filt_mode_state = IGMP_FMS_INCLUDE;
  (*pp_igr)->igr_src_a_tib = ptree_init (IPV4_MAX_PREFIXLEN);
  (*pp_igr)->igr_src_b_tib = ptree_init (IPV4_MAX_PREFIXLEN);
  (*pp_igr)->v_igr_liveness = igif->igif_gmi;

  ret = igmp_if_grec_update_cmode ((*pp_igr));

  if (ret < IGMP_ERR_NONE)
    {
      IGMP_DBG_INFO (igi, TIB, "Update Compat-Mode Failed(%d)", ret);

      goto EXIT;
    }

  /* Do not Run-up the Counter(s) for VLAN Interface in Snooping mode */
  if (CHECK_FLAG (igif->igif_sflags, IGMP_IF_SFLAG_SVC_TYPE_L2)
      && CHECK_FLAG (igif->igif_sflags, IGMP_IF_SFLAG_SVC_TYPE_L3)
      && CHECK_FLAG (igif->igif_sflags, IGMP_IF_SFLAG_SNOOPING))
    goto EXIT;

  /* Run-up the Num G-Recs Counter(s) */
  igif->igif_num_grecs += 1;

  if (CHECK_FLAG (igif->igif_sflags, IGMP_IF_SFLAG_SVC_TYPE_L2)
      && ! CHECK_FLAG (igif->igif_sflags, IGMP_IF_SFLAG_SVC_TYPE_L3)
      && igif->igif_hst_igif)
    igif->igif_hst_igif->igif_num_grecs += 1;

  igi->igi_num_grecs += 1;

EXIT:

  IGMP_FN_EXIT (ret);
}

s_int32_t
igmp_if_grec_rexmit_tib_delete (struct igmp_group_rec *igr)
{
  struct igmp_source_rec *isr;
  struct ptree_node *pn_next;
  struct igmp_instance *igi;
  struct ptree_node *pn;
  struct igmp_if *igif;
  s_int32_t ret;

  IGMP_FN_ENTER (Group Rec Del);

  ret = IGMP_ERR_NONE;

  if (! igr
      || ! (igif = igr->igr_owning_igif)
      || ! (igi = igif->igif_owning_igi))
    {
      ret = IGMP_ERR_MALFORMED_ARG;
      goto EXIT;
    }

  /* Release the Re-Transmission Sources P-Trie */
  if (igr->igr_rexmit_srcs_tib != igr->igr_src_a_tib
      && igr->igr_rexmit_srcs_tib != igr->igr_src_b_tib)
    {
      for (pn = ptree_top (igr->igr_rexmit_srcs_tib); pn; pn = pn_next)
        {
          pn_next = ptree_next (pn);

          if (! (isr = pn->info))
            continue;

          igmp_if_srec_delete (isr);

          igr->igr_rexmit_srcs_tib_count -= 1;
        }
      ptree_finish (igr->igr_rexmit_srcs_tib);
    }

  igr->igr_rexmit_srcs_tib_count = 0;
  igr->igr_rexmit_srcs_tib = NULL;
  
  for (pn = ptree_top (igr->igr_rexmit_allow_tib); pn; pn = pn_next)
    {
      pn_next = ptree_next (pn);
 
      if (! (isr = pn->info))
        continue;

      igmp_if_srec_delete (isr);

      igr->igr_rexmit_srcs_allow_tib_count -= 1;
    }
  ptree_finish (igr->igr_rexmit_allow_tib);
  
  igr->igr_rexmit_srcs_allow_tib_count = 0;
  igr->igr_rexmit_allow_tib = NULL;

  for (pn = ptree_top (igr->igr_rexmit_block_tib); pn; pn = pn_next)
    {
      pn_next = ptree_next (pn);

      if (! (isr = pn->info))
        continue;

      igmp_if_srec_delete (isr);

      igr->igr_rexmit_srcs_block_tib_count -= 1;
    }
  ptree_finish (igr->igr_rexmit_block_tib);

  igr->igr_rexmit_srcs_block_tib_count = 0;
  igr->igr_rexmit_block_tib = NULL;

EXIT:

  IGMP_FN_EXIT (ret);
}

/* IGMP IF Group Record Delete */
s_int32_t
igmp_if_grec_delete (struct igmp_group_rec *igr)
{
  struct igmp_source_rec *isr;
  struct ptree_node *pn_next;
  struct igmp_instance *igi;
  struct ptree_node *pn;
  struct igmp_if *igif;
  s_int32_t ret;

  IGMP_FN_ENTER (Group Rec Del);

  ret = IGMP_ERR_NONE;

  if (! igr
      || ! (igif = igr->igr_owning_igif)
      || ! (igi = igif->igif_owning_igi))
    {
      ret = IGMP_ERR_MALFORMED_ARG;
      goto EXIT;
    }

  /* Release the Re-Transmission Sources P-Trie */
  (void_t) igmp_if_grec_rexmit_tib_delete (igr);

  /* Release the Source P-Trie 'A' */
  for (pn = ptree_top (igr->igr_src_a_tib); pn; pn = pn_next)
    {
      pn_next = ptree_next (pn);

      if (! (isr = pn->info))
        continue;

      igmp_if_srec_delete (isr);

      igr->igr_src_a_tib_count -= 1;
    }
  ptree_finish (igr->igr_src_a_tib);

  igr->igr_src_a_tib = NULL;

  /* Release the Source P-Trie 'B' */
  for (pn = ptree_top (igr->igr_src_b_tib); pn; pn = pn_next)
    {
      pn_next = ptree_next (pn);

      if (! (isr = pn->info))
        continue;

      igmp_if_srec_delete (isr);

      igr->igr_src_b_tib_count -= 1;
    }
  ptree_finish (igr->igr_src_b_tib);

  igr->igr_src_b_tib = NULL;

  /* Cancel associated Timers */
  IGMP_TIMER_OFF (IGMP_LG (igi), igr->t_igr_liveness);
  IGMP_TIMER_OFF (IGMP_LG (igi), igr->t_igr_v1_host_present);
  IGMP_TIMER_OFF (IGMP_LG (igi), igr->t_igr_v2_host_present);
  IGMP_TIMER_OFF (IGMP_LG (igi), igr->t_igr_rexmit_group);
  IGMP_TIMER_OFF (IGMP_LG (igi), igr->t_igr_rexmit_group_source);
  IGMP_TIMER_OFF (IGMP_LG (igi), igr->t_igr_join_group);

  /* Release the owning P-Tree Node */
  IGMP_DBG_INFO (igi, TIB, "G=%r", PTREE_NODE_KEY (igr->igr_owning_pn));

  igr->igr_owning_pn->info = NULL;
  ptree_unlock_node (igr->igr_owning_pn);
  igr->igr_owning_pn = NULL;

  /* Release the Group Rec */
  XFREE (MTYPE_IGMP_GRP_REC, igr);

  /* Do not Count-down the Counter(s) for VLAN Interface in Snooping mode */
  if (CHECK_FLAG (igif->igif_sflags, IGMP_IF_SFLAG_SVC_TYPE_L2)
      && CHECK_FLAG (igif->igif_sflags, IGMP_IF_SFLAG_SVC_TYPE_L3)
      && CHECK_FLAG (igif->igif_sflags, IGMP_IF_SFLAG_SNOOPING))
    goto EXIT;

  /* Count-down the Num G-Recs Counter(s) */
  igif->igif_num_grecs -= 1;

  if (CHECK_FLAG (igif->igif_sflags, IGMP_IF_SFLAG_SVC_TYPE_L2)
      && ! CHECK_FLAG (igif->igif_sflags, IGMP_IF_SFLAG_SVC_TYPE_L3)
      && igif->igif_hst_igif)
    igif->igif_hst_igif->igif_num_grecs -= 1;

  igi->igi_num_grecs -= 1;

EXIT:

  IGMP_FN_EXIT (ret);
}

/* Get IGMP IF Router-side Group Membership Record */
struct igmp_group_rec *
igmp_if_grec_get (struct igmp_if *igif,
                  struct pal_in4_addr *pgrp)
{
  struct igmp_group_rec *igr;

  IGMP_FN_ENTER (Group Rec Get);

  igr = igmp_if_grec_get_tib (igif, igif->igif_gmr_tib, pgrp);

  IGMP_FN_EXIT (igr);
}

/* Get IGMP IF Host-side Group Membership Record */
struct igmp_group_rec *
igmp_if_hst_grec_get (struct igmp_if *igif,
                      struct pal_in4_addr *pgrp)
{
  struct igmp_group_rec *igr;

  IGMP_FN_ENTER (Host Group Rec Get);

  igr = igmp_if_grec_get_tib (igif, igif->igif_hst_gmr_tib, pgrp);

  IGMP_FN_EXIT (igr);
}

/* Update IGMP IF Host-side Group Membership Record */
s_int32_t
igmp_if_hst_grec_update (struct igmp_if *igif,
                         struct pal_in4_addr *pgrp, 
                         struct igmp_group_rec *igr)
{
  struct igmp_group_rec *hst_igr;
  s_int32_t ret;

  IGMP_FN_ENTER (Host Group Rec Update);

  hst_igr = NULL;
  ret = IGMP_ERR_NONE;
		
  if (! igif
      || ! pgrp 
      || ! igr )
    {
      ret = IGMP_ERR_MALFORMED_ARG;
      goto EXIT;
    }
	
  hst_igr = igmp_if_hst_grec_lookup(igif->igif_hst_igif, pgrp);
  
  if ( ! hst_igr )
    {
      ret = IGMP_ERR_NO_SUCH_GROUP_REC;
      goto EXIT;
    }
  else
    hst_igr->igr_last_reporter = igr->igr_last_reporter;

EXIT:

  IGMP_FN_EXIT (ret);
}

/* Get IGMP Group Membership Record on specified TIB */
struct igmp_group_rec *
igmp_if_grec_get_tib (struct igmp_if *igif,
                      struct ptree *gmr_tib,
                      struct pal_in4_addr *pgrp)
{
  struct igmp_group_rec *igr;
  struct ptree_node *pn;
  s_int32_t ret;

  IGMP_FN_ENTER (Group Rec Get TIB);

  ret = IGMP_ERR_NONE;
  igr = NULL;

  if (! igif
      || ! pgrp
      || ! gmr_tib
      || (igif->igif_gmr_tib != gmr_tib
          && igif->igif_hst_gmr_tib != gmr_tib))
    goto EXIT;

  pn = ptree_node_get (gmr_tib, (u_int8_t *) pgrp,
                       IPV4_MAX_PREFIXLEN);

  if (! pn)
    {
      IGMP_DBG_INFO (igif->igif_owning_igi, TIB,
                     "Failed to get P-Trie node for I=%s,G=%r",
                     igif->igif_owning_ifp->name, pgrp);

      goto EXIT;
    }

  if (! (igr = pn->info))
    {
      ret = igmp_if_grec_create (igif, &igr);

      if (ret < IGMP_ERR_NONE)
        {
          IGMP_DBG_INFO (igif->igif_owning_igi, TIB,
                         "Failed to create Group-Rec for I=%s,G=%r",
                         igif->igif_owning_ifp->name, pgrp);

          goto EXIT;
        }

      /* Complete the IGMP Group-Rec Initialization */
      pn->info = igr;
      igr->igr_owning_pn = pn;
    }
  else
    ptree_unlock_node (pn);

EXIT:

  IGMP_FN_EXIT (igr);
}

/* Lookup IGMP IF Router-side Group Membership Record */
struct igmp_group_rec *
igmp_if_grec_lookup (struct igmp_if *igif,
                     struct pal_in4_addr *pgrp)
{
  struct igmp_group_rec *igr;

  IGMP_FN_ENTER (Group Rec Lkup);

  igr = igmp_if_grec_lookup_tib (igif, igif->igif_gmr_tib, pgrp);

  IGMP_FN_EXIT (igr);
}

/* Lookup next IGMP IF Router-side Group Membership Record */
struct igmp_group_rec *
igmp_if_grec_lookup_next (struct igmp_if *igif,
                          struct pal_in4_addr *pgrp)
{
  struct igmp_group_rec *igr;
  struct ptree_node *pn;

  IGMP_FN_ENTER (Group Rec Lkup Nxt);

  pn = NULL;
  igr = NULL;

  if (! igif)
    goto EXIT;

  if (pgrp)
    {
      igr = igmp_if_grec_lookup (igif, pgrp);

      if (igr)
        {
          pn = ptree_lock_node (igr->igr_owning_pn);

          pn = ptree_next (pn);
        }
    }

  if (! igr || ! pgrp)
    pn = ptree_top (igif->igif_gmr_tib);

  for (igr = NULL; pn; pn = ptree_next (pn))
    if ((igr = (pn->info)))
      break;

EXIT:

  IGMP_FN_EXIT (igr);
}

/* Lookup IGMP IF Host-side Group Membership Record */
struct igmp_group_rec *
igmp_if_hst_grec_lookup (struct igmp_if *igif,
                         struct pal_in4_addr *pgrp)
{
  struct igmp_group_rec *igr;

  IGMP_FN_ENTER (Host Group Rec Lkup);

  igr = igmp_if_grec_lookup_tib (igif, igif->igif_hst_gmr_tib, pgrp);

  IGMP_FN_EXIT (igr);
}

/* Lookup IGMP Group Membership Record on specified TIB */
struct igmp_group_rec *
igmp_if_grec_lookup_tib (struct igmp_if *igif,
                         struct ptree *gmr_tib,
                         struct pal_in4_addr *pgrp)
{
  struct igmp_group_rec *igr;
  struct ptree_node *pn;

  IGMP_FN_ENTER (Group Rec Lkup TIB);

  igr = NULL;

  if (! igif
      || ! pgrp
      || ! gmr_tib
      || (igif->igif_gmr_tib != gmr_tib
          && igif->igif_hst_gmr_tib != gmr_tib))
    goto EXIT;

  pn = ptree_node_lookup (gmr_tib, (u_int8_t *) pgrp,
                          IPV4_MAX_PREFIXLEN);
  if (pn)
    {
      igr = pn->info;
      ptree_unlock_node (pn);
    }

EXIT:

  IGMP_FN_EXIT (igr);
}

/* IGMP IF Group Record Update */
s_int32_t
igmp_if_grec_update (struct igmp_group_rec *igr,
                     u_int32_t max_rsp_time)
{
  struct pal_timeval timer_now;
  struct igmp_instance *igi;
  u_int32_t time_remain;
  struct igmp_if *igif;
  s_int32_t ret;

  IGMP_FN_ENTER (Group Rec Update);

  ret = IGMP_ERR_NONE;
  time_remain = 0;

  if (! igr
      || ! (igif = igr->igr_owning_igif)
      || ! (igi = igif->igif_owning_igi))
    {
      ret = IGMP_ERR_MALFORMED_ARG;
      goto EXIT;
    }

  IGMP_DBG_INFO (igif->igif_owning_igi, EVENTS, "Grp %r on %s",
                 PTREE_NODE_KEY (igr->igr_owning_pn),
                 igif->igif_owning_ifp->name);

  /* Calculate the LMQT in seconds */
  igr->v_igr_liveness = max_rsp_time * igif->igif_conf.igifc_lmqc;

  if (igr->t_igr_liveness)
    {
      pal_time_tzcurrent (&timer_now, NULL);

      time_remain = thread_timer_remain_second (igr->t_igr_liveness);
    }

  /* Lower to LMQT if necessary */
  if (! igr->t_igr_liveness
      || time_remain > igr->v_igr_liveness)
    {
      IGMP_TIMER_OFF (IGMP_LG (igi), igr->t_igr_liveness);
      if (CHECK_FLAG (igr->igr_sflags, IGMP_IGR_SFLAG_DYNAMIC))
        IGMP_TIMER_ON (IGMP_LG (igi), igr->t_igr_liveness, igr,
                       igmp_if_grec_timer_liveness,
                       igr->v_igr_liveness);
    }

EXIT:

  IGMP_FN_EXIT (ret);
}

/* IGMP IF Update Group Record State Refresh */
s_int32_t
igmp_if_grec_update_state_refresh (struct igmp_if *igif,
                                   struct pal_in4_addr *pgrp,
                                   bool_t sr_on)
{
  struct igmp_group_rec *igr;
  s_int32_t ret;

  IGMP_FN_ENTER (Group Rec State Refresh Update);

  ret = 0;

  igr = igmp_if_grec_lookup (igif, pgrp);
   if (! igr)
     {
        IGMP_DBG_INFO (igif->igif_owning_igi, DECODE,
                       "No Group Rec for %r on %s",
                       pgrp, igif->igif_owning_ifp->name);

        ret = 0;
        goto EXIT;
     }

 if (sr_on == PAL_TRUE)
   SET_FLAG (igr->igr_sflags, IGMP_IGR_SFLAG_STATE_REFRESH);
 else
   UNSET_FLAG (igr->igr_sflags, IGMP_IGR_SFLAG_STATE_REFRESH);

EXIT:

  IGMP_FN_EXIT (ret);
}


/* IGMP IF Update Group Record Compatibility-Mode */
s_int32_t
igmp_if_grec_update_cmode (struct igmp_group_rec *igr)
{
  struct igmp_if *igif;
  s_int32_t ret;

  IGMP_FN_ENTER (Group Rec Update C-Mode);

  igif = igr->igr_owning_igif;
  ret = IGMP_ERR_NONE;

  switch (igif->igif_conf.igifc_version)
    {
    case IGMP_VERSION_1:
      UNSET_FLAG (igr->igr_sflags, (IGMP_IGR_SFLAG_COMPAT_V2
                                    | IGMP_IGR_SFLAG_COMPAT_V3));
      SET_FLAG (igr->igr_sflags, IGMP_IGR_SFLAG_COMPAT_V1);
      break;

    case IGMP_VERSION_2:
      UNSET_FLAG (igr->igr_sflags, (IGMP_IGR_SFLAG_COMPAT_V1
                                    | IGMP_IGR_SFLAG_COMPAT_V2
                                    | IGMP_IGR_SFLAG_COMPAT_V3));

      if (igr->t_igr_v1_host_present)
        SET_FLAG (igr->igr_sflags, IGMP_IGR_SFLAG_COMPAT_V1);
      else
        SET_FLAG (igr->igr_sflags, IGMP_IGR_SFLAG_COMPAT_V2);
      break;

    case IGMP_VERSION_3:
      UNSET_FLAG (igr->igr_sflags, (IGMP_IGR_SFLAG_COMPAT_V1
                                    | IGMP_IGR_SFLAG_COMPAT_V2
                                    | IGMP_IGR_SFLAG_COMPAT_V3));

      if (igr->t_igr_v1_host_present)
        SET_FLAG (igr->igr_sflags, IGMP_IGR_SFLAG_COMPAT_V1);
      else if (igr->t_igr_v2_host_present)
        SET_FLAG (igr->igr_sflags, IGMP_IGR_SFLAG_COMPAT_V2);
      else
        SET_FLAG (igr->igr_sflags, IGMP_IGR_SFLAG_COMPAT_V3);
      break;

    default:
      ret = IGMP_ERR_DOOM;
      goto EXIT;
    }

EXIT:

  IGMP_FN_EXIT (ret);
}

/* IGMP IF Update Group Record Configuration */
s_int32_t
igmp_if_grec_update_static (struct igmp_group_rec *igr,
                            struct igmp_source_list *isl,
                            bool_t is_igr_add)
{
  s_int32_t ret;

  IGMP_FN_ENTER (IF Grec Update);

  ret = IGMP_ERR_NONE;

  if (! igr || ! isl)
    {
      ret = IGMP_ERR_MALFORMED_ARG;
      goto EXIT;
    }

  if (is_igr_add)
    {
      if (! isl->isl_num)
        ret = igmp_igr_fsm_process_event (igr, IGMP_FME_CHG_TO_EXCL, isl);
      else
        ret = igmp_igr_fsm_process_event (igr, IGMP_FME_CHG_TO_INCL, isl);
    }
   else
    {
      if (! isl->isl_num)
        ret = igmp_igr_fsm_process_event (igr, IGMP_FME_CHG_TO_INCL, isl);
      else
        ret = igmp_igr_fsm_process_event (igr, IGMP_FME_BLOCK_OLD_SRCS, isl);
    }

EXIT:
  IGMP_FN_EXIT (ret);
}

/* IGMP IF Source Record Create */
s_int32_t
igmp_if_srec_create (struct igmp_group_rec *igr,
                     struct igmp_source_rec **pp_isr)
{
  struct igmp_instance *igi;
  s_int32_t ret;

  IGMP_FN_ENTER (Source Rec Create);

  ret = IGMP_ERR_NONE;

  if (! igr || ! pp_isr)
    {
      ret = IGMP_ERR_MALFORMED_ARG;
      goto EXIT;
    }

  igi = igr->igr_owning_igif->igif_owning_igi;

  (*pp_isr) = XCALLOC (MTYPE_IGMP_SRC_REC,
                       sizeof (struct igmp_source_rec));
  if (! (*pp_isr))
    {
      IGMP_DBG_INFO (igi, TIB, "Cannot allocate memory (%d) @ %s:%d",
                     sizeof (struct igmp_source_rec),
                     __FILE__, __LINE__);

      ret = IGMP_ERR_OOM;
      goto EXIT;
    }

  /* Initialize the IGMP Group Record */
  (*pp_isr)->isr_owning_igr = igr;
  (*pp_isr)->v_isr_liveness = igr->v_igr_liveness;
  (*pp_isr)->isr_uptime = pal_time_current (NULL);

EXIT:

  IGMP_FN_EXIT (ret);
}

/* IGMP IF Source Record Delete */
s_int32_t
igmp_if_srec_delete (struct igmp_source_rec *isr)
{
  struct igmp_instance *igi;
  s_int32_t ret;

  IGMP_FN_ENTER (Source Rec Del);

  igi = isr->isr_owning_igr->igr_owning_igif->igif_owning_igi;
  ret = IGMP_ERR_NONE;

  /* Turn of the Source Timer */
  IGMP_TIMER_OFF (IGMP_LG (igi), isr->t_isr_liveness);

  /* Release the owning P-Tree Node */
  if (isr->isr_owning_pn)
    {
      IGMP_DBG_INFO (igi, TIB, "S=%r",
                     PTREE_NODE_KEY (isr->isr_owning_pn));

      isr->isr_owning_pn->info = NULL;
      ptree_unlock_node (isr->isr_owning_pn);
      isr->isr_owning_pn = NULL;
    }

  /* Release the Source Rec */
  XFREE (MTYPE_IGMP_SRC_REC, isr);

  IGMP_FN_EXIT (ret);
}

/* Get IGMP Source Membership Record on the Interface */
struct igmp_source_rec *
igmp_if_srec_get (struct igmp_group_rec *igr,
                  bool_t use_tib_a,
                  struct pal_in4_addr *msrc)
{
  struct igmp_source_rec *isr;
  struct ptree_node *pn;
  s_int32_t ret;

  IGMP_FN_ENTER (Source Get);

  isr = NULL;

  if (! igr || ! msrc)
    goto EXIT;

  pn = ptree_node_get (use_tib_a == PAL_TRUE ? igr->igr_src_a_tib :
                       igr->igr_src_b_tib, (u_int8_t *) msrc,
                       IPV4_MAX_PREFIXLEN);

  if (! pn)
    {
      IGMP_DBG_INFO (igr->igr_owning_igif->igif_owning_igi, TIB,
                     "Failed to get P-Trie node for %r on I=%s,G=%r,TIB-%s",
                     msrc, igr->igr_owning_igif->igif_owning_ifp->name,
                     PTREE_NODE_KEY (igr->igr_owning_pn),
                     use_tib_a == PAL_TRUE ? "A" : "B");

      goto EXIT;
    }

  if (! (isr = pn->info))
    {
      ret = igmp_if_srec_create (igr, &isr);

      if (ret < IGMP_ERR_NONE)
        {
          IGMP_DBG_INFO (igr->igr_owning_igif->igif_owning_igi, TIB,
                         "Failed to create Src node for %r on "
                         "I=%s,G=%r,TIB-%s", msrc,
                         igr->igr_owning_igif->igif_owning_ifp->name,
                         PTREE_NODE_KEY (igr->igr_owning_pn),
                         use_tib_a == PAL_TRUE ? "A" : "B");

          goto EXIT;
        }

      /* Complete the IGMP Source-Rec Initialization */
      pn->info = isr;
      isr->isr_owning_pn = pn;

      /* Account for the Source-Rec */
      if (use_tib_a)
        igr->igr_src_a_tib_count += 1;
      else
        igr->igr_src_b_tib_count += 1;
    }
  else
    ptree_unlock_node (pn);

EXIT:

  IGMP_FN_EXIT (isr);
}

/* Lookup IGMP Source Membership Record on the Interface */
struct igmp_source_rec *
igmp_if_srec_lookup (struct igmp_group_rec *igr,
                     bool_t use_tib_a,
                     struct pal_in4_addr *msrc)
{
  struct igmp_source_rec *isr;
  struct ptree_node *pn;

  IGMP_FN_ENTER (Source Lkup);

  isr = NULL;

  if (! igr || ! msrc)
    goto EXIT;

  pn = ptree_node_lookup (use_tib_a == PAL_TRUE ? igr->igr_src_a_tib :
                          igr->igr_src_b_tib, (u_int8_t *) msrc,
                          IPV4_MAX_PREFIXLEN);

  if (pn)
    {
      isr = pn->info;
      ptree_unlock_node (pn);
    }

EXIT:

  IGMP_FN_EXIT (isr);
}

/* Lookup next IGMP Source Membership Record on the Interface  */
struct igmp_source_rec *
igmp_if_srec_lookup_next (struct igmp_group_rec *igr,
                          struct pal_in4_addr *msrc)
{
  struct igmp_source_rec *isr;
  struct ptree_node *pn;

  IGMP_FN_ENTER (Source Rec Lkup Nxt);

  pn = NULL;
  isr = NULL;

  if (! igr)
    goto EXIT;

  if (msrc)
    {
      pn = ptree_node_lookup (igr->igr_src_a_tib, (u_int8_t *) msrc,
                              IPV4_MAX_PREFIXLEN);
      if (! pn)
        pn = ptree_node_lookup (igr->igr_src_b_tib, (u_int8_t *) msrc,
                                IPV4_MAX_PREFIXLEN);
      if (pn)
        pn = ptree_next (pn);
    }

  if (! msrc || ! (msrc->s_addr))
    {
      pn = ptree_top (igr->igr_src_a_tib);
      if (! pn)
        pn = ptree_top (igr->igr_src_b_tib);
    }

  for (isr = NULL; pn; pn = ptree_next (pn))
    if ((isr = (pn->info)))
      break;

EXIT:

  IGMP_FN_EXIT (isr);
}

/* Count number of Static/Dynamic sources defined by sflag
 * in the tib specified for a group record.
 */
u_int16_t
igmp_igr_srec_count (struct igmp_group_rec *igr,
                     bool_t use_tib_a,
                     u_int16_t sflag)
{
  struct ptree *igr_src_tib;
  struct igmp_source_rec *isr;
  struct ptree_node *pn_src;
  u_int16_t count;

  IGMP_FN_ENTER (Source Count);

  isr = NULL;
  pn_src = NULL;
  count = 0;

  if (! igr)
    goto EXIT;

  igr_src_tib = (use_tib_a == PAL_TRUE ? igr->igr_src_a_tib :
                          igr->igr_src_b_tib);

  for (pn_src = ptree_top (igr_src_tib);
       pn_src; pn_src = ptree_next (pn_src))
    {
      if (! (isr = pn_src->info))
        continue;

      if (CHECK_FLAG (isr->isr_sflags, sflag))
        count += 1;
    }

EXIT:

  IGMP_FN_EXIT (count);
}

/* IGMP IF Source Membership Record Update */
s_int32_t
igmp_if_srec_update (struct igmp_source_rec *isr,
                     u_int32_t max_rsp_time)
{
  struct pal_timeval timer_now;
  struct igmp_group_rec *igr;
  struct igmp_instance *igi;
  u_int32_t time_remain;
  s_int32_t ret;

  IGMP_FN_ENTER (Src Rec Update);

  ret = IGMP_ERR_NONE;
  time_remain = 0;

  if (! isr
      || ! (igr = isr->isr_owning_igr)
      || ! (igi = igr->igr_owning_igif->igif_owning_igi))
    {
      ret = IGMP_ERR_MALFORMED_ARG;
      goto EXIT;
    }

  IGMP_DBG_INFO (igi, DECODE, "Src-Rec %r in group %r on %s",
                 PTREE_NODE_KEY (isr->isr_owning_pn),
                 PTREE_NODE_KEY (igr->igr_owning_pn),
                 igr->igr_owning_igif->igif_owning_ifp->name);

  /* Calculate the LMQT in seconds */
  isr->v_isr_liveness = max_rsp_time *
                        igr->igr_owning_igif->igif_conf.igifc_lmqc;

  if (isr->t_isr_liveness)
    {
      pal_time_tzcurrent (&timer_now, NULL);

      time_remain = thread_timer_remain_second (isr->t_isr_liveness);
    }

  /* Lower to LMQT if necessary */
  if (! isr->t_isr_liveness
      || time_remain > isr->v_isr_liveness)
    {
      IGMP_TIMER_OFF (IGMP_LG (igi), isr->t_isr_liveness);
      IGMP_TIMER_ON (IGMP_LG (igi), isr->t_isr_liveness,
                     isr, igmp_if_srec_timer_liveness,
                     isr->v_isr_liveness);
    }

EXIT:

  IGMP_FN_EXIT (ret);
}

/* IGMP IF Group-Rec V1 Host Present Timer Handler */
s_int32_t
igmp_if_grec_timer_v1_host_present (struct thread *t_timer)
{
  struct igmp_group_rec *igr;
  struct lib_globals *iglg;
  s_int32_t ret;

  IGMP_FN_ENTER (V1 Host Present Timer);

  ret = IGMP_ERR_NONE;

  /* Obtain the IGMP Lib Global pointer */
  iglg = THREAD_GLOB (t_timer);

  /* Sanity check */
  if (! iglg)
    {
      ret = IGMP_ERR_MALFORMED_ARG;
      goto EXIT;
    }

  /* Obtain the IGMP Group-Rec structure */
  igr = THREAD_ARG (t_timer);

  /* Sanity check */
  if (! igr)
    {
      ret = IGMP_ERR_MALFORMED_ARG;
      goto EXIT;
    }

  igr->t_igr_v1_host_present = NULL;

  IGMP_DBG_INFO (igr->igr_owning_igif->igif_owning_igi, EVENTS,
                 "Exipry for Grp %r on %s",
                 PTREE_NODE_KEY (igr->igr_owning_pn),
                 igr->igr_owning_igif->igif_owning_ifp->name);

  ret = igmp_if_grec_update_cmode (igr);

EXIT:

  IGMP_FN_EXIT (ret);
}

/* IGMP IF Group-Rec V2 Host Present Timer Handler */
s_int32_t
igmp_if_grec_timer_v2_host_present (struct thread *t_timer)
{
  struct igmp_group_rec *igr;
  struct lib_globals *iglg;
  s_int32_t ret;

  IGMP_FN_ENTER (V2 Host Present Timer);

  ret = IGMP_ERR_NONE;

  /* Obtain the IGMP Lib Global pointer */
  iglg = THREAD_GLOB (t_timer);

  /* Sanity check */
  if (! iglg)
    {
      ret = IGMP_ERR_MALFORMED_ARG;
      goto EXIT;
    }

  /* Obtain the IGMP Group-Rec structure */
  igr = THREAD_ARG (t_timer);

  /* Sanity check */
  if (! igr)
    {
      ret = IGMP_ERR_MALFORMED_ARG;
      goto EXIT;
    }

  igr->t_igr_v2_host_present = NULL;

  IGMP_DBG_INFO (igr->igr_owning_igif->igif_owning_igi, EVENTS,
                 "Exipry for Grp %r on %s",
                 PTREE_NODE_KEY (igr->igr_owning_pn),
                 igr->igr_owning_igif->igif_owning_ifp->name);

  ret = igmp_if_grec_update_cmode (igr);

EXIT:

  IGMP_FN_EXIT (ret);
}

/* IGMP IF Group Query Retransmission Timer Handler */
s_int32_t
igmp_if_grec_timer_rexmit_group (struct thread *t_timer)
{
  struct igmp_group_rec *igr;
  struct lib_globals *iglg;
  s_int32_t ret;

  IGMP_FN_ENTER (Grp Query Rexmit);

  ret = IGMP_ERR_NONE;

  /* Obtain the IGMP Lib Global pointer */
  iglg = THREAD_GLOB (t_timer);

  /* Sanity check */
  if (! iglg)
    {
      ret = IGMP_ERR_MALFORMED_ARG;
      goto EXIT;
    }

  /* Obtain the IGMP Group-Rec structure */
  igr = THREAD_ARG (t_timer);

  /* Sanity check */
  if (! igr)
    {
      ret = IGMP_ERR_MALFORMED_ARG;
      goto EXIT;
    }

  igr->t_igr_rexmit_group = NULL;

  IGMP_DBG_INFO (igr->igr_owning_igif->igif_owning_igi, EVENTS,
                 "Exipry for Grp %r on %s",
                 PTREE_NODE_KEY (igr->igr_owning_pn),
                 igr->igr_owning_igif->igif_owning_ifp->name);

  if (--igr->igr_rexmit_group_lmqc)
    {
      ret = igmp_if_send_group_query (igr);

      if (ret < IGMP_ERR_NONE)
        IGMP_DBG_WARN (igr->igr_owning_igif->igif_owning_igi, EVENTS,
                       "Group Query Rexmit failed(%d)", ret);
    }

EXIT:

  IGMP_FN_EXIT (ret);
}

/* IGMP IF Group-Source Query Retransmission Timer Handler */
s_int32_t
igmp_if_grec_timer_rexmit_group_source (struct thread *t_timer)
{
  struct igmp_group_rec *igr;
  struct lib_globals *iglg;
  s_int32_t ret;

  IGMP_FN_ENTER (Grp-Src Query Rexmit);

  ret = IGMP_ERR_NONE;

  /* Obtain the IGMP Lib Global pointer */
  iglg = THREAD_GLOB (t_timer);

  /* Sanity check */
  if (! iglg)
    {
      ret = IGMP_ERR_MALFORMED_ARG;
      goto EXIT;
    }

  /* Obtain the IGMP IF structure */
  igr = THREAD_ARG (t_timer);

  /* Sanity check */
  if (! igr)
    {
      ret = IGMP_ERR_MALFORMED_ARG;
      goto EXIT;
    }

  igr->t_igr_rexmit_group_source = NULL;

  IGMP_DBG_INFO (igr->igr_owning_igif->igif_owning_igi, EVENTS,
                 "Exipry for Grp %r on %s",
                 PTREE_NODE_KEY (igr->igr_owning_pn),
                 igr->igr_owning_igif->igif_owning_ifp->name);

  if (--igr->igr_rexmit_group_source_lmqc)
    {
      ret = igmp_if_send_group_source_query (igr, igr->igr_rexmit_srcs_tib,
                                             igr->igr_rexmit_srcs_tib_count);

      if (ret < IGMP_ERR_NONE)
        IGMP_DBG_WARN (igr->igr_owning_igif->igif_owning_igi, EVENTS,
                       "Group-Source Query Rexmit failed(%d)", ret);
    }
  /* Discard the Re-Transmission TIB */
  else if (igr->igr_rexmit_srcs_tib_count)
    (void_t) igmp_if_grec_rexmit_tib_delete (igr);

EXIT:

  IGMP_FN_EXIT (ret);
}

/* IGMP Group Record Liveness Timer */
s_int32_t
igmp_if_grec_timer_liveness (struct thread *t_timer)
{
  struct igmp_group_rec *igr;
  struct igmp_instance *igi;
  struct lib_globals *iglg;
  s_int32_t ret;

  IGMP_FN_ENTER (Grp-Rec Liveness Timer);

  ret = IGMP_ERR_NONE;

  /* Obtain the IGMP Lib Global pointer */
  iglg = THREAD_GLOB (t_timer);

  /* Sanity check */
  if (! iglg)
    {
      ret = IGMP_ERR_MALFORMED_ARG;
      goto EXIT;
    }

  /* Obtain the IGMP Group-Rec structure */
  igr = THREAD_ARG (t_timer);

  /* Sanity check */
  if (! igr)
    {
      ret = IGMP_ERR_MALFORMED_ARG;
      goto EXIT;
    }

  igi = igr->igr_owning_igif->igif_owning_igi;

  igr->t_igr_liveness = NULL;

  IGMP_DBG_INFO (igi, EVENTS, "Exipry for Grp %r on %s",
                 PTREE_NODE_KEY (igr->igr_owning_pn),
                 igr->igr_owning_igif->igif_owning_ifp->name);

  /* Invoke FSM to process the Timer Expiry */
  ret = igmp_igr_fsm_process_event (igr, IGMP_FME_GROUP_TIMER_EXPIRY, NULL);

EXIT:

  IGMP_FN_EXIT (ret);
}

/* IGMP Source Record Liveness Timer */
s_int32_t
igmp_if_srec_timer_liveness (struct thread *t_timer)
{
  struct igmp_source_list isl;
  struct igmp_source_rec *isr;
  struct igmp_instance *igi;
  struct lib_globals *iglg;
  s_int32_t ret;

  IGMP_FN_ENTER (Src-Rec Liveness Timer);

  ret = IGMP_ERR_NONE;

  /* Obtain the IGMP Lib Global pointer */
  iglg = THREAD_GLOB (t_timer);

  /* Sanity check */
  if (! iglg)
    {
      ret = IGMP_ERR_MALFORMED_ARG;
      goto EXIT;
    }

  /* Obtain the IGMP Source-Rec structure */
  isr = THREAD_ARG (t_timer);

  /* Sanity check */
  if (! isr)
    {
      ret = IGMP_ERR_MALFORMED_ARG;
      goto EXIT;
    }

  igi = isr->isr_owning_igr->igr_owning_igif->igif_owning_igi;

  isr->t_isr_liveness = NULL;

  IGMP_DBG_INFO (igi, EVENTS, "Exipry for Src %r on %s",
                 PTREE_NODE_KEY (isr->isr_owning_pn),
                 isr->isr_owning_igr->igr_owning_igif->igif_owning_ifp->name);

  /* Prepare a Source List with single source */
  pal_mem_set (&isl, 0, sizeof (struct igmp_source_list));
  isl.isl_num += 1;
  IPV4_ADDR_COPY (&isl.isl_saddr [0], PTREE_NODE_KEY (isr->isr_owning_pn));

  /* Invoke FSM to process the Timer Expiry */
  ret = igmp_igr_fsm_process_event (isr->isr_owning_igr,
                                    IGMP_FME_SOURCE_TIMER_EXPIRY, &isl);

EXIT:

  IGMP_FN_EXIT (ret);
}

/* IGMP IF Generic Timer Jitter Generator */
u_int32_t
igmp_if_timer_generate_jitter (u_int32_t time)
{
  u_int32_t rand_val;

  IGMP_FN_ENTER (Gen Jitter);

  rand_val = (u_int32_t) (100.0 * pal_rand () / RAND_MAX);

  rand_val = rand_val * time / 100;

  IGMP_FN_EXIT (rand_val);
}

/* IGMP Interface Warnings Rate-Limiter Timer */
s_int32_t
igmp_if_timer_warn_rlimit (struct thread *t_timer)
{
  struct lib_globals *iglg;
  struct igmp_if *igif;
  s_int32_t ret;

  IGMP_FN_ENTER (Warn R-Limit Timer);

  ret = IGMP_ERR_NONE;

  /* Obtain the IGMP Lib Global pointer */
  iglg = THREAD_GLOB (t_timer);

  /* Sanity check */
  if (! iglg)
    {
      ret = IGMP_ERR_MALFORMED_ARG;
      goto EXIT;
    }

  /* Obtain the IGMP IF structure */
  igif = THREAD_ARG (t_timer);

  /* Sanity check */
  if (! igif)
    {
      ret = IGMP_ERR_MALFORMED_ARG;
      goto EXIT;
    }

  igif->t_igif_warn_rlimit = NULL;

  IGMP_DBG_INFO (igif->igif_owning_igi, EVENTS, "Exipry on %s",
                 igif->igif_owning_ifp->name);

  if (igif->igif_v1_querier_wcount)
    IGMP_DBG_WARN (igif->igif_owning_igi, EVENTS,
                   "%u V1 Query Msgs received on %s configured for V%u%s",
                   igif->igif_v1_querier_wcount,
                   igif->igif_owning_ifp->name,
                   igif->igif_conf.igifc_version,
                   CHECK_FLAG (igif->igif_conf.igifc_cflags,
                               IGMP_IF_CFLAG_PROXY_SERVICE) ?
                   " Proxy-Service" : "");

  if (igif->igif_v2_querier_wcount)
    IGMP_DBG_WARN (igif->igif_owning_igi, EVENTS,
                   "%u V2 Query Msgs received on %s configured for V%u%s",
                   igif->igif_v2_querier_wcount,
                   igif->igif_owning_ifp->name,
                   igif->igif_conf.igifc_version,
                   CHECK_FLAG (igif->igif_conf.igifc_cflags,
                               IGMP_IF_CFLAG_PROXY_SERVICE) ?
                   " Proxy-Service" : "");

  if (igif->igif_v3_querier_wcount)
    IGMP_DBG_WARN (igif->igif_owning_igi, EVENTS,
                   "%u V3 Query Msgs received on %s configured for V%u%s",
                   igif->igif_v3_querier_wcount,
                   igif->igif_owning_ifp->name,
                   igif->igif_conf.igifc_version,
                   CHECK_FLAG (igif->igif_conf.igifc_cflags,
                               IGMP_IF_CFLAG_PROXY_SERVICE) ?
                   " Proxy-Service" : "");

  /* Reset the Warning Counters */
  igif->igif_v1_querier_wcount = 0;
  igif->igif_v2_querier_wcount = 0;
  igif->igif_v3_querier_wcount = 0;

EXIT:

  IGMP_FN_EXIT (ret);
}

/* Querier Timer handler */
s_int32_t
igmp_if_timer_querier (struct thread *t_timer)
{
  u_int32_t querier_interval;
  struct lib_globals *iglg;
  struct igmp_if *igif;
  s_int32_t ret;

  IGMP_FN_ENTER (Querier Timer);

  ret = IGMP_ERR_NONE;

  /* Obtain the IGMP Lib Global pointer */
  iglg = THREAD_GLOB (t_timer);

  /* Sanity check */
  if (! iglg)
    {
      ret = IGMP_ERR_MALFORMED_ARG;
      goto EXIT;
    }

  /* Obtain the IGMP IF structure */
  igif = THREAD_ARG (t_timer);

  /* Sanity check */
  if (! igif)
    {
      ret = IGMP_ERR_MALFORMED_ARG;
      goto EXIT;
    }

  igif->t_igif_querier = NULL;

  IGMP_DBG_INFO (igif->igif_owning_igi, EVENTS, "Exipry on %s",
                 igif->igif_owning_ifp->name);

  /* Encode and send General Query */
  ret = igmp_if_send_general_query (igif);

  if (ret < IGMP_ERR_NONE)
    IGMP_DBG_WARN (igif->igif_owning_igi, EVENTS,
                   "Sending General Query failed(%d)", ret);

  /* Determine Querier Timer Interval */
  if (igif->igif_sqc - 1)
    {
      igif->igif_sqc -= 1;
      if (CHECK_FLAG(igif->igif_conf.igifc_cflags,
                     IGMP_IF_CFLAG_STARTUP_QUERY_INTERVAL))
        querier_interval = igif->igif_conf.igifc_sqi;
      else
        querier_interval = igif->igif_conf.igifc_qi >> 2;
    }
  else
    querier_interval = igif->igif_conf.igifc_qi;

  /* Start the Querier Timer */
  IGMP_TIMER_ON (iglg, igif->t_igif_querier, igif,
                 igmp_if_timer_querier, querier_interval);

EXIT:

  IGMP_FN_EXIT (ret);
}

/* Other-Querier-Present Timer handler */
s_int32_t
igmp_if_timer_other_querier (struct thread *t_timer)
{
  struct lib_globals *iglg;
  struct igmp_if *hst_igif;
  struct igmp_if *igif;
  struct igmp_instance *igi;
  struct igmp_svc_reg *igsr;
  struct listnode *nn;
  s_int32_t ret;

  IGMP_FN_ENTER (Other Querier Timer);

  ret = IGMP_ERR_NONE;
  nn = NULL;

  /* Obtain the IGMP Lib Global pointer */
  iglg = THREAD_GLOB (t_timer);

  /* Sanity check */
  if (! iglg)
    {
      ret = IGMP_ERR_MALFORMED_ARG;
      goto EXIT;
    }

  /* Obtain the IGMP IF structure */
  igif = THREAD_ARG (t_timer);

  /* Sanity check */
  if (! igif)
    {
      ret = IGMP_ERR_MALFORMED_ARG;
      goto EXIT;
    }

  igi = igif->igif_owning_igi;
  igif->t_igif_other_querier = NULL;

  /* Time out the MRouter presence on this interface */
  if (CHECK_FLAG (igif->igif_sflags, IGMP_IF_SFLAG_SNOOPING))
    {
     /* Delete the mrouter interface from all group entries in forwarder */
     if (CHECK_FLAG (igif->igif_sflags, IGMP_IF_SFLAG_SNOOP_MROUTER_IF) && igi)
        LIST_LOOP (&igi->igi_svc_reg_lst, igsr, nn)
         if (igsr->igsr_cback_mrt_if_update)
           {
             igsr->igsr_cback_mrt_if_update (igsr->igsr_su_id,
                                             igif->igif_owning_ifp,
                                             igif->igif_su_id,
                                             PAL_TRUE);
           }
 
     UNSET_FLAG (igif->igif_sflags, IGMP_IF_SFLAG_SNOOP_MROUTER_IF);
   }


  if (CHECK_FLAG (igif->igif_sflags, IGMP_IF_SFLAG_SNOOPING)
      && ! (CHECK_FLAG (igif->igif_conf.igifc_cflags,
                        IGMP_IF_CFLAG_SNOOP_QUERIER)
            || CHECK_FLAG (igif->igif_conf.igifc_cflags,
                           IGMP_IF_CFLAG_CONFIG_ENABLED)
            || CHECK_FLAG (igif->igif_sflags,
                           IGMP_IF_SFLAG_MCAST_ENABLED)))
    {
      ret = IGMP_ERR_NONE;
      goto EXIT;
    }

  /* Become the Querier on this interface */
  SET_FLAG (igif->igif_sflags, IGMP_IF_SFLAG_QUERIER);

  /* Update the VLAN interface Querier Status */
  if (CHECK_FLAG (igif->igif_sflags, IGMP_IF_SFLAG_SNOOPING)
      && (hst_igif = igif->igif_hst_igif))
    SET_FLAG (hst_igif->igif_sflags, IGMP_IF_SFLAG_QUERIER);

  IGMP_DBG_INFO (igif->igif_owning_igi, EVENTS,
                 "Non-Querier->Querier on %s",
                 igif->igif_owning_ifp->name);

  (void_t) igmp_if_update_rtp_vars (igif);

  /* Encode and send General Query */
  ret = igmp_if_send_general_query (igif);

  if (ret < IGMP_ERR_NONE)
    IGMP_DBG_WARN (igif->igif_owning_igi, EVENTS,
                   "Sending General Query failed(%d)", ret);

  /* Start the Querier Timer */
  IGMP_TIMER_ON (iglg, igif->t_igif_querier, igif,
                 igmp_if_timer_querier,
                 igif->igif_conf.igifc_qi);

EXIT:

  IGMP_FN_EXIT (ret);
}

/* Recalculate run-time protocol variables */
s_int32_t
igmp_if_update_rtp_vars (struct igmp_if *igif)
{
  s_int32_t ret;

  IGMP_FN_ENTER (Update Timer);

  ret = IGMP_ERR_NONE;

  /* Sanity check */
  if (! igif)
    {
      ret = IGMP_ERR_MALFORMED_ARG;
      goto EXIT;
    }

  igif->igif_qi = igif->igif_conf.igifc_qi;

  igif->igif_sqc = igif->igif_conf.igifc_sqc;

  igif->igif_rv = igif->igif_conf.igifc_rv;

  igif->igif_gmi = igif->igif_rv *
                   igif->igif_qi +
                   igif->igif_conf.igifc_qri;

  if (CHECK_FLAG (igif->igif_conf.igifc_cflags,
                  IGMP_IF_CFLAG_QUERIER_TIMEOUT))
    igif->igif_oqi = igif->igif_conf.igifc_oqi;
  else
    igif->igif_oqi = igif->igif_rv *
                     igif->igif_qi +
                     (igif->igif_conf.igifc_qri / 2);

EXIT:

  IGMP_FN_EXIT (ret);
}

/* Update IGMP Access-List */
s_int32_t
igmp_if_access_list_update (struct igmp_if *igif)
{
  struct ptree_node *pn_next;
  struct igmp_group_rec *igr;
  struct access_list *al;
  struct ptree_node *pn;
  enum filter_type ft;
  struct prefix p;
  s_int32_t ret;

  IGMP_FN_ENTER (Access List Update);

  ret = IGMP_ERR_NONE;

  al = access_list_lookup
       (LIB_VRF_GET_VR (igif->igif_owning_igi->igi_owning_ivrf),
        AFI_IP, igif->igif_conf.igifc_access_list);

  if (! al)
    {
      ret = IGMP_ERR_NONE;
      goto EXIT;
    }

  for (pn = ptree_top (igif->igif_gmr_tib); pn; pn = pn_next)
    {
      pn_next = ptree_next (pn);

      if (! (igr = pn->info))
        continue;

      /* Apply the access list */
      p.family = AF_INET;
      p.prefixlen = IPV4_MAX_BITLEN;
      IPV4_ADDR_COPY (&p.u.prefix4, PTREE_NODE_KEY (pn));

      ft = access_list_apply (al, (void_t *)&p);
      switch (ft)
        {
        case FILTER_DENY:
        case FILTER_NO_MATCH:
          /* Deny the host from joining */
          IGMP_DBG_INFO (igif->igif_owning_igi, EVENTS,
                         "IGMP Grp %r on %s denied by alist %s",
                         PTREE_NODE_KEY (pn), igif->igif_owning_ifp->name,
                         al->name);

          ret = igmp_igr_fsm_process_event (igr, IGMP_FME_IMMEDIATE_LEAVE,
                                            NULL);

          if (ret < 0)
            IGMP_DBG_INFO (igif->igif_owning_igi, EVENTS,
                           "IGR FSM Failed(%d)! on %s for %r", ret,
                           igif->igif_owning_ifp->name,
                           PTREE_NODE_KEY (pn));
          break;

        case FILTER_PERMIT:
        case FILTER_DYNAMIC:
          break;
        }
    }

EXIT:

  IGMP_FN_EXIT (ret);
}

/* Update IGMP IF Local-membership Information to clients */
s_int32_t
igmp_if_send_lmem_update (struct igmp_if *igif)
{
  struct igmp_group_rec *igr;
  struct ptree_node *pn;
  s_int32_t ret;

  IGMP_FN_ENTER (IF Send Lmem Update);

  ret = IGMP_ERR_NONE;

  for (pn = ptree_top (igif->igif_gmr_tib); pn; pn = ptree_next (pn))
    {
      if (! (igr = pn->info))
        continue;

      ret = igmp_igr_fsm_process_event
            (igr, IGMP_FME_MANUAL_LMEM_UPDATE, NULL);

      if (ret < IGMP_ERR_NONE)
        IGMP_DBG_INFO (igif->igif_owning_igi, EVENTS,
                       "LMEM Update event failed(%d) for %s, continuing",
                       ret, igif->igif_owning_ifp->name);
    }

  IGMP_FN_EXIT (ret);
}

/* IGMP IF Hash Table Iterate Clear Function */
s_int32_t
igmp_if_avl_traverse_clear (void_t *avl_node_info,
                            void_t *arg1,
                            void_t *arg2)
{
  struct igmp_if *igif;
  s_int32_t ret = 0;

  IGMP_FN_ENTER (IF Hash Iterate Clear);

  igif = (struct igmp_if *) avl_node_info;

  if (igif)
    igmp_if_clear (igif, arg1, arg2, PAL_FALSE);

  IGMP_FN_EXIT (ret);
}

/* IGMP Host-side IF Hash Table Iterate to associate DS-IFs */
s_int32_t
igmp_if_hst_avl_traverse_associate (void_t *avl_node_info,
                                    void_t *arg1)
{
  struct igmp_if *hst_igif;
  struct igmp_if *ds_igif;
  s_int32_t ret = 0;

  IGMP_FN_ENTER (IF Host Hash Iterate Assoc);

  ds_igif = (struct igmp_if *) avl_node_info;
  hst_igif = arg1;

  if (ds_igif && hst_igif)
    {
      if (CHECK_FLAG (hst_igif->igif_conf.igifc_cflags,
                      IGMP_IF_CFLAG_PROXY_SERVICE))
        (void_t) igmp_if_pxy_service_association
                 (hst_igif, ds_igif, PAL_TRUE);

      if (CHECK_FLAG (hst_igif->igif_sflags,
                      IGMP_IF_SFLAG_SNOOPING)
          || ! CHECK_FLAG (hst_igif->igif_sflags,
                           IGMP_IF_SFLAG_SVC_TYPE_L3))
        (void_t) igmp_if_snp_service_association
                 (hst_igif, ds_igif, PAL_TRUE);
    }

  IGMP_FN_EXIT (ret);
}

/* IGMP Host-side IF Hash Table Iterate to associate DS-IFs */
s_int32_t
igmp_if_hst_avl_traverse_dissociate (void_t *avl_node_info,
                                     void_t *arg1)
{
  struct igmp_if *hst_igif;
  struct igmp_if *ds_igif;
  s_int32_t ret = 0;

  IGMP_FN_ENTER (IF Host Hash Iterate Dissoc);

  ds_igif = (struct igmp_if *) avl_node_info;
  hst_igif = arg1;

  if (ds_igif && hst_igif)
    {
      if (CHECK_FLAG (hst_igif->igif_conf.igifc_cflags,
                      IGMP_IF_CFLAG_PROXY_SERVICE))
        (void_t) igmp_if_pxy_service_association
                 (hst_igif, ds_igif, PAL_FALSE);

      if (CHECK_FLAG (hst_igif->igif_sflags,
                      IGMP_IF_SFLAG_SNOOPING)
          || ! CHECK_FLAG (hst_igif->igif_sflags,
                           IGMP_IF_SFLAG_SVC_TYPE_L3))
        (void_t) igmp_if_snp_service_association
                 (hst_igif, ds_igif, PAL_FALSE);
    }

  IGMP_FN_EXIT (ret);
}

/* IGMP Global Snooping Enable/Disable Hash-Table Iterator */
s_int32_t
igmp_if_snp_avl_traverse_update (void *avl_node_info,
                                 void_t *arg1)
{
  enum igmp_if_update_mode umode;
  struct igmp_if *igif;
  s_int32_t ret = 0;

  IGMP_FN_ENTER (IF Snoop Hash Iterate Update);

  igif = (struct igmp_if *) avl_node_info;
  if (igif == NULL)
    {
      ret = -1;
      goto EXIT;
    }

  umode = (enum igmp_if_update_mode) arg1;

  (void_t) igmp_if_update (igif, NULL, umode);

EXIT:

  IGMP_FN_EXIT (ret);
}

/* IGMP IF Snooping Status Hash-Table Iterator */
s_int32_t
igmp_if_snp_avl_traverse_match_status (void *avl_node_info,
                                       void_t *arg1,
                                       void_t *arg2)
{
  struct igmp_if *igif;
  bool_t *inp_enabled;
  bool_t *oup_status;
  s_int32_t ret = 0;

  IGMP_FN_ENTER (IF Snoop Hash Iterate Status);

  igif = (struct igmp_if *) avl_node_info;
  inp_enabled = (bool_t *) arg1;
  oup_status = (bool_t *) arg2;

  /* If mismatched, we've already acheived result */
  if (*inp_enabled != *oup_status)
    goto EXIT;

  if (*inp_enabled
      && ! CHECK_FLAG (igif->igif_sflags, IGMP_IF_SFLAG_SNOOPING))
    *oup_status = PAL_FALSE;
  else if (! *inp_enabled
           && CHECK_FLAG (igif->igif_sflags, IGMP_IF_SFLAG_SNOOPING))
    *oup_status = PAL_TRUE;

EXIT:

  IGMP_FN_EXIT (ret);
}

/* IGMP IF Proxy-Svc Associate/Dissociate with/from DS-IFs */
s_int32_t
igmp_if_pxy_service_association (struct igmp_if *hst_igif,
                                 struct igmp_if *ds_igif,
                                 bool_t associate)
{
  struct igmp_if_idx *mrtr_igifidx;
  s_int32_t ret;

  IGMP_FN_ENTER (IF Proxy Svc Association);

  ret = IGMP_ERR_NONE;
  mrtr_igifidx = NULL;

  if (! hst_igif
      || ! ds_igif)
    {
      ret = IGMP_ERR_MALFORMED_ARG;
      goto EXIT;
    }

  /* Do not Associate with Self */
  if (hst_igif == ds_igif
      || (associate
          && (! CHECK_FLAG (hst_igif->igif_conf.igifc_cflags,
                            IGMP_IF_CFLAG_PROXY_SERVICE)
              || ! CHECK_FLAG (ds_igif->igif_conf.igifc_cflags,
                               IGMP_IF_CFLAG_MROUTE_PROXY))))
    {
      ret = IGMP_ERR_NONE;
      goto EXIT;
    }

  /* String up DS-IFs if in Proxy-Service Mode */
  if (associate)
    {
      if (! LIST_ISEMPTY (&ds_igif->igif_conf.igifc_mrouter_if_lst))
        mrtr_igifidx = GETDATA (LISTHEAD
                                (&ds_igif->igif_conf.igifc_mrouter_if_lst));

      if (mrtr_igifidx
          && hst_igif->igif_idx.igifidx_ifp ==
             mrtr_igifidx->igifidx_ifp
          && hst_igif->igif_idx.igifidx_sid ==
             mrtr_igifidx->igifidx_sid
          && ! ds_igif->igif_hst_igif)
        {
          listnode_add (&hst_igif->igif_hst_dsif_lst, ds_igif);
          ds_igif->igif_hst_igif = hst_igif;
        }
    }
  else if (ds_igif->igif_hst_igif == hst_igif)
    {
      listnode_delete (&hst_igif->igif_hst_dsif_lst, ds_igif);
      ds_igif->igif_hst_igif = NULL;
    }

EXIT:

  IGMP_FN_EXIT (ret);
}

/* IGMP IF Snooping-Svc Associate/Dissociate with/from DS-IFs */
s_int32_t
igmp_if_snp_service_association (struct igmp_if *hst_igif,
                                 struct igmp_if *ds_igif,
                                 bool_t associate)
{
  struct igmp_if_idx *mrtr_igifidx;
  s_int32_t ret;

  IGMP_FN_ENTER (IF Snoop Svc Association);

  ret = IGMP_ERR_NONE;
  mrtr_igifidx = NULL;

  if (! hst_igif
      || ! ds_igif)
    {
      ret = IGMP_ERR_MALFORMED_ARG;
      goto EXIT;
    }

  /* Do not Associate with Self */
  if (ds_igif == hst_igif)
    {
      ret = IGMP_ERR_NONE;
      goto EXIT;
    }

  if (associate)
    {
      /* First, the L2 Physical IFs */
      if (CHECK_FLAG (hst_igif->igif_sflags, IGMP_IF_SFLAG_SVC_TYPE_L2)
          && ! CHECK_FLAG (hst_igif->igif_sflags, IGMP_IF_SFLAG_SVC_TYPE_L3))
        {
          /* Associate with VLAN-IF only if it has already started */
          if (! hst_igif->igif_hst_igif
              && CHECK_FLAG (ds_igif->igif_sflags, IGMP_IF_SFLAG_SNOOPING)
              && CHECK_FLAG (ds_igif->igif_sflags, IGMP_IF_SFLAG_SVC_TYPE_L2)
              && CHECK_FLAG (ds_igif->igif_sflags, IGMP_IF_SFLAG_SVC_TYPE_L3)
              && CHECK_FLAG (ds_igif->igif_sflags, IGMP_IF_SFLAG_ACTIVE)
              && ds_igif->igif_su_id == hst_igif->igif_su_id)
            {
              hst_igif->igif_hst_igif = ds_igif;
              listnode_add (&ds_igif->igif_hst_dsif_lst, hst_igif);

              /* Inherit the IF-Addr from VLAN-IF */
              hst_igif->igif_paddr = ds_igif->igif_paddr;
            }
        }
      /* Next, the L2 VLAN-IF (Dual Mode L2/L3) */
      else if (CHECK_FLAG (hst_igif->igif_sflags, IGMP_IF_SFLAG_SNOOPING)
               && CHECK_FLAG (hst_igif->igif_sflags, IGMP_IF_SFLAG_SVC_TYPE_L2)
               && CHECK_FLAG (hst_igif->igif_sflags, IGMP_IF_SFLAG_SVC_TYPE_L3))
        {
          /* Associate only with Inactive L2 Phy-IFs of same VLAN */
          if (! ds_igif->igif_hst_igif
              && CHECK_FLAG (ds_igif->igif_sflags, IGMP_IF_SFLAG_SVC_TYPE_L2)
              && ! CHECK_FLAG (ds_igif->igif_sflags, IGMP_IF_SFLAG_SVC_TYPE_L3)
              && CHECK_FLAG (ds_igif->igif_sflags, IGMP_IF_SFLAG_L2_MCAST_ENABLED)
              && ! CHECK_FLAG (ds_igif->igif_sflags, IGMP_IF_SFLAG_ACTIVE)
              && ds_igif->igif_su_id == hst_igif->igif_su_id)
            {
              ds_igif->igif_hst_igif = hst_igif;
              listnode_add (&hst_igif->igif_hst_dsif_lst, ds_igif);

              ds_igif->igif_paddr = hst_igif->igif_paddr;
            }
        }
    }
  else
    {
      if (ds_igif->igif_hst_igif == hst_igif)
        {
          listnode_delete (&hst_igif->igif_hst_dsif_lst, ds_igif);
          ds_igif->igif_hst_igif = NULL;
        }
      else if (hst_igif->igif_hst_igif == ds_igif)
        {
          listnode_delete (&ds_igif->igif_hst_dsif_lst, hst_igif);
          hst_igif->igif_hst_igif = NULL;
        }
    }

EXIT:

  IGMP_FN_EXIT (ret);
}

/* IGMP Snooping IF Start Decision Maker */
bool_t
igmp_if_snp_start (struct igmp_if *igif)
{
  struct igmp_instance *igi;
  bool_t bret;

  IGMP_FN_ENTER (IF Snoop Start);

  bret = PAL_FALSE;

  igi = igif->igif_owning_igi;

  if (CHECK_FLAG (igif->igif_sflags, IGMP_IF_SFLAG_SVC_TYPE_L2)
      && ! CHECK_FLAG (igif->igif_sflags, IGMP_IF_SFLAG_ACTIVE)
      && CHECK_FLAG (igif->igif_sflags, IGMP_IF_SFLAG_L2_MCAST_ENABLED)
      && ((CHECK_FLAG (igi->igi_cflags,
                       IGMP_INST_CFLAG_SNOOP_DISABLED)
           && ((CHECK_FLAG (igif->igif_conf.igifc_cflags,
                            IGMP_IF_CFLAG_SNOOP_ENABLED)
                || CHECK_FLAG (igif->igif_conf.igifc_cflags,
                               IGMP_IF_CFLAG_SNOOP_FAST_LEAVE)
                || CHECK_FLAG (igif->igif_conf.igifc_cflags,
                               IGMP_IF_CFLAG_SNOOP_MROUTER_IF)
                || CHECK_FLAG (igif->igif_conf.igifc_cflags,
                               IGMP_IF_CFLAG_SNOOP_QUERIER)
                || CHECK_FLAG (igif->igif_conf.igifc_cflags,
                               IGMP_IF_CFLAG_SNOOP_NO_REPORT_SUPPRESS))
               || ! CHECK_FLAG (igif->igif_sflags, IGMP_IF_SFLAG_SVC_TYPE_L3)))
          || (! CHECK_FLAG (igi->igi_cflags,
                            IGMP_INST_CFLAG_SNOOP_DISABLED)
              && ! CHECK_FLAG (igif->igif_conf.igifc_cflags,
                               IGMP_IF_CFLAG_SNOOP_DISABLED))
          || (CHECK_FLAG (igif->igif_conf.igifc_cflags,
                          ~ IGMP_IF_CFLAG_SNOOP_DISABLED)
              || CHECK_FLAG (igif->igif_sflags,
                             IGMP_IF_SFLAG_MCAST_ENABLED))))
    bret = PAL_TRUE;

  IGMP_FN_EXIT (bret);
}

/* IGMP Snooping IF Stop Decision Maker */
bool_t
igmp_if_snp_stop (struct igmp_if *igif)
{
  struct igmp_instance *igi;
  bool_t bret;

  IGMP_FN_ENTER (IF Snoop Stop);

  bret = PAL_FALSE;

  igi = igif->igif_owning_igi;

  if ((CHECK_FLAG (igif->igif_sflags, IGMP_IF_SFLAG_SVC_TYPE_L2)
       && ! CHECK_FLAG (igif->igif_conf.igifc_cflags,
                        IGMP_IF_CFLAG_SNOOP_ENABLED)
       && ! CHECK_FLAG (igi->igi_sflags,
                        IGMP_INST_SFLAG_SNOOP_ENABLED))
      || (CHECK_FLAG (igif->igif_sflags, IGMP_IF_SFLAG_ACTIVE)
          && CHECK_FLAG (igif->igif_sflags, IGMP_IF_SFLAG_SNOOPING)
          && ((CHECK_FLAG (igi->igi_cflags,
                           IGMP_INST_CFLAG_SNOOP_DISABLED)
               && ! CHECK_FLAG (igif->igif_conf.igifc_cflags,
                                IGMP_IF_CFLAG_SNOOP_ENABLED)
               && ! CHECK_FLAG (igif->igif_conf.igifc_cflags,
                                IGMP_IF_CFLAG_SNOOP_FAST_LEAVE)
               && ! CHECK_FLAG (igif->igif_conf.igifc_cflags,
                                IGMP_IF_CFLAG_SNOOP_MROUTER_IF)
               && ! CHECK_FLAG (igif->igif_conf.igifc_cflags,
                                IGMP_IF_CFLAG_SNOOP_QUERIER)
               && ! CHECK_FLAG (igif->igif_conf.igifc_cflags,
                                IGMP_IF_CFLAG_SNOOP_NO_REPORT_SUPPRESS))
              || (! CHECK_FLAG (igi->igi_cflags,
                                IGMP_INST_CFLAG_SNOOP_DISABLED)
                  && CHECK_FLAG (igif->igif_conf.igifc_cflags,
                                 IGMP_IF_CFLAG_SNOOP_DISABLED)))
          && ! (CHECK_FLAG (igif->igif_conf.igifc_cflags,
                             ~ IGMP_IF_CFLAG_SNOOP_DISABLED)
                || CHECK_FLAG (igif->igif_sflags,
                               IGMP_IF_SFLAG_MCAST_ENABLED))))
    bret = PAL_TRUE;

  IGMP_FN_EXIT (bret);
}

/* IGMP Snooping IF inheritance of IF Configuration by VLAN Constituent IFs */
s_int32_t
igmp_if_snp_inherit_config (struct igmp_if *hst_igif)
{
  struct igmp_if *ds_igif;
  struct listnode *nn;
  s_int32_t ret;

  IGMP_FN_ENTER (Snp Inherit IF Config)

  ret = IGMP_ERR_NONE;

  if (! hst_igif)
    {
      ret = IGMP_ERR_MALFORMED_ARG;
      goto EXIT;
    }

  LIST_LOOP (&hst_igif->igif_hst_dsif_lst, ds_igif, nn)
    {
      ds_igif->igif_conf = hst_igif->igif_conf;
      ds_igif->igif_gmi  = hst_igif->igif_gmi;
      ds_igif->igif_qi   = hst_igif->igif_qi;
      ds_igif->igif_sqc   = hst_igif->igif_sqc;
      ds_igif->igif_oqi  = hst_igif->igif_oqi;
      ds_igif->igif_rv   = hst_igif->igif_rv;
    }

EXIT:

  IGMP_FN_EXIT (ret);
}

/* IGMP IF Host-side V1 Querier Present Timer Handler */
s_int32_t
igmp_if_hst_timer_v1_querier_present (struct thread *t_timer)
{
  struct lib_globals *iglg;
  struct igmp_if *hst_igif;
  s_int32_t ret;

  IGMP_FN_ENTER (V1 Querier Present Timer);

  ret = IGMP_ERR_NONE;

  /* Obtain the IGMP Lib Global pointer */
  iglg = THREAD_GLOB (t_timer);

  /* Sanity check */
  if (! iglg)
    {
      ret = IGMP_ERR_MALFORMED_ARG;
      goto EXIT;
    }

  /* Obtain the IGMP IF structure */
  hst_igif = THREAD_ARG (t_timer);

  /* Sanity check */
  if (! hst_igif)
    {
      ret = IGMP_ERR_MALFORMED_ARG;
      goto EXIT;
    }

  hst_igif->t_igif_hst_v1_querier_present = NULL;

  IGMP_DBG_INFO (hst_igif->igif_owning_igi, EVENTS,
                 "Exipry on %s",
                 hst_igif->igif_owning_ifp->name);

  ret = igmp_if_hst_update_host_cmode (hst_igif);

EXIT:

  IGMP_FN_EXIT (ret);
}

/* IGMP IF Host-side V2 Querier Present Timer Handler */
s_int32_t
igmp_if_hst_timer_v2_querier_present (struct thread *t_timer)
{
  struct lib_globals *iglg;
  struct igmp_if *hst_igif;
  s_int32_t ret;

  IGMP_FN_ENTER (V2 Querier Present Timer);

  ret = IGMP_ERR_NONE;

  /* Obtain the IGMP Lib Global pointer */
  iglg = THREAD_GLOB (t_timer);

  /* Sanity check */
  if (! iglg)
    {
      ret = IGMP_ERR_MALFORMED_ARG;
      goto EXIT;
    }

  /* Obtain the IGMP IF structure */
  hst_igif = THREAD_ARG (t_timer);

  /* Sanity check */
  if (! hst_igif)
    {
      ret = IGMP_ERR_MALFORMED_ARG;
      goto EXIT;
    }

  hst_igif->t_igif_hst_v2_querier_present = NULL;

  IGMP_DBG_INFO (hst_igif->igif_owning_igi, EVENTS,
                 "Exipry on %s",
                 hst_igif->igif_owning_ifp->name);

  ret = igmp_if_hst_update_host_cmode (hst_igif);

EXIT:

  IGMP_FN_EXIT (ret);
}

/* IGMP IF Host-side Generate General-Report Timer Handler */
s_int32_t
igmp_if_hst_timer_general_report (struct thread *t_timer)
{
  struct lib_globals *iglg;
  struct igmp_if *hst_igif;
  s_int32_t ret;

  IGMP_FN_ENTER (General Report Timer);

  ret = IGMP_ERR_NONE;

  /* Obtain the IGMP Lib Global pointer */
  iglg = THREAD_GLOB (t_timer);

  /* Sanity check */
  if (! iglg)
    {
      ret = IGMP_ERR_MALFORMED_ARG;
      goto EXIT;
    }

  /* Obtain the IGMP IF structure */
  hst_igif = THREAD_ARG (t_timer);

  /* Sanity check */
  if (! hst_igif)
    {
      ret = IGMP_ERR_MALFORMED_ARG;
      goto EXIT;
    }

  hst_igif->t_igif_hst_send_general_report = NULL;

  IGMP_DBG_INFO (hst_igif->igif_owning_igi, EVENTS, "Exipry on %s",
                 hst_igif->igif_owning_ifp->name);

  ret = igmp_if_hst_send_general_report (hst_igif);

EXIT:

  IGMP_FN_EXIT (ret);
}

/* IGMP IF Host-side Generate Grp Report Timer Handler */
s_int32_t
igmp_if_hst_timer_group_report (struct thread *t_timer)
{
  struct igmp_group_rec *hst_igr;
  struct lib_globals *iglg;
  struct igmp_if *hst_igif;
  s_int32_t ret;

  IGMP_FN_ENTER (Grp Report Timer);

  ret = IGMP_ERR_NONE;

  /* Obtain the IGMP Lib Global pointer */
  iglg = THREAD_GLOB (t_timer);

  /* Sanity check */
  if (! iglg)
    {
      ret = IGMP_ERR_MALFORMED_ARG;
      goto EXIT;
    }

  /* Obtain the IGMP Group structure */
  hst_igr = THREAD_ARG (t_timer);

  /* Sanity check */
  if (! hst_igr
      || ! (hst_igif = hst_igr->igr_owning_igif))
    {
      ret = IGMP_ERR_MALFORMED_ARG;
      goto EXIT;
    }

  hst_igr->t_igr_rexmit_group = NULL;

  IGMP_DBG_INFO (hst_igif->igif_owning_igi, EVENTS,
                 "Exipry for %r on %s",
                 PTREE_NODE_KEY (hst_igr->igr_owning_pn),
                 hst_igif->igif_owning_ifp->name);

  ret = igmp_if_hst_send_group_report (hst_igr);

EXIT:

  IGMP_FN_EXIT (ret);
}

/* IGMP IF Host-side Generate Grp-Src Report Timer Handler */
s_int32_t
igmp_if_hst_timer_group_source_report (struct thread *t_timer)
{
  struct igmp_group_rec *hst_igr;
  struct lib_globals *iglg;
  struct igmp_if *hst_igif;
  s_int32_t ret;

  IGMP_FN_ENTER (Grp-Src Report Timer);

  ret = IGMP_ERR_NONE;

  /* Obtain the IGMP Lib Global pointer */
  iglg = THREAD_GLOB (t_timer);

  /* Sanity check */
  if (! iglg)
    {
      ret = IGMP_ERR_MALFORMED_ARG;
      goto EXIT;
    }

  /* Obtain the IGMP Group structure */
  hst_igr = THREAD_ARG (t_timer);

  /* Sanity check */
  if (! hst_igr
      || ! (hst_igif = hst_igr->igr_owning_igif))
    {
      ret = IGMP_ERR_MALFORMED_ARG;
      goto EXIT;
    }

  hst_igr->t_igr_rexmit_group_source = NULL;

  IGMP_DBG_INFO (hst_igif->igif_owning_igi, EVENTS,
                 "Exipry for %r on %s",
                 PTREE_NODE_KEY (hst_igr->igr_owning_pn),
                 hst_igif->igif_owning_ifp->name);

  ret = igmp_if_hst_send_group_source_report
        (hst_igr, hst_igr->igr_rexmit_srcs_tib);

EXIT:

  IGMP_FN_EXIT (ret);
}

/* IGMP IF Host-side Group Report Retransmission Timer Handler */
s_int32_t
igmp_if_hst_grec_timer_rexmit_group (struct thread *t_timer)
{
  struct igmp_group_rec *hst_igr;
  struct lib_globals *iglg;
  s_int32_t ret;

  IGMP_FN_ENTER (Grp Report Rexmit);

  ret = IGMP_ERR_NONE;

  /* Obtain the IGMP Lib Global pointer */
  iglg = THREAD_GLOB (t_timer);

  /* Sanity check */
  if (! iglg)
    {
      ret = IGMP_ERR_MALFORMED_ARG;
      goto EXIT;
    }

  /* Obtain the IGMP Group-Rec structure */
  hst_igr = THREAD_ARG (t_timer);

  /* Sanity check */
  if (! hst_igr)
    {
      ret = IGMP_ERR_MALFORMED_ARG;
      goto EXIT;
    }

  hst_igr->t_igr_rexmit_group = NULL;

  IGMP_DBG_INFO (hst_igr->igr_owning_igif->igif_owning_igi, EVENTS,
                 "Exipry for Grp %r on %s",
                 PTREE_NODE_KEY (hst_igr->igr_owning_pn),
                 hst_igr->igr_owning_igif->igif_owning_ifp->name);

  if (--hst_igr->igr_rexmit_group_lmqc)
    {

      if (hst_igr->igr_filt_mode_state == IGMP_FMS_INCLUDE)
        hst_igr->igr_rexmit_hrt = IGMP_HRT_CHG_TO_INCL;
      else
        hst_igr->igr_rexmit_hrt = IGMP_HRT_CHG_TO_EXCL;

      ret = igmp_if_hst_send_group_report (hst_igr);

      if (ret < IGMP_ERR_NONE)
        IGMP_DBG_WARN (hst_igr->igr_owning_igif->igif_owning_igi, EVENTS,
                       "Group Report Rexmit failed(%d)", ret);

      /*While re-transmission of leave report, 
        group record has to be deleted when lmqc becomes 1 */ 
      if (! (hst_igr->igr_rexmit_group_lmqc - 1) 
           && hst_igr->igr_filt_mode_state == IGMP_FMS_INCLUDE
           && ! hst_igr->igr_src_a_tib_count)
        igmp_if_grec_delete (hst_igr);
    }

EXIT:

  IGMP_FN_EXIT (ret);
}

/* IGMP IF Host-side Group-Source Report Retransmission Timer Handler */
s_int32_t
igmp_if_hst_grec_timer_rexmit_group_source (struct thread *t_timer)
{
  struct igmp_group_rec *hst_igr;
  struct lib_globals *iglg;
  s_int32_t ret;

  IGMP_FN_ENTER (Grp-Src Report Rexmit);

  ret = IGMP_ERR_NONE;

  /* Obtain the IGMP Lib Global pointer */
  iglg = THREAD_GLOB (t_timer);

  /* Sanity check */
  if (! iglg)
    {
      ret = IGMP_ERR_MALFORMED_ARG;
      goto EXIT;
    }

  /* Obtain the IGMP IF structure */
  hst_igr = THREAD_ARG (t_timer);

  /* Sanity check */
  if (! hst_igr)
    {
      ret = IGMP_ERR_MALFORMED_ARG;
      goto EXIT;
    }

  hst_igr->t_igr_rexmit_group_source = NULL;

  IGMP_DBG_INFO (hst_igr->igr_owning_igif->igif_owning_igi, EVENTS,
                 "Exipry for Grp %r on %s",
                 PTREE_NODE_KEY (hst_igr->igr_owning_pn),
                 hst_igr->igr_owning_igif->igif_owning_ifp->name);

  if (--hst_igr->igr_rexmit_group_source_lmqc)
    {
      ret = igmp_if_hst_send_group_source_report
            (hst_igr, hst_igr->igr_rexmit_srcs_tib);

      if (ret < IGMP_ERR_NONE)
        IGMP_DBG_WARN (hst_igr->igr_owning_igif->igif_owning_igi, EVENTS,
                       "Group-Source Report Rexmit failed(%d)", ret);

      hst_igr->igr_rexmit_hrt = IGMP_HRT_ALLOW_NEW_SRCS;

      ret = igmp_if_hst_send_group_source_report
            (hst_igr, hst_igr->igr_rexmit_allow_tib);

      if (ret < IGMP_ERR_NONE)
        IGMP_DBG_WARN (hst_igr->igr_owning_igif->igif_owning_igi, EVENTS,
                       "Group-Source Report Rexmit failed(%d)", ret);

      hst_igr->igr_rexmit_hrt = IGMP_HRT_BLOCK_OLD_SRCS;

      ret = igmp_if_hst_send_group_source_report
            (hst_igr, hst_igr->igr_rexmit_block_tib);

      if (ret < IGMP_ERR_NONE)
        IGMP_DBG_WARN (hst_igr->igr_owning_igif->igif_owning_igi, EVENTS,
                       "Group-Source Report Rexmit failed(%d)", ret);
    }
  /* Discard the Re-Transmission TIB */
  else if (hst_igr->igr_rexmit_srcs_tib_count
           ||hst_igr->igr_rexmit_srcs_allow_tib_count
           || hst_igr->igr_rexmit_srcs_block_tib_count)
    (void_t) igmp_if_grec_rexmit_tib_delete (hst_igr);

EXIT:

  IGMP_FN_EXIT (ret);
}

/* IGMP IF Host-side Update Host Compatibility-Mode */
s_int32_t
igmp_if_hst_update_host_cmode (struct igmp_if *hst_igif)
{
  s_int32_t ret;

  IGMP_FN_ENTER (Proxy Update Host C-Mode);

  ret = IGMP_ERR_NONE;

  switch (hst_igif->igif_conf.igifc_version)
    {
    case IGMP_VERSION_1:
      UNSET_FLAG (hst_igif->igif_sflags,
                  (IGMP_IF_SFLAG_HOST_COMPAT_V2
                   | IGMP_IF_SFLAG_HOST_COMPAT_V3));
      SET_FLAG (hst_igif->igif_sflags, IGMP_IF_SFLAG_HOST_COMPAT_V1);
      break;

    case IGMP_VERSION_2:
      UNSET_FLAG (hst_igif->igif_sflags,
                  (IGMP_IF_SFLAG_HOST_COMPAT_V1
                   | IGMP_IF_SFLAG_HOST_COMPAT_V2
                   | IGMP_IF_SFLAG_HOST_COMPAT_V3));

      if (hst_igif->t_igif_hst_v1_querier_present)
        SET_FLAG (hst_igif->igif_sflags, IGMP_IF_SFLAG_HOST_COMPAT_V1);
      else
        SET_FLAG (hst_igif->igif_sflags, IGMP_IF_SFLAG_HOST_COMPAT_V2);
      break;

    case IGMP_VERSION_3:
      UNSET_FLAG (hst_igif->igif_sflags,
                  (IGMP_IF_SFLAG_HOST_COMPAT_V1
                   | IGMP_IF_SFLAG_HOST_COMPAT_V2
                   | IGMP_IF_SFLAG_HOST_COMPAT_V3));

      if (hst_igif->t_igif_hst_v1_querier_present)
        SET_FLAG (hst_igif->igif_sflags, IGMP_IF_SFLAG_HOST_COMPAT_V1);
      else if (hst_igif->t_igif_hst_v2_querier_present)
        SET_FLAG (hst_igif->igif_sflags, IGMP_IF_SFLAG_HOST_COMPAT_V2);
      else
        SET_FLAG (hst_igif->igif_sflags, IGMP_IF_SFLAG_HOST_COMPAT_V3);
      break;

    default:
      ret = IGMP_ERR_DOOM;
      goto EXIT;
    }

EXIT:

  IGMP_FN_EXIT (ret);
}

/* Consolidate IGMP L-mem Information on Host-side Interface */
s_int32_t
igmp_if_hst_consolidate_mship (struct igmp_group_rec *igr,
                               struct igmp_source_list *isl)
{
  struct igmp_group_rec *hst_igr;
  struct igmp_instance *igi;
  struct igmp_if *hst_igif;
  struct igmp_if *igif;
  s_int32_t ret;

  IGMP_FN_ENTER (IF Consol MShip);

  ret = IGMP_ERR_NONE;

  if (! igr
      || ! (igif = igr->igr_owning_igif)
      || ! (igi = igif->igif_owning_igi))
    {
      ret = IGMP_ERR_MALFORMED_ARG;
      goto EXIT;
    }

  if ((! CHECK_FLAG (igif->igif_conf.igifc_cflags,
                     IGMP_IF_CFLAG_MROUTE_PROXY)
       && ! CHECK_FLAG (igif->igif_sflags,
                        IGMP_IF_SFLAG_SNOOPING))
      || ! (hst_igif = igif->igif_hst_igif))
    {
      IGMP_DBG_WARN (igi, EVENTS, "Host-side interface for %s NOT FOUND!",
                     igif->igif_owning_ifp->name);
    
      ret = IGMP_ERR_NONE;
      goto EXIT;
    }

  if (! CHECK_FLAG (hst_igif->igif_conf.igifc_cflags,
                    IGMP_IF_CFLAG_PROXY_SERVICE)
      && ! CHECK_FLAG (hst_igif->igif_sflags,
                       IGMP_IF_SFLAG_SNOOPING))
    {
      IGMP_DBG_WARN (igi, EVENTS, "Proxy-Svc/Snooping not config'd on"
                     " Host-side IF %s!",
                     hst_igif->igif_owning_ifp->name);

      ret = IGMP_ERR_NONE;
      goto EXIT;
    }

  /* Obtain the Group Rec */
  hst_igr = igmp_if_hst_grec_get (hst_igif,
                                  (struct pal_in4_addr *)
                                  PTREE_NODE_KEY
                                  (igr->igr_owning_pn));

  if (! hst_igr)
    {
      IGMP_DBG_INFO (igi, DECODE, "Failed to 'get' Group-Rec!");

      ret = IGMP_ERR_OOM;
      goto EXIT;
    }

  /* Update the last reporter field of the group record */  
  hst_igr->igr_last_reporter = igr->igr_last_reporter;

  /* Process Proxy-Service Host-side FSM event */
  ret = igmp_hst_igr_fsm_process_event
        (hst_igr,
         (enum igmp_hst_filter_mode_event)
         igr->igr_filt_mode_state, isl);

  /* 'hst_igr' not necessarily valid from here onwards */
  if (ret < IGMP_ERR_NONE)
    IGMP_DBG_INFO (igi, DECODE, "%s Host-side FSM Failed(%d)! for %s %r",
                   hst_igif->igif_owning_ifp->name, ret,
                   igif->igif_owning_ifp->name,
                   PTREE_NODE_KEY (igr->igr_owning_pn));

EXIT:

  IGMP_FN_EXIT (ret);
}

/* IGMP IF Host-side Schedule IGMP-Report Transmission */
s_int32_t
igmp_if_hst_sched_report (struct igmp_if *igif,
                          struct pal_in4_addr *pgrp,
                          struct pal_in4_addr *msrc,
                          u_int32_t rsp_time)
{
  struct igmp_group_rec *hst_igr;
  struct igmp_source_rec *isr;
  struct igmp_instance *igi;
  struct igmp_if *hst_igif;
  s_int32_t ret;

  IGMP_FN_ENTER (Host Sched Report);

  ret = IGMP_ERR_NONE;
  hst_igif = NULL;
  hst_igr = NULL;
  isr = NULL;

  /* Sanity Checks */
  if (! igif
      || ! (igi = igif->igif_owning_igi)
      || (msrc
          && (! pgrp
              || IPV4_ADDR_SAME (pgrp, &igi->igi_in4any_addr))))
    {
      ret = IGMP_ERR_MALFORMED_ARG;
      goto EXIT;
    }

  /* Proceed only of IF in Proxy-Service/Snooping Mode */
  if (! CHECK_FLAG (igif->igif_conf.igifc_cflags,
                    IGMP_IF_CFLAG_PROXY_SERVICE)
      && ! CHECK_FLAG (igif->igif_sflags, IGMP_IF_SFLAG_SNOOPING))
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

  /* No action if Gen-Query Reponse is pending sooner */
  if (igif->t_igif_hst_send_general_report
      && rsp_time >= thread_timer_remain_second
                     (igif->t_igif_hst_send_general_report))
    {
      ret = IGMP_ERR_NONE;
      goto EXIT;
    }

  /* Find the Host-side Grp-Rec &/ Src-Rec */
  if (pgrp
      && ! IPV4_ADDR_SAME (pgrp, &igi->igi_in4any_addr))
    {
      hst_igr = igmp_if_hst_grec_lookup (hst_igif, pgrp);

      if (! hst_igr)
        {
          ret = IGMP_ERR_NONE;
          goto EXIT;
        }

      if (msrc)
        {
          isr = igmp_if_srec_lookup (hst_igr, PAL_TRUE, msrc);

          if (! isr)
            isr = igmp_if_srec_lookup (hst_igr, PAL_FALSE, msrc);

          if (! isr)
            {
              ret = IGMP_ERR_NONE;
              goto EXIT;
            }
        }
    }

  /* Schedule Source-Specific Query Reponse */
  if (isr)
    {
      SET_FLAG (isr->isr_sflags, IGMP_ISR_SFLAG_REPORT_PENDING);

      if (! hst_igr->t_igr_rexmit_group_source
          || rsp_time < thread_timer_remain_second
                        (hst_igr->t_igr_rexmit_group_source))
        {
          /* Clear Rexmit-HRT/Srcs-TIB so its interpret CSR-Sources */
          hst_igr->igr_rexmit_hrt = IGMP_HRT_INVALID;

          if (hst_igr->igr_rexmit_srcs_tib)
            {
              ptree_finish (hst_igr->igr_rexmit_srcs_tib);
              hst_igr->igr_rexmit_srcs_tib = NULL;
            }

          IGMP_TIMER_OFF (IGMP_LG (hst_igif->igif_owning_igi),
                          hst_igr->t_igr_rexmit_group_source);
          IGMP_TIMER_ON (IGMP_LG (hst_igif->igif_owning_igi),
                         hst_igr->t_igr_rexmit_group_source,
                         hst_igr,
                         igmp_if_hst_timer_group_source_report,
                         rsp_time);
        }
    }
  /* Schedule Group-Specific Query Reponse */
  else if (hst_igr)
    {
      if ( hst_igr->igr_filt_mode_state == IGMP_FMS_INCLUDE &&
            ! hst_igr->igr_src_a_tib_count)
         goto EXIT;

      SET_FLAG (hst_igr->igr_sflags, IGMP_IGR_SFLAG_REPORT_PENDING);

      if (! hst_igr->t_igr_rexmit_group
          || rsp_time < thread_timer_remain_second
                        (hst_igr->t_igr_rexmit_group))
        {
          /* Clear Rexmit-HRT/Srcs-TIB so its interpret CSR-Group */
          hst_igr->igr_rexmit_hrt = IGMP_HRT_INVALID;

          if (hst_igr->igr_rexmit_srcs_tib)
            {
              ptree_finish (hst_igr->igr_rexmit_srcs_tib);
              hst_igr->igr_rexmit_srcs_tib = NULL;
            }

          IGMP_TIMER_OFF (IGMP_LG (hst_igif->igif_owning_igi),
                          hst_igr->t_igr_rexmit_group);
          IGMP_TIMER_ON (IGMP_LG (hst_igif->igif_owning_igi),
                         hst_igr->t_igr_rexmit_group, hst_igr,
                         igmp_if_hst_timer_group_report,
                         rsp_time);
        }
    }
  /* Schedule New Gen-Query Reponse (CSR-General) */
  else if (hst_igif->igif_num_grecs)
    {
      IGMP_TIMER_OFF (IGMP_LG (igif->igif_owning_igi),
                      igif->t_igif_hst_send_general_report);
      IGMP_TIMER_ON (IGMP_LG (igif->igif_owning_igi),
                     igif->t_igif_hst_send_general_report,
                     igif, igmp_if_hst_timer_general_report,
                     rsp_time);
    }

EXIT:

  IGMP_FN_EXIT (ret);
}

/* IGMP IF Proxy-Service Process MCast Forwarder Cache Query */
s_int32_t
igmp_if_hst_process_mfc_query (struct igmp_if *iigif,
                               u_int32_t mfc_msg_type,
                               struct pal_in4_addr *pgrp,
                               struct pal_in4_addr *msrc)
{
  struct igmp_source_list hst_isl;
  struct igmp_group_rec *hst_igr;
  struct igmp_instance *igi;
  struct igmp_if *hst_igif;
  s_int32_t ret;

  IGMP_FN_ENTER (IF Pxy MFC Msg Proc);

  ret = IGMP_ERR_NONE;

  /* Sanity checks */
  if (! iigif
      || ! pgrp
      || ! msrc
      || ! (igi = iigif->igif_owning_igi))
    {
      ret = IGMP_ERR_MALFORMED_ARG;
      goto EXIT;
    }

  /* Validate Msg Type */
  switch (mfc_msg_type)
    {
      case IGMP_MFC_MSG_NOCACHE:
      case IGMP_MFC_MSG_WRONGVIF:
        break;

    default:
      ret = IGMP_ERR_UNKNOWN_MSG;
      goto EXIT;
    }

  /* Validate that Incoming-IF is part of Proxy-Config */
  if (! CHECK_FLAG (iigif->igif_conf.igifc_cflags,
                    IGMP_IF_CFLAG_PROXY_SERVICE)
      && ! (CHECK_FLAG (iigif->igif_conf.igifc_cflags,
                        IGMP_IF_CFLAG_MROUTE_PROXY)
            && iigif->igif_hst_igif))
    {
      ret = IGMP_ERR_NO_VALID_CONFIG;
      goto EXIT;
    }

  /* Obtain Host-side Interface */
  hst_igif = iigif;
  if (CHECK_FLAG (iigif->igif_conf.igifc_cflags,
                  IGMP_IF_CFLAG_MROUTE_PROXY))
    hst_igif = iigif->igif_hst_igif;

  if (! hst_igif)
    {
      ret = IGMP_ERR_NO_VALID_CONFIG;
      goto EXIT;
    }

  /* If Host G-Rec does not exist, we're saved */
  hst_igr = igmp_if_hst_grec_lookup (hst_igif, pgrp);

  if (! hst_igr)
    {
      ret = IGMP_ERR_NONE;
      goto EXIT;
    }

  /* Set transient flag to mark the Incoming IF */
  SET_FLAG (iigif->igif_sflags, IGMP_IF_SFLAG_MFC_MSG_IN_IF);

  /* Prepare IGMP Sources-List */
  pal_mem_set (&hst_isl, 0, sizeof (struct igmp_source_list));
  hst_isl.isl_num += 1;
  IPV4_ADDR_COPY (&hst_isl.isl_saddr [0], msrc);

  /* Process Proxy-Service Host-side FSM event */
  ret = igmp_hst_igr_fsm_process_event
        (hst_igr, IGMP_HFME_MFC_MSG, &hst_isl);

EXIT:

  IGMP_FN_EXIT (ret);
}

/* IGMP IF Join Group Generate General-Report Timer Handler */
s_int32_t
igmp_if_igr_timer_general_report (struct thread *t_timer)
{
  struct lib_globals *iglg;
  struct igmp_if *jg_igif;
  s_int32_t ret;

  IGMP_FN_ENTER (Join Group General Report Timer);

  ret = IGMP_ERR_NONE;

  /* Obtain the IGMP Lib Global pointer */
  iglg = THREAD_GLOB (t_timer);

  /* Sanity check */
  if (! iglg)
    {
      ret = IGMP_ERR_MALFORMED_ARG;
      goto EXIT;
    }

  /* Obtain the IGMP IF structure */
  jg_igif = THREAD_ARG (t_timer);

  /* Sanity check */
  if (! jg_igif)
    {
      ret = IGMP_ERR_MALFORMED_ARG;
      goto EXIT;
    }
  
  jg_igif->t_igif_jg_send_general_report = NULL;

  IGMP_DBG_INFO (jg_igif->igif_owning_igi, EVENTS, "Exipry on %s",
                 jg_igif->igif_owning_ifp->name);

  ret = igmp_if_jg_send_general_report (jg_igif);

EXIT:

  IGMP_FN_EXIT (ret);
}

/* IGMP IF Join Group Generate Grp Report Timer Handler */
s_int32_t
igmp_if_jg_timer_group_report (struct thread *t_timer)
{
  struct igmp_group_rec *jg_igr;
  struct lib_globals *iglg;
  struct igmp_if *jg_igif;
  s_int32_t ret;

  IGMP_FN_ENTER (Join Grp Report Timer);

  ret = IGMP_ERR_NONE;

  /* Obtain the IGMP Lib Global pointer */
  iglg = THREAD_GLOB (t_timer);

  /* Sanity check */
  if (! iglg)
    {
      ret = IGMP_ERR_MALFORMED_ARG;
      goto EXIT;
    }

  /* Obtain the IGMP Group structure */
  jg_igr = THREAD_ARG (t_timer);

  /* Sanity check */
  if (! jg_igr
      || ! (jg_igif = jg_igr->igr_owning_igif))
    {
      ret = IGMP_ERR_MALFORMED_ARG;
      goto EXIT;
    }

  jg_igr->t_igr_join_group = NULL;

  IGMP_DBG_INFO (jg_igif->igif_owning_igi, EVENTS,
                 "Exipry for %r on %s",
                 PTREE_NODE_KEY (jg_igr->igr_owning_pn),
                 jg_igif->igif_owning_ifp->name);

  ret = igmp_if_jg_send_group_report (jg_igr);

EXIT:

  IGMP_FN_EXIT (ret);
}

/* IGMP IF Router-side, Join Group response specific group query*/
s_int32_t
igmp_if_igr_group_report (struct igmp_if *igif,
                          struct igmp_group_rec *igr,
                          u_int32_t rsp_time)
{
  s_int32_t ret;

  IGMP_FN_ENTER (Join Group Sched Report);

  ret = IGMP_ERR_NONE;

    /* Sanity Checks */
  if (! igif
      || ! igif->igif_owning_igi)
    {
      ret = IGMP_ERR_MALFORMED_ARG;
      goto EXIT;
    }

    /* No action if Gen-Query Reponse is pending sooner */
  if (igif->t_igif_jg_send_general_report
      && rsp_time >= thread_timer_remain_second
                     (igif->t_igif_jg_send_general_report))
    {/*!!*/
      ret = IGMP_ERR_NONE;
      goto EXIT;
    }
  
    /* Schedule Group-Specific Query Reponse */
  if (igr)
    {
      /*igr->igr_filt_mode_state == IGMP_FMS_EXCLUDE -- join group*/
      SET_FLAG (igr->igr_sflags, IGMP_IGR_SFLAG_REPORT_PENDING);

      if (! igr->t_igr_join_group
          || rsp_time < thread_timer_remain_second
                        (igr->t_igr_join_group))
        {
          IGMP_TIMER_OFF (IGMP_LG (igif->igif_owning_igi),
                          igr->t_igr_join_group);
          IGMP_TIMER_ON (IGMP_LG (igif->igif_owning_igi),
                         igr->t_igr_join_group, igr,
                         igmp_if_jg_timer_group_report,
                         rsp_time);
        }
    }
  else if (igif->igif_num_grecs)
    {/* Schedule New Gen-Query Reponse */
      IGMP_TIMER_OFF (IGMP_LG (igif->igif_owning_igi),
                      igif->t_igif_jg_send_general_report);
      IGMP_TIMER_ON (IGMP_LG (igif->igif_owning_igi),
                     igif->t_igif_jg_send_general_report,
                     igif, igmp_if_igr_timer_general_report,
                     rsp_time);
    }

EXIT:

  IGMP_FN_EXIT (ret);
}

/* SSM map processing */
s_int32_t
igmp_if_ssm_map_process (struct igmp_if *igif,
                         struct pal_in4_addr *pgrp,
                         struct igmp_source_list **isl)
{
  struct igmp_ssm_map_static *igssms;
  struct igmp_instance *igi;
  u_int32_t src_list_size;
  struct access_list *al;
  struct listnode *nn;
  enum filter_type ft;
  struct prefix p;
  s_int32_t ret;

  IGMP_FN_ENTER (IF SSM Process);

  igi = igif->igif_owning_igi;
  ret = IGMP_ERR_NONE;

  /* SSM-Map processing */
  if (! CHECK_FLAG (igi->igi_cflags, IGMP_INST_CFLAG_SSM_MAP_DISABLED)
      && (pal_ntoh32 (pgrp->s_addr) & IN_CLASSA_NET) ==
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
            IPV4_ADDR_COPY (&p.u.prefix4, pgrp);

            ft = access_list_apply (al, (void_t *) &p);

            switch (ft)
              {
              case FILTER_PERMIT:
                IGMP_DBG_ERR (igi, EVENTS, "Grp %r on %s matched"
                              " SSM-Map alist %s, MSrc %r",
                              pgrp, igif->igif_owning_ifp->name,
                              igssms->igssms_grp_alist,
                              &igssms->igssms_msrc);
                /* Create the Source-List */
                if (! *isl)
                  {
                    src_list_size = sizeof (struct igmp_source_list);

                    *isl = XCALLOC (MTYPE_IGMP_SRC_LIST, src_list_size);

                    if (! *isl)
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
                                    * (*isl)->isl_num;

                    *isl = XREALLOC (MTYPE_IGMP_SRC_LIST,
                                     *isl, src_list_size);

                    if (! *isl)
                      {
                        IGMP_DBG_ERR (igi, DECODE, "Cannot allocate "
                                      "memory (%d) @ %s:%d",
                                      src_list_size,
                                      __FILE__, __LINE__);

                        ret = IGMP_ERR_OOM;
                        goto EXIT;
                      }
                  }
                IPV4_ADDR_COPY (&(*isl)->isl_saddr [(*isl)->isl_num],
                                &igssms->igssms_msrc);
                (*isl)->isl_num += 1;
                break;

              case FILTER_DENY:
              case FILTER_DYNAMIC:
              case FILTER_NO_MATCH:
                break;
              }
          }
    }

EXIT:

  IGMP_FN_EXIT (ret);
}
 
/* Compare function to insert igmp_if in AVL tree */
int
igmp_ifindex_cmp (void *data1, void *data2)
{
  struct igmp_if *igif1 = (struct igmp_if *)data1;
  struct igmp_if *igif2 = (struct igmp_if *)data2;
   
  if (data1 == NULL
      || data2 == NULL
      || igif1->igif_idx.igifidx_ifp == NULL
      || igif2->igif_idx.igifidx_ifp == NULL)
    return -1;

  if (igif1->igif_owning_ifp > igif2->igif_owning_ifp)
    return 1;

  if (igif2->igif_owning_ifp > igif1->igif_owning_ifp)
    return -1;

  if (igif1->igif_su_id > igif2->igif_su_id)
    return 1;

  if (igif2->igif_su_id > igif1->igif_su_id)
    return -1;

  return 0;
}
