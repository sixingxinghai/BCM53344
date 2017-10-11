/* Copyright (C) 2004  All Rights Reserved. */

#include "config.h"
#include "hsl_os.h"
#include "hsl_types.h"
#include "hsl_oss.h"
#if defined(VXWORKS) && !defined(PNE_VERSION_2_2)
#include <ctype.h>
#endif /* VXWORKS & !PNE_VERSION_2_2 */
/* 
   Maskbit. 
*/
static const u_int8_t maskbit[] = {0x00, 0x80, 0xc0, 0xe0, 0xf0,
                                  0xf8, 0xfc, 0xfe, 0xff};

#ifdef HAVE_IPV6
#define HSL_INT16SZ      sizeof(u_int16_t)
#define HSL_IN6ADDRSZ    16
#endif 

/*
   Mac addr to string 
 */ 
char * 
hsl_mac_2_str(u_char *mac,char *dst, size_t size) 
{
  int sz = sizeof "ff:ff:ff:ff:ff:ff";
  static const char *fmt = "%02x:%02x:%02x:%02x:%02x:%02x";
  char tmp[sz];

  if ((size_t) sprintf (tmp, fmt, mac[0],mac[1],mac[2],mac[3],mac[4],mac[5]) >= size)
    {
      return (NULL);
    }

  memcpy (dst, tmp, sz);

  return (dst);
}
 
/* 
   Prefix same? 
*/
int
hsl_prefix_same (hsl_prefix_t *p1, hsl_prefix_t *p2)
{
  if (p1->family == p2->family && p1->prefixlen == p2->prefixlen)
    {
      if (p1->family == AF_INET)
        if (IPV4_ADDR_SAME (&p1->u.prefix, &p2->u.prefix))
          return 1;
#ifdef HAVE_IPV6
      if (p1->family == AF_INET6 )
        if (IPV6_ADDR_SAME (&p1->u.prefix, &p2->u.prefix))
          return 1;
#endif /* HAVE_IPV6 */
    }
  return 0;
}

/*
  Mask length to mask. 
*/
void
hsl_masklen2ip (u_int32_t masklen, hsl_ipv4Address_t *netmask)
{
  *netmask = (0xffffffff  << (32 - masklen));
}
#ifdef HAVE_IPV6
/*
  Mask length to mask. 
*/
void
hsl_masklen2ip6 (s_int32_t masklen, hsl_ipv6Address_t *netmask)
{
  char *pnt;
  int bit;
  int offset;
  /* Maskbit. */
  static const u_int8_t maskbit[] = {0x00, 0x80, 0xc0, 0xe0, 0xf0,
                                     0xf8, 0xfc, 0xfe, 0xff};


  memset (netmask, 0, sizeof (hsl_ipv6Address_t));
  pnt = (char *) netmask;

  offset = masklen / 8;
  bit = masklen % 8;

  while (offset--)
    *pnt++ = 0xff;

  if (bit)
    *pnt = maskbit[bit];
}
#endif /* HAVE_IPV6 */
/* If n includes p prefix then return 1 else return 0. */
int
hsl_prefix_match (hsl_prefix_t *n, hsl_prefix_t *p)
{
  int offset;
  int shift;

  /* Set both prefix's head pointer. */
  u_char *np = (u_char *)&n->u.prefix;
  u_char *pp = (u_char *)&p->u.prefix;
    
  /* If n's prefix is longer than p's one return 0. */
  if (n->prefixlen > p->prefixlen)
    return 0;

  offset = n->prefixlen / PNBBY;
  shift =  n->prefixlen % PNBBY;

  if (shift)
    if (maskbit[shift] & (np[offset] ^ pp[offset]))
      return 0;

  while (offset--)
    if (np[offset] != pp[offset])
      return 0;
  return 1;
}

