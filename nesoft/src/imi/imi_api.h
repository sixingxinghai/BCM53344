/* Copyright (C) 2003  All Rights Reserved. */

#ifndef _PACOS_IMI_API_H
#define _PACOS_IMI_API_H

#include "pal.h"
#include "cli.h"
#include "imi.h"

#define IMI_API_SET_SUCCESS                                             0
#define IMI_API_SET_ERROR                                              -1

#define IMI_API_SET_ERR_TERMINAL_MONITOR                              -10
#define IMI_API_SET_ERR_ATTACH_TERM_TO_VR                             -11
#define IMI_API_SET_ERR_DETACH_TERM_FROM_VR                           -12

#ifdef HAVE_VLOGD
/* Commands for VLOG API function. */
typedef enum {
  IMI_VLOG_TERM_CMD__ATTACH_A_VR = 1,
  IMI_VLOG_TERM_CMD__ATTACH_ALL_VRS,
  IMI_VLOG_TERM_CMD__DETACH_A_VR,
  IMI_VLOG_TERM_CMD__DETACH_ALL_VRS
} IMI_VLOG_TERM_CMD;
#endif

/* External CLI function definition. */
#define CLI_EXTERN CLI

/* External CLI print definition. */
#ifdef HAVE_ISO_MACRO_VARARGS
#define CLI_PRINT(...)                                                  \
  do {                                                                  \
     cli_out(cli, __VA_ARGS__);                                         \
     cli_out(cli, "\n");                                                \
  } while (0)
#else
#define CLI_PRINT(ARGS...)                                              \
  do {                                                                  \
     cli_out(cli, ARGS);                                                \
     cli_out(cli, "\n");                                                \
  } while (0)
#endif /* HAVE_ISO_MACRO_VARARGS. */

/* API functions.  */
void imi_extern_init ();
void imi_extern_shutdown ();
void imi_extern_config_write ();

#ifdef HAVE_VLOGD
ZRESULT imi_vlog_send_term_cmd (struct line *xline, char *vr_name,
                                IMI_VLOG_TERM_CMD cmd);
ZRESULT imi_vlog_send_vr_instance_cmd(char *vr_name,
                                      u_int32_t vr_id,
                                      bool_t is_addition);
ZRESULT imi_vlog_send_all_vrs();
#endif

#endif /* _PACOS_IMI_API_H */
