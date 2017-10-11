/* Copyright (C) 2002-2003  All Rights Reserved. */

#ifndef _PACOS_PAL_BWMGR_H
#define _PACOS_PAL_BWMGR_H

#ifdef HAVE_GMPLS
/***************************************************************************
 \brief This API is used to derive the bw parameters of a TE link

 \param tel     Pointer to the TE link
 \param change  Pointer returned by the API specifies if there is a change in 
                the TE link property
 \return If the call is a success or failure 
***************************************************************************/
s_int32_t
apn_bw_derive_te_link (struct telink *tel, bool_t *change); 

#ifdef HAVE_PACKET
/***************************************************************************
 \brief This API is used to derive Max LSP Size parameters of a TE link

 \param tel     Pointer to the TE link
 \param change  Pointer returned by the API specifies if there is a change in 
                the TE link property
 \return If the call is a success or failure 
***************************************************************************/
s_int32_t
apn_bw_te_link_get_max_lsp_size (struct telink *tel, bool_t *change);
#endif /* HAVE_PACKET */

/***************************************************************************
 \brief This API is used to derive Max LSP Bandwidth parameters of a TE link
        at a particular priority

 \param tel     Pointer to the TE link
 \param prio    Priority for which to dervie the Max LSP Bandwidth
 \param change  Pointer returned by the API specifies if there is a change in
                the TE link property

 \return If the call is a success or failure
***************************************************************************/
s_int32_t
apn_bw_te_link_get_max_lsp_bw (struct telink *tel, u_char prio,
                               bool_t *change);

/***************************************************************************
 \brief This API is used to derive the parameters of a TE link
        given a datalink getting deleted

 \param tel     Pointer to the TE link
 \param dl      Pointer to the datalink that is getting deleted
 \param change  Pointer returned by the API specifies if there is a change in
                the TE link property

 \return If the call is a success or failure
***************************************************************************/

s_int32_t
apn_bw_del_te_link_change (struct telink *tel, struct datalink *dl,
                           bool_t *change);

/***************************************************************************
 \brief This API is used to derive the parameters of a TE link
        given a datalink getting added

 \param tel     Pointer to the TE link
 \param dl      Pointer to the datalink that is getting added
 \param change  Pointer returned by the API specifies if there is a change in
                the TE link property

 \return If the call is a success or failure
***************************************************************************/
s_int32_t
apn_bw_add_te_link_change (struct telink *tel, struct datalink *dl,
                           bool_t *change);

/***************************************************************************
 \brief This API is used to derive check if a bandwidth can be reserved on 
        any data link of a TE link
                              
 \param tel     Pointer to the TE link
 \param bw      The bandwidth to reserve on the TE link
 \param setup_prio  The setup priority of the request
                
 \return TRUE if the reservation is allowed FALSE otherwise
***************************************************************************/
bool_t
apn_bw_allow_resv_on_te_link (struct telink *tel, float32_t bw,
                              u_char setup_prio);

/***************************************************************************
 \brief This API is used to reserve bandwidth at a particular priority
        on a TE link
                              
 \param tel     Pointer to the TE link
 \param bw      The bandwidth to reserve on the TE link
 \param setup_prio  The setup priority of the request
 \param dlink   Pointer to the datalink that is returned by the API

 \return If the call is a success or failure
***************************************************************************/
s_int32_t
apn_bw_resv_on_te_link (struct telink *tel, float32_t bw, u_char setup_prio,
                        struct datalink *dlink);

/***************************************************************************
 \brief This API is used to update the property of a datalink that is created

 \param ifp     Pointer to the physical interface
 \param dlink   Pointer to the datalink that is returned by the API

 \return If the call is a success or failure
***************************************************************************/
s_int32_t
apn_bw_update_data_link_bw (struct interface *ifp, struct datalink *dl); 

#endif /* HAVE_GMPLS */
#endif /* _PACOS_PAL_BWMGR_H */
