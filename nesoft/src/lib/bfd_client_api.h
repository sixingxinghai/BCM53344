/* Copyright (C) 2001-2003  All Rights Reserved. */

#ifndef _PACOS_BFD_CLIENT_API_H
#define _PACOS_BFD_CLIENT_API_H

#include "message.h"
#include "bfd_message.h"

struct bfd_client_api_server
{
  /* Global pointer.  */
  struct lib_globals *zg;
};

struct bfd_client_api_session
{
  /* Global pointer.  */
  struct lib_globals *zg;

  /* VR ID.  */
  u_int32_t vr_id;

  /* VRF ID.  */
  vrf_id_t vrf_id;

  /* Interface index.  */
  u_int32_t ifindex;

  /* Flags.  */
  u_int32_t flags;
#define BFD_CLIENT_API_FLAG_MH BFD_MSG_SESSION_FLAG_MH /* Multi-Hop.  */
#define BFD_CLIENT_API_FLAG_DC BFD_MSG_SESSION_FLAG_DC /* Demand Circuit.  */
#define BFD_CLIENT_API_FLAG_PS BFD_MSG_SESSION_FLAG_PS /* Persistent Session.  */
#define BFD_CLIENT_API_FLAG_AD BFD_MSG_SESSION_FLAG_AD /* User Admin Down.  */

  /* Link layer type.  */
  u_char ll_type;

  /* Address family.  */
  afi_t afi;

  /* Source and destination addresses.  */
  union
  {
    struct
    {
      struct pal_in4_addr src;
      struct pal_in4_addr dst;
    } ipv4;
#ifdef HAVE_IPV6
    struct
    {
      struct pal_in6_addr src;
      struct pal_in6_addr dst;
    } ipv6;
#endif /* HAVE_IPV6 */
  } addr;
 
#ifdef HAVE_MPLS_OAM
  union 
  {
    struct bfd_mpls_params mpls_params; /** BFD MPLS Params */
#ifdef HAVE_VCCV
    struct bfd_vccv_params vccv_params; /** BFD VCCV Params */
#endif /* HAVE_VCCV */
  }addl_data;
#endif /* HAVE_MPLS_OAM */
};

typedef void (*BFD_CLIENT_API_CB) (struct bfd_client_api_server *);
typedef void (*BFD_CLIENT_API_SESSION_CB) (struct bfd_client_api_session *);
#ifdef HAVE_MPLS_OAM
typedef int (*BFD_CLIENT_API_OAM_CB) (struct bfd_msg_header *, void *, void *);
#endif /* HAVE_MPLS_OAM */

struct bfd_client_api_callbacks
{
  BFD_CLIENT_API_CB connected;
  BFD_CLIENT_API_CB disconnected;
  BFD_CLIENT_API_CB reconnected;
  BFD_CLIENT_API_SESSION_CB session_up;
  BFD_CLIENT_API_SESSION_CB session_down;
  BFD_CLIENT_API_SESSION_CB session_admin_down;
  BFD_CLIENT_API_SESSION_CB session_error;
#ifdef HAVE_MPLS_OAM
  BFD_CLIENT_API_SESSION_CB mpls_session_up;
  BFD_CLIENT_API_SESSION_CB mpls_session_down;
#ifdef HAVE_VCCV
  BFD_CLIENT_API_SESSION_CB vccv_session_up;
  BFD_CLIENT_API_SESSION_CB vccv_session_down;
#endif /* HAVE_VCCV */
  BFD_CLIENT_API_OAM_CB oam_ping_response;
  BFD_CLIENT_API_OAM_CB oam_trace_response;
  BFD_CLIENT_API_OAM_CB oam_error;
#endif /* HAVE_MPLS_OAM */
};

/* Add the BFD client.  */
int
bfd_client_api_client_new (struct lib_globals *zg,
                           struct bfd_client_api_callbacks *cb);

#ifdef HAVE_MPLS_OAM
int
bfd_client_api_oam_client_new (struct lib_globals *zg, 
                               struct bfd_client_api_callbacks *cb);
#endif /* HAVE_MPLS_OAM */

/* Delete the BFD client. */
int
bfd_client_api_client_delete (struct lib_globals *zg);

/* Client start.  */
int
bfd_client_api_client_start (struct lib_globals *zg);

/* Client stop.  */
int
bfd_client_api_client_stop (struct lib_globals *zg);

/* Session Add.  */
int
bfd_client_api_session_add (struct bfd_client_api_session *session);

/* Session Delete.  */
int
bfd_client_api_session_delete (struct bfd_client_api_session *session);

/* Change the debug status.  */
int
bfd_client_api_debug_set (struct lib_globals *zg);

int
bfd_client_api_debug_unset (struct lib_globals *zg);


#endif /* _PACOS_BFD_CLIENT_API_H */

