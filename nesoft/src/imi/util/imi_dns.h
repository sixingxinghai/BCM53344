/* Copyright (C) 2001-2003  All Rights Reserved. */

#ifndef _PACOS_IMI_DNS_H
#define _PACOS_IMI_DNS_H

/* Name string. */
#define DNS_STR         "Domain Name Service"

/* Global pointer to DNS structure. */
#define DNS             imig->dns

/* Data structures. */

/* DNS */
struct imi_dns
{
  /* On/off */
  bool_t enabled;

  /* Nameserver(s). */
  struct list *ns_list;

  /* Default domain. */
  char *default_domain;

  /* Domain search list. */
  struct list *domain_list;

  /* PAL. */
  pal_handle_t pal_dns;

  /* System configuration exists. */
  bool_t sysconfig;
  bool_t shutdown_flag;
};

/* Function prototypes: */

/* DNS Client Init. */
void imi_dns_init ();

/* DNS Client shutdown. */
void imi_dns_shutdown ();

/* DNS Cleint reset. */
result_t imi_dns_reset ();

/* DNS Client configuration write. */
struct cli;
int imi_dns_config_write (struct cli *cil);
int imi_dns_config_encode (struct imi_dns *dns, cfg_vect_t *cv);

#endif /* _PACOS_IMI_DNS_H */