/* Apply mask to IPv4 prefix. */
void
hsl_apply_mask_ipv4 (hsl_prefix_t *p)
{
  u_int8_t *pnt;
  s_int32_t index;
  s_int32_t offset;

  index = p->prefixlen / 8;

  if (index < 4)
    {
      pnt = (u_int8_t *) &p->u.prefix;
      offset = p->prefixlen % 8;

      pnt[index] &= maskbit[offset];
      index++;

      while (index < 4)
        pnt[index++] = 0;
    }
}

#ifdef HAVE_IPV6
void
hsl_apply_mask_ipv6 (hsl_prefix_t *p)
{
  u_int8_t *pnt;
  s_int32_t index;
  s_int32_t offset;

  index = p->prefixlen / 8;

  if (index < 16)
    {
      pnt = (u_int8_t *) &p->u.prefix;
      offset = p->prefixlen % 8;

      pnt[index] &= maskbit[offset];
      index++;

      while (index < 16)
        pnt[index++] = 0;
    }
}
#endif /* HAVE_IPV6 */

void
hsl_apply_mask (hsl_prefix_t *p)
{
  switch (p->family)
    {
      case AF_INET:
        hsl_apply_mask_ipv4 (p);
        break;
#ifdef HAVE_IPV6
      case AF_INET6:
        hsl_apply_mask_ipv6 (p);
        break;
#endif /* HAVE_IPV6 */
      default:
        break;
    }
  return;
}

/*
   Prefix to string.
*/
int
hsl_prefix2str (hsl_prefix_t *p, char *str, int size)
{
  char buf[256];

  hsl_inet_ntop (p->family, &p->u.prefix, buf, 256);
  snprintf (str, size, "%s/%d", buf, p->prefixlen);
  return 0;
}

/*
   Prefix copy.
*/
/* Copy prefix from src to dest. */
void
hsl_prefix_copy (hsl_prefix_t *dest, hsl_prefix_t *src)
{
  dest->family = src->family;
  dest->prefixlen = src->prefixlen;
  
  if (src->family == AF_INET)
    dest->u.prefix4 = src->u.prefix4;
#ifdef HAVE_IPV6
  else if (src->family == AF_INET6)
    dest->u.prefix6 = src->u.prefix6;
#endif /* HAVE_IPV6 */
}

/*
   IPv4 inet_ntop
*/
char *
_hsl_inet_ntop4 (const u_int8_t * src, s_int8_t * dst, size_t size)
{
  int sz = sizeof "255.255.255.255";
  static const char *fmt = "%u.%u.%u.%u";
  char tmp[sz];

  if ((size_t) sprintf (tmp, fmt, src[0], src[1], src[2], src[3]) >= size)
    {
      return (NULL);
    }
  memcpy (dst, tmp, sz);

  return ((char*)dst);
}

