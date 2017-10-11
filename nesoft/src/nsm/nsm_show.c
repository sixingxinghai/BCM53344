/* Copyright (C) 2001-2003  All Rights Reserved. */

#include <pal.h>

#include "cli.h"
#include "nexthop.h"
#include "table.h"
#include "message.h"
#include "nsm_message.h"

#include "nsm/nsmd.h"
#include "nsm/nsm_server.h"
#include "nsm/nsm_router.h"
#ifdef HAVE_L3
#include "nsm/rib/nsm_table.h"
#include "nsm/rib/rib.h"
#endif /* HAVE_L3 */
#include "nsm/nsm_fea.h"

#ifdef HAVE_HAL
#include "hal_incl.h"
#endif /* HAVE_HAL */

#include "ptree.h"

#ifdef HAVE_VR
void nsm_vr_show_init (struct cli_tree *);
#endif /* HAVE_VR */

#ifdef HAVE_L3

#ifdef HAVE_RESTART
s_int16_t
nsm_vrep_show_cb (intptr_t usr_ref, char *str)
{
  struct cli *cli = (struct cli *)usr_ref;

  if (! cli)
    return VREP_ERROR;

  cli_out (cli, "%s", str);
  return VREP_SUCCESS;
}
#endif /* HAVE_RESTART */ 

/* Return route type string for CLI output.  */
const char *
route_type_str (u_char type)
{
  switch (type)
    {
    case APN_ROUTE_KERNEL:
      return "kernel";
    case APN_ROUTE_CONNECT:
      return "connected";
    case APN_ROUTE_STATIC:
      return "static";
    case APN_ROUTE_RIP:
    case APN_ROUTE_RIPNG:
      return "rip";
    case APN_ROUTE_OSPF:
    case APN_ROUTE_OSPF6:
      return "ospf";
    case APN_ROUTE_BGP:
      return "bgp";
    case APN_ROUTE_ISIS:
      return "isis";
    default:
      return "unknown";
    }
}

/* Return route type string for CLI output.  */
static const char *
route_type_char (u_char type)
{
  switch (type)
    {
    case APN_ROUTE_KERNEL:
      return "K";
    case APN_ROUTE_CONNECT:
      return "C";
    case APN_ROUTE_STATIC:
      return "S";
    case APN_ROUTE_RIP:
    case APN_ROUTE_RIPNG:
      return "R";
    case APN_ROUTE_OSPF:
    case APN_ROUTE_OSPF6:
      return "O";
    case APN_ROUTE_BGP:
      return "B";
    case APN_ROUTE_ISIS:
      return "i";
    default:
      return "?";
    }
}

/* Return route type string for CLI output.  */
const char *
route_sub_type_char (u_char sub_type)
{
  switch (sub_type)
    {
    case APN_ROUTE_OSPF_IA:
      return "IA";
    case APN_ROUTE_OSPF_NSSA_1:
      return "N1";
    case APN_ROUTE_OSPF_NSSA_2:
      return "N2";
    case APN_ROUTE_OSPF_EXTERNAL_1:
      return "E1";
    case APN_ROUTE_OSPF_EXTERNAL_2:
      return "E2";
    case APN_ROUTE_ISIS_L1:
      return "L1";
    case APN_ROUTE_ISIS_L2:
      return "L2";
    case APN_ROUTE_ISIS_IA:
      return "ia";
    case APN_ROUTE_BGP_MPLS:
    default:
      return "  ";
    }
}

/* Utility function to display uptime.  */
static void
nsm_show_uptime (struct cli *cli, struct rib *rib, char *prefix, char *postfix)
{
  pal_time_t uptime;
  struct pal_tm tmp;
  struct pal_tm *tm;
#define ONE_DAY_SECOND           60*60*24
#define ONE_WEEK_SECOND          60*60*24*7

  uptime = pal_time_current (NULL);
  uptime -= rib->uptime;
  pal_time_gmt (&uptime, &tmp);
  tm = &tmp;

  if (prefix)
    cli_out (cli, "%s", prefix);

  if (uptime < ONE_DAY_SECOND)
    cli_out (cli,  "%02d:%02d:%02d",
             tm->tm_hour, tm->tm_min, tm->tm_sec);
  else if (uptime < ONE_WEEK_SECOND)
    cli_out (cli, "%dd%02dh%02dm",
             tm->tm_yday, tm->tm_hour, tm->tm_min);
  else
    cli_out (cli, "%02dw%dd%02dh",
             tm->tm_yday / 7,
             tm->tm_yday - ((tm->tm_yday / 7) * 7), tm->tm_hour);

  if (postfix)
    cli_out (cli, "%s", postfix);
}

