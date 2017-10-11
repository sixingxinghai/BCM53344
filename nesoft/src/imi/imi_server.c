/* Copyright (C) 2004  All Rights Reserved. */

#include "pal.h"

#include "lib.h"
#include "line.h"
#include "modbmap.h"
#include "vector.h"
#include "thread.h"
#include "network.h"
#include "message.h"
#include "imi_client.h"

#include "imi/imi.h"
#include "imi/imi_api.h"
#include "imi/imi_line.h"
#include "imi/imi_config.h"
#include "imi/imi_parser.h"
#include "imi/imi_server.h"

int imi_server_send_config (struct imi_server_entry *ise);
#include "cfg_data_types.h"
#include "cfg_seq.h"
#include "keychain.h"
#include "routemap.h"


void
imi_server_pm_set (struct lib_globals *zg, modbmap_t module)
{
  struct imi_globals *ig = zg->proto;
  struct imi_master *im;
  struct apn_vr *vr;
  int i;

  /* Set the active PM bit in the globals.  */
  ig->module = modbmap_or (ig->module, module);

  /* Set for each VR.  */
  for (i = 0; i < vector_max (zg->vr_vec); i++)
    if ((vr = vector_slot (zg->vr_vec, i)))
      if ((im = vr->proto))
        if (modbmap_check (im->module_config, module))
          im->module = modbmap_or (im->module, module);
}

void
imi_server_pm_unset (struct lib_globals *zg, modbmap_t module)
{
  struct imi_globals *ig = zg->proto;
  struct imi_master *im;
  struct apn_vr *vr;
  int i;

  /* Reset the active PM bit in the globals.  */
  ig->module = modbmap_sub (ig->module, module);

  /* Reset for each VR.  */
  for (i = 0; i < vector_max (zg->vr_vec); i++)
    if ((vr = vector_slot (zg->vr_vec, i)))
      if ((im = vr->proto))
        im->module = modbmap_sub (im->module, module);
}


struct imi_server_entry *
imi_server_entry_new (struct message_handler *ms, struct message_entry *me)
{
  struct imi_server_entry *new;

  new = XMALLOC (MTYPE_IMI_SERVER_ENTRY, sizeof (struct imi_server_entry));
  pal_mem_set (new, 0, sizeof (struct imi_server_entry));

  me->info = new;
  new->me = me;
  new->is = ms->info;
  new->line.zg = ms->zg;
  new->connect_time = pal_time_sys_current (NULL);

  return new;
}

void
imi_server_entry_free (struct imi_server_entry *ise)
{
  struct imi_server_client *isc = ise->isc;

  /* Remove ise from linked list.  */
  if (ise->prev)
    ise->prev->next = ise->next;
  else if (isc)
    isc->head = ise->next;

  if (ise->next)
    ise->next->prev = ise->prev;
  else if (isc)
    isc->tail = ise->prev;

  /* Free IMI server entry.  */
  XFREE (MTYPE_IMI_SERVER_ENTRY, ise);
}

void
imi_server_entry_free_all (struct imi_server_client *isc)
{
  struct imi_server_entry *ise, *next;

  for (ise = isc->head; ise;)
  {
    next = ise->next;

    /* Free IMI server entry.  */
    imi_server_entry_free (ise);

    ise = next;
  }
}


struct imi_server_client *
imi_server_client_new (struct imi_server *is, module_id_t module_id)
{
  struct imi_server_client *new;

  new = XCALLOC (MTYPE_IMI_SERVER_CLIENT, sizeof (struct imi_server_client));
  new->module_id = vector_set_index (is->client, module_id, new);

  /* Set the PM active bit.  */
  imi_server_pm_set (is->ms->zg, modbmap_id2bit (module_id));

  return new;
}

void
imi_server_client_free (struct imi_server *is, module_id_t module_id)
{
  struct imi_server_client *isc = vector_slot (is->client, module_id);

  /* Reset the PM active bit.  */
  imi_server_pm_unset (is->ms->zg, modbmap_id2bit (module_id));

  if (isc)
    XFREE (MTYPE_IMI_SERVER_CLIENT, isc);

  vector_slot (is->client, module_id) = NULL;
}

struct imi_server_client *
imi_server_client_lookup (struct imi_server *is, module_id_t module_id)
{
  if (module_id > APN_PROTO_UNSPEC && module_id < vector_max (is->client))
    return vector_slot (is->client, module_id);

  return NULL;
}

/* A thread callback to perform configuration of next VR. */

static int
_imi_server_config_next_vr (struct thread *t)
{
  struct imi_server_entry *ise = THREAD_ARG(t);

  ise->line.code  = LINE_CODE_CONFIG_REQUEST;
  ise->line.vr_id = THREAD_VAL(t);

  ise->line.vr = apn_vr_lookup_by_id (THREAD_GLOB(t), ise->line.vr_id);
  if (ise->line.vr == NULL)
    return -1;
  return imi_server_send_config (ise);
}

/*-----------------------------------------------------------------------
 * imi_server_line_pm_send - Send a "line" to protocol module
 *
 * Returns
 *   ZRES_ERR - comm failure
 *   ZRES_OK  - success
 *   ZRES_FAIL- failed on the other side
 */
ZRESULT
imi_server_line_pm_send (module_id_t protocol, struct line *line)
{
  struct imi_server_client *isc = NULL;
  struct imi_server_entry *ise = NULL;
  struct imi_server *is = imim->imh->info;
  struct line reply;
  int ret;

  if ((isc = imi_server_client_lookup (is, protocol)))
    for (ise = isc->head; ise; ise = ise->next)
      {
        ret = imi_server_send_line (ise, line, &reply);
        if (ret < 0)
          return ZRES_ERR;
        else
          if (reply.code != LINE_CODE_SUCCESS)
            return ZRES_FAIL;
          else
            return ZRES_OK;
      }
  /* Client module is not running. */
  return ZRES_ERR;
}

