/* Copyright (C) 2001-2003  All Rights Reserved. */

#include <pal.h>

#include "cli.h"
#include "line.h"
#include "host.h"
#include "fifo.h"
#include "message.h"
#include "snprintf.h"
#include "routemap.h"
#include "keychain.h"

#include "imi/imi.h"
#include "imi/imi_parser.h"
#include "imi/imi_config.h"
#include "imi/imi_server.h"
#include "cfg_seq.h"

#ifdef HAVE_ACL
#include "imi/util/imi_filter.h"
#endif /* HAVE_ACL */
#ifdef HAVE_NAT
#include "imi/util/imi_vs.h"
#endif /* HAVE_NAT */
#ifdef HAVE_APS_TOOLKIT
#include "imi/imi_api.h"
#endif /* HAVE_APS_TOOLKIT */

#ifdef HAVE_CRX
#include "crx.h"
#endif /* HAVE_CRX */

#ifdef HAVE_CONFIG_COMPARE
#include "imi/imi_config_cmp.h"
#endif /* HAVE_CONFIG_COMPARE */

#define IMI_STRCMP(S)   (pal_strncmp (line, S, pal_strlen (S)) == 0)

#define MTYPE_IMI_CONFIG_MODE     MTYPE_TMP
#define MTYPE_IMI_CONFIG_LINE     MTYPE_TMP

/* Allocate a new configuration line.  */
struct imi_config_line *
imi_config_line_new ()
{
  return XCALLOC (MTYPE_IMI_CONFIG_LINE, sizeof (struct imi_config_line));
}

/* Free configuration line.  */
void
imi_config_line_free (struct imi_config_line *line)
{
  if (line->str)
    XFREE (MTYPE_TMP, line->str);

  XFREE (MTYPE_IMI_CONFIG_LINE, line);
}

/* Allocate a new configuration mode.  */
struct imi_config_mode *
imi_config_mode_new ()
{
  struct imi_config_mode *cm;

  cm = XCALLOC (MTYPE_IMI_CONFIG_MODE, sizeof (struct imi_config_mode));

  FIFO_INIT (&cm->main);
  FIFO_INIT (&cm->match);
  FIFO_INIT (&cm->set);
  FIFO_INIT (&cm->sub);

  return cm;
}

/* Free configuration mode.  */
void
imi_config_mode_free (struct imi_config_mode *cm)
{
  struct imi_config_line *line;

  if (cm->str)
    XFREE (MTYPE_TMP, cm->str);

  while ((line = (struct imi_config_line *)FIFO_HEAD (&cm->main)) != NULL)
    {
      FIFO_DEL (line);
      imi_config_line_free (line);
    }
  while ((line = (struct imi_config_line *)FIFO_HEAD (&cm->match)) != NULL)
    {
      FIFO_DEL (line);
      imi_config_line_free (line);
    }
  while ((line = (struct imi_config_line *)FIFO_HEAD (&cm->set)) != NULL)
    {
      FIFO_DEL (line);
      imi_config_line_free (line);
    }
  XFREE (MTYPE_IMI_CONFIG_MODE, cm);
}

/* Allocate a new configuration. */
struct imi_config *
imi_config_new ()
{
  struct imi_config *conf;

  conf = XCALLOC (MTYPE_IMI_CONFIG, sizeof (struct imi_config));
  conf->modes = vector_init (VECTOR_MIN_SIZE);

  return conf;
}

/* Free configuration. */
void
imi_config_free (struct imi_config *conf)
{
  int i;
  struct imi_config_mode *cm;
  struct imi_config_mode *sub;

  for (i = 0; i < vector_max (conf->modes); i++)
    if ((cm = vector_slot (conf->modes, i)) != NULL)
      {
        while ((sub = (struct imi_config_mode *)FIFO_HEAD (&cm->sub)) != NULL)
          {
            FIFO_DEL (sub);
            imi_config_mode_free (sub);
          }
        imi_config_mode_free (cm);
      }
  vector_free (conf->modes);
  XFREE (MTYPE_IMI_CONFIG, conf);
}

/* Add one line configuration.  */
void
imi_config_add (struct imi_config_fifo *fifo, char *str)
{
  struct imi_config_line *line;

  line = imi_config_line_new ();
  line->str = XSTRDUP (MTYPE_TMP, str);
  FIFO_ADD (fifo, line);
}

/* Flag configuration addition such as "ip access-list".  */
void
imi_config_add_unique (struct imi_config_fifo *fifo, char *str)
{
  struct imi_config_line *line;

  FIFO_LOOP (fifo, line)
    {
      if (pal_strcmp (line->str, str) == 0)
        return;
    }
  imi_config_add (fifo, str);
}

/* Lookup mode configuration.  */
struct imi_config_mode *
imi_config_lookup (struct imi_config *config, int mode)
{
  if (mode >= MAX_MODE)
    return NULL;

  return vector_lookup_index (config->modes, mode);
}

/* Get sub configuration.  */
struct imi_config_mode *
imi_config_sub_get (struct imi_config_mode *cm, char *str)
{
  struct imi_config_mode *find;

  FIFO_LOOP (&cm->sub, find)
    {
      if (pal_strcmp (find->str, str) == 0)
        return find;
    }

  find = imi_config_mode_new ();
  find->str = XSTRDUP (MTYPE_TMP, str);
  FIFO_ADD (&cm->sub, find);

  return find;
}

#ifdef HAVE_CUSTOM1
/* Get sub configuration.  */
struct imi_config_mode *
imi_config_if_lookup (struct imi_config *config, int mode, char *str)
{
  struct imi_config_mode *cm;
  struct imi_config_mode *find;

  cm = imi_config_lookup (config, mode);
  if (! cm)
    {
      cm = imi_config_mode_new ();
      vector_set_index (config->modes, mode, cm);
    }

  FIFO_LOOP (&cm->sub, find)
    {
      if (pal_strcmp (find->str, str) == 0)
        {
          config->mode = mode;
          config->cm = find;
          return find;
        }
    }
  config->cm = NULL;

  return NULL;
}
#endif /* HAVE_CUSTOM1 */

/* Get mode specific configuration.  */
struct imi_config_mode *
imi_config_get (struct imi_config *config, int mode, char *str,
                enum imi_config_type type)
{
  struct imi_config_mode *cm;

  cm = imi_config_lookup (config, mode);
  if (! cm)
    {
      cm = imi_config_mode_new ();
      vector_set_index (config->modes, mode, cm);
    }

  switch (type)
    {
    case imi_config_type_add:
      imi_config_add (&cm->main, str);
      break;
    case imi_config_type_add_unique:
      imi_config_add_unique (&cm->main, str);
      break;
    case imi_config_type_sub:
      cm = imi_config_sub_get (cm, str);
      break;
    }

  config->mode = mode;
  config->cm = cm;

  return cm;
}

/* Add configuration to */
void
imi_config_line_add (struct imi_config *config, struct imi_config_mode *cm,
                     char *line)
{
  struct imi_config_fifo *fifo;

  if (config->mode == RMAP_MODE)
    {
      if (IMI_STRCMP (" match"))
        fifo = &cm->match;
      else if (IMI_STRCMP (" set"))
        fifo = &cm->set;
      else
        fifo = &cm->main;
      imi_config_add_unique (fifo, line);
    }
  else if (config->mode == INTERFACE_MODE)
    {
      /* For the sub mode inside INTERFACE_MODE,
       * this comaprision is added  as "sh run"
       * should allow same CLI configuration of one
       * evc mode in the other evc mode (EVC_SERVICE_INSTANCE_MODE).
       * Similarly one esp mode in the other esp mode (PBB_TE_ESP_IF_CONFIG_MODE).
       * So imi_config_add () is called instead of imi_config_add_unique ().
       */
      if (IMI_STRCMP ("  ingress-policing") ||
          IMI_STRCMP ("  egress-shaping") ||
          IMI_STRCMP ("  esp egress pnp") ||
          IMI_STRCMP ("  pbb-te") ||
          IMI_STRCMP ("  remote-mac"))
        {
          imi_config_add (&cm->main, line);
        }
      else
        imi_config_add_unique (&cm->main, line);
    }
  else
    imi_config_add (&cm->main, line);
}

/* Parse line and insert into imim->config_top vector. */
void
imi_config_parse (void *arg, char *line, module_id_t module)
{
  char c;
  struct imi_config_mode *cm;
  struct imi_config *config = arg;

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
        imi_config_line_add (config, cm, line);
      else
        ;
      return;
    }

  /* Interface which exists in NSM shold be shown.  */
 if (IMI_STRCMP ("interface"))
    {
#ifdef HAVE_CUSTOM1
      if (module == APN_PROTO_NSM)
#endif /* HAVE_CUSTOM1 */
        imi_config_get (config, INTERFACE_MODE, line, imi_config_type_sub);
#ifdef HAVE_CUSTOM1
      else
       imi_config_if_lookup (config, INTERFACE_MODE, line);
#endif /* HAVE_CUSTOM1 */
    }
  if (IMI_STRCMP ("pw"))
  		imi_config_get (config,PW_MODE, line, imi_config_type_sub);
  else if (IMI_STRCMP ("ac"))
  		imi_config_get (config,AC_MODE, line, imi_config_type_sub);
  else if ((IMI_STRCMP ("fe")) || (IMI_STRCMP ("ge")))
  		imi_config_get (config,RPORT_MODE, line, imi_config_type_sub);  
  else if (IMI_STRCMP ("vpn"))
  		imi_config_get (config,VPN_MODE, line, imi_config_type_sub);
  else if (IMI_STRCMP ("lsp"))
  		imi_config_get (config,LSP_MODE, line, imi_config_type_sub);
  else if (IMI_STRCMP ("lsp-group"))
  		imi_config_get (config,LSP_GRP_MODE, line, imi_config_type_sub);
  else if (IMI_STRCMP ("lsp-oam"))
  		imi_config_get (config,LSP_OAM_MODE, line, imi_config_type_sub);
  else if (IMI_STRCMP ("tun"))
  		imi_config_get (config,TUN_MODE, line, imi_config_type_sub);
  else if (IMI_STRCMP ("meg"))
  		imi_config_get (config,MEG_MODE, line, imi_config_type_sub);  //test 031  add by czh
  else if (IMI_STRCMP ("oam1731"))
  		imi_config_get (config,OAM_1731_MODE, line, imi_config_type_sub);  //test 031  add by czh
  else if (IMI_STRCMP ("ptp"))
  		imi_config_get (config,PTP_MODE, line, imi_config_type_sub);  
  else if (IMI_STRCMP ("port"))
  		imi_config_get (config,PTP_PORT_MODE, line, imi_config_type_sub);
  /* "debug" */
  else if (IMI_STRCMP ("debug"))
    imi_config_get (config, DEBUG_MODE, line, imi_config_type_add);

  else if (IMI_STRCMP ("key"))
    imi_config_get (config, KEYCHAIN_MODE, line, imi_config_type_sub);

  else if (IMI_STRCMP ("ip vrf"))
    imi_config_get (config, VRF_MODE, line, imi_config_type_sub);

#ifdef HAVE_VR
  else if (IMI_STRCMP ("virtual-router"))
    imi_config_get (config, VR_MODE, line, imi_config_type_sub);
#endif /* HAVE_VR */

#ifdef HAVE_VLOGD
  else if (IMI_STRCMP ("log file"))
    imi_config_get (config, VLOG_MODE, line, imi_config_type_add_unique);
#endif /* HAVE_VLOGD. */

  /* MPLS vpls.  */
  else if (IMI_STRCMP ("mpls vpls"))
    imi_config_get (config, NSM_VPLS_MODE, line, imi_config_type_sub);

#ifdef HAVE_MCAST_IPV4
  /* NSM MCAST.  */
  else if (IMI_STRCMP ("ip multicast") ||
      IMI_STRCMP ("ip multicast-routing") ||
      IMI_STRCMP ("no ip multicast-routing"))
    imi_config_get (config, NSM_MCAST_MODE, line, imi_config_type_add);
#endif /* HAVE_MCAST_IPV4 */

#ifdef HAVE_MCAST_IPV6
  /* NSM MCAST.  */
  else if (IMI_STRCMP ("ipv6 multicast") ||
      IMI_STRCMP ("ipv6 multicast-routing") ||
      IMI_STRCMP ("no ipv6 multicast-routing"))
    imi_config_get (config, NSM_MCAST6_MODE, line, imi_config_type_add);
#endif /* HAVE_MCAST_IPV6 */

  /* RIP.  */
  else if (IMI_STRCMP ("router rip"))
    imi_config_get (config, RIP_MODE, line, imi_config_type_add);

  /* RIPng.  */
  else if (IMI_STRCMP ("router ipv6 rip"))
    imi_config_get (config, RIPNG_MODE, line, imi_config_type_add);

  /* OSPF.  */
  else if (IMI_STRCMP ("router ospf"))
    imi_config_get (config, OSPF_MODE, line, imi_config_type_sub);

  /* OSPFv3.  */
  else if (IMI_STRCMP ("router ipv6 ospf"))
    imi_config_get (config, OSPF6_MODE, line, imi_config_type_sub);

  /* IS-IS.  */
  else if (IMI_STRCMP ("router isis"))
    imi_config_get (config, ISIS_MODE, line, imi_config_type_sub);

  /* BGP.  */
  else if (IMI_STRCMP ("router bgp"))
    imi_config_get (config, BGP_MODE, line, imi_config_type_sub);

  /* LDP.  */
  else if (IMI_STRCMP ("router ldp"))
    imi_config_get (config, LDP_MODE, line, imi_config_type_add);

  /* CR-LDP.  */
  else if (IMI_STRCMP ("ldp-path"))
    imi_config_get (config, LDP_PATH_MODE, line, imi_config_type_sub);

  else if (IMI_STRCMP ("ldp-trunk"))
    imi_config_get (config, LDP_TRUNK_MODE, line, imi_config_type_sub);

  else if (IMI_STRCMP ("targeted-peer ipv4"))
    imi_config_get (config, TARGETED_PEER_MODE, line, imi_config_type_add);

  /* RSVP.  */
  else if (IMI_STRCMP ("router rsvp"))
    imi_config_get (config, RSVP_MODE, line, imi_config_type_add);

  else if (IMI_STRCMP ("rsvp-path"))
    imi_config_get (config, RSVP_PATH_MODE, line, imi_config_type_sub);

  else if (IMI_STRCMP ("rsvp-trunk"))
    imi_config_get (config, RSVP_TRUNK_MODE, line, imi_config_type_sub);

#ifdef HAVE_MPLS_FRR
  else if (IMI_STRCMP ("rsvp-bypass"))
    imi_config_get (config, RSVP_BYPASS_MODE, line, imi_config_type_sub);
#endif /* HAVE_MPLS_FRR */

#ifdef HAVE_GMPLS
  else if (IMI_STRCMP ("gmpls ftn-entry"))
    imi_config_get (config, NSM_GMPLS_STATIC_MODE, line, imi_config_type_add);
  else if (IMI_STRCMP ("gmpls ilm-entry"))
    imi_config_get (config, NSM_GMPLS_STATIC_MODE, line, imi_config_type_add);
  else if (IMI_STRCMP ("te-link"))
    imi_config_get (config, TELINK_MODE, line, imi_config_type_sub);
  else if (IMI_STRCMP ("control-channel"))
    imi_config_get (config, CTRL_CHNL_MODE, line, imi_config_type_sub);
  else if (IMI_STRCMP ("control-adjacency"))
    imi_config_get (config, CTRL_ADJ_MODE, line, imi_config_type_sub);
#endif
#ifdef HAVE_LMP
   else if (IMI_STRCMP ("lmp-configuration"))
     imi_config_get (config, LMP_MODE, line, imi_config_type_sub);
#endif /* HAVE_LMP */
  /* VRRP.  */
  else if (IMI_STRCMP ("router vrrp"))
    imi_config_get (config, VRRP_MODE, line, imi_config_type_add);
