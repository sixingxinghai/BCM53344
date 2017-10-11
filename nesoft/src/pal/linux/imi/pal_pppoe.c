/* Copyright (C) 2002,   All Rights Reserved. */

/* pal_pppoe.c -- PacOS PAL PPPoE operations definitions
                 for Linux */

/* Include files */
#include "pal.h"

#ifdef HAVE_PPPOE
#include "imi/pal_pppoe.h"
#include "pal_file.h"
#include "log.h"
#include "imi/imi.h"

#ifdef HAVE_IMI_SYSCONFIG
/* Read startup config. */
static result_t
pal_pppoe_read_startup_config (struct lib_globals *lib_node,
                              struct imi_pppoe *pppoe)
{
  char buf[PAL_FILE_DEFAULT_LINELEN];
  char buf_cmd[20];
  result_t ret = RESULT_ERROR;
  char *cp, *start;
  int cmdlen;
  FILE *fp;

  /* Open file (/etc/ppp/pppoe.conf). */
  fp = pal_fopen (PPPOE_FILE, PAL_OPEN_RO);
  if (fp == NULL)
    return IMI_API_FILE_OPEN_ERROR;

  /* Seek to start of file. */
  ret = pal_fseek (fp, 0, PAL_SEEK_SET);
  if (ret < 0)
    {
      pal_fclose (fp);
      return IMI_API_FILE_SEEK_ERROR;
    }

  /* Read line-by-line. */
  while (pal_fgets (buf, PAL_FILE_DEFAULT_LINELEN, fp)
         != NULL)
    {
      cp = buf;
      ret = RESULT_ERROR;

      /* Skip white spaces. */
      while (pal_char_isspace ((int) * cp) && *cp != '\0')
        cp++;

      /* If only white spaces, continue to next line. */
      if (*cp == '\0')
        continue;

      /* If this comment or empty line, continue to next line. */
      if ((buf[0] == '#') || (buf[0] == '\n') || buf[0] == '\r')
        continue;

      /* Read first word (command) into buffer. */
      start = cp;
      while (!
             (pal_char_isspace ((int) * cp) || *cp == '\r'
              || *cp == '\n') && *cp != '\0')
        cp++;
      cmdlen = cp - start;
      pal_strncpy (buf_cmd, start, cmdlen);
      buf_cmd[cmdlen] = '\0';

      /* Skip white spaces. */
      while (pal_char_isspace ((int) * cp) && *cp != '\0')
        cp++;

      /* If no argument, it's an error. */
      if (*cp == '\0')
        {
          /* Unless it's '}' to exit pool configuration. */
          if (buf_cmd[0] != '}')
            {
              zlog_warn (lib_node, "%s : Invalid line : %s\n", PPPOE_FILE, buf);
              continue;
            }
        }

      /* Disable PPPOE while we reconfigure to avoid overwriting the file. */
      imi_pppoe_disable ();

      switch (ret)
        {
        case IMI_API_SUCCESS:
          /* Set flag to inform IMI that system config is present. */
          pppoe->sysconfig = PAL_TRUE;
          break;
        case IMI_API_UNKNOWN_CMD_ERROR:
          zlog_warn (lib_node, "%s : Unknown command > %s\n", PPPOE_FILE, buf);
          break;
        case IMI_API_DUPLICATE_ERROR:
          zlog_warn (lib_node, "%s : Duplicate > %s\n", PPPOE_FILE, buf);
          break;
        case IMI_API_IPV4_ERROR:
          zlog_warn (lib_node, "%s : Invalid prefix/mask > %s\n", 
                     PPPOE_FILE, buf);
          break;
        case IMI_API_CANT_SET_ERROR:
          zlog_warn (lib_node, "%s : Set Error > %s\n", PPPOE_FILE, buf);
          break;
        case RESULT_ERROR:
        default:
          /* Unknown command. */
          zlog_warn (lib_node, "%s : Invalid command > %s\n", PPPOE_FILE, buf);
          break;
        }
    }

  /* Close filehandle. */
  pal_fclose (fp);

  return RESULT_OK;
}
#endif /* HAVE_IMI_SYSCONFIG */

/* Init. */
pal_handle_t
pal_pppoe_start (struct lib_globals *lib_node, struct imi_pppoe *pppoe)
{
#ifdef HAVE_IMI_SYSCONFIG
  /* Read pppoe.conf at startup if enabled. */
  pal_pppoe_read_startup_config (lib_node, pppoe); 
#endif /* HAVE_IMI_SYSCONFIG */

  return (pal_handle_t) 1;
}

/* Shutdown. */
result_t
pal_pppoe_stop (struct lib_globals *lib_node, pal_handle_t pal_pppoe)
{
  char buf[255];

  pal_snprintf (buf, 255, "%s %s\n", PPPOE_CLIENT_APP_STOP, PPPOE_FILE);
#ifndef DEBUG
  system (buf);
#else
  printf ("%s\n", buf);
#endif

  return RESULT_OK;
}

/* Restart pppoe. */
static void
pal_pppoe_restart_protocol (struct lib_globals *lib_node,
                            struct imi_pppoe *pppoe)
{
  char buf[255];
#ifndef DEBUG
  int i = 0;
  char *argv[10];
#endif /* DEBUG */

