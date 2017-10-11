/* Copyright (C) 2001-2003  All Rights Reserved. */

#include "pal.h"

#if defined (HAVE_BFD) || defined (HAVE_MPLS_OAM)

#include "lib.h"
#include "log.h"
#include "if.h"
#include "prefix.h"

#include "bfd_message.h"
#include "bfd_client.h"
#include "bfd_client_api.h"

/* Add the BFD client.  */
int
bfd_client_api_client_new (struct lib_globals *zg,
                           struct bfd_client_api_callbacks *cb)
{
  if (! zg)
    return LIB_API_ERROR;
  if (zg->bc)
    bfd_client_api_client_delete (zg);
  zg->bc = bfd_client_new (zg, cb);
  if (! zg->bc)
    return LIB_API_ERROR;
  bfd_client_set_service (zg->bc, BFD_SERVICE_SESSION);
  bfd_client_set_service (zg->bc, BFD_SERVICE_RESTART);
  bfd_client_set_version (zg->bc, BFD_PROTOCOL_VERSION_1);
  return LIB_API_SUCCESS;
}

#ifdef HAVE_MPLS_OAM
int
bfd_client_api_oam_client_new (struct lib_globals *zg, 
                               struct bfd_client_api_callbacks *cb)
{
  if (! zg)
    return LIB_API_ERROR;
  if (zg->bc)
    bfd_client_api_client_delete (zg);
  zg->bc = bfd_oam_client_new (zg, cb);
  if (! zg->bc)
    return LIB_API_ERROR;
  bfd_client_set_service (zg->bc, BFD_SERVICE_OAM);

  return LIB_API_SUCCESS;
}
#endif /* HAVE_MPLS_OAM */

/* Delete the BFD client. */
int
bfd_client_api_client_delete (struct lib_globals *zg)
{
  if (! zg || ! zg->bc)
    return LIB_API_ERROR;
  bfd_client_delete (zg->bc);
  zg->bc = NULL;
  return LIB_API_SUCCESS;
}

/* Client start.  */
int
bfd_client_api_client_start (struct lib_globals *zg)
{
  if (! zg || ! zg->bc)
    return LIB_API_ERROR;
  bfd_client_start (zg->bc);
  return LIB_API_SUCCESS;
}

/* Client stop.  */
int
bfd_client_api_client_stop (struct lib_globals *zg)
{
  if (! zg || ! zg->bc)
    return LIB_API_ERROR;
  bfd_client_stop (zg->bc);
  return LIB_API_SUCCESS;
}

/* Session Add.  */
int
bfd_client_api_session_add (struct bfd_client_api_session *session)
{
  struct lib_globals *zg;
  struct bfd_msg_session s;
  int ret = 0;

  if (! session || ! session->zg || ! session->zg->bc)
    return LIB_API_ERROR;
  zg = session->zg;
  pal_mem_set (&s, 0, sizeof (struct bfd_msg_session));
  s.ifindex = session->ifindex;

  /* For now all sessions are persistent - will be changed later */
  s.flags = session->flags | BFD_MSG_SESSION_FLAG_PS;
  s.afi = session->afi;
  if (s.afi == AFI_IP)
    {
      s.ll_type = BFD_MSG_LL_TYPE_IPV4;
      IPV4_ADDR_COPY (&s.addr.ipv4.src, &session->addr.ipv4.src);
      IPV4_ADDR_COPY (&s.addr.ipv4.dst, &session->addr.ipv4.dst);
    }
#ifdef HAVE_IPV6
  else if (s.afi == AFI_IP6)
    {
      s.ll_type = BFD_MSG_LL_TYPE_IPV6;
      IPV6_ADDR_COPY (&s.addr.ipv6.src, &session->addr.ipv6.src);
      IPV6_ADDR_COPY (&s.addr.ipv6.dst, &session->addr.ipv6.dst);
    }
#endif /* HAVE_IPV6 */
 
#ifdef HAVE_MPLS_OAM
  if (session->ll_type == BFD_MSG_LL_TYPE_MPLS_LSP)
    {
      s.ll_type = BFD_MSG_LL_TYPE_MPLS_LSP;
      pal_mem_cpy (&s.addl_data.mpls_params, &session->addl_data.mpls_params,
                    sizeof (struct bfd_mpls_params));
    }
#ifdef HAVE_VCCV
  else if (session->ll_type == BFD_MSG_LL_TYPE_MPLS_VCCV)
    {
      s.ll_type = BFD_MSG_LL_TYPE_MPLS_VCCV;
      pal_mem_cpy (&s.addl_data.vccv_params, &session->addl_data.vccv_params,
                  sizeof (struct bfd_vccv_params));
    }
#endif /* HAVE_VCCV */
#endif /* HAVE_MPLS_OAM */

  ret = bfd_client_send_session_add (zg->bc, session->vr_id, 
                                     session->vrf_id, &s);
  if (ret < 0)
    return LIB_API_ERROR;

  return LIB_API_SUCCESS;
}

