/* Copyright (C) 2001-2003  All Rights Reserved. */

#include "pal.h"

#include "lib.h"
#include "if.h"
#include "prefix.h"
#include "thread.h"
#include "hash.h"
#include "log.h"
#include "snprintf.h"

#include "ospfd/ospfd.h"
#include "ospfd/ospf_interface.h"
#include "ospfd/ospf_vlink.h"
#include "ospfd/ospf_packet.h"
#include "ospfd/ospf_neighbor.h"
#include "ospfd/ospf_lsa.h"
#include "ospfd/ospf_lsdb.h"
#include "ospfd/ospf_route.h"
#include "ospfd/ospf_debug.h"
#ifdef HAVE_VRF_OSPF
#include "ospfd/ospf_vrf.h"
#endif /* HAVE_VRF_OSPF */
#ifdef HAVE_RESTART
#include "ospfd/ospf_restart.h"
#endif /* HAVE_RESTART */
#ifdef HAVE_OSPF_TE
#include "ospfd/ospf_te.h"
#endif /* HAVE_OSPF_TE */


struct message ospf_lsa_type_msg[] =
{
  { OSPF_UNKNOWN_LSA,      "unknown" },
  { OSPF_ROUTER_LSA,       "router-LSA" },
  { OSPF_NETWORK_LSA,      "network-LSA" },
  { OSPF_SUMMARY_LSA,      "summary-LSA" },
  { OSPF_SUMMARY_LSA_ASBR, "ASBR-summary-LSA" },
  { OSPF_AS_EXTERNAL_LSA,  "AS-external-LSA" },
  { OSPF_GROUP_MEMBER_LSA, "GROUP_MEMBER_LSA" },
  { OSPF_AS_NSSA_LSA,      "AS-NSSA-LSA" },
  { 8,                     "Type-8 LSA" },
  { OSPF_LINK_OPAQUE_LSA,  "Link-Local Opaque-LSA" },
  { OSPF_AREA_OPAQUE_LSA,  "Area-Local Opaque-LSA" },
  { OSPF_AS_OPAQUE_LSA,    "AS-external Opaque-LSA" }
};
int ospf_lsa_type_msg_max = OSPF_MAX_LSA;

struct message ospf_link_state_id_type_msg[] =
{
  { OSPF_UNKNOWN_LSA,      "(unknown)" },
  { OSPF_ROUTER_LSA,       "" },
  { OSPF_NETWORK_LSA,      "(address of Designated Router)" },
  { OSPF_SUMMARY_LSA,      "(summary Network Number)" },
  { OSPF_SUMMARY_LSA_ASBR, "(AS Boundary Router address)" },
  { OSPF_AS_EXTERNAL_LSA,  "(External Network Number)" },
  { OSPF_GROUP_MEMBER_LSA, "(Group membership information)" },
  { OSPF_AS_NSSA_LSA,      "(External Network Number For NSSA)" },
  { 8,                     "(Type-8 LSID)" },
  { OSPF_LINK_OPAQUE_LSA,  "(Link-Local Opaque-Type/ID)" },
  { OSPF_AREA_OPAQUE_LSA,  "(Area-Local Opaque-Type/ID)" },
  { OSPF_AS_OPAQUE_LSA,    "(AS-external Opaque-Type/ID)" }
};
int ospf_link_state_id_type_msg_max = OSPF_MAX_LSA;

struct message ospf_link_type_msg[] =
{
  { 0,                         "(unknown)" },
  { LSA_LINK_TYPE_POINTOPOINT, "Point-to-Point" },
  { LSA_LINK_TYPE_TRANSIT,     "Transit Network" },
  { LSA_LINK_TYPE_STUB,        "Stub Network" },
  { LSA_LINK_TYPE_VIRTUALLINK, "Virtual-Link" },
};
int ospf_link_type_msg_max = 5;


char *
ospf_area_name_string (struct ospf_area *area, char *buf)
{
  if (!area)
    return "-";

  zsnprintf (buf, OSPF_AREA_STRING_MAXLEN, "%r", &area->area_id);

  return buf;
}

char *
ospf_area_fmt_string (struct pal_in4_addr area_id,
                      u_char format, char *buf)
{
  u_int32_t area_val;

  area_val = pal_ntoh32 (area_id.s_addr);

  if (format == OSPF_AREA_ID_FORMAT_DECIMAL)
    zsnprintf (buf, OSPF_AREA_STRING_MAXLEN, "%lu", area_val);
  else
    zsnprintf (buf, OSPF_AREA_STRING_MAXLEN, "%r", &area_id);

  return buf;
}

#ifdef HAVE_VRF_OSPF
char *
ospf_domain_id_fmt_string (u_char *domainid, char *buf)
{
  struct ospf_vrf_domain_id pdomain_id;
  u_char domain_id[16];
   
  pal_mem_cpy (&pdomain_id, domainid, sizeof (struct ospf_vrf_domain_id));
  pdomain_id.type = pal_ntoh16 (pdomain_id.type);
  if (pdomain_id.type == TYPE_IP_DID)
    {
      pal_inet_ntop (AF_INET, &pdomain_id.value, domain_id, 32); 
      zsnprintf (buf, OSPF_AREA_STRING_MAXLEN, "%s", &domain_id);
    }
  else
    zsnprintf (buf, OSPF_AREA_STRING_MAXLEN, "%x%x%x%x%x%x",
                 pdomain_id.value[0], pdomain_id.value[1],
                 pdomain_id.value[2], pdomain_id.value[3],
                 pdomain_id.value[4], pdomain_id.value[5]);

  return buf;
}
#endif /*HAVE_VRF_OSPF*/

