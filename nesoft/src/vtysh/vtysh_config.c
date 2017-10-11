/* Copyright (C) 2001-2003  All Rights Reserved. */

#include <pal.h>

#include "cli.h"
#include "show.h"
#include "line.h"
#include "host.h"
#include "fifo.h"
#include "network.h"
#include "version.h"

#include "vtysh.h"
#include "vtysh_cli.h"
#include "vtysh_config.h"
#include "vtysh_pm.h"

/* Line server. */
#define MTYPE_VTYSH_CONFIG          MTYPE_TMP
#define MTYPE_VTYSH_CONFIG_MODE     MTYPE_TMP
#define MTYPE_VTYSH_CONFIG_LINE     MTYPE_TMP

/* Allocate a new configuration line.  */
struct vtysh_config_line *
vtysh_config_line_new ()
{
  return XCALLOC (MTYPE_VTYSH_CONFIG_LINE, sizeof (struct vtysh_config_line));
}

/* Free configuration line.  */
void
vtysh_config_line_free (struct vtysh_config_line *line)
{
  XFREE (MTYPE_VTYSH_CONFIG_LINE, line);
}

/* Allocate a new configuration mode.  */
struct vtysh_config_mode *
vtysh_config_mode_new ()
{
  struct vtysh_config_mode *cm;

  cm = XCALLOC (MTYPE_VTYSH_CONFIG_MODE, sizeof (struct vtysh_config_mode));

  FIFO_INIT (&cm->main);
  FIFO_INIT (&cm->match);
  FIFO_INIT (&cm->set);
  FIFO_INIT (&cm->sub);

  return cm;
}

/* Free configuration mode.  */
void
vtysh_config_mode_free (struct vtysh_config_mode *cm)
{
  struct vtysh_config_line *line;

  while ((line = FIFO_HEAD (&cm->main)) != NULL)
    {
      FIFO_DEL (line);
      vtysh_config_line_free (line);
    }
  while ((line = FIFO_HEAD (&cm->match)) != NULL)
    {
      FIFO_DEL (line);
      vtysh_config_line_free (line);
    }
  while ((line = FIFO_HEAD (&cm->set)) != NULL)
    {
      FIFO_DEL (line);
      vtysh_config_line_free (line);
    }
  XFREE (MTYPE_VTYSH_CONFIG_MODE, cm);
}

/* Allocate a new configuration. */
struct vtysh_config *
vtysh_config_new ()
{
  struct vtysh_config *conf;

  conf = XCALLOC (MTYPE_VTYSH_CONFIG, sizeof (struct vtysh_config));
  conf->modes = vector_init (VECTOR_MIN_SIZE);

  return conf;
}

/* Free configuration. */
void
vtysh_config_free (struct vtysh_config *conf)
{
  int i;
  struct vtysh_config_mode *cm;
  struct vtysh_config_mode *sub;

  for (i = 0; i < vector_max (conf->modes); i++)
    if ((cm = vector_slot (conf->modes, i)) != NULL)
      {
        while ((sub = FIFO_HEAD (&cm->sub)) != NULL)
          {
            FIFO_DEL (sub);
            vtysh_config_mode_free (sub);
          }
        vtysh_config_mode_free (cm);
      }
  vector_free (conf->modes);
  XFREE (MTYPE_VTYSH_CONFIG, conf);
}

/* Add one line configuration.  */
void
vtysh_config_add (struct vtysh_config_fifo *fifo, char *str)
{
  struct vtysh_config_line *line;

  line = vtysh_config_line_new ();
  line->str = XSTRDUP (MTYPE_TMP, str);
  FIFO_ADD (fifo, line);
}

/* Flag configuration addition such as "ip access-list".  */
void
vtysh_config_add_unique (struct vtysh_config_fifo *fifo, char *str)
{
  struct vtysh_config_line *line;

  FIFO_LOOP (fifo, line)
    {
      if (pal_strcmp (line->str, str) == 0)
        return;
    }
  vtysh_config_add (fifo, str);
}

/* Lookup mode configuration.  */
struct vtysh_config_mode *
vtysh_config_lookup (struct vtysh_config *config, int mode)
{
  if (mode >= MAX_MODE)
    return NULL;

  return vector_lookup_index (config->modes, mode);
}

/* Get sub configuration.  */
struct vtysh_config_mode *
vtysh_config_sub_get (struct vtysh_config_mode *cm, char *str)
{
  struct vtysh_config_mode *find;

  FIFO_LOOP (&cm->sub, find)
    {
      if (pal_strcmp (find->str, str) == 0)
        return find;
    }

  find = vtysh_config_mode_new ();
  find->str = XSTRDUP (MTYPE_TMP, str);
  FIFO_ADD (&cm->sub, find);

  return find;
}

/* Get mode specific configuration.  */
struct vtysh_config_mode *
vtysh_config_get (struct vtysh_config *config, int mode, char *str,
                  enum vtysh_config_type type)
{
  struct vtysh_config_mode *cm;

  cm = vtysh_config_lookup (config, mode);
  if (! cm)
    {
      cm = vtysh_config_mode_new ();
      vector_set_index (config->modes, mode, cm);
    }

  switch (type)
    {
    case vtysh_config_type_add:
      vtysh_config_add (&cm->main, str);
      break;
    case vtysh_config_type_add_unique:
      vtysh_config_add_unique (&cm->main, str);
      break;
    case vtysh_config_type_sub:
      cm = vtysh_config_sub_get (cm, str);
      break;
    }

  config->mode = mode;
  config->cm = cm;

  return cm;
}

/* Add configuration to */
void
vtysh_config_line_add (struct vtysh_config *config,
                       struct vtysh_config_mode *cm, char *line)
{
  struct vtysh_config_fifo *fifo;

  if (config->mode == RMAP_MODE)
    {
      if (VTYSH_STRCMP (" match"))
        fifo = &cm->match;
      else if (VTYSH_STRCMP (" set"))
        fifo = &cm->set;
      else
        fifo = &cm->main;
      vtysh_config_add_unique (fifo, line);
    }
  else if (config->mode == INTERFACE_MODE)
    vtysh_config_add_unique (&cm->main, line);
  else
    vtysh_config_add (&cm->main, line);
}

