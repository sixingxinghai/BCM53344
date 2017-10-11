/*=============================================================================
**
** Copyright (C) 2008-2009   All Rights Reserved.
**
** vlog_api.h - Definition of API functions.
*/
#ifndef _PACOS_VLOG_API_H
#define _PACOS_VLOG_API_H

#include "pal.h"

/* Local error codes. */

#define VLOG_ZRES_BASE (APN_PROTO_VLOG*(-100))
#define VLOG_ZRES_MULTIPLE_FILES_ERR  VLOG_ZRES_BASE-1
#define VLOG_ZRES_OUT_OF_MEM_ERR      VLOG_ZRES_BASE-2
#define VLOG_ZRES_FILE_OPEN_ERR       VLOG_ZRES_BASE-3


ZRESULT vlog_add_vr(char *vr_name, u_int32_t vr_id);
void vlog_del_vr(char *vr_name, u_int32_t vr_id);

/*--------------------------------------------------------
 * vlog_add_term_to_vr - Add terminal destination to
 *                       VR distribution list.
 * Params:
 *   vr_id     - Id of VR to which user is connected.
 *   term_name - TTY name or encoded socket number of the user terminal
 *   vr_name   - NULL if adding this VR only
 *               != NULL if adding specific VR to PVR user VR list "all" VRs
 *   all_vrs   - PAL_TRUE if adding all VRs to PVR user VR list
 */
ZRESULT vlog_add_term_to_vr (u_int32_t vr_id,
                             char     *term_name,
                             char     *vr_name);

/*-------------------------------------------------------------------------
 * vlog_del_term_from_vr - Delete terminal destination from
 *                         VR distribution list.
 * Params:
 *   vr_id     - VR id in the context of terminal
 *   term_name - TTY name or encoded socket number of the user terminal
 *   vr_name   - NULL if adding this VR only
 *               != NULL if adding specific VR to PVR user VR list "all" VRs
 *
 */
ZRESULT vlog_del_term_from_vr (u_int32_t vr_id,
                               char     *term_name,
                               char     *vr_name);



/*-----------------------------------------------------------
 *-----------------------------------------------------------
 *                  LOGGING TO FILES
 *-----------------------------------------------------------
 *-----------------------------------------------------------
 */
#ifdef PAL_LOG_FILESYS
ZRESULT vlog_set_log_file (u_int32_t vr_id, char *fname);
ZRESULT vlog_unset_log_file (u_int32_t vr_id);
ZRESULT vlog_get_log_file (u_int32_t vr_id, char **pfname);
ZRESULT vlog_reset_log_file (u_int32_t vr_id);
#endif /* PAL_LOG_FILESYS */

typedef ZRESULT (* VLOG_SHOW_ROW_CB)(intptr_t user_ref, char *str);

ZRESULT vlog_show_terminals(VLOG_SHOW_ROW_CB show_row_cb, intptr_t user_ref);
ZRESULT vlog_show_vrs(VLOG_SHOW_ROW_CB show_row_cb, intptr_t user_ref);
ZRESULT vlog_show_clients(VLOG_SHOW_ROW_CB show_row_cb, intptr_t user_ref);
ZRESULT vlog_show_all(VLOG_SHOW_ROW_CB show_row_cb, intptr_t user_ref);


#endif

