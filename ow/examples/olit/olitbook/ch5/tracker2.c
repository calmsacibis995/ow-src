/*****************************************************************************
 * tracker2.c:
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

void  clear_tracker(Widget, XtPointer, XEvent *, Boolean *);
void  track_mouse_position(Widget, XtPointer, XEvent *, Boolean *);
void  show_mouse_position(Widget, XtPointer, XEvent *, Boolean *);

void
create_mouse_tracker(
  Widget parent,
  Widget target)
{
  Widget       tracker;

  /*
   * Create the tracker widget.
   */
  tracker = XtCreateManagedWidget("mousetracker", 
                                  staticTextWidgetClass,
                                  parent, NULL, 0);
  /*
   * Set up event handlers on target widget.
   */
  XtAddEventHandler(target, ButtonPressMask, FALSE,
                    show_mouse_position, tracker);
  XtAddEventHandler(target, 
                    Button1MotionMask | Button2MotionMask, 
                    FALSE, track_mouse_position, tracker);
  XtAddEventHandler(target, ButtonReleaseMask, FALSE, 
                    clear_tracker, tracker);
}

void
show_mouse_position(
  Widget    widget,
  XtPointer client_data,
  XEvent   *event,
  Boolean  *continue_to_dispatch)
{
  Widget tracker = (Widget)client_data;
  char   str[32];

  /*
   * Extract the position of the pointer from the event
   * and display it in the tracker widget. 
   */
  sprintf(str, "X: %4d, Y: %4d", 
             event->xbutton.x, event->xbutton.y);
  XtVaSetValues(tracker, XtNstring, str, NULL);
}

void
track_mouse_position(
  Widget    widget,
  XtPointer client_data,
  XEvent   *event,
  Boolean  *continue_to_dispatch)
{
  Widget tracker = (Widget)client_data;
  char   str[32];

  /*
   * Extract the position of the pointer from the event
   * and display it in the tracker widget. 
   */
  sprintf(str, "X: %4d, Y: %4d", 
          event->xmotion.x, event->xmotion.y);
  XtVaSetValues(tracker, XtNstring, str, NULL);
}

void
clear_tracker(
  Widget    widget,
  XtPointer client_data,
  XEvent   *event,
  Boolean  *continue_to_dispatch)
{
  Widget tracker = (Widget)client_data;

  /*  
   * Display an empty string in the tracker widget. 
   */
  XtVaSetValues(tracker, XtNstring, "", NULL);
}