/* Parse line and insert into vtyshm->config_top vector. */
void
vtysh_config_parse (void *arg, char *line)
{
  char c;
  struct vtysh_config_mode *cm;
  struct vtysh_config *config = arg;

  /* Sanity check.  */
  if (! line)
    return;

  /* Point out beginning of the line.  */
  c = *line;

  /* Empty line.  */
  if (c == '\0')
    return;

  /* Comment line.  */
  if (c == '!' || c == '#')
    return;

  /* Get current cm.  */
  cm = config->cm;

  /* Add to current node.  */
  if (c == ' ')
    {
      if (cm)
        vtysh_config_line_add (config, cm, line);
      else
        ;
      return;
    }

  if (VTYSH_STRCMP ("interface"))
    vtysh_config_get (config, INTERFACE_MODE, line, vtysh_config_type_sub);
  /* "debug" */
  else if (VTYSH_STRCMP ("debug"))
    vtysh_config_get (config, DEBUG_MODE, line, vtysh_config_type_add);
  /* "log" */
  else if (VTYSH_STRCMP ("log"))
    vtysh_config_get (config, CONFIG_MODE, line, vtysh_config_type_add_unique);
  else if (VTYSH_STRCMP ("key"))
    vtysh_config_get (config, KEYCHAIN_MODE, line, vtysh_config_type_sub);
  else if (VTYSH_STRCMP ("ip vrf"))
    vtysh_config_get (config, VRF_MODE, line, vtysh_config_type_sub);

  /* MPLS vpls.  */
  else if (VTYSH_STRCMP ("mpls vpls"))
    vtysh_config_get (config, NSM_VPLS_MODE, line, vtysh_config_type_sub);

  /* RIP.  */
  else if (VTYSH_STRCMP ("router rip"))
    vtysh_config_get (config, RIP_MODE, line, vtysh_config_type_add);

  /* RIPng.  */
  else if (VTYSH_STRCMP ("router ipv6 rip"))
    vtysh_config_get (config, RIPNG_MODE, line, vtysh_config_type_add);

  /* OSPF.  */
  else if (VTYSH_STRCMP ("router ospf"))
    vtysh_config_get (config, OSPF_MODE, line, vtysh_config_type_sub);

  /* OSPFv3.  */
  else if (VTYSH_STRCMP ("router ipv6 ospf"))
    vtysh_config_get (config, OSPF6_MODE, line, vtysh_config_type_sub);

  /* IS-IS.  */
  else if (VTYSH_STRCMP ("router isis"))
    vtysh_config_get (config, ISIS_MODE, line, vtysh_config_type_sub);

  /* BGP.  */
  else if (VTYSH_STRCMP ("router bgp"))
    vtysh_config_get (config, BGP_MODE, line, vtysh_config_type_sub);

#ifdef HAVE_LMP
  /* LMP */
  else if (VTYSH_STRCMP ("lmp-configuration"))
    vtysh_config_get (config, LMP_MODE, line, vtysh_config_type_sub);
#endif /* HAVE_LMP */

  /* LDP.  */
  else if (VTYSH_STRCMP ("router ldp"))
    vtysh_config_get (config, LDP_MODE, line, vtysh_config_type_add);
  /* CR-LDP.  */
  else if (VTYSH_STRCMP ("ldp-path"))
    vtysh_config_get (config, LDP_PATH_MODE, line, vtysh_config_type_sub);
  else if (VTYSH_STRCMP ("ldp-trunk"))
    vtysh_config_get (config, LDP_TRUNK_MODE, line, vtysh_config_type_sub);

  /* RSVP.  */
  else if (VTYSH_STRCMP ("router rsvp"))
    vtysh_config_get (config, RSVP_MODE, line, vtysh_config_type_add);
  else if (VTYSH_STRCMP ("rsvp-path"))
    vtysh_config_get (config, RSVP_PATH_MODE, line, vtysh_config_type_sub);
  else if (VTYSH_STRCMP ("rsvp-trunk"))
    vtysh_config_get (config, RSVP_TRUNK_MODE, line, vtysh_config_type_sub);
#ifdef HAVE_MPLS_FRR
  else if (VTYSH_STRCMP ("rsvp-bypass"))
    vtysh_config_get (config, RSVP_BYPASS_MODE, line, vtysh_config_type_sub);
#endif /* HAVE_MPLS_FRR */

#ifdef HAVE_GMPLS
  else if (VTYSH_STRCMP ("te-link"))
    vtysh_config_get (config, TELINK_MODE, line, vtysh_config_type_sub);
  else if (VTYSH_STRCMP ("control-channel"))
    vtysh_config_get (config, CTRL_CHNL_MODE, line, vtysh_config_type_sub);
  else if (VTYSH_STRCMP ("control-adjacency"))
    vtysh_config_get (config, CTRL_ADJ_MODE, line, vtysh_config_type_sub);
#endif

  /* VRRP.  */
  else if (VTYSH_STRCMP ("router vrrp"))
    vtysh_config_get (config, VRRP_MODE, line, vtysh_config_type_add);
#ifdef HAVE_IPV6
  else if (VTYSH_STRCMP ("router ipv6 vrrp"))
    vtysh_config_get (config, VRRP6_MODE, line, vtysh_config_type_add);
#endif /* HAVE_IPV6 */

  /* "mpls" */
  else if (VTYSH_STRCMP ("mpls ftn-entry"))
    vtysh_config_get (config, NSM_MPLS_STATIC_MODE, line, vtysh_config_type_add);
  else if (VTYSH_STRCMP ("mpls ilm-entry"))
    vtysh_config_get (config, NSM_MPLS_STATIC_MODE, line, vtysh_config_type_add);
  else if (VTYSH_STRCMP ("mpls l2-circuit-fib-entry"))
    vtysh_config_get (config, NSM_MPLS_STATIC_MODE, line, vtysh_config_type_add);
  else if (VTYSH_STRCMP ("mpls"))
    vtysh_config_get (config, NSM_MPLS_MODE, line, vtysh_config_type_add);
#ifdef HAVE_MCAST_IPV4
  /* "ip multicast" */
  else if (VTYSH_STRCMP ("ip multicast") || IMI_STRCMP ("ip multicast-routing"))
    vtysh_config_get (config, NSM_MCAST_MODE, line, vtysh_config_type_add);
#endif /* HAVE_MCAST_IPV4 */
#ifdef HAVE_MCAST_IPV6
  /* "ipv6 multicast" */
  else if (VTYSH_STRCMP ("ipv6 multicast") ||
      IMI_STRCMP ("ipv6 multicast-routing"))
    vtysh_config_get (config, NSM_MCAST6_MODE, line, vtysh_config_type_add);
#endif /* HAVE_MCAST_IPV6 */
#if defined HAVE_MCAST_IPV4 || defined HAVE_IGMP_SNOOP
  /* "ip igmp" */
  else if (VTYSH_STRCMP ("ip igmp"))
    vtysh_config_get (config, IGMP_MODE, line, vtysh_config_type_add);
#endif /* HAVE_MCAST_IPV4 || HAVE_IGMP_SNOOP */
  /* "ip route" */
  else if (VTYSH_STRCMP ("ip route"))
    vtysh_config_get (config, IP_MODE, line, vtysh_config_type_add);
#ifdef HAVE_BFD
  else if (VTYSH_STRCMP ("ip static"))
    vtysh_config_get (config, IP_BFD_MODE, line, vtysh_config_type_add);
#endif /* HAVE_BFD */
  /* "ip mroute" */
  else if (VTYSH_STRCMP ("ip mroute"))
    vtysh_config_get (config, IP_MROUTE_MODE, line, vtysh_config_type_add);
  /* "ip community-list" */
  else if (VTYSH_STRCMP ("ip community-list"))
    vtysh_config_get (config, COMMUNITY_LIST_MODE, line, vtysh_config_type_add);
  /* "ip as-path access-list" */
  else if (VTYSH_STRCMP ("ip as-path access-list"))
    vtysh_config_get (config, AS_LIST_MODE, line, vtysh_config_type_add);
  /* "access-list" */
  else if (VTYSH_STRCMP ("access-list"))
    vtysh_config_get (config, ACCESS_MODE, line, vtysh_config_type_add_unique);
  /* "ip prefix-list" */
  else if (VTYSH_STRCMP ("no ip prefix-list"))
    vtysh_config_get (config, EXEC_MODE, line, vtysh_config_type_add_unique);
  else if (VTYSH_STRCMP ("ip prefix-list"))
    vtysh_config_get (config, PREFIX_MODE, line, vtysh_config_type_add_unique);
  /* "ipv6 route" */
  else if (VTYSH_STRCMP ("ipv6 route"))
    vtysh_config_get (config, IPV6_MODE, line, vtysh_config_type_add);
  /* "ipv6 mroute" */
  else if (VTYSH_STRCMP ("ipv6 mroute"))
    vtysh_config_get (config, IPV6_MROUTE_MODE, line, vtysh_config_type_add);
  /* "ipv6 access-list" */
  else if (VTYSH_STRCMP ("ipv6 access-list"))
    vtysh_config_get (config, ACCESS_IPV6_MODE, line, vtysh_config_type_add_unique);
  /* "ipv6 prefix-list" */
  else if (VTYSH_STRCMP ("ipv6 prefix-list"))
    vtysh_config_get (config, PREFIX_IPV6_MODE, line, vtysh_config_type_add_unique);

  /* "route-map" */
  else if (VTYSH_STRCMP ("route-map"))
    vtysh_config_get (config, RMAP_MODE, line, vtysh_config_type_sub);

  else if (VTYSH_STRCMP ("ip pim dense-mode"))
    vtysh_config_get (config, PDM_MODE, line, vtysh_config_type_add);

#ifdef HAVE_MCAST_IPV6
  else if (VTYSH_STRCMP ("ipv6 pim dense-mode"))
    vtysh_config_get (config, PDM6_MODE, line, vtysh_config_type_add);
#endif /* HAVE_MCAST_IPV6 */

  else if (VTYSH_STRCMP ("ip pim"))
    vtysh_config_get (config, PIM_MODE, line, vtysh_config_type_add);

  else if (VTYSH_STRCMP ("ipv6 pim"))
    vtysh_config_get (config, PIM6_MODE, line, vtysh_config_type_add);

  else if (VTYSH_STRCMP ("bfd"))
    vtysh_config_get (config, BFD_MODE, line, vtysh_config_type_add);

  /* "smux".  */
  else if (VTYSH_STRCMP ("smux"))
    vtysh_config_get (config, SMUX_MODE, line, vtysh_config_type_add);

  else if (VTYSH_STRCMP ("stacking"))
    vtysh_config_get (config, STACKING_MODE, line, vtysh_config_type_add);
  else
    vtysh_config_get (config, EXEC_MODE, line, vtysh_config_type_add_unique);
}

