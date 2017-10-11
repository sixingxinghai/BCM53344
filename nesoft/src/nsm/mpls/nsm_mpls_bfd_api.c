/* Copyright (C) 2009-10  All Rights Reserved. */

/**@file  - nsm_mpls_bfd_api.c
 *
 * @brief - This file contains the APIs to Set/Unset BFD Fall-over check for
 * MPLS LSPs
 */
#include "pal.h"
#include "lib.h"

#if defined (HAVE_MPLS_OAM) && defined (HAVE_BFD)

#include "nsmd.h"
#include "nsm_router.h"
#include "rib.h"
#include "nsm_api.h"
#ifdef HAVE_DIFFSERV
#include "diffserv.h"
#endif /* HAVE_DIFFSERV */

#include "nsm_mpls.h"
#ifdef HAVE_MPLS_VC
#include "nsm_mpls_vc.h"
#endif /* HAVE_MPLS_VC */

#include "nsm_mpls_bfd.h"
#include "nsm_mpls_bfd_api.h"

#ifdef HAVE_VCCV
#include "mpls_util.h"
#endif /* HAVE_VCCV */

/**@brief - This function sets the BFD fall-over check for the FEC/Trunk 
 *  of an LSP-type.

 *  @param vr_id - vr id of the VR in context.
 *  @param *lsp_name - LSP Type Name : LDP/RSVP/Static. 
 *  @param *input - FEC Prefix/Trunk name.
 *  @param lsp_ping_intvl - LSP Ping Interval in seconds.
 *  @param min_tx - Minimum BFD Tx in milliseconds.
 *  @param min_rx - Minimum BFD Rx in milliseconds.
 *  @param mult - BFD Detection Multiplier value.

 *  @return Error code (< 0) or success (0)
 */
int
nsm_mpls_bfd_api_fec_set (u_int32_t vr_id, char *lsp_name, char *input,
                          u_int32_t lsp_ping_intvl, u_int32_t min_tx, 
                          u_int32_t min_rx, u_int32_t mult, 
                          bool_t force_explicit_null)
{
  struct nsm_master *nm = NULL;
  struct nsm_mpls_bfd_fec_conf *bfd_conf = NULL;
  struct nsm_mpls_bfd_fec_conf temp_conf;
  u_int16_t lsp_type;
  int ret;

  nm = nsm_master_lookup_by_id (nzg, vr_id);
  if (nm == NULL)
    return NSM_API_SET_ERR_VR_NOT_EXIST;

  if ((! NSM_MPLS) || (! NSM_MPLS->bfd_fec_conf_list))
    return NSM_FAILURE;

  lsp_type = nsm_mpls_bfd_lsp_name2type (lsp_name);
  if (lsp_type == BFD_MPLS_LSP_TYPE_UNKNOWN)
    return NSM_MPLS_BFD_ERR_LSP_UNKNOWN;

  /* Copy the received input/default attributes into the temp conf. */
  temp_conf.lsp_type = lsp_type;
  temp_conf.tunnel_name = NULL;

  temp_conf.lsp_ping_intvl = lsp_ping_intvl;
  temp_conf.min_tx = min_tx;
  temp_conf.min_rx = min_rx;
  temp_conf.mult = mult;
  temp_conf.force_explicit_null = force_explicit_null;

  /* LSP Ping interval should be greater than BFD min Tx & min Rx intervals by
   * atleast BFD_MPLS_MIN_LSP_PING_CONSTR times.
   */
  if (((temp_conf.lsp_ping_intvl * 1000) < 
        (BFD_MPLS_MIN_LSP_PING_CONSTR * temp_conf.min_tx)) ||
      ((temp_conf.lsp_ping_intvl * 1000) < 
       (BFD_MPLS_MIN_LSP_PING_CONSTR * temp_conf.min_rx)))
    {
      return NSM_MPLS_BFD_ERR_INVALID_INTVL;
    }

  /* Get the FEC Prefix/Tunnel-Name from input and do a BFD FEC conf list 
   * look up. 
   */
  if (lsp_type == BFD_MPLS_LSP_TYPE_LDP ||
      lsp_type == BFD_MPLS_LSP_TYPE_STATIC)
    {
      if (! str2prefix (input, &temp_conf.fec))
        return NSM_MPLS_BFD_ERR_INVALID_PREFIX;

      bfd_conf = nsm_mpls_bfd_fec_conf_lookup (NSM_MPLS->bfd_fec_conf_list, 
                                               lsp_type, &temp_conf.fec);
    }
  else
    {
      temp_conf.tunnel_name = input;

      bfd_conf = nsm_mpls_bfd_fec_conf_lookup (NSM_MPLS->bfd_fec_conf_list,
                                               lsp_type, input);
    }

  /* If BFD conf entry already exists & BFD is enabled for this FEC then 
   * check if the new attributes are different from existing entry or 
   * FEC conf cflag was set with BFD_DISABLE. If yes, 
   * update the attributes and send BFD session Add.
   * else return error.
   */
  if (bfd_conf != NULL)
    {
      ret = nsm_mpls_bfd_fec_conf_update (nm, bfd_conf, &temp_conf);
      if (ret < 0)
        return ret;
    }
  /* Else, create a new BFD conf entry and send BFD session add, 
   * if the coresponding FTN is installed. 
   */
  else
    {
      bfd_conf = nsm_mpls_bfd_fec_conf_new (NSM_MPLS->bfd_fec_conf_list, 
                                            &temp_conf);
      if (! bfd_conf)
        return NSM_ERR_MEM_ALLOC_FAILURE;

      return nsm_mpls_bfd_fec_conf_session_add (nm, bfd_conf, PAL_FALSE);
    }

  return NSM_SUCCESS;
}