int
imi_server_recv_line (struct imi_server_entry *ise, struct line *reply)
{
  int nbytes;

  /* Here we are waiting mostly to receive a reply. However, it may happen that
     the PM may send us VR config download request. We will schedule an event
     thread that will be executed after downloading of configuration to the
     current VR is completed.
   */
  do {
  /* Read header.  */
  if (readn(ise->me->sock, (u_char *)reply->buf, LINE_HEADER_LEN)
      != LINE_HEADER_LEN)
    return -1;

  /* Decode line header.  */
  reply->zg = ise->is->ms->zg;

  line_header_decode (reply);

  if (reply->length<LINE_HEADER_LEN || reply->length>LINE_MESSAGE_MAX)
    return -1;

  nbytes = reply->length - LINE_HEADER_LEN;

  if (nbytes != 0 &&
      readn(ise->me->sock, (u_char *)reply->str, nbytes) != nbytes)
    return -1;

    if (reply->code == LINE_CODE_CONFIG_REQUEST) {
      thread_add_event (reply->zg,
                        _imi_server_config_next_vr,
                        ise,
                        reply->vr_id);
    }
  } while (reply->code == LINE_CODE_CONFIG_REQUEST);

  return reply->length;
}

static void
_prepare_error_reply(struct line *reply, char *errmsg)
{
  pal_snprintf(&reply->buf[LINE_HEADER_LEN],LINE_BODY_LEN-1,"%s\n", errmsg);
  reply->str    = &reply->buf[LINE_HEADER_LEN];
  reply->code   = LINE_CODE_ERROR;
  line_header_encode (reply);
}

int
imi_server_send_line (struct imi_server_entry *ise,
                      struct line *line, struct line *reply)
{
  int ret;

  /* Encode line header. */
  line_header_encode (line);

  /* Send message to the protocol module. */
  ret = writen (ise->me->sock, (u_char *)line->buf, line->length);
  if (ret != line->length)  {
    _prepare_error_reply(reply,"% Cannot send command to protocol daemon");
    return -1;
  }
  /* Receive reply: we shall receive at least line header */
  ret = imi_server_recv_line (ise, reply);
  if (ret < LINE_HEADER_LEN) {
    _prepare_error_reply(reply, "% Cannot get reply from protocol daemon");
    return -1;
  }
  return line->length;
}

/* Send notification message to IMI client (except IMISH).
   This was introduced in order to send LINE_CODE_SESSION_DEL to delete
   session info stored locally at PM at the time the IMISH is terminated.
   */
int
imi_server_send_notify (struct imi_server_entry *ise,
                        struct line *line,
                        u_char code)
{
  line->code = code;
  line->str  = NULL;  /* Send only line message header */
  line_header_encode (line);
  return writen (ise->me->sock, (u_char *)line->buf, line->length);

  /* We do not expect any reply */
}

/* Flushing all the configuration sessions states maintained
   at the protocol daemons.
   This happens in 2 cases:
   1. The IMISH is being terminated.
   2. The IMISH CLI is switching back to the CONFIG_MODE or higher.
      NOTE: A single configuration command can be sent to many deamons
            so flushing just the last contacted daemon will not
   The "line" parameter here is the IMISH client's "line" object.
   It must contain the valid IMISH process/socket id
*/

void
imi_server_flush_module(struct line *line)
{
  int i;
  struct imi_server_entry *ise;
  struct imi_server_client *isc;
  struct message_handler *ms = imim->imh;
  struct imi_server *is = ms->info;

  modbmap_t module = line->module;
  line->module = PM_EMPTY;

  for (i=APN_PROTO_NSM; i<vector_max(is->client); i++)
  {
    if (i == APN_PROTO_IMISH || i == APN_PROTO_IMI)
      continue;
    if ((isc = vector_slot (is->client, i)))
        for (ise = isc->head; ise; ise = ise->next)
        {
          MODBMAP_SET (line->module, i);
          imi_server_send_notify (ise, line, LINE_CODE_CONFSES_CLR);
          MODBMAP_UNSET (line->module, i);
        }
  }
  line->module = module;
}

/* Proxy IMI shell input to proper protocol modules.  */
int
imi_server_proxy (struct line *line, struct line *reply, bool_t do_mcast)
{
  int i;
  int ret;
  int sent_cnt=0;
  struct imi_server_entry *ise;
  struct imi_server_client *isc;
  struct message_handler *ms = imim->imh;
  struct imi_server *is = ms->info;
  struct imi_master *im = line->vr->proto;

  for (i = APN_PROTO_NSM; i < vector_max (is->client); i++)
    if ((isc = vector_slot (is->client, i)))
      if (MODBMAP_ISSET (im->module, isc->module_id))
        if (MODBMAP_ISSET (line->module, isc->module_id))
          for (ise = isc->head; ise; ise = ise->next)
          {
            /* Send CLI message.  */
            ret = imi_server_send_line (ise, line, reply);

            /* Bypass any error checking in the multicast delivery mode. */
            if (do_mcast) {
              continue;
            }
            if (ret < 0)
              return -1;
            sent_cnt++;

            if (reply->code == LINE_CODE_ERROR)
            {
                return -1;
            }
          }
  /* If neither PM is running let the user know. */
  if (sent_cnt==0 && !do_mcast) {
    _prepare_error_reply(reply,"% Protocol daemon is not running");
    return -2;
  }
  return 0;
}

int
imi_server_send_end_config (struct imi_server_entry *ise, struct line *line)
{
  int ret;

  line->code = LINE_CODE_CONFIG_END;
  line->str[0] = '\0';
  line_header_encode (line);

  ret = writen (ise->me->sock, (u_char *)line->buf, line->length);
  if (ret != line->length)
    return ret;

  return 0;
}


/*======================================================================
 * Downloading configuration at the PM startup.
 *  Configuration is copied from config file or from locally stored in IMI.
 *  If configuration is stored locally in IMI, the respective content of
 *    config file is ignored.
 *======================================================================
 */
#define _IS_MODE_SET(mode)       ((mode)>=CONFIG_MODE && (mode)<MAX_MODE)
#define _IS_DTYPE_SET(dtype)     ((dtype)>=CFG_DTYP_CONFIG && (dtype)<CFG_DTYP_MAX)
#define _IS_NOT_DTYPE_SET(dtype) (! _IS_DTYPE_SET(dtype))
#define _UNSET_DTYPE(dtype)      dtype = CFG_DTYP_MAX