char *
ospf_area_desc_string (struct ospf_area *area, char *buf)
{
  if (!area)
    return "(incomplete)";

#ifdef HAVE_NSSA
  else if (IS_AREA_NSSA (area))
    zsnprintf (buf, OSPF_AREA_DESC_STRING_MAXLEN, "%r [NSSA]", &area->area_id);
#endif /* HAVE_NSSA */

  else if (IS_AREA_STUB (area))
    zsnprintf (buf, OSPF_AREA_DESC_STRING_MAXLEN, "%r [Stub]", &area->area_id);

  else
    zsnprintf (buf, OSPF_AREA_DESC_STRING_MAXLEN, "%r", &area->area_id);

  return buf;
}

char *
ospf_if_name_string (struct ospf_interface *oi, char *buf)
{
  if (!oi)
    return "inactive";

  if (oi->type == OSPF_IFTYPE_VIRTUALLINK)
    return oi->u.vlink->name;

  zsnprintf (buf, OSPF_IF_STRING_MAXLEN, "%s:%r",
             oi->u.ifp->name, &oi->ident.address->u.prefix4);

  return buf;
}

char *
ospf_nbr_name_string (struct ospf_neighbor *nbr, char *buf)
{
  char if_str[OSPF_IF_STRING_MAXLEN];

  if (!nbr)
    return "inactive";

  zsnprintf (buf, OSPF_NBR_STRING_MAXLEN,
             "%s-%r", IF_NAME (nbr->oi), &nbr->ident.router_id);

  return buf;
}

char *
ospf_lsa_name_string (struct ospf_lsa *lsa, char *buf)
{
  char area_str[OSPF_AREA_STRING_MAXLEN];
  char *area_name;

  if (!lsa)
    return "inactive";

  if (lsa->lsdb->scope == LSA_FLOOD_SCOPE_AREA)
    area_name = ospf_area_name_string (lsa->lsdb->u.area, area_str);
  else
    area_name = "-";

  if (IS_LSA_SELF (lsa))
    zsnprintf (buf, OSPF_LSA_STRING_MAXLEN, "%s:Type%d:%r:(self)",
               area_name, lsa->data->type, &lsa->data->id);
  else
    zsnprintf (buf, OSPF_LSA_STRING_MAXLEN, "%s:Type%d:%r:%r",
               area_name, lsa->data->type, &lsa->data->id,
               &lsa->data->adv_router);

  return buf;
}

char *
ospf_vlink_name_string (struct ospf_vlink *vlink, char *buf)
{
  if (!vlink)
    return "inactive";

  zsnprintf (buf, OSPF_VLINK_STRING_MAXLEN, "%s%d",
             OSPF_VLINK_NAME_PREFIX, vlink->index);
  return buf;
}

char *
ospf_nbr_state_message (struct ospf_neighbor *nbr, char *buf)
{
  struct ospf_interface *oi = nbr->oi;
  int state;

  if (oi->type != OSPF_IFTYPE_POINTOPOINT
      && oi->type != OSPF_IFTYPE_VIRTUALLINK
      && oi->type != OSPF_IFTYPE_POINTOMULTIPOINT
      && oi->type != OSPF_IFTYPE_POINTOMULTIPOINT_NBMA)
    {
      if (IPV4_ADDR_SAME (&oi->ident.d_router, &nbr->ident.address->u.prefix4))
        state = IFSM_DR;
      else if (IPV4_ADDR_SAME (&oi->ident.bd_router,
                               &nbr->ident.address->u.prefix4))
        state = IFSM_Backup;
      else
        state = IFSM_DROther;

      zsnprintf (buf, OSPF_NBR_STATE_STR_MAXLEN, "%s/%s",
                 LOOKUP (ospf_nfsm_state_msg, nbr->state),
                 LOOKUP (ospf_ifsm_state_msg, state));
    }
  else
    zsnprintf (buf, OSPF_NBR_STATE_STR_MAXLEN, "%s/ -",
               LOOKUP (ospf_nfsm_state_msg, nbr->state));

  return buf;
}

char *
ospf_option_dump (u_char options, char *buf)
{
  zsnprintf (buf, OSPF_OPTION_STR_MAXLEN, "%s|%s|%s|%s|%s|%s|%s|%s",
             (options & OSPF_OPTION_DN) ? "DN" : "-",
             (options & OSPF_OPTION_O) ? "O" : "-",
             (options & OSPF_OPTION_DC) ? "DC" : "-",
#ifdef HAVE_RESTART
             (options & OSPF_OPTION_L) ? "L" : "-",
#else /* HAVE_RESTART */
             (options & OSPF_OPTION_EA) ? "EA" : "-",
#endif /* HAVE_RESTART */
             (options & OSPF_OPTION_NP) ? "N/P" : "-",
             (options & OSPF_OPTION_MC) ? "MC" : "-",
             (options & OSPF_OPTION_E) ? "E" : "-",
             (options & OSPF_OPTION_T) ? "T" : "-");

  return buf;
}

#ifdef HAVE_RESTART
char *
ospf_extended_option_dump (u_int32_t options, char *buf)
{
  zsnprintf (buf, OSPF_EXTENDED_OPTION_STR_MAXLEN, "%s|%s",
             (options & OSPF_EXTENDED_OPTION_RS) ? "RS" : "-",
             (options & OSPF_EXTENDED_OPTION_LR) ? "LR" : "-");
  return buf;
}
#endif /* HAVE_RESTART */

