/* Copyright (C) 2001-2003  All Rights Reserved. */

#include "pal.h"
#include "version.h"
#include "copyright.h"
#include "snprintf.h"

/*
** host_name is a string containing the manufacturer's platform description.
** HOST_NAME is defined by the platform.
*/
const char *host_name = HOST_NAME;

/* PacOS Version. */
const char *
pacos_version (char *buf, int len)
{
  char *curr = CURR_RELEASE;

  if (CURR_VERSION_IS_FCS (curr))
    curr = NULL;

  zsnprintf (buf, len, "%s.%s.%s%s%s",
             PACOS_MAJOR_RELEASE,
             PACOS_MINOR_RELEASE,
             PACOS_POINT_RELEASE,
             curr && pal_strlen (curr) ? "." : "",
             curr ? curr : "");
  return buf;
}

/* PacOS BUILDNO. */
const char *
pacos_buildno (char *buf, int len)
{
  char *curr = CURR_RELEASE;
  char *acr = PLATFORM_ACRONYM;

  if (CURR_VERSION_IS_FCS (curr))
    curr = NULL;

  zsnprintf (buf, len, "PacOS-%s-%s-%s%s%s%s%s",
             PACOS_MAJOR_RELEASE,
             PACOS_MINOR_RELEASE,
             PACOS_POINT_RELEASE,
             curr && pal_strlen (curr) ? "-" : "",
             curr ? curr : "",
             pal_strlen (acr) ? "-" : "",
             acr);
  return buf;
}

/* Utility function to print out version for main() for PMs. */
void
print_version (char *progname_l)
{
  char buf[50];

  printf ("%s version %s (%s)\n", progname_l, pacos_version (buf, 50),
          host_name);
}
