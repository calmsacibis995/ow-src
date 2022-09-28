/*****************************************************************************
 * memo6.c: Demonstrate an action proc with arguments. This version requires
 *          an entry like this:
 *
 *          *msg*translations: #override \n\
 *		<BtnDown>: OlAction() action_proc(5, 9)
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
#include <Xol/OblongButt.h>
#include <stdio.h>
#include <stdlib.h>

static void printit(Widget, XEvent *, String *, Cardinal *);
static void select_callback(Widget, XtPointer, XtPointer);

static XtActionsRec actionsTable [] = {
  {"action_proc",   printit},
};

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
   * Register the new actions, and compile the translations table
   */
  XtAppAddActions(app, actionsTable, XtNumber(actionsTable));
  /*
   * Create the OblongButton widget.
   */
  msg_widget = XtVaCreateManagedWidget("msg", oblongButtonWidgetClass,
                                       toplevel,
				       XtNlabel, "Press Me",
				       NULL);
  /*
   * Register the callback to be called when the button is pressed
   */
  XtAddCallback(msg_widget, XtNselect, select_callback, NULL);
  /*
   * Realize all widgets and enter the event loop.
   */
  XtRealizeWidget(toplevel);
  XtAppMainLoop(app);
}

void
select_callback(
  Widget    widget,
  XtPointer client_data,
  XtPointer call_data)
{
  String name, class;
  
  XtGetApplicationNameAndClass(XtDisplay(widget), &name, &class);
  printf("Application Name: %s\n", name);
  printf("Application Class: %s\n", class);
} 

void
printit(
  Widget    widget, 
  XEvent   *event, 
  String   *params, 
  Cardinal *num_params)
{
  if(*num_params == 2)
    printf("printit action procedure invoked with arguments %d and %d\n",
	    atoi(params[0]), atoi(params[1]));
}
