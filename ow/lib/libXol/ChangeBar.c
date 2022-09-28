#pragma ident	"@(#)ChangeBar.c	302.4	97/03/26 lib/libXol SMI"	/* changebar:ChangeBar.c 1.8	*/

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


#include <X11/IntrinsicP.h>
#include <X11/StringDefs.h>

#include <Xol/Category.h>
#include <Xol/ChangeBar.h>
#include <Xol/ControlAre.h>
#include <Xol/MenuP.h>
#include <Xol/OlCursors.h>
#include <Xol/OpenLookP.h>


/*
 * Global data:
 */

char			XtNallowChangeBars [] = "allowChangeBars";
char			XtNchangeBar	   [] = "changeBar";

char			XtCAllowChangeBars [] = "AllowChangeBars";
char			XtCChangeBar	   [] = "ChangeBar";

/*
 * Local routines:
 */

static void		PropagateToCategory (
	Widget			w,
	OlDefine		state,
	unsigned int		propagate
);

/**
 ** _OlCreateChangeBar()
 **/

ChangeBar *
_OlCreateChangeBar (
	Widget	w
)
{
	ChangeBar *		cb	= XtNew(ChangeBar);


	memset (cb, 0, sizeof(ChangeBar));

#define PtToPixel(AXIS,V) OlScreenPointToPixel(AXIS,V,XtScreenOfObject(w))
	cb->width  = PtToPixel(OL_HORIZONTAL, CHANGE_BAR_WIDTH);
	cb->height = PtToPixel(OL_VERTICAL,   CHANGE_BAR_HEIGHT);
	cb->pad	   = PtToPixel(OL_HORIZONTAL, CHANGE_BAR_PAD);

	return (cb);
} /* _OlCreateChangeBar */

/**
 ** _OlDestroyChangeBar()
 **/

void
_OlDestroyChangeBar (
	Widget			w,
	ChangeBar *		cb
)
{
	if (cb) {
		_OlFreeChangeBarGCs (w, cb);
		XtFree ((char*)cb);
	}

	return;
} /* _OlDestroyChangeBar */

/**
 ** _OlSetChangeBarState()
 **/

void
_OlSetChangeBarState (
	Widget			w,
	OlDefine		state,
	unsigned int		propagate
)
{
	Widget			x = 0;


	if (!w)
		return;

	if (propagate & OL_PROPAGATE_TO_CONTROL_AREA) {
		/*
		 * Find the ancestor of this widget which is a direct
		 * child of a control area. We assume the widget passed
		 * to us isn't a control area.
		 *
		 * Note: Don't stop at a control area that's a descendent
		 * of a MenuShell.
		 */
Loop:		do
			w = XtParent((x = w));
		while (w && XtClass(w) != controlAreaWidgetClass);
		if (w && XtIsSubclass(_OlGetShellOfWidget(w), menuShellWidgetClass))
			goto Loop;
		if (!w)
			x = 0;
	} else
		x = w;

	if (x) {
		XtVaSetValues (x, XtNchangeBar, (XtArgVal)state, (String)0);
		PropagateToCategory (w, state, propagate);
	}

	return;
} /* _OlSetChangeBarState */

/**
 ** _OlFlatSetChangeBarState()
 **/

void
_OlFlatSetChangeBarState (
	Widget			w,
	Cardinal		indx,
	OlDefine		state,
	unsigned int		propagate
)
{
	OlVaFlatSetValues (w, indx, XtNchangeBar, state, (String)0);
	PropagateToCategory (w, state, propagate);

	return;
} /* _OlFlatSetChangeBarState */

/**
 ** _OlDrawChangeBar()
 **/

void
_OlDrawChangeBar (
	Widget			w,
	ChangeBar *		cb,
	OlDefine		state,
	Boolean			expose,
	Position		x,
	Position		y,
	Region			region
)
{
	Display *		display	= XtDisplayOfObject(w);

	Window			window	= XtWindowOfObject(w);

	Dimension		width;
	Dimension		height;

	GC			gc;


	if (!XtIsRealized(w))
		return;

	/*
	 * Get the GCs now, if we don't have them yet. We wait
	 * until the last moment like this so that (1) we don't
	 * do any work if we never need them and (2) so we are
	 * guaranteed to have the resources we need (e.g. a window).
	 */
	if (!cb->normal_GC || !cb->dim_GC)
		_OlGetChangeBarGCs (w, cb);

	width = cb->width;
	height = cb->height;

	/*
	 * If a region was given, see if the change bar is within it.
	 */
	if (region)
		switch (XRectInRegion(region, x, y, width, height)) {

		case RectangleOut:
			return;

		case RectanglePart:
			/*
			 * Don't bother trying to optimize drawing a
			 * partial change bar, as they are pretty small.
			 */
		case RectangleIn:
			break;

		}

	/*
	 * Now draw (or erase) the change bar according to its type.
	 */
	switch ((expose? OL_NONE : state)) {

	case OL_DIM:
		gc = cb->dim_GC;
		goto Draw;
	case OL_NORMAL:
		gc = cb->normal_GC;
Draw:		XFillRectangle (display, window, gc, x, y, width, height);
		break;

	case OL_NONE:
		XClearArea (display, window, x, y, width, height, expose);
		break;

	}

	return;
} /* _OlDrawChangeBar */

