#ifndef	_XOL_POPUPWINDP_H
#define	_XOL_POPUPWINDP_H

#pragma	ident	"@(#)PopupWindP.h	302.6	95/09/26 include/Xol SMI"	/* popupwindo:include/Xol/PopupWindP.h 1.7 	*/

/*
 *	Copyright (C) 1986,1991  Sun Microsystems, Inc
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


#include	<Xol/PopupWindo.h>

#include	<X11/ShellP.h>


#ifdef	__cplusplus
extern "C" {
#endif


typedef struct
{
	int keep_compiler_happy;   /* No new procedures */
} PopupWindowShellClassPart;

typedef struct _PopupWindowShellClassRec {
  	CoreClassPart		core_class;
	CompositeClassPart	composite_class;
	ShellClassPart		shell_class;
	WMShellClassPart	wm_shell_class;
	VendorShellClassPart	vendor_shell_class;
	TransientShellClassPart		transient_shell_class;
	PopupWindowShellClassPart	popup_shell_class;
} PopupWindowShellClassRec;

extern PopupWindowShellClassRec popupWindowShellClassRec;

/* New fields for the OLIT Popup Window widget */

typedef struct {
	Widget		upperControlArea;
	Widget		lowerControlArea;
	Widget		footerPanel;
	OlStrRep	text_format;
	Boolean		propchange;
	XtCallbackList	apply;
	XtCallbackList	setDefaults;
	XtCallbackList	reset;
	XtCallbackList	resetFactory;
	XtCallbackList	verify;
	Widget		menu;
	OlStr		menuTitle;
	OlStr		applyLabel;
	OlStr		setDefaultsLabel;
	OlStr		resetLabel;
	OlStr		resetFactoryLabel;
	char		applyMnemonic;
	char		setDefaultsMnemonic;
	char		resetMnemonic;
	char		resetFactoryMnemonic;
	Boolean     	warp_pointer;

	/* Private Members */
	Boolean     do_unwarp;      /* unwarp only when no motion */
    	int         root_x;         /* to restore warped pointer */
    	int         root_y;         /* to restore warped pointer */

} PopupWindowShellPart;

typedef struct _PopupWindowShellRec
{
	CorePart 	core;
	CompositePart 	composite;
	ShellPart 	shell;
	WMShellPart	wm;
	VendorShellPart	vendor;
	TransientShellPart	transient;
	PopupWindowShellPart	popupwindow;
} PopupWindowShellRec;

#ifdef  __STDC__
extern void	_OlPopupWiShPopDown(const Widget wid, 
	const Boolean override_pushpin);
#else   /* __STDC__ */
extern void	_OlPopupWiShPopDown();
#endif  /* __STDC__ */


#ifdef	__cplusplus
}
#endif


#endif	/* _XOL_POPUPWINDP_H */
