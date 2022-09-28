/*****************************************************************************
 * rmtest3.c: simple test of the resource manager
 *            Demonstrate type conversion
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

#define XtRVerlevel "Verlevel"

typedef enum {
    LEVEL1,
    LEVEL2,
    LEVEL3,
    LEVEL4
} verlevel;

static char *levels[] = { "LEVEL1", "LEVEL2", "LEVEL3", "LEVEL4" };

typedef struct {
   Pixel    fg, bg;
   float    delay;
   verlevel verbose;
   Position x, y;
} ApplicationData, *ApplicationDataPtr;

static XtResource resources[] = {
  { XtNforeground, XtCForeground, XtRPixel, sizeof (Pixel),
    XtOffset(ApplicationDataPtr, fg), XtRString, "Black" },
  { XtNbackground, XtCBackground, XtRPixel, sizeof (Pixel),
    XtOffset(ApplicationDataPtr, bg), XtRString, "White" },
  { "delay", "Delay", XtRFloat, sizeof (float),
    XtOffset(ApplicationDataPtr, delay), XtRString, "2.5" },
  { "verbose", "Verbose", XtRVerlevel, sizeof (verlevel),
    XtOffset(ApplicationDataPtr, verbose), XtRString, "LEVEL4"},
  { XtNx, XtCX, XtRPosition, sizeof (Position),
    XtOffset(ApplicationDataPtr, x), XtRImmediate, (XtPointer)25 },
  { XtNy, XtCY, XtRPosition, sizeof (Position),
    XtOffset(ApplicationDataPtr, y), XtRImmediate, (XtPointer)33 },
  };

static XrmOptionDescRec options[] = { 
  {"-verbose", "*verbose", XrmoptionSepArg, NULL},
  {"-delay",   "*delay",   XrmoptionSepArg, NULL}
};

static Boolean str_to_verlevel(
  Display *,
  XrmValuePtr,
  Cardinal *,
  XrmValuePtr,
  XrmValuePtr,
  XtPointer *);

void
main(unsigned int argc, char **argv)
{
  XtAppContext    app;
  Widget          toplevel;
  ApplicationData data;

  OlToolkitInitialize((XtPointer)NULL);
  toplevel = XtAppInitialize(&app, "Rmtest3", options,
                             XtNumber(options), 
                             &argc, argv, NULL, 
                             (ArgList) NULL, 0);
  /*
   *   Add the string to Verlevel type-converter.
   */
  XtAppSetTypeConverter(app, XtRString, XtRVerlevel, str_to_verlevel,
                     (XtConvertArgList)NULL, 0,
                     XtCacheAll, (XtDestructor)NULL);
  /*
   *  Retrieve the resources.
   */
  XtGetApplicationResources(toplevel, &data, resources, 
                            XtNumber(resources), NULL, 0);
  /*
   * Print the result.
   */
  printf("fg=%d, bg=%d, delay=%.2f, verbose=%s, x=%d, y=%d\n", 
         data.fg, data.bg, data.delay, levels[data.verbose], 
         data.x, data.y);
}

Boolean
str_to_verlevel(
  Display     *dpy,
  XrmValuePtr  args,
  Cardinal    *nargs,
  XrmValuePtr  fromVal,
  XrmValuePtr  toVal,
  XtPointer   *data)
{
  static verlevel result;
  /*
   * Make sure the number of args is correct.
   */
  if (*nargs != 0)
    XtWarning("String to verlevel conversion needs no args");
  /*
   * Convert the string in the fromVal to a verlevel pt.
   */
  if(!strcmp(fromVal->addr, "LEVEL1"))
    result =  LEVEL1;
  else if(!strcmp(fromVal->addr, "LEVEL2"))
    result =  LEVEL2;
  else if(!strcmp(fromVal->addr, "LEVEL3"))
    result =  LEVEL3;
  else if(!strcmp(fromVal->addr, "LEVEL4"))
    result =  LEVEL4;
  else {
     XtDisplayStringConversionWarning(dpy, fromVal->addr, 
                                      "Verlevel");
     return FALSE;
  }
  /*
   * Make the toVal point to the result.
   */
  toVal->size = sizeof (verlevel); 
  *(verlevel *)toVal->addr = result; 
  return TRUE;
}