/*----------------------------------------------------------------------
 * _parse_config_cmd - Parse command line from configuration file
 *----------------------------------------------------------------------
 */

static int
_imi_server_parse_config_cmd (struct cli_tree *ctree,
                              struct line     *line,
                              cfgDataType_e   *new_dtype,
                              int             *new_mode,
                              modbmap_t       *modules)
{
  struct cli_node *node;
  int    ret;

  *new_dtype = CFG_DTYP_MAX;
  *new_mode  = line->mode; /* by default, the new mode is eq. the current mode */
  *modules   = PM_EMPTY;

    ret = cli_parse (ctree, line->mode, line->privilege, line->str, 1, 0);

    switch (ret)
    {
    case CLI_PARSE_SUCCESS:
      node = ctree->exec_node;
      if (_IS_DTYPE_SET((int)node->cel->data_type)) {
        *new_dtype = node->cel->data_type;
      }
      if (_IS_MODE_SET((int)node->cel->new_mode)) {
        *new_mode  = node->cel->new_mode;
      }
      *modules   = node->cel->module;
      break;

    case CLI_PARSE_INCOMPLETE:
    case CLI_PARSE_INCOMPLETE_PIPE:
    case CLI_PARSE_AMBIGUOUS:
    case CLI_PARSE_NO_MATCH:
    case CLI_PARSE_NO_MODE:
      /* Switch to the CONFIG_MODE and try again. */
      cli_free_arguments (ctree);
      line->mode = CONFIG_MODE;
      *new_mode  = CONFIG_MODE;

      ret = cli_parse (ctree, line->mode, line->privilege, line->str, 1, 0);

      if (ret == CLI_PARSE_SUCCESS)
      {
        node = ctree->exec_node;
        if (_IS_DTYPE_SET(node->cel->data_type)) {
          *new_dtype = node->cel->data_type;
        }
        if (_IS_MODE_SET(node->cel->new_mode)) {
          *new_mode  = node->cel->new_mode;
        }
        *modules   = node->cel->module;
      }
      break;
    case CLI_PARSE_EMPTY_LINE:
    default:
      break;
    }
    cli_free_arguments (ctree);
  return ret;

} /* end of _parse_cmd_line() */

/*-------------------------------------------------------------------------
 * _send_config_cmd - Sends a single command line to PM.
 *-------------------------------------------------------------------------
 */
static int
_imi_server_send_config_cmd (struct imi_server_entry *ise,
                             struct line *line,
                             struct line *reply,
                             modbmap_t    modules,
                             int          new_mode,
                             int         *prev_mode)
{
  MODBMAP_UNSET(modules, APN_PROTO_IMI);

  if (modbmap_check(modules, line->module))
  {
    if (reply->code == LINE_CODE_SUCCESS)
    {
      if (imi_server_send_line (ise, line, reply) < 0)
      {
        return(-1);
      }
      /* Record the current CLI mode if PM returns error.
         We must skip all commands until we come back to this mode.
      */
      if (reply->code != LINE_CODE_SUCCESS)
      {
        *prev_mode = line->mode;
      }
    }
  }
/*
  else
  {
    printf("discard-cmd-->%s \n", line->str);
  }
*/
  /* We do not execute local IMI functions. Instead, if mode changed, we assign
     the mode from the new_mode variable.
  */
  line->mode = new_mode;

  /* Reset the reply code if the mode doesn't change.
     Otherwise, silently skip all the command of the new_mode.
   */
  if (new_mode == *prev_mode)
  {
    reply->code = LINE_CODE_SUCCESS;
  }
  return(0);
} /* end of _send_config_cmd(...) */


/*-------------------------------------------------------------------------
 * _encode_imi_config_cmds - Encode a single data type stored at IMI.
 *-------------------------------------------------------------------------
 */
int
_imi_server_encode_imi_config_cmds (struct apn_vr *vr,
                                    cfgDataType_e dtype,
                                    cfg_vect_t *cv)
{
  switch (dtype)
  {
    case CFG_DTYP_IMI_HOST_SERVICE:
      host_service_encode (vr->host, cv);
      break;

#ifdef HAVE_DHCP_SERVER
    case CFG_DTYP_IMI_DHCPS_SERVICE:
      imi_dhcps_service_encode (DHCP, cv);
      break;
#endif /* HAVE_DHCP_SERVER */

    case CFG_DTYP_IMI_HOST:
      host_config_encode (vr->host, cv);
      break;

#ifdef HAVE_CRX
    case CFG_DTYP_IMI_CRX_DEBUG:
      /*    crx_debug_config_encode (cli); */
      break;
#endif /* HAVE_CRX */

#ifdef HAVE_PPPOE
    case CFG_DTYP_IMI_PPPOE:
      imi_pppoe_config_encode (PPPOE, cv);
      break;
#endif /* HAVE_PPPOE */

#ifdef HAVE_DNS
    case CFG_DTYP_IMI_DNS:
      imi_dns_config_encode (DNS, cv);
      break;
#endif /* HAVE_DNS */

#ifdef HAVE_DHCP_SERVER
    case CFG_DTYP_IMI_DHCPS:
      imi_dhcps_config_encode (DHCP, cv);
      break;
#endif /* HAVE_DHCP_SERVER */

#ifdef HAVE_CUSTOM1
    case CFG_DTYP_IMI_SNMP:
      imi_snmp_config_encode (cv);
      break;
#endif /* HAVE_CUSTOM1 */

#ifdef HAVE_VR
    case CFG_DTYP_IMI_VR:
      if (vr->id == 0)
      {
        imi_vr_config_encode (cv);
      }
      break;
#endif /* HAVE_VR */

    case CFG_DTYP_KEYCHAIN:
      keychain_config_encode (vr, cv);
      break;

    case CFG_DTYP_ACCESS_IPV4:
      config_encode_access_ipv4 (vr, cv);
      break;

    case CFG_DTYP_PREFIX_IPV4:
      config_encode_prefix_ipv4 (vr, cv);
      break;
#ifdef HAVE_IPV6
    case CFG_DTYP_ACCESS_IPV6:
      config_encode_access_ipv6 (vr, cv);
      break;

    case CFG_DTYP_PREFIX_IPV6:
      config_encode_prefix_ipv6 (vr, cv);
      break;
#endif
    case CFG_DTYP_RMAP:
      route_map_config_encode (vr, cv);
      break;

#ifdef HAVE_NAT
    case CFG_DTYP_IMI_NAT:
      imi_nat_config_encode (cv);
      break;

    case CFG_DTYP_IMI_VIRTUAL_SERVER:
      imi_virtual_server_config_encode (cv);
      break;
#endif /* HAVE_NAT */

#ifdef HAVE_NTP
    case CFG_DTYP_IMI_NTP:
      imi_ntp_config_encode (cv);
      break;
#endif /* HAVE_NTP. */

#ifdef HAVE_APS_TOOLKIT
    case CFG_DTYP_IMI_EXTERN:
      imi_extern_config_encode ();
      break;
#endif /* HAVE_APS_TOOLKIT */

#ifdef HAVE_CRX
    case CFG_DTYP_IMI_CRX:
      crx_config_encode (cv);
      break;
#endif /* HAVE_CRX */

#ifdef HAVE_IMISH
    case CFG_DTYP_LINE:
      imi_line_config_encode (vr->proto, cv);
      break;
#endif
    default:
      return (-1);
  }
  return (0);
}

