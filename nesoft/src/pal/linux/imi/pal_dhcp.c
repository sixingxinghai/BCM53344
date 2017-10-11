/* Copyright (C) 2002,   All Rights Reserved. */

/* pal_dhcp.c -- PacOS PAL DHCP operations definitions
                 for Linux */

/* Include files */
#include "pal.h"

#if defined(HAVE_DHCP_CLIENT) || defined(HAVE_DHCP_SERVER)
#include "pal_file.h"
#include "imi/pal_dhcp.h"
#include "log.h"
#include "imi/imi.h"

/* IMI prototypes used to load dhcpd.conf. */
int imi_dhcps_pool (char *id_str);
int imi_dhcps_network_set (struct imi_dhcp_pool *pool,
                           char *network_str,
                           char *mask_str);
int imi_dhcps_domain_set (struct imi_dhcp_pool *pool,
                          char *domain_name);
int imi_dhcps_dns_add (struct imi_dhcp_pool *pool,
                       char *dns_str);
int imi_dhcps_default_router_add (struct imi_dhcp_pool *pool,
                                  char *router_str);
int imi_dhcps_pool_range_add (struct imi_dhcp_pool *pool,
                              char *start,
                              char *end);
int imi_dhcps_lease_set (struct imi_dhcp_pool *pool,
                         char *days_str,
                         char *hrs_str,
                         char *mins_str);
int imi_dhcps_disable ();
struct imi_dhcp_pool *imi_dhcps_pool_lookup (char *name,
                                             struct list *pool_list);
#endif /* defined(HAVE_DHCP_CLIENT) || defined(HAVE_DHCP_SERVER) */

#ifdef HAVE_DHCP_CLIENT
/*-------------------------------------------------------------------------
  DHCP Client - dhcpcd  */

/* Execute dhcp client for interface. */
result_t
pal_dhcp_client_start (struct lib_globals *lib_node,
                       struct imi_dhcp_client *idc)
{
  int ret = RESULT_OK;
  char *argv[10];
#ifndef DEBUG

  int i = 0;
  argv[i++] = DHCP_CLIENT_APP;
  if (idc->clientid)
    {
      argv[i++] = DHCP_OPT_CLIENTID;
      argv[i++] = idc->clientid->name;
    }
  if (idc->hostname)
    {
      argv[i++] = DHCP_OPT_HOSTNAME;
      argv[i++] = idc->hostname;
    }
  argv[i++] = DHCP_OPT_CONFIGDIR;
  argv[i++] = DHCP_CONFIGDIR_PATH;
  argv[i++] = if_kernel_name (idc->ifp);
  argv[i] = 0;
  pal_daemonize_program (DHCP_CLIENT_APP, argv, 0, 0);
#else
  fprintf (stderr, "%s\n", DHCP_CLIENT_APP);
#endif /* DEBUG */

  return ret;
}

/* Stop dhcp client for an interface. */
result_t
pal_dhcp_client_stop (struct lib_globals* lib_node,
struct imi_dhcp_client *idc)
{
  char buf[255];
  char pid[255];
  size_t bytes = 255;
  char pidfile[128];
  FILE *fp;

  pal_snprintf (pidfile, 128, DHCP_CLIENT_PID_FILE, if_kernel_name (idc->ifp));

  fp = pal_fopen (pidfile, PAL_OPEN_RO);
  if (fp != NULL)
    {
      /* Read PID. */
      pal_fgets (pid, bytes, fp);

      /* Close file handle. */
      pal_fclose (fp);

      /* Kill existing daemon and remove PID file. */
      pal_snprintf (buf, 255, "kill %s", pid);
#ifndef DEBUG
      system (buf); 
#else
      printf ("%s\n", buf);
#endif /* DEBUG */

      /* Remove PID file. */
      pal_snprintf (buf, 255, "rm -f %s", pidfile);
#ifndef DEBUG
      system (buf);
#else
      printf ("%s\n", buf);
#endif /* DEBUG */

/* Make the interface up */
      pal_snprintf (buf, 255, "ifconfig %s up", if_kernel_name (idc->ifp));
#ifndef DEBUG
      system(buf);
#else
      printf ("%s\n", buf);
#endif /* DEBUG */
    }

  return RESULT_OK;
}
#endif /* HAVE_DHCP_CLIENT */

