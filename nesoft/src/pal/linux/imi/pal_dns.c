/* Copyright (C) 2002,   All Rights Reserved. */

/* pal_dns.c -- PacOS PAL DNS operations definitions
                for Linux */

/* Include files. */
#include "pal.h"

#ifdef HAVE_DNS
#include "imi/pal_dns.h"
#include "pal_file.h"
#include "log.h"
#include "imi/imi.h"

/* Functions */
#ifdef HAVE_IMI_SYSCONFIG

/* IMI prototypes used to load resolv.conf. */
int imi_dns_disable ();
int imi_dns_set_default_domain (char *);
int imi_dns_domain_list_add (char *);
int imi_dns_name_server_add (char *);

/* Read startup config. */
static result_t
pal_dns_read_startup_config (struct lib_globals *lib_node,
                             struct imi_dns *dns)
{
  char buf[PAL_FILE_DEFAULT_LINELEN];
  char buf_cmd[10];                             /* strlen("nameserver"). */
  char buf_arg[PAL_FILE_DEFAULT_LINELEN-7];     /* 7=strlen("domain "). */
  result_t ret = CLI_SUCCESS;
  const char *domain = "domain";
  const char *search = "search";
  const char *ns = "nameserver";
  char *cp, *start;
  int cmdlen;
  int arglen;
  FILE *fp;

  /* Open file (/etc/resolv.conf). */
  fp = pal_fopen (DNS_FILE, PAL_OPEN_RO);
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
          zlog_warn (lib_node, "%s : Invalid line : %s\n", DNS_FILE, buf);
          continue;
        }

      /* Read second word (argument) into buffer. */
      start = cp;
      while (!
             (pal_char_isspace ((int) * cp) || *cp == '\r'
              || *cp == '\n') && *cp != '\0')
        cp++;
      arglen = cp - start;
      pal_strncpy (buf_arg, start, arglen);
      buf_arg[arglen] = '\0';

      /* Disable DNS while we reconfigure to avoid overwriting the file. */
      imi_dns_disable ();

      /* command is 'domain' */
      if (cmdlen >= pal_strlen (domain))
        if (! pal_strncmp (buf_cmd, domain, pal_strlen (domain)))
          ret = imi_dns_set_default_domain (buf_arg);

      /* command is 'search' */
      if (cmdlen >= pal_strlen (search))
        if (! pal_strncmp (buf_cmd, search, pal_strlen (search)))
          ret = imi_dns_domain_list_add (buf_arg);

      /* command is 'nameserver' */
      if (cmdlen >= pal_strlen (ns))
        if (! pal_strncmp (buf_cmd, ns, pal_strlen (ns)))
          ret = imi_dns_name_server_add (buf_arg);

      switch (ret)
        {
        case IMI_API_SUCCESS:
          /* Set flag to inform IMI that system config is present. */
          dns->sysconfig = PAL_TRUE;
          break;
        case IMI_API_DUPLICATE_ERROR:
          zlog_warn (lib_node, "%s : Duplicate > %s\n", DNS_FILE, buf);
          break;
        case IMI_API_IPV4_ERROR:
          zlog_warn (lib_node, "%s : Invalid prefix > %s\n", DNS_FILE, buf);
          break;
        case RESULT_ERROR:
        default:        
          /* Unknown command. */
          zlog_warn (lib_node, "%s : Invalid command > %s\n", DNS_FILE, buf);
          break;
        }
    }

  /* Close filehandle. */
  pal_fclose (fp);

  return ret;
}
#endif /* HAVE_IMI_SYSCONFIG */

/* Init. */
pal_handle_t
pal_dns_start (struct lib_globals *lib_node, struct imi_dns *dns)
{
#ifdef HAVE_IMI_SYSCONFIG
  /* Read resolv.conf at startup if enabled. */
  pal_dns_read_startup_config (lib_node, dns);
#endif /* HAVE_IMI_SYSCONFIG */

  return (pal_handle_t) 1;
}

/* Shutdown. */
result_t
pal_dns_stop (struct lib_globals *lib_node, pal_handle_t pal_dns)
{
  return RESULT_OK;
}

/* Refresh Linux DNS. */
result_t
pal_dns_refresh (struct lib_globals *lib_node, struct imi_dns *dns)
{
  struct listnode *node;
  char *search = "search";
  struct prefix_ipv4 *p;
  char *tmp;
  int ret;
  char buf[PAL_FILE_DEFAULT_LINELEN];
  char buf_ipv4[IPV4_ADDR_STRLEN];
  FILE *fp;

#ifdef HAVE_IMI_SYSCONFIG
  /* If shutdown, don't overwrite file. */
  if (dns->shutdown_flag)
    return RESULT_OK;
#endif /* HAVE_IMI_SYSCONFIG */

  /* Open file (/etc/resolv.conf). */
  fp = pal_fopen (DNS_FILE, PAL_OPEN_RW);
  if (fp == NULL)
    return IMI_API_FILE_OPEN_ERROR;

  /* Seek to start of file. */
  ret = pal_fseek (fp, 0, PAL_SEEK_SET);
  if (ret < 0)
    {
      pal_fclose (fp);
      return IMI_API_FILE_SEEK_ERROR;
    }

  /* Is DNS enabled? */
  if ((dns->enabled) && (!dns->shutdown_flag))
    {
      /* Write top line. */
      ret = pal_fputs (DNS_TOP_FILE_TEXT, fp);

      /* Write default domain. */
      if (ret != PAL_EOF)
        ret = pal_fputs (DNS_DOMAIN_DFLT_TEXT, fp);

      if (dns->default_domain != NULL)
        {
          sprintf (buf, "domain %s\n", dns->default_domain);
          if (ret != PAL_EOF)
            ret = pal_fputs (buf, fp);
        }

      /* Write domain search list. */
      if (ret != PAL_EOF)
        ret = pal_fputs (DNS_DOMAIN_LIST_TEXT, fp);

      if (listcount (dns->domain_list) > 0)
        {
          ret = pal_fputs (search, fp);
          node = LISTHEAD (dns->domain_list);
          while (node && (ret != PAL_EOF))
            {
              tmp = GETDATA (node);
              sprintf (buf, " %s", tmp);
              ret = pal_fputs (buf, fp);
              NEXTNODE (node);
            }
          sprintf (buf, "\n");
          ret = pal_fputs (buf, fp);
        }

      /* Write name server(s). */
      if (ret != PAL_EOF)
        ret = pal_fputs (DNS_NAME_SERVER_TEXT, fp);

      if ((ret != PAL_EOF) && (listcount (dns->ns_list) > 0))
        {
          for (node = LISTHEAD (dns->ns_list); node; NEXTNODE (node))
            {
              p = GETDATA (node);
              pal_inet_ntoa (p->prefix, buf_ipv4);
              sprintf (buf, "nameserver %s\n", buf_ipv4);
              ret = pal_fputs (buf, fp);
            }
        }
    }
  else
    {
      /* Write disabled message to resolv.conf instead of configuration. */
      ret = pal_fputs (DNS_DISABLE_FILE_TEXT, fp);
    }

  /* Check write result. */
  if (ret == PAL_EOF)
    {
      pal_fclose (fp);
      return IMI_API_FILE_WRITE_ERROR;
    }

  /* Close filehandle. */
  pal_fclose (fp);

  return RESULT_OK;
}
#endif /* HAVE_DNS */
