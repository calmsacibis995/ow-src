/*****************************************************************************
 * scrollbar.c: Demonstrate the Scrollbar widget.
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
#include <Xol/ControlAre.h>
#include <Xol/OblongButt.h>
#include <Xol/Scrollbar.h>
#include <stdio.h>

static void scrollbar_callback(Widget, XtPointer, XtPointer);
static void button_callback(Widget, XtPointer, XtPointer);

void
main(unsigned int argc, char **argv)
{
  XtAppContext app;
  Boolean      verbose = FALSE;
  Widget       toplevel, con, menu_pane, scrollbar, button;

  OlToolkitInitialize((XtPointer)NULL);
  OlSetDefaultTextFormat(OL_MB_STR_REP);
  toplevel = XtAppInitialize(&app, "Scrollbar", NULL, 0, &argc, 
			     argv, NULL, NULL, 0);
  con = XtVaCreateManagedWidget("control", controlAreaWidgetClass,
                                toplevel, NULL);
  /*
   * Create the scrollbar widget.
   */
  scrollbar = XtVaCreateManagedWidget("scrollbar", 
                                      scrollbarWidgetClass,
                                      con, NULL);
  XtAddCallback(scrollbar, XtNsliderMoved, scrollbar_callback, 
                &verbose);
  /*
   * Get the MenuPane widget
   */
  XtVaGetValues(scrollbar, XtNmenuPane, &menu_pane, NULL);

  /*
   * Add a button to the MenuPane
   */
  button = XtVaCreateManagedWidget("verify", oblongButtonWidgetClass,
                                   menu_pane, NULL);
  XtAddCallback(button, XtNselect, button_callback, &verbose);

  XtRealizeWidget(toplevel);
  XtAppMainLoop(app);
}

void
scrollbar_callback(
  Widget    widget, 
  XtPointer client_data, 
  XtPointer call_data)
{
  OlScrollbarVerify *sbv = (OlScrollbarVerify *)call_data;
  Boolean *verbose = (Boolean *)client_data;

  if(*verbose)
    printf("%s %3d, %s %d, %s %d, %s %d, %s %d, %s %4d,\n",
           "new_location", sbv->new_location,
           "new_page",     sbv->new_page,
           "ok",           sbv->ok,
           "slidermin",    sbv->slidermin,
           "slidermax",    sbv->slidermax,
           "delta",        sbv->delta);
}

void
button_callback(
  Widget    widget, 
  XtPointer client_data, 
  XtPointer call_data)
{
  Boolean *verbose = (Boolean *)client_data;

  if(*verbose)
    *verbose = FALSE;
  else
    *verbose = TRUE;
}