#ifdef HAVE_DHCP_SERVER
/*-------------------------------------------------------------------------
  DHCP Server - dhcpd  */ 

#ifdef HAVE_IMI_SYSCONFIG

/* Read 'subnet' line from /etc/dhcpd.conf:
   - 'cp' points to second argument (prefix). */
static result_t
pal_dhcp_read_line_subnet (struct lib_globals *lib_node,
                           struct imi_dhcp *dhcp,
                           char *buf,
                           char *cp,
                           struct imi_dhcp_pool **pool)
{
  char *start;
  char buf_pref[IPV4_ADDR_STRLEN+1];
  char buf_mask[IPV4_ADDR_STRLEN+1];
  const char *netmask = "netmask";
  struct imi_dhcp_pool *new_pool;
  char buf_cmd[7];                              /* strlen("netmask"). */
  int cmdlen;
  int arglen;
  result_t ret;

  /* Read second word (argument) into buffer. */
  start = cp;
  while (!
         (pal_char_isspace ((int) * cp) || *cp == '\r'
          || *cp == '\n') && *cp != '\0')
    cp++;
  arglen = cp - start;
  if (arglen > IPV4_ADDR_STRLEN+1)
    return IMI_API_IPV4_ERROR;
  else
    {
      pal_strncpy (buf_pref, start, arglen);
      buf_pref[arglen] = '\0';
    }

  /* Skip white spaces. */
  while (pal_char_isspace ((int) * cp) && *cp != '\0')
    cp++;

  /* If no more arguments, it's an error. */
  if (*cp == '\0')
    return IMI_API_UNKNOWN_CMD_ERROR;

  /* Read third word ('netmask') into buffer. */
  start = cp;
  while (!
         (pal_char_isspace ((int) * cp) || *cp == '\r'
          || *cp == '\n') && *cp != '\0')
    cp++;
  cmdlen = cp - start;
  pal_strncpy (buf_cmd, start, cmdlen);
  buf_cmd[cmdlen] = '\0';

  /* Third word must be 'netmask' or it's an error. */
  if (cmdlen == pal_strlen (netmask))
    {
      if (! pal_strncmp (buf_cmd, netmask, pal_strlen (netmask)))
        {
          /* Skip white spaces. */
          while (pal_char_isspace ((int) * cp) && *cp != '\0')
            cp++;

          /* If no more arguments, it's an error. */
          if (*cp == '\0')
            return IMI_API_UNKNOWN_CMD_ERROR;

          /* Read fourth argument into mask buffer. */
          start = cp;
          while (!
                 (pal_char_isspace ((int) * cp) || *cp == '\r'
                  || *cp == '\n') && *cp != '\0')
            cp++;
          arglen = cp - start;
          if (arglen > IPV4_ADDR_STRLEN+1)
            return IMI_API_IPV4_ERROR;
          else
            {
              pal_strncpy (buf_mask, start, arglen);
              buf_mask[arglen] = '\0';
            }

          /* Create DHCP pool using prefix as id. */
          ret = imi_dhcps_pool (buf_pref);
          if (ret != IMI_API_SUCCESS)
            return ret;

          /* Set pool handle. */
          new_pool = imi_dhcps_pool_lookup (buf_pref, dhcp->pool_list);
          if (new_pool)
            *pool = new_pool;
          else
            *pool = NULL;

          /* Set network & mask. */
          ret = imi_dhcps_network_set ((*pool), buf_pref, buf_mask);
          if (ret != IMI_API_SUCCESS)
            return ret;
        }
    }
  else
    return IMI_API_UNKNOWN_CMD_ERROR;

  return IMI_API_SUCCESS;
}

/* Read 'option' line from /etc/dhcpd.conf:
   - 'cp' points to second argument (command). */
static result_t
pal_dhcp_read_line_option (struct lib_globals *lib_node,
                           struct imi_dhcp *dhcp,
                           char *buf,
                           char *cp,
                           struct imi_dhcp_pool *pool)
{
  char *start;
  char buf_ipv4[IPV4_ADDR_STRLEN+1];
  char buf_arg[PAL_FILE_DEFAULT_LINELEN];
  const char *domain = "domain-name";
  const char *dns = "domain-name-servers";
  const char *routers = "routers";
  char buf_cmd[19];                        /* strlen("domain-name-servers"). */
  int cmdlen;
  int arglen;
  result_t ret = IMI_API_SUCCESS;

