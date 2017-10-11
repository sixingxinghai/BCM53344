/* Copyright (C) 2002-2003  All Rights Reserved. */

#include "pal.h"
#include "mpls_common.h"
#include "mpls.h"

/* Helper routine to get string for MPLS owner. */
char *
mpls_dump_owner (mpls_owner_t owner)
{
  switch (owner)
    {
    case MPLS_OTHER:           return "Other";
    case MPLS_SNMP:            return "SNMP";
    case MPLS_LDP:             return "LDP";
    case MPLS_RSVP:            return "RSVP";
    case MPLS_CRLDP:           return "CRLDP";
    case MPLS_POLICYAGENT:     return "Policy Agent";
    case MPLS_UNKNOWN:         return "Unknown";
    case MPLS_OTHER_BGP:       return "BGP";
    case MPLS_OTHER_CLI:       return "CLI";
    case MPLS_OTHER_LDP_VC:    return "LDP VC";
    case MPLS_IGP_SHORTCUT:    return "IGP-Shortcut";
    default:                   return "Unknown";
    }
}

/* Helper routine to get char for MPLS owner. */
char *
mpls_dump_owner_char (mpls_owner_t owner)
{
  switch (owner)
    {
    case MPLS_SNMP:            return "S";
    case MPLS_LDP:             return "L";
    case MPLS_RSVP:            return "R";
    case MPLS_CRLDP:           return "C";
    case MPLS_OTHER_BGP:       return "B";
    case MPLS_OTHER_CLI:       return "K";
    case MPLS_OTHER_LDP_VC:    return "V";
    case MPLS_IGP_SHORTCUT:    return "I";
    case MPLS_UNKNOWN:
    case MPLS_OTHER:
    case MPLS_POLICYAGENT:
    default:                   return " ";
    }
}

/* Helper routing to get string for action type. */
char *
mpls_dump_action_type (ftn_action_type_t type)
{
  switch (type)
    {
    case DROP:                 return "Drop";
    case REDIRECT_LSP:         return "Redirect to LSP";
    case REDIRECT_TUNNEL:      return "Redirect to Tunnel";
    default:                   return "N/A";
    }
}

/* Helper function to dump opcodes. */
char *
nsm_mpls_dump_op_code (u_char opcode)
{
  switch (opcode)
    {
    case PUSH:                   return "Push";
    case POP:                    return "Pop";
    case SWAP:                   return "Swap";
    case POP_FOR_VPN:            return "Pop for VPN";
    case DLVR_TO_IP:             return "Deliver to IP";
    case PUSH_AND_LOOKUP:        return "Push and Lookup";
    case PUSH_FOR_VC:            return "Push for VC";
    case PUSH_AND_LOOKUP_FOR_VC: return "Push and Lookup for VC";
    case POP_FOR_VC:             return "Pop for VC";
    case SWAP_AND_LOOKUP:        return "Swap and Lookup";
    default:                     return "N/A";
    }
}

/* Helper function to dump MPLS notification type. */
char *
nsm_mpls_dump_notification_type (mpls_notification_t type)
{
  switch (type)
    {
    case MPLS_NTF_ILM_ADD_FAILURE:    return "ILM addition";
    case MPLS_NTF_FTN_ADD_FAILURE:    return "FTN addition";
    case MPLS_NTF_VC_FTN_ADD_FAILURE: return "VC FTN addition";
    case MPLS_NTF_VC_ILM_ADD_FAILURE: return "VC ILM addition";
    default:                          return "N/A";
    }
}

/* Helper function to dump MPLS notification type. */
char *
nsm_mpls_dump_notification_status (mpls_status_code_t type)
{
  switch (type)
    {
    default: return "N/A";
    }
}

/* Convert APN specific protocol id to SNMP specific protocol id. */
mpls_owner_t
gmpls_proto_to_owner (u_int32_t proto_id)
{
  switch (proto_id)
    {
    case APN_PROTO_LDP:          return MPLS_LDP;
    case APN_PROTO_RSVP:         return MPLS_RSVP;
    case APN_PROTO_BGP:          return MPLS_OTHER_BGP; 
    case APN_PROTO_VTYSH:        return MPLS_OTHER_CLI;
    default:                     return MPLS_UNKNOWN;
    }
}

