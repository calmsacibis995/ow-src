#ifndef	lint
#ifdef sccs
static char     sccsid[] = "@(#)ndet_event.c 20.10 90/06/21 Copyr 1985 Sun Micro";
#endif
#endif

/*
 *	(c) Copyright 1989 Sun Microsystems, Inc. Sun design patents 
 *	pending in the U.S. and foreign countries. See LEGAL NOTICE 
 *	file for terms of the license.
 */

/*
 * Ndet_event.c - Implement event specific calls that are shared among
 * NTFY_SAFE_EVENT and NTFY_IMMEDIATE_EVENT.
 */

#include <xview_private/ntfy.h>
#include <xview_private/ndet.h>

pkg_private int
ndet_check_when(when, type_ptr)
    Notify_event_type when;
    NTFY_TYPE      *type_ptr;
{
    NTFY_TYPE       type;

    switch (when) {
      case NOTIFY_SAFE:
	type = NTFY_SAFE_EVENT;
	break;
      case NOTIFY_IMMEDIATE:
	type = NTFY_IMMEDIATE_EVENT;
	break;
      default:
	ntfy_set_errno(NOTIFY_INVAL);
	return (-1);
    }
    if (type_ptr)
	*type_ptr = type;
    return (0);
}