#ifdef HAVE_IPV6
  else if (IMI_STRCMP ("router ipv6 vrrp"))
    imi_config_get (config, VRRP6_MODE, line, imi_config_type_add);
#endif /* HAVE_IPV6 */

  /* "mpls" */
  else if (IMI_STRCMP ("mpls ftn-entry"))
    imi_config_get (config, NSM_MPLS_STATIC_MODE, line, imi_config_type_add);
  else if (IMI_STRCMP ("mpls ilm-entry"))
    imi_config_get (config, NSM_MPLS_STATIC_MODE, line, imi_config_type_add);
  else if (IMI_STRCMP ("mpls l2-circuit-fib-entry"))
    imi_config_get (config, NSM_MPLS_STATIC_MODE, line, imi_config_type_add);
#ifdef HAVE_VPLS
  /* "vpls fib entry" */
  else if (IMI_STRCMP ("vpls fib-entry"))
    imi_config_get (config, NSM_MPLS_STATIC_MODE, line, imi_config_type_add);
#endif /* HAVE_VPLS */
  else if (IMI_STRCMP ("mpls"))
    imi_config_get (config, NSM_MPLS_MODE, line, imi_config_type_add);

  /* "ip route" */
  else if (IMI_STRCMP ("ip route"))
    imi_config_get (config, IP_MODE, line, imi_config_type_add);
  /* "ip mroute" */
  else if (IMI_STRCMP ("ip mroute"))
    imi_config_get (config, IP_MROUTE_MODE, line, imi_config_type_add);
#ifdef HAVE_BFD
  else if (IMI_STRCMP ("ip static"))
    imi_config_get (config, IP_BFD_MODE, line, imi_config_type_add);
#endif /* HAVE_BFD */
  /* "ip community-list" */
  else if (IMI_STRCMP ("ip community-list"))
    imi_config_get (config, COMMUNITY_LIST_MODE, line, imi_config_type_add);
  /* "ip as-path access-list" */
  else if (IMI_STRCMP ("ip as-path access-list"))
    imi_config_get (config, AS_LIST_MODE, line, imi_config_type_add);
  /* "access-list" */
  else if (IMI_STRCMP ("access-list"))
    imi_config_get (config, ACCESS_MODE, line, imi_config_type_add_unique);
  /* "ip prefix-list" */
  else if (IMI_STRCMP ("ip prefix-list"))
    imi_config_get (config, PREFIX_MODE, line, imi_config_type_add_unique);
  /* "ipv6 route" */
  else if (IMI_STRCMP ("ipv6 route"))
    imi_config_get (config, IPV6_MODE, line, imi_config_type_add);
  /* "ipv6 mroute" */
  else if (IMI_STRCMP ("ipv6 mroute"))
    imi_config_get (config, IPV6_MROUTE_MODE, line, imi_config_type_add);
  /* "ipv6 access-list" */
  else if (IMI_STRCMP ("ipv6 access-list"))
    imi_config_get (config, ACCESS_IPV6_MODE, line, imi_config_type_add_unique);
  /* "ipv6 prefix-list" */
  else if (IMI_STRCMP ("ipv6 prefix-list"))
    imi_config_get (config, PREFIX_IPV6_MODE, line, imi_config_type_add_unique);

  /* "route-map" */
  else if (IMI_STRCMP ("route-map"))
    imi_config_get (config, RMAP_MODE, line, imi_config_type_sub);

  /* PIM-DM */
  else if (IMI_STRCMP ("ip pim dense-mode"))
    imi_config_get (config, PDM_MODE, line, imi_config_type_add);

  /* PIM-DM6 */
  else if (IMI_STRCMP ("ipv6 pim dense-mode"))
    imi_config_get (config, PDM6_MODE, line, imi_config_type_add);

  /* PIM-SM */
  else if (IMI_STRCMP ("ip pim"))
    imi_config_get (config, PIM_MODE, line, imi_config_type_add);

  /* PIM-SM6 */
  else if (IMI_STRCMP ("ipv6 pim"))
    imi_config_get (config, PIM6_MODE, line, imi_config_type_add);

#ifdef HAVE_QOS
  else if (IMI_STRCMP ("class-map"))
    imi_config_get (config, CMAP_MODE, line, imi_config_type_sub);

  else if (IMI_STRCMP ("policy-map"))
    imi_config_get (config, PMAP_MODE, line, imi_config_type_sub);

  else if (IMI_STRCMP ("class"))
    imi_config_get (config, PMAPC_MODE, line, imi_config_type_sub);
#endif /* HAVE_QOS */

  /* VLAN */
  else if (IMI_STRCMP ("vlan database"))
    imi_config_get (config, VLAN_MODE, line, imi_config_type_sub);
  else if (IMI_STRCMP ("pbb isid list"))
    imi_config_get (config, PBB_ISID_CONFIG_MODE, line, imi_config_type_sub);
  else if (IMI_STRCMP ("pbb-te configure tesid"))
    imi_config_get (config, PBB_TE_VLAN_CONFIG_MODE, line, imi_config_type_sub);

  else if (IMI_STRCMP ("ethernet cfm domain"))
    imi_config_get (config, ETH_CFM_MODE, line, imi_config_type_sub);
  else if (IMI_STRCMP ("ethernet cfm pbb domain"))
    imi_config_get (config, ETH_CFM_PBB_MODE, line, imi_config_type_sub);
  else if (IMI_STRCMP ("ethernet cfm configure vlan"))
    imi_config_get (config, ETH_CFM_VLAN_MODE, line, imi_config_type_sub);
  else if (IMI_STRCMP ("ethernet cfm pbb-te domain"))
    imi_config_get (config, ETH_CFM_PBB_TE_MODE, line, imi_config_type_sub);
  else if (IMI_STRCMP ("pbb-te configure esp tesi"))
    imi_config_get (config, PBB_TE_ESP_IF_CONFIG_MODE, line, imi_config_type_sub);
  else if (IMI_STRCMP ("pbb-te configure switching"))
    imi_config_get (config, ETH_PBB_TE_EPS_CONFIG_SWITCHING_MODE, line, imi_config_type_sub);


  else if (IMI_STRCMP ("g8031 configure vlan"))
        imi_config_get (config, G8031_CONFIG_VLAN_MODE, line, imi_config_type_sub);
  else if (IMI_STRCMP ("g8031 configure switching"))
        imi_config_get (config,G8031_CONFIG_SWITCHING_MODE, line, imi_config_type_sub);

  else if (IMI_STRCMP ("g8032 configure vlan"))
    imi_config_get (config,G8032_CONFIG_VLAN_MODE, line, imi_config_type_sub);

  else if (IMI_STRCMP ("g8032 configure switching"))
    imi_config_get (config,G8032_CONFIG_MODE, line, imi_config_type_sub);


  else if (IMI_STRCMP ("vlan access-map"))
    imi_config_get (config, VLAN_ACCESS_MODE, line, imi_config_type_sub);

  else if (IMI_STRCMP ("cvlan registration table"))
    imi_config_get (config, CVLAN_REGIST_MODE, line, imi_config_type_sub);

  /* "smux".  */
  else if (IMI_STRCMP ("smux"))
    imi_config_get (config, SMUX_MODE, line, imi_config_type_add);

  /* GVRP.  */
  else if (IMI_STRCMP ("set gvrp") || IMI_STRCMP ("set port gvrp"))
    imi_config_get (config, GVRP_MODE, line, imi_config_type_add);

  /* GMRP.  */
  else if (IMI_STRCMP ("set gmrp") || IMI_STRCMP ("set port gmrp"))
    imi_config_get (config, GMRP_MODE, line, imi_config_type_add);

#if defined HAVE_MCAST_IPV4 || defined HAVE_IGMP_SNOOP
  /* IGMP-Global Mode */
  else if (IMI_STRCMP ("ip igmp"))
   imi_config_get (config, IGMP_MODE, line, imi_config_type_add);
#endif /* HAVE_MCAST_IPV4 || HAVE_IGMP_SNOOP */

  else if (IMI_STRCMP ("arp"))
    imi_config_get (config, ARP_MODE, line, imi_config_type_add);

  /* MSTP */
  else if (IMI_STRCMP ("spanning-tree mst"))
    imi_config_get (config, MST_CONFIG_MODE, line, imi_config_type_add);

#if defined (HAVE_PROVIDER_BRIDGE) || defined (HAVE_B_BEB)
  else if (IMI_STRCMP ("spanning-tree te-msti"))
    imi_config_get (config, TE_MSTI_CONFIG_MODE, line, imi_config_type_add);
#endif /*(HAVE_PROVIDER_BRIDGE) || (HAVE_B_BEB)*/

#ifdef HAVE_RPVST_PLUS
  else if (IMI_STRCMP ("spanning-tree rpvst"))
    imi_config_get (config, RPVST_PLUS_CONFIG_MODE, line,
                    imi_config_type_add);
#endif /* HAVE_RPVST_PLUS */

  else if (IMI_STRCMP ("radius-server"))
    imi_config_get (config, AUTH8021X_MODE, line, imi_config_type_add);

  else if (IMI_STRCMP ("bridge"))
   {
     imi_config_get (config, BRIDGE_MODE , line, imi_config_type_add);
     imi_config_get (config, EXEC_MODE , line, imi_config_type_add);
   }

  /* Router Id */
  else if (IMI_STRCMP ("router-id"))
    imi_config_get (config, CONFIG_MODE, line, imi_config_type_add);

  else if (IMI_STRCMP ("bgp"))
    imi_config_get (config, VR_MODE, line, imi_config_type_add);

  else if (IMI_STRCMP ("bfd"))
    imi_config_get (config, BFD_MODE, line, imi_config_type_add);

  else if (IMI_STRCMP ("stacking"))
    imi_config_get (config, STACKING_MODE, line, imi_config_type_add);
  else if (IMI_STRCMP ("uni evc service instance"))
     imi_config_get (config, EVC_SERVICE_INSTANCE_MODE, line, imi_config_type_add);

  else
    imi_config_get (config, EXEC_MODE, line, imi_config_type_add_unique);
}

/* Interface section is special. */
static void
imi_extended_interface_dump (struct cli *cli, char *name)
{
  struct interface *ifp;

  ifp = imi_interface_line2ifp (name);
  if (ifp)
    {
#ifdef HAVE_NAT
      /* Interface scope config write. */
      imi_interface_scope_config_write (cli, ifp);
#endif /* HAVE_NAT */

#ifdef HAVE_DHCP_CLIENT
      /* DHCP client config write. */
      imi_dhcpc_config_write_if (cli, ifp);
#endif /* HAVE_DHCP_CLIENT */

#ifdef HAVE_ACL
      /* Firewall config write. */
      imi_acl_if_config_write (cli, ifp);
#endif /* HAVE_ACL */
    }
}

/* Write state for mode. */
void
imi_config_write_mode (struct cli *cli, struct imi_config *config,
                       int mode)
{
  struct imi_config_mode *cm;
  struct imi_config_mode *sub;
  struct imi_config_line *line;
  struct interface *ifp;
  struct imi_interface *imi_if;

  cm = imi_config_lookup (config, mode);
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
              if (mode == INTERFACE_MODE && ! pal_strncmp (line->str, " ip address", 11))
                {
                  ifp = imi_interface_line2ifp(sub->str);
                  if ((ifp) && (ifp->info))
                    {
                      imi_if = ifp->info;
                      if (imi_if->idc == NULL)
                        cli_out (cli, "%s\n", line->str);
                    }
                }
              else
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

          /* Add IMI specific interface configuration. */
          if (mode == INTERFACE_MODE
              && ! pal_strncmp (sub->str, "interface ", 10))
            imi_extended_interface_dump (cli, sub->str);

          cli_out (cli, "!\n");
        }
    }
}

/* Query configuration to each protocol.  */
void
imi_config_build (struct apn_vr *vr,
                  struct imi_config *config, char *str, modbmap_t module)
{
  if (MODBMAP_ISSET (module, APN_PROTO_NSM))
    imi_show_protocol (vr, APN_PROTO_NSM, NULL,
                       str, config, &imi_config_parse);
  if (MODBMAP_ISSET (module, APN_PROTO_RIP))
    imi_show_protocol (vr, APN_PROTO_RIP, NULL,
                       str, config, &imi_config_parse);
  if (MODBMAP_ISSET (module, APN_PROTO_RIPNG))
    imi_show_protocol (vr, APN_PROTO_RIPNG, NULL,
                       str, config, &imi_config_parse);
  if (MODBMAP_ISSET (module, APN_PROTO_OSPF))
    imi_show_protocol (vr, APN_PROTO_OSPF, NULL,
                       str, config, &imi_config_parse);
  if (MODBMAP_ISSET (module, APN_PROTO_OSPF6))
    imi_show_protocol (vr, APN_PROTO_OSPF6, NULL,
                       str, config, &imi_config_parse);
  if (MODBMAP_ISSET (module, APN_PROTO_ISIS))
    imi_show_protocol (vr, APN_PROTO_ISIS, NULL,
                       str, config, &imi_config_parse);
  if (MODBMAP_ISSET (module, APN_PROTO_BGP))
    imi_show_protocol (vr, APN_PROTO_BGP, NULL,
                       str, config, &imi_config_parse);
  if (MODBMAP_ISSET (module, APN_PROTO_LDP))
    imi_show_protocol (vr, APN_PROTO_LDP, NULL,
                       str, config, &imi_config_parse);
  if (MODBMAP_ISSET (module, APN_PROTO_RSVP))
    imi_show_protocol (vr, APN_PROTO_RSVP, NULL,
                       str, config, &imi_config_parse);
  if (MODBMAP_ISSET (module, APN_PROTO_PIMDM))
    imi_show_protocol (vr, APN_PROTO_PIMDM, NULL,
                       str, config, &imi_config_parse);
  if (MODBMAP_ISSET (module, APN_PROTO_PIMSM))
    imi_show_protocol (vr, APN_PROTO_PIMSM, NULL,
                       str, config, &imi_config_parse);
  if (MODBMAP_ISSET (module, APN_PROTO_PIMSM6))
    imi_show_protocol (vr, APN_PROTO_PIMSM6, NULL,
                       str, config, &imi_config_parse);
  if (MODBMAP_ISSET (module, APN_PROTO_DVMRP))
    imi_show_protocol (vr, APN_PROTO_DVMRP, NULL,
                       str, config, &imi_config_parse);
  if (MODBMAP_ISSET (module, APN_PROTO_8021X))
    imi_show_protocol (vr, APN_PROTO_8021X, NULL,
                       str, config, &imi_config_parse);
  if (MODBMAP_ISSET (module, APN_PROTO_ONM))
    imi_show_protocol (vr, APN_PROTO_ONM, NULL,
                       str, config, &imi_config_parse);
  if (MODBMAP_ISSET (module, APN_PROTO_LACP))
    imi_show_protocol (vr, APN_PROTO_LACP, NULL,
                       str, config, &imi_config_parse);
  if (MODBMAP_ISSET (module, APN_PROTO_STP))
    imi_show_protocol (vr, APN_PROTO_STP, NULL,
                       str, config, &imi_config_parse);
  if (MODBMAP_ISSET (module, APN_PROTO_RSTP))
    imi_show_protocol (vr, APN_PROTO_RSTP, NULL,
                       str, config, &imi_config_parse);
  if (MODBMAP_ISSET (module, APN_PROTO_MSTP))
    imi_show_protocol (vr, APN_PROTO_MSTP, NULL,
                       str, config, &imi_config_parse);
  if (MODBMAP_ISSET (module, APN_PROTO_RMON))
    imi_show_protocol (vr, APN_PROTO_RMON, NULL,
                       str, config, &imi_config_parse);
  if (MODBMAP_ISSET (module, APN_PROTO_OAM))
    imi_show_protocol (vr, APN_PROTO_OAM, NULL,
                       str, config, &imi_config_parse);
  if (MODBMAP_ISSET (module, APN_PROTO_ELMI))
    imi_show_protocol (vr, APN_PROTO_ELMI, NULL,
                       str, config, &imi_config_parse);
  if (MODBMAP_ISSET (module, APN_PROTO_VLOG))
    imi_show_protocol (vr, APN_PROTO_VLOG, NULL,
                       str, config, &imi_config_parse);
#ifdef HAVE_LMP
  if (MODBMAP_ISSET (module, APN_PROTO_LMP))
    imi_show_protocol (vr, APN_PROTO_LMP, NULL,
                       str, config, &imi_config_parse);
#endif /* HAVE_LMP */
}