/* Write state for mode. */
void
vtysh_config_write_mode (struct cli *cli, struct vtysh_config *config,
                         int mode)
{
  struct vtysh_config_mode *cm;
  struct vtysh_config_mode *sub;
  struct vtysh_config_line *line;

  cm = vtysh_config_lookup (config, mode);
  if (cm)
    {
      if (! FIFO_EMPTY (&cm->main))
        {
          FIFO_LOOP (&cm->main, line)
            {
              cli_out (cli, "%s\n", line->str);
            }
          cli_out (cli, "!\n");
        }

      FIFO_LOOP (&cm->sub, sub)
        {
          if (sub->str)
            cli_out (cli, "%s\n", sub->str);

          FIFO_LOOP (&sub->main, line)
            {
              cli_out (cli, "%s\n", line->str);
            }
          FIFO_LOOP (&sub->match, line)
            {
              cli_out (cli, "%s\n", line->str);
            }
          FIFO_LOOP (&sub->set, line)
            {
              cli_out (cli, "%s\n", line->str);
            }
          cli_out (cli, "!\n");
        }
    }
}

/* Connect to PM for execution of "show" command. */
int
vtysh_show_protocol (module_id_t protocol, char *str, void *out_arg,
                     void (*out_func) (void *, char *), vrf_id_t vrf_id)
{
  pal_sock_handle_t sock;
  char buf[BUFSIZ];
  int nbytes;
  FILE *fp;
#ifdef HAVE_TCP_MESSAGE
  int port;
#else /* HAVE_TCP_MESSAGE */
  char *path;
#endif /* HAVE_TCP_MESSAGE */

#ifdef HAVE_TCP_MESSAGE
  port = show_get_port_from_protocol (protocol);
  if (port < 0)
    return RESULT_ERROR;

  /* Open socket. */
  sock = line_client_sock (vtyshm, port);
#else /* HAVE_TCP_MESSAGE */
  path = show_get_path_from_protocol (protocol);
  if (! path)
    return RESULT_ERROR;

  /* Open socket. */
  sock = line_client_sock (vtyshm, path);
#endif /* HAVE_TCP_MESSAGE */
  if (sock < 0)
    return RESULT_ERROR;

  /* Send command. */
  nbytes = writen (sock, str, strlen (str));
  if (nbytes <= 0)
    return RESULT_ERROR;

  /* Use fgets() to parse line. */
  fp = fdopen (sock, "r");

  /* Write output to out_func. */
  while (fgets (buf, BUFSIZ, fp))
    {
      buf[strlen(buf) - 1] = '\0';
      (*out_func) (out_arg, buf);
    }

  /* Close socket. */
  fclose (fp);
  pal_sock_close (vtyshm, sock);

  return RESULT_OK;
}