void
ospf_packet_hello_dump (u_char *p, u_int16_t length)
{
  struct pal_in4_addr *netmask = (struct pal_in4_addr *)p;
  u_int16_t hello_interval = OSPF_PNT_UINT16_GET (p, 4);
  u_char options = OSPF_PNT_UCHAR_GET (p, 6);
  u_char priority = OSPF_PNT_UCHAR_GET (p, 7);
  u_int32_t dead_interval = OSPF_PNT_UINT32_GET (p, 8);
  struct pal_in4_addr *d_router = (struct pal_in4_addr *)(p + 12);
  struct pal_in4_addr *bd_router = (struct pal_in4_addr *)(p + 16);
  char option_str[OSPF_OPTION_STR_MAXLEN];
  u_char *lim = p + length;

  zlog_info (ZG, "Hello");
  zlog_info (ZG, "  NetworkMask %r", netmask);
  zlog_info (ZG, "  HelloInterval %d", hello_interval);
  zlog_info (ZG, "  Options 0x%x (%s)", options,
             ospf_option_dump (options, option_str));
  zlog_info (ZG, "  RtrPriority %d", priority);
  zlog_info (ZG, "  RtrDeadInterval %d", dead_interval);
  zlog_info (ZG, "  DRouter %r", d_router);
  zlog_info (ZG, "  BDRouter %r", bd_router);

  p += OSPF_HELLO_MIN_SIZE;
  length -= OSPF_HELLO_MIN_SIZE;

  zlog_info (ZG, "  # Neighbors %d", length / 4);

  while (p < lim)
    {
      zlog_info (ZG, "    Neighbor %r", p);
      p += 4;
    }
}

char *
ospf_dd_bits_dump (u_char bits, char *buf)
{
  zsnprintf (buf, OSPF_DD_BITS_STR_MAXLEN, "%s|%s|%s|%s",
             (bits & OSPF_DD_FLAG_R) ? "R" : "-",
             (bits & OSPF_DD_FLAG_I) ? "I" : "-",
             (bits & OSPF_DD_FLAG_M) ? "M" : "-",
             (bits & OSPF_DD_FLAG_MS) ? "MS" : "-");

  return buf;
}

void
ospf_lsa_header_dump (struct lsa_header *lsah)
{
  zlog_info (ZG, "  LSA Header");

  zlog_info (ZG, "    LS age %d", pal_ntoh16 (lsah->ls_age));
  zlog_info (ZG, "    Options 0x%x", lsah->options);
  zlog_info (ZG, "    LS type %d (%s)", lsah->type,
             LOOKUP (ospf_lsa_type_msg, lsah->type));
  zlog_info (ZG, "    Link State ID %r", &lsah->id);
  zlog_info (ZG, "    Advertising Router %r", &lsah->adv_router);
  zlog_info (ZG, "    LS sequence number 0x%x", pal_ntoh32 (lsah->ls_seqnum));
  zlog_info (ZG, "    LS checksum 0x%x", pal_ntoh16 (lsah->checksum));
  zlog_info (ZG, "    length %d", pal_ntoh16 (lsah->length));
}

char *
ospf_router_lsa_flags_dump (u_char flags, char *buf, size_t size)
{
  pal_mem_set (buf, 0, size);

  pal_snprintf (buf, size, "%s|%s|%s",
                (flags & ROUTER_LSA_BIT_V) ? "V" : "-",
                (flags & ROUTER_LSA_BIT_E) ? "E" : "-",
                (flags & ROUTER_LSA_BIT_B) ? "B" : "-");

  return buf;
}

void
ospf_router_lsa_dump (u_char *p, u_int16_t length)
{
  char buf[BUFSIZ];
  u_char flags;
  u_int16_t links;
  u_char *lim = p + length;
  u_char type;
  u_char tos_num;
  u_char tos;
  u_int16_t metric;

  p += OSPF_LSA_HEADER_SIZE;

  flags = OSPF_PNT_UCHAR_GET (p, 0);
  links = OSPF_PNT_UINT16_GET (p, 2);

  zlog_info (ZG, "  Router-LSA");
  zlog_info (ZG, "    flags %s",
             ospf_router_lsa_flags_dump (flags, buf, BUFSIZ));
  zlog_info (ZG, "    # links %d", links);

  p += 4;
  while (p < lim)
    {
      if (p + OSPF_LINK_DESC_SIZE > lim)
        {
          zlog_info (ZG, "    *LSA corrupted*");
          break;
        }

      zlog_info (ZG, "    Link ID %r", p);
      zlog_info (ZG, "    Link Data %r", p + 4);

      type = OSPF_PNT_UCHAR_GET (p, 8);
      tos_num = OSPF_PNT_UCHAR_GET (p, 9);
      metric = OSPF_PNT_UINT16_GET (p, 10);

      zlog_info (ZG, "    Type %d, #TOS %d, metric %d", type, tos_num, metric);

      p += OSPF_LINK_DESC_SIZE;
      if (tos_num > 0)
        {
          if (p + OSPF_LINK_TOS_SIZE * tos_num > lim)
            {
              zlog_info (ZG, "    *LSA corrupted*");
              break;
            }

          while (tos_num > 0)
            {
              tos = OSPF_PNT_UCHAR_GET (p, 0);
              metric = OSPF_PNT_UCHAR_GET (p, 2);

              zlog_info (ZG, "    TOS %d, metric %d", tos, metric);
              p += OSPF_LINK_TOS_SIZE;
              tos_num--;
            }
        }
    }
}