/* Utility function to display IPv4 routing entry.  */
static int
nsm_show_ipv4_route (struct cli *cli, struct prefix *p, struct rib *rib,
                     int database)
{
  struct apn_vr *vr = cli->vr;
  struct nexthop *nexthop;
  struct interface *ifp = NULL;
  struct nexthop *rhop;
  struct rib *rrib = NULL;
  int last_resort;
  int first;
  int count;
  int len = 0;
  int nhdislen = 0;

  /* First line display route type character and prefix.  */  
  first = 1;
  count = 0;

  /* Default route check.  */
  if (! database && p->prefixlen == 0 && p->u.prefix4.s_addr == 0)
    last_resort = 1;
  else
    last_resort = 0;

  /* Walk nexthop entries.  */
  for (nexthop = rib->nexthop; nexthop; nexthop = nexthop->next)
    {
      nhdislen = 0;
      /* Display forwarding table information only unless database
         option is specified.  */
      if (! database
          && ! (CHECK_FLAG (nexthop->flags, NEXTHOP_FLAG_FIB)
                && CHECK_FLAG (rib->flags, RIB_FLAG_SELECTED)))
        continue;

      /* First entry display all of information.  */
      if (first)
        {
          /* Route type and sub type.  */
          len = cli_out (cli, "%s%c%s ",
                         route_type_char (rib->type),
                         last_resort ? '*' : ' ',
                         route_sub_type_char (rib->sub_type));

          /* Display FIB status when database flag is specified.  */
          if (database)
            len += cli_out (cli, "%c%c%c",
                            CHECK_FLAG (nexthop->flags, NEXTHOP_FLAG_FIB)
                            ? '*' : ' ',
                            CHECK_FLAG (rib->flags, RIB_FLAG_SELECTED)
                            ? '>' : ' ',
                            CHECK_FLAG (rib->flags, RIB_FLAG_STALE)
                            ? 'p' : ' ');
          else
            len += cli_out (cli, "   ");

          /* Prefix.  */
          len += cli_out (cli, "%r/%d", &p->u.prefix4, p->prefixlen);

          /* First line is displayed.  len will be used by after
             second line.  */
          first = 0;
        }
      else
        {
          /* Display FIB status when database flag is specified.  */
          if (database)
            cli_out (cli, "     %c%c%c",
                     CHECK_FLAG (nexthop->flags, NEXTHOP_FLAG_FIB)
                     ? '*' : ' ',
                     CHECK_FLAG (rib->flags, RIB_FLAG_SELECTED)
                     ? '>' : ' ',
                     CHECK_FLAG (rib->flags, RIB_FLAG_STALE)
                     ? 'p' : ' ');
          else
            cli_out (cli, "        ");

          /* Padding.  */
          cli_out (cli, "%*c", len - 8, ' ');
        }

      /* Distance and metric display.  */
      if (rib->type != APN_ROUTE_CONNECT && rib->type != APN_ROUTE_KERNEL)
        nhdislen += cli_out (cli, " [%d/%lu]", rib->distance, rib->metric);

      switch (nexthop->type)
        {
        case NEXTHOP_TYPE_IPV4:
        case NEXTHOP_TYPE_IPV4_IFINDEX:
        case NEXTHOP_TYPE_IPV4_IFNAME:
          {
            nhdislen += cli_out (cli, " via %r", &nexthop->gate.ipv4);

            if (CHECK_FLAG (rib->flags, RIB_FLAG_BLACKHOLE))
              nhdislen += cli_out (cli, ", Null0");
            else if (nexthop->ifname)
              nhdislen += cli_out (cli, ", %s", nexthop->ifname);
            else if (nexthop->ifindex)
              nhdislen += cli_out (cli, ", %s",
                       if_index2name (&vr->ifm, nexthop->ifindex));
            break;
          }
#ifdef HAVE_VRF
        case NEXTHOP_TYPE_MPLS:
          {
            IF_NSM_CAP_HAVE_VRF
              {
                nhdislen += cli_out (cli, " MPLS nexthop %r", &nexthop->gate.ipv4);
              }
            break;
          }
#endif /* HAVE_VRF */
        case NEXTHOP_TYPE_IFINDEX:
          ifp = if_lookup_by_index (&vr->ifm, nexthop->ifindex);

          if (ifp == NULL)
            nhdislen += cli_out (cli, " is incomplete");
          else if (! (ifp->flags & IFF_POINTOPOINT))
            nhdislen += cli_out (cli, " is directly connected, %s",
                                 if_index2name (&vr->ifm, nexthop->ifindex));
          else if (if_match_ifc_ipv4_direct (ifp, p))
            nhdislen += cli_out (cli, " is directly connected, %s",
                                 if_index2name (&vr->ifm, nexthop->ifindex));
          else
            nhdislen += cli_out (cli, " via %s",
                                 if_index2name (&vr->ifm, nexthop->ifindex));
          break;
        case NEXTHOP_TYPE_IFNAME:
          if (IS_NULL_INTERFACE_STR (nexthop->ifname))
            nhdislen += cli_out (cli, " is a summary, %s", nexthop->ifname);
          else
            nhdislen += cli_out (cli, " is directly connected, %s",
                                 nexthop->ifname);
          break;
        default:
          break;
        }
      if (! CHECK_FLAG (nexthop->flags, NEXTHOP_FLAG_ACTIVE))
        nhdislen += cli_out (cli, " inactive");

      if (CHECK_FLAG (nexthop->flags, NEXTHOP_FLAG_RECURSIVE))
        {
          if (!nexthop->rrn || !nexthop->rrn->info)
            {
              cli_out (cli, "\n");
              continue;
            }
          /* find the selected rib */
          for (rrib= nexthop->rrn->info; rrib; rrib = rrib->next)
            if (CHECK_FLAG (rrib->flags, RIB_FLAG_SELECTED))
              break;

          /* if selected rib is not found */
          if (!rrib || !rrib->nexthop)
            continue;

          
          nhdislen += cli_out (cli, " (recursive");

          for (rhop = rrib->nexthop; rhop; rhop = rhop->next)
             {
               if (! database &&
                   ! (CHECK_FLAG (rhop->flags, NEXTHOP_FLAG_FIB)) )
                 {
                   if (rhop->next == NULL)
                      cli_out (cli, ")");
                   continue;
                 }

               switch (rhop->type)
                 {
                   case NEXTHOP_TYPE_IPV4:
                   case NEXTHOP_TYPE_IPV4_IFINDEX:
                   case NEXTHOP_TYPE_IPV4_IFNAME:
                     {
                       if ( !CHECK_FLAG (rhop->flags, NEXTHOP_FLAG_ACTIVE) ) 
                         cli_out (cli, " via %r inactive", &rhop->gate.ipv4);
                       else
                         cli_out (cli, " via %r",&rhop->gate.ipv4);
                       break;
                     }
#ifdef HAVE_VRF
                   case NEXTHOP_TYPE_MPLS:
                     {
                       IF_NSM_CAP_HAVE_VRF
                         {
                           cli_out (cli, " MPLS nexthop %r)", &rhop->gate.ipv4);
                         }
                       break;
                     }
#endif /* HAVE_VRF */
                   case NEXTHOP_TYPE_IFINDEX:
                   case NEXTHOP_TYPE_IFNAME:
                     if (CHECK_FLAG (nexthop->flags, 
                                     NEXTHOP_FLAG_RECURSIVE_BLACKHOLE))
                       cli_out (cli, " blackhole)");
                     else
                     cli_out (cli, " is directly connected, %s)",
                              if_index2name (&vr->ifm, rhop->ifindex));
                                                                                                                                                                                   break;
                   default:
                   break;
                 }
                 if (rhop->next
                     && ((CHECK_FLAG (rhop->next->flags, NEXTHOP_FLAG_FIB))
                          || database ))
                   {
                     cli_out (cli, "\n");
                     cli_out (cli, "%*c", nhdislen + len , ' ');
                   }
                     if ( !rhop->next )
                       cli_out (cli, " )");
             }


#if 0
          switch (nexthop->rtype)
            {
            case NEXTHOP_TYPE_IPV4:
            case NEXTHOP_TYPE_IPV4_IFINDEX:
            case NEXTHOP_TYPE_IPV4_IFNAME:
                {
                  cli_out (cli, " via %r)", &nexthop->rgate.ipv4);
                  break;
                }
#ifdef HAVE_VRF
            case NEXTHOP_TYPE_MPLS:
                {
                  IF_NSM_CAP_HAVE_VRF
                    {
                      cli_out (cli, " MPLS nexthop %r)", &nexthop->rgate.ipv4);
                    }
                  break;
                }
#endif /* HAVE_VRF */
            case NEXTHOP_TYPE_IFINDEX:
            case NEXTHOP_TYPE_IFNAME:
              if (CHECK_FLAG (nexthop->flags, NEXTHOP_FLAG_RECURSIVE_BLACKHOLE))
                cli_out (cli, " blackhole)");
              else
                cli_out (cli, " is directly connected, %s)",
                    if_index2name (&vr->ifm, nexthop->rifindex));
              break;
            default:
              break;
            }
#endif /*if 0 */
        }
      if (rib->type == APN_ROUTE_RIP
          || rib->type == APN_ROUTE_OSPF
          || rib->type == APN_ROUTE_BGP
          || rib->type == APN_ROUTE_ISIS)
        nsm_show_uptime (cli, rib, ", ", NULL);

      cli_out (cli, "\n");

      count++;
    }
  return count;
}

/* Show default route as gatey of last resort.  */
static void
nsm_show_ipv4_default (struct cli *cli, struct rib *rib)
{
  int exist;
  struct nexthop *nexthop;

  exist = 0;

  for (nexthop = rib->nexthop; nexthop; nexthop = nexthop->next)
    if (CHECK_FLAG (nexthop->flags, NEXTHOP_FLAG_FIB)
        && CHECK_FLAG (rib->flags, RIB_FLAG_SELECTED))
      {
        switch (nexthop->type)
          {
          case NEXTHOP_TYPE_IPV4:
          case NEXTHOP_TYPE_IPV4_IFINDEX:
          case NEXTHOP_TYPE_IPV4_IFNAME:
            cli_out (cli,
                     "Gateway of last resort is %r to network 0.0.0.0\n",
                     &nexthop->gate.ipv4);
            break;
          default:
            cli_out (cli,
                     "Gateway of last resort is 0.0.0.0 to network 0.0.0.0\n");
            break;
          }
        exist = 1;
        break;
      }

  if (! exist)
    cli_out (cli, "Gateway of last resort is not set\n");

  cli_out (cli, "\n");
}

/* Show RIB table callback.  */
static int
nsm_show_callback (struct cli *cli)
{
  struct nsm_ptree_node *rn;
  struct rib *rib;
  int header = 0;
  int count = 0;
  int database;
  struct prefix p;

  /* "show ip route" common headers.  */
  const char nsm_show_header[] =
    "Codes: K - kernel, C - connected, S - static, R - RIP, B - BGP\n"
    "       O - OSPF, IA - OSPF inter area\n"
    "       N1 - OSPF NSSA external type 1, N2 - OSPF NSSA external type 2\n"
    "       E1 - OSPF external type 1, E2 - OSPF external type 2\n"
    "       i - IS-IS, L1 - IS-IS level-1, L2 - IS-IS level-2,"
    " ia - IS-IS inter area\n";

  /* Normal case.  */
  const char header_normal[] =
    "       * - candidate default\n\n";

  /* Database case.  */
  const char header_database[] =
    "       > - selected route, * - FIB route, p - stale info\n\n";

  /* Fetch current node.  */
  rn = cli->current;

  /* Check output style.  */
  if (cli->arg)
    database = 1;
  else
    database = 0;

  /* "show" connection is closed. */
  if (cli->status == CLI_CLOSE)
    goto cleanup;

  /* This is first time.  */
  if (cli->status == CLI_NORMAL)
    {
      /* When route type is specified, do not display header.  */
      if (! cli->type)
        header = 1;

      cli->count = 0;
    }

  /* Walk routing table.  */
  for (; rn; rn = nsm_ptree_next (rn))
    if (rn->info != NULL)
      {
        for (rib = rn->info; rib; rib = rib->next)
          {
            /* Filter type.  */
            if (cli->type && rib->type != cli->type)
              continue;
             pal_mem_set(&p, 0, sizeof(struct prefix));
             p.prefixlen = rn->key_len;
             NSM_PTREE_KEY_COPY(&p.u.prefix4, rn);
        
            /* Display header string.  */
            if (header)
              {
                cli_out (cli, "%s", nsm_show_header);

                if (database)
                  cli_out (cli, "%s", header_database);
                else
                  cli_out (cli, "%s", header_normal);
                
             /* Gateway of last resort.  */
                if (! database
                    && p.prefixlen == 0 && p.u.prefix4.s_addr == 0)
                  nsm_show_ipv4_default (cli, rib);

                /* Turn off header.  */
                header = 0;
              }

            /* Display route.  */
            count += nsm_show_ipv4_route (cli, &p, rib, database);
          }

        if (count >= 20)
          {
            cli->status = CLI_CONTINUE;
            cli->count += count;
            cli->current = nsm_ptree_next (rn);
            cli->callback = nsm_show_callback;
            return 0;
          }
      }

  /* Clean up.  */
 cleanup:

  /* Unlock the node.  */
  if (rn)
    nsm_ptree_unlock_node (rn);

  /* Call clean up routine.  */
  if (cli->cleanup)
    (*cli->cleanup) (cli);

  /* Set NULL to callback pointer.  */
  cli->status = CLI_CONTINUE;
  cli->count += count;
  cli->callback = NULL;

  return 0;
}

