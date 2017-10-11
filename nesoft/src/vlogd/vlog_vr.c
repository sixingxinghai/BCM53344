/*=============================================================================
**
** Copyright (C) 2008-2009   All Rights Reserved.
**
** vlog_vr.c - Implementation of the VLOG_VR and VLOG_VRS (container of VRs)
**             objects.
*/

#include "pal.h"
#include "lib.h"
#include "linklist.h"
#include "log.h"
#include "imi_client.h"
#include "vlogd.h"

extern VLOG_GLOBALS *vlog_vgb;

/*--------------------------------------------------------------
 *                    THE VLOG_VRS CONTAINER
 *--------------------------------------------------------------
 * NOTE: We use the lib_globals VR container but the code does
 *       not depend on any of its implementation details.
 *----------------------------------------------------------------
 * VLOG_VR_WALK_USER_PARMS - Control block to hold user callback and
 *                           the user ref when walking through the
 *                           lib_globals VRs container.
 */
typedef struct vlog_vr_walk_user_parms
{
  VLOG_VRS_WALK_CB user_cb;
  intptr_t         user_ref;
} VLOG_VR_WALK_USER_PARMS;

/*----------------------------------------------------------------
 * _vlog_vr_walk_cb - Generic callback function to be passed to
 *                    the apn_vrs_walk_and_exec. When executed it decouples
 *                    the user parameters: callback and the reference
 *                    and executes teh callback.
 *                    It provides also mapping betwen apn_vr and VLOG_VR.
 */
static ZRESULT
_vlog_vr_walk_cb(struct apn_vr *vr, intptr_t ref)
{
  VLOG_VR *vvr = LIB_VR_GET_PROTO_VR(vr);

  VLOG_VR_WALK_USER_PARMS *user_parms = (VLOG_VR_WALK_USER_PARMS *)ref;

  if (vvr)
    if (user_parms && user_parms->user_cb)
      return user_parms->user_cb(vvr, user_parms->user_ref);

  return ZRES_OK;
}

/*----------------------------------------------------------------
 * vlog_vrs_walk - Walk all the VRs and execute given callback.
 *                 Walking goes through the lib_globals VR container
 *                 but we would not refer any of its internals.
 */
ZRESULT
vlog_vrs_walk (VLOG_VRS_WALK_CB user_cb, intptr_t user_ref)
{
  struct lib_globals *zg = vlog_vgb->vgb_lgb;
  VLOG_VR_WALK_USER_PARMS user_parms;

  if (! user_cb || ! zg)
    return ZRES_ERR;

  /* Encapsulate user parameters: callback and reference to push it through
     the apn_vr walk function.
   */
  user_parms.user_cb  = user_cb;
  user_parms.user_ref = user_ref;

  apn_vrs_walk_and_exec(zg, _vlog_vr_walk_cb, (intptr_t)&user_parms);

  return ZRES_OK;
}

/*----------------------------------------------------------------
 * vlog_vrs_delete - Delete all VLOG_VR instances.
 */
static ZRESULT
_vlog_vr_delete_cb(VLOG_VR *vvr, intptr_t dummy)
{
  vlog_vr_delete(vvr);
  return ZRES_OK;
}

void
vlog_vrs_delete()
{
  vlog_vrs_walk(_vlog_vr_delete_cb, 0);
}

/*----------------------------------------------------------------
 * vlog_vrs_lookup_vr - Find VLOG_VR instance given by VR id.
 */
VLOG_VR *
vlog_vrs_lookup_vr(u_int32_t vr_id)
{
  struct apn_vr *ivr;

  ivr = apn_vr_get_by_id (vlog_vgb->vgb_lgb, vr_id);
  if (ivr)
    return ivr->proto;
  return NULL;
}

/*--------------------------------------------------------------
 *                    VLOG_VR OBJECT
 *--------------------------------------------------------------
 * vlog_vr_new - Constructor of the VLOG_VR object
 */
