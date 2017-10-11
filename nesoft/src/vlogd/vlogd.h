/*=============================================================================
**
** Copyright (C) 2008-2009   All Rights Reserved.
**
** vlogd.h -- All the local to the VLOGD definitions.
*/

#ifndef _PACOS_VLOGD_H
#define _PACOS_VLOGD_H

#include "pal.h"
#include "lib.h"

#include "thread.h"
#include "timeutil.h"

#include "vlog_client.h"
#include "vlog_server.h"
#include "vlog_api.h"

/* Error definitions. */
#define VLOG_ERR_MEM_ALLOC_FAILURE      -7
#define VLOG_VR_LOOKUP_FAILURE          -9

#define VLOG_VTY_SOCK_STR_LEN            5
#define VLOG_LOG_MSG_MAX_SIZE          (ZLOG_BUF_MAXLEN+200)
#define VLOG_TIME_STRING_LENGTH        256
#define VLOG_PVR_ID                      0

#define VLOG_TERM_NAME_MAX 64

#define VLOG_TERM_SHUTDOWN_GRACE_PERIOD (120)

/*-------------------------------------------------------------------------
 * vlog_vr_open_pvr_def
 *
 * Default directory: /vr/log/pacos/pvr
 * Default log file: pvr_log.<seq_num>
 * <seq-num> 1..9
 */
#define VLOG_PACOS_LOG_DIR    "/var/local/pacos/log"
#define VLOG_PVR_NAME         "pvr"
#define VLOG_LOG_WORD         "log"
#define VLOG_DEF_FILE_SIZE    (10*1024*1024)

typedef enum vlog_fname_type
{
  VLOG_FNAME_PVR_DEFAULT,
  VLOG_FNAME_PVR_LOCAL,
  VLOG_FNAME_PVR_GLOBAL,
  VLOG_FNAME_VR_DEFAULT,
  VLOG_FNAME_VR_LOCAL,
} VLOG_FNAME_TYPE;


/*---------------------------------------------------------------------
 * VLOG_TERMS   - Container of VLOG_TERM object references
 *                Used by VLOG_VR and VLOG_VGB
 * Members:
 *    vts_vect - Vector of references to VLOG_TERM objects
 *---------------------------------------------------------------------
 */
typedef struct vlog_terms
{
  struct list vts_list;
  s_int8_t    vts_num_pvr_terms;
  s_int8_t    vts_num_vr_terms;
} VLOG_TERMS;

/*---------------------------------------------------------------------
 * VLOG_VGB   - VLOG globals data type
 *
 * Members:
 *  vgb_lgb   - Pointer to lib_globals
 *  vgb_terms - Container of all active terminals
 *  vgb_server- VLOGD server for PM clients
 *  vgb_timer_thread - Terminals shutdown timer thread.
 *---------------------------------------------------------------------
 */
typedef struct vlog_globals
{
  struct lib_globals *vgb_lgb;
  VLOG_TERMS         *vgb_terms;
  VLOG_SERVER        *vgb_server;
  struct thread      *vgb_timer_thread;
} VLOG_GLOBALS;


/*---------------------------------------------------------------------
 * VLOG_TERM   - VLOG terminal object data structure
 *
 * Members:
 *  vtm_type   - Terminal type: TTY or socket
 *  vtm_vr_id  - VR id of the user's home VR
 *  vtm_is_pvr - TRUE if its user is PVR
 *  vtm_all_vrs- PVR term only: TRUE if the PVR user requested debug output
 *               from all VRs. Adding a new non-PVR will automatically
 *               bind it to this terminal.
 *  vtm_shutdown - Set to TRUE when IMI dies. After grace period if still
 *               TRUE, it will cause the terminal to shutdown.
 *  vtm_name   - Terminal name or ASCI encoded socket number
 *  vtm_fd     - File descriptor
 *  vtm_vr_cnt - Number of VRs using this terminal
 *---------------------------------------------------------------------
 */
typedef enum vlog_term_type
{
  VLOG_TERM_TYPE_NONE,
  VLOG_TERM_TYPE_TTY,
  VLOG_TERM_TYPE_SOCK,
  VLOG_TERM_TYPE_MAX

} VLOG_TERM_TYPE;

typedef struct vlog_term
{
  VLOG_TERM_TYPE vtm_type;
  u_int32_t      vtm_vr_id;
  bool_t         vtm_is_pvr;
  bool_t         vtm_all_vrs;
  bool_t         vtm_shutdown;
  char          *vtm_name;
  s_int32_t      vtm_fd;
  s_int32_t      vtm_vr_cnt;

} VLOG_TERM;

/*---------------------------------------------------------------------
 * VLOG_VR   - VLOG's VR master
 *             Holds container of VLOG_TERM objects (references),
 *             where this VR forwards log messages.
 * Members:
 *    vvr_ivr   - Pointer to apn_vr object.
 *    vvr_name_len - Length of the VR name (we do not need to strlen it
 *                   all the time.
 *    vvr_terms - Pointer to VLOG_TERMS container.
 *    vvr_pvf   - Pointer to PAL_VLOG_FILE instance for this VR.
 *---------------------------------------------------------------------
 */