void
ospf_network_lsa_dump (u_char *p, u_int16_t length)
{
  int count;
  u_char *lim = p + length;

  p += OSPF_LSA_HEADER_SIZE;
  length -= OSPF_LSA_HEADER_SIZE;

  count = (length - 4) / 4;

  zlog_info (ZG, "  Network-LSA");
  zlog_info (ZG, "    Network Mask %r", p);
  zlog_info (ZG, "    # Attached Routers %d", count);

  p += 4;
  length -= 4;

  while (p < lim)
    {
      zlog_info (ZG, "      Attached Router %r", p);
      p += 4;
    }
}

void
ospf_summary_lsa_dump (u_char *p, u_int16_t length)
{
  u_char tos;
  u_int32_t metric;
  u_char *lim = p + length;

  p += OSPF_LSA_HEADER_SIZE;

  zlog_info (ZG, "  Summary-LSA");
  zlog_info (ZG, "    Network Mask %r", p);

  p += 4;

  while (p < lim)
    {
      tos = OSPF_PNT_UCHAR_GET (p, 0);
      metric = OSPF_PNT_UINT24_GET (p, 1);
      zlog_info (ZG, "    TOS=%d metric %d", tos, metric);
      p += 4;
    }
}

void
ospf_as_external_lsa_dump (u_char *p, u_int16_t length)
{
  u_char tos;
  u_int32_t metric;
  struct pal_in4_addr *nexthop;
  u_int32_t tag;
  u_char *lim = p + length;

  p += OSPF_LSA_HEADER_SIZE;

  zlog_info (ZG, "  AS-external-LSA");
  zlog_info (ZG, "    Network Mask %r", p);

  p += 4;

  while (p < lim)
    {
      tos = OSPF_PNT_UCHAR_GET (p, 0);
      metric = OSPF_PNT_UINT24_GET (p, 1);
      nexthop = (struct pal_in4_addr *)(p + 4);
      tag = OSPF_PNT_UINT32_GET (p, 8);

      zlog_info (ZG, "    bit %s TOS=%d metric %d",
                 IS_EXTERNAL_METRIC (tos) ? "E" : "-", (tos & 0x7f), metric);
      zlog_info (ZG, "    Forwarding address %r", nexthop);
      zlog_info (ZG, "    External Route Tag %d", tag);

      p += 12;
    }
}

#ifdef HAVE_OPAQUE_LSA
#ifdef HAVE_RESTART
void
ospf_grace_lsa_grace_period_dump (u_char *p, u_int16_t length)
{
  u_int32_t seconds;

  if (length == OSPF_GRACE_LSA_GRACE_PERIOD_LEN)
    {
      seconds = OSPF_PNT_UINT32_GET (p, 4);
      zlog_info (ZG, "    Grace Period: %d", seconds);
    }
}

void 
ospf_grace_lsa_restart_reason_dump (u_char *p, u_int16_t length)
{
  u_char reason;

  if (length == OSPF_GRACE_LSA_RESTART_REASON_LEN)
    {
      reason = OSPF_PNT_UCHAR_GET (p, 4);

      switch (reason)
        {
        case OSPF_RESTART_REASON_RESTART:
          zlog_info (ZG, "    Graceful Restart Reason: Software Restart");
          break;
        case OSPF_RESTART_REASON_UPGRADE:
          zlog_info (ZG, "    Graceful Restart Reason: "
                     "Software Reload/Upgrade");
          break;
        case OSPF_RESTART_REASON_SWITCH_REDUNDANT:
          zlog_info (ZG, "    Graceful Restart Reason: "
                     "Switch to redundant control processor");
          break;
        case OSPF_RESTART_REASON_UNKNOWN:
        default:
          zlog_info (ZG, "    Graceful Restart Reason: Unknown");
          break;
        }
    }
}

void
ospf_grace_lsa_ip_interface_address_dump (u_char *p, u_int16_t length)
{
  struct pal_in4_addr *addr;

  if (length == OSPF_GRACE_LSA_IP_INTERFACE_ADDRESS_LEN)
    {
      addr = OSPF_PNT_IN_ADDR_GET (p, 4);
      zlog_info (ZG, "    IP interface Address: %r", addr);
    }
}

void
ospf_grace_lsa_dump (u_char *p, u_int16_t length)
{
  u_char *lim = p + length;
  u_int16_t type;

  zlog_info (ZG, "  Grace-LSA");

  while (p + 4 < lim)
    {
      type = OSPF_PNT_UINT16_GET (p, 0);
      length = OSPF_PNT_UINT16_GET (p, 2);

      if (length > 0 && p + OSPF_TLV_SPACE (length) <= lim)
        switch (type)
          {
          case OSPF_GRACE_LSA_GRACE_PERIOD:
            ospf_grace_lsa_grace_period_dump (p, length);
            break;
          case OSPF_GRACE_LSA_RESTART_REASON:
            ospf_grace_lsa_restart_reason_dump (p, length);
            break;
          case OSPF_GRACE_LSA_IP_INTERFACE_ADDRESS:
            ospf_grace_lsa_ip_interface_address_dump (p, length);
            break;
          default:
            zlog_info (ZG, "    Unknown TLV type (%hu)", type);
          }
      p += OSPF_TLV_SPACE (length);
    }
}
#endif /* HAVE_RESTART */

#ifdef HAVE_OSPF_TE
void
ospf_te_router_tlv_dump (u_char *p, u_int16_t length)
{
  struct pal_in4_addr *addr;

  if (length == OSPF_TE_TLV_RTADDR_FIX_LEN)
    {
      addr = OSPF_PNT_IN_ADDR_GET (p, 4);

      zlog_info (ZG, "    Router Address TLV");
      zlog_info (ZG, "      Router Address: %r", addr);
    }
}

