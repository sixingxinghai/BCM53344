/* Copyright (C) 2002-2003  All Rights Reserved. */

#ifndef _PACOS_OSPF_SHOW_H
#define _PACOS_OSPF_SHOW_H

#ifdef HAVE_NSSA
#define OSPF_LSA_TYPE_NSSA_DESC         "NSSA external link state"
#define OSPF_LSA_TYPE_NSSA_STR          "|nssa-external"
#else  /* HAVE_NSSA */
#define OSPF_LSA_TYPE_NSSA_DESC         ""
#define OSPF_LSA_TYPE_NSSA_STR          ""
#endif /* HAVE_NSSA */

#ifdef HAVE_OPAQUE_LSA
#define OSPF_LSA_TYPE_LINK_OPAQUE_DESC  "Link local Opaque-LSA"
#define OSPF_LSA_TYPE_AREA_OPAQUE_DESC  "Link area Opaque-LSA"
#define OSPF_LSA_TYPE_AS_OPAQUE_DESC    "Link AS Opaque-LSA"
#define OSPF_LSA_TYPE_OPAQUE_STR        "|opaque-link|opaque-area|opaque-as"
#else /* HAVE_OPAQUE_LSA */
#define OSPF_LSA_TYPE_LINK_OPAQUE_DESC  ""
#define OSPF_LSA_TYPE_AREA_OPAQUE_DESC  ""
#define OSPF_LSA_TYPE_AS_OPAQUE_DESC    ""
#define OSPF_LSA_TYPE_OPAQUE_STR        ""
#endif /* HAVE_OPAQUE_LSA */

#define OSPF_LSA_TYPES_STRING "asbr-summary|external|network|router|summary"  \
    OSPF_LSA_TYPE_NSSA_STR                                                    \
    OSPF_LSA_TYPE_OPAQUE_STR

#define OSPF_LSA_TYPES_DESC                                                   \
       "ASBR summary link states",                                            \
       "External link states",                                                \
       "Network link states",                                                 \
       "Router link states",                                                  \
       "Network summary link states",                                         \
       OSPF_LSA_TYPE_NSSA_DESC,                                               \
       OSPF_LSA_TYPE_LINK_OPAQUE_DESC,                                        \
       OSPF_LSA_TYPE_AREA_OPAQUE_DESC,                                        \
       OSPF_LSA_TYPE_AS_OPAQUE_DESC

void ospf_show_init (struct lib_globals *);

#endif /* _PACOS_OSPF_SHOW_H */
