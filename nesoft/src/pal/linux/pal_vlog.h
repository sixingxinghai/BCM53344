/*=============================================================================
**
** Copyright (C) 2008-2009   All Rights Reserved.
**
** pal_vlog.c -- PAL support for VLOGD.
*/

#ifndef _PAL_VLOG_H_
#define _PAL_VLOG_H_


/*------------------------------------------------------------------
 * PAL_VLOG_FILE - Definition of the PAL's log file representation.
 *
 * Members:
 *   pvf_fp             - FILE handle of open log file.
 *   pvf_full_path      - Global log file path
 *   pvf_cfg_fname      - Configured file name (may be "")
 *   pvf_max_file_size  - Maximum file size
 *   pvf_cur_file_size  - Current files size
 */
typedef struct pal_vlog_file
{
  FILE      *pvf_fp;
  char      *pvf_full_path;
  char      *pvf_cfg_fname;
  u_int32_t  pvf_max_file_size;
  u_int32_t  pvf_cur_file_size;
} PAL_VLOG_FILE;


ZRESULT pal_vlog_open_file(PAL_VLOG_FILE *pvf,
                           char     *fullpath,
                           u_int32_t max_file_size);

ZRESULT pal_vlog_write_file(PAL_VLOG_FILE *pvf, char *log_str, int len);

ZRESULT pal_vlog_close_file(PAL_VLOG_FILE *pvf);

ZRESULT pal_vlog_shift_files(PAL_VLOG_FILE *pvf);

#endif



