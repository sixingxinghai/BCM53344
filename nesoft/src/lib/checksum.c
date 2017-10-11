/* Copyright (C) 2001-2003  All Rights Reserved. */

#include "pal.h"
#include "checksum.h"

result_t
in_checksum (u_int16_t *ptr, size_t nbytes)
{
  register s_int32_t sum;
  u_int16_t oddbyte = 0;
  register u_int16_t result;
  u_int8_t *p, *q;

  sum = 0;
  while (nbytes > 1)  
    {
      sum += *ptr++;
      nbytes -= 2;
    }

  if (nbytes == 1) 
    {
      p = (u_int8_t *)&oddbyte;
      q = (u_int8_t *)ptr;
      *p = *q;
      sum += oddbyte;
    }

  sum  = (sum >> 16) + (sum & 0xffff);
  sum += (sum >> 16);
  result = ~sum;

  return result;
}

#ifdef HAVE_IPV6
result_t
in6_checksum (struct prefix *psrc,
              struct prefix *pdst,
              u_int8_t nxt_hdr,
              u_int16_t *ptr,
              size_t nbytes)
{
  struct in6_pseudo_hdr in6_ph;
  register u_int16_t result;
  register s_int32_t sum;
  u_int16_t tmp_size;
  u_int16_t *tmp_p;

  sum = 0;

  /* Prepare IPv6 Pseudo-Header */
  pal_mem_set (&in6_ph, 0, sizeof (struct in6_pseudo_hdr));
  IPV6_ADDR_COPY (&in6_ph.in6_src, &psrc->u.prefix6);
  IPV6_ADDR_COPY (&in6_ph.in6_dst, &pdst->u.prefix6);
  in6_ph.length = pal_hton32 (nbytes);
  in6_ph.nxt_hdr = nxt_hdr;

  /* First checksum IPv6 Pseudo-Header */
  tmp_size = sizeof (struct in6_pseudo_hdr);
  tmp_p = (u_int16_t *) &in6_ph;
  while (tmp_size)
    {
      sum += *tmp_p++;
      tmp_size -= sizeof (u_int16_t);
    }

  /* Next checksum IPv6 PDU */
  tmp_size = nbytes;
  tmp_p = ptr;
  while (tmp_size > 1)
    {
      sum += *tmp_p++;
      tmp_size -= sizeof (u_int16_t);
    }

  if (tmp_size == 1)
    sum += *((u_int8_t *) tmp_p);

  sum  = (sum >> 16) + (sum & 0xffff);
  sum += (sum >> 16);
  result = ~sum;

  return result;
}
#endif /* HAVE_IPV6 */

