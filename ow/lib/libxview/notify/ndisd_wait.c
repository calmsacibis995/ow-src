#ifndef	lint
#ifdef sccs
static char     sccsid[] = "@(#)ndisd_wait.c 20.12 93/08/18 Copyr 1985 Sun Micro";
#endif
#endif

/*
 *	(c) Copyright 1989 Sun Microsystems, Inc. Sun design patents 
 *	pending in the U.S. and foreign countries. See LEGAL NOTICE 
 *	file for terms of the license.
 */

/*
 * Ndis_d_wait.c - Default wait3 function that is a nop.
 */
#include <xview_private/ntfy.h>
#include <xview_private/ndis.h>
#include <signal.h>

/* ARGSUSED */
extern          Notify_value
notify_default_wait3(client, pid, status, rusage)
    Notify_client   client;
    int             pid;
#ifndef SVR4
    union wait     *status;
    struct rusage  *rusage;
#else SVR4
    int *status;
    void *rusage;
#endif SVR4
{
    return (NOTIFY_IGNORED);
}
