#pragma ident	"@(#)OpenLook.c	302.7	97/03/26 lib/libXol SMI"	/* misc:src/OpenLook.c 1.44	*/

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


#include <stdio.h>

#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <X11/Xmd.h>
#include <X11/Xutil.h>

#include <Xol/OlClients.h>
#include <Xol/OlDnDVCX.h>
#include <Xol/OpenLookP.h>
#include <Xol/RootShell.h>


#define INTERN(property) property = OlInternAtom(dpy,#property)
#define PROPERTY(property)	Atom property

	/* property structures */

typedef struct {
	CARD32	flags;
	CARD32	menu_type;
	CARD32	pushpin_initial_state;	/* for _OL_PIN_STATE */
} xPropWMDecorationHints;

#define NumPropWMDecorationHintsElements 3

typedef struct {
	CARD32		state;
	Window		icon;
} xPropWMState;

#define NumPropWMStateElements 2

typedef struct {
	CARD32	min_width;
	CARD32	min_height;
	CARD32	max_width;
	CARD32	max_height;
	CARD32	width_inc;
	CARD32	height_inc;
} xPropWMIconSize;

#define NumPropWMIconSizeElements 6

/***********************************************************************
 *
 * WARNING. see note about OlWinAttrs in OlClients.h
 *
 ***********************************************************************/

typedef struct {
#ifndef	sun
	CARD32	flags;
#endif
	CARD32	win_type;
	CARD32	menu_type;
	CARD32	pin_state;
#ifndef	sun
	CARD32	cancel;
#endif
} xPropOLWinAttr;

#define NumPropOLWinAttrElements (sizeof(xPropOLWinAttr) / sizeof(CARD32))

#ifdef	sun
typedef struct {
	CARD32	flags;
	CARD32	win_type;
	CARD32	menu_type;
	CARD32	pin_state;
	CARD32	cancel;
} xPropNewOLWinAttr;

#define NumPropNewOLWinAttrElements (sizeof(xPropNewOLWinAttr) / sizeof(CARD32))
#else	/* for compilation outside sun environment */
#define	xPropNewOLWinAttr		xPropOLWinAttr
#define	NumPropNewOLWinAttrElements NumPropOLWinAttrElements
#endif

typedef struct {
	CARD32	flags;
	CARD32	fg_red;
	CARD32	fg_green;
	CARD32	fg_blue;
	CARD32	bg_red;
	CARD32	bg_green;
	CARD32	bg_blue;
	CARD32	bd_red;
	CARD32	bd_green;
	CARD32	bd_blue;
} xPropOLWinColors;

#define NumPropOLWinColorsElements 10


	/* "Pre-defined" Atoms */

Atom	WM_DECORATION_HINTS = 0;
Atom	WM_DISMISS = 0;

Atom	WM_WINDOW_MOVED = 0;
Atom	WM_TAKE_FOCUS = 0;
Atom	WM_SAVE_YOURSELF = 0;
Atom	WM_DELETE_WINDOW = 0;
Atom	BANG = 0;

Atom	_OL_HELP_KEY = 0;

Atom	_OL_WIN_ATTR = 0;
Atom	_OL_WT_BASE = 0;
Atom	_OL_WT_CMD = 0;
Atom	_OL_WT_PROP = 0;
Atom	_OL_WT_HELP = 0;
Atom	_OL_WT_NOTICE = 0;
Atom	_OL_WT_OTHER = 0;

Atom	_OL_DECOR_ADD = 0;
Atom	_OL_DECOR_DEL = 0;
Atom	_OL_DECOR_CLOSE = 0;
Atom	_OL_DECOR_RESIZE = 0;
Atom	_OL_DECOR_HEADER = 0;
Atom	_OL_DECOR_PIN = 0;

Atom	_OL_WIN_COLORS = 0;
Atom	_OL_PIN_STATE = 0;
Atom	_OL_WIN_BUSY = 0;

