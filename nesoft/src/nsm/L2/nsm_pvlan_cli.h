/* Copyright (C) 2004  All Rights Reserved.  */

#ifndef _NSM_PVLAN_CLI_H_
#define _NSM_PVLAN_CLI_H

int
nsm_pvlan_config_write (struct cli *cli);

int
nsm_pvlan_if_config_write (struct cli *cli, struct interface *ifp);

#endif /* _NSM_PVLAN_CLI_H */