  /* Read second word (argument) into buffer. */
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

  /* If no more arguments, it's an error. */
  if (*cp == '\0')
    return IMI_API_UNKNOWN_CMD_ERROR;

  /* command is 'domain-name' */
  if (cmdlen == pal_strlen (domain))
    if (! pal_strncmp (buf_cmd, domain, pal_strlen (domain)))
      {
        /* Read domain name into buffer. */
        start = cp;
        while (! 
               (pal_char_isspace ((int) * cp) || *cp == '\r'
                || *cp == '\n') && *cp != '\0')
          cp++;
        arglen = cp - start; 
        pal_strncpy (buf_arg, start, arglen);
        buf_arg[arglen] = '\0';

        /* dhcpd.conf: "abc.com";
           IMI: abc.com    */
        if ((buf_arg[0] != '"') || (buf_arg[arglen-2] != '"'))
          return IMI_API_UNKNOWN_CMD_ERROR;

        /* Recopy the string if it's proper syntax. */
        pal_strncpy (buf_arg, start+1, arglen-3);
        buf_arg[arglen-3] = '\0';

        /* Call IMI API. */
        ret = imi_dhcps_domain_set (pool, buf_arg);
      }

  /* command is 'domain-name-servers' */
  if (cmdlen == pal_strlen (dns))
    if (! pal_strncmp (buf_cmd, dns, pal_strlen (dns)))
      {
        ret = IMI_API_SUCCESS;

        /* Read list of dns servers. */
        while ((*cp != '\0') && (ret == IMI_API_SUCCESS))
          {
            /* Read ipv4 into buffer. */
            start = cp;
            while (!
                   (pal_char_isspace ((int) * cp) || *cp == '\r'
                    || *cp == '\n') && *cp != '\0')
              cp++;
            arglen = cp - start;
            if (arglen > IPV4_ADDR_STRLEN+1)
              return IMI_API_IPV4_ERROR;
            else
              {
                pal_strncpy (buf_ipv4, start, arglen);
                if (buf_ipv4[arglen-1] == ';' ||
                    buf_ipv4[arglen-1] == ',')
                  buf_ipv4[arglen-1] = '\0';
                else
                  buf_ipv4[arglen] = '\0';
              }

            /* Call IMI API. */
            ret = imi_dhcps_dns_add (pool, buf_ipv4);

            /* Skip white spaces. */
            while (pal_char_isspace ((int) * cp) && *cp != '\0')
              cp++;
          }
      }

  /* command is 'routers' */
  if (cmdlen == pal_strlen (routers))
    if (! pal_strncmp (buf_cmd, routers, pal_strlen (routers)))
      {
        ret = IMI_API_SUCCESS;

        /* Read list of routers. */
        while (*cp != '\0' && ret == IMI_API_SUCCESS)
          {
            /* Read ipv4 into buffer. */
            start = cp;
            while (!
                   (pal_char_isspace ((int) * cp) || *cp == '\r'
                    || *cp == '\n') && *cp != '\0')
              cp++;
            arglen = cp - start;
            if (arglen > IPV4_ADDR_STRLEN+1)
              return IMI_API_IPV4_ERROR;
            else
              {
                pal_strncpy (buf_ipv4, start, arglen);
                if (buf_ipv4[arglen-1] == ';' || 
                    buf_ipv4[arglen-1] == ',')
                  buf_ipv4[arglen-1] = '\0';
                else
                  buf_ipv4[arglen] = '\0';
              }

            /* Call IMI API. */
            ret = imi_dhcps_default_router_add (pool, buf_ipv4);

            /* Skip white spaces. */
            while (pal_char_isspace ((int) * cp) && *cp != '\0')
              cp++;
          }
      }

  return ret;
}

/* Read 'range' line from /etc/dhcpd.conf:
   - 'cp' points to second argument (start-addr). */
static result_t
pal_dhcp_read_line_range (struct lib_globals *lib_node,
                          struct imi_dhcp *dhcp,
                          char *buf,
                          char *cp,
                          struct imi_dhcp_pool *pool)
{
  char *start;
  char buf_start[IPV4_ADDR_STRLEN+1];           /* +1=extra ',' ';' etc. */
  char buf_end[IPV4_ADDR_STRLEN+1];
  int buflen;
  int ret = IMI_API_SUCCESS;

