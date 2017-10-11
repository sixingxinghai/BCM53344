/* Copyright (C) 2001-2003  All Rights Reserved. */

#include "pal.h"

#include "lib.h"
#include "keychain.h"

struct keychain *
keychain_new (void)
{
  struct keychain *new;

  new = XMALLOC (MTYPE_KEYCHAIN, sizeof (struct keychain));
  pal_mem_set (new, 0, sizeof (struct keychain));

  return new;
}

void
keychain_free (struct keychain *keychain)
{
  XFREE (MTYPE_KEYCHAIN, keychain);
}

struct key *
keychain_key_new (void)
{
  struct key *new;

  new = XMALLOC (MTYPE_KEY, sizeof (struct key));
  pal_mem_set (new, 0, sizeof (struct key));

  return new;
}

void
keychain_key_free (struct key *key)
{
  XFREE (MTYPE_KEY, key);
}

struct keychain *
keychain_lookup (struct apn_vr *vr, char *name)
{
  struct listnode *nn;
  struct keychain *keychain;

  if (name == NULL)
    return NULL;

  LIST_LOOP (vr->keychain_list, keychain, nn)
    {
      if (pal_strcmp (keychain->name, name) == 0)
        return keychain;
    }
  return NULL;
}

int
keychain_key_cmp_func (struct key *k1, struct key *k2)
{
  if (k1->index > k2->index)
    return 1;
  if (k1->index < k2->index)
    return -1;
  return 0;
}

void
keychain_key_delete_func (struct key *key)
{
  if (key->string)
    XFREE (MTYPE_KEY_STRING, key->string);
  keychain_key_free (key);
}

struct keychain *
keychain_get (struct apn_vr *vr, char *name)
{
  struct keychain *keychain;

  keychain = keychain_lookup (vr, name);

  if (keychain)
    return keychain;

  keychain = keychain_new ();
  keychain->name = XSTRDUP (MTYPE_KEYCHAIN_NAME,name);
  keychain->key = list_new ();
  keychain->key->cmp = (list_cmp_cb_t)keychain_key_cmp_func;
  keychain->key->del = (list_del_cb_t)keychain_key_delete_func;
  listnode_add (vr->keychain_list, keychain);

  return keychain;
}

void
keychain_delete (struct apn_vr *vr, struct keychain *keychain)
{
  if (keychain->name)
    XFREE (MTYPE_KEYCHAIN_NAME, keychain->name);

  list_delete (keychain->key);
  listnode_delete (vr->keychain_list, keychain);
  keychain_free (keychain);
}

struct key *
keychain_key_lookup (struct keychain *keychain, u_int32_t index)
{
  struct listnode *nn;
  struct key *key;

  LIST_LOOP (keychain->key, key, nn)
    {
      if (key->index == index)
        return key;
    }
  return NULL;
}

struct key *
keychain_key_lookup_for_accept (struct keychain *keychain, u_int32_t index)
{
  struct listnode *nn;
  struct key *key;
  pal_time_t now;

  now = pal_time_sys_current (NULL);

  LIST_LOOP (keychain->key, key, nn)
    {
      /* Only lookup the valid key. */
      if (key->index >= index && key->string)
        {
          if (key->accept.start == 0)
            return key;

          if (key->accept.start <= now)
            if (key->accept.end >= now || key->accept.end == -1)
              return key;
        }
    }
  return NULL;
}

struct key *
keychain_key_match_for_accept (struct keychain *keychain, char *auth_str)
{
  struct listnode *nn;
  struct key *key;
  pal_time_t now;

  now = pal_time_sys_current (NULL);

  LIST_LOOP (keychain->key, key, nn)
    {
      /* Skipped the key without key-string configuration. */
      if (key->string == NULL)
        continue;

      if (key->accept.start == 0 ||
          (key->accept.start <= now &&
           (key->accept.end >= now || key->accept.end == -1)))
        if (pal_strncmp (key->string, auth_str, 16) == 0)
          return key;
    }
  return NULL;
}

/* To check if atleast one key is configured for accepting*/
struct key *
keychain_key_for_accept (struct keychain *keychain)
{
  struct listnode *nn = NULL;
  struct key *key = NULL;
  pal_time_t now;

  if (!keychain)
    return NULL;

  now = pal_time_sys_current (NULL);

  LIST_LOOP (keychain->key, key, nn)
    {
      /* Skipped the key without key-string configuration. */
      if (key->string == NULL)
        continue;

      if (key->accept.start == 0 ||
          (key->accept.start <= now &&
           (key->accept.end >= now || key->accept.end == -1)))
        return key;
    }
  return NULL;
}

struct key *
keychain_key_lookup_for_send (struct keychain *keychain)
{
  struct listnode *nn;
  struct key *key;
  pal_time_t now;

  now = pal_time_sys_current (NULL);

  LIST_LOOP (keychain->key, key, nn)
    {
      /* skipped the key without key-string configuration. */
      if (key->string == NULL)
        continue;

      if (key->send.start == 0)
        return key;

      if (key->send.start <= now)
        if (key->send.end >= now || key->send.end == -1)
          return key;
    }
  return NULL;
}

