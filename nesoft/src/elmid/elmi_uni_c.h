/**@file elmi_unic.h
 *  ** @brief  This file declares the ELMI UNIC Transmit Module functions.
 *   **/
/* Copyright (C) 2010  All Rights Reserved. */

#ifndef _PACOS_ELMI_UNIC_H_
#define _PACOS_ELMI_UNIC_H_

int 
elmi_uni_c_tx (struct elmi_port *port, u_int8_t type);

int
elmi_get_seqeunce_num(struct elmi_port *port);

int
elmi_validate_and_create_cvlans (struct elmi_port *port);

void 
elmi_hexdump (unsigned char *buf, int len);

#endif /* _PACOS_ELMI_UNIC_H_ */