  /* Read second word (start ipv4 addr) into buffer. */
  start = cp;
  while (!
         (pal_char_isspace ((int) * cp) || *cp == '\r'
          || *cp == '\n') && *cp != '\0')
    cp++;
  buflen = cp - start;
  if (buflen > IPV4_ADDR_STRLEN+1)
    return IMI_API_IPV4_ERROR;
  else
    {
      pal_strncpy (buf_start, start, buflen);
      if (buf_start[buflen-1] == ';' || 
          buf_start[buflen-1] == ',')
        buf_start[buflen-1] = '\0';
      else
        buf_start[buflen] = '\0';
    }

  /* Skip white spaces. */
  while (pal_char_isspace ((int) * cp) && *cp != '\0')
    cp++;

  /* If no more arguments, it's single address. */
  if (*cp == '\0')
    {
      /* Call IMI API to add the address. */
      ret = imi_dhcps_pool_range_add (pool, buf_start, NULL);
      return ret;
    }

  /* Else, read end range. */
  start = cp;
  while (!
         (pal_char_isspace ((int) * cp) || *cp == '\r'
          || *cp == '\n') && *cp != '\0')
    cp++;
  buflen = cp - start;
  if (buflen > IPV4_ADDR_STRLEN+1)
    return IMI_API_IPV4_ERROR;
  else
    {
      pal_strncpy (buf_end, start, buflen);
      if (buf_end[buflen-1] == ';' || 
          buf_end[buflen-1] == ',')
        buf_end[buflen-1] = '\0';
      else
        buf_end[buflen] = '\0';
    }

  /* Call IMI API to add the range. */
  ret = imi_dhcps_pool_range_add (pool, buf_start, buf_end);
  return ret;
}

/* Read 'default-lease-time' line from /etc/dhcpd.conf:
   - 'cp' points to second argument (start-addr). */
static result_t
pal_dhcp_read_line_lease (struct lib_globals *lib_node,
                          struct imi_dhcp *dhcp,
                          char *buf,
                          char *cp,
                          struct imi_dhcp_pool *pool)
{
  char buf_arg[PAL_FILE_DEFAULT_LINELEN];
  int buflen;
  int lease_s;
  u_int8_t lease_d = 0;
  u_int8_t lease_h = 0;
  u_int8_t lease_m = 0;
  char lbuf_d[4];
  char lbuf_h[4];
  char lbuf_m[4];
  int day_s = 24*60*60;                 /* 1 day = 24 * 60 * 60 sec */
  int hr_s = 60 * 60;                   /* 1 hr = 60 * 60 sec */
  int min_s = 60;
  char *start;
  int ret = IMI_API_SUCCESS;

  /* Read second word (lease in seconds) into buffer. */
  start = cp;
  while (!
         (pal_char_isspace ((int) * cp) || *cp == '\r'
          || *cp == '\n') && *cp != '\0')
    cp++;
  buflen = cp - start;
  pal_strncpy (buf_arg, start, buflen);
  if (buf_arg[buflen-1] == ';' ||
      buf_arg[buflen-1] == ',')
    buf_arg[buflen-1] = '\0';
  else
    buf[buflen] = '\0';

  /* Convert to integer. */
  lease_s = pal_atoi (buf_arg);

  /* Calculate days/hrs/mins for calling API. 
     Note: Linux dhcpd uses 'seconds' while Cisco uses d/h/m format.
     As a result, some precision is lost during the conversion. */
  while (lease_s > day_s)
    {
      lease_d++;
      lease_s = lease_s - day_s;
    }
  while (lease_s > hr_s)
    {
      lease_h++;
      lease_s = lease_s - hr_s;
    }
  while (lease_s > min_s)
    {
      lease_m++;
      lease_s = lease_s - min_s;
    }

  /* Call IMI API. */
  sprintf (lbuf_d, "%d", lease_d);
  sprintf (lbuf_h, "%d", lease_h);
  sprintf (lbuf_m, "%d", lease_m);
  ret = imi_dhcps_lease_set (pool, lbuf_d, lbuf_h, lbuf_m);
  return ret;
}

