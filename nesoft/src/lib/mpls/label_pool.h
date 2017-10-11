/* Copyright (C) 2001-2003  All Rights Reserved. */

#ifndef _PACOS_LABEL_POOL_H
#define _PACOS_LABEL_POOL_H

struct stream;

#define LABEL_IPV4_EXP_NULL     0
#define LABEL_ROUTER_ALERT      1
#define LABEL_IPV6_EXP_NULL     2
#define LABEL_IMPLICIT_NULL     3
#define LABEL_UNINITIALIZED_VALUE 0

#if (defined HAVE_PACKET) || (defined HAVE_GMPLS)
#define LABEL_BUCKET_SIZE             640
#define PACKET_LABEL_BUCKET_SIZE      LABEL_BUCKET_SIZE
#endif
#ifdef HAVE_PORT_WAVELENGTH
#define WAVELENGTH_LABEL_BUCKET_SIZE  64
#endif
#ifdef HAVE_FREEFORM
#define FREEFORM_LABEL_BUCKET_SIZE    64
#endif
#ifdef HAVE_SONET
#define SONET_LABEL_BUCKET_SIZE       64
#endif
#ifdef HAVE_SDH
#define SDH_LABEL_BUCKET_SIZE         64
#endif
#ifdef HAVE_WAVEBAND
#define WAVEBAND_LABEL_BUCKET_SIZE    64
#endif
#ifdef HAVE_TDM
#define TDM_LABEL_BUCKET_SIZE         16
#endif

#define LABEL_VALUE_INVALID     1048576
#define LABEL_VALUE_INITIAL     16      /* 0 - 15 are reserved */
#define LABEL_VALUE_MAX         1048575
#define LABEL_BLOCK_INVALID     (LABEL_VALUE_MAX / LABEL_BUCKET_SIZE)
#define MAXIMUM_LABEL_SPACE     60000
#define MAX_LABEL_SPACE_BITLEN  16
#define MAX_LABEL_BITLEN        20

/* Label space types */
#define PLATFORM_WIDE_LABEL_SPACE            0
#define INTERFACE_SPECIFIC_LABEL_SPACE       1
#define PBB_TE_LABEL_SPACE                   50000

/* Flag a modified return value. */
#define LABEL_SPACE_DATA_MODIFIED            -2

/* Macro to figure out label space */
#define LABEL_SPACE_IS_PLATFORM_WIDE(l) ((l) == 0)

#define VALID_LABEL(l) (((l) <= LABEL_VALUE_MAX) && \
                       (((l) >= LABEL_VALUE_INITIAL) || \
                       (((l) == LABEL_IMPLICIT_NULL) || \
                       ((l) == LABEL_IPV4_EXP_NULL))))

/* Define number of service types per label range owner */
#define LP_RSVP_SERV_TYPE_MAX   1
#define LP_LDP_SERV_TYPE_MAX    3
#define LP_BGP_SERV_TYPE_MAX    1
#define LP_SERV_TYPE_MAX        LP_LDP_SERV_TYPE_MAX

#define LP_DATA_LEN_ZERO        0
#define LP_DATA_NULL            NULL
#define LABEL_POOL_RANGE_INITIAL  0     /* Start of label range index */
#define LABEL_POOL_RANGE_MAX      5     /* Total ranges as above */

/* Range based block allocation */
#define LABEL_RANGE_RETURN_DATA_NONE     0
#define LABEL_RANGE_BLOCK_ALLOC_SUCCESS  1

 /**@brief A list of different label types supported as part of GMPLS
   Label Management.*/
enum gmpls_label_type
{
#ifdef HAVE_PACKET
   GMPLS_LABEL_PACKET,
#endif
#ifdef HAVE_GMPLS
#ifdef HAVE_PBB_TE
   GMPLS_LABEL_PBB_TE,
#endif /* HAVE_PBB_TE */
#ifdef HAVE_PORT_WAVELENGTH
   GMPLS_LABEL_PORT_WLENGTH,
#endif /* HAVE_PORT_WAVELENGTH */
#ifdef HAVE_FREEFORM
   GMPLS_LABEL_FREEFORM,
#endif /* HAVE_FREEFORM */
#ifdef HAVE_SONET
   GMPLS_LABEL_SONET,
#endif /* HAVE_SONET */
#ifdef HAVE_SDH
   GMPLS_LABEL_SDH,
#endif /* HAVE_SDH */
#ifdef HAVE_WAVEBAND
   GMPLS_LABEL_WAVEBAND,
#endif /* HAVE_WAVEBAND */
#ifdef HAVE_TDM
   GMPLS_LABEL_TDM,
#endif /* HAVE_TDM */
#ifdef HAVE_LSC
   GMPLS_LABEL_LSC,
#endif /* HAVE_LSC */
#ifdef HAVE_FSC
   GMPLS_LABEL_FSC,
#endif /* HAVE_FSC */
#endif /* HAVE_GMPLS */
   GMPLS_LABEL_TYPE_MAX,

   GMPLS_LABEL_ERROR           = -1,
};

