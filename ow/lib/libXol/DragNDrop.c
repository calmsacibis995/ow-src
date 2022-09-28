#pragma ident	"@(#)DragNDrop.c	302.4	92/09/11 lib/libXol SMI"	/* olmisc:DragNDrop.c 1.5	*/

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

#include <Xol/OlDnDVCX.h>	/* drag and drop */
#include <Xol/OpenLookP.h>
#include <Xol/RootShellP.h>

extern Boolean	OlDragAndDrop (
			Widget, Window *, Position *, Position *);

extern void	OlGrabDragPointer (
			Widget, Cursor, Window);

static Boolean	HandleDragKey (
			Widget, XEvent *);

extern void	OlUngrabDragPointer (
			Widget);

/*
 * OlDragAndDrop
 *
 * The \fIOlDragAndDrop\fR function is used to monitor a direct
 * manipulation operation; returning, when the operation is completed,
 * the \fIdrop_window\fR and the \fIx\fR and \fIy\fR coordinates
 * corresponding to the location of the drop.  These return values
 * will reflect the highest (in the stacking order) window located
 * under the pointer at the time of the button release.
 *
 * OlDragAndDrop returns True if the drop terminated normally, False
 * if it was aborted (i.e., via the cancel key)
 *
 * Example:
 *
 * The following code provides a sample of the use of the
 * facilities: LookupOlInputEvent, OlDetermineMouseAction,
 * OlGrabDragPointer, OlDragAndDrop, OlUngrabDragPointer, and
 * OlReplayBtnEvent
 *
 * .so CWstart
 * static void Button();
 * static char translations[]   = \*(DQ<BtnDown> button()\n\*(DQ;
 * static XtActionRec actions[] = \*(LC \*(DQbutton()\*(DQ, Button \*(RC;
 * static void Button(widget, event, params, num_params)
 * Widget     widget;
 * XEvent *   event;
 * String *   params;
 * Cardinal * num_params;
 * \*(LC
 * Window   drop_window;
 * Position x;
 * Position y;
 *
 * switch (LookupOLInputEvent(widget, event, NULL, NULL))
 *    \*(LC
 *    case OL_SELECT:~
 *       switch(OlDetermineMouseAction(widget, event))
 *          \*(LC
 *          case MOUSE_MOVE:~
 *             OlGrabDragPointer(widget, OlGetMoveCursor(widget),
 *                               None);
 *             if (OlDragAndDrop(widget, &drop_window, &x, &y))
 *              DropOn(widget, drop_window, x, y, ....);
 *             OlUngrabDragPointer(widget);
 *             break;
 *          case MOUSE_CLICK:~
 *             ClickSelect(widget, ....);
 *             break;
 *          case MOUSE_MULTI_CLICK:~
 *             MultiClickSelect(widget,  ....);
 *             break;
 *          default:~
 *             Panic(widget, ....);
 *             break;
 *          \*(RC
 *       break;
 *    default:~
 *       OlReplayBtnEvent(widget, NULL, event);
 *       break;
 *    \*(RC
 * \*(RC
 * .so CWend
 *
 * See also:
 *
 * OlDetermineMouseAction(3), OlGrabDragPointer(3), OlUngrabDragPointer(3)
 *
 * Synopsis:
 *
 *#include <OpenLook.h>
 *#include <Dynamic.h>
 * ...
 */
extern Boolean
OlDragAndDrop (Widget w, Window *window, Position *xPosition, Position *yPosition)
{
	return OlDnDDragAndDrop(w, window, xPosition, yPosition, 
				(OlDnDDragDropInfoPtr)NULL,
				(OlDnDPreviewAnimateCbP)NULL,
				(XtPointer)NULL);
} /* for backwards compatibility */

/* new function returns root info for calling trigger message with ... */

typedef struct	{
	Boolean			done;
	Boolean			retval;
	XEvent			*grabevent;
	OlDnDPreviewAnimateCbP	proc;
	XtPointer		closure;
} _DND_PD;

