/*	@(#)destruct.h 3.1 IEI SMI	*/

/*
 * Copyright (c) 1992 by Sun Microsystems, Inc.
 */

/*
 * Routines which implement mailtool's self destruct capability. 
 */

#ifndef _DESTRUCT_H
#define _DESTRUCT_H

/* Itimer callback to exit mailtool */
extern Notify_value	mt_self_destruct();

/* Start the self-destruct sequence */
extern int		mt_start_self_destruct();

/* Terminate the self-destruct sequence */
extern int		mt_stop_self_destruct();

/* Check if mailtool is completely unmapped */
extern int		mt_mailtool_hidden();

#endif /* _DESTRUCT_H */