/* Query configuration to each protocol.  */
void
vtysh_config_build (vrf_id_t vrf_id, struct vtysh_config *config)
{
  vtysh_show_protocol (APN_PROTO_NSM, "show running-config pm",
                       config, vtysh_config_parse, vrf_id);
  vtysh_show_protocol (APN_PROTO_RIP, "show running-config pm",
                       config, vtysh_config_parse, vrf_id);
  vtysh_show_protocol (APN_PROTO_RIPNG, "show running-config pm",
                       config, vtysh_config_parse, vrf_id);
  vtysh_show_protocol (APN_PROTO_OSPF, "show running-config pm",
                       config, vtysh_config_parse, vrf_id);
  vtysh_show_protocol (APN_PROTO_OSPF6, "show running-config pm",
                       config, vtysh_config_parse, vrf_id);
  vtysh_show_protocol (APN_PROTO_ISIS, "show running-config pm",
                       config, vtysh_config_parse, vrf_id);
  vtysh_show_protocol (APN_PROTO_BGP, "show running-config pm",
                       config, vtysh_config_parse, vrf_id);
  vtysh_show_protocol (APN_PROTO_PIMDM, "show running-config pm",
                       config, vtysh_config_parse, vrf_id);
  vtysh_show_protocol (APN_PROTO_PIMSM, "show running-config pm",
                       config, vtysh_config_parse, vrf_id);
#ifdef HAVE_MCAST_IPV6
  vtysh_show_protocol (APN_PROTO_PIMSM6, "show running-config pm",
                       config, vtysh_config_parse, vrf_id);
#endif /* HAVE_MCAST_IPV6 */
#ifdef HAVE_LMP
  /* LMP */
  vtysh_show_protocol (APN_PROTO_LMP, "show running-config pm",
                       config, vtysh_config_parse, vrf_id);
#endif /* HAVE_LMP */
  vtysh_show_protocol (APN_PROTO_DVMRP, "show running-config pm",
                       config, vtysh_config_parse, vrf_id);
  vtysh_show_protocol (APN_PROTO_LDP, "show running-config pm",
                       config, vtysh_config_parse, vrf_id);
  vtysh_show_protocol (APN_PROTO_RSVP, "show running-config pm",
                       config, vtysh_config_parse, vrf_id);
  vtysh_show_protocol (APN_PROTO_RSTP, "show running-config pm",
                       config, vtysh_config_parse, vrf_id);
  vtysh_show_protocol (APN_PROTO_STP, "show running-config pm",
                       config, vtysh_config_parse, vrf_id);
  vtysh_show_protocol (APN_PROTO_8021X, "show running-config pm",
                       config, vtysh_config_parse, vrf_id);
  vtysh_show_protocol (APN_PROTO_ONM, "show running-config pm",
                       config, vtysh_config_parse, vrf_id);

}

/* Write full PacOS configuration.  */
void
vtysh_config_write_full (struct cli *cli)
{
  struct vtysh_config *vtysh_config;

  vtysh_config = vtysh_config_new ();

  /* Build configuration.  */
  vtysh_config_build (cli->vrf_id, vtysh_config);

  /* Begin with version number. */
  cli_out (cli, "!\n");

  /* Service command. */
  if (host_service_write (cli))
    cli_out (cli, "!\n");

  /* Write host information. */
  host_config_write (cli);

  /* Configure information. */
  vtysh_config_write_mode (cli, vtysh_config, CONFIG_MODE);

  /* Debug information. */
  vtysh_config_write_mode (cli, vtysh_config, DEBUG_MODE);

  /* Stacking information. */
  vtysh_config_write_mode (cli, vtysh_config, STACKING_MODE);

  /* VRF configuration. */
  vtysh_config_write_mode (cli, vtysh_config, VRF_MODE);

  /* Key chian information. */
  vtysh_config_write_mode (cli, vtysh_config, KEYCHAIN_MODE);

  /* NSM MPLS information. */
  vtysh_config_write_mode (cli, vtysh_config, NSM_MPLS_MODE);

  /* NSM VPLS information. */
  vtysh_config_write_mode (cli, vtysh_config, NSM_VPLS_MODE);

#ifdef HAVE_MCAST_IPV4
  /* NSM MCAST information. */
  vtysh_config_write_mode (cli, vtysh_config, NSM_MCAST_MODE);
#endif /* HAVE_MCAST_IPV4 */

#ifdef HAVE_MCAST_IPV6
  /* NSM MCAST6 information. */
  vtysh_config_write_mode (cli, vtysh_config, NSM_MCAST6_MODE);
#endif /* HAVE_MCAST_IPV6 */

#if defined HAVE_MCAST_IPV4 || defined HAVE_IGMP_SNOOP
  /* IGMP-Global information. */
  vtysh_config_write_mode (cli, vtysh_config, IGMP_MODE);

  /* IGMP-Interface information. */
  vtysh_config_write_mode (cli, vtysh_config, IGMP_IF_MODE);
#endif /* HAVE_MCAST_IPV4 || HAVE_IGMP_SNOOP */

  /* Orphan configurations.  */
  vtysh_config_write_mode (cli, vtysh_config, EXEC_MODE);

  /* IP PIM-DM */
  vtysh_config_write_mode (cli, vtysh_config, PDM_MODE);

#ifdef HAVE_MCAST_IPV6
  /* IP PIM-DM6 */
  vtysh_config_write_mode (cli, vtysh_config, PDM6_MODE);
#endif /* HAVE_MCAST_IPV6 */

  /* IP PIM. */
  vtysh_config_write_mode (cli, vtysh_config, PIM_MODE);

  /* IPv6 PIM. */
  vtysh_config_write_mode (cli, vtysh_config, PIM6_MODE);

  /* Interface information. */
  vtysh_config_write_mode (cli, vtysh_config, INTERFACE_MODE);

  /* NSM MPLS STATIC information. */
  vtysh_config_write_mode (cli, vtysh_config, NSM_MPLS_STATIC_MODE);

  /* Routing configuration. */
  vtysh_config_write_mode (cli, vtysh_config, OSPF_MODE);
  vtysh_config_write_mode (cli, vtysh_config, OSPF6_MODE);
  vtysh_config_write_mode (cli, vtysh_config, ISIS_MODE);
  vtysh_config_write_mode (cli, vtysh_config, RIP_MODE);
  vtysh_config_write_mode (cli, vtysh_config, RIPNG_MODE);
  vtysh_config_write_mode (cli, vtysh_config, BGP_MODE);
  vtysh_config_write_mode (cli, vtysh_config, VRRP_MODE);
#ifdef HAVE_IPV6
  vtysh_config_write_mode (cli, vtysh_config, VRRP6_MODE);
#endif /* HAVE_IPV6 */
#ifdef HAVE_LMP
  vtysh_config_write_mode (cli, vtysh_config, LMP_MODE);
#endif /* HAVE_LMP */
  /* MPLS signalling. */
  vtysh_config_write_mode (cli, vtysh_config, LDP_MODE);
  vtysh_config_write_mode (cli, vtysh_config, LDP_PATH_MODE);
  vtysh_config_write_mode (cli, vtysh_config, LDP_TRUNK_MODE);
  vtysh_config_write_mode (cli, vtysh_config, RSVP_MODE);
  vtysh_config_write_mode (cli, vtysh_config, RSVP_PATH_MODE);
  vtysh_config_write_mode (cli, vtysh_config, RSVP_TRUNK_MODE);
#ifdef HAVE_MPLS_FRR
  vtysh_config_write_mode (cli, vtysh_config, RSVP_BYPASS_MODE);
#endif /* HAVE_MPLS_FRR */


  /* IP route. */
  vtysh_config_write_mode (cli, vtysh_config, IP_MODE);

#ifdef HAVE_BFD
  /* IP static bfd */
  vtysh_config_write_mode (cli, vtysh_config, IP_BFD_MODE);
#endif /* HAVE_BFD */

  /* IP mroute. */
  vtysh_config_write_mode (cli, vtysh_config, IP_MROUTE_MODE);

  /* IP community-list.  */
  vtysh_config_write_mode (cli, vtysh_config, COMMUNITY_LIST_MODE);

  /* IP as-path access-list.  */
  vtysh_config_write_mode (cli, vtysh_config, AS_LIST_MODE);

  /* IP prefix-list. */
  vtysh_config_write_mode (cli, vtysh_config, PREFIX_MODE);

  /* IP access-list. */
  vtysh_config_write_mode (cli, vtysh_config, ACCESS_MODE);
  cli_out (cli, "!\n");

#ifdef HAVE_IPV6
  /* IPv6 route. */
  vtysh_config_write_mode (cli, vtysh_config, IPV6_MODE);

  /* IPv6 route. */
  vtysh_config_write_mode (cli, vtysh_config, IPV6_MROUTE_MODE);

  /* IPv6 access-list. */
  vtysh_config_write_mode (cli, vtysh_config, ACCESS_IPV6_MODE);
  cli_out (cli, "!\n");

  /* IPv6 prefix-list. */
  vtysh_config_write_mode (cli, vtysh_config, PREFIX_IPV6_MODE);
#endif /* HAVE_IPV6 */

  /* Routemap. */
  vtysh_config_write_mode (cli, vtysh_config, RMAP_MODE);

  /* BFD mode */
  vtysh_config_write_mode (cli, vtysh_config, BFD_MODE);

#ifdef HAVE_GMPLS
  /* TELINK mode */
  vtysh_config_write_mode (cli, vtysh_config, TELINK_MODE);

  /* Control Channel mode */
  vtysh_config_write_mode (cli, vtysh_config, CTRL_CHNL_MODE);

  /* Control Adjacency mode */
  vtysh_config_write_mode (cli, vtysh_config, CTRL_ADJ_MODE);
#endif /*HAVE_GMPLS */

  /* End should be displayed last. */
  cli_out (cli, "end\n\n");

  /* Free displayed configuration.  */
  vtysh_config_free (vtysh_config);
}