void
ospf_te_link_stlv_link_type_dump (u_char *p, u_int16_t length)
{
  u_char type;

  if (length == OSPF_TE_TLV_LINK_TYPE_FIX_LEN)
    {
      type = OSPF_PNT_UCHAR_GET (p, 4);

      switch (type)
        {
        case TE_LINK_TYPE_PTP:
          zlog_info (ZG, "      Link Type: Point-to-Point network");
          break;
        case TE_LINK_TYPE_MULTIACCESS:
          zlog_info (ZG, "      Link Type: Multi-access network");
          break;
        default:
          zlog_info (ZG, "      Link Type: Unknown network");
          break;
        }
    }
}

void
ospf_te_link_stlv_link_id_dump (u_char *p, u_int16_t length)
{
  struct pal_in4_addr *addr;

  if (length == OSPF_TE_TLV_LINK_ID_FIX_LEN)
    {
      addr = OSPF_PNT_IN_ADDR_GET (p, 4);
      zlog_info (ZG, "      Link ID: %r", addr);
    }
}

void
ospf_te_link_stlv_local_dump (u_char *p, u_int16_t length)
{
  u_char *lim = p + OSPF_TLV_SPACE (length);
  struct pal_in4_addr *addr;

  for (p += 4; p + 4 <= lim; p += 4)
    {
      addr = OSPF_PNT_IN_ADDR_GET (p, 0);
      zlog_info (ZG, "      Local Interface IP Address: %r", addr);
    }
}

void
ospf_te_link_stlv_remote_dump (u_char *p, u_int16_t length)
{
  u_char *lim = p + OSPF_TLV_SPACE (length);
  struct pal_in4_addr *addr;

  for (p += 4; p + 4 <= lim; p += 4)
    {
      addr = OSPF_PNT_IN_ADDR_GET (p, 0);
      zlog_info (ZG, "      Remote Interface IP Address: %r", addr);
    }
}

void
ospf_te_link_stlv_metric_dump (u_char *p, u_int16_t length)
{
  u_int32_t metric;

  if (length == OSPF_TE_TLV_METRIC_FIX_LEN)
    {
      metric = OSPF_PNT_UINT32_GET (p, 4);
      zlog_info (ZG, "      Traffic Engineering Metric: %lu", metric);
    }
}

void
ospf_te_link_stlv_max_bw_dump (u_char *p, u_int16_t length)
{
  union
  {
    u_int32_t i;
    float32_t f;
  } bw;

  if (length == OSPF_TE_TLV_BW_FIX_LEN)
    {
      bw.i = OSPF_PNT_UINT32_GET (p, 4);
      zlog_info (ZG, "      Maximum Bandwidth: %.2f Kbits/s", bw.f * 8 / 1000);
    }
}

void
ospf_te_link_stlv_res_bw_dump (u_char *p, u_int16_t length)
{
  union
  {
    u_int32_t i;
    float32_t f;
  } bw;

  if (length == OSPF_TE_TLV_BW_FIX_LEN)
    {
      bw.i = OSPF_PNT_UINT32_GET (p, 4);
      zlog_info (ZG, "      Maximum Reservable Bandwidth: %.2f Kbits/s",
                 bw.f * 8 / 1000);
    }
}

void
ospf_te_link_stlv_unres_bw_dump (u_char *p, u_int16_t length)
{
  union
  {
    u_int32_t i;
    float32_t f;
  } bw;
  int i;

  if (length == OSPF_TE_TLV_UNRSV_BW_FIX_LEN)
    {
      zlog_info (ZG, "      Unreservable Bandwidth:");

      for (i = 0; i < MAX_PRIORITIES; i++)
        {
          bw.i = OSPF_PNT_UINT32_GET (p + 4, 4 * i);
          zlog_info (ZG, "        Priority %d: %.2f Kbits/s",
                     i, bw.f * 8 / 1000);
        }
    }
}

void
ospf_te_link_stlv_admin_group_dump (u_char *p, u_int16_t length)
{
  u_int32_t admin;

  if (length == OSPF_TE_TLV_RSRC_COLOR_FIX_LEN)
    {
      admin = OSPF_PNT_UINT32_GET (p, 4);
      zlog_info (ZG, "      Administrative Group: 0x%x", admin);
    }
}

#ifdef HAVE_GMPLS
void
ospf_te_link_stlv_local_remote_id_dump (u_char *p, u_int16_t length)
{
  u_int32_t local, remote;

  if (length == OSPF_TE_TLV_LOCAL_REMOTE_ID_FIX_LEN)
    {
      local = OSPF_PNT_UINT32_GET (p, 4);
      remote = OSPF_PNT_UINT32_GET (p, 8);
      zlog_info (ZG, "      Link Local Identifier:  %d", local);
      zlog_info (ZG, "      Link Remote Identifier: %d", remote);
    }
}

