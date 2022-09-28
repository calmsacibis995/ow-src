#ifndef	_XOL_BASEWINDOP_H
#define	_XOL_BASEWINDOP_H

#pragma	ident	"@(#)BaseWindoP.h	302.1	92/03/26 include/Xol SMI"	/* basewindow:include/Xol/BaseWindoP.h 1.7 	*/

/*
 *	Copyright (C) 1986,1992  Sun Microsystems, Inc
 *			All rights reserved.
 *		Notice of copyright on this source code
 *		product does not indicate publication.
 *
 * RESTRICTED RIGHTS LEGEND: Use, duplication, or disclosure by
 * the U.S. Government is subject to restrictions as set forth
 * in subparagraph (c)(1)(ii) of the Rights in Technical Data
 * and Computer Software Clause at DFARS 252.227-7013 (Oct. 1988)
 * and FAR 52.227-19 (c) (June 1987).
 *
 *	Sun Microsystems, Inc., 2550 Garcia Avenue,
 *	Mountain View, California 94043.
 *
 */

/*	Copyright (c) 1990 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*************************************************************************
 *
 * Description:
 *		Private ".h" file for the BaseWindow Widget
 *
 *************************************************************************/


#include <Xol/BaseWindow.h>

#include <X11/ShellP.h>


#ifdef	__cplusplus
extern "C" {
#endif


/* New fields for the BaseWindow widget class record */
typedef struct _BaseWindowClass {
	int                     no_new_fields;
}                       BaseWindowShellClassPart;

/* Full class record declaration 	*/
typedef struct _BaseWindowClassRec {
	CoreClassPart           core_class;
	CompositeClassPart      composite_class;
	ShellClassPart          shell_class;
	WMShellClassPart        wm_shell_class;
	VendorShellClassPart    vendor_shell_class;
	TopLevelShellClassPart  top_level_shell_class;
	ApplicationShellClassPart application_shell_class;
	BaseWindowShellClassPart base_window_shell_class;
}                       BaseWindowShellClassRec;

extern BaseWindowShellClassRec baseWindowShellClassRec;


/***************************************
 *
 *  Instance (widget) structure 
 *
 **************************************/

/* New fields for the BaseWindow widget record */
typedef struct {
	int                     not_used;
}                       BaseWindowShellPart;

/*
 * Widget Instance declaration
 */
typedef struct _BaseWindowShellRec {
	CorePart                core;
	CompositePart           composite;
	ShellPart               shell;
	WMShellPart             wm;
	VendorShellPart         vendor;
	TopLevelShellPart       topLevel;
	ApplicationShellPart    application;
	BaseWindowShellPart     base_window;
}                       BaseWindowShellRec;


#ifdef	__cplusplus
}
#endif


#endif	/* _XOL_BASEWINDOP_H */