/* Write configuration for each mode.  */
void
vtysh_config_write_pm (struct cli *cli, int mode)
{
  struct vtysh_config *vtysh_config;

  /* Allocate a new configuration.  */
  vtysh_config = vtysh_config_new ();

  /* Begin with version number. */
  cli_out (cli, "!\n");

  /* Build configuration.  */
  vtysh_config_build (cli->vrf_id, vtysh_config);

  /* Write mode's configuration.  */
  vtysh_config_write_mode (cli, vtysh_config, mode);

  /* Free used configuration.  */
  vtysh_config_free (vtysh_config);
}

/* Write configuration for each mode.  */
void
vtysh_config_write_if (struct cli *cli, char *ifname)
{
  struct vtysh_config *vtysh_config;
  struct vtysh_config_mode *cm;
  struct vtysh_config_mode *sub;
  struct vtysh_config_line *line;

  /* Allocate a new configuration.  */
  vtysh_config = vtysh_config_new ();

  /* Begin with version number. */
  cli_out (cli, "!\n");

  /* Build configuration.  */
  vtysh_config_build (cli->vrf_id, vtysh_config);

  /* Write interface configuration.  */
  cm = vtysh_config_lookup (vtysh_config, INTERFACE_MODE);
  if (cm)
    {
      FIFO_LOOP (&cm->sub, sub)
        {
          if (sub->str && pal_strstr (sub->str, ifname))
            {
              cli_out (cli, "%s\n", sub->str);

              FIFO_LOOP (&sub->main, line)
                {
                  cli_out (cli, "%s\n", line->str);
                }
              FIFO_LOOP (&sub->match, line)
                {
                  cli_out (cli, "%s\n", line->str);
                }
              FIFO_LOOP (&sub->set, line)
                {
                  cli_out (cli, "%s\n", line->str);
                }
              cli_out (cli, "!\n");
              break;
            }
        }
    }

  /* Free used configuration.  */
  vtysh_config_free (vtysh_config);
}
#ifdef HAVE_GMPLS
/* Write configuration for each mode.  */
void
vtysh_config_write_te_link (struct cli *cli, char *tlname)
{
  struct vtysh_config *vtysh_config;
  struct vtysh_config_mode *cm;
  struct vtysh_config_mode *sub;
  struct vtysh_config_line *line;

  /* Allocate a new configuration.  */
  vtysh_config = vtysh_config_new ();

  /* Begin with version number. */
  cli_out (cli, "!\n");

  /* Build configuration.  */
  vtysh_config_build (cli->vrf_id, vtysh_config);

  /* Write interface configuration.  */
  cm = vtysh_config_lookup (vtysh_config, TELINK_MODE);
  if (cm)
    {
      FIFO_LOOP (&cm->sub, sub)
        {
          if (sub->str && pal_strstr (sub->str, tlname))
            {
              cli_out (cli, "%s\n", sub->str);

              FIFO_LOOP (&sub->main, line)
                {
                  cli_out (cli, "%s\n", line->str);
                }
              FIFO_LOOP (&sub->match, line)
                {
                  cli_out (cli, "%s\n", line->str);
                }
              FIFO_LOOP (&sub->set, line)
                {
                  cli_out (cli, "%s\n", line->str);
                }
              cli_out (cli, "!\n");
              break;
            }
        }
    }

  /* Free used configuration.  */
  vtysh_config_free (vtysh_config);
}

