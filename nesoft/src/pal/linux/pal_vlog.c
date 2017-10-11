/*=============================================================================
**
** Copyright (C) 2008-2009   All Rights Reserved.
**
** pal_vlog.c -- PAL support for VLOGD.
*/

#include "pal.h"


ZRESULT
pal_vlog_shift_files(PAL_VLOG_FILE *pvf)
{
  char fname[256];

  /* Close teh current file. */
  pal_vlog_close_file(pvf);

  sprintf(fname, "%s.1", pvf->pvf_full_path);

  /* Delete the .1 file */
  remove(fname);

  /* Move the current file to the .1 file */
  rename(pvf->pvf_full_path, fname);

  /* Open the new current file. */
  pvf->pvf_fp = fopen (pvf->pvf_full_path, "a+");
  pvf->pvf_cur_file_size= 0;

  return ZRES_OK;
}

/*---------------------------------------------------------------------
 * pal_vlog_write_file - Write a line to the file.
 *                       If file full, re-sequence them.
 * Params:
 *   pvf     - PAL instantiation of VLOG log file.
 *   log_str - The string to write to the log file.
 *   len     - The length of teh string.
 */

ZRESULT
pal_vlog_write_file(PAL_VLOG_FILE *pvf, char *log_str, int len)
{
  fprintf (pvf->pvf_fp, "%s\n", log_str);
  fflush (pvf->pvf_fp);

  pvf->pvf_cur_file_size += len;

  if (pvf->pvf_cur_file_size >= pvf->pvf_max_file_size)
    pal_vlog_shift_files(pvf);

  return (ZRES_OK);
}

/*--------------------------------------------------------------------
 * pal_vlog_open_file
 *
 * Params
 *    pvf     - PAL instantiation of VLOG log file.
 *              NOTE: This is object is created by application.
 *              PAL should restrain itself from allocating/freeing memory.
 *    fullpath- Global path to the log file (including file name)
 *    max-file-size - Maximum file size.
 */
ZRESULT
pal_vlog_open_file(PAL_VLOG_FILE *pvf,
                   char     *fullpath,
                   u_int32_t max_file_size)
{
  int res, len, i;
  char *cp;

 /* Recursive mkdir. Skip leading '/' */
  len = strlen (fullpath);

  cp = fullpath;
  for (i = 1; i < len; i++)
    {
      if (cp[i] == DIR_CHAR)
        {
          cp[i] = '\0';
          res = mkdir (cp, 00750);
          cp[i] = DIR_CHAR;
          if (res != 0 && errno != EEXIST)
            return ZRES_ERR;
        }
    }
  /* Open file. */
  pvf->pvf_fp = fopen (fullpath, "a+");
  if (pvf->pvf_fp == NULL)
    return ZRES_ERR;

  pvf->pvf_full_path     = fullpath;
  pvf->pvf_max_file_size = max_file_size;
  /* Read the current file size. */
  fseek(pvf->pvf_fp, 0, SEEK_END);
  pvf->pvf_cur_file_size = ftell(pvf->pvf_fp);
  return ZRES_OK;
}


/*--------------------------------------------------------------------
 * pal_vlog_close_file
 *
 * Params
 *    pvf     - PAL instantiation of VLOG log file.
 *              Application will destruct this object if necessary.
 */
ZRESULT
pal_vlog_close_file(PAL_VLOG_FILE *pvf)
{
  if (pvf->pvf_fp != NULL)
  {
    fclose(pvf->pvf_fp);
    pvf->pvf_fp = NULL;
  }
  return (ZRES_OK);
}


