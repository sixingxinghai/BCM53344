/*=============================================================================
**
** Copyright (C) 2008-2009   All Rights Reserved.
**
** vlog_api.c - Implementation of API functions.
*/

#include "pal.h"
#include "lib.h"
#include "vrep.h"
#include "vlogd.h"


extern VLOG_GLOBALS *vlog_vgb;

/*--------------------------------------------------------------------
 *                       V R' S
 *--------------------------------------------------------------------
 * vlog_add_vr - Addition of VR.
 *
 * Params:
 *   vr_name  - VR name
 *   vr_id    - VR id
 */
ZRESULT
vlog_add_vr(char *vr_name, u_int32_t vr_id)
{
  VLOG_VR *vvr;

  vvr =  vlog_vr_new(vr_name, vr_id);
  if (vvr == NULL)
    return ZRES_ERR;

  /* Walk all VLOG_TERMs and add a term to the new VR distribution list
     if the terminal user required debug output from "all" VRs.
   */
  vlog_terms_walk(vlog_vgb->vgb_terms,
                  (VLOG_TERMS_WALK_CB)vlog_term_bind_all_vr,
                  (intptr_t)vvr);
  return ZRES_OK;
}

/*------------------------------------------------------------------------
 * vlog_del_vr - Deletion of VR.
 *
 * Params:
 *   vr_name  - VR name
 *   vr_id    - VR id
 */
void
vlog_del_vr(char *vr_name, u_int32_t vr_id)
{
  struct apn_vr *vr;

  /* Must delete all terminals on the distribution list. */
  vr = apn_vr_lookup_by_name(vlog_vgb->vgb_lgb, vr_name);
  if (vr != NULL)
  {
    vlog_vr_delete(vr->proto);
  }
}

/*--------------------------------------------------------------------
 *                   T E R M I N A L S
 *--------------------------------------------------------------------
 * vlog_add_term_to_vr - Add terminal destination to
 *                       VR distribution list.
 * Params:
 *   vr_id     - Id of VR to which user is connected.
 *   term_name - TTY name or encoded socket number of the user terminal
 *   vr_name   - NULL if adding this VR only
 *               != NULL: PVR usage only: addition of specific VR or "all" VRs
 */
ZRESULT
vlog_add_term_to_vr (u_int32_t vr_id,
                     char     *term_name,
                     char     *vr_name)
{
  struct apn_vr  *user_vr, *mon_vr;
  struct vlog_vr *user_vvr, *mon_vvr;
  bool_t          all_vrs = PAL_FALSE;
  VLOG_TERM      *term;
  ZRESULT         res = ZRES_ERR;

  if (vr_name)
    all_vrs = pal_strcmp(vr_name, "all") == 0;

  /* Exclude the case when non-PVR selects "all" or another VR. */
  if (vr_id != 0)
    if (vr_name != NULL)
      return ZRES_ERR;

  /* Check the term object is instantiated. */
  term = vlog_terms_lookup(vlog_vgb->vgb_terms, term_name);

  /* If VR context has changed, delete the terminal. */
  if (term != NULL && term->vtm_vr_id != vr_id)
    {
      vlog_term_delete(term);
      term = NULL;
    }

  if (term == NULL)
    {
      /* Create the terminal object. */
      term = vlog_term_new(term_name, vr_id);
      if (term == NULL)
        return ZRES_ERR;
    }
  /* VLOG_TERM instantiated - add it to the VR distribution list. */
  term->vtm_shutdown = PAL_FALSE;

  /* Find APN_VR object. */
  user_vr = apn_vr_lookup_by_id(vlog_vgb->vgb_lgb, vr_id);
  if (user_vr == NULL)
    goto vlog_add_term_to_vr_exit;

  user_vvr = user_vr->proto;

  if (vr_name == NULL)
    {
      /* Simple "terminal monitor" without any parameters.
         Binding term to the user VR.
       */
      if (vlog_vr_bind_term(user_vvr, term) != ZRES_OK)
        goto vlog_add_term_to_vr_exit;
    }
  else if (all_vrs)
    {
      /* PVR wants to view debugging from all VRs.
         Binding term to any active VR.
         It will also be bound to any VR that will be instantiated later.
       */
      term->vtm_all_vrs = PAL_TRUE;
      vlog_vrs_walk((VLOG_VRS_WALK_CB)vlog_vr_bind_term, (intptr_t)term);
    }
  else
    {
      /* PVR wants to view specific VR output.
         Binding term to this VR output.
      */
      mon_vr = apn_vr_lookup_by_name(vlog_vgb->vgb_lgb, vr_name);
      if (mon_vr == NULL)
        goto vlog_add_term_to_vr_exit;

      mon_vvr = mon_vr->proto;
      if (vlog_vr_bind_term(mon_vvr, term) != ZRES_OK)
        goto vlog_add_term_to_vr_exit;
    }
  res = ZRES_OK;

vlog_add_term_to_vr_exit:
  if (res != ZRES_OK)
    {
      vlog_term_del_unused(term);
      return ZRES_ERR;
    }
  else
    return ZRES_OK;
}