#ifdef HAVE_IPV6
/*
  IPv6 inet_ntop
*/
char *
_hsl_inet_ntop6 (const u_int8_t * src, s_int8_t * dst, size_t size)
{
  int i;

  struct {
    int base, len;
  } best = {0, 0}, cur = {0, 0};

  char tmp[sizeof "ffff:ffff:ffff:ffff:ffff:ffff:255.255.255.255"], *tp;
  unsigned int words[HSL_IN6ADDRSZ / HSL_INT16SZ];
 
  /*
   * Preprocess:
   *      Copy the input (bytewise) array into a wordwise array.
   *      Find the longest run of 0x00's in src[] for :: shorthanding.
   */
  memset(words, '\0', sizeof words);

  for ( i = 0; i < HSL_IN6ADDRSZ; i++ )
    words[i / 2] |= (src[i] << ((1 - (i % 2)) << 3));

  best.base = -1;
  cur.base = -1;
  for ( i = 0; i < (HSL_IN6ADDRSZ / HSL_INT16SZ); i++ ) 
  {
    if ( words[i] == 0 ) 
    {
      if ( cur.base == -1 )
        cur.base = i, cur.len = 1;
      else
        cur.len++;
    }  
    else 
    {
      if ( cur.base != -1 ) 
      {
        if ( best.base == -1 || cur.len > best.len )
          best = cur;
        cur.base = -1;
      }
    }
  }
  if ( cur.base != -1 ) 
  {
    if ( best.base == -1 || cur.len > best.len )
      best = cur;
  }
  if ( best.base != -1 && best.len < 2 )
    best.base = -1;

  /*
   * Format the result.
   */
  tp = tmp;
  for ( i = 0; i < (HSL_IN6ADDRSZ / HSL_INT16SZ); i++ ) 
  {
    /* Are we inside the best run of 0x00's? */
    if ( best.base != -1 && i >= best.base &&
         i < (best.base + best.len) ) 
    {
      if ( i == best.base )
        *tp++ = ':';
      continue;
    }
    /* Are we following an initial run of 0x00s or any real hex? */
    if ( i != 0 )
      *tp++ = ':';
    /* Is this address an encapsulated IPv4? */
    if ( i == 6 && best.base == 0 &&
         (best.len == 6 || (best.len == 5 && words[5] == 0xffff)) ) 
    {
      if ( !_hsl_inet_ntop4(src+12, (s_int8_t*)tp,
                            sizeof tmp - (tp - tmp)) )
        return 0;
      tp += strlen(tp);
      break;
    }
    tp += sprintf(tp, "%x", words[i]);
  }
  /* Was it a trailing run of 0x00's? */
  if ( best.base != -1 && (best.base + best.len) ==
       (HSL_IN6ADDRSZ / HSL_INT16SZ) )
    *tp++ = ':';
  *tp++ = '\0';
 
  /*
   * Check for overflow, copy, and we're done.
   */
  if ( (size_t)(tp - tmp) > size ) 
  {
    return NULL;
  }
  strcpy((char*)dst, tmp);
  return (char*)dst;
}
#endif /* HAVE_IPV6 */

/*
   inet_ntop
*/
char *
hsl_inet_ntop (int af, void *src, char *dst, int size)
{
  switch (af)
    {
    case AF_INET:
      return (_hsl_inet_ntop4 (src, (s_int8_t*)dst, size));
#ifdef HAVE_IPV6
    case AF_INET6:
      return (_hsl_inet_ntop6 (src, (s_int8_t*)dst, size));
#endif /* HAVE_IPV6 */
    default:
      return (NULL);
    }
  /* NOTREACHED */
}

int
_hsl_inet_pton4 (const char *strptr, void *addrptr)
{
  int    octet;
  int    octval;
  u_int32_t addr = 0;
  
  for (octet = 1; octet <= 4; octet++)
  {
      octval = 0;
      while (isdigit((int)*strptr))
          octval = octval * 10 + (*strptr++ - '0');
      if (octval > 255 || (*strptr++ != '.' && octet != 4))
          goto errout;
      
      addr = (addr << 8) + octval;
  }
  HSL_SET_HTONL(addrptr, addr);
  return 1;

errout:
  return 0;
}

