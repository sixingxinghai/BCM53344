/* Copyright (C) 2001-2003  All Rights Reserved. */

#include "pal.h"
#include "lib.h"
#include "log.h"
#include "network.h"

#include "imi/imi.h"
#include "imi/util/imi_server.h"


/* Prototypes. */
struct imi_serv *imi_server_new ();
void imi_server_free (struct imi_serv *);

#ifdef HAVE_IMI_DEFAULT_GW
/* Default Gateway - In PacOS NSM, default gateway is configured as:
                        > ip route 0.0.0.0/0 <ADDR>
                     However, it is useful for home gateway to be able
                     to specify only the address.  For convenience, IMI
                     stores the single home gateway & provides simpler
                     API for the web interface. */

/* Initialize default gateway. */
static void
imi_default_gw_init (u_int8_t family, struct imi_serv *imi_serv)
{
  char prefix_buf[INET_ADDRSTRLEN];
  const char *str4 = "show ip route 0.0.0.0/0";
  int str4len = pal_strlen (str4);
  int count = 0;
  char *cp, *start;
  int len;

  switch (family)
    {
    case AF_INET:
      /* Get default route from NSM. */
      pal_strncpy (imi_serv->buf, str4, str4len);

      /* Execute the command. */
      imi_server_show_command (imi_serv);

      /* Was there a response? */
      if (pal_strlen (imi_serv->outbuf) > 0)
        {
          cp = imi_serv->outbuf;

          /* Skip first two lines. */
          while (count < 2)
            {
              if (*cp == '\n')
                count++;
              if (*cp == '\0')
                return;
              cp++;
            }

          SKIP_WHITE_SPACE (cp);

          /* First arg might be '*' if route is installed in FIB. */
          if (*cp == '*')
            {
              cp++;
              SKIP_WHITE_SPACE (cp);
            }

          /* Get the prefix. */
          READ_NEXT_WORD_TO_BUF (cp, start, prefix_buf, len);

          /* Remove comma. */
          if (prefix_buf[len-1] == ',')
            prefix_buf[len-1] = '\0';
          else
            prefix_buf[len] = '\0';

          IMI->gateway = XSTRDUP (MTYPE_IMI_STRING, prefix_buf);
        }
      else
        IMI->gateway = NULL;

      break;
#ifdef HAVE_IPV6
    case AF_INET6:
      break;
#endif /* HAVE_IPV6 */
    default:
      break;
    }
}

/* Shutdown default gateway. */
void
imi_default_gw_shutdown ()
{
  /* If gateway exists, free it. */
  if (IMI->gateway)
    XFREE (MTYPE_IMI_STRING, IMI->gateway);
  IMI->gateway = NULL;
}

/* Reset default gateway. */
int
imi_default_gw_reset (struct imi_serv *imi_serv)
{
  /* Delete default route in the NSM. */
  if (IMI->gateway)
    {
      /* Delete route. */
      sprintf (imi_serv->buf, "no ip route 0.0.0.0/0 %s\n", IMI->gateway);
      imi_serv->pnt = imi_serv->buf;
      imi_server_config_command (imi_serv, NULL);

      /* Free the local copy. */
      XFREE (MTYPE_IMI_STRING, IMI->gateway);
      IMI->gateway = NULL;
    }
  return RESULT_OK;
}

/* Set default gateway. */
int
imi_default_gw_set (struct imi_serv *imi_serv)
{
  char prefix_buf[INET_ADDRSTRLEN];
  char *cp, *start;
  int len;

  cp = imi_serv->pnt;

  SKIP_WHITE_SPACE (cp);

  /* Get the prefix. */
  READ_NEXT_WORD_TO_BUF (cp, start, prefix_buf, len);

  /* Clear existing gateway. */
  if (IMI->gateway)
    imi_default_gw_reset (imi_serv);

  /* Store locally. */
  IMI->gateway = XSTRDUP (MTYPE_IMI_STRING, prefix_buf);

  /* Add route to NSM. */
  sprintf (imi_serv->buf, "ip route 0.0.0.0/0 %s\n", IMI->gateway);
  imi_serv->pnt = imi_serv->buf;
  imi_server_config_command (imi_serv, NULL);

  return RESULT_OK;
}
#endif /* HAVE_IMI_DEFAULT_GW */

#ifdef HAVE_IMI_WAN_STATUS
/* WAN Satatus:
     The following functions are used to maintain the status of the WAN
     interface.  Such capability is useful for a web application to store
     the current status so it is preserved for subsequent reference.  Also,
     such information may be useful for imposing restrictions within IMI.  */

/* Get. */
enum wan_status
imi_wan_status_get ()
{
  /* Return status. */
  return IMI->wan_status;
}

/* Set. */
int
imi_wan_status_set (enum wan_status new)
{
  /* Set new status. */
  IMI->wan_status = new;
  return RESULT_OK;
}

/* Reset default. */
int
imi_wan_status_reset_default ()
{
  /* Default is static. */
  IMI->wan_status = WAN_STATIC;
  return RESULT_OK;
}
#endif /* HAVE_IMI_WAN_STATUS */

#ifdef HAVE_IMI_DEFAULT_GW
/* Initialize Utilities. */
void
imi_util_init ()
{
  struct imi_serv *imi_init;

  /* Initialize default gateway. */
  imi_init = imi_server_new ();
  if (imi_init == NULL)
    {
      zlog_warn (imim, "Can't init IMI utilities; CLI is locked.");
      return;
    }
  imi_default_gw_init (AF_INET, imi_init);
  imi_server_free (imi_init);
}
#endif /* HAVE_IMI_DEFAULT_GW */


