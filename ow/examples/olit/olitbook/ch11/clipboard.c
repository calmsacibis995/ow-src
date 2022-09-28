/*****************************************************************************
 * clipboard.c: A simple clipboard using X selections
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
#include <Xol/OblongButt.h>
#include <Xol/ControlAre.h>
#include <X11/Xatom.h>

static void    grab_selection(Widget, XtPointer, XtPointer);
static Boolean convert_selection(Widget, Atom *, Atom *, Atom *,
				 XtPointer *, unsigned long *, int *);
static void    lose_selection(Widget, Atom *);
static void    invert_widget(Widget);
static void    show_selection(Widget, XtPointer, Atom *, Atom *,
			      XtPointer, unsigned long *, int *);
static void    request_selection(Widget, XtPointer, XtPointer);

void
main(unsigned int argc, char **argv)
{
  Widget       toplevel, selection, request, con;
  XtAppContext app;

  OlToolkitInitialize((XtPointer)NULL);
  OlSetDefaultTextFormat(OL_MB_STR_REP);
  toplevel = XtAppInitialize(&app, "Clipboard", NULL, 0, &argc, 
			     argv, NULL, NULL, 0);
  con = XtCreateManagedWidget("con", controlAreaWidgetClass,
                              toplevel, NULL, 0);

  /*
   * Create a button used to request the selection and
   * a text widget to display it.
   */
  request = XtCreateManagedWidget("getselection", 
                                  oblongButtonWidgetClass,
                                  con, NULL, 0); 

  selection = XtVaCreateManagedWidget("currentselection", 
                                      staticTextWidgetClass, con, 
                                      XtNstring, "staticText",
                                      NULL);

  XtAddCallback(request, XtNselect, request_selection, selection);
  XtAddCallback(selection, XtNconsumeEvent, grab_selection, NULL);
  
  XtRealizeWidget(toplevel);
  XtAppMainLoop(app);
}

void
request_selection(
  Widget    w,
  XtPointer client_data,
  XtPointer call_data)
{
  Widget selection = (Widget)client_data;

  XtGetSelectionValue(w, XA_PRIMARY, XA_STRING,
                      show_selection, selection, 
                      XtLastTimestampProcessed(XtDisplay(w)));
}

void
show_selection(
  Widget         w,
  XtPointer      client_data,
  Atom          *selection,
  Atom          *type,
  XtPointer      value,
  unsigned long *length,
  int           *format)
{
  Widget select = (Widget)client_data;
  if (*type == XA_STRING) {
    XtVaSetValues(select, XtNstring, value, NULL);
  }
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
  Widget   w,
  Atom    *selection)
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
