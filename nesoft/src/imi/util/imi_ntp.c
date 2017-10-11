/* Copyright (C) 2003  All Rights Reserved. */

#include <pal.h>

#ifdef HAVE_NTP

#include "lib.h"
#include "fp.h"
#include "cli.h"

#include "imi/imi.h"
#include "imi/util/imi_ntp.h"

#include "imi/pal_ntp.h"

/* NTP master structure. */
struct ntp_master *imi_ntp_master;

static const char *ntp_day[] =
  {
    "Sun",
    "Mon",
    "Tue",
    "Wed",
    "Thu",
    "Fri",
    "Sat"
  };

static const char *ntp_month[] =
  {
    "Jan",
    "Feb",
    "Mar",
    "Apr",
    "May",
    "Jun",
    "Jul",
    "Aug",
    "Sep",
    "Oct",
    "Nov",
    "Dec"
  };

/* Flash decode for peer status. */
static const char ntp_flash[] = " x.-+#*o";


static char *
imi_ntp_get_sync_status (u_int16_t leap)
{
  switch (leap)
    {
    case NTP_LEAP_NOWARNING:
    case NTP_LEAP_ADDSECOND:
    case NTP_LEAP_DELSECOND:
      {
        /* Clock is synchronised. */
        return "Clock is synchronized";
      }
      break;
    case NTP_LEAP_NOTINSYNC:
    default:
      {
        /* Clock is unsynchronized. */
        return "Clock is unsynchronized";
      }
    }
}

static char *
imi_ntp_mode_str (u_int32_t mode)
{
  switch (mode)
    {
    case NTP_MODE_UNSPEC:
      {
        /* unspecified (probably old NTP version). */
        return "unspec";
      }
      break;
    case NTP_MODE_ACTIVE:
      {
        /* symmetric active. */
        return "active";
      }
      break;
    case NTP_MODE_PASSIVE:
      {
        /* symmetric passive. */
        return "passive";
      }
      break;
    case NTP_MODE_CLIENT:
      {
        /* client mode. */
        return "client";
      }
      break;
    case NTP_MODE_SERVER:
      {
        /* server mode. */
        return "server";
      }
      break;
    case NTP_MODE_BROADCAST:
      {
        /* broadcast mode. */
        return "broadcast";
      }
      break;
    case NTP_MODE_CONTROL:
      {
        /* control mode packet. */
        return "control";
      }
    case NTP_MODE_PRIVATE:
      {
        /* implementation defined function. */
        return "private";
      }
      break;
    case NTP_MODE_BCLIENT:
      {
        /* broadcast client. */
        return "broadcast";
      }
      break;
    default:
      return "unspec";
    }

  return "unspec";
}

/* Map codes from NTP to Cisco. */
static char
imi_ntp_map_codes (char c)
{

  switch (c)
    {
    case 'x':
    case '.':
    case '-':
      return '#';
    case ' ':
      return ' ';
    case '+':
      return '-';
    case '#':
      return '+';
    case '*':
      return '*';
    case 'o':
      return '*';
    default:
      return ' ';
    }

  return ' ';
}

struct ntp_master *
imi_ntp_master_create ()
{
  struct ntp_master *ntpm;

  ntpm = XCALLOC (MTYPE_IMI_NTP_MASTER, sizeof(struct ntp_master));

  if (!ntpm)
    return NULL;

  /* Initialize NTP neighbor list. */
  ntpm->ntp_neighbor_list = list_new ();

  /* Initialize NTP key list. */
  ntpm->ntp_key_list = list_new ();

  /* Initialize NTP trusted key list. */
  ntpm->ntp_trusted_key_list = list_new ();

  return ntpm;
}

void
imi_ntp_master_free (struct ntp_master **ntpm)
{
  if (*ntpm)
    {
      /* Free NTP neighbor list. */
      list_free ((*ntpm)->ntp_neighbor_list);

      /* Free NTP key list. */
      list_free ((*ntpm)->ntp_key_list);

      /* Free NTP trusted key list. */
      list_free ((*ntpm)->ntp_trusted_key_list);

      /* Free NTP master structure. */
      XFREE (MTYPE_IMI_NTP_MASTER, *ntpm);
      *ntpm = NULL;
    }

  return;
}

struct ntp_key *
imi_ntp_key_create ()
{
  struct ntp_key *key;

  key = XCALLOC (MTYPE_IMI_NTP_AUTH_KEY, sizeof (struct ntp_key));
  if (!key)
    return NULL;

  return key;
}

void
imi_ntp_key_free (struct ntp_key **key)
{
  if (*key)
    {
      XFREE (MTYPE_IMI_NTP_AUTH_KEY, *key);
      *key = NULL;
    }

  return;
}

struct ntp_neighbor *
imi_ntp_neighbor_create ()
{
  struct ntp_neighbor *neighbor;

  neighbor = XCALLOC (MTYPE_IMI_NTP_NEIGHBOR, sizeof (struct ntp_neighbor));
  if (!neighbor)
    return NULL;

  return neighbor;
}

void
imi_ntp_neighbor_free (struct ntp_neighbor **nbr)
{
  if (*nbr)
    {
      /* Free name. */
      XFREE (MTYPE_TMP, (*nbr)->name);

      /* Free NTP neighbor. */
      XFREE (MTYPE_IMI_NTP_NEIGHBOR, *nbr);
      *nbr = NULL;
    }

  return;
}

/* Do DNS resolution. 0 for success, < 0 for failure. */
static int
ntp_resolve_name_by_dns (struct cli *cli, int family, char *host)
{
  int ret;
  struct pal_addrinfo *res = NULL, *res_h = NULL, dhints;
  void *ptr = NULL;
  char addrstr[INET_NTOP_BUFSIZ+1];

  if (family != AF_INET
#ifdef HAVE_IPV6
      && family != AF_INET6
#endif /* HAVE_IPV6 */
      )
    {
      cli_out(cli, "Bad address family value %d", family);
      return -1;
    }

  memset (&dhints, 0, sizeof(struct addrinfo));
  dhints.ai_family = family;
  dhints.ai_flags |= AI_CANONNAME;
  ret = pal_sock_getaddrinfo (host, NULL, &dhints, &res);

  if (ret != 0)
    {
      /* pal_sock_getaddrinfo failed */
      if (ret != EAI_SYSTEM)
        {
          /* Encode into string the getaddrinfo specific error. */
          cli_out (cli, "getaddrinfo error: %s", pal_gai_strerror(ret));
        }
      else
        {
          /* Encode into string the "errno" error. */
          cli_out (cli, "getaddrinfo system error: %s", pal_strerror(errno));
        }
      ret = -1;
    }
  else
    {
      res_h = res;
      while (res)
        {
          pal_inet_ntop (res->ai_family, res->ai_addr->sa_data, addrstr,
                         INET_NTOP_BUFSIZ);
          switch (res->ai_family)
            {
              case AF_INET:
                ptr = &((struct pal_sockaddr_in4 *) res->ai_addr)->sin_addr;
                break;
              case AF_INET6:
              #ifdef HAVE_IPV6
                ptr = &((struct pal_sockaddr_in6 *) res->ai_addr)->sin6_addr;
              #endif /*HAVE_IPV6*/
                break;
            }
          if (ptr)
            {
              pal_inet_ntop (res->ai_family, ptr, addrstr, INET_NTOP_BUFSIZ);
              cli_out (cli, "Translating IPv%d address: %s \"%s\" ... OK \n",
                       res->ai_family == PF_INET6 ? 6 : 4, addrstr, 
                       (res->ai_canonname ? res->ai_canonname : " "));
          }

          res = res->ai_next;
        }
      pal_sock_freeaddrinfo (res_h);
      ret = 0;
    }
  return (ret);
}

/* Check if key exists. Return 1 if exists otherwise 0. */
int
imi_ntp_authentication_key_exists (struct list *ntp_key_list,
                                   enum ntp_key_type type,
                                   ntp_auth_key_num key_num,
                                   char *key)
{
  struct listnode *node;
  struct ntp_key *k;

  IMI_ASSERT (ntp_key_list != NULL);
  IMI_ASSERT (key != NULL);

  LIST_LOOP (ntp_key_list, k, node)
    {
      if ((k->type == type)
          && (k->key_num == key_num))
        return 1;
    }

  return 0;
}

