/* Copyright (C) 2002-2003  All Rights Reserved. */

#include "pal.h"
#include "lib.h"

#include "thread.h"
#include "log.h"
#include "cli.h"
#include "table.h"
#include "snprintf.h"
#ifdef HAVE_OSPF_CSPF
#include "cspf_common.h"
#endif /* HAVE_OSPF_CSPF */

#include "ospfd/ospfd.h"
#include "ospfd/ospf_interface.h"
#include "ospfd/ospf_vlink.h"
#ifdef HAVE_OSPF_MULTI_AREA
#include "ospfd/ospf_multi_area_link.h"
#endif /* HAVE_OSPF_MULTI_AREA */
#include "ospfd/ospf_packet.h"
#include "ospfd/ospf_neighbor.h"
#include "ospfd/ospf_ifsm.h"
#include "ospfd/ospf_nfsm.h"
#include "ospfd/ospf_lsa.h"
#include "ospfd/ospf_lsdb.h"
#include "ospfd/ospf_flood.h"
#include "ospfd/ospf_spf.h"
#include "ospfd/ospf_route.h"
#include "ospfd/ospf_nsm.h"
#include "ospfd/ospf_api.h"
#include "ospfd/ospf_show.h"
#include "ospfd/ospf_debug.h"
#include "ospfd/ospf_vrf.h"
#ifdef HAVE_RESTART
#include "ospfd/ospf_restart.h"
#endif /* HAVE_RESTART */
#ifdef HAVE_BFD
#include "ospfd/ospf_bfd_show.h"
#endif /* HAVE_BFD */
#ifdef HAVE_OSPF_TE
#include "ospfd/ospf_te.h"
#endif /* HAVE_OSPF_TE */
#ifdef HAVE_OSPF_CSPF
#include "ospfd/cspf/ospf_cspf.h"
#endif /* HAVE_OSPF_CSPF */
#ifdef HAVE_MPLS
#include "ospfd/ospf_mpls.h"
#endif /* HAVE_MPLS */

#define OSPF_CLI_OSPF_INFO_STR                                                \
    "OSPF information"
#define OSPF_CLI_OSPF_PROCESS_ID_STR                                          \
    "Process ID number"

char *ospf_abr_type_descr_str[] = 
{
  "Unknown",
  "Standard (RFC2328)",
  "Alternative Cisco (RFC3509)",
  "Alternative IBM (RFC3509)",
  "Alternative Shortcut"
};

char *ospf_shortcut_mode_descr_str[] = 
{
  "Default",
  "Enabled",
  "Disabled"
};


/* Utility functions. */
int
ospf_show_str2lsa_type (char *str)
{
  int type = -1;
  
  if (pal_strncmp (str, "r", 1) == 0)
    type = OSPF_ROUTER_LSA;
  else if (pal_strncmp (str, "ne", 2) == 0)
    type = OSPF_NETWORK_LSA;
  else if (pal_strncmp (str, "su", 2) == 0)
    type = OSPF_SUMMARY_LSA;
  else if (pal_strncmp (str, "a", 1) == 0)
    type = OSPF_SUMMARY_LSA_ASBR;
  else if (pal_strncmp (str, "e", 1) == 0)
    type = OSPF_AS_EXTERNAL_LSA;
#ifdef HAVE_NSSA
  else if (pal_strncmp (str, "ns", 2) == 0)
    type = OSPF_AS_NSSA_LSA;
#endif /* HAVE_NSSA */
#ifdef HAVE_OPAQUE_LSA
  else if (pal_strncmp (str, "opaque-l", 8) == 0)
    type = OSPF_LINK_OPAQUE_LSA;
  else if (pal_strncmp (str, "opaque-ar", 9) == 0)
    type = OSPF_AREA_OPAQUE_LSA;
  else if (pal_strncmp (str, "opaque-as", 9) == 0)
    type = OSPF_AS_OPAQUE_LSA;
#endif /* HAVE_OPAQUE_LSA */
  
  return type;
}


/* Show functions. */
void
show_ip_ospf_area (struct cli *cli, struct ospf_area *area)
{
  char area_id_str[OSPF_AREA_STRING_MAXLEN];
  struct pal_timeval now, delta;
  u_char auth_type, mode;
  int h, m, s;

  auth_type = ospf_area_auth_type_get (area);
  mode = ospf_area_shortcut_get (area);
  ospf_area_fmt_string (area->area_id, area->conf.format, area_id_str);

  /* Show Area ID. */
  cli_out (cli, "    Area %s", area_id_str);

  /* Show Area type/mode. */
  if (IS_AREA_BACKBONE (area))
    cli_out (cli, " (BACKBONE)");

  else if (!IS_AREA_DEFAULT (area))
    cli_out (cli, " (%s%s)",
             LOOKUP (ospf_area_type_msg, area->conf.external_routing),
             OSPF_AREA_CONF_CHECK (area, NO_SUMMARY) ? ", no-summary" : "");

  if (!IS_AREA_ACTIVE (area))
    cli_out (cli, " (Inactive)");
  cli_out (cli, "\n");

  /* Show number of interfaces. */
  cli_out (cli, "        Number of interfaces in this area is %lu(%lu)\n",
           area->active_if_count, ospf_area_if_count (area));

  /* Show number of fully adjacent neighbors. */
  cli_out (cli, "        Number of fully adjacent neighbors in this area is "
           "%d\n", area->full_nbr_count);

  /* Show number of fully adjacent virtual neighbors. */
  if (!IS_AREA_BACKBONE (area))
    cli_out (cli, "        Number of fully adjacent virtual neighbors through "
             "this area is %d\n", area->full_virt_nbr_count);

  /* Show authentication type. */
  cli_out (cli, "        Area has ");
  if (auth_type == OSPF_AUTH_NULL)
    cli_out (cli, "no authentication\n");
  else if (auth_type == OSPF_AUTH_SIMPLE)
    cli_out (cli, "simple password authentication\n");
  else if (auth_type == OSPF_AUTH_CRYPTOGRAPHIC)
    cli_out (cli, "message digest authentication\n");

  /* Show SPF calculation times. */
  if (area->spf_calc_count > 0)
    {
      pal_time_tzcurrent (&now, NULL);
      delta = TV_SUB (now, area->tv_spf);
      h = delta.tv_sec / 3600;
      m = (delta.tv_sec % 3600) / 60;
      s = delta.tv_sec % 60;

      cli_out (cli, "        SPF algorithm last executed "
               "%02d:%02d:%02d.%03d ago\n", h, m, s, delta.tv_usec / 1000);
    }
  cli_out (cli, "        SPF algorithm executed %lu times\n",
           area->spf_calc_count);

  /* Show number of LSA. */
  cli_out (cli, "        Number of LSA %ld. Checksum 0x%06x\n",
           ospf_lsdb_count_all (area->lsdb),
           ospf_lsdb_checksum_all (area->lsdb));

#ifdef HAVE_NSSA
  if (IS_AREA_NSSA (area))
    {
      cli_out (cli, "        NSSA Translator State is");
      if (area->translator_state == OSPF_NSSA_TRANSLATOR_DISABLED)
        cli_out (cli, " disabled");
      else if (area->translator_state == OSPF_NSSA_TRANSLATOR_ENABLED)
        cli_out (cli, " enabled");
      else if (area->translator_state == OSPF_NSSA_TRANSLATOR_ELECTED)
        cli_out (cli, " elected");
      cli_out (cli, "\n");
    }
#endif /* HAVE_NSSA */

  /* Show short-cut status. */
  if (!IS_AREA_DEFAULT (area))
    cli_out (cli, "        Shortcutting mode: %s, S-bit consensus: %s\n",
             ospf_shortcut_mode_descr_str[mode],
             CHECK_FLAG (area->flags, OSPF_AREA_SHORTCUT) ? "ok" : "no");
}

void
ospf_cli_show_uptime (struct cli *cli, struct ospf *top)
{
  pal_time_t uptime = ospf_proc_uptime (top);
  int d, h, m;

  if (uptime == 0)
    cli_out (cli, " Process is not up\n");
  else
    {
      d = uptime / 86400;
      h = (uptime % 86400) / 3600;
      m = (uptime % 3600) / 60;

      cli_out (cli, " Process uptime is");
      if (d != 0)
        cli_out (cli, " %d day%s", d, d <= 1 ? "" : "s");
      if (d != 0 || h != 0)
        cli_out (cli, " %d hour%s", h, h <= 1 ? "" : "s");
      cli_out (cli, " %d minute%s\n", m, m <= 1 ? "" : "s");
    }
}

void
ospf_cli_show_ospf (struct cli *cli, struct ospf *top)
{
  struct ls_node *rn;
  int count = 0;

  /* Show Router ID. */
  cli_out (cli, " Routing Process \"ospf %d\" with ID %r\n",
           top->ospf_id, &top->router_id);

  /* Show Uptime.  */
  ospf_cli_show_uptime (cli, top);

  /* Show VRF. */
  cli_out (cli, " Process bound to VRF %s\n",
           IS_APN_VRF_DEFAULT (top->ov->iv) ? "default" : top->ov->iv->name);

#ifdef HAVE_RESTART
  /* Show Graceful State. */
  if (CHECK_FLAG (top->flags, OSPF_ROUTER_RESTART))
    {
      if (top->restart_method == OSPF_RESTART_METHOD_GRACEFUL)
        cli_out (cli, " Router is in \"Graceful Restart\"\n");
      else if (top->restart_method == OSPF_RESTART_METHOD_SIGNALING)
        cli_out (cli, " Router is in \"Restart Signaling\"\n");
    }
#endif /* HAVE_RESTART */

#ifdef HAVE_BFD
  ospf_bfd_show_instance (cli, top);
#endif /* HAVE_BFD */

  cli_out (cli, " Conforms to RFC2328, and RFC1583 Compatibility flag is %s\n",
           IS_RFC1583_COMPATIBLE (top) ? "enabled" : "disabled");

  /* Show capability. */
  cli_out (cli, " Supports only single TOS(TOS0) routes\n");
#ifdef HAVE_OPAQUE_LSA
  if (IS_OPAQUE_CAPABLE (top))
    cli_out (cli, " Supports opaque LSA\n");
#endif /* HAVE_OPAQUE_LSA */

#ifdef HAVE_RESTART
  if (top->restart_method == OSPF_RESTART_METHOD_NEVER)
    cli_out (cli, " Do not support Restarting\n");
  else if (top->restart_method == OSPF_RESTART_METHOD_GRACEFUL)
    cli_out (cli, " Supports Graceful Restart\n");
  else if (top->restart_method == OSPF_RESTART_METHOD_SIGNALING)
    cli_out (cli, " Supports Restart Signaling\n");
#endif /* HAVE_RESTART */

#ifdef HAVE_VRF_OSPF
  /* When the instance is vrf enabled then it is 
     connected to MPLS VPN Superbackbone */
  if (CHECK_FLAG (top->ov->flags, OSPF_VRF_ENABLED))
    cli_out (cli, " Connected to MPLS VPN Superbackbone \n");
#endif /* HAVE_VRF_OSPF */

  /* Show ABR/ASBR flags. */
  if (IS_OSPF_ABR (top))
    cli_out (cli, " This router is an ABR, ABR Type is %s\n",
             ospf_abr_type_descr_str[top->abr_type]);

  if (IS_OSPF_ASBR (top))
    cli_out (cli, " This router is an ASBR "
             "(injecting external routing information)\n");

  /* Show SPF timers. */
  cli_out (cli, " SPF schedule delay min %lu.%lu secs, "
           "SPF schedule delay max %lu.%lu secs\n",
           top->spf_min_delay.tv_sec, (top->spf_min_delay.tv_usec / 1000), 
           top->spf_max_delay.tv_sec, (top->spf_max_delay.tv_usec / 1000));

  /* Show refresh parameters. */
  cli_out (cli, " Refresh timer %d secs\n", top->lsa_refresh.interval);

  /* Show current DD exchange neighbors.  */
  cli_out (cli, " Number of incomming current DD exchange neighbors %hu/%hu\n",
           top->dd_count_in, top->max_dd);
  cli_out (cli, " Number of outgoing current DD exchange neighbors %hu/%hu\n",
           top->dd_count_out, top->max_dd);

  /* Show number of AS-external-LSAs. */
  cli_out (cli, " Number of external LSA %d. Checksum 0x%06X\n",
           ospf_lsdb_count_all (top->lsdb),
           ospf_lsdb_checksum (top->lsdb, OSPF_AS_EXTERNAL_LSA));

  /* Show number of AS-opaque-LSAs. */
  cli_out (cli, " Number of opaque AS LSA %d. Checksum 0x%06X\n", 0, 0);

#ifdef HAVE_OSPF_DB_OVERFLOW
  cli_out (cli, " Number of non-default external LSA %lu\n",
           ospf_lsdb_count_external_non_default (top));
  cli_out (cli, " External LSA database ");
  if (top->ext_lsdb_limit == OSPF_DEFAULT_LSDB_LIMIT)
    cli_out (cli, "is unlimited.\n");
  else
    {
      cli_out (cli, "limit %lu\n", top->ext_lsdb_limit);
      cli_out (cli, " Exit database overflow state interval is ");
      if (top->exit_overflow_interval)
        cli_out (cli, "%lu seconds\n", top->exit_overflow_interval);
      else
        cli_out (cli, "not configured\n");

      cli_out (cli, " OSPF is%s in database overflow state now\n",
               IS_DB_OVERFLOW (top) ? "" : " not");

      if (top->t_overflow_exit)
        {
          struct pal_timeval now;
          struct pal_timeval rest;

          pal_time_tzcurrent (&now, NULL);
          rest = TV_SUB (top->t_overflow_exit->u.sands, now);
          cli_out (cli, " Next attempt to exit database overflow state "
                   "in %lu seconds\n", rest.tv_sec);
        }
    }
#endif /* HAVE_OSPF_DB_OVERFLOW */

  /* Show lsdb limit if configured. */
  if (CHECK_FLAG (top->config, OSPF_CONFIG_OVERFLOW_LSDB_LIMIT))
    {
      cli_out (cli, " LSDB database overflow limit is %u\n", 
               top->lsdb_overflow_limit);
      if (CHECK_FLAG (top->flags, OSPF_LSDB_EXCEED_OVERFLOW_LIMIT))
        cli_out (cli, " LSDB exceed overflow limit.\n");
    }

  cli_out (cli, " Number of LSA originated %lu\n", top->lsa_originate_count);
  cli_out (cli, " Number of LSA received %lu\n", top->rx_lsa_count);

  /* Show number of areas attached. */
  for (rn = ls_table_top (top->area_table); rn; rn = ls_route_next (rn))
    if (RN_INFO (rn, RNI_DEFAULT))
      count++;
  cli_out (cli, " Number of areas attached to this router: %d\n", count);

  /* Show each area status. */
  for (rn = ls_table_top (top->area_table); rn; rn = ls_route_next (rn))
    if (RN_INFO (rn, RNI_DEFAULT))
      show_ip_ospf_area (cli, RN_INFO (rn, RNI_DEFAULT));

  cli_out (cli, "\n");
}

void
ospf_cli_show_ospf_interface (struct cli *cli, struct ospf_interface *oi)
{
  struct ospf *top = oi->top;
  struct ospf_neighbor *nbr;
  char buf[OSPF_AREA_DESC_STRING_MAXLEN];
  char timer_str[OSPF_TIMER_STR_MAXLEN];

  /* Show OSPF interface information. */
  cli_out (cli, "  Internet Address ");
  if (IS_OSPF_IPV4_UNNUMBERED (oi))
    cli_out (cli, "Unnumbered (source %O)", oi->ident.address);
  else
    cli_out (cli, "%O,", oi->ident.address);

  cli_out (cli, " Area %s,", ospf_area_desc_string (oi->area, buf));
  cli_out (cli, " MTU %d\n", ospf_if_mtu_get (oi));

  cli_out (cli, "  Process ID %lu, Router ID %r, Network Type %s, Cost: %d\n",
           top->ospf_id, &top->router_id,
           LOOKUP (ospf_if_network_type_msg, oi->type), oi->output_cost);

  /* Show Transmit Delay and State. */
  cli_out (cli, "  Transmit Delay is %d sec, State %s",
           ospf_if_transmit_delay_get (oi),
           LOOKUP (ospf_ifsm_state_msg, oi->state));

  /* Show Priority on only Broadcast and NBMA. */
  if (oi->type == OSPF_IFTYPE_BROADCAST
      || oi->type == OSPF_IFTYPE_NBMA)
    cli_out (cli, ", Priority %d", oi->ident.priority);

#ifdef HAVE_OSPF_TE
  cli_out (cli, ", TE Metric %lu", oi->te_metric);
#endif /* HAVE_OSPF_TE */
  cli_out (cli, "\n");

  /* Show DR information. */
  if (oi->type == OSPF_IFTYPE_BROADCAST
      || oi->type == OSPF_IFTYPE_NBMA)
    {
      if (oi->ident.d_router.s_addr == 0)
        cli_out (cli, "  No designated router on this network\n");
      else
        {
          struct ospf_identity *ident = NULL;

          if (IS_DECLARED_DR (&oi->ident))
            ident = &oi->ident;
          else if ((nbr = ospf_nbr_lookup_by_addr (oi->nbrs,
                                                   &oi->ident.d_router)))
            ident = &nbr->ident;

          if (ident == NULL)
            cli_out (cli, "  No designated router on this network\n");
          else
            {
              cli_out (cli, "  Designated Router (ID) %r,",
                       &ident->router_id);
              cli_out (cli, " Interface Address %r\n",
                       &ident->address->u.prefix4);
            }
        }

      /* Show BDR information. */
      if (oi->ident.bd_router.s_addr == 0)
        cli_out (cli, "  No backup designated router on this network\n");
      else
        {
          struct ospf_identity *ident = NULL;

          if (IS_DECLARED_BACKUP (&oi->ident))
            ident = &oi->ident;
          else if ((nbr = ospf_nbr_lookup_by_addr (oi->nbrs,
                                                   &oi->ident.bd_router)))
            ident = &nbr->ident;

          if (ident == NULL)
            cli_out (cli, "  No backup designated router on this network\n");
          else
            {
              cli_out (cli, "  Backup Designated Router (ID) %r,",
                       &ident->router_id);
              cli_out (cli, " Interface Address %r\n",
                       &ident->address->u.prefix4);
            }
        }
    }

  /* Database filter. */
  if (ospf_if_database_filter_get (oi))
    cli_out (cli, "  Database-filter all out\n");

  cli_out (cli, "  Timer intervals configured,");
  cli_out (cli, " Hello %d, Dead %d, Wait %d, Retransmit %d\n",
           ospf_if_hello_interval_get (oi), ospf_if_dead_interval_get (oi),
           ospf_if_dead_interval_get (oi),
           ospf_if_retransmit_interval_get (oi));

#ifdef HAVE_RESTART
  if (top->restart_method == OSPF_RESTART_METHOD_SIGNALING)
    cli_out (cli, "    oob-resync timeout %lu\n",
             ospf_if_resync_timeout_get (oi));
#endif /* HAVE_RESTART */

  /* Drop the information below if the interface is loopback or not up.  */
  if (oi->type == OSPF_IFTYPE_LOOPBACK
      || !CHECK_FLAG (oi->flags, OSPF_IF_UP))
    return;

  if (!CHECK_FLAG (oi->flags, OSPF_IF_PASSIVE))
    cli_out (cli, "    Hello due in %s\n",
             ospf_timer_dump (oi->t_hello, timer_str));
  else /* OSPF_IF_PASSIVE is set */
    cli_out (cli, "    No Hellos (Passive interface)\n");

  cli_out (cli, "  Neighbor Count is %d, Adjacent neighbor count is %d\n",
           ospf_nbr_count_all (oi), ospf_nbr_count_full (oi));

#ifdef HAVE_MD5
  cli_out (cli, "  Crypt Sequence Number is %lu\n", oi->crypt_seqnum);
#endif /* HAVE_MD5 */

  cli_out (cli, "  Hello received %lu sent %lu, DD received %lu sent %lu\n",
           oi->hello_in, oi->hello_out, oi->db_desc_in, oi->db_desc_out);
  cli_out (cli, "  LS-Req received %lu sent %lu, "
           "LS-Upd received %lu sent %lu\n",
           oi->ls_req_in, oi->ls_req_out, oi->ls_upd_in, oi->ls_upd_out);

  cli_out (cli, "  LS-Ack received %lu sent %lu, Discarded %lu\n",
           oi->ls_ack_in, oi->ls_ack_out, oi->discarded);

#ifdef HAVE_BFD
  ospf_bfd_show_interface (cli, oi);
#endif /* HAVE_BFD */

  /* XXX Suppress hello for ? neighbor() */
  /* Message digest authentication */
  /*   Youngest key id is ? */
}