/*-------------------------------------------------------------------------
 * vlog_del_term_from_vr - Delete terminal destination from
 *                         VR distribution list.
 * Params:
 *   vr_id     - VR id in the context of terminal
 *   term_name - TTY name or encoded socket number of the user terminal
 *   vr_name   - NULL if adding this VR only
 *               != NULL PVR usage only: deletion of specific VR or "all" VRs
 *
 */
ZRESULT
vlog_del_term_from_vr (u_int32_t vr_id,
                       char     *term_name,
                       char     *vr_name)
{
  struct apn_vr  *user_vr, *mon_vr;
  struct vlog_vr *user_vvr, *mon_vvr;
  bool_t         all_vrs = PAL_FALSE;
  VLOG_TERM     *term;

  if (vr_name != NULL)
    {
      all_vrs = pal_strcmp(vr_name, "all") == 0;
    }
  /* Exclude the case when non-PVR selects "all" or another VR. */
  if (vr_id != 0)
    if (vr_name != NULL)
      return ZRES_ERR;

  /* Check the term object is instantiated. */
  term = vlog_terms_lookup(vlog_vgb->vgb_terms, term_name);
  if (term == NULL)
    /* Not instantiated means already detached. */
    return ZRES_OK;

  term->vtm_shutdown = PAL_FALSE;

  /* vlog_term instantiated - remove it from the VR distribution list. */

  /* Find apn_vr object. */
  user_vr = apn_vr_lookup_by_id(vlog_vgb->vgb_lgb, vr_id);
  if (user_vr == NULL)
    return ZRES_ERR;

  user_vvr = user_vr->proto;

  if (vr_name == NULL && vr_id != 0 )
    {
      /* From non-PVR user: "terminal no monitor"
         This is equal to removal of this terminal.
      */
      vlog_vr_unbind_term(user_vvr, term);
    }
  else if (all_vrs || (vr_name == NULL && vr_id == 0))
    {
      /* From PVR user:
         "terminal no monitor all" or
         "terminal no monitor"
       */
      term->vtm_all_vrs = PAL_FALSE;
      vlog_vrs_walk((VLOG_VRS_WALK_CB)vlog_vr_unbind_term, (intptr_t)term);
    }
  else
    {
      /* Delete this PVR user terminal from a given VR distribution list.
      */
      mon_vr = apn_vr_lookup_by_name(vlog_vgb->vgb_lgb, vr_name);
      if (mon_vr == NULL)
        return ZRES_ERR;

      mon_vvr = mon_vr->proto;
      vlog_vr_unbind_term(mon_vvr, term);
    }
  return ZRES_OK;
}

#ifdef PAL_LOG_FILESYS

/*--------------------------------------------------------------------
 *                      L O G   F I L E S
 *-------------------------------------------------------------------------
 * vlog_set_log_file - Setting a log file for a given VR
 *                     Maximum one log file per VR can be open at any time.
 *
 * Params:
 *   vr_id     - VR id in the context of terminal
 *   fname     - File name
 */
