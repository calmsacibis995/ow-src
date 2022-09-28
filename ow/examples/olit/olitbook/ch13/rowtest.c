/*****************************************************************************
 * rowtest.c: Program to test the Row widget
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

#include <X11/StringDefs.h> 
#include <X11/Intrinsic.h> 
#include <Xol/OpenLook.h>
#include <Xol/OblongButt.h> 
#include "Row.h"

void   grow (); 
void   unmanage(); 
void   manage(); 

char *names[] = {"Button1", "Button2", "Button3", "Button4"};

main(argc, argv)
  int    argc;
  char  *argv[];
{
  Widget       toplevel, row, buttons[4];
  Arg          wargs[2];
  int          i;
  XtAppContext app;

  /*
   * Initialize the Intrinsics.
  */
  OlToolkitInitialize((XtPointer)NULL);
  toplevel = XtAppInitialize(&app, "Rowtest", (XrmOptionDescList)NULL,
                             0, (Cardinal *)&argc, argv, NULL, 
                             (ArgList) NULL, 0);
  /*
   * Create a Row widget.
   */
  row = XtCreateManagedWidget("row", XsrowWidgetClass,
                               toplevel, NULL, 0);
  /*
   * Add children to the Row widget.
   */
  for(i=0;i<XtNumber(names);i++)
    buttons[i] = XtCreateWidget(names[i], oblongButtonWidgetClass,
                                row, NULL, 0);

  XtAddCallback(buttons[0], XtNselect, grow , NULL);
  XtAddCallback(buttons[1], XtNselect, unmanage, NULL);
  XtAddCallback(buttons[2], XtNselect, manage, buttons[1]);
  XtAddCallback(buttons[3], XtNselect, grow , NULL);

  XtManageChildren(buttons, XtNumber(buttons));
  XtRealizeWidget(toplevel);
  XtAppMainLoop(app);
}

void grow (w, client_data, call_data)
  Widget     w;
  XtPointer  client_data;
  XtPointer  call_data;
{
  Arg        wargs[3];
  Dimension  width, height;

  /*
   *  Get the current width and height of the widget.
   */
  XtSetArg(wargs[0], XtNwidth,  &width);
  XtSetArg(wargs[1], XtNheight, &height);
  XtGetValues(w, wargs, 2);
  /*
   * Increment the width and height by 10 pixels before
   * setting the size.
   */
  width  +=10;
  height +=10;
  XtSetArg(wargs[0], XtNwidth, width);
  XtSetArg(wargs[1], XtNheight, height);
  XtSetArg(wargs[2], XtNrecomputeSize, FALSE);
  XtSetValues(w, wargs, 3);
}

void
unmanage(w, client_data, call_data)
  Widget     w;
  XtPointer  client_data;
  XtPointer  call_data;
{
  XtUnmanageChild(w);
}

void
manage(w, client_data, call_data)
   Widget     w;
   XtPointer  client_data;
   XtPointer  call_data;
{
  Widget button = (Widget)client_data;

  XtManageChild(button);
}
