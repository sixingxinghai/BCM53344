/* Copyright (C) 2006 i All Rights Reserved. */
#include "pal.h"
#include "pal_firewall.h"
#ifdef HAVE_IPNET
#include "pal_ipfirewall.h"
#endif /* HAVE_IPNET */

/* Function to flush all rules */
s_int32_t
pal_firewall_flush_rules (void)
{
#ifdef HAVE_IPNET
  return pal_ipfirewall_flush_rules ();
#endif /* HAVE_IPNET */
  return RESULT_OK;
}

/* Fuction to show the rules from Kernel */
s_int32_t
pal_firewall_show_rules (s_int32_t group, struct pal_firewall_rule *rule)
{
#ifdef HAVE_IPNET
  return pal_ipfirewall_get_rules (group, rule);
#endif /* HAVE_IPNET */
  return RESULT_OK;
}

/* Function Called by NSM to add/remove a rule to ipfirewall kernel */
s_int32_t
pal_firewall_addremove_rule (struct pal_firewall_rule *rule, bool_t remove)
{
#ifdef HAVE_IPNET
  return pal_ipfirewall_addremove_rule (rule, remove);
#endif /* HAVE_IPNET */
  return RESULT_OK;
}

/* End of File */