/* Convert SNMP specific protocol id to APN Protocol Id */
u_int32_t
gmpls_owner_to_proto (mpls_owner_t owner)
{
  switch (owner)
    {
    case MPLS_LDP:          return APN_PROTO_LDP;
    case MPLS_RSVP:         return APN_PROTO_RSVP;
    case MPLS_OTHER_BGP:    return APN_PROTO_BGP;
    case MPLS_OTHER_CLI:    return APN_PROTO_VTYSH;
    default:                return APN_PROTO_UNSPEC;
    }
}

int
gmpls_addr_in_same (struct addr_in *a1, struct addr_in *a2)
{
  if ((! a1) || (! a2))
    return 0;

  if (a1->afi != a2->afi)
    return 0;

  if (((a1->afi == AFI_IP) 
#ifdef HAVE_GMPLS
       || (a1->afi == AFI_UNNUMBERED)
#endif /* HAVE_GMPLS */
       ) && (IPV4_ADDR_SAME (&a1->u.ipv4, &a2->u.ipv4)))
    return 1;
#ifdef HAVE_IPV6
  else if ((a1->afi == AFI_IP6) &&
      (IPV6_ADDR_SAME (&a1->u.ipv6, &a2->u.ipv6)))
    return 1;
#endif /* HAVE_IPV6 */

  return 0;
}

int
gmpls_addr_in_zero (struct addr_in *addr)
{
  if (! addr)
    return 1;

  if ((addr->afi == AFI_IP)
#ifdef HAVE_GMPLS
      || (addr->afi == AFI_UNNUMBERED)
#endif /* HAVE_GMPLS */
      )
    {
      if (addr->u.ipv4.s_addr == 0)
        return 1;
    }
#ifdef HAVE_IPV6
  else 
    {
      if (addr->afi == AFI_IP6 && (IN6_IS_ADDR_UNSPECIFIED (&addr->u.ipv6)))
        return 1;
    }
#endif /* HAVE_IPV6 */

  return 0;
}


int
mpls_addr_in_to_prefix (struct addr_in *addr, struct prefix *p)
{
  if (! addr || ! p)
    return -1;

  pal_mem_set (p, 0, sizeof (struct prefix));

  if ((addr->afi == AFI_IP)
#ifdef HAVE_GMPLS
      || (addr->afi == AFI_UNNUMBERED)
#endif /* HAVE_GMPLS */
      )
    {
      p->family = AF_INET;
      p->prefixlen = IPV4_MAX_BITLEN;
      IPV4_ADDR_COPY (&p->u.prefix4, &addr->u.ipv4);
    }
#ifdef HAVE_IPV6
  else if (addr->afi == AFI_IP6)
    {
      p->family = AF_INET6;
      p->prefixlen = IPV6_MAX_BITLEN;
      IPV6_ADDR_COPY (&p->u.prefix6, &addr->u.ipv6);
    }
#endif /* HAVE_IPV6 */
  else
    return -1;

  return 0;
}

int
mpls_addr_in_to_str (struct addr_in *addr, char *sz_addr)
{
  if (! addr || ! sz_addr)
    return -1;

  if (addr->afi == AFI_IP)
    pal_inet_ntoa (addr->u.ipv4, sz_addr);
#ifdef HAVE_IPV6
  else if (addr->afi == AFI_IP6)
    pal_inet_ntop (AF_INET6, &addr->u.ipv6, sz_addr, INET6_ADDRSTRLEN);
#endif /* HAVE_IPV6 */
  else
    return -1;
  
  return 0;
}


int 
mpls_owner_ldp_key_same (struct ldp_key *k1, struct ldp_key *k2)
{
  if ((! k1) || (! k2))
    return 0;
  
  if (k1->afi != k2->afi)
    return 0;

  if ((k1->afi == AFI_IP) && 
      (pal_mem_cmp (&k1->u.ipv4, &k2->u.ipv4, 
                    sizeof (struct ldp_key_ipv4)) == 0))
    return 1;
#ifdef HAVE_IPV6
  else if ((k1->afi == AFI_IP6) && 
      (pal_mem_cmp (&k1->u.ipv6, &k2->u.ipv6, 
                    sizeof (struct ldp_key_ipv6)) == 0))
    return 1;
#endif /* HAVE_IPV6 */

  return 0;
}

