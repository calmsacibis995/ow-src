/*****************************************************************************
 *  controldata.c: The data controller using properties
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

void       slider_moved();
Widget     speed_ctl, angle_ctl, temp_ctl;
Widget     create_control();
Widget     make_controller();

void
main(unsigned int argc, char **argv)
{
  Widget       toplevel, cont_area;
  flight_data  data;
  XtAppContext app;

  data.speed = data.angle = data.altitude = 0;
  OlToolkitInitialize((XtPointer)NULL);
  OlSetDefaultTextFormat(OL_MB_STR_REP);
  toplevel = XtAppInitialize(&app, "Controldata", NULL, 0, &argc, 
			     argv, NULL, NULL, 0);
  /*
   * Create the atoms to represent the properties
   * used to store the data.
   */
  create_atoms(toplevel);
  cont_area = XtCreateManagedWidget("panel", controlAreaWidgetClass,
                                    toplevel, NULL, 0);
  /*
   *  Make three columns, each containing a label and a
   *  slider control to control: speed, direction,
   *  and altitude.
   */
  speed_ctl = make_controller("speed",     MAX_SPEED,
                              cont_area, &data);
  angle_ctl = make_controller("direction", MAX_ANGLE,
                              cont_area, &data);
  temp_ctl  = make_controller("altitude",  MAX_ALT,
                              cont_area, &data);

  XtRealizeWidget(toplevel);
  XtAppMainLoop(app);
}

create_atoms(w)
  Widget w;
{
  Display     *dpy = XtDisplay(w);
  FLIGHT_DATA      = XInternAtom(dpy, "Flight Data",      0);
  FLIGHT_DATA_TYPE = XInternAtom(dpy, "Flight Data Type", 0);
}

Widget
make_controller(name, max, parent, data)
  char          *name;
  int            max;
  Widget         parent;
  flight_data  *data;
{
  Widget rc, w;

  /*
   * Create a ControlArea widget to manage a single
   * control and a label.
   */
  rc = XtCreateManagedWidget(name, controlAreaWidgetClass,
                             parent, NULL, 0);
  XtCreateManagedWidget("label", staticTextWidgetClass,
                        rc, NULL, 0);
  w = create_control(rc, "control", 0, max, data);

  return (w);
}

Widget
create_control(parent, name, minimum, maximum, data)
  Widget        parent;
  char         *name;
  int           minimum, maximum;
  flight_data  *data;
{
  int    n;
  Arg    wargs[2];
  Widget w;

  /*
   * Create a slider with range minimum to maximum.
   */
  n = 0;
  XtSetArg(wargs[n], XtNsliderMin, minimum); n++;
  XtSetArg(wargs[n], XtNsliderMax, maximum); n++;
  w = XtCreateManagedWidget(name, sliderWidgetClass,
                            parent, wargs, n);
  /*
   * Register callback function for when the user moves the
   * slider.
   */
  XtAddCallback(w, XtNsliderMoved, slider_moved, data);
  return (w);
}

void
slider_moved(w, client_data, call_data)
  Widget        w;
  XtPointer     client_data;
  XtPointer     call_data;
{
  int    n;
  Arg    wargs[2];
  int max, min;
  flight_data  *data = (flight_data *)client_data;
  int value = *(int *)call_data;

  /*
   * Set the member of the flight_data corresponding to
   * the slider that invoked this callback.
   */
  n = 0;
  XtSetArg(wargs[n], XtNsliderMin, &min); n++;
  XtSetArg(wargs[n], XtNsliderMax, &max); n++;
  XtGetValues(w, wargs, n);
  if(w == angle_ctl)
    data->angle = value;
  else if(w == speed_ctl)
    data->speed = value;
  else if(w == temp_ctl)
    data->altitude = (float) value / 10.0;
  /*
   * Replace the previous contents of the property
   * with the new data.
   */
  XChangeProperty(XtDisplay(w),
                  DefaultRootWindow(XtDisplay(w)),
                  FLIGHT_DATA, FLIGHT_DATA_TYPE,
                  32, PropModeReplace,
                  (unsigned char *) data,
                  sizeof(flight_data) / 4);
}
