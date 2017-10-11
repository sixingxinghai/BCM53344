/* Copyright (C) 2005  All Rights Reserved. */

#include "pal.h"
#include "lib.h"
#include "lib_mtrace.h"

char *
mtrace_rtg_proto_str (u_int8_t rtg_proto)
{
  char *p;

  switch (rtg_proto)
    {
    case  MTRACE_PROTO_DVMRP: 
      p =  "DVMRP";
      break;
    case MTRACE_PROTO_MOSPF:
      p =  "MOSPF";
      break;
    case MTRACE_PROTO_PIM:
      p =  "PIM";
      break;
    case MTRACE_PROTO_CBT:
      p =  "CBT";
      break;
    case MTRACE_PROTO_PIM_SPCL:
      p =  "PIM special routing table";
      break;
    case MTRACE_PROTO_PIM_STATIC:
      p =  "PIM static route";
      break;
    case MTRACE_PROTO_DVMRP_STATIC:
      p =  "DVMRP static route";
      break;
    case MTRACE_PROTO_PIM_MBGP:
      p =  "PIM MBGP route";
      break;
    case MTRACE_PROTO_CBT_SPCL:
      p =  "CBT special routing table";
      break;
    case MTRACE_PROTO_CBT_STATIC:
      p =  "CBT static route";
      break;
    case MTRACE_PROTO_PIM_ASSERT:
      p =  "PIM Assert state";
      break;
    default:
      p =  "Unknown";
      break;
    }
  return p;
}

char *
mtrace_fwd_code_str (u_int8_t fwd_code)
{
  char *p;

  switch (fwd_code)
    {
    case MTRACE_CODE_NO_ERR:
      p = "NO ERROR";
      break;
    case MTRACE_CODE_WRONG_IF:
      p = "WRONG IF";
      break;
    case MTRACE_CODE_PRUNE_SENT:
      p = "PRUNE SENT";
      break;
    case MTRACE_CODE_PRUNE_RCVD:
      p = "PRUNE RCVD";
      break;
    case MTRACE_CODE_SCOPED:
      p = "SCOPED";
      break;
    case MTRACE_CODE_NO_ROUTE:
      p = "NO ROUTE";
      break;
    case MTRACE_CODE_WRONG_LAST_HOP:
      p = "WRONG LAST HOP";
      break;
    case MTRACE_CODE_NOT_FWDING:
      p = "NOT FORWARDING";
      break;
    case MTRACE_CODE_REACHED_RP:
      p = "REACHED RP";
      break;
    case MTRACE_CODE_RPF_IF:
      p = "RPF IF";
      break;
    case MTRACE_CODE_NO_MULTICAST:
      p = "NO MULTICAST";
      break;
    case MTRACE_CODE_INFO_HIDDEN:
      p = "INFO HIDDEN";
      break;
    case MTRACE_CODE_FATAL_ERROR:
      p = "FATAL ERROR";
      break;
    case MTRACE_CODE_NO_SPACE:
      p = "NO SPACE";
      break;
    case MTRACE_CODE_OLD_ROUTER:
      p = "OLD ROUTER";
      break;
    case MTRACE_CODE_ADMIN_PROHIB:
      p = "ADMIN PROHIB";
      break;
    default:
      p = "UNKNOWN";
      break;
    }
  return p;
}