ZRESULT
vlog_set_log_file (u_int32_t vr_id, char *fname)
{
  struct apn_vr *ivr;
  ZRESULT        res = ZRES_ERR;

  /* Find apn_vr object. */
  ivr = apn_vr_lookup_by_id(vlog_vgb->vgb_lgb, vr_id);
  if (ivr == NULL)
    return ZRES_ERR;

  /* For PVR, it might be by global path, local name or default name.*/
  if (vr_id == 0)
    {
      /* PVR log file. */
      if (fname == NULL)
        {
          res = vlog_vr_open_log_file(ivr->proto, VLOG_FNAME_PVR_DEFAULT, NULL);
        }
      else if (fname[0] == '/')
        {
          /* Set global name log file. Create directories if not existent. */
          res = vlog_vr_open_log_file(ivr->proto, VLOG_FNAME_PVR_GLOBAL, fname);
        }
      else
        {
          /* This is a local file name. The file will be open in a default
             directory: /var/local/pacos/log/pvr/
          */
          res = vlog_vr_open_log_file(ivr->proto, VLOG_FNAME_PVR_LOCAL, fname);
        }
    }
  else
    {
      /* File will be stored in the /var/local/pacos/log/<vr-name>/log subdirectory.
       */
      if (fname == NULL)
        {
          /* Open log file with a default name. */
          res = vlog_vr_open_log_file(ivr->proto, VLOG_FNAME_VR_DEFAULT, NULL);
        }
      else if (fname [0] == '/')
        {
          /* Non-PVR log file: A global path is disallowed. */
          res = ZRES_ERR;
        }
      else
        {
          /* Open log file with user selected name. */
          res = vlog_vr_open_log_file(ivr->proto, VLOG_FNAME_VR_LOCAL, fname);
        }
    }
  return res;
}

/*-------------------------------------------------------------------------
 * vlog_unset_log_file - Unsetting a log file for a given VR
 *
 * Params:
 *   vr_id     - VR id in the context of terminal
 *   fname     - File name
 */
ZRESULT
vlog_unset_log_file (u_int32_t vr_id)
{
  struct apn_vr *ivr;

  /* Find apn_vr object. */
  ivr = apn_vr_lookup_by_id(vlog_vgb->vgb_lgb, vr_id);
  if (ivr == NULL) {
    return ZRES_ERR;
  }
  return vlog_vr_close_log_file((VLOG_VR *)ivr->proto);
}

ZRESULT
vlog_get_log_file (u_int32_t vr_id, char **pfname)
{
  struct apn_vr *ivr;

  /* Find apn_vr object. */
  ivr = apn_vr_lookup_by_id(vlog_vgb->vgb_lgb, vr_id);
  if (ivr == NULL) {
    return ZRES_ERR;
  }
  return vlog_vr_get_log_file(ivr->proto, pfname);
}

ZRESULT
vlog_reset_log_file(u_int32_t vr_id)
{
  struct apn_vr *ivr;

  /* Find apn_vr object. */
  ivr = apn_vr_lookup_by_id(vlog_vgb->vgb_lgb, vr_id);
  if (ivr == NULL) {
    return ZRES_ERR;
  }
  return vlog_vr_reset_log_file(ivr->proto);
}

#endif /* PAL_LOG_FILESYS */

/*--------------------------------------------------------------------
 *                   S H O W   F U N C T I O N S
 *--------------------------------------------------------------------
 * Encapsulating of the API show function control parameters into a single
 * VREP reference. We should not use the VREP definitions outside of the
 * vlog_api as it would make the caller aware about the implementation.
 */
typedef struct vlog_show_ref
{
  VLOG_SHOW_ROW_CB vsr_show_row_cb;
  intptr_t         vsr_user_ref;
} VLOG_SHOW_REF;

/*---------------------------------------------------------------------
 * _vlog_vrep_show_cb - Common "show a single report line" callback.
 *                      It will be used by all show API function
 *                      when calling the vrep_show().
 *                      NOTE: The VREP has no idea where the report
 *                            is sent/displayed.
 */
static s_int16_t
_vlog_vrep_show_cb (intptr_t show_ref, char *str)
{
  VLOG_SHOW_REF *user_ref = (VLOG_SHOW_REF *)show_ref;

  return user_ref->vsr_show_row_cb(user_ref->vsr_user_ref, str);
}

/*---------------------------------------------------------------------
 * _vlog_show_term_cb - Adding a terminal entry to the "VLOG terminals"
 *                      report.
 */
static ZRESULT
_vlog_show_term_cb(VLOG_TERM *term, intptr_t rtab)
{
  char    *vr_name;
  VLOG_VR *vvr;
  s_int16_t rc;

  vvr = vlog_vrs_lookup_vr(term->vtm_vr_id);
  if (! vvr)
    return ZRES_ERR;

  vr_name = (LIB_VR_GET_VR_NAME(vvr->vvr_ivr) == NULL) ?
            "<PVR>" : vvr->vvr_ivr->name;

  rc = vrep_add_next_row ((struct vrep_table *)rtab,
                          NULL,
                          "%s \t %s \t %d \t %s \t %s \t %d",
                          (term->vtm_type == VLOG_TERM_TYPE_TTY) ? "tty" : "sock",
                          term->vtm_name,
                          term->vtm_fd,
                          vr_name,
                          (term->vtm_all_vrs) ? "all-vrs" : "---",
                          term->vtm_vr_cnt);
  return (rc == VREP_SUCCESS ? ZRES_OK : ZRES_ERR);
}