void
ospf_cli_show_interface (struct cli *cli, struct interface *ifp)
{
  struct ospf_master *om = cli->vr->proto;
  struct ospf_interface *oi;
  struct connected *ifc;
#ifdef HAVE_OSPF_MULTI_INST
  struct ospf *top = NULL;
  struct ospf_interface *oi_alt = NULL;
  struct listnode *node = NULL;
#endif /* HAVE_OSPF_MULTI_INST */
  int count = 0;

  for (ifc = ifp->ifc_ipv4; ifc; ifc = ifc->next)
    if (ospf_global_if_entry_lookup (om, ifc->address->u.prefix4,
                                     ifp->ifindex))
      {
        count++;
        break;
      }

  if (count == 0)
    return;

  /* Is interface up?  */
  if (!if_is_up (ifp))
    cli_out (cli, "%s is down, line protocol is down\n", ifp->name);
  else
    cli_out (cli, "%s is up, line protocol is up\n", ifp->name);

  for (ifc = ifp->ifc_ipv4; ifc; ifc = ifc->next)
    if ((oi = ospf_global_if_entry_lookup (om, ifc->address->u.prefix4,
                                           ifp->ifindex)))
      {
         ospf_cli_show_ospf_interface (cli, oi);
#ifdef HAVE_OSPF_MULTI_INST
         /* If multiple instances are running on this interface */
         if (CHECK_FLAG (oi->flags, OSPF_IF_EXT_MULTI_INST))
         /* Display all ospf instances those are running on this interface */
           LIST_LOOP(om->ospf, top, node)
             {
               if (top != oi->top)
                 {
                   oi_alt = ospf_if_entry_match (top, oi);
                   if (oi_alt != NULL)
                     ospf_cli_show_ospf_interface (cli, oi_alt);
                 }
             }
#endif /* HAVE_OSPF_MULTI_INST */
      }
}

void
ospf_cli_show_ospf_virtual_links (struct cli *cli, struct ospf_vlink *vlink)
{
  struct ospf_neighbor *nbr;
  char timer_str[OSPF_TIMER_STR_MAXLEN];

  cli_out (cli, "Virtual Link %s to router %r is %s\n",
           vlink->name, &vlink->peer_id,
           CHECK_FLAG (vlink->flags, OSPF_VLINK_UP) ? "up" : "down");
  cli_out (cli, "  Transit area %r via interface %s\n",
           &vlink->area_id, vlink->out_oi ? vlink->out_oi->u.ifp->name : "*");

  cli_out (cli, "  Local address ");
  if (vlink->oi->ident.address->u.prefix4.s_addr)
    cli_out (cli, "%P\n", vlink->oi->ident.address);
  else
    cli_out (cli, "*\n");

  cli_out (cli, "  Remote address ");
  if (vlink->oi->destination->u.prefix4.s_addr)
    cli_out (cli, "%P\n", vlink->oi->destination);
  else
    cli_out (cli, "*\n");

  cli_out (cli, "  Transmit Delay is %d sec, State %s,\n",
           ospf_if_transmit_delay_get (vlink->oi),
           LOOKUP (ospf_ifsm_state_msg, vlink->oi->state));
  cli_out (cli, "  Timer intervals configured, "
           "Hello %d, Dead %d, Wait %d, Retransmit %d\n",
           ospf_if_hello_interval_get (vlink->oi),
           ospf_if_dead_interval_get (vlink->oi),
           ospf_if_dead_interval_get (vlink->oi),
           ospf_if_retransmit_interval_get (vlink->oi));
  cli_out (cli, "    Hello due in %s\n",
           ospf_timer_dump (vlink->oi->t_hello, timer_str));

  nbr = ospf_nbr_lookup_ptop (vlink->oi);

  if (nbr != NULL)
    cli_out (cli, "    Adjacency state %s\n",
             LOOKUP (ospf_nfsm_state_msg, nbr->state));
  else
    cli_out (cli, "    Adjacency state Down\n");
}

void
ospf_cli_show_virtual_links (struct cli *cli, struct ospf *top)
{
  struct ospf_vlink *vlink;
  struct ls_node *rn;

  for (rn = ls_table_top (top->vlink_table); rn; rn = ls_route_next (rn))
    if ((vlink = RN_INFO (rn, RNI_DEFAULT)))
      ospf_cli_show_ospf_virtual_links (cli, vlink);
}
#ifdef HAVE_OSPF_MULTI_AREA
void
ospf_cli_show_ospf_multi_area_links (struct cli *cli, 
                                     struct ospf_multi_area_link *mlink)
{
  struct ospf *top = NULL;
  struct ospf_interface *oi = NULL;
  char buf[OSPF_AREA_DESC_STRING_MAXLEN];
  char timer_str[OSPF_TIMER_STR_MAXLEN];

  cli_out (cli, "  Multi-area-adjacency link on interface %s"
           " to neighbor %r\n",
           mlink->ifname, &mlink->nbr_addr);
  if ((oi = mlink->oi))
    { 
      top = oi->top;
      /* Show OSPF interface information. */
      cli_out (cli, "  Internet Address ");
      if (IS_OSPF_IPV4_UNNUMBERED (oi))
        cli_out (cli, "Unnumbered (source %O)", oi->ident.address);
      else
        cli_out (cli, "%O,", oi->ident.address);

      cli_out (cli, " Area %s,", ospf_area_desc_string (oi->area, buf));
      cli_out (cli, " MTU %d\n", ospf_if_mtu_get (oi));

      cli_out (cli, "  Process ID %lu, Router ID %r, Network Type %s,"
               " Cost: %d\n", top->ospf_id, &top->router_id,
               LOOKUP (ospf_if_network_type_msg, oi->type), oi->output_cost);
      /* Show Transmit Delay and State. */
      cli_out (cli, "  Transmit Delay is %d sec, State %s",
           ospf_if_transmit_delay_get (oi),
           LOOKUP (ospf_ifsm_state_msg, oi->state));

#ifdef HAVE_OSPF_TE
      cli_out (cli, ", TE Metric %lu", oi->te_metric);
#endif /* HAVE_OSPF_TE */
      cli_out (cli, "\n");

      /* Database filter. */
      if (ospf_if_database_filter_get (oi))
        cli_out (cli, "  Database-filter all out\n");

      cli_out (cli, "  Timer intervals configured,");
      cli_out (cli, " Hello %d, Dead %d, Wait %d, Retransmit %d\n",
               ospf_if_hello_interval_get (oi), ospf_if_dead_interval_get (oi),
               ospf_if_dead_interval_get (oi),
               ospf_if_retransmit_interval_get (oi));

#ifdef HAVE_RESTART
      if (top->restart_method == OSPF_RESTART_METHOD_SIGNALING)
        cli_out (cli, "    oob-resync timeout %lu\n",
                 ospf_if_resync_timeout_get (oi));
#endif /* HAVE_RESTART */
      /* Drop the information below if the interface is loopback or not up.  */
      if (oi->type == OSPF_IFTYPE_LOOPBACK
          || !CHECK_FLAG (oi->flags, OSPF_IF_UP))
        return;

      if (!CHECK_FLAG (oi->flags, OSPF_IF_PASSIVE))
        cli_out (cli, "    Hello due in %s\n",
                  ospf_timer_dump (oi->t_hello, timer_str));
      else /* OSPF_IF_PASSIVE is set */
        cli_out (cli, "    No Hellos (Passive interface)\n");

      cli_out (cli, "  Neighbor Count is %d, Adjacent neighbor count is %d\n",
               ospf_nbr_count_all (oi), ospf_nbr_count_full (oi));

#ifdef HAVE_MD5
      cli_out (cli, "  Crypt Sequence Number is %lu\n", oi->crypt_seqnum);
#endif /* HAVE_MD5 */

      cli_out (cli, "  Hello received %lu sent %lu, DD received %lu sent %lu\n",
               oi->hello_in, oi->hello_out, oi->db_desc_in, oi->db_desc_out);
      cli_out (cli, "  LS-Req received %lu sent %lu, "
               "LS-Upd received %lu sent %lu\n",
               oi->ls_req_in, oi->ls_req_out, oi->ls_upd_in, oi->ls_upd_out);

      cli_out (cli, "  LS-Ack received %lu sent %lu, Discarded %lu\n",
               oi->ls_ack_in, oi->ls_ack_out, oi->discarded);
    }
}

void
ospf_cli_show_multi_area_links (struct cli *cli, struct ospf *top)
{
  struct ospf_multi_area_link *mlink = NULL;
  struct ls_node *rn = NULL;

  for (rn = ls_table_top (top->multi_area_link_table); rn;
       rn = ls_route_next (rn))
    if ((mlink = RN_INFO (rn, RNI_DEFAULT)))
      ospf_cli_show_ospf_multi_area_links (cli, mlink);
}
#endif /* HAVE_OSPF_MULTI_AREA */



void
ospf_cli_show_ospf_neighbor (struct cli *cli, struct ospf_neighbor *nbr,
                             int private)
{
  struct ospf_interface *oi = nbr->oi;
  char timer_str[OSPF_TIMER_STR_MAXLEN];
  char nbr_state_str[OSPF_NBR_STATE_STR_MAXLEN];
  char if_str[OSPF_IF_STRING_MAXLEN];

  ospf_nbr_state_message (nbr, nbr_state_str);

  if ((nbr->state == NFSM_Down
       || nbr->state == NFSM_Attempt)
      && nbr->ident.router_id.s_addr == 0)
    cli_out (cli, "%-15s %3d   %-15s %8s    ", "N/A",
             nbr->ident.priority, nbr_state_str,
             ospf_timer_dump (nbr->t_inactivity, timer_str));
#ifdef HAVE_RESTART
  else if (CHECK_FLAG (nbr->flags, OSPF_NEIGHBOR_RESTART))
    cli_out (cli, "%-15r %3d   %-15s %8s%c   ",
             &nbr->ident.router_id, nbr->ident.priority, nbr_state_str,
             ospf_timer_dump (nbr->t_helper, timer_str), '*');
#endif /* HAVE_RESTART */
  else
    cli_out (cli, "%-15r %3d   %-15s %8s%c   ",
             &nbr->ident.router_id, nbr->ident.priority, nbr_state_str,
             ospf_timer_dump (nbr->t_inactivity, timer_str), ' ');

  cli_out (cli, "%-15r ", &nbr->ident.address->u.prefix4);
  if (private)
    {
#ifdef HAVE_OSPF_MULTI_INST
      if (oi->network)
        cli_out (cli, "%-15s %d %5lu %5lu %5lu", IF_NAME (oi),
                 oi->network->inst_id,
                 ospf_ls_retransmit_count (nbr),
                 ospf_ls_request_count (nbr), ospf_db_summary_count (nbr));
      else
#endif /* HAVE_OSPF_MULTI_INST */
      cli_out (cli, "%-15s %5lu %5lu %5lu", IF_NAME (oi),
               ospf_ls_retransmit_count (nbr),
               ospf_ls_request_count (nbr), ospf_db_summary_count (nbr));
    }
  else
    {
      if (oi->type != OSPF_IFTYPE_VIRTUALLINK)
        cli_out (cli, "%s", oi->u.ifp->name);
      else
        cli_out (cli, "%s", oi->u.vlink->name);
#ifdef HAVE_OSPF_MULTI_INST
      if (oi->network)
        cli_out (cli, "%8d", oi->network->inst_id);
#endif /* HAVE_OSPF_MULTI_INST */
    }
  cli_out (cli, "\n");
}

void
ospf_cli_show_ospf_neighbor_if (struct cli *cli, struct ospf_interface *oi,
                                int private)
{
  struct ospf_neighbor *nbr;
  struct ls_node *rn;

  if (!CHECK_FLAG (oi->flags, OSPF_IF_UP))
    return;

  for (rn = ls_table_top (oi->nbrs); rn; rn = ls_route_next (rn))
    if ((nbr = RN_INFO (rn, RNI_DEFAULT)))
      if (nbr->state != NFSM_Down)
        ospf_cli_show_ospf_neighbor (cli, nbr, private);
}

void
ospf_cli_show_ospf_neighbor_if_all (struct cli *cli, struct ospf_interface *oi)
{
  struct ospf_neighbor *nbr;
  struct ls_node *rn;

  if (!CHECK_FLAG (oi->flags, OSPF_IF_UP))
    return;

  for (rn = ls_table_top (oi->nbrs); rn; rn = ls_route_next (rn))
    if ((nbr = RN_INFO (rn, RNI_DEFAULT)))
      ospf_cli_show_ospf_neighbor (cli, nbr, 0);
}
#ifdef HAVE_OSPF_MULTI_INST
#define OSPF_SHOW_NEIGHBOR_HEADER                                             \
    "Neighbor ID     Pri   State           Dead Time   Address         "      \
    "Interface Instance ID"
#define OSPF_SHOW_NEIGHBOR_PRIVATE_HEADER                                     \
    "Neighbor ID     Pri   State           Dead Time   Address         "      \
    "Interface  Instance ID  RXmtL RqstL DBsmL"
#else
#define OSPF_SHOW_NEIGHBOR_HEADER                                             \
   "Neighbor ID     Pri   State           Dead Time   Address         "       \
   "Interface"
#define OSPF_SHOW_NEIGHBOR_PRIVATE_HEADER                                     \
   "Neighbor ID     Pri   State           Dead Time   Address         "       \
    "Interface           RXmtL RqstL DBsmL"
#endif /* HAVE_OSPF_MULTI_INST */

void
ospf_cli_show_neighbor (struct cli *cli, struct ospf *top, int private)
{
  struct ospf_interface *oi;
  struct ospf_vlink *vlink;
  struct ls_node *rn;
#ifdef HAVE_OSPF_MULTI_AREA
  struct ospf_multi_area_link *mlink = NULL;
#endif /* HAVE_OSPF_MULTI_AREA */

  cli_out (cli, "\nOSPF process %d:\n", top->ospf_id);
  if (private)
    cli_out (cli, OSPF_SHOW_NEIGHBOR_PRIVATE_HEADER "\n");
  else
    cli_out (cli, OSPF_SHOW_NEIGHBOR_HEADER "\n");

  /* Show neighbors. */
  for (rn = ls_table_top (top->if_table); rn; rn = ls_route_next (rn))
    if ((oi = RN_INFO (rn, RNI_DEFAULT)))
      ospf_cli_show_ospf_neighbor_if (cli, oi, private);

  /* Show virtual neighbors. */
  for (rn = ls_table_top (top->vlink_table); rn; rn = ls_route_next (rn))
    if ((vlink = RN_INFO (rn, RNI_DEFAULT)))
      ospf_cli_show_ospf_neighbor_if (cli, vlink->oi, private);

#ifdef HAVE_OSPF_MULTI_AREA
  /* Show multi area adj neighbors */
  for (rn = ls_table_top (top->multi_area_link_table); rn;
                                             rn = ls_route_next (rn))
    if ((mlink = RN_INFO (rn, RNI_DEFAULT)))
      if (mlink->oi)
        ospf_cli_show_ospf_neighbor_if (cli, mlink->oi , private);
#endif /* HAVE_OSPF_MULTI_AREA */
}

void
ospf_cli_show_neighbor_all (struct cli *cli, struct ospf *top)
{
  struct ospf_interface *oi;
  struct ospf_vlink *vlink;
  struct ls_node *rn;
#ifdef HAVE_OSPF_MULTI_AREA
  struct ospf_multi_area_link *mlink = NULL;
#endif /* HAVE_OSPF_MULTI_AREA */

  cli_out (cli, "\nOSPF process %d:\n", top->ospf_id);

  /* Show All neighbors. */
  cli_out (cli, OSPF_SHOW_NEIGHBOR_HEADER "\n");

  for (rn = ls_table_top (top->if_table); rn; rn = ls_route_next (rn))
    if ((oi = RN_INFO (rn, RNI_DEFAULT)))
      ospf_cli_show_ospf_neighbor_if_all (cli, oi);

  for (rn = ls_table_top (top->vlink_table); rn; rn = ls_route_next (rn))
    if ((vlink = RN_INFO (rn, RNI_DEFAULT)))
      if (vlink->oi)
        ospf_cli_show_ospf_neighbor_if_all (cli, vlink->oi);
#ifdef HAVE_OSPF_MULTI_AREA
  /* Show multi area adj neighbors */
  for (rn = ls_table_top (top->multi_area_link_table); rn;
               rn = ls_route_next (rn))
    if ((mlink = RN_INFO (rn, RNI_DEFAULT)))
      if (mlink->oi)
        ospf_cli_show_ospf_neighbor_if_all (cli, mlink->oi);
#endif /* HAVE_OSPF_MULTI_AREA */
}

