/* Copyright (C) 2001, 2002  All Rights Reserved. */

#include "pal.h"
#include "lib.h"

#include "thread.h"
#include "log.h"
#include "snprintf.h"
#include "timeutil.h"

#include "ospf_const.h"
#include "ospf_util.h"
#include "ospf_fsm.h"
#include "ospf_debug_msg.h"


/* Interface State messages. */
struct message ospf_ifsm_state_msg[] =
{
  { IFSM_DependUpon,   "DependUpon" },
  { IFSM_Down,         "Down" },
  { IFSM_Loopback,     "Loopback" },
  { IFSM_Waiting,      "Waiting" },
  { IFSM_PointToPoint, "Point-To-Point" },
  { IFSM_DROther,      "DROther" },
  { IFSM_Backup,       "Backup" },
  { IFSM_DR,           "DR" },
};
int ospf_ifsm_state_msg_max = OSPF_IFSM_STATE_MAX;

/* Neighbor State messages. */
struct message ospf_nfsm_state_msg[] =
{
  { NFSM_DependUpon, "DependUpon" },
  { NFSM_Down,       "Down" },
  { NFSM_Attempt,    "Attempt" },
  { NFSM_Init,       "Init" },
  { NFSM_TwoWay,     "2-Way" },
  { NFSM_ExStart,    "ExStart" },
  { NFSM_Exchange,   "Exchange" },
  { NFSM_Loading,    "Loading" },
  { NFSM_Full,       "Full" },
};
int ospf_nfsm_state_msg_max = OSPF_NFSM_STATE_MAX;

/* Interface event messages. */
struct message ospf_ifsm_event_msg[] =
{
  { IFSM_NoEvent,               "NoEvent" }, 
  { IFSM_InterfaceUp,           "InterfaceUp" },
  { IFSM_WaitTimer,             "WaitTimer" },
  { IFSM_BackupSeen,            "BackupSeen" },
  { IFSM_NeighborChange,        "NeighborChange" },
  { IFSM_LoopInd,               "LoopInd" },
  { IFSM_UnloopInd,             "UnLoopInd" },
  { IFSM_InterfaceDown,         "InterfaceDown" },
};
int ospf_ifsm_event_msg_max = OSPF_IFSM_EVENT_MAX;

/* Neighbor event messages. */
struct message ospf_nfsm_event_msg[] =
{
  { NFSM_NoEvent,               "NoEvent" },
  { NFSM_HelloReceived,         "HelloReceived" },
  { NFSM_Start,                 "Start" },
  { NFSM_TwoWayReceived,        "2-WayReceived" },
  { NFSM_NegotiationDone,       "NegotiationDone" },
  { NFSM_ExchangeDone,          "ExchangeDone" },
  { NFSM_BadLSReq,              "BadLSReq" },
  { NFSM_LoadingDone,           "LoadingDone" },
  { NFSM_AdjOK,                 "AdjOK?" },
  { NFSM_SeqNumberMismatch,     "SeqNumberMismatch" },
  { NFSM_OneWayReceived,        "1-WayReceived" },
  { NFSM_KillNbr,               "KillNbr" },
  { NFSM_InactivityTimer,       "InactivityTimer" },
  { NFSM_LLDown,                "LLDown" },
};
int ospf_nfsm_event_msg_max = OSPF_NFSM_EVENT_MAX;

/* OSPF Packet type messages. */
struct message ospf_packet_type_msg[] =
{
  {0,                   "Unknown"},
  {OSPF_PACKET_HELLO,   "Hello"},
  {OSPF_PACKET_DB_DESC, "DD"},
  {OSPF_PACKET_LS_REQ,  "LS-Req"},
  {OSPF_PACKET_LS_UPD,  "LS-Upd"},
  {OSPF_PACKET_LS_ACK,  "LS-Ack"}
};
int ospf_packet_type_msg_max = OSPF_PACKET_TYPE_MAX;

/* Redistributed Protocol messages. */
struct message ospf_redist_proto_msg[] =
{
  { APN_ROUTE_DEFAULT,  "Default" },
  { APN_ROUTE_KERNEL,   "Kernel" },
  { APN_ROUTE_CONNECT,  "Connected" },
  { APN_ROUTE_STATIC,   "Static" },
  { APN_ROUTE_RIP,      "RIP" },
  { APN_ROUTE_RIPNG,    "RIPng" },
  { APN_ROUTE_OSPF,     "OSPF" },
  { APN_ROUTE_OSPF6,    "OSPFv3" },
  { APN_ROUTE_BGP,      "BGP" },
  { APN_ROUTE_ISIS,     "IS-IS" },
  { APN_ROUTE_MAX,      "" },
};
int ospf_redist_proto_msg_max = APN_ROUTE_MAX;