void
ospf_te_link_stlv_protection_type_dump (u_char *p, u_int16_t length)
{
  u_char type;

  if (length == OSPF_TE_TLV_PROTECTION_TYPE_FIX_LEN)
    {
      type = OSPF_PNT_UCHAR_GET (p, 4);
      if (CHECK_FLAG (type, GMPLS_PROTECTION_TYPE_EXTRA_TRAFFIC))
        zlog_info (ZG, "      Link Protection Type: Extra Traffic");
      if (CHECK_FLAG (type, GMPLS_PROTECTION_TYPE_UNPROTECTED))
        zlog_info (ZG, "      Link Protection Type: Unprotected");
      if (CHECK_FLAG (type, GMPLS_PROTECTION_TYPE_SHARED))
        zlog_info (ZG, "      Link Protection Type: Shared");
      if (CHECK_FLAG (type, GMPLS_PROTECTION_TYPE_DEDICATED_1TO1))
        zlog_info (ZG, "      Link Protection Type: Dedicated 1:1");
      if (CHECK_FLAG (type, GMPLS_PROTECTION_TYPE_DEDICATED_1PLUS1))
        zlog_info (ZG, "      Link Protection Type: Dedicated 1+1");
      if (CHECK_FLAG (type, GMPLS_PROTECTION_TYPE_ENHANCED))
        zlog_info (ZG, "      Link Protection Type: Enhanced");
    }
}

void
ospf_te_link_stlv_capability_dump (u_char *p, u_int16_t length)
{
  u_char capability;
  u_char encoding;
#ifdef HAVE_TDM
  u_char indication;
#endif /* HAVE_TDM */
  u_int16_t mtu;
  union
  {
    u_int32_t i;
    float32_t f;
  } bw;
  int i;

  if (length < OSPF_TE_TLV_IF_SW_CAPABILITY_MIN_LEN)
    return;

  capability = OSPF_PNT_UCHAR_GET (p, 4);

  zlog_info (ZG, "      Link Switching Capability Discriptor: ");

  zlog_info (ZG, "        Capability Type: %s",
             gmpls_capability_type_str (capability));

  encoding = OSPF_PNT_UCHAR_GET (p, 5);
  zlog_info (ZG, "        Encoding Type: %s",
             gmpls_encoding_type_str (encoding));

  for (i = 0; i < MAX_PRIORITIES; i++)
    {
      bw.i = OSPF_PNT_UINT32_GET (p + 8, 4 * i);
      zlog_info (ZG, "        Max LSP Bandwidth at priority %d: %.2f Kbits/s", 
                 i, bw.f * 8 / 1000);
    }

  /* Switching Capability-specific information.  */
  switch (capability)
    {
    case GMPLS_SWITCH_CAPABILITY_FLAG_PSC1:
    case GMPLS_SWITCH_CAPABILITY_FLAG_PSC2:
    case GMPLS_SWITCH_CAPABILITY_FLAG_PSC3:
    case GMPLS_SWITCH_CAPABILITY_FLAG_PSC4:
      if (length < OSPF_TE_TLV_IF_SW_CAPABILITY_PSC_LEN)
        return;
      bw.i = OSPF_PNT_UINT32_GET (p + 8, 4 * MAX_PRIORITIES);
      mtu = OSPF_PNT_UINT16_GET (p + 12, 4 * MAX_PRIORITIES);
      zlog_info (ZG, "        Minimum LSP Bandwidth : %.02f Kbits/s", 
                 bw.f * 8 / 1000);
      zlog_info (ZG, "        Interface MTU: %d", mtu);
      break;

#ifdef HAVE_TDM
    case GMPLS_CAPABILITY_FLAG_TDM:
      if (length < OSPF_TE_TLV_IF_SW_CAPABILITY_TDM_LEN)
        return;
      bw.i = OSPF_PNT_UINT32_GET (p + 8, 4 * MAX_PRIORITIES);
      indication = OSPF_PNT_UCHAR_GET (p + 12, 4 * MAX_PRIORITIES);
      zlog_info (ZG, "      Minimum LSP Bandwidth: %.02f Kbits/s", 
                 bw.f * 8 / 1000);
      zlog_info (ZG, "      SONET/SDH Indication: %s",
                 gmpls_sdh_indication_str (indication));
      break;
#endif /* HAVE_TDM */
    default:
      break;
    }
}

void
ospf_te_link_stlv_shared_risk_group_dump (u_char *p, u_int16_t length)
{
  u_char *lim = p + OSPF_TLV_SPACE (length);
  u_int32_t group;

  zlog_info (ZG, "      Shared Risk Link Group: ");
  for (p += 4; p + 4 <= lim; p += 4)
    {
      group = OSPF_PNT_UINT32_GET (p, 0);
      zlog_info (ZG, "        %lu", group);
    }
}
#endif /* HAVE_GMPLS */

