/*****************************************************************************
 * rubberband.c: rubberband line example
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
#include <X11/cursorfont.h>
#include <Xol/OpenLook.h>
#include <Xol/DrawArea.h>

typedef struct {
    int start_x, start_y, last_x, last_y;
    GC  gc;
} rubber_band_data;

static void start_rubber_band(Widget, rubber_band_data *, XEvent *);
static void end_rubber_band(Widget, rubber_band_data *, XEvent *);
static void track_rubber_band(Widget, rubber_band_data *, XEvent *);
static GC create_xor_gc(Widget);

void
main(unsigned int argc, char **argv)
{
  Widget           toplevel, canvas;
  rubber_band_data data;
  XtAppContext     app;
   
  OlToolkitInitialize((XtPointer)NULL);
  toplevel = XtAppInitialize(&app, "Rubberband", NULL, 0, &argc, 
			     argv, NULL, NULL, 0);
  /*
   * Create a drawing surface, and add event handlers for
   * ButtonPress, ButtonRelease and MotionNotify events.
   */
  canvas = XtCreateManagedWidget("canvas", 
                                 drawAreaWidgetClass, 
                                 toplevel, NULL, 0);
  XtAddEventHandler(canvas, ButtonPressMask, FALSE,
                    start_rubber_band, &data);
  XtAddEventHandler(canvas, ButtonMotionMask, FALSE,
                    track_rubber_band, &data);
  XtAddEventHandler(canvas, ButtonReleaseMask,
                    FALSE, end_rubber_band, &data);
  XtRealizeWidget(toplevel);
  /*
   * Establish a passive grab, for any button press.
   * Force the pointer to stay within the canvas window, and
   * change the pointer to a cross_hair.
   */
   XGrabButton(XtDisplay(canvas), AnyButton, AnyModifier, 
               XtWindow(canvas), TRUE, 
               ButtonPressMask | ButtonMotionMask | 
               ButtonReleaseMask,
               GrabModeAsync, GrabModeAsync,
               XtWindow(canvas), 
               XCreateFontCursor(XtDisplay(canvas),
                                 XC_crosshair));
  /*
   * Create the GC used by the rubber banding functions.
   */
  data.gc = create_xor_gc(canvas); 
  XtAppMainLoop(app);
}

void
start_rubber_band(
  Widget           w,
  rubber_band_data *data,
  XEvent           *event)
{
  data->last_x  =  data->start_x = event->xbutton.x;
  data->last_y  =  data->start_y = event->xbutton.y;
  XDrawLine(XtDisplay(w), XtWindow(w), 
            data->gc, data->start_x, 
            data->start_y, data->last_x, data->last_y);
}

void
track_rubber_band(
  Widget           w,
  rubber_band_data *data,
  XEvent           *event)
{
  /*
   * Draw once to clear the previous line.
   */
  XDrawLine(XtDisplay(w), XtWindow(w), data->gc, 
            data->start_x,data->start_y, 
            data->last_x, data->last_y);
  /*
   * Update the endpoints.
   */
  data->last_x  =  event->xbutton.x;
  data->last_y  =  event->xbutton.y;
  /*
   * Draw the new line.
   */
  XDrawLine(XtDisplay(w), XtWindow(w), data->gc, 
            data->start_x, data->start_y, 
            data->last_x, data->last_y);
}

void
end_rubber_band(
  Widget           w,
  rubber_band_data *data,
  XEvent           *event)
{
 /*
  * Clear the current line and update the endpoint info.
  */
  XDrawLine(XtDisplay(w), XtWindow(w), data->gc, 
            data->start_x, data->start_y, 
            data->last_x, data->last_y);
  data->last_x  =  event->xbutton.x;
  data->last_y  =  event->xbutton.y;
}

GC
create_xor_gc(Widget w)
{
  XGCValues values;
  GC        gc;

  /*
   * Get the background and foreground colors.
   */
  XtVaGetValues(w,
		XtNforeground, &values.foreground,
		XtNbackground, &values.background,
		NULL);
  /*
   * Set the fg to the XOR of the fg and bg, so if it is
   * XOR'ed with bg, the result will be fg and vice-versa.
   * This effectively achieves inverse video for the line.
   */
  values.foreground = values.foreground ^ values.background;
  /*
   * Set the rubber band gc to use XOR mode and draw 
   * a dashed line.
   */
  values.line_style = LineOnOffDash;
  values.function   = GXxor;
  gc = XtGetGC(w, GCForeground | GCBackground | 
               GCFunction | GCLineStyle, &values);
  return gc;
}