void
ospf_cli_show_neighbor_by_if (struct cli *cli, struct ospf *top,
                              struct pal_in4_addr addr)
{
  struct ospf_interface *oi;
  
  oi = ospf_if_entry_lookup (top, addr, 0);
  if (oi != NULL)
    {
      cli_out (cli, "\nOSPF process %d\n", top->ospf_id);
      cli_out (cli, OSPF_SHOW_NEIGHBOR_HEADER "\n");
      ospf_cli_show_ospf_neighbor_if (cli, oi, 0);
    }
}

void
ospf_cli_show_ospf_neighbor_detail (struct cli *cli, struct ospf_neighbor *nbr)
{
  struct ospf_interface *oi = nbr->oi;
  struct ospf_nbr_static *nbr_static = NULL;
  char area_desc_str[OSPF_AREA_DESC_STRING_MAXLEN];
  char timer_str[OSPF_TIMER_STR_MAXLEN];
  char option_str[OSPF_OPTION_STR_MAXLEN];

  if (CHECK_FLAG (nbr->flags, OSPF_NEIGHBOR_STATIC))
    nbr_static = ospf_nbr_static_lookup (nbr->oi->top,
                                         nbr->ident.address->u.prefix4);

  if ((nbr->state == NFSM_Down
       || nbr->state == NFSM_Attempt)
      && nbr->ident.router_id.s_addr == 0)
    cli_out (cli, " Neighbor N/A,");
  else
    cli_out (cli, " Neighbor %r,", &nbr->ident.router_id);

  cli_out (cli, " interface address %r\n", &nbr->ident.address->u.prefix4);
  cli_out (cli, "    In the area %s via interface %s\n",
           ospf_area_desc_string (oi->area, area_desc_str),
           oi->type == OSPF_IFTYPE_VIRTUALLINK ?
           oi->u.vlink->name : oi->u.ifp->name);
  cli_out (cli, "    Neighbor priority is %d, State is %s, %d state changes\n",
           nbr->ident.priority, LOOKUP (ospf_nfsm_state_msg, nbr->state),
           nbr->state_change);

  if (nbr_static)
    cli_out (cli, "    Poll interval %d\n", nbr_static->poll_interval);

  cli_out (cli, "    DR is %r, BDR is %r\n",
           &nbr->ident.d_router, &nbr->ident.bd_router);

  cli_out (cli, "    Options is 0x%02X (%s)\n", nbr->options,
           ospf_option_dump (nbr->options, option_str));
#ifdef HAVE_RESTART
  if (oi->top->restart_method == OSPF_RESTART_METHOD_SIGNALING)
    {
      char eo_str[OSPF_EXTENDED_OPTION_STR_MAXLEN];
      cli_out (cli, "    LLS Options is 0x%x (%s)", nbr->eo_options,
               ospf_extended_option_dump (nbr->eo_options, eo_str));
      if (CHECK_FLAG (nbr->flags, OSPF_NEIGHBOR_RESYNC_PROCESS)
          && !CHECK_FLAG (oi->top->flags, OSPF_ROUTER_RESTART))
        cli_out (cli, ", OOB-Resync in progress (receiver)");
      else if (nbr->last_oob > 0)
        cli_out (cli, ", last OOB-Resync %s ago",
                 ospf_uptime_dump (nbr->last_oob, timer_str));
      cli_out (cli, "\n");

      if (nbr->t_resync)
        cli_out (cli, "    oob-resync timeout in %s\n",
                 ospf_timer_dump (nbr->t_resync, timer_str));
    }
#endif /* HAVE_RESTART */
  cli_out (cli, "    Dead timer due in %s\n",
           ospf_timer_dump (nbr->t_inactivity, timer_str));

  if (nbr_static)
    cli_out (cli, "    Poll due in %s\n",
             ospf_timer_dump (nbr_static->t_poll, timer_str));

  cli_out (cli, "    Neighbor is up for %s\n",
           ospf_uptime_dump (nbr->ctime, timer_str));

  cli_out (cli, "    Database Summary List %d\n", ospf_db_summary_count (nbr));
  cli_out (cli, "    Link State Request List %ld\n",
           ospf_ls_request_count (nbr));
  cli_out (cli, "    Link State Retransmission List %ld\n",
           ospf_ls_retransmit_count (nbr));

#ifdef HAVE_MD5
  cli_out (cli, "    Crypt Sequence Number is %lu\n", nbr->crypt_seqnum);
#endif /* HAVE_MD5 */

  cli_out (cli, "    Thread Inactivity Timer %s\n", 
           nbr->t_inactivity != NULL ? "on" : "off");
  cli_out (cli, "    Thread Database Description Retransmission %s\n",
           nbr->t_db_desc != NULL ? "on" : "off");
  cli_out (cli, "    Thread Link State Request Retransmission %s\n",
           nbr->t_ls_req != NULL ? "on" : "off");
  cli_out (cli, "    Thread Link State Update Retransmission %s\n",
           nbr->t_ls_upd != NULL ? "on" : "off");

  if (nbr_static)
    cli_out (cli, "    Thread Poll Timer %s\n", 
             nbr_static->t_poll != NULL ? "on" : "off");

#ifdef HAVE_BFD
  ospf_bfd_show_nbr_detail (cli, nbr);
#endif /* HAVE_BFD */

  cli_out (cli, "\n");
}

void
ospf_cli_show_neighbor_by_nbr_id (struct cli *cli, struct ospf *top,
                                  struct pal_in4_addr nbr_id)
{
  struct ospf_interface *oi;
  struct ospf_neighbor *nbr;
  struct ospf_vlink *vlink;
  struct ls_node *rn;
#ifdef HAVE_OSPF_MULTI_AREA
  struct ospf_multi_area_link *mlink = NULL;
#endif /* HAVE_OSPF_MULTI_AREA */

  cli_out (cli, "\nOSPF process %d:\n", top->ospf_id);

  for (rn = ls_table_top (top->if_table); rn; rn = ls_route_next (rn))
    if ((oi = RN_INFO (rn, RNI_DEFAULT)))
      if ((nbr = ospf_nbr_lookup_by_router_id (oi->nbrs, &nbr_id)))
        ospf_cli_show_ospf_neighbor_detail (cli, nbr);

  for (rn = ls_table_top (top->vlink_table); rn; rn = ls_route_next (rn))
    if ((vlink = RN_INFO (rn, RNI_DEFAULT)))
      if (vlink->oi)
        if ((nbr = ospf_nbr_lookup_by_router_id (vlink->oi->nbrs, &nbr_id)))
          ospf_cli_show_ospf_neighbor_detail (cli, nbr);
#ifdef HAVE_OSPF_MULTI_AREA
  for (rn = ls_table_top (top->multi_area_link_table); rn;
                                          rn = ls_route_next (rn))
    if ((mlink = RN_INFO (rn, RNI_DEFAULT)))
      if (mlink->oi)
        if ((nbr = ospf_nbr_lookup_by_router_id (mlink->oi->nbrs, &nbr_id)))
        ospf_cli_show_ospf_neighbor_detail (cli, nbr);
#endif /* HAVE_OSPF_MULTI_AREA */
}

void
ospf_cli_show_ospf_neighbor_if_detail_all (struct cli *cli,
                                           struct ospf_interface *oi)
{
  struct ospf_neighbor *nbr;
  struct ls_node *rn;

  for (rn = ls_table_top (oi->nbrs); rn; rn = ls_route_next (rn))
    if ((nbr = RN_INFO (rn, RNI_DEFAULT)))
      ospf_cli_show_ospf_neighbor_detail (cli, nbr);
}

void
ospf_cli_show_neighbor_detail_all (struct cli *cli, struct ospf *top)
{
  struct ospf_interface *oi;
  struct ospf_vlink *vlink;
  struct ls_node *rn;
#ifdef HAVE_OSPF_MULTI_AREA
  struct ospf_multi_area_link *mlink = NULL;
#endif /* HAVE_OSPF_MULTI_AREA */
  
  cli_out (cli, "OSPF process %d:\n\n", top->ospf_id);

  for (rn = ls_table_top (top->if_table); rn; rn = ls_route_next (rn))
    if ((oi = RN_INFO (rn, RNI_DEFAULT)))
      ospf_cli_show_ospf_neighbor_if_detail_all (cli, oi);

  for (rn = ls_table_top (top->vlink_table); rn; rn = ls_route_next (rn))
    if ((vlink = RN_INFO (rn, RNI_DEFAULT)))
      if (vlink->oi)
        ospf_cli_show_ospf_neighbor_if_detail_all (cli, vlink->oi);

#ifdef HAVE_OSPF_MULTI_AREA
  for (rn = ls_table_top (top->multi_area_link_table); rn;
                                       rn = ls_route_next (rn))
    if ((mlink = RN_INFO (rn, RNI_DEFAULT)))
      if (mlink->oi)
        ospf_cli_show_ospf_neighbor_if_detail_all (cli, mlink->oi);
#endif /* HAVE_OSPF_MULTI_AREA */
}

void
ospf_cli_show_neighbor_detail_by_if (struct cli *cli, struct ospf *top,
                                     struct pal_in4_addr addr)
{
  struct ospf_interface *oi;
  struct ospf_neighbor *nbr;
  struct ls_node *rn;
  
  oi = ospf_if_entry_lookup (top, addr, 0);
  if (oi != NULL)
    for (rn = ls_table_top (oi->nbrs); rn; rn = ls_route_next (rn))
      if ((nbr = RN_INFO (rn, RNI_DEFAULT)))
        if (nbr->state != NFSM_Down)
          ospf_cli_show_ospf_neighbor_detail (cli, nbr);
}

void
ospf_cli_show_ospf_neighbor_if_detail (struct cli *cli,
                                       struct ospf_interface *oi)
{
  struct ospf_neighbor *nbr;
  struct ls_node *rn;

  for (rn = ls_table_top (oi->nbrs); rn; rn = ls_route_next (rn))
    if ((nbr = RN_INFO (rn, RNI_DEFAULT)))
      if (nbr->state != NFSM_Down)
        ospf_cli_show_ospf_neighbor_detail (cli, nbr);
}

void
ospf_cli_show_neighbor_detail (struct cli *cli, struct ospf *top)
{
  struct ospf_interface *oi;
  struct ospf_vlink *vlink;
  struct ls_node *rn;
#ifdef HAVE_OSPF_MULTI_AREA
  struct ospf_multi_area_link *mlink = NULL;
#endif /* HAVE_OSPF_MULTI_AREA */
  
  for (rn = ls_table_top (top->if_table); rn; rn = ls_route_next (rn))
    if ((oi = RN_INFO (rn, RNI_DEFAULT)))
      ospf_cli_show_ospf_neighbor_if_detail (cli, oi);

  for (rn = ls_table_top (top->vlink_table); rn; rn = ls_route_next (rn))
    if ((vlink = RN_INFO (rn, RNI_DEFAULT)))
      ospf_cli_show_ospf_neighbor_if_detail (cli, vlink->oi);

#ifdef HAVE_OSPF_MULTI_AREA
  for (rn = ls_table_top (top->multi_area_link_table); rn;
                                 rn = ls_route_next (rn))
    if ((mlink = RN_INFO (rn, RNI_DEFAULT)))
      if (mlink->oi)
        ospf_cli_show_ospf_neighbor_if_detail (cli, mlink->oi);
#endif /* HAVE_OSPF_MULTI_AREA */
}
  

struct ospf_show_database_arg
{
  /* CLI argument. */
  int proc_id;
  int flags;
#define OSPF_SHOW_DATABASE_SELF         (1 << 0)
#define OSPF_SHOW_DATABASE_MAXAGE       (1 << 1)
#define OSPF_SHOW_DATABASE_ADV_ROUTER   (1 << 2)
#define OSPF_SHOW_DATABASE_ID           (1 << 3)

  int type;
  struct pal_in4_addr id;
  struct pal_in4_addr adv_router;

  /* Show LSA function. */
  int (*show) (struct cli *, struct ospf_lsa *,
               struct ospf_show_database_arg *);
  int max_count;
  int show_flags;
#define OSPF_SHOW_DATABASE_HEADER_PROC          (1 << 0)
#define OSPF_SHOW_DATABASE_HEADER_TYPE          (1 << 1)
#define OSPF_SHOW_DATABASE_HEADER_BRIEF         (1 << 2)

  /* State. */
  struct
  {
    /* Current OSPF process ID. */
    int proc_id;

    /* Current LSDB parent type. */
    int state;
#define OSPF_SHOW_DATABASE_STATE_INIT           0
#define OSPF_SHOW_DATABASE_STATE_LINK           1
#define OSPF_SHOW_DATABASE_STATE_VLINK          2
#define OSPF_SHOW_DATABASE_STATE_AREA           3
#define OSPF_SHOW_DATABASE_STATE_AS             4

    /* LSDB parent key. */
    union
    {
      struct
      {
        struct pal_in4_addr addr;
        int ifindex;
      } i;

      struct
      {
        struct pal_in4_addr area_id;
        struct pal_in4_addr nbr_id;
      } v;

      struct pal_in4_addr area_id;

    } u;

    int indexlen;

    int type;

    /* Current LSA. */
    struct ospf_lsa *lsa;

  } st;
};

char *show_database_desc[] =
{
  "unknown",
  "Router Link States",
  "Net Link States",
  "Summary Link States",
  "ASBR-Summary Link States",
  "AS External Link States",
#if defined (HAVE_NSSA) || defined (HAVE_OPAQUE_LSA)
  "Group Membership LSA",
  "NSSA-external Link States",
#endif /* HAVE_NSSA */
#ifdef HAVE_OPAQUE_LSA
  "Type-8 LSA",
  "Link-Local Opaque-LSA",
  "Area-Local Opaque-LSA",
  "AS-Global Opaque-LSA",
#endif /* HAVE_OPAQUE_LSA */
};

#define SHOW_OSPF_COMMON_HEADER \
  "Link ID         ADV Router      Age  Seq#       CkSum"

char *show_database_header[] =
{
  "",
  "Link ID         ADV Router      Age  Seq#       CkSum  Link count",
  "Link ID         ADV Router      Age  Seq#       CkSum",
  "Link ID         ADV Router      Age  Seq#       CkSum  Route",
  "Link ID         ADV Router      Age  Seq#       CkSum",
  "Link ID         ADV Router      Age  Seq#       CkSum  Route              Tag",
#ifdef HAVE_NSSA
  " --- header for Group Member ----",
  "Link ID         ADV Router      Age  Seq#       CkSum  Route              Tag",
#endif /* HAVE_NSSA */
#ifdef HAVE_OPAQUE_LSA
#ifndef HAVE_NSSA
  "--- type-6 ---",
  "--- type-7 ---",
#endif /* HAVE_NSSA */
  "--- type-8 ---",
  "Link ID         ADV Router      Age  Seq#       CkSum Opaque ID",
  "Link ID         ADV Router      Age  Seq#       CkSum Opaque ID",
  "Link ID         ADV Router      Age  Seq#       CkSum Opaque ID",
#endif /* HAVE_OPAQUE_LSA */
};

int
ospf_show_database_header (struct cli *cli, struct ospf_lsa *lsa,
                           struct ospf_show_database_arg *arg)
{
  struct ospf *top = ospf_proc_lookup_by_lsdb (lsa->lsdb);
  struct ospf_interface *oi;
  struct ospf_area *area;
  u_char type = lsa->data->type;
  char if_str[OSPF_IF_STRING_MAXLEN];
  char area_desc_str[OSPF_AREA_DESC_STRING_MAXLEN];

  if (CHECK_FLAG (arg->show_flags, OSPF_SHOW_DATABASE_HEADER_PROC))
    {
      if (top)
        cli_out (cli, "\n            OSPF Router with ID (%r) (Process ID %d)\n",
                 &top->router_id, top->ospf_id);

      UNSET_FLAG (arg->show_flags, OSPF_SHOW_DATABASE_HEADER_PROC);
    }

  if (CHECK_FLAG (arg->show_flags, OSPF_SHOW_DATABASE_HEADER_TYPE))
    {
      switch (LSA_FLOOD_SCOPE (type))
        {
        case LSA_FLOOD_SCOPE_LINK:
          oi = lsa->lsdb->u.oi;
          cli_out (cli, "\n                %s (Link %s)\n\n",
                   show_database_desc[type], IF_NAME (oi));
          break;
        case LSA_FLOOD_SCOPE_AREA:
          area = lsa->lsdb->u.area;
          cli_out (cli, "\n                %s (Area %s)\n\n",
                   show_database_desc[type],
                   ospf_area_desc_string (area, area_desc_str));
          break;
        case LSA_FLOOD_SCOPE_AS:
          cli_out (cli, "\n                %s \n\n", show_database_desc[type]);
          break;
        }

      UNSET_FLAG (arg->show_flags, OSPF_SHOW_DATABASE_HEADER_TYPE);
    }

  if (CHECK_FLAG (arg->show_flags, OSPF_SHOW_DATABASE_HEADER_BRIEF))
    {
      cli_out (cli, "%s\n", show_database_header[type]);
      UNSET_FLAG (arg->show_flags, OSPF_SHOW_DATABASE_HEADER_BRIEF);
    }

  return 0;
}

/* `show ip ospf database' related stuff. */
int
ospf_show_database_brief (struct cli *cli, struct ospf_lsa *lsa,
                          struct ospf_show_database_arg *arg)
{
  struct pal_in4_addr *mask;
  struct prefix_ipv4 p;
  u_int32_t tag;
  u_int16_t links;
  u_char e_bit;

  ospf_show_database_header (cli, lsa, arg);

