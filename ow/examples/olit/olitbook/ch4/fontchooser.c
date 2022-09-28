/*****************************************************************************
 * fontchooser.c: Demonstrate the Exclusives and 
 *                Nonexclusives widgets.
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
#include <Xol/ControlAre.h>
#include <Xol/Exclusives.h>
#include <Xol/Nonexclusi.h>
#include <Xol/Caption.h>
#include <Xol/RectButton.h>
#include <Xol/StaticText.h>
#include <Xol/OlCursors.h>
#include <stdio.h>
#include "fontchooser.h"

static Widget       st;
static Widget       font_rb[3], style_rb[2], fc_rb[6], bg_rb[6], scale_rb[6];
static fontchoice  *fc;
static void         font_callback(Widget, XtPointer, XtPointer);
static void         scale_callback(Widget, XtPointer, XtPointer);
static void         fontcolor_callback(Widget, XtPointer, XtPointer);
static void         bgcolor_callback(Widget, XtPointer, XtPointer);
static void         style_callback(Widget, XtPointer, XtPointer);
static Widget       create_ex_nonex(Widget, WidgetList, String *, int,
		                    String, WidgetClass, void (*)());
static void	    default_choice(fontchoice *);
static void         set_font(Widget);
static Boolean      str_to_pointer(Display *, XrmValuePtr, Cardinal *,
			           XrmValuePtr, XrmValuePtr, XtPointer *);
static Cursor       BusyCursor;
String fallback_resources[] = {
	"Fontchooser*font1.label: times",
	"Fontchooser*font2.label: new century schoolbook",
	"Fontchooser*font3.label: palatino",
	"Fontchooser*st.borderWidget: 4",
	NULL };

void
main(unsigned int argc, char **argv)
{
  XtAppContext app;
  Widget       toplevel, con;
  Widget       font, style, fontcolor, bgcolor, scale;

  /*
   *   Add the string to XtPointer type-converter.
   */
  XtSetTypeConverter(XtRString, XtRPointer, str_to_pointer,
                     (XtConvertArgList)NULL, 0,
                     XtCacheAll, (XtDestructor)NULL);
  /*
   * Initialize the fontchoice structure
   */
  fc      = (fontchoice *)XtMalloc(sizeof(fontchoice));
  OlToolkitInitialize((XtPointer)NULL);
  OlSetDefaultTextFormat(OL_MB_STR_REP);
  toplevel = XtAppInitialize(&app, "Fontchooser", NULL, 0, &argc, 
			     argv, fallback_resources, NULL, 0);
  con = XtVaCreateManagedWidget("control", 
                                controlAreaWidgetClass,
                                toplevel, 
                                XtNalignCaptions, TRUE,
                                XtNlayoutType,    OL_FIXEDCOLS,
                                XtNmeasure,       1,
                                NULL);
  BusyCursor = OlGetBusyCursor(toplevel);
  /*
   * Create Exclusives and Nonexclusives widgets
   */
  font      = create_ex_nonex(con, font_rb, fonts, XtNumber(fonts), 
                  "font", exclusivesWidgetClass, 
                  font_callback);
  style     = create_ex_nonex(con, style_rb, styles, XtNumber(styles), 
                  "style", nonexclusivesWidgetClass, 
                  style_callback);
  fontcolor = create_ex_nonex(con, fc_rb, colors, XtNumber(colors), 
                  "fontcolor", exclusivesWidgetClass, 
                  fontcolor_callback);
  bgcolor   = create_ex_nonex(con, bg_rb, colors, XtNumber(colors),
                  "backgroundcolor", exclusivesWidgetClass, 
                  bgcolor_callback);
  scale     = create_ex_nonex(con, scale_rb, scales, XtNumber(scales),
                  "scale", exclusivesWidgetClass, 
                  scale_callback);
  /*
   * Create the StaticText widget used to display the font
   */
  st = XtVaCreateManagedWidget("st", staticTextWidgetClass, 
                               con, 
			       XtNtextFormat, OL_SB_STR_REP,
                               NULL);
  XtRealizeWidget(toplevel);
  default_choice(fc);
  set_font(st);
  XtAppMainLoop(app);
}

Widget
create_ex_nonex(
  Widget      parent,
  WidgetList  buttons,
  String     *names,
  int         number,
  String      label,
  WidgetClass class,
  void        (*callback)())
{
  Widget caption, ex_nonex;
  int    i;
  char   name_ex[64];

  /* Create Exclusives or Nonexclusives widgets */
  caption = XtVaCreateManagedWidget(label, captionWidgetClass, 
                           parent, 
                           NULL);
  sprintf(name_ex, "%s_ex",label);
  ex_nonex = XtVaCreateManagedWidget(name_ex, class,
                           caption, NULL);
  for(i=0;i<number;i++) {
    buttons[i] = XtVaCreateManagedWidget(names[i], 
                           rectButtonWidgetClass,
                           ex_nonex, NULL);
    if(callback != NULL)
      XtAddCallback(buttons[i], XtNselect, callback, names[i]);
    if(class == nonexclusivesWidgetClass)
      XtAddCallback(buttons[i], XtNunselect, callback, 
                    names[i]);
  }
  return(ex_nonex);
}

