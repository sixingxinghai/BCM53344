/* Copyright (C) 2009-10  All Rights Reserved. */

/**@file  - nsm_mpls_bfd.c
 *
 * @brief - This file contains the functions to give BFD support for 
 * MPLS LSPs and VCCV
 */

#include "pal.h"

#if defined (HAVE_MPLS_OAM) && defined (HAVE_BFD)
#include "lib.h"
#include "table.h"
#include "bfd_client_api.h"

#include "nsmd.h"
#include "nsm_server.h"
#include "nsm_router.h"
#include "nsm_debug.h"

#ifdef HAVE_DIFFSERV
#include "diffserv.h"
#endif /* HAVE_DIFFSERV */

#include "nsm_mpls.h"

#ifdef HAVE_MPLS_VC
#include "nsm_mpls_vc.h"
#endif /* HAVE_MPLS_VC */
#include "nsm_mpls_rib.h"
#include "nsm_mpls_bfd.h"

#ifdef HAVE_VCCV
#include "mpls_util.h"
#endif /* HAVE_VCCV */

/**@brief - This function converts the LSP Name to the LSP Type. 

 *  @param *lsp_name - LSP Type Name : LDP/RSVP/Static. 

 *  @return LSP Type value 
 */
u_int16_t
nsm_mpls_bfd_lsp_name2type (char *lsp_name)
{
  if (pal_strcmp (lsp_name, "ldp") == 0)
    return BFD_MPLS_LSP_TYPE_LDP;
  else if (pal_strcmp (lsp_name, "rsvp") == 0)
    return BFD_MPLS_LSP_TYPE_RSVP;
  else if (pal_strcmp (lsp_name, "static") == 0)
    return BFD_MPLS_LSP_TYPE_STATIC;

  return BFD_MPLS_LSP_TYPE_UNKNOWN;
}

/**@brief - This function converts the LSP Type value to LSP Name. 

 *  @param lsp_type - LSP Type value 

 *  @return LSP Name : ldp/rsvp/static/unknown
 */
char *
nsm_mpls_bfd_lsp_type2name (u_int16_t lsp_type)
{
  switch (lsp_type)
    {
    case BFD_MPLS_LSP_TYPE_LDP:
      return "ldp";
    case BFD_MPLS_LSP_TYPE_RSVP:
      return "rsvp";
    case BFD_MPLS_LSP_TYPE_STATIC:
      return "static";
    }

  return "unknown";
}

/**@brief - This function converts the LSP Type value to the LSP Flag bit. 

 *  @param lsp_type - LSP Type value 

 *  @return LSP Flag or NSM_FAILURE 
 */
int
nsm_mpls_bfd_lsp_type2flag (u_int16_t lsp_type)
{
  switch (lsp_type)
    {
    case BFD_MPLS_LSP_TYPE_LDP:
      return NSM_MPLS_BFD_ALL_LDP;
    case BFD_MPLS_LSP_TYPE_RSVP:
      return NSM_MPLS_BFD_ALL_RSVP;
    case BFD_MPLS_LSP_TYPE_STATIC:
      return NSM_MPLS_BFD_ALL_STATIC;
    }

  return NSM_FAILURE;
}

/**@brief - This function converts the LSP Type value to the MPLS FTN Owner. 

 *  @param lsp_type - LSP Type value 

 *  @return MPLS FTN Owner 
 */
u_char
nsm_mpls_bfd_lsp_type2ftn_owner (u_int16_t lsp_type)
{
  switch (lsp_type)
    {
    case BFD_MPLS_LSP_TYPE_LDP:
      return MPLS_LDP;
    case BFD_MPLS_LSP_TYPE_RSVP:
      return MPLS_RSVP;
    case BFD_MPLS_LSP_TYPE_STATIC:
      return MPLS_OTHER_CLI;
    }

  return MPLS_UNKNOWN; 
}

/**@brief - This function converts the MPLS FTN Owner to the LSP Type value. 

 *  @param owner - MPLS FTN Owner 

 *  @return LSP Type value 
 */
u_int16_t
nsm_mpls_bfd_ftn_owner2lsp_type (u_char owner)
{
  switch (owner)
    {
    case MPLS_LDP:
      return BFD_MPLS_LSP_TYPE_LDP;
    case MPLS_RSVP:
      return BFD_MPLS_LSP_TYPE_RSVP;
    case MPLS_OTHER_CLI:
      return BFD_MPLS_LSP_TYPE_STATIC;
    }

  return BFD_MPLS_LSP_TYPE_UNKNOWN;
}

/**@brief - Lookup for BFD FEC Conf Entry in the BFD FEC Conf List 
 *  based on input.

 *  @param *bfd_fec_conf_list - BFD for FEC/trunk Configuration list.
 *  @param lsp_type - LSP Type 
 *  @param *input - FEC Prefix/RSVP Trunk name 

 *  @return BFD for FEC/Trunk Conf entry or NULL 
 */
struct nsm_mpls_bfd_fec_conf *
nsm_mpls_bfd_fec_conf_lookup (struct list *bfd_fec_conf_list, 
                              u_int16_t lsp_type, void *input)
{
  struct nsm_mpls_bfd_fec_conf *bfd_conf = NULL;
  struct listnode *ln = NULL;
  struct prefix *fec = NULL;
  char *rsvp_trunk = NULL;

  if ((! bfd_fec_conf_list) || (! input))
    return NULL;

  if (lsp_type == BFD_MPLS_LSP_TYPE_LDP ||
      lsp_type == BFD_MPLS_LSP_TYPE_STATIC)
    fec = (struct prefix *)input;
  else if (lsp_type == BFD_MPLS_LSP_TYPE_RSVP)
    rsvp_trunk = (char *)input;
  else
    return NULL;

  LIST_LOOP (bfd_fec_conf_list, bfd_conf, ln)
    {
      if (bfd_conf->lsp_type == lsp_type)
        {
          if (lsp_type == BFD_MPLS_LSP_TYPE_RSVP)
            {
              if (rsvp_trunk && 
                  pal_strcmp (bfd_conf->tunnel_name, rsvp_trunk) == 0)
                return bfd_conf;
            }
          else
            {
              if (fec && prefix_same (&bfd_conf->fec, fec))
                return bfd_conf;
            }
        }
    } 

  return NULL;
}

/**@brief - This function frames and Sends the FTN Down data message 
 * to the corresponding FTN Owner.

 *  @param *nm - NSM Master structure.
 *  @param *ftn - FTN Entry 
 *  @param lsp_type - LSP Type Value 

 *  @return Success (0) or Failure (-1)
 */
int
nsm_mpls_bfd_send_ftn_down (struct nsm_master *nm, struct ftn_entry *ftn,
                             u_int32_t lsp_type)
{
  struct nsm_server_client *nsc = NULL;
  struct nsm_server_entry *nse = NULL;
  struct ftn_down_data fdd;
  u_char proto;
  int nbytes;
  int i;

  if (! nm || ! NSM_MPLS)
    return NSM_FAILURE;

  if (!ftn)
    return NSM_FAILURE;

  pal_mem_set (&fdd, 0, sizeof (struct ftn_down_data));

  /* Filling the FTN Down message. */
  if (lsp_type == BFD_MPLS_LSP_TYPE_LDP)
    {
      proto = APN_PROTO_LDP;
      fdd.ftn_owner = MPLS_LDP;
      pal_mem_cpy (&fdd.u.ldp_key, &ftn->owner.u.l_key.u.ipv4, 
                    sizeof (struct ldp_key_ipv4));
    }
  else if (lsp_type == BFD_MPLS_LSP_TYPE_RSVP)
    {
      proto = APN_PROTO_RSVP;
      fdd.ftn_owner = MPLS_RSVP;
      pal_mem_cpy (&fdd.u.rsvp_key, &ftn->owner.u.r_key.u.ipv4,
                    sizeof (struct rsvp_key_ipv4));
    }
  else
    return NSM_FAILURE;

  NSM_SERVER_CLIENT_LOOP (ng, i, nsc, nse)
    if (nse->service.protocol_id == proto)
      {
        /* Set nse pointer and size. */
        nse->send.pnt = nse->send.buf + NSM_MSG_HEADER_SIZE;
        nse->send.size = nse->send.len - NSM_MSG_HEADER_SIZE;

        /* Encode MPLS FTN Down message. */
        nbytes = nsm_encode_ftn_down_data (&nse->send.pnt, &nse->send.size, 
                                            &fdd);
        if (nbytes < 0)
          return NSM_FAILURE;

        /* Send MPLS FTN Down message. */
        return nsm_server_send_message (nse, 0, 0, NSM_MSG_MPLS_FTN_DOWN, 0, 
                                         nbytes);
      }

  return NSM_FAILURE;
}