int 
mpls_owner_rsvp_key_same (struct rsvp_key *k1, struct rsvp_key *k2)
{
  if ((! k1) || (! k2))
    return 0;
  
  if (k1->afi != k2->afi)
    return 0;

  if (k1->len != k2->len)
    return 0;

  if ((k1->afi == AFI_IP) && 
      (pal_mem_cmp (&k1->u.ipv4, &k2->u.ipv4, 
                    sizeof (struct rsvp_key_ipv4)) == 0))
    return 1;
#ifdef HAVE_IPV6
  else if ((k1->afi == AFI_IP6) && 
      (pal_mem_cmp (&k1->u.ipv6, &k2->u.ipv6, 
                    sizeof (struct rsvp_key_ipv6)) == 0))
    return 1;
#endif /* HAVE_IPV6 */

  return 0;
}

int 
mpls_owner_bgp_key_same (struct bgp_key *k1, struct bgp_key *k2)
{
  struct prefix *p1, *p2;

  if ((! k1) || (! k2))
    return 0;

  if (k1->afi != k2->afi)
    return 0;

  if (k1->afi == AFI_IP)
    {
      p1 = (struct prefix *)&k1->u.ipv4.p;
      p2 = (struct prefix *)&k2->u.ipv4.p;

      if (IPV4_ADDR_SAME (&k1->u.ipv4.peer, &k2->u.ipv4.peer) &&
          prefix_same (p1, p2))
        return 1;
    }
#ifdef HAVE_IPV6
  else if (k1->afi == AFI_IP6)
    {
      p1 = (struct prefix *)&k1->u.ipv6.p;
      p2 = (struct prefix *)&k2->u.ipv6.p;
      
      if (IPV6_ADDR_SAME (&k1->u.ipv6.peer, &k2->u.ipv6.peer) &&
          prefix_same (p1, p2))
        return 1;
    }
#endif /* HAVE_IPV6 */
  
  return 0;
}

int 
mpls_owner_crldp_key_same (struct crldp_key *k1, struct crldp_key *k2)
{
  return 1;
}

int
mpls_owner_same (struct mpls_owner *o1, struct mpls_owner *o2)
{
  if ((! o1) || (! o2))
    return 0;

  if (o1->owner != o2->owner)
    return 0;

  switch (o1->owner)
    {
    case MPLS_LDP:
      return mpls_owner_ldp_key_same (&o1->u.l_key, &o2->u.l_key);
    case MPLS_RSVP:
      return mpls_owner_rsvp_key_same (&o1->u.r_key, &o2->u.r_key);
    case MPLS_CRLDP:
      return mpls_owner_crldp_key_same (&o1->u.c_key, &o2->u.c_key);
    case MPLS_OTHER_BGP:
      return mpls_owner_bgp_key_same (&o1->u.b_key, &o2->u.b_key);
    case MPLS_IGP_SHORTCUT:
      return mpls_owner_rsvp_key_same (&o1->u.r_key, &o2->u.r_key);
    case MPLS_OTHER_LDP_VC:
      if (o1->u.vc_key.vc_id == o2->u.vc_key.vc_id)
        return 1;
      return 0;
    case MPLS_OTHER_CLI:
      return 1;
    default:
      return 0;
    }
}

#ifdef HAVE_MPLS_VC
const char *
mpls_vc_type_to_str (u_int16_t vc_type)
{
  switch (vc_type)
    {
    case VC_TYPE_ETH_VLAN:
      return "Ethernet VLAN";
    case VC_TYPE_ETHERNET:
      return "Ethernet";
    case VC_TYPE_HDLC:
      return "hdlc";
    case VC_TYPE_PPP:
      return "Point-to-Point";
    default:
      return "N/A";
    }
}

const char *
mpls_vc_type_to_str2 (u_int16_t vc_type)
{
  switch (vc_type)
    {
    case VC_TYPE_ETH_VLAN:
      return "vlan";
    case VC_TYPE_ETHERNET:
      return "ethernet";
    case VC_TYPE_HDLC:
      return "hdlc";
    case VC_TYPE_PPP:
      return "ppp";
    default:
      return "N/A";
    }
}
#endif /* HAVE_MPLS_VC */

/* Standard mapping between Diffserv class and Differentiated Service
   Code Point value. */
