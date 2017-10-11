/* Copyright (C) 2003   All Rights Reserved.  */

#include "pal.h"
#include "log.h"
#include "snprintf.h"
#include "cli.h"
#include "vty.h"
#include "thread.h"

#ifdef HAVE_VLOGD
#include "vlog_client.h"
#endif /* HAVE_VLOGD. */

/* #define ZLOG_VLOGD_DEBUG */
#ifdef ZLOG_VLOGD_DEBUG
/* Linux environment testing only. */
struct thread *_zlog_vlogd_test_thread = NULL;
static int _zlog_vlogd_test_cb (struct thread *thread);
static void _zlog_vlogd_start_test(struct lib_globals *zg);
#endif /* ZLOG_VLOGD_DEBUG */

char *zlog_priority[] =
{
  "emergencies",
  "alerts",
  "critical",
  "errors",
  "warnings",
  "notifications",
  "informational",
  "debugging",
  NULL
};

char *
zlog_get_priority_str(s_int8_t prio)
{
  if (prio < ZLOG_EMERGENCY || prio > ZLOG_DEBUG)
    return "";
  else
    return zlog_priority[prio];
}

struct zlog *
zlog_new ()
{
  struct zlog *zl;

  zl = XCALLOC (MTYPE_ZLOG, sizeof(struct zlog));

  return zl;
}

void
zlog_free (struct zlog **zl)
{
  pal_assert (*zl != NULL);

  XFREE (MTYPE_ZLOG, *zl);
  *zl = NULL;

  return;
}

/* Set flag. */
static void
zlog_set_flag (struct lib_globals *zg, struct zlog *zl, u_char flags)
{
  if (zl == NULL)
    zl = zg->log_default;

  zl->flags |= flags;
}

/* Unset flags. */
static void
zlog_unset_flag (struct lib_globals *zg, struct zlog *zl, u_char flags)
{
  if (zl == NULL)
    zl = zg->log_default;

  zl->flags &= ~flags;
}

/* Open a particular log. */
struct zlog *
openzlog (struct lib_globals *zg, u_int32_t instance, module_id_t protocol,
          enum log_destination dest)
{
  struct zlog *zl = NULL;
  void *pal_log_data;

  zl = zlog_new ();
  if (zl)
    {
      zl->instance = instance;
      zl->protocol = protocol;
      zl->maskpri = ZLOG_DEBUG;
      zl->record_priority = 0;
      zl->dest = dest;

      pal_log_data = pal_log_open (zg, zl, dest);
      if (! pal_log_data)
        {
          zlog_free (&zl);
          return NULL;
        }

      /* PAL specific data. */
      zl->pal_log_data = pal_log_data;

#ifdef HAVE_VLOGD
      /* When in the context of VLOGD, we cannot be a client of VLOG server. */
      if (protocol != APN_PROTO_VLOG)
        /* Write log messages to VLOG server. */
        vlog_client_create(zg);
      /* Create VLOG client. */
#endif

#ifdef HAVE_IMISH
      zlog_set_flag (zg, zl, ZLOG_SYSTEM);
#endif /* HAVE_IMISH */

#ifdef ZLOG_VLOGD_DEBUG
      _zlog_vlogd_start_test(zg);
#endif
      return zl;
    }

  return NULL;
}

/* Close log. */
void
closezlog (struct lib_globals *zg, struct zlog *zl)
{
  pal_assert ((zg != NULL) && (zl != NULL));

  if (zl)
    {
#ifdef HAVE_VLOGD
      vlog_client_delete(zg);
#endif /* HAVE_VLOGD. */
      pal_log_close (zg, zl);
      zlog_free (&zl);
    }

  return;
}