  /* LSA common part show. */
  cli_out (cli, "%-15r ", &lsa->data->id);
  cli_out (cli, "%-15r %4d 0x%08x 0x%04x",
           &lsa->data->adv_router, LS_AGE (lsa),
           pal_ntoh32 (lsa->data->ls_seqnum),
           pal_ntoh16 (lsa->data->checksum));
  /* LSA specific part show. */
  switch (lsa->data->type)
    {
    case OSPF_ROUTER_LSA:
      links = OSPF_PNT_UINT16_GET (lsa->data,
                                   OSPF_ROUTER_LSA_NUM_LINKS_OFFSET);
      cli_out (cli, " %-d", links);
      break;
    case OSPF_SUMMARY_LSA:
      mask =
        OSPF_PNT_IN_ADDR_GET (lsa->data, OSPF_SUMMARY_LSA_NETMASK_OFFSET);
      p.family = AF_INET;
      p.prefix = lsa->data->id;
      p.prefixlen = ip_masklen (*mask);
      apply_mask_ipv4 (&p);

      cli_out (cli, " %P", &p);
      break;
    case OSPF_AS_EXTERNAL_LSA:
#ifdef HAVE_NSSA
    case OSPF_AS_NSSA_LSA:
#endif /* HAVE_NSSA */
      mask = 
        OSPF_PNT_IN_ADDR_GET (lsa->data, OSPF_EXTERNAL_LSA_NETMASK_OFFSET);
      e_bit = OSPF_PNT_UCHAR_GET (lsa->data,
                                  OSPF_EXTERNAL_LSA_E_BIT_OFFSET);
      tag = OSPF_PNT_UINT32_GET (lsa->data,
                                 OSPF_EXTERNAL_LSA_TAG_OFFSET);

      p.family = AF_INET;
      p.prefix = lsa->data->id;
      p.prefixlen = ip_masklen (*mask);
      apply_mask_ipv4 (&p);

      cli_out (cli, " %s %-15P %u",
               IS_EXTERNAL_METRIC (e_bit) ? "E2" : "E1", &p, tag);
      break;
    case OSPF_NETWORK_LSA:
    case OSPF_SUMMARY_LSA_ASBR:
      break;
#ifdef HAVE_OPAQUE_LSA
    case OSPF_LINK_OPAQUE_LSA:
    case OSPF_AREA_OPAQUE_LSA:
    case OSPF_AS_OPAQUE_LSA:
      cli_out (cli, " %u", OPAQUE_ID (lsa->data->id));
      break;
#endif /* HAVE_OPAQUE_LSA */
    default:
      break;
    }
  /* cli_out (cli, " L=%d", lsa->lock); XXX */
  cli_out (cli, "\n");

  return 0;
}

void
show_ip_ospf_database_header (struct cli *cli, struct ospf_lsa *lsa)
{
  char option_str[OSPF_OPTION_STR_MAXLEN];
  u_char bits;

  cli_out (cli, "  LS age: %d\n", LS_AGE (lsa));
  cli_out (cli, "  Options: 0x%x (%s)\n", lsa->data->options,
           ospf_option_dump (lsa->data->options, option_str));

  if (lsa->data->type == OSPF_ROUTER_LSA)
    {
      bits = OSPF_PNT_UCHAR_GET (lsa->data, OSPF_ROUTER_LSA_BITS_OFFSET);

      cli_out (cli, "  Flags: 0x%x" , bits);

      if (bits)
        cli_out (cli, " :%s%s%s%s%s",
                 IS_ROUTER_LSA_SET (bits, BIT_B) ? " ABR" : "",
                 IS_ROUTER_LSA_SET (bits, BIT_E) ? " ASBR" : "",
                 IS_ROUTER_LSA_SET (bits, BIT_V) ? " VL-endpoint" : "",
                 IS_ROUTER_LSA_SET (bits, BIT_S) ? " Shortcut" : "",
                 IS_ROUTER_LSA_SET (bits, BIT_NT) ? " NSSA-Translator" : "");
      cli_out (cli, "\n");
    }
  cli_out (cli, "  LS Type: %s\n",
           LOOKUP (ospf_lsa_type_msg, lsa->data->type));
  cli_out (cli, "  Link State ID: %r %s\n", &lsa->data->id,
           LOOKUP (ospf_link_state_id_type_msg, lsa->data->type));
#ifdef HAVE_OPAQUE_LSA
  if (IS_OPAQUE_LSA (lsa->data))
      cli_out (cli, "  Opaque Type: %u\n  Opaque ID: %u\n",
               OPAQUE_TYPE (lsa->data->id), OPAQUE_ID (lsa->data->id));
#endif /* HAVE_OPAQUE_LSA */
  cli_out (cli, "  Advertising Router: %r\n", &lsa->data->adv_router);
  cli_out (cli, "  LS Seq Number: %08x\n", pal_ntoh32 (lsa->data->ls_seqnum));
  cli_out (cli, "  Checksum: 0x%04x\n", pal_ntoh16 (lsa->data->checksum));
  cli_out (cli, "  Length: %d\n", pal_ntoh16 (lsa->data->length));
}

char *link_type_desc[] =
{
  "(null)",
  "another Router (point-to-point)",
  "a Transit Network",
  "Stub Network",
  "a Virtual Link",
};

char *link_id_desc[] =
{
  "(null)",
  "Neighboring Router ID",
  "Designated Router address",
  "Network/subnet number",
  "Neighboring Router ID",
};

char *link_data_desc[] =
{
  "(null)",
  "Router Interface address",
  "Router Interface address",
  "Network Mask",
  "Router Interface address",
};

/* Show router-LSA detail information. */
int
show_router_lsa_detail (struct cli *cli, struct ospf_lsa *lsa)
{
  struct ospf_link_desc link;
  u_int16_t links;
  u_char *p;
  u_char *lim;

  links = OSPF_PNT_UINT16_GET (lsa->data,
                               OSPF_ROUTER_LSA_NUM_LINKS_OFFSET);

  show_ip_ospf_database_header (cli, lsa);
          
  cli_out (cli, "   Number of Links: %d\n\n", links);

  p = (u_char *)lsa->data + OSPF_ROUTER_LSA_LINK_DESC_OFFSET;
  lim = (u_char *)lsa->data + pal_ntoh16 (lsa->data->length);

  while (p + OSPF_LINK_DESC_SIZE <= lim)
    {
      OSPF_LINK_DESC_GET (p, link);

      p += OSPF_LINK_DESC_SIZE + OSPF_LINK_TOS_SIZE * link.num_tos;

      cli_out (cli, "    Link connected to: %s\n",
               link_type_desc[link.type]);
      cli_out (cli, "     (Link ID) %s: %r\n",
               link_id_desc[link.type], link.id);
      cli_out (cli, "     (Link Data) %s: %r\n",
               link_data_desc[link.type], link.data);
      cli_out (cli, "      Number of TOS metrics: %d\n", link.num_tos);
      cli_out (cli, "       TOS 0 Metric: %d\n", link.metric);
      cli_out (cli, "\n");
    }

  return 0;
}

/* Show network-LSA detail information. */
int
show_network_lsa_detail (struct cli *cli, struct ospf_lsa *lsa)
{
  struct pal_in4_addr *mask;
  struct pal_in4_addr *nbr_id;
  u_char *lim, *p;

  mask = OSPF_PNT_IN_ADDR_GET (lsa->data, OSPF_NETWORK_LSA_NETMASK_OFFSET);
  p = (u_char *)lsa->data + OSPF_NETWORK_LSA_ROUTERS_OFFSET;
  lim = (u_char *)lsa->data + pal_ntoh16 (lsa->data->length);

  show_ip_ospf_database_header (cli, lsa);

  cli_out (cli, "  Network Mask: /%d\n", ip_masklen (*mask));

  while (p < lim)
    {
      nbr_id = (struct pal_in4_addr *)p;

      cli_out (cli, "        Attached Router: %r\n", nbr_id);

      p += sizeof (struct pal_in4_addr);
    }

  cli_out (cli, "\n");

  return 0;
}

/* Show summary-LSA detail information. */
int
show_summary_lsa_detail (struct cli *cli, struct ospf_lsa *lsa)
{
  struct pal_in4_addr *mask;
  u_int32_t metric;

  mask = OSPF_PNT_IN_ADDR_GET (lsa->data, OSPF_SUMMARY_LSA_NETMASK_OFFSET);
  metric = OSPF_PNT_UINT24_GET (lsa->data, OSPF_SUMMARY_LSA_METRIC_OFFSET);

  show_ip_ospf_database_header (cli, lsa);

  cli_out (cli, "  Network Mask: /%d\n", ip_masklen (*mask));
  cli_out (cli, "        TOS: 0  Metric: %d\n", metric);
  cli_out (cli, "\n"); 

  return 0;
}

/* Show summary-ASBR-LSA detail information. */
int
show_summary_asbr_lsa_detail (struct cli *cli, struct ospf_lsa *lsa)
{
  struct pal_in4_addr *mask;
  u_int32_t metric;

  mask = OSPF_PNT_IN_ADDR_GET (lsa->data, OSPF_SUMMARY_LSA_NETMASK_OFFSET);
  metric = OSPF_PNT_UINT24_GET (lsa->data, OSPF_SUMMARY_LSA_METRIC_OFFSET);

  show_ip_ospf_database_header (cli, lsa);

  cli_out (cli, "  Network Mask: /%d\n", ip_masklen (*mask));
  cli_out (cli, "        TOS: 0  Metric: %d\n", metric);
  cli_out (cli, "\n"); 

  return 0;
}

/* Show AS-external-LSA detail information. */
int
show_as_external_lsa_detail (struct cli *cli, struct ospf_lsa *lsa)
{
  struct pal_in4_addr *mask =
    OSPF_PNT_IN_ADDR_GET (lsa->data, OSPF_EXTERNAL_LSA_NETMASK_OFFSET);
  struct pal_in4_addr *nexthop =
    OSPF_PNT_IN_ADDR_GET (lsa->data, OSPF_EXTERNAL_LSA_NEXTHOP_OFFSET);
  u_int32_t metric =
    OSPF_PNT_UINT24_GET (lsa->data, OSPF_EXTERNAL_LSA_METRIC_OFFSET);
  u_int32_t tag =
    OSPF_PNT_UINT32_GET (lsa->data, OSPF_EXTERNAL_LSA_TAG_OFFSET);
  u_char e_bit =
    OSPF_PNT_UCHAR_GET (lsa->data, OSPF_EXTERNAL_LSA_E_BIT_OFFSET);

  show_ip_ospf_database_header (cli, lsa);

  cli_out (cli, "  Network Mask: /%d\n", ip_masklen (*mask));
  cli_out (cli, "        Metric Type: %s\n",
           IS_EXTERNAL_METRIC (e_bit) ?
           "2 (Larger than any link state path)" : "1");
  cli_out (cli, "        TOS: 0\n");
  cli_out (cli, "        Metric: %d\n", metric);
  cli_out (cli, "        Forward Address: %r\n", nexthop);
  cli_out (cli, "        External Route Tag: %u\n\n", tag);

  return 0;
}

#ifdef HAVE_NSSA
/* Show AS-NSSA-LSA detail information. */
int
show_as_nssa_lsa_detail (struct cli *cli, struct ospf_lsa *lsa)
{
  struct pal_in4_addr *mask =
    OSPF_PNT_IN_ADDR_GET (lsa->data, OSPF_EXTERNAL_LSA_NETMASK_OFFSET);
  struct pal_in4_addr *nexthop =
    OSPF_PNT_IN_ADDR_GET (lsa->data, OSPF_EXTERNAL_LSA_NEXTHOP_OFFSET);
  u_int32_t metric =
    OSPF_PNT_UINT24_GET (lsa->data, OSPF_EXTERNAL_LSA_METRIC_OFFSET);
  u_int32_t tag =
    OSPF_PNT_UINT32_GET (lsa->data, OSPF_EXTERNAL_LSA_TAG_OFFSET);
  u_char e_bit =
    OSPF_PNT_UCHAR_GET (lsa->data, OSPF_EXTERNAL_LSA_E_BIT_OFFSET);

  show_ip_ospf_database_header (cli, lsa);

  cli_out (cli, "  Network Mask: /%d\n", ip_masklen (*mask));
  cli_out (cli, "        Metric Type: %s\n",
           IS_EXTERNAL_METRIC (e_bit) ?
           "2 (Larger than any link state path)" : "1");
  cli_out (cli, "        TOS: 0\n");
  cli_out (cli, "        Metric: %d\n", metric);
  cli_out (cli, "        NSSA: Forward Address: %r\n", nexthop);
  cli_out (cli, "        External Route Tag: %u\n\n", tag);

  return 0;
}
#endif /* HAVE_NSSA */

#ifdef HAVE_OPAQUE_LSA
vector ospf_show_opaque_vec;

void
ospf_show_opaque_set (u_int32_t type,
                      int (*show_func) (struct cli *, struct opaque_lsa *))
{
  vector_set_index (ospf_show_opaque_vec, type, show_func);
}

int
show_opaque_lsa_detail (struct cli *cli, struct ospf_lsa *lsa)
{
  int (*show_func) (struct cli *, struct opaque_lsa *);
  u_int32_t type = OPAQUE_TYPE (lsa->data->id);

  show_ip_ospf_database_header (cli, lsa);

  if (type < vector_max (ospf_show_opaque_vec))
    if ((show_func = vector_slot (ospf_show_opaque_vec, type)))
      (*show_func) (cli, (struct opaque_lsa *)lsa->data);

  return 0;
}
#endif /* HAVE_OPAQUE_LSA */

#ifdef HAVE_RESTART
void
ospf_show_grace_lsa_grace_period (struct cli *cli, u_char *p)
{
  u_int32_t seconds;

  seconds = OSPF_PNT_UINT32_GET (p, 0);

  cli_out (cli, "      Grace Period: %d\n", seconds);
}

void
ospf_show_grace_lsa_restart_reason (struct cli *cli, u_char *p)
{
  u_char reason;

  reason = OSPF_PNT_UCHAR_GET (p, 0);

  cli_out (cli, "      Graceful Restart Reason: ");

  switch (reason)
    {
    case OSPF_RESTART_REASON_RESTART:
      cli_out (cli, "Software Restart");
      break;
    case OSPF_RESTART_REASON_UPGRADE:
      cli_out (cli, "Software Reload/Upgrade");
      break;
    case OSPF_RESTART_REASON_SWITCH_REDUNDANT:
      cli_out (cli, "Switch to redundant control processor");
      break;
    case OSPF_RESTART_REASON_UNKNOWN:
    default:
      cli_out (cli, "Unknown");
      break;
    }
  cli_out (cli, "\n");
}

void
ospf_show_grace_lsa_ip_interface_address (struct cli *cli, u_char *p)
{
  cli_out (cli, "      IP Interface Address: %r\n", p);
}

int
ospf_show_opaque_grace_lsa (struct cli *cli, struct opaque_lsa *lsa)
{
  u_int16_t len = OPAQUE_DATA_LENGTH (lsa);
  u_char *p = lsa->opaque_data;
  u_char *lim = p + len;
  u_int16_t tlv_type;
  u_int16_t tlv_len;

  while (p < lim)
    {
      tlv_type = OSPF_PNT_UINT16_GET (p, 0);
      tlv_len = OSPF_PNT_UINT16_GET (p, 2);

      switch (tlv_type)
        {
        case OSPF_GRACE_LSA_GRACE_PERIOD:
          ospf_show_grace_lsa_grace_period (cli, p + 4);
          break;
        case OSPF_GRACE_LSA_RESTART_REASON:
          ospf_show_grace_lsa_restart_reason (cli, p + 4);
          break;
        case OSPF_GRACE_LSA_IP_INTERFACE_ADDRESS:
          ospf_show_grace_lsa_ip_interface_address (cli, p + 4);
          break;
        }

      p += OSPF_TLV_SPACE (tlv_len);
    }

  return 0;
}
#endif /* HAVE_RESTART */

int (*ospf_show_function[])(struct cli *, struct ospf_lsa *) =
{
  NULL,
  show_router_lsa_detail,
  show_network_lsa_detail,
  show_summary_lsa_detail,
  show_summary_asbr_lsa_detail,
  show_as_external_lsa_detail,
#ifdef HAVE_NSSA
  NULL,
  show_as_nssa_lsa_detail,
#endif /* HAVE_NSSA */
#ifdef HAVE_OPAQUE_LSA
#ifndef HAVE_NSSA
  NULL,
  NULL,
#endif /* HAVE_NSSA */
  NULL, 
  show_opaque_lsa_detail,
  show_opaque_lsa_detail,
  show_opaque_lsa_detail,
#endif /* HAVE_OPAQUE_LSA */
};

int
ospf_show_database_detail (struct cli *cli, struct ospf_lsa *lsa,
                           struct ospf_show_database_arg *arg)
{
  u_char type = arg->type;

  ospf_show_database_header (cli, lsa, arg);

  if (type >= OSPF_MIN_LSA
      && type < OSPF_MAX_LSA)
    if (ospf_show_function[type])
      ospf_show_function[type] (cli, lsa);

  return 0;
}

struct ospf *
ospf_show_database_proc_get_next (struct cli *cli,
                                  struct ospf_show_database_arg *arg)
{
  struct ospf_master *om = cli->vr->proto;
  struct ospf *top;
  struct listnode *node;

  LIST_LOOP (om->ospf, top, node)
    if (arg->proc_id < 0
        || arg->proc_id == top->ospf_id)
      if (top->ospf_id > arg->st.proc_id)
        {
          arg->st.proc_id = top->ospf_id;
          SET_FLAG (arg->show_flags, OSPF_SHOW_DATABASE_HEADER_PROC);
          return top;
        }

  return NULL;
}

struct ospf *
ospf_show_database_proc_get (struct cli *cli,
                             struct ospf_show_database_arg *arg)
{
  struct ospf_master *om = cli->vr->proto;
  struct ospf *top;

  top = ospf_proc_lookup (om, arg->st.proc_id);
  if (top != NULL)
    return top;

  return ospf_show_database_proc_get_next (cli, arg);
}

struct ospf_interface *ospf_if_entry_lookup_next (struct ospf *,
                                                  struct pal_in4_addr *,
                                                  int *, int);
struct ospf_vlink *ospf_vlink_entry_lookup_next (struct ospf *,
                                                 struct pal_in4_addr *,
                                                 struct pal_in4_addr *, int);
struct ospf_area *ospf_area_entry_lookup_next (struct ospf *,
                                               struct pal_in4_addr *, int);