Atom	_OL_MENU_FULL = 0;
Atom	_OL_MENU_LIMITED = 0;
Atom	_OL_NONE = 0;

Atom	_OL_COPY = 0;
Atom	_OL_CUT = 0;

Atom	WM_STATE = 0;
Atom	WM_CHANGE_STATE = 0;
Atom	WM_ICON_SIZE = 0;
Atom	WM_PROTOCOLS = 0;

#ifndef XGETTEXT
const char OlMsgsDomain[] = "SUNW_WST_LIBXOL";
#endif

#ifndef	sun
PROPERTY(_OL_WSM_QUEUE) = 0;
PROPERTY(_OL_WSM_REPLY) = 0;

PROPERTY(_OL_FM_QUEUE) = 0;
PROPERTY(_OL_FM_REPLY) = 0;


	/* Initialization routines for Open Look Properties */

/*
 * WSMInitialize
 *
 */

extern void WSMInitialize(dpy)
Display * dpy;
{

INTERN(_OL_WSM_QUEUE);
INTERN(_OL_WSM_REPLY);

} /* end of WSMInitialize */


/*
 * FMInitialize
 *
 */

extern void FMInitialize(dpy)
Display * dpy;
{

INTERN(_OL_FM_QUEUE);
INTERN(_OL_FM_REPLY);

} /* end of FMInitialize */
#endif


void
InitializeOpenLook(Display *dpy)
{

	if (dpy == toplevelDisplay) {
		WM_DECORATION_HINTS = OlInternAtom(dpy, WM_DECORATION_HINTS_NAME);
		WM_DISMISS	 = OlInternAtom(dpy, WM_DISMISS_NAME);

		WM_WINDOW_MOVED  = OlInternAtom(dpy, WM_WINDOW_MOVED_NAME);
		WM_TAKE_FOCUS	 = OlInternAtom(dpy, WM_TAKE_FOCUS_NAME);
		BANG		 = OlInternAtom(dpy, BANG_NAME);

		_OL_HELP_KEY	 = OlInternAtom(dpy, _OL_HELP_KEY_NAME);
		_OL_WIN_ATTR	 = OlInternAtom(dpy, _OL_WIN_ATTR_NAME);
		_OL_WT_BASE	 = OlInternAtom(dpy, _OL_WT_BASE_NAME);
		_OL_WT_CMD	 = OlInternAtom(dpy, _OL_WT_CMD_NAME);
		_OL_WT_PROP	 = OlInternAtom(dpy, _OL_WT_PROP_NAME);
		_OL_WT_HELP	 = OlInternAtom(dpy, _OL_WT_HELP_NAME);
		_OL_WT_NOTICE	 = OlInternAtom(dpy, _OL_WT_NOTICE_NAME);
		_OL_WT_OTHER	 = OlInternAtom(dpy, _OL_WT_OTHER_NAME);
		_OL_DECOR_ADD	 = OlInternAtom(dpy, _OL_DECOR_ADD_NAME);
		_OL_DECOR_DEL	 = OlInternAtom(dpy, _OL_DECOR_DEL_NAME);
		_OL_DECOR_CLOSE  = OlInternAtom(dpy, _OL_DECOR_CLOSE_NAME);
		_OL_DECOR_RESIZE = OlInternAtom(dpy, _OL_DECOR_RESIZE_NAME);
		_OL_DECOR_HEADER = OlInternAtom(dpy, _OL_DECOR_HEADER_NAME);
		_OL_DECOR_PIN	 = OlInternAtom(dpy, _OL_DECOR_PIN_NAME);
		_OL_WIN_COLORS	 = OlInternAtom(dpy, _OL_WIN_COLORS_NAME);
		_OL_PIN_STATE	 = OlInternAtom(dpy, _OL_PIN_STATE_NAME);
		_OL_WIN_BUSY	 = OlInternAtom(dpy, _OL_WIN_BUSY_NAME);
		_OL_MENU_FULL	 = OlInternAtom(dpy, _OL_MENU_FULL_NAME);
		_OL_MENU_LIMITED = OlInternAtom(dpy, _OL_MENU_LIMITED_NAME);
		_OL_NONE	 = OlInternAtom(dpy, _OL_NONE_NAME);
		_OL_COPY	 = OlInternAtom(dpy, _OL_COPY_NAME);
		_OL_CUT		 = OlInternAtom(dpy, _OL_CUT_NAME);

		WM_STATE	 = OlInternAtom(dpy, WM_STATE_NAME);
		WM_CHANGE_STATE	 = OlInternAtom(dpy, WM_CHANGE_STATE_NAME);
		WM_ICON_SIZE	 = OlInternAtom(dpy, WM_ICON_SIZE_NAME);
		WM_PROTOCOLS	 = OlInternAtom(dpy, WM_PROTOCOLS_NAME);

		WM_SAVE_YOURSELF = OlInternAtom(dpy, WM_SAVE_YOURSELF_NAME);
		WM_DELETE_WINDOW = OlInternAtom(dpy, WM_DELETE_WINDOW_NAME);
	}

	OlDnDInitialize(dpy);	/* initialize drag and drop stuff */
}


	/* Get Property routines for Open Look Properties */