void
font_callback(
  Widget    widget,
  XtPointer client_data,
  XtPointer call_data)
{
  String label;

  XDefineCursor(XtDisplay(widget), XtWindow(widget), BusyCursor);
  XtVaGetValues(widget, XtNlabel, &label, NULL);
  fc->font = label;
  set_font(st);
  XUndefineCursor(XtDisplay(widget), XtWindow(widget));
}

void
style_callback(
  Widget    widget,
  XtPointer client_data,
  XtPointer call_data)
{
  Boolean set;

  XDefineCursor(XtDisplay(widget), XtWindow(widget), BusyCursor);
  /*
   * Get the current state of the buttons
   */
  if(strcmp(client_data, "italic") == NULL) {
    XtVaGetValues(widget, XtNset, &set, NULL);
    if(set)
	fc->italic = "i";
    else
	fc->italic = "r";
 }
  if(strcmp(client_data, "bold") == NULL) {
    XtVaGetValues(widget, XtNset, &set, NULL);
    if(set)
	fc->bold = "bold";
    else
	fc->bold = "medium";
  }
  set_font(st);
  XUndefineCursor(XtDisplay(widget), XtWindow(widget));
}

void
scale_callback(
  Widget    widget,
  XtPointer client_data,
  XtPointer call_data)
{
  String label;

  XDefineCursor(XtDisplay(widget), XtWindow(widget), BusyCursor);
  XtVaGetValues(widget, XtNlabel, &label, NULL);
  fc->scale = label;
  set_font(st);
  XUndefineCursor(XtDisplay(widget), XtWindow(widget));
}

void
fontcolor_callback(
  Widget    widget,
  XtPointer client_data,
  XtPointer call_data)
{
  String color = (String)client_data;
  XtPointer user_data;

  XtVaGetValues(widget, 
		XtNuserData, &user_data,
		NULL);
  /*
   * If the userData resource is NULL, set default
   * to the value passed in client_data
   */
  if(user_data == NULL)
    fc->fontcolor = color;
  else 
    fc->fontcolor = user_data;
  set_font(st);
}

void
bgcolor_callback(
  Widget    widget,
  XtPointer client_data,
  XtPointer call_data)
{
  String color = (String)client_data;
  XtPointer user_data;

  XtVaGetValues(widget, 
		XtNuserData, &user_data,
		NULL);
  /*
   * If the userData resource is NULL, set default
   * to the value passed in cliet_data
   */

  if(user_data == NULL)
    fc->bgcolor = color;
  else 
    fc->bgcolor = user_data;
  set_font(st);
}

void
default_choice(
  fontchoice *fchoice)
{
  String str;
  XtPointer user_data;

  XtVaGetValues(font_rb[0], XtNlabel, &str, NULL);
  fchoice->font           = str;
  fchoice->bold           = "medium";
  fchoice->italic         = "r";
  XtVaGetValues(fc_rb[0], XtNuserData, &user_data, NULL);
  if(user_data == NULL)
    fchoice->fontcolor    = colors[0];
  else
    fchoice->fontcolor    = (String)user_data;
  XtVaGetValues(bg_rb[0], XtNuserData, &user_data, NULL);
  if(user_data == NULL)
    fchoice->bgcolor    = (String)colors[0];
  else
    fchoice->bgcolor    = (String)user_data;
  fchoice->scale          = scales[0];
}

void
set_font(
  Widget w)
{
  char   fontname[128];

  /*
   * Create the string for the font name
   */
  sprintf(fontname, "-%s-%s-%s-%s-%s-%s-%s-%s-%s-%s-%s-%s-%s-%s",
		"*", 		/* Foundry */
		fc->font,	/* Family */
		fc->bold, 	/* Weight */
		fc->italic,	/* Slant */
		"*", 		/* Set Width */
		"*", 		/* Add Style */
		fc->scale,	/* Pixel Size */
		"*", 		/* Point Size */
		"*", 		/* Resolution X */
		"*", 		/* Resolution Y */
		"*", 		/* Spacing */
		"*", 		/* Average Width */
		"*", 		/* Registry */
		"*" 		/* Encoding */
                );
  /*
   * Set the values from the fontchoice structure
   */
  XtVaSetValues(w,
                XtNstring, fontname,
                XtVaTypedArg, XtNfont, XtRString,
                fontname, strlen(fontname)+1,
                XtVaTypedArg, XtNfontColor, XtRString,
                fc->fontcolor, strlen(fc->fontcolor)+1,
                XtVaTypedArg, XtNbackground, XtRString,
                fc->bgcolor, strlen(fc->bgcolor)+1,
                NULL);

}

Boolean
str_to_pointer(
  Display     *dpy,
  XrmValuePtr  args,
  Cardinal    *nargs,
  XrmValuePtr  fromVal,
  XrmValuePtr  toVal,
  XtPointer   *data)
{
  /*
   * Make sure the number of args is correct.
   */
  if (*nargs != 0)
    XtWarning("String to verlevel conversion needs no args");
  /*
   * Convert the string in the fromVal to an XtPointer
   * Just cast it to XtPointer
   *
   * Make the toVal point to the result.
   */
  toVal->size = sizeof (XtPointer); 
  *(XtPointer *)toVal->addr = fromVal->addr; 
  return TRUE;
}