struct ospf_lsdb *
ospf_show_database_lsdb_get_next (struct cli *cli,
                                  struct ospf_show_database_arg *arg,
                                  struct ospf *top)
{
#ifdef HAVE_OPAQUE_LSA
  struct ospf_interface *oi;
  struct ospf_vlink *vlink;
#endif /* HAVE_OPAQUE_LSA */
  struct ospf_area *area;

  switch (arg->st.state)
    {
    case OSPF_SHOW_DATABASE_STATE_INIT:
      arg->st.state = OSPF_SHOW_DATABASE_STATE_LINK;
#ifdef HAVE_OPAQUE_LSA
    case OSPF_SHOW_DATABASE_STATE_LINK:
      oi = ospf_if_entry_lookup_next (top, &arg->st.u.i.addr,
                                      &arg->st.u.i.ifindex, arg->st.indexlen);
      arg->st.indexlen = 5;
      if (oi != NULL)
        return oi->lsdb;

      arg->st.indexlen = 0;
      arg->st.state = OSPF_SHOW_DATABASE_STATE_VLINK;
    case OSPF_SHOW_DATABASE_STATE_VLINK:
      vlink = ospf_vlink_entry_lookup_next (top, &arg->st.u.v.area_id,
                                            &arg->st.u.v.nbr_id,
                                            arg->st.indexlen);
      arg->st.indexlen = 8;
      if (vlink != NULL)
        if ((oi = vlink->oi))
          return oi->lsdb;

      arg->st.indexlen = 0;
      arg->st.state = OSPF_SHOW_DATABASE_STATE_AREA;
#endif /* HAVE_OPAQUE_LSA */
    case OSPF_SHOW_DATABASE_STATE_AREA:
      area = ospf_area_entry_lookup_next (top, &arg->st.u.area_id,
                                          arg->st.indexlen);
      arg->st.indexlen = 4;
      if (area != NULL)
        return area->lsdb;

      arg->st.indexlen = 0;

      arg->st.state = OSPF_SHOW_DATABASE_STATE_AS;
      return top->lsdb;
    }

  arg->st.state = OSPF_SHOW_DATABASE_STATE_INIT;
  return NULL;
}

struct ospf_lsdb *
ospf_show_database_lsdb_get (struct cli *cli,
                             struct ospf_show_database_arg *arg,
                             struct ospf *top)
{
#ifdef HAVE_OPAQUE_LSA
  struct ospf_interface *oi;
  struct ospf_vlink *vlink;
#endif /* HAVE_OPAQUE_LSA */
  struct ospf_area *area;

  switch (arg->st.state)
    {
    case OSPF_SHOW_DATABASE_STATE_INIT:
      break;
#ifdef HAVE_OPAQUE_LSA
    case OSPF_SHOW_DATABASE_STATE_LINK:
      oi = ospf_if_entry_lookup (top, arg->st.u.i.addr,
                                 arg->st.u.i.ifindex);
      if (oi != NULL)
        return oi->lsdb;
      break;
    case OSPF_SHOW_DATABASE_STATE_VLINK:
      vlink = ospf_vlink_entry_lookup (top, arg->st.u.v.area_id,
                                       arg->st.u.v.nbr_id);
      if (vlink != NULL)
        if ((oi = vlink->oi))
          return oi->lsdb;

      break;
#endif /* HAVE_OPAQUE_LSA */
    case OSPF_SHOW_DATABASE_STATE_AREA:
      area = ospf_area_entry_lookup (top, arg->st.u.area_id);
      if (area != NULL)
        return area->lsdb;

      break;
    case OSPF_SHOW_DATABASE_STATE_AS:
      return top->lsdb;
    }

  return ospf_show_database_lsdb_get_next (cli, arg, top);
}

struct ls_table *
ospf_show_database_table_get_next (struct cli *cli,
                                   struct ospf_show_database_arg *arg,
                                   struct ospf_lsdb *lsdb)
{
  struct ls_table *table;
  int type = arg->st.type;

  type++;
  for (; type < OSPF_MAX_LSA; type++)
    if (arg->type == 0
        || arg->type == type)
      if ((table = lsdb->type[type].db))
        {
          arg->st.type = type;
          SET_FLAG (arg->show_flags, OSPF_SHOW_DATABASE_HEADER_TYPE);

          if (arg->type == 0)
            SET_FLAG (arg->show_flags, OSPF_SHOW_DATABASE_HEADER_BRIEF);
          return table;
        }

  arg->st.type = 0;
  return NULL;
}

struct ls_table *
ospf_show_database_table_get (struct cli *cli,
                              struct ospf_show_database_arg *arg,
                              struct ospf_lsdb *lsdb)
{
  struct ls_table *table;

  if (arg->st.type != 0)
    {
      table = lsdb->type[arg->st.type].db;
      if (table != NULL)
        return table;
    }

  return ospf_show_database_table_get_next (cli, arg, lsdb);
}

struct ospf_lsa *
ospf_show_database_lsa_get_next (struct cli *cli,
                                 struct ospf_show_database_arg *arg,
                                 struct ls_table *table)
{
  struct ospf_lsa *lsa = arg->st.lsa;
  struct ospf_lsa *next;
  struct ls_node *rn;
  struct ls_prefix_ls lp;

  if (lsa == NULL)
    {
      rn = ls_node_lookup_first (table);
      if (rn != NULL)
        {
          next = RN_INFO (rn, RNI_LSDB_LSA);
          ls_unlock_node (rn);
          arg->st.lsa = next;
          return ospf_lsa_lock (next);
        }
    }  
  else
    {
      ospf_lsa_unlock (lsa);

      lsdb_prefix_set (&lp, lsa->data);
      rn = ls_node_get (table, (struct ls_prefix *)&lp);
      for (rn = ls_route_next (rn); rn; rn = ls_route_next (rn))
        if ((next = RN_INFO (rn, RNI_LSDB_LSA)))
          {
            ls_unlock_node (rn);
            arg->st.lsa = next;
            return ospf_lsa_lock (next);
          }
    }
  arg->st.lsa = NULL;

  return NULL;
}

int
ospf_show_database_check (struct ospf_show_database_arg *arg,
                          struct ospf_lsa *lsa)
{
  if (CHECK_FLAG (arg->flags, OSPF_SHOW_DATABASE_SELF))
    if (!IS_LSA_SELF (lsa))
      return 0;

  if (CHECK_FLAG (arg->flags, OSPF_SHOW_DATABASE_MAXAGE))
    if (!IS_LSA_MAXAGE (lsa))
      return 0;

  if (CHECK_FLAG (arg->flags, OSPF_SHOW_DATABASE_ADV_ROUTER))
    if (!IPV4_ADDR_SAME (&lsa->data->adv_router, &arg->adv_router))
      return 0;

  if (CHECK_FLAG (arg->flags, OSPF_SHOW_DATABASE_ID))
    if (!IPV4_ADDR_SAME (&lsa->data->id, &arg->id))
      return 0;

  return 1;
}

int
ospf_show_database_callback (struct cli *cli)
{
  struct ospf_show_database_arg *arg = cli->arg;
  struct ospf_lsdb *lsdb;
  struct ospf_lsa *lsa;
  struct ospf *top;
  struct ls_table *table;
  int count = 0;

  if (cli->status == CLI_CLOSE)
    goto cleanup;

  top = ospf_show_database_proc_get (cli, arg);
  while (top != NULL)
    {
      lsdb = ospf_show_database_lsdb_get (cli, arg, top);

      while (lsdb != NULL)
        {
          table = ospf_show_database_table_get (cli, arg, lsdb);

          while (table != NULL)
            {
              do {
                lsa = ospf_show_database_lsa_get_next (cli, arg, table);
                if (lsa == NULL)
                  break;

                if (!ospf_show_database_check (arg, lsa))
                  continue;

                if (arg->show)
                  (*arg->show) (cli, lsa, arg);

                if (++count == arg->max_count)
                  {
                    cli->status = CLI_CONTINUE;
                    return 0;
                  }

              } while (1);

              table = ospf_show_database_table_get_next (cli, arg, lsdb);
            }

          lsdb = ospf_show_database_lsdb_get_next (cli, arg, top);
        }

      top = ospf_show_database_proc_get_next (cli, arg);
    }

  /* Clean up. */
 cleanup:

  if (arg)
    XFREE (MTYPE_TMP, arg);

  if (cli->cleanup)
    (*cli->cleanup) (cli);

  /* Set NULL to callback pointer. */
  cli->status = CLI_CONTINUE;
  cli->callback = NULL;
  cli->arg = NULL;

  return 0;
}


struct ospf_show_route_arg
{
  /* Max line per callback. */
  int max_count;

  /* Show flags. */
  int show_flags;
#define OSPF_SHOW_ROUTE_HEADER_PROC     (1 << 0)

  /* CLI argument. */
  int proc_id;

  /* Current status. */
  struct
  {
    /* Flags. */
    int flags;
#define OSPF_SHOW_ROUTE_STATE_INIT      (1 << 0)

    /* Process ID. */
    int proc_id;

    /* Prefix. */
    struct ls_prefix p;

  } st;
};

void
ospf_show_route (struct cli *cli, struct ospf_route *route,
                 struct ospf_path *path, struct ospf_show_route_arg *arg)
{
  struct ls_prefix *p = &arg->st.p;
  struct ospf_nexthop *nh;
  char buf[44];
  vector vec;
  int count = 0;
  int c1;
  int c2 = -1;
  int len;
  int i;

  if (CHECK_FLAG (arg->show_flags, OSPF_SHOW_ROUTE_HEADER_PROC))
    {
      cli_out (cli, "\n");
      cli_out (cli, "OSPF process %d:\n", arg->st.proc_id);
  
      cli_out (cli, "Codes: C - connected, D - Discard, "
               "O - OSPF, IA - OSPF inter area\n");
      cli_out (cli, "       N1 - OSPF NSSA external type 1, "
               "N2 - OSPF NSSA external type 2\n");
      cli_out (cli, "       E1 - OSPF external type 1, "
               "E2 - OSPF external type 2\n\n");

      UNSET_FLAG (arg->show_flags, OSPF_SHOW_ROUTE_HEADER_PROC);
    }

  if (path->type == OSPF_PATH_DISCARD)
    {
      c1 = ospf_path_code (path, NULL);
      cli_out (cli, "%-2s %P [-] via Null0\n",
               ospf_path_code_str[c1], p);
    }
  else
    {
      if (CHECK_FLAG (path->flags, OSPF_PATH_TYPE2))
        len = zsnprintf (buf, 44, "%P [%lu/%lu] ",
                         p, OSPF_PATH_COST (path), path->type2_cost);
      else
        len = zsnprintf (buf, 44, "%P [%lu] ", p, OSPF_PATH_COST (path));

      /* Get the actual nexthop vector.  */
      vec = ospf_path_nexthop_vector (path);

      for (i = 0; i < vector_max (vec); i++)
        if ((nh = vector_slot (vec, i)))
          {
            c1 = ospf_path_code (path, nh);

            cli_out (cli, "%-2s %*s",
                     c1 != c2 ? ospf_path_code_str[c1] : "",
                     len, count == 0 ? buf : "");

            if (CHECK_FLAG (nh->flags, OSPF_NEXTHOP_CONNECTED))
              cli_out (cli, "is directly connected, ");
            else
              cli_out (cli, "via %r, ", &nh->nbr_id);

            cli_out (cli, "%s", nh->oi->u.ifp->name);

            if (c1 != OSPF_PATH_CODE_E1
                && c1 != OSPF_PATH_CODE_E2)
              cli_out (cli, ", %sArea %r",
                       IS_AREA_TRANSIT (nh->oi->area) ? "Transit" : "",
                       &nh->oi->area->area_id);

            cli_out (cli, "\n");

            c2 = c1;
            count++;
          }
      vector_free (vec);
    }
}

struct ospf *
ospf_show_route_proc_get_next (struct cli *cli,
                               struct ospf_show_route_arg *arg)
{
  struct ospf_master *om = cli->vr->proto;
  struct ospf *top;
  struct listnode *node;

  LIST_LOOP (om->ospf, top, node)
    if (arg->proc_id < 0
        || arg->proc_id == top->ospf_id)
      if (top->ospf_id > arg->st.proc_id)
        {
          arg->st.proc_id = top->ospf_id;
          SET_FLAG (arg->show_flags, OSPF_SHOW_ROUTE_HEADER_PROC);
          return top;
        }

  return NULL;
}

struct ospf *
ospf_show_route_proc_get (struct cli *cli, struct ospf_show_route_arg *arg)
{
  struct ospf_master *om = cli->vr->proto;
  struct ospf *top;

  top = ospf_proc_lookup (om, arg->st.proc_id);
  if (top != NULL)
    return top;

  return ospf_show_route_proc_get_next (cli, arg);
}

struct ospf_route *
ospf_show_route_get_next (struct cli *cli,
                          struct ospf_show_route_arg *arg, struct ospf *top)
{
  struct ospf_route *next;
  struct ls_node *rn;

  if (CHECK_FLAG (arg->st.flags, OSPF_SHOW_ROUTE_STATE_INIT))
    {
      UNSET_FLAG (arg->st.flags, OSPF_SHOW_ROUTE_STATE_INIT);
      
      rn = ls_node_lookup_first (top->rt_network); 
      if (rn != NULL)
        {
          next = RN_INFO (rn, RNI_DEFAULT);
          ls_unlock_node (rn);
          arg->st.p = *rn->p;

          return next;
        }
    }
  else
    {
      rn = ls_node_get (top->rt_network, &arg->st.p);
      for (rn = ls_route_next (rn); rn; rn = ls_route_next (rn))
        if ((next = RN_INFO (rn, RNI_DEFAULT)))
          {
            ls_unlock_node (rn);
            arg->st.p = *rn->p;

            return next;
          }
    }

  pal_mem_set (&arg->st.p, 0, sizeof (struct ls_prefix));
  SET_FLAG (arg->st.flags, OSPF_SHOW_ROUTE_STATE_INIT);

  return NULL;
}

int
ospf_show_route_callback (struct cli *cli)
{
  struct ospf_show_route_arg *arg = cli->arg;
  struct ospf_route *route;
  struct ospf *top;
  int count = 0;

  if (cli->status == CLI_CLOSE)
    goto cleanup;

  top = ospf_show_route_proc_get (cli, arg);
  while (top != NULL)
    {
      while ((route = ospf_show_route_get_next (cli, arg, top)))
        if (route != NULL)
          if (route->selected != NULL)
            {
              ospf_show_route (cli, route, route->selected, arg);
              if (++count == arg->max_count)
                {
                  cli->status = CLI_CONTINUE;
                  return 0;
                }
            }

      top = ospf_show_route_proc_get_next (cli, arg);
    }

  /* Clean up. */
 cleanup:

  if (arg)
    XFREE (MTYPE_TMP, arg);

  if (cli->cleanup)
    (*cli->cleanup) (cli);

  /* Set NULL to callback pointer. */
  cli->status = CLI_CONTINUE;
  cli->callback = NULL;
  cli->arg = NULL;

  return 0;
}

void
ospf_cli_show_abr_by_area (struct cli *cli, struct ospf_area *area)
{
  struct ospf_vertex *v;
  struct ospf_nexthop *nh;
  struct ls_node *rn;
  u_char bits;
  int count;
  int len;
  int i;
  char buf[31];

  for (rn = ls_table_top (area->vertices); rn; rn = ls_route_next (rn))
    if ((v = RN_INFO (rn, RNI_VERTEX_INDEX (OSPF_ROUTER_LSA))))
      {
        bits = OSPF_PNT_UCHAR_GET (v->lsa->data, OSPF_ROUTER_LSA_BITS_OFFSET);
        if (IS_ROUTER_LSA_SET (bits, BIT_B)
            && !IS_ROUTER_LSA_SET (bits, BIT_E))
          {
            count = 0;
            len = zsnprintf (buf, 31, "i %r [%d] ",
                             &v->lsa->data->id, v->distance);

            for (i = 0; i < vector_max (v->nexthop); i++)
              if ((nh = vector_slot (v->nexthop, i)))
                {
                  cli_out (cli, "%*s", len, count == 0 ? buf : "");

                  if (CHECK_FLAG (nh->flags, OSPF_NEXTHOP_INVALID))
                    cli_out (cli, "(incomplete), ABR");
                  else
                    {
                      if (nh->nbr_id.s_addr == 0)
                        cli_out (cli, "is directly connected, ");
                      else
                        cli_out (cli, "via %r, ", &nh->nbr_id.s_addr);

                      if (CHECK_FLAG (nh->flags, OSPF_NEXTHOP_VIA_TRANSIT))
                        cli_out (cli, "through TransitArea %r, ABR, "
                                 "Area 0.0.0.0", &nh->if_id);
                      else
                        cli_out (cli, "%s, ABR, %sArea %r",
                                 nh->oi->u.ifp->name,
                                 IS_AREA_TRANSIT (nh->oi->area)
                                 ? "Transit" : "", &nh->oi->area->area_id);
                    }

                  cli_out (cli, "\n");
                  count++;
                }
          }
      }   
}

void
ospf_cli_show_abr_all (struct cli *cli, struct ospf *top)
{
  struct ospf_area *area;
  struct ls_node *rn;

  for (rn = ls_table_top (top->area_table); rn; rn = ls_route_next (rn))
    if ((area = RN_INFO (rn, RNI_DEFAULT)))
      if (IS_AREA_ACTIVE (area))
        ospf_cli_show_abr_by_area (cli, area);
}

