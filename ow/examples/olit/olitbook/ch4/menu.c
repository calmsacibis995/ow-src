/*****************************************************************************
 * menu.c: Demonstrate the Menu widget.
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
#include <Xol/StaticText.h>
#include <Xol/Menu.h>
#include <stdio.h>

static char *button_names[] = { "New", "Open", "Save", "Print" };
static void button_callback(Widget, XtPointer, XtPointer);

void
main(unsigned int argc, char **argv)
{
  XtAppContext app;
  Widget       toplevel, con, menu, menu_pane, st, 
               oblong[XtNumber(button_names)];
  int          i;

  OlToolkitInitialize((XtPointer)NULL);
  OlSetDefaultTextFormat(OL_WC_STR_REP);
  toplevel = XtAppInitialize(&app, "Menu", NULL, 0, &argc, 
			     argv, NULL, NULL, 0);
  con = XtVaCreateManagedWidget("control", controlAreaWidgetClass, 
                                toplevel, NULL);
  /*
   * Create a StaticText widget
   */
  st = XtVaCreateManagedWidget("st", staticTextWidgetClass, 
                               con, NULL);
  /*
   * Create menu widget.
   */
  menu = XtVaCreatePopupShell("popup", menuShellWidgetClass,
                              st, NULL);
  /*
   * Get the MenuPane widget
   */
  XtVaGetValues(menu, XtNmenuPane, &menu_pane, NULL);
  /*
   * Create the buttons on the menu pane.
   */
  for(i=0;i<XtNumber(button_names);i++) {
    oblong[i] = XtVaCreateManagedWidget(button_names[i], 
                                      oblongButtonWidgetClass,
                                      menu_pane, NULL);
    XtAddCallback(oblong[i], XtNselect, button_callback, NULL);
  }

  XtRealizeWidget(toplevel);
  XtAppMainLoop(app);
}

void
button_callback(
  Widget widget,
  XtPointer client_data,
  XtPointer call_data)
{
  OlStr label;

  XtVaGetValues(widget, XtNlabel, &label, NULL);
  printf("%ws\n", label);
}