/**@brief - Util function for BFD MPLS LSP session Down handling.

 *  @param *s - BFD Client Api Session structure.

 *  @return NONE
 */
void
nsm_mpls_bfd_session_down (struct bfd_client_api_session *s)
{
  struct nsm_master *nm = NULL;
  struct ftn_entry *ftn = NULL;
  u_char *tun_name = NULL;
  struct prefix fec;
  u_int16_t lsp_type;
 
  nm = nsm_master_lookup_by_id (nzg, s->vr_id);
  if (! nm || ! NSM_MPLS)
    return;

  lsp_type = s->addl_data.mpls_params.lsp_type;
  pal_mem_cpy (&fec, &s->addl_data.mpls_params.fec, 
                sizeof (struct prefix_ipv4));
  tun_name = s->addl_data.mpls_params.tun_name;

  /* For LDP and RSVP FTNs, the FTN Down msg is sent to the respective 
   * LSP Owner. For Static LSPs the info is just logged.
   */
  if (lsp_type == BFD_MPLS_LSP_TYPE_LDP)
    {
      ftn = nsm_gmpls_get_ldp_ftn (nm, &fec);
      if (IS_NSM_DEBUG_BFD)
        zlog_info (nzg, "LDP FEC: %O Down receievd from BFD\n", 
                    &s->addl_data.mpls_params.fec); 
    }
  else if (lsp_type == BFD_MPLS_LSP_TYPE_RSVP)
    {
      if (tun_name)
        ftn = nsm_gmpls_get_rsvp_ftn_by_tunnel (nm, tun_name, RSVP_TUNNEL_NAME);

      if (IS_NSM_DEBUG_BFD)
        zlog_info (nzg, "RSVP Tunnel: %s Down receievd from BFD\n", 
                    s->addl_data.mpls_params.tun_name); 
    }
  else
   {
     if (IS_NSM_DEBUG_BFD)
       zlog_info (nzg, "Static FEC: %O Down receievd from BFD\n", 
                   &s->addl_data.mpls_params.fec); 
   }

  /* Send the FTN Down to the LSP Owner. */
  if (ftn)
    nsm_mpls_bfd_send_ftn_down (nm, ftn, lsp_type);
}

#ifdef HAVE_VCCV
/**@brief - Util function for BFD VCCV session Down handling.

 *  @param *s - BFD Client Api Session structure.

 *  @return NONE
 */
void
nsm_vccv_bfd_session_down (struct bfd_client_api_session *s)
{
  if (IS_NSM_DEBUG_BFD)
    zlog_info (nzg, "BFD VCCV Down receievd for VC ID: %d\n", 
               &s->addl_data.vccv_params.vc_id); 
}
#endif /* HAVE_VCCV */

/**@brief - This function enables BFD and sends session adds for all the FTNs 
 *  in the FTN Table, which have been configured for BFD Monitoring.
 *  It is invoked when BFD connects to NSM. 

 *  @param *nm - NSM Master Structure.

 *  @return NONE
 */
void
nsm_mpls_bfd_enable (struct nsm_master *nm)
{
  struct nsm_mpls_bfd_fec_conf *bfd_conf = NULL;
  struct listnode *ln = NULL;
  struct ftn_entry *ftn, *list;
  struct ptree_node *pn = NULL;
  struct ptree *pt = NULL;
  u_int16_t lsp_type;

  if (! nm || ! NSM_MPLS)
    return;

  pt = FTN_TABLE4;
  if (!pt)
    return;

  /* Initially looping through the BFD FEC conf list and 
   * sending session adds for each BFD configured FEC. 
   */
  LIST_LOOP (NSM_MPLS->bfd_fec_conf_list, bfd_conf, ln)
    {
      if (! CHECK_FLAG (bfd_conf->cflag, NSM_MPLS_BFD_FEC_DISABLE))
        nsm_mpls_bfd_fec_conf_session_add (nm, bfd_conf, PAL_FALSE);
    }

  /* If BFD was configured for any of the LSP-types globally 
   * then loop through all the FTNs and send session adds for those 
   * belonging to above LSP-type. */
  if (CHECK_FLAG (NSM_MPLS->bfd_flag, NSM_MPLS_BFD_ALL_LDP) ||
      CHECK_FLAG (NSM_MPLS->bfd_flag, NSM_MPLS_BFD_ALL_RSVP) ||
      CHECK_FLAG (NSM_MPLS->bfd_flag, NSM_MPLS_BFD_ALL_STATIC))
    {
      for (pn = ptree_top (pt); pn; pn = ptree_next (pn))
        {
          list = pn->info;
          if (! list)
            continue;

          for (ftn = list; ftn; ftn = ftn->next)
            {
              if (! CHECK_FLAG (ftn->flags, FTN_ENTRY_FLAG_INSTALLED))
                continue;

              lsp_type = nsm_mpls_bfd_ftn_owner2lsp_type (ftn->owner.owner);
              if (lsp_type == BFD_MPLS_LSP_TYPE_UNKNOWN)
                continue;

              /* If BFD is not configured for this LSP-Type then continue. */
              if (! CHECK_FLAG (NSM_MPLS->bfd_flag, 
                                nsm_mpls_bfd_lsp_type2flag (lsp_type)))
                continue;

              /* Send BFD session add only if BFD is not already enabled
               * for this FEC.
               */
              if (! CHECK_FLAG (ftn->bfd_flag, NSM_MPLS_BFD_CONFIGURED))
                nsm_mpls_bfd_lsp_update_session_by_ftn (nm, ftn, lsp_type, 
                                                        PAL_TRUE, PAL_FALSE);
            }
        }
    }
}

/**@brief - This function disables BFD for all the FTNs in the FTN Table, 
 *  which have been enabled with BFD Monitoring initially.
 *  It is invoked when BFD disconnects with NSM. 

 *  @param *nm - NSM Master Structure.

 *  @return NONE
 */
void
nsm_mpls_bfd_disable (struct nsm_master *nm)
{
  struct ftn_entry *ftn = NULL, *list = NULL;
  struct ptree_node *pn = NULL;
  struct ptree *pt = NULL;


  if (! nm || ! NSM_MPLS)
    return;

  pt = FTN_TABLE4;
  if (! pt)
    return;

  /* Loop through all the FTNs and Unset the BFD enabled flag in each if
   * it was already enabled. 
   */
  for (pn = ptree_top (pt); pn ; pn = ptree_next (pn))
    {
      list = pn->info;
      if (! list)
        continue;

      for (ftn = list; ftn; ftn = ftn->next)
        {
          if (CHECK_FLAG (ftn->bfd_flag, NSM_MPLS_BFD_ENABLED))
            UNSET_FLAG (ftn->bfd_flag, NSM_MPLS_BFD_ENABLED);
        }
    }
}