/* Interface Network Type. */
struct message ospf_if_network_type_msg[] =
{
  { OSPF_IFTYPE_NONE,                   "Null" },
  { OSPF_IFTYPE_POINTOPOINT,            "POINTOPOINT" },
  { OSPF_IFTYPE_BROADCAST,              "BROADCAST" },
  { OSPF_IFTYPE_NBMA,                   "NBMA" },
  { OSPF_IFTYPE_POINTOMULTIPOINT,       "POINTOMULTIPOINT" },
  { OSPF_IFTYPE_POINTOMULTIPOINT_NBMA,  "P2MP-NBMA" },
  { OSPF_IFTYPE_VIRTUALLINK,            "VIRTUALLINK" },
  { OSPF_IFTYPE_LOOPBACK,               "LOOPBACK" },
};
int ospf_if_network_type_msg_max = OSPF_IFTYPE_MAX;

struct message ospf_nsm_route_type_msg[] =
{
  { APN_ROUTE_DEFAULT,  "system" },
  { APN_ROUTE_KERNEL,   "kernel" },
  { APN_ROUTE_CONNECT,  "connected" },
  { APN_ROUTE_STATIC,   "static" },
  { APN_ROUTE_RIP,      "rip" },
  { APN_ROUTE_RIPNG,    "rip" },
  { APN_ROUTE_OSPF,     "ospf" },
  { APN_ROUTE_OSPF6,    "ospf" },
  { APN_ROUTE_BGP,      "bgp" },
  { APN_ROUTE_ISIS,     "isis" },
};
int ospf_nsm_route_type_msg_max = APN_ROUTE_MAX;

struct message ospf_area_type_msg[] =
{
  { OSPF_AREA_DEFAULT,  "Default" },
  { OSPF_AREA_STUB,     "Stub" },
  { OSPF_AREA_NSSA,     "NSSA" },
};
int ospf_area_type_msg_max = OSPF_AREA_TYPE_MAX;


/* Interface Type String. */
char *ospf_if_type_str[] = 
{
  "unknown",               /* should never be used. */
  "point-to-point",
  "broadcast",
  "non-broadcast",
  "point-to-multipoint",
  "point-to-multipoint non-broadcast",
  "virtual-link",          /* should never be used. */
  "loopback",
  "host"
};

/* ABR Type String.  */
char *ospf_abr_type_str[] = 
{
  "unknown",
  "standard",
  "cisco",
  "ibm",
  "shortcut"
};

/* Packet Type String. */
char *ospf_packet_type_str[] =
{
  "unknown",
  "Hello",
  "Database Description",
  "Link State Request",
  "Link State Update",
  "Link State Acknowledgment",
};

/* Path Code String.  */
char *ospf_path_code_str[] =
{
  "*",
  "C",
  "D",
  "O",
  "IA",
  "E1",
  "E2",
  "N1",
  "N2",
};


char *
ospf_if_type_string (u_char type)
{
  if (type < OSPF_IFTYPE_MAX)
    return ospf_if_type_str[type];

  return "unknown";
}

char *
ospf_abr_type_string (u_char type)
{
  if (type < OSPF_ABR_TYPE_MAX)
    return ospf_abr_type_str[type];

  return "unknown";
}

char *
ospf_packet_type_string (u_char type)
{
  if (type < OSPF_PACKET_TYPE_MAX)
    return ospf_packet_type_str[type];

  return "unknown";
}

char *
ospf_timer_dump (struct thread *t, char *buf)
{
  struct pal_timeval now;
  unsigned long h = 0, m = 0, s = 0;

  if (!t)
    return "inactive";

  pal_time_tzcurrent (&now, NULL);

  if (t->u.sands.tv_sec - now.tv_sec > 0)
    s = t->u.sands.tv_sec - now.tv_sec;
  else
    s = 0;

  if (s >= 3600)
    {
      h = s / 3600;
      s %= 3600;
    }

  if (s >= 60)
    {
      m = s / 60;
      s %= 60;
    }

  zsnprintf (buf, OSPF_TIMER_STR_MAXLEN, "%02ld:%02ld:%02ld", h, m, s);

  return buf;
}

char *
ospf_uptime_dump (pal_time_t ts, char *buf)
{
  pal_time_t now;
  unsigned long h = 0, m = 0, s = 0;

  now = pal_time_current (NULL);
  s = now - ts;
  if (s >= 3600)
    {
      h = s / 3600;
      s %= 3600;
    }

  if (s >= 60)
    {
      m = s / 60;
      s %= 60;
    }

  zsnprintf (buf, OSPF_TIMER_STR_MAXLEN, "%02ld:%02ld:%02ld", h, m, s);

  return buf;
}