int
imi_ntp_authentication_key_add (struct list *ntp_key_list, enum ntp_key_type type,
                                ntp_auth_key_num key_num, char *key)
{
  struct ntp_key *ntp_key;

  IMI_ASSERT (ntp_key_list != NULL);
  IMI_ASSERT (key != NULL);

  ntp_key = imi_ntp_key_create ();
  if (!ntp_key)
    return NTP_ERROR_KEY_ADD;

  /* Initialize key. */
  ntp_key->type = type;
  ntp_key->key_num = key_num;
  ntp_key->key = XSTRDUP (MTYPE_TMP, key);

  /* Add key to list. */
  listnode_add (ntp_key_list, ntp_key);

  return 0;
}

int
imi_ntp_set_authentication_key (struct ntp_master *ntpm,
                                enum ntp_key_type type, ntp_auth_key_num key_num,
                                char *key)
{
  int ret;

  /* Check if key exists. */
  ret = imi_ntp_authentication_key_exists (ntpm->ntp_key_list, type, key_num, key);
  if (ret)
    return NTP_ERROR_KEY_EXISTS;

  /* Add key. */
  ret = imi_ntp_authentication_key_add (ntpm->ntp_key_list, type, key_num, key);
  if (ret < 0)
    return NTP_ERROR_KEY_ADD;

  /* Add key to PAL. */
  ret = pal_ntp_refresh_keys (imim, ntpm->ntp_key_list);
  if (ret < 0)
    return NTP_ERROR_KEY_ADD;

  return 0;
}

int
imi_ntp_unset_authentication_key (struct ntp_master *ntpm,
                                  enum ntp_key_type type, ntp_auth_key_num key_num,
                                  char *key)
{
  struct listnode *node, *node_next;
  struct ntp_key *k;
  int ret;

  for (node = LISTHEAD (ntpm->ntp_key_list); node; node = node_next)
    {
      node_next = node->next;

      k = GETDATA (node);

      if ((k->type == type)
          && (k->key_num == key_num)
          && (!pal_strcmp (k->key, key)))
        {
          /* Free key. */
          imi_ntp_key_free (&k);

          /* Free listnode. */
          list_delete_node (ntpm->ntp_key_list, node);

          /* Add key to PAL. */
          ret = pal_ntp_refresh_keys (imim, ntpm->ntp_key_list);
          if (ret < 0)
            return NTP_ERROR_KEY_DEL;
          return 0;
        }
    }

  return NTP_ERROR_KEY_DEL;
}

/* Check if trusted key exists. Return 1 if exists otherwise 0. */
int
imi_ntp_trusted_key_exists (struct list *ntp_trusted_key_list,
                            ntp_auth_key_num key_num)
{
  struct listnode *node;
  ntp_auth_key_num *k;

  IMI_ASSERT (ntp_trusted_key_list != NULL);

  LIST_LOOP (ntp_trusted_key_list, k, node)
    {
      if (*k == key_num)
        return 1;
    }

  return 0;
}

int
imi_trusted_key_add (struct list *ntp_trusted_key_list,
                     ntp_auth_key_num key_num)
{
  ntp_auth_key_num *k;

  IMI_ASSERT (ntp_trusted_key_list != NULL);

  k = XCALLOC (MTYPE_IMI_NTP_TRUSTED_KEY, sizeof (ntp_auth_key_num));

  /* Initialize key. */
  *k = key_num;

  /* Add key to list. */
  listnode_add (ntp_trusted_key_list, k);

  return 0;
}

int
imi_ntp_set_trusted_key (struct ntp_master *ntpm,
                         ntp_auth_key_num key_num)
{
  int ret;

  /* Check if trusted key exists. */
  ret = imi_ntp_trusted_key_exists (ntpm->ntp_trusted_key_list, key_num);
  if (ret)
    return NTP_ERROR_KEY_EXISTS;

  /* Add trusted key. */
  ret = imi_trusted_key_add (ntpm->ntp_trusted_key_list, key_num);
  if (ret < 0)
    return NTP_ERROR_KEY_ADD;

  /* Refresh the NTP config. */
  ret = pal_ntp_refresh_config(imim, ntpm);
  if (ret != RESULT_OK)
    return NTP_ERROR_NTP_CONFIG;

  return 0;
}

int
imi_ntp_unset_trusted_key (struct ntp_master *ntpm,
                           ntp_auth_key_num key_num)
{
  struct listnode *node, *node_next;
  ntp_auth_key_num *k;
  int ret;

  for (node = LISTHEAD (ntpm->ntp_trusted_key_list); node; node = node_next)
    {
      node_next = node->next;

      k = GETDATA (node);

      if (*k == key_num)
        {
          /* Free key. */
          XFREE (MTYPE_IMI_NTP_TRUSTED_KEY, k);

          /* Free listnode. */
          list_delete_node (ntpm->ntp_trusted_key_list, node);

          /* Refresh the NTP config. */
          ret = pal_ntp_refresh_config(imim, ntpm);
          if (ret != RESULT_OK)
            return NTP_ERROR_NTP_CONFIG;

          return 0;
        }
    }

  return NTP_ERROR_KEY_DEL;
}

/* Check if NTP neighbor exists. Return 1 if exists otherwize 0. */
int
imi_ntp_neighbor_exists (struct list *ntp_neighbor_list,
                         char *neighbor, enum ntp_preference prefer,
                         ntp_auth_key_num key_num)
{
  struct listnode *node;
  struct ntp_neighbor *n;

  IMI_ASSERT (ntp_neighbor_list != NULL);
  IMI_ASSERT (neighbor != NULL);

  LIST_LOOP (ntp_neighbor_list, n, node)
    {
      if ((n->key_num == key_num)
          && (n->preference == prefer)
          && (!pal_strcmp (n->name, neighbor)))
        return 1;
    }

  return 0;
}

/* Set neighbor. */
int
imi_ntp_neighbor_set (struct ntp_master *ntpm, enum ntp_nbr_type type,
                      char *neighbor,  ntp_auth_key_num key_num,
                      enum ntp_preference preference,
                      enum ntp_version version)
{
  struct listnode *node;
  struct ntp_neighbor *n;
  int ret;

  /* Check of neighbor exists. If yes, overwrite with new parameters. */
  LIST_LOOP (ntpm->ntp_neighbor_list, n, node)
    {
      if ((n->type == type)
          && (!pal_strcmp (n->name, neighbor)))
        {
          n->key_num = key_num;
          n->preference = preference;
          n->version = version;

          /* Refresh the NTP config. */
          ret = pal_ntp_refresh_config(imim, ntpm);
          if (ret != RESULT_OK)
            return NTP_ERROR_NTP_CONFIG;

          return 0;
        }
    }

  /* Create new neighbor. */
  n = imi_ntp_neighbor_create ();
  if (!n)
    return NTP_ERROR_NBR_ADD;

  n->type = type;
  n->name = XSTRDUP (MTYPE_TMP, neighbor);
  n->key_num = key_num;
  n->preference = preference;
  n->version = version;

  /* Add to list. */
  listnode_add (ntpm->ntp_neighbor_list, (void *)n);

  /* Refresh the NTP config. */
  ret = pal_ntp_refresh_config(imim, ntpm);
  if (ret != RESULT_OK)
    return NTP_ERROR_NTP_CONFIG;

  return 0;
}

int
imi_ntp_neighbor_unset (struct ntp_master *ntpm, enum ntp_nbr_type type,
                        char *neighbor)
{
  struct listnode *node, *node_next;
  struct ntp_neighbor *nbr;
  int ret;

  for (node = LISTHEAD (ntpm->ntp_neighbor_list); node; node = node_next)
    {
      node_next = node->next;

      nbr = GETDATA (node);

      if ((nbr->type == type)
          && (!pal_strcmp (nbr->name, neighbor)))
        {
          /* Free neighbor. */
          imi_ntp_neighbor_free (&nbr);

          /* Free listnode. */
          list_delete_node (ntpm->ntp_neighbor_list, node);

          /* Refresh the NTP config. */
          ret = pal_ntp_refresh_config(imim, ntpm);
          if (ret != RESULT_OK)
            return NTP_ERROR_NTP_CONFIG;

          return 0;
        }
    }

  return NTP_ERROR_NBR_DEL;
}