static void
vzlog (struct lib_globals *zg, struct zlog *zl, u_int32_t priority,
       const char *format, va_list args)
{
  char buf[ZLOG_BUF_MAXLEN];
  char *protostr;
  int   len;

#ifdef HAVE_VLOGD
  /* We need to place the VR name in front the message for syslog.
  */
  char fmt[ZLOG_BUF_MAXLEN];
  char *vr_name="<PVR>";
  int vr_name_len = 0;

  if (zg->vr_in_cxt != NULL)
    if (zg->vr_in_cxt->name)
      vr_name = zg->vr_in_cxt->name;

  sprintf(fmt, "%s: %s", vr_name, format);
  vr_name_len = pal_strlen(vr_name)+2;

  if (! zl)
    zl = zg->log_default;
  /* First prepare output string. */
  len = zvsnprintf (buf, sizeof(buf), fmt, args);
#else

  if (! zl)
    zl = zg->log_default;
  /* First prepare output string. */
  len = zvsnprintf (buf, sizeof(buf), format, args);
#endif
  if (zl == NULL)
    {
      struct zlog tzl;

      pal_mem_set (&tzl, 0, sizeof(struct zlog));

      /* Use stderr. */
      zlog_set_flag (zg, &tzl, ZLOG_STDERR);

      pal_log_output (zg, &tzl, "", "", buf);
      return;
    }

  /* Log this information only if it has not been masked out. */
  if (priority > zl->maskpri)
    return;

  /* Protocol string. */
  protostr = modname_strl (zg->protocol);

  /* Log to log devices and Terminal monitor. */
  if (zl->record_priority)
    {
      /* Always try to send syslog, stderr, stdout. */
      pal_log_output (zg, zl, zlog_priority[priority], protostr, buf);
#ifdef HAVE_VLOGD
      /* VLOGD gets the message without VR name. */
      vlog_client_send_data_msg (zg, priority,
                                 &buf[vr_name_len],
                                 len-vr_name_len);
#else
      vty_log (zg, zlog_priority[priority], protostr, buf);
#endif /* HAVE_VLOGD. */
    }
  else
    {
    /* Always try to send syslog, stderr, stdout. */
      pal_log_output (zg, zl, "", protostr, buf);
#ifdef HAVE_VLOGD
       vlog_client_send_data_msg (zg, -1,
                                  &buf[vr_name_len],
                                  len-vr_name_len);
#else
      vty_log (zg, "", protostr, buf);
#endif /* HAVE_VLOGD. */
    }
}

void
zlog (struct lib_globals *zg, struct zlog *zl, int priority,
      const char *format, ...)
{
  va_list args;

  va_start (args, format);
  vzlog (zg, zl, priority, format, args);
  va_end (args);
}

/* Log error. */
void
zlog_err (struct lib_globals *zg, const char *format, ...)
{
  va_list args;
  struct zlog *zl;

  if (zg->log)
    zl = zg->log;
  else
    zl = zg->log_default;

  va_start (args, format);
  vzlog (zg, zl, ZLOG_ERROR, format, args);
  va_end (args);
}

/* Log warning. */
void
zlog_warn (struct lib_globals *zg, const char *format, ...)
{
  va_list args;
  struct zlog *zl;

  if (zg->log)
    zl = zg->log;
  else
    zl = zg->log_default;

  va_start (args, format);
  vzlog (zg, zl, ZLOG_WARN, format, args);
  va_end (args);
}

/* Log informational. */
void
zlog_info (struct lib_globals *zg, const char *format, ...)
{
  va_list args;
  struct zlog *zl;

  if (zg->log)
    zl = zg->log;
  else
    zl = zg->log_default;

  va_start (args, format);
  vzlog (zg, zl, ZLOG_INFO, format, args);
  va_end (args);
}

/* Log debug. */
void
zlog_debug (struct lib_globals *zg, const char *format, ...)
{
  va_list args;
  struct zlog *zl;

  if (zg->log)
    zl = zg->log;
  else
    zl = zg->log_default;

  va_start (args, format);
  vzlog (zg, zl, ZLOG_INFO, format, args);
  va_end (args);
}

/* Log error to specific log destination. */
void
plog_err (struct lib_globals *zg, struct zlog *zl, const char *format, ...)
{
  va_list args;

  va_start (args, format);
  vzlog (zg, zl, ZLOG_ERROR, format, args);
  va_end (args);
}

