/* Copyright (C) 2004  All Rights Reserved. */

#include "pal.h"
#include "copyright.h"
#include "version.h"
#include "snprintf.h"

/* Pass TRUE is '\n' is required at the end of the string. */
const char *
pacos_copyright (char *buf, int len)
{
  char tmp[50];
  zsnprintf (buf, len, " version %s %s %s",
             pacos_version (tmp, 50),
             PLATFORM,
             BUILDDATE);
  return buf;
}