/* Show routing table.  */
void
nsm_show_ipv4 (struct cli *cli, struct nsm_vrf *vrf, int database, u_char type)
{
  /* Set top node to current pointer in CLI structure.  */
  cli->afi = AFI_IP;
  cli->safi = SAFI_UNICAST;
  cli->current = nsm_ptree_top (vrf->RIB_IPV4_UNICAST);
  cli->type = type;

  /* When database output is enabled put a value to cli->arg.  */
  if (database)
    cli->arg = vrf;
  else
    cli->arg = NULL;

  /* Call display function.  */
  nsm_show_callback (cli);
}

#ifdef HAVE_IPV6
/* IPv6 "show ipv6 route" commands.  */

/* Utility function to display IPv6 route entry.  */
static int
nsm_show_ipv6_route (struct cli *cli, struct prefix *p, struct rib *rib,
                     int database)
{
  struct apn_vr *vr = cli->vr;
  struct nexthop *nexthop;
  int len = 0;
  int first = 1;
  int count = 0;

  /* Nexthop information. */
  for (nexthop = rib->nexthop; nexthop; nexthop = nexthop->next)
    {
      /* Display forwarding table information only unless database
         option is specified.  */
      if (! database
          && ! (CHECK_FLAG (nexthop->flags, NEXTHOP_FLAG_FIB)
                && CHECK_FLAG (rib->flags, RIB_FLAG_SELECTED)))
        continue;

      if (first)
        {
          /* Route type information. */
          len = cli_out (cli, "%s",
                         route_type_char (rib->type));

          if (database)
            len += cli_out (cli, "%c%c%c",
                           CHECK_FLAG (nexthop->flags, NEXTHOP_FLAG_FIB)
                           ? '*' : ' ',
                           CHECK_FLAG (rib->flags, RIB_FLAG_SELECTED)
                           ? '>' : ' ',
                           CHECK_FLAG (rib->flags, RIB_FLAG_STALE)
                           ? 'p' : ' ');
          else
            len += cli_out (cli, "   ");

          /* Prefix information. */
          len += cli_out (cli, "%R/%d", &p->u.prefix6, p->prefixlen);

          first = 0;
        }
      else 
        {
          if (database)
             cli_out (cli, " %c%c%c",
                      CHECK_FLAG (nexthop->flags, NEXTHOP_FLAG_FIB)
                      ? '*' : ' ',
                      CHECK_FLAG (rib->flags, RIB_FLAG_SELECTED)
                      ? '>' : ' ',
                      CHECK_FLAG (rib->flags, RIB_FLAG_STALE)
                      ? 'p' : ' ');

          else
            cli_out (cli, "    ");

           cli_out (cli, "%*c", len - 4, ' ');
         }

       /* Distance and metric display. */
       if (rib->type != APN_ROUTE_CONNECT && rib->type != APN_ROUTE_KERNEL)
          cli_out (cli, " [%d/%lu]", rib->distance,  rib->metric);

      switch (nexthop->type)
        {
        case NEXTHOP_TYPE_IPV6:
        case NEXTHOP_TYPE_IPV6_IFINDEX:
        case NEXTHOP_TYPE_IPV6_IFNAME:
          cli_out (cli, " via %R", &nexthop->gate.ipv6);

          if (nexthop->type == NEXTHOP_TYPE_IPV6_IFNAME)
            cli_out (cli, ", %s", nexthop->ifname);
          else if (nexthop->ifindex)
            cli_out (cli, ", %s",
                     if_index2name (&vr->ifm, nexthop->ifindex));
          break;
        case NEXTHOP_TYPE_IFINDEX:
          cli_out (cli, " via ::, %s",
                   if_index2name (&vr->ifm, nexthop->ifindex));
          break;
        case NEXTHOP_TYPE_IFNAME:
          cli_out (cli, " via ::, %s",
                   nexthop->ifname);
          break;
        default:
          break;
        }
      if (! CHECK_FLAG (nexthop->flags, NEXTHOP_FLAG_ACTIVE))
        cli_out (cli, " inactive");

      if (CHECK_FLAG (nexthop->flags, NEXTHOP_FLAG_RECURSIVE))
        {
          cli_out (cli, " (recursive");

          switch (nexthop->rtype)
            {
            case NEXTHOP_TYPE_IPV6:
            case NEXTHOP_TYPE_IPV6_IFINDEX:
            case NEXTHOP_TYPE_IPV6_IFNAME:
              cli_out (cli, " via %R)", &nexthop->rgate.ipv6);
              if (nexthop->rifindex)
                cli_out (cli, ", %s",
                    if_index2name (&vr->ifm, nexthop->rifindex));
              break;
            case NEXTHOP_TYPE_IFINDEX:
            case NEXTHOP_TYPE_IFNAME:
              if (CHECK_FLAG (nexthop->flags, NEXTHOP_FLAG_RECURSIVE_BLACKHOLE))
                cli_out (cli, " blackhole)");
              else
                cli_out (cli, " via ::, %s)",
                    if_index2name (&vr->ifm, nexthop->rifindex));
              break;
            default:
              break;
            }
        }

      nsm_show_uptime (cli, rib, ", ", "\n");

      count++;
    }
  return count;
}
/* Show RIB table callback.  */
static int
nsm_show_callback_ipv6 (struct cli *cli)
{
  struct nsm_ptree_node *rn;
  struct rib *rib;
  int header = 0;
  int count = 0;
  int database;
  struct prefix p;

  /* "show ipv6 route" common headers.  */
  const char nsm_show_header[] =
    "IPv6 Routing Table\n"
    "Codes: K - kernel route, C - connected, S - static, R - RIP, O - OSPF,\n"
    "       I - IS-IS, B - BGP\n";

  /* Database case.  */
  const char header_database[] =
    "       > - selected route, * - FIB route, p - stale info\n";

  /* Timers.  */
  const char nsm_show_header2[] =
    "Timers: Uptime\n\n";

  /* Fetch current node.  */
  rn = cli->current;

  /* Check output style.  */
  if (cli->arg)
    database = 1;
  else
    database = 0;

  /* "show" connection is closed. */
  if (cli->status == CLI_CLOSE)
    goto cleanup;

  /* This is first time.  */
  if (cli->status == CLI_NORMAL)
    {
      header = 1;
      cli->count = 0;
    }

  /* Walk routing table.  */
  for (; rn; rn = nsm_ptree_next (rn))
    if (rn->info != NULL)
      {
        for (rib = rn->info; rib; rib = rib->next)
          {
            /* Filter type.  */
            if (cli->type && rib->type != cli->type)
              continue;

            /* Display header string.  */
            if (header)
              {
                cli_out (cli, "%s", nsm_show_header);
                if (database)
                  cli_out (cli, "%s", header_database);
                cli_out (cli, "%s", nsm_show_header2);

                /* Turn off header.  */
                header = 0;
              }
            pal_mem_set(&p, 0, sizeof(struct prefix));
            p.prefixlen = rn->key_len;
            NSM_PTREE_KEY_COPY (&p.u.prefix6, rn);
            
            /* Display route.  */
            count += nsm_show_ipv6_route (cli, &p, rib, database);
          }

        if (count >= 20)
          {
            cli->status = CLI_CONTINUE;
            cli->count += count;
            cli->current = nsm_ptree_next (rn);
            cli->callback = nsm_show_callback_ipv6;
            return 0;
          }
      }

  /* Clean up.  */
 cleanup:

  /* Unlock the node.  */
  if (rn)
    nsm_ptree_unlock_node (rn);

  /* Call clean up routine.  */
  if (cli->cleanup)
    (*cli->cleanup) (cli);

  /* Set NULL to callback pointer.  */
  cli->status = CLI_CONTINUE;
  cli->count += count;
  cli->callback = NULL;

  return 0;
}