int
imi_ntp_set_authentication (struct ntp_master *ntpm)
{
  int ret;

  IMI_ASSERT (ntpm != NULL);

  if (!ntpm->authentication)
    ntpm->authentication = NTP_AUTHENTICATE;
  else
    return NTP_ERROR_AUTH_SET;

  /* Refresh the NTP config. */
  ret = pal_ntp_refresh_config(imim, ntpm);
  if (ret != RESULT_OK)
    return NTP_ERROR_NTP_CONFIG;

  return 0;
}

int
imi_ntp_unset_authentication (struct ntp_master *ntpm)
{
  int ret;

  IMI_ASSERT (ntpm != NULL);

  if (ntpm->authentication)
    ntpm->authentication = 0;
  else
    return NTP_ERROR_AUTH_NOTSET;

  /* Refresh the NTP config. */
  ret = pal_ntp_refresh_config(imim, ntpm);
  if (ret != RESULT_OK)
    return NTP_ERROR_NTP_CONFIG;

  return 0;
}

int
imi_ntp_broadcastdelay_set (struct ntp_master *ntpm, u_int32_t bdelay)
{
  int ret;

  IMI_ASSERT (ntpm != NULL);

  ntpm->broadcastdelay = bdelay;

  /* Refresh the NTP config. */
  ret = pal_ntp_refresh_config(imim, ntpm);
  if (ret != RESULT_OK)
    return NTP_ERROR_NTP_CONFIG;

  return 0;
}

int
imi_ntp_master_clock_set (struct ntp_master *ntpm, u_char stratum)
{
  int ret;

  IMI_ASSERT (ntpm != NULL);

  ntpm->master_clock_flag = NTP_MASTER_CLOCK;
  ntpm->stratum = stratum;

  /* Refresh the NTP config. */
  ret = pal_ntp_refresh_config(imim, ntpm);
  if (ret != RESULT_OK)
    return NTP_ERROR_NTP_CONFIG;

  return 0;
}

int
imi_ntp_master_clock_unset (struct ntp_master *ntpm)
{
  int ret;

  IMI_ASSERT (ntpm != NULL);

  ntpm->master_clock_flag = 0;
  ntpm->stratum = NTP_STRATUM_ZERO;

  /* Refresh the NTP config. */
  ret = pal_ntp_refresh_config(imim, ntpm);
  if (ret != RESULT_OK)
    return NTP_ERROR_NTP_CONFIG;

  return 0;
}

int
imi_ntp_access_group_set (struct ntp_master *ntpm, enum ntp_access_group_type type,
                          u_int32_t acl_no)
{
  int ret;
  struct ntp_access_group *access;
  char name[sizeof (ntp_access_group_num)];

  IMI_ASSERT (ntpm != NULL);
  access = &ntpm->access;

  pal_sprintf (name, "%d", acl_no);

  switch (type)
    {
    case NTP_ACCESS_GROUP_PEER:
      access->peer = acl_no;
      break;
    case NTP_ACCESS_GROUP_QUERY_ONLY:
      access->query_only = acl_no;
      break;
    case NTP_ACCESS_GROUP_SERVE:
      access->serve = acl_no;
      break;
    case NTP_ACCESS_GROUP_SERVE_ONLY:
      access->serve_only = acl_no;
      break;
    default:
      return -1;
    }

  /* Refresh the NTP config. */
  if (access_list_lookup (imim->vr_in_cxt, AFI_IP, name))
    {
      ret = pal_ntp_refresh_config(imim, ntpm);
      if (ret != RESULT_OK)
        return NTP_ERROR_NTP_CONFIG;
    }

  return 0;
}

int
imi_ntp_access_group_unset (struct ntp_master *ntpm, enum ntp_access_group_type type)
{
  int ret;
  struct ntp_access_group *access;
  char name[sizeof (ntp_access_group_num)];
  bool_t ntp_restart = PAL_FALSE;

  IMI_ASSERT (ntpm != NULL);
  access = &ntpm->access;

  switch (type)
    {
    case NTP_ACCESS_GROUP_PEER:
      if (access->peer)
        {
          pal_sprintf (name, "%d", access->peer);
          if (access_list_lookup (imim->vr_in_cxt, AFI_IP, name))
            ntp_restart = PAL_TRUE;
          access->peer = 0;
        }
      break;
    case NTP_ACCESS_GROUP_QUERY_ONLY:
      if (access->query_only)
        {
          pal_sprintf (name, "%d", access->query_only);
          if (access_list_lookup (imim->vr_in_cxt, AFI_IP, name))
            ntp_restart = PAL_TRUE;
          access->query_only = 0;
        }
      break;
    case NTP_ACCESS_GROUP_SERVE:
      if (access->serve)
        {
          pal_sprintf (name, "%d", access->serve);
          if (access_list_lookup (imim->vr_in_cxt, AFI_IP, name))
            ntp_restart = PAL_TRUE;
          access->serve = 0;
        }
      break;
    case NTP_ACCESS_GROUP_SERVE_ONLY:
      if (access->serve_only)
        {
          pal_sprintf (name, "%d", access->serve_only);
          if (access_list_lookup (imim->vr_in_cxt, AFI_IP, name))
            ntp_restart = PAL_TRUE;
          access->serve_only = 0;
        }
      break;
    default:
      return -1;
    }

  /* Refresh the NTP config. */
  if (ntp_restart == PAL_TRUE)
    {
      ret = pal_ntp_refresh_config(imim, ntpm);
      if (ret != RESULT_OK)
        return NTP_ERROR_NTP_CONFIG;
    }
  return 0;
}

static int
imi_ntp_format_decimal (struct long_fp *fp, char *buf, int len, int ndec,
                        int msec)
{
  int negative = 0;
  u_char *cp, *cpend;
  u_int32_t lwork;
  int dec;
  u_char cbuf[24];
  u_char *cpdec;
  char *bp;

#if 0
  /* Is negative? */
  if ((fp->uintegral & 0x80000000) != 0)
    {
      negative = 1;
      LFP_NEGATE(*fp);
    }
#endif

  /* Zero the character buffer */
  memset((char *) cbuf, 0, sizeof(cbuf));

  /* Integral part. */
  cp = cpend = &cbuf[10];
  lwork = fp->uintegral;
  if (lwork & 0xffff0000)
    {
      u_int32_t lten = 10;
      u_int32_t ltmp;

      do
        {
          ltmp = lwork;
          lwork /= lten;
          ltmp -= (lwork << 3) + (lwork << 1);
          *--cp = (u_char)ltmp;
        } while (lwork & 0xffff0000);
    }

  if (lwork != 0)
    {
      u_int16_t sten = 10;
      u_int16_t stmp;
      u_int16_t swork = (u_int16_t)lwork;

      do
        {
          stmp = swork;
          swork /= sten;
          stmp -= (swork << 3) + (swork << 1);
          *--cp = (u_char)stmp;
        } while (swork != 0);
    }

  /* Fractional part. Find the number of decimal places.  */
  if (msec)
    {
      dec = ndec + 3;
      if (dec < 3)
        dec = 3;
      cpdec = &cbuf[13];
    }
  else
    {
      dec = ndec;
      if (dec < 0)
        dec = 0;
      cpdec = &cbuf[10];
    }

  if (dec > 12)
    dec = 12;

  /* If there's a fraction to deal with, do so. */
  if (fp->ufractional != 0)
    {
      struct long_fp work;

      work.uintegral = 0;
      work.ufractional = fp->ufractional;
      while (dec > 0)
        {
          struct long_fp ftmp;

          dec--;
          /* The scheme here is to multiply the fraction (0.1234...) by ten.
             This moves a junk of BCD into the units part. record that and
             iterate.  */
          work.uintegral = 0;
          LFP_LSHIFT(work);
          ftmp = work;
          LFP_LSHIFT(work);
          LFP_LSHIFT(work);
          LFP_ADD(work, ftmp);
          *cpend++ = (u_char)work.uintegral;
          if (work.ufractional == 0)
            break;
        }

      /* Rounding */
      if (work.ufractional & 0x80000000)
        {
          u_char *tp = cpend;

          *(--tp) += 1;
          while (*tp >= 10)
            {
              *tp = 0;
              *(--tp) += 1;
            };
          if (tp < cp)
            cp = tp;
        }
    }
  cpend += dec;

  while (cp < cpdec)
    {
      if (*cp != 0)
        break;
      cp++;
    }
  if (cp == cpdec)
    --cp;

  /* First check if length fits. */
  bp = buf;

#if 0
  if ((cpend - cp) > len)
    {
      /* given length not enough for output buffer. */
      return -1;
    }
  else
#endif
    {
      bp = buf;
      if (negative)
        *bp++ = '-';
      while (cp < cpend)
        {
          if (cp == cpdec)
            *bp++ = '.';
          *bp++ = (char)(*cp++ + '0');
        }
      *bp = '\0';
    }

  return 0;
}

