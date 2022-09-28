/*****************************************************************************
 * rmtest2.c: simple test of the resource manager
 *            retrieve resources from command line
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
#include <stdio.h>

typedef struct {
   Pixel    fg, bg;
   int      delay;
   Boolean  verbose;
   Position x, y;
} ApplicationData, *ApplicationDataPtr;

static XtResource resources[] = {
{ XtNforeground, XtCForeground, XtRPixel, sizeof (Pixel),
  XtOffset(ApplicationDataPtr, fg), XtRString, "Black" },
{ XtNbackground, XtCBackground, XtRPixel, sizeof (Pixel),
  XtOffset(ApplicationDataPtr, bg), XtRString, "White" },
{ "delay", "Delay", XtRInt, sizeof (int),
  XtOffset(ApplicationDataPtr, delay),
  XtRImmediate, (XtPointer) 2},
{ "verbose", "Verbose", XtRBoolean, sizeof (Boolean),
  XtOffset(ApplicationDataPtr, verbose), XtRString, "FALSE"},
{ XtNx, XtCX, XtRPosition, sizeof (Position),
  XtOffset(ApplicationDataPtr, x), XtRImmediate, (XtPointer)25 },
{ XtNy, XtCY, XtRPosition, sizeof (Position),
  XtOffset(ApplicationDataPtr, y), XtRImmediate, (XtPointer)33 },
};

XrmOptionDescRec options[] = {
  {"-verbose", "*verbose", XrmoptionNoArg, "TRUE"},
  {"-delay",   "*delay",   XrmoptionSepArg, NULL }
};

void
main(unsigned int argc, char **argv)
{
  XtAppContext    app;
  Widget          toplevel;
  ApplicationData data;

  OlToolkitInitialize((XtPointer)NULL);
  toplevel = XtAppInitialize(&app, "Rmtest", options,
                             XtNumber(options), &argc, 
			     argv, NULL, NULL, 0);
  /*
   * Retrieve the application resources.
   */
  XtGetApplicationResources(toplevel, &data, resources, 
                            XtNumber(resources), NULL, 0);
  /*
   * Print the results.
   */
  printf("fg=%d, bg=%d, delay=%d, verbose=%d, x=%d, y=%d\n", 
         data.fg, data.bg, data.delay, data.verbose, 
         data.x, data.y);
}
