/*****************************************************************************
 * oblong.c: Demonstrate the OblongButton widget.
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
#include <stdio.h>

static void button_callback(Widget, XtPointer, XtPointer);

void
main(unsigned int argc, char **argv)
{
  XtAppContext app;
  Widget       toplevel, con, but1;

  OlToolkitInitialize((XtPointer)NULL);
  OlSetDefaultTextFormat(OL_WC_STR_REP);
  toplevel = XtAppInitialize(&app, "Oblong", NULL, 0, &argc, 
			     argv, NULL, NULL, 0);
  con = XtVaCreateManagedWidget("control", controlAreaWidgetClass, 
                                toplevel, NULL);
  /*
   * Create the OblongButton widget.
   */
  but1 = XtVaCreateManagedWidget("button", oblongButtonWidgetClass,
                                 con, NULL);
  XtAddCallback(but1, XtNselect, button_callback, NULL);

  XtRealizeWidget(toplevel);
  XtAppMainLoop(app);
}

void 
button_callback(
  Widget    widget, 
  XtPointer client_data, 
  XtPointer call_data)
{
  printf("callback\n");
}