/**@brief - This function checks if the BFD attributes for the BFD FEC Conf 
 * entry have been modified. If yes, sends BFD session Add for that FEC.

 *  @param *nm - NSM Master Structure.
 *  @param *bfd_conf - BFD FEC Conf entry.
 *  @param *input - Temp BFD FEC Conf entry.

 *  @return Success (0) if the attributes are modified else
 *          Failure (-1)
 */
int
nsm_mpls_bfd_fec_conf_update (struct nsm_master *nm,
                              struct nsm_mpls_bfd_fec_conf *bfd_conf, 
                              struct nsm_mpls_bfd_fec_conf *input)
{
  bool_t is_update = PAL_FALSE;

  /* Need to Update, If BFD was Disabled for this entry initially or
   * any of its attrbutes has been changed. 
   */
  if (CHECK_FLAG (bfd_conf->cflag, NSM_MPLS_BFD_FEC_DISABLE))
    {
      is_update = PAL_TRUE;
      UNSET_FLAG (bfd_conf->cflag, NSM_MPLS_BFD_FEC_DISABLE);
    }
  else if (bfd_conf->lsp_ping_intvl != input->lsp_ping_intvl)
    is_update = PAL_TRUE;
  else if (bfd_conf->min_tx != input->min_tx)
    is_update = PAL_TRUE;
  else if (bfd_conf->min_rx != input->min_rx)
    is_update = PAL_TRUE;
  else if (bfd_conf->mult != input->mult)
    is_update = PAL_TRUE;
  else if (bfd_conf->force_explicit_null != input->force_explicit_null)
    is_update = PAL_TRUE;

  /* Send BFD Update Attribute. */
  if (is_update)
    {
      bfd_conf->lsp_ping_intvl = input->lsp_ping_intvl;
      bfd_conf->min_tx = input->min_tx;
      bfd_conf->min_rx = input->min_rx;
      bfd_conf->mult = input->mult;
      bfd_conf->force_explicit_null = input->force_explicit_null;

      return nsm_mpls_bfd_fec_conf_session_add (nm, bfd_conf, PAL_TRUE);
    }
  else
    return NSM_MPLS_BFD_ERR_ENTRY_EXISTS;
}

/* Check if the BFD LSP attributes have been changed. If yes, 
 * update the attributes. 
 */
/**@brief - This function checks if the BFD attributes for the BFD LSP Conf 
 * entry have been modified.

 *  @param *nm - NSM Master Structure.
 *  @param *input - BFD LSP Conf entry of the LSP Type.
 *  @param lsp_type - LSP Type value.

 *  @return Success (0) if the attributes are modified else
 *          Failure (-1)
 */
int
nsm_mpls_bfd_lsp_conf_update (struct nsm_master *nm, 
                              struct nsm_mpls_bfd_lsp_conf *input, 
                              u_int16_t lsp_type)
{
  bool_t is_update = PAL_FALSE;

  if ((! nm) || (! NSM_MPLS))
    return NSM_FAILURE;

  if (NSM_MPLS->bfd_lsp_conf [lsp_type].lsp_ping_intvl != input->lsp_ping_intvl)
    is_update = PAL_TRUE;
  else if (NSM_MPLS->bfd_lsp_conf [lsp_type].min_tx != input->min_tx)
    is_update = PAL_TRUE;
  else if (NSM_MPLS->bfd_lsp_conf [lsp_type].min_rx != input->min_rx)
    is_update = PAL_TRUE;
  else if (NSM_MPLS->bfd_lsp_conf [lsp_type].mult != input->mult)
    is_update = PAL_TRUE;
  else if (NSM_MPLS->bfd_lsp_conf [lsp_type].force_explicit_null != 
      input->force_explicit_null)
    is_update = PAL_TRUE;

  if (is_update)
    return NSM_SUCCESS;
  else
    return NSM_MPLS_BFD_ERR_ENTRY_EXISTS;
}

/**@brief - This function creates a new BFD FEC Conf entry, 
 *  initializes it's  values with the input or default values
 *  and add it to the BFD conf list.

 *  @param *mpls_bfd_fec_conf_list - BFD FEC Conf List.
 *  @param *input - Temp BFD FEC Conf entry with input values.

 *  @return New BFD FEC Conf Entry or NULL 
 */
struct nsm_mpls_bfd_fec_conf *
nsm_mpls_bfd_fec_conf_new (struct list *bfd_fec_conf_list, 
                           struct nsm_mpls_bfd_fec_conf *input)
{
  struct nsm_mpls_bfd_fec_conf *bfd_conf = NULL;

  if (! bfd_fec_conf_list)
    return NULL;
    
  bfd_conf = XCALLOC (MTYPE_NSM_MPLS_BFD_CONF, 
                       sizeof (struct nsm_mpls_bfd_fec_conf));
  if (! bfd_conf)
    return NULL;

  /* Initialize the FEC prefix. */ 
  if (input->lsp_type == BFD_MPLS_LSP_TYPE_LDP ||
      input->lsp_type == BFD_MPLS_LSP_TYPE_STATIC)
    {
      pal_mem_cpy (&bfd_conf->fec, &input->fec, sizeof (struct prefix));
    }
  /* Initialize the RSVP Tunnel-name. */
  else if (input->lsp_type == BFD_MPLS_LSP_TYPE_RSVP)
    {
      bfd_conf->tunnel_name = pal_strdup (MTYPE_TMP, input->tunnel_name);
     if (! bfd_conf->tunnel_name) 
       {
         XFREE (MTYPE_NSM_MPLS_BFD_CONF, bfd_conf);
         return NULL;
       }
    }

  bfd_conf->lsp_type = input->lsp_type;
  bfd_conf->lsp_ping_intvl = input->lsp_ping_intvl;
  bfd_conf->min_tx = input->min_tx;
  bfd_conf->min_rx = input->min_rx;
  bfd_conf->mult = input->mult;
  bfd_conf->force_explicit_null = input->force_explicit_null;

  /* Add the entry to the BFD FEC conf list.  */
  listnode_add (bfd_fec_conf_list, bfd_conf);

  return bfd_conf;
}

/**@brief - This function looks up for the FTN entry based on the BFD FEC conf 
 * entry parameters and sends the session Add request for the FEC/Trunk, 
 * if the FTN entry is installed in the Forwarder.

 *  @param *nm - NSM Master Structure.
 *  @param *bfd_conf - BFD FEC Conf entry.
 *  @param is_update - bool varable : update/new request.

 *  @return Success (0) or Failure (-1) 
 */