struct key *
keychain_key_get (struct keychain *keychain, u_int32_t index)
{
  struct key *key;

  key = keychain_key_lookup (keychain, index);
  if (key)
    return key;

  key = keychain_key_new ();
  key->index = index;
  listnode_add_sort (keychain->key, key);

  return key;
}

void
keychain_key_delete (struct keychain *keychain, struct key *key)
{
  listnode_delete (keychain->key, key);

  if (key->string)
    XFREE (MTYPE_KEY_STRING, key->string);
  keychain_key_free (key);
}


CLI (keychain_key_chain,
     keychain_key_chain_cli,
     "key chain WORD",
     CLI_KEY_STR,
     CLI_CHAIN_STR,
     "Key-chain name")
{
  struct keychain *keychain;

  keychain = keychain_get (cli->vr, argv[0]);
  cli->index = keychain;
  cli->mode = KEYCHAIN_MODE;

  return CLI_SUCCESS;
}

CLI (no_keychain_key_chain,
     no_keychain_key_chain_cli,
     "no key chain WORD",
     CLI_NO_STR,
     CLI_KEY_STR,
     CLI_CHAIN_STR,
     "Key-chain name")
{
  struct keychain *keychain;

  keychain = keychain_lookup (cli->vr, argv[0]);

  if (! keychain)
    {
      cli_out (cli, "Can't find keychain %s\n", argv[0]);
      return CLI_ERROR;
    }

  keychain_delete (cli->vr, keychain);
  cli->mode = CONFIG_MODE;

  return CLI_SUCCESS;
}

CLI (keychain_key,
     keychain_key_cli,
     "key <0-2147483647>",
     "Configure a key",
     "Key identifier number")
{
  struct keychain *keychain;
  struct key *key;
  u_int32_t index;
  char *endptr = NULL;

  keychain = cli->index;

  index = pal_strtou32(argv[0], &endptr, 10);
  if (index == UINT32_MAX || *endptr != '\0')
    {
      cli_out (cli, "Key identifier number error\n");
      return CLI_ERROR;
    }
  key = keychain_key_get (keychain, index);
  cli->index_sub = key;
  cli->mode = KEYCHAIN_KEY_MODE;

  return CLI_SUCCESS;
}

CLI (no_keychain_key,
     no_keychain_key_cli,
     "no key <0-2147483647>",
     CLI_NO_STR,
     "Delete a key",
     "Key identifier number")
{
  struct keychain *keychain;
  struct key *key;
  u_int32_t index;
  char *endptr = NULL;

  keychain = cli->index;

  index = pal_strtou32 (argv[0], &endptr, 10);
  if (index == UINT32_MAX || *endptr != '\0')
    {
      cli_out (cli, "Key identifier number error\n");
      return CLI_ERROR;
    }

  key = keychain_key_lookup (keychain, index);
  if (! key)
    {
      cli_out (cli, "Can't find key %d\n", index);
      return CLI_ERROR;
    }

  keychain_key_delete (keychain, key);

  cli->mode = KEYCHAIN_MODE;

  return CLI_SUCCESS;
}

CLI (keychain_key_string,
     keychain_key_string_cli,
     "key-string LINE",
     CLI_KEYSTRING_STR,
     "The key")
{
  struct key *key;

  key = cli->index_sub;

  if (key->string)
    XFREE (MTYPE_KEY_STRING, key->string);
  key->string = XSTRDUP (MTYPE_KEY_STRING,argv[0]);

  return CLI_SUCCESS;
}

CLI (no_keychain_key_string,
     no_keychain_key_string_cli,
     "no key-string",
     CLI_NO_STR,
     CLI_KEYSTRING_STR)
{
  struct key *key;

  key = cli->index_sub;

  if (key->string)
    {
      XFREE (MTYPE_KEY_STRING, key->string);
      key->string = NULL;
    }

  return CLI_SUCCESS;
}

char *
keychain_key_str_tolower (char *str)
{
  int i;
  char *ptr = str;
  int len = pal_strlen (str);

  for (i = 0; i < len; i++, ptr++)
    if (pal_char_isalpha ((int) *ptr))
      *ptr = (char) pal_char_tolower ((int) *ptr);

  return str;
}

int
keychain_key_month_str2digit (char *month_str)
{
  int len = pal_strlen (month_str);
  char *str = keychain_key_str_tolower (month_str);

  /* More equal three charactors required. */
  if (len < 3)
    return -1;

  if (pal_strncmp ("january", str, len) == 0)
    return 0;
  else if (pal_strncmp ("february", str, len) == 0)
    return 1;
  else if (pal_strncmp ("march", str, len) == 0)
    return 2;
  else if (pal_strncmp ("april", str, len) == 0)
    return 3;
  else if (pal_strncmp ("may", str, len) == 0)
    return 4;
  else if (pal_strncmp ("june", str, len) == 0)
    return 5;
  else if (pal_strncmp ("july", str, len) == 0)
    return 6;
  else if (pal_strncmp ("august", str, len) == 0)
    return 7;
  else if (pal_strncmp ("september", str, len) == 0)
    return 8;
  else if (pal_strncmp ("october", str, len) == 0)
    return 9;
  else if (pal_strncmp ("november", str, len) == 0)
    return 10;
  else if (pal_strncmp ("december", str, len) == 0)
    return 11;
  else
    return -1;
}

/* Convert HH:MM:SS MON DAY YEAR to pal_time_t value.  -1 is returned when
   given string is malformed. */