/*-------------------------------------------------------------------------
 * _send_imi_config_cmds - Configure PM with IMI stored config data of selected
 *                         data type.
 * When we are at this point we always come back to the CONFIG_MODE.
 *-------------------------------------------------------------------------
 */
int
_imi_server_send_imi_config_cmds (struct imi_server_entry *ise,
                                  cfgDataType_e dtype,
                                  cfg_vect_t *cv)
{
  int ret;
  struct line reply;
  struct line saved_line = ise->line;
  struct line *line = &ise->line;
  struct lib_globals *zg = ise->is->ms->zg;
  struct cli_tree *ctree = zg->ctree;
  cfgDataType_e new_dtype;
  int vix, slen;
  char *cmd;

  modbmap_t modules = PM_EMPTY;

  int new_mode = MAX_MODE;    /* New mode given by the parser.        */
  int prev_mode= CONFIG_MODE; /* Set to skip commands after mode change command has failed. */
  line->mode   = CONFIG_MODE; /* Before execution of current command. */

  cv = cfg_vect_init(cv);

  reply.code = LINE_CODE_SUCCESS;

  _imi_server_encode_imi_config_cmds (line->vr, dtype, cv);

  VECTOR_LOOP(cv->cv_v, cmd, vix)
  {
    pal_strncpy(line->str, cmd, LINE_BODY_LEN-1);

    /* Clear trailing new line.  */
    slen = pal_strlen (line->str);
    if (--slen >= 0)
    {
      line->str[slen] = '\0';
    }
    modules = PM_EMPTY;
    /* Parse the cmd to get the current data type and the command exit mode (if any).
       Return to CONFIG_MODE if parsing fails in the current mode.
     */
    ret = _imi_server_parse_config_cmd (ctree, line,
                                        &new_dtype,
                                        &new_mode,
                                        &modules);
    if (ret != CLI_PARSE_SUCCESS)
    {
      /* Skip the line and get the next one. */
      continue;
    }

 /*   printf("send-imi-cmd->%s \n", line->str); */
    if (_imi_server_send_config_cmd (ise, line, &reply, modules,
                                     new_mode,
                                     &prev_mode) != 0)
    {
      ise->line = saved_line;
      cfg_vect_reset(cv);
      return(-1);
    }
  }
  ise->line = saved_line;
  cfg_vect_reset(cv);
  return(0);
}


/*-----------------------------------------------------------------------
 * _send_config_to_pm
 *
 * Read file and selectively send the config to PM from the IMI or config
 * file.
 *-----------------------------------------------------------------------
 */
static int
_imi_server_send_config_to_pm(struct imi_server_entry *ise,
                              FILE *fp,
                              struct line *line)
{
  int ret;
  int length;
  struct line reply;
  struct lib_globals *zg = ise->is->ms->zg;
  struct cli_tree *ctree = zg->ctree;
  cfg_vect_t *cv;

  cfgDataType_e new_dtype;
  cfgDataType_e skip_dtype;
  cfgDataType_e copy_dtype;
  cfgDataType_e cur_dtype = CFG_DTYP_MAX;
  int new_mode = MAX_MODE;     /* New mode given by the parser. */
  int prev_mode = line->mode;  /* Set to skip commands after mode change command has failed. */
  modbmap_t modules=PM_EMPTY;
  int cur_dtype_ix = 0;
  int new_dtype_ix = 0;

  reply.code  = LINE_CODE_SUCCESS;

  _UNSET_DTYPE(new_dtype);
  _UNSET_DTYPE(skip_dtype);
  _UNSET_DTYPE(copy_dtype);

  cv = cfg_vect_init(NULL);
/*  printf("\n"); */