typedef struct vlog_vr
{
  struct apn_vr *vvr_ivr;
  u_int16_t      vvr_name_len;
  VLOG_TERMS    *vvr_terms;
  PAL_VLOG_FILE *vvr_pvf;
} VLOG_VR;

/*---------------------------------------------------------------------
 * VLOG_TERM_REQ   - Write request control
 *
 * Purpose:
 *    Encapsulates write request attributes so thye can be passed
 *    as a single function parameter.
 *
 * Members:
 *    vtr_orig_vrid       - Id of the originating VR
 *    vtr_orig_vrname_len - The length of VR name
 *    vtr_tot_msg_len     - The total length of the log message
 *    vtr_msg             - Ptr to log message
 *---------------------------------------------------------------------
 */
typedef struct vlog_term_req
{
  u_int16_t   vtr_orig_vrid;
  u_int16_t   vtr_orig_vrname_len;
  u_int16_t   vtr_tot_msg_len;
  char       *vtr_msg;
} VLOG_TERM_REQ;


typedef ZRESULT (* VLOG_VRS_WALK_CB)(VLOG_VR *, intptr_t);

ZRESULT  vlog_vrs_walk (VLOG_VRS_WALK_CB, intptr_t);
VLOG_VR *vlog_vrs_lookup_vr(u_int32_t vr_id);
void     vlog_vrs_delete();
VLOG_VR *vlog_vr_new(char *vr_name, u_int32_t vr_id);
void     vlog_vr_delete(VLOG_VR *vvr);
void     vlog_vr_proto_delete(VLOG_VR *vvr);

ZRESULT  vlog_vr_bind_term(VLOG_VR *vvr, VLOG_TERM *term);
ZRESULT  vlog_vr_unbind_term(VLOG_VR *vvr, VLOG_TERM *term);
ZRESULT  vlog_vr_forward_log_msg(VLOG_VR      *vvr,
                                 module_id_t  clt_mod_id,
                                 u_int32_t    clt_proc_id,
                                 s_int8_t     priority,
                                 char        *in_msg);

ZRESULT vlog_vr_open_log_file(VLOG_VR *vvr,
                              VLOG_FNAME_TYPE file_name_type,
                              char *fname);
ZRESULT vlog_vr_close_log_file(VLOG_VR *vvr);
ZRESULT vlog_vr_write_log_file(VLOG_VR *vvr, VLOG_TERM_REQ *wr_req);
ZRESULT vlog_vr_get_log_file(VLOG_VR *vvr, char **pfname);
ZRESULT vlog_vr_reset_log_file(VLOG_VR *vvr);

typedef void (* VLOG_TERMS_WALK_CB)(VLOG_TERM *, intptr_t);
typedef ZRESULT (* VLOG_TERMS_WALK_WITH_ABORT_CB)(VLOG_TERM *, intptr_t);

VLOG_TERMS *vlog_terms_new();
ZRESULT     vlog_terms_delete(VLOG_TERMS *terms);
ZRESULT     vlog_terms_init_shutdown(VLOG_TERMS *terms);
VLOG_TERM  *vlog_terms_lookup(VLOG_TERMS *terms, char *term_name);
ZRESULT     vlog_terms_link_term(VLOG_TERMS *terms, VLOG_TERM *term);
void        vlog_terms_unlink_term(VLOG_TERMS *terms, VLOG_TERM *term);
void        vlog_terms_walk (VLOG_TERMS *, VLOG_TERMS_WALK_CB, intptr_t);
ZRESULT     vlog_terms_walk_with_abort (VLOG_TERMS *, VLOG_TERMS_WALK_WITH_ABORT_CB,
                                        intptr_t);
void        vlog_terms_finish (VLOG_TERMS *terms);
void        vlog_terms_write (VLOG_TERMS *terms, VLOG_TERM_REQ *wr_req);

void        vlog_terms_broadcast (VLOG_TERMS *terms, char *info);

VLOG_TERM * vlog_term_new(char *term_name, u_int32_t vr_id);
void        vlog_term_del_unused(VLOG_TERM *term);
void        vlog_term_delete(VLOG_TERM *term);
ZRESULT     vlog_term_include_vr(VLOG_TERM *term, VLOG_VR *vvr);
ZRESULT     vlog_term_exclude_vr(VLOG_TERM *term, VLOG_VR *vvr);
ZRESULT     vlog_term_write (VLOG_TERM *term, VLOG_TERM_REQ *wr_req);
ZRESULT     vlog_term_bind_all_vr(VLOG_TERM *term, VLOG_VR *vvr);


void vlog_cli_init (struct lib_globals *zg);
ZRESULT vlog_lib_init(struct lib_globals *zg);

#endif /* #define _PACOS_VLOGD_H */

