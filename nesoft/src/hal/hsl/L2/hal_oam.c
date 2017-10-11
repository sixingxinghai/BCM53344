/* Copyright (C) 2004  All Rights Reserved. */

#include "pal.h"
#include "lib.h"

#include "hal_incl.h"
#include "hal_types.h"
#include "hal_error.h"
#include "hal_netlink.h"
#include "hal_comm.h"
#include "hal_msg.h"

#ifdef HAVE_ONMD
#include "hal_oam.h"

int
hal_efm_dying_gasp_cb (struct hal_nlmsghdr *h);

int
hal_efm_frame_period_window_expiry_cb(struct hal_nlmsghdr *h,
                                    struct hal_msg_efm_err_frame_secs_resp *msg);
int
_hal_efm_get_err_frames(struct hal_nlmsghdr *h, void *data);

int
_hal_efm_get_err_frame_secs(struct hal_nlmsghdr *h, void *data);

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
                        enum hal_efm_mux_action local_mux_action)
{
  struct hal_nlmsghdr *nlh;
  struct hal_msg_efm_port_state *msg;
  struct
  {
    struct hal_nlmsghdr nlh;
    struct hal_msg_efm_port_state msg;
  } req;

  int ret = 0;

  pal_mem_set (&req.nlh, 0, sizeof (struct hal_nlmsghdr));
  pal_mem_set (&req.msg, 0, sizeof(struct hal_msg_efm_port_state));

  /* Set header. */
  nlh = &req.nlh;
  nlh->nlmsg_len = HAL_NLMSG_LENGTH (sizeof (struct hal_msg_efm_port_state));
  nlh->nlmsg_flags = HAL_NLM_F_CREATE | HAL_NLM_F_REQUEST;
  nlh->nlmsg_type = HAL_MSG_EFM_SET_PORT_STATE;
  nlh->nlmsg_seq = ++hallink_cmd.seq;

  /* Set message. */
  msg = &req.msg;
  msg->index = index;
  msg->local_par_action = local_par_action;
  msg->local_mux_action = local_mux_action;

  /* Send message and process ack. */
  ret = hal_talk (&hallink_cmd, nlh, NULL, NULL);
  if (ret < 0)
    return ret;