  while (pal_fgets (line->str, LINE_BODY_LEN - 1, fp))
  {
    /* Clear trailing new line.  */
    length = pal_strlen (line->str);
    if (--length >= 0)
    {
      line->str[length] = '\0';
    }
    /* Parse the cmd to get the current data type and mode (if any).
       Return to CONFIG_MODE if parsing fails in the current mode.
     */
    ret = _imi_server_parse_config_cmd (ctree, line,
                                        &new_dtype,
                                        &new_mode,
                                        &modules);
    if (ret != CLI_PARSE_SUCCESS) {
      /* Skip the line and get the next one. */
      continue;
    }
    /* If parser exited to CONFIG_MODE and parsed the new command successfuly
       set the reply->code to SUCCESS to let the commad to go to PM.
     */
    if (line->mode == CONFIG_MODE)
    {
      reply.code = LINE_CODE_SUCCESS;
    }
    /* Input skip mode takes place if we replace a section from startup
       file conifg with locally - in IMI - stored config.
       For IMI stored commands that are going to PM the new_dtype is always set.
     */
    if (_IS_DTYPE_SET(skip_dtype) && (skip_dtype == new_dtype))
    {
/*      printf("skip-nxt-cmd->%s \n", line->str); */
      continue;
    }
    /* Passing a section of PM specific config to PM.
       Mode change command is stamped with data type, which is telling
       us that this is a PM specific command.
    */
    if (_IS_DTYPE_SET(copy_dtype) &&
        ((copy_dtype == new_dtype) ||_IS_NOT_DTYPE_SET(new_dtype)))
    {
/*      printf("copy-nxt-cmd->%s \n", line->str); */
      if (_imi_server_send_config_cmd (ise, line, &reply, modules,
                                       new_mode,
                                       &prev_mode) != 0) {
        return (-1);
      }
      continue;
    }
    /* For any orphaned - out of any command mode context
       or command not stamped with data type, or not IMI only command
       send to PM.
     */
    if (_IS_NOT_DTYPE_SET(new_dtype))
    {
      /* If first command is IMI only command, skip the whole section
         of commands.
      */
      if (modbmap_isempty(modbmap_sub(modules, PM_IMI)))
      {
        continue;
      }
/*      printf("copy-\?\?\?-cmd->%s \n", line->str); */
      if (_imi_server_send_config_cmd (ise, line, &reply, modules,
                                       new_mode,
                                       &prev_mode) != 0) {
        return (-1);
      }
      continue;
    }
    /* If new_dtype - that determined from file content - is more
       advanced to cur_dtype, we must force the cur_dtype to catch up
       with the new_dtype.
       Write all IMI located config for all data types in-between.
     */
    _UNSET_DTYPE(skip_dtype);
    _UNSET_DTYPE(copy_dtype);

    /* Get the sequencer index for new data type. */
    new_dtype_ix = cfg_seq_get_index(new_dtype);

    while (cur_dtype_ix <= new_dtype_ix)
    {
      if (cfg_seq_is_imi_dtype(cur_dtype_ix))
      {
        cur_dtype = cfg_seq_get_dtype(cur_dtype_ix);
        if (_imi_server_send_imi_config_cmds (ise, cur_dtype, cv) != 0) {
          return (-1);
        }
      }
      /* Advance the sequencer index. */
      cur_dtype_ix++;
      prev_mode = line->mode;
    }
    /* Skip the new dtype section in the config file or copy it to the PM.
       Do it until the new valid data type is detected.
    */
    if (cfg_seq_is_imi_dtype(new_dtype_ix))
    {
      skip_dtype = new_dtype;
/*      printf("skip-1st-cmd->%s \n", line->str); */
    }
    else
    {
      copy_dtype = new_dtype;
      reply.code = LINE_CODE_SUCCESS;
/*      printf("copy-1st-cmd->%s \n", line->str); */
      if (_imi_server_send_config_cmd (ise, line, &reply, modules,
                                       new_mode,
                                       &prev_mode)  != 0) {
        return (-1);
      }
    }
  }
  return (0);
}

/*-----------------------------------------------------------------------
 * imi_server_send_config
 *
 * Open config file and selectively send the config to PM from the IMI
 * or config file.
 *-----------------------------------------------------------------------
 */
int
imi_server_send_config (struct imi_server_entry *ise)
{
  int ret;
  FILE *fp;
  struct line *line = &ise->line;

  /* Prepare line header info.  */
  line->module    = modbmap_id2bit (ise->isc->module_id);
  line->code      = LINE_CODE_COMMAND;
  line->mode      = CONFIG_MODE;
  line->privilege = PRIVILEGE_MAX;
  line->str       = &line->buf[LINE_HEADER_LEN];
  line->vr_id     = line->vr->id;
  line->config_id = 0; /* XXX */
  line->pid       = 0;

  LIB_GLOB_SET_VR_CONTEXT(line->vr->zg, line->vr);

  /* Open the file. */
  fp = pal_fopen (line->vr->host->config_file, PAL_OPEN_RO);
  if (fp == NULL)
  {
    /* Send "end" marker. */
    imi_server_send_end_config (ise, line);
    return (0);
  }
  ret = _imi_server_send_config_to_pm(ise, fp, line);

  if (! ret) {
  /* Send "end" marker. */
    imi_server_send_end_config (ise, line);
  }
  pal_fclose (fp);

#ifdef HAVE_VLOGD
  /* Here we need to update VLOGD with all VR instances.
     We do it only after downloading the main VR config
     in the context of VR 0 and configuring the VLOGD.
  */
  if (line->vr_id == 0 && ise->isc->module_id == APN_PROTO_VLOG)
  {
    imi_vlog_send_all_vrs();
  }

#endif
  return (ret);
}


int
imi_server_line_send_error (struct imi_server_entry *ise, char *err_str)
{
  struct line *line;

  line = &ise->line;

  line->code = LINE_CODE_ERROR;
  line->mode = IMISH_MODE;

  line->str = &line->buf[LINE_HEADER_LEN];
  pal_strcpy(line->str, err_str);;

  line_header_encode (line);

  writen (ise->me->sock, (u_char *)line->buf, line->length);

  /* Always return error forcing connection close */
  return -1;
}

