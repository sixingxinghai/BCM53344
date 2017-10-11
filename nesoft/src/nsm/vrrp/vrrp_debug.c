/* Copyright (C) 2002-2003  All Rights Reserved. */

#include "pal.h"
#include "lib.h"

#include "nsm/nsmd.h"
#include "nsm/nsm_debug.h"
#include "nsm/vrrp/vrrp.h"
#include "nsm/vrrp/vrrp_api.h"
#include "nsm/vrrp/vrrp_debug.h"

u_int32_t vrrp_term_debug;
u_int32_t vrrp_conf_debug;


void
vrrp_debug_init (void)
{
  vrrp_term_debug = 0;
  vrrp_conf_debug = 0;
}