/* Show routing table.  */
static void
nsm_show_ipv6 (struct cli *cli, struct nsm_vrf *vrf, int database, u_char type)
{
  /* Set top node to current pointer in CLI structure.  */
  cli->afi = AFI_IP6;
  cli->safi = SAFI_UNICAST;
  cli->current = nsm_ptree_top (vrf->RIB_IPV6_UNICAST);
  cli->type = type;

  /* When database output is enabled put a value to cli->arg.  */
  if (database)
    cli->arg = vrf;
  else
    cli->arg = NULL;

  /* Call display function.  */
  nsm_show_callback_ipv6 (cli);
}
#endif

void
nsm_show_router_id (struct cli *cli, struct nsm_vrf *nv)
{
  struct apn_vrf *iv = nv->vrf;

  if (nv->router_id_type == NSM_ROUTER_ID_CONFIG)
    cli_out (cli, "Router ID: %r (config)\n", &iv->router_id);
  else if (nv->router_id_type == NSM_ROUTER_ID_AUTOMATIC)
    cli_out (cli, "Router ID: %r (automatic)\n", &iv->router_id);
  else if (nv->router_id_type == NSM_ROUTER_ID_AUTOMATIC_LOOPBACK)
    cli_out (cli, "Router ID: %r (loopback)\n", &iv->router_id);
  else
    cli_out (cli, "Router ID is not set\n");
}

int
nsm_cli_str2route_type (afi_t afi, char *str)
{
  int type = -1;

  if (afi != AFI_IP && afi != AFI_IP6)
    return -1;

  if (pal_strncmp (str, "b", 1) == 0)
    type = APN_ROUTE_BGP;
  else if (pal_strncmp (str, "c", 1) == 0)
    type = APN_ROUTE_CONNECT;
  else if (pal_strncmp (str, "k", 1) ==0)
    type = APN_ROUTE_KERNEL;
  else if (pal_strncmp (str, "o", 1) == 0)
    {
      if (afi == AFI_IP)
        type = APN_ROUTE_OSPF;
      else
        type = APN_ROUTE_OSPF6;
    }
  else if (pal_strncmp (str, "i", 1) == 0)
    type = APN_ROUTE_ISIS;
  else if (pal_strncmp (str, "r", 1) == 0)
    {
      if (afi == AFI_IP)
        type = APN_ROUTE_RIP;
      else
        type = APN_ROUTE_RIPNG;
    }
  else if (pal_strncmp (str, "s", 1) == 0)
    type = APN_ROUTE_STATIC;

  return type;
}

CLI (show_ip_route,
     show_ip_route_cmd,
     "show ip route (database|)",
     CLI_SHOW_STR,
     CLI_IP_STR,
     "IP routing table",
     "IP routing table database")
{
  struct nsm_master *nm = cli->vr->proto;
  struct nsm_vrf *vrf;

  /* Get default VRF. */
  vrf = nsm_vrf_lookup_by_id (nm, 0);
  if (vrf == NULL)
    {
      cli_out (cli, "%% Main table does not exist\n");
      return CLI_ERROR;
    }

  nsm_show_ipv4 (cli, vrf, argc, 0);

  return CLI_SUCCESS;
}

CLI (show_ip_route_protocol,
     show_ip_route_protocol_cmd,
     "show ip route (database|) (bgp|connected|isis|kernel|ospf|rip|static)",
     CLI_SHOW_STR,
     CLI_IP_STR,
     "IP routing table",
     "IP routing table database",
     "Border Gateway Protocol (BGP)",
     "Connected",
     "ISO IS-IS",
     "Kernel",
     "Open Shortest Path First (OSPF)",
     "Routing Information Protocol (RIP)",
     "Static routes")
{
  struct nsm_master *nm = cli->vr->proto;
  struct nsm_vrf *vrf;
  int type;

  vrf = nsm_vrf_lookup_by_id (nm, 0);
  if (vrf == NULL)
    return CLI_ERROR;

  if ((type = nsm_cli_str2route_type (AFI_IP, argv[argc - 1])) < 0)
    {
      cli_out (cli, "%% Unknown route type\n");
      return CLI_ERROR;
    }

  nsm_show_ipv4 (cli, vrf, argc - 1, type);

  return CLI_SUCCESS;
}

CLI (show_ip_route_summary,
     show_ip_route_summary_cmd,
     "show ip route summary",
     CLI_SHOW_STR,
     CLI_IP_STR,
     "IP routing table",
     "Summary of all routes")
{
  struct nsm_master *nm = cli->vr->proto;
  struct nsm_vrf *vrf;
  struct nsm_vrf_afi *nva;
  int type;
  u_int32_t total = 0;

  vrf = nsm_vrf_lookup_by_id (nm, 0);
  if (vrf == NULL)
    return CLI_ERROR;

  nva = &vrf->afi[AFI_IP];

  cli_out (cli, "IP routing table name is Default-IP-Routing-Table(0)\n");
  cli_out (cli, "IP routing table maximum-paths is %d\n", nm->multipath_num);
  cli_out (cli, "Route Source    Networks\n");

  for (type = 0; type < APN_ROUTE_MAX; type++)
    if (nva->counter.type[type] > 0)
      {
        cli_out (cli, "%-15s %u\n",
                 route_type_str (type), nva->counter.type[type]);
        total += nva->counter.type[type];
      }

  cli_out (cli, "%-15s %u\n", "Total", total);
  cli_out (cli, "%-15s %u\n", "FIB", nva->counter.fib);

  return CLI_SUCCESS;
}

#ifdef HAVE_VRF
/* "show ip route vrf" */
CLI (show_ip_route_vrf,
     show_ip_route_vrf_cmd,
     "show ip route vrf WORD (database|)",
     CLI_SHOW_STR,
     CLI_IP_STR,
     "IP routing table",
     CLI_VRF_DISP_STR,
     CLI_VRF_NAME_STR,
     "IP routing table database")
{
  struct nsm_master *nm = cli->vr->proto;
  struct nsm_vrf *vrf;

  vrf = nsm_vrf_lookup_by_name (nm, argv[0]);
  if (vrf == NULL)
    {
      cli_out(cli, "%% IP routing table %s does not exist\n", argv[0]);
      return CLI_ERROR;
    }

  nsm_show_ipv4 (cli, vrf, argc - 1, 0);

  return CLI_SUCCESS;
}

CLI (show_ip_route_vrf_protocol,
     show_ip_route_vrf_protocol_cmd,
     "show ip route vrf WORD (database|) (bgp|connected|isis|kernel|ospf|rip|static)",
     CLI_SHOW_STR,
     CLI_IP_STR,
     "IP routing table",
     CLI_VRF_DISP_STR,
     CLI_VRF_NAME_STR,
     "IP routing table database",
     "Border Gateway Protocol (BGP)",
     "Connected",
     "ISO IS-IS",
     "Kernel",
     "Open Shortest Path First (OSPF)",
     "Routing Information Protocol (RIP)",
     "Static routes")
{
  struct nsm_master *nm = cli->vr->proto;
  struct nsm_vrf *vrf;
  int type;

  vrf = nsm_vrf_lookup_by_name (nm, argv[0]);
  if (! vrf)
    {
      cli_out(cli, "%% IP routing table %s does not exist\n", argv[0]);
      return CLI_ERROR;
    }

  if ((type = nsm_cli_str2route_type (AFI_IP, argv[argc - 1])) < 0)
    {
      cli_out (cli, "%% Unknown route type\n");
      return CLI_ERROR;
    }

  nsm_show_ipv4 (cli, vrf, argc - 2, type);

  return CLI_SUCCESS;
}

result_t
nsm_cli_show_ip_vrf_detail (struct cli *cli, struct nsm_vrf *nv)
{
  struct interface *ifp;
  struct route_node *rn;

  cli_out (cli, "VRF %s, FIB ID %u\n", nv->vrf->name, nv->vrf->fib_id);

  if (nv->desc)
    cli_out (cli, "Description: %s\n", nv->desc);

  /* Router ID.  */
  nsm_show_router_id (cli, nv);

  cli_out (cli, "Interfaces:\n");
  for (rn = route_top (nv->vrf->ifv.if_table); rn; rn = route_next (rn))
    if ((ifp = rn->info))
      cli_out (cli, "  %s\n", ifp->name);

  return CLI_SUCCESS;
}

