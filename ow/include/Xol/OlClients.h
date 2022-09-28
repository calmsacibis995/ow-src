#ifndef	_XOL_OLCLIENTS_H
#define	_XOL_OLCLIENTS_H

#pragma	ident	"@(#)OlClients.h	302.1	92/03/26 include/Xol SMI"	/* misc:include/Xol/OlClients.h 1.16	*/

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


#include <Xol/OpenLook.h>

#include <X11/X.h>
#include <X11/Xlib.h>


#ifdef	__cplusplus
extern "C" {
#endif


typedef struct {
	long		flags;
	enum {
		MENU_FULL = 0,
		MENU_LIMITED = 1,
		MENU_NONE = 2
	}		menu_type;
	long		pushpin_initial_state; /* as for WM_PUSHPIN_STATE */
}		WMDecorationHints;


#define WMWindowNotBusy		0
#define WMWindowIsBusy		1

#define WMPushpinIsOut		0
#define WMPushpinIsIn		1


typedef struct {
	int		state;
	Window		icon;
} WMState;

typedef struct {
	int	min_width;
	int	min_height;
	int	max_width;
	int	max_height;
	int	width_inc;
	int	height_inc;
} WMIconSize;


/**************************************************************************
 *
 * WARNING. OlWinAttr and the _OL_WIN_ATTR property.
 * -------- --------- --- --- ------------ ---------
 *
 * In earlier versions of OLIT prior to, and including, version 2.5 the 
 * _OL_WIN_ATTR property recognised by the Sun olwm was incompatible with 
 * that defined by the AT&T version of the toolkit in that there were no
 * "flags" or "cancel" fields in the Sun version of the property.
 *
 * In version 2 (and 2.5) of OLIT these fields were #ifdef'ed out of the
 * AT&T strutcure in order to support interoperability with OpenWindows 
 * V2.0 olwm.
 *
 * From V2.5 of OLIT it is intended to move over to the longer AT&T version
 * of this property containing the 'flags' and 'cancel' fields.
 * However to maintain backwards compatibility with V2.0 code the shorter
 * variant shall remain. Its use is restricted to provide source and binary
 * backwards compatibility for V2.0 clients ONLY.
 *
 * In general applications developers building on OLIT V2.5 or later are 
 * STRONGLY ADVISED NOT TO USE THE OlWinAttrs STRUCTURE DIRECTLY IN EITHER 
 * OF ITS FORMS, ALONG WITH THE GetOLWinAttrs, SetOLWinAttrs, 
 * XGetWindowPropety OR XChangeProperty FUNCTIONS IN ORDER TO AFFECT WINDOW 
 * DECORATION. Applications programmers should use the appropriate resources
 * affecting window decoration defined by the Vendor Shell Class Extension 
 * found in OLIT V2.5 or later. 
 *
 * Should the applications programmer insist on direct use of the
 * _OL_WIN_ATTR property, access should be via the (New)OlWinAttr structure
 * and the Set/GetOLWinAttrs and Set/GetNewOLWinAttrs functions,
 * since they take note (as does the Vendor Shell extension) of the 
 * XtNuseShortOLWinAttrs toolkit resource which defines the appropriate
 * variant of the property recognised by the local olwm.
 * Applications programmers using Xlib functions to read and write the
 * _OL_WIN_ATTR property should note the setting of this resource and
 * and the length of the property returned from XGetWindowProperty to
 * determine the appropriate variant in use.
 *
 **************************************************************************/

/* 
 * THE FOLLOWING STRUCTURE USED IN A SUN ENVIRONMENT IS USED FOR BACKWARDS 
 * COMPATIBILITY ONLY
 */

typedef struct OlWinAttr {	/* old short version DO NOT USE */
	Atom		win_type;
	long		menu_type;
	long		pin_state;
} OLWinAttr;

#define	ULongsInOLWinAttr	(sizeof (OLWinAttr) / sizeof (unsigned long))

/*
 * This New structure should be used if at all, developers are recommended
 * to use the appropriate shell resources.
 */

typedef struct NewOlWinAttr {	/* new longer AT&T compliant struct */
	unsigned long	flags;
	Atom		win_type;
	long		menu_type;
	long		pin_state;
	long		cancel;
} NewOLWinAttr;

#define	ULongsInNewOLWinAttr	(sizeof (NewOLWinAttr) / sizeof (unsigned long))

typedef struct OlWinColors {
	unsigned long	flags;
	unsigned long	fg_red;
	unsigned long	fg_green;
	unsigned long	fg_blue;
	unsigned long	bg_red;
	unsigned long	bg_green;
	unsigned long	bg_blue;
	unsigned long	bd_red;
	unsigned long	bd_green;
	unsigned long	bd_blue;
} OLWinColors;

/*
 * values for _OL_WIN_ATTR flags
 */
#define	_OL_WA_WIN_TYPE		(1<<0)
#define	_OL_WA_MENU_TYPE	(1<<1)
#define	_OL_WA_PIN_STATE	(1<<2)
#define	_OL_WA_CANCEL		(1<<3)

/*
 * values for _OL_WIN_COLORS flags
 */
#define	_OL_WC_FOREGROUND	(1<<0)
#define	_OL_WC_BACKGROUND	(1<<1)
#define	_OL_WC_BORDER		(1<<2)

/*
 * for compatiblity with earlier software
 */
#define WM_PUSHPIN_STATE	_OL_PIN_STATE
#define WM_WINDOW_BUSY		_OL_WIN_BUSY
#define MENU_DISMISS_ONLY	MENU_NONE


extern Atom	WM_DISMISS;
extern Atom	WM_DECORATION_HINTS;
extern Atom	WM_WINDOW_MOVED;
extern Atom	WM_TAKE_FOCUS;
extern Atom	WM_DELETE_WINDOW;
extern Atom	BANG;
extern Atom	WM_SAVE_YOURSELF;
extern Atom	WM_STATE;
extern Atom	WM_CHANGE_STATE;
extern Atom	WM_PROTOCOLS;

extern Atom	_OL_COPY;
extern Atom	_OL_CUT;
extern Atom	_OL_HELP_KEY;
extern Atom	_OL_WIN_ATTR;
extern Atom	_OL_WT_BASE;
extern Atom	_OL_WT_CMD;
extern Atom	_OL_WT_PROP;
extern Atom	_OL_WT_HELP;
extern Atom	_OL_WT_NOTICE;
extern Atom	_OL_WT_OTHER;
extern Atom	_OL_DECOR_ADD;
extern Atom	_OL_DECOR_DEL;
extern Atom	_OL_DECOR_CLOSE;
extern Atom	_OL_DECOR_RESIZE;
extern Atom	_OL_DECOR_HEADER;
extern Atom	_OL_DECOR_PIN;
extern Atom	_OL_WIN_COLORS;
extern Atom	_OL_PIN_STATE;
extern Atom	_OL_WIN_BUSY;
extern Atom	_OL_MENU_FULL;
extern Atom	_OL_MENU_LIMITED;
extern Atom	_OL_NONE;


/*
 * OpenLook module
 */

#if	defined(__STDC__) || defined(__cplusplus)

extern Atom*		GetAtomList(Display* dpy, Window w, Atom property,
	int* length, int c_delete);

extern char*		GetCharProperty(Display* dpy, Window w, Atom property, 
	int* length);
	
extern int		GetHelpKeyMessage(Display* dpy, XEvent* ev,
	Window* window_return, int* x_return, int* y_return, int* root_x_return,
	int* root_y_return);

extern int		GetNewOLWinAttr(Display* dpy, Window client_window, 
	NewOLWinAttr* olwa);

extern int		GetOLWinAttr(Display* dpy, Window client_window, 
	OLWinAttr* olwa);

extern int		GetOLWinColors(Display* dpy, Window win,
	OLWinColors* win_colors);

extern void		InitializeOpenLook(Display* dpy);

extern void		SetNewOLWinAttr(Display* dpy, Window w,
	NewOLWinAttr* olwa);

extern void		SetOLWinAttr(Display* dpy, Window w, OLWinAttr* olwa);

extern void		SetWMDecorationHints(Display* dpy, Window w, 
	WMDecorationHints* wmdh);

extern void		SetWMPushpinState(Display* dpy, Window w, long state);
extern void		SetWMState(Display* dpy, Window w, WMState* wms);
extern void		SetWMWindowBusy(Display* dpy, Window w, long state);

#else	/* __STDC__ || __cplusplus */

extern Atom*		GetAtomList();
extern char*		GetCharProperty();
extern int		GetHelpKeyMessage();
extern int		GetNewOLWinAttr();
extern int		GetOLWinAttr();
extern int		GetOLWinColors();
extern void		InitializeOpenLook();
extern void		SetNewOLWinAttr();
extern void		SetOLWinAttr();
extern void		SetWMDecorationHints();
extern void		SetWMPushpinState();
extern void		SetWMState();
extern void		SetWMWindowBusy();

#endif	/* __STDC__ || __cplusplus */


extern void		EnqueueCharProperty();
extern void		SendProtocolMessage();


#ifdef	__cplusplus
}
#endif


#endif	/* _XOL_OLCLIENTS_H */