static int
imi_ntp_format_date (struct long_fp *fp, char *date, int len)
{
  struct pal_tm tm;
  pal_time_t sec;
  u_int32_t msec;

  sec = fp->uintegral - JAN_1970;
  msec = fp->ufractional / 4294967l; /* fractional / (2 ** 32)/1000) */

  pal_time_gmt (&sec, &tm);

  pal_snprintf (date, len, "%08lx.%08lx (%2d:%2d:%2d.%03u UTC %s %s %2d %4d)",
                (unsigned long)fp->uintegral, (unsigned long)fp->ufractional,
                tm.tm_hour, tm.tm_min, tm.tm_sec, msec,
                ntp_day[tm.tm_wday], ntp_month[tm.tm_mon],
                tm.tm_mday, 1900 + tm.tm_year);

  return 0;
}

int
imi_ntp_show_status (struct cli *cli, struct ntp_master *ntpm)
{
  int ret;
  struct ntp_sys_stats *stats;

  ret = pal_ntp_update_status (&ntpm->stats);
  if (ret == 0)
    {
      char reftime[NTP_TIME_BUFFER_SZ], offset[NTP_TIME_BUFFER_SZ];
      char freq[NTP_TIME_BUFFER_SZ], rootdispersion[NTP_TIME_BUFFER_SZ];
      char rootdelay[NTP_TIME_BUFFER_SZ];

      stats = &ntpm->stats;

      /* Clock status. */
      cli_out (cli, "%s, ", imi_ntp_get_sync_status (stats->leap));

      /* Stratum. */
      cli_out (cli, "stratum %d, ", stats->stratum);

      /* Reference. */
      if (stats->refid)
        cli_out (cli, "reference is %s\n", stats->refid);

      /* Frequency. */
      imi_ntp_format_decimal(&stats->frequency, freq, NTP_TIME_BUFFER_SZ, 4, 0);
      cli_out (cli, "actual frequency is %s Hz, ", freq);

      /* Precision. */
      cli_out (cli, "precision is 2**%d\n", stats->precision);

      /* Reference time. */
      imi_ntp_format_date(&stats->reftime, reftime, NTP_TIME_BUFFER_SZ);
      cli_out (cli, "reference time is %s\n", reftime);

      /* clock offset. */
      imi_ntp_format_decimal(&stats->offset, offset, NTP_TIME_BUFFER_SZ, 3, 0);
      cli_out (cli, "clock offset is %s msec, ", offset);

      /* Root delay. */
      imi_ntp_format_decimal(&stats->rootdelay, rootdelay, NTP_TIME_BUFFER_SZ, 3, 0);
      cli_out (cli, "root delay is %s msec\n", rootdelay);

      /* Root dispersion. */
      imi_ntp_format_decimal(&stats->rootdispersion, rootdispersion, NTP_TIME_BUFFER_SZ, 3, 1);
      cli_out (cli, "root dispersion is %s msec, \n", rootdispersion);
    }

  return CLI_SUCCESS;
}

/* Free peer statistics. */
static int
imi_ntp_free_peer_stats (struct ntp_all_peer_stats *peerstats)
{
  int i;
  struct ntp_peer_stats *stats;

  pal_assert (peerstats != NULL);

  for (i = 0; i < peerstats->num; i++)
    {
      stats = &peerstats->stats[i];

      if (stats->srcadr)
        {
          XFREE (MTYPE_TMP, stats->srcadr);
          stats->srcadr = NULL;
        }
      if (stats->dstadr)
        {
          XFREE (MTYPE_TMP, stats->dstadr);
          stats->dstadr = NULL;
        }
      if (stats->refid)
        {
          XFREE (MTYPE_TMP, stats->refid);
          stats->refid = NULL;
        }
    }

  XFREE (MTYPE_TMP, peerstats->stats);
  peerstats->stats = NULL;
  peerstats->num = 0;

  return 0;
}