/*-----------------------------------------------------------------
 * vlog_show_terminals - API to create a report and return it to the
 *                       caller using the show_row_cb. Note: This function
 *                       is agnostic about the report destination.
 */
ZRESULT
vlog_show_terminals(VLOG_SHOW_ROW_CB show_row_cb, intptr_t user_ref)
{
  struct vrep_table *rtab;
  s_int16_t          rc;
  VLOG_SHOW_REF      show_ref;
  ZRESULT            zres;

  /* Create the VREP object: 6 column report - maximum width 120 characters. */
  rtab = vrep_create (6, 120);
  if (! rtab)
    return ZRES_ERR;

  /* Add the header row. */
  rc = vrep_add_next_row (rtab, NULL,
                          "Type \t Name \t FD \t UserVR \t AllVrs \t VRCnt");
  if (rc != VREP_SUCCESS)
    {
      vrep_delete (rtab);
      return ZRES_ERR;
    }
  /* Walking all terminals and executing _vlog_show_term callback. */
  zres = vlog_terms_walk_with_abort(vlog_vgb->vgb_terms,
                                    (VLOG_TERMS_WALK_WITH_ABORT_CB)_vlog_show_term_cb,
                                    (intptr_t)rtab);
  /* In case of error, we display whatever has been collected.
  */
  /* Prepare control info for _vlog_vrep_show callback. */
  show_ref.vsr_show_row_cb = show_row_cb;
  show_ref.vsr_user_ref    = user_ref;

  /* Sending the report to the caller. */
  rc = vrep_show (rtab, _vlog_vrep_show_cb, (intptr_t)&show_ref);

  /* Destruct the VREP object. */
  vrep_delete (rtab);

  return ((rc == VREP_SUCCESS && zres == ZRES_OK) ? ZRES_OK : ZRES_ERR);
}

/*---------------------------------------------------------------------
 * _vlog_show_vr_cb - Adding the next VR entry to the VLOG VRs report.
 */
static ZRESULT
_vlog_show_vr_cb(VLOG_VR *vr, intptr_t rtab)
{
  PAL_VLOG_FILE *pvf = vr->vvr_pvf;
  char *fname = pvf ? pvf->pvf_full_path : " n/a ";
  char  fsize_str[32];
  s_int16_t rc;

  if (pvf)
    pal_sprintf(fsize_str, "%lu", pvf->pvf_cur_file_size);
  else
    pal_sprintf(fsize_str, " n/a ");

  rc = vrep_add_next_row ((struct vrep_table *)rtab,
                          NULL,
                          "%s \t %d\t %d\t %d\t\%s \t %s",
                          (vr->vvr_ivr->name == NULL) ? "<PVR>" : vr->vvr_ivr->name,
                          vr->vvr_ivr->id,
                          vr->vvr_terms->vts_num_pvr_terms,
                          vr->vvr_terms->vts_num_vr_terms,
                          fname ? fname : "????",
                          fsize_str);
  return (rc == VREP_SUCCESS ? ZRES_OK : ZRES_ERR);
}

/*-----------------------------------------------------------------
 * vlog_show_vrs - API to create the VRs report and return it to the
 *                 caller using the show_row_cb. Note: This function
 *                 is agnostic about the report destination.
 */
