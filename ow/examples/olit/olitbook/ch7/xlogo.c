/*****************************************************************************
 * xlogo.c: Display the X logo
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
#include <Xol/DrawArea.h>
#include <X11/Xutil.h>
#include "xlogo64"

static void   redisplay_callback(Widget, XtPointer, XtPointer);
static void   resize_callback(Widget, XtPointer, XtPointer);
static Pixmap create_logo(Widget, GC, unsigned char *, Dimension, Dimension);

typedef struct {
  Pixmap     pix;
  GC         gc;
  Dimension  width, height;
} pixmap_data;

void
main(unsigned int argc, char **argv)
{
  Widget       toplevel, canvas;
  XGCValues    values;
  pixmap_data  data;
  XtAppContext app;

  OlToolkitInitialize((XtPointer)NULL);
  toplevel = XtAppInitialize(&app, "XLogo", NULL, 0, &argc, 
			     argv, NULL, NULL, 0);
  /*
   * Create a widget in which to display the logo.
   */
  canvas = XtVaCreateManagedWidget("canvas",
                                   drawAreaWidgetClass,
                                   toplevel,
				   XtNuserData, &data,
				   NULL);
  XtAddCallback(canvas, XtNexposeCallback, redisplay_callback, NULL);
  XtAddCallback(canvas, XtNresizeCallback, resize_callback, NULL);
 /*
  * Use the foreground and background colors
  * of the canvas to create a graphics context.
  */
  XtVaGetValues(canvas,
		XtNforeground, &values.foreground,
		XtNbackground, &values.background,
		NULL);
  data.gc = XtGetGC(canvas, GCForeground | GCBackground, &values);
  /*
   * Create the pixmap conatinign the X logo. Store the
   * pixmap, as well as the size of the pixmap in the struct.
   */
  data.width = xlogo64_width;
  data.height = xlogo64_height;
  data.pix = create_logo(canvas, data.gc, xlogo64_bits,
                         xlogo64_width, xlogo64_height );

  XtRealizeWidget(toplevel);
  XtAppMainLoop(app);
}

Pixmap
create_logo(
  Widget         widget,
  GC             gc,
  unsigned char *bits,
  Dimension      width,
  Dimension      height)
{
  Pixmap bitmap, pix;

  /*
   * Create a bitmap from the data.
   */
  bitmap=XCreateBitmapFromData(XtDisplay(widget),
                               RootWindowOfScreen(XtScreen(widget)),
                               bits, width, height);
  /*
   * Create a pixmap of the same depth as the default screen.
   */
  pix = XCreatePixmap(XtDisplay(widget),
                      RootWindowOfScreen(XtScreen(widget)),
                      width, height,
                      DefaultDepthOfScreen(XtScreen(widget)));
  /*
   * Copy the contents of plane 1 of the bitmap to the
   * pixmap, using the widget's colors.
   */
  XCopyPlane(XtDisplay(widget), bitmap, pix, gc, 0, 0,
             xlogo64_width, xlogo64_height, 0, 0, 1);
  /*
   * We don't need the bitmap anymore, so free it.
   */
  XFreePixmap(XtDisplay(widget), bitmap);
  return(pix);
}

void
redisplay_callback(
  Widget    widget,
  XtPointer client_data,
  XtPointer call_data)
{
  pixmap_data *data;
  Dimension    widget_width, widget_height;

  /*
   * Get the current size of the widget window.
   */
  XtVaGetValues(widget,
		XtNwidth, &widget_width,
		XtNheight, &widget_height,
		XtNuserData, &data,
		NULL);
  /*
   * Copy the contents of the pixmap to the
   * center of the window.
   */
  XCopyArea(XtDisplay(widget), data->pix, XtWindow(widget), data->gc,
            0, 0, data->width, data->height,
            (int)(widget_width  - data->width) / 2,
            (int)(widget_height - data->height) / 2);
}

void
resize_callback(
  Widget    widget,
  XtPointer client_data,
  XtPointer call_data)
{
  if(XtIsRealized(widget))
    XClearArea(XtDisplay(widget), XtWindow(widget), 0, 0, 0, 0, TRUE);
}
