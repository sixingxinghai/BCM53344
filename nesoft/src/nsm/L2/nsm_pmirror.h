/* Copyright (C) 2004  All Rights Reserved. */

#ifndef _PACOS_L2_PMIRROR_H
#define _PACOS_L2_PMIRROR_H

struct list *port_mirror_list;
struct list *port_mirror_list_config_restore;
struct pm_node {
       struct interface *to;
       struct interface *from;
       int    direction;
};

void port_mirroring_deinit ();
void * port_mirroring_get_first (void);
void port_mirroring_init(struct lib_globals *zg);
int port_mirroring_write(struct cli *, int);
int port_add_mirror_interface (struct interface * to_ifp, char *ifname,
                               int *mirror_direction);
int
port_mirroring_list_del(struct interface *to,
                        struct interface *from,
                        int    *direction,
                        struct lib_globals *zg);
int
port_mirroring_list_add (struct lib_globals *, 
                         struct list *,
                         struct interface *to,
                         struct interface *from,
                         int    direction);
int  
port_mirror_list_config_restore_after_sync (struct lib_globals *);
#endif /* _PACOS_L2_PMIRROR_H */