VLOG_VR *
vlog_vr_new(char *vr_name, u_int32_t vr_id)
{
  struct apn_vr *vr;
  VLOG_VR *vvr;
  u_int16_t name_len= (vr_name != NULL) ? pal_strlen(vr_name) : 0;
  struct lib_globals *zg = vlog_vgb->vgb_lgb;

  /* Create a VR with first better id. */
  vr = apn_vr_get_by_name(zg, vr_name);
  if (vr == NULL)
    return NULL;

  /* Delete any duplicate VR, if any.  */
  vr = apn_vr_update_by_name (zg, vr_name, vr_id);
  if (vr == NULL)
    return NULL;

  /* Check we managed what we wanted. */
  vr = apn_vr_get_by_id (zg, vr_id);
  if (vr == NULL)
    return NULL;

  /* Create VLOG_VR only if it does not exist.
   */
  if (! (vvr = vr->proto))
    {
      /* Create vlog_vr */
      vvr = XCALLOC(MTYPE_VLOG_VR, sizeof (VLOG_VR));
      if (vvr == NULL)
        return NULL;

      vvr->vvr_ivr      = vr;
      vvr->vvr_name_len = name_len;
      vvr->vvr_terms    = vlog_terms_new();

      /* Backward reference to VLOG_VR. */
      vr->proto = vvr;

      /* Read the configuration.  */
      if (vr_id != 0) {
        HOST_CONFIG_VR_START (vr);
      }
    }
  return vvr;
}

/*------------------------------------------------------------------
 * vlog_vr_delete - Deletion of VR
 *
 * Purpose:
 *    To delete completely the VLOG_VR instance.
 *    Decrementing reference counts in all attached VLOG_TERMs.
 *    Closing VLOG_TERM if ref count reaches zero.
 *    Closing VR log file.
 *
 */
void
vlog_vr_delete(VLOG_VR *vvr)
{
  struct apn_vr *ivr;

  if (vvr)
    {
    /* Save the apn_vr pointer. */
      ivr = vvr->vvr_ivr;

      vlog_vr_proto_delete(vvr);

      if (ivr)
        apn_vr_delete(ivr);
    }
}

/*------------------------------------------------------------------
 * vlog_vr_proto_delete - Delete protocol part of the VR
 *
 * Purpose:
 *    To be called from inside the lib at the time of VR update by name.
 *    We must remove the application part - the lib will remove the apn_vr.
 */
void
vlog_vr_proto_delete(VLOG_VR *vvr)
{
  struct apn_vr *ivr;

  if (vvr == NULL)
    return;

  /* Update all the terminal reference numbers.
   * Close terminals if not longer referenced.
   */
  vlog_terms_walk(vvr->vvr_terms,
                  (VLOG_TERMS_WALK_CB)vlog_term_exclude_vr,
                  (intptr_t)vvr);

  /* Closing the VR log file. */
  vlog_vr_close_log_file(vvr);

  ivr = vvr->vvr_ivr;
  if (ivr)
    ivr->proto = NULL;

  XFREE(MTYPE_VLOG_VR, vvr);
}


/*------------------------------------------------------------------
 * vlog_vr_bind_term - Binding a terminal to VLOG_VR.
 *
 * Params:
 *    vvr   - Ptr to vlog_vr object
 *    term  - Ptr to terminal name or encoded socket number
 *
 *------------------------------------------------------------------
 */
ZRESULT
vlog_vr_bind_term(VLOG_VR *vvr, VLOG_TERM *term)
{
  /* Check it is not duplicate addition. */

  if (vlog_terms_lookup(vvr->vvr_terms, term->vtm_name) != NULL) {
    return ZRES_OK;
  }
  if (vlog_terms_link_term(vvr->vvr_terms, term) != ZRES_OK)
    return ZRES_ERR;

  vlog_term_include_vr(term, vvr);
  return ZRES_OK;
}

/*------------------------------------------------------------------
 * vlog_vr_unbind_term - Unbinding terminal from VLOG_VR.
 *
 * Params:
 *    vvr  - Ptr to VLOG_VR object.
 *    term - Ptr to VLOG_TERM object.
 *------------------------------------------------------------------
 */
ZRESULT
vlog_vr_unbind_term(VLOG_VR *vvr, VLOG_TERM *term)
{
  /* Get the terminal object. */
  if (vlog_terms_lookup(vvr->vvr_terms, term->vtm_name) == NULL) {
    return ZRES_OK;
  }
  /* Delete terminal from the VR distribution list container. */
  vlog_terms_unlink_term(vvr->vvr_terms, term);

  /* Remove one more VR from the terminal context.
     Return ZRES_OK or ZRES_NO_MORE if not attached to another VR.
     This would break "walk" function.
  */
  return vlog_term_exclude_vr(term, vvr);
}

/*------------------------------------------------------------------
 * vlog_vr_forward_log_msg - Forwarding log message to attached terminals
 *         and the log file.
 *
 * Params:
 *    vvr        - Ptr to VLOG_VR object. This is the message originating VR.
 *    clt_mod_id - The module id of the sending VLOG client
 *    clt_proc_id- The process id of the sending VLOG client
 *    priority   - Priority (not used at present)
 *    in_msg     - The log message itself
 *------------------------------------------------------------------
 */
