/* Copyright (C) 2001-2003  All Rights Reserved. */

#include <pal.h>

#include "cli.h"
#include "line.h"
#include "snprintf.h"

#include "vtysh.h"
#include "vtysh_cli.h"
#include "vtysh_exec.h"
#include "vtysh_system.h"
#include "vtysh_line.h"
#include "vtysh_pm.h"

/* Extern user input line.  */
extern char *input_line;

/* Extern vtysh line.  */
extern struct line *vtysh_line;

void vtysh_send_cli (struct cli *cli, int argc, char **argv, u_int16_t);

/* Send a line to protocol module using "line".  */
int
vtysh_pm_line_send (struct vtysh_pm *pm, char *str, int mode, vrf_id_t vrf_id)
{
  int ret;
  struct line line;
  u_char *pnt;
  u_int16_t size;

  /* When string is not specified, just return.  */
  if (! str)
    return 0;

  /* Check protocol connection.  */
  if (! pm->connected)
    return 0;

  /* Set "line" protocol values.  */
  strcpy (line.buf + LINE_HEADER_LEN, str);
  line.length = strlen (str) + 1 + LINE_HEADER_LEN;
  line.module = PM_EMPTY;
  line.key = 0;
  line.mode = mode;
  line.privilege = PRIVILEGE_MAX;
  line.code = LINE_CODE_COMMAND;
#ifdef HAVE_VR
  line.cli.vrf_id = vrf_id;
#endif /* HAVE_VR */

  /* Encode it to buffer.  */
  pnt = line.buf;
  size = LINE_MESSAGE_MAX;
  line_encode (&pnt, &size, &line);

  /* Send message to the protocol module "line".  */
  ret = pal_sock_write (pm->sock, line.buf, line.length);
  if (ret <= 0)
    {
      vtysh_pm_close (pm);
      return -1;
    }

  return 0;
}

/* Proxy shell input to proper protocol modules.  */
int
vtysh_line_proxy (struct cli_element *cel, char *line, struct line *reply)
{
  int i;
  struct vtysh_pm *pm;
  int ret;
  modbmap_t module = cel->module;
  int mode;

  mode = vtysh_mode_get ();

  /* "end" treatment.  */
  if (pal_strncmp (cel->str, "end", 3) == 0)
    {
      module = PM_ALL;
      mode = CONFIG_MODE;
    }

  /* "exit" and "quit" treatment.  */
  if (pal_strncmp (cel->str, "exit", 4) == 0
      || pal_strncmp (cel->str, "quit", 4) == 0)
    {
      switch (mode)
        {
        case CONFIG_MODE:
          module = PM_ALL;
          break;
        case VRF_MODE:
          module = modbmap_or (PM_NSM, PM_BGP);
          break;
        case INTERFACE_MODE:
          module = PM_IF;
          break;
        case KEYCHAIN_MODE:
        case KEYCHAIN_KEY_MODE:
          module = PM_KEYCHAIN;
          break;
        case RMAP_MODE:
          module = PM_RMAP;
          break;
        case RIP_MODE:
        case RIP_VRF_MODE:
          module = PM_RIP;
          break;
        case RIPNG_MODE:
          module = PM_RIPNG;
          break;
        case OSPF_MODE:
          module = PM_OSPF;
          break;
        case OSPF6_MODE:
          module = PM_OSPF6;
          break;
        case ISIS_MODE:
        case ISIS_IPV6_MODE:
          module = PM_ISIS;
          break;
        case BGP_MODE:
        case BGP_IPV4_MODE:
        case BGP_IPV4M_MODE:
        case BGP_IPV4_VRF_MODE:
        case BGP_IPV6_MODE:
        case BGP_6PE_MODE:
        case BGP_VPNV4_MODE:
/*6VPE*/
         case BGP_VPNV6_MODE:
         case BGP_IPV6_VRF_MODE:
          module = PM_BGP;
          break;
        case LDP_MODE:
        case LDP_PATH_MODE:
        case LDP_TRUNK_MODE:
          module = PM_LDP;
          break;
#ifdef HAVE_LMP
        case LMP_MODE:
          module = PM_LMP;
          break;
#endif /* HAVE_LMP */
        case RSVP_MODE:
        case RSVP_PATH_MODE:
        case RSVP_TRUNK_MODE:
          module = PM_RSVP;
          break;
#ifdef HAVE_MPLS_FRR
  case RSVP_BYPASS_MODE:
          module = PM_RSVP;
          break;
#endif /* HAVE_MPLS_FRR */
        case NSM_VPLS_MODE:
        case VRRP_MODE:
        case VRRP6_MODE:
		case PW_MODE:
		case AC_MODE:
		case RPORT_MODE:	
		case VSI_MODE:
	    case VLL_MODE:
		case TUN_MODE:
		case MEG_MODE:
		case OAM_1731_MODE:
		case PTP_MODE:
		case PTP_PORT_MODE:
          module = PM_NSM;
          break;
        }
    }

  /* Check each index.  */
  FOREACH_APN_PROTO (i)
    if (MODBMAP_ISSET (module, i)
        && (pm = vtysh_pm_lookup (vtysh_pmaster, i)) != NULL)
      {
        /* When protocol module is not connected.  */
        if (pm->connected != PAL_TRUE)
          continue;

        /* Send message.  */
        ret = vtysh_pm_line_send (pm, line, mode, 0);
        if (ret < 0)
          {
            zsnprintf (reply->buf + LINE_HEADER_LEN, 256, "");
            return -1;
          }

        /* Get reply.  */
        reply->buf[LINE_HEADER_LEN] = '\0';
        line_client_read (pm->sock, reply);

        if (reply->code == LINE_CODE_ERROR)
          return -1;
      }
  return 0;
}