CLI (show_ip_vrf_name,
     show_ip_vrf_name_cmd,
     "show ip vrf WORD",
     CLI_SHOW_STR,
     CLI_IP_STR,
     CLI_VRF_STR,
     CLI_VRF_NAME_STR)
{
  struct nsm_master *nm = cli->vr->proto;
  struct nsm_vrf *vrf;

  vrf = nsm_vrf_lookup_by_name (nm, argv[0]);
  if (vrf == NULL)
    {
      cli_out(cli, "No such VRF is defined\n");
      return CLI_ERROR;
    }

  nsm_cli_show_ip_vrf_detail (cli, vrf);

  return CLI_SUCCESS;
}

CLI (show_ip_vrf,
     show_ip_vrf_cmd,
     "show ip vrf",
     CLI_SHOW_STR,
     CLI_IP_STR,
     CLI_VRF_STR)
{
  struct apn_vr *vr = cli->vr;
  struct apn_vrf *iv;
  int i;

  for (i = 1; i < vector_max (vr->vrf_vec); i++)
    if ((iv = vector_slot (vr->vrf_vec, i)))
      {
        nsm_cli_show_ip_vrf_detail (cli, iv->proto);
        cli_out(cli, "!\n");
      }

  return CLI_SUCCESS;
}

#ifdef HAVE_IPV6 /*6VPE*/
/* "show ipv6 route vrf" */
CLI (show_ipv6_route_vrf,
     show_ipv6_route_vrf_cmd,
     "show ipv6 route vrf WORD (database|)",
     CLI_SHOW_STR,
     CLI_IPV6_STR,
     "IP routing table",
     CLI_VRF_DISP_STR,
     CLI_VRF_NAME_STR,
     "IPV6 routing table database")
{
  struct nsm_master *nm = cli->vr->proto;
  struct nsm_vrf *vrf;

  vrf = nsm_vrf_lookup_by_name (nm, argv[0]);
  if (vrf == NULL)
    {
      cli_out(cli, "%% IPV6 routing table %s does not exist\n", argv[0]);
      return CLI_ERROR;
    }

  nsm_show_ipv6 (cli, vrf, argc - 1, 0);

  return CLI_SUCCESS;
}

CLI (show_ipv6_route_vrf_protocol,
     show_ipv6_route_vrf_protocol_cmd,
     "show ipv6 route vrf WORD (database|) (bgp|connected|isis|kernel|ospf|rip|static)",
     CLI_SHOW_STR,
     CLI_IPV6_STR,
     "IP routing table",
     CLI_VRF_DISP_STR,
     CLI_VRF_NAME_STR,
     "IPv6 routing table database",
     "Border Gateway Protocol (BGP)",
     "Connected",
     "ISO IS-IS",
     "Kernel",
     "Open Shortest Path First (OSPF)",
     "Routing Information Protocol (RIP)",
     "Static routes")
{
  struct nsm_master *nm = cli->vr->proto;
  struct nsm_vrf *vrf;
  int type;

  vrf = nsm_vrf_lookup_by_name (nm, argv[0]);
  if (! vrf)
    {
      cli_out(cli, "%% IP routing table %s does not exist\n", argv[0]);
      return CLI_ERROR;
    }

  if ((type = nsm_cli_str2route_type (AFI_IP6, argv[argc - 1])) < 0)
    {
      cli_out (cli, "%% Unknown route type\n");
      return CLI_ERROR;
    }

  nsm_show_ipv6 (cli, vrf, argc - 2, type);

  return CLI_SUCCESS;
}

#endif /*HAVE_IPV6*/
#endif /* HAVE_VRF */


/* New RIB.  Detailed information for IPv4 route. */
static void
nsm_show_ip_route_detail (struct cli *cli, struct nsm_ptree_node *rn)
{
  struct apn_vr *vr = cli->vr;
  struct rib *rib;
  struct nexthop *nexthop;
  struct interface *ifp = NULL;
  struct prefix p;
  struct rib *rrib = NULL;
  struct nexthop *rhop;
  int nhdislen = 0;

  for (rib = rn->info; rib; rib = rib->next)
    {
      pal_mem_set(&p, 0, sizeof(struct prefix));
      p.prefixlen = rn->key_len;
      NSM_PTREE_KEY_COPY(&p.u.prefix4, rn);

      cli_out (cli, "Routing entry for %r/%d\n",
               &p.u.prefix4, p.prefixlen);
      cli_out (cli, "  Known via \"%s\"", route_type_str (rib->type));
      cli_out (cli, ", distance %d, metric %d", rib->distance, rib->metric);
      if (CHECK_FLAG (rib->flags, RIB_FLAG_SELECTED))
        cli_out (cli, ", best");
      cli_out (cli, "\n");

      if (rib->type == APN_ROUTE_RIP
          || rib->type == APN_ROUTE_OSPF
          || rib->type == APN_ROUTE_BGP
          || rib->type == APN_ROUTE_ISIS)
        nsm_show_uptime (cli, rib, "  Last update ", " ago\n");

      for (nexthop = rib->nexthop; nexthop; nexthop = nexthop->next)
        {
          nhdislen += cli_out (cli, "  %c",
                               CHECK_FLAG (nexthop->flags,
                                            NEXTHOP_FLAG_FIB) ? '*' : ' ');

          switch (nexthop->type)
            {
            case NEXTHOP_TYPE_IPV4:
            case NEXTHOP_TYPE_IPV4_IFINDEX:
            case NEXTHOP_TYPE_IPV4_IFNAME:
              {
                nhdislen += cli_out (cli, " %r", &nexthop->gate.ipv4);
                if (nexthop->ifindex)
                nhdislen += cli_out (cli, ", via %s",
                                    if_index2name (&vr->ifm, nexthop->ifindex));
                else if (nexthop->ifname)
                  nhdislen += cli_out (cli, ", via %s", nexthop->ifname);
                break;
              }
#ifdef HAVE_VRF
            case NEXTHOP_TYPE_MPLS:
              {
                IF_NSM_CAP_HAVE_VRF
                  {
                    nhdislen += cli_out (cli, " %r via MPLS", 
                                         &nexthop->gate.ipv4);
                  }
                break;
              }
#endif /* HAVE_VRF */
            case NEXTHOP_TYPE_IFINDEX:
              ifp = if_lookup_by_index (&vr->ifm, nexthop->ifindex);
              if (ifp == NULL)
                nhdislen += cli_out (cli, " is incomplete");
              else if (!(ifp->flags & IFF_POINTOPOINT))
                nhdislen += cli_out (cli, " is directly connected, %s",
                                    if_index2name (&vr->ifm, nexthop->ifindex));
              else if (if_match_ifc_ipv4_direct (ifp, &p))
                nhdislen += cli_out (cli, " is directly connected, %s",
                                    if_index2name (&vr->ifm, nexthop->ifindex));
              else
                nhdislen += cli_out (cli, " via %s",
                                    if_index2name (&vr->ifm, nexthop->ifindex));
              break;
            case NEXTHOP_TYPE_IFNAME:
              nhdislen += cli_out (cli, " directly connected, %s",
                                   nexthop->ifname);
              break;
            default:
              break;
            }
          if (! CHECK_FLAG (nexthop->flags, NEXTHOP_FLAG_ACTIVE))
            nhdislen += cli_out (cli, " inactive");

          if (CHECK_FLAG (nexthop->flags, NEXTHOP_FLAG_RECURSIVE))
            {

	      if (!nexthop->rrn || !nexthop->rrn->info)
                {
                  cli_out (cli, "\n");
                  continue;
                }

	      /* Find the selected RIB */
	      for (rrib= nexthop->rrn->info; rrib; rrib = rrib->next)
		if (CHECK_FLAG (rrib->flags, RIB_FLAG_SELECTED))
		  break;

	      /* if selected rib is not found */
              if (!rrib || !rrib->nexthop)
                continue;

              nhdislen += cli_out (cli, " (recursive");
              for (rhop = rrib->nexthop; rhop; rhop = rhop->next)
                {
                  switch (rhop->type)
                    {
                    case NEXTHOP_TYPE_IPV4:
                    case NEXTHOP_TYPE_IPV4_IFINDEX:
                    case NEXTHOP_TYPE_IPV4_IFNAME:
#ifdef HAVE_VRF
                    case NEXTHOP_TYPE_MPLS:
#endif /* HAVE_VRF */
                        {
                          if ( !CHECK_FLAG (rhop->flags, NEXTHOP_FLAG_ACTIVE) )
                            cli_out (cli, " via %r inactive", &rhop->gate.ipv4);
                          else
                            cli_out (cli, " via %r", &rhop->gate.ipv4);
                          break;
                        }
                    case NEXTHOP_TYPE_IFINDEX:
                    case NEXTHOP_TYPE_IFNAME:
                      cli_out (cli, " is directly connected, %s)",
                          if_index2name (&vr->ifm, rhop->ifindex));
                      break;
                    default:
                      break;
                    }
                  if ( rhop->next )
                    {
                      printf ("nav");
                      cli_out (cli, "\n");
                      cli_out (cli, "%*c", nhdislen, ' ');
                    }
                  if ( rhop->next == NULL)
                    cli_out (cli, ")");
                }

#if 0
              switch (nexthop->rtype)
                {
                case NEXTHOP_TYPE_IPV4:
                case NEXTHOP_TYPE_IPV4_IFINDEX:
                case NEXTHOP_TYPE_IPV4_IFNAME:
#ifdef HAVE_VRF
                case NEXTHOP_TYPE_MPLS:
#endif /* HAVE_VRF */
                  cli_out (cli, " via %r)", &nexthop->rgate.ipv4);
                  break;
                case NEXTHOP_TYPE_IFINDEX:
                case NEXTHOP_TYPE_IFNAME:
                  if (CHECK_FLAG (nexthop->flags, NEXTHOP_FLAG_RECURSIVE_BLACKHOLE))
                    cli_out (cli, " blackhole)");
                  else
                    cli_out (cli, " is directly connected, %s)",
                        if_index2name (&vr->ifm, nexthop->rifindex));
                  break;
                default:
                  break;
                }
#endif /* if 0 */
            }
          cli_out (cli, "\n");
        }
      cli_out (cli, "\n");
    }
}

