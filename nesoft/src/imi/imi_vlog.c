/* Copyright (C) 2009  All Rights Reserved. */


#include "pal.h"
#include "lib.h"

#include "cli.h"
#include "modbmap.h"
#include "show.h"
#include "line.h"
#include "vty.h"
#include "host.h"
#include "log.h"
#include "snprintf.h"

#include "imi/imi.h"
#include "imi/imi_api.h"
#include "imi/imi_line.h"
#include "imi/imi_interface.h"
#include "imi/imi_parser.h"
#include "imi/imi_server.h"

#ifdef HAVE_VLOGD

/*--------------------------------------------------------------------
 * imi_vlog_send_term_cmd - Send "terminal-monitor-vr command to the
 *                          VLOGD.
 * Description:
 *  It will generate internal CLI to forward to the VLOGD a user requested
 *  binding/unbinding to/from a specific VR or "all"vrs.
 */

ZRESULT
imi_vlog_send_term_cmd (struct line *line,
                        char *vr_name,
                        IMI_VLOG_TERM_CMD cmd)
{
  struct apn_vr *vr;
  int ret = 0;
  struct line vline;
  bool_t attach = PAL_TRUE;

  if (! line->tty)
    return IMI_API_SET_ERROR;

  if ((vr = line->vr) == NULL)
    return IMI_API_SET_ERROR;

  /* Only PVR may attach/detach a specific VR or all VRs. */
  if (vr_name != NULL && vr->id != 0)
    return IMI_API_SET_ERROR;

  /* We will reuse the IMISH client line structure.
   * Preserve vital parameters.
   */
  vline = *line;
  vline.str = &vline.buf[LINE_HEADER_LEN];

  /* Changing line Parameters. */
  vline.code   = LINE_CODE_COMMAND;
  vline.mode   = EXEC_MODE;
  vline.module = PM_VLOG;

  switch (cmd)
  {
  case IMI_VLOG_TERM_CMD__ATTACH_A_VR:
    if (vr_name != NULL)
      pal_sprintf (vline.str, "vlog terminal-vr %s %s", line->tty, vr_name);
    else
      pal_sprintf (vline.str, "vlog terminal-vr %s", line->tty);
    break;
  case IMI_VLOG_TERM_CMD__ATTACH_ALL_VRS:
    pal_sprintf (vline.str, "vlog terminal-vr %s all", line->tty);
    break;
  case IMI_VLOG_TERM_CMD__DETACH_A_VR:
    attach = PAL_FALSE;
    if (vr_name != NULL)
      pal_sprintf (vline.str, "no vlog terminal-vr %s %s",
                   line->tty, vr_name);
    else
      /* Remove this terminal from VLOG. */
      pal_sprintf (vline.str, "no vlog terminal-vr %s", line->tty);
    break;
  case IMI_VLOG_TERM_CMD__DETACH_ALL_VRS:
    attach = PAL_FALSE;
    /* Remove this terminal from VLOG. */
    pal_sprintf (vline.str, "no vlog terminal-vr %s", line->tty);
    break;
  default:
    return IMI_API_SET_ERROR;
  }
  ret = imi_server_line_pm_send (APN_PROTO_VLOG, &vline);

  switch (ret)
    {
    case ZRES_OK:
      return IMI_API_SET_SUCCESS;
    case ZRES_FAIL:
      if (attach)
        return IMI_API_SET_ERR_ATTACH_TERM_TO_VR;
      else
        return IMI_API_SET_ERR_DETACH_TERM_FROM_VR;
    default:
      return IMI_API_SET_ERROR;
    }
  return IMI_API_SET_ERROR;
}