  return HAL_SUCCESS;
}

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
  struct hal_nlmsghdr *nlh;
  struct hal_msg_oam_hw_addr *msg;
  struct
  {
    struct hal_nlmsghdr nlh;
    struct hal_msg_oam_hw_addr msg;
  } req;

  int ret = 0;

  pal_mem_set (&req.nlh, 0, sizeof (struct hal_nlmsghdr));
  pal_mem_set (&req.msg, 0, sizeof(struct hal_msg_oam_hw_addr));

  /* Set header. */
  nlh = &req.nlh;
  nlh->nlmsg_len = HAL_NLMSG_LENGTH (sizeof (struct hal_msg_oam_hw_addr));
  nlh->nlmsg_flags = HAL_NLM_F_CREATE | HAL_NLM_F_REQUEST;
  nlh->nlmsg_type = HAL_MSG_L2_OAM_SET_HW_ADDR;
  nlh->nlmsg_seq = ++hallink_cmd.seq;

  /* Set message. */
  msg = &req.msg;
  pal_mem_cpy (msg->dest_addr, dest_addr, HAL_HW_LENGTH);
  msg->proto = proto;

  /* Send message and process ack. */
  ret = hal_talk (&hallink_cmd, nlh, NULL, NULL);
  if (ret < 0)
    return ret;

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
  struct hal_nlmsghdr *nlh;
  struct hal_msg_oam_ether_type *msg;
  struct
  {
    struct hal_nlmsghdr nlh;
    struct hal_msg_oam_ether_type msg;
  } req;

  int ret = 0;

  pal_mem_set (&req.nlh, 0, sizeof (struct hal_nlmsghdr));
  pal_mem_set (&req.msg, 0, sizeof (struct hal_msg_oam_ether_type));

  /* Set header. */
  nlh = &req.nlh;
  nlh->nlmsg_len = HAL_NLMSG_LENGTH (sizeof (struct hal_msg_oam_ether_type));
  nlh->nlmsg_flags = HAL_NLM_F_CREATE | HAL_NLM_F_REQUEST;
  nlh->nlmsg_type = HAL_MSG_L2_OAM_SET_ETHER_TYPE;
  nlh->nlmsg_seq = ++hallink_cmd.seq;

  /* Set message. */
  msg = &req.msg;
  msg->ether_type = ether_type;
  msg->proto = proto;

  /* Send message and process ack. */
  ret = hal_talk (&hallink_cmd, nlh, NULL, NULL);
  if (ret < 0)
    return ret;

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
  struct hal_nlmsghdr *nlh;
  struct hal_msg_cfm_trap_level_pdu *msg;
  struct
  {
    struct hal_nlmsghdr nlh;
    struct hal_msg_cfm_trap_level_pdu msg;
  } req;

  int ret = 0;

  pal_mem_set (&req.nlh, 0, sizeof (struct hal_nlmsghdr));
  pal_mem_set (&req.msg, 0, sizeof (struct hal_msg_cfm_trap_level_pdu));

  /* Set header. */
  nlh = &req.nlh;
  nlh->nlmsg_len = HAL_NLMSG_LENGTH 
                       (sizeof (struct hal_msg_cfm_trap_level_pdu));
  nlh->nlmsg_flags = HAL_NLM_F_CREATE | HAL_NLM_F_REQUEST;
  nlh->nlmsg_type = HAL_MSG_CFM_TRAP_PDU;
  nlh->nlmsg_seq = ++hallink_cmd.seq;

  /* Set message. */
  msg = &req.msg;
  msg->level = level;
  msg->pdu = pdu;

  /* Send message and process ack. */
  ret = hal_talk (&hallink_cmd, nlh, NULL, NULL);
  if (ret < 0)
    return ret;

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
  struct hal_nlmsghdr *nlh;
  struct hal_msg_cfm_trap_level_pdu *msg;
  struct
  {
    struct hal_nlmsghdr nlh;
    struct hal_msg_cfm_trap_level_pdu msg;
  } req;

  int ret = 0;

  pal_mem_set (&req.nlh, 0, sizeof (struct hal_nlmsghdr));
  pal_mem_set (&req.msg, 0, sizeof (struct hal_msg_cfm_trap_level_pdu));

  /* Set header. */
  nlh = &req.nlh;
  nlh->nlmsg_len = HAL_NLMSG_LENGTH 
                       (sizeof (struct hal_msg_cfm_trap_level_pdu));
  nlh->nlmsg_flags = HAL_NLM_F_CREATE | HAL_NLM_F_REQUEST;
  nlh->nlmsg_type = HAL_MSG_CFM_UNTRAP_PDU;
  nlh->nlmsg_seq = ++hallink_cmd.seq;

  /* Set message. */
  msg = &req.msg;
  msg->level = level;
  msg->pdu = pdu;

  /* Send message and process ack. */
  ret = hal_talk (&hallink_cmd, nlh, NULL, NULL);
  if (ret < 0)
    return ret;

  return HAL_SUCCESS;
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
  struct hal_nlmsghdr *nlh;
  struct hal_msg_efm_set_symbol_period_window *msg;
  struct
    {
      struct hal_nlmsghdr nlh;
      struct hal_msg_efm_set_symbol_period_window msg;
    } req;

  int ret = 0;

  /*FOR UT*/
  return 1;
  
  pal_mem_set(&req.nlh, 0, sizeof (struct hal_nlmsghdr));
  pal_mem_set(&req.msg, 0, 
              sizeof(struct hal_msg_efm_set_symbol_period_window));

  /* Set header. */
  nlh = &req.nlh;
  nlh->nlmsg_len = HAL_NLMSG_LENGTH (sizeof (
                                struct hal_msg_efm_set_symbol_period_window));
  nlh->nlmsg_flags = HAL_NLM_F_CREATE | HAL_NLM_F_REQUEST;
  nlh->nlmsg_type = HAL_MSG_EFM_SET_SYMBOL_PERIOD_WINDOW;
  nlh->nlmsg_seq = ++hallink_cmd.seq;

  /* Set message. */
  msg = &req.msg;
  msg->index = index;
  pal_mem_cpy(&msg->symbol_period_window, &symbol_period_window, 
              sizeof(ut_int64_t));

  /* Send message and process ack. */
  ret = hal_talk (&hallink_cmd, nlh, NULL, NULL);
  if (ret < 0)
    return ret;

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
  struct hal_nlmsghdr *nlh;
  struct hal_msg_efm_set_frame_period_window *msg;
  struct
    {
      struct hal_nlmsghdr nlh;
      struct hal_msg_efm_set_frame_period_window msg;
    } req;

  int ret = 0;

  pal_mem_set (&req.nlh, 0, sizeof (struct hal_nlmsghdr));
  pal_mem_set (&req.msg, 0, 
               sizeof(struct hal_msg_efm_set_frame_period_window));

  /* Set header. */
  nlh = &req.nlh;
  nlh->nlmsg_len = HAL_NLMSG_LENGTH (sizeof (
                          struct hal_msg_efm_set_frame_period_window));
  nlh->nlmsg_flags = HAL_NLM_F_CREATE | HAL_NLM_F_REQUEST;
  nlh->nlmsg_type = HAL_MSG_EFM_SET_FRAME_PERIOD_WINDOW;
  nlh->nlmsg_seq = ++hallink_cmd.seq;

  /* Set message. */
  msg = &req.msg;
  msg->index = index;
  msg->frame_period_window = frame_period_window;

  /* Send message and process ack. */
  ret = hal_talk (&hallink_cmd, nlh, NULL, NULL);
  if (ret < 0)
    return ret;

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
  int ret;
  u_char *pnt;
  u_int32_t size;
  int msgsz;
  struct hal_nlmsghdr *nlh;
  u_int32_t msg = index;
  struct hal_msg_efm_err_frames_resp resp;  

  struct
    {
      struct hal_nlmsghdr nlh;
      u_int32_t req_index;
    }req;

  pal_mem_set (&req, 0, sizeof req); 

  resp.index = 0;
  resp.err_frames = no_of_errors;

  pnt = (u_char*)&req.req_index;
  size = sizeof(u_int32_t);

  msgsz = hal_msg_encode_err_frames_req(&pnt, &size, &msg);
  if(msgsz<0)
    return -1;

  /*Set header*/
  nlh = &req.nlh;
  nlh->nlmsg_len = HAL_NLMSG_LENGTH (msgsz);
  nlh->nlmsg_type = HAL_MSG_EFM_FRAME_ERROR_COUNT;
  nlh->nlmsg_flags = HAL_NLM_F_CREATE | HAL_NLM_F_REQUEST | HAL_NLM_F_ACK;
  nlh->nlmsg_seq = ++hallink_cmd.seq;

  /* Send message and process ack. */
  ret = hal_talk (&hallink_cmd, &req.nlh, _hal_efm_get_err_frames, &resp);
  if (ret < 0)
    {
      return 0;
    }

  return HAL_SUCCESS;
}