/* Log informational to specific log destination. */
void
plog_info (struct lib_globals *zg, struct zlog *zl, const char *format, ...)
{
  va_list args;

  va_start (args, format);
  vzlog (zg, zl, ZLOG_INFO, format, args);
  va_end (args);
}

/* Rotate logs. */
void
zlog_rotate (struct lib_globals *zg, struct zlog *zl)
{
  if (zl == NULL)
    zl = zg->log_default;

  if (zl == NULL)
    return;

  pal_log_rotate (zl);
}

#ifdef PAL_LOG_STDOUT
CLI (config_log_stdout,
     config_log_stdout_cli,
     "log stdout",
     "Logging control",
     "Logging goes to stdout")
{
  struct lib_globals *zg = cli->zg;

  zlog_set_flag (zg, zg->log, ZLOG_STDOUT);

  return CLI_SUCCESS;
}

CLI (no_config_log_stdout,
     no_config_log_stdout_cli,
     "no log stdout",
     CLI_NO_STR,
     "Logging control",
     "Cancel logging to stdout")
{
  struct lib_globals *zg = cli->zg;

  zlog_unset_flag (zg, zg->log, ZLOG_STDOUT);

  return CLI_SUCCESS;
}

#endif /* PAL_LOG_STDOUT. */

#ifdef PAL_LOG_FILESYS

#ifdef HAVE_VLOGD

CLI (config_vlog_log_file,
     config_vlog_log_file_cli,
     "log file (FILENAME|)",
     "Logging control",
     "Logging to file",
     "Logging filename")
{
  ZRESULT zres = ZRES_ERR;
  char *fname = NULL;
  struct lib_globals *zg = cli->zg;
  struct apn_vr      *vr = zg->vr_in_cxt;

  if (argc == 1)
    fname = argv[0];

  if (vr->id != 0 && fname)
    if (fname[0] == '/')
      {
        cli_out (cli, "%% Global path is not allowed in non-privileged VR context\n");
        return CLI_SUCCESS;
      }
  if (zg->vlog_file_set_cb)
    zres = zg->vlog_file_set_cb(zg, vr->id, fname);

  if (zres == ZRES_OK)
    return CLI_SUCCESS;
  else
      cli_out (cli, "%% Error while setting logging to file\n");
  return CLI_ERROR;
}

CLI (no_config_vlog_log_file,
     no_config_vlog_log_file_cli,
     "no log file (FILENAME|)",
     CLI_NO_STR,
     "Logging control")
{
  /* NOTE: The FILENAME is for backward compatibility.
           Only one file can be set at any time anyway.
  */
  ZRESULT zres = ZRES_ERR;
  struct lib_globals *zg = cli->zg;
  struct apn_vr      *vr = zg->vr_in_cxt;

  if (zg->vlog_file_unset_cb)
    zres = zg->vlog_file_unset_cb(zg, vr->id);

  if (zres == ZRES_OK)
    return CLI_SUCCESS;
  else
    cli_out (cli, "%% Error while canceling Logging to a file\n");
  return CLI_ERROR;
}

static void
_vlog_encode_file_name(struct lib_globals *zg, cfg_vect_t *cv)
{
  struct apn_vr *vr = zg->vr_in_cxt;
  char *fname;

  if (vr == NULL)
    vr = apn_vr_get_privileged(zg);

  if (zg->vlog_file_get_cb && vr)
  {
    if (zg->vlog_file_get_cb(zg, vr->id, &fname) != ZRES_OK)
      return;
    if (fname != NULL)
      cfg_vect_add_cmd (cv, "log file %s\n", fname);
  }
}

/* end of case for HAVE_VLOGD */

#else /* starting case for ! HAVE_VLOGD */

