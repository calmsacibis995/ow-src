/*****************************************************************************
 * xclock.c:
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
#include <time.h>

void update_time(XtPointer, XtIntervalId *);
XtAppContext app;

void
main(unsigned int argc, char **argv)
{
  Widget       toplevel, clock;

  /*
   * Create the widgets.
   */
  OlToolkitInitialize((XtPointer)NULL);
  OlSetDefaultTextFormat(OL_MB_STR_REP);
  toplevel = XtAppInitialize(&app, "XClock", NULL, 0, &argc, 
			     argv, NULL, NULL, 0);
  clock = XtCreateManagedWidget("face", staticTextWidgetClass,
                                toplevel, NULL, 0);
  /*
   * Get the initial time.
   */
  update_time((XtPointer)clock, NULL);
  XtRealizeWidget(toplevel);
  XtAppMainLoop(app);
}

void
update_time(
  XtPointer      client_data,
  XtIntervalId  *id)
{
  Widget w = (Widget)client_data;
  long   next_minute;
  char   thetime[256];
  time_t clock, rounded_clock;
  struct tm *tm;

  /*
   * Ask Unix for the time.
   */
  clock = time((time_t *)0);
  /*
   * Convert the time to a string and display it,
   * after rounding it down to the last minute.
   */
  rounded_clock = clock / 60 * 60;
  tm = localtime(&rounded_clock);
  strftime(thetime, sizeof(thetime), "%c %Z", tm);
  XtVaSetValues(w, XtNstring, thetime, NULL);
  /*
   * Adjust the time to reflect the time till 
   * the next round minute.
   */
  next_minute = (60 - clock % 60) * 1000;
  /*
   * The Intrinsics removes timeouts when they occur,
   * so put ourselves back. 
   */
  XtAppAddTimeOut(app, next_minute, update_time, w);
}