#ifdef HAVE_IPV6
int
_hsl_inet_pton6 (const char *strptr, void *addrptr)
{
  hsl_ipv6Address_t *in6_val = (hsl_ipv6Address_t *) addrptr;
  int                 colon_count = 0;
  HSL_BOOL            found_double_colon = HSL_FALSE;
  int                 xstart = 0;  /* first zero (double colon) */
  int                 len = 7;     /* numbers of zero words the double colon represents */
  int                 i;
  const char      *s = strptr;
  
  /* First pass, verify the syntax and locate the double colon if present */
  for (;;)
  {
      while (isxdigit((int)*s))
          s++;
      if (*s == '\0')
          break;
      if (*s != ':')
      {
          if (*s == '.' && len >= 2)
          {
              while (s != strptr && *(s-1) != ':')
                  --s;
              if (hsl_inet_pton(AF_INET, s, in6_val) == 1)
              {
                  len -= 2;
                  break;
              }
          }
          /* This could be a valid address */
          break;
      }
      if (s == strptr)
      {
          /* The address begins with a colon */
          if (*++s != ':')
              /* Must start with a double colon or a number */
              goto errout1;
      }
      else
      {
          s++;
          if (found_double_colon)
              len--;
          else
              xstart++;
      }
      
      if (*s == ':')
      {
          if (found_double_colon)
              /* Two double colons are not allowed */
              goto errout1;
          found_double_colon = HSL_TRUE;
          len -= xstart;
          s++;
      }
      
      if (++colon_count == 7)
          /* Found all colons */
          break;
  }
  
  if (colon_count == 0 || colon_count > 7)
      goto errout1;
  if (*--s == ':')
      len++;
  
  /* Second pass, read the address */
  s = strptr;
  for (i = 0; i < 8; i++)
  {
      int val = 0;
      
      if (found_double_colon && i >= xstart && i < xstart + len)
      {
          ((u_int16_t *)in6_val)[i] = 0;
          continue;
      }
      while (*s == ':')
          s++;

      if (i == 6 && isdigit((int)*s)
          && hsl_inet_pton(AF_INET, s, &(((u_int16_t *)in6_val)[i])) == 1)
          /* Ending with :IPv4-address, which is valid */
          break;

      while (isxdigit((int)*s))
      {
          val <<= 4;
          if (*s >= '0' && *s <= '9')
              val += (*s - '0');
          else if (*s >= 'A' && *s <= 'F')
              val += (*s - 'A' + 10);
          else
              val += (*s - 'a' + 10);
          s++;
      }
      ((u_int16_t *)in6_val)[i] = (u_int16_t)htons(val);
  }
  return 1;

errout1:
  return 0;
}
#endif /* HAVE_IPV6 */

/*
   inet_pton
*/
int
hsl_inet_pton (int af, const char *strptr, void *addrptr)
{
  switch (af)
    {
    case AF_INET:
      return (_hsl_inet_pton4 (strptr, addrptr));
#ifdef HAVE_IPV6
    case AF_INET6:
      return (_hsl_inet_pton6 (strptr, addrptr));
#endif /* HAVE_IPV6 */
    default:
      return -1;
    }
  /* NOTREACHED */
}

/*
   hsl_strlen 
*/
int
hsl_strlen(char *s)
{
  char *p = s;
  int str_len; 

  while (*p != '\0')
     p++;

#define HSL_MAX_STR_LEN (500)  

  str_len = (int)(p - s);

  if((str_len < 0) || (str_len > HSL_MAX_STR_LEN))
  {
     str_len = 0; 
  } 
  return str_len;
}
/* 
   64 bit addition
*/
void
hsl_add_uint64(ut_int64_t *a,ut_int64_t *b, ut_int64_t *res)
{
  u_int32_t maxv;
  if(!res)
    return;

  maxv = MAX(a->l[0],b->l[0]);
  res->l[0] = a->l[0] + b->l[0];         
  res->l[1] = a->l[1] + b->l[1];         
  res->l[1] += (res->l[0] < maxv)?1:0; 
}
/* 
   64 bit substruction
*/
void
hsl_sub_uint64(ut_int64_t *a,ut_int64_t *b, ut_int64_t *res)
{                                      
  if(!res)
    return;

  if((a->l[1] < b->l[1]) || 
     ((a->l[1] == b->l[1]) && (a->l[0] < b->l[0])))                   
  {                                       
    res->l[0] = 0xFFFFFFFF - a->l[0];    
    res->l[1] = 0xFFFFFFFF - a->l[1];    
    hsl_add_uint64(res, b, res);

    return; 
  }                                       
  if(a->l[0] < b->l[0])                  
  {                                  
    res->l[0] = (0xFFFFFFFF - (b->l[0] - a->l[0] - 1)); 
    a->l[1] -= 1;                   
  }                                  
  else                                 
  {                                  
    res->l[0] = (a->l[0] - b->l[0]); 
  }                                  
  res->l[1] = a->l[1] - b->l[1];        
}

/* 
  Generic malloc function. 
*/
void *
hsl_malloc(u_int32_t size)
{
  return oss_malloc(size,OSS_MEM_HEAP); 
}

/* 
    Generic free function 
 */
void
hsl_free(void *ptr)
{
  oss_free(ptr,OSS_MEM_HEAP); 
  return;
}