CLI (show_ip_route_addr,
     show_ip_route_addr_cmd,
     "show ip route A.B.C.D",
     CLI_SHOW_STR,
     CLI_IP_STR,
     "IP routing table",
     "Network in the IP routing table to display")
{
  struct nsm_master *nm = cli->vr->proto;
  struct nsm_ptree_node *rn;
  struct nsm_vrf *vrf;
  struct prefix_ipv4 p;
  struct prefix_ipv4 addr;

  CLI_GET_IPV4_PREFIX ("address", p, argv[0]);

  /* Since there is no prefix length mentioned in the cli,
     we assume IPV4_MAX_PREFIXLEN as the most appropriate
     prefix length */
  p.prefixlen = IPV4_MAX_PREFIXLEN;

  vrf = nsm_vrf_lookup_by_id (nm, 0);
  if (! vrf)
    {
      cli_out (cli, "%% Main table does not exist\n");
      return CLI_ERROR;
    }

  rn = nsm_ptree_node_match (vrf->RIB_IPV4_UNICAST, 
                             (u_char *)&p.prefix, p.prefixlen);
  if (rn != NULL)
    {
      pal_mem_set(&addr, 0, sizeof(struct prefix_ipv4));
      addr.prefixlen = rn->key_len;
      NSM_PTREE_KEY_COPY(&addr.prefix, rn);
    }
  if (rn == NULL
      || (addr.prefix.s_addr == 0 && p.prefix.s_addr != 0))
    {
      cli_out (cli, "%% Network not in table\n");
      if (rn)
        nsm_ptree_unlock_node (rn);
      return CLI_ERROR;
    }

  nsm_show_ip_route_detail (cli, rn);
  nsm_ptree_unlock_node (rn);

  return CLI_SUCCESS;
}

CLI (show_ip_route_prefix,
     show_ip_route_prefix_cmd,
     "show ip route A.B.C.D/M",
     CLI_SHOW_STR,
     CLI_IP_STR,
     "IP routing table",
     "IP prefix <network>/<length>, e.g., 35.0.0.0/8")
{
  struct nsm_master *nm = cli->vr->proto;
  struct nsm_ptree_node *rn;
  struct nsm_vrf *vrf;
  struct prefix_ipv4 p;
  struct prefix_ipv4 addr;

  CLI_GET_IPV4_PREFIX ("address", p, argv[0]);

  vrf = nsm_vrf_lookup_by_id (nm, 0);
  if (! vrf)
    {
      cli_out (cli, "%% Main table does not exist\n");
      return CLI_ERROR;
    }

  rn = nsm_ptree_node_match (vrf->RIB_IPV4_UNICAST, (u_char *) &p.prefix, p.prefixlen);

  if (rn != NULL)
    {
      pal_mem_set(&addr, 0, sizeof(struct prefix_ipv4));
      addr.prefixlen = rn->key_len;
      NSM_PTREE_KEY_COPY(&addr.prefix, rn);
    }
 
  if (! rn || addr.prefixlen != p.prefixlen)
    {
      cli_out (cli, "%% Network not in table\n");
      if (rn)
        nsm_ptree_unlock_node (rn);
      return CLI_ERROR;
    }

  nsm_show_ip_route_detail (cli, rn);
  nsm_ptree_unlock_node (rn);

  return CLI_SUCCESS;
}

/* Only display ip forwarding is enabled or not. */
CLI (show_ip_forwarding,
     show_ip_forwarding_cmd,
     "show ip forwarding",
     CLI_SHOW_STR,
     CLI_IP_STR,
     "IP forwarding status")
{
  struct nsm_master *nm = cli->vr->proto;

  if (CHECK_FLAG (nm->flags, NSM_IPV4_FORWARDING))
    cli_out (cli, "IP forwarding is on\n");
  else
    cli_out (cli, "IP forwarding is off\n");

  return CLI_SUCCESS;
}


#ifdef HAVE_IPV6

/* Utility function to display IPv6 route entry in detail.  */
void
nsm_show_ipv6_route_detail (struct cli *cli, struct nsm_ptree_node *rn)
{
  struct apn_vr *vr = cli->vr;
  struct rib *rib;
  struct nexthop *nexthop;
  struct prefix_ipv6 p;

  for (rib = rn->info; rib; rib = rib->next)
    {
      pal_mem_set (&p, 0, sizeof (struct prefix_ipv6));
      p.prefixlen = rn->key_len;
      NSM_PTREE_KEY_COPY (&p.prefix, rn);
      cli_out (cli, "Routing entry for %R/%d\n",
               &p.prefix, p.prefixlen);
      cli_out (cli, "  Known via \"%s\"", route_type_str (rib->type));
      cli_out (cli, ", distance %d, metric %d", rib->distance, rib->metric);
      if (CHECK_FLAG (rib->flags, RIB_FLAG_SELECTED))
        cli_out (cli, ", best");
      cli_out (cli, "\n");

      nsm_show_uptime (cli, rib, "  Last update ", " ago\n");

      for (nexthop = rib->nexthop; nexthop; nexthop = nexthop->next)
        {
          cli_out (cli, "  %c",
                   CHECK_FLAG (nexthop->flags, NEXTHOP_FLAG_FIB) ? '*' : ' ');

          switch (nexthop->type)
            {
            case NEXTHOP_TYPE_IPV6:
            case NEXTHOP_TYPE_IPV6_IFINDEX:
            case NEXTHOP_TYPE_IPV6_IFNAME:
              cli_out (cli, " via %R", &nexthop->gate.ipv6);
              if (CHECK_FLAG (rib->flags, RIB_FLAG_BLACKHOLE))
                cli_out (cli, ", Null0");
              else if (nexthop->type == NEXTHOP_TYPE_IPV6_IFNAME)
                cli_out (cli, ", %s", nexthop->ifname);
              else if (nexthop->ifindex)
                cli_out (cli, ", %s",
                         if_index2name (&vr->ifm, nexthop->ifindex));
              break;
            case NEXTHOP_TYPE_IFINDEX:
              cli_out (cli, " directly connected, %s",
                       if_index2name (&vr->ifm, nexthop->ifindex));
              break;
            case NEXTHOP_TYPE_IFNAME:
              cli_out (cli, " directly connected, %s",
                       nexthop->ifname);
              break;
            default:
              break;
            }
          if (! CHECK_FLAG (nexthop->flags, NEXTHOP_FLAG_ACTIVE))
            cli_out (cli, " inactive");

          if (CHECK_FLAG (nexthop->flags, NEXTHOP_FLAG_RECURSIVE))
            {
              cli_out (cli, " (recursive");

              switch (nexthop->rtype)
                {
                case NEXTHOP_TYPE_IPV6:
                case NEXTHOP_TYPE_IPV6_IFINDEX:
                case NEXTHOP_TYPE_IPV6_IFNAME:
                  cli_out (cli, " via %R)", &nexthop->rgate.ipv6);
                  if (nexthop->rifindex)
                    cli_out (cli, ", %s",
                        if_index2name (&vr->ifm, nexthop->rifindex));
                  break;
                case NEXTHOP_TYPE_IFINDEX:
                case NEXTHOP_TYPE_IFNAME:
                  if (CHECK_FLAG (nexthop->flags, NEXTHOP_FLAG_RECURSIVE_BLACKHOLE))
                    cli_out (cli, " blackhole)");
                  else
                    cli_out (cli, " is directly connected, %s)",
                        if_index2name (&vr->ifm, nexthop->rifindex));
                  break;
                default:
                  break;
                }
            }
          cli_out (cli, "\n");
        }
      cli_out (cli, "\n");
    }
}