/* Write configuration for each mode.  */
void
vtysh_config_write_cc (struct cli *cli, char *ccname)
{
  struct vtysh_config *vtysh_config;
  struct vtysh_config_mode *cm;
  struct vtysh_config_mode *sub;
  struct vtysh_config_line *line;

  /* Allocate a new configuration.  */
  vtysh_config = vtysh_config_new ();

  /* Begin with version number. */
  cli_out (cli, "!\n");

  /* Build configuration.  */
  vtysh_config_build (cli->vrf_id, vtysh_config);

  /* Write interface configuration.  */
  cm = vtysh_config_lookup (vtysh_config, CTRL_CHNL_MODE);
  if (cm)
    {
      FIFO_LOOP (&cm->sub, sub)
        {
          if (sub->str && pal_strstr (sub->str, ccname))
            {
              cli_out (cli, "%s\n", sub->str);

              FIFO_LOOP (&sub->main, line)
                {
                  cli_out (cli, "%s\n", line->str);
                }
              FIFO_LOOP (&sub->match, line)
                {
                  cli_out (cli, "%s\n", line->str);
                }
              FIFO_LOOP (&sub->set, line)
                {
                  cli_out (cli, "%s\n", line->str);
                }
              cli_out (cli, "!\n");
              break;
            }
        }
    }

  /* Free used configuration.  */
  vtysh_config_free (vtysh_config);
}

/* Write configuration for each mode.  */
void
vtysh_config_write_ca (struct cli *cli, char *caname)
{
  struct vtysh_config *vtysh_config;
  struct vtysh_config_mode *cm;
  struct vtysh_config_mode *sub;
  struct vtysh_config_line *line;

  /* Allocate a new configuration.  */
  vtysh_config = vtysh_config_new ();

  /* Begin with version number. */
  cli_out (cli, "!\n");

  /* Build configuration.  */
  vtysh_config_build (cli->vrf_id, vtysh_config);

  /* Write interface configuration.  */
  cm = vtysh_config_lookup (vtysh_config, CTRL_ADJ_MODE);
  if (cm)
    {
      FIFO_LOOP (&cm->sub, sub)
        {
          if (sub->str && pal_strstr (sub->str, caname))
            {
              cli_out (cli, "%s\n", sub->str);

              FIFO_LOOP (&sub->main, line)
                {
                  cli_out (cli, "%s\n", line->str);
                }
              FIFO_LOOP (&sub->match, line)
                {
                  cli_out (cli, "%s\n", line->str);
                }
              FIFO_LOOP (&sub->set, line)
                {
                  cli_out (cli, "%s\n", line->str);
                }
              cli_out (cli, "!\n");
              break;
            }
        }
    }

  /* Free used configuration.  */
  vtysh_config_free (vtysh_config);
}
#endif /*HAVE_GMPLS*/

/* Execute command for start up configuration.  */
int
vtysh_startup_execute (struct cli_node *node, struct cli *cli, char *str,
                       int local_only)
{
  int i;
  int ret;
  struct vtysh_pm *pm;
  struct line reply;

  /* Send it to protocol module.  */
  if (! local_only)
    FOREACH_APN_PROTO (i)
      if (MODBMAP_ISSET (node->cel->module, i)
          && (pm = vtysh_pm_lookup (vtysh_pmaster, i)) != NULL)
        {
          if (pm->connected != PAL_TRUE)
            continue;

          ret = vtysh_pm_line_send (pm, str, cli->mode, 0);
          if (ret < 0)
            continue;

          line_client_read (pm->sock, &reply);
          if (reply.code == LINE_CODE_ERROR)
            return -1;
        }

  /* Execute local function.  */
  if (node->cel->func)
    ret = (*node->cel->func) (cli, ctree->argc, ctree->argv);

  cli_free_arguments (ctree);

  return 0;
}

/* Load the start up configuration.  */
int
vtysh_startup_config (char *file_name, int local_only)
{
  int ret;
  FILE *fp;
  char buf[VTYSH_BUFSIZ];
  struct cli *cli;
  struct line line;
  int length;

  /* Open the file. */
  fp = fopen (file_name, "r");
  if (! fp)
    return -1;

  /* Line server setting. */
  pal_mem_set (&line, 0, sizeof (struct line));
  /* line.server = vtysh_line_server; */

  /* Set up CLI structure.  */
  cli = &line.cli;
  cli->zg = vtyshm;
  cli->line = &line;
  cli->vr = apn_vr_get_privileged (vtyshm);
  cli->mode = CONFIG_MODE;
  cli->source = CLI_SOURCE_FILE;

  /* CLI output function and argument.  */
  cli->out_func = (CLI_OUT_FUNC) fprintf;
  cli->out_val = stdout;

  /* Parse line one by one.  Mode is set to CONFIG_MODE.  */
  while (fgets (buf, VTYSH_BUFSIZ, fp))
    {
      /* Clear training new line.  */
      length = strlen (buf);
      if (--length >= 0)
        buf[length] = '\0';

      ret = cli_parse (ctree, cli->mode, PRIVILEGE_MAX, buf, 1, 0);
      switch (ret)
        {
        case CLI_PARSE_EMPTY_LINE:
          /* Simply ignore empty line.  */
          break;
        case CLI_PARSE_SUCCESS:
          vtysh_startup_execute (ctree->exec_node, cli, buf, local_only);
          break;
        case CLI_PARSE_INCOMPLETE:
        case CLI_PARSE_INCOMPLETE_PIPE:
        case CLI_PARSE_AMBIGUOUS:
        case CLI_PARSE_NO_MATCH:
        case CLI_PARSE_NO_MODE:
          /* Second try.  */
          /* printf ("error!\n");*/

          cli->mode = CONFIG_MODE;

          ret = cli_parse (ctree, cli->mode, PRIVILEGE_MAX, buf, 1, 0);
          switch (ret)
            {
            case CLI_PARSE_SUCCESS:
              vtysh_startup_execute (ctree->exec_node, cli, buf, local_only);
              break;
            default:
              break;
            }

          break;
        default:
          break;
        }
    }

  return 0;
}

/* Write configuration to the file.  */
int
vtysh_write_config ()
{
  struct cli cli;
  FILE *fp;

  fp = fopen (config_file, "w");
  if (! fp)
    return -1;

  /* Set output function and variable.  */
  cli.zg = vtyshm;
  cli.out_func = (int (*) (void *, char *, ...))fprintf;
  cli.out_val = fp;
  cli.host = vtyshm->host;

  /* Time stamp.  */
#ifdef HAVE_LICENSE_MGR
  fprintf (fp,"!\n!\n! Config for PacOS%s\n!\n",PACOS_COPYRIGHT_SRS);
#else
  fprintf (fp,"!\n!\n! Config for PacOS%s\n!\n",PACOS_COPYRIGHT_1);
#endif /* HAVE_LICENSE_MGR */

  /* Call show running-config.  */
  vtysh_config_write_full (&cli);

  fclose (fp);

  printf ("[Saved to %s]\n", config_file);

  return 0;
}

