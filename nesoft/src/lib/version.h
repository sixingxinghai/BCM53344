/* Copyright (C) 2001-2004  All Rights Reserved. */

#ifndef _PACOS_VERSION_H
#define _PACOS_VERSION_H

#include "pal.h"
#include "copyright.h"

#define PACOS_MAJOR_RELEASE    "1"
#define PACOS_MINOR_RELEASE    "0"
#define PACOS_POINT_RELEASE    "0"
#define PACOS_PATCH_RELEASE    "0"

#ifndef PACOS_PLATFORM
#define PLATFORM               "iAPN Platform"
#else
#define PLATFORM               PACOS_PLATFORM
#endif

#ifndef PACOS_PLATFORM_ACRONYM
#define PLATFORM_ACRONYM       "APN"
#else
#define PLATFORM_ACRONYM       PACOS_PLATFORM_ACRONYM
#endif

#ifndef PACOS_CURR_RELEASE
#define CURR_RELEASE           ""
#else
#define CURR_RELEASE           PACOS_CURR_RELEASE
#endif

#define PACOS_BUG_ADDRESS      "support@pacbridge.net"

extern const char *host_name;

/* Check if current version is FCS or GA. */
#define CURR_VERSION_IS_FCS(c)                                             \
        ((pal_strcasecmp (c, "FCS") == 0 || pal_strcasecmp (c, "GA") == 0) \
         ? PAL_TRUE : PAL_FALSE)

/* Prototypes. */
const char *pacos_version (char *, int);
const char *pacos_buildno (char *, int);
void print_version (char *);

#endif /* _PACOS_VERSION_H */
