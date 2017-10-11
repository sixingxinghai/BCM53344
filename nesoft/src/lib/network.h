/* Copyright (C) 2001-2003  All Rights Reserved. */

#ifndef _PACOS_NETWORK_H
#define _PACOS_NETWORK_H

#include "pal.h"

s_int32_t readn (pal_sock_handle_t, u_char *, s_int32_t);
s_int32_t writen (pal_sock_handle_t, u_char *, s_int32_t);

#endif /* _PACOS_NETWORK_H */