int
imi_ntp_show_detail_associations (struct cli *cli, struct ntp_master *ntpm)
{
  int ret;
  struct ntp_peer_stats *stats;
  int i, j;
  char reftime[NTP_TIME_BUFFER_SZ], rootdelay[NTP_TIME_BUFFER_SZ];
  char rootdispersion[NTP_TIME_BUFFER_SZ];
  char org[NTP_TIME_BUFFER_SZ], rec[NTP_TIME_BUFFER_SZ];
  char xmt[NTP_TIME_BUFFER_SZ];
  char offset[NTP_TIME_BUFFER_SZ], dispersion[NTP_TIME_BUFFER_SZ];
  char delay[NTP_TIME_BUFFER_SZ];
  char filtdelay[NTP_TIME_BUFFER_SZ], filtoffset[NTP_TIME_BUFFER_SZ];
  char filtdisp[NTP_TIME_BUFFER_SZ];
  u_char pstatus, leap;
  char c;

  /* First free old status. */
  imi_ntp_free_peer_stats (&ntpm->peerstats);

  ret = pal_ntp_update_associations (&ntpm->peerstats);
  if (ret == 0)
    {
      for (i = 0; i != ntpm->peerstats.num; i++)
        {
          stats = &ntpm->peerstats.stats[i];

          /* Status. */
          pstatus = (u_char) NTP_CONTROL_PEER_STATVAL(stats->status);

          /* Flash character. */
          c = ntp_flash [pstatus & 0x7];

          /* Address. */
          if (stats->srcadr)
            {
              cli_out (cli, "%s ", stats->srcadr);
              if (pstatus & NTP_CONTROL_PST_CONFIG)
                cli_out (cli, "configured, ");
              else
                cli_out (cli, "dynamic, ");
            }

          /* check for sanity. */
          if (stats->flash & NTP_TEST1
              || stats->flash & NTP_TEST2
              || stats->flash & NTP_TEST3
              || stats->flash & NTP_TEST4
              || stats->flash & NTP_TEST5
              || stats->flash & NTP_TEST6
              || stats->flash & NTP_TEST7
              || stats->flash & NTP_TEST8
              || stats->flash & NTP_TEST9
              || stats->flash & NTP_TEST10
              || stats->flash & NTP_TEST11)
            cli_out (cli, "insane, ");
          else
            cli_out (cli, "sane, ");

          /* check for validity. */
          if (stats->flash & NTP_TEST6
              || stats->flash & NTP_TEST7)
            cli_out (cli, "invalid, ");
          else
            cli_out (cli, "valid, ");

          if (c == '*')
            cli_out (cli, "our_master, ");
          else if (c == '+')
            cli_out (cli, "selected, ");
          else if (c == '-')
            cli_out (cli, "candidate, ");

          /* Leap. */
          leap = NTP_CONTROL_SYS_LI(stats->status);
          if (leap == NTP_LEAP_ADDSECOND)
            cli_out (cli, "leap_add, ");
          else if (leap == NTP_LEAP_DELSECOND)
            cli_out (cli, "leap_sub, ");
          else if (leap == NTP_LEAP_NOTINSYNC)
            cli_out (cli, "unsynced, ");

          /* Stratum. */
          cli_out (cli, "stratum %d\n", stats->stratum);

          /* Reference ID. */
          if (stats->refid)
            cli_out (cli, "ref ID %s, ", stats->refid);

          /* Reference time. */
          imi_ntp_format_date (&stats->reftime, reftime, NTP_TIME_BUFFER_SZ);
          cli_out (cli, "time %s\n", reftime);

          /* Local mode. */
          cli_out (cli, "our mode %s, ", imi_ntp_mode_str (stats->hmode));

          /* Peer mode. */
          cli_out (cli, "peer mode %s, ", imi_ntp_mode_str (stats->pmode));

          /* Local poll interval. */
          cli_out (cli, "our poll intvl %d, ", stats->hpoll);

          /* Peer poll interval. */
          cli_out (cli, "peer poll intvl %d\n", stats->ppoll);

          /* Root delay. */
          imi_ntp_format_decimal (&stats->rootdelay, rootdelay, NTP_TIME_BUFFER_SZ, 2, 0);
          cli_out (cli, "root delay %s msec, ", rootdelay);

          /* Root dispersion. */
          imi_ntp_format_decimal (&stats->rootdispersion, rootdispersion, NTP_TIME_BUFFER_SZ, 2, 1);
          cli_out (cli, "root disp %s, ", rootdispersion);

          /* Reach. */
          cli_out (cli, "reach %03lo, ", stats->reach);

          /* Peer sync distance?. */
          cli_out (cli, "\n");

          /* Delay. */
          imi_ntp_format_decimal (&stats->delay, delay, NTP_TIME_BUFFER_SZ, 2, 0);
          cli_out (cli, "delay %s msec, ", delay);

          /* Offset. */
          imi_ntp_format_decimal (&stats->offset, offset, NTP_TIME_BUFFER_SZ, 4, 0);
          cli_out (cli, "offset %s msec, ", offset);

          /* Dispersion. */
          imi_ntp_format_decimal (&stats->dispersion, dispersion, NTP_TIME_BUFFER_SZ, 2, 1);
          cli_out (cli, "dispersion %s\n", dispersion);

          /* Precision. */
          cli_out (cli, "precision 2**%d, ", stats->precision);

          /* Version?. */
          cli_out (cli, "\n");

          /* Originate time stamp. */
          imi_ntp_format_date (&stats->org, org, NTP_TIME_BUFFER_SZ);
          cli_out (cli, "org time %s\n", org);

          /* Receive time stamp. */
          imi_ntp_format_date (&stats->rec, rec, NTP_TIME_BUFFER_SZ);
          cli_out (cli, "rcv time %s\n", rec);

          /* Transmit time stamp. */
          imi_ntp_format_date (&stats->xmt, xmt, NTP_TIME_BUFFER_SZ);
          cli_out (cli, "xmt time %s\n", xmt);

          /* Delay shift register. */
          cli_out (cli, "filtdelay =  ");
          for (j = 0; j != 8; j++)
            {
              imi_ntp_format_decimal (&stats->filtdelay[j], filtdelay, NTP_TIME_BUFFER_SZ, 2, 0);
              cli_out (cli, "%s  ", filtdelay);
            }
          cli_out (cli, "\n");

          /* Delay offset register. */
          cli_out (cli, "filtoffset =  ");
          for (j = 0; j != 8; j++)
            {
              imi_ntp_format_decimal (&stats->filtoffset[j], filtoffset, NTP_TIME_BUFFER_SZ, 2, 0);
              cli_out (cli, "%s  ", filtoffset);
            }
          cli_out (cli, "\n");

          /* Dispersion shift register. */
          cli_out (cli, "filterror =  ");
          for (j = 0; j != 8; j++)
            {
              imi_ntp_format_decimal (&stats->filtdisp[j], filtdisp, NTP_TIME_BUFFER_SZ, 2, 0);
              cli_out (cli, "%s  ", filtdisp);
            }
          cli_out (cli, "\n");

          cli_out (cli, "\n");
        }
    }

  return 0;
}

int
imi_ntp_show_associations (struct cli *cli, struct ntp_master *ntpm)
{
  int ret, i;
  struct ntp_peer_stats *stats;
  char b[3];
  char delay[NTP_TIME_BUFFER_SZ], offset[NTP_TIME_BUFFER_SZ];
  char dispersion[NTP_TIME_BUFFER_SZ];
  char c;
  u_char pstatus;
  u_int32_t poll_sec;

  /* First free old status. */
  imi_ntp_free_peer_stats (&ntpm->peerstats);

  /* Header. */
  cli_out (cli, "      address         ref clock     st  when  poll reach  delay  offset    disp\n");

  /* Get all peer status. */
  ret = pal_ntp_update_associations (&ntpm->peerstats);
  if (ret == 0)
    {
      for (i = 0; i != ntpm->peerstats.num; i++)
        {
          stats = &ntpm->peerstats.stats[i];

          pstatus = (u_char) NTP_CONTROL_PEER_STATVAL(stats->status);

          /* Flash codes. */
          c = ntp_flash [pstatus & 0x7];
          cli_out (cli, "%c", imi_ntp_map_codes (c));
          if (pstatus & NTP_CONTROL_PST_CONFIG)
            cli_out (cli, "%c", '~');

          /* Address. */
          if (stats->srcadr)
            cli_out (cli, "%s      ", stats->srcadr);

          /* Reference clock. */
          if (stats->refid)
            cli_out (cli, "%s      ", stats->refid);

          /* Stratum. */
          cli_out (cli, "%u    ", stats->stratum);

          /* Time of last NTP packet received. */
          if (stats->when == 0)
            cli_out (cli, "-    ");
          else
            cli_out (cli, "%2d  ", stats->when);

          /* Remote poll interval. */
          poll_sec = 1 << max(min3(stats->ppoll, stats->hpoll, NTP_MAXPOLL), NTP_MINPOLL);
          cli_out (cli, "%u    ", poll_sec);

          /* Peer reachability. */
          b[0] = b[1] = '0';
          if (stats->reach & 0x2)
            b[0] = '1';
          if (stats->reach & 0x1)
            b[1] = '1';
          b[2] = '\0';
          cli_out (cli, "%03lo    ", stats->reach);

          /* Delay. */
          imi_ntp_format_decimal (&stats->delay, delay, NTP_TIME_BUFFER_SZ, 1, 0);
          cli_out (cli, "%s    ", delay);

          /* Offset. */
          imi_ntp_format_decimal (&stats->offset, offset, NTP_TIME_BUFFER_SZ, 1, 0);
          cli_out (cli, "%s   ", offset);

          /* Dispersion. */
          imi_ntp_format_decimal (&stats->dispersion, dispersion, NTP_TIME_BUFFER_SZ, 1, 1);
          cli_out (cli, "%s\n", dispersion);
        }
    }

  /* Trailer. */
  cli_out (cli, " * master (synced), # master (unsynced), + selected, - candidate, ~ configured");

  return 0;
}

CLI (ntp_authenticate,
     ntp_authenticate_cmd,
     "ntp authenticate",
     NTP_STR,
     NTP_AUTHENTICATE_STR)
{
  int ret;

  ret = imi_ntp_set_authentication (imi_ntp_master);
  if (ret == NTP_ERROR_AUTH_SET)
    {
      cli_out (cli, "%NTP Authentication already set\n");
      return CLI_ERROR;
    }
  else if (ret == NTP_ERROR_NTP_CONFIG)
    {
      cli_out (cli, "%NTP config error\n");
      return CLI_ERROR;
    }

  return CLI_SUCCESS;
}

CLI (no_ntp_authenticate,
     no_ntp_authenticate_cmd,
     "no ntp authenticate",
     CLI_NO_STR,
     NTP_STR,
     "Authenticate time sources")
{
  int ret;

  ret = imi_ntp_unset_authentication (imi_ntp_master);
  if (ret == NTP_ERROR_AUTH_NOTSET)
    {
      cli_out (cli, "%NTP authentication not set\n");
      return CLI_ERROR;
    }
  else if (ret == NTP_ERROR_NTP_CONFIG)
    {
      cli_out (cli, "%NTP config error\n");
      return CLI_ERROR;
    }

  return CLI_SUCCESS;
}