/**@brief - This function disables the BFD fall-over check for the FEC/Trunk 
 *  of an LSP-type.

 *  @param vr_id - vr id of the VR in context.
 *  @param *lsp_name - LSP Type Name : LDP/RSVP/Static. 
 *  @param *input - FEC Prefix/Trunk name.

 *  @return Error code (< 0) or success (0)
 */
int
nsm_mpls_bfd_api_fec_disable_set (u_int32_t vr_id, char *lsp_name, char *input)
{
  struct nsm_master *nm = NULL;
  struct nsm_mpls_bfd_fec_conf *bfd_conf = NULL;
  struct nsm_mpls_bfd_fec_conf temp_conf;
  u_int16_t lsp_type;

  nm = nsm_master_lookup_by_id (nzg, vr_id);
  if (nm == NULL)
    return NSM_API_SET_ERR_VR_NOT_EXIST;

  if ((! NSM_MPLS) || (! NSM_MPLS->bfd_fec_conf_list))
    return NSM_FAILURE;

  lsp_type = nsm_mpls_bfd_lsp_name2type (lsp_name);
  if (lsp_type == BFD_MPLS_LSP_TYPE_UNKNOWN)
    return NSM_MPLS_BFD_ERR_LSP_UNKNOWN;

  pal_mem_set (&temp_conf, 0, sizeof (struct nsm_mpls_bfd_fec_conf));

  /* Copy the received input/default attributes into the temp conf. */
  temp_conf.lsp_type = lsp_type;
  temp_conf.tunnel_name = NULL;

  /* Get the FEC Prefix/Tunnel-Name from input and do a BFD FEC conf list 
   * look up. 
   */
  if (lsp_type == BFD_MPLS_LSP_TYPE_LDP ||
      lsp_type == BFD_MPLS_LSP_TYPE_STATIC)
    {
      if (! str2prefix (input, &temp_conf.fec))
        return NSM_MPLS_BFD_ERR_INVALID_PREFIX;

      bfd_conf = nsm_mpls_bfd_fec_conf_lookup (NSM_MPLS->bfd_fec_conf_list, 
                                               lsp_type, &temp_conf.fec);
    }
  else
    {
      temp_conf.tunnel_name = input;

      bfd_conf = nsm_mpls_bfd_fec_conf_lookup (NSM_MPLS->bfd_fec_conf_list,
                                               lsp_type, input);
    }

  /* If BFD conf entry already exists & bfd flag is not set to BFD_DISABLE 
   * send BFD session delete for the entry and set the flags to BFD_DISABLE. 
   */
  if (bfd_conf != NULL)
    {
      if (CHECK_FLAG (bfd_conf->cflag, NSM_MPLS_BFD_FEC_DISABLE))
        return NSM_MPLS_BFD_ERR_ENTRY_EXISTS;
    }
  /* Else, create a new BFD conf entry and Set the BFD_DISABLE flag for it. 
   */
  else
    {
      bfd_conf = nsm_mpls_bfd_fec_conf_new (NSM_MPLS->bfd_fec_conf_list, 
                                            &temp_conf);
      if (! bfd_conf)
        return NSM_ERR_MEM_ALLOC_FAILURE;
    }

  return nsm_mpls_bfd_fec_disable (nm, bfd_conf);
}

/**@brief - This function sets the BFD fall-over check for all the FECs/Trunks 
 *  of an LSP-type.

 *  @param vr_id - vr id of the VR in context.
 *  @param *lsp_name - LSP Type Name : LDP/RSVP/Static. 
 *  @param lsp_ping_intvl - LSP Ping Interval in seconds.
 *  @param min_tx - Minimum BFD Tx in milliseconds.
 *  @param min_rx - Minimum BFD Rx in milliseconds.
 *  @param mult - BFD Detection Multiplier value.

 *  @return Error code (< 0) or success (0)
 */