#ifdef HAVE_IMISH

#define LINE_SAME(L1, L2)                                                     \
    ((L1)->config != (L2)->config ? 0 :                                       \
     (L1)->privilege != (L2)->privilege ? 0 :                                 \
     (L1)->exec_timeout_min != (L2)->exec_timeout_min ? 0 :                   \
     (L1)->exec_timeout_sec != (L2)->exec_timeout_sec ? 0 :                   \
     (L1)->maxhist != (L2)->maxhist ? 0 : 1)

/* Encode line conifguration. */
void
imi_line_config_out_encode (struct line *line,
                            u_char type,
                            int min, int max,
                            cfg_vect_t *cv)
{
  if (line == NULL)
    return;

  if (min == max)
    cfg_vect_add_cmd (cv, "line %s %d\n", LINE_TYPE_STR (type), min);
  else
    cfg_vect_add_cmd (cv, "line %s %d %d\n", LINE_TYPE_STR (type), min, max);

  /* "exec-timeout" */
  if (CHECK_FLAG (line->config, LINE_CONFIG_TIMEOUT))
    cfg_vect_add_cmd (cv, " exec-timeout %d %d\n",
             line->exec_timeout_min, line->exec_timeout_sec);

  /* "privilege" */
  if (CHECK_FLAG (line->config, LINE_CONFIG_PRIVILEGE))
    cfg_vect_add_cmd (cv, " privilege level %d\n", line->privilege);

  /* "history max" */
  if (CHECK_FLAG (line->config, LINE_CONFIG_HISTORY))
    cfg_vect_add_cmd (cv, " history max %d\n", line->maxhist);

  /* "login" */
  if (!CHECK_FLAG (line->config, LINE_CONFIG_LOGIN)
      && !CHECK_FLAG (line->config, LINE_CONFIG_LOGIN_LOCAL))
    cfg_vect_add_cmd (cv, " no login\n");
  else if (CHECK_FLAG (line->config, LINE_CONFIG_LOGIN))
    cfg_vect_add_cmd (cv, " login\n");
  else if (CHECK_FLAG (line->config, LINE_CONFIG_LOGIN_LOCAL))
    cfg_vect_add_cmd (cv, " login local\n");
}

/* Display specified line type configuration.  */
int
imi_line_config_type_encode (struct imi_master *im, u_char type, cfg_vect_t *cv)
{
  struct line *line, *line_last;
  int i;
  int min, max;

#ifndef HAVE_LINE_AUX
  /* Skip AUX.  */
  if (type == LINE_TYPE_AUX)
    return 0;
#endif /* HAVE_LINE_AUX */

  /* Initialize current index.  */
  min = max = 0;
  line_last = NULL;

  for (i = 0; i < vector_max (im->lines[type]); i++)
    {
      line = vector_slot (im->lines[type], i);

      /* When we reach to the last entry.  */
      if (line == NULL)
        {
          imi_line_config_out_encode (line_last, type, min, max, cv);
          return 0;
        }

      /* Remember current line.  */
      if (line_last == NULL)
        line_last = line;

      /* When configuration is same, just skip it.  */
      if (LINE_SAME (line_last, line))
        {
          max = i;
          continue;
        }

      /* Display configuration.  */
      imi_line_config_out_encode (line_last, type, min, max, cv);

      /* Update max index. */
      min = i;
      max = i;
      line_last = line;
    }

  /* Display configuration.  */
  imi_line_config_out_encode (line_last, type, min, max, cv);

  return 0;
}

int
imi_line_config_encode (struct imi_master *im, cfg_vect_t *cv)
{
  int type;

  for (type = LINE_TYPE_CONSOLE; type < LINE_TYPE_MAX; type++)
    imi_line_config_type_encode (im, type, cv);

  if (cfg_vect_count(cv)) {
    cfg_vect_add_cmd(cv, "!\n");
  }
  return 0;
}

int
imi_line_config_write (struct cli *cli)
{
  cli->cv = cfg_vect_init(cli->cv);
  imi_line_config_encode(cli->vr->proto, cli->cv);
  cfg_vect_out(cli->cv, (cfg_vect_out_fun_t)cli->out_func, cli->out_val);
  return 0;
}

#endif /* HAVE_IMISH */

/*--------------------------------------------------------------------------
 * VR config encode/write.
 * Used only in case of VR 0.
 *--------------------------------------------------------------------------
 */
#ifdef HAVE_VR
int
imi_vr_config_encode (cfg_vect_t *cv)
{
  struct apn_vr *vr;
  struct imi_master *im;
  int i;

  for (i = 1; i < vector_max (imim->vr_vec); i++)
    if ((vr = vector_slot (imim->vr_vec, i)))
      {
        im = vr->proto;
        cfg_vect_add_cmd (cv, "virtual-router %s\n", vr->name);

        if (im->desc)
          cfg_vect_add_cmd (cv, " description %s\n", im->desc);

        if (im->filename)
          cfg_vect_add_cmd (cv, " configuration file %s\n", im->filename);

#if 0  /* PacOS_33684: It is considered a dead code,
          but - this is IMI and you never know...
        */
        struct imi_config_mode *sub;
        struct imi_config_line *line;
        struct imi_config_mode *cm; /* 33684: move it up if necessary */
        cm = imi_config_lookup (config, VR_MODE);
        if (cm)
          {
            FIFO_LOOP (&cm->sub, sub)
              if (pal_strcmp (&sub->str[15], vr->name) == 0)
                {
                  FIFO_LOOP (&sub->main, line)
                    cli_out (v, "%s\n", line->str);
                }
          }
#endif
        if (MODBMAP_ISSET (im->module_config, APN_PROTO_OSPF))
          cfg_vect_add_cmd (cv, " load ospf\n");
        if (MODBMAP_ISSET (im->module_config, APN_PROTO_BGP))
          cfg_vect_add_cmd (cv, " load bgp\n");
        if (MODBMAP_ISSET (im->module_config, APN_PROTO_RIP))
          cfg_vect_add_cmd (cv, " load rip\n");
        if (MODBMAP_ISSET (im->module_config, APN_PROTO_ISIS))
          cfg_vect_add_cmd (cv, " load isis\n");
#ifdef HAVE_MCAST_IPV4
        if (MODBMAP_ISSET (im->module_config, APN_PROTO_PIMSM))
          cfg_vect_add_cmd (cv, " load pimsm\n");
#endif /* HAVE_MCAST_IPV4 */

#ifdef HAVE_IPV6
        if (MODBMAP_ISSET (im->module_config, APN_PROTO_OSPF6))
          cfg_vect_add_cmd (cv, " load ipv6 ospf\n");
        if (MODBMAP_ISSET (im->module_config, APN_PROTO_RIPNG))
          cfg_vect_add_cmd (cv, " load ipv6 rip\n");
#ifdef HAVE_MCAST_IPV6
        if (MODBMAP_ISSET (im->module_config, APN_PROTO_PIMSM6))
          cfg_vect_add_cmd (cv, " load ipv6 pimsm6\n");
#endif /* HAVE_MCAST_IPV6 */
#endif /* HAVE_IPV6 */
#if defined HAVE_MCAST_IPV4 || defined HAVE_MCAST_IPV6
        if (MODBMAP_ISSET (im->module_config, APN_PROTO_PIMDM))
          cfg_vect_add_cmd (cv, " load pimdm\n");
#endif /* defined HAVE_MCAST_IPV4 || defined HAVE_MCAST_IPV6 */

        host_config_encode_user_all_vr (vr->host, cv);

        cfg_vect_add_cmd (cv, "!\n");
      }

  return 0;
}

int
imi_vr_config_write (struct cli *cli)
{
  if (cli->vr->id != 0)
    return 0;

  cli->cv = cfg_vect_init(cli->cv);
  imi_vr_config_encode(cli->cv);
  cfg_vect_out(cli->cv, (cfg_vect_out_fun_t)cli->out_func, cli->out_val);
  return 0;
}

#endif /* HAVE_VR */


static void
_imi_config_write_local(struct cli *cli, cfgDataType_e dtype)
{
  switch (dtype)
    {
    case CFG_DTYP_IMI_HOST_SERVICE:
      host_service_write (cli);
      break;

#ifdef HAVE_DHCP_SERVER
    case CFG_DTYP_IMI_DHCPS_SERVICE:
      imi_dhcps_service_write (cli);
      break;
#endif /* HAVE_DHCP_SERVER */

    case CFG_DTYP_IMI_HOST:
      host_config_write (cli);
      break;

#ifdef HAVE_CRX
    case CFG_DTYP_IMI_CRX_DEBUG:
      crx_debug_config_write (cli);
      break;
#endif /* HAVE_CRX */

  /* This must be just after service command.
     Because PPPoE use "service" statement.
   */
#ifdef HAVE_PPPOE
    case CFG_DTYP_IMI_PPPOE:
      imi_pppoe_config_write (cli);
      break;
#endif /* HAVE_PPPOE */

#ifdef HAVE_DNS
    case CFG_DTYP_IMI_DNS:
      imi_dns_config_write (cli);
      break;
#endif /* HAVE_DNS */

#ifdef HAVE_DHCP_SERVER
    case CFG_DTYP_IMI_DHCPS:
      imi_dhcps_config_write (cli);
      break;
#endif /* HAVE_DHCP_SERVER */

#ifdef HAVE_CUSTOM1
    case CFG_DTYP_IMI_SNMP:
      imi_snmp_config_write (cli);
      break;
#endif /* HAVE_CUSTOM1 */

    case CFG_DTYP_KEYCHAIN:
      keychain_config_write (cli);
      break;

    case CFG_DTYP_ACCESS_IPV4:
      /* ip access-list ... */
      config_write_access_ipv4 (cli);
      break;

    case CFG_DTYP_PREFIX_IPV4:
      /* ip prefix-list ... */
      config_write_prefix_ipv4 (cli);
      break;

#ifdef HAVE_IPV6
    case CFG_DTYP_ACCESS_IPV6:
      /* ipv6 access-list ... */
      config_write_access_ipv6 (cli);
      break;

    case CFG_DTYP_PREFIX_IPV6:
      /* ipv6 prefix-list ... */
      config_write_prefix_ipv6 (cli);
      break;
#endif /* HAVE_IPV6 */

    case CFG_DTYP_RMAP:
      route_map_config_write (cli);
      break;

#ifdef HAVE_NAT
    case CFG_DTYP_IMI_NAT:
      imi_nat_config_write (cli);
      break;

    case CFG_DTYP_IMI_VIRTUAL_SERVER:
      imi_virtual_server_config_write (cli);
      break;
#endif /* HAVE_NAT */

#ifdef HAVE_NTP
    case CFG_DTYP_IMI_NTP:
      imi_ntp_config_write (cli);
      break;
#endif /* HAVE_NTP. */

#ifdef HAVE_APS_TOOLKIT
    case CFG_DTYP_IMI_EXTERN:
      imi_extern_config_write ();
      break;
#endif /* HAVE_APS_TOOLKIT */

#ifdef HAVE_CRX
    case CFG_DTYP_IMI_CRX:
      crx_config_write (cli);
      break;
#endif /* HAVE_CRX */

    case CFG_DTYP_IMI_LINE:
      IMI_LINE_CONFIG_WRITE (cli);
      break;

#ifdef HAVE_VR
    case CFG_DTYP_IMI_VR:
      imi_vr_config_write (cli);
      break;
#endif /* HAVE_VR */

    default:
      break;
    }
}

static void
_imi_config_cfg_seq_walk_cb(intptr_t cli_ref,
                            intptr_t cfg_ref,
                            int           store_type,
                            cfgDataType_e data_type,
                            int           config_mode)
{
  struct cli *cli = (struct cli *)cli_ref;
  struct imi_config *imi_cfg = (struct imi_config *)cfg_ref;

  switch (store_type)
    {
      case CFG_STORE_IMI:
        /* Write locally - in IMI stored configuration data. */
        _imi_config_write_local(cli, data_type);
        break;

      case CFG_STORE_PM:
        /* Write configuration retrieved from protocol modules. */
        imi_config_write_mode(cli, imi_cfg, config_mode);
        break;

      default:
        cli_out(cli, "!\n");
        cli_out(cli, "! **** Unhandled storage type: %d - data type: %d ****\n",
                store_type, data_type);
        cli_out(cli, "!\n");
    }
}

/* Write full PacOS configuration using the config sequencer. */
void
imi_config_write_config (struct cli *cli, struct imi_config *imi_cfg)
{
  cli->cv = cfg_vect_init(cli->cv);

  /* Starting... */
  cli_out (cli, "!\n");

  cfg_seq_walk((intptr_t)cli, (intptr_t)imi_cfg,
               _imi_config_cfg_seq_walk_cb);

  /* End should be displayed last. */
  cli_out (cli, "end\n\n");
}

#define IMI_SHOW_STR_PM          "show running-config pm"
#define IMI_SHOW_STR_IF          "show running-config interface"
#define IMI_SHOW_STR_INSTANCE    "show running-config instance"
#define BRIDGE_CMD(A)            pal_strstr (A, "bridge") ||                  \
                                 pal_strstr (A, "switch") ||                  \
                                 pal_strstr (A, "user") ||                    \
                                 pal_strstr (A, "traffic") ||                 \
                                 pal_strstr (A, "flow") ||                    \
                                 pal_strstr (A, "shut") ||                    \
                                 pal_strstr (A, "mirror")

#define IMI_SHOW_STR(M)                                                       \
    ((M) == RIP_MODE    ? IMI_SHOW_STR_INSTANCE :                             \
     (M) == RIPNG_MODE  ? IMI_SHOW_STR_INSTANCE :                             \
     (M) == OSPF_MODE   ? IMI_SHOW_STR_INSTANCE :                             \
     (M) == OSPF6_MODE  ? IMI_SHOW_STR_INSTANCE :                             \
     (M) == ISIS_MODE   ? IMI_SHOW_STR_INSTANCE :                             \
     (M) == BGP_MODE    ? IMI_SHOW_STR_INSTANCE :                             \
     (M) == LDP_MODE    ? IMI_SHOW_STR_INSTANCE :                             \
     (M) == RSVP_MODE   ? IMI_SHOW_STR_INSTANCE :                             \
     (M) == PDM_MODE    ? IMI_SHOW_STR_INSTANCE :                             \
     (M) == PDM6_MODE   ? IMI_SHOW_STR_INSTANCE :                             \
     (M) == PIM_MODE    ? IMI_SHOW_STR_INSTANCE :                             \
     (M) == BFD_MODE    ? IMI_SHOW_STR_INSTANCE :                             \
     (M) == PIM6_MODE   ? IMI_SHOW_STR_INSTANCE :                             \
     (M) == DVMRP_MODE  ? IMI_SHOW_STR_INSTANCE :                             \
                          IMI_SHOW_STR_PM)

/* Write configuration for each mode.  */
void
imi_config_write_pm (struct cli *cli, modbmap_t module, int mode)
{
  struct imi_config *imi_config;

  /* Allocate a new configuration.  */
  imi_config = imi_config_new ();

  /* Begin with version number. */
  cli_out (cli, "!\n");

  /* Build configuration.  */
  imi_config_build (cli->vr, imi_config, IMI_SHOW_STR (mode), module);

  /* Write mode's configuration.  */
  imi_config_write_mode (cli, imi_config, mode);

  /* Free used configuration.  */
  imi_config_free (imi_config);
}

