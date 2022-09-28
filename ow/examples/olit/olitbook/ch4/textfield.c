/*****************************************************************************
 * textfield.c: Demonstrate the TextField widget.
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
#include <Xol/StaticText.h>
#include <Xol/TextField.h>
#include <stdio.h>

static void verify_callback(Widget, XtPointer, XtPointer);

void
main(unsigned int argc, char **argv)
{
  XtAppContext app;
  Widget       toplevel, control, st[3], tf[3];
  char         name[8];
  int          i;

  OlToolkitInitialize((XtPointer)NULL);
  OlSetDefaultTextFormat(OL_MB_STR_REP);
  toplevel = XtAppInitialize(&app, "Textfield", NULL, 0, &argc, 
			     argv, NULL, NULL, 0);
  control = XtVaCreateManagedWidget("control", controlAreaWidgetClass,
                                    toplevel, NULL);
  /*
   * Create the StaticText and TextField widgets
   */
  for(i=0;i<3;i++) {
    sprintf(name, "st%d",i);
    st[i] = XtVaCreateManagedWidget(name, staticTextWidgetClass,
                                    control, NULL);
    sprintf(name, "tf%d",i);
    tf[i] = XtVaCreateManagedWidget(name, textFieldWidgetClass,
                                    control, NULL);
  }
  XtAddCallback(tf[0], XtNverification, verify_callback, tf[1]);
  XtAddCallback(tf[1], XtNverification, verify_callback, tf[2]);
  XtAddCallback(tf[2], XtNverification, verify_callback, tf[0]);

  XtRealizeWidget(toplevel);
  XtAppMainLoop(app);
}

void
verify_callback(
  Widget    widget, 
  XtPointer client_data, 
  XtPointer call_data)
{
  OlTextFieldVerify *tfv     = (OlTextFieldVerify *)call_data;
  Widget             nextwid = (Widget)client_data;

  /*
   * If the user hits return, move focus to specified field
   */
  if(tfv->reason == OlTextFieldReturn)
    if(!OlCallAcceptFocus(nextwid, CurrentTime))
      printf("cannot set input focus\n");
}