Status
GetHelpKeyMessage(Display *dpy, XEvent *ev, Window *window_return, int *x_return, int *y_return, int *root_x_return, int *root_y_return)
{
	if (ev->type != ClientMessage ||
				ev->xclient.message_type != _OL_HELP_KEY)
	{
		*window_return = (Window) NULL;
		*x_return = 0;
		*y_return = 0;
		*root_x_return = 0;
		*root_y_return = 0;
		return ~Success;
	}

	*window_return = ev->xclient.data.l[0];
	*x_return = ev->xclient.data.l[1];
	*y_return = ev->xclient.data.l[2];
	*root_x_return = ev->xclient.data.l[3];
	*root_y_return = ev->xclient.data.l[4];

	return Success;
}


GetWMDecorationHints(Display *dpy, Window w, WMDecorationHints *wmdh)
{
	Atom			atr;
	int			afr;
	unsigned long		nir,
				bar;
	xPropWMDecorationHints	*prop;
	int			Failure;
	Atom			atom;

	atom = OlInternAtom(dpy, WM_DECORATION_HINTS_NAME);
	if ((Failure = XGetWindowProperty(dpy, w, atom, 0L,
				NumPropWMDecorationHintsElements, False,
				atom, &atr, &afr, &nir, &bar,
				(unsigned char **) (&prop))) != Success)
		return Failure;

        if (atr != atom ||
			nir < NumPropWMDecorationHintsElements || afr != 32)
	{
		if (prop != (xPropWMDecorationHints *) 0)
			XtFree((char *)prop);

                return BadValue;
	}
	else
	{
		wmdh->flags = prop->flags;
		wmdh->menu_type = prop->menu_type;
		wmdh->pushpin_initial_state = prop->pushpin_initial_state;
		if (prop != (xPropWMDecorationHints *) 0)
			XtFree((char *)prop);

		return Success;
	}
}

#ifndef	sun
typedef struct {
	CARD32		state;
} xPropOLManagerState;

#define NumPropOLManagerStateElements 1

GetOLManagerState(dpy,scr)
Display *dpy;
Screen *scr;
{
	OLManagerState	wms;
	Atom		atr;
	int		afr;
	unsigned long	nir,
			bar;
	xPropOLManagerState	*prop;
	Window rootwin = RootWindowOfScreen(scr);

	if (XGetWindowProperty(dpy, rootwin, OL_MANAGER_STATE, 0L,
				NumPropOLManagerStateElements, False,
				OL_MANAGER_STATE, &atr, &afr, &nir, &bar,
				(unsigned char **) (&prop)) != Success)
	{
		return(0);
	}

        if (atr != OL_MANAGER_STATE || nir < NumPropOLManagerStateElements ||
								 afr != 32)
	{
		if (prop != (xPropOLManagerState *) 0)
			XtFree ((char *)prop);

		return(0);
	}
	else
	{
		wms.state = prop->state;
		if (prop != (xPropOLManagerState *) 0)
			XtFree((char *)prop);
	}

	return wms.state;
}
#endif