pal_time_t
keychain_key_str2time (char *time_str, char *day_str,
                       char *month_str, char *year_str)
{
  char *colon;
  struct pal_tm tm;
  pal_time_t time;
  s_int32_t sec, min, hour;
  s_int32_t day, month, year;
  char *endptr = NULL;

  /* Check hour field of time_str. */
  colon = pal_strchr (time_str, ':');
  if (colon == NULL)
    return -1;
  *colon = '\0';

  /* Hour must be between 0 and 23. */
  hour = pal_strtou32(time_str, &endptr, 10);
  if (hour == UINT32_MAX || *endptr != '\0' || hour < 0 || hour > 23)
    return -1;

  /* Check min field of time_str. */
  time_str = colon + 1;
  colon = pal_strchr (time_str, ':');
  if (*time_str == '\0' || colon == NULL)
    return -1;
  *colon = '\0';

  /* Min must be between 0 and 59. */
  min = pal_strtou32(time_str, &endptr, 10);
  if (min == UINT32_MAX || *endptr != '\0' || min < 0 || min > 59)
    return -1;

  /* Check sec field of time_str. */
  time_str = colon + 1;
  if (*time_str == '\0')
    return -1;

  /* Sec must be between 0 and 59. */
  sec = pal_strtou32 (time_str, &endptr, 10);
  if (sec == UINT32_MAX || *endptr != '\0' || sec < 0 || sec > 59)
    return -1;

  /* Check day_str.  Day must be <1-31>. */
  day = pal_strtou32 (day_str, &endptr, 10);
  if (day == UINT32_MAX || *endptr != '\0' || day < 0 || day > 31)
    return -1;

  /* Check month_str. */
  if ((month = keychain_key_month_str2digit (month_str)) < 0)
    return -1;

  /* Check year_str.  Year must be <1993-2035>. */
  year = pal_strtou32 (year_str, &endptr, 10);
  if (year == UINT32_MAX || *endptr != '\0' || year < 1993 || year > 2035)
    return -1;

  pal_mem_set(&tm, 0, sizeof (struct pal_tm));
  tm.tm_sec = sec;
  tm.tm_min = min;
  tm.tm_hour = hour;
  tm.tm_mon = month;
  tm.tm_mday = day;
  tm.tm_year = year - 1900;
  tm.tm_isdst = -1;

  time = pal_time_mk (&tm);

  return time;
}

int
keychain_key_lifetime_set (struct cli *cli, struct key_range *krange,
                           char *stime_str, char *sday_str,
                           char *smonth_str, char *syear_str,
                           char *etime_str, char *eday_str,
                           char *emonth_str, char *eyear_str)
{
  pal_time_t time_start;
  pal_time_t time_end;

  time_start = keychain_key_str2time (stime_str, sday_str,
                                      smonth_str, syear_str);
  if (time_start < 0)
    {
      cli_out (cli, "Malformed time value\n");
      return CLI_ERROR;
    }

  time_end = keychain_key_str2time (etime_str, eday_str,
                                    emonth_str, eyear_str);
  if (time_end < 0)
    {
      cli_out (cli, "Malformed time value\n");
      return CLI_ERROR;
    }

  if (time_end <= time_start)
    {
      cli_out (cli, "Expire time is not later than start time\n");
      return CLI_ERROR;
    }

  krange->start = time_start;
  krange->end = time_end;

  return CLI_SUCCESS;
}

int
keychain_key_lifetime_duration_set (struct cli *cli, struct key_range *krange,
                                    char *stime_str, char *sday_str,
                                    char *smonth_str, char *syear_str,
                                    char *duration_str)
{
  pal_time_t time_start;
  u_int32_t duration;
  char *endptr = NULL;

  time_start = keychain_key_str2time (stime_str, sday_str,
                                      smonth_str, syear_str);
  if (time_start < 0)
    {
      cli_out (cli, "Malformed time value\n");
      return CLI_ERROR;
    }
  krange->start = time_start;

  duration = pal_strtou32 (duration_str, &endptr, 10);
  if (duration == UINT32_MAX || *endptr != '\0')
    {
      cli_out (cli, "Malformed duration\n");
      return CLI_ERROR;
    }
  krange->duration = 1;
  krange->end = time_start + duration;

  return CLI_SUCCESS;
}

int
keychain_key_lifetime_infinite_set (struct cli *cli, struct key_range *krange,
                                    char *stime_str, char *sday_str,
                                    char *smonth_str, char *syear_str)
{
  pal_time_t time_start;

  time_start = keychain_key_str2time (stime_str, sday_str,
                                      smonth_str, syear_str);
  if (time_start < 0)
    {
      cli_out (cli, "Malformed time value\n");
      return CLI_ERROR;
    }
  krange->start = time_start;

  krange->end = -1;

  return CLI_SUCCESS;
}

int
keychain_key_lifetime_unset (struct cli *cli, struct key_range *krange)
{
  krange->start = 0;
  krange->end = 0;
  krange->duration = 0;

  return CLI_SUCCESS;
}


#define KEYCHAIN_KEY_LIFETIME_DAY_MONTH_START_STR                             \
       "HH:MM:SS <1-31> MONTH <1993-2035> "