ZRESULT
vlog_show_vrs(VLOG_SHOW_ROW_CB show_row_cb, intptr_t user_ref)
{
  struct vrep_table *rtab;
  s_int16_t          rc;
  VLOG_SHOW_REF      show_ref;
  ZRESULT            zres;

  /* Create the VREP object: 6 column report - maximum width 120 characters. */
  rtab = vrep_create (6, 120);
  if (! rtab)
    return ZRES_ERR;

  /* Add the header row. */
  rc =vrep_add_next_row (rtab, NULL,
                         " VR-Name \t VR-Id \t PVR-Terms \t VR-Terms \t"
                         " LogFile \t CurSize");
  if (rc != VREP_SUCCESS)
    {
      vrep_delete (rtab);
      return ZRES_ERR;
    }

  /* Walking all VRs and executing _vlog_show_vr callback. */
  zres = vlog_vrs_walk((VLOG_VRS_WALK_CB)_vlog_show_vr_cb,
                       (intptr_t)rtab);
  /* In case of error, we display whatever has been collected.
  */
  /* Prepare control info for _vlog_vrep_show callback. */
  show_ref.vsr_show_row_cb = show_row_cb;
  show_ref.vsr_user_ref    = user_ref;

  /* Sending the report to the caller. */
  rc = vrep_show (rtab, _vlog_vrep_show_cb, (intptr_t)&show_ref);

  /* Destruct the VREP object. */
  vrep_delete (rtab);

  return ((rc == VREP_SUCCESS && zres == ZRES_OK) ? ZRES_OK : ZRES_ERR);
}

/*---------------------------------------------------------------------
 * _vlog_show_client_cb - Adding the next client entry to the
 *                        VLOG clients report.
 */
static ZRESULT
_vlog_show_client_cb(VLOG_SRV_ENTRY *vse, intptr_t rtab)
{
  char con_time_buf[128];
  char read_time_buf[128];
  struct pal_tm exptime;
  s_int16_t rc;

  pal_time_loc (&vse->vse_connect_time, &exptime);
  pal_time_strf (con_time_buf, sizeof(con_time_buf)-1,
                 "%a %b-%d %H:%M:%S",
                 &exptime);

  pal_time_loc (&vse->vse_read_time, &exptime);
  pal_time_strf (read_time_buf, sizeof(con_time_buf)-1,
                 "%a %b-%d %H:%M:%S",
                 &exptime);

  rc= vrep_add_next_row ((struct vrep_table *)rtab,
                         NULL,
                         "%s \t %d\t %d\t%s \t%s",
                         modname_strl(vse->vse_mod_id),
                         vse->vse_mod_id,
                         vse->vse_recv_msg_count,
                         con_time_buf,
                         read_time_buf);
  return (rc == VREP_SUCCESS ? ZRES_OK : ZRES_ERR);
}

/*-----------------------------------------------------------------
 * vlog_show_clients - API to create the VRs report and return it to the
 *                     caller using the show_row_cb. Note: This function
 *                     is agnostic about the report destination.
 */
ZRESULT
vlog_show_clients(VLOG_SHOW_ROW_CB show_row_cb, intptr_t user_ref)
{
  struct vrep_table *rtab;
  s_int16_t          rc;
  VLOG_SHOW_REF      show_ref;
  ZRESULT            zres;

  /* Create the VREP object: 5 column report - maximum width 120 characters. */
  rtab = vrep_create (5, 120);
  if (! rtab)
    return ZRES_ERR;

  /* Add the header row. */
  rc = vrep_add_next_row (rtab, NULL,
                          " Name \t Id \t MsgCnt \t ConTime \t ReadTime ");
  if (rc != VREP_SUCCESS)
    {
      vrep_delete (rtab);
      return ZRES_ERR;
    }

  /* Walking all VLOG clients and executing _vlog_show_client_cb callback. */
  zres = vlog_server_walk_clients(vlog_vgb->vgb_server,
                                  _vlog_show_client_cb,
                                  (intptr_t)rtab);
  /* In case of error, we display whatever has been collected.
  */
  /* Prepare control info for _vlog_vrep_show callback. */
  show_ref.vsr_show_row_cb = show_row_cb;
  show_ref.vsr_user_ref    = user_ref;

  /* Sending the report to the caller. */
  rc = vrep_show (rtab, _vlog_vrep_show_cb, (intptr_t)&show_ref);

  /* Destruct the VREP object. */
  vrep_delete (rtab);

  return ((rc == VREP_SUCCESS && zres == ZRES_OK) ? ZRES_OK : ZRES_ERR);
}


/*-----------------------------------------------------------------
 * vlog_show_all - API to generate and return to the caller all three reports.
 */
ZRESULT
vlog_show_all(VLOG_SHOW_ROW_CB show_row_cb, intptr_t user_ref)
{
  ZRESULT zres = ZRES_OK;

  zres |= vlog_show_terminals(show_row_cb, user_ref);
  zres |= vlog_show_vrs(show_row_cb, user_ref);
  zres |= vlog_show_clients(show_row_cb, user_ref);

  return zres;
}

