/*****************************************************************************
 * caption.c: Demonstrate the Caption widget.
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
#include <Xol/Gauge.h>
#include <Xol/Caption.h>

static void slider_callback(Widget, XtPointer, XtPointer);

void
main(unsigned int argc, char **argv)
{
  XtAppContext app;
  Widget       toplevel, con, slider, gauge, cap_slider, cap_gauge;

  OlToolkitInitialize((XtPointer)NULL);
  OlSetDefaultTextFormat(OL_MB_STR_REP);
  toplevel = XtAppInitialize(&app, "Caption", NULL, 0, &argc, 
			     argv, NULL, NULL, 0);
  con = XtVaCreateManagedWidget("control", controlAreaWidgetClass,
                                toplevel, NULL);
  /*
   * Create the caption widgets
   */
  cap_slider = XtVaCreateManagedWidget("cap_slider", captionWidgetClass,
                                       con, NULL);
  cap_gauge = XtVaCreateManagedWidget("cap_gauge", captionWidgetClass,
                                       con, NULL);
  /*
   * Create a slider and a gauge widget
   */
  slider = XtVaCreateManagedWidget("slider", sliderWidgetClass,
                                   cap_slider, NULL);
  gauge = XtVaCreateManagedWidget("gauge", gaugeWidgetClass,
                                  cap_gauge, NULL);
  XtAddCallback(slider, XtNsliderMoved, slider_callback, gauge);

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
  int     celsius, fahrenheit;
  Widget  gauge = (Widget)client_data;

  celsius = sv->new_location;
  fahrenheit = celsius * 9 / 5 + 32;
  OlSetGaugeValue(gauge, fahrenheit);
}
