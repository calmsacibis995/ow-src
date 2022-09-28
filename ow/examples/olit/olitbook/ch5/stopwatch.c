/*****************************************************************************
 * stopwatch.c: A digital stopwatch using workprocs.
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
#include <Xol/OblongButt.h>
#include <time.h>
#include <stdio.h>

Boolean update_time(Widget);
void    start_timing(Widget, XtPointer, XtPointer);
void    stop_timing(Widget, XtPointer, XtPointer);

long         start_time;
XtWorkProcId work_proc_id = NULL;
XtAppContext app;

void
main(unsigned int argc, char **argv)
{
  Widget toplevel, panel, commands, start, stop, timer;
  char   str[32];

  OlToolkitInitialize((XtPointer)NULL);
  OlSetDefaultTextFormat(OL_MB_STR_REP);
  toplevel = XtAppInitialize(&app, "Stopwatch", NULL, 0, &argc, 
			     argv, NULL, NULL, 0);
  /*
   * Create a ControlArea widget to hold everything.
   */
  panel = XtCreateManagedWidget("panel", controlAreaWidgetClass,
                                toplevel, NULL, 0);
  /*
   * A StaticText widget shows the current time.
   */ 
  timer = XtCreateManagedWidget("timer", staticTextWidgetClass,
                                panel, NULL, 0);
  /*
   * Add start and stop buttons and register callbacks.
   * Pass the timer widget to all callbacks.
   */ 
  commands = XtCreateManagedWidget("commands", 
                                   controlAreaWidgetClass,
                                   panel, NULL, 0);
  start = XtCreateManagedWidget("start", oblongButtonWidgetClass,
                                 commands, NULL, 0);
  XtAddCallback(start, XtNselect, start_timing, timer);
  stop = XtCreateManagedWidget("stop", oblongButtonWidgetClass,
                               commands, NULL, 0);
  XtAddCallback(stop, XtNselect, stop_timing, NULL);
  sprintf(str, "%02d : %02d", 0, 0);
  XtVaSetValues(timer, XtNstring, str, NULL);
 
  XtRealizeWidget(toplevel);
  XtAppMainLoop(app);
}

void
start_timing(
  Widget    widget,
  XtPointer client_data,
  XtPointer call_data)
{
  Widget timer = (Widget)client_data;

  /* 
   * Get the initial time, and save it in a global.
   */
  time(&start_time); 
  /*
   * If a WorkProc has already been added, remove it.
   */
  if(work_proc_id)
    XtRemoveWorkProc(work_proc_id);
  /*
   * Register update_time() as a WorkProc.
   */
  work_proc_id = XtAppAddWorkProc(app, update_time, timer);
}

void
stop_timing(
  Widget    widget,
  XtPointer client_data,
  XtPointer call_data)
{
  if(work_proc_id)
    XtRemoveWorkProc(work_proc_id);
  work_proc_id = NULL; 
}

Boolean
update_time(
  Widget   w)
{
  static long elapsed_time, current_time, last_time = -1;
  int         minutes, seconds;
  char        str[32];

  /*
   * Retrieve the current time and calculate the elapsed time.
   */     
  time(&current_time);
  elapsed_time = current_time - start_time;
  /*
   * WorkProcs are irregularly called; don't update the
   * display if it's been less than a second since the last
   * time it was updated.
   */
  if(last_time == elapsed_time)
    return(FALSE);
  /*
   * If one or more seconds has elapsed, remember this time,
   * and convert the elapsed time to minutes and seconds. 
   */
  last_time = elapsed_time;
  minutes = elapsed_time / 60;
  seconds = elapsed_time % 60;
  /*
   * Display the time as minutes and seconds.
   */
  sprintf(str, "%02d : %02d", minutes, seconds);
  XtVaSetValues(w, XtNstring, str, NULL);
  /*
   * Return FALSE so this WorkProc keeps getting called.
   */
  return(FALSE);
}
