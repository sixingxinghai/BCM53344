/* 
   Copyright (C) 2003  All Rights Reserved. 
   
   LAYER 2 VLANS 
   
   This module defines the API to the "range list" maintenance
   functions.
  
*/

#ifndef __MSTP_RLIST_H__
#define __MSTP_RLIST_H__


extern int 
mstp_rlist_add (struct rlist_info **vlan_list , mstp_vid_t vid ,
                mstp_vid_t vlan_range_indx );

extern int
mstp_rlist_delete (struct rlist_info **range_list , mstp_vid_t element,
                       u_int16_t *rlist_index);

void
mstp_rlist_move (struct rlist_info **dest_list, struct rlist_info **src_list);

void
mstp_rlist_free (struct rlist_info **range_list);

#endif
