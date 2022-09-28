/*****************************************************************************
 * footerpanel.c: Demonstrate the FooterPanel widget.
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
#include <Xol/FooterPane.h>
#include <Xol/Form.h>
#include <Xol/StaticText.h>

typedef struct _footermessage {
  Widget w;
  int     index;
  char  **message;
} footermessage;

void
main(unsigned int argc, char **argv)
{
  static char *leftmess[]  = { "Status Message", "Error Message", "" };
  static char *rightmess[] = { "State", "Mode", "" };
  void       button_callback();
  Widget     toplevel, footerpanel, con, form, status, state,
             st_left, st_right;
  static footermessage left  = { (Widget)0, 0, leftmess };
  static footermessage right = { (Widget)0, 0, rightmess };
  XtAppContext app;

  OlToolkitInitialize((XtPointer)NULL);
  OlSetDefaultTextFormat(OL_MB_STR_REP);
  toplevel = XtAppInitialize(&app, "Footerpanel", (XrmOptionDescList)NULL,
                             0, (Cardinal *)&argc, argv, NULL, 
                             (ArgList) NULL, 0);
  /*
   * Create the base FooterPanel widget.
   */
  footerpanel = XtCreateManagedWidget("footerpanel", 
                                      footerPanelWidgetClass, 
                                      toplevel, NULL, 0);
  /*
   * Create the Top Child, a ControlArea widget
   */
  con = XtCreateManagedWidget("control", controlAreaWidgetClass,
                              footerpanel, NULL, 0);
  /*
   * Create the Footer Child, a Form widget
   */
  form = XtCreateManagedWidget("form", formWidgetClass,
                               footerpanel, NULL, 0);
  /*
   * Create the buttons and static text widgets
   */
  status = XtCreateManagedWidget("Status", oblongButtonWidgetClass,
                                 con, NULL, 0);
  state = XtCreateManagedWidget("State", oblongButtonWidgetClass,
                                con, NULL, 0);
  st_left  = XtCreateManagedWidget("st_left", staticTextWidgetClass,
                                   form, NULL, 0);
  st_right  = XtCreateManagedWidget("st_right", staticTextWidgetClass,
                                    form, NULL, 0);
  left.w = st_left;
  right.w = st_right;
  XtAddCallback(status, XtNselect, button_callback, &left);
  XtAddCallback(state, XtNselect, button_callback, &right);

  XtRealizeWidget(toplevel);
  XtAppMainLoop(app);
}

void
button_callback(w, client_data, call_data)
  Widget w;
  XtPointer client_data, call_data;
{
  Arg     wargs[1];
  footermessage *fm = (footermessage *)client_data;

  XtVaSetValues(fm->w, XtNstring, fm->message[fm->index], NULL);
  fm->index = fm->index == 2 ? 0 : fm->index + 1;
}
