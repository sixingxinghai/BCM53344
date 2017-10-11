/**@file elmi_unin.h
 ** @brief  This file declares the ELMI UNIN Transmit Module functions.
 **/
/* Copyright (C) 2010  All Rights Reserved. */

#ifndef _PACOS_ELMI_UNIN_H_
#define _PACOS_ELMI_UNIN_H_

int
elmi_get_seqeunce_num(struct elmi_port *port);


int
elmi_uni_n_tx (struct elmi_port *port, u_int8_t report_type);

int
elmi_unin_tx_status_msg (struct elmi_port *port, u_int8_t report_type);

int
elmi_unin_send_status_enq_msg (u_char **pnt,
                     u_int32_t *size, struct elmi_port *port);

int
elmi_unin_fullstatus_msg (struct elmi_port *port);

int
elmi_unin_send_fullstatus_msg (u_char **pnt,
                     u_int32_t *size, struct elmi_port *port);

int
elmi_unin_send_fullstatus_contd_msg (u_char **pnt,
                         u_int32_t *size, struct elmi_port *port);

int
elmi_unin_send_async_evc_msg (struct elmi_evc_status *evc_info,
                              struct elmi_port *port);

#endif /* _PACOS_ELMI_UNIN_H_ */
