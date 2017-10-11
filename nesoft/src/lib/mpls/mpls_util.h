/* Copyright (C) 2009-2010  All Rights Reserved. */

#ifndef _PACOS_LIB_MPLS_MPLS_UTIL_H
#define _PACOS_LIB_MPLS_MPLS_UTIL_H

#ifdef HAVE_VCCV
u_int8_t oam_util_get_cctype_in_use (u_int8_t, u_int8_t);
u_int8_t oam_util_get_bfd_cvtype_in_use (u_int8_t, u_int8_t);
bool_t mpls_util_is_valid_cc_type (u_int8_t, u_int8_t);
#endif /* HAVE_VCCV */

#endif /* _PACOS_LIB_MPLS_MPLS_UTIL_H */