int
nsm_mpls_bfd_fec_conf_session_add (struct nsm_master *nm, 
                                   struct nsm_mpls_bfd_fec_conf *bfd_conf,
                                   bool_t is_update)
{
  struct ftn_entry *ftn = NULL;
  struct fec_gen_entry fec;

  if (! nm || ! NSM_MPLS || !bfd_conf)
    return NSM_FAILURE;

  /* Get the installed ftn entry correspondng to the FEC/Tunnel name. */
  if (bfd_conf->lsp_type == BFD_MPLS_LSP_TYPE_LDP)
    {
      ftn = nsm_gmpls_get_ldp_ftn (nm, &bfd_conf->fec);
    }
  else if (bfd_conf->lsp_type == BFD_MPLS_LSP_TYPE_STATIC)
    {
      NSM_MPLS_PREFIX_TO_FEC_GEN_ENTRY (&fec, &bfd_conf->fec);
      ftn = nsm_gmpls_get_static_ftn (nm, &fec);
    }
  else if (bfd_conf->lsp_type == BFD_MPLS_LSP_TYPE_RSVP)
    {
      ftn = nsm_gmpls_get_rsvp_ftn_by_tunnel (nm, bfd_conf->tunnel_name, 
                                              RSVP_TUNNEL_NAME);
    }
  else
    return NSM_MPLS_BFD_ERR_LSP_UNKNOWN;

  SET_FLAG (bfd_conf->cflag, NSM_MPLS_BFD_FEC_ENABLE);

  if (! ftn)
    {
      if (IS_NSM_DEBUG_BFD)
        {
          if (bfd_conf->lsp_type == BFD_MPLS_LSP_TYPE_RSVP)
            zlog_info (NSM_ZG, "Installed FTN entry not found for the RSVP "
                               "Tunnel: %s", 
                       bfd_conf->tunnel_name);
          else
            zlog_info (NSM_ZG, "Installed FTN entry not found for the %s "
                               "FEC: %O", 
                       nsm_mpls_bfd_lsp_type2name (bfd_conf->lsp_type), 
                       &bfd_conf->fec);
        }

      return NSM_SUCCESS;
    }

  if (! CHECK_FLAG (ftn->bfd_flag, NSM_MPLS_BFD_CONFIGURED))
    SET_FLAG (ftn->bfd_flag, NSM_MPLS_BFD_CONFIGURED);

  if (CHECK_FLAG (ftn->bfd_flag, NSM_MPLS_BFD_DISABLED))
    UNSET_FLAG (ftn->bfd_flag, NSM_MPLS_BFD_DISABLED);

  /* Not an update & BFD has already been enabled for this FTN entry means
   * global LSP attributes have been used. So, check if the new attributes 
   * are same as its LSP attributes. If same, do nothing else send session Add.
   */
  if ((! is_update) && CHECK_FLAG (ftn->bfd_flag, NSM_MPLS_BFD_ENABLED))
    {
      if (! nsm_mpls_bfd_lsp_fec_conf_attr_diff (nm, bfd_conf))
        return NSM_SUCCESS;
    }

  /* Send BFD session Add for the entry. */
  return nsm_mpls_bfd_update_session_by_fec (nm, bfd_conf, ftn, PAL_TRUE, 
                                              PAL_FALSE);
}

/**@brief - This function disables BFD for the FEC conf entry,
 * looks up for the FTN entry based on the BFD FEC conf entry parameters and 
 * sends the session Delete for the FEC/Trunk,
 * if BFD has already been enabled for the entry.

 *  @param *nm - NSM Master Structure.
 *  @param *bfd_conf - BFD FEC Conf entry.

 *  @return Success (0) or Failure (-1) 
 */
int
nsm_mpls_bfd_fec_disable (struct nsm_master *nm, 
                          struct nsm_mpls_bfd_fec_conf *bfd_conf)
{
  struct ftn_entry *ftn = NULL;
  struct fec_gen_entry fec;
  int ret = NSM_SUCCESS;

  if (! nm || ! NSM_MPLS || ! bfd_conf)
    return NSM_FAILURE;

  /* Get the installed ftn entry correspondng to the FEC/Tunnel name. */
  if (bfd_conf->lsp_type == BFD_MPLS_LSP_TYPE_LDP)
    {
      ftn = nsm_gmpls_get_ldp_ftn (nm, &bfd_conf->fec);
    }
  else if (bfd_conf->lsp_type == BFD_MPLS_LSP_TYPE_STATIC)
    {
      NSM_MPLS_PREFIX_TO_FEC_GEN_ENTRY (&fec, &bfd_conf->fec);
      ftn = nsm_gmpls_get_static_ftn (nm, &fec);
    }
  else if (bfd_conf->lsp_type == BFD_MPLS_LSP_TYPE_RSVP)
    {
      ftn = nsm_gmpls_get_rsvp_ftn_by_tunnel (nm, bfd_conf->tunnel_name, 
                                              RSVP_TUNNEL_NAME);
    }
  else
    return NSM_MPLS_BFD_ERR_LSP_UNKNOWN;

  /* Set the BFD_DISABLE Flag. */
  UNSET_FLAG (bfd_conf->cflag, NSM_MPLS_BFD_FEC_ENABLE);
  SET_FLAG (bfd_conf->cflag, NSM_MPLS_BFD_FEC_DISABLE);

  /* Set the BFD attributes of the entry to defaults. */
  bfd_conf->lsp_ping_intvl = BFD_MPLS_LSP_PING_INTERVAL_DEF;
  bfd_conf->min_tx = BFD_MPLS_MIN_TX_INTERVAL_DEF;
  bfd_conf->min_rx = BFD_MPLS_MIN_RX_INTERVAL_DEF;
  bfd_conf->mult = BFD_MPLS_DETECT_MULT_DEF;
  bfd_conf->force_explicit_null = PAL_FALSE;

  if (ftn)
    {
      SET_FLAG (ftn->bfd_flag, NSM_MPLS_BFD_CONFIGURED);

      /* If BFD has already been enabled for this entry, 
       * send BFD session delete for the entry.
       */
      if (CHECK_FLAG (ftn->bfd_flag, NSM_MPLS_BFD_ENABLED))
        {
          /* Send BFD session delete */
          ret = nsm_mpls_bfd_update_session_by_fec (nm, bfd_conf, ftn, 
                                                     PAL_FALSE, PAL_TRUE);
        }

      if (ret == NSM_SUCCESS)
        SET_FLAG (ftn->bfd_flag, NSM_MPLS_BFD_DISABLED);
      else
        return ret;
    }

  return NSM_SUCCESS;
}

/**@brief - This function checks if BFD FEC conf attributes are different 
 * from its LSP type global attributes. 

 *  @param *nm - NSM Master Structure.
 *  @param *bfd_conf - BFD FEC Conf entry.

 *  @return True (1) : If BFD FEC attributes are different from the 
 *                     LSP global attributes.
 *          False (0) : If they are same. 
 */
bool_t
nsm_mpls_bfd_lsp_fec_conf_attr_diff (struct nsm_master *nm, 
                                     struct nsm_mpls_bfd_fec_conf *bfd_conf)
{
  bool_t is_updated;
  u_int16_t lsp_type;
 
  is_updated = PAL_FALSE;
  lsp_type = bfd_conf->lsp_type;

  if (bfd_conf->lsp_ping_intvl != 
      NSM_MPLS->bfd_lsp_conf [lsp_type].lsp_ping_intvl)
    is_updated = PAL_TRUE;
  else if (bfd_conf->min_tx != NSM_MPLS->bfd_lsp_conf [lsp_type].min_tx)
    is_updated = PAL_TRUE;
  else if (bfd_conf->min_rx != NSM_MPLS->bfd_lsp_conf [lsp_type].min_rx)
    is_updated = PAL_TRUE;
  else if (bfd_conf->mult != NSM_MPLS->bfd_lsp_conf [lsp_type].mult)
    is_updated = PAL_TRUE; 
  else if (bfd_conf->force_explicit_null != 
      NSM_MPLS->bfd_lsp_conf [lsp_type].force_explicit_null)
    is_updated = PAL_TRUE;

  return is_updated;  
}

/**@brief - This function sends BFD session Add/Delete request for the FEC/trunk
 *  based on the is_add flag

 *  @param *nm - NSM Master Structure.
 *  @param *bfd_conf - BFD FEC Conf entry.
 *  @param *ftn - FTN Entry corresponding to the FEC/Trunk.
 *  @param is_add - bool variable : add/delete.
 *  @param admin_flag - bool flag specifying Admin Down.

 *  @return Success (0) or Failure (-1) 
 */