#define KEYCHAIN_KEY_LIFETIME_MONTH_DAY_START_STR                             \
       "HH:MM:SS MONTH <1-31> <1993-2035> "
#define KEYCHAIN_KEY_LIFETIME_DAY_MONTH_EXPIRE_STR                            \
       "HH:MM:SS <1-31> MONTH <1993-2035>"
#define KEYCHAIN_KEY_LIFETIME_MONTH_DAY_EXPIRE_STR                            \
       "HH:MM:SS MONTH <1-31> <1993-2035>"

#define KEYCHAIN_KEY_LIFETIME_DAY_MONTH_START_HELP_STR                        \
       "Time to start",                                                       \
       "Day of the month to start",                                           \
       "Month of the year to start (First three letters of the month)",       \
       "Year to start"
#define KEYCHAIN_KEY_LIFETIME_MONTH_DAY_START_HELP_STR                        \
       "Time to start",                                                       \
       "Month of the year to start (First three letters of the month)",       \
       "Day of the month to start",                                           \
       "Year to start"
#define KEYCHAIN_KEY_LIFETIME_DAY_MONTH_EXPIRE_HELP_STR                       \
       "Time to expire",                                                      \
       "Day of the month to expire",                                          \
       "Month of the year to expire (First three letters of the month)",      \
       "Year to start"
#define KEYCHAIN_KEY_LIFETIME_MONTH_DAY_EXPIRE_HELP_STR                       \
       "Time to expire",                                                      \
       "Month of the year to expire (First three letters of the month)",      \
       "Day of the month to expire",                                          \
       "Year to expire"

CLI (keychain_accept_lifetime_day_month_day_month,
     keychain_accept_lifetime_day_month_day_month_cli,
     "accept-lifetime "
     KEYCHAIN_KEY_LIFETIME_DAY_MONTH_START_STR
     KEYCHAIN_KEY_LIFETIME_DAY_MONTH_EXPIRE_STR,
     CLI_KEYCHAIN_ACCEPT_LIFETIME_STR,
     KEYCHAIN_KEY_LIFETIME_DAY_MONTH_START_HELP_STR,
     KEYCHAIN_KEY_LIFETIME_DAY_MONTH_EXPIRE_HELP_STR)
{
  struct key *key;

  key = cli->index_sub;

  return keychain_key_lifetime_set (cli, &key->accept,
                                    argv[0], argv[1], argv[2], argv[3],
                                    argv[4], argv[5], argv[6], argv[7]);
}

CLI (keychain_accept_lifetime_day_month_month_day,
     keychain_accept_lifetime_day_month_month_day_cli,
     "accept-lifetime "
     KEYCHAIN_KEY_LIFETIME_DAY_MONTH_START_STR
     KEYCHAIN_KEY_LIFETIME_MONTH_DAY_EXPIRE_STR,
     CLI_KEYCHAIN_ACCEPT_LIFETIME_STR,
     KEYCHAIN_KEY_LIFETIME_DAY_MONTH_START_HELP_STR,
     KEYCHAIN_KEY_LIFETIME_MONTH_DAY_EXPIRE_HELP_STR)
{
  struct key *key;

  key = cli->index_sub;

  return keychain_key_lifetime_set (cli, &key->accept,
                                    argv[0], argv[1], argv[2], argv[3],
                                    argv[4], argv[6], argv[5], argv[7]);
}

CLI (keychain_accept_lifetime_month_day_day_month,
     keychain_accept_lifetime_month_day_day_month_cli,
     "accept-lifetime "
     KEYCHAIN_KEY_LIFETIME_MONTH_DAY_START_STR
     KEYCHAIN_KEY_LIFETIME_DAY_MONTH_EXPIRE_STR,
     CLI_KEYCHAIN_ACCEPT_LIFETIME_STR,
     KEYCHAIN_KEY_LIFETIME_MONTH_DAY_START_HELP_STR,
     KEYCHAIN_KEY_LIFETIME_DAY_MONTH_EXPIRE_HELP_STR)
{
  struct key *key;

  key = cli->index_sub;

  return keychain_key_lifetime_set (cli, &key->accept,
                                    argv[0], argv[2], argv[1], argv[3],
                                    argv[4], argv[5], argv[6], argv[7]);
}

CLI (keychain_accept_lifetime_month_day_month_day,
     keychain_accept_lifetime_month_day_month_day_cli,
     "accept-lifetime "
     KEYCHAIN_KEY_LIFETIME_MONTH_DAY_START_STR
     KEYCHAIN_KEY_LIFETIME_MONTH_DAY_EXPIRE_STR,
     CLI_KEYCHAIN_ACCEPT_LIFETIME_STR,
     KEYCHAIN_KEY_LIFETIME_MONTH_DAY_START_HELP_STR,
     KEYCHAIN_KEY_LIFETIME_MONTH_DAY_EXPIRE_HELP_STR)
{
  struct key *key;

  key = cli->index_sub;

  return keychain_key_lifetime_set (cli, &key->accept,
                                    argv[0], argv[2], argv[1], argv[3],
                                    argv[4], argv[6], argv[5], argv[7]);
}