/* Set file params. */
static int
zlog_set_file (struct lib_globals *zg, struct zlog *zl,
               char *filename, u_int32_t size)
{
  int ret;

  if (zl == NULL)
    zl = zg->log_default;

  if (zl == NULL)
    return -1;

  zlog_set_flag (zg, zl, ZLOG_FILE);

  ret = pal_log_set_file (zl, filename, size);

  return ret;
}

/* Unset file params. */
static int
zlog_unset_file (struct lib_globals *zg, struct zlog *zl,
                 char *filename)
{
  int ret;

  if (zl == NULL)
    zl = zg->log_default;

  if (zl == NULL)
    return -1;

  if (! (zl->flags & ZLOG_FILE))
    return -1;

  ret = pal_log_unset_file (zl, filename);
  if (ret < 0)
    {
      /* Filename not matched. */
      return -1;
    }

  zl->logfile = NULL;
  zl->log_maxsize = 0;

  zlog_unset_flag (zg, zl, ZLOG_FILE);

  return 0;
}


CLI (config_log_file,
     config_log_file_cli,
     "log file FILENAME",
     "Logging control",
     "Logging to file",
     "Logging filename")
{
  struct lib_globals *zg = cli->zg;
  u_int32_t size = 0;
  int ret = 0;

  ret = zlog_set_file (zg, zg->log, argv[0], size);

  if (ret < 0)
    {
      cli_out (cli, "Specified log file %s is not set.\n", argv[0]);
      return CLI_ERROR;
    }
  return CLI_SUCCESS;
}

CLI (no_config_log_file,
     no_config_log_file_cli,
     "no log file (|FILENAME)",
     CLI_NO_STR,
     "Logging control",
     "Cancel logging to file",
     "Logging file name")
{
  struct lib_globals *zg = cli->zg;
  int ret;

  if (argc == 1)
    ret = zlog_unset_file (zg, zg->log, argv[0]);
  else
    ret = zlog_unset_file (zg, zg->log, NULL);
  if (ret < 0)
    {
      if (argc == 1)
        cli_out (cli, "Specified log file %s is not set.\n", argv[0]);
      else
        cli_out (cli, "No log file configured.\n");
      return CLI_ERROR;
    }

  return CLI_SUCCESS;
}
/* end of case for ! HAVE_VLOGD */
#endif /* ! HAVE_VLOGD */

#endif /* PAL_LOG_FILESYS */

#ifdef PAL_LOG_SYSTEM
CLI (config_log_syslog,
     config_log_syslog_cli,
     "log syslog",
     "Logging control",
     "Logging goes to syslog")
{
  struct lib_globals *zg = cli->zg;

  zlog_set_flag (zg, zg->log, ZLOG_SYSTEM);

  return CLI_SUCCESS;
}

CLI (no_config_log_syslog,
     no_config_log_syslog_cli,
     "no log syslog",
     CLI_NO_STR,
     "Logging control",
     "Cancel logging to syslog")
{
  struct lib_globals *zg = cli->zg;

  zlog_unset_flag (zg, zg->log, ZLOG_SYSTEM);

  return CLI_SUCCESS;
}
#endif /* PAL_LOG_SYSTEM. */

CLI (config_log_trap,
     config_log_trap_cli,
     "log trap (emergencies|alerts|critical|errors|warnings|notifications|informational|debugging)",
     "Logging control",
     "Limit logging to specified level",
     "Emergencies",
     "Alerts",
     "Critical",
     "Errors",
     "Warnings",
     "Notifications",
     "Informational",
     "Debugging")
{
  int new_level ;
  struct lib_globals *zg = cli->zg;
  struct zlog *zl;

  if (! zg->log)
    zl = zg->log_default;
  else
    zl = zg->log;

  for (new_level = 0; zlog_priority[new_level] != NULL; new_level ++)
    {
      /* Find new logging level */
      if (! pal_strcmp (argv[0], zlog_priority[new_level]))
        {
          zl->maskpri = new_level;
          return CLI_SUCCESS;
        }
    }
  return CLI_ERROR;
}