int
nsm_mpls_bfd_update_session_by_fec (struct nsm_master *nm, 
                                    struct nsm_mpls_bfd_fec_conf *bfd_conf,
                                    struct ftn_entry *ftn,
                                    bool_t is_add,
                                    bool_t admin_flag)
{
  struct bfd_client_api_session s;
  struct gmpls_gen_label tmp_lbl;
  int ret;

  if (! nm || ! NSM_MPLS)
    return NSM_FAILURE;

  if (! ftn || ! bfd_conf)
    return NSM_FAILURE;

  /* Sanity check. */
  if (is_add && (! CHECK_FLAG (ftn->flags, FTN_ENTRY_FLAG_INSTALLED)))
    return NSM_SUCCESS;

  /* BFD session add not sent for directly connetced LSRs. */
  gmpls_nhlfe_outgoing_label (FTN_NHLFE (ftn), &tmp_lbl);
  if (is_add && (tmp_lbl.u.pkt == LABEL_IMPLICIT_NULL))
    {
      if (IS_NSM_DEBUG_BFD)
        {
          if (bfd_conf->lsp_type == BFD_MPLS_LSP_TYPE_RSVP)
            zlog_info (NSM_ZG, "BFD session add not sent for directly connected"
                               " RSVP Tunnel: %s",
                       ftn->sz_desc);
          else
            zlog_info (NSM_ZG, "BFD session add not sent for directly "
                               "connected %s FEC: %O",
                       nsm_mpls_bfd_lsp_type2name (bfd_conf->lsp_type),
                       &bfd_conf->fec);
        }

      return NSM_SUCCESS;
    }

  pal_mem_set (&s, 0, sizeof (struct bfd_client_api_session));

  /* Fill the session data. */
  ret = nsm_mpls_bfd_session_mpls_data_set (nm, &s, ftn, bfd_conf->lsp_type);
  if (ret != NSM_SUCCESS)
    return ret;

  /* Fill the BFD attributes. */
  s.addl_data.mpls_params.lsp_ping_intvl = bfd_conf->lsp_ping_intvl;
  s.addl_data.mpls_params.min_tx = bfd_conf->min_tx;
  s.addl_data.mpls_params.min_rx = bfd_conf->min_rx;
  s.addl_data.mpls_params.mult = bfd_conf->mult;
  if (bfd_conf->force_explicit_null == PAL_TRUE)
    SET_FLAG(s.addl_data.mpls_params.fwd_flags, MPLSONM_FWD_OPTION_NOPHP);

  /* Set Admin down flag. */
  if (admin_flag)
    SET_FLAG (s.flags, BFD_CLIENT_API_FLAG_AD);

  /* Send BFD session Add. */
  if (is_add)
    {
      ret = bfd_client_api_session_add (&s);
     /* Set the BFD enabled Flag in the FTN entry. */
     if (ret == LIB_API_SUCCESS)
       {
         SET_FLAG (ftn->bfd_flag, NSM_MPLS_BFD_ENABLED);
       }
     else
       {
         zlog_err (NSM_ZG, "NSM: BFD session add message send failed!!!");
       }    
    }
  /* Send BFD session Delete. */
  else
    {
      ret = bfd_client_api_session_delete (&s);
      /* Unset the BFD enabled Flag in the FTN entry. */
      if (ret == LIB_API_SUCCESS)
        {
          UNSET_FLAG (ftn->bfd_flag, NSM_MPLS_BFD_ENABLED);
        }
      else
        {
          zlog_err (NSM_ZG, "NSM: BFD session delete message send failed!!!");
        }    
    }

  /* Cleaning tmp memory. */
  if (bfd_conf->lsp_type == BFD_MPLS_LSP_TYPE_RSVP)
    XFREE (MTYPE_TMP, s.addl_data.mpls_params.tun_name);

  return ret;
}

/**@brief - This function deletes the BFD FEC conf entry from the 
 * BFD FEC conf list and sends BFD session delete request for the FEC/Trunk. 

 *  @param *nm - NSM Master Structure.
 *  @param *bfd_conf - BFD FEC Conf entry.

 *  @return Success (0) or Failure (-1) 
 */
int
nsm_mpls_bfd_fec_conf_session_delete (struct nsm_master *nm,
                                      struct nsm_mpls_bfd_fec_conf *bfd_conf)
{
  struct ftn_entry *ftn = NULL;
  struct fec_gen_entry fec;
  bool_t is_update = PAL_FALSE;

  if (! nm || ! NSM_MPLS)
    return NSM_FAILURE;

  /* Get the installed ftn entry correspondng to the FEC/Tunnel name. */
  if (bfd_conf->lsp_type == BFD_MPLS_LSP_TYPE_LDP)
    {
      ftn = nsm_gmpls_get_ldp_ftn (nm, &bfd_conf->fec);
    }
  else if (bfd_conf->lsp_type == BFD_MPLS_LSP_TYPE_STATIC)
    {
      NSM_MPLS_PREFIX_TO_FEC_GEN_ENTRY (&fec, &bfd_conf->fec);
      ftn = nsm_gmpls_get_static_ftn (nm, &fec);
    }
  else if (bfd_conf->lsp_type == BFD_MPLS_LSP_TYPE_RSVP)
    {
      ftn = nsm_gmpls_get_rsvp_ftn_by_tunnel (nm, bfd_conf->tunnel_name,
                                             RSVP_TUNNEL_NAME);
    }

  /* If the global LSP BFD flag for this LSP-Type is set then 
   * check if BFD conf attributes are different from its LSP attributes. 
   * If different then send session Add with its global LSP attributes. 
   * Else if the BFD flag in ftn entry is enabled then send BFD session delete.
   */
  if (ftn)
    {
      UNSET_FLAG (ftn->bfd_flag, NSM_MPLS_BFD_CONFIGURED);

      if (CHECK_FLAG (ftn->bfd_flag, NSM_MPLS_BFD_DISABLED))
        {
          UNSET_FLAG (ftn->bfd_flag, NSM_MPLS_BFD_DISABLED);
          is_update = PAL_TRUE;
        }

      if (CHECK_FLAG (NSM_MPLS->bfd_flag, 
           nsm_mpls_bfd_lsp_type2flag (bfd_conf->lsp_type)))
        {
          if (nsm_mpls_bfd_lsp_fec_conf_attr_diff (nm, bfd_conf) || is_update)
            {
              /* Send Session add with global LSP attributes. */
              nsm_mpls_bfd_lsp_update_session_by_ftn (nm, ftn, 
                                                      bfd_conf->lsp_type, 
                                                      PAL_TRUE, PAL_FALSE);
            }
        }
      else if (CHECK_FLAG (ftn->bfd_flag, NSM_MPLS_BFD_ENABLED))
        /* Send BFD sesssion delete. */
        nsm_mpls_bfd_update_session_by_fec (nm, bfd_conf, ftn, PAL_FALSE, 
                                             PAL_TRUE);
    }

  /* Remove the FEC conf entry from the BFD FEC conf list. */
  listnode_delete (NSM_MPLS->bfd_fec_conf_list, bfd_conf);

  /* Free the allocated memory. */
  if (bfd_conf->lsp_type == BFD_MPLS_LSP_TYPE_RSVP)
    XFREE (MTYPE_TMP, bfd_conf->tunnel_name);

  XFREE (MTYPE_NSM_MPLS_BFD_CONF, bfd_conf);

  return NSM_SUCCESS;
}

/**@brief - This function sends BFD session add requests for all the 
 * installed FTNs of the given LSP-Type.

 *  @param *nm - NSM Master Structure.
 *  @param lsp_type - LSP Type value.

 *  @return Success (0) or Failure (-1) 
 */
