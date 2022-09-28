/*****************************************************************************
 * slider.c: Demonstrate the Slider widget.
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
#include <Xol/Slider.h>

static void slider_callback(Widget, XtPointer, XtPointer);

void
main(unsigned int argc, char **argv)
{
  XtAppContext app;
  Widget       toplevel, con, slider1, slider2;

  OlToolkitInitialize((XtPointer)NULL);
  OlSetDefaultTextFormat(OL_SB_STR_REP);
  toplevel = XtAppInitialize(&app, "Slider", NULL, 0, &argc, 
			     argv, NULL, NULL, 0);
  con = XtVaCreateManagedWidget("control", controlAreaWidgetClass,
                                toplevel, NULL);
  /*
   * Create two slider widgets and their callbacks.
   */
  slider1 = XtVaCreateManagedWidget("slider1", sliderWidgetClass,
                                    con, NULL);
  slider2 = XtVaCreateManagedWidget("slider2", sliderWidgetClass,
                                    con, NULL);
  XtAddCallback(slider1, XtNsliderMoved, slider_callback, slider2);
  XtAddCallback(slider2, XtNsliderMoved, slider_callback, slider1);

  XtRealizeWidget(toplevel);
  XtAppMainLoop(app);
}

void
slider_callback(
  Widget    widget, 
  XtPointer client_data, 
  XtPointer call_data)
{
  OlSliderVerify *sv = (OlSliderVerify *)call_data;
  Widget          otherslider = (Widget)client_data;
  int             s1val, s2val, tmp;

  if(strcmp(XtName(widget), "slider1") == NULL) {
    tmp = s1val = sv->new_location;
    XtVaGetValues(otherslider, XtNsliderValue, &s2val, NULL);
  } else {
    tmp = s2val = sv->new_location;
    XtVaGetValues(otherslider, XtNsliderValue, &s1val, NULL);
  }
  if(s1val > s2val)
    XtVaSetValues(otherslider, XtNsliderValue, tmp, NULL);
}