CLI (keychain_accept_lifetime_infinite_day_month,
     keychain_accept_lifetime_infinite_day_month_cli,
     "accept-lifetime "
     KEYCHAIN_KEY_LIFETIME_DAY_MONTH_START_STR
     "infinite",
     CLI_KEYCHAIN_ACCEPT_LIFETIME_STR,
     KEYCHAIN_KEY_LIFETIME_DAY_MONTH_START_HELP_STR,
     "Never expires")
{
  struct key *key;

  key = cli->index_sub;

  return keychain_key_lifetime_infinite_set (cli, &key->accept,
                                             argv[0], argv[1],
                                             argv[2], argv[3]);
}

CLI (keychain_accept_lifetime_infinite_month_day,
     keychain_accept_lifetime_infinite_month_day_cli,
     "accept-lifetime "
     KEYCHAIN_KEY_LIFETIME_MONTH_DAY_START_STR
     "infinite",
     CLI_KEYCHAIN_ACCEPT_LIFETIME_STR,
     KEYCHAIN_KEY_LIFETIME_MONTH_DAY_START_HELP_STR,
     "Never expires")
{
  struct key *key;

  key = cli->index_sub;

  return keychain_key_lifetime_infinite_set (cli, &key->accept,
                                             argv[0], argv[2],
                                             argv[1], argv[3]);
}

CLI (keychain_accept_lifetime_duration_day_month,
     keychain_accept_lifetime_duration_day_month_cli,
     "accept-lifetime "
     KEYCHAIN_KEY_LIFETIME_DAY_MONTH_START_STR
     "duration <1-2147483646>",
     CLI_KEYCHAIN_ACCEPT_LIFETIME_STR,
     KEYCHAIN_KEY_LIFETIME_DAY_MONTH_START_HELP_STR,
     "Duration of the key",
     "Duration seconds")
{
  struct key *key;

  key = cli->index_sub;

  return keychain_key_lifetime_duration_set (cli, &key->accept,
                                             argv[0], argv[1], argv[2],
                                             argv[3], argv[4]);
}

CLI (keychain_accept_lifetime_duration_month_day,
     keychain_accept_lifetime_duration_month_day_cli,
     "accept-lifetime "
     KEYCHAIN_KEY_LIFETIME_MONTH_DAY_START_STR
     "duration <1-2147483646>",
     CLI_KEYCHAIN_ACCEPT_LIFETIME_STR,
     KEYCHAIN_KEY_LIFETIME_MONTH_DAY_START_HELP_STR,
     "Duration of the key",
     "Duration seconds")
{
  struct key *key;

  key = cli->index_sub;

  return keychain_key_lifetime_duration_set (cli, &key->accept,
                                             argv[0], argv[2], argv[1],
                                             argv[3], argv[4]);
}

CLI (no_keychain_accept_lifetime,
     no_keychain_accept_lifetime_cli,
     "no accept-lifetime",
     CLI_NO_STR,
     CLI_KEYCHAIN_ACCEPT_LIFETIME_STR)
{
  struct key *key;

  key = cli->index_sub;

  return keychain_key_lifetime_unset (cli, &key->accept);
}


CLI (keychain_send_lifetime_day_month_day_month,
     keychain_send_lifetime_day_month_day_month_cli,
     "send-lifetime "
     KEYCHAIN_KEY_LIFETIME_DAY_MONTH_START_STR
     KEYCHAIN_KEY_LIFETIME_DAY_MONTH_EXPIRE_STR,
     CLI_KEYCHAIN_SEND_LIFETIME_STR,
     KEYCHAIN_KEY_LIFETIME_DAY_MONTH_START_HELP_STR,
     KEYCHAIN_KEY_LIFETIME_DAY_MONTH_EXPIRE_HELP_STR)
{
  struct key *key;

  key = cli->index_sub;

  return keychain_key_lifetime_set (cli, &key->send,
                                    argv[0], argv[1], argv[2], argv[3],
                                    argv[4], argv[5], argv[6], argv[7]);
}

CLI (keychain_send_lifetime_day_month_month_day,
     keychain_send_lifetime_day_month_month_day_cli,
     "send-lifetime "
     KEYCHAIN_KEY_LIFETIME_DAY_MONTH_START_STR
     KEYCHAIN_KEY_LIFETIME_MONTH_DAY_EXPIRE_STR,
     CLI_KEYCHAIN_SEND_LIFETIME_STR,
     KEYCHAIN_KEY_LIFETIME_DAY_MONTH_START_HELP_STR,
     KEYCHAIN_KEY_LIFETIME_MONTH_DAY_EXPIRE_HELP_STR)
{
  struct key *key;

  key = cli->index_sub;

  return keychain_key_lifetime_set (cli, &key->send,
                                    argv[0], argv[1], argv[2], argv[3],
                                    argv[4], argv[6], argv[5], argv[7]);
}

CLI (keychain_send_lifetime_month_day_day_month,
     keychain_send_lifetime_month_day_day_month_cli,
     "send-lifetime "
     KEYCHAIN_KEY_LIFETIME_MONTH_DAY_START_STR
     KEYCHAIN_KEY_LIFETIME_DAY_MONTH_EXPIRE_STR,
     CLI_KEYCHAIN_SEND_LIFETIME_STR,
     KEYCHAIN_KEY_LIFETIME_MONTH_DAY_START_HELP_STR,
     KEYCHAIN_KEY_LIFETIME_DAY_MONTH_EXPIRE_HELP_STR)
{
  struct key *key;

  key = cli->index_sub;

  return keychain_key_lifetime_set (cli, &key->send,
                                    argv[0], argv[2], argv[1], argv[3],
                                    argv[4], argv[5], argv[6], argv[7]);
}

