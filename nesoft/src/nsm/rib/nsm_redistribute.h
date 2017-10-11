/* Copyright (C) 2001-2003  All Rights Reserved. */

#ifndef _NSM_REDISTRIBUTE_H
#define _NSM_REDISTRIBUTE_H

void nsm_redistribute_add (struct prefix *, struct rib *);
void nsm_redistribute_delete (struct prefix *, struct rib *);

#endif /* _NSM_REDISTRIBUTE_H */

