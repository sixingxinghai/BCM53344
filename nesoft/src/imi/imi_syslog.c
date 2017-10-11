/* Copyright (C) 2004  All Rights Reserved. */

#include "pal.h"

#ifdef HAVE_IMISH

#include "lib.h"
#include "line.h"
#include "message.h"

#include "imi/imi.h"
#include "imi/imi_server.h"


/* Write PacOS information. */
void
imi_syslog_write (FILE *fp, struct imi_server *is)
{
  struct imi_server_entry *ise;
  struct imi_server_client *isc;

  /* Write pacos-start. */
  pal_fprintf (fp, "%s", SYSLOG_PACOS_START);

  /* Write lines. */
  if ((isc = vector_slot (is->client, APN_PROTO_IMISH)))
    for (ise = isc->head; ise; ise = ise->next)
      if (CHECK_FLAG (ise->line.flags, LINE_FLAG_MONITOR))
        if (ise->line.tty)
          pal_fprintf (fp, "%s.%s\t\t\t\t\t%s\n",
                       SYSLOG_SELECTOR_STR, SYSLOG_ACTION_STR,
                       ise->line.tty);

  /* Write pacos-stop. */
  pal_fprintf (fp, "%s", SYSLOG_PACOS_STOP);
}

/* Refresh /etc/syslog.conf */
int
imi_syslog_refresh (struct imi_server *is)
{
  FILE *fp, *fp_tmp;
  char buf[512];
  bool_t skip_flag;
  int written = PAL_FALSE;

  /* Open file (/etc/syslog.conf). */
  fp = pal_fopen (SYSLOG_FILE, PAL_OPEN_RO);
  if (fp == NULL)
    return -1;

  /* Open temporary file. */
  fp_tmp = pal_fopen (SYSLOG_FILE_TMP, PAL_OPEN_RW);
  if (fp_tmp == NULL)
    {
      pal_fclose (fp);
      return -1;
    }

  /* Seek for pacos-start. */
  skip_flag = PAL_FALSE;
  while (pal_fgets (buf, sizeof buf, fp) != NULL)
    {
      /* Skip rewriting old PacOS data. */ 
      if (skip_flag == PAL_TRUE)
        {
          if (! pal_strncmp (buf, SYSLOG_PACOS_STOP,
                             pal_strlen (SYSLOG_PACOS_STOP)))
            skip_flag = PAL_FALSE;
          continue;
        }
      else if (! pal_strncmp (buf, SYSLOG_PACOS_START,
                              pal_strlen (SYSLOG_PACOS_START)))
        {
          skip_flag = PAL_TRUE;
          continue;
        }

      /* PacOS must write all terminals before lines starting with '!'. */
      if (buf[0] == '!' && written != PAL_TRUE)
        {
          /* Write PacOS terminals. */
          imi_syslog_write (fp_tmp, is);
          written = PAL_TRUE;
        }

      /* Write the current line.  */
      pal_fprintf (fp_tmp, "%s", buf);
    }

  /* Write PacOS terminals. */
  if (written == PAL_FALSE)
    imi_syslog_write (fp_tmp, is);

  /* Close files. */
  pal_fclose (fp);
  pal_fclose (fp_tmp);

  /* Move syslog.tmp -> syslog.conf. */
  pal_rename (SYSLOG_FILE_TMP, SYSLOG_FILE);

  /* Restart syslogd.  */
  pal_syslog_restart ();

  return 0;
}

#endif /* HAVE_IMISH */