CLI (ntp_authentication_key,
     ntp_authentication_key_cmd,
     "ntp authentication-key <1-4294967295> md5 WORD",
     NTP_STR,
     "Authentication key for trusted time sources",
     "Key number",
     "MD5 authentication",
     "Authentication key")
{
  u_int32_t key_num;
  int ret;

  CLI_GET_INTEGER ("Authentication key", key_num, argv[0]);

  ret = imi_ntp_set_authentication_key (imi_ntp_master, NTP_KEY_MD5, key_num, argv[1]);
  switch (ret)
    {
    case NTP_ERROR_KEY_EXISTS:
      cli_out (cli, "%Authentication key %ld exists\n", key_num);
      return CLI_ERROR;
    case NTP_ERROR_KEY_ADD:
      cli_out (cli, "%Authentication key %ld addition failed\n", key_num);
      return CLI_ERROR;
    }

  return CLI_SUCCESS;
}

CLI (no_ntp_authentication_key,
     no_ntp_authentication_key_cmd,
     "no ntp authentication-key <1-4294967295> md5 WORD",
     CLI_NO_STR,
     NTP_STR,
     "Authentication key for trusted time sources",
     "Key number",
     "MD5 authentication",
     "Authentication key")
{
  u_int32_t key_num;
  int ret;

  CLI_GET_INTEGER ("Authentication key", key_num, argv[0]);

  ret = imi_ntp_unset_authentication_key (imi_ntp_master, NTP_KEY_MD5, key_num, argv[1]);
  if (ret == NTP_ERROR_KEY_DEL)
    cli_out (cli, "%Authentication key %ld not found\n", key_num);

  return CLI_SUCCESS;
}

CLI (ntp_trusted_key,
     ntp_trusted_key_cmd,
     "ntp trusted-key <1-4294967295>",
     NTP_STR,
     "Key numbers for trusted time sources",
     "Key number")
{
  u_int32_t key;
  int ret;

  CLI_GET_INTEGER ("Trusted key", key, argv[0]);

  ret = imi_ntp_set_trusted_key (imi_ntp_master, key);
  switch (ret)
    {
    case NTP_ERROR_KEY_EXISTS:
      cli_out (cli, "%Trusted key %ld exists\n", key);
      return CLI_ERROR;
    case NTP_ERROR_KEY_ADD:
    case NTP_ERROR_NTP_CONFIG:
      cli_out (cli, "%Trusted key %ld addition failed\n", key);
      return CLI_ERROR;
    }

  return CLI_SUCCESS;
}

CLI (no_ntp_trusted_key,
     no_ntp_trusted_key_cmd,
     "no ntp trusted-key <1-4294967295>",
     CLI_NO_STR,
     NTP_STR,
     "Key numbers for trusted time sources",
     "Key number")
{
  u_int32_t key;
  int ret;

  CLI_GET_INTEGER ("Trusted key", key, argv[0]);

  ret = imi_ntp_unset_trusted_key (imi_ntp_master, key);
  switch (ret)
    {
    case NTP_ERROR_NTP_CONFIG:
      cli_out (cli, "%Trusted key %ld removal error\n", key);
      return CLI_ERROR;
    case NTP_ERROR_KEY_DEL:
      cli_out (cli, "%Trusted key %ld not found\n", key);
      return CLI_ERROR;
    }

  return CLI_SUCCESS;
}

CLI (ntp_peer,
     ntp_peer_cmd,
     "ntp peer WORD",
     NTP_STR,
     NTP_PEER_STR,
     "IP address of peer")
{
  int ret;

  ret = ntp_resolve_name_by_dns (cli, AF_INET, argv[0]);
  if (ret < 0)
    return CLI_ERROR;

  ret = imi_ntp_neighbor_set (imi_ntp_master, NTP_MODE_PEER, argv[0], 0, NTP_PREFERENCE_IGNORE, NTP_VERSION_IGNORE);
  if (ret == NTP_ERROR_NBR_ADD)
    {
      cli_out (cli, "%Error adding peer %s\n", argv[0]);
      return CLI_ERROR;
    }

  return CLI_SUCCESS;
}

CLI (no_ntp_peer,
     no_ntp_peer_cmd,
     "no ntp peer WORD",
     CLI_NO_STR,
     NTP_STR,
     NTP_PEER_STR,
     "IP address of peer")
{
  int ret;

  ret = imi_ntp_neighbor_unset (imi_ntp_master, NTP_MODE_PEER, argv[0]);
  switch (ret)
    {
    case NTP_ERROR_NBR_DEL:
    case NTP_ERROR_NBR_NOTEXISTS:
      cli_out (cli, "%NTP: unrecognized peer\n");
      return CLI_ERROR;
    }

  return CLI_SUCCESS;
}

CLI (ntp_peer_options,
     ntp_peer_options_cmd,
     "ntp peer WORD {prefer|version <1-4>|key <1-4294967295>}",
     NTP_STR,
     NTP_PEER_STR,
     "IP address of peer",
     "Prefer this peer when possible",
     "Configure NTP version",
     "NTP version number",
     "Configure peer authentication key",
     "Peer key number")
{
  int arg_cnt = 1;
  enum ntp_preference preference = NTP_PREFERENCE_IGNORE;
  enum ntp_version version = NTP_VERSION_IGNORE;
  ntp_auth_key_num key = 0;
  int ret;

  ret = ntp_resolve_name_by_dns (cli, AF_INET, argv[0]);
  if (ret < 0)
    return CLI_ERROR;

  while (arg_cnt < argc)
    {
      if (argv[arg_cnt][0] == 'p')
        {
          preference = NTP_PREFERENCE_YES;
          arg_cnt++;
        }
      else if (argv[arg_cnt][0] == 'v')
        {
          arg_cnt++;
          CLI_GET_INTEGER ("Version", version, argv[arg_cnt]);
          arg_cnt++;
        }
      else if (argv[arg_cnt][0] == 'k')
        {
          arg_cnt++;
          CLI_GET_INTEGER ("Key", key, argv[arg_cnt]);
          arg_cnt++;
        }
    }

  ret = imi_ntp_neighbor_set (imi_ntp_master, NTP_MODE_PEER, argv[0], key,
                              preference, version);
  if (ret == NTP_ERROR_NBR_ADD)
    {
      cli_out (cli, "Error adding peer %s\n", argv[0]);
      return CLI_ERROR;
    }

  return CLI_SUCCESS;
}

CLI (ntp_server,
     ntp_server_cmd,
     "ntp server WORD",
     NTP_STR,
     NTP_SERVER_STR,
     "IP address of server")
{
  int ret;

  ret = ntp_resolve_name_by_dns (cli, AF_INET, argv[0]);
  if (ret < 0)
    {
      cli_out (cli, "Bad IP Address or Host Name %s\n",argv[0]);
      return CLI_ERROR;
    }

  ret = imi_ntp_neighbor_set (imi_ntp_master, NTP_MODE_SERVER, argv[0], 0, NTP_PREFERENCE_IGNORE, NTP_VERSION_IGNORE);
  if (ret == NTP_ERROR_NBR_ADD)
    {
      cli_out (cli, "Error adding peer %s\n", argv[0]);
      return CLI_ERROR;
    }

  return CLI_SUCCESS;
}

CLI (no_ntp_server,
     no_ntp_server_cmd,
     "no ntp server WORD",
     CLI_NO_STR,
     NTP_STR,
     NTP_SERVER_STR,
     "IP address of server")
{
  int ret;

  ret = imi_ntp_neighbor_unset (imi_ntp_master, NTP_MODE_SERVER, argv[0]);
  switch (ret)
    {
    case NTP_ERROR_NBR_DEL:
    case NTP_ERROR_NBR_NOTEXISTS:
      cli_out (cli, "%NTP: unrecognized server\n");
      return CLI_ERROR;
    }

  return CLI_SUCCESS;
}

