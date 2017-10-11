/* Copyright (C) 2003  All Rights Reserved. */

#include "pal.h"

#ifdef HAVE_ONMD

#include "hal_incl.h"
#include "hal_comm.h"
#include "hal_oam.h"
#include "efm/if_efm.h"

#include "lib.h"

static u_int32_t err_cnt;
static u_int32_t no_of_seconds;
static u_int32_t no_frame_errors;
static u_int32_t no_frame_second_errors;

int
hal_efm_set_port_state (unsigned int index,
                        enum hal_efm_par_action local_par_action,
                        enum hal_efm_mux_action local_mux_action)
{
  return hal_ioctl (APNEFM_SET_PORT_STATE, index, local_par_action,
                    local_mux_action, 0, 0, 0);
}

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
                                  ut_int64_t symbol_period_window)

{
  return HAL_SUCCESS;
}

/*
   Name: hal_efm_set_frame_period_window

   Description:
   This API sets the number of frames defining the window
   for polling.

   Parameters:
   IN -> index - port index
   IN -> frame_period_window - Number of symbols defining the window.

   Returns:
   < 0 on error
   HAL_SUCCESS
*/

int
hal_efm_set_frame_period_window (unsigned int index,
                                 u_int32_t frame_period_window)

{
  return HAL_SUCCESS;
}

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
                        ut_int64_t *no_of_errors)
{
  ut_int64_t temp;

  pal_mem_set (&temp, 0, sizeof (ut_int64_t));

  PAL_MUL_32_UINT (no_frame_errors, err_cnt, temp);

  no_of_errors->l[0] = temp.l[0];
  no_of_errors->l[1] = temp.l[1];
  err_cnt++;

  return HAL_SUCCESS;
}

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
                             ut_int64_t *no_of_errors)
{

  if (no_of_seconds < no_frame_second_errors)
    {
      no_of_errors->l[0] = 1;
      no_of_errors->l[1] = 0;
    }
  else
    {
      no_of_errors->l[0] = 0;
      no_of_errors->l[1] = 0;
    }

  no_of_seconds++;
  return HAL_SUCCESS;
}

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
hal_efm_set_err_frames (u_int32_t no_of_errors)
{
  err_cnt = 0;
  no_frame_errors = no_of_errors;
  return HAL_SUCCESS;
}

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
hal_efm_set_err_frame_seconds (u_int32_t no_of_errors)
{
  no_frame_second_errors = no_of_errors;
  return HAL_SUCCESS;
}

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
hal_efm_reset_err_frame_second_count ()
{ 
  no_of_seconds = 0;
  return HAL_SUCCESS;
}

#endif /* HAVE_HARDWARE_OAM_SUPPORT */

/*
   Name: hal_set_oam_hw_addr

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
hal_set_oam_dest_addr (u_int8_t *dest_addr, enum hal_l2_proto proto)
{
  return HAL_SUCCESS;
}

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
hal_set_oam_ether_type (u_int16_t ether_type, enum hal_l2_proto proto)
{
  return HAL_SUCCESS;
}


/*
   Name: hal_set_cfm_trap_level_pdu

   Description:
   This enables the PDU at particular level.

   Parameters:
   IN -> level  - Level for which the PDU is to be trapped.
   IN -> pdu    - The control packets to be trapped.

   Returns:
   < 0 on error
   HAL_SUCCESS
*/

int
hal_set_cfm_trap_level_pdu (u_int8_t level, enum hal_cfm_pdu_type pdu)
{
  return HAL_SUCCESS;
}

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
hal_unset_cfm_trap_level_pdu (u_int8_t level, enum hal_cfm_pdu_type pdu)
{
  return HAL_SUCCESS;
}

#endif /* HAVE_ONMD */