int
_hal_efm_get_err_frames (struct hal_nlmsghdr *h, void *data)
{
  struct hal_msg_efm_err_frames_resp* resp;
  u_char *pnt;
  u_int32_t size;
  int ret;

  resp =  (struct hal_msg_efm_err_frames_resp*)data;
  pnt = HAL_NLMSG_DATA (h);
  size = h->nlmsg_len - sizeof (struct hal_nlmsghdr);

  /* Decode message. */
  ret = hal_msg_decode_err_frames_resp(&pnt, &size, resp);
  if (ret < 0)
    return ret;

  return 0;
}

/*
   Name: hal_efm_get_err_frame_secs

   Description:
   This API gets the total numbers of error frame seconds.

   Parameters:
   IN -> index - port index
   IN -> no_of_error_frame_secs - Total number of error frame seconds.

   Returns:
   < 0 on error
   HAL_SUCCESS
*/

int
hal_efm_get_err_frame_secs (unsigned int index,
                        u_int32_t *no_of_error_frame_secs)
{
  int ret;
  u_char *pnt;
  u_int32_t size;
  int msgsz;
  struct hal_nlmsghdr *nlh;
  u_int32_t msg = index;
  struct hal_msg_efm_err_frame_secs_resp resp;

  struct
    {
      struct hal_nlmsghdr nlh;
      u_int32_t req_index;
    }req;
  
  pal_mem_set (&req, 0, sizeof req);

  resp.index = 0;
  resp.err_frame_secs = no_of_error_frame_secs;

  pnt = (u_char*)&req.req_index;
  size = sizeof(u_int32_t);

  msgsz = hal_msg_encode_err_frame_secs_req(&pnt, &size, &msg);
  if(msgsz<0)
    return -1;

  /*Set header*/
  nlh = &req.nlh;
  nlh->nlmsg_len = HAL_NLMSG_LENGTH (msgsz);
  nlh->nlmsg_type = HAL_MSG_EFM_FRAME_ERROR_SECONDS_COUNT;
  nlh->nlmsg_flags = HAL_NLM_F_CREATE | HAL_NLM_F_REQUEST | HAL_NLM_F_ACK;
  nlh->nlmsg_seq = ++hallink_cmd.seq;

  /* Send message and process ack. */
  ret = hal_talk (&hallink_cmd, &req.nlh, _hal_efm_get_err_frame_secs, &resp);
  if (ret < 0)
    {
      return 0;
    }

  return HAL_SUCCESS;
}