/**
 ** _OlGetChangeBarGCs()
 **/

void
_OlGetChangeBarGCs (
	Widget			w,
	ChangeBar *		cb
)
{
	XGCValues		v;


	if (!cb)
		return;

	/*
	 * WARNING:
	 * Never call this routine if the GC is already in the
	 * change bar structure. Call _OlFreeChangeBarGCs() first
	 * if news GC are needed, so that the old GCs are released.
	 */
	if (!cb->normal_GC) {
		v.background = w->core.background_pixel;
		v.foreground = _OlContrastingColor(w, v.background, 25);
		cb->normal_GC = XtGetGC(
			w,
			GCForeground|GCBackground,
			&v
		);
	}
	if (!cb->dim_GC) {
		v.background = w->core.background_pixel;
		v.foreground = _OlContrastingColor(w, v.background, 25);
		v.fill_style = FillOpaqueStippled;
		v.stipple    = OlGet50PercentGrey(XtScreenOfObject(w));
		cb->dim_GC = XtGetGC(
			w,
			GCForeground|GCBackground|GCFillStyle|GCStipple,
			&v
		);
	}

	return;
} /* _OlGetChangeBarGCs */

/**
 ** _OlFreeChangeBarGCs()
 **/

void
_OlFreeChangeBarGCs (
	Widget			w,
	ChangeBar *		cb
)
{
	if (cb->normal_GC) {
		XtReleaseGC (w, cb->normal_GC);
		cb->normal_GC = 0;
	}
	if (cb->dim_GC) {
		XtReleaseGC (w, cb->dim_GC);
		cb->dim_GC = 0;
	}
	return;
} /* _OlFreeChangeBarGCs */

/**
 ** _OlContrastingColor ()
 **/

Pixel
_OlContrastingColor (
	Widget			w,
	Pixel			pixel,
	int			percent
)
{
	static Pixel		prev_pixel;
	static Pixel		contrasting_pixel;

	static Boolean		have_prev_pixel	= False;

	Screen *		screen		= XtScreen(w);


	if (have_prev_pixel && pixel == prev_pixel)
		return (contrasting_pixel);

	if (pixel == OlBlackPixel(w))
		contrasting_pixel = OlWhitePixel(w);

	else if (pixel == OlWhitePixel(w))
		contrasting_pixel = OlBlackPixel(w);

	else {
		Display *		display	= XtDisplayOfObject(w);

		Window			window	= XtWindowOfObject(w);

		XWindowAttributes	attributes;

		XColor			xcolor;

		unsigned long		intensity;


		XGetWindowAttributes (display, window, &attributes);
		
		xcolor.pixel = pixel;
		XQueryColor (display, attributes.colormap, &xcolor);

# define F(C) (unsigned long)C ## L
		intensity =
		      (xcolor.flags & DoRed?   F(39) * xcolor.red   : 0)
		    + (xcolor.flags & DoGreen? F(50) * xcolor.green : 0)
		    + (xcolor.flags & DoBlue?  F(11) * xcolor.blue  : 0);
#undef F

		if (intensity > (unsigned long)(65535L * percent))
			contrasting_pixel = OlBlackPixel(w);
		else
			contrasting_pixel = OlWhitePixel(w);
	}

	prev_pixel = pixel;
	have_prev_pixel = True;
	return (contrasting_pixel);
} /* _OlContrastingColor */

/**
 ** PropagateToCategory()
 **/

static void
PropagateToCategory (
	Widget			w,
	OlDefine		state,
	unsigned int		propagate
)
{
	Widget			x;


	/*
	 * We propagate the change indication up to a category
	 * widget only if (1) we've been asked to, and (2) if
	 * the change bar state is dim or normal. We don't
	 * propagate the change up to a category widget if the
	 * change bar state is ``none'', because the category
	 * ``changed'' state is the union of potentially several
	 * changes. The client has to clear a category ``changed''
	 * state manually.
	 */
	if (propagate & OL_PROPAGATE_TO_CATEGORY && state != OL_NONE) {
		do
			w = XtParent((x = w));
		while (w && XtClass(w) != categoryWidgetClass);
		if (x)
			XtVaSetValues (
				x,
				XtNchanged, (XtArgVal)True,
				(String)0
			);
	}
	return;
} /* PropagateToCategory */
