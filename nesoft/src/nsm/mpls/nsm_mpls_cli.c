/* Copyright (C) 2002-2003  All Rights Reserved. */

#include "pal.h"
#include "lib.h"

#include "cli.h"
#include "label_pool.h"
#include "ptree.h"
#include "table.h"
#include "avl_tree.h"
#include "if.h"
#include "log.h"
#ifdef HAVE_TE
#include "admin_grp.h"
#endif /* HAVE_TE */
#include "mpls.h"
#include "mpls_common.h"
#include "mpls_client.h"
#ifdef HAVE_GMPLS
#include "nsm_gmpls.h"
#endif /* HAVE_GMPLS */
#include "nsmd.h"
#include "nsm_server.h"
#ifdef HAVE_DSTE
#include "dste.h"
#endif /* HAVE_DSTE */
#ifdef HAVE_DIFFSERV
#include "diffserv.h"
#endif /* HAVE_DIFFSERV */
#include "bitmap.h"

#include "rib/rib.h"
#include "nsm_api.h"
#include "nsm_mpls.h"
#include "nsm_mpls_vc.h"
#include "nsm_mpls_vc_snmp.h"
#include "nsm_debug.h"
#include "nsm_connected.h"
#include "nsm_lp_serv.h"
#include "nsm_mpls_rib.h"
#include "nsm_mpls_api.h"
#include "nsm_message.h"
#include "nsm_router.h"
#ifdef HAVE_DSTE
#include "nsm_dste.h"
#endif /* HAVE_DSTE */
#ifdef HAVE_VPLS
#include "nsm_vpls.h"
#endif /* HAVE_VPLS */
#ifdef HAVE_NSM_MPLS_OAM
#include "nsm_mpls_oam.h"
#endif /* HAVE_NSM_MPLS_OAM */
#if defined (HAVE_BFD) && defined (HAVE_MPLS_OAM)
#include "nsm_mpls_bfd_cli.h"
#include "nsm_mpls_bfd.h"
#endif /* HAVE_BFD && HAVE_MPLS_OAM */

#ifdef HAVE_MPLS_VC
#ifdef HAVE_MS_PW
#include "nsm_mpls_ms_pw.h"
#include "hash.h"
#endif /* HAVE_MS_PW */
#endif /* HAVE_MPLS_VC */

#ifdef HAVE_VCCV
#include "mpls_util.h"
#endif /* HAVE_VCCV */

#define CHECK_LABEL 0

void
nsm_mpls_dump_log (struct cli *cli)
{
  struct nsm_master *nm = cli->vr->proto;
  int found = NSM_FALSE;

  if ((! NSM_MPLS) || (! NSM_MPLS->iflist))
    return;

  cli_out (cli, " Log message detail:");

  if (CHECK_FLAG (NSM_MPLS->kern_msgs, NSM_MPLS_SHOW_MSG_ERROR))
    {
      cli_out (cli, "\n  Error messages being logged by MPLS Forwarder");
      found = NSM_TRUE;
    }
  if (CHECK_FLAG (NSM_MPLS->kern_msgs, NSM_MPLS_SHOW_MSG_WARNING))
    {
      cli_out (cli, "\n  Warning messages being logged by MPLS Forwarder");
      found = NSM_TRUE;
    }
  if (CHECK_FLAG (NSM_MPLS->kern_msgs, NSM_MPLS_SHOW_MSG_DEBUG))
    {
      cli_out (cli, "\n  Debug messages being logged by MPLS Forwarder");
      found = NSM_TRUE;
    }
  if (CHECK_FLAG (NSM_MPLS->kern_msgs, NSM_MPLS_SHOW_MSG_NOTICE))
    {
      cli_out (cli, "\n  Notice messages being logged by MPLS Forwarder");
      found = NSM_TRUE;
    }

  if (found == NSM_FALSE)
    cli_out (cli, " none\n");
  else
    cli_out (cli, "\n");
}

#if (!defined (HAVE_GMPLS) || defined (HAVE_PACKET))
#ifdef HAVE_DIFFSERV
void
nsm_mpls_diffserv_dscp_exp_map_dump (struct cli *cli)
{
  struct nsm_master *nm = cli->vr->proto;
  int i;
  u_char found = NSM_FALSE;

  if (! NSM_MPLS)
    return;

  cli_out (cli, "MPLS Differentiated Services CLASS to EXP mapping data:\n");

  /* The title. */
  cli_out (cli, "CLASS    DSCP_value     EXP_value\n");

  for (i = 0; i < DIFFSERV_MAX_DSCP_EXP_MAPPINGS; i++)
    {
      if (NSM_MPLS->dscp_exp_map[i] != DIFFSERV_INVALID_DSCP)
        {
          found = NSM_TRUE;

          if (NSM_MPLS->dscp_exp_map[i] != DIFFSERV_INVALID_DSCP)
            cli_out (cli, "%4s      %d%d%d%d%d%d           %d\n",
                     diffserv_class_name(NSM_MPLS->dscp_exp_map[i]),
                     DSCPBITS(NSM_MPLS->dscp_exp_map[i]), i);
        }
    }

  if (found == NSM_FALSE)
    cli_out (cli, " none\n");
  cli_out (cli, "\n");
}

void
nsm_mpls_diffserv_configurable_dscp_dump (struct cli *cli)
{
  struct nsm_master *nm = cli->vr->proto;
  u_char i;
  u_char found = NSM_FALSE;

  if (! NSM_MPLS)
    return;

  cli_out (cli, "MPLS Differentiated Services Configurable Classes list:\n");

  for (i = 0; i < DIFFSERV_MAX_SUPPORTED_DSCP; i++)
    {
      if (diffserv_class_configurable (i))
        {
          if (found == NSM_FALSE)
            found = NSM_TRUE;

          cli_out (cli, "DSCP Class: %4s, value: %d%d%d%d%d%d\n",
                   diffserv_class_name(i),
                   DSCPBITS(i));
        }
    }

  if (found == NSM_FALSE)
    cli_out (cli, " none\n");
  cli_out (cli, "\n");
}

void
nsm_mpls_diffserv_supported_dscp_dump (struct cli *cli)
{
  struct nsm_master *nm = cli->vr->proto;
  u_char i;
  u_char found = NSM_FALSE;

  if (! NSM_MPLS)
    return;

  cli_out (cli, "MPLS Differentiated Services Supported Classes data:\n");

  /* The title. */
  cli_out (cli, "CLASS       DSCP_value\n");

  for (i = 0; i < DIFFSERV_MAX_SUPPORTED_DSCP; i++)
    {
      if (NSM_MPLS->supported_dscp[i] == DIFFSERV_DSCP_SUPPORTED)
        {
          found = NSM_TRUE;

          cli_out (cli, "%4s           %d%d%d%d%d%d\n",
                   diffserv_class_name(i),
                   DSCPBITS(i));
        }
    }

  if (found == NSM_FALSE)
    cli_out (cli, " none\n");
  cli_out (cli, "\n");
}

#endif /* HAVE_DIFFSERV */
#endif /* !HAVE_GMPLS || HAVE_PACKET */

#ifdef HAVE_VCCV
void
nsm_mpls_cli_set_bfd_cv_types (u_int8_t *cv_types, char * bfd_cv_type_str)
{
  if (!cv_types || !bfd_cv_type_str)
    return;

  if (pal_strncmp (bfd_cv_type_str, "1", 1) == 0)
    SET_FLAG (*cv_types, CV_TYPE_BFD_IPUDP_DET_BIT);
  else if (pal_strncmp (bfd_cv_type_str, "2", 1) == 0)
    SET_FLAG (*cv_types, CV_TYPE_BFD_IPUDP_DET_SIG_BIT);
  else if (pal_strncmp (bfd_cv_type_str, "3", 1) == 0)
    SET_FLAG (*cv_types, CV_TYPE_BFD_ACH_DET_BIT);
  else
    SET_FLAG (*cv_types, CV_TYPE_BFD_ACH_DET_SIG_BIT);
}

int
nsm_mpls_parse_vccv_args (struct cli* cli, char **argv, int argc,
                          s_int32_t fec_type_vc, u_char c_word, int *count,
                          u_int8_t* cc_types, u_int8_t *cv_types)
{
  int i = 0;

  if (!count)
    return CLI_SUCCESS;

  /* copy the current count of args */
  i = *count;

  if (pal_strncmp (argv[i], "vccv", 4) != 0)
    return CLI_SUCCESS;


  /* If specific cc type is configured */
  if ((i+1 < argc) && pal_strncmp (argv[i+1], "c", 1) == 0)
    {
      if (!c_word && (pal_strncmp (argv[i+2], "1", 1) == 0))
        {
          cli_out (cli, "%% Cannot configure CC type 1, when CW is"
              " not in use\n");
          return CLI_ERROR;
        }
      else if (c_word && (pal_strncmp (argv[i+2], "1", 1) != 0))
        {
          cli_out (cli, "%% CC type 1 should be configured when CW is"
              " in use\n");
          return CLI_ERROR;
        }
      nsm_mpls_set_cc_types (cc_types, argv[i+2]);
      i+=2;
    }
  else
    (*cc_types) |= nsm_mpls_get_default_cc_types (c_word);

  /* Set the default cv types */
  (*cv_types) |= nsm_mpls_get_default_cv_types ();

  /* If BFD VCCV is desired */
  if ((i+1 < argc) && pal_strncmp (argv[i+1], "b", 1) == 0)
    {
      i++;
      /* If specific BFD cv type is configured */
      if ((i+1 < argc) && pal_strncmp (argv[i+1], "b", 1) == 0)
        {
          if ((fec_type_vc != PW_OWNER_MANUAL) &&
              ((pal_strncmp (argv[i+2], "2", 1) == 0) ||
               (pal_strncmp (argv[i+2], "4", 1) == 0)))
            {
              cli_out (cli, "%% Cannot configure BFD CV type %s"
                  " for Dynamic Vc's\n", argv[i+2]);
              return CLI_ERROR;
            }

          if ((!c_word) &&
              ((pal_strncmp (argv[i+2], "3", 1) == 0) ||
               (pal_strncmp (argv[i+2], "4", 1) == 0)))
            {
              cli_out (cli, "%% Cannot configure BFD CV type %s"
                  " if control word is not configured\n", argv[i+2]);
              return CLI_ERROR;
            }

          nsm_mpls_cli_set_bfd_cv_types (cv_types, argv[i+2]);
          i+=2;
        }
      else
        {
          (*cv_types) |= nsm_mpls_get_default_bfd_cv_types
            ((fec_type_vc == PW_OWNER_MANUAL), c_word);
        }
    }

  *count = i;

  return CLI_SUCCESS;
}
#endif /* HAVE_VCCV */

/* Show PacOS MPLS data. */
CLI (show_mpls,
     show_mpls_cmd,
     "show mpls",
     CLI_SHOW_STR,
     SHOW_MPLS_STR)
{
  struct nsm_master *nm = cli->vr->proto;

  if ((! NSM_MPLS) || (! NSM_MPLS->iflist))
    return CLI_SUCCESS;

#if (!defined (HAVE_GMPLS) || defined (HAVE_PACKET))

  /* Minimum label value. */
  cli_out (cli, " Minimum label configured: %d\n",
           NSM_MPLS->min_label_val);

  /* Maximum label value. */
  cli_out (cli, " Maximum label configured: %d\n",
           NSM_MPLS->max_label_val);
#endif /* !HAVE_GMPLS || HAVE_PACKET  */

  {
    struct nsm_label_space *nls;
    struct route_node *rn;
    bool_t found = NSM_FALSE;

    cli_out (cli, " Per label-space information: ");

    for (rn = route_top (NSM_MPLS->ls_table); rn; rn = route_next (rn))
      {
        nls = rn->info;
        if (! nls)
          continue;

        if (found == NSM_FALSE)
          found = NSM_TRUE;

        cli_out (cli, "\n");
        cli_out (cli, "   Label-space %d is using minimum label: %d "
                 "and maximum label: %d", nls->label_space,
                 nls->min_label_val, nls->max_label_val);
      }
    if (found == NSM_FALSE)
      cli_out (cli, "none\n");
    else
      cli_out (cli, "\n");
  }

#if (!defined (HAVE_GMPLS) || defined (HAVE_PACKET))
  /* Custom ingress ttl. */
  if (NSM_MPLS->ingress_ttl == NSM_MPLS_NO_TTL)
    cli_out (cli, " Custom ingress TTL configured: none\n");
  else
    cli_out (cli, " Custom ingress TTL configured: %d\n",
             NSM_MPLS->ingress_ttl);

  /* Custom egress ttl. */
  if (NSM_MPLS->egress_ttl == NSM_MPLS_NO_TTL)
    cli_out (cli, " Custom egress TTL configured: none\n");
  else
    cli_out (cli, " Custom egress TTL configured: %d\n",
             NSM_MPLS->ingress_ttl);
#endif /* !HAVE_GMPLS || HAVE_PACKET */

  /* Kernel messages. */
  nsm_mpls_dump_log (cli);

#ifdef HAVE_TE
  /* Admin groups. */
  admin_group_dump (cli, NSM_MPLS->admin_group_array);
#endif /* HAVE_TE */

#ifdef HAVE_MPLS_FWD
  {
    char buf[BUFSIZ];

    pal_mpls_fib_stats_update (NSM_ZG, &NSM_MPLS->stats);
    pal_mpls_fib_stats (&NSM_MPLS->stats, buf, BUFSIZ);
    if (pal_strcmp (buf, ""))
      cli_out (cli, "%s\n", buf);
  }
#endif /* HAVE_MPLS_FWD */

#if (!defined (HAVE_GMPLS) || defined (HAVE_PACKET))
#ifdef HAVE_DIFFSERV
  cli_out (cli, "\n");
  /* Diffserv info dump. */
  nsm_mpls_diffserv_supported_dscp_dump (cli);
  nsm_mpls_diffserv_dscp_exp_map_dump (cli);
#endif /* HAVE_DIFFSERV */
#endif /* !HAVE_GMPLS || HAVE_PACKET */

  return CLI_SUCCESS;
}

/* Show interfaces that are part of the MPLS object. */
CLI (show_mpls_if,
     show_mpls_if_cmd,
     "show mpls interface",
     CLI_SHOW_STR,
     SHOW_MPLS_STR,
     "Show interfaces bound to MPLS interface")
{
  struct nsm_master *nm = cli->vr->proto;
  struct nsm_mpls_if *mif;
  struct interface *ifp;
  struct listnode *ln;
  int i;
  int mif_cnt = 0;

  if (! NSM_MPLS || ! LISTCOUNT (NSM_MPLS->iflist))
    return CLI_SUCCESS;

#ifdef HAVE_MPLS_FWD
  /* Get mpls interface statistics */
  pal_mpls_if_stats_update (nm);
#endif /* HAVE_MPLS_FWD */

  i = 0;
  LIST_LOOP (NSM_MPLS->iflist, mif, ln)
    {
      ifp = mif->ifp;
      if (ifp)
        cli_out (cli, "Interface %s\n", ifp->name);

      if (mif->nls)
        {
          /* Label space data. */
          cli_out (cli, "  Label switching is enabled");
          cli_out (cli, " with label-space %d\n", mif->nls->label_space);
          cli_out (cli, "   minimum label value configured is %d\n",
                   ((mif->nls->min_label_val != 0) ?
                    mif->nls->min_label_val :
                    nsm_mpls_min_label_val_get (nm)));
          cli_out (cli, "   maximum label value configured is %d\n",
                   ((mif->nls->max_label_val != 0) ?
                    mif->nls->max_label_val :
                    nsm_mpls_max_label_val_get (nm)));
          mif_cnt ++;
#ifdef HAVE_MPLS_FWD
          {
            char buf[BUFSIZ];

            pal_mpls_if_entry_stats (&mif->stats, buf, BUFSIZ);
            if (pal_strcmp (buf, ""))
              cli_out (cli, "%s", buf);
          }
#endif /* HAVE_MPLS_FWD */
        }
      else
        {
          /* Label space data. */
          cli_out (cli, "  Label switching is disabled");
          cli_out (cli, "\n");
        }
      i++;
    }

#ifdef HAVE_MPLS_FWD
  cli_out (cli, "\n");
  cli_out (cli, "Non mpls interface statistics (in-labels used:platform wide label space)\n");
    {
      char buf[BUFSIZ];

      pal_mpls_if_entry_stats (&nm->nmpls->if_stats, buf, BUFSIZ);
      if (pal_strcmp (buf, ""))
        cli_out (cli, "%s", buf);
    }
#endif /* HAVE_MPLS_FWD */

  cli_out (cli, "\n");
  cli_out (cli, "Total number of mpls interface is %d\n", mif_cnt);

  return CLI_SUCCESS;
}

#if (!defined (HAVE_GMPLS) || defined (HAVE_PACKET))
#ifdef HAVE_DIFFSERV
void
nsm_mpls_diffserv_info_dump (struct cli *cli, struct ds_info *p)
{
  u_char i;

  if (p->lsp_type == LLSP)
    {
      cli_out (cli, "  LSP Type: LLSP, DSCP value: %d%d%d%d%d%d, DS Class: %s",
               DSCPBITS(p->dscp), diffserv_class_name(p->dscp));
      if (p->af_set == NSM_TRUE)
        cli_out (cli, " %s %s\n", diffserv_class_name(p->dscp+2),
                 diffserv_class_name(p->dscp+4));
      else
        cli_out (cli, "\n");
    }
  else
    {
      cli_out (cli, "LSP Type: %s\n",
               (p->lsp_type == ELSP_SIGNAL) ?
               "ELSP_SIGNAL" : "ELSP_CONFIG");
      cli_out (cli, "Class_Exp mapping:\n");
      cli_out (cli, "CLASS    DSCP_value     EXP_value\n");

      /* Output of the value.*/
      for (i = 0; i < DIFFSERV_MAX_DSCP_EXP_MAPPINGS; i++)
        if (p->dscp_exp_map[i] != DIFFSERV_INVALID_DSCP)
          cli_out (cli, "%4s      %d%d%d%d%d%d           %d\n",
                   diffserv_class_name(p->dscp_exp_map[i]),
                   DSCPBITS(p->dscp_exp_map[i]), i);
    }

  return;
}
#endif /* HAVE_DIFFSERV */
#endif /* !HAVE_GMPLS || HAVE_PACKET */

/* Dump NHLFE entry. */
void
nsm_mpls_nhlfe_entry_dump (struct cli *cli, struct nhlfe_entry *nhlfe,
                           mpls_nhlfe_owner_t n_owner)
{
  struct nsm_master *nm = cli->vr->proto;
  struct interface *ifp = NULL;
  u_char opcode;
  char *name = NULL;
  struct nhlfe_key *key = NULL;
  struct pal_in4_addr def_addr;
  bool_t is_default = PAL_FALSE;
#ifdef HAVE_GMPLS
#ifdef HAVE_PBB_TE
struct nhlfe_pbb_key *pbb_key = NULL;
#endif /* HAVE_PBB_TE */
#endif /* HAVE_GMPLS */

  if (! nhlfe)
    return;

  def_addr.s_addr = pal_hton32 (DEFAULT_NEXTHOP_ADDR);

  opcode = nhlfe->opcode;
  if (n_owner == MPLS_NHLFE_OWNER_ILM && opcode == DLVR_TO_IP)
    opcode = POP_FOR_VPN;
  else if (n_owner == MPLS_NHLFE_OWNER_FTN && opcode == POP_FOR_VPN)
    opcode = DLVR_TO_IP;

  switch (nhlfe->type)
  {
    case gmpls_entry_type_ip:
      key = (struct nhlfe_key *) nhlfe->nkey;

      if (key->afi == AFI_IP)
        {
          NSM_MPLS_GET_INDEX_PTR (CHECK_FLAG (nhlfe->flags,
                                              NHLFE_ENTRY_FLAG_GMPLS),
                                  key->u.ipv4.oif_ix, ifp);
          if(ifp)
            name = ifp->name;

          cli_out (cli, "   Out-segment with ix: %d, owner: %s, out intf: %s, "
                   "out label: %d\n",
                   nhlfe->nhlfe_ix,
                   mpls_dump_owner (nhlfe->owner), (name == NULL) ?
                   "N/A" : name, (key->u.ipv4.out_label == LABEL_VALUE_INVALID ?
                   "N/A" : key->u.ipv4.out_label));

          if (IPV4_ADDR_CMP(&key->u.ipv4.nh_addr, &def_addr.s_addr) == 0)
            is_default = PAL_TRUE;

          if (is_default != PAL_TRUE)
            cli_out (cli, "    Nexthop addr: %r",&key->u.ipv4.nh_addr);
          else
            cli_out (cli, "    Nexthop addr: %s", "-");

          cli_out(cli, "    cross connect ix: %d, "
                  "op code: %s\n",
                  nhlfe->xc_ix,
                  nsm_mpls_dump_op_code (opcode));
        }
#ifdef HAVE_IPV6
      else if (key->afi == AFI_IP6)
        {
          NSM_MPLS_GET_INDEX_PTR (CHECK_FLAG (nhlfe->flags,
                                              NHLFE_ENTRY_FLAG_GMPLS),
                                  key->u.ipv6.oif_ix, ifp);
          if(ifp)
            name = ifp->name;

          cli_out (cli, "   Out-segment with ix: %d, owner: %s, out intf: %s, "
                   "out label: %d\n",
                   nhlfe->nhlfe_ix,
                   mpls_dump_owner (nhlfe->owner),
                   (name == NULL) ? "N/A" : name,
                   key->u.ipv6.out_label);
          cli_out (cli, "    Nexthop addr: %40R, cross connect ix: %d, "
                   "\nop code: %s\n",
                   &key->u.ipv6.nh_addr,
                   nhlfe->xc_ix,
                   nsm_mpls_dump_op_code (opcode));
        }
#endif /* HAVE_IPV6 */
#ifdef HAVE_GMPLS
      else if (key->afi == AFI_UNNUMBERED)
        {
          NSM_MPLS_GET_INDEX_PTR (CHECK_FLAG (nhlfe->flags,
                                              NHLFE_ENTRY_FLAG_GMPLS),
                                  key->u.unnum.oif_ix, ifp);
          if(ifp)
            name = ifp->name;

          cli_out (cli, "   Out-segment with ix: %d, owner: %s, out intf: %s, "
                   "out label: %d\n",
                   nhlfe->nhlfe_ix,
                   mpls_dump_owner (nhlfe->owner),
                   ((name == NULL) ? "N/A" : name),
                   key->u.unnum.out_label);
          cli_out (cli, "    Nexthop addr: %40d, cross connect ix: %d, "
                   "\nop code: %s\n",
                   &key->u.unnum.id, nhlfe->xc_ix,
                   nsm_mpls_dump_op_code (opcode));
        }
#endif /* HAVE_GMPLS */
#ifdef HAVE_MPLS_FWD
      if (n_owner == MPLS_NHLFE_OWNER_NHLFE)
        {
          char buf[BUFSIZ];

          pal_mpls_nhlfe_entry_stats (&nhlfe->stats, buf, BUFSIZ);
          if (pal_strcmp (buf, ""))
            cli_out (cli, "%s\n", buf);
        }
#endif /* HAVE_MPLS_FWD */
      break;

#ifdef HAVE_GMPLS
#ifdef HAVE_PBB_TE
    case gmpls_entry_type_pbb_te:
      pbb_key = (struct nhlfe_pbb_key *) nhlfe->nkey;
      NSM_MPLS_GET_INDEX_PTR (CHECK_FLAG (nhlfe->flags,
                                          NHLFE_ENTRY_FLAG_GMPLS),
                              pbb_key->oif_ix, ifp);
      if(ifp)
        name = ifp->name;

      cli_out (cli, "   Out-segment with ix: %d, owner: %s, out intf: %s, "
                   "out label: [%.04hx.%.04hx.%.04hx,%d]\n",
                   nhlfe->nhlfe_ix,
                   mpls_dump_owner (nhlfe->owner),
                   ((name == NULL) ? "N/A" : name),
                   pal_ntoh16(((unsigned short *)pbb_key->lbl.bmac)[0]),
                   pal_ntoh16(((unsigned short *)pbb_key->lbl.bmac)[1]),
                   pal_ntoh16(((unsigned short *)pbb_key->lbl.bmac)[2]),
                   pbb_key->lbl.bvid);
      cli_out (cli, "    Cross connect ix: %d, \nop code: %s\n",
                   nhlfe->xc_ix, nsm_mpls_dump_op_code (opcode));
      break;
#endif /* HAVE_PBB_TE */
#endif /* HAVE_GMPLS */

    default:
      return;
  }
}

/* Dump XC entry. */
void
nsm_mpls_xc_entry_dump (struct cli *cli, struct xc_entry *xc,
                        mpls_nhlfe_owner_t n_owner)
{
  struct nsm_master *nm = cli->vr->proto;
  struct interface *ifp;
#if (defined (HAVE_GMPLS) && defined (HAVE_PBB_TE))
  u_int8_t zero_mac[ETHER_ADDR_LEN] = {0};
#endif /* HAVE_GMPLS && HAVE_PBB_TE */

  if (! xc)
    return;

  NSM_MPLS_GET_INDEX_PTR (CHECK_FLAG (xc->flags, XC_ENTRY_FLAG_GMPLS),
                          xc->key.iif_ix, ifp);

  switch (xc->type)
  {
    case gmpls_entry_type_ip:
      cli_out (cli, "  Cross connect ix: %d, in intf: %s "
           "in label: %d "
           "out-segment ix: %d\n",
           xc->key.xc_ix,
           (ifp == NULL) ? "-" : ifp->name,
           xc->key.in_label.u.pkt,
           xc->key.nhlfe_ix);
      break;

#if (defined (HAVE_GMPLS) && defined (HAVE_PBB_TE))
    case gmpls_entry_type_pbb_te:
      cli_out (cli, "  Cross connect ix: %d, in intf: %s ",
           xc->key.xc_ix,
           (ifp == NULL) ? "-" : ifp->name);

      /* If the mac/vlan is a valid vlan other than 0, then label is valid */
      if((pal_mem_cmp (xc->key.in_label.u.pbb.bmac, zero_mac, ETHER_ADDR_LEN) != 0) &&
         (xc->key.in_label.u.pbb.bvid))
        {
          cli_out (cli, "in label: [%.04hx.%.04hx.%.04hx,%d]",
               pal_ntoh16(((unsigned short *)xc->key.in_label.u.pbb.bmac)[0]),
               pal_ntoh16(((unsigned short *)xc->key.in_label.u.pbb.bmac)[1]),
               pal_ntoh16(((unsigned short *)xc->key.in_label.u.pbb.bmac)[2]),
               xc->key.in_label.u.pbb.bvid);
        }
      else
        cli_out (cli, "%s", "in label: - ");

      cli_out (cli, "out-segment ix: %d\n",
           xc->key.nhlfe_ix);

      break;
#endif /* HAVE_GMPLS && HAVE_PBB_TE */

    default:
      break;
  }

  cli_out (cli, "    Owner: %s, Persistent: %s, Admin Status: %s, "
           "Oper Status: %s\n",
           mpls_dump_owner (xc->owner),
           ((xc->persistent == NSM_TRUE) ? "Yes" : "No"),
           nsm_mpls_dump_admn_status (xc->admn_status),
           nsm_mpls_dump_opr_status (xc->opr_status));

  if (xc->nhlfe)
    nsm_mpls_nhlfe_entry_dump (cli, xc->nhlfe, n_owner);
}

/* Dump ILM entry. */
void
nsm_mpls_ilm_entry_dump (struct cli *cli, struct ilm_entry *ilm)
{
  struct nsm_master *nm = cli->vr->proto;
  struct interface *ifp;
  struct prefix fec_prefix;
  struct ilm_key *key;
  char *name = NULL;
  u_int32_t ifIndex;
#ifdef HAVE_GMPLS
#ifdef HAVE_PBB_TE
struct ilm_key_pbb *pbb_key = NULL;
#endif /* HAVE_PBB_TE */
#endif /* HAVE_GMPLS */

  if (! ilm)
    return;

  switch (ilm->ent_type)
  {
    case ILM_ENTRY_TYPE_PACKET:
      key = &ilm->key.u.pkt;
      ifIndex = key->iif_ix;

      NSM_MPLS_GET_INDEX_PTR (PAL_TRUE, ifIndex, ifp);
      if(ifp)
        name = ifp->name;

      fec_prefix.family = ilm->family;
      fec_prefix.prefixlen = ilm->prefixlen;

#ifdef HAVE_IPV6
      if (fec_prefix.family == AF_INET6)
        pal_mem_cpy (&fec_prefix.u.prefix6, ilm->prfx,
                     sizeof (struct pal_in6_addr));
      else
#endif
        pal_mem_cpy (&fec_prefix.u.prefix4, ilm->prfx,
                     sizeof (struct pal_in4_addr));

      cli_out (cli, "  Owner: %s, # of pops: %d, fec: %O\n",
           mpls_dump_owner (ilm->owner.owner),
           ilm->n_pops,
           &fec_prefix);

#ifdef HAVE_MPLS_FWD
      {
        char buf[BUFSIZ];

        pal_mpls_ilm_entry_stats (&ilm->stats, buf, BUFSIZ);
        if (pal_strcmp (buf, ""))
          cli_out (cli, "%s\n", buf);
      }
#endif /* HAVE_MPLS_FWD */

#if (!defined (HAVE_GMPLS) || defined (HAVE_PACKET))
#ifdef HAVE_DIFFSERV
      if (ilm->ds_info.lsp_type != LSP_DEFAULT)
        nsm_mpls_diffserv_info_dump (cli, &(ilm->ds_info));
#endif /* HAVE_DIFFSERV */
#endif /* !HAVE_GMPLS || HAVE_PACKET */
      break;

#ifdef HAVE_GMPLS
#ifdef HAVE_PBB_TE
    case ILM_ENTRY_TYPE_PBB_TE:
      pbb_key = &ilm->key.u.pbb_key;
      ifIndex = pbb_key->iif_ix;

      NSM_MPLS_GET_INDEX_PTR (PAL_TRUE, ifIndex, ifp);
      if(ifp)
        name = ifp->name;

      fec_prefix.family = ilm->family;
      fec_prefix.prefixlen = ilm->prefixlen;

      cli_out (cli, " In-segment entry with in label: [%.04hx.%.04hx.%.04hx,%d], \
           id: %u, in intf: %s, "
           "row status: %s\n",
           pal_ntoh16(((unsigned short *)pbb_key->in_label.bmac)[0]),
           pal_ntoh16(((unsigned short *)pbb_key->in_label.bmac)[1]),
           pal_ntoh16(((unsigned short *)pbb_key->in_label.bmac)[2]),
           pbb_key->in_label.bvid,
           ilm->ilm_ix,
           ((name == NULL) ? "-" : name),
           nsm_mpls_dump_row_status (ilm->row_status));
      break;
#endif /* HAVE_PBB_TE */
#endif /* HAVE_GMPLS */

    default:
      return;
  }
  if (ilm->xc)
    nsm_mpls_xc_entry_dump (cli, ilm->xc, MPLS_NHLFE_OWNER_ILM);

  return;
}

/* Dump FTN entry. */
void
nsm_mpls_ftn_entry_dump (struct cli *cli, struct ftn_entry *ftn,
                         struct prefix *fec, bool_t is_primary)
{
  cli_out (cli, " %s FTN entry with FEC: %O, id: %d, row status: %s\n",
           (is_primary ? "Primary" : "Non-primary"),
           fec,
           ftn->ftn_ix,
           nsm_mpls_dump_row_status (ftn->row_status));
  cli_out (cli, "  Owner: %s, Action-type: %s, Exp-bits: 0x%d, "
           "Incoming DSCP: %s\n",
           mpls_dump_owner (ftn->owner.owner),
           mpls_dump_action_type (ftn->lsp_bits.bits.act_type),
           ftn->exp_bits,
           ftn->dscp_in == DIFFSERV_INVALID_DSCP
           ? "none" : diffserv_class_name (ftn->dscp_in));
  cli_out (cli, "  Tunnel id: %u, ", ftn->tun_id);
  cli_out (cli, "  Protected LSP id: %u, ", ftn->protected_lsp_id);
  if (ftn->lsp_bits.bits.act_type == REDIRECT_TUNNEL)
    cli_out (cli, "QoS Resource id: %u, ", ftn->qos_resrc_id);
  cli_out (cli, "Description: %s\n",
           ftn->sz_desc ? ftn->sz_desc : "N/A");

#ifdef HAVE_GMPLS
#ifdef HAVE_PBB_TE
  if((ftn->tgen_data) &&
     (((struct gmpls_tgen_data *)ftn->tgen_data)->gmpls_type ==
                                         gmpls_entry_type_pbb_te))
    cli_out (cli, "  Pbb_te tesid: %d\n",
            (ftn->tgen_data) ?
            ((struct gmpls_tgen_data *)ftn->tgen_data)->u.pbb.tesid : 0);
#endif /*HAVE_PBB_TE */
#endif /* HAVE_GMPLS */

#ifdef HAVE_MPLS_FWD
  {
    char buf[BUFSIZ];

    pal_mpls_ftn_entry_stats (&ftn->stats, buf, BUFSIZ);
    if (pal_strcmp (buf, ""))
      cli_out (cli, "%s\n", buf);
   }
#endif /* HAVE_MPLS_FWD */
  if (ftn->xc)
    nsm_mpls_xc_entry_dump (cli, ftn->xc, MPLS_NHLFE_OWNER_FTN);

#if (!defined (HAVE_GMPLS) || defined (HAVE_PACKET))
#ifdef HAVE_DIFFSERV
  if (ftn->ds_info.lsp_type != LSP_DEFAULT)
    nsm_mpls_diffserv_info_dump (cli, &(ftn->ds_info));
#endif /* HAVE_DIFFSERV */
#endif /* !HAVE_GMPLS ||  HAVE_PACKET */

#if defined (HAVE_BFD) && defined (HAVE_MPLS_OAM)
  if (CHECK_FLAG (ftn->bfd_flag, NSM_MPLS_BFD_CONFIGURED))
    cli_out (cli, "  Bidirectional Forwarding Detection is configured\n");

  if (CHECK_FLAG (ftn->bfd_flag, NSM_MPLS_BFD_ENABLED))
    cli_out (cli, "  Bidirectional Forwarding Detection is enabled\n");

  if (CHECK_FLAG (ftn->bfd_flag, NSM_MPLS_BFD_DISABLED))
    cli_out (cli, "  Bidirectional Forwarding Detection is Disabled\n");
#endif /* HAVE_BFD && HAVE_MPLS_OAM */
}

/* Show FTN Data. */
CLI (mpls_ftn_table,
     mpls_ftn_table_cmd,
     "show mpls ftn-table",
     CLI_SHOW_STR,
     SHOW_MPLS_STR,
     "Show MPLS FTN table")
{
  struct nsm_master *nm = cli->vr->proto;
  struct ftn_entry *ftn, *list;
  struct ptree_node *pn;
  struct ptree *pt = FTN_TABLE4;
  bool_t done = NSM_FALSE;
  struct fec_gen_entry fec;

  if (! NSM_MPLS ||
#ifdef HAVE_IPV6
     ! FTN_TABLE6 ||
#endif
      ! FTN_TABLE4)
    return CLI_SUCCESS;

#ifdef HAVE_MPLS_FWD
  pal_mpls_ftn_stats_update (nm);
#endif /* HAVE_MPLS_FWD */

  while (!done)
    {
      for (pn = ptree_top (pt); pn; pn = ptree_next (pn))
        {
          list = pn->info;
          if (! list)
            continue;

          for (ftn = list; ftn; ftn = ftn->next)
            {
              nsm_gmpls_set_fec_from_ftn (ftn, &fec);
              nsm_mpls_ftn_entry_dump (cli, ftn, &fec.u.prefix,
                                       CHECK_FLAG (ftn->flags,
                                       FTN_ENTRY_FLAG_PRIMARY));
              cli_out (cli, "\n");
            }
        }
#ifdef HAVE_IPV6
      if (pt != FTN_TABLE6)
        pt = FTN_TABLE6;
      else
#endif
        done = NSM_TRUE;
    }

  return CLI_SUCCESS;
}

#if (!defined (HAVE_GMPLS) || defined (HAVE_PACKET))
#ifdef HAVE_DIFFSERV
void
nsm_mpls_diffserv_lsp_type_dump (struct cli *cli, u_char type)
{
  switch (type)
    {
    case LSP_DEFAULT:
      cli_out (cli, "LSP_DEFAULT");
      break;
    case ELSP_CONFIG:
      cli_out (cli, "ELSP_CONFIG");
      break;
    case ELSP_SIGNAL:
      cli_out (cli, "ELSP_SIGNAL");
      break;
    case LLSP:
      cli_out (cli, "LLSP");
      break;
    }
}
#endif /* HAVE_DIFFSERV */
#endif /* !HAVE_GMPLS || HAVE_PACKET*/

/* Show MPLS IPv4 Forwarding table data. */
CLI (mpls_fwd_table,
     mpls_fwd_table_cmd,
     "show mpls forwarding-table",
     CLI_SHOW_STR,
     SHOW_MPLS_STR,
     "Show MPLS Forwarding table")
{
  struct nsm_master *nm = cli->vr->proto;
  struct ftn_entry *ftn, *list;
  struct xc_entry *xc;
  struct nhlfe_entry *nhlfe;
  struct interface *ifp = NULL;
  struct ptree_node *pn;
  bool_t heading = NSM_FALSE; 
  bool_t done = NSM_FALSE;
  struct ptree *pt = FTN_TABLE4;
  struct fec_gen_entry fec;
  struct nhlfe_key *key = NULL;
  char *name = NULL;
  struct pal_in4_addr def_addr;
  bool_t is_default = PAL_FALSE;

  if (! NSM_MPLS ||
#ifdef HAVE_IPV6
      ! FTN_TABLE6 ||
#endif
      ! FTN_TABLE4)
    return CLI_SUCCESS;

  def_addr.s_addr = pal_hton32 (DEFAULT_NEXTHOP_ADDR);

  while (!done)
    {
      for (pn = ptree_top (pt); pn; pn = ptree_next (pn))
        {
          list = pn->info;
          if (! list)
            continue;

          for (ftn = list; ftn; ftn = ftn->next)
            {
#ifdef HAVE_GMPLS
#ifdef HAVE_PBB_TE
              /* If created for PBB_TE tunnel don't display it */
              if((ftn->tgen_data) &&
                 (((struct gmpls_tgen_data *)ftn->tgen_data)->gmpls_type ==
                                                     gmpls_entry_type_pbb_te))
                continue;
#endif /*HAVE_PBB_TE */
#endif /* HAVE_GMPLS */
              is_default = PAL_FALSE;
              nsm_gmpls_set_fec_from_ftn (ftn, &fec);

              /* Header. */
              if (heading == NSM_FALSE)
                {
                  heading = NSM_TRUE;
                  cli_out (cli,
                          "Codes: > - selected FTN, p - stale FTN, B - BGP FTN"
                          ", K - CLI FTN,\n"
                          "       L - LDP FTN, R - RSVP-TE FTN, S - SNMP FTN, "
                          "I - IGP-Shortcut,\n"
                          "       U - unknown FTN\n\n");

                  cli_out (cli,
                         "Code    FEC                 Tunnel-id   FTN-ID    "
                         "Pri  Nexthop          Out-Label    Out-Intf    ");
#if (!defined (HAVE_GMPLS) || defined (HAVE_PACKET))
#ifdef HAVE_DIFFSERV
                  cli_out (cli, " LSP-Type");
#endif /* HAVE_DIFFSERV */
#endif /* !HAVE_GMPLS || HAVE_PACKET */
                  cli_out (cli, "\n");
                }

              if (ftn->row_status == RS_ACTIVE)
                {
                  xc = FTN_XC (ftn);
                  nhlfe = FTN_NHLFE (ftn);
                }
              else
                {
                  xc = NULL;
                  nhlfe = NULL;
                }

              if (nhlfe)
                {
                  key = (struct nhlfe_key *) nhlfe->nkey;
                  if (key->afi == AFI_IP)
                    {
                      NSM_MPLS_GET_INDEX_PTR (CHECK_FLAG (ftn->flags,
                                                          FTN_ENTRY_FLAG_GMPLS),
                                              key->u.ipv4.oif_ix, ifp);
                      if(ifp)
                        name = ifp->name;
                    }
#ifdef HAVE_IPV6
                  else if (key->afi == AFI_IP6)
                    {
                      NSM_MPLS_GET_INDEX_PTR (CHECK_FLAG (ftn->flags,
                                                          FTN_ENTRY_FLAG_GMPLS),
                                              key->u.ipv6.oif_ix, ifp);
                      if(ifp)
                        name = ifp->name;
                    }
#endif /* HAVE_IPV6 */
#ifdef HAVE_GMPLS
                  else if (key->afi == AFI_UNNUMBERED)
                    {
                      NSM_MPLS_GET_INDEX_PTR (CHECK_FLAG (ftn->flags,
                                                          FTN_ENTRY_FLAG_GMPLS),
                                              key->u.unnum.oif_ix, ifp);
                      if(ifp)
                        name = ifp->name;
                    }
#endif /* HAVE_GMPLS */
                }
              else
                ifp = NULL;

#ifdef HAVE_IPV6
              if (pt == FTN_TABLE6)
                {
                  cli_out (cli, "%s%s %s    %-20O%-12d%-10d%-5s",
                      mpls_dump_owner_char (ftn->owner.owner),
                      (CHECK_FLAG (ftn->flags, FTN_ENTRY_FLAG_SELECTED)
                       ? ">" : " "),
                      (CHECK_FLAG (ftn->flags, FTN_ENTRY_FLAG_STALE)
                       ? "p" : " "),
                      &fec.u.prefix,
                      ftn->tun_id,
                      ftn->ftn_ix,
                      (CHECK_FLAG (ftn->flags, FTN_ENTRY_FLAG_PRIMARY) ?
                       "Yes" : "No"));
                  if (nhlfe != NULL)
                    {                  
                      if (key->afi == AFI_IP6)
                        {
                          cli_out (cli, "%-17R%-13d%-13s", 
                              &key->u.ipv6.nh_addr,
                              key->u.ipv6.out_label,
                              (name == NULL) ? "-" : name);
                        }
                      else
                        {
                          cli_out (cli, "%-17r%-13d%-13s", 
                              &key->u.ipv4.nh_addr,
                              key->u.ipv4.out_label,
                              (name == NULL) ? "-" : name);
                        }
                    }
                  else
                    cli_out (cli, "%-17s%-13d%-13s", "-", -1, "-");
                }
              else
#endif /* HAVE_IPV6 */
                {
                  cli_out (cli, "%s%s %s    %-20O%-12d%-10d%-5s",
                           mpls_dump_owner_char (ftn->owner.owner),
                           (CHECK_FLAG (ftn->flags, FTN_ENTRY_FLAG_SELECTED)
                           ? ">" : " "),
                           (CHECK_FLAG (ftn->flags, FTN_ENTRY_FLAG_STALE)
                           ? "p" : " "),
                           &fec.u.prefix,
                           ftn->tun_id,
                           ftn->ftn_ix,
                           (CHECK_FLAG (ftn->flags, FTN_ENTRY_FLAG_PRIMARY) ?
                           "Yes" : "No"));

                  if ((nhlfe != NULL) && IPV4_ADDR_CMP(&key->u.ipv4.nh_addr,
                                                       &def_addr.s_addr) == 0)
                    is_default = PAL_TRUE;

                  if (is_default != PAL_TRUE)
                    cli_out (cli, "%-17r",  &key->u.ipv4.nh_addr);
                  else
                    cli_out (cli, "%-17s", "-");

                  cli_out (cli, "%-13d%-13s", ((nhlfe == NULL) ? -1 :
                           key->u.ipv4.out_label),
                           ((ifp == NULL) ? "-" : ifp->name));
                }
#if (!defined (HAVE_GMPLS) || defined (HAVE_PACKET))
#ifdef HAVE_DIFFSERV
              nsm_mpls_diffserv_lsp_type_dump (cli, ftn->ds_info.lsp_type);
#endif /* HAVE_DIFFSERV */
#endif /* !HAVE_GMPLS || HAVE_PACKET */
              cli_out (cli,"\n");
            }
        }
#ifdef HAVE_IPV6
    if (pt != FTN_TABLE6)
      pt = FTN_TABLE6;
    else
#endif
      done = NSM_TRUE;
    }

  return CLI_SUCCESS;
}

void
nsm_mpls_ilm_entry_show (struct cli *cli, struct ilm_entry *ilm)
{
  struct nsm_master *nm = cli->vr->proto;
  struct xc_entry *xc;
  struct ilm_key *ikey;
  struct nhlfe_entry *nhlfe;
  struct nhlfe_key *key = NULL;
  struct interface *ifp_in, *ifp_out;
  struct pal_in4_addr nh_dmy_addr;
  struct pal_in4_addr def_addr;
  bool_t is_default = PAL_FALSE;
#ifdef HAVE_IPV6
  struct pal_in6_addr nh_dmy_addr6;
#endif
  struct prefix fec_prefix;
#ifdef HAVE_GMPLS
#ifdef HAVE_PBB_TE
  struct ilm_key_pbb *il_pbb_key = NULL;
  struct nhlfe_pbb_key *nh_pbb_key = NULL;
  u_int8_t zero_mac[ETHER_ADDR_LEN] = {0};
#endif /* HAVE_PBB_TE */
#endif /* HAVE_GMPLS */
  char *in_name = NULL, *out_name = NULL;

  def_addr.s_addr = pal_hton32 (DEFAULT_NEXTHOP_ADDR);

  xc = ilm->xc;
  if (xc)
    nhlfe = xc->nhlfe;
  else
    nhlfe = NULL;

  switch (ilm->ent_type)
  {
    case ILM_ENTRY_TYPE_PACKET:
      ikey = &ilm->key.u.pkt;

      pal_mem_set (&nh_dmy_addr, 0, sizeof (struct pal_in4_addr));
#ifdef HAVE_IPV6
      pal_mem_set (&nh_dmy_addr6, 0, sizeof (struct pal_in6_addr));
#endif

      /* Incoming Interface. */
      NSM_MPLS_GET_INDEX_PTR (CHECK_FLAG (ilm->flags,
                                          ILM_ENTRY_FLAG_GMPLS),
                              ilm->key.u.pkt.iif_ix, ifp_in);
      if(ifp_in)
        in_name = ifp_in->name;

      /* Outgoing interface. */
      if (nhlfe)
        {
          key = (struct nhlfe_key *) nhlfe->nkey;
          if (key->afi == AFI_IP)
            {
              NSM_MPLS_GET_INDEX_PTR (CHECK_FLAG (ilm->flags,
                                                  ILM_ENTRY_FLAG_GMPLS),
                                      key->u.ipv4.oif_ix, ifp_out);
              if(ifp_out)
                out_name = ifp_out->name;
            }
#ifdef HAVE_IPV6
          else if (key->afi == AFI_IP6)
            {
              NSM_MPLS_GET_INDEX_PTR (CHECK_FLAG (ilm->flags,
                                                  ILM_ENTRY_FLAG_GMPLS),
                                      key->u.ipv6.oif_ix, ifp_out);
              if(ifp_out)
                out_name = ifp_out->name;
            }
#endif /* HAVE_IPV6 */
#ifdef HAVE_GMPLS
          else if (key->afi == AF_UNNUMBERED)
            {
              NSM_MPLS_GET_INDEX_PTR (CHECK_FLAG (ilm->flags,
                                                  ILM_ENTRY_FLAG_GMPLS),
                                      key->u.unnum.oif_ix, ifp_out);
              if(ifp_out)
                out_name = ifp_out->name;
            }
#endif /* HAVE_GMPLS */
          else
            {
              ifp_out = NULL;
            }
        }
      else
        {
          ifp_out = NULL;
        }

      fec_prefix.family = ilm->family;
      fec_prefix.prefixlen = ilm->prefixlen;

#ifdef HAVE_IPV6
      if (ilm->family == AF_INET6)
        pal_mem_cpy (&fec_prefix.u.prefix6, ilm->prfx,
                     sizeof (struct pal_in6_addr));
      else
#endif
        pal_mem_cpy (&fec_prefix.u.prefix4, ilm->prfx,
                     sizeof (struct pal_in4_addr));

      cli_out (cli, "  %s%-2s %s ",
               (CHECK_FLAG (ilm->flags, ILM_ENTRY_FLAG_STATIC) ? "K" : " "),
               (CHECK_FLAG (ilm->flags, ILM_ENTRY_FLAG_SELECTED) ? ">" : " "),
               (CHECK_FLAG (ilm->flags, ILM_ENTRY_FLAG_STALE) ? "p" : " "));
      cli_out (cli, "%-10d", ikey->in_label);

  if (nhlfe)
    {
      if (key->afi == AFI_IP)
        {
          if (key->u.ipv4.out_label != LABEL_VALUE_INVALID)
            cli_out (cli, "%-11d", key->u.ipv4.out_label);
          else
            cli_out (cli, "%-11s", "N/A");
        }
#ifdef HAVE_IPV6
      else if (key->afi == AFI_IP6)
        {
          if (key->u.ipv6.out_label != LABEL_VALUE_INVALID)
            cli_out (cli, "%-11d", key->u.ipv6.out_label);
          else
            cli_out (cli, "%-11s", "N/A");
        }
         
#endif /* HAVE_IPV6 */
#ifdef HAVE_GMPLS
      else if (key->afi == AFI_UNNUMBERED)
        {
          if (key->u.unnum.out_label != LABEL_VALUE_INVALID)
            cli_out (cli, "%-11d", key->u.unnum.out_label);
          else 
            cli_out (cli, "%-11s", "N/A");
        }
#endif /* HAVE_GMPLS*/
    }
  else
    cli_out (cli, "%-11s", "N/A");

      cli_out (cli, "%-9s", (in_name ? in_name : "N/A"));
      cli_out (cli, "%-10s", (out_name ? out_name : "N/A"));

      if ((nhlfe != NULL) && IPV4_ADDR_CMP (&key->u.ipv4.nh_addr,
                                            &def_addr.s_addr) == 0)
        is_default = PAL_TRUE;

      if (nhlfe != NULL)
        {
          if (key->afi == AFI_IP)
            {
              if (is_default != PAL_TRUE)
                cli_out (cli, "%-16r", &key->u.ipv4.nh_addr);
              else
                cli_out (cli, "%-16s", "-");
            }
#ifdef HAVE_IPV6
          else if (key->afi == AFI_IP6)
            {
              cli_out (cli, "%-16R", &key->u.ipv6.nh_addr);
            }
#endif /* HAVE_IPV6 */
#ifdef HAVE_GMPLS
          else if (key->afi == AFI_UNNUMBERED)
            {
              cli_out (cli, "%-16r", &key->u.ipv4.nh_addr);
            }
#endif /* HAVE_GMPLS */
        }
      else
        cli_out (cli, "%-16r", &nh_dmy_addr);

      cli_out (cli, "%-20O", &fec_prefix);
      break;
#ifdef HAVE_GMPLS
#ifdef HAVE_PBB_TE
    case ILM_ENTRY_TYPE_PBB_TE:
      il_pbb_key = &ilm->key.u.pbb_key;

      /* Incoming Interface. */
      NSM_MPLS_GET_INDEX_PTR (PAL_TRUE, il_pbb_key->iif_ix, ifp_in);
      if(ifp_in)
        in_name = ifp_in->name;

      /* Outgoing interface. */
      if (nhlfe)
        {
          nh_pbb_key = (struct nhlfe_pbb_key *) nhlfe->nkey;

          NSM_MPLS_GET_INDEX_PTR (PAL_TRUE, nh_pbb_key->oif_ix, ifp_out);
          if(ifp_out)
            out_name = ifp_out->name;
        }
      else
        {
          ifp_out = NULL;
        }

      cli_out (cli, "  %s%-2s %s ",
               (CHECK_FLAG (ilm->flags, ILM_ENTRY_FLAG_STATIC) ? "K" : " "),
               (CHECK_FLAG (ilm->flags, ILM_ENTRY_FLAG_SELECTED) ? ">" : " "),
               (CHECK_FLAG (ilm->flags, ILM_ENTRY_FLAG_STALE) ? "p" : " "));

      /* If the mac/vlan is a valid vlan other than 0, then label is valid */
      if((pal_mem_cmp (il_pbb_key->in_label.bmac, zero_mac, ETHER_ADDR_LEN) != 0) &&
         (il_pbb_key->in_label.bvid))
        {
          cli_out (cli, "  [%.04hx.%.04hx.%.04hx,%d]",
               pal_ntoh16(((unsigned short *)il_pbb_key->in_label.bmac)[0]),
               pal_ntoh16(((unsigned short *)il_pbb_key->in_label.bmac)[1]),
               pal_ntoh16(((unsigned short *)il_pbb_key->in_label.bmac)[2]),
               il_pbb_key->in_label.bvid);
        }
      else
        cli_out (cli, "%s", "  - ");

      /* If the mac/vlan is a valid vlan other than 0, then label is valid */
      if(nhlfe && 
         (pal_mem_cmp (nh_pbb_key->lbl.bmac, zero_mac, ETHER_ADDR_LEN) != 0) &&
         (nh_pbb_key->lbl.bvid))
        {
          cli_out (cli, "  [%.04hx.%.04hx.%.04hx,%d]",
               pal_ntoh16(((unsigned short *)nh_pbb_key->lbl.bmac)[0]),
               pal_ntoh16(((unsigned short *)nh_pbb_key->lbl.bmac)[1]),
               pal_ntoh16(((unsigned short *)nh_pbb_key->lbl.bmac)[2]),
               nh_pbb_key->lbl.bvid);
        }
      else
        cli_out (cli, "%s", "  - ");

      cli_out (cli, "%-9s", (in_name ? in_name : "N/A"));
      cli_out (cli, "%-10s", (out_name ? out_name : "N/A"));
      cli_out (cli, "%-15s", "N/A");
      cli_out (cli, " tesid: %-4d ",
              (ilm->tgen_data) ?
              ((struct gmpls_tgen_data *)ilm->tgen_data)->u.pbb.tesid : 0);

      break;
#endif /* HAVE_PBB_TE */
#endif /* HAVE_GMPLS */

    default:
      return;
  }

  cli_out (cli, "%-13d", ilm->ilm_ix);

#if (!defined (HAVE_GMPLS) || defined (HAVE_PACKET))
#ifdef HAVE_DIFFSERV
  nsm_mpls_diffserv_lsp_type_dump (cli, ilm->ds_info.lsp_type);
#endif /* HAVE_DIFFSERV */
#endif /* !HAVE_GMPLS || HAVE_PACKET */
  cli_out (cli, "\n");
}


/* Show MPLS IPv4 ILM table data. */
CLI (mpls_ilm_table,
     mpls_ilm_table_cmd,
     "show mpls ilm-table",
     CLI_SHOW_STR,
     SHOW_MPLS_STR,
     "Show MPLS Incoming Label Map table")
{
  struct nsm_master *nm = cli->vr->proto;
  struct ilm_entry *ilm;
  struct avl_node *an;
  bool_t heading = NSM_FALSE;

  if (! NSM_MPLS || ! ILM_TABLE)
    return CLI_SUCCESS;

  for (an = avl_top (ILM_TABLE); an; an = avl_next (an))
    {
      if (! an->info)
        continue;

      /* Header. */
      if (heading == NSM_FALSE)
        {
          heading = NSM_TRUE;
          cli_out (cli, "Codes: > - selected ILM, p - stale ILM, K - CLI ILM\n\n");
          cli_out (cli, "Code  In-Label  Out-Label  In-Intf  Out-Intf  "
                   "Nexthop         FEC                ILM-ID");
#if (!defined (HAVE_GMPLS) || defined (HAVE_PACKET))
#ifdef HAVE_DIFFSERV
          cli_out (cli, "        LSP-Type");
#endif /* HAVE_DIFFSERV */
#endif /* !HAVE_GMPLS || HAVE_PACKET */
          cli_out (cli, "\n");
        }

      for (ilm = (struct ilm_entry *)an->info; ilm; ilm = ilm->next)
        nsm_mpls_ilm_entry_show (cli, ilm);
    }

#ifdef HAVE_GMPLS
#ifdef HAVE_PBB_TE

  for (an = avl_top (ILM_PBB_TABLE); an; an = avl_next (an))
    {
      if (! an->info)
        continue;

      /* Header. */
      if (heading == NSM_FALSE)
        {
          heading = NSM_TRUE;
          cli_out (cli, "Codes: > - selected ILM, p - stale ILM, K - CLI ILM\n\n");
          cli_out (cli, "Code  In-Label  Out-Label  In-Intf  Out-Intf  "
                   "Nexthop         FEC                ILM-ID");
#if (!defined (HAVE_GMPLS) || defined (HAVE_PACKET))
#ifdef HAVE_DIFFSERV
          cli_out (cli, "        LSP-Type");
#endif /* HAVE_DIFFSERV */
#endif /* !HAVE_GMPLS || HAVE_PACKET */
          cli_out (cli, "\n");
        }

      for (ilm = (struct ilm_entry *)an->info; ilm; ilm = ilm->next)
        nsm_mpls_ilm_entry_show (cli, ilm);
    }

#endif /* HAVE_GMPLS */
#endif /* HAVE_PBB_TE */



  return CLI_SUCCESS;
}

CLI (mpls_insegment,
     mpls_insegment_cmd,
     "show mpls in-segment-table",
     CLI_SHOW_STR,
     SHOW_MPLS_STR,
     "Show MPLS In-segment table")
{
  struct nsm_master *nm = cli->vr->proto;
  struct ilm_entry *ilm;
  struct avl_node *an;

  if (! NSM_MPLS || ! ILM_TABLE)
    return CLI_SUCCESS;

#ifdef HAVE_MPLS_FWD
  pal_mpls_ilm_stats_update (nm);
#endif /* HAVE_MPLS_FWD */

  for (an = avl_top (ILM_TABLE); an; an = avl_next (an))
    {
      if (! an->info)
        continue;

      for (ilm = (struct ilm_entry *)an->info; ilm; ilm = ilm->next)
        {
          nsm_mpls_ilm_entry_dump (cli, ilm);
          cli_out (cli, "\n");
        }
    }

  return CLI_SUCCESS;
}

/* Show XC data. */
CLI (mpls_xc_table,
     mpls_xc_table_cmd,
     "show mpls cross-connect-table",
     CLI_SHOW_STR,
     SHOW_MPLS_STR,
     "Show MPLS Cross-connect table")
{
  struct nsm_master *nm = cli->vr->proto;
  struct xc_entry *xc;
  struct avl_node *an;

  if (! NSM_MPLS || ! XC_TABLE)
    return CLI_SUCCESS;

  for (an = avl_top (XC_TABLE); an; an = avl_next (an))
    {
      if ((xc = an->info) != NULL)
        {
          nsm_mpls_xc_entry_dump (cli, xc, MPLS_NHLFE_OWNER_UNKNOWN);
          cli_out (cli, "\n");
        }
    }

  return CLI_SUCCESS;
}

/* Show OS data. */
CLI (mpls_nhlfe_table,
     mpls_nhlfe_table_cmd,
     "show mpls out-segment-table",
     CLI_SHOW_STR,
     SHOW_MPLS_STR,
     "Show MPLS Out-segment table")
{
  struct nsm_master *nm = cli->vr->proto;
  struct nhlfe_entry *nhlfe;
  struct avl_node *an;
  struct avl_tree *nh = NHLFE_TABLE4;
  bool_t done = NSM_FALSE;

  if (! NSM_MPLS)
    return CLI_SUCCESS;

#ifdef HAVE_MPLS_FWD
  nsm_gmpls_nhlfe_stats_cleanup_all (nm);
  pal_mpls_nhlfe_stats_update (nm);
#endif /* HAVE_MPLS_FWD */

  while (!done)
    {
      for (an = avl_top (nh); an; an = avl_next (an))
        {
          if ((nhlfe = an->info) != NULL)
            {
              nsm_mpls_nhlfe_entry_dump (cli, nhlfe, MPLS_NHLFE_OWNER_NHLFE);
              cli_out (cli, "\n");
            }
        }

#ifdef HAVE_IPV6
      if (nh != NHLFE_TABLE6)
        nh = NHLFE_TABLE6;
      else
#endif
        done = NSM_TRUE;
    }

  return CLI_SUCCESS;
}

#ifdef HAVE_VRF
/* Show VRF data. */
CLI (mpls_vrf_table,
     mpls_vrf_table_cmd,
     "show mpls vrf-table",
     CLI_SHOW_STR,
     SHOW_MPLS_STR,
     "Show MPLS VRF tables")
{
  struct nsm_master *nm = cli->vr->proto;
  struct vrf_table *v_table;
  struct ptree_node *pn;
  struct ftn_entry *vrf;
  struct fec_gen_entry fec;
  int i;
  bool_t heading;

  if (! NSM_MPLS)
    return CLI_SUCCESS;

#ifdef HAVE_MPLS_FWD
  pal_mpls_ftn_stats_update (nm);
#endif /* HAVE_MPLS_FWD */

  for (i = 0; i < VRF_HASH_SIZE; i++)
    {
      for (v_table = NSM_MPLS->vrf_hash[i]; v_table; v_table = v_table->next)
        {
          if (VRF_TABLE4 (v_table))
            {
              heading = NSM_FALSE;
              for (pn = ptree_top (VRF_TABLE4 (v_table)); pn;
                   pn = ptree_next (pn))
                {
                  if ((vrf = pn->info) != NULL)
                    {
                      if (heading == NSM_FALSE)
                        {
                          heading = NSM_TRUE;
                          cli_out (cli,
                                   "Output for IPv4 VRF table with id: %d\n",
                                   v_table->vrf_id);
                        }

                      nsm_gmpls_set_fec_from_ftn (vrf, &fec);
                      nsm_mpls_ftn_entry_dump (cli, vrf, &fec.u.prefix,
                                               NSM_TRUE);
                      cli_out (cli, "\n");
                    }
                }
            }

#ifdef HAVE_IPV6
          if (VRF_TABLE6 (v_table))
            {
              heading = NSM_FALSE;
              for (pn = ptree_top (VRF_TABLE6 (v_table)); pn;
                   pn = ptree_next (pn))
                {
                  if ((vrf = pn->info) != NULL)
                    {
                      if (heading == NSM_FALSE)
                        {
                          heading = NSM_TRUE;
                          cli_out (cli,
                                   "Output for IPv6 VRF table with id: %d\n",
                                   v_table->vrf_id);
                        }

                      nsm_gmpls_set_fec_from_ftn (vrf, &fec);
                      nsm_mpls_ftn_entry_dump (cli, vrf, &fec.u.prefix,
                                               NSM_TRUE);
                      cli_out (cli, "\n");
                    }
                }
            }
#endif
        }
    }

  return CLI_SUCCESS;
}

/* Show VRF data. */
CLI (mpls_vrf_table_name,
     mpls_vrf_table_name_cmd,
     "show mpls vrf-table VRFNAME",
     CLI_SHOW_STR,
     SHOW_MPLS_STR,
     "Show MPLS VRF tables",
     "Show MPLS VRF table by name")
{
  struct nsm_master *nm = cli->vr->proto;
  struct nsm_vrf *n_vrf;
  struct ptree_ix_table *v_table;
  struct fec_gen_entry fec;
  struct ftn_entry *vrf;
  struct ptree_node *pn;
  bool_t done = NSM_FALSE;
  u_int8_t family = AF_INET;

  if (! NSM_MPLS)
    return CLI_SUCCESS;

#ifdef HAVE_MPLS_FWD
  pal_mpls_ftn_stats_update (nm);
#endif /* HAVE_MPLS_FWD */

  /* Lookup vrf by name. */
  n_vrf = nsm_vrf_lookup_by_name (nm, argv[0]);
  if (! n_vrf)
    {
      cli_out (cli, "%% VRF %s not configured\n", argv[0]);
      return CLI_ERROR;
    }

  fec.type = gmpls_entry_type_ip;

  while (!done)
    {
      fec.u.prefix.family = family;

      /* Lookup vrf matching id. */
      v_table = nsm_gmpls_ftn_table_lookup (nm, n_vrf->vrf->id, &fec);
      if (! v_table)
        {
          cli_out (cli, "%% VRF table with id %d not found\n", n_vrf->vrf->id);
          return CLI_ERROR;
        }

      for (pn = ptree_top (v_table->m_table); pn; pn = ptree_next (pn))
        {
          if ((vrf = pn->info) != NULL)
            {
              nsm_gmpls_set_fec_from_ftn (vrf, &fec);
              nsm_mpls_ftn_entry_dump (cli, vrf, &fec.u.prefix, NSM_TRUE);
              cli_out (cli, "\n");
            }
        }
#ifdef HAVE_IPV6
      if (family != AF_INET6)
        family = AF_INET6;
      else
#endif
        done = NSM_TRUE;
    }


  return CLI_SUCCESS;
}
#endif /* HAVE_VRF */

#ifdef HAVE_PACKET
/* Show mapped routes. */
CLI (mpls_mapped_routes,
     mpls_mapped_routes_cmd,
     "show mpls mapped-routes",
     CLI_SHOW_STR,
     SHOW_MPLS_STR,
     "Show mapped MPLS routes")
{
  struct nsm_master *nm = cli->vr->proto;
  struct mapped_route *mroute;
  struct ptree_node *pn;
  struct fec_gen_entry fec;
  bool_t heading;

  if (! NSM_MPLS || ! NSM_MPLS->mapped_routes)
    return CLI_SUCCESS;

  heading = NSM_FALSE;
  for (pn = ptree_top (NSM_MPLS->mapped_routes); pn; pn = ptree_next (pn))
    {
      if ((mroute = pn->info) != NULL)
        {
          if (heading == NSM_FALSE)
            {
              heading = NSM_TRUE;
              cli_out (cli, "Mapped-route        IPv4 FEC\n");
            }

          nsm_gmpls_set_fec_from_mapped_route (pn, &fec);
          cli_out (cli, "%-20O", &fec.u.prefix);
          cli_out (cli, "%O\n", &mroute->fec);
        }
    }

  return CLI_SUCCESS;
}
#endif /* HAVE_PACKET */

#ifdef HAVE_VCCV
void
nsm_mpls_l2_circuit_vccv_dump (struct cli *cli,
                               struct nsm_mpls_circuit *vc)
{
  u_int8_t cc_type_inuse = CC_TYPE_NONE;
  u_int8_t cv_type_inuse = CV_TYPE_NONE;

   /* VCCV Not configured. Simply return */
   if (!vc || !vc->cc_types)
    return;

   cli_out (cli, "Local VCCV Capability:\n");
   /* get cc type in use */

   if (vc->vc_fib)
     cc_type_inuse = oam_util_get_cctype_in_use (vc->cc_types,
                                                 vc->vc_fib->remote_cc_types);

   /* display cc-types */
   cli_out (cli, " CC-Types: ");
   if (CHECK_FLAG (vc->cc_types, CC_TYPE_1_BIT))
     {
       cli_out (cli, " Type 1");
       if (cc_type_inuse == CC_TYPE_1)
         cli_out (cli, "(in use)");
     }
   if (CHECK_FLAG (vc->cc_types, CC_TYPE_2_BIT))
     {
       cli_out (cli, " Type 2");
       if (cc_type_inuse == CC_TYPE_2)
         cli_out (cli, "(in use)");
     }
   if (CHECK_FLAG (vc->cc_types, CC_TYPE_3_BIT))
     {
       cli_out (cli, " Type 3");
       if (cc_type_inuse == CC_TYPE_3)
         cli_out (cli, "(in use)");
     }
   cli_out (cli, "\n");

   /* display CV-Types */
   cli_out (cli, " CV-Types:\n");
   if (CHECK_FLAG (vc->cv_types, CV_TYPE_ICMP_PING_BIT))
     {
       cli_out (cli, " ICMP ping\n");
       if (vc->vc_fib &&
           CHECK_FLAG (vc->vc_fib->remote_cv_types, CV_TYPE_ICMP_PING_BIT))
         cli_out (cli, "(in use)");
       else
         cli_out (cli, "\n");
     }
   if (CHECK_FLAG (vc->cv_types, CV_TYPE_LSP_PING_BIT))
     {
       cli_out (cli, " LSP ping");
       if (vc->vc_fib &&
           CHECK_FLAG (vc->vc_fib->remote_cv_types,CV_TYPE_LSP_PING_BIT))
         cli_out (cli, "(in use)");
       else
         cli_out (cli, "\n");
     }

   /* If BFD is not configured, simply return */
   if (nsm_mpls_is_bfd_set(vc->cv_types) == PAL_FALSE)
     return;

   /* get cvtype in use for bfd vccv */
   if (vc->vc_fib)
     cv_type_inuse = oam_util_get_bfd_cvtype_in_use (vc->cv_types,
                                                   vc->vc_fib->remote_cv_types);

   if (CHECK_FLAG (vc->cv_types, CV_TYPE_BFD_IPUDP_DET_BIT))
     {
       cli_out (cli, " BFD IP/UDP-encapsulated, for PW Fault Detection only");
       if (cv_type_inuse == CV_TYPE_BFD_IPUDP_DET)
         cli_out (cli, "(in use)\n");
       else
          cli_out (cli, "\n");
     }
   if (CHECK_FLAG (vc->cv_types, CV_TYPE_BFD_IPUDP_DET_SIG_BIT))
     {
       cli_out (cli, " BFD IP/UDP-encapsulated, for PW Fault Detection");
       cli_out (cli, " and AC/PW Fault Status Signaling");
       if (cv_type_inuse == CV_TYPE_BFD_IPUDP_DET_SIG)
          cli_out (cli, "(in use)\n");
       else
          cli_out (cli, "\n");
     }
   if (CHECK_FLAG (vc->cv_types, CV_TYPE_BFD_ACH_DET_BIT))
     {
       cli_out (cli, " BFD PW-ACH-encapsulated, for PW Fault Detection only");
       if (cv_type_inuse == CV_TYPE_BFD_ACH_DET)
          cli_out (cli, "(in use)\n");
       else
          cli_out (cli, "\n");
     }

   if (CHECK_FLAG (vc->cv_types, CV_TYPE_BFD_ACH_DET_SIG_BIT))
     {
       cli_out (cli, " BFD PW-ACH-encapsulated, for PW Fault Detection");
       cli_out (cli, " and AC/PW Fault Status Signaling");
       if (cv_type_inuse == CV_TYPE_BFD_ACH_DET_SIG)
          cli_out (cli, "(in use)\n");
       else
          cli_out (cli, "\n");
     }
}
#endif /* HAVE_VCCV */

/* Dump virtual circuit. */
#ifdef HAVE_MPLS_VC
void
nsm_mpls_l2_circuit_dump (struct cli *cli, struct nsm_mpls_circuit *vc)
{
  struct nsm_mpls_vc_group *group;
  char vcstr[10];

  /* Header. */
#ifdef HAVE_SNMP
  cli_out (cli, "MPLS Layer-2 Virtual Circuit: %s, id: %u  PW-INDEX: %d",
           vc->name, vc->id,vc->vc_snmp_index);
#else
  cli_out (cli, "MPLS Layer-2 Virtual Circuit: %s, id: %u",
           vc->name, vc->id);
#endif /*HAVE_SNMP */
  if (vc->tunnel_id > 0)
    cli_out (cli, " tunnel_id %d", vc->tunnel_id);

  cli_out (cli, "\n");

  /* End-point. */
  if (vc->address.family == AF_INET)
    cli_out (cli, " Endpoint: %r\n", &vc->address.u.prefix4);
#ifdef HAVE_IPV6
  else if (vc->address.family == AF_INET6)
    {
      char buf[INET_NTOP_BUFSIZ];
      pal_inet_ntop (AF_INET6, &vc->address.u.prefix6, buf, INET_NTOP_BUFSIZ);
      cli_out (cli, " Endpoint: %s\n", buf);
    }
#endif /* HAVE_IPV6 */
  else
    cli_out (cli, " Endpoint: N/A\n");

  /* Control Word. */
  cli_out (cli, " Control Word: %d\n", vc->cw);

  /* Group. */
  group = vc->group;
  if (group)
    cli_out (cli, " MPLS Layer-2 Virtual Circuit Group: %s\n", group->name);
  else
    cli_out (cli, " MPLS Layer-2 Virtual Circuit Group: none\n");

  if (vc->vc_info && vc->vc_info->mif)
    {
      /* Outgoing interface. */
      cli_out (cli, " Bound to interface: %s\n",
               (vc->vc_info->mif ? vc->vc_info->mif->ifp->name : "none"));

      /* Virtual Circuit type. */
      cli_out (cli, " Virtual Circuit Type: %s\n",
               ((vc->vc_info) ?
                mpls_vc_type_to_str (vc->vc_info->vc_type) : "N/A"));
      /* Set Virtual Circuit Mode */
      if (! CHECK_FLAG (vc->vc_info->conf_flag, NSM_MPLS_VC_CONF_SECONDARY))
        pal_strcpy (vcstr, "Primary");
      else
        pal_strcpy (vcstr, "Secondary");

      cli_out (cli, " Virtual Circuit is configured as %s\n",
               vcstr);

      if (! CHECK_FLAG (vc->vc_info->conf_flag, NSM_MPLS_VC_CONF_SECONDARY) &&
          (! vc->vc_info->sibling_info ||
           ! CHECK_FLAG (vc->vc_info->sibling_info->conf_flag,
                         NSM_MPLS_VC_CONF_SECONDARY)))
        {
          if (! CHECK_FLAG (vc->vc_info->conf_flag, NSM_MPLS_VC_CONF_STANDBY))
            cli_out (cli, " Virtual Circuit is configured as Active\n");
          else
            cli_out (cli, " Virtual Circuit is configured as Standby\n");
        }
      else
       {
          if (! CHECK_FLAG (vc->vc_info->conf_flag, NSM_MPLS_VC_CONF_REVERTIVE))
            cli_out (cli, " Virtual Circuit is configured as Non-Revertive\n");
          else
            cli_out (cli, " Virtual Circuit is configured as Revertive\n");
       }

      if (vc->vc_fib && vc->vc_fib->install_flag == NSM_TRUE)
        cli_out (cli, " Virtual Circuit is active\n");
      else
        cli_out (cli, " Virtual Circuit is standby\n");

#ifdef HAVE_VCCV
      nsm_mpls_l2_circuit_vccv_dump (cli, vc);
#endif /* HAVE_VCCV */

    }
#ifdef HAVE_VPLS
  else if (vc->vpls)
    {
      struct nsm_vpls_spoke_vc *svc;

      /* VPLS binding. */
      cli_out (cli, " Bound to VPLS instance: %s\n",
               (vc->vpls ? vc->vpls->vpls_name : "none"));

      svc = nsm_vpls_spoke_vc_lookup_by_name (vc->vpls, vc->name, NSM_FALSE);
      if (svc)
        {
          /* Circuit type is always ETHERNET VPLS. */
          cli_out (cli, " Virtual Circuit Type: %s\n",
                   mpls_vc_type_to_str (svc->vc_type));
        }
    }
#endif /* HAVE_VPLS */

  cli_out (cli, "\n");
}

void
nsm_mpls_l2_circuit_group_dump (struct cli *cli,
                                struct nsm_mpls_vc_group *group)
{
  struct listnode *ln;
  struct nsm_mpls_circuit *vc;
  bool_t found = NSM_FALSE;
  int i = 0;
#ifdef HAVE_VPLS
  struct nsm_vpls *vpls = NULL;
#endif /* HAVE_VPLS */

  /* Header. */
  cli_out (cli, "MPLS Layer-2 Virtual Circuit Group: %s, id: %u\n",
           group->name, group->id);

  cli_out (cli, " Virtual Circuits configured:");
  NSM_ASSERT (group->vc_list != NULL);
  LIST_LOOP (group->vc_list, vc, ln)
    {
      /* Set found flag. */
      if (found == NSM_FALSE)
        found = NSM_TRUE;

      /* Get virtual circuit and print data. */
      cli_out (cli, "\n  %d. %s", ++i, vc->name);
    }

#ifdef HAVE_VPLS
  cli_out (cli, " \n VPLS configured:");
  NSM_ASSERT (group->vpls_list != NULL);
  LIST_LOOP (group->vpls_list, vpls, ln)
    {
      /* Set found flag. */
      if (found == NSM_FALSE)
        found = NSM_TRUE;

      /* Get virtual circuit and print data. */
      cli_out (cli, "\n  %d. %s", ++i, vpls->vpls_name);
    }
#endif /* HAVE_VPLS */

  if (found != NSM_TRUE)
    cli_out (cli, "no VC/VPLS in this group!");

  /* End. */
  cli_out (cli, "\n");
}

/* Show Virtual Circuit group data. */
CLI (mpls_l2_vc_group_show,
     mpls_l2_vc_group_show_cmd,
     "show mpls l2-circuit-group",
     CLI_SHOW_STR,
     SHOW_MPLS_STR,
     "Show MPLS Layer-2 Virtual Circuit group data")
{
  struct nsm_master *nm = cli->vr->proto;
  struct nsm_mpls_vc_group *group;
  struct listnode *ln;

  if ((! NSM_MPLS) || (! NSM_MPLS->vc_table))
    return CLI_SUCCESS;

  LIST_LOOP (NSM_MPLS->vc_group_list, group, ln)
    nsm_mpls_l2_circuit_group_dump (cli, group);

  return CLI_SUCCESS;
}

/* Show specific Virtual Circuit group. */
CLI (mpls_l2_vc_group_show_by_name,
     mpls_l2_vc_group_show_by_name_cmd,
     "show mpls l2-circuit-group NAME",
     CLI_SHOW_STR,
     SHOW_MPLS_STR,
     "Show MPLS Layer-2 Virtual Circuit group data",
     "Name of virtual circuit group to be shown")
{
  struct nsm_master *nm = cli->vr->proto;
  struct nsm_mpls_vc_group *group;

  if ((! NSM_MPLS) || (! NSM_MPLS->vc_table))
    return CLI_SUCCESS;

  /* Get virtual circuit group. */
  group = nsm_mpls_l2_vc_group_lookup_by_name (cli->vr->proto, argv[0]);
  if (! group)
    {
      cli_out (cli, "%% MPLS Layer-2 Virtual Circuit group with name %s "
               "not found\n", argv[0]);
      return CLI_ERROR;
    }

  /* Dump virtual circuit group. */
  nsm_mpls_l2_circuit_group_dump (cli, group);

  return CLI_SUCCESS;
}

/* Show Virtual Circuit data. */
CLI (mpls_l2_circuit_show,
     mpls_l2_circuit_show_cmd,
     "show mpls l2-circuit",
     CLI_SHOW_STR,
     SHOW_MPLS_STR,
     "Show MPLS Layer-2 Virtual Circuit data")
{
  struct nsm_master *nm = cli->vr->proto;
  struct nsm_mpls_circuit *vc;
  struct route_node *rn;

  if ((! NSM_MPLS) || (! NSM_MPLS->vc_table))
    return CLI_SUCCESS;

  for (rn = route_top (NSM_MPLS->vc_table); rn; rn = route_next (rn))
    {
      /* If no info, skip. */
      vc = rn->info;
      if (! vc)
        continue;

      /* Dump. */
      nsm_mpls_l2_circuit_dump (cli, vc);
    }

  return CLI_SUCCESS;
}

/* Show specific Virtual Circuit. */
CLI (mpls_l2_circuit_show_by_name,
     mpls_l2_circuit_show_by_name_cmd,
     "show mpls l2-circuit NAME",
     CLI_SHOW_STR,
     SHOW_MPLS_STR,
     "Show MPLS Layer-2 Virtual Circuit data",
     "Name of virtual circuit to be shown")
{
  struct nsm_master *nm = cli->vr->proto;
  struct nsm_mpls_circuit *vc;

  if ((! NSM_MPLS) || (! NSM_MPLS->vc_table))
    return CLI_SUCCESS;

  /* Get virtual circuit. */
  vc = nsm_mpls_l2_circuit_lookup_by_name (nm, argv[0]);
  if (! vc)
    {
      cli_out (cli, "%% MPLS Layer-2 Virtual Circuit with name %s "
               "not found\n", argv[0]);
      return CLI_ERROR;
    }

  /* Dump virtual circuit. */
  nsm_mpls_l2_circuit_dump (cli, vc);

  return CLI_SUCCESS;
}

/* Create l2 circuit. */
int
mpls_l2_circuit_create_cli (struct cli *cli, char *vc_name, char *val_str,
                            char *address, char *group_name, u_int32_t group_id,
                            u_char c_word, char mapping_type, 
                            char *tunnel_info, u_char tunnel_dir,
#ifdef HAVE_VCCV
                            u_int8_t cc_types, u_int8_t cv_types,
#endif /* HAVE_VCCV */
                            u_int8_t fec_type_vc, u_int8_t is_passive)
{
  struct nsm_master *nm = cli->vr->proto;
  u_int32_t val;
  struct pal_in4_addr addr;
  int ret;

  /* Get value. */
  CLI_GET_UINT32_RANGE ("Identifier", val, val_str, NSM_MPLS_L2_VC_MIN,
                         NSM_MPLS_L2_VC_MAX);

  /* Get end-point. */
  CLI_GET_IPV4_ADDRESS ("to", addr, address);

  /* Create l2 circuit. */
  ret = nsm_mpls_l2_circuit_add (nm, vc_name, val, &addr, group_name, group_id,
                                 c_word, mapping_type, tunnel_info, tunnel_dir, 
#ifdef HAVE_VCCV
                                 cc_types, cv_types,
#endif /* HAVE_VCCV */
                                 fec_type_vc, is_passive);

  if (ret < 0)
    {
      switch (ret)
        {
        case NSM_ERR_INVALID_VC_ID:
          cli_out (cli, "%% Invalid VC-ID specified. Same ID cannot be "
                   "used by multiple VC/VPLS instances.\n");
          break;
        case NSM_ERR_MEM_ALLOC_FAILURE:
          cli_out (cli, "%% Operation failed. No memory available.\n");
          break;
        case NSM_ERR_OWNER_MISMATCH:
          cli_out (cli, "%% Operation failed. Owner cannot be modified when "
                   "VC is active\n");
          break;
        case NSM_ERR_GROUP_ID_EXISTS:
          cli_out (cli, "%% ac-group-id already exists\n");
          break;
        case NSM_ERR_GROUP_EXISTS:
          cli_out (cli, "%% ac-group-name already exists\n");
          break;
        case NSM_ERR_MS_PW_ROLE_MISMATCH:
          cli_out (cli, "%% VC role cannot be changed\n");
          break;
        default:
          cli_out (cli, "%% Virtual Circuit creation failure.\n");
          break;
        }
      return CLI_ERROR;
    }

  return CLI_SUCCESS;
}

/* Since RSVP only send tunnel_id to NSM and static configured FTN only
   provide tunnel_id, so VC only map to tunnel_id */
#if 0
CLI (mpls_l2_circuit_with_grp,
     mpls_l2_circuit_with_grp_cmd,
     "mpls l2-circuit NAME <1-4294967295> A.B.C.D GROUPNAME (control-word|)"
     " (tunnel-name NAME |tunnel-id <100-1500>|) (manual|)",
     CONFIG_MPLS_STR,
     "Specify an MPLS Layer-2 Virtual Circuit",
     "Identifying string for MPLS Layer-2 Virtual Circuit",
     "Identifying value for MPLS Layer-2 Virtual Circuit",
     "IPv4 Address for end-point for MPLS Layer-2 Virtual Circuit",
     "Group name identifier",
     "Control-word",
     "Name of MPLS LSP (or Layer 2 Tunnel) to be used for VC",
     "Name to be configured",
     "Identifier of the MPLS LSP (or Layer 2 Tunnel) to be used",
     "Tunnel-Identifier - Is obtained only after the tunnel is configured",
     "No signaling is used to set-up the Virtual Circuit")
{
  u_char c_word = 0;
  u_int32_t i, fec_type_vc;
  u_char *tunnel_info = NULL;
  u_char mapping_type = MPLS_VC_MAPPING_NONE;

#ifdef HAVE_FEC129
  fec_type_vc = PW_OWNER_GEN_FEC_SIGNALING;
#else
  fec_type_vc = PW_OWNER_PWID_FEC_SIGNALING;
#endif /* HAVE_FEC129 */

  for (i = 4; i < argc; i++)
    {
      if (pal_strncmp (argv[i], "c", 1) == 0)
        c_word = 1;
      else if (pal_strncmp (argv[i], "tunnel-name", 11) == 0 ||
               pal_strncmp (argv[i], "tunnel-id", 9) == 0)
        {
	  if (argv[i][7] == 'n')
	    mapping_type = MPLS_VC_MAPPING_TUNNEL_NAME;
	  else
	    mapping_type = MPLS_VC_MAPPING_TUNNEL_ID;

          tunnel_info = argv[i+1];
        }
      else if (pal_strncmp (argv[i], "m", 1) == 0)
        fec_type_vc = PW_OWNER_MANUAL;
    }

  return mpls_l2_circuit_create_cli (cli, argv[0], argv[1], argv[2], argv[3],
                                     c_word, mapping_type, tunnel_info, 
                                     fec_type_vc);
}

CLI (mpls_l2_circuit,
     mpls_l2_circuit_cmd,
     "mpls l2-circuit NAME <1-4294967295> A.B.C.D (control-word|)"
     " (tunnel-name NAME | tunnel-id <100-1500>|) (manual|)",
     CONFIG_MPLS_STR,
     "Specify an MPLS Layer-2 Virtual Circuit",
     "Identifying string for MPLS Layer-2 Virtual Circuit",
     "Identifying value for MPLS Layer-2 Virtual Circuit",
     "IPv4 Address for end-point for MPLS Layer-2 Virtual Circuit",
     "Control-word",
     "Name of MPLS LSP (or Layer 2 Tunnel) to be used for VC",
     "Name to be configured",
     "Identifier of the MPLS LSP (or Layer 2 Tunnel) to be used",
     "Tunnel-Identifier - Is obtained only after the tunnel is configured",
     "No signaling is used to set-up the Virtual Circuit")
{
  u_char c_word = 0;
  s_int32_t i, fec_type_vc;
  u_char *tunnel_info = NULL;
  u_char mapping_type = MPLS_VC_MAPPING_NONE;

#ifdef HAVE_FEC129
  fec_type_vc = PW_OWNER_GEN_FEC_SIGNALING;
#else
  fec_type_vc = PW_OWNER_PWID_FEC_SIGNALING;
#endif /* HAVE_FEC129 */

  for (i = 3; i < argc; i++)
    {
      if (pal_strncmp (argv[i], "c", 1) == 0)
        c_word = 1;
      else if (pal_strncmp (argv[i], "tunnel-name", 11) == 0 ||
               pal_strncmp (argv[i], "tunnel-id", 9) == 0)
        {
	  if (argv[i][7] == 'n')
	    mapping_type = MPLS_VC_MAPPING_TUNNEL_NAME;
	  else
	    mapping_type = MPLS_VC_MAPPING_TUNNEL_ID;
          tunnel_info = argv[i+1];
        }
      else if (pal_strncmp (argv[i], "m", 1) == 0)
        fec_type_vc = PW_OWNER_MANUAL;
    }

  return mpls_l2_circuit_create_cli (cli, argv[0], argv[1], argv[2], NULL,
                                     c_word, mapping_type, tunnel_info, 
                                     fec_type_vc);
}
#endif

CLI (mpls_ac_group,
     mpls_ac_group_cmd,
     "mpls ac-group NAME <1-4294967295>",
     CONFIG_MPLS_STR,
     "Create a new access circuit group",
     "Name of access circuit group",
     "Identifier for the group (group-id to be used in LDP)" )
{
  struct nsm_master *nm = cli->vr->proto;
  u_int32_t group_id;
  char* name = argv[0];
  int ret = CLI_ERROR;

  CLI_GET_UINT32_RANGE ("Group-ID", group_id, argv[1], 1, 4294967295UL);

  if ( !nsm_mpls_l2_vc_group_lookup_by_name (nm, name))
    {
      if ( !nsm_mpls_l2_vc_group_lookup_by_id (nm, group_id))
        {
          if (nsm_mpls_l2_vc_group_create (nm, name, group_id))
            ret = CLI_SUCCESS;
          else
            cli_out (cli, "%%ac-group creation failed\n");
        }
      else 
        cli_out (cli, "%%ac-group-id already existing\n");
    }
  else 
    cli_out ( cli, "%%ac-group-name already existing\n");

  return ret;
}

CLI (mpls_no_ac_group,
     mpls_no_ac_group_cmd,
     "no mpls ac-group NAME",
     CLI_NO_STR,
     CONFIG_MPLS_STR,
     "Remove an access circuit group",
     "Name of access circuit group")
{
  struct nsm_master *nm = cli->vr->proto;
  char* name = argv[0];
  int ret = CLI_ERROR;
  struct nsm_mpls_vc_group *group;

  group = nsm_mpls_l2_vc_group_lookup_by_name (nm, name);

  if (group)
    {
      if ((listcount(group->vc_list) == 0)
#ifdef HAVE_VPLS
          && (listcount(group->vpls_list) == 0)
#endif /* HAVE_VPLS */
         )
        {
          ret = nsm_mpls_l2_vc_group_delete (nm, group);
          if (ret == CLI_SUCCESS)
            return ret;
        }
      else 
        cli_out (cli, "%%can't remove: still pending VCs/VPLS!\n");
    }
  else 
    cli_out (cli, "%%ac-group does not exist\n");

  return ret;
}

CLI (mpls_l2_circuit_with_grp,
     mpls_l2_circuit_with_grp_cmd,
     "mpls l2-circuit NAME <1-4294967295> A.B.C.D GROUPNAME "
     "(group-id <1-4294967295>|) "
     "(control-word|) ((tunnel-id <1-65535> (forward|reverse|))|) (manual|)",
     CONFIG_MPLS_STR,
     "Specify an MPLS Layer-2 Virtual Circuit",
     "Identifying string for MPLS Layer-2 Virtual Circuit",
     "Identifying value for MPLS Layer-2 Virtual Circuit",
     "IPv4 Address for end-point for MPLS Layer-2 Virtual Circuit",
     "Group name identifier",
     "Group ID",
     "Identifying value for group id",
     "Control-word",
     "Identifier of the MPLS LSP (or Layer 2 Tunnel) to be used",
     "Tunnel-Identifier - Is obtained only after the tunnel is configured",
     "Tunnel Direction - Forward tunnel identifier - Default Value",
     "Tunnel Direction - Reverse tunnel identifier",
     "No signaling is used to set-up the Virtual Circuit")
{
  u_char c_word = 0;
  u_int32_t group_id = 0;
  u_int32_t i, fec_type_vc;
  u_char *tunnel_info = NULL;
  u_char mapping_type = MPLS_VC_MAPPING_NONE;
  u_int8_t is_passive = 0;
  u_char tunnel_dir = TUNNEL_DIR_FWD;
#ifdef HAVE_VCCV
  u_int8_t cc_types = 0;
  u_int8_t cv_types = 0;
  int ret = 0;
#endif /* HAVE_VCCV */

#ifdef HAVE_FEC129
  fec_type_vc = PW_OWNER_GEN_FEC_SIGNALING;
#else
  fec_type_vc = PW_OWNER_PWID_FEC_SIGNALING;
#endif /* HAVE_FEC129 */

  for (i = 4; i < argc; i++)
    {
      if (pal_strncmp (argv[i], "group-id", 8) == 0)
        CLI_GET_UINT32_RANGE ("Group-ID", group_id, argv[i+1], 1, 4294967295UL);
      else if (pal_strncmp (argv[i], "c", 1) == 0)
        c_word = 1;
      else if (pal_strncmp (argv[i], "tunnel-name", 11) == 0 ||
               pal_strncmp (argv[i], "tunnel-id", 9) == 0)
        {
	  if (argv[i][7] == 'n')
	    mapping_type = MPLS_VC_MAPPING_TUNNEL_NAME;
	  else
	    mapping_type = MPLS_VC_MAPPING_TUNNEL_ID;

          i++; /* Go to the next argument and get the tunnel_name/tunnel_id */
          tunnel_info = argv[i];

          /* If there are more arguments */
          if ((i+1) < argc)
            {
              if (pal_strncmp (argv[i+1], "r", 1) == 0)
                {
                  tunnel_dir = TUNNEL_DIR_REV;
                  i++; /* skip this argument */
                }
              else if (pal_strncmp (argv[i+1], "f", 1) == 0)
                {
                  /* tunnel_dir = TUNNEL_DIR_FWD; */
                  i++; /* skip this argument */
                }
            }
        }
      else if (pal_strncmp (argv[i], "m", 1) == 0)
        fec_type_vc = PW_OWNER_MANUAL;
#ifdef HAVE_MS_PW
      else if (pal_strncmp (argv[i], "p", 1) == 0)
        is_passive = 1;
#endif /* HAVE_MS_PW */

#ifdef HAVE_VCCV
      else if (pal_strncmp (argv[i], "vccv", 4) == 0)
        {
          ret = nsm_mpls_parse_vccv_args (cli, argv, argc, fec_type_vc, c_word,
                                          &i, &cc_types, &cv_types);
        }
#endif /* HAVE_VCCV */
    }

  return mpls_l2_circuit_create_cli (cli, argv[0], argv[1], argv[2], argv[3],
                                     group_id, c_word, mapping_type, 
                                     tunnel_info, tunnel_dir,
#ifdef HAVE_VCCV
                                     cc_types, cv_types,
#endif /* HAVE_VCCV */
                                     fec_type_vc, is_passive);
}

#if defined HAVE_MS_PW && defined HAVE_VCCV
ALI (mpls_l2_circuit_with_grp,
     mpls_l2_circuit_with_grp_mspw_vccv_cmd,
     "mpls l2-circuit NAME <1-4294967295> A.B.C.D GROUPNAME (control-word|)"
     " ((tunnel-id <1-65535> (forward|reverse|))|) (manual|passive|)"
     " (vccv (cc-type (1|2|3)|) (bfd (bfd-cv-type (1|2|3|4)|)|)|)",
     CONFIG_MPLS_STR,
     "Specify an MPLS Layer-2 Virtual Circuit",
     "Identifying string for MPLS Layer-2 Virtual Circuit",
     "Identifying value for MPLS Layer-2 Virtual Circuit",
     "IPv4 Address for end-point for MPLS Layer-2 Virtual Circuit",
     "Group name identifier",
     "Control-word",
     "Identifier of the MPLS LSP (or Layer 2 Tunnel) to be used",
     "Tunnel-Identifier - Is obtained only after the tunnel is configured",
     "Tunnel Direction - Forward tunnel identifier - Default Value",
     "Tunnel Direction - Reverse tunnel identifier",
     "No signaling is used to set-up the Virtual Circuit",
     "T-PE is passive",
     "VCCV Required",
     "Specific CC type to be signaled/used",
     "1. CC Type 1 - PWE3 Control Word with 0001b as first nibble",
     "2. CC Type 2 - MPLS Router Alert Label ",
     "3. CC Type 3 -  MPLS PW Label with TTL == 1",
     "BFD VCCV Required",
     "Specific BFD CV type to be signaled/used",
     "1. BFD IP/UDP-encapsulated, for PW Fault Detection only",
     "2. BFD IP/UDP-encapsulated, for PW Fault Detection and AC/PW Fault Status Signaling",
     "3. BFD PW-ACH-encapsulated, for PW Fault Detection only",
     "4. BFD PW-ACH-encapsulated, for PW Fault Detection and AC/PW Fault Status Signaling"
     );
#elif defined HAVE_MS_PW
ALI (mpls_l2_circuit_with_grp,
     mpls_l2_circuit_with_grp_mspw_cmd,
     "mpls l2-circuit NAME <1-4294967295> A.B.C.D GROUPNAME (control-word|)"
     " ((tunnel-id <1-65535> (forward|reverse|))|) (manual|passive|)",
     CONFIG_MPLS_STR,
     "Specify an MPLS Layer-2 Virtual Circuit",
     "Identifying string for MPLS Layer-2 Virtual Circuit",
     "Identifying value for MPLS Layer-2 Virtual Circuit",
     "IPv4 Address for end-point for MPLS Layer-2 Virtual Circuit",
     "Group name identifier",
     "Control-word",
     "Identifier of the MPLS LSP (or Layer 2 Tunnel) to be used",
     "Tunnel-Identifier - Is obtained only after the tunnel is configured",
     "Tunnel Direction - Forward tunnel identifier - Default Value",
     "Tunnel Direction - Reverse tunnel identifier",
     "No signaling is used to set-up the Virtual Circuit",
     "T-PE is passive"
     );
#elif defined HAVE_VCCV
ALI (mpls_l2_circuit_with_grp,
     mpls_l2_circuit_with_grp_vccv_cmd,
     "mpls l2-circuit NAME <1-4294967295> A.B.C.D GROUPNAME (control-word|)"
     " ((tunnel-id <1-65535> (forward|reverse|))|) (manual|)"
     " (vccv (cc-type (1|2|3)|) (bfd (bfd-cv-type (1|2|3|4)|)|)|)",
     CONFIG_MPLS_STR,
     "Specify an MPLS Layer-2 Virtual Circuit",
     "Identifying string for MPLS Layer-2 Virtual Circuit",
     "Identifying value for MPLS Layer-2 Virtual Circuit",
     "IPv4 Address for end-point for MPLS Layer-2 Virtual Circuit",
     "Group name identifier",
     "Control-word",
     "Identifier of the MPLS LSP (or Layer 2 Tunnel) to be used",
     "Tunnel-Identifier - Is obtained only after the tunnel is configured",
     "Tunnel Direction - Forward tunnel identifier - Default Value",
     "Tunnel Direction - Reverse tunnel identifier",
     "No signaling is used to set-up the Virtual Circuit"
     ,"VCCV Required",
     "Specific CC type to be signaled/used",
     "1. CC Type 1 - PWE3 Control Word with 0001b as first nibble",
     "2. CC Type 2 - MPLS Router Alert Label ",
     "3. CC Type 3 -  MPLS PW Label with TTL == 1",
     "BFD VCCV Required",
     "Specific BFD CV type to be signaled/used",
     "1. BFD IP/UDP-encapsulated, for PW Fault Detection only",
     "2. BFD IP/UDP-encapsulated, for PW Fault Detection and AC/PW Fault Status Signaling",
     "3. BFD PW-ACH-encapsulated, for PW Fault Detection only",
     "4. BFD PW-ACH-encapsulated, for PW Fault Detection and AC/PW Fault Status Signaling"
     );
#endif /* defined HAVE_MS_PW && HAVE_VCCV */

CLI (mpls_l2_circuit,
     mpls_l2_circuit_cmd,
     "mpls l2-circuit NAME <1-4294967295> A.B.C.D (control-word|)"
     " ((tunnel-id <1-65535> (forward|reverse|))|) (manual|)",
     CONFIG_MPLS_STR,
     "Specify an MPLS Layer-2 Virtual Circuit",
     "Identifying string for MPLS Layer-2 Virtual Circuit",
     "Identifying value for MPLS Layer-2 Virtual Circuit",
     "IPv4 Address for end-point for MPLS Layer-2 Virtual Circuit",
     "Control-word",
     "Identifier of the MPLS LSP (or Layer 2 Tunnel) to be used",
     "Tunnel-Identifier - Is obtained only after the tunnel is configured",
     "Tunnel Direction - Forward tunnel identifier - Default Value",
     "Tunnel Direction - Reverse tunnel identifier",
     "No signaling is used to set-up the Virtual Circuit")
{
  u_char c_word = 0;
  s_int32_t i, fec_type_vc;
  u_char *tunnel_info = NULL;
  u_char mapping_type = MPLS_VC_MAPPING_NONE;
  u_int8_t is_passive = 0;
  u_char tunnel_dir = TUNNEL_DIR_FWD;
#ifdef HAVE_VCCV
  u_int8_t cc_types = 0;
  u_int8_t cv_types = 0;
  int ret = 0;
#endif /* HAVE_VCCV */

#ifdef HAVE_FEC129
  fec_type_vc = PW_OWNER_GEN_FEC_SIGNALING;
#else
  fec_type_vc = PW_OWNER_PWID_FEC_SIGNALING;
#endif /* HAVE_FEC129 */

  for (i = 3; i < argc; i++)
    {
      if (pal_strncmp (argv[i], "c", 1) == 0)
        c_word = 1;
      else if (pal_strncmp (argv[i], "tunnel-name", 11) == 0 ||
               pal_strncmp (argv[i], "tunnel-id", 9) == 0)
        {
	  if (argv[i][7] == 'n')
	    mapping_type = MPLS_VC_MAPPING_TUNNEL_NAME;
	  else
	    mapping_type = MPLS_VC_MAPPING_TUNNEL_ID;

          i++; /* Go to the next argument and get the tunnel_name/tunnel_id */
          tunnel_info = argv[i];

          /* If there are more arguments */
          if ((i+1) < argc)
            {
              if (pal_strncmp (argv[i+1], "r", 1) == 0)
                {
                  tunnel_dir = TUNNEL_DIR_REV;
                  i++; /* skip this argument */
                }
              else if (pal_strncmp (argv[i+1], "f", 1) == 0)
                {
                  /* tunnel_dir = TUNNEL_DIR_FWD; */
                  i++; /* skip this argument */
                }
            }
        }
      else if (pal_strncmp (argv[i], "m", 1) == 0)
        fec_type_vc = PW_OWNER_MANUAL;
#ifdef HAVE_MS_PW
      else if (pal_strncmp (argv[i], "p", 1) == 0)
        is_passive = 1;
#endif /* HAVE_MS_PW */
#ifdef HAVE_VCCV
      else if (pal_strncmp (argv[i], "vccv", 4) == 0)
        {
          ret = nsm_mpls_parse_vccv_args (cli, argv, argc, fec_type_vc, c_word,
                                          &i, &cc_types, &cv_types);
          if (ret != CLI_SUCCESS)
            return ret;
        }
#endif /* HAVE_VCCV */
    }

  return mpls_l2_circuit_create_cli (cli, argv[0], argv[1], argv[2], NULL, 0,
                                     c_word, mapping_type, 
                                     tunnel_info, tunnel_dir,
#ifdef HAVE_VCCV
                                     cc_types, cv_types,
#endif /* HAVE_VCCV */
                                     fec_type_vc, is_passive);
}

#if defined HAVE_MS_PW && defined HAVE_VCCV
ALI (mpls_l2_circuit,
     mpls_l2_circuit_mspw_vccv_cmd,
     "mpls l2-circuit NAME <1-4294967295> A.B.C.D (control-word|)"
     " ((tunnel-id <1-65535> (forward|reverse|))|) (manual|passive|)"
     " (vccv (cc-type (1|2|3)|) (bfd (bfd-cv-type (1|2|3|4)|)|)|)",
     CONFIG_MPLS_STR,
     "Specify an MPLS Layer-2 Virtual Circuit",
     "Identifying string for MPLS Layer-2 Virtual Circuit",
     "Identifying value for MPLS Layer-2 Virtual Circuit",
     "IPv4 Address for end-point for MPLS Layer-2 Virtual Circuit",
     "Control-word",
     "Identifier of the MPLS LSP (or Layer 2 Tunnel) to be used",
     "Tunnel-Identifier - Is obtained only after the tunnel is configured",
     "Tunnel Direction - Forward tunnel identifier - Default Value",
     "Tunnel Direction - Reverse tunnel identifier",
     "No signaling is used to set-up the Virtual Circuit",
     "T-PE is passive",
     "VCCV",
     "Specific CC type to be signaled/used",
     "1. CC Type 1 - PWE3 Control Word with 0001b as first nibble",
     "2. CC Type 2 - MPLS Router Alert Label ",
     "3. CC Type 3 -  MPLS PW Label with TTL == 1",
     "BFD VCCV Required",
     "Specific BFD CV type to be signaled/used",
     "1. BFD IP/UDP-encapsulated, for PW Fault Detection only",
     "2. BFD IP/UDP-encapsulated, for PW Fault Detection and AC/PW Fault Status Signaling",
     "3. BFD PW-ACH-encapsulated, for PW Fault Detection only",
     "4. BFD PW-ACH-encapsulated, for PW Fault Detection and AC/PW Fault Status Signaling"
     );
#elif defined HAVE_MS_PW
ALI (mpls_l2_circuit,
     mpls_l2_circuit_mspw_cmd,
     "mpls l2-circuit NAME <1-4294967295> A.B.C.D (control-word|)"
     " ((tunnel-id <1-65535> (forward|reverse|))|) (manual|passive|)",
     CONFIG_MPLS_STR,
     "Specify an MPLS Layer-2 Virtual Circuit",
     "Identifying string for MPLS Layer-2 Virtual Circuit",
     "Identifying value for MPLS Layer-2 Virtual Circuit",
     "IPv4 Address for end-point for MPLS Layer-2 Virtual Circuit",
     "Control-word",
     "Identifier of the MPLS LSP (or Layer 2 Tunnel) to be used",
     "Tunnel-Identifier - Is obtained only after the tunnel is configured",
     "Tunnel Direction - Forward tunnel identifier - Default Value",
     "Tunnel Direction - Reverse tunnel identifier",
     "No signaling is used to set-up the Virtual Circuit",
     "T-PE is passive"
     );
#elif defined HAVE_VCCV
ALI (mpls_l2_circuit,
     mpls_l2_circuit_vccv_cmd,
     "mpls l2-circuit NAME <1-4294967295> A.B.C.D (control-word|)"
     " ((tunnel-id <1-65535> (forward|reverse|))|) (manual|)"
     " (vccv (cc-type (1|2|3)|) (bfd (bfd-cv-type (1|2|3|4)|)|)|)",
     CONFIG_MPLS_STR,
     "Specify an MPLS Layer-2 Virtual Circuit",
     "Identifying string for MPLS Layer-2 Virtual Circuit",
     "Identifying value for MPLS Layer-2 Virtual Circuit",
     "IPv4 Address for end-point for MPLS Layer-2 Virtual Circuit",
     "Control-word",
     "Identifier of the MPLS LSP (or Layer 2 Tunnel) to be used",
     "Tunnel-Identifier - Is obtained only after the tunnel is configured",
     "Tunnel Direction - Forward tunnel identifier - Default Value",
     "Tunnel Direction - Reverse tunnel identifier",
     "No signaling is used to set-up the Virtual Circuit",
     "VCCV",
     "Specific CC type to be signaled/used",
     "1. CC Type 1 - PWE3 Control Word with 0001b as first nibble",
     "2. CC Type 2 - MPLS Router Alert Label ",
     "3. CC Type 3 -  MPLS PW Label with TTL == 1",
     "BFD VCCV Required",
     "Specific BFD CV type to be signaled/used",
     "1. BFD IP/UDP-encapsulated, for PW Fault Detection only",
     "2. BFD IP/UDP-encapsulated, for PW Fault Detection and AC/PW Fault Status Signaling",
     "3. BFD PW-ACH-encapsulated, for PW Fault Detection only",
     "4. BFD PW-ACH-encapsulated, for PW Fault Detection and AC/PW Fault Status Signaling"
     );
#endif /* defined HAVE_MS_PW && defined HAVE_VCCV */

CLI (no_mpls_l2_circuit,
     no_mpls_l2_circuit_basic_cmd,
     "no mpls l2-circuit NAME <1-4294967295>",
     CLI_NO_STR,
     NO_CONFIG_MPLS_STR,
     "Specify the MPLS Layer-2 Virtual Circuit to be removed",
     "Identifying string used for the MPLS Layer-2 Virtual Circuit",
     "Identifying value used for the MPLS Layer-2 Virtual Circuit")
{
  struct nsm_master *nm = cli->vr->proto;
  u_int32_t val;
  int ret;

  /* Get value. */
  CLI_GET_UINT32_RANGE ("Identifier", val, argv[1], NSM_MPLS_L2_VC_MIN,
                         NSM_MPLS_L2_VC_MAX);

  /* Delete l2 circuit. */
  ret = nsm_mpls_l2_circuit_del2 (nm, argv[0], val);
  if (ret < 0)
    {
      switch (ret)
        {
        case NSM_ERR_VC_ID_NOT_FOUND:
          cli_out (cli, "%% Virtual Circuit with identifier %s not found\n",
                   argv[1]);
          break;
        case NSM_ERR_VC_NAME_NOT_FOUND:
          cli_out (cli, "%% Virtual Circuit with name %s not found\n",
                   argv[0]);
          break;
        case NSM_ERR_VC_NAME_ID_MISMATCH:
          cli_out (cli, "%% Virtual Circuit %s not bound to identifier %s\n",
                   argv[0], argv[1]);
          break;
        default:
          break;
        }
      return CLI_ERROR;
    }

  return CLI_SUCCESS;
}

ALI (no_mpls_l2_circuit,
     no_mpls_l2_circuit_cmd,
     "no mpls l2-circuit NAME <1-4294967295> A.B.C.D",
     CLI_NO_STR,
     NO_CONFIG_MPLS_STR,
     "Specify the MPLS Layer-2 Virtual Circuit to be removed",
     "Identifying string used for the MPLS Layer-2 Virtual Circuit",
     "Identifying value used for MPLS Layer-2 Virtual Circuit",
     "IPv4 Address used for endpoint for MPLS Layer-2 Virtual Circuit");

ALI (no_mpls_l2_circuit,
     no_mpls_l2_circuit_with_grp_cmd,
     "no mpls l2-circuit NAME <1-4294967295> A.B.C.D GROUPNAME",
     CLI_NO_STR,
     NO_CONFIG_MPLS_STR,
     "Specify the MPLS Layer-2 Virtual Circuit to be removed",
     "Identifying string used for the MPLS Layer-2 Virtual Circuit",
     "Identifying value used for MPLS Layer-2 Virtual Circuit",
     "IPv4 Address used for endpoint for MPLS Layer-2 Virtual Circuit",
     "Group name identifier used");

ALI (no_mpls_l2_circuit,
     no_mpls_l2_circuit_with_grp_cw_cmd,
     "no mpls l2-circuit NAME <1-4294967295> A.B.C.D GROUPNAME control-word",
     CLI_NO_STR,
     NO_CONFIG_MPLS_STR,
     "Specify the MPLS Layer-2 Virtual Circuit to be removed",
     "Identifying string used for the MPLS Layer-2 Virtual Circuit",
     "Identifying value used for MPLS Layer-2 Virtual Circuit",
     "IPv4 Address used for endpoint for MPLS Layer-2 Virtual Circuit",
     "Group name identifier used",
     "Control-word");

/* CLI to manually Switchover between VC's. */
CLI (mpls_vc_switchover,
     mpls_vc_switchover_cmd,
     "vc-switchover NAME NAME",
     "virtual-circuit switching",
     "name of VC in use",
     "name of VC to be switched to")
{
  int ret = 0;

  if ((cli == NULL) || (cli->vr == NULL))
    return CLI_ERROR;

  ret = nsm_mpls_api_pw_switchover (cli->vr->id, argv[0], argv[1]);

  return nsm_cli_return (cli, ret);

}

/* Show VC data. */
CLI (mpls_vc_table,
     mpls_vc_table_cmd,
     "show mpls vc-table",
     CLI_SHOW_STR,
     SHOW_MPLS_STR,
     "Show MPLS Virtual Circuit table")
{
  struct nsm_master *nm = cli->vr->proto;
  struct nsm_mpls_circuit *vc = NULL;
  struct interface *ifp_ac = NULL, *ifp_nw = NULL;
  bool_t heading;
  struct route_node *rn;
  struct nhlfe_key *key;

  if ((! NSM_MPLS) || (! NSM_MPLS->vc_table))
    return CLI_SUCCESS;

  heading = NSM_FALSE;
  for (rn = route_top (NSM_MPLS->vc_table); rn; rn = route_next (rn))
    {
      /* If no info, skip. */
      vc = rn->info;
      if (! vc || (! vc->vc_fib))
        continue;

      if (heading == NSM_FALSE)
        {
          heading = NSM_TRUE;
          cli_out (cli, "VC-ID      Vlan-ID  Access-Intf   Network-Intf  "
              "Out Label  Tunnel-Label  Nexthop         Status\n");
        }

      if (vc->vc_fib->ac_if_ix > 0)
        NSM_MPLS_GET_INDEX_PTR (PAL_FALSE, vc->vc_fib->ac_if_ix, ifp_ac);
      if (vc->vc_fib->nw_if_ix > 0)
        {
#ifdef HAVE_GMPLS
          if (vc->ftn)
            {
              NSM_MPLS_GET_INDEX_PTR (CHECK_FLAG (vc->ftn->flags, 
                    FTN_ENTRY_FLAG_GMPLS), vc->vc_fib->nw_if_ix, ifp_nw);
            }
          else
#endif /* HAVE_GMPLS */
            NSM_MPLS_GET_INDEX_PTR (PAL_FALSE, vc->vc_fib->nw_if_ix, ifp_nw);
        }

      cli_out (cli, "%-7u", vc->id);

      /* vc_info may be NULL in MSPW case as the vc_info is be created
       * after receiving message from LDP, for signaled segments.
       */
      if ((vc->vc_info) && (vc->vc_info->vlan_id))
        cli_out (cli, "%-9d", vc->vc_info->vlan_id);
      else
        cli_out (cli, "%-9s", "N/A");

      cli_out (cli, "%-14s", ((ifp_ac == NULL) ? "-" : ifp_ac->name));
      cli_out (cli, "%-14s", ((ifp_nw == NULL) ? "-" : ifp_nw->name));
      cli_out (cli, "%-11d", vc->vc_fib->out_label);

      if (vc->ftn && (FTN_XC (vc->ftn)) && (FTN_NHLFE (vc->ftn)))
        {
          key =  (struct nhlfe_key *) (FTN_NHLFE (vc->ftn))->nkey;
          cli_out (cli, "%-14d", key->u.ipv4.out_label);
        }
      else
        cli_out (cli, "%-14s", "N/A");

      if (vc->address.family == AF_INET)
        cli_out (cli, "%-16r", &vc->address.u.prefix4);
#ifdef HAVE_IPV6
      else if (vc->address.family == AF_INET6)
        cli_out (cli, "%-40R", &vc->address.u.prefix6);
#endif /* HAVE_IPV6 */
      else
        cli_out (cli, "%-16s", "-");

      if (vc->vc_fib->install_flag == NSM_TRUE)
        cli_out (cli, "%s", "Active");
      else if (vc->state == NSM_MPLS_L2_CIRCUIT_UP &&
          ! IS_NSM_CODE_PW_FAULT (vc->pw_status) &&
          ! IS_NSM_CODE_PW_FAULT (vc->remote_pw_status))
        cli_out (cli, "%s", "Standby");
      else
        cli_out (cli, "%s", "Inactive");

      cli_out (cli, "\n");
    }

  return CLI_SUCCESS;
}

#ifdef HAVE_MS_PW
CLI (mpls_ms_pw_stitch_vc,
     mpls_ms_pw_stitch_vc_cmd,
     "mpls ms-pw-stitch MS_PW_NAME VC1_NAME VC2_NAME "
     "(((mtu <68-9216>) (ethernet|(vlan <2-4096>)))|)",
     CONFIG_MPLS_STR,
     "add a ms-pw-stitch command",
     "MS-PW stitching identifying string",
     "Virtual Circuit-1 name",
     "Virtual Circuit-2 name",
     "Interface MTU size  - needed when one vc is not signaled",
     "mtu size <68-9216>",
     "VC type Ethernet - needed when one vc is not signaled",
     "VC type Ethernet VLAN - needed when one vc is not signaled",
     "Vlan ID between 2 - 4096")
{
  struct nsm_master *nm = cli->vr->proto;
  u_int32_t ret = NSM_SUCCESS;
  u_int32_t vlan_id = 0, mtu = 0;

  if (!nm)
    {
      cli_out (cli, "Unable to find the context\n");
      return CLI_ERROR;
    }

  if (argc == 3)
    ret = nsm_mpls_ms_pw_stitch_vc_instance (nm, argv[0], argv[1],
                                             argv[2], NSM_TRUE,
                                             NSM_FALSE, NSM_FALSE);
  else if (argc > 3)
    {
      CLI_GET_UINT32_RANGE ("MTU", mtu, argv[4], NSM_MPLS_MTU_MIN,
                            NSM_MPLS_MTU_MAX);

      if (pal_strncmp (argv[5], "e", 1) == 0)
          vlan_id = 0;
      else if (pal_strncmp (argv[5], "v", 1) == 0)
        {
          CLI_GET_INTEGER_RANGE ("VLAN id", vlan_id, argv[6],
                                 NSM_MPLS_VLAN_CLI_MIN, NSM_MPLS_VLAN_CLI_MAX);
        }
      ret = nsm_mpls_ms_pw_stitch_vc_instance (nm, argv[0], argv[1],
                                              argv[2], NSM_TRUE,
                                              vlan_id, mtu);
    }

  return nsm_cli_return (cli, ret);
}

CLI (no_mpls_ms_pw_stitch_vc,
     no_mpls_ms_pw_stitch_vc_cmd,
     "no mpls ms-pw-stitch MS_PW_NAME VC1_NAME VC2_NAME",
     CLI_NO_STR,
     NO_CONFIG_MPLS_STR,
     "delete ms-pw-stitch command",
     "MS-PW stitching identifying string",
     "Virtual Circuit-1 name",
     "Virtual Circuit-2 name")
{
  struct nsm_master *nm = cli->vr->proto;
  u_int32_t ret = NSM_SUCCESS;

  if (!nm)
    {
      cli_out (cli, "Unable to find the context\n");
      return CLI_ERROR;
    }

  ret = nsm_mpls_ms_pw_stitch_vc_instance (nm, argv[0], argv[1],
                                           argv[2], NSM_FALSE,
                                           NSM_FALSE, NSM_FALSE);

  return nsm_cli_return (cli, ret);
}


CLI (mpls_ms_pw_spe_descr,
    mpls_ms_pw_spe_descr_cmd,
    "mpls ms-pw MS-PW S-PE-DESCR",
    CONFIG_MPLS_STR,
    "MS-PW confirmation string",
    "MS-PW identifying string",
    "S-PE description, upto 80 characters long")
{
  struct nsm_master *nm = cli->vr->proto;
  int ret = CLI_SUCCESS;
  bool_t is_add = NSM_TRUE;
  if (!nm)
    {
      cli_out (cli, "Unable to find the context\n");
      return CLI_ERROR;
    }

  if ((pal_strlen (argv[1])) > NSM_MPLS_MS_PW_DESCR_LEN)
    {
      cli_out (cli, "INVALID S-PE Description Length\n");
      return CLI_ERROR;
    }

  ret = nsm_mpls_ms_pw_set_spe_descr (nm, argv[0], argv[1], is_add);

  return nsm_cli_return (cli, ret);
}

CLI (no_mpls_ms_pw_spe_descr,
     no_mpls_ms_pw_spe_descr_cmd,
     "no mpls ms-pw MS-PW S-PE-DESCR",
     CLI_NO_STR,
     NO_CONFIG_MPLS_STR,
     "MS-PW confirmation string",
     "MS-PW identifying string",
     "S-PE description, upto 80 characters long")
{
  struct nsm_master *nm = cli->vr->proto;
  int ret = CLI_SUCCESS;
  bool_t is_add = NSM_FALSE;
  if (!nm)
    {
      cli_out (cli, "Unable to find the context\n");
      return CLI_ERROR;
    }

  if ((argc == 2) &&
      ((pal_strlen (argv[1])) > NSM_MPLS_MS_PW_DESCR_LEN))
    {
      cli_out (cli, "INVALID S-PE Description Length\n");
      return CLI_ERROR;
    }

  ret = nsm_mpls_ms_pw_set_spe_descr (nm, argv[0], argv[1], is_add);

  return nsm_cli_return (cli, ret);
}

CLI (show_mpls_mspw_det,
    show_mpls_mspw_det_cmd,
    "show mpls ms-pw ((NAME (vc-table|)|)|)",
    CLI_SHOW_STR,
    SHOW_MPLS_STR,
    "Multi-Segment PW information",
    "Name of MS-PW",
    "VC table details")
{
  struct nsm_master *nm = cli->vr->proto;
  struct vrep_table *vrep = NULL;

  if (!nm)
    {
      cli_out (cli, "Unable to find the context\n");
      return CLI_ERROR;
    }

  /* MSPW VC table */
  if (argc == 2)
    {
      nsm_mpls_ms_pw_show_vc_tab (nm, cli, argv[0]);
      return CLI_SUCCESS;
    }
  /* MSPW detail */
  else if (argc == 1)
    {
      vrep = vrep_create (NSM_MPLS_MS_PW_DET_CLI_NUM_COLS, VREP_MAX_ROW_WIDTH);
      if (!vrep)
        {
          cli_out (cli, "Could not alloc memory\n");
          return CLI_ERROR;
        }

      nsm_mpls_ms_pw_show_det (nm, argv[0], vrep);
    }
  /* MSPW summary */
  else
    {
      vrep = vrep_create (NSM_MPLS_MS_PW_CLI_NUM_COLS, VREP_MAX_ROW_WIDTH);
      if (!vrep)
        {
          cli_out (cli, "Could not alloc memory\n");
          return CLI_ERROR;
        }

      if (NSM_MS_PW_HASH != NULL)
        {
          vrep_add (vrep, 0, 0,
                    "MS-PW \t Segment-1 \t VC1-ID \t Segment-2 \t VC2-ID");
          hash_iterate (NSM_MS_PW_HASH,
                ((void (*) (struct hash_backet *, void *))nsm_mpls_ms_pw_show),
                vrep);
        }
    }
  vrep_show (vrep, nsm_mpls_ms_pw_vrep_show, (intptr_t)cli);
  vrep_delete (vrep);

  return VREP_SUCCESS;
}

#endif /* HAVE_MS_PW */
#endif /* HAVE_MPLS_VC */

#ifdef HAVE_VPLS
void
nsm_mpls_vpls_dump_detail_cli (struct cli *cli, struct nsm_vpls *vpls)
{
  struct nsm_vpls_peer *peer;
  struct nsm_vpls_spoke_vc *svc;
  struct nsm_mpls_vc_info *vc_info;
  struct route_node *rn;
  struct listnode *ln;
  u_char first;
  int count;

  if (! vpls)
    return;

  cli_out (cli, "Virtual Private LAN Service Instance: %s, ID: %u\n",
           vpls->vpls_name,
           vpls->vpls_id);

  cli_out (cli, " Group ID: %u, VPLS Type: %s, Configured MTU: %d\n",
           vpls->grp_id,
           mpls_vc_type_to_str (vpls->vpls_type),
           vpls->ifmtu);

  cli_out (cli, " Description: %s\n",
           vpls->desc ? vpls->desc : "none");

  /* Configure interfaces. */
  cli_out (cli, " Configured interfaces: ");
  count = 0;
  if (LISTCOUNT (vpls->vpls_info_list))
    {
      LIST_LOOP (vpls->vpls_info_list, vc_info, ln)
        {
          if (vc_info && vc_info->mif && vc_info->mif->ifp)
            {
              cli_out (cli, "%s", vc_info->mif->ifp->name);
              count++;
              if (count == 1 || count % 6)
                cli_out (cli, " ");
              else
                {
                  cli_out (cli, "\n");
                  if (count < LISTCOUNT (vpls->vpls_info_list))
                    cli_out (cli, "                      : ");
                }
            }
        }
    }
  if (count == 0)
    cli_out (cli, "none\n");
  else if (count == 1 || count % 6)
    cli_out (cli, "\n");

  /* Mesh Peers. */
  first = NSM_FALSE;
  for (rn = route_top (vpls->mp_table); rn; rn = route_next (rn))
    if ((peer = rn->info) != NULL)
      {
        if (first == NSM_FALSE)
          {
            first = NSM_TRUE;
            cli_out (cli, " Mesh Peers:  ");

            if (peer->peer_addr.afi == AFI_IP)
              cli_out (cli, "%r",
                       &peer->peer_addr.u.ipv4);
#ifdef HAVE_IPV6
            else if (peer->peer_addr.afi == AFI_IP6)
              cli_out (cli, "%40R",
                       &peer->peer_addr.u.ipv6);
#endif /* HAVE_IPV6 */
          }
        else
          {
            if (peer->peer_addr.afi == AFI_IP)
              cli_out (cli, "              %r",
                       &peer->peer_addr.u.ipv4);
#ifdef HAVE_IPV6
            else if (peer->peer_addr.afi == AFI_IP6)
              cli_out (cli, "              %40R",
                       &peer->peer_addr.u.ipv6);
#endif /* HAVE_IPV6 */
          }

        cli_out (cli, " (%s)\n",
                 peer->state == NSM_VPLS_PEER_UP
                 ? "Up" : "Dn");
      }

  /* Spoke Peers. */
  first = NSM_FALSE;
  LIST_LOOP (vpls->svc_list, svc, ln)
    {
      if (first == NSM_FALSE)
        {
          first = NSM_TRUE;
          cli_out (cli, " Spoke Peers: %s", svc->vc_name);
        }
      else
        cli_out (cli, "              %s", svc->vc_name);

      cli_out (cli, " (%s)\n",
               svc->state == NSM_VPLS_SPOKE_VC_UP
               ? "Up" : "Dn");
    }
}

/* Dump summarized VPLS mesh data. */
static void
nsm_mpls_vpls_mesh_dump (struct cli *cli,
                         struct nsm_vpls *vpls,
                         struct nsm_vpls_peer *peer,
                         u_char print_header)
{
  struct nsm_master *nm = cli->vr->proto;
  struct interface *ifp = NULL;
  struct nhlfe_key *key = NULL;

  if (print_header)
#ifdef HAVE_SNMP
    cli_out (cli, "VPLS-ID    Peer Addr         Tunnel-Label  "
             "In-Label   Network-Intf   Out-Label  Lkps/St   PW-INDEX\n");
#else
    cli_out (cli, "VPLS-ID    Peer Addr    Tunnel-Label "
             "In-Label     Network-Intf   Out-Label  Lkps/St ");
#endif /*HAVE_SNMP*/

  /* ID. */
  cli_out (cli, "%-11u", vpls->vpls_id);

  /* Peer address. */
  cli_out (cli, "%-18r", &peer->peer_addr.u.ipv4);

  /* Tunnel-Label */
  if (peer->ftn && (FTN_XC (peer->ftn)) && (FTN_NHLFE (peer->ftn)))
    {
      key =  (struct nhlfe_key *) (FTN_NHLFE (peer->ftn))->nkey;
      cli_out (cli, "%-14d", key->u.ipv4.out_label);
    }
  else
    cli_out (cli, "%-14s", "N/A");

  if (peer->vc_fib)
    {
#ifdef HAVE_GMPLS
      if (peer->ftn)
        {
          NSM_MPLS_GET_INDEX_PTR (CHECK_FLAG (peer->ftn->flags, 
                FTN_ENTRY_FLAG_GMPLS), peer->vc_fib->nw_if_ix, ifp);
        }
      else
#endif /* HAVE_GMPLS */
        NSM_MPLS_GET_INDEX_PTR (PAL_FALSE, peer->vc_fib->nw_if_ix, ifp);

      cli_out (cli, "%-11d", peer->vc_fib->in_label);
      cli_out (cli, "%-15s", ifp ? ifp->name : "N/A");
      cli_out (cli, "%-11d", peer->vc_fib->out_label);
      cli_out (cli, "%d/%-8s",
               peer->vc_fib->opcode == PUSH_FOR_VC ? 1
               : peer->vc_fib->opcode == PUSH_AND_LOOKUP_FOR_VC ? 2 : 0,
               peer->state == NSM_VPLS_PEER_UP ? "Up" : "Dn");
#ifdef HAVE_SNMP
      cli_out (cli, "%d", peer->vc_snmp_index);
#endif /*HAVE_SNMP */
    }
  else
    {
      cli_out (cli, "%-11s", "none");
      cli_out (cli, "%-15s", "N/A");
      cli_out (cli, "%-11s", "none");
      cli_out (cli, "%-10s", "0/Dn");
#ifdef HAVE_SNMP
      cli_out (cli, "%d", peer->vc_snmp_index);
#endif /*HAVE_SNMP */
    }

  cli_out (cli, "\n");
}

CLI (show_mpls_vpls_mesh_fwding,
     show_mpls_vpls_mesh_fwding_cmd,
     "show mpls vpls mesh",
     CLI_SHOW_STR,
     SHOW_MPLS_STR,
     "Show MPLS VPLS instance information",
     "Show MPLS VPLS Mesh Forwarding information")
{
  struct nsm_master *nm = cli->vr->proto;
  struct nsm_vpls *vpls;
  struct nsm_vpls_peer *peer;
  struct ptree_node *pn;
  struct route_node *rn;
  u_char first = NSM_FALSE;

  if (! NSM_MPLS || ! NSM_MPLS->vpls_table)
    return CLI_SUCCESS;

  for (pn = ptree_top (NSM_MPLS->vpls_table); pn; pn = ptree_next (pn))
    if ((vpls = pn->info) != NULL)
      for (rn = route_top (vpls->mp_table); rn; rn = route_next (rn))
        if ((peer = rn->info) != NULL
            && peer->peer_addr.afi == AFI_IP)
          {
            if (first == NSM_FALSE)
              {
                first = NSM_TRUE;
                nsm_mpls_vpls_mesh_dump (cli, vpls, peer, NSM_TRUE);
              }
            else
              nsm_mpls_vpls_mesh_dump (cli, vpls, peer, NSM_FALSE);
          }

  return CLI_SUCCESS;
}

/* Dump summarized VPLS spoke data. */
static void
nsm_mpls_vpls_spoke_dump (struct cli *cli,
                          struct nsm_vpls *vpls,
                          struct nsm_vpls_spoke_vc *svc,
                          u_char print_header)
{
  struct interface *ifp = NULL;
  struct nhlfe_key *key = NULL;
  struct nsm_master *nm = cli->vr->proto;

  if (print_header)
    cli_out (cli, "VPLS-ID    Virtual Circuit  Tunnel-Label "
             "In-Label   Network-Intf   Out-Label  Lkps/St\n");

  /* ID. */
  cli_out (cli, "%-11u", vpls->vpls_id);

  /* Peer address. */
  cli_out (cli, "%-17s", svc->vc_name);

  /* Tunnel-Label */
  if (svc->vc)
    {
      if (svc->vc->ftn && (FTN_XC (svc->vc->ftn)) && (FTN_NHLFE (svc->vc->ftn)))
        {
          key =  (struct nhlfe_key *) (FTN_NHLFE (svc->vc->ftn))->nkey;
          cli_out (cli, "%-14d", key->u.ipv4.out_label);
        }
      else
        cli_out (cli, "%-14s", "N/A");
    }
  else
    cli_out (cli, "%-14d", 0);


  if (svc->vc_fib)
    {
      NSM_MPLS_GET_INDEX_PTR (PAL_FALSE, svc->vc_fib->nw_if_ix, ifp);

      cli_out (cli, "%-11d", svc->vc_fib->in_label);
      cli_out (cli, "%-15s", ifp ? ifp->name : "N/A");
      cli_out (cli, "%-11d", svc->vc_fib->out_label);
      cli_out (cli, "%d/%s",
               svc->vc_fib->opcode == PUSH_FOR_VC ? 1
               : svc->vc_fib->opcode == PUSH_AND_LOOKUP_FOR_VC ? 2 : 0,
               svc->state == NSM_VPLS_SPOKE_VC_UP ? "Up" : "Dn");
    }
  else
    {
      cli_out (cli, "%-11s", "none");
      cli_out (cli, "%-15s", "N/A");
      cli_out (cli, "%-11s", "none");
      cli_out (cli, "0/Dn");
    }

  cli_out (cli, "\n");
}

CLI (show_mpls_vpls_spoke_fwding,
     show_mpls_vpls_spoke_fwding_cmd,
     "show mpls vpls spoke",
     CLI_SHOW_STR,
     SHOW_MPLS_STR,
     "Show MPLS VPLS instance information",
     "Show MPLS VPLS Spoke Forwarding information")
{
  struct nsm_master *nm = cli->vr->proto;
  struct nsm_vpls *vpls;
  struct nsm_vpls_spoke_vc *svc;
  struct ptree_node *pn;
  struct listnode *ln;
  u_char first = NSM_FALSE;

  if (! NSM_MPLS || ! NSM_MPLS->vpls_table)
    return CLI_SUCCESS;

  for (pn = ptree_top (NSM_MPLS->vpls_table); pn; pn = ptree_next (pn))
    if ((vpls = pn->info) != NULL)
      {
        LIST_LOOP (vpls->svc_list, svc, ln)
          {
            if (first == NSM_FALSE)
              {
                first = NSM_TRUE;
                nsm_mpls_vpls_spoke_dump (cli, vpls, svc, NSM_TRUE);
              }
            else
              nsm_mpls_vpls_spoke_dump (cli, vpls, svc, NSM_FALSE);
          }
      }

  return CLI_SUCCESS;
}

CLI (show_mpls_vpls,
     show_mpls_vpls_cmd,
     "show mpls vpls",
     CLI_SHOW_STR,
     SHOW_MPLS_STR,
     "Show MPLS VPLS instance information")
{
  struct nsm_master *nm = cli->vr->proto;
  struct nsm_vpls *vpls;
  struct ptree_node *pn;
  u_char first = NSM_FALSE;

  if (! NSM_MPLS || ! NSM_MPLS->vpls_table)
    return CLI_SUCCESS;

  for (pn = ptree_top (NSM_MPLS->vpls_table); pn; pn = ptree_next (pn))
    if ((vpls = pn->info) != NULL)
      {
        if (first == NSM_FALSE)
          {
            first = NSM_TRUE;
            cli_out (cli, "Name          VPLS-ID     Type              "
                     "MPeers    SPeers\n");
          }

        /* Name. */
        cli_out (cli, "%-14s", vpls->vpls_name);

        /* ID. */
        cli_out (cli, "%-12u", vpls->vpls_id);

        /* Type. */
        cli_out (cli, "%-18s", mpls_vc_type_to_str (vpls->vpls_type));

        /* Mesh Peers. */
        cli_out (cli, "%-10d", vpls->mp_count);

        /* Spoke Peers. */
        cli_out (cli, "%-10d", LISTCOUNT (vpls->svc_list));

        cli_out (cli, "\n");
      }

  return CLI_SUCCESS;
}

CLI (show_mpls_vpls_name,
     show_mpls_vpls_name_cmd,
     "show mpls vpls NAME",
     CLI_SHOW_STR,
     SHOW_MPLS_STR,
     "Show MPLS VPLS instance information",
     "Identifying string for VPLS")
{
  struct nsm_master *nm = cli->vr->proto;
  struct nsm_vpls *vpls;

  if (NSM_MPLS && NSM_MPLS->vpls_table)
    {
      vpls = nsm_vpls_lookup_by_name (nm, argv[0]);
      if (vpls)
        {
          nsm_mpls_vpls_dump_detail_cli (cli, vpls);
          return CLI_SUCCESS;
        }
    }

  cli_out (cli, "%% No VPLS with name: %s configured.\n", argv[0]);
  return CLI_ERROR;
}

CLI (show_mpls_vpls_name_mesh,
     show_mpls_vpls_name_mesh_cmd,
     "show mpls vpls NAME mesh",
     CLI_SHOW_STR,
     SHOW_MPLS_STR,
     "Show MPLS VPLS instance information",
     "Identifying string for VPLS",
     "Show MPLS VPLS Mesh Forwarding information")
{
  struct nsm_master *nm = cli->vr->proto;
  struct nsm_vpls *vpls;
  struct route_node *rn;
  struct nsm_vpls_peer *peer;
  u_char first = NSM_FALSE;

  if (NSM_MPLS && NSM_MPLS->vpls_table)
    {
      vpls = nsm_vpls_lookup_by_name (nm, argv[0]);
      if (vpls)
        for (rn = route_top (vpls->mp_table); rn; rn = route_next (rn))
          if ((peer = rn->info) != NULL && peer->peer_addr.afi == AFI_IP)
            {
              if (first == NSM_FALSE)
                {
                  first = NSM_TRUE;
                  nsm_mpls_vpls_mesh_dump (cli, vpls, peer, NSM_TRUE);
                }
              else
                nsm_mpls_vpls_mesh_dump (cli, vpls, peer, NSM_FALSE);
            }

      return CLI_SUCCESS;
    }

  cli_out (cli, "%% No VPLS with name: %s configured.\n",
           argv[0]);
  return CLI_ERROR;

}

CLI (show_mpls_vpls_name_spoke,
     show_mpls_vpls_name_spoke_cmd,
     "show mpls vpls NAME spoke",
     CLI_SHOW_STR,
     SHOW_MPLS_STR,
     "Show MPLS VPLS instance information",
     "Identifying string for VPLS",
     "Show MPLS VPLS Spoke Forwarding information")
{
  struct nsm_master *nm = cli->vr->proto;
  struct nsm_vpls *vpls;
  struct listnode *ln;
  struct nsm_vpls_spoke_vc *svc;
  u_char first = NSM_FALSE;

  if (NSM_MPLS && NSM_MPLS->vpls_table)
    {
      vpls = nsm_vpls_lookup_by_name (nm, argv[0]);
      if (vpls)
        {
          LIST_LOOP (vpls->svc_list, svc, ln)
            {
              if (first == NSM_FALSE)
                {
                  first = NSM_TRUE;
                  nsm_mpls_vpls_spoke_dump (cli, vpls, svc, NSM_TRUE);
                }
              else
                nsm_mpls_vpls_spoke_dump (cli, vpls, svc, NSM_FALSE);
            }
        }

      return CLI_SUCCESS;
    }

  cli_out (cli, "%% No VPLS with name: %s configured.\n",
           argv[0]);
  return CLI_ERROR;
}

CLI (show_mpls_vpls_detail,
     show_mpls_vpls_detail_cmd,
     "show mpls vpls detail",
     CLI_SHOW_STR,
     SHOW_MPLS_STR,
     "Show MPLS VPLS instance information",
     "Show detailed VPLS information")
{
  struct nsm_master *nm = cli->vr->proto;
  struct nsm_vpls *vpls;
  struct ptree_node *pn;
  u_char first = NSM_FALSE;

  if (! NSM_MPLS || ! NSM_MPLS->vpls_table)
    return CLI_SUCCESS;

  for (pn = ptree_top (NSM_MPLS->vpls_table); pn; pn = ptree_next (pn))
    {
      if ((vpls = pn->info) != NULL)
        {
          if (first == NSM_FALSE)
            first = NSM_TRUE;
          else
            cli_out (cli, "\n");

          nsm_mpls_vpls_dump_detail_cli (cli, vpls);
        }
    }

  return CLI_SUCCESS;
}

int
nsm_vpls_create_cli (struct cli *cli, char *sz_name, u_int32_t vid)
{
  struct nsm_master *nm = cli->vr->proto;
  struct nsm_vpls *vpls;
  int ret;

  /* Create vpls instance. */
  vpls = NULL;
  ret = nsm_vpls_add_process (nm,
                              sz_name,
                              vid,
                              VC_TYPE_ETHERNET,
                              IF_ETHER_DEFAULT_MTU,
                              0,
                              &vpls);
  if (ret < 0)
    {
      switch (ret)
        {
        case NSM_ERR_INVALID_VPLS_ID:
          cli_out (cli, "%% Invalid VPLS-ID specified. Same ID cannot be "
                   "used by multiple VC/VPLS instances.\n");
          break;
        default:
          cli_out (cli, "%% Failed to create VPLS instance.\n");
          break;
        }

      return CLI_ERROR;
    }

  cli->mode = NSM_VPLS_MODE;
  cli->index = vpls;



  return CLI_SUCCESS;
}

int
nsm_vpls_delete_cli (struct cli *cli, char *sz_name, char *sz_id)
{
  struct nsm_master *nm = cli->vr->proto;
  int ret;
  u_int32_t vid;
  struct nsm_vpls *vpls;

  /* Get VPLS ID */
  if (sz_id)
    {
      CLI_GET_INTEGER_RANGE ("VPLS Identifier", vid, sz_id,
                             VPLS_ID_MIN, VPLS_ID_MAX);
      vpls = nsm_vpls_lookup (nm, vid, NSM_FALSE);
    }
  else
    vpls = nsm_vpls_lookup_by_name (nm, sz_name);

  if (! vpls)
    {
      cli_out (cli, "%% VPLS not found. \n");
      return CLI_ERROR;
    }

  ret = nsm_vpls_delete_process (nm, vpls);
  if (ret < 0)
    {
      cli_out (cli, "%% Failed to delete VPLS instance.\n");
      return CLI_ERROR;
    }

  return CLI_SUCCESS;
}

/** @brief API for adding VPLS mesh peer.

    @param vr_id         - VR identifier
    @param vpls_id       - VPLS identifier
    @param sz_peer       - Peer address
    @param mapping_type  - Mapping type
    @param tunnel_info   - tunnel name/id
    @param tunnel_dir    - forward/reverse lsp 
    @param fec_type_vc   - Identify VC type
    @param sz_mode       - mode
    @param sec_peer      - secondary peer address
*/
int
nsm_vpls_mesh_peer_ipv4_add_cli (u_int32_t vr_id, u_int32_t vpls_id,
                                 char *sz_peer, u_char mapping_type,
                                 char *tunnel_info, u_char tunnel_dir,
                                 u_int8_t fec_type_vc,
                                 char *sz_mode, char *sec_peer)
{
  struct nsm_master *nm = NULL;
  struct nsm_vpls *vpls = NULL;
  struct addr_in addr, sec_addr;
  int ret = 0;

  nm = nsm_master_lookup_by_id (nzg, vr_id);
  if (!nm)
    return NSM_API_SET_ERR_MASTER_NOT_EXIST;

  /* VPLS validation. */
  vpls = nsm_vpls_lookup_by_id (nm, vpls_id);
  if (!vpls)
    return NSM_ERR_VPLS_NOT_FOUND;

  if (sz_peer == NULL)
    return NSM_ERR_INVALID_ARGS;

  addr.afi = AFI_IP;
  ret = pal_inet_pton (AF_INET, sz_peer, &addr.u.ipv4);

  if (ret < 0)
    return NSM_ERR_INVALID_ARGS;
  if (sec_peer)
    {
      sec_addr.afi = AFI_IP;
      ret = pal_inet_pton (AF_INET, sec_peer, &sec_addr.u.ipv4);
      if (ret < 0)
        return NSM_ERR_INVALID_ARGS;
    }

  return nsm_vpls_mesh_peer_add_process (nm, vpls, &addr, mapping_type,
                                         tunnel_info, tunnel_dir, 
                                         fec_type_vc, sz_mode,
                                         &sec_addr);
}

/** @brief API to delete VPLS mesh peer.

    @param vr_id         - VR identifier
    @param vpls_id       - VPLS identifier
    @param sz_peer       - Peer address
    @param mapping_type  - Mapping type
    @param tunnel_info   - tunnel name/id
    @param tunnel_dir   -  forward/reverse
    @param fec_type_vc   - Identify VC type
    @param sz_mode       - mode
    @param sec_peer      - secondary peer address
 */
int
nsm_vpls_mesh_peer_ipv4_delete_cli (u_int32_t vr_id, u_int32_t vpls_id,
                                    char *sz_peer, u_char mapping_type,
                                    char *tunnel_info, u_char tunnel_dir,
                                    u_int8_t fec_type_vc,
                                    u_char params)
{
  struct nsm_master *nm = NULL;
  struct nsm_vpls_peer *peer = NULL;
  struct nsm_vpls *vpls = NULL;
  struct addr_in addr;
  char *temp_tunnel_name = NULL;
  u_int32_t temp_tunnel_id = -1;
  int ret = 0;

  nm = nsm_master_lookup_by_id (nzg, vr_id);
  if (! nm)
    return NSM_API_SET_ERR_MASTER_NOT_EXIST;

  /* VPLS validation. */
  vpls = nsm_vpls_lookup_by_id (nm, vpls_id);
  if ( !vpls)
    return NSM_ERR_VPLS_NOT_FOUND;

  if (mapping_type != MPLS_VC_MAPPING_NONE && tunnel_info == NULL)
    return NSM_ERR_INVALID_ARGS;

  if (sz_peer == NULL)
    return NSM_ERR_INVALID_ARGS;

  addr.afi = AFI_IP;
  ret = pal_inet_pton (AF_INET, sz_peer, &addr.u.ipv4);

  peer = nsm_vpls_mesh_peer_lookup (vpls, &addr, NSM_FALSE);
  if (! peer)
    return NSM_ERR_VPLS_NOT_FOUND;

  /* Validate parameters. */
  if (mapping_type == MPLS_VC_MAPPING_TUNNEL_NAME)
    {
      temp_tunnel_name = XSTRDUP (MTYPE_TMP, tunnel_info);
      if (pal_strcmp (temp_tunnel_name, peer->tunnel_name) != 0)
        {
          XFREE (MTYPE_TMP, tunnel_info);
          return NSM_ERR_VPLS_TUNNEL_NAME_MISMATCH;
        }

      if (tunnel_dir != TUNNEL_DIR_NONE &&
          peer->tunnel_dir != tunnel_dir)
        return NSM_ERR_VPLS_TUNNEL_DIR_MISMATCH;
    }
  else if (mapping_type == MPLS_VC_MAPPING_TUNNEL_ID)
    {
      temp_tunnel_id = pal_strtou32 (tunnel_info, NULL, 10);
      if (temp_tunnel_id != peer->tunnel_id)
        return NSM_ERR_VPLS_TUNNEL_ID_MISMATCH;

      if (tunnel_dir != TUNNEL_DIR_NONE &&
          peer->tunnel_dir != tunnel_dir)
        return NSM_ERR_VPLS_TUNNEL_DIR_MISMATCH;
    }

  if ((params == CLI_MAX_PARAMS_ENTRY_DELETE) &&
      (peer->fec_type_vc != fec_type_vc))
    return NSM_ERR_VPLS_VC_TYPE_MISMATCH;

  return nsm_vpls_mesh_peer_delete_process (nm, vpls, &addr);
}

int
nsm_vpls_vc_add_cli (struct cli *cli, struct nsm_vpls *vpls,
                     char *vc_name, char *str1, char *sz_mode,
                     char *sz_name)
{
  int ret;
  u_int16_t vc_type = VC_TYPE_ETHERNET;

  if (str1 && str1[0] == 'v')
    vc_type = VC_TYPE_ETH_VLAN;

  ret = nsm_vpls_spoke_vc_add_process (vpls, vc_name, vc_type, sz_mode, sz_name);
  if (ret < 0)
    {
      cli_out (cli, "%% Failed to add Layer-2 MPLS Virtual Circuit to VPLS.\n");
      return CLI_ERROR;
    }

  return CLI_SUCCESS;
}


int
nsm_vpls_vc_delete_cli (struct cli *cli, struct nsm_vpls *vpls,
                        char *vc_name)
{
  struct nsm_master *nm = cli->vr->proto;
  int ret;

  ret = nsm_vpls_spoke_vc_delete_process (nm, vpls, vc_name);
  if (ret < 0)
    {
      cli_out (cli, "%% Failed to delete vc from VPLS.\n");
      return CLI_ERROR;
    }

  return CLI_SUCCESS;
}


CLI (mpls_vpls,
     mpls_vpls_cmd,
     "mpls vpls NAME <1-4294967295>",
     CONFIG_MPLS_STR,
     "Create an instance of MPLS based Virtual Private Lan Service (VPLS)",
     "Identifying string for VPLS",
     "Identifying value for VPLS")
{
  u_int32_t vid;

  /* Get VPLS ID. */
  CLI_GET_UINT32_RANGE ("VPLS Identifier", vid, argv[1], VPLS_ID_MIN,
                         VPLS_ID_MAX);

  return nsm_vpls_create_cli (cli, argv[0], vid);
}

CLI (mpls_vpls_by_name,
     mpls_vpls_by_name_cmd,
     "mpls vpls NAME",
     CONFIG_MPLS_STR,
     "Create an instance of MPLS based Virtual Private Lan Service (VPLS)",
     "Identifying string for VPLS")
{
  struct nsm_master *nm = cli->vr->proto;
  struct nsm_vpls *vpls;

  /* Lookup VPLS. */
  vpls = nsm_vpls_lookup_by_name (nm, argv[0]);
  if (! vpls)
    {
      cli_out (cli, "%% No VPLS configured with name: %s.\n", argv[0]);
      return CLI_ERROR;
    }

  cli->index = vpls;
  cli->mode = NSM_VPLS_MODE;

  return CLI_SUCCESS;
}

CLI (no_mpls_vpls,
     no_mpls_vpls_cmd,
     "no mpls vpls NAME <1-4294967295>",
     CLI_NO_STR,
     NO_CONFIG_MPLS_STR,
     "Remove an instance of MPLS based Virtual Private Lan Service (VPLS)",
     "Identifying string for VPLS",
     "Identifying value for VPLS")
{
  return nsm_vpls_delete_cli (cli, argv[0], argv[1]);
}

CLI (no_mpls_vpls_name,
     no_mpls_vpls_name_cmd,
     "no mpls vpls NAME",
     CLI_NO_STR,
     NO_CONFIG_MPLS_STR,
     "Remove an instance of MPLS based Virtual Private Lan Service (VPLS)",
     "Identifying string for VPLS")

{
  return nsm_vpls_delete_cli (cli, argv[0], NULL);
}

CLI (mpls_vpls_mtu,
     mpls_vpls_mtu_cmd,
     "vpls-mtu <576-65535>",
     "Set VPLS configured MTU",
     "Allowed MTU range")
{
  struct nsm_vpls *vpls = cli->index;
  u_int32_t mtu;

  CLI_GET_UINT32_RANGE ("MTU size", mtu, argv[0], 576, 65535);
  nsm_vpls_mtu_set (vpls, (u_int16_t)mtu);
  return CLI_SUCCESS;
}

CLI (mpls_vpls_no_mtu,
     mpls_vpls_no_mtu_cmd,
     "no vpls-mtu (<576-65535>|)",
     CLI_NO_STR,
     "Unset VPLS configured MTU",
     "Allowed MTU range")
{
  struct nsm_vpls *vpls = cli->index;
  nsm_vpls_mtu_unset (vpls);
  return CLI_SUCCESS;
}

CLI (mpls_vpls_group,
     mpls_vpls_group_cmd,
     "vpls-ac-group GROUPNAME",
     "assign ac-group to VPLS",
     "name of pre-configured ac-group")
{
  struct nsm_vpls *vpls = cli->index;
  struct nsm_master *nm = cli->vr->proto;
  struct nsm_mpls_vc_group* group = NULL;

  group = nsm_mpls_l2_vc_group_lookup_by_name( nm, argv[0]);

  if ( !group )
    {
      cli_out ( cli, "%%no such group: %s\n", argv[0] );
      return CLI_ERROR;
    }

  nsm_vpls_group_set (vpls, group->id );
  nsm_vpls_group_register (nm, group, vpls );
  return CLI_SUCCESS;
}

CLI (mpls_vpls_no_group,
     mpls_vpls_no_group_cmd,
     "no vpls-ac-group",
     CLI_NO_STR,
     "unassign ac-group from VPLS")
{
  struct nsm_vpls *vpls = cli->index;
  struct nsm_master *nm = cli->vr->proto;
  nsm_vpls_group_unregister(nm, vpls );
  nsm_vpls_group_unset (vpls);
  return CLI_SUCCESS;
}

CLI (mpls_vpls_type,
     mpls_vpls_type_cmd,
     "vpls-type (ethernet|vlan)",
     "Set allowed type for VPLS Mesh Virtual Circuits",
     "Ethernet",
     "VLAN")
{
  struct nsm_vpls *vpls;
  u_int16_t type;
  int ret;

  if (argv[0][0] == 'e')
    type = VC_TYPE_ETHERNET;
  else
    type = VC_TYPE_ETH_VLAN;

  vpls = cli->index;
  ret = nsm_vpls_type_set (vpls, type);
  if (ret < 0)
    {
      switch (ret)
        {
        case NSM_ERR_VPLS_PEER_EXISTS:
          cli_out (cli, "%% Operation not allowed. VPLS:%s already has "
                   "peer(s) configured.\n", vpls->vpls_name);
          break;
        default:
          cli_out (cli, "%% Failed to set type for VPLS: %s\n",
                   vpls->vpls_name);
          break;
        }

      return CLI_ERROR;
    }

  return CLI_SUCCESS;
}

CLI (no_mpls_vpls_type,
     no_mpls_vpls_type_cmd,
     "no vpls-type ((ethernet|vlan)|)",
     CLI_NO_STR,
     "Reset allowed type for VPLS Mesh Virtual Circuits to default (VLAN)",
     "Ethernet",
     "VLAN")
{
  struct nsm_vpls *vpls;
  int ret;

  vpls = cli->index;

  ret = nsm_vpls_type_unset (vpls);
  if (ret < 0)
    {
      switch (ret)
        {
        case NSM_ERR_VPLS_PEER_EXISTS:
          cli_out (cli, "%% Operation not allowed. VPLS:%s already has "
                   "peer(s) configured.\n", vpls->vpls_name);
          break;
        default:
          cli_out (cli, "%% Failed to unset type for VPLS: %s\n",
                   vpls->vpls_name);
          break;
        }

      return CLI_ERROR;
    }

  return CLI_SUCCESS;
}

CLI (mpls_vpls_desc,
     mpls_vpls_desc_cmd,
     "vpls-description LINE",
     "Specify a description for this VPLS instance",
     "Characters describing this VPLS instance")
{
  struct nsm_vpls *vpls = cli->index;
  nsm_vpls_desc_set (vpls, argv[0]);
  return CLI_SUCCESS;
}

CLI (no_mpls_vpls_desc,
     no_mpls_vpls_desc_cmd,
     "no vpls-description (LINE|)",
     CLI_NO_STR,
     "Unset VPLS description",
     "Characters describing this VPLS instance")
{
  struct nsm_vpls *vpls = cli->index;
  nsm_vpls_desc_unset (vpls);
  return CLI_SUCCESS;
}

#if 0
CLI (mpls_vpls_peer,
     mpls_vpls_peer_cmd,
     "vpls-peer A.B.C.D (secondary A.B.C.D|)",
     "Add a peer to VPLS domain",
     "IP address of the peer node to be added",
     "Secondary Peer fo this VPLS Node",
     "IP address of the secondary peer")
{
  struct nsm_vpls *vpls;
  vpls = cli->index;

  if (argc > 1)
    return nsm_vpls_mesh_peer_ipv4_add_cli (cli, vpls, argv[0], argv[1], argv[2]);
  else
    return nsm_vpls_mesh_peer_ipv4_add_cli (cli, vpls, argv[0], NULL, NULL);
}
#endif

/* CLI for adding VPLS peer. */
CLI (mpls_vpls_peer,
     mpls_vpls_peer_cmd,
     "vpls-peer A.B.C.D ((tunnel-id <1-65535> (forward|reverse|))|) (manual|)",
     "Add a peer to VPLS domain",
     "IP address of the peer node to be added",
     "Tunnel-Identifier",
     "Identifying value for Tunnel-id",
     "Tunnel Direction - Forward tunnel identifier - Default Value",
     "Tunnel Direction - Reverse tunnel identifier",
     "No signaling is used to set-up the Virtual Circuit")
{
  struct nsm_vpls *vpls = NULL;
  u_int32_t tunnel_value = 0;
  u_int8_t fec_type_vc;
  u_char mapping_type = MPLS_VC_MAPPING_NONE;
  u_char tunnel_dir = TUNNEL_DIR_FWD;
  int ret = 0;
  int i = 0;

#ifdef HAVE_FEC129
  fec_type_vc = PW_OWNER_GEN_FEC_SIGNALING;
#else
  fec_type_vc = PW_OWNER_PWID_FEC_SIGNALING;
#endif /* HAVE_FEC129 */

  if ((cli == NULL) || (cli->vr == NULL))
    return CLI_ERROR;

  vpls = cli->index;

  /* Identify Tunnel info and vc type. */
  for (i = 1; argc > i ; i++)
    {
      if (pal_strncmp (argv[i], "t", 1) == 0)
        {
          mapping_type = MPLS_VC_MAPPING_TUNNEL_ID;

          i++; /* Move to the next argument and get tunnel_id */
          CLI_GET_INTEGER_RANGE ("Tunnel-ID", tunnel_value, argv[i],
              MPLS_TUNNEL_ID_MIN, MPLS_TUNNEL_ID_MAX);

          /* There are more arguments */
          if (argc > (i+1))
            {
              if (pal_strncmp (argv[i+1], "r", 1) == 0)
                {
                  tunnel_dir = TUNNEL_DIR_REV;
                  i++;
                }
              else if (pal_strncmp (argv[i+1], "f", 1) == 0)
                {
                  /* tunnel_dir = TUNNEL_DIR_FWD; */
                  i++;
                }
            }
        }
      else if (pal_strncmp (argv[i], "m", 1) == 0)
        fec_type_vc = PW_OWNER_MANUAL;
    }

  ret = nsm_vpls_mesh_peer_ipv4_add_cli (cli->vr->id, vpls->vpls_id, argv[0],
                                         mapping_type, argv[2], tunnel_dir,
                                         fec_type_vc, NULL, NULL);
  return nsm_cli_return (cli, ret);
}

/* CLI for deleting VPLS peer. */
CLI (no_mpls_vpls_peer,
     no_mpls_vpls_peer_tunnel_cmd,
     "no vpls-peer A.B.C.D ((tunnel-id <1-65535> (forward|reverse|))|) (manual|)",
     CLI_NO_STR,
     "Remove a peer from VPLS domain",
     "IP address of the peer node to be removed",
     "Tunnel-Identifier",
     "Identifying value for Tunnel-id",
     "Tunnel Direction - Forward tunnel identifier - Default Value",
     "Tunnel Direction - Reverse tunnel identifier",
     "No signaling is used to set-up the Virtual Circuit")
{
  struct nsm_vpls *vpls = NULL;
  u_int32_t tunnel_value = 0;
  u_int8_t fec_type_vc;
  u_char mapping_type = MPLS_VC_MAPPING_NONE;
  u_char cli_params = CLI_MIN_PARAMS_ENTRY_DELETE;
  u_char tunnel_dir = TUNNEL_DIR_NONE;
  int ret = 0;
  int i = 0;

  if ((cli == NULL) || (cli->vr == NULL))
    return CLI_ERROR;

  vpls = cli->index;

#ifdef HAVE_FEC129
  fec_type_vc = PW_OWNER_GEN_FEC_SIGNALING;
#else
  fec_type_vc = PW_OWNER_PWID_FEC_SIGNALING;
#endif /* HAVE_FEC129 */

  /* Identify Tunnel info and vc type. */
  for (i = 1; argc > i ; i++)
    {
      if (pal_strncmp (argv[i], "t", 1) == 0)
        {
          mapping_type = MPLS_VC_MAPPING_TUNNEL_ID;

          i++; /* Move to the next argument and get tunnel_id */
          CLI_GET_INTEGER_RANGE ("Tunnel-ID", tunnel_value, argv[i],
              MPLS_TUNNEL_ID_MIN, MPLS_TUNNEL_ID_MAX);

          /* There are more arguments */
          if (argc > (i+1))
            {
              if (pal_strncmp (argv[i+1], "r", 1) == 0)
                {
                  tunnel_dir = TUNNEL_DIR_REV;
                  i++;
                }
              else if (pal_strncmp (argv[i+1], "f", 1) == 0)
                {
                  tunnel_dir = TUNNEL_DIR_FWD;
                  i++;
                }
            }
        }
      else if (pal_strncmp (argv[i], "m", 1) == 0)
        fec_type_vc = PW_OWNER_MANUAL;
    }

  ret = nsm_vpls_mesh_peer_ipv4_delete_cli (cli->vr->id, vpls->vpls_id,
                                            argv[0], mapping_type, argv[2],
                                            tunnel_dir, fec_type_vc, cli_params);
  return nsm_cli_return (cli, ret);
}

#if 0
CLI (mpls_vpls_vc,
     mpls_vpls_vc_cmd,
     "vpls-vc NAME (secondary NAME|) (ethernet|vlan|)",
     "Add a spoke virtual circuit to VPLS domain",
     "Name of mpls virtual circuit to be added",
     "Secondary spoke configuration",
     "Secondary spoke name",
     "Spoke type : Ethernet (default)",
     "Spoke type : Ethernet VLAN")
{
  struct nsm_vpls *vpls;

  vpls = cli->index;

  switch (argc)
    {
      case 1:
        return nsm_vpls_vc_add_cli (cli,vpls,argv[0],NULL,NULL,NULL);
        break;

      case 2:
        return nsm_vpls_vc_add_cli (cli,vpls,argv[0],argv[1],NULL,NULL);
        break;

      case 3:
        return nsm_vpls_vc_add_cli (cli,vpls,argv[0],NULL,argv[1],argv[2]);
        break;

      case 4:
        return nsm_vpls_vc_add_cli (cli,vpls,argv[0],argv[3],argv[1],argv[2]);
        break;
    }
  return CLI_SUCCESS;
}
#endif

CLI (mpls_vpls_vc,
     mpls_vpls_vc_cmd,
     "vpls-vc NAME (ethernet|vlan|)",
     "Add a spoke virtual circuit to VPLS domain",
     "Name of mpls virtual circuit to be added",
     "Spoke type : Ethernet (default)",
     "Spoke type : Ethernet VLAN")
{
  struct nsm_vpls *vpls;

  vpls = cli->index;

  switch (argc)
    {
      case 1:
        return nsm_vpls_vc_add_cli (cli,vpls,argv[0],NULL,NULL,NULL);
        break;

      case 2:
        return nsm_vpls_vc_add_cli (cli,vpls,argv[0],argv[1],NULL,NULL);
        break;

      case 3:
        return nsm_vpls_vc_add_cli (cli,vpls,argv[0],NULL,argv[1],argv[2]);
        break;

      case 4:
        return nsm_vpls_vc_add_cli (cli,vpls,argv[0],argv[3],argv[1],argv[2]);
        break;
    }
  return CLI_SUCCESS;
}

CLI (no_mpls_vpls_vc,
     no_mpls_vpls_vc_cmd,
     "no vpls-vc NAME",
     CLI_NO_STR,
     "Remove a spoke virtual circuit from VPLS domain",
     "Name of mpls virtual circuit to be removed")
{
  struct nsm_vpls *vpls;

  vpls = cli->index;

  /* Remove spoke vc from vpls domain */
  return nsm_vpls_vc_delete_cli (cli, vpls, argv[0]);
}

int nsm_vpls_mac_withdraw_send (struct nsm_master *, u_int32_t,
                                u_char *, u_int32_t);
int
nsm_vpls_flush_mac_cli (struct cli *cli, struct nsm_vpls *vpls)
{
  int ret;

  ret = nsm_vpls_mac_withdraw_send (cli->vr->proto, vpls->vpls_id, NULL, 0);
  if (ret < 0)
    {
      cli_out (cli, "%% Failed to send withdraw message for "
               "MAC Addresses for VPLS instance:%s.\n",
               vpls->vpls_name);
      return CLI_ERROR;
    }

  return CLI_SUCCESS;
}

CLI (clear_mpls_vpls_mac,
     clear_mpls_vpls_mac_cmd,
     "clear mpls vpls NAME mac-addresses",
     CLI_CLEAR_STR,
     CLEAR_MPLS_STR,
     "Clear VPLS data",
     "Clear data for VPLS instance with specified name",
     "Flush all MAC addresses learnt for VPLS instance")
{
  struct nsm_master *nm = cli->vr->proto;
  struct nsm_vpls *vpls;

  if (! NSM_MPLS || ! NSM_MPLS->vpls_table)
    return CLI_SUCCESS;

  vpls = nsm_vpls_lookup_by_name (nm, argv[0]);
  if (vpls)
    return nsm_vpls_flush_mac_cli (cli, vpls);

  cli_out (cli, "%% No matching VPLS instance found.\n");
  return CLI_ERROR;
}
#endif /* HAVE_VPLS */

CLI (mpls_enable_all_ifs,
     mpls_enable_all_ifs_cmd,
     "mpls enable-all-interfaces",
     CONFIG_MPLS_STR,
     "Enable all interfaces for MPLS")
{
  nsm_mpls_api_enable_all_interfaces (cli->vr->proto, 0);
  return CLI_SUCCESS;
}

CLI (mpls_disable_all_ifs,
     mpls_disable_all_ifs_cmd,
     "mpls disable-all-interfaces",
     CONFIG_MPLS_STR,
     "Disable all interfaces for MPLS")
{
  nsm_mpls_api_disable_all_interfaces (cli->vr->proto);
  return CLI_SUCCESS;
}

/* Set the maximum label pool size for all interfaces. */
CLI (max_label_pool_size,
     max_label_pool_size_cmd,
     "mpls (rsvp|ldp|bgp|) max-label-value <16-1048575> "
     "(label-space <0-60000>|)",
     CONFIG_MPLS_STR,
     "Specify label range value for rsvp",
     "Specify label range value for ldp",
     "Specify label range value for bgp",
     "Specify a maximum label value",
     "Maximum size to be used for label pools",
     "Label-space for which maximum label value needs to be updated",
     "Label-space value")
{
  struct nsm_master *nm = cli->vr->proto;
  int label_space;
  u_int32_t val;
  int ret = NSM_SUCCESS;

  if (argc == 4)
  {
    CLI_GET_INTEGER_RANGE ("label-space", label_space, argv[3], 0,
                           MAXIMUM_LABEL_SPACE);
    ret = nsm_mpls_api_max_lbl_range_set (cli, cli->vr->id, argv[0], argv[1],
                                            label_space);
    if (ret != NSM_SUCCESS)
      {
        return CLI_ERROR;
      }
    return CLI_SUCCESS;
  }
  else if (argc == 2)
  {
    if (pal_char_isalpha (argv[0][0]))
    {
      ret = nsm_mpls_api_max_lbl_range_set (cli, cli->vr->id, argv[0],
                                        argv[1], PLATFORM_WIDE_LABEL_SPACE);
      if (ret != NSM_SUCCESS)
        {
          return CLI_ERROR;
        }
      return CLI_SUCCESS;
    }
  }

  pal_sscanf (argv[0], "%ux", &val);

  if (val > LABEL_VALUE_MAX || val < LABEL_VALUE_INITIAL ||
      val < NSM_MPLS->min_label_val)
    {
      cli_out (cli, "Invalid maximum label space\n");
      return CLI_ERROR;
    }

  if (argc > 1)
    {
      CLI_GET_INTEGER_RANGE ("label-space", label_space, argv[2], 0,
                             MAXIMUM_LABEL_SPACE);
      ret = nsm_mpls_api_ls_max_label_val_set (nm, label_space, val);
    }
  else
    ret = nsm_mpls_api_max_label_val_set (nm, val);

  if (ret == NSM_ERR_LS_IN_USE)
    {
      cli_out (cli, "%% Operation not allowed. Label space in use.\n");
      return CLI_ERROR;
    }
  else if (ret == NSM_ERR_LS_INVALID_MAX_LABEL)
    {
      cli_out (cli, "%% Invalid maximum label value. .\n");
      return CLI_ERROR;
    }

  return CLI_SUCCESS;
}

/* Set the minimum label pool size for all interfaces. */
CLI (min_label_pool_size,
     min_label_pool_size_cmd,
     "mpls (rsvp|ldp|bgp|) min-label-value <16-1048575> "
     "(label-space <0-60000>|)",
     CONFIG_MPLS_STR,
     "Specify label range value for rsvp",
     "Specify label range value for ldp",
     "Specify label range value for bgp",
     "Specify a minimum label value",
     "Minimum size to be used for label pools or protocol range",
     "Label-space for which minimum label value needs to be modified",
     "Label-space value")
{
  struct nsm_master *nm = cli->vr->proto;
  int label_space = PLATFORM_WIDE_LABEL_SPACE;
  u_int32_t val;
  int ret = NSM_SUCCESS;

  if (argc == 4)
  {
    CLI_GET_INTEGER_RANGE ("label-space", label_space, argv[3], 0,
                           MAXIMUM_LABEL_SPACE);
    ret = nsm_mpls_api_min_lbl_range_set (cli, cli->vr->id, argv[0], argv[1],
                                            label_space);
    if (ret != NSM_SUCCESS)
      {
        return CLI_ERROR;
      }
    return CLI_SUCCESS;
  }
  else if (argc == 2)
  {
    if (pal_char_isalpha (argv[0][0]))
    {
      ret = nsm_mpls_api_min_lbl_range_set (cli, cli->vr->id, argv[0],
                                         argv[1], label_space);
      if (ret != NSM_SUCCESS)
        {
          return CLI_ERROR;
        }
      return CLI_SUCCESS;
    }
  }

  pal_sscanf (argv[0], "%ux", &val);

  if (val > LABEL_VALUE_MAX || val < LABEL_VALUE_INITIAL ||
      val > NSM_MPLS->max_label_val)
    {
      cli_out (cli, "Invalid minimum label value\n");
      return CLI_ERROR;
    }

  if (argc > 1)
    {
      CLI_GET_INTEGER_RANGE ("label-space", label_space, argv[2], 0,
                             MAXIMUM_LABEL_SPACE);
      ret = nsm_mpls_api_ls_min_label_val_set (nm, label_space, val);
    }
  else
    ret = nsm_mpls_api_min_label_val_set (nm, val);

  if (ret == NSM_ERR_LS_IN_USE)
    {
      cli_out (cli, "Operation not allowed. Label space in use.\n");
      return CLI_ERROR;
    }
  else if (ret == NSM_ERR_LS_INVALID_MIN_LABEL)
    {
      cli_out (cli, "Invalid minimum label value. .\n");
      return CLI_ERROR;
    }

  return CLI_SUCCESS;
}

/* Unset the maximum label pool size for all interfaces. */
CLI (no_max_label_pool_size,
     no_max_label_pool_size_cmd,
     "no mpls (rsvp|ldp|bgp|) max-label-value "
     "(<16-1048575>|) (label-space <0-60000>|)",
     CLI_NO_STR,
     NO_CONFIG_MPLS_STR,
     "Reset label range value for rsvp",
     "Reset label range value for ldp",
     "Reset label range value for bgp",
     "Use default maximum size for label pools",
     "Maximum label value",
     "Label-space for which maximum label value needs to be modified",
     "Label-space value")
{
  struct nsm_master *nm = cli->vr->proto;
  int label_space = 0;
  bool_t ls = NSM_FALSE;
  int ret, i;

  if (argc == 4)
  {
    CLI_GET_INTEGER_RANGE ("label-space", label_space, argv[3], 0,
                           MAXIMUM_LABEL_SPACE);
    /* Sending only the protocol and label space; ignore the label value*/
    ret = nsm_mpls_api_max_lbl_range_unset (cli, cli->vr->id, argv[0],
                                              label_space);
    if (ret != NSM_SUCCESS)
      {
        return CLI_ERROR;
      }
    return CLI_SUCCESS;

  }
  else if ((argc == 2) || (argc == 1))
  {
    if (pal_char_isalpha (argv[0][0]))
    {
      ret = nsm_mpls_api_max_lbl_range_unset (cli, cli->vr->id, argv[0],
                                               PLATFORM_WIDE_LABEL_SPACE);

      if (ret != NSM_SUCCESS)
        {
          return CLI_ERROR;
        }
      return CLI_SUCCESS;
    }
  }

  for (i = 0; i < argc; i++)
    {
      if (pal_strncmp (argv[i], "label-space", 11) == 0)
        {
          CLI_GET_INTEGER_RANGE ("label-space", label_space, argv[i+1], 0,
                                 MAXIMUM_LABEL_SPACE);
          ls = NSM_TRUE;
        }
    }

  if (ls)
    ret =  nsm_mpls_api_ls_max_label_val_unset (nm, label_space);
  else
    ret = nsm_mpls_api_max_label_val_unset (nm);

  if (ret == NSM_ERR_LS_IN_USE)
    {
      cli_out (cli, "Operation not allowed. Label space in use.\n");
      return CLI_ERROR;
    }

  return CLI_SUCCESS;
}

/* Unset the minimum label pool size for all interfaces. */
CLI (no_min_label_pool_size,
     no_min_label_pool_size_cmd,
     "no mpls (rsvp|ldp|bgp|) min-label-value (<16-1048575>|) "
     "(label-space <0-60000>|)",
     CLI_NO_STR,
     NO_CONFIG_MPLS_STR,
     "Reset label range value for rsvp",
     "Reset label range value for ldp",
     "Reset label range value for bgp",
     "Minimum label value",
     "Use default minimum size for label pools",
     "Label-space for which minimum label value needs to be modified",
     "Label-space value")
{
  struct nsm_master *nm = cli->vr->proto;
  int lbl_space = 0;
  bool_t ls = NSM_FALSE;
  int ret, i;

  if (argc == 4)
  {
    CLI_GET_INTEGER_RANGE ("label-space", lbl_space, argv[3], 0,
                           MAXIMUM_LABEL_SPACE);
    /* Sending only the protocol and label space; ignore the label value*/
    return (nsm_mpls_api_min_lbl_range_unset (cli, cli->vr->id, argv[0],
                                              lbl_space));
  }
  else if ((argc == 2) || (argc == 1))
  {
    if (pal_char_isalpha (argv[0][0]))
    {
      return (nsm_mpls_api_min_lbl_range_unset (cli, cli->vr->id, argv[0],
                                               PLATFORM_WIDE_LABEL_SPACE));
    }
  }

  for (i = 0; i < argc; i++)
    {
      if (pal_strncmp (argv[i], "label-space", 11) == 0)
        {
          CLI_GET_INTEGER_RANGE ("label-space", lbl_space, argv[i+1], 0,
                                 MAXIMUM_LABEL_SPACE);
          ls = NSM_TRUE;
        }
    }

  if (ls)
    ret = nsm_mpls_api_ls_min_label_val_unset (nm, lbl_space);
  else
    ret = nsm_mpls_api_min_label_val_unset (nm);

  if (ret == NSM_ERR_LS_IN_USE)
    {
      cli_out (cli, "Operation not allowed. Label space in use.\n");
      return CLI_ERROR;
    }

  return CLI_SUCCESS;
}

#if (!defined (HAVE_GMPLS) || defined (HAVE_PACKET))

CLI (ingress_ttl,
     ingress_ttl_cmd,
     "mpls ingress-ttl <0-255>",
     CONFIG_MPLS_STR,
     "Specify a TTL value for LSPs for which this LSR is the ingress",
     "TTL value to be used")
{
  int ttl_val;

  pal_sscanf (argv[0], "%d", &ttl_val);
  if ((ttl_val > 255) || (ttl_val < 0))
    {
      cli_out (cli, "%% Invalid TTL value specified\n");
      return CLI_ERROR;
    }

  nsm_mpls_api_ingress_ttl_set (cli->vr->proto, ttl_val);
  return CLI_SUCCESS;
}

CLI (egress_ttl,
     egress_ttl_cmd,
     "mpls egress-ttl <0-255>",
     CONFIG_MPLS_STR,
     "Specify a TTL value for LSPs for which this LSR is the egress",
     "TTL value to be used")
{
  int ttl_val;

  pal_sscanf (argv[0], "%d", &ttl_val);
  if ((ttl_val > 255) || (ttl_val < 0))
    {
      cli_out (cli, "%% Invalid TTL value specified\n");
      return CLI_ERROR;
    }

  nsm_mpls_api_egress_ttl_set (cli->vr->proto, ttl_val);
  return CLI_SUCCESS;
}

CLI (no_ingress_ttl,
     no_ingress_ttl_cmd,
     "no mpls ingress-ttl",
     CLI_NO_STR,
     NO_CONFIG_MPLS_STR,
     "Unset the custom TTL value being used for LSPs for which this "
     "LSR is the ingress")
{
  nsm_mpls_api_ingress_ttl_set (cli->vr->proto, NSM_MPLS_NO_TTL);
  return CLI_ERROR;
}

ALI (no_ingress_ttl,
     no_ingress_ttl_val_cmd,
     "no mpls ingress-ttl <0-255>",
     CLI_NO_STR,
     NO_CONFIG_MPLS_STR,
     "Unset the custom TTL value being used for LSPs for which this "
     "LSR is the ingress",
     "TTL value used");

CLI (no_egress_ttl,
     no_egress_ttl_cmd,
     "no mpls egress-ttl",
     CLI_NO_STR,
     NO_CONFIG_MPLS_STR,
     "Unset the custom TTL value being used for LSPs for which this "
     "LSR is the egress")
{
  nsm_mpls_api_egress_ttl_set (cli->vr->proto, NSM_MPLS_NO_TTL);
  return CLI_ERROR;
}

ALI (no_egress_ttl,
     no_egress_ttl_val_cmd,
     "no mpls egress-ttl <0-255>",
     CLI_NO_STR,
     NO_CONFIG_MPLS_STR,
     "Unset the custom TTL value being used for LSPs for which this "
     "LSR is the egress",
     "TTL value used");
#endif /* !HAVE_GMPLS || HAVE_PACKET */

CLI (local_packet_handle,
     local_packet_handle_cmd,
     "mpls local-packet-handling",
     CONFIG_MPLS_STR,
     "Enable labeling of locally generated TCP packets")
{
  nsm_mpls_api_local_pkt_handling (cli->vr->proto, NSM_TRUE);
  return CLI_SUCCESS;
}

CLI (no_local_packet_handle,
     no_local_packet_handle_cmd,
     "no mpls local-packet-handling",
     CLI_NO_STR,
     NO_CONFIG_MPLS_STR,
     "Disable labeling of locally generated TCP packets")
{
  nsm_mpls_api_local_pkt_handling (cli->vr->proto, NSM_FALSE);
  return CLI_SUCCESS;
}

CLI (log_error_message,
     log_error_message_cmd,
     "mpls log error",
     CONFIG_MPLS_STR,
     "Enable logging in MPLS Forwarder",
     "Log Error message in MPLS Forwarder")
{
  nsm_mpls_log (cli->vr->proto, NSM_MPLS_SHOW_MSG_ERROR);
  return CLI_SUCCESS;
}

CLI (show_mpls_log,
     show_mpls_log_cmd,
     "show mpls log",
     CLI_SHOW_STR,
     SHOW_MPLS_STR,
     "Show MPLS logging information")
{
  struct nsm_master *nm = cli->vr->proto;

  if ((! NSM_MPLS) || (! NSM_MPLS->iflist))
    return CLI_SUCCESS;

  nsm_mpls_dump_log (cli);
  return CLI_SUCCESS;
}

CLI (no_log_error_message,
     no_log_error_message_cmd,
     "no mpls log error",
     CLI_NO_STR,
     NO_CONFIG_MPLS_STR,
     "Disable logging in MPLS Forwarder",
     "Stop logging Error message in MPLS Forwarder")
{
  nsm_mpls_no_log (cli->vr->proto, NSM_MPLS_SHOW_MSG_ERROR);
  return CLI_SUCCESS;
}

CLI (log_warning_message,
     log_warning_message_cmd,
     "mpls log warning",
     CONFIG_MPLS_STR,
     "Enable logging in MPLS Forwarder",
     "Log Warning message in MPLS Forwarder")
{
  nsm_mpls_log (cli->vr->proto, NSM_MPLS_SHOW_MSG_WARNING);
  return CLI_SUCCESS;
}

CLI (no_log_warning_message,
     no_log_warning_message_cmd,
     "no mpls log warning",
     CLI_NO_STR,
     NO_CONFIG_MPLS_STR,
     "Disable logging in MPLS Forwarder",
     "Stop logging Warning message in MPLS Forwarder")
{
  nsm_mpls_no_log (cli->vr->proto, NSM_MPLS_SHOW_MSG_WARNING);
  return CLI_SUCCESS;
}

CLI (log_all_message,
     log_all_message_cmd,
     "mpls log all",
     CONFIG_MPLS_STR,
     "Enable logging in MPLS Forwarder",
     "Log all messages in MPLS Forwarder")
{
  nsm_mpls_log (cli->vr->proto, NSM_MPLS_SHOW_MSG_ALL);
  return CLI_SUCCESS;
}

CLI (no_log_all_message,
     no_log_all_message_cmd,
     "no mpls log all",
     CLI_NO_STR,
     NO_CONFIG_MPLS_STR,
     "Disable logging in MPLS Forwarder",
     "Stop logging all messages in MPLS Forwarder")
{
  nsm_mpls_no_log (cli->vr->proto, NSM_MPLS_SHOW_MSG_ALL);
  return CLI_SUCCESS;
}

CLI (log_debug_message,
     log_debug_message_cmd,
     "mpls log debug",
     CONFIG_MPLS_STR,
     "Enable logging in MPLS Forwarder",
     "Log Debug messages in MPLS Forwarder")
{
  nsm_mpls_log (cli->vr->proto, NSM_MPLS_SHOW_MSG_DEBUG);
  return CLI_SUCCESS;
}

CLI (no_log_debug_message,
     no_log_debug_message_cmd,
     "no mpls log debug",
     CLI_NO_STR,
     NO_CONFIG_MPLS_STR,
     "Disable logging in MPLS Forwarder",
     "Stop logging Debug messages in MPLS Forwarder")
{
  nsm_mpls_no_log (cli->vr->proto, NSM_MPLS_SHOW_MSG_DEBUG);
  return CLI_SUCCESS;
}

CLI (log_notice_message,
     log_notice_message_cmd,
     "mpls log notice",
     CONFIG_MPLS_STR,
     "Enable logging in MPLS Forwarder",
     "Log Notice messages in MPLS Forwarder")
{
  nsm_mpls_log (cli->vr->proto, NSM_MPLS_SHOW_MSG_NOTICE);
  return CLI_SUCCESS;
}

CLI (no_log_notice_message,
     no_log_notice_message_cmd,
     "no mpls log notice",
     CLI_NO_STR,
     NO_CONFIG_MPLS_STR,
     "Disable logging in MPLS Forwarder",
     "Stop logging Notice messages in MPLS Forwarder")
{
  nsm_mpls_no_log (cli->vr->proto, NSM_MPLS_SHOW_MSG_NOTICE);
  return CLI_SUCCESS;
}

#ifdef HAVE_TE
/* Show the list of existing administrative groups */
CLI (show_admin_grp,
     show_admin_grp_cmd,
     "show mpls admin-groups",
     CLI_SHOW_STR,
     SHOW_MPLS_STR,
     "Show all Administrative Group configured")
{
  struct nsm_master *nm = cli->vr->proto;

  if ((! NSM_MPLS) || (! NSM_MPLS->iflist))
    return CLI_SUCCESS;

  admin_group_dump (cli, NSM_MPLS->admin_group_array);
  return CLI_SUCCESS;
}

/* Add a new admin group. */
CLI (admin_grp,
     admin_grp_cmd,
     "mpls admin-group NAME <0-31>",
     CONFIG_MPLS_STR,
     "Add a new Administrative Group",
     "Name of administrative group to be added",
     "Value of administrative group to be added")
{
  int val;
  int ret;

  /* Get admin group value. */
  CLI_GET_INTEGER_RANGE ("admin-group value", val, argv[1], 0,
                         ADMIN_GROUP_MAX - 1);

  ret = nsm_mpls_api_admin_group_add (cli->vr->proto, argv[0], val);
  if (ret < 0)
    {
      switch (ret)
        {
        case NSM_ERR_NAME_TOO_LONG:
          cli_out (cli, "%% Administrative group name length cannot "
                   "exceed %d\n", ADMIN_GROUP_NAMSIZ);
          break;
        case ADMIN_GROUP_ERR_DUPLICATE:
          cli_out (cli, "%% Administrative group name or value already "
                   "exists\n");
          break;
        case ADMIN_GROUP_ERR_EEXIST:
          cli_out (cli, "%% Administrative group value specified is "
                   "already in use\n");
          break;
        }
      return CLI_ERROR;
    }

  return CLI_SUCCESS;
}

/* Delete an existing administrative group. */
CLI (no_admin_grp,
     no_admin_grp_cmd,
     "no mpls admin-group NAME <0-31>",
     CLI_NO_STR,
     NO_CONFIG_MPLS_STR,
     "Delete an existing Administrative Group",
     "Name of administrative group to be deleted",
     "Value of administrative group to be deleted")
{
  int val;
  int ret;

  /* Get admin group value. */
  CLI_GET_INTEGER_RANGE ("admin-group value", val, argv[1], 0,
                         ADMIN_GROUP_MAX - 1);

  ret = nsm_mpls_api_admin_group_del (cli->vr->proto, argv[0], val);
  if (ret < 0)
    {
      switch (ret)
        {
        case NSM_ERR_NAME_TOO_LONG:
          cli_out (cli, "%% Administrative group name length cannot "
                   "exceed %d\n", ADMIN_GROUP_NAMSIZ);
          break;
        case NSM_ERR_NAME_IN_USE:
          cli_out (cli, "%% Administrative group name being used in "
                   "interface. Please delete it first.\n");
          break;
        case ADMIN_GROUP_ERR_EEXIST:
          cli_out (cli, "%% Administrative group value specified does not "
                   "exist with value specified\n");
          break;
        }
      return CLI_ERROR;
    }

  return CLI_SUCCESS;
}
#endif /* HAVE_TE */

#ifdef HAVE_DEV_TEST
void
nsm_mpls_ix_mgr_node_dump (struct cli *cli, struct bitmap_block *node)
{
  struct bitmap *parent;
  u_int32_t ix;
  int i, j;

  if (! node)
    return;

  parent = node->parent;
  cli_out (cli, "%% Output for index manager node with id %d:\n",
           node->id);

  /* Used indices. */
  for (i = 0; i < parent->array_size; i++)
    {
      for (j = 0; j < INTEGER_BITS; j++)
        {
          if (! CHECK_FLAG (node->map[i], (1 << j)))
            {
              ix = (node->id * parent->block_size) + (i * INTEGER_BITS) + j;
              cli_out (cli, "   Index %d is not available\n", ix);
            }
        }
    }

  /* Available indices. */
  cli_out (cli, "\n");
  for (i = 0; i < parent->array_size; i++)
    {
      for (j = 0; j < INTEGER_BITS; j++)
        {
          if (CHECK_FLAG (node->map[i], (1 << j)))
            {
              ix = (node->id * parent->block_size) + (i * INTEGER_BITS) + j;
              cli_out (cli, "   Index %d is available\n", ix);
            }
        }
    }
}

void
nsm_mpls_ix_mgr_dump (struct cli *cli, struct bitmap *map, u_int32_t ix)
{
  struct bitmap_block *node;
  int i;

  if (! map)
    return;

  /* If ix is non-zero, figure out which node it belongs to. */
  if (ix)
    {
      node = bitmap_block_get (map, ix);
      if (! node)
        {
          cli_out (cli, "%% Bitmap block corresponding to index %d not "
                   "found\n", ix);
          return;
        }

      /* Dump to cli. */
      nsm_mpls_ix_mgr_node_dump (cli, node);

      /* If the node is completely unused, remove it, since it was
         was created due to get request. */
      if (node->total_free_indices == 0)
        bitmap_block_free (node);

      return;
    }

  for (i = 0; i < BITMAP_HASH_SIZE; i++)
    {
      for (node = map->hash[i]; node; node = node->next)
        {
          /* Dump to cli. */
          nsm_mpls_ix_mgr_node_dump (cli, node);

          /* If the node is completely unused, remove it, since it was
             was created due to get request. */
          if (node->total_free_indices == 0)
            bitmap_block_free (node);
        }
    }
}

/* Show index manager internals. */
CLI (show_ftn_ix_mgr,
     show_ftn_ix_mgr_cmd,
     "show mpls index-manager ftn",
     CLI_SHOW_STR,
     SHOW_MPLS_STR,
     "Show Index Manager details",
     "Show details for FTN Index Manager")
{
  struct nsm_master *nm = cli->vr->proto;

  nsm_mpls_ix_mgr_dump (cli, NSM_MPLS->ftn_ix_table4.ix_mgr, 0);
  return CLI_SUCCESS;
}

/* Show index manager internals. */
CLI (show_ftn_ix_mgr_ix,
     show_ftn_ix_mgr_ix_cmd,
     "show mpls index-manager ftn INDEX",
     CLI_SHOW_STR,
     SHOW_MPLS_STR,
     "Show Index Manager details",
     "Show details for FTN Index Manager",
     "Show details for bitmap that holds this index")
{
  struct nsm_master *nm = cli->vr->proto;
  u_int32_t ix;

  if (! NSM_MPLS ||
#ifdef HAVE_IPV6
     ! FTN_TABLE6 ||
#endif
     ! FTN_TABLE4)
    return CLI_SUCCESS;

  /* Get index. */
  CLI_GET_UINT32 ("Ftn Index", ix, argv[0]);

  /* The index manager is the same for IPv4 and IPv6 */
  nsm_mpls_ix_mgr_dump (cli, NSM_MPLS->ftn_ix_table4.ix_mgr, ix);

  return CLI_SUCCESS;
}

/* Show index manager internals. */
CLI (show_nhlfe_ix_mgr,
     show_nhlfe_ix_mgr_cmd,
     "show mpls index-manager out-segment",
     CLI_SHOW_STR,
     SHOW_MPLS_STR,
     "Show Index Manager details",
     "Show details for Out Segment Index Manager")
{
  struct nsm_master *nm = cli->vr->proto;
  nsm_mpls_ix_mgr_dump (cli, NSM_MPLS->nhlfe_ix_table4.ix_mgr, 0);
  return CLI_SUCCESS;
}

/* Show index manager internals. */
CLI (show_nhlfe_ix_mgr_ix,
     show_nhlfe_ix_mgr_ix_cmd,
     "show mpls index-manager out-segment INDEX",
     CLI_SHOW_STR,
     SHOW_MPLS_STR,
     "Show Index Manager details",
     "Show details for Out Segment Index Manager",
     "Show details for bitmap that holds this index")
{
  struct nsm_master *nm = cli->vr->proto;
  u_int32_t ix;

  if (! NSM_MPLS ||
#ifdef HAVE_IPV6
      ! NHLFE_TABLE6 ||
#endif
      ! NHLFE_TABLE4)
    return CLI_SUCCESS;

  /* Get index. */
  CLI_GET_UINT32 ("Out Segment Index", ix, argv[0]);
  nsm_mpls_ix_mgr_dump (cli, NSM_MPLS->nhlfe_ix_table4.ix_mgr, ix);
  return CLI_SUCCESS;
}

/* Show index manager internals. */
CLI (show_xc_ix_mgr,
     show_xc_ix_mgr_cmd,
     "show mpls index-manager cross-connect",
     CLI_SHOW_STR,
     SHOW_MPLS_STR,
     "Show Index Manager details",
     "Show details for Cross Connect Index Manager")
{
  struct nsm_master *nm = cli->vr->proto;
  nsm_mpls_ix_mgr_dump (cli, NSM_MPLS->xc_ix_table.ix_mgr, 0);
  return CLI_SUCCESS;
}

/* Show index manager internals. */
CLI (show_xc_ix_mgr_ix,
     show_xc_ix_mgr_ix_cmd,
     "show mpls index-manager cross-connect INDEX",
     CLI_SHOW_STR,
     SHOW_MPLS_STR,
     "Show Index Manager details",
     "Show details for Cross Connect Index Manager",
     "Show details for bitmap that holds this index")
{
  struct nsm_master *nm = cli->vr->proto;
  u_int32_t ix;

  if (! NSM_MPLS || ! XC_TABLE)
    return CLI_SUCCESS;

  /* Get index. */
  CLI_GET_UINT32 ("Cross Connect Index", ix, argv[0]);
  nsm_mpls_ix_mgr_dump (cli, NSM_MPLS->xc_ix_table.ix_mgr, ix);
  return CLI_SUCCESS;
}

/* Lookup next FTN without the index. */
CLI (mpls_next_ftn,
     mpls_next_ftn_cmd,
     "mpls next ftn",
     SHOW_MPLS_STR,
     "Get next value",
     "Get next FTN based on index")
{
  struct nsm_master *nm = cli->vr->proto;
  struct ftn_entry *ftn;
  struct prefix *p;

  /* Lookup. */
  ftn = nsm_gmpls_ftn_ix_lookup_next (nm, 0, &p);
  if (ftn)
    {
      cli_out (cli, "%% Next FTN index: %d\n", ftn->ftn_ix);
      return CLI_SUCCESS;
    }

  cli_out (cli, "%% Next FTN entry not found\n");
  return CLI_ERROR;
}

/* Lookup next FTN. */
CLI (mpls_next_ftn_ix,
     mpls_next_ftn_ix_cmd,
     "mpls next ftn INDEX",
     SHOW_MPLS_STR,
     "Get next value",
     "Get next FTN based on index",
     "Index value")
{
  struct nsm_master *nm = cli->vr->proto;
  struct ftn_entry *ftn;
  struct prefix *p;
  u_int32_t ix;

  /* Lookup. */
  CLI_GET_UINT32 ("FTN Index", ix, argv[0]);
  ftn = nsm_mpls_ftn_ix_lookup_next (nm, ix, &p);
  if (ftn)
    {
      cli_out (cli, "%% Next FTN index: %d\n", ftn->ftn_ix);
      return CLI_SUCCESS;
    }

  cli_out (cli, "%% Next FTN entry not found\n");
  return CLI_ERROR;
}

/* Lookup next NHLFE. */
CLI (mpls_next_nhlfe,
     mpls_next_nhlfe_cmd,
     "mpls next out-segment",
     SHOW_MPLS_STR,
     "Get next value",
     "Get next Out Segment based on index")
{
  struct nsm_master *nm = cli->vr->proto;
  struct nhlfe_entry *nhlfe;

  /* Lookup. */
  nhlfe = nsm_mpls_nhlfe_ix_lookup_next (nm, 0);
  if (nhlfe)
    {
      cli_out (cli, "%% Next Out-Segment index: %d\n", nhlfe->nhlfe_ix);
      return CLI_SUCCESS;
    }

  cli_out (cli, "%% Next Out-Segment entry not found\n");
  return CLI_ERROR;
}

/* Lookup next NHLFE with index. */
CLI (mpls_next_nhlfe_ix,
     mpls_next_nhlfe_ix_cmd,
     "mpls next out-segment INDEX",
     SHOW_MPLS_STR,
     "Get next value",
     "Get next Out Segment based on index",
     "Index value")
{
  struct nsm_master *nm = cli->vr->proto;
  struct nhlfe_entry *nhlfe;
  u_int32_t ix;

  /* Lookup. */
  CLI_GET_UINT32 ("Out segment Index", ix, argv[0]);
  nhlfe = nsm_mpls_nhlfe_ix_lookup_next (nm, ix);
  if (nhlfe)
    {
      cli_out (cli, "%% Next Out-Segment index: %d\n", nhlfe->nhlfe_ix);
      return CLI_SUCCESS;
    }

  cli_out (cli, "%% Next Out-Segment entry not found\n");
  return CLI_ERROR;
}

/* Lookup next ILM. */
CLI (mpls_next_ilm,
     mpls_next_ilm_cmd,
     "mpls next in-segment",
     SHOW_MPLS_STR,
     "Get next value",
     "Get next Out Segment based on index")
{
  struct nsm_master *nm = cli->vr->proto;
  struct ilm_entry *ilm;

  ilm = nsm_mpls_ilm_lookup_next (nm, 0, 0, 0);
  if (ilm)
    {
      cli_out (cli, "%% Next In-Segment with out intf: %d and label: %d\n",
               ilm->key.iif_ix, ilm->key.in_label);
      return CLI_SUCCESS;
    }

  cli_out (cli, "%% Next In-Segment entry not found\n");
  return CLI_ERROR;
}

/* Lookup next ILM. */
CLI (mpls_next_ilm1,
     mpls_next_ilm1_cmd,
     "mpls next in-segment IFINDEX",
     SHOW_MPLS_STR,
     "Get next value",
     "Get next Out Segment based on index",
     "Incoming interface index")
{
  struct nsm_master *nm = cli->vr->proto;
  struct ilm_entry *ilm;
  u_int32_t ix;

  CLI_GET_UINT32 ("Incoming interface Index", ix, argv[0]);
  ilm = nsm_mpls_ilm_lookup_next (nm, ix, 0, ILM_KEY_BITLEN - MAX_LABEL_BITLEN);
  if (ilm)
    {
      cli_out (cli, "%% Next In-Segment with out intf: %d and label: %d\n",
               ilm->key.iif_ix, ilm->key.in_label);
      return CLI_SUCCESS;
    }

  cli_out (cli, "%% Next In-Segment entry not found\n");
  return CLI_ERROR;
}

/* Lookup next ILM. */
CLI (mpls_next_ilm2,
     mpls_next_ilm2_cmd,
     "mpls next in-segment IFINDEX LABEL",
     SHOW_MPLS_STR,
     "Get next value",
     "Get next Out Segment based on index",
     "Incoming interface index",
     "Incoming label (16-1047875)")
{
  struct nsm_master *nm = cli->vr->proto;
  struct ilm_entry *ilm;
  u_int32_t ix;
  u_int32_t label;

  CLI_GET_UINT32 ("Incoming interface index", ix, argv[0]);
  CLI_GET_UINT32 ("Incoming label", label, argv[1]);
  ilm = nsm_mpls_ilm_lookup_next (nm, ix, label, ILM_KEY_BITLEN);
  if (ilm)
    {
      cli_out (cli, "%% Next In-Segment with out intf: %d and label: %d\n",
               ilm->key.iif_ix, ilm->key.in_label);
      return CLI_SUCCESS;
    }

  cli_out (cli, "%% Next In-Segment entry not found\n");
  return CLI_ERROR;
}

/* Lookup next XC. */
CLI (mpls_next_xc,
     mpls_next_xc_cmd,
     "mpls next cross-connect",
     SHOW_MPLS_STR,
     "Get next value",
     "Get next Cross Connect based on index")
{
  struct nsm_master *nm = cli->vr->proto;
  struct xc_entry *xc;
  struct gmpls_gen_label in_label;

  pal_mem_set (&in_label, 0, sizeof (struct gmpls_gen_label));
  in_label->type = gmpls_entry_type_ip;
  xc = nsm_gmpls_xc_lookup_next (nm, 0, 0, &in_label, 0, 0);
  if (xc)
    {
      cli_out (cli, "%% Next Cross Connect with index: %d, in intf: %d "
               "in label: %d and out-segment ix: %d\n",
               xc->key.xc_ix, xc->key.iif_ix, xc->key.in_label,
               xc->key.nhlfe_ix);
      return CLI_SUCCESS;
    }

  cli_out (cli, "%% Next Cross Connect entry not found\n");
  return CLI_ERROR;
}

/* Lookup next XC. */
CLI (mpls_next_xc1,
     mpls_next_xc1_cmd,
     "mpls next cross-connect INDEX",
     SHOW_MPLS_STR,
     "Get next value",
     "Get next Cross Connect based on index",
     "Cross Connect index")
{
  struct nsm_master *nm = cli->vr->proto;
  struct xc_entry *xc;
  u_int32_t xc_ix;
  struct gmpls_gen_label in_label;

  pal_mem_set (&in_label, 0, sizeof (struct gmpls_gen_label));
  in_label->type = gmpls_entry_type_ip;

  CLI_GET_UINT32 ("XC index", xc_ix, argv[0]);
  xc = nsm_gmpls_xc_lookup_next (nm, xc_ix, 0, &in_label, 0, 32);
  if (xc)
    {
      cli_out (cli, "%% Next Cross Connect with index: %d, in intf: %d "
               "in label: %d and out-segment ix: %d\n",
               xc->key.xc_ix, xc->key.iif_ix, xc->key.in_label,
               xc->key.nhlfe_ix);
      return CLI_SUCCESS;
    }

  cli_out (cli, "%% Next Cross Connect entry not found\n");
  return CLI_ERROR;
}

/* Lookup next XC. */
CLI (mpls_next_xc2,
     mpls_next_xc2_cmd,
     "mpls next cross-connect INDEX INDEX",
     SHOW_MPLS_STR,
     "Get next value",
     "Get next Cross Connect based on index",
     "Cross Connect index",
     "Incoming interface index")
{
  struct nsm_master *nm = cli->vr->proto;
  struct xc_entry *xc;
  u_int32_t xc_ix;
  u_int32_t ifindex;
  struct gmpls_gen_label in_label;

  pal_mem_set (&in_label, 0, sizeof (struct gmpls_gen_label));
  in_label->type = gmpls_entry_type_ip;

  CLI_GET_UINT32 ("XC index", xc_ix, argv[0]);
  CLI_GET_UINT32 ("Incoming interface index", ifindex, argv[1]);
  xc = nsm_gmpls_xc_lookup_next (nm, xc_ix, ifindex, 0, &in_label, 2 * 32);
  if (xc)
    {
      cli_out (cli, "%% Next Cross Connect with index: %d, in intf: %d "
               "in label: %d and out-segment ix: %d\n",
               xc->key.xc_ix, xc->key.iif_ix, xc->key.in_label,
               xc->key.nhlfe_ix);
      return CLI_SUCCESS;
    }

  cli_out (cli, "%% Next Cross Connect entry not found\n");
  return CLI_ERROR;
}

/* Lookup next XC. */
CLI (mpls_next_xc3,
     mpls_next_xc3_cmd,
     "mpls next cross-connect INDEX INDEX LABEL",
     SHOW_MPLS_STR,
     "Get next value",
     "Get next Cross Connect based on index",
     "Cross Connect index",
     "Incoming interface index",
     "Incoming label")
{
  struct nsm_master *nm = cli->vr->proto;
  struct xc_entry *xc;
  u_int32_t xc_ix;
  u_int32_t ifindex;
  u_int32_t label;

  CLI_GET_UINT32 ("XC index", xc_ix, argv[0]);
  CLI_GET_UINT32 ("Incoming interface index", ifindex, argv[1]);
  CLI_GET_UINT32 ("Incoming label", label, argv[2]);
  xc = nsm_gmpls_xc_lookup_next (nm, xc_ix, ifindex, label, 0, 3 * 32);
  if (xc)
    {
      cli_out (cli, "%% Next Cross Connect with index: %d, in intf: %d "
               "in label: %d and out-segment ix: %d\n",
               xc->key.xc_ix, xc->key.iif_ix, xc->key.in_label,
               xc->key.nhlfe_ix);
      return CLI_SUCCESS;
    }

  cli_out (cli, "%% Next Cross Connect entry not found\n");
  return CLI_ERROR;
}

/* Lookup next XC. */
CLI (mpls_next_xc4,
     mpls_next_xc4_cmd,
     "mpls next cross-connect INDEX INDEX LABEL INDEX",
     SHOW_MPLS_STR,
     "Get next value",
     "Get next Cross Connect based on index",
     "Cross Connect index",
     "Incoming interface index",
     "Incoming label",
     "Out Segment index")
{
  struct nsm_master *nm = cli->vr->proto;
  struct xc_entry *xc;
  u_int32_t xc_ix;
  u_int32_t ifindex;
  u_int32_t label;
  u_int32_t nhlfe_ix;

  CLI_GET_UINT32 ("XC index", xc_ix, argv[0]);
  CLI_GET_UINT32 ("Incoming interface index", ifindex, argv[1]);
  CLI_GET_UINT32 ("Incoming label", label, argv[2]);
  CLI_GET_UINT32 ("Out Segment index", nhlfe_ix, argv[3]);
  xc = nsm_gmpls_xc_lookup_next (nm, xc_ix, ifindex, label, nhlfe_ix, 4 * 32);
  if (xc)
    {
      cli_out (cli, "%% Next Cross Connect with index: %d, in intf: %d "
               "in label: %d and out-segment ix: %d\n",
               xc->key.xc_ix, xc->key.iif_ix, xc->key.in_label,
               xc->key.nhlfe_ix);
      return CLI_SUCCESS;
    }

  cli_out (cli, "%% Next Cross Connect entry not found\n");
  return CLI_ERROR;
}

CLI (show_used_labels,
     show_used_labels_cmd,
     "show mpls label-block-data",
     CLI_SHOW_STR,
     SHOW_MPLS_STR,
     "Show MPLS label block usage data")
{
  struct nsm_master *nm = cli->vr->proto;
  struct route_node *rn;
  struct lp_server *server;
  int i;
  bool_t heading_done = NSM_FALSE;

  if ((! nm) || (! nm->label_pool_table[GMPLS_LABEL_PACKET]))
    return CLI_SUCCESS;

  for (rn = route_top (nm->label_pool_table[GMPLS_LABEL_PACKET]); rn;
       rn = route_next (rn))
    {
      server = rn->info;
      if (! server)
        continue;

      if (! heading_done)
        {
          heading_done = PAL_TRUE;
          cli_out (cli, "Label-space   Block-id    Protocol\n");
        }

      for (i = 0; i < LABEL_BLOCK_INVALID; i++)
        {
          if (((server->lp_block[i]).status == LP_SERVER_BLOCK_IN_USE) &&
              ((server->lp_block[i]).protocol != APN_PROTO_UNSPEC) &&
              ((server->lp_block[i]).protocol != APN_PROTO_MAX))
            {
              cli_out (cli, "%-12d   %-8d   %-8s\n",
                       server->label_space,
                       i,
                       modname_strl ((server->lp_block[i]).protocol));
            }
        }
    }

  return CLI_SUCCESS;
}

#ifdef HAVE_MPLS_FWD
CLI (clear_mpls_stats,
     clear_mpls_stats_cmd,
     "clear mpls statistics (top|ftn|ilm|)",
     CLI_CLEAR_STR,
     CLEAR_MPLS_STR,
     "Clear MPLS statistics",
     "Clear top level statistics",
     "Clear FTN statistics",
     "Clear ILM statistics")
{
  if (argc == 0)
    {
      apn_mpls_clear_fwd_stats (APN_PROTO_NSM, MPLS_STAT_CLEAR_TYPE_ALL);
      return CLI_SUCCESS;
    }

  if (pal_strncmp (argv[0], "top", pal_strlen (argv[0])) == 0)
    apn_mpls_clear_fwd_stats (APN_PROTO_NSM, MPLS_STAT_CLEAR_TYPE_TOP);
  else if (pal_strncmp (argv[0], "ftn", pal_strlen (argv[0])) == 0)
    apn_mpls_clear_fwd_stats (APN_PROTO_NSM, MPLS_STAT_CLEAR_TYPE_FTN);
  else if (pal_strncmp (argv[0], "ilm", pal_strlen (argv[0])) == 0)
    apn_mpls_clear_fwd_stats (APN_PROTO_NSM, MPLS_STAT_CLEAR_TYPE_ILM);
  else
    {
      cli_out (cli, "%% Invalid argument specified.\n");
      return CLI_ERROR;
    }

  return CLI_SUCCESS;
}

#ifdef HAVE_NSM_MPLS_OAM
/* Temporary DEV TEST CLI for MPLS OAM */
CLI (mpls_oam_test,
     mpls_oam_test_cmd,
     "mpls echo-request (ldp|rsvp) A.B.C.D (send|recv) A.B.C.D",
     SHOW_MPLS_STR,
     "Test MPLS Echo Requests",
     "LDP FEC TLV",
     "RSVP FEC TLV",
     "FEC for testing",
     "Send a packet",
     "Receive an Echo Request",
     "Default Source Address")
{
  struct nsm_master *nm = cli->vr->proto;
  struct nsm_msg_mpls_ping_req msg;
  int ret = 0;

  pal_mem_set (&msg, 0 , sizeof (struct nsm_msg_mpls_ping_req));
  if (pal_strncmp (argv[0], "ldp", pal_strlen (argv[0]))== 0)
    {
      SET_FLAG (msg.cindex, (1 << 2));
      msg.type = MPLSONM_OPTION_LDP;
      CLI_GET_IPV4_ADDRESS ("FEC", msg.u.fec.ip_addr, argv[1]);
    }
  else if (pal_strncmp (argv[0], "rsvp", pal_strlen (argv[0])) == 0)
    {
      SET_FLAG (msg.cindex, (1 << 3));
      CLI_GET_IPV4_ADDRESS ("FEC", msg.u.rsvp.addr, argv[1]);
    }

  if (pal_strncmp (argv[2], "send", pal_strlen (argv[2]))== 0)
    msg.req_type = NSM_SUCCESS;
  else
    msg.req_type = NSM_FAILURE;

    CLI_GET_IPV4_ADDRESS ("SOURCE", msg.src, argv[3]);
    CLI_GET_IPV4_ADDRESS ("DEST", msg.dst, "127.0.0.10");

    msg.interval = 1;
    msg.timeout = 5;
    msg.ttl = 255;
    msg.ping_count = 10;
    msg.flags = NSM_FALSE;
    msg.reply_mode = 1;
    msg.sock = 80;

    ret = nsm_mpls_oam_process_echo_request (nm, NULL, &msg);
    if (ret < 0)
      return CLI_ERROR;

  return CLI_SUCCESS;
}
#endif /* HAVE_NSM_MPLS_OAM */
#endif /* HAVE_MPLS_FWD */
#endif /* HAVE_DEV_TEST */

/* Add an FTN entry to the Forwarder. */
int
nsm_mpls_ftn_entry (struct cli *cli, int vrf, u_int32_t t_id, char *fec,
                    char *fec_mask, char *out_label_str, char *nhop_str,
                    char *ifname,  u_char opcode, bool_t is_add,
                    char *ftn_ix_str, u_int32_t flags, u_int32_t rev_idx)
{
  struct nsm_master *nm = cli->vr->proto;
  struct ftn_add_data fad;
  struct interface *ifp;
  struct prefix p;
  struct pal_in4_addr mask;
  struct pal_in4_addr nhop;
  u_int32_t out_label;
  u_int32_t ftn_ix, oif_ix;
  bool_t is_gmpls = PAL_FALSE;
#ifdef HAVE_GMPLS
  struct datalink *dl;
#endif /* HAVE_GMPLS */
  int ret;
  bool_t is_primary;
#ifdef HAVE_GMPLS
  bool_t is_bidir;
#endif /* HAVE_GMPLS */

  if (CHECK_FLAG (flags, NSM_MPLS_CLI_FTN_PRIMARY))
    is_primary = PAL_TRUE;
  else
    is_primary = PAL_FALSE;

#ifdef HAVE_GMPLS
  if ((CHECK_FLAG (flags, NSM_MPLS_CLI_FTN_BIDIR_CONFIG)) &&
       (CHECK_FLAG (flags, NSM_MPLS_CLI_FTN_BIDIR_ENABLED)))
     is_bidir = PAL_TRUE;
  else
     is_bidir = PAL_FALSE;
#endif /* HAVE_GMPLS */

  /* The same tunnel-id can be used for one primary and one secondary */
  if (ftn_ix_str == NULL)
    {
      ret = nsm_gmpls_static_ftn_tunnel_id_check (nm, t_id, vrf, is_primary);
      if (ret > 0 && is_add)
        {
          cli_out (cli, "%% Tunnel-ID %d has been used for %s. "
                   "To update ftn-entry, please provide ftn index.\n",
                   t_id, (is_primary ? "primary" : "secondary"));
          return CLI_ERROR;
        }

      if (ret == 0 && !is_add)
        {
          cli_out (cli, "%% Tunnel-ID %d has not been configured "
                   "for this LSP.\n", t_id);
          return CLI_ERROR;
        }

      if (ret < 0)
        return CLI_ERROR;
    }

  /* Get outgoing label. */
  CLI_GET_UINT32 ("outgoing-label", out_label, out_label_str);
#if CHECK_LABEL
  if (! VALID_LABEL (out_label))
    {
      cli_out (cli, "%% Label value %s is invalid\n", out_label_str);
      return CLI_ERROR;
    }
#endif /* CHECK_LABEL */

  /* Get FEC. */
  pal_mem_set (&p, 0, sizeof (struct prefix));
  ret = str2prefix (fec, &p);
  if (ret <= 0)
    {
      cli_out (cli, "%% Malformed address\n");
      return CLI_ERROR;
    }

  /* Use mask string, if any. */
  if (fec_mask)
    {
      CLI_GET_IPV4_ADDRESS ("FEC mask", mask, fec_mask);
      p.prefixlen = ip_masklen (mask);
    }
  apply_mask (&p);

  /* Primary and secondary ftn fec should be the same for one tunnel-id. */
  ret = nsm_mpls_static_ftn_fec_check (nm, t_id, vrf, is_primary, &p);
  if (ret == 0 && is_add)
    {
      cli_out (cli, "%% This %s ftn fec does not match %s. For update, "
               "please delete %s ftn first.\n",
               (is_primary ? "primary" : "secondary"),
               (is_primary ? "secondary" : "primary"),
               (is_primary ? "secondary" : "primary"));
      return CLI_ERROR;
    }

  /* Get nexthop. */
  CLI_GET_IPV4_ADDRESS ("Nexthop", nhop, nhop_str);

  /* Get interface. */
  ifp = if_lookup_by_name (&cli->vr->ifm, ifname);
  if ((! ifp) || if_is_loopback (ifp))
    {
      cli_out (cli, "%% Interface %s is not found or is loopback\n", ifname);
      return CLI_ERROR;
    }

#ifdef HAVE_GMPLS
  if ((is_bidir) && ((ifp->gmpls_type == PHY_PORT_TYPE_GMPLS_UNKNOWN) ||
      (ifp->gmpls_type == PHY_PORT_TYPE_GMPLS_CONTROL)))
    {
      cli_out (cli, "%% Bidir LSP on GMPLS data interfaces only: %s\n",
                     ifname);
      return CLI_ERROR;
    }

  if (!is_bidir && !CHECK_FLAG (flags, NSM_MPLS_CLI_FTN_MPLS_CONFIG)
      && (ifp->gmpls_type == PHY_PORT_TYPE_GMPLS_UNKNOWN))
    {
      cli_out (cli, "%% Uni-dir GMPLS LSP allowed only on GMPLS interfaces: %s\n",
                   ifname);
      return CLI_ERROR;
    }

  if (!is_bidir && CHECK_FLAG (flags, NSM_MPLS_CLI_FTN_MPLS_CONFIG)
      && (ifp->gmpls_type != PHY_PORT_TYPE_GMPLS_UNKNOWN))
    {
      cli_out (cli, "%% Uni-dir MPLS LSP allowed only on MPLS interfaces: %s\n",
                    ifname);
      return CLI_ERROR;
    }

  if (ifp->gmpls_type == PHY_PORT_TYPE_GMPLS_UNKNOWN)
    {
      oif_ix = ifp->gifindex;
    }
  else
    {
      dl = (ifp->num_dl == 1)  ? ifp->dlink.datalink : NULL;
      if (! dl)
        {
          zlog_err (NSM_ZG, "%% Internal Error \n");
          return CLI_ERROR;
        }

      oif_ix = dl->gifindex;
      is_gmpls = PAL_TRUE;
    }

#else /* HAVE_GMPLS */
  oif_ix = ifp->ifindex;
#endif /* HAVE_GMPLS */
  /* Get FTN ix. */
  if (ftn_ix_str)
    {
       CLI_GET_UINT32 ("ftn index", ftn_ix, ftn_ix_str);
     }
   else
     ftn_ix = 0;

  /* Fill add data. */
  pal_mem_set (&fad, 0, sizeof (struct ftn_add_data));
  if (vrf != GLOBAL_FTN_ID)
    {
      fad.owner.u.b_key.afi = AFI_IP;
      prefix_copy ((struct prefix *)&fad.owner.u.b_key.u.ipv4.p, &p);
      IPV4_ADDR_COPY (&fad.owner.u.b_key.u.ipv4.peer, &nhop);
    }

  fad.owner.owner = MPLS_OTHER_CLI;
  fad.vrf_id = vrf;
  prefix_copy (&fad.fec_prefix, &p);
  fad.ftn_ix = ftn_ix;
  fad.nh_addr.afi = AFI_IP;
  IPV4_ADDR_COPY (&fad.nh_addr.u.ipv4, &nhop);
  fad.out_label = out_label;
  fad.oif_ix = oif_ix;
#ifdef HAVE_GMPLS
  fad.rev_idx = rev_idx;
#endif /*  HAVE_GMPLS */
  if (is_primary)
    fad.lsp_bits.bits.lsp_type = LSP_TYPE_PRIMARY;
  else
    fad.lsp_bits.bits.lsp_type = LSP_TYPE_SECONDARY;
  fad.lsp_bits.bits.act_type = REDIRECT_LSP;
  fad.opcode = opcode;
  fad.tunnel_id = t_id;
  if (is_gmpls)
    fad.flags = NSM_MSG_FTN_GMPLS;

  if (is_add)
     SET_FLAG (fad.flags, NSM_MSG_FTN_ADD);

#ifdef HAVE_GMPLS
  if (is_bidir)
    SET_FLAG (fad.flags, NSM_MSG_FTN_BIDIR);
#endif /* HAVE_GMPLS */

  if (is_add == NSM_TRUE)
    {
      struct ftn_ret_data rd;

      ret = nsm_mpls_ftn_add_msg_process (nm, &fad, &rd,
                                          vrf == GLOBAL_FTN_ID ?
                                          MPLS_FTN_REGULAR :
                                          MPLS_FTN_BGP_VRF, NSM_TRUE);
    }
  else
    ret = nsm_mpls_ftn_del_slow_msg_process (nm, &fad);

  if (ret < 0)
    {
      if (ret == NSM_ERR_FTN_INSTALL_FAILURE)
        {
          nsm_mpls_ftn_del_msg_process(nm, fad.vrf_id, &fad.fec_prefix,
                                       fad.ftn_ix);
        }

      zlog_warn (NSM_ZG, "Could not %s %s entry %r --> %s",
                 ((is_add == NSM_TRUE) ? "add/modify" : "delete"),
                 ((vrf == GLOBAL_FTN_ID) ? "FTN" : "VRF"),
                 &p.u.prefix4, out_label_str);
      cli_out (cli, "%% Could not %s %s entry %r --> %s\n",
               ((is_add == NSM_TRUE) ? "add/modify" : "delete"),
               ((vrf == GLOBAL_FTN_ID) ? "FTN" : "VRF"),
               &p.u.prefix4, out_label_str);
      return CLI_ERROR;
    }
  else if (IS_NSM_DEBUG_EVENT)
    zlog_info (NSM_ZG, "%s %s entry %r --> %s",
               ((is_add == NSM_TRUE) ? "Added/Modified" : "Deleted"),
               ((vrf == GLOBAL_FTN_ID) ? "FTN" : "VRF"),
               &p.u.prefix4, out_label_str);

  return CLI_SUCCESS;
}

CLI (nsm_mpls_ftn_add_cli,
     nsm_mpls_ftn_add_cmd,
     "mpls ftn-entry tunnel-id <1-100> A.B.C.D/M LABEL A.B.C.D IFNAME (primary|secondary|)",
     CONFIG_MPLS_STR,
     "Add an FTN entry",
     "Tunnel-ID",
     "Tunnel-ID <1-100>",
     "Forwarding Equivalence Class with Mask",
     "Outgoing label <16-1048575>",
     "Nexthop IPv4 address",
     "Outgoing interface name",
     "Primary LSP (default)",
     "Secondary LSP")
{
  u_int32_t id, flags = 0;

  CLI_GET_INTEGER_RANGE ("Tunnel-ID", id, argv[0], 1, 100);

  if (!(argc == 6 && pal_strncmp (argv[5], "s", 1) == 0))
    {
      SET_FLAG (flags, NSM_MPLS_CLI_FTN_PRIMARY);
    }

  SET_FLAG (flags, NSM_MPLS_CLI_FTN_MPLS_CONFIG);

  return nsm_mpls_ftn_entry (cli, GLOBAL_FTN_ID, id, argv[1], NULL,
                             argv[2], argv[3], argv[4], PUSH, NSM_TRUE,
                             NULL, flags, 0);
}

CLI (nsm_mpls_ftn_update_cli,
     nsm_mpls_ftn_update_cmd,
     "mpls ftn-entry tunnel-id <1-100> A.B.C.D/M LABEL A.B.C.D IFNAME INDEX (primary|secondary|)",
     CONFIG_MPLS_STR,
     "Add an FTN entry",
     "Tunnel-ID",
     "Tunnel-ID <1-100>",
     "Forwarding Equivalence Class with Mask",
     "Outgoing label <16-1048575>",
     "Nexthop IPv4 address",
     "Outgoing interface name",
     "FTN Index for update",
     "Primary LSP (default)",
     "Secondary LSP")
{
  u_int32_t id, flags = 0;

  CLI_GET_INTEGER_RANGE ("Tunnel-ID", id, argv[0], 1, 100);

  if (!(argc == 7 && pal_strncmp (argv[6], "s", 1) == 0))
    {
      SET_FLAG (flags, NSM_MPLS_CLI_FTN_PRIMARY);
    }

  SET_FLAG (flags, NSM_MPLS_CLI_FTN_MPLS_CONFIG);

  return nsm_mpls_ftn_entry (cli, GLOBAL_FTN_ID, id, argv[1], NULL,
                             argv[2], argv[3], argv[4], PUSH, NSM_TRUE,
                             argv[5], flags, 0);
}

CLI (nsm_mpls_ftn_mask_add,
     nsm_mpls_ftn_mask_add_cmd,
     "mpls ftn-entry tunnel-id <1-100> A.B.C.D A.B.C.D LABEL A.B.C.D IFNAME (primary|secondary|)",
     CONFIG_MPLS_STR,
     "Add an FTN entry",
     "Tunnel-ID",
     "Tunnel-ID <1-100>",
     "Forwarding Equivalence Class",
     "Mask for Forwarding Equivalence Class",
     "Outgoing label <16-1048575>",
     "Nexthop IPv4 address",
     "Outgoing interface name",
     "Primary LSP (default)",
     "Secondary LSP")
{
  u_int32_t id, flags = 0;

  CLI_GET_INTEGER_RANGE ("Tunnel-ID", id, argv[0], 1, 100);

  if (!(argc == 7 && pal_strncmp (argv[6], "s", 1) == 0))
    {
      SET_FLAG (flags, NSM_MPLS_CLI_FTN_PRIMARY);
    }

  SET_FLAG (flags, NSM_MPLS_CLI_FTN_MPLS_CONFIG);

  return nsm_mpls_ftn_entry (cli, GLOBAL_FTN_ID, id, argv[1], argv[2],
                             argv[3], argv[4], argv[5], PUSH,
                             NSM_TRUE, NULL, flags, 0);
}

CLI (nsm_mpls_ftn_mask_update,
     nsm_mpls_ftn_mask_update_cmd,
     "mpls ftn-entry tunnel-id <1-100> A.B.C.D A.B.C.D LABEL A.B.C.D IFNAME INDEX (primary|secondary|)",
     CONFIG_MPLS_STR,
     "Add an FTN entry",
     "Tunnel-ID",
     "Tunnel-ID <1-100>",
     "Forwarding Equivalence Class",
     "Mask for Forwarding Equivalence Class",
     "Outgoing label <16-1048575>",
     "Nexthop IPv4 address",
     "Outgoing interface name",
     "FTN Index for update",
     "Primary LSP (default)",
     "Secondary LSP")
{
  u_int32_t id, flags = 0;

  CLI_GET_INTEGER_RANGE ("Tunnel-ID", id, argv[0], 1, 100);

  if (!(argc == 8 && pal_strncmp (argv[7], "s", 1) == 0))
    {
      SET_FLAG (flags, NSM_MPLS_CLI_FTN_PRIMARY);
    }

  SET_FLAG (flags,  NSM_MPLS_CLI_FTN_MPLS_CONFIG);

  return nsm_mpls_ftn_entry (cli, GLOBAL_FTN_ID, id, argv[1], argv[2],
                             argv[3], argv[4], argv[5], PUSH,
                             NSM_TRUE, argv[6], flags, 0);
}

CLI (nsm_mpls_ftn_del,
     nsm_mpls_ftn_del_all_cmd,
     "no mpls ftn-entry tunnel-id <1-100> A.B.C.D/M WORD A.B.C.D IFNAME (primary|secondary|)",
     CLI_NO_STR,
     NO_CONFIG_MPLS_STR,
     "Delete an FTN entry",
     "Tunnel-ID",
     "Tunnel-ID <1-100>",
     "Forwarding Equivalence Class with Mask",
     "Outgoing label <16-1048575> or FTN index",
     "Nexthop IPv4 address",
     "Outgoing interface name",
     "Primary LSP (default)",
     "Secondary LSP")
{
  u_int32_t id, flags = 0;

  CLI_GET_INTEGER_RANGE ("Tunnel-ID", id, argv[0], 1, 100);

  if (!(argc == 6 && pal_strncmp (argv[5], "s", 1) == 0))
    {
      SET_FLAG (flags, NSM_MPLS_CLI_FTN_PRIMARY);
    }

  SET_FLAG (flags, NSM_MPLS_CLI_FTN_MPLS_CONFIG);

  return nsm_mpls_ftn_entry (cli, GLOBAL_FTN_ID, id, argv[1], NULL, argv[2],
                             argv[3], argv[4], PUSH, NSM_FALSE,
                             NULL, flags, 0);
}

CLI (nsm_mpls_ftn_mask_del,
     nsm_mpls_ftn_mask_del_all_cmd,
     "no mpls ftn-entry tunnel-id <1-100> A.B.C.D A.B.C.D WORD A.B.C.D IFNAME (primary|secondary|)",
     CLI_NO_STR,
     NO_CONFIG_MPLS_STR,
     "Delete an FTN entry",
     "Tunnel-ID",
     "Tunnel-ID <1-100>",
     "Forwarding Equivalence Class",
     "Mask for Forwarding Equivalence Class",
     "Outgoing label <16-1048575> or FTN index",
     "Nexthop IPv4 address",
     "Outgoing interface name",
     "Primary LSP (default)",
     "Secondary LSP")
{
  u_int32_t id, flags = 0;

  CLI_GET_INTEGER_RANGE ("Tunnel-ID", id, argv[0], 1, 100);

  if (!(argc == 7 && pal_strncmp (argv[6], "s", 1) == 0))
    {
      SET_FLAG (flags, NSM_MPLS_CLI_FTN_PRIMARY);
    }

  SET_FLAG (flags, NSM_MPLS_CLI_FTN_MPLS_CONFIG);

  return nsm_mpls_ftn_entry (cli, GLOBAL_FTN_ID, id, argv[1], argv[2],
                             argv[3], argv[4], argv[5], PUSH,
                             NSM_FALSE, NULL, flags, 0);
}

#ifdef HAVE_DEV_TEST
int
nsm_mpls_ftn_entry_del (struct cli *cli, int vrf, char *fec, char *fec_mask,
                        char *ix_str)
{
  struct nsm_master *nm = cli->vr->proto;
  struct prefix p;
  struct pal_in4_addr mask;
  u_int32_t ix;
  int ret;

  /* Get FEC. */
  pal_mem_set (&p, 0, sizeof (struct prefix));
  ret = str2prefix (fec, &p);
  if (ret <= 0)
    {
      cli_out (cli, "%% Malformed address\n");
      return CLI_ERROR;
    }

  /* Use mask string, if any. */
  if (fec_mask)
    {
      CLI_GET_IPV4_ADDRESS ("FEC mask", mask, fec_mask);
      p.prefixlen = ip_masklen (mask);
    }
  apply_mask (&p);

  /* Index value. */
  CLI_GET_UINT32 ("FTN index", ix, ix_str);
  if (ix == 0)
    {
      cli_out (cli, "%% Invalid index value\n");
      return CLI_ERROR;
    }

  ret = nsm_mpls_ftn_del_msg_process (nm, vrf, &p, ix);
  if (ret < 0)
    {
      zlog_warn (NSM_ZG, "Could not delete %s entry %r with index %s",
                 ((vrf == GLOBAL_FTN_ID) ? "FTN" : "VRF"),
                 &p.u.prefix4, ix_str);
      cli_out (cli, "%% Could not delete %s entry %r with index %s\n",
                 ((vrf == GLOBAL_FTN_ID) ? "FTN" : "VRF"),
                 &p.u.prefix4, ix_str);
    }
  else if (IS_NSM_DEBUG_EVENT)
    zlog_info (NSM_ZG, "Deleted %d entry %r with index %s",
               ((vrf == GLOBAL_FTN_ID) ? "FTN" : "VRF"),
               &p.u.prefix4, ix_str);

  return CLI_SUCCESS;
}

CLI (nsm_mpls_ftn_del2,
     nsm_mpls_ftn_del2_cmd,
     "no mpls ftn-entry A.B.C.D/M WORD",
     CLI_NO_STR,
     NO_CONFIG_MPLS_STR,
     "Delete an FTN entry",
     "Forwarding Equivalence Class",
     "Outgoing label <16-1048575> or FTN index")
{
  return nsm_mpls_ftn_entry_del (cli, GLOBAL_FTN_ID, argv[0], NULL,
                                 argv[1]);
}

CLI (nsm_mpls_ftn_mask_del2,
     nsm_mpls_ftn_mask_del2_cmd,
     "no mpls ftn-entry A.B.C.D A.B.C.D WORD",
     CLI_NO_STR,
     NO_CONFIG_MPLS_STR,
     "Delete an FTN entry",
     "Forwarding Equivalence Class",
     "Mask for Forwarding Equivalence Class",
     "Outgoing label <16-1048575> or FTN index")
{
  return nsm_mpls_ftn_entry_del (cli, GLOBAL_FTN_ID, argv[0], argv[1],
                                 argv[1]);
}

#ifdef HAVE_VRF
/* Show index manager internals. */
CLI (show_vrf_ix_mgr,
     show_vrf_ix_mgr_cmd,
     "show mpls index-manager vrf NAME",
     CLI_SHOW_STR,
     SHOW_MPLS_STR,
     "Show Index Manager details",
     "Show details for VRF Index Manager",
     "Name of VRF to be looked up")
{
  struct nsm_master *nm = cli->vr->proto;
  struct nsm_vrf *vrf;
  struct route_ix_table *table;
  bool_t done = NSM_FALSE;
  u_int8_t family = AF_INET;

  if (! NSM_MPLS)
    return CLI_SUCCESS;

  /* Lookup vrf. */
  vrf = nsm_vrf_lookup_by_name (nm, argv[0]);
  if (! vrf)
    {
      cli_out (cli, "%% VRF %s not configured\n", argv[0]);
      return CLI_ERROR;
    }

  /* Lookup vrf matching id. */
  while (!done)
    {
      table = nsm_gmpls_ftn_table_lookup (nm, vrf->vrf->id, family);
      if (! table)
        {
          cli_out (cli, "%% VRF table with id %d not found\n", vrf->vrf->id);
          return CLI_ERROR;
        }

      nsm_mpls_ix_mgr_dump (cli, table->ix_mgr, 0);

#ifdef HAVE_IPV6
      if (family != AF_INET6)
        family = AF_INET6;
      else
#endif
      done = NSM_TRUE;
    }
  return CLI_SUCCESS;
}

/* Show index manager internals. */
CLI (show_vrf_ix_mgr_ix,
     show_vrf_ix_mgr_ix_cmd,
     "show mpls index-manager vrf NAME INDEX",
     CLI_SHOW_STR,
     SHOW_MPLS_STR,
     "Show Index Manager details",
     "Show details for VRF Index Manager",
     "Name of VRF to be looked up",
     "Show details for bitmap that holds this index")
{
  struct nsm_master *nm = cli->vr->proto;
  struct nsm_vrf *vrf;
  struct route_ix_table *table;
  u_int32_t ix;
  bool_t done = NSM_FALSE;
  u_int8_t family = AF_INET;

  if (! NSM_MPLS)
    return CLI_SUCCESS;

  /* Lookup vrf. */
  vrf = nsm_vrf_lookup_by_name (nm, argv[0]);
  if (! vrf)
    {
      cli_out (cli, "%% VRF %s not configured\n", argv[0]);
      return CLI_ERROR;
    }

  while (!done)
    {
      /* Lookup vrf matching id. */
      table = nsm_gmpls_ftn_table_lookup (nm, vrf->vrf->id);
      if (! table)
        {
          cli_out (cli, "%% VRF table with id %d not found\n", vrf->vrf->id);
          return CLI_ERROR;
        }

      /* Get index. */
      CLI_GET_UINT32 ("VRF Index", ix, argv[1]);
      nsm_mpls_ix_mgr_dump (cli, table->ix_mgr, ix);

#ifdef HAVE_IPV6
      if (family != AF_INET6)
        family = AF_INET6;
      else
#endif
      done = NSM_TRUE;
    }
  return CLI_SUCCESS;
}

/* Add a VRF entry to the Forwarder. */
int
nsm_mpls_vrf_entry (struct cli *cli, char *vrf_str, u_int32_t t_id, char *fec,
                    char *fec_mask, char *out_label_str, char *nhop_str,
                    char *ifname, char *opcode_str, bool_t is_add,
                    char *vrf_ix_str, bool_t is_primary)
{
  struct nsm_vrf *vrf;
  u_char opcode;
  u_int32_t flags = 0

  /* Lookup vrf. */
  vrf = nsm_vrf_lookup_by_name (cli->vr->proto, vrf_str);
  if (vrf == NULL)
    {
      cli_out (cli, "%% Invalid VRF specified\n");
      return CLI_ERROR;
    }

  /* Check opcode. */
  if (pal_strncmp (opcode_str, "PUSH", pal_strlen (opcode_str)) == 0)
    opcode = PUSH;
  else if (pal_strncmp (opcode_str,"DLVR_TO_IP",pal_strlen (opcode_str)) == 0)
    opcode = DLVR_TO_IP;
  else
    {
      cli_out (cli, "%% Invalid operational code specified\n");
      return CLI_ERROR;
    }

  if (is_primary == PAL_TRUE)
    {
      SET_FLAG (flags, NSM_MPLS_CLI_FTN_PRIMARY);
    }

  return nsm_mpls_ftn_entry (cli, vrf->vrf->id, t_id, fec, fec_mask,
                             out_label_str, nhop_str, ifname, opcode,
                             is_add, vrf_ix_str, flags, 0);
}

CLI (nsm_mpls_vrf_add_cli,
     nsm_mpls_vrf_add_cmd,
     "mpls vrf-entry VRF-NAME A.B.C.D/M LABEL A.B.C.D IFNAME "
     "(PUSH|DLVR_TO_IP)",
     CONFIG_MPLS_STR,
     "Add a VRF entry",
     "VRF Identifier",
     "Forwarding Equivalence Class with Mask",
     "Outgoing label <16-1048575>",
     "Nexthop IPv4 address",
     "Outgoing interface name",
     "PUSH label",
     "Forward over IP")
{
  return nsm_mpls_vrf_entry (cli, argv[0], 0, argv[1], NULL, argv[2],
                             argv[3], argv[4], argv[5], NSM_TRUE, NULL,
                             NSM_TRUE);
}

CLI (nsm_mpls_vrf_modify_cli,
     nsm_mpls_vrf_modify_cmd,
     "mpls vrf-entry VRF-NAME A.B.C.D/M LABEL A.B.C.D IFNAME "
     "(PUSH|DLVR_TO_IP) INDEX",
     CONFIG_MPLS_STR,
     "Add a VRF entry",
     "VRF Identifier",
     "Forwarding Equivalence Class with Mask",
     "Outgoing label <16-1048575>",
     "Nexthop IPv4 address",
     "Outgoing interface name",
     "PUSH label",
     "Forward over IP",
     "VRF Index for update")
{
  return nsm_mpls_vrf_entry (cli, argv[0], 0, argv[1], NULL, argv[2],
                             argv[3], argv[4], argv[5], NSM_TRUE, argv[6],
                             NSM_TRUE);
}

CLI (nsm_mpls_vrf_mask_add,
     nsm_mpls_vrf_mask_add_cmd,
     "mpls vrf-entry VRF-NAME A.B.C.D A.B.C.D LABEL A.B.C.D IFNAME "
     "(PUSH|DLVR_TO_IP)",
     CONFIG_MPLS_STR,
     "Add a VRF entry",
     "VRF Identifier",
     "Forwarding Equivalence Class",
     "Mask for Forwarding Equivalence Class",
     "Outgoing label <16-1048575>",
     "Nexthop IPv4 address",
     "Outgoing interface name",
     "PUSH label",
     "Forward over IP")
{
  return nsm_mpls_vrf_entry (cli, argv[0], 0, argv[1], argv[2], argv[3],
                             argv[4], argv[5], argv[6], NSM_TRUE, NULL,
                             NSM_TRUE);
}

CLI (nsm_mpls_vrf_mask_modify,
     nsm_mpls_vrf_mask_modify_cmd,
     "mpls vrf-entry VRF-NAME A.B.C.D A.B.C.D LABEL A.B.C.D IFNAME "
     "(PUSH|DLVR_TO_IP) INDEX",
     CONFIG_MPLS_STR,
     "Add a VRF entry",
     "VRF Identifier",
     "Forwarding Equivalence Class",
     "Mask for Forwarding Equivalence Class",
     "Outgoing label <16-1048575>",
     "Nexthop IPv4 address",
     "Outgoing interface name",
     "PUSH label",
     "Forward over IP",
     "VRF index for update")
{
  return nsm_mpls_vrf_entry (cli, argv[0], 0, argv[1], argv[2], argv[3],
                             argv[4], argv[5], argv[6], NSM_TRUE, argv[7],
                             NSM_TRUE);
}

CLI (nsm_mpls_vrf_del_cli,
     nsm_mpls_vrf_del_all_cmd,
     "no mpls vrf-entry VRF-NAME A.B.C.D/M LABEL A.B.C.D IFNAME "
     "(PUSH|DLVR_TO_IP)",
     CLI_NO_STR,
     NO_CONFIG_MPLS_STR,
     "Delete a VRF entry",
     "VRF Identifier",
     "Forwarding Equivalence Class with Mask",
     "Outgoing label <16-1048575>",
     "Nexthop IPv4 address",
     "Outgoing interface name",
     "PUSH label",
     "Forward over IP")
{
  return nsm_mpls_vrf_entry (cli, argv[0], 0, argv[1], NULL, argv[2],
                             argv[3], argv[4], argv[5], NSM_FALSE, NULL,
                             NSM_TRUE);
}

CLI (nsm_mpls_vrf_mask_del,
     nsm_mpls_vrf_mask_del_all_cmd,
     "no mpls vrf-entry VRF-NAME A.B.C.D A.B.C.D LABEL A.B.C.D IFNAME "
     "(PUSH|DLVR_TO_IP)",
     CLI_NO_STR,
     NO_CONFIG_MPLS_STR,
     "Delete a VRF entry",
     "VRF Identifier",
     "Forwarding Equivalence Class",
     "Mask for Forwarding Equivalence Class",
     "Outgoing label <16-1048575>",
     "Nexthop IPv4 address",
     "Outgoing interface name",
     "PUSH label",
     "Forward over IP")
{
  return nsm_mpls_vrf_entry (cli, argv[0], 0, argv[1], argv[2], argv[3],
                             argv[4], argv[5], argv[6], NSM_FALSE, NULL,
                             NSM_TRUE);
}

/* Delete a VRF entry. */
int
nsm_mpls_vrf_entry_del (struct cli *cli, char *vrf_str, char *fec,
                        char *fec_mask, char *ix_str)
{
  struct nsm_vrf *vrf;

  /* Lookup vrf. */
  vrf = nsm_vrf_lookup_by_name (cli->vr->proto, vrf_str);
  if (vrf == NULL)
    {
      cli_out (cli, "%% Invalid VRF specified\n");
      return CLI_ERROR;
    }

  return nsm_mpls_ftn_entry_del (cli, vrf->vrf->id, fec, fec_mask, ix_str);
}


CLI (nsm_mpls_vrf_del2,
     nsm_mpls_vrf_del2_cmd,
     "no mpls vrf-entry VRF-ID A.B.C.D/M VRFINDEX",
     CLI_NO_STR,
     NO_CONFIG_MPLS_STR,
     "Delete a VRF entry",
     "VRF Identifier",
     "Forwarding Equivalence Class",
     "Index for VRF entry to be deleted")
{
  return nsm_mpls_vrf_entry_del (cli, argv[0], argv[1], NULL, argv[2]);
}

CLI (nsm_mpls_vrf_mask_del2,
     nsm_mpls_vrf_mask_del2_cmd,
     "no mpls vrf-entry VRF-ID A.B.C.D A.B.C.D VRFINDEX",
     CLI_NO_STR,
     NO_CONFIG_MPLS_STR,
     "Delete a VRF entry",
     "VRF Identifier",
     "Forwarding Equivalence Class",
     "Mask for Forwarding Equivalence Class",
     "Index for VRF entry to be deleted")
{
  return nsm_mpls_vrf_entry_del (cli, argv[0], argv[1], argv[2], argv[3]);
}
#endif /* HAVE_VRF */
#endif /* HAVE_DEV_TEST */

/* Add an ILM entry to the MPLS Forwarder. */
int
nsm_mpls_ilm_entry_add (struct cli *cli, char *vc_id_str, char *in_lbl_str,
                        char *if_in, char *out_lbl_str, char *if_out,
                        char *nhop_str, u_char opcode, char *fec_str,
                        char *fmask_str, char *ilm_ix_str, u_int32_t flags,
                        u_int32_t rev_idx)
{
  struct nsm_master *nm = cli->vr->proto;
  struct pal_in4_addr nhop;
  u_int32_t vc_id, in_label, out_label;
  struct prefix_ipv4 fec;
  struct pal_in4_addr fmask;
  int ret;
  u_int32_t ilm_ix = 0;

  if (vc_id_str)
    {
      /* Get VC-ID */
      CLI_GET_UINT32 ("virtual-circuit id", vc_id, vc_id_str);
      if (vc_id == 0)
        {
          cli_out (cli, "%% Invalid Virtual Circuit Identifier\n");
          return CLI_ERROR;
        }
    }
  else
    vc_id = 0;

  /* Get incoming label. */
  CLI_GET_UINT32 ("incoming-label", in_label, in_lbl_str);

  /* Outgoing label invalid for VC. */
  if ((opcode == POP_FOR_VC) && (out_lbl_str != NULL))
    {
      cli_out (cli, "%% Outgoing label not allowed for VC ILM"
               "entry\n");
      return CLI_ERROR;
    }

  if (opcode != POP_FOR_VC)
    {
      /* Get outgoing label. */
      if (opcode == POP || opcode == POP_FOR_VPN)
        out_label = LABEL_VALUE_INVALID;
      else
        {
          CLI_GET_UINT32 ("outgoing-label", out_label, out_lbl_str);
        }

    }
  else
    out_label = LABEL_VALUE_INVALID;

  /* Get nexthop. */
  if (pal_strncmp (if_out, "loopback", pal_strlen (if_out)) == 0)
    nhop.s_addr = pal_ntoh32 (INADDR_LOOPBACK);
  else
    CLI_GET_IPV4_ADDRESS ("Nexthop", nhop, nhop_str);

  /* Figure out FEC. */
  pal_mem_set (&fec, 0, sizeof (struct prefix_ipv4));
  if (fec_str)
    {
      ret = str2prefix_ipv4 (fec_str, &fec);
      if (ret <= 0)
        {
          cli_out (cli, "%% Malformed address\n");
          return CLI_ERROR;
        }
      if (fmask_str)
        {
          ret = pal_inet_pton (AF_INET, fmask_str, &fmask);
          if (ret <= 0)
            {
              cli_out (cli, "%% Malformed FEC mask\n");
              return CLI_ERROR;
            }
          fec.prefixlen = ip_masklen (fmask);
        }
    }

  if (ilm_ix_str)
    CLI_GET_UINT32 ("ILM Index", ilm_ix, ilm_ix_str);

  ret = nsm_mpls_ilm_entry_add_sub_pfx (nm, vc_id, in_label, if_in,
                                        out_label, if_out, &nhop, opcode,
                                        (struct prefix *)&fec, ilm_ix, flags,
                                        rev_idx);

  if (ret == NSM_SUCCESS)
    return CLI_SUCCESS;

  switch(ret)
    {
    case NSM_ERR_IF_NOT_FOUND:
      cli_out (cli, "%% Invalid Interface argument\n");
      break;
    case NSM_ERR_VC_ID_NOT_FOUND:
      cli_out (cli, "%% Invalid Virtual Circuit Identifier\n");
      break;
    case NSM_ERR_INVALID_LABEL:
      cli_out(cli, "%% Invalid Label Value \n");
      break;
    case NSM_ERR_ILM_DUP_ENTRY:
      cli_out (cli, "%% Duplicate ILM entry \n");
      break;
    case NSM_ERR_IF_GMPLS_ENABLED:
      cli_out (cli, "%% Interface is of type non mpls \n");
      break;
    case NSM_ERR_IF_MPLS_ENABLED:
      cli_out (cli, "%% Interface is of type non gmpls \n");
      break;
    default:
      if (out_lbl_str)
        cli_out (cli, "%% Could not add ILM Entry %d --> %d\n",
                 in_label, out_label);
      else
        cli_out (cli, "%% Could not add ILM Entry %d \n",
            in_label);
      break;
    }
  return CLI_ERROR;
}

int
nsm_mpls_ilm_entry_add_opcode (struct cli *cli, char *vc_id_str,
                               char *in_lbl_str, char *if_in, char *out_lbl_str,
                               char *if_out, char *nhop_str,
                               char *op_str, char *fec_str, char *fmask_str,
                               char *ilm_ix_str, bool_t is_mpls)
{
  u_char opcode;
  u_int32_t flags = 0;

  /* Is egress or not. */
  if (pal_strncmp (op_str, "swap", pal_strlen (op_str)) == 0)
    opcode = SWAP;
  else if (pal_strncmp (op_str, "pop", pal_strlen (op_str)) == 0)
    opcode = POP;
  else if (pal_strncmp (op_str, "vpnpop", pal_strlen (op_str)) == 0)
    opcode = POP_FOR_VPN;
  else
    {
      cli_out (cli, "%% Invalid input\n");
      return CLI_ERROR;
    }

  if (is_mpls)
    SET_FLAG (flags, NSM_MPLS_CLI_ILM_MPLS_CONFIG);

  return nsm_mpls_ilm_entry_add (cli, vc_id_str, in_lbl_str,
                                 if_in, out_lbl_str, if_out, nhop_str,
                                 opcode, fec_str, fmask_str, ilm_ix_str, flags, 0);
}

CLI (nsm_mpls_ilm_add_cli,
     nsm_mpls_ilm_add_cmd,
     "mpls ilm-entry LABEL IFNAME (swap|vpnpop) LABEL IFNAME "
     "A.B.C.D (A.B.C.D/M|A.B.C.D A.B.C.D) (<1-4294967295>|)",
     CONFIG_MPLS_STR,
     "Add an ILM entry",
     "Incoming label <16-1048575>",
     "Incoming interface name",
     "Swap incoming label",
     "Pop incoming label and forward VPN packet",
     "Outgoing label <0(explicit-null), 3(implicit-null), 16-1048575>",
     "Outgoing interface name",
     "Nexthop IPv4 address",
     "FEC for which this ILM entry is being created, plus mask",
     "FEC for which this ILM entry is being created",
     "FEC Mask",
     "ILM Index for updating an exisiting entry")
{
  bool_t is_mpls = PAL_TRUE;
  if (argc == 3)
    return nsm_mpls_ilm_entry_add_opcode (cli, NULL, argv[0], argv[1], NULL,
                                          "loopback", NULL, argv[2],
                                          NULL, NULL, NULL, is_mpls);

  if (argc == 7)
    return nsm_mpls_ilm_entry_add_opcode (cli, NULL, argv[0], argv[1], argv[3],
                                              argv[4], argv[5], argv[2],
                                              argv[6], NULL, NULL, is_mpls);

  if (argc == 8)
    {
      if (pal_strchr (argv[7], '.') != NULL)
        return nsm_mpls_ilm_entry_add_opcode (cli, NULL, argv[0], argv[1], argv[3],
                                              argv[4], argv[5], argv[2],
                                              argv[6], argv[7], NULL, is_mpls);

      return nsm_mpls_ilm_entry_add_opcode (cli, NULL, argv[0], argv[1], argv[3],
                                            argv[4], argv[5], argv[2],
                                            argv[6], NULL, argv[7], is_mpls);
    }

  return nsm_mpls_ilm_entry_add_opcode (cli, NULL, argv[0], argv[1], argv[3],
                                        argv[4], argv[5], argv[2],
                                        argv[6], argv[7], argv[8], is_mpls);
}

ALI (nsm_mpls_ilm_add_cli,
    nsm_mpls_ilm_add_pop1_cmd,
    "mpls ilm-entry LABEL IFNAME (pop)",
    CONFIG_MPLS_STR,
    "Add an ILM entry",
    "Incoming label <16-1048575>",
    "Incoming interface name",
    "Pop incoming label");

CLI (nsm_mpls_ilm_add_pop_cli,
    nsm_mpls_ilm_add_pop2_cmd,
    "mpls ilm-entry LABEL IFNAME (pop) IFNAME "
    "A.B.C.D (A.B.C.D/M|A.B.C.D A.B.C.D) (<1-4294967295>|)",
    CONFIG_MPLS_STR,
    "Add an ILM entry",
    "Incoming label <16-1048575>",
    "Incoming interface name",
    "Pop incoming label",
    "Outgoing interface name",
    "Nexthop IPv4 address",
    "FEC for which this ILM entry is being created, plus mask",
    "FEC for which this ILM entry is being created",
    "FEC Mask",
    "ILM Index for updating an exisiting entry")
{
    bool_t is_mpls = PAL_TRUE;
    if (argc == 6)
      return nsm_mpls_ilm_entry_add_opcode (cli, NULL, argv[0], argv[1], NULL,
                                            argv[3], argv[4], argv[2],
                                            argv[5], NULL, NULL, is_mpls);

    if (pal_strchr (argv[6], '.') != NULL)
      {
        if (argc == 7)
          return nsm_mpls_ilm_entry_add_opcode (cli, NULL, argv[0], argv[1], NULL,
                                                argv[3], argv[4], argv[2],
                                                argv[5], argv[6], NULL, is_mpls);

        return nsm_mpls_ilm_entry_add_opcode (cli, NULL, argv[0], argv[1], NULL,
                                              argv[3], argv[4], argv[2],
                                              argv[5], argv[6], argv[7], is_mpls);
      }

    return nsm_mpls_ilm_entry_add_opcode (cli, NULL, argv[0], argv[1], NULL,
                                          argv[3], argv[4], argv[2],
                                          argv[5], NULL, argv[6], is_mpls);
}


#ifdef HAVE_DEV_TEST
#ifdef HAVE_MPLS_VC
CLI (nsm_mpls_vc_ilm_add,
     nsm_mpls_vc_ilm_add_cmd,
     "mpls l2-circuit-ilm-entry VC-ID LABEL IFNAME IFNAME A.B.C.D "
     "(<1-4294967295>|)",
     CONFIG_MPLS_STR,
     "Add a Layer-2 MPLS Virtual Circuit ILM entry",
     "Virtual Circuit Identifier",
     "Incoming label <16-1048575>",
     "Incoming interface name",
     "Outgoing interface name",
     "Nexthop IPv4 address",
     "ILM Index")
{
  if (argc == 5)
    return nsm_mpls_ilm_entry_add (cli, argv[0], argv[1], argv[2], NULL,
                                   argv[3], argv[4], POP_FOR_VC,
                                   NULL, NULL, NULL, 0, 0);

  return nsm_mpls_ilm_entry_add (cli, argv[0], argv[1], argv[2], NULL,
                                 argv[3], argv[4], POP_FOR_VC,
                                 NULL, NULL, argv[5], 0, 0);
}
#endif /* HAVE_MPLS_VC */
#endif /* HAVE_DEV_TEST */

/* Delete an ILM entry from the MPLS Forwarder. */
int
nsm_mpls_ilm_entry_del (struct cli *cli, char *in_label_str, char *if_in,
                        char *ilm_ix_str)
{
  struct nsm_master *nm = cli->vr->proto;
  struct interface *ifp_in;
  u_int32_t in_label, ilm_ix, iif_ix;
  struct gmpls_gen_label lbl;
  int ret;
#ifdef HAVE_GMPLS
  struct gmpls_if *gmif;
  struct datalink *dl;
#endif /* HAVE_GMPLS */

  /* Get incoming label. */
  CLI_GET_UINT32 ("incoming-label", in_label, in_label_str);
#if CHECK_LABEL
  if (! VALID_LABEL (in_label))
    {
      cli_out (cli, "%% Label value %s is invalid\n", in_label_str);
      return CLI_ERROR;
    }
#endif /* CHECK_LABEL */

  /* Get interface. */
  ifp_in = if_lookup_by_name (&cli->vr->ifm, if_in);
  if (! ifp_in || if_is_loopback (ifp_in))
    {
      cli_out (cli, "%% Incoming interface %s is not found or is loopback\n",
               ifp_in);
      return CLI_ERROR;
    }

#ifdef HAVE_GMPLS
  gmif = gmpls_if_get (nm->vr->zg, nm->vr->id);
  if (gmif == NULL)
    return CLI_ERROR;

  if (ifp_in->gmpls_type == PHY_PORT_TYPE_GMPLS_UNKNOWN)
    {
      iif_ix = ifp_in->gifindex;
    }
  else
    {
      dl = (ifp_in->num_dl == 1)  ? ifp_in->dlink.datalink : NULL;
      if (! dl)
        {
          zlog_err (NSM_ZG, "%% Internal Error \n");
          return CLI_ERROR;
        }

      iif_ix = dl->gifindex;
    }

#else /* HAVE_GMPLS */
  iif_ix = ifp_in->ifindex;
#endif /* HAVE_GMPLS */

  /* Get ilm index */
  if (! ilm_ix_str)
    ilm_ix = 0;
  else
    CLI_GET_UINT32 ("ilm-index", ilm_ix, ilm_ix_str);

  lbl.type = gmpls_entry_type_ip;
  lbl.u.pkt = in_label;

  ret = nsm_gmpls_ilm_fast_del_msg_process (nm, iif_ix, &lbl, ilm_ix);
  if (ret < 0)
    {
      zlog_warn (NSM_ZG, "Could not remove ILM Entry %s:%s",
                 in_label_str, if_in);
      cli_out (cli, "%% Could not remove ILM Entry %s:%s\n",
               in_label_str, if_in);
    }
  else if (IS_NSM_DEBUG_EVENT)
    zlog_info (NSM_ZG, "Successfully removed ILM Entry %s:%s",
               in_label_str, if_in);

  return CLI_SUCCESS;
}

CLI (nsm_mpls_ilm_del,
     nsm_mpls_ilm_del_cmd,
     "no mpls ilm-entry LABEL IFNAME (swap|vpnpop) LABEL IFNAME "
     "A.B.C.D (A.B.C.D/M|A.B.C.D A.B.C.D|) (<1-4294967295>|)",
     CLI_NO_STR,
     NO_CONFIG_MPLS_STR,
     "Delete  an ILM entry",
     "Incoming label <16-1048575>",
     "Incoming interface name",
     "Swap incoming label",
     "Pop incoming label and forward VPN packet",
     "Outgoing label <0(explicit-null), 3(implicit-null), 16-1048575>",
     "Outgoing interface name",
     "Nexthop IPv4 address",
     "FEC for which this ILM entry is being created, plus mask",
     "FEC for which this ILM entry is being created",
     "Mask for FEC",
     "ILM Index")
{
  int index = 0;

  switch (argc)
    {
    case 7:
      if (pal_strchr (argv[6], '.') == NULL)
        index = 6;
      break;
    case 8:
      if (pal_strchr (argv[7], '.') == NULL)
        index = 7;
      break;
    case 9:
      index = 8;
      break;
    default:
      break;
    }

  return nsm_mpls_ilm_entry_del (cli, argv[0], argv[1],
                                 index > 0 ? argv[index] : NULL);
}

ALI (nsm_mpls_ilm_del,
     nsm_mpls_ilm_del_pop1_cmd,
     "no mpls ilm-entry LABEL IFNAME (pop)",
     CLI_NO_STR,
     NO_CONFIG_MPLS_STR,
     "Delete  an ILM entry",
     "Incoming label <16-1048575>",
     "Incoming interface name",
     "Pop incoming label");

CLI (nsm_mpls_ilm_del_pop,
     nsm_mpls_ilm_del_pop2_cmd,
     "no mpls ilm-entry LABEL IFNAME (pop) IFNAME "
     "A.B.C.D (A.B.C.D/M|A.B.C.D A.B.C.D|) (<1-4294967295>|)",
      CLI_NO_STR,
      NO_CONFIG_MPLS_STR,
      "Delete  an ILM entry",
      "Incoming label <16-1048575>",
      "Incoming interface name",
      "Pop incoming label",
      "Outgoing interface name",
      "Nexthop IPv4 address",
      "FEC for which this ILM entry is being created, plus mask",
      "FEC for which this ILM entry is being created",
      "Mask for FEC",
      "ILM Index")
{
  int index = 0;

  switch (argc)
    {
    case 6:
      if (pal_strchr (argv[5], '.') == NULL)
        index = 5;
      break;
    case 7:
      if (pal_strchr (argv[6], '.') == NULL)
        index = 6;
      break;
    case 8:
      index = 7;
      break;
    default:
      break;
    }

  return nsm_mpls_ilm_entry_del (cli, argv[0], argv[1],
                                             index > 0 ? argv[index] : NULL);
}



#ifdef HAVE_MPLS_VC
int
nsm_mpls_vc_fib_entry_create (struct cli *cli, struct nsm_mpls_circuit *vc,
                           char *in_label_str, char *out_label_str,
                           char *nhop_str, char *if_in, char *if_out_or_vc)
{
  struct nsm_master *nm = cli->vr->proto;
  struct interface *ifp_in = NULL, *ifp_out = NULL;
  struct nsm_mpls_if *mif;
  struct nsm_msg_vc_fib_add msg;
  struct addr_in addr;
  u_int32_t in_label, out_label;
  u_int16_t type;
  char *vc_name;
  struct nsm_vrf *nv;
  int ret = 0;
  u_int8_t is_ms_pw = NSM_FALSE;
#ifdef HAVE_MS_PW
  struct nsm_mpls_circuit *vc_other = NULL;
#endif /* HAVE_MS_PW */

  addr.afi = AFI_IP;
  CLI_GET_IPV4_ADDRESS ("Peer address", addr.u.ipv4, nhop_str);
  pal_mem_set(&msg, 0, sizeof (struct nsm_msg_vc_fib_add));
  msg.addr = addr;
  msg.vc_style = MPLS_VC_STYLE_MARTINI;
  msg.vc_id = vc->id;
#ifdef HAVE_MS_PW
  if (vc->ms_pw)
    is_ms_pw = NSM_TRUE;
#endif /* HAVE_MS_PW */

  nv = nsm_vrf_lookup_by_id (nm, VRF_ID_UNSPEC);
  if (nv == NULL)
  {
    cli_out (cli, "%% Failure to lookup vrf \n");
    return CLI_ERROR;
  }
  msg.lsr_id = pal_ntoh32 (nv->vrf->router_id.s_addr);

  type = VC_TYPE_UNKNOWN;
  vc_name = vc->name;

  /* Get Incoming Label */
  CLI_GET_UINT32 ("incoming-label", in_label, in_label_str);
  if (! VALID_LABEL (in_label))
    {
      cli_out (cli, "%% Incoming label value %s is invalid \n", in_label_str);
      return CLI_ERROR;
    }

  /* Get Outgoing Label */
  CLI_GET_UINT32 ("outgoing-label", out_label, out_label_str);
  if (! VALID_LABEL (out_label))
    {
      cli_out (cli, "%% Outgoing label value %s is invalid \n", out_label_str);
      return CLI_ERROR;
    }

  if (! IPV4_ADDR_SAME (&vc->address.u.prefix4, &addr.u.ipv4))
    {
      cli_out (cli, "%% Nexthop IP address should be same as peer IP address \n");
      return CLI_ERROR;
    }

  /* Incoming interface. */
  ifp_in = if_lookup_by_name (&cli->vr->ifm, if_in);
  if (nsm_mpls_check_valid_interface (ifp_in, SWAP) != NSM_TRUE)
    {
      cli_out (cli, "%% Incoming interface %s is not valid \n",
               if_in);
      return CLI_ERROR;
    }

  if (!is_ms_pw)
    {
      /* Outgoing interface. */
      ifp_out = if_lookup_by_name (&cli->vr->ifm, if_out_or_vc);
      if (nsm_mpls_check_valid_interface (ifp_out, POP) != NSM_TRUE)
        {
          cli_out (cli, "%% Outgoing interface %s is not valid \n",
                   if_out_or_vc);
          return CLI_ERROR;
        }

      mif = nsm_mpls_if_lookup (ifp_out);
      if (! mif)
        {
          cli_out (cli, "%% Outgoing interface %s is not valid \n",
                   if_out_or_vc);
          return CLI_ERROR;
        }

      /* Check if ifp_out is binded to vc */
      if (! vc->vc_info)
       {
          cli_out (cli, "%% Need bind VC %s to interface first \n", vc->name);
          return CLI_ERROR;
        }

      if (vc->vc_info->mif != mif)
        {
          cli_out (cli, "%% VC is not binded to interface %s \n", if_out_or_vc);
          return CLI_ERROR;
        }
    }
#ifdef HAVE_MS_PW
  else
    {
      vc_other = nsm_mpls_l2_circuit_lookup_by_name (nm, if_out_or_vc);
      if (!vc_other)
        {
          cli_out (cli, "%% VC %s is not found\n", if_out_or_vc);
          return CLI_ERROR;
        }
      if (vc->vc_other->id != vc_other->id)
        {
          cli_out (cli, "%% VC %s is not stitched to %s\n", if_out_or_vc,
                   vc->name);
          return CLI_ERROR;
        }
    }
#endif /* HAVE_MS_PW */

  /* Populate the fib add msg structure */
  msg.in_label = in_label;
  msg.out_label = out_label;
#ifdef HAVE_GMPLS
  msg.nw_if_ix = ifp_in->gifindex;
#else
  msg.nw_if_ix = ifp_in->ifindex;
#endif /* HAVE_GMPLS */
  if (!is_ms_pw)
    {
#ifdef HAVE_GMPLS
      msg.ac_if_ix = ifp_out->gifindex;
#else
      msg.ac_if_ix = ifp_out->ifindex;
#endif /* HAVE_GMPLS */
    }
#ifdef HAVE_VCCV
  msg.remote_cc_types = vc->cc_types;
  msg.remote_cv_types = vc->cv_types;
#endif /* HAVE_VCCV */

  ret = nsm_mpls_vc_fib_add_msg_process (nm, &msg);
  if ( ret < 0)
    {
      cli_out (cli, "%% Error: VC Fib entry add process failed \n");
      return CLI_ERROR;
    }

  return CLI_SUCCESS;
}

int
nsm_mpls_vc_fib_entry_add (struct cli *cli, char *vc_id_str,
                           char *in_label_str, char *out_label_str,
                           char *nhop_str, char *if_in, char *if_out_or_vc)
{
  struct nsm_master *nm = cli->vr->proto;
  struct nsm_mpls_circuit *vc;
  u_int32_t vc_id;

  CLI_GET_UINT32 ("vc-id", vc_id, vc_id_str);

  if (vc_id < NSM_MPLS_L2_VC_MIN ||
      vc_id > NSM_MPLS_L2_VC_MAX)
    {
      cli_out (cli, "%% Invalid Virtual Circuit Identifier\n");
      return CLI_ERROR;
    }

  vc = nsm_mpls_l2_circuit_lookup_by_id (nm, vc_id);

  if (! vc)
    {
      cli_out (cli, "%% Invalid Virtual Circuit Identifier\n");
      return CLI_ERROR;
    }

  if (vc->fec_type_vc != PW_OWNER_MANUAL)
    {
      cli_out (cli, "%% VC not configured as set-up Manually\n");
      return CLI_ERROR;
    }

  return nsm_mpls_vc_fib_entry_create (cli, vc, in_label_str, out_label_str,
                                       nhop_str, if_in, if_out_or_vc);
}

CLI (nsm_mpls_vc_fib_add_cli,
     nsm_mpls_vc_fib_add_cmd,
     "mpls l2-circuit-fib-entry VC-ID LABEL LABEL A.B.C.D IFNAME NAME",
     CONFIG_MPLS_STR,
     "Add a Layer-2 MPLS Virtual Circuit FIB entry",
     "Virtual Circuit Identifier",
     "In-coming Label <16-1048575>",
     "Out-going Label <16-1048575>",
     "Nexthop IPv4 address",
     "Provider facing interface name",
     "Access interface name or VC to be stitched to")
{
  return nsm_mpls_vc_fib_entry_add (cli, argv[0], argv[1], argv[2],
                                    argv[3], argv[4], argv[5]);
}

int
nsm_mpls_vc_fib_entry_del (struct cli *cli, char *vc_id_str)
{
  struct nsm_master *nm = cli->vr->proto;
  struct nsm_mpls_circuit *vc;
  struct nsm_msg_vc_fib_delete msg;
  u_int32_t vc_id;
  int ret;

  CLI_GET_UINT32 ("vc-id", vc_id, vc_id_str);

  if (vc_id < NSM_MPLS_L2_VC_MIN ||
      vc_id > NSM_MPLS_L2_VC_MAX)
    {
      cli_out (cli, "%% Invalid Virtual Circuit Identifier\n");
      return CLI_ERROR;
    }

  vc = nsm_mpls_l2_circuit_lookup_by_id (nm, vc_id);

  if (! vc)
    {
      cli_out (cli, "%% Invalid Virtual Circuit Identifier\n");
      return CLI_ERROR;
    }

  if (vc->fec_type_vc != PW_OWNER_MANUAL)
    {
      cli_out (cli, "%% VC not configured as set-up Manually\n");
      return CLI_ERROR;
    }

  msg.vc_id = vc_id;
  ret = nsm_mpls_vc_fib_del_msg_process (nm, &msg);

  if (ret < 0)
    {
      cli_out (cli, "VC FIB delete precessing failed for VC Id %d\n", vc_id);
      return CLI_ERROR;
    }

  return CLI_SUCCESS;
}

CLI (nsm_mpls_vc_fib_del_cli,
     nsm_mpls_vc_fib_del_cmd,
     "no mpls l2-circuit-fib-entry VC-ID",
     CLI_NO_STR,
     NO_CONFIG_MPLS_STR,
     "Delete a Layer-2 MPLS Virtual Circuit FIB entry",
     "Virtual Circuit Identifier")
{
  return nsm_mpls_vc_fib_entry_del (cli, argv[0]);
}
#endif /* HAVE_MPLS_VC */

#ifdef HAVE_DEV_TEST
#ifdef HAVE_MPLS_VC
CLI (nsm_mpls_vc_ilm_del,
     nsm_mpls_vc_ilm_del_cmd,
     "no mpls l2-circuit-ilm-entry LABEL IFNAME LABEL IFNAME A.B.C.D"
     "(<1-4294967295>|)",
     CLI_NO_STR,
     NO_CONFIG_MPLS_STR,
     "Delete a Layer-2 MPLS Virtual Circuit ILM entry",
     "Incoming label <16-1048575>",
     "Incoming interface name",
     "Outgoing label <16-1048575>",
     "Outgoing interface name",
     "Nexthop IPv4 address",
     "ILM Index")
{
  if (argc == 5)
    return nsm_mpls_ilm_entry_del (cli, argv[0], argv[1], NULL);

  return nsm_mpls_ilm_entry_del (cli, argv[0], argv[1], argv[5]);
}
/* Add a VC FTN entry to the Forwarder. */
int
nsm_mpls_vc_ftn_entry_add (struct cli *cli, char *vc_id_str,
                           char *out_label_str, char *nhop_str,
                           char *if_in, char *if_out)
{
#if 0
  struct nsm_master *nm = cli->vr->proto;
  VC_FTN_ADD_DATA vad;
  struct vc_ftn_ret_data ret_data;
  struct interface *ifp_in, *ifp_out;
  struct pal_in4_addr nhop;
  u_int32_t vc_id, out_label;
  int ret;

  /* Get VC id. */
  CLI_GET_UINT32 ("virtual circuit id", vc_id, vc_id_str);
  if (vc_id == NSM_MPLS_L2_VC_UNDEFINED)
    {
      cli_out (cli, "%% VC id %s is invalid\n", vc_id_str);
      return CLI_ERROR;
    }

  /* Get outgoing label. */
  CLI_GET_UINT32 ("outgoing-label", out_label, out_label_str);
#if CHECK_LABEL
  if (! VALID_LABEL (out_label))
    {
      cli_out (cli, "%% Label value %s is invalid\n", out_label_str);
      return CLI_ERROR;
    }
#endif /* CHECK_LABEL */

  /* Get nexthop. */
  CLI_GET_IPV4_ADDRESS ("Nexthop", nhop, nhop_str);

  /* Get incoming interface. */
  ifp_in = if_lookup_by_name (&cli->vr->ifm, if_in);
  if (nsm_mpls_check_valid_interface (ifp_in, MPLS_NO_OP) != NSM_TRUE)
    {
      cli_out (cli, "%% Incoming interface %s is not valid\n", if_in);
      return CLI_ERROR;
    }

  /* Get outgoing interface. */
  ifp_out = if_lookup_by_name (&cli->vr->ifm, if_out);
  if (nsm_mpls_check_valid_interface (ifp_out, MPLS_NO_OP) != NSM_TRUE)
    {
      cli_out (cli, "%% Outgoing interface %s is not valid\n", if_out);
      return CLI_ERROR;
    }

  /* Check if interfaces are same. */
  if (ifp_out->ifindex == ifp_in->ifindex)
    {
      cli_out (cli, "%% Incoming and outgoing interfaces MUST be different\n");
      return CLI_ERROR;
    }

  /* Confirm outgoing interface for PUSH_FOR_VC. */
  if (ifp_out->ls_data.status != LABEL_SPACE_VALID)
    {
      cli_out (cli, "%% Outgoing interface is invalid\n");
      return CLI_ERROR;
    }

  /* Fill add data. */
  pal_mem_set (&vad, 0, sizeof (VC_FTN_ADD_DATA));
  vad.owner.owner = MPLS_OTHER_LDP_VC;
  vad.owner.u.vc_key.vc_id = vc_id;
  vad.iif_ix = ifp_in->ifindex;
  vad.oif_ix = ifp_out->ifindex;
  vad.out_label = out_label;
  vad.nh_addr.afi = AFI_IP;
  IPV4_ADDR_COPY (&vad.nh_addr.u.ipv4, &nhop);

  ret = nsm_mpls_vc_ftn_add_msg_process (nm, &vad, &ret_data);
  if (ret < 0)
    {
      zlog_warn (NSM_ZG, "Could not add VC FTN entry %s --> %s",
                 if_in, out_label_str);
    }
  else
    {
      if (IS_NSM_DEBUG_EVENT)
        zlog_info (NSM_ZG, "Added/Modified FTN entry %s --> %s",
                   if_in, out_label_str);
    }

#endif
  return CLI_SUCCESS;
}

CLI (nsm_mpls_vc_ftn_add_cli,
     nsm_mpls_vc_ftn_add_cmd,
     "mpls l2-circuit-ftn-entry VC-ID LABEL A.B.C.D IFNAME IFNAME",
     CONFIG_MPLS_STR,
     "Add a Layer-2 MPLS Virtual Circuit FTN entry",
     "Virtual Circuit Identifier",
     "Outgoing label <16-1048575>",
     "Nexthop IPv4 address",
     "Incoming interface name",
     "Outgoing interface name")
{
  return nsm_mpls_vc_ftn_entry_add (cli, argv[0], argv[1], argv[2],
                                    argv[3], argv[4]);
}

/* Delete a VC FTN entry from the MPLS Forwarder. */
int
nsm_mpls_vc_ftn_entry_del (struct cli *cli, char *vc_id_str, char *if_in)
{
#if 0
  struct nsm_master *nm = cli->vr->proto;
  struct vc_ftn_del_data vdd;
  struct interface *ifp_in;
  u_int32_t vc_id;
  int ret;

  /* Get incoming interface. */
  ifp_in = if_lookup_by_name (&cli->vr->ifm, if_in);
  if (nsm_mpls_check_valid_interface (ifp_in, MPLS_NO_OP) != NSM_TRUE)
    {
      cli_out (cli, "%% Incoming interface %s is not valid\n", if_in);
      return CLI_ERROR;
    }

  /* Get VC id. */
  CLI_GET_UINT32 ("virtual circuit id", vc_id, vc_id_str);
  if (vc_id == NSM_MPLS_L2_VC_UNDEFINED)
    {
      cli_out (cli, "%% VC id %s is invalid\n", vc_id_str);
      return CLI_ERROR;
    }

  /* Fill delete data. */
  pal_mem_set (&vdd, 0, sizeof (struct vc_ftn_del_data));
  vdd.owner.owner = MPLS_OTHER_LDP_VC;
  vdd.owner.u.vc_key.vc_id = vc_id;
  vdd.iif_ix = ifp_in->ifindex;

  ret = nsm_mpls_vc_ftn_del_msg_process (nm, &vdd);
  if (ret < 0)
    zlog_warn (NSM_ZG, "Could not remove VC FTN entry for interface %s",
               if_in);
  else if (IS_NSM_DEBUG_EVENT)
    zlog_info (NSM_ZG, "Successfully removed FTN entry for interface %s",
               if_in);

#endif
  return CLI_SUCCESS;
}

CLI (nsm_mpls_vc_ftn_del_cli,
     nsm_mpls_vc_ftn_del_cmd,
     "no mpls l2-circuit-ftn-entry VC-ID IFNAME",
     CLI_NO_STR,
     NO_CONFIG_MPLS_STR,
     "Delete a Layer-2 MPLS Virtual Circuit FTN entry",
     "Incoming interface name",
     "Virtual Circuit identifier configured")
{
  return nsm_mpls_vc_ftn_entry_del (cli, argv[0], argv[1]);
}

CLI (nsm_mpls_vc_ftn_del_all,
     nsm_mpls_vc_ftn_del_all_cmd,
     "no mpls l2-circuit-ftn-entry VC-ID LABEL A.B.C.D IFNAME IFNAME",
     CLI_NO_STR,
     NO_CONFIG_MPLS_STR,
     "Delete a Layer-2 MPLS Virtual Circuit FTN entry",
     "Virtual Circuit Identifier",
     "Outgoing label <16-1048575>",
     "Nexthop IPv4 address",
     "Incoming interface name",
     "Outgoing interface name")
{
  return nsm_mpls_vc_ftn_entry_del (cli, argv[0], argv[3]);
}
#endif /* HAVE_MPLS_VC */
#endif /* HAVE_DEV_TEST */

#ifdef HAVE_VPLS

/** @brief Function to create vpls fib entry.
    @param nm          - NSM master
    @param vpls        - vpls
    @param vc_style    - VC style (Mesh/Spoke)
    @param egress      - Peer address
    @param in_lbl_str  - Incoming label
    @param if_out      - Provider facing interface
    @param out_lbl_str - Outgoing label
 */
int
nsm_vpls_fib_entry_create (struct nsm_master *nm, struct nsm_vpls *vpls,
                           u_int8_t vc_style, char *egress,
                           char* in_lbl_str, char* if_out, char* out_lbl_str)
{
  struct nsm_msg_vc_fib_add msg;
  struct nsm_vpls_peer *peer = NULL;
  struct nsm_vpls_spoke_vc *svc = NULL;
  struct interface *ifp_out = NULL;
  struct addr_in addr;
  u_int32_t in_label, out_label;
  u_int32_t min_label, max_label;
  int ret = 0;

  pal_mem_set (&msg, 0, sizeof(struct nsm_msg_vc_fib_add));

  msg.vpn_id = vpls->vpls_id;
  addr.afi = AFI_IP;

  if (vc_style == MPLS_VC_STYLE_VPLS_MESH)
    {
      ret = pal_inet_pton (AF_INET, egress, &addr.u.ipv4);
      if (ret < 0)
        return NSM_ERR_INVALID_ARGS;

      peer = nsm_vpls_mesh_peer_lookup (vpls, &addr, NSM_FALSE);
      if (! peer)
        return NSM_ERR_VPLS_PEER_NOT_FOUND;

      /* Identify whether peer is configured to use static fib entry. */
      if (peer->fec_type_vc != PW_OWNER_MANUAL)
        return NSM_ERR_VPLS_PEER_PW_MANUAL_NOT_CONFIGURED;

      msg.addr = addr;
      msg.vc_style = MPLS_VC_STYLE_VPLS_MESH;
      msg.vc_id = vpls->vpls_id;
    }
  else if (vc_style == MPLS_VC_STYLE_VPLS_SPOKE)
    {
       svc = nsm_vpls_spoke_vc_lookup_by_name (vpls, egress, NSM_FALSE);
       if (! svc)
         return NSM_ERR_SPOKE_VC_NOT_FOUND;

      msg.vc_style = MPLS_VC_STYLE_VPLS_SPOKE;
      if (svc->vc == NULL)
        return NSM_ERR_SPOKE_VC_VIRTUAL_CIRCUIT_NOT_FOUND;

      if (svc->vc->fec_type_vc != PW_OWNER_MANUAL)
        return NSM_ERR_VPLS_SPOKE_VC_MANUAL_NOT_CONFIGURED;

      msg.vc_id = svc->vc->id;
    }
  else
    return NSM_ERR_INVALID_ARGS;

  /* Get Incoming Label */
  in_label = cmd_str2int (in_lbl_str, &ret);
  if (! VALID_LABEL (in_label))
    return NSM_ERR_VPLS_INVALID_INCOMING_LABEL;

  /* Get Outgoing Label */
  out_label = cmd_str2int (out_lbl_str, &ret);
  if (! VALID_LABEL (out_label))
    return NSM_ERR_VPLS_INVALID_OUTGOING_LABEL;

  /* Identify Outgoing interface. */
  ifp_out = if_lookup_by_name (&nm->vr->ifm, if_out);

  if (nsm_mpls_check_valid_interface (ifp_out, POP) != NSM_TRUE)
    return NSM_ERR_VPLS_INVALID_OUTGOING_INTERFACE;

  ret = nsm_gmpls_static_label_range_check (in_label, ifp_out,
                                            &min_label, &max_label);
  if (ret < 0)
    {
      zlog_err (NSM_ZG, "%% Incoming label should within range of %d - %d \n",
                min_label, max_label);
      return NSM_ERR_INVALID_LABEL;
    }

  /* Populate the fib add msg structure */
  msg.in_label = in_label;
  msg.out_label = out_label;
#ifdef HAVE_GMPLS
  msg.nw_if_ix = ifp_out->gifindex;
#else
  msg.nw_if_ix = ifp_out->ifindex;
#endif /* HAVE_GMPLS */

  /* Activate VPLS. */
  if (vpls->state == NSM_VPLS_STATE_INACTIVE)
    nsm_vpls_activate (vpls);

  return nsm_vpls_fib_add_msg_process (nm, &msg);
}

/** @brief API to add VPLS mesh/spoke VC static fib entry.
    @param vr_id         - VR identifier
    @param vpls_id       - VPLS identifier
    @param vc_style      - VC style (Mesh/Spoke)
    @param egress       - Peer address
    @param in_lbl             - Incoming label
    @param out_if             - Provider facing interface
    @param out_lbl            - Outgoing label
 */
int
nsm_vpls_vc_fib_entry_add_process (u_int32_t vr_id, char* vpls_id_str,
                                   u_int8_t vc_style, char *egress,
                                   char* in_lbl, char* out_if, char* out_lbl)
{
  struct nsm_master *nm = NULL;
  struct nsm_vpls *vpls = NULL;
  u_int32_t vpls_id = 0;
  int ret = 0;

  nm = nsm_master_lookup_by_id (nzg, vr_id);
  if (! nm)
    return NSM_API_SET_ERR_MASTER_NOT_EXIST;

  /* Process VPLS ID. */
  vpls_id = cmd_str2int (vpls_id_str, &ret);

  /* VPLS validation. */
  vpls = nsm_vpls_lookup_by_id (nm, vpls_id);
  if ( !vpls)
    return NSM_ERR_VPLS_NOT_FOUND;

  return nsm_vpls_fib_entry_create (nm, vpls, vc_style, egress,
                                    in_lbl, out_if, out_lbl);
}

/* CLI for Static VPLS fib Entry */
CLI (nsm_vpls_fib_entry_add,
     nsm_vpls_fib_entry_add_cmd,
     "vpls fib-entry VPLS-ID (peer A.B.C.D| spoke-vc VC-NAME) IN-LABEL"
     " OUT-INTF OUT-LABEL",
     CONFIG_VPLS_STR,
     "Create a VPLS Fib entry",
     "VPLS Identifier",
     "Mesh Peer",
     "Peer IPv4 Address",
     "Spoke Virtual Circuit",
     "Virtual Circuit Name",
     "In-coming Label <16-1048575>",
     "Provider facing Interface",
     "Out-going Label <16-1048575>")
{
  struct apn_vr *vr = NULL;
  struct interface *ifp = NULL;
  u_int8_t vc_style = 0;
  int ret = 0;

  if ((cli == NULL) || (cli->vr == NULL))
    return CLI_ERROR;

  vr = cli->vr;

  /* Validate interfaces. */
  ifp = if_lookup_by_name (&vr->ifm, argv[4]);
  if (ifp == NULL)
    {
      cli_out (cli, "%% Provider-facing interface is not valid \n");
      return CLI_ERROR;
    }

  /* Identify VC style. */
  if (argv[1][0] == 'p')
    vc_style = MPLS_VC_STYLE_VPLS_MESH;
  else
    vc_style = MPLS_VC_STYLE_VPLS_SPOKE;

  ret = nsm_vpls_vc_fib_entry_add_process (cli->vr->id, argv[0], vc_style,
                                           argv[2], argv[3],
                                           argv[4], argv[5]);
  return nsm_cli_return (cli, ret);
}

/** @brief Function to delete vpls fib entry.
    @param nm         - NSM master
    @param vpls       - struct vpls
    @param vc_style   - VC style (Mesh/Spoke)
    @param egress     - Peer address
 */
int
nsm_vpls_fib_delete_entry (struct nsm_master *nm, struct nsm_vpls *vpls,
                           u_int8_t vc_style, char* egress)
{
  struct nsm_msg_vc_fib_delete dmsg;
  struct nsm_vpls_peer *peer = NULL;
  struct nsm_vpls_spoke_vc *svc = NULL;
  struct addr_in addr;
  int ret = 0;

  pal_mem_set (&dmsg, 0, sizeof (struct nsm_msg_vc_fib_delete));

  dmsg.vpn_id = vpls->vpls_id;
  addr.afi = AFI_IP;

  if (vc_style == MPLS_VC_STYLE_VPLS_MESH)
    {
      ret = pal_inet_pton (AF_INET, egress, &addr.u.ipv4);
      if (ret < 0)
        return NSM_ERR_INVALID_ARGS;

      peer = nsm_vpls_mesh_peer_lookup (vpls, &addr, NSM_FALSE);
      if (peer == NULL)
        return NSM_ERR_VPLS_PEER_NOT_FOUND;

      dmsg.vc_style = MPLS_VC_STYLE_VPLS_MESH;
      dmsg.addr = addr;
      dmsg.vc_id = vpls->vpls_id;
    }
  else if (vc_style == MPLS_VC_STYLE_VPLS_SPOKE)
    {
       svc = nsm_vpls_spoke_vc_lookup_by_name (vpls, egress, NSM_FALSE);
       if (! svc)
         return NSM_ERR_SPOKE_VC_NOT_FOUND;

       dmsg.vc_style = MPLS_VC_STYLE_VPLS_SPOKE;
       dmsg.vc_id = svc->vc->id;
    }
  else
    return NSM_ERR_INVALID_ARGS;

  return nsm_vpls_fib_delete_msg_process (nm, &dmsg);
}

/** @brief API to delete Static VPLS fib entry.
    @param vr_id              - VR identifier
    @param vpls_id_str        - VPLS Identifier
    @param vc_style           - VC style (Mesh/Spoke)
    @param egress             - Peer address
    @param in_lbl             - Incoming label
    @param out_if             - Provider facing interface
    @param out_lbl            - Outgoing label
    @param Params             - Used to identify whether all the parameters
                                are provided for deleting the entry
 */
int
nsm_vpls_vc_fib_entry_delete_process (u_int32_t vr_id, char* vpls_id_str,
                                      u_int8_t vc_style, char* egress,
                                      char* in_lbl_str, char* if_out,
                                      char* out_lbl_str, u_char params)
{
  struct nsm_master *nm = NULL;
  struct nsm_vpls *vpls = NULL;
  struct nsm_vpls_peer *peer = NULL;
  struct nsm_vpls_spoke_vc *svc = NULL;
  struct vc_fib *vc_fib = NULL;
  struct addr_in addr;
  s_int8_t fec_type_vc = 0;
  u_int32_t vpls_id = 0;
  int ret = 0;

  nm = nsm_master_lookup_by_id (nzg, vr_id);
  if (! nm)
    return NSM_API_SET_ERR_MASTER_NOT_EXIST;

  vpls_id = cmd_str2int (vpls_id_str, &ret);

  if (vpls_id < VPLS_ID_MIN || vpls_id > VPLS_ID_MAX)
    return NSM_ERR_INVALID_VPLS_ID;

  vpls = nsm_vpls_lookup_by_id (nm, vpls_id);
  if ( !vpls)
    return NSM_ERR_VPLS_NOT_FOUND;

  if (vc_style == MPLS_VC_STYLE_VPLS_MESH)
    {
      pal_mem_set (&addr, 0, sizeof (struct addr_in));
      addr.afi = AFI_IP;

      ret = pal_inet_pton (AF_INET, egress, &(addr.u.ipv4));
      if (ret <= 0)
        return NSM_ERR_INVALID_ARGS;

      peer = nsm_vpls_mesh_peer_lookup (vpls, &addr, NSM_FALSE);
      if (! peer)
        return NSM_ERR_VPLS_PEER_NOT_FOUND;

      vc_fib = peer->vc_fib;
      fec_type_vc = peer->fec_type_vc;
    }
  else if (vc_style == MPLS_VC_STYLE_VPLS_SPOKE)
    {
      svc = nsm_vpls_spoke_vc_lookup_by_name (vpls, egress, NSM_FALSE);
      if (! svc)
        return NSM_ERR_SPOKE_VC_NOT_FOUND;

      vc_fib = svc->vc_fib;
      fec_type_vc = svc->vc->fec_type_vc;
    }

      /* Identify whether VPLS entry is configured to use static fib entry. */
      if (fec_type_vc != PW_OWNER_MANUAL)
        return NSM_ERR_VPLS_PEER_PW_MANUAL_NOT_CONFIGURED;

      /* Validate VPLS entry fib. */
      if (vc_fib == NULL)
        return NSM_ERR_VPLS_FIB_ENTRY_NOT_FOUND;

  /*
   * Validation done only when all the parameters are provided for
   * deleting the entry.
   */
  if (params == CLI_MAX_PARAMS_ENTRY_DELETE)
    {
      struct interface *ifp_out = NULL;
      u_int32_t in_label, out_label;

      in_label = cmd_str2int (in_lbl_str, &ret);
      if (vc_fib->in_label != in_label)
        return NSM_ERR_VPLS_INCOMING_LABEL_MISMATCH;

      out_label = cmd_str2int (out_lbl_str, &ret);
      if (vc_fib->out_label != out_label)
        return NSM_ERR_VPLS_OUTGOING_LABEL_MISMATCH;

      ifp_out = if_lookup_by_name (&nm->vr->ifm, if_out);

      if (ifp_out)
        {
#ifdef HAVE_GMPLS
          if (vc_fib->nw_if_ix != ifp_out->gifindex)
#else
          if (vc_fib->nw_if_ix != ifp_out->ifindex)
#endif /* HAVE_GMPLS */
            return NSM_ERR_VPLS_OUTGOING_INTERFACE_MISMATCH;
        }
    }

  return nsm_vpls_fib_delete_entry (nm, vpls, vc_style, egress);
}

/* Static CLI for VPLS fib entry delete */
CLI (nsm_vpls_fib_entry_delete,
     nsm_vpls_fib_entry_delete_label_cmd,
     "no vpls fib-entry VPLS-ID ((peer A.B.C.D)| (spoke-vc VC-NAME)) IN-LABEL"
     " OUT-INTF OUT-LABEL",
     CLI_NO_STR,
     CONFIG_VPLS_STR,
     "Delete a VPLS Fib entry",
     "VPLS Identifier",
     "Mesh Peer IPv4 Address",
     "Spoke Virtual Circuit",
     "Virtual Circuit Name",
     "In-coming Label <16-1048575>",
     "Provider facing Interface",
     "Out-going Label <16-1048575>")
{
  struct apn_vr *vr = NULL;
  struct interface *ifp = NULL;
  u_char cli_params = CLI_MIN_PARAMS_ENTRY_DELETE;
  u_int8_t vc_style = 0;
  int ret = 0;
  if ((cli == NULL) || (cli->vr == NULL))
    return CLI_ERROR;

  vr = cli->vr;

  /* Validate interfaces. */
  if (argc > 3)
    {
      ifp = if_lookup_by_name (&vr->ifm, argv[4]);
      if (ifp == NULL)
        {
          cli_out (cli, "%% Outgoing interface is not valid \n");
          return CLI_ERROR;
        }
      cli_params = CLI_MAX_PARAMS_ENTRY_DELETE;
    }

  if (argv[1][0] == 'p')
    vc_style = MPLS_VC_STYLE_VPLS_MESH;
  else
    vc_style = MPLS_VC_STYLE_VPLS_SPOKE;

  ret = nsm_vpls_vc_fib_entry_delete_process (cli->vr->id, argv[0], vc_style,
                                               argv[2], argv[3], argv[4],
                                               argv[5], cli_params);
  return nsm_cli_return (cli, ret);
}

ALI (nsm_vpls_fib_entry_delete,
     nsm_vpls_fib_entry_delete_cmd,
     "no vpls fib-entry VPLS-ID ((peer A.B.C.D)| (spoke-vc VC-NAME))",
     CLI_NO_STR,
     CONFIG_VPLS_STR,
     "Delete a VPLS Fib entry",
     "VPLS Identifier",
     "Mesh Peer IPv4 Address",
     "Spoke Virtual Circuit",
     "Virtual Circuit Name");
#endif /* HAVE_VPLS */

int
nsm_mpls_map_route_func (struct cli *cli, char *route_str, char *rmask_str,
                         char *fec_str, char *fmask_str)
{
  struct nsm_master *nm = cli->vr->proto;
  struct prefix_ipv4 fec;
  struct fec_gen_entry route;
  struct pal_in4_addr rmask, fmask;
  int ret;

  /* Figure out route. */
  ret = str2prefix_ipv4 (route_str, (struct prefix_ipv4 *) &route.u.prefix);
  if (ret <= 0)
    {
      cli_out (cli, "%% Malformed address\n");
      return CLI_ERROR;
    }
  if (rmask_str)
    {
      ret = pal_inet_pton (AF_INET, rmask_str, &rmask);
      if (ret <= 0)
        {
          cli_out (cli, "%% Malformed route mask\n");
          return CLI_ERROR;
        }

      route.u.prefix.prefixlen = ip_masklen (rmask);
    }

  route.type = gmpls_entry_type_ip;

  /* Figure out FEC. */
  ret = str2prefix_ipv4 (fec_str, &fec);
  if (ret <= 0)
    {
      cli_out (cli, "%% Malformed address\n");
      return CLI_ERROR;
    }
  if (fmask_str)
    {
      ret = pal_inet_pton (AF_INET, fmask_str, &fmask);
      if (ret <= 0)
        {
          cli_out (cli, "%% Malformed FEC mask\n");
          return CLI_ERROR;
        }
      fec.prefixlen = ip_masklen (fmask);
    }

  ret = nsm_gmpls_api_add_mapped_route (nm, &route, (struct prefix *)&fec);
  if (ret != NSM_SUCCESS)
    {
      switch (ret)
        {
        case NSM_FAILURE:
          cli_out (cli, "%% Could not map %P to %P\n",
                   &route, &fec);
          break;
        case NSM_ERR_MEM_ALLOC_FAILURE:
          cli_out (cli, "%% Memory allocation failure\n");
          break;
        default:
          break;
        }
      return CLI_ERROR;
    }

  return CLI_SUCCESS;
}

CLI (mapped_route_add,
     mapped_route_add_cmd,
     "mpls map-route A.B.C.D A.B.C.D A.B.C.D A.B.C.D",
     CONFIG_MPLS_STR,
     "Map an IPv4 route",
     "IPv4 prefix to be mapped",
     "Mask for IPv4 address to be mapped",
     "IPv4 Forwarding Equivalence Class for route to be mapped to",
     "Mask for IPv4 Forwarding Equivalence Class")
{
  return nsm_mpls_map_route_func (cli, argv[0], argv[1], argv[2], argv[3]);
}

CLI (mapped_route_add_mask,
     mapped_route_add_mask_cmd,
     "mpls map-route A.B.C.D/M A.B.C.D/M",
     CONFIG_MPLS_STR,
     "Map an IPv4 route",
     "IPv4 prefix to be mapped, plus mask",
     "IPv4 FEC for route to be mapped to, plus mask")
{
  return nsm_mpls_map_route_func (cli, argv[0], NULL, argv[1], NULL);
}

int
nsm_mpls_unmap_route_func (struct cli *cli, char *route_str, char *rmask_str,
                           char *fec_str, char *fmask_str)
{
  struct nsm_master *nm = cli->vr->proto;
  struct prefix_ipv4 route, fec;
  struct fec_gen_entry fentry;
  struct pal_in4_addr rmask, fmask;
  int ret;

  /* Figure out route. */
  ret = str2prefix_ipv4 (route_str, &route);
  if (ret <= 0)
    {
      cli_out (cli, "%% Malformed address\n");
      return CLI_ERROR;
    }
  if (rmask_str)
    {
      ret = pal_inet_pton (AF_INET, rmask_str, &rmask);
      if (ret <= 0)
        {
          cli_out (cli, "%% Malformed route mask\n");
          return CLI_ERROR;
        }
      route.prefixlen = ip_masklen (rmask);
    }

  /* Figure out FEC. */
  ret = str2prefix_ipv4 (fec_str, &fec);
  if (ret <= 0)
    {
      cli_out (cli, "%% Malformed address\n");
      return CLI_ERROR;
    }
  if (fmask_str)
    {
      ret = pal_inet_pton (AF_INET, fmask_str, &fmask);
      if (ret <= 0)
        {
          cli_out (cli, "%% Malformed FEC mask\n");
          return CLI_ERROR;
        }
      fec.prefixlen = ip_masklen (fmask);
    }

  fentry.type = gmpls_entry_type_ip;
  *(struct prefix_ipv4 *) &fentry.u.prefix = route;
  ret = nsm_gmpls_api_del_mapped_route (nm, &fentry, (struct prefix *)&fec);
  if (ret != NSM_SUCCESS)
    {
      switch (ret)
        {
        case NSM_ERR_NOT_FOUND:
          cli_out (cli, "%% Entry not found\n");
          break;
        case NSM_ERR_INVALID_ARGS:
          cli_out (cli, "%% Route %P not mapped to %P\n", &route, &fec);
          break;
        }
      return CLI_ERROR;
    }

  return CLI_SUCCESS;
}

CLI (mapped_route_del,
     mapped_route_del_cmd,
     "no mpls map-route A.B.C.D A.B.C.D A.B.C.D A.B.C.D",
     CLI_NO_STR,
     NO_CONFIG_MPLS_STR,
     "Unmap an IPv4 route",
     "IPv4 prefix to be unmapped",
     "Mask for IPv4 address to be unmapped",
     "IPv4 Forwarding Equivalence Class for route to be unmapped to",
     "Mask for IPv4 Forwarding Equivalence Class")
{
  return nsm_mpls_unmap_route_func (cli, argv[0], argv[1], argv[2], argv[3]);
}

CLI (mapped_route_del_mask,
     mapped_route_del_mask_cmd,
     "no mpls map-route A.B.C.D/M A.B.C.D/M",
     CLI_NO_STR,
     NO_CONFIG_MPLS_STR,
     "Unmap an IPv4 route",
     "IPv4 prefix to be unmapped, plus mask",
     "IPv4 FEClass for route to be unmapped to, plus mask")
{
  return nsm_mpls_unmap_route_func (cli, argv[0], NULL, argv[1], NULL);
}


CLI (lsp_tunneling,
     lsp_tunneling_cmd,
     "mpls lsp-tunneling IFNAME <16-1048575> <16-1048575> A.B.C.D/M",
     CONFIG_MPLS_STR,
     "Tunnel a transit LSP",
     "Incoming interface name",
     "Incoming Label Value between 16 and 1048575",
     "Outgoing label of the transit lsp (used for label swap operation)",
     "IPv4 network FEC for identifying tunnel LSP")
{
  struct nsm_master *nm = cli->vr->proto;
  struct interface *ifp;
  u_int32_t in_label, out_label;
  struct prefix pfx;
  int ret;

  pal_mem_set (&pfx, 0, sizeof (struct prefix));
  /* Incoming interface. */
  ifp = if_lookup_by_name (&cli->vr->ifm, argv[0]);
  if (nsm_mpls_check_valid_interface (ifp, SWAP) != NSM_TRUE)
    {
      cli_out (cli, "%% Incoming interface %p is not valid\n", ifp);
      return CLI_ERROR;
    }

  /* Get incoming label. */
  CLI_GET_UINT32_RANGE ("in-label", in_label, argv[1],
                        LABEL_VALUE_INITIAL, LABEL_VALUE_MAX);

  /* Get outgoing label. */
  CLI_GET_UINT32_RANGE ("out-label", out_label, argv[2],
                        LABEL_VALUE_INITIAL, LABEL_VALUE_MAX);

  /* get fec prefix */
  ret = str2prefix (argv[3], &pfx);
  if (ret <= 0)
    {
      cli_out (cli, "%% Malformed address\n");
      return CLI_ERROR;
    }

  ret = nsm_gmpls_api_add_lsp_tunnel (nm, ifp, in_label, out_label, &pfx);
  if (ret != NSM_SUCCESS)
    {
      cli_out (cli, "%% Failed to add lsp tunnel \n");
      return CLI_ERROR;
    }

  return CLI_SUCCESS;
}



CLI (no_lsp_tunneling,
     no_lsp_tunneling_cmd,
     "no mpls lsp-tunneling IFNAME <16-1048575>",
     CLI_NO_STR,
     CONFIG_MPLS_STR,
     "Tunnel a transit LSP",
     "Incoming interface name",
     "Incoming Label Value between 16 and 1048575")
{
  struct nsm_master *nm = cli->vr->proto;
  struct interface *ifp;
  u_int32_t in_label;
  int ret;

  /* Incoming interface. */
  ifp = if_lookup_by_name (&cli->vr->ifm, argv[0]);
  if (nsm_mpls_check_valid_interface (ifp, SWAP) != NSM_TRUE)
    {
      cli_out (cli, "%% Incoming interface %p is not valid\n", ifp);
      return CLI_ERROR;
    }

  /* Get incoming label. */
  CLI_GET_UINT32_RANGE ("in-label", in_label, argv[1],
                        LABEL_VALUE_INITIAL, LABEL_VALUE_MAX);

  ret = nsm_mpls_api_del_lsp_tunnel (nm, ifp, in_label);
  if (ret != NSM_SUCCESS)
    {
      cli_out (cli, "%% Failed to delete lsp tunnel \n");
      return CLI_ERROR;
    }

  return CLI_SUCCESS;
}


#ifdef HAVE_VPLS
int
nsm_vpls_static_config_write (struct cli *cli)
{
  struct nsm_master *nm = cli->vr->proto;
  struct ptree_node *pn = NULL;
  struct route_node *rn = NULL;
  struct listnode *ln = NULL;
  struct nsm_vpls *vpls = NULL;
  struct nsm_vpls_peer *peer = NULL;
  struct nsm_vpls_spoke_vc *svc = NULL;
  struct interface *ifp_out = NULL;

  if (! NSM_MPLS || ! NSM_MPLS->vpls_table)
    return 0;

  for (pn = ptree_top (NSM_MPLS->vpls_table); pn; pn = ptree_next (pn))
    {
      if (! pn->info)
        continue;

      vpls = pn->info;
      if (! vpls->mp_table)
        continue;

      /* Lookup for VPLS Static Mesh Fib entries. */
      for (rn = route_top (vpls->mp_table); rn; rn = route_next (rn))
        {
          if (! rn->info)
            continue;

          peer = rn->info;

          if (peer->peer_addr.afi == AFI_IP)
            {
              if ((peer->fec_type_vc == PW_OWNER_MANUAL) &&
                  (peer->vc_fib != NULL))
                {

                  NSM_MPLS_GET_INDEX_PTR (PAL_FALSE, peer->vc_fib->nw_if_ix,
                                          ifp_out);

                  cli_out (cli, "vpls fib-entry %u peer %r %d %s %d\n",
                           vpls->vpls_id, &peer->peer_addr.u.ipv4,
                           peer->vc_fib->in_label,
                           ((ifp_out == NULL) ? "N/A" : ifp_out->name),
                           peer->vc_fib->out_label);
                }
            }
        }

      /*Lookup for VPLS static Spoke VC fib entries. */
      LIST_LOOP (vpls->svc_list, svc, ln)
        {
          if (svc->vc == NULL)
            continue;

          if ((svc->vc->fec_type_vc == PW_OWNER_MANUAL)
              && (svc->vc_fib != NULL))
            {

              NSM_MPLS_GET_INDEX_PTR (PAL_FALSE, svc->vc_fib->nw_if_ix,
                                      ifp_out);

              cli_out (cli, "vpls fib-entry %u spoke-vc %s %d %s %d\n",
                       vpls->vpls_id, svc->vc_name,
                       svc->vc_fib->in_label,
                       ((ifp_out == NULL) ? "N/A" : ifp_out->name),
                       svc->vc_fib->out_label);

            }
        }
    }
return 0;
}

int
nsm_vpls_config_write (struct cli *cli)
{
  struct nsm_master *nm = cli->vr->proto;
  struct ptree_node *pn;
  struct route_node *rn;
  struct nsm_vpls *vpls;
  struct nsm_vpls_peer *peer;
  struct nsm_vpls_spoke_vc *svc;
  struct listnode *ln;

  if (! NSM_MPLS || ! NSM_MPLS->vpls_table)
    return 0;

  for (pn = ptree_top (NSM_MPLS->vpls_table); pn; pn = ptree_next (pn))
    {
      if (! pn->info)
        continue;

      vpls = pn->info;

      cli_out (cli, "mpls vpls %s %u\n",
               vpls->vpls_name, vpls->vpls_id);

      if (CHECK_FLAG (vpls->config, NSM_VPLS_CONFIG_GROUP_ID))
        {
          struct nsm_mpls_vc_group* group;

          group = nsm_mpls_l2_vc_group_lookup_by_id (nm, vpls->grp_id);

          if ( group && group->name )
            cli_out (cli, " vpls-ac-group %s\n", group->name);
        }

      if (CHECK_FLAG (vpls->config, NSM_VPLS_CONFIG_TYPE))
        cli_out (cli, " vpls-type %s\n",
                 mpls_vc_type_to_str2 (vpls->vpls_type));
      if (CHECK_FLAG (vpls->config, NSM_VPLS_CONFIG_MTU))
        cli_out (cli, " vpls-mtu %d\n", vpls->ifmtu);
      if (vpls->desc)
        cli_out (cli, " vpls-description %s\n", vpls->desc);

      if (! vpls->mp_table)
        continue;
      for (rn = route_top (vpls->mp_table); rn; rn = route_next (rn))
        {
          if (! rn->info)
            continue;

          peer = rn->info;

          if (peer->peer_addr.afi == AFI_IP)
            {
              if (peer->peer_mode == NSM_VPLS_PEER_PRIMARY)
                {
                  if (peer->secondary)
                    cli_out (cli, " vpls-peer %r secondary %r\n",
                             &peer->peer_addr.u.ipv4,
                             &peer->secondary->peer_addr.u.ipv4);
                  else
                    {
                      cli_out (cli, " vpls-peer %r", &peer->peer_addr.u.ipv4);

                      if (peer->mapping_type == MPLS_VC_MAPPING_TUNNEL_ID)
                        {
                          cli_out (cli, " tunnel-id %d", peer->tunnel_id);

                          if (peer->tunnel_dir == TUNNEL_DIR_REV)
                            cli_out (cli, " reverse");
                          /* else - Default Value 
                            cli_out (cli, " forward"); */
                        }

                      if (peer->fec_type_vc == PW_OWNER_MANUAL)
                        cli_out (cli, " manual");

                      cli_out (cli, "\n");
                    }
                }
              else
                continue;
            }
#ifdef HAVE_IPV6
          else if (peer->peer_addr.afi == AFI_IP6)
            cli_out (cli, " vpls-peer %40R\n", &peer->peer_addr.u.ipv6);
#endif /* HAVE_IPV6 */
        }

      LIST_LOOP (vpls->svc_list, svc, ln)
        {
          if (svc->svc_mode == NSM_MPLS_VC_PRIMARY)
            {
              cli_out (cli, " vpls-vc %s ", svc->vc_name);
              if (svc->secondary)
                cli_out (cli, "secondary %s ", svc->secondary->vc_name);
              if (svc->vc_type == VC_TYPE_ETH_VLAN)
                cli_out (cli, "%s ", "vlan");
              else
                cli_out (cli, "%s ", "ethernet");
              cli_out (cli, "\n");

             }
        }
      cli_out (cli, "!\n");
    }

  return 0;
}
#endif /* HAVE_VPLS */


/* Write MPLS specific cli commands to desired output. */
int
nsm_mpls_config_write (struct cli *cli)
{
  struct nsm_master *nm = cli->vr->proto;
#ifdef HAVE_PACKET
  struct ptree_node *pn;
  struct fec_gen_entry fec;
  struct mapped_route *mroute;
#endif /* HAVE_PACKET */
#ifdef HAVE_MPLS_VC
  struct route_node *rn;
  struct nsm_mpls_circuit *vc;
#endif /* HAVE_MPLS_VC */
#if defined (HAVE_BFD) && defined (HAVE_MPLS_OAM)
  struct nsm_mpls_bfd_fec_conf *bfd_conf = NULL;
  struct listnode *ln = NULL;
  u_int16_t i;
#endif /* HAVE_BFD && HAVE_MPLS_OAM */

  if (! NSM_MPLS)
    return 0;

#ifdef HAVE_PACKET
  /* Mapped routes. */
  for (pn = ptree_top (NSM_MPLS->mapped_routes); pn; pn = ptree_next (pn))
    {
      if ((mroute = pn->info) != NULL)
        {
          nsm_gmpls_set_fec_from_mapped_route (pn, &fec);
          cli_out (cli, "mpls map-route %O ", &fec.u.prefix);
          cli_out (cli, "%O\n",  &mroute->fec);
        }
    }
#endif /* HAVE_PACKET */

#ifdef HAVE_TE
  {
    int i;

    /* Print admin groups. */
    for (i = 0; i < ADMIN_GROUP_MAX; i++)
      if (NSM_MPLS->admin_group_array [i].val != ADMIN_GROUP_UNDEF)
        cli_out (cli, "mpls admin-group %s %d\n",
                 NSM_MPLS->admin_group_array [i].name,
                 NSM_MPLS->admin_group_array [i].val);
  }
#endif /* HAVE_TE */

  /* Enable all. */
  if (CHECK_FLAG (NSM_MPLS->config, NSM_MPLS_CONFIG_ENABLE_ALL_IFS))
    cli_out (cli, "mpls enable-all-interfaces\n");

#if (!defined (HAVE_GMPLS) || defined (HAVE_PACKET))
  /* Set the minimum label-pool size. */
  if (CHECK_FLAG (NSM_MPLS->config, NSM_MPLS_CONFIG_MIN_LABEL_VAL))
    cli_out (cli, "mpls min-label-value %d\n",
             NSM_MPLS->min_label_val);

  /* Set the maximum label-pool size. */
  if (CHECK_FLAG (NSM_MPLS->config, NSM_MPLS_CONFIG_MAX_LABEL_VAL))
    cli_out (cli, "mpls max-label-value %d\n",
             NSM_MPLS->max_label_val);
#endif /* !HAVE_GMPLS || HAVE_PACKET */

  {
    struct nsm_label_space *nls;
    struct route_node *rn;
    char *serv_str = NULL;
    int ctr;

    for (rn = route_top (NSM_MPLS->ls_table); rn; rn = route_next (rn))
      {
        nls = rn->info;
        if (! nls)
          continue;

        if (CHECK_FLAG (nls->config, NSM_LABEL_SPACE_CONFIG_MIN_LABEL_VAL))
          cli_out (cli, "mpls min-label-value %d label-space %d\n",
                   nls->min_label_val, nls->label_space);

        if (CHECK_FLAG (nls->config, NSM_LABEL_SPACE_CONFIG_MAX_LABEL_VAL))
          cli_out (cli, "mpls max-label-value %d label-space %d\n",
                   nls->max_label_val, nls->label_space);

        if ((CHECK_FLAG (nls->config, NSM_LABEL_SPACE_CONFIG_MIN_LABEL_RANGE))
                                      ||
            (CHECK_FLAG (nls->config, NSM_LABEL_SPACE_CONFIG_MAX_LABEL_RANGE)))
          {
            for (ctr = LABEL_POOL_RANGE_INITIAL;
                 ctr < LABEL_POOL_RANGE_MAX; ctr++)
              {
                /* Skip unsupported cases - LDP-PW and LDP-VPLS */
                if ((ctr == 2) || (ctr == 3))
                 continue;

                serv_str = nsm_gmpls_get_lbl_range_service_name(ctr);

                if (nls->service_ranges[ctr].from_label !=
                                                   LABEL_UNINITIALIZED_VALUE)
                  {
                    cli_out (cli, "mpls %s min-label-value %d label-space %d\n",
                              serv_str, nls->service_ranges[ctr].from_label,
                              nls->label_space);
                  }
                if (nls->service_ranges[ctr].to_label !=
                                                   LABEL_UNINITIALIZED_VALUE)
                  {
                    cli_out (cli, "mpls %s max-label-value %d label-space %d\n",
                             serv_str, nls->service_ranges[ctr].to_label,
                             nls->label_space);
                  }
              }
          }
      }
  }

#ifdef HAVE_MPLS_VC
  if ( NSM_MPLS->vc_group_list)
    {
      struct nsm_mpls_vc_group *group;
      struct listnode *ln;

      LIST_LOOP (NSM_MPLS->vc_group_list, group, ln)
        {
          cli_out (cli, "mpls ac-group %s %u\n", group->name, group->id );
        }
    }

  /* VC data. */
    for (rn = route_top (NSM_MPLS->vc_table); rn; rn = route_next (rn))
      {
        /* If no data, skip. */
        if (! rn->info)
          continue;

        /* Get vc. */
        vc = rn->info;

        /* Dump. */
        cli_out (cli, "mpls l2-circuit %s %u %r", vc->name, vc->id,
                 &vc->address.u.prefix4);

        if (vc->group && vc->group->name)
          cli_out (cli, " %s", vc->group->name);

        if (vc->cw)
          cli_out (cli, " control-word");

        if (vc->mapping_type != MPLS_VC_MAPPING_NONE)
          {
            if (vc->tunnel_name)
              cli_out (cli, " tunnel-name %s", vc->tunnel_name);
            else /* if (vc->tunnel_id != 0) */
              cli_out (cli, " tunnel-id %d", vc->tunnel_id);

            if (vc->tunnel_dir == TUNNEL_DIR_REV)
              cli_out (cli, " reverse");
            /* else - Default value 
              cli_out (cli, " forward"); */
          }

#ifdef HAVE_MS_PW
        if (vc->ms_pw_role == NSM_MSPW_ROLE_PASSIVE)
          cli_out(cli, " %s", "passive");
        else
#endif /* HAVE_MS_PW */
        cli_out(cli, " %s",
               (vc->fec_type_vc == PW_OWNER_MANUAL) ? "manual " : "");

#ifdef HAVE_VCCV
        if (vc->cc_types > 0)
          {
            cli_out (cli, " %s", "vccv");
            if (vc->cc_types != nsm_mpls_get_default_cc_types (vc->cw))
              cli_out (cli, " cc-type %s",
                                   nsm_mpls_get_cctype_from_bit(vc->cc_types));

            if (nsm_mpls_is_bfd_set(vc->cv_types))
              {
                cli_out (cli," %s", "bfd");
                if (nsm_mpls_is_specific_bfd_in_use(vc->cv_types,
                                  (vc->fec_type_vc == PW_OWNER_MANUAL),
                                   vc->cw))
                  {
                    cli_out (cli, " bfd-cv-type %s",
                               nsm_mpls_get_bfd_cvtype_from_bit (vc->cv_types));
                  }
              }
          }
#endif /* HAVE_VCCV */
        cli_out (cli, "\n");
      }
#ifdef HAVE_MS_PW
      if (NSM_MS_PW_HASH != NULL)
        {
          hash_iterate (NSM_MS_PW_HASH,
                        ((void (*) (struct hash_backet *, void *))\
                         nsm_ms_pw_show_iterator),
                        cli);
        }
#endif /* HAVE_MS_PW */
#endif /* HAVE_MPLS_VC */
#if (!defined (HAVE_GMPLS) || defined (HAVE_PACKET))
#ifdef HAVE_DSTE
  {
    int i;

    /* Class Type configuration. */
    for (i = 0; i < MAX_CLASS_TYPE; i++)
      if (NSM_MPLS->ct_name[i][0] != '\0')
  cli_out (cli, "mpls class-type ct%d %s\n",
     i, NSM_MPLS->ct_name[i]);

    /* TE Class configuration. */
    for (i = 0; i < MAX_TE_CLASS; i++)
      if (NSM_MPLS->te_class[i].ct_num != CT_NUM_INVALID)
  cli_out (cli, "mpls te-class te%d %s %d\n",
     i,
     NSM_MPLS->ct_name[NSM_MPLS->te_class[i].ct_num],
     NSM_MPLS->te_class[i].priority);
  }
#endif /* HAVE_DSTE */

#ifdef HAVE_DIFFSERV
  {
    int i;

    if (NSM_MPLS->lsp_model == LSP_MODEL_PIPE)
      cli_out (cli, "mpls lsp-model pipe\n");

    if (CHECK_FLAG (NSM_MPLS->config,  NSM_MPLS_CONFIG_SUPPORTED_DSCP))
      {
        /* Best Effort check. */
        if (NSM_MPLS->supported_dscp[DIFFSERV_BEST_EFFORT] ==
            DIFFSERV_DSCP_NOT_SUPPORTED)
          cli_out (cli, "no mpls support-diffserv-class %s\n",
                   diffserv_class_name(DIFFSERV_BEST_EFFORT));

        /* Rest of the supported DSCP data, if any. */
        for (i = 1; i < DIFFSERV_MAX_SUPPORTED_DSCP; i++)
          {
            if (NSM_MPLS->supported_dscp[i] == DIFFSERV_DSCP_SUPPORTED)
              cli_out (cli, "mpls support-diffserv-class %s\n",
                       diffserv_class_name(i));
          }
      }

    if (CHECK_FLAG (NSM_MPLS->config,  NSM_MPLS_CONFIG_DSCP_EXP_MAP))
      {
        /* Output of the value. */
        if ((NSM_MPLS->dscp_exp_map[0] != DIFFSERV_BEST_EFFORT) &&
            (NSM_MPLS->dscp_exp_map[0] != DIFFSERV_INVALID_DSCP))
          cli_out (cli, "mpls class-to-exp-bit %s %d\n",
                   diffserv_class_name(NSM_MPLS->dscp_exp_map[0]),
                   0);
        else if  (NSM_MPLS->dscp_exp_map[0] == DIFFSERV_INVALID_DSCP)
          cli_out (cli, "no mpls class-to-exp-bit be 0\n");

        for (i = 1; i < DIFFSERV_MAX_DSCP_EXP_MAPPINGS; i++)
          if (NSM_MPLS->dscp_exp_map[i] != DIFFSERV_INVALID_DSCP)
            cli_out (cli, "mpls class-to-exp-bit %s %d\n",
                     diffserv_class_name(NSM_MPLS->dscp_exp_map[i]),
                     i);
      }
  }
#endif /* HAVE_DIFFSERV */
#endif /* !HAVE_GMPLS || HAVE_PACKET */

#ifdef HAVE_MPLS_OAM_ITUT
 {
   struct nsm_mpls_oam_itut *oam;
   struct listnode *ln;
   int i;

   if (!LIST_ISEMPTY(NSM_MPLS->mpls_oam_list))
   LIST_LOOP(NSM_MPLS->mpls_oam_itut_list, oam, ln)
   {
     if (! LIST_ISEMPTY (oam->oam_req_list) )
     {
       switch (oam->msg.pkt_type)
        {
          case NSM_MPLS_CV_PKT:
               cli_out(cli, "mpls CV");
               if (oam->msg.flag == NSM_MPLS_OAM_PKT_START)
                {
                  cli_out(cli, "start lsp-id %d ", oam->msg.lsp_id);
                  if (oam->msg.ttl)
                   {
                    cli_out(cli, "ttl %d", oam->msg.ttl);
                   }
                  else if (oam->msg.timeout)
                   {
                     cli_out(cli, "timeout %d", oam->msg.timeout);
                   }
                  if (oam->msg.lsr_type == NSM_MPLS_OAM_SINK_LSR)
                   {
                     cli_out(cli,"sink-lsr src-lsr-ip %r\n",oam->msg.lsr_addr);
                   }
                }
               if (oam->msg.flag == NSM_MPLS_OAM_PKT_STOP)
                {
                  cli_out(cli, "stop lsp-id %d \n", oam->msg.lsp_id);
                }
               cli_out (cli, "!\n");
              break;

          case NSM_MPLS_FFD_PKT:
               cli_out(cli, "mpls ffd");
               if (oam->msg.flag == NSM_MPLS_OAM_PKT_START)
                {
                  cli_out(cli, "start frequency %d lsp-id %d ",
                                oam->msg.frequency, oam->msg.lsp_id);
                  if (oam->msg.ttl)
                   {
                    cli_out(cli, "ttl %d", oam->msg.ttl);
                   }
                  else if (oam->msg.timeout)
                   {
                     cli_out(cli, "timeout %d", oam->msg.timeout);
                   }
                  if (oam->msg.lsr_type == NSM_MPLS_OAM_SINK_LSR)
                   {
                     cli_out(cli,"sink-lsr src-lsr-ip %r\n",oam->msg.lsr_addr);
                   }
                }
               if (oam->msg.flag == NSM_MPLS_OAM_PKT_STOP)
                {
                  cli_out(cli, "stop lsp-id %d \n", oam->msg.lsp_id);
                }
               cli_out (cli, "!\n");
               break;

          case NSM_MPLS_FDI_PKT:
               cli_out(cli, "mpls fdi %d", oam->msg.lsp_id);
               if (oam->msg.ttl)
                {
                  cli_out(cli, "ttl %d", oam->msg.ttl);
                }
               else if (oam->msg.timeout)
                {
                  cli_out(cli, "timeout %d", oam->msg.timeout);
                }
                cli_out(cli, "level %d",oam->msg.fdi_level.level_no);
                for ( i = 0; i < oam->msg.fdi_level.level_no; i++)
                 {
                  cli_out(cli,"label%d %d ",oam->msg.fdi_level.lable_id[i]);
                 }
                 cli_out (cli, "!\n");
               break;

          case NSM_MPLS_BDI_PKT:
               cli_out(cli, "mpls bdi dst-lsp-bdi %d src-lsp-bdi %d ",
                             oam->msg.lsp_id, oam->msg.fwd_lsp_id);
               if (oam->msg.ttl)
                {
                  cli_out(cli, "ttl %d ", oam->msg.ttl);
                }
               else if (oam->msg.timeout)
                {
                  cli_out(cli, "timeout %d ", oam->msg.timeout);
                }
                cli_out (cli, "!\n");
               break;
        }
     }
   }
 }
#endif /* HAVE_MPLS_OAM_ITUT */

#if defined (HAVE_BFD) && defined (HAVE_MPLS_OAM)
  LIST_LOOP (NSM_MPLS->bfd_fec_conf_list, bfd_conf, ln)
    {
      cli_out (cli, "mpls bfd");

      if (bfd_conf->lsp_type == BFD_MPLS_LSP_TYPE_LDP)
        {
          cli_out (cli, " ldp %O", &bfd_conf->fec);
        }
      else if (bfd_conf->lsp_type == BFD_MPLS_LSP_TYPE_RSVP)
        {
          cli_out (cli, " rsvp tunnel-name %s", bfd_conf->tunnel_name);
        }
      else
        {
          cli_out (cli, " static %O", &bfd_conf->fec);
        }

      if (CHECK_FLAG (bfd_conf->cflag, NSM_MPLS_BFD_FEC_DISABLE))
        {
          cli_out (cli, " disable\n");
          continue;
        }

      /* Write the BFD attributes. */
      if (bfd_conf->lsp_ping_intvl != BFD_MPLS_LSP_PING_INTERVAL_DEF)
        cli_out (cli, " lsp-ping-intvl %lu", bfd_conf->lsp_ping_intvl);

      if (bfd_conf->min_tx != BFD_MPLS_MIN_TX_INTERVAL_DEF)
        cli_out (cli, " min-tx %lu", bfd_conf->min_tx);

      if (bfd_conf->min_rx != BFD_MPLS_MIN_RX_INTERVAL_DEF)
        cli_out (cli, " min-rx %lu", bfd_conf->min_tx);

      if (bfd_conf->mult != BFD_MPLS_DETECT_MULT_DEF)
        cli_out (cli, " multiplier %lu", bfd_conf->mult);

      if (bfd_conf->force_explicit_null == PAL_TRUE)
        cli_out (cli, " force-explicit-null");

      cli_out (cli, "\n");
    }

  for (i = 0; i < BFD_MPLS_LSP_TYPE_UNKNOWN; i++)
    {
      if (CHECK_FLAG (NSM_MPLS->bfd_flag, nsm_mpls_bfd_lsp_type2flag (i)))
        {
          cli_out (cli, "mpls bfd %s all", nsm_mpls_bfd_lsp_type2name (i));

          if (NSM_MPLS->bfd_lsp_conf [i].lsp_ping_intvl !=
               BFD_MPLS_LSP_PING_INTERVAL_DEF)
            {
              cli_out (cli, " lsp-ping-intvl %lu",
                  NSM_MPLS->bfd_lsp_conf [i].lsp_ping_intvl);
            }

          if (NSM_MPLS->bfd_lsp_conf [i].min_tx != BFD_MPLS_MIN_TX_INTERVAL_DEF)
            cli_out (cli, " min-tx %lu", NSM_MPLS->bfd_lsp_conf [i].min_tx);

          if (NSM_MPLS->bfd_lsp_conf [i].min_rx != BFD_MPLS_MIN_RX_INTERVAL_DEF)
            cli_out (cli, " min-rx %lu", NSM_MPLS->bfd_lsp_conf [i].min_rx);

          if (NSM_MPLS->bfd_lsp_conf [i].mult != BFD_MPLS_DETECT_MULT_DEF)
            cli_out (cli, " multiplier %lu", NSM_MPLS->bfd_lsp_conf [i].mult);

          if (NSM_MPLS->bfd_lsp_conf [i].force_explicit_null != PAL_FALSE)
            cli_out (cli, " force-explicit-null");

          cli_out (cli, "\n");
        }
    }

  cli_out (cli, "!\n");
#endif /* HAVE_BFD && HAVE_MPLS_OAM */

#if (!defined (HAVE_GMPLS) || defined (HAVE_PACKET))

  /* TTL Propagation */
  if (NSM_MPLS->propagate_ttl == TTL_PROPAGATE_ENABLED)
    cli_out (cli, "mpls propagate-ttl\n");

  /* Set ingress ttl value. */
  if (CHECK_FLAG (NSM_MPLS->config, NSM_MPLS_CONFIG_INGRESS_TTL))
    cli_out (cli, "mpls ingress-ttl %d\n", NSM_MPLS->ingress_ttl);

  /* Set egress ttl value. */
  if (CHECK_FLAG (NSM_MPLS->config, NSM_MPLS_CONFIG_EGRESS_TTL))
    cli_out (cli, "mpls egress-ttl %d\n", NSM_MPLS->egress_ttl);
#endif /* !HAVE_GMPLS || HAVE_PACKET */

  /* Local packet handling. */
  if (CHECK_FLAG (NSM_MPLS->config, NSM_MPLS_CONFIG_LCL_PKT_HANDLE))
    cli_out (cli, "mpls local-packet-handling\n");

  /* Logging. */
  if ((CHECK_FLAG (NSM_MPLS->kern_msgs, NSM_MPLS_SHOW_MSG_ERROR)) &&
      (CHECK_FLAG (NSM_MPLS->kern_msgs, NSM_MPLS_SHOW_MSG_WARNING)) &&
      (CHECK_FLAG (NSM_MPLS->kern_msgs, NSM_MPLS_SHOW_MSG_DEBUG)) &&
      (CHECK_FLAG (NSM_MPLS->kern_msgs, NSM_MPLS_SHOW_MSG_NOTICE)))
    cli_out (cli, "mpls log all\n");
  else
    {
      if (CHECK_FLAG (NSM_MPLS->kern_msgs, NSM_MPLS_SHOW_MSG_ERROR))
        cli_out (cli, "mpls log error\n");
      if (CHECK_FLAG (NSM_MPLS->kern_msgs, NSM_MPLS_SHOW_MSG_WARNING))
        cli_out (cli, "mpls log warning\n");
      if (CHECK_FLAG (NSM_MPLS->kern_msgs, NSM_MPLS_SHOW_MSG_DEBUG))
        cli_out (cli, "mpls log debug\n");
      if (CHECK_FLAG (NSM_MPLS->kern_msgs, NSM_MPLS_SHOW_MSG_NOTICE))
        cli_out (cli, "mpls log notice\n");
    }

  /* End. */
  cli_out (cli, "!\n");

  return 0;
}

/* Write MPLS static FTN/ILM/VC-FIB cli commands to desired output. */
int
nsm_mpls_static_config_write (struct cli *cli)
{
  struct nsm_master *nm = cli->vr->proto;
#ifdef HAVE_MPLS_VC
  struct route_node *rn;
#endif /* HAVE_MPLS_VC */
  struct interface *ifp = NULL;
  struct ptree_node *pn;
  struct avl_node *an;
  struct ftn_entry *ftn, *list;
  struct fec_gen_entry  fec;
  struct ilm_entry *ilm;
  struct pal_in4_addr nh_dmy_addr;
  struct xc_entry *xc;
  struct nhlfe_entry *nhlfe;
  struct interface *ifp_in, *ifp_out;
  struct nhlfe_key *key = NULL;
  struct gmpls_gen_label tmp_lbl;
  struct addr_in nh_addr;
  struct pal_in4_addr def_addr;
#ifdef HAVE_MPLS_VC
  struct nsm_mpls_circuit *vc;
#endif /* HAVE_MPLS_VC */
  struct prefix fec_prefix;
  int ret = 0, idx;
  bool_t is_default = PAL_FALSE;

  if (! NSM_MPLS)
    return 0;

  pal_mem_set (&nh_dmy_addr, 0, sizeof (struct pal_in4_addr));
  pal_mem_set (&fec_prefix, 0, sizeof (struct prefix));
  def_addr.s_addr = pal_hton32 (DEFAULT_NEXTHOP_ADDR);

  if (FTN_TABLE4)
    {
      for (pn = ptree_top (FTN_TABLE4); pn; pn = ptree_next (pn))
        {
          list = pn->info;
          if (! list)
            continue;

          for (ftn = list; ftn; ftn = ftn->next)
            if (CHECK_FLAG (ftn->flags, FTN_ENTRY_FLAG_STATIC))
              {
                is_default = PAL_FALSE;
                nsm_gmpls_set_fec_from_ftn (ftn, &fec);
                if (ftn->xc->nhlfe)
                  {
                    gmpls_nhlfe_outgoing_label (FTN_NHLFE (ftn), &tmp_lbl);
                    ret = gmpls_nhlfe_nh_addr (ftn->xc->nhlfe, &nh_addr);
                    if (ret == NSM_SUCCESS &&
                        (IPV4_ADDR_CMP(&nh_addr.u.ipv4,
                                       &def_addr.s_addr) == 0))
                      is_default = PAL_TRUE;
                    key = (struct nhlfe_key *) ftn->xc->nhlfe->nkey;
                    idx = gmpls_nhlfe_outgoing_if (ftn->xc->nhlfe);
                    NSM_MPLS_GET_INDEX_PTR (CHECK_FLAG (ftn->flags,
                                            FTN_ENTRY_FLAG_GMPLS), idx, ifp);
                  }

                if (ftn->bde != NULL)
                  {
                    cli_out (cli,"gmpls ftn-entry bidirectional %s tunnel-id "
                             "%d %O %d ",
                             ((ftn->bde == NULL) ? "N/A" : ftn->bde->b_name),
                             ftn->tun_id,
                             &fec.u.prefix, tmp_lbl.u.pkt);

                    if (ret == NSM_SUCCESS && is_default != PAL_TRUE)
                      cli_out (cli, "nexthop %r ", &nh_addr.u.ipv4);

                    cli_out (cli, "%s \n", ((ifp == NULL) ? "N/A" :
                             ifp->name));
                  }
                else
                  {
                    if (!CHECK_FLAG (ftn->flags, FTN_ENTRY_FLAG_BIDIR)
                        && CHECK_FLAG (ftn->flags, FTN_ENTRY_FLAG_GMPLS))
                      {
                        cli_out (cli,"gmpls ftn-entry uni-directional"
                                     " tunnel-id %d %O %d ", ftn->tun_id,
                                     &fec.u.prefix, tmp_lbl.u.pkt);

                        if (ret == NSM_SUCCESS && is_default != PAL_TRUE)
                          cli_out (cli, "nexthop %r ", &nh_addr.u.ipv4);
     
                        cli_out (cli, "%s \n",                        
                                 ((ifp == NULL) ? "N/A" : ifp->name));
                      }
                    else
                      cli_out (cli,"mpls ftn-entry tunnel-id "
                                   "%d %O %d %r %s %s\n",
                              ftn->tun_id,
                              &fec.u.prefix, tmp_lbl.u.pkt,
                              ((ret < NSM_SUCCESS) ?
                              (struct pal_in4_addr *)NULL : &nh_addr.u.ipv4),
                              ((ifp == NULL) ? "N/A" : ifp->name),
                              (CHECK_FLAG (ftn->flags, FTN_ENTRY_FLAG_PRIMARY)?
                              "primary" : "secondary"));
                  }
              }
        }
    }
#ifdef HAVE_IPV6
  if (FTN_TABLE6)
    {
      for (pn = ptree_top (FTN_TABLE6); pn; pn = ptree_next (pn))
        {
          list = pn->info;
          if (! list)
            continue;

          for (ftn = list; ftn; ftn = ftn->next)
            if (CHECK_FLAG (ftn->flags, FTN_ENTRY_FLAG_STATIC))
              {
                nsm_gmpls_set_fec_from_ftn (ftn, &fec);
                if (ftn->xc->nhlfe)
                  {
                    key = (struct nhlfe_key *) ftn->xc->nhlfe->nkey;
                    gmpls_nhlfe_outgoing_label (FTN_NHLFE (ftn), &tmp_lbl);
                    ret = gmpls_nhlfe_nh_addr (ftn->xc->nhlfe, &nh_addr);
                    idx = gmpls_nhlfe_outgoing_if (ftn->xc->nhlfe);
                    NSM_MPLS_GET_INDEX_PTR (CHECK_FLAG (ftn->flags,
                                            FTN_ENTRY_FLAG_GMPLS), idx, ifp);
                  }

                cli_out (cli,"mpls ftn-entry tunnel-id %d %O %d %40R\n %s %s\n",
                             ftn->tun_id,
                             tmp_lbl.u.pkt,
                             (ret < NSM_SUCCESS ?
                             (struct pal_in6_addr *)NULL :
                             &nh_addr.u.ipv6),
                             ((ifp == NULL) ? "N/A" : ifp->name),
                             (CHECK_FLAG (ftn->flags, FTN_ENTRY_FLAG_PRIMARY) ?
                             "primary" : "secondary"));
              }
        }
    }
#endif /* HAVE_IPV6 */

#ifdef HAVE_MPLS_VC
  /* VC data. */
  for (rn = route_top (NSM_MPLS->vc_table); rn; rn = route_next (rn))
    {
      /* If no data, skip. */
      if (! rn->info)
        continue;

      /* Get vc. */
      vc = rn->info;

      /* If no data, skip. */
      if (! vc)
        continue;

      if (vc->fec_type_vc == PW_OWNER_MANUAL &&
          vc->vc_fib && vc->vc_fib->in_label)
        {
          if (vc->vc_fib->nw_if_ix_conf > 0)
            {
              NSM_MPLS_GET_INDEX_PTR (PAL_FALSE, vc->vc_fib->nw_if_ix_conf, ifp_in);
            }
          else
            {
              NSM_MPLS_GET_INDEX_PTR (PAL_FALSE, vc->vc_fib->nw_if_ix, ifp_in);
            }
#ifdef HAVE_MS_PW
          if (vc->vc_other)
            cli_out (cli, "mpls l2-circuit-fib-entry %u %d %d %r %s %s\n",
              vc->id, vc->vc_fib->in_label, vc->vc_fib->out_label,
              &vc->address.u.prefix4, ((ifp_in == NULL) ? "N/A" : ifp_in->name),
              vc->vc_other->name);
          else
#endif /* HAVE_MS_PW */
          cli_out (cli, "mpls l2-circuit-fib-entry %u %d %d %r %s %s\n",
              vc->id, vc->vc_fib->in_label, vc->vc_fib->out_label,
              &vc->address.u.prefix4, ((ifp_in == NULL) ? "N/A" : ifp_in->name),
              vc->vc_info->mif->ifp->name);
        }
    }
#endif /* HAVE_MPLS_VC */

  if (ILM_TABLE)
    {
      for (an = avl_top (ILM_TABLE); an; an = avl_next (an))
        {
          if (! an->info)
            continue;

          for (ilm = (struct ilm_entry *)an->info; ilm; ilm = ilm->next)
            {
              xc = ilm->xc;
              if (xc)
                nhlfe = xc->nhlfe;
              else
                nhlfe = NULL;

              NSM_MPLS_GET_INDEX_PTR (CHECK_FLAG (ilm->flags,
                                                  ILM_ENTRY_FLAG_GMPLS),
                                      ilm->key.u.pkt.iif_ix, ifp_in);
              if (nhlfe)
                {
                  key = (struct nhlfe_key *) xc->nhlfe->nkey;
#ifdef HAVE_GMPLS
                  if (key->u.ipv4.oif_ix == NSM_MPLS->loop_gindex)
                    {
                      ifp_out = NULL;
                    }
                  else
#endif /* HAVE_GMPLS */
                    {
                      NSM_MPLS_GET_INDEX_PTR (CHECK_FLAG (ilm->flags,
                                                          ILM_ENTRY_FLAG_GMPLS),
                                              key->u.ipv4.oif_ix, ifp_out);
                    }
                }
              else
                {
                  ifp_out = NULL;
                }
#ifdef HAVE_MPLS_VC
              if (CHECK_FLAG (ilm->flags, ILM_ENTRY_FLAG_STATIC) &&
                  (ilm->owner.u.vc_key.vc_id != 0))
                {
                  cli_out (cli, "mpls l2-circuit-ilm-entry %d %d %s %s %r",
                      ilm->owner.u.vc_key.vc_id, ilm->key.u.pkt.in_label,
                      (ifp_in ? ifp_in->name : "N/A"),
                      (ifp_out ? ifp_out->name: "N/A"),
                      (nhlfe ? &key->u.ipv4.nh_addr :
                      &nh_dmy_addr));
                  cli_out (cli, " %d \n", ilm->ilm_ix);
                }
#endif /* HAVE_MPLS_VC */
              if (CHECK_FLAG (ilm->flags, ILM_ENTRY_FLAG_STATIC) &&
                  (ilm->owner.u.vc_key.vc_id == 0) && nhlfe)
                {
                  is_default = PAL_FALSE;
                  if (ilm->bde != NULL)
                    cli_out (cli, "gmpls ilm-entry bidirectional %s %d %s ",
                             ilm->bde->b_name,
                             ilm->key.u.pkt.in_label,
                             (ifp_in ? ifp_in->name : "N/A"));
                  else
                    {
                      if (!CHECK_FLAG (ilm->flags, ILM_ENTRY_FLAG_BIDIR)
                          && CHECK_FLAG (ilm->flags, ILM_ENTRY_FLAG_GMPLS))
                        cli_out (cli, "gmpls ilm-entry uni-directional %d %s ",
                                 ilm->key.u.pkt.in_label,
                                 (ifp_in ? ifp_in->name : "N/A"));
                      else
                        cli_out (cli, "mpls ilm-entry %d %s ",
                                 ilm->key.u.pkt.in_label,
                                 (ifp_in ? ifp_in->name : "N/A"));
                    }

                  if ((nhlfe->opcode == SWAP) ||
                      ((nhlfe->opcode == PUSH) && (ilm->n_pops == 1)))
                    cli_out (cli, "swap");
                  else if (nhlfe->opcode == POP)
                    cli_out (cli, "pop");
                  else
                    cli_out (cli, "vpnpop");

                  if ((ifp_out == NULL) ||
                      (ifp_out->hw_type == IF_TYPE_LOOPBACK))
                    {
                      cli_out (cli, " \n");
                    }
                  else
                    {
                      if (nhlfe->opcode == POP)
                        cli_out (cli, " %s",
                            ifp_out->name);
                      else
                        cli_out (cli, " %d %s",
                            key->u.ipv4.out_label,
                            ifp_out->name);

                      if (IPV4_ADDR_CMP (&key->u.ipv4.nh_addr,
                                         &def_addr.s_addr) == 0)
                        is_default = PAL_TRUE;

                      if (ilm->bde != NULL)
                        {
                          if (is_default != PAL_TRUE)
                            cli_out (cli, " nexthop %r", &key->u.ipv4.nh_addr);
                        }
                      else
                        {
                          if (!CHECK_FLAG (ilm->flags, ILM_ENTRY_FLAG_BIDIR)
                              && is_default != PAL_TRUE)
                            cli_out (cli, " nexthop %r", &key->u.ipv4.nh_addr);
                          else
                            cli_out (cli, " %r", &key->u.ipv4.nh_addr);
                        }

                      fec_prefix.family = ilm->family;
                      fec_prefix.prefixlen = ilm->prefixlen;

#ifdef HAVE_IPV6
                      if (ilm->family == AF_INET6)
                        pal_mem_cpy (&fec_prefix.u.prefix6, ilm->prfx,
                            sizeof (struct pal_in6_addr));
                      else
#endif
                        pal_mem_cpy (&fec_prefix.u.prefix4, ilm->prfx,
                            sizeof (struct pal_in4_addr));

                      cli_out (cli, " %O \n",
                          &fec_prefix);
                    }
                }
            }
        }
    }

  /* End. */
  cli_out (cli, "!\n");

#ifdef HAVE_VPLS
  /* Write VPLS static configuration. */
  nsm_vpls_static_config_write (cli);
#endif /* HAVE_VPLS */

  return 0;
}

#if (!defined (HAVE_GMPLS) || defined (HAVE_PACKET))
#ifdef HAVE_DIFFSERV

CLI (show_mpls_diffserv,
     show_mpls_diffserv_cmd,
     "show mpls diffserv",
     CLI_SHOW_STR,
     "Show MPLS specific data",
     "Show MPLS Differentiated Services specific data")
{
  nsm_mpls_diffserv_configurable_dscp_dump (cli);
  nsm_mpls_diffserv_supported_dscp_dump (cli);
  nsm_mpls_diffserv_dscp_exp_map_dump (cli);
  return CLI_SUCCESS;
}

CLI (show_mpls_diffserv_dscp,
     show_mpls_diffserv_dscp_cmd,
     "show mpls diffserv supported-dscp",
     CLI_SHOW_STR,
     "Show MPLS specific data",
     "Show MPLS Differentiated Services specific data",
     "Show supported Differentiated Services Code Point data")
{
  nsm_mpls_diffserv_supported_dscp_dump (cli);
  return CLI_SUCCESS;
}

CLI (show_mpls_diffserv_exp_map,
     show_mpls_diffserv_exp_map_cmd,
     "show mpls diffserv class-to-exp",
     CLI_SHOW_STR,
     "Show MPLS specific data",
     "Show MPLS Differentiated Services specific data",
     "Show MPLS DSCP to EXP bit mapping data")
{
  nsm_mpls_diffserv_dscp_exp_map_dump (cli);
  return CLI_SUCCESS;
}

CLI (show_mpls_diffserv_conf,
     show_mpls_diffserv_conf_cmd,
     "show mpls diffserv configurable-dscp",
     CLI_SHOW_STR,
     "Show MPLS specific data",
     "Show MPLS Differentiated Services specific data",
     "Show configurable Differentiated Services Code Points")
{
  nsm_mpls_diffserv_configurable_dscp_dump (cli);
  return CLI_SUCCESS;
}

/* CLI1 - To show configured LSP model and TTL probagation configuration */

/* Configure the dscp_exp map. */
CLI (class_exp_map_config,
     class_exp_map_config_cmd,
     "mpls class-to-exp-bit CLASS <0-7>",
     CONFIG_MPLS_STR,
     "Map a Differentiated Services Class to an EXP bit",
     "Diffserv class alias, eg: be, ef, af11 etc",
     "EXP bit to be mapped to")
{
  struct nsm_master *nm = cli->vr->proto;
  int exp_val;
  int ret;

  /* Read exp bit value. */
  CLI_GET_INTEGER_RANGE ("explicit-bit value", exp_val, argv[1], 0,
                         DIFFSERV_MAX_DSCP_EXP_MAPPINGS -1);

  ret = nsm_mpls_dscp_exp_map_add (nm, argv[0], exp_val);
  if (ret < 0)
    {
      switch (ret)
        {
        case NSM_ERR_DSCP_INVALID:
          cli_out(cli, "%% Invalid diffserv class alias: %s\n",
                  argv[0]);
          break;
        case NSM_ERR_DSCP_NOT_SUPPORTED:
          cli_out(cli, "%% Diffserv class '%s' is not supported\n",
                  argv[0]);
          break;
        }
      return CLI_ERROR;
    }

  return CLI_SUCCESS;
}

/* Delete an dscp_exp mapping. */
CLI (no_class_exp_map_config,
     no_class_exp_map_config_cmd,
     "no mpls class-to-exp-bit CLASS <0-7>",
     CLI_NO_STR,
     CONFIG_MPLS_STR,
     "Unmap a Differentiated Services Class from an EXP bit",
     "Diffserv class alias, eg: be, ef, af11 etc",
     "Mapped EXP bit")
{
  struct nsm_master *nm = cli->vr->proto;
  int exp_val;
  int ret;

  /* Read exp bit value. */
  CLI_GET_INTEGER_RANGE ("explicit-bit value", exp_val, argv[1], 0,
                         DIFFSERV_MAX_DSCP_EXP_MAPPINGS -1);

  ret = nsm_mpls_dscp_exp_map_del (nm, argv[0], exp_val);
  if (ret < 0)
    {
      switch (ret)
        {
        case NSM_ERR_DSCP_INVALID:
          cli_out(cli, "%% Invalid diffserv class alias: %s\n",
                  argv[0]);
          break;
        case NSM_ERR_DSCP_EXP_MISMATCH:
          cli_out(cli, "%% Diffserv class '%s' is not mapped to exp-bit "
                  "specified.\n", argv[0]);
          break;
        }
      return CLI_ERROR;
    }

  return CLI_SUCCESS;
}

/* Configure supported class.*/
CLI (supported_class,
     supported_class_cmd,
     "mpls support-diffserv-class CLASS",
     CONFIG_MPLS_STR,
     "Enable support for MPLS Differentiated Services Class",
     "Diffserv class alias. eg: be,ef,af11 etc.")
{
  struct nsm_master *nm = cli->vr->proto;
  int ret;

  /* Add class. */
  ret = nsm_mpls_dscp_support_add (nm, argv[0]);
  if (ret < 0)
    {
      switch (ret)
        {
        case NSM_ERR_DSCP_INVALID:
          cli_out(cli, "%% Invalid diffserv class: %s\n",
                  argv[0]);
          break;
        }
      return CLI_ERROR;
    }

  return CLI_SUCCESS;
}

/* delete the supported class.*/
CLI (no_supported_class,
     no_supported_class_cmd,
     "no mpls support-diffserv-class CLASS",
     CLI_NO_STR,
     CONFIG_MPLS_STR,
     "Disable supported for MPLS Differentiated Services Class",
     "Diffserv class alias. eg: be,ef,af11 etc.")
{
  struct nsm_master *nm = cli->vr->proto;
  int ret;

  /* Remove */
  ret = nsm_mpls_dscp_support_del (nm, argv[0]);
  if (ret < 0)
    {
      switch (ret)
        {
        case NSM_ERR_DSCP_INVALID:
          cli_out(cli, "%% Invalid diffserv class: %s\n",
                  argv[0]);
          break;
        }
      return CLI_ERROR;
    }

  return CLI_SUCCESS;
}

/* Set LSP MODEL to PIPE */
CLI (lsp_model_pipe,
     lsp_model_pipe_cmd,
     "mpls lsp-model pipe",
     CONFIG_MPLS_STR,
     "LSP Model",
     "Model to be configured")
{
  nsm_mpls_api_pipe_model_update (cli->vr->proto, NSM_TRUE);
  return CLI_SUCCESS;
}

/* Reset LSP MODEL to UNIFORM */
CLI (no_lsp_model_pipe,
     no_lsp_model_pipe_cmd,
     "no mpls lsp-model pipe",
     CLI_NO_STR,
     CONFIG_MPLS_STR,
     "LSP Model",
     "Model to be configured")
{
  nsm_mpls_api_pipe_model_update (cli->vr->proto, NSM_FALSE);
  return CLI_SUCCESS;
}
#endif /* HAVE_DIFFSERV */

/* Configure TTL propagation in MPLS domain */
CLI (propagate_ttl,
     propagate_ttl_cmd,
     "mpls propagate-ttl",
     CONFIG_MPLS_STR,
     "Propogate TTL")
{
  nsm_mpls_api_ttl_propagate_cap_update (cli->vr->proto, NSM_TRUE);
  return CLI_SUCCESS;
}

/* Unconfigure TTL propagation in MPLS domain */
CLI (no_propagate_ttl,
     no_propagate_ttl_cmd,
     "no mpls propagate-ttl",
     CLI_NO_STR,
     CONFIG_MPLS_STR,
     "Propogate TTL")
{
  nsm_mpls_api_ttl_propagate_cap_update (cli->vr->proto, NSM_FALSE);
  return CLI_SUCCESS;
}

#ifdef HAVE_DSTE
/* Input MUST be of form cti, where i is b/w 0 and 7, inclusive. */
static int
mpls_parse_class_type (char *str)
{
  if (pal_strlen (str) == 3
      && str[0] == 'c'
      && str[1] == 't'
      && str[2] >= 48
      && str[2] <= 55)
    return str[2] - 48;

  return CT_NUM_INVALID;
}

CLI (class_type,
     class_type_cmd,
     "mpls class-type CLASS-TYPE CLASS-TYPE-NAME",
     CONFIG_MPLS_STR,
     "Configure a DSTE Class Type",
     "Class Type to be configured (ct0 -- ct7)",
     "Name to be configured for specified Class Type")
{
  struct nsm_master *nm = cli->vr->proto;
  u_char ct_num;

  /* Check Class Type. */
  ct_num = mpls_parse_class_type (argv[0]);
  if (ct_num == CT_NUM_INVALID)
    {
      cli_out (cli, "%% Invalid Class Type specified.\n");
      return CLI_ERROR;
    }

  if (pal_strlen (argv[1]) > MAX_CT_NAME_LEN)
    {
      cli_out (cli, "%% Class Type name cannot exceed %d characters.\n",
               MAX_CT_NAME_LEN);
      return CLI_ERROR;
    }

  if (nsm_dste_get_class_type_num (nm, argv[1]) != CT_NUM_INVALID)
    {
      cli_out (cli, "%% Duplicated Class Type name\n");
      return CLI_ERROR;
    }

  nsm_dste_class_type_add (nm, ct_num, argv[1]);
  return CLI_SUCCESS;
}

CLI (no_class_type,
     no_class_type_cmd,
     "no mpls class-type CLASS-TYPE CLASS-TYPE-NAME",
     CLI_NO_STR,
     NO_CONFIG_MPLS_STR,
     "Unset configured DiffServ TE Class Type",
     "Class Type configured (ct0 -- ct7)",
     "Configured name for Class Type")
{
  struct nsm_master *nm = cli->vr->proto;
  u_char ct_num;

  /* Check Class Type. */
  ct_num = mpls_parse_class_type (argv[0]);
  if (ct_num == CT_NUM_INVALID)
    {
      cli_out (cli, "%% Invalid Class Type specified.\n");
      return CLI_ERROR;
    }

  if (pal_strcmp(NSM_MPLS->ct_name[ct_num], argv[1]) != 0)
    {
      cli_out (cli, "%% Class Type and Class Type name mismatch.\n");
      return CLI_ERROR;
    }

  nsm_dste_class_type_delete (nm, ct_num);
  return CLI_SUCCESS;

}

void
nsm_mpls_dste_class_type_dump (struct cli *cli)
{
  struct nsm_master *nm = cli->vr->proto;
  u_char i;

  for (i = 0; i < MAX_CLASS_TYPE; i++)
    if (NSM_MPLS->ct_name[i][0] != '\0')
      cli_out (cli, "ct%d: %s\n", i, NSM_MPLS->ct_name[i]);
}

CLI (show_class_type,
     show_class_type_cmd,
     "show mpls dste class-type",
     CLI_SHOW_STR,
     "Show MPLS specific data",
     "Show MPLS DSTE configuration infomation",
     "Show MPLS DSTE Class Type")
{
  struct nsm_master *nm = cli->vr->proto;

  if (! NSM_MPLS)
    return CLI_SUCCESS;

  nsm_mpls_dste_class_type_dump (cli);
  cli_out (cli, "\n");

  return CLI_SUCCESS;

}

/* Input MUST be of form tei, where i is b/w 0 and 7, inclusive. */
static int
mpls_parse_te_class (char *str)
{
  if (pal_strlen (str) == 3
      && str[0] == 't'
      && str[1] == 'e'
      && str[2] >= 48
      && str[2] <= 55)
    return str[2] - 48;

  return MAX_CLASS_TYPE;
}

CLI (te_class,
     te_class_cmd,
     "mpls te-class TE-CLASS CLASS-TYPE-NAME <0-7>",
     CONFIG_MPLS_STR,
     "Configure a DiffServ TE Class",
     "DiffServ TE Class (te0 -- te7)",
     "Class type name",
     "Preemption priority from 0 to 7")
{
  struct nsm_master *nm = cli->vr->proto;
  u_char tecl_index;
  int prio;
  u_char i;

  /* Check TE CLASS. */
  tecl_index = mpls_parse_te_class (argv[0]);
  if (tecl_index == MAX_CLASS_TYPE)
    {
      cli_out (cli, "%% Invalid DiffServ TE Class specified.\n");
      return CLI_ERROR;
    }

  /* Priority. */
  CLI_GET_INTEGER_RANGE ("priority", prio, argv[2], 0, 7);

  for (i = 0; i < MAX_TE_CLASS; i++)
    if (pal_strcmp (argv[1],
                    NSM_MPLS->ct_name[NSM_MPLS->te_class[i].ct_num]) == 0
        && prio == NSM_MPLS->te_class[i].priority)
      {
        cli_out (cli, "%% Add failed: Duplicated configuration with te%d\n",
                 i);
        return CLI_ERROR;
      }

  /* Check CLASS TYPE name. */
  for (i = 0; i < MAX_CLASS_TYPE; i++)
    if (pal_strncmp (argv[1], NSM_MPLS->ct_name[i], pal_strlen (argv[1])) == 0)
      {
        nsm_dste_te_class_add (nm, tecl_index, i, (u_char)prio);
        return CLI_SUCCESS;
      }

  cli_out (cli, "%% Add failed: Invalid/Mismatched input parameters "
           "specified.\n");
  return CLI_ERROR;
}

CLI (no_te_class,
     no_te_class_cmd,
     "no mpls te-class TE-CLASS CLASS-TYPE-NAME <0-7>",
     CLI_NO_STR,
     NO_CONFIG_MPLS_STR,
     "Unset a DiffServ TE Class",
     "DiffServ TE Class (te0 -- te7)",
     "Class type name",
     "Preemption priority from 0 to 7")
{
  struct nsm_master *nm = cli->vr->proto;
  int tecl_index;
  int prio;
  int i;

  /* Check TE CLASS. */
  tecl_index = mpls_parse_te_class (argv[0]);
  if (tecl_index == MAX_CLASS_TYPE)
    {
      cli_out (cli, "%% Invalid DiffServ TE Class specified.\n");
      return CLI_ERROR;
    }

  /* Priority. */
  CLI_GET_INTEGER_RANGE ("priority", prio, argv[2], 0, 7);

  /* Check CLASS TYPE name. */
  for (i = 0; i < MAX_CLASS_TYPE; i++)
    if (pal_strncmp (argv[1], NSM_MPLS->ct_name[i], pal_strlen (argv[1])) == 0
        && NSM_MPLS->te_class[tecl_index].ct_num == i
        && NSM_MPLS->te_class[tecl_index].priority == (u_char)prio)
      {
        nsm_dste_te_class_delete (nm, tecl_index);
        return CLI_SUCCESS;
      }


  cli_out (cli, "%% Delete failed: Invalid/Mismatched input parameters "
           "specified.\n");
  return CLI_ERROR;
}

void
nsm_mpls_dste_te_class_dump (struct cli *cli)
{
  struct nsm_master *nm = cli->vr->proto;
  u_char i;

  for (i = 0; i < MAX_TE_CLASS; i++)
    if (NSM_MPLS->te_class[i].ct_num != CT_NUM_INVALID)
      cli_out (cli, "te%d: { %s, %d }\n",
         i,
         NSM_MPLS->ct_name[NSM_MPLS->te_class[i].ct_num],
         NSM_MPLS->te_class[i].priority);
}

CLI (show_te_class,
     show_te_class_cmd,
     "show mpls dste te-class",
     CLI_SHOW_STR,
     "Show MPLS specific data",
     "Show MPLS DSTE configuration infomation",
     "Show MPLS DSTE TE Class")
{
  struct nsm_master *nm = cli->vr->proto;

  if (! NSM_MPLS)
    return CLI_SUCCESS;

  nsm_mpls_dste_te_class_dump (cli);
  cli_out (cli, "\n");

  return CLI_SUCCESS;

}

CLI (show_dste_info,
     show_dste_info_cmd,
     "show mpls dste",
     CLI_SHOW_STR,
     "Show MPLS specific data",
     "Show MPLS DSTE configuration infomation")
{
  struct nsm_master *nm = cli->vr->proto;

  if (! NSM_MPLS)
    return CLI_SUCCESS;

  nsm_mpls_dste_te_class_dump (cli);
  nsm_mpls_dste_class_type_dump (cli);
  cli_out (cli, "\n");

  return CLI_SUCCESS;
}
#endif /* HAVE_DSTE */
#endif /* !HAVE_GMPLS || HAVE_PACKET */

#ifdef HAVE_NSM_MPLS_OAM
/* Hidden CLI to control ping mpls request and reply tlv set */

CLI (enable_mpls_ping_req_dsmap_tlv,
     enable_mpls_ping_req_dsmap_tlv_cmd,
     "mpls-ping-request-dsmap-tlv",
     "Enable mpls ping request dsmap tlv")
{
  oam_req_dsmap_tlv = NSM_TRUE;
  return CLI_SUCCESS;
}

CLI (disable_mpls_ping_req_dsmap_tlv,
     disable_mpls_ping_req_dsmap_tlv_cmd,
     "no mpls-ping-request-dsmap-tlv",
     CLI_NO_STR,
     "Disable mpls ping request dsmap tlv")
{
  /* disable is default */
  oam_req_dsmap_tlv = NSM_FALSE;
  return CLI_SUCCESS;
}

CLI (enable_mpls_ping_reply_tlv,
     enable_mpls_ping_reply_tlv_cmd,
     "mpls-ping-reply-tlv",
     "Enable mpls ping reply tlv")
{
  oam_reply_tlv = NSM_TRUE;
  return CLI_SUCCESS;
}

CLI (disable_mpls_ping_reply_tlv,
     disable_mpls_ping_reply_tlv_cmd,
     "no mpls-ping-reply-tlv",
     CLI_NO_STR,
     "Disable mpls ping reply tlv")
{
  /* disable is default */
  oam_reply_tlv = NSM_FALSE;
  return CLI_SUCCESS;
}

#endif /* HAVE_NSM_MPLS_OAM */

#ifdef HAVE_MPLS_OAM_ITUT

CLI (imish_mpls_cv_cli,
     imish_mpls_cv_cmd,
     "mpls cv ((start|stop) lsp-id <1-65535> (|ttl <1-255>|timeout <1-500>"
     "|sink-lsr src-lsr-ip A.B.C.D)|)",
     "mpls oam command",
     "OAM CV",
     "start OAM",
     "stop OAM",
     "lsp id",
     "lsp id range <1-65535>",
     "Time-to-live",
     "ttl Value",
     "Timeout of OAM",
     "timeout value",
     "Sink LSR configuration",
     "SRC LSR IP",
     "LSR IPv4 Address"
     )
{
  struct nsm_master *nm = cli->vr->proto;
  struct nsm_msg_mpls_oam_itut_req msg;
  struct nsm_server_entry *nse = NULL;
  int ret = 0;

  pal_mem_set (&msg, 0 , sizeof (struct nsm_msg_mpls_oam_itut_req));
  msg.pkt_type = NSM_MPLS_CV_PKT;
  if (pal_strncmp (argv[0], "start", pal_strlen (argv[0]))== 0)
   {
     itut_flag.flag  = NSM_MPLS_OAM_ENABLE;
     msg.flag = NSM_MPLS_OAM_PKT_START;
   }
  else if (pal_strncmp(argv[0], "stop", pal_strlen(argv[0]))== 0)
   {
     itut_flag.flag = NSM_MPLS_OAM_DISABLE;
     msg.flag = NSM_MPLS_OAM_PKT_STOP;
   }
  CLI_GET_INTEGER_RANGE ("LSP-ID", msg.lsp_id, argv[2], 1, 65535);
  if (pal_strncmp(argv[3], "ttl", pal_strlen (argv[3]))== 0)
   {
     CLI_GET_INTEGER_RANGE ("ttl", msg.ttl, argv[4], 1, 255);
     msg.timeout = 0;
   }
  else if (pal_strncmp(argv[3], "timeout", pal_strlen (argv[3]))== 0)
   {
     CLI_GET_INTEGER_RANGE ("timeout", msg.timeout, argv[4], 1, 500);
     msg.ttl = 0;
   }
   if (pal_strncmp (argv[3], "sink-lsr", pal_strlen (argv[3]))== 0)
    {
      CLI_GET_IPV4_ADDRESS("SRC-LSRIP", msg.lsr_addr, argv [5]);
      msg.lsr_type = NSM_MPLS_OAM_SINK_LSR;
    }

   ret = nsm_mpls_oam_itut_process_request (nm, nse, &msg);
   if (ret < 0)
      return CLI_ERROR;

    return CLI_SUCCESS;
}

CLI (imish_mpls_ffd_cli,
     imish_mpls_ffd_cmd,
     "mpls ffd (((start frequency (<1-6>|default))| stop) lsp-id <1-65535>"
     "(|ttl <1-255>|timeout <1-500>|sink-lsr src-lsp-ip A.B.C.D)|)",
     "mpls oam command",
     "OAM FFD",
     "start OAM",
     "frequency",
     "frequency value<1-6>",
     "default(3)",
     "stop OAM",
     "lsp id",
     "lsp id range <1-65535>",
     "Time-to-live",
     "ttl Value",
     "Timeout of OAM",
     "timeout value",
     "Sink LSR configuration",
     "SRC LSR IP",
     "LSR IPv4 Address"
     )
{
  struct nsm_master *nm = cli->vr->proto;
  struct nsm_msg_mpls_oam_itut_req msg;
  struct nsm_server_entry *nse = NULL;
  u_int16_t cntr = 0;
  int ret = 0;

  pal_mem_set (&msg, 0 , sizeof (struct nsm_msg_mpls_oam_itut_req));
  msg.pkt_type = NSM_MPLS_FFD_PKT;
  if (pal_strncmp (argv[0], "start", pal_strlen (argv[0]))== 0)
   {
     itut_flag.flag = NSM_MPLS_OAM_ENABLE;
     msg.flag = NSM_MPLS_OAM_PKT_START;
     if (pal_strncmp (argv[1], "frequency", pal_strlen (argv[1]))== 0)
      {
        if (pal_strncmp (argv[2], "default", pal_strlen (argv[2]))== 0)
         {
           msg.frequency = NSM_MPLS_OAM_FFD_DEFAULT_FREQ;
         }
        else
         {
           CLI_GET_INTEGER_RANGE ("frequency", msg.frequency, argv[2], 1, 6);
         }
	cntr += 3;
      }
   }
   else if (pal_strncmp(argv[0], "stop", pal_strlen(argv[0]))== 0)
    {
      itut_flag.flag = NSM_MPLS_OAM_DISABLE;
      msg.flag = NSM_MPLS_OAM_PKT_STOP;
      msg.frequency = NSM_MPLS_OAM_FFD_FREQ_RESV;
      cntr++;
    }
   CLI_GET_INTEGER_RANGE ("LSP-ID", msg.lsp_id, argv[cntr+1], 1, 65535);
   if (pal_strncmp(argv[cntr+2], "ttl", pal_strlen (argv[cntr+2]))== 0)
    {
      CLI_GET_INTEGER_RANGE ("ttl", msg.ttl, argv[cntr+3], 1, 255);
      msg.timeout = 0;
    }
   else if (pal_strncmp(argv[cntr+2], "timeout", pal_strlen (argv[cntr+2]))== 0)
    {
      CLI_GET_INTEGER_RANGE ("timeout", msg.timeout, argv[cntr+3], 1, 500);
      msg.ttl = 0;
    }
   if (pal_strncmp (argv[cntr+2], "sink-lsr", pal_strlen (argv[cntr+2]))== 0)
    {
      CLI_GET_IPV4_ADDRESS("SRC-LSRIP", msg.lsr_addr, argv [cntr+4]);
      msg.lsr_type = NSM_MPLS_OAM_SINK_LSR;
    }
   ret = nsm_mpls_oam_itut_process_request (nm, nse, &msg);
   if (ret < 0)
      return CLI_ERROR;

   return CLI_SUCCESS;
}

CLI (imish_mpls_oam_cli,
     imish_mpls_oam_cmd,
     "mpls oam (enable|disable|)",
     "mpls oam command",
     "OAM",
     "enable OAM",
     "disable OAM"
     )
{
  itut_flag.flag = NSM_MPLS_OAM_ENABLE;
  if (argc > 0)
    {
      if (pal_strncmp(argv[0], "enable", pal_strlen(argv[0]))== 0)
        {
          itut_flag.flag = NSM_MPLS_OAM_ENABLE;
        }
      else
        {
          itut_flag.flag = NSM_MPLS_OAM_DISABLE;
        }
    }
  else
    itut_flag.flag = NSM_MPLS_OAM_ENABLE;

  return CLI_SUCCESS;
}

CLI (imish_mpls_fdi_cli,
     imish_mpls_fdi_cmd,
     "mpls fdi lsp-id <1-65535> ((ttl <1-255>|timeout <1-500>)"
     "level ((1 label-1 LABEL)|(2 label-1 LABEL label-2 LABEL)|"
     "(3 label-1 LABEL label-2 LABEL label-3 LABEL))|)",
     "mpls oam command",
     "OAM FDI",
     "lsp id",
     "lsp id range <1-65535>",
     "Time-to-live",
     "ttl Value",
     "Timeout of OAM",
     "timeout value",
     "level No <1-3>",
     "level No 1",
     "immediate first tunnel lable",
     "Label-1 <0(explicit-null), 16-1048575>",
     "level No 2",
     "immediate first tunnel lable",
     "Label-1 <0(explicit-null), 16-1048575>",
     "middle tunnel label",
     "Label-2 <0(explicit-null), 16-1048575>",
     "level No 3",
     "immediate first tunnel lable",
     "Label-1 <0(explicit-null), 16-1048575>",
     "middle tunnel label",
     "Label-2 <0(explicit-null), 16-1048575>",
     "final/last tunnel label",
     "label-3 <0(explicit-null), 16-1048575>"
     )
{
  struct nsm_master *nm = cli->vr->proto;
  struct nsm_msg_mpls_oam_itut_req msg;
  struct nsm_server_entry *nse = NULL;
  u_int16_t cntr = 0, arg_num = 0;
  int ret = 0;

  pal_mem_set (&msg, 0 , sizeof (struct nsm_msg_mpls_oam_itut_req));
  msg.pkt_type = NSM_MPLS_FDI_PKT;

  CLI_GET_INTEGER_RANGE ("LSP-ID", msg.lsp_id, argv[0], 1, 65535);
  if (pal_strncmp(argv[1], "ttl", pal_strlen (argv[1]))== 0)
   {
     CLI_GET_INTEGER_RANGE ("ttl", msg.ttl, argv[2], 1, 255);
     msg.timeout = 0;
   }
  else if (pal_strncmp(argv[1], "timeout", pal_strlen (argv[1]))== 0)
   {
     CLI_GET_INTEGER_RANGE ("timeout", msg.timeout, argv[2], 1, 500);
     msg.ttl = 0;
   }
  CLI_GET_INTEGER_RANGE ("tunnel-level", msg.fdi_level.level_no, argv[4], 1, 3);
      arg_num = 6;
  for (cntr = 0; cntr < msg.fdi_level.level_no; cntr++)
   {
     CLI_GET_UINT32 ("outgoing-label", msg.fdi_level.lable_id[cntr], argv[arg_num]);
     arg_num+=2;
   }

  ret = nsm_mpls_oam_itut_process_request (nm, nse, &msg);
  if (ret < 0)
    return CLI_ERROR;

  return CLI_SUCCESS;
}

CLI (imish_mpls_bdi_cli,
     imish_mpls_bdi_cmd,
     "mpls bdi (dst-lsp-id <1-65535> src-lsp-id <1-65535>"
     "(ttl <1-255>|timeout <1-500>)|)",
     "mpls oam command",
     "OAM BDI",
     "Return lSP ID",
     "lsp id range <1-65535>",
     "SRC LSP Id",
     "lsp id range <1-65535>"
     "Time-to-live",
     "ttl Value",
     "Timeout of OAM",
     "timeout value"
     )
{

  struct nsm_master *nm = cli->vr->proto;
  struct nsm_msg_mpls_oam_itut_req msg;
  struct nsm_server_entry *nse = NULL;
  int ret = 0;

  pal_mem_set (&msg, 0 , sizeof (struct nsm_msg_mpls_oam_itut_req));
  msg.pkt_type = NSM_MPLS_FDI_PKT;

  CLI_GET_INTEGER_RANGE ("LSP-ID", msg.lsp_id, argv[1], 1, 65535);
  CLI_GET_INTEGER_RANGE ("FWD-LSP-ID", msg.fwd_lsp_id, argv[3], 1, 65535);
  if (pal_strncmp(argv[2], "ttl", pal_strlen (argv[4]))== 0)
   {
     CLI_GET_INTEGER_RANGE ("ttl", msg.ttl, argv[5], 1, 255);
     msg.timeout = 0;
   }
  else if (pal_strncmp(argv[2], "timeout", pal_strlen (argv[4]))== 0)
   {
     CLI_GET_INTEGER_RANGE ("timeout", msg.timeout, argv[5], 1, 500);
     msg.ttl = 0;
   }

  ret = nsm_mpls_oam_itut_process_request (nm, nse, &msg);
  if (ret < 0)
   return CLI_ERROR;

  return CLI_SUCCESS;
}

CLI (imish_mpls_stats_cli,
     imish_mpls_stats_cmd,
     "mpls statistics (lsp-id <1-65535>|)",
     "mpls oam command",
     "Statistics",
     "lSP Id to monitor statistics",
     "LSP Id value <1-65535>"
     )
{
  struct nsm_master *nm = cli->vr->proto;
  struct nsm_mpls_oam_stats msg;
  pal_mem_set (&msg, 0 , sizeof (struct nsm_mpls_oam_stats));
  CLI_GET_INTEGER_RANGE ("LSP-ID", msg.lsp_id, argv[0], 1, 65535);
  nsm_mpls_oam_stats_display(nm, &msg);
  cli_out(cli, " LSP-Id: %d, total oam packets sent on LSP : %d, %d\n",
          msg.lsp_id, msg.oampkt_counter);
  return CLI_SUCCESS;
}
#endif /* HAVE_MPLS_OAM_ITUT*/

/* Install the PacOS MPLS node commands. */
void
nsm_mpls_init_commands ()
{
  struct cli_tree *ctree;

  ctree = NSM_ZG->ctree;

  /* Install NSM MPLS node. */
  cli_install_config (ctree, NSM_MPLS_MODE, nsm_mpls_config_write);

  /* Install NSM MPLS STATIC node. */
  cli_install_config (ctree, NSM_MPLS_STATIC_MODE,
                      nsm_mpls_static_config_write);

  /* Install VPLS node */
#ifdef HAVE_VPLS
  cli_install_config (ctree, NSM_VPLS_MODE, nsm_vpls_config_write);
#endif /* HAVE_VPLS */

  /* Show commands. */
  cli_install_gen (ctree, EXEC_MODE, PRIVILEGE_NORMAL, 0, &show_mpls_cmd);
#ifdef HAVE_MPLS_FWD
  cli_install_gen (ctree, EXEC_MODE, PRIVILEGE_NORMAL, 0, &show_mpls_if_cmd);
#endif /* HAVE_MPLS_FWD */
  cli_install_gen (ctree, EXEC_MODE, PRIVILEGE_NORMAL, 0, &show_mpls_log_cmd);
  cli_install_gen (ctree, EXEC_MODE, PRIVILEGE_NORMAL, 0, &mpls_fwd_table_cmd);
  cli_install_gen (ctree, EXEC_MODE, PRIVILEGE_NORMAL, 0, &mpls_ilm_table_cmd);
  cli_install_gen (ctree, EXEC_MODE, PRIVILEGE_NORMAL, 0, &mpls_insegment_cmd);
  cli_install_gen (ctree, EXEC_MODE, PRIVILEGE_NORMAL, 0, &mpls_xc_table_cmd);
  cli_install_gen (ctree, EXEC_MODE, PRIVILEGE_NORMAL, 0,
                   &mpls_nhlfe_table_cmd);
  cli_install_gen (ctree, EXEC_MODE, PRIVILEGE_NORMAL, 0, &mpls_ftn_table_cmd);

#ifdef HAVE_PACKET
  cli_install_gen (ctree, EXEC_MODE, PRIVILEGE_NORMAL, 0,
                   &mpls_mapped_routes_cmd);
#endif /* HAVE_PACKET */
#ifdef HAVE_VRF
  cli_install_gen (ctree, EXEC_MODE, PRIVILEGE_NORMAL, 0, &mpls_vrf_table_cmd);
  cli_install_gen (ctree, EXEC_MODE, PRIVILEGE_NORMAL, 0,
                   &mpls_vrf_table_name_cmd);
#endif /* HAVE_VRF */

#ifdef HAVE_TE
  cli_install_gen (ctree, EXEC_MODE, PRIVILEGE_NORMAL, 0, &show_admin_grp_cmd);
#endif /* HAVE_TE */

#ifdef HAVE_MPLS_VC
  cli_install_gen (ctree, EXEC_MODE, PRIVILEGE_NORMAL, 0, &mpls_vc_table_cmd);
  cli_install_gen (ctree, EXEC_MODE, PRIVILEGE_NORMAL, 0,
                   &mpls_l2_circuit_show_cmd);
  cli_install_gen (ctree, EXEC_MODE, PRIVILEGE_NORMAL, 0,
                   &mpls_l2_circuit_show_by_name_cmd);
  cli_install_gen (ctree, EXEC_MODE, PRIVILEGE_NORMAL, 0,
                   &mpls_vc_switchover_cmd);
  cli_install_gen (ctree, EXEC_MODE, PRIVILEGE_NORMAL, 0,
                   &mpls_l2_vc_group_show_cmd);
  cli_install_gen (ctree, EXEC_MODE, PRIVILEGE_NORMAL, 0,
                   &mpls_l2_vc_group_show_by_name_cmd);
#ifdef HAVE_MS_PW
  cli_install_gen (ctree, EXEC_MODE, PRIVILEGE_NORMAL, 0,
                   &show_mpls_mspw_det_cmd);
#endif /* HAVE_MS_PW */

#endif /* HAVE_MPLS_VC */

#if (!defined (HAVE_GMPLS) || defined (HAVE_PACKET))
#ifdef HAVE_DIFFSERV
  cli_install_gen (ctree, EXEC_MODE, PRIVILEGE_NORMAL, 0,
                   &show_mpls_diffserv_cmd);
  cli_install_gen (ctree, EXEC_MODE, PRIVILEGE_NORMAL, 0,
                   &show_mpls_diffserv_dscp_cmd);
  cli_install_gen (ctree, EXEC_MODE, PRIVILEGE_NORMAL, 0,
                   &show_mpls_diffserv_exp_map_cmd);
  cli_install_gen (ctree, EXEC_MODE, PRIVILEGE_NORMAL, 0,
                   &show_mpls_diffserv_conf_cmd);
#endif /* HAVE_DIFFSERV */

#ifdef HAVE_DSTE
  cli_install_gen (ctree, EXEC_MODE, PRIVILEGE_NORMAL, 0,
                   &show_class_type_cmd);
  cli_install_gen (ctree, EXEC_MODE, PRIVILEGE_NORMAL, 0,
                   &show_te_class_cmd);
  cli_install_gen (ctree, EXEC_MODE, PRIVILEGE_NORMAL, 0,
                   &show_dste_info_cmd);
#endif /* HAVE_DSTE */
#endif /* !HAVE_GMPLS || HAVE_PACKET */
  /* Other commands. */
  cli_install_gen (ctree, CONFIG_MODE, PRIVILEGE_MAX, 0, &lsp_tunneling_cmd);
  cli_install_gen (ctree, CONFIG_MODE, PRIVILEGE_MAX, 0, &no_lsp_tunneling_cmd);
#ifdef HAVE_TE
  cli_install_gen (ctree, CONFIG_MODE, PRIVILEGE_MAX, 0, &admin_grp_cmd);
  cli_install_gen (ctree, CONFIG_MODE, PRIVILEGE_MAX, 0, &no_admin_grp_cmd);
#endif /* HAVE_TE */
  cli_install_gen (ctree, CONFIG_MODE, PRIVILEGE_MAX, 0,
                   &mpls_enable_all_ifs_cmd);
  cli_install_gen (ctree, CONFIG_MODE, PRIVILEGE_MAX, 0,
                   &mpls_disable_all_ifs_cmd);

  cli_install_gen (ctree, CONFIG_MODE, PRIVILEGE_MAX, 0,
                   &min_label_pool_size_cmd);
  cli_install_gen (ctree, CONFIG_MODE, PRIVILEGE_MAX, 0,
                   &max_label_pool_size_cmd);
  cli_install_gen (ctree, CONFIG_MODE, PRIVILEGE_MAX, 0,
                   &no_min_label_pool_size_cmd);
  cli_install_gen (ctree, CONFIG_MODE, PRIVILEGE_MAX, 0,
                   &no_max_label_pool_size_cmd);

#if (!defined (HAVE_GMPLS) || defined (HAVE_PACKET))
  cli_install_gen (ctree, CONFIG_MODE, PRIVILEGE_MAX, 0,
                   &ingress_ttl_cmd);
  cli_install_gen (ctree, CONFIG_MODE, PRIVILEGE_MAX, 0,
                   &no_ingress_ttl_cmd);
  cli_install_gen (ctree, CONFIG_MODE, PRIVILEGE_MAX, 0,
                   &no_ingress_ttl_val_cmd);
  cli_install_gen (ctree, CONFIG_MODE, PRIVILEGE_MAX, 0,
                   &egress_ttl_cmd);
  cli_install_gen (ctree, CONFIG_MODE, PRIVILEGE_MAX, 0,
                   &no_egress_ttl_cmd);
  cli_install_gen (ctree, CONFIG_MODE, PRIVILEGE_MAX, 0,
                   &no_egress_ttl_val_cmd);
#endif /* !HAVE_GMPLS || HAVE_PACKET */
  cli_install_gen (ctree, CONFIG_MODE, PRIVILEGE_MAX, 0,
                   &local_packet_handle_cmd);
  cli_install_gen (ctree, CONFIG_MODE, PRIVILEGE_MAX, 0,
                   &no_local_packet_handle_cmd);
  cli_install_gen (ctree, CONFIG_MODE, PRIVILEGE_MAX, 0,
                   &log_all_message_cmd);
  cli_install_gen (ctree, CONFIG_MODE, PRIVILEGE_MAX, 0,
                   &no_log_all_message_cmd);
  cli_install_gen (ctree, CONFIG_MODE, PRIVILEGE_MAX, 0,
                   &log_error_message_cmd);
  cli_install_gen (ctree, CONFIG_MODE, PRIVILEGE_MAX, 0,
                   &no_log_error_message_cmd);
  cli_install_gen (ctree, CONFIG_MODE, PRIVILEGE_MAX, 0,
                   &log_warning_message_cmd);
  cli_install_gen (ctree, CONFIG_MODE, PRIVILEGE_MAX, 0,
                   &no_log_warning_message_cmd);
  cli_install_gen (ctree, CONFIG_MODE, PRIVILEGE_MAX, 0,
                   &log_debug_message_cmd);
  cli_install_gen (ctree, CONFIG_MODE, PRIVILEGE_MAX, 0,
                   &no_log_debug_message_cmd);
  cli_install_gen (ctree, CONFIG_MODE, PRIVILEGE_MAX, 0,
                   &log_notice_message_cmd);
  cli_install_gen (ctree, CONFIG_MODE, PRIVILEGE_MAX, 0,
                   &no_log_notice_message_cmd);
  cli_install_gen (ctree, CONFIG_MODE, PRIVILEGE_MAX, 0,
                   &mapped_route_add_cmd);
  cli_install_gen (ctree, CONFIG_MODE, PRIVILEGE_MAX, 0,
                   &mapped_route_add_mask_cmd);
  cli_install_gen (ctree, CONFIG_MODE, PRIVILEGE_MAX, 0,
                   &mapped_route_del_cmd);
  cli_install_gen (ctree, CONFIG_MODE, PRIVILEGE_MAX, 0,
                   &mapped_route_del_mask_cmd);
#ifdef HAVE_MPLS_VC
  cli_install_gen (ctree, CONFIG_MODE, PRIVILEGE_MAX, 0,
                   &mpls_ac_group_cmd );
  cli_install_gen (ctree, CONFIG_MODE, PRIVILEGE_MAX, 0,
                   &mpls_no_ac_group_cmd );
  cli_install_gen (ctree, CONFIG_MODE, PRIVILEGE_MAX, 0,
                   &mpls_l2_circuit_with_grp_cmd);
  cli_install_gen (ctree, CONFIG_MODE, PRIVILEGE_MAX, 0,
                   &mpls_l2_circuit_cmd);
  cli_install_gen (ctree, CONFIG_MODE, PRIVILEGE_MAX, 0,
                   &no_mpls_l2_circuit_basic_cmd);
  cli_install_gen (ctree, CONFIG_MODE, PRIVILEGE_MAX, 0,
                   &no_mpls_l2_circuit_with_grp_cmd);
  cli_install_gen (ctree, CONFIG_MODE, PRIVILEGE_MAX, 0,
                   &no_mpls_l2_circuit_with_grp_cw_cmd);
  cli_install_gen (ctree, CONFIG_MODE, PRIVILEGE_MAX, 0,
                   &no_mpls_l2_circuit_cmd);

#if defined (HAVE_MS_PW) && defined (HAVE_VCCV)
  cli_install_gen (ctree, CONFIG_MODE, PRIVILEGE_MAX, 0,
                   &mpls_l2_circuit_mspw_vccv_cmd);
  cli_install_gen (ctree, CONFIG_MODE, PRIVILEGE_MAX, 0,
                   &mpls_l2_circuit_with_grp_mspw_vccv_cmd);
#elif defined HAVE_MS_PW
  cli_install_gen (ctree, CONFIG_MODE, PRIVILEGE_MAX, 0,
                   &mpls_l2_circuit_mspw_cmd);
  cli_install_gen (ctree, CONFIG_MODE, PRIVILEGE_MAX, 0,
                   &mpls_l2_circuit_with_grp_mspw_cmd);
#elif defined HAVE_VCCV
  cli_install_gen (ctree, CONFIG_MODE, PRIVILEGE_MAX, 0,
                   &mpls_l2_circuit_vccv_cmd);
  cli_install_gen (ctree, CONFIG_MODE, PRIVILEGE_MAX, 0,
                   &mpls_l2_circuit_with_grp_vccv_cmd);
#endif /* defined HAVE_MS_PW && defined HAVE_VCCV */

#ifdef HAVE_MS_PW
  cli_install_gen (ctree, CONFIG_MODE, PRIVILEGE_MAX, 0,
                   &mpls_ms_pw_spe_descr_cmd);
  cli_install_gen (ctree, CONFIG_MODE, PRIVILEGE_MAX, 0,
                   &no_mpls_ms_pw_spe_descr_cmd);
  cli_install_gen (ctree, CONFIG_MODE, PRIVILEGE_MAX, 0,
                   &mpls_ms_pw_stitch_vc_cmd);
  cli_install_gen (ctree, CONFIG_MODE, PRIVILEGE_MAX, 0,
                   &no_mpls_ms_pw_stitch_vc_cmd);
#endif /* HAVE_MS_PW */

#endif /* HAVE_MPLS_VC */

#ifdef HAVE_VPLS
  cli_install_gen (ctree, CONFIG_MODE, PRIVILEGE_MAX, 0,
                   &mpls_vpls_cmd);
  cli_install_gen (ctree, CONFIG_MODE, PRIVILEGE_MAX, 0,
                   &mpls_vpls_by_name_cmd);
  cli_install_gen (ctree, CONFIG_MODE, PRIVILEGE_MAX, 0,
                   &no_mpls_vpls_cmd);
  cli_install_gen (ctree, CONFIG_MODE, PRIVILEGE_MAX, 0,
                   &no_mpls_vpls_name_cmd);
#endif /* HAVE_VPLS */

#if (!defined (HAVE_GMPLS) || defined (HAVE_PACKET))
#ifdef HAVE_DIFFSERV
  cli_install_gen (ctree, CONFIG_MODE, PRIVILEGE_MAX, 0,
                   &class_exp_map_config_cmd);
  cli_install_gen (ctree, CONFIG_MODE, PRIVILEGE_MAX, 0,
                   &no_class_exp_map_config_cmd);
  cli_install_gen (ctree, CONFIG_MODE, PRIVILEGE_MAX, 0,
                   &supported_class_cmd);
  cli_install_gen (ctree, CONFIG_MODE, PRIVILEGE_MAX, 0,
                   &no_supported_class_cmd);
  cli_install_gen (ctree, CONFIG_MODE, PRIVILEGE_MAX, 0,
                   &lsp_model_pipe_cmd);
  cli_install_gen (ctree, CONFIG_MODE, PRIVILEGE_MAX, 0,
                   &no_lsp_model_pipe_cmd);
#endif /* HAVE_DIFFSERV */

  cli_install_gen (ctree, CONFIG_MODE, PRIVILEGE_MAX, 0,
                   &propagate_ttl_cmd);
  cli_install_gen (ctree, CONFIG_MODE, PRIVILEGE_MAX, 0,
                   &no_propagate_ttl_cmd);

#ifdef HAVE_DSTE
  cli_install_gen (ctree, CONFIG_MODE, PRIVILEGE_MAX, 0,
                   &class_type_cmd);
  cli_install_gen (ctree, CONFIG_MODE, PRIVILEGE_MAX, 0,
                   &no_class_type_cmd);
  cli_install_gen (ctree, CONFIG_MODE, PRIVILEGE_MAX, 0,
                   &te_class_cmd);
  cli_install_gen (ctree, CONFIG_MODE, PRIVILEGE_MAX, 0,
                   &no_te_class_cmd);
#endif /* HAVE_DSTE */
#endif /* !HAVE_GMPLS || HAVE_PACKET */

#ifdef HAVE_DEV_TEST
  cli_install_gen (ctree, EXEC_MODE, PRIVILEGE_NORMAL, 0,
                   &show_used_labels_cmd);
  cli_install_gen (ctree, EXEC_MODE, PRIVILEGE_NORMAL, 0,
                   &show_ftn_ix_mgr_cmd);
  cli_install_gen (ctree, EXEC_MODE, PRIVILEGE_NORMAL, 0,
                   &show_ftn_ix_mgr_ix_cmd);
  cli_install_gen (ctree, EXEC_MODE, PRIVILEGE_NORMAL, 0,
                   &show_nhlfe_ix_mgr_cmd);
  cli_install_gen (ctree, EXEC_MODE, PRIVILEGE_NORMAL, 0,
                   &show_nhlfe_ix_mgr_ix_cmd);
  cli_install_gen (ctree, EXEC_MODE, PRIVILEGE_NORMAL, 0,
                   &show_xc_ix_mgr_cmd);
  cli_install_gen (ctree, EXEC_MODE, PRIVILEGE_NORMAL, 0,
                   &show_xc_ix_mgr_ix_cmd);
  cli_install_gen (ctree, EXEC_MODE, PRIVILEGE_NORMAL, 0,
                   &mpls_next_ftn_cmd);
  cli_install_gen (ctree, EXEC_MODE, PRIVILEGE_NORMAL, 0,
                   &mpls_next_ftn_ix_cmd);
  cli_install_gen (ctree, EXEC_MODE, PRIVILEGE_NORMAL, 0,
                   &mpls_next_nhlfe_cmd);
  cli_install_gen (ctree, EXEC_MODE, PRIVILEGE_NORMAL, 0,
                   &mpls_next_nhlfe_ix_cmd);
  cli_install_gen (ctree, EXEC_MODE, PRIVILEGE_NORMAL, 0,
                   &mpls_next_xc_cmd);
  cli_install_gen (ctree, EXEC_MODE, PRIVILEGE_NORMAL, 0,
                   &mpls_next_xc1_cmd);
  cli_install_gen (ctree, EXEC_MODE, PRIVILEGE_NORMAL, 0,
                   &mpls_next_xc2_cmd);
  cli_install_gen (ctree, EXEC_MODE, PRIVILEGE_NORMAL, 0,
                   &mpls_next_xc3_cmd);
  cli_install_gen (ctree, EXEC_MODE, PRIVILEGE_NORMAL, 0,
                   &mpls_next_xc4_cmd);
  cli_install_gen (ctree, EXEC_MODE, PRIVILEGE_NORMAL, 0,
                   &mpls_next_ilm_cmd);
  cli_install_gen (ctree, EXEC_MODE, PRIVILEGE_NORMAL, 0,
                   &mpls_next_ilm1_cmd);
  cli_install_gen (ctree, EXEC_MODE, PRIVILEGE_NORMAL, 0,
                   &mpls_next_ilm2_cmd);
#ifdef HAVE_MPLS_FWD
#ifdef HAVE_NSM_MPLS_OAM
  cli_install_gen (ctree, EXEC_MODE, PRIVILEGE_NORMAL, 0,
                   &mpls_oam_test_cmd);
  cli_install_gen (ctree, EXEC_MODE, PRIVILEGE_NORMAL, 0,
                   &clear_mpls_stats_cmd);
#endif /* HAVE_NSM_MPLS_OAM */
#endif /* HAVE_MPLS_FWD */
#endif /* HAVE_DEV_TEST */
  cli_install_gen (ctree, CONFIG_MODE, PRIVILEGE_MAX, 0,
                   &nsm_mpls_ftn_add_cmd);
  cli_install_gen (ctree, CONFIG_MODE, PRIVILEGE_MAX, 0,
                   &nsm_mpls_ftn_mask_add_cmd);
  cli_install_gen (ctree, CONFIG_MODE, PRIVILEGE_MAX, 0,
                   &nsm_mpls_ftn_update_cmd);
  cli_install_gen (ctree, CONFIG_MODE, PRIVILEGE_MAX, 0,
                   &nsm_mpls_ftn_mask_update_cmd);
  cli_install_gen (ctree, CONFIG_MODE, PRIVILEGE_MAX, 0,
                   &nsm_mpls_ftn_del_all_cmd);
  cli_install_gen (ctree, CONFIG_MODE, PRIVILEGE_MAX, 0,
                   &nsm_mpls_ftn_mask_del_all_cmd);
  cli_install_gen (ctree, CONFIG_MODE, PRIVILEGE_MAX, 0,
                   &nsm_mpls_ilm_add_cmd);
  cli_install_gen (ctree, CONFIG_MODE, PRIVILEGE_MAX, 0,
                   &nsm_mpls_ilm_add_pop1_cmd);
#ifdef HAVE_DEV_TEST
  cli_install_gen (ctree, CONFIG_MODE, PRIVILEGE_MAX, 0,
                   &nsm_mpls_ilm_add_pop2_cmd);
#endif /* HAVE_DEV_TEST */
  cli_install_gen (ctree, CONFIG_MODE, PRIVILEGE_MAX, 0,
                   &nsm_mpls_ilm_del_cmd);
  cli_install_gen (ctree, CONFIG_MODE, PRIVILEGE_MAX, 0,
                   &nsm_mpls_ilm_del_pop1_cmd);
#ifdef HAVE_DEV_TEST
  cli_install_gen (ctree, CONFIG_MODE, PRIVILEGE_MAX, 0,
                   &nsm_mpls_ilm_del_pop2_cmd);
#endif /* HAVE_DEV_TEST */
#ifdef HAVE_MPLS_VC
  cli_install_gen (ctree, CONFIG_MODE, PRIVILEGE_MAX, 0,
                   &nsm_mpls_vc_fib_add_cmd);
  cli_install_gen (ctree, CONFIG_MODE, PRIVILEGE_MAX, 0,
                   &nsm_mpls_vc_fib_del_cmd);
#endif /* HAVE_MPLS_VC */
#ifdef HAVE_DEV_TEST
  cli_install_gen (ctree, CONFIG_MODE, PRIVILEGE_MAX, 0,
                   &nsm_mpls_ftn_del2_cmd);
  cli_install_gen (ctree, CONFIG_MODE, PRIVILEGE_MAX, 0,
                   &nsm_mpls_ftn_mask_del2_cmd);
#ifdef HAVE_VRF
  cli_install_gen (ctree, EXEC_MODE, PRIVILEGE_NORMAL, 0,
                   &show_vrf_ix_mgr_cmd);
  cli_install_gen (ctree, EXEC_MODE, PRIVILEGE_NORMAL, 0,
                   &show_vrf_ix_mgr_ix_cmd);
  cli_install_gen (ctree, CONFIG_MODE, PRIVILEGE_MAX, 0,
                   &nsm_mpls_vrf_add_cmd);
  cli_install_gen (ctree, CONFIG_MODE, PRIVILEGE_MAX, 0,
                   &nsm_mpls_vrf_mask_add_cmd);
  cli_install_gen (ctree, CONFIG_MODE, PRIVILEGE_MAX, 0,
                   &nsm_mpls_vrf_modify_cmd);
  cli_install_gen (ctree, CONFIG_MODE, PRIVILEGE_MAX, 0,
                   &nsm_mpls_vrf_mask_modify_cmd);
  cli_install_gen (ctree, CONFIG_MODE, PRIVILEGE_MAX, 0,
                   &nsm_mpls_vrf_del_all_cmd);
  cli_install_gen (ctree, CONFIG_MODE, PRIVILEGE_MAX, 0,
                   &nsm_mpls_vrf_mask_del_all_cmd);
  cli_install_gen (ctree, CONFIG_MODE, PRIVILEGE_MAX, 0,
                   &nsm_mpls_vrf_del2_cmd);
  cli_install_gen (ctree, CONFIG_MODE, PRIVILEGE_MAX, 0,
                   &nsm_mpls_vrf_mask_del2_cmd);
#endif /* HAVE_VRF */
#ifdef HAVE_MPLS_VC
  cli_install_gen (ctree, CONFIG_MODE, PRIVILEGE_MAX, 0,
                   &nsm_mpls_vc_ftn_add_cmd);
  cli_install_gen (ctree, CONFIG_MODE, PRIVILEGE_MAX, 0,
                   &nsm_mpls_vc_ftn_del_all_cmd);
  cli_install_gen (ctree, CONFIG_MODE, PRIVILEGE_MAX, 0,
                   &nsm_mpls_vc_ilm_add_cmd);
  cli_install_gen (ctree, CONFIG_MODE, PRIVILEGE_MAX, 0,
                   &nsm_mpls_vc_ilm_del_cmd);
#endif /* HAVE_MPLS_VC */
#endif /* HAVE_DEV_TEST */
#ifdef HAVE_VPLS
  cli_install_gen (ctree, CONFIG_MODE, PRIVILEGE_MAX, 0,
                   &nsm_vpls_fib_entry_add_cmd);
  cli_install_gen (ctree, CONFIG_MODE, PRIVILEGE_MAX, 0,
                   &nsm_vpls_fib_entry_delete_cmd);

  cli_install_default (ctree, NSM_VPLS_MODE);
  cli_install_gen (ctree, EXEC_MODE, PRIVILEGE_MAX, 0,
                   &clear_mpls_vpls_mac_cmd);
  cli_install_gen (ctree, EXEC_MODE, PRIVILEGE_NORMAL, 0,
                   &show_mpls_vpls_cmd);
  cli_install_gen (ctree, EXEC_MODE, PRIVILEGE_NORMAL, 0,
                   &show_mpls_vpls_detail_cmd);
  cli_install_gen (ctree, EXEC_MODE, PRIVILEGE_NORMAL, 0,
                   &show_mpls_vpls_name_cmd);
  cli_install_gen (ctree, EXEC_MODE, PRIVILEGE_NORMAL, 0,
                   &show_mpls_vpls_name_mesh_cmd);
  cli_install_gen (ctree, EXEC_MODE, PRIVILEGE_NORMAL, 0,
                   &show_mpls_vpls_name_spoke_cmd);
  cli_install_gen (ctree, EXEC_MODE, PRIVILEGE_NORMAL, 0,
                   &show_mpls_vpls_mesh_fwding_cmd);
  cli_install_gen (ctree, EXEC_MODE, PRIVILEGE_NORMAL, 0,
                   &show_mpls_vpls_spoke_fwding_cmd);
  cli_install_gen (ctree, NSM_VPLS_MODE, PRIVILEGE_MAX, 0,
                   &mpls_vpls_peer_cmd);
  cli_install_gen (ctree, NSM_VPLS_MODE, PRIVILEGE_MAX, 0,
                   &no_mpls_vpls_peer_tunnel_cmd);
  cli_install_gen (ctree, NSM_VPLS_MODE, PRIVILEGE_MAX, 0,
                   &mpls_vpls_vc_cmd);
  cli_install_gen (ctree, NSM_VPLS_MODE, PRIVILEGE_MAX, 0,
                   &no_mpls_vpls_vc_cmd);
  cli_install_gen (ctree, NSM_VPLS_MODE, PRIVILEGE_MAX, 0,
                   &mpls_vpls_mtu_cmd);
  cli_install_gen (ctree, NSM_VPLS_MODE, PRIVILEGE_MAX, 0,
                   &mpls_vpls_no_mtu_cmd);
  cli_install_gen (ctree, NSM_VPLS_MODE, PRIVILEGE_MAX, 0,
                   &mpls_vpls_group_cmd);
  cli_install_gen (ctree, NSM_VPLS_MODE, PRIVILEGE_MAX, 0,
                   &mpls_vpls_no_group_cmd);
  cli_install_gen (ctree, NSM_VPLS_MODE, PRIVILEGE_MAX, 0,
                   &mpls_vpls_desc_cmd);
  cli_install_gen (ctree, NSM_VPLS_MODE, PRIVILEGE_MAX, 0,
                   &no_mpls_vpls_desc_cmd);
  cli_install_gen (ctree, NSM_VPLS_MODE, PRIVILEGE_MAX, 0,
                   &mpls_vpls_type_cmd);
  cli_install_gen (ctree, NSM_VPLS_MODE, PRIVILEGE_MAX, 0,
                   &no_mpls_vpls_type_cmd);
#endif /* HAVE_VPLS */

#ifdef HAVE_NSM_MPLS_OAM
  cli_install_hidden (ctree, EXEC_MODE,
		      &enable_mpls_ping_req_dsmap_tlv_cmd);
  cli_install_hidden (ctree, EXEC_MODE,
		      &disable_mpls_ping_req_dsmap_tlv_cmd);
  cli_install_hidden (ctree, EXEC_MODE,
		      &enable_mpls_ping_reply_tlv_cmd);
  cli_install_hidden (ctree, EXEC_MODE,
		      &disable_mpls_ping_reply_tlv_cmd);
#endif /* HAVE_NSM_MPLS_OAM */

#ifdef HAVE_MPLS_OAM_ITUT
  cli_install_gen (ctree, CONFIG_MODE, PRIVILEGE_MAX, 0,
                   &imish_mpls_cv_cmd);
  cli_install_gen (ctree, CONFIG_MODE, PRIVILEGE_MAX, 0,
                   &imish_mpls_ffd_cmd);
  cli_install_gen (ctree, CONFIG_MODE, PRIVILEGE_MAX, 0,
                   &imish_mpls_oam_cmd);
  cli_install_gen (ctree, CONFIG_MODE, PRIVILEGE_MAX, 0,
                   &imish_mpls_fdi_cmd);
  cli_install_gen (ctree, CONFIG_MODE, PRIVILEGE_MAX, 0,
                   &imish_mpls_bdi_cmd);
  cli_install_gen (ctree, CONFIG_MODE, PRIVILEGE_MAX, 0,
  	               &imish_mpls_stats_cmd);
#endif /* HAVE_MPLS_OAM_ITUT*/

#if defined (HAVE_BFD) && defined (HAVE_MPLS_OAM)
  /* Initialize the NSM MPLS BFD commands. */
  nsm_mpls_bfd_cli_init (ctree);
#endif /* HAVE_BFD && HAVE_MPLS_OAM */
}
