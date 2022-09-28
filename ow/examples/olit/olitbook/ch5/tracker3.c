/*****************************************************************************
 * tracker3.c:
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
#include <Xol/StaticText.h>
#include <stdio.h>

typedef struct {
  Widget        tracker;
  Widget        target;
  XtIntervalId  id;
  int           delay;
} track_data, *track_data_ptr; 

#define DELAY 1000
extern XtAppContext app;
void enter_window_handler(Widget, XtPointer, XEvent *, Boolean *);
void disable_alarm(Widget, XtPointer, XEvent *, Boolean *);
void leave_window_handler(Widget, XtPointer, XEvent *, Boolean *);
void track_mouse_position(Widget, XtPointer, XEvent *, Boolean *);
void start_tracking(XtPointer, XtIntervalId *);

void
create_mouse_tracker(
  Widget parent,
  Widget target)
{
  static track_data data;

  data.delay = DELAY;
  /*
   * Store the target and tracker widgets in the data.
   */
  data.target  = target;
  data.tracker = XtCreateManagedWidget("mousetracker", 
                                        staticTextWidgetClass,
                                        parent, NULL, 0);
  /*
   * Start with a single event handler.
   */  
  XtAddEventHandler(data.target, EnterWindowMask, FALSE,
                    enter_window_handler, &data);
}

void
enter_window_handler(
  Widget    widget,
  XtPointer client_data,
  XEvent   *event,
  Boolean  *continue_to_dispatch)
{
  track_data *data = (track_data *)client_data;

  /*
   * When the pointer enters the window, install
   * a timeout callback, and start the count-down.
   */
  XtAddEventHandler(data->target, LeaveWindowMask, FALSE,
                    disable_alarm, data);
  data->id = XtAppAddTimeOut(app, data->delay, start_tracking, data);
}

void
disable_alarm(
  Widget    widget,
  XtPointer client_data,
  XEvent   *event,
  Boolean  *continue_to_dispatch)
{
  track_data *data = (track_data *)client_data;

  /*
   * Remove the timeout callback and then remove
   * ourself as an event handler.
   */
  XtRemoveTimeOut(data->id);
  XtRemoveEventHandler(data->target, LeaveWindowMask, FALSE,
                       disable_alarm, data);
}

void
start_tracking(
  XtPointer     client_data,
  XtIntervalId *id)
{
  track_data *data = (track_data *)client_data;

  /*
   * If this function was called, the alarm must have
   * gone off, so remove the disable_alarm event handler.
   */
  XtRemoveEventHandler(data->target, LeaveWindowMask, 
                       FALSE, disable_alarm, data);
  /*
   * Now add event handlers to track the pointer motion
   * and clear the tracker when we leave the target window.
   */
  XtAddEventHandler(data->target, PointerMotionMask, 
                    FALSE, track_mouse_position, data);
  XtAddEventHandler(data->target, LeaveWindowMask, 
                    FALSE, leave_window_handler, data);
}

void
track_mouse_position(
  Widget    widget,
  XtPointer client_data,
  XEvent   *event,
  Boolean  *continue_to_dispatch)
{
  track_data *data = (track_data *)client_data;
  char        str[32];

  /*
   * Extract the position of the pointer from the event
   * and display it in the tracker widget. 
   */
  sprintf(str, "X: %4d, Y: %4d",
          event->xmotion.x, event->xmotion.y);
  XtVaSetValues(data->tracker, XtNstring, str, NULL);
}

void
leave_window_handler(
  Widget    widget,
  XtPointer client_data,
  XEvent   *event,
  Boolean  *continue_to_dispatch)
{
  track_data *data = (track_data *)client_data;

  /*
   * Clear the tracker widget display.
   */
  XtVaSetValues(data->tracker, XtNstring, "", NULL);
  /*
   * Remove the dynamically installed event handlers.
   */
  XtRemoveEventHandler(data->target, PointerMotionMask, FALSE,
                       track_mouse_position, data);
  XtRemoveEventHandler(data->target, LeaveWindowMask, FALSE,
                       leave_window_handler, data);
}