CLI (keychain_send_lifetime_month_day_month_day,
     keychain_send_lifetime_month_day_month_day_cli,
     "send-lifetime "
     KEYCHAIN_KEY_LIFETIME_MONTH_DAY_START_STR
     KEYCHAIN_KEY_LIFETIME_MONTH_DAY_EXPIRE_STR,
     CLI_KEYCHAIN_SEND_LIFETIME_STR,
     KEYCHAIN_KEY_LIFETIME_MONTH_DAY_START_HELP_STR,
     KEYCHAIN_KEY_LIFETIME_MONTH_DAY_EXPIRE_HELP_STR)
{
  struct key *key;

  key = cli->index_sub;

  return keychain_key_lifetime_set (cli, &key->send,
                                    argv[0], argv[2], argv[1], argv[3],
                                    argv[4], argv[6], argv[5], argv[7]);
}

CLI (keychain_send_lifetime_infinite_day_month,
     keychain_send_lifetime_infinite_day_month_cli,
     "send-lifetime "
     KEYCHAIN_KEY_LIFETIME_DAY_MONTH_START_STR
     "infinite",
     CLI_KEYCHAIN_SEND_LIFETIME_STR,
     KEYCHAIN_KEY_LIFETIME_DAY_MONTH_START_HELP_STR,
     "Never expires")
{
  struct key *key;

  key = cli->index_sub;

  return keychain_key_lifetime_infinite_set (cli, &key->send,
                                             argv[0], argv[1],
                                             argv[2], argv[3]);
}

CLI (keychain_send_lifetime_infinite_month_day,
     keychain_send_lifetime_infinite_month_day_cli,
     "send-lifetime "
     KEYCHAIN_KEY_LIFETIME_MONTH_DAY_START_STR
     "infinite",
     CLI_KEYCHAIN_SEND_LIFETIME_STR,
     KEYCHAIN_KEY_LIFETIME_MONTH_DAY_START_HELP_STR,
     "Never expires")
{
  struct key *key;

  key = cli->index_sub;

  return keychain_key_lifetime_infinite_set (cli, &key->send,
                                             argv[0], argv[2],
                                             argv[1], argv[3]);
}

CLI (keychain_send_lifetime_duration_day_month,
     keychain_send_lifetime_duration_day_month_cli,
     "send-lifetime "
     KEYCHAIN_KEY_LIFETIME_DAY_MONTH_START_STR
     "duration <1-2147483646>",
     CLI_KEYCHAIN_SEND_LIFETIME_STR,
     KEYCHAIN_KEY_LIFETIME_DAY_MONTH_START_HELP_STR,
     "Duration of the key",
     "Duration seconds")
{
  struct key *key;

  key = cli->index_sub;

  return keychain_key_lifetime_duration_set (cli, &key->send,
                                             argv[0], argv[1], argv[2],
                                             argv[3], argv[4]);
}

CLI (keychain_send_lifetime_duration_month_day,
     keychain_send_lifetime_duration_month_day_cli,
     "send-lifetime "
     KEYCHAIN_KEY_LIFETIME_MONTH_DAY_START_STR
     "duration <1-2147483646>",
     CLI_KEYCHAIN_SEND_LIFETIME_STR,
     KEYCHAIN_KEY_LIFETIME_MONTH_DAY_START_HELP_STR,
     "Duration of the key",
     "Duration seconds")
{
  struct key *key;

  key = cli->index_sub;

  return keychain_key_lifetime_duration_set (cli, &key->send,
                                             argv[0], argv[2], argv[1],
                                             argv[3], argv[4]);
}

CLI (no_keychain_send_lifetime,
     no_keychain_send_lifetime_cli,
     "no send-lifetime",
     CLI_NO_STR,
     CLI_KEYCHAIN_SEND_LIFETIME_STR)
{
  struct key *key;

  key = cli->index_sub;

  return keychain_key_lifetime_unset (cli, &key->send);
}

int
keychain_strftime (char *buf, s_int32_t bufsiz, pal_time_t *time)
{
  struct pal_tm tm;
  size_t len;

  pal_time_loc (time,&tm);

  len = pal_time_strf (buf, bufsiz, "%T %b %d %Y", &tm);

  return len;
}

