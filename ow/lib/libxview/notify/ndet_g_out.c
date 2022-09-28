#ifndef	lint
#ifdef sccs
static char     sccsid[] = "@(#)ndet_g_out.c 20.10 90/06/21 Copyr 1985 Sun Micro";
#endif
#endif

/*
 *	(c) Copyright 1989 Sun Microsystems, Inc. Sun design patents 
 *	pending in the U.S. and foreign countries. See LEGAL NOTICE 
 *	file for terms of the license.
 */

/*
 * Ndet_g_out.c - Implement notify_get_output_func call.
 */

#include <xview_private/ntfy.h>
#include <xview_private/ndet.h>

extern          Notify_func
notify_get_output_func(nclient, fd)
    Notify_client   nclient;
    int             fd;
{
    return (ndet_get_fd_func(nclient, fd, NTFY_OUTPUT));
}