CLI (no_config_log_trap,
     no_config_log_trap_cli,
     "no log trap",
     CLI_NO_STR,
     "Logging control",
     "Permit all logging information")
{
  struct lib_globals *zg = cli->zg;
  struct zlog *zl;

  if (! zg->log)
    zl = zg->log_default;
  else
    zl = zg->log;

  zl->maskpri = ZLOG_DEBUG;

  return CLI_SUCCESS;
}

CLI (config_log_record_priority,
     config_log_record_priority_cli,
     "log record-priority",
     "Logging control",
     "Log the priority of the message within the message")
{
  struct lib_globals *zg = cli->zg;
  struct zlog *zl;

  if (! zg->log)
    zl = zg->log_default;
  else
    zl = zg->log;

  zl->record_priority = 1 ;

  return CLI_SUCCESS;
}

CLI (no_config_log_record_priority,
     no_config_log_record_priority_cli,
     "no log record-priority",
     CLI_NO_STR,
     "Logging control",
     "Do not log the priority of the message within the message")
{
  struct lib_globals *zg = cli->zg;
  struct zlog *zl;

  if (! zg->log)
    zl = zg->log_default;
  else
    zl = zg->log;

  zl->record_priority = 0 ;

  return CLI_SUCCESS;
}

/* Message lookup function.  */
char *
lookup (struct message *mes, s_int32_t key)
{
  struct message *pnt;

  for (pnt = mes; pnt->key != 0; pnt++)
    {
      if (pnt->key == key)
        {
          return pnt->str;
        }
    }
  return "";
}

/* Message lookup function.  Still partly used in bgpd and ospfd. */
char *
mes_lookup (struct message *meslist, s_int32_t max, s_int32_t index)
{
  if (index < 0 || index >= max)
    return "invalid";

  return meslist[index].str;
}

void
zlog_cli_init (struct cli_tree *ctree)
{
#ifdef PAL_LOG_STDOUT
  cli_install_gen (ctree, CONFIG_MODE, PRIVILEGE_MAX, 0,
                   &config_log_stdout_cli);
  cli_install_gen (ctree, CONFIG_MODE, PRIVILEGE_MAX, 0,
                   &no_config_log_stdout_cli);
#endif /* PAL_LOG_STDOUT. */

#ifdef PAL_LOG_FILESYS
#ifdef HAVE_VLOGD
  cli_install_gen (ctree, CONFIG_MODE, PRIVILEGE_NORMAL, 0,
                   &config_vlog_log_file_cli);
  cli_install_gen (ctree, CONFIG_MODE, PRIVILEGE_NORMAL, 0,
                   &no_config_vlog_log_file_cli);
#else
  cli_install_gen (ctree, CONFIG_MODE, PRIVILEGE_MAX, 0,
                   &config_log_file_cli);
  cli_install_gen (ctree, CONFIG_MODE, PRIVILEGE_MAX, 0,
                   &no_config_log_file_cli);
#endif /* !HAVE_VLOGD. */
#endif /* PAL_LOG_FILESYS. */

#ifdef PAL_LOG_SYSTEM
  cli_install_gen (ctree, CONFIG_MODE, PRIVILEGE_MAX, 0,
                   &config_log_syslog_cli);
  cli_install_gen (ctree, CONFIG_MODE, PRIVILEGE_MAX, 0,
                   &no_config_log_syslog_cli);
#endif /* PAL_LOG_SYSTEM. */

  cli_install_gen (ctree, CONFIG_MODE, PRIVILEGE_MAX, 0,
                   &config_log_trap_cli);
  cli_install_gen (ctree, CONFIG_MODE, PRIVILEGE_MAX, 0,
                   &no_config_log_trap_cli);

  cli_install_gen (ctree, CONFIG_MODE, PRIVILEGE_MAX, 0,
                   &config_log_record_priority_cli);
  cli_install_gen (ctree, CONFIG_MODE, PRIVILEGE_MAX, 0,
                   &no_config_log_record_priority_cli);
}