int
keychain_config_encode (struct apn_vr *vr, cfg_vect_t *cv)
{
  struct keychain *keychain;
  struct key *key;
  struct listnode *nn;
  struct listnode *nm;
  char buf[BUFSIZ];
  char buf2[BUFSIZ];

  LIST_LOOP (vr->keychain_list, keychain, nn)
    {
      cfg_vect_add_cmd (cv, "key chain %s\n", keychain->name);

      LIST_LOOP (keychain->key, key, nm)
        {
          cfg_vect_add_cmd (cv, " key %d\n", key->index);

          if (key->string)
            cfg_vect_add_cmd (cv, "  key-string %s\n", key->string);

          if (key->accept.start)
            {
              keychain_strftime (buf, BUFSIZ, &key->accept.start);

              if (key->accept.end == -1)
                cfg_vect_add_cmd (cv, "  accept-lifetime %s infinite\n", buf);
              else if (key->accept.duration)
                cfg_vect_add_cmd (cv, "  accept-lifetime %s duration %ld\n", buf,
                         key->accept.end - key->accept.start);
              else
                {
                  keychain_strftime (buf2, BUFSIZ, &key->accept.end);
                  cfg_vect_add_cmd (cv, "  accept-lifetime %s %s\n", buf, buf2);
                }
            }

          if (key->send.start)
            {
              keychain_strftime (buf, BUFSIZ, &key->send.start);

              if (key->send.end == -1)
                cfg_vect_add_cmd (cv, "  send-lifetime %s infinite\n", buf);
              else if (key->send.duration)
                cfg_vect_add_cmd (cv, "  send-lifetime %s duration %ld\n", buf,
                         key->send.end - key->send.start);
              else
                {
                  keychain_strftime (buf2, BUFSIZ, &key->send.end);
                  cfg_vect_add_cmd (cv, "  send-lifetime %s %s\n", buf, buf2);
                }
            }
        }
      cfg_vect_add_cmd (cv, "!\n");
    }

  return 0;
}

int
keychain_config_write (struct cli *cli)
{
#ifdef HAVE_IMI
  if (cli->zg->protocol != APN_PROTO_IMI)
    return 0;
#endif /* HAVE_IMI */

  cli->cv = cfg_vect_init(cli->cv);
  keychain_config_encode(cli->vr, cli->cv);
  cfg_vect_out(cli->cv, (cfg_vect_out_fun_t)cli->out_func, cli->out_val);
  return 0;
}


void
keychain_init (struct apn_vr *vr)
{
  vr->keychain_list = list_new ();
}

void
keychain_finish (struct apn_vr *vr)
{
  struct listnode *node, *next;

  for (node = LISTHEAD (vr->keychain_list); node; node = next)
    {
      next = node->next;

      /* Delete the key-chain.  */
      if (GETDATA (node) != NULL)
        keychain_delete (vr, GETDATA (node));
    }
  list_free (vr->keychain_list);
}