/* Show running configuration.  */
CLI (vtysh_show_running_config,
     vtysh_show_running_config_cli,
     "show running-config",
     "Show running system information",
     "Current Operating configuration")
{
  printf ("\nCurrent configuration:\n");
  vtysh_config_write_full (cli);
  return CLI_SUCCESS;
}

ALI (vtysh_show_running_config,
     vtysh_show_running_config_full_cli,
     "show running-config full",
     "Show running system information",
     "Current Operating configuration",
     "full configuration");

ALI (vtysh_show_running_config,
     vtysh_write_terminal_cli,
     "write terminal",
     CLI_WRITE_STR,
     "Write to terminal");

CLI (vtysh_show_running_config_if,
     vtysh_show_running_config_if_cli,
     "show running-config interface IFNAME",
     "Show running system information",
     "Current Operating configuration",
     "Show interface configuration",
     "Interface name")
{
  vtysh_config_write_if (cli, argv[0]);
  return CLI_SUCCESS;
}

#ifdef HAVE_GMPLS
CLI (vtysh_show_running_config_te_link,
     vtysh_show_running_config_te_link_cli,
     "show running-config te-link TLNAME",
     "Show running system information",
     "Current Operating configuration",
     "Show TE Link configuration",
     "TE Link name")
{
  vtysh_config_write_te_link (cli, argv[0]);
  return CLI_SUCCESS;
}

CLI (vtysh_show_running_config_cc,
     vtysh_show_running_config_cc_cli,
     "show running-config control-channel CCNAME",
     "Show running system information",
     "Current Operating configuration",
     "Show Control Channel configuration",
     "Control Channel name")
{
  vtysh_config_write_cc (cli, argv[0]);
  return CLI_SUCCESS;
}

CLI (vtysh_show_running_config_ca,
     vtysh_show_running_config_ca_cli,
     "show running-config control-adjacency CANAME",
     "Show running system information",
     "Current Operating configuration",
     "Show Control Adjacency configuration",
     "Control Adjacency name")
{
  vtysh_config_write_ca (cli, argv[0]);
  return CLI_SUCCESS;
}
#endif /*HAVE_GMPLS */

/* Protocol module configuration.  */
#ifdef HAVE_RIPD
CLI (vtysh_show_running_config_rip,
     vtysh_show_running_config_rip_cli,
     "show running-config router rip",
     "Show running system information",
     "Current Operating configuration",
     "Router configuration",
     "Routing Information Protocol (RIP)")
{
  vtysh_config_write_pm (cli, RIP_MODE);
  return CLI_SUCCESS;
}
#endif /* HAVE_RIPD */

#ifdef HAVE_OSPFD
CLI (vtysh_show_running_config_ospf,
     vtysh_show_running_config_ospf_cli,
     "show running-config router ospf",
     "Show running system information",
     "Current Operating configuration",
     "Router configuration",
     "Open Shortest Path First (OSPF)")
{
  vtysh_config_write_pm (cli, OSPF_MODE);
  return CLI_SUCCESS;
}
#endif /* HAVE_OSPFD */

#ifdef HAVE_ISISD
CLI (vtysh_show_running_config_isis,
     vtysh_show_running_config_isis_cli,
     "show running-config router isis",
     "Show running system information",
     "Current Operating configuration",
     "Router configuration",
     "ISO IS-IS")
{
  vtysh_config_write_pm (cli, ISIS_MODE);
  return CLI_SUCCESS;
}
#endif /* HAVE_ISISD */

#ifdef HAVE_BGPD
CLI (vtysh_show_running_config_bgp,
     vtysh_show_running_config_bgp_cli,
     "show running-config router bgp",
     "Show running system information",
     "Current Operating configuration",
     "Router configuration",
     "Border Gateway Protocol (BGP)")
{
  vtysh_config_write_pm (cli, BGP_MODE);
  return CLI_SUCCESS;
}
#endif /* HAVE_BGPD */

#ifdef HAVE_IPV6
#ifdef HAVE_RIPNGD
CLI (vtysh_show_running_config_ripng,
     vtysh_show_running_config_ripng_cli,
     "show running-config router ipv6 rip",
     "Show running system information",
     "Current Operating configuration",
     "Router configuration",
     CLI_IPV6_STR,
     "Routing Information Protocol (RIP)")
{
  vtysh_config_write_pm (cli, RIPNG_MODE);
  return CLI_SUCCESS;
}
#endif /* HAVE_RIPNGD */

#ifdef HAVE_LMP
CLI (vtysh_show_running_config_lmp,
     vtysh_show_running_config_lmp_cli,
     "show running-config lmp-configuration",
     "Show running system information",
     "Current Operating configuration",
     CLI_LMP_STR,
     "Configuration")
{
  vtysh_config_write_pm (cli, LMP_MODE);
  return CLI_SUCCESS;
}
#endif /* HAVE_LMP */

#ifdef HAVE_OSPF6D
CLI (vtysh_show_running_config_ospf6,
     vtysh_show_running_config_ospf6_cli,
     "show running-config router ipv6 ospf",
     "Show running system information",
     "Current Operating configuration",
     "Router configuration",
     CLI_IPV6_STR,
     "Open Shortest Path First (OSPF)")
{
  vtysh_config_write_pm (cli, OSPF6_MODE);
  return CLI_SUCCESS;
}
#endif /* HAVE_OSPF6D */
#endif /* HAVE_IPV6 */

#ifdef HAVE_LDPD
CLI (vtysh_show_running_config_ldp,
     vtysh_show_running_config_ldp_cli,
     "show running-config router ldp",
     "Show running system information",
     "Current Operating configuration",
     "Router configuration",
     "Lable Distribution Protocol (LDP)")
{
  vtysh_config_write_pm (cli, LDP_MODE);
  return CLI_SUCCESS;
}
#endif /* HAVE_LDPD */

#ifdef HAVE_RSVPD
CLI (vtysh_show_running_config_rsvp,
     vtysh_show_running_config_rsvp_cli,
     "show running-config router rsvp",
     "Show running system information",
     "Current Operating configuration",
     "Router configuration",
     "Resource Reservation Protocol (RSVP)")
{
  vtysh_config_write_pm (cli, RSVP_MODE);
  return CLI_SUCCESS;
}
#endif /* HAVE_RSVPD */