/* Write configuration for each mode.  */
void
imi_config_write_if (struct cli *cli, char *ifname, modbmap_t module)
{
  char ifline[INTERFACE_NAMSIZ+11];
  struct imi_config *imi_config;
  struct imi_config_mode *cm;
  struct imi_config_mode *sub;
  struct imi_config_line *line;
  int written = 0;

  /* Allocate a new configuration.  */
  imi_config = imi_config_new ();

  /* Build configuration.  */
  imi_config_build (cli->vr, imi_config, IMI_SHOW_STR_PM, module);

  /* Write interface configuration.  */
  cm = imi_config_lookup (imi_config, INTERFACE_MODE);
  if (cm)
    {
      FIFO_LOOP (&cm->sub, sub)
        {
          zsnprintf (ifline, INTERFACE_NAMSIZ+11, "interface %s", ifname);
          if (sub->str && (pal_strcmp (sub->str, ifline) == 0))
            {
              written = 1;

              cli_out (cli, "!\n");
              cli_out (cli, "%s\n", sub->str);

              FIFO_LOOP (&sub->main, line)
                cli_out (cli, "%s\n", line->str);

              FIFO_LOOP (&sub->match, line)
                cli_out (cli, "%s\n", line->str);

              FIFO_LOOP (&sub->set, line)
                cli_out (cli, "%s\n", line->str);

              /* Add IMI specific interface configuration. */
              imi_extended_interface_dump (cli, sub->str);

              cli_out (cli, "!\n");
              break;
            }
        }
    }

  /* Interface data written? */
  if (!written)
    cli_out (cli, "%% Can't find interface %s.\n", ifname);

  /* Free used configuration.  */
  imi_config_free (imi_config);
}

/* Write configuration for TE Link.  */
void
imi_config_write_te_link (struct cli *cli, char *tl_name, modbmap_t module)
{
  char tl_line[IMI_BUFSIZ];
  struct imi_config *imi_config;
  struct imi_config_mode *cm;
  struct imi_config_mode *sub;
  struct imi_config_line *line;
  int written = 0;

  /* Allocate a new configuration.  */
  imi_config = imi_config_new ();

  /* Build configuration.  */
  imi_config_build (cli->vr, imi_config, IMI_SHOW_STR_PM, module);

  /* Write interface configuration.  */
  cm = imi_config_lookup (imi_config, TELINK_MODE);
  if (cm)
    {
      FIFO_LOOP (&cm->sub, sub)
        {
          zsnprintf (tl_line, IMI_BUFSIZ, "te-link %s ", tl_name);
          if (sub->str && pal_strstr (sub->str, tl_line))
            {
              written = 1;

              cli_out (cli, "!\n");
              cli_out (cli, "%s\n", sub->str);

              FIFO_LOOP (&sub->main, line)
                cli_out (cli, "%s\n", line->str);

              FIFO_LOOP (&sub->match, line)
                cli_out (cli, "%s\n", line->str);

              FIFO_LOOP (&sub->set, line)
                cli_out (cli, "%s\n", line->str);

              /* Add IMI specific interface configuration. */
              imi_extended_interface_dump (cli, sub->str);

              cli_out (cli, "!\n");
              break;
            }
        }
    }

  /* Interface data written? */
  if (!written)
    cli_out (cli, "%% Can't find te-link %s.\n", tl_name);

  /* Free used configuration.  */
  imi_config_free (imi_config);
}

/* Write configuration for TE Link.  */
void
imi_config_write_cc (struct cli *cli, char *ccname, modbmap_t module)
{
  char cc_line[IMI_BUFSIZ];
  struct imi_config *imi_config;
  struct imi_config_mode *cm;
  struct imi_config_mode *sub;
  struct imi_config_line *line;
  int written = 0;

  /* Allocate a new configuration.  */
  imi_config = imi_config_new ();

  /* Build configuration.  */
  imi_config_build (cli->vr, imi_config, IMI_SHOW_STR_PM, module);

  /* Write interface configuration.  */
  cm = imi_config_lookup (imi_config, CTRL_CHNL_MODE);
  if (cm)
    {
      FIFO_LOOP (&cm->sub, sub)
        {
          zsnprintf (cc_line, IMI_BUFSIZ, "control-channel %s ", ccname);
          if (sub->str && pal_strstr (sub->str, cc_line))
            {
              written = 1;

              cli_out (cli, "!\n");
              cli_out (cli, "%s\n", sub->str);

              FIFO_LOOP (&sub->main, line)
                cli_out (cli, "%s\n", line->str);

              FIFO_LOOP (&sub->match, line)
                cli_out (cli, "%s\n", line->str);

              FIFO_LOOP (&sub->set, line)
                cli_out (cli, "%s\n", line->str);

              /* Add IMI specific interface configuration. */
              imi_extended_interface_dump (cli, sub->str);

              cli_out (cli, "!\n");
              break;
            }
        }
    }

  /* Interface data written? */
  if (!written)
    cli_out (cli, "%% Can't find control channel %s.\n", ccname);

  /* Free used configuration.  */
  imi_config_free (imi_config);
}

/* Write configuration for TE Link.  */
void
imi_config_write_ca (struct cli *cli, char *caname, modbmap_t module)
{
  char ca_line[IMI_BUFSIZ];
  struct imi_config *imi_config;
  struct imi_config_mode *cm;
  struct imi_config_mode *sub;
  struct imi_config_line *line;
  int written = 0;

  /* Allocate a new configuration.  */
  imi_config = imi_config_new ();

  /* Build configuration.  */
  imi_config_build (cli->vr, imi_config, IMI_SHOW_STR_PM, module);

  /* Write interface configuration.  */
  cm = imi_config_lookup (imi_config, CTRL_ADJ_MODE);
  if (cm)
    {
      FIFO_LOOP (&cm->sub, sub)
        {
          zsnprintf (ca_line, IMI_BUFSIZ, "control-adjacency %s ", caname);
          if (sub->str && pal_strstr (sub->str, ca_line))
            {
              written = 1;

              cli_out (cli, "!\n");
              cli_out (cli, "%s\n", sub->str);

              FIFO_LOOP (&sub->main, line)
                cli_out (cli, "%s\n", line->str);

              FIFO_LOOP (&sub->match, line)
                cli_out (cli, "%s\n", line->str);

              FIFO_LOOP (&sub->set, line)
                cli_out (cli, "%s\n", line->str);

              /* Add IMI specific interface configuration. */
              imi_extended_interface_dump (cli, sub->str);

              cli_out (cli, "!\n");
              break;
            }
        }
    }

  /* Interface data written? */
  if (!written)
    cli_out (cli, "%% Can't find control adjacency %s.\n", caname);

  /* Free used configuration.  */
  imi_config_free (imi_config);
}

#ifdef HAVE_LMP
/* Write configuration for LMP */
void
imi_config_write_lmp (struct cli *cli, modbmap_t module)
{
  char lmp_line[IMI_BUFSIZ];
  struct imi_config *imi_config;
  struct imi_config_mode *cm;
  struct imi_config_mode *sub;
  struct imi_config_line *line;
  int written = 0;

  /* Allocate a new configuration.  */
  imi_config = imi_config_new ();

  /* Build configuration.  */
  imi_config_build (cli->vr, imi_config, IMI_SHOW_STR_PM, module);

  /* Write LMP configuration configuration.  */
  cm = imi_config_lookup (imi_config, LMP_MODE);
  if (cm)
    {
      FIFO_LOOP (&cm->sub, sub)
        {
          zsnprintf (lmp_line, IMI_BUFSIZ, "lmp-configuration");
          if (sub->str && (pal_strcmp (sub->str, lmp_line) == 0))
            {
              written = 1;

              cli_out (cli, "!\n");
              cli_out (cli, "%s\n", sub->str);

              FIFO_LOOP (&sub->main, line)
                cli_out (cli, "%s\n", line->str);

              FIFO_LOOP (&sub->match, line)
                cli_out (cli, "%s\n", line->str);

              FIFO_LOOP (&sub->set, line)
                cli_out (cli, "%s\n", line->str);

              /* Add IMI specific interface configuration. */
              imi_extended_interface_dump (cli, sub->str);

              cli_out (cli, "!\n");
              break;
            }
        }
    }

  /* Interface data written? */
  if (!written)
    cli_out (cli, "%% Can't find LMP configuration\n");

  /* Free used configuration.  */
  imi_config_free (imi_config);
}
#endif /* HAVE_LMP */

/* Write mcast igmp bridge configuration present in Interface mode.  */
void
imi_config_write_if_nsm_config (struct cli *cli, char *ifname,
                                modbmap_t module, u_int32_t mode)
{
  char ifline[INTERFACE_NAMSIZ+11];
  struct imi_config *imi_config;
  struct imi_config_mode *cm;
  struct imi_config_mode *sub;
  struct imi_config_line *line;
  int written = 0;

  /* Allocate a new configuration.  */
  imi_config = imi_config_new ();

  /* Build configuration.  */
  imi_config_build (cli->vr, imi_config, IMI_SHOW_STR_IF, module);

  /* Write interface configuration.  */
  cm = imi_config_lookup (imi_config, INTERFACE_MODE);
  if (cm)
    {
      FIFO_LOOP (&cm->sub, sub)
        {
          zsnprintf (ifline, INTERFACE_NAMSIZ+11, "interface %s", ifname);
          if (sub->str && (pal_strcmp (sub->str, ifline) == 0))
            {
              written = 1;

              cli_out (cli, "!\n");
              cli_out (cli, "%s\n", sub->str);

              FIFO_LOOP (&sub->main, line)
                {
                  switch (mode)
                   {
                     case NSM_MCAST_MODE:
                       if ((pal_strstr (line->str, " ip multicast")))
                         cli_out (cli, "%s\n", line->str);
                       break;

                     case IGMP_IF_MODE:
                       if ((pal_strstr (line->str, " ip igmp")))
                         cli_out (cli, "%s\n", line->str);
                       break;

                     case BRIDGE_MODE:
                       if (BRIDGE_CMD (line->str))
                         cli_out (cli, "%s\n", line->str);
                       break;

                     default:
                       break;
                   }
                }

                cli_out (cli, "!\n");
                break;
            }
        }
    }

  /* Interface data written? */
  if (!written)
    cli_out (cli, "%% Can't find interface %s.\n", ifname);

  /* Free used configuration.  */
  imi_config_free (imi_config);
}

#ifdef HAVE_VRF
/* Write configuration for vrf mode.  */
void
imi_config_write_vrf (struct cli *cli, char *vrf_name, modbmap_t module)
{
  char vrf_line[IMI_BUFSIZ];
  struct imi_config *imi_config;
  struct imi_config_mode *cm;
  struct imi_config_mode *sub;
  struct imi_config_line *line;
  int written = 0;

  /* Allocate a new configuration.  */
  imi_config = imi_config_new ();

  /* Build configuration.  */
  imi_config_build (cli->vr, imi_config, IMI_SHOW_STR_PM, module);

  /* Write VRF mode configuration.  */
  cm = imi_config_lookup (imi_config, VRF_MODE);
  if (cm)
    {
      FIFO_LOOP (&cm->sub, sub)
        {
          zsnprintf (vrf_line, IMI_BUFSIZ, "ip vrf %s", vrf_name);
          if (sub->str && (pal_strcmp (sub->str, vrf_line) == 0))
            {
              written = 1;

              cli_out (cli, "!\n");
              cli_out (cli, "%s\n", sub->str);

              FIFO_LOOP (&sub->main, line)
                cli_out (cli, "%s\n", line->str);

              FIFO_LOOP (&sub->match, line)
                cli_out (cli, "%s\n", line->str);

              FIFO_LOOP (&sub->set, line)
                cli_out (cli, "%s\n", line->str);

              cli_out (cli, "!\n");
              break;
            }
        }
    }

  /* Interface data written? */
  if (!written)
    cli_out (cli, "%% Can't find vrf %s.\n", vrf_name);

  /* Free used configuration.  */
  imi_config_free (imi_config);
}
#endif /* HAVE_VRF */

void
imi_config_write_filter (struct cli *cli, int mode)
{
  int ret = 0;

  /* Begin with version number. */
  cli_out (cli, "!\n");

  switch (mode)
    {
    case ACCESS_MODE:
      ret = config_write_access_ipv4 (cli);
      break;

    case PREFIX_MODE:
      ret = config_write_prefix_ipv4 (cli);
      break;

#ifdef HAVE_IPV6
    case ACCESS_IPV6_MODE:
      ret = config_write_access_ipv6 (cli);
      break;

    case PREFIX_IPV6_MODE:
      ret = config_write_prefix_ipv6 (cli);
      break;
#endif /* HAVE_IPV6 */

    case RMAP_MODE:
      ret = route_map_config_write (cli);
      break;
    }

  if (ret)
    cli_out (cli, "!\n");
}

/* Write key chain mode configuration */
void
imi_config_write_keychain (struct cli *cli)
{
  int ret = 0;

  /* Begin with version number. */
  cli_out (cli, "!\n");

  ret = keychain_config_write (cli);
  if (ret)
    cli_out (cli, "!\n");

}

/* Write full PacOS configuration.  */
void
imi_config_write_full (struct cli *cli, int type)
{
  struct imi_config *imi_config;

  imi_config = imi_config_new ();

  /* Build configuration.  */
  if(type==0)
   { 
   imi_config_build (cli->vr, imi_config, IMI_SHOW_STR_PM, PM_ALL);// 11.2
  	}
  else
  	{
  	imi_config_build (cli->vr, imi_config, IMI_SHOW_STR_PM, PM_HOSTNAME);//11.2
   }

  /* Write configuration. */
  imi_config_write_config (cli, imi_config);

  /* Free displayed configuration.  */
  imi_config_free (imi_config);
}

/* Write full BGP configuration.  */
void
imi_config_write_bgp_vr (struct cli *cli)
{
  struct imi_config *imi_config;

  imi_config = imi_config_new ();

  /* Build configuration.  */
  imi_config_build (cli->vr, imi_config, IMI_SHOW_STR_PM, PM_BGP);

  /* Write configuration. */
  /* Begin with version number. */
  cli_out (cli, "!\n");

  /* BGP VR MODE */
  imi_config_write_mode (cli, imi_config, VR_MODE);
  imi_config_write_mode (cli, imi_config, BGP_MODE);

  /* Free displayed configuration.  */
  imi_config_free (imi_config);
}

#ifdef HAVE_IMISH
/* Execute command for start up configuration.  */
void
imi_startup_execute (struct cli_tree *ctree, struct cli *cli, char *str)
{
  struct cli_node *node = ctree->exec_node;

  /* Execute local IMI only.  PMs will get the configuration later
     when PM connects to the IMI.  */
  if (node->cel->func)
    (*node->cel->func) (cli, ctree->argc, ctree->argv);

  cli_free_arguments (ctree);
}