int
ospf_te_link_tlv_dump (u_char *p, u_int16_t length)
{
  u_char *lim = p + OSPF_TLV_SPACE (length);
  u_int16_t type;

  zlog_info (ZG, "    Link TLV");

  for (p += 4; p + 4 < lim; p += OSPF_TLV_SPACE (length))
    {
      type = OSPF_PNT_UINT16_GET (p, 0);
      length = OSPF_PNT_UINT16_GET (p, 2);

      if (length > 0 && p + OSPF_TLV_SPACE (length) <= lim)
        switch (type)
          {
          case TE_LINK_SUBTLV_LINK_TYPE:
            ospf_te_link_stlv_link_type_dump (p, length);
            break;
          case TE_LINK_SUBTLV_LINK_ID:
            ospf_te_link_stlv_link_id_dump (p, length);
            break;
          case TE_LINK_SUBTLV_LOCAL_ADDRESS:
            ospf_te_link_stlv_local_dump (p, length);
            break;
          case TE_LINK_SUBTLV_REMOTE_ADDRESS:
            ospf_te_link_stlv_remote_dump (p, length);
            break;
          case TE_LINK_SUBTLV_TE_METRIC:
            ospf_te_link_stlv_metric_dump (p, length);
            break;
          case TE_LINK_SUBTLV_MAX_BANDWIDTH:
            ospf_te_link_stlv_max_bw_dump (p, length);
            break;
          case TE_LINK_SUBTLV_MAX_RES_BANDWIDTH:
            ospf_te_link_stlv_res_bw_dump (p, length);
            break;
          case TE_LINK_SUBTLV_UNRES_BANDWIDTH:
            ospf_te_link_stlv_unres_bw_dump (p, length);
            break;
          case TE_LINK_SUBTLV_RESOURCE_CLASS:
            ospf_te_link_stlv_admin_group_dump (p, length);
            break;
#ifdef HAVE_GMPLS
          case TE_LINK_SUBTLV_LOCAL_REMOTE_ID:
            ospf_te_link_stlv_local_remote_id_dump (p, length);
            break;
          case TE_LINK_SUBTLV_PROTECTION_TYPE:
            ospf_te_link_stlv_protection_type_dump (p, length);
            break;
          case TE_LINK_SUBTLV_IF_SW_CAPABILITY:
            ospf_te_link_stlv_capability_dump (p, length);
            break;
          case TE_LINK_SUBTLV_SHARED_RISK_GROUP:
            ospf_te_link_stlv_shared_risk_group_dump (p, length);
            break;
#endif /* HAVE_GMPLS */
          default:
            zlog_info (ZG, "      Unknown sub-TLV type (%hu)", type);
            break;
          }
    }
  return 1;
}

void
ospf_te_lsa_dump (u_char *p, u_int16_t length)
{
  u_char *lim = p + length;
  u_int16_t type;

  zlog_info (ZG, "  Traffic Engineering LSA");

  while (p + 4 < lim)
    {
      type = OSPF_PNT_UINT16_GET (p, 0);
      length = OSPF_PNT_UINT16_GET (p, 2);

      if (length > 0 && p + OSPF_TLV_SPACE (length) <= lim)
        switch (type)
          {
          case TE_TLV_ROUTER_TYPE:
            ospf_te_router_tlv_dump (p, length);
            break;
          case TE_TLV_LINK_TYPE:
            ospf_te_link_tlv_dump (p, length);
            break;
          }
      p += OSPF_TLV_SPACE (length);
    }
}
#endif /* HAVE_OSPF_TE */

void 
ospf_opaque_lsa_dump (u_char *p, u_int16_t length)
{
  struct lsa_header *lsa = (struct lsa_header *)p;

  p += OSPF_LSA_HEADER_SIZE;
  length -= OSPF_LSA_HEADER_SIZE;

  switch (OPAQUE_TYPE (lsa->id))
    {
#ifdef HAVE_RESTART
    case OSPF_LSA_OPAQUE_TYPE_GRACE:
      ospf_grace_lsa_dump (p, length);
      break;
#endif /* HAVE_RESTART */
#ifdef HAVE_OSPF_TE
    case OSPF_LSA_OPAQUE_TYPE_TE:
      ospf_te_lsa_dump (p, length);
      break;
#endif /* HAVE_OSPF_TE */
    default:
      zlog_info (ZG, "  Opaque-LSA (unknown opaque type (%u))",
                 OPAQUE_TYPE (lsa->id));
      break;
    }
}
#endif /* HAVE_OPAQUE_LSA */

void
ospf_lsa_header_list_dump (u_char *p, u_int16_t length)
{
  struct lsa_header *lsa;

  zlog_info (ZG, "  # LSA Headers %d", length / OSPF_LSA_HEADER_SIZE);

  /* LSA Headers. */
  while (length > 0)
    {
      lsa = (struct lsa_header *) p;
      ospf_lsa_header_dump (lsa);

      p += OSPF_LSA_HEADER_SIZE;
      length -= OSPF_LSA_HEADER_SIZE;
    }
}

void
ospf_packet_db_desc_dump (u_char *p, u_int16_t length)
{
  u_int16_t mtu = OSPF_PNT_UINT16_GET (p, 0);
  u_char options = OSPF_PNT_UCHAR_GET (p, 2);
  u_char bits = OSPF_PNT_UCHAR_GET (p, 3);
  u_int32_t seqnum = OSPF_PNT_UINT32_GET (p, 4);
  char dd_bits_str[OSPF_DD_BITS_STR_MAXLEN];
  char option_str[OSPF_OPTION_STR_MAXLEN];

  zlog_info (ZG, "Database Description");
  zlog_info (ZG, "  Interface MTU %d", mtu);
  zlog_info (ZG, "  Options 0x%x (%s)", options,
             ospf_option_dump (options, option_str));
  zlog_info (ZG, "  Bits %d (%s)", bits,
             ospf_dd_bits_dump (bits, dd_bits_str));
  zlog_info (ZG, "  Sequence Number 0x%08x", seqnum);

  length -= OSPF_DB_DESC_MIN_SIZE;

  ospf_lsa_header_list_dump (p + OSPF_DB_DESC_MIN_SIZE, length);
}

void
ospf_packet_ls_req_dump (u_char *p, u_int16_t length)
{
  u_int32_t ls_type;
  u_char *lim = p + length;

  zlog_info (ZG, "Link State Request");
  zlog_info (ZG, "  # Requests %d", length / OSPF_LS_REQ_KEY_SIZE);

  while (p < lim)
    {
      ls_type = OSPF_PNT_UINT32_GET (p, 0);

      zlog_info (ZG, "  LS type %d", ls_type);
      zlog_info (ZG, "  Link State ID %r", p + 4);
      zlog_info (ZG, "  Advertising Router %r", p + 8);

      p += OSPF_LS_REQ_KEY_SIZE;
    }
}

