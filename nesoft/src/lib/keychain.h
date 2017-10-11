/* Copyright (C) 2001-2003  All Rights Reserved. */

#ifndef _PACOS_KEYCHAIN_H
#define _PACOS_KEYCHAIN_H

#include "pal.h"
#include "linklist.h"

struct keychain
{
  char *name;

  struct list *key;
};

struct key_range
{
  pal_time_t start;
  pal_time_t end;

  u_int8_t duration;
};

struct key
{
  u_int32_t index;

  char *string;

  struct key_range send;
  struct key_range accept;
};


struct keychain *keychain_lookup (struct apn_vr *, char *);
struct key *keychain_key_lookup_for_accept (struct keychain *, u_int32_t);
struct key *keychain_key_match_for_accept (struct keychain *, char *);
struct key *keychain_key_for_accept (struct keychain *);
struct key *keychain_key_lookup_for_send (struct keychain *);

int keychain_config_write (struct cli *);

int keychain_config_encode (struct apn_vr *vr, cfg_vect_t *cv);

void keychain_init (struct apn_vr *);
void keychain_finish (struct apn_vr *);
void keychain_cli_init (struct lib_globals *);

#endif /* _PACOS_KEYCHAIN_H */