ZRESULT
imi_vlog_send_vr_instance_cmd(char *vr_name,
                              u_int32_t vr_id,
                              bool_t is_addition)
{
  int ret = 0;
  struct line cline;


  pal_mem_set(&cline, 0, sizeof(cline));

  cline.str = &cline.buf[LINE_HEADER_LEN];

  /* Changing line Parameters. */
  cline.code = LINE_CODE_COMMAND;
  cline.mode = EXEC_MODE;
  cline.module = PM_VLOG;
  cline.privilege = PRIVILEGE_MAX;

  if (is_addition)
    pal_sprintf (cline.str, "vr-instance %s %d", vr_name, vr_id);
  else
    pal_sprintf (cline.str, "no vr-instance %s %d", vr_name, vr_id);

  ret = imi_server_line_pm_send (APN_PROTO_VLOG, &cline);

  if (ret != ZRES_OK)
    return IMI_API_SET_ERROR;
  else
    return IMI_API_SET_SUCCESS;
}

static ZRESULT
_imi_vlog_vr_vect_walk_cb(struct apn_vr *vr, intptr_t dummy)
{
  struct imi_master *im = vr->proto;

  /* Just to make sure that we operate in the PVR context when
     configuring VLOGD with virtual routers.
   */
  LIB_GLOB_SET_VR_CONTEXT(vr->zg, apn_vr_get_privileged (vr->zg));

  /* We send only VRs "added" by NSM. */
  if (vr->id != 0)
    if (im->im_nsm_add_flag)
      imi_vlog_send_vr_instance_cmd(vr->name, vr->id, PAL_TRUE);
  return ZRES_OK;
}

ZRESULT
imi_vlog_send_all_vrs()
{
  apn_vrs_walk_and_exec(imig->zg,
                        _imi_vlog_vr_vect_walk_cb,
                        0);
  return ZRES_OK;
}



/*---------------------------------------------------------------
 * Callbacks to communicate with lib/log library.
 *
 */
static ZRESULT
_vlog_vr_file_set_cb(struct lib_globals *zg, u_int32_t vrid, char *fname)
{
  struct apn_vr *vr;
  struct imi_master *im;

  vr = apn_vr_get_by_id(zg, vrid);
  if (vr == NULL)
    return ZRES_ERR;

  if ((im = vr->proto) == NULL)
    return ZRES_ERR;

  /* If file already set, we replace it with the new one. */
  if (im->im_vlog_fname != NULL)
    {
      XFREE(MTYPE_VLOG_FNAME, im->im_vlog_fname);
      im->im_vlog_fname = NULL;
    }

  if (fname)
    im->im_vlog_fname = XSTRDUP(MTYPE_VLOG_FNAME, fname);
  else
    im->im_vlog_fname = XSTRDUP(MTYPE_VLOG_FNAME, "");

  if (im->im_vlog_fname)
    return ZRES_OK;
  else
    return ZRES_ERR;
}

static ZRESULT
_vlog_vr_file_unset_cb(struct lib_globals *zg, u_int32_t vrid)
{
  struct apn_vr *vr;
  struct imi_master *im;

  vr = apn_vr_get_by_id(zg, vrid);
  if (vr == NULL)
    return ZRES_ERR;

  if ((im = vr->proto) == NULL)
    return ZRES_ERR;

  if (im->im_vlog_fname == NULL)
    return ZRES_OK;

  XFREE(MTYPE_VLOG_FNAME, im->im_vlog_fname);
  im->im_vlog_fname = NULL;
  return ZRES_OK;
}

static ZRESULT
_vlog_vr_file_get_cb(struct lib_globals *zg, u_int32_t vrid, char **pfname)
{
  struct apn_vr *vr;
  struct imi_master *im;

  vr = apn_vr_get_by_id(zg, vrid);
  if (vr == NULL)
    return ZRES_ERR;

  if ((im = vr->proto) != NULL)
    {
      *pfname = im->im_vlog_fname;
      return ZRES_OK;
    }
  return ZRES_ERR;
}

ZRESULT
imi_vlog_init(struct lib_globals *zg)
{
  lib_reg_vlog_cbs(zg,
                   _vlog_vr_file_set_cb,
                   _vlog_vr_file_unset_cb,
                   _vlog_vr_file_get_cb);
  return ZRES_OK;
}

#endif /* HAVE_VLOGD. */


