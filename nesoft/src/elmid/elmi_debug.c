/* Copyright (C) 2010  All Rights Reserved. */

#include "pal.h"
#include "elmid.h"
#include "elmi_debug.h"
#include "elmi_types.h"

void
elmi_debug_init (struct elmi_master *em)
{
  if (!em)
   return;
  /* Initialize debug variables. */
  em->debug.term.event = 0;
  em->debug.term.packet = 0;
  em->debug.term.timer = 0;
  em->debug.term.protocol = 0;
  em->debug.term.tx = 0;
  em->debug.term.rx = 0;
  em->debug.term.nsm = 0;

  em->debug.conf.event = 0;
  em->debug.conf.packet = 0;
  em->debug.conf.timer = 0;
  em->debug.conf.protocol = 0;
  em->debug.conf.tx = 0;
  em->debug.conf.rx = 0;
  em->debug.conf.nsm = 0;
}