#ifdef HAVE_VRRP
CLI (vtysh_show_running_config_vrrp,
     vtysh_show_running_config_vrrp_cli,
     "show running-config router vrrp",
     "Show running system information",
     "Current Operating configuration",
     "Router configuration",
     "Virtual Redundancy Routing Protocol (VRRP)")
{
  vtysh_config_write_pm (cli, VRRP_MODE);
  return CLI_SUCCESS;
}
#ifdef HAVE_IPV6
CLI (vtysh_show_running_config_vrrp_ipv6,
     vtysh_show_running_config_vrrp_ipv6_cli,
     "show running-config router ipv6 vrrp",
     "Show running system information",
     "Current Operating configuration",
     "IPv6 Router configuration",
     CLI_IPV6_STR,
     "Virtual Redundancy Routing Protocol (VRRP)")
{
  vtysh_config_write_pm (cli, VRRP6_MODE);
  return CLI_SUCCESS;
}
#endif /* HAVE_IPV6 */
#endif /* HAVE_VRRP */

/* Show startup configuration file. */
CLI (vtysh_show_startup_config,
     vtysh_show_startup_config_cli,
     "show startup-config",
     CLI_SHOW_STR,
     "Contents of startup configuration")
{
  FILE *fp;
  char buf[VTYSH_BUFSIZ];

  fp = fopen (config_file, "r");
  if (! fp)
    {
      cli_out (cli, "%% Can't open startup-config\n");
      return CLI_ERROR;
    }

  while (fgets (buf, VTYSH_BUFSIZ, fp))
    cli_out (cli, "%s", buf);

  fclose (fp);

  return CLI_SUCCESS;
}

CLI (vtysh_write,
     vtysh_write_cli,
     "write",
     CLI_WRITE_STR)
{
  int ret;

  ret = vtysh_write_config ();
  if (ret < 0)
    {
      cli_out (cli, "%% Can't write configuration\n");
      return CLI_ERROR;
    }
  return CLI_SUCCESS;
}

ALI (vtysh_write,
     vtysh_write_file_cli,
     "write file",
     CLI_WRITE_STR,
     "Write to file");

ALI (vtysh_write,
     vtysh_write_memory_cli,
     "write memory",
     CLI_WRITE_STR,
     "Write to NV memory");

ALI (vtysh_write,
     vtysh_copy_runconfig_startconfig_cli,
     "copy running-config startup-config",
     "Copy from one file to another",
     "Copy from current system configuration",
     "Copy to startup configuration");

void
vtysh_config_init ()
{
  int i;

  for (i = 0; i < MAX_MODE; i++)
    if (i != EXEC_MODE)
    {
      cli_install (ctree, i, &vtysh_show_running_config_cli);
      cli_install (ctree, i, &vtysh_show_running_config_full_cli);
      if (i == EXEC_PRIV_MODE)
        cli_install (ctree, i, &vtysh_write_terminal_cli);
      else
        cli_install_hidden (ctree, i, &vtysh_write_terminal_cli);
      SET_FLAG (vtysh_write_terminal_cli.flags, CLI_FLAG_SHOW);
    }

  cli_install (ctree, EXEC_PRIV_MODE, &vtysh_show_running_config_if_cli);
#ifdef HAVE_GMPLS
  cli_install (ctree, EXEC_PRIV_MODE, &vtysh_show_running_config_te_link_cli);
  cli_install (ctree, EXEC_PRIV_MODE, &vtysh_show_running_config_cc_cli);
  cli_install (ctree, EXEC_PRIV_MODE, &vtysh_show_running_config_ca_cli);
#endif /*HAVE_GMPLS*/

#ifdef HAVE_RIPD
  cli_install (ctree, EXEC_PRIV_MODE, &vtysh_show_running_config_rip_cli);
#endif /* HAVE_RIPD */
#ifdef HAVE_OSPFD
  cli_install (ctree, EXEC_PRIV_MODE, &vtysh_show_running_config_ospf_cli);
#endif /* HAVE_OSPFD */
#ifdef HAVE_ISISD
  cli_install (ctree, EXEC_PRIV_MODE, &vtysh_show_running_config_isis_cli);
#endif /* HAVE_ISISD */
#ifdef HAVE_BGPD
  cli_install (ctree, EXEC_PRIV_MODE, &vtysh_show_running_config_bgp_cli);
#endif /* HAVE_BGPD */
#ifdef HAVE_LMP
  cli_install (ctree, EXEC_PRIV_MODE, &vtysh_show_running_config_lmp_cli);
#endif /* HAVE_LMP */
#ifdef HAVE_IPV6
#ifdef HAVE_RIPNGD
  cli_install (ctree, EXEC_PRIV_MODE, &vtysh_show_running_config_ripng_cli);
#endif /* HAVE_RIPNGD */
#ifdef HAVE_OSPF6D
  cli_install (ctree, EXEC_PRIV_MODE, &vtysh_show_running_config_ospf6_cli);
#endif /* HAVE_OSPF6D */
#endif /* HAVE_IPV6 */
#ifdef HAVE_LDPD
  cli_install (ctree, EXEC_PRIV_MODE, &vtysh_show_running_config_ldp_cli);
#endif /* HAVE_LDPD */
#ifdef HAVE_RSVPD
  cli_install (ctree, EXEC_PRIV_MODE, &vtysh_show_running_config_rsvp_cli);
#endif /* HAVE_RSVPD */
#ifdef HAVE_VRRP
  cli_install (ctree, EXEC_PRIV_MODE, &vtysh_show_running_config_vrrp_cli);
#ifdef HAVE_IPV6
  cli_install (ctree, EXEC_PRIV_MODE, &vtysh_show_running_config_vrrp_ipv6_cli);
#endif /* HAVE_IPV6 */
#endif /* HAVE_VRRP */

  cli_install (ctree, EXEC_PRIV_MODE, &vtysh_show_startup_config_cli);

  cli_install (ctree, EXEC_PRIV_MODE, &vtysh_write_cli);
  cli_install (ctree, EXEC_PRIV_MODE, &vtysh_write_file_cli);
  cli_install (ctree, EXEC_PRIV_MODE, &vtysh_write_memory_cli);
  cli_install (ctree, EXEC_PRIV_MODE, &vtysh_copy_runconfig_startconfig_cli);

  {
    int i;

    for (i = 0; i < MAX_MODE; i++)
      if (i != EXEC_MODE && i != EXEC_PRIV_MODE)
        cli_install_hidden (ctree, i, &vtysh_write_memory_cli);
  }
}
