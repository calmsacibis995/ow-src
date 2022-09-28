/*****************************************************************************
 * notice.c: Demonstrate the Notice widget.
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
#include <Xol/Notice.h>
#include <stdio.h>

static void button_callback(Widget, XtPointer, XtPointer);
static void confirm_callback(Widget, XtPointer, XtPointer);

void
main(unsigned int argc, char **argv)
{
  XtAppContext app;
  Widget       toplevel, con, button, notice; 
  Widget       n_controlarea, n_textarea, confirm, cancel;

  OlToolkitInitialize((XtPointer)NULL);
  OlSetDefaultTextFormat(OL_MB_STR_REP);
  toplevel = XtAppInitialize(&app, "Notice", NULL, 0, &argc, 
			     argv, NULL, NULL, 0);
  con = XtVaCreateManagedWidget("control", controlAreaWidgetClass, 
                                toplevel, NULL);
  /*
   * Create the OblongButton widget.
   */
  button = XtVaCreateManagedWidget("Print", oblongButtonWidgetClass,
                                   con, NULL);
  /*
   * Create the notice widget
   * The notice widget emanates from the button widget.
   */
  notice = XtVaCreatePopupShell("notice", noticeShellWidgetClass,
                                button, 
				XtNemanateWidget, button,
				NULL);
  /*
   * Retrieve the XtNtextArea and XtNcontrolArea widgets.
   */
  XtVaGetValues(notice,
		XtNtextArea,    &n_textarea,
		XtNcontrolArea, &n_controlarea,
		NULL);
  /*
   * Set the message and create the buttons on the notice pop-up.
   */
  confirm = XtVaCreateManagedWidget("Confirm", oblongButtonWidgetClass,
                                    n_controlarea, NULL);
  cancel = XtVaCreateManagedWidget("Cancel", oblongButtonWidgetClass,
                                   n_controlarea, NULL);
  XtAddCallback(confirm, XtNselect, confirm_callback, NULL);
  XtAddCallback(button, XtNselect, button_callback, notice);

  XtRealizeWidget(toplevel);
  XtAppMainLoop(app);
}

void 
button_callback(
  Widget    widget, 
  XtPointer client_data, 
  XtPointer call_data)
{
  Widget notice = (Widget)client_data;

  XtPopup(notice, XtGrabExclusive);
}

void 
confirm_callback(
  Widget    widget, 
  XtPointer client_data, 
  XtPointer call_data)
{
  printf("Message\n");
}
