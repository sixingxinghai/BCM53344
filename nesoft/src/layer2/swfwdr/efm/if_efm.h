/* Copyright (C) 2003,  All Rights Reserved. 

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License
    as published by the Free Software Foundation; either version
    2 of the License, or (at your option) any later version.

*/
#ifndef __IF_EFM_H__
#define __IF_EFM_H__

#define ETH_P_EFM 0x8809

#define APNEFM_BASE           400
#define APNEFM_GET_VERSION    (APNEFM_BASE + 1)
#define APNEFM_ADD_PORT       (APNEFM_BASE + 2)
#define APNEFM_DEL_PORT       (APNEFM_BASE + 3)
#define APNEFM_SET_PORT_STATE (APNEFM_BASE + 4)

enum efm_local_par_action
{
  EFM_OAM_PAR_FWD,
  EFM_OAM_PAR_LB,
  EFM_OAM_PAR_DISCARD,
  EFM_OAM_PAR_INVALID,
};

enum efm_local_mux_action
{
  EFM_OAM_MUX_FWD,
  EFM_OAM_MUX_DISCARD,
  EFM_OAM_MUX_INVALID,
};

#endif