int
imi_server_context_set (struct imi_server_entry *ise)
{
  struct imi_master *im;
  struct line *line;
  int ret;
  int i;
  int type;
  char err_str[128];

  if (ise->line.str[0] != '\0')
    ise->line.vr = apn_vr_lookup_by_name (imim, ise->line.str);

  if (ise->line.vr == NULL)
    return imi_server_line_send_error (ise, "No VR set\n");

  im = ise->line.vr->proto;
  ise->line.vr_id = ise->line.vr->id;

  if (CHECK_FLAG ((ise->line.flags), LINE_FLAG_UP))
    return imi_server_line_send_error (ise, "VR Context already set\n");

  if (ise->line.key == LINE_TYPE_CONSOLE)
    type = LINE_TYPE_CONSOLE;
  else
    type = LINE_TYPE_VTY;
  ise->line.type = ise->line.key;

  for (i = 0; i < vector_max (im->lines[type]); i++)
    if ((line = vector_slot (im->lines[type], i)))
      if (! CHECK_FLAG (line->flags, LINE_FLAG_UP))
      {
        SET_FLAG (line->flags, LINE_FLAG_UP);

        /* Copy the line information.  */
        ise->line.flags = line->flags;
        ise->line.config = line->config;
        ise->line.index = i;
        ise->line.privilege = line->privilege;
        ise->line.maxhist = line->maxhist;
        ise->line.exec_timeout_min = line->exec_timeout_min;
        ise->line.exec_timeout_sec = line->exec_timeout_sec;

        /* Encode line header.  */
        line_header_encode (&ise->line);

        /* Send message to the IMI shell.  */
        ret = writen (ise->me->sock,(u_char *)ise->line.buf, ise->line.length);
        if (ret != ise->line.length)
          return ret;

        return 0;
      }
      /* Let message handler to handle disconnect event.  */

  pal_sprintf(err_str, "All %d lines for VR:%s are busy."
              " Try again later\n",
              i,
              ise->line.vr->name ? ise->line.vr->name : "PVR");

  return imi_server_line_send_error (ise, err_str);
}

int
imi_server_line_connect (struct imi_server_entry *ise)
{
  int ret;

  /* Set initial information. */
  ise->line.zg = ise->is->ms->zg;
  ise->line.sock = ise->me->sock;
  /*  ise->line.str[0] = '\0'; */

  /* Encode line header.  */
  line_header_encode (&ise->line);

  /* Send message to the IMI shell.  */
  ret = writen (ise->me->sock, (u_char *)ise->line.buf, ise->line.length);
  if (ret != ise->line.length)
    return ret;

  return 0;
}

#ifdef HAVE_CUSTOM1
/* Please add modle name bellow when PacOS protocol module is added */
enum {
  AP_PM_NSM = 0,
  AP_PM_PIM,
  AP_PM_RIP,
  AP_PM_OSPF,
#ifdef HAVE_RSTPD
  AP_PM_RSTP,
#endif /* HAVE_RSTPD */
#ifdef HAVE_MSTPD
  /* AP_PM_MSTP, */
#endif /* HAVE_MSTPD */
  AP_PM_COUNT,
};
#endif /* HAVE_CUSTOM1 */

int
imi_server_client_connect (struct imi_server_entry *ise)
{
  module_id_t module_id = modbmap_bit2id (ise->line.module);
  struct imi_server *is = ise->is;
  struct imi_globals *imig = is->ms->zg->proto;
  struct imi_server_client *isc;

  /* Drop the duplicate connection request.  */
  if (ise->isc != NULL)
    return imi_server_line_send_error (ise, "Duplicate connection\n");

  /* If this type of client has not been registered yet,
     create a new "server_client" entry and link to IMI server.  */
  if ((isc = imi_server_client_lookup (is, module_id)) == NULL)
    isc = imi_server_client_new (is, module_id);

  ise->prev = isc->tail;
  if (isc->head == NULL)
    isc->head = ise;
  else
    isc->tail->next = ise;
  isc->tail = ise;

  /* Remember which IMI client this IMI client entry belongs to.  */
  ise->isc = isc;

  /* Add new IMI client instance entry to a given IMI "client type" list  */
  if (module_id == APN_PROTO_IMISH)
    return imi_server_line_connect (ise);
  else
  {
    /* Kick out IMISH/VTY to the configuration mode.  */
    if (imig->imi_callback[IMI_CALLBACK_CONNECT])
      (*imig->imi_callback[IMI_CALLBACK_CONNECT]) (NULL);

#ifdef HAVE_CUSTOM1
    {
#define ZINIT_DONE_FILE "/var/run/zinit_done"
      static int pm_count = 0;
      FILE *init_fp;
      int ret = imi_server_send_config (ise);
      pm_count++;
      /* create file to show all protocol module loading end. */
      if (pm_count == AP_PM_COUNT) {
        init_fp = pal_fopen(ZINIT_DONE_FILE, PAL_OPEN_RW);
          if(init_fp) {
          pal_fclose(init_fp);
        }
      }
      /* create file to show nsm loading end. */
      if (module_id == APN_PROTO_NSM) {
        init_fp = pal_fopen(NSM_DONE_FILE, PAL_OPEN_RW);
          if(init_fp) {
          pal_fclose(init_fp);
        }
      }
      return ret;
    }
#else /* HAVE_CUSTOM1 */
    return imi_server_send_config (ise);
#endif /* HAVE_CUSTOM1 */
  }
}

/* Clean up line.  */
void
imi_server_line_clean (struct line *line)
{
  struct apn_vr *vr;

  /* Unset terminal monitor.  */
#ifdef HAVE_VLOGD
  imi_vlog_send_term_cmd (line, NULL,
                          IMI_VLOG_TERM_CMD__DETACH_ALL_VRS);
#else
  imi_terminal_monitor_unset (line->vr_id, line->type, line->index);
#endif /* HAVE_VLOGD. */

  /* Unlock the configuration terminal.  */
  if ((vr = apn_vr_lookup_by_id (line->zg, line->vr_id)))
    host_config_unlock (vr->host, &line->cli);

  if (line->user)
  {
    XFREE (MTYPE_TMP, line->user);
    line->user = NULL;
  }
  if (line->tty)
  {
    XFREE (MTYPE_TMP, line->tty);
    line->tty = NULL;
  }
}

/* "line" client disconnected.  */
void
imi_server_line_disconnect (struct imi_server_entry *ise)
{
  struct line *line;
  struct apn_vr *vr;
  struct imi_master *im;

  imi_server_line_clean (&ise->line);

  if (CHECK_FLAG(ise->line.cli.flags, CLI_FROM_PVR))
    {
      /* This line belongs to the PVR. */
      vr = apn_vr_get_privileged(ise->me->zg);
    }
  else
    {
      vr = apn_vr_lookup_by_id (ise->me->zg, ise->line.vr_id);
    }
  if (vr != NULL)
    if ((im = vr->proto))
      {
        if ((line = vector_slot (im->lines[ise->line.type], ise->line.index)))
          UNSET_FLAG (line->flags, LINE_FLAG_UP);
      }
}