CLI (ntp_server_options,
     ntp_server_options_cmd,
     "ntp server WORD {prefer|version <1-4>|key <1-4294967295>}",
     NTP_STR,
     NTP_SERVER_STR,
     "IP address of peer",
     "Prefer this peer when possible",
     "Configure NTP version",
     "NTP version number",
     "Configure peer authentication key",
     "Peer key number")
{
  int arg_cnt = 1;
  enum ntp_preference preference = NTP_PREFERENCE_IGNORE;
  enum ntp_version version = NTP_VERSION_IGNORE;
  ntp_auth_key_num key = 0;
  int ret;

  ret = ntp_resolve_name_by_dns (cli, AF_INET, argv[0]);
  if (ret < 0)
    return CLI_ERROR;

  while (arg_cnt < argc)
    {
      if (argv[arg_cnt][0] == 'p')
        {
          preference = NTP_PREFERENCE_YES;
          arg_cnt++;
        }
      else if (argv[arg_cnt][0] == 'v')
        {
          arg_cnt++;
          CLI_GET_INTEGER ("Version", version, argv[arg_cnt]);
          arg_cnt++;
        }
      else if (argv[arg_cnt][0] == 'k')
        {
          arg_cnt++;
          CLI_GET_INTEGER ("Key", key, argv[arg_cnt]);
          arg_cnt++;
        }
    }

  ret = imi_ntp_neighbor_set (imi_ntp_master, NTP_MODE_SERVER, argv[0], key,
                              preference, version);
  if (ret == NTP_ERROR_NBR_ADD)
    {
      cli_out (cli, "Error adding peer %s\n", argv[0]);
      return CLI_ERROR;
    }

  return CLI_SUCCESS;
}

CLI (ntp_broadcastdelay,
     ntp_broadcastdelay_cmd,
     "ntp broadcastdelay <1-999999>",
     NTP_STR,
     "Estimated round-trip delay",
     "Round-trip delay in microseconds")
{
  u_int32_t bdelay;

  CLI_GET_INTEGER ("Broadcast delay", bdelay, argv[0]);

  imi_ntp_broadcastdelay_set (imi_ntp_master, bdelay);

  return CLI_SUCCESS;
}

CLI (no_ntp_broadcastdelay,
     no_ntp_broadcastdelay_cmd,
     "no ntp broadcastdelay",
     CLI_NO_STR,
     NTP_STR,
     "Estimated round-trip delay")
{
  imi_ntp_broadcastdelay_set (imi_ntp_master, 0);

  return CLI_SUCCESS;
}

CLI (ntp_master_clock,
     ntp_master_clock_cmd,
     "ntp master (<1-15>|)",
     NTP_STR,
     "Act as a NTP master clock",
     "Stratum number")
{
  int stratum = 0;

  if (argc == 1)
    CLI_GET_INTEGER ("Stratum number", stratum, argv[0]);

  imi_ntp_master_clock_set (imi_ntp_master, stratum);

  return CLI_SUCCESS;
}

CLI (no_ntp_master_clock,
     no_ntp_master_clock_cmd,
     "no ntp master",
     CLI_NO_STR,
     NTP_STR,
     "Act as a NTP master clock")
{
  imi_ntp_master_clock_unset (imi_ntp_master);

  return CLI_SUCCESS;
}

#define NTP_ACCESS_GROUP_GET_TYPE(A,B) \
   do { \
     if (pal_strncmp (A, "p", 1) == 0) \
       B = NTP_ACCESS_GROUP_PEER; \
     else if (pal_strncmp (A, "q", 1) == 0) \
       B = NTP_ACCESS_GROUP_QUERY_ONLY; \
     else if (pal_strncmp (A, "serve-only", 10) == 0) \
       B = NTP_ACCESS_GROUP_SERVE_ONLY; \
     else \
       B = NTP_ACCESS_GROUP_SERVE; \
   } while (0)


CLI (ntp_access_group,
     ntp_access_group_cmd,
     "ntp access-group (peer|query-only|serve|serve-only) (<1-99>|<1300-1999>)",
     NTP_STR,
     "Control NTP access",
     "Provide full access",
     "Allow only control queries",
     "Provide server and query access",
     "Provide only server access",
     "Standard IP access list",
     "Standard IP access list (expanded range)")
{
  enum ntp_access_group_type type;
  int acl_no;
  int ret;

  NTP_ACCESS_GROUP_GET_TYPE(argv[0], type);

  CLI_GET_INTEGER ("Access-list number", acl_no, argv[1]);

  ret = imi_ntp_access_group_set (imi_ntp_master, type, acl_no);

  return CLI_SUCCESS;
}

CLI (no_ntp_access_group,
     no_ntp_access_group_cmd,
     "no ntp access-group (peer|query-only|serve|serve-only)",
     CLI_NO_STR,
     NTP_STR,
     "Control NTP access",
     "Provide full access",
     "Allow only control queries",
     "Provide server and query access",
     "Provide only server access")
{
  enum ntp_access_group_type type;
  int ret;

  NTP_ACCESS_GROUP_GET_TYPE(argv[0], type);

  ret = imi_ntp_access_group_unset (imi_ntp_master, type);

  return CLI_SUCCESS;
}

CLI (show_ntp_status,
     show_ntp_status_cmd,
     "show ntp status",
     CLI_SHOW_STR,
     NTP_SHOW_STR,
     "NTP Status")
{
  imi_ntp_show_status (cli, imi_ntp_master);

  return CLI_SUCCESS;
}

CLI (show_ntp_associations,
     show_ntp_associations_cmd,
     "show ntp associations (|detail)",
     CLI_SHOW_STR,
     NTP_SHOW_STR,
     "NTP associations",
     "Show detail")
{
  if (argc == 1)
    imi_ntp_show_detail_associations (cli, imi_ntp_master);
  else
    imi_ntp_show_associations (cli, imi_ntp_master);
  return CLI_SUCCESS;
}

/* Compare NTP access-group list with string. */
int
ntp_acl_cmp_by_name (char *name)
{
  struct ntp_access_group *access = NULL;
  int ret = 0, num =0;

  /* Converting String to Integer. Because NTP access-group list is integer*/
  num = cmd_str2int(name, &ret);

  if (imi_ntp_master)
    {
      /*  NTP Access groups. */
      access = &imi_ntp_master->access;

      if (access->peer == num)
         return NTP_SUCCESS;

      if (access->query_only == num)
         return NTP_SUCCESS;

      if (access->serve == num)
         return NTP_SUCCESS;

      if (access->serve_only == num)
         return NTP_SUCCESS;
    }
  return NTP_FAILURE;
}

void
imi_ntp_acl_hook (struct access_list *access, struct filter_list *filter)
{
  int ret;

  IMI_ASSERT (access != NULL);
  if (filter == NULL)
    return;

  /* Only common <1-99> and <1300-1999> common access-lists allowed. */
  if (filter->common != FILTER_COMMON)
    return;

  ret = ntp_acl_cmp_by_name (access->name);

  /* Refresh the NTP config. */
  if (ret == NTP_SUCCESS)
    {
      ret = pal_ntp_refresh_config(imim, imi_ntp_master);
      if (ret != RESULT_OK)
        return;
    }
}

void
imi_ntp_acl_add_hook (struct access_list *access, struct filter_list *filter)
{
  imi_ntp_acl_hook (access, filter);
}

void
imi_ntp_acl_delete_hook (struct access_list *access, struct filter_list *filter)
{
  imi_ntp_acl_hook (access, filter);
}