void
ospf_cli_show_asbr_all (struct cli *cli, struct ospf *top)
{
  struct ospf_route *route;
  struct ospf_path *path;
  struct ospf_nexthop *nh;
  struct ls_node *rn;
  int i, j;
  int count;
  int len;
  char buf[31];

  for (rn = ls_table_top (top->rt_asbr); rn; rn = ls_route_next (rn))
    if ((route = RN_INFO (rn, RNI_DEFAULT)))
      {
        for (i = 0; i < vector_max (route->paths); i++)
          if ((path = vector_slot (route->paths, i)))
            {
              count = 0;
              len = zsnprintf (buf, 31, "%c %r [%d] ",
                               path->type == OSPF_PATH_INTRA_AREA ? 'i' : 'I',
                               rn->p->prefix, OSPF_PATH_COST (path));

              for (j = 0; j < vector_max (OSPF_PATH_NEXTHOPS (path)); j++)
                if ((nh = vector_slot (OSPF_PATH_NEXTHOPS (path), j)))
                  {
                    cli_out (cli, "%*s", len, count == 0 ? buf : "");

                    if (CHECK_FLAG (nh->flags, OSPF_NEXTHOP_INVALID))
                      cli_out (cli, "(incomplete)");
                    else
                      {
                        if (nh->nbr_id.s_addr == 0)
                          cli_out (cli, "is directly connected, ");
                        else
                          cli_out (cli, "via %r, ", &nh->nbr_id.s_addr);

                        if (CHECK_FLAG (nh->flags, OSPF_NEXTHOP_VIA_TRANSIT))
                          cli_out (cli, "through TransitArea %r, ",
                                   &nh->if_id);
                        else
                          cli_out (cli, "%s, ", nh->oi->u.ifp->name);

                        if (CHECK_FLAG (path->flags, OSPF_PATH_ABR))
                          cli_out (cli, "ABR, ");
                        if (CHECK_FLAG (path->flags, OSPF_PATH_ASBR))
                          cli_out (cli, "ASBR, ");

                        if (CHECK_FLAG (nh->flags, OSPF_NEXTHOP_VIA_TRANSIT))
                          cli_out (cli, "Area 0.0.0.0");
                        else
                          cli_out (cli, "%sArea %r",
                                   IS_AREA_TRANSIT (nh->oi->area)
                                   ? "Transit" : "", &nh->oi->area->area_id);
                      }

                    cli_out (cli, "\n");
                    count++;
                  }
            }
      }
}

void
ospf_cli_show_border_router (struct cli *cli, struct ospf *top)
{
  cli_out (cli, "\n");
  cli_out (cli, "OSPF process %d internal Routing Table\n\n", top->ospf_id);
  cli_out (cli, "Codes: i - Intra-area route, I - Inter-area route\n\n");

  ospf_cli_show_abr_all (cli, top);
  ospf_cli_show_asbr_all (cli, top);
}

void
ospf_cli_show_buffers (struct cli *cli, struct ospf *top)
{
  cli_out (cli, "OSPF process %d:\n", top->ospf_id);

  cli_out (cli, "  Packet RECV buffer: %d bytes\n",
           stream_get_size (top->ibuf));
  cli_out (cli, "  Packet SEND buffer: %d bytes\n",
           stream_get_size (top->obuf));
  cli_out (cli, "  LSA buffer: %d bytes\n",
           stream_get_size (top->lbuf));
  cli_out (cli, "  Packet unused list: %lu/%lu packets\n",
           top->op_unuse_count, top->op_unuse_max);
  cli_out (cli, "  LSA unused list: %lu/%lu LSAs\n",
           top->lsa_unuse_count, top->lsa_unuse_max);
  cli_out (cli, "\n");
}

CLI (show_debugging_ospf,
     show_debugging_ospf_cmd,
     "show debugging ospf",
     CLI_SHOW_STR,
     CLI_DEBUG_STR,
     CLI_OSPF_STR)
{
  struct ospf_master *om = cli->vr->proto;
  int i;

  cli_out (cli, "OSPF debugging status:\n");
  
  /* Show debug status for IFSM. */
  if (IS_DEBUG_OSPF (ifsm, IFSM) == OSPF_DEBUG_IFSM)
    cli_out (cli, "  OSPF all IFSM debugging is on\n");
  else
    {
      if (IS_DEBUG_OSPF (ifsm, IFSM_EVENTS))
        cli_out (cli, "  OSPF IFSM event debugging is on\n");
      if (IS_DEBUG_OSPF (ifsm, IFSM_STATUS))
        cli_out (cli, "  OSPF IFSM status debugging is on\n");
      if (IS_DEBUG_OSPF (ifsm, IFSM_TIMERS))
        cli_out (cli, "  OSPF IFSM timer debugging is on\n");
    }

  /* Show debug status for NFSM. */
  if (IS_DEBUG_OSPF (nfsm, NFSM) == OSPF_DEBUG_NFSM)
    cli_out (cli, "  OSPF all NFSM debugging is on\n");
  else
    {
      if (IS_DEBUG_OSPF (nfsm, NFSM_EVENTS))
        cli_out (cli, "  OSPF NFSM event debugging is on\n");
      if (IS_DEBUG_OSPF (nfsm, NFSM_STATUS))
        cli_out (cli, "  OSPF NFSM status debugging is on\n");
      if (IS_DEBUG_OSPF (nfsm, NFSM_TIMERS))
        cli_out (cli, "  OSPF NFSM timer debugging is on\n");
    }

  /* Show debug status for OSPF Packets. */
  for (i = OSPF_PACKET_HELLO; i < OSPF_PACKET_TYPE_MAX; i++)
    if (IS_DEBUG_OSPF_PACKET (i, SEND) && IS_DEBUG_OSPF_PACKET (i, RECV))
      {
        cli_out (cli, "  OSPF packet %s%s debugging is on\n",
                 ospf_packet_type_str[i],
                 IS_DEBUG_OSPF_PACKET (i, DETAIL) ? " detail" : "");
      }
    else
      {
        if (IS_DEBUG_OSPF_PACKET (i, SEND))
          cli_out (cli, "  OSPF packet %s send%s debugging is on\n",
                   ospf_packet_type_str[i],
                   IS_DEBUG_OSPF_PACKET (i, DETAIL) ? " detail" : "");
        else if (IS_DEBUG_OSPF_PACKET (i, RECV))
          cli_out (cli, "  OSPF packet %s receive%s debugging is on\n",
                   ospf_packet_type_str[i],
                   IS_DEBUG_OSPF_PACKET (i, DETAIL) ? " detail" : "");
      }

  /* Show debug status for OSPF LSAs. */
  if (IS_DEBUG_OSPF (lsa, LSA) == OSPF_DEBUG_LSA)
    cli_out (cli, "  OSPF all LSA debugging is on\n");
  else
    {
      if (IS_DEBUG_OSPF (lsa, LSA_FLOODING))
        cli_out (cli, "  OSPF LSA flooding debugging is on\n");
      if (IS_DEBUG_OSPF (lsa, LSA_GENERATE))
        cli_out (cli, "  OSPF LSA generation debugging is on\n");
      if (IS_DEBUG_OSPF (lsa, LSA_INSTALL))
        cli_out (cli, "  OSPF LSA installation debugging is on\n");
      if (IS_DEBUG_OSPF (lsa, LSA_MAXAGE))
        cli_out (cli, "  OSPF LSA MaxAge processing debugging is on\n");
      if (IS_DEBUG_OSPF (lsa, LSA_REFRESH))
        cli_out (cli, "  OSPF LSA refreshment debugging is on\n");
    }

  /* Show debug status for PacOS. */
  if (IS_DEBUG_OSPF (nsm, NSM) == OSPF_DEBUG_NSM)
    cli_out (cli, "  OSPF all NSM debugging is on\n");
  else
    {
      if (IS_DEBUG_OSPF (nsm, NSM_INTERFACE))
        cli_out (cli, "  OSPF NSM interface debugging is on\n");
      if (IS_DEBUG_OSPF (nsm, NSM_REDISTRIBUTE))
        cli_out (cli, "  OSPF NSM redistribute debugging is on\n");
    }

  if (IS_DEBUG_OSPF (event, EVENT) == OSPF_DEBUG_EVENT)
    cli_out (cli, "  OSPF all events debugging is on\n");
  else
    {
      if (IS_DEBUG_OSPF (event, EVENT_ABR))
        cli_out (cli, "  OSPF ABR events debugging is on\n");
      if (IS_DEBUG_OSPF (event, EVENT_ASBR))
        cli_out (cli, "  OSPF ASBR events debugging is on\n");
      if (IS_DEBUG_OSPF (event, EVENT_LSA))
        cli_out (cli, "  OSPF LSA events debugging is on\n");
      if (IS_DEBUG_OSPF (event, EVENT_NSSA))
        cli_out (cli, "  OSPF NSSA events debugging is on\n");
      if (IS_DEBUG_OSPF (event, EVENT_OS))
        cli_out (cli, "  OSPF OS interaction events debugging is on\n");
      if (IS_DEBUG_OSPF (event, EVENT_ROUTER))
        cli_out (cli, "  Other router events debugging is on\n");
      if (IS_DEBUG_OSPF (event, EVENT_VLINK))
        cli_out (cli, "  OSPF Virtual-Link events debugging is on\n");
    }

  if (IS_DEBUG_OSPF (rt_calc, RT_ALL) == OSPF_DEBUG_RT_ALL)
    cli_out (cli, "  OSPF all route calculation debugging is on\n");
  else
    {
      if (IS_DEBUG_OSPF (rt_calc, RT_CALC_ASE))
        cli_out (cli, "  OSPF External route calculation debugging is on\n");
      if (IS_DEBUG_OSPF (rt_calc, RT_CALC_IA))
        cli_out (cli, "  OSPF Inter-Area route calculation debugging is on\n");
      if (IS_DEBUG_OSPF (rt_calc, RT_INSTALL))
        cli_out (cli, "  OSPF route installation debugging is on\n");
      if (IS_DEBUG_OSPF (rt_calc, RT_CALC_SPF))
        cli_out (cli, "  OSPF SPF calculation debugging is on\n");
    }

#ifdef HAVE_BFD
  if (IS_DEBUG_OSPF (bfd, BFD) == OSPF_DEBUG_BFD)
    cli_out (cli, "  OSPF BFD debugging is on\n");
#endif /* HAVE_BFD */

  return CLI_SUCCESS;
}

CLI (show_ip_ospf,
     show_ip_ospf_cmd,
     "show ip ospf (<0-65535>|)",
     CLI_SHOW_STR,
     CLI_IP_STR,
     OSPF_CLI_OSPF_INFO_STR,
     OSPF_CLI_OSPF_PROCESS_ID_STR)
{
  struct ospf_master *om = cli->vr->proto;
  struct ospf *top;
  struct listnode *node;

  /* Check OSPF is enable. */
  if (list_isempty (om->ospf))
    {
      cli_out (cli, " OSPF Routing Process not enabled.\n");
      return CLI_SUCCESS;
    }

  if (argc > 0)
    {
      int id;

      CLI_GET_INTEGER_RANGE ("OSPF process ID", id, argv[0], 0, 65535);
      top = ospf_proc_lookup (om, id);
      if (top == NULL)
        {
          cli_out (cli, "%%OSPF: No router process %d\n", id);
          return CLI_ERROR;
        }

      ospf_cli_show_ospf (cli, top);
      return CLI_SUCCESS;
    }

  LIST_LOOP (om->ospf, top, node)
    ospf_cli_show_ospf (cli, top);

  return CLI_SUCCESS;
}

CLI (show_ip_ospf_interface,
     show_ip_ospf_interface_cmd,
     "show ip ospf interface (IFNAME|)",
     CLI_SHOW_STR,
     CLI_IP_STR,
     OSPF_CLI_OSPF_INFO_STR,
     "Interface information",
     "Interface name")
{
  struct apn_vr *vr = cli->vr;
  struct interface *ifp;
  struct listnode *node;

  if (argc == 0)
    {
      LIST_LOOP (vr->ifm.if_list, ifp, node)
        ospf_cli_show_interface (cli, ifp);
    }
  else
    {
      ifp = if_lookup_by_name (&vr->ifm, argv[0]);
      if (ifp == NULL)
        {
          cli_out (cli, "No such interface name\n");
          return CLI_ERROR;
        }
      ospf_cli_show_interface (cli, ifp);
    }

  return CLI_SUCCESS;
}

CLI (show_ip_ospf_virtual_links,
     show_ip_ospf_virtual_links_cmd,
     "show ip ospf (<0-65535>|) virtual-links",
     CLI_SHOW_STR,
     CLI_IP_STR,
     OSPF_CLI_OSPF_INFO_STR,
     OSPF_CLI_OSPF_PROCESS_ID_STR,
     "Virtual link information")
{
  struct ospf_master *om = cli->vr->proto;
  struct ospf *top;
  struct listnode *node;
  int id;

  if (argc > 0)
    {
      CLI_GET_INTEGER_RANGE ("OSPF process ID", id, argv[0], 0, 65535);
      top = ospf_proc_lookup (om, id);
      if (top == NULL)
        {
          cli_out (cli, "%%OSPF: No router process %d\n", id);
          return CLI_ERROR;
        }
      ospf_cli_show_virtual_links (cli, top);
    }
  else
    LIST_LOOP (om->ospf, top, node)
      ospf_cli_show_virtual_links (cli, top);

  return CLI_SUCCESS;
}
#ifdef HAVE_OSPF_MULTI_AREA
CLI (show_ip_ospf_multi_area_adjacencies,
     show_ip_ospf_multi_area_adjacencies_cmd,
     "show ip ospf (<0-65535>|) multi-area-adjacencies",
     CLI_SHOW_STR,
     CLI_IP_STR,
     OSPF_CLI_OSPF_INFO_STR,
     OSPF_CLI_OSPF_PROCESS_ID_STR,
     "Multi area adjacencies information")
{
  struct ospf_master *om = cli->vr->proto;
  struct ospf *top;
  struct listnode *node;
  int id;

  if (argc > 0)
    {
      CLI_GET_INTEGER_RANGE ("OSPF process ID", id, argv[0], 0, 65535);
      top = ospf_proc_lookup (om, id);
      if (top == NULL)
        {
          cli_out (cli, "%%OSPF: No router process %d\n", id);
          return CLI_ERROR;
        }
      ospf_cli_show_multi_area_links (cli, top);
    }
  else
    LIST_LOOP (om->ospf, top, node)
      ospf_cli_show_multi_area_links (cli, top);

  return CLI_SUCCESS;
}
#endif /* HAVE_OSPF_MULTI_AREA */
CLI (show_ip_ospf_neighbor,
     show_ip_ospf_neighbor_cmd,
     "show ip ospf (<0-65535>|) neighbor",
     CLI_SHOW_STR,
     CLI_IP_STR,
     OSPF_CLI_OSPF_INFO_STR,
     OSPF_CLI_OSPF_PROCESS_ID_STR,
     "Neighbor list")
{
  struct ospf_master *om = cli->vr->proto;
  struct ospf *top;
  struct listnode *node;
  int id;

  if (list_isempty (om->ospf))
    return CLI_SUCCESS;

  if (argc > 0)
    {
      CLI_GET_INTEGER_RANGE ("OSPF process ID", id, argv[0], 0, 65535);
      top = ospf_proc_lookup (om, id);
      if (top == NULL)
        {
          cli_out (cli, "%%OSPF: No router process %d\n", id);
          return CLI_ERROR;
        }

      ospf_cli_show_neighbor (cli, top, 0);
    }
  else
    {
      LIST_LOOP (om->ospf, top, node)
        ospf_cli_show_neighbor (cli, top, 0);
    }

  return CLI_SUCCESS;
}

CLI (show_ip_ospf_neighbor_private,
     show_ip_ospf_neighbor_private_cmd,
     "show ip ospf (<0-65535>|) neighbor private",
     CLI_SHOW_STR,
     CLI_IP_STR,
     OSPF_CLI_OSPF_INFO_STR,
     OSPF_CLI_OSPF_PROCESS_ID_STR,
     "Neighbor list",
     "Private information")
{
  struct ospf_master *om = cli->vr->proto;
  struct ospf *top;
  struct listnode *node;

  if (list_isempty (om->ospf))
    return CLI_SUCCESS;

  if (argc > 0)
    {
      int id;

      CLI_GET_INTEGER_RANGE ("OSPF process ID", id, argv[0], 0, 65535);
      top = ospf_proc_lookup (om, id);
      if (top == NULL)
        {
          cli_out (cli, "%%OSPF: No router process %d\n", id);
          return CLI_ERROR;
        }

      ospf_cli_show_neighbor (cli, top, 1);
    }
  else
    {
      LIST_LOOP (om->ospf, top, node)
        ospf_cli_show_neighbor (cli, top, 1);
    }

  return CLI_SUCCESS;
}

CLI (show_ip_ospf_neighbor_all,
     show_ip_ospf_neighbor_all_cmd,
     "show ip ospf (<0-65535>|) neighbor all",
     CLI_SHOW_STR,
     CLI_IP_STR,
     OSPF_CLI_OSPF_INFO_STR,
     OSPF_CLI_OSPF_PROCESS_ID_STR,
     "Neighbor list",
     "include down status neighbor")
{
  struct ospf_master *om = cli->vr->proto;
  struct ospf *top;
  struct listnode *node;

  if (list_isempty (om->ospf))
    return CLI_SUCCESS;

  if (argc > 0)
    {
      int id;

      CLI_GET_INTEGER_RANGE ("OSPF process ID", id, argv[0], 0, 65535);
      top = ospf_proc_lookup (om, id);
      if (top == NULL)
        {
          cli_out (cli, "%%OSPF: No router process %d\n", id);
          return CLI_ERROR;
        }

      ospf_cli_show_neighbor_all (cli, top);
    }
  else
    {
      LIST_LOOP (om->ospf, top, node)
        ospf_cli_show_neighbor_all (cli, top);
    }
  
  return CLI_SUCCESS;
}

CLI (show_ip_ospf_neighbor_int,
     show_ip_ospf_neighbor_int_cmd,
     "show ip ospf (<0-65535>|) neighbor interface A.B.C.D",
     CLI_SHOW_STR,
     CLI_IP_STR,
     OSPF_CLI_OSPF_INFO_STR,
     OSPF_CLI_OSPF_PROCESS_ID_STR,
     "Neighbor list",
     "Interface",
     "Interface address")
{
  struct ospf_master *om = cli->vr->proto;
  struct ospf *top;
  struct pal_in4_addr addr;
  struct listnode *node;
  
  if (argc > 1)
    {
      int id;
      
      CLI_GET_INTEGER_RANGE ("OSPF process ID", id, argv[0], 0, 65535);
      top = ospf_proc_lookup (om, id);
      if (top == NULL)
        {
          cli_out (cli, "%%OSPF: No router process %d\n", id);
          return CLI_ERROR;
        }
      CLI_GET_IPV4_ADDRESS ("interface address", addr, argv[1]);

      ospf_cli_show_neighbor_by_if (cli, top, addr);
    }
  else
    {
      CLI_GET_IPV4_ADDRESS ("interface address", addr, argv[0]);
      LIST_LOOP (om->ospf, top, node)
        ospf_cli_show_neighbor_by_if (cli, top, addr);
    }

  return CLI_SUCCESS;
}