/* Label pool scope enum list. Ranges can be carved out of this for different
 * purposes */
enum label_pool_id
{
  /*******************************************************************/
  /* Define a set of bit fields to indicate the ownership of a       */
  /* label block based on the label range configured; these are used */
  /* in the label pool server/client block management.               */
  /*                                                                 */
  /* NOTE: NEW ENTRIES MUST BE ADDED AT THE END. VALUE ASSIGNED MUST */
  /*       NOT BE MODIFIED. See struct lp_client                     */
  /*                                                                 */
  /*******************************************************************/
  LABEL_POOL_SERVICE_ERROR           = -1,
  LABEL_POOL_SERVICE_UNSUPPORTED     = 0x00,
  LABEL_POOL_SERVICE_RSVP            = 0x01,
  LABEL_POOL_SERVICE_LDP             = 0x02,  /* LDP service 1 for LSPs */
  LABEL_POOL_SERVICE_LDP_LDP_PW      = 0x04,  /* LDP service 2 */
  LABEL_POOL_SERVICE_LDP_VPLS        = 0x08,  /* LDP service 3*/
  LABEL_POOL_SERVICE_BGP             = 0x10,

  /*************************************************************************/
  /* Label space return codes. MUST always be a negative value for errors. */
  /* Server : -10000 to -10999. Success codes : +10000 to +10999           */
  /*************************************************************************/
  LP_SERVER_SUCCESS                 = 10000,
  LP_SERVER_LSET_BLOCK_FOUND        = 10001,

  LP_SERVER_FAILURE                 = -10000,
  LP_SERVER_UNDEFINED_LBL_TYPE      = -10001,
  LP_SERVER_INVALID_RANGE_INDEX     = -10002,
  LP_SERVER_RANGE_BLOCK_FREE_ERROR  = -10003,
  LP_SERVER_LSET_BLOCK_NOT_FOUND    = -10004,

  /*******************************************************************/
  /* Client return Codes : errors -20000 to -20999.                  */
  /* Success codes : +20000 to +20999                                */
  /*******************************************************************/
  LP_CLIENT_SUCCESS                   = 0,
  LP_CLIENT_PKT_LBL_MATCH_FOUND       = LP_CLIENT_SUCCESS,
  LP_CLIENT_PKT_ACCEPT_LSET_SUCC      = LP_CLIENT_SUCCESS,

  LP_CLIENT_FAILURE                   = -20000,
  LP_CLIENT_DOESNT_EXIST              = -20001,
  LP_CLIENT_INVALID_SERVICE_TYPE      = -20002,
  LP_CLIENT_INVALID_RANGE_OWNER       = -20003,
  LP_CLIENT_LBL_SET_ERROR             = -20004,
  LP_CLIENT_PKT_LBL_MATCH_NOT_FOUND   = -20005,
  LP_CLIENT_PKT_INVALID_LSET_DATA     = -20006,
  LP_CLIENT_LBL_SET_MSG_SEND_ERROR    = -20007,
  LP_CLIENT_LBL_SET_LBL_ALLOC_ERROR   = -20008,
  LP_CLIENT_INVALID_LABEL_TYPE        = -20009,
  LP_CLIENT_PKT_ACCEPT_LSET_SEND_FAIL = -20010,
  LP_CLIENT_PKT_ACCEPT_LSET_FAIL      = -20011,
  LP_CLIENT_MEMORY_ALLOC_ERROR        = -20012,
  LP_CLIENT_LBL_REQ_BY_VAL_ERROR      = -20013,
  LP_CLIENT_BLOCK_LIST_GET_ERROR      = -20014,
  LP_CLIENT_LBL_SET_INVALID_LABELS    = -20015,
  LP_CLIENT_LBL_SET_OUT_OF_BOUND_LIST = -20016,
  LP_CLIENT_LBL_SET_LIST_DOESNT_FIT   = -20017,
  LP_CLIENT_LBL_SET_LBL_NOT_AVAIL     = -20018,
  LP_CLIENT_SUGG_LBL_ERROR            = -20019,
  LP_CLIENT_EXP_LBL_MATCH_NOT_FOUND   = -20020,
};

struct label_range_data
{
  u_int32_t from_label;
  u_int32_t to_label;
};

/* Label space data. */
struct label_space_data
{
#define LABEL_SPACE_INVALID                  0
#define LABEL_SPACE_VALID                    1
  u_char status;

  /* Two octet label space value. */
  u_int16_t label_space;

  /* Minmum and maximum label values */
  u_int32_t min_label_value;
  u_int32_t max_label_value;

 /* Maintain a list of disjoint label ranges on a per module basis. */
 struct label_range_data ls_module_ranges[LABEL_POOL_RANGE_MAX];

};

/* Prototypes */
void label_space_data_put (struct label_space_data *, struct stream **);
void label_space_data_get (struct label_space_data *, struct stream **);
int label_space_data_update (struct label_space_data *,
                             struct label_space_data *);
/* End Prototypes */

#endif /* _PACOS_LABEL_POOL_H */
