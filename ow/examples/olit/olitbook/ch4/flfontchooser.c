/*****************************************************************************
 * flfontchooser.c: Demonstrate the FlatExclusives and 
 *                  FlatNonexclusives widgets.
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
#include <Xol/FExclusive.h>
#include <Xol/FNonexclus.h>
#include <Xol/Caption.h>
#include <Xol/RectButton.h>
#include <Xol/StaticText.h>
#include <stdio.h>
#include "flfontchooser.h"

typedef struct {
	String font[3];		/* Single Byte */
	String color[6];	/* Single Byte */
	OlStr  colorname[6];	/* Multi Byte */
} ApplicationData, *ApplicationDataPtr;

ApplicationData data;

#define Offset(field) XtOffsetOf(ApplicationData, field)

static XtResource resources[] = {
{ "font1", "Font1", XtRString, sizeof(String),
	    Offset(font[0]), XtRString, "times" },
{ "font2", "Font2", XtRString, sizeof(String),
	    Offset(font[1]), XtRString, "rockwell" },
{ "font3", "Font3", XtRString, sizeof(String),
	    Offset(font[2]), XtRString, "palatino" },
{ "color1", "Color1", XtRString, sizeof(String),
	    Offset(color[0]), XtRString, "cyan" },
{ "color2", "Color2", XtRString, sizeof(String),
	    Offset(color[1]), XtRString, "azure" },
{ "color3", "Color3", XtRString, sizeof(String),
	    Offset(color[2]), XtRString, "coral" },
{ "color4", "Color4", XtRString, sizeof(String),
	    Offset(color[3]), XtRString, "plum" },
{ "color5", "Color5", XtRString, sizeof(String),
	    Offset(color[4]), XtRString, "salmon" },
{ "color6", "Color6", XtRString, sizeof(String),
	    Offset(color[5]), XtRString, "beige" },
{ "colorname1", "Colorname1", XtROlStr, sizeof(OlStr),
	    Offset(colorname[0]), XtRString, "Cyan" },
{ "colorname2", "Colorname2", XtROlStr, sizeof(OlStr),
	    Offset(colorname[1]), XtRString, "Azure" },
{ "colorname3", "Colorname3", XtROlStr, sizeof(OlStr),
	    Offset(colorname[2]), XtRString, "Coral" },
{ "colorname4", "Colorname4", XtROlStr, sizeof(OlStr),
	    Offset(colorname[3]), XtRString, "Plum" },
{ "colorname5", "Colorname5", XtROlStr, sizeof(OlStr),
	    Offset(colorname[4]), XtRString, "Salmon" },
{ "colorname6", "Colorname6", XtROlStr, sizeof(OlStr),
	    Offset(colorname[5]), XtRString, "Beige" },
};

#undef Offset

static Widget       st, font, style, fontcolor, bgcolor, scale;
static fontchoice  *fc;
static void         font_callback(Widget, XtPointer, XtPointer);
static void         scale_callback(Widget, XtPointer, XtPointer);
static void         fontcolor_callback(Widget, XtPointer, XtPointer);
static void         bgcolor_callback(Widget, XtPointer, XtPointer);
static void         style_callback(Widget, XtPointer, XtPointer);
static Widget       create_fl_ex_nonex(Widget, FlatItems *, OlStr *,
					String *, int, String, 
					WidgetClass, void (*)());
static void	    default_choice(fontchoice *);
static void         set_font(Widget);
static Cursor       BusyCursor;
static String       fallback_resources[] = {
	"Fontchooser*font1.label: times",
	"Fontchooser*font2.label: new century schoolbook",
	"Fontchooser*font3.label: palatino",
	"Fontchooser*st.borderWidget: 4",
	NULL };

void
main(unsigned int argc, char **argv)
{
  XtAppContext    app;
  Widget          toplevel, con;

  /*
   * Initialize the fontchoice structure
   */
  fc      = (fontchoice *)XtMalloc(sizeof(fontchoice));
  OlToolkitInitialize((XtPointer)NULL);
  OlSetDefaultTextFormat(OL_MB_STR_REP);
  toplevel = XtAppInitialize(&app, "Flfontchooser", NULL, 0, &argc, 
			     argv, fallback_resources, NULL, 0);
  XtGetApplicationResources(toplevel, &data, resources,
			    XtNumber(resources), NULL, 0);
  con = XtVaCreateManagedWidget("control", 
                                controlAreaWidgetClass,
                                toplevel, 
                                XtNalignCaptions, TRUE,
                                XtNlayoutType,    OL_FIXEDCOLS,
                                XtNmeasure,       1,
                                NULL);
  BusyCursor = OlGetBusyCursor(toplevel);
  /*
   * Create FlatExclusives and FlatNonexclusives widgets
   */
  font      = create_fl_ex_nonex(con, fontItems, data.font, 
                NULL, XtNumber(data.font), "font", 
                flatExclusivesWidgetClass, font_callback);

  style     = create_fl_ex_nonex(con, styleItems, styles, 
                NULL, XtNumber(styles), "style", 
                flatNonexclusivesWidgetClass, style_callback);

  fontcolor = create_fl_ex_nonex(con, fontColorItems, 
                data.colorname, data.color, XtNumber(data.color), 
                "fontcolor", flatExclusivesWidgetClass, 
                fontcolor_callback);

  bgcolor   = create_fl_ex_nonex(con, bgColorItems, data.colorname, 
                data.color, XtNumber(data.color), "backgroundcolor", 
                flatExclusivesWidgetClass, bgcolor_callback);

  scale     = create_fl_ex_nonex(con, scaleItems, scales,
                NULL, XtNumber(scales),
                "scale", flatExclusivesWidgetClass, 
                scale_callback);
  /*
   * Create the StaticText widget used to display the font
   */
  st = XtVaCreateManagedWidget("st", staticTextWidgetClass, con, 
			       XtNtextFormat, OL_SB_STR_REP,
                               NULL);
  XtRealizeWidget(toplevel);
  default_choice(fc);
  set_font(st);
  XtAppMainLoop(app);
}

