/* Copyright (C) 2002,   All Rights Reserved. */

#ifndef _PAL_SYSLOG_H
#define _PAL_SYSLOG_H

/* Syslog PID. */
#define SYSLOG_PIDFILE          "/var/run/syslogd.pid"

/* Syslog configuration file(s). */
#define SYSLOG_FILE             "/etc/syslog.conf"
#define SYSLOG_FILE_TMP         "/etc/syslog.tmp"

/* Strings. */
#define SYSLOG_PACOS_START      "# pacos-start\n"
#define SYSLOG_PACOS_STOP       "# pacos-stop\n"
#define SYSLOG_SELECTOR_STR     "user"
#define SYSLOG_ACTION_STR       "debug"

void pal_syslog_restart (void);

#endif /* _PAL_SYSLOG_H */