Bool
OlIsFMRunning(Display *dpy, Screen *scr)
{
		return(False);
}

Bool
OlIsWSMRunning(Display *dpy, Screen *scr)
{
		return(False);
}

Bool
OlIsWMRunning(Display *dpy, Screen *scr)
{
		return(False);
}


GetWMState(Display *dpy, Window w)
{
	WMState		wms;
	Atom		atr;
	int		afr;
	unsigned long	nir,
			bar;
	xPropWMState	*prop;
	Atom		atom;

	atom = OlInternAtom(dpy, WM_STATE_NAME);
	if (XGetWindowProperty(dpy, w, atom, 0L,
				NumPropWMStateElements, False,
				atom, &atr, &afr, &nir, &bar,
				(unsigned char **) (&prop)) != Success)
	{
		/*
		fprintf(stderr, "WM_STATE property not found\n");
		 */
		return NormalState;		/* Punt */
	}

        if (atr != atom || nir < NumPropWMStateElements || afr != 32)
	{
		if (prop != (xPropWMState *) 0)
			XtFree ((char *)prop);

		/*
		fprintf(stderr, "WM_STATE property not found\n");
		 */
		wms.state =  NormalState;		/* Punt */
	}
	else
	{
		wms.state = prop->state;
		if (prop != (xPropWMState *) 0)
			XtFree((char *)prop);
	}

	return wms.state;
}


GetWMWindowBusy(Display *dpy, Window w, long int *state)
{
	Atom		atr;
	int		afr;
	unsigned long	nir,
			bar;
	long		*value;
	int		Failure;

	if ((Failure = XGetWindowProperty(dpy, w,
				OlInternAtom(dpy, _OL_WIN_BUSY_NAME),
				0L, 1L, False,
				XA_INTEGER, &atr, &afr, &nir, &bar,
				(unsigned char **) (&value))) != Success)
		return Failure;

        if (atr != XA_INTEGER || nir < 1L || afr != 32)
	{
		if (value != (long *) 0)
			XtFree ((char *) value);

                return BadValue;
	}
	else
	{
		*state = *value;
		if (value != (long *) 0)
			XtFree((char *) value);

		return Success;
	}
}


GetWMPushpinState(Display *dpy, Window w, long int *state)
{
	Atom		atr;
	int		afr;
	unsigned long	nir,
			bar;
	long		*value;
	int		Failure;

	if ((Failure = XGetWindowProperty(dpy, w, 
				OlInternAtom(dpy, _OL_PIN_STATE_NAME),
				0L, 1L, False,
				XA_INTEGER, &atr, &afr, &nir, &bar,
				(unsigned char **) (&value))) != Success)
		return Failure;

        if (atr != XA_INTEGER || nir < 1L || afr != 32)
	{
		if (value != (long *) 0)
			XtFree ((char *) value);

                return BadValue;
	}
	else
	{
		*state = *value;
		if (value != (long *) 0)
			XtFree((char *) value);

		return Success;
	}
}


