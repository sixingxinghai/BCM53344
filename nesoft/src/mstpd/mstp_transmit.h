/* Copyriht (C) 2003  

        This modules contains the external declarations for the
        MSTP transmit functions. 
*/

#ifndef __PACOS_MSTP_TRANSMIT_H__
#define __PACOS_MSTP_TRANSMIT_H__

extern void mstp_tx_bridge (struct mstp_bridge *br);
extern int mstp_tx (struct mstp_port *port);

extern void stp_transmit_tcn_bpdu (struct mstp_bridge *br);
extern void stp_generate_bpdu_on_bridge (struct mstp_bridge *br);

extern void mstp_sendstp (struct mstp_port *port);
extern void mstp_sendmstp (struct mstp_port *port);

int
mstp_port_send (struct mstp_port *port, unsigned char *data, int length);

#if defined (HAVE_L2GP) || defined (HAVE_RPVST_PLUS)
inline u_char *
mstp_format_timer (u_char *dest, int tics);
#endif /* HAVE_L2GP || HAVE_RPVST_PLUS */

#endif
