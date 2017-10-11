/* Copyright (C) 2004  All Rights Reserved. */

#ifndef _HAL_PMIRROR_H_
#define _HAL_PMIRROR_H_



/* 
   Name: hal_port_mirror_init

   Description:
   This API initializes the port mirroring hardware layer.

   Parameters:
   None

   Returns:
   HAL_ERR_PMIRROR_INIT
   HAL_SUCCESS
*/
int
hal_port_mirror_init (void);

/* 
   Name: hal_port_mirror_deinit

   Description:
   This API deinitializes the port mirroring hardware layer.

   Parameters:
   None

   Returns:
   HAL_ERR_PMIRROR_DEINIT
   HAL_SUCCESS
*/
int
hal_port_mirror_deinit (void);

/* 
   Name: hal_port_mirror_set

   Description:
   This API sets the port mirroring.

   Parameters:
   IN -> to_ifindex - Mirrored-to port
   IN -> from_ifindex - Mirrored-from port
   IN -> direction - Direction to set for mirroring

   Returns:
   HAL_ERR_PMIRROR_SET
   HAL_SUCCESS
*/
int
hal_port_mirror_set (unsigned int to_ifindex, unsigned int from_ifindex,
                     enum hal_port_mirror_direction direction);


/* 
   Name: hal_port_mirror_unset

   Description:
   This API unsets port mirroring.

   Parameters:
   IN -> to_ifindex - Mirrored-to port
   IN -> from_ifindex - Mirrored-from port
   IN -> Direction to unset for mirroring

   Returns:
   HAL_ERR_PMIRROR_UNSET
   HAL_SUCCESS
*/
int
hal_port_mirror_unset (unsigned int to_ifindex, unsigned int from_ifindex,
                       enum hal_port_mirror_direction direction);

#endif /* _HAL_PMIRROR_H_ */