/* Load the start up configuration.  */
int
imi_config_read (struct apn_vr *vr)
{
  char buf[IMI_BUFSIZ];
  struct cli *cli;
  struct line line;
  int length;
  FILE *fp;
  int ret;
  struct apn_vr *saved_vr = LIB_GLOB_GET_VR_CONTEXT(imim);

  /* Open the file. */
  fp = pal_fopen (vr->host->config_file, PAL_OPEN_RO);
  if (fp == NULL)
    return -1;

  LIB_GLOB_SET_VR_CONTEXT(imim, vr);

  /* Line server setting. */
  pal_mem_set (&line, 0, sizeof (struct line));

  /* Set up CLI structure.  */
  cli = &line.cli;
  cli->zg = imim;
  cli->ctree = imim->ctree;
  cli->line = &line;
  cli->mode = CONFIG_MODE;
  cli->source = CLI_SOURCE_FILE;
  cli->vr = vr;

  /* CLI output function and argument.  */
  cli->out_func = (CLI_OUT_FUNC) fprintf;
  cli->out_val = stdout;

  /* Parse line one by one.  Mode is set to CONFIG_MODE.  */
  while (pal_fgets (buf, IMI_BUFSIZ, fp))
    {
      /* Clear trailing new line.  */
      length = strlen (buf);
      if (--length >= 0)
        buf[length] = '\0';

      ret = cli_parse (cli->ctree, cli->mode, PRIVILEGE_MAX, buf, 1, 0);
      switch (ret)
        {
        case CLI_PARSE_EMPTY_LINE:
          /* Simply ignore empty line.  */
          break;
        case CLI_PARSE_SUCCESS:
          imi_startup_execute (cli->ctree, cli, buf);
          break;
        case CLI_PARSE_INCOMPLETE:
        case CLI_PARSE_INCOMPLETE_PIPE:
        case CLI_PARSE_AMBIGUOUS:
        case CLI_PARSE_NO_MATCH:
        case CLI_PARSE_NO_MODE:
        case CLI_PARSE_ARGV_TOO_LONG:
          /* Second try.  */

          /* Free the previous arguments.  */
          cli_free_arguments (cli->ctree);

          cli->mode = CONFIG_MODE;

          ret = cli_parse (cli->ctree, cli->mode, PRIVILEGE_MAX, buf, 1, 0);
          switch (ret)
            {
            case CLI_PARSE_SUCCESS:
              imi_startup_execute (cli->ctree, cli, buf);
              break;
            default:
              break;
            }

          break;
        default:
          break;
        }

      /* Free arguments.  */
      cli_free_arguments (cli->ctree);
    }
  pal_fclose (fp);

  LIB_GLOB_SET_VR_CONTEXT(imim, saved_vr);

  return 0;
}
#endif /* HAVE_IMISH */

/* Write configuration to the file.  */
int
imi_config_write (struct cli *cli, char *file_name, int type)
{
#ifndef HAVE_IMISH
  struct vty *vty;
#endif /* !HAVE_IMISH */
  struct cli cli_file;
  FILE *fp;

  /* Open the file. */
  fp = pal_fopen (file_name, PAL_OPEN_RW);
  if (fp == NULL)
    return -1;

#ifndef HAVE_IMISH
  /* Create pseudo vty structure.  */
  vty = vty_new (cli->vr->zg);
  if (vty == NULL)
    return -1;
  vty->server = cli->vr->zg->vty_master;

  /* Print out message.  */
  cli_out (cli, "Building configuration...\n");
#endif /* !HAVE_IMISH */

  /* Setup the cli structure to dump the configuration to the file.  */
  pal_mem_set (&cli_file, 0, sizeof (struct cli));
  cli_file.zg = cli->vr->zg;
  cli_file.vr = cli->vr;
#ifndef HAVE_IMISH
  cli_file.line = vty;
#endif /* !HAVE_IMISH */
  cli_file.out_func = (int (*) (void *, char *, ...))fprintf;
  cli_file.out_val = fp;

  /* Call show running-config.  */
  imi_config_write_full (&cli_file, type);//11.2

#ifndef HAVE_IMISH
  vty_close (vty);
#endif /* !HAVE_IMISH */

  pal_fclose (fp);

  return 0;
}


/* Show running configuration.  */
CLI (imi_show_running_config,
     imi_show_running_config_cli,
     "show running-config",
     "Show running system information",
     "Current Operating configuration")
{
  imi_config_write_full (cli,0);//11.2
  return CLI_SUCCESS;
}

CLI (imi_show_running_config_virtual_router,
     imi_show_running_config_virtual_router_cli,
     "show running-config virtual-router WORD",
     "Show running system information",
     "Current Operating configuration",
     CLI_VR_STR,
     CLI_VR_NAME_STR)
{
  struct apn_vr *vr;

  vr = apn_vr_lookup_by_name (imim, argv[0]);
  if (vr == NULL)
    {
      cli_out (cli, "%% No such VR");
      return CLI_ERROR;
    }

  cli->vr = vr;
  imi_config_write_full (cli,0);//11.2

  return CLI_SUCCESS;
}

#ifdef HAVE_VRF
CLI (imi_show_running_config_vrf,
     imi_show_running_config_vrf_cli,
     "show running-config vrf WORD",
     "Show running-config information",
     "Current Operating configuration",
     CLI_VRF_STR,
     CLI_VRF_NAME_STR)
{
  imi_config_write_vrf (cli, argv[0], PM_VRF);
  return CLI_SUCCESS;
}
#endif /* HAVE_VRF */

ALI (imi_show_running_config,
     imi_show_running_config_full_cli,
     "show running-config full",
     "Show running system information",
     "Current Operating configuration",
     "full configuration");

ALI (imi_show_running_config,
     imi_write_terminal_cli,
     "write terminal",
     CLI_WRITE_STR,
     "Write to terminal");

CLI (imi_show_running_config_if,
     imi_show_running_config_if_cli,
     "show running-config interface IFNAME",
     "Show running system information",
     "Current Operating configuration",
     "Interface configuration",
     CLI_IFNAME_STR)
{
  imi_config_write_if (cli, argv[0], PM_ALL);
  return CLI_SUCCESS;
}

/* Protocol Module configuration on a given interface */
#ifdef HAVE_RIPD
CLI (imi_show_running_config_if_rip,
     imi_show_running_config_if_rip_cli,
     "show running-config interface IFNAME rip",
     "Show running system information",
     "Current Operating configuration",
     "Interface configuration",
     CLI_IFNAME_STR,
     CLI_RIP_STR)
{
  imi_config_write_if (cli, argv[0], PM_RIP);
  return CLI_SUCCESS;
}
#endif /* HAVE_RIPD */

#ifdef HAVE_OSPFD
CLI (imi_show_running_config_if_ospf,
     imi_show_running_config_if_ospf_cli,
     "show running-config interface IFNAME ospf",
     "Show running system information",
     "Current Operating configuration",
     "Interface configuration",
     CLI_IFNAME_STR,
     CLI_OSPF_STR)
{
  imi_config_write_if (cli, argv[0], PM_OSPF);
  return CLI_SUCCESS;
}
#endif /* HAVE_OSPFD */

#ifdef HAVE_ISISD
CLI (imi_show_running_config_if_isis,
     imi_show_running_config_if_isis_cli,
     "show running-config interface IFNAME isis",
     "Show running system information",
     "Current Operating configuration",
     "Interface configuration",
     CLI_IFNAME_STR,
     CLI_ISIS_STR)
{
  imi_config_write_if (cli, argv[0], PM_ISIS);
  return CLI_SUCCESS;
}
#endif /* HAVE_ISISD */

#ifdef HAVE_IPV6
#ifdef HAVE_RIPNGD
CLI (imi_show_running_config_if_ipv6_rip,
     imi_show_running_config_if_ipv6_rip_cli,
     "show running-config interface IFNAME ipv6 rip",
     "Show running system information",
     "Current Operating configuration",
     "Interface configuration",
     CLI_IFNAME_STR,
     CLI_IPV6_STR,
     CLI_RIPNG_STR)
{
  imi_config_write_if (cli, argv[0], PM_RIPNG);
  return CLI_SUCCESS;
}
#endif /* HAVE_RIPNGD */

#ifdef HAVE_OSPF6D
CLI (imi_show_running_config_if_ipv6_ospf,
     imi_show_running_config_if_ipv6_ospf_cli,
     "show running-config interface IFNAME ipv6 ospf",
     "Show running system information",
     "Current Operating configuration",
     "Interface configuration",
     CLI_IFNAME_STR,
     CLI_IPV6_STR,
     CLI_OSPF6_STR)
{
  imi_config_write_if (cli, argv[0], PM_OSPF6);
  return CLI_SUCCESS;
}
#endif /* HAVE_OSPF6D */
#endif /* HAVE_IPV6 */

#ifdef HAVE_MPLS
CLI (imi_show_running_config_if_mpls,
     imi_show_running_config_if_mpls_cli,
     "show running-config interface IFNAME mpls",
     "Show running system information",
     "Current Operating configuration",
     "Interface configuration",
     CLI_IFNAME_STR,
     CLI_MPLS_STR)
{
  imi_config_write_if (cli, argv[0], PM_MPLS);
  return CLI_SUCCESS;
}
#endif /* HAVE_MPLS */

#ifdef HAVE_LDPD
CLI (imi_show_running_config_if_ldp,
     imi_show_running_config_if_ldp_cli,
     "show running-config interface IFNAME ldp",
     "Show running system information",
     "Current Operating configuration",
     "Interface configuration",
     CLI_IFNAME_STR,
     CLI_LDP_STR)
{
  imi_config_write_if (cli, argv[0], PM_LDP);
  return CLI_SUCCESS;
}
#endif /* HAVE_LDPD */

#ifdef HAVE_RSVPD
CLI (imi_show_running_config_if_rsvp,
     imi_show_running_config_if_rsvp_cli,
     "show running-config interface IFNAME rsvp",
     "Show running system information",
     "Current Operating configuration",
     "Interface configuration",
     CLI_IFNAME_STR,
     CLI_RSVP_STR)
{
  imi_config_write_if (cli, argv[0], PM_RSVP);
  return CLI_SUCCESS;
}
#endif /* HAVE_RSVPD */

#ifdef HAVE_PDMD
CLI (imi_show_running_config_if_ip_pim_dense_mode,
     imi_show_running_config_if_ip_pim_dense_mode_cli,
     "show running-config interface IFNAME ip pim dense-mode",
     "Show running system information",
     "Current Operating configuration",
     "Interface configuration",
     CLI_IFNAME_STR,
     CLI_IP_STR,
     CLI_PIM_STR,
     CLI_PIMDM_STR)
{
  imi_config_write_if (cli, argv[0], PM_PDM);
  return CLI_SUCCESS;
}
#endif /* HAVE_PIMD */

#ifdef HAVE_PIMD
CLI (imi_show_running_config_if_ip_pim_sparse_mode,
     imi_show_running_config_if_ip_pim_sparse_mode_cli,
     "show running-config interface IFNAME ip pim sparse-mode",
     "Show running system information",
     "Current Operating configuration",
     "Interface configuration",
     CLI_IFNAME_STR,
     CLI_IP_STR,
     CLI_PIM_STR,
     CLI_PIMSM_STR)
{
  imi_config_write_if (cli, argv[0], PM_PIM);
  return CLI_SUCCESS;
}
#endif /* HAVE_PIMD */

#ifdef HAVE_IPV6
#ifdef HAVE_PDMD
CLI (imi_show_running_config_if_ipv6_pim_dense_mode,
     imi_show_running_config_if_ipv6_pim_dense_mode_cli,
     "show running-config interface IFNAME ipv6 pim dense-mode",
     "Show running system information",
     "Current Operating configuration",
     "Interface configuration",
     CLI_IFNAME_STR,
     CLI_IPV6_STR,
     CLI_PIM6_STR,
     CLI_PIMDM6_STR)
{
  imi_config_write_if (cli, argv[0] , PM_PDM);
  return CLI_SUCCESS;
}
#endif /* HAVE_PDMD */

#ifdef HAVE_PIM6D
CLI (imi_show_running_config_if_ipv6_pim_sparse_mode,
     imi_show_running_config_if_ipv6_pim_sparse_mode_cli,
     "show running-config interface IFNAME ipv6 pim sparse-mode",
     "Show running system information",
     "Current Operating configuration",
     "Interface configuration",
     CLI_IFNAME_STR,
     CLI_IPV6_STR,
     CLI_PIM6_STR,
     CLI_PIMSM6_STR)
{
  imi_config_write_if (cli, argv[0], PM_PIM6);
  return CLI_SUCCESS;
}
#endif /* HAVE_PIM6D */
#endif /* HAVE_IPV6 */

#ifdef HAVE_DVMRPD
CLI (imi_show_running_config_if_ip_dvmrp,
     imi_show_running_config_if_ip_dvmrp_cli,
     "show running-config interface IFNAME ip dvmrp",
     "Show running system information",
     "Current Operating configuration",
     "Interface configuration",
     CLI_IFNAME_STR,
     CLI_IP_STR,
     CLI_DVMRP_STR)
{
  imi_config_write_if (cli, argv[0], PM_DVMRP);
  return CLI_SUCCESS;
}
#endif /* HAVE_DVMRPD */

#if defined HAVE_MCAST_IPV4 || defined HAVE_IGMP_SNOOP
CLI (imi_show_running_config_if_ip_igmp,
     imi_show_running_config_if_ip_igmp_cli,
     "show running-config interface IFNAME ip igmp",
     "Show running system information",
     "Current Operating configuration",
     "Interface configuration",
     CLI_IFNAME_STR,
     CLI_IP_STR,
     CLI_IGMP_STR)
{
  imi_config_write_if_nsm_config (cli, argv[0], PM_NSM, IGMP_IF_MODE);
  return CLI_SUCCESS;
}
#endif /* HAVE_MCAST_IPV4 || HAVE_IGMP_SNOOP */

#ifdef HAVE_MCAST_IPV4
CLI (imi_show_running_config_if_ip_multicast,
     imi_show_running_config_if_ip_multicast_cli,
     "show running-config interface IFNAME ip multicast",
     "Show running system information",
     "Current Operating configuration",
     "Interface configuration",
     CLI_IFNAME_STR,
     CLI_IP_STR,
     "Global IP multicast commands")
{
  imi_config_write_if_nsm_config (cli, argv[0], PM_NSM, NSM_MCAST_MODE);
  return CLI_SUCCESS;
}
#endif /* HAVE_MCAST_IPV4 */

#ifdef HAVE_AUTHD
CLI (imi_show_running_config_if_dot1x,
     imi_show_running_config_if_dot1x_cli,
     "show running-config interface IFNAME dot1x",
     "Show running system information",
     "Current Operating configuration",
     "Interface configuration",
     CLI_IFNAME_STR,
     CLI_8021X_STR)
{
  imi_config_write_if (cli, argv[0], PM_AUTH);
  return CLI_SUCCESS;
}
#endif /* HAVE_AUTHD */

CLI (imi_show_running_config_if_onmd,
     imi_show_running_config_if_onmd_cli,
     "show running-config interface IFNAME onmd",
     "Show running system information",
     "Current Operating configuration",
     "Interface configuration",
     CLI_IFNAME_STR,
     CLI_ONMD_STR)
{
  imi_config_write_if (cli, argv[0], PM_ONM);
  return CLI_SUCCESS;
}

#ifdef HAVE_LACPD
CLI (imi_show_running_config_if_lacp,
     imi_show_running_config_if_lacp_cli,
     "show running-config interface IFNAME lacp",
     "Show running system information",
     "Current Operating configuration",
     "Interface configuration",
     CLI_IFNAME_STR,
     CLI_LACP_STR)
{
  imi_config_write_if (cli, argv[0], PM_LACP);
  return CLI_SUCCESS;
}
#endif /* HAVE_LACPD */

#ifdef HAVE_STPD
CLI (imi_show_running_config_if_stp,
     imi_show_running_config_if_stp_cli,
     "show running-config interface IFNAME stp",
     "Show running system information",
     "Current Operating configuration",
     "Interface configuration",
     CLI_IFNAME_STR,
     CLI_STP_STR)
{
  imi_config_write_if (cli, argv[0], PM_STP);
  return CLI_SUCCESS;
}
#endif /* HAVE_STPD */

#ifdef HAVE_RSTPD
CLI (imi_show_running_config_if_rstp,
     imi_show_running_config_if_rstp_cli,
     "show running-config interface IFNAME rstp",
     "Show running system information",
     "Current Operating configuration",
     "Interface configuration",
     CLI_IFNAME_STR,
     CLI_RSTP_STR)
{
  imi_config_write_if (cli, argv[0], PM_RSTP);
  return CLI_SUCCESS;
}
#endif /* HAVE_RSTPD */

#ifdef HAVE_MSTPD
CLI (imi_show_running_config_if_mstp,
     imi_show_running_config_if_mstp_cli,
     "show running-config interface IFNAME mstp",
     "Show running system information",
     "Current Operating configuration",
     "Interface configuration",
     CLI_IFNAME_STR,
     CLI_MSTP_STR)
{
  imi_config_write_if (cli, argv[0], PM_MSTP);
  return CLI_SUCCESS;
}
#endif /* HAVE_MSTPD */