int
nsm_mpls_bfd_lsp_session_add (struct nsm_master *nm, u_int16_t lsp_type)
{
  struct ftn_entry *ftn, *list;
  struct ptree_node *pn = NULL;
  struct ptree *pt = NULL;

  if (! nm || ! NSM_MPLS)
    return NSM_FAILURE;

  pt = FTN_TABLE4;
  if (! pt)
    return NSM_FAILURE;

  for (pn = ptree_top (pt); pn; pn = ptree_next (pn))
    {
      list = pn->info;
      if (! list)
        continue;

      for (ftn = list; ftn; ftn = ftn->next)
        {
          if ((ftn->owner.owner == nsm_mpls_bfd_lsp_type2ftn_owner (lsp_type)) 
              && (CHECK_FLAG (ftn->flags, FTN_ENTRY_FLAG_INSTALLED)))
            {
              /* Send BFD session add only if BFD enable/disable is not 
               * explicitly configured for this FEC. 
               */
              if (! (CHECK_FLAG (ftn->bfd_flag, NSM_MPLS_BFD_CONFIGURED) ||
                     CHECK_FLAG (ftn->bfd_flag, NSM_MPLS_BFD_DISABLED)))
                nsm_mpls_bfd_lsp_update_session_by_ftn (nm, ftn, lsp_type, 
                                                        PAL_TRUE, PAL_FALSE);
            }
        }
    }

  return NSM_SUCCESS; 
}

/**@brief - This function sends BFD session Add/Delete 
 * for the FTN entry of an LSP-Type based on the is_add flag.

 *  @param *nm - NSM Master Structure.
 *  @param *ftn - FTN Entry corresponding to the FEC/Trunk.
 *  @param lsp_type - LSP Type value.
 *  @param is_add - bool variable : add/delete.
 *  @param admin_flag - bool flag specifying Admin Down.

 *  @return Success (0) or Failure (-1) 
 */
int
nsm_mpls_bfd_lsp_update_session_by_ftn (struct nsm_master *nm, 
                                        struct ftn_entry *ftn, 
                                        u_int16_t lsp_type,
                                        bool_t is_add,
                                        bool_t admin_flag)
{
  struct bfd_client_api_session s;
  struct gmpls_gen_label tmp_lbl;
  int ret;

  if (! nm || ! NSM_MPLS)
    return NSM_FAILURE;

  if (! ftn)
    return NSM_FAILURE;

  /* Sanity check. */
  if (is_add && (! CHECK_FLAG (ftn->flags, FTN_ENTRY_FLAG_INSTALLED)))
    return NSM_SUCCESS;

  /* BFD session add not sent for RSVP mapped routes. */
  if (ftn->ftn_type == MPLS_FTN_RSVP_MAPPED)
    {
      if (IS_NSM_DEBUG_BFD)
        zlog_info (NSM_ZG, "BFD session add not sent for RSVP mapped routes");

      return NSM_SUCCESS;
    }

  /* BFD session add not sent for directly connetced LSRs. */
  gmpls_nhlfe_outgoing_label (FTN_NHLFE (ftn), &tmp_lbl);
  if (is_add && (tmp_lbl.u.pkt == LABEL_IMPLICIT_NULL))
    {
      if (IS_NSM_DEBUG_BFD)
        {
          if (lsp_type == BFD_MPLS_LSP_TYPE_RSVP)
            zlog_info (NSM_ZG, "BFD session add not sent for directly connected"
                               " RSVP Tunnel: %s",
                       ftn->sz_desc);
          else
            zlog_info (NSM_ZG, "BFD session add not sent for directly "
                               "connected %s FEC",
                       nsm_mpls_bfd_lsp_type2name (lsp_type));
        }

      return NSM_SUCCESS;
    }

  pal_mem_set (&s, 0, sizeof (struct bfd_client_api_session));

  /* Fill the Session data. */
  ret = nsm_mpls_bfd_session_mpls_data_set (nm, &s, ftn, lsp_type);
  if (ret != NSM_SUCCESS)
    return ret;

  /* Fill BFD attributes. */
  s.addl_data.mpls_params.lsp_ping_intvl = 
    NSM_MPLS->bfd_lsp_conf [lsp_type].lsp_ping_intvl;
  s.addl_data.mpls_params.min_tx = NSM_MPLS->bfd_lsp_conf [lsp_type].min_tx;
  s.addl_data.mpls_params.min_rx = NSM_MPLS->bfd_lsp_conf [lsp_type].min_rx;
  s.addl_data.mpls_params.mult = NSM_MPLS->bfd_lsp_conf [lsp_type].mult;
  if (NSM_MPLS->bfd_lsp_conf [lsp_type].force_explicit_null == PAL_TRUE)
    SET_FLAG(s.addl_data.mpls_params.fwd_flags, MPLSONM_FWD_OPTION_NOPHP);

  /* Set Admin down flag. */
  if (admin_flag)
    SET_FLAG (s.flags, BFD_CLIENT_API_FLAG_AD);

  /* Send BFD session Add. */
  if (is_add)
    {
      ret = bfd_client_api_session_add (&s);
     /* Set the BFD enabled Flag in BFD Conf entry and FTN entry. */
     if (ret == LIB_API_SUCCESS)
       {
         SET_FLAG (ftn->bfd_flag, NSM_MPLS_BFD_ENABLED);
       }
     else
       {
         zlog_err (NSM_ZG, "NSM: BFD session add message send failed!!!");
       }    
    }
  /* Send BFD session Delete. */
  else
    {
      ret = bfd_client_api_session_delete (&s);
      /* Unset the BFD enabled Flag in BFD Conf entry and FTN entry. */
      if (ret == LIB_API_SUCCESS)
        {
          UNSET_FLAG (ftn->bfd_flag, NSM_MPLS_BFD_ENABLED);
        }
      else
        {
          zlog_err (NSM_ZG, "NSM: BFD session delete message send failed!!!");
        }    
    }

  return ret;
}

/**@brief - This function sends the BFD session delete requests for all the 
 * installed FTNs of the given LSP-Type. 

 *  @param *nm - NSM Master Structure.
 *  @param lsp_type - LSP Type value.

 *  @return Success (0) or Failure (-1) 
 */
int
nsm_mpls_bfd_lsp_session_delete (struct nsm_master *nm, u_int16_t lsp_type)
{
  struct ftn_entry *ftn, *list;
  struct ptree_node *pn = NULL;
  struct ptree *pt = NULL;

  if (! nm || ! NSM_MPLS)
    return NSM_FAILURE;

  pt = FTN_TABLE4;
  if (! pt)
    return NSM_FAILURE;

  for (pn = ptree_top (pt); pn; pn = ptree_next (pn))
    {
      list = pn->info;
      if (! list)
        continue;

      for (ftn = list; ftn; ftn = ftn->next)
        {
          if ((ftn->owner.owner == nsm_mpls_bfd_lsp_type2ftn_owner (lsp_type))
              && (CHECK_FLAG (ftn->flags, FTN_ENTRY_FLAG_INSTALLED)))
            {
              /* Send BFD session delete only if the BFD enabled flag is set and
               * config flag is not set 
               * which means BFD is not configured for this FEC expilicitly.
               */
              if (CHECK_FLAG (ftn->bfd_flag, NSM_MPLS_BFD_ENABLED) &&
                  ! CHECK_FLAG (ftn->bfd_flag, NSM_MPLS_BFD_CONFIGURED))
                nsm_mpls_bfd_lsp_update_session_by_ftn (nm, ftn, 
                                                        lsp_type, PAL_FALSE, 
                                                        PAL_TRUE);
            }
        }
    }

  return NSM_SUCCESS;
}

/**@brief - This function frames the MPLS data for the 
 * BFD Client Session structure.

 *  @param *nm - NSM Master Structure.
 *  @param *s - BFD Client session structure.
 *  @param *ftn - FTN Entry corresponding to the FEC.
 *  @param lsp_type - LSP Type value.

 *  @return Success (0) or Failure (-1) 
 */