GetOLWinColors(Display *dpy, Window win, OLWinColors *win_colors)
{
	Atom			atr;
	int			afr;
	unsigned long		nir,
				bar;
	xPropOLWinColors	*color_struct;
	int			Failure;
	Atom			atom;

	atom = OlInternAtom(dpy, _OL_WIN_COLORS_NAME);
	if ((Failure = XGetWindowProperty(dpy, win, atom,
				0L, NumPropOLWinColorsElements, False,
				atom, &atr, &afr, &nir, &bar,
				(unsigned char **) (&color_struct))) != Success)
		return Failure;

        if (atr != atom || nir < NumPropOLWinColorsElements || afr != 32)
	{
		if (color_struct != (xPropOLWinColors *) 0)
			XtFree((char *) color_struct);

                return BadValue;
	}
	else
	{
		win_colors->flags = color_struct->flags;
		win_colors->fg_red = color_struct->fg_red;
		win_colors->fg_green = color_struct->fg_green;
		win_colors->fg_blue = color_struct->fg_blue;
		win_colors->bg_red = color_struct->bg_red;
		win_colors->bg_green = color_struct->bg_green;
		win_colors->bg_blue = color_struct->bg_blue;
		win_colors->bd_red = color_struct->bd_red;
		win_colors->bd_green = color_struct->bd_green;
		win_colors->bd_blue = color_struct->bd_blue;

		if (color_struct != (xPropOLWinColors *) 0)
			XtFree((char *) color_struct);

		return Success;
	}
}

GetOLWinAttr(Display *dpy, Window client_window, OLWinAttr *olwa)
{
	Atom		atr;
	int		afr;
	unsigned long	nir,
			bar;
	xPropOLWinAttr	*prop;
	int		Failure;
	Atom		atom;
#ifdef	sun
	int			numtofetch;
	unsigned char 		**ptr;
	xPropNewOLWinAttr	*lprop;

	if (_OlUseShortOLWinAttr(dpy)) {
		numtofetch = NumPropOLWinAttrElements;
		ptr = (unsigned char **)&prop;
	} else { 
		numtofetch = NumPropNewOLWinAttrElements;
		ptr = (unsigned char **)&lprop;
	}
#endif

	atom = OlInternAtom(dpy, _OL_WIN_ATTR_NAME);
	if ((Failure = XGetWindowProperty(dpy, client_window, atom, 0L,
#ifndef	sun
				NumPropOLWinAttrElements, False,
#else
				numtofetch, False,
#endif
				atom, &atr, &afr, &nir, &bar,
#ifndef	sun
				(unsigned char **) (&prop))) != Success)
#else
				ptr)) != Success)
#endif
		return Failure;

#ifndef	sun
        if (atr != atom || nir < NumPropOLWinAttrElements || afr != 32)
	{
		if (prop != (xPropOLWinAttr *) 0)
			XtFree((char *) prop);
#else
        if (atr != atom || nir != numtofetch || afr != 32)
#endif
	{
		if (*ptr != (unsigned char *) 0)
			XtFree((char *) *ptr);

                return BadValue;
	}
	else
	{
#ifdef	sun
		if (_OlUseShortOLWinAttr(dpy)) {
#endif
#ifndef	sun
			olwa->flags = prop->flags;
#endif
			olwa->win_type = prop->win_type;
			olwa->menu_type = prop->menu_type;
			olwa->pin_state = prop->pin_state;
#ifndef	sun
			olwa->cancel = prop->cancel;
#endif
#ifdef	sun
		} else {
			/* olwa->flags = lprop->flags; */
			olwa->win_type = lprop->win_type;
			olwa->menu_type = lprop->menu_type;
			olwa->pin_state = lprop->pin_state;
			/* olwa->cancel = lprop->cancel; */
		}
#endif
#ifndef	sun
		if (prop != (xPropOLWinAttr *) 0)
			XtFree((char *) prop);
#else
		if (*ptr != (unsigned char *) 0)
			XtFree((char *) *ptr);
#endif

		return Success;
	}
}

