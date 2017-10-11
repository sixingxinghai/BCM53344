/* Copyright (C) 2002-2003  All Rights Reserved. */

/* NSM message implementation.  This implementation is used by both
   server and client side.  */

#include "pal.h"
#include "network.h"
#include "nexthop.h"
#include "message.h"
#include "nsm_message.h"
#include "log.h"
#include "tlv.h"
#include "nsm_client.h"
#ifdef HAVE_MPLS
#include "label_pool.h"
#include "mpls.h"
#include "mpls_common.h"
#ifdef HAVE_DIFFSERV
#include "diffserv.h"
#endif /* HAVE_DIFFSERV */
#ifdef HAVE_DSTE
#include "dste.h"
#include "dste/nsm_dste.h"
#endif /* HAVE_DSTE */

#include "nsmd.h"


#include "nsm_mpls.h"
#ifdef HAVE_MPLS_VC
#include "nsm_mpls_vc.h"
#endif /* HAVE_MPLS_VC */
#endif /* HAVE_MPLS */
#ifdef HAVE_GMPLS
#include "gmpls/gmpls.h"
#endif /* HAVE_GMPLS */
#ifdef HAVE_MCAST_IPV4
#include "lib_mtrace.h"
#endif /* HAVE_MCAST_IPV4 */
#ifdef HAVE_MPLS_OAM
#include "bfd_message.h"
#endif /* HAVE_MPLS_OAM */

/* NSM message strings.  */
static const char *nsm_msg_str[] =
{
  "Service Request",                             /* 0 */
  "Service Reply",                               /* 1 */
  "VR Add",                                      /* 2 */
  "VR Delete",                                   /* 3 */
  "VR Update",                                   /* 4 */
  "Link Add",                                    /* 5 */
  "Link Delete",                                 /* 6 */
  "Link Bulk Update",                            /* 7 */
  "Link Attribute Update",                       /* 8 */
  "VR Bind",                                     /* 9 */
  "VR Bind Bulk",                                /* 10 */
  "VR Unbind",                                   /* 11 */
  "Address Add",                                 /* 12 */
  "Address Delete",                              /* 13 */
  "Address Bulk Update",                         /* 14 */

  "VRF Add",                                     /* 15 */
  "VRF Delete",                                  /* 16 */
  "VRF Update",                                  /* 17 */
  "VRF Bind",                                    /* 18 */
  "VRF Bind Bulk",                               /* 19 */
  "VRF Unbind",                                  /* 20 */
  "Route Preserve State",                        /* 21 */
  "Route Stale Remove",                          /* 22 */
  "Protocol restart",                            /* 23 */
  "Unknown",                                     /* 24 */ /* No message defined */
  "User add",                                    /* 25 */
  "User delete",                                 /* 26 */
  "User update",                                 /* 27 */
  "Hostname update",                             /* 28 */

  "Link Up",                                     /* 29 */
  "Link Down",                                   /* 30 */
  "IPv4 Route",                                  /* 31 */
  "IPv6 Route",                                  /* 32 */
  "Redistribute Set",                            /* 33 */
  "Redistribute Unset",                          /* 34 */
  "Route Clean",                                 /* 35 */
  "Router ID update",                            /* 36 */

  "IPv4 nexthop best match lookup",              /* 37 */
  "IPv6 nexthop best match lookup",              /* 38 */
  "IPv4 nexthop exact match lookup",             /* 39 */
  "IPv6 nexthop exact match lookup",             /* 40 */
  "IPv4 nexthop best lookup register",           /* 41 */
  "IPv4 nexthop best lookup deregister",         /* 42 */
  "IPv4 nexthop exact lookup register",          /* 43 */
  "IPv4 nexthop exact lookup deregister",        /* 44 */
  "IPv6 nexthop best lookup register",           /* 45 */
  "IPv6 nexthop best lookup deregister",         /* 46 */
  "IPv6 nexthop exact lookup register",          /* 47 */
  "IPv6 nexthop exact lookup deregister",        /* 48 */

  "Label Pool Request",                          /* 49 */
  "Label Pool Release",                          /* 50 */

  "Admin Group Update",                          /* 51 */
  "Interface Priority BW Update",                /* 52 */

  "QoS client init",                             /* 53 */
  "QoS client probe",                            /* 54 */
  "QoS client reserve",                          /* 55 */
  "QoS client release",                          /* 56 */
  "QoS client release slow",                     /* 57 */
  "QoS client preempt",                          /* 58 */
  "QoS client modify",                           /* 59 */
  "QoS client clean",                            /* 60 */

  "MPLS VC add",                                 /* 61 */
  "MPLS VC delete",                              /* 62 */

  "MPLS FTN IPv4",                               /* 63 */
  "MPLS FTN IPv6",                               /* 64 */
  "MPLS FTN reply",                              /* 65 */
  "MPLS ILM IPv4",                               /* 66 */
  "MPLS ILM IPv6",                               /* 67 */
  "MPLS ILM reply",                              /* 68 */
  "MPLS IGP SHORTCUT LSP",                       /* 69 */
  "MPLS IGP SHORTCUT ROUTE",                     /* 70 */
  "MPLS VC FIB ADD",                             /* 71 */
  "MPLS VC FIB DELETE",                          /* 72 */
  "MPLS VC STATUS",                              /* 73 */
  "Unknown",                                     /* 74 */
  "MPLS NOTIFICATION",                           /* 75 */

  "GMPLS interface TE data",                     /* 76 */
  "IGMP Local Membership",                       /* 77 */
  "Supported DSCP Update",                       /* 78 */
  "DSCP EXP Map Update",                         /* 79 */

  "VPLS Add",                                    /* 80 */
  "VPLS Delete",                                 /* 81 */
  "VPLS Fib Add",                                /* 82 */
  "VPLS Fib Delete",                             /* 83 */
  "VPLS MAC Address Withdraw",                   /* 84 */
  "DSTE Class Type Update",                      /* 85 */
  "DSTE TE Class Update",                        /* 86 */

  "IPV4 MRIB VIF add",                           /* 87 */
  "IPV4 MRIB VIF del",                           /* 88 */
  "IPV4 MRIB MRT add",                           /* 89 */
  "IPV4 MRIB MRT del",                           /* 90 */
  "IPV4 MRIB MRT Stat Flags Update",             /* 91 */
  "IPV4 MRIB MRT NoCache",                       /* 92 */
  "IPV4 MRIB MRT WrongVif",                      /* 93 */
  "IPV4 MRIB MRT Wholepkt Req",                  /* 94 */
  "IPV4 MRIB MRT Wholepkt Reply",                /* 95 */
  "IPV4 MRIB MRT stat update",                   /* 96 */
  "IPV4 MRIB Notification",                      /* 97 */

  "IPV6 MRIB MIF add",                           /* 98 */
  "IPV6 MRIB MIF del",                           /* 99 */
  "IPV6 MRIB MRT add",                           /* 100 */
  "IPV6 MRIB MRT del",                           /* 101 */
  "IPV6 MRIB MRT Stat Flags Update",             /* 102 */
  "IPV6 MRIB MRT NoCache",                       /* 103 */
  "IPV6 MRIB MRT WrongVif",                      /* 104 */
  "IPV6 MRIB MRT Wholepkt Req",                  /* 105 */
  "IPV6 MRIB MRT Wholepkt Reply",                /* 106 */
  "IPV6 MRIB MRT stat update",                   /* 107 */
  "IPV6 MRIB Notification",                      /* 108 */

  "MLD Local-Membership",                        /* 109 */

  "Unknown",                                     /* 110  Missing !! */
  "Unknown",                                     /* 111  Missing !! */
  "Unknown",                                     /* 112  Missing !! */

  "Bridge Port State Sync",                      /* 113 */
  "STP Interface",                               /* 114 */
  "Bridge Add",                                  /* 115 */
  "Bridge Delete",                               /* 116 */
  "Bridge Add Port",                             /* 117 */
  "Bridge Delete Port",                          /* 118 */

  "LACP Aggregate Add",                          /* 119 */
  "LACP Aggregate Del",                          /* 120 */

  "VLAN Add",                                    /* 121 */
  "VLAN Delete",                                 /* 122 */
  "VLAN Add Port",                               /* 123 */
  "VLAN Delete Port",                            /* 124 */
  "VLAN port type",                              /* 125 */

  "VLAN Classifier Add",                         /* 126 */
  "VLAN Classifier Delete",                      /* 127 */
  "VLAN Port Classifier Add",                    /* 128 */
  "VLAN Port Classifier Delete" ,                /* 129 */

  "Bridge Port State",                           /* 130 */
  "Bridge Port Flush FDB",                       /* 131 */
  "Bridge Topology Change Notification",         /* 132 */
  "Bridge Set Ageing Time",                      /* 133 */

  "VLAN Add To Instance",                        /* 134 */
  "VLAN Del From Instance",                      /* 135 */

  "AUTH Port State",                             /* 136 */

  "MPLS FTN add reply",                          /* 137 */
  "MPLS FTN del reply",                          /* 138 */
  "MPLS ILM add reply",                          /* 139 */
  "MPLS ILM del reply",                          /* 140 */
  "MPLS VC FTN add reply",                       /* 141 */
  "MPLS VC FTN del reply",                       /* 142 */
  "MPLS VC ILM add reply",                       /* 143 */
  "MPLS VC ILM del reply",                       /* 144 */
  "Static AGG count",                            /* 145 */
  "NSM LACP Aggregator Config",                  /* 146 */
  "Interface Del Done",                          /* 147 */
  "PVLAN Port Host Associate",                   /* 148 */
  "RMON Request Stats",                          /* 149 */
  "RMON Service Stats",                          /* 150 */
  "MPLS FTN Add Reply",                          /* 151 */
  "Mtrace Request msg",                          /* 152 */
  "PVLAN Configure",                             /* 153 */
  "PVLAN Secondary VLAN Associate",              /* 154 */
  "IPV6 Mcast route state refresh flag update",  /* 155 */
  "IPV4 Mcast route state refresh flag update",  /* 156 */
  "Mtrace Query msg",                            /* 157 */
  "MACAUTH Port State",                          /* 158 */
  "AUTH MACAUTH Status",                         /* 159 */
  "MPLS ILM Lookup",                             /* 160 */
  "MPLS VC ILM add",                             /* 161 */
  "MPLS ECHO request",                           /* 162 */
  "MPLS PING reply",                             /* 163 */
  "MPLS TRACERT reply",                          /* 164 */
  "MPLS OAM error",                              /* 165 */
  "MPLS OAM L3VPN msg",                          /* 166 */
  "SVLAN add CE port msg",                       /* 167 */
  "SVLAN delete CE port msg",                    /* 168 */
  "EFM OAM interface msg",                       /* 169 */
  "MPLS VC TUNNEL Info",                         /* 170 */
  "OAM  LLDP msg",                               /* 171 */
  "VLAN set PVID",                               /* 172 */
  "COMMSG",                                      /* 173 */
  "Set BPDU process",                            /* 174 */
  "CFM Interface Index Request",                 /* 175 */
  "CFM Get Interface Index",                     /* 176 */
  "OAM CFM message",                             /* 177 */
  "Bridge set state Message",                    /* 178 */
  "SVID-ISID ADD at PIP",                        /* 179 */
  "SVLAN to ISID delete at PIP",                 /* 180 */
  "ISID to BVID Mapping",                        /* 181 */
  "ISID to BVID Mapping DEL",                    /* 182 */
  "ISID Delete at PIP and CBP",                  /* 183 */
  "PBB ISID Dispatch",                           /* 184 */
  "MPLS OAM ITUT Process Req",                   /* 185 */
  "VR Sync MSG to ONMD after config restore",    /* 186 */
  "Route Stale Mark",                            /* 187 */
  "Nexthop Tracking ",                           /* 188 */
  "ISIS Wait for BGP convergence",               /* 189 */
  "BGP convergence done",                        /* 190 */
  "BGP convergence done to ISIS",                /* 191 */
  "BGP is Up",                                   /* 192 */
  "BGP is down",                                 /* 193 */
  "pbb-te vlan add",                             /* 194 */
  "pbb-te vlan delete",                          /* 195 */
  "pbb-te esp add",                              /* 196 */
  "pbb-te esp delete",                           /* 197 */
  "pbb-te esp pnp add",                          /* 198 */
  "pbb-te esp pnp delete",                       /* 199 */
  "Pbb-te bridge port state",                    /* 200 */
  "Pbb-te APS Group Add",                        /* 201 */
  "Pbb-te APS Group Delete",                     /* 202 */
  "G8031 Protection group create",               /* 203 */
  "G8031 VLAN Group create",                     /* 204 */
  "G8031 Delete Protection Group",               /* 205 */
  "G8031 PG Initialized",                        /* 206 */
  "G8031 PG Port State",                         /* 207 */
  "G8032 Ring Add",                              /* 208 */
  "G8032 Ring Delete",                           /* 209 */
  "G8032 VLAN Group Create",                     /* 210 */
  "G8032 VLAN Group Delete",                     /* 211 */
  "G8032 PG Port State",                         /* 212 */
  "LACP Add Aggregator Member",                  /* 213 */
  "LACP Delete Aggregator Member",               /* 214 */
  "VLAN add to Protection",                      /* 215 */
  "VLAN del from Protection",                    /* 216 */
  "Bridge add Protection group",                 /* 217 */
  "Bridge del Protection group",                 /* 218 */
  "Block Instance",                              /* 219 */
  "Unblock Instance",                            /* 220 */
  "Stale Recovery Info",                         /* 221 */
  "Untagged vid PE port",                        /* 222 */
  "Default vid PE port",                         /* 223 */
  "ELMI CFM Operational State",                  /* 224 */
  "ELMI EVC Add",                                /* 225 */
  "ELMI EVC delete",                             /* 226 */
  "ELMI EVC Update",                             /* 227 */
  "ELMI UNI Add",                                /* 228 */
  "ELMI UNI Delete",                             /* 229 */
  "ELMI UNI Update",                             /* 230 */
  "ELMI UNI BW Add",                             /* 231 */
  "ELMI UNI BW Delete",                          /* 232 */
  "ELMI EVC BW Add",                             /* 233 */
  "ELMI EVC BW Delete",                          /* 234 */
  "ELMI EVC COS BW Add",                         /* 235 */
  "ELMI EVC COS BW Delete",                      /* 236 */
  "ELMI Operational State",                      /* 237 */
  "ELMI NSM Auto vlan add",                      /* 238 */
  "ELMI NSM Auto vlan delete",                   /* 239 */
  "Data Link",                                   /* 240 */
  "Data Link Sub",                               /* 241 */
  "TE Link",                                     /* 242 */
  "Control Channel",                             /* 243 */
  "Control Adjacency",                           /* 244 */
  "QoS Client Probe Release",                    /* 245 */
  "GMPLS FTN",                                   /* 246 */
  "GMPLS ILM",                                   /* 247 */
  "GMPLS BIDIR FTN",                             /* 248 */
  "GMPLS BIDIR ILM",                             /* 249 */
  "GMPLS LABEL POOL Request",                    /* 250 */
  "GMPLS LABEL POOL Release",                    /* 251 */
  "ILM GEN LOOKUP",                              /* 252 */
  "FTN GEN IPV4",                                /* 253 */
  "FTN GEN Reply",                               /* 254 */
  "ILM GEN IPV4",                                /* 255 */
  "ILM GEN Add Reply",                           /* 256 */
  "ILM GEN Delete Reply",                        /* 257 */
  "ILM GEN Reply",                               /* 258 */
  "BIDIR FTN Reply",                              /* 259 */
  "BIDIR ILM REPLY",                              /* 260 */
  "GEN Stale Info",                               /* 261 */
  "LMP Greaceful Restart",                        /* 262 */
  "DLINK OPAQUE",				 /* 263 */
  "LABEL POOL IN USE",				 /* 264 */
  "PBB TESTID INFO",                              /* 265 */
  "QOS CLIENT INIT",                              /* 266 */
  "QOs CLIENT PROBE",                             /* 267 */
  "Qos CLIENT RESERVE",                           /* 268 */
  "QOS CLIENT RELEASE",                           /* 269 */
  "QOS CLIENT RELEASE SLOW",                      /* 270 */
  "QOS CLIENT PREMPT",                            /* 271 */
  "QOS CLIENT MODIFY",                            /* 272 */
  "QOS CLIENT CLEAN",                             /* 273 */
  "DLINK TEST SEND MESSAGE",                      /* 274 */
  "OAM echo request process",                    /* 275 */
  "OAM echo reply process LDP FEC data",         /* 276 */
  "OAM echo reply process Static FEC data",      /* 277 */
  "OAM echo reply process RSVP Tunnel data",     /* 278 */
  "OAM echo reply process L2VC data",            /* 279 */
  "OAM echo reply process L3VPN data",           /* 280 */
  "OAM echo respone LDP FEC data",               /* 281 */
  "OAM echo respone Static FEC data",            /* 282 */
  "OAM echo respone RSVP Tunnel data",           /* 283 */
  "OAM echo respone L2VC data",                  /* 284 */
  "OAM echo respone L3VPN data",                 /* 285 */
  "OAM echo respone error",                      /* 286 */
  "OAM echo reply response",                      /* 287 */
  "OAM UPDATE",                                   /* 288 */
  "FTN DOWN",                                     /* 289 */
  "MS PW",                                        /* 290 */
  "DLINK CHANNEL MONITOR",                        /* 291 */
  "BRIDGE PORT SPANNING TREE ENABLE",             /* 292 */
  "NSM Server Status",                            /* 293 */
  "LDP UP",                                       /* 294 */
  "LDP DOWN",                                     /* 295 */
  "LDP Session state",                            /* 296 */
  "LDP Session state query"                       /* 297 */
};

/* NSM service strings.  */
static const char *nsm_service_str[] =
{
  "Interface Service",                    /* 0 */
  "Route Service",                        /* 1 */
  "Router ID Service",                    /* 2 */
  "VRF Service",                          /* 3 */
  "Nexthop Lookup Service",               /* 4 */
  "Label Service",                        /* 5 */
  "TE service",                           /* 6 */
  "QoS Service",                          /* 7 */
  "QoS Preempt Service",                  /* 8 */
  "User Service",                         /* 9 */
  "Hostname service",                     /* 10 */
  "Virtual Circuit",                      /* 11 */
  "MPLS service",                         /* 12 */
  "GMPLS service",                        /* 13 */
  "Diffserv service",                     /* 14 */
  "VPLS service",                         /* 15 */
  "DSTE service",                         /* 16 */
  "IPV4 MRIB service",                    /* 17 */
  "IPV4 MRIB PIM service",                /* 18 */
  "IPV4 MRIB mcast tunnel service",       /* 19 */
  "IPV6 MRIB service",                    /* 20 */
  "IPV6 MRIB PIM service",                /* 21 */
  "Bridge service",                       /* 22 */
  "VLAN service",                         /* 23 */
  "IGP Shortcut service",                 /* 24 */
  "Control Adjacency service",            /* 25 */
  "Control Channel service",              /* 26 */
  "TE Link service",                      /* 27 */
  "Data Link service",                    /* 28 */
  "Data Link Sub service"                 /* 29 */
};

/* NSM message to string.  */
const char *
nsm_msg_to_str (int type)
{
  if (type < NSM_MSG_MAX)
    return nsm_msg_str [type];

  return "Unknown";
}

/* NSM service to string.  */
const char *
nsm_service_to_str (int service)
{
  if (service < NSM_SERVICE_MAX)
    return nsm_service_str [service];

  return "Unknown Service";
}

#ifdef HAVE_L2
static const char *nsm_bridge_type_str[] =
  {
    "STP IEEE Bridge",
    "STP VLAN Aware Bridge",
    "RSTP IEEE Bridge",
    "RSTP VLAN Aware Bridge",
    "MSTP Bridge"
  };

static const char *nsm_bridge_port_state_str[] =
  {
    "Disabled",
    "Listening",
    "Learning",
    "Forwarding",
    "Blocking",
    "Discarding"
  };

#endif /* HAVE_L2 */

void
nsm_header_dump (struct lib_globals *zg, struct nsm_msg_header *header)
{
  zlog_info (zg, "NSM Message Header");
  zlog_info (zg, " VR ID: %lu", header->vr_id);
  zlog_info (zg, " VRF ID: %lu", header->vrf_id);
  zlog_info (zg, " Message type: %s (%d)", nsm_msg_to_str (header->type),
             header->type);
  zlog_info (zg, " Message length: %d", header->length);
  zlog_info (zg, " Message ID: 0x%08x", header->message_id);
}

void
nsm_service_dump (struct lib_globals *zg, struct nsm_msg_service *service)
{
  int i;
  afi_t afi;
  safi_t safi;

  zlog_info (zg, "NSM Service");
  zlog_info (zg, " Version: %d", service->version);
  zlog_info (zg, " Reserved: %d", service->reserved);
  zlog_info (zg, " Protocol ID: %s (%d)", modname_strl (service->protocol_id),
             service->protocol_id);
  zlog_info (zg, " Client ID: %d", service->client_id);
  zlog_info (zg, " Bits: %d", service->bits);
  zlog_info (zg, " Restart State: %d", service->restart_state);
  for (i = 0; i < NSM_CINDEX_SIZE; i++)
    {
      if (NSM_CHECK_CTYPE (service->bits, i))
        zlog_info (zg, "  %s", nsm_service_to_str (i));
    }

  if (NSM_CHECK_CTYPE (service->cindex, NSM_SERVICE_CTYPE_RESTART))
    {
      for (afi = AFI_IP; afi < AFI_MAX; afi++)
        for (safi = SAFI_UNICAST; safi < SAFI_MAX; safi++)
          if (service->restart[afi][safi])
            zlog_info (zg, " Restart: AFI %d SAFI %d", afi, safi);
    }
#ifdef HAVE_MPLS
  if (NSM_CHECK_CTYPE (service->cindex, NSM_SERVICE_CTYPE_LABEL_POOLS))
    {
      zlog_info (zg, " Label Pool Num: %d", service->label_pool_num);
      for (i = 0; i < service->label_pool_num; i++)
        {
          zlog_info (zg, "Label Pool [%d]: %d: %d: %d: %d: %d",
                     i,
                     service->label_pools[i].cindex,
                     service->label_pools[i].label_space,
                     service->label_pools[i].label_block,
                     service->label_pools[i].min_label,
                     service->label_pools[i].max_label);
        }
    }
#endif /* HAVE_MPLS */
}

void
nsm_interface_dump (struct lib_globals *zg, struct nsm_msg_link *msg)
{
  zlog_info (zg, "NSM Interface");
  zlog_info (zg, " Interface index: %d", msg->ifindex);

  if (NSM_CHECK_CTYPE (msg->cindex, NSM_LINK_CTYPE_NAME))
    zlog_info (zg, " Name: %s", msg->ifname);
#ifdef HAVE_GMPLS
  if (NSM_CHECK_CTYPE (msg->cindex, NSM_LINK_CTYPE_GMPLS_TYPE))
    zlog_info (zg, " GMPLS-type: %s", gmpls_interface_type_to_str (msg->gtype));
#endif /* HAVE_GMPLS */
  if (NSM_CHECK_CTYPE (msg->cindex, NSM_LINK_CTYPE_FLAGS))
    zlog_info (zg, " Flags: %d", msg->flags);
  if (NSM_CHECK_CTYPE (msg->cindex, NSM_LINK_CTYPE_STATUS))
    zlog_info (zg, " Status: 0x%08x", msg->status);
#ifdef HAVE_VRX
  if (NSM_CHECK_CTYPE (msg->cindex, NSM_LINK_CTYPE_VRX_FLAG))
    zlog_info (zg, " VRX_flag: %d", msg->vrx_flag);
  if (NSM_CHECK_CTYPE (msg->cindex, NSM_LINK_CTYPE_VRX_FLAG_LOCALSRC))
    zlog_info (zg, " Local_flag: %d", msg->local_flag);
#endif /* HAVE_VRX */
  if (NSM_CHECK_CTYPE (msg->cindex, NSM_LINK_CTYPE_METRIC))
    zlog_info (zg, " Metric: %d", msg->metric);
  if (NSM_CHECK_CTYPE (msg->cindex, NSM_LINK_CTYPE_MTU))
    zlog_info (zg, " MTU: %d", msg->mtu);
  if (NSM_CHECK_CTYPE (msg->cindex, NSM_LINK_CTYPE_HW_ADDRESS))
    {
      zlog_info (zg, " HW type: %d", msg->hw_type);
      zlog_info (zg, " HW len: %d", msg->hw_addr_len);
      if (msg->hw_addr_len == ETHER_ADDR_LEN)
        zlog_info (zg, " HW address: %02x%02x.%02x%02x.%02x%02x",
                   msg->hw_addr[0], msg->hw_addr[1], msg->hw_addr[2],
                   msg->hw_addr[3], msg->hw_addr[4], msg->hw_addr[5]);
    }
  if (NSM_CHECK_CTYPE (msg->cindex, NSM_LINK_CTYPE_BANDWIDTH))
    zlog_info (zg, " Bandwidth: %f", msg->bandwidth);
#ifdef HAVE_TE
  if (NSM_CHECK_CTYPE (msg->cindex, NSM_LINK_CTYPE_MAX_RESV_BANDWIDTH))
    zlog_info (zg, " Max Resv Bandwidth: %f", msg->max_resv_bw);
#ifdef HAVE_DSTE
  if (NSM_CHECK_CTYPE (msg->cindex, NSM_LINK_CTYPE_BC_MODE))
    zlog_info (zg, " Bandwidth constraint model: %d",
               msg->bc_mode);
  if (NSM_CHECK_CTYPE (msg->cindex, NSM_LINK_CTYPE_BW_CONSTRAINT))
    {
      int ctr;

      for (ctr = 0; ctr < MAX_BW_CONST; ctr++)
        zlog_info (zg, " Bandwidth Constraint[%d]: %f",
                   ctr, msg->bw_constraint[ctr]);
    }
#endif /* HAVE_DSTE */
#endif /* HAVE_TE */

#ifdef HAVE_TE
  {
    int i;

    if (NSM_CHECK_CTYPE (msg->cindex, NSM_LINK_CTYPE_ADMIN_GROUP))
      zlog_info (zg, " Admin Group: %d", msg->admin_group);
    if (NSM_CHECK_CTYPE (msg->cindex, NSM_LINK_CTYPE_PRIORITY_BW))
      for (i = 0; i < MAX_PRIORITIES; i++)
        {
#ifdef HAVE_DSTE
          zlog_info (zg, " Available b/w for TE CLASS %d: %f", i,
                     msg->tecl_priority_bw[i]);
#else /* HAVE_DSTE */
          zlog_info (zg, " Priority Bandwidth %d: %f", i,
                     msg->tecl_priority_bw[i]);
#endif /* HAVE_DSTE */
        }
  }
#endif /* HAVE_TE */
#ifdef HAVE_MPLS
  if (NSM_CHECK_CTYPE (msg->cindex, NSM_LINK_CTYPE_LABEL_SPACE))
    {
      zlog_info (zg, " Label status: %d", msg->lp.status);
      zlog_info (zg, " Label space: %d", msg->lp.label_space);
      zlog_info (zg, " Min label value: %d", msg->lp.min_label_value);
      zlog_info (zg, " Max label value: %d", msg->lp.max_label_value);
    }
#endif /* HAVE_MPLS */

#ifdef HAVE_VPLS
  if (NSM_CHECK_CTYPE (msg->cindex, NSM_LINK_CTYPE_BINDING))
    zlog_info (zg, " Interface bind flag %d", msg->bind);
#endif /* HAVE_VPLS */

#ifdef HAVE_LACPD
  if (NSM_CHECK_CTYPE (msg->cindex, NSM_LINK_CTYPE_LACP_KEY))
    {
      zlog_info (zg, " Interface lacp key flag %d", msg->lacp_admin_key);
      zlog_info (zg, " Interface lacp aggregator update flag %d", msg->agg_param_update);
      zlog_info (zg, " Interface lacp aggregator key %d", msg->lacp_agg_key);

    }
#endif /* HAVE_LACPD */
}

void
nsm_address_dump (struct lib_globals *zg, struct nsm_msg_address *msg)
{
  zlog_info (zg, "NSM Address");
  zlog_info (zg, " Interface index: %d", msg->ifindex);

  zlog_info (zg, " Flags: %d", msg->flags);
  zlog_info (zg, " Prefixlen: %d", msg->prefixlen);
  zlog_info (zg, " AFI: %d", msg->afi);

  if (msg->afi == AFI_IP)
    {
      zlog_info (zg, " Interface Address(SRC): %r", &msg->u.ipv4.src);
      zlog_info (zg, " Interface Address(DST): %r", &msg->u.ipv4.dst);
    }
#ifdef HAVE_IPV6
  else if (msg->afi == AFI_IP6)
    {
      zlog_info (zg, " Interface Address(SRC): %R", &msg->u.ipv6.src);
      zlog_info (zg, " Interface Address(DST): %R", &msg->u.ipv6.dst);
    }
#endif /* HAVE_IPV6 */
}

void
nsm_route_ipv4_dump (struct lib_globals *zg, struct nsm_msg_route_ipv4 *msg)
{
  int i;
  struct nsm_tlv_ipv4_nexthop *nexthop;

  if (CHECK_FLAG (msg->flags, NSM_MSG_ROUTE_FLAG_ADD))
    zlog_info (zg, "NSM IPv4 route add");
  else
    zlog_info (zg, "NSM IPv4 route delete");

  if (NSM_CHECK_CTYPE (msg->cindex, NSM_ROUTE_CTYPE_IPV4_TAG))
    zlog_info (zg, " Tag: %lu", msg->tag);

#ifdef HAVE_MPLS
  if (NSM_CHECK_CTYPE (msg->cindex, NSM_ROUTE_CTYPE_MPLS_IPV4_NHOP))
    {
      zlog_info (zg, " MPLS mapped data:");
      zlog_info (zg, "  Outgoing ifindex: %d", msg->mpls_oif_ix);
      zlog_info (zg, "  Outgoing label: %d", msg->mpls_out_label);
      zlog_info (zg, "  Nexthop address: %r", &msg->mpls_nh_addr);
      zlog_info (zg, "  FTN IX: %d", &msg->mpls_ftn_ix);
    }
#endif /* HAVE_MPLS */

  zlog_info (zg, " Flags: %d", msg->flags);
  zlog_info (zg, " Route: %r/%d", &msg->prefix, msg->prefixlen);
  zlog_info (zg, " Type: %d", msg->type);
  zlog_info (zg, " Metric: %d", msg->metric);
  zlog_info (zg, " Distance: %d", msg->distance);

  if (msg->nexthop_num > NSM_TLV_IPV4_NEXTHOP_NUM)
    nexthop = msg->nexthop_opt;
  else
    nexthop = msg->nexthop;

  for (i = 0; i < msg->nexthop_num; i++)
    zlog_info (zg, " Nexthop: %r ifindex %d", &nexthop[i].addr,
               nexthop[i].ifindex);
}

#ifdef HAVE_IPV6
void
nsm_route_ipv6_dump (struct lib_globals *zg, struct nsm_msg_route_ipv6 *msg)
{
  int i;
  struct nsm_tlv_ipv6_nexthop *nexthop;

  if (CHECK_FLAG (msg->flags, NSM_MSG_ROUTE_FLAG_ADD))
    zlog_info (zg, "NSM IPv6 route add");
  else
    zlog_info (zg, "NSM IPv6 route delete");

  if (NSM_CHECK_CTYPE (msg->cindex, NSM_ROUTE_CTYPE_IPV6_TAG))
    zlog_info (zg, " Tag: %lu", msg->tag);

#ifdef HAVE_MPLS
  if (NSM_CHECK_CTYPE (msg->cindex, NSM_ROUTE_CTYPE_MPLS_IPV6_NHOP))
    {
      zlog_info (zg, " MPLS mapped data:");
      zlog_info (zg, "  Outgoing ifindex: %d", msg->mpls_oif_ix);
      zlog_info (zg, "  Outgoing label: %d", msg->mpls_out_label);
      zlog_info (zg, "  Nexthop address: %R", &msg->mpls_nh_addr);
    }
#endif /* HAVE_MPLS */

  zlog_info (zg, " Flags: %d", msg->flags);
  zlog_info (zg, " Route: %R/%d", &msg->prefix, msg->prefixlen);
  zlog_info (zg, " Type: %d", msg->type);
  zlog_info (zg, " Metric: %d", msg->metric);
  zlog_info (zg, " Distance: %d", msg->distance);

  if (msg->nexthop_num > NSM_TLV_IPV6_NEXTHOP_NUM)
    nexthop = msg->nexthop_opt;
  else
    nexthop = msg->nexthop;

  for (i = 0; i < msg->nexthop_num; i++)
    zlog_info (zg, " Nexthop: %R ifindex %d", &nexthop[i].addr,
               nexthop[i].ifindex);
}
#endif /* HAVE_IPV6 */

void
nsm_redistribute_dump (struct lib_globals *zg,
                       struct nsm_msg_redistribute *msg)
{
  zlog_info (zg, "NSM redistribute");
  zlog_info (zg, " Route type: %d", msg->type);
  zlog_info (zg, " AFI: %d", msg->afi);
}

void
nsm_route_lookup_ipv4_dump (struct lib_globals *zg,
                            struct nsm_msg_route_lookup_ipv4 *msg)
{
  zlog_info (zg, "Nexthop lookup IPV4");
  zlog_info (zg, " IP address: %r", &msg->addr);
  if (NSM_CHECK_CTYPE (msg->cindex, NSM_ROUTE_LOOKUP_CTYPE_PREFIXLEN))
    zlog_info (zg, " Prefixlen: %d", msg->prefixlen);
  if (NSM_CHECK_CTYPE (msg->cindex, NSM_ROUTE_LOOKUP_CTYPE_LSP))
    zlog_info (zg, " MPLS lookup desired");
  if (NSM_CHECK_CTYPE (msg->cindex, NSM_ROUTE_LOOKUP_CTYPE_LSP_PROTO))
    zlog_info (zg, " MPLS lookup desired(protocol)");
}

#ifdef HAVE_IPV6
void
nsm_route_lookup_ipv6_dump (struct lib_globals *zg,
                            struct nsm_msg_route_lookup_ipv6 *msg)
{
  zlog_info (zg, "Nexthop lookup IPV6");
  zlog_info (zg, " IP address: %R", &msg->addr);
  if (NSM_CHECK_CTYPE (msg->cindex, NSM_ROUTE_LOOKUP_CTYPE_PREFIXLEN))
    zlog_info (zg, " Prefixlen: %d", msg->prefixlen);
}
#endif /* HAVE_IPV6 */

void
nsm_vr_dump (struct lib_globals *zg, struct nsm_msg_vr *msg)
{
  zlog_info (zg, " VR ID: %d", msg->vr_id);
  if (msg->len)
    zlog_info (zg, " VR Name: %s", msg->name);
}

void
nsm_vrf_dump (struct lib_globals *zg, struct nsm_msg_vrf *msg)
{
  zlog_info (zg, " VRF ID: %d", msg->vrf_id);

  if (NSM_CHECK_CTYPE (msg->cindex, NSM_VRF_CTYPE_NAME))
    zlog_info (zg, " VRF Name: %s", msg->name);
}

void
nsm_vr_bind_dump (struct lib_globals *zg, struct nsm_msg_vr_bind *msg)
{
  zlog_info (zg, " VR ID: %d", msg->vr_id);
  zlog_info (zg, " Interface Index: %d", msg->ifindex);
}

void
nsm_vrf_bind_dump (struct lib_globals *zg, struct nsm_msg_vrf_bind *msg)
{
  zlog_info (zg, " VRF ID: %d", msg->vrf_id);
  zlog_info (zg, " Interface Index: %d", msg->ifindex);
}

void
nsm_route_manage_dump (struct lib_globals *zg,
                       struct nsm_msg_route_manage *msg)
{
  if (NSM_CHECK_CTYPE (msg->cindex, NSM_ROUTE_MANAGE_CTYPE_RESTART_TIME))
    zlog_info (zg, " Restart Time: %d", msg->restart_time);
  if (NSM_CHECK_CTYPE (msg->cindex, NSM_ROUTE_MANAGE_CTYPE_AFI_SAFI))
    {
      zlog_info (zg, " AFI: %d", msg->afi);
      zlog_info (zg, " SAFI: %d", msg->safi);
    }
  if (NSM_CHECK_CTYPE (msg->cindex, NSM_ROUTE_MANAGE_CTYPE_PROTOCOL_ID))
    zlog_info (zg, " Protocol ID: %s (%d)", modname_strl (msg->protocol_id),
               msg->protocol_id);

#if defined HAVE_RESTART && defined HAVE_MPLS
  if (NSM_CHECK_CTYPE (msg->cindex, NSM_ROUTE_MANAGE_CTYPE_MPLS_FLAGS))
    {
      zlog_info (zg, " NSM MPLS Force stale Clean Up :");
      zlog_info (zg, "   Stale cleanup %s", (CHECK_FLAG (msg->flags,
                 NSM_MSG_ILM_FORCE_STALE_DEL) ? "ILM Del" : "Unknown"));
    }
#endif /* HAVE_RESTART  && HAVE_MPLS */

}

void
nsm_router_id_dump (struct lib_globals *zg,
                    struct nsm_msg_router_id *msg)
{
  zlog_info (zg, "Router ID:");
  zlog_info (zg, " ID: %r", &msg->router_id);
}

void
nsm_route_clean_dump (struct lib_globals *zg,
                      struct nsm_msg_route_clean *msg)
{
  zlog_info (zg, "Route Clean:");
  zlog_info (zg, " AFI: %d", msg->afi);
  zlog_info (zg, " SAFI: %d", msg->safi);
  zlog_info (zg, " Route type mask: %x", msg->route_type_mask);
}

void
nsm_protocol_restart_dump (struct lib_globals *zg,
                           struct nsm_msg_protocol_restart *msg)
{
  zlog_info (zg, "Protocol Restarting:");
  zlog_info (zg, " Protocol: %s(%d)", modname_strl (msg->protocol_id),
             msg->protocol_id);
  zlog_info (zg, " Protocol disconnect time %d", msg->disconnect_time);
}

#ifndef HAVE_IMI
void
nsm_user_dump (struct lib_globals *zg, struct nsm_msg_user *msg, char *op)
{
  zlog_info (zg, "User %s", op);

  zlog_info (zg, " Privilege: %d", msg->privilege);
  zlog_info (zg, " Username: %s", msg->username);
#ifdef HAVE_DEV_TEST
  if (msg->password)
    zlog_info (zg, " Password: %s", msg->password);
  if (msg->password_encrypt)
    zlog_info (zg, " Password: %s", msg->password_encrypt);
#endif /* HAVE_DEV_TEST */
}
#endif /* HAVE_IMI */


#ifdef HAVE_MPLS
void
nsm_label_pool_dump (struct lib_globals *zg,
                     struct nsm_msg_label_pool *msg)
{
  int i;
  int bt;

  zlog_info (zg, "NSM Label Pool");
  zlog_info (zg, " Label Space: %d", msg->label_space);

  if (NSM_CHECK_CTYPE (msg->cindex, NSM_LABEL_POOL_CTYPE_LABEL_BLOCK))
    zlog_info (zg, " Label Block: %d", msg->label_block);

  if (NSM_CHECK_CTYPE (msg->cindex, NSM_LABEL_POOL_CTYPE_LABEL_RANGE))
    {
      zlog_info (zg, " Min Label: %d", msg->min_label);
      zlog_info (zg, " Max Label: %d", msg->max_label);
    }
  if (NSM_CHECK_CTYPE (msg->cindex, NSM_LABEL_POOL_CTYPE_BLOCK_LIST))
    {
      for (bt = 0; bt < 2; bt++)
        {
          zlog_info (zg, " Action type: %d", msg->blk_list[bt].action_type);
          zlog_info (zg, " Blk re type: %d", msg->blk_list[bt].blk_req_type);
          zlog_info (zg, " Blk count : %d", msg->blk_list[bt].blk_count);

          for (i = 0; i < msg->blk_list[bt].blk_count; i++)
            {
              zlog_info (zg, " blk id : %d", msg->blk_list[bt].lset_blocks[i]);
            }
        }
    }
}


void
nsm_mpls_owner_dump (struct lib_globals *zg, struct mpls_owner *owner)
{
  zlog_info (zg, " Owner: %s", mpls_dump_owner (owner->owner));
  switch (owner->owner)
    {
    case MPLS_RSVP:
      if (owner->u.r_key.afi == AFI_IP)
        {
          zlog_info (zg, "  Trunk ID: %d", owner->u.r_key.u.ipv4.trunk_id);
          zlog_info (zg, "  LSP ID: %d", owner->u.r_key.u.ipv4.lsp_id);
          if (owner->u.r_key.u.ipv4.ingr.s_addr)
            zlog_info (zg, "  Ingress: %r", &owner->u.r_key.u.ipv4.ingr);
          else
            zlog_info (zg, "  Ingress: -");
          if (owner->u.r_key.u.ipv4.egr.s_addr)
            zlog_info (zg, "  Egress: %r", &owner->u.r_key.u.ipv4.egr);
          else
            zlog_info (zg, "  Egress: -");
        }
#ifdef HAVE_IPV6
      else if (owner->u.r_key.afi == AFI_IP6)
        {
          zlog_info (zg, "  Trunk ID: %d", owner->u.r_key.u.ipv6.trunk_id);
          zlog_info (zg, "  LSP ID: %d", owner->u.r_key.u.ipv6.lsp_id);
          if (! IN6_IS_ADDR_UNSPECIFIED (&owner->u.r_key.u.ipv6.ingr))
            zlog_info (zg, "  Ingress: %R", &owner->u.r_key.u.ipv6.ingr);
          else
            zlog_info (zg, "  Ingress: -");
          if (! IN6_IS_ADDR_UNSPECIFIED (&owner->u.r_key.u.ipv6.egr))
            zlog_info (zg, "  Egress: %R", &owner->u.r_key.u.ipv6.egr);
          else
            zlog_info (zg, "  Egress: -");
        }
#endif /* HAVE_IPV6 */
      break;
    case MPLS_LDP:
      if (owner->u.l_key.afi == AFI_IP)
        {
          zlog_info (zg, "  Address: %r", &owner->u.l_key.u.ipv4.addr);
          zlog_info (zg, "  LSR ID: %d", owner->u.l_key.u.ipv4.lsr_id);
          zlog_info (zg, "  Label Space: %d", owner->u.l_key.u.ipv4.label_space);        }
#ifdef HAVE_IPV6
      else if (owner->u.l_key.afi == AFI_IP6)
        {
          zlog_info (zg, "  Address: %R", &owner->u.l_key.u.ipv6.addr);
          zlog_info (zg, "  LSR ID: %d", owner->u.l_key.u.ipv6.lsr_id);
          zlog_info (zg, "  Label Space: %d", owner->u.l_key.u.ipv6.label_space);        }
#endif /* HAVE_IPV6 */
      break;
    case MPLS_OTHER_BGP:
      if (owner->u.b_key.afi == AFI_IP)
        {
          zlog_info (zg, "  Address: %r", &owner->u.b_key.u.ipv4.peer);
          zlog_info (zg, "  Prefix: %r/%d", &owner->u.b_key.u.ipv4.p.prefix,
                     owner->u.b_key.u.ipv4.p.prefixlen);
        }
#ifdef HAVE_IPV6
      else if (owner->u.b_key.afi == AFI_IP6)
        {
          zlog_info (zg, "  Address: %R", &owner->u.b_key.u.ipv6.peer);
          zlog_info (zg, "  Prefix: %R/%d", &owner->u.b_key.u.ipv6.p.prefix,
                     owner->u.b_key.u.ipv6.p.prefixlen);
        }
#endif /* HAVE_IPV6 */
      zlog_info (zg, "  VRF Id: %d", owner->u.b_key.vrf_id);
      break;
#ifdef HAVE_MPLS_VC
    case MPLS_OTHER_LDP_VC:
      zlog_info (zg, "  VC id: %d", owner->u.vc_key.vc_id);
      break;
#endif /* HAVE_MPLS_VC */
    default:
      zlog_info (zg, "  Skip detailed information");
      break;
    }
}

void
nsm_ilm_lookup_gen_dump (struct lib_globals *zg,
                         struct nsm_msg_ilm_gen_lookup *msg)
{
  zlog_info (zg, "NSM ILM lookup");
  zlog_info (zg, " Owner: %d", msg->owner);
  zlog_info (zg, " Opcode: %d", msg->opcode);
  zlog_info (zg, " ILM IN IX: %d", msg->ilm_in_ix);
  if (msg->ilm_in_label.type == gmpls_entry_type_ip)
    {
      zlog_info (zg, " ILM IN Label: %d", msg->ilm_in_label.u.pkt);
    }

  zlog_info (zg, " ILM OUT IX: %d", msg->ilm_out_ix);
  if (msg->ilm_out_label.type == gmpls_entry_type_ip)
    {
      zlog_info (zg, " ILM OUT Label: %d", msg->ilm_out_label.u.pkt);
    }
  zlog_info (zg, " ILM IX: %d", msg->ilm_ix);
}

#ifdef HAVE_PACKET
void
nsm_ilm_lookup_dump (struct lib_globals *zg, struct nsm_msg_ilm_lookup *msg)
{
  zlog_info (zg, "NSM ILM lookup");
  zlog_info (zg, " Owner: %d", msg->owner);
  zlog_info (zg, " Opcode: %d", msg->opcode);
  zlog_info (zg, " ILM IN IX: %d", msg->ilm_in_ix);
  zlog_info (zg, " ILM IN Label: %d", msg->ilm_in_label);
  zlog_info (zg, " ILM OUT IX: %d", msg->ilm_out_ix);
  zlog_info (zg, " ILM OUT Label: %d", msg->ilm_out_label);
  zlog_info (zg, " ILM IX: %d", msg->ilm_ix);
}
#endif /* HAVE_PACKET */

char *
nsm_igp_shortcut_lsp_action_to_str (u_int16_t action)
{
  switch (action)
    {
      case NSM_MSG_IGP_SHORTCUT_ADD:
        return "ADD";
      case NSM_MSG_IGP_SHORTCUT_DELETE:
        return "Delete";
      case NSM_MSG_IGP_SHORTCUT_UPDATE:
        return "Update";
      default:
        return "Unknown";
    }
}

void
nsm_igp_shortcut_lsp_dump (struct lib_globals *zg,
                           struct nsm_msg_igp_shortcut_lsp *msg)
{
  zlog_info (zg, " NSM IGP-Shortcut LSP %s",
                  nsm_igp_shortcut_lsp_action_to_str (msg->action));
  zlog_info (zg, " LSP-Metric: %d", msg->metric);
  zlog_info (zg, " Tunnel-id: %d", msg->tunnel_id);
  zlog_info (zg, " Tunnel end point address: %r", &msg->t_endp);
}

void
nsm_igp_shortcut_route_dump (struct lib_globals *zg,
                             struct nsm_msg_igp_shortcut_route *msg)
{
  zlog_info (zg, " NSM IGP-Shortcut ROUTE %s",
                  nsm_igp_shortcut_lsp_action_to_str (msg->action));

  zlog_info (zg, " Tunnel-id %d", msg->tunnel_id);
  zlog_info (zg, " FEC: %O", &msg->fec);
  zlog_info (zg, " Tunnel end point address: %r", &msg->t_endp);
}

#ifdef HAVE_RESTART
void
nsm_mpls_stale_info_dump (struct lib_globals *zg,
                          struct nsm_msg_stale_info *msg)
{
   zlog_info (zg, " NSM MPLS STALE INFO:");
#ifdef HAVE_IPV6
  if (msg->fec_prefix.family == AF_INET6)
    zlog_info (zg, " Prefix % R", &msg->fec_prefix.u.prefix6);
  else
#endif /* HAVE_IPV6 */
    zlog_info (zg, " Prefix %r", &msg->fec_prefix.u.prefix4);

  zlog_info (zg, " In Label %d", msg->in_label);
  zlog_info (zg, " In Interface Index %d", msg->iif_ix);
  zlog_info (zg, " Out Label %d", msg->out_label);
  zlog_info (zg, " Out Interface Index %d", msg->oif_ix);
  zlog_info (zg, " ILM index %d", msg->ilm_ix);
}
#endif /* HAVE_RESTART */
#endif /* HAVE_MPLS */


#ifdef HAVE_TE
void
nsm_qos_clean_dump (struct lib_globals *zg, struct nsm_msg_qos_clean *msg)
{
  zlog_info (zg, "NSM QoS cleanup");
  zlog_info (zg, " Protocol ID: %s", modname_strl (msg->protocol_id));

  if (NSM_CHECK_CTYPE (msg->cindex, NSM_QOS_CLEAN_CTYPE_IFINDEX))
    zlog_info (zg, " Ifindex: %d", msg->ifindex);
}

void
nsm_qos_release_dump (struct lib_globals *zg,
                      struct nsm_msg_qos_release *msg)
{
  zlog_info (zg, "NSM QoS release");
  zlog_info (zg, " Protocol ID: %s", modname_strl (msg->protocol_id));
  zlog_info (zg, " Resource ID: %d", msg->resource_id);

  if (NSM_CHECK_CTYPE (msg->cindex, NSM_QOS_RELEASE_CTYPE_IFINDEX))
    zlog_info (zg, " Ifindex: %d", msg->ifindex);
  if (NSM_CHECK_CTYPE (msg->cindex, NSM_QOS_RELEASE_CTYPE_STATUS))
    zlog_info (zg, " Ifindex: %d", msg->status);
}

void
nsm_qos_preempt_dump (struct lib_globals *zg, struct nsm_msg_qos_preempt *msg)
{
  int i;

  zlog_info (zg, "NSM QoS Preempt");
  zlog_info (zg, " Protocol ID: %s", modname_strl (msg->protocol_id));
  zlog_info (zg, " # of Preempted LSPs: %d", msg->count);
  for (i = 0; i < msg->count; i++)
    zlog_info (zg, "  Preempted LSP # %d has id: %d", i + 1,
               msg->preempted_ids[i]);
}


void
nsm_qos_dump (struct lib_globals *zg,
              struct nsm_msg_qos *msg,
              int cmd)
{
  struct nsm_msg_qos_t_spec t_spec;
  struct nsm_msg_qos_if_spec if_spec;
  struct nsm_msg_qos_ad_spec ad_spec;

  switch (cmd)
    {
    case NSM_MSG_QOS_CLIENT_PROBE:
      zlog_info (zg, "NSM QoS Probe");
      break;
    case NSM_MSG_QOS_CLIENT_PROBE_RELEASE:
      zlog_info (zg, "NSM QoS Probe Release");
      break;
    case NSM_MSG_QOS_CLIENT_RESERVE:
      zlog_info (zg, "NSM QoS Reserve");
      break;
    case NSM_MSG_QOS_CLIENT_PREEMPT:
      zlog_info (zg, "NSM QoS Preempt");
      break;
    case NSM_MSG_QOS_CLIENT_MODIFY:
      zlog_info (zg, "NSM QoS Modify");
      break;
    case NSM_MSG_QOS_CLIENT_RELEASE_SLOW:
      zlog_info (zg, "NSM QoS Slow Release");
      break;
    }

  zlog_info (zg, " Protocol ID: %s", modname_strl (msg->protocol_id));
  zlog_info (zg, " Resource ID: %d", msg->resource_id);
  zlog_info (zg, " Message ID: %d", msg->id);

  /* Dump Owner. */
  nsm_mpls_owner_dump (zg, &msg->owner);

  /* Class Type dump. */
  zlog_info (zg, " Class Type number: %d", msg->ct_num);

  if (NSM_CHECK_CTYPE (msg->cindex, NSM_QOS_CTYPE_SETUP_PRIORITY))
    zlog_info (zg, " Setup priority: %d", msg->setup_priority);
  if (NSM_CHECK_CTYPE (msg->cindex, NSM_QOS_CTYPE_HOLD_PRIORITY))
    zlog_info (zg, " Hold priority: %d", msg->hold_priority);
  if (NSM_CHECK_CTYPE (msg->cindex, NSM_QOS_CTYPE_STATUS))
    zlog_info (zg, " Status: %d", msg->status);

  if (NSM_CHECK_CTYPE (msg->cindex, NSM_QOS_CTYPE_T_SPEC))
    {
      t_spec = msg->t_spec;

      zlog_info (zg, " Traffic Specification");
      zlog_info (zg, "  Service Type: %d", t_spec.service_type);
      zlog_info (zg, "  Exclusive? : %d", t_spec.is_exclusive);

      if (NSM_CHECK_CTYPE (t_spec.cindex, NSM_QOS_CTYPE_T_SPEC_MIN_POLICED_UNIT))
        zlog_info (zg, "   Min policed unit: %d", t_spec.min_policed_unit);
      if (NSM_CHECK_CTYPE (t_spec.cindex, NSM_QOS_CTYPE_T_SPEC_MAX_PACKET_SIZE))
        zlog_info (zg, "   Max packet size: %d", t_spec.max_packet_size);
      if (NSM_CHECK_CTYPE (t_spec.cindex, NSM_QOS_CTYPE_T_SPEC_PEAK_RATE))
        zlog_info (zg, "   Peak rate: %f", t_spec.peak_rate);
      if (NSM_CHECK_CTYPE (t_spec.cindex, NSM_QOS_CTYPE_T_SPEC_QOS_COMMITED_BUCKET))
        {
          zlog_info (zg, "   Committed bucket rate: %f", t_spec.rate);
          zlog_info (zg, "   Committed bucket size: %f", t_spec.size);
        }
      if (NSM_CHECK_CTYPE (t_spec.cindex, NSM_QOS_CTYPE_T_SPEC_EXCESS_BURST_SIZE))
        zlog_info (zg, "   Excess burst: %f", t_spec.excess_burst);
      if (NSM_CHECK_CTYPE (t_spec.cindex, NSM_QOS_CTYPE_T_SPEC_WEIGHT))
        zlog_info (zg, "   Weight: %f", t_spec.weight);
    }

  if (NSM_CHECK_CTYPE (msg->cindex, NSM_QOS_CTYPE_IF_SPEC))
    {
      if_spec = msg->if_spec;

      zlog_info (zg, " I/F Specification");

      if (NSM_CHECK_CTYPE (if_spec.cindex, NSM_QOS_CTYPE_IF_SPEC_IFINDEX))
        {
          zlog_info (zg, "   In ifindex: %d", if_spec.ifindex.in);
          zlog_info (zg, "   Out ifindex: %d", if_spec.ifindex.out);
        }
      if (NSM_CHECK_CTYPE (if_spec.cindex, NSM_QOS_CTYPE_IF_SPEC_ADDR))
        {
          zlog_info (zg, "   Previous hop: %r", &if_spec.addr.prev_hop);
          zlog_info (zg, "   Next hop: %r", &if_spec.addr.next_hop);
        }
    }

  if (NSM_CHECK_CTYPE (msg->cindex, NSM_QOS_CTYPE_AD_SPEC))
    {
      ad_spec = msg->ad_spec;

      zlog_info (zg, " AD Specification");

      if (NSM_CHECK_CTYPE (ad_spec.cindex, NSM_QOS_CTYPE_AD_SPEC_SLACK))
        zlog_info (zg, "   Slack: %f", ad_spec.slack);
      if (NSM_CHECK_CTYPE (ad_spec.cindex, NSM_QOS_CTYPE_AD_SPEC_COMPOSE_TOTAL))
        zlog_info (zg, "   Compose total: %f", ad_spec.compose_total);
      if (NSM_CHECK_CTYPE (ad_spec.cindex, NSM_QOS_CTYPE_AD_SPEC_DERIVED_TOTAL))
        zlog_info (zg, "   Derived total: %f", ad_spec.derived_total);
      if (NSM_CHECK_CTYPE (ad_spec.cindex, NSM_QOS_CTYPE_AD_SPEC_COMPOSE_SUM))
        zlog_info (zg, "   Compose sum: %f", ad_spec.derived_sum);
      if (NSM_CHECK_CTYPE (ad_spec.cindex, NSM_QOS_CTYPE_AD_SPEC_MIN_PATH_LATENCY))
        zlog_info (zg, "   Min path latency: %f", ad_spec.min_path_latency);
      if (NSM_CHECK_CTYPE (ad_spec.cindex, NSM_QOS_CTYPE_AD_SPEC_PATH_BW_EST))
        zlog_info (zg, "   Path b/w estimate: %f", ad_spec.path_bw_est);
    }
}
#ifdef HAVE_GMPLS
void
nsm_gmpls_qos_clean_dump (struct lib_globals *zg, struct nsm_msg_gmpls_qos_clean *msg)
{
  zlog_info (zg, "NSM QoS cleanup");
  zlog_info (zg, " Protocol ID: %s", modname_strl (msg->protocol_id));

  if (NSM_CHECK_CTYPE (msg->cindex, NSM_GMPLS_QOS_CLEAN_CTYPE_TL_GIFINDEX))
    zlog_info (zg, " TE Link gIfindex: %d", msg->tel_gifindex);
  if (NSM_CHECK_CTYPE (msg->cindex, NSM_GMPLS_QOS_CLEAN_CTYPE_DL_GIFINDEX))
    zlog_info (zg, " Data Link gIfindex: %d", msg->dl_gifindex);
}

void
nsm_gmpls_qos_release_dump (struct lib_globals *zg,
                            struct nsm_msg_gmpls_qos_release *msg)
{
  zlog_info (zg, "NSM QoS release");
  zlog_info (zg, " Protocol ID: %s", modname_strl (msg->protocol_id));
  zlog_info (zg, " Resource ID: %d", msg->resource_id);

  if (NSM_CHECK_CTYPE (msg->cindex, NSM_GMPLS_QOS_RELEASE_CTYPE_TL_GIFINDEX))
    zlog_info (zg, " TE Link gIfindex: %d", msg->tel_gifindex);
  if (NSM_CHECK_CTYPE (msg->cindex, NSM_GMPLS_QOS_RELEASE_CTYPE_DL_GIFINDEX))
    zlog_info (zg, " Data Link gIfindex: %d", msg->dl_gifindex);
  if (NSM_CHECK_CTYPE (msg->cindex, NSM_QOS_RELEASE_CTYPE_STATUS))
    zlog_info (zg, " Status: %d", msg->status);
}

void
nsm_gmpls_qos_dump (struct lib_globals *zg,
                    struct nsm_msg_gmpls_qos *msg,
                    int cmd)
{
  struct nsm_msg_qos_t_spec t_spec;
  struct nsm_msg_qos_ad_spec ad_spec;
  struct nsm_msg_gmpls_qos_if_spec gif_spec;
  struct nsm_msg_gmpls_qos_attr gmpls_attr;
  struct nsm_msg_gmpls_qos_label_spec label_spec;

  switch (cmd)
    {
    case NSM_MSG_QOS_CLIENT_PROBE:
      zlog_info (zg, "NSM QoS Probe");
      break;
    case NSM_MSG_QOS_CLIENT_PROBE_RELEASE:
      zlog_info (zg, "NSM QoS Probe Release");
      break;
    case NSM_MSG_QOS_CLIENT_RESERVE:
      zlog_info (zg, "NSM QoS Reserve");
      break;
    case NSM_MSG_QOS_CLIENT_PREEMPT:
      zlog_info (zg, "NSM QoS Preempt");
      break;
    case NSM_MSG_QOS_CLIENT_MODIFY:
      zlog_info (zg, "NSM QoS Modify");
      break;
    case NSM_MSG_QOS_CLIENT_RELEASE_SLOW:
      zlog_info (zg, "NSM QoS Slow Release");
      break;
    }

  zlog_info (zg, " Protocol ID: %s", modname_strl (msg->protocol_id));
  zlog_info (zg, " Resource ID: %d", msg->resource_id);
  zlog_info (zg, " Message ID: %d", msg->id);

  /* Dump Owner. */
  nsm_mpls_owner_dump (zg, &msg->owner);

  /* Class Type dump. */
  zlog_info (zg, " Class Type number: %d", msg->ct_num);

  if (NSM_CHECK_CTYPE (msg->cindex, NSM_QOS_CTYPE_SETUP_PRIORITY))
    zlog_info (zg, " Setup priority: %d", msg->setup_priority);
  if (NSM_CHECK_CTYPE (msg->cindex, NSM_QOS_CTYPE_HOLD_PRIORITY))
    zlog_info (zg, " Hold priority: %d", msg->hold_priority);
  if (NSM_CHECK_CTYPE (msg->cindex, NSM_QOS_CTYPE_STATUS))
    zlog_info (zg, " Status: %d", msg->status);

  if (NSM_CHECK_CTYPE (msg->cindex, NSM_QOS_CTYPE_T_SPEC))
    {
      t_spec = msg->t_spec;

      zlog_info (zg, " Traffic Specification");
      zlog_info (zg, "  Service Type: %d", t_spec.service_type);
      zlog_info (zg, "  Exclusive? : %d", t_spec.is_exclusive);

      if (NSM_CHECK_CTYPE (t_spec.cindex, NSM_QOS_CTYPE_T_SPEC_MIN_POLICED_UNIT))
        zlog_info (zg, "   Min policed unit: %d", t_spec.min_policed_unit);
      if (NSM_CHECK_CTYPE (t_spec.cindex, NSM_QOS_CTYPE_T_SPEC_MAX_PACKET_SIZE))
        zlog_info (zg, "   Max packet size: %d", t_spec.max_packet_size);
      if (NSM_CHECK_CTYPE (t_spec.cindex, NSM_QOS_CTYPE_T_SPEC_PEAK_RATE))
        zlog_info (zg, "   Peak rate: %f", t_spec.peak_rate);
      if (NSM_CHECK_CTYPE (t_spec.cindex, NSM_QOS_CTYPE_T_SPEC_QOS_COMMITED_BUCKET))
        {
          zlog_info (zg, "   Committed bucket rate: %f", t_spec.rate);
          zlog_info (zg, "   Committed bucket size: %f", t_spec.size);
        }
      if (NSM_CHECK_CTYPE (t_spec.cindex, NSM_QOS_CTYPE_T_SPEC_EXCESS_BURST_SIZE))
        zlog_info (zg, "   Excess burst: %f", t_spec.excess_burst);
      if (NSM_CHECK_CTYPE (t_spec.cindex, NSM_QOS_CTYPE_T_SPEC_WEIGHT))
        zlog_info (zg, "   Weight: %f", t_spec.weight);
    }

  if (NSM_CHECK_CTYPE (msg->cindex, NSM_QOS_CTYPE_AD_SPEC))
    {
      ad_spec = msg->ad_spec;

      zlog_info (zg, " AD Specification");

      if (NSM_CHECK_CTYPE (ad_spec.cindex, NSM_QOS_CTYPE_AD_SPEC_SLACK))
        zlog_info (zg, "   Slack: %f", ad_spec.slack);
      if (NSM_CHECK_CTYPE (ad_spec.cindex, NSM_QOS_CTYPE_AD_SPEC_COMPOSE_TOTAL))
        zlog_info (zg, "   Compose total: %f", ad_spec.compose_total);
      if (NSM_CHECK_CTYPE (ad_spec.cindex, NSM_QOS_CTYPE_AD_SPEC_DERIVED_TOTAL))
        zlog_info (zg, "   Derived total: %f", ad_spec.derived_total);
      if (NSM_CHECK_CTYPE (ad_spec.cindex, NSM_QOS_CTYPE_AD_SPEC_COMPOSE_SUM))
        zlog_info (zg, "   Compose sum: %f", ad_spec.derived_sum);
      if (NSM_CHECK_CTYPE (ad_spec.cindex, NSM_QOS_CTYPE_AD_SPEC_MIN_PATH_LATENCY))
        zlog_info (zg, "   Min path latency: %f", ad_spec.min_path_latency);
      if (NSM_CHECK_CTYPE (ad_spec.cindex, NSM_QOS_CTYPE_AD_SPEC_PATH_BW_EST))
        zlog_info (zg, "   Path b/w estimate: %f", ad_spec.path_bw_est);
    }

  if (NSM_CHECK_CTYPE (msg->cindex, NSM_QOS_CTYPE_GMPLS_IF_SPEC))
    {
      gif_spec = msg->gif_spec;

      zlog_info (zg, " GMPLS I/F Specification");

      if (NSM_CHECK_CTYPE (gif_spec.cindex, NSM_QOS_CTYPE_GMPLS_IF_SPEC_TL_GIFINDEX))
        zlog_info (zg, " TeLink gifindex: %d", gif_spec.tel_gifindex);
      if (NSM_CHECK_CTYPE (gif_spec.cindex, NSM_QOS_CTYPE_GMPLS_IF_SPEC_DL_GIFINDEX))
        zlog_info (zg, " DataLink gifindex: %d", gif_spec.dl_gifindex);
    }

  if (NSM_CHECK_CTYPE (msg->cindex, NSM_QOS_CTYPE_GMPLS_ATTR))
    {
      gmpls_attr = msg->gmpls_attr;

      zlog_info (zg, " GMPLS Attributes");

      if (NSM_CHECK_CTYPE (gmpls_attr.cindex, NSM_QOS_CTYPE_GMPLS_ATTR_SW_CAP))
        {
          zlog_info (zg, " Switching Capability: %d", gmpls_attr.sw_cap);
          zlog_info (zg, " Encoding Type: %d", gmpls_attr.encoding);
        }
      if (NSM_CHECK_CTYPE (gmpls_attr.cindex, NSM_QOS_CTYPE_GMPLS_ATTR_PROTECTION))
        zlog_info (zg, " Protection: %d", gmpls_attr.protection);
      if (NSM_CHECK_CTYPE (gmpls_attr.cindex, NSM_QOS_CTYPE_GMPLS_ATTR_GPID))
        zlog_info (zg, " GPID: %d", gmpls_attr.gpid);
    }

  if (NSM_CHECK_CTYPE (msg->cindex, NSM_QOS_CTYPE_GMPLS_LABEL_SPEC))
    {
      label_spec = msg->label_spec;

      zlog_info (zg, " GMPLS Label Specification");

      if (NSM_CHECK_CTYPE (label_spec.cindex, NSM_QOS_CTYPE_GMPLS_LABEL_SPEC_LABEL_SET))
        {
          zlog_info (zg, " Label Set From: %d", label_spec.sl_lbl_set_from);
          zlog_info (zg, " Label Set To: %d", label_spec.sl_lbl_set_to);
        }
      if (NSM_CHECK_CTYPE (label_spec.cindex, NSM_QOS_CTYPE_GMPLS_LABEL_SPEC_SUGG_LABEL))
        {
          zlog_info (zg, " Suggested Label Type: %d", label_spec.sugg_label.type);
          if (label_spec.sugg_label.type == gmpls_entry_type_ip)
            zlog_info (zg, " Suggested Label: %d", label_spec.sugg_label.u.pkt);
        }
    }
}
#endif /*HAVE_GMPLS*/
#endif /* HAVE_TE */

#ifdef HAVE_MPLS_VC
void
nsm_mpls_pw_status_dump (struct lib_globals *zg, u_int32_t pw_status)
{
  if (pw_status == NSM_CODE_PW_FORWARDING)
    {
      zlog_info (zg, "  Forwarding");
      return;
    }

  if (CHECK_FLAG (pw_status, NSM_CODE_PW_NOT_FORWARDING))
    zlog_info (zg, "  Not Forwarding");
  if (CHECK_FLAG (pw_status, NSM_CODE_PW_AC_INGRESS_RX_FAULT))
    zlog_info (zg, "  AC Ingress RX Fault");
  if (CHECK_FLAG (pw_status, NSM_CODE_PW_AC_EGRESS_TX_FAULT))
    zlog_info (zg, "  AC Engress TX Fault");
  if (CHECK_FLAG (pw_status, NSM_CODE_PW_PSN_INGRESS_RX_FAULT))
    zlog_info (zg, "  PSN Ingress RX Fault");
  if (CHECK_FLAG (pw_status, NSM_CODE_PW_PSN_EGRESS_TX_FAULT))
    zlog_info (zg, "  PSN Egress TX Fault");
  if (CHECK_FLAG (pw_status, NSM_CODE_PW_STANDBY))
    zlog_info (zg, "  PW Standby");
  if (CHECK_FLAG (pw_status, NSM_CODE_PW_REQUEST_SWITCHOVER))
    zlog_info (zg, "  PW Request Switchover");
}

void
nsm_mpls_pw_status_msg_dump (struct lib_globals *zg,
                             struct nsm_msg_pw_status *msg)
{
  zlog_info (zg, " VPN ID %d", msg->vpn_id);
  zlog_info (zg, " VC ID %d", msg->vc_id);
  nsm_mpls_pw_status_dump (zg, msg->pw_status);
}

void
nsm_mpls_vc_dump (struct lib_globals *zg, struct nsm_msg_mpls_vc *msg)
{
  zlog_info (zg, "Virtual Circuit");
  zlog_info (zg, " VC id: %d", msg->vc_id);
  if (NSM_CHECK_CTYPE (msg->cindex, NSM_MPLS_VC_CTYPE_NEIGHBOR))
    {
      if (msg->afi == AFI_IP)
        zlog_info (zg, " Neighbor: %r", &msg->nbr_addr_ipv4);
#ifdef HAVE_IPV6
      else if (msg->afi == AFI_IP6)
        zlog_info (zg, " Neighbor: %R", &msg->nbr_addr_ipv6);
#endif /* HAVE_IPV6 */
    }
  if (NSM_CHECK_CTYPE (msg->cindex, NSM_MPLS_VC_CTYPE_CONTROL_WORD))
    zlog_info (zg, " Control word: %d", msg->c_word);
  if (NSM_CHECK_CTYPE (msg->cindex, NSM_MPLS_VC_CTYPE_GROUP_ID))
    zlog_info (zg, " Group ID: %u", msg->grp_id);
  if (NSM_CHECK_CTYPE (msg->cindex, NSM_MPLS_VC_CTYPE_IF_INDEX))
    zlog_info (zg, " Ifindex: %d", msg->ifindex);
#ifdef HAVE_VPLS
  if (NSM_CHECK_CTYPE (msg->cindex, NSM_MPLS_VC_CTYPE_VPLS_ID))
    zlog_info (zg, " VPLS Id: %u", msg->vpls_id);
#endif /* HAVE_VPLS */
  if (NSM_CHECK_CTYPE (msg->cindex, NSM_MPLS_VC_CTYPE_IF_MTU))
    zlog_info (zg, " Interface MTU: %hu", msg->ifmtu);
  if (NSM_CHECK_CTYPE (msg->cindex, NSM_MPLS_VC_CTYPE_VC_TYPE))
    zlog_info (zg, " VC Type: %s", mpls_vc_type_to_str (msg->vc_type));
  if (NSM_CHECK_CTYPE (msg->cindex, NSM_MPLS_VC_CTYPE_VLAN_ID))
    zlog_info (zg, " Vlan ID: %d", msg->vlan_id);
  if (NSM_CHECK_CTYPE (msg->cindex, NSM_MPLS_VC_CTYPE_PW_STATUS))
    {
      zlog_info (zg, " PW Status: ");
      nsm_mpls_pw_status_dump (zg, msg->pw_status);
    }
}
#endif /* HAVE_MPLS_VC */

#ifdef HAVE_VPLS
void
nsm_vpls_mac_withdraw_dump (struct lib_globals *zg,
                            struct nsm_msg_vpls_mac_withdraw *msg)
{
  u_char *mac_addr;
  int i;

  if (! msg)
    return;

  zlog_info (zg, "VPLS MAC Withdraw Message");
  zlog_info (zg, " VPLS ID: %u", msg->vpls_id);
  if (msg->num == 0)
    {
      zlog_info (zg, " MAC Addresses to be withdrawn: all");
      return;
    }

  zlog_info (zg, " Number of MAC Addresses to be withdrawn: %d", msg->num);
  mac_addr = msg->mac_addrs;
  for (i = 1; i <= msg->num; i++)
    {
      zlog_info (zg, " MAC Address # %d: %02x:%02x:%02x:%02x:%02x:%02x",
                 i, mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3],
                 mac_addr[4], mac_addr[5]);
      mac_addr += VPLS_MAC_ADDR_LEN;
    }
}

void
nsm_vpls_add_dump (struct lib_globals *zg, struct nsm_msg_vpls_add *msg)
{
  int ctr;

  zlog_info (zg, "Virtual Private Lan Service");
  zlog_info (zg, " VPLS id: %u", msg->vpls_id);

  if (NSM_CHECK_CTYPE (msg->cindex, NSM_VPLS_CTYPE_VPLS_TYPE))
    zlog_info (zg, " VC Type: %s", mpls_vc_type_to_str (msg->vpls_type));

  if (NSM_CHECK_CTYPE (msg->cindex, NSM_VPLS_CTYPE_GROUP_ID))
    zlog_info (zg, " Group ID: %u", msg->grp_id);

  if (NSM_CHECK_CTYPE (msg->cindex, NSM_VPLS_CTYPE_CONTROL_WORD))
    zlog_info (zg, " Control word: %d", msg->c_word);

  if (NSM_CHECK_CTYPE (msg->cindex, NSM_MPLS_VC_CTYPE_IF_MTU))
    zlog_info (zg, " Interface MTU: %d", msg->ifmtu);

  if (NSM_CHECK_CTYPE (msg->cindex, NSM_VPLS_CTYPE_PEERS_ADDED))
    {
      for (ctr = 0; ctr < msg->add_count; ctr++)
        {
          if (msg->peers_added[ctr].afi == AFI_IP)
            zlog_info (zg, " Peer: %r", &msg->peers_added[ctr].u.ipv4);
#ifdef HAVE_IPV6
          else if (msg->peers_added[ctr].afi == AFI_IP6)
            zlog_info (zg, " Peer: %R", &msg->peers_added[ctr].u.ipv6);
#endif /* HAVE_IPV6 */
        }
    }

  if (NSM_CHECK_CTYPE (msg->cindex, NSM_VPLS_CTYPE_PEERS_DELETED))
    {
      for (ctr = 0; ctr < msg->del_count; ctr++)
        {
          if (msg->peers_deleted[ctr].afi == AFI_IP)
            zlog_info (zg, " Peer deleted: %r",
                       &msg->peers_deleted[ctr].u.ipv4);
#ifdef HAVE_IPV6
          else if (msg->peers_deleted[ctr].afi == AFI_IP6)
            zlog_info (zg, " Peer deleted: %R",
                       &msg->peers_deleted[ctr].u.ipv6);
#endif /* HAVE_IPV6 */
        }
    }
}
#endif /* HAVE_VPLS */


#ifdef HAVE_MPLS
static const char *
nsm_mpls_act_type_str (ftn_action_type_t fat)
{
  switch (fat)
    {
    case DROP:
      return "Drop";
    case REDIRECT_LSP:
      return "Redirect LSP";
    case REDIRECT_TUNNEL:
      return "Redirect Tunnel";
    default:
      return "Unknown";
    }
}

void
nsm_gen_label_print (struct lib_globals *zg, struct gmpls_gen_label *lbl,
                     u_char *str)
{
  switch (lbl->type)
    {
#ifdef HAVE_PACKET
      case gmpls_entry_type_ip:
        zlog_info (zg, "%s label: %25u", str, lbl->u.pkt);
        break;
#endif /* HAVE_PACKET */

#ifdef HAVE_GMPLS
#ifdef HAVE_PBB_TE
      case gmpls_entry_type_pbb_te:
        zlog_info (zg, "%s label: %02u:%02u:%02u:%02u:%02u:%02u %d", str,
                   lbl->u.pbb.bmac [0], lbl->u.pbb.bmac [1], lbl->u.pbb.bmac [2],
                   lbl->u.pbb.bmac [3], lbl->u.pbb.bmac [4], lbl->u.pbb.bmac [5],
                   lbl->u.pbb.bvid);
        break;
#endif /* HAVE_PBB_TE */

#ifdef HAVE_TDM
      case gmpls_entry_type_tdm:
        zlog_info (zg, "%s label: Index %08d slot %08d", str, lbl->u.tdm.gifndex
                   lbl->u.tdm.tslot);

        break;
#endif /* HAVE_TDM */
#endif /* HAVE_GMPLS */
      default:
        break;
    }

}

void
nsm_gen_fec_print (struct lib_globals *zg, struct fec_gen_entry *fec,
                   u_char *str)
{
  switch (fec->type)
    {
#ifdef HAVE_PACKET
      case gmpls_entry_type_ip:
        zlog_info (zg, "%s: %O", str, &fec->u.prefix);
        break;
#endif /* HAVE_PACKET */

#ifdef HAVE_GMPLS
#ifdef HAVE_PBB_TE
      case gmpls_entry_type_pbb_te:
        zlog_info (zg, "%s : %06d", str, fec->u.pbb.isid [0] * 255 *255 +
                   fec->u.pbb.isid [1] * 255 + fec->u.pbb.isid [2]);
        break;
#endif /* HAVE_PBB_TE */

#ifdef HAVE_TDM
      case gmpls_entry_type_tdm:
        zlog_info (zg, "%s : Index %08d slot %08d", str, fec->u.tdm.gifndex
                   fec->u.tdm.tslot);

        break;
#endif /* HAVE_TDM */
#endif /* HAVE_GMPLS */
      default:
        break;
    }

}

void
nsm_ftn_ip_gen_dump (struct lib_globals *zg, struct ftn_add_gen_data *fad)
{
  zlog_info (zg, "FTN IPv4 %s",
             (CHECK_FLAG (fad->flags, NSM_MSG_GEN_FTN_ADD) ? "Add" :
              CHECK_FLAG (fad->flags, NSM_MSG_GEN_FTN_FAST_ADD) ? "Fast Add" :
              CHECK_FLAG (fad->flags, NSM_MSG_GEN_FTN_FAST_DEL) ? "Delete" :
              "Slow Delete"));

  if (CHECK_FLAG (fad->flags, NSM_MSG_GEN_FTN_ADD)
      || ! CHECK_FLAG (fad->flags, NSM_MSG_GEN_FTN_FAST_DEL))
    nsm_mpls_owner_dump (zg, &fad->owner);

  zlog_info (zg, " VRF ID: %d", fad->vrf_id);
  nsm_gen_fec_print (zg, &fad->ftn, "FEC: ");
  zlog_info (zg, " FTN index: %d", fad->ftn_ix);

  if (CHECK_FLAG (fad->flags, NSM_MSG_GEN_FTN_ADD) ||
      ! CHECK_FLAG (fad->flags, NSM_MSG_GEN_FTN_FAST_DEL))
    {
      if (fad->nh_addr.afi == AFI_IP)
        zlog_info (zg, " NH addr: %r", &fad->nh_addr.u.ipv4);
#ifdef HAVE_IPV6
      else if (fad->nh_addr.afi == AFI_IP6)
        zlog_info (zg, " NH addr: %R", &fad->nh_addr.u.ipv6);
#endif
#ifdef HAVE_GMPLS
      else if (fad->nh_addr.afi == AFI_UNNUMBERED)
        zlog_info (zg, " NH addr: %x", &fad->nh_addr.u.ipv4);
#endif /* HAVE_GMPLS */
      nsm_gen_label_print (zg, &fad->out_label, "Out");
      zlog_info (zg, " Out index: %d", fad->oif_ix);

      if (fad->owner.owner == MPLS_RSVP)
         zlog_info (zg, " LSP ID: %x", fad->lspid.u.rsvp_lspid);
      else
         zlog_info (zg, " LSP ID: %x %x %x %x %x %x",
                 fad->lspid.u.cr_lspid[5], fad->lspid.u.cr_lspid[4],
                 fad->lspid.u.cr_lspid[3], fad->lspid.u.cr_lspid[2],
                 fad->lspid.u.cr_lspid[1], fad->lspid.u.cr_lspid[0]);

      zlog_info (zg, " Exp bits: %d", fad->exp_bits);
#ifdef HAVE_PACKET
      if (CHECK_FLAG (fad->flags, NSM_MSG_FTN_DSCP_IN))
        zlog_info (zg, " Incoming DSCP: %s",
                   diffserv_class_name (fad->dscp_in));
#endif /* HAVE_PACKET */

      if (fad->lsp_bits.bits.lsp_type == LSP_TYPE_PRIMARY)
        zlog_info (zg, " LSP-Type: Primary");
      else if (fad->lsp_bits.bits.lsp_type == LSP_TYPE_SECONDARY)
        zlog_info (zg, " LSP-Type: Secondary");
      else if (fad->lsp_bits.bits.lsp_type == LSP_TYPE_BACKUP)
        zlog_info (zg, " LSP-Type: BACKUP");
      else if (fad->lsp_bits.bits.lsp_type == LSP_TYPE_BYPASS)
        zlog_info (zg, " LSP-Type: Bypass");
      zlog_info (zg, " Action type: %s",
                 nsm_mpls_act_type_str (fad->lsp_bits.bits.act_type));

      if (fad->lsp_bits.bits.igp_shortcut)
        zlog_info (zg, " IGP-Shortcut enabled");
      else
        zlog_info (zg, " IGP-Shortcut disabled");

      zlog_info (zg, " LSP Metric: %d", fad->lsp_bits.bits.lsp_metric);

      zlog_info (zg, " Opcode: %s", nsm_mpls_dump_op_code (fad->opcode));
      zlog_info (zg, " Tunnel id: %u", fad->tunnel_id);
      zlog_info (zg, " Protected LSP id: %u", fad->protected_lsp_id);

      if (fad->lsp_bits.bits.act_type == REDIRECT_TUNNEL)
        zlog_info (zg, " QoS Resource id: %u", fad->qos_resrc_id);
      if (fad->sz_desc)
        zlog_info (zg, " Description: %s", fad->sz_desc);
      if (fad->pri_fec_prefix)
        zlog_info (zg, " Primary FEC: %O", fad->pri_fec_prefix);

      zlog_info (zg, " bypass_ftn_ix: %d", fad->bypass_ftn_ix);
    }
}

void
nsm_ftn_bidir_ip_gen_dump (struct lib_globals *zg,
                           struct ftn_bidir_add_data *fad)
{
  zlog_info (zg, "Bidirectional FTN\n");
  nsm_ftn_ip_gen_dump (zg, &fad->ftn);
  nsm_ilm_ip_gen_dump (zg, &fad->ilm);
  return;
}

void
nsm_ilm_bidir_ip_gen_dump (struct lib_globals *zg,
                           struct ilm_bidir_add_data *iad)
{
  zlog_info (zg, "Bidirectional ILM\n");
  nsm_ilm_ip_gen_dump (zg, &iad->ilm_fwd);
  nsm_ilm_ip_gen_dump (zg, &iad->ilm_bwd);
  return;
}

#ifdef HAVE_PACKET
void
nsm_ftn_ip_dump (struct lib_globals *zg, struct ftn_add_data *fad)
{
  zlog_info (zg, "FTN IPv4 %s",
             (CHECK_FLAG (fad->flags, NSM_MSG_FTN_ADD) ? "Add" :
              CHECK_FLAG (fad->flags, NSM_MSG_FTN_FAST_DELETE) ? "Delete" :
              "Slow Delete"));

  if (CHECK_FLAG (fad->flags, NSM_MSG_FTN_ADD)
      || ! CHECK_FLAG (fad->flags, NSM_MSG_FTN_FAST_DELETE))
    nsm_mpls_owner_dump (zg, &fad->owner);

  zlog_info (zg, " VRF ID: %d", fad->vrf_id);
  zlog_info (zg, " FEC: %O", &fad->fec_prefix);
  zlog_info (zg, " FTN index: %d", fad->ftn_ix);

  if (CHECK_FLAG (fad->flags, NSM_MSG_FTN_ADD) ||
      ! CHECK_FLAG (fad->flags, NSM_MSG_FTN_FAST_DELETE))
    {
      if (fad->nh_addr.afi == AFI_IP)
        zlog_info (zg, " NH addr: %r", &fad->nh_addr.u.ipv4);
#ifdef HAVE_IPV6
      else if (fad->nh_addr.afi == AFI_IP6)
        zlog_info (zg, " NH addr: %R", &fad->nh_addr.u.ipv6);
#endif
      zlog_info (zg, " Out Label: %d", fad->out_label);
      zlog_info (zg, " Out index: %d", fad->oif_ix);

      if (fad->owner.owner == MPLS_RSVP)
         zlog_info (zg, " LSP ID: %x", fad->lspid.u.rsvp_lspid);
      else
         zlog_info (zg, " LSP ID: %x %x %x %x %x %x",
                 fad->lspid.u.cr_lspid[5], fad->lspid.u.cr_lspid[4],
                 fad->lspid.u.cr_lspid[3], fad->lspid.u.cr_lspid[2],
                 fad->lspid.u.cr_lspid[1], fad->lspid.u.cr_lspid[0]);

      zlog_info (zg, " Exp bits: %d", fad->exp_bits);
      if (CHECK_FLAG (fad->flags, NSM_MSG_FTN_DSCP_IN))
        zlog_info (zg, " Incoming DSCP: %s",
                   diffserv_class_name (fad->dscp_in));
      if (fad->lsp_bits.bits.lsp_type == LSP_TYPE_PRIMARY)
        zlog_info (zg, " LSP-Type: Primary");
      else if (fad->lsp_bits.bits.lsp_type == LSP_TYPE_SECONDARY)
        zlog_info (zg, " LSP-Type: Secondary");
      else if (fad->lsp_bits.bits.lsp_type == LSP_TYPE_BACKUP)
        zlog_info (zg, " LSP-Type: BACKUP");
      else if (fad->lsp_bits.bits.lsp_type == LSP_TYPE_BYPASS)
        zlog_info (zg, " LSP-Type: Bypass");
      zlog_info (zg, " Action type: %s",
                 nsm_mpls_act_type_str (fad->lsp_bits.bits.act_type));

      if (fad->lsp_bits.bits.igp_shortcut)
        zlog_info (zg, " IGP-Shortcut enabled");
      else
        zlog_info (zg, " IGP-Shortcut disabled");

      zlog_info (zg, " LSP Metric: %d", fad->lsp_bits.bits.lsp_metric);

      zlog_info (zg, " Opcode: %s", nsm_mpls_dump_op_code (fad->opcode));
      zlog_info (zg, " Tunnel id: %u", fad->tunnel_id);
      zlog_info (zg, " Protected LSP id: %u", fad->protected_lsp_id);

      if (fad->lsp_bits.bits.act_type == REDIRECT_TUNNEL)
        zlog_info (zg, " QoS Resource id: %u", fad->qos_resrc_id);
      if (fad->sz_desc)
        zlog_info (zg, " Description: %s", fad->sz_desc);
      if (fad->pri_fec_prefix)
        zlog_info (zg, " Primary FEC: %O", fad->pri_fec_prefix);

      zlog_info (zg, " bypass_ftn_ix: %d", fad->bypass_ftn_ix);
    }
}
#endif /* HAVE_PACKET */

void
nsm_ilm_ip_gen_dump (struct lib_globals *zg, struct ilm_add_gen_data *iad)
{
  zlog_info (zg, "ILM IPv4 %s",
             CHECK_FLAG (iad->flags, NSM_MSG_ILM_ADD) ? "Add"
             : CHECK_FLAG (iad->flags, NSM_MSG_ILM_DELETE) ? "Delete"
             : "Fast Delete");

  if (CHECK_FLAG (iad->flags, NSM_MSG_ILM_ADD)
      || CHECK_FLAG (iad->flags, NSM_MSG_ILM_DELETE))
    nsm_mpls_owner_dump (zg, &iad->owner);

  zlog_info (zg, " In gifindex: %u", iad->iif_ix);
  nsm_gen_label_print (zg, &iad->in_label, "IN ");
  zlog_info (zg, " ILM index:%u", iad->ilm_ix);

  if (CHECK_FLAG (iad->flags, NSM_MSG_ILM_ADD)
      || CHECK_FLAG (iad->flags, NSM_MSG_ILM_DELETE))
    {
      nsm_gen_label_print (zg, &iad->out_label, "OUT ");

      zlog_info (zg, " Out index: %u", iad->oif_ix);
      zlog_info (zg, " NH addr: %r", &iad->nh_addr.u.ipv4);

      if (iad->owner.owner == MPLS_RSVP)
         zlog_info (zg, " LSP ID: %x", iad->lspid.u.rsvp_lspid);
      else
         zlog_info (zg, " LSP ID: %x %x %x %x %x %x",
                          iad->lspid.u.cr_lspid[5], iad->lspid.u.cr_lspid[4],
                          iad->lspid.u.cr_lspid[3], iad->lspid.u.cr_lspid[2],
                          iad->lspid.u.cr_lspid[1], iad->lspid.u.cr_lspid[0]);

      zlog_info (zg, " # of pops: %d", iad->n_pops);
      if (iad->fec_prefix.family == AF_INET)
       zlog_info (zg, " FEC: %r", &iad->fec_prefix.u.prefix4);
#ifdef HAVE_IPV6
      else if (iad->fec_prefix.family == AF_INET6)
        zlog_info (zg, " FEC: %R", &iad->fec_prefix.u.prefix6);
#endif
      zlog_info (zg, " QOS resource ID: %u", iad->qos_resrc_id);
      zlog_info (zg, " Opcode: %s", nsm_mpls_dump_op_code (iad->opcode));
      zlog_info (zg, " Primary ILM entry: %d", iad->primary);
    }
}

#ifdef HAVE_PACKET
void
nsm_ilm_ip_dump (struct lib_globals *zg, struct ilm_add_data *iad)
{
  zlog_info (zg, "ILM IPv4 %s",
             CHECK_FLAG (iad->flags, NSM_MSG_ILM_ADD) ? "Add"
             : CHECK_FLAG (iad->flags, NSM_MSG_ILM_DELETE) ? "Delete"
             : "Fast Delete");

  if (CHECK_FLAG (iad->flags, NSM_MSG_ILM_ADD)
      || CHECK_FLAG (iad->flags, NSM_MSG_ILM_DELETE))
    nsm_mpls_owner_dump (zg, &iad->owner);

  zlog_info (zg, " In index: %u", iad->iif_ix);
  zlog_info (zg, " In label: %u", iad->in_label);
  zlog_info (zg, " ILM index:%u", iad->ilm_ix);

  if (CHECK_FLAG (iad->flags, NSM_MSG_ILM_ADD)
      || CHECK_FLAG (iad->flags, NSM_MSG_ILM_DELETE))
    {
      zlog_info (zg, " Out Label: %u", iad->out_label);
      zlog_info (zg, " Out index: %u", iad->oif_ix);
      zlog_info (zg, " NH addr: %r", &iad->nh_addr.u.ipv4);

      if (iad->owner.owner == MPLS_RSVP)
         zlog_info (zg, " LSP ID: %x", iad->lspid.u.rsvp_lspid);
      else
         zlog_info (zg, " LSP ID: %x %x %x %x %x %x",
                          iad->lspid.u.cr_lspid[5], iad->lspid.u.cr_lspid[4],
                          iad->lspid.u.cr_lspid[3], iad->lspid.u.cr_lspid[2],
                          iad->lspid.u.cr_lspid[1], iad->lspid.u.cr_lspid[0]);

      zlog_info (zg, " # of pops: %d", iad->n_pops);
      if (iad->fec_prefix.family == AF_INET)
       zlog_info (zg, " FEC: %r", &iad->fec_prefix.u.prefix4);
#ifdef HAVE_IPV6
      else if (iad->fec_prefix.family == AF_INET6)
        zlog_info (zg, " FEC: %R", &iad->fec_prefix.u.prefix6);
#endif
      zlog_info (zg, " QOS resource ID: %u", iad->qos_resrc_id);
      zlog_info (zg, " Opcode: %s", nsm_mpls_dump_op_code (iad->opcode));
      zlog_info (zg, " Primary ILM entry: %d", iad->primary);
    }
}
#endif /* HAVE_PACKET */

void
nsm_mpls_notification_dump (struct lib_globals *zg,
                            struct mpls_notification *mn)
{
  zlog_info (zg, "MPLS Notification message");
  nsm_mpls_owner_dump (zg, &mn->owner);
  zlog_info (zg, " Notification type: %s",
             nsm_mpls_dump_notification_type (mn->type));
  zlog_info (zg, " Notification status: %s",
             nsm_mpls_dump_notification_status (mn->status));
}

void
nsm_ftn_ret_data_dump (struct lib_globals *zg, struct ftn_ret_data *frd)
{
  zlog_info (zg, "FTN Return Data for %s message",
             (CHECK_FLAG (frd->flags, NSM_MSG_REPLY_TO_FTN_ADD)
              ? "ADD" : "DELETE"));
  nsm_mpls_owner_dump (zg, &frd->owner);
  zlog_info (zg, " FTN Index: %d", frd->ftn_ix);
  zlog_info (zg, " Cross Connect Index: %d", frd->xc_ix);
  zlog_info (zg, " Out Segment Index: %d", frd->nhlfe_ix);
  zlog_info (zg, " Message id: %d", frd->id);
}

void
nsm_ilm_ret_data_dump (struct lib_globals *zg, struct ilm_ret_data *ird)
{
  zlog_info (zg, "ILM Return Data for %s message",
             (CHECK_FLAG (ird->flags, NSM_MSG_REPLY_TO_ILM_ADD)
              ? "ADD" : "DELETE"));
  nsm_mpls_owner_dump (zg, &ird->owner);
  zlog_info (zg, " Incoming interface index: %u", ird->iif_ix);
  zlog_info (zg, "Incoming Label:", ird->in_label);
  zlog_info (zg, " ILM Index: %u", ird->ilm_ix);
  zlog_info (zg, " Cross Connect Index: %u", ird->xc_ix);
  zlog_info (zg, " Out Segment Index: %u", ird->nhlfe_ix);
  zlog_info (zg, " Message id: %u", ird->id);
}

#endif /* HAVE_MPLS */

#ifdef HAVE_GMPLS
void
nsm_gmpls_if_dump_capability (struct lib_globals *zg, u_int32_t capability)
{
  zlog_info (zg, "  Switching Capability Type :");

  if (capability == 0)
    zlog_info (zg, "    Unknown");
  else
    {
      if (CHECK_FLAG (capability, GMPLS_SWITCH_CAPABILITY_FLAG_PSC1))
        zlog_info (zg, "    %s",
                   gmpls_capability_type_str (GMPLS_SWITCH_CAPABILITY_PSC1));
      if (CHECK_FLAG (capability, GMPLS_SWITCH_CAPABILITY_FLAG_PSC2))
        zlog_info (zg, "    %s",
                   gmpls_capability_type_str (GMPLS_SWITCH_CAPABILITY_PSC2));
      if (CHECK_FLAG (capability, GMPLS_SWITCH_CAPABILITY_FLAG_PSC3))
        zlog_info (zg, "    %s",
                   gmpls_capability_type_str (GMPLS_SWITCH_CAPABILITY_PSC3));
      if (CHECK_FLAG (capability, GMPLS_SWITCH_CAPABILITY_FLAG_PSC4))
        zlog_info (zg, "    %s",
                   gmpls_capability_type_str (GMPLS_SWITCH_CAPABILITY_PSC4));
      if (CHECK_FLAG (capability, GMPLS_SWITCH_CAPABILITY_FLAG_L2SC))
        zlog_info (zg, "    %s",
                   gmpls_capability_type_str (GMPLS_SWITCH_CAPABILITY_L2SC));
      if (CHECK_FLAG (capability, GMPLS_SWITCH_CAPABILITY_FLAG_TDM))
        zlog_info (zg, "    %s",
                   gmpls_capability_type_str (GMPLS_SWITCH_CAPABILITY_TDM));
      if (CHECK_FLAG (capability, GMPLS_SWITCH_CAPABILITY_FLAG_LSC))
        zlog_info (zg, "    %s",
                   gmpls_capability_type_str (GMPLS_SWITCH_CAPABILITY_LSC));
      if (CHECK_FLAG (capability, GMPLS_SWITCH_CAPABILITY_FLAG_FSC))
        zlog_info (zg, "    %s",
                   gmpls_capability_type_str (GMPLS_SWITCH_CAPABILITY_FSC));
    }
}

void
nsm_ilm_gen_ret_data_dump (struct lib_globals *zg, struct ilm_gen_ret_data *igrd)
{
  zlog_info (zg, "ILM Return Data for %s message",
             (CHECK_FLAG (igrd->flags, NSM_MSG_REPLY_TO_ILM_ADD)
              ? "ADD" : "DELETE"));
  nsm_mpls_owner_dump (zg, &igrd->owner);
  zlog_info (zg, " Incoming interface index: %u", igrd->iif_ix);
  zlog_info (zg, "Incoming Label type :", igrd->in_label.type);
  zlog_info (zg, "Incoming Label :", igrd->in_label.u.pkt);
  zlog_info (zg, " ILM Index: %u", igrd->ilm_ix);
  zlog_info (zg, " Cross Connect Index: %u", igrd->xc_ix);
  zlog_info (zg, " Out Segment Index: %u", igrd->nhlfe_ix);
  zlog_info (zg, " Message id: %u", igrd->id);
}

#if 0 /*TBD Old Code*/
void
nsm_gmpls_if_dump (struct lib_globals *zg, struct nsm_msg_gmpls_if *msg)
{
  int i;
  u_int32_t *group;

  zlog_info (zg, "GMPLS Interface TE data");
  zlog_info (zg, " Ifindex : %d", msg->ifindex);
  zlog_info (zg, "  Link Local ID  : %d", msg->link_ids.local);
  zlog_info (zg, "  Link Remote ID : %d", msg->link_ids.remote);
  zlog_info (zg, "  Link Protection Type : %s",
             gmpls_protection_type_str (msg->protection));
  nsm_gmpls_if_dump_capability (zg, msg->capability);
  zlog_info (zg, "  LSP Encoding : %s",
             gmpls_encoding_type_str (msg->encoding));
  zlog_info (zg, "  Minimum LSP Bandwidth : %.02f", msg->min_lsp_bw);
  zlog_info (zg, "  SONET/SDH Indication : %s",
             gmpls_sdh_indication_str (msg->indication));
  for (i = 0; i < vector_count (msg->srlg); i++)
    if ((group = vector_slot (msg->srlg, i)))
      zlog_info (zg, "  Shared Risk Link Group[%d] : %u", i, *group);
}
#endif
#endif /* HAVE_GMPLS */

#ifdef HAVE_L2
/* Bridge dump. */
void
nsm_bridge_dump (struct lib_globals *zg, struct nsm_msg_bridge *msg)
{
  zlog_info (zg, " Bridge Name: %s", msg->bridge_name);
  if (msg->type >= NSM_BRIDGE_TYPE_STP && msg->type <= NSM_BRIDGE_TYPE_MSTP)
    zlog_info (zg, " Type : %s", nsm_bridge_type_str[msg->type-1]);
  else
    zlog_info (zg, " Type : Unknown");
  zlog_info (zg, " Ageing time : %u", msg->ageing_time);
}

void
nsm_bridge_enable_dump (struct lib_globals *zg,
                        struct nsm_msg_bridge_enable *msg)
{
  zlog_info (zg, " Bridge Name: %s", msg->bridge_name);
  zlog_info (zg, " Enable : %u", msg->enable);
}

/* Bridge dump. */
void
nsm_bridge_port_dump (struct lib_globals *zg,
                      struct nsm_msg_bridge_port *msg)
{
  int i;

  zlog_info (zg, " Interface Index: %d", msg->ifindex);
  zlog_info (zg, " Bridge Name: %s", msg->bridge_name);
  if ((msg->num > 0) && (msg->instance) && (msg->state))
    for (i = 0; i < msg->num; i++)
      {
        if ((msg->state[i]) > 0 &&
            (msg->state[i] <= NSM_BRIDGE_PORT_STATE_DISCARDING))
          {
            zlog_info (zg, " Instance: %d", msg->instance[i]);
            zlog_info (zg, " Port State: %s",
                       nsm_bridge_port_state_str[msg->state[i] - 1]);
          }
      }
}
#endif /* HAVE_L2 */

#ifndef HAVE_CUSTOM1
#ifdef HAVE_VLAN
/* VLAN message dump. */
void
nsm_vlan_dump (struct lib_globals *zg, struct nsm_msg_vlan *msg)
{
  zlog_info (zg, "  Flags %s %s %s",
             (msg->flags & NSM_MSG_VLAN_STATE_ACTIVE) ?  "Active" : "Suspend",
             (msg->flags & NSM_MSG_VLAN_STATIC) ? "Static" : "",
             (msg->flags & NSM_MSG_VLAN_DYNAMIC) ? "Dynamic" : "");
  if (NSM_CHECK_CTYPE (msg->cindex, NSM_VLAN_CTYPE_NAME))
   {
     if (pal_strlen (msg->vlan_name))
       zlog_info (zg, " %s", msg->vlan_name);
   }
  if (NSM_CHECK_CTYPE (msg->cindex, NSM_VLAN_CTYPE_BRIDGENAME))
   {
     if (pal_strlen (msg->bridge_name))
       zlog_info (zg, " Bridge: %s", msg->bridge_name);
   }
  if (NSM_CHECK_CTYPE (msg->cindex, NSM_VLAN_CTYPE_INSTANCE))
   {
       zlog_info (zg, " Instance: %d", msg->br_instance);
   }
}

/* VLAN if message dump. */
void
nsm_vlan_port_dump (struct lib_globals *zg, struct nsm_msg_vlan_port *msg)
{
  int i;
  zlog_info (zg, " Ifindex: %d", msg->ifindex);

  for (i = 0; i < msg->num; i++)
    {
      zlog_info (zg, " VLAN ID: %d egress %s %s\n",
                 msg->vid_info[i] & NSM_VLAN_VID_MASK,
                 ((1 << NSM_VLAN_EGRESS_SHIFT_BITS) & msg->vid_info[i]) ? "tagged" : "untagged", ((1 << NSM_VLAN_DEFAULT_SHIFT_BITS) & msg->vid_info[i]) ? ": default" : "");
    }
}

/* VLAN port type message dump. */
void
nsm_vlan_port_type_dump (struct lib_globals *zg, struct nsm_msg_vlan_port_type *msg)
{
  zlog_info (zg, " Ifindex: %d", msg->ifindex);
  zlog_info (zg, " Porttype: %s", (((msg->port_type == NSM_MSG_VLAN_PORT_MODE_INVALID) ? "Invalid" : (msg->port_type == NSM_MSG_VLAN_PORT_MODE_ACCESS) ? "Access" : (msg->port_type == NSM_MSG_VLAN_PORT_MODE_HYBRID) ? "Hybrid" : (msg->port_type == NSM_MSG_VLAN_PORT_MODE_TRUNK) ? "Trunk" : "Invalid")));
  zlog_info (zg, " Ingress filter: %s", ((msg->ingress_filter == NSM_MSG_VLAN_PORT_DISABLE_INGRESS_FILTER) ? "Disable" : (msg->ingress_filter == NSM_MSG_VLAN_PORT_ENABLE_INGRESS_FILTER) ? "Enable" : "Invalid"));
  zlog_info (zg, " Acceptable frame type: %s", ((msg->acceptable_frame_type == NSM_MSG_VLAN_ACCEPT_FRAME_ALL) ? "All" : (msg->acceptable_frame_type == NSM_MSG_VLAN_ACCEPT_TAGGED_ONLY) ? "Tagged only" : (msg->acceptable_frame_type == NSM_MSG_VLAN_ACCEPT_UNTAGGED_ONLY) ? "Untagged only" : "Invalid"));
}

#ifdef HAVE_VLAN_CLASS
/* VLAN classifier message dump */
void
nsm_vlan_classifier_dump (struct lib_globals *zg,
                          struct nsm_msg_vlan_classifier *msg)
{
  zlog_info (zg, " Classifier Group Id: %d", msg->class_group);
  switch((msg->class_type) & NSM_MSG_VLAN_CLASS_TYPE_MASK)
  {
    case NSM_MSG_VLAN_CLASS_SRC_MAC:
      zlog_info (zg, " Type: Source Mac");
      zlog_info (zg, " Source Mac Address: %s", msg->class_data);
     break;
    case NSM_MSG_VLAN_CLASS_SUBNET:
      zlog_info (zg, " Type: IPv4 Subnet");
      zlog_info (zg, " IPv4 Subnet: %s", msg->class_data);
     break;
    case NSM_MSG_VLAN_CLASS_PROTO:
      {
        zlog_info(zg, " Type: Protocol");
        zlog_info(zg, " Protocol: ");
        switch (msg->class_type)
        {
          case NSM_MSG_VLAN_CLASS_LLC:
           zlog_info(zg, " LLC");
           break;
          case NSM_MSG_VLAN_CLASS_APPLETALK:
           zlog_info(zg, " AppleTalk");
           break;
          case NSM_MSG_VLAN_CLASS_IPX:
           zlog_info(zg, " IPX");
           break;
          case NSM_MSG_VLAN_CLASS_ETHERNET:
            zlog_info(zg, " Ethernet");
           break;
          default:
            zlog_info(zg, " Unknown");
           break;
        }
      }
     break;
    default:
     zlog_info(zg, " Type: Unknown");
     break;
  }
}

/* VLAN port classifier message dump. */
void
nsm_vlan_port_class_dump (struct lib_globals *zg,
                          struct nsm_msg_vlan_port_classifier *msg)
{
  int i;
  zlog_info (zg, " Ifindex: %d", msg->ifindex);
  zlog_info (zg, " VLAN ID: %d\n Classifier Groups: ",
                 (msg->vid_info & NSM_VLAN_VID_MASK));

  for (i = 0; i < msg->cnum; i++)
    {
      zlog_info (zg, " %d ", msg->class_info[i]);
    }
  zlog_info(zg, "\n");
}

#endif /* HAVE_VLAN_CLASS */

#endif /* HAVE_VLAN */
#endif /* HAVE_CUSTOM1 */

#ifdef HAVE_AUTHD
/* Port authentication port state message dump.  */
void
nsm_auth_port_state_dump (struct lib_globals *zg,
                          struct nsm_msg_auth_port_state *msg)
{
  zlog_info (zg, "802.1X port authorization");
  zlog_info (zg, " Port index: %lu", msg->ifindex);
  zlog_info (zg, " Port control direction: %s",
             msg->ctrl == NSM_AUTH_PORT_CTRL_UNCONTROLLED
             ? "Uncontrolled" : msg->ctrl == NSM_AUTH_PORT_CTRL_DIR_BOTH
             ? "Both" : "In");
  zlog_info (zg, " Port control state: %s",
             msg->state == NSM_AUTH_PORT_STATE_AUTHORIZED
             ? "Authorized" : "Unauthorized");
}

#ifdef HAVE_MAC_AUTH
/*MAC-based authentication Enhancement*/
void
nsm_auth_mac_port_state_dump (struct lib_globals *zg,
                              struct nsm_msg_auth_mac_port_state *msg)
{
  zlog_info (zg, "802.1X port authorization");
  zlog_info (zg, " Port index: %lu", msg->ifindex);
  zlog_info (zg, " Port control direction: %s",
             msg->state == NSM_MACAUTH_PORT_STATE_ENABLED
             ? "Enabled" : "Disabled");
}
#endif /* HAVE_MAC_AUTH */
#endif /* HAVE_AUTHD */

#ifdef HAVE_ONMD

/* EFM port state message dump.  */
void
nsm_efm_port_msg_dump (struct lib_globals *zg,
                       struct nsm_msg_efm_if *msg)
{
  zlog_info (zg, " Ethernet First Mile OAM");

  zlog_info (zg, " Port index: %lu", msg->ifindex);

  zlog_info (zg, " Opcode : %lu", msg->opcode);

  zlog_info (zg, " Port Parser state : %s",
             msg->local_par_action ? "Parser Enabled":
                                     "Parser Disabled");
  zlog_info (zg, " Port Multiplexer state : %s",
             msg->local_mux_action ? "Multiplexer Enabled":
                                     "Multiplexer Disabled");
}

void
nsm_lldp_msg_dump (struct lib_globals *zg,
                   struct nsm_msg_lldp *msg)
{
  zlog_info (zg, " LLDP");

  zlog_info (zg, " Opcode : %lu", msg->opcode);

  zlog_info (zg, " Port index: %lu", msg->ifindex);

  zlog_info (zg, " System Capability: %lu", msg->syscap);

  zlog_info (zg, " HW address: %02x%02x.%02x%02x.%02x%02x",
             msg->lldp_dest_addr[0], msg->lldp_dest_addr[1],
             msg->lldp_dest_addr[2], msg->lldp_dest_addr[3],
             msg->lldp_dest_addr[4], msg->lldp_dest_addr[5]);

}

void
nsm_cfm_msg_dump (struct lib_globals *zg,
                  struct nsm_msg_cfm *msg)
{
  zlog_info (zg, " CFM");


  zlog_info (zg, " Opcode : %lu", msg->opcode);


  zlog_info (zg, " Port index: %lu", msg->ifindex);


  zlog_info (zg, " HW address: %02x%02x.%02x%02x.%02x%02x",
             msg->cfm_dest_addr[0], msg->cfm_dest_addr[1],
             msg->cfm_dest_addr[2], msg->cfm_dest_addr[3],
             msg->cfm_dest_addr[4], msg->cfm_dest_addr[5]);

  zlog_info (zg, " Ethernet Type: %lu", msg->ether_type);
}

#endif

#ifdef HAVE_ELMID

void
nsm_elmi_bw_dump (struct lib_globals *zg, struct nsm_msg_bw_profile *msg)
{
  zlog_info (zg, "NSM ELMI BW");

   zlog_info (zg, " cinsdex = %d", msg->cindex);
  if (NSM_CHECK_CTYPE (msg->cindex, NSM_ELMI_CTYPE_UNI_BW))
    {
       zlog_info (zg, " BW profile set for UNI");
    }
  else if (NSM_CHECK_CTYPE (msg->cindex, NSM_ELMI_CTYPE_EVC_BW))
    {
      zlog_info (zg, " BW profile set for EVC: %d", msg->cindex);
    }

  zlog_info (zg, " Ifindex: %d", msg->ifindex);
  zlog_info (zg, " CIR: %d", msg->cir);
  zlog_info (zg, " CBS: %d", msg->cbs);
  zlog_info (zg, " EIR: %d", msg->eir);
  zlog_info (zg, " EBS: %d", msg->ebs);
  zlog_info (zg, " coupling_flag: %d", msg->cf);
  zlog_info (zg, " color_mode: %d", msg->cm);

}

#endif

/* NSM message parser.  */

/* NSM message header encode.  */
s_int32_t
nsm_encode_header (u_char **pnt, u_int16_t *size,
                   struct nsm_msg_header *header)
{
  u_char *sp = *pnt;

  if (*size < NSM_MSG_HEADER_SIZE)
    return NSM_ERR_PKT_TOO_SMALL;

  TLV_ENCODE_PUTL (header->vr_id);
  TLV_ENCODE_PUTL (header->vrf_id);
  TLV_ENCODE_PUTW (header->type);
  TLV_ENCODE_PUTW (header->length);
  TLV_ENCODE_PUTL (header->message_id);

  return *pnt - sp;
}

/* NSM message header decode.  */
s_int32_t
nsm_decode_header (u_char **pnt, u_int16_t *size,
                   struct nsm_msg_header *header)
{
  if (*size < NSM_MSG_HEADER_SIZE)
    return NSM_ERR_PKT_TOO_SMALL;

  TLV_DECODE_GETL (header->vr_id);
  TLV_DECODE_GETL (header->vrf_id);
  TLV_DECODE_GETW (header->type);
  TLV_DECODE_GETW (header->length);
  TLV_DECODE_GETL (header->message_id);

  return NSM_MSG_HEADER_SIZE;
}

s_int32_t
nsm_encode_tlv (u_char **pnt, u_int16_t *size, u_int16_t type,
                u_int16_t length)
{
  length += NSM_TLV_HEADER_SIZE;

  TLV_ENCODE_PUTW (type);
  TLV_ENCODE_PUTW (length);

  return NSM_TLV_HEADER_SIZE;
}

/* NSM service encode.  */
s_int32_t
nsm_encode_service (u_char **pnt, u_int16_t *size, struct nsm_msg_service *msg)
{
  u_char *sp = *pnt;
  afi_t afi;
  safi_t safi;

  if (*size < NSM_MSG_SERVICE_SIZE)
    return NSM_ERR_PKT_TOO_SMALL;

  TLV_ENCODE_PUTW (msg->version);
  TLV_ENCODE_PUTW (0);
  TLV_ENCODE_PUTL (msg->protocol_id);
  TLV_ENCODE_PUTL (msg->client_id);
  TLV_ENCODE_PUTL (msg->bits);
  TLV_ENCODE_PUTC (msg->restart_state);

  if (NSM_CHECK_CTYPE (msg->cindex, NSM_SERVICE_CTYPE_RESTART))
    {
      int num = 0;
      u_char on = 1;

      for (afi = AFI_IP; afi < AFI_MAX; afi++)
        for (safi = SAFI_UNICAST; safi < SAFI_MAX; safi++)
          if (msg->restart[afi][safi])
            num++;

      nsm_encode_tlv (pnt, size, NSM_SERVICE_CTYPE_RESTART, num * 4);

      for (afi = AFI_IP; afi < AFI_MAX; afi++)
        for (safi = SAFI_UNICAST; safi < SAFI_MAX; safi++)
          if (msg->restart[afi][safi])
            {
              TLV_ENCODE_PUTW (afi);
              TLV_ENCODE_PUTC (safi);
              TLV_ENCODE_PUTC (on);
            }
    }

  if (NSM_CHECK_CTYPE (msg->cindex, NSM_SERVICE_CTYPE_GRACE_EXPIRES))
    {
      nsm_encode_tlv (pnt, size, NSM_SERVICE_CTYPE_GRACE_EXPIRES, 4);
      TLV_ENCODE_PUTL (msg->grace_period);
    }

  if (NSM_CHECK_CTYPE (msg->cindex, NSM_SERVICE_CTYPE_RESTART_OPTION))
    {
      nsm_encode_tlv (pnt, size, NSM_SERVICE_CTYPE_RESTART_OPTION,
                      msg->restart_length);
      TLV_ENCODE_PUT (msg->restart_val, msg->restart_length);
    }

#ifdef HAVE_MPLS
  if (NSM_CHECK_CTYPE (msg->cindex, NSM_SERVICE_CTYPE_LABEL_POOLS))
    {
      nsm_encode_tlv (pnt, size, NSM_SERVICE_CTYPE_LABEL_POOLS,
                 sizeof (struct nsm_msg_label_pool) * msg->label_pool_num + 2);
      TLV_ENCODE_PUTW (msg->label_pool_num);
      TLV_ENCODE_PUT (msg->label_pools,
                      sizeof (struct nsm_msg_label_pool) * msg->label_pool_num);
    }
#endif /* HAVE_MPLS */

  return *pnt - sp;
}

/* NSM service decode.  */
s_int32_t
nsm_decode_service (u_char **pnt, u_int16_t *size, struct nsm_msg_service *msg)
{
  u_char *sp = *pnt;
  struct nsm_tlv_header tlv;
  int i;
  afi_t afi;
  safi_t safi;
  u_char on;

  if (*size < NSM_MSG_SERVICE_SIZE)
    return NSM_ERR_PKT_TOO_SMALL;

  TLV_DECODE_GETW (msg->version);
  TLV_DECODE_GETW (msg->reserved);
  TLV_DECODE_GETL (msg->protocol_id);
  TLV_DECODE_GETL (msg->client_id);
  TLV_DECODE_GETL (msg->bits);
  TLV_DECODE_GETC (msg->restart_state);

  /* Optional TLV parser.  */
  while (*size)
    {
      if (*size < NSM_TLV_HEADER_SIZE)
        return NSM_ERR_PKT_TOO_SMALL;

      NSM_DECODE_TLV_HEADER (tlv);

      switch (tlv.type)
        {
        case NSM_SERVICE_CTYPE_RESTART:
          for (i = 0; i < tlv.length / 4; i++)
            {
              TLV_DECODE_GETW (afi);
              TLV_DECODE_GETC (safi);
              TLV_DECODE_GETC (on);
              msg->restart[afi][safi] = on;
            }
          NSM_SET_CTYPE (msg->cindex, tlv.type);
          break;
        case NSM_SERVICE_CTYPE_GRACE_EXPIRES:
          TLV_DECODE_GETL (msg->grace_period);
          NSM_SET_CTYPE (msg->cindex, tlv.type);
          break;
        case NSM_SERVICE_CTYPE_RESTART_OPTION:
          msg->restart_length = tlv.length;
          msg->restart_val = *pnt;
          TLV_DECODE_SKIP (tlv.length);
          NSM_SET_CTYPE (msg->cindex, tlv.type);
          break;
#ifdef HAVE_MPLS
        case NSM_SERVICE_CTYPE_LABEL_POOLS:
          TLV_DECODE_GETW (msg->label_pool_num);
          msg->label_pools = (struct nsm_msg_label_pool *)*pnt;
          TLV_DECODE_SKIP (tlv.length-2);
          NSM_SET_CTYPE (msg->cindex, tlv.type);
          break;
#endif /* HAVE_MPLS */
        default:
          TLV_DECODE_SKIP (tlv.length);
          break;
        }
    }
  return *pnt - sp;
}

s_int32_t
nsm_encode_link (u_char **pnt, u_int16_t *size, struct nsm_msg_link *msg)
{
  u_char *sp = *pnt;
  int i;

  /* Check size.  */
  if (*size < NSM_MSG_LINK_SIZE)
    return NSM_ERR_PKT_TOO_SMALL;

  /* Interface index.  */
  TLV_ENCODE_PUTL (msg->ifindex);

#ifdef HAVE_GMPLS
  /* GMPLS Interface index.  */
  TLV_ENCODE_PUTL (msg->gifindex);
#endif /*HAVE_GMPLS */

  /* Interface TLV parse.  */
  for (i = 0; i < NSM_CINDEX_SIZE; i++)
    if (NSM_CHECK_CTYPE (msg->cindex, i))
      {
        if (*size < NSM_TLV_HEADER_SIZE)
          return NSM_ERR_PKT_TOO_SMALL;

        switch (i)
          {
          case NSM_LINK_CTYPE_NAME:
            nsm_encode_tlv (pnt, size, i, INTERFACE_NAMSIZ);
            TLV_ENCODE_PUT (msg->ifname, INTERFACE_NAMSIZ);
            break;
          case NSM_LINK_CTYPE_FLAGS:
            nsm_encode_tlv (pnt, size, i, 4);
            TLV_ENCODE_PUTL (msg->flags);
            break;
          case NSM_LINK_CTYPE_STATUS:
            nsm_encode_tlv (pnt, size, i, 4);
            TLV_ENCODE_PUTL (msg->status);
            break;
          case NSM_LINK_CTYPE_METRIC:
            nsm_encode_tlv (pnt, size, i, 4);
            TLV_ENCODE_PUTL (msg->metric);
            break;
          case NSM_LINK_CTYPE_MTU:
            nsm_encode_tlv (pnt, size, i, 4);
            TLV_ENCODE_PUTL (msg->mtu);
            break;
          case NSM_LINK_CTYPE_DUPLEX:
            nsm_encode_tlv (pnt, size, i, 4);
            TLV_ENCODE_PUTL (msg->duplex);
            break;
          case NSM_LINK_CTYPE_ARP_AGEING_TIMEOUT:
            nsm_encode_tlv (pnt, size, i, 4);
            TLV_ENCODE_PUTL (msg->arp_ageing_timeout);
            break;
          case NSM_LINK_CTYPE_AUTONEGO:
            nsm_encode_tlv (pnt, size, i, 4);
            TLV_ENCODE_PUTL (msg->autonego);
            break;
          case NSM_LINK_CTYPE_BANDWIDTH:
            nsm_encode_tlv (pnt, size, i, 4);
            TLV_ENCODE_PUTF (msg->bandwidth);
            break;
#ifdef HAVE_TE
          case NSM_LINK_CTYPE_MAX_RESV_BANDWIDTH:
            nsm_encode_tlv (pnt, size, i, 4);
            TLV_ENCODE_PUTF (msg->max_resv_bw);
            break;
#ifdef HAVE_DSTE
          case NSM_LINK_CTYPE_BC_MODE:
            nsm_encode_tlv (pnt, size, i, 1);
            TLV_ENCODE_PUTC (msg->bc_mode);
            break;
          case NSM_LINK_CTYPE_BW_CONSTRAINT:
            {
              int j;

              nsm_encode_tlv (pnt, size, i, 32);
              for (j = 0; j < MAX_BW_CONST; j++)
                TLV_ENCODE_PUTF (msg->bw_constraint[j]);
            }
            break;
#endif /* HAVE_DSTE */
          case NSM_LINK_CTYPE_ADMIN_GROUP:
            nsm_encode_tlv (pnt, size, i, 4);
            TLV_ENCODE_PUTL (msg->admin_group);
            break;
#endif /* HAVE_TE */
#ifdef HAVE_MPLS
          case NSM_LINK_CTYPE_LABEL_SPACE:
            nsm_encode_tlv (pnt, size, i, 11);
            TLV_ENCODE_PUTC (msg->lp.status);
            TLV_ENCODE_PUTW (msg->lp.label_space);
            TLV_ENCODE_PUTL (msg->lp.min_label_value);
            TLV_ENCODE_PUTL (msg->lp.max_label_value);
            break;
#endif /* HAVE_MPLS */
          case NSM_LINK_CTYPE_HW_ADDRESS:
            nsm_encode_tlv (pnt, size, i,msg->hw_addr_len + 6);
            TLV_ENCODE_PUTW (msg->hw_type);
            TLV_ENCODE_PUTL (msg->hw_addr_len);
            TLV_ENCODE_PUT (msg->hw_addr, msg->hw_addr_len);
            break;
#ifdef HAVE_TE
          case NSM_LINK_CTYPE_PRIORITY_BW:
            {
              int j;

              nsm_encode_tlv (pnt, size, i, 32);

              for (j = 0; j < MAX_PRIORITIES; j++)
                TLV_ENCODE_PUTF (msg->tecl_priority_bw[j]);
            }
            break;
#endif /* HAVE_TE */
#ifdef HAVE_VPLS
          case NSM_LINK_CTYPE_BINDING:
            nsm_encode_tlv (pnt, size, i, 1);
            TLV_ENCODE_PUTC (msg->bind);
            break;
#endif /* HAVE_VPLS */
#ifdef HAVE_VRX
          case NSM_LINK_CTYPE_VRX_FLAG:
            nsm_encode_tlv (pnt, size, i, 1);
            TLV_ENCODE_PUTC (msg->vrx_flag);
            break;
          case NSM_LINK_CTYPE_VRX_FLAG_LOCALSRC:
            nsm_encode_tlv (pnt, size, i, 1);
            TLV_ENCODE_PUTC (msg->local_flag);
            break;
#endif /* HAVE_VRX */
#ifdef HAVE_LACPD
          case NSM_LINK_CTYPE_LACP_KEY:
            nsm_encode_tlv (pnt, size, i, 8);
            TLV_ENCODE_PUTW (msg->lacp_admin_key);
            TLV_ENCODE_PUTW (msg->agg_param_update);
            TLV_ENCODE_PUTL (msg->lacp_agg_key);
            break;
#endif /* HAVE_LACPD */
          case NSM_LINK_CTYPE_NW_ADDRESS:
            nsm_encode_tlv (pnt, size, i, sizeof (u_int32_t));
            TLV_ENCODE_PUT_IN4_ADDR (&msg->nw_addr);
            break;
#ifdef HAVE_GMPLS
          case NSM_LINK_CTYPE_GMPLS_TYPE:
            nsm_encode_tlv (pnt, size, i, 1);
            TLV_ENCODE_PUTC (msg->gtype);
            break;
#endif /* HAVE_GMPLS */
          default:
            break;
          }
      }
  return *pnt - sp;
}

s_int32_t
nsm_decode_link (u_char **pnt, u_int16_t *size,
                 struct nsm_msg_link *msg)
{
  struct nsm_tlv_header tlv;
  u_char *sp = *pnt;

  /* Check size.  */
  if (*size < NSM_MSG_LINK_SIZE)
    return NSM_ERR_PKT_TOO_SMALL;

  /* Interface index.  */
  TLV_DECODE_GETL (msg->ifindex);

#ifdef HAVE_GMPLS
  /* Interface index.  */
  TLV_DECODE_GETL (msg->gifindex);
#endif /*HAVE_GMPLS*/

  /* Interface TLV parse.  */
  while (*size)
    {
      if (*size < NSM_TLV_HEADER_SIZE)
        return NSM_ERR_PKT_TOO_SMALL;

      NSM_DECODE_TLV_HEADER (tlv);

      switch (tlv.type)
        {
        case NSM_LINK_CTYPE_NAME:
          msg->ifname = *pnt;
          TLV_DECODE_SKIP (tlv.length);
          NSM_SET_CTYPE (msg->cindex, tlv.type);
          break;
        case NSM_LINK_CTYPE_FLAGS:
          TLV_DECODE_GETL (msg->flags);
          NSM_SET_CTYPE (msg->cindex, tlv.type);
          break;
        case NSM_LINK_CTYPE_STATUS:
          TLV_DECODE_GETL (msg->status);
          NSM_SET_CTYPE (msg->cindex, tlv.type);
          break;
        case NSM_LINK_CTYPE_METRIC:
          TLV_DECODE_GETL (msg->metric);
          NSM_SET_CTYPE (msg->cindex, tlv.type);
          break;
        case NSM_LINK_CTYPE_MTU:
          TLV_DECODE_GETL (msg->mtu);
          NSM_SET_CTYPE (msg->cindex, tlv.type);
          break;
        case NSM_LINK_CTYPE_DUPLEX:
          TLV_DECODE_GETL (msg->duplex);
          NSM_SET_CTYPE (msg->cindex, tlv.type);
          break;
        case NSM_LINK_CTYPE_ARP_AGEING_TIMEOUT:
          TLV_DECODE_GETL (msg->arp_ageing_timeout);
          NSM_SET_CTYPE (msg->cindex, tlv.type);
          break;
        case NSM_LINK_CTYPE_AUTONEGO:
          TLV_DECODE_GETL (msg->autonego);
          NSM_SET_CTYPE (msg->cindex, tlv.type);
          break;
        case NSM_LINK_CTYPE_BANDWIDTH:
          TLV_DECODE_GETF (msg->bandwidth);
          NSM_SET_CTYPE (msg->cindex, tlv.type);
          break;
#ifdef HAVE_TE
        case NSM_LINK_CTYPE_MAX_RESV_BANDWIDTH:
          TLV_DECODE_GETF (msg->max_resv_bw);
          NSM_SET_CTYPE (msg->cindex, tlv.type);
          break;
#ifdef HAVE_DSTE
        case NSM_LINK_CTYPE_BC_MODE:
          TLV_DECODE_GETC (msg->bc_mode);
          NSM_SET_CTYPE (msg->cindex, tlv.type);
          break;

        case NSM_LINK_CTYPE_BW_CONSTRAINT:
          {
            int i;

            for (i = 0; i < MAX_BW_CONST; i++)
              TLV_DECODE_GETF (msg->bw_constraint[i]);

            NSM_SET_CTYPE (msg->cindex, tlv.type);
          }
          break;
#endif /* HAVE_DSTE */
        case NSM_LINK_CTYPE_ADMIN_GROUP:
          TLV_DECODE_GETL (msg->admin_group);
          NSM_SET_CTYPE (msg->cindex, tlv.type);
          break;
#endif /* HAVE_TE */
#ifdef HAVE_MPLS
        case NSM_LINK_CTYPE_LABEL_SPACE:
          TLV_DECODE_GETC (msg->lp.status);
          TLV_DECODE_GETW (msg->lp.label_space);
          TLV_DECODE_GETL (msg->lp.min_label_value);
          TLV_DECODE_GETL (msg->lp.max_label_value);
          NSM_SET_CTYPE (msg->cindex, tlv.type);
          break;
#endif /* HAVE_MPLS */
        case NSM_LINK_CTYPE_HW_ADDRESS:
          TLV_DECODE_GETW (msg->hw_type);
          TLV_DECODE_GETL (msg->hw_addr_len);
          TLV_DECODE_GET (msg->hw_addr, msg->hw_addr_len);
          NSM_SET_CTYPE (msg->cindex, tlv.type);
          break;
#ifdef HAVE_TE
        case NSM_LINK_CTYPE_PRIORITY_BW:
          {
            int i;

            for (i = 0; i < MAX_PRIORITIES; i++)
              TLV_DECODE_GETF (msg->tecl_priority_bw[i]);

            NSM_SET_CTYPE (msg->cindex, tlv.type);
          }
          break;
#endif /* HAVE_TE */
#ifdef HAVE_VPLS
        case NSM_LINK_CTYPE_BINDING:
          TLV_DECODE_GETC (msg->bind);
          NSM_SET_CTYPE (msg->cindex, tlv.type);
          break;
#endif /* HAVE_VPLS */
#ifdef HAVE_VRX
        case NSM_LINK_CTYPE_VRX_FLAG:
          TLV_DECODE_GETC (msg->vrx_flag);
          NSM_SET_CTYPE (msg->cindex, tlv.type);
          break;
        case NSM_LINK_CTYPE_VRX_FLAG_LOCALSRC:
          TLV_DECODE_GETC (msg->local_flag);
          NSM_SET_CTYPE (msg->cindex, tlv.type);
          break;
#endif /* HAVE_VRX */
#ifdef HAVE_LACPD
        case NSM_LINK_CTYPE_LACP_KEY:
          TLV_DECODE_GETW (msg->lacp_admin_key);
          TLV_DECODE_GETW (msg->agg_param_update);
          TLV_DECODE_GETL (msg->lacp_agg_key);
          NSM_SET_CTYPE (msg->cindex, tlv.type);
          break;
#endif /* HAVE_LACPD */
        case NSM_LINK_CTYPE_NW_ADDRESS:
          TLV_DECODE_GET_IN4_ADDR (&msg->nw_addr);
          NSM_SET_CTYPE (msg->cindex, tlv.type);
          break;
#ifdef HAVE_GMPLS
        case NSM_LINK_CTYPE_GMPLS_TYPE:
          TLV_DECODE_GETC (msg->gtype);
          NSM_SET_CTYPE (msg->cindex, tlv.type);
          break;
#endif /* HAVE_GMPLS */
        default:
          TLV_DECODE_SKIP (tlv.length);
          break;
        }
    }
  return *pnt - sp;
}

s_int32_t
nsm_encode_vr_bind (u_char **pnt, u_int16_t *size,
                    struct nsm_msg_vr_bind *msg)
{
  u_char *sp = *pnt;

  /* Check size. */
  if (*size < NSM_MSG_VR_BIND_SIZE)
    return NSM_ERR_PKT_TOO_SMALL;

  /* VR ID. */
  TLV_ENCODE_PUTL (msg->vr_id);

  /* Interface index. */
  TLV_ENCODE_PUTL (msg->ifindex);

  return *pnt - sp;
}

s_int32_t
nsm_decode_vr_bind (u_char **pnt, u_int16_t *size,
                    struct nsm_msg_vr_bind *msg)
{
  u_char *sp = *pnt;

  /* Check size. */
  if (*size < NSM_MSG_VR_BIND_SIZE)
    return NSM_ERR_PKT_TOO_SMALL;

  /* VR ID. */
  TLV_DECODE_GETL (msg->vr_id);

  /* Interface index. */
  TLV_DECODE_GETL (msg->ifindex);

  return *pnt - sp;
}

s_int32_t
nsm_encode_vrf_bind (u_char **pnt, u_int16_t *size,
                     struct nsm_msg_vrf_bind *msg)
{
  u_char *sp = *pnt;

  /* Check size. */
  if (*size < NSM_MSG_VRF_BIND_SIZE)
    return NSM_ERR_PKT_TOO_SMALL;

  /* VR ID. */
  TLV_ENCODE_PUTL (msg->vrf_id);

  /* Interface index. */
  TLV_ENCODE_PUTL (msg->ifindex);

  return *pnt - sp;
}

s_int32_t
nsm_decode_vrf_bind (u_char **pnt, u_int16_t *size,
                     struct nsm_msg_vrf_bind *msg)
{
  u_char *sp = *pnt;

  /* Check size. */
  if (*size < NSM_MSG_VRF_BIND_SIZE)
    return NSM_ERR_PKT_TOO_SMALL;

  /* VR ID. */
  TLV_DECODE_GETL (msg->vrf_id);

  /* Interface index. */
  TLV_DECODE_GETL (msg->ifindex);

  return *pnt - sp;
}

#ifdef HAVE_DSTE
s_int32_t
nsm_encode_dste_te_class_update (u_char **pnt, u_int16_t *size,
                                 struct nsm_msg_dste_te_class_update *msg)
{
  u_char i;
  u_char *sp = *pnt;

  TLV_ENCODE_PUTC (msg->te_class_index);
  TLV_ENCODE_PUTC (msg->te_class.ct_num);
  TLV_ENCODE_PUTC (msg->te_class.priority);
  for (i = 0; i <= MAX_CT_NAME_LEN; i++)
    TLV_ENCODE_PUTC (msg->ct_name[i]);

  return *pnt - sp;
}

s_int32_t
nsm_decode_dste_te_class_update (u_char **pnt, u_int16_t *size,
                                 struct nsm_msg_dste_te_class_update *msg)
{
  u_char i;
  u_char *sp = *pnt;

  TLV_DECODE_GETC (msg->te_class_index);
  TLV_DECODE_GETC (msg->te_class.ct_num);
  TLV_DECODE_GETC (msg->te_class.priority);

  for (i = 0; i <= MAX_CT_NAME_LEN; i++)
    TLV_DECODE_GETC (msg->ct_name[i]);

  return *pnt - sp;
}

/* Parse Diffserv Supported DSCP message.  */
s_int32_t
nsm_parse_dste_te_class_update (u_char **pnt, u_int16_t *size,
                                struct nsm_msg_header *header, void *arg,
                                NSM_CALLBACK callback)
{
  struct nsm_msg_dste_te_class_update msg;
  int ret;

  ret = nsm_decode_dste_te_class_update (pnt, size, &msg);
  if (ret < 0)
    return ret;

  if (callback)
    {
      ret = (*callback) (header, arg, &msg);
      if (ret < 0)
        return ret;
    }
  return 0;
}
#endif /* HAVE_DSTE */

s_int32_t
nsm_encode_address (u_char **pnt, u_int16_t *size,
                    struct nsm_msg_address *msg)
{
  u_char *sp = *pnt;

  /* Check size.  */
  if (*size < NSM_MSG_ADDRESS_SIZE)
    return NSM_ERR_PKT_TOO_SMALL;

  /* Interface index.  */
  TLV_ENCODE_PUTL (msg->ifindex);

  /* Flags, prefixlen and AFI.  */
  TLV_ENCODE_PUTC (msg->flags);
  TLV_ENCODE_PUTC (msg->prefixlen);
  TLV_ENCODE_PUTW (msg->afi);

  if (msg->afi == AFI_IP)
    {
      TLV_ENCODE_PUT_IN4_ADDR (&msg->u.ipv4.src);
      TLV_ENCODE_PUT_IN4_ADDR (&msg->u.ipv4.dst);
    }
#ifdef HAVE_IPV6
  else if (msg->afi == AFI_IP6)
    {
      TLV_ENCODE_PUT_IN6_ADDR (&msg->u.ipv6.src);
      TLV_ENCODE_PUT_IN6_ADDR (&msg->u.ipv6.dst);
    }
#endif /* HAVE_IPV6 */

  return *pnt - sp;
}

s_int32_t
nsm_decode_address (u_char **pnt, u_int16_t *size,
                    struct nsm_msg_address *msg)
{
  u_char *sp = *pnt;

  /* Check size.  */
  if (*size < NSM_MSG_ADDRESS_SIZE)
    return NSM_ERR_PKT_TOO_SMALL;

  /* Interface index.  */
  TLV_DECODE_GETL (msg->ifindex);
  TLV_DECODE_GETC (msg->flags);
  TLV_DECODE_GETC (msg->prefixlen);
  TLV_DECODE_GETW (msg->afi);

  if (msg->afi == AFI_IP)
    {
      TLV_DECODE_GET_IN4_ADDR (&msg->u.ipv4.src);
      TLV_DECODE_GET_IN4_ADDR (&msg->u.ipv4.dst);
    }
#ifdef HAVE_IPV6
  else if (msg->afi == AFI_IP6)
    {
      TLV_DECODE_GET_IN6_ADDR (&msg->u.ipv6.src);
      TLV_DECODE_GET_IN6_ADDR (&msg->u.ipv6.dst);
    }
#endif /* HAVE_IPV6 */
  else
    return NSM_ERR_INVALID_AFI;

  return *pnt - sp;
}

s_int32_t
nsm_encode_route_ipv4 (u_char **pnt, u_int16_t *size,
                       struct nsm_msg_route_ipv4 *msg)
{
  int i;
  u_char *sp = *pnt;
  u_int16_t length;
  /* For remeber length part.  */
  u_char *lp;
  struct nsm_tlv_ipv4_nexthop *nexthop;

  /* Check size.  */
  if (*size < NSM_MSG_ROUTE_IPV4_SIZE)
    return NSM_ERR_PKT_TOO_SMALL;

  /* Flags.  */
  TLV_ENCODE_PUTW (msg->flags);

  /* Total length of this message.  First we put place holder then
     fill in later on.  */
  lp = *pnt;
  TLV_ENCODE_PUT_EMPTY (2);

  /* Type. */
  TLV_ENCODE_PUTC (msg->type);
  TLV_ENCODE_PUTC (msg->prefixlen);
  TLV_ENCODE_PUTC (msg->distance);
  TLV_ENCODE_PUTC (msg->sub_type);

  TLV_ENCODE_PUTL (msg->metric);
  TLV_ENCODE_PUT_IN4_ADDR (&msg->prefix);

#ifdef HAVE_MPLS
  if (NSM_CHECK_CTYPE (msg->cindex, NSM_ROUTE_CTYPE_MPLS_IPV4_NHOP))
    {
      nsm_encode_tlv (pnt, size, NSM_ROUTE_CTYPE_MPLS_IPV4_NHOP, 18);

      TLV_ENCODE_PUTC (msg->mpls_flags);
      TLV_ENCODE_PUTC (msg->mpls_out_protocol);
      TLV_ENCODE_PUTL (msg->mpls_oif_ix);
      TLV_ENCODE_PUTL (msg->mpls_out_label);
      TLV_ENCODE_PUT_IN4_ADDR (&msg->mpls_nh_addr);
      TLV_ENCODE_PUTL (msg->mpls_ftn_ix);
    }
#endif /* HAVE_MPLS */

  /* Encode nexthop. */
  if (NSM_CHECK_CTYPE (msg->cindex, NSM_ROUTE_CTYPE_IPV4_NEXTHOP))
    {
      if (msg->nexthop_num <= NSM_TLV_IPV4_NEXTHOP_NUM)
        nexthop = msg->nexthop;
      else
        nexthop = msg->nexthop_opt;

      for (i = 0; i < msg->nexthop_num; i++)
        {
          nsm_encode_tlv (pnt, size, NSM_ROUTE_CTYPE_IPV4_NEXTHOP, 8);
          TLV_ENCODE_PUTL (nexthop[i].ifindex);
          TLV_ENCODE_PUT_IN4_ADDR (&nexthop[i].addr);
        }
    }

  /* Encode tag.  */
  if (NSM_CHECK_CTYPE (msg->cindex, NSM_ROUTE_CTYPE_IPV4_TAG))
    {
      nsm_encode_tlv (pnt, size, NSM_ROUTE_CTYPE_IPV4_TAG, 4);
      TLV_ENCODE_PUTL (msg->tag);
    }

  /* Encode process id.  */
  if (NSM_CHECK_CTYPE (msg->cindex, NSM_ROUTE_CTYPE_IPV4_PROCESS_ID))
    {
      nsm_encode_tlv (pnt, size, NSM_ROUTE_CTYPE_IPV4_PROCESS_ID, 4);
      TLV_ENCODE_PUTL (msg->pid);
    }

#ifdef HAVE_VRF
  /* Encode domain id. */
  if (NSM_CHECK_CTYPE (msg->cindex, NSM_ROUTE_CTYPE_DOMAIN_INFO))
    {
      nsm_encode_tlv (pnt, size, NSM_ROUTE_CTYPE_DOMAIN_INFO, 14);
      TLV_ENCODE_PUT (&msg->domain_info.domain_id, 8);
      TLV_ENCODE_PUT_IN4_ADDR (&msg->domain_info.area_id);
      TLV_ENCODE_PUTC (msg->domain_info.r_type);
      TLV_ENCODE_PUTC (msg->domain_info.metric_type_E2);
      if (NSM_CHECK_CTYPE (msg->cindex, NSM_ROUTE_CTYPE_OSPF_ROUTER_ID))
        {
          nsm_encode_tlv (pnt, size, NSM_ROUTE_CTYPE_OSPF_ROUTER_ID, 4);
          TLV_ENCODE_PUT_IN4_ADDR (&msg->domain_info.router_id);
        }
    }
#endif /* HAVE_VRF */

  /* Fill in real length.  */
  length = *pnt - sp;
  length = pal_hton16 (length);
  pal_mem_cpy (lp, &length, 2);

  return *pnt - sp;
}

s_int32_t
nsm_decode_route_ipv4 (u_char **pnt, u_int16_t *size,
                       struct nsm_msg_route_ipv4 *msg)
{
  int i;
  u_char *sp = *pnt;
  u_int16_t length;
  u_int16_t diff;
  struct nsm_tlv_header tlv;

  /* Check size.  */
  if (*size < NSM_MSG_ROUTE_IPV4_SIZE)
    return NSM_ERR_PKT_TOO_SMALL;

  /* Flags.  */
  TLV_DECODE_GETW (msg->flags);

  /* Total length of this message.  */
  TLV_DECODE_GETW (length);
  length -= NSM_MSG_ROUTE_IPV4_SIZE;

  /* Type. */
  TLV_DECODE_GETC (msg->type);
  TLV_DECODE_GETC (msg->prefixlen);
  TLV_DECODE_GETC (msg->distance);
  TLV_DECODE_GETC (msg->sub_type);

  TLV_DECODE_GETL (msg->metric);
  TLV_DECODE_GET_IN4_ADDR (&msg->prefix);

  /* TLV parse. */
  while (length)
    {
      if (length < NSM_TLV_HEADER_SIZE)
        return NSM_ERR_PKT_TOO_SMALL;

      diff = *size;

      NSM_DECODE_TLV_HEADER (tlv);

      switch (tlv.type)
        {
#ifdef HAVE_MPLS
        case NSM_ROUTE_CTYPE_MPLS_IPV4_NHOP:
          TLV_DECODE_GETC (msg->mpls_flags);
          TLV_DECODE_GETC (msg->mpls_out_protocol);
          TLV_DECODE_GETL (msg->mpls_oif_ix);
          TLV_DECODE_GETL (msg->mpls_out_label);
          TLV_DECODE_GET_IN4_ADDR (&msg->mpls_nh_addr);
          TLV_DECODE_GETL (msg->mpls_ftn_ix);

          NSM_SET_CTYPE (msg->cindex, tlv.type);
          break;
#endif /* HAVE_MPLS */
        case NSM_ROUTE_CTYPE_IPV4_NEXTHOP:
          i = msg->nexthop_num;
          if (i >= NSM_TLV_IPV4_NEXTHOP_NUM)
            {
              if (i == NSM_TLV_IPV4_NEXTHOP_NUM)
                {
                  msg->nexthop_opt
                    = XCALLOC (MTYPE_NSM_MSG_NEXTHOP_IPV4,
                               sizeof (struct nsm_tlv_ipv4_nexthop) * (i + 1));
                  pal_mem_cpy (msg->nexthop_opt, msg->nexthop,
                               sizeof (struct nsm_tlv_ipv4_nexthop) * i);
                }
              else
                msg->nexthop_opt
                  = XREALLOC (MTYPE_NSM_MSG_NEXTHOP_IPV4,
                              msg->nexthop_opt,
                              sizeof (struct nsm_tlv_ipv4_nexthop) * (i + 1));

              TLV_DECODE_GETL (msg->nexthop_opt[i].ifindex);
              TLV_DECODE_GET_IN4_ADDR (&msg->nexthop_opt[i].addr);
            }
          else
            {
              TLV_DECODE_GETL (msg->nexthop[i].ifindex);
              TLV_DECODE_GET_IN4_ADDR (&msg->nexthop[i].addr);
            }
          NSM_SET_CTYPE (msg->cindex, tlv.type);
          msg->nexthop_num++;
          break;
        case NSM_ROUTE_CTYPE_IPV4_TAG:
          TLV_DECODE_GETL (msg->tag);
          NSM_SET_CTYPE (msg->cindex, tlv.type);
          break;
        case NSM_ROUTE_CTYPE_IPV4_PROCESS_ID:
          TLV_DECODE_GETL (msg->pid);
          NSM_SET_CTYPE (msg->cindex, tlv.type);
          break;
#ifdef HAVE_VRF
        case NSM_ROUTE_CTYPE_DOMAIN_INFO:
   	  TLV_DECODE_GET (msg->domain_info.domain_id, 8);
          TLV_DECODE_GET_IN4_ADDR (&msg->domain_info.area_id);
          TLV_DECODE_GETC (msg->domain_info.r_type);
          TLV_DECODE_GETC (msg->domain_info.metric_type_E2);

          NSM_SET_CTYPE (msg->cindex, tlv.type);
	  break;
        case NSM_ROUTE_CTYPE_OSPF_ROUTER_ID:
          TLV_DECODE_GET_IN4_ADDR (&msg->domain_info.router_id);
          NSM_SET_CTYPE (msg->cindex, tlv.type);
          break;
#endif /* HAVE_VRF */
        default:
          TLV_DECODE_SKIP (tlv.length);
          break;
        }
      length -= (diff - *size);
    }
  return *pnt - sp;
}

void
nsm_finish_route_ipv4 (struct nsm_msg_route_ipv4 *msg)
{
  if (msg->nexthop_num > NSM_TLV_IPV4_NEXTHOP_NUM && msg->nexthop_opt)
    XFREE (MTYPE_NSM_MSG_NEXTHOP_IPV4, msg->nexthop_opt);
}

#ifdef HAVE_IPV6
s_int32_t
nsm_encode_route_ipv6 (u_char **pnt, u_int16_t *size,
                       struct nsm_msg_route_ipv6 *msg)
{
  int i;
  u_char *sp = *pnt;
  u_int16_t length;
  /* For remeber length part.  */
  u_char *lp;
  struct nsm_tlv_ipv6_nexthop *nexthop;

  /* Check size.  */
  if (*size < NSM_MSG_ROUTE_IPV6_SIZE)
    return NSM_ERR_PKT_TOO_SMALL;

  /* Flags.  */
  TLV_ENCODE_PUTW (msg->flags);

  /* Total length of this message.  First we put place holder then
     fill in later on.  */
  lp = *pnt;
  TLV_ENCODE_PUT_EMPTY (2);

  /* Type. */
  TLV_ENCODE_PUTC (msg->type);
  TLV_ENCODE_PUTC (msg->prefixlen);
  TLV_ENCODE_PUTC (msg->distance);
  TLV_ENCODE_PUTC (msg->sub_type);

  TLV_ENCODE_PUTL (msg->metric);
  TLV_ENCODE_PUT_IN6_ADDR (&msg->prefix);

#ifdef HAVE_MPLS
  if (NSM_CHECK_CTYPE (msg->cindex, NSM_ROUTE_CTYPE_MPLS_IPV6_NHOP))
    {
      nsm_encode_tlv (pnt, size, NSM_ROUTE_CTYPE_MPLS_IPV6_NHOP, 24);

      TLV_ENCODE_PUTL (msg->mpls_oif_ix);
      TLV_ENCODE_PUTL (msg->mpls_out_label);
      TLV_ENCODE_PUT_IN6_ADDR (&msg->mpls_nh_addr);
    }
#endif /* HAVE_MPLS */

  /* Encode nexthop. */
  if (NSM_CHECK_CTYPE (msg->cindex, NSM_ROUTE_CTYPE_IPV6_NEXTHOP))
    {
      if (msg->nexthop_num <= NSM_TLV_IPV6_NEXTHOP_NUM)
        nexthop = msg->nexthop;
      else
        nexthop = msg->nexthop_opt;

      for (i = 0; i < msg->nexthop_num; i++)
        {
          nsm_encode_tlv (pnt, size, NSM_ROUTE_CTYPE_IPV6_NEXTHOP, 20);
          TLV_ENCODE_PUTL (nexthop[i].ifindex);
          TLV_ENCODE_PUT_IN6_ADDR (&nexthop[i].addr);
        }
    }

  /* Encode tag.  */
  if (NSM_CHECK_CTYPE (msg->cindex, NSM_ROUTE_CTYPE_IPV6_TAG))
    {
      nsm_encode_tlv (pnt, size, NSM_ROUTE_CTYPE_IPV6_TAG, 4);
      TLV_ENCODE_PUTL (msg->tag);
    }

  /* Encode process id.  */
  if (NSM_CHECK_CTYPE (msg->cindex, NSM_ROUTE_CTYPE_IPV6_PROCESS_ID))
    {
      nsm_encode_tlv (pnt, size, NSM_ROUTE_CTYPE_IPV6_PROCESS_ID, 4);
      TLV_ENCODE_PUTL (msg->pid);
    }

  /* Fill in real length.  */
  length = *pnt - sp;
  length = pal_hton16 (length);
  pal_mem_cpy (lp, &length, 2);

  return *pnt - sp;
}

s_int32_t
nsm_decode_route_ipv6 (u_char **pnt, u_int16_t *size,
                       struct nsm_msg_route_ipv6 *msg)
{
  int i;
  u_char *sp = *pnt;
  u_int16_t length;
  u_int16_t diff;
  struct nsm_tlv_header tlv;

  /* Check size.  */
  if (*size < NSM_MSG_ROUTE_IPV6_SIZE)
    return NSM_ERR_PKT_TOO_SMALL;

  /* Flags.  */
  TLV_DECODE_GETW (msg->flags);

  /* Total length of this message.  */
  TLV_DECODE_GETW (length);
  length -= NSM_MSG_ROUTE_IPV6_SIZE;

  /* Type. */
  TLV_DECODE_GETC (msg->type);
  TLV_DECODE_GETC (msg->prefixlen);
  TLV_DECODE_GETC (msg->distance);
  TLV_DECODE_GETC (msg->sub_type);

  TLV_DECODE_GETL (msg->metric);
  TLV_DECODE_GET_IN6_ADDR (&msg->prefix);

  /* TLV parse. */
  while (length)
    {
      if (length < NSM_TLV_HEADER_SIZE)
        return NSM_ERR_PKT_TOO_SMALL;

      diff = *size;

      NSM_DECODE_TLV_HEADER (tlv);

      switch (tlv.type)
        {
#ifdef HAVE_MPLS
        case NSM_ROUTE_CTYPE_MPLS_IPV6_NHOP:
          TLV_DECODE_GETL (msg->mpls_oif_ix);
          TLV_DECODE_GETL (msg->mpls_out_label);
          TLV_DECODE_GET_IN6_ADDR (&msg->mpls_nh_addr);

          NSM_SET_CTYPE (msg->cindex, tlv.type);
          break;
#endif /* HAVE_MPLS */
        case NSM_ROUTE_CTYPE_IPV6_NEXTHOP:
          i = msg->nexthop_num;
          if (i >= NSM_TLV_IPV6_NEXTHOP_NUM)
            {
              if (i == NSM_TLV_IPV6_NEXTHOP_NUM)
                {
                  msg->nexthop_opt
                    = XCALLOC (MTYPE_NSM_MSG_NEXTHOP_IPV6,
                               sizeof (struct nsm_tlv_ipv6_nexthop) * (i + 1));
                  pal_mem_cpy (msg->nexthop_opt, msg->nexthop,
                               sizeof (struct nsm_tlv_ipv6_nexthop) * i);
                }
              else
                msg->nexthop_opt
                  = XREALLOC (MTYPE_NSM_MSG_NEXTHOP_IPV6,
                              msg->nexthop_opt,
                              sizeof (struct nsm_tlv_ipv6_nexthop) * (i + 1));

              TLV_DECODE_GETL (msg->nexthop_opt[i].ifindex);
              TLV_DECODE_GET_IN6_ADDR (&msg->nexthop_opt[i].addr);
            }
          else
            {
              TLV_DECODE_GETL (msg->nexthop[i].ifindex);
              TLV_DECODE_GET_IN6_ADDR (&msg->nexthop[i].addr);
            }
          NSM_SET_CTYPE (msg->cindex, tlv.type);
          msg->nexthop_num++;
          break;
        case NSM_ROUTE_CTYPE_IPV6_TAG:
          TLV_DECODE_GETL (msg->tag);
          NSM_SET_CTYPE (msg->cindex, tlv.type);
          break;
        case NSM_ROUTE_CTYPE_IPV6_PROCESS_ID:
          TLV_DECODE_GETL (msg->pid);
          NSM_SET_CTYPE (msg->cindex, tlv.type);
          break;
        default:
          TLV_DECODE_SKIP (tlv.length);
          break;
        }
      length -= (diff - *size);
    }
  return *pnt - sp;
}

void
nsm_finish_route_ipv6 (struct nsm_msg_route_ipv6 *msg)
{
  if (msg->nexthop_num > NSM_TLV_IPV6_NEXTHOP_NUM && msg->nexthop_opt)
    XFREE (MTYPE_NSM_MSG_NEXTHOP_IPV6, msg->nexthop_opt);
}
#endif /* HAVE_IPV6 */

s_int32_t
nsm_encode_redistribute (u_char **pnt, u_int16_t *size,
                         struct nsm_msg_redistribute *msg)
{
  u_char *sp = *pnt;

  /* Check size.  */
  if (*size < NSM_MSG_REDISTRIBUTE_SIZE)
    return NSM_ERR_PKT_TOO_SMALL;

  /* Route type.  */
  TLV_ENCODE_PUTC (msg->type);

#ifdef HAVE_IPV6
  if (NSM_CHECK_CTYPE (msg->cindex, NSM_ROUTE_CTYPE_IPV6_PROCESS_ID))
    {
      nsm_encode_tlv (pnt, size, NSM_ROUTE_CTYPE_IPV6_PROCESS_ID, 4);
      TLV_ENCODE_PUTL (msg->pid);
    }
  else
#endif /* HAVE_IPV6 */
  if (NSM_CHECK_CTYPE (msg->cindex, NSM_ROUTE_CTYPE_IPV4_PROCESS_ID))
    {
      nsm_encode_tlv (pnt, size, NSM_ROUTE_CTYPE_IPV4_PROCESS_ID, 4);
      TLV_ENCODE_PUTL (msg->pid);
    }
  else
    nsm_encode_tlv (pnt, size, NSM_ROUTE_CTYPE_DEFAULT, 4);

  TLV_ENCODE_PUTW (msg->afi);
  return *pnt - sp;
}

s_int32_t
nsm_decode_redistribute (u_char **pnt, u_int16_t *size,
                         struct nsm_msg_redistribute *msg)
{
  u_char *sp = *pnt;
  struct nsm_tlv_header tlv;

  /* Check size.  */
  if (*size < NSM_MSG_REDISTRIBUTE_SIZE)
    return NSM_ERR_PKT_TOO_SMALL;

  /* Interface index.  */
  TLV_DECODE_GETC (msg->type);
  NSM_DECODE_TLV_HEADER (tlv);

  if (tlv.type == NSM_ROUTE_CTYPE_IPV4_PROCESS_ID)
    {
      TLV_DECODE_GETL (msg->pid);
      NSM_SET_CTYPE (msg->cindex, tlv.type);
    }

#ifdef HAVE_IPV6
  if (tlv.type == NSM_ROUTE_CTYPE_IPV6_PROCESS_ID)
    {
      TLV_DECODE_GETL (msg->pid);
      NSM_SET_CTYPE (msg->cindex, tlv.type);
    }
#endif /* HAVE_IPV6 */

  TLV_DECODE_GETW (msg->afi);

  return *pnt - sp;
}

s_int32_t
nsm_encode_bgp_conv (u_char **pnt, u_int16_t *size,
                     struct nsm_msg_wait_for_bgp *msg)
{
 u_char *sp = *pnt;

  /* Check size.  */
  if (*size < NSM_MSG_WAIT_FOR_BGP_SIZE)
    return NSM_ERR_PKT_TOO_SMALL;

  /* flag set/unset */
  TLV_ENCODE_PUTW (msg->flag);

  return *pnt - sp;
}

s_int32_t
nsm_decode_bgp_conv (u_char **pnt, u_int16_t *size,
                     struct nsm_msg_wait_for_bgp *msg)
{
  u_char *sp = *pnt;

  if (*size < NSM_MSG_WAIT_FOR_BGP_SIZE)
    return NSM_ERR_PKT_TOO_SMALL;

  TLV_DECODE_GETW (msg->flag);

  return *pnt - sp;
}

s_int32_t
nsm_parse_bgp_conv (u_char **pnt, u_int16_t *size,
                    struct nsm_msg_header *header, void *arg,
                    NSM_CALLBACK callback)
{
  struct nsm_msg_wait_for_bgp msg;
  int ret = 0;

  ret = nsm_decode_bgp_conv (pnt, size, &msg);
  if (ret < 0)
    return ret;

  if (callback)
    ret = (*callback) (header, arg, &msg);

  return ret;
}

s_int32_t
nsm_parse_bgp_up_down (u_char **pnt, u_int16_t *size,
                       struct nsm_msg_header *header, void *arg,
                       NSM_CALLBACK callback)
{
  int ret = 0;

  if (callback)
    ret = (*callback) (header, arg, NULL);

  return ret;
}

s_int32_t
nsm_parse_bgp_conv_done (u_char **pnt, u_int16_t *size,
                    struct nsm_msg_header *header, void *arg,
                    NSM_CALLBACK callback)
{
  struct nsm_msg_wait_for_bgp msg;
  int ret = 0;

  ret = nsm_decode_bgp_conv (pnt, size, &msg);
  if (ret < 0)
    return ret;

  if (callback)
    ret = (*callback) (header, arg, &msg);

  return ret;
}

s_int32_t              
nsm_encode_ldp_session_state (u_char **pnt, u_int16_t *size,
                              struct nsm_msg_ldp_session_state *msg)
{
 u_char *sp = *pnt;

  /* Check size.  */
  if (*size < NSM_MSG_LDP_SESSION_STATE_SIZE)
    return NSM_ERR_PKT_TOO_SMALL;

  TLV_ENCODE_PUTL (msg->state);
  TLV_ENCODE_PUTL (msg->ifindex);

  return *pnt - sp;
}

s_int32_t
nsm_decode_ldp_session_state (u_char **pnt, u_int16_t *size,
                              struct nsm_msg_ldp_session_state *msg)
{
  u_char *sp = *pnt;

  if (*size < NSM_MSG_LDP_SESSION_STATE_SIZE)
    return NSM_ERR_PKT_TOO_SMALL;

  TLV_DECODE_GETL (msg->state);
  TLV_DECODE_GETL (msg->ifindex);

  return *pnt - sp;
}

s_int32_t
nsm_parse_ldp_session_state (u_char **pnt, u_int16_t *size,
                             struct nsm_msg_header *header,
                             void *arg, NSM_CALLBACK callback)
{
  struct nsm_msg_ldp_session_state msg;
  int ret = 0;

  ret = nsm_decode_ldp_session_state (pnt, size, &msg);
  if (ret < 0)
    return ret;

  if (callback)
    ret = (*callback) (header, arg, &msg);

  return ret;
}

s_int32_t
nsm_parse_ldp_up_down  (u_char **pnt, u_int16_t *size,
                       struct nsm_msg_header *header, void *arg,
                       NSM_CALLBACK callback)
{
  int ret = 0;

  if (callback)
    ret = (*callback) (header, arg, NULL);

  return ret;
}

s_int32_t
nsm_encode_ipv4_route_lookup (u_char **pnt, u_int16_t *size,
                              struct nsm_msg_route_lookup_ipv4 *msg)
{
  u_char *sp = *pnt;
  int i;

  /* Check size.  */
  if (*size < NSM_MSG_IPV4_ROUTE_LOOKUP_SIZE)
    return NSM_ERR_PKT_TOO_SMALL;

  /* IPv4 addr.  */
  TLV_ENCODE_PUT_IN4_ADDR (&msg->addr);

  /* Nexthop lookup  TLV parse.  */
  for (i = 0; i < NSM_CINDEX_SIZE; i++)
    if (NSM_CHECK_CTYPE (msg->cindex, i))
      {
        if (*size < NSM_TLV_HEADER_SIZE)
          return NSM_ERR_PKT_TOO_SMALL;

        switch (i)
          {
          case NSM_ROUTE_LOOKUP_CTYPE_PREFIXLEN:
            nsm_encode_tlv (pnt, size, i, 1);
            TLV_ENCODE_PUTC (msg->prefixlen);
            break;
          case NSM_ROUTE_LOOKUP_CTYPE_LSP:
            nsm_encode_tlv (pnt, size, i, 0);
            break;
          case NSM_ROUTE_LOOKUP_CTYPE_LSP_PROTO:
            nsm_encode_tlv (pnt, size, i, 0);
            break;
          default:
            break;
          }
      }

  return *pnt - sp;
}

s_int32_t
nsm_decode_ipv4_route_lookup (u_char **pnt, u_int16_t *size,
                              struct nsm_msg_route_lookup_ipv4 *msg)
{
  u_char *sp = *pnt;
  struct nsm_tlv_header tlv;

  /* Check size.  */
  if (*size < NSM_MSG_IPV4_ROUTE_LOOKUP_SIZE)
    return NSM_ERR_PKT_TOO_SMALL;

  /* IPv4 addr. */
  TLV_DECODE_GET_IN4_ADDR (&msg->addr);

  /* Nexthop lookup TLV parse.  */
  while (*size)
    {
      if (*size < NSM_TLV_HEADER_SIZE)
        return NSM_ERR_PKT_TOO_SMALL;

      NSM_DECODE_TLV_HEADER (tlv);

      switch (tlv.type)
        {
        case NSM_ROUTE_LOOKUP_CTYPE_PREFIXLEN:
          TLV_DECODE_GETC (msg->prefixlen);
          NSM_SET_CTYPE (msg->cindex, tlv.type);
          break;
        case NSM_ROUTE_LOOKUP_CTYPE_LSP:
          NSM_SET_CTYPE (msg->cindex, tlv.type);
          break;
       case NSM_ROUTE_LOOKUP_CTYPE_LSP_PROTO:
          NSM_SET_CTYPE (msg->cindex, tlv.type);
          break;
        default:
          TLV_DECODE_SKIP (tlv.length);
          break;
        }
    }

  return *pnt - sp;
}

#ifdef HAVE_IPV6
s_int32_t
nsm_encode_ipv6_route_lookup (u_char **pnt, u_int16_t *size,
                              struct nsm_msg_route_lookup_ipv6 *msg)
{
  u_char *sp = *pnt;
  int i;

  /* Check size.  */
  if (*size < NSM_MSG_IPV6_ROUTE_LOOKUP_SIZE)
    return NSM_ERR_PKT_TOO_SMALL;

  /* IPv4 addr.  */
  TLV_ENCODE_PUT_IN6_ADDR (&msg->addr);

  /* Nexthop lookup TLV parse.  */
  for (i = 0; i < NSM_CINDEX_SIZE; i++)
    if (NSM_CHECK_CTYPE (msg->cindex, i))
      {
        if (*size < NSM_TLV_HEADER_SIZE)
          return NSM_ERR_PKT_TOO_SMALL;

        switch (i)
          {
          case NSM_ROUTE_LOOKUP_CTYPE_PREFIXLEN:
            nsm_encode_tlv (pnt, size, i, 1);
            TLV_ENCODE_PUTC (msg->prefixlen);
            break;
          case NSM_ROUTE_LOOKUP_CTYPE_LSP:
            nsm_encode_tlv (pnt, size, i, 0);
            break;
          default:
            break;
          }
      }

  return *pnt - sp;
}

s_int32_t
nsm_decode_ipv6_route_lookup (u_char **pnt, u_int16_t *size,
                              struct nsm_msg_route_lookup_ipv6 *msg)
{
  u_char *sp = *pnt;
  struct nsm_tlv_header tlv;

  /* Check size.  */
  if (*size < NSM_MSG_IPV6_ROUTE_LOOKUP_SIZE)
    return NSM_ERR_PKT_TOO_SMALL;

  /* IPv4 addr. */
  TLV_DECODE_GET_IN6_ADDR (&msg->addr);

  /* Nexthop lookup TLV parse.  */
  while (*size)
    {
      if (*size < NSM_TLV_HEADER_SIZE)
        return NSM_ERR_PKT_TOO_SMALL;

      NSM_DECODE_TLV_HEADER (tlv);

      switch (tlv.type)
        {
        case NSM_ROUTE_LOOKUP_CTYPE_PREFIXLEN:
          TLV_DECODE_GETC (msg->prefixlen);
          NSM_SET_CTYPE (msg->cindex, tlv.type);
          break;
        case NSM_ROUTE_LOOKUP_CTYPE_LSP:
          NSM_SET_CTYPE (msg->cindex, tlv.type);
          break;
        default:
          TLV_DECODE_SKIP (tlv.length);
          break;
        }
    }
  return *pnt - sp;
}
#endif /* HAVE_IPV6 */

s_int32_t
nsm_encode_route_manage (u_char **pnt, u_int16_t *size,
                         struct nsm_msg_route_manage *msg)
{
  int i;
  u_char *sp = *pnt;

  /* Interface TLV parse.  */
  for (i = 0; i < NSM_CINDEX_SIZE; i++)
    if (NSM_CHECK_CTYPE (msg->cindex, i))
      {

        if (*size < NSM_TLV_HEADER_SIZE)
          return NSM_ERR_PKT_TOO_SMALL;

        switch (i)
          {
          case NSM_ROUTE_MANAGE_CTYPE_RESTART_TIME:
            nsm_encode_tlv (pnt, size, i, 4);
            TLV_ENCODE_PUTL (msg->restart_time);
            break;
          case NSM_ROUTE_MANAGE_CTYPE_AFI_SAFI:
            nsm_encode_tlv (pnt, size, i, 4);
            TLV_ENCODE_PUTW (msg->afi);
            TLV_ENCODE_PUTC (msg->safi);
            TLV_ENCODE_PUTC (0);
            break;
          case NSM_ROUTE_MANAGE_CTYPE_RESTART_OPTION:
            nsm_encode_tlv (pnt, size, NSM_ROUTE_MANAGE_CTYPE_RESTART_OPTION,
                            msg->restart_length);
            TLV_ENCODE_PUT (msg->restart_val, msg->restart_length);
            break;
          case NSM_ROUTE_MANAGE_CTYPE_PROTOCOL_ID:
            nsm_encode_tlv (pnt, size, i, 4);
            TLV_ENCODE_PUTL (msg->protocol_id);
#if defined HAVE_RESTART && defined HAVE_MPLS
          case NSM_ROUTE_MANAGE_CTYPE_MPLS_FLAGS:
            nsm_encode_tlv (pnt, size, i, 4);
            TLV_ENCODE_PUTL (msg->flags);
            if ((CHECK_FLAG (msg->flags, NSM_MSG_HELPER_NODE_STALE_MARK))
                ||(CHECK_FLAG (msg->flags, NSM_MSG_HELPER_NODE_STALE_UNMARK)))
              {
                TLV_ENCODE_PUTL (msg->oi_ifindex);
              } 
            break;
#endif /* HAVE_RESTART && HAVE_MPLS */
          default:
            break;
          }
      }
  return *pnt - sp;
}

s_int32_t
nsm_decode_route_manage (u_char **pnt, u_int16_t *size,
                         struct nsm_msg_route_manage *msg)
{
  u_char *sp = *pnt;
  struct nsm_tlv_header tlv;

  /* Interface TLV parse.  */
  while (*size)
    {
      if (*size < NSM_TLV_HEADER_SIZE)
        return NSM_ERR_PKT_TOO_SMALL;

      NSM_DECODE_TLV_HEADER (tlv);

      switch (tlv.type)
        {
        case NSM_ROUTE_MANAGE_CTYPE_RESTART_TIME:
          TLV_DECODE_GETL (msg->restart_time);
          NSM_SET_CTYPE (msg->cindex, tlv.type);
          break;
        case NSM_ROUTE_MANAGE_CTYPE_AFI_SAFI:
          TLV_DECODE_GETW (msg->afi);
          TLV_DECODE_GETC (msg->safi);
          TLV_DECODE_SKIP (1);
          NSM_SET_CTYPE (msg->cindex, tlv.type);
          break;
        case NSM_ROUTE_MANAGE_CTYPE_RESTART_OPTION:
          msg->restart_length = tlv.length;
          msg->restart_val = *pnt;
          NSM_SET_CTYPE (msg->cindex, tlv.type);
          TLV_DECODE_SKIP (tlv.length);
          break;
        case NSM_ROUTE_MANAGE_CTYPE_PROTOCOL_ID:
          TLV_DECODE_GETL (msg->protocol_id);
          NSM_SET_CTYPE (msg->cindex, tlv.type);
          break;
#if defined HAVE_RESTART && defined HAVE_MPLS
        case NSM_ROUTE_MANAGE_CTYPE_MPLS_FLAGS:
          TLV_DECODE_GETL (msg->flags);
          NSM_SET_CTYPE (msg->cindex, tlv.type);
          if ((CHECK_FLAG (msg->flags, NSM_MSG_HELPER_NODE_STALE_MARK))
              ||(CHECK_FLAG (msg->flags, NSM_MSG_HELPER_NODE_STALE_UNMARK)))
            {
              TLV_DECODE_GETL (msg->oi_ifindex);
            }
          break;
#endif /* HAVE_RESTART && HAVE_MPLS */
        default:
          TLV_DECODE_SKIP (tlv.length);
          break;
        }
    }
  return *pnt - sp;
}

s_int32_t
nsm_encode_vr (u_char **pnt, u_int16_t *size, struct nsm_msg_vr *msg)
{
  u_char *sp = *pnt;

  /* Check size.  */
  if (*size < NSM_MSG_VR_SIZE)
    return NSM_ERR_PKT_TOO_SMALL;

  TLV_ENCODE_PUTL (msg->vr_id);
  if (msg->len)
    TLV_ENCODE_PUT (msg->name, msg->len + 1); /* Null terminated. */

  return *pnt - sp;
}

s_int32_t
nsm_decode_vr (u_char **pnt, u_int16_t *size,
               struct nsm_msg_vr *msg)
{
  u_char *sp = *pnt;

  /* Check size.  */
  if (*size < NSM_MSG_VR_SIZE)
    return NSM_ERR_PKT_TOO_SMALL;

  /* VR ID. */
  TLV_DECODE_GETL (msg->vr_id);

  /* VR name. */
  msg->len = (*size) - 1;
  if (msg->len)
    msg->name = (char *)*pnt;

  return *pnt - sp;
}

s_int32_t
nsm_encode_vrf (u_char **pnt, u_int16_t *size, struct nsm_msg_vrf *msg)
{
  u_char *sp = *pnt;
  int i;

  /* Check size.  */
  if (*size < NSM_MSG_VRF_SIZE)
    return NSM_ERR_PKT_TOO_SMALL;

  /* VRF ID.  */
  TLV_ENCODE_PUTL (msg->vrf_id);

  /* FIB ID.  */
  TLV_ENCODE_PUTL (msg->fib_id);

  for (i = 0; i < NSM_CINDEX_SIZE; i++)
    if (NSM_CHECK_CTYPE (msg->cindex, i))
      {
        if (*size < NSM_TLV_HEADER_SIZE)
          return NSM_ERR_PKT_TOO_SMALL;

        switch (i)
          {
          case NSM_VRF_CTYPE_NAME:
            nsm_encode_tlv (pnt, size, i, msg->len);
            TLV_ENCODE_PUT (msg->name, msg->len);
            break;
          default:
            break;
          }
      }

  return *pnt - sp;
}

s_int32_t
nsm_decode_vrf (u_char **pnt, u_int16_t *size,
                struct nsm_msg_vrf *msg)
{
  u_char *sp = *pnt;
  struct nsm_tlv_header tlv;

  /* Check size.  */
  if (*size < NSM_MSG_VRF_SIZE)
    return NSM_ERR_PKT_TOO_SMALL;

  /* VRF ID. */
  TLV_DECODE_GETL (msg->vrf_id);

  /* FIB ID.  */
  TLV_DECODE_GETL (msg->fib_id);

  while (*size)
    {
      if (*size < NSM_TLV_HEADER_SIZE)
        return NSM_ERR_PKT_TOO_SMALL;

      NSM_DECODE_TLV_HEADER (tlv);

      switch (tlv.type)
        {
        case NSM_VRF_CTYPE_NAME:
          msg->name = *pnt;
          msg->len = tlv.length;
          TLV_DECODE_SKIP (tlv.length);
          NSM_SET_CTYPE (msg->cindex, tlv.type);
          break;
        default:
          TLV_DECODE_SKIP (tlv.length);
          break;
        }
    }
  return *pnt - sp;
}

#ifndef HAVE_IMI
s_int32_t
nsm_encode_user (u_char **pnt, u_int16_t *size, struct nsm_msg_user *msg)
{
  u_char *sp = *pnt;
  int i;

  /* Check size.  */
  if (*size < NSM_MSG_USER_SIZE)
    return NSM_ERR_PKT_TOO_SMALL;

  /* Privilege. */
  TLV_ENCODE_PUTC (msg->privilege);
  TLV_ENCODE_PUTC (0);

  /* Username. */
  TLV_ENCODE_PUTW (msg->username_len);
  TLV_ENCODE_PUT (msg->username, msg->username_len);

  /* Username/passwords TLV parse.  */
  for (i = 0; i < NSM_CINDEX_SIZE; i++)
    if (NSM_CHECK_CTYPE (msg->cindex, i))
      {
        if (*size < NSM_TLV_HEADER_SIZE)
          return NSM_ERR_PKT_TOO_SMALL;

        switch (i)
          {
          case NSM_USER_CTYPE_PASSWORD:
            nsm_encode_tlv (pnt, size, i, msg->password_len);
            TLV_ENCODE_PUT (msg->password, msg->password_len);
            break;
          case NSM_USER_CTYPE_PASSWORD_ENCRYPT:
            nsm_encode_tlv (pnt, size, i, msg->password_encrypt_len);
            TLV_ENCODE_PUT (msg->password, msg->password_encrypt_len);
            break;
          default:
            break;
          }
      }
  return *pnt - sp;
}

s_int32_t
nsm_decode_user (u_char **pnt, u_int16_t *size, struct nsm_msg_user *msg)
{
  struct nsm_tlv_header tlv;
  u_char *sp = *pnt;

  /* Check size.  */
  if (*size < NSM_MSG_USER_SIZE)
    return NSM_ERR_PKT_TOO_SMALL;

  /* Privilege. */
  TLV_DECODE_GETC (msg->privilege);
  TLV_DECODE_SKIP (1);

  /* Username. */
  TLV_DECODE_GETW (msg->username_len);
  msg->username = *pnt;
  TLV_DECODE_SKIP (msg->username_len);

  /* User TLV parse.  */
  while (*size)
    {
      if (*size < NSM_TLV_HEADER_SIZE)
        return NSM_ERR_PKT_TOO_SMALL;

      NSM_DECODE_TLV_HEADER (tlv);

      switch (tlv.type)
        {
        case NSM_USER_CTYPE_PASSWORD:
          msg->password = *pnt;
          TLV_DECODE_SKIP (tlv.length);
          NSM_SET_CTYPE (msg->cindex, tlv.type);
          break;
        case NSM_USER_CTYPE_PASSWORD_ENCRYPT:
          msg->password_encrypt = *pnt;
          TLV_DECODE_SKIP (tlv.length);
          NSM_SET_CTYPE (msg->cindex, tlv.type);
          break;
        default:
          TLV_DECODE_SKIP (tlv.length);
          break;
        }
    }
  return *pnt - sp;
}
#endif /* HAVE_IMI */

#if defined (HAVE_MPLS) || defined (HAVE_GMPLS)
s_int32_t
nsm_encode_gmpls_label (u_char **pnt, u_int16_t *size,
                        struct gmpls_gen_label *lbl)
{
  u_char *sp = *pnt;

  TLV_ENCODE_PUTC (lbl->type);

  switch (lbl->type)
    {
#ifdef HAVE_PACKET
       case gmpls_entry_type_ip:
         TLV_ENCODE_PUTL (lbl->u.pkt);
         break;
#endif /* HAVE_PACKET */

#ifdef HAVE_GMPLS
#ifdef HAVE_PBB_TE
       case gmpls_entry_type_pbb_te:
         TLV_ENCODE_PUT (lbl->u.pbb.bmac, 6);
         TLV_ENCODE_PUTW (lbl->u.pbb.bvid);
         break;
#endif /* HAVE_PBB_TE */

#ifdef HAVE_TDM
       case gmpls_entry_type_tdm:
         TLV_ENCODE_PUTL (lbl->u.tdm.gifindex);
         TLV_ENCODE_PUTL (lbl->u.tdm.tslot);
         break;
#endif /* HAVE_TDM */
#endif /* HAVE_GMPLS */

       default:
         break;
    }

  return *pnt - sp;
}

/* Encode GMPLS FTN IPv4 data */
s_int32_t
nsm_decode_gmpls_label (u_char **pnt, u_int16_t *size,
                        struct gmpls_gen_label *lbl)
{
  u_char *sp = *pnt;

  TLV_DECODE_GETC (lbl->type);

  switch (lbl->type)
    {
#ifdef HAVE_PACKET
       case gmpls_entry_type_ip:
         TLV_DECODE_GETL (lbl->u.pkt);
         break;
#endif /* HAVE_PACKET */

#ifdef HAVE_GMPLS
#ifdef HAVE_PBB_TE
       case gmpls_entry_type_pbb_te:
         TLV_DECODE_GET (lbl->u.pbb.bmac, 6);
         TLV_DECODE_GETW (lbl->u.pbb.bvid);
         break;
#endif /* HAVE_PBB_TE */

#ifdef HAVE_TDM
       case gmpls_entry_type_tdm:
         TLV_DECODE_GETL (lbl->u.tdm.gifindex);
         TLV_DECODE_GETL (lbl->u.tdm.tslot);
         break;
#endif /* HAVE_TDM */
#endif /* HAVE_GMPLS */

       default:
         break;
    }

  return *pnt - sp;
}

#ifdef HAVE_GMPLS

s_int32_t
nsm_encode_block_set (u_char **pnt, u_int16_t *size,
                      struct nsm_msg_label_pool *msg)
{
  u_char *sp = *pnt;
  int i;
  int bt;

  for (bt = 0; bt < 2; bt++)
    {
      TLV_ENCODE_PUTC (msg->blk_list[bt].action_type);
      TLV_ENCODE_PUTC (msg->blk_list[bt].blk_req_type);
      TLV_ENCODE_PUTL (msg->blk_list[bt].blk_count);

      if (msg->blk_list[bt].blk_count == 0)
        {
          continue;
        }

      for (i = 0; i < msg->blk_list[bt].blk_count; i++)
        {
          TLV_ENCODE_PUTL (msg->blk_list[bt].lset_blocks[i]);
        }
    }

  return *pnt - sp;
}

s_int32_t
nsm_decode_block_set (u_char **pnt, u_int16_t *size,
                      struct nsm_msg_label_pool *msg)
{
  u_char *sp = *pnt;
  int i;
  int bt;

  /* Look for server returned block(s) */
  for (bt = 0; bt < 2; bt++)
    {
      TLV_DECODE_GETC (msg->blk_list[bt].action_type);
      TLV_DECODE_GETC (msg->blk_list[bt].blk_req_type);
      TLV_DECODE_GETL (msg->blk_list[bt].blk_count);

      if (msg->blk_list[bt].blk_count > 0)
        {
          msg->blk_list[bt].lset_blocks = XCALLOC (
                                          MTYPE_TMP, (sizeof (u_int32_t) *
                                             msg->blk_list[bt].blk_count));
        }
      else
        {
          continue;
        }

      if (msg->blk_list[bt].lset_blocks)
        {
          for (i = 0; i < msg->blk_list[bt].blk_count; i++)
            {
              TLV_DECODE_GETL (msg->blk_list[bt].lset_blocks[i]);
            }
        }
      else
        {
          return NSM_ERR_MALLOC_FAILURE;
        }
    }

  return *pnt - sp;
}

#endif /* HAVE_GMPLS */

#ifdef HAVE_MPLS

s_int32_t
nsm_encode_label_pool (u_char **pnt, u_int16_t *size,
                       struct nsm_msg_label_pool *msg)
{
  u_char *sp = *pnt;
  u_char *spnt;
  int i;
  int nbytes;

  /* Check size.  */
  if (*size < NSM_MSG_LABEL_POOL_SIZE)
    return NSM_ERR_PKT_TOO_SMALL;

  /* Label space. */
  TLV_ENCODE_PUTW (msg->label_space);

  /* TLV encode.  */
  for (i = 0; i < NSM_CINDEX_SIZE; i++)
    if (NSM_CHECK_CTYPE (msg->cindex, i))
      {
        if (*size < NSM_TLV_HEADER_SIZE)
          return NSM_ERR_PKT_TOO_SMALL;

        switch (i)
          {
            case NSM_LABEL_POOL_CTYPE_LABEL_RANGE:
              nsm_encode_tlv (pnt, size, i, 1);
              TLV_ENCODE_PUTL (msg->range_owner);
              break;
            case NSM_LABEL_POOL_CTYPE_LABEL_BLOCK:
              nsm_encode_tlv (pnt, size, i, 12);
              TLV_ENCODE_PUTL (msg->label_block);
              TLV_ENCODE_PUTL (msg->min_label);
              TLV_ENCODE_PUTL (msg->max_label);
              break;
#ifdef HAVE_GMPLS
            case NSM_LABEL_POOL_CTYPE_BLOCK_LIST:
              spnt = *pnt;
              *pnt += NSM_TLV_HEADER_SIZE;
              nbytes = nsm_encode_block_set (pnt, size, msg);
              nsm_encode_tlv (&spnt, size, i, nbytes);
              break;
#endif /* HAVE_GMPLS */
            default:
              break;
          }
      }
  return *pnt - sp;
}

s_int32_t
nsm_decode_label_pool (u_char **pnt, u_int16_t *size,
                       struct nsm_msg_label_pool *msg)
{
  u_char *sp = *pnt;
  struct nsm_tlv_header tlv;
  int nbytes;
  u_int16_t len;

  /* Check size.  */
  if (*size < NSM_MSG_LABEL_POOL_SIZE)
    return NSM_ERR_PKT_TOO_SMALL;

  /* Label space. */
  TLV_DECODE_GETW (msg->label_space);

  /* TLV parse.  */
  while (*size)
    {
      if (*size < NSM_TLV_HEADER_SIZE)
        return NSM_ERR_PKT_TOO_SMALL;

      NSM_DECODE_TLV_HEADER (tlv);

      switch (tlv.type)
        {
        case NSM_LABEL_POOL_CTYPE_LABEL_RANGE:
          TLV_DECODE_GETL (msg->range_owner);
          NSM_SET_CTYPE (msg->cindex, tlv.type);
          break;
        case NSM_LABEL_POOL_CTYPE_LABEL_BLOCK:
          TLV_DECODE_GETL (msg->label_block);
          TLV_DECODE_GETL (msg->min_label);
          TLV_DECODE_GETL (msg->max_label);
          NSM_SET_CTYPE (msg->cindex, tlv.type);
          break;
#ifdef HAVE_GMPLS
        case NSM_LABEL_POOL_CTYPE_BLOCK_LIST:
          len = tlv.length;
          nbytes = nsm_decode_block_set(pnt, &len, msg);
          *size -= nbytes;
          NSM_SET_CTYPE (msg->cindex, tlv.type);
          break;
#endif /* HAVE_GMPLS */
        default:
          TLV_DECODE_SKIP (tlv.length);
          break;
        }
    }
  return *pnt - sp;
}

/* Encode MPLS owner to stream. */
static int
nsm_encode_mpls_owner_ipv4 (u_char **pnt, u_int16_t *size,
                            struct mpls_owner *owner)
{
  TLV_ENCODE_PUTC (owner->owner);
  switch (owner->owner)
    {
    case MPLS_LDP:
      if (owner->u.l_key.afi != AFI_IP)
        return -1;
      TLV_ENCODE_PUTC (owner->u.l_key.afi);
      TLV_ENCODE_PUT_IN4_ADDR (&owner->u.l_key.u.ipv4.addr);
      TLV_ENCODE_PUTL (owner->u.l_key.u.ipv4.lsr_id);
      TLV_ENCODE_PUTW (owner->u.l_key.u.ipv4.label_space);
      break;
    case MPLS_RSVP:
      if (owner->u.r_key.afi != AFI_IP)
        return -1;
      TLV_ENCODE_PUTC (owner->u.r_key.afi);
      TLV_ENCODE_PUTW (owner->u.r_key.u.ipv4.trunk_id);
      TLV_ENCODE_PUTW (owner->u.r_key.u.ipv4.lsp_id);
      TLV_ENCODE_PUT_IN4_ADDR (&owner->u.r_key.u.ipv4.ingr);
      TLV_ENCODE_PUT_IN4_ADDR (&owner->u.r_key.u.ipv4.egr);
      break;
    case MPLS_OTHER_BGP:
      if (owner->u.b_key.afi != AFI_IP)
        return -1;
      TLV_ENCODE_PUTC (owner->u.b_key.afi);
      TLV_ENCODE_PUT_IN4_ADDR (&owner->u.b_key.u.ipv4.peer);
      TLV_ENCODE_PUT_IN4_ADDR (&owner->u.b_key.u.ipv4.p.prefix);
      TLV_ENCODE_PUTC (owner->u.b_key.u.ipv4.p.prefixlen);
      TLV_ENCODE_PUTL (owner->u.b_key.vrf_id);
      break;
    case MPLS_OTHER_LDP_VC:
      TLV_ENCODE_PUTL (owner->u.vc_key.vc_id);
      TLV_ENCODE_PUT_IN4_ADDR (&owner->u.vc_key.vc_peer);
      break;
    case MPLS_CRLDP:
      break;
    default:
      return -1;
    }

  return 0;
}

/* Decode MPLS owner. */
static int
nsm_decode_mpls_owner_ipv4 (u_char **pnt, u_int16_t *size,
                            struct mpls_owner *owner)
{
  TLV_DECODE_GETC (owner->owner);
  TLV_DECODE_SKIP (1);
  switch (owner->owner)
    {
    case MPLS_LDP:
      owner->u.l_key.afi = AFI_IP;
      TLV_DECODE_GET_IN4_ADDR (&owner->u.l_key.u.ipv4.addr);
      TLV_DECODE_GETL (owner->u.l_key.u.ipv4.lsr_id);
      TLV_DECODE_GETW (owner->u.l_key.u.ipv4.label_space);
      break;
    case MPLS_RSVP:
      owner->u.r_key.afi = AFI_IP;
      TLV_DECODE_GETW (owner->u.r_key.u.ipv4.trunk_id);
      TLV_DECODE_GETW (owner->u.r_key.u.ipv4.lsp_id);
      TLV_DECODE_GET_IN4_ADDR (&owner->u.r_key.u.ipv4.ingr);
      TLV_DECODE_GET_IN4_ADDR (&owner->u.r_key.u.ipv4.egr);
      break;
    case MPLS_OTHER_BGP:
      owner->u.b_key.afi = AFI_IP;
      TLV_DECODE_GET_IN4_ADDR (&owner->u.b_key.u.ipv4.peer);
      owner->u.b_key.u.ipv4.p.family = AF_INET;
      TLV_DECODE_GET_IN4_ADDR (&owner->u.b_key.u.ipv4.p.prefix);
      TLV_DECODE_GETC (owner->u.b_key.u.ipv4.p.prefixlen);
      TLV_DECODE_GETL (owner->u.b_key.vrf_id);
      break;
    case MPLS_OTHER_LDP_VC:
      TLV_DECODE_GETL (owner->u.vc_key.vc_id);
      TLV_DECODE_GET_IN4_ADDR (&owner->u.vc_key.vc_peer);
      break;
    case MPLS_CRLDP:
      break;
    default:
      return -1;
    }

  return 0;
}

#ifdef HAVE_IPV6
static int
nsm_encode_mpls_owner_ipv6 (u_char **pnt, u_int16_t *size,
                            struct mpls_owner *owner)
{
  TLV_ENCODE_PUTC (owner->owner);
  switch (owner->owner)
    {
    case MPLS_LDP:
      if (owner->u.l_key.afi != AFI_IP6)
        {
          TLV_ENCODE_PUTC (owner->u.l_key.afi);
          TLV_ENCODE_PUT_IN4_ADDR (&owner->u.l_key.u.ipv4.addr);
          TLV_ENCODE_PUTL (owner->u.l_key.u.ipv4.lsr_id);
          TLV_ENCODE_PUTW (owner->u.l_key.u.ipv4.label_space);
        }
      else
        {
          TLV_ENCODE_PUTC (owner->u.l_key.afi);
          TLV_ENCODE_PUT_IN6_ADDR (&owner->u.l_key.u.ipv6.addr);
          TLV_ENCODE_PUTL (owner->u.l_key.u.ipv6.lsr_id);
          TLV_ENCODE_PUTW (owner->u.l_key.u.ipv6.label_space);
        }
      break;
    case MPLS_RSVP:
      if (owner->u.r_key.afi != AFI_IP6)
        {
          TLV_ENCODE_PUTC (owner->u.r_key.afi);
          TLV_ENCODE_PUTW (owner->u.r_key.u.ipv4.trunk_id);
          TLV_ENCODE_PUTW (owner->u.r_key.u.ipv4.lsp_id);
          TLV_ENCODE_PUT_IN4_ADDR (&owner->u.r_key.u.ipv4.ingr);
          TLV_ENCODE_PUT_IN4_ADDR (&owner->u.r_key.u.ipv4.egr);
        }
      else
        {
          TLV_ENCODE_PUTC (owner->u.r_key.afi);
          TLV_ENCODE_PUTW (owner->u.r_key.u.ipv6.trunk_id);
          TLV_ENCODE_PUTW (owner->u.r_key.u.ipv6.lsp_id);
          TLV_ENCODE_PUT_IN6_ADDR (&owner->u.r_key.u.ipv6.ingr);
          TLV_ENCODE_PUT_IN6_ADDR (&owner->u.r_key.u.ipv6.egr);
        }

      break;
    case MPLS_OTHER_BGP:
      if (owner->u.b_key.afi != AFI_IP6)
        {
          TLV_ENCODE_PUTC (owner->u.b_key.afi);
          TLV_ENCODE_PUT_IN4_ADDR (&owner->u.b_key.u.ipv4.peer);
          TLV_ENCODE_PUT_IN4_ADDR (&owner->u.b_key.u.ipv4.p.prefix);
          TLV_ENCODE_PUTC (owner->u.b_key.u.ipv4.p.prefixlen);
          TLV_ENCODE_PUTL (owner->u.b_key.vrf_id);
        }
      else
        {
          TLV_ENCODE_PUTC (owner->u.b_key.afi);
          TLV_ENCODE_PUT_IN6_ADDR (&owner->u.b_key.u.ipv6.peer);
          TLV_ENCODE_PUT_IN6_ADDR (&owner->u.b_key.u.ipv6.p.prefix);
          TLV_ENCODE_PUTC (owner->u.b_key.u.ipv6.p.prefixlen);
          TLV_ENCODE_PUTL (owner->u.b_key.vrf_id);
        }
      break;
    case MPLS_OTHER_LDP_VC:
      TLV_ENCODE_PUTL (owner->u.vc_key.vc_id);
      TLV_ENCODE_PUT_IN4_ADDR (&owner->u.vc_key.vc_peer);
      break;
    case MPLS_CRLDP:
      break;
    default:
      return -1;
    }

  return 0;
}

/* Decode MPLS owner. */
static int
nsm_decode_mpls_owner_ipv6 (u_char **pnt, u_int16_t *size,
                            struct mpls_owner *owner)
{
  char afi;

  TLV_DECODE_GETC (owner->owner);
  TLV_DECODE_GETC (afi);

  if (afi == AFI_IP6)
    {
      switch (owner->owner)
        {
          case MPLS_LDP:
            owner->u.l_key.afi = AFI_IP6;
            TLV_DECODE_GET_IN6_ADDR (&owner->u.l_key.u.ipv6.addr);
            TLV_DECODE_GETL (owner->u.l_key.u.ipv6.lsr_id);
            TLV_DECODE_GETW (owner->u.l_key.u.ipv6.label_space);
            break;
          case MPLS_RSVP:
            owner->u.r_key.afi = AFI_IP6;
            TLV_DECODE_GETW (owner->u.r_key.u.ipv6.trunk_id);
            TLV_DECODE_GETW (owner->u.r_key.u.ipv6.lsp_id);
            TLV_DECODE_GET_IN6_ADDR (&owner->u.r_key.u.ipv6.ingr);
            TLV_DECODE_GET_IN6_ADDR (&owner->u.r_key.u.ipv6.egr);
            break;
          case MPLS_OTHER_BGP:
            owner->u.b_key.afi = AFI_IP6;
            TLV_DECODE_GET_IN6_ADDR (&owner->u.b_key.u.ipv6.peer);
            owner->u.b_key.u.ipv6.p.family = AF_INET6;
            TLV_DECODE_GET_IN6_ADDR (&owner->u.b_key.u.ipv6.p.prefix);
            TLV_DECODE_GETC (owner->u.b_key.u.ipv6.p.prefixlen);
            TLV_DECODE_GETL (owner->u.b_key.vrf_id);
            break;
          case MPLS_OTHER_LDP_VC:
            TLV_DECODE_GETL (owner->u.vc_key.vc_id);
            TLV_DECODE_GET_IN4_ADDR (&owner->u.vc_key.vc_peer);
            break;
          case MPLS_CRLDP:
            break;
          default:
            return -1;
        }
    }
  else
    {
      switch (owner->owner)
        {
          case MPLS_LDP:
            owner->u.l_key.afi = AFI_IP;
            TLV_DECODE_GET_IN4_ADDR (&owner->u.l_key.u.ipv4.addr);
            TLV_DECODE_GETL (owner->u.l_key.u.ipv4.lsr_id);
            TLV_DECODE_GETW (owner->u.l_key.u.ipv4.label_space);
            break;
          case MPLS_RSVP:
            owner->u.r_key.afi = AFI_IP;
            TLV_DECODE_GETW (owner->u.r_key.u.ipv4.trunk_id);
            TLV_DECODE_GETW (owner->u.r_key.u.ipv4.lsp_id);
            TLV_DECODE_GET_IN4_ADDR (&owner->u.r_key.u.ipv4.ingr);
            TLV_DECODE_GET_IN4_ADDR (&owner->u.r_key.u.ipv4.egr);
            break;
          case MPLS_OTHER_BGP:
            owner->u.b_key.afi = AFI_IP;
            TLV_DECODE_GET_IN4_ADDR (&owner->u.b_key.u.ipv4.peer);
            owner->u.b_key.u.ipv4.p.family = AF_INET;
            TLV_DECODE_GET_IN4_ADDR (&owner->u.b_key.u.ipv4.p.prefix);
            TLV_DECODE_GETC (owner->u.b_key.u.ipv4.p.prefixlen);
            TLV_DECODE_GETL (owner->u.b_key.vrf_id);
            break;
          case MPLS_OTHER_LDP_VC:
            TLV_DECODE_GETL (owner->u.vc_key.vc_id);
            TLV_DECODE_GET_IN4_ADDR (&owner->u.vc_key.vc_peer);
            break;
          case MPLS_CRLDP:
            break;
          default:
            return -1;
        }
    }

  return 0;
}
#endif

#ifdef HAVE_PACKET
s_int32_t
nsm_encode_ilm_lookup (u_char **pnt, u_int16_t *size,
                       struct nsm_msg_ilm_lookup *msg)
{
  u_char *sp = *pnt;
  TLV_ENCODE_PUTC (msg->owner);
  TLV_ENCODE_PUTC (msg->opcode);
  TLV_ENCODE_PUTL (msg->ilm_in_ix);
  TLV_ENCODE_PUTL (msg->ilm_in_label);
  TLV_ENCODE_PUTL (msg->ilm_out_ix);
  TLV_ENCODE_PUTL (msg->ilm_out_label);
  TLV_ENCODE_PUTL (msg->ilm_ix);
  return *pnt - sp;
}

s_int32_t
nsm_decode_ilm_lookup (u_char **pnt, u_int16_t *size,
                       struct nsm_msg_ilm_lookup *msg)
{
  u_char *sp = *pnt;
  TLV_DECODE_GETC (msg->owner);
  TLV_DECODE_GETC (msg->opcode);
  TLV_DECODE_GETL (msg->ilm_in_ix);
  TLV_DECODE_GETL (msg->ilm_in_label);
  TLV_DECODE_GETL (msg->ilm_out_ix);
  TLV_DECODE_GETL (msg->ilm_out_label);
  TLV_DECODE_GETL (msg->ilm_ix);
  return *pnt - sp;
}
#endif /* HAVE_PACKET */

#ifdef HAVE_MPLS
s_int32_t
nsm_decode_ilm_gen_lookup (u_char **pnt, u_int16_t *size,
                           struct nsm_msg_ilm_gen_lookup *msg)
{
  u_char *sp = *pnt;
  TLV_DECODE_GETC (msg->owner);
  TLV_DECODE_GETC (msg->opcode);
  TLV_DECODE_GETL (msg->ilm_in_ix);
  nsm_decode_gmpls_label (pnt, size, &msg->ilm_in_label);
  TLV_DECODE_GETL (msg->ilm_out_ix);
  nsm_decode_gmpls_label (pnt, size, &msg->ilm_out_label);
  TLV_DECODE_GETL (msg->ilm_ix);
  return *pnt - sp;
}

s_int32_t
nsm_encode_ilm_gen_lookup (u_char **pnt, u_int16_t *size,
                           struct nsm_msg_ilm_gen_lookup *msg)
{
  u_char *sp = *pnt;
  TLV_ENCODE_PUTC (msg->owner);
  TLV_ENCODE_PUTC (msg->opcode);
  TLV_ENCODE_PUTL (msg->ilm_in_ix);
  nsm_encode_gmpls_label (pnt, size, &msg->ilm_in_label);
  TLV_ENCODE_PUTL (msg->ilm_out_ix);
  nsm_encode_gmpls_label (pnt, size, &msg->ilm_out_label);
  TLV_ENCODE_PUTL (msg->ilm_ix);
  return *pnt - sp;
}

#endif /* HAVE_MPLS */
/* Encode NSM_MSG_IGP_SHORTCUT_LSP */
s_int32_t
nsm_encode_igp_shortcut_lsp (u_char **pnt, u_int16_t *size,
                             struct nsm_msg_igp_shortcut_lsp *msg)
{
  u_char *sp = *pnt;

  TLV_ENCODE_PUTC (msg->action);
  TLV_ENCODE_PUTW (msg->metric);
  TLV_ENCODE_PUTL (msg->tunnel_id);
  TLV_ENCODE_PUT_IN4_ADDR (&msg->t_endp);

  return *pnt - sp;
}

/* Decode NSM_MSG_IGP_SHORTCUT_LSP */
s_int32_t
nsm_decode_igp_shortcut_lsp (u_char **pnt, u_int16_t *size,
                             struct nsm_msg_igp_shortcut_lsp *msg)
{
  u_char *sp = *pnt;

  TLV_DECODE_GETC (msg->action);
  TLV_DECODE_GETW (msg->metric);
  TLV_DECODE_GETL (msg->tunnel_id);
  TLV_DECODE_GET_IN4_ADDR (&msg->t_endp);

  return *pnt - sp;
}

/* Encode igp_shortcut route */
s_int32_t
nsm_encode_igp_shortcut_route (u_char **pnt, u_int16_t *size,
                               struct nsm_msg_igp_shortcut_route *msg)
{
  u_char *sp = *pnt;

  TLV_ENCODE_PUTC (msg->action);
  TLV_ENCODE_PUTL (msg->tunnel_id);
  TLV_ENCODE_PUTC (msg->fec.prefixlen);
  TLV_ENCODE_PUT_IN4_ADDR (&msg->fec.u.prefix4);
  TLV_ENCODE_PUT_IN4_ADDR (&msg->t_endp);

  return *pnt - sp;
}

/* Decode igp_shortcut route */
s_int32_t
nsm_decode_igp_shortcut_route (u_char **pnt, u_int16_t *size,
                               struct nsm_msg_igp_shortcut_route *msg)
{
  u_char *sp = *pnt;

  TLV_DECODE_GETC (msg->action);
  TLV_DECODE_GETL (msg->tunnel_id);
  TLV_DECODE_GETC (msg->fec.prefixlen);
  TLV_DECODE_GET_IN4_ADDR (&msg->fec.u.prefix4);
  TLV_DECODE_GET_IN4_ADDR (&msg->t_endp);

  msg->fec.family = AF_INET;

  return *pnt - sp;
}
#endif /* HAVE_MPLS */

#ifdef HAVE_TE
void
qos_prot_nsm_message_map (struct qos_resource *resource,
                          struct nsm_msg_qos *msg)
{
  struct nsm_msg_qos_t_spec *dt_spec;
  struct qos_traffic_spec *st_spec;
  struct nsm_msg_qos_if_spec *dif_spec;
  struct qos_if_spec *sif_spec;
  struct nsm_msg_qos_ad_spec *dad_spec;
  struct qos_ad_spec *sad_spec;

  /* Cindex. */
  msg->cindex = 0;

  msg->resource_id = resource->resource_id;
  msg->protocol_id = resource->protocol_id;
  msg->id = resource->id;

  /* Copy over owner. */
  pal_mem_cpy (&msg->owner, &resource->owner, sizeof (struct mpls_owner));

  /* Copy Class Type number. */
  msg->ct_num = resource->ct_num;

  if (CHECK_FLAG (resource->flags, QOS_RESOURCE_FLAG_SETUP_PRIORITY))
    {
      NSM_SET_CTYPE (msg->cindex, NSM_QOS_CTYPE_SETUP_PRIORITY);
      msg->setup_priority = resource->setup_priority;
    }

  if (CHECK_FLAG (resource->flags, QOS_RESOURCE_FLAG_HOLD_PRIORITY))
    {
      NSM_SET_CTYPE (msg->cindex, NSM_QOS_CTYPE_HOLD_PRIORITY);
      msg->hold_priority = resource->hold_priority;
    }

  if (CHECK_FLAG (resource->flags, QOS_RESOURCE_FLAG_TRAFFIC_SPEC))
    {
      st_spec = &resource->t_spec;
      dt_spec = &msg->t_spec;

      /* Tspec cindex. */
      dt_spec->cindex = 0;
      NSM_SET_CTYPE (msg->cindex, NSM_QOS_CTYPE_T_SPEC);

      dt_spec->service_type = st_spec->service_type;
      dt_spec->is_exclusive = st_spec->is_exclusive;

      if (CHECK_FLAG (st_spec->flags, QOS_TSPEC_FLAG_PEAK_RATE))
        {
          NSM_SET_CTYPE (dt_spec->cindex, NSM_QOS_CTYPE_T_SPEC_PEAK_RATE);
          dt_spec->peak_rate = st_spec->peak_rate;
        }
      if (CHECK_FLAG (st_spec->flags, QOS_TSPEC_FLAG_COMMITTED_BUCKET))
        {
          NSM_SET_CTYPE (dt_spec->cindex,
                         NSM_QOS_CTYPE_T_SPEC_QOS_COMMITED_BUCKET);

          dt_spec->rate = st_spec->committed_bucket.rate;
          dt_spec->size = st_spec->committed_bucket.size;
        }
      if (CHECK_FLAG (st_spec->flags, QOS_TSPEC_FLAG_EXCESS_BURST))
        {
          NSM_SET_CTYPE (dt_spec->cindex,
                         NSM_QOS_CTYPE_T_SPEC_EXCESS_BURST_SIZE);
          dt_spec->excess_burst = st_spec->excess_burst;
        }
      if (CHECK_FLAG (st_spec->flags, QOS_TSPEC_FLAG_WEIGHT))
        {
          NSM_SET_CTYPE (dt_spec->cindex, NSM_QOS_CTYPE_T_SPEC_WEIGHT);
          dt_spec->weight = st_spec->weight;
        }
      if (CHECK_FLAG (st_spec->flags, QOS_TSPEC_FLAG_MIN_POLICED_UNIT))
        {
          NSM_SET_CTYPE (dt_spec->cindex,
                         NSM_QOS_CTYPE_T_SPEC_MIN_POLICED_UNIT);
          dt_spec->min_policed_unit = st_spec->mpu;
        }
      if (CHECK_FLAG (st_spec->flags, QOS_TSPEC_FLAG_MAX_PACKET_SIZE))
        {
          NSM_SET_CTYPE (dt_spec->cindex,
                         NSM_QOS_CTYPE_T_SPEC_MAX_PACKET_SIZE);
          dt_spec->max_packet_size = st_spec->max_packet_size;
        }
    }

  if (CHECK_FLAG (resource->flags, QOS_RESOURCE_FLAG_IF_SPEC))
    {
      dif_spec = &msg->if_spec;
      sif_spec = &resource->if_spec;

      /* Tspec cindex. */
      dif_spec->cindex = 0;
      NSM_SET_CTYPE (msg->cindex, NSM_QOS_CTYPE_IF_SPEC);

      if (CHECK_FLAG (sif_spec->flags, QOS_IFSPEC_FLAG_PREFIX))
        {
          NSM_SET_CTYPE (dif_spec->cindex, NSM_QOS_CTYPE_IF_SPEC_ADDR);
          dif_spec->addr.prev_hop= sif_spec->prev_hop.u.prefix4;
          dif_spec->addr.next_hop = sif_spec->next_hop.u.prefix4;
        }

      NSM_SET_CTYPE (dif_spec->cindex, NSM_QOS_CTYPE_IF_SPEC_IFINDEX);
      dif_spec->ifindex.in = sif_spec->in_ifindex;
      dif_spec->ifindex.out = sif_spec->out_ifindex;
    }

  if (CHECK_FLAG (resource->flags, QOS_RESOURCE_FLAG_AD_SPEC))
    {
      dad_spec = &msg->ad_spec;
      sad_spec = &resource->ad_spec;

      /* ADspec cindex. */
      dad_spec->cindex = 0;
      NSM_SET_CTYPE (msg->cindex, NSM_QOS_CTYPE_AD_SPEC);

      if (CHECK_FLAG (sad_spec->flags, QOS_ADSPEC_FLAG_SLACK))
        {
          NSM_SET_CTYPE (dad_spec->cindex, NSM_QOS_CTYPE_AD_SPEC_SLACK);
          dad_spec->slack = sad_spec->slack;
        }
      if (CHECK_FLAG (sad_spec->flags, QOS_ADSPEC_FLAG_CTOT))
        {
          NSM_SET_CTYPE (dad_spec->cindex,
                         NSM_QOS_CTYPE_AD_SPEC_COMPOSE_TOTAL);
          dad_spec->compose_total = sad_spec->Ctot;
        }
      if (CHECK_FLAG (sad_spec->flags, QOS_ADSPEC_FLAG_DTOT))
        {
          NSM_SET_CTYPE (dad_spec->cindex,
                         NSM_QOS_CTYPE_AD_SPEC_DERIVED_TOTAL);
          dad_spec->derived_total = sad_spec->Dtot;
        }
      if (CHECK_FLAG (sad_spec->flags, QOS_ADSPEC_FLAG_CSUM))
        {
          NSM_SET_CTYPE (dad_spec->cindex, NSM_QOS_CTYPE_AD_SPEC_COMPOSE_SUM);
          dad_spec->compose_sum = sad_spec->Csum;
        }
      if (CHECK_FLAG (sad_spec->flags, QOS_ADSPEC_FLAG_DSUM))
        {
          NSM_SET_CTYPE (dad_spec->cindex, NSM_QOS_CTYPE_AD_SPEC_DERIVED_SUM);
          dad_spec->derived_sum = sad_spec->Dsum;
        }
      if (CHECK_FLAG (sad_spec->flags, QOS_ADSPEC_FLAG_MPL))
        {
          NSM_SET_CTYPE (dad_spec->cindex,
                         NSM_QOS_CTYPE_AD_SPEC_MIN_PATH_LATENCY);
          dad_spec->min_path_latency = sad_spec->mpl;
        }
      if (CHECK_FLAG (sad_spec->flags, QOS_ADSPEC_FLAG_PBE))
        {
          NSM_SET_CTYPE (dad_spec->cindex, NSM_QOS_CTYPE_AD_SPEC_PATH_BW_EST);
          dad_spec->path_bw_est = sad_spec->pbe;
        }
    }
}

void
qos_nsm_prot_message_map (struct nsm_msg_qos *msg,
                          struct qos_resource *resource)
{
  struct nsm_msg_qos_t_spec *st_spec;
  struct qos_traffic_spec *dt_spec;
  struct nsm_msg_qos_if_spec *sif_spec;
  struct qos_if_spec *dif_spec;
  struct nsm_msg_qos_ad_spec *sad_spec;
  struct qos_ad_spec *dad_spec;

  resource->flags = 0;

  resource->resource_id = msg->resource_id;
  resource->protocol_id = msg->protocol_id;
  resource->id = msg->id;

  /* Copy over MPLS Owner. */
  pal_mem_cpy (&resource->owner, &msg->owner, sizeof (struct mpls_owner));

  /* Copy Class Type number. */
  resource->ct_num = msg->ct_num;

  if (NSM_CHECK_CTYPE (msg->cindex, NSM_QOS_CTYPE_SETUP_PRIORITY))
    {
      SET_FLAG (resource->flags, QOS_RESOURCE_FLAG_SETUP_PRIORITY);
      resource->setup_priority = msg->setup_priority;
    }

  if (NSM_CHECK_CTYPE (msg->cindex, NSM_QOS_CTYPE_HOLD_PRIORITY))
    {
      SET_FLAG (resource->flags, QOS_RESOURCE_FLAG_HOLD_PRIORITY);
      resource->hold_priority = msg->hold_priority;
    }

  if (NSM_CHECK_CTYPE (msg->cindex, NSM_QOS_CTYPE_T_SPEC))
    {
      dt_spec = &resource->t_spec;
      st_spec = &msg->t_spec;

      /* Tspec cindex. */
      dt_spec->flags = 0;
      SET_FLAG (resource->flags, QOS_RESOURCE_FLAG_TRAFFIC_SPEC);

      dt_spec->service_type = st_spec->service_type;
      dt_spec->is_exclusive = st_spec->is_exclusive;

      if (NSM_CHECK_CTYPE (st_spec->cindex, NSM_QOS_CTYPE_T_SPEC_PEAK_RATE))
        {
          SET_FLAG (dt_spec->flags , QOS_TSPEC_FLAG_PEAK_RATE);
          dt_spec->peak_rate = st_spec->peak_rate;
        }
      if (NSM_CHECK_CTYPE (st_spec->cindex,
                           NSM_QOS_CTYPE_T_SPEC_QOS_COMMITED_BUCKET))
        {
          SET_FLAG (dt_spec->flags, QOS_TSPEC_FLAG_COMMITTED_BUCKET);
          SET_FLAG (dt_spec->committed_bucket.flags, QOS_BUCKET_RATE);
          SET_FLAG (dt_spec->committed_bucket.flags, QOS_BUCKET_SIZE);

          dt_spec->committed_bucket.rate = st_spec->rate;
          dt_spec->committed_bucket.size = st_spec->size;
        }
      if (NSM_CHECK_CTYPE (st_spec->cindex,
                           NSM_QOS_CTYPE_T_SPEC_EXCESS_BURST_SIZE))
        {
          SET_FLAG (dt_spec->flags, QOS_TSPEC_FLAG_EXCESS_BURST);
          dt_spec->excess_burst = st_spec->excess_burst;
        }
      if (NSM_CHECK_CTYPE (st_spec->cindex, NSM_QOS_CTYPE_T_SPEC_WEIGHT))
        {
          SET_FLAG (dt_spec->flags, QOS_TSPEC_FLAG_WEIGHT);
          dt_spec->weight = st_spec->weight;
        }
      if (NSM_CHECK_CTYPE (st_spec->cindex,
                           NSM_QOS_CTYPE_T_SPEC_MIN_POLICED_UNIT))
        {
          SET_FLAG (dt_spec->flags, QOS_TSPEC_FLAG_MIN_POLICED_UNIT);
          dt_spec->mpu = st_spec->min_policed_unit;
        }
      if (NSM_CHECK_CTYPE (st_spec->cindex,
                           NSM_QOS_CTYPE_T_SPEC_MAX_PACKET_SIZE))
        {
          SET_FLAG (dt_spec->flags, QOS_TSPEC_FLAG_MAX_PACKET_SIZE);
          dt_spec->max_packet_size = st_spec->max_packet_size;
        }
    }

  if (NSM_CHECK_CTYPE (msg->cindex, NSM_QOS_CTYPE_IF_SPEC))
    {
      sif_spec = &msg->if_spec;
      dif_spec = &resource->if_spec;

      dif_spec->flags = 0;
      SET_FLAG (resource->flags, QOS_RESOURCE_FLAG_IF_SPEC);

      if (NSM_CHECK_CTYPE (sif_spec->cindex, NSM_QOS_CTYPE_IF_SPEC_ADDR))
        {
          SET_FLAG (dif_spec->flags, QOS_IFSPEC_FLAG_PREFIX);
          dif_spec->prev_hop.u.prefix4 = sif_spec->addr.prev_hop;
          dif_spec->next_hop.u.prefix4 = sif_spec->addr.next_hop;
        }
      dif_spec->in_ifindex = sif_spec->ifindex.in;
      dif_spec->out_ifindex = sif_spec->ifindex.out;
    }

  if (CHECK_FLAG (resource->flags, QOS_RESOURCE_FLAG_AD_SPEC))
    if (NSM_CHECK_CTYPE (msg->cindex, NSM_QOS_CTYPE_AD_SPEC))
      {
        sad_spec = &msg->ad_spec;
        dad_spec = &resource->ad_spec;

        dad_spec->flags = 0;
        SET_FLAG (resource->flags, QOS_RESOURCE_FLAG_AD_SPEC);

        if (NSM_CHECK_CTYPE (sad_spec->cindex, NSM_QOS_CTYPE_AD_SPEC))
          {
            SET_FLAG (dad_spec->flags, QOS_ADSPEC_FLAG_SLACK);
            dad_spec->slack = sad_spec->slack;
          }
        if (NSM_CHECK_CTYPE (sad_spec->cindex,
                             NSM_QOS_CTYPE_AD_SPEC_COMPOSE_TOTAL))
          {
            SET_FLAG (dad_spec->flags, QOS_ADSPEC_FLAG_CTOT);
            dad_spec->Ctot = sad_spec->compose_total;
          }
        if (NSM_CHECK_CTYPE (sad_spec->cindex,
                             NSM_QOS_CTYPE_AD_SPEC_DERIVED_TOTAL))
          {
            SET_FLAG (dad_spec->flags, QOS_ADSPEC_FLAG_DTOT);
            dad_spec->Dtot = sad_spec->derived_total;
          }
        if (NSM_CHECK_CTYPE (sad_spec->cindex,
                             NSM_QOS_CTYPE_AD_SPEC_COMPOSE_SUM))
          {
            SET_FLAG (dad_spec->flags, QOS_ADSPEC_FLAG_CSUM);
            dad_spec->Csum = sad_spec->compose_sum;
          }
        if (NSM_CHECK_CTYPE (sad_spec->cindex,
                             NSM_QOS_CTYPE_AD_SPEC_DERIVED_SUM))
          {
            SET_FLAG (dad_spec->flags, QOS_ADSPEC_FLAG_DSUM);
            dad_spec->Dsum = sad_spec->derived_sum;
          }
        if (NSM_CHECK_CTYPE (sad_spec->cindex,
                             NSM_QOS_CTYPE_AD_SPEC_MIN_PATH_LATENCY))
          {
            SET_FLAG (dad_spec->flags, QOS_ADSPEC_FLAG_MPL);
            dad_spec->mpl = sad_spec->min_path_latency;
          }
        if (NSM_CHECK_CTYPE (sad_spec->cindex,
                             NSM_QOS_CTYPE_AD_SPEC_PATH_BW_EST))
          {
            SET_FLAG (dad_spec->flags, QOS_ADSPEC_FLAG_PBE);
            dad_spec->pbe = sad_spec->path_bw_est;
          }
      }
}

s_int32_t
nsm_encode_qos_client_init (u_char **pnt, u_int16_t *size,
                            u_int32_t *msg)
{
  u_char *sp = *pnt;
  u_int32_t protocol_id = *msg;

  /* Check size.  */
  if (*size < NSM_MSG_QOS_CLIENT_INIT_SIZE)
    return NSM_ERR_PKT_TOO_SMALL;

  /* Protocol ID. */
  TLV_ENCODE_PUTL (protocol_id);

  return *pnt - sp;
}

s_int32_t
nsm_decode_qos_client_init (u_char **pnt, u_int16_t *size,
                            u_int32_t *msg)
{
  u_char *sp = *pnt;

  /* Check size.  */
  if (*size < NSM_MSG_LABEL_POOL_SIZE)
    return NSM_ERR_PKT_TOO_SMALL;

  /* Protocol ID. */
  TLV_DECODE_GETL (*msg);

  return *pnt - sp;
}

s_int32_t
nsm_encode_t_spec (u_char **pnt, u_int16_t *size,
                   struct nsm_msg_qos_t_spec *msg)
{
  u_char *sp = *pnt;
  int i;

  /* Service type. */
  TLV_ENCODE_PUTC (msg->service_type);

  /* Exclusive or not. */
  TLV_ENCODE_PUTC (msg->is_exclusive);

  for (i = 0; i < NSM_CINDEX_SIZE; i++)
    if (NSM_CHECK_CTYPE (msg->cindex, i))
      {
        if (*size < NSM_TLV_HEADER_SIZE)
          return NSM_ERR_PKT_TOO_SMALL;

        switch (i)
          {
          case NSM_QOS_CTYPE_T_SPEC_MIN_POLICED_UNIT:
            nsm_encode_tlv (pnt, size, i, 4);
            TLV_ENCODE_PUTL (msg->min_policed_unit);
            break;
          case NSM_QOS_CTYPE_T_SPEC_MAX_PACKET_SIZE:
            nsm_encode_tlv (pnt, size, i, 4);
            TLV_ENCODE_PUTL (msg->max_packet_size);
            break;
          case NSM_QOS_CTYPE_T_SPEC_PEAK_RATE:
            nsm_encode_tlv (pnt, size, i, 4);
            TLV_ENCODE_PUTF (msg->peak_rate);
            break;
          case NSM_QOS_CTYPE_T_SPEC_QOS_COMMITED_BUCKET:
            nsm_encode_tlv (pnt, size, i, 8);
            TLV_ENCODE_PUTF (msg->rate);
            TLV_ENCODE_PUTF (msg->size);
            break;
          case NSM_QOS_CTYPE_T_SPEC_EXCESS_BURST_SIZE:
            nsm_encode_tlv (pnt, size, i, 4);
            TLV_ENCODE_PUTF (msg->excess_burst);
            break;
          case NSM_QOS_CTYPE_T_SPEC_WEIGHT:
            nsm_encode_tlv (pnt, size, i, 4);
            TLV_ENCODE_PUTF (msg->weight);
            break;
          default:
            break;
          }
      }

  return *pnt - sp;
}

s_int32_t
nsm_decode_t_spec (u_char **pnt, u_int16_t *size,
                   struct nsm_msg_qos_t_spec *msg)
{
  u_char *sp = *pnt;
  struct nsm_tlv_header tlv;

  /* Check size.  */
  if (*size < NSM_MSG_QOS_T_SPEC_SIZE)
    return NSM_ERR_PKT_TOO_SMALL;

  /* Service type. */
  TLV_DECODE_GETC (msg->service_type);

  /* Exclusive or not. */
  TLV_DECODE_GETC (msg->is_exclusive);

  /* TLV parse.  */
  while (*size)
    {
      if (*size < NSM_TLV_HEADER_SIZE)
        return NSM_ERR_PKT_TOO_SMALL;

      NSM_DECODE_TLV_HEADER (tlv);

      switch (tlv.type)
        {
        case NSM_QOS_CTYPE_T_SPEC_MIN_POLICED_UNIT:
          TLV_DECODE_GETL (msg->min_policed_unit);
          NSM_SET_CTYPE (msg->cindex, tlv.type);
          break;
        case NSM_QOS_CTYPE_T_SPEC_MAX_PACKET_SIZE:
          TLV_DECODE_GETL (msg->max_packet_size);
          NSM_SET_CTYPE (msg->cindex, tlv.type);
          break;
        case NSM_QOS_CTYPE_T_SPEC_PEAK_RATE:
          TLV_DECODE_GETF (msg->peak_rate);
          NSM_SET_CTYPE (msg->cindex, tlv.type);
          break;
        case NSM_QOS_CTYPE_T_SPEC_QOS_COMMITED_BUCKET:
          TLV_DECODE_GETF (msg->rate);
          NSM_SET_CTYPE (msg->cindex, tlv.type);

          TLV_DECODE_GETF (msg->size);
          NSM_SET_CTYPE (msg->cindex, tlv.type);
          break;
        case NSM_QOS_CTYPE_T_SPEC_EXCESS_BURST_SIZE:
          TLV_DECODE_GETF (msg->excess_burst);
          NSM_SET_CTYPE (msg->cindex, tlv.type);
          break;
        case NSM_QOS_CTYPE_T_SPEC_WEIGHT:
          TLV_DECODE_GETF (msg->weight);
          NSM_SET_CTYPE (msg->cindex, tlv.type);
          break;
        default:
          TLV_DECODE_SKIP (tlv.length);
          break;
        }
    }

  return *pnt - sp;
}

s_int32_t
nsm_encode_if_spec (u_char **pnt, u_int16_t *size,
                    struct nsm_msg_qos_if_spec *msg)
{
  u_char *sp = *pnt;
  int i;

  for (i = 0; i < NSM_CINDEX_SIZE; i++)
    if (NSM_CHECK_CTYPE (msg->cindex, i))
      {
        if (*size < NSM_TLV_HEADER_SIZE)
          return NSM_ERR_PKT_TOO_SMALL;

        switch (i)
          {
          case NSM_QOS_CTYPE_IF_SPEC_IFINDEX:
            nsm_encode_tlv (pnt, size, i, 8);
            /* Encode 'in' interface index */
            TLV_ENCODE_PUTL (msg->ifindex.in);

            /* Encode 'out' interface index */
            TLV_ENCODE_PUTL (msg->ifindex.out);
            break;
          case NSM_QOS_CTYPE_IF_SPEC_ADDR:
            nsm_encode_tlv (pnt, size, i, 8);
            /* Encode 'in' interface address */
            TLV_ENCODE_PUT_IN4_ADDR (&msg->addr.prev_hop);

            /* Encode 'out' interface address */
            TLV_ENCODE_PUT_IN4_ADDR (&msg->addr.next_hop);
            break;
          default:
            break;
          }
      }

  return *pnt - sp;
}

s_int32_t
nsm_decode_if_spec (u_char **pnt, u_int16_t *size,
                    struct nsm_msg_qos_if_spec *msg)
{
  u_char *sp = *pnt;
  struct nsm_tlv_header tlv;

  /* Check size.  */
  if (*size < NSM_MSG_QOS_IF_SPEC_SIZE)
    return NSM_ERR_PKT_TOO_SMALL;

  /* TLV parse.  */
  while (*size)
    {
      if (*size < NSM_TLV_HEADER_SIZE)
        return NSM_ERR_PKT_TOO_SMALL;

      NSM_DECODE_TLV_HEADER (tlv);

      switch (tlv.type)
        {
        case NSM_QOS_CTYPE_IF_SPEC_IFINDEX:
          /* Encode 'in' interface index */
          TLV_DECODE_GETL (msg->ifindex.in);

          /* Encode 'out' interface index */
          TLV_DECODE_GETL (msg->ifindex.out);

          NSM_SET_CTYPE (msg->cindex, tlv.type);
          break;
        case NSM_QOS_CTYPE_IF_SPEC_ADDR:
          /* Encode 'in' interface address */
          TLV_DECODE_GET_IN4_ADDR (&msg->addr.prev_hop);

          /* Encode 'out' interface address */
          TLV_DECODE_GET_IN4_ADDR (&msg->addr.next_hop);

          NSM_SET_CTYPE (msg->cindex, tlv.type);
          break;
        default:
          TLV_DECODE_SKIP (tlv.length);
          break;
        }
    }

  return *pnt - sp;
}

s_int32_t
nsm_encode_ad_spec (u_char **pnt, u_int16_t *size,
                    struct nsm_msg_qos_ad_spec *msg)
{
  u_char *sp = *pnt;
  int i;

  for (i = 0; i < NSM_CINDEX_SIZE; i++)
    if (NSM_CHECK_CTYPE (msg->cindex, i))
      {
        if (*size < NSM_TLV_HEADER_SIZE)
          return NSM_ERR_PKT_TOO_SMALL;

        switch (i)
          {
          case NSM_QOS_CTYPE_AD_SPEC_SLACK:
            nsm_encode_tlv (pnt, size, i, 4);
            TLV_ENCODE_PUTF (msg->slack);
            break;
          case NSM_QOS_CTYPE_AD_SPEC_COMPOSE_TOTAL:
            nsm_encode_tlv (pnt, size, i, 4);
            TLV_ENCODE_PUTF (msg->compose_total);
            break;
          case NSM_QOS_CTYPE_AD_SPEC_DERIVED_TOTAL:
            nsm_encode_tlv (pnt, size, i, 4);
            TLV_ENCODE_PUTF (msg->derived_total);
            break;
          case NSM_QOS_CTYPE_AD_SPEC_COMPOSE_SUM:
            nsm_encode_tlv (pnt, size, i, 4);
            TLV_ENCODE_PUTF (msg->compose_sum);
            break;
          case NSM_QOS_CTYPE_AD_SPEC_DERIVED_SUM:
            nsm_encode_tlv (pnt, size, i, 4);
            TLV_ENCODE_PUTF (msg->derived_sum);
            break;
          case NSM_QOS_CTYPE_AD_SPEC_MIN_PATH_LATENCY:
            nsm_encode_tlv (pnt, size, i, 4);
            TLV_ENCODE_PUTF (msg->min_path_latency);
            break;
          case NSM_QOS_CTYPE_AD_SPEC_PATH_BW_EST:
            nsm_encode_tlv (pnt, size, i, 4);
            TLV_ENCODE_PUTF (msg->path_bw_est);
          default:
            break;
          }
      }

  return *pnt - sp;
}

s_int32_t
nsm_decode_ad_spec (u_char **pnt, u_int16_t *size,
                    struct nsm_msg_qos_ad_spec *msg)
{
  u_char *sp = *pnt;
  struct nsm_tlv_header tlv;

  /* No ned to check size as fields can be optional.  */

  /* TLV parse.  */
  while (*size)
    {
      if (*size < NSM_TLV_HEADER_SIZE)
        return NSM_ERR_PKT_TOO_SMALL;

      NSM_DECODE_TLV_HEADER (tlv);

      switch (tlv.type)
        {
        case NSM_QOS_CTYPE_AD_SPEC_SLACK:
          TLV_DECODE_GETF (msg->slack);
          NSM_SET_CTYPE (msg->cindex, tlv.type);
          break;
        case NSM_QOS_CTYPE_AD_SPEC_COMPOSE_TOTAL:
          TLV_DECODE_GETF (msg->compose_total);
          NSM_SET_CTYPE (msg->cindex, tlv.type);
          break;
        case NSM_QOS_CTYPE_AD_SPEC_DERIVED_TOTAL:
          TLV_DECODE_GETF (msg->derived_total);
          NSM_SET_CTYPE (msg->cindex, tlv.type);
          break;
        case NSM_QOS_CTYPE_AD_SPEC_COMPOSE_SUM:
          TLV_DECODE_GETF (msg->compose_sum);
          NSM_SET_CTYPE (msg->cindex, tlv.type);
          break;
        case NSM_QOS_CTYPE_AD_SPEC_DERIVED_SUM:
          TLV_DECODE_GETF (msg->derived_sum);
          NSM_SET_CTYPE (msg->cindex, tlv.type);
          break;
        case NSM_QOS_CTYPE_AD_SPEC_MIN_PATH_LATENCY:
          TLV_DECODE_GETF (msg->min_path_latency);
          NSM_SET_CTYPE (msg->cindex, tlv.type);
          break;
        case NSM_QOS_CTYPE_AD_SPEC_PATH_BW_EST:
          TLV_DECODE_GETF (msg->path_bw_est);
          NSM_SET_CTYPE (msg->cindex, tlv.type);
          break;
        default:
          TLV_DECODE_SKIP (tlv.length);
          break;
        }
    }

  return *pnt - sp;
}


s_int32_t
nsm_encode_qos_preempt (u_char **pnt,
                        u_int16_t *size,
                        struct nsm_msg_qos_preempt *msg)
{
  u_char *sp = *pnt;
  int i;

  /* Check size.  */
  if (*size < NSM_MSG_QOS_SIZE)
    return NSM_ERR_PKT_TOO_SMALL;

  /* Protocol ID. */
  TLV_ENCODE_PUTL (msg->protocol_id);

  /* Count. */
  TLV_ENCODE_PUTL (msg->count);

  if (msg->count && msg->preempted_ids == NULL)
    return -1;

  for (i = 0; i < msg->count; i++)
    TLV_ENCODE_PUTL (msg->preempted_ids[i]);

  return *pnt - sp;
}

s_int32_t
nsm_decode_qos_preempt (u_char **pnt,
                        u_int16_t *size,
                        struct nsm_msg_qos_preempt *msg)
{
  u_char *sp = *pnt;
  int i;

  /* Check size.  */
  if (*size < NSM_MSG_QOS_SIZE)
    return NSM_ERR_PKT_TOO_SMALL;

  /* Protocol ID. */
  TLV_DECODE_GETL (msg->protocol_id);

  /* Count. */
  TLV_DECODE_GETL (msg->count);

  if (msg->count)
    {
      msg->preempted_ids = XCALLOC (MTYPE_TMP,
                                    sizeof (u_int32_t) * msg->count);
      if (! msg->preempted_ids)
        return -1;

      for (i = 0; i < msg->count; i++)
        TLV_DECODE_GETL (msg->preempted_ids[i]);
    }
  else
    msg->preempted_ids = NULL;

  return *pnt - sp;
}



s_int32_t
nsm_encode_qos (u_char **pnt, u_int16_t *size,
                struct nsm_msg_qos *msg)
{
  u_char *sp = *pnt;
  u_char *spnt;
  int i, ret, nbytes;

  /* Check size.  */
  if (*size < NSM_MSG_QOS_SIZE)
    return NSM_ERR_PKT_TOO_SMALL;

  /* Resource ID. */
  TLV_ENCODE_PUTL (msg->resource_id);

  /* Protocol ID. */
  TLV_ENCODE_PUTL (msg->protocol_id);

  /* ID. */
  TLV_ENCODE_PUTL (msg->id);

  /* Encode owner the ipv6 routine encodes for ipv4 and ipv6. */
#ifdef HAVE_IPV6
  ret = nsm_encode_mpls_owner_ipv6 (pnt, size, &msg->owner);
#else
  ret = nsm_encode_mpls_owner_ipv4 (pnt, size, &msg->owner);
#endif
  if (ret < 0)
    return ret;

  /* Encode Class Type. */
  TLV_ENCODE_PUTC (msg->ct_num);

  /* TLV encode.  */
  for (i = 0; i < NSM_CINDEX_SIZE; i++)
    if (NSM_CHECK_CTYPE (msg->cindex, i))
      {
        if (*size < NSM_TLV_HEADER_SIZE)
          return NSM_ERR_PKT_TOO_SMALL;

        switch (i)
          {
          case NSM_QOS_CTYPE_SETUP_PRIORITY:
            nsm_encode_tlv (pnt, size, i, 1);
            TLV_ENCODE_PUTC (msg->setup_priority);
            break;
          case NSM_QOS_CTYPE_HOLD_PRIORITY:
            nsm_encode_tlv (pnt, size, i, 1);
            TLV_ENCODE_PUTC (msg->hold_priority);
            break;
          case NSM_QOS_CTYPE_STATUS:
            nsm_encode_tlv (pnt, size, i, 4);
            TLV_ENCODE_PUTL (msg->status);
            break;
          case NSM_QOS_CTYPE_T_SPEC:
            spnt = *pnt;
            *pnt += NSM_TLV_HEADER_SIZE;
            nbytes = nsm_encode_t_spec (pnt, size, &msg->t_spec);
            nsm_encode_tlv (&spnt, size, i, nbytes);
            break;
          case NSM_QOS_CTYPE_IF_SPEC:
            spnt = *pnt;
            *pnt += NSM_TLV_HEADER_SIZE;
            nbytes = nsm_encode_if_spec (pnt, size, &msg->if_spec);
            nsm_encode_tlv (&spnt, size, i, nbytes);
            break;
          case NSM_QOS_CTYPE_AD_SPEC:
            spnt = *pnt;
            *pnt += NSM_TLV_HEADER_SIZE;
            nbytes = nsm_encode_ad_spec (pnt, size, &msg->ad_spec);
            nsm_encode_tlv (&spnt, size, i, nbytes);
            break;
          default:
            break;
          }
      }

  return *pnt - sp;
}


s_int32_t
nsm_decode_qos (u_char **pnt, u_int16_t *size,
                struct nsm_msg_qos *msg)
{
  u_char *sp = *pnt;
  struct nsm_tlv_header tlv;
  int nbytes;
  u_int16_t len;
  int ret;

  /* Check size.  */
  if (*size < NSM_MSG_QOS_SIZE)
    return NSM_ERR_PKT_TOO_SMALL;

  /* Resource ID. */
  TLV_DECODE_GETL (msg->resource_id);

  /* Protocol ID. */
  TLV_DECODE_GETL (msg->protocol_id);

  /* ID. */
  TLV_DECODE_GETL (msg->id);

  /* Decode owner. */
#ifdef HAVE_IPV6
  if (msg->owner.u.b_key.u.ipv6.p.family == AF_INET6)
    ret = nsm_decode_mpls_owner_ipv6 (pnt, size, &msg->owner);
  else
#endif
    ret = nsm_decode_mpls_owner_ipv4 (pnt, size, &msg->owner);

  if (ret < 0)
    return ret;

  /* Decode Class Type. */
  TLV_DECODE_GETC (msg->ct_num);

  /* TLV parse. */
  while (*size)
    {
      if (*size < NSM_TLV_HEADER_SIZE)
        return NSM_ERR_PKT_TOO_SMALL;

      NSM_DECODE_TLV_HEADER (tlv);

      switch (tlv.type)
        {
        case NSM_QOS_CTYPE_SETUP_PRIORITY:
          TLV_DECODE_GETC (msg->setup_priority);
          NSM_SET_CTYPE (msg->cindex, tlv.type);
          break;
        case NSM_QOS_CTYPE_HOLD_PRIORITY:
          TLV_DECODE_GETC (msg->hold_priority);
          NSM_SET_CTYPE (msg->cindex, tlv.type);
          break;
        case NSM_QOS_CTYPE_STATUS:
          TLV_DECODE_GETL (msg->status);
          NSM_SET_CTYPE (msg->cindex, tlv.type);
          break;
        case NSM_QOS_CTYPE_T_SPEC:
          len = tlv.length;
          nbytes = nsm_decode_t_spec (pnt, &len, &msg->t_spec);
          *size -= nbytes;
          NSM_SET_CTYPE (msg->cindex, tlv.type);
          break;
        case NSM_QOS_CTYPE_IF_SPEC:
          len = tlv.length;
          nbytes = nsm_decode_if_spec (pnt, &len, &msg->if_spec);
          *size -= nbytes;
          NSM_SET_CTYPE (msg->cindex, tlv.type);
          break;
        case NSM_QOS_CTYPE_AD_SPEC:
          len = tlv.length;
          nbytes = nsm_decode_ad_spec (pnt, &len, &msg->ad_spec);
          *size -= nbytes;
          NSM_SET_CTYPE (msg->cindex, tlv.type);
          break;
        default:
          TLV_DECODE_SKIP (tlv.length);
          break;
        }
    }

  return *pnt - sp;
}

s_int32_t
nsm_encode_qos_release (u_char **pnt, u_int16_t *size,
                        struct nsm_msg_qos_release *msg)
{
  u_char *sp = *pnt;
  int i;

  /* Check size.  */
  if (*size < NSM_MSG_QOS_RELEASE_SIZE)
    return NSM_ERR_PKT_TOO_SMALL;

  /* Resource ID. */
  TLV_ENCODE_PUTL (msg->resource_id);

  /* Protocol ID. */
  TLV_ENCODE_PUTL (msg->protocol_id);

  /* TLV encode.  */
  for (i = 0; i < NSM_CINDEX_SIZE; i++)
    if (NSM_CHECK_CTYPE (msg->cindex, i))
      {
        if (*size < NSM_TLV_HEADER_SIZE)
          return NSM_ERR_PKT_TOO_SMALL;

        switch (i)
          {
          case NSM_QOS_RELEASE_CTYPE_IFINDEX:
            nsm_encode_tlv (pnt, size, i, 4);
            TLV_ENCODE_PUTL (msg->ifindex);
            break;
          case NSM_QOS_RELEASE_CTYPE_STATUS:
            nsm_encode_tlv (pnt, size, i, 4);
            TLV_ENCODE_PUTL (msg->status);
            break;
          default:
            break;
          }
      }

  return *pnt -sp;
}

s_int32_t
nsm_decode_qos_release (u_char **pnt, u_int16_t *size,
                        struct nsm_msg_qos_release *msg)
{
  u_char *sp = *pnt;
  struct nsm_tlv_header tlv;

  /* Check size.  */
  if (*size < NSM_MSG_QOS_RELEASE_SIZE)
    return NSM_ERR_PKT_TOO_SMALL;

  /* Resource ID. */
  TLV_DECODE_GETL (msg->resource_id);

  /* Protocol ID. */
  TLV_DECODE_GETL (msg->protocol_id);

  /* TLV parse.  */
  while (*size)
    {
      if (*size < NSM_TLV_HEADER_SIZE)
        return NSM_ERR_PKT_TOO_SMALL;

      NSM_DECODE_TLV_HEADER (tlv);

      switch (tlv.type)
        {
        case NSM_QOS_RELEASE_CTYPE_IFINDEX:
          TLV_DECODE_GETL (msg->ifindex);
          NSM_SET_CTYPE (msg->cindex, tlv.type);
          break;
        case NSM_QOS_RELEASE_CTYPE_STATUS:
          TLV_DECODE_GETL (msg->status);
          NSM_SET_CTYPE (msg->cindex, tlv.type);
          break;
        default:
          TLV_DECODE_SKIP (tlv.length);
          break;
        }
    }

  return *pnt - sp;
}

s_int32_t
nsm_encode_qos_clean (u_char **pnt, u_int16_t *size,
                      struct nsm_msg_qos_clean *msg)
{
  u_char *sp = *pnt;
  int i;

  /* Check size. */
  if (*size < NSM_MSG_QOS_CLEAN_SIZE)
    return NSM_ERR_PKT_TOO_SMALL;

  /* Protocol ID. */
  TLV_ENCODE_PUTL (msg->protocol_id);

  /* TLV encode.  */
  for (i = 0; i < NSM_CINDEX_SIZE; i++)
    if (NSM_CHECK_CTYPE (msg->cindex, i))
      {
        if (*size < NSM_TLV_HEADER_SIZE)
          return NSM_ERR_PKT_TOO_SMALL;

        switch (i)
          {
          case NSM_QOS_CLEAN_CTYPE_IFINDEX:
            nsm_encode_tlv (pnt, size, i, 4);
            TLV_ENCODE_PUTL (msg->ifindex);
            break;
          default:
            break;
          }
      }

  return *pnt - sp;
}

s_int32_t
nsm_decode_qos_clean (u_char **pnt, u_int16_t *size,
                      struct nsm_msg_qos_clean *msg)
{
  u_char *sp = *pnt;
  struct nsm_tlv_header tlv;

  /* Check size. */
  if (*size < NSM_MSG_QOS_CLEAN_SIZE)
    return NSM_ERR_PKT_TOO_SMALL;

  /* Protocol ID. */
  TLV_DECODE_GETL (msg->protocol_id);

  /* TLV parse.  */
  while (*size)
    {
      if (*size < NSM_TLV_HEADER_SIZE)
        return NSM_ERR_PKT_TOO_SMALL;

      NSM_DECODE_TLV_HEADER (tlv);

      switch (tlv.type)
        {
        case NSM_QOS_CLEAN_CTYPE_IFINDEX:
          TLV_DECODE_GETL (msg->ifindex);
          NSM_SET_CTYPE (msg->cindex, tlv.type);
          break;
        default:
          TLV_DECODE_SKIP (tlv.length);
          break;
        }
    }

  return *pnt - sp;
}


void
nsm_admin_group_dump (struct lib_globals *zg,
                      struct nsm_msg_admin_group *msg)
{
  int i;
  struct admin_group *array;

  zlog_info (zg, "Admin Group");

  array = msg->array;

  for (i = 0; i < ADMIN_GROUP_MAX; i++)
    if (array[i].val >= 0)
      zlog_info (zg, " Index/Name: %d - %s", array[i].val, array[i].name);
}

s_int32_t
nsm_encode_admin_group (u_char **pnt, u_int16_t *size,
                        struct nsm_msg_admin_group *msg)
{
  int i;
  u_char *sp = *pnt;
  struct admin_group *array;

  array = msg->array;

  /* Admin group encode.  */
  for (i = 0; i < ADMIN_GROUP_MAX; i++)
    {
      TLV_ENCODE_PUTL (array[i].val);
      TLV_ENCODE_PUT (array[i].name, ADMIN_GROUP_NAMSIZ);
    }
  return *pnt -sp;
}

s_int32_t
nsm_decode_admin_group (u_char **pnt, u_int16_t *size,
                        struct nsm_msg_admin_group *msg)
{
  int i;
  u_char *sp = *pnt;
  struct admin_group *array;

  array = msg->array;

  /* Admin group decode.  */
  for (i = 0; i < ADMIN_GROUP_MAX; i++)
    {
      TLV_DECODE_GETL (array[i].val);
      TLV_DECODE_GET (array[i].name, ADMIN_GROUP_NAMSIZ);
    }
  return *pnt -sp;
}
#endif /* HAVE_TE */

#ifdef HAVE_GMPLS
#ifdef HAVE_PACKET
void
qos_prot_nsm_message_map_gmpls (struct qos_resource *resource,
                                struct nsm_msg_gmpls_qos *msg)
{
  struct nsm_msg_qos_t_spec *dt_spec;
  struct qos_traffic_spec *st_spec;
  struct nsm_msg_qos_ad_spec *dad_spec;
  struct qos_ad_spec *sad_spec;
  struct nsm_msg_gmpls_qos_if_spec *dgif_spec;
  struct gmpls_qos_if_spec *sgif_spec;
  struct nsm_msg_gmpls_qos_attr *dgmpls_attr;
  struct gmpls_qos_attr *sgmpls_attr;
  struct nsm_msg_gmpls_qos_label_spec *dlabel_spec;
  struct gmpls_qos_label_spec *slabel_spec;

  /* Cindex. */
  msg->cindex = 0;

  msg->resource_id = resource->resource_id;
  msg->protocol_id = resource->protocol_id;
  msg->id = resource->id;

  /* Copy over owner. */
  pal_mem_cpy (&msg->owner, &resource->owner, sizeof (struct mpls_owner));

  /* Copy Class Type number. */
  msg->ct_num = resource->ct_num;

  if (CHECK_FLAG (resource->flags, QOS_RESOURCE_FLAG_SETUP_PRIORITY))
    {
      NSM_SET_CTYPE (msg->cindex, NSM_QOS_CTYPE_SETUP_PRIORITY);
      msg->setup_priority = resource->setup_priority;
    }

  if (CHECK_FLAG (resource->flags, QOS_RESOURCE_FLAG_HOLD_PRIORITY))
    {
      NSM_SET_CTYPE (msg->cindex, NSM_QOS_CTYPE_HOLD_PRIORITY);
      msg->hold_priority = resource->hold_priority;
    }

  if (CHECK_FLAG (resource->flags, QOS_RESOURCE_FLAG_TRAFFIC_SPEC))
    {
      st_spec = &resource->t_spec;
      dt_spec = &msg->t_spec;

      /* Tspec cindex. */
      dt_spec->cindex = 0;
      NSM_SET_CTYPE (msg->cindex, NSM_QOS_CTYPE_T_SPEC);

      dt_spec->service_type = st_spec->service_type;
      dt_spec->is_exclusive = st_spec->is_exclusive;

      if (CHECK_FLAG (st_spec->flags, QOS_TSPEC_FLAG_PEAK_RATE))
        {
          NSM_SET_CTYPE (dt_spec->cindex, NSM_QOS_CTYPE_T_SPEC_PEAK_RATE);
          dt_spec->peak_rate = st_spec->peak_rate;
        }
      if (CHECK_FLAG (st_spec->flags, QOS_TSPEC_FLAG_COMMITTED_BUCKET))
        {
          NSM_SET_CTYPE (dt_spec->cindex,
                         NSM_QOS_CTYPE_T_SPEC_QOS_COMMITED_BUCKET);

          dt_spec->rate = st_spec->committed_bucket.rate;
          dt_spec->size = st_spec->committed_bucket.size;
        }
      if (CHECK_FLAG (st_spec->flags, QOS_TSPEC_FLAG_EXCESS_BURST))
        {
          NSM_SET_CTYPE (dt_spec->cindex,
                         NSM_QOS_CTYPE_T_SPEC_EXCESS_BURST_SIZE);
          dt_spec->excess_burst = st_spec->excess_burst;
        }
      if (CHECK_FLAG (st_spec->flags, QOS_TSPEC_FLAG_WEIGHT))
        {
          NSM_SET_CTYPE (dt_spec->cindex, NSM_QOS_CTYPE_T_SPEC_WEIGHT);
          dt_spec->weight = st_spec->weight;
        }
      if (CHECK_FLAG (st_spec->flags, QOS_TSPEC_FLAG_MIN_POLICED_UNIT))
        {
          NSM_SET_CTYPE (dt_spec->cindex,
                         NSM_QOS_CTYPE_T_SPEC_MIN_POLICED_UNIT);
          dt_spec->min_policed_unit = st_spec->mpu;
        }
      if (CHECK_FLAG (st_spec->flags, QOS_TSPEC_FLAG_MAX_PACKET_SIZE))
        {
          NSM_SET_CTYPE (dt_spec->cindex,
                         NSM_QOS_CTYPE_T_SPEC_MAX_PACKET_SIZE);
          dt_spec->max_packet_size = st_spec->max_packet_size;
        }
    }

  if (CHECK_FLAG (resource->flags, QOS_RESOURCE_FLAG_AD_SPEC))
    {
      dad_spec = &msg->ad_spec;
      sad_spec = &resource->ad_spec;

      /* ADspec cindex. */
      dad_spec->cindex = 0;
      NSM_SET_CTYPE (msg->cindex, NSM_QOS_CTYPE_AD_SPEC);

      if (CHECK_FLAG (sad_spec->flags, QOS_ADSPEC_FLAG_SLACK))
        {
          NSM_SET_CTYPE (dad_spec->cindex, NSM_QOS_CTYPE_AD_SPEC_SLACK);
          dad_spec->slack = sad_spec->slack;
        }
      if (CHECK_FLAG (sad_spec->flags, QOS_ADSPEC_FLAG_CTOT))
        {
          NSM_SET_CTYPE (dad_spec->cindex,
                         NSM_QOS_CTYPE_AD_SPEC_COMPOSE_TOTAL);
          dad_spec->compose_total = sad_spec->Ctot;
        }
      if (CHECK_FLAG (sad_spec->flags, QOS_ADSPEC_FLAG_DTOT))
        {
          NSM_SET_CTYPE (dad_spec->cindex,
                         NSM_QOS_CTYPE_AD_SPEC_DERIVED_TOTAL);
          dad_spec->derived_total = sad_spec->Dtot;
        }
      if (CHECK_FLAG (sad_spec->flags, QOS_ADSPEC_FLAG_CSUM))
        {
          NSM_SET_CTYPE (dad_spec->cindex, NSM_QOS_CTYPE_AD_SPEC_COMPOSE_SUM);
          dad_spec->compose_sum = sad_spec->Csum;
        }
      if (CHECK_FLAG (sad_spec->flags, QOS_ADSPEC_FLAG_DSUM))
        {
          NSM_SET_CTYPE (dad_spec->cindex, NSM_QOS_CTYPE_AD_SPEC_DERIVED_SUM);
          dad_spec->derived_sum = sad_spec->Dsum;
        }
      if (CHECK_FLAG (sad_spec->flags, QOS_ADSPEC_FLAG_MPL))
        {
          NSM_SET_CTYPE (dad_spec->cindex,
                         NSM_QOS_CTYPE_AD_SPEC_MIN_PATH_LATENCY);
          dad_spec->min_path_latency = sad_spec->mpl;
        }
      if (CHECK_FLAG (sad_spec->flags, QOS_ADSPEC_FLAG_PBE))
        {
          NSM_SET_CTYPE (dad_spec->cindex, NSM_QOS_CTYPE_AD_SPEC_PATH_BW_EST);
          dad_spec->path_bw_est = sad_spec->pbe;
        }
    }

  if (CHECK_FLAG (resource->flags, QOS_RESOURCE_FLAG_GMPLS_REVERSE))
    msg->direction = GMPLS_LSP_REVERSE;
  else
    msg->direction = GMPLS_LSP_FORWARD;

  if (CHECK_FLAG (resource->flags, QOS_RESOURCE_FLAG_GMPLS_IF_SPEC))
    {
      dgif_spec = &msg->gif_spec;
      sgif_spec = &resource->gif_spec;

      /* GMPLS IFspec cindex. */
      dgif_spec->cindex = 0;
      NSM_SET_CTYPE (msg->cindex, NSM_QOS_CTYPE_GMPLS_IF_SPEC);

      if (CHECK_FLAG (sgif_spec->flags, GMPLS_QOS_IFSPEC_FLAG_TL_GIFINDEX))
        {
          NSM_SET_CTYPE (dgif_spec->cindex,
                         NSM_QOS_CTYPE_GMPLS_IF_SPEC_TL_GIFINDEX);
          dgif_spec->tel_gifindex = sgif_spec->tel_gifindex;
        }
      if (CHECK_FLAG (sgif_spec->flags, GMPLS_QOS_IFSPEC_FLAG_DL_GIFINDEX))
        {
          NSM_SET_CTYPE (dgif_spec->cindex,
                         NSM_QOS_CTYPE_GMPLS_IF_SPEC_DL_GIFINDEX);
          dgif_spec->dl_gifindex = sgif_spec->dl_gifindex;
        }
      if (CHECK_FLAG (sgif_spec->flags, GMPLS_QOS_IFSPEC_FLAG_CA_RTR_ID))
        {
          NSM_SET_CTYPE (dgif_spec->cindex,
                         NSM_QOS_CTYPE_GMPLS_IF_SPEC_CA_RTR_ID);
          dgif_spec->rtr_id = sgif_spec->rtr_id;
        }
    }

  if (CHECK_FLAG (resource->flags, QOS_RESOURCE_FLAG_GMPLS_ATTR))
    {
      dgmpls_attr = &msg->gmpls_attr;
      sgmpls_attr = &resource->gmpls_attr;

      /* GMPLS QoS Attribute cindex. */
      dgmpls_attr->cindex = 0;
      NSM_SET_CTYPE (msg->cindex, NSM_QOS_CTYPE_GMPLS_ATTR);

      if (CHECK_FLAG (sgmpls_attr->flags, GMPLS_QOS_ATTR_FLAG_PROTECTION))
        {
          NSM_SET_CTYPE (dgmpls_attr->cindex,
                         NSM_QOS_CTYPE_GMPLS_ATTR_PROTECTION);
          dgmpls_attr->protection = sgmpls_attr->protection;
        }

      if (CHECK_FLAG (sgmpls_attr->flags, GMPLS_QOS_ATTR_FLAG_GPID))
        {
          NSM_SET_CTYPE (dgmpls_attr->cindex,
                         NSM_QOS_CTYPE_GMPLS_ATTR_GPID);
          dgmpls_attr->gpid = sgmpls_attr->gpid;
        }

      NSM_SET_CTYPE (dgmpls_attr->cindex, NSM_QOS_CTYPE_GMPLS_ATTR_SW_CAP);
      dgmpls_attr->sw_cap = sgmpls_attr->sw_cap;
      dgmpls_attr->encoding = sgmpls_attr->encoding;
    }

  if (CHECK_FLAG (resource->flags, QOS_RESOURCE_FLAG_GMPLS_LABEL_SPEC))
    {
      dlabel_spec = &msg->label_spec;
      slabel_spec = &resource->label_spec;

      /* GMPLS Label Spec cindex. */
      dlabel_spec->cindex = 0;
      NSM_SET_CTYPE (msg->cindex, NSM_QOS_CTYPE_GMPLS_LABEL_SPEC);

      if (CHECK_FLAG (slabel_spec->flags, GMPLS_QOS_LABEL_SPEC_LABEL_SET))
        {
          NSM_SET_CTYPE (dlabel_spec->cindex,
                         NSM_QOS_CTYPE_GMPLS_LABEL_SPEC_LABEL_SET);
          dlabel_spec->sl_lbl_set_from = slabel_spec->sl_lbl_set_from;
          dlabel_spec->sl_lbl_set_to = slabel_spec->sl_lbl_set_to;
        }

      if (CHECK_FLAG (slabel_spec->flags, GMPLS_QOS_LABEL_SPEC_SUGG_LABEL))
        {
          NSM_SET_CTYPE (dlabel_spec->cindex,
                         NSM_QOS_CTYPE_GMPLS_LABEL_SPEC_SUGG_LABEL);
          dlabel_spec->sugg_label = slabel_spec->sugg_label;
        }
    }
}

void
qos_nsm_prot_message_map_gmpls (struct nsm_msg_gmpls_qos *msg,
                                struct qos_resource *resource)
{
  struct nsm_msg_qos_t_spec *st_spec;
  struct qos_traffic_spec *dt_spec;
  struct nsm_msg_qos_ad_spec *sad_spec;
  struct qos_ad_spec *dad_spec;
  struct nsm_msg_gmpls_qos_if_spec *sgif_spec;
  struct gmpls_qos_if_spec *dgif_spec;
  struct nsm_msg_gmpls_qos_attr *sgmpls_attr;
  struct gmpls_qos_attr *dgmpls_attr;
  struct nsm_msg_gmpls_qos_label_spec *slabel_spec;
  struct gmpls_qos_label_spec *dlabel_spec;

  resource->flags = 0;

  resource->resource_id = msg->resource_id;
  resource->protocol_id = msg->protocol_id;
  resource->id = msg->id;

  /* Copy over MPLS Owner. */
  pal_mem_cpy (&resource->owner, &msg->owner, sizeof (struct mpls_owner));

  /* Copy Class Type number. */
  resource->ct_num = msg->ct_num;

  if (NSM_CHECK_CTYPE (msg->cindex, NSM_QOS_CTYPE_SETUP_PRIORITY))
    {
      SET_FLAG (resource->flags, QOS_RESOURCE_FLAG_SETUP_PRIORITY);
      resource->setup_priority = msg->setup_priority;
    }

  if (NSM_CHECK_CTYPE (msg->cindex, NSM_QOS_CTYPE_HOLD_PRIORITY))
    {
      SET_FLAG (resource->flags, QOS_RESOURCE_FLAG_HOLD_PRIORITY);
      resource->hold_priority = msg->hold_priority;
    }

  if (NSM_CHECK_CTYPE (msg->cindex, NSM_QOS_CTYPE_T_SPEC))
    {
      dt_spec = &resource->t_spec;
      st_spec = &msg->t_spec;

      /* Tspec cindex. */
      dt_spec->flags = 0;
      SET_FLAG (resource->flags, QOS_RESOURCE_FLAG_TRAFFIC_SPEC);

      dt_spec->service_type = st_spec->service_type;
      dt_spec->is_exclusive = st_spec->is_exclusive;

      if (NSM_CHECK_CTYPE (st_spec->cindex, NSM_QOS_CTYPE_T_SPEC_PEAK_RATE))
        {
          SET_FLAG (dt_spec->flags , QOS_TSPEC_FLAG_PEAK_RATE);
          dt_spec->peak_rate = st_spec->peak_rate;
        }
      if (NSM_CHECK_CTYPE (st_spec->cindex,
                           NSM_QOS_CTYPE_T_SPEC_QOS_COMMITED_BUCKET))
        {
          SET_FLAG (dt_spec->flags, QOS_TSPEC_FLAG_COMMITTED_BUCKET);
          SET_FLAG (dt_spec->committed_bucket.flags, QOS_BUCKET_RATE);
          SET_FLAG (dt_spec->committed_bucket.flags, QOS_BUCKET_SIZE);

          dt_spec->committed_bucket.rate = st_spec->rate;
          dt_spec->committed_bucket.size = st_spec->size;
        }
      if (NSM_CHECK_CTYPE (st_spec->cindex,
                           NSM_QOS_CTYPE_T_SPEC_EXCESS_BURST_SIZE))
        {
          SET_FLAG (dt_spec->flags, QOS_TSPEC_FLAG_EXCESS_BURST);
          dt_spec->excess_burst = st_spec->excess_burst;
        }
      if (NSM_CHECK_CTYPE (st_spec->cindex, NSM_QOS_CTYPE_T_SPEC_WEIGHT))
        {
          SET_FLAG (dt_spec->flags, QOS_TSPEC_FLAG_WEIGHT);
          dt_spec->weight = st_spec->weight;
        }
      if (NSM_CHECK_CTYPE (st_spec->cindex,
                           NSM_QOS_CTYPE_T_SPEC_MIN_POLICED_UNIT))
        {
          SET_FLAG (dt_spec->flags, QOS_TSPEC_FLAG_MIN_POLICED_UNIT);
          dt_spec->mpu = st_spec->min_policed_unit;
        }
      if (NSM_CHECK_CTYPE (st_spec->cindex,
                           NSM_QOS_CTYPE_T_SPEC_MAX_PACKET_SIZE))
        {
          SET_FLAG (dt_spec->flags, QOS_TSPEC_FLAG_MAX_PACKET_SIZE);
          dt_spec->max_packet_size = st_spec->max_packet_size;
        }
    }

  if (CHECK_FLAG (resource->flags, QOS_RESOURCE_FLAG_AD_SPEC))
    if (NSM_CHECK_CTYPE (msg->cindex, NSM_QOS_CTYPE_AD_SPEC))
      {
        sad_spec = &msg->ad_spec;
        dad_spec = &resource->ad_spec;

        dad_spec->flags = 0;
        SET_FLAG (resource->flags, QOS_RESOURCE_FLAG_AD_SPEC);

        if (NSM_CHECK_CTYPE (sad_spec->cindex, NSM_QOS_CTYPE_AD_SPEC))
          {
            SET_FLAG (dad_spec->flags, QOS_ADSPEC_FLAG_SLACK);
            dad_spec->slack = sad_spec->slack;
          }
        if (NSM_CHECK_CTYPE (sad_spec->cindex,
                             NSM_QOS_CTYPE_AD_SPEC_COMPOSE_TOTAL))
          {
            SET_FLAG (dad_spec->flags, QOS_ADSPEC_FLAG_CTOT);
            dad_spec->Ctot = sad_spec->compose_total;
          }
        if (NSM_CHECK_CTYPE (sad_spec->cindex,
                             NSM_QOS_CTYPE_AD_SPEC_DERIVED_TOTAL))
          {
            SET_FLAG (dad_spec->flags, QOS_ADSPEC_FLAG_DTOT);
            dad_spec->Dtot = sad_spec->derived_total;
          }
        if (NSM_CHECK_CTYPE (sad_spec->cindex,
                             NSM_QOS_CTYPE_AD_SPEC_COMPOSE_SUM))
          {
            SET_FLAG (dad_spec->flags, QOS_ADSPEC_FLAG_CSUM);
            dad_spec->Csum = sad_spec->compose_sum;
          }
        if (NSM_CHECK_CTYPE (sad_spec->cindex,
                             NSM_QOS_CTYPE_AD_SPEC_DERIVED_SUM))
          {
            SET_FLAG (dad_spec->flags, QOS_ADSPEC_FLAG_DSUM);
            dad_spec->Dsum = sad_spec->derived_sum;
          }
        if (NSM_CHECK_CTYPE (sad_spec->cindex,
                             NSM_QOS_CTYPE_AD_SPEC_MIN_PATH_LATENCY))
          {
            SET_FLAG (dad_spec->flags, QOS_ADSPEC_FLAG_MPL);
            dad_spec->mpl = sad_spec->min_path_latency;
          }
        if (NSM_CHECK_CTYPE (sad_spec->cindex,
                             NSM_QOS_CTYPE_AD_SPEC_PATH_BW_EST))
          {
            SET_FLAG (dad_spec->flags, QOS_ADSPEC_FLAG_PBE);
            dad_spec->pbe = sad_spec->path_bw_est;
          }
      }

  if (msg->direction == GMPLS_LSP_REVERSE)
    SET_FLAG (resource->flags, QOS_RESOURCE_FLAG_GMPLS_REVERSE);

  if (NSM_CHECK_CTYPE (msg->cindex, NSM_QOS_CTYPE_GMPLS_IF_SPEC))
    {
      sgif_spec = &msg->gif_spec;
      dgif_spec = &resource->gif_spec;

      dgif_spec->flags = 0;
      SET_FLAG (resource->flags, QOS_RESOURCE_FLAG_GMPLS_IF_SPEC);

      if (NSM_CHECK_CTYPE (sgif_spec->cindex, NSM_QOS_CTYPE_GMPLS_IF_SPEC_TL_GIFINDEX))
        {
          SET_FLAG (dgif_spec->flags, GMPLS_QOS_IFSPEC_FLAG_TL_GIFINDEX);
          dgif_spec->tel_gifindex = sgif_spec->tel_gifindex;
        }
      if (NSM_CHECK_CTYPE (sgif_spec->cindex, NSM_QOS_CTYPE_GMPLS_IF_SPEC_DL_GIFINDEX))
        {
          SET_FLAG (dgif_spec->flags, GMPLS_QOS_IFSPEC_FLAG_DL_GIFINDEX);
          dgif_spec->dl_gifindex = sgif_spec->dl_gifindex;
        }
      if (NSM_CHECK_CTYPE (sgif_spec->cindex, NSM_QOS_CTYPE_GMPLS_IF_SPEC_CA_RTR_ID))
        {
          SET_FLAG (dgif_spec->flags, GMPLS_QOS_IFSPEC_FLAG_CA_RTR_ID);
          dgif_spec->rtr_id = sgif_spec->rtr_id;
        }
    }

  if (NSM_CHECK_CTYPE (msg->cindex, NSM_QOS_CTYPE_GMPLS_ATTR))
    {
      sgmpls_attr = &msg->gmpls_attr;
      dgmpls_attr = &resource->gmpls_attr;

      dgmpls_attr->flags = 0;
      SET_FLAG (resource->flags, QOS_RESOURCE_FLAG_GMPLS_ATTR);

      if (NSM_CHECK_CTYPE (sgmpls_attr->cindex, NSM_QOS_CTYPE_GMPLS_ATTR_PROTECTION))
        {
          SET_FLAG (dgmpls_attr->flags, GMPLS_QOS_ATTR_FLAG_PROTECTION);
          dgmpls_attr->protection = sgmpls_attr->protection;
        }
      if (NSM_CHECK_CTYPE (sgmpls_attr->cindex, NSM_QOS_CTYPE_GMPLS_ATTR_GPID))
        {
          SET_FLAG (dgmpls_attr->flags, GMPLS_QOS_ATTR_FLAG_GPID);
          dgmpls_attr->gpid = sgmpls_attr->gpid;
        }

      dgmpls_attr->sw_cap = sgmpls_attr->sw_cap;
      dgmpls_attr->encoding = sgmpls_attr->encoding;
    }

  if (NSM_CHECK_CTYPE (msg->cindex, NSM_QOS_CTYPE_GMPLS_LABEL_SPEC))
    {
      slabel_spec = &msg->label_spec;
      dlabel_spec = &resource->label_spec;

      dlabel_spec->flags = 0;
      SET_FLAG (resource->flags, QOS_RESOURCE_FLAG_GMPLS_LABEL_SPEC);

      if (NSM_CHECK_CTYPE (slabel_spec->cindex, NSM_QOS_CTYPE_GMPLS_LABEL_SPEC_LABEL_SET))
        {
          SET_FLAG (dlabel_spec->flags, GMPLS_QOS_LABEL_SPEC_LABEL_SET);
          dlabel_spec->sl_lbl_set_from = slabel_spec->sl_lbl_set_from;
          dlabel_spec->sl_lbl_set_to = slabel_spec->sl_lbl_set_to;
        }
      if (NSM_CHECK_CTYPE (slabel_spec->cindex, NSM_QOS_CTYPE_GMPLS_LABEL_SPEC_SUGG_LABEL))
        {
          SET_FLAG (dlabel_spec->flags, GMPLS_QOS_LABEL_SPEC_SUGG_LABEL);
          dlabel_spec->sugg_label = slabel_spec->sugg_label;
        }
    }
}

s_int32_t
nsm_encode_gmpls_if_spec (u_char **pnt, u_int16_t *size,
                          struct nsm_msg_gmpls_qos_if_spec *msg)
{
  u_char *sp = *pnt;
  int i;

  for (i = 0; i < NSM_CINDEX_SIZE; i++)
    if (NSM_CHECK_CTYPE (msg->cindex, i))
      {
        if (*size < NSM_TLV_HEADER_SIZE)
          return NSM_ERR_PKT_TOO_SMALL;

        switch (i)
          {
          case NSM_QOS_CTYPE_GMPLS_IF_SPEC_TL_GIFINDEX:
          /* Encode TE Link gIFindex */
            nsm_encode_tlv (pnt, size, i, 4);
            TLV_ENCODE_PUTL (msg->tel_gifindex);
            break;
          case NSM_QOS_CTYPE_GMPLS_IF_SPEC_DL_GIFINDEX:
          /* Encode Data Link gIFindex */
            nsm_encode_tlv (pnt, size, i, 4);
            TLV_ENCODE_PUTL (msg->dl_gifindex);
            break;
          case NSM_QOS_CTYPE_GMPLS_IF_SPEC_CA_RTR_ID:
          /* Encode Data Link gIFindex */
            nsm_encode_tlv (pnt, size, i, 4);
            TLV_ENCODE_PUT_IN4_ADDR (&msg->rtr_id);
            break;
          default:
            break;
          }
      }

  return *pnt - sp;
}

s_int32_t
nsm_decode_gmpls_if_spec (u_char **pnt, u_int16_t *size,
                          struct nsm_msg_gmpls_qos_if_spec *msg)
{
  u_char *sp = *pnt;
  struct nsm_tlv_header tlv;

  /* Check size.  */
  if (*size < NSM_MSG_GMPLS_QOS_IF_SPEC_SIZE)
    return NSM_ERR_PKT_TOO_SMALL;

  /* TLV parse.  */
  while (*size)
    {
      if (*size < NSM_TLV_HEADER_SIZE)
        return NSM_ERR_PKT_TOO_SMALL;

      NSM_DECODE_TLV_HEADER (tlv);

      switch (tlv.type)
        {
        case NSM_QOS_CTYPE_GMPLS_IF_SPEC_TL_GIFINDEX:
          /* Decode TE Link gIFindex */
          TLV_DECODE_GETL (msg->tel_gifindex);
          NSM_SET_CTYPE (msg->cindex, tlv.type);
          break;
        case NSM_QOS_CTYPE_GMPLS_IF_SPEC_DL_GIFINDEX:
          /* Decode Data Link gIFindex */
          TLV_DECODE_GETL (msg->dl_gifindex);
          NSM_SET_CTYPE (msg->cindex, tlv.type);
          break;
        case NSM_QOS_CTYPE_GMPLS_IF_SPEC_CA_RTR_ID:
          /* Decode RTR ID */
          TLV_DECODE_GET_IN4_ADDR (&msg->rtr_id);
          NSM_SET_CTYPE (msg->cindex, tlv.type);
          break;
        default:
          TLV_DECODE_SKIP (tlv.length);
          break;
        }
    }

  return *pnt - sp;
}

s_int32_t
nsm_encode_gmpls_attr (u_char **pnt, u_int16_t *size,
                       struct nsm_msg_gmpls_qos_attr *msg)
{
  u_char *sp = *pnt;
  int i;

  for (i = 0; i < NSM_CINDEX_SIZE; i++)
    if (NSM_CHECK_CTYPE (msg->cindex, i))
      {
        if (*size < NSM_TLV_HEADER_SIZE)
          return NSM_ERR_PKT_TOO_SMALL;

        switch (i)
          {
          case NSM_QOS_CTYPE_GMPLS_ATTR_SW_CAP:
          /* Encode SW CAP and Encoding Type */
            nsm_encode_tlv (pnt, size, i, 2);
            TLV_ENCODE_PUTC (msg->sw_cap);
            TLV_ENCODE_PUTC (msg->encoding);
            break;
          case NSM_QOS_CTYPE_GMPLS_ATTR_PROTECTION:
          /* Encode Protection */
            nsm_encode_tlv (pnt, size, i, 1);
            TLV_ENCODE_PUTC (msg->protection);
            break;
          case NSM_QOS_CTYPE_GMPLS_ATTR_GPID:
          /* Encode Protection */
            nsm_encode_tlv (pnt, size, i, 2);
            TLV_ENCODE_PUTW (msg->gpid);
            break;
          default:
            break;
          }
      }

  return *pnt - sp;
}

s_int32_t
nsm_decode_gmpls_attr (u_char **pnt, u_int16_t *size,
                       struct nsm_msg_gmpls_qos_attr *msg)
{
  u_char *sp = *pnt;
  struct nsm_tlv_header tlv;

  /* Check size.  */
  if (*size < NSM_MSG_GMPLS_QOS_ATTR_SIZE)
    return NSM_ERR_PKT_TOO_SMALL;

  /* TLV parse.  */
  while (*size)
    {
      if (*size < NSM_TLV_HEADER_SIZE)
        return NSM_ERR_PKT_TOO_SMALL;

      NSM_DECODE_TLV_HEADER (tlv);

      switch (tlv.type)
        {
        case NSM_QOS_CTYPE_GMPLS_ATTR_SW_CAP:
          /* Decode SW CAP and Encoding Type */
          TLV_DECODE_GETC (msg->sw_cap);
          TLV_DECODE_GETC (msg->encoding);
          NSM_SET_CTYPE (msg->cindex, tlv.type);
          break;
        case NSM_QOS_CTYPE_GMPLS_ATTR_PROTECTION:
          /* Decode Protection */
          TLV_DECODE_GETC (msg->protection);
          NSM_SET_CTYPE (msg->cindex, tlv.type);
          break;
        case NSM_QOS_CTYPE_GMPLS_ATTR_GPID:
          /* Decode Protection */
          TLV_DECODE_GETW (msg->gpid);
          NSM_SET_CTYPE (msg->cindex, tlv.type);
          break;
        default:
          TLV_DECODE_SKIP (tlv.length);
          break;
        }
    }

  return *pnt - sp;
}

s_int32_t
nsm_encode_gmpls_gen_label (u_char **pnt, u_int16_t *size,
                            struct gmpls_gen_label *label)
{
  u_char *sp = *pnt;
#ifdef HAVE_PBB_TE
  int i = 0;
#endif /*HAVE_PBB_TE */
  TLV_ENCODE_PUTL (label->type);
  switch (label->type)
    {
    case gmpls_entry_type_ip:
      TLV_ENCODE_PUTL (label->u.pkt);
      break;
#ifdef HAVE_PBB_TE
    case gmpls_entry_type_pbb_te:
      TLV_ENCODE_PUTW (label->u.pbb.bvid);
      for (i = 0; i < 6; i++)
        TLV_ENCODE_PUTC (label->u.pbb.bmac[i]);
      break;
#endif /*HAVE_PBB_TE */
#ifdef HAVE_TDM
    case gmpls_entry_type_tdm:
      TLV_ENCODE_PUTL (label->u.tdm->gifindex);
      TLV_ENCODE_PUTL (label->u.tdm->tslot);
      break;
#endif /*HAVE_TDM */
    default:
      return -1;
    }
  return *pnt - sp;
}


s_int32_t
nsm_decode_gmpls_gen_label (u_char **pnt, u_int16_t *size,
                            struct gmpls_gen_label *label)
{
  u_char *sp = *pnt;
#ifdef HAVE_PBB_TE
  int i = 0;
#endif /*HAVE_PBB_TE */
  TLV_DECODE_GETL (label->type);
  switch (label->type)
    {
    case gmpls_entry_type_ip:
      TLV_DECODE_GETL (label->u.pkt);
      break;
#ifdef HAVE_PBB_TE
    case gmpls_entry_type_pbb_te:
      TLV_DECODE_GETW (label->u.pbb.bvid);
      for (i = 0; i < 6; i++)
        TLV_DECODE_GETC (label->u.pbb.bmac[i]);
      break;
#endif /*HAVE_PBB_TE */
#ifdef HAVE_TDM
    case gmpls_entry_type_tdm:
      TLV_DECODE_GETL (label->u.tdm->gifindex);
      TLV_DECODE_GETL (label->u.tdm->tslot);
      break;
#endif /*HAVE_TDM */
    default:
      return -1;
    }
  return *pnt - sp;
}

s_int32_t
nsm_encode_gmpls_label_spec (u_char **pnt, u_int16_t *size,
                             struct nsm_msg_gmpls_qos_label_spec *msg)
{
  u_char *sp = *pnt;
  int i;
  int nbytes;
  u_char *spnt;

  for (i = 0; i < NSM_CINDEX_SIZE; i++)
    if (NSM_CHECK_CTYPE (msg->cindex, i))
      {
        if (*size < NSM_TLV_HEADER_SIZE)
          return NSM_ERR_PKT_TOO_SMALL;

        switch (i)
          {
          case NSM_QOS_CTYPE_GMPLS_LABEL_SPEC_LABEL_SET:
            /* Encode Label Set Range*/
            nsm_encode_tlv (pnt, size, i, 8);
            TLV_ENCODE_PUTL (msg->sl_lbl_set_from);
            TLV_ENCODE_PUTL (msg->sl_lbl_set_to);
            break;
          case NSM_QOS_CTYPE_GMPLS_LABEL_SPEC_SUGG_LABEL:
            /* Encode Suggested Label */
            spnt = *pnt;
            *pnt += NSM_TLV_HEADER_SIZE;
            nbytes = nsm_encode_gmpls_gen_label (pnt, size, &msg->sugg_label);
            nsm_encode_tlv (&spnt, size, i, nbytes);
            break;
          default:
            break;
          }
      }

  return *pnt - sp;
}

s_int32_t
nsm_decode_gmpls_label_spec (u_char **pnt, u_int16_t *size,
                             struct nsm_msg_gmpls_qos_label_spec *msg)
{
  u_char *sp = *pnt;
  struct nsm_tlv_header tlv;
  u_int16_t len;
  int nbytes;

  /* TLV parse.  */
  while (*size)
    {
      if (*size < NSM_TLV_HEADER_SIZE)
        return NSM_ERR_PKT_TOO_SMALL;

      NSM_DECODE_TLV_HEADER (tlv);

      switch (tlv.type)
        {
        case NSM_QOS_CTYPE_GMPLS_LABEL_SPEC_LABEL_SET:
          /* Check size.  */
          if (*size < NSM_MSG_GMPLS_QOS_LABEL_SET_SIZE)
            return NSM_ERR_PKT_TOO_SMALL;

          /* Decode Label Set Range*/
          TLV_DECODE_GETL (msg->sl_lbl_set_from);
          TLV_DECODE_GETL (msg->sl_lbl_set_to);
          NSM_SET_CTYPE (msg->cindex, tlv.type);
          break;
        case NSM_QOS_CTYPE_GMPLS_LABEL_SPEC_SUGG_LABEL:
          /* Decode Suggested Label */
          len = tlv.length;
          nbytes = nsm_decode_gmpls_gen_label (pnt, &len, &msg->sugg_label);
          *size -= nbytes;
          NSM_SET_CTYPE (msg->cindex, tlv.type);
          break;
        default:
          break;
        }
    }
  return *pnt - sp;
}

s_int32_t
nsm_encode_gmpls_qos (u_char **pnt, u_int16_t *size,
                      struct nsm_msg_gmpls_qos *msg)
{
  u_char *sp = *pnt;
  u_char *spnt;
  int i, ret, nbytes;

  /* Check size.  */
  if (*size < NSM_MSG_QOS_SIZE)
    return NSM_ERR_PKT_TOO_SMALL;

  /* Resource ID. */
  TLV_ENCODE_PUTL (msg->resource_id);

  /* Protocol ID. */
  TLV_ENCODE_PUTL (msg->protocol_id);

  /* ID. */
  TLV_ENCODE_PUTL (msg->id);

  /* Encode owner the ipv6 routine encodes for ipv4 and ipv6. */
#ifdef HAVE_IPV6
  ret = nsm_encode_mpls_owner_ipv6 (pnt, size, &msg->owner);
#else
  ret = nsm_encode_mpls_owner_ipv4 (pnt, size, &msg->owner);
#endif
  if (ret < 0)
    return ret;

  /* Encode Class Type. */
  TLV_ENCODE_PUTC (msg->ct_num);

  /* Encode LSP Direction */
  TLV_ENCODE_PUTC (msg->direction);

  /* TLV encode.  */
  for (i = 0; i < NSM_CINDEX_SIZE; i++)
    if (NSM_CHECK_CTYPE (msg->cindex, i))
      {
        if (*size < NSM_TLV_HEADER_SIZE)
          return NSM_ERR_PKT_TOO_SMALL;

        switch (i)
          {
          case NSM_QOS_CTYPE_SETUP_PRIORITY:
            nsm_encode_tlv (pnt, size, i, 1);
            TLV_ENCODE_PUTC (msg->setup_priority);
            break;
          case NSM_QOS_CTYPE_HOLD_PRIORITY:
            nsm_encode_tlv (pnt, size, i, 1);
            TLV_ENCODE_PUTC (msg->hold_priority);
            break;
          case NSM_QOS_CTYPE_STATUS:
            nsm_encode_tlv (pnt, size, i, 4);
            TLV_ENCODE_PUTL (msg->status);
            break;
          case NSM_QOS_CTYPE_T_SPEC:
            spnt = *pnt;
            *pnt += NSM_TLV_HEADER_SIZE;
            nbytes = nsm_encode_t_spec (pnt, size, &msg->t_spec);
            nsm_encode_tlv (&spnt, size, i, nbytes);
            break;
          case NSM_QOS_CTYPE_AD_SPEC:
            spnt = *pnt;
            *pnt += NSM_TLV_HEADER_SIZE;
            nbytes = nsm_encode_ad_spec (pnt, size, &msg->ad_spec);
            nsm_encode_tlv (&spnt, size, i, nbytes);
            break;
          case NSM_QOS_CTYPE_GMPLS_IF_SPEC:
            spnt = *pnt;
            *pnt += NSM_TLV_HEADER_SIZE;
            nbytes = nsm_encode_gmpls_if_spec (pnt, size, &msg->gif_spec);
            nsm_encode_tlv (&spnt, size, i, nbytes);
            break;
          case NSM_QOS_CTYPE_GMPLS_ATTR:
            spnt = *pnt;
            *pnt += NSM_TLV_HEADER_SIZE;
            nbytes = nsm_encode_gmpls_attr (pnt, size, &msg->gmpls_attr);
            nsm_encode_tlv (&spnt, size, i, nbytes);
            break;
          case NSM_QOS_CTYPE_GMPLS_LABEL_SPEC:
            spnt = *pnt;
            *pnt += NSM_TLV_HEADER_SIZE;
            nbytes = nsm_encode_gmpls_label_spec (pnt, size, &msg->label_spec);
            nsm_encode_tlv (&spnt, size, i, nbytes);
            break;
          default:
            break;
          }
      }

  return *pnt - sp;
}

s_int32_t
nsm_decode_gmpls_qos (u_char **pnt, u_int16_t *size,
                      struct nsm_msg_gmpls_qos *msg)
{
  u_char *sp = *pnt;
  struct nsm_tlv_header tlv;
  int nbytes;
  u_int16_t len;
  int ret;

  /* Check size.  */
  if (*size < NSM_MSG_QOS_SIZE)
    return NSM_ERR_PKT_TOO_SMALL;

  /* Resource ID. */
  TLV_DECODE_GETL (msg->resource_id);

  /* Protocol ID. */
  TLV_DECODE_GETL (msg->protocol_id);

  /* ID. */
  TLV_DECODE_GETL (msg->id);

  /* Decode owner. */
#ifdef HAVE_IPV6
  if (msg->owner.u.b_key.u.ipv6.p.family == AF_INET6)
    ret = nsm_decode_mpls_owner_ipv6 (pnt, size, &msg->owner);
  else
#endif
    ret = nsm_decode_mpls_owner_ipv4 (pnt, size, &msg->owner);

  if (ret < 0)
    return ret;

  /* Decode Class Type. */
  TLV_DECODE_GETC (msg->ct_num);

  /* Encode LSP Direction */
  TLV_DECODE_GETC (msg->direction);

  /* TLV parse. */
  while (*size)
    {
      if (*size < NSM_TLV_HEADER_SIZE)
        return NSM_ERR_PKT_TOO_SMALL;

      NSM_DECODE_TLV_HEADER (tlv);

      switch (tlv.type)
        {
        case NSM_QOS_CTYPE_SETUP_PRIORITY:
          TLV_DECODE_GETC (msg->setup_priority);
          NSM_SET_CTYPE (msg->cindex, tlv.type);
          break;
        case NSM_QOS_CTYPE_HOLD_PRIORITY:
          TLV_DECODE_GETC (msg->hold_priority);
          NSM_SET_CTYPE (msg->cindex, tlv.type);
          break;
        case NSM_QOS_CTYPE_STATUS:
          TLV_DECODE_GETL (msg->status);
          NSM_SET_CTYPE (msg->cindex, tlv.type);
          break;
        case NSM_QOS_CTYPE_T_SPEC:
          len = tlv.length;
          nbytes = nsm_decode_t_spec (pnt, &len, &msg->t_spec);
          *size -= nbytes;
          NSM_SET_CTYPE (msg->cindex, tlv.type);
          break;
        case NSM_QOS_CTYPE_AD_SPEC:
          len = tlv.length;
          nbytes = nsm_decode_ad_spec (pnt, &len, &msg->ad_spec);
          *size -= nbytes;
          NSM_SET_CTYPE (msg->cindex, tlv.type);
          break;
        case NSM_QOS_CTYPE_GMPLS_IF_SPEC:
          len = tlv.length;
          nbytes = nsm_decode_gmpls_if_spec (pnt, &len, &msg->gif_spec);
          *size -= nbytes;
          NSM_SET_CTYPE (msg->cindex, tlv.type);
          break;
        case NSM_QOS_CTYPE_GMPLS_ATTR:
          len = tlv.length;
          nbytes = nsm_decode_gmpls_attr (pnt, &len, &msg->gmpls_attr);
          *size -= nbytes;
          NSM_SET_CTYPE (msg->cindex, tlv.type);
          break;
        case NSM_QOS_CTYPE_GMPLS_LABEL_SPEC:
          len = tlv.length;
          nbytes = nsm_decode_gmpls_label_spec (pnt, &len, &msg->label_spec);
          *size -= nbytes;
          NSM_SET_CTYPE (msg->cindex, tlv.type);
          break;
        default:
          TLV_DECODE_SKIP (tlv.length);
          break;
        }
    }

  return *pnt - sp;
}

s_int32_t
nsm_encode_gmpls_qos_release (u_char **pnt, u_int16_t *size,
                              struct nsm_msg_gmpls_qos_release *msg)
{
  u_char *sp = *pnt;
  int i;

  /* Check size.  */
  if (*size < NSM_MSG_QOS_RELEASE_SIZE)
    return NSM_ERR_PKT_TOO_SMALL;

  /* Resource ID. */
  TLV_ENCODE_PUTL (msg->resource_id);

  /* Protocol ID. */
  TLV_ENCODE_PUTL (msg->protocol_id);

  /* TLV encode.  */
  for (i = 0; i < NSM_CINDEX_SIZE; i++)
    if (NSM_CHECK_CTYPE (msg->cindex, i))
      {
        if (*size < NSM_TLV_HEADER_SIZE)
          return NSM_ERR_PKT_TOO_SMALL;

        switch (i)
          {
          case NSM_GMPLS_QOS_RELEASE_CTYPE_TL_GIFINDEX:
            nsm_encode_tlv (pnt, size, i, 4);
            TLV_ENCODE_PUTL (msg->tel_gifindex);
            break;
          case NSM_GMPLS_QOS_RELEASE_CTYPE_DL_GIFINDEX:
            nsm_encode_tlv (pnt, size, i, 4);
            TLV_ENCODE_PUTL (msg->dl_gifindex);
            break;
          case NSM_GMPLS_QOS_RELEASE_CTYPE_STATUS:
            nsm_encode_tlv (pnt, size, i, 4);
            TLV_ENCODE_PUTL (msg->status);
            break;
          default:
            break;
          }
      }

  return *pnt -sp;
}

s_int32_t
nsm_decode_gmpls_qos_release (u_char **pnt, u_int16_t *size,
                              struct nsm_msg_gmpls_qos_release *msg)
{
  u_char *sp = *pnt;
  struct nsm_tlv_header tlv;

  /* Check size.  */
  if (*size < NSM_MSG_QOS_RELEASE_SIZE)
    return NSM_ERR_PKT_TOO_SMALL;

  /* Resource ID. */
  TLV_DECODE_GETL (msg->resource_id);

  /* Protocol ID. */
  TLV_DECODE_GETL (msg->protocol_id);

  /* TLV parse.  */
  while (*size)
    {
      if (*size < NSM_TLV_HEADER_SIZE)
        return NSM_ERR_PKT_TOO_SMALL;

      NSM_DECODE_TLV_HEADER (tlv);

      switch (tlv.type)
        {
        case NSM_GMPLS_QOS_RELEASE_CTYPE_TL_GIFINDEX:
          TLV_DECODE_GETL (msg->tel_gifindex);
          NSM_SET_CTYPE (msg->cindex, tlv.type);
          break;
        case NSM_GMPLS_QOS_RELEASE_CTYPE_DL_GIFINDEX:
          TLV_DECODE_GETL (msg->dl_gifindex);
          NSM_SET_CTYPE (msg->cindex, tlv.type);
          break;
        case NSM_GMPLS_QOS_RELEASE_CTYPE_STATUS:
          TLV_DECODE_GETL (msg->status);
          NSM_SET_CTYPE (msg->cindex, tlv.type);
          break;
        default:
          TLV_DECODE_SKIP (tlv.length);
          break;
        }
    }

  return *pnt - sp;
}

s_int32_t
nsm_encode_gmpls_qos_clean (u_char **pnt, u_int16_t *size,
                            struct nsm_msg_gmpls_qos_clean *msg)
{
  u_char *sp = *pnt;
  int i;

  /* Check size. */
  if (*size < NSM_MSG_QOS_CLEAN_SIZE)
    return NSM_ERR_PKT_TOO_SMALL;

  /* Protocol ID. */
  TLV_ENCODE_PUTL (msg->protocol_id);

  /* TLV encode.  */
  for (i = 0; i < NSM_CINDEX_SIZE; i++)
    if (NSM_CHECK_CTYPE (msg->cindex, i))
      {
        if (*size < NSM_TLV_HEADER_SIZE)
          return NSM_ERR_PKT_TOO_SMALL;

        switch (i)
          {
          case NSM_GMPLS_QOS_CLEAN_CTYPE_TL_GIFINDEX:
            nsm_encode_tlv (pnt, size, i, 4);
            TLV_ENCODE_PUTL (msg->tel_gifindex);
            break;
          case NSM_GMPLS_QOS_CLEAN_CTYPE_DL_GIFINDEX:
            nsm_encode_tlv (pnt, size, i, 4);
            TLV_ENCODE_PUTL (msg->dl_gifindex);
            break;
          default:
            break;
          }
      }

  return *pnt - sp;
}

s_int32_t
nsm_decode_gmpls_qos_clean (u_char **pnt, u_int16_t *size,
                            struct nsm_msg_gmpls_qos_clean *msg)
{
  u_char *sp = *pnt;
  struct nsm_tlv_header tlv;

  /* Check size. */
  if (*size < NSM_MSG_QOS_CLEAN_SIZE)
    return NSM_ERR_PKT_TOO_SMALL;

  /* Protocol ID. */
  TLV_DECODE_GETL (msg->protocol_id);

  /* TLV parse.  */
  while (*size)
    {
      if (*size < NSM_TLV_HEADER_SIZE)
        return NSM_ERR_PKT_TOO_SMALL;

      NSM_DECODE_TLV_HEADER (tlv);

      switch (tlv.type)
        {
        case NSM_GMPLS_QOS_CLEAN_CTYPE_TL_GIFINDEX:
          TLV_DECODE_GETL (msg->tel_gifindex);
          NSM_SET_CTYPE (msg->cindex, tlv.type);
          break;
        case NSM_GMPLS_QOS_CLEAN_CTYPE_DL_GIFINDEX:
          TLV_DECODE_GETL (msg->dl_gifindex);
          NSM_SET_CTYPE (msg->cindex, tlv.type);
          break;
        default:
          TLV_DECODE_SKIP (tlv.length);
          break;
        }
    }

  return *pnt - sp;
}

#endif /* HAVE_PACKET */
#endif /* HAVE_GMPLS */

#ifdef HAVE_MPLS_VC
s_int32_t
nsm_encode_mpls_vc (u_char **pnt, u_int16_t *size,
                    struct nsm_msg_mpls_vc *msg)
{
  u_char *sp = *pnt;
  u_char *spnt;
  int i;

  /* Check size.  */
  if (*size < NSM_MSG_MPLS_VC_SIZE)
    return NSM_ERR_PKT_TOO_SMALL;

  /* VC ID.  */
  TLV_ENCODE_PUTL (msg->vc_id);

  /* VC TLV parse. */
  for (i = 0; i < NSM_CINDEX_SIZE; i++)
    {
      if (NSM_CHECK_CTYPE (msg->cindex, i))
        {
          if (*size < NSM_TLV_HEADER_SIZE)
            return NSM_ERR_PKT_TOO_SMALL;

          switch (i)
            {
            case NSM_MPLS_VC_CTYPE_NEIGHBOR:
              /* Neighbor address */
              spnt = *pnt;
              *pnt += NSM_TLV_HEADER_SIZE;
              TLV_ENCODE_PUTW (msg->afi);
              if (msg->afi == AFI_IP)
                {
                  TLV_ENCODE_PUT_IN4_ADDR (&msg->nbr_addr_ipv4);
                  nsm_encode_tlv (&spnt, size, i, 6);
                }
#ifdef HAVE_IPV6
              else if (msg->afi == AFI_IP6)
                {
                  TLV_ENCODE_PUT_IN6_ADDR (&msg->nbr_addr_ipv6);
                  nsm_encode_tlv (&spnt, size, i, 18);
                }
#endif /* HAVE_IPV6 */
              break;
            case NSM_MPLS_VC_CTYPE_CONTROL_WORD:
              nsm_encode_tlv (pnt, size, i, 1);
              TLV_ENCODE_PUTC (msg->c_word);
              break;
            case NSM_MPLS_VC_CTYPE_GROUP_ID:
              nsm_encode_tlv (pnt, size, i, 4);
              TLV_ENCODE_PUTL (msg->grp_id);
              break;
            case NSM_MPLS_VC_CTYPE_IF_INDEX:
              nsm_encode_tlv (pnt, size, i, 4);
              TLV_ENCODE_PUTL (msg->ifindex);
              break;
#ifdef HAVE_VPLS
            case NSM_MPLS_VC_CTYPE_VPLS_ID:
              nsm_encode_tlv (pnt, size, i, 4);
              TLV_ENCODE_PUTL (msg->vpls_id);
              break;
#endif /* HAVE_VPLS */
            case NSM_MPLS_VC_CTYPE_IF_MTU:
              nsm_encode_tlv (pnt, size, i, 2);
              TLV_ENCODE_PUTW (msg->ifmtu);
              break;
            case NSM_MPLS_VC_CTYPE_VC_TYPE:
              nsm_encode_tlv (pnt, size, i, 2);
              TLV_ENCODE_PUTW (msg->vc_type);
              break;
            case NSM_MPLS_VC_CTYPE_VLAN_ID:
              nsm_encode_tlv (pnt, size, i, 2);
              TLV_ENCODE_PUTW (msg->vlan_id);
              break;
            case NSM_MPLS_VC_CTYPE_PW_STATUS:
              nsm_encode_tlv (pnt, size, i ,4);
              TLV_ENCODE_PUTL (msg->pw_status);
              break;
#ifdef HAVE_MS_PW
            case NSM_MPLS_VC_CTYPE_MS_PW_ROLE:
              nsm_encode_tlv (pnt, size, i, 1);
              TLV_ENCODE_PUTC (msg->ms_pw_role);
              break;
#endif /* HAVE_MS_PW */
#ifdef HAVE_VCCV
            case NSM_MPLS_VC_CTYPE_VCCV_CCTYPES:
              nsm_encode_tlv (pnt, size, i ,1);
              TLV_ENCODE_PUTC (msg->cc_types);
              break;
            case NSM_MPLS_VC_CTYPE_VCCV_CVTYPES:
              nsm_encode_tlv (pnt, size, i ,1);
              TLV_ENCODE_PUTC (msg->cv_types);
              break;
#endif /* HAVE_VCCV */
            }
        }
    }

  return *pnt - sp;
}

s_int32_t
nsm_decode_mpls_vc (u_char **pnt, u_int16_t *size,
                    struct nsm_msg_mpls_vc *msg)
{
  u_char *sp = *pnt;
  struct nsm_tlv_header tlv;

  /* Check size.  */
  if (*size < NSM_MSG_MPLS_VC_SIZE)
    return NSM_ERR_PKT_TOO_SMALL;

  /* VC ID.  */
  TLV_DECODE_GETL (msg->vc_id);

  /* TLV parse.  */
  while (*size)
    {
      if (*size < NSM_TLV_HEADER_SIZE)
        return NSM_ERR_PKT_TOO_SMALL;

      NSM_DECODE_TLV_HEADER (tlv);

      switch (tlv.type)
        {
          /* Neighbor address.  */
        case NSM_MPLS_VC_CTYPE_NEIGHBOR:
          TLV_DECODE_GETW (msg->afi);
          if (msg->afi == AFI_IP)
            TLV_DECODE_GET_IN4_ADDR (&msg->nbr_addr_ipv4);
#ifdef HAVE_IPV6
          else if (msg->afi == AFI_IP6)
            TLV_DECODE_GET_IN6_ADDR (&msg->nbr_addr_ipv6);
#endif /* HAVE_IPV6 */
          NSM_SET_CTYPE (msg->cindex, tlv.type);
          break;
        case NSM_MPLS_VC_CTYPE_CONTROL_WORD:
          TLV_DECODE_GETC (msg->c_word);
          NSM_SET_CTYPE (msg->cindex, tlv.type);
          break;
        case NSM_MPLS_VC_CTYPE_GROUP_ID:
          TLV_DECODE_GETL (msg->grp_id);
          NSM_SET_CTYPE (msg->cindex, tlv.type);
          break;
        case NSM_MPLS_VC_CTYPE_IF_INDEX:
          TLV_DECODE_GETL (msg->ifindex);
          NSM_SET_CTYPE (msg->cindex, tlv.type);
          break;
#ifdef HAVE_VPLS
        case NSM_MPLS_VC_CTYPE_VPLS_ID:
          TLV_DECODE_GETL (msg->vpls_id);
          NSM_SET_CTYPE (msg->cindex, tlv.type);
          break;
#endif /* HAVE_VPLS */
        case NSM_MPLS_VC_CTYPE_IF_MTU:
          TLV_DECODE_GETW (msg->ifmtu);
          NSM_SET_CTYPE (msg->cindex, tlv.type);
          break;
        case NSM_MPLS_VC_CTYPE_VC_TYPE:
          TLV_DECODE_GETW (msg->vc_type);
          NSM_SET_CTYPE (msg->cindex, tlv.type);
          break;
        case NSM_MPLS_VC_CTYPE_VLAN_ID:
          TLV_DECODE_GETW (msg->vlan_id);
          NSM_SET_CTYPE (msg->cindex, tlv.type);
          break;
        case NSM_MPLS_VC_CTYPE_PW_STATUS:
          TLV_DECODE_GETL (msg->pw_status);
          NSM_SET_CTYPE (msg->cindex, tlv.type);
          break;
#ifdef HAVE_MS_PW
        case NSM_MPLS_VC_CTYPE_MS_PW_ROLE:
          TLV_DECODE_GETC (msg->ms_pw_role);
          NSM_SET_CTYPE (msg->cindex, tlv.type);
          break;
#endif /* HAVE_MS_PW */
#ifdef HAVE_VCCV
        case NSM_MPLS_VC_CTYPE_VCCV_CCTYPES:
          TLV_DECODE_GETC (msg->cc_types);
          NSM_SET_CTYPE (msg->cindex, tlv.type);
          break;
        case NSM_MPLS_VC_CTYPE_VCCV_CVTYPES:
          TLV_DECODE_GETC (msg->cv_types);
          NSM_SET_CTYPE (msg->cindex, tlv.type);
          break;
#endif /* HAVE_VCCV */
        default:
          TLV_DECODE_SKIP (tlv.length);
          break;
        }
    }
  return *pnt - sp;
}
#ifdef HAVE_MS_PW
/**@brief Function to encode the mspw message.
    @param **pnt - pointer to the message buffer.
    @param *size - pointer to the size of message buffer.
    @param *msg  - pointer to the mspw message.
    @return      - length of the encoded message.
*/
int
nsm_encode_mpls_ms_pw_msg (u_char **pnt, u_int16_t *size,
                           struct nsm_msg_mpls_ms_pw_msg *msg)
{
  u_char *sp = *pnt;
  u_char *temp_pnt = NULL;
  int i;
  int len = 0;

  /* Check size */
  if (*size < NSM_MSG_MPLS_MS_PW_MSG_SIZE)
    return NSM_ERR_PKT_TOO_SMALL;

  /* Add/Delete */
  TLV_ENCODE_PUTL (msg->ms_pw_action);

  /* MS PW name */
  TLV_ENCODE_PUT (msg->ms_pw_name, NSM_MS_PW_NAME_LEN);

  for (i = 0; i < NSM_MSG_MPLS_MS_PW_CINDEX_MAX; i++)
    {
      if (NSM_CHECK_CTYPE (msg->cindex, i))
        {
          if (*size < NSM_TLV_HEADER_SIZE)
            return NSM_ERR_PKT_TOO_SMALL;

          switch (i)
            {
            case NSM_MPLS_MS_PW_VC1_INFO:
              temp_pnt = *pnt;
              *pnt += NSM_TLV_HEADER_SIZE;
              len = nsm_encode_mpls_vc (pnt, size, &(msg->vc1_info));
              if (len < 0)
                return len;
              nsm_encode_tlv (&temp_pnt, size, i, len);
              break;
            case NSM_MPLS_MS_PW_VC2_INFO:
              temp_pnt = *pnt;
              *pnt += NSM_TLV_HEADER_SIZE;
              len = nsm_encode_mpls_vc (pnt, size, &(msg->vc2_info));
              if (len < 0)
                return len;
              nsm_encode_tlv (&temp_pnt, size, i, len);
              break;
            case NSM_MPLS_MS_PW_VC1_ID:
              nsm_encode_tlv (pnt, size, i, 4);
              TLV_ENCODE_PUTL (msg->vcid1);
              break;
            case NSM_MPLS_MS_PW_VC2_ID:
              nsm_encode_tlv (pnt, size, i, 4);
              TLV_ENCODE_PUTL (msg->vcid2);
              break;
            case NSM_MPLS_MS_PW_SPE_DESCR:
              nsm_encode_tlv (pnt, size, i, NSM_MS_PW_DESCR_LEN);
              TLV_ENCODE_PUT (msg->ms_pw_spe_descr, NSM_MS_PW_DESCR_LEN);
              break;
            }
        }
    }
  return *pnt - sp;
}

/**@brief Function to decode the mspw message.
    @param **pnt - pointer to the message buffer.
    @param *size - pointer to the size of message buffer.
    @param *msg  - pointer to the mspw message.
    @return      - length of the decoded message.
*/
int
nsm_decode_mpls_ms_pw_msg (u_char **pnt, u_int16_t *size,
                           struct nsm_msg_mpls_ms_pw_msg *msg)
{
  u_char *sp = *pnt;
  struct nsm_tlv_header tlv;
  u_int16_t temp_size = 0;

  /* Check size.  */
  if (*size < NSM_MSG_MPLS_MS_PW_MSG_SIZE)
    return NSM_ERR_PKT_TOO_SMALL;

  /* Add/Delete */
  TLV_DECODE_GETL(msg->ms_pw_action);

  TLV_DECODE_GET(msg->ms_pw_name, NSM_MS_PW_NAME_LEN);

  /* TLV parse.  */
  while (*size)
    {
      if (*size < NSM_TLV_HEADER_SIZE)
        return NSM_ERR_PKT_TOO_SMALL;

      NSM_DECODE_TLV_HEADER (tlv);

      switch (tlv.type)
        {
        case NSM_MPLS_MS_PW_VC1_INFO:
          temp_size = tlv.length;
          nsm_decode_mpls_vc(pnt, &temp_size, &(msg->vc1_info));
          NSM_SET_CTYPE (msg->cindex, tlv.type);
          *size -= tlv.length;
          break;
        case NSM_MPLS_MS_PW_VC2_INFO:
          temp_size = tlv.length;
          nsm_decode_mpls_vc(pnt, &temp_size, &(msg->vc2_info));
          NSM_SET_CTYPE (msg->cindex, tlv.type);
          *size -= tlv.length;
          break;
        case NSM_MPLS_MS_PW_VC1_ID:
          TLV_DECODE_GETL (msg->vcid1);
          NSM_SET_CTYPE (msg->cindex, tlv.type);
          break;
        case NSM_MPLS_MS_PW_VC2_ID:
          TLV_DECODE_GETL (msg->vcid2);
          NSM_SET_CTYPE (msg->cindex, tlv.type);
          break;
        case NSM_MPLS_MS_PW_SPE_DESCR:
          TLV_DECODE_GET (msg->ms_pw_spe_descr, NSM_MS_PW_DESCR_LEN);
          NSM_SET_CTYPE (msg->cindex, tlv.type);
          break;
        }
    }
  return *pnt - sp;
}
#endif /* HAVE_MS_PW */
#endif /* HAVE_MPLS_VC */



#ifdef HAVE_VPLS
s_int32_t
nsm_encode_vpls_mac_withdraw (u_char **pnt, u_int16_t *size,
                              struct nsm_msg_vpls_mac_withdraw *msg)
{
  u_char *sp = *pnt;
  u_char *mac_addr;

  /* Sanity check. */
  if ((msg->num != 0 && msg->mac_addrs == NULL)
      || (msg->num == 0 && msg->mac_addrs != NULL))
    return NSM_ERR_INVALID_PKT;

  /* Size check. */
  if (*size < NSM_MSG_MIN_VPLS_MAC_WITHDRAW_SIZE)
    return NSM_ERR_PKT_TOO_SMALL;

  /* VPLS Id. */
  TLV_ENCODE_PUTL (msg->vpls_id);

  /* MAC address count. */
  TLV_ENCODE_PUTW (msg->num);

  /* Recheck size. */
  if (*size < msg->num * VPLS_MAC_ADDR_LEN)
    return NSM_ERR_PKT_TOO_SMALL;

  if (msg->num > 0)
    {
      mac_addr = msg->mac_addrs;
      TLV_ENCODE_PUT (mac_addr, VPLS_MAC_ADDR_LEN * msg->num);
    }

  return *pnt - sp;
}


s_int32_t
nsm_encode_vpls_peers_tlv (u_char **pnt, u_int16_t *size, struct addr_in *addr,
                           u_int16_t addr_count, u_int16_t type)
{
  u_char *sp = *pnt;
  u_char *s_tlv;
  int ctr;
  u_int16_t tmp16;
  u_int16_t len = 0;

  /* encode blank tlv header */
  s_tlv = *pnt;
  TLV_ENCODE_PUT_EMPTY (NSM_TLV_HEADER_SIZE);

  for (ctr = 0; ctr < addr_count; ctr++)
    {
      if (addr[ctr].afi == AFI_IP)
        {
          if (*size < IPV4_MAX_BYTELEN + sizeof (u_int16_t))
            return NSM_ERR_PKT_TOO_SMALL;

          TLV_ENCODE_PUTW (addr[ctr].afi);
          TLV_ENCODE_PUT_IN4_ADDR (&addr[ctr].u.ipv4);
          len += (IPV4_MAX_BYTELEN + 2);
        }
#ifdef HAVE_IPv6
      else if (addr[i].afi == AFI_IP6)
        {
          if (*size < IPV6_MAX_BYTELEN + sizeof (u_int16_t))
            return NSM_ERR_PKT_TOO_SMALL;

          TLV_ENCODE_PUTW (addr[ctr].afi);
          TLV_ENCODE_PUT_IN6_ADDR (&addr[ctr].u.ipv6);
          len += (IPV6_MAX_BYTELEN + 2);
        }
#endif /* HAVE_IPv6 */
      else
        return NSM_ERR_INVALID_PKT;
    }

  /* encode tlv header before peer address data */
  tmp16 = NSM_TLV_HEADER_SIZE;
  nsm_encode_tlv (&s_tlv, &tmp16, type, len);

  return *pnt - sp;
}


s_int32_t
nsm_encode_vpls_add (u_char **pnt, u_int16_t *size,
                     struct nsm_msg_vpls_add *msg)
{
  u_char *sp = *pnt;
  int i;
  int ret;

  /* Check size.  */
  if (*size < NSM_MSG_VPLS_SIZE)
    return NSM_ERR_PKT_TOO_SMALL;

  /* VPLS ID.  */
  TLV_ENCODE_PUTL (msg->vpls_id);

  /* VPLS TLV parse. */
  for (i = 0; i < NSM_CINDEX_SIZE; i++)
    {
      if (NSM_CHECK_CTYPE (msg->cindex, i))
        {
          if (*size < NSM_TLV_HEADER_SIZE)
            return NSM_ERR_PKT_TOO_SMALL;

          switch (i)
            {
            case NSM_VPLS_CTYPE_GROUP_ID:
              nsm_encode_tlv (pnt, size, i, 4);
              TLV_ENCODE_PUTL (msg->grp_id);
              break;
            case NSM_VPLS_CTYPE_PEERS_ADDED:
              ret = nsm_encode_vpls_peers_tlv (pnt, size, msg->peers_added,
                                               msg->add_count, i);
              if (ret < 0)
                return ret;
              break;
            case NSM_VPLS_CTYPE_PEERS_DELETED:
              ret = nsm_encode_vpls_peers_tlv (pnt, size, msg->peers_deleted,
                                               msg->del_count, i);
              if (ret < 0)
                return ret;
              break;
            case NSM_VPLS_CTYPE_IF_MTU:
              nsm_encode_tlv (pnt, size, i, 2);
              TLV_ENCODE_PUTW (msg->ifmtu);
              break;
            case NSM_VPLS_CTYPE_VPLS_TYPE:
              nsm_encode_tlv (pnt, size, i, 2);
              TLV_ENCODE_PUTW (msg->vpls_type);
              break;
            case NSM_VPLS_CTYPE_CONTROL_WORD:
              nsm_encode_tlv (pnt, size, i, 1);
              TLV_ENCODE_PUTC (msg->c_word);
              break;
            default:
              break;
            }
        }
    }

  return *pnt - sp;
}

s_int32_t
nsm_encode_vpls_delete (u_char **pnt, u_int16_t *size,
                        u_int32_t vpls_id)
{
  /* Check size.  */
  if (*size < NSM_MSG_VPLS_SIZE)
    return NSM_ERR_PKT_TOO_SMALL;

  /* VPLS ID.  */
  TLV_ENCODE_PUTL (vpls_id);

  return NSM_MSG_VPLS_SIZE;
}
#endif /* HAVE_VPLS */


#ifdef HAVE_MPLS_VC
s_int32_t
nsm_encode_vc_fib_add (u_char **pnt, u_int16_t *size,
                       struct nsm_msg_vc_fib_add *msg)
{
  /* Check size.  */
  if (*size < NSM_MSG_VC_FIB_ADD_SIZE)
    return NSM_ERR_PKT_TOO_SMALL;

  /* VPN ID.  */
  TLV_ENCODE_PUTL (msg->vpn_id);

  /* VC ID */
  TLV_ENCODE_PUTL (msg->vc_id);

  TLV_ENCODE_PUTL (msg->vc_style);

  /* Incoming Label.  */
  TLV_ENCODE_PUTL (msg->in_label);

  /* Outgoing Label.  */
  TLV_ENCODE_PUTL (msg->out_label);

  /* Control Word */
  TLV_ENCODE_PUTC (msg->c_word);

  /* Access Interface Index.  */
  TLV_ENCODE_PUTL (msg->ac_if_ix);

  /* Network Interface Index.  */
  TLV_ENCODE_PUTL (msg->nw_if_ix);

  /* LSR ID.  */
  TLV_ENCODE_PUTL (msg->lsr_id);

  /* ENTITY INDEX */
  TLV_ENCODE_PUTL (msg->index);

  /* Remote Group Id */
  TLV_ENCODE_PUTL (msg->remote_grp_id);

  /* Result of local pw_status & remote pw_status */
  TLV_ENCODE_PUTL (msg->remote_pw_status);

#ifdef HAVE_VCCV
  /* remote cc types */
  TLV_ENCODE_PUTC (msg->remote_cc_types);
  /* remote cv types */
  TLV_ENCODE_PUTC (msg->remote_cv_types);
#endif /* HAVE_VCCV */

  /* Remote Node PW Status Cap */
  TLV_ENCODE_PUTC (msg->rem_pw_status_cap);

  /* VC Peer address */
  TLV_ENCODE_PUTW (msg->addr.afi);
  if (msg->addr.afi == AFI_IP)
    {
      TLV_ENCODE_PUT_IN4_ADDR (&msg->addr.u.ipv4);
    }
#ifdef HAVE_IPV6
  else if (msg->addr.afi == AFI_IP6)
    {
      TLV_ENCODE_PUT_IN6_ADDR (&msg->addr.u.ipv6);
    }
#endif /* HAVE_IPV6 */
  else
    return NSM_ERR_INVALID_PKT;

  return NSM_MSG_VC_FIB_ADD_SIZE;
}

s_int32_t
nsm_encode_pw_status (u_char **pnt, u_int16_t *size,
                      struct nsm_msg_pw_status *msg)
{
  /* Check size.  */
  if (*size < NSM_MSG_VC_STATUS_SIZE)
    return NSM_ERR_PKT_TOO_SMALL;

  /* VPN ID.  */
  TLV_ENCODE_PUTL (msg->vpn_id);

  /* VC ID */
  TLV_ENCODE_PUTL (msg->vc_id);

  /* Result of local pw_status & remote pw_status */
  TLV_ENCODE_PUTL (msg->pw_status);

#ifdef HAVE_VCCV
  /* VC Incoming Label */
  TLV_ENCODE_PUTL (msg->in_vc_label);
#endif /* HAVE_VCCV */

  return NSM_MSG_VC_STATUS_SIZE;
}

s_int32_t
nsm_encode_vc_tunnel_info (u_char **pnt, u_int16_t *size,
                           struct nsm_msg_vc_tunnel_info *msg)
{
  /* Check size.  */
  if (*size < NSM_MSG_VC_TUNNEL_INFO_SIZE)
    return NSM_ERR_PKT_TOO_SMALL;

 /* LSR ID.  */
  TLV_ENCODE_PUTL (msg->lsr_id);

  /* ENTITY INDEX */
  TLV_ENCODE_PUTL (msg->index);

  /* MSG TYPE */
  TLV_ENCODE_PUTC (msg->type);

  return NSM_MSG_VC_TUNNEL_INFO_SIZE;
}

s_int32_t
nsm_encode_vc_fib_delete (u_char **pnt, u_int16_t *size,
                            struct nsm_msg_vc_fib_delete *msg)
{
  /* Check size.  */
  if (*size < NSM_MSG_VC_FIB_DELETE_SIZE)
    return NSM_ERR_PKT_TOO_SMALL;

  /* VPN ID.  */
  TLV_ENCODE_PUTL (msg->vpn_id);

  /* VC ID */
  TLV_ENCODE_PUTL (msg->vc_id);

  TLV_ENCODE_PUTL (msg->vc_style);

  /* VC Peer address */
  TLV_ENCODE_PUTW (msg->addr.afi);
  if (msg->addr.afi == AFI_IP)
    {
            TLV_ENCODE_PUT_IN4_ADDR (&msg->addr.u.ipv4);
    }
#ifdef HAVE_IPV6
  else if (msg->addr.afi == AFI_IP6)
    {
            TLV_ENCODE_PUT_IN6_ADDR (&msg->addr.u.ipv6);
    }
#endif /* HAVE_MPLS_VC */
  else
          return NSM_ERR_INVALID_PKT;

  return NSM_MSG_VC_FIB_DELETE_SIZE;
}
#endif /* HAVE_MPLS_VC */


#ifdef HAVE_VPLS
s_int32_t
nsm_decode_vpls_mac_withdraw (u_char **pnt, u_int16_t *size,
                              struct nsm_msg_vpls_mac_withdraw *msg)
{
  u_char *sp = *pnt;
  u_char *mac_addr;

  /* Size check. */
  if (*size < NSM_MSG_MIN_VPLS_MAC_WITHDRAW_SIZE)
    return NSM_ERR_PKT_TOO_SMALL;

  /* VPLS Id. */
  TLV_DECODE_GETL (msg->vpls_id);

  /* MAC address count. */
  TLV_DECODE_GETW (msg->num);

  /* Recheck size. */
  if (*size < msg->num * VPLS_MAC_ADDR_LEN)
    return NSM_ERR_PKT_TOO_SMALL;

  if (msg->num > 0)
    {
      msg->mac_addrs = XCALLOC (MTYPE_TMP, msg->num * VPLS_MAC_ADDR_LEN);
      if (! msg->mac_addrs)
        return NSM_ERR_SYSTEM_FAILURE;

      mac_addr = msg->mac_addrs;
      TLV_DECODE_GET (mac_addr, VPLS_MAC_ADDR_LEN * msg->num);
    }
  else
    msg->mac_addrs = NULL;

  /* Sanity check. */
  if ((msg->num != 0 && msg->mac_addrs == NULL)
      || (msg->num == 0 && msg->mac_addrs != NULL))
    return NSM_ERR_INVALID_PKT;

  return *pnt - sp;
}

s_int32_t
nsm_decode_vpls_peers_tlv (u_char **pnt, u_int16_t *size,
                           struct addr_in *addr,
                           u_int16_t *addr_count,
                           struct nsm_tlv_header *tlv_h)
{
  u_char *sp = *pnt;
  int len, ctr;

  len = tlv_h->length;
  ctr = 0;

  while (len > 0)
    {
      if (*size < 2)
        return NSM_ERR_PKT_TOO_SMALL;

      TLV_DECODE_GETW (addr[ctr].afi);
      len -= 2;

      if (addr[ctr].afi == AFI_IP)
        {
          if (*size < IPV4_MAX_BYTELEN)
            return NSM_ERR_PKT_TOO_SMALL;

          TLV_DECODE_GET_IN4_ADDR (&addr[ctr].u.ipv4);
          len -= IPV4_MAX_BYTELEN;
        }
#ifdef HAVE_IPV6
      else if (addr[ctr].afi == AFI_IP6)
        {
          if (*size < IPV6_MAX_BYTELEN)
            return NSM_ERR_PKT_TOO_SMALL;

          TLV_DECODE_GET_IN6_ADDR (&addr[ctr].u.ipv6);
          len -= IPV6_MAX_BYTELEN;
        }
#endif /* HAVE_IPV6 */
      else
        return NSM_ERR_INVALID_PKT;

      ctr++;
    }

  *addr_count = ctr;

  return *pnt - sp;
}

s_int32_t
nsm_decode_vpls_add (u_char **pnt, u_int16_t *size,
                     struct nsm_msg_vpls_add *msg)
{
  u_char *sp = *pnt;
  struct nsm_tlv_header tlv;
  int ret;

  /* Check size.  */
  if (*size < NSM_MSG_VPLS_SIZE)
    return NSM_ERR_PKT_TOO_SMALL;

  pal_mem_set (msg, 0, sizeof (struct nsm_msg_vpls_add));

  /* VPLS ID.  */
  TLV_DECODE_GETL (msg->vpls_id);

  /* TLV parse.  */
  while (*size)
    {
      if (*size < NSM_TLV_HEADER_SIZE)
        return NSM_ERR_PKT_TOO_SMALL;

      NSM_DECODE_TLV_HEADER (tlv);

      switch (tlv.type)
        {
        case NSM_VPLS_CTYPE_GROUP_ID:
          TLV_DECODE_GETL (msg->grp_id);
          NSM_SET_CTYPE (msg->cindex, tlv.type);
          break;
        case NSM_VPLS_CTYPE_PEERS_ADDED:
          {
            int count;

            count = tlv.length/(IPV4_MAX_BYTELEN + 2) +
              ((tlv.length % (IPV4_MAX_BYTELEN + 2)) > 0 ? 1 : 0);

            msg->peers_added = XCALLOC (MTYPE_TMP,
                                        sizeof (struct addr_in) * count);
            if (! msg->peers_added)
              return NSM_ERR_SYSTEM_FAILURE;

            ret = nsm_decode_vpls_peers_tlv (pnt, size, msg->peers_added,
                                             &msg->add_count, &tlv);
            if (ret < 0)
              {
                XFREE (MTYPE_TMP, msg->peers_added);
                msg->peers_added = NULL;
                msg->add_count = 0;
                return ret;
              }

            NSM_SET_CTYPE (msg->cindex, tlv.type);
          }
          break;
        case NSM_VPLS_CTYPE_PEERS_DELETED:
          {
            int count;

            count = tlv.length/(IPV4_MAX_BYTELEN + 2) +
              ((tlv.length % (IPV4_MAX_BYTELEN + 2)) > 0 ? 1 : 0);

            msg->peers_deleted = XCALLOC (MTYPE_TMP,
                                          sizeof (struct addr_in) * count);
            if (! msg->peers_deleted)
              return NSM_ERR_SYSTEM_FAILURE;

            ret = nsm_decode_vpls_peers_tlv (pnt, size, msg->peers_deleted,
                                             &msg->del_count, &tlv);
            if (ret < 0)
              {
                XFREE (MTYPE_TMP, msg->peers_deleted);
                msg->peers_deleted = NULL;
                msg->del_count = 0;
                return ret;
              }

            NSM_SET_CTYPE (msg->cindex, tlv.type);
          }
          break;
        case NSM_VPLS_CTYPE_IF_MTU:
          TLV_DECODE_GETW (msg->ifmtu);
          NSM_SET_CTYPE (msg->cindex, tlv.type);
          break;
        case NSM_VPLS_CTYPE_VPLS_TYPE:
          TLV_DECODE_GETW (msg->vpls_type);
          NSM_SET_CTYPE (msg->cindex, tlv.type);
          break;
        case NSM_VPLS_CTYPE_CONTROL_WORD:
          TLV_DECODE_GETC (msg->c_word);
          NSM_SET_CTYPE (msg->cindex, tlv.type);
          break;
        default:
          TLV_DECODE_SKIP (tlv.length);
          break;
        }
    }
  return *pnt - sp;
}

s_int32_t
nsm_decode_vpls_delete (u_char **pnt, u_int16_t *size,
                        u_int32_t *vpls_id)
{
  /* Check size.  */
  if (*size < NSM_MSG_VPLS_SIZE)
    return NSM_ERR_PKT_TOO_SMALL;

  /* VPLS ID.  */
  TLV_DECODE_GETL (*vpls_id);

  return NSM_MSG_VPLS_SIZE;
}
#endif /* HAVE_VPLS */

#ifdef HAVE_MPLS_VC
s_int32_t
nsm_decode_vc_fib_add (u_char **pnt, u_int16_t *size,
                       struct nsm_msg_vc_fib_add *msg)
{
  /* Check size.  */
  if (*size < NSM_MSG_VC_FIB_ADD_SIZE)
    return NSM_ERR_PKT_TOO_SMALL;

  pal_mem_set (msg, 0, sizeof (struct nsm_msg_vc_fib_add));

  /* VPN ID.  */
  TLV_DECODE_GETL (msg->vpn_id);

  TLV_DECODE_GETL (msg->vc_id);

  TLV_DECODE_GETL (msg->vc_style);

  /* Incoming Label.  */
  TLV_DECODE_GETL (msg->in_label);

  /* Outgoing Label.  */
  TLV_DECODE_GETL (msg->out_label);

  /* Control Word */
  TLV_DECODE_GETC (msg->c_word);

  /* Access Interface Index.  */
  TLV_DECODE_GETL (msg->ac_if_ix);

  /* Network Interface Index.  */
  TLV_DECODE_GETL (msg->nw_if_ix);

  /* LSR ID.  */
  TLV_DECODE_GETL (msg->lsr_id);

  /* ENTITY INDEX */
  TLV_DECODE_GETL (msg->index);

  /* Remote Group Id */
  TLV_DECODE_GETL (msg->remote_grp_id);

  /* Result of local pw_status & remote pw_status */
  TLV_DECODE_GETL (msg->remote_pw_status);

#ifdef HAVE_VCCV
  /* remote cc types */
  TLV_DECODE_GETC (msg->remote_cc_types);

  /* remote cv types */
  TLV_DECODE_GETC (msg->remote_cv_types);
#endif /* HAVE_VCCV */

  /* Remote Node PW Status cap */
  TLV_DECODE_GETC (msg->rem_pw_status_cap);

  TLV_DECODE_GETW (msg->addr.afi);
  if (msg->addr.afi == AFI_IP)
    TLV_DECODE_GET_IN4_ADDR (&msg->addr.u.ipv4);
#ifdef HAVE_IPV6
  else if (msg->addr.afi == AFI_IP6)
    TLV_DECODE_GET_IN6_ADDR (&msg->addr.u.ipv6);
#endif /* HAVE_IPV6 */
  else
    return NSM_ERR_INVALID_PKT;


  return NSM_MSG_VC_FIB_ADD_SIZE;
}

s_int32_t
nsm_decode_pw_status (u_char **pnt, u_int16_t *size,
                      struct nsm_msg_pw_status *msg)
{
  /* Check size.  */
  if (*size < NSM_MSG_VC_STATUS_SIZE)
    return NSM_ERR_PKT_TOO_SMALL;

  /* VPN ID.  */
  TLV_DECODE_GETL (msg->vpn_id);

  /* VC ID */
  TLV_DECODE_GETL (msg->vc_id);

  /* Result of local pw_status & remote pw_status */
  TLV_DECODE_GETL (msg->pw_status);

#ifdef HAVE_VCCV
  /* VC Incoming Label */
  TLV_DECODE_GETL (msg->in_vc_label);
#endif /* HAVE_VCCV */

  return NSM_MSG_VC_STATUS_SIZE;
}

s_int32_t
nsm_decode_vc_tunnel_info (u_char **pnt, u_int16_t *size,
                           struct nsm_msg_vc_tunnel_info *msg)
{
  /* Check size.  */
  if (*size < NSM_MSG_VC_TUNNEL_INFO_SIZE)
    return NSM_ERR_PKT_TOO_SMALL;

  pal_mem_set (msg, 0, sizeof (struct nsm_msg_vc_tunnel_info));

  /* LSR ID.  */
  TLV_DECODE_GETL (msg->lsr_id);

  /* ENTITY INDEX */
  TLV_DECODE_GETL (msg->index);

  /* MSG TYPE */
  TLV_DECODE_GETC (msg->type);

  return NSM_MSG_VC_TUNNEL_INFO_SIZE;
}

s_int32_t
nsm_decode_vc_fib_delete (u_char **pnt, u_int16_t *size,
                          struct nsm_msg_vc_fib_delete *msg)
{
  /* Check size.  */
  if (*size < NSM_MSG_VC_FIB_DELETE_SIZE)
    return NSM_ERR_PKT_TOO_SMALL;

  pal_mem_set (msg, 0, sizeof (struct nsm_msg_vc_fib_delete));

  /* VPN ID.  */
  TLV_DECODE_GETL (msg->vpn_id);

  TLV_DECODE_GETL (msg->vc_id);

  TLV_DECODE_GETL (msg->vc_style);

        TLV_DECODE_GETW (msg->addr.afi);
        if (msg->addr.afi == AFI_IP)
          TLV_DECODE_GET_IN4_ADDR (&msg->addr.u.ipv4);
#ifdef HAVE_IPV6
  else if (msg->addr.afi == AFI_IP6)
          TLV_DECODE_GET_IN6_ADDR (&msg->addr.u.ipv6);
#endif /* HAVE_IPV6 */
        else
          return NSM_ERR_INVALID_PKT;


  return NSM_MSG_VC_FIB_DELETE_SIZE;
}
#endif /* HAVE_MPLS_VC */
#endif /* HAVE_MPLS || HAVE_GMPLS */


s_int32_t
nsm_encode_router_id (u_char **pnt, u_int16_t *size,
                      struct nsm_msg_router_id *msg)
{
  u_char *sp = *pnt;

  /* Check size.  */
  if (*size < NSM_MSG_ROUTER_ID_SIZE)
    return NSM_ERR_PKT_TOO_SMALL;

  /* Router ID.  */
  TLV_ENCODE_PUTL (msg->router_id);
  TLV_ENCODE_PUTC (msg->config);

  return *pnt - sp;
}

s_int32_t
nsm_decode_router_id (u_char **pnt, u_int16_t *size,
                      struct nsm_msg_router_id *msg)
{
  u_char *sp = *pnt;

  /* Check size.  */
  if (*size < NSM_MSG_ROUTER_ID_SIZE)
    return NSM_ERR_PKT_TOO_SMALL;

  /* Router ID.  */
  TLV_DECODE_GETL (msg->router_id);
  TLV_DECODE_GETC (msg->config);

  return *pnt - sp;
}

#ifdef HAVE_MPLS
s_int32_t
nsm_encode_gmpls_fec (u_char **pnt, u_int16_t *size,
                      struct fec_gen_entry *fec)
{
  u_char *sp = *pnt;

  TLV_ENCODE_PUTC (fec->type);

  switch (fec->type)
    {
#ifdef HAVE_PACKET
       case gmpls_entry_type_ip:
         TLV_ENCODE_PUTC (fec->u.prefix.family);
         TLV_ENCODE_PUTC (fec->u.prefix.prefixlen);
         TLV_ENCODE_PUT_IN4_ADDR (&fec->u.prefix.u.prefix4);
         break;
#endif /* HAVE_PACKET */

#ifdef HAVE_PBB_TE
       case gmpls_entry_type_pbb_te:
         TLV_ENCODE_PUT (fec->u.pbb.isid, 3);
         break;
#endif /* HAVE_PBB_TE */

#ifdef HAVE_TDM
       case gmpls_entry_type_tdm:
         TLV_ENCODE_PUTL (fec->u.tdm.gifindex);
         TLV_ENCODE_PUTL (fec->u.tdm.tslot);
         break;
#endif /* HAVE_TDM */

       default:
         break;
    }
  return *pnt - sp;
}

s_int32_t
nsm_decode_gmpls_fec (u_char **pnt, u_int16_t *size,
                      struct fec_gen_entry *fec)
{
  u_char *sp = *pnt;

  TLV_DECODE_GETC (fec->type);

  switch (fec->type)
    {
#ifdef HAVE_PACKET
       case gmpls_entry_type_ip:
         TLV_DECODE_GETC (fec->u.prefix.family);
         TLV_DECODE_GETC (fec->u.prefix.prefixlen);
         TLV_DECODE_GET_IN4_ADDR (&fec->u.prefix.u.prefix4);
         break;
#endif /* HAVE_PACKET */

#ifdef HAVE_PBB_TE
       case gmpls_entry_type_pbb_te:
         TLV_DECODE_GET (fec->u.pbb.isid, 3);
         break;
#endif /* HAVE_PBB_TE */

#ifdef HAVE_TDM
       case gmpls_entry_type_tdm:
         TLV_DECODE_GETL (fec->u.tdm.gifindex);
         TLV_DECODE_GETL (fec->u.tdm.tslot);
         break;
#endif /* HAVE_TDM */

       default:
         break;
    }

  return *pnt - sp;
}

s_int32_t
nsm_encode_gmpls_ftn_data (u_char **pnt, u_int16_t *size,
                           struct ftn_add_gen_data *fad)
{
  u_char *sp = *pnt;
  u_char *lp;
  u_int16_t length;
  int ret;

  /* Encode flags.  */
  TLV_ENCODE_PUTW (fad->flags);

  /* Total length of this message.  First we put place holder then
     fill in later on.  */
  lp = *pnt;
  TLV_ENCODE_PUT_EMPTY (2);

  /* ID of this message.  */
  TLV_ENCODE_PUTL (fad->id);

  if ((CHECK_FLAG (fad->flags, NSM_MSG_GEN_FTN_FAST_DEL) ||
      (CHECK_FLAG (fad->flags, NSM_MSG_GEN_FTN_FAST_ADD))))
    {
      /* Encode fast delete data.  */
      TLV_ENCODE_PUTL (fad->vrf_id);
      nsm_encode_gmpls_fec (pnt, size, &fad->ftn);
      TLV_ENCODE_PUTL (fad->ftn_ix);
#ifdef HAVE_GMPLS
      TLV_ENCODE_PUTL (fad->rev_xc_ix);
      TLV_ENCODE_PUTL (fad->rev_ilm_ix);
#endif /*HAVE_GMPLS */
    }
  else
    {
      /* Encode owner.  */
      ret = nsm_encode_mpls_owner_ipv4 (pnt, size, &fad->owner);
      if (ret < 0)
        return ret;

      /* Encode common part.  */
      TLV_ENCODE_PUTL (fad->vrf_id);
      nsm_encode_gmpls_fec (pnt, size, &fad->ftn);
      TLV_ENCODE_PUTL (fad->ftn_ix);
#ifdef HAVE_GMPLS
      TLV_ENCODE_PUTL (fad->rev_xc_ix);
      TLV_ENCODE_PUTL (fad->rev_ilm_ix);
#endif /*HAVE_GMPLS */

#ifdef HAVE_PACKET
#ifdef HAVE_DIFFSERV
      TLV_ENCODE_PUTC (fad->ds_info.lsp_type);
      TLV_ENCODE_PUT (fad->ds_info.dscp_exp_map, 8);
      TLV_ENCODE_PUTC (fad->ds_info.dscp);
      TLV_ENCODE_PUTC (fad->ds_info.af_set);
#endif /* HAVE_DIFFSERV */
#endif /* HAVE_PACKET */

      TLV_ENCODE_PUT_IN4_ADDR (&fad->nh_addr.u.ipv4);
      nsm_encode_gmpls_label (pnt, size, &fad->out_label);
      TLV_ENCODE_PUTL (fad->oif_ix);
      TLV_ENCODE_PUT (&fad->lspid, 6);
      TLV_ENCODE_PUTL (fad->exp_bits);
#ifdef HAVE_PACKET
      if (CHECK_FLAG (fad->flags, NSM_MSG_FTN_DSCP_IN))
        TLV_ENCODE_PUTC (fad->dscp_in);
#endif /* HAVE_PACKET */

      TLV_ENCODE_PUTL (fad->lsp_bits.data);
      TLV_ENCODE_PUTC (fad->opcode);
      TLV_ENCODE_PUTL (fad->tunnel_id);
      TLV_ENCODE_PUTL (fad->protected_lsp_id);
      TLV_ENCODE_PUTL (fad->qos_resrc_id);
      TLV_ENCODE_PUTL (fad->bypass_ftn_ix);

      /* String is encoded as optional TLV.  */
      if (fad->sz_desc)
        {
          nsm_encode_tlv (pnt, size, NSM_FTN_CTYPE_DESCRIPTION,
                          pal_strlen (fad->sz_desc));
          TLV_ENCODE_PUT (fad->sz_desc, pal_strlen (fad->sz_desc));
        }

      /* GELS : For sending the Tech specific data in the FTN_ADD message */
      if (fad->tgen_data)
      {
        switch (fad->tgen_data->gmpls_type)
        {
#ifdef HAVE_PBB_TE
          case gmpls_entry_type_pbb_te:
            nsm_encode_tlv (pnt, size, NSM_FTN_CTYPE_TGEN_DATA,
                                         (sizeof (struct pbb_te_data)+1));
             TLV_ENCODE_PUTC (fad->tgen_data->gmpls_type);
             TLV_ENCODE_PUTL (fad->tgen_data->u.pbb.tesid);
             break;
#endif /* HAVE_PBB_TE */

          default:
             break;
        }
      }

      if (fad->pri_fec_prefix)
        {
          nsm_encode_tlv (pnt, size, NSM_FTN_CTYPE_PRIMARY_FEC_PREFIX,
                          (sizeof (struct pal_in4_addr) + 1));
          TLV_ENCODE_PUTC (fad->pri_fec_prefix->prefixlen);
          TLV_ENCODE_PUT_IN4_ADDR (&fad->pri_fec_prefix->u.prefix4);
        }
   }

  /* Fill in real length.  */
  length = *pnt - sp;
  length = pal_hton16 (length);
  pal_mem_cpy (lp, &length, 2);

  return *pnt - sp;
}

/* Decode GMPLS FTN IPv4 add message.  */
s_int32_t
nsm_decode_gmpls_ftn_data (u_char **pnt, u_int16_t *size,
                           struct ftn_add_gen_data *fad)
{
  u_char *sp = *pnt;
  u_int16_t length;
  u_int16_t diff;
  struct nsm_tlv_header tlv;
  int ret;

  /* Flags.  */
  TLV_DECODE_GETW (fad->flags);

  /* Total length of this message.  */
  TLV_DECODE_GETW (length);

  /* ID of this message.  */
  TLV_DECODE_GETL (fad->id);

  if ((CHECK_FLAG (fad->flags, NSM_MSG_GEN_FTN_FAST_DEL)) ||
      (CHECK_FLAG (fad->flags, NSM_MSG_GEN_FTN_FAST_ADD)))
    {
      /* Decode fast delete data.  */
      TLV_DECODE_GETL (fad->vrf_id);
      nsm_decode_gmpls_fec (pnt, size, &fad->ftn);
      TLV_DECODE_GETL (fad->ftn_ix);
#ifdef HAVE_GMPLS
      TLV_DECODE_GETL (fad->rev_xc_ix);
      TLV_DECODE_GETL (fad->rev_ilm_ix);
#endif /*HAVE_GMPLS */
    }
  else
    {
      /* Decode owner.  */
      ret = nsm_decode_mpls_owner_ipv4 (pnt, size, &fad->owner);
      if (ret < 0)
        return ret;

      /* Decode common part.  */
      TLV_DECODE_GETL (fad->vrf_id);
      nsm_decode_gmpls_fec (pnt, size, &fad->ftn);
      TLV_DECODE_GETL (fad->ftn_ix);
#ifdef HAVE_GMPLS
      TLV_DECODE_GETL (fad->rev_xc_ix);
      TLV_DECODE_GETL (fad->rev_ilm_ix);
#endif /*HAVE_GMPLS */
#ifdef HAVE_PACKET
#ifdef HAVE_DIFFSERV
      TLV_DECODE_GETC (fad->ds_info.lsp_type);
      TLV_DECODE_GET (fad->ds_info.dscp_exp_map, 8);
      TLV_DECODE_GETC (fad->ds_info.dscp);
      TLV_DECODE_GETC (fad->ds_info.af_set);
#endif /* HAVE_DIFFSERV */
#endif /* HAVE_PACKET */
      fad->nh_addr.afi = AFI_IP;
      TLV_DECODE_GET_IN4_ADDR (&fad->nh_addr.u.ipv4);
      nsm_decode_gmpls_label (pnt, size, &fad->out_label);
      TLV_DECODE_GETL (fad->oif_ix);
      TLV_DECODE_GET (&fad->lspid, 6);
      TLV_DECODE_GETL (fad->exp_bits);

#ifdef HAVE_PACKET
      if (CHECK_FLAG (fad->flags, NSM_MSG_FTN_DSCP_IN))
        TLV_DECODE_GETC (fad->dscp_in);
      else
        fad->dscp_in = DIFFSERV_INVALID_DSCP;
#endif /* HAVE_PACKET */

      TLV_DECODE_GETL (fad->lsp_bits.data);
      TLV_DECODE_GETC (fad->opcode);
      TLV_DECODE_GETL (fad->tunnel_id);
      TLV_DECODE_GETL (fad->protected_lsp_id);
      TLV_DECODE_GETL (fad->qos_resrc_id);
      TLV_DECODE_GETL (fad->bypass_ftn_ix);
    }

  /* Calculate remaining length.  */
  length -= (*pnt - sp);

  /* TLV parse. */
  while (length)
    {
      if (length < NSM_TLV_HEADER_SIZE)
        return NSM_ERR_PKT_TOO_SMALL;

      diff = *size;

      NSM_DECODE_TLV_HEADER (tlv);

      switch (tlv.type)
        {
        /* GELS : For sending the Tech specific data in the FTN_ADD message */
#ifdef HAVE_GMPLS
        case NSM_FTN_CTYPE_TGEN_DATA:
          fad->tgen_data= XCALLOC (MTYPE_TMP, sizeof(struct gmpls_tgen_data));
          TLV_DECODE_GETC (fad->tgen_data->gmpls_type);
          switch (fad->tgen_data->gmpls_type)
          {
#ifdef HAVE_PBB_TE
            case gmpls_entry_type_pbb_te:
              TLV_DECODE_GETL (fad->tgen_data->u.pbb.tesid);
              break;
#endif /* HAVE_PBB_TE */
            default:
              break;
          }
        break;
#endif /*HAVE_GMPLS*/
        case NSM_FTN_CTYPE_DESCRIPTION:
          fad->sz_desc = XCALLOC (MTYPE_TMP, tlv.length + 1);
          TLV_DECODE_GET (fad->sz_desc, tlv.length);
          break;
        case NSM_FTN_CTYPE_PRIMARY_FEC_PREFIX:
          fad->pri_fec_prefix = prefix_new ();
          fad->pri_fec_prefix->family = AF_INET;
          TLV_DECODE_GETC (fad->pri_fec_prefix->prefixlen);
          TLV_DECODE_GET_IN4_ADDR (&fad->pri_fec_prefix->u.prefix4);
          break;
        default:
          TLV_DECODE_SKIP (tlv.length);
          break;
        }
      length -= (diff - *size);
    }

  return *pnt - sp;
}

s_int32_t
nsm_parse_gmpls_ftn_data (u_char **pnt, u_int16_t *size,
                          struct nsm_msg_header *header, void *arg,
                          NSM_CALLBACK callback)
{
  int ret;
  struct ftn_add_gen_data msg;

  pal_mem_set (&msg, 0, sizeof (struct ftn_add_gen_data));

  ret = nsm_decode_gmpls_ftn_data (pnt, size, &msg);
  if (ret < 0)
    return ret;

  ret = 0;

  if (callback)
    ret = (*callback) (header, arg, &msg);

  if (msg.sz_desc)
    XFREE (MTYPE_TMP, msg.sz_desc);

  if (msg.pri_fec_prefix)
    prefix_free (msg.pri_fec_prefix);

  /* GELS : Delete the Tech specific data */
  if (msg.tgen_data)
    XFREE (MTYPE_TMP, msg.tgen_data);

  return ret;
}

s_int32_t
nsm_encode_gmpls_ilm_data (u_char **pnt, u_int16_t *size,
                           struct ilm_add_gen_data *iad)
{
  u_char *sp = *pnt;
  u_char *lp;
  u_int16_t length;
  int ret;
  struct fec_gen_entry gen_entry;

  NSM_MPLS_PREFIX_TO_FEC_GEN_ENTRY(&gen_entry, &iad->fec_prefix);

  /* Encode flags.  */
  TLV_ENCODE_PUTW (iad->flags);

  /* Total length of this message.  First we put place holder then
     fill in later on.  */
  lp = *pnt;
  TLV_ENCODE_PUT_EMPTY (2);

#ifdef HAVE_GMPLS
#endif /* HAVE_GMPLS */

  /* ID of this message.  */
  TLV_ENCODE_PUTL (iad->id);

  if ((CHECK_FLAG (iad->flags, NSM_MSG_ILM_FAST_DELETE)) ||
      (CHECK_FLAG (iad->flags, NSM_MSG_ILM_FAST_ADD)))
    {
      /* Fast Delete.  */
      TLV_ENCODE_PUTL (iad->iif_ix);
      nsm_encode_gmpls_label (pnt, size, &iad->in_label);

      TLV_ENCODE_PUTL (iad->ilm_ix);
#ifdef HAVE_GMPLS
      TLV_ENCODE_PUTL (iad->rev_xc_ix);
      TLV_ENCODE_PUTL (iad->rev_ftn_ix);
      TLV_ENCODE_PUTL (iad->rev_ilm_ix);
#endif /*HAVE_GMPLS */
    }
  else
    {
      /* Encode owner. */
      ret = nsm_encode_mpls_owner_ipv4 (pnt, size, &iad->owner);
      if (ret < 0)
        return ret;

      /* Encode common part.  */
      TLV_ENCODE_PUTL (iad->iif_ix);
      nsm_encode_gmpls_label (pnt, size, &iad->in_label);

      TLV_ENCODE_PUTL (iad->ilm_ix);
#ifdef HAVE_GMPLS
      TLV_ENCODE_PUTL (iad->rev_xc_ix);
      TLV_ENCODE_PUTL (iad->rev_ftn_ix);
      TLV_ENCODE_PUTL (iad->rev_ilm_ix);
#endif /*HAVE_GMPLS */

#ifdef HAVE_PACKET
#ifdef HAVE_DIFFSERV
      TLV_ENCODE_PUTC (iad->ds_info.lsp_type);
      TLV_ENCODE_PUT (iad->ds_info.dscp_exp_map, 8);
      TLV_ENCODE_PUTC (iad->ds_info.dscp);
      TLV_ENCODE_PUTC (iad->ds_info.af_set);
#endif /* HAVE_DIFFSERV */
#endif /* HAVE_PACKET */

      TLV_ENCODE_PUTL (iad->oif_ix);
      nsm_encode_gmpls_label (pnt, size, &iad->out_label);

      TLV_ENCODE_PUT_IN4_ADDR (&iad->nh_addr.u.ipv4);
      TLV_ENCODE_PUT (&iad->lspid, 6);
      TLV_ENCODE_PUTL (iad->n_pops);
      TLV_ENCODE_PUTC (iad->fec_prefix.prefixlen);
      TLV_ENCODE_PUT_IN4_ADDR (&iad->fec_prefix.u.prefix4);
      nsm_encode_gmpls_fec (pnt, size, &gen_entry);

      TLV_ENCODE_PUTL (iad->qos_resrc_id);
      TLV_ENCODE_PUTC (iad->opcode);
      TLV_ENCODE_PUTC (iad->primary);

      /* GELS : For sending the Tech specific data in the ILM_ADD message */
      if (iad->tgen_data)
      {
        switch (iad->tgen_data->gmpls_type)
        {
#ifdef HAVE_PBB_TE
          case gmpls_entry_type_pbb_te:
            nsm_encode_tlv (pnt, size, NSM_ILM_CTYPE_TGEN_DATA,
                                         (sizeof (struct pbb_te_data)+1));
             TLV_ENCODE_PUTC (iad->tgen_data->gmpls_type);
             TLV_ENCODE_PUTL (iad->tgen_data->u.pbb.tesid);
             break;
#endif /* HAVE_PBB_TE */

          default:
             break;
        }
      }
    }

  /* Fill in real length.  */
  length = *pnt - sp;
  length = pal_hton16 (length);
  pal_mem_cpy (lp, &length, 2);

  return *pnt - sp;
}

s_int32_t
nsm_decode_gmpls_ilm_data (u_char **pnt, u_int16_t *size,
                           struct ilm_add_gen_data *iad)
{
  u_char *sp = *pnt;
  u_int16_t length;
  u_int16_t diff;
  struct nsm_tlv_header tlv;
  int ret;
  struct fec_gen_entry gen_entry;

  NSM_MPLS_PREFIX_TO_FEC_GEN_ENTRY(&gen_entry, &iad->fec_prefix);

  /* Flags.  */
  TLV_DECODE_GETW (iad->flags);

  /* Total length of this message.  */
  TLV_DECODE_GETW (length);

  /* ID of this message.  */
  TLV_DECODE_GETL (iad->id);

  if (CHECK_FLAG (iad->flags, NSM_MSG_ILM_FAST_DELETE))
    {
      /* Delete.  */
      TLV_DECODE_GETL (iad->iif_ix);
      nsm_decode_gmpls_label (pnt, size, &iad->in_label);
      TLV_DECODE_GETL (iad->ilm_ix);
#ifdef HAVE_GMPLS
      TLV_DECODE_GETL (iad->rev_xc_ix);
      TLV_DECODE_GETL (iad->rev_ftn_ix);
      TLV_DECODE_GETL (iad->rev_ilm_ix);
#endif /*HAVE_GMPLS */
    }
  else
    {
      /* Decode owner. */
      ret = nsm_decode_mpls_owner_ipv4 (pnt, size, &iad->owner);
      if (ret < 0)
        return ret;

      /* Decode common part.  */
      TLV_DECODE_GETL (iad->iif_ix);
      nsm_decode_gmpls_label (pnt, size, &iad->in_label);
      TLV_DECODE_GETL (iad->ilm_ix);
#ifdef HAVE_GMPLS
      TLV_DECODE_GETL (iad->rev_xc_ix);
      TLV_DECODE_GETL (iad->rev_ftn_ix);
      TLV_DECODE_GETL (iad->rev_ilm_ix);
#endif /*HAVE_GMPLS */

#ifdef HAVE_PACKET
#ifdef HAVE_DIFFSERV
      TLV_DECODE_GETC (iad->ds_info.lsp_type);
      TLV_DECODE_GET (iad->ds_info.dscp_exp_map, 8);
      TLV_DECODE_GETC (iad->ds_info.dscp);
      TLV_DECODE_GETC (iad->ds_info.af_set);
#endif /* HAVE_DIFFSERV */
#endif /* HAVE_PACKET */

      TLV_DECODE_GETL (iad->oif_ix);
      nsm_decode_gmpls_label (pnt, size, &iad->out_label);
      iad->nh_addr.afi = AFI_IP;
      TLV_DECODE_GET_IN4_ADDR (&iad->nh_addr.u.ipv4);
      TLV_DECODE_GET (&iad->lspid, 6);
      TLV_DECODE_GETL (iad->n_pops);
      iad->fec_prefix.family = AF_INET;
      TLV_DECODE_GETC (iad->fec_prefix.prefixlen);
      TLV_DECODE_GET_IN4_ADDR (&iad->fec_prefix.u.prefix4);
      nsm_decode_gmpls_fec (pnt, size, &gen_entry);

      TLV_DECODE_GETL (iad->qos_resrc_id);
      TLV_DECODE_GETC (iad->opcode);
      TLV_DECODE_GETC (iad->primary);
    }

  /* Calculate remaining length.  */
  length -= (*pnt - sp);

  /* TLV parse. */
  while (length)
    {
      if (length < NSM_TLV_HEADER_SIZE)
        return NSM_ERR_PKT_TOO_SMALL;

      diff = *size;

      NSM_DECODE_TLV_HEADER (tlv);

      switch (tlv.type)
      {

        /* GELS : For decoding the Tech specific data */
#ifdef HAVE_GMPLS
        case NSM_ILM_CTYPE_TGEN_DATA:
          iad->tgen_data= XCALLOC (MTYPE_TMP, sizeof(struct gmpls_tgen_data));
          TLV_DECODE_GETC (iad->tgen_data->gmpls_type);
          switch (iad->tgen_data->gmpls_type)
          {
#ifdef HAVE_PBB_TE
            case gmpls_entry_type_pbb_te:
              TLV_DECODE_GETL (iad->tgen_data->u.pbb.tesid);
              break;
#endif /* HAVE_PBB_TE */

            default:
              break;
          }
        break;
#endif /*HAVE_GMPLS */
        default:
          TLV_DECODE_SKIP (tlv.length);
          break;
      }
      length -= (diff - *size);
    }

  return *pnt - sp;
}

s_int32_t
nsm_parse_gmpls_ilm_data (u_char **pnt, u_int16_t *size,
                          struct nsm_msg_header *header, void *arg,
                          NSM_CALLBACK callback)
{
  int ret;
  struct ilm_add_gen_data msg;

  pal_mem_set (&msg, 0, sizeof (struct ilm_add_gen_data));

  ret = nsm_decode_gmpls_ilm_data (pnt, size, &msg);
  if (ret < 0)
    return ret;

  ret = 0;
  if (callback)
    ret = (*callback) (header, arg, &msg);

  /* GELS : For deleting the Tech specific data */
  if (msg.tgen_data)
    XFREE (MTYPE_TMP, msg.tgen_data);

  return ret;
}

s_int32_t
nsm_encode_gmpls_ftn_bidir_data (u_char **pnt, u_int16_t *size,
                                 struct ftn_bidir_add_data *fad)
{
  s_int32_t fbytes, ibytes;
  u_char *sp = *pnt;

  /* Identifier.  */
  TLV_ENCODE_PUTL (fad->id);

  fbytes = nsm_encode_gmpls_ftn_data (pnt, size, &fad->ftn);
  if (fbytes < 0)
    return fbytes;

  ibytes = nsm_encode_gmpls_ilm_data (pnt, size, &fad->ilm);
  if (ibytes < 0)
    return ibytes;

  return *pnt - sp;
}

s_int32_t
nsm_decode_gmpls_ftn_bidir_data (u_char **pnt, u_int16_t *size,
                                 struct ftn_bidir_add_data *fad)
{
  s_int32_t fbytes, ibytes;

  /* Identifier.  */
  TLV_DECODE_GETL (fad->id);

  fbytes = nsm_decode_gmpls_ftn_data (pnt, size, &fad->ftn);
  if (fbytes < 0)
    return fbytes;

  ibytes = nsm_decode_gmpls_ilm_data (pnt, size, &fad->ilm);
  if (ibytes < 0)
    return ibytes;

  return fbytes + ibytes;
}

s_int32_t
nsm_parse_gmpls_ftn_bidir_data (u_char **pnt, u_int16_t *size,
                                struct nsm_msg_header *header, void *arg,
                                NSM_CALLBACK callback)
{
  int ret;
  struct ftn_bidir_add_data msg;

  pal_mem_set (&msg, 0, sizeof (struct ftn_bidir_add_data));

  ret = nsm_decode_gmpls_ftn_bidir_data (pnt, size, &msg);
  if (ret < 0)
    return ret;

  ret = 0;
  if (callback)
    ret = (*callback) (header, arg, &msg);

  if (msg.ftn.sz_desc)
    XFREE (MTYPE_TMP, msg.ftn.sz_desc);

  if (msg.ftn.pri_fec_prefix)
    prefix_free (msg.ftn.pri_fec_prefix);

  return ret;
}

s_int32_t
nsm_decode_gmpls_ilm_bidir_data (u_char **pnt, u_int16_t *size,
                                 struct ilm_bidir_add_data *iad)
{
  s_int32_t i1bytes, i2bytes;
  u_char *sp = *pnt;

  i1bytes = nsm_decode_gmpls_ilm_data (pnt, size, &iad->ilm_fwd);
  if (i1bytes < 0)
    return i1bytes;

  i2bytes = nsm_decode_gmpls_ilm_data (pnt, size, &iad->ilm_bwd);
  if (i2bytes < 0)
    return i2bytes;

  return *pnt - sp;
}

s_int32_t
nsm_encode_gmpls_ilm_bidir_data (u_char **pnt, u_int16_t *size,
                                 struct ilm_bidir_add_data *iad)
{
  s_int32_t i1bytes, i2bytes;

  i1bytes = nsm_encode_gmpls_ilm_data (pnt, size, &iad->ilm_fwd);
  if (i1bytes < 0)
    return i1bytes;

  i2bytes = nsm_encode_gmpls_ilm_data (pnt, size, &iad->ilm_bwd);
  if (i2bytes < 0)
    return i2bytes;

  return i1bytes + i2bytes;
}

s_int32_t
nsm_parse_gmpls_ilm_bidir_data (u_char **pnt, u_int16_t *size,
                          struct nsm_msg_header *header, void *arg,
                          NSM_CALLBACK callback)
{
  int ret;
  struct ilm_bidir_add_data msg;

  pal_mem_set (&msg, 0, sizeof (struct ilm_bidir_add_data));

  ret = nsm_decode_gmpls_ilm_bidir_data (pnt, size, &msg);
  if (ret < 0)
    return ret;

  ret = 0;
  if (callback)
    ret = (*callback) (header, arg, &msg);

  return ret;
}

#endif /* HAVE_MPLS */

#ifdef HAVE_MPLS
/* Encode MPLS FTN IPv4 data */
s_int32_t
nsm_encode_ftn_data_ipv4 (u_char **pnt, u_int16_t *size,
                          struct ftn_add_data *fad)
{
  u_char *sp = *pnt;
  u_char *lp;
  u_int16_t length;
  int ret;

  /* Encode flags.  */
  TLV_ENCODE_PUTW (fad->flags);

  /* Total length of this message.  First we put place holder then
     fill in later on.  */
  lp = *pnt;
  TLV_ENCODE_PUT_EMPTY (2);

  /* ID of this message.  */
  TLV_ENCODE_PUTL (fad->id);

  if (CHECK_FLAG (fad->flags, NSM_MSG_FTN_FAST_DELETE))
    {
      /* Encode fast delete data.  */
      TLV_ENCODE_PUTL (fad->vrf_id);
      TLV_ENCODE_PUTC (fad->fec_prefix.prefixlen);
      TLV_ENCODE_PUT_IN4_ADDR (&fad->fec_prefix.u.prefix4);
      TLV_ENCODE_PUTL (fad->ftn_ix);
    }
  else
    {
      /* Encode owner.  */
      ret = nsm_encode_mpls_owner_ipv4 (pnt, size, &fad->owner);
      if (ret < 0)
        return ret;

      /* Encode common part.  */
      TLV_ENCODE_PUTL (fad->vrf_id);
      TLV_ENCODE_PUTC (fad->fec_prefix.prefixlen);
      TLV_ENCODE_PUT_IN4_ADDR (&fad->fec_prefix.u.prefix4);
      TLV_ENCODE_PUTL (fad->ftn_ix);
#ifdef HAVE_DIFFSERV
      TLV_ENCODE_PUTC (fad->ds_info.lsp_type);
      TLV_ENCODE_PUT (fad->ds_info.dscp_exp_map, 8);
      TLV_ENCODE_PUTC (fad->ds_info.dscp);
      TLV_ENCODE_PUTC (fad->ds_info.af_set);
#endif /* HAVE_DIFFSERV */
      TLV_ENCODE_PUT_IN4_ADDR (&fad->nh_addr.u.ipv4);
      TLV_ENCODE_PUTL (fad->out_label);
      TLV_ENCODE_PUTL (fad->oif_ix);
      TLV_ENCODE_PUT (&fad->lspid, 6);
      TLV_ENCODE_PUTL (fad->exp_bits);
      if (CHECK_FLAG (fad->flags, NSM_MSG_FTN_DSCP_IN))
        TLV_ENCODE_PUTC (fad->dscp_in);
      TLV_ENCODE_PUTL (fad->lsp_bits.data);
      TLV_ENCODE_PUTC (fad->opcode);
      TLV_ENCODE_PUTL (fad->tunnel_id);
      TLV_ENCODE_PUTL (fad->protected_lsp_id);
      TLV_ENCODE_PUTL (fad->qos_resrc_id);
      TLV_ENCODE_PUTL (fad->bypass_ftn_ix);

      /* String is encoded as optional TLV.  */
      if (fad->sz_desc)
        {
          nsm_encode_tlv (pnt, size, NSM_FTN_CTYPE_DESCRIPTION,
                          pal_strlen (fad->sz_desc));
          TLV_ENCODE_PUT (fad->sz_desc, pal_strlen (fad->sz_desc));
        }
      if (fad->pri_fec_prefix)
        {
          nsm_encode_tlv (pnt, size, NSM_FTN_CTYPE_PRIMARY_FEC_PREFIX,
                          (sizeof (struct pal_in4_addr) + 1));
          TLV_ENCODE_PUTC (fad->pri_fec_prefix->prefixlen);
          TLV_ENCODE_PUT_IN4_ADDR (&fad->pri_fec_prefix->u.prefix4);
        }
   }

  /* Fill in real length.  */
  length = *pnt - sp;
  length = pal_hton16 (length);
  pal_mem_cpy (lp, &length, 2);

  return *pnt - sp;
}

/* Decode MPLS FTN IPv4 add message.  */
s_int32_t
nsm_decode_ftn_data_ipv4 (u_char **pnt, u_int16_t *size,
                          struct ftn_add_data *fad)
{
  u_char *sp = *pnt;
  u_int16_t length;
  u_int16_t diff;
  struct nsm_tlv_header tlv;
  int ret;

  /* Flags.  */
  TLV_DECODE_GETW (fad->flags);

  /* Total length of this message.  */
  TLV_DECODE_GETW (length);

  /* ID of this message.  */
  TLV_DECODE_GETL (fad->id);

  if (CHECK_FLAG (fad->flags, NSM_MSG_FTN_FAST_DELETE))
    {
      /* Decode fast delete data.  */
      TLV_DECODE_GETL (fad->vrf_id);
      fad->fec_prefix.family = AF_INET;
      TLV_DECODE_GETC (fad->fec_prefix.prefixlen);
      TLV_DECODE_GET_IN4_ADDR (&fad->fec_prefix.u.prefix4);
      TLV_DECODE_GETL (fad->ftn_ix);
    }
  else
    {
      /* Decode owner.  */
      ret = nsm_decode_mpls_owner_ipv4 (pnt, size, &fad->owner);
      if (ret < 0)
        return ret;

      /* Decode common part.  */
      TLV_DECODE_GETL (fad->vrf_id);
      fad->fec_prefix.family = AF_INET;
      TLV_DECODE_GETC (fad->fec_prefix.prefixlen);
      TLV_DECODE_GET_IN4_ADDR (&fad->fec_prefix.u.prefix4);
      TLV_DECODE_GETL (fad->ftn_ix);
#ifdef HAVE_DIFFSERV
      TLV_DECODE_GETC (fad->ds_info.lsp_type);
      TLV_DECODE_GET (fad->ds_info.dscp_exp_map, 8);
      TLV_DECODE_GETC (fad->ds_info.dscp);
      TLV_DECODE_GETC (fad->ds_info.af_set);
#endif /* HAVE_DIFFSERV */
      fad->nh_addr.afi = AFI_IP;
      TLV_DECODE_GET_IN4_ADDR (&fad->nh_addr.u.ipv4);
      TLV_DECODE_GETL (fad->out_label);
      TLV_DECODE_GETL (fad->oif_ix);
      TLV_DECODE_GET (&fad->lspid, 6);
      TLV_DECODE_GETL (fad->exp_bits);
      if (CHECK_FLAG (fad->flags, NSM_MSG_FTN_DSCP_IN))
        TLV_DECODE_GETC (fad->dscp_in);
      else
        fad->dscp_in = DIFFSERV_INVALID_DSCP;
      TLV_DECODE_GETL (fad->lsp_bits.data);
      TLV_DECODE_GETC (fad->opcode);
      TLV_DECODE_GETL (fad->tunnel_id);
      TLV_DECODE_GETL (fad->protected_lsp_id);
      TLV_DECODE_GETL (fad->qos_resrc_id);
      TLV_DECODE_GETL (fad->bypass_ftn_ix);
    }

  /* Calculate remaining length.  */
  length -= (*pnt - sp);

  /* TLV parse. */
  while (length)
    {
      if (length < NSM_TLV_HEADER_SIZE)
        return NSM_ERR_PKT_TOO_SMALL;

      diff = *size;

      NSM_DECODE_TLV_HEADER (tlv);

      switch (tlv.type)
        {
        case NSM_FTN_CTYPE_DESCRIPTION:
          fad->sz_desc = XCALLOC (MTYPE_TMP, tlv.length + 1);
          TLV_DECODE_GET (fad->sz_desc, tlv.length);
          break;
        case NSM_FTN_CTYPE_PRIMARY_FEC_PREFIX:
          fad->pri_fec_prefix = prefix_new ();
          fad->pri_fec_prefix->family = AF_INET;
          TLV_DECODE_GETC (fad->pri_fec_prefix->prefixlen);
          TLV_DECODE_GET_IN4_ADDR (&fad->pri_fec_prefix->u.prefix4);
          break;
        default:
          TLV_DECODE_SKIP (tlv.length);
          break;
        }
      length -= (diff - *size);
    }

  return *pnt - sp;
}

#ifdef HAVE_IPV6
s_int32_t
nsm_encode_ftn_data_ipv6 (u_char **pnt, u_int16_t *size,
                          struct ftn_add_data *fad)
{
  u_char *sp = *pnt;
  u_char *lp;
  u_int16_t length;
  int ret;

  /* Encode flags.  */
  TLV_ENCODE_PUTW (fad->flags);

  /* Total length of this message.  First we put place holder then
     fill in later on.  */
  lp = *pnt;
  TLV_ENCODE_PUT_EMPTY (2);

  /* ID of this message.  */
  TLV_ENCODE_PUTL (fad->id);

  if (CHECK_FLAG (fad->flags, NSM_MSG_GEN_FTN_FAST_DEL))
    {
      /* Encode fast delete data.  */
      TLV_ENCODE_PUTL (fad->vrf_id);
      TLV_ENCODE_PUTC (fad->fec_prefix.family);
      TLV_ENCODE_PUTC (fad->fec_prefix.prefixlen);
      if (fad->fec_prefix.family == AF_INET6)
        {
          TLV_ENCODE_PUT_IN6_ADDR (&fad->fec_prefix.u.prefix6);
        }
      else
        {
          TLV_ENCODE_PUT_IN4_ADDR (&fad->fec_prefix.u.prefix4);
        }

      TLV_ENCODE_PUTL (fad->ftn_ix);
    }
  else
    {
      /* Encode owner.  */
      if (fad->nh_addr.afi == AFI_IP)
        ret = nsm_encode_mpls_owner_ipv4 (pnt, size, &fad->owner);
      else
        ret = nsm_encode_mpls_owner_ipv6 (pnt, size, &fad->owner);

      if (ret < 0)
        return ret;

      /* Encode common part.  */
      TLV_ENCODE_PUTL (fad->vrf_id);
      TLV_ENCODE_PUTC (fad->fec_prefix.family);
      TLV_ENCODE_PUTC (fad->fec_prefix.prefixlen);
      if (fad->fec_prefix.family == AF_INET6)
        {
          TLV_ENCODE_PUT_IN6_ADDR (&fad->fec_prefix.u.prefix6);
        }
      else
        {
          TLV_ENCODE_PUT_IN4_ADDR (&fad->fec_prefix.u.prefix4);
        }

      TLV_ENCODE_PUTL (fad->ftn_ix);
#ifdef HAVE_DIFFSERV
      TLV_ENCODE_PUTC (fad->ds_info.lsp_type);
      TLV_ENCODE_PUT (fad->ds_info.dscp_exp_map, 8);
      TLV_ENCODE_PUTC (fad->ds_info.dscp);
      TLV_ENCODE_PUTC (fad->ds_info.af_set);
#endif /* HAVE_DIFFSERV */
      TLV_ENCODE_PUTC (fad->nh_addr.afi);
      if (fad->nh_addr.afi == AFI_IP)
        TLV_ENCODE_PUT_IN4_ADDR (&fad->nh_addr.u.ipv4);
      else if (fad->nh_addr.afi == AFI_IP6)
        TLV_ENCODE_PUT_IN6_ADDR (&fad->nh_addr.u.ipv6);

      TLV_ENCODE_PUTL (fad->out_label);
      TLV_ENCODE_PUTL (fad->oif_ix);
      TLV_ENCODE_PUT (&fad->lspid, 6);
      TLV_ENCODE_PUTL (fad->exp_bits);
      if (CHECK_FLAG (fad->flags, NSM_MSG_FTN_DSCP_IN))
        TLV_ENCODE_PUTC (fad->dscp_in);
      TLV_ENCODE_PUTL (fad->lsp_bits.data);
      TLV_ENCODE_PUTC (fad->opcode);
      TLV_ENCODE_PUTL (fad->tunnel_id);
      TLV_ENCODE_PUTL (fad->protected_lsp_id);
      TLV_ENCODE_PUTL (fad->qos_resrc_id);
      TLV_ENCODE_PUTL (fad->bypass_ftn_ix);

      /* String is encoded as optional TLV.  */
      if (fad->sz_desc)
        {
          nsm_encode_tlv (pnt, size, NSM_FTN_CTYPE_DESCRIPTION,
                          pal_strlen (fad->sz_desc));
          TLV_ENCODE_PUT (fad->sz_desc, pal_strlen (fad->sz_desc));
        }
      if (fad->pri_fec_prefix)
        {
          nsm_encode_tlv (pnt, size, NSM_FTN_CTYPE_PRIMARY_FEC_PREFIX,
                          (sizeof (struct pal_in4_addr) + 1));
          TLV_ENCODE_PUTC (fad->pri_fec_prefix->prefixlen);
          TLV_ENCODE_PUT_IN6_ADDR (&fad->pri_fec_prefix->u.prefix6);
        }
   }

  /* Fill in real length.  */
  length = *pnt - sp;
  length = pal_hton16 (length);
  pal_mem_cpy (lp, &length, 2);

  return *pnt - sp;
}

/* Decode MPLS FTN IPv6 add message.  */
s_int32_t
nsm_decode_ftn_data_ipv6 (u_char **pnt, u_int16_t *size,
                          struct ftn_add_data *fad)
{
  u_char *sp = *pnt;
  u_int16_t length;
  u_int16_t diff;
  struct nsm_tlv_header tlv;
  int ret;

  /* Flags.  */
  TLV_DECODE_GETW (fad->flags);

  /* Total length of this message.  */
  TLV_DECODE_GETW (length);

  /* ID of this message.  */
  TLV_DECODE_GETL (fad->id);

  if (CHECK_FLAG (fad->flags, NSM_MSG_FTN_FAST_DELETE))
    {
      /* Decode fast delete data.  */
      TLV_DECODE_GETL (fad->vrf_id);
      TLV_DECODE_GETC (fad->fec_prefix.family);
      TLV_DECODE_GETC (fad->fec_prefix.prefixlen);

      if (fad->fec_prefix.family == AF_INET6)
        {
          TLV_DECODE_GET_IN6_ADDR (&fad->fec_prefix.u.prefix6);
        }
      else
        {
          TLV_DECODE_GET_IN4_ADDR (&fad->fec_prefix.u.prefix4);
        }

      TLV_DECODE_GETL (fad->ftn_ix);
    }
  else
    {
      /* Decode owner.  */
      ret = nsm_decode_mpls_owner_ipv6 (pnt, size, &fad->owner);
      if (ret < 0)
        return ret;

      /* Decode common part.  */
      TLV_DECODE_GETL (fad->vrf_id);
      TLV_DECODE_GETC (fad->fec_prefix.family);
      TLV_DECODE_GETC (fad->fec_prefix.prefixlen);
      if (fad->fec_prefix.family == AF_INET6)
        {
          TLV_DECODE_GET_IN6_ADDR (&fad->fec_prefix.u.prefix6);
        }
      else
        {
          TLV_DECODE_GET_IN4_ADDR (&fad->fec_prefix.u.prefix4);
        }

      TLV_DECODE_GETL (fad->ftn_ix);
#ifdef HAVE_DIFFSERV
      TLV_DECODE_GETC (fad->ds_info.lsp_type);
      TLV_DECODE_GET (fad->ds_info.dscp_exp_map, 8);
      TLV_DECODE_GETC (fad->ds_info.dscp);
      TLV_DECODE_GETC (fad->ds_info.af_set);
#endif /* HAVE_DIFFSERV */
      TLV_DECODE_GETC (fad->nh_addr.afi);
      if (fad->nh_addr.afi == AFI_IP)
        {
          TLV_DECODE_GET_IN4_ADDR (&fad->nh_addr.u.ipv4);
        }
      else if (fad->nh_addr.afi == AFI_IP6)
        {
          TLV_DECODE_GET_IN6_ADDR (&fad->nh_addr.u.ipv6);
        }

      TLV_DECODE_GETL (fad->out_label);
      TLV_DECODE_GETL (fad->oif_ix);
      TLV_DECODE_GET (&fad->lspid, 6);
      TLV_DECODE_GETL (fad->exp_bits);
      if (CHECK_FLAG (fad->flags, NSM_MSG_FTN_DSCP_IN))
        TLV_DECODE_GETC (fad->dscp_in);
      else
        fad->dscp_in = DIFFSERV_INVALID_DSCP;
      TLV_DECODE_GETL (fad->lsp_bits.data);
      TLV_DECODE_GETC (fad->opcode);
      TLV_DECODE_GETL (fad->tunnel_id);
      TLV_DECODE_GETL (fad->protected_lsp_id);
      TLV_DECODE_GETL (fad->qos_resrc_id);
      TLV_DECODE_GETL (fad->bypass_ftn_ix);
    }

  /* Calculate remaining length.  */
  length -= (*pnt - sp);

  /* TLV parse. */
  while (length)
    {
      if (length < NSM_TLV_HEADER_SIZE)
        return NSM_ERR_PKT_TOO_SMALL;

      diff = *size;

      NSM_DECODE_TLV_HEADER (tlv);

      switch (tlv.type)
        {
        case NSM_FTN_CTYPE_DESCRIPTION:
          fad->sz_desc = XCALLOC (MTYPE_TMP, tlv.length + 1);
          TLV_DECODE_GET (fad->sz_desc, tlv.length);
          break;
        case NSM_FTN_CTYPE_PRIMARY_FEC_PREFIX:
          fad->pri_fec_prefix = prefix_new ();
          fad->pri_fec_prefix->family = AF_INET;
          TLV_DECODE_GETC (fad->pri_fec_prefix->prefixlen);
          TLV_DECODE_GET_IN6_ADDR (&fad->pri_fec_prefix->u.prefix6);
          break;
        default:
          TLV_DECODE_SKIP (tlv.length);
          break;
        }
      length -= (diff - *size);
    }

  return *pnt - sp;
}
#endif /* HAVE_IPV6 */

/* Encode MPLS FTN return data.  */
s_int32_t
nsm_encode_ftn_ret_data (u_char **pnt, u_int16_t *size,
                         struct ftn_ret_data *frd)
{
  u_char *sp = *pnt;
  int ret;

  /* Encode flags.  */
  TLV_ENCODE_PUTW (frd->flags);

  /* ID of this message.  */
  TLV_ENCODE_PUTL (frd->id);

  /* Encode owner. */
#ifdef HAVE_IPV6
  ret = nsm_encode_mpls_owner_ipv6 (pnt, size, &frd->owner);
#else
  ret = nsm_encode_mpls_owner_ipv4 (pnt, size, &frd->owner);
#endif

  if (ret < 0)
    return ret;

  /* Encode common part.  */
  TLV_ENCODE_PUTL (frd->ftn_ix);
  TLV_ENCODE_PUTL (frd->xc_ix);
  TLV_ENCODE_PUTL (frd->nhlfe_ix);

  return *pnt - sp;
}

/* Decode MPLS FTN return data.  */
s_int32_t
nsm_decode_ftn_ret_data (u_char **pnt, u_int16_t *size,
                         struct ftn_ret_data *frd)
{
  u_char *sp = *pnt;
  int ret;

  /* Flags.  */
  TLV_DECODE_GETW (frd->flags);

  /* Message ID.  */
  TLV_DECODE_GETL (frd->id);

  /* Decode owner. */
#ifdef HAVE_IPV6
  ret = nsm_decode_mpls_owner_ipv6 (pnt, size, &frd->owner);
#else
  ret = nsm_decode_mpls_owner_ipv4 (pnt, size, &frd->owner);
#endif

  if (ret < 0)
    return ret;

  /* Decode common part.  */
  TLV_DECODE_GETL (frd->ftn_ix);
  TLV_DECODE_GETL (frd->xc_ix);
  TLV_DECODE_GETL (frd->nhlfe_ix);

  return *pnt - sp;
}

#ifdef HAVE_BFD
/* Encode the FTN Down data. */
int
nsm_encode_ftn_down_data (u_char **pnt, u_int16_t *size,
                           struct ftn_down_data *fdd)
{
  u_char *sp = *pnt;

  TLV_ENCODE_PUTC (fdd->ftn_owner);

  if (fdd->ftn_owner == MPLS_LDP)
    {
      TLV_ENCODE_PUT_IN4_ADDR (&fdd->u.ldp_key.addr);
      TLV_ENCODE_PUTL (fdd->u.ldp_key.lsr_id);
      TLV_ENCODE_PUTW (fdd->u.ldp_key.label_space);
    }
  else
    {
      TLV_ENCODE_PUTW (fdd->u.rsvp_key.trunk_id);
      TLV_ENCODE_PUTW (fdd->u.rsvp_key.lsp_id);
      TLV_ENCODE_PUT_IN4_ADDR (&fdd->u.rsvp_key.ingr);
      TLV_ENCODE_PUT_IN4_ADDR (&fdd->u.rsvp_key.egr);
    }

  return *pnt - sp;
}

/* Decode the FTN Down data. */
int
nsm_decode_ftn_down_data (u_char **pnt, u_int16_t *size,
                           struct ftn_down_data *fdd)
{
  u_char *sp = *pnt;

  TLV_DECODE_GETC (fdd->ftn_owner);

  if (fdd->ftn_owner == MPLS_LDP)
    {
      TLV_DECODE_GET_IN4_ADDR (&fdd->u.ldp_key.addr);
      TLV_DECODE_GETL (fdd->u.ldp_key.lsr_id);
      TLV_DECODE_GETW (fdd->u.ldp_key.label_space);
    }
  else
    {
      TLV_DECODE_GETW (fdd->u.rsvp_key.trunk_id);
      TLV_DECODE_GETW (fdd->u.rsvp_key.lsp_id);
      TLV_DECODE_GET_IN4_ADDR (&fdd->u.rsvp_key.ingr);
      TLV_DECODE_GET_IN4_ADDR (&fdd->u.rsvp_key.egr);
    }

  return *pnt - sp;
}
#endif /* HAVE_BFD */

s_int32_t
nsm_encode_ilm_data_ipv4 (u_char **pnt, u_int16_t *size,
                          struct ilm_add_data *iad)
{
  u_char *sp = *pnt;
  u_char *lp;
  u_int16_t length;
  int ret;

  /* Encode flags.  */
  TLV_ENCODE_PUTW (iad->flags);

  /* Total length of this message.  First we put place holder then
     fill in later on.  */
  lp = *pnt;
  TLV_ENCODE_PUT_EMPTY (2);

  /* ID of this message.  */
  TLV_ENCODE_PUTL (iad->id);

  if (CHECK_FLAG (iad->flags, NSM_MSG_ILM_FAST_DELETE))
    {
      /* Fast Delete.  */
      TLV_ENCODE_PUTL (iad->iif_ix);
      TLV_ENCODE_PUTL (iad->in_label);
      TLV_ENCODE_PUTL (iad->ilm_ix);
    }
  else
    {
      /* Encode owner. */
      ret = nsm_encode_mpls_owner_ipv4 (pnt, size, &iad->owner);
      if (ret < 0)
        return ret;

      /* Encode common part.  */
      TLV_ENCODE_PUTL (iad->iif_ix);
      TLV_ENCODE_PUTL (iad->in_label);
      TLV_ENCODE_PUTL (iad->ilm_ix);
#ifdef HAVE_DIFFSERV
      TLV_ENCODE_PUTC (iad->ds_info.lsp_type);
      TLV_ENCODE_PUT (iad->ds_info.dscp_exp_map, 8);
      TLV_ENCODE_PUTC (iad->ds_info.dscp);
      TLV_ENCODE_PUTC (iad->ds_info.af_set);
#endif /* HAVE_DIFFSERV */
      TLV_ENCODE_PUTL (iad->oif_ix);
      TLV_ENCODE_PUTL (iad->out_label);
      TLV_ENCODE_PUT_IN4_ADDR (&iad->nh_addr.u.ipv4);
      TLV_ENCODE_PUT (&iad->lspid, 6);
      TLV_ENCODE_PUTL (iad->n_pops);
      TLV_ENCODE_PUTC (iad->fec_prefix.prefixlen);
      TLV_ENCODE_PUT_IN4_ADDR (&iad->fec_prefix.u.prefix4);
      TLV_ENCODE_PUTL (iad->qos_resrc_id);
      TLV_ENCODE_PUTC (iad->opcode);
      TLV_ENCODE_PUTC (iad->primary);
    }


  /* Fill in real length.  */
  length = *pnt - sp;
  length = pal_hton16 (length);
  pal_mem_cpy (lp, &length, 2);

  return *pnt - sp;
}

s_int32_t
nsm_decode_ilm_data_ipv4 (u_char **pnt, u_int16_t *size,
                          struct ilm_add_data *iad)
{
  u_char *sp = *pnt;
  u_int16_t length;
  u_int16_t diff;
  struct nsm_tlv_header tlv;
  int ret;

  /* Flags.  */
  TLV_DECODE_GETW (iad->flags);

  /* Total length of this message.  */
  TLV_DECODE_GETW (length);

  /* ID of this message.  */
  TLV_DECODE_GETL (iad->id);

  if (CHECK_FLAG (iad->flags, NSM_MSG_ILM_FAST_DELETE))
    {
      /* Delete.  */
      TLV_DECODE_GETL (iad->iif_ix);
      TLV_DECODE_GETL (iad->in_label);
      TLV_DECODE_GETL (iad->ilm_ix);
    }
  else
    {
      /* Decode owner. */
      ret = nsm_decode_mpls_owner_ipv4 (pnt, size, &iad->owner);
      if (ret < 0)
        return ret;

      /* Decode common part.  */
      TLV_DECODE_GETL (iad->iif_ix);
      TLV_DECODE_GETL (iad->in_label);
      TLV_DECODE_GETL (iad->ilm_ix);
#ifdef HAVE_DIFFSERV
      TLV_DECODE_GETC (iad->ds_info.lsp_type);
      TLV_DECODE_GET (iad->ds_info.dscp_exp_map, 8);
      TLV_DECODE_GETC (iad->ds_info.dscp);
      TLV_DECODE_GETC (iad->ds_info.af_set);
#endif /* HAVE_DIFFSERV */
      TLV_DECODE_GETL (iad->oif_ix);
      TLV_DECODE_GETL (iad->out_label);
      iad->nh_addr.afi = AFI_IP;
      TLV_DECODE_GET_IN4_ADDR (&iad->nh_addr.u.ipv4);
      TLV_DECODE_GET (&iad->lspid, 6);
      TLV_DECODE_GETL (iad->n_pops);
      iad->fec_prefix.family = AF_INET;
      TLV_DECODE_GETC (iad->fec_prefix.prefixlen);
      TLV_DECODE_GET_IN4_ADDR (&iad->fec_prefix.u.prefix4);
      TLV_DECODE_GETL (iad->qos_resrc_id);
      TLV_DECODE_GETC (iad->opcode);
      TLV_DECODE_GETC (iad->primary);
    }

  /* Calculate remaining length.  */
  length -= (*pnt - sp);

  /* TLV parse. */
  while (length)
    {
      if (length < NSM_TLV_HEADER_SIZE)
        return NSM_ERR_PKT_TOO_SMALL;

      diff = *size;

      NSM_DECODE_TLV_HEADER (tlv);

      switch (tlv.type)
        {
        default:
          TLV_DECODE_SKIP (tlv.length);
          break;
        }
      length -= (diff - *size);
    }

  return *pnt - sp;
}

#ifdef HAVE_IPV6
s_int32_t
nsm_encode_ilm_data_ipv6 (u_char **pnt, u_int16_t *size,
                          struct ilm_add_data *iad)
{
  u_char *sp = *pnt;
  u_char *lp;
  u_int16_t length;
  int ret;

  /* Encode flags.  */
  TLV_ENCODE_PUTW (iad->flags);

  /* Total length of this message.  First we put place holder then
     fill in later on.  */
  lp = *pnt;
  TLV_ENCODE_PUT_EMPTY (2);

  /* ID of this message.  */
  TLV_ENCODE_PUTL (iad->id);

  if (CHECK_FLAG (iad->flags, NSM_MSG_ILM_FAST_DELETE))
    {
      /* Fast Delete.  */
      TLV_ENCODE_PUTL (iad->iif_ix);
      TLV_ENCODE_PUTL (iad->in_label);
      TLV_ENCODE_PUTL (iad->ilm_ix);
    }
  else
    {
      /* Encode owner. */
      ret = nsm_encode_mpls_owner_ipv6 (pnt, size, &iad->owner);
      if (ret < 0)
        return ret;

      /* Encode common part.  */
      TLV_ENCODE_PUTL (iad->iif_ix);
      TLV_ENCODE_PUTL (iad->in_label);
      TLV_ENCODE_PUTL (iad->ilm_ix);
#ifdef HAVE_DIFFSERV
      TLV_ENCODE_PUTC (iad->ds_info.lsp_type);
      TLV_ENCODE_PUT (iad->ds_info.dscp_exp_map, 8);
      TLV_ENCODE_PUTC (iad->ds_info.dscp);
      TLV_ENCODE_PUTC (iad->ds_info.af_set);
#endif /* HAVE_DIFFSERV */
      TLV_ENCODE_PUTL (iad->oif_ix);
      TLV_ENCODE_PUTL (iad->out_label);
      TLV_ENCODE_PUTC (iad->nh_addr.afi);

      /* based on address family encode the address */
      if (iad->nh_addr.afi == AFI_IP)
        TLV_ENCODE_PUT_IN4_ADDR (&iad->nh_addr.u.ipv4);
      else
        TLV_ENCODE_PUT_IN6_ADDR (&iad->nh_addr.u.ipv6);

      TLV_ENCODE_PUT (&iad->lspid, 6);
      TLV_ENCODE_PUTL (iad->n_pops);
      TLV_ENCODE_PUTC (iad->fec_prefix.prefixlen);
      TLV_ENCODE_PUT_IN6_ADDR (&iad->fec_prefix.u.prefix6);
      TLV_ENCODE_PUTL (iad->qos_resrc_id);
      TLV_ENCODE_PUTC (iad->opcode);
      TLV_ENCODE_PUTC (iad->primary);
    }


  /* Fill in real length.  */
  length = *pnt - sp;
  length = pal_hton16 (length);
  pal_mem_cpy (lp, &length, 2);

  return *pnt - sp;
}

s_int32_t
nsm_decode_ilm_data_ipv6 (u_char **pnt, u_int16_t *size,
                          struct ilm_add_data *iad)
{
  u_char *sp = *pnt;
  u_int16_t length;
  u_int16_t diff;
  struct nsm_tlv_header tlv;
  int ret;

  /* Flags.  */
  TLV_DECODE_GETW (iad->flags);

  /* Total length of this message.  */
  TLV_DECODE_GETW (length);

  /* ID of this message.  */
  TLV_DECODE_GETL (iad->id);

  if (CHECK_FLAG (iad->flags, NSM_MSG_ILM_FAST_DELETE))
    {
      /* Delete.  */
      TLV_DECODE_GETL (iad->iif_ix);
      TLV_DECODE_GETL (iad->in_label);
      TLV_DECODE_GETL (iad->ilm_ix);
    }
  else
    {
      /* Decode owner. */
      ret = nsm_decode_mpls_owner_ipv6 (pnt, size, &iad->owner);
      if (ret < 0)
        return ret;

      /* Decode common part.  */
      TLV_DECODE_GETL (iad->iif_ix);
      TLV_DECODE_GETL (iad->in_label);
      TLV_DECODE_GETL (iad->ilm_ix);
#ifdef HAVE_DIFFSERV
      TLV_DECODE_GETC (iad->ds_info.lsp_type);
      TLV_DECODE_GET (iad->ds_info.dscp_exp_map, 8);
      TLV_DECODE_GETC (iad->ds_info.dscp);
      TLV_DECODE_GETC (iad->ds_info.af_set);
#endif /* HAVE_DIFFSERV */
      TLV_DECODE_GETL (iad->oif_ix);
      TLV_DECODE_GETL (iad->out_label);
      TLV_DECODE_GETC (iad->nh_addr.afi);

      /* based on address family encode the address */
      if (iad->nh_addr.afi == AFI_IP)
        TLV_DECODE_GET_IN4_ADDR (&iad->nh_addr.u.ipv4);
      else
        TLV_DECODE_GET_IN6_ADDR (&iad->nh_addr.u.ipv6);

      TLV_DECODE_GET (&iad->lspid, 6);
      TLV_DECODE_GETL (iad->n_pops);
      iad->fec_prefix.family = AF_INET6;
      TLV_DECODE_GETC (iad->fec_prefix.prefixlen);
      TLV_DECODE_GET_IN6_ADDR (&iad->fec_prefix.u.prefix6);
      TLV_DECODE_GETL (iad->qos_resrc_id);
      TLV_DECODE_GETC (iad->opcode);
      TLV_DECODE_GETC (iad->primary);
    }

  /* Calculate remaining length.  */
  length -= (*pnt - sp);

  /* TLV parse. */
  while (length)
    {
      if (length < NSM_TLV_HEADER_SIZE)
        return NSM_ERR_PKT_TOO_SMALL;

      diff = *size;

      NSM_DECODE_TLV_HEADER (tlv);

      switch (tlv.type)
        {
        default:
          TLV_DECODE_SKIP (tlv.length);
          break;
        }
      length -= (diff - *size);
    }

  return *pnt - sp;
}
#endif

/* Encode MPLS ILM return message.  */
s_int32_t
nsm_encode_ilm_ret_data (u_char **pnt, u_int16_t *size,
                         struct ilm_ret_data *ird)
{
  u_char *sp = *pnt;
  int ret;

  /* Encode flags.  */
  TLV_ENCODE_PUTW (ird->flags);

  /* Encode id. */
  TLV_ENCODE_PUTL (ird->id);

  /* Encode owner. */
#ifdef HAVE_IPV6
  ret = nsm_encode_mpls_owner_ipv6 (pnt, size, &ird->owner);
#else
  ret = nsm_encode_mpls_owner_ipv4 (pnt, size, &ird->owner);
#endif

  if (ret < 0)
    return ret;

  TLV_ENCODE_PUTL (ird->iif_ix);
  TLV_ENCODE_PUTL (ird->in_label);
  TLV_ENCODE_PUTL (ird->ilm_ix);
  TLV_ENCODE_PUTL (ird->xc_ix);
  TLV_ENCODE_PUTL (ird->nhlfe_ix);

  return *pnt - sp;
}

/* Decode MPLS ILM IPv4 return message.  */
s_int32_t
nsm_decode_ilm_ret_data (u_char **pnt, u_int16_t *size,
                         struct ilm_ret_data *ird)
{
  u_char *sp = *pnt;
  int ret;

  /* Decode flags.  */
  TLV_DECODE_GETW (ird->flags);

  /* Decode id. */
  TLV_DECODE_GETL (ird->id);

  /* Decode owner. */
#ifdef HAVE_IPV6
  ret = nsm_decode_mpls_owner_ipv6 (pnt, size, &ird->owner);
#else
  ret = nsm_decode_mpls_owner_ipv4 (pnt, size, &ird->owner);
#endif

  if (ret < 0)
    return ret;

  TLV_DECODE_GETL (ird->iif_ix);
  TLV_DECODE_GETL (ird->in_label);
  TLV_DECODE_GETL (ird->ilm_ix);
  TLV_DECODE_GETL (ird->xc_ix);
  TLV_DECODE_GETL (ird->nhlfe_ix);

  return *pnt - sp;
}

s_int32_t
nsm_encode_ilm_gen_ret_data (u_char **pnt, u_int16_t *size,
                             struct ilm_gen_ret_data *ird)
{
  u_char *sp = *pnt;
  int ret;

  /* Encode flags.  */
  TLV_ENCODE_PUTW (ird->flags);

  /* Encode id. */
  TLV_ENCODE_PUTL (ird->id);

  /* Encode owner. */
#ifdef HAVE_IPV6
  ret = nsm_encode_mpls_owner_ipv6 (pnt, size, &ird->owner);
#else
  ret = nsm_encode_mpls_owner_ipv4 (pnt, size, &ird->owner);
#endif

  if (ret < 0)
    return ret;

  TLV_ENCODE_PUTL (ird->iif_ix);
  nsm_encode_gmpls_label (pnt, size, &ird->in_label);
  TLV_ENCODE_PUTL (ird->ilm_ix);
  TLV_ENCODE_PUTL (ird->xc_ix);
  TLV_ENCODE_PUTL (ird->nhlfe_ix);

  return *pnt - sp;
}

/* Decode MPLS ILM IPv4 return message.  */
s_int32_t
nsm_decode_ilm_gen_ret_data (u_char **pnt, u_int16_t *size,
                             struct ilm_gen_ret_data *ird)
{
  u_char *sp = *pnt;
  int ret;

  /* Decode flags.  */
  TLV_DECODE_GETW (ird->flags);

  /* Decode id. */
  TLV_DECODE_GETL (ird->id);

  /* Decode owner. */
#ifdef HAVE_IPV6
  ret = nsm_decode_mpls_owner_ipv6 (pnt, size, &ird->owner);
#else
  ret = nsm_decode_mpls_owner_ipv4 (pnt, size, &ird->owner);
#endif

  if (ret < 0)
    return ret;

  TLV_DECODE_GETL (ird->iif_ix);
  nsm_decode_gmpls_label (pnt, size, &ird->in_label);
  TLV_DECODE_GETL (ird->ilm_ix);
  TLV_DECODE_GETL (ird->xc_ix);
  TLV_DECODE_GETL (ird->nhlfe_ix);

  return *pnt - sp;
}

#ifdef HAVE_GMPLS
s_int32_t
nsm_encode_ilm_bidir_ret_data (u_char **pnt, u_int16_t *size,
                               struct ilm_bidir_ret_data *ird)
{
  u_char *sp = *pnt;

  TLV_ENCODE_PUTL (ird->id);
  nsm_encode_ilm_gen_ret_data (pnt, size, &ird->ilm_fwd);
  nsm_encode_ilm_gen_ret_data (pnt, size, &ird->ilm_bwd);

  return *pnt - sp;
}

/* Decode MPLS ILM IPv4 return message.  */
s_int32_t
nsm_decode_ilm_bidir_ret_data (u_char **pnt, u_int16_t *size,
                               struct ilm_bidir_ret_data *ird)
{
  u_char *sp = *pnt;

  TLV_DECODE_GETL (ird->id);
  nsm_decode_ilm_gen_ret_data (pnt, size, &ird->ilm_fwd);
  nsm_decode_ilm_gen_ret_data (pnt, size, &ird->ilm_bwd);

  return *pnt - sp;
}

s_int32_t
nsm_encode_ftn_bidir_ret_data (u_char **pnt, u_int16_t *size,
                               struct ftn_bidir_ret_data *frd)
{
  u_char *sp = *pnt;

  TLV_ENCODE_PUTL (frd->id);
  nsm_encode_ftn_ret_data (pnt, size, &frd->ftn);
  nsm_encode_ilm_gen_ret_data (pnt, size, &frd->ilm);

  return *pnt - sp;
}

/* Decode MPLS ILM IPv4 return message.  */
s_int32_t
nsm_decode_ftn_bidir_ret_data (u_char **pnt, u_int16_t *size,
                               struct ftn_bidir_ret_data *frd)
{
  u_char *sp = *pnt;

  TLV_DECODE_GETL (frd->id);
  nsm_decode_ftn_ret_data (pnt, size, &frd->ftn);
  nsm_decode_ilm_gen_ret_data (pnt, size, &frd->ilm);

  return *pnt - sp;
}
#endif /* HAVE_GMPLS */

s_int32_t
nsm_encode_mpls_notification (u_char **pnt, u_int16_t *size,
                              struct mpls_notification *mn)
{
  u_char *sp = *pnt;
  int ret;

  /* Encode owner. */
#ifdef HAVE_IPV6
  ret = nsm_encode_mpls_owner_ipv6 (pnt, size, &mn->owner);
#else
  ret = nsm_encode_mpls_owner_ipv4 (pnt, size, &mn->owner);
#endif

  if (ret < 0)
    return ret;

  TLV_ENCODE_PUTC (mn->type);
  TLV_ENCODE_PUTC (mn->status);

  return *pnt - sp;
}

s_int32_t
nsm_decode_mpls_notification (u_char **pnt, u_int16_t *size,
                              struct mpls_notification *mn)
{
  u_char *sp = *pnt;
  int ret;

  /* Decode owner. */
#ifdef HAVE_IPV6
  ret = nsm_decode_mpls_owner_ipv6 (pnt, size, &mn->owner);
#else
  ret = nsm_decode_mpls_owner_ipv4 (pnt, size, &mn->owner);
#endif
  if (ret < 0)
    return ret;

  TLV_DECODE_GETC (mn->type);
  TLV_DECODE_GETC (mn->status);

  return *pnt - sp;
}

#if defined HAVE_MPLS_OAM || defined HAVE_NSM_MPLS_OAM
s_int32_t
nsm_encode_mpls_vpn_rd (u_char **pnt, u_int16_t *size,
                        struct nsm_msg_vpn_rd *msg)
{
  u_char *sp = *pnt;

  TLV_ENCODE_PUTC (msg->type);
  TLV_ENCODE_PUTL (msg->vr_id);
  TLV_ENCODE_PUTL (msg->vrf_id);

  TLV_ENCODE_PUT (&msg->rd, (sizeof (struct vpn_rd)));

  return *pnt - sp;
}

s_int32_t
nsm_decode_mpls_vpn_rd (u_char **pnt, u_int16_t *size,
                        struct nsm_msg_vpn_rd *msg)
{

  u_char *sp = *pnt;

  TLV_DECODE_GETC (msg->type);
  TLV_DECODE_GETL (msg->vr_id);
  TLV_DECODE_GETL (msg->vrf_id);

  TLV_DECODE_GET (&msg->rd, sizeof (struct vpn_rd));

  return *pnt - sp;
}
#endif /* HAVE_MPLS_OAM || HAVE_NSM_MPLS_OAM */
#endif /* HAVE_MPLS */

#ifdef HAVE_GMPLS
#if 0 /* TBD old code need remove */
s_int32_t
nsm_encode_gmpls_if (u_char **pnt, u_int16_t *size,
                     struct nsm_msg_gmpls_if *msg)
{
  u_char *sp = *pnt;
  int i, j;
  u_int32_t count;
  u_int32_t *val;

  /* Check size.  */
  if (*size < NSM_MSG_GMPLS_IF_SIZE)
    return NSM_ERR_PKT_TOO_SMALL;

  /* Ifindex.  */
  TLV_ENCODE_PUTL (msg->ifindex);

  /*  parse. */
  for (i = 0; i < NSM_CINDEX_SIZE; i++)
    {
      if (NSM_CHECK_CTYPE (msg->cindex, i))
        {
          if (*size < NSM_TLV_HEADER_SIZE)
            return NSM_ERR_PKT_TOO_SMALL;

          switch (i)
            {
            case NSM_GMPLS_LINK_IDENTIFIER:
              nsm_encode_tlv (pnt, size, i, 8);

              TLV_ENCODE_PUTL (msg->link_ids.local);
              TLV_ENCODE_PUTL (msg->link_ids.remote);
              break;
            case NSM_GMPLS_PROTECTION_TYPE:
              nsm_encode_tlv (pnt, size, i, 1);
              TLV_ENCODE_PUTC (msg->protection);
              break;
            case NSM_GMPLS_CAPABILITY_TYPE:
              nsm_encode_tlv (pnt, size, i, 4);
              TLV_ENCODE_PUTL (msg->capability);
              break;
            case NSM_GMPLS_ENCODING_TYPE:
              nsm_encode_tlv (pnt, size, i, 1);
              TLV_ENCODE_PUTC (msg->encoding);
              break;
            case NSM_GMPLS_MIN_LSP_BW:
              nsm_encode_tlv (pnt, size, i, 4);
              TLV_ENCODE_PUTF (msg->min_lsp_bw);
              break;
            case NSM_GMPLS_SDH_INDICATION:
              nsm_encode_tlv (pnt, size, i, 1);
              TLV_ENCODE_PUTC (msg->indication);
              break;
            case NSM_GMPLS_SHARED_RISK_LINK_GROUP:
              count = vector_count (msg->srlg);
              nsm_encode_tlv (pnt, size, i, 4 + count * sizeof (u_int32_t));
              TLV_ENCODE_PUTL (count);
              if (count)
                for (j = 0; j < count; j++)
                  if ((val = vector_slot (msg->srlg, j)))
                    TLV_ENCODE_PUTL ((u_int32_t) *val);
              break;
            }
        }
    }

  return *pnt - sp;
}

s_int32_t
nsm_decode_gmpls_if (u_char **pnt, u_int16_t *size,
                     struct nsm_msg_gmpls_if *msg)
{
  u_char *sp = *pnt;
  struct nsm_tlv_header tlv;
  int i;
  u_int32_t count;
  u_int32_t group;

  /* Check size.  */
  if (*size < NSM_MSG_GMPLS_IF_SIZE)
    return NSM_ERR_PKT_TOO_SMALL;

  /* Ifindex.  */
  TLV_DECODE_GETL (msg->ifindex);

  /* TLV parse.  */
  while (*size)
    {
      if (*size < NSM_TLV_HEADER_SIZE)
        return NSM_ERR_PKT_TOO_SMALL;

      NSM_DECODE_TLV_HEADER (tlv);

      switch (tlv.type)
        {
        case NSM_GMPLS_LINK_IDENTIFIER:
          TLV_DECODE_GETL (msg->link_ids.local);
          TLV_DECODE_GETL (msg->link_ids.remote);
          NSM_SET_CTYPE (msg->cindex, tlv.type);
          break;
        case NSM_GMPLS_PROTECTION_TYPE:
          TLV_DECODE_GETC (msg->protection);
          NSM_SET_CTYPE (msg->cindex, tlv.type);
          break;
        case NSM_GMPLS_CAPABILITY_TYPE:
          TLV_DECODE_GETL (msg->capability);
          NSM_SET_CTYPE (msg->cindex, tlv.type);
          break;
        case NSM_GMPLS_ENCODING_TYPE:
          TLV_DECODE_GETC (msg->encoding);
          NSM_SET_CTYPE (msg->cindex, tlv.type);
          break;
        case NSM_GMPLS_MIN_LSP_BW:
          TLV_DECODE_GETF (msg->min_lsp_bw);
          NSM_SET_CTYPE (msg->cindex, tlv.type);
          break;
        case NSM_GMPLS_SDH_INDICATION:
          TLV_DECODE_GETC (msg->indication);
          NSM_SET_CTYPE (msg->cindex, tlv.type);
          break;
        case NSM_GMPLS_SHARED_RISK_LINK_GROUP:
          TLV_DECODE_GETL (count);
          msg->srlg = gmpls_srlg_new (count);
          if (count)
            for (i = 0; i < count; i++)
              {
                TLV_DECODE_GETL (group);
                gmpls_srlg_add (msg->srlg, group);
              }
          NSM_SET_CTYPE (msg->cindex, tlv.type);
          break;
        default:
          TLV_DECODE_SKIP (tlv.length);
          break;
        }
    }
  return *pnt - sp;
}
#endif
#endif /* HAVE_GMPLS */

#ifdef HAVE_CRX
/* Encode route clean message. */
s_int32_t
nsm_encode_route_clean (u_char **pnt, u_int16_t *size,
                        struct nsm_msg_route_clean *msg)
{
  u_char *sp = *pnt;
  u_char reserved = 0;

  /* Check size.  */
  if (*size < NSM_MSG_ROUTE_CLEAN_SIZE)
    return NSM_ERR_PKT_TOO_SMALL;

  /* AFI. */
  TLV_ENCODE_PUTW (msg->afi);

  /* SAFI. */
  TLV_ENCODE_PUTC (msg->safi);

  /* Reserved. */
  TLV_ENCODE_PUTC (reserved);

  /* Route type mask. */
  TLV_ENCODE_PUTL (msg->route_type_mask);

  return *pnt - sp;
}

/* Decode route clean message. */
s_int32_t
nsm_decode_route_clean (u_char **pnt, u_int16_t *size,
                        struct nsm_msg_route_clean *msg)
{
  u_char *sp = *pnt;
  struct nsm_tlv_header tlv;
  u_char reserved;

  /* Check size. */
  if (*size < NSM_MSG_ROUTE_CLEAN_SIZE)
    return NSM_ERR_PKT_TOO_SMALL;

  /* AFI. */
  TLV_DECODE_GETW (msg->afi);

  /* SAFI. */
  TLV_DECODE_GETC (msg->safi);

  /* Reserved. */
  TLV_DECODE_GETC (reserved);

  /* Route type mask. */
  TLV_DECODE_GETL (msg->route_type_mask);

  return *pnt - sp;
}
/* Encode protocol restart message. */
s_int32_t
nsm_encode_protocol_restart (u_char **pnt, u_int16_t *size,
                             struct nsm_msg_protocol_restart *msg)
{
  u_char *sp = *pnt;

  /* Check size.  */
  if (*size < NSM_MSG_PROTOCOL_RESTART_SIZE)
    return NSM_ERR_PKT_TOO_SMALL;

  TLV_ENCODE_PUTL (msg->protocol_id);
  TLV_ENCODE_PUTL (msg->disconnect_time);

  return *pnt - sp;
}

/* Decode protocol restart message. */
s_int32_t
nsm_decode_protocol_restart (u_char **pnt, u_int16_t *size,
                             struct nsm_msg_protocol_restart *msg)
{
  u_char *sp = *pnt;

  /* Check size. */
  if (*size < NSM_MSG_PROTOCOL_RESTART_SIZE)
    return NSM_ERR_PKT_TOO_SMALL;

  TLV_DECODE_GETL (msg->protocol_id);
  TLV_DECODE_GETL (msg->disconnect_time);

  return *pnt - sp;
}
#endif /* HAVE_CRX */

#ifdef HAVE_LACPD
int
nsm_encode_lacp_aggregator_add (u_char **pnt, u_int16_t *size,
                                struct nsm_msg_lacp_agg_add *msg)
{
  u_char *sp = *pnt;
  int i;

  /* check size */
  if (*size <  LACP_AGG_NAME_LEN)
    return NSM_ERR_PKT_TOO_SMALL;

  /* aggregator name */
  TLV_ENCODE_PUT (msg->agg_name, LACP_AGG_NAME_LEN);

  /* aggregator mac */
  TLV_ENCODE_PUT (msg->mac_addr, ETHER_ADDR_LEN);

  /* aggregator type*/
  TLV_ENCODE_PUTW (msg->agg_type);

  /* Aggregator TLVs */
  for (i = 0; i < NSM_LACP_AGG_CINDEX_NUM; i++)
    {
      if (NSM_CHECK_CTYPE (msg->cindex, i))
        {
          if (*size < NSM_TLV_HEADER_SIZE)
            return NSM_ERR_PKT_TOO_SMALL;

          switch (i)
            {
            case NSM_LACP_AGG_CTYPE_PORTS_ADDED:
              nsm_encode_tlv (pnt, size, i,
                              msg->add_count * sizeof (u_int32_t));

              if (*size < msg->add_count * sizeof (u_int32_t))
                return NSM_ERR_PKT_TOO_SMALL;

              TLV_ENCODE_PUT (msg->ports_added,
                              msg->add_count * sizeof (u_int32_t));

              break;
            case NSM_LACP_AGG_CTYPE_PORTS_DELETED:
              nsm_encode_tlv (pnt, size, i,
                              msg->del_count * sizeof (u_int32_t));

              if (*size < msg->del_count * sizeof (u_int32_t))
                return NSM_ERR_PKT_TOO_SMALL;

              TLV_ENCODE_PUT (msg->ports_deleted,
                              msg->del_count * sizeof (u_int32_t));

              break;
            default:
              return LIB_ERR_INVALID_TLV;
            }
        }
    }

  return *pnt - sp;
}


s_int32_t
nsm_encode_lacp_aggregator_del (u_char **pnt, u_int16_t *size,
                                char *agg_name)
{
  /* check size */
  if (*size < LACP_AGG_NAME_LEN)
    return NSM_ERR_PKT_TOO_SMALL;

  /* aggregator name */
  TLV_ENCODE_PUT (agg_name, LACP_AGG_NAME_LEN);

  return LACP_AGG_NAME_LEN;
}


s_int32_t
nsm_decode_lacp_aggregator_add (u_char **pnt, u_int16_t *size,
                                struct nsm_msg_lacp_agg_add *msg)
{
  u_char *sp = *pnt;
  struct nsm_tlv_header tlv;

  /* check size */
  if (*size < LACP_AGG_NAME_LEN)
    {
      return NSM_ERR_PKT_TOO_SMALL;
    }

  pal_mem_set (msg, 0, sizeof (struct nsm_msg_lacp_agg_add));

  /* aggregator name */
  TLV_DECODE_GET (msg->agg_name, LACP_AGG_NAME_LEN);

  /* aggregator mac */
  TLV_DECODE_GET (msg->mac_addr, ETHER_ADDR_LEN);

  /*aggregator type*/
  TLV_DECODE_GETW (msg->agg_type);

  /* TLV parse.  */
  while (*size)
    {
      if (*size < NSM_TLV_HEADER_SIZE)
        return NSM_ERR_PKT_TOO_SMALL;

      NSM_DECODE_TLV_HEADER (tlv);

      switch (tlv.type)
        {
        case NSM_LACP_AGG_CTYPE_PORTS_ADDED:
          msg->add_count = tlv.length/sizeof (u_int32_t);

          msg->ports_added = XCALLOC (MTYPE_TMP, tlv.length);
          if (! msg->ports_added)
            {
              return NSM_ERR_SYSTEM_FAILURE;
            }

          TLV_DECODE_GET (msg->ports_added, tlv.length);
          NSM_SET_CTYPE (msg->cindex, tlv.type);
          break;
        case NSM_LACP_AGG_CTYPE_PORTS_DELETED:
          msg->del_count = tlv.length / sizeof (u_int32_t);

          msg->ports_deleted = XCALLOC (MTYPE_TMP, tlv.length);
          if (! msg->ports_deleted)
            return NSM_ERR_SYSTEM_FAILURE;

          TLV_DECODE_GET (msg->ports_deleted, tlv.length);
          NSM_SET_CTYPE (msg->cindex, tlv.type);
          break;
        default:
          TLV_DECODE_SKIP (tlv.length);
          break;
        }
    }

  return *pnt - sp;
}

s_int32_t
nsm_decode_lacp_aggregator_del (u_char **pnt, u_int16_t *size,
                                char *agg_name)
{
  /* Check size.  */
  if (*size < LACP_AGG_NAME_LEN)
    return NSM_ERR_PKT_TOO_SMALL;

  /* aggregator name */
  TLV_DECODE_GET (agg_name, LACP_AGG_NAME_LEN);

  return LACP_AGG_NAME_LEN;
}

/* Encode message for sending to LACP regarding the addition/deletion of a */
/* static aggregator in NSM - to update the aggrgator count in LACP*/
s_int32_t
nsm_encode_static_agg_cnt (u_char **pnt, u_int16_t *size,
                              struct nsm_msg_agg_cnt *msg)
{
  u_char *sp = *pnt;

  /* Check size. */
  if (*size < NSM_MSG_AGG_CNT_SIZE)
    return NSM_ERR_PKT_TOO_SMALL;

  /* Encode agg count. */
  TLV_ENCODE_PUTW (msg->agg_msg_type);

  return *pnt - sp;

}

/* Decode the message for LACP regarding the addition/deletion of a static */
/* aggregator in NSM - to update the aggregator count*/
s_int32_t
nsm_decode_static_agg_cnt (u_char **pnt, u_int16_t *size,
                              struct nsm_msg_agg_cnt *msg)
{
  u_char *sp = *pnt;

  /* Check size. */
  if (*size < NSM_MSG_AGG_CNT_SIZE)
     return NSM_ERR_PKT_TOO_SMALL;

  /* Aggregator count */
  TLV_DECODE_GETW (msg->agg_msg_type);

  return *pnt - sp;

}
/* Encode the message for LACP regarding the addition of aggregator in LACP*/
/* This has been introduced as result of configuring LACP aggregator from nsm */
/* We need to inform LACP about the configuring of LACP aggregator so that
   protocols takes care of creating aggregator*/
s_int32_t
nsm_encode_lacp_send_agg_config (u_char **pnt, u_int16_t *size,
                              struct nsm_msg_lacp_aggregator_config *msg)
{
  u_char *sp = *pnt;

  /* Check size. */
  if (*size < NSM_MSG_LACP_AGG_CONFIG_SIZE)
    return NSM_ERR_PKT_TOO_SMALL;

  /* Encode agg add message. */
  TLV_ENCODE_PUTL (msg->ifindex);
  TLV_ENCODE_PUTL (msg->key);
  TLV_ENCODE_PUTC (msg->chan_activate);
  TLV_ENCODE_PUTC (msg->add_agg);

  return *pnt - sp;
}

s_int32_t
nsm_decode_lacp_send_agg_config (u_char **pnt, u_int16_t *size,
                              struct nsm_msg_lacp_aggregator_config *msg)
{
  u_char *sp = *pnt;

  /* Check size. */
  if (*size < NSM_MSG_LACP_AGG_CONFIG_SIZE)
     return NSM_ERR_PKT_TOO_SMALL;

  /* Aggregator count */
  TLV_DECODE_GETL (msg->ifindex);
  TLV_DECODE_GETL (msg->key);
  TLV_DECODE_GETC (msg->chan_activate);
  TLV_DECODE_GETC (msg->add_agg);

  return *pnt - sp;

}
#endif /*HAVE_LACPD*/

#ifdef HAVE_L2
/* Encode bridge message. */
int
nsm_encode_bridge_msg (u_char **pnt, u_int16_t *size,
                       struct nsm_msg_bridge *msg)
{
  u_char *sp = *pnt;

  /* Check size. */
  if (*size < NSM_MSG_BRIDGE_SIZE)
    return NSM_ERR_PKT_TOO_SMALL;

  /* Encode bridgename. */
  TLV_ENCODE_PUT (msg->bridge_name, NSM_BRIDGE_NAMSIZ);

  /* Encode type. */
  TLV_ENCODE_PUTC (msg->type);

  /* Encode default flag. */
  TLV_ENCODE_PUTC (msg->is_default);

  /* Encode Topology type. */
  TLV_ENCODE_PUTC (msg->topology_type);

  /* Encode whether Provider Edge Bridge or not */
  TLV_ENCODE_PUTC (msg->is_edge);

  /* Encode type. */
  TLV_ENCODE_PUTL (msg->ageing_time);

  return *pnt - sp;
}

/* Decode bridge message. */
int
nsm_decode_bridge_msg (u_char **pnt, u_int16_t *size,
                       struct nsm_msg_bridge *msg)
{
  u_char *sp = *pnt;

  /* Check size. */
  if (*size < NSM_MSG_BRIDGE_SIZE)
     return NSM_ERR_PKT_TOO_SMALL;

  /* Bridge name. */
  TLV_DECODE_GET (msg->bridge_name, NSM_BRIDGE_NAMSIZ);

  /* Message type. */
  TLV_DECODE_GETC (msg->type);

  /* Default Bridge */
  TLV_DECODE_GETC (msg->is_default);

  /* Topology type */
  TLV_DECODE_GETC (msg->topology_type);

  /* Whether Provider Edge Bridge or not */
  TLV_DECODE_GETC (msg->is_edge);

  /* Ageing time.  */
  TLV_DECODE_GETL (msg->ageing_time);

  return *pnt - sp;
}

/* Encode bridge interface message. */
int
nsm_encode_bridge_if_msg (u_char **pnt, u_int16_t *size,
                          struct nsm_msg_bridge_if *msg)
{
  u_char *sp = *pnt;

  /* Check size. */
  if (*size < NSM_MSG_BRIDGE_IF_SIZE)
    return NSM_ERR_PKT_TOO_SMALL;

  /* Encode bridgename. */
  TLV_ENCODE_PUT (msg->bridge_name, NSM_BRIDGE_NAMSIZ);

#ifdef HAVE_PROVIDER_BRIDGE
  TLV_ENCODE_PUTC (msg->cust_bpdu_process);
#endif /* HAVE_PROVIDER_BRIDGE */

  TLV_ENCODE_PUTC (msg->spanning_tree_disable);

  if (*size < msg->num * sizeof (u_int32_t))
    return NSM_ERR_PKT_TOO_SMALL;

  if (*size < msg->num * sizeof (u_int32_t))
    return NSM_ERR_PKT_TOO_SMALL;

  /* Encode port ifindexes. */
  nsm_encode_tlv (pnt, size, NSM_MSG_BRIDGE_CTYPE_PORT,
                  msg->num * sizeof (u_int32_t));

  TLV_ENCODE_PUT (msg->ifindex,
                  msg->num * sizeof (u_int32_t));

  return *pnt - sp;
}

/* Decode bridge interface message. */
int
nsm_decode_bridge_if_msg (u_char **pnt, u_int16_t *size,
                          struct nsm_msg_bridge_if *msg)
{
  u_char *sp = *pnt;
  struct nsm_tlv_header tlv;

  /* Check size. */
  if (*size < NSM_MSG_BRIDGE_IF_SIZE)
     return NSM_ERR_PKT_TOO_SMALL;

  /* Bridge name. */
  TLV_DECODE_GET (msg->bridge_name, NSM_BRIDGE_NAMSIZ);

#ifdef HAVE_PROVIDER_BRIDGE
  TLV_DECODE_GETC (msg->cust_bpdu_process);
#endif /* HAVE_PROVIDER_BRIDGE */

  TLV_DECODE_GETC (msg->spanning_tree_disable);

  msg->num = 0;

  /* TLV parse.  */
  while (*size)
    {
      if (*size < NSM_TLV_HEADER_SIZE)
        return NSM_ERR_PKT_TOO_SMALL;

      NSM_DECODE_TLV_HEADER (tlv);

      switch (tlv.type)
        {
        case NSM_MSG_BRIDGE_CTYPE_PORT:
          msg->num = tlv.length/sizeof (u_int32_t);

          msg->ifindex = XCALLOC (MTYPE_TMP, tlv.length);
          if (! msg->ifindex)
            {
              return NSM_ERR_SYSTEM_FAILURE;
            }

          TLV_DECODE_GET (msg->ifindex, tlv.length);
          NSM_SET_CTYPE (msg->cindex, tlv.type);
          break;
        default:
          TLV_DECODE_SKIP (tlv.length);
          break;
        }
    }

  return *pnt - sp;
}

#if defined HAVE_G8031 || defined HAVE_G8032

/* Encode bridge Protection Group message. */
int
nsm_encode_bridge_pg_msg (u_char **pnt, u_int16_t *size,
                          struct nsm_msg_bridge_pg *msg)
{
  u_char *sp = *pnt;

  /* Check size. */
  if (*size < NSM_MSG_BRIDGE_IF_SIZE)
    return NSM_ERR_PKT_TOO_SMALL;

  /* Encode bridgename. */
  TLV_ENCODE_PUT (msg->bridge_name, NSM_BRIDGE_NAMSIZ);

  TLV_ENCODE_PUTW (msg->instance);

  TLV_ENCODE_PUTC (msg->spanning_tree_disable);

  if (*size < msg->num * sizeof (u_int32_t))
    return NSM_ERR_PKT_TOO_SMALL;

  if (*size < msg->num * sizeof (u_int32_t))
    return NSM_ERR_PKT_TOO_SMALL;

  /* Encode port ifindexes. */
  nsm_encode_tlv (pnt, size, NSM_MSG_BRIDGE_CTYPE_PORT,
                  msg->num * sizeof (u_int32_t));

  TLV_ENCODE_PUT (msg->ifindex,
                  msg->num * sizeof (u_int32_t));

  return *pnt - sp;
}

/* Decode bridge interface message. */
int
nsm_decode_bridge_pg_msg (u_char **pnt, u_int16_t *size,
                          struct nsm_msg_bridge_pg *msg)
{
  u_char *sp = *pnt;
  struct nsm_tlv_header tlv;

  /* Check size. */
  if (*size < NSM_MSG_BRIDGE_IF_SIZE)
     return NSM_ERR_PKT_TOO_SMALL;

  /* Bridge name. */
  TLV_DECODE_GET (msg->bridge_name, NSM_BRIDGE_NAMSIZ);

  TLV_DECODE_GETW (msg->instance);

  TLV_DECODE_GETC (msg->spanning_tree_disable);

  msg->num = 0;

  /* TLV parse.  */
  while (*size)
    {
      if (*size < NSM_TLV_HEADER_SIZE)
        return NSM_ERR_PKT_TOO_SMALL;

      NSM_DECODE_TLV_HEADER (tlv);

      switch (tlv.type)
        {
        case NSM_MSG_BRIDGE_CTYPE_PORT:
          msg->num = tlv.length/sizeof (u_int32_t);

          msg->ifindex = XCALLOC (MTYPE_TMP, tlv.length);
          if (! msg->ifindex)
            {
              return NSM_ERR_SYSTEM_FAILURE;
            }

          TLV_DECODE_GET (msg->ifindex, tlv.length);
          NSM_SET_CTYPE (msg->cindex, tlv.type);
          break;
        default:
          TLV_DECODE_SKIP (tlv.length);
          break;
        }
    }

  return *pnt - sp;
}
#endif /* HAVE_G8031 || HAVE_G8032 */

/* Encode bridge Port message. */
s_int32_t
nsm_encode_bridge_port_msg (u_char **pnt, u_int16_t *size,
                          struct nsm_msg_bridge_port *msg)
{
  u_char *sp = *pnt;

  /* Check size. */
  if (*size < NSM_MSG_BRIDGE_SIZE)
    return NSM_ERR_PKT_TOO_SMALL;

  /* Encode bridgename. */
  TLV_ENCODE_PUT (msg->bridge_name, NSM_MSG_BRIDGE_SIZE);

  /* Encode ifindex */
  TLV_ENCODE_PUTL (msg->ifindex);

  if ((msg->num > 0) && (msg->instance) && (msg->state))
    {
      if (*size <  2* msg->num * sizeof (u_int32_t))
        return NSM_ERR_PKT_TOO_SMALL;

      /* Encode port instances. */
      nsm_encode_tlv (pnt, size, NSM_BRIDGE_CTYPE_PORT_INSTANCE,
                      msg->num * sizeof (u_int32_t));
      TLV_ENCODE_PUT (msg->instance, (msg->num * sizeof (u_int32_t)));
      /* Encode port states. */
      nsm_encode_tlv (pnt, size, NSM_BRIDGE_CTYPE_PORT_STATE,
                      msg->num * sizeof (u_int32_t));
      TLV_ENCODE_PUT (msg->state, (msg->num * sizeof (u_int32_t)));

    }
  return *pnt - sp;
}

/* Decode bridge port message. */
s_int32_t
nsm_decode_bridge_port_msg (u_char **pnt, u_int16_t *size,
                           struct nsm_msg_bridge_port *msg)
{
  u_char *sp = *pnt;
  struct nsm_tlv_header tlv;

  /* Check size. */
  if (*size < NSM_MSG_BRIDGE_PORT_SIZE)
     return NSM_ERR_PKT_TOO_SMALL;

  /* Bridge name. */
  TLV_DECODE_GET(msg->bridge_name, NSM_MSG_BRIDGE_SIZE);

  /* ifindex */
  TLV_DECODE_GETL (msg->ifindex);

  msg->num = 0;

  /* TLV parse.  */
  while (*size)
    {
      if (*size < NSM_TLV_HEADER_SIZE)
        return NSM_ERR_PKT_TOO_SMALL;

      NSM_DECODE_TLV_HEADER (tlv);

      switch (tlv.type)
        {
          case NSM_BRIDGE_CTYPE_PORT_INSTANCE:
            msg->num = tlv.length/sizeof (u_int32_t);

            msg->instance = XCALLOC (MTYPE_TMP, tlv.length);
            if (! msg->instance)
              {
                return NSM_ERR_SYSTEM_FAILURE;
              }
            TLV_DECODE_GET (msg->instance, tlv.length);
            NSM_SET_CTYPE (msg->cindex, NSM_BRIDGE_CTYPE_PORT_INSTANCE);
           break;
          case NSM_BRIDGE_CTYPE_PORT_STATE:
            msg->num = tlv.length/sizeof (u_int32_t);
            msg->state = XCALLOC (MTYPE_TMP, tlv.length);
            if (!msg->state)
              {
                return NSM_ERR_SYSTEM_FAILURE;
              }
            TLV_DECODE_GET (msg->state, tlv.length);
            NSM_SET_CTYPE (msg->cindex, NSM_BRIDGE_CTYPE_PORT_STATE);
           break;
          default:
            TLV_DECODE_SKIP (tlv.length);
           break;
        }
    }

  return *pnt - sp;
}

#ifdef HAVE_PBB_TE
s_int32_t
nsm_encode_bridge_pbb_te_port_msg (u_char **pnt, u_int16_t *size,
                          struct nsm_msg_bridge_pbb_te_port *msg)
{
  u_char *sp = *pnt;

  /* Encode eps group id */
  TLV_ENCODE_PUTL (msg->eps_grp_id);

  /* Encode tesid */
  TLV_ENCODE_PUTL (msg->te_sid_a);

  return *pnt - sp;
}

s_int32_t
nsm_decode_bridge_pbb_te_port_msg (u_char **pnt, u_int16_t *size,
                           struct nsm_msg_bridge_pbb_te_port *msg)
{
  u_char *sp = *pnt;

  /* deocode eps id */
  TLV_DECODE_GETL(msg->eps_grp_id);

  /* decode tesid */
  TLV_DECODE_GETL(msg->te_sid_a);

  return *pnt - sp;
}

#endif /* HAVE_PBB_TE */



/* Encode bridge Enable message. */
s_int32_t
nsm_encode_bridge_enable_msg (u_char **pnt, u_int16_t *size,
                              struct nsm_msg_bridge_enable *msg)
{
  u_char *sp = *pnt;

  /* Check size. */
  if (*size < NSM_MSG_BRIDGE_SIZE)
    return NSM_ERR_PKT_TOO_SMALL;

  /* Encode bridgename. */
  TLV_ENCODE_PUT (msg->bridge_name, NSM_MSG_BRIDGE_SIZE);

  /* Encode Enable */
  TLV_ENCODE_PUTW (msg->enable);

  return *pnt - sp;
}

/* Decode bridge Enable message. */
s_int32_t
nsm_decode_bridge_enable_msg (u_char **pnt, u_int16_t *size,
                              struct nsm_msg_bridge_enable *msg)
{
  u_char *sp = *pnt;

  /* Check size. */
  if (*size < NSM_MSG_BRIDGE_SIZE)
     return NSM_ERR_PKT_TOO_SMALL;

  /* Bridge name. */
  TLV_DECODE_GET(msg->bridge_name, NSM_MSG_BRIDGE_SIZE);

  /* Enable */
  TLV_DECODE_GETW (msg->enable);

  return *pnt - sp;
}

#ifdef HAVE_ONMD
int
nsm_encode_cfm_mac_msg (u_char **pnt,
                        u_int16_t *size,
                        struct nsm_msg_cfm_mac *msg)
{
  u_char *sp = *pnt;

  TLV_ENCODE_PUT (msg->name, NSM_BRIDGE_NAMSIZ);

  TLV_ENCODE_PUT (msg->mac, ETHER_ADDR_LEN);

  TLV_ENCODE_PUTW (msg->vid);

  TLV_ENCODE_PUTW (msg->svid);

  return *pnt - sp;
}

int
nsm_decode_cfm_mac_msg (u_char **pnt,
                        u_int16_t *size,
                        struct nsm_msg_cfm_mac *msg)
{
  u_char *sp = *pnt;

  TLV_DECODE_GET (msg->name, NSM_BRIDGE_NAMSIZ);

  TLV_DECODE_GET (msg->mac, ETHER_ADDR_LEN);

  TLV_DECODE_GETW (msg->vid);

  TLV_DECODE_GETW (msg->svid);

  return *pnt - sp;
}

s_int32_t
nsm_decode_cfm_index (u_char **pnt,
                      u_int16_t *size,
                      struct nsm_msg_cfm_ifindex *msg)
{
  u_char *sp = *pnt;

  /* Get the ifindex */
  TLV_DECODE_GETL (msg->index);

  return *pnt - sp;

}

s_int32_t
nsm_encode_cfm_index (u_char **pnt,
                      u_int16_t *size,
                      struct nsm_msg_cfm_ifindex *msg)
{
  u_char *sp = *pnt;

  TLV_ENCODE_PUTL (msg->index);

  return *pnt - sp;
}
#endif /* HAVE_ONMD */

#endif /* HAVE_L2 */

#ifdef HAVE_RMOND
/* In nsm_message.c */
s_int32_t
nsm_encode_rmon_msg (u_char **pnt,
                     u_int16_t *size,
                     struct nsm_msg_rmon *msg)
{
  u_char *sp = *pnt;

  /* Check size.  */
  if (*size < NSM_MSG_RMON_SIZE)
    return NSM_ERR_PKT_TOO_SMALL;

  /* Put ifindex */
  TLV_ENCODE_PUTL (msg->ifindex);

  return *pnt - sp;
}

s_int32_t
nsm_decode_rmon_msg (u_char **pnt,
                     u_int16_t *size,
                     struct nsm_msg_rmon *msg)
{
  u_char *sp = *pnt;

  /* Check size.  */
  if (*size < NSM_MSG_RMON_SIZE)
    return NSM_ERR_PKT_TOO_SMALL;

  /* Get the ifindex */
  TLV_DECODE_GETL (msg->ifindex);

  return *pnt - sp;

}
s_int32_t
nsm_encode_rmon_stats (u_char **pnt, u_int16_t *size,
                       struct nsm_msg_rmon_stats *msg)
{
  u_char *sp = *pnt;

  /* Check size.  */
  if (*size < NSM_MSG_RMON_STATS_SIZE)
    return NSM_ERR_PKT_TOO_SMALL;

  TLV_ENCODE_PUTL (msg->ifindex);

  TLV_ENCODE_PUT (&msg->good_octets_rcv, sizeof (ut_int64_t));
  TLV_ENCODE_PUT (&msg->bad_octets_rcv, sizeof (ut_int64_t));
  TLV_ENCODE_PUT (&msg->mac_transmit_err, sizeof (ut_int64_t));
  TLV_ENCODE_PUT (&msg->good_pkts_rcv, sizeof (ut_int64_t));
  TLV_ENCODE_PUT (&msg->bad_pkts_rcv, sizeof (ut_int64_t));
  TLV_ENCODE_PUT (&msg->brdc_pkts_rcv, sizeof (ut_int64_t));
  TLV_ENCODE_PUT (&msg->mc_pkts_rcv, sizeof (ut_int64_t));
  TLV_ENCODE_PUT (&msg->pkts_64_octets, sizeof (ut_int64_t));
  TLV_ENCODE_PUT (&msg->pkts_65_127_octets, sizeof (ut_int64_t));
  TLV_ENCODE_PUT (&msg->pkts_128_255_octets, sizeof (ut_int64_t));
  TLV_ENCODE_PUT (&msg->pkts_256_511_octets, sizeof (ut_int64_t));
  TLV_ENCODE_PUT (&msg->pkts_512_1023_octets, sizeof (ut_int64_t));
  TLV_ENCODE_PUT (&msg->pkts_1024_max_octets, sizeof (ut_int64_t));
  TLV_ENCODE_PUT (&msg->good_pkts_sent, sizeof (ut_int64_t));
  TLV_ENCODE_PUT (&msg->excessive_collisions, sizeof (ut_int64_t));
  TLV_ENCODE_PUT (&msg->mc_pkts_sent, sizeof (ut_int64_t));
  TLV_ENCODE_PUT (&msg->brdc_pkts_sent, sizeof (ut_int64_t));
  TLV_ENCODE_PUT (&msg->unrecog_mac_cntr_rcv, sizeof (ut_int64_t));
  TLV_ENCODE_PUT (&msg->fc_sent, sizeof (ut_int64_t));
  TLV_ENCODE_PUT (&msg->good_fc_rcv, sizeof (ut_int64_t));
  TLV_ENCODE_PUT (&msg->drop_events, sizeof (ut_int64_t));
  TLV_ENCODE_PUT (&msg->undersize_pkts, sizeof (ut_int64_t));
  TLV_ENCODE_PUT (&msg->fragments_pkts, sizeof (ut_int64_t));
  TLV_ENCODE_PUT (&msg->oversize_pkts, sizeof (ut_int64_t));
  TLV_ENCODE_PUT (&msg->jabber_pkts, sizeof (ut_int64_t));
  TLV_ENCODE_PUT (&msg->mac_rcv_error, sizeof (ut_int64_t));
  TLV_ENCODE_PUT (&msg->bad_crc, sizeof (ut_int64_t));
  TLV_ENCODE_PUT (&msg->collisions, sizeof (ut_int64_t));
  TLV_ENCODE_PUT (&msg->late_collisions, sizeof (ut_int64_t));
  TLV_ENCODE_PUT (&msg->bad_fc_rcv, sizeof (ut_int64_t));

  return *pnt - sp;
}

s_int32_t
nsm_decode_rmon_stats (u_char **pnt, u_int16_t *size,
                       struct nsm_msg_rmon_stats *msg)
{
  u_char *sp = *pnt;

  /* Check size.  */
  if (*size < NSM_MSG_RMON_STATS_SIZE)
    return NSM_ERR_PKT_TOO_SMALL;

  TLV_DECODE_GETL (msg->ifindex);

  TLV_DECODE_GET (&msg->good_octets_rcv, sizeof (ut_int64_t));
  TLV_DECODE_GET (&msg->bad_octets_rcv, sizeof (ut_int64_t));
  TLV_DECODE_GET (&msg->mac_transmit_err, sizeof (ut_int64_t));
  TLV_DECODE_GET (&msg->good_pkts_rcv, sizeof (ut_int64_t));
  TLV_DECODE_GET (&msg->bad_pkts_rcv, sizeof (ut_int64_t));
  TLV_DECODE_GET (&msg->brdc_pkts_rcv, sizeof (ut_int64_t));
  TLV_DECODE_GET (&msg->mc_pkts_rcv, sizeof (ut_int64_t));
  TLV_DECODE_GET (&msg->pkts_64_octets, sizeof (ut_int64_t));
  TLV_DECODE_GET (&msg->pkts_65_127_octets, sizeof (ut_int64_t));
  TLV_DECODE_GET (&msg->pkts_128_255_octets, sizeof (ut_int64_t));
  TLV_DECODE_GET (&msg->pkts_256_511_octets, sizeof (ut_int64_t));
  TLV_DECODE_GET (&msg->pkts_512_1023_octets, sizeof (ut_int64_t));
  TLV_DECODE_GET (&msg->pkts_1024_max_octets, sizeof (ut_int64_t));
  TLV_DECODE_GET (&msg->good_pkts_sent, sizeof (ut_int64_t));
  TLV_DECODE_GET (&msg->excessive_collisions, sizeof (ut_int64_t));
  TLV_DECODE_GET (&msg->mc_pkts_sent, sizeof (ut_int64_t));
  TLV_DECODE_GET (&msg->brdc_pkts_sent, sizeof (ut_int64_t));
  TLV_DECODE_GET (&msg->unrecog_mac_cntr_rcv, sizeof (ut_int64_t));
  TLV_DECODE_GET (&msg->fc_sent, sizeof (ut_int64_t));
  TLV_DECODE_GET (&msg->good_fc_rcv, sizeof (ut_int64_t));
  TLV_DECODE_GET (&msg->drop_events, sizeof (ut_int64_t));
  TLV_DECODE_GET (&msg->undersize_pkts, sizeof (ut_int64_t));
  TLV_DECODE_GET (&msg->fragments_pkts, sizeof (ut_int64_t));
  TLV_DECODE_GET (&msg->oversize_pkts, sizeof (ut_int64_t));
  TLV_DECODE_GET (&msg->jabber_pkts, sizeof (ut_int64_t));
  TLV_DECODE_GET (&msg->mac_rcv_error, sizeof (ut_int64_t));
  TLV_DECODE_GET (&msg->bad_crc, sizeof (ut_int64_t));
  TLV_DECODE_GET (&msg->collisions, sizeof (ut_int64_t));
  TLV_DECODE_GET (&msg->late_collisions, sizeof (ut_int64_t));
  TLV_DECODE_GET (&msg->bad_fc_rcv, sizeof (ut_int64_t));


  return *pnt - sp;
}

#endif /* HAVE_RMON */

#ifndef HAVE_CUSTOM1
#ifdef HAVE_VLAN
/* Encode VLAN message. */
int
nsm_encode_vlan_msg (u_char **pnt, u_int16_t *size,
                     struct nsm_msg_vlan *msg)
{
  u_char *sp = *pnt;
  int i;

  /* Check size. */
  if (*size < NSM_MSG_VLAN_SIZE)
     return NSM_ERR_PKT_TOO_SMALL;

  /* Put flags. */
  TLV_ENCODE_PUTW (msg->flags);

  /* Put VID. */
  TLV_ENCODE_PUTW (msg->vid);

  for (i = 0; i < NSM_VLAN_CINDEX_NUM; i++)
    {
      if (NSM_CHECK_CTYPE (msg->cindex, i))
        {
          if (*size < NSM_TLV_HEADER_SIZE)
            return NSM_ERR_PKT_TOO_SMALL;

          switch (i)
            {
            case NSM_VLAN_CTYPE_NAME:
            nsm_encode_tlv (pnt, size, i, NSM_VLAN_NAMSIZ);
            TLV_ENCODE_PUT (msg->vlan_name, NSM_VLAN_NAMSIZ);
            break;
            case NSM_VLAN_CTYPE_BRIDGENAME:
            nsm_encode_tlv (pnt, size, i, NSM_BRIDGE_NAMSIZ);
            TLV_ENCODE_PUT (msg->bridge_name, NSM_BRIDGE_NAMSIZ);
            break;
            case NSM_VLAN_CTYPE_INSTANCE:
            nsm_encode_tlv (pnt, size, i, 2);
            TLV_ENCODE_PUTW (msg->br_instance);
            break;
            default:
              return LIB_ERR_INVALID_TLV;
            }
        }
     }

  return *pnt - sp;
}

/* Decode VLAN message. */
s_int32_t
nsm_decode_vlan_msg (u_char **pnt, u_int16_t *size,
                     struct nsm_msg_vlan *msg)
{
  struct nsm_tlv_header tlv;
  u_char *sp = *pnt;

  /* Check size. */
  if (*size < NSM_MSG_VLAN_SIZE)
     return NSM_ERR_PKT_TOO_SMALL;

  /* Flags. */
  TLV_DECODE_GETW (msg->flags);

  /* VID. */
  TLV_DECODE_GETW (msg->vid);

  while (*size)
   {
     if (*size < NSM_TLV_HEADER_SIZE)
       return NSM_ERR_PKT_TOO_SMALL;

     NSM_DECODE_TLV_HEADER (tlv);

     switch (tlv.type)
      {
        case NSM_VLAN_CTYPE_NAME:
        TLV_DECODE_GET (msg->vlan_name, tlv.length);
        NSM_SET_CTYPE (msg->cindex, tlv.type);
        break;
        case NSM_VLAN_CTYPE_BRIDGENAME:
        TLV_DECODE_GET (msg->bridge_name, tlv.length);
        NSM_SET_CTYPE (msg->cindex, tlv.type);
        break;
        case NSM_VLAN_CTYPE_INSTANCE:
        TLV_DECODE_GETW (msg->br_instance);
        NSM_SET_CTYPE (msg->cindex, tlv.type);
        break;
        default:
          TLV_DECODE_SKIP (tlv.length);
          break;
      }
   }
 return *pnt - sp;
}

/* Encode VLAN port message. */
int
nsm_encode_vlan_port_msg (u_char **pnt, u_int16_t *size,
                        struct nsm_msg_vlan_port *msg)
{
  u_char *sp = *pnt;

  /* Check size. */
  if (*size < NSM_MSG_VLAN_PORT_SIZE)
    return NSM_ERR_PKT_TOO_SMALL;

  /* Encode ifindex. */
  TLV_ENCODE_PUTL (msg->ifindex);

  if (*size < (msg->num * sizeof (u_int16_t)))
    return NSM_ERR_PKT_TOO_SMALL;

  /* Encode port ifindexes. */
  nsm_encode_tlv (pnt, size, NSM_VLAN_PORT_CTYPE_VID_INFO,
                  msg->num * sizeof (u_int16_t));

  TLV_ENCODE_PUT (msg->vid_info,
                  msg->num * sizeof (u_int16_t));

  return *pnt - sp;
}

/* Decode VLAN port message. */
s_int32_t
nsm_decode_vlan_port_msg (u_char **pnt, u_int16_t *size,
                        struct nsm_msg_vlan_port *msg)
{
  struct nsm_tlv_header tlv;
  u_char *sp = *pnt;

  /* Check size. */
  if (*size < NSM_MSG_VLAN_PORT_SIZE)
     return NSM_ERR_PKT_TOO_SMALL;

  /* Decode ifindex. */
  TLV_DECODE_GETL (msg->ifindex);

  msg->num = 0;

  /* TLV parse.  */
  while (*size)
    {
      if (*size < NSM_TLV_HEADER_SIZE)
        return NSM_ERR_PKT_TOO_SMALL;

      NSM_DECODE_TLV_HEADER (tlv);

      switch (tlv.type)
        {
        case NSM_VLAN_PORT_CTYPE_VID_INFO:
          msg->num = tlv.length/sizeof (u_int16_t);

          msg->vid_info = XCALLOC (MTYPE_TMP, tlv.length);
          if (! msg->ifindex)
            {
              return NSM_ERR_SYSTEM_FAILURE;
            }

          TLV_DECODE_GET (msg->vid_info, tlv.length);
          NSM_SET_CTYPE (msg->cindex, tlv.type);
          break;
        default:
          TLV_DECODE_SKIP (tlv.length);
          break;
        }
    }

  return *pnt - sp;
}

/* Encode port type message. */
s_int32_t
nsm_encode_vlan_port_type_msg (u_char **pnt, u_int16_t *size,
                               struct nsm_msg_vlan_port_type *msg)
{
  u_char *sp = *pnt;

  /* Check size. */
  if (*size < NSM_MSG_VLAN_PORT_TYPE_SIZE)
    return NSM_ERR_PKT_TOO_SMALL;

  /* Encode ifindex. */
  TLV_ENCODE_PUTL (msg->ifindex);

  /* Encode port type. */
  TLV_ENCODE_PUTC (msg->port_type);

  /* Encode ingress filter. */
  TLV_ENCODE_PUTC (msg->ingress_filter);

  /* Encode acceptable frame type. */
  TLV_ENCODE_PUTC (msg->acceptable_frame_type);

  return *pnt - sp;
}

/* Decode port type message. */
s_int32_t
nsm_decode_vlan_port_type_msg (u_char **pnt, u_int16_t *size,
                               struct nsm_msg_vlan_port_type *msg)
{
  u_char *sp = *pnt;

  /* Check size. */
  if (*size < NSM_MSG_VLAN_PORT_TYPE_SIZE)
    return NSM_ERR_PKT_TOO_SMALL;

  /* Encode ifindex. */
  TLV_DECODE_GETL (msg->ifindex);

  /* Encode port type. */
  TLV_DECODE_GETC (msg->port_type);

  /* Encode ingress filter. */
  TLV_DECODE_GETC (msg->ingress_filter);

  /* Encode acceptable frame type. */
  TLV_DECODE_GETC (msg->acceptable_frame_type);

  return *pnt - sp;
}

#if (defined HAVE_I_BEB || defined HAVE_B_BEB )

/* Encode isid2svid msg */
s_int32_t
nsm_encode_isid2svid_msg ( u_char **pnt, u_int16_t *size,
    struct nsm_msg_pbb_isid_to_pip * msg)
{

    u_char *sp = *pnt;

    /* Encode bridge_name */
    TLV_ENCODE_PUT (msg->bridge_name, NSM_BRIDGE_NAMSIZ);

    /* Encode Type */
    TLV_ENCODE_PUTW (msg->type);

    /* Encode cnp_ifindex */
    TLV_ENCODE_PUTL (msg->cnp_ifindex);

    /* Encode pip_ifindex */
    TLV_ENCODE_PUTL (msg->pip_ifindex);

    /* Encode cbp_ifindex */
    TLV_ENCODE_PUTL (msg->cbp_ifindex);

    /* Encode svid_low */
    TLV_ENCODE_PUTW (msg->svid_low);

    /* Encode svid_high */
    TLV_ENCODE_PUTW (msg->svid_high);

    /* Enocode isid */
    TLV_ENCODE_PUTL(msg->isid);

    /* Encode dispatch_status */
    TLV_ENCODE_PUTW(msg->dispatch_status);

    return *pnt - sp;
}

/* Decode port type message. */
s_int32_t
nsm_decode_isid2svid_msg (u_char **pnt, u_int16_t *size,
    struct nsm_msg_pbb_isid_to_pip *msg)
{
  u_char *sp = *pnt;

  /* decoding bridge_name */
  TLV_DECODE_GET(msg->bridge_name, NSM_BRIDGE_NAMSIZ);

  /* decode type */
  TLV_DECODE_GETW(msg->type);

  /* decode cnp_ifindex */
  TLV_DECODE_GETL(msg->cnp_ifindex);

  /* decode pip_ifindex */
  TLV_DECODE_GETL(msg->pip_ifindex);

  /* decode cbp_ifindex */
  TLV_DECODE_GETL(msg->cbp_ifindex);

  /* decode svid_low */
  TLV_DECODE_GETW(msg->svid_low);

  /* decode svid_high */
  TLV_DECODE_GETW(msg->svid_high);

  /* decode isid */
  TLV_DECODE_GETL(msg->isid);

  /* decode dispatch_status */
  TLV_DECODE_GETW(msg->dispatch_status);

  return *pnt - sp;
}

/* Encode isid2bvid msg */
s_int32_t
nsm_encode_isid2bvid_msg ( u_char **pnt, u_int16_t *size,
                               struct nsm_msg_pbb_isid_to_bvid * msg)
{
  u_char *sp = *pnt;

  /* Encode bridge_name */
  TLV_ENCODE_PUT (msg->bridge_name, NSM_BRIDGE_NAMSIZ);

  /* Encode cbp_ifindex */
  TLV_ENCODE_PUTL (msg->cbp_ifindex);

  /* Enocode isid */
  TLV_ENCODE_PUTL(msg->isid);

  /* Encode bvid */
  TLV_ENCODE_PUTW(msg->bvid);

  /* Encode dispatch_status */
  TLV_ENCODE_PUTW(msg->dispatch_status);

  return *pnt - sp;
}

/* decode isid2bvid msg */
s_int32_t
nsm_decode_isid2bvid_msg ( u_char **pnt, u_int16_t *size,
                               struct nsm_msg_pbb_isid_to_bvid * msg)
{
  u_char *sp = *pnt;

  /* decode bridge_name */
  TLV_DECODE_GET(msg->bridge_name, NSM_BRIDGE_NAMSIZ);

  /* decode cbp_ifindex */
  TLV_DECODE_GETL(msg->cbp_ifindex);

  /* deocode isid */
  TLV_DECODE_GETL(msg->isid);

  /* decode bvid */
  TLV_DECODE_GETW(msg->bvid);

  /* decode dispatch_status */
  TLV_DECODE_GETW(msg->dispatch_status);

  return *pnt - sp;
}

#endif /* if (defined HAVE_I_BEB || defined HAVE_B_BEB ) */

#ifdef HAVE_G8031

/* Encode Protection Group msg */
s_int32_t
nsm_encode_g8031_create_pg_msg ( u_char **pnt, u_int16_t *size,
                                 struct nsm_msg_g8031_pg * msg)
{
  u_char *sp = *pnt;

  /* Encode bridge_name */
  TLV_ENCODE_PUT (msg->bridge_name, NSM_BRIDGE_NAMSIZ);

  /* Encode EPS-ID */
  TLV_ENCODE_PUTW (msg->eps_id);

  /* Encode working_ifindex */
  TLV_ENCODE_PUTL (msg->working_ifindex);

  /* Encode working_ifindex */
  TLV_ENCODE_PUTL (msg->protected_ifindex);

  return *pnt - sp;
}

 /* Decode Protection Group msg */
s_int32_t
nsm_decode_g8031_create_pg_msg ( u_char **pnt, u_int16_t *size,
                                 struct nsm_msg_g8031_pg * msg)
{
  u_char *sp = *pnt;

  /* decode bridge_name */
  TLV_DECODE_GET(msg->bridge_name, NSM_BRIDGE_NAMSIZ);

  /* deocode eps_id */
  TLV_DECODE_GETW(msg->eps_id);

  /* Encode working_ifindex */
  TLV_DECODE_GETL (msg->working_ifindex);

  /* Encode working_ifindex */
  TLV_DECODE_GETL (msg->protected_ifindex);

  return *pnt - sp;
}

/* Encode Vlan Group msg */
s_int32_t
nsm_encode_g8031_create_vlan_msg ( u_char **pnt, u_int16_t *size,
                                 struct nsm_msg_g8031_vlanpg * msg)
{
  u_char *sp = *pnt;

  /* Encode PVID */
  TLV_ENCODE_PUTW (msg->pvid);

  /* Encode bridge_name */
  TLV_ENCODE_PUT (msg->bridge_name, NSM_BRIDGE_NAMSIZ);

  /* Encode EPS-ID */
  TLV_ENCODE_PUTW (msg->eps_id);

  /* Encode VID */
  TLV_ENCODE_PUTW (msg->vid);

  return *pnt - sp;
}

/* Decode Vlan Group msg */
s_int32_t
nsm_decode_g8031_create_vlan_msg ( u_char **pnt, u_int16_t *size,
                                   struct nsm_msg_g8031_vlanpg * msg)
{
  u_char *sp = *pnt;

  /* deocode EPS-ID */
  TLV_DECODE_GETW(msg->pvid);

  /* decode bridge_name */
  TLV_DECODE_GET(msg->bridge_name, NSM_BRIDGE_NAMSIZ);

  /* deocode EPS-ID */
  TLV_DECODE_GETW(msg->eps_id);

  /* deocode VID */
  TLV_DECODE_GETW(msg->vid);

  return *pnt - sp;
}

/* Encode Protection group initialize msg */
s_int32_t
nsm_encode_pg_init_msg (u_char **pnt, u_int16_t *size,
                        struct nsm_msg_pg_initialized *msg)
{
  u_char *sp = *pnt;

  /* Encode EPS-ID */
  TLV_ENCODE_PUTW (msg->eps_id);

  /* Encode bridge_name */
  TLV_ENCODE_PUT (msg->bridge_name, NSM_BRIDGE_NAMSIZ);

  /* encode pg state */
  TLV_ENCODE_PUTW (msg->g8031_pg_state);

  return *pnt - sp;
}

/* Decode PG Initialize msg */
s_int32_t
nsm_decode_pg_init_msg ( u_char **pnt, u_int16_t *size,
                         struct nsm_msg_pg_initialized * msg)
{
  u_char *sp = *pnt;

  /* deocode EPS-ID */
  TLV_DECODE_GETW(msg->eps_id);

  /* decode bridge_name */
  TLV_DECODE_GET(msg->bridge_name, NSM_BRIDGE_NAMSIZ);

  /* deocode pg state */
  TLV_DECODE_GETW(msg->g8031_pg_state);

  return *pnt - sp;
}

/* Encode Protection group switchover msg */
s_int32_t
nsm_encode_g8031_portstate_msg (u_char **pnt, u_int16_t *size,
                              struct nsm_msg_g8031_portstate *msg)
{
  u_char *sp = *pnt;

  /* Encode EPS-ID */
   TLV_ENCODE_PUTW (msg->eps_id);

  /* Encode bridge_name */
  TLV_ENCODE_PUT (msg->bridge_name, NSM_BRIDGE_NAMSIZ);

  /* Encode Ifindex to be fwd state */
  TLV_ENCODE_PUTW (msg->ifindex_fwd);

  /* encode pg  portstate */
  TLV_ENCODE_PUTW (msg->state_f);

  /* Encode Ifindex to be blocking state */
  TLV_ENCODE_PUTW (msg->ifindex_blck);

  /* encode pg  portstate */
  TLV_ENCODE_PUTW (msg->state_b);

  return *pnt - sp;
}

/* Decode PG Switching over msg */
s_int32_t
nsm_decode_g8031_portstate_msg (u_char **pnt, u_int16_t *size,
                             struct nsm_msg_g8031_portstate * msg)
{
  u_char *sp = *pnt;

  /* deocode EPS-ID */
  TLV_DECODE_GETW(msg->eps_id);

  /* decode bridge_name */
  TLV_DECODE_GET(msg->bridge_name, NSM_BRIDGE_NAMSIZ);

  /* deocode Ifindex to be fwd state */
  TLV_DECODE_GETW(msg->ifindex_fwd);

  /* deocode pg switch to */
  TLV_DECODE_GETW(msg->state_f);

  /* deocode Ifindex to be blck state */
  TLV_DECODE_GETW(msg->ifindex_blck);

  /* deocode pg switch to */
  TLV_DECODE_GETW(msg->state_b);

  return *pnt - sp;
}

#endif /* HAVE_G8031 */

#ifdef HAVE_G8032
/* Encode Vlan Group msg */
s_int32_t
nsm_encode_g8032_create_vlan_msg ( u_char **pnt, u_int16_t *size,
    struct nsm_msg_g8032_vlan * msg)
{
  u_char *sp = *pnt;

  /* PVID */
  TLV_ENCODE_PUTW (msg->is_primary);

  /* bridge_name */
  TLV_ENCODE_PUT (msg->bridge_name, NSM_BRIDGE_NAMSIZ);

  /* Ring-ID */
  TLV_ENCODE_PUTW (msg->ring_id);

  /* VID */
  TLV_ENCODE_PUTW (msg->vid);

  return *pnt - sp;
}

/* Decode Vlan Group msg */
s_int32_t
nsm_decode_g8032_create_vlan_msg ( u_char **pnt, u_int16_t *size,
                                   struct nsm_msg_g8032_vlan * msg)
{
  u_char *sp = *pnt;

  /* RING-ID */
  TLV_DECODE_GETW(msg->is_primary);

  /* decode bridge_name */
  TLV_DECODE_GET(msg->bridge_name, NSM_BRIDGE_NAMSIZ);

  /* RING-ID */
  TLV_DECODE_GETW(msg->ring_id);

  /* VID */
  TLV_DECODE_GETW(msg->vid);

  return *pnt - sp;
}
#endif /*HAVE_G8032*/

#ifdef HAVE_PBB_TE
s_int32_t
nsm_encode_te_vlan_msg (u_char **pnt, u_int16_t *size,
                       struct nsm_msg_pbb_te_vlan *msg)
{
  u_char *sp = *pnt;

  /* Encode bridge_name */
  TLV_ENCODE_PUTL (msg->tesid);

  /* Encode cbp_ifindex */
  TLV_ENCODE_PUTW (msg->vid);

  /* Encode bridge_name */
  TLV_ENCODE_PUT (msg->brname, NSM_BRIDGE_NAMSIZ);

  return *pnt - sp;
}

s_int32_t
nsm_decode_te_vlan_msg (u_char **pnt, u_int16_t *size,
                       struct nsm_msg_pbb_te_vlan *msg)
{
  u_char *sp = *pnt;

  /* deocode isid */
  TLV_DECODE_GETL(msg->tesid);

  /* decode bvid */
  TLV_DECODE_GETW(msg->vid);

  /* decode bridge_name */
  TLV_DECODE_GET(msg->brname, NSM_BRIDGE_NAMSIZ);

  return *pnt - sp;
}

s_int32_t
nsm_encode_te_esp_msg (u_char **pnt, u_int16_t *size,
                       struct nsm_msg_pbb_te_esp *msg)
{
  u_char *sp = *pnt;

  /* Encode bridge_name */
  TLV_ENCODE_PUTL (msg->cbp_ifindex);

  /* Encode cbp_ifindex */
  TLV_ENCODE_PUTL (msg->tesid);

  TLV_ENCODE_PUT (msg->remote_mac, ETHER_ADDR_LEN);

  TLV_ENCODE_PUT (msg->mcast_rx_mac, ETHER_ADDR_LEN);

  TLV_ENCODE_PUTW (msg->esp_vid);

  TLV_ENCODE_PUTC (msg->multicast);

  TLV_ENCODE_PUTC (msg->egress);

  return *pnt - sp;
}

s_int32_t
nsm_decode_te_esp_msg (u_char **pnt, u_int16_t *size,
                       struct nsm_msg_pbb_te_esp *msg)
{
  u_char *sp = *pnt;


  TLV_DECODE_GETL (msg->cbp_ifindex);

  /* Encode cbp_ifindex */
  TLV_DECODE_GETL (msg->tesid);

  TLV_DECODE_GET (msg->remote_mac, ETHER_ADDR_LEN);

  TLV_DECODE_GET (msg->mcast_rx_mac, ETHER_ADDR_LEN);

  TLV_DECODE_GETW (msg->esp_vid);

  TLV_DECODE_GETC (msg->multicast);

  TLV_DECODE_GETC (msg->egress);

  return *pnt - sp;
}

s_int32_t
nsm_encode_esp_pnp_msg (u_char **pnt, u_int16_t *size,
                       struct nsm_msg_pbb_esp_pnp *msg)
{
  u_char *sp = *pnt;

  /* Encode tesid */
  TLV_ENCODE_PUTL (msg->tesid);

  /* Encode pnp_ifindex */
  TLV_ENCODE_PUTL (msg->pnp_ifindex);

  /* encode cbp_ifindex */
  TLV_ENCODE_PUTL (msg->cbp_ifindex);

  return *pnt - sp;
}

s_int32_t
nsm_decode_esp_pnp_msg (u_char **pnt, u_int16_t *size,
                       struct nsm_msg_pbb_esp_pnp *msg)
{
  u_char *sp = *pnt;

  /* deocode tesid */
  TLV_DECODE_GETL(msg->tesid);

  /* decode pnp_ifindex */
  TLV_DECODE_GETL(msg->pnp_ifindex);

  /* decode pnp_ifindex */
  TLV_DECODE_GETL(msg->cbp_ifindex);

  return *pnt - sp;
}

s_int32_t
nsm_encode_te_aps_grp (u_char **pnt, u_int16_t *size,
                       struct nsm_msg_te_aps_grp *msg)
{
  u_char *sp = *pnt;

  TLV_ENCODE_PUTL (msg->te_aps_grpid);

  TLV_ENCODE_PUTL (msg->tesid_w);

  TLV_ENCODE_PUTL (msg->tesid_p);

  TLV_ENCODE_PUTL (msg->ifindex);

  return *pnt - sp;
}

s_int32_t
nsm_decode_te_aps_grp (u_char **pnt, u_int16_t *size,
                       struct nsm_msg_te_aps_grp *msg)
{
  u_char *sp = *pnt;

  TLV_DECODE_GETL (msg->te_aps_grpid);

  TLV_DECODE_GETL (msg->tesid_w);

  TLV_DECODE_GETL (msg->tesid_p);

  TLV_DECODE_GETL (msg->ifindex);

  return *pnt - sp;
}


#endif /* HAVE_PBB_TE */

#ifdef HAVE_PVLAN
/* Encode pvlan message. */
s_int32_t
nsm_encode_pvlan_if_msg (u_char **pnt, u_int16_t *size,
                          struct nsm_msg_pvlan_if *msg)
{
  u_char *sp = *pnt;

  /* Check size. */
  if (*size < NSM_MSG_PVLAN_IF_MSG)
    return NSM_ERR_PKT_TOO_SMALL;

  /* Encode bridgename. */
  TLV_ENCODE_PUT (msg->bridge_name, NSM_BRIDGE_NAMSIZ);

  TLV_ENCODE_PUTL (msg->ifindex);

  TLV_ENCODE_PUTW (msg->flags);

  return *pnt - sp;
}

s_int32_t
nsm_decode_pvlan_if_msg (u_char **pnt, u_int16_t *size,
                          struct nsm_msg_pvlan_if *msg)
{
   u_char *sp = *pnt;

  /* Check size. */
  if (*size < NSM_MSG_PVLAN_IF_MSG)
     return NSM_ERR_PKT_TOO_SMALL;

  /* Bridge name. */
  TLV_DECODE_GET (msg->bridge_name, NSM_BRIDGE_NAMSIZ);
  /* ifindex */
  TLV_DECODE_GETL (msg->ifindex);
 /*flags */
  TLV_DECODE_GETW (msg->flags);

  return *pnt - sp;
}

/* Encode pvlan configure message. */
s_int32_t
nsm_encode_pvlan_configure (u_char **pnt, u_int16_t *size,
                            struct nsm_msg_pvlan_configure *msg)
{
  u_char *sp = *pnt;

  /* Check size. */
  if (*size < NSM_MSG_PVLAN_CONFIG_MSG)
    return NSM_ERR_PKT_TOO_SMALL;

  /* Encode bridgename. */
  TLV_ENCODE_PUT (msg->bridge_name, NSM_BRIDGE_NAMSIZ);

  TLV_ENCODE_PUTW (msg->vid);

  TLV_ENCODE_PUTW (msg->type);

  return *pnt - sp;
}

/* Decode pvlan configure message */
s_int32_t
nsm_decode_pvlan_configure (u_char **pnt, u_int16_t *size,
                            struct nsm_msg_pvlan_configure *msg)
{
   u_char *sp = *pnt;

  /* Check size. */
  if (*size < NSM_MSG_PVLAN_CONFIG_MSG)
     return NSM_ERR_PKT_TOO_SMALL;

  /* Bridge name. */
  TLV_DECODE_GET (msg->bridge_name, NSM_BRIDGE_NAMSIZ);

  TLV_DECODE_GETW (msg->vid);

  TLV_DECODE_GETW (msg->type);

  return *pnt - sp;
}

/* Encode pvlan association of secondary vlan message. */
s_int32_t
nsm_encode_pvlan_associate (u_char **pnt, u_int16_t *size,
                            struct nsm_msg_pvlan_association *msg)
{
  u_char *sp = *pnt;

  /* Check size. */
  if (*size < NSM_MSG_PVLAN_ASSOCIATE_MSG)
    return NSM_ERR_PKT_TOO_SMALL;

  /* Encode bridgename. */
  TLV_ENCODE_PUT (msg->bridge_name, NSM_BRIDGE_NAMSIZ);

  TLV_ENCODE_PUTW (msg->primary_vid);

  TLV_ENCODE_PUTW (msg->secondary_vid);

  TLV_ENCODE_PUTW (msg->flags);

  return *pnt - sp;
}

/* Decode pvlan association of secondary vlan message. */
s_int32_t
nsm_decode_pvlan_associate (u_char **pnt, u_int16_t *size,
                            struct nsm_msg_pvlan_association *msg)
{
   u_char *sp = *pnt;

  /* Check size. */
  if (*size < NSM_MSG_PVLAN_ASSOCIATE_MSG)
     return NSM_ERR_PKT_TOO_SMALL;

  /* Bridge name. */
  TLV_DECODE_GET (msg->bridge_name, NSM_BRIDGE_NAMSIZ);

  TLV_DECODE_GETW (msg->primary_vid);

  TLV_DECODE_GETW (msg->secondary_vid);

  TLV_DECODE_GETW (msg->flags);

  return *pnt - sp;
}

#endif /* HAVE_PVLAN */

#ifdef HAVE_VLAN_CLASS
/* Encode Vlan Classifier message. */
s_int32_t
nsm_encode_vlan_classifier_msg (u_char **pnt, u_int16_t *size,
                                struct nsm_msg_vlan_classifier *msg)
{
  u_char *sp = *pnt;

  /* Check size. */
  if (*size < NSM_MSG_VLAN_CLASSIFIER_SIZE)
    return NSM_ERR_PKT_TOO_SMALL;

  /* Encode classifier group id. */
  TLV_ENCODE_PUTL (msg->class_group);

  /* Encode classifier type. */
  TLV_ENCODE_PUTL (msg->class_type);

  if (*size < NSM_MSG_VLAN_CLASSSIZ)
    return NSM_ERR_PKT_TOO_SMALL;

  /* Encode classifier data. */
  nsm_encode_tlv (pnt, size, NSM_VLAN_CTYPE_CLASSIFIER,
                  NSM_MSG_VLAN_CLASSSIZ);
  TLV_ENCODE_PUT (msg->class_data, NSM_MSG_VLAN_CLASSSIZ);

  return *pnt - sp;
}

/* Decode VLAN Classifier message. */
s_int32_t
nsm_decode_vlan_classifier_msg (u_char **pnt, u_int16_t *size,
                                struct nsm_msg_vlan_classifier *msg)
{
  struct nsm_tlv_header tlv;
  u_char *sp = *pnt;

  /* Check size. */
  if (*size < NSM_MSG_VLAN_CLASSIFIER_SIZE)
     return NSM_ERR_PKT_TOO_SMALL;

  /* Group Id. */
  TLV_DECODE_GETL(msg->class_group);

  /* Type */
  TLV_DECODE_GETL(msg->class_type);

  if (*size < NSM_TLV_HEADER_SIZE)
    return NSM_ERR_PKT_TOO_SMALL;

  NSM_DECODE_TLV_HEADER (tlv);

  switch (tlv.type)
  {
    case NSM_VLAN_CTYPE_CLASSIFIER:
      TLV_DECODE_GET (msg->class_data, tlv.length);
      NSM_SET_CTYPE (msg->cindex, tlv.type);
     break;
    default:
      TLV_DECODE_SKIP (NSM_MSG_VLAN_CLASSSIZ);
     break;
  }

  return *pnt - sp;
}

/* Encode VLAN port classifier message. */
s_int32_t
nsm_encode_vlan_port_class_msg (u_char **pnt, u_int16_t *size,
                                struct nsm_msg_vlan_port_classifier *msg)
{
  u_char *sp = *pnt;

  /* Check size. */
  if (*size < NSM_MSG_VLAN_PORT_SIZE)
    return NSM_ERR_PKT_TOO_SMALL;

  /* Encode ifindex. */
  TLV_ENCODE_PUTL (msg->ifindex);

  /* Encode Vlan Id */
  TLV_ENCODE_PUTW (msg->vid_info);

  if (*size < (msg->cnum * sizeof (u_int8_t)))
    return NSM_ERR_PKT_TOO_SMALL;

  /* Encode port ifindexes. */
  nsm_encode_tlv (pnt, size, NSM_VLAN_PORT_CTYPE_CLASS_INFO,
                  msg->cnum * sizeof (u_int8_t));
  TLV_ENCODE_PUT (msg->class_info,
                  msg->cnum * sizeof (u_int8_t));

  return *pnt - sp;
}

/* Decode VLAN port classifier message. */
s_int32_t
nsm_decode_vlan_port_class_msg (u_char **pnt, u_int16_t *size,
                                struct nsm_msg_vlan_port_classifier *msg)
{
  struct nsm_tlv_header tlv;
  u_char *sp = *pnt;

  /* Check size. */
  if (*size < NSM_MSG_VLAN_PORT_SIZE)
     return NSM_ERR_PKT_TOO_SMALL;

  /* Decode ifindex. */
  TLV_DECODE_GETL (msg->ifindex);

  /* Decode Vlan Id */
  TLV_DECODE_GETW (msg->vid_info);

  msg->cnum = 0;

  /* TLV parse.  */
  while (*size)
    {
      if (*size < NSM_TLV_HEADER_SIZE)
        return NSM_ERR_PKT_TOO_SMALL;

      NSM_DECODE_TLV_HEADER (tlv);

      switch (tlv.type)
        {
        case NSM_VLAN_PORT_CTYPE_CLASS_INFO:
          msg->cnum = tlv.length/sizeof (u_int8_t);
          msg->class_info = XCALLOC (MTYPE_TMP, tlv.length);

          if (! msg->ifindex)
            {
              return NSM_ERR_SYSTEM_FAILURE;
            }

          TLV_DECODE_GET (msg->class_info, tlv.length);
          NSM_SET_CTYPE (msg->cindex, tlv.type);
          break;
        default:
          TLV_DECODE_SKIP (tlv.length);
          break;
        }
    }

  return *pnt - sp;
}

#endif /* HAVE_VLAN_CLASS */

#endif /* HAVE_VLAN */
#endif /* HAVE_CUSTOM1 */

#ifdef HAVE_AUTHD
/* Encode port suthentication port state message */
int
nsm_encode_auth_port_state_msg (u_char **pnt, u_int16_t *size,
                                struct nsm_msg_auth_port_state *msg)
{
  /* Check size. */
  if (*size < NSM_MSG_AUTH_PORT_STATE_SIZE)
    return NSM_ERR_PKT_TOO_SMALL;

  /* Put the port ifindex */
  TLV_ENCODE_PUTL (msg->ifindex);

  /* Put the port control direction.  */
  TLV_ENCODE_PUTW (msg->ctrl);

  /* Put the port control state.  */
  TLV_ENCODE_PUTW (msg->state);

  return NSM_MSG_AUTH_PORT_STATE_SIZE;
}

#ifdef HAVE_MAC_AUTH
/* Encode auth-mac port state message */
s_int32_t
nsm_encode_auth_mac_port_state_msg (u_char **pnt, u_int16_t *size,
                                    struct nsm_msg_auth_mac_port_state *msg)
{
  /* Check size. */
  if (*size < NSM_MSG_MACAUTH_PORT_STATE_SIZE)
    return NSM_ERR_PKT_TOO_SMALL;

  /* Put the port ifindex */
  TLV_ENCODE_PUTL (msg->ifindex);

  /* Put the port control state.  */
  TLV_ENCODE_PUTW (msg->state);

  /* Put the port control mode.  */
  TLV_ENCODE_PUTW (msg->mode);

  return NSM_MSG_MACAUTH_PORT_STATE_SIZE;
}

/* Encode mac authentication status message */
s_int32_t
nsm_encode_auth_mac_status_msg (u_char **pnt, u_int16_t *size,
                                struct nsm_msg_auth_mac_status *msg)
{
  s_int32_t mac_counter;
  /* Check size. */
  if (*size < NSM_MSG_AUTH_MAC_STATUS_SIZE)
    return NSM_ERR_PKT_TOO_SMALL;

  /* Put the port ifindex */
  TLV_ENCODE_PUTL (msg->ifindex);

  TLV_ENCODE_PUTC (msg->status);

  TLV_ENCODE_PUTW (msg->radius_dynamic_vid);

  TLV_ENCODE_PUTW (msg->auth_fail_restrict_vid);

  for (mac_counter=0; mac_counter < (AUTH_ETHER_LEN * 2 +
       AUTH_ETHER_DELIMETER_LEN); mac_counter++)
    TLV_ENCODE_PUTC (msg->mac_addr[mac_counter]);

  TLV_ENCODE_PUTC (msg->mac_address_aging);
  TLV_ENCODE_PUTC (msg->auth_fail_action);
  TLV_ENCODE_PUTC (msg->dynamic_vlan_creation);

  return NSM_MSG_AUTH_MAC_STATUS_SIZE;
}

/* Decode auth-mac port state message */
s_int32_t
nsm_decode_auth_mac_port_state_msg (u_char **pnt, u_int16_t *size,
                                    struct nsm_msg_auth_mac_port_state *msg)
{
  /* Check size. */
  if (*size < NSM_MSG_MACAUTH_PORT_STATE_SIZE)
    return NSM_ERR_PKT_TOO_SMALL;

  /* Get the port ifindex */
  TLV_DECODE_GETL (msg->ifindex);

  /* Get the port control state */
  TLV_DECODE_GETW (msg->state);

  /* Get the port control mode */
  TLV_DECODE_GETW (msg->mode);

  return NSM_MSG_MACAUTH_PORT_STATE_SIZE;
}

/* Decode mac authentication status message */
s_int32_t
nsm_decode_auth_mac_status_msg (u_char **pnt, u_int16_t *size,
                                struct nsm_msg_auth_mac_status *msg)
{
  s_int32_t mac_counter;
  /* Check size. */
  if (*size < NSM_MSG_AUTH_MAC_STATUS_SIZE)
    return NSM_ERR_PKT_TOO_SMALL;

  TLV_DECODE_GETL (msg->ifindex);

  TLV_DECODE_GETC (msg->status);

  TLV_DECODE_GETW (msg->radius_dynamic_vid);

  TLV_DECODE_GETW (msg->auth_fail_restrict_vid);

  for (mac_counter=0; mac_counter < (AUTH_ETHER_LEN * 2 +
       AUTH_ETHER_DELIMETER_LEN); mac_counter++)
    TLV_DECODE_GETC (msg->mac_addr[mac_counter]);

  TLV_DECODE_GETC (msg->mac_address_aging);
  TLV_DECODE_GETC (msg->auth_fail_action);
  TLV_DECODE_GETC (msg->dynamic_vlan_creation);

  return NSM_MSG_AUTH_MAC_STATUS_SIZE;
}
#endif /* HAVE_MAC_AUTH */

/* Decode port suthentication port state message */
int
nsm_decode_auth_port_state_msg (u_char **pnt, u_int16_t *size,
                                struct nsm_msg_auth_port_state *msg)
{
  /* Check size. */
  if (*size < NSM_MSG_AUTH_PORT_STATE_SIZE)
    return NSM_ERR_PKT_TOO_SMALL;

  /* Get the port ifindex */
  TLV_DECODE_GETL (msg->ifindex);

  /* Get the port control direction.  */
  TLV_DECODE_GETW (msg->ctrl);

  /* Get the port control state */
  TLV_DECODE_GETW (msg->state);

  return NSM_MSG_AUTH_PORT_STATE_SIZE;
}
#endif /* HAVE_AUTHD */

#ifdef HAVE_ONMD

/* Encode EFM Interface message */
int
nsm_encode_efm_if_msg (u_char **pnt, u_int16_t *size,
                       struct nsm_msg_efm_if *msg)
{
  /* Check size. */
  if (*size < NSM_MSG_EFM_IF_SIZE)
    return NSM_ERR_PKT_TOO_SMALL;

  /* Put the port ifindex */
  TLV_ENCODE_PUTL (msg->ifindex);

  /* Put the Operation Code */
  TLV_ENCODE_PUTW (msg->opcode);

  /* Put the port parser state.  */
  TLV_ENCODE_PUTW (msg->local_par_action);

  /* Put the port multiplexer state.  */
  TLV_ENCODE_PUTW (msg->local_mux_action);

  /* Put the port local event.  */
  TLV_ENCODE_PUTW (msg->local_event);

  /* Put the port remote event.  */
  TLV_ENCODE_PUTW (msg->remote_event);

  /* Put the low word of symbol_period_window.  */
  TLV_ENCODE_PUTL (msg->symbol_period_window.l[0]);

  /* Put the high word of symbol_period_window.  */
  TLV_ENCODE_PUTL (msg->symbol_period_window.l[1]);

  /* Put the Window of other events.  */
  TLV_ENCODE_PUTL (msg->other_event_window);

  /* Put the low word of symbol_period_window.  */
  TLV_ENCODE_PUTL (msg->num_of_error.l[0]);

  /* Put the high word of symbol_period_window.  */
  TLV_ENCODE_PUTL (msg->num_of_error.l[1]);

  return NSM_MSG_EFM_IF_SIZE;
}


int
nsm_decode_efm_if_msg (u_char **pnt, u_int16_t *size,
                       struct nsm_msg_efm_if *msg)
{
  /* Check size. */
  if (*size < NSM_MSG_EFM_IF_SIZE)
    return NSM_ERR_PKT_TOO_SMALL;

  /* Get the port ifindex */
  TLV_DECODE_GETL (msg->ifindex);

  /* Get the Operation Code.  */
  TLV_DECODE_GETW (msg->opcode);

  /* Get the port parser state.  */
  TLV_DECODE_GETW (msg->local_par_action);

  /* Get the port multiplexer state */
  TLV_DECODE_GETW (msg->local_mux_action);

  /* Get the port local event.  */
  TLV_DECODE_GETW (msg->local_event);

  /* Get the port remote event.  */
  TLV_DECODE_GETW (msg->remote_event);

  /* Get the low word of symbol_period_window.  */
  TLV_DECODE_GETL (msg->symbol_period_window.l[0]);

  /* Get the high word of symbol_period_window.  */
  TLV_DECODE_GETL (msg->symbol_period_window.l[1]);

  /* Get the Window of other events.  */
  TLV_DECODE_GETL (msg->other_event_window);

  /* Get the low word of symbol_period_window.  */
  TLV_DECODE_GETL (msg->num_of_error.l[0]);

  /* Get the high word of symbol_period_window.  */
  TLV_DECODE_GETL (msg->num_of_error.l[1]);

  return NSM_MSG_EFM_IF_SIZE;
}

/* Encode LLDP message */
int
nsm_encode_lldp_msg (u_char **pnt, u_int16_t *size,
                     struct nsm_msg_lldp *msg)
{
  /* Check size. */
  if (*size < NSM_MSG_LLDP_SIZE)
    return NSM_ERR_PKT_TOO_SMALL;

  /* Put the Operation Code */
  TLV_ENCODE_PUTW (msg->opcode);

  /* Put the port ifindex */
  TLV_ENCODE_PUTL (msg->ifindex);

  /* Put the Systemp capability.  */
  TLV_ENCODE_PUTW (msg->syscap);

  /* Put the Protocol.  */
  TLV_ENCODE_PUTW (msg->protocol);

  /* Put the Aggregator ifindex.  */
  TLV_ENCODE_PUTW (msg->agg_ifindex);

  /* Put the LLDP HW MAC ADDRESS */
  TLV_ENCODE_PUT (msg->lldp_dest_addr, ETHER_ADDR_LEN);

  /* Put the return status */
  TLV_ENCODE_PUTC (msg->lldp_ret_status);

  return NSM_MSG_LLDP_SIZE;
}

int
nsm_decode_lldp_msg (u_char **pnt, u_int16_t *size,
                     struct nsm_msg_lldp *msg)
{
  /* Check size. */
  if (*size < NSM_MSG_LLDP_SIZE)
    return NSM_ERR_PKT_TOO_SMALL;

  /* Get the Operation Code */
  TLV_DECODE_GETW (msg->opcode);

  /* Get the port ifindex */
  TLV_DECODE_GETL (msg->ifindex);

  /* Get the Systemp capability.  */
  TLV_DECODE_GETW (msg->syscap);

  /* Get the Protocol.  */
  TLV_DECODE_GETW (msg->protocol);

  /* Get the Aggregator ifindex.  */
  TLV_DECODE_GETW (msg->agg_ifindex);

  /* Get the HW Address */
  TLV_DECODE_GET (msg->lldp_dest_addr, ETHER_ADDR_LEN);

  /* Get the Ret Status */
  TLV_DECODE_GETC (msg->lldp_ret_status);

  return NSM_MSG_LLDP_SIZE;

}

int
nsm_encode_cfm_msg (u_char **pnt, u_int16_t *size,
                    struct nsm_msg_cfm *msg)
{
  /* Check size. */
  if (*size < NSM_MSG_CFM_SIZE)
    return NSM_ERR_PKT_TOO_SMALL;

  /* Put the Operation Code */
  TLV_ENCODE_PUTW (msg->opcode);

  /* Put the port ifindex */
  TLV_ENCODE_PUTL (msg->ifindex);

  /* Put the CFM Dest ADDRESS */
  TLV_ENCODE_PUT (msg->cfm_dest_addr, ETHER_ADDR_LEN);

  /* Put the CFM Level */
  TLV_ENCODE_PUTC (msg->level);

  /* Put the Ehernet Type */
  TLV_ENCODE_PUTW (msg->ether_type);

  return NSM_MSG_CFM_SIZE;
}

/* Encode CFM message */
int
nsm_decode_cfm_msg (u_char **pnt, u_int16_t *size,
                    struct nsm_msg_cfm *msg)
{
  /* Check size. */
  if (*size < NSM_MSG_CFM_SIZE)
    return NSM_ERR_PKT_TOO_SMALL;


  /* Decode the Operation Code */
  TLV_DECODE_GETW (msg->opcode);

  /* Decode the port ifindex */
  TLV_DECODE_GETL (msg->ifindex);

  /* Decode the CFM Dest ADDRESS */
  TLV_DECODE_GET (msg->cfm_dest_addr, ETHER_ADDR_LEN);

  /* Decode the CFM Level */
  TLV_DECODE_GETC (msg->level);

  /* Decode the Ehernet Type */
  TLV_DECODE_GETW (msg->ether_type);

  return NSM_MSG_CFM_SIZE;

}

/* Encode CFM UNI-MEG status MSG */
int
nsm_encode_cfm_status_msg (u_char **pnt, u_int16_t *size,
                    struct nsm_msg_cfm_status *msg)
{
  /* Check size. */
  if (*size < NSM_MSG_CFM_STATUS_SIZE)
    return NSM_ERR_PKT_TOO_SMALL;

  /* Put the port ifindex */
  TLV_ENCODE_PUTL (msg->ifindex);

  /* Put the UNI-MEP Operational Status */
  TLV_ENCODE_PUTC (msg->uni_mep_status);

  return NSM_MSG_CFM_STATUS_SIZE;
}

/* Decode CFM UNI-MEG status MSG */
int
nsm_decode_cfm_status_msg (u_char **pnt, u_int16_t *size,
                    struct nsm_msg_cfm_status *msg)
{
  /* Check size. */
  if (*size < NSM_MSG_CFM_STATUS_SIZE)
    return NSM_ERR_PKT_TOO_SMALL;

  /* Decode the port ifindex */
  TLV_DECODE_GETL (msg->ifindex);

  /* Decode the UNI-MEP CFM Status */
  TLV_DECODE_GETC (msg->uni_mep_status);

  return NSM_MSG_CFM_STATUS_SIZE;

}

#endif /* HAVE_ONMD */

#ifdef HAVE_ELMID
/* Encode and Decode functions for ELMI Protocol */
/* Encode EVC delete message. */
int
nsm_encode_evc_msg (u_char **pnt, u_int16_t *size,
                    struct nsm_msg_elmi_evc *msg)
{
  u_char *sp = *pnt;
  int i = 0;

  /* Check size. */
  if (*size < NSM_MSG_EVC_SIZE)
    return NSM_ERR_PKT_TOO_SMALL;

  /* Put VID. */
  TLV_ENCODE_PUTL (msg->ifindex);
  TLV_ENCODE_PUTW (msg->evc_ref_id);

  for (i = 0; i < NSM_EVC_CINDEX_NUM; i++)
    {
      if (NSM_CHECK_CTYPE (msg->cindex, i))
        {
          if (*size < NSM_TLV_HEADER_SIZE)
            return NSM_ERR_PKT_TOO_SMALL;

          switch (i)
            {
            case NSM_ELMI_CTYPE_BR_NAME:
              nsm_encode_tlv (pnt, size, i, NSM_BRIDGE_NAMSIZ);
              TLV_ENCODE_PUT (msg->bridge_name, NSM_BRIDGE_NAMSIZ);
              break;
            case NSM_ELMI_CTYPE_EVC_STATUS_TYPE:
              nsm_encode_tlv (pnt, size, i, 1);
              TLV_ENCODE_PUTC (msg->evc_status_type);
              break;
            case NSM_ELMI_CTYPE_EVC_TYPE:
              nsm_encode_tlv (pnt, size, i, 1);
              TLV_ENCODE_PUTC (msg->evc_type);
              break;
            case NSM_ELMI_CTYPE_EVC_ID_STR:
              nsm_encode_tlv (pnt, size, i, NSM_EVC_NAME_LEN);
              TLV_ENCODE_PUT (msg->evc_id, NSM_EVC_NAME_LEN);
              break;
            case NSM_ELMI_CTYPE_DEFAULT_EVC:
              nsm_encode_tlv (pnt, size, i, 1);
              TLV_ENCODE_PUTC (msg->default_evc);
              break;
            case NSM_ELMI_CTYPE_UNTAGGED_PRI:
              nsm_encode_tlv (pnt, size, i, 1);
              TLV_ENCODE_PUTC (msg->untagged_frame);
              break;
            case NSM_ELMI_CTYPE_EVC_CVLAN_BITMAPS:
                {

                  nsm_encode_tlv (pnt, size, i, msg->num * sizeof(u_int16_t));
                  TLV_ENCODE_PUT (msg->vid_info, msg->num * sizeof(u_int16_t));
                }
              break;
            default:
              break;
            }
        }
    }

  return *pnt - sp;

}

/* Encode EVC delete message. */
int
nsm_decode_evc_msg (u_char **pnt, u_int16_t *size,
                    struct nsm_msg_elmi_evc *msg)
{

  struct nsm_tlv_header tlv;
  u_char *sp = *pnt;

  pal_mem_set(&tlv, 0, sizeof (struct nsm_tlv_header));

  /* Check size. */
  if (*size < NSM_MSG_EVC_SIZE)
    return NSM_ERR_PKT_TOO_SMALL;

  /* ifindex */
  TLV_DECODE_GETL (msg->ifindex);

  /* vid is used as EVC Ref ID. */
  TLV_DECODE_GETW (msg->evc_ref_id);

  msg->num = 0;

  while (*size)
    {
      if (*size < NSM_TLV_HEADER_SIZE)
        return NSM_ERR_PKT_TOO_SMALL;

      NSM_DECODE_TLV_HEADER (tlv);

      switch (tlv.type)
        {
        case NSM_ELMI_CTYPE_BR_NAME:
          TLV_DECODE_GET (msg->bridge_name, tlv.length);
          NSM_SET_CTYPE (msg->cindex, tlv.type);
          break;
        case NSM_ELMI_CTYPE_EVC_STATUS_TYPE:
          TLV_DECODE_GETC (msg->evc_status_type);
          NSM_SET_CTYPE (msg->cindex, tlv.type);
          break;
        case NSM_ELMI_CTYPE_EVC_TYPE:
          TLV_DECODE_GETC (msg->evc_type);
          NSM_SET_CTYPE (msg->cindex, tlv.type);
          break;
        case NSM_ELMI_CTYPE_EVC_ID_STR:
          TLV_DECODE_GET (msg->evc_id, tlv.length);
          NSM_SET_CTYPE (msg->cindex, tlv.type);
          break;
        case NSM_ELMI_CTYPE_DEFAULT_EVC:
          TLV_DECODE_GETC (msg->default_evc);
          NSM_SET_CTYPE (msg->cindex, tlv.type);
          break;
        case NSM_ELMI_CTYPE_UNTAGGED_PRI:
          TLV_DECODE_GETC (msg->untagged_frame);
          NSM_SET_CTYPE (msg->cindex, tlv.type);
          break;
        case NSM_ELMI_CTYPE_EVC_CVLAN_BITMAPS:
              msg->num = tlv.length/sizeof(u_int16_t);
              msg->vid_info = XCALLOC (MTYPE_TMP, tlv.length);
              TLV_DECODE_GET (msg->vid_info, tlv.length);
              NSM_SET_CTYPE (msg->cindex, tlv.type);
          break;
        default:
          TLV_DECODE_SKIP (tlv.length);
          break;
        }

    }
  return *pnt - sp;

}

/* Encode ELMI status MSG */
int
nsm_encode_elmi_status_msg (u_char **pnt, u_int16_t *size,
                            struct nsm_msg_elmi_status *msg)
{
  /* Check size. */
  if (*size < NSM_MSG_ELMI_STATUS_SIZE)
    return NSM_ERR_PKT_TOO_SMALL;

  /* Put the port ifindex */
  TLV_ENCODE_PUTL (msg->ifindex);

  /* Put the ELMI Operational Status */
  TLV_ENCODE_PUTC (msg->elmi_operational_status);

  return NSM_MSG_ELMI_STATUS_SIZE;
}

/* Decode ELMI status MSG */
int
nsm_decode_elmi_status_msg (u_char **pnt, u_int16_t *size,
                           struct nsm_msg_elmi_status *msg)
{

  u_char *sp = *pnt;

  /* Check size. */
  if (*size < NSM_MSG_ELMI_STATUS_SIZE)
    return NSM_ERR_PKT_TOO_SMALL;

  /* Decode the port ifindex */
  TLV_DECODE_GETL (msg->ifindex);

  /* Decode the ELMI Operational Status */
  TLV_DECODE_GETC (msg->elmi_operational_status);

  return *pnt - sp;

}

/* Encode auto vlan config MSG */
int
nsm_encode_elmi_vlan_msg (u_char **pnt, u_int16_t *size,
                          struct nsm_msg_vlan_port *msg)
{

  u_char *sp = *pnt;

  /* Check size. */
  if (*size < NSM_MSG_VLAN_PORT_SIZE)
    return NSM_ERR_PKT_TOO_SMALL;

  /* Put the port ifindex */
  TLV_ENCODE_PUTL (msg->ifindex);

  if (*size < (msg->num * sizeof (u_int16_t)))
    return NSM_ERR_PKT_TOO_SMALL;

   /* Encode port ifindexes. */
    nsm_encode_tlv (pnt, size, NSM_VLAN_PORT_CTYPE_VID_INFO,
                          msg->num * sizeof (u_int16_t));

  TLV_ENCODE_PUT (msg->vid_info,
      msg->num * sizeof (u_int16_t));

  return *pnt - sp;

}

/* Decode auto vlan config MSG */
int
nsm_decode_elmi_vlan_msg (u_char **pnt, u_int16_t *size,
                          struct nsm_msg_vlan_port *msg)
{
  struct nsm_tlv_header tlv;
  u_char *sp = *pnt;

  /* Check size. */
  if (*size < NSM_MSG_VLAN_PORT_SIZE)
    return NSM_ERR_PKT_TOO_SMALL;

  /* Decode ifindex. */
  TLV_DECODE_GETL (msg->ifindex);

  msg->num = 0;

  /* TLV parse.  */
  while (*size)
    {
      if (*size < NSM_TLV_HEADER_SIZE)
        return NSM_ERR_PKT_TOO_SMALL;

      NSM_DECODE_TLV_HEADER (tlv);

      switch (tlv.type)
        {
        case NSM_VLAN_PORT_CTYPE_VID_INFO:
          msg->num = tlv.length/sizeof (u_int16_t);

          msg->vid_info = XCALLOC (MTYPE_TMP, tlv.length);
          if (! msg->ifindex)
            {
              return NSM_ERR_SYSTEM_FAILURE;
            }

          TLV_DECODE_GET (msg->vid_info, tlv.length);
          NSM_SET_CTYPE (msg->cindex, tlv.type);
          break;
        default:
          TLV_DECODE_SKIP (tlv.length);
          break;
        }
    }

  return *pnt - sp;

}

int
nsm_encode_uni_msg (u_char **pnt, u_int16_t *size,
                    struct nsm_msg_uni *msg)
{
  u_char *sp = *pnt;
  int i = 0;

  /* Check size. */
  if (*size < NSM_MSG_UNI_SIZE)
    return NSM_ERR_PKT_TOO_SMALL;

  /* Put VID. */
  TLV_ENCODE_PUTL (msg->ifindex);

  for (i = 0; i < NSM_EVC_CINDEX_NUM; i++)
    {
      if (NSM_CHECK_CTYPE (msg->cindex, i))
        {
          if (*size < NSM_TLV_HEADER_SIZE)
            return NSM_ERR_PKT_TOO_SMALL;

          switch (i)
            {
            case NSM_ELMI_CTYPE_UNI_CVLAN_MAP_TYPE:
              nsm_encode_tlv (pnt, size, i, 1);
              TLV_ENCODE_PUTC (msg->cevlan_evc_map_type);
              break;
            case NSM_ELMI_CTYPE_UNI_ID:
              nsm_encode_tlv (pnt, size, i, NSM_UNI_NAME_LEN);
              TLV_ENCODE_PUT (msg->uni_id, NSM_UNI_NAME_LEN);
              break;
            default:
              break;
            }
        }
    }

  return *pnt - sp;

}


/* decode UNI message. */
int
nsm_decode_uni_msg (u_char **pnt, u_int16_t *size,
                    struct nsm_msg_uni *msg)
{

  struct nsm_tlv_header tlv;
  u_char *sp = *pnt;

  /* Check size. */
  if (*size < NSM_MSG_UNI_SIZE)
    return NSM_ERR_PKT_TOO_SMALL;

  /* ifindex. */
  TLV_DECODE_GETL (msg->ifindex);

  pal_mem_set(&tlv, 0, sizeof (struct nsm_tlv_header));

  while (*size)
    {
      if (*size < NSM_TLV_HEADER_SIZE)
        return NSM_ERR_PKT_TOO_SMALL;

      NSM_DECODE_TLV_HEADER (tlv);

      switch (tlv.type)
        {
        case NSM_ELMI_CTYPE_UNI_CVLAN_MAP_TYPE:
          TLV_DECODE_GETC (msg->cevlan_evc_map_type);
          NSM_SET_CTYPE (msg->cindex, tlv.type);
          break;

         case NSM_ELMI_CTYPE_UNI_ID:
          TLV_DECODE_GET (msg->uni_id, tlv.length);
          NSM_SET_CTYPE (msg->cindex, tlv.type);
          break;

        default:
          TLV_DECODE_SKIP (tlv.length);
          break;
        }

    }

  return *pnt - sp;
}

int
nsm_encode_elmi_bw_msg (u_char **pnt, u_int16_t *size,
                        struct nsm_msg_bw_profile *msg)
{
  u_char *sp = *pnt;
  s_int32_t i = 0;

  /* Check size.  */
  if (*size < NSM_ELMI_BW_SIZE)
    return NSM_ERR_PKT_TOO_SMALL;

  TLV_ENCODE_PUTL (msg->ifindex);

  TLV_ENCODE_PUTL (msg->cir);
  TLV_ENCODE_PUTL (msg->cbs);
  TLV_ENCODE_PUTL (msg->eir);
  TLV_ENCODE_PUTL (msg->ebs);

  TLV_ENCODE_PUTC (msg->cm);
  TLV_ENCODE_PUTC (msg->cf);

  /* TLV parse. */
  for (i = 0; i < NSM_CINDEX_SIZE; i++)
    if (NSM_CHECK_CTYPE (msg->cindex, i))
      {
        if (*size < NSM_TLV_HEADER_SIZE)
          return NSM_ERR_PKT_TOO_SMALL;

        switch(i)
          {
          case NSM_ELMI_CTYPE_UNI_BW:
            nsm_encode_tlv (pnt, size, i, 0);
            break;
          case NSM_ELMI_CTYPE_EVC_BW:
            nsm_encode_tlv (pnt, size, i, 2);
            TLV_ENCODE_PUTW (msg->evc_ref_id);
            break;
          case NSM_ELMI_CTYPE_EVC_COS_BW:
            nsm_encode_tlv (pnt, size, i, 4);
            TLV_ENCODE_PUTW (msg->evc_ref_id);
            TLV_ENCODE_PUTC (msg->per_cos);
            TLV_ENCODE_PUTC (msg->cos_id);
            break;
          default:
            break;
          }
      }

  return *pnt - sp;

}

int
nsm_decode_elmi_bw_msg (u_char **pnt, u_int16_t *size,
                        struct nsm_msg_bw_profile *msg)
{
  u_char *sp = *pnt;
  struct nsm_tlv_header tlv;

  /* Check size.  */
  if (*size < NSM_ELMI_BW_SIZE)
    return NSM_ERR_PKT_TOO_SMALL;

  TLV_DECODE_GETL (msg->ifindex);

  TLV_DECODE_GETL (msg->cir);
  TLV_DECODE_GETL (msg->cbs);
  TLV_DECODE_GETL (msg->eir);
  TLV_DECODE_GETL (msg->ebs);

  TLV_DECODE_GETC (msg->cm);
  TLV_DECODE_GETC (msg->cf);

  while (*size)
    {
      if (*size < NSM_TLV_HEADER_SIZE)
        return NSM_ERR_PKT_TOO_SMALL;

      NSM_DECODE_TLV_HEADER (tlv);

      switch (tlv.type)
        {
        case NSM_ELMI_CTYPE_UNI_BW:
          NSM_SET_CTYPE (msg->cindex, tlv.type);
          break;
        case NSM_ELMI_CTYPE_EVC_BW:
          TLV_DECODE_GETW (msg->evc_ref_id);
          NSM_SET_CTYPE (msg->cindex, tlv.type);
          break;
        case NSM_ELMI_CTYPE_EVC_COS_BW:
          TLV_DECODE_GETW (msg->evc_ref_id);
          TLV_DECODE_GETC (msg->cos_id);
          TLV_DECODE_GETC (msg->per_cos);
          NSM_SET_CTYPE (msg->cindex, tlv.type);
          break;
        default:
          TLV_DECODE_SKIP (tlv.length);
          break;
        }
    }

  return *pnt - sp;

}

#endif /* HAVE_ELMID */

/* All message parser.  These parser are used by both server and
   client side. */

/* Parse NSM service message.  */
s_int32_t
nsm_parse_service (u_char **pnt, u_int16_t *size,
                   struct nsm_msg_header *header, void *arg,
                   NSM_CALLBACK callback)
{
  int ret;
  struct nsm_msg_service msg;

  pal_mem_set (&msg, 0, sizeof (struct nsm_msg_service));

  /* Parse service.  */
  ret = nsm_decode_service (pnt, size, &msg);
  if (ret < 0)
    return ret;

  /* Call callback with arg. */
  if (callback)
    {
      ret = (*callback) (header, arg, &msg);
      if (ret < 0)
        return ret;
    }
  return 0;
}

/* Parse NSM interface message.  */
s_int32_t
nsm_parse_link (u_char **pnt, u_int16_t *size,
                struct nsm_msg_header *header, void *arg,
                NSM_CALLBACK callback)
{
  struct nsm_msg_link msg;
  int ret = 0;

  pal_mem_set (&msg, 0, sizeof (struct nsm_msg_link));

  ret = nsm_decode_link (pnt, size, &msg);
  if (ret < 0)
    return ret;

  if (callback)
    ret = (*callback) (header, arg, &msg);

  return ret;
}

s_int32_t
nsm_parse_vr (u_char **pnt, u_int16_t *size,
              struct nsm_msg_header *header, void *arg,
              NSM_CALLBACK callback)
{
  struct nsm_msg_vr msg;
  int ret;

  pal_mem_set (&msg, 0, sizeof (struct nsm_msg_vr));

  ret = nsm_decode_vr (pnt, size, &msg);
  if (ret < 0)
    return ret;

  ret = 0;
  if (callback)
    ret = (*callback) (header, arg, &msg);

  return ret;
}

s_int32_t
nsm_parse_vr_done (u_char **pnt, u_int16_t *size,
    struct nsm_msg_header *header, void *arg,
    NSM_CALLBACK callback)
{
  int ret = 0;

  if (callback)
    ret = (*callback) (header, arg, NULL);

  return ret;
}

s_int32_t
nsm_parse_vr_bind (u_char **pnt, u_int16_t *size,
                   struct nsm_msg_header *header, void *arg,
                   NSM_CALLBACK callback)
{
  struct nsm_msg_vr_bind msg;
  int ret = 0;

  pal_mem_set (&msg, 0, sizeof msg);

  ret = nsm_decode_vr_bind (pnt, size, &msg);
  if (ret < 0)
    return ret;

  if (callback)
    ret = (*callback) (header, arg, &msg);

  return ret;
}

s_int32_t
nsm_parse_vrf_bind (u_char **pnt, u_int16_t *size,
                    struct nsm_msg_header *header, void *arg,
                    NSM_CALLBACK callback)
{
  struct nsm_msg_vrf_bind msg;
  int ret = 0;

  pal_mem_set (&msg, 0, sizeof msg);

  ret = nsm_decode_vrf_bind (pnt, size, &msg);
  if (ret < 0)
    return ret;

  if (callback)
    ret = (*callback) (header, arg, &msg);

  return ret;
}

int
nsm_encode_server_status_msg (u_char **pnt, u_int16_t *size,
                              struct nsm_msg_server_status *msg)
{
  u_char *sp = *pnt;
  
   /* NSM Server status. */
  TLV_ENCODE_PUTC (msg->nsm_status);

  return *pnt - sp;
}

int
nsm_decode_server_status_msg (u_char **pnt, u_int16_t *size,
                               struct nsm_msg_server_status *msg)
{
  u_char *sp = *pnt;
  
  /* NSM Server status  */
  TLV_DECODE_GETC (msg->nsm_status);

  return *pnt - sp;
}

int
nsm_parse_nsm_server_status_notify (u_char **pnt, u_int16_t *size,
                                    struct nsm_msg_header *header, void *arg,
                                    NSM_CALLBACK callback)
{
  int ret = 0;
  struct nsm_msg_server_status msg;
 
  pal_mem_set (&msg, 0, sizeof (struct nsm_msg_server_status)); 
  
  ret = nsm_decode_server_status_msg(pnt, size, &msg);
  if (ret < 0)
    return ret;

  if (callback)
    ret = (*callback) (header, arg, &msg);

  return ret;
}                    


/* Parse NSM interface address message.  */
s_int32_t
nsm_parse_address (u_char **pnt, u_int16_t *size,
                   struct nsm_msg_header *header, void *arg,
                   NSM_CALLBACK callback)
{
  struct nsm_msg_address msg;
  int ret;

  pal_mem_set (&msg, 0, sizeof (struct nsm_msg_address));

  ret = nsm_decode_address (pnt, size, &msg);
  if (ret < 0)
    return ret;

  if (callback)
    {
      ret = (*callback) (header, arg, &msg);
      if (ret < 0)
        return ret;
    }

  return 0;
}

#if 0 /* XXX-VR */
s_int32_t
nsm_parse_address (u_char **pnt, u_int16_t *size,
                   struct nsm_msg_header *header, void *arg,
                   NSM_CALLBACK callback)
{
  int ret;
  struct nsm_msg_address msg;

  pal_mem_set (&msg, 0, sizeof (struct nsm_msg_address));

  ret = nsm_decode_address (pnt, size, &msg);
  if (ret < 0)
    return ret;

  if (callback)
    {
      ret = (*callback) (header, arg, &msg);
      if (ret < 0)
        return ret;
    }

  if (msg.ifname)
    XFREE (MTYPE_TMP, msg.ifname);

  return 0;
}
#endif

/* Parse NSM redistribute message.  */
s_int32_t
nsm_parse_redistribute (u_char **pnt, u_int16_t *size,
                        struct nsm_msg_header *header, void *arg,
                        NSM_CALLBACK callback)
{
  int ret;
  struct nsm_msg_redistribute msg;

  pal_mem_set (&msg, 0, sizeof (struct nsm_msg_redistribute));

  ret = nsm_decode_redistribute (pnt, size, &msg);
  if (ret < 0)
    return ret;

  if (callback)
    {
      ret = (*callback) (header, arg, &msg);
      if (ret < 0)
        return ret;
    }
  return 0;
}

/* Parse NSM IPv4 message.  */
s_int32_t
nsm_parse_route_ipv4 (u_char **pnt, u_int16_t *size,
                      struct nsm_msg_header *header, void *arg,
                      NSM_CALLBACK callback)
{
  int ret = 0;
  struct nsm_msg_route_ipv4 msg;
  int nbytes;
  u_int16_t length = *size;

  while (length)
    {
      pal_mem_set (&msg, 0, sizeof (struct nsm_msg_route_ipv4));

      nbytes = nsm_decode_route_ipv4 (pnt, size, &msg);
      if (nbytes < 0)
        return nbytes;

      if (callback)
        {
          ret = (*callback) (header, arg, &msg);
          if (ret < 0)
            return ret;
        }

      if (nbytes > length)
        break;

      length -= nbytes;
    }

  return 0;
}

#ifdef HAVE_IPV6
/* Parse NSM IPv6 message.  */
s_int32_t
nsm_parse_route_ipv6 (u_char **pnt, u_int16_t *size,
                      struct nsm_msg_header *header, void *arg,
                      NSM_CALLBACK callback)
{
  int ret = 0;
  struct nsm_msg_route_ipv6 msg;
  int nbytes;
  u_int16_t length = *size;

  while (length)
    {
      pal_mem_set (&msg, 0, sizeof (struct nsm_msg_route_ipv6));

      nbytes = nsm_decode_route_ipv6 (pnt, size, &msg);
      if (nbytes < 0)
        return nbytes;

      if (callback)
        {
          ret = (*callback) (header, arg, &msg);
          if (ret < 0)
            return ret;
        }

      if (nbytes > length)
        break;

      length -= nbytes;
    }

  return 0;
}
#endif /* HAVE_IPV6 */

/* Parse NSM IPv4 exact nexthop lookup message. */
s_int32_t
nsm_parse_ipv4_route_lookup (u_char **pnt, u_int16_t *size,
                             struct nsm_msg_header *header, void *arg,
                             NSM_CALLBACK callback)
{
  int ret = 0;
  struct nsm_msg_route_lookup_ipv4 msg;

  pal_mem_set (&msg, 0, sizeof (struct nsm_msg_route_lookup_ipv4));

  ret = nsm_decode_ipv4_route_lookup (pnt, size, &msg);
  if (ret < 0)
    return ret;

  if (callback)
    {
      ret = (*callback) (header, arg, &msg);
      if (ret < 0)
        return ret;
    }
  return 0;
}

#ifdef HAVE_IPV6
s_int32_t
nsm_parse_ipv6_route_lookup (u_char **pnt, u_int16_t *size,
                             struct nsm_msg_header *header, void *arg,
                             NSM_CALLBACK callback)
{
  int ret = 0;
  struct nsm_msg_route_lookup_ipv6 msg;

  pal_mem_set (&msg, 0, sizeof (struct nsm_msg_route_lookup_ipv6));

  ret = nsm_decode_ipv6_route_lookup (pnt, size, &msg);
  if (ret < 0)
    return ret;

  if (callback)
    {
      ret = (*callback) (header, arg, &msg);
      if (ret < 0)
        return ret;
    }
  return 0;
}
#endif /* HAVE_IPV6 */

s_int32_t
nsm_parse_route_manage (u_char **pnt, u_int16_t *size,
                        struct nsm_msg_header *header, void *arg,
                        NSM_CALLBACK callback)
{
  int ret = 0;
  struct nsm_msg_route_manage msg;

  pal_mem_set (&msg, 0, sizeof (struct nsm_msg_route_manage));

  ret = nsm_decode_route_manage (pnt, size, &msg);
  if (ret < 0)
    return ret;

  if (callback)
    {
      ret = (*callback) (header, arg, &msg);
      if (ret < 0)
        return ret;
    }
  return 0;
}

s_int32_t
nsm_parse_vrf (u_char **pnt, u_int16_t *size,
               struct nsm_msg_header *header, void *arg,
               NSM_CALLBACK callback)
{
  struct nsm_msg_vrf msg;
  int ret;

  pal_mem_set (&msg, 0, sizeof (struct nsm_msg_vrf));

  ret = nsm_decode_vrf (pnt, size, &msg);
  if (ret < 0)
    return ret;

  ret = 0;
  if (callback)
    ret = (*callback) (header, arg, &msg);

  return ret;
}

#ifndef HAVE_IMI
s_int32_t
nsm_parse_user (u_char **pnt, u_int16_t *size,
                struct nsm_msg_header *header, void *arg,
                NSM_CALLBACK callback)
{
  struct nsm_msg_user msg;
  int ret = 0;

  pal_mem_set (&msg, 0, sizeof (struct nsm_msg_user));

  ret = nsm_decode_user (pnt, size, &msg);
  if (ret < 0)
    return ret;

  ret = 0;

  if (callback)
    ret = (*callback) (header, arg, &msg);

  return ret;
}
#endif /* HAVE_IMI */

#ifdef HAVE_MPLS
s_int32_t
nsm_parse_label_pool (u_char **pnt, u_int16_t *size,
                      struct nsm_msg_header *header, void *arg,
                      NSM_CALLBACK callback)
{
  int ret = 0;
  int lsblt = 0;

  struct nsm_msg_label_pool msg;

  pal_mem_set (&msg, 0, sizeof (struct nsm_msg_label_pool));

  ret = nsm_decode_label_pool (pnt, size, &msg);
  if (ret < 0)
    return ret;

  if (callback)
    {
      ret = (*callback) (header, arg, &msg);
      if (ret < 0)
        return ret;
    }

  for (lsblt = INCL_BLK_TYPE; lsblt < LSO_BLK_TYPE_MAX; lsblt++)
    {
      if (msg.blk_list[lsblt].lset_blocks)
        {
          XFREE(MTYPE_TMP, msg.blk_list[lsblt].lset_blocks);
        }
    }

  return 0;
}

s_int32_t
nsm_parse_gen_label_pool (u_char **pnt, u_int16_t *size,
                struct nsm_msg_header *header, void *arg,
                NSM_CALLBACK callback)
{
  return 0; /* TODO */
}

#ifdef HAVE_PACKET
s_int32_t
nsm_parse_ilm_lookup (u_char **pnt, u_int16_t *size,
                      struct nsm_msg_header *header, void *arg,
                      NSM_CALLBACK callback)
{
  int ret = 0;
  struct nsm_msg_ilm_lookup msg;

  pal_mem_set (&msg, 0, sizeof (struct nsm_msg_ilm_lookup));

  ret = nsm_decode_ilm_lookup (pnt, size, &msg);
  if (ret < 0)
    return ret;

  if (callback)
    {
      ret = (*callback) (header, arg, &msg);
      if (ret < 0)
        return ret;
    }
  return 0;
}
#endif /* HAVE_PACKET */

s_int32_t
nsm_parse_ilm_gen_lookup (u_char **pnt, u_int16_t *size,
                          struct nsm_msg_header *header, void *arg,
                          NSM_CALLBACK callback)
{
  int ret = 0;
  struct nsm_msg_ilm_gen_lookup msg;

  pal_mem_set (&msg, 0, sizeof (struct nsm_msg_ilm_gen_lookup));

  ret = nsm_decode_ilm_gen_lookup (pnt, size, &msg);
  if (ret < 0)
    return ret;

  if (callback)
    {
      ret = (*callback) (header, arg, &msg);
      if (ret < 0)
        return ret;
    }
  return 0;
}

#endif /* HAVE_MPLS */

#ifdef HAVE_TE
s_int32_t
nsm_parse_qos_client_init (u_char **pnt, u_int16_t *size,
                           struct nsm_msg_header *header, void *arg,
                           NSM_CALLBACK callback)
{
  int ret = 0;
  u_int32_t  protocol_id;

  ret = nsm_decode_qos_client_init (pnt, size, &protocol_id);
  if (ret < 0)
    return ret;

  if (callback)
    {
      ret = (*callback) (header, arg, &protocol_id);
      if (ret < 0)
        return ret;
    }
  return 0;
}

s_int32_t
nsm_parse_qos_preempt (u_char **pnt, u_int16_t *size,
                       struct nsm_msg_header *header, void *arg,
                       NSM_CALLBACK callback)
{
  int ret = 0;
  struct nsm_msg_qos_preempt msg;

  pal_mem_set (&msg, 0, sizeof (struct nsm_msg_qos_preempt));
  ret = nsm_decode_qos_preempt (pnt, size, &msg);
  if (ret < 0)
    return ret;

  ret = 0;

  if (callback)
    ret = (*callback) (header, arg, &msg);

  if (msg.preempted_ids)
    XFREE (MTYPE_TMP, msg.preempted_ids);

  return 0;
}

#ifdef HAVE_GMPLS
s_int32_t
nsm_parse_gmpls_qos_preempt (u_char **pnt, u_int16_t *size,
                             struct nsm_msg_header *header, void *arg,
                              NSM_CALLBACK callback)
{
  return nsm_parse_qos_preempt (pnt, size, header, arg, callback);
}
#endif /*HAVE_GMPLS*/

s_int32_t
nsm_parse_qos (u_char **pnt, u_int16_t *size,
               struct nsm_msg_header *header, void *arg,
               NSM_CALLBACK callback)
{
  int ret = 0;
  struct nsm_msg_qos msg;

  pal_mem_set (&msg, 0, sizeof (struct nsm_msg_qos));

  ret = nsm_decode_qos (pnt, size, &msg);
  if (ret < 0)
    return ret;

  if (callback)
    {
      ret = (*callback) (header, arg, &msg);
      if (ret < 0)
        return ret;
    }
  return 0;
}

s_int32_t
nsm_parse_qos_release (u_char **pnt, u_int16_t *size,
                       struct nsm_msg_header *header, void *arg,
                       NSM_CALLBACK callback)
{
  int ret = 0;
  struct nsm_msg_qos_release msg;

  pal_mem_set (&msg, 0, sizeof (struct nsm_msg_qos_release));

  ret = nsm_decode_qos_release (pnt, size, &msg);
  if (ret < 0)
    return ret;

  if (callback)
    {
      ret = (*callback) (header, arg, &msg);
      if (ret < 0)
        return ret;
    }
  return 0;
}

s_int32_t
nsm_parse_qos_clean (u_char **pnt, u_int16_t *size,
                     struct nsm_msg_header *header, void *arg,
                     NSM_CALLBACK callback)
{
  int ret;
  struct nsm_msg_qos_clean msg;

  pal_mem_set (&msg, 0, sizeof (struct nsm_msg_qos_clean));

  ret = nsm_decode_qos_clean (pnt, size, &msg);
  if (ret < 0)
    return ret;

  if (callback)
    {
      ret = (*callback) (header, arg, &msg);
      if (ret < 0)
        return ret;
    }
  return 0;
}


s_int32_t
nsm_parse_admin_group (u_char **pnt, u_int16_t *size,
                       struct nsm_msg_header *header, void *arg,
                       NSM_CALLBACK callback)
{
  int ret = 0;
  struct nsm_msg_admin_group msg;

  pal_mem_set (&msg, 0, sizeof (struct nsm_msg_admin_group));

  ret = nsm_decode_admin_group (pnt, size, &msg);
  if (ret < 0)
    return ret;

  if (callback)
    {
      ret = (*callback) (header, arg, &msg);
      if (ret < 0)
        return ret;
    }
  return 0;
}
#endif /* HAVE_TE */

#ifdef HAVE_GMPLS
#ifdef HAVE_PACKET
s_int32_t
nsm_parse_gmpls_qos_client_init (u_char **pnt, u_int16_t *size,
                                 struct nsm_msg_header *header, void *arg,
                                 NSM_CALLBACK callback)
{
  return nsm_parse_qos_client_init (pnt, size, header, arg, callback);
}

s_int32_t
nsm_parse_gmpls_qos (u_char **pnt, u_int16_t *size,
                     struct nsm_msg_header *header, void *arg,
                     NSM_CALLBACK callback)
{
  int ret = 0;
  struct nsm_msg_gmpls_qos msg;

  pal_mem_set (&msg, 0, sizeof (struct nsm_msg_gmpls_qos));

  ret = nsm_decode_gmpls_qos (pnt, size, &msg);
  if (ret < 0)
    return ret;

  if (callback)
    {
      ret = (*callback) (header, arg, &msg);
      if (ret < 0)
        return ret;
    }
  return 0;
}

s_int32_t
nsm_parse_gmpls_qos_release (u_char **pnt, u_int16_t *size,
                             struct nsm_msg_header *header, void *arg,
                             NSM_CALLBACK callback)
{
  int ret = 0;
  struct nsm_msg_gmpls_qos_release msg;

  pal_mem_set (&msg, 0, sizeof (struct nsm_msg_gmpls_qos_release));

  ret = nsm_decode_gmpls_qos_release (pnt, size, &msg);
  if (ret < 0)
    return ret;

  if (callback)
    {
      ret = (*callback) (header, arg, &msg);
      if (ret < 0)
        return ret;
    }
  return 0;
}

s_int32_t
nsm_parse_gmpls_qos_clean (u_char **pnt, u_int16_t *size,
                           struct nsm_msg_header *header, void *arg,
                           NSM_CALLBACK callback)
{
  int ret;
  struct nsm_msg_gmpls_qos_clean msg;

  pal_mem_set (&msg, 0, sizeof (struct nsm_msg_gmpls_qos_clean));

  ret = nsm_decode_gmpls_qos_clean (pnt, size, &msg);
  if (ret < 0)
    return ret;

  if (callback)
    {
      ret = (*callback) (header, arg, &msg);
      if (ret < 0)
        return ret;
    }
  return 0;
}

#endif /*HAVE_PACKET */
#endif /* HAVE_GMPLS */

#ifdef HAVE_MPLS_VC
s_int32_t
nsm_parse_mpls_vc (u_char **pnt, u_int16_t *size,
                   struct nsm_msg_header *header, void *arg,
                   NSM_CALLBACK callback)
{
  int ret = 0;
  struct nsm_msg_mpls_vc msg;

  pal_mem_set (&msg, 0, sizeof (struct nsm_msg_mpls_vc));

  ret = nsm_decode_mpls_vc (pnt, size, &msg);
  if (ret < 0)
    return ret;

  if (callback)
    {
      ret = (*callback) (header, arg, &msg);
      if (ret < 0)
        return ret;
    }
  return 0;
}
#endif /* HAVE_MPLS_VC */

s_int32_t
nsm_parse_router_id (u_char **pnt, u_int16_t *size,
                     struct nsm_msg_header *header, void *arg,
                     NSM_CALLBACK callback)
{
  int ret = -1;
  struct nsm_msg_router_id msg;

  pal_mem_set (&msg, 0, sizeof (struct nsm_msg_router_id));

  ret = nsm_decode_router_id (pnt, size, &msg);
  if (ret < 0)
    return ret;

  if (callback)
    {
      ret = (*callback) (header, arg, &msg);
      if (ret < 0)
        return ret;
    }
  return 0;
}

#ifdef HAVE_MPLS
s_int32_t
nsm_parse_ftn_ipv4 (u_char **pnt, u_int16_t *size,
                    struct nsm_msg_header *header, void *arg,
                    NSM_CALLBACK callback)
{
  int ret;
  struct ftn_add_data msg;

  pal_mem_set (&msg, 0, sizeof (struct ftn_add_data));

  ret = nsm_decode_ftn_data_ipv4 (pnt, size, &msg);
  if (ret < 0)
    return ret;

  ret = 0;

  if (callback)
    ret = (*callback) (header, arg, &msg);

  if (msg.sz_desc)
    XFREE (MTYPE_TMP, msg.sz_desc);
  if (msg.pri_fec_prefix)
    prefix_free (msg.pri_fec_prefix);

  return ret;
}


#ifdef HAVE_IPV6
s_int32_t
nsm_parse_ftn_ipv6 (u_char **pnt, u_int16_t *size,
                    struct nsm_msg_header *header, void *arg,
                    NSM_CALLBACK callback)
{
  int ret;
  struct ftn_add_data msg;

  pal_mem_set (&msg, 0, sizeof (struct ftn_add_data));

  ret = nsm_decode_ftn_data_ipv6 (pnt, size, &msg);
  if (ret < 0)
    return ret;

  ret = 0;

  if (callback)
    ret = (*callback) (header, arg, &msg);

  if (msg.sz_desc)
    XFREE (MTYPE_TMP, msg.sz_desc);
  if (msg.pri_fec_prefix)
    prefix_free (msg.pri_fec_prefix);

  return ret;
}
#endif /* HAVE_IPV6 */

#ifdef HAVE_MPLS
s_int32_t
nsm_parse_ftn_gen_data (u_char **pnt, u_int16_t *size,
                        struct nsm_msg_header *header, void *arg,
                        NSM_CALLBACK callback)
{
  int ret;
  struct ftn_add_gen_data msg;

  pal_mem_set (&msg, 0, sizeof (struct ftn_add_gen_data));

  ret = nsm_decode_gmpls_ftn_data (pnt, size, &msg);
  if (ret < 0)
    return ret;

  ret = 0;

  if (callback)
    ret = (*callback) (header, arg, &msg);

  if (msg.sz_desc)
    XFREE (MTYPE_TMP, msg.sz_desc);
  if (msg.pri_fec_prefix)
    prefix_free (msg.pri_fec_prefix);

  return ret;
}

#endif /* HAVE_MPLS */
s_int32_t
nsm_parse_mpls_notification (u_char **pnt, u_int16_t *size,
                             struct nsm_msg_header *header, void *arg,
                             NSM_CALLBACK callback)
{
  int ret;
  struct mpls_notification msg;

  pal_mem_set (&msg, 0, sizeof (struct mpls_notification));

  ret = nsm_decode_mpls_notification (pnt, size, &msg);
  if (ret < 0)
    return ret;

  if (callback)
    {
      ret = (*callback) (header, arg, &msg);
      if (ret < 0)
        return ret;
    }
  return 0;
}

s_int32_t
nsm_parse_igp_shortcut_lsp (u_char **pnt, u_int16_t *size,
                            struct nsm_msg_header *header, void *arg,
                            NSM_CALLBACK callback)
{
  int ret = 0;
  struct nsm_msg_igp_shortcut_lsp msg;

  pal_mem_set (&msg, 0, sizeof (struct nsm_msg_igp_shortcut_lsp));

  ret = nsm_decode_igp_shortcut_lsp (pnt, size, &msg);
  if (ret < 0)
    return ret;

  if (callback)
    {
      ret = (*callback) (header, arg, &msg);
      if (ret < 0)
        return ret;
    }
  return 0;
}

#ifdef HAVE_GMPLS
s_int32_t
nsm_ftn_bidir_reply_callback (struct nsm_msg_header *header,
                              void *arg, void *message)
{
  int ret;
  struct nsm_client *nc;
  struct nsm_client_handler *nch;
  struct ftn_bidir_ret_data *frd = message;
  NSM_CALLBACK callback;

  nch = arg;
  nc = nch->nc;

  callback = nc->callback [NSM_MSG_MPLS_BIDIR_FTN_REPLY];

  if (callback)
    {
      ret = (*callback) (header, arg, frd);
      if (ret < 0)
        return ret;
    }

  return 0;
}

s_int32_t
nsm_ilm_bidir_reply_callback (struct nsm_msg_header *header,
                              void *arg, void *message)
{
  int ret;
  struct nsm_client *nc;
  struct nsm_client_handler *nch;
  struct ilm_bidir_ret_data *frd = message;
  NSM_CALLBACK callback;

  nch = arg;
  nc = nch->nc;

  callback = nc->callback [NSM_MSG_MPLS_BIDIR_ILM_REPLY];

  if (callback)
    {
      ret = (*callback) (header, arg, frd);
      if (ret < 0)
        return ret;
    }

  return 0;
}

#endif /* HAVE_GMPLS */

s_int32_t
nsm_ftn_reply_callback (struct nsm_msg_header *header,
                        void *arg, void *message)
{
  int ret;
  struct nsm_client *nc;
  struct nsm_client_handler *nch;
  struct ftn_ret_data *frd = message;
  NSM_CALLBACK callback;

  nch = arg;
  nc = nch->nc;

  if (CHECK_FLAG (frd->flags, NSM_MSG_REPLY_TO_FTN_ADD))
    callback = nc->callback [NSM_MSG_MPLS_FTN_ADD_REPLY];
  else
    callback = nc->callback [NSM_MSG_MPLS_FTN_DEL_REPLY];

  if (callback)
    {
      ret = (*callback) (header, arg, frd);
      if (ret < 0)
        return ret;
    }

  return 0;
}

s_int32_t
nsm_parse_ftn_reply (u_char **pnt, u_int16_t *size,
                     struct nsm_msg_header *header, void *arg,
                     NSM_CALLBACK callback)
{
  int ret;
  struct ftn_ret_data msg;

  pal_mem_set (&msg, 0, sizeof (struct ftn_ret_data));
  ret = nsm_decode_ftn_ret_data (pnt, size, &msg);
  if (ret < 0)
    return ret;

  if (callback)
    {
      ret = (*callback) (header, arg, &msg);
      if (ret < 0)
        return ret;
    }

  return 0;
}

#ifdef HAVE_BFD
int
nsm_parse_ftn_down (u_char **pnt, u_int16_t *size,
                     struct nsm_msg_header *header, void *arg,
                     NSM_CALLBACK callback)
{
  int ret;
  struct ftn_down_data msg;

  pal_mem_set (&msg, 0, sizeof (struct ftn_down_data));
  ret = nsm_decode_ftn_down_data (pnt, size, &msg);
  if (ret < 0)
    return ret;

  if (callback)
    {
      ret = (*callback) (header, arg, &msg);
      if (ret < 0)
        return ret;
    }

  return 0;
}
#endif /* HAVE_BFD */

s_int32_t
nsm_parse_ilm_ipv4 (u_char **pnt, u_int16_t *size,
                    struct nsm_msg_header *header, void *arg,
                    NSM_CALLBACK callback)
{
  int ret;
  struct ilm_add_data msg;

  pal_mem_set (&msg, 0, sizeof (struct ilm_add_data));

  ret = nsm_decode_ilm_data_ipv4 (pnt, size, &msg);
  if (ret < 0)
    return ret;

  if (callback)
    {
      ret = (*callback) (header, arg, &msg);
      if (ret < 0)
        return ret;
    }
  return 0;
}

#ifdef HAVE_MPLS
s_int32_t
nsm_parse_ilm_gen_data (u_char **pnt, u_int16_t *size,
                        struct nsm_msg_header *header, void *arg,
                        NSM_CALLBACK callback)
{
  int ret;
  struct ilm_add_gen_data msg;

  pal_mem_set (&msg, 0, sizeof (struct ilm_add_gen_data));

  ret = nsm_decode_gmpls_ilm_data (pnt, size, &msg);
  if (ret < 0)
    return ret;

  if (callback)
    {
      ret = (*callback) (header, arg, &msg);
      if (ret < 0)
        return ret;
    }
  return 0;
}
#endif /* HAVE_MPLS */

#ifdef HAVE_IPV6
s_int32_t
nsm_parse_ilm_ipv6 (u_char **pnt, u_int16_t *size,
                    struct nsm_msg_header *header, void *arg,
                    NSM_CALLBACK callback)
{
  int ret;
  struct ilm_add_data msg;

  pal_mem_set (&msg, 0, sizeof (struct ilm_add_data));

  ret = nsm_decode_ilm_data_ipv6 (pnt, size, &msg);
  if (ret < 0)
    return ret;

  if (callback)
    {
      ret = (*callback) (header, arg, &msg);
      if (ret < 0)
        return ret;
    }
  return 0;
}
#endif

s_int32_t
nsm_ilm_reply_callback (struct nsm_msg_header *header,
                        void *arg, void *message)
{
  int ret;
  struct nsm_client *nc;
  struct nsm_client_handler *nch;
  struct ilm_ret_data *ird = message;
  NSM_CALLBACK callback;

  nch = arg;
  nc = nch->nc;

  if (CHECK_FLAG (ird->flags, NSM_MSG_REPLY_TO_ILM_ADD))
    callback = nc->callback [NSM_MSG_MPLS_ILM_ADD_REPLY];
  else
    callback = nc->callback [NSM_MSG_MPLS_ILM_DEL_REPLY];

  if (callback)
    {
      ret = (*callback) (header, arg, ird);
      if (ret < 0)
        return ret;
    }

  return 0;
}

s_int32_t
nsm_ilm_reply_gen_callback (struct nsm_msg_header *header,
                            void *arg, void *message)
{
  int ret;
  struct nsm_client *nc;
  struct nsm_client_handler *nch;
  struct ilm_gen_ret_data *ird = message;
  NSM_CALLBACK callback;

  nch = arg;
  nc = nch->nc;

  if (CHECK_FLAG (ird->flags, NSM_MSG_REPLY_TO_GEN_ILM_ADD))
    callback = nc->callback [NSM_MSG_MPLS_ILM_GEN_ADD_REPLY];
  else
    callback = nc->callback [NSM_MSG_MPLS_ILM_GEN_DEL_REPLY];

  if (callback)
    {
      ret = (*callback) (header, arg, ird);
      if (ret < 0)
        return ret;
    }

  return 0;
}

s_int32_t
nsm_parse_ilm_reply (u_char **pnt, u_int16_t *size,
                     struct nsm_msg_header *header, void *arg,
                     NSM_CALLBACK callback)
{
  int ret;
  struct ilm_ret_data msg;

  pal_mem_set (&msg, 0, sizeof (struct ilm_ret_data));
  ret = nsm_decode_ilm_ret_data (pnt, size, &msg);
  if (ret < 0)
    return ret;

  if (callback)
    {
      ret = (*callback) (header, arg, &msg);
      if (ret < 0)
        return ret;
    }

  return 0;
}

s_int32_t
nsm_parse_ilm_gen_reply (u_char **pnt, u_int16_t *size,
                         struct nsm_msg_header *header, void *arg,
                         NSM_CALLBACK callback)
{
  int ret;
  struct ilm_gen_ret_data msg;

  pal_mem_set (&msg, 0, sizeof (struct ilm_gen_ret_data));
  ret = nsm_decode_ilm_gen_ret_data (pnt, size, &msg);
  if (ret < 0)
    return ret;

  if (callback)
    {
      ret = (*callback) (header, arg, &msg);
      if (ret < 0)
        return ret;
    }

  return 0;
}

#ifdef HAVE_GMPLS
s_int32_t
nsm_parse_ilm_bidir_reply (u_char **pnt, u_int16_t *size,
                           struct nsm_msg_header *header, void *arg,
                           NSM_CALLBACK callback)
{
  int ret;
  struct ilm_bidir_ret_data msg;

  pal_mem_set (&msg, 0, sizeof (struct ilm_bidir_ret_data));
  ret = nsm_decode_ilm_bidir_ret_data (pnt, size, &msg);
  if (ret < 0)
    return ret;

  if (callback)
    {
      ret = (*callback) (header, arg, &msg);
      if (ret < 0)
        return ret;
    }

  return 0;
}

s_int32_t
nsm_parse_ftn_bidir_reply (u_char **pnt, u_int16_t *size,
                           struct nsm_msg_header *header, void *arg,
                           NSM_CALLBACK callback)
{
  int ret;
  struct ftn_bidir_ret_data msg;

  pal_mem_set (&msg, 0, sizeof (struct ftn_bidir_ret_data));
  ret = nsm_decode_ftn_bidir_ret_data (pnt, size, &msg);
  if (ret < 0)
    return ret;

  if (callback)
    {
      ret = (*callback) (header, arg, &msg);
      if (ret < 0)
        return ret;
    }

  return 0;
}
#endif /* HAVE_GMPLS */

s_int32_t
nsm_parse_igp_shortcut_route (u_char **pnt, u_int16_t *size,
                              struct nsm_msg_header *header, void *arg,
                              NSM_CALLBACK callback)
{
  int ret;
  struct nsm_msg_igp_shortcut_route msg;

  pal_mem_set (&msg, 0, sizeof (struct nsm_msg_igp_shortcut_route));
  ret = nsm_decode_igp_shortcut_route (pnt, size, &msg);
  if (ret < 0)
    return ret;

  if (callback)
    {
      ret = (*callback) (header, arg, &msg);
      if (ret < 0)
        return ret;
    }

  return 0;
}

#if defined HAVE_MPLS_OAM || defined HAVE_NSM_MPLS_OAM
s_int32_t
nsm_parse_mpls_vpn_rd (u_char **pnt, u_int16_t *size,
                       struct nsm_msg_header *header, void *arg,
                       NSM_CALLBACK callback)
{

  int ret;
  struct nsm_msg_vpn_rd msg;

  pal_mem_set (&msg, 0, sizeof (struct nsm_msg_vpn_rd));
  ret = nsm_decode_mpls_vpn_rd (pnt, size, &msg);
  if (ret < 0)
    return ret;

  if (callback)
    {
      ret = (*callback) (header, arg, &msg);
      if (ret < 0)
        return ret;
    }

  return 0;
}
#endif /* HAVE_MPLS_OAM || HAVE_NSM_MPLS_OAM */
#endif /* HAVE_MPLS */

#ifdef HAVE_MPLS_VC
/* Dump VC FIB add Message */
void
nsm_vc_fib_add_dump (struct lib_globals *zg,
                       struct nsm_msg_vc_fib_add *msg)
{
  zlog_info (zg, " VC FIB ADD IPv4");
  zlog_info (zg, " VPN id: %u", msg->vpn_id);
  zlog_info (zg, " VC id: %u", msg->vc_id);
  zlog_info (zg, " VC Style: %u", msg->vc_style);
  zlog_info (zg, " Incoming Label: %u", msg->in_label);
  zlog_info (zg, " Outgoing Label: %u", msg->out_label);
  zlog_info (zg, " Access Interface Index: %u", msg->ac_if_ix);
  zlog_info (zg, " Network Interface Index: %u", msg->nw_if_ix);
  zlog_info (zg, " Remote Group Id: %u", msg->remote_grp_id);
  zlog_info (zg, " PW Status Local & Remote: ");
  nsm_mpls_pw_status_dump (zg, msg->remote_pw_status);

  if (msg->vc_style == MPLS_VC_STYLE_VPLS_MESH)
    {
      if (msg->addr.afi == AFI_IP)
        zlog_info (zg, " Peer: %r", &msg->addr.u.ipv4);
#ifdef HAVE_IPV6
      else if (msg->addr.afi == AFI_IP6)
        zlog_info (zg, " Peer: %R", &msg->addr.u.ipv6);
#endif /* HAVE_IPV6 */
    }
}

/* Dump VC FID delete message */
void
nsm_vc_fib_delete_dump (struct lib_globals *zg,
                          struct nsm_msg_vc_fib_delete *msg)
{
  zlog_info (zg, " VC FIB DEL IPv4");
  zlog_info (zg, " VPN id: %u", msg->vpn_id);
  zlog_info (zg, " VC id: %u", msg->vc_id);
  zlog_info (zg, " VC Style: %u\n", msg->vc_style);
  if (msg->vc_style == MPLS_VC_STYLE_VPLS_MESH)
    {
      if (msg->addr.afi == AFI_IP)
        zlog_info (zg, " Peer: %r", &msg->addr.u.ipv4);
#ifdef HAVE_IPV6
      else if (msg->addr.afi == AFI_IP6)
        zlog_info (zg, " Peer: %R", &msg->addr.u.ipv6);
#endif /* HAVE_IPV6 */
    }
}


/* Parse the Fib add/del messages */
s_int32_t
nsm_parse_vc_fib_add (u_char **pnt, u_int16_t *size,
                        struct nsm_msg_header *header, void *arg,
                        NSM_CALLBACK callback)
{
  int ret;
  struct nsm_msg_vc_fib_add msg;

  pal_mem_set (&msg, 0, sizeof (struct nsm_msg_vc_fib_add));
  ret = nsm_decode_vc_fib_add (pnt, size, &msg);
  if (ret < 0)
    return ret;

  if (callback)
    {
      ret = (*callback) (header, arg, &msg);
      if (ret < 0)
        return ret;
    }

  return 0;
}

s_int32_t
nsm_parse_vc_tunnel_info (u_char **pnt, u_int16_t *size,
                          struct nsm_msg_header *header, void *arg,
                          NSM_CALLBACK callback)
{
  int ret;
  struct nsm_msg_vc_tunnel_info msg;

  pal_mem_set (&msg, 0, sizeof (struct nsm_msg_vc_tunnel_info));
  ret = nsm_decode_vc_tunnel_info (pnt, size, &msg);
  if (ret < 0)
    return ret;

  if (callback)
    {
      ret = (*callback) (header, arg, &msg);
      if (ret < 0)
        return ret;
    }

  return 0;
}

s_int32_t
nsm_parse_pw_status (u_char **pnt, u_int16_t *size,
                     struct nsm_msg_header *header, void *arg,
                     NSM_CALLBACK callback)
{
  int ret;
  struct nsm_msg_pw_status msg;

  pal_mem_set (&msg, 0, sizeof (msg));
  ret = nsm_decode_pw_status (pnt, size, &msg);
  if (ret < 0)
    return ret;

  if (callback)
    {
      ret = (*callback) (header, arg, &msg);
      if (ret < 0)
        return ret;
    }

  return 0;
}

s_int32_t
nsm_parse_vc_fib_delete (u_char **pnt, u_int16_t *size,
                         struct nsm_msg_header *header, void *arg,
                         NSM_CALLBACK callback)
{
  int ret;
  struct nsm_msg_vc_fib_delete msg;

  pal_mem_set (&msg, 0, sizeof (struct nsm_msg_vc_fib_delete));
  ret = nsm_decode_vc_fib_delete (pnt, size, &msg);
  if (ret < 0)
    return ret;

  if (callback)
    {
      ret = (*callback) (header, arg, &msg);
      if (ret < 0)
        return ret;
    }

  return 0;
}

#ifdef HAVE_MS_PW
/**@brief Function to parse the mspw message.
    @param **pnt    - pointer to the message buffer.
    @param *size    - pointer to the size of message buffer.
    @param *header  - pointer to the header of mspw message.
    @param *arg     - pointer to the list of arguments
    @param callback - callback function to execute on successful decoding.
    @return         - length of the encoded message.
*/
int
nsm_parse_mpls_ms_pw_msg (u_char **pnt, u_int16_t *size,
                          struct nsm_msg_header *header, void *arg,
                          NSM_CALLBACK callback)
{
  int ret;
  struct nsm_msg_mpls_ms_pw_msg msg;

  pal_mem_set(&msg, 0, sizeof (struct nsm_msg_mpls_ms_pw_msg));
  ret = nsm_decode_mpls_ms_pw_msg (pnt, size, &msg);
  if (ret < 0)
    return ret;

  if (callback)
    {
      ret = (*callback) (header, arg, &msg);
      if (ret < 0)
        return ret;
    }

  return 0;
}
#endif /* HAVE_MS_PW */
#endif /* HAVE_MPLS_VC */

#ifdef HAVE_GMPLS
#if 0 /*TBD OLD CODE */
s_int32_t
nsm_parse_gmpls_if (u_char **pnt, u_int16_t *size,
                    struct nsm_msg_header *header, void *arg,
                    NSM_CALLBACK callback)
{
  int ret = 0;
  struct nsm_msg_gmpls_if msg;

  pal_mem_set (&msg, 0, sizeof (struct nsm_msg_gmpls_if));

  ret = nsm_decode_gmpls_if (pnt, size, &msg);
  if (ret < 0)
    return ret;

  if (callback)
    ret = (*callback) (header, arg, &msg);

  /* Free up allocated memory. */
  gmpls_srlg_free (msg.srlg);

  return ret;
}
#endif
#endif /* HAVE_GMPLS */

#ifdef HAVE_CRX
s_int32_t
nsm_parse_route_clean (u_char **pnt, u_int16_t *size,
                       struct nsm_msg_header *header, void *arg,
                       NSM_CALLBACK callback)
{
  int ret = -1;
  struct nsm_msg_route_clean msg;

  pal_mem_set (&msg, 0, sizeof (struct nsm_msg_route_clean));

  ret = nsm_decode_route_clean (pnt, size, &msg);
  if (ret < 0)
    return ret;

  if (callback)
    {
      ret = (*callback) (header, arg, &msg);
      if (ret < 0)
        return ret;
    }

  return 0;
}

s_int32_t
nsm_parse_protocol_restart (u_char **pnt, u_int16_t *size,
                            struct nsm_msg_header *header, void *arg,
                            NSM_CALLBACK callback)
{
 int ret = -1;
 struct nsm_msg_protocol_restart msg;

  pal_mem_set (&msg, 0, sizeof (struct nsm_msg_protocol_restart));

  ret = nsm_decode_protocol_restart (pnt, size, &msg);
  if (ret < 0)
    return ret;

  if (callback)
    {
      ret = (*callback) (header, arg, &msg);
      if (ret < 0)
        return ret;
    }

  return 0;
}
#endif /* HAVE_CRX */

#ifdef HAVE_DIFFSERV

/* Encode DiffServ supported dscp update data.  */
s_int32_t
nsm_encode_supported_dscp_update (u_char **pnt, u_int16_t *size,
                                  struct nsm_msg_supported_dscp_update *msg)
{
  u_char *sp = *pnt;

  /* Encode dscp value.  */
  TLV_ENCODE_PUTC (msg->dscp);

  /* Encode "supported or not" value. */
  TLV_ENCODE_PUTC (msg->support);

  return *pnt - sp;
}

/* Encode DiffServ exp dscp update data.  */
s_int32_t
nsm_encode_dscp_exp_map_update (u_char **pnt, u_int16_t *size,
                                struct nsm_msg_dscp_exp_map_update *msg)
{
  u_char *sp = *pnt;

  /* Encode exp value.  */
  TLV_ENCODE_PUTC (msg->exp);

  /* Encode dscp value. */
  TLV_ENCODE_PUTC (msg->dscp);

  return *pnt - sp;
}


s_int32_t
nsm_decode_supported_dscp_update (u_char **pnt, u_int16_t *size,
                                  struct nsm_msg_supported_dscp_update *msg)
{
  u_char *sp = *pnt;

  /* DSCP value.  */
  TLV_DECODE_GETC (msg->dscp);

  /* Supported or not.  */
  TLV_DECODE_GETC (msg->support);

  return *pnt - sp;
}

s_int32_t
nsm_decode_dscp_exp_map_update (u_char **pnt, u_int16_t *size,
                                struct nsm_msg_dscp_exp_map_update *msg)
{
  u_char *sp = *pnt;

  /* EXP value.  */
  TLV_DECODE_GETC (msg->exp);

  /* DSCP value.  */
  TLV_DECODE_GETC (msg->dscp);

  return *pnt - sp;
}


/* Parse Diffserv Supported DSCP message.  */
s_int32_t
nsm_parse_supported_dscp_update (u_char **pnt, u_int16_t *size,
                                 struct nsm_msg_header *header, void *arg,
                                 NSM_CALLBACK callback)
{
  int ret;
  struct nsm_msg_supported_dscp_update msg;

  ret = nsm_decode_supported_dscp_update (pnt, size, &msg);
  if (ret < 0)
    return ret;

  if (callback)
    {
      ret = (*callback) (header, arg, &msg);
      if (ret < 0)
        return ret;
    }
  return 0;
}

/* Parse Diffserv DSCP EXP map message.  */
s_int32_t
nsm_parse_dscp_exp_map_update (u_char **pnt, u_int16_t *size,
                               struct nsm_msg_header *header, void *arg,
                               NSM_CALLBACK callback)
{
  int ret;
  struct nsm_msg_dscp_exp_map_update msg;

  ret = nsm_decode_dscp_exp_map_update (pnt, size, &msg);
  if (ret < 0)
    return ret;

  if (callback)
    {
      ret = (*callback) (header, arg, &msg);
      if (ret < 0)
        return ret;
    }
  return 0;
}

void
nsm_supported_dscp_update_dump (struct lib_globals *zg,
                                struct nsm_msg_supported_dscp_update *msg)
{
  zlog_info (zg, "DSCP value %x: %s", msg->dscp,
             msg->support ? "supported" : "unsupported");
}

void
nsm_dscp_exp_map_update_dump (struct lib_globals *zg,
                              struct nsm_msg_dscp_exp_map_update *msg)
{
  zlog_info (zg, "DSCP value %x is mapped to exp value %d", msg->dscp,
             msg->exp);
}

#endif /* HAVE_DIFFSERV */

#ifdef HAVE_DSTE
void
nsm_dste_te_class_update_dump (struct lib_globals *zg,
                               struct nsm_msg_dste_te_class_update *msg)
{
  zlog_info (zg, "DSTE class type update:");
  zlog_info (zg, "  TE index: %d", msg->te_class_index);
  zlog_info (zg, "  Class Type: %d", msg->te_class.ct_num);
  zlog_info (zg, "  Preemption priority: %d", msg->te_class.priority);
  zlog_info (zg, "  Class Type name: %s", msg->ct_name);
}
#endif /* HAVE_DSTE */

#ifdef HAVE_VPLS
s_int32_t
nsm_parse_vpls_mac_withdraw (u_char **pnt, u_int16_t *size,
                             struct nsm_msg_header *header, void *arg,
                             NSM_CALLBACK callback)
{
  struct nsm_msg_vpls_mac_withdraw msg;
  int ret;

  ret = nsm_decode_vpls_mac_withdraw (pnt, size, &msg);
  if (ret < 0)
    return ret;

  if (callback)
    {
      ret = (*callback) (header, arg, &msg);
      if (ret < 0)
        {
          if (msg.mac_addrs)
            XFREE (MTYPE_TMP, msg.mac_addrs);
          return ret;
        }
    }

  if (msg.mac_addrs)
    XFREE (MTYPE_TMP, msg.mac_addrs);

  return 0;
}


s_int32_t
nsm_parse_vpls_add (u_char **pnt, u_int16_t *size,
                    struct nsm_msg_header *header, void *arg,
                    NSM_CALLBACK callback)
{
  struct nsm_msg_vpls_add msg;
  int ret;
  int retval = 0;

  ret = nsm_decode_vpls_add (pnt, size, &msg);
  if (ret < 0)
    {
      retval = ret;
      goto vad_cleanup;
    }

  if (callback)
    {
      ret = (*callback) (header, arg, &msg);
      if (ret < 0)
        {
          retval = ret;
          goto vad_cleanup;
        }
    }

 vad_cleanup:
  if (msg.peers_added)
    XFREE (MTYPE_TMP, msg.peers_added);

  if (msg.peers_deleted)
    XFREE (MTYPE_TMP, msg.peers_deleted);

  return retval;
}

s_int32_t
nsm_parse_vpls_delete (u_char **pnt, u_int16_t *size,
                       struct nsm_msg_header *header, void *arg,
                       NSM_CALLBACK callback)
{
  u_int32_t vpls_id;
  int ret;

  ret = nsm_decode_vpls_delete (pnt, size, &vpls_id);
  if (ret < 0)
    return ret;

  if (callback)
    {
      ret = (*callback) (header, arg, &vpls_id);
      if (ret < 0)
        return ret;
    }

  return 0;
}

#endif /* HAVE_VPLS */

#ifdef HAVE_LACPD
s_int32_t
nsm_parse_lacp_aggregator_add (u_char **pnt, u_int16_t *size,
                               struct nsm_msg_header *header, void *arg,
                               NSM_CALLBACK callback)
{
  struct nsm_msg_lacp_agg_add msg;
  int ret = 0;

  ret = nsm_decode_lacp_aggregator_add (pnt, size, &msg);
  if (ret < 0)
    {
      goto laa_cleanup;
    }

  if (callback)
    ret = (*callback) (header, arg, &msg);

 laa_cleanup:
  if (msg.ports_added)
    XFREE (MTYPE_TMP, msg.ports_added);

  if (msg.ports_deleted)
    XFREE (MTYPE_TMP, msg.ports_deleted);

  return ret;
}

s_int32_t
nsm_parse_lacp_aggregator_del (u_char **pnt, u_int16_t *size,
                               struct nsm_msg_header *header, void *arg,
                               NSM_CALLBACK callback)
{
  char agg_name[LACP_AGG_NAME_LEN + 1];
  int ret;

  pal_mem_set (agg_name, '\0', LACP_AGG_NAME_LEN + 1);
  ret = nsm_decode_lacp_aggregator_del (pnt, size, agg_name);
  if (ret < 0)
    return ret;

  if (callback)
    {
      ret = (*callback) (header, arg, agg_name);
      if (ret < 0)
        return ret;
    }

  return 0;
}

/* Parse function for message from NSM regarding the addition/deletion of */
/* static aggregator - to update the aggregator count in LACP*/
s_int32_t
nsm_parse_static_agg_cnt_update (u_char **pnt, u_int16_t *size,
                    struct nsm_msg_header *header, void *arg,
                    NSM_CALLBACK callback)
{
  int ret = 0;
  struct nsm_msg_agg_cnt msg;

  ret = nsm_decode_static_agg_cnt (pnt, size, &msg);
  if (ret < 0)
    return ret;

  if (callback)
    {
      ret = (*callback) (header, arg, &msg);
      if (ret < 0)
        return ret;
    }
  return ret;
}

s_int32_t
nsm_parse_lacp_send_agg_config (u_char **pnt, u_int16_t *size,
                    struct nsm_msg_header *header, void *arg,
                    NSM_CALLBACK callback)
{
  int ret = 0;
  struct nsm_msg_lacp_aggregator_config msg;

  ret = nsm_decode_lacp_send_agg_config (pnt, size, &msg);
  if (ret < 0)
    return ret;

  if (callback)
    {
      ret = (*callback) (header, arg, &msg);
      if (ret < 0)
        return ret;
    }
  return ret;

}
#endif /* HAVE_LACP */

#ifdef HAVE_L2
/* Parse bridge message. */
s_int32_t
nsm_parse_bridge_msg (u_char **pnt, u_int16_t *size,
                      struct nsm_msg_header *header, void *arg,
                      NSM_CALLBACK callback)
{
  struct nsm_msg_bridge msg;
  int ret;

  ret = nsm_decode_bridge_msg (pnt, size, &msg);
  if (ret < 0)
    return ret;

  if (callback)
    {
      ret = (*callback) (header, arg, &msg);
      if (ret < 0)
        return ret;
    }

  return 0;
}

/* Parse bridge interface message. */
s_int32_t
nsm_parse_bridge_if_msg (u_char **pnt, u_int16_t *size,
                         struct nsm_msg_header *header, void *arg,
                         NSM_CALLBACK callback)
{
  struct nsm_msg_bridge_if msg;
  int ret;

  pal_mem_set (&msg, 0, sizeof (struct nsm_msg_bridge_if));
  ret = nsm_decode_bridge_if_msg (pnt, size, &msg);
  if (ret < 0)
    return ret;

  if (callback)
    {
      ret = (*callback) (header, arg, &msg);
      if (ret < 0)
        return ret;
    }

  return 0;
}

#if defined HAVE_G8031 || defined HAVE_G8032

/* Parse bridge Protection Group message. */
s_int32_t
nsm_parse_bridge_pg_msg (u_char **pnt, u_int16_t *size,
                         struct nsm_msg_header *header, void *arg,
                         NSM_CALLBACK callback)
{
  struct nsm_msg_bridge_pg msg;
  int ret;

  pal_mem_set (&msg, 0, sizeof (struct nsm_msg_bridge_pg));
  ret = nsm_decode_bridge_pg_msg (pnt, size, &msg);
  if (ret < 0)
    return ret;

  if (callback)
    {
      ret = (*callback) (header, arg, &msg);
      if (ret < 0)
        return ret;
    }

  return 0;
}

#endif /* HAVE_G8031 || HAVE_G8032 */

/* Parse bridge port message. */
s_int32_t
nsm_parse_bridge_port_msg (u_char **pnt, u_int16_t *size,
                         struct nsm_msg_header *header, void *arg,
                         NSM_CALLBACK callback)
{
  struct nsm_msg_bridge_port msg;
  int ret;

  pal_mem_set(&msg, 0, sizeof (struct nsm_msg_bridge_port));
  ret = nsm_decode_bridge_port_msg (pnt, size, &msg);
  if (ret < 0)
    return ret;

  if (callback)
    {
      ret = (*callback) (header, arg, &msg);
      if (ret < 0)
        return ret;
    }

  return 0;
}

s_int32_t
nsm_parse_bridge_enable_msg (u_char **pnt, u_int16_t *size,
                             struct nsm_msg_header *header, void *arg,
                             NSM_CALLBACK callback)
{
  struct nsm_msg_bridge_enable msg;
  int ret;

  ret = nsm_decode_bridge_enable_msg (pnt, size, &msg);

  if (ret < 0)
    return ret;

  if (callback)
    {
      ret = (*callback) (header, arg, &msg);
      if (ret < 0)
        return ret;
    }

  return 0;
}

#ifdef HAVE_ONMD
s_int32_t
nsm_parse_cfm_mac_msg (u_char **pnt, u_int16_t *size,
                       struct nsm_msg_header *header, void *arg,
                       NSM_CALLBACK callback)
{
  struct nsm_msg_cfm_mac msg;
  int ret;

  ret = nsm_decode_cfm_mac_msg (pnt, size, &msg);
  if (ret < 0)
    return ret;

  if (callback)
    {
      ret = (*callback) (header, arg, &msg);
      if (ret < 0)
        return ret;
    }

  return 0;
}
#endif /* HAVE_ONMD */

#ifdef HAVE_PBB_TE
s_int32_t
nsm_parse_bridge_pbb_te_port_state_msg (u_char **pnt, u_int16_t *size,
                         struct nsm_msg_header *header, void *arg,
                         NSM_CALLBACK callback)
{
  struct nsm_msg_bridge_pbb_te_port msg;
  int ret;

  ret = nsm_decode_bridge_pbb_te_port_msg (pnt, size, &msg);
  if (ret < 0)
    return ret;

  if (callback)
    {
      ret = (*callback) (header, arg, &msg);
      if (ret < 0)
        return ret;
    }

  return 0;
}
#endif /* HAVE_PBB_TE */

#endif /* HAVE_L2 */

#ifdef HAVE_RMOND
s_int32_t
nsm_parse_rmon_msg (u_char **pnt, u_int16_t *size,
                    struct nsm_msg_header *header, void *arg,
                    NSM_CALLBACK callback)
{
  struct nsm_msg_rmon msg;
  int ret;

  ret = nsm_decode_rmon_msg (pnt, size, &msg);
  if (ret < 0)
    return ret;

  if (callback)
    {
      ret = (*callback) (header, arg, &msg);
      if (ret < 0)
        return ret;
    }

  return 0;
}
#endif /* HAVE_RMON */

#ifndef HAVE_CUSTOM1
#ifdef HAVE_VLAN
/* Parse VLAN message. */
s_int32_t
nsm_parse_vlan_msg (u_char **pnt, u_int16_t *size,
                    struct nsm_msg_header *header, void *arg,
                    NSM_CALLBACK callback)
{
  struct nsm_msg_vlan msg;
  int ret;

  pal_mem_set (&msg, 0, sizeof (struct nsm_msg_vlan));
  ret = nsm_decode_vlan_msg (pnt, size, &msg);
  if (ret < 0)
    return ret;

  if (callback)
    {
      ret = (*callback) (header, arg, &msg);
      if (ret < 0)
        return ret;
    }

  return 0;
}

/* Parse VLAN interface message. */
s_int32_t
nsm_parse_vlan_port_msg (u_char **pnt, u_int16_t *size,
                       struct nsm_msg_header *header, void *arg,
                       NSM_CALLBACK callback)
{
  struct nsm_msg_vlan_port msg;
  int ret;

  pal_mem_set(&msg, 0, sizeof(struct nsm_msg_vlan_port));
  ret = nsm_decode_vlan_port_msg (pnt, size, &msg);
  if (ret < 0)
    return ret;

  if (callback)
    {
      ret = (*callback) (header, arg, &msg);
      if (ret < 0)
        return ret;
    }

  return 0;
}

/* Parse VLAN port type message. */
s_int32_t
nsm_parse_vlan_port_type_msg (u_char **pnt, u_int16_t *size,
                              struct nsm_msg_header *header, void *arg,
                              NSM_CALLBACK callback)
{
  struct nsm_msg_vlan_port_type msg;
  int ret;

  ret = nsm_decode_vlan_port_type_msg (pnt, size, &msg);
  if (ret < 0)
    return ret;

  if (callback)
    {
      ret = (*callback) (header, arg, &msg);
      if (ret < 0)
        return ret;
    }

  return 0;
}

#if (defined HAVE_I_BEB || defined HAVE_B_BEB )

/* Parse svidtopip. */
s_int32_t
nsm_parse_isid2svid_msg (u_char **pnt, u_int16_t *size,
                       struct nsm_msg_header *header, void *arg,
                       NSM_CALLBACK callback)
{
  struct nsm_msg_pbb_isid_to_pip msg;
  int ret;

  ret = nsm_decode_isid2svid_msg (pnt, size, &msg);
  if (ret < 0)
    return ret;

  if (callback)
    {
      ret = (*callback) (header, arg, &msg);
      if (ret < 0)
        return ret;
    }

  return 0;
}

/* Parse svidtobvid. */
s_int32_t
nsm_parse_isid2bvid_msg (u_char **pnt, u_int16_t *size,
                       struct nsm_msg_header *header, void *arg,
                       NSM_CALLBACK callback)
{
  struct nsm_msg_pbb_isid_to_bvid msg;
  int ret;

  ret = nsm_decode_isid2bvid_msg (pnt, size, &msg);
  if (ret < 0)
    return ret;

  if (callback)
    {
      ret = (*callback) (header, arg, &msg);
      if (ret < 0)
        return ret;
    }

  return 0;
}

#endif /* #if (defined HAVE_I_BEB || defined HAVE_B_BEB ) */
#ifdef HAVE_G8032
s_int32_t
nsm_parse_g8032_create_vlan_msg (u_char **pnt, u_int16_t *size,
                                 struct nsm_msg_header *header,
                                 void *arg,
                                 NSM_CALLBACK callback)
{
  struct nsm_msg_g8032_vlan msg;
  int ret;

  ret = nsm_decode_g8032_create_vlan_msg (pnt, size, &msg);
  if (ret < 0)
    return ret;

  if (callback)
    {
      ret = (*callback) (header, arg, &msg);
      if (ret < 0)
        return ret;
    }

  return 0;
}

s_int32_t
nsm_encode_g8032_create_msg ( u_char **pnt, u_int16_t *size,
                              struct nsm_msg_g8032 * msg)
{
  u_char *sp = *pnt;

  /* bridge_name */
  TLV_ENCODE_PUT (msg->bridge_name, NSM_BRIDGE_NAMSIZ);

  /*  RING-ID */
  TLV_ENCODE_PUTW (msg->ring_id);

  /* east_ifindex */
  TLV_ENCODE_PUTL (msg->east_ifindex);

  /* east_ifindex */
  TLV_ENCODE_PUTL (msg->west_ifindex);

  return *pnt - sp;
}

s_int32_t
nsm_decode_g8032_create_msg ( u_char **pnt, u_int16_t *size,
                              struct nsm_msg_g8032 * msg)
{
  u_char *sp = *pnt;

  /* bridge_name */
  TLV_DECODE_GET(msg->bridge_name, NSM_BRIDGE_NAMSIZ);

  /* ring-id */
  TLV_DECODE_GETW(msg->ring_id);

  /* east_ifindex */
  TLV_DECODE_GETL (msg->east_ifindex);

  /* west_ifindex */
  TLV_DECODE_GETL (msg->west_ifindex);

  return *pnt - sp;
}

/* Parse G8032 message. */
s_int32_t
nsm_parse_g8032_create_msg (u_char **pnt, u_int16_t *size,
                       struct nsm_msg_header *header, void *arg,
                       NSM_CALLBACK callback)
{
  struct nsm_msg_g8032 msg;
  int ret;

  ret = nsm_decode_g8032_create_msg (pnt, size, &msg);

  if (ret < 0)
    return ret;

  if (callback)
    {
      ret = (*callback) (header, arg, &msg);
      if (ret < 0)
        return ret;
    }

  return 0;

}

s_int32_t
nsm_parse_g8032_delete_msg (u_char **pnt, u_int16_t *size,
                       struct nsm_msg_header *header, void *arg,
                       NSM_CALLBACK callback)
{
  struct nsm_msg_g8032 msg;
  int ret;

  ret = nsm_decode_g8032_create_msg (pnt, size, &msg);

  if (ret < 0)
    return ret;

  if (callback)
    {
      ret = (*callback) (header, arg, &msg);
      if (ret < 0)
        return ret;
    }

  return 0;

}
s_int32_t
nsm_decode_bridge_g8032_port_msg (u_char **pnt, u_int16_t *size,
                           struct nsm_msg_bridge_g8032_port *msg)
{
  u_char *sp = *pnt;

  /*  Ring-ID */
  TLV_DECODE_GETW(msg->ring_id);

  /*  port state */
  TLV_DECODE_GETC(msg->state);

  /*  FDB flush */
  TLV_DECODE_GETC(msg->fdb_flush);

  /* Bridge name. */
  TLV_DECODE_GET(msg->bridge_name, NSM_BRIDGE_NAMSIZ);

  /* ifindex */
  TLV_DECODE_GETL (msg->ifindex);

  return *pnt - sp;
}
s_int32_t
nsm_encode_g8032_port_msg (u_char **pnt,
    	                   u_int16_t *size,
			   struct nsm_msg_bridge_g8032_port *msg)
{
  u_char *sp = *pnt;
  /* RING-ID */
  TLV_ENCODE_PUTW (msg->ring_id);

  /* portstate */
  TLV_ENCODE_PUTC (msg->state);

  /* to flush FDB or not.  */
  TLV_ENCODE_PUTC (msg->fdb_flush);

  /* bridge_name */
  TLV_ENCODE_PUT (msg->bridge_name, NSM_BRIDGE_NAMSIZ);

  /* Ifindex to be fwd state */
  TLV_ENCODE_PUTL (msg->ifindex);

  return *pnt - sp;
}

/* Parse bridge port message. */
s_int32_t
nsm_parse_bridge_g8032_port_msg (u_char **pnt,
	                         u_int16_t *size,
                                 struct nsm_msg_header *header,
				 void *arg,
                         	 NSM_CALLBACK callback)
{
  struct nsm_msg_bridge_g8032_port msg;
  int ret;

  ret = nsm_decode_bridge_g8032_port_msg (pnt, size, &msg);
  if (ret < 0)
    return ret;

  if (callback)
    {
      ret = (*callback) (header, arg, &msg);
      if (ret < 0)
        return ret;
    }
   return 0;
}
s_int32_t
nsm_decode_g8032_portstate_msg (u_char **pnt, u_int16_t *size,
                               struct nsm_msg_bridge_g8032_port * msg)
{
  u_char *sp = *pnt;

  /* Ring-ID */
  TLV_DECODE_GETW(msg->ring_id);

  /* port state */
  TLV_DECODE_GETC(msg->state);

  /* FDB flush */
  TLV_DECODE_GETC(msg->fdb_flush);

  /* Bridge name. */
  TLV_DECODE_GET(msg->bridge_name, NSM_BRIDGE_NAMSIZ);

  /* ifindex */
  TLV_DECODE_GETL (msg->ifindex);

  return *pnt - sp;
}

s_int32_t
nsm_parse_oam_g8032_portstate_msg(u_char **pnt, u_int16_t *size,
 				      struct nsm_msg_header *header,
                                  void *arg, NSM_CALLBACK callback)
{
  struct nsm_msg_bridge_g8032_port msg;
  int ret;

  ret = nsm_decode_g8032_portstate_msg (pnt, size, &msg);
  if (ret < 0)
    return ret;

  if (callback)
    {
      ret = (*callback) (header, arg, &msg);
      if (ret < 0)
         return ret;
    }

  return 0;
}

#endif /* HAVE_G8032*/

#ifdef HAVE_G8031
s_int32_t
nsm_parse_g8031_create_pg_msg (u_char **pnt, u_int16_t *size,
                               struct nsm_msg_header *header, void *arg,
                               NSM_CALLBACK callback)
{
  struct nsm_msg_g8031_pg msg;
  int ret;

  ret = nsm_decode_g8031_create_pg_msg (pnt, size, &msg);
  if (ret < 0)
    return ret;

  if (callback)
    {
      ret = (*callback) (header, arg, &msg);
      if (ret < 0)
        return ret;
    }

  return 0;
}

s_int32_t
nsm_parse_g8031_create_vlan_msg (u_char **pnt, u_int16_t *size,
                                 struct nsm_msg_header *header, void *arg,
                                 NSM_CALLBACK callback)
{
  struct nsm_msg_g8031_vlanpg msg;
  int ret;

  ret = nsm_decode_g8031_create_vlan_msg (pnt, size, &msg);
  if (ret < 0)
    return ret;

  if (callback)
    {
      ret = (*callback) (header, arg, &msg);
      if (ret < 0)
        return ret;
    }

  return 0;
}
 /* Pg initialization msg to NSM */
s_int32_t
nsm_parse_oam_pg_init_msg (u_char **pnt, u_int16_t *size,
                       struct nsm_msg_header *header, void *arg,
                       NSM_CALLBACK callback)
{
  struct nsm_msg_pg_initialized msg;
  int ret;

  ret = nsm_decode_pg_init_msg (pnt, size, &msg);
  if (ret < 0)
    return ret;

  if (callback)
    {
      ret = (*callback) (header, arg, &msg);
      if (ret < 0)
        return ret;
    }

  return 0;
}

/* Pg Portstate msg to NSM*/
s_int32_t
nsm_parse_oam_g8031_portstate_msg (u_char **pnt, u_int16_t *size,
                                   struct nsm_msg_header *header, void *arg,
                                   NSM_CALLBACK callback)
{
  struct nsm_msg_g8031_portstate msg;
  int ret;

  ret = nsm_decode_g8031_portstate_msg (pnt, size, &msg);
  if (ret < 0)
    return ret;

  if (callback)
    {
      ret = (*callback) (header, arg, &msg);
      if (ret < 0)
        return ret;
    }

  return 0;
}

#endif /* HAVE_G8031 */

#ifdef HAVE_PBB_TE

s_int32_t
nsm_parse_te_vlan_msg (u_char **pnt, u_int16_t *size,
                       struct nsm_msg_header *header, void *arg,
                       NSM_CALLBACK callback)
{
  struct nsm_msg_pbb_te_vlan msg;
  int ret;

  ret = nsm_decode_te_vlan_msg (pnt, size, &msg);
  if (ret < 0)
    return ret;

  if (callback)
    {
      ret = (*callback) (header, arg, &msg);
      if (ret < 0)
        return ret;
    }

  return 0;
}

/* Parse */
s_int32_t
nsm_parse_te_esp_msg (u_char **pnt, u_int16_t *size,
                      struct nsm_msg_header *header, void *arg,
                      NSM_CALLBACK callback)
{
  struct nsm_msg_pbb_te_esp msg;
  int ret;

  ret = nsm_decode_te_esp_msg (pnt, size, &msg);
  if (ret < 0)
    return ret;

  if (callback)
    {
      ret = (*callback) (header, arg, &msg);
      if (ret < 0)
        return ret;
    }

  return 0;
}

s_int32_t
nsm_parse_esp_pnp_msg (u_char **pnt, u_int16_t *size,
                       struct nsm_msg_header *header, void *arg,
                       NSM_CALLBACK callback)
{
  struct nsm_msg_pbb_esp_pnp msg;
  int ret;

  ret = nsm_decode_esp_pnp_msg (pnt, size, &msg);
  if (ret < 0)
    return ret;

  if (callback)
    {
      ret = (*callback) (header, arg, &msg);
      if (ret < 0)
        return ret;
    }

  return 0;
}

s_int32_t
nsm_parse_te_aps_grp(u_char **pnt, u_int16_t *size,
                     struct nsm_msg_header *header, void *arg,
                     NSM_CALLBACK callback)
{
  struct nsm_msg_te_aps_grp msg;
  int ret;

  ret = nsm_decode_te_aps_grp (pnt, size, &msg);
  if (ret < 0)
    return ret;

  if (callback)
    {
      ret = (*callback) (header, arg, &msg);
      if (ret < 0)
        return ret;
    }
  return 0;
}

#endif /* HAVE_PBB_TE */

#ifdef HAVE_PVLAN
int
nsm_parse_pvlan_if_msg (u_char **pnt, u_int16_t *size,
                        struct nsm_msg_header *header, void *arg,
                        NSM_CALLBACK callback)
{
  struct nsm_msg_pvlan_if msg;
  int ret;

  ret = nsm_decode_pvlan_if_msg (pnt, size, &msg);
  if (ret < 0)
    return ret;

  if (callback)
    {
      ret = (*callback) (header, arg, &msg);
      if (ret < 0)
        return ret;
    }

  return 0;
}

s_int32_t
nsm_parse_pvlan_configure (u_char **pnt, u_int16_t *size,
                           struct nsm_msg_header *header, void *arg,
                           NSM_CALLBACK callback)
{
  struct nsm_msg_pvlan_configure msg;
  int ret;

  ret = nsm_decode_pvlan_configure (pnt, size, &msg);
  if (ret < 0)
    return ret;

  if (callback)
    {
      ret = (*callback) (header, arg, &msg);
      if (ret < 0)
        return ret;
    }

  return 0;
}

s_int32_t
nsm_parse_pvlan_associate (u_char **pnt, u_int16_t *size,
                           struct nsm_msg_header *header, void *arg,
                           NSM_CALLBACK callback)
{
  struct nsm_msg_pvlan_association msg;
  int ret;

  ret = nsm_decode_pvlan_associate (pnt, size, &msg);
  if (ret < 0)
    return ret;

  if (callback)
    {
      ret = (*callback) (header, arg, &msg);
      if (ret < 0)
        return ret;
    }

  return 0;
}
#endif /* HAVE_PVLAN */

#ifdef HAVE_VLAN_CLASS
/* Parse VLAN classifier message. */
s_int32_t
nsm_parse_vlan_classifier_msg (u_char **pnt, u_int16_t *size,
                               struct nsm_msg_header *header, void *arg,
                               NSM_CALLBACK callback)
{
  struct nsm_msg_vlan_classifier msg;
  int ret;

  pal_mem_set (&msg, 0, sizeof (struct nsm_msg_vlan_classifier));
  ret = nsm_decode_vlan_classifier_msg (pnt, size, &msg);
  if (ret < 0)
    return ret;

  if (callback)
    {
      ret = (*callback) (header, arg, &msg);
      if (ret < 0)
        return ret;
    }

  return 0;
}

/* Parse VLAN port classifier message. */
s_int32_t
nsm_parse_vlan_port_class_msg (u_char **pnt, u_int16_t *size,
                              struct nsm_msg_header *header, void *arg,                              NSM_CALLBACK callback)
{
  struct nsm_msg_vlan_port_classifier msg;
  int ret;

  pal_mem_set (&msg, 0, sizeof(struct nsm_msg_vlan_port_classifier));
  ret = nsm_decode_vlan_port_class_msg (pnt, size, &msg);
  if (ret < 0)
    return ret;

  if (callback)
    {
      ret = (*callback) (header, arg, &msg);
      if (ret < 0)
        return ret;
    }

  return 0;
}
#endif /* HAVE_VLAN_CLASS */
#endif /* HAVE_VLAN */
#endif /* HAVE_CUSTOM1 */

#ifdef HAVE_AUTHD
/* Parse the port authentication port state message.  */
s_int32_t
nsm_parse_auth_port_state_msg (u_char **pnt, u_int16_t *size,
                               struct nsm_msg_header *header, void *arg,
                               NSM_CALLBACK callback)
{
  struct nsm_msg_auth_port_state msg;
  int ret;

  ret = nsm_decode_auth_port_state_msg (pnt, size, &msg);
  if (ret < 0)
    return ret;

  if (callback)
    {
      ret = (*callback) (header, arg, &msg);
      if (ret < 0)
        return ret;
    }

  return 0;
}

#ifdef HAVE_MAC_AUTH
/* Parse the auth-mac port state message. */
s_int32_t
nsm_parse_auth_mac_port_state_msg (u_char **pnt, u_int16_t *size,
                                   struct nsm_msg_header *header, void *arg,
                                   NSM_CALLBACK callback)
{
  struct nsm_msg_auth_mac_port_state msg;
  s_int32_t ret;

  ret = nsm_decode_auth_mac_port_state_msg (pnt, size, &msg);
  if (ret < 0)
    return ret;

  if (callback)
    {
      ret = (*callback) (header, arg, &msg);
      if (ret < 0)
        return ret;
    }

  return 0;
}

/* Parse the mac authentication status message. */
s_int32_t
nsm_parse_auth_mac_status_msg (u_char **pnt, u_int16_t *size,
                               struct nsm_msg_header *header, void *arg,
                               NSM_CALLBACK callback)
{
  struct nsm_msg_auth_mac_status msg;
  s_int32_t ret;

  ret = nsm_decode_auth_mac_status_msg (pnt, size, &msg);
  if (ret < 0)
    return ret;

  if (callback)
    {
      ret = (*callback) (header, arg, &msg);
      if (ret < 0)
        return ret;
    }

  return 0;
}
#endif /* HAVE_MAC_AUTH */
#endif /* HAVE_AUTHD */

#ifdef HAVE_ONMD

/* Parse EFM message. */
s_int32_t
nsm_parse_oam_efm_msg (u_char **pnt, u_int16_t *size,
                       struct nsm_msg_header *header, void *arg,
                       NSM_CALLBACK callback)
{
  struct nsm_msg_efm_if msg;
  int ret;

  ret = nsm_decode_efm_if_msg (pnt, size, &msg);

  if (ret < 0)
    return ret;

  if (callback)
    {
      ret = (*callback) (header, arg, &msg);
      if (ret < 0)
        return ret;
    }

  return 0;
}

/* Parse LLDP message. */
s_int32_t
nsm_parse_oam_lldp_msg (u_char **pnt, u_int16_t *size,
                        struct nsm_msg_header *header, void *arg,
                        NSM_CALLBACK callback)
{
  struct nsm_msg_lldp msg;
  int ret;

  ret = nsm_decode_lldp_msg (pnt, size, &msg);

  if (ret < 0)
    return ret;

  if (callback)
    {
      ret = (*callback) (header, arg, &msg);
      if (ret < 0)
        return ret;
    }

  return 0;
}

/* Parse CFM message. */
s_int32_t
nsm_parse_oam_cfm_msg (u_char **pnt, u_int16_t *size,
                       struct nsm_msg_header *header, void *arg,
                       NSM_CALLBACK callback)
{
  struct nsm_msg_cfm msg;
  int ret;

  ret = nsm_decode_cfm_msg (pnt, size, &msg);

  if (ret < 0)
    return ret;

  if (callback)
    {
      ret = (*callback) (header, arg, &msg);
      if (ret < 0)
        return ret;
    }

  return 0;

}

/* Parse CFM Status message. */
int
nsm_parse_oam_cfm_status_msg (u_char **pnt, u_int16_t *size,
                       struct nsm_msg_header *header, void *arg,
                       NSM_CALLBACK callback)
{
  struct nsm_msg_cfm_status msg;
  int ret;

  ret = nsm_decode_cfm_status_msg (pnt, size, &msg);

  if (ret < 0)
    return ret;

  if (callback)
    {
      ret = (*callback) (header, arg, &msg);
      if (ret < 0)
        return ret;
    }

  return 0;
}

#endif /* HAVE_ONMD */


#ifdef HAVE_ELMID

/* Parse EVC message. */
int
nsm_parse_elmi_evc_msg (u_char **pnt, u_int16_t *size,
                    struct nsm_msg_header *header, void *arg,
                    NSM_CALLBACK callback)
{
  struct nsm_msg_elmi_evc msg;
  int ret;

  pal_mem_set (&msg, 0, sizeof (struct nsm_msg_elmi_evc));
  ret = nsm_decode_evc_msg (pnt, size, &msg);
  if (ret < 0)
    return ret;

  if (callback)
    {
      ret = (*callback) (header, arg, &msg);
      if (ret < 0)
        return ret;
    }

  return 0;
}

/* Parse UNI message. */
int
nsm_parse_uni_msg (u_char **pnt, u_int16_t *size,
                    struct nsm_msg_header *header, void *arg,
                    NSM_CALLBACK callback)
{
  struct nsm_msg_uni msg;
  int ret = 0;

  pal_mem_set (&msg, 0, sizeof (struct nsm_msg_uni));
  ret = nsm_decode_uni_msg (pnt, size, &msg);
  if (ret < 0)
    return ret;

  if (callback)
    {
      ret = (*callback) (header, arg, &msg);
      if (ret < 0)
        return ret;
    }

  return 0;
}


/* Parse the elmi operational state message.  */
int
nsm_parse_elmi_status_msg (u_char **pnt, u_int16_t *size,
                              struct nsm_msg_header *header, void *arg,
                              NSM_CALLBACK callback)
{
  struct nsm_msg_elmi_status msg;
  int ret;

  ret = nsm_decode_elmi_status_msg (pnt, size, &msg);

  if (ret < 0)
    return ret;

  if (callback)
    {
      ret = (*callback) (header, arg, &msg);
      if (ret < 0)
        return ret;
    }

  return 0;

}

/* Parse the elmi operational state message.  */
int
nsm_parse_elmi_vlan_msg (u_char **pnt, u_int16_t *size,
                         struct nsm_msg_header *header, void *arg,
                         NSM_CALLBACK callback)
{
  struct nsm_msg_vlan_port msg;
  int ret;

  pal_mem_set (&msg, 0, sizeof (struct nsm_msg_vlan_port));
  ret = nsm_decode_elmi_vlan_msg (pnt, size, &msg);

  if (ret < 0)
    return ret;

  if (callback)
    {
      ret = (*callback) (header, arg, &msg);
      if (ret < 0)
        return ret;
    }

  return 0;

}

/* Parse BW profile message. */
int
nsm_parse_elmi_bw_msg (u_char **pnt, u_int16_t *size,
                    struct nsm_msg_header *header, void *arg,
                    NSM_CALLBACK callback)
{
  struct nsm_msg_bw_profile msg;
  int ret = 0;

  pal_mem_set (&msg, 0, sizeof (struct nsm_msg_bw_profile));
  ret = nsm_decode_elmi_bw_msg (pnt, size, &msg);
  if (ret < 0)
    return ret;

  if (callback)
    {
      ret = (*callback) (header, arg, &msg);
      if (ret < 0)
        return ret;
    }

  return 0;

}

#endif /* HAVE_ELMID */

#ifdef HAVE_MCAST_IPV4
static const char *ipv4_mrib_notif_type_str[] =
{
  "VIF Add Notification",                   /* VIF Add */
  "VIF Del Notification",                   /* VIF Del */
  "MRT Add Notification",                   /* MRT Add */
  "MRT Del Notification",                   /* MRT Del */
  "MCAST Notification",                     /* MCAST */
  "NO-MCAST Notification",                  /* MCAST */
  "CLR MRT Notification"                    /* CLR MRT */
};

static const char *ipv4_mrib_notif_status_str[] =
{
  "VIF Exists",
  "VIF Not Exist",
  "VIF No Index",
  "VIF Not Belong",
  "VIF Fwd Add Err",
  "VIF Fwd Del Err",
  "VIF Add Sucess",
  "VIF Del Success",
  "MRT Limit Exceeded",
  "MRT Exists",
  "MRT Not Exist",
  "MRT Not Belong",
  "MRT Fwd Add Err",
  "MRT Fwd Del Err",
  "MRT Add Sucess",
  "MRT Del Success",
  "MRIB Internal Error",
  "N/A"
};

static const char *
nsm_ipv4_mrib_notif_type_str (u_int16_t type)
{
  if (type < 7)
    return ipv4_mrib_notif_type_str[type];

  return "Invalid";
}

static const char *
nsm_ipv4_mrib_notif_status_str (u_int16_t status)
{
  if (status < 17)
    return ipv4_mrib_notif_status_str[status];

  return "Invalid";
}

/* IPV4 Vif Add */
s_int32_t
nsm_encode_ipv4_vif_add (u_char **pnt, u_int16_t *size,
    struct nsm_msg_ipv4_vif_add *msg)
{
  u_char *sp = *pnt;

  /* Check size.  */
  if (*size < NSM_MSG_IPV4_VIF_ADD_SIZE)
    return NSM_ERR_PKT_TOO_SMALL;

  /* Ifindex.  */
  TLV_ENCODE_PUTL (msg->ifindex);

  /* VIF local address */
  TLV_ENCODE_PUT_IN4_ADDR (&msg->local_addr);

  /* VIF remote address */
  TLV_ENCODE_PUT_IN4_ADDR (&msg->remote_addr);

  /* Flags.  */
  TLV_ENCODE_PUTW (msg->flags);

  return *pnt - sp;
}

s_int32_t
nsm_decode_ipv4_vif_add (u_char **pnt, u_int16_t *size,
    struct nsm_msg_ipv4_vif_add *msg)
{
  u_char *sp = *pnt;

  /* Check size.  */
  if (*size < NSM_MSG_IPV4_VIF_ADD_SIZE)
    return NSM_ERR_PKT_TOO_SMALL;

  /* Ifindex.  */
  TLV_DECODE_GETL (msg->ifindex);

  /* VIF local address */
  TLV_DECODE_GET_IN4_ADDR (&msg->local_addr);

  /* VIF remote address */
  TLV_DECODE_GET_IN4_ADDR (&msg->remote_addr);

  /* Flags.  */
  TLV_DECODE_GETW (msg->flags);

  return *pnt - sp;
}

s_int32_t
nsm_parse_ipv4_vif_add (u_char **pnt, u_int16_t *size,
    struct nsm_msg_header *header, void *arg, NSM_CALLBACK callback)
{
  int ret = -1;
  struct nsm_msg_ipv4_vif_add msg;

  pal_mem_set (&msg, 0, sizeof (struct nsm_msg_ipv4_vif_add));

  ret = nsm_decode_ipv4_vif_add (pnt, size, &msg);
  if (ret < 0)
    return ret;

  if (callback)
    {
      ret = (*callback) (header, arg, &msg);
      if (ret < 0)
        return ret;
    }

  return ret;
}

void
nsm_ipv4_vif_add_dump (struct lib_globals *zg,
    struct nsm_msg_ipv4_vif_add *msg)
{
  zlog_info (zg, "IPV4 Vif add:");
  zlog_info (zg, " Ifindex: %d", msg->ifindex);
  zlog_info (zg, " Local Address: %r", &msg->local_addr);
  zlog_info (zg, " Remote Address: %r", &msg->remote_addr);
  zlog_info (zg, " Flags: 0x%04x %s", msg->flags,
      (CHECK_FLAG (msg->flags, NSM_MSG_IPV4_VIF_FLAG_REGISTER) ? "Register Vif":(CHECK_FLAG (msg->flags, NSM_MSG_IPV4_VIF_FLAG_TUNNEL) ? "Tunnel Vif":"")));
  return;
}

/* IPV4 Vif Del */
s_int32_t
nsm_encode_ipv4_vif_del (u_char **pnt, u_int16_t *size,
    struct nsm_msg_ipv4_vif_del *msg)
{
  u_char *sp = *pnt;

  /* Check size.  */
  if (*size < NSM_MSG_IPV4_VIF_DEL_SIZE)
    return NSM_ERR_PKT_TOO_SMALL;

  /* Vif index.  */
  TLV_ENCODE_PUTW (msg->vif_index);

  /* Flags.  */
  TLV_ENCODE_PUTW (msg->flags);

  return *pnt - sp;
}

s_int32_t
nsm_decode_ipv4_vif_del (u_char **pnt, u_int16_t *size,
    struct nsm_msg_ipv4_vif_del *msg)
{
  u_char *sp = *pnt;

  /* Check size.  */
  if (*size < NSM_MSG_IPV4_VIF_DEL_SIZE)
    return NSM_ERR_PKT_TOO_SMALL;

  /* Vif index.  */
  TLV_DECODE_GETW (msg->vif_index);

  /* Flags.  */
  TLV_DECODE_GETW (msg->flags);

  return *pnt - sp;
}

s_int32_t
nsm_parse_ipv4_vif_del (u_char **pnt, u_int16_t *size,
    struct nsm_msg_header *header, void *arg, NSM_CALLBACK callback)
{
  int ret = -1;
  struct nsm_msg_ipv4_vif_del msg;

  pal_mem_set (&msg, 0, sizeof (struct nsm_msg_ipv4_vif_del));

  ret = nsm_decode_ipv4_vif_del (pnt, size, &msg);
  if (ret < 0)
    return ret;

  if (callback)
    {
      ret = (*callback) (header, arg, &msg);
      if (ret < 0)
        return ret;
    }

  return ret;
}

void
nsm_ipv4_vif_del_dump (struct lib_globals *zg,
    struct nsm_msg_ipv4_vif_del *msg)
{
  zlog_info (zg, "IPV4 Vif del:");
  zlog_info (zg, " Vif index: %d", msg->vif_index);
  zlog_info (zg, " Flags: 0x%04x %s", msg->flags,
      (CHECK_FLAG (msg->flags, NSM_MSG_IPV4_VIF_FLAG_REGISTER) ? "Register Vif":(CHECK_FLAG (msg->flags, NSM_MSG_IPV4_VIF_FLAG_TUNNEL) ? "Tunnel Vif":"")));
  return;
  return;
}

/* IPV4 Mrt add */
s_int32_t
nsm_encode_ipv4_mrt_add (u_char **pnt, u_int16_t *size,
    struct nsm_msg_ipv4_mrt_add *msg)
{
  u_char *sp = *pnt;
  int i = 0;

  /* Check size.  */
  if (*size < (NSM_MSG_IPV4_MRT_ADD_SIZE +
        (msg->num_olist * sizeof (u_int16_t))))
    return NSM_ERR_PKT_TOO_SMALL;

  /* Source address.  */
  TLV_ENCODE_PUT_IN4_ADDR (&msg->src);

  /* Group address.  */
  TLV_ENCODE_PUT_IN4_ADDR (&msg->group);

  /* RPF address.  */
  TLV_ENCODE_PUT_IN4_ADDR (&msg->rpf_addr);

  /* Flags.  */
  TLV_ENCODE_PUTW (msg->flags);

  /* Stat time.  */
  TLV_ENCODE_PUTW (msg->stat_time);

  /* Pkt count.  */
  TLV_ENCODE_PUTW (msg->pkt_count);

  /* Iif.  */
  TLV_ENCODE_PUTW (msg->iif);

  /* Number in olist.  */
  TLV_ENCODE_PUTW (msg->num_olist);

  /* Encode olist */
  while (i < msg->num_olist)
    {
      TLV_ENCODE_PUTW (msg->olist_vifs[i]);
      i++;
    }

  return *pnt - sp;
}

/* Note that this function allocates memory to hold decoded olist.
 * This memory is expected to be freed by the caller of this function.
 */
s_int32_t
nsm_decode_ipv4_mrt_add (u_char **pnt, u_int16_t *size,
    struct nsm_msg_ipv4_mrt_add *msg)
{
  u_char *sp = *pnt;
  int i = 0;

  /* Check minimum required size.  */
  if (*size < NSM_MSG_IPV4_MRT_ADD_SIZE)
    return NSM_ERR_PKT_TOO_SMALL;

  /* Source address.  */
  TLV_DECODE_GET_IN4_ADDR (&msg->src);

  /* Group address.  */
  TLV_DECODE_GET_IN4_ADDR (&msg->group);

  /* RPF address.  */
  TLV_DECODE_GET_IN4_ADDR (&msg->rpf_addr);

  /* Flags.  */
  TLV_DECODE_GETW (msg->flags);

  /* Stat time.  */
  TLV_DECODE_GETW (msg->stat_time);

  /* Pkt count.  */
  TLV_DECODE_GETW (msg->pkt_count);

  /* Iif.  */
  TLV_DECODE_GETW (msg->iif);

  /* Number in olist.  */
  TLV_DECODE_GETW (msg->num_olist);

  if (msg->num_olist == 0)
    return *pnt - sp;

  /* Check minimum required size.  */
  if (*size < (msg->num_olist * sizeof (u_int16_t)))
    return NSM_ERR_PKT_TOO_SMALL;

  /* Decode olist */
  msg->olist_vifs = XMALLOC (MTYPE_TMP, (msg->num_olist * sizeof (u_int16_t)));
  if (msg->olist_vifs == NULL)
    return -1;

  while (i < msg->num_olist)
    {
      TLV_DECODE_GETW (msg->olist_vifs[i]);
      i++;
    }

  return *pnt - sp;
}

s_int32_t
nsm_parse_ipv4_mrt_add (u_char **pnt, u_int16_t *size,
    struct nsm_msg_header *header, void *arg, NSM_CALLBACK callback)
{
  int ret = -1;
  struct nsm_msg_ipv4_mrt_add msg;

  pal_mem_set (&msg, 0, sizeof (struct nsm_msg_ipv4_mrt_add));

  ret = nsm_decode_ipv4_mrt_add (pnt, size, &msg);
  if (ret < 0)
    return ret;

  if (callback)
    {
      ret = (*callback) (header, arg, &msg);
    }

  /* Free the allocated olist */
  if (msg.olist_vifs != NULL)
    XFREE (MTYPE_TMP, msg.olist_vifs);

  return ret;
}

void
nsm_ipv4_mrt_add_dump (struct lib_globals *zg,
    struct nsm_msg_ipv4_mrt_add *msg)
{
  int i = 0;
  char buf[71];
  char *cp;
  int wlen;

  zlog_info (zg, "IPV4 MRT add:");
  zlog_info (zg, " Source Address: %r", &msg->src);
  zlog_info (zg, " Group Address:  %r", &msg->group);
  zlog_info (zg, " RPF Address:    %r", &msg->rpf_addr);
  zlog_info (zg, " Flags: 0x%04x %s %s", msg->flags,
      (CHECK_FLAG (msg->flags, NSM_MSG_IPV4_MRT_STAT_IMMEDIATE) ? "Stat Imm":""),
      (CHECK_FLAG (msg->flags, NSM_MSG_IPV4_MRT_STAT_TIMED) ? "Stat Timed":""));
  zlog_info (zg, " Stat Time: %d", msg->stat_time);
  zlog_info (zg, " Pkt count: %d", msg->pkt_count);
  zlog_info (zg, " iif: %d", msg->iif);
  zlog_info (zg, " olist number: %d", msg->num_olist);
  zlog_info (zg, " olist :");

  cp = buf;
  wlen = 0;
  /* We will print 10 elements in the olist per row */
  while (i < msg->num_olist)
    {
      /* If this is the tenth member, print it */
      if (((i + 1) % 10) == 0)
        {
          wlen = pal_snprintf (cp, sizeof (buf) - (cp - buf), "%d", msg->olist_vifs[i]);

          cp += wlen;

          /* Terminate the buffer */
          *cp = '\0';

          /* Now print the row */
          zlog_info (zg, "   %s", buf);

          /* Initialize pointers and continue */
          cp = buf;

          i++;

          continue;
        }

      /* Add this element to the buffer */
      if (i == msg->num_olist - 1)
        wlen = pal_snprintf (cp, sizeof (buf) - (cp - buf), "%d", msg->olist_vifs[i]);
      else
        wlen = pal_snprintf (cp, sizeof (buf) - (cp - buf), "%d, ", msg->olist_vifs[i]);

      cp += wlen;

      i++;
    }

  /* If the number in olist is not a multiple of 10, then we have to print
   * the last row.
   */
  if ((msg->num_olist % 10) != 0)
    {
      /* Terminate the buffer */
      *cp = '\0';

      zlog_info (zg, "   %s", buf);
    }

  return;
}

/* IPV4 Mrt del */
s_int32_t
nsm_encode_ipv4_mrt_del (u_char **pnt, u_int16_t *size,
    struct nsm_msg_ipv4_mrt_del *msg)
{
  u_char *sp = *pnt;

  /* Check size.  */
  if (*size < NSM_MSG_IPV4_MRT_DEL_SIZE)
    return NSM_ERR_PKT_TOO_SMALL;

  /* Source address.  */
  TLV_ENCODE_PUT_IN4_ADDR (&msg->src);

  /* Group address.  */
  TLV_ENCODE_PUT_IN4_ADDR (&msg->group);

  return *pnt - sp;
}

s_int32_t
nsm_decode_ipv4_mrt_del (u_char **pnt, u_int16_t *size,
    struct nsm_msg_ipv4_mrt_del *msg)
{
  u_char *sp = *pnt;

  /* Check minimum required size.  */
  if (*size < NSM_MSG_IPV4_MRT_DEL_SIZE)
    return NSM_ERR_PKT_TOO_SMALL;

  /* Source address.  */
  TLV_DECODE_GET_IN4_ADDR (&msg->src);

  /* Group address.  */
  TLV_DECODE_GET_IN4_ADDR (&msg->group);

  return *pnt - sp;
}

s_int32_t
nsm_parse_ipv4_mrt_del (u_char **pnt, u_int16_t *size,
    struct nsm_msg_header *header, void *arg, NSM_CALLBACK callback)
{
  int ret = -1;
  struct nsm_msg_ipv4_mrt_del msg;

  pal_mem_set (&msg, 0, sizeof (struct nsm_msg_ipv4_mrt_del));

  ret = nsm_decode_ipv4_mrt_del (pnt, size, &msg);
  if (ret < 0)
    return ret;

  if (callback)
    {
      ret = (*callback) (header, arg, &msg);
    }

  return ret;
}

void
nsm_ipv4_mrt_del_dump (struct lib_globals *zg,
    struct nsm_msg_ipv4_mrt_del *msg)
{
  zlog_info (zg, "IPV4 MRT del:");
  zlog_info (zg, " Source Address: %r", &msg->src);
  zlog_info (zg, " Group Address: %r", &msg->group);
  return;
}

/* IPV4 Mrt Stat Flags Update */
s_int32_t
nsm_encode_ipv4_mrt_stat_flags_update (u_char **pnt, u_int16_t *size,
    struct nsm_msg_ipv4_mrt_stat_flags_update *msg)
{
  u_char *sp = *pnt;

  /* Check size.  */
  if (*size < NSM_MSG_IPV4_MRT_STAT_FLAGS_UPDATE_SIZE)
    return NSM_ERR_PKT_TOO_SMALL;

  /* Source address.  */
  TLV_ENCODE_PUT_IN4_ADDR (&msg->src);

  /* Group address.  */
  TLV_ENCODE_PUT_IN4_ADDR (&msg->group);

  /* Flags.  */
  TLV_ENCODE_PUTW (msg->flags);

  /* Stat time.  */
  TLV_ENCODE_PUTW (msg->stat_time);

  return *pnt - sp;
}

s_int32_t
nsm_decode_ipv4_mrt_stat_flags_update (u_char **pnt, u_int16_t *size,
    struct nsm_msg_ipv4_mrt_stat_flags_update *msg)
{
  u_char *sp = *pnt;

  /* Check minimum required size.  */
  if (*size < NSM_MSG_IPV4_MRT_STAT_FLAGS_UPDATE_SIZE)
    return NSM_ERR_PKT_TOO_SMALL;

  /* Source address.  */
  TLV_DECODE_GET_IN4_ADDR (&msg->src);

  /* Group address.  */
  TLV_DECODE_GET_IN4_ADDR (&msg->group);

  /* Flags.  */
  TLV_DECODE_GETW (msg->flags);

  /* Stat time.  */
  TLV_DECODE_GETW (msg->stat_time);

  return *pnt - sp;
}

s_int32_t
nsm_parse_ipv4_mrt_stat_flags_update (u_char **pnt, u_int16_t *size,
    struct nsm_msg_header *header, void *arg, NSM_CALLBACK callback)
{
  int ret = -1;
  struct nsm_msg_ipv4_mrt_stat_flags_update msg;

  pal_mem_set (&msg, 0, sizeof (struct nsm_msg_ipv4_mrt_stat_flags_update));

  ret = nsm_decode_ipv4_mrt_stat_flags_update (pnt, size, &msg);
  if (ret < 0)
    return ret;

  if (callback)
    {
      ret = (*callback) (header, arg, &msg);
    }

  return ret;
}

void
nsm_ipv4_mrt_stat_flags_update_dump (struct lib_globals *zg,
    struct nsm_msg_ipv4_mrt_stat_flags_update *msg)
{
  zlog_info (zg, "IPV4 MRT Stat Flags Update:");
  zlog_info (zg, " Source Address: %r", &msg->src);
  zlog_info (zg, " Group Address: %r", &msg->group);
  zlog_info (zg, " Flags: 0x%04x %s %s", msg->flags,
      (CHECK_FLAG (msg->flags, NSM_MSG_IPV4_MRT_STAT_IMMEDIATE) ? "Stat Imm":""),
      (CHECK_FLAG (msg->flags, NSM_MSG_IPV4_MRT_STAT_TIMED) ? "Stat Timed":""));
  zlog_info (zg, " Stat Time: %d", msg->stat_time);

  return;
}

/* IPV4 Mrt State Refresh Flag Update */
s_int32_t
nsm_encode_ipv4_mrt_state_refresh_flag_update (u_char **pnt, u_int16_t *size,
    struct nsm_msg_ipv4_mrt_state_refresh_flag_update *msg)
{
  u_char *sp = *pnt;

  /* Check size.  */
  if (*size < NSM_MSG_IPV4_MRT_ST_REFRESH_FLAG_UPDATE_SIZE)
    return NSM_ERR_PKT_TOO_SMALL;

  /* Group address.  */
  TLV_ENCODE_PUT_IN4_ADDR (&msg->grp);

  /* Interface Index */
  TLV_ENCODE_PUTL (msg->ifindex);

  return *pnt - sp;
}

s_int32_t
nsm_decode_ipv4_mrt_state_refresh_flag_update (u_char **pnt, u_int16_t *size,
    struct nsm_msg_ipv4_mrt_state_refresh_flag_update *msg)
{
  u_char *sp = *pnt;

  /* Check minimum required size.  */
  if (*size < NSM_MSG_IPV4_MRT_ST_REFRESH_FLAG_UPDATE_SIZE)
    return NSM_ERR_PKT_TOO_SMALL;

  /* Group address.  */
  TLV_DECODE_GET_IN4_ADDR (&msg->grp);


  /* Interface Index */
  TLV_DECODE_GETL (msg->ifindex);

  return *pnt - sp;
}

s_int32_t
nsm_parse_ipv4_mrt_state_refresh_flag_update (u_char **pnt, u_int16_t *size,
    struct nsm_msg_header *header, void *arg, NSM_CALLBACK callback)
{
  struct nsm_msg_ipv4_mrt_state_refresh_flag_update msg;
  int ret = -1;

  pal_mem_set (&msg, 0,
               sizeof (struct nsm_msg_ipv4_mrt_state_refresh_flag_update));

  ret = nsm_decode_ipv4_mrt_state_refresh_flag_update (pnt, size, &msg);
  if (ret < 0)
    return ret;

  if (callback)
    {
      ret = (*callback) (header, arg, &msg);
    }

  return ret;
}

/* IPV4 Mrt NOCACHE */
s_int32_t
nsm_encode_ipv4_mrt_nocache (u_char **pnt, u_int16_t *size,
    struct nsm_msg_ipv4_mrt_nocache *msg)
{
  u_char *sp = *pnt;

  /* Check size.  */
  if (*size < NSM_MSG_IPV4_MRT_NOCACHE_SIZE)
    return NSM_ERR_PKT_TOO_SMALL;

  /* Source address.  */
  TLV_ENCODE_PUT_IN4_ADDR (&msg->src);

  /* Group address.  */
  TLV_ENCODE_PUT_IN4_ADDR (&msg->group);

  /* vif index.  */
  TLV_ENCODE_PUTW (msg->vif_index);

  return *pnt - sp;
}

s_int32_t
nsm_decode_ipv4_mrt_nocache (u_char **pnt, u_int16_t *size,
    struct nsm_msg_ipv4_mrt_nocache *msg)
{
  u_char *sp = *pnt;

  /* Check minimum required size.  */
  if (*size < NSM_MSG_IPV4_MRT_NOCACHE_SIZE)
    return NSM_ERR_PKT_TOO_SMALL;

  /* Source address.  */
  TLV_DECODE_GET_IN4_ADDR (&msg->src);

  /* Group address.  */
  TLV_DECODE_GET_IN4_ADDR (&msg->group);

  /* vif index.  */
  TLV_DECODE_GETW (msg->vif_index);

  return *pnt - sp;
}

s_int32_t
nsm_parse_ipv4_mrt_nocache (u_char **pnt, u_int16_t *size,
    struct nsm_msg_header *header, void *arg, NSM_CALLBACK callback)
{
  int ret = -1;
  struct nsm_msg_ipv4_mrt_nocache msg;

  pal_mem_set (&msg, 0, sizeof (struct nsm_msg_ipv4_mrt_nocache));

  ret = nsm_decode_ipv4_mrt_nocache (pnt, size, &msg);
  if (ret < 0)
    return ret;

  if (callback)
    {
      ret = (*callback) (header, arg, &msg);
    }

  return ret;
}

void
nsm_ipv4_mrt_nocache_dump (struct lib_globals *zg,
    struct nsm_msg_ipv4_mrt_nocache *msg)
{
  zlog_info (zg, "IPV4 MRT NOCACHE:");
  zlog_info (zg, " Source Address: %r", &msg->src);
  zlog_info (zg, " Group Address: %r", &msg->group);
  zlog_info (zg, " Vif index: %d", msg->vif_index);
  return;
}

/* IPV4 Mrt WRONGVIF */
s_int32_t
nsm_encode_ipv4_mrt_wrongvif (u_char **pnt, u_int16_t *size,
    struct nsm_msg_ipv4_mrt_wrongvif *msg)
{
  u_char *sp = *pnt;

  /* Check size.  */
  if (*size < NSM_MSG_IPV4_MRT_WRONGVIF_SIZE)
    return NSM_ERR_PKT_TOO_SMALL;

  /* Source address.  */
  TLV_ENCODE_PUT_IN4_ADDR (&msg->src);

  /* Group address.  */
  TLV_ENCODE_PUT_IN4_ADDR (&msg->group);

  /* vif index.  */
  TLV_ENCODE_PUTW (msg->vif_index);

  return *pnt - sp;
}

s_int32_t
nsm_decode_ipv4_mrt_wrongvif (u_char **pnt, u_int16_t *size,
    struct nsm_msg_ipv4_mrt_wrongvif *msg)
{
  u_char *sp = *pnt;

  /* Check minimum required size.  */
  if (*size < NSM_MSG_IPV4_MRT_WRONGVIF_SIZE)
    return NSM_ERR_PKT_TOO_SMALL;

  /* Source address.  */
  TLV_DECODE_GET_IN4_ADDR (&msg->src);

  /* Group address.  */
  TLV_DECODE_GET_IN4_ADDR (&msg->group);

  /* vif index.  */
  TLV_DECODE_GETW (msg->vif_index);

  return *pnt - sp;
}

s_int32_t
nsm_parse_ipv4_mrt_wrongvif (u_char **pnt, u_int16_t *size,
    struct nsm_msg_header *header, void *arg, NSM_CALLBACK callback)
{
  int ret = -1;
  struct nsm_msg_ipv4_mrt_wrongvif msg;

  pal_mem_set (&msg, 0, sizeof (struct nsm_msg_ipv4_mrt_wrongvif));

  ret = nsm_decode_ipv4_mrt_wrongvif (pnt, size, &msg);
  if (ret < 0)
    return ret;

  if (callback)
    {
      ret = (*callback) (header, arg, &msg);
    }

  return ret;
}

void
nsm_ipv4_mrt_wrongvif_dump (struct lib_globals *zg,
    struct nsm_msg_ipv4_mrt_wrongvif *msg)
{
  zlog_info (zg, "IPV4 MRT WRONGVIF:");
  zlog_info (zg, " Source Address: %r", &msg->src);
  zlog_info (zg, " Group Address: %r", &msg->group);
  zlog_info (zg, " Vif index: %d", msg->vif_index);
  return;
}

/* IPV4 Mrt WHOLEPKT REQ */
s_int32_t
nsm_encode_ipv4_mrt_wholepkt_req (u_char **pnt, u_int16_t *size,
    struct nsm_msg_ipv4_mrt_wholepkt_req *msg)
{
  u_char *sp = *pnt;

  /* Check size.  */
  if (*size < NSM_MSG_IPV4_MRT_WHOLEPKT_REQ_SIZE)
    return NSM_ERR_PKT_TOO_SMALL;

  /* Source address.  */
  TLV_ENCODE_PUT_IN4_ADDR (&msg->src);

  /* Group address.  */
  TLV_ENCODE_PUT_IN4_ADDR (&msg->group);

  /* vif index.  */
  TLV_ENCODE_PUTW (msg->vif_index);

  /* Data TTL */
  TLV_ENCODE_PUTC (msg->data_ttl);

  return *pnt - sp;
}

s_int32_t
nsm_decode_ipv4_mrt_wholepkt_req (u_char **pnt, u_int16_t *size,
    struct nsm_msg_ipv4_mrt_wholepkt_req *msg)
{
  u_char *sp = *pnt;

  /* Check minimum required size.  */
  if (*size < NSM_MSG_IPV4_MRT_WHOLEPKT_REQ_SIZE)
    return NSM_ERR_PKT_TOO_SMALL;

  /* Source address.  */
  TLV_DECODE_GET_IN4_ADDR (&msg->src);

  /* Group address.  */
  TLV_DECODE_GET_IN4_ADDR (&msg->group);

  /* vif index.  */
  TLV_DECODE_GETW (msg->vif_index);

  /* Data TTL */
  TLV_DECODE_GETC (msg->data_ttl);

  return *pnt - sp;
}

s_int32_t
nsm_parse_ipv4_mrt_wholepkt_req (u_char **pnt, u_int16_t *size,
    struct nsm_msg_header *header, void *arg, NSM_CALLBACK callback)
{
  int ret = -1;
  struct nsm_msg_ipv4_mrt_wholepkt_req msg;

  pal_mem_set (&msg, 0, sizeof (struct nsm_msg_ipv4_mrt_wholepkt_req));

  ret = nsm_decode_ipv4_mrt_wholepkt_req (pnt, size, &msg);
  if (ret < 0)
    return ret;

  if (callback)
    {
      ret = (*callback) (header, arg, &msg);
    }

  return ret;
}

void
nsm_ipv4_mrt_wholepkt_req_dump (struct lib_globals *zg,
    struct nsm_msg_ipv4_mrt_wholepkt_req *msg)
{
  zlog_info (zg, "IPV4 MRT WHOLEPKT REQ:");
  zlog_info (zg, " Source Address: %r", &msg->src);
  zlog_info (zg, " Group Address: %r", &msg->group);
  zlog_info (zg, " Vif index: %d", msg->vif_index);
  zlog_info (zg, " Data TTL: %d", msg->data_ttl);
  return;
}

/* IPV4 Mrt WHOLEPKT reply */
s_int32_t
nsm_encode_ipv4_mrt_wholepkt_reply (u_char **pnt, u_int16_t *size,
    struct nsm_msg_ipv4_mrt_wholepkt_reply *msg)
{
  u_char *sp = *pnt;

  /* Check size.  */
  if (*size < NSM_MSG_IPV4_MRT_WHOLEPKT_REPLY_SIZE)
    return NSM_ERR_PKT_TOO_SMALL;

  /* Source address.  */
  TLV_ENCODE_PUT_IN4_ADDR (&msg->src);

  /* Group address.  */
  TLV_ENCODE_PUT_IN4_ADDR (&msg->group);

  /* RP address.  */
  TLV_ENCODE_PUT_IN4_ADDR (&msg->rp_addr);

  /* Reg pkt src address.  */
  TLV_ENCODE_PUT_IN4_ADDR (&msg->src_addr);

  /* Identifier.  */
  TLV_ENCODE_PUTL (msg->id);

  /* reply.  */
  TLV_ENCODE_PUTW (msg->reply);

  /* Reg pkt chksum type.  */
  TLV_ENCODE_PUTW (msg->chksum_type);

  return *pnt - sp;
}

s_int32_t
nsm_decode_ipv4_mrt_wholepkt_reply (u_char **pnt, u_int16_t *size,
    struct nsm_msg_ipv4_mrt_wholepkt_reply *msg)
{
  u_char *sp = *pnt;

  /* Check minimum required size.  */
  if (*size < NSM_MSG_IPV4_MRT_WHOLEPKT_REPLY_SIZE)
    return NSM_ERR_PKT_TOO_SMALL;

  /* Source address.  */
  TLV_DECODE_GET_IN4_ADDR (&msg->src);

  /* Group address.  */
  TLV_DECODE_GET_IN4_ADDR (&msg->group);

  /* RP address.  */
  TLV_DECODE_GET_IN4_ADDR (&msg->rp_addr);

  /* Reg pkt src address.  */
  TLV_DECODE_GET_IN4_ADDR (&msg->src_addr);

  /* Identifier.  */
  TLV_DECODE_GETL (msg->id);

  /* Reply.  */
  TLV_DECODE_GETW (msg->reply);

  /* Reg pkt chksum type.  */
  TLV_DECODE_GETW (msg->chksum_type);

  return *pnt - sp;
}

s_int32_t
nsm_parse_ipv4_mrt_wholepkt_reply (u_char **pnt, u_int16_t *size,
    struct nsm_msg_header *header, void *arg, NSM_CALLBACK callback)
{
  int ret = -1;
  struct nsm_msg_ipv4_mrt_wholepkt_reply msg;

  pal_mem_set (&msg, 0, sizeof (struct nsm_msg_ipv4_mrt_wholepkt_reply));

  ret = nsm_decode_ipv4_mrt_wholepkt_reply (pnt, size, &msg);
  if (ret < 0)
    return ret;

  if (callback)
    {
      ret = (*callback) (header, arg, &msg);
    }

  return ret;
}

void
nsm_ipv4_mrt_wholepkt_reply_dump (struct lib_globals *zg,
    struct nsm_msg_ipv4_mrt_wholepkt_reply *msg)
{
  zlog_info (zg, "IPV4 MRT WHOLEPKT REPLY:");
  zlog_info (zg, " Source Address: %r", &msg->src);
  zlog_info (zg, " Group Address: %r", &msg->group);
  zlog_info (zg, " RP Address: %r", &msg->rp_addr);
  zlog_info (zg, " Pkt Src Address: %r", &msg->src_addr);
  zlog_info (zg, " Identifier: %u", msg->id);
  zlog_info (zg, " Reply: %u %s", msg->reply,
      ((msg->reply == NSM_MSG_IPV4_WHOLEPKT_REPLY_ACK) ? "ACK" : "NACK"));
  zlog_info (zg, " Chksum type: %u %s", msg->chksum_type,
      ((msg->chksum_type == NSM_MSG_IPV4_REG_PKT_CHKSUM_REG) ? "Regular" : "Cisco"));
  return;
}

/* IPV4 Stat update */
static int
nsm_encode_ipv4_mrt_stat_block (u_char **pnt, u_int16_t *size,
    struct nsm_msg_ipv4_mrt_stat_block *blk)
{
  u_char *sp = *pnt;

  /* Source Address */
  TLV_ENCODE_PUT_IN4_ADDR (&blk->src);

  /* Group Address */
  TLV_ENCODE_PUT_IN4_ADDR (&blk->group);

  /* Packets Forwarde */
  TLV_ENCODE_PUTL (blk->pkts_fwd);

  /* Bytes Forwarded */
  TLV_ENCODE_PUTL (blk->bytes_fwd);

  /* Flags */
  TLV_ENCODE_PUTW (blk->flags);

  /* Reserved */
  TLV_ENCODE_PUTW (blk->resv);

  return *pnt - sp;
}

s_int32_t
nsm_encode_ipv4_mrt_stat_update (u_char **pnt, u_int16_t *size,
    struct nsm_msg_ipv4_mrt_stat_update *msg)
{
  u_char *sp = *pnt;
  int i = 0;

  /* Check size.  */
  if (*size < (NSM_MSG_IPV4_MRT_STAT_UPDATE_SIZE +
        (msg->num_blocks *  NSM_MSG_IPV4_MRT_STAT_BLOCK_SIZE)))
    return NSM_ERR_PKT_TOO_SMALL;

  /* Number of blocks.  */
  TLV_ENCODE_PUTW (msg->num_blocks);

  /* Encode olist */
  while (i < msg->num_blocks)
    {
      nsm_encode_ipv4_mrt_stat_block (pnt, size, &msg->blocks[i]);

      i++;
    }

  return *pnt - sp;
}

static int
nsm_decode_ipv4_mrt_stat_block (u_char **pnt, u_int16_t *size,
    struct nsm_msg_ipv4_mrt_stat_block *blk)
{
  u_char *sp = *pnt;

  /* Source Address */
  TLV_DECODE_GET_IN4_ADDR (&blk->src);

  /* Group Address */
  TLV_DECODE_GET_IN4_ADDR (&blk->group);

  /* Packets Forwarde */
  TLV_DECODE_GETL (blk->pkts_fwd);

  /* Bytes Forwarded */
  TLV_DECODE_GETL (blk->bytes_fwd);

  /* Flags */
  TLV_DECODE_GETW (blk->flags);

  /* Reserved */
  TLV_DECODE_GETW (blk->resv);

  return *pnt - sp;
}

/* Note that this function allocates memory to hold decoded stat blocks.
 * This memory is expected to be freed by the caller of this function.
 */
s_int32_t
nsm_decode_ipv4_mrt_stat_update (u_char **pnt, u_int16_t *size,
    struct nsm_msg_ipv4_mrt_stat_update *msg)
{
  u_char *sp = *pnt;
  int i = 0;

  /* Check minimum required size.  */
  if (*size < NSM_MSG_IPV4_MRT_STAT_UPDATE_SIZE)
    return NSM_ERR_PKT_TOO_SMALL;

  /* Number of blocks.  */
  TLV_DECODE_GETW (msg->num_blocks);

  /* Check minimum required size.  */
  if (*size < (msg->num_blocks * NSM_MSG_IPV4_MRT_STAT_BLOCK_SIZE))
    return NSM_ERR_PKT_TOO_SMALL;

  /* Decode olist */
  msg->blocks = XMALLOC (MTYPE_TMP, (msg->num_blocks * NSM_MSG_IPV4_MRT_STAT_BLOCK_SIZE));
  if (msg->blocks == NULL)
    return -1;

  while (i < msg->num_blocks)
    {
      nsm_decode_ipv4_mrt_stat_block (pnt, size, &msg->blocks[i]);
      i++;
    }

  return *pnt - sp;
}

s_int32_t
nsm_parse_ipv4_mrt_stat_update (u_char **pnt, u_int16_t *size,
    struct nsm_msg_header *header, void *arg, NSM_CALLBACK callback)
{
  int ret = -1;
  struct nsm_msg_ipv4_mrt_stat_update msg;

  pal_mem_set (&msg, 0, sizeof (struct nsm_msg_ipv4_mrt_stat_update));

  ret = nsm_decode_ipv4_mrt_stat_update (pnt, size, &msg);
  if (ret < 0)
    return ret;

  if (callback)
    {
      ret = (*callback) (header, arg, &msg);
    }

  /* Free the allocated olist */
  if (msg.blocks != NULL)
    XFREE (MTYPE_TMP, msg.blocks);

  return ret;
}

void
nsm_ipv4_mrt_stat_update_dump (struct lib_globals *zg,
    struct nsm_msg_ipv4_mrt_stat_update *msg)
{
  int i = 0;

  zlog_info (zg, "IPV4 MRT Stat update:");
  zlog_info (zg, " Number blocks: %d", msg->num_blocks);
  while (i < msg->num_blocks)
    {
      zlog_info (zg, " Block %d:", i + 1);
      zlog_info (zg, "  Source Address: %r", &msg->blocks[i].src);
      zlog_info (zg, "  Group Address: %r", &msg->blocks[i].group);
      zlog_info (zg, "  Pkts Forwarded: %u", msg->blocks[i].pkts_fwd);
      zlog_info (zg, "  Bytes Forwarded: %u", msg->blocks[i].bytes_fwd);
      zlog_info (zg, "  Flags: 0x%04x %s %s", msg->blocks[i].flags,
          (CHECK_FLAG (msg->blocks[i].flags, NSM_MSG_IPV4_MRT_STAT_IMMEDIATE) ? "Stat Imm" : ""),
          (CHECK_FLAG (msg->blocks[i].flags, NSM_MSG_IPV4_MRT_STAT_TIMED) ? "Stat Timed" : ""));
      zlog_info (zg, "  Reserved: %u", msg->blocks[i].resv);

      i++;
    }
  return;
}

/* IPV4 MRIB notification */
static int
nsm_encode_ipv4_mrib_vif_notification (u_char **pnt, u_int16_t *size,
    struct nsm_msg_ipv4_mrib_notification_key_vif *key)
{
  u_char *sp = *pnt;

  if (*size < NSM_IPV4_MRIB_NOTIF_KEY_VIF_SIZE)
    return NSM_ERR_PKT_TOO_SMALL;

  /* Ifindex.  */
  TLV_ENCODE_PUTL (key->ifindex);

  /* Tunnel Local Address.  */
  TLV_ENCODE_PUT_IN4_ADDR (&key->local_addr);

  /* Tunnel Remote Address.  */
  TLV_ENCODE_PUT_IN4_ADDR (&key->remote_addr);

  /* vif_index.  */
  TLV_ENCODE_PUTW (key->vif_index);

  /* Flags.  */
  TLV_ENCODE_PUTW (key->flags);

  return *pnt - sp;
}

static int
nsm_encode_ipv4_mrib_mrt_notification (u_char **pnt, u_int16_t *size,
    struct nsm_msg_ipv4_mrib_notification_key_mrt *key)
{
  u_char *sp = *pnt;

  if (*size < NSM_IPV4_MRIB_NOTIF_KEY_MRT_SIZE)
    return NSM_ERR_PKT_TOO_SMALL;

  /* Source Address.  */
  TLV_ENCODE_PUT_IN4_ADDR (&key->src);

  /* Group Address.  */
  TLV_ENCODE_PUT_IN4_ADDR (&key->group);

  /* Flags.  */
  TLV_ENCODE_PUTW (key->flags);

  return *pnt - sp;
}

s_int32_t
nsm_encode_ipv4_mrib_notification (u_char **pnt, u_int16_t *size,
    struct nsm_msg_ipv4_mrib_notification *msg)
{
  u_char *sp = *pnt;
  int ret;

  /* Check size for atleast type and status.  */
  if (*size < 4)
    return NSM_ERR_PKT_TOO_SMALL;

  /* Type.  */
  TLV_ENCODE_PUTW (msg->type);

  /* Status.  */
  TLV_ENCODE_PUTW (msg->status);

  switch (msg->type)
    {
    case NSM_MSG_IPV4_MRIB_NOTIFICATION_VIF_ADD:
    case NSM_MSG_IPV4_MRIB_NOTIFICATION_VIF_DEL:
      ret = nsm_encode_ipv4_mrib_vif_notification (pnt, size, &msg->key.vif_key);
      if (ret < 0)
        return ret;
      break;

    case NSM_MSG_IPV4_MRIB_NOTIFICATION_MRT_ADD:
    case NSM_MSG_IPV4_MRIB_NOTIFICATION_MRT_DEL:
    case NSM_MSG_IPV4_MRIB_NOTIFICATION_CLR_MRT:
      ret = nsm_encode_ipv4_mrib_mrt_notification (pnt, size, &msg->key.mrt_key);
      if (ret < 0)
        return ret;
      break;

    case NSM_MSG_IPV4_MRIB_NOTIFICATION_MCAST:
    case NSM_MSG_IPV4_MRIB_NOTIFICATION_NO_MCAST:
      break;
    }

  return *pnt - sp;
}

static int
nsm_decode_ipv4_mrib_vif_notification (u_char **pnt, u_int16_t *size,
    struct nsm_msg_ipv4_mrib_notification_key_vif *key)
{
  u_char *sp = *pnt;

  if (*size < NSM_IPV4_MRIB_NOTIF_KEY_VIF_SIZE)
    return NSM_ERR_PKT_TOO_SMALL;

  /* Ifindex.  */
  TLV_DECODE_GETL (key->ifindex);

  /* Tunnel local Address.  */
  TLV_DECODE_GET_IN4_ADDR (&key->local_addr);

  /* Tunnel remote Address.  */
  TLV_DECODE_GET_IN4_ADDR (&key->remote_addr);

  /* vif_index.  */
  TLV_DECODE_GETW (key->vif_index);

  /* Flags.  */
  TLV_DECODE_GETW (key->flags);

  return *pnt - sp;
}

static int
nsm_decode_ipv4_mrib_mrt_notification (u_char **pnt, u_int16_t *size,
    struct nsm_msg_ipv4_mrib_notification_key_mrt *key)
{
  u_char *sp = *pnt;

  if (*size < NSM_IPV4_MRIB_NOTIF_KEY_MRT_SIZE)
    return NSM_ERR_PKT_TOO_SMALL;

  /* Source Address.  */
  TLV_DECODE_GET_IN4_ADDR (&key->src);

  /* Group Address.  */
  TLV_DECODE_GET_IN4_ADDR (&key->group);

  /* Flags.  */
  TLV_DECODE_GETW (key->flags);

  return *pnt - sp;
}

s_int32_t
nsm_decode_ipv4_mrib_notification (u_char **pnt, u_int16_t *size,
    struct nsm_msg_ipv4_mrib_notification *msg)
{
  u_char *sp = *pnt;
  int ret;

  /* Check size for atleast type and status.  */
  if (*size < 4)
    return NSM_ERR_PKT_TOO_SMALL;

  /* Type.  */
  TLV_DECODE_GETW (msg->type);

  /* Status.  */
  TLV_DECODE_GETW (msg->status);

  switch (msg->type)
    {
    case NSM_MSG_IPV4_MRIB_NOTIFICATION_VIF_ADD:
    case NSM_MSG_IPV4_MRIB_NOTIFICATION_VIF_DEL:
      ret = nsm_decode_ipv4_mrib_vif_notification (pnt, size, &msg->key.vif_key);
      if (ret < 0)
        return ret;
      break;

    case NSM_MSG_IPV4_MRIB_NOTIFICATION_MRT_ADD:
    case NSM_MSG_IPV4_MRIB_NOTIFICATION_MRT_DEL:
    case NSM_MSG_IPV4_MRIB_NOTIFICATION_CLR_MRT:
      ret = nsm_decode_ipv4_mrib_mrt_notification (pnt, size, &msg->key.mrt_key);
      if (ret < 0)
        return ret;
      break;

    case NSM_MSG_IPV4_MRIB_NOTIFICATION_MCAST:
    case NSM_MSG_IPV4_MRIB_NOTIFICATION_NO_MCAST:
      break;
    }

  return *pnt - sp;
}

s_int32_t
nsm_parse_ipv4_mrib_notification (u_char **pnt, u_int16_t *size,
    struct nsm_msg_header *header, void *arg, NSM_CALLBACK callback)
{
  int ret = -1;
  struct nsm_msg_ipv4_mrib_notification msg;

  pal_mem_set (&msg, 0, sizeof (struct nsm_msg_ipv4_mrib_notification));

  ret = nsm_decode_ipv4_mrib_notification (pnt, size, &msg);
  if (ret < 0)
    return ret;

  if (callback)
    {
      ret = (*callback) (header, arg, &msg);
    }

  return ret;
}

void
nsm_ipv4_mrib_notification_dump (struct lib_globals *zg,
    struct nsm_msg_ipv4_mrib_notification *msg)
{
  zlog_info (zg, "IPV4 MRIB notification:");

  zlog_info (zg, " Type:%u (%s)", msg->type,
      nsm_ipv4_mrib_notif_type_str (msg->type));

  zlog_info (zg, " Status:%u (%s)", msg->status,
      nsm_ipv4_mrib_notif_status_str (msg->status));

  switch (msg->type)
    {
    case NSM_MSG_IPV4_MRIB_NOTIFICATION_VIF_ADD:
    case NSM_MSG_IPV4_MRIB_NOTIFICATION_VIF_DEL:
      zlog_info (zg, " Ifindex: %u", msg->key.vif_key.ifindex);
      zlog_info (zg, " Vif index: %u", msg->key.vif_key.vif_index);
      zlog_info (zg, " Local Address: %r", &msg->key.vif_key.local_addr);
      zlog_info (zg, " Remote Address: %r", &msg->key.vif_key.remote_addr);
      zlog_info (zg, " Flags: 0x%04x %s", msg->key.vif_key.flags,
          (CHECK_FLAG (msg->key.vif_key.flags, NSM_MSG_IPV4_VIF_FLAG_REGISTER) ?
           "Register Vif":
           (CHECK_FLAG (msg->key.vif_key.flags, NSM_MSG_IPV4_VIF_FLAG_TUNNEL) ?
            "Tunnel Vif":"")));
      break;

    case NSM_MSG_IPV4_MRIB_NOTIFICATION_MRT_ADD:
      zlog_info (zg, " MRT Source: %r", &msg->key.mrt_key.src);
      zlog_info (zg, " MRT Group: %r", &msg->key.mrt_key.group);
      zlog_info (zg, " Flags: 0x%04x %s %s", msg->key.mrt_key.flags,
          (CHECK_FLAG (msg->key.mrt_key.flags, NSM_MSG_IPV4_MRT_STAT_IMMEDIATE) ? "Stat Imm":""),
          (CHECK_FLAG (msg->key.mrt_key.flags, NSM_MSG_IPV4_MRT_STAT_TIMED) ? "Stat Timed":""));
      break;

    case NSM_MSG_IPV4_MRIB_NOTIFICATION_MRT_DEL:
    case NSM_MSG_IPV4_MRIB_NOTIFICATION_CLR_MRT:
      zlog_info (zg, " MRT Source: %r", &msg->key.mrt_key.src);
      zlog_info (zg, " MRT Group: %r", &msg->key.mrt_key.group);
      break;

    case NSM_MSG_IPV4_MRIB_NOTIFICATION_MCAST:
    case NSM_MSG_IPV4_MRIB_NOTIFICATION_NO_MCAST:
      break;
    }

  return;
}

/* Mtrace query message key */
static int
nsm_encode_mtrace_msg_key (u_char **pnt, u_int16_t *size,
    struct nsm_msg_mtrace_key *key)
{
  u_char *sp = *pnt;

  /* Check size  */
  if (*size < NSM_MSG_MTRACE_KEY_SIZE)
    return NSM_ERR_PKT_TOO_SMALL;

  /* IP source address.  */
  TLV_ENCODE_PUT_IN4_ADDR (&key->ip_src);
  /* Query Id.  */
  TLV_ENCODE_PUTL (key->query_id);

  return *pnt - sp;
}

static int
nsm_decode_mtrace_msg_key (u_char **pnt, u_int16_t *size,
    struct nsm_msg_mtrace_key *key)
{
  u_char *sp = *pnt;

  /* Check size  */
  if (*size < NSM_MSG_MTRACE_KEY_SIZE)
    return NSM_ERR_PKT_TOO_SMALL;

  /* IP source address.  */
  TLV_DECODE_GET_IN4_ADDR (&key->ip_src);
  /* Query Id.  */
  TLV_DECODE_GETL (key->query_id);

  return *pnt - sp;
}

static void
nsm_mtrace_key_dump(struct lib_globals *zg,
    struct nsm_msg_mtrace_key *key)
{
  zlog_info (zg, " IP Src Addr: %r", &key->ip_src);
  zlog_info (zg, " Query Id:    %u", key->query_id);
  return;
}

/* Mtrace query */
s_int32_t
nsm_encode_mtrace_query_msg (u_char **pnt, u_int16_t *size,
    struct nsm_msg_mtrace_query *msg)
{
  u_char *sp = *pnt;

  /* Check size  */
  if (*size < NSM_MSG_MTRACE_QUERY_SIZE)
    return NSM_ERR_PKT_TOO_SMALL;

  /* Key.  */
  nsm_encode_mtrace_msg_key (pnt, size, &msg->key);
  /* Mtrace source address.  */
  TLV_ENCODE_PUT_IN4_ADDR (&msg->q_src);
  /* Oif index */
  TLV_ENCODE_PUTL (msg->oif_index);
  /* Fwd Code */
  TLV_ENCODE_PUTC (msg->fwd_code);

  return *pnt - sp;
}

s_int32_t
nsm_decode_mtrace_query_msg (u_char **pnt, u_int16_t *size,
    struct nsm_msg_mtrace_query *msg)
{
  u_char *sp = *pnt;

  /* Check size  */
  if (*size < NSM_MSG_MTRACE_QUERY_SIZE)
    return NSM_ERR_PKT_TOO_SMALL;

  /* Key.  */
  nsm_decode_mtrace_msg_key (pnt, size, &msg->key);
  /* Mtrace source address.  */
  TLV_DECODE_GET_IN4_ADDR (&msg->q_src);
  /* Oif index */
  TLV_DECODE_GETL (msg->oif_index);
  /* Fwd Code */
  TLV_DECODE_GETC (msg->fwd_code);

  return *pnt - sp;
}

s_int32_t
nsm_parse_mtrace_query_msg (u_char **pnt, u_int16_t *size,
    struct nsm_msg_header *header, void *arg, NSM_CALLBACK callback)
{
  int ret = -1;
  struct nsm_msg_mtrace_query msg;

  pal_mem_set (&msg, 0, sizeof (struct nsm_msg_mtrace_query));

  ret = nsm_decode_mtrace_query_msg (pnt, size, &msg);
  if (ret < 0)
    return ret;

  if (callback)
    {
      ret = (*callback) (header, arg, &msg);
    }

  return ret;
}

void
nsm_mtrace_query_dump_msg (struct lib_globals *zg,
    struct nsm_msg_mtrace_query *msg)
{
  zlog_info (zg, "Mtrace Query msg:");

  nsm_mtrace_key_dump (zg, &msg->key);
  zlog_info (zg, " Query Source Addr: %r", &msg->q_src);
  zlog_info (zg, " Oif Ifindex:       %u", msg->oif_index);
  zlog_info (zg, " Fwd code:          %u (%s)", msg->fwd_code,
      mtrace_fwd_code_str (msg->fwd_code));

  return;
}

/* Mtrace request */
s_int32_t
nsm_encode_mtrace_request_msg (u_char **pnt, u_int16_t *size,
    struct nsm_msg_mtrace_request *msg)
{
  u_char *sp = *pnt;

  /* Check size  */
  if (*size < NSM_MSG_MTRACE_REQUEST_SIZE)
    return NSM_ERR_PKT_TOO_SMALL;

  /* Key.  */
  nsm_encode_mtrace_msg_key (pnt, size, &msg->key);
  /* Req num */
  TLV_ENCODE_PUTL (msg->req_num);
  /* Mtrace source address.  */
  TLV_ENCODE_PUT_IN4_ADDR (&msg->q_src);
  /* Mtrace group address.  */
  TLV_ENCODE_PUT_IN4_ADDR (&msg->q_grp);
  /* Oif index */
  TLV_ENCODE_PUTL (msg->oif_index);
  /* Iif index */
  TLV_ENCODE_PUTL (msg->iif_index);
  /* Previous hop address.  */
  TLV_ENCODE_PUT_IN4_ADDR (&msg->phop_addr);
  /* Rtg Proto */
  TLV_ENCODE_PUTC (msg->rtg_proto);
  /* Src Mask */
  TLV_ENCODE_PUTC (msg->src_mask);
  /* Fwd Code */
  TLV_ENCODE_PUTC (msg->fwd_code);
  /* Flags */
  TLV_ENCODE_PUTC (msg->flags);

  return *pnt - sp;
}

s_int32_t
nsm_decode_mtrace_request_msg (u_char **pnt, u_int16_t *size,
    struct nsm_msg_mtrace_request *msg)
{
  u_char *sp = *pnt;

  /* Check size  */
  if (*size < NSM_MSG_MTRACE_REQUEST_SIZE)
    return NSM_ERR_PKT_TOO_SMALL;

  /* Key.  */
  nsm_decode_mtrace_msg_key (pnt, size, &msg->key);
  /* Req num */
  TLV_DECODE_GETL (msg->req_num);
  /* Mtrace source address.  */
  TLV_DECODE_GET_IN4_ADDR (&msg->q_src);
  /* Mtrace group address.  */
  TLV_DECODE_GET_IN4_ADDR (&msg->q_grp);
  /* Oif index */
  TLV_DECODE_GETL (msg->oif_index);
  /* Iif index */
  TLV_DECODE_GETL (msg->iif_index);
  /* Previous hop address.  */
  TLV_DECODE_GET_IN4_ADDR (&msg->phop_addr);
  /* Rtg Proto */
  TLV_DECODE_GETC (msg->rtg_proto);
  /* Src Mask */
  TLV_DECODE_GETC (msg->src_mask);
  /* Fwd Code */
  TLV_DECODE_GETC (msg->fwd_code);
  /* Flags */
  TLV_DECODE_GETC (msg->flags);

  return *pnt - sp;
}

void
nsm_mtrace_request_dump_msg (struct lib_globals *zg,
    struct nsm_msg_mtrace_request *msg)
{
  zlog_info (zg, "Mtrace Request msg:");

  nsm_mtrace_key_dump (zg, &msg->key);
  zlog_info (zg, " Request Num:       %u", msg->req_num);
  zlog_info (zg, " Query Source Addr: %r", &msg->q_src);
  zlog_info (zg, " Query Group  Addr: %r", &msg->q_grp);
  zlog_info (zg, " Oif Ifindex:       %u", msg->oif_index);
  zlog_info (zg, " Iif Ifindex:       %u", msg->iif_index);
  zlog_info (zg, " Prev Hop  Addr:    %r", &msg->phop_addr);
  zlog_info (zg, " Rtg Proto:         %u (%s)", msg->rtg_proto,
      mtrace_rtg_proto_str (msg->rtg_proto));
  zlog_info (zg, " Src Mask:          0x%01x", msg->src_mask);
  zlog_info (zg, "   S-Bit:           %u",
      MTRACE_RESP_GET_S_BIT (msg->src_mask));
  zlog_info (zg, "   Mask:            %u",
      MTRACE_RESP_GET_SRC_MASK (msg->src_mask));
  zlog_info (zg, " Fwd code:          %u (%s)", msg->fwd_code,
      mtrace_fwd_code_str (msg->fwd_code));
  zlog_info (zg, " Flags:             0x%01x", msg->flags);

  return;
}

s_int32_t
nsm_parse_mtrace_request_msg (u_char **pnt, u_int16_t *size,
    struct nsm_msg_header *header, void *arg, NSM_CALLBACK callback)
{
  int ret = -1;
  struct nsm_msg_mtrace_request msg;

  pal_mem_set (&msg, 0, sizeof (struct nsm_msg_mtrace_request));

  ret = nsm_decode_mtrace_request_msg (pnt, size, &msg);
  if (ret < 0)
    return ret;

  if (callback)
    {
      ret = (*callback) (header, arg, &msg);
    }

  return ret;
}

s_int32_t
nsm_encode_igmp (u_int8_t **pnt,
                 u_int16_t *size,
                 struct nsm_msg_igmp *msg)
{
  u_int32_t idx;
  u_int8_t *sp;

  sp = *pnt;

  /* Check size */
  if (*size < (NSM_MSG_IGMP_SIZE + sizeof (struct pal_in4_addr)
               * msg->num_srcs))
    return NSM_ERR_PKT_TOO_SMALL;

  /* Encode IFINDEX */
  TLV_ENCODE_PUTL (msg->ifindex);

  /* Encode Filter-Mode */
  TLV_ENCODE_PUTW (msg->filt_mode);

  /* Encode Num Sources */
  TLV_ENCODE_PUTW (msg->num_srcs);

  /* Encode Group Address */
  TLV_ENCODE_PUT_IN4_ADDR (&msg->grp_addr);

  /* Encode Sources List */
  if (msg->num_srcs)
    for (idx = 0; idx < msg->num_srcs; idx++)
      TLV_ENCODE_PUT_IN4_ADDR (& msg->src_addr_list [idx]);

  return *pnt - sp;
}

s_int32_t
nsm_decode_igmp (u_int8_t **pnt,
                 u_int16_t *size,
                 struct nsm_msg_igmp **msg)
{
  struct nsm_msg_igmp msg_tmp;
  u_int32_t idx;
  u_int8_t *sp;

  sp = *pnt;

  /* Check size */
  if (*size < NSM_MSG_IGMP_SIZE)
    return NSM_ERR_PKT_TOO_SMALL;

  /* Zero the temp mesg structure */
  pal_mem_set (&msg_tmp, 0, sizeof (struct nsm_msg_igmp));

  /* Decode IFINDEX */
  TLV_DECODE_GETL (msg_tmp.ifindex);

  /* Decode Filter-Mode */
  TLV_DECODE_GETW (msg_tmp.filt_mode);

  /* Decode Num Sources */
  TLV_DECODE_GETW (msg_tmp.num_srcs);

  /* Decode Group Address */
  TLV_DECODE_GET_IN4_ADDR (&msg_tmp.grp_addr);

  /* Allocate Memory for the IGMP Message (few extra bytes ok) */
  (*msg) = XCALLOC (MTYPE_TMP, sizeof (struct nsm_msg_igmp)
                               + sizeof (struct pal_in4_addr)
                               * msg_tmp.num_srcs);

  if (! (*msg))
    return NSM_ERR_PKT_TOO_SMALL;

  /* Copy over bytes currently decoded */
  pal_mem_cpy ((*msg), &msg_tmp, sizeof (struct nsm_msg_igmp));

  /* Decode Sources List */
  if ((*msg)->num_srcs)
    for (idx = 0; idx < (*msg)->num_srcs; idx++)
      TLV_DECODE_GET_IN4_ADDR (& (*msg)->src_addr_list [idx]);

  return *pnt - sp;
}

s_int32_t
nsm_parse_igmp (u_char **pnt, u_int16_t *size,
                struct nsm_msg_header *header, void *arg,
                NSM_CALLBACK callback)
{
  struct nsm_msg_igmp *msg;
  s_int32_t ret;

  msg = NULL;
  ret = 0;

  ret = nsm_decode_igmp (pnt, size, &msg);
  if (ret < 0)
    goto EXIT;

  if (callback)
    ret = (*callback) (header, arg, msg);

  EXIT:
  if (msg)
    XFREE (MTYPE_TMP, msg);

  return ret;
}

s_int32_t
nsm_igmp_dump (struct lib_globals *zg,
               struct nsm_msg_igmp *msg)
{
  u_int32_t idx;

  zlog_info (zg, "IGMP Message");

  zlog_info (zg, " Ifindex: %d", msg->ifindex);
  zlog_info (zg, " Filter Mode: %s", msg->filt_mode ? "EXCL" : "INCL");
  zlog_info (zg, " Num Source Addresses: %d", msg->num_srcs);
  zlog_info (zg, " Group Address: %r", &msg->grp_addr);

  if (msg->num_srcs)
    {
      zlog_info (zg, " Source Address List:");
      for (idx = 0; idx < msg->num_srcs; idx++)
        zlog_info (zg, "   %r", &msg->src_addr_list [idx]);
    }

  return 0;
}

#endif /* HAVE_MCAST_IPV4 */

#ifdef HAVE_MCAST_IPV6
static const char *ipv6_mrib_notif_type_str[] =
{
  "MIF Add Notification",                   /* MIF Add */
  "MIF Del Notification",                   /* MIF Del */
  "MRT Add Notification",                   /* MRT Add */
  "MRT Del Notification",                   /* MRT Del */
  "MCAST Notification",                     /* MCAST */
  "NO-MCAST Notification",                  /* MCAST */
  "CLR MRT Notification"                    /* CLR MRT */
};

static const char *ipv6_mrib_notif_status_str[] =
{
  "MIF Exists",
  "MIF Not Exist",
  "MIF No Index",
  "MIF Not Belong",
  "MIF Fwd Add Err",
  "MIF Fwd Del Err",
  "MIF Add Success",
  "MIF Del Success",
  "MRT Limit Exceeded",
  "MRT Exists",
  "MRT Not Exist",
  "MRT Not Belong",
  "MRT Fwd Add Err",
  "MRT Fwd Del Err",
  "MRT Add Success",
  "MRT Del Success",
  "MRIB Internal Error",
  "N/A"
};

static const char *
nsm_ipv6_mrib_notif_type_str (u_int16_t type)
{
  if (type < 7)
    return ipv6_mrib_notif_type_str[type];

  return "Invalid";
}

static const char *
nsm_ipv6_mrib_notif_status_str (u_int16_t status)
{
  if (status < 17)
    return ipv6_mrib_notif_status_str[status];

  return "Invalid";
}

/* IPV6 Mif Add */
s_int32_t
nsm_encode_ipv6_mif_add (u_char **pnt, u_int16_t *size,
    struct nsm_msg_ipv6_mif_add *msg)
{
  u_char *sp = *pnt;

  /* Check size.  */
  if (*size < NSM_MSG_IPV6_MIF_ADD_SIZE)
    return NSM_ERR_PKT_TOO_SMALL;

  /* Ifindex.  */
  TLV_ENCODE_PUTL (msg->ifindex);

  /* Flags.  */
  TLV_ENCODE_PUTW (msg->flags);

  return *pnt - sp;
}

s_int32_t
nsm_decode_ipv6_mif_add (u_char **pnt, u_int16_t *size,
    struct nsm_msg_ipv6_mif_add *msg)
{
  u_char *sp = *pnt;

  /* Check size.  */
  if (*size < NSM_MSG_IPV6_MIF_ADD_SIZE)
    return NSM_ERR_PKT_TOO_SMALL;

  /* Ifindex.  */
  TLV_DECODE_GETL (msg->ifindex);

  /* Flags.  */
  TLV_DECODE_GETW (msg->flags);

  return *pnt - sp;
}

s_int32_t
nsm_parse_ipv6_mif_add (u_char **pnt, u_int16_t *size,
    struct nsm_msg_header *header, void *arg, NSM_CALLBACK callback)
{
  int ret = -1;
  struct nsm_msg_ipv6_mif_add msg;

  pal_mem_set (&msg, 0, sizeof (struct nsm_msg_ipv6_mif_add));

  ret = nsm_decode_ipv6_mif_add (pnt, size, &msg);
  if (ret < 0)
    return ret;

  if (callback)
    {
      ret = (*callback) (header, arg, &msg);
      if (ret < 0)
        return ret;
    }

  return ret;
}

void
nsm_ipv6_mif_add_dump (struct lib_globals *zg,
    struct nsm_msg_ipv6_mif_add *msg)
{
  zlog_info (zg, "IPV6 Mif add:");
  zlog_info (zg, " Ifindex: %d", msg->ifindex);
  zlog_info (zg, " Flags: 0x%04x %s", msg->flags,
      (CHECK_FLAG (msg->flags, NSM_MSG_IPV6_MIF_FLAG_REGISTER) ? "Register Mif":""));
  return;
}

/* IPV6 Mif Del */
s_int32_t
nsm_encode_ipv6_mif_del (u_char **pnt, u_int16_t *size,
    struct nsm_msg_ipv6_mif_del *msg)
{
  u_char *sp = *pnt;

  /* Check size.  */
  if (*size < NSM_MSG_IPV6_MIF_DEL_SIZE)
    return NSM_ERR_PKT_TOO_SMALL;

  /* Vif index.  */
  TLV_ENCODE_PUTW (msg->mif_index);

  /* flags  */
  TLV_ENCODE_PUTW (msg->flags);

  return *pnt - sp;
}

s_int32_t
nsm_decode_ipv6_mif_del (u_char **pnt, u_int16_t *size,
    struct nsm_msg_ipv6_mif_del *msg)
{
  u_char *sp = *pnt;

  /* Check size.  */
  if (*size < NSM_MSG_IPV6_MIF_DEL_SIZE)
    return NSM_ERR_PKT_TOO_SMALL;

  /* Vif index.  */
  TLV_DECODE_GETW (msg->mif_index);

  /* Flags.  */
  TLV_DECODE_GETW (msg->flags);

  return *pnt - sp;
}

s_int32_t
nsm_parse_ipv6_mif_del (u_char **pnt, u_int16_t *size,
    struct nsm_msg_header *header, void *arg, NSM_CALLBACK callback)
{
  int ret = -1;
  struct nsm_msg_ipv6_mif_del msg;

  pal_mem_set (&msg, 0, sizeof (struct nsm_msg_ipv6_mif_del));

  ret = nsm_decode_ipv6_mif_del (pnt, size, &msg);
  if (ret < 0)
    return ret;

  if (callback)
    {
      ret = (*callback) (header, arg, &msg);
      if (ret < 0)
        return ret;
    }

  return ret;
}

void
nsm_ipv6_mif_del_dump (struct lib_globals *zg,
    struct nsm_msg_ipv6_mif_del *msg)
{
  zlog_info (zg, "IPV6 Vif del:");
  zlog_info (zg, " Vif index: %d", msg->mif_index);
  zlog_info (zg, " Flags: 0x%04x %s", msg->flags,
      (CHECK_FLAG (msg->flags, NSM_MSG_IPV6_MIF_FLAG_REGISTER) ? "Register Mif":""));
  return;
}

/* IPV6 Mrt add */
s_int32_t
nsm_encode_ipv6_mrt_add (u_char **pnt, u_int16_t *size,
    struct nsm_msg_ipv6_mrt_add *msg)
{
  u_char *sp = *pnt;
  int i = 0;

  /* Check size.  */
  if (*size < (NSM_MSG_IPV6_MRT_ADD_SIZE +
        (msg->num_olist * sizeof (u_int16_t))))
    return NSM_ERR_PKT_TOO_SMALL;

  /* Source address.  */
  TLV_ENCODE_PUT_IN6_ADDR (&msg->src);

  /* Group address.  */
  TLV_ENCODE_PUT_IN6_ADDR (&msg->group);

  /* RPF address.  */
  TLV_ENCODE_PUT_IN6_ADDR (&msg->rpf_addr);

  /* Flags.  */
  TLV_ENCODE_PUTW (msg->flags);

  /* Stat time.  */
  TLV_ENCODE_PUTW (msg->stat_time);

  /* Pkt count.  */
  TLV_ENCODE_PUTW (msg->pkt_count);

  /* Iif.  */
  TLV_ENCODE_PUTW (msg->iif);

  /* Number in olist.  */
  TLV_ENCODE_PUTW (msg->num_olist);

  /* Encode olist */
  while (i < msg->num_olist)
    {
      TLV_ENCODE_PUTW (msg->olist_mifs[i]);
      i++;
    }

  return *pnt - sp;
}

/* Note that this function allocates memory to hold decoded olist.
 * This memory is expected to be freed by the caller of this function.
 */
s_int32_t
nsm_decode_ipv6_mrt_add (u_char **pnt, u_int16_t *size,
    struct nsm_msg_ipv6_mrt_add *msg)
{
  u_char *sp = *pnt;
  int i = 0;

  /* Check minimum required size.  */
  if (*size < NSM_MSG_IPV6_MRT_ADD_SIZE)
    return NSM_ERR_PKT_TOO_SMALL;

  /* Source address.  */
  TLV_DECODE_GET_IN6_ADDR (&msg->src);

  /* Group address.  */
  TLV_DECODE_GET_IN6_ADDR (&msg->group);

  /* RPF address.  */
  TLV_DECODE_GET_IN6_ADDR (&msg->rpf_addr);

  /* Flags.  */
  TLV_DECODE_GETW (msg->flags);

  /* Stat time.  */
  TLV_DECODE_GETW (msg->stat_time);

  /* Pkt count.  */
  TLV_DECODE_GETW (msg->pkt_count);

  /* Iif.  */
  TLV_DECODE_GETW (msg->iif);

  /* Number in olist.  */
  TLV_DECODE_GETW (msg->num_olist);

  if (msg->num_olist == 0)
    return *pnt - sp;

  /* Check minimum required size.  */
  if (*size < (msg->num_olist * sizeof (u_int16_t)))
    return NSM_ERR_PKT_TOO_SMALL;

  /* Decode olist */
  msg->olist_mifs = XMALLOC (MTYPE_TMP, (msg->num_olist * sizeof (u_int16_t)));
  if (msg->olist_mifs == NULL)
    return -1;

  while (i < msg->num_olist)
    {
      TLV_DECODE_GETW (msg->olist_mifs[i]);
      i++;
    }

  return *pnt - sp;
}

s_int32_t
nsm_parse_ipv6_mrt_add (u_char **pnt, u_int16_t *size,
    struct nsm_msg_header *header, void *arg, NSM_CALLBACK callback)
{
  int ret = -1;
  struct nsm_msg_ipv6_mrt_add msg;

  pal_mem_set (&msg, 0, sizeof (struct nsm_msg_ipv6_mrt_add));

  ret = nsm_decode_ipv6_mrt_add (pnt, size, &msg);
  if (ret < 0)
    return ret;

  if (callback)
    {
      ret = (*callback) (header, arg, &msg);
    }

  /* Free the allocated olist */
  if (msg.olist_mifs != NULL)
    XFREE (MTYPE_TMP, msg.olist_mifs);

  return ret;
}

void
nsm_ipv6_mrt_add_dump (struct lib_globals *zg,
    struct nsm_msg_ipv6_mrt_add *msg)
{
  int i = 0;
  char buf[71];
  char *cp;
  int wlen;

  zlog_info (zg, "IPV6 MRT add:");
  zlog_info (zg, " Source Address: %R", &msg->src);
  zlog_info (zg, " Group Address:  %R", &msg->group);
  zlog_info (zg, " RPF Address:    %R", &msg->rpf_addr);
  zlog_info (zg, " Flags: 0x%04x %s %s", msg->flags,
          (CHECK_FLAG (msg->flags, NSM_MSG_IPV6_MRT_STAT_IMMEDIATE) ? "Stat Imm" : ""),
          (CHECK_FLAG (msg->flags, NSM_MSG_IPV6_MRT_STAT_TIMED) ? "Stat Timed" : ""));
  zlog_info (zg, " Stat Time: %d", msg->stat_time);
  zlog_info (zg, " Pkt count: %d", msg->pkt_count);
  zlog_info (zg, " iif: %d", msg->iif);
  zlog_info (zg, " olist number: %d", msg->num_olist);
  zlog_info (zg, " olist :");

  cp = buf;
  wlen = 0;
  /* We will print 10 elements in the olist per row */
  while (i < msg->num_olist)
    {
      /* If this is the tenth member, print it */
      if (((i + 1) % 10) == 0)
        {
          wlen = pal_snprintf (cp, sizeof (buf) - (cp - buf), "%d", msg->olist_mifs[i]);

          cp += wlen;

          /* Terminate the buffer */
          *cp = '\0';

          /* Now print the row */
          zlog_info (zg, "   %s", buf);

          /* Initialize pointers and continue */
          cp = buf;

          i++;

          continue;
        }

      /* Add this element to the buffer */
      if (i == msg->num_olist - 1)
        wlen = pal_snprintf (cp, sizeof (buf) - (cp - buf), "%d", msg->olist_mifs[i]);
      else
        wlen = pal_snprintf (cp, sizeof (buf) - (cp - buf), "%d, ", msg->olist_mifs[i]);

      cp += wlen;

      i++;
    }

  /* If the number in olist is not a multiple of 10, then we have to print
   * the last row.
   */
  if ((msg->num_olist % 10) != 0)
    {
      /* Terminate the buffer */
      *cp = '\0';

      zlog_info (zg, "   %s", buf);
    }

  return;
}

/* IPV6 Mrt del */
s_int32_t
nsm_encode_ipv6_mrt_del (u_char **pnt, u_int16_t *size,
    struct nsm_msg_ipv6_mrt_del *msg)
{
  u_char *sp = *pnt;

  /* Check size.  */
  if (*size < NSM_MSG_IPV6_MRT_DEL_SIZE)
    return NSM_ERR_PKT_TOO_SMALL;

  /* Source address.  */
  TLV_ENCODE_PUT_IN6_ADDR (&msg->src);

  /* Group address.  */
  TLV_ENCODE_PUT_IN6_ADDR (&msg->group);

  return *pnt - sp;
}

s_int32_t
nsm_decode_ipv6_mrt_del (u_char **pnt, u_int16_t *size,
    struct nsm_msg_ipv6_mrt_del *msg)
{
  u_char *sp = *pnt;

  /* Check minimum required size.  */
  if (*size < NSM_MSG_IPV6_MRT_DEL_SIZE)
    return NSM_ERR_PKT_TOO_SMALL;

  /* Source address.  */
  TLV_DECODE_GET_IN6_ADDR (&msg->src);

  /* Group address.  */
  TLV_DECODE_GET_IN6_ADDR (&msg->group);

  return *pnt - sp;
}

s_int32_t
nsm_parse_ipv6_mrt_del (u_char **pnt, u_int16_t *size,
    struct nsm_msg_header *header, void *arg, NSM_CALLBACK callback)
{
  int ret = -1;
  struct nsm_msg_ipv6_mrt_del msg;

  pal_mem_set (&msg, 0, sizeof (struct nsm_msg_ipv6_mrt_del));

  ret = nsm_decode_ipv6_mrt_del (pnt, size, &msg);
  if (ret < 0)
    return ret;

  if (callback)
    {
      ret = (*callback) (header, arg, &msg);
    }

  return ret;
}

void
nsm_ipv6_mrt_del_dump (struct lib_globals *zg,
    struct nsm_msg_ipv6_mrt_del *msg)
{
  zlog_info (zg, "IPV6 MRT del:");
  zlog_info (zg, " Source Address: %R", &msg->src);
  zlog_info (zg, " Group Address: %R", &msg->group);
  return;
}

/* IPV6 Mrt Stat Flags Update */
s_int32_t
nsm_encode_ipv6_mrt_stat_flags_update (u_char **pnt, u_int16_t *size,
    struct nsm_msg_ipv6_mrt_stat_flags_update *msg)
{
  u_char *sp = *pnt;

  /* Check size.  */
  if (*size < NSM_MSG_IPV6_MRT_STAT_FLAGS_UPDATE_SIZE)
    return NSM_ERR_PKT_TOO_SMALL;

  /* Source address.  */
  TLV_ENCODE_PUT_IN6_ADDR (&msg->src);

  /* Group address.  */
  TLV_ENCODE_PUT_IN6_ADDR (&msg->group);

  /* Flags.  */
  TLV_ENCODE_PUTW (msg->flags);

  /* Stat time.  */
  TLV_ENCODE_PUTW (msg->stat_time);

  return *pnt - sp;
}

s_int32_t
nsm_decode_ipv6_mrt_stat_flags_update (u_char **pnt, u_int16_t *size,
    struct nsm_msg_ipv6_mrt_stat_flags_update *msg)
{
  u_char *sp = *pnt;

  /* Check minimum required size.  */
  if (*size < NSM_MSG_IPV6_MRT_STAT_FLAGS_UPDATE_SIZE)
    return NSM_ERR_PKT_TOO_SMALL;

  /* Source address.  */
  TLV_DECODE_GET_IN6_ADDR (&msg->src);

  /* Group address.  */
  TLV_DECODE_GET_IN6_ADDR (&msg->group);

  /* Flags.  */
  TLV_DECODE_GETW (msg->flags);

  /* Stat time.  */
  TLV_DECODE_GETW (msg->stat_time);

  return *pnt - sp;
}

s_int32_t
nsm_parse_ipv6_mrt_stat_flags_update (u_char **pnt, u_int16_t *size,
    struct nsm_msg_header *header, void *arg, NSM_CALLBACK callback)
{
  int ret = -1;
  struct nsm_msg_ipv6_mrt_stat_flags_update msg;

  pal_mem_set (&msg, 0, sizeof (struct nsm_msg_ipv6_mrt_stat_flags_update));

  ret = nsm_decode_ipv6_mrt_stat_flags_update (pnt, size, &msg);
  if (ret < 0)
    return ret;

  if (callback)
    {
      ret = (*callback) (header, arg, &msg);
    }

  return ret;
}

void
nsm_ipv6_mrt_stat_flags_update_dump (struct lib_globals *zg,
    struct nsm_msg_ipv6_mrt_stat_flags_update *msg)
{
  zlog_info (zg, "IPV6 MRT Stat Flags Update:");
  zlog_info (zg, " Source Address: %R", &msg->src);
  zlog_info (zg, " Group Address: %R", &msg->group);
  zlog_info (zg, " Flags: 0x%04x %s %s", msg->flags,
      (CHECK_FLAG (msg->flags, NSM_MSG_IPV6_MRT_STAT_IMMEDIATE) ? "Stat Imm":""),
      (CHECK_FLAG (msg->flags, NSM_MSG_IPV6_MRT_STAT_TIMED) ? "Stat Timed":""));
  zlog_info (zg, " Stat Time: %d", msg->stat_time);

  return;
}

/* IPV6 Mrt State Refresh Flag Update */
int
nsm_encode_ipv6_mrt_state_refresh_flag_update (u_char **pnt, u_int16_t *size,
    struct nsm_msg_ipv6_mrt_state_refresh_flag_update *msg)
{
  u_char *sp = *pnt;

  /* Check size.  */
  if (*size < NSM_MSG_IPV6_MRT_ST_REFRESH_FLAG_UPDATE_SIZE)
    return NSM_ERR_PKT_TOO_SMALL;

  /* Group address.  */
  TLV_ENCODE_PUT_IN6_ADDR (&msg->grp);

  /* Interface Index */
  TLV_ENCODE_PUTL (msg->ifindex);

  return *pnt - sp;
}

int
nsm_decode_ipv6_mrt_state_refresh_flag_update (u_char **pnt, u_int16_t *size,
    struct nsm_msg_ipv6_mrt_state_refresh_flag_update *msg)
{
  u_char *sp = *pnt;

  /* Check minimum required size.  */
  if (*size < NSM_MSG_IPV6_MRT_ST_REFRESH_FLAG_UPDATE_SIZE)
    return NSM_ERR_PKT_TOO_SMALL;

  /* Group address.  */
  TLV_DECODE_GET_IN6_ADDR (&msg->grp);


  /* Interface Index */
  TLV_DECODE_GETL (msg->ifindex);

  return *pnt - sp;
}

s_int32_t
nsm_parse_ipv6_mrt_state_refresh_flag_update (u_char **pnt, u_int16_t *size,
    struct nsm_msg_header *header, void *arg, NSM_CALLBACK callback)
{
  struct nsm_msg_ipv6_mrt_state_refresh_flag_update msg;
  int ret = -1;

  pal_mem_set (&msg, 0,
               sizeof (struct nsm_msg_ipv6_mrt_state_refresh_flag_update));

  ret = nsm_decode_ipv6_mrt_state_refresh_flag_update (pnt, size, &msg);
  if (ret < 0)
    return ret;

  if (callback)
    {
      ret = (*callback) (header, arg, &msg);
    }

  return ret;
}

/* IPV6 Mrt NOCACHE */
s_int32_t
nsm_encode_ipv6_mrt_nocache (u_char **pnt, u_int16_t *size,
    struct nsm_msg_ipv6_mrt_nocache *msg)
{
  u_char *sp = *pnt;

  /* Check size.  */
  if (*size < NSM_MSG_IPV6_MRT_NOCACHE_SIZE)
    return NSM_ERR_PKT_TOO_SMALL;

  /* Source address.  */
  TLV_ENCODE_PUT_IN6_ADDR (&msg->src);

  /* Group address.  */
  TLV_ENCODE_PUT_IN6_ADDR (&msg->group);

  /* mif index.  */
  TLV_ENCODE_PUTW (msg->mif_index);

  return *pnt - sp;
}

s_int32_t
nsm_decode_ipv6_mrt_nocache (u_char **pnt, u_int16_t *size,
    struct nsm_msg_ipv6_mrt_nocache *msg)
{
  u_char *sp = *pnt;

  /* Check minimum required size.  */
  if (*size < NSM_MSG_IPV6_MRT_NOCACHE_SIZE)
    return NSM_ERR_PKT_TOO_SMALL;

  /* Source address.  */
  TLV_DECODE_GET_IN6_ADDR (&msg->src);

  /* Group address.  */
  TLV_DECODE_GET_IN6_ADDR (&msg->group);

  /* mif index.  */
  TLV_DECODE_GETW (msg->mif_index);

  return *pnt - sp;
}

s_int32_t
nsm_parse_ipv6_mrt_nocache (u_char **pnt, u_int16_t *size,
    struct nsm_msg_header *header, void *arg, NSM_CALLBACK callback)
{
  int ret = -1;
  struct nsm_msg_ipv6_mrt_nocache msg;

  pal_mem_set (&msg, 0, sizeof (struct nsm_msg_ipv6_mrt_nocache));

  ret = nsm_decode_ipv6_mrt_nocache (pnt, size, &msg);
  if (ret < 0)
    return ret;

  if (callback)
    {
      ret = (*callback) (header, arg, &msg);
    }

  return ret;
}

void
nsm_ipv6_mrt_nocache_dump (struct lib_globals *zg,
    struct nsm_msg_ipv6_mrt_nocache *msg)
{
  zlog_info (zg, "IPV6 MRT NOCACHE:");
  zlog_info (zg, " Source Address: %R", &msg->src);
  zlog_info (zg, " Group Address: %R", &msg->group);
  zlog_info (zg, " Mif index: %d", msg->mif_index);
  return;
}

/* IPV6 Mrt WRONGMIF */
s_int32_t
nsm_encode_ipv6_mrt_wrongmif (u_char **pnt, u_int16_t *size,
    struct nsm_msg_ipv6_mrt_wrongmif *msg)
{
  u_char *sp = *pnt;

  /* Check size.  */
  if (*size < NSM_MSG_IPV6_MRT_WRONGMIF_SIZE)
    return NSM_ERR_PKT_TOO_SMALL;

  /* Source address.  */
  TLV_ENCODE_PUT_IN6_ADDR (&msg->src);

  /* Group address.  */
  TLV_ENCODE_PUT_IN6_ADDR (&msg->group);

  /* mif index.  */
  TLV_ENCODE_PUTW (msg->mif_index);

  return *pnt - sp;
}

s_int32_t
nsm_decode_ipv6_mrt_wrongmif (u_char **pnt, u_int16_t *size,
    struct nsm_msg_ipv6_mrt_wrongmif *msg)
{
  u_char *sp = *pnt;

  /* Check minimum required size.  */
  if (*size < NSM_MSG_IPV6_MRT_WRONGMIF_SIZE)
    return NSM_ERR_PKT_TOO_SMALL;

  /* Source address.  */
  TLV_DECODE_GET_IN6_ADDR (&msg->src);

  /* Group address.  */
  TLV_DECODE_GET_IN6_ADDR (&msg->group);

  /* mif index.  */
  TLV_DECODE_GETW (msg->mif_index);

  return *pnt - sp;
}

s_int32_t
nsm_parse_ipv6_mrt_wrongmif (u_char **pnt, u_int16_t *size,
    struct nsm_msg_header *header, void *arg, NSM_CALLBACK callback)
{
  int ret = -1;
  struct nsm_msg_ipv6_mrt_wrongmif msg;

  pal_mem_set (&msg, 0, sizeof (struct nsm_msg_ipv6_mrt_wrongmif));

  ret = nsm_decode_ipv6_mrt_wrongmif (pnt, size, &msg);
  if (ret < 0)
    return ret;

  if (callback)
    {
      ret = (*callback) (header, arg, &msg);
    }

  return ret;
}

void
nsm_ipv6_mrt_wrongmif_dump (struct lib_globals *zg,
    struct nsm_msg_ipv6_mrt_wrongmif *msg)
{
  zlog_info (zg, "IPV6 MRT WRONGMIF:");
  zlog_info (zg, " Source Address: %R", &msg->src);
  zlog_info (zg, " Group Address: %R", &msg->group);
  zlog_info (zg, " Mif index: %d", msg->mif_index);
  return;
}

/* IPV6 Mrt WHOLEPKT REQ */
s_int32_t
nsm_encode_ipv6_mrt_wholepkt_req (u_char **pnt, u_int16_t *size,
    struct nsm_msg_ipv6_mrt_wholepkt_req *msg)
{
  u_char *sp = *pnt;

  /* Check size.  */
  if (*size < NSM_MSG_IPV6_MRT_WHOLEPKT_REQ_SIZE)
    return NSM_ERR_PKT_TOO_SMALL;

  /* Source address.  */
  TLV_ENCODE_PUT_IN6_ADDR (&msg->src);

  /* Group address.  */
  TLV_ENCODE_PUT_IN6_ADDR (&msg->group);

  /* mif index.  */
  TLV_ENCODE_PUTW (msg->mif_index);

  /* Data Hop Limit */
  TLV_ENCODE_PUTC (msg->data_hlim);

  return *pnt - sp;
}

s_int32_t
nsm_decode_ipv6_mrt_wholepkt_req (u_char **pnt, u_int16_t *size,
    struct nsm_msg_ipv6_mrt_wholepkt_req *msg)
{
  u_char *sp = *pnt;

  /* Check minimum required size.  */
  if (*size < NSM_MSG_IPV6_MRT_WHOLEPKT_REQ_SIZE)
    return NSM_ERR_PKT_TOO_SMALL;

  /* Source address.  */
  TLV_DECODE_GET_IN6_ADDR (&msg->src);

  /* Group address.  */
  TLV_DECODE_GET_IN6_ADDR (&msg->group);

  /* mif index.  */
  TLV_DECODE_GETW (msg->mif_index);

  /* Data Hop Limit */
  TLV_DECODE_GETC (msg->data_hlim);

  return *pnt - sp;
}

s_int32_t
nsm_parse_ipv6_mrt_wholepkt_req (u_char **pnt, u_int16_t *size,
    struct nsm_msg_header *header, void *arg, NSM_CALLBACK callback)
{
  int ret = -1;
  struct nsm_msg_ipv6_mrt_wholepkt_req msg;

  pal_mem_set (&msg, 0, sizeof (struct nsm_msg_ipv6_mrt_wholepkt_req));

  ret = nsm_decode_ipv6_mrt_wholepkt_req (pnt, size, &msg);
  if (ret < 0)
    return ret;

  if (callback)
    {
      ret = (*callback) (header, arg, &msg);
    }

  return ret;
}

void
nsm_ipv6_mrt_wholepkt_req_dump (struct lib_globals *zg,
    struct nsm_msg_ipv6_mrt_wholepkt_req *msg)
{
  zlog_info (zg, "IPV6 MRT WHOLEPKT REQ:");
  zlog_info (zg, " Source Address: %R", &msg->src);
  zlog_info (zg, " Group Address: %R", &msg->group);
  zlog_info (zg, " Mif index: %d", msg->mif_index);
  zlog_info (zg, " Data Hop Limit: %d", msg->data_hlim);
  return;
}

/* IPV6 Mrt WHOLEPKT reply */
s_int32_t
nsm_encode_ipv6_mrt_wholepkt_reply (u_char **pnt, u_int16_t *size,
    struct nsm_msg_ipv6_mrt_wholepkt_reply *msg)
{
  u_char *sp = *pnt;

  /* Check size.  */
  if (*size < NSM_MSG_IPV6_MRT_WHOLEPKT_REPLY_SIZE)
    return NSM_ERR_PKT_TOO_SMALL;

  /* Source address.  */
  TLV_ENCODE_PUT_IN6_ADDR (&msg->src);

  /* Group address.  */
  TLV_ENCODE_PUT_IN6_ADDR (&msg->group);

  /* RP address.  */
  TLV_ENCODE_PUT_IN6_ADDR (&msg->rp_addr);

  /* Reg pkt src address.  */
  TLV_ENCODE_PUT_IN6_ADDR (&msg->src_addr);

  /* Identifier.  */
  TLV_ENCODE_PUTL (msg->id);

  /* reply.  */
  TLV_ENCODE_PUTW (msg->reply);

  /* Reg pkt chksum type.  */
  TLV_ENCODE_PUTW (msg->chksum_type);

  return *pnt - sp;
}

s_int32_t
nsm_decode_ipv6_mrt_wholepkt_reply (u_char **pnt, u_int16_t *size,
    struct nsm_msg_ipv6_mrt_wholepkt_reply *msg)
{
  u_char *sp = *pnt;

  /* Check minimum required size.  */
  if (*size < NSM_MSG_IPV6_MRT_WHOLEPKT_REPLY_SIZE)
    return NSM_ERR_PKT_TOO_SMALL;

  /* Source address.  */
  TLV_DECODE_GET_IN6_ADDR (&msg->src);

  /* Group address.  */
  TLV_DECODE_GET_IN6_ADDR (&msg->group);

  /* RP address.  */
  TLV_DECODE_GET_IN6_ADDR (&msg->rp_addr);

  /* Reg pkt src address.  */
  TLV_DECODE_GET_IN6_ADDR (&msg->src_addr);

  /* Identifier.  */
  TLV_DECODE_GETL (msg->id);

  /* Reply.  */
  TLV_DECODE_GETW (msg->reply);

  /* Reg pkt chksum type.  */
  TLV_DECODE_GETW (msg->chksum_type);

  return *pnt - sp;
}

s_int32_t
nsm_parse_ipv6_mrt_wholepkt_reply (u_char **pnt, u_int16_t *size,
    struct nsm_msg_header *header, void *arg, NSM_CALLBACK callback)
{
  int ret = -1;
  struct nsm_msg_ipv6_mrt_wholepkt_reply msg;

  pal_mem_set (&msg, 0, sizeof (struct nsm_msg_ipv6_mrt_wholepkt_reply));

  ret = nsm_decode_ipv6_mrt_wholepkt_reply (pnt, size, &msg);
  if (ret < 0)
    return ret;

  if (callback)
    {
      ret = (*callback) (header, arg, &msg);
    }

  return ret;
}

void
nsm_ipv6_mrt_wholepkt_reply_dump (struct lib_globals *zg,
    struct nsm_msg_ipv6_mrt_wholepkt_reply *msg)
{
  zlog_info (zg, "IPV6 MRT WHOLEPKT REPLY:");
  zlog_info (zg, " Source Address: %R", &msg->src);
  zlog_info (zg, " Group Address: %R", &msg->group);
  zlog_info (zg, " RP Address: %R", &msg->rp_addr);
  zlog_info (zg, " Pkt Src Address: %R", &msg->src_addr);
  zlog_info (zg, " Identifier: %u", msg->id);
  zlog_info (zg, " Reply: %u %s", msg->reply,
      ((msg->reply == NSM_MSG_IPV6_WHOLEPKT_REPLY_ACK) ? "ACK" : "NACK"));
  zlog_info (zg, " Chksum type: %u %s", msg->chksum_type,
      ((msg->chksum_type == NSM_MSG_IPV6_REG_PKT_CHKSUM_REG) ? "Regular" : "Cisco"));
  return;
}

/* IPV6 Stat update */
static int
nsm_encode_ipv6_mrt_stat_block (u_char **pnt, u_int16_t *size,
    struct nsm_msg_ipv6_mrt_stat_block *blk)
{
  u_char *sp = *pnt;

  /* Source Address */
  TLV_ENCODE_PUT_IN6_ADDR (&blk->src);

  /* Group Address */
  TLV_ENCODE_PUT_IN6_ADDR (&blk->group);

  /* Packets Forwarde */
  TLV_ENCODE_PUTL (blk->pkts_fwd);

  /* Bytes Forwarded */
  TLV_ENCODE_PUTL (blk->bytes_fwd);

  /* Flags */
  TLV_ENCODE_PUTW (blk->flags);

  /* Reserved */
  TLV_ENCODE_PUTW (blk->resv);

  return *pnt - sp;
}

s_int32_t
nsm_encode_ipv6_mrt_stat_update (u_char **pnt, u_int16_t *size,
    struct nsm_msg_ipv6_mrt_stat_update *msg)
{
  u_char *sp = *pnt;
  int i = 0;

  /* Check size.  */
  if (*size < (NSM_MSG_IPV6_MRT_STAT_UPDATE_SIZE +
        (msg->num_blocks *  NSM_MSG_IPV6_MRT_STAT_BLOCK_SIZE)))
    return NSM_ERR_PKT_TOO_SMALL;

  /* Number of blocks.  */
  TLV_ENCODE_PUTW (msg->num_blocks);

  /* Encode olist */
  while (i < msg->num_blocks)
    {
      nsm_encode_ipv6_mrt_stat_block (pnt, size, &msg->blocks[i]);

      i++;
    }

  return *pnt - sp;
}

static int
nsm_decode_ipv6_mrt_stat_block (u_char **pnt, u_int16_t *size,
    struct nsm_msg_ipv6_mrt_stat_block *blk)
{
  u_char *sp = *pnt;

  /* Source Address */
  TLV_DECODE_GET_IN6_ADDR (&blk->src);

  /* Group Address */
  TLV_DECODE_GET_IN6_ADDR (&blk->group);

  /* Packets Forwarde */
  TLV_DECODE_GETL (blk->pkts_fwd);

  /* Bytes Forwarded */
  TLV_DECODE_GETL (blk->bytes_fwd);

  /* Flags */
  TLV_DECODE_GETW (blk->flags);

  /* Reserved */
  TLV_DECODE_GETW (blk->resv);

  return *pnt - sp;
}

/* Note that this function allocates memory to hold decoded stat blocks.
 * This memory is expected to be freed by the caller of this function.
 */
s_int32_t
nsm_decode_ipv6_mrt_stat_update (u_char **pnt, u_int16_t *size,
    struct nsm_msg_ipv6_mrt_stat_update *msg)
{
  u_char *sp = *pnt;
  int i = 0;

  /* Check minimum required size.  */
  if (*size < NSM_MSG_IPV6_MRT_STAT_UPDATE_SIZE)
    return NSM_ERR_PKT_TOO_SMALL;

  /* Number of blocks.  */
  TLV_DECODE_GETW (msg->num_blocks);

  /* Check minimum required size.  */
  if (*size < (msg->num_blocks * NSM_MSG_IPV6_MRT_STAT_BLOCK_SIZE))
    return NSM_ERR_PKT_TOO_SMALL;

  /* Decode olist */
  msg->blocks = XMALLOC (MTYPE_TMP, (msg->num_blocks * NSM_MSG_IPV6_MRT_STAT_BLOCK_SIZE));
  if (msg->blocks == NULL)
    return -1;

  while (i < msg->num_blocks)
    {
      nsm_decode_ipv6_mrt_stat_block (pnt, size, &msg->blocks[i]);
      i++;
    }

  return *pnt - sp;
}

s_int32_t
nsm_parse_ipv6_mrt_stat_update (u_char **pnt, u_int16_t *size,
    struct nsm_msg_header *header, void *arg, NSM_CALLBACK callback)
{
  int ret = -1;
  struct nsm_msg_ipv6_mrt_stat_update msg;

  pal_mem_set (&msg, 0, sizeof (struct nsm_msg_ipv6_mrt_stat_update));

  ret = nsm_decode_ipv6_mrt_stat_update (pnt, size, &msg);
  if (ret < 0)
    return ret;

  if (callback)
    {
      ret = (*callback) (header, arg, &msg);
    }

  /* Free the allocated olist */
  if (msg.blocks != NULL)
    XFREE (MTYPE_TMP, msg.blocks);

  return ret;
}

void
nsm_ipv6_mrt_stat_update_dump (struct lib_globals *zg,
    struct nsm_msg_ipv6_mrt_stat_update *msg)
{
  int i = 0;

  zlog_info (zg, "IPV6 MRT Stat update:");
  zlog_info (zg, " Number blocks: %d", msg->num_blocks);
  while (i < msg->num_blocks)
    {
      zlog_info (zg, " Block %d:", i + 1);
      zlog_info (zg, "  Source Address: %R", &msg->blocks[i].src);
      zlog_info (zg, "  Group Address: %R", &msg->blocks[i].group);
      zlog_info (zg, "  Pkts Forwarded: %u", msg->blocks[i].pkts_fwd);
      zlog_info (zg, "  Bytes Forwarded: %u", msg->blocks[i].bytes_fwd);
      zlog_info (zg, "  Flags: 0x%04x %s %s", msg->blocks[i].flags,
          (CHECK_FLAG (msg->blocks[i].flags, NSM_MSG_IPV6_MRT_STAT_IMMEDIATE) ? "Stat Imm" : ""),
          (CHECK_FLAG (msg->blocks[i].flags, NSM_MSG_IPV6_MRT_STAT_TIMED) ? "Stat Timed" : ""));
      zlog_info (zg, "  Reserved: %u", msg->blocks[i].resv);

      i++;
    }
  return;
}

/* IPV6 MRIB notification */
static int
nsm_encode_ipv6_mrib_mif_notification (u_char **pnt, u_int16_t *size,
    struct nsm_msg_ipv6_mrib_notification_key_mif *key)
{
  u_char *sp = *pnt;

  if (*size < NSM_IPV6_MRIB_NOTIF_KEY_MIF_SIZE)
    return NSM_ERR_PKT_TOO_SMALL;

  /* Ifindex.  */
  TLV_ENCODE_PUTL (key->ifindex);

  /* mif_index.  */
  TLV_ENCODE_PUTW (key->mif_index);

  /* Flags.  */
  TLV_ENCODE_PUTW (key->flags);

  return *pnt - sp;
}

static int
nsm_encode_ipv6_mrib_mrt_notification (u_char **pnt, u_int16_t *size,
    struct nsm_msg_ipv6_mrib_notification_key_mrt *key)
{
  u_char *sp = *pnt;

  if (*size < NSM_IPV6_MRIB_NOTIF_KEY_MRT_SIZE)
    return NSM_ERR_PKT_TOO_SMALL;

  /* Source Address.  */
  TLV_ENCODE_PUT_IN6_ADDR (&key->src);

  /* Group Address.  */
  TLV_ENCODE_PUT_IN6_ADDR (&key->group);

  /* Flags.  */
  TLV_ENCODE_PUTW (key->flags);

  return *pnt - sp;
}

s_int32_t
nsm_encode_ipv6_mrib_notification (u_char **pnt, u_int16_t *size,
    struct nsm_msg_ipv6_mrib_notification *msg)
{
  u_char *sp = *pnt;
  int ret;

  /* Check size for atleast type and status.  */
  if (*size < 4)
    return NSM_ERR_PKT_TOO_SMALL;

  /* Type.  */
  TLV_ENCODE_PUTW (msg->type);

  /* Status.  */
  TLV_ENCODE_PUTW (msg->status);

  switch (msg->type)
    {
    case NSM_MSG_IPV6_MRIB_NOTIFICATION_MIF_ADD:
    case NSM_MSG_IPV6_MRIB_NOTIFICATION_MIF_DEL:
      ret = nsm_encode_ipv6_mrib_mif_notification (pnt, size, &msg->key.mif_key);
      if (ret < 0)
        return ret;
      break;

    case NSM_MSG_IPV6_MRIB_NOTIFICATION_MRT_ADD:
    case NSM_MSG_IPV6_MRIB_NOTIFICATION_MRT_DEL:
    case NSM_MSG_IPV6_MRIB_NOTIFICATION_CLR_MRT:
      ret = nsm_encode_ipv6_mrib_mrt_notification (pnt, size, &msg->key.mrt_key);
      if (ret < 0)
        return ret;
      break;

    case NSM_MSG_IPV6_MRIB_NOTIFICATION_MCAST:
    case NSM_MSG_IPV6_MRIB_NOTIFICATION_NO_MCAST:
      break;
    }

  return *pnt - sp;
}

static int
nsm_decode_ipv6_mrib_mif_notification (u_char **pnt, u_int16_t *size,
    struct nsm_msg_ipv6_mrib_notification_key_mif *key)
{
  u_char *sp = *pnt;

  if (*size < NSM_IPV6_MRIB_NOTIF_KEY_MIF_SIZE)
    return NSM_ERR_PKT_TOO_SMALL;

  /* Ifindex.  */
  TLV_DECODE_GETL (key->ifindex);

  /* mif_index.  */
  TLV_DECODE_GETW (key->mif_index);

  /* Flags.  */
  TLV_DECODE_GETW (key->flags);

  return *pnt - sp;
}

static int
nsm_decode_ipv6_mrib_mrt_notification (u_char **pnt, u_int16_t *size,
    struct nsm_msg_ipv6_mrib_notification_key_mrt *key)
{
  u_char *sp = *pnt;

  if (*size < NSM_IPV6_MRIB_NOTIF_KEY_MRT_SIZE)
    return NSM_ERR_PKT_TOO_SMALL;

  /* Source Address.  */
  TLV_DECODE_GET_IN6_ADDR (&key->src);

  /* Group Address.  */
  TLV_DECODE_GET_IN6_ADDR (&key->group);

  /* Flags.  */
  TLV_DECODE_GETW (key->flags);

  return *pnt - sp;
}

s_int32_t
nsm_decode_ipv6_mrib_notification (u_char **pnt, u_int16_t *size,
    struct nsm_msg_ipv6_mrib_notification *msg)
{
  u_char *sp = *pnt;
  int ret;

  /* Check size for atleast type and status.  */
  if (*size < 4)
    return NSM_ERR_PKT_TOO_SMALL;

  /* Type.  */
  TLV_DECODE_GETW (msg->type);

  /* Status.  */
  TLV_DECODE_GETW (msg->status);

  switch (msg->type)
    {
    case NSM_MSG_IPV6_MRIB_NOTIFICATION_MIF_ADD:
    case NSM_MSG_IPV6_MRIB_NOTIFICATION_MIF_DEL:
      ret = nsm_decode_ipv6_mrib_mif_notification (pnt, size, &msg->key.mif_key);
      if (ret < 0)
        return ret;
      break;

    case NSM_MSG_IPV6_MRIB_NOTIFICATION_MRT_ADD:
    case NSM_MSG_IPV6_MRIB_NOTIFICATION_MRT_DEL:
    case NSM_MSG_IPV6_MRIB_NOTIFICATION_CLR_MRT:
      ret = nsm_decode_ipv6_mrib_mrt_notification (pnt, size, &msg->key.mrt_key);
      if (ret < 0)
        return ret;
      break;

    case NSM_MSG_IPV6_MRIB_NOTIFICATION_MCAST:
    case NSM_MSG_IPV6_MRIB_NOTIFICATION_NO_MCAST:
      break;
    }

  return *pnt - sp;
}

s_int32_t
nsm_parse_ipv6_mrib_notification (u_char **pnt, u_int16_t *size,
    struct nsm_msg_header *header, void *arg, NSM_CALLBACK callback)
{
  int ret = -1;
  struct nsm_msg_ipv6_mrib_notification msg;

  pal_mem_set (&msg, 0, sizeof (struct nsm_msg_ipv6_mrib_notification));

  ret = nsm_decode_ipv6_mrib_notification (pnt, size, &msg);
  if (ret < 0)
    return ret;

  if (callback)
    {
      ret = (*callback) (header, arg, &msg);
    }

  return ret;
}

void
nsm_ipv6_mrib_notification_dump (struct lib_globals *zg,
    struct nsm_msg_ipv6_mrib_notification *msg)
{
  zlog_info (zg, "IPV6 MRIB notification:");

  zlog_info (zg, " Type:%u (%s)", msg->type,
      nsm_ipv6_mrib_notif_type_str (msg->type));

  zlog_info (zg, " Status:%u (%s)", msg->status,
      nsm_ipv6_mrib_notif_status_str (msg->status));

  switch (msg->type)
    {
    case NSM_MSG_IPV6_MRIB_NOTIFICATION_MIF_ADD:
    case NSM_MSG_IPV6_MRIB_NOTIFICATION_MIF_DEL:
      zlog_info (zg, " Ifindex: %u", msg->key.mif_key.ifindex);
      zlog_info (zg, " Mif index: %u", msg->key.mif_key.mif_index);
      zlog_info (zg, " Flags: 0x%04x %s", msg->key.mif_key.flags,
          (CHECK_FLAG (msg->key.mif_key.flags, NSM_MSG_IPV6_MIF_FLAG_REGISTER) ? "Register Mif":""));
      break;

    case NSM_MSG_IPV6_MRIB_NOTIFICATION_MRT_ADD:
      zlog_info (zg, " MRT Source: %R", &msg->key.mrt_key.src);
      zlog_info (zg, " MRT Group: %R", &msg->key.mrt_key.group);
      zlog_info (zg, " Flags: 0x%04x %s %s", msg->key.mrt_key.flags,
          (CHECK_FLAG (msg->key.mrt_key.flags, NSM_MSG_IPV6_MRT_STAT_IMMEDIATE) ? "Stat Imm" : ""),
          (CHECK_FLAG (msg->key.mrt_key.flags, NSM_MSG_IPV6_MRT_STAT_TIMED) ? "Stat Timed" : ""));
      break;

    case NSM_MSG_IPV6_MRIB_NOTIFICATION_MRT_DEL:
    case NSM_MSG_IPV6_MRIB_NOTIFICATION_CLR_MRT:
      zlog_info (zg, " MRT Source: %R", &msg->key.mrt_key.src);
      zlog_info (zg, " MRT Group: %R", &msg->key.mrt_key.group);
      break;

    case NSM_MSG_IPV6_MRIB_NOTIFICATION_MCAST:
    case NSM_MSG_IPV6_MRIB_NOTIFICATION_NO_MCAST:
      break;
    }

  return;
}

s_int32_t
nsm_encode_mld (u_int8_t **pnt,
                u_int16_t *size,
                struct nsm_msg_mld *msg)
{
  u_int32_t idx;
  u_int8_t *sp;

  sp = *pnt;

  /* Check size */
  if (*size < (NSM_MSG_MLD_SIZE + sizeof (struct pal_in6_addr)
               * msg->num_srcs))
    return NSM_ERR_PKT_TOO_SMALL;

  /* Encode IFINDEX */
  TLV_ENCODE_PUTL (msg->ifindex);

  /* Encode Filter-Mode */
  TLV_ENCODE_PUTW (msg->filt_mode);

  /* Encode Num Sources */
  TLV_ENCODE_PUTW (msg->num_srcs);

  /* Encode Group Address */
  TLV_ENCODE_PUT_IN6_ADDR (&msg->grp_addr);

  /* Encode Sources List */
  if (msg->num_srcs)
    for (idx = 0; idx < msg->num_srcs; idx++)
      TLV_ENCODE_PUT_IN6_ADDR (& msg->src_addr_list [idx]);

  return *pnt - sp;
}

s_int32_t
nsm_decode_mld (u_int8_t **pnt,
                u_int16_t *size,
                struct nsm_msg_mld **msg)
{
  struct nsm_msg_mld msg_tmp;
  u_int32_t idx;
  u_int8_t *sp;

  sp = *pnt;

  /* Check size */
  if (*size < NSM_MSG_MLD_SIZE)
    return NSM_ERR_PKT_TOO_SMALL;

  /* Zero the temp mesg structure */
  pal_mem_set (&msg_tmp, 0, sizeof (struct nsm_msg_mld));

  /* Decode IFINDEX */
  TLV_DECODE_GETL (msg_tmp.ifindex);

  /* Decode Filter-Mode */
  TLV_DECODE_GETW (msg_tmp.filt_mode);

  /* Decode Num Sources */
  TLV_DECODE_GETW (msg_tmp.num_srcs);

  /* Decode Group Address */
  TLV_DECODE_GET_IN6_ADDR (&msg_tmp.grp_addr);

  /* Allocate Memory for the MLD Message (few extra bytes ok) */
  (*msg) = XCALLOC (MTYPE_TMP, sizeof (struct nsm_msg_mld)
                               + sizeof (struct pal_in6_addr)
                               * msg_tmp.num_srcs);

  if (! (*msg))
    return NSM_ERR_PKT_TOO_SMALL;

  /* Copy over bytes currently decoded */
  pal_mem_cpy ((*msg), &msg_tmp, sizeof (struct nsm_msg_mld));

  /* Decode Sources List */
  if ((*msg)->num_srcs)
    for (idx = 0; idx < (*msg)->num_srcs; idx++)
      TLV_DECODE_GET_IN6_ADDR (& (*msg)->src_addr_list [idx]);

  return *pnt - sp;
}

s_int32_t
nsm_parse_mld (u_char **pnt, u_int16_t *size,
               struct nsm_msg_header *header, void *arg,
               NSM_CALLBACK callback)
{
  struct nsm_msg_mld *msg;
  s_int32_t ret;

  msg = NULL;
  ret = 0;

  ret = nsm_decode_mld (pnt, size, &msg);
  if (ret < 0)
    goto EXIT;

  if (callback)
    ret = (*callback) (header, arg, msg);

EXIT:

  if (msg)
    XFREE (MTYPE_TMP, msg);

  return ret;
}

s_int32_t
nsm_mld_dump (struct lib_globals *zg,
              struct nsm_msg_mld *msg)
{
  u_int32_t idx;

  zlog_info (zg, "MLD Message");

  zlog_info (zg, " Ifindex: %d", msg->ifindex);
  zlog_info (zg, " Filter Mode: %s", msg->filt_mode ? "EXCL" : "INCL");
  zlog_info (zg, " Num Source Addresses: %d", msg->num_srcs);
  zlog_info (zg, " Group Address: %R", &msg->grp_addr);

  if (msg->num_srcs)
    {
      zlog_info (zg, " Sources Address List:");
      for (idx = 0; idx < msg->num_srcs; idx++)
        zlog_info (zg, "   %R", &msg->src_addr_list [idx]);
    }

  return 0;
}
#endif /* HAVE_MCAST_IPV6 */

#ifdef HAVE_L2
s_int32_t
nsm_encode_stp_msg (u_char **pnt,
                    u_int16_t *size,
                    struct nsm_msg_stp *msg)
{
  u_char *sp = *pnt;

  /* Check size.  */
  if (*size < NSM_MSG_STP_SIZE)
    return NSM_ERR_PKT_TOO_SMALL;

  /* Put flags. */
  TLV_ENCODE_PUTW (msg->flags);

  /* Put ifindex */
  TLV_ENCODE_PUTL (msg->ifindex);

  return *pnt - sp;
}

s_int32_t
nsm_decode_stp_msg (u_char **pnt,
                    u_int16_t *size,
                    struct nsm_msg_stp *msg)
{
  u_char *sp = *pnt;

  /* Check size.  */
  if (*size < NSM_MSG_STP_SIZE)
    return NSM_ERR_PKT_TOO_SMALL;

  /* Get the flags */
  TLV_DECODE_GETW (msg->flags);

  /* Get the ifindex */
  TLV_DECODE_GETL (msg->ifindex);

  return *pnt - sp;

}

/* parse the message received from stp protocol */
s_int32_t
nsm_parse_stp_msg (u_char **pnt, u_int16_t *size,
                  struct nsm_msg_header *header, void *arg,
                  NSM_CALLBACK callback)
{
  struct nsm_msg_stp msg;
  int ret;

  ret = nsm_decode_stp_msg (pnt, size, &msg);
  if (ret < 0)
    return ret;

  if (callback)
    {
      ret = (*callback) (header, arg, &msg);
      if (ret < 0)
        return ret;
    }

  return 0;
}

/* STP interface message dump */
void
nsm_stp_interface_dump (struct lib_globals *zg,
                        struct nsm_msg_stp *msg)
{
  zlog_info (zg, " Ifindex: %d", msg->ifindex);
  zlog_info (zg, "  Flags %s ",
             (msg->flags & NSM_MSG_INTERFACE_DOWN) ?  "Interface down" :
                                                      "Interface Up");
  return;
}

#endif /* HAVE_L2 */

#ifdef HAVE_MPLS_FRR
/* Encode Control Packet message */
s_int32_t
nsm_encode_rsvp_control_packet (u_char **pnt,
                                u_int16_t *size,
                                struct nsm_msg_rsvp_ctrl_pkt *msg)
{
  u_char *sp = *pnt;

  /* FTN Index  */
  TLV_ENCODE_PUTL (msg->ftn_index);

  /* Control packet id */
  TLV_ENCODE_PUTL (msg->cpkt_id);

  /* Control packet flags */
  TLV_ENCODE_PUTL (msg->flags);

  /* Control packet total_len */
  TLV_ENCODE_PUTL (msg->total_len);

  /* Control packet seq_num */
  TLV_ENCODE_PUTL (msg->seq_num);

  /* Control Packet Size */
  TLV_ENCODE_PUTL (msg->pkt_size);

  /* Control Packet */
  TLV_ENCODE_PUT (msg->pkt, msg->pkt_size);

  return *pnt - sp;
}

/* Decode Control Packet message.  */
s_int32_t
nsm_decode_rsvp_control_packet (u_char **pnt,
                                u_int16_t *size,
                                struct nsm_msg_rsvp_ctrl_pkt *ctrl_pkt)
{
  u_char *sp = *pnt;

  /* FTN Index */
  TLV_DECODE_GETL (ctrl_pkt->ftn_index);

  /* Control packet id */
  TLV_DECODE_GETL (ctrl_pkt->cpkt_id);

  /* Control packet flags */
  TLV_DECODE_GETL (ctrl_pkt->flags);

  /* Control packet total_len */
  TLV_DECODE_GETL (ctrl_pkt->total_len);

  /* Control packet seq_num */
  TLV_DECODE_GETL (ctrl_pkt->seq_num);

  /* Control Packet Size */
  TLV_DECODE_GETL (ctrl_pkt->pkt_size);

  /* Control Packet  */
  ctrl_pkt->pkt = XCALLOC (MTYPE_TMP, ctrl_pkt->pkt_size);
  if (! ctrl_pkt->pkt)
    return NSM_ERR_SYSTEM_FAILURE;

  TLV_DECODE_GET (ctrl_pkt->pkt, ctrl_pkt->pkt_size);

  return *pnt - sp;
}

s_int32_t
nsm_parse_rsvp_control_packet (u_char **pnt, u_int16_t *size,
                               struct nsm_msg_header *header, void *arg,
                               NSM_CALLBACK callback)
{
  int ret;
  struct nsm_msg_rsvp_ctrl_pkt ctrl_pkt;

  pal_mem_set (&ctrl_pkt, 0, sizeof (struct nsm_msg_rsvp_ctrl_pkt));

  ret = nsm_decode_rsvp_control_packet (pnt, size, &ctrl_pkt);
  if (ret < 0)
    return ret;

  if (callback)
    {
      ret = (*callback) (header, arg, &ctrl_pkt);
    }


  if (ctrl_pkt.pkt)
    {
      XFREE (MTYPE_TMP, ctrl_pkt.pkt);
      ctrl_pkt.pkt = NULL;
    }

  return ret;
}

#endif /* HAVE_MPLS_FRR */

#ifdef HAVE_NSM_MPLS_OAM
s_int32_t
nsm_parse_mpls_oam_request (u_char **pnt, u_int16_t *size,
                          struct nsm_msg_header *header, void *arg,
                          NSM_CALLBACK callback)
{
  int ret;
  struct nsm_msg_mpls_ping_req msg;

  pal_mem_set (&msg, 0, sizeof (struct nsm_msg_mpls_ping_req));

  ret = nsm_decode_mpls_onm_req (pnt, size, &msg);
  if (ret < 0)
    return ret;

  if (callback)
    {
      ret = (*callback) (header, arg, &msg);
      if (ret < 0)
        return ret;
    }
  return 0;
}

s_int32_t
nsm_encode_mpls_onm_req (u_char **pnt, u_int16_t *size,
                         struct nsm_msg_mpls_ping_req *msg)
{
  u_char *sp = *pnt;
  int i;

  /* Check size.  */
  if (*size < NSM_MSG_MPLS_ONM_SIZE)
    return NSM_ERR_PKT_TOO_SMALL;

  /* MPLS ONM TLV parse. */
  for (i = 0; i < NSM_CINDEX_SIZE; i++)
    {
      if (NSM_CHECK_CTYPE (msg->cindex, i))
        {
          if (*size < NSM_TLV_HEADER_SIZE)
            return NSM_ERR_PKT_TOO_SMALL;

          switch (i)
            {
              case NSM_CTYPE_MSG_MPLSONM_OPTION_ONM_TYPE :
                nsm_encode_tlv (pnt, size, i, 1);
                TLV_ENCODE_PUTC (msg->req_type);
              break;
              case NSM_CTYPE_MSG_MPLSONM_OPTION_LSP_TYPE:
                nsm_encode_tlv (pnt, size, i, 1);
                TLV_ENCODE_PUTC (msg->type);
                break;
              case NSM_CTYPE_MSG_MPLSONM_OPTION_LSP_ID:
                if (msg->type == MPLSONM_OPTION_L2CIRCUIT ||
                    msg->type == MPLSONM_OPTION_VPLS)
                  {
                    nsm_encode_tlv (pnt, size, i, 4);
                    TLV_ENCODE_PUTL (msg->u.l2_data.vc_id);
                  }
                else if (msg->type == MPLSONM_OPTION_L3VPN)
                  {
                    nsm_encode_tlv (pnt, size, i, msg->u.fec.vrf_name_len+2);
                    TLV_ENCODE_PUTW (msg->u.fec.vrf_name_len);
                   TLV_ENCODE_PUT (msg->u.fec.vrf_name,msg->u.fec.vrf_name_len);
                  }
                break;
              case NSM_CTYPE_MSG_MPLSONM_OPTION_DESTINATION:
                nsm_encode_tlv (pnt, size, i, 4);
                TLV_ENCODE_PUT_IN4_ADDR (&msg->dst);
                break;
              case NSM_CTYPE_MSG_MPLSONM_OPTION_SOURCE:
                nsm_encode_tlv (pnt, size, i, 4);
                TLV_ENCODE_PUT_IN4_ADDR (&msg->src);
              break;
              case NSM_CTYPE_MSG_MPLSONM_OPTION_TUNNELNAME:
                nsm_encode_tlv (pnt, size, i, msg->u.rsvp.tunnel_len+2);
                TLV_ENCODE_PUTW (msg->u.rsvp.tunnel_len);
                TLV_ENCODE_PUT (msg->u.rsvp.tunnel_name, msg->u.rsvp.tunnel_len);
                break;
              case NSM_CTYPE_MSG_MPLSONM_OPTION_EGRESS:
                nsm_encode_tlv (pnt, size, i, 4);
                TLV_ENCODE_PUT_IN4_ADDR (&msg->u.rsvp.addr);
                break;
              case NSM_CTYPE_MSG_MPLSONM_OPTION_PREFIX:
                nsm_encode_tlv (pnt, size, i, 8);
                TLV_ENCODE_PUT_IN4_ADDR (&msg->u.fec.vpn_prefix);
                TLV_ENCODE_PUTL (msg->u.fec.prefixlen);
                break;
              case NSM_CTYPE_MSG_MPLSONM_OPTION_FEC:
                nsm_encode_tlv (pnt, size, i, 8);
                TLV_ENCODE_PUT_IN4_ADDR (&msg->u.fec.ip_addr);
                TLV_ENCODE_PUTL (msg->u.fec.prefixlen);
                break;
              case NSM_CTYPE_MSG_MPLSONM_OPTION_PEER:
                nsm_encode_tlv (pnt, size, i, 4);
                TLV_ENCODE_PUT_IN4_ADDR (&msg->u.l2_data.vc_peer);
                break;
              case NSM_CTYPE_MSG_MPLSONM_OPTION_EXP:
                nsm_encode_tlv (pnt, size, i, 1);
                TLV_ENCODE_PUTC (msg->exp_bits);
                break;
              case NSM_CTYPE_MSG_MPLSONM_OPTION_TTL:
                nsm_encode_tlv (pnt, size, i, 1);
                TLV_ENCODE_PUTC (msg->ttl);
                break;
              case NSM_CTYPE_MSG_MPLSONM_OPTION_TIMEOUT:
                nsm_encode_tlv (pnt, size,i, 4);
                TLV_ENCODE_PUTL (msg->timeout);
                break;
              case NSM_CTYPE_MSG_MPLSONM_OPTION_REPEAT:
                nsm_encode_tlv (pnt, size,i, 4);
                TLV_ENCODE_PUTL (msg->repeat);
                break;
              case NSM_CTYPE_MSG_MPLSONM_OPTION_VERBOSE:
                nsm_encode_tlv (pnt, size,i, 1);
                TLV_ENCODE_PUTC (msg->timeout);
                break;
              case NSM_CTYPE_MSG_MPLSONM_OPTION_REPLYMODE:
                nsm_encode_tlv (pnt, size, i,1);
                TLV_ENCODE_PUTC (msg->reply_mode);
                break;
              case NSM_CTYPE_MSG_MPLSONM_OPTION_INTERVAL:
                nsm_encode_tlv (pnt, size, i,4);
                TLV_ENCODE_PUTL (msg->interval);
                break;
              case NSM_CTYPE_MSG_MPLSONM_OPTION_FLAGS:
                nsm_encode_tlv (pnt, size,i, 1);
                TLV_ENCODE_PUTC (msg->flags);
                break;
              case NSM_CTYPE_MSG_MPLSONM_OPTION_NOPHP:
                nsm_encode_tlv (pnt, size,i, 1);
                TLV_ENCODE_PUTC (msg->nophp);
                break;
            }
        }
    }

  return *pnt - sp;


}

s_int32_t
nsm_decode_mpls_onm_req (u_char **pnt, u_int16_t *size,
                         struct nsm_msg_mpls_ping_req *msg)
{
  u_char *sp = *pnt;
  struct nsm_tlv_header tlv;

  /* Check size.  */
  if (*size < NSM_MSG_MPLS_ONM_SIZE)
    return NSM_ERR_PKT_TOO_SMALL;

  /* TLV parse.  */
  while (*size)
    {
      if (*size < NSM_TLV_HEADER_SIZE)
        return NSM_ERR_PKT_TOO_SMALL;

      NSM_DECODE_TLV_HEADER (tlv);

      switch (tlv.type)
        {
           case NSM_CTYPE_MSG_MPLSONM_OPTION_ONM_TYPE :
             TLV_DECODE_GETC (msg->req_type);
             NSM_SET_CTYPE (msg->cindex, tlv.type);
             break;
            case NSM_CTYPE_MSG_MPLSONM_OPTION_LSP_TYPE:
              TLV_DECODE_GETC (msg->type);
             NSM_SET_CTYPE (msg->cindex, tlv.type);
              break;
            case NSM_CTYPE_MSG_MPLSONM_OPTION_LSP_ID:
              if (msg->type == MPLSONM_OPTION_L2CIRCUIT ||
                msg->type == MPLSONM_OPTION_VPLS)
                TLV_DECODE_GETL (msg->u.l2_data.vc_id);
              else if (msg->type == MPLSONM_OPTION_L3VPN)
                {
                  TLV_DECODE_GETW (msg->u.fec.vrf_name_len);
                  msg->u.fec.vrf_name = XCALLOC (MTYPE_VRF_NAME,
                                                 msg->u.fec.vrf_name_len+1);
                  TLV_DECODE_GET (msg->u.fec.vrf_name, msg->u.fec.vrf_name_len);
                }
             NSM_SET_CTYPE (msg->cindex, tlv.type);
              break;
           case NSM_CTYPE_MSG_MPLSONM_OPTION_DESTINATION:
             TLV_DECODE_GET_IN4_ADDR (&msg->dst);
             NSM_SET_CTYPE (msg->cindex, tlv.type);
             break;
           case NSM_CTYPE_MSG_MPLSONM_OPTION_SOURCE:
             TLV_DECODE_GET_IN4_ADDR (&msg->src);
             NSM_SET_CTYPE (msg->cindex, tlv.type);
             break;
           case NSM_CTYPE_MSG_MPLSONM_OPTION_TUNNELNAME:
             TLV_DECODE_GETW (msg->u.rsvp.tunnel_len);
             msg->u.rsvp.tunnel_name = XMALLOC (MTYPE_TMP,
                                                msg->u.rsvp.tunnel_len);
             TLV_DECODE_GET (msg->u.rsvp.tunnel_name, msg->u.rsvp.tunnel_len);
             NSM_SET_CTYPE (msg->cindex, tlv.type);
             break;
           case NSM_CTYPE_MSG_MPLSONM_OPTION_EGRESS:
             TLV_DECODE_GET_IN4_ADDR (&msg->u.rsvp.addr);
             NSM_SET_CTYPE (msg->cindex, tlv.type);
             break;
           case NSM_CTYPE_MSG_MPLSONM_OPTION_PREFIX:
             TLV_DECODE_GET_IN4_ADDR (&msg->u.fec.vpn_prefix);
             TLV_DECODE_GETL (msg->u.fec.prefixlen);
             NSM_SET_CTYPE (msg->cindex, tlv.type);
             break;
           case NSM_CTYPE_MSG_MPLSONM_OPTION_PEER:
             TLV_DECODE_GET_IN4_ADDR (&msg->u.l2_data.vc_peer);
             NSM_SET_CTYPE (msg->cindex, tlv.type);
             break;
           case NSM_CTYPE_MSG_MPLSONM_OPTION_FEC:
             TLV_DECODE_GET_IN4_ADDR (&msg->u.fec.ip_addr);
             TLV_DECODE_GETL (msg->u.fec.prefixlen);
             NSM_SET_CTYPE (msg->cindex, tlv.type);
                break;
           case NSM_CTYPE_MSG_MPLSONM_OPTION_EXP:
             TLV_DECODE_GETC (msg->exp_bits);
             NSM_SET_CTYPE (msg->cindex, tlv.type);
             break;
           case NSM_CTYPE_MSG_MPLSONM_OPTION_TTL:
             TLV_DECODE_GETC (msg->ttl);
             NSM_SET_CTYPE (msg->cindex, tlv.type);
             break;
           case NSM_CTYPE_MSG_MPLSONM_OPTION_TIMEOUT:
             TLV_DECODE_GETL (msg->timeout);
             NSM_SET_CTYPE (msg->cindex, tlv.type);
             break;
           case NSM_CTYPE_MSG_MPLSONM_OPTION_REPEAT:
             TLV_DECODE_GETL (msg->repeat);
             NSM_SET_CTYPE (msg->cindex, tlv.type);
             break;
           case NSM_CTYPE_MSG_MPLSONM_OPTION_VERBOSE:
             TLV_DECODE_GETC (msg->timeout);
             NSM_SET_CTYPE (msg->cindex, tlv.type);
             break;
           case NSM_CTYPE_MSG_MPLSONM_OPTION_REPLYMODE:
             TLV_DECODE_GETC (msg->reply_mode);
             NSM_SET_CTYPE (msg->cindex, tlv.type);
             break;
           case NSM_CTYPE_MSG_MPLSONM_OPTION_INTERVAL:
             TLV_DECODE_GETL (msg->interval);
             NSM_SET_CTYPE (msg->cindex, tlv.type);
             break;
           case NSM_CTYPE_MSG_MPLSONM_OPTION_FLAGS:
             TLV_DECODE_GETC (msg->flags);
             NSM_SET_CTYPE (msg->cindex, tlv.type);
             break;
           case NSM_CTYPE_MSG_MPLSONM_OPTION_NOPHP:
             TLV_DECODE_GETC (msg->nophp);
             NSM_SET_CTYPE (msg->cindex, tlv.type);
             break;
           default:
             TLV_DECODE_SKIP (tlv.length);
             break;
        }
    }
  return *pnt - sp;
}

s_int32_t
nsm_encode_mpls_onm_ping_reply (u_char **pnt, u_int16_t *size,
                                struct nsm_msg_mpls_ping_reply *msg)
{
  u_int32_t i;
  u_char *sp = *pnt;

  /* Check size.  */
  if (*size < NSM_MSG_MPLS_ONM_SIZE)
    return NSM_ERR_PKT_TOO_SMALL;

  /* MPLS ONM TLV parse. */
  for (i = 0; i < NSM_CINDEX_SIZE; i++)
    {
      if (NSM_CHECK_CTYPE (msg->cindex, i))
        {
          if (*size < NSM_TLV_HEADER_SIZE)
            return NSM_ERR_PKT_TOO_SMALL;

          switch (i)
            {
              case NSM_CTYPE_MSG_MPLSONM_PING_SENTTIME :
                nsm_encode_tlv (pnt, size, i,8);
                TLV_ENCODE_PUTL (msg->sent_time_sec);
                TLV_ENCODE_PUTL (msg->sent_time_usec);
                break;
              case NSM_CTYPE_MSG_MPLSONM_PING_RECVTIME :
                nsm_encode_tlv (pnt, size, i,8);
                TLV_ENCODE_PUTL (msg->recv_time_sec);
                TLV_ENCODE_PUTL (msg->recv_time_usec);
                break;
              case NSM_CTYPE_MSG_MPLSONM_PING_REPLYADDR :
                nsm_encode_tlv (pnt, size,i, 4);
                TLV_ENCODE_PUT_IN4_ADDR (&msg->reply);
                break;
              case NSM_CTYPE_MSG_MPLSONM_PING_VPNID:
                nsm_encode_tlv (pnt, size, i,8);
                TLV_ENCODE_PUTL (msg->recv_time_sec);
                TLV_ENCODE_PUTL (msg->recv_time_usec);
                break;
              case NSM_CTYPE_MSG_MPLSONM_PING_COUNT:
                nsm_encode_tlv (pnt, size, i,4);
                TLV_ENCODE_PUTL (msg->ping_count);
                break;
              case NSM_CTYPE_MSG_MPLSONM_PING_RECVCOUNT:
                nsm_encode_tlv (pnt, size,i, 4);
                TLV_ENCODE_PUTL (msg->recv_count);
                break;
              case NSM_CTYPE_MSG_MPLSONM_PING_RETCODE:
                nsm_encode_tlv (pnt, size, i,4);
                TLV_ENCODE_PUTL (msg->ret_code);
                break;

            }
        }
    }

  return *pnt - sp;

}

s_int32_t
nsm_decode_mpls_onm_ping_reply (u_char **pnt, u_int16_t *size,
                                struct nsm_msg_mpls_ping_reply *msg)
{
  u_char *sp = *pnt;
  struct nsm_tlv_header tlv;

  /* Check size.  */
  if (*size < NSM_MSG_MPLS_ONM_SIZE)
    return NSM_ERR_PKT_TOO_SMALL;

  /* TLV parse.  */
  while (*size)
    {
      if (*size < NSM_TLV_HEADER_SIZE)
        return NSM_ERR_PKT_TOO_SMALL;

      NSM_DECODE_TLV_HEADER (tlv);

      switch (tlv.type)
        {
          case NSM_CTYPE_MSG_MPLSONM_PING_SENTTIME :
            TLV_DECODE_GETL (msg->sent_time_sec);
            TLV_DECODE_GETL (msg->sent_time_usec);
            NSM_SET_CTYPE (msg->cindex, tlv.type);
            break;
          case NSM_CTYPE_MSG_MPLSONM_PING_RECVTIME :
            TLV_DECODE_GETL (msg->recv_time_sec);
            TLV_DECODE_GETL (msg->recv_time_usec);
            NSM_SET_CTYPE (msg->cindex, tlv.type);
            break;
          case NSM_CTYPE_MSG_MPLSONM_PING_REPLYADDR :
            TLV_DECODE_GET_IN4_ADDR (&msg->reply);
            NSM_SET_CTYPE (msg->cindex, tlv.type);
            break;
          case NSM_CTYPE_MSG_MPLSONM_PING_COUNT:
            TLV_DECODE_GETL (msg->ping_count);
            NSM_SET_CTYPE (msg->cindex, tlv.type);
            break;
          case NSM_CTYPE_MSG_MPLSONM_PING_RECVCOUNT:
            TLV_DECODE_GETL (msg->recv_count);
            NSM_SET_CTYPE (msg->cindex, tlv.type);
            break;
          case NSM_CTYPE_MSG_MPLSONM_PING_RETCODE:
            TLV_DECODE_GETL (msg->ret_code);
            NSM_SET_CTYPE (msg->cindex, tlv.type);
            break;
          default:
            TLV_DECODE_SKIP (tlv.length);
            break;
        }
    }

  return *pnt - sp;
}

s_int32_t
nsm_encode_mpls_onm_trace_reply (u_char **pnt, u_int16_t *size,
                                 struct nsm_msg_mpls_tracert_reply *msg)
{
  u_char *sp = *pnt;
  int i;
  struct shimhdr *shim;
  struct listnode *node;

  /* Check size.  */
  if (*size < NSM_MSG_MPLS_ONM_SIZE)
    return NSM_ERR_PKT_TOO_SMALL;

  /* MPLS ONM TLV parse. */
  for (i = 0; i < NSM_CINDEX_SIZE; i++)
    {
      if (NSM_CHECK_CTYPE (msg->cindex, i))
        {
          if (*size < NSM_TLV_HEADER_SIZE)
            return NSM_ERR_PKT_TOO_SMALL;

          switch (i)
            {
              case NSM_CTYPE_MSG_MPLSONM_TRACE_REPLYADDR :
                nsm_encode_tlv (pnt, size,i, 4);
                TLV_ENCODE_PUT_IN4_ADDR (&msg->reply);
                break;
              case NSM_CTYPE_MSG_MPLSONM_TRACE_SENTTIME :
                nsm_encode_tlv (pnt, size, i,8);
                TLV_ENCODE_PUTL (msg->sent_time_sec);
                TLV_ENCODE_PUTL (msg->sent_time_usec);
                break;
              case NSM_CTYPE_MSG_MPLSONM_TRACE_RECVTIME :
                nsm_encode_tlv (pnt, size, i,8);
                TLV_ENCODE_PUTL (msg->recv_time_sec);
                TLV_ENCODE_PUTL (msg->recv_time_usec);
                break;
              case NSM_CTYPE_MSG_MPLSONM_TRACE_TTL:
                nsm_encode_tlv (pnt, size, i,1);
                TLV_ENCODE_PUTC (msg->ttl);
                break;
              case NSM_CTYPE_MSG_MPLSONM_TRACE_RETCODE:
                nsm_encode_tlv (pnt, size, i,1);
                TLV_ENCODE_PUTC (msg->ret_code);
                break;
              case NSM_CTYPE_MSG_MPLSONM_TRACE_DSMAP:
                nsm_encode_tlv (pnt, size, i,msg->ds_len*4);
                LIST_LOOP (msg->ds_label, shim, node)
                  TLV_ENCODE_PUT (shim, sizeof (struct shimhdr));
                break;
              case NSM_CTYPE_MSG_MPLSONM_TRACE_LAST_MSG:
                nsm_encode_tlv (pnt, size, i , 1);
                TLV_ENCODE_PUTC (msg->last);
                break;
              case NSM_CTYPE_MSG_MPLSONM_TRACE_RECVCOUNT :
                nsm_encode_tlv (pnt, size, i,4);
                TLV_ENCODE_PUTL (msg->recv_count);
                break;
              case NSM_CTYPE_MSG_MPLSONM_TRACE_OUTLABEL:
                nsm_encode_tlv (pnt, size, i,4);
                TLV_ENCODE_PUTL (msg->out_label);
                break;


            }
        }
    }

  return *pnt - sp;

}

s_int32_t
nsm_decode_mpls_onm_trace_reply (u_char **pnt, u_int16_t *size,
                                struct nsm_msg_mpls_tracert_reply *msg)
{
  u_char *sp = *pnt;
  struct nsm_tlv_header tlv;
  struct shimhdr *shim;
  int cnt;

  /* Check size.  */
  if (*size < NSM_MSG_MPLS_ONM_SIZE)
    return NSM_ERR_PKT_TOO_SMALL;

  /* TLV parse.  */
  while (*size)
    {
      if (*size < NSM_TLV_HEADER_SIZE)
        return NSM_ERR_PKT_TOO_SMALL;

      NSM_DECODE_TLV_HEADER (tlv);

      switch (tlv.type)
        {
           case NSM_CTYPE_MSG_MPLSONM_TRACE_REPLYADDR :
              TLV_DECODE_GET_IN4_ADDR (&msg->reply);
              NSM_SET_CTYPE (msg->cindex, tlv.type);
              break;
           case NSM_CTYPE_MSG_MPLSONM_TRACE_SENTTIME:
              TLV_DECODE_GETL (msg->sent_time_sec);
              TLV_DECODE_GETL (msg->sent_time_usec);
              NSM_SET_CTYPE (msg->cindex, tlv.type);
             break;
           case NSM_CTYPE_MSG_MPLSONM_TRACE_RECVTIME:
              TLV_DECODE_GETL (msg->recv_time_sec);
              TLV_DECODE_GETL (msg->recv_time_usec);
              NSM_SET_CTYPE (msg->cindex, tlv.type);
             break;
           case NSM_CTYPE_MSG_MPLSONM_TRACE_TTL:
             TLV_DECODE_GETC (msg->ttl);
             NSM_SET_CTYPE (msg->cindex, tlv.type);
             break;
           case NSM_CTYPE_MSG_MPLSONM_TRACE_RETCODE:
             TLV_DECODE_GETC (msg->ret_code);
             NSM_SET_CTYPE (msg->cindex, tlv.type);
             break;
           case NSM_CTYPE_MSG_MPLSONM_TRACE_DSMAP :
             msg->ds_label = list_new();
             for (cnt = 0; cnt < tlv.length/4; cnt++)
               {
                 shim = XMALLOC (MTYPE_TMP, sizeof (struct shimhdr));
                 DECODE_GET (shim, sizeof (struct shimhdr));
                 listnode_add (msg->ds_label, shim);
               }
             NSM_SET_CTYPE (msg->cindex, tlv.type);
             msg->ds_len = tlv.length/4;
             break;
           case NSM_CTYPE_MSG_MPLSONM_TRACE_LAST_MSG:
             TLV_DECODE_GETC (msg->last);
             NSM_SET_CTYPE (msg->cindex, tlv.type);
             break;
           case NSM_CTYPE_MSG_MPLSONM_TRACE_RECVCOUNT:
              TLV_DECODE_GETL (msg->recv_count);
              NSM_SET_CTYPE (msg->cindex, tlv.type);
             break;
           case NSM_CTYPE_MSG_MPLSONM_TRACE_OUTLABEL:
              TLV_DECODE_GETL (msg->out_label);
              NSM_SET_CTYPE (msg->cindex, tlv.type);
             break;
           default:
             TLV_DECODE_SKIP (tlv.length);
             break;
        }
    }

  return *pnt - sp;
}

s_int32_t
nsm_encode_mpls_onm_error (u_char **pnt, u_int16_t *size,
                           struct nsm_msg_mpls_ping_error *msg)
{
  u_char *sp = *pnt;
  int i;
  /* Check size.  */
  if (*size < NSM_MSG_MPLS_ONM_ERR_SIZE)
    return NSM_ERR_PKT_TOO_SMALL;

  /* MPLS ONM TLV parse. */
  for (i = 0; i < NSM_CINDEX_SIZE; i++)
    {
      if (NSM_CHECK_CTYPE (msg->cindex, i))
        {
          if (*size < NSM_TLV_HEADER_SIZE)
            return NSM_ERR_PKT_TOO_SMALL;

          switch (i)
            {
              case NSM_CTYPE_MSG_MPLSONM_ERROR_MSGTYPE :
                nsm_encode_tlv (pnt, size, i,1);
                TLV_ENCODE_PUTC (msg->msg_type);
                break;
              case NSM_CTYPE_MSG_MPLSONM_ERROR_TYPE:
                nsm_encode_tlv (pnt, size, i,1);
                TLV_ENCODE_PUTC (msg->type);
                break;
              case NSM_CTYPE_MSG_MPLSONM_ERROR_VPNID:
                nsm_encode_tlv (pnt, size, i,4);
                TLV_ENCODE_PUTL (msg->vpn_id);
                break;
              case NSM_CTYPE_MSG_MPLSONM_ERROR_FEC:
                nsm_encode_tlv (pnt, size, i,4);
                TLV_ENCODE_PUT_IN4_ADDR (&msg->fec);
                break;
              case NSM_CTYPE_MSG_MPLSONM_ERROR_VPN_PEER:
                nsm_encode_tlv (pnt, size, i,4);
                TLV_ENCODE_PUT_IN4_ADDR (&msg->vpn_peer);
                break;
              case NSM_CTYPE_MSG_MPLSONM_ERROR_SENTTIME:
                nsm_encode_tlv (pnt, size, i,4);
                TLV_ENCODE_PUTL (msg->sent_time);
                break;
              case NSM_CTYPE_MSG_MPLSONM_ERROR_RETCODE:
                nsm_encode_tlv (pnt, size, i,4);
                TLV_ENCODE_PUTL (msg->ret_code);
               break;
              case NSM_CTYPE_MSG_MPLSONM_ERROR_RECVCOUNT:
                nsm_encode_tlv (pnt, size, i,4);
                TLV_ENCODE_PUTL (msg->recv_count);
               break;

            }
        }
    }

  return *pnt - sp;

}

s_int32_t
nsm_decode_mpls_onm_error (u_char **pnt, u_int16_t *size,
                           struct nsm_msg_mpls_ping_error *msg)
{
  u_char *sp = *pnt;
  struct nsm_tlv_header tlv;

  /* Check size.  */
  if (*size < NSM_MSG_MPLS_ONM_ERR_SIZE)
    return NSM_ERR_PKT_TOO_SMALL;

  /* TLV parse.  */
  while (*size)
    {
      if (*size < NSM_TLV_HEADER_SIZE)
        return NSM_ERR_PKT_TOO_SMALL;

      NSM_DECODE_TLV_HEADER (tlv);

      switch (tlv.type)
        {
           case NSM_CTYPE_MSG_MPLSONM_ERROR_MSGTYPE :
              TLV_DECODE_GETC (msg->msg_type);
              NSM_SET_CTYPE (msg->cindex, tlv.type);
              break;
           case NSM_CTYPE_MSG_MPLSONM_ERROR_TYPE:
              TLV_DECODE_GETC (msg->type);
              NSM_SET_CTYPE (msg->cindex, tlv.type);
              break;
           case NSM_CTYPE_MSG_MPLSONM_ERROR_VPNID:
              TLV_DECODE_GETC (msg->vpn_id);
              NSM_SET_CTYPE (msg->cindex, tlv.type);
              break;
           case NSM_CTYPE_MSG_MPLSONM_ERROR_FEC:
              TLV_DECODE_GET_IN4_ADDR (&msg->fec);
              NSM_SET_CTYPE (msg->cindex, tlv.type);
              break;
           case NSM_CTYPE_MSG_MPLSONM_ERROR_VPN_PEER:
              TLV_DECODE_GET_IN4_ADDR (&msg->vpn_peer);
              NSM_SET_CTYPE (msg->cindex, tlv.type);
              break;
           case NSM_CTYPE_MSG_MPLSONM_ERROR_SENTTIME:
              TLV_DECODE_GETL (msg->sent_time);
              NSM_SET_CTYPE (msg->cindex, tlv.type);
              break;
           case NSM_CTYPE_MSG_MPLSONM_ERROR_RETCODE:
              TLV_DECODE_GETL (msg->ret_code);
              NSM_SET_CTYPE (msg->cindex, tlv.type);
              break;
           case NSM_CTYPE_MSG_MPLSONM_ERROR_RECVCOUNT:
              TLV_DECODE_GETL (msg->recv_count);
              NSM_SET_CTYPE (msg->cindex, tlv.type);
              break;
           default:
             TLV_DECODE_SKIP (tlv.length);
             break;
        }
    }

  return *pnt - sp;
}

/* Parse MPLS OAM PING Reply. */
s_int32_t
nsm_parse_oam_ping_response (u_char **pnt, u_int16_t *size,
                             struct nsm_msg_header *header, void *arg,
                             NSM_CALLBACK callback)
{
  struct nsm_msg_mpls_ping_reply msg;
  int ret;

  pal_mem_set (&msg, 0, sizeof (struct nsm_msg_mpls_ping_reply));
  ret = nsm_decode_mpls_onm_ping_reply (pnt, size, &msg);
  if (ret < 0)
    return ret;

  if (callback)
    {
      ret = (*callback) (header, arg, &msg);
      if (ret < 0)
        return ret;
    }

  return 0;
}

/* Parse MPLS OAM Tracert Reply. */
s_int32_t
nsm_parse_oam_tracert_reply (u_char **pnt, u_int16_t *size,
                             struct nsm_msg_header *header, void *arg,
                             NSM_CALLBACK callback)
{
  struct nsm_msg_mpls_tracert_reply msg;
  int ret;

  pal_mem_set (&msg, 0, sizeof (struct nsm_msg_mpls_tracert_reply));
  ret = nsm_decode_mpls_onm_trace_reply (pnt, size, &msg);
  if (ret < 0)
    return ret;

  if (callback)
    {
      ret = (*callback) (header, arg, &msg);
      if (ret < 0)
        return ret;
    }

  return 0;
}
s_int32_t
nsm_parse_oam_error (u_char **pnt, u_int16_t *size,
                     struct nsm_msg_header *header, void *arg,
                     NSM_CALLBACK callback)
{
  struct nsm_msg_mpls_ping_error msg;
  int ret;

  pal_mem_set(&msg, 0, sizeof (struct nsm_msg_mpls_ping_error));
  ret = nsm_decode_mpls_onm_error (pnt, size, &msg);
  if (ret < 0)
    return ret;

  if (callback)
    {
      ret = (*callback) (header, arg, &msg);
      if (ret < 0)
        return ret;
    }

  return 0;
}
#endif /* HAVE_NSM_MPLS_OAM */

#ifdef HAVE_MPLS_OAM_ITUT
s_int32_t
nsm_parse_mpls_oam_process_request(u_char **pnt, u_int16_t *size,
		          struct nsm_msg_header *header, void *arg,
		          NSM_CALLBACK callback)
{
  int ret;
  struct nsm_msg_mpls_oam_itut_req msg;

  pal_mem_set (&msg, 0, sizeof (struct nsm_msg_mpls_oam_itut_req));

  ret = nsm_mpls_oam_decode_itut_req (pnt, size, &msg);
  if (ret < 0)
    return ret;

  if (callback)
    {
      ret = (*callback) (header, arg, &msg);
      if (ret < 0)
	return ret;
    }
  return 0;
}

s_int32_t
nsm_mpls_oam_decode_itut_req (u_char **pnt, u_int16_t *size,
                         struct nsm_msg_mpls_oam_itut_req *msg)
{
  u_char *sp = *pnt;
  u_int32_t oam_alert_label;
  u_int16_t defect_type;
  u_int16_t bip16;
  u_int32_t defect_location;
  u_int32_t lsrpad;
  u_int8_t  pkt_process_type;
  struct nsm_msg_mpls_oam_itut_req req;

  DECODE_GETL(&oam_alert_label);
  oam_alert_label = pal_ntoh32(oam_alert_label);

  if ( oam_alert_label != MPLS_OAM_ITUT_ALERT_LABEL )
       return -1;

   /* Decode the OAM payload */
   DECODE_GETC(&pkt_process_type);
   DECODE_GETC_EMPTY();

   switch (pkt_process_type)
   {
     case MPLS_OAM_CV_PROCESS_MESSAGE:
     case MPLS_OAM_FFD_PROCESS_MESSAGE:
    	    DECODE_GETW_EMPTY();
     	break;
     case MPLS_OAM_FDI_PROCESS_MESSAGE:
     case MPLS_OAM_BDI_PROCESS_MESSAGE:
    	    DECODE_GETW(&defect_type);
     	break;
     default:
     	 return -1;
   }

   DECODE_GETL_EMPTY();
   DECODE_GETL_EMPTY();
   DECODE_GETW_EMPTY();
   DECODE_GETW(&lsrpad);
   DECODE_GETL(&req.lsr_addr);
   DECODE_GETL(&req.lsp_id);

   switch (req.pkt_type)
   {
     case MPLS_OAM_CV_PROCESS_MESSAGE:
    	    DECODE_GETL_EMPTY();
         break;
     case MPLS_OAM_FFD_PROCESS_MESSAGE:
    	    DECODE_GETC(&req.frequency);
    	    DECODE_GETW_EMPTY();
     	break;
     case MPLS_OAM_FDI_PROCESS_MESSAGE:
     case MPLS_OAM_BDI_PROCESS_MESSAGE:
    	    DECODE_GETW(&defect_location);
     	break;
     default:
     	 return -1;
   }
  DECODE_GETL_EMPTY();
  DECODE_GETL_EMPTY();
  DECODE_GETL_EMPTY();
  DECODE_GETW_EMPTY();
  DECODE_GETW(&bip16);

  msg->pkt_type = req.pkt_type;
  msg->lsr_addr = req.lsr_addr;
  msg->lsp_id = pal_ntoh32(req.lsp_id);
  msg->frequency = req.frequency;
  return *pnt - sp;
}
#endif /* HAVE_MPLS_OAM_ITUT*/

void
nsm_nh_track_dump (struct lib_globals *zg,
                   struct nsm_msg_nexthop_tracking *msg)
{

  zlog_info (zg, "Received NHT message from client with flags %d",
              msg->flags);
}
s_int32_t
nsm_encode_nexthop_tracking (u_char **pnt, u_int16_t *size,
                             struct nsm_msg_nexthop_tracking *msg)
{
  u_char *sp = *pnt;

  TLV_ENCODE_PUTC (msg->flags);
  return *pnt - sp;
}

s_int32_t
nsm_decode_nexthop_tracking (u_char **pnt, u_int16_t *size,
                             struct nsm_msg_nexthop_tracking *msg)
{
  u_char *sp = *pnt;

  TLV_DECODE_GETC (msg->flags);

  return *pnt - sp;
}

int nsm_parse_nexthop_tracking (u_char **pnt,
                                u_int16_t *size,
                                struct nsm_msg_header *header,
                                void *arg,
                                NSM_CALLBACK callback)
{
  int ret;
  struct nsm_msg_nexthop_tracking msg;

  pal_mem_set (&msg, 0, sizeof (struct nsm_msg_nexthop_tracking));

  ret = nsm_decode_nexthop_tracking (pnt, size, &msg);
  if (ret < 0)
    return ret;

  if (callback)
    {
      ret = (*callback) (header, arg, &msg);
      if (ret < 0)
        return ret;
    }
  return 0;
}

#if defined HAVE_RESTART && defined HAVE_MPLS
s_int32_t
nsm_parse_gen_stale_bundle_msg (u_char **pnt, u_int16_t *size,
                                struct nsm_msg_header *header, void *arg,
                                NSM_CALLBACK callback)
{
  int ret = 0;
  int nbytes;
  u_int16_t length = *size;
  struct nsm_gen_msg_stale_info msg;

  pal_mem_set (&msg, 0, sizeof (struct nsm_gen_msg_stale_info));

  while (length)
    {
      pal_mem_set (&msg, 0, sizeof (struct nsm_gen_msg_stale_info));
      nbytes = nsm_decode_gen_stale_bundle_msg (pnt, size, &msg);
      if (nbytes < 0)
        return nbytes;

      if (callback)
        {
          ret = (*callback) (header, arg, &msg);
          if (ret < 0)
            return ret;
        }

      if (nbytes > length)
        break;

      length -= nbytes;
    }

  return 0;
}

s_int32_t
nsm_parse_stale_bundle_msg (u_char **pnt, u_int16_t *size,
                        struct nsm_msg_header *header, void *arg,
                        NSM_CALLBACK callback)
{
  int ret = 0;
  int nbytes;
  u_int16_t length = *size;
  struct nsm_msg_stale_info msg;

  pal_mem_set (&msg, 0, sizeof (struct nsm_msg_stale_info));

  while (length)
    {
      pal_mem_set (&msg, 0, sizeof (struct nsm_msg_stale_info));
      nbytes = nsm_decode_stale_bundle_msg (pnt, size, &msg);
      if (nbytes < 0)
        return nbytes;

      if (callback)
        {
          ret = (*callback) (header, arg, &msg);
          if (ret < 0)
            return ret;
        }

      if (nbytes > length)
        break;

      length -= nbytes;
    }

  return 0;
}

 /* Encode Stale bundle message. */
s_int32_t
nsm_encode_gen_stale_bundle_msg (u_char **pnt, u_int16_t *size,
                                 struct nsm_gen_msg_stale_info *msg)
{
  u_char *sp = *pnt;
  u_int16_t length;
  /* For remember length part.  */
  u_char *lp;

  /* Check size. */
  if (*size < NSM_MSG_STALE_INFO_SIZE)
    return NSM_ERR_PKT_TOO_SMALL;

  /* Total length of this message.  First we put place holder then
   * fill in later on.  */
  lp = *pnt;
  TLV_ENCODE_PUT_EMPTY (2);

  TLV_ENCODE_PUTC (msg->fec_prefix.family);
  TLV_ENCODE_PUTC (msg->fec_prefix.prefixlen);

#ifdef HAVE_IPV6
  if (msg->fec_prefix.family == AF_INET6)
    TLV_ENCODE_PUT_IN6_ADDR (&msg->fec_prefix.u.prefix6);
  else
#endif /* HAVE_IPV6 */
    TLV_ENCODE_PUT_IN4_ADDR (&msg->fec_prefix.u.prefix4);

  nsm_encode_gmpls_label (pnt, size, &msg->in_label);
  TLV_ENCODE_PUTL (msg->iif_ix);
  nsm_encode_gmpls_label (pnt, size, &msg->out_label);
  TLV_ENCODE_PUTL (msg->oif_ix);
  TLV_ENCODE_PUTL (msg->ilm_ix);

  /* Fill in real length.  */
  length = *pnt - sp;
  length = pal_hton16 (length);
  pal_mem_cpy (lp, &length, 2);

  return *pnt - sp;
}

/* Decode Stale bundle message. */
s_int32_t
nsm_decode_gen_stale_bundle_msg (u_char **pnt, u_int16_t *size,
                                 struct nsm_gen_msg_stale_info *msg)
{
  u_char *sp = *pnt;
  u_int16_t length;

  /* Check size. */
  if (*size < NSM_MSG_STALE_INFO_SIZE)
    return NSM_ERR_PKT_TOO_SMALL;

  /* Total length of this message.  */
  TLV_DECODE_GETW (length);

  TLV_DECODE_GETC (msg->fec_prefix.family);
  TLV_DECODE_GETC (msg->fec_prefix.prefixlen);

#ifdef HAVE_IPV6
  if (msg->fec_prefix.family == AF_INET6)
    TLV_DECODE_GET_IN6_ADDR (&msg->fec_prefix.u.prefix6);
  else
#endif /* HAVE_IPV6 */
    TLV_DECODE_GET_IN4_ADDR (&msg->fec_prefix.u.prefix4);

  nsm_decode_gmpls_label (pnt, size, &msg->in_label);
  TLV_DECODE_GETL (msg->iif_ix);

  nsm_decode_gmpls_label (pnt, size, &msg->out_label);
  TLV_DECODE_GETL (msg->oif_ix);
  TLV_DECODE_GETL (msg->ilm_ix);

  return *pnt - sp;
}
 /* Encode Stale bundle message. */
s_int32_t
nsm_encode_stale_bundle_msg (u_char **pnt, u_int16_t *size,
                         struct nsm_msg_stale_info *msg)
{
  u_char *sp = *pnt;
  u_int16_t length;
  /* For remember length part.  */
  u_char *lp;

  /* Check size. */
  if (*size < NSM_MSG_STALE_INFO_SIZE)
    return NSM_ERR_PKT_TOO_SMALL;

  /* Total length of this message.  First we put place holder then
   * fill in later on.  */
  lp = *pnt;
  TLV_ENCODE_PUT_EMPTY (2);

  TLV_ENCODE_PUTC (msg->fec_prefix.family);
  TLV_ENCODE_PUTC (msg->fec_prefix.prefixlen);

#ifdef HAVE_IPV6
  if (msg->fec_prefix.family == AF_INET6)
    TLV_ENCODE_PUT_IN6_ADDR (&msg->fec_prefix.u.prefix6);
  else
#endif /* HAVE_IPV6 */
    TLV_ENCODE_PUT_IN4_ADDR (&msg->fec_prefix.u.prefix4);

  TLV_ENCODE_PUTL (msg->in_label);
  TLV_ENCODE_PUTL (msg->iif_ix);
  TLV_ENCODE_PUTL (msg->out_label);
  TLV_ENCODE_PUTL (msg->oif_ix);
  TLV_ENCODE_PUTL (msg->ilm_ix);

  /* Fill in real length.  */
  length = *pnt - sp;
  length = pal_hton16 (length);
  pal_mem_cpy (lp, &length, 2);

  return *pnt - sp;
}

/* Decode Stale bundle message. */
s_int32_t
nsm_decode_stale_bundle_msg (u_char **pnt, u_int16_t *size,
                         struct nsm_msg_stale_info *msg)
{
  u_char *sp = *pnt;
  u_int16_t length;

  /* Check size. */
  if (*size < NSM_MSG_STALE_INFO_SIZE)
    return NSM_ERR_PKT_TOO_SMALL;

  /* Total length of this message.  */
  TLV_DECODE_GETW (length);

  TLV_DECODE_GETC (msg->fec_prefix.family);
  TLV_DECODE_GETC (msg->fec_prefix.prefixlen);

#ifdef HAVE_IPV6
  if (msg->fec_prefix.family == AF_INET6)
    TLV_DECODE_GET_IN6_ADDR (&msg->fec_prefix.u.prefix6);
  else
#endif /* HAVE_IPV6 */
    TLV_DECODE_GET_IN4_ADDR (&msg->fec_prefix.u.prefix4);

  TLV_DECODE_GETL (msg->in_label);
  TLV_DECODE_GETL (msg->iif_ix);
  TLV_DECODE_GETL (msg->out_label);
  TLV_DECODE_GETL (msg->oif_ix);
  TLV_DECODE_GETL (msg->ilm_ix);

  return *pnt - sp;
}
#endif /* HAVE_RESTART  && HAVE_MPLS*/


#ifdef HAVE_MPLS_OAM
/* Encode, Decode & Parse functions for echo request/reply process
 * messages from OAMD.
 */

/* MPLS OAM control data encode. */
int
nsm_encode_oam_ctrl_data (u_char **pnt, u_int16_t *size,
                          struct mpls_oam_ctrl_data *msg)
{
  u_char *sp = *pnt;
  u_char i;

  TLV_ENCODE_PUT_IN4_ADDR (&msg->src_addr);
  TLV_ENCODE_PUTL (msg->in_ifindex);
  TLV_ENCODE_PUTL (msg->label_stack_depth);

  for (i = 0; i < msg->label_stack_depth; i++)
    {
      TLV_ENCODE_PUTL (msg->label_stack [i]);
    }
  TLV_ENCODE_PUTL (msg->input1);
  TLV_ENCODE_PUTL (msg->input2);

  return *pnt - sp;
}

/* Encode OAM echo request process message. */
int
nsm_encode_oam_req_process (u_char **pnt, u_int16_t *size,
                            struct nsm_msg_oam_lsp_ping_req_process *msg)
{
  u_char *sp = *pnt;
  int i;

  for (i = 0; i < NSM_CINDEX_SIZE; i++)
    {
      if (NSM_CHECK_CTYPE (msg->cindex, i))
        {
          if (*size < NSM_TLV_HEADER_SIZE)
            return NSM_ERR_PKT_TOO_SMALL;

          switch (i)
           {
            case NSM_CTYPE_MSG_OAM_OPTION_LSP_TYPE:
              nsm_encode_tlv (pnt, size, i, 1);
              TLV_ENCODE_PUTC (msg->type);
              break;
            case NSM_CTYPE_MSG_OAM_OPTION_DSMAP:
              nsm_encode_tlv (pnt, size, i, 1);
              TLV_ENCODE_PUTC (msg->is_dsmap);
              break;
            case NSM_CTYPE_MSG_OAM_OPTION_LSP_ID:
              if (msg->type == MPLSONM_OPTION_L2CIRCUIT ||
                  msg->type == MPLSONM_OPTION_VPLS)
                {
                  nsm_encode_tlv (pnt, size, i, 4+1);
                  TLV_ENCODE_PUTL (msg->u.l2_data.vc_id);
#ifdef HAVE_VCCV
                  TLV_ENCODE_PUTC (msg->u.l2_data.is_vccv);
#endif /* HAVE_VCCV */
                }
              else if (msg->type == MPLSONM_OPTION_L3VPN)
                {
                  nsm_encode_tlv (pnt, size, i, msg->u.fec.vrf_name_len+2);
                  TLV_ENCODE_PUTW (msg->u.fec.vrf_name_len);
                  TLV_ENCODE_PUT (msg->u.fec.vrf_name, msg->u.fec.vrf_name_len);
                }
              break;
            case NSM_CTYPE_MSG_OAM_OPTION_TUNNELNAME:
              nsm_encode_tlv (pnt, size, i, msg->u.rsvp.tunnel_len+2);
              TLV_ENCODE_PUTW (msg->u.rsvp.tunnel_len);
              TLV_ENCODE_PUT (msg->u.rsvp.tunnel_name, msg->u.rsvp.tunnel_len);
              break;
            case NSM_CTYPE_MSG_OAM_OPTION_EGRESS:
              nsm_encode_tlv (pnt, size, i, 4);
              TLV_ENCODE_PUT_IN4_ADDR (&msg->u.rsvp.addr);
              break;
            case NSM_CTYPE_MSG_OAM_OPTION_PREFIX:
              nsm_encode_tlv (pnt, size, i, 8);
              TLV_ENCODE_PUT_IN4_ADDR (&msg->u.fec.vpn_prefix);
              TLV_ENCODE_PUTL (msg->u.fec.prefixlen);
              break;
            case NSM_CTYPE_MSG_OAM_OPTION_PEER:
              nsm_encode_tlv (pnt, size, i, 4);
              TLV_ENCODE_PUT_IN4_ADDR (&msg->u.l2_data.vc_peer);
              break;
            case NSM_CTYPE_MSG_OAM_OPTION_FEC:
              nsm_encode_tlv (pnt, size, i, 8);
              TLV_ENCODE_PUT_IN4_ADDR (&msg->u.fec.ip_addr);
              TLV_ENCODE_PUTL (msg->u.fec.prefixlen);
              break;
            case NSM_CTYPE_MSG_OAM_OPTION_SEQ_NUM:
              nsm_encode_tlv (pnt, size, i, 4);
              TLV_ENCODE_PUTL (msg->seq_no);
              break;
         }
       }
    }

  return *pnt - sp;
}

/* Encode OAM reply process message for LDP type LSP. */
int
nsm_encode_oam_rep_process_ldp (u_char **pnt, u_int16_t *size,
                               struct nsm_msg_oam_lsp_ping_rep_process_ldp *msg)
{
  u_char *sp = *pnt;

  nsm_encode_oam_ldp_ipv4_data (pnt, size, &msg->data);

  TLV_ENCODE_PUTC (msg->is_dsmap);

  TLV_ENCODE_PUTL (msg->seq_no);

  nsm_encode_oam_ctrl_data (pnt, size, &msg->ctrl_data);

  return *pnt - sp;
}

/* Encode OAM reply process message for RSVP type LSP. */
int
nsm_encode_oam_rep_process_rsvp (u_char **pnt, u_int16_t *size,
                              struct nsm_msg_oam_lsp_ping_rep_process_rsvp *msg)
{
  u_char *sp = *pnt;

  nsm_encode_oam_rsvp_ipv4_data (pnt, size, &msg->data);

  TLV_ENCODE_PUTC (msg->is_dsmap);

  TLV_ENCODE_PUTL (msg->seq_no);

  nsm_encode_oam_ctrl_data (pnt, size, &msg->ctrl_data);

  return *pnt - sp;
}

/* Encode OAM reply process message for Static LSP. */
int
nsm_encode_oam_rep_process_gen (u_char **pnt, u_int16_t *size,
                               struct nsm_msg_oam_lsp_ping_rep_process_gen *msg)
{
  u_char *sp = *pnt;

  nsm_encode_oam_gen_ipv4_data (pnt, size, &msg->data);

  TLV_ENCODE_PUTC (msg->is_dsmap);

  TLV_ENCODE_PUTL (msg->seq_no);

  nsm_encode_oam_ctrl_data (pnt, size, &msg->ctrl_data);

  return *pnt - sp;
}

#ifdef HAVE_MPLS_VC
/* Encode OAM reply process message for L2VC. */
int
nsm_encode_oam_rep_process_l2vc (u_char **pnt, u_int16_t *size,
                              struct nsm_msg_oam_lsp_ping_rep_process_l2vc *msg)
{
  u_char *sp = *pnt;

  TLV_ENCODE_PUTW (msg->l2_data_type);
  TLV_ENCODE_PUTW (msg->tunnel_type);

  TLV_ENCODE_PUTC (msg->is_dsmap);

  TLV_ENCODE_PUTL (msg->seq_no);

  switch (msg->l2_data_type)
    {
      case NSM_OAM_L2_CIRCUIT_PREFIX_DEP:
        nsm_encode_oam_l2vc_prefix_dep (pnt, size, &msg->l2_data.pw_128_dep);
        break;
      case NSM_OAM_L2_CIRCUIT_PREFIX_CURR:
        nsm_encode_oam_l2vc_prefix_cur (pnt, size, &msg->l2_data.pw_128_cur);
        break;
      case NSM_OAM_GEN_PW_ID_PREFIX:
        nsm_encode_oam_gen_pw_id_prefix (pnt, size, &msg->l2_data.pw_129);
        break;
    }

  switch (msg->tunnel_type)
    {
      case NSM_OAM_LDP_IPv4_PREFIX:
        nsm_encode_oam_ldp_ipv4_data (pnt, size, &msg->tunnel_data.ldp);
        break;
      case NSM_OAM_RSVP_IPv4_PREFIX:
        nsm_encode_oam_rsvp_ipv4_data (pnt, size, &msg->tunnel_data.rsvp);
        break;
      case NSM_OAM_GEN_IPv4_PREFIX:
        nsm_encode_oam_gen_ipv4_data (pnt, size, &msg->tunnel_data.gen);
        break;
    }

  nsm_encode_oam_ctrl_data (pnt, size, &msg->ctrl_data);

  return *pnt - sp;
}
#endif /* HAVE_MPLS_VC */

/* Encode OAM reply process message for L3VPN. */
int
nsm_encode_oam_rep_process_l3vpn (u_char **pnt, u_int16_t *size,
                             struct nsm_msg_oam_lsp_ping_rep_process_l3vpn *msg)
{
  u_char *sp = *pnt;

  TLV_ENCODE_PUTW (msg->tunnel_type);

  TLV_ENCODE_PUTC (msg->is_dsmap);

  TLV_ENCODE_PUTL (msg->seq_no);

  nsm_encode_oam_l3vpn_data (pnt, size, &msg->data);

  switch (msg->tunnel_type)
    {
      case NSM_OAM_LDP_IPv4_PREFIX:
        nsm_encode_oam_ldp_ipv4_data (pnt, size, &msg->tunnel_data.ldp);
        break;
      case NSM_OAM_RSVP_IPv4_PREFIX:
        nsm_encode_oam_rsvp_ipv4_data (pnt, size, &msg->tunnel_data.rsvp);
        break;
      case NSM_OAM_GEN_IPv4_PREFIX:
        nsm_encode_oam_gen_ipv4_data (pnt, size, &msg->tunnel_data.gen);
        break;
    }

  nsm_encode_oam_ctrl_data (pnt, size, &msg->ctrl_data);

  return *pnt - sp;
}

/* MPLS OAM control data decode. */
int
nsm_decode_oam_ctrl_data (u_char **pnt, u_int16_t *size,
                          struct mpls_oam_ctrl_data *msg)
{
  u_char *sp = *pnt;
  u_char i;

  TLV_DECODE_GET_IN4_ADDR (&msg->src_addr);
  TLV_DECODE_GETL (msg->in_ifindex);
  TLV_DECODE_GETL (msg->label_stack_depth);

  for (i = 0; i < msg->label_stack_depth; i++)
    {
      TLV_DECODE_GETL (msg->label_stack [i]);
    }
  TLV_DECODE_GETL (msg->input1);
  TLV_DECODE_GETL (msg->input2);

  return *pnt - sp;
}

/* Decode OAM echo request process message. */
int
nsm_decode_oam_req_process (u_char **pnt, u_int16_t *size,
                            struct nsm_msg_oam_lsp_ping_req_process *msg)
{
  u_char *sp = *pnt;
  struct nsm_tlv_header tlv;

  /* TLV parse.  */
  while (*size)
    {
      if (*size < NSM_TLV_HEADER_SIZE)
        return NSM_ERR_PKT_TOO_SMALL;

      NSM_DECODE_TLV_HEADER (tlv);

      switch (tlv.type)
        {
          case NSM_CTYPE_MSG_OAM_OPTION_LSP_TYPE:
            TLV_DECODE_GETC (msg->type);
            NSM_SET_CTYPE (msg->cindex, tlv.type);
            break;
          case NSM_CTYPE_MSG_OAM_OPTION_DSMAP:
            TLV_DECODE_GETC (msg->is_dsmap);
            NSM_SET_CTYPE (msg->cindex, tlv.type);
            break;
          case NSM_CTYPE_MSG_OAM_OPTION_LSP_ID:
            if (msg->type == MPLSONM_OPTION_L2CIRCUIT ||
                msg->type == MPLSONM_OPTION_VPLS)
              {
                TLV_DECODE_GETL (msg->u.l2_data.vc_id);
#ifdef HAVE_VCCV
                TLV_DECODE_GETC (msg->u.l2_data.is_vccv);
#endif /* HAVE_VCCV */
              }
            else if (msg->type == MPLSONM_OPTION_L3VPN)
              {
                TLV_DECODE_GETW (msg->u.fec.vrf_name_len);
                msg->u.fec.vrf_name = XCALLOC (MTYPE_VRF_NAME,
                                               msg->u.fec.vrf_name_len+1);
                TLV_DECODE_GET (msg->u.fec.vrf_name, msg->u.fec.vrf_name_len);
              }
            NSM_SET_CTYPE (msg->cindex, tlv.type);
            break;
          case NSM_CTYPE_MSG_OAM_OPTION_TUNNELNAME:
            TLV_DECODE_GETW (msg->u.rsvp.tunnel_len);
            msg->u.rsvp.tunnel_name = XMALLOC (MTYPE_TMP,
                                               msg->u.rsvp.tunnel_len);
            TLV_DECODE_GET (msg->u.rsvp.tunnel_name, msg->u.rsvp.tunnel_len);
            NSM_SET_CTYPE (msg->cindex, tlv.type);
            break;
          case NSM_CTYPE_MSG_OAM_OPTION_EGRESS:
            TLV_DECODE_GET_IN4_ADDR (&msg->u.rsvp.addr);
            NSM_SET_CTYPE (msg->cindex, tlv.type);
            break;
          case NSM_CTYPE_MSG_OAM_OPTION_PREFIX:
            TLV_DECODE_GET_IN4_ADDR (&msg->u.fec.vpn_prefix);
            TLV_DECODE_GETL (msg->u.fec.prefixlen);
            NSM_SET_CTYPE (msg->cindex, tlv.type);
            break;
          case NSM_CTYPE_MSG_OAM_OPTION_PEER:
            TLV_DECODE_GET_IN4_ADDR (&msg->u.l2_data.vc_peer);
            NSM_SET_CTYPE (msg->cindex, tlv.type);
            break;
          case NSM_CTYPE_MSG_OAM_OPTION_FEC:
            TLV_DECODE_GET_IN4_ADDR (&msg->u.fec.ip_addr);
            TLV_DECODE_GETL (msg->u.fec.prefixlen);
            NSM_SET_CTYPE (msg->cindex, tlv.type);
            break;
          case NSM_CTYPE_MSG_OAM_OPTION_SEQ_NUM:
            TLV_DECODE_GETL (msg->seq_no);
            NSM_SET_CTYPE (msg->cindex, tlv.type);
            break;
          default:
            TLV_DECODE_SKIP (tlv.length);
            break;
        }
    }

  return *pnt - sp;
}

/* Decode OAM reply process message for LDP type LSP. */
int
nsm_decode_oam_rep_process_ldp (u_char **pnt, u_int16_t *size,
                               struct nsm_msg_oam_lsp_ping_rep_process_ldp *msg)
{
  u_char *sp = *pnt;

  nsm_decode_oam_ldp_ipv4_data (pnt, size, &msg->data);

  TLV_DECODE_GETC (msg->is_dsmap);

  TLV_DECODE_GETL (msg->seq_no);

  nsm_decode_oam_ctrl_data (pnt, size, &msg->ctrl_data);

  return *pnt - sp;
}

/* Decode OAM reply process message for RSVP type LSP. */
int
nsm_decode_oam_rep_process_rsvp (u_char **pnt, u_int16_t *size,
                              struct nsm_msg_oam_lsp_ping_rep_process_rsvp *msg)
{
  u_char *sp = *pnt;

  nsm_decode_oam_rsvp_ipv4_data (pnt, size, &msg->data);

  TLV_DECODE_GETC (msg->is_dsmap);

  TLV_DECODE_GETL (msg->seq_no);

  nsm_decode_oam_ctrl_data (pnt, size, &msg->ctrl_data);

  return *pnt - sp;
}

/* Decode OAM reply process message for Static LSP. */
int
nsm_decode_oam_rep_process_gen (u_char **pnt, u_int16_t *size,
                               struct nsm_msg_oam_lsp_ping_rep_process_gen *msg)
{
  u_char *sp = *pnt;

  nsm_decode_oam_gen_ipv4_data (pnt, size, &msg->data);

  TLV_DECODE_GETC (msg->is_dsmap);

  TLV_DECODE_GETL (msg->seq_no);

  nsm_decode_oam_ctrl_data (pnt, size, &msg->ctrl_data);

  return *pnt - sp;
}

#ifdef HAVE_MPLS_VC
/* Decode OAM reply process message for L2VC. */
int
nsm_decode_oam_rep_process_l2vc (u_char **pnt, u_int16_t *size,
                              struct nsm_msg_oam_lsp_ping_rep_process_l2vc *msg)
{
  u_char *sp = *pnt;

  TLV_DECODE_GETW (msg->l2_data_type);
  TLV_DECODE_GETW (msg->tunnel_type);

  TLV_DECODE_GETC (msg->is_dsmap);

  TLV_DECODE_GETL (msg->seq_no);

  switch (msg->l2_data_type)
    {
      case NSM_OAM_L2_CIRCUIT_PREFIX_DEP:
        nsm_decode_oam_l2vc_prefix_dep (pnt, size, &msg->l2_data.pw_128_dep);
        break;
      case NSM_OAM_L2_CIRCUIT_PREFIX_CURR:
        nsm_decode_oam_l2vc_prefix_cur (pnt, size, &msg->l2_data.pw_128_cur);
        break;
      case NSM_OAM_GEN_PW_ID_PREFIX:
        nsm_decode_oam_gen_pw_id_prefix (pnt, size, &msg->l2_data.pw_129);
        break;
    }

  switch (msg->tunnel_type)
    {
      case NSM_OAM_LDP_IPv4_PREFIX:
        nsm_decode_oam_ldp_ipv4_data (pnt, size, &msg->tunnel_data.ldp);
        break;
      case NSM_OAM_RSVP_IPv4_PREFIX:
        nsm_decode_oam_rsvp_ipv4_data (pnt, size, &msg->tunnel_data.rsvp);
        break;
      case NSM_OAM_GEN_IPv4_PREFIX:
        nsm_decode_oam_gen_ipv4_data (pnt, size, &msg->tunnel_data.gen);
        break;
    }

  nsm_decode_oam_ctrl_data (pnt, size, &msg->ctrl_data);

  return *pnt - sp;
}
#endif /* HAVE_MPLS_VC */

/* Decode OAM reply process message for L3VPN. */
int
nsm_decode_oam_rep_process_l3vpn (u_char **pnt, u_int16_t *size,
                             struct nsm_msg_oam_lsp_ping_rep_process_l3vpn *msg)
{
  u_char *sp = *pnt;

  TLV_DECODE_GETW (msg->tunnel_type);

  TLV_DECODE_GETC (msg->is_dsmap);

  TLV_DECODE_GETL (msg->seq_no);

  nsm_decode_oam_l3vpn_data (pnt, size, &msg->data);

  switch (msg->tunnel_type)
    {
      case NSM_OAM_LDP_IPv4_PREFIX:
        nsm_decode_oam_ldp_ipv4_data (pnt, size, &msg->tunnel_data.ldp);
        break;
      case NSM_OAM_RSVP_IPv4_PREFIX:
        nsm_decode_oam_rsvp_ipv4_data (pnt, size, &msg->tunnel_data.rsvp);
        break;
      case NSM_OAM_GEN_IPv4_PREFIX:
        nsm_decode_oam_gen_ipv4_data (pnt, size, &msg->tunnel_data.gen);
        break;
    }

  nsm_decode_oam_ctrl_data (pnt, size, &msg->ctrl_data);

  return *pnt - sp;
}

/* Parse OAM request process message. */
int
nsm_parse_mpls_oam_req_process (u_char **pnt, u_int16_t *size,
                                struct nsm_msg_header *header, void *arg,
                                NSM_CALLBACK callback)
{
  int ret;
  struct nsm_msg_oam_lsp_ping_req_process msg;

  pal_mem_set (&msg, 0, sizeof (struct nsm_msg_oam_lsp_ping_req_process));

  ret = nsm_decode_oam_req_process (pnt, size, &msg);
  if (ret < 0)
    return ret;

  if (callback)
    {
      ret = (*callback) (header, arg, &msg);
      if (ret < 0)
        return ret;
    }
  return 0;
}

/* Parse OAM reply process message for LDP type LSP. */
int
nsm_parse_mpls_oam_rep_process_ldp (u_char **pnt, u_int16_t *size,
                                    struct nsm_msg_header *header, void *arg,
                                    NSM_CALLBACK callback)
{
  int ret;
  struct nsm_msg_oam_lsp_ping_rep_process_ldp msg;

  pal_mem_set (&msg, 0, sizeof (struct nsm_msg_oam_lsp_ping_rep_process_ldp));

  ret = nsm_decode_oam_rep_process_ldp (pnt, size, &msg);
  if (ret < 0)
    return ret;

  if (callback)
    {
      ret = (*callback) (header, arg, &msg);
      if (ret < 0)
        return ret;
    }
  return 0;
}

/* Parse OAM reply process message for RSVP type LSP. */
int
nsm_parse_mpls_oam_rep_process_rsvp (u_char **pnt, u_int16_t *size,
                                     struct nsm_msg_header *header, void *arg,
                                     NSM_CALLBACK callback)
{
  int ret;
  struct nsm_msg_oam_lsp_ping_rep_process_rsvp msg;

  pal_mem_set (&msg, 0, sizeof (struct nsm_msg_oam_lsp_ping_rep_process_rsvp));

  ret = nsm_decode_oam_rep_process_rsvp (pnt, size, &msg);
  if (ret < 0)
    return ret;

  if (callback)
    {
      ret = (*callback) (header, arg, &msg);
      if (ret < 0)
        return ret;
    }
  return 0;
}

/* Parse OAM reply process message for Static LSP. */
int
nsm_parse_mpls_oam_rep_process_gen (u_char **pnt, u_int16_t *size,
                                    struct nsm_msg_header *header, void *arg,
                                    NSM_CALLBACK callback)
{
  int ret;
  struct nsm_msg_oam_lsp_ping_rep_process_gen msg;

  pal_mem_set (&msg, 0, sizeof (struct nsm_msg_oam_lsp_ping_rep_process_gen));

  ret = nsm_decode_oam_rep_process_gen (pnt, size, &msg);
  if (ret < 0)
    return ret;

  if (callback)
    {
      ret = (*callback) (header, arg, &msg);
      if (ret < 0)
        return ret;
    }
  return 0;
}

#ifdef HAVE_MPLS_VC
/* Parse OAM reply process message for L2VC. */
int
nsm_parse_mpls_oam_rep_process_l2vc (u_char **pnt, u_int16_t *size,
                                     struct nsm_msg_header *header, void *arg,
                                     NSM_CALLBACK callback)
{
  int ret;
  struct nsm_msg_oam_lsp_ping_rep_process_l2vc msg;

  pal_mem_set (&msg, 0, sizeof (struct nsm_msg_oam_lsp_ping_rep_process_l2vc));

  ret = nsm_decode_oam_rep_process_l2vc (pnt, size, &msg);
  if (ret < 0)
    return ret;

  if (callback)
    {
      ret = (*callback) (header, arg, &msg);
      if (ret < 0)
        return ret;
    }
  return 0;
}
#endif /* HAVE_MPLS_VC */

/* Parse OAM reply process message for L3VPN. */
int
nsm_parse_mpls_oam_rep_process_l3vpn (u_char **pnt, u_int16_t *size,
                                      struct nsm_msg_header *header, void *arg,
                                      NSM_CALLBACK callback)
{
  int ret;
  struct nsm_msg_oam_lsp_ping_rep_process_l3vpn msg;

  pal_mem_set (&msg, 0, sizeof (struct nsm_msg_oam_lsp_ping_rep_process_l3vpn));

  ret = nsm_decode_oam_rep_process_l3vpn (pnt, size, &msg);
  if (ret < 0)
    return ret;

  if (callback)
    {
      ret = (*callback) (header, arg, &msg);
      if (ret < 0)
        return ret;
    }
  return 0;
}

/* Encode, Decode & Parse functions for echo request/reply processing
 * related response messages from NSM to OAMD.
 */

/* MPLS OAM DSMAP Data encode. */
int
nsm_encode_oam_dsmap_data (u_char **pnt, u_int16_t *size,
                           struct mpls_oam_downstream_map *msg)
{
  u_char *sp = *pnt;
  struct listnode *node;
  struct shimhdr *shim;

  TLV_ENCODE_PUTL (msg->u.l_packet);
  TLV_ENCODE_PUT_IN4_ADDR (&msg->ds_ip);
  TLV_ENCODE_PUT_IN4_ADDR (&msg->ds_if_ip);
  TLV_ENCODE_PUTC (msg->multipath_type);
  TLV_ENCODE_PUTC (msg->depth);
  TLV_ENCODE_PUTW (msg->multipath_len);

  if (msg->multipath_len > 0)
    {
      TLV_ENCODE_PUT (msg->multi_info, msg->multipath_len);
    }

  if (msg->ds_label)
    {
      TLV_ENCODE_PUTL (msg->ds_label->count);

      LIST_LOOP (msg->ds_label, shim, node)
      {
        TLV_ENCODE_PUTL (shim->shim);
      }
    }
  else
    TLV_ENCODE_PUTL (0);

  return *pnt - sp;
}

/* Encode LDP FEC data. */
int
nsm_encode_oam_ldp_ipv4_data (u_char **pnt, u_int16_t *size,
                              struct ldp_ipv4_prefix *msg)
{
  u_char *sp = *pnt;

  TLV_ENCODE_PUT_IN4_ADDR (&msg->ip_addr);
  TLV_ENCODE_PUTL (msg->u.l_packet);

  return *pnt - sp;
}

/* Encode echo request process-response to OAMD with LDP LSP data. */
int
nsm_encode_oam_ping_req_resp_ldp (u_char **pnt, u_int16_t *size,
                                  struct nsm_msg_oam_lsp_ping_req_resp_ldp *msg)
{
  u_char *sp = *pnt;

  nsm_encode_oam_ldp_ipv4_data (pnt, size, &msg->data);

  TLV_ENCODE_PUTC (msg->is_dsmap);
  if (msg->is_dsmap)
    nsm_encode_oam_dsmap_data (pnt, size, &msg->dsmap);

  TLV_ENCODE_PUTL (msg->seq_no);
  TLV_ENCODE_PUTL (msg->label.shim);
  TLV_ENCODE_PUTL (msg->ftn_ix);
  TLV_ENCODE_PUTL (msg->oif_ix);

  return *pnt - sp;
}

/* Encode RSVP FEC data. */
int
nsm_encode_oam_rsvp_ipv4_data (u_char **pnt, u_int16_t *size,
                             struct rsvp_ipv4_prefix *msg)
{
  u_char *sp = *pnt;

  TLV_ENCODE_PUT_IN4_ADDR (&msg->ip_addr);
  TLV_ENCODE_PUTW (msg->tunnel_id);
  TLV_ENCODE_PUT_IN4_ADDR (&msg->ext_tunnel_id);
  TLV_ENCODE_PUT_IN4_ADDR (&msg->send_addr);
  TLV_ENCODE_PUTW (msg->lsp_id);

  return *pnt - sp;
}

/* Encode echo request process-response to OAMD with RSVP LSP data. */
int
nsm_encode_oam_ping_req_resp_rsvp (u_char **pnt, u_int16_t *size,
                                 struct nsm_msg_oam_lsp_ping_req_resp_rsvp *msg)
{
  u_char *sp = *pnt;

  nsm_encode_oam_rsvp_ipv4_data (pnt, size, &msg->data);

  TLV_ENCODE_PUTC (msg->is_dsmap);
  if (msg->is_dsmap)
    nsm_encode_oam_dsmap_data (pnt, size, &msg->dsmap);

  TLV_ENCODE_PUTL (msg->seq_no);
  TLV_ENCODE_PUTL (msg->label.shim);
  TLV_ENCODE_PUTL (msg->ftn_ix);
  TLV_ENCODE_PUTL (msg->oif_ix);

  return *pnt - sp;
}

/* Encode Static FEC data. */
int
nsm_encode_oam_gen_ipv4_data (u_char **pnt, u_int16_t *size,
                             struct generic_ipv4_prefix *msg)
{
  u_char *sp = *pnt;

  TLV_ENCODE_PUT_IN4_ADDR (&msg->ipv4_addr);
  TLV_ENCODE_PUTL (msg->u.l_packet);

  return *pnt - sp;
}

/* Encode echo request process-response to OAMD with Static LSP data. */
int
nsm_encode_oam_ping_req_resp_gen (u_char **pnt, u_int16_t *size,
                                  struct nsm_msg_oam_lsp_ping_req_resp_gen *msg)
{
  u_char *sp = *pnt;

  nsm_encode_oam_gen_ipv4_data (pnt, size, &msg->data);

  TLV_ENCODE_PUTC (msg->is_dsmap);
  if (msg->is_dsmap)
    nsm_encode_oam_dsmap_data (pnt, size, &msg->dsmap);

  TLV_ENCODE_PUTL (msg->seq_no);
  TLV_ENCODE_PUTL (msg->label.shim);
  TLV_ENCODE_PUTL (msg->ftn_ix);
  TLV_ENCODE_PUTL (msg->oif_ix);

  return *pnt - sp;
}

#ifdef HAVE_MPLS_VC
/* Encode L2VC Deprecated data. */
int
nsm_encode_oam_l2vc_prefix_dep (u_char **pnt, u_int16_t *size,
                                struct l2_circuit_dep *msg)
{
  u_char *sp = *pnt;

  TLV_ENCODE_PUT_IN4_ADDR (&msg->remote);
  TLV_ENCODE_PUTL (msg->vc_id);
  TLV_ENCODE_PUTW (msg->pw_type);

  return *pnt - sp;
}

/* Encode L2VC current data. */
int
nsm_encode_oam_l2vc_prefix_cur (u_char **pnt, u_int16_t *size,
                                struct l2_circuit_curr *msg)
{
  u_char *sp = *pnt;

  TLV_ENCODE_PUT_IN4_ADDR (&msg->source);
  TLV_ENCODE_PUT_IN4_ADDR (&msg->remote);
  TLV_ENCODE_PUTL (msg->vc_id);
  TLV_ENCODE_PUTW (msg->pw_type);

  return *pnt - sp;
}
#endif /* HAVE_MPLS_VC */

/* Encode general PW data. */
int
nsm_encode_oam_gen_pw_id_prefix (u_char **pnt, u_int16_t *size,
                                 struct mpls_pw *msg)
{
  u_char *sp = *pnt;

  TLV_ENCODE_PUT_IN4_ADDR (&msg->source);
  TLV_ENCODE_PUT_IN4_ADDR (&msg->remote);
  TLV_ENCODE_PUTW (msg->pw_type);
  TLV_ENCODE_PUTC (msg->agi_type);
  TLV_ENCODE_PUTC (msg->agi_length);

  if (msg->agi_length > 0)
    TLV_ENCODE_PUT (msg->agi_value, msg->agi_length);

  TLV_ENCODE_PUTC (msg->saii_type);
  TLV_ENCODE_PUTC (msg->saii_length);

  if (msg->agi_length > 0)
    TLV_ENCODE_PUT (msg->saii_value, msg->saii_length);

  TLV_ENCODE_PUTC (msg->taii_type);
  TLV_ENCODE_PUTC (msg->taii_length);

  if (msg->agi_length > 0)
    TLV_ENCODE_PUT (msg->taii_value, msg->taii_length);

  return *pnt - sp;
}

#ifdef HAVE_MPLS_VC
/* Encode echo request process-response to OAMD with L2VC data. */
int
nsm_encode_oam_ping_req_resp_l2vc (u_char **pnt, u_int16_t *size,
                                 struct nsm_msg_oam_lsp_ping_req_resp_l2vc *msg)
{
  u_char *sp = *pnt;

  TLV_ENCODE_PUTW (msg->l2_data_type);
  TLV_ENCODE_PUTW (msg->tunnel_type);

  TLV_ENCODE_PUTC (msg->is_dsmap);
  if (msg->is_dsmap)
    nsm_encode_oam_dsmap_data (pnt, size, &msg->dsmap);

  TLV_ENCODE_PUTL (msg->seq_no);

  switch (msg->l2_data_type)
    {
      case NSM_OAM_L2_CIRCUIT_PREFIX_DEP:
        nsm_encode_oam_l2vc_prefix_dep (pnt, size, &msg->l2_data.pw_128_dep);
        break;
      case NSM_OAM_L2_CIRCUIT_PREFIX_CURR:
        nsm_encode_oam_l2vc_prefix_cur (pnt, size, &msg->l2_data.pw_128_cur);
        break;
      case NSM_OAM_GEN_PW_ID_PREFIX:
        nsm_encode_oam_gen_pw_id_prefix (pnt, size, &msg->l2_data.pw_129);
        break;
    }

  switch (msg->tunnel_type)
    {
      case NSM_OAM_LDP_IPv4_PREFIX:
        nsm_encode_oam_ldp_ipv4_data (pnt, size, &msg->tunnel_data.ldp);
        break;
      case NSM_OAM_RSVP_IPv4_PREFIX:
        nsm_encode_oam_rsvp_ipv4_data (pnt, size, &msg->tunnel_data.rsvp);
        break;
      case NSM_OAM_GEN_IPv4_PREFIX:
        nsm_encode_oam_gen_ipv4_data (pnt, size, &msg->tunnel_data.gen);
        break;
    }

  TLV_ENCODE_PUTC (msg->cc_type);

  TLV_ENCODE_PUTL (msg->tunnel_label.shim);
  TLV_ENCODE_PUTL (msg->vc_label.shim);

  TLV_ENCODE_PUTL (msg->ftn_ix);
  TLV_ENCODE_PUTL (msg->acc_if_index);
  TLV_ENCODE_PUTL (msg->oif_ix);

  return *pnt - sp;
}
#endif /* HAVE_MPLS_VC */

/* Encode L3VPN data */
int
nsm_encode_oam_l3vpn_data (u_char **pnt, u_int16_t *size,
                           struct l3vpn_ipv4_prefix *msg)
{
  u_char *sp = *pnt;
  u_int16_t i;

  TLV_ENCODE_PUT_IN4_ADDR (&msg->vpn_pfx);
  TLV_ENCODE_PUTL (msg->u.l_packet);

  for (i = 0; i < VPN_RD_SIZE; i++)
    {
      TLV_ENCODE_PUTC (msg->rd.u.rd_val [i]);
    }

  return *pnt - sp;
}

/* Encode echo request process-response to OAMD with L3VPN data. */
int
nsm_encode_oam_ping_req_resp_l3vpn (u_char **pnt, u_int16_t *size,
                               struct nsm_msg_oam_lsp_ping_req_resp_l3vpn *msg)
{
  u_char *sp = *pnt;

  TLV_ENCODE_PUTW (msg->tunnel_type);

  TLV_ENCODE_PUTC (msg->is_dsmap);
  if (msg->is_dsmap)
    nsm_encode_oam_dsmap_data (pnt, size, &msg->dsmap);

  TLV_ENCODE_PUTL (msg->seq_no);

  nsm_encode_oam_l3vpn_data (pnt, size, &msg->data);

  TLV_ENCODE_PUTL (msg->vrf_id);

  switch (msg->tunnel_type)
    {
      case NSM_OAM_LDP_IPv4_PREFIX:
        nsm_encode_oam_ldp_ipv4_data (pnt, size, &msg->tunnel_data.ldp);
        break;
      case NSM_OAM_RSVP_IPv4_PREFIX:
        nsm_encode_oam_rsvp_ipv4_data (pnt, size, &msg->tunnel_data.rsvp);
        break;
      case NSM_OAM_GEN_IPv4_PREFIX:
        nsm_encode_oam_gen_ipv4_data (pnt, size, &msg->tunnel_data.gen);
        break;
    }

  TLV_ENCODE_PUTL (msg->tunnel_label.shim);
  TLV_ENCODE_PUTL (msg->vpn_label.shim);
  TLV_ENCODE_PUTL (msg->ftn_ix);
  TLV_ENCODE_PUTL (msg->oif_ix);

  return *pnt - sp;
}

/* Encode error-response to OAMD. */
int
nsm_encode_oam_ping_req_resp_err (u_char **pnt, u_int16_t *size,
                                  struct nsm_msg_oam_lsp_ping_req_resp_err *msg)
{
  u_char *sp = *pnt;

  TLV_ENCODE_PUTL (msg->seq_no);
  TLV_ENCODE_PUTL (msg->err_code);

  return *pnt - sp;
}

/* Encode echo reply process-response to OAMD. */
int
nsm_encode_oam_ping_rep_resp (u_char **pnt, u_int16_t *size,
                              struct nsm_msg_oam_lsp_ping_rep_resp *msg)
{
  u_char *sp = *pnt;

  TLV_ENCODE_PUTL (msg->seq_no);
  TLV_ENCODE_PUTC (msg->err_code);
  TLV_ENCODE_PUTC (msg->return_code);
  TLV_ENCODE_PUTC (msg->ret_subcode);
  TLV_ENCODE_PUTC (msg->is_dsmap);

  nsm_encode_oam_dsmap_data (pnt, size, &msg->dsmap);

  return *pnt - sp;
}

/* Encode OAM params update message. */
int
nsm_encode_oam_update (u_char **pnt, u_int16_t *size,
                       struct nsm_msg_oam_update *msg)
{
  u_char *sp = *pnt;

  TLV_ENCODE_PUTC (msg->param_type);

  switch (msg->param_type)
  {
    case NSM_MSG_OAM_VC_UPDATE:
     TLV_ENCODE_PUTL (msg->param.vc_update.vc_id);
     TLV_ENCODE_PUT_IN4_ADDR (&msg->param.vc_update.peer);
     TLV_ENCODE_PUTC (msg->param.vc_update.update_type);
    break;
    case NSM_MSG_OAM_FTN_UPDATE:
     TLV_ENCODE_PUTL (msg->param.ftn_update.ftn_ix);
     TLV_ENCODE_PUTC (msg->param.ftn_update.update_type);
    break;
  }

  return *pnt - sp;
}

/* MPLS OAM DSMAP Data encode. */
int
nsm_decode_oam_dsmap_data (u_char **pnt, u_int16_t *size,
                           struct mpls_oam_downstream_map *msg)
{
  u_char *sp = *pnt;
  u_int32_t ds_label_count = 0;
  u_int32_t ds_label_data = 0;
  struct shimhdr *shim = NULL;
  u_int32_t i;

  TLV_DECODE_GETL (msg->u.l_packet);
  TLV_DECODE_GET_IN4_ADDR (&msg->ds_ip);
  TLV_DECODE_GET_IN4_ADDR (&msg->ds_if_ip);
  TLV_DECODE_GETC (msg->multipath_type);
  TLV_DECODE_GETC (msg->depth);
  TLV_DECODE_GETW (msg->multipath_len);

  if (msg->multipath_len > 0)
    {
      TLV_DECODE_GET (msg->multi_info, msg->multipath_len);
    }

  TLV_DECODE_GETL (ds_label_count);

  if (ds_label_count)
    {
      msg->ds_label = list_new();
      for (i = 0; i < ds_label_count; i++)
        {
          TLV_DECODE_GETL (ds_label_data);
          shim = XCALLOC (MTYPE_TMP, sizeof (struct shimhdr));
          shim->shim = ds_label_data;
          listnode_add (msg->ds_label, shim);
        }
    }

  return *pnt - sp;
}

/* Decode LDP FEC data. */
int
nsm_decode_oam_ldp_ipv4_data (u_char **pnt, u_int16_t *size,
                              struct ldp_ipv4_prefix *msg)
{
  u_char *sp = *pnt;

  TLV_DECODE_GET_IN4_ADDR (&msg->ip_addr);
  TLV_DECODE_GETL (msg->u.l_packet);

  return *pnt - sp;
}

/* Decode LDP LSP data for echo request process-response from NSM. */
int
nsm_decode_oam_ping_req_resp_ldp (u_char **pnt, u_int16_t *size,
                                  struct nsm_msg_oam_lsp_ping_req_resp_ldp *msg)
{
  u_char *sp = *pnt;

  nsm_decode_oam_ldp_ipv4_data (pnt, size, &msg->data);

  TLV_DECODE_GETC (msg->is_dsmap);
  if (msg->is_dsmap)
    nsm_decode_oam_dsmap_data (pnt, size, &msg->dsmap);

  TLV_DECODE_GETL (msg->seq_no);
  TLV_DECODE_GETL (msg->label.shim);
  TLV_DECODE_GETL (msg->ftn_ix);
  TLV_DECODE_GETL (msg->oif_ix);

  return *pnt - sp;
}

/* Decode RSVP FEC data. */
int
nsm_decode_oam_rsvp_ipv4_data (u_char **pnt, u_int16_t *size,
                             struct rsvp_ipv4_prefix *msg)
{
  u_char *sp = *pnt;

  TLV_DECODE_GET_IN4_ADDR (&msg->ip_addr);
  TLV_DECODE_GETW (msg->tunnel_id);
  TLV_DECODE_GET_IN4_ADDR (&msg->ext_tunnel_id);
  TLV_DECODE_GET_IN4_ADDR (&msg->send_addr);
  TLV_DECODE_GETW (msg->lsp_id);

  return *pnt - sp;
}

/* Decode RSVP LSP data for echo request process-response from NSM. */
int
nsm_decode_oam_ping_req_resp_rsvp (u_char **pnt, u_int16_t *size,
                                 struct nsm_msg_oam_lsp_ping_req_resp_rsvp *msg)
{
  u_char *sp = *pnt;

  nsm_decode_oam_rsvp_ipv4_data (pnt, size, &msg->data);

  TLV_DECODE_GETC (msg->is_dsmap);
  if (msg->is_dsmap)
    nsm_decode_oam_dsmap_data (pnt, size, &msg->dsmap);

  TLV_DECODE_GETL (msg->seq_no);
  TLV_DECODE_GETL (msg->label.shim);
  TLV_DECODE_GETL (msg->ftn_ix);
  TLV_DECODE_GETL (msg->oif_ix);

  return *pnt - sp;
}

/* Decode Static FEC data. */
int
nsm_decode_oam_gen_ipv4_data (u_char **pnt, u_int16_t *size,
                             struct generic_ipv4_prefix *msg)
{
  u_char *sp = *pnt;

  TLV_DECODE_GET_IN4_ADDR (&msg->ipv4_addr);
  TLV_DECODE_GETL (msg->u.l_packet);

  return *pnt - sp;
}

/* Decode Static LSP data for echo request process-response from NSM. */
int
nsm_decode_oam_ping_req_resp_gen (u_char **pnt, u_int16_t *size,
                                  struct nsm_msg_oam_lsp_ping_req_resp_gen *msg)
{
  u_char *sp = *pnt;

  nsm_decode_oam_gen_ipv4_data (pnt, size, &msg->data);

  TLV_DECODE_GETC (msg->is_dsmap);
  if (msg->is_dsmap)
    nsm_decode_oam_dsmap_data (pnt, size, &msg->dsmap);

  TLV_DECODE_GETL (msg->seq_no);
  TLV_DECODE_GETL (msg->label.shim);
  TLV_DECODE_GETL (msg->ftn_ix);
  TLV_DECODE_GETL (msg->oif_ix);

  return *pnt - sp;
}

#ifdef HAVE_MPLS_VC
/* Decode L2VC Dependent TLV data. */
int
nsm_decode_oam_l2vc_prefix_dep (u_char **pnt, u_int16_t *size,
                                struct l2_circuit_dep *msg)
{
  u_char *sp = *pnt;

  TLV_DECODE_GET_IN4_ADDR (&msg->remote);
  TLV_DECODE_GETL (msg->vc_id);
  TLV_DECODE_GETW (msg->pw_type);

  return *pnt - sp;
}

/* Decode L2VC current TLV data. */
int
nsm_decode_oam_l2vc_prefix_cur (u_char **pnt, u_int16_t *size,
                                struct l2_circuit_curr *msg)
{
  u_char *sp = *pnt;

  TLV_DECODE_GET_IN4_ADDR (&msg->source);
  TLV_DECODE_GET_IN4_ADDR (&msg->remote);
  TLV_DECODE_GETL (msg->vc_id);
  TLV_DECODE_GETW (msg->pw_type);

  return *pnt - sp;
}
#endif /* HAVE_MPLS_VC */

/* Decode general PW TLV data. */
int
nsm_decode_oam_gen_pw_id_prefix (u_char **pnt, u_int16_t *size,
                                 struct mpls_pw *msg)
{
  u_char *sp = *pnt;

  TLV_DECODE_GET_IN4_ADDR (&msg->source);
  TLV_DECODE_GET_IN4_ADDR (&msg->remote);
  TLV_DECODE_GETW (msg->pw_type);
  TLV_DECODE_GETC (msg->agi_type);
  TLV_DECODE_GETC (msg->agi_length);

  if (msg->agi_length > 0)
    TLV_DECODE_GET (msg->agi_value, msg->agi_length);

  TLV_DECODE_GETC (msg->saii_type);
  TLV_DECODE_GETC (msg->saii_length);

  if (msg->agi_length > 0)
    TLV_DECODE_GET (msg->saii_value, msg->saii_length);

  TLV_DECODE_GETC (msg->taii_type);
  TLV_DECODE_GETC (msg->taii_length);

  if (msg->agi_length > 0)
    TLV_DECODE_GET (msg->taii_value, msg->taii_length);

  return *pnt - sp;
}

#ifdef HAVE_MPLS_VC
/* Decode L2VC data for echo request process-response from NSM. */
int
nsm_decode_oam_ping_req_resp_l2vc (u_char **pnt, u_int16_t *size,
                                 struct nsm_msg_oam_lsp_ping_req_resp_l2vc *msg)
{
  u_char *sp = *pnt;

  TLV_DECODE_GETW (msg->l2_data_type);
  TLV_DECODE_GETW (msg->tunnel_type);

  TLV_DECODE_GETC (msg->is_dsmap);
  if (msg->is_dsmap)
    nsm_decode_oam_dsmap_data (pnt, size, &msg->dsmap);

  TLV_DECODE_GETL (msg->seq_no);

  switch (msg->l2_data_type)
    {
      case NSM_OAM_L2_CIRCUIT_PREFIX_DEP:
        nsm_decode_oam_l2vc_prefix_dep (pnt, size, &msg->l2_data.pw_128_dep);
        break;
      case NSM_OAM_L2_CIRCUIT_PREFIX_CURR:
        nsm_decode_oam_l2vc_prefix_cur (pnt, size, &msg->l2_data.pw_128_cur);
        break;
      case NSM_OAM_GEN_PW_ID_PREFIX:
        nsm_decode_oam_gen_pw_id_prefix (pnt, size, &msg->l2_data.pw_129);
        break;
    }

  switch (msg->tunnel_type)
    {
      case NSM_OAM_LDP_IPv4_PREFIX:
        nsm_decode_oam_ldp_ipv4_data (pnt, size, &msg->tunnel_data.ldp);
        break;
      case NSM_OAM_RSVP_IPv4_PREFIX:
        nsm_decode_oam_rsvp_ipv4_data (pnt, size, &msg->tunnel_data.rsvp);
        break;
      case NSM_OAM_GEN_IPv4_PREFIX:
        nsm_decode_oam_gen_ipv4_data (pnt, size, &msg->tunnel_data.gen);
        break;
    }
  TLV_DECODE_GETC (msg->cc_type);

  TLV_DECODE_GETL (msg->tunnel_label.shim);
  TLV_DECODE_GETL (msg->vc_label.shim);

  TLV_DECODE_GETL (msg->ftn_ix);
  TLV_DECODE_GETL (msg->acc_if_index);
  TLV_DECODE_GETL (msg->oif_ix);

  return *pnt - sp;
}
#endif /* HAVE_MPLS_VC */

/* Decode L3VPN data */
int
nsm_decode_oam_l3vpn_data (u_char **pnt, u_int16_t *size,
                           struct l3vpn_ipv4_prefix *msg)
{
  u_char *sp = *pnt;
  u_int16_t i;

  TLV_DECODE_GET_IN4_ADDR (&msg->vpn_pfx);
  TLV_DECODE_GETL (msg->u.l_packet);

  for (i = 0; i < VPN_RD_SIZE; i++)
    {
      TLV_DECODE_GETC (msg->rd.u.rd_val [i]);
    }

  return *pnt - sp;
}

/* Decode L3VPN data for echo request process-response from NSM. */
int
nsm_decode_oam_ping_req_resp_l3vpn (u_char **pnt, u_int16_t *size,
                               struct nsm_msg_oam_lsp_ping_req_resp_l3vpn *msg)
{
  u_char *sp = *pnt;

  TLV_DECODE_GETW (msg->tunnel_type);

  TLV_DECODE_GETC (msg->is_dsmap);
  if (msg->is_dsmap)
    nsm_decode_oam_dsmap_data (pnt, size, &msg->dsmap);

  TLV_DECODE_GETL (msg->seq_no);

  nsm_decode_oam_l3vpn_data (pnt, size, &msg->data);

  TLV_DECODE_GETL (msg->vrf_id);

  switch (msg->tunnel_type)
    {
      case NSM_OAM_LDP_IPv4_PREFIX:
        nsm_decode_oam_ldp_ipv4_data (pnt, size, &msg->tunnel_data.ldp);
        break;
      case NSM_OAM_RSVP_IPv4_PREFIX:
        nsm_decode_oam_rsvp_ipv4_data (pnt, size, &msg->tunnel_data.rsvp);
        break;
      case NSM_OAM_GEN_IPv4_PREFIX:
        nsm_decode_oam_gen_ipv4_data (pnt, size, &msg->tunnel_data.gen);
        break;
    }

  TLV_DECODE_GETL (msg->tunnel_label.shim);
  TLV_DECODE_GETL (msg->vpn_label.shim);
  TLV_DECODE_GETL (msg->ftn_ix);
  TLV_DECODE_GETL (msg->oif_ix);

  return *pnt - sp;
}

/* Decode error-response from NSM. */
int
nsm_decode_oam_ping_req_resp_err (u_char **pnt, u_int16_t *size,
                                  struct nsm_msg_oam_lsp_ping_req_resp_err *msg)
{
  u_char *sp = *pnt;

  TLV_DECODE_GETL (msg->seq_no);
  TLV_DECODE_GETL (msg->err_code);

  return *pnt - sp;
}

/* Decode echo reply process-response from NSM. */
int
nsm_decode_oam_ping_rep_resp (u_char **pnt, u_int16_t *size,
                              struct nsm_msg_oam_lsp_ping_rep_resp *msg)
{
  u_char *sp = *pnt;

  TLV_DECODE_GETL (msg->seq_no);
  TLV_DECODE_GETC (msg->err_code);
  TLV_DECODE_GETC (msg->return_code);
  TLV_DECODE_GETC (msg->ret_subcode);
  TLV_DECODE_GETC (msg->is_dsmap);

  nsm_decode_oam_dsmap_data (pnt, size, &msg->dsmap);

  return *pnt - sp;
}

/* Decode OAM params update message from NSM. */
int
nsm_decode_oam_update (u_char **pnt, u_int16_t *size,
                       struct nsm_msg_oam_update *msg)
{
  u_char *sp = *pnt;

  TLV_DECODE_GETC (msg->param_type);

  switch (msg->param_type)
  {
    case NSM_MSG_OAM_VC_UPDATE:
     TLV_DECODE_GETL (msg->param.vc_update.vc_id);
     TLV_DECODE_GET_IN4_ADDR (&msg->param.vc_update.peer);
     TLV_DECODE_GETC (msg->param.vc_update.update_type);
    break;
    case NSM_MSG_OAM_FTN_UPDATE:
     TLV_DECODE_GETL (msg->param.ftn_update.ftn_ix);
     TLV_DECODE_GETC (msg->param.ftn_update.update_type);
    break;
  }

  return *pnt - sp;
}

/* Parse OAM request process-response message for LDP LSP. */
int
nsm_parse_oam_ping_req_resp_ldp (u_char **pnt, u_int16_t *size,
                                 struct nsm_msg_header *header, void *arg,
                                 NSM_CALLBACK callback)
{
  struct nsm_msg_oam_lsp_ping_req_resp_ldp msg;
  int ret;

  pal_mem_set (&msg, 0, sizeof (struct nsm_msg_oam_lsp_ping_req_resp_ldp));
  ret = nsm_decode_oam_ping_req_resp_ldp (pnt, size, &msg);
  if (ret < 0)
    return ret;

  if (callback)
    {
      ret = (*callback) (header, arg, &msg);
      if (ret < 0)
        return ret;
    }

  return 0;
}

/* Parse OAM request process-response message for RSVP LSP. */
int
nsm_parse_oam_ping_req_resp_rsvp (u_char **pnt, u_int16_t *size,
                                  struct nsm_msg_header *header, void *arg,
                                  NSM_CALLBACK callback)
{
  struct nsm_msg_oam_lsp_ping_req_resp_rsvp msg;
  int ret;

  pal_mem_set (&msg, 0, sizeof (struct nsm_msg_oam_lsp_ping_req_resp_rsvp));
  ret = nsm_decode_oam_ping_req_resp_rsvp (pnt, size, &msg);
  if (ret < 0)
    return ret;

  if (callback)
    {
      ret = (*callback) (header, arg, &msg);
      if (ret < 0)
        return ret;
    }

  return 0;
}

/* Parse OAM request process-response message for Static LSP. */
int
nsm_parse_oam_ping_req_resp_gen (u_char **pnt, u_int16_t *size,
                                 struct nsm_msg_header *header, void *arg,
                                 NSM_CALLBACK callback)
{
  struct nsm_msg_oam_lsp_ping_req_resp_gen msg;
  int ret;

  pal_mem_set (&msg, 0, sizeof (struct nsm_msg_oam_lsp_ping_req_resp_gen));
  ret = nsm_decode_oam_ping_req_resp_gen (pnt, size, &msg);
  if (ret < 0)
    return ret;

  if (callback)
    {
      ret = (*callback) (header, arg, &msg);
      if (ret < 0)
        return ret;
    }

  return 0;
}

#ifdef HAVE_MPLS_VC
/* Parse OAM request process-response message for L2VC. */
int
nsm_parse_oam_ping_req_resp_l2vc (u_char **pnt, u_int16_t *size,
                                  struct nsm_msg_header *header, void *arg,
                                  NSM_CALLBACK callback)
{
  struct nsm_msg_oam_lsp_ping_req_resp_l2vc msg;
  int ret;

  pal_mem_set (&msg, 0, sizeof (struct nsm_msg_oam_lsp_ping_req_resp_l2vc));
  ret = nsm_decode_oam_ping_req_resp_l2vc (pnt, size, &msg);
  if (ret < 0)
    return ret;

  if (callback)
    {
      ret = (*callback) (header, arg, &msg);
      if (ret < 0)
        return ret;
    }

  return 0;
}
#endif /* HAVE_MPLS_VC */

/* Parse OAM request process-response message for L3VPN. */
int
nsm_parse_oam_ping_req_resp_l3vpn (u_char **pnt, u_int16_t *size,
                                   struct nsm_msg_header *header, void *arg,
                                   NSM_CALLBACK callback)
{
  struct nsm_msg_oam_lsp_ping_req_resp_l3vpn msg;
  int ret;

  pal_mem_set (&msg, 0, sizeof (struct nsm_msg_oam_lsp_ping_req_resp_l3vpn));
  ret = nsm_decode_oam_ping_req_resp_l3vpn (pnt, size, &msg);
  if (ret < 0)
    return ret;

  if (callback)
    {
      ret = (*callback) (header, arg, &msg);
      if (ret < 0)
        return ret;
    }

  return 0;
}

/* Parse OAM request process-response error message. */
int
nsm_parse_oam_ping_req_resp_err (u_char **pnt, u_int16_t *size,
                                 struct nsm_msg_header *header, void *arg,
                                 NSM_CALLBACK callback)
{
  struct nsm_msg_oam_lsp_ping_req_resp_err msg;
  int ret;

  pal_mem_set (&msg, 0, sizeof (struct nsm_msg_oam_lsp_ping_req_resp_err));
  ret = nsm_decode_oam_ping_req_resp_err (pnt, size, &msg);
  if (ret < 0)
    return ret;

  if (callback)
    {
      ret = (*callback) (header, arg, &msg);
      if (ret < 0)
        return ret;
    }

  return 0;
}

/* Parse OAM reply process-response message. */
int
nsm_parse_oam_ping_rep_resp (u_char **pnt, u_int16_t *size,
                             struct nsm_msg_header *header, void *arg,
                             NSM_CALLBACK callback)
{
  struct nsm_msg_oam_lsp_ping_rep_resp msg;
  int ret;

  pal_mem_set (&msg, 0, sizeof (struct nsm_msg_oam_lsp_ping_rep_resp));
  ret = nsm_decode_oam_ping_rep_resp (pnt, size, &msg);
  if (ret < 0)
    return ret;

  if (callback)
    {
      ret = (*callback) (header, arg, &msg);
      if (ret < 0)
        return ret;
    }

  return 0;
}

/* Parse OAM reply process-response message. */
int
nsm_parse_oam_update (u_char **pnt, u_int16_t *size,
                      struct nsm_msg_header *header, void *arg,
                      NSM_CALLBACK callback)
{
  struct nsm_msg_oam_update msg;
  int ret;

  pal_mem_set (&msg, 0, sizeof (struct nsm_msg_oam_update));
  ret = nsm_decode_oam_update (pnt, size, &msg);
  if (ret < 0)
    return ret;

  if (callback)
    {
      ret = (*callback) (header, arg, &msg);
      if (ret < 0)
        return ret;
    }

  return 0;
}
#endif /* HAVE_MPLS_OAM */

#ifdef HAVE_GMPLS
s_int32_t
nsm_encode_link_id (u_char **pnt, u_int16_t *size,
                    struct gmpls_link_id *link_id)
{
  u_char *sp = *pnt;

  TLV_ENCODE_PUTC (link_id->type);

#ifdef HAVE_IPV6
  if (link_id->type == GMPLS_LINK_ID_TYPE_IPV6)
    {
      TLV_ENCODE_PUT (&link_id->linkid.ipv6, sizeof (struct pal_in6_addr));
    }
  else
#endif /* HAVE_IPV6*/
    {
      if (link_id->type == GMPLS_LINK_ID_TYPE_IPV4)
        {
          TLV_ENCODE_PUT (&link_id->linkid.ipv4,
                         sizeof (struct pal_in4_addr));
        }
      else
        {
          TLV_ENCODE_PUTL (link_id->linkid.unnumbered);
        }
    }

  return *pnt - sp;
}

s_int32_t
nsm_decode_link_id (u_char **pnt, u_int16_t *size,
                    struct gmpls_link_id *link_id)
{
  u_char *sp = *pnt;

  TLV_DECODE_GETC (link_id->type);

#ifdef HAVE_IPV6
  if (link_id->type == GMPLS_LINK_ID_TYPE_IPV6)
    {
      TLV_DECODE_GET (&link_id->linkid.ipv6, sizeof (struct pal_in6_addr));
    }
  else
#endif /* HAVE_IPV6*/
    {
      if (link_id->type == GMPLS_LINK_ID_TYPE_IPV4)
        {
          TLV_DECODE_GET (&link_id->linkid.ipv4,
                         sizeof (struct pal_in4_addr));
        }
      else
        {
          TLV_DECODE_GETL (link_id->linkid.unnumbered);
        }
    }

  return *pnt - sp;
}

s_int32_t
nsm_encode_link_properties (u_char **pnt, u_int16_t *size,
                            struct link_properties *prop)
{
  u_char *sp = *pnt, cnt = 0;

#ifdef HAVE_PACKET
  TLV_ENCODE_PUTF (prop->max_lsp_size);
#endif /* HAVE_PACKET */

  while (cnt < MAX_PRIORITIES)
    {
      TLV_ENCODE_PUTF (prop->max_lsp_bw [cnt]);
      cnt ++;
    }

  TLV_ENCODE_PUTF (prop->bandwidth);
  TLV_ENCODE_PUTF (prop->max_resv_bw);

  cnt  = 0;
  while (cnt < MAX_PRIORITIES)
    {
      TLV_ENCODE_PUTF (prop->tecl_priority_bw [cnt]);
      cnt ++;
    }

#if defined HAVE_DSTE && defined HAVE_PACKET
  cnt = 0;
  while (cnt < MAX_BW_CONST)
    {
      TLV_ENCODE_PUTF (prop->bw_constraint[cnt]);
      cnt ++;
    }
#endif /* HAVE_DSTE && HAVE_PACKET */

  return *pnt - sp;
}

s_int32_t
nsm_decode_link_properties (u_char **pnt, u_int16_t *size,
                            struct link_properties *prop)
{
  u_char *sp = *pnt, cnt = 0;

#ifdef HAVE_PACKET
  TLV_DECODE_GETF (prop->max_lsp_size);
#endif /* HAVE_PACKET */

  while (cnt < MAX_PRIORITIES)
    {
      TLV_DECODE_GETF (prop->max_lsp_bw [cnt]);
      cnt ++;
    }

  TLV_DECODE_GETF (prop->bandwidth);
  TLV_DECODE_GETF (prop->max_resv_bw);

  cnt  = 0;
  while (cnt < MAX_PRIORITIES)
    {
      TLV_DECODE_GETF (prop->tecl_priority_bw [cnt]);
      cnt ++;
    }

#if defined HAVE_DSTE && defined HAVE_PACKET
  cnt = 0;
  while (cnt < MAX_BW_CONST)
    {
      TLV_DECODE_GETF (prop->bw_constraint[cnt]);
      cnt ++;
    }
#endif /* HAVE_DSTE && HAVE_PACKET */

  return *pnt - sp;
}

s_int32_t
nsm_encode_phy_property (u_char **pnt, u_int16_t *size,
                         struct phy_properties *phy)
{
  u_char *sp = *pnt;

  TLV_ENCODE_PUTL (phy->protection);
  TLV_ENCODE_PUTL (phy->switching_cap);
  TLV_ENCODE_PUTW (phy->encoding_type);
  TLV_ENCODE_PUTF (phy->min_lsp_bw);

  return *pnt - sp;
}

s_int32_t
nsm_decode_phy_property (u_char **pnt, u_int16_t *size,
                         struct phy_properties *phy)
{
  u_char *sp = *pnt;

  TLV_DECODE_GETL (phy->protection);
  TLV_DECODE_GETL (phy->switching_cap);
  TLV_DECODE_GETW (phy->encoding_type);
  TLV_DECODE_GETF (phy->min_lsp_bw);

  return *pnt - sp;
}


s_int32_t
nsm_encode_srlg_property (u_char **pnt, u_int16_t *size,
                          struct nsm_msg_te_link *msg)
{
  u_char cnt = 0;
  u_char *sp = *pnt;

  TLV_ENCODE_PUTC (msg->srlg_num);

  while (cnt < msg->srlg_num)
    {
      TLV_ENCODE_PUTL (msg->srlg [cnt]);
      cnt ++;
      if (cnt >= MAX_NUM_SRLG_SUPPORTED)
        {
          break;
        }
    }

  return *pnt - sp;
}

s_int32_t
nsm_decode_srlg_property (u_char **pnt, u_int16_t *size,
                          struct nsm_msg_te_link *msg)
{
  u_char cnt = 0;
  u_char *sp = *pnt;

  TLV_DECODE_GETC (msg->srlg_num);

  if (msg->srlg_num)
    msg->srlg = XCALLOC (MTYPE_GMPLS_SRLG, sizeof(u_int32_t)* msg->srlg_num);

  if (!msg->srlg)
    return -1;

  while (cnt < msg->srlg_num)
    {
      TLV_DECODE_GETL (msg->srlg [cnt]);
      cnt ++;
      if (cnt >= MAX_NUM_SRLG_SUPPORTED)
        {
          break;
        }
    }

  return *pnt - sp;
}

s_int32_t
nsm_encode_te_link (u_char **pnt, u_int16_t *size,
                    struct nsm_msg_te_link *msg)
{
  u_char *sp = *pnt;

  /* Cindex */
  TLV_ENCODE_PUTL (msg->cindex);

  /* Name */
  TLV_ENCODE_PUT (msg->name, GINTERFACE_NAMSIZ);

  /* Put gifindex */
  TLV_ENCODE_PUTL (msg->gifindex);

  if (NSM_CHECK_CTYPE_FLAG (msg->cindex, NSM_TELINK_CTYPE_ADD) ||
      NSM_CHECK_CTYPE_FLAG (msg->cindex, NSM_TELINK_CTYPE_LOCAL_LINKID))
    nsm_encode_link_id (pnt, size, &msg->l_linkid);

  if (NSM_CHECK_CTYPE_FLAG (msg->cindex, NSM_TELINK_CTYPE_ADD) ||
      NSM_CHECK_CTYPE_FLAG (msg->cindex, NSM_TELINK_CTYPE_REMOTE_LINKID))
    nsm_encode_link_id (pnt, size, &msg->r_linkid);

  if (NSM_CHECK_CTYPE_FLAG (msg->cindex, NSM_TELINK_CTYPE_ADD) ||
      NSM_CHECK_CTYPE_FLAG (msg->cindex, NSM_TELINK_CTYPE_STATUS))
    TLV_ENCODE_PUTC (msg->status);

#ifdef HAVE_DSTE
  if (NSM_CHECK_CTYPE_FLAG (msg->cindex, NSM_TELINK_CTYPE_ADD) ||
      NSM_CHECK_CTYPE_FLAG (msg->cindex, NSM_TELINK_CTYPE_BCMODE))
    TLV_ENCODE_PUTC (msg->mode);
#endif /* HAVE_DSTE */

  if (NSM_CHECK_CTYPE_FLAG (msg->cindex, NSM_TELINK_CTYPE_ADD) ||
      NSM_CHECK_CTYPE_FLAG (msg->cindex,NSM_TELINK_CTYPE_LINK_ID))
    {
      TLV_ENCODE_PUT_IN4_ADDR (&msg->linkid);
    }

  if (NSM_CHECK_CTYPE_FLAG (msg->cindex, NSM_TELINK_CTYPE_ADD) ||
      NSM_CHECK_CTYPE_FLAG (msg->cindex, NSM_TELINK_CTYPE_CAGIFINDEX))
    {
      TLV_ENCODE_PUTL (msg->ca_gifindex);
      TLV_ENCODE_PUT (msg->caname, GINTERFACE_NAMSIZ);
    }

  if (NSM_CHECK_CTYPE_FLAG (msg->cindex, NSM_TELINK_CTYPE_ADD) ||
      NSM_CHECK_CTYPE_FLAG (msg->cindex, NSM_TELINK_CTYPE_ADDR))
    {
      TLV_ENCODE_PUTC (msg->addr.family);
#ifdef HAVE_IPV6
      if (msg->addr.family == AF_INET6)
        {
          TLV_ENCODE_PUT_IN6_ADDR (&msg->addr.u.prefix6);
        }
      else
#endif /* HAVE_IPV6 */
        {
          TLV_ENCODE_PUT_IN4_ADDR (&msg->addr.u.prefix4);
        }
    }

  if (NSM_CHECK_CTYPE_FLAG (msg->cindex, NSM_TELINK_CTYPE_ADD) ||
      NSM_CHECK_CTYPE_FLAG (msg->cindex, NSM_TELINK_CTYPE_REMOTE_ADDR))
    {
      TLV_ENCODE_PUTC (msg->remote_addr.family);
#ifdef HAVE_IPV6
      if (msg->remote_addr.family == AF_INET6)
        {
          TLV_ENCODE_PUT_IN6_ADDR (&msg->remote_addr.u.prefix6);
        }
      else
#endif /* HAVE_IPV6 */
        {
          TLV_ENCODE_PUT_IN4_ADDR (&msg->remote_addr.u.prefix4);
        }
    }

  if (NSM_CHECK_CTYPE_FLAG (msg->cindex, NSM_TELINK_CTYPE_ADD) ||
      NSM_CHECK_CTYPE_FLAG (msg->cindex, NSM_TELINK_CTYPE_LINK_PROPERTY) ||
      NSM_CHECK_CTYPE_FLAG (msg->cindex, NSM_TELINK_CTYPE_TE_PROPERTY))
    {
      nsm_encode_link_properties (pnt, size, &msg->l_prop);
    }

  if (NSM_CHECK_CTYPE_FLAG (msg->cindex, NSM_TELINK_CTYPE_ADD) ||
      NSM_CHECK_CTYPE_FLAG (msg->cindex, NSM_TELINK_CTYPE_PHY_PROPERTY) ||
      NSM_CHECK_CTYPE_FLAG (msg->cindex, NSM_TELINK_CTYPE_TE_PROPERTY))
    {
      nsm_encode_phy_property (pnt, size, &msg->p_prop);
    }

  if (NSM_CHECK_CTYPE_FLAG (msg->cindex, NSM_TELINK_CTYPE_ADD) ||
      NSM_CHECK_CTYPE_FLAG (msg->cindex, NSM_TELINK_CTYPE_TE_PROPERTY) ||
      NSM_CHECK_CTYPE_FLAG (msg->cindex, NSM_TELINK_CTYPE_ADMIN_GROUP))
    {
      TLV_ENCODE_PUTL (msg->admin_group);
    }

  if (NSM_CHECK_CTYPE_FLAG (msg->cindex, NSM_TELINK_CTYPE_ADD) ||
      NSM_CHECK_CTYPE_FLAG (msg->cindex, NSM_TELINK_CTYPE_FLAG))
    {
      TLV_ENCODE_PUTL (msg->flags);
    }

  if (NSM_CHECK_CTYPE_FLAG (msg->cindex, NSM_TELINK_CTYPE_ADD) ||
      NSM_CHECK_CTYPE_FLAG (msg->cindex, NSM_TELINK_CTYPE_SRLG_UPD))
    {
      nsm_encode_srlg_property (pnt, size, msg);
    }
  else
    {
      if (NSM_CHECK_CTYPE_FLAG (msg->cindex, NSM_TELINK_CTYPE_SRLG_CLR))
        {
          /* Nothing to do */
        }
    }

  if (NSM_CHECK_CTYPE_FLAG (msg->cindex, NSM_TELINK_CTYPE_ADD) ||
      NSM_CHECK_CTYPE_FLAG (msg->cindex, NSM_TELINK_CTYPE_MTU))
    {
      TLV_ENCODE_PUTL (msg->mtu);
    }

  return *pnt - sp;
}

s_int32_t
nsm_decode_te_link (u_char **pnt, u_int16_t *size,
                    struct nsm_msg_te_link *msg)
{
  u_char *sp = *pnt;

  if (*size < NSM_MSG_TELINK_MIN_SIZE)
    return NSM_ERR_PKT_TOO_SMALL;

  /* Cindex */
  TLV_DECODE_GETL (msg->cindex);

  /* Name */
  TLV_DECODE_GET (msg->name, GINTERFACE_NAMSIZ);

  /* Put gifindex */
  TLV_DECODE_GETL (msg->gifindex);

  if (NSM_CHECK_CTYPE_FLAG (msg->cindex, NSM_TELINK_CTYPE_ADD) ||
      NSM_CHECK_CTYPE_FLAG (msg->cindex, NSM_TELINK_CTYPE_LOCAL_LINKID))
    nsm_decode_link_id (pnt, size, &msg->l_linkid);

  if (NSM_CHECK_CTYPE_FLAG (msg->cindex, NSM_TELINK_CTYPE_ADD) ||
      NSM_CHECK_CTYPE_FLAG (msg->cindex, NSM_TELINK_CTYPE_REMOTE_LINKID))
    nsm_decode_link_id (pnt, size, &msg->r_linkid);

  if (NSM_CHECK_CTYPE_FLAG (msg->cindex, NSM_TELINK_CTYPE_ADD) ||
      NSM_CHECK_CTYPE_FLAG (msg->cindex, NSM_TELINK_CTYPE_STATUS))
    TLV_DECODE_GETC (msg->status);

#ifdef HAVE_DSTE
  if (NSM_CHECK_CTYPE_FLAG (msg->cindex, NSM_TELINK_CTYPE_ADD) ||
      NSM_CHECK_CTYPE_FLAG (msg->cindex, NSM_TELINK_CTYPE_BCMODE))
    TLV_DECODE_GETC (msg->mode);
#endif /* HAVE_DSTE */

  if (NSM_CHECK_CTYPE_FLAG (msg->cindex, NSM_TELINK_CTYPE_ADD) ||
      NSM_CHECK_CTYPE_FLAG (msg->cindex, NSM_TELINK_CTYPE_LINK_ID))
    {
      TLV_DECODE_GET_IN4_ADDR (&msg->linkid);
    }

  if (NSM_CHECK_CTYPE_FLAG (msg->cindex, NSM_TELINK_CTYPE_ADD) ||
      NSM_CHECK_CTYPE_FLAG (msg->cindex, NSM_TELINK_CTYPE_CAGIFINDEX))
    {
      TLV_DECODE_GETL (msg->ca_gifindex);
      TLV_DECODE_GET (msg->caname, GINTERFACE_NAMSIZ);
    }

  if (NSM_CHECK_CTYPE_FLAG (msg->cindex, NSM_TELINK_CTYPE_ADD) ||
      NSM_CHECK_CTYPE_FLAG (msg->cindex, NSM_TELINK_CTYPE_ADDR))
    {
      TLV_DECODE_GETC (msg->addr.family);
#ifdef HAVE_IPV6
      if (msg->addr.family == AF_INET6)
        {
          TLV_DECODE_GET_IN6_ADDR (&msg->addr.u.prefix6);
        }
      else
#endif /* HAVE_IPV6 */
        {
          TLV_DECODE_GET_IN4_ADDR (&msg->addr.u.prefix4);
        }
    }

  if (NSM_CHECK_CTYPE_FLAG (msg->cindex, NSM_TELINK_CTYPE_ADD) ||
      NSM_CHECK_CTYPE_FLAG (msg->cindex, NSM_TELINK_CTYPE_REMOTE_ADDR))
    {
      TLV_DECODE_GETC (msg->remote_addr.family);
#ifdef HAVE_IPV6
      if (msg->remote_addr.family == AF_INET6)
        {
          TLV_DECODE_GET_IN6_ADDR (&msg->remote_addr.u.prefix6);
        }
      else
#endif /* HAVE_IPV6 */
        {
          TLV_DECODE_GET_IN4_ADDR (&msg->remote_addr.u.prefix4);
        }
    }

  if (NSM_CHECK_CTYPE_FLAG (msg->cindex, NSM_TELINK_CTYPE_ADD) ||
      NSM_CHECK_CTYPE_FLAG (msg->cindex, NSM_TELINK_CTYPE_LINK_PROPERTY) ||
      NSM_CHECK_CTYPE_FLAG (msg->cindex, NSM_TELINK_CTYPE_TE_PROPERTY))
    {
      nsm_decode_link_properties (pnt, size, &msg->l_prop);
    }

  if (NSM_CHECK_CTYPE_FLAG (msg->cindex, NSM_TELINK_CTYPE_ADD) ||
      NSM_CHECK_CTYPE_FLAG (msg->cindex, NSM_TELINK_CTYPE_PHY_PROPERTY) ||
      NSM_CHECK_CTYPE_FLAG (msg->cindex, NSM_TELINK_CTYPE_TE_PROPERTY))
    {
      nsm_decode_phy_property (pnt, size, &msg->p_prop);
    }

  if (NSM_CHECK_CTYPE_FLAG (msg->cindex, NSM_TELINK_CTYPE_ADD) ||
      NSM_CHECK_CTYPE_FLAG (msg->cindex, NSM_TELINK_CTYPE_TE_PROPERTY) ||
      NSM_CHECK_CTYPE_FLAG (msg->cindex, NSM_TELINK_CTYPE_ADMIN_GROUP))
    {
      TLV_DECODE_GETL (msg->admin_group);
    }

  if (NSM_CHECK_CTYPE_FLAG (msg->cindex, NSM_TELINK_CTYPE_ADD) ||
      NSM_CHECK_CTYPE_FLAG (msg->cindex, NSM_TELINK_CTYPE_FLAG))
    {
      TLV_DECODE_GETL (msg->flags);
    }

  if (NSM_CHECK_CTYPE_FLAG (msg->cindex, NSM_TELINK_CTYPE_ADD) ||
      NSM_CHECK_CTYPE_FLAG (msg->cindex, NSM_TELINK_CTYPE_SRLG_UPD))
    {
      nsm_decode_srlg_property (pnt, size, msg);
    }
  else
    {
      if (NSM_CHECK_CTYPE_FLAG (msg->cindex, NSM_TELINK_CTYPE_SRLG_CLR))
        {
          /* Nothing to do */
        }
    }

  if (NSM_CHECK_CTYPE_FLAG (msg->cindex, NSM_TELINK_CTYPE_ADD) ||
      NSM_CHECK_CTYPE_FLAG (msg->cindex, NSM_TELINK_CTYPE_MTU))
    {
      TLV_DECODE_GETL (msg->mtu);
    }

  return *pnt - sp;
}

s_int32_t
nsm_encode_data_link_sub (u_char **pnt, u_int16_t *size,
                          struct nsm_msg_data_link_sub *msg)
{
  u_char *sp = *pnt;

  /* Cindex */
  TLV_ENCODE_PUTL (msg->cindex);

  /* Name */
  TLV_ENCODE_PUT (msg->name, GINTERFACE_NAMSIZ);

  /* Put gifindex */
  TLV_ENCODE_PUTL (msg->gifindex);
  TLV_ENCODE_PUTL (msg->ifindex);

  if (NSM_CHECK_CTYPE_FLAG (msg->cindex, NSM_DATALINK_CTYPE_ADD) ||
      NSM_CHECK_CTYPE_FLAG (msg->cindex, NSM_DATALINK_CTYPE_LOCAL_LINKID))
    {
      nsm_encode_link_id (pnt, size, &msg->l_linkid);
    }

  if (NSM_CHECK_CTYPE_FLAG (msg->cindex, NSM_DATALINK_CTYPE_ADD) ||
      NSM_CHECK_CTYPE_FLAG (msg->cindex, NSM_DATALINK_CTYPE_REMOTE_LINKID))
    {
      nsm_encode_link_id (pnt, size, &msg->r_linkid);
    }

  if (NSM_CHECK_CTYPE_FLAG (msg->cindex, NSM_DATALINK_CTYPE_ADD) ||
      NSM_CHECK_CTYPE_FLAG (msg->cindex, NSM_DATALINK_CTYPE_TE_GIFINDEX))
    {
      TLV_ENCODE_PUTL (msg->te_gifindex);
      TLV_ENCODE_PUT (msg->tlname, GINTERFACE_NAMSIZ);
    }

  if (NSM_CHECK_CTYPE_FLAG (msg->cindex, NSM_DATALINK_CTYPE_ADD) ||
      NSM_CHECK_CTYPE_FLAG (msg->cindex, NSM_DATALINK_CTYPE_STATUS))
    {
      TLV_ENCODE_PUTC (msg->status);
      TLV_ENCODE_PUTW (msg->flags);
    }

  return *pnt - sp;
}

/* Dump NSM Data Link Sub Message */
void
nsm_data_link_sub_dump (struct lib_globals *zg,
                      struct nsm_msg_data_link_sub *msg)
{
  zlog_info (zg, " NSM Data Link (Sub)");
  zlog_info (zg, " Name: %s", msg->name);
  zlog_info (zg, " ifindex: %d", msg->ifindex);
  zlog_info (zg, " gifindex: %d", msg->gifindex);

  if ((NSM_CHECK_CTYPE_FLAG (msg->cindex, NSM_DATALINK_CTYPE_LOCAL_LINKID) ||
      NSM_CHECK_CTYPE_FLAG (msg->cindex, NSM_DATALINK_CTYPE_ADD)) &&
      (msg->l_linkid.type == GMPLS_LINK_ID_TYPE_IPV4))
    zlog_info (zg, " Local Link ID: %r", &msg->l_linkid.linkid.ipv4.s_addr);

  if ((NSM_CHECK_CTYPE_FLAG (msg->cindex, NSM_DATALINK_CTYPE_REMOTE_LINKID) ||
      NSM_CHECK_CTYPE_FLAG (msg->cindex, NSM_DATALINK_CTYPE_ADD)) &&
      (msg->r_linkid.type == GMPLS_LINK_ID_TYPE_IPV4))
    zlog_info (zg, " Remote Link ID: %r", &msg->r_linkid.linkid.ipv4.s_addr);

  if (NSM_CHECK_CTYPE_FLAG (msg->cindex, NSM_DATALINK_CTYPE_TE_GIFINDEX) ||
      NSM_CHECK_CTYPE_FLAG (msg->cindex, NSM_DATALINK_CTYPE_ADD))
    {
      zlog_info (zg, " TE Link name: %s", msg->tlname);
      zlog_info (zg, " TE Link gifindex: %s", msg->te_gifindex);
    }
}

s_int32_t
nsm_decode_data_link_sub (u_char **pnt, u_int16_t *size,
                          struct nsm_msg_data_link_sub *msg)
{
  u_char *sp = *pnt;

  if (*size < NSM_MSG_DATA_LINK_MIN_SIZE)
    return NSM_ERR_PKT_TOO_SMALL;

  pal_mem_set (msg, '\0', sizeof (struct nsm_msg_data_link_sub));

  /* Cindex */
  TLV_DECODE_GETL (msg->cindex);

  /* Name */
  TLV_DECODE_GET (msg->name, GINTERFACE_NAMSIZ);

  /* Put gifindex */
  TLV_DECODE_GETL (msg->gifindex);
  TLV_DECODE_GETL (msg->ifindex);

  if (NSM_CHECK_CTYPE_FLAG (msg->cindex, NSM_DATALINK_CTYPE_ADD) ||
      NSM_CHECK_CTYPE_FLAG (msg->cindex, NSM_DATALINK_CTYPE_LOCAL_LINKID))
    {
      nsm_decode_link_id (pnt, size, &msg->l_linkid);
    }

  if (NSM_CHECK_CTYPE_FLAG (msg->cindex, NSM_DATALINK_CTYPE_ADD) ||
      NSM_CHECK_CTYPE_FLAG (msg->cindex, NSM_DATALINK_CTYPE_REMOTE_LINKID))
    {
      nsm_decode_link_id (pnt, size, &msg->r_linkid);
    }

  if (NSM_CHECK_CTYPE_FLAG (msg->cindex, NSM_DATALINK_CTYPE_ADD) ||
      NSM_CHECK_CTYPE_FLAG (msg->cindex, NSM_DATALINK_CTYPE_TE_GIFINDEX))
    {
      TLV_DECODE_GETL (msg->te_gifindex);
      TLV_DECODE_GET (msg->tlname, GINTERFACE_NAMSIZ);
    }

  if (NSM_CHECK_CTYPE_FLAG (msg->cindex, NSM_DATALINK_CTYPE_ADD) ||
      NSM_CHECK_CTYPE_FLAG (msg->cindex, NSM_DATALINK_CTYPE_STATUS))
    {
      TLV_DECODE_GETC (msg->status);
      TLV_DECODE_GETW (msg->flags);
    }

  return *pnt - sp;
}

/* Dump NSM Data Link Message */
void
nsm_data_link_dump (struct lib_globals *zg,
                      struct nsm_msg_data_link *msg)
{
  zlog_info (zg, " NSM Data Link");
  zlog_info (zg, " Name: %s", msg->name);
  zlog_info (zg, " ifindex: %d", msg->ifindex);
  zlog_info (zg, " gifindex: %d", msg->gifindex);

  if ((NSM_CHECK_CTYPE_FLAG (msg->cindex, NSM_DATALINK_CTYPE_LOCAL_LINKID) ||
      NSM_CHECK_CTYPE_FLAG (msg->cindex, NSM_DATALINK_CTYPE_ADD)) &&
      (msg->l_linkid.type == GMPLS_LINK_ID_TYPE_IPV4))
    zlog_info (zg, " Local Link ID: %r", &msg->l_linkid.linkid.ipv4.s_addr);

  if ((NSM_CHECK_CTYPE_FLAG (msg->cindex, NSM_DATALINK_CTYPE_REMOTE_LINKID) ||
      NSM_CHECK_CTYPE_FLAG (msg->cindex, NSM_DATALINK_CTYPE_ADD)) &&
       (msg->r_linkid.type == GMPLS_LINK_ID_TYPE_IPV4))
    zlog_info (zg, " Remote Link ID: %r", &msg->r_linkid.linkid.ipv4.s_addr);

  if (NSM_CHECK_CTYPE_FLAG (msg->cindex, NSM_DATALINK_CTYPE_TE_GIFINDEX) ||
      NSM_CHECK_CTYPE_FLAG (msg->cindex, NSM_DATALINK_CTYPE_ADD))
    {
      zlog_info (zg, " TE Link name: %s", msg->tlname);
      zlog_info (zg, " TE Link gifindex: %d", msg->te_gifindex);
    }

  if (NSM_CHECK_CTYPE_FLAG (msg->cindex, NSM_TELINK_CTYPE_LINK_PROPERTY) ||
      NSM_CHECK_CTYPE_FLAG (msg->cindex, NSM_TELINK_CTYPE_ADD))
    {
      zlog_info (zg, " Link Properties");
      zlog_info (zg, " Bandwidth: %f", msg->prop.bandwidth);
      zlog_info (zg, " Max Reservable Bandwidth: %f", msg->prop.max_resv_bw);
    }
}

s_int32_t
nsm_encode_data_link (u_char **pnt, u_int16_t *size,
                      struct nsm_msg_data_link *msg)
{
  u_char *sp = *pnt;

  /* Cindex */
  TLV_ENCODE_PUTL (msg->cindex);

  /* Name */
  TLV_ENCODE_PUT (msg->name, GINTERFACE_NAMSIZ);

  /* Put gifindex */
  TLV_ENCODE_PUTL (msg->gifindex);
  TLV_ENCODE_PUTL (msg->ifindex);

  if (NSM_CHECK_CTYPE_FLAG (msg->cindex, NSM_DATALINK_CTYPE_ADD) ||
      NSM_CHECK_CTYPE_FLAG (msg->cindex, NSM_DATALINK_CTYPE_LOCAL_LINKID))
    {
      nsm_encode_link_id (pnt, size, &msg->l_linkid);
    }

  if (NSM_CHECK_CTYPE_FLAG (msg->cindex, NSM_DATALINK_CTYPE_ADD) ||
      NSM_CHECK_CTYPE_FLAG (msg->cindex, NSM_DATALINK_CTYPE_REMOTE_LINKID))
    {
      nsm_encode_link_id (pnt, size, &msg->r_linkid);
    }

  if (NSM_CHECK_CTYPE_FLAG (msg->cindex, NSM_DATALINK_CTYPE_ADD) ||
      NSM_CHECK_CTYPE_FLAG (msg->cindex, NSM_DATALINK_CTYPE_TE_GIFINDEX))
    {
      TLV_ENCODE_PUTL (msg->te_gifindex);
      TLV_ENCODE_PUT (msg->tlname, GINTERFACE_NAMSIZ);
    }

  if (NSM_CHECK_CTYPE_FLAG (msg->cindex, NSM_DATALINK_CTYPE_ADD) ||
      NSM_CHECK_CTYPE_FLAG (msg->cindex, NSM_DATALINK_CTYPE_STATUS))
    {
      TLV_ENCODE_PUTC (msg->status);
      TLV_ENCODE_PUTW (msg->flags);
    }

  if (NSM_CHECK_CTYPE_FLAG (msg->cindex, NSM_DATALINK_CTYPE_ADD) ||
      NSM_CHECK_CTYPE_FLAG (msg->cindex, NSM_DATALINK_CTYPE_PROPERTY))
    {
      nsm_encode_link_properties (pnt, size, &msg->prop);
    }

  if (NSM_CHECK_CTYPE_FLAG (msg->cindex, NSM_DATALINK_CTYPE_ADD) ||
      NSM_CHECK_CTYPE_FLAG (msg->cindex, NSM_DATALINK_CTYPE_PHY_PROPERTY))
    {
      nsm_encode_phy_property (pnt, size, &msg->p_prop);
    }
  return *pnt - sp;
}

s_int32_t
nsm_decode_data_link (u_char **pnt, u_int16_t *size,
                      struct nsm_msg_data_link *msg)
{
  u_char *sp = *pnt;

  if (*size < NSM_MSG_DATA_LINK_MIN_SIZE)
    return NSM_ERR_PKT_TOO_SMALL;

  pal_mem_set (msg, '\0', sizeof (struct nsm_msg_data_link));
  TLV_DECODE_GETL (msg->cindex);

  /* Name */
  TLV_DECODE_GET (msg->name, GINTERFACE_NAMSIZ);

  /* Put gifindex */
  TLV_DECODE_GETL (msg->gifindex);
  TLV_DECODE_GETL (msg->ifindex);

  if (NSM_CHECK_CTYPE_FLAG (msg->cindex, NSM_DATALINK_CTYPE_ADD) ||
      NSM_CHECK_CTYPE_FLAG (msg->cindex, NSM_DATALINK_CTYPE_LOCAL_LINKID))
    {
      nsm_decode_link_id (pnt, size, &msg->l_linkid);
    }

  if (NSM_CHECK_CTYPE_FLAG (msg->cindex, NSM_DATALINK_CTYPE_ADD) ||
      NSM_CHECK_CTYPE_FLAG (msg->cindex, NSM_DATALINK_CTYPE_REMOTE_LINKID))
    {
      nsm_decode_link_id (pnt, size, &msg->r_linkid);
    }

  if (NSM_CHECK_CTYPE_FLAG (msg->cindex, NSM_DATALINK_CTYPE_ADD) ||
      NSM_CHECK_CTYPE_FLAG (msg->cindex, NSM_DATALINK_CTYPE_TE_GIFINDEX))
    {
      TLV_DECODE_GETL (msg->te_gifindex);
      TLV_DECODE_GET (msg->tlname, GINTERFACE_NAMSIZ);
    }

  if (NSM_CHECK_CTYPE_FLAG (msg->cindex, NSM_DATALINK_CTYPE_ADD) ||
      NSM_CHECK_CTYPE_FLAG (msg->cindex, NSM_DATALINK_CTYPE_STATUS))
    {
      TLV_DECODE_GETC (msg->status);
      TLV_DECODE_GETW (msg->flags);
    }

  if (NSM_CHECK_CTYPE_FLAG (msg->cindex, NSM_DATALINK_CTYPE_ADD) ||
      NSM_CHECK_CTYPE_FLAG (msg->cindex, NSM_DATALINK_CTYPE_PROPERTY))
    {
      nsm_decode_link_properties (pnt, size, &msg->prop);
    }

  if (NSM_CHECK_CTYPE_FLAG (msg->cindex, NSM_DATALINK_CTYPE_ADD) ||
      NSM_CHECK_CTYPE_FLAG (msg->cindex, NSM_DATALINK_CTYPE_PHY_PROPERTY))
    {
      nsm_decode_phy_property (pnt, size, &msg->p_prop);
    }
  return *pnt - sp;
}

s_int32_t
nsm_parse_data_link (u_char **pnt, u_int16_t *size,
                     struct nsm_msg_header *header, void *arg,
                     NSM_CALLBACK callback)
{
  int ret = 0;
  struct nsm_msg_data_link msg;
  int nbytes;
  u_int16_t length = *size;

  while (length)
    {
      pal_mem_set (&msg, 0, sizeof (struct nsm_msg_data_link));

      nbytes = nsm_decode_data_link (pnt, size, &msg);
      if (nbytes < 0)
        return nbytes;

      if (callback)
        {
          ret = (*callback) (header, arg, &msg);
          if (ret < 0)
            return ret;
        }

      if (nbytes > length)
        break;

      length -= nbytes;
    }

  return 0;
}

s_int32_t
nsm_parse_data_link_sub (u_char **pnt, u_int16_t *size,
                         struct nsm_msg_header *header, void *arg,
                         NSM_CALLBACK callback)
{
  int ret = 0;
  struct nsm_msg_data_link_sub msg;
  int nbytes;
  u_int16_t length = *size;

  while (length)
    {
      pal_mem_set (&msg, 0, sizeof (struct nsm_msg_data_link_sub));

      nbytes = nsm_decode_data_link_sub (pnt, size, &msg);
      if (nbytes < 0)
        return nbytes;

      if (callback)
        {
          ret = (*callback) (header, arg, &msg);
          if (ret < 0)
            return ret;
        }

      if (nbytes > length)
        break;

      length -= nbytes;
    }

  return 0;
}

/* Dump NSM TE Link Message */
void
nsm_te_link_dump (struct lib_globals *zg,
                  struct nsm_msg_te_link *msg)
{
  if (NSM_CHECK_CTYPE_FLAG (msg->cindex, NSM_TELINK_CTYPE_ADD))
    zlog_info (zg, " NSM TE Link Add");
  else if (NSM_CHECK_CTYPE_FLAG (msg->cindex, NSM_TELINK_CTYPE_DEL))
    zlog_info (zg, " NSM TE Link Delete");
  else
    zlog_info (zg, " NSM TE Link");

  zlog_info (zg, " Name: %s", msg->name);
  zlog_info (zg, " gifindex: %d", msg->gifindex);

  if ((NSM_CHECK_CTYPE_FLAG (msg->cindex, NSM_TELINK_CTYPE_LOCAL_LINKID) ||
      NSM_CHECK_CTYPE_FLAG (msg->cindex, NSM_TELINK_CTYPE_ADD)) &&
      (msg->l_linkid.type == GMPLS_LINK_ID_TYPE_IPV4))
    zlog_info (zg, " Local Link ID: %r", &msg->l_linkid.linkid.ipv4.s_addr);

  if ((NSM_CHECK_CTYPE_FLAG (msg->cindex, NSM_TELINK_CTYPE_REMOTE_LINKID) ||
      NSM_CHECK_CTYPE_FLAG (msg->cindex, NSM_TELINK_CTYPE_ADD)) &&
      (msg->r_linkid.type == GMPLS_LINK_ID_TYPE_IPV4))
    zlog_info (zg, " Remote Link ID: %r", &msg->r_linkid.linkid.ipv4.s_addr);

  if (NSM_CHECK_CTYPE_FLAG (msg->cindex, NSM_TELINK_CTYPE_CAGIFINDEX) ||
      NSM_CHECK_CTYPE_FLAG (msg->cindex, NSM_TELINK_CTYPE_ADD))
    {
      zlog_info (zg, " Control Adjacency name: %s", msg->caname);
      zlog_info (zg, " Control Adjacency gifindex: %d", msg->ca_gifindex);
    }

#ifdef HAVE_DSTE
  if (NSM_CHECK_CTYPE_FLAG (msg->cindex, NSM_TELINK_CTYPE_BCMODE) ||
      NSM_CHECK_CTYPE_FLAG (msg->cindex, NSM_TELINK_CTYPE_ADD))
    {
      zlog_info (zg, " TE link Status : %s", gmpls_te_link_bcmode (msg->mode));
    }
#endif /* HAVE_DSTE */

  if (NSM_CHECK_CTYPE_FLAG (msg->cindex, NSM_TELINK_CTYPE_STATUS) ||
      NSM_CHECK_CTYPE_FLAG (msg->cindex, NSM_TELINK_CTYPE_ADD))
    {
      zlog_info (zg, " TE link mode: %s", gmpls_te_link_status (msg->status));
    }

  if (NSM_CHECK_CTYPE_FLAG (msg->cindex, NSM_TELINK_CTYPE_LINK_PROPERTY) ||
      NSM_CHECK_CTYPE_FLAG (msg->cindex, NSM_TELINK_CTYPE_ADD))
    {
      zlog_info (zg, " Link Properties");
      zlog_info (zg, " Bandwidth: %f", msg->l_prop.bandwidth);
      zlog_info (zg, " Max Reservable Bandwidth: %f", msg->l_prop.max_resv_bw);
    }

  if (NSM_CHECK_CTYPE_FLAG (msg->cindex, NSM_TELINK_CTYPE_PHY_PROPERTY) ||
      NSM_CHECK_CTYPE_FLAG (msg->cindex, NSM_TELINK_CTYPE_ADD))
    {
      zlog_info (zg, " Physical Properties");
      zlog_info (zg, " Protection Type : %s",
                      gmpls_protection_type_str (msg->p_prop.protection));
      zlog_info (zg, " Switching Capability Type : %s",
                      gmpls_capability_type_str (msg->p_prop.switching_cap));
      zlog_info (zg, " Encoding Type : %s",
                     gmpls_encoding_type_str (msg->p_prop.switching_cap));
      zlog_info (zg, " Minimum LSP bandwidth : %f", msg->p_prop.min_lsp_bw);
    }

  if (NSM_CHECK_CTYPE_FLAG (msg->cindex, NSM_TELINK_CTYPE_MTU) ||
      NSM_CHECK_CTYPE_FLAG (msg->cindex, NSM_TELINK_CTYPE_ADD))
    {
      zlog_info (zg, " TE Link MTU: %d", msg->mtu);
    }
}

s_int32_t
nsm_parse_te_link (u_char **pnt, u_int16_t *size,
                   struct nsm_msg_header *header, void *arg,
                   NSM_CALLBACK callback)
{
  int ret = 0;
  struct nsm_msg_te_link msg;
  int nbytes;
  u_int16_t length = *size;

  while (length)
    {
      pal_mem_set (&msg, 0, sizeof (struct nsm_msg_te_link));

      nbytes = nsm_decode_te_link (pnt, size, &msg);
      if (nbytes < 0)
        {
          if (msg.srlg)
            XFREE (MTYPE_GMPLS_SRLG, msg.srlg);

          return nbytes;
        }

      if (callback)
        {
          ret = (*callback) (header, arg, &msg);
          if (ret < 0)
            {
              if (msg.srlg)
                XFREE (MTYPE_GMPLS_SRLG, msg.srlg);

              return ret;
            }
        }

      if (nbytes > length)
        break;

      length -= nbytes;
    }

  if (msg.srlg)
   XFREE (MTYPE_GMPLS_SRLG, msg.srlg);

  return 0;
}

s_int32_t
nsm_encode_control_channel (u_char **pnt, u_int16_t *size,
                            struct nsm_msg_control_channel *msg)
{
  u_char *sp = *pnt;

  /* Cindex */
  TLV_ENCODE_PUTL (msg->cindex);

  /* Name */
  TLV_ENCODE_PUT (msg->name, GINTERFACE_NAMSIZ);

  TLV_ENCODE_PUTL (msg->gifindex);

  if (NSM_CHECK_CTYPE (msg->cindex, NSM_CONTROL_CHANNEL_CTYPE_BINDING) ||
      NSM_CHECK_CTYPE (msg->cindex, NSM_CONTROL_CHANNEL_CTYPE_ADD))
    TLV_ENCODE_PUTL (msg->bifindex);

  TLV_ENCODE_PUTL (msg->ccid);

  if (NSM_CHECK_CTYPE (msg->cindex, NSM_CONTROL_CHANNEL_CTYPE_LOCAL_ADDR) ||
      NSM_CHECK_CTYPE (msg->cindex, NSM_CONTROL_CHANNEL_CTYPE_ADD))
    TLV_ENCODE_PUT_IN4_ADDR (&msg->l_addr);

  if (NSM_CHECK_CTYPE (msg->cindex, NSM_CONTROL_CHANNEL_CTYPE_REMOTE_ADDR) ||
      NSM_CHECK_CTYPE (msg->cindex, NSM_CONTROL_CHANNEL_CTYPE_ADD))
    TLV_ENCODE_PUT_IN4_ADDR (&msg->r_addr);

  if (NSM_CHECK_CTYPE (msg->cindex, NSM_CONTROL_CHANNEL_CTYPE_STATUS) ||
      NSM_CHECK_CTYPE (msg->cindex, NSM_CONTROL_CHANNEL_CTYPE_ADD))
    TLV_ENCODE_PUTC (msg->status);

  if (NSM_CHECK_CTYPE (msg->cindex, NSM_CONTROL_CHANNEL_CTYPE_CAGIFINDEX) ||
      NSM_CHECK_CTYPE (msg->cindex, NSM_CONTROL_CHANNEL_CTYPE_ADD))
    {
      TLV_ENCODE_PUTC (msg->primary);
      TLV_ENCODE_PUT (msg->caname, GINTERFACE_NAMSIZ);
      TLV_ENCODE_PUTL (msg->cagifindex);
    }

  return *pnt - sp;
}

s_int32_t
nsm_decode_control_channel (u_char **pnt, u_int16_t *size,
                            struct nsm_msg_control_channel *msg)
{
  u_char *sp = *pnt;

  /* Cindex */
  TLV_DECODE_GETL (msg->cindex);

  /* Name */
  TLV_DECODE_GET (msg->name, GINTERFACE_NAMSIZ);

  TLV_DECODE_GETL (msg->gifindex);

  if (NSM_CHECK_CTYPE (msg->cindex, NSM_CONTROL_CHANNEL_CTYPE_BINDING) ||
      NSM_CHECK_CTYPE (msg->cindex, NSM_CONTROL_CHANNEL_CTYPE_ADD))
    TLV_DECODE_GETL (msg->bifindex);

  TLV_DECODE_GETL (msg->ccid);

  if (NSM_CHECK_CTYPE (msg->cindex, NSM_CONTROL_CHANNEL_CTYPE_LOCAL_ADDR) ||
      NSM_CHECK_CTYPE (msg->cindex, NSM_CONTROL_CHANNEL_CTYPE_ADD))
    TLV_DECODE_GET_IN4_ADDR (&msg->l_addr);

  if (NSM_CHECK_CTYPE (msg->cindex, NSM_CONTROL_CHANNEL_CTYPE_REMOTE_ADDR) ||
      NSM_CHECK_CTYPE (msg->cindex, NSM_CONTROL_CHANNEL_CTYPE_ADD))
    TLV_DECODE_GET_IN4_ADDR (&msg->r_addr);

  if (NSM_CHECK_CTYPE (msg->cindex, NSM_CONTROL_CHANNEL_CTYPE_STATUS) ||
      NSM_CHECK_CTYPE (msg->cindex, NSM_CONTROL_CHANNEL_CTYPE_ADD))
    TLV_DECODE_GETC (msg->status);

  if (NSM_CHECK_CTYPE (msg->cindex, NSM_CONTROL_CHANNEL_CTYPE_CAGIFINDEX) ||
      NSM_CHECK_CTYPE (msg->cindex, NSM_CONTROL_CHANNEL_CTYPE_ADD))
    {
      TLV_DECODE_GETC (msg->primary);
      TLV_DECODE_GET (msg->caname, GINTERFACE_NAMSIZ);
      TLV_DECODE_GETL (msg->cagifindex);
    }

  return *pnt - sp;
}

void
nsm_control_channel_dump (struct lib_globals *zg,
                          struct nsm_msg_control_channel *msg)
{
  int add = 0;
  int del = 0;

  if (NSM_CHECK_CTYPE (msg->cindex, NSM_CONTROL_CHANNEL_CTYPE_ADD))
    add = 1;
  if (NSM_CHECK_CTYPE (msg->cindex, NSM_CONTROL_CHANNEL_CTYPE_DEL))
    del = 1;

  zlog_info (zg, "NSM Control Channel %s",
             add ? "Add" : (del ? "Delete" : "Update"));
  zlog_info (zg, " Name: %s", msg->name);
  zlog_info (zg, " gifindex: %d", msg->gifindex);

  if (NSM_CHECK_CTYPE (msg->cindex, NSM_CONTROL_CHANNEL_CTYPE_BINDING) ||
      NSM_CHECK_CTYPE (msg->cindex, NSM_CONTROL_CHANNEL_CTYPE_ADD))
    zlog_info (zg, " Binding interface ifindex: %d", msg->bifindex);

  zlog_info (zg, " ccid: %d", msg->ccid);

  if (NSM_CHECK_CTYPE (msg->cindex, NSM_CONTROL_CHANNEL_CTYPE_LOCAL_ADDR) ||
      NSM_CHECK_CTYPE (msg->cindex, NSM_CONTROL_CHANNEL_CTYPE_ADD))
    zlog_info (zg, " Local address: %r", &msg->l_addr);

  if (NSM_CHECK_CTYPE (msg->cindex, NSM_CONTROL_CHANNEL_CTYPE_REMOTE_ADDR) ||
      NSM_CHECK_CTYPE (msg->cindex, NSM_CONTROL_CHANNEL_CTYPE_ADD))
    zlog_info (zg, " Remote address: %r", &msg->r_addr);

  if (NSM_CHECK_CTYPE (msg->cindex, NSM_CONTROL_CHANNEL_CTYPE_STATUS) ||
      NSM_CHECK_CTYPE (msg->cindex, NSM_CONTROL_CHANNEL_CTYPE_ADD))
    zlog_info (zg, " Status: 0x%08x", msg->status);

  if (NSM_CHECK_CTYPE (msg->cindex, NSM_CONTROL_CHANNEL_CTYPE_CAGIFINDEX) ||
      NSM_CHECK_CTYPE (msg->cindex, NSM_CONTROL_CHANNEL_CTYPE_ADD))
    {
      zlog_info (zg, " Primary: %s", msg->primary? "TRUE" : "FALSE");
      zlog_info (zg, " CA name: %s", msg->caname);
      zlog_info (zg, " CA gifindex: %d", msg->cagifindex);
    }
}

/* Parse NSM GMPLS Control Channel message */
s_int32_t
nsm_parse_control_channel (u_char **pnt, u_int16_t *size,
                           struct nsm_msg_header *header, void *arg,
                           NSM_CALLBACK callback)
{
  int ret = 0;
  struct nsm_msg_control_channel msg;
  int nbytes;
  u_int16_t length = *size;

  while (length)
    {
      pal_mem_set (&msg, 0, sizeof (struct nsm_msg_control_channel));

      nbytes = nsm_decode_control_channel (pnt, size, &msg);
      if (nbytes < 0)
        return nbytes;

      if (callback)
        {
          ret = (*callback) (header, arg, &msg);
          if (ret < 0)
            return ret;
        }

      if (nbytes > length)
        break;

      length -= nbytes;
    }

  return 0;
}

s_int32_t
nsm_encode_control_adj (u_char **pnt, u_int16_t *size,
                        struct nsm_msg_control_adj *msg)
{
  u_char *sp = *pnt;

  /* Cindex */
  TLV_ENCODE_PUTL (msg->cindex);

  /* Name */
  TLV_ENCODE_PUT (msg->name, GINTERFACE_NAMSIZ);

  TLV_ENCODE_PUTL (msg->gifindex);
  TLV_ENCODE_PUTC (msg->flags);

  if (NSM_CHECK_CTYPE (msg->cindex, NSM_CONTROL_ADJ_CTYPE_REMOTE_NODEID) ||
      NSM_CHECK_CTYPE (msg->cindex, NSM_CONTROL_ADJ_CTYPE_ADD))
    TLV_ENCODE_PUT_IN4_ADDR (&msg->r_nodeid);

  if (NSM_CHECK_CTYPE (msg->cindex, NSM_CONTROL_ADJ_CTYPE_LMP_INUSE) ||
      NSM_CHECK_CTYPE (msg->cindex, NSM_CONTROL_ADJ_CTYPE_ADD))
    TLV_ENCODE_PUTC (msg->lmp_inuse);

  TLV_ENCODE_PUTL (msg->ccupcntrs);
  if (! NSM_CHECK_CTYPE (msg->cindex, NSM_CONTROL_ADJ_CTYPE_USE_CC))
    TLV_ENCODE_PUTL (msg->ccgifindex);

  if (NSM_CHECK_CTYPE (msg->cindex, NSM_CONTROL_ADJ_CTYPE_USE_CC))
    {
      TLV_ENCODE_PUT (msg->ucc.name, GINTERFACE_NAMSIZ);
      TLV_ENCODE_PUTL (msg->ucc.gifindex);
      TLV_ENCODE_PUTL (msg->ucc.ccid);
      TLV_ENCODE_PUTL (msg->ucc.bifindex);
      TLV_ENCODE_PUT_IN4_ADDR (&msg->ucc.l_addr);
      TLV_ENCODE_PUT_IN4_ADDR (&msg->ucc.r_addr);
    }

  return *pnt - sp;
}

s_int32_t
nsm_decode_control_adj (u_char **pnt, u_int16_t *size,
                        struct nsm_msg_control_adj *msg)
{
  u_char *sp = *pnt;

  /* Cindex */
  TLV_DECODE_GETL (msg->cindex);

  /* Name */
  TLV_DECODE_GET (msg->name, GINTERFACE_NAMSIZ);

  TLV_DECODE_GETL (msg->gifindex);
  TLV_DECODE_GETC (msg->flags);

  if (NSM_CHECK_CTYPE (msg->cindex, NSM_CONTROL_ADJ_CTYPE_REMOTE_NODEID) ||
      NSM_CHECK_CTYPE (msg->cindex, NSM_CONTROL_ADJ_CTYPE_ADD))
    TLV_DECODE_GET_IN4_ADDR (&msg->r_nodeid);

  if (NSM_CHECK_CTYPE (msg->cindex, NSM_CONTROL_ADJ_CTYPE_LMP_INUSE) ||
      NSM_CHECK_CTYPE (msg->cindex, NSM_CONTROL_ADJ_CTYPE_ADD))
    TLV_DECODE_GETC (msg->lmp_inuse);

  TLV_DECODE_GETL (msg->ccupcntrs);
  if (! NSM_CHECK_CTYPE (msg->cindex, NSM_CONTROL_ADJ_CTYPE_USE_CC))
    TLV_DECODE_GETL (msg->ccgifindex);

  if (NSM_CHECK_CTYPE (msg->cindex, NSM_CONTROL_ADJ_CTYPE_USE_CC))
    {
      TLV_DECODE_GET (msg->ucc.name, GINTERFACE_NAMSIZ);
      TLV_DECODE_GETL (msg->ucc.gifindex);
      TLV_DECODE_GETL (msg->ucc.ccid);
      TLV_DECODE_GETL (msg->ucc.bifindex);
      TLV_DECODE_GET_IN4_ADDR (&msg->ucc.l_addr);
      TLV_DECODE_GET_IN4_ADDR (&msg->ucc.r_addr);
    }

  return *pnt - sp;
}

void
nsm_control_adj_dump (struct lib_globals *zg,
                      struct nsm_msg_control_adj *msg)
{
  int add = 0;
  int del = 0;

  if (NSM_CHECK_CTYPE (msg->cindex, NSM_CONTROL_ADJ_CTYPE_ADD))
    add = 1;
  if (NSM_CHECK_CTYPE (msg->cindex, NSM_CONTROL_ADJ_CTYPE_DEL))
    del = 1;

  zlog_info (zg, "NSM Control Adjacency %s",
             add ? "Add" : (del ? "Delete" : "Update"));
  zlog_info (zg, " Name: %s", msg->name);
  zlog_info (zg, " gifindex: %d", msg->gifindex);

  if (NSM_CHECK_CTYPE (msg->cindex, NSM_CONTROL_ADJ_CTYPE_REMOTE_NODEID) ||
      NSM_CHECK_CTYPE (msg->cindex, NSM_CONTROL_ADJ_CTYPE_ADD))
    zlog_info (zg, " Remote Node ID: %r", &msg->r_nodeid);

  if (NSM_CHECK_CTYPE (msg->cindex, NSM_CONTROL_ADJ_CTYPE_LMP_INUSE) ||
      NSM_CHECK_CTYPE (msg->cindex, NSM_CONTROL_ADJ_CTYPE_ADD))
    zlog_info (zg, " LMP in-use: %s", msg->lmp_inuse? "TRUE" : "FALSE");

  zlog_info (zg, " UP Control Channel count: %d", msg->ccupcntrs);

  if (! NSM_CHECK_CTYPE (msg->cindex, NSM_CONTROL_ADJ_CTYPE_USE_CC))
    zlog_info (zg, " Control Channel in-use gifindex: %d", msg->ccgifindex);

  if (NSM_CHECK_CTYPE (msg->cindex, NSM_CONTROL_ADJ_CTYPE_USE_CC))
    {
      zlog_info (zg, " Control Channel in-use name: %s", msg->ucc.name);
      zlog_info (zg, " Control Channel in-use gifindex: %d", msg->ucc.gifindex);
      zlog_info (zg, " Control Channel in-use ccid: %d", msg->ucc.ccid);
      zlog_info (zg, " Control Channel in-use local addr: %r",
                 &msg->ucc.l_addr);
      zlog_info (zg, " Control Channel in-use remote addr: %r",
                 &msg->ucc.r_addr);

    }
}

/* Parse NSM GMPLS Control Adjacency message */
s_int32_t
nsm_parse_control_adj (u_char **pnt, u_int16_t *size,
                       struct nsm_msg_header *header, void *arg,
                       NSM_CALLBACK callback)
{
  int ret = 0;
  struct nsm_msg_control_adj msg;
  int nbytes;
  u_int16_t length = *size;

  while (length)
    {
      pal_mem_set (&msg, 0, sizeof (struct nsm_msg_control_adj));

      nbytes = nsm_decode_control_adj (pnt, size, &msg);
      if (nbytes < 0)
        return nbytes;

      if (callback)
        {
          ret = (*callback) (header, arg, &msg);
          if (ret < 0)
            return ret;
        }

      if (nbytes > length)
        break;

      length -= nbytes;
    }

  return 0;
}

#ifdef HAVE_LMP
int
nsm_encode_dlink_opaque (u_char **pnt, u_int16_t *size,
                         struct nsm_msg_dlink_opaque *msg)
{
  u_char *sp = *pnt;

  /* Encode name of the data-link or name of the dlink-interface */
  TLV_ENCODE_PUT(msg->name, GINTERFACE_NAMSIZ);

  /* Encode gifindex of the dlink */
  TLV_ENCODE_PUTL(msg->gifindex);

  /* Encode flags (OPAQUE or not) of the dlink */
  TLV_ENCODE_PUTC(msg->flags);

  return *pnt - sp;
}

int
nsm_decode_dlink_opaque (u_char **pnt, u_int16_t *size,
                         struct nsm_msg_dlink_opaque *msg)
{
  u_char *sp = *pnt;

  /* Decode name of the data-link or name of the dlink-interface */
  TLV_DECODE_GET(msg->name, GINTERFACE_NAMSIZ);

  /* Decode gifindex of the dlink */
  TLV_DECODE_GETL(msg->gifindex);

  /* Decode flags (OPAQUE or not) of the dlink */
  TLV_DECODE_GETC(msg->flags);

  return *pnt - sp;
}

int
nsm_encode_dlink_testmsg (u_char **pnt, u_int16_t *size,
                          struct nsm_msg_dlink_testmsg *msg)
{
  u_char *sp = *pnt;

  /* Encode name of the data-link or name of the dlink-interface */
  TLV_ENCODE_PUT(msg->name, GINTERFACE_NAMSIZ);

  /* Encode gifindex of the dlink */
  TLV_ENCODE_PUTL(msg->gifindex);

  /* Encode dlink-test-msg message-length */
  TLV_ENCODE_PUTW(msg->msg_len);

  /* Encode LMP-encoded-test-msg */
  TLV_ENCODE_PUT (msg->buf, msg->msg_len);

  return *pnt - sp;
}

int
nsm_decode_dlink_testmsg (u_char **pnt, u_int16_t *size,
                          struct nsm_msg_dlink_testmsg *msg)
{
  u_char *sp = *pnt;

  /* Decode name of the data-link or name of the dlink-interface */
  TLV_DECODE_GET(msg->name, GINTERFACE_NAMSIZ);

  /* Decode gifindex of the dlink */
  TLV_DECODE_GETL(msg->gifindex);

  /* Decode length of dlink-test-msg */
  TLV_DECODE_GETW(msg->msg_len);

  /* Decode LMP-Encoded-dlink-test-msg. */
  TLV_DECODE_GET (msg->buf, msg->msg_len);

  return *pnt - sp;
}

#ifdef HAVE_RESTART
int
nsm_encode_lmp_protocol_restart (u_char **pnt, u_int16_t *size,
                                 struct nsm_msg_lmp_protocol_restart *msg)
{
  u_char *sp = *pnt;

  /* Check size.  */
  if (*size < NSM_MSG_PROTOCOL_RESTART_SIZE)
    return NSM_ERR_PKT_TOO_SMALL;

  /* Cindex */
  TLV_ENCODE_PUTL (msg->cindex);

  /* Restart time */
  TLV_ENCODE_PUTL (msg->restart_time);

  /* Protocol id */
  TLV_ENCODE_PUTL (msg->protocol_id);

  return *pnt - sp;
}

/* Encode protocol restart message. */
int
nsm_parse_lmp_protocol_restart (u_char **pnt, u_int16_t *size,
                                struct nsm_msg_header *header, void *arg,
                                NSM_CALLBACK callback)
{
  struct nsm_msg_lmp_protocol_restart msg;
  int ret;

  ret = nsm_decode_lmp_protocol_restart (pnt, size, &msg);
  if (ret < 0)
    return ret;

  if (callback)
    {
      ret = (*callback) (header, arg, &msg);
      if (ret < 0)
        return ret;
    }
  return 0;
}

/* Decode protocol restart message. */
int
nsm_decode_lmp_protocol_restart (u_char **pnt, u_int16_t *size,
                                 struct nsm_msg_lmp_protocol_restart *msg)
{
  u_char *sp = *pnt;

  /* Check Size */
  if (*size < NSM_MSG_PROTOCOL_RESTART_SIZE)
    return NSM_ERR_PKT_TOO_SMALL;

  TLV_DECODE_GETL (msg->cindex);
  TLV_DECODE_GETL (msg->restart_time);
  TLV_DECODE_GETL (msg->protocol_id);

  return *pnt - sp;
}
#endif /* HAVE_LMP */
#endif /* HAVE_RESTART */

#ifdef HAVE_PBB_TE
s_int32_t
nsm_encode_te_tesi_msg (u_char **pnt, u_int16_t *size,
                         struct nsm_msg_tesi_info *msg)
{
  u_char *sp = *pnt;

  TLV_ENCODE_PUTL (msg->cindex);
  TLV_ENCODE_PUT (msg->tesi_name, MAX_TRUNK_NAME_LEN);
  TLV_ENCODE_PUTL (msg->tesid);
  TLV_ENCODE_PUTC (msg->action);
  TLV_ENCODE_PUTC (msg->mode);

  return *pnt - sp;
}

s_int32_t
nsm_decode_te_tesi_msg (u_char **pnt, u_int16_t *size,
                        struct nsm_msg_tesi_info *msg)
{
  u_char *sp = *pnt;

  TLV_DECODE_GETL (msg->cindex);
  TLV_DECODE_GET (msg->tesi_name, MAX_TRUNK_NAME_LEN);
  TLV_DECODE_GETL (msg->tesid);
  TLV_DECODE_GETC (msg->action);
  TLV_DECODE_GETC (msg->mode);

  return *pnt - sp;
}

s_int32_t
nsm_parse_te_tesi_msg(u_char **pnt, u_int16_t *size,
                     struct nsm_msg_header *header, void *arg,
                     NSM_CALLBACK callback)
{
  struct nsm_msg_tesi_info msg;
  int ret;

  ret = nsm_decode_te_tesi_msg (pnt, size, &msg);
  if (ret < 0)
    return ret;

  if (callback)
    {
      ret = (*callback) (header, arg, &msg);
      if (ret < 0)
        return ret;
    }
  return 0;
}
#endif /* HAVE_PBB_TE */

s_int32_t
nsm_encode_generic_label_pool (u_char **pnt, u_int16_t *size,
                       struct nsm_msg_generic_label_pool *msg)
{
  u_char *sp = *pnt;

#ifdef HAVE_PBB_TE
  struct nsm_glabel_pbb_te_service_data *pbb_te_data =
           (struct nsm_glabel_pbb_te_service_data *)msg->u.pb_te_service_data;
#endif /* HAVE_PBB_TE */

  /* Check size.  */
  if (*size < NSM_MSG_LABEL_POOL_SIZE)
    return NSM_ERR_PKT_TOO_SMALL;

  /* Label space. */
  TLV_ENCODE_PUTW (msg->label_space);

#ifdef HAVE_PBB_TE
  if (NSM_CHECK_CTYPE (msg->cindex, NSM_LABEL_POOL_CTYPE_PBB_TE_LABEL))
    {
      nsm_encode_tlv (pnt, size, NSM_LABEL_POOL_CTYPE_PBB_TE_LABEL,
                      sizeof(struct nsm_glabel_pbb_te_service_data));

      TLV_ENCODE_PUTL (pbb_te_data->lbl_req_type);
      TLV_ENCODE_PUTW((pbb_te_data->pbb_label).bvid);
      TLV_ENCODE_PUT ((pbb_te_data->pbb_label).bmac, MAC_ADDR_LEN);

      if(pbb_te_data->lbl_req_type == label_request_ingress)
        TLV_ENCODE_PUT ((pbb_te_data->data).trunk_name, MAX_TRUNK_NAME_LEN);
      else if(pbb_te_data->lbl_req_type == label_request_transit)
        TLV_ENCODE_PUTL ((pbb_te_data->data).ifIndex);

      TLV_ENCODE_PUTL (pbb_te_data->tesid);
      TLV_ENCODE_PUTL (pbb_te_data->result);
    }
#endif /* HAVE_PBB_TE */

  return *pnt - sp;
}


s_int32_t
nsm_decode_generic_label_pool (u_char **pnt, u_int16_t *size,
                       struct nsm_msg_generic_label_pool *msg)
{
  u_char *sp = *pnt;
  struct nsm_tlv_header tlv;
#ifdef HAVE_PBB_TE
  struct nsm_glabel_pbb_te_service_data *pbb_te_data =
           (struct nsm_glabel_pbb_te_service_data *)msg->u.pb_te_service_data;
#endif /* HAVE_PBB_TE */

  /* Check size.  */
  if (*size < NSM_MSG_LABEL_POOL_SIZE)
    return NSM_ERR_PKT_TOO_SMALL;

  /* Label space. */
  TLV_DECODE_GETW (msg->label_space);

  /* TLV parse.  */
  while (*size)
    {
      if (*size < NSM_TLV_HEADER_SIZE)
        return NSM_ERR_PKT_TOO_SMALL;

      NSM_DECODE_TLV_HEADER (tlv);

      switch (tlv.type)
      {
#ifdef HAVE_PBB_TE
        case NSM_LABEL_POOL_CTYPE_PBB_TE_LABEL:
          TLV_DECODE_GETL (pbb_te_data->lbl_req_type);
          TLV_DECODE_GETW((pbb_te_data->pbb_label).bvid);
          TLV_DECODE_GET ((pbb_te_data->pbb_label).bmac, MAC_ADDR_LEN);

          if(pbb_te_data->lbl_req_type == label_request_ingress)
            TLV_DECODE_GET ((pbb_te_data->data).trunk_name, MAX_TRUNK_NAME_LEN);
          else if(pbb_te_data->lbl_req_type == label_request_transit)
            TLV_DECODE_GETL ((pbb_te_data->data).ifIndex);

          TLV_DECODE_GETL (pbb_te_data->tesid);
          TLV_DECODE_GETL (pbb_te_data->result);
          NSM_SET_CTYPE (msg->cindex, tlv.type);
          break;
#endif /* HAVE_PBB_TE */
        default:
          TLV_DECODE_SKIP (tlv.length);
          break;
      }
    }
  return *pnt - sp;
}

s_int32_t
nsm_parse_generic_label_pool (u_char **pnt, u_int16_t *size,
                              struct nsm_msg_header *header, void *arg,
                              NSM_CALLBACK callback)
{
  int ret = 0;
  struct nsm_msg_generic_label_pool msg;
#ifdef HAVE_PBB_TE
  struct nsm_glabel_pbb_te_service_data pbb_te_data;
#endif /* HAVE_PBB_TE */

  pal_mem_set (&msg, 0, sizeof (struct nsm_msg_generic_label_pool));

#ifdef HAVE_PBB_TE
  pal_mem_set (&pbb_te_data, 0, sizeof (struct nsm_glabel_pbb_te_service_data));
  msg.u.pb_te_service_data = &pbb_te_data;
#endif /* HAVE_PBB_TE */

  ret = nsm_decode_generic_label_pool (pnt, size, &msg);
  if (ret < 0)
    return ret;

  if (callback)
    {
      ret = (*callback) (header, arg, &msg);
      if (ret < 0)
        return ret;
    }
  return 0;
}
#endif /* HAVE_GMPLS */
