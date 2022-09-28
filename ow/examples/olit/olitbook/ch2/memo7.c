/*****************************************************************************
 * memo7.c: Demonstrate an event handler
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
#include <Xol/StaticText.h>

static void enterleave(Widget, XtPointer, XEvent *, Boolean *);

void
main(unsigned int argc, char **argv)
{
  XtAppContext app;
  Widget       toplevel, msg_widget;

  /*
   * Initialize OLIT and the Intrinsics
   */
  OlToolkitInitialize((XtPointer)NULL);
  toplevel = XtAppInitialize(&app, "Memo", NULL, 0, &argc, 
			     argv, NULL, NULL, 0);
  /*
   * Create the staticText widget.
   */
  msg_widget = XtVaCreateManagedWidget("msg", staticTextWidgetClass,
                                       toplevel,
				       XtNstring, "Hello World",
				       NULL);
  /*
   * Register two event handlers to be called when 
   * there is an Enter or Leave event.
   */
  XtAddEventHandler(msg_widget, EnterWindowMask, FALSE,
                    enterleave, "Enter Window");
  XtAddEventHandler(msg_widget, LeaveWindowMask, FALSE,
                    enterleave, "Leave Window");
  /*
   * Realize the widgets and enter the event loop.
   */
  XtRealizeWidget(toplevel);
  XtAppMainLoop(app);
}

void
enterleave(
  Widget     widget, 
  XtPointer  client_data, 
  XEvent    *event, 
  Boolean   *continue_to_dispatch)
{
  String message = (String)client_data;

  XtVaSetValues(widget, XtNstring, message, NULL);
}