#ifdef HAVE_IMI
void
zlog_cli_init_imi (struct cli_tree *ctree)
{
#ifdef PAL_LOG_STDOUT
  cli_install_imi (ctree, CONFIG_MODE, PM_LOG, PRIVILEGE_MAX, 0,
                   &config_log_stdout_cli);
  cli_set_imi_cmd (&config_log_stdout_cli, CONFIG_MODE, CFG_DTYP_IMI_HOST);
  cli_install_imi (ctree, CONFIG_MODE, PM_LOG, PRIVILEGE_MAX, 0,
                   &no_config_log_stdout_cli);
#endif /* PAL_LOG_STDOUT. */

#ifdef PAL_LOG_FILESYS
#ifdef HAVE_VLOGD
  cli_install_imi (ctree, CONFIG_MODE, PM_VLOG, PRIVILEGE_NORMAL, 0,
                   &config_vlog_log_file_cli);
  cli_set_imi_cmd (&config_vlog_log_file_cli, CONFIG_MODE, CFG_DTYP_IMI_HOST);
  cli_install_imi (ctree, CONFIG_MODE, PM_VLOG, PRIVILEGE_NORMAL, 0,
                   &no_config_vlog_log_file_cli);
#else
  cli_install_imi (ctree, CONFIG_MODE, PM_LOG, PRIVILEGE_MAX, 0,
                   &config_log_file_cli);
  cli_set_imi_cmd (&config_log_file_cli, CONFIG_MODE, CFG_DTYP_IMI_HOST);
  cli_install_imi (ctree, CONFIG_MODE, PM_LOG, PRIVILEGE_MAX, 0,
                   &no_config_log_file_cli);
#endif /* !HAVE_VLOGD. */
#endif /* PAL_LOG_FILESYS. */

#ifdef PAL_LOG_SYSTEM
  cli_install_imi (ctree, CONFIG_MODE, PM_LOG, PRIVILEGE_MAX, 0,
                   &config_log_syslog_cli);
  cli_set_imi_cmd (&config_log_syslog_cli, CONFIG_MODE, CFG_DTYP_IMI_HOST);
  cli_install_imi (ctree, CONFIG_MODE, PM_LOG, PRIVILEGE_MAX, 0,
                   &no_config_log_syslog_cli);
#endif /* PAL_LOG_SYSTEM */

  cli_install_imi (ctree, CONFIG_MODE, PM_LOG, PRIVILEGE_MAX, 0,
                   &config_log_trap_cli);
  cli_set_imi_cmd (&config_log_trap_cli, CONFIG_MODE, CFG_DTYP_IMI_HOST);
  cli_install_imi (ctree, CONFIG_MODE, PM_LOG, PRIVILEGE_MAX, 0,
                   &no_config_log_trap_cli);
  cli_install_imi (ctree, CONFIG_MODE, PM_LOG, PRIVILEGE_MAX, 0,
                   &config_log_record_priority_cli);
  cli_set_imi_cmd (&config_log_record_priority_cli, CONFIG_MODE, CFG_DTYP_IMI_HOST);
  cli_install_imi (ctree, CONFIG_MODE, PM_LOG, PRIVILEGE_MAX, 0,
                   &no_config_log_record_priority_cli);
}
#endif /* HAVE_IMI. */


/* Encode zlog config into vector of strings. */
int
zlog_config_encode(struct lib_globals *zg, cfg_vect_t *cv)
{
  struct zlog *zl;

  pal_assert (zg != NULL);

  if (zg->log == NULL)
    zl = zg->log_default;
  else
    zl = zg->log;

  pal_assert (zl != NULL);

#ifdef PAL_LOG_STDOUT
  if (zl->flags & ZLOG_STDOUT)
    cfg_vect_add_cmd (cv, "log stdout\n");
#endif /* PAL_LOG_STDOUT. */

#if defined(PAL_LOG_SYSTEM) && ! defined(HAVE_IMI)
  if (zl->flags & ZLOG_SYSTEM)
    cfg_vect_add_cmd (cv, "log syslog\n");
#endif /* PAL_LOG_SYSTEM && ! HAVE_IMI. */

#ifdef PAL_LOG_FILESYS
#ifdef HAVE_VLOGD
  _vlog_encode_file_name(zg, cv);
#else
  if (zl->flags & ZLOG_FILE)
    cfg_vect_add_cmd (cv, "log file %s\n", zl->logfile);
#endif /* PAL_LOG_FILESYS. */
#endif /* HAVE_VLOGD */

  if (zl->maskpri != ZLOG_DEBUG)
    cfg_vect_add_cmd (cv, "log trap %s\n", zlog_priority[zl->maskpri]);

  if (zl->record_priority)
    cfg_vect_add_cmd (cv, "log record-priority\n");

  return 0;
}