#ifdef HAVE_L2
CLI (imi_show_running_config_if_bridge,
     imi_show_running_config_if_bridge_cli,
     "show running-config interface IFNAME bridge",
     "Show running system information",
     "Current Operating configuration",
     "Interface configuration",
     CLI_IFNAME_STR,
     CLI_SHOW_BRIDGE_STR)
{
  imi_config_write_if_nsm_config (cli, argv[0], PM_NSM, BRIDGE_MODE);
  return CLI_SUCCESS;
}

#ifdef HAVE_ELMID
CLI (imi_show_running_config_if_elmi,
    imi_show_running_config_if_elmi_cli,
    "show running-config interface IFNAME elmi",
    "Show running system information",
    "Current Operating configuration",
    "Interface configuration",
    CLI_IFNAME_STR,
    CLI_STP_STR)
{
    imi_config_write_if (cli, argv[0], PM_ELMI);
    return CLI_SUCCESS;
}
#endif /* HAVE_ELMID */

#endif /* HAVE_L2 */

#ifdef HAVE_GMPLS
CLI (imi_show_running_config_te_link_all,
     imi_show_running_config_te_link_all_cli,
     "show running-config te-link",
     "Show running system information",
     "Current Operating configuration",
     "TE Link configuration")
{
  imi_config_write_pm (cli, PM_ALL, TELINK_MODE);
  return CLI_SUCCESS;
}

CLI (imi_show_running_config_cc_all,
     imi_show_running_config_cc_all_cli,
     "show running-config control-channel",
     "Show running system information",
     "Current Operating configuration",
     "Control Channel configuration")
{
  imi_config_write_pm (cli, PM_ALL, CTRL_CHNL_MODE);
  return CLI_SUCCESS;
}

CLI (imi_show_running_config_ca_all,
     imi_show_running_config_ca_all_cli,
     "show running-config control-adjacency",
     "Show running system information",
     "Current Operating configuration",
     "Control Adjacency configuration")
{
  imi_config_write_pm (cli, PM_ALL, CTRL_ADJ_MODE);
  return CLI_SUCCESS;
}

CLI (imi_show_running_config_te_link,
     imi_show_running_config_te_link_cli,
     "show running-config te-link TLNAME",
     "Show running system information",
     "Current Operating configuration",
     "TE Link configuration",
     CLI_TELINK_NAME_STR)
{
  imi_config_write_te_link (cli, argv[0], PM_ALL);
  return CLI_SUCCESS;
}

CLI (imi_show_running_config_cc,
     imi_show_running_config_cc_cli,
     "show running-config control-channel CCNAME",
     "Show running system information",
     "Current Operating configuration",
     "Control Channel configuration",
     CLI_CCNAME_STR)
{
  imi_config_write_cc (cli, argv[0], PM_ALL);
  return CLI_SUCCESS;
}

CLI (imi_show_running_config_ca,
     imi_show_running_config_ca_cli,
     "show running-config control-adjacency CANAME",
     "Show running system information",
     "Current Operating configuration",
     "Control Adjacency configuration",
     CLI_CANAME_STR)
{
  imi_config_write_ca (cli, argv[0], PM_ALL);
  return CLI_SUCCESS;
}
#endif /*HAVE_GMPLS */

#ifdef HAVE_LMP
CLI (imi_show_runnng_config_lmp,
     imi_show_runnng_config_lmp_cli,
     "show running-config lmp-configuration",
     "show running system information",
     "Current Operating configuration")
{
  imi_config_write_lmp (cli, PM_ALL);
  return CLI_SUCCESS;
}
#endif /*HAVE_LMP */

void
imi_config_interface_init (struct cli_tree *ctree)
{
  cli_install_gen (ctree, EXEC_MODE, PRIVILEGE_VR_MAX, 0,
                   &imi_show_running_config_if_cli);
#ifdef HAVE_RIPD
  cli_install_gen (ctree, EXEC_MODE,PRIVILEGE_VR_MAX, 0,
                   &imi_show_running_config_if_rip_cli);
#endif /* HAVE_RIPD */
#ifdef HAVE_OSPFD
  cli_install_gen (ctree, EXEC_MODE, PRIVILEGE_VR_MAX, 0,
                   &imi_show_running_config_if_ospf_cli);
#endif /* HAVE_OSPF */
#ifdef HAVE_ISISD
  cli_install_gen (ctree, EXEC_MODE, PRIVILEGE_VR_MAX, 0,
                   &imi_show_running_config_if_isis_cli);
#endif /* HAVE_ISISD */
#ifdef HAVE_IPV6
#ifdef HAVE_RIPNGD
  cli_install_gen (ctree, EXEC_MODE, PRIVILEGE_VR_MAX, 0,
                   &imi_show_running_config_if_ipv6_rip_cli);
#endif /* HAVE_RIPNGD */
#ifdef HAVE_OSPF6D
  cli_install_gen (ctree, EXEC_MODE, PRIVILEGE_VR_MAX, 0,
                   &imi_show_running_config_if_ipv6_ospf_cli);
#endif /* HAVE_OSPF6D */
#endif /* HAVE_IPV6 */
#ifdef HAVE_MPLS
  cli_install_gen (ctree, EXEC_MODE, PRIVILEGE_VR_MAX, 0,
                   &imi_show_running_config_if_mpls_cli);
#endif /* HAVE_MPLS */
#ifdef HAVE_LDPD
  cli_install_gen (ctree, EXEC_MODE, PRIVILEGE_VR_MAX, 0,
                   &imi_show_running_config_if_ldp_cli);
#endif /* HAVE_LDPD */
#ifdef HAVE_RSVPD
  cli_install_gen (ctree, EXEC_MODE, PRIVILEGE_VR_MAX, 0,
                   &imi_show_running_config_if_rsvp_cli);
#endif /* HAVE_RSVPD */
#ifdef HAVE_PDMD
  cli_install_gen (ctree, EXEC_MODE, PRIVILEGE_VR_MAX, 0,
                   &imi_show_running_config_if_ip_pim_dense_mode_cli);
#endif /* HAVE_PDMD */
#ifdef HAVE_PIMD
  cli_install_gen (ctree, EXEC_MODE, PRIVILEGE_VR_MAX, 0,
                   &imi_show_running_config_if_ip_pim_sparse_mode_cli);
#endif /* HAVE_PIMD */
#ifdef HAVE_IPV6
#ifdef HAVE_PDMD
  cli_install_gen (ctree, EXEC_MODE, PRIVILEGE_VR_MAX, 0,
                   &imi_show_running_config_if_ipv6_pim_dense_mode_cli);
#endif /* HAVE_PDMD */
#ifdef HAVE_PIM6D
  cli_install_gen (ctree, EXEC_MODE, PRIVILEGE_VR_MAX, 0,
                   &imi_show_running_config_if_ipv6_pim_sparse_mode_cli);
#endif /* HAVE_PIM6D */
#endif /* HAVE_IPV6 */
#ifdef HAVE_DVMRPD
  cli_install_gen (ctree, EXEC_MODE, PRIVILEGE_VR_MAX, 0,
                   &imi_show_running_config_if_ip_dvmrp_cli);
#endif /* HAVE_DVMRPD */
#ifdef HAVE_MCAST_IPV4
  cli_install_gen (ctree, EXEC_MODE, PRIVILEGE_VR_MAX, 0,
                   &imi_show_running_config_if_ip_multicast_cli);
#endif /* HAVE_MCAST_IPV4 */
#ifdef HAVE_AUTHD
  cli_install_gen (ctree, EXEC_MODE, PRIVILEGE_VR_MAX, 0,
                   &imi_show_running_config_if_dot1x_cli);
#endif /* HAVE_AUTHD */
#ifdef HAVE_LACPD
  cli_install_gen (ctree, EXEC_MODE, PRIVILEGE_VR_MAX, 0,
                   &imi_show_running_config_if_lacp_cli);
#endif /* HAVE_LACPD */
#ifdef HAVE_STPD
  cli_install_gen (ctree, EXEC_MODE, PRIVILEGE_VR_MAX, 0,
                   &imi_show_running_config_if_stp_cli);
#endif /* HAVE_STPD */
#ifdef HAVE_RSTPD
  cli_install_gen (ctree, EXEC_MODE, PRIVILEGE_VR_MAX, 0,
                   &imi_show_running_config_if_rstp_cli);
#endif /* HAVE_RSTPD */
#ifdef HAVE_MSTPD
  cli_install_gen (ctree, EXEC_MODE, PRIVILEGE_VR_MAX, 0,
                   &imi_show_running_config_if_mstp_cli);
#endif /* HAVE_MSTPD */
#ifdef HAVE_ONMD
 cli_install_gen (ctree, EXEC_MODE, PRIVILEGE_VR_MAX, 0,
                  &imi_show_running_config_if_onmd_cli);
#endif /* HAVE_ONMD */
#ifdef HAVE_L2
  cli_install_gen (ctree, EXEC_MODE, PRIVILEGE_VR_MAX, 0,
                   &imi_show_running_config_if_bridge_cli);
#endif /* HAVE_L2 */
#ifdef HAVE_ELMID
  cli_install_gen (ctree, EXEC_MODE, PRIVILEGE_VR_MAX, 0,
                   &imi_show_running_config_if_elmi_cli);
#endif /* HAVE_ELMID */
#if defined HAVE_MCAST_IPV4 || defined HAVE_IGMP_SNOOP
  cli_install_gen (ctree, EXEC_MODE, PRIVILEGE_VR_MAX, 0,
                  &imi_show_running_config_if_ip_igmp_cli);
#endif /* HAVE_MCAST_IPV4 || HAVE_IGMP_SNOOP */

#ifdef HAVE_GMPLS
  cli_install_gen (ctree, EXEC_MODE, PRIVILEGE_VR_MAX, 0,
                   &imi_show_running_config_te_link_all_cli);
  cli_install_gen (ctree, EXEC_MODE, PRIVILEGE_VR_MAX, 0,
                   &imi_show_running_config_cc_all_cli);
  cli_install_gen (ctree, EXEC_MODE, PRIVILEGE_VR_MAX, 0,
                   &imi_show_running_config_ca_all_cli);
  cli_install_gen (ctree, EXEC_MODE, PRIVILEGE_VR_MAX, 0,
                   &imi_show_running_config_te_link_cli);
  cli_install_gen (ctree, EXEC_MODE, PRIVILEGE_VR_MAX, 0,
                   &imi_show_running_config_cc_cli);
  cli_install_gen (ctree, EXEC_MODE, PRIVILEGE_VR_MAX, 0,
                   &imi_show_running_config_ca_cli);
#endif /*HAVE_GMPLS */
#ifdef HAVE_LMP
  cli_install_gen (ctree, EXEC_MODE, PRIVILEGE_VR_MAX, 0,
                   &imi_show_runnng_config_lmp_cli);
#endif /* HAVE_LMP */
}

/* Show static route Configuration */
CLI (imi_show_running_config_ip_route,
     imi_show_running_config_ip_route_cli,
     "show running-config ip route",
     CLI_SHOW_STR,
     "Current Operating configuration",
     CLI_IP_STR,
     "Static IP route")
{
  imi_config_write_pm (cli, PM_NSM, IP_MODE);
  return CLI_SUCCESS;
}

#ifdef HAVE_BFD
/* Show static route Configuration */
CLI (imi_show_running_config_ip_bfd_route,
     imi_show_running_config_ip_bfd_route_cli,
     "show running-config ip static bfd",
     CLI_SHOW_STR,
     "Current Operating configuration",
     CLI_IP_STR,
     "Static IP route",
     CLI_BFD_STRING)
{
  imi_config_write_pm (cli, PM_NSM, IP_BFD_MODE);
  return CLI_SUCCESS;
}
#endif /* HAVE_BFD */

#ifdef HAVE_IPV6
CLI (imi_show_running_config_ipv6_route,
     imi_show_running_config_ipv6_route_cli,
     "show running-config ipv6 route",
     CLI_SHOW_STR,
     "Current Operating configuration",
     CLI_IPV6_STR,
     "Static IP route")
{
  imi_config_write_pm (cli, PM_NSM, IPV6_MODE);
  return CLI_SUCCESS;
}
#endif /* HAVE_IPV6 */

void
imi_config_static_route_init (struct cli_tree *ctree)
{
  cli_install_gen (ctree, EXEC_MODE, PRIVILEGE_VR_MAX, 0,
                   &imi_show_running_config_ip_route_cli);
#ifdef HAVE_IPV6
  cli_install_gen (ctree, EXEC_MODE, PRIVILEGE_VR_MAX, 0,
                   &imi_show_running_config_ipv6_route_cli);
#endif /* HAVE_IPV6 */
}

#ifdef HAVE_BFD
void
imi_config_bfd_static_route_init (struct cli_tree *ctree)
{
  cli_install_gen (ctree, EXEC_MODE, PRIVILEGE_VR_MAX, 0,
                   &imi_show_running_config_ip_bfd_route_cli);
}
#endif /* HAVE_BFD */

/* Show static route Configuration */
CLI (imi_show_running_config_ip_mroute,
     imi_show_running_config_ip_mroute_cli,
     "show running-config ip mroute",
     CLI_SHOW_STR,
     "Current Operating configuration",
     CLI_IP_STR,
     "Static IP multicast route")
{
  imi_config_write_pm (cli, PM_NSM, IP_MROUTE_MODE);
  return CLI_SUCCESS;
}

#ifdef HAVE_IPV6
CLI (imi_show_running_config_ipv6_mroute,
     imi_show_running_config_ipv6_mroute_cli,
     "show running-config ipv6 mroute",
     CLI_SHOW_STR,
     "Current Operating configuration",
     CLI_IPV6_STR,
     "Static IPv6 multicast route")
{
  imi_config_write_pm (cli, PM_NSM, IPV6_MROUTE_MODE);
  return CLI_SUCCESS;
}
#endif /* HAVE_IPV6 */

void
imi_config_static_mroute_init (struct cli_tree *ctree)
{
  cli_install_gen (ctree, EXEC_MODE, PRIVILEGE_VR_MAX, 0,
                   &imi_show_running_config_ip_mroute_cli);
#ifdef HAVE_IPV6
  cli_install_gen (ctree, EXEC_MODE, PRIVILEGE_VR_MAX, 0,
                   &imi_show_running_config_ipv6_mroute_cli);
#endif /* HAVE_IPV6 */
}

/* Show global mode router-id configuration */
CLI (imi_show_running_config_router_id,
     imi_show_running_config_router_id_cli,
     "show running-config router-id",
     CLI_SHOW_STR,
     "Current Operating configuration",
     "Router identifier for this system")
{
  imi_config_write_pm (cli, PM_NSM, CONFIG_MODE);
  return CLI_SUCCESS;
}

/* Show key-chain mode configuration */
CLI (imi_show_running_config_key_chain,
     imi_show_running_config_key_chain_cli,
     "show running-config key chain",
     CLI_SHOW_STR,
     "Current Operating configuration",
     CLI_KEY_STR,
     CLI_CHAIN_STR)
{
  imi_config_write_keychain (cli);
  return CLI_SUCCESS;
}

#ifdef HAVE_LACPD
CLI (imi_show_running_config_switch_lacp,
     imi_show_running_config_switch_lacp_cli,
     "show running-config switch lacp",
     "Show running system information",
     "Current Operating configuration",
     "Switch configuration",
     CLI_LACP_STR)
{
  imi_config_write_pm (cli, PM_LACP, EXEC_MODE);
  return CLI_SUCCESS;
}
#endif /* HAVE_LACPD */

#ifdef HAVE_ELMID
CLI (imi_show_running_config_switch_elmi,
    imi_show_running_config_switch_elmi_cli,
    "show running-config switch lmi",
    "Show running system information",
    "Current Operating configuration",
    "Switch configuration",
    CLI_ELMI_STR)
{
    imi_config_write_pm (cli, PM_ELMI, EXEC_MODE);
    return CLI_SUCCESS;
}
#endif /* HAVE_ELMID */