#ifdef	sun
void
SetOLWinAttr(Display *dpy, Window w, OLWinAttr *olwa)
{
        xPropOLWinAttr  	prop;
	xPropNewOLWinAttr	lprop;
	int			numtowrite;
	unsigned char 		**ptr;
	Atom			atom;

        if (olwa == (OLWinAttr *) 0)
                return;

	if (_OlUseShortOLWinAttr(dpy)) {
		numtowrite = NumPropOLWinAttrElements;
		ptr = (unsigned char **)&prop;
#ifndef	sun
		prop.flags = olwa->flags;
#endif
		prop.win_type = olwa->win_type;
		prop.menu_type = olwa->menu_type;
		prop.pin_state = olwa->pin_state;
#ifndef	sun
		prop.cancel = olwa->cancel;
#endif
	} else {
		numtowrite = NumPropNewOLWinAttrElements;
		ptr = (unsigned char **)&lprop;
		lprop.flags = _OL_WA_WIN_TYPE | _OL_WA_MENU_TYPE | 
			     _OL_WA_PIN_STATE;
		lprop.win_type = olwa->win_type;
		lprop.menu_type = olwa->menu_type;
		lprop.pin_state = olwa->pin_state;
		lprop.cancel = 0L;
	}

	atom = OlInternAtom(dpy, _OL_WIN_ATTR_NAME);
        XChangeProperty(dpy, w, atom, atom, 32,
			PropModeReplace, (unsigned char*)ptr,
			numtowrite);

        XFlush(dpy);
}

GetNewOLWinAttr(Display *dpy, Window client_window, NewOLWinAttr *olwa)
{
	Atom			atr;
	int			afr;
	unsigned long		nir,
				bar;
	xPropOLWinAttr		*prop;
	int			Failure;
	int			numtofetch;
	unsigned char 		**ptr;
	xPropNewOLWinAttr	*lprop;
	Atom			atom;

	if (_OlUseShortOLWinAttr(dpy)) {
		numtofetch = NumPropOLWinAttrElements;
		ptr = (unsigned char **)&prop;
	} else { 
		numtofetch = NumPropNewOLWinAttrElements;
		ptr = (unsigned char **)&lprop;
	}

	atom = OlInternAtom(dpy, _OL_WIN_ATTR_NAME);
	if ((Failure = XGetWindowProperty(dpy, client_window, atom, 0L,
				numtofetch, False,
				atom, &atr, &afr, &nir, &bar,
				ptr)) != Success)
		return Failure;

        if (atr != _OL_WIN_ATTR || nir != numtofetch || afr != 32)
	{
		if (*ptr != (unsigned char *) 0)
			XtFree((char *) *ptr);

                return BadValue;
	}
	else
	{
		if (_OlUseShortOLWinAttr(dpy)) {
			olwa->flags = _OL_WA_WIN_TYPE | _OL_WA_MENU_TYPE | _OL_WA_PIN_STATE;
			olwa->win_type = prop->win_type;
			olwa->menu_type = prop->menu_type;
			olwa->pin_state = prop->pin_state;
			olwa->cancel = 0; 
		} else {
			olwa->flags = lprop->flags;
			olwa->win_type = lprop->win_type;
			olwa->menu_type = lprop->menu_type;
			olwa->pin_state = lprop->pin_state;
			olwa->cancel = lprop->cancel;
		}
		if (*ptr != (unsigned char *) 0)
			XtFree((char *) *ptr);

		return Success;
	}
}