void
keychain_cli_init (struct lib_globals *zg)
{
  struct cli_tree *ctree = zg->ctree;

  cli_install_config (ctree, KEYCHAIN_MODE, keychain_config_write);

  cli_install_default (ctree, KEYCHAIN_MODE);
  cli_install_default (ctree, KEYCHAIN_KEY_MODE);

  CLI_INSTALL (zg, CONFIG_MODE, PM_KEYCHAIN,
               &keychain_key_chain_cli);
  cli_set_imi_cmd (&keychain_key_chain_cli, KEYCHAIN_MODE, CFG_DTYP_KEYCHAIN);

  CLI_INSTALL (zg, CONFIG_MODE, PM_KEYCHAIN,
               &no_keychain_key_chain_cli);

  CLI_INSTALL (zg, KEYCHAIN_MODE, PM_KEYCHAIN,
               &keychain_key_cli);
  cli_set_imi_cmd (&keychain_key_cli, KEYCHAIN_KEY_MODE, CFG_DTYP_KEYCHAIN);

  CLI_INSTALL (zg, KEYCHAIN_MODE, PM_KEYCHAIN,
               &no_keychain_key_cli);

  CLI_INSTALL (zg, KEYCHAIN_MODE, PM_KEYCHAIN,
               &keychain_key_chain_cli);
  cli_set_imi_cmd (&keychain_key_chain_cli, KEYCHAIN_MODE, CFG_DTYP_KEYCHAIN);

  CLI_INSTALL (zg, KEYCHAIN_MODE, PM_KEYCHAIN,
               &no_keychain_key_chain_cli);

  CLI_INSTALL (zg, KEYCHAIN_KEY_MODE, PM_KEYCHAIN,
               &keychain_key_string_cli);
  cli_set_imi_cmd (&keychain_key_string_cli, KEYCHAIN_KEY_MODE, CFG_DTYP_KEYCHAIN);

  CLI_INSTALL (zg, KEYCHAIN_KEY_MODE, PM_KEYCHAIN,
               &no_keychain_key_string_cli);

  CLI_INSTALL (zg, KEYCHAIN_KEY_MODE, PM_KEYCHAIN,
               &keychain_key_cli);
  cli_set_imi_cmd (&keychain_key_cli, KEYCHAIN_KEY_MODE, CFG_DTYP_KEYCHAIN);

  CLI_INSTALL (zg, KEYCHAIN_KEY_MODE, PM_KEYCHAIN,
               &no_keychain_key_cli);

  CLI_INSTALL (zg, KEYCHAIN_KEY_MODE, PM_KEYCHAIN,
               &keychain_accept_lifetime_day_month_day_month_cli);
  cli_set_imi_cmd (&keychain_accept_lifetime_day_month_day_month_cli, KEYCHAIN_KEY_MODE, CFG_DTYP_KEYCHAIN);

  CLI_INSTALL (zg, KEYCHAIN_KEY_MODE, PM_KEYCHAIN,
               &keychain_accept_lifetime_day_month_month_day_cli);
  cli_set_imi_cmd (&keychain_accept_lifetime_day_month_month_day_cli, KEYCHAIN_KEY_MODE, CFG_DTYP_KEYCHAIN);

  CLI_INSTALL (zg, KEYCHAIN_KEY_MODE, PM_KEYCHAIN,
               &keychain_accept_lifetime_month_day_day_month_cli);
  cli_set_imi_cmd (&keychain_accept_lifetime_month_day_day_month_cli, KEYCHAIN_KEY_MODE, CFG_DTYP_KEYCHAIN);

  CLI_INSTALL (zg, KEYCHAIN_KEY_MODE, PM_KEYCHAIN,
               &keychain_accept_lifetime_month_day_month_day_cli);
  cli_set_imi_cmd (&keychain_accept_lifetime_month_day_month_day_cli, KEYCHAIN_KEY_MODE, CFG_DTYP_KEYCHAIN);

  CLI_INSTALL (zg, KEYCHAIN_KEY_MODE, PM_KEYCHAIN,
               &keychain_accept_lifetime_infinite_day_month_cli);
  cli_set_imi_cmd (&keychain_accept_lifetime_infinite_day_month_cli, KEYCHAIN_KEY_MODE, CFG_DTYP_KEYCHAIN);

  CLI_INSTALL (zg, KEYCHAIN_KEY_MODE, PM_KEYCHAIN,
               &keychain_accept_lifetime_infinite_month_day_cli);
  cli_set_imi_cmd (&keychain_accept_lifetime_infinite_month_day_cli, KEYCHAIN_KEY_MODE, CFG_DTYP_KEYCHAIN);

  CLI_INSTALL (zg, KEYCHAIN_KEY_MODE, PM_KEYCHAIN,
               &keychain_accept_lifetime_duration_day_month_cli);
  cli_set_imi_cmd (&keychain_accept_lifetime_duration_day_month_cli, KEYCHAIN_KEY_MODE, CFG_DTYP_KEYCHAIN);

  CLI_INSTALL (zg, KEYCHAIN_KEY_MODE, PM_KEYCHAIN,
               &keychain_accept_lifetime_duration_month_day_cli);
  cli_set_imi_cmd (&keychain_accept_lifetime_duration_month_day_cli, KEYCHAIN_KEY_MODE, CFG_DTYP_KEYCHAIN);

  CLI_INSTALL (zg, KEYCHAIN_KEY_MODE, PM_KEYCHAIN,
               &no_keychain_accept_lifetime_cli);

  CLI_INSTALL (zg, KEYCHAIN_KEY_MODE, PM_KEYCHAIN,
               &keychain_send_lifetime_day_month_day_month_cli);
  cli_set_imi_cmd (&keychain_send_lifetime_day_month_day_month_cli, KEYCHAIN_KEY_MODE, CFG_DTYP_KEYCHAIN);

  CLI_INSTALL (zg, KEYCHAIN_KEY_MODE, PM_KEYCHAIN,
               &keychain_send_lifetime_day_month_month_day_cli);
  cli_set_imi_cmd (&keychain_send_lifetime_day_month_month_day_cli, KEYCHAIN_KEY_MODE, CFG_DTYP_KEYCHAIN);

  CLI_INSTALL (zg, KEYCHAIN_KEY_MODE, PM_KEYCHAIN,
               &keychain_send_lifetime_month_day_day_month_cli);
  cli_set_imi_cmd (&keychain_send_lifetime_month_day_day_month_cli, KEYCHAIN_KEY_MODE, CFG_DTYP_KEYCHAIN);

  CLI_INSTALL (zg, KEYCHAIN_KEY_MODE, PM_KEYCHAIN,
               &keychain_send_lifetime_month_day_month_day_cli);
  cli_set_imi_cmd (&keychain_send_lifetime_month_day_month_day_cli, KEYCHAIN_KEY_MODE, CFG_DTYP_KEYCHAIN);

  CLI_INSTALL (zg, KEYCHAIN_KEY_MODE, PM_KEYCHAIN,
               &keychain_send_lifetime_infinite_day_month_cli);
  cli_set_imi_cmd (&keychain_send_lifetime_infinite_day_month_cli, KEYCHAIN_KEY_MODE, CFG_DTYP_KEYCHAIN);

  CLI_INSTALL (zg, KEYCHAIN_KEY_MODE, PM_KEYCHAIN,
               &keychain_send_lifetime_infinite_month_day_cli);
  cli_set_imi_cmd (&keychain_send_lifetime_infinite_month_day_cli, KEYCHAIN_KEY_MODE, CFG_DTYP_KEYCHAIN);

  CLI_INSTALL (zg, KEYCHAIN_KEY_MODE, PM_KEYCHAIN,
               &keychain_send_lifetime_duration_day_month_cli);
  cli_set_imi_cmd (&keychain_send_lifetime_duration_day_month_cli, KEYCHAIN_KEY_MODE, CFG_DTYP_KEYCHAIN);

  CLI_INSTALL (zg, KEYCHAIN_KEY_MODE, PM_KEYCHAIN,
               &keychain_send_lifetime_duration_month_day_cli);
  cli_set_imi_cmd (&keychain_send_lifetime_duration_month_day_cli, KEYCHAIN_KEY_MODE, CFG_DTYP_KEYCHAIN);

  CLI_INSTALL (zg, KEYCHAIN_KEY_MODE, PM_KEYCHAIN,
               &no_keychain_send_lifetime_cli);
}
