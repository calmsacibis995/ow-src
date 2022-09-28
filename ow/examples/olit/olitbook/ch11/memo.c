/*****************************************************************************
 * memo.c: Selection version of the memo program
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
#include <Xol/ControlAre.h>
#include <X11/Xatom.h>

static void    grab_selection(Widget, XtPointer, XtPointer);
static Boolean convert_selection(Widget, Atom *, Atom *, Atom *,
				 XtPointer *, unsigned long *, int *);
static void    lose_selection(Widget, Atom *);
static void    invert_widget(Widget);

void
main(unsigned int argc, char **argv)
{
  XtAppContext app;
  Widget       toplevel, msg_widget, con;
  String       message;

  OlToolkitInitialize((XtPointer)NULL);
  OlSetDefaultTextFormat(OL_MB_STR_REP);
  toplevel = XtAppInitialize(&app, "Memo", NULL, 0, &argc, 
			     argv, NULL, NULL, 0);
  con = XtCreateManagedWidget("con", controlAreaWidgetClass,
                              toplevel, NULL, 0);
  /*
   * Get the contents of the command line and display it in
   * the message window.
   */
  if(argc > 1) {
    message = argv[1];
  } else {
    message = "Hello World";
  }
  msg_widget = XtVaCreateManagedWidget("message", 
                                       staticTextWidgetClass,
                                       con,
				       XtNstring, message,
				       NULL);
  XtAddCallback(msg_widget, XtNconsumeEvent, grab_selection, NULL);
  /*
   * Realize all widgets and enter the event loop.
   */
  XtRealizeWidget(toplevel);
  XtAppMainLoop(app);
}

void
grab_selection(
  Widget    w,
  XtPointer client_data,
  XtPointer call_data)
{
  OlVirtualEvent vevent = (OlVirtualEvent)call_data;
  /*
   * Claim ownership of the PRIMARY selection.
   */

  if(vevent->virtual_name == OL_SELECT) {
    if(XtOwnSelection(w, XA_PRIMARY, 
                      XtLastTimestampProcessed(XtDisplay(w)),
                      convert_selection,  /* handle requests */
                      lose_selection,     /* Give up selection*/
                      (XtSelectionDoneProc) NULL) == TRUE) {
      invert_widget(w);
      XtSetSensitive(w, FALSE);
      vevent->consumed = TRUE;
    }
  }
}

Boolean
convert_selection(
  Widget         w,
  Atom          *selection,
  Atom          *target,
  Atom          *type,
  XtPointer     *value,
  unsigned long *length,
  int           *format)
{
  String str;
  static String savestr = NULL;

  if (*target == XA_STRING) {
    if(savestr == NULL) {
        XtVaGetValues(w, XtNstring, &str, NULL);
        savestr = XtNewString(str);
    }
    *type   = XA_STRING;
    *value  = (XtPointer)savestr;
    *length = strlen(*value);
    *format = 8;
    return(TRUE);
  } else
    return(FALSE);
}

void
lose_selection(
  Widget w,
  Atom  *selection)
{
  invert_widget(w);
  XtSetSensitive(w, TRUE);
}

void
invert_widget(Widget w)
{
  Pixel fc, bg;

  /*
   * Get the widget's current colors.
   */
  XtVaGetValues(w,
		XtNfontColor, &fc,
		XtNbackground, &bg,
		NULL);
  /*
   * Reverse them and set the new colors.
   */
  XtVaSetValues(w,
		XtNfontColor, bg,
		XtNbackground, fc,
		NULL);
}