ZRESULT
vlog_vr_forward_log_msg(VLOG_VR      *vvr,
                        module_id_t  clt_mod_id,
                        u_int32_t    clt_proc_id,
                        s_int8_t     priority,
                        char        *in_msg)

{
  static char out_msg[VLOG_LOG_MSG_MAX_SIZE];

  struct apn_vr *ivr;
  struct pal_tm exptime;
  pal_time_t curtime;
  char *mod_name = modname_strl (clt_mod_id);
  char time_str[VLOG_TIME_STRING_LENGTH];
  VLOG_TERM_REQ wr_req;
  s_int16_t out_msg_len;

  if (vvr == NULL)
    return ZRES_ERR;

  ivr = vvr->vvr_ivr;

  /* Build the message */
  curtime = pal_time_sys_current (NULL);
  pal_time_loc (&curtime, &exptime);
  pal_time_strf (time_str, VLOG_TIME_STRING_LENGTH, "%Y/%m/%d %H:%M:%S", &exptime);

  if (priority == -1)
    {
      out_msg_len = pal_snprintf (out_msg,
                                  VLOG_LOG_MSG_MAX_SIZE,
                                  "%-6s: %s %s[%5d]: %s\r\n",
                                  (ivr->name==NULL) ? "<pvr>" : ivr->name,
                                  time_str,
                                  mod_name,
                                  clt_proc_id,
                                  in_msg);
    }
  else
    {
      out_msg_len = pal_snprintf (out_msg,
                                  VLOG_LOG_MSG_MAX_SIZE,
                                  "%-6s: %s %s: %s[%5d]: %s\r\n",
                                  (ivr->name==NULL) ? "<pvr>" : ivr->name,
                                  time_str,
                                  zlog_get_priority_str(priority),
                                  mod_name,
                                  clt_proc_id,
                                  in_msg);
    }
  /* Create write request for VLOG_TERM. */
  wr_req.vtr_orig_vrid = LIB_VR_GET_VR_ID(ivr);
  wr_req.vtr_orig_vrname_len = 10;
  wr_req.vtr_tot_msg_len = out_msg_len;
  wr_req.vtr_msg = out_msg;

  /* Forward the request to this VLOG_VR attached terminals. */
  vlog_terms_write (vvr->vvr_terms,  &wr_req);

  /* Write the message to the originating VLOG_VR's log file. */
  vlog_vr_write_log_file(vvr, &wr_req);

  /* If the originating VLOG_VR is not PVR, write the msg also to
     the PVR log file (if enabled).
   */
  if (! IS_APN_VR_PRIVILEGED(ivr))
  {
    vvr = vlog_vrs_lookup_vr(0);
    vlog_vr_write_log_file(vvr, &wr_req);
  }
  return ZRES_OK;
}

/*------------------------------------------------------------------
 * vlog_vr_open_log_file - Combo function t open any possible log file.
 *
 */
