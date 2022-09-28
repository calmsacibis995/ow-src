/*****************************************************************************
 *  monitordata.c: display the data set by controldata
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

#include "data.h"

Widget    make_display();

void
main(unsigned int argc, char **argv)
{
  Widget       toplevel, rc, speed, direction,  altitude;
  Window       root;
  XEvent       event;
  XtAppContext app;

  /* 
   * Initialize the Intrinsics,saving the default root window.
   */
  OlToolkitInitialize((XtPointer)NULL);
  OlSetDefaultTextFormat(OL_MB_STR_REP);
  toplevel = XtAppInitialize(&app, "Monitordata", NULL, 0, &argc, 
			     argv, NULL, NULL, 0);
  root =  DefaultRootWindow(XtDisplay(toplevel));
  /*
   * Initialize the Atoms used for the properties.
   */
  create_atoms(toplevel);
  rc = XtCreateManagedWidget("panel", controlAreaWidgetClass,
                             toplevel,  NULL, 0);
  /*
   * Create the display widgets.
   */
  speed      = make_display("speed",     rc);
  direction  = make_display("direction", rc);
  altitude   = make_display("altitude",  rc);

  XtRealizeWidget(toplevel);
  /*
   * Request property change event for the ROOT window.
   */
  XSelectInput(XtDisplay(toplevel), root, PropertyChangeMask);
  /*
   *  Get the initial value of the data.
   */
  update_data(speed, direction, altitude);
  /*
   * We must use our own event loop to get properties 
   * events for the ROOT window.
   */
  while(TRUE){
    XtAppNextEvent(app, &event);
    /*
     * Check for property change events on the ROOT window
     * before dispatching the event through the Intrinsics.
     */
    switch (event.type){
      case PropertyNotify:
        if(event.xproperty.window == root &&
           event.xproperty.atom == FLIGHT_DATA)
         update_data(speed, direction, altitude);
        else
         XtDispatchEvent(&event);
        break;
      default:
         XtDispatchEvent(&event);
    }
  }
}


update_data(speed, direction, altitude)
  Widget speed, direction, altitude;
{
  int            format;
  unsigned long  nitems, left;
  flight_data   *retdata;
  char           str[100];
  Atom           type;

  /*
   * Retrieve the data from the root window property.
   */
  if(XGetWindowProperty(XtDisplay(speed), 
                        DefaultRootWindow(XtDisplay(speed)),
                        FLIGHT_DATA, 0, sizeof(flight_data), 
                        FALSE, FLIGHT_DATA_TYPE,
                        &type, &format, &nitems, &left,
                        (unsigned char **)&retdata) == Success &&
       type ==FLIGHT_DATA_TYPE){
    /*
     * If the data exists, display it.
     */
    sprintf(str, "%2d", retdata->speed);
    XtVaSetValues(speed, XtNstring, str, NULL);
    sprintf(str, "%2d", retdata->angle);
    XtVaSetValues(direction, XtNstring, str, NULL);
    sprintf(str, "%2d", retdata->altitude + 0.05);
    XtVaSetValues(altitude, XtNstring, str, NULL);
    XFree((XtPointer)retdata);
  }
}

Widget make_display(name, parent)
  char     *name;
  Widget    parent;
{
  Widget cap, w;

  /*
   * Create a ControlArea widget containing two
   * StaticText widgets.
   */
  cap = XtCreateManagedWidget(name, staticTextWidgetClass, 
                              parent, NULL, 0);
  w = XtCreateManagedWidget("display", staticTextWidgetClass, 
                            parent, NULL, 0);
  return (w);
}

create_atoms(w)
  Widget w;
{
  Display * dpy = XtDisplay(w);
  FLIGHT_DATA      = XInternAtom(dpy, "Flight Data",      0);
  FLIGHT_DATA_TYPE = XInternAtom(dpy, "Flight Data Type", 0);
}
