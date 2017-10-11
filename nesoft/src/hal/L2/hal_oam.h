/* Copyright (C) 2004  All Rights Reserved. */

#ifndef _HAL_OAM_H_
#define _HAL_OAM_H_

enum hal_efm_mux_action
{
  HAL_MUX_ACTION_FWD,
  HAL_MUX_ACTION_DISCARD,
  HAL_MUX_ACTION_INVALID,
};

enum hal_efm_par_action
{
  HAL_PAR_ACTION_FWD,
  HAL_PAR_ACTION_LB,
  HAL_PAR_ACTION_DISCARD,
  HAL_PAR_ACTION_INVALID,
};

enum hal_cfm_pdu_type
{
  HSL_CFM_TR_FRAME,
  HSL_CFM_CCM_FRAME,
};

#define HAL_LINK_MONITOR_POLL_PERIOD_NONE   0


/* 
   Name: hal_efm_set_port_state 

   Description:
   This API sets the multiplexer and parser state of a port 

   Parameters:
   IN -> index - port index
   IN -> local_par_action - port parser state
   IN -> local_mux_action - port multiplexer state
   
   Returns:
   < 0 on error
   HAL_SUCCESS
*/

int
hal_efm_set_port_state (unsigned int index,
                        enum hal_efm_par_action local_par_action,
                        enum hal_efm_mux_action local_mux_action);

/*
int
_hal_efm_get_err_frames(struct hal_nlmsghdr *h, void *data);

int
_hal_efm_get_err_frame_secs(struct hal_nlmsghdr *h, void *data);
*/
  
/*
   Name: hal_set_oam_dest_addr

   Description:
   This API sets the destination address of the control
   frames of the specified protocol.

   Parameters:
   IN -> dest_addr - destination address for the protocol
   IN -> proto     - protocol for which this destination address
                     is to be used.

   Returns:
   < 0 on error
   HAL_SUCCESS
*/

int
hal_set_oam_dest_addr (u_int8_t *dest_addr, enum hal_l2_proto proto);

/*
   Name: hal_set_oam_ether_type

   Description:
   This API sets the Ethernet Type of the control
   frames of the specified protocol.

   Parameters:
   IN -> ether_type - Ethernet Type
   IN -> proto      - protocol for which this destination address
                      is to be used.

   Returns:
   < 0 on error
   HAL_SUCCESS
*/

int
hal_set_oam_ether_type (u_int16_t ether_type, enum hal_l2_proto proto);

/*
   Name: hal_set_cfm_trap_level_pdu

   Description:
   This API sets the Ethernet Type of the control
   frames of the specified protocol.

   Parameters:
   IN -> level  - Level for which the PDU is to be trapped.
   IN -> pdu    - The control packets to be trapped.

   Returns:
   < 0 on error
   HAL_SUCCESS
*/

int
hal_set_cfm_trap_level_pdu (u_int8_t level, enum hal_cfm_pdu_type pdu);

/*
   Name: hal_unset_cfm_trap_level_pdu

   Description:
   This disables the PDU at particular level.

   Parameters:
   IN -> level  - Level for which the PDU is to be trapped.
   IN -> pdu    - The control packets to be trapped.

   Returns:
   < 0 on error
   HAL_SUCCESS
*/

int
hal_unset_cfm_trap_level_pdu (u_int8_t level, enum hal_cfm_pdu_type pdu);


/*
   Name: hal_efm_set_symbol_period_window

   Description:
   This API sets the number of symbols defining the window
   for polling.

   Parameters:
   IN -> index - port index
   IN -> symbol_period_window - Number of symbols defining the window.

   Returns:
   < 0 on error
   HAL_SUCCESS
*/

int
hal_efm_set_symbol_period_window (unsigned int index,
                                  ut_int64_t symbol_period_window);

/*
   Name: hal_efm_set_frame_period_window

   Description:
   This API sets the number of frames defining the window
   for polling.

   Parameters:
   IN -> index - port index
   IN -> frame_period_window - Number of frames defining the window.

   Returns:
   < 0 on error
   HAL_SUCCESS
*/

int
hal_efm_set_frame_period_window (unsigned int index,
                                 u_int32_t frame_period_window);

/*
   Name: hal_efm_get_err_frames

   Description:
   This API gets the total numbers of errored frames.

   Parameters:
   IN -> index - port index
   IN -> no_of_errors - Total number of errors.

   Returns:
   < 0 on error
   HAL_SUCCESS
*/

int
hal_efm_get_err_frames (unsigned int index,
                        ut_int64_t *no_of_errors);

#ifndef HAVE_HARDWARE_OAM_SUPPORT

/*
   Name: hal_efm_get_err_frames_secs

   Description:
   This is a Dummy API used for testing OAM without hardware support.

   Parameters:
   IN -> index - port index
   IN -> no_of_errors - Total number of errors.

   Returns:
   < 0 on error
   HAL_SUCCESS
*/

int
hal_efm_get_err_frames_secs (unsigned int index,
                             ut_int64_t *no_of_errors);

/*
   Name: hal_efm_set_err_frames

   Description:
   This API sets the total numbers of errored frames. Used for testing
   OAM without hardware support.

   Parameters:
   IN -> no_of_errors - Total number of errors.

   Returns:
   < 0 on error
   HAL_SUCCESS
*/

int
hal_efm_set_err_frames (u_int32_t no_of_errors);

/*
   Name: hal_efm_set_err_frame_seconds

   Description:
   This API sets the total numbers of errored frame seconds. Used for testing
   OAM without hardware support.

   Parameters:
   IN -> no_of_errors - Total number of errored frame seconds.

   Returns:
   < 0 on error
   HAL_SUCCESS
*/

int
hal_efm_set_err_frame_seconds (u_int32_t no_of_error);

/*
   Name: hal_efm_reset_err_frame_second_count

   Description:
   This API resets the frame second count.
   Used for testing OAM without hardware support.

   Parameters:
   IN -> index - port index
   IN -> no_of_errors - Total number of errored frame seconds.

   Returns:
   < 0 on error
   HAL_SUCCESS
*/

int
hal_efm_reset_err_frame_second_count ();

#endif /* HAVE_HARDWARE_OAM_SUPPORT */

#endif /* _HAL_OAM_H_ */
