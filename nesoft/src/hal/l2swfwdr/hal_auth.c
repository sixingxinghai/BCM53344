/* Copyright (C) 2003  All Rights Reserved. */

#include "pal.h"

#ifdef HAVE_AUTHD

#include "hal_incl.h"
#include "hal_comm.h"
#include "auth/if_eapol.h"

#include "lib.h"

int 
hal_auth_init (void)
{
  return HAL_SUCCESS;
}

int
hal_auth_deinit (void)
{
  return HAL_SUCCESS;
}
  
int
hal_auth_set_port_state (unsigned int index, enum hal_auth_port_state state)
{
  return hal_ioctl (APNEAPOL_SET_PORT_STATE, index, state, 0, 0, 0, 0);
}

/* Set port auth-mac state */
int
hal_auth_mac_set_port_state (unsigned int index, int mode,
                             enum hal_auth_mac_port_state state)
{
  return hal_ioctl (APNEAPOL_SET_PORT_MACAUTH_STATE, index, state, 0, 0, 0, 0);
}

int
hal_auth_mac_unset_irq (unsigned int index)
{
  return HAL_SUCCESS;
}

#endif /* HAVE_AUTHD */