CLI (show_ip_ospf_neighbor_id,
     show_ip_ospf_neighbor_id_cmd,
     "show ip ospf (<0-65535>|) neighbor A.B.C.D",
     CLI_SHOW_STR,
     CLI_IP_STR,
     OSPF_CLI_OSPF_INFO_STR,
     OSPF_CLI_OSPF_PROCESS_ID_STR,
     "Neighbor list",
     "Neighbor ID")
{
  struct ospf_master *om = cli->vr->proto;
  struct ospf *top;
  struct pal_in4_addr router_id;
  struct listnode *node;

  if (list_isempty (om->ospf))
    return CLI_SUCCESS;

  if (argc > 1)
    {
      int id;

      CLI_GET_INTEGER_RANGE ("OSPF process ID", id, argv[0], 0, 65535);
      top = ospf_proc_lookup (om, id);
      if (top == NULL)
        {
          cli_out (cli, "%%OSPF: No router process %d\n", id);
          return CLI_ERROR;
        }
      CLI_GET_IPV4_ADDRESS ("neighbor ID", router_id, argv[1]);
      ospf_cli_show_neighbor_by_nbr_id (cli, top, router_id);
    }
  else
    {
      CLI_GET_IPV4_ADDRESS ("neighbor ID", router_id, argv[0]);
      LIST_LOOP (om->ospf, top, node)
        ospf_cli_show_neighbor_by_nbr_id (cli, top, router_id);
    }

  return CLI_SUCCESS;
}

ALI (show_ip_ospf_neighbor_id,
     show_ip_ospf_neighbor_int_detail_cmd,
     "show ip ospf (<0-65535>|) neighbor A.B.C.D detail",
     CLI_SHOW_STR,
     CLI_IP_STR,
     OSPF_CLI_OSPF_INFO_STR,
     OSPF_CLI_OSPF_PROCESS_ID_STR,
     "Neighbor list",
     "Interface address",
     "detail of all neighbors");

CLI (show_ip_ospf_neighbor_detail,
     show_ip_ospf_neighbor_detail_cmd,
     "show ip ospf (<0-65535>|) neighbor detail",
     CLI_SHOW_STR,
     CLI_IP_STR,
     OSPF_CLI_OSPF_INFO_STR,
     OSPF_CLI_OSPF_PROCESS_ID_STR,
     "Neighbor list",
     "detail of all neighbors")
{
  struct ospf_master *om = cli->vr->proto;
  struct ospf *top;
  struct listnode *node;

  if (list_isempty (om->ospf))
    return CLI_SUCCESS;

  if (argc > 0)
    {
      int id;

      CLI_GET_INTEGER_RANGE ("OSPF process ID", id, argv[0], 0, 65535);
      top = ospf_proc_lookup (om, id);
      if (top == NULL)
        {
          cli_out (cli, "%%OSPF: No router process %d\n", id);
          return CLI_ERROR;
        }
      ospf_cli_show_neighbor_detail (cli, top);
    }
  else
    {
      LIST_LOOP (om->ospf, top, node)
        ospf_cli_show_neighbor_detail (cli, top);
    }
  
  return CLI_SUCCESS;
}

CLI (show_ip_ospf_neighbor_detail_all,
     show_ip_ospf_neighbor_detail_all_cmd,
     "show ip ospf (<0-65535>|) neighbor detail all",
     CLI_SHOW_STR,
     CLI_IP_STR,
     OSPF_CLI_OSPF_INFO_STR,
     OSPF_CLI_OSPF_PROCESS_ID_STR,
     "Neighbor list",
     "detail of all neighbors",
     "include down status neighbor")
{
  struct ospf_master *om = cli->vr->proto;
  struct ospf *top;
  struct listnode *node;

  if (list_isempty (om->ospf))
    return CLI_SUCCESS;

  if (argc > 0)
    {
      int id;

      CLI_GET_INTEGER_RANGE ("OSPF process ID", id, argv[0], 0, 65535);
      top = ospf_proc_lookup (om, id);
      if (top == NULL)
        {
          cli_out (cli, "%%OSPF: No router process %d\n", id);
          return CLI_ERROR;
        }
      ospf_cli_show_neighbor_detail_all (cli, top);
    }
  else
    {
      LIST_LOOP (om->ospf, top, node)
        ospf_cli_show_neighbor_detail_all (cli, top);
    }

  return CLI_SUCCESS;
}

CLI (show_ip_ospf_database,
     show_ip_ospf_database_cmd,
     "show ip ospf database (self-originate|max-age|adv-router A.B.C.D|)",
     CLI_SHOW_STR,
     CLI_IP_STR,
     OSPF_CLI_OSPF_INFO_STR,
     "Database summary",
     "Self-originated link states",
     "LSAs in MaxAge list",
     "Advertising Router link states",
     "Advertising Router (as an IP address)")
{
  struct ospf_show_database_arg *arg;
  struct pal_in4_addr adv_router;
  int flags = 0;

  pal_mem_set(&adv_router, 0, sizeof (struct pal_in4_addr)); 
  if (argc > 1)
    {
      CLI_GET_IPV4_ADDRESS ("Advertising Router", adv_router, argv[1]);
      SET_FLAG (flags, OSPF_SHOW_DATABASE_ADV_ROUTER);
    }

  if (argc > 0)
    {
      if (pal_strncmp (argv[0], "s", 1) == 0)
        SET_FLAG (flags, OSPF_SHOW_DATABASE_SELF);
      else if (pal_strncmp (argv[0], "m", 1) == 0)
        SET_FLAG (flags, OSPF_SHOW_DATABASE_MAXAGE);
    }

  /* Set arguments. */
  arg = XCALLOC (MTYPE_TMP, sizeof (struct ospf_show_database_arg));
  arg->flags = flags;
  arg->adv_router = adv_router;
  arg->proc_id = -1;
  arg->max_count = 20;
  arg->show = ospf_show_database_brief;

  /* Start show callback. */
  cli->arg = arg;
  cli->callback = ospf_show_database_callback;
  ospf_show_database_callback (cli);

  return CLI_SUCCESS;
}

CLI (show_ip_ospf_database_type,
     show_ip_ospf_database_type_cmd,
     "show ip ospf database (" OSPF_LSA_TYPES_STRING ") (self-originate|adv-router A.B.C.D|)",
     CLI_SHOW_STR,
     CLI_IP_STR,
     OSPF_CLI_OSPF_INFO_STR,
     "Database summary",
     OSPF_LSA_TYPES_DESC,
     "Self-originated link states",
     "Advertising Router link states",
     "Advertising Router (as an IP address)")
{
  struct ospf_show_database_arg *arg;
  struct pal_in4_addr adv_router;
  int flags = 0;
  int type;
  
  pal_mem_set(&adv_router, 0, sizeof (struct pal_in4_addr));
  if ((type = ospf_show_str2lsa_type (argv[0])) < 0)
    return CLI_ERROR;

  if (argc > 2)
    {
      CLI_GET_IPV4_ADDRESS ("Advertising Router", adv_router, argv[2]);
      SET_FLAG (flags, OSPF_SHOW_DATABASE_ADV_ROUTER);
    }

  if (argc > 1)
    {
      if (pal_strncmp (argv[1], "s", 1) == 0)
        SET_FLAG (flags, OSPF_SHOW_DATABASE_SELF);
    }

  /* Set arguments. */
  arg = XCALLOC (MTYPE_TMP, sizeof (struct ospf_show_database_arg));
  arg->type = type;
  arg->flags = flags;
  arg->adv_router = adv_router;
  arg->proc_id = -1;
  arg->max_count = 1;
  arg->show = ospf_show_database_detail;

  /* Start show callback. */
  cli->arg = arg;
  cli->callback = ospf_show_database_callback;
  ospf_show_database_callback (cli);

  return CLI_SUCCESS;
}

CLI (show_ip_ospf_database_type_id,
     show_ip_ospf_database_type_id_cmd,
     "show ip ospf database (" OSPF_LSA_TYPES_STRING ") A.B.C.D (self-originate|adv-router A.B.C.D|)",
     CLI_SHOW_STR,
     CLI_IP_STR,
     OSPF_CLI_OSPF_INFO_STR,
     "Database summary",
     OSPF_LSA_TYPES_DESC,
     "Link State ID (as an IP address)",
     "Self-originated link states",
     "Advertising Router link states",
     "Advertising Router (as an IP address)")
{
  struct ospf_show_database_arg *arg;
  struct pal_in4_addr adv_router;
  struct pal_in4_addr id;
  int flags = 0;
  int type;
  
  pal_mem_set(&adv_router, 0, sizeof (struct pal_in4_addr));
  if ((type = ospf_show_str2lsa_type (argv[0])) < 0)
    return CLI_ERROR;

  CLI_GET_IPV4_ADDRESS ("Link State ID", id, argv[1]);
  SET_FLAG (flags, OSPF_SHOW_DATABASE_ID);

  if (argc > 3)
    {
      CLI_GET_IPV4_ADDRESS ("Advertising Router", adv_router, argv[3]);
      SET_FLAG (flags, OSPF_SHOW_DATABASE_ADV_ROUTER);
    }

  if (argc > 2)
    {
      if (pal_strncmp (argv[2], "s", 1) == 0)
        SET_FLAG (flags, OSPF_SHOW_DATABASE_SELF);
    }

  /* Set arguments. */
  arg = XCALLOC (MTYPE_TMP, sizeof (struct ospf_show_database_arg));
  arg->type = type;
  arg->flags = flags;
  arg->adv_router = adv_router;
  arg->id = id;
  arg->proc_id = -1;
  arg->max_count = 1;
  arg->show = ospf_show_database_detail;

  /* Start show callback. */
  cli->arg = arg;
  cli->callback = ospf_show_database_callback;
  ospf_show_database_callback (cli);

  return CLI_SUCCESS;
}

CLI (show_ip_ospf_id_database,
     show_ip_ospf_id_database_cmd,
     "show ip ospf <0-65535> database (self-originate|max-age|adv-router A.B.C.D|)",
     CLI_SHOW_STR,
     CLI_IP_STR,
     OSPF_CLI_OSPF_INFO_STR,
     OSPF_CLI_OSPF_PROCESS_ID_STR,
     "Database summary",
     "Self-originated link states",
     "LSAs in MaxAge list",
     "Advertising Router link states",
     "Advertising Router (as an IP address)")
{
  struct ospf_master *om = cli->vr->proto;
  struct ospf_show_database_arg *arg;
  struct ospf *top;
  struct pal_in4_addr adv_router;
  int flags = 0;
  int proc_id;
  
  pal_mem_set(&adv_router, 0, sizeof (struct pal_in4_addr));
  CLI_GET_INTEGER_RANGE ("OSPF process ID", proc_id, argv[0], 0, 65535);
  top = ospf_proc_lookup (om, proc_id);
  if (top == NULL)
    {
      cli_out (cli, "%%OSPF: No router process %d\n", proc_id);
      return CLI_ERROR;
    }

  if (argc > 2)
    {
      CLI_GET_IPV4_ADDRESS ("Advertising Router", adv_router, argv[2]);
      SET_FLAG (flags, OSPF_SHOW_DATABASE_ADV_ROUTER);
    }

  if (argc > 1)
    {
      if (pal_strncmp (argv[1], "s", 1) == 0)
        SET_FLAG (flags, OSPF_SHOW_DATABASE_SELF);
      else if (pal_strncmp (argv[1], "m", 1) == 0)
        SET_FLAG (flags, OSPF_SHOW_DATABASE_MAXAGE);
    }

  /* Set arguments. */
  arg = XCALLOC (MTYPE_TMP, sizeof (struct ospf_show_database_arg));
  arg->flags = flags;
  arg->adv_router = adv_router;
  arg->proc_id = proc_id;
  arg->max_count = 20;
  arg->show = ospf_show_database_brief;

  /* Start show callback. */
  cli->arg = arg;
  cli->callback = ospf_show_database_callback;
  ospf_show_database_callback (cli);

  return CLI_SUCCESS;
}

CLI (show_ip_ospf_iddatabase_type,
     show_ip_ospf_id_database_type_cmd,
     "show ip ospf <0-65535> database (" OSPF_LSA_TYPES_STRING ") (self-originate|adv-router A.B.C.D|)",
     CLI_SHOW_STR,
     CLI_IP_STR,
     OSPF_CLI_OSPF_INFO_STR,
     OSPF_CLI_OSPF_PROCESS_ID_STR,
     "Database summary",
     OSPF_LSA_TYPES_DESC,
     "Self-originated link states",
     "Advertising Router link states",
     "Advertising Router (as an IP address)")
{
  struct ospf_master *om = cli->vr->proto;
  struct ospf_show_database_arg *arg;
  struct ospf *top;
  struct pal_in4_addr adv_router;
  int flags = 0;
  int type;
  int proc_id;
  
  pal_mem_set(&adv_router, 0, sizeof (struct pal_in4_addr));
  CLI_GET_INTEGER_RANGE ("OSPF process ID", proc_id, argv[0], 0, 65535);
  top = ospf_proc_lookup (om, proc_id);
  if (top == NULL)
    {
      cli_out (cli, "%%OSPF: No router process %d\n", proc_id);
      return CLI_ERROR;
    }
  
  if ((type = ospf_show_str2lsa_type (argv[1])) < 0)
    return CLI_ERROR;

  if (argc > 3)
    {
      CLI_GET_IPV4_ADDRESS ("Advertising Router", adv_router, argv[3]);
      SET_FLAG (flags, OSPF_SHOW_DATABASE_ADV_ROUTER);
    }

  if (argc > 2)
    {
      if (pal_strncmp (argv[2], "s", 1) == 0)
        SET_FLAG (flags, OSPF_SHOW_DATABASE_SELF);
    }

  /* Set arguments. */
  arg = XCALLOC (MTYPE_TMP, sizeof (struct ospf_show_database_arg));
  arg->type = type;
  arg->flags = flags;
  arg->adv_router = adv_router;
  arg->proc_id = proc_id;
  arg->max_count = 1;
  arg->show = ospf_show_database_detail;

  /* Start show callback. */
  cli->arg = arg;
  cli->callback = ospf_show_database_callback;
  ospf_show_database_callback (cli);

  return CLI_SUCCESS;
}

CLI (show_ip_ospf_id_database_type_id,
     show_ip_ospf_id_database_type_id_cmd,
     "show ip ospf <0-65535> database (" OSPF_LSA_TYPES_STRING ") A.B.C.D (self-originate|adv-router A.B.C.D|)",
     CLI_SHOW_STR,
     CLI_IP_STR,
     OSPF_CLI_OSPF_INFO_STR,
     OSPF_CLI_OSPF_PROCESS_ID_STR,
     "Database summary",
     OSPF_LSA_TYPES_DESC,
     "Link State ID (as an IP address)",
     "Self-originated link states",
     "Advertising Router link states",
     "Advertising Router (as an IP address)")
{
  struct ospf_master *om = cli->vr->proto;
  struct ospf_show_database_arg *arg;
  struct ospf *top;
  struct pal_in4_addr adv_router;
  struct pal_in4_addr id;
  int flags = 0;
  int type;
  int proc_id;
 
  pal_mem_set(&adv_router, 0, sizeof (struct pal_in4_addr)); 
  CLI_GET_INTEGER_RANGE ("OSPF process ID", proc_id, argv[0], 0, 65535);
  top = ospf_proc_lookup (om, proc_id);
  if (top == NULL)
    {
      cli_out (cli, "%%OSPF: No router process %d\n", proc_id);
      return CLI_ERROR;
    }
  
  if ((type = ospf_show_str2lsa_type (argv[1])) < 0)
    return CLI_ERROR;

  CLI_GET_IPV4_ADDRESS ("Link State ID", id, argv[2]);
  SET_FLAG (flags, OSPF_SHOW_DATABASE_ID);

  if (argc > 4)
    {
      CLI_GET_IPV4_ADDRESS ("Advertising Router", adv_router, argv[4]);
      SET_FLAG (flags, OSPF_SHOW_DATABASE_ADV_ROUTER);
    }

  if (argc > 3)
    {
      if (pal_strncmp (argv[3], "s", 1) == 0)
        SET_FLAG (flags, OSPF_SHOW_DATABASE_SELF);
    }

  /* Set arguments. */
  arg = XCALLOC (MTYPE_TMP, sizeof (struct ospf_show_database_arg));
  arg->type = type;
  arg->flags = flags;
  arg->adv_router = adv_router;
  arg->id = id;
  arg->proc_id = proc_id;
  arg->max_count = 1;
  arg->show = ospf_show_database_detail;

  /* Start show callback. */
  cli->arg = arg;
  cli->callback = ospf_show_database_callback;
  ospf_show_database_callback (cli);

  return CLI_SUCCESS;
}

CLI (show_ip_ospf_border_routers,
     show_ip_ospf_border_routers_cmd,
     "show ip ospf (<0-65535>|) border-routers",
     CLI_SHOW_STR,
     CLI_IP_STR,
     OSPF_CLI_OSPF_INFO_STR,
     OSPF_CLI_OSPF_PROCESS_ID_STR,
     "Border and Boundary Router Information")
{
  struct ospf_master *om = cli->vr->proto;
  struct ospf *top;
  struct listnode *node;
  
  if (list_isempty (om->ospf))
    return CLI_SUCCESS;

  if (argc > 0)
    {
      int id;
      
      CLI_GET_INTEGER_RANGE ("OSPF process ID", id, argv[0], 0, 65535);
      top = ospf_proc_lookup (om, id);
      if (top == NULL)
        {
          cli_out (cli, "%%OSPF: No router process %d\n", id);
          return CLI_ERROR;
        }

      ospf_cli_show_border_router (cli, top);

      return CLI_SUCCESS;
    }

  LIST_LOOP (om->ospf, top, node)
    ospf_cli_show_border_router (cli, top);

  return CLI_SUCCESS;
}

