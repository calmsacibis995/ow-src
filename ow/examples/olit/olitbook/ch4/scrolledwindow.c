/*****************************************************************************
 * scrolledwindow.c: Demonstrate the ScrolledWindow widget.
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
#include <Xol/TextEdit.h>
#include <Xol/StaticText.h>
#include <Xol/OblongButt.h>
#include <Xol/RubberTile.h>
#include <Xol/CheckBox.h>
#include <Xol/ScrolledWi.h>
#include <stdio.h>

main(argc, argv)
  int argc;
  char *argv[];
{
  XtAppContext app;
  Widget       toplevel, sw, te, clear, retrieve, con, rt, cb;
  void         clear_callback(), insert_callback(), 
               getbuffer_callback(), verify_callback(), 
               motionverify_callback();
  String       filename = NULL;
  Arg          wargs[3];
  int          n;

  OlToolkitInitialize((XtPointer)NULL);
  toplevel = XtAppInitialize(&app, "Scrolledwindow", (XrmOptionDescList)NULL,
                             0, (Cardinal *)&argc, argv, NULL, 
                             (ArgList) NULL, 0);
  if(argc < 2) {
    printf("Usage: scrolledwindow filename\n");
    exit(0);
  }
  /*
   * Create the RubberTile and ControlArea widgets.
   */
  rt = XtCreateManagedWidget("rt", rubberTileWidgetClass,
                             toplevel, NULL, 0);
  con = XtCreateManagedWidget("con", controlAreaWidgetClass,
                              rt, NULL, 0);
  /*
   * Get command line argument, if any, and create the
   * ScrolledWindow and TextEdit widgets
   */
  sw = XtCreateManagedWidget("sw", scrolledWindowWidgetClass,
                         rt, NULL, 0);
  filename = argv[1];
  n = 0;
  if(filename) {
    XtSetArg(wargs[n], XtNsource, filename); n++;
    XtSetArg(wargs[n], XtNsourceType, OL_DISK_SOURCE); n++;
  }
  XtSetArg(wargs[n], XtNwrapMode, OL_WRAP_OFF); n++;
  te = XtCreateManagedWidget("te", textEditWidgetClass,
                             sw, wargs, n);
  clear = XtCreateManagedWidget("clear", 
                                oblongButtonWidgetClass,
                                con, NULL, 0);
  XtAddCallback(clear, XtNselect, clear_callback, te);
  retrieve = XtCreateManagedWidget("retrieve", 
                                   oblongButtonWidgetClass,
                                   con, NULL, 0);
  XtAddCallback(retrieve, XtNselect, getbuffer_callback, te);
  cb = XtCreateManagedWidget("cb", checkBoxWidgetClass,
                             con, NULL, 0);
  XtAddCallback(te, XtNmotionVerification, 
                motionverify_callback, cb);

  XtRealizeWidget(toplevel);
  XtAppMainLoop(app);
}

void
clear_callback(w, client_data, call_data)
  Widget w;
  XtPointer client_data, call_data;
{
  Widget te = (Widget)client_data;

  (void)OlTextEditClearBuffer((TextEditWidget)te);
}

void
getbuffer_callback(w, client_data, call_data)
  Widget w;
  XtPointer client_data, call_data;
{
  Widget te = (Widget)client_data;
  Boolean retval;
  char *buffer;

  retval = OlTextEditCopyBuffer((TextEditWidget)te, &buffer);
  if(retval == FALSE) {
    printf("Unable to retrieve buffer\n");
    return;
  }
  printf("buffer = %s\n", buffer);
  XtFree(buffer);
}

void
motionverify_callback(w, client_data, call_data)
  Widget w;
  XtPointer client_data, call_data;
{
  OlTextMotionCallData *tmcd = 
                        (OlTextMotionCallData *)call_data;
  Widget checkbox = (Widget)client_data;
  Arg wargs[1];
  Boolean set;

  XtSetArg(wargs[0], XtNset, &set);
  XtGetValues(checkbox, wargs, 1);
  if(set)
    printf("%s %d, %s = %d, %s = %d, %s = %d, %s = %d\n",
           "ok", tmcd->ok,
           "current_cursor", tmcd->current_cursor,
           "new_cursor", tmcd->new_cursor,
           "select_start", tmcd->select_start,
           "select_end", tmcd->select_end);
}