int
imi_ntp_cli_init (struct lib_globals *zg)
{
  struct cli_tree *ctree = zg->ctree;

  cli_install_imi (ctree, CONFIG_MODE, PM_IMI, PRIVILEGE_NORMAL, 0,
                   &ntp_authenticate_cmd);
  cli_set_imi_cmd (&ntp_authenticate_cmd, CONFIG_MODE, CFG_DTYP_IMI_NTP);

  cli_install_imi (ctree, CONFIG_MODE, PM_IMI, PRIVILEGE_NORMAL, 0,
                   &no_ntp_authenticate_cmd);

  cli_install_imi (ctree, CONFIG_MODE, PM_IMI, PRIVILEGE_NORMAL, 0,
                   &ntp_authentication_key_cmd);
  cli_set_imi_cmd (&ntp_authentication_key_cmd, CONFIG_MODE, CFG_DTYP_IMI_NTP);

  cli_install_imi (ctree, CONFIG_MODE, PM_IMI, PRIVILEGE_NORMAL, 0,
                   &no_ntp_authentication_key_cmd);

  cli_install_imi (ctree, CONFIG_MODE, PM_IMI, PRIVILEGE_NORMAL, 0,
                   &ntp_trusted_key_cmd);
  cli_set_imi_cmd (&ntp_trusted_key_cmd, CONFIG_MODE, CFG_DTYP_IMI_NTP);

  cli_install_imi (ctree, CONFIG_MODE, PM_IMI, PRIVILEGE_NORMAL, 0,
                   &no_ntp_trusted_key_cmd);

  cli_install_imi (ctree, CONFIG_MODE, PM_IMI, PRIVILEGE_NORMAL, 0,
                   &ntp_peer_cmd);
  cli_set_imi_cmd (&ntp_peer_cmd, CONFIG_MODE, CFG_DTYP_IMI_NTP);

  cli_install_imi (ctree, CONFIG_MODE, PM_IMI, PRIVILEGE_NORMAL, 0,
                   &ntp_peer_options_cmd);
  cli_set_imi_cmd (&ntp_peer_options_cmd, CONFIG_MODE, CFG_DTYP_IMI_NTP);

  cli_install_imi (ctree, CONFIG_MODE, PM_IMI, PRIVILEGE_NORMAL, 0,
                   &no_ntp_peer_cmd);

  cli_install_imi (ctree, CONFIG_MODE, PM_IMI, PRIVILEGE_NORMAL, 0,
                   &ntp_server_cmd);
  cli_set_imi_cmd (&ntp_server_cmd, CONFIG_MODE, CFG_DTYP_IMI_NTP);

  cli_install_imi (ctree, CONFIG_MODE, PM_IMI, PRIVILEGE_NORMAL, 0,
                   &ntp_server_options_cmd);
  cli_set_imi_cmd (&ntp_server_options_cmd, CONFIG_MODE, CFG_DTYP_IMI_NTP);

  cli_install_imi (ctree, CONFIG_MODE, PM_IMI, PRIVILEGE_NORMAL, 0,
                   &no_ntp_server_cmd);

  cli_install_imi (ctree, CONFIG_MODE, PM_IMI, PRIVILEGE_NORMAL, 0,
                   &ntp_broadcastdelay_cmd);
  cli_set_imi_cmd (&ntp_broadcastdelay_cmd, CONFIG_MODE, CFG_DTYP_IMI_NTP);

  cli_install_imi (ctree, CONFIG_MODE, PM_IMI, PRIVILEGE_NORMAL, 0,
                   &no_ntp_broadcastdelay_cmd);

  cli_install_imi (ctree, CONFIG_MODE, PM_IMI, PRIVILEGE_NORMAL, 0,
                   &ntp_master_clock_cmd);
  cli_set_imi_cmd (&ntp_master_clock_cmd, CONFIG_MODE, CFG_DTYP_IMI_NTP);

  cli_install_imi (ctree, CONFIG_MODE, PM_IMI, PRIVILEGE_NORMAL, 0,
                   &no_ntp_master_clock_cmd);

  cli_install_imi (ctree, CONFIG_MODE, PM_IMI, PRIVILEGE_NORMAL, 0,
                   &ntp_access_group_cmd);
  cli_set_imi_cmd (&ntp_access_group_cmd, CONFIG_MODE, CFG_DTYP_IMI_NTP);

  cli_install_imi (ctree, CONFIG_MODE, PM_IMI, PRIVILEGE_NORMAL, 0,
                   &no_ntp_access_group_cmd);

  cli_install_imi (ctree, EXEC_MODE, PM_IMI, PRIVILEGE_NORMAL, 0,
                   &show_ntp_status_cmd);

  cli_install_imi (ctree, EXEC_MODE, PM_IMI, PRIVILEGE_NORMAL, 0,
                   &show_ntp_associations_cmd);

  return 0;
}

int
imi_ntp_init ()
{
  /* Create NTP master structure. */
  imi_ntp_master = imi_ntp_master_create ();
  if (!imi_ntp_master)
    return -1;

  /* Initialise NTP cli. */
  imi_ntp_cli_init (imim);

  /* Initialize NTP PAL component. */
  pal_ntp_init ();

  return 0;
}

int
imi_ntp_deinit ()
{
  /* Deinitialize NTP PAL component. */
  pal_ntp_deinit ();

  /* Free NTP master. */
  imi_ntp_master_free (&imi_ntp_master);

  return 0;
}

/* Config write. */
int
imi_ntp_config_encode (cfg_vect_t *cv)
{
  struct ntp_key *ntp_key;
  ntp_auth_key_num *trusted_key;
  struct listnode *nm;
  struct ntp_neighbor *neighbor;
  struct ntp_access_group *access;

  if (imi_ntp_master)
    {
      if (imi_ntp_master->authentication == NTP_AUTHENTICATE)
        cfg_vect_add_cmd (cv, "ntp authenticate\n");

      /* NTP keys. */
      LIST_LOOP(imi_ntp_master->ntp_key_list, ntp_key, nm)
        {
          cfg_vect_add_cmd (cv, "ntp authentication-key ");
          cfg_vect_add_cmd (cv, "%lu ", ntp_key->key_num);

          if (ntp_key->type == NTP_KEY_MD5)
            cfg_vect_add_cmd (cv, "md5 %s\n", ntp_key->key);
        }

      /* NTP Trusted keys. */
      LIST_LOOP(imi_ntp_master->ntp_trusted_key_list, trusted_key, nm)
        {
          cfg_vect_add_cmd (cv, "ntp trusted-key %lu\n", *trusted_key);
        }

      /* NTP Peers. */
      LIST_LOOP(imi_ntp_master->ntp_neighbor_list, neighbor, nm)
        {
          cfg_vect_add_cmd (cv, "ntp ");

          if (neighbor->type == NTP_MODE_SERVER)
            cfg_vect_add_cmd (cv, "server ");
          else if (neighbor->type == NTP_MODE_PEER)
            cfg_vect_add_cmd (cv, "peer ");

          cfg_vect_add_cmd (cv, "%s ", neighbor->name);

          if (neighbor->preference == NTP_PREFERENCE_YES)
            cfg_vect_add_cmd (cv, "prefer ");

          if (neighbor->version == NTP_VERSION_1)
            cfg_vect_add_cmd (cv, "version 1 ");
          else if (neighbor->version == NTP_VERSION_2)
            cfg_vect_add_cmd (cv, "version 2 ");
          else if (neighbor->version == NTP_VERSION_3)
            cfg_vect_add_cmd (cv, "version 3 ");
          else if (neighbor->version == NTP_VERSION_4)
            cfg_vect_add_cmd (cv, "version 4 ");

          if (neighbor->key_num)
            cfg_vect_add_cmd (cv, "key %lu\n", neighbor->key_num);
          else
            cfg_vect_add_cmd (cv, "\n");
        }

      /* NTP Broadcast delay. */
      if (imi_ntp_master->broadcastdelay)
        cfg_vect_add_cmd (cv, "ntp broadcastdelay %lu\n", imi_ntp_master->broadcastdelay);

      /* NTP Master clock. */
      if (imi_ntp_master->master_clock_flag == NTP_MASTER_CLOCK)
        {
          cfg_vect_add_cmd (cv, "ntp master ");

          if (imi_ntp_master->stratum != NTP_STRATUM_ZERO)
            cfg_vect_add_cmd (cv, "%d\n", imi_ntp_master->stratum);
          else
            cfg_vect_add_cmd (cv, "\n");
        }

      /* NTP Access groups. */
      access = &imi_ntp_master->access;

      if (access->peer)
        cfg_vect_add_cmd (cv, "ntp access-group peer %lu\n", access->peer);

      if (access->query_only)
        cfg_vect_add_cmd (cv, "ntp access-group query-only %lu\n", access->query_only);

      if (access->serve)
        cfg_vect_add_cmd (cv, "ntp access-group serve %lu\n", access->serve);

      if (access->serve_only)
        cfg_vect_add_cmd (cv, "ntp access-group serve-only %lu\n", access->serve_only);
    }

  return 0;
}

/* Config write. */
int
imi_ntp_config_write (struct cli *cli)
{
  cli->cv = cfg_vect_init(cli->cv);
  imi_ntp_config_encode(cli->cv);
  cfg_vect_out(cli->cv, (cfg_vect_out_fun_t)cli->out_func, cli->out_val);
  return 0;
}


#endif /* HAVE_NTP */

