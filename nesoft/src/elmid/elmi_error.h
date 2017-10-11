/**@file elmi_error.h
 *  * * @brief  This file contains Error codes used for ELMI User
 *   *          interface error handling functionality.
 *    * **/
/* Copyright (C) 2010  All Rights Reserved. */

#ifndef _ELMI_ERRROS_H
#define _ELMI_ERRORS_H

/* ELMI Module Miscellaneous defines */
#define ELMI_NULL_STR                          " "

/* ELMI Module-wide Error Codes */
#define ELMI_ERR_NONE                           (0)
#define ELMI_ERR_BASE                           (-200)
#define ELMI_ERR_GENERIC                        (ELMI_ERR_BASE + 1)
#define ELMI_ERR_MIS_CONFIGURATION              (ELMI_ERR_BASE + 2)
#define ELMI_ERR_INVALID_VALUE                  (ELMI_ERR_BASE + 3)
#define ELMI_ERR_INVALID_INTERFACE              (ELMI_ERR_BASE + 4)
#define ELMI_ERR_BRIDGE_NOT_FOUND               (ELMI_ERR_BASE + 5)
#define ELMI_ERR_PORT_NOT_FOUND                 (ELMI_ERR_BASE + 6)
#define ELMI_ERR_NOT_PROVIDER_BRIDGE            (ELMI_ERR_BASE + 7)
#define ELMI_ERR_NOT_CUSTOMER_BRIDGE            (ELMI_ERR_BASE + 8)
#define ELMI_ERR_NOT_CE_PORT                    (ELMI_ERR_BASE + 9)
#define ELMI_ERR_WRONG_PORT_TYPE                (ELMI_ERR_BASE + 10)
#define ELMI_ERR_NO_VALID_CONFIG                (ELMI_ERR_BASE + 11)
#define ELMI_ERR_NO_SUCH_VALUE                  (ELMI_ERR_BASE + 12)
#define ELMI_ERR_PORT_ELMI_ENABLED              (ELMI_ERR_BASE + 13)
#define ELMI_ERR_MEMORY                         (ELMI_ERR_BASE + 14)
#define ELMI_ERR_NOT_ALLOWED_UNIC               (ELMI_ERR_BASE + 15)
#define ELMI_ERR_NOT_ALLOWED_UNIN               (ELMI_ERR_BASE + 16)
#define ELMI_ERR_ALREADY_ELMI_ENABLED           (ELMI_ERR_BASE + 17)
#define ELMI_ERR_INTERVAL_OUTOFBOUNDS           (ELMI_ERR_BASE + 18)
#define ELMI_ERR_ELMI_NOT_ENABLED               (ELMI_ERR_BASE + 19)
#define ELMI_ERR_EVC_STATUS                     (ELMI_ERR_BASE + 20)
#define ELMI_ERR_CEVLAN_EVC_MAP                 (ELMI_ERR_BASE + 21)
#define ELMI_ERR_NO_SUCH_INTERFACE              (ELMI_ERR_BASE + 22)
#define ELMI_ERR_WRONG_BRIDGE_TYPE              (ELMI_ERR_BASE + 23)
#define ELMI_ERR_INVALID_EVC_LEN                (ELMI_ERR_BASE + 24)
#define ELMI_ERR_NOT_CE_BRIDGE                  (ELMI_ERR_BASE + 25)

#endif /* _ELMI_ERRROS_H */
