/*****************************************************************************
 * getpixel.c: Test the rgb functions from chapter 6
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

#include <X11/Intrinsic.h> 
#include <X11/StringDefs.h>
#include <Xol/OpenLook.h>
#include <stdio.h>

static Pixel get_pixel(Widget, unsigned short, unsigned short, unsigned short);
static Pixel get_pixel_by_name(Widget, char *);
static void  load_rgb(Widget, unsigned short *,
		      unsigned short *, unsigned short *);

void
main(unsigned int argc, char **argv)
{
  XtAppContext   app;
  Widget         toplevel;
  unsigned short red, green, blue;

  /*
   * Initialize the Intrinsics.
   */
  OlToolkitInitialize((XtPointer)NULL);
  toplevel = XtAppInitialize(&app, "Getpixel", NULL, 0, &argc, 
			     argv, NULL, NULL, 0);
  /*
   * Create a Core widget.
   */
  XtVaCreateManagedWidget("widget", widgetClass, toplevel, 
			  XtNheight, 50,
			  XtNwidth,  50,
			  NULL);

  printf("%d\n", get_pixel(toplevel, 1000, 2000, 3000));
  printf("%d\n", get_pixel_by_name(toplevel, "Red"));
  load_rgb(toplevel, &red, &green, &blue);
  printf("%d %d %d\n", red, green, blue);
  XtRealizeWidget(toplevel);

  XtAppMainLoop(app);
}

Pixel
get_pixel(
  Widget  w,
  unsigned short red,
  unsigned short green,
  unsigned short blue)
{
  Display *dpy =  XtDisplay(w);
  Colormap cmap = OlColormapOfObject(w);
  XColor   color;

  /* 
   * Fill in the color structure.
   */
  color.red   = red;
  color.green = green;
  color.blue  = blue;
  /* 
   * Try to allocate the color.
   */
  if(XAllocColor(dpy, cmap, &color))
     return (color.pixel);
  else {
     printf("Warning: Couldn't allocate requested color\n");
     return(OlBlackPixel(w));
  }
}

void
load_rgb(
  Widget  w,
  unsigned short *red,
  unsigned short *green,
  unsigned short *blue)
{
  Display *dpy  = XtDisplay(w);
  Colormap cmap = OlColormapOfObject(w);
  XColor   color;
  unsigned long      cells[3];

  /*
   *  Try to allocate three consecutive color cells.
   */
  if(XAllocColorCells(dpy, cmap, True, NULL, 0, cells, 3)) {
    /* 
     *  If successful, store red in the first allocated cell,
     *  green in the second and blue in the third.
     */
    color.flags = DoRed | DoGreen | DoBlue;
    color.red = 65535;
    color.green = color.blue = 0;
    *red = color.pixel = cells[0];
    XStoreColor(dpy, cmap, &color);
    /*
     *  Store Green in the second cell.
     */
    color.green = 65535;
    color.red =  color.blue = 0;
    *green = color.pixel = cells[1];
    XStoreColor(dpy, cmap, &color);
    /* 
     * Store Blue in the second cell.
     */
    color.blue = 65535;
    color.red = color.green = 0;
    *blue = color.pixel = cells[2];
    XStoreColor(dpy, cmap, &color);
  } else {
    printf("Warning:Couldn't allocate color cells\n");
    *blue = *red = *green = OlBlackPixel(w);
  }
}

Pixel
get_pixel_by_name(
  Widget w,
  char  *colorname)
{
  Display *dpy  = XtDisplay(w);
  Colormap cmap = OlColormapOfObject(w);
  XColor   color, ignore;

  /* 
   * Allocate the named color.
   */
  if(XAllocNamedColor(dpy, cmap, colorname, &color, &ignore))
    return (color.pixel);
  else{
    printf("Warning: Couldn't allocate color %s\n", colorname);
    return (OlBlackPixel(w));
  }
}