ZRESULT
vlog_vr_open_log_file(VLOG_VR *vvr,
                      VLOG_FNAME_TYPE file_name_type,
                      char *fname)
{
  char tmppath[256];
  char *fullpath;
  PAL_VLOG_FILE *pvf;
  ZRESULT res;

  /* Close the current log file, if open. */
  if (vvr->vvr_pvf)
      vlog_vr_close_log_file(vvr);

  pvf = XCALLOC(MTYPE_VLOG_PAL, sizeof(PAL_VLOG_FILE));
  if (pvf == NULL)
    return VLOG_ZRES_OUT_OF_MEM_ERR;

  switch (file_name_type)
  {
    case VLOG_FNAME_PVR_DEFAULT:
      pal_sprintf(tmppath, "%s/%s/%s_%s",
                  VLOG_PACOS_LOG_DIR,
                  VLOG_PVR_NAME,
                  VLOG_PVR_NAME,
                  VLOG_LOG_WORD);
      break;
    case VLOG_FNAME_PVR_LOCAL:
      pal_sprintf(tmppath, "%s/%s/%s",
                  VLOG_PACOS_LOG_DIR,
                  VLOG_PVR_NAME,
                  fname);
      break;
    case VLOG_FNAME_PVR_GLOBAL:
      pal_sprintf(tmppath, "%s", fname);
      break;
    case VLOG_FNAME_VR_DEFAULT:
      pal_sprintf(tmppath, "%s/%s/%s-%s",
                  VLOG_PACOS_LOG_DIR,
                  LIB_VR_GET_VR_NAME(vvr->vvr_ivr),
                  LIB_VR_GET_VR_NAME(vvr->vvr_ivr),
                  VLOG_LOG_WORD);
      break;
    case VLOG_FNAME_VR_LOCAL:
      pal_sprintf(tmppath, "%s/%s/%s",
                  VLOG_PACOS_LOG_DIR,
                  LIB_VR_GET_VR_NAME(vvr->vvr_ivr),
                  fname);
      break;
    default:
      break;
  }
  fullpath = XSTRDUP(MTYPE_VLOG_FNAME, tmppath);

  res = pal_vlog_open_file(pvf, fullpath, VLOG_DEF_FILE_SIZE);
  if (res != ZRES_OK)
  {
    XFREE(MTYPE_VLOG_PAL, pvf);
    XFREE(MTYPE_VLOG_FNAME, fullpath);
    return VLOG_ZRES_FILE_OPEN_ERR;
  }
  /* For show running config purpose we need to store the file in the
     format given by the user.
   */
  if (fname)
    pvf->pvf_cfg_fname = XSTRDUP(MTYPE_VLOG_FNAME, fname);
  else
    pvf->pvf_cfg_fname = XSTRDUP(MTYPE_VLOG_FNAME, "");

  if (pvf->pvf_cfg_fname)
    vvr->vvr_pvf = pvf;
  else
    return ZRES_ERR;

  return ZRES_OK;
}

/*------------------------------------------------------------------
 * vlog_vr_close_log_file - Close the VLOG_VR log file.
 */
ZRESULT
vlog_vr_close_log_file(VLOG_VR *vvr)
{
  ZRESULT res;
  PAL_VLOG_FILE *pvf;

  if (vvr == NULL || (pvf = vvr->vvr_pvf) == NULL)
    return ZRES_OK;

  res = pal_vlog_close_file(vvr->vvr_pvf);

  XFREE(MTYPE_VLOG_FNAME, pvf->pvf_full_path);
  XFREE(MTYPE_VLOG_FNAME, pvf->pvf_cfg_fname);
  XFREE(MTYPE_VLOG_PAL, pvf);
  vvr->vvr_pvf = NULL;
  return ZRES_OK;
}

/*------------------------------------------------------------------
 * vlog_vr_write_log_file - Write to VLOG_VR log file.
 */
ZRESULT
vlog_vr_write_log_file(VLOG_VR *vvr, VLOG_TERM_REQ *wr_req)
{
  char *log_msg = NULL;
  int   msg_offs=0;

  if (vvr == NULL || vvr->vvr_pvf == NULL)
    return ZRES_OK;

  if (! IS_APN_VR_PRIVILEGED(vvr->vvr_ivr))
  {
    if (LIB_VR_GET_VR_ID(vvr->vvr_ivr) != wr_req->vtr_orig_vrid)
      return ZRES_OK;
    else
      /* Writing to the non-PVR log file. Remove VR name. */
      msg_offs = wr_req->vtr_orig_vrname_len;
  }
  /* Remove the \r\n. */
  log_msg = wr_req->vtr_msg;
  log_msg[wr_req->vtr_tot_msg_len-2] = 0;
  log_msg[wr_req->vtr_tot_msg_len-1] = 0;

  pal_vlog_write_file(vvr->vvr_pvf,
                      &wr_req->vtr_msg[msg_offs],
                      wr_req->vtr_tot_msg_len-2-msg_offs);

  log_msg[wr_req->vtr_tot_msg_len-2] = '\r';
  log_msg[wr_req->vtr_tot_msg_len-1] = '\n';

  return ZRES_OK;
}

/*------------------------------------------------------------------
 * vlog_vr_get_log_file - Return VLOG_VR log file name.
 */
ZRESULT
vlog_vr_get_log_file(VLOG_VR *vvr, char **pfname)
{
  PAL_VLOG_FILE *pvf;

  if (vvr == NULL || (pvf = vvr->vvr_pvf) == NULL)
    return ZRES_ERR;

  *pfname = pvf->pvf_cfg_fname;
  return ZRES_OK;
}

ZRESULT
vlog_vr_reset_log_file(VLOG_VR *vvr)
{
  if (vvr->vvr_pvf == NULL)
    return ZRES_ERR;

  return pal_vlog_shift_files(vvr->vvr_pvf);
}