CLI (show_ipv6_route,
     show_ipv6_route_cmd,
     "show ipv6 route (database|)",
     CLI_SHOW_STR,
     CLI_IP_STR,
     "IPv6 routing table",
     "IPv6 routing table database")
{
  struct nsm_master *nm;
  struct nsm_vrf *vrf;

  nm = nsm_master_lookup_by_id (nzg, cli->vr->id);
  if (nm == NULL)
    return CLI_ERROR;

  vrf = nsm_vrf_lookup_by_id (nm, 0);
  if (! vrf)
    {
      cli_out (cli, "%% Main table does not exist\n");
      return CLI_ERROR;
    }

  if (argc)
    nsm_show_ipv6 (cli, vrf, 1, 0);
  else
    nsm_show_ipv6 (cli, vrf, 0, 0);

  return CLI_SUCCESS;
}

CLI (show_ipv6_route_protocol,
     show_ipv6_route_protocol_cmd,
     "show ipv6 route (database|) (bgp|connected|isis|kernel|ospf|rip|static)",
     CLI_SHOW_STR,
     CLI_IP_STR,
     "IPv6 routing table",
     "IPv6 routing table database",
     "Border Gateway Protocol (BGP)",
     "Connected",
     "ISO IS-IS",
     "Kernel",
     "Open Shortest Path First (OSPFv3)",
     "Routing Information Protocol (RIPng)",
     "Static routes")
{
  struct nsm_master *nm = cli->vr->proto;
  struct nsm_vrf *vrf;
  int type;
  char *type_str;
  int database;

  vrf = nsm_vrf_lookup_by_id (nm, 0);
  if (! vrf)
    {
      cli_out (cli, "%% Main table does not exist\n");
      return CLI_ERROR;
    }

  if (argc == 1)
    {
      type_str = argv[0];
      database = 0;
    }
  else
    {
      type_str = argv[1];
      database = 1;
    }

  if (pal_strncmp (type_str, "b", 1) == 0)
    type = APN_ROUTE_BGP;
  else if (pal_strncmp (type_str, "c", 1) == 0)
    type = APN_ROUTE_CONNECT;
  else if (pal_strncmp (type_str, "i", 1) == 0)
    type = APN_ROUTE_ISIS;
  else if (pal_strncmp (type_str, "k", 1) ==0)
    type = APN_ROUTE_KERNEL;
  else if (pal_strncmp (type_str, "o", 1) == 0)
    type = APN_ROUTE_OSPF6;
  else if (pal_strncmp (type_str, "r", 1) == 0)
    type = APN_ROUTE_RIPNG;
  else if (pal_strncmp (type_str, "s", 1) == 0)
    type = APN_ROUTE_STATIC;
  else
    {
      cli_out (cli, "Unknown route type\n");
      return CLI_ERROR;
    }

  nsm_show_ipv6 (cli, vrf, database, type);

  return CLI_SUCCESS;
}

CLI (show_ipv6_route_addr,
     show_ipv6_route_addr_cmd,
     "show ipv6 route X:X::X:X",
     CLI_SHOW_STR,
     CLI_IP_STR,
     "IPv6 routing table",
     "IPv6 Address")
{
  struct nsm_master *nm = cli->vr->proto;
  struct nsm_ptree_node *rn;
  struct nsm_vrf *vrf;
  struct prefix_ipv6 p;

  CLI_GET_IPV6_PREFIX ("IPv6 address", p, argv[0]);

  vrf = nsm_vrf_lookup_by_id (nm, 0);
  if (! vrf)
    {
      cli_out (cli, "%% Main table does not exist\n");
      return CLI_ERROR;
    }

  rn = nsm_ptree_node_match (vrf->RIB_IPV6_UNICAST, (u_char *) &p.prefix, p.prefixlen);
  if (! rn)
    {
      cli_out (cli, "%% Network not in table\n");
      return CLI_ERROR;
    }

  nsm_show_ipv6_route_detail (cli, rn);

  nsm_ptree_unlock_node (rn);

  return CLI_SUCCESS;
}

CLI (show_ipv6_route_prefix,
     show_ipv6_route_prefix_cmd,
     "show ipv6 route X:X::X:X/M",
     CLI_SHOW_STR,
     CLI_IP_STR,
     "IPv6 routing table",
     "IPv6 prefix")
{
  struct nsm_master *nm = cli->vr->proto;
  struct nsm_ptree_node *rn;
  struct nsm_vrf *vrf;
  struct prefix_ipv6 p;

  CLI_GET_IPV6_PREFIX ("IPv6 address", p, argv[0]);

  vrf = nsm_vrf_lookup_by_id (nm, 0);
  if (! vrf)
    {
      cli_out (cli, "%% Main table does not exist\n");
      return CLI_ERROR;
    }

  rn = nsm_ptree_node_match (vrf->RIB_IPV6_UNICAST, (u_char *) &p.prefix, p.prefixlen);
  
if (! rn || rn->key_len != p.prefixlen)
    {
      cli_out (cli, "%% Network not in table\n");
      if (rn)
        nsm_ptree_unlock_node(rn);
      return CLI_ERROR;
    }

  nsm_show_ipv6_route_detail (cli, rn);

  nsm_ptree_unlock_node (rn);

  return CLI_SUCCESS;
}

CLI (show_ipv6_route_summary,
     show_ipv6_route_summary_cmd,
     "show ipv6 route summary",
     CLI_SHOW_STR,
     CLI_IPV6_STR,
     "IPv6 routing table",
     "Summary of all routes")
{
  struct nsm_master *nm = cli->vr->proto;
  struct nsm_vrf *vrf;
  struct nsm_vrf_afi *nva;
  int type;
  u_int32_t total = 0;

  vrf = nsm_vrf_lookup_by_id (nm, 0);
  if (vrf == NULL)
    return CLI_ERROR;

  nva = &vrf->afi[AFI_IP6];

  cli_out (cli, "IPv6 routing table name is Default-IPv6-Routing-Table(0)\n");
  cli_out (cli, "IPv6 routing table maximum-paths is %d\n", nm->multipath_num);
  cli_out (cli, "Route Source    Networks\n");

  for (type = 0; type < APN_ROUTE_MAX; type++)
    if (nva->counter.type[type] > 0)
      {
        cli_out (cli, "%-15s %d\n",
                 route_type_str (type), nva->counter.type[type]);
        total += nva->counter.type[type];
      }

  cli_out (cli, "%-15s %d\n", "Total", total);
  cli_out (cli, "%-15s %d\n", "FIB", nva->counter.fib);

  return CLI_SUCCESS;
}

/* Only display ipv6 forwarding is enabled or not. */
CLI (show_ipv6_forwarding,
     show_ipv6_forwarding_cmd,
     "show ipv6 forwarding",
     CLI_SHOW_STR,
     CLI_IPV6_STR,
     "Forwarding status")
{
  struct nsm_master *nm = cli->vr->proto;

  if (CHECK_FLAG (nm->flags, NSM_IPV6_FORWARDING))
    cli_out (cli, "IPv6 forwarding is on\n");
  else
    cli_out (cli, "IPv6 forwarding is off\n");

  return CLI_SUCCESS;
}
#endif /* HAVE_IPV6 */
#endif /* HAVE_L3 */