#if defined HAVE_MCAST_IPV4 || defined HAVE_IGMP_SNOOP
CLI (imi_show_running_config_ip_igmp,
     imi_show_running_config_ip_igmp_cli,
     "show running-config ip igmp",
     "Show running system information",
     "Current Operating configuration",
     CLI_IP_STR,
     CLI_IGMP_STR)
{
  imi_config_write_pm (cli, PM_NSM, IGMP_MODE);
  return CLI_SUCCESS;
}
#endif /* HAVE_MCAST_IPV4 || HAVE_IGMP_SNOOP */

void
imi_config_keychain_init (struct cli_tree *ctree)
{
  cli_install_gen (ctree, EXEC_MODE, PRIVILEGE_VR_MAX, 0,
                   &imi_show_running_config_key_chain_cli);
}


/* Protocol module configuration.  */
#ifdef HAVE_RIPD
CLI (imi_show_running_config_rip,
     imi_show_running_config_rip_cli,
     "show running-config router rip",
     "Show running system information",
     "Current Operating configuration",
     "Router configuration",
     CLI_RIP_STR)
{
  imi_config_write_pm (cli, PM_RIP, RIP_MODE);
  return CLI_SUCCESS;
}
#endif /* HAVE_RIPD */

#ifdef HAVE_OSPFD
CLI (imi_show_running_config_ospf,
     imi_show_running_config_ospf_cli,
     "show running-config router ospf",
     "Show running system information",
     "Current Operating configuration",
     "Router configuration",
     CLI_OSPF_STR)
{
  imi_config_write_pm (cli, PM_OSPF, OSPF_MODE);
  return CLI_SUCCESS;
}
#endif /* HAVE_OSPFD */

#ifdef HAVE_ISISD
CLI (imi_show_running_config_isis,
     imi_show_running_config_isis_cli,
     "show running-config router isis",
     "Show running system information",
     "Current Operating configuration",
     "Router configuration",
     CLI_ISIS_STR)
{
  imi_config_write_pm (cli, PM_ISIS, ISIS_MODE);
  return CLI_SUCCESS;
}
#endif /* HAVE_ISISD */

#ifdef HAVE_BGPD
CLI (imi_show_running_config_virtual_router_bgp,
     imi_show_running_config_virtual_router_bgp_cli,
     "show running-config bgp",
     "Show running system information",
     "Current Operating configuration",
     CLI_BGP_STR)
{
  imi_config_write_bgp_vr (cli);
  return CLI_SUCCESS;
}

CLI (imi_show_running_config_bgp,
     imi_show_running_config_bgp_cli,
     "show running-config router bgp",
     "Show running system information",
     "Current Operating configuration",
     "Router configuration",
     CLI_BGP_STR)
{
  imi_config_write_pm (cli, PM_BGP, BGP_MODE);
  return CLI_SUCCESS;
}
#endif /* HAVE_BGPD */

#ifdef HAVE_IPV6
#ifdef HAVE_RIPNGD
CLI (imi_show_running_config_ripng,
     imi_show_running_config_ripng_cli,
     "show running-config router ipv6 rip",
     "Show running system information",
     "Current Operating configuration",
     "Router configuration",
     CLI_IPV6_STR,
     CLI_RIPNG_STR)
{
  imi_config_write_pm (cli, PM_RIPNG, RIPNG_MODE);
  return CLI_SUCCESS;
}
#endif /* HAVE_RIPNGD */

#ifdef HAVE_OSPF6D
CLI (imi_show_running_config_ospf6,
     imi_show_running_config_ospf6_cli,
     "show running-config router ipv6 ospf",
     "Show running system information",
     "Current Operating configuration",
     "Router configuration",
     CLI_IPV6_STR,
     CLI_OSPF6_STR)
{
  imi_config_write_pm (cli, PM_OSPF6, OSPF6_MODE);
  return CLI_SUCCESS;
}
#endif /* HAVE_OSPF6D */
#endif /* HAVE_IPV6 */

#ifdef HAVE_MPLS
CLI (imi_show_running_config_mpls,
     imi_show_running_config_mpls_cli,
     "show running-config mpls",
     "Show running system information",
     "Current Operating configuration",
     CLI_MPLS_STR)
{
  imi_config_write_pm (cli, PM_ALL, NSM_MPLS_MODE);
  imi_config_write_pm (cli, PM_ALL, NSM_MPLS_STATIC_MODE);
#ifdef HAVE_VPLS
  imi_config_write_pm (cli, PM_ALL, NSM_VPLS_MODE);
#endif /* HAVE_VPLS */
  return CLI_SUCCESS;
}
#endif /* HAVE_MPLS */

#ifdef HAVE_LDPD
CLI (imi_show_running_config_ldp,
     imi_show_running_config_ldp_cli,
     "show running-config router ldp",
     "Show running system information",
     "Current Operating configuration",
     "Router configuration",
     CLI_LDP_STR)
{
  imi_config_write_pm (cli, PM_LDP, LDP_MODE);
  return CLI_SUCCESS;
}
#endif /* HAVE_LDPD */

#ifdef HAVE_RSVPD
CLI (imi_show_running_config_rsvp,
     imi_show_running_config_rsvp_cli,
     "show running-config router rsvp",
     "Show running system information",
     "Current Operating configuration",
     "Router configuration",
     CLI_RSVP_STR)
{
  imi_config_write_pm (cli, PM_RSVP, RSVP_MODE);
  return CLI_SUCCESS;
}

CLI (imi_show_running_config_rsvp_path,
     imi_show_running_config_rsvp_path_cli,
     "show running-config rsvp-path",
     "Show running system information",
     "Current Operating configuration",
     CLI_RSVP_STR)
{
  imi_config_write_pm (cli, PM_ALL, RSVP_PATH_MODE);
  return CLI_SUCCESS;
}

CLI (imi_show_running_config_rsvp_trunk,
     imi_show_running_config_rsvp_trunk_cli,
     "show running-config rsvp-trunk",
     "Show running system information",
     "Current Operating configuration",
     CLI_RSVP_STR)
{
  imi_config_write_pm (cli, PM_ALL, RSVP_TRUNK_MODE);
  return CLI_SUCCESS;
}

#ifdef HAVE_MPLS_FRR
CLI (imi_show_running_config_rsvp_bypass,
     imi_show_running_config_rsvp_bypass_cli,
     "show running-config rsvp-bypass",
     "Show running system information",
     "Current Operating configuration",
     CLI_RSVP_STR)
{
  imi_config_write_pm (cli, PM_ALL, RSVP_BYPASS_MODE);
  return CLI_SUCCESS;
}
#endif /* HAVE_MPLS_FRR */
#endif /* HAVE_RSVPD */

#ifdef HAVE_VRRP
CLI (imi_show_running_config_vrrp,
     imi_show_running_config_vrrp_cli,
     "show running-config router vrrp",
     "Show running system information",
     "Current Operating configuration",
     "Router configuration",
     CLI_VRRP_STR)
{
  imi_config_write_pm (cli, PM_NSM, VRRP_MODE);
  return CLI_SUCCESS;
}
#ifdef HAVE_IPV6
CLI (imi_show_running_config_vrrp_ipv6,
     imi_show_running_config_vrrp_ipv6_cli,
     "show running-config router ipv6 vrrp",
     "Show running system information",
     "Current Operating configuration",
     "IPV6 Router configuration",
     CLI_IPV6_STR,
     CLI_VRRP_STR)
{
  imi_config_write_pm (cli, PM_NSM, VRRP6_MODE);
  return CLI_SUCCESS;
}
#endif /* HAVE_IPV6 */
#endif /* HAVE_VRRP */

void
imi_config_router_init (struct cli_tree *ctree)
{
#ifdef HAVE_RIPD
  cli_install_gen (ctree, EXEC_MODE, PRIVILEGE_VR_MAX, 0,
                   &imi_show_running_config_rip_cli);
#endif /* HAVE_RIPD */
#ifdef HAVE_OSPFD
  cli_install_gen (ctree, EXEC_MODE, PRIVILEGE_VR_MAX, 0,
                   &imi_show_running_config_ospf_cli);
#endif /* HAVE_OSPFD */
#ifdef HAVE_ISISD
  cli_install_gen (ctree, EXEC_MODE, PRIVILEGE_VR_MAX, 0,
                   &imi_show_running_config_isis_cli);
#endif /* HAVE_ISISD */
#ifdef HAVE_BGPD
  cli_install_gen (ctree, EXEC_MODE, PRIVILEGE_VR_MAX, 0,
                   &imi_show_running_config_virtual_router_bgp_cli);
  cli_install_gen (ctree, EXEC_MODE, PRIVILEGE_VR_MAX, 0,
                   &imi_show_running_config_bgp_cli);
#endif /* HAVE_BGPD */
#ifdef HAVE_IPV6
#ifdef HAVE_RIPNGD
  cli_install_gen (ctree, EXEC_MODE, PRIVILEGE_VR_MAX, 0,
                   &imi_show_running_config_ripng_cli);
#endif /* HAVE_RIPNGD */
#ifdef HAVE_OSPF6D
  cli_install_gen (ctree, EXEC_MODE, PRIVILEGE_VR_MAX, 0,
                   &imi_show_running_config_ospf6_cli);
#endif /* HAVE_OSPF6D */
#endif /* HAVE_IPV6 */
#ifdef HAVE_MPLS
  cli_install_gen (ctree, EXEC_MODE, PRIVILEGE_VR_MAX, 0,
                   &imi_show_running_config_mpls_cli);
#endif /* HAVE_MPLS */
#ifdef HAVE_LDPD
  cli_install_gen (ctree, EXEC_MODE, PRIVILEGE_VR_MAX, 0,
                   &imi_show_running_config_ldp_cli);
#endif /* HAVE_LDPD */
#ifdef HAVE_RSVPD
  cli_install_gen (ctree, EXEC_MODE, PRIVILEGE_VR_MAX, 0,
                   &imi_show_running_config_rsvp_cli);
  cli_install_gen (ctree, EXEC_MODE, PRIVILEGE_VR_MAX, 0,
                   &imi_show_running_config_rsvp_path_cli);
  cli_install_gen (ctree, EXEC_MODE, PRIVILEGE_VR_MAX, 0,
                   &imi_show_running_config_rsvp_trunk_cli);
#endif /* HAVE_RSVPD */
#ifdef HAVE_VRRP
  cli_install_gen (ctree, EXEC_MODE, PRIVILEGE_VR_MAX, 0,
                   &imi_show_running_config_vrrp_cli);
#ifdef HAVE_IPV6
  cli_install_gen (ctree, EXEC_MODE, PRIVILEGE_VR_MAX, 0,
                   &imi_show_running_config_vrrp_ipv6_cli);
#endif /* HAVE_IPV6 */
#endif /* HAVE_VRRP */
}


#ifdef HAVE_PDMD
CLI (imi_show_running_config_ip_pim_dense_mode,
     imi_show_running_config_ip_pim_dense_mode_cli,
     "show running-config ip pim dense-mode",
     "Show running system information",
     "Current Operating configuration",
     CLI_IP_STR,
     CLI_PIM_STR,
     CLI_PIMDM_STR)
{
  imi_config_write_pm (cli, PM_PDM, PDM_MODE);
  return CLI_SUCCESS;
}
#endif /* HAVE_PDMD */

#ifdef HAVE_PIMD
CLI (imi_show_running_config_ip_pim_sparse_mode,
     imi_show_running_config_ip_pim_sparse_mode_cli,
     "show running-config ip pim sparse-mode",
     "Show running system information",
     "Current Operating configuration",
     CLI_IP_STR,
     CLI_PIM_STR,
     CLI_PIMSM_STR)
{
  imi_config_write_pm (cli, PM_PIM, PIM_MODE);
  return CLI_SUCCESS;
}
#endif /* HAVE_PIMD */

#ifdef HAVE_BFD
CLI (imi_show_running_config_bfd,
     imi_show_running_config_bfd_cli,
     "show running-config bfd",
     "Show running system information",
     "Current Operating configuration",
     CLI_IP_STR,
     CLI_BFD_STRING)
{
  imi_config_write_pm (cli, PM_OAM, BFD_MODE);
  return CLI_SUCCESS;
}
#endif /* HAVE_BFD */

#ifdef HAVE_IPV6
#ifdef HAVE_PDMD
CLI (imi_show_running_config_ipv6_pim_dense_mode,
     imi_show_running_config_ipv6_pim_dense_mode_cli,
     "show running-config ipv6 pim dense-mode",
     "Show running system information",
     "Current Operating configuration",
      CLI_IPV6_STR,
     CLI_PIM6_STR,
     CLI_PIMDM6_STR)
{
  imi_config_write_pm (cli, PM_PDM, PDM_MODE);
  return CLI_SUCCESS;
}
#endif /* HAVE_PDMD */

#ifdef HAVE_PIM6D
CLI (imi_show_running_config_ipv6_pim_sparse_mode,
     imi_show_running_config_ipv6_pim_sparse_mode_cli,
     "show running-config ipv6 pim sparse-mode",
     "Show running system information",
     "Current Operating configuration",
     CLI_IPV6_STR,
     CLI_PIM6_STR,
     CLI_PIMSM6_STR)
{
  imi_config_write_pm (cli, PM_PIM6, PIM6_MODE);
  return CLI_SUCCESS;
}
#endif /*HAVE_PIM6D*/
#endif /* HAVE_IPV6 */

#ifdef HAVE_MCAST_IPV4
/* Show ip multicast configuration */
CLI (imi_show_running_config_ip_multicast,
     imi_show_running_config_ip_multicast_cli,
     "show running-config ip multicast",
     "Show running system information",
     "Current Operating configuration",
     CLI_IP_STR,
     "Global IP multicast commands")
{
  imi_config_write_pm (cli, PM_NSM, NSM_MCAST_MODE);
  return CLI_SUCCESS;
}
#endif /* HAVE_MCAST_IPV4 */

void
imi_config_pim_init (struct cli_tree *ctree)
{
#ifdef HAVE_PDMD
 cli_install_gen(ctree, EXEC_MODE, PRIVILEGE_VR_MAX, 0,
                 &imi_show_running_config_ip_pim_dense_mode_cli);
#endif /* HAVE_PDMD */
#ifdef HAVE_PIMD
  cli_install_gen (ctree, EXEC_MODE, PRIVILEGE_VR_MAX, 0,
                   &imi_show_running_config_ip_pim_sparse_mode_cli);
#endif /* HAVE_PIMD */
#ifdef HAVE_IPV6
#ifdef HAVE_PDMD
  cli_install_gen (ctree, EXEC_MODE, PRIVILEGE_VR_MAX, 0,
                   &imi_show_running_config_ipv6_pim_dense_mode_cli);
#endif /* HAVE_PDMD */
#ifdef HAVE_PIM6D
 cli_install_gen (ctree, EXEC_MODE, PRIVILEGE_VR_MAX, 0,
                  &imi_show_running_config_ipv6_pim_sparse_mode_cli);
#endif /* HAVE_PIM6D */
#endif /* HAVE_IPV6 */
#ifdef HAVE_MCAST_IPV4
 cli_install_gen (ctree, EXEC_MODE, PRIVILEGE_VR_MAX, 0,
                  &imi_show_running_config_ip_multicast_cli);
#endif /* HAVE_MCAST */
#ifdef HAVE_LACPD
  cli_install_gen (ctree, EXEC_MODE, PRIVILEGE_VR_MAX, 0,
                   &imi_show_running_config_switch_lacp_cli);
#endif /* HAVE_LACPD */
#ifdef HAVE_ELMID
  cli_install_gen (ctree, EXEC_MODE, PRIVILEGE_VR_MAX, 0,
                   &imi_show_running_config_switch_elmi_cli);
#endif /* HAVE_ELMID */
#if defined HAVE_MCAST_IPV4 || defined HAVE_IGMP_SNOOP
  cli_install_gen (ctree, EXEC_MODE, PRIVILEGE_VR_MAX, 0,
                   &imi_show_running_config_ip_igmp_cli);
#endif /* HAVE_MCAST_IPV4 || HAVE_IGMP_SNOOP */
#ifdef HAVE_BFD
  cli_install_gen (ctree, EXEC_MODE, PRIVILEGE_VR_MAX, 0,
                   &imi_show_running_config_bfd_cli);
#endif /* HAVE_BFD */
}