/* Session Delete.  */
int
bfd_client_api_session_delete (struct bfd_client_api_session *session)
{
  struct lib_globals *zg;
  struct bfd_msg_session s;
  int ret = 0;

  if (! session || ! session->zg || ! session->zg->bc)
    return LIB_API_ERROR;
  zg = session->zg;
  pal_mem_set (&s, 0, sizeof (struct bfd_msg_session));
  s.ifindex = session->ifindex;
  s.flags = session->flags;
  s.afi = session->afi;
  if (s.afi == AFI_IP)
    {
      s.ll_type = BFD_MSG_LL_TYPE_IPV4;
      IPV4_ADDR_COPY (&s.addr.ipv4.src, &session->addr.ipv4.src);
      IPV4_ADDR_COPY (&s.addr.ipv4.dst, &session->addr.ipv4.dst);
    }
#ifdef HAVE_IPV6
  else if (s.afi == AFI_IP6)
    {
      s.ll_type = BFD_MSG_LL_TYPE_IPV6;
      IPV6_ADDR_COPY (&s.addr.ipv6.src, &session->addr.ipv6.src);
      IPV6_ADDR_COPY (&s.addr.ipv6.dst, &session->addr.ipv6.dst);
    }
#endif /* HAVE_IPV6 */
 
#ifdef HAVE_MPLS_OAM
  if (session->ll_type == BFD_MSG_LL_TYPE_MPLS_LSP)
    {
      s.ll_type = BFD_MSG_LL_TYPE_MPLS_LSP;
      pal_mem_cpy (&s.addl_data.mpls_params, &session->addl_data.mpls_params,
                    sizeof (struct bfd_mpls_params));
    }
#ifdef HAVE_VCCV
  else if (session->ll_type == BFD_MSG_LL_TYPE_MPLS_VCCV)
    {
      s.ll_type = BFD_MSG_LL_TYPE_MPLS_VCCV;
      pal_mem_cpy (&s.addl_data.vccv_params, &session->addl_data.vccv_params,
                  sizeof (struct bfd_vccv_params));
    }
#endif /* HAVE_VCCV */
#endif /* HAVE_MPLS_OAM */

  ret = bfd_client_send_session_delete (zg->bc, session->vr_id, 
                                        session->vrf_id, &s);
  if (ret < 0)
    return LIB_API_ERROR;

  return LIB_API_SUCCESS;
}

int
bfd_client_api_debug_set (struct lib_globals *zg)
{
  struct bfd_client *bc = zg->bc;
  if (! bc)
    return LIB_API_ERROR;
  bc->debug = PAL_TRUE;
  return LIB_API_SUCCESS;
}

int
bfd_client_api_debug_unset (struct lib_globals *zg)
{
  struct bfd_client *bc = zg->bc;
  if (! bc)
    return LIB_API_ERROR;
  bc->debug = PAL_FALSE;
  return LIB_API_SUCCESS;
}

#endif /* HAVE_BFD  || HAVE_MPLS_OAM*/