Widget
create_fl_ex_nonex(
  Widget      parent,
  FlatItems  *flitems,
  OlStr      *names,
  String     *userData,
  int         number,
  String      label,
  WidgetClass class,
  void        (*callback)())
{
  Widget caption, flat;
  int    i;
  char   name_flat[64];

  /* Create Exclusives widgets */
  caption = XtVaCreateManagedWidget(label, captionWidgetClass, 
                       parent, NULL);
  for(i=0;i<number;i++) {
    flitems[i].label = (XtArgVal)names[i];
    flitems[i].selectProc = (XtArgVal)callback;
    if(userData)
	flitems[i].userData = (XtArgVal)userData[i];
    if(class == flatNonexclusivesWidgetClass)
      flitems[i].unselectProc = (XtArgVal)callback;
    else
      flitems[i].unselectProc = (XtArgVal)NULL;
    flitems[i].clientData = (XtArgVal)names[i];
  }
  sprintf(name_flat, "%s_flat",label);
  flat = XtVaCreateManagedWidget(name_flat, class, caption, 
                       XtNitems, flitems,
                       XtNnumItems, number,
                       XtNitemFields, FlatFields,
                       XtNnumItemFields, XtNumber(FlatFields),
                       NULL);
  return(flat);
}

void
font_callback(
  Widget    widget,
  XtPointer client_data,
  XtPointer call_data)
{
  OlStr label;
  OlFlatCallData *olfcd = (OlFlatCallData *)call_data;

  XDefineCursor(XtDisplay(widget), XtWindow(widget), BusyCursor);
  /*
   * Get the font from the button label
   */
  OlVaFlatGetValues(widget, olfcd->item_index,
		    XtNlabel, &label, 
		    NULL);
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
  OlFlatCallData *olfcd = (OlFlatCallData *)call_data;
  static Boolean italic = FALSE, bold = FALSE;

  XDefineCursor(XtDisplay(widget), XtWindow(widget), BusyCursor);
  /*
   * Get the current state of the buttons
   */
  if(olfcd->item_index == BOLDINDEX) {
    OlVaFlatGetValues(widget, BOLDINDEX, XtNset, &bold, NULL);
    if(bold)
	fc->bold = "bold";
    else
	fc->bold = "medium";
  }
  if(olfcd->item_index == ITALICINDEX) {
    OlVaFlatGetValues(widget, ITALICINDEX, XtNset, &italic, NULL);
    if(italic)
	fc->italic = "i";
    else
	fc->italic = "r";
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
  XDefineCursor(XtDisplay(widget), XtWindow(widget), BusyCursor);
  fc->scale = (String)client_data;
  set_font(st);
  XUndefineCursor(XtDisplay(widget), XtWindow(widget));
}

void
fontcolor_callback(
  Widget    widget,
  XtPointer client_data,
  XtPointer call_data)
{
  OlFlatCallData *olfcd = (OlFlatCallData *)call_data;

  fc->fontcolor = data.color[olfcd->item_index];
  set_font(st);
}

void
bgcolor_callback(
  Widget    widget,
  XtPointer client_data,
  XtPointer call_data)
{
  OlFlatCallData *olfcd = (OlFlatCallData *)call_data;

  fc->bgcolor = data.color[olfcd->item_index];
  set_font(st);
}

void
default_choice(fontchoice *fchoice)
{
  fchoice->font       = data.font[0];
  fchoice->bold       = "medium";
  fchoice->italic     = "r";
  fchoice->fontcolor  = data.color[0];
  fchoice->bgcolor    = data.color[0];
  fchoice->scale      = scales[0];
}

void
set_font(Widget widget)
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
  XtVaSetValues(widget,
                XtNstring, fontname,
                XtVaTypedArg, XtNfont, XtRString,
                fontname, strlen(fontname)+1,
                XtVaTypedArg, XtNfontColor, XtRString,
                fc->fontcolor, strlen(fc->fontcolor)+1,
                XtVaTypedArg, XtNbackground, XtRString,
                fc->bgcolor, strlen(fc->bgcolor)+1,
                NULL);
}