/* CLI function to show NSM client information.  */
CLI (show_nsm_client,
     show_nsm_client_cmd,
     "show nsm client",
     CLI_SHOW_STR,
     "NSM",
     "client")
{
  struct nsm_server_client *nsc;
  struct nsm_server_entry *nse;
  struct nsm_redistribute *redist;
  struct nsm_restart *restart;
  int i, j;
  char buf[50];

  if (ng->server == NULL)
    return CLI_SUCCESS;

  for (i = 0; i < vector_max (ng->server->client); i++)
    if ((nsc = vector_slot (ng->server->client, i)) != NULL)
      {
        cli_out (cli, "NSM client ID: %d\n", nsc->client_id);

        for (nse = nsc->head; nse; nse = nse->next)
          {
            cli_out (cli, " %s, socket %d\n",
                     modname_strl (nse->service.protocol_id), nse->me->sock);

            if (nse->service.bits)
              {
                int first = 1;
                cli_out (cli, "  Service:");

                for (j = 0; j < 32; j++)
                  {
                    if (CHECK_FLAG (nse->service.bits, 1 << j))
                      {
                        if (first)
                          first = 0;
                        else
                          cli_out (cli, ",");
                        cli_out (cli, " %s", nsm_service_to_str (j));
                      }
                  }
                cli_out (cli, "\n");
              }
            for (redist = nse->redist; redist; redist = redist->next)
              {
                if (redist->vrf_id)
                  cli_out (cli, "  Redistribute: %d AFI (%d) VRF (%d)\n",
                           redist->type, redist->afi, redist->vrf_id);
                else
                  cli_out (cli, "  Redistribute: %d AFI (%d)\n",
                           redist->type, redist->afi);
              }

            cli_out (cli, "  Messsage received %d, sent %d\n",
                     nse->recv_msg_count, nse->send_msg_count);

            pal_time_calendar (&nse->connect_time, buf);
            buf [pal_strlen(buf) - 1] = '\0';
            cli_out (cli, "  Connection time: %s\n", buf);

            /* Debug of messages.  */
            cli_out (cli, "  Last message read: %s\n",
                     nsm_msg_to_str (nse->last_read_type));
            cli_out (cli, "  Last message write: %s\n",
                     nsm_msg_to_str (nse->last_write_type));
          }
      }

  /* Restart information.  */
  for (i = 0; i < APN_PROTO_MAX; i++)
    {
      restart = &ng->restart[i];
      if (restart->restart_time || restart->t_preserve
          || restart->restart_length)
        {
          cli_out (cli, "Restart %s: ", modname_strl (i));

          if (restart->restart_time)
            cli_out (cli, "restart time %d", restart->restart_time);
          if (restart->t_preserve)
            cli_out (cli, " preserve thread is running");
          if (restart->restart_length)
            cli_out (cli, " option value length %d", restart->restart_length);
          cli_out (cli, "\n");
        }
    }

  return CLI_SUCCESS;
}
#ifdef HAVE_L3
#ifdef HAVE_RESTART
CLI (show_nsm_fwd_timer,
     show_nsm_fwd_timer_cmd,
     "show nsm (ldp| ospf | rsvp| bgp| ) forwarding-timer",
     CLI_SHOW_STR,
     "NSM",
     "LDP",
     "OSPF",
     "RSVP",
     "BGP",
     "Forwarding timer")
{
  struct vrep_table *vrep = NULL;
  u_int8_t proto_id = APN_PROTO_UNSPEC;

  vrep = vrep_create (4, VREP_MAX_ROW_WIDTH);
  vrep_add (vrep, 0, 0, " Protocol-Name \t GR-State \t Time Remaining (sec) "
                          "\t Disconnected-time ");

  if (argv[0])
    {
      if (argv[0][0] == 'l')
        proto_id = APN_PROTO_LDP; 
      else if (argv[0][0] == 'o')
        proto_id = APN_PROTO_OSPF;
      else if (argv[0][0] == 'r')
        proto_id = APN_PROTO_RSVP;
      else if (argv[0][0] == 'b')
        proto_id = APN_PROTO_BGP;
    }

  nsm_api_show_forwarding_time (vrep, proto_id);
 
  vrep_show (vrep, nsm_vrep_show_cb, (intptr_t)cli);
  vrep_delete (vrep);

  return CLI_SUCCESS;
}
#endif /* HAVE_RESTART */
#endif /* HAVE_L3 */
 
#ifdef HAVE_L3
CLI (show_nsm_router_id,
     show_nsm_router_id_cmd,
     "show router-id",
     CLI_SHOW_STR,
     "Router ID")
{
  struct apn_vrf *iv = apn_vrf_lookup_default (cli->vr);
  struct nsm_vrf *nv = NULL;

  /* Router ID.  */
  if (!iv)
   return CLI_SUCCESS;
  nv =  iv->proto;
  nsm_show_router_id (cli, nv);

  return CLI_SUCCESS;
}
#endif /* HAVE_L3 */

/* NSM show functions.  */
void
nsm_show_init (struct lib_globals *zg)
{
  struct cli_tree *ctree = zg->ctree;

#ifdef HAVE_L3
  /* "show ip route" */
  cli_install_gen (ctree, EXEC_MODE, PRIVILEGE_NORMAL, 0,
                   &show_ip_route_cmd);
  cli_install_gen (ctree, EXEC_MODE, PRIVILEGE_NORMAL, 0,
                   &show_ip_route_protocol_cmd);
  cli_install_gen (ctree, EXEC_MODE, PRIVILEGE_NORMAL, 0,
                   &show_ip_route_summary_cmd);

#ifdef HAVE_VRF
  if (NSM_CAP_HAVE_VRF)
    {
      cli_install_gen (ctree, EXEC_MODE, PRIVILEGE_NORMAL, 0,
                       &show_ip_route_vrf_cmd);
      cli_install_gen  (ctree, EXEC_MODE, PRIVILEGE_NORMAL, 0,
                        &show_ip_route_vrf_protocol_cmd);
#ifdef HAVE_IPV6 /*6VPE*/
      cli_install_gen (ctree, EXEC_MODE, PRIVILEGE_NORMAL, 0,
                       &show_ipv6_route_vrf_cmd);
      cli_install_gen  (ctree, EXEC_MODE, PRIVILEGE_NORMAL, 0,
                        &show_ipv6_route_vrf_protocol_cmd);
#endif /*HAVE_IPV6*/	  
    }
#endif /* HAVE_VRF */

  /* "show ip route A.B.C.D" commands.  */
  cli_install_gen (ctree, EXEC_MODE, PRIVILEGE_NORMAL, 0,
                   &show_ip_route_addr_cmd);
  cli_install_gen (ctree, EXEC_MODE, PRIVILEGE_NORMAL, 0,
                   &show_ip_route_prefix_cmd);

  /* "show ip forwarding" command. */
  cli_install_gen (ctree, EXEC_MODE, PRIVILEGE_NORMAL, 0,
                   &show_ip_forwarding_cmd);

#ifdef HAVE_IPV6
  if (NSM_CAP_HAVE_IPV6)
    {
      /* "show ipv6 route" commands. */
      cli_install_gen (ctree, EXEC_MODE, PRIVILEGE_NORMAL, 0,
                       &show_ipv6_route_cmd);
      cli_install_gen (ctree, EXEC_MODE, PRIVILEGE_NORMAL, 0,
                       &show_ipv6_route_protocol_cmd);
      cli_install_gen (ctree, EXEC_MODE, PRIVILEGE_NORMAL, 0,
                       &show_ipv6_route_addr_cmd);
      cli_install_gen (ctree, EXEC_MODE, PRIVILEGE_NORMAL, 0,
                       &show_ipv6_route_prefix_cmd);
      cli_install_gen (ctree, EXEC_MODE, PRIVILEGE_NORMAL, 0,
                       &show_ipv6_route_summary_cmd);

      /* "show ipv6 forwarding" command. */
      cli_install_gen (ctree, EXEC_MODE, PRIVILEGE_NORMAL, 0,
                       &show_ipv6_forwarding_cmd);
    }
#endif /* HAVE_IPV6 */

#ifdef HAVE_VRF
  /* "show ip vrf" commands. */
  cli_install_gen (ctree, EXEC_MODE, PRIVILEGE_NORMAL, 0,
                   &show_ip_vrf_cmd);
  cli_install_gen (ctree, EXEC_MODE, PRIVILEGE_NORMAL, 0,
                   &show_ip_vrf_name_cmd);
#endif /* HAVE_VRF */

#endif /* HAVE_L3 */
  /* "show nsm client" command. */
  cli_install_gen (ctree, EXEC_MODE, PRIVILEGE_MAX, 0,
                   &show_nsm_client_cmd);

#ifdef HAVE_L3
#ifdef HAVE_RESTART
  cli_install_gen (ctree, EXEC_MODE, PRIVILEGE_MAX, 0,
                   &show_nsm_fwd_timer_cmd);
#endif /* HAVE_RESTART */
#endif /* HAVE_L3 */

#ifdef HAVE_L3
  /* "show nsm router-id" command. */
  cli_install_gen (ctree, EXEC_MODE, PRIVILEGE_MAX, 0,
                   &show_nsm_router_id_cmd);

  cli_install (ctree, EXEC_MODE, &show_nsm_router_id_cmd);
#endif /* HAVE_L3 */

#ifdef HAVE_VR
  nsm_vr_show_init (ctree);
#endif /* HAVE_VR */

}