#ifdef HAVE_L2
CLI (imi_show_running_config_switch_bridge,
     imi_show_running_config_switch_bridge_cli,
     "show running-config switch bridge",
     "Show running system information",
     "Current Operating configuration",
     "Switch configuration",
     "Bridge group commands")
{
   imi_config_write_pm (cli, PM_NSM, BRIDGE_MODE);
   return CLI_SUCCESS;
}
#endif /* HAVE_L2 */

#ifdef HAVE_AUTHD
CLI (imi_show_running_config_switch_dot1x,
     imi_show_running_config_switch_dot1x_cli,
     "show running-config switch dot1x",
     "Show running system information",
     "Current Operating configuration",
     "Switch configuration",
     CLI_8021X_STR)
{
  imi_config_write_pm (cli, PM_AUTH, EXEC_MODE);
  return CLI_SUCCESS;
}

CLI (imi_show_running_config_switch_radius_server,
     imi_show_running_config_switch_radius_server_cli,
     "show running-config switch radius-server",
     "Show running system information",
     "Current Operating configuration",
     "Switch configuration",
     "radius-server - radius server commands")
{
  imi_config_write_pm (cli, PM_AUTH, AUTH8021X_MODE);
  return CLI_SUCCESS;
}
#endif /* HAVE_AUTHD */

#ifdef HAVE_GVRP
CLI (imi_show_running_config_switch_gvrp,
     imi_show_running_config_switch_gvrp_cli,
     "show running-config switch gvrp",
     "Show running system information",
     "Current Operating configuration",
     "Switch configuration",
     CLI_GVRP_STR)
{
  imi_config_write_pm (cli, PM_NSM, GVRP_MODE);
  return CLI_SUCCESS;
}
#endif /* HAVE_GVRP */

#ifdef HAVE_GMRP
CLI (imi_show_running_config_switch_gmrp,
     imi_show_running_config_switch_gmrp_cli,
     "show running-config switch gmrp",
     "Show running system information",
     "Current Operating configuration",
     "Switch configuration",
     CLI_GMRP_STR)
{
  imi_config_write_pm (cli, PM_NSM, GMRP_MODE);
  return CLI_SUCCESS;
}
#endif /* HAVE_GMRP */

#ifdef HAVE_STPD
CLI (imi_show_running_config_switch_stp,
     imi_show_running_config_switch_stp_cli,
     "show running-config switch stp",
     "Show running system information",
     "Current Operating configuration",
     "Switch configuration",
     CLI_STP_STR)
{
  imi_config_write_pm (cli, PM_STP, EXEC_MODE);
  return CLI_SUCCESS;
}
#endif /* HAVE_STPD */

#ifdef HAVE_RSTPD
CLI (imi_show_running_config_switch_rstp,
     imi_show_running_config_switch_rstp_cli,
     "show running-config switch rstp",
     "Show running system information",
     "Current Operating configuration",
     "Switch configuration",
     CLI_RSTP_STR)
{
  imi_config_write_pm (cli, PM_RSTP, EXEC_MODE);
  return CLI_SUCCESS;
}
#endif /* HAVE_RSTPD */

#ifdef HAVE_MSTPD
CLI (imi_show_running_config_switch_mstp,
     imi_show_running_config_switch_mstp_cli,
     "show running-config switch mstp",
     "Show running system information",
     "Current Operating configuration",
     "Switch configuration",
     CLI_MSTP_STR)
{
  imi_config_write_pm (cli, PM_MSTP, EXEC_MODE);
  imi_config_write_pm (cli, PM_MSTP, MST_CONFIG_MODE);
  return CLI_SUCCESS;
}
#endif /* HAVE_MSTPD */

#if defined (HAVE_PROVIDER_BRIDGE) || defined (HAVE_B_BEB)
CLI (imi_show_running_config_switch_te_msti,
     imi_show_running_config_switch_te_msti_cli,
     "show running-config switch te_msti",
     "Show running system information",
     "Current Operating configuration",
     "Switch configuration",
     CLI_MSTP_STR)
{
  imi_config_write_pm (cli, PM_MSTP, EXEC_MODE);
  imi_config_write_pm (cli, PM_MSTP, TE_MSTI_CONFIG_MODE);
  return CLI_SUCCESS;
}
#endif /*(HAVE_PROVIDER_BRIDGE) || (HAVE_B_BEB) */

#ifdef HAVE_RPVST_PLUS
CLI (imi_show_running_config_switch_rpvst,
     imi_show_running_config_switch_rpvst_cli,
     "show running-config switch rpvst+",
     "Show running system information",
     "Current Operating configuration",
     "Switch configuration",
     CLI_MSTP_STR)
{
  imi_config_write_pm (cli, PM_MSTP, EXEC_MODE);
  imi_config_write_pm (cli, PM_MSTP, RPVST_PLUS_CONFIG_MODE);
  return CLI_SUCCESS;
}
#endif /* HAVE_RPVST_PLUS */

CLI (imi_show_running_config_switch_rmon,
     imi_show_running_config_switch_rmon_cli,
     "show running-config rmon",
     "Show running system information",
     "Current Operating configuration",
     CLI_RMON_STR)
{
  imi_config_write_pm (cli, PM_RMON, EXEC_MODE);
  return CLI_SUCCESS;
}

CLI (imi_show_running_config_switch_vlan,
     imi_show_running_config_switch_vlan_cli,
     "show running-config switch vlan",
     "Show running system information",
     "Current Operating configuration",
     "Switch configuration",
     CLI_VLAN_STR)
{
  imi_config_write_pm (cli, PM_ALL, VLAN_MODE);
  return CLI_SUCCESS;
}

void
imi_config_switch_init (struct cli_tree *ctree)
{
#ifdef HAVE_L2
  cli_install_gen (ctree, EXEC_MODE, PRIVILEGE_VR_MAX, 0,
                   &imi_show_running_config_switch_bridge_cli);
#endif /* HAVE_L2 */
#ifdef HAVE_AUTHD
 cli_install_gen (ctree, EXEC_MODE, PRIVILEGE_VR_MAX, 0,
                  &imi_show_running_config_switch_dot1x_cli);
 cli_install_gen (ctree, EXEC_MODE, PRIVILEGE_VR_MAX, 0,
                  &imi_show_running_config_switch_radius_server_cli);
#endif /* HAVE_AUTHD */
#ifdef HAVE_GVRP
 cli_install_gen (ctree, EXEC_MODE, PRIVILEGE_VR_MAX, 0,
                  &imi_show_running_config_switch_gvrp_cli);
#endif /* HAVE_GVRP */
#ifdef HAVE_GMRP
 cli_install_gen (ctree, EXEC_MODE, PRIVILEGE_VR_MAX, 0,
                  &imi_show_running_config_switch_gmrp_cli);
#endif /* HAVE_GMRP */
#ifdef HAVE_STPD
 cli_install_gen (ctree, EXEC_MODE, PRIVILEGE_VR_MAX, 0,
                  &imi_show_running_config_switch_stp_cli);
#endif /* HAVE_STPD */
#ifdef HAVE_RSTPD
 cli_install_gen (ctree, EXEC_MODE, PRIVILEGE_VR_MAX, 0,
                  &imi_show_running_config_switch_rstp_cli);
#endif /* HAVE_RSTPD */
#ifdef HAVE_MSTPD
 cli_install_gen (ctree, EXEC_MODE, PRIVILEGE_VR_MAX, 0,
                  &imi_show_running_config_switch_mstp_cli);
#endif /* HAVE_MSTPD */
#ifdef HAVE_RPVST_PLUS
  cli_install_gen (ctree, EXEC_MODE, PRIVILEGE_VR_MAX, 0,
                   &imi_show_running_config_switch_rpvst_cli);
#endif /* HAVE_RPVST_PLUS */

 cli_install_gen (ctree, EXEC_MODE, PRIVILEGE_VR_MAX, 0,
                  &imi_show_running_config_switch_rmon_cli);
 cli_install_gen (ctree, EXEC_MODE, PRIVILEGE_VR_MAX, 0,
                 &imi_show_running_config_switch_vlan_cli);
}

CLI (imi_show_running_config_access_list,
     imi_show_running_config_access_list_cli,
     "show running-config access-list",
     "Show running system information",
     "Current Operating configuration",
     "Access-list")
{
  imi_config_write_filter (cli, ACCESS_MODE);
  return CLI_SUCCESS;
}

CLI (imi_show_running_config_prefix_list,
     imi_show_running_config_prefix_list_cli,
     "show running-config prefix-list",
     "Show running system information",
     "Current Operating configuration",
     "Prefix-list")
{
  imi_config_write_filter (cli, PREFIX_MODE);
  return CLI_SUCCESS;
}

#ifdef HAVE_IPV6
CLI (imi_show_running_config_ipv6_access_list,
     imi_show_running_config_ipv6_access_list_cli,
     "show running-config ipv6 access-list",
     "Show running system information",
     "Current Operating configuration",
     CLI_IPV6_STR,
     "Access-list")
{
  imi_config_write_filter (cli, ACCESS_IPV6_MODE);
  return CLI_SUCCESS;
}

CLI (imi_show_running_config_ipv6_prefix_list,
     imi_show_running_config_ipv6_prefix_list_cli,
     "show running-config ipv6 prefix-list",
     "Show running system information",
     "Current Operating configuration",
     CLI_IPV6_STR,
     "Prefix-list")
{
  imi_config_write_filter (cli, PREFIX_IPV6_MODE);
  return CLI_SUCCESS;
}
#endif /* HAVE_IPV6 */

CLI (imi_show_running_config_as_path_access_list,
     imi_show_running_config_as_path_access_list_cli,
     "show running-config as-path access-list",
     "Show running system information",
     "Current Operating configuration",
     "Autonomous system path filter",
     "Access-list")
{
  imi_config_write_pm (cli, PM_BGP, AS_LIST_MODE);
  return CLI_SUCCESS;
}

CLI (imi_show_running_config_community_list,
     imi_show_running_config_community_list_cli,
     "show running-config community-list",
     "Show running system information",
     "Current Operating configuration",
     "Community-list")
{
  imi_config_write_pm (cli, PM_BGP, COMMUNITY_LIST_MODE);
  return CLI_SUCCESS;
}

CLI (imi_show_running_config_route_map,
     imi_show_running_config_route_map_cli,
     "show running-config route-map",
     "Show running system information",
     "Current Operating configuration",
     "Route-map")
{
  imi_config_write_filter (cli, RMAP_MODE);
  return CLI_SUCCESS;
}

void
imi_config_filter_init (struct cli_tree *ctree)
{
  cli_install_gen (ctree, EXEC_MODE, PRIVILEGE_VR_MAX, 0,
                   &imi_show_running_config_access_list_cli);
  cli_install_gen (ctree, EXEC_MODE, PRIVILEGE_VR_MAX, 0,
                   &imi_show_running_config_prefix_list_cli);
#ifdef HAVE_IPV6
  cli_install_gen (ctree, EXEC_MODE, PRIVILEGE_VR_MAX, 0,
                   &imi_show_running_config_ipv6_access_list_cli);
  cli_install_gen (ctree, EXEC_MODE, PRIVILEGE_VR_MAX, 0,
                   &imi_show_running_config_ipv6_prefix_list_cli);
#endif /* HAVE_IPV6 */
  cli_install_gen (ctree, EXEC_MODE, PRIVILEGE_VR_MAX, 0,
                   &imi_show_running_config_as_path_access_list_cli);
  cli_install_gen (ctree, EXEC_MODE, PRIVILEGE_VR_MAX, 0,
                   &imi_show_running_config_community_list_cli);
  cli_install_gen (ctree, EXEC_MODE, PRIVILEGE_VR_MAX, 0,
                   &imi_show_running_config_route_map_cli);
}


CLI (imi_write,
     imi_write_cli,
     "write (file|miniconfig)",
     CLI_WRITE_STR,
     "Write to file",
     "Write a miniconfig")
{
  int ret;
  int type;
  type=(0==pal_strcmp("file",argv[0]))?0:1;//11.2
  ret = imi_config_write (cli, cli->vr->host->config_file,type);//11.2
  if (ret < 0)
    {
      cli_out (cli, "%% Can't write configuration\n");
      return CLI_ERROR;
    }
  else
    cli_out (cli, "[OK]\n");

#ifdef HAVE_CONFIG_COMPARE
  imi_star_off ();
#endif /* HAVE_CONFIG_COMPARE */

  return CLI_SUCCESS;
}

ALI (imi_write,
     imi_write_memory_cli,
     "write memory",
     CLI_WRITE_STR,
     "Write to NV memory");

ALI (imi_write,
     imi_copy_runconfig_startconfig_cli,
     "copy running-config startup-config",
     "Copy from one file to another",
     "Copy from current system configuration",
     "Copy to startup configuration");


void
imi_config_init (struct lib_globals *zg)
{
  struct cli_tree *ctree = zg->ctree;
  int i;

  cli_install_gen (ctree, EXEC_MODE, PRIVILEGE_VR_MAX, 0,
                   &imi_show_running_config_cli);
  cli_install_gen (ctree, EXEC_MODE, PRIVILEGE_VR_MAX, 0,
                   &imi_show_running_config_full_cli);
  cli_install_gen (ctree, EXEC_MODE, PRIVILEGE_PVR_MAX, 0,
                   &imi_show_running_config_virtual_router_cli);
  cli_install_gen (ctree, EXEC_MODE, PRIVILEGE_VR_MAX, 0,
                   &imi_show_running_config_router_id_cli);
#ifdef HAVE_VRF
  cli_install_gen (ctree, EXEC_MODE, PRIVILEGE_VR_MAX, 0,
                   &imi_show_running_config_vrf_cli);
#endif /* HAVE_VRF */
  cli_install_gen (ctree, EXEC_MODE, PRIVILEGE_VR_MAX, 0,
                   &imi_write_terminal_cli);
  cli_install_gen (ctree, EXEC_MODE, PRIVILEGE_VR_MAX, 0,
                   &imi_write_cli);
  cli_install_gen (ctree, EXEC_MODE, PRIVILEGE_VR_MAX, 0,
                   &imi_write_memory_cli);
  cli_install_gen (ctree, EXEC_MODE, PRIVILEGE_VR_MAX, 0,
                   &imi_copy_runconfig_startconfig_cli);

  for (i = 0; i < MAX_MODE; i++)
    if (i != EXEC_MODE && i != EXEC_PRIV_MODE)
      cli_install_gen (ctree, i, PRIVILEGE_VR_MAX, CLI_FLAG_HIDDEN,
                       &imi_write_memory_cli);

  /* "show running-config interface" CLIs.  */
  imi_config_interface_init (ctree);

  /* "show running-config static route" CLIs. */
  imi_config_static_route_init (ctree);

#ifdef HAVE_BFD
  /* "show running-config static route" CLIs. */
  imi_config_bfd_static_route_init (ctree);
#endif /* HAVE_BFD */

  /* "show running-config static route" CLIs. */
  imi_config_static_mroute_init (ctree);

  /* "show running-config key chain" CLIs. */
  imi_config_keychain_init (ctree);

  /* "show running-config router" CLIs.  */
  imi_config_router_init (ctree);

  /* "show running-config ip/ipv6 pim" CLIs.  */
  imi_config_pim_init (ctree);

  /* "show running-config switch" CLIs.  */
  imi_config_switch_init (ctree);

  /* show running-config access-list/prefix-list/route-map CLI's  */
  imi_config_filter_init (ctree);

  /* For "s run" alias.  */
  cli_install_shortcut (ctree, EXEC_MODE, "*s=show", "s", "show");
}
