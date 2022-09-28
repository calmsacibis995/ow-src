/*****************************************************************************
 * fractal2.c: A simple fractal generator, improved
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
  Widget       toplevel, canvas;
  image_data   data;
  XtAppContext app;

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
  int x, y, iteration;
  /*
   * We have to buffer all points of the same color, until
   * enough points are available to draw efficiently. Start
   * by zeroing all buffers.
   */
  init_buffer(data);
  /*
   * For each pixel on the window....
   */
  for (y = 0; y < (int)data->height; y++) {
    for (x = 0; x < (int)data->width; x++) {
      complex z, k;
      /*
       *  Initialize K to the normalized, floating coordinate in 
       *  the x,y plane. Init Z to (0.0, 0.0).
       */
      z.real = z.imag = 0.0;
      k.real =  data->origin.real + (float) x / 
                    (float) data->width * data->range;
      k.imag =  data->origin.imag - (float) y / 
                   (float) data->height * data->range;
      /*
       * Calculate z = (z + k) * (z + k) over and over.
       */
      for (iteration = 0; iteration < data->depth; iteration++){
        float distance, real_part, imag_part;
        real_part = z.real + k.real;
        imag_part = z.imag + k.imag;
        z.real    = real_part * real_part - imag_part * imag_part;
        z.imag    = 2 * real_part * imag_part;
        distance  = z.real * z.real + z.imag * z.imag;     
        /*
         * If the z point has moved off the plane, buffer the
         * point using the integerized distance (modulo the 
         * number of colors we have) as the color.
         */
        if (distance  >= data->max_distance) { 
          buffer_point(w, data, (int) distance % data->ncolors,
                       x, y);
          break;
        }
      }
    }
  }
  /*
   * Display all remaining points.
   */
  flush_buffer(w, data);
}

void
redisplay(
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
resize(
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

#define MAXPOINTS 500
#define MAXCOLOR  256

struct {
   XPoint  data[MAXCOLOR][MAXPOINTS];
   int     npoints[MAXCOLOR];
} points;

void
init_buffer(image_data *data)
{
  int i;
  if (data->ncolors > MAXCOLOR)
    XtError("This display has too many colors");
  for(i=0;i<MAXCOLOR;i++)
    points.npoints[i] = 0;
}

void
buffer_point(
  Widget      w,
  image_data *data,
  int         color,
  int         x,
  int         y)
{
  if(points.npoints[color] == MAXPOINTS - 1) {
    /*
     * If the buffer is full, set the foreground color
     * of the graphics context and draw the points in both
     * the window and the pixmap.
     */
    XSetForeground(XtDisplay(w), data->gc, color);
    if(XtIsRealized(w))
      XDrawPoints (XtDisplay(w), XtWindow(w), data->gc, 
                   points.data[color], points.npoints[color], 
                   CoordModeOrigin);
    XDrawPoints (XtDisplay(w), data->pix, data->gc, 
                 points.data[color], points.npoints[color], 
                 CoordModeOrigin);
    /*
     * Reset the buffer.
     */
    points.npoints[color] = 0;
  }
  /*
   * Store the point in the buffer according to its color.
   */
  points.data[color][points.npoints[color]].x = x;
  points.data[color][points.npoints[color]].y = y;
  points.npoints[color] += 1;
}

void
flush_buffer(
  Widget      w,
  image_data *data)
{ 
  int i;
  /*
   * Check each buffer.
   */
  for(i=0;i<data->ncolors;i++)
    /*
     * If there are any points in this buffer, display them
     * in the window and the pixmap.
     */
    if(points.npoints[i]){
      XSetForeground(XtDisplay(w), data->gc, i);
      if(XtIsRealized(w))
        XDrawPoints (XtDisplay(w), XtWindow(w), data->gc, 
                     points.data[i], points.npoints[i], 
                     CoordModeOrigin);
        XDrawPoints (XtDisplay(w), data->pix, data->gc, 
                     points.data[i], points.npoints[i], 
                     CoordModeOrigin);
      points.npoints[i] = 0;
    }
}