int
nsm_mpls_bfd_api_lsp_all_set (u_int32_t vr_id, char *lsp_name,
                              u_int32_t lsp_ping_intvl, u_int32_t min_tx,
                              u_int32_t min_rx, u_int32_t mult,
                              bool_t force_explicit_null)
{
  struct nsm_master *nm = NULL;
  struct nsm_mpls_bfd_lsp_conf temp_conf;
  u_int16_t lsp_type;
  int ret;

  nm = nsm_master_lookup_by_id (nzg, vr_id);
  if (nm == NULL)
    return NSM_API_SET_ERR_VR_NOT_EXIST;

  if (! NSM_MPLS)
    return NSM_FAILURE;

  lsp_type = nsm_mpls_bfd_lsp_name2type (lsp_name);
  if (lsp_type == BFD_MPLS_LSP_TYPE_UNKNOWN)
    return NSM_MPLS_BFD_ERR_LSP_UNKNOWN;

  /* Copy the received input/default attributes into the temp conf. */
  temp_conf.lsp_ping_intvl = lsp_ping_intvl;
  temp_conf.min_tx = min_tx;
  temp_conf.min_rx = min_rx;
  temp_conf.mult = mult;
  temp_conf.force_explicit_null = force_explicit_null;

  /* LSP Ping interval should be greater than BFD min Tx & min Rx intervals by
   * atleast BFD_MPLS_MIN_LSP_PING_CONSTR times.
   */
  if (((temp_conf.lsp_ping_intvl * 1000) < 
        (BFD_MPLS_MIN_LSP_PING_CONSTR * temp_conf.min_tx)) ||
      ((temp_conf.lsp_ping_intvl * 1000) < 
       (BFD_MPLS_MIN_LSP_PING_CONSTR * temp_conf.min_rx)))
    {
      return NSM_MPLS_BFD_ERR_INVALID_INTVL;
    }

  /* If BFD has already been configured for this LSP-Type, 
   * check if its attributes have been changed. If yes, 
   * update the attributes and send BFD session updates 
   * else return error.
   */
  if (CHECK_FLAG (NSM_MPLS->bfd_flag, nsm_mpls_bfd_lsp_type2flag (lsp_type)))
    {
      ret = nsm_mpls_bfd_lsp_conf_update (nm, &temp_conf, lsp_type);
      if (ret < 0)
        return ret;
    }

  /* Send BFD session adds for all installed FTNs of this LSP-type. */ 
  NSM_MPLS->bfd_lsp_conf [lsp_type].lsp_ping_intvl = 
    temp_conf.lsp_ping_intvl;
  NSM_MPLS->bfd_lsp_conf [lsp_type].min_tx = temp_conf.min_tx;
  NSM_MPLS->bfd_lsp_conf [lsp_type].min_rx = temp_conf.min_rx;
  NSM_MPLS->bfd_lsp_conf [lsp_type].mult = temp_conf.mult;
  NSM_MPLS->bfd_lsp_conf [lsp_type].force_explicit_null = 
    temp_conf.force_explicit_null;

  SET_FLAG (NSM_MPLS->bfd_flag, nsm_mpls_bfd_lsp_type2flag (lsp_type));

  return nsm_mpls_bfd_lsp_session_add (nm, lsp_type);
}

/**@brief - This function Unsets the BFD fall-over check for the FEC/Trunk 
 *  of an LSP-type.

 *  @param vr_id - vr id of the VR in context.
 *  @param *lsp_name - LSP Type Name : LDP/RSVP/Static. 
 *  @param *input - FEC Prefix/Trunk name.

 *  @return Error code (< 0) or success (0)
 */
int
nsm_mpls_bfd_api_fec_unset (u_int32_t vr_id, char *lsp_name,  char *input)
{
  struct nsm_master *nm = NULL;
  struct nsm_mpls_bfd_fec_conf *bfd_conf = NULL;
  struct prefix pfx;
  u_int16_t lsp_type;

  nm = nsm_master_lookup_by_id (nzg, vr_id);
  if (nm == NULL)
    return NSM_API_SET_ERR_VR_NOT_EXIST;

  if ((! NSM_MPLS) || (! NSM_MPLS->bfd_fec_conf_list))
    return NSM_FAILURE;

  lsp_type = nsm_mpls_bfd_lsp_name2type (lsp_name);
  if (lsp_type == BFD_MPLS_LSP_TYPE_UNKNOWN)
    return NSM_MPLS_BFD_ERR_LSP_UNKNOWN;

  /* Get the FEC Prefix/Tunnel-Name from input and do a BFD FEC conf list 
   * look up. 
   */
  if (lsp_type == BFD_MPLS_LSP_TYPE_LDP ||
      lsp_type == BFD_MPLS_LSP_TYPE_STATIC)
    {
      if (! str2prefix (input, &pfx))
        return NSM_FAILURE;

      bfd_conf = nsm_mpls_bfd_fec_conf_lookup (NSM_MPLS->bfd_fec_conf_list, 
                                               lsp_type, &pfx);
    }
  else
    {
      bfd_conf = nsm_mpls_bfd_fec_conf_lookup (NSM_MPLS->bfd_fec_conf_list,
                                               lsp_type, input);
    }

  if (bfd_conf != NULL)
    {
      return nsm_mpls_bfd_fec_conf_session_delete (nm, bfd_conf);
    }
  else
    return NSM_MPLS_BFD_ERR_ENTRY_NOT_FOUND;
}

