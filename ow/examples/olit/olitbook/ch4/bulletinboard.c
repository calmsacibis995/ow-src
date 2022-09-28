/*****************************************************************************
 * bulletinboard.c: Demonstrate the BulletinBoard widget
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
#include <Xol/OblongButt.h>
#include <Xol/BulletinBo.h>
#include <Xol/StaticText.h>

typedef struct _appWidgets {
  Widget bb;
  Widget st;
  Widget *buttons;
} appWidgets;

static String button_names[] = { "Minimize", "Grow", "Shrink", "Maximize" };
static void   minimize_callback (Widget, XtPointer, XtPointer); 
static void   maximize_callback (Widget, XtPointer, XtPointer);
static void   resize_callback   (Widget, XtPointer, XtPointer);
static void   position_text     (Widget, Widget);
static void   position_button   (Widget, Position, Position);

void
main(unsigned int argc, char **argv)
{
  XtAppContext app;
  Widget       toplevel, bulletinboard;
  Widget       statictext, button[XtNumber(button_names)];
  int          i;
  appWidgets   mywidgets;

  OlToolkitInitialize((XtPointer)NULL);
  OlSetDefaultTextFormat(OL_MB_STR_REP);
  toplevel = XtVaAppInitialize(&app, "Bulletinboard", NULL, 
			       0, &argc, argv, NULL, NULL);
  /*
   * Create the base area Bulletin Board widget.
   */
  bulletinboard = XtVaCreateManagedWidget("bulletinboard", 
                               bulletinBoardWidgetClass, 
                               toplevel, NULL);
  /*
   * Create the four control buttons
   */
  for(i=0;i<XtNumber(button_names);i++) {
      button[i] = XtVaCreateManagedWidget(button_names[i],
                               oblongButtonWidgetClass,
                               bulletinboard, NULL);
  }
  statictext = XtVaCreateManagedWidget("st", staticTextWidgetClass,
                               bulletinboard, 
			       XtNstring, "OL_MINIMIZE",
			       NULL);
  mywidgets.bb = bulletinboard;
  mywidgets.st = statictext;
  mywidgets.buttons = button;
  XtAddCallback(button[0], XtNselect, minimize_callback, mywidgets);
  XtAddCallback(button[1], XtNselect, resize_callback,   mywidgets);
  XtAddCallback(button[2], XtNselect, resize_callback,   mywidgets);
  XtAddCallback(button[3], XtNselect, maximize_callback, mywidgets);
  XtRealizeWidget(toplevel);
  position_text(bulletinboard, statictext);
  XtAppMainLoop(app);
}

void
minimize_callback(
  Widget widget, 
  XtPointer client_data, 
  XtPointer call_data)
{
  appWidgets *mywidgets = (appWidgets *)client_data;

  XtVaSetValues(mywidgets->bb, 
		XtNlayout, OL_MINIMIZE,
		NULL);
  XtVaSetValues(mywidgets->st, 
		XtNstring, "OL_MINIMIZE", 
		NULL);
  position_text(mywidgets->bb, mywidgets->st);
}

void
maximize_callback(
  Widget widget, 
  XtPointer client_data, 
  XtPointer call_data)
{
  appWidgets *mywidgets = (appWidgets *)client_data;

  XtVaSetValues(mywidgets->bb,
		XtNlayout, OL_MAXIMIZE,
		NULL);
  XtVaSetValues(mywidgets->st,
		XtNstring, "OL_MAXIMIZE",
		NULL);
}

void
resize_callback(
  Widget widget, 
  XtPointer client_data, 
  XtPointer call_data)
{
  String name;
  Position delta;
  appWidgets *mywidgets = (appWidgets *)client_data;

  name = XtName(widget);
  delta = strcmp(name, "grow") ? -10 : 10;
  position_button(mywidgets->buttons[1], delta, 0);
  position_button(mywidgets->buttons[2], 0, delta);
  position_button(mywidgets->buttons[3], delta, delta);
  position_text(mywidgets->bb, mywidgets->st);
}

void
position_button(Widget button, 
		Position xdelta, 
		Position ydelta)
{
  Position x, y;

  XtVaGetValues(button,
		XtNx, &x,
		XtNy, &y,
		NULL);
  XtVaSetValues(button,
		XtNx, x + xdelta,
		XtNy, y + ydelta,
		NULL);
}

void
position_text(
  Widget parent, 
  Widget statictext)
{
  Dimension bbwidth, bbheight;
  Dimension stwidth, stheight;

  XtVaGetValues(statictext,
		XtNwidth, &stwidth,
		XtNheight, &stheight,
		NULL);
  XtUnmanageChild(statictext);
  XtVaGetValues(parent,
		XtNwidth,  &bbwidth,
		XtNheight, &bbheight,
		NULL);
  XtManageChild(statictext);
  XtVaSetValues(statictext,
		XtNx, (bbwidth/2)  - (stwidth/2),
		XtNy, (bbheight/2) - (stheight/2),
		NULL);
}
