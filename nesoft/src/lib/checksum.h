/* Copyright (C) 2001-2003  All Rights Reserved. */

#ifndef _CHECKSUM_H
#define _CHECKSUM_H

#include "pal.h"

#ifdef HAVE_IPV6
struct in6_pseudo_hdr
{
  struct pal_in6_addr in6_src;
  struct pal_in6_addr in6_dst;
  u_int32_t length;
  u_int8_t zero [3];
  u_int8_t nxt_hdr;
};
#endif /* HAVE_IPV6 */

result_t in_checksum (u_int16_t *ptr, size_t nbytes);
#ifdef HAVE_IPV6
result_t
in6_checksum (struct prefix *, struct prefix *, u_int8_t,
              u_int16_t *ptr, size_t nbytes);
#endif /* HAVE_IPV6 */

#endif /* def _CHECKSUM_H */