#ifdef HAVE_IMISH
/* Send SIGTERM signal to the specific VR IMISH to disconnect.  */
int
imi_server_line_vr_close (struct apn_vr *vr)
{
  struct imi_server_client *isc;
  struct imi_server_entry *ise;
  struct imi_server *is = imim->imh->info;

  if ((isc = imi_server_client_lookup (is, APN_PROTO_IMISH)))
    for (ise = isc->head; ise; ise = ise->next)
      if (CHECK_FLAG (ise->line.flags, LINE_FLAG_UP))
        if (vr == NULL || ise->line.vr_id == vr->id)
          pal_kill (ise->line.pid, SIGTERM);

  return 0;
}

/* Send SIGUSR2 signal to the specific VR IMISH to move to CONFIG mode.  */
int
imi_server_line_vr_unbind (struct apn_vr *vr)
{
  struct imi_server_client *isc;
  struct imi_server_entry *ise;
  struct imi_server *is = imim->imh->info;

  if ((isc = imi_server_client_lookup (is, APN_PROTO_IMISH)))
    for (ise = isc->head; ise; ise = ise->next)
      if (CHECK_FLAG (ise->line.flags, LINE_FLAG_UP))
        if (vr == NULL || ise->line.vr_id == vr->id)
          if (ise->line.cli.mode == INTERFACE_MODE)
            pal_kill (ise->line.pid, SIGUSR2);

  return 0;
}

/* Send SIGUSR2 signal to kick IMISH out to the CONFIG mode.  */
int
imi_server_line_pm_connect (struct apn_vr *vr)
{
  struct imi_server_client *isc;
  struct imi_server_entry *ise;
  struct imi_server *is = imim->imh->info;

  if ((isc = imi_server_client_lookup (is, APN_PROTO_IMISH)))
    for (ise = isc->head; ise; ise = ise->next)
      if (CHECK_FLAG (ise->line.flags, LINE_FLAG_UP))
        if (vr == NULL || ise->line.vr_id == vr->id)
          if (ise->line.cli.mode > CONFIG_MODE)
            pal_kill (ise->line.pid, SIGUSR2);

  return 0;
}

/* Send SIGINT signal to IMISH to reset the line.  */
int
imi_server_line_pm_disconnect (struct apn_vr *vr)
{
  struct imi_server_client *isc;
  struct imi_server_entry *ise;
  struct imi_server *is = imim->imh->info;

  if ((isc = imi_server_client_lookup (is, APN_PROTO_IMISH)))
    for (ise = isc->head; ise; ise = ise->next)
      if (CHECK_FLAG (ise->line.flags, LINE_FLAG_UP))
        if (vr == NULL || ise->line.vr_id == vr->id)
          pal_kill (ise->line.pid, SIGINT);

  return 0;
}
#endif /* HAVE_IMISH */


/* PM connects to IMI server.  */
int
imi_server_connect (struct message_handler *ms,
                    struct message_entry *me, pal_sock_handle_t sock)
{
  /* Make the client socket blocking. */
  pal_sock_set_nonblocking (sock, PAL_FALSE);

  /* Create IMI server entry.  */
  imi_server_entry_new (ms, me);

  return 0;
}


/* Client disconnect from IMI server.  This function should not be
   called directly.  Message library call this.  */
int
imi_server_disconnect (struct message_handler *ms,
                       struct message_entry *me, pal_sock_handle_t sock)
{
  struct imi_server_entry *ise = me->info;
  struct imi_server_client *isc = ise->isc;
  struct imi_server *is = ise->is;
  struct imi_globals *imig = ms->zg->proto;

  if (CHECK_FLAG (ise->line.flags, LINE_FLAG_UP))
    imi_server_line_disconnect (ise);

  /* Reset the pending IMISH line when PM dies.  */
  if (isc && isc->module_id != APN_PROTO_IMISH)
  {
    if (imig->imi_callback[IMI_CALLBACK_DISCONNECT])
      (*imig->imi_callback[IMI_CALLBACK_DISCONNECT]) (NULL);
  }
  else if (isc && isc->module_id == APN_PROTO_IMISH)
  { /* IMISH disconnect: send message to all PMs to delete
       this IMISH's configuration session states.
     */
    imi_server_flush_module(&ise->line);
  }
  /* Free IMI server entry for the client. */
  imi_server_entry_free (ise);

  /* Free IMI server client if all clients were gone.  */
  if (isc && isc->head == NULL)
    imi_server_client_free (is, isc->module_id);

  me->info = NULL;

  return 0;
}

/* Call back function to read IMI line header. */
int
imi_server_read (struct message_handler *ms,
                 struct message_entry *me, pal_sock_handle_t sock)
{
  struct imi_server_entry *ise = me->info;
  int length;
  int ret;
  struct lib_globals *zg = ms->zg;

  /* Read IMI line header.  */
  length = readn (sock, (u_char *)ise->line.buf, LINE_HEADER_LEN);

  /* Let message handler to handle disconnect event.  */
  if (length != LINE_HEADER_LEN)
    return -1;
  ms->zg->vr_in_cxt = ise->line.vr;

  /* Decode line header.  */
  line_header_decode (&ise->line);

  /* Check header length.  Let message handler to handle disconnect event
     if the length is smaller than the line message header length.  */
  if (ise->line.length < LINE_HEADER_LEN)
    return -1;

  if (ise->line.length > LINE_MESSAGE_MAX)
  {
    zlog_info (zg, "Line Message Length is %d ", length);
    zlog_info (zg, "Line Message Length exceed maximum message length");
    return -1;
  }

  /* Check client module ID.  */
  if (modbmap_bit2id (ise->line.module) == APN_PROTO_UNSPEC)
    return -1;

  length = ise->line.length - LINE_HEADER_LEN;
  if (length > 0)
  {
    ret = readn (sock, (u_char *)ise->line.str, length);
    if (ret != length)
      return -1;
  }