void
SetNewOLWinAttr(Display *dpy, Window w, NewOLWinAttr *olwa)
{
        xPropOLWinAttr  	prop;
	xPropNewOLWinAttr	lprop;
	int			numtowrite;
	unsigned char		**ptr;
	Atom			atom;

        if (olwa == (NewOLWinAttr *) 0)
                return;

	if (_OlUseShortOLWinAttr(dpy)) {
		numtowrite = NumPropOLWinAttrElements;
		ptr = (unsigned char **)&prop;
		if (olwa->flags &  _OL_WA_WIN_TYPE)
			prop.win_type = olwa->win_type;
		else
			prop.win_type = OlInternAtom(dpy, _OL_WT_OTHER_NAME);
		if (olwa->flags & _OL_WA_MENU_TYPE)
			prop.menu_type = olwa->menu_type;
		else
			prop.menu_type = OlInternAtom(dpy, _OL_NONE_NAME);
		if (olwa->flags & _OL_WA_PIN_STATE)
			prop.pin_state = olwa->pin_state;
	} else {
		numtowrite = NumPropNewOLWinAttrElements;
		ptr = (unsigned char **)&lprop;
		lprop.flags = olwa->flags;
		if (olwa->flags &  _OL_WA_WIN_TYPE)
			lprop.win_type = olwa->win_type;
		if (olwa->flags & _OL_WA_MENU_TYPE)
			lprop.menu_type = olwa->menu_type;
		if (olwa->flags & _OL_WA_PIN_STATE)
			lprop.pin_state = olwa->pin_state;
		if (olwa->flags & _OL_WA_CANCEL)
			lprop.cancel = olwa->cancel;
	}

	atom = OlInternAtom(dpy, _OL_WIN_ATTR_NAME);
        XChangeProperty(dpy, w, atom, atom, 32,
			PropModeReplace, (unsigned char*)ptr, numtowrite);

        XFlush(dpy);
}


#endif

	/* Set Property routines for Open Look Properties */

#ifndef	sun
void
SendProtocolMessage(dpy, w, protocol, time)
Display *dpy;
Window w;
Atom protocol;
unsigned long time;
{
	XEvent	sev;

	sev.xclient.type = ClientMessage;
	sev.xclient.display = dpy;
	sev.xclient.window = w;
	sev.xclient.message_type = WM_PROTOCOLS;
	sev.xclient.format = 32;
	sev.xclient.data.l[0] = (long) protocol;
	sev.xclient.data.l[1] = time;

	XSendEvent(dpy, w, False, NoEventMask, &sev);
}
#endif


void
SetWMDecorationHints(Display *dpy, Window w, WMDecorationHints *wmdh)
{
	xPropWMDecorationHints	prop;
	Atom			atom;

	if (wmdh == (WMDecorationHints *) 0)
		return;

	prop.flags = wmdh->flags;
	prop.menu_type = wmdh->menu_type;
	prop.pushpin_initial_state = wmdh->pushpin_initial_state;

	atom = OlInternAtom(dpy, WM_DECORATION_HINTS_NAME);
	XChangeProperty(dpy, w, atom, atom, 32,
				PropModeReplace, (unsigned char *) &prop,
				NumPropWMDecorationHintsElements);

	XFlush(dpy);
}


void
SetWMState(Display *dpy, Window w, WMState *wms)
{
	xPropWMState	prop;
	Atom		atom;

	if (wms == (WMState *) 0)
		return;

	prop.state = wms->state;
	prop.icon = wms->icon;

	atom = OlInternAtom(dpy, WM_STATE_NAME);
	XChangeProperty(dpy, w, atom, atom, 32,
				PropModeReplace, (unsigned char *) &prop,
				NumPropWMStateElements);

	XFlush(dpy);
}


void
SetWMWindowBusy(Display *dpy, Window w, long int state)
{
	long	value = state;

	XChangeProperty(dpy, w, OlInternAtom(dpy, _OL_WIN_BUSY_NAME),
			XA_INTEGER, 32, PropModeReplace,
			(unsigned char *) &value, 1);
	XFlush(dpy);
}


void
SetWMPushpinState(Display *dpy, Window w, long int state)
{
	long	value = state;

	XChangeProperty(dpy, w, OlInternAtom(dpy, _OL_PIN_STATE_NAME),
			XA_INTEGER, 32, PropModeReplace,
			(unsigned char *) &value, 1);
	XFlush(dpy);
}


void
SetWMIconSize(Display *dpy, Window w, WMIconSize *wmis)
{
	xPropWMIconSize	prop;
	Atom		atom;

	prop.min_width = wmis->min_width;
	prop.min_height = wmis->min_height;
	prop.max_width = wmis->max_width;
	prop.max_height = wmis->max_height;
	prop.width_inc = wmis->width_inc;
	prop.height_inc = wmis->height_inc;

	atom = OlInternAtom(dpy, WM_ICON_SIZE_NAME);
	XChangeProperty(dpy, w, atom, atom, 32,
				PropModeReplace, (unsigned char *) &prop,
				NumPropWMIconSizeElements);

	XFlush(dpy);
}


