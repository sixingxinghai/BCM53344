/* Copyright (C) 2003,   All Rights Reserved. */
#include "pal.h"
#include <sys/types.h>
#include <dirent.h>

#include "pal_pm.h"

/* Get numeric pid. */
static int
pal_which_number (char *s)
{
  int len, i;

  len = strlen (s);

  for (i = 0; i < len; i++)
    if ((s[i] < '0' || s[i] > '9'))
     return -1;

  return atoi (s);
}

/* Get protocol daemon name. */
static char *
pal_imi_get_proto_daemon_name (module_id_t protocol)
{
  switch (protocol)
    {
    case APN_PROTO_NSM:  return "nsm";
    case APN_PROTO_RIP: return "ripd";
    case APN_PROTO_RIPNG: return "ripngd";
    case APN_PROTO_OSPF: return "ospfd";
    case APN_PROTO_OSPF6: return "ospf6d";
    case APN_PROTO_ISIS: return "isisd";
    case APN_PROTO_BGP: return "bgpd";
    case APN_PROTO_LDP: return "ldpd";
    case APN_PROTO_RSVP: return "rsvpd";
    case APN_PROTO_PIMDM: return "pdmd";
    case APN_PROTO_PIMSM: return "pimd";
    case APN_PROTO_DVMRP: return "dvmrpd";
    case APN_PROTO_STP: return "stpd";
    case APN_PROTO_RSTP: return "rstpd";
    case APN_PROTO_8021X: return "authd";
    case APN_PROTO_ONM :  return "onmd";
    case APN_PROTO_ELMI :  return "elmid";
    case APN_PROTO_PIMPKTGEN: return "pimpktgend";
    case APN_PROTO_LMP: return "lmpd";
    default:
      return "";
    }
}

/* Get pid of protocol. */
int
pal_get_pid (module_id_t protocol)
{
 DIR *dp;
  struct dirent *dir;
  char buf[100], line[1024], tag[100], name[100];
  int pid;
  FILE *fp;
  char *str;

  str = pal_imi_get_proto_daemon_name (protocol);

  dp = opendir ("/proc");
  if (! dp)
   return -1;

  while ((dir = readdir (dp)))
    {
      pid = pal_which_number (dir->d_name);

      if (pid == -1)
        continue;

      /* Open /proc/pid/status file. */   
      snprintf (buf, 100, "/proc/%d/status", pid);
      fp = fopen (buf, "r");
      if (fp == NULL)
        continue;

      /* Get first line with name. */
      fgets (line, 1024, fp);

      /* Close stream. */
      fclose (fp);

      sscanf (line, "%s %s", tag, name);
      if (! strcmp (name, str))
        {
          closedir (dp);
          return pid;
        }
    }

  closedir (dp);
  return -1;
}

/* Stop the protocol daemon. */
int
pal_imi_stop_pm (module_id_t protocol)
{
  char buf[50];
  char *killall = "killall";
  char *str;

  str = pal_imi_get_proto_daemon_name (protocol);
  snprintf (buf, 50, "%s %s", killall, str);

  system(buf);

  return 0;
}

/* Start the protocol daemon. */
int
pal_imi_start_pm (module_id_t protocol)
{
  char buf[256];
  char *str;
  char *argv[2];
  int i = 0;
  pid_t pid;

  str = pal_imi_get_proto_daemon_name (protocol);
  snprintf (buf, 256, "%s/%s", PACOS_BIN_PATH, str);

  /* First stop the protocol daemon. */
  pal_imi_stop_pm  (protocol);

  /* then start it. */
  argv[i++] = buf;
  argv[i] = 0;

  /* Start process. */
  pal_daemonize_program (buf, argv, 0, 0);

  /* Now get the pid. */
  pid = pal_get_pid (protocol);
  if (pid <= 0)
    return -1;
  else
    return pid;
}

/* Process exists?
   Returns 0 if not-exist, 1 for exist. */
int
pal_imi_pid_exists (int pid)
{
  if (kill (pid, 0) == -1 && errno == ESRCH)
    return 0;
  else
    return 1;
}