void
ospf_packet_ls_upd_dump (u_char *p, u_int16_t length)
{
  struct lsa_header *lsa;
  u_int32_t count = OSPF_PNT_UINT32_GET (p, 0);
  int lsa_len;
  u_char *lim = p + length;

  length -= OSPF_LS_UPD_MIN_SIZE;
  p += OSPF_LS_UPD_MIN_SIZE;

  zlog_info (ZG, "Link State Update");
  zlog_info (ZG, "  # LSAs %d", count);

  while (p < lim && count > 0)
    {
      lsa = (struct lsa_header *)p;
      lsa_len = pal_ntoh16 (lsa->length);
      ospf_lsa_header_dump (lsa);

      if (p + lsa_len > lim)
        {
          zlog_info (ZG, "  *LS-Update correupted");
          break;
        }

      switch (lsa->type)
        {
        case OSPF_ROUTER_LSA:
          ospf_router_lsa_dump (p, lsa_len);
          break;
        case OSPF_NETWORK_LSA:
          ospf_network_lsa_dump (p, lsa_len);
          break;
        case OSPF_SUMMARY_LSA:
        case OSPF_SUMMARY_LSA_ASBR:
          ospf_summary_lsa_dump (p, lsa_len);
          break;
        case OSPF_AS_EXTERNAL_LSA:
#ifdef HAVE_NSSA
        case OSPF_AS_NSSA_LSA:
#endif /* HAVE_NSSA */
          ospf_as_external_lsa_dump (p, lsa_len);
          break;
#ifdef HAVE_OPAQUE_LSA
        case OSPF_LINK_OPAQUE_LSA:
        case OSPF_AREA_OPAQUE_LSA:
        case OSPF_AS_OPAQUE_LSA:
          ospf_opaque_lsa_dump (p, lsa_len);
          break;
#endif /* HAVE_OPAQUE_LSA */
        default:
          break;
        }

      p += lsa_len;
      length -= lsa_len;
      count--;
    }
}

void
ospf_packet_ls_ack_dump (u_char *p, u_int16_t length)
{
  zlog_info (ZG, "Link State Acknowledgment");
  ospf_lsa_header_list_dump (p, length);
}

void
ospf_header_dump (struct ospf_header *ospfh)
{
  char buf[9];
  u_int16_t auth_type;

  zlog_info (ZG, "Header");
  zlog_info (ZG, "  Version %d", ospfh->version);
  zlog_info (ZG, "  Type %d (%s)", ospfh->type,
             ospf_packet_type_string (ospfh->type));
  zlog_info (ZG, "  Packet Len %d", pal_ntoh16 (ospfh->length));
  zlog_info (ZG, "  Router ID %r", &ospfh->router_id);
  zlog_info (ZG, "  Area ID %r", &ospfh->area_id);
  zlog_info (ZG, "  Checksum 0x%x", pal_ntoh16 (ospfh->checksum));

#ifdef HAVE_OSPF_MULTI_INST
  zlog_info (ZG, "  Instance ID %d", ospfh->instance_id);
#endif /* HAVE_OSPF_MULTI_INST */

  auth_type = ospf_packet_auth_type (ospfh);
  zlog_info (ZG, "  AuType %d", auth_type);

  switch (auth_type)
    {
    case OSPF_AUTH_NULL:
      break;
    case OSPF_AUTH_SIMPLE:
      pal_mem_set (buf, 0, 9);
      pal_strncpy (buf, ospfh->u.auth_data, 8);
      zlog_info (ZG, "  Simple Password %s", buf);
      break;
    case OSPF_AUTH_CRYPTOGRAPHIC:
      zlog_info (ZG, "  Cryptographic Authentication");
      zlog_info (ZG, "  Key ID %d", ospfh->u.crypt.key_id);
      zlog_info (ZG, "  Auth Data Len %d", ospfh->u.crypt.auth_data_len);
      zlog_info (ZG, "  Sequence number %d",
                 pal_ntoh32 (ospfh->u.crypt.crypt_seqnum));
      break;
    default:
      zlog_info (ZG, "* This is not supported authentication type");
      break;
    }
}

void
ospf_packet_dump (struct ospf_master *om, u_char *buf)
{
  struct ospf_header *ospfh;
  u_int16_t length;
  u_char *p = buf;

  zlog_info (ZG, "-----------------------------------------------------");

  /* OSPF Header dump. */
  ospfh = (struct ospf_header *)p;

  /* Unless detail flag is set, return. */
  if (!(om->debug.term.packet[ospfh->type - 1] & OSPF_DEBUG_DETAIL))
    return;

  /* Show OSPF header detail. */
  ospf_header_dump (ospfh);
  p += OSPF_HEADER_SIZE;
  length = pal_ntoh16 (ospfh->length) - OSPF_HEADER_SIZE;

  switch (ospfh->type)
    {
    case OSPF_PACKET_HELLO:
      ospf_packet_hello_dump (p, length);
      break;
    case OSPF_PACKET_DB_DESC:
      ospf_packet_db_desc_dump (p, length);
      break;
    case OSPF_PACKET_LS_REQ:
      ospf_packet_ls_req_dump (p, length);
      break;
    case OSPF_PACKET_LS_UPD:
      ospf_packet_ls_upd_dump (p, length);
      break;
    case OSPF_PACKET_LS_ACK:
      ospf_packet_ls_ack_dump (p, length);
      break;
    default:
      break;
    }

  zlog_info (ZG, "-----------------------------------------------------");
}