/* Read startup config. */
static result_t
pal_dhcp_read_startup_config (struct lib_globals *lib_node,
                              struct imi_dhcp *dhcp)
{
  char buf[PAL_FILE_DEFAULT_LINELEN];
  char buf_cmd[20];
  struct imi_dhcp_pool *pool = NULL;
  result_t ret = RESULT_ERROR;
  const char *subnet = "subnet";
  const char *range = "range";
  const char *option = "option";
  const char *lease = "default-lease-time";
  const char *pool_exit = "}";
  char *cp, *start;
  int cmdlen;
  FILE *fp;

  /* Open file (/etc/dhcpd.conf). */
  fp = pal_fopen (DHCP_FILE, PAL_OPEN_RO);
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
              zlog_warn (lib_node, "%s : Invalid line : %s\n", DHCP_FILE, buf);
              continue;
            }
        }

      /* Disable DHCP while we reconfigure to avoid overwriting the file. */
      imi_dhcps_disable ();

      /* Are we in pool configuration mode? */
      if (pool)
        {
          /* command is 'option' */
          if (cmdlen == pal_strlen (option))
            if (! pal_strncmp (buf_cmd, option, pal_strlen (option)))
              ret = pal_dhcp_read_line_option (lib_node, dhcp, buf, cp, pool);

          /* command is 'range' */
          if (cmdlen == pal_strlen (range))
            if (! pal_strncmp (buf_cmd, range, pal_strlen (range)))
              ret = pal_dhcp_read_line_range (lib_node, dhcp, buf, cp, pool);

          /* command is 'default-lease-time' */
          if (cmdlen == pal_strlen (lease))
            if (! pal_strncmp (buf_cmd, lease, pal_strlen (lease)))
              ret = pal_dhcp_read_line_lease (lib_node, dhcp, buf, cp, pool);

          /* command is '}' (quit pool configuration). */
          if (cmdlen == pal_strlen (pool_exit))
            if (! pal_strncmp (buf_cmd, pool_exit, pal_strlen (pool_exit)))
              {
                /* Set pool to NULL. */
                pool = NULL;
                ret = IMI_API_SUCCESS;
              }
        }
      else
        {
          /* command is 'subnet' */
          if (cmdlen == pal_strlen (subnet))
            if (! pal_strncmp (buf_cmd, subnet, pal_strlen (subnet)))
              ret = pal_dhcp_read_line_subnet (lib_node, dhcp, buf, cp, &pool);
        }

      switch (ret)
        {
        case IMI_API_SUCCESS:
          /* Set flag to inform IMI that system config is present. */
          dhcp->sysconfig = PAL_TRUE;
          break;
        case IMI_API_UNKNOWN_CMD_ERROR:
          zlog_warn (lib_node, "%s : Unknown command > %s\n", DHCP_FILE, buf);
          break;
        case IMI_API_DUPLICATE_ERROR:
          zlog_warn (lib_node, "%s : Duplicate > %s\n", DHCP_FILE, buf);
          break;
        case IMI_API_IPV4_ERROR:
          zlog_warn (lib_node, "%s : Invalid prefix/mask > %s\n", 
                     DHCP_FILE, buf);
          break;
        case IMI_API_CANT_SET_ERROR:
          zlog_warn (lib_node, "%s : Set Error > %s\n", DHCP_FILE, buf);
          break;
        case RESULT_ERROR:
        default:
          /* Unknown command. */
          zlog_warn (lib_node, "%s : Invalid command > %s\n", DHCP_FILE, buf);
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
pal_dhcp_start (struct lib_globals *lib_node, struct imi_dhcp *dhcp)
{
#ifdef HAVE_IMI_SYSCONFIG
  /* Read dhcpd.conf at startup if enabled. */
  pal_dhcp_read_startup_config (lib_node, dhcp); 
#endif /* HAVE_IMI_SYSCONFIG */

  return (pal_handle_t) 1;
}

/* Shutdown. */
result_t
pal_dhcp_stop (struct lib_globals *lib_node, pal_handle_t pal_dhcp)
{
  char buf[255];
  int ret = RESULT_ERROR;
  char pid[255];
  size_t bytes = 255;
  FILE *fp;

  fp = pal_fopen (DHCP_PID_FILE, PAL_OPEN_RO);
  if (fp != NULL)
    {
      /* Read PID. */
      pal_fgets (pid, bytes, fp);

      /* Close file handle. */
      pal_fclose (fp);

      /* Kill existing daemon and remove PID file. */
      pal_snprintf (buf, 255, "kill -s KILL %s", pid);
#ifndef DEBUG
      system (buf); 
#else
      printf ("%s\n", buf);
#endif /* DEBUG */

      /* Remove PID file. */
      pal_snprintf (buf, 255, "rm -f %s", DHCP_PID_FILE);
#ifndef DEBUG
      system (buf);
#else
      printf ("%s\n", buf);
#endif /* DEBUG */

      ret = RESULT_OK;
    }

  return ret;
}

/* Restart dhcpd. */
static void
pal_dhcp_restart_protocol (struct lib_globals *lib_node,
                           struct imi_dhcp *dhcp)
{
  char buf[255];
  char *argv[3];
  FILE *fp;

  fp = pal_fopen (DHCP_PID_FILE, PAL_OPEN_RO);
  if (fp != NULL)
    {
      /* Close file handle. */
      pal_fclose (fp);

      /* 1. Kill existing daemon (no restart for dhcpd). */
      pal_snprintf (buf, 255, "kill -s KILL `cat %s`\n", DHCP_PID_FILE);
#ifndef DEBUG
      system (buf);
#else
      printf ("%s\n", buf);
#endif /* DEBUG */

      /* Remove PID file. */
      pal_snprintf (buf, 255, "rm -f %s", DHCP_PID_FILE);
#ifndef DEBUG
      system (buf);
#else
      printf ("%s\n", buf);
#endif /* DEBUG */
    }

  /* Restart? */
  if (dhcp->enabled)
    {
      /* 2. Restart daemon. */
#ifndef DEBUG
      argv[0] = DHCP_SERVER_APP;
      argv[1] = DHCP_SERVER_OPTIONS;
      argv[2] = 0;
      pal_daemonize_program (DHCP_SERVER_APP, argv, 0, 0);
#else
      pal_snprintf (buf, 255, "DHCP_SERVER_APP");
      printf ("3 %s\n", buf);
#endif /* DEBUG */
    }
}

/* Refresh Linux DHCP. */
result_t
pal_dhcp_refresh (struct lib_globals *lib_node, struct imi_dhcp *dhcp)
{
  int ret;
  struct listnode *node;
  struct listnode *pool_node;
  struct prefix_ipv4 *p;
  char buf[PAL_FILE_DEFAULT_LINELEN];
  char buf2[IPV4_ADDR_STRLEN+1];
  char buf3[IPV4_ADDR_STRLEN+1];
  struct pal_in4_addr in4;
  struct imi_dhcp_range *r;
  struct imi_dhcp_pool *pool;
  int count;
  FILE *fp;

#ifdef HAVE_IMI_SYSCONFIG
  if (dhcp->shutdown_flag)
    return RESULT_OK;
#endif /* HAVE_IMI_SYSCONFIG */

  /* Open file (/etc/dhcpd.conf). */
  fp = pal_fopen (DHCP_FILE, PAL_OPEN_RW);
  if (fp == NULL)
    return IMI_API_FILE_OPEN_ERROR;

  /* Seek to start of file. */
  ret = pal_fseek (fp, 0, PAL_SEEK_SET);
  if (ret < 0)
    {
      pal_fclose (fp);
      return IMI_API_FILE_SEEK_ERROR;
    }

  /* Is DHCP service enabled? */
  if ((dhcp->enabled) && (!dhcp->shutdown_flag))
    {
      /* Write top line. */
      ret = pal_fputs (DHCP_TOP_FILE_TEXT, fp);

      /* Write each pool. */
      if (listcount (dhcp->pool_list) <= 0)
        {
          /* If no pools defined, return write it & return success. */
          sprintf (buf, "# No address pools have been defined.\n");
          pal_fputs (buf, fp);
          pal_fclose (fp);
          return IMI_API_SUCCESS;
        }
      else
        {
          /* Write each pool. */
          for (pool_node = LISTHEAD (dhcp->pool_list); pool_node; NEXTNODE (pool_node))
            {
              pool = GETDATA (pool_node);

              /* Is the network set yet? */
              if (!pool->network_set)
                {
                  sprintf (buf, "\n# DHCP Pool %s network unset.\n", 
                           pool->pool_id);
                  pal_fputs (buf, fp);

#if 0
                  /* Close filehandle. */
                  pal_file_close (lib_node, cfg);

                  return IMI_API_NOT_SET_ERROR;
#endif
                }
              else
                {
                  sprintf (buf, "\n# DHCP Pool %s\n", pool->pool_id);
                  pal_fputs (buf, fp);

                  /* Subnet & mask. */
                  masklen2ip (pool->network.prefixlen, &in4);
                  pal_inet_ntoa (pool->network.prefix, buf2);
                  pal_inet_ntoa (in4, buf3);
                  sprintf (buf, "subnet %s netmask %s {\n", buf2, buf3);
                  pal_fputs (buf, fp);

                  /* Subnet mask option. */
                  sprintf (buf, " option subnet-mask %s;\n", buf3);
                  pal_fputs (buf, fp);

                  /* Range(s). */
                  if (listcount (pool->addr_list) > 0)
                    for (node = LISTHEAD (pool->addr_list); node; NEXTNODE (node))
                      {
                        p = GETDATA (node);
                        pal_inet_ntoa (p->prefix, buf2);
                        sprintf (buf, " range %s;\n", buf2);
                        pal_fputs (buf, fp);
                      }
                  if (listcount (pool->range_list) > 0)
                    for (node = LISTHEAD (pool->range_list); node; NEXTNODE (node))
                      {
                        r = GETDATA (node);
                        pal_inet_ntoa (r->low.prefix, buf2);
                        pal_inet_ntoa (r->high.prefix, buf3);
                        sprintf (buf, " range %s %s;\n", buf2, buf3);
                        pal_fputs (buf, fp);
                      }

                  /* Lease time. */
                  if (!pool->infinite)
                    {
                      if (pool->lease_seconds > 0)
                        {
                          sprintf (buf, " default-lease-time %d;\n", 
                                   pool->lease_seconds);
                          pal_fputs (buf, fp);
                          sprintf (buf, " max-lease-time %d;\n", 
                                   pool->lease_seconds);
                          pal_fputs (buf, fp);
                        }
                    }

                  /* Domain. */
                  if (pool->domain_name)
                    {
                      sprintf (buf, " option domain-name \"%s\";\n", 
                               pool->domain_name);
                      pal_fputs (buf, fp);
                    }

                  /* DNS Server(s). */
                  if (listcount (pool->dns_list) > 0)
                    {
                      sprintf (buf, " option domain-name-servers");
                      pal_fputs (buf, fp);

                      count = 0;
                      for (node = LISTHEAD (pool->dns_list); node; NEXTNODE (node))
                        {
                          p = GETDATA (node);
                          pal_inet_ntoa (p->prefix, buf2);
                          if (count > 0)
                            sprintf (buf, ", %s", buf2);
                          else
                            sprintf (buf, " %s", buf2);
                          count++;

                          pal_fputs (buf, fp);
                        }

                      sprintf (buf, ";\n");
                      pal_fputs (buf, fp);
                    }

                  /* Default router(s). */
                  if (listcount (pool->dr_list) > 0)
                    {
                      sprintf (buf, " option routers");
                      pal_fputs (buf, fp);

                      count = 0;
                      for (node = LISTHEAD (pool->dr_list); node; NEXTNODE (node))
                        {
                          p = GETDATA (node);
                          pal_inet_ntoa (p->prefix, buf2);
                          if (count > 0)
                            sprintf (buf, ", %s", buf2);
                          else
                            sprintf (buf, " %s", buf2);
                          count++;

                          pal_fputs (buf, fp);
                        }

                      sprintf (buf, ";\n");
                      pal_fputs (buf, fp);
                    }

                  /* Close bracket. */
                  sprintf (buf, "}");
                  pal_fputs (buf, fp);

                } /* else */

              sprintf (buf, "\n");
              pal_fputs (buf, fp);
            }  /* for */

          /* Write update style (none) required to run dhcpd. */
#ifdef HAVE_DHCP_UPDATE_STYLE
          sprintf (buf, "ddns-update-style none;");
          pal_fputs (buf, fp);
#endif
        } /* else */
    }
  else
    {
      /* Write disabled message to dhcpd.conf instead of configuration. */
      ret = pal_fputs (DHCP_DISABLE_FILE_TEXT, fp);
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
  pal_dhcp_restart_protocol (lib_node, dhcp);

  return RESULT_OK;
}
#endif /* HAVE_DHCP_SERVER */