  /* Check VR.  */
  ise->line.vr = apn_vr_lookup_by_id (zg, ise->line.vr_id);
  if (ise->line.vr == NULL)
    return -1;
   ms->zg->vr_in_cxt = ise->line.vr;
 
  LIB_GLOB_SET_VR_CONTEXT(zg, ise->line.vr);

  if (ise->line.code == LINE_CODE_CONNECT)
    return imi_server_client_connect (ise);
  else if (ise->line.code == LINE_CODE_CONTEXT_SET)
    return imi_server_context_set (ise);
  else if (ise->line.code == LINE_CODE_CONFIG_REQUEST)
    return imi_server_send_config (ise);
  else if (CHECK_FLAG (ise->line.flags, LINE_FLAG_UP))
    return imi_line_parser (&ise->line);

  return 0;
}

/* Initialize IMI PM server.  */
int
imi_server_init (struct lib_globals *zg)
{
  int ret;
  struct imi_server *is;
  struct message_handler *ms;

  /* When message server works fine, go forward to create IMI server
     structure.  */
  is = XCALLOC (MTYPE_IMI_SERVER, sizeof (struct imi_server));
  if (is == NULL)
    return 0;

  pal_mem_set (is, 0, sizeof (struct imi_server));
  /* Create message server.  */
  ms = message_server_create (zg);
  if (ms == NULL)
    return 0;

  /* Set server type. */
#ifdef HAVE_TCP_MESSAGE
  message_server_set_style_tcp (ms, IMI_LINE_PORT);
#else
  message_server_set_style_domain (ms, IMI_LINE_PATH);
#endif /* HAVE_TCP_MESSAGE */

  /* Set call back functions.  */
  message_server_set_callback (ms, MESSAGE_EVENT_CONNECT,
                               imi_server_connect);
  message_server_set_callback (ms, MESSAGE_EVENT_DISCONNECT,
                               imi_server_disconnect);
  message_server_set_callback (ms, MESSAGE_EVENT_READ_MESSAGE,
                               imi_server_read);

  /* Link each other.  */
  zg->imh = ms;
  ms->info = is;
  is->ms = ms;

  /* Prepare IMI client vector.  */
  is->client = vector_init (IMI_SERVER_CLIENT_VEC_MIN_SIZE);
  vector_set (is->client, is->client);                  /* Dummy. */

#ifdef HAVE_IMISH
  /* Set VR connection close callback function.  */
  apn_vr_add_callback (zg, VR_CALLBACK_CLOSE, imi_server_line_vr_close);

  /* Set VR unbind callback function.  */
  apn_vr_add_callback (zg, VR_CALLBACK_UNBIND, imi_server_line_vr_unbind);

  /* Set PM connection callback function.  */
  imi_add_callback (zg, IMI_CALLBACK_CONNECT, imi_server_line_pm_connect);

  /* Set PM disconnection callback function.  */
  imi_add_callback (zg, IMI_CALLBACK_DISCONNECT,
                    imi_server_line_pm_disconnect);
#endif /* HAVE_IMISH */

  /* Start the IMI server.  */
  ret = message_server_start (ms);

  return ret;
}

int
imi_server_shutdown (struct lib_globals *zg)
{
  int i;
  int max;
  struct message_handler *ms;
  struct imi_server *is;
  struct imi_server_client *isc;

  if ((ms = zg->imh) == NULL || (is = ms->info) == NULL)
    return 0;

#ifdef HAVE_IMISH
  /* Cleanup all the IMISH session.  */
  imi_server_line_vr_close (NULL);
#endif /* HAVE_IMISH */

  /* Free the IMI server client vector.  */
  max = vector_max (is->client);
  for (i = APN_PROTO_NSM; i < vector_max (is->client); i++)
    if ((isc = vector_slot (is->client, i)))
    {
      /* Free all the IMI server entry.  */
      imi_server_entry_free_all (isc);

      /* Free the IMI server client.  */
      imi_server_client_free (is, i);
    }
  vector_free (is->client);

  /* Free message server.  */
  message_server_delete (ms);

  XFREE (MTYPE_IMI_SERVER, is);

  zg->imh = NULL;

  return 0;
}

u_int16_t
imi_server_check_index (struct lib_globals *zg,  void *index,
                        void *sub_index, s_int32_t mode)
{
  int i;
  struct imi_server *is;
  struct message_handler *ms;
  struct imi_server_entry *ise;
  struct imi_server_client *isc;

  if ((ms = zg->imh) == NULL || (is = ms->info) == NULL)
    return PAL_FALSE;

  for (i = APN_PROTO_NSM; i < vector_max(is->client); i++)
    {
      if ((isc = vector_slot (is->client, i)))
        {
          for (ise = isc->head; ise; ise = ise->next)
            {
              if (ise->line.cli.mode != mode)
                continue;

              if (index != NULL
                  && ise->line.cli.index != index)
                continue;

              if (sub_index != NULL
                  && ise->line.cli.index_sub != sub_index)
                continue;

              return PAL_TRUE;
            }
        }
    }

  return PAL_FALSE;
}

void
imi_server_update_index (struct lib_globals *zg,  void *index,
                         void *sub_index, s_int32_t mode,  void *new_index,
                         void *new_sub_index)
{
  int i;
  struct imi_server *is;
  struct message_handler *ms;
  struct imi_server_entry *ise;
  struct imi_server_client *isc;

  if ((ms = zg->imh) == NULL || (is = ms->info) == NULL)
    return;

  for (i = APN_PROTO_NSM; i < vector_max(is->client); i++)
    {
      if ((isc = vector_slot (is->client, i)))
        {
          for (ise = isc->head; ise; ise = ise->next)
            {
              if (ise->line.cli.mode != mode)
                continue;

              if (index != NULL
                  && ise->line.cli.index != index)
                continue;

              if (sub_index != NULL
                  && ise->line.cli.index_sub != sub_index)
                continue;

              if (index != NULL)
                ise->line.cli.index = new_index;

              if (sub_index != NULL)
                ise->line.cli.index_sub = new_sub_index;
            }
        }
    }

  return;
}
