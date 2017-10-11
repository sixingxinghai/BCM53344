/* Copyright (C) 2004   All Rights Reserved.  */

#ifndef _PAL_SIGNAL_H
#define _PAL_SIGNAL_H

RETSIGTYPE *pal_signal_set (int, void (*func) (int));
void pal_signal_init (void);

#endif /* _PAL_SIGNAL_H */
