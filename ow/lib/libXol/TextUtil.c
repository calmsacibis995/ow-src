#pragma ident	"@(#)TextUtil.c	302.7	92/10/22 lib/libXol SMI"	/* textedit:TextUtil.c 1.5	*/

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

#include <X11/IntrinsicP.h>
#include <X11/Xatom.h>

#include <Xol/OpenLookP.h>
#include <Xol/Dynamic.h>
#include <Xol/RootShell.h>
#include <Xol/PrimitiveP.h>
#include <Xol/TextUtil.h>
#include <Xol/Util.h>
#include <Xol/OlStrMthdsI.h>

/*
 * BEWARE: Do *NOT* add any code until after the #undef char.
 * See the comment in OlCursors.c for a description of what we are doing
 * here.
 */
#define char _OlReadOnlyBitmapType
#include <Xol/bitmaps/textdupe_drag.xbm>
#include <Xol/bitmaps/textdupe_drag_mask.xbm>
#include <Xol/bitmaps/textdupe_insert.xbm>
#include <Xol/bitmaps/textdupe_insert_mask.xbm>
#include <Xol/bitmaps/textdupe_nodrop.xbm>
#include <Xol/bitmaps/textdupe_nodrop_mask.xbm>
 
#include <Xol/bitmaps/textmove_drag.xbm>
#include <Xol/bitmaps/textmove_drag_mask.xbm>
#include <Xol/bitmaps/textmove_insert.xbm>
#include <Xol/bitmaps/textmove_insert_mask.xbm>
#include <Xol/bitmaps/textmove_nodrop.xbm>
#include <Xol/bitmaps/textmove_nodrop_mask.xbm>
#undef char


typedef struct _PropertyEventInfo
   {
   Display * display;
   Window    window;
   Atom      atom;
   } PropertyEventInfo;


static Bool _IsPropertyNotify(Display *d, XEvent *event, char *arg);


/*
 * _IsGraphicsExpose
 *
 * The \fI_IsGraphicsExpose\fR function is used as a predicate function
 * for check event processing.  It is normally used to determine if
 * GraphicsExpose events caused by scrolling (XCopyArea) window contents
 * has occurred.  Note a XCopyArea is guaranteed to generate at least
 * one NoExpose event and this event will follow all necessary GraphicsExpose
 * events caused by the copy.
 *
 * Synopsis:
 *
 *#include <TextUtil.h>
 * ...
 */

extern Bool
_IsGraphicsExpose(Display *d, XEvent *event, char *arg)
{

return (Bool)(event-> xany.window == (Window)arg
                                  &&
             (event-> type == GraphicsExpose || event-> type == NoExpose));

} /* _IsGraphicsExpose() */


/*
 * _OlCreateCursorFromBitmaps
 *
 * The \fI_OlCreateCursorFromBitmaps\fR procedure is used to create the
 * the TextEdit/TextLine  drag-and-drop cursor.  It uses the bitmaps created
 * by the _OlCreateDnDCursors procedure ; clearing the text area
 * in the source bitmap then drawing the text into the source.
 *
 */

Cursor
_OlCreateCursorFromBitmaps(Display*  dpy, Widget widget,
        Pixmap source, Pixmap mask, GC gc, char* foreground, char* background,
        OlStr s, int l, Position x, Position y, int XHot, int YHot,
        Pixmap more, OlStrRep rep, OlFont fs)
{
XColor    fg;
XColor    bg;
XColor    xfg;
XColor    xbg;
 
XAllocNamedColor(dpy, widget->core.colormap, foreground, &fg, &xfg);
XAllocNamedColor(dpy, widget->core.colormap, background, &bg, &xbg);
 
XSetFunction(dpy, gc, GXcopy);
 
/*
 * note: the rectangle values can be set in the create function
 *       and used in the fill rectangle below
 *
 */
 
XFillRectangle(dpy, source, gc, 20, 23, 29, 9);
XSetFunction(dpy, gc, GXcopyInverted);
 
/*
 * note: x and y should be calculated in the routines here!
 *
 */
 
(*str_methods[rep].StrDraw)(dpy,source,fs,gc,x,y,s,l);
 
if (more)
   XCopyPlane(dpy, more, source, gc, 0, 0, 5, 9, 35, 10, 1L);
 
 
return (XCreatePixmapCursor(dpy, source, mask, &fg, &bg, XHot, YHot));

} /* end of _OlCreateCursorFromBitmaps */