int
nsm_mpls_bfd_session_mpls_data_set (struct nsm_master *nm,
                                    struct bfd_client_api_session *s,
                                    struct ftn_entry *ftn,
                                    u_int16_t lsp_type)
{
  struct nsm_vrf *nv = NULL;
  struct gmpls_gen_label tmp_lbl;
  u_char *temp = "127.0.0.12";
  int ret;

  if (! ftn)
    return NSM_FAILURE;

  nv = nsm_vrf_lookup_by_id (nm, VRF_ID_UNSPEC);
  if (! nv)
    return NSM_FAILURE;

  /* Fill session data. */
  s->zg = nzg;
  s->vr_id = nm->vr->id;
  s->vrf_id = nv->vrf->id;
  SET_FLAG (s->flags, BFD_CLIENT_API_FLAG_PS);

#ifdef HAVE_GMPLS
  s->ifindex = nsm_gmpls_get_ifindex_from_gifindex (nm,
      gmpls_nhlfe_outgoing_if (FTN_NHLFE (ftn)));
#else
  s->ifindex = gmpls_nhlfe_outgoing_if (FTN_NHLFE (ftn));
#endif /* HAVE_GMPLS */
  s->ll_type = BFD_MSG_LL_TYPE_MPLS_LSP;
  s->afi = AFI_IP;
  s->addr.ipv4.src = nv->vrf->router_id;
  ret = pal_inet_pton (AF_INET, temp, &s->addr.ipv4.dst);
  if (! ret)
    return NSM_FAILURE;

  s->addl_data.mpls_params.ftn_ix = ftn->ftn_ix;
  s->addl_data.mpls_params.is_egress = PAL_FALSE;
  s->addl_data.mpls_params.lsp_type = lsp_type;

  if (lsp_type == BFD_MPLS_LSP_TYPE_RSVP)
    {
      if (!ftn->sz_desc)
        return NSM_FAILURE;

      s->addl_data.mpls_params.tun_length = pal_strlen (ftn->sz_desc);
      s->addl_data.mpls_params.tun_name = pal_strdup (MTYPE_TMP, ftn->sz_desc);

      if (s->addl_data.mpls_params.tun_name == NULL)
        return NSM_FAILURE;
    }
  else
    NSM_MPLS_GET_FEC_PREFIX4_FROM_FTN (ftn, s->addl_data.mpls_params.fec);

  gmpls_nhlfe_outgoing_label (FTN_NHLFE (ftn), &tmp_lbl);
  s->addl_data.mpls_params.tunnel_label = tmp_lbl.u.pkt;

  return NSM_SUCCESS;
}

/**@brief - This function checks if BFD has been configured for the FEC/Trunk 
 * corresponding to this FTN entry and sends BFD session Add request if yes.

 *  @param *nm - NSM Master Structure.
 *  @param *ftn - FTN Entry corresponding to the FEC.

 *  @return NONE 
 */
void
nsm_mpls_ftn_bfd_update (struct nsm_master *nm, struct ftn_entry *ftn) 
{
  struct nsm_mpls_bfd_fec_conf *bfd_conf = NULL;
  u_int16_t lsp_type;
  struct prefix_ipv4 fec;

  if (! nm || ! NSM_MPLS)
    return;

  if (! ftn || ! NSM_MPLS->bfd_fec_conf_list)
    return;

  lsp_type = nsm_mpls_bfd_ftn_owner2lsp_type (ftn->owner.owner);
  if (lsp_type == BFD_MPLS_LSP_TYPE_UNKNOWN)
    return;

  if (lsp_type == BFD_MPLS_LSP_TYPE_RSVP)
    bfd_conf = nsm_mpls_bfd_fec_conf_lookup (NSM_MPLS->bfd_fec_conf_list, 
                                             lsp_type, ftn->sz_desc);
  else
    {
      NSM_MPLS_GET_FEC_PREFIX4_FROM_FTN (ftn, fec);
      bfd_conf = nsm_mpls_bfd_fec_conf_lookup (NSM_MPLS->bfd_fec_conf_list, 
                                               lsp_type, &fec);
    }

  /* If no BFD conf entry exists for this entry, check if the 
   * global LSP BFD flag is set. 
   */
  if (! bfd_conf)
    {
      if (CHECK_FLAG (NSM_MPLS->bfd_flag, 
           nsm_mpls_bfd_lsp_type2flag (lsp_type)))
        {
          /* Send BFD session add with LSP attributes. */
          nsm_mpls_bfd_lsp_update_session_by_ftn (nm, ftn, lsp_type, PAL_TRUE, 
                                                   PAL_FALSE);
        }
    }
  /* Send BFD session Add with BFD FEC conf attributes. */
  else
    {
      SET_FLAG (ftn->bfd_flag, NSM_MPLS_BFD_CONFIGURED);

      if (! CHECK_FLAG (bfd_conf->cflag, NSM_MPLS_BFD_FEC_DISABLE))
        nsm_mpls_bfd_update_session_by_fec (nm, bfd_conf, ftn, 
                                             PAL_TRUE, PAL_FALSE);
      else
        SET_FLAG (ftn->bfd_flag, NSM_MPLS_BFD_DISABLED);
    }
}

/**@brief - This function sends BFD session delete request for the FEC/trunk
 * corresponding to this FTN entry.

 *  @param *nm - NSM Master Structure.
 *  @param *ftn - FTN Entry corresponding to the FEC.

 *  @return NONE 
 */
void
nsm_mpls_ftn_bfd_delete (struct nsm_master *nm, struct ftn_entry *ftn)
{
  struct nsm_mpls_bfd_fec_conf *bfd_conf = NULL;
  struct prefix_ipv4 fec;
  u_int16_t lsp_type;

  if (! nm || ! NSM_MPLS)
    return;

  if (! ftn || ! NSM_MPLS->bfd_fec_conf_list)
    return;

  lsp_type = nsm_mpls_bfd_ftn_owner2lsp_type (ftn->owner.owner);
  if (lsp_type == BFD_MPLS_LSP_TYPE_UNKNOWN)
    return;

  if (CHECK_FLAG (ftn->bfd_flag, NSM_MPLS_BFD_CONFIGURED))
    {
      if (lsp_type == BFD_MPLS_LSP_TYPE_RSVP)
        bfd_conf = nsm_mpls_bfd_fec_conf_lookup (NSM_MPLS->bfd_fec_conf_list,
                                                 lsp_type, ftn->sz_desc);
      else
        {
          NSM_MPLS_GET_FEC_PREFIX4_FROM_FTN (ftn, fec); 
          bfd_conf = nsm_mpls_bfd_fec_conf_lookup (NSM_MPLS->bfd_fec_conf_list,
                                                   lsp_type, &fec);
        }

      nsm_mpls_bfd_update_session_by_fec (nm, bfd_conf, ftn, PAL_FALSE, 
                                           PAL_FALSE);
    }
  else
    {
      nsm_mpls_bfd_lsp_update_session_by_ftn (nm, ftn, lsp_type, PAL_FALSE, 
                                               PAL_FALSE);
    }
}

#ifdef HAVE_VCCV
/**@brief - This function enables BFD and sends session adds for all the VCs 
 *  in the VC Table, which have been configured for BFD Monitoring.
 *  It is invoked when BFD connects to NSM. 

 *  @param *nm - NSM Master Structure.

 *  @return NONE
 */