/**@brief - This function Unsets the BFD fall-over check for all the 
 *  FECs/Trunks of an LSP-type.

 *  @param vr_id - vr id of the VR in context.
 *  @param *lsp_name - LSP Type Name : LDP/RSVP/Static. 

 *  @return Error code (< 0) or success (0)
 */
int
nsm_mpls_bfd_api_lsp_all_unset (u_int32_t vr_id, char *lsp_name)
{
  struct nsm_master *nm = NULL;
  u_int16_t lsp_type;
  int ret = NSM_SUCCESS;

  nm = nsm_master_lookup_by_id (nzg, vr_id);
  if (nm == NULL)
    return NSM_API_SET_ERR_VR_NOT_EXIST;

  if (! NSM_MPLS)
    return NSM_FAILURE;

  lsp_type = nsm_mpls_bfd_lsp_name2type (lsp_name);
  if (lsp_type == BFD_MPLS_LSP_TYPE_UNKNOWN)
    return NSM_MPLS_BFD_ERR_LSP_UNKNOWN;

  /* If BFD has been configured for this LSP-type, send BFD session deletes
   * for all installed FTNs of the LSP-Type.
   */
  if (CHECK_FLAG (NSM_MPLS->bfd_flag, nsm_mpls_bfd_lsp_type2flag (lsp_type)))
    {
      ret = nsm_mpls_bfd_lsp_session_delete (nm, lsp_type);

      /* Setting the values back to defaults. */
      NSM_MPLS->bfd_lsp_conf [lsp_type].lsp_ping_intvl =
        BFD_MPLS_LSP_PING_INTERVAL_DEF;
      NSM_MPLS->bfd_lsp_conf [lsp_type].min_tx = BFD_MPLS_MIN_TX_INTERVAL_DEF;
      NSM_MPLS->bfd_lsp_conf [lsp_type].min_rx = BFD_MPLS_MIN_RX_INTERVAL_DEF;
      NSM_MPLS->bfd_lsp_conf [lsp_type].mult = BFD_MPLS_DETECT_MULT_DEF;

      UNSET_FLAG (NSM_MPLS->bfd_flag, nsm_mpls_bfd_lsp_type2flag (lsp_type));
    }

  return ret;
}

#ifdef HAVE_VCCV
int
nsm_mpls_bfd_api_vccv_trigger (struct nsm_master *nm, u_int32_t vc_id, u_char op)
{
  struct nsm_mpls_circuit *vc = NULL;
  int ret = 0;
  u_int8_t bfd_cvtype_inuse = CV_TYPE_NONE;
   
  vc = nsm_mpls_l2_circuit_lookup_by_id (nm, vc_id);

  if (! vc || !vc->vc_fib)
      return NSM_ERR_VC_ID_NOT_FOUND;

  switch (op)
    {
    case BFD_VCCV_OP_START:
      if (vc->is_bfd_running)
        return NSM_MPLS_BFD_VCCV_ERR_SESS_EXISTS;

      bfd_cvtype_inuse = oam_util_get_bfd_cvtype_in_use(vc->cv_types, 
                                                   vc->vc_fib->remote_cv_types);
      if (bfd_cvtype_inuse != CV_TYPE_NONE)
        ret = nsm_mpls_vc_bfd_session_add (nm, vc);
      else
        return NSM_MPLS_BFD_VCCV_ERR_NOT_CONFIGURED;
      break;

    case BFD_VCCV_OP_STOP:
      if (!vc->is_bfd_running)
        return NSM_MPLS_BFD_VCCV_ERR_SESS_NOT_EXISTS;
      else
        ret = nsm_mpls_vc_bfd_session_delete (nm, vc, PAL_TRUE);
      break;

    default:
      return NSM_FAILURE;
    };

  if (ret != NSM_SUCCESS)
    return NSM_FAILURE;

  return NSM_SUCCESS;
}    
#endif /* HAVE_VCCV */
#endif /* HAVE_MPLS_OAM  && HAVE_BFD */
