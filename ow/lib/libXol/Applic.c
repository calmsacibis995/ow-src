#pragma ident	"@(#)Applic.c	302.5	97/03/26 lib/libXol SMI"	/* olmisc:Applic.c 1.8	*/

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

/*
 *************************************************************************
 * Description:
 *	This file contains the routines and static information that is
 * Global to the application.
 ******************************file*header********************************
 */


#include <X11/IntrinsicP.h>
#include <X11/StringDefs.h>

#include <Xol/Notice.h>
#include <Xol/OpenLookI.h>
#include <Xol/RootShellP.h>


/*
 *************************************************************************
 *
 * Define global/static variables and #defines, and
 * Declare externally referenced variables
 *
 *****************************file*variables******************************
 */

static Widget	widget_with_grab = (Widget)NULL;


/*
 *************************************************************************
 *
 * Public Procedures
 *
 ****************************public*procedures****************************
 */

/*
 *************************************************************************
 * _OlBeepDisplay - this routine beeps the display.  It takes a widget
 * id as an argument.  The widget's class is checked with an internal
 * list of widget classes that are permitted to beep the display.
 * If the widget is on the list, the display is beeped.  If not, the
 * procedure returns immediately.
 ****************************procedure*header*****************************
 */
void
_OlBeepDisplay(Widget widget, Cardinal count)
{
	if (widget != (Widget)NULL && count != (Cardinal)0)
	{
		switch(OlGetBeep(widget))
		{
		case OL_BEEP_NOTICES:
			if (XtIsSubclass(widget, noticeShellWidgetClass)
						== False)
			{
				count = 0;
			}
			break;
		case OL_BEEP_NEVER:
			count = 0;
			break;
		default:			/* OL_BEEP_ALWAYS	*/
			break;
		}

		if(count != 0) {
			XKeyboardControl kb_values;
			int 	value = -1;

			kb_values.bell_duration = 
				((value = OlGetBeepDuration(widget)) < -1 ? 
						-1 :
						value);
			XChangeKeyboardControl(
					XtDisplayOfObject(widget),
					KBBellDuration,
					&kb_values);
		}

		for(; count != (Cardinal)0; --count)
		{
			XBell(XtDisplayOfObject(widget),
				OlGetBeepVolume(widget));
		}
	}
} /* END OF _OlBeepDisplay() */

/*
 *************************************************************************
 * _OlGrabPointer - this routine acts as a locking switch that
 * keeps track of a widget who has explicitly grabbed the pointer.
 * It returns a Boolean value to indicate whether or not a widget is
 * allowed to grab the pointer and an XtGrabPointer is done. 
 * If this routine returns True, the requesting widget will be saved.
 * If this routine returns False, than the requesting should not grab
 * the pointer, since some other widget already has a grab on it.
 ****************************procedure*header*****************************
 */
Boolean
_OlGrabPointer(Widget widget, int owner_events, unsigned int event_mask, int pointer_mode, int keyboard_mode, Window confine_to, Cursor cursor, Time time)
{
	Boolean		return_val = False;

				/* If NULL is passed in, remove any
				 * prior grabs; else, try to register a
				 * new grab				*/

	if (widget == (Widget)NULL)
	{
		_OlUngrabPointer(widget);
	}
	else if (OlOKToGrabPointer(widget) == True)
	{
		if (widget_with_grab == (Widget)NULL ||
		    widget_with_grab == widget)
		{
			if (XtIsRealized(widget) == True)
			{
#if Xt_works_right
				if (GrabSuccess == XtGrabPointer(
						(_OlIsGadget(widget) ?
						XtParent(widget) : widget),
#else
		/* Use Xlib directly since Xt is broken	*/

				if (GrabSuccess == XGrabPointer(
						XtDisplayOfObject(widget),
						XtWindowOfObject(widget),
#endif
						owner_events, event_mask,
						pointer_mode, keyboard_mode,
						confine_to, cursor, time))
				{
					return_val = True;
					widget_with_grab = widget;
				}
			}
			else
			{
				OlVaDisplayWarningMsg(XtDisplayOfObject(widget),
					"", "", "OlToolkitWarning",
					"_OlGrabPointer: widget \"%s\"\
 (class \"%s\") must be realized to grab pointer", XrmQuarkToString(widget->core.xrm_name), XtClass(widget)->core_class.class_name);
			}
		}
	}

	return(return_val);
} /* END OF _OlGrabPointer() */

/*
 *************************************************************************
 * _OlGrabServer - this grabs the server if the application attributes
 * permit it.  The function returns true if the grab is successful; false
 * is returned otherwise.
 ****************************procedure*header*****************************
 */
Boolean
_OlGrabServer(Widget widget)
{
	if (OlOKToGrabServer(widget)  == True && widget != (Widget)NULL)
	{
		XGrabServer(XtDisplayOfObject(widget));
		return ((Boolean)True);
	}
	return((Boolean)False);
} /* END OF _OlGrabServer() */


/*
 *************************************************************************
 * _OlUngrabPointer - this routine is called by widgets that want to
 * ungrab the mouse pointer, but are not sure if some other widget
 * really has the grab and is letting events pass through to the owner.
 * If some other widget really has the grab, the routine simply returns.
 * If the widget requesting the ungrab has the grab or if no other widget
 * has a grab, the routine calls XtUngrabPointer().
 *	If a NULL widget id is passed in and there has not been a 
 * previous _OlGrabPointer() call, the routine routines.  But, if
 * the passed-in widget is NULL and there was a previous _OlGrabPointer()
 * call, the routine ungrabs the pointer and resets the "widget_with_grab"
 * variable.
 ****************************procedure*header*****************************
 */
void
_OlUngrabPointer (Widget widget)
{
	extern Widget widget_with_grab;

	if (widget != (Widget) NULL) {
		if (widget_with_grab == (Widget) NULL ||
		    widget_with_grab == widget) {
			widget_with_grab = (Widget) NULL;
		}
		else {
			widget = NULL;
		}
	}
	else if (widget_with_grab != (Widget) NULL) {
		widget_with_grab = (Widget) NULL;
	}
	else {
		widget = NULL;
	}

	if (widget != (Widget)NULL) {
#if Xt_works_right
		XtUngrabPointer((_OlIsGadget(widget) ?
			XtParent(widget) : widget), CurrentTime);
#else
	/* Use XLib directly since Xt is broken
	 */
		XUngrabPointer(XtDisplayOfObject(widget), CurrentTime);
#endif
	}
} /* END OF _OlUngrabPointer() */

/*
 *************************************************************************
 * _OlUngrabServer - this ungrabs the server.
 ****************************procedure*header*****************************
 */
void
_OlUngrabServer (Widget widget)
{
	if (widget != (Widget)NULL)
	{
		XUngrabServer(XtDisplayOfObject(widget));
	}
} /* END OF _OlUngrabServer() */