void
nsm_vccv_bfd_enable (struct nsm_master *nm)
{
  struct nsm_mpls_circuit *vc = NULL;
  struct route_node *rn = NULL;

  if (!nm || !NSM_MPLS || !NSM_MPLS->vc_table)
    return;

  for (rn = route_top (NSM_MPLS->vc_table); rn; rn = route_next (rn))
    {
      /* If no info, skip. */
      vc = rn->info;
      if (! vc || ! vc->vc_fib || (vc->vc_fib->install_flag == NSM_FALSE))
        continue;

      /* If the VC is installed in the VC Fib and BFD has been configured
       * for the VC entry then send BFD VCCV session Add.
       */
      if (oam_util_get_bfd_cvtype_in_use(vc->cv_types, 
                                   vc->vc_fib->remote_cv_types) != CV_TYPE_NONE)
        nsm_mpls_vc_bfd_session_add (nm, vc);
    }
}

/**@brief - This function resets the BFD running flag for all the VCs in the 
 * VC Table, which have been enabled with BFD Monitoring initially.
 * It is invoked when BFD disconnects with NSM. 

 *  @param *nm - NSM Master Structure.

 *  @return NONE
 */
void
nsm_vccv_bfd_disable (struct nsm_master *nm)
{
  struct nsm_mpls_circuit *vc = NULL;
  struct route_node *rn = NULL;

  if (!nm || !NSM_MPLS || !NSM_MPLS->vc_table)
    return;

  for (rn = route_top (NSM_MPLS->vc_table); rn; rn = route_next (rn))
    {
      /* If no info, skip. */
      vc = rn->info;
      if (! vc || ! vc->vc_fib)
        continue;

      /* If the BFD Running flag is set for the VC, Unset it.
       */
      if (vc->is_bfd_running == PAL_TRUE)
        vc->is_bfd_running = PAL_FALSE;
    }
}

/**@brief - This function frames the VCCV data for the
 * BFD Client Session structure.

 *  @param *nm - NSM Master Structure.
 *  @param *vc - NSM MPLS VC structure.
 *  @param *s - BFD Client session structure.

 *  @return Success (0) or Failure (-1)
 */
int
nsm_mpls_bfd_session_vccv_data_set (struct nsm_master *nm,
                                    struct nsm_mpls_circuit *vc,
                                    struct bfd_client_api_session *s)
{
  struct nsm_vrf *nv = NULL;
  u_char *temp = "127.0.0.12";
  struct ftn_entry *ftn = NULL;
  struct gmpls_gen_label tmp_lbl;
  struct prefix pfx;
  int ret = 0;;

  if (!nm || !vc || !vc->vc_fib || !s)
    return NSM_FAILURE;

  nv = nsm_vrf_lookup_by_id (nm, VRF_ID_UNSPEC);

  if (! nv)
    return NSM_FAILURE;

  pal_mem_cpy (&pfx, &vc->address, sizeof (struct prefix));

  /* This gets a Tunnel for the VC. Try to get MIB code*/
  ftn = nsm_gmpls_ftn_lookup_installed (nm, GLOBAL_FTN_ID, &pfx);
  if (!ftn)
    return MPLS_OAM_ERR_NO_FTN_FOUND;

  if (ftn->xc && ftn->xc->nhlfe)
    gmpls_nhlfe_outgoing_label (FTN_NHLFE (ftn), &tmp_lbl);

  if (! ftn->xc ||
      tmp_lbl.u.pkt == LABEL_IMPLICIT_NULL)
    return MPLS_OAM_ERR_EXPLICIT_NULL;

  s->zg = nzg;
  s->vr_id = nm->vr->id;
  s->vrf_id = nv->vrf->id;
  SET_FLAG (s->flags, BFD_CLIENT_API_FLAG_PS);

#ifdef HAVE_GMPLS
  s->ifindex = nsm_gmpls_get_ifindex_from_gifindex(nm, vc->vc_fib->nw_if_ix);
#else
  s->ifindex = vc->vc_fib->nw_if_ix;
#endif /* HAVE_GMPLS */
  s->ll_type = BFD_MSG_LL_TYPE_MPLS_VCCV;
  s->afi = AFI_IP;
  s->addr.ipv4.src = nv->vrf->router_id;
  ret = pal_inet_pton (AF_INET, temp, &s->addr.ipv4.dst);
  if (! ret)
    return NSM_FAILURE;

  /* Fill VCCV specific data. */
  s->addl_data.vccv_params.vc_id = vc->id;
#ifdef HAVE_GMPLS
  s->addl_data.vccv_params.ac_ix = nsm_gmpls_get_ifindex_from_gifindex (nm, 
                                                          vc->vc_fib->ac_if_ix);
#else
  s->addl_data.vccv_params.ac_ix = vc->vc_fib->ac_if_ix;
#endif /* HAVE_GMPLS */
  s->addl_data.vccv_params.in_vc_label = vc->vc_fib->in_label;
  s->addl_data.vccv_params.out_vc_label = vc->vc_fib->out_label;
  s->addl_data.vccv_params.cc_type = oam_util_get_cctype_in_use(
                                     vc->cc_types, vc->vc_fib->remote_cc_types);
  s->addl_data.vccv_params.cv_type = oam_util_get_bfd_cvtype_in_use(
                                     vc->cv_types, vc->vc_fib->remote_cv_types);
  s->addl_data.vccv_params.tunnel_label = tmp_lbl.u.pkt;

  return NSM_SUCCESS;
}

/**@brief - This function sends BFD session Add message
 * for the VC entry.

 *  @param *nm - NSM Master Structure.
 *  @param *vc - NSM MPLS VC structure.

 *  @return Success (0) or Failure (-1)
 */
int
nsm_mpls_vc_bfd_session_add (struct nsm_master *nm,  
                             struct nsm_mpls_circuit *vc)
{
  struct bfd_client_api_session s;
  int ret;

  if (! nm || ! vc)
    return NSM_FAILURE;

  /* Sanity check. */
  if (vc->vc_fib->install_flag != NSM_TRUE)
    return NSM_FAILURE;

  pal_mem_set (&s, 0, sizeof (struct bfd_client_api_session));

  /* Fill the session data. */
  ret = nsm_mpls_bfd_session_vccv_data_set (nm, vc, &s);
  if (ret != NSM_SUCCESS)
    return ret;

  ret =  bfd_client_api_session_add (&s);
  if (ret == LIB_API_SUCCESS) 
    vc->is_bfd_running = PAL_TRUE;
  else
    return NSM_FAILURE;

  return NSM_SUCCESS;
}

/**@brief - This function sends BFD session Delete message
 * for the VC entry.

 *  @param *nm - NSM Master Structure.
 *  @param *vc - NSM MPLS VC structure.
 *  @param admin_flag - bool flag specifying Admin Down.

 *  @return Success (0) or Failure (-1)
 */
int
nsm_mpls_vc_bfd_session_delete (struct nsm_master *nm,
                                 struct nsm_mpls_circuit *vc,
                                 bool_t admin_flag)
{ 
  struct bfd_client_api_session s;
  int ret;

  if (! nm || ! vc)
    return NSM_FAILURE;

  pal_mem_set (&s, 0, sizeof (struct bfd_client_api_session));

  /* Fill the session data. */
  ret = nsm_mpls_bfd_session_vccv_data_set (nm, vc, &s);
  if (ret != NSM_SUCCESS)
    return ret;

  /* Set Admin down flag. */
  if (admin_flag)
    SET_FLAG (s.flags, BFD_CLIENT_API_FLAG_AD);

  ret =  bfd_client_api_session_delete (&s);
  if (ret == LIB_API_SUCCESS)
    vc->is_bfd_running = PAL_FALSE;
  else
    return NSM_FAILURE;

  return NSM_SUCCESS;
}
#endif /* HAVE_VCCV */
#endif /* HAVE_BFD &&  HAVE_MPLS_OAM */
