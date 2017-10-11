/* Copyright (C) 2009-10  All Rights Reserved. */

#include "pal.h"
#include "lib.h"

#include "mpls_common.h"

/**@file  - mpls_util.c
 *
 * @brief - This file contains the utility functions that can be used
 * across modules 
 */


#ifdef HAVE_VCCV
/**@brief - Util function to get the CC type in use based on the local
 * and remote cc types 
 *
 * @param local_cctypes - CC types supported by us.
 *
 * @param remote_cctypes - CC types supported by the peer.
 *
 * @return CC type in use or CC_TYPE_NONE if none matches 
 */ 
u_int8_t
oam_util_get_cctype_in_use (u_int8_t local_cctypes, u_int8_t remote_cctypes)
{
  /* Order of checks below should be maintained */
  if (CHECK_FLAG (local_cctypes, CC_TYPE_1_BIT) && 
      CHECK_FLAG (remote_cctypes, CC_TYPE_1_BIT))
    return CC_TYPE_1;

  else if (CHECK_FLAG (local_cctypes, CC_TYPE_2_BIT) && 
      CHECK_FLAG (remote_cctypes, CC_TYPE_2_BIT))
    return CC_TYPE_2;

  else if (CHECK_FLAG (local_cctypes, CC_TYPE_3_BIT) && 
      CHECK_FLAG (remote_cctypes, CC_TYPE_3_BIT))
    return CC_TYPE_3;

  else
    return CC_TYPE_NONE;

}

/**@brief - Util function to get the BFD CV type in use based on the local
 * and remote cv types 
 *
 * @param local_cvtypes - CV types supported by us.
 *
 * @param remote_cvtypes - CV types supported by the peer
 *
 * @return CV type in use or CV_TYPE_NONE if none matches
 */

u_int8_t
oam_util_get_bfd_cvtype_in_use (u_int8_t local_cvtypes, u_int8_t remote_cvtypes)
{
  /* Order of checks below should be maintained */
  if (CHECK_FLAG (local_cvtypes, CV_TYPE_BFD_ACH_DET_SIG_BIT) &&
      CHECK_FLAG (remote_cvtypes, CV_TYPE_BFD_ACH_DET_SIG_BIT))
    return CV_TYPE_BFD_ACH_DET_SIG;

  else if (CHECK_FLAG (local_cvtypes, CV_TYPE_BFD_ACH_DET_BIT) &&
      CHECK_FLAG (remote_cvtypes, CV_TYPE_BFD_ACH_DET_BIT))
    return CV_TYPE_BFD_ACH_DET;

  else if (CHECK_FLAG (local_cvtypes, CV_TYPE_BFD_IPUDP_DET_SIG_BIT) &&
      CHECK_FLAG (remote_cvtypes, CV_TYPE_BFD_IPUDP_DET_SIG_BIT))
    return CV_TYPE_BFD_IPUDP_DET_SIG;

  else if (CHECK_FLAG (local_cvtypes, CV_TYPE_BFD_IPUDP_DET_BIT) &&
      CHECK_FLAG (remote_cvtypes, CV_TYPE_BFD_IPUDP_DET_BIT))
    return CV_TYPE_BFD_IPUDP_DET;

  return CV_TYPE_NONE;
}
/**@brief -  This function validates if the cc type on which packet is received
 * matches with the CC type in use.
 * 
 * @param cctype_inuse - CC Type in use
 * @param recvd_cctype - Received cctype bit
 *
 * @return  PAL_TRUE if valid, else PAL_FALSE
 */
bool_t
mpls_util_is_valid_cc_type (u_int8_t cctype_inuse, u_int8_t recvd_cctype)
{
  switch (cctype_inuse)
    {
    case CC_TYPE_1:
      if (CHECK_FLAG(recvd_cctype, CC_TYPE_1_BIT))
        return PAL_TRUE;
      break;
    case CC_TYPE_2:
      if (CHECK_FLAG(recvd_cctype, CC_TYPE_2_BIT))
        return PAL_TRUE;
      break;
    case CC_TYPE_3:
      if (CHECK_FLAG(recvd_cctype, CC_TYPE_3_BIT))
        return PAL_TRUE;
      break;
    }

  return PAL_FALSE;
}
#endif /* HAVE_VCCV */
