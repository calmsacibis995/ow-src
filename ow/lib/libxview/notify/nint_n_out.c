#ifndef	lint
#ifdef sccs
static char     sccsid[] = "@(#)nint_n_out.c 20.10 90/06/21 Copyr 1985 Sun Micro";
#endif
#endif

/*
 *	(c) Copyright 1989 Sun Microsystems, Inc. Sun design patents 
 *	pending in the U.S. and foreign countries. See LEGAL NOTICE 
 *	file for terms of the license.
 */

/*
 * Nint_n_out.c - Implement the notify_next_output_func interface.
 */

#include <xview_private/ntfy.h>
#include <xview_private/ndet.h>
#include <xview_private/nint.h>

extern          Notify_value
notify_next_output_func(nclient, fd)
    Notify_client   nclient;
    int             fd;
{
    return (nint_next_fd_func(nclient, NTFY_OUTPUT, fd));
}
