/*****************************************************************************
 * gauge.c: Demonstrate the Gauge widget.
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
#include <stdio.h>

static void slider_callback(Widget, XtPointer, XtPointer);
static Widget toplevel;

void
main(unsigned int argc, char **argv)
{
  XtAppContext app;
  Widget       con, slider, gauge;

  OlToolkitInitialize((XtPointer)NULL);
  OlSetDefaultTextFormat(OL_MB_STR_REP);
  toplevel = XtVaAppInitialize(&app, "Gauge", NULL, 0, &argc, 
		  	       argv, NULL, 
			       XtNfooterPresent, TRUE,
			       XtNleftFooterVisible, TRUE,
			       XtNleftFooterString, "C: 0   F: 32",
			       NULL);
  con = XtVaCreateManagedWidget("control", controlAreaWidgetClass,
                                toplevel, NULL);
  /*
   * Create a slider and a gauge widget
   */
  slider = XtVaCreateManagedWidget("slider", sliderWidgetClass,
                                   con, NULL);
  gauge = XtVaCreateManagedWidget("gauge", gaugeWidgetClass,
                                  con, NULL);
  XtAddCallback(slider, XtNsliderMoved, slider_callback, gauge);

  XtRealizeWidget(toplevel);
  XtAppMainLoop(app);
}

void
slider_callback(
  Widget widget, 
  XtPointer client_data, 
  XtPointer call_data)
{
  OlSliderVerify *sv = (OlSliderVerify *)call_data;
  int             celsius, fahrenheit;
  Widget          gauge = (Widget)client_data;
  char            values[8];

  celsius = sv->new_location;
  fahrenheit = celsius * 9 / 5 + 32;
  OlSetGaugeValue(gauge, fahrenheit);
  sprintf(values,"C:%2d   F:%3d",celsius, fahrenheit);
  XtVaSetValues(toplevel,
		XtNleftFooterString, values,
		NULL);
}