static void
_poll_for_drag_event (Widget w, XtPointer closure, XEvent *ev, Boolean *decide_to_dispatch)
{
	DisplayShellWidget dsw = 
		(DisplayShellWidget)_OlGetDisplayShellOfWidget(w);
	_DND_PD	*pd = (_DND_PD *)closure;

	if (pd->done) return;

	switch (ev->type) {
		case ButtonRelease:
			pd->done = (Boolean)True;
			*decide_to_dispatch = False;
			*pd->grabevent = *ev;
			break;

		case KeyPress:
			pd->retval = HandleDragKey(w, ev);
			if (!dsw->display.doing_drag) {
				pd->done = True;
			}
			*pd->grabevent = *ev;
			*decide_to_dispatch = False;
			break;

		case MotionNotify:
			OlDnDPreviewAndAnimate(w,
					   ev->xmotion.root,
					   ev->xmotion.x_root,
					   ev->xmotion.y_root,
					   ev->xmotion.time,
					   pd->proc, pd->closure);
			*decide_to_dispatch = False;
			break;
		
	}
}

extern Boolean
OlDnDDragAndDrop (Widget w, Window *window, Position *xPosition, Position *yPosition, OlDnDDragDropInfoPtr rootinfo, OlDnDPreviewAnimateCbP animate, XtPointer closure)
{
	XEvent		grabevent;
	Window		parent;
	Window		child;
	Window		temp;
	int		x;
	int		y;
	Time		timestamp;
	_DND_PD		pd;
	DisplayShellWidget dsw;

	GetToken();
	pd.done		= False;
	pd.retval	= True;
	pd.grabevent	= &grabevent;
	pd.proc		= animate;
	pd.closure	= closure;

	dsw = (DisplayShellWidget)_OlGetDisplayShellOfWidget(w);
	XGrabKeyboard(	XtDisplayOfObject(w), XtWindowOfObject(w),
			False, GrabModeAsync, GrabModeAsync, CurrentTime);

#define EM      (ButtonReleaseMask | KeyPressMask | PointerMotionMask)

	XtInsertRawEventHandler(w, EM, False, _poll_for_drag_event,
				(XtPointer)&pd, XtListHead);

	OlDnDInitializeDragState(w);

	while (!pd.done) {
		XWindowEvent(   XtDisplayOfObject(w), XtWindowOfObject(w),
                                EM, &grabevent);
		XtDispatchEvent(&grabevent);
	}
	XtRemoveRawEventHandler(w, EM, False, _poll_for_drag_event,
				(XtPointer)&pd);

	OlDnDClearDragState(w);

	switch (grabevent.type) {
		case ButtonRelease:
			parent = child = grabevent.xbutton.root;
			x = grabevent.xbutton.x_root;
			y = grabevent.xbutton.y_root;
			timestamp = grabevent.xbutton.time;
			break;

		case KeyPress:
			if (!dsw->display.doing_drag) {
				parent = child = grabevent.xkey.root;
				x = grabevent.xkey.x_root;
				y = grabevent.xkey.y_root;
				timestamp = grabevent.xkey.time;
			}
	}

	if (rootinfo != (OlDnDDragDropInfoPtr)NULL) {
		rootinfo->root_x = x;
		rootinfo->root_y = y;
		rootinfo->root_window = parent;
		rootinfo->drop_timestamp = timestamp;
	}

	XUngrabKeyboard(XtDisplayOfObject(w), CurrentTime);

	while (child != 0)
	{
		temp = parent;
		parent = child;
		XTranslateCoordinates(	XtDisplayOfObject(w),
					temp, parent, x, y, &x, &y, &child);
	}

	*window = parent;
	*xPosition = (Position)x;
	*yPosition = (Position)y;

	ReleaseToken();
	return (pd.retval);
} /* END OF OlDragAndDrop */