/* Log config-write. */
int
zlog_config_write (struct cli *cli)
{
  struct lib_globals *zg = cli->zg;

#ifdef HAVE_IMI
  if (cli->zg->protocol != APN_PROTO_IMI)
    return 0;
#endif /* HAVE_IMI */

  /* Currently only PVR support logging. */
  if (cli->vr->id != 0)
    return 0;
  pal_assert (zg != NULL);

  cli->cv = cfg_vect_init(cli->cv);
  zlog_config_encode(zg, cli->cv);
  cfg_vect_out(cli->cv, (cfg_vect_out_fun_t)cli->out_func, cli->out_val);
  return 0;
}

/*//////////////////////////////////////////////////////////////////
 *      TESTING VLOGD
 */
#ifdef ZLOG_VLOGD_DEBUG

static int
_zlog_vlogd_test_cb (struct thread *thread)
{
  struct lib_globals *zg = THREAD_GLOB (thread);
  int    seq_num = (int)THREAD_ARG(thread);
  struct apn_vr *vr;
  int    vid;
  char  *vr_name;

  _zlog_vlogd_test_thread = NULL;

  for (vid = 0; vid < vector_max (zg->vr_vec); vid++)
  {
    if ((vr = vector_slot (zg->vr_vec, vid)))
    {
      zg->vr_in_cxt = vr;
      vr_name = (vr->name == NULL) ? "<PVR>" : vr->name;
      switch (seq_num % 8)
      {
      case ZLOG_ERROR:         /* Error. */
        zlog_err(zg, "Error from vr-name:%s vr-id:%d seq-num:%d",
                 vr_name, vr->id, seq_num);
        break;
      case ZLOG_WARN:          /* Warning. */
        zlog_warn(zg, "Warning from vr-name:%s vr-id:%d seq-num:%d",
                  vr_name, vr->id, seq_num);
        break;
      case ZLOG_INFO:          /* Informational. */
        zlog_info(zg, "Info from vr-name:%s vr-id:%d seq-num:%d",
                  vr_name, vr->id, seq_num);
      case ZLOG_DEBUG:          /* Debugging. */
        zlog_debug(zg, "Debug from vr-name:%s vr-id:%d seq-num:%d",
                  vr_name, vr->id, seq_num);
        break;
      default:
        zlog(zg, zg->log, 0, "Severity:%d from vr-name:%s vr-id:%d seq-num:%d",
             seq_num % 8, vr_name, vr->id, seq_num);
        break;
      }
    }
  }
  THREAD_TIMER_ON(zg,
                  _zlog_vlogd_test_thread,
                  _zlog_vlogd_test_cb,
                  (void *)(seq_num+1),  /* incr seq # */
                  1);         /* timeout */
  return 0;
}

static void
_zlog_vlogd_start_test(struct lib_globals *zg)
{
  /* Do not do that for VLOGD. */
  if (zg->protocol == APN_PROTO_VLOG)
    return;

  /* Every 5 seconds we will generate one log message for
     every VR in the vr_vec.
   */
  THREAD_TIMER_ON(zg,
                  _zlog_vlogd_test_thread,
                  _zlog_vlogd_test_cb,
                  (void *)1,  /* seq # */
                  1);         /* timeout */
}

#endif /* ZLOG_VLOGD_DEBUG */