/*
 * GetCharProperty (generic routine to get a char type property)
 */

char * GetCharProperty(Display *dpy, Window w, Atom property, int *length)
{
	Atom			actual_type;
	int			actual_format;
	unsigned long		num_items;
	unsigned long		bytes_remaining = 1;
	char *                  buffer;
	char *                  Buffer;
	int			Buffersize = 0;
	int			Result;
	int			EndOfBuffer;
	register		i;

	Buffer = (char *) XtMalloc(1);
	Buffer[0] = '\0';
	EndOfBuffer = 0;
	do
	{
	if ((Result = XGetWindowProperty(dpy, w, property, 
		(long) ((Buffersize+3) /4),
		(long) ((bytes_remaining+3) / 4), True,
		XA_STRING, &actual_type, &actual_format, 
		&num_items, &bytes_remaining,
		(unsigned char **) &buffer)) != Success)
		{
		if (buffer) XtFree(buffer);
		if (Buffer) XtFree(Buffer);
		*length = 0;
		return NULL;
		}
        
	if (buffer) 
		{
		register int i;
		Buffersize += num_items;
		Buffer = (char *) XtRealloc(Buffer, Buffersize + 1);
		if (Buffer == NULL) 
			{
			XtFree(buffer);
			*length = 0;
			return NULL;
			}
		for (i = 0; i < num_items; i++)
		   Buffer[EndOfBuffer++] = buffer[i];
		Buffer[EndOfBuffer] = '\0';
		XtFree(buffer);
		}
	} while (bytes_remaining > 0);

	*length = Buffersize;
	if (Buffersize == 0 && Buffer != NULL)
		{
		XtFree (Buffer);
		return NULL;
		}
	else
		return (Buffer);

} /* end of GetCharProperty */
/*
 * GetLongProperty
 */

Status GetLongProperty(Display *dpy, Window w, Atom property, long int *result)
{
	Atom			actual_type;
	int			actual_format;
	unsigned long		num_items,
				bytes_remaining;
	int			Result;
	long *			value;

	if ((Result = XGetWindowProperty(dpy, w, property, 0L,
				1L, False,
				property, &actual_type, &actual_format, 
                                &num_items, &bytes_remaining,
				(unsigned char **) &value)) != Success)
		return Result;
	else
		{
		*result = *value;
		XtFree((char*)value);
	        if (actual_type != property)
			return 42 /* BadValue */;
		else
			return 15 /* Success */;
		}

} /* end of GetLongProperty */


Atom*
GetAtomList(Display *dpy, Window w, Atom property, int *length, int delete)
{
	Atom                    actual_type;
	unsigned long           num_items,
	                        bytes_remaining;
	Atom                   *buffer = (Atom *)0;
	int                     actual_format;

	if (XGetWindowProperty(dpy, w, property,
			(long)0, (long)1, False,
			XA_ATOM, &actual_type, &actual_format,
			&num_items, &bytes_remaining,
			(unsigned char **)&buffer) != Success) {
		if (buffer)
			XtFree((char *)buffer);

		*length = 0;
		return (Atom *)0;
	}
	
	if (buffer)
		XtFree((char *)buffer);

	if (XGetWindowProperty(dpy, w, property,
			(long)0,
			(long)(1 + bytes_remaining / 4), delete,
			actual_type, &actual_type, &actual_format,
			&num_items, &bytes_remaining,
			(unsigned char **)&buffer) != Success) {
		if (buffer)
			XtFree((char *)buffer);
		*length = 0;
		return NULL;
	}
	
	*length = num_items;
	
	return buffer;
}