static const char *diffserv_class[DIFFSERV_MAX_SUPPORTED_DSCP] =
{   
  /* Class name. */              /* DSCP value. */   
  "be",                          /* 000000      */
  "undefined_000001",            /* 000001      */
  "undefined_000010",            /* 000010      */
  "undefined_000011",            /* 000011      */
  "undefined_000100",            /* 000100      */
  "undefined_000101",            /* 000101      */
  "undefined_000110",            /* 000110      */
  "undefined_000111",            /* 000111      */
  "cs1",                         /* 001000      */
  "undefined_001001",            /* 001001      */
  "af11",                        /* 001010      */  
  "undefined_001011",            /* 001011      */
  "af12",                        /* 001100      */
  "undefined_001101",            /* 001101      */
  "af13",                        /* 001110      */
  "undefined_001111",            /* 001111      */
  "cs2",                         /* 010000      */
  "undefined_010001",            /* 010001      */
  "af21",                        /* 010010      */
  "undefined_010011",            /* 010011      */
  "af22",                        /* 010100      */
  "undefined_010101",            /* 010101      */
  "af23",                        /* 010110      */
  "undefined_010111",            /* 010111      */
  "cs3",                         /* 011000      */
  "undefined_011001",            /* 011001      */
  "af31",                        /* 011010      */
  "undefined_011011",            /* 011011      */
  "af32",                        /* 011100      */
  "undefined_011101",            /* 011101      */
  "af33",                        /* 011110      */
  "undefined_011111",            /* 011111      */
  "cs4",                         /* 100000      */
  "undefined_100001",            /* 100001      */
  "af41",                        /* 100010      */
  "undefined_100011",            /* 100011      */
  "af42",                        /* 100100      */
  "undefined_100101",            /* 100101      */
  "af43",                        /* 100110      */
  "undefined_100111",            /* 100111      */
  "cs5",                         /* 101000      */
  "undefined_101001",            /* 101001      */
  "undefined_101010",            /* 101010      */
  "undefined_101011",            /* 101011      */
  "undefined_101100",            /* 101100      */
  "undefined_101101",            /* 101101      */
  "ef",                          /* 101110      */
  "undefined_101111",            /* 101111      */
  "cs6",                         /* 110000      */
  "undefined_110001",            /* 110001      */
  "undefined_110010",            /* 110010      */
  "undefined_110011",            /* 110011      */
  "undefined_110100",            /* 110100      */
  "undefined_110101",            /* 110101      */
  "undefined_110110",            /* 110110      */
  "undefined_110111",            /* 110111      */
  "cs7",                         /* 111000      */
  "undefined_111001",            /* 111001      */
  "undefined_111010",            /* 111010      */
  "undefined_111011",            /* 111011      */
  "undefined_111100",            /* 111100      */
  "undefined_111101",            /* 111101      */
  "undefined_111110",            /* 111110      */
  "undefined_111111"             /* 111111      */
};

/* Get diffserv class name. */
char *
diffserv_class_name (u_char dscp)
{
  if (dscp < DIFFSERV_MAX_SUPPORTED_DSCP)
    return (char *)diffserv_class[dscp];
  else
    return "invalid_class";
}

bool_t
gmpls_are_label_equal (struct gmpls_gen_label *lbl1,
                       struct gmpls_gen_label *lbl2)
{
  if (lbl1->type != lbl2->type)
    return PAL_FALSE;
  switch (lbl1->type)
    {
      case gmpls_entry_type_ip:
        if (lbl1->u.pkt != lbl2->u.pkt)
          return PAL_FALSE;
        break;

#ifdef HAVE_GMPLS
#ifdef HAVE_PBB_TE
      case gmpls_entry_type_pbb_te:
        if (pal_mem_cmp (&lbl1->u.pbb, &lbl2->u.pbb,
                         sizeof (struct pbb_te_label)))
          return PAL_FALSE;

        break;
#endif /* HAVE_PBB_TE */
#ifdef HAVE_TDM
      case gmpls_entry_type_tdm:
        if (pal_mem_cmp (&lbl1->u.tdm, lbl2->u.tdm,
                         sizeof (struct tdm_label)))
          return PAL_FALSE;
        break;
#endif /* HAVE_TDM */
#endif /* HAVE_GMPLS */
      default:
        return PAL_FALSE;
    }
  return PAL_TRUE;
}
