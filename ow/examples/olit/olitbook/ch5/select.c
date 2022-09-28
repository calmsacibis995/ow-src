/*****************************************************************************
 * select.c: Demo a main loop that uses select() to do 
 *           background processing
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
#include <Xol/OpenLook.h>
#include <Xol/OblongButt.h>
#include <sys/time.h>
#include <signal.h>
#include <errno.h>
#include <stdio.h>

/*
 * Define callbacks.
 */

void activate_button(
  Widget    widget,
  XtPointer client_data,
  XtPointer call_data)
{
  printf("Button activated\n");
}

void
main(unsigned int argc, char **argv)
{
  Widget       toplevel, button;
  XtAppContext app;
   
  OlToolkitInitialize((XtPointer)NULL);
  toplevel = XtAppInitialize(&app, "Select", NULL, 0, &argc, 
			     argv, NULL, NULL, 0);
  /*
   * Create the pushbutton widget.
   */
  button = XtCreateManagedWidget("button", oblongButtonWidgetClass, 
                                 toplevel, NULL, 0);
  /*
   * Add select callbacks.
   */
  XtAddCallback(button, XtNselect, activate_button, NULL);

  XtRealizeWidget(toplevel);

  while(TRUE){
    if(XtPending()){
      XEvent event;
      XtAppNextEvent(app, &event);
      XtDispatchEvent (&event);
    } else {
      struct timeval timeout;
      int readfds = 0;
      int maxfds = 1 + ConnectionNumber(XtDisplay(toplevel));
      timeout.tv_sec = 1;
      timeout.tv_usec = 0; 
      /* Do something else for a while */
      printf("doing something in the background\n");
      readfds = 1 << ConnectionNumber(XtDisplay(toplevel));
      if (select(maxfds, (fd_set *)&readfds, NULL, NULL, &timeout) == -1){
        if (EINTR != errno)
          exit(1);
      }
    }
  }
}
