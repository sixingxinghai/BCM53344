/* Copyright 2003  All Rights Reserved. */
#ifndef _PACOS_GIP_H
#define _PACOS_GIP_H

#include "pal.h"

/*
    FUNCTION : gip_create_gip

    DESCRIPTION : 
    GIP instance maintains the propagated count for each garp application 
    attributes upto maximum attributes.  In simple it is an unsigned int 
    array which keeps the propagated count. 

    IN :
    unsigned int max_attributes : Number of gip structure that needs to be 
                                  created
    unsigned int **gip : pointer to the gip structure's created
    
    OUT :
    PAL_TRUE : creation of gip structure succeeded
    PAL_FALSE : creation of gip structure failed

    CALLED BY :
    gmr_create_gmr
*/
extern bool_t
gip_create_gip (struct garp_instance *garp_instance);


/*
    FUNCTION : gip_destroy_gip

    DESCRIPTION : 
    Frees up the GIP instance that was created for the garp application

    IN :
    unsigned int *gip : pointer to the GIP instance that needs to be freed 

    OUT :
    None

    CALLED BY :
*/
extern void
gip_destroy_gip (struct garp_instance *garp_instance);

/*
    FUNCTION : gid_propagate_join

    DESCRIPTION : 
    Progates a join indication, causing join request to other ports for the
    specified gid_index and also inform the garp application about the join
    This needs to be propagated only when the this is the first port in the 
    port in the connected ring or if there is one another port that would cause
    a hoin request.

    IN :
    struct gid *gid : gid_instance needed to propagate the registration 
                      information to all the ports in the connected ring
    unsigned int gid_index : gid_index that needs to be propagated to the other 
                             ports
    enum gid_event event   : NEW or JOIN

    OUT :
    None

    CALLED BY :
    gid_set_attribute_states, gid_rcv_msg, gip_connect_port
*/
extern void
gip_propagate_join_event (struct gid *gid, u_int32_t gid_index,
                          enum gid_event event);


/*
    FUNCTION : gid_propagate_leave

    DESCRIPTION :
    Progates a leave indication, causing leave request to other ports for the
    specified gid_index and also inform the garp application about the leave
    provided the the gid_instance is connected to the propagation ring
    (connected ring) and the decremented joining members value is 0.
    In other word this needs to be propagated only when the this gid_instance 
    is the last port in the port in the connected ring or if there is one 
    another port that has registered.

    IN :
    struct gid *gid : gid_instance needed to propagate the deregistration 
                      information to all the ports in the connected ring
    unsigned int gid_index : gid_index that needs to be propagated to the other 
                             ports

    OUT :
    None

    CALLED BY :
    gid_set_attribute_states, gid_rcv_msg, gip_disconnect_port
*/
extern void
gip_propagate_leave (struct gid *gid, u_int32_t gid_used);

/*
    FUNCTION : gip_do_actions

    DESCRIPTION :

    IN :
    struct gid *gid :

    OUT :
    None

    CALLED BY :
*/
extern void
gip_do_actions (struct gid *gid);

#endif /* _PACOS_GIP_H */