/*
 *	Create the animation cursors required for DnD animation ... 
 *  Don't forget to call _OLFreeDnDCursors when you are done ...
 */

OlDnDCursors
_OlCreateDnDCursors(Widget w, OlStr string, OlFont font, OlStrRep format, OlDragMode drag_mode)
{
	Display        *dpy = XtDisplay(w);
	Window          win = RootWindowOfScreen(XtScreen(w));

	GC              CursorGC;

	Pixmap          DragSource;
	Pixmap          DragMask;
	Pixmap          DropSource;
	Pixmap          DropMask;
	Pixmap          NoDropSource;
	Pixmap          NoDropMask;

	int		len;
	XRectangle      rect;
	 
	OlDnDCursors 	cursors;

	rect.x = 21;                /* this really sucks big time!! */
	rect.y = 18;
	rect.width = 29;
	rect.height = 13;

#ifndef __STDC__
#define MKBM(bm_name)                                                   \
                XCreateBitmapFromData(dpy, win,                         \
                              (char *)bm_name/**/_bits,                 \
                              bm_name/**/_width, bm_name/**/_height)
#else
#define MKBM(bm_name)                                                   \
                XCreateBitmapFromData(dpy, win,                         \
                              (char *)bm_name ## _bits,                 \
                              bm_name ## _width, bm_name ## _height)
#endif

	if (drag_mode == OlMoveDrag) {
        	DragSource = MKBM(textmove_drag);
        	DragMask = MKBM(textmove_drag_mask);
        	DropSource = MKBM(textmove_insert);
        	DropMask = MKBM(textmove_insert_mask);
        	NoDropSource = MKBM(textmove_nodrop);
        	NoDropMask = MKBM(textmove_nodrop_mask);
    	} else {                    /* OlCopyDrag */
        	DragSource = MKBM(textdupe_drag);
        	DragMask = MKBM(textdupe_drag_mask);
        	DropSource = MKBM(textdupe_insert);
        	DropMask = MKBM(textdupe_insert_mask);
        	NoDropSource = MKBM(textdupe_nodrop);
        	NoDropMask = MKBM(textdupe_nodrop_mask);
    	}

	if(format == OL_SB_STR_REP) {
        	XGCValues       values;

        	values.font = ((XFontStruct *)font)->fid;
        	CursorGC = XCreateGC(dpy, DragSource, GCFont, (XGCValues *)&values);
    	} else
        	CursorGC = XCreateGC(dpy, DragSource, 0L, (XGCValues *)NULL);

	XSetClipRectangles(dpy, CursorGC, 0, 0, &rect, 1, Unsorted);

	len = (*str_methods[format].StrNumUnits)(string);

	cursors.Drag = _OlCreateCursorFromBitmaps(dpy, w,
                                              DragSource, DragMask,
                                              CursorGC, "black", "white",
                                              string, len,
                                              rect.x, rect.y+10,
                                              textmove_drag_x_hot,
                                              textmove_drag_y_hot, 0,
                                              format, font);
	cursors.Drop = _OlCreateCursorFromBitmaps(dpy, w,
                                              DropSource, DropMask,
                                              CursorGC, "black", "white",
                                              string, len,
                                              rect.x, rect.y + 10,
                                              textmove_drag_x_hot,
                                              textmove_drag_y_hot, 0,
                                              format, font);
	cursors.NoDrop = _OlCreateCursorFromBitmaps(dpy, w,
                                              NoDropSource, NoDropMask,
                                              CursorGC, "black", "white",
                                              string, len,
                                              rect.x, rect.y + 10,
                                              textmove_drag_x_hot,
                                              textmove_drag_y_hot, 0,
                                              format, font);
	
/* We CAN Free these since no explicit reference is being made to these later... */
	XFreePixmap(dpy, DragSource);
    	XFreePixmap(dpy, DragMask);
    	XFreePixmap(dpy, DropSource);
    	XFreePixmap(dpy, DropMask);
    	XFreePixmap(dpy, NoDropSource);
    	XFreePixmap(dpy, NoDropMask);
    	XFreeGC(dpy, CursorGC);

	return cursors;
}

void
_OlFreeDnDCursors(Widget w, OlDnDCursors cursors)
{
	Display *dpy = XtDisplay(w);
	
	XFreeCursor(dpy, cursors.Drag);
	XFreeCursor(dpy, cursors.Drop);
	XFreeCursor(dpy, cursors.NoDrop);
}

void
_OlDnDAnimate (Widget widget, int eventcode, Time timestamp, Boolean insensitive, XtPointer closure)
{
	Cursor          c;
	OlDnDCursors *cp = (OlDnDCursors *) closure;

	if (eventcode == EnterNotify) {
		c = (insensitive ? cp->NoDrop : cp->Drop);
	} else {
		c = cp->Drag;
	}
	
	OlChangeGrabbedDragCursor(widget, c);
}


/*
 * _XtLastTimestampProcessed
 *
 */

extern Time
_XtLastTimestampProcessed(Widget widget)
{
PropertyEventInfo lti;
XEvent            event;
Widget		  save = widget;

while (widget != (Widget)NULL && !XtIsRealized(widget))
	widget = XtParent(widget);

if (widget == (Widget)NULL) {
	lti.display = XtDisplay(save);
	lti.window  = RootWindowOfScreen(XtScreen(save));
} else {
	lti.display = XtDisplay(widget);
	lti.window  = XtWindow(widget);
}
lti.atom    = OlInternAtom(XtDisplay(widget), TIMESTAMP_NAME);

XChangeProperty(lti.display, lti.window, lti.atom, 
                XA_STRING, 8, PropModeAppend, (const unsigned char *)"", 0);

XIfEvent(lti.display, &event, _IsPropertyNotify, (char *)&lti);

return (event.xproperty.time);

} /* end of _XtLastTimestampProcessed */
/*
 * _IsPropertyNotify
 *
 * The \fI_IsPropertyNotify\fR function is used as a predicate function
 * for check event processing.  It is normally used to determine when
 * PropertyNotify events caused when attempting to get a timestamp
 * has occurred.
 *
 * Synopsis:
 *
 *#include <TextUtil.h>
 * ...
 */

static Bool
_IsPropertyNotify(Display *d, XEvent *event, char *arg)
{
PropertyEventInfo * propinfo = (PropertyEventInfo *)arg;

#ifdef DEBUG
fprintf(stderr,"type = %d (%d)\ndpy  = %x (%x)\nwin  = %x (%x)atom = %d (%d)\n",
event-> type, PropertyNotify, 
event-> xproperty.display , propinfo-> display ,
event-> xproperty.window  , propinfo-> window  ,
event-> xproperty.atom    , propinfo-> atom);
#endif

return (Bool)(event-> type == PropertyNotify                  &&
              event-> xproperty.display == propinfo-> display &&
              event-> xproperty.window  == propinfo-> window  &&
              event-> xproperty.atom    == propinfo-> atom);

} /* _IsPropertyNotify */

extern ButtonAction
_OlPeekAheadForEvents(Widget	 w,
		   XEvent *	event)
{
    static Time     last_click_time = 0;
    static int      last_x_root = 0, last_y_root = 0;
    static Window   last_root = (Window) 0;
    static Boolean  multi_click_pending = False;
    static Boolean  pointer_grabbed = False;
    Boolean         found_event = False;
    ButtonAction    action = NOT_DETERMINED;
    XEvent          newevent;
    Cardinal        mouse_damping_factor =
    _OlGetMouseDampingFactor(w), multi_click_timeout =
    _OlGetMultiClickTimeout(w);

#define ABS_DELTA(x1, x2)       (x1 < x2 ? x2 - x1 : x1 - x2)

    if (!pointer_grabbed)
	while (XGrabPointer(XtDisplayOfObject(w),
			    XtWindowOfObject(w), False,
			    ButtonPressMask |
			    ButtonMotionMask | ButtonReleaseMask,
			    GrabModeAsync,
			    GrabModeAsync, None, None, CurrentTime)
	       != GrabSuccess);

    found_event = XCheckWindowEvent(XtDisplayOfObject(w),
				    XtWindowOfObject(w),
				    ButtonMotionMask |
				    ButtonReleaseMask, &newevent);

    if (found_event) {
	if ((newevent.type == MotionNotify) &&
	    ((ABS_DELTA(newevent.xmotion.x,
			event->xbutton.x) >=
	      mouse_damping_factor) ||
	     (ABS_DELTA(newevent.xmotion.y,
			event->xbutton.y) >=
	      mouse_damping_factor))) {
	    multi_click_pending = False;
	    pointer_grabbed = False;
	    action = MOUSE_MOVE;
	} else if (newevent.type == ButtonRelease) {
	    if (!multi_click_pending)
		action = MOUSE_CLICK;

	    if ((event->xbutton.time - last_click_time <=
		 multi_click_timeout) &&
		(last_root == event->xbutton.root) &&
		((ABS_DELTA(event->xbutton.x_root,
			    last_x_root) <=
		  mouse_damping_factor) ||
		 (ABS_DELTA(event->xbutton.y_root,
			    last_y_root) <=
		  mouse_damping_factor)) &&
		!(multi_click_pending))
		action = MOUSE_MULTI_CLICK;
	    else if (multi_click_pending)
		action = MOUSE_MULTI_CLICK_DONE;


	    last_click_time = newevent.xbutton.time;
	    last_x_root = newevent.xbutton.x_root;
	    last_y_root = newevent.xbutton.y_root;
	    last_root = newevent.xbutton.root;

	    XUngrabPointer(XtDisplayOfObject(w),
			   CurrentTime);
	    pointer_grabbed = False;
	    multi_click_pending = False;
	}
    } else if ((event->xbutton.time - last_click_time <=
		multi_click_timeout) &&
	       (last_root == event->xbutton.root) &&
	       ((ABS_DELTA(event->xbutton.x_root,
			   last_x_root) <=
		 mouse_damping_factor) ||
		(ABS_DELTA(event->xbutton.y_root,
			   last_y_root) <=
		 mouse_damping_factor)) &&
	       !(multi_click_pending)) {
	multi_click_pending = True;
	action = MOUSE_MULTI_CLICK_PENDING;
    }
    return (action);

#undef ABS_DELTA
}				/* end of _OlPeekAheadForEvents */

extern int
_OlFontAscent(Widget w)
{
	/* "w" SHOULD be a subclass of Primtive */
	PrimitiveWidget pw = (PrimitiveWidget)w;

        if (pw->primitive.text_format == OL_SB_STR_REP)
                return ((XFontStruct *)pw->primitive.font)->ascent;
        else
                return (-(XExtentsOfFontSet(
			(XFontSet)pw->primitive.font))->max_logical_extent.y);
}

extern int
_OlFontDescent(Widget w)
{
	/* "w" SHOULD be a subclass of Primtive */ 
        PrimitiveWidget pw = (PrimitiveWidget)w;

        if (pw->primitive.text_format == OL_SB_STR_REP)
                return ((XFontStruct *)pw->primitive.font)->descent;
        else {
                XRectangle r = (XExtentsOfFontSet(
				(XFontSet)pw->primitive.font))->max_logical_extent;
                return (r.height + r.y);
	}
}

extern int
_OlFontWidth(Widget w)
{
	/* "w" SHOULD be a subclass of Primtive */
        PrimitiveWidget pw = (PrimitiveWidget)w;

	 if (pw->primitive.text_format == OL_SB_STR_REP)
                return ((XFontStruct *)pw->primitive.font)->max_bounds.width;
        else
                return (XExtentsOfFontSet(
			(XFontSet)pw->primitive.font)->max_logical_extent.width);
}
