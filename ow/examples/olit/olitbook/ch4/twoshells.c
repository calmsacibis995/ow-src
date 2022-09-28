/*****************************************************************************
 * twoshells.c: Example of two independent top-level shells
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
#include <X11/Shell.h> 
#include <Xol/OpenLook.h> 

void
main(unsigned int argc, char **argv)
{
  XtAppContext app;
  Widget       toplevel, shell2;

  /*
   * Initialize the Intrinsics, create one ApplicationShell.
   */
  OlToolkitInitialize((XtPointer)NULL);
  toplevel = XtAppInitialize(&app, "Twoshells", NULL, 0, &argc, 
			     argv, NULL, NULL, 0);
  /*
   * Create a second ApplicationShell widget. 
   */
  shell2 = XtVaAppCreateShell("window2", "Window2",
                              applicationShellWidgetClass,
                              XtDisplay(toplevel),
                              NULL);
  /*
   * Create a Core widget as a child of each shell.
   */
  XtVaCreateManagedWidget("Widget", widgetClass, toplevel, NULL);
  XtVaCreateManagedWidget("Widget2", widgetClass, shell2, NULL);
  /*
   * Realize both shell widgets.
   */
  XtRealizeWidget(toplevel);
  XtRealizeWidget(shell2);
  XtAppMainLoop(app);
}