  /* Enabled? */
  if (pppoe->enabled)
    {
      /* Execute */
#ifndef DEBUG
      argv[i++] = PPPOE_CLIENT_APP;
      argv[i] = 0;
      pal_daemonize_program (PPPOE_CLIENT_APP, argv, 0, 0);
#else
      printf ("%s\n", PPPOE_CLIENT_APP);
#endif /* DEBUG */
    }
  else
    if (pppoe->disabling)
      {
        pal_snprintf (buf, 255, "%s %s\n", PPPOE_CLIENT_APP_STOP, PPPOE_FILE);
#ifndef DEBUG
        system (buf);
#else
        printf ("%s\n", buf);
#endif /* DEBUG */
      }
}

/* Refresh Linux PPPOE. */
result_t
pal_pppoe_refresh (struct lib_globals *lib_node, struct imi_pppoe *pppoe)
{
  int ret;
  char buf[PAL_FILE_DEFAULT_LINELEN];
  FILE *fp;

#ifdef HAVE_IMI_SYSCONFIG
  if (pppoe->shutdown_flag)
    return RESULT_OK;
#endif /* HAVE_IMI_SYSCONFIG */

  /* 
    If disabling PPPoE, must call restart function first because adsl-stop
    requires values contained in /etc/ppp/pppoe.conf.
  */
  if (pppoe->disabling)
  {
    pal_pppoe_restart_protocol (lib_node, pppoe);
    return RESULT_OK;
  }

  /* Open file (/etc/ppp/pppoe.conf). */
  fp = pal_fopen (PPPOE_FILE, PAL_OPEN_RW);
  if (fp == NULL)
    return IMI_API_FILE_OPEN_ERROR;

  /* Seek to start of file. */
  ret = pal_fseek (fp, 0, PAL_SEEK_SET);
  if (ret < 0)
    {
      pal_fclose (fp);
      return IMI_API_FILE_SEEK_ERROR;
    }

  /* Is PPPOE service enabled? */
  if ((pppoe->enabled) && (!pppoe->shutdown_flag))
    {
      ret = pal_fputs (PPPOE_CONF_COPYRIGHT_TEXT, fp);
      ret = pal_fputs (PPPOE_CONF_ETH_TEXT, fp);
      /* Write configured interface. */
      sprintf (buf, "ETH=%s\n", if_kernel_name (pppoe->ifp));
      pal_fputs (buf, fp);

      ret = pal_fputs (PPPOE_CONF_USERNAME_TEXT, fp);
      /* Write configured username. */
      sprintf (buf, "USER=%s\n", pppoe->username);
      pal_fputs (buf, fp);

      ret = pal_fputs (PPPOE_CONF_DEMAND_TEXT, fp);
      ret = pal_fputs (PPPOE_CONF_DNS_TEXT, fp);
      ret = pal_fputs (PPPOE_CONF_DEFAULTROUTE_TEXT, fp);
      ret = pal_fputs (PPPOE_CONF_EXPERT_TEXT, fp);
    }

  /* Check write result. */
  if (ret == PAL_EOF)
    {
      pal_fclose (fp);
      return IMI_API_FILE_WRITE_ERROR;
    }

  /* Close filehandle. */
  pal_fclose (fp);

  /* Open file (/etc/ppp/pap-secrets). */
  fp = pal_fopen (PPPOE_PAP_SECRETS_FILE, PAL_OPEN_RW);
  if (fp == NULL)
    return IMI_API_FILE_OPEN_ERROR;

  /* Seek to start of file. */
  ret = pal_fseek (fp, 0, PAL_SEEK_SET);
  if (ret < 0)
    {
      pal_fclose (fp);
      return IMI_API_FILE_SEEK_ERROR;
    }

  /* Is PPPOE service enabled? */
  if ((pppoe->enabled) && (!pppoe->shutdown_flag))
    {
      ret = pal_fputs (PPPOE_PAP_SECRETS_TOP_FILE_TEXT, fp);
      /* Write configured username & passwd. */
      sprintf (buf, "\"%s\"   *       \"%s\"\n", pppoe->username, pppoe->passwd);
      pal_fputs (buf, fp);
    }

  /* Check write result. */
  if (ret == PAL_EOF)
    {
      pal_fclose (fp);
      return IMI_API_FILE_WRITE_ERROR;
    }

  /* Close filehandle. */
  pal_fclose (fp);

  /* Do the same for chap. Open file (/etc/ppp/chap-secrets). */
  fp = pal_fopen (PPPOE_CHAP_SECRETS_FILE, PAL_OPEN_RW);
  if (fp == NULL)
    return IMI_API_FILE_OPEN_ERROR;

  /* Seek to start of file. */
  ret = pal_fseek (fp, 0, PAL_SEEK_SET);
  if (ret < 0)
    {
      pal_fclose (fp);
      return IMI_API_FILE_SEEK_ERROR;
    }

  /* Is PPPOE service enabled? */
  if ((pppoe->enabled) && (!pppoe->shutdown_flag))
    {
      ret = pal_fputs (PPPOE_CHAP_SECRETS_TOP_FILE_TEXT, fp);
      /* Write configured username & passwd. */
      sprintf (buf, "\"%s\"   *       \"%s\"\n", pppoe->username, pppoe->passwd);
      pal_fputs (buf, fp);
    }

  /* Check write result. */
  if (ret == PAL_EOF)
    {
      pal_fclose (fp);
      return IMI_API_FILE_WRITE_ERROR;
    }

  /* Close filehandle. */
  pal_fclose (fp);

  /* Restart the protocol. */
  if (pppoe->enabled)
    pal_pppoe_restart_protocol (lib_node, pppoe);

  return RESULT_OK;
}

#endif /* HAVE_PPPOE */