int
_hal_efm_get_err_frame_secs (struct hal_nlmsghdr *h, void *data)
{
  struct hal_msg_efm_err_frame_secs_resp* resp;
  u_char *pnt;
  u_int32_t size;
  int ret;

  resp =  (struct hal_msg_efm_err_frame_secs_resp*)data;
  pnt = HAL_NLMSG_DATA (h);
  size = h->nlmsg_len - sizeof (struct hal_nlmsghdr);

  /* Decode message. */
  ret = hal_msg_decode_err_frame_secs_resp(&pnt, &size, resp);
  if (ret < 0)
    return ret;
  
  return 0;
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
hal_efm_set_err_frame_seconds (u_int32_t no_of_error)
{
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
  return HAL_SUCCESS;
}

#endif /* HAVE_HARDWARE_OAM_SUPPORT */

/*
  Callback for HAL_MSG_EFM_DYING_GASP.
*/
int
hal_efm_dying_gasp(struct hal_nlmsghdr *h, void *data)
{
#ifdef HAVE_EFM
  return hal_efm_dying_gasp_cb (h);
#else
  return 0;
#endif /* HAVE_EFM */
}


int 
hal_efm_frame_period_window_expiry (struct hal_nlmsghdr *h, void *data)
{ 
  struct hal_msg_efm_err_frame_secs_resp msg;
  u_char *pnt;
  u_int32_t size;
  int ret;
  u_int32_t* counter = NULL;
  
  pnt = HAL_NLMSG_DATA (h);
  size = h->nlmsg_len - sizeof (struct hal_nlmsghdr);
  counter = (u_int32_t*) XCALLOC (MTYPE_TMP, sizeof (u_int32_t));
  msg.err_frame_secs = counter;

  /* Decode message. */
  ret = hal_msg_decode_err_frame_secs_resp(&pnt, &size, &msg);
  if(ret < 0)
    { 
      XFREE (MTYPE_TMP, counter); 
      return ret;
    }

#ifdef HAVE_EFM
  hal_efm_frame_period_window_expiry_cb(h, &msg);
#endif /* HAVE_EFM */

  XFREE (MTYPE_TMP, counter);

  return 0;
}

#endif /* HAVE_ONMD */