/*
**  If I'm in the process of a drag-and-drop operation, this routine
**  provides special handling for keys.  Return True for normal
**  operations, False if the key was a cancel key.
*/
static Boolean
HandleDragKey (Widget w, XEvent *pevent)
{
	OlVirtualEventRec	nve;
	Position		deltax=0, deltay=0;
/* for query pointer */
	Window			root;
	Window			child;
	int			rootx, rooty, winx, winy;
	unsigned int		mask;
	Boolean			retval = True;
	DisplayShellWidget dsw = 
		(DisplayShellWidget)_OlGetDisplayShellOfWidget(w);

	XQueryPointer(	XtDisplay(w), XtWindow(w),
			&root, &child, &rootx, &rooty, &winx, &winy, &mask);

	if (!dsw->display.doing_drag)
		return(retval);

	/*
	 * Remove any Button masks, since (for Sun) the Drag operation
	 * implies some (SELECT) button is pressed
	 */
#ifdef sun
	((XKeyEvent *)pevent)->state &= ~(Button1Mask | Button2Mask |
			Button3Mask | Button4Mask | Button5Mask);
#endif
	OlLookupInputEvent(w, pevent, &nve, OL_CORE_IE);

	switch (nve.virtual_name) {
#ifndef sun	/* Sun's model has no mouseless DnD */
	case OL_MOVERIGHT:
		deltax = (Position)1;
		break;
	case OL_MOVELEFT:
		deltax = (Position)-1;
		break;
	case OL_MOVEUP:
		deltay = (Position)-1;
		break;
	case OL_MOVEDOWN:
		deltay = (Position)1;
		break;
	case OL_MULTIRIGHT:
		deltax = _OlGetMultiObjectCount(w) * 2;
		break;
	case OL_MULTILEFT:
		deltax = -_OlGetMultiObjectCount(w) * 2;
		break;
	case OL_MULTIUP:
		deltay = -_OlGetMultiObjectCount(w) * 2;
		break;
	case OL_MULTIDOWN:
		deltay = _OlGetMultiObjectCount(w) * 2;
		break;
	case OL_DROP:
#endif
	case OL_DEFAULTACTION:
		OlUngrabDragPointer(w);
		break;
	case OL_CANCEL:
	case OL_STOP:
		retval = False;
		OlUngrabDragPointer(w);
		break;
#ifndef sun	/* Sun's model has no mouseless DnD */
	case OL_DRAG:
		_OlBeepDisplay(w, 1);
		break;
#endif
	default:
		return (retval);
	}

	XWarpPointer(	XtDisplay(w),
			None, RootWindowOfScreen(XtScreen(w)),
			0, 0, 0, 0,
			rootx+deltax, rooty+deltay);

	return (retval);
} /* END OF HandleDragKey */

/*
 * OlGrabDragPointer
 *
 * The \fIOlGrabDragPointer\fR procedure is used to effect an active
 * grab of the mouse pointer.  This function is normally called
 * after a mouse drag operation is experienced and prior to calling
 * the OlDragAndDrop procedure which is used to monitor a drag operation.
 *
 * See also:
 *
 * OlDetermineMouseAction(3), OlDragAndDrop(3), OlUngrabDragPointer(3)
 *
 * Synopsis:
 *
 *#include <OpenLook.h>
 *#include <Dynamic.h>
 * ...
 */
extern void
OlGrabDragPointer (Widget w, Cursor cursor, Window window)
{
	DisplayShellWidget dsw;

	GetToken();
	dsw = (DisplayShellWidget)_OlGetDisplayShellOfWidget(w);
	dsw->display.doing_drag = TRUE;
	ReleaseToken();

	while (XGrabPointer(XtDisplayOfObject(w), XtWindowOfObject(w), False,
		(ButtonReleaseMask | PointerMotionMask), GrabModeAsync,
		GrabModeAsync, window, cursor, CurrentTime) != GrabSuccess)
			;
} /* END OF OlGrabDragPointer */

/*
 * OlUngrabDragPointer
 *
 * The \fIOlUngrabDragPointer\fR procedure is used to relinquish the
 * active pointer grab which was initiated by the OlGrabDragPointer
 * procedure.  This function simply ungrabs the pointer.
 *
 * See also:
 *
 * OlDetermineMouseAction(3), OlGrabDragPointer(3), OlDragAndDrop(3)
 *
 * Synopsis:
 *
 *#include <OpenLook.h>
 *#include <Dynamic.h>
 * ...
 */
extern void
OlUngrabDragPointer (Widget w)
{
	DisplayShellWidget dsw;

	XUngrabPointer(XtDisplayOfObject(w), CurrentTime);

	GetToken();
	dsw = (DisplayShellWidget)_OlGetDisplayShellOfWidget(w);
	dsw->display.doing_drag = FALSE;
	ReleaseToken();
} /* END OF OlUngrabDragPointer */

/*
 * OlChangeGrabbedDragCursor
 *
 * The \fIOlChangeGrabbedDragCursor\fR procedure is used to change the
 * cursor of the active pointer grab which was initiated by the 
 * OlGrabDragPointer procedure.
 *
 */

extern void 
OlChangeGrabbedDragCursor (Widget w, Cursor cursor)
{
	XChangeActivePointerGrab(XtDisplay(w),
				 (ButtonReleaseMask | PointerMotionMask),
				 cursor, CurrentTime);
}
