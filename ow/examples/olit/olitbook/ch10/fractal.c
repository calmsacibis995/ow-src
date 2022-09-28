/*****************************************************************************
 * fractal.c: A simple fractal generator
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

#include "fractal.h"

void
main(unsigned int argc, char **argv)
{
  XtAppContext app;
  Widget       toplevel, canvas;
  image_data   data;

  OlToolkitInitialize((XtPointer)NULL);
  toplevel = XtAppInitialize(&app, "Fractal", NULL, 0, &argc, 
			     argv, NULL, NULL, 0);
  XtGetApplicationResources(toplevel, &data, resources, 
                            XtNumber(resources), NULL, 0);
  /*
   * Create the widget to display the fractal and register
   * callbacks for resize and refresh.
   */
  canvas = XtCreateManagedWidget("canvas", 
                                 drawAreaWidgetClass, 
                                 toplevel, NULL, 0);
  XtAddCallback(canvas, XtNexposeCallback, redisplay, &data);
  XtAddCallback(canvas, XtNresizeCallback, resize, &data);
  init_data(canvas, &data);  
  XtRealizeWidget(toplevel);
  resize(canvas, (XtPointer)&data, NULL);
  XtAppMainLoop(app);
}

void
init_data(
  Widget      w,
  image_data *data)
{
  /*
   * Get the size of the drawing area.
   */
  XtVaGetValues(w,
		XtNwidth,  &data->width,
		XtNheight, &data->height,
		NULL);
  /*
   * Find out how many colors we have to work with, and  
   * create a default, writable, graphics context.
   */
  data->ncolors = OlVisualOfObject(w)->map_entries;
  data->gc = XCreateGC(XtDisplay(w),
                       DefaultRootWindow(XtDisplay(w)),
                       NULL, NULL); 
  /*
   *  Initialize the pixmap to NULL.
   */
  data->pix = NULL;
}

void
create_image (
  Widget      w,
  image_data *data)
{
  int  x, y, iteration;
  /*
   * For each pixel on the window....
   */
  for (y = 0; y < (int)data->height; y++) {
    for (x = 0; x < (int)data->width; x++) {
      complex z, k;
      /*
       * Initialize K to the normalized, floating coordinate
       * in the x, y plane. Init Z to (0.0, 0.0).
       */
      z.real =  z.imag = 0.0;
      k.real =  data->origin.real + (float) x / 
                   (float) data->width * data->range;
      k.imag =  data->origin.imag - (float) y / 
                   (float) data->height * data->range;
      /*
       * Calculate z = (z + k) * (z + k) over and over.
       */
      for (iteration = 0; iteration < data->depth; iteration++) {
        float   distance, real_part, imag_part;

        real_part = z.real + k.real;
        imag_part = z.imag + k.imag;
        z.real = real_part * real_part - imag_part * imag_part;
        z.imag = 2 * real_part * imag_part;
        distance  = z.real * z.real + z.imag * z.imag;     
        /*
         * If the z point has moved off the plane, set the 
         * current foreground color to the distance (coerced to 
         * an int and modulo the number of colors available),
         * and draw a point in both the window and the pixmap.
         */
        if (distance  >= data->max_distance) { 
          int color = (int) distance % data->ncolors;

          XSetForeground(XtDisplay(w), data->gc, color);
          XDrawPoint (XtDisplay(w), data->pix, data->gc, x, y);
          if(XtIsRealized(w))
            XDrawPoint (XtDisplay(w), XtWindow(w), data->gc,x,y);
          break;
        }
      }
    }
  }
}

void
redisplay (
  Widget    w,
  XtPointer client_data,
  XtPointer call_data)
{
  OlDrawAreaCallbackStruct *cb = (OlDrawAreaCallbackStruct *)call_data;
  image_data *data = (image_data *)client_data;
  /*
   * Extract the exposed area from the event and copy
   * from the saved pixmap to the window.
   */
  XCopyArea(XtDisplay(w), data->pix, XtWindow(w), data->gc, 
            cb->x, cb->y, cb->width, cb->height, cb->x, cb->y);
}

void
resize (
  Widget    w,
  XtPointer client_data,
  XtPointer call_data)
{
  image_data *data = (image_data *)client_data;
  /*
   *  Get the new window size.
   */
  XtVaGetValues(w,
		XtNwidth,  &data->width,
		XtNheight, &data->height,
		NULL);
  /*
   * Clear the window.
   */
  if(XtIsRealized(w))
    XClearArea(XtDisplay(w), XtWindow(w), 0, 0, 0, 0, TRUE);
  /*
   *  Free the old pixmap and create a new pixmap 
   *  the size of the window.
   */
  if(data->pix)
     XFreePixmap(XtDisplay(w), data->pix);
  data->pix= XCreatePixmap(XtDisplay(w),
                           DefaultRootWindow(XtDisplay(w)),
                           data->width, data->height, 
                           OlDepthOfObject(w));
  XSetForeground(XtDisplay(w), data->gc, OlBlackPixel(w));
  XFillRectangle(XtDisplay(w), data->pix, data->gc, 0, 0, 
                 data->width,  data->height);
  /*
   * Generate a new image.
    */
  create_image(w, data);
}