CLI (show_ip_ospf_route,
     show_ip_ospf_route_cmd,
     "show ip ospf (<0-65535>|) route",
     CLI_SHOW_STR,
     CLI_IP_STR,
     OSPF_CLI_OSPF_INFO_STR,
     OSPF_CLI_OSPF_PROCESS_ID_STR,
     "OSPF routing table")
{
  struct ospf_master *om = cli->vr->proto;
  struct ospf_show_route_arg *arg;
  struct ospf *top;
  int proc_id = -1;

  if (argc > 0)
    {
      CLI_GET_INTEGER_RANGE ("OSPF process ID", proc_id, argv[0], 0, 65535);
      top = ospf_proc_lookup (om, proc_id);
      if (top == NULL)
        {
          cli_out (cli, "%%OSPF: No router process %d\n", proc_id);
          return CLI_ERROR;
        }
    }
  
  /* Set arguments. */
  arg = XCALLOC (MTYPE_TMP, sizeof (struct ospf_show_route_arg));
  SET_FLAG (arg->st.flags, OSPF_SHOW_ROUTE_STATE_INIT);
  arg->proc_id = proc_id;
  arg->max_count = 1;

  /* Set show callback. */
  cli->arg = arg;
  cli->callback = ospf_show_route_callback;
  ospf_show_route_callback (cli);

  return CLI_SUCCESS;
}

CLI (show_ip_ospf_buffers,
     show_ip_ospf_buffers_cmd,
     "show ip ospf (<0-65535>|) buffers",
     CLI_SHOW_STR,
     CLI_IP_STR,
     OSPF_CLI_OSPF_INFO_STR,
     OSPF_CLI_OSPF_PROCESS_ID_STR,
     "OSPF internal buffers")
{
  struct ospf_master *om = cli->vr->proto;
  struct ospf *top;
  struct listnode *node;

  if (list_isempty (om->ospf))
    return CLI_SUCCESS;

  if (argc > 0)
    {
      int id;

      CLI_GET_INTEGER_RANGE ("OSPF process ID", id, argv[0], 0, 65535);
      top = ospf_proc_lookup (om, id);
      if (top == NULL)
        {
          cli_out (cli, "%%OSPF: No router process %d\n", id);
          return CLI_ERROR;
        }

      ospf_cli_show_buffers (cli, top);
    }
  else
    LIST_LOOP (om->ospf, top, node)
      ospf_cli_show_buffers (cli, top);

  return CLI_SUCCESS;
}

CLI (ospf_show_ip_protocols,
     ospf_show_ip_protocols_cmd,
     "show ip protocols",
     CLI_SHOW_STR,
     CLI_IP_STR,
     "IP routing protocol process parameters and statistics")
{
  struct ospf_master *om = cli->vr->proto;
  struct ospf *top;
  struct ospf_network *nw;
  struct ospf_redist_conf *rc;
  struct ls_node *rn;
  struct listnode *node;
  int type, flag = 0;

  LIST_LOOP (om->ospf, top, node)
    {
      flag = 0;
      cli_out (cli, "Routing Protocol is \"ospf %d\"\n", top->ospf_id);
      /* Show filter for each protocols by distribute-list. */
      for (type = 0; type < APN_ROUTE_MAX; type++)
        if (DIST_NAME (&top->redist[type]))
          cli_out (cli, "    Redistributed %s filtered by %s\n",
                   LOOKUP (ospf_nsm_route_type_msg, type),
                   DIST_NAME (&top->redist[type]));

      /* Show redistributed protocols. */
      cli_out (cli, "  Redistributing: ");
      for (type = 0; type < APN_ROUTE_MAX; type++)
       for (rc = &top->redist[type]; rc; rc = rc->next)
        if (REDIST_PROTO_CHECK (rc))
          {
            cli_out (cli, "%s%s %d", flag ? ", " : "",
                     LOOKUP (ospf_nsm_route_type_msg, type), rc->pid);
            flag++;
          }
      cli_out (cli, "\n");

      /* Show networks. */
      cli_out (cli, "  Routing for Networks:\n");
      for (rn = ls_table_top (top->networks); rn; rn = ls_route_next (rn))
        if ((nw = RN_INFO (rn, RNI_DEFAULT)))
          cli_out (cli, "    %P\n", nw->lp);

      /* Show distances. */
      cli_out (cli, "  Distance: (default is 110)\n");

      if (top->distance_all)
        cli_out (cli, "    All %lu", top->distance_all);
      if (top->distance_intra)
        cli_out (cli, "    Intra-Area %lu", top->distance_intra);
      if (top->distance_inter)
        cli_out (cli, "    Inter-Area %lu", top->distance_inter);
      if (top->distance_external)
        cli_out (cli, "    External %lu", top->distance_external);

      cli_out (cli, "\n");
    }

  return CLI_SUCCESS;
}

ALI (ospf_show_ip_protocols,
     ospf_show_ip_protocols_ospf_cmd,
     "show ip protocols ospf",
     CLI_SHOW_STR,
     CLI_IP_STR,
     "IP routing protocol process parameters and statistics",
     "Open Shortest Path First (OSPF)");

#ifdef HAVE_OSPF_CSPF
CLI (show_ip_ospf_te_database,
     show_ip_ospf_te_database_cmd,
     "show ip ospf (<0-65535>|) te-database",
     CLI_SHOW_STR,
     CLI_IP_STR,
     OSPF_CLI_OSPF_INFO_STR,
     OSPF_CLI_OSPF_PROCESS_ID_STR,
     "TE Database")
{
  struct ospf_master *om = cli->vr->proto;
  struct ospf *top;
  struct listnode *node;
  int id;

  if (list_isempty (om->ospf))
    return CLI_SUCCESS;

  if (argc > 0)
    {
      CLI_GET_INTEGER_RANGE ("OSPF process ID", id, argv[0], 0, 65535);

      top = ospf_proc_lookup (om, id);
      if (top == NULL)
        {
          cli_out (cli, "%%OSPF: No router process %d\n", id);
          return CLI_ERROR;
        }

      ospf_show_te_database (cli, top);
    }
  else
    LIST_LOOP (om->ospf, top, node)
      ospf_show_te_database (cli, top);

  return CLI_SUCCESS;
}

CLI (show_debugging_cspf,
     show_debugging_cspf_cmd,
     "show debugging cspf",
     CLI_SHOW_STR,
     CLI_DEBUG_STR,
     "CSPF Information")
{
  struct ospf_master *om = cli->vr->proto;

  cli_out (cli, "CSPF debugging status:\n");
  
  if (IS_DEBUG_CSPF (&om->debug_cspf, EVENT))
    cli_out (cli, "CSPF events debugging is on\n");
  if (IS_DEBUG_CSPF (&om->debug_cspf, HEXDUMP))
    cli_out (cli, "CSPF message hexdump  debugging is on\n");

  return CLI_SUCCESS;
}

CLI (show_cspf_lsp,
     show_cspf_lsp_cmd,
     "show cspf lsp",
     CLI_SHOW_STR,
     "CSPF Information",
     "LSP Information")
{
  struct ospf_master *om = cli->vr->proto;
  struct ospf *top;
  struct listnode *node;
  
  if (list_isempty (om->ospf))
    return CLI_SUCCESS;
  
  LIST_LOOP (om->ospf, top, node)
    {
      if (top->cspf)
        {
          cspf_cli_show_lsp (cli, top->cspf);
          break;
        }
    }

  return CLI_SUCCESS;
}
#endif /* HAVE_OSPF_CSPF */

CLI (clear_ip_ospf_process,
     clear_ip_ospf_process_cmd,
     "clear ip ospf (<0-65535>|) process",
     "Reset functions",
     CLI_IP_STR,
     CLI_OSPF_STR,
     "Process ID number",
     "Reset OSPF process")
{
  struct ospf_master *om = cli->vr->proto;
  struct ospf *top;
  struct listnode *node;
  int id;

  if (list_isempty (om->ospf))
    return CLI_SUCCESS;

  if (argc > 0)
    {
      CLI_GET_INTEGER_RANGE ("OSPF process ID", id, argv[0], 1, 65535);

      top = ospf_proc_lookup (om, id);
      if (top == NULL)
        {
          cli_out (cli, "%%OSPF: No router process %d\n", id);
          return CLI_ERROR;
        }
      ospf_proc_update (top);
    }
  else
    LIST_LOOP (om->ospf, top, node)
      ospf_proc_update (top);

  return CLI_SUCCESS;
}

#ifdef HAVE_MPLS
CLI (show_ip_ospf_igp_shortcut_lsp,
     show_ip_ospf_igp_shortcut_lsp_cmd,
     "show ip ospf igp-shortcut-lsp",
     CLI_SHOW_STR,
     CLI_IP_STR,
     OSPF_CLI_OSPF_INFO_STR,
     "IGP-Shortcut LSP entries")
{
  struct ospf_master *om = cli->vr->proto;
  struct ospf_vrf *ov;
  struct route_node *rn;
  struct ospf_igp_shortcut_lsp *list, *lsp;
  bool_t heading = PAL_FALSE;
  
  if (list_isempty (om->ospf))
    {
      cli_out (cli, " OSPF Routing Process not enabled.\n");
      return CLI_SUCCESS;
    }

  ov = ospf_vrf_lookup_by_id (om, 0);
  if (ov == NULL || ! ov->igp_lsp_table)
    return CLI_SUCCESS;

  for (rn = route_top (ov->igp_lsp_table); rn; rn = route_next (rn))
    {
      list = (struct ospf_igp_shortcut_lsp *)rn->info;
      if (! list)
        continue;

      for (lsp = list; lsp; lsp = lsp->next)
        {
          if (heading == PAL_FALSE)
            {
              cli_out (cli, "Tunnel-endpoint     Tunnel-id"
                            "     Tunnel-metric\n");
              heading = PAL_TRUE;
            }
          /*if (! CHECK_FLAG (lsp->flags, OSPF_IGP_SHORTCUT_LSP_INACTIVE)) */
            cli_out (cli, "%-20r%-14d%-15d %s %d\n", &lsp->t_endp, lsp->tunnel_id, 
                     lsp->metric, 
                     CHECK_FLAG (lsp->flags, OSPF_IGP_SHORTCUT_LSP_INACTIVE)? "inactive" : "active", lsp->lock);
        }
    }

  return CLI_SUCCESS;
}

void
ospf_show_igp_shortcut_route (struct cli *cli, struct ospf *top)
{
  struct ls_node *rn;
  struct ospf_route *route;
  struct ospf_path *path;
  char buf[44];
  int len;

  cli_out (cli, "\n");
  cli_out (cli, "OSPF process %d:\n", top->ospf_id);

  for (rn = ls_table_top (top->rt_network); rn; rn = ls_route_next (rn))
    if ((route = RN_INFO (rn, RNI_CURRENT)))
      if ((path = route->selected))
        if (path->t_lsp)
          {
            pal_mem_set (buf, 0, 44);
            len = zsnprintf (buf, 44, "%P [%d] ", rn->p, path->t_cost);
            cli_out (cli, "%*s tunnel-id: %d,  %r", len, buf, 
                     path->t_lsp->tunnel_id, &path->t_lsp->t_endp);
            cli_out (cli, "\n");
          }
}

CLI (show_ip_ospf_igp_shortcut_route,
     show_ip_ospf_igp_shortcut_route_cmd,
     "show ip ospf (<0-65535>|) igp-shortcut-route",
     CLI_SHOW_STR,
     CLI_IP_STR,
     OSPF_CLI_OSPF_INFO_STR,
     OSPF_CLI_OSPF_PROCESS_ID_STR,
     "IGP-Shortcut route entries")
{
  struct ospf_master *om = cli->vr->proto;
  struct listnode *node;
  struct ospf *top;
  int proc_id = -1;

  if (argc > 0)
    {
      CLI_GET_INTEGER_RANGE ("OSPF process ID", proc_id, argv[0], 0, 65535);
      top = ospf_proc_lookup (om, proc_id);
      if (top == NULL)
        {
          cli_out (cli, "%%OSPF: No router process %d\n", proc_id);
          return CLI_ERROR;
        }
      ospf_show_igp_shortcut_route (cli, top);
    }
 
  LIST_LOOP (om->ospf, top, node)
    ospf_show_igp_shortcut_route (cli, top);

  return CLI_SUCCESS;
}
#endif /* HAVE_MPLS */

void
ospf_show_init (struct lib_globals *zg)
{
  struct cli_tree *ctree = zg->ctree;

  /* "show ip ospf" comamnds. */
  cli_install_gen (ctree, EXEC_MODE, PRIVILEGE_NORMAL, 0,
                   &show_ip_ospf_cmd);

  /* "show ip ospf interface" commands. */
  cli_install_gen (ctree, EXEC_MODE, PRIVILEGE_NORMAL, 0,
                   &show_ip_ospf_interface_cmd);

  /* "show ip ospf virtual-links" commands. */
  cli_install_gen (ctree, EXEC_MODE, PRIVILEGE_NORMAL, 0,
                   &show_ip_ospf_virtual_links_cmd);

#ifdef HAVE_OSPF_MULTI_AREA
  /* "show ip ospf multi-area-adj" commands. */
  cli_install_gen (ctree, EXEC_MODE, PRIVILEGE_NORMAL, 0,
                   &show_ip_ospf_multi_area_adjacencies_cmd);
#endif /* HAVE_OSPF_MULTI_AREA */

  /* "show ip ospf neighbor" commands. */
  cli_install_gen (ctree, EXEC_MODE, PRIVILEGE_NORMAL, 0,
                   &show_ip_ospf_neighbor_int_detail_cmd);
  cli_install_gen (ctree, EXEC_MODE, PRIVILEGE_NORMAL, 0,
                   &show_ip_ospf_neighbor_int_cmd);
  cli_install_gen (ctree, EXEC_MODE, PRIVILEGE_NORMAL, 0,
                   &show_ip_ospf_neighbor_id_cmd);
  cli_install_gen (ctree, EXEC_MODE, PRIVILEGE_NORMAL, 0,
                   &show_ip_ospf_neighbor_detail_all_cmd);
  cli_install_gen (ctree, EXEC_MODE, PRIVILEGE_NORMAL, 0,
                   &show_ip_ospf_neighbor_detail_cmd);
  cli_install_gen (ctree, EXEC_MODE, PRIVILEGE_NORMAL, 0,
                   &show_ip_ospf_neighbor_cmd);
  cli_install_gen (ctree, EXEC_MODE, PRIVILEGE_NORMAL, 0,
                   &show_ip_ospf_neighbor_all_cmd);
  cli_install_gen (ctree, EXEC_MODE, PRIVILEGE_NORMAL, CLI_FLAG_HIDDEN,
                   &show_ip_ospf_neighbor_private_cmd);
  
  /* "show ip protocols" command. */
  cli_install_gen (ctree, EXEC_MODE, PRIVILEGE_NORMAL, 0,
                   &ospf_show_ip_protocols_cmd);
  cli_install_gen (ctree, EXEC_MODE, PRIVILEGE_NORMAL, 0,
                   &ospf_show_ip_protocols_ospf_cmd);

  /* "show ip ospf database" commands. */
  cli_install_gen (ctree, EXEC_MODE, PRIVILEGE_NORMAL, 0,
                   &show_ip_ospf_database_cmd);
  cli_install_gen (ctree, EXEC_MODE, PRIVILEGE_NORMAL, 0,
                   &show_ip_ospf_database_type_cmd);
  cli_install_gen (ctree, EXEC_MODE, PRIVILEGE_NORMAL, 0,
                   &show_ip_ospf_database_type_id_cmd);
  cli_install_gen (ctree, EXEC_MODE, PRIVILEGE_NORMAL, 0,
                   &show_ip_ospf_id_database_cmd);
  cli_install_gen (ctree, EXEC_MODE, PRIVILEGE_NORMAL, 0,
                   &show_ip_ospf_id_database_type_cmd);
  cli_install_gen (ctree, EXEC_MODE, PRIVILEGE_NORMAL, 0,
                   &show_ip_ospf_id_database_type_id_cmd);
  
  /* "show ip ospf route" command. */
  cli_install_gen (ctree, EXEC_MODE, PRIVILEGE_NORMAL, 0,
                   &show_ip_ospf_route_cmd);

  /* "show ip ospf border-routers" command. */
  cli_install_gen (ctree, EXEC_MODE, PRIVILEGE_NORMAL, 0,
                   &show_ip_ospf_border_routers_cmd);

  /* "show ip ospf buffers" command. */
  cli_install_gen (ctree, EXEC_MODE, PRIVILEGE_VR_MAX, CLI_FLAG_HIDDEN,
                   &show_ip_ospf_buffers_cmd);

  /* "show debugging ospf" command. */
  cli_install_gen (ctree, EXEC_MODE, PRIVILEGE_NORMAL, 0,
                   &show_debugging_ospf_cmd);

  /* "clear ip ospf process" command. */
  cli_install_gen (ctree, EXEC_MODE, PRIVILEGE_VR_MAX, 0,
                   &clear_ip_ospf_process_cmd);

#ifdef HAVE_OSPF_CSPF
  /* "show cspf lsp" commands. */
  cli_install_gen (ctree, EXEC_MODE, PRIVILEGE_NORMAL, 0,
                   &show_cspf_lsp_cmd);

  /* "show ip ospf te-database" commands. */
  cli_install_gen (ctree, EXEC_MODE, PRIVILEGE_NORMAL, 0,
                   &show_ip_ospf_te_database_cmd);

  /* "show debugging cspf" commands. */
  cli_install_gen (ctree, EXEC_MODE, PRIVILEGE_NORMAL, 0,
                   &show_debugging_cspf_cmd);
#endif /* HAVE_OSPF_CSPF */

#ifdef HAVE_OPAQUE_LSA
  ospf_show_opaque_vec = vector_init (OSPF_VECTOR_SIZE_MIN);

#ifdef HAVE_OSPF_TE
  ospf_show_opaque_set (OSPF_LSA_OPAQUE_TYPE_TE,
                        ospf_te_show_opaque_data);
#endif /* HAVE_OSPF_TE */

#ifdef HAVE_RESTART
  ospf_show_opaque_set (OSPF_LSA_OPAQUE_TYPE_GRACE,
                        ospf_show_opaque_grace_lsa);
#endif /* HAVE_RESTART */
#endif /* HAVE_OPAQUE_LSA */

#ifdef HAVE_MPLS
  cli_install_gen (ctree, EXEC_MODE, PRIVILEGE_NORMAL, 0,
                   &show_ip_ospf_igp_shortcut_lsp_cmd);
  cli_install_gen (ctree, EXEC_MODE, PRIVILEGE_NORMAL, 0,
                   &show_ip_ospf_igp_shortcut_route_cmd);
#endif /* HAVE_MPLS */
}