/* Error handler.  */
void
vtysh_line_proxy_error (struct line *reply)
{
  if (reply->buf[LINE_HEADER_LEN] != '\0')
    printf ("%s", reply->buf + LINE_HEADER_LEN);
}

/* Parser for user input and IMI commands.  */
void
vtysh_parse (char *line, int mode)
{
  int ret;
  struct cli *cli;
  struct cli_node *node;
  u_char privilege;

  cli = &vtysh_line->cli;

  /* When line is specified, mode is also specified.  */
  if (line)
    privilege = PRIVILEGE_MAX;
  else
    {
      /* Normal case we use cli of vtysh line.  */
      mode = cli->mode;
      privilege = cli->privilege;
      line = input_line;
    }

  /* Line length check.  */
  if (strlen (line) >= LINE_BODY_LEN - 1)
    {
      printf ("%% Input line is too long\n");
      return;
    }

#ifdef HAVE_VR
  /* Set vrf_id. */
  ctree->vrf_id = cli->vrf_id = imiline.vrf_id;
#endif /* HAVE_VR */

  /* Parse user input.  */
  ret = cli_parse (ctree, mode, privilege, line, 1, 0);

  /* Check return value.  */
  switch (ret)
    {
    case CLI_PARSE_NO_MODE:
      /* Ignore no mode line.  */
      break;
    case CLI_PARSE_EMPTY_LINE:
      /* Ignore empty line.  */
      break;
    case CLI_PARSE_SUCCESS:
      /* Node to be executed.  */
      node = ctree->exec_node;

      /* Set cli structure.  */
      cli->str = line;
      cli->cel = node->cel;
      cli->ctree = ctree;
      cli->vr = apn_vr_get_privileged (vtyshm);

      /* Show command has special treatment.  */
      if (ctree->show_node || CHECK_FLAG (node->cel->flags, CLI_FLAG_SHOW))
        {
          /* Run command with pager.  */
          vtysh_more ((INTERNAL_FUNC) node->cel->func, cli,
                      ctree->argc, ctree->argv);
        }
      else
        {
          /* This a protocol module command...  */
          if (! modbmap_isempty (node->cel->module))
            {
              struct line reply;

              memset (&reply, 0, sizeof (struct line));
              reply.str = reply.buf + LINE_HEADER_LEN;

              ret = vtysh_line_proxy (node->cel, line, &reply);
              if (ret < 0)
                {
                  vtysh_line_proxy_error (&reply);
                  return;
                }

              ret = LINE_CODE_SUCCESS;

              /* Check return value.  */
              switch (ret)
                {
                case LINE_CODE_AUTH_REQUIRED:
                  /* Tell the function that authentication is
                     required.  */
                  cli->auth = 1;

                  /* Fall through.  */
                case LINE_CODE_SUCCESS:
                  /* Execute local function when it is defined.  */
                  if (node->cel->func)
                    (*node->cel->func) (cli, ctree->argc, ctree->argv);
                  vtysh_line_print_response ();
                  break;
                default:
                  /* Display error message from IMI.  */
                  vtysh_line_print_response ();
                  break;
                }
            }
          else
            /* Execute local function.  */
            (*node->cel->func) (cli, ctree->argc, ctree->argv);
        }
      break;

    case CLI_PARSE_AMBIGUOUS:
      printf ("%% Ambiguous command:  \"%s\"\n", line);
      break;

    case CLI_PARSE_INCOMPLETE:
      printf ("%% Incomplete command.\n\n");
      break;

    case CLI_PARSE_INCOMPLETE_PIPE:
      printf ("%% Incomplete command before pipe.\n\n");
      break;

    case CLI_PARSE_NO_MATCH:
      if (ctree->invalid)
        {
          printf ("%*c^\n",
                  strlen (vtysh_prompt ()) + (ctree->invalid - line), ' ');
          printf ("%% Invalid input detected at '^' marker.\n\n");
        }
      else
        printf ("%% Unrecognized command\n");
      break;

    default:
      break;
    }

  /* Free arguments.  */
  cli_free_arguments (ctree);

  /* Check close.  */
  if (cli->status == CLI_CLOSE)
    {
      vtysh_stop ();
      vtysh_exit (0);
    }
}
