#ifndef	lint
#ifdef sccs
static char     sccsid[] = "@(#)nintrdeath.c 20.10 90/06/21 Copyr 1985 Sun Micro";
#endif
#endif

/*
 *	(c) Copyright 1989 Sun Microsystems, Inc. Sun design patents 
 *	pending in the U.S. and foreign countries. See LEGAL NOTICE 
 *	file for terms of the license.
 */

/*
 * Nint_r_death.c - Implement the notify_remove_destroy_func interface.
 */

#include <xview_private/ntfy.h>
#include <xview_private/ndet.h>
#include <xview_private/nint.h>

extern          Notify_error
notify_remove_destroy_func(nclient, func)
    Notify_client   nclient;
    Notify_func     func;
{
    return (nint_remove_func(nclient, func, NTFY_DESTROY, NTFY_DATA_NULL,
			     NTFY_IGNORE_DATA));
}
