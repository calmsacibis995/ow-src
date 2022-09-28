/*****************************************************************************
 * browser.c : Display some tiling patterns
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
#include <X11/Xutil.h>
#include <Xol/OpenLook.h>
#include <Xol/Exclusives.h>
#include <Xol/RectButton.h>
#include "bitmaps.h"

typedef struct _bitmap_struct {
    unsigned char *bitmap;
    Dimension width;
    Dimension height;
} bitmap_struct;

bitmap_struct bitmaps[] = {
  solid_bits,       solid_width,       solid_height,
  clear_bits,       clear_width,       clear_height,
  vertical_bits,    vertical_width,    vertical_height,
  horizontal_bits,  horizontal_width,  horizontal_height,
  slant_right_bits, slant_right_width, slant_right_height,
  slant_left_bits,  slant_left_width,  slant_left_height,
  fg50_bits,        fg50_width,        fg50_height,
  fg25_bits,        fg25_width,        fg25_height,
  cross_bits,       cross_width,       cross_height,
};

static Widget create_pixmap_button();
static Widget create_pixmap_browser(Widget, bitmap_struct *,
		    int, void (*)(), XtPointer, WidgetList *);
static Widget create_pixmap_button(Widget, XImage *);
static XImage *create_image(Widget, unsigned char *, Dimension, Dimension);

void
main(unsigned int argc, char **argv)
{
  XtAppContext app;
  Widget       toplevel, browser;
   
  OlToolkitInitialize((XtPointer)NULL);
  toplevel = XtAppInitialize(&app, "Browser", NULL, 0, &argc, 
			     argv, NULL, NULL, 0);
  /*
   * Create the browser.
   */
  browser = create_pixmap_browser(toplevel,bitmaps, 
                                  XtNumber(bitmaps), 
                                  NULL, NULL, NULL);
  XtManageChild(browser);
  XtRealizeWidget(toplevel);
  XtAppMainLoop(app);
}

Widget 
create_pixmap_browser(
  Widget         parent,         /* widget to manage the browser */
  bitmap_struct *bitmaps,        /* list of bitmaps              */
  int            n_bmaps,        /* how many bitmaps             */
  void           (*callback)(),  /* invoked when state changes   */
  XtPointer      data,           /* data to be passed to callback*/
  WidgetList    *widgets)
{
  Widget       browser;
  WidgetList   buttons;
  XImage     **images;
  int          i;

  /*
   * Malloc room for button widgets.
   */
  buttons = (WidgetList) XtMalloc(n_bmaps * sizeof(Widget));
  images = (XImage **) XtMalloc(n_bmaps * sizeof(XImage *));
  /*
   * Create an Exclusives widget.
   */
  browser = XtVaCreateManagedWidget("browser",
                                    exclusivesWidgetClass,
                                    parent,
				    XtNlayoutType, OL_FIXEDCOLS,
				    XtNmeasure,    3,
				    NULL);
  /*
   * Create a button for each tile. If a callback function
   * has been given, register it as an XtNselect
   */
  for(i=0;i< n_bmaps;i++) {
    images[i] = create_image(parent, bitmaps[i].bitmap,
                             bitmaps[i].width,
                             bitmaps[i].height);

    buttons[i] = create_pixmap_button(browser, images[i]);
    if(callback)
      XtAddCallback(buttons[i], XtNselect, callback, data);
  }
  /*
   * Manage all buttons and return the Exclusives widget
   */ 
  XtManageChildren(buttons, n_bmaps);
  if(widgets != NULL)
    *widgets = buttons;
  return(browser);
}

Widget
create_pixmap_button(
  Widget   parent,
  XImage  *image)
{
  Widget   button;

  /*
   * Display the XImage in the button and also store it
   * so it can be retrieved from the button later.
   */
  button = XtVaCreateWidget("", rectButtonWidgetClass,
                            parent,
			    XtNlabelType,  OL_IMAGE,
			    XtNlabelImage, image,
			    XtNuserData,   image,
			    NULL);
  /*
   * Return the unmanaged button.
   */
  return(button);
}

XImage *
create_image(
  Widget         widget,
  unsigned char *bits,
  Dimension      width,
  Dimension      height)
{
  XImage *image;
  image = XCreateImage(XtDisplay(widget),
                       OlVisualOfObject(widget), 
                       1, XYBitmap, 0, 
                       bits, width, height, 32, 0);
  return(image);
}
