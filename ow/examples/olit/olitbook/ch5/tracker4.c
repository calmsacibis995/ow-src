/*****************************************************************************
 * tracker4.c:
 *
 *         From:
 *                   The X Window System, 
 *            Programming and Applications with Xt
 *                   OPEN LOOK Edition
 *         by
 *              Douglas Young & John Pew
 *              Prentice Hall, 1993
 *
 *              Example described on pages: 
 *
 *
 *  Copyright 1993 by Prentice Hall
 *  All Rights Reserved
 *
 * This code is based on the OPEN LOOK Intrinsics Toolkit (OLIT) and 
 * the X Window System
 *
 * Permission to use, copy, modify, and distribute this software for 
 * any purpose and without fee is hereby granted, provided that the above
 * copyright notice appear in all copies and that both the copyright notice
 * and this permission notice appear in supporting documentation.
 *
 * Prentice Hall and the authors disclaim all warranties with regard to 
 * this software, including all implied warranties of merchantability and 
 * fitness.
 * In no event shall Prentice Hall or the authors be liable for any special,
 * indirect or consequential damages or any damages whatsoever resulting from 
 * loss of use, data or profits, whether in an action of contract, negligence 
 * or other tortious action, arising out of or in connection with the use 
 * or performance of this software.
 *
 * OPEN LOOK is a trademark of UNIX System Laboratories.
 * X Window System is a trademark of the Massachusetts Institute of Technology
 ****************************************************************************/

#include <X11/StringDefs.h>
#include <X11/Intrinsic.h>
#include <Xol/OpenLook.h>
#include <Xol/StaticText.h>
#include <stdio.h>

void consume_callback(Widget, XtPointer, XtPointer);

void
create_mouse_tracker(
  Widget parent,
  Widget target)
{
  Widget tracker;

  /*
   * Create the tracker widget and register event 
   * handlers for the target widget. 
   */
  tracker = XtCreateManagedWidget("mousetracker", 
                                  staticTextWidgetClass,
                                  parent, NULL, 0);
  XtAddCallback(target, XtNconsumeEvent, consume_callback, tracker);
}

void
consume_callback(
  Widget    widget,
  XtPointer client_data,
  XtPointer call_data)
{
  OlVirtualEvent vevent = (OlVirtualEvent)call_data;
  XMotionEvent *event = (XMotionEvent *)vevent->xevent;
  Widget tracker = (Widget)client_data;
  char   str[32];

  if(vevent->virtual_name == OL_SELECT && event->type == MotionNotify) {
    sprintf(str, "X: %4d, Y: %4d", event->x, event->y);
    XtVaSetValues(tracker, XtNstring, str, NULL);
  } else
    XtVaSetValues(tracker, XtNstring, "", NULL);
}
