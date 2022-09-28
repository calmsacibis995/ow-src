/*****************************************************************************
 * popupwindow.c: Demonstrate the PopupWindow widget.
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
#include <Xol/Form.h>
#include <Xol/TextEdit.h>
#include <Xol/TextField.h>
#include <Xol/StaticText.h>
#include <Xol/OblongButt.h>
#include <Xol/RubberTile.h>
#include <Xol/ScrolledWi.h>
#include <Xol/PopupWindo.h>
#include <Xol/Caption.h>
#include <Xol/FExclusive.h>
#include <Xol/FNonexclus.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <stdio.h>
#include "flfontchooser.h"

extern char *sys_errlist[];
extern int errno;

typedef struct Popupdata {
  Widget tf;    /* TextField */
  Widget te;    /* TextEdit */
  Widget st;    /* StaticText */
} popupdata;

typedef struct _choiceindex {
  int font, fontcolor, bgcolor, scale;
  Boolean styleb, stylei;
} choiceindex;

static Widget font, style, fontcolor, bgcolor, scale;
static Boolean popdown = TRUE;
static fontchoice *fc, *current;
static popupdata pd;
static choiceindex lastindex, currentindex;

main(argc, argv)
  int argc;
  char *argv[];
{
  Widget       toplevel, file, prop, ca, rt, sw,
               loadpopup, proppopup;
  Widget       CreateLoadPopup(), CreatePropPopup();
  void         popup_callback(), set_font(), default_choice();
  XtAppContext app;

  /*
   * Initialize the fontchoice structure
   */
  fc      = (fontchoice *)XtMalloc(sizeof(fontchoice));
  current = (fontchoice *)XtMalloc(sizeof(fontchoice));
  default_choice(fc);
  default_choice(current);
  OlToolkitInitialize((XtPointer)NULL);
  toplevel = XtAppInitialize(&app, "Popupwindow", (XrmOptionDescList)NULL,
                             0, (Cardinal *)&argc, argv, NULL, 
                             (ArgList) NULL, 0);
  /*
   * Create the RubberTile widget.
   */
  rt = XtVaCreateManagedWidget("rt", rubberTileWidgetClass,
                               toplevel, NULL);
  /*
   * Create a ControlArea widget to hold the control buttons
   */
  ca = XtVaCreateManagedWidget("ca", controlAreaWidgetClass, 
                               rt, NULL);
  file = XtVaCreateManagedWidget("file", 
                                 oblongButtonWidgetClass, ca,
                                 XtNlabelType, OL_POPUP,
                                 NULL);
  prop = XtVaCreateManagedWidget("prop", 
                                 oblongButtonWidgetClass, ca,
                                 XtNlabelType, OL_POPUP,
                                 NULL);
  /*
   * Create ScrolledWindow and TextEdit widgets.
   */
  sw = XtVaCreateManagedWidget("sw", scrolledWindowWidgetClass,
                               rt, NULL);
  pd.te = XtVaCreateManagedWidget("te", textEditWidgetClass, 
                                 sw,
                                 XtNwrapMode, OL_WRAP_OFF,
                                 XtNsourceType, OL_DISK_SOURCE,
                                 NULL);
  loadpopup = CreateLoadPopup(file);
  proppopup = CreatePropPopup(prop);
  set_font(pd.te);
  XtAddCallback(file, XtNselect, popup_callback, loadpopup);
  XtAddCallback(prop, XtNselect, popup_callback, proppopup);

  XtRealizeWidget(toplevel);
  XtAppMainLoop(app);
}

void
popup_callback(w, client_data, call_data)
  Widget w;
  XtPointer client_data, call_data;
{
  Widget popup = (Widget)client_data;
  XtPopup(popup, XtGrabNone);
}

void
verify_callback(w, client_data, call_data)
  Widget w;
  XtPointer client_data, call_data;
{
  Boolean *popdownmenu = (Boolean *)call_data;

  /*
   * Determine if the pop-up should be popped down
   */
  if(popdown == FALSE)
    *popdownmenu = FALSE;
}

Widget
CreateLoadPopup(parent)
  Widget parent;
{
  Widget popup, upper, lower, footer, caption, tf, load, st;
  void load_callback(), verify_callback();
  static XtCallbackRec verify[] = {
      { (XtCallbackProc)verify_callback, (XtPointer)NULL },
      { NULL, NULL },
  };


  popup = XtVaCreatePopupShell("loadpopup", 
                popupWindowShellWidgetClass,
                parent,
                XtNverify, verify,
                XtNtitle, "Load File",
                NULL);
  XtVaGetValues(popup,
                XtNupperControlArea, &upper,
                XtNlowerControlArea, &lower,
                XtNfooterPanel,      &footer,
                NULL);
  caption = XtVaCreateManagedWidget("popupcaption", 
                captionWidgetClass,
                upper, NULL);
  pd.tf = XtVaCreateManagedWidget("popuptf", 
                textFieldWidgetClass, caption,
                XtNwidth, 400,
                NULL);
  load = XtVaCreateManagedWidget("popupload", 
                oblongButtonWidgetClass,
                lower,
                XtNlabel, "Load",
                NULL);
  pd.st = XtVaCreateManagedWidget("popupst", 
                staticTextWidgetClass, footer,
                XtNstring, "",
                XtNgravity, WestGravity,
                XtNwrap, FALSE,
                NULL);
  XtAddCallback(load, XtNselect, load_callback, (XtPointer)&pd);
  return(popup);
}

void
load_callback(w, client_data, call_data)
  Widget w;
  XtPointer client_data, call_data;
{
  popupdata *pd = (popupdata *)client_data;
  String filename;
  struct stat info;
  int size, retval;
  char errormsg[256];

  /*
   * read the filename
   */
  filename = OlTextFieldGetString(pd->tf, &size);
  retval = stat(filename, &info);
  if(retval == 0) {
    XtVaSetValues(pd->te, XtNsource, filename, NULL);
    XtVaSetValues(pd->st, XtNstring, "", NULL);
    popdown = TRUE;
  } else {
    /*
     * Put an error message in the footer
     */
    sprintf(errormsg, "%s: %s", filename, sys_errlist[errno]);
    XtVaSetValues(pd->st, XtNstring, errormsg, NULL);
    popdown = FALSE;
  }
}

Widget
CreatePropPopup(parent)
  Widget parent;
{
  Widget  popup, upper, lower, footer;
  void    load_callback(), verify_callback(), apply_callback(), 
          reset_callback(), resetFactory_callback(), 
          set_button_defaults(), copy_index();
  Widget  create_fl_ex_nonex();
  void    font_callback(), style_callback(), fontcolor_callback(), 
          bgcolor_callback(), scale_callback();

  static XtCallbackRec verify[] = {
      { (XtCallbackProc)verify_callback, (XtPointer)NULL },
      { NULL, NULL },
  };
  static XtCallbackRec apply[] = {
      { (XtCallbackProc)apply_callback, (XtPointer)NULL },
      { NULL, NULL },
  };
  static XtCallbackRec reset[] = {
      { (XtCallbackProc)reset_callback, (XtPointer)NULL },
      { NULL, NULL },
  };
  static XtCallbackRec resetFactory[] = {
      { (XtCallbackProc)resetFactory_callback, (XtPointer)NULL },
      { NULL, NULL },
  };

  popup = XtVaCreatePopupShell("proppopup", 
                popupWindowShellWidgetClass,
                parent,
                XtNapply, apply,
                XtNreset, reset,
                XtNresetFactory, resetFactory,
                XtNverify, verify,
                XtNtitle, "Properties",
                NULL);
  XtVaGetValues(popup,
                XtNupperControlArea, &upper,
                XtNlowerControlArea, &lower,
                XtNfooterPanel,      &footer,
                NULL);
  /*
   * Create FlatExclusives and FlatNonexclusives widgets
   */
  font      = create_fl_ex_nonex(upper, fontItems, fonts,
                XtNumber(fonts), "Font:", 
                flatExclusivesWidgetClass,
                font_callback);
  style     = create_fl_ex_nonex(upper, styleItems, styles,
                XtNumber(styles), "Style:",
                flatNonexclusivesWidgetClass,
                style_callback);
  fontcolor = create_fl_ex_nonex(upper, fontColorItems, colors,
                XtNumber(colors), "Font Color:",
                flatExclusivesWidgetClass, fontcolor_callback);
  bgcolor   = create_fl_ex_nonex(upper, bgColorItems, colors,
                XtNumber(colors), "Background Color:",
                flatExclusivesWidgetClass, bgcolor_callback);
  scale     = create_fl_ex_nonex(upper, scaleItems, scales,
                XtNumber(scales), "Scale:", 
                flatExclusivesWidgetClass,
                scale_callback);
  /*
   * Set up the defaults
   */
  set_button_defaults(0, FALSE, FALSE, 1, 0, 1);
  copy_index(&lastindex, &currentindex);
  return(popup);
}

void
apply_callback(w, client_data, call_data)
  Widget w;
  XtPointer client_data, call_data;
{
  void set_font(), copy_index();

  /*
   * Store current settings so Reset 
   * will know what values to use
   */
  copy_index(&lastindex, &currentindex);
  set_font(pd.te);
  popdown = TRUE;
}

void
copy_index(to, from)
  choiceindex *to, *from;
{
  to->font = from->font;
  to->styleb = from->styleb;
  to->stylei = from->stylei;
  to->fontcolor = from->fontcolor;
  to->bgcolor = from->bgcolor;
  to->scale = from->scale;
}

void
reset_callback(w, client_data, call_data)
  Widget w;
  XtPointer client_data, call_data;
{
  void set_button_defaults();

  /*
   * Set the button defaults to whatever was set
   * during the last "apply"
   */
  set_button_defaults(lastindex.font,    lastindex.styleb,
                      lastindex.stylei,  lastindex.fontcolor,
                      lastindex.bgcolor, lastindex.scale);
  copy_index(&currentindex, &lastindex);
}

void
resetFactory_callback(w, client_data, call_data)
  Widget w;
  XtPointer client_data, call_data;
{
  void set_button_defaults(), default_choice();

  set_button_defaults(0, FALSE, FALSE, 1, 0, 1);
  default_choice(fc);
  popdown = FALSE;
}

void
set_button_defaults(fontd, stylebd, styleid, fontcolord,
                    bgcolord, scaled)
  int fontd, fontcolord, bgcolord, scaled;
  Boolean stylebd, styleid;
{
  Boolean bold, italic;

  OlActivateWidget(font, OL_SELECTKEY, (XtPointer)(fontd+1));
  OlVaFlatGetValues(style, 0, XtNset, &bold, NULL);
  if(stylebd != bold)
    OlActivateWidget(style, OL_SELECTKEY, (XtPointer)(BOLDINDEX+1));
  OlVaFlatGetValues(style, 1, XtNset, &italic, NULL);
  if(styleid != italic)
    OlActivateWidget(style, OL_SELECTKEY, (XtPointer)(ITALICINDEX+1));
  OlActivateWidget(fontcolor, OL_SELECTKEY,(XtPointer)(fontcolord+1));
  OlActivateWidget(bgcolor, OL_SELECTKEY, (XtPointer)(bgcolord+1));
  OlActivateWidget(scale, OL_SELECTKEY, (XtPointer)(scaled+1));
}

void
font_callback(w, client_data, call_data)
  Widget    w;
  XtPointer client_data, call_data;
{
  OlFlatCallData *olfcd = (OlFlatCallData *)call_data;

  fc->font = (String)client_data;
  currentindex.font = olfcd->item_index;
}

void
style_callback(w, client_data, call_data)
  Widget    w;
  XtPointer client_data, call_data;
{
  OlFlatCallData *olfcd = (OlFlatCallData *)call_data;
  static Boolean italic = FALSE, bold = FALSE;

  /*
   * Get the current state of the buttons
   */
  if(olfcd->item_index == BOLDINDEX)
    OlVaFlatGetValues(w, BOLDINDEX, XtNset, &bold, NULL);
  if(olfcd->item_index == ITALICINDEX)
    OlVaFlatGetValues(w, ITALICINDEX, XtNset, &italic, NULL);
  currentindex.stylei = fc->italic = italic;
  currentindex.styleb = fc->bold = bold;
}

void
scale_callback(w, client_data, call_data)
  Widget w;
  XtPointer client_data, call_data;
{
  OlFlatCallData *olfcd = (OlFlatCallData *)call_data;

  fc->scale = (String)client_data;
  currentindex.scale = olfcd->item_index;
}

void
fontcolor_callback(w, client_data, call_data)
  Widget    w;
  XtPointer client_data, call_data;
{
  OlFlatCallData *olfcd = (OlFlatCallData *)call_data;

  fc->fontcolor = (String)client_data;
  currentindex.fontcolor = olfcd->item_index;
}

void
bgcolor_callback(w, client_data, call_data)
  Widget    w;
  XtPointer client_data, call_data;
{
  OlFlatCallData *olfcd = (OlFlatCallData *)call_data;

  fc->bgcolor = (String)client_data;
  currentindex.bgcolor = olfcd->item_index;
}

void
set_font(w)
  Widget w;
{
  char   fontname[64];

  /*
   * Create the string for the font name
   */
  sprintf(fontname, "%s%s%s%s-%s",
                fc->font,
                ((fc->bold || fc->italic) ? "-" : ""),
                (fc->bold ? "bold" : ""),
                (fc->italic ? "italic" : ""),
                fc->scale);
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

void
default_choice(fchoice)
  fontchoice *fchoice;
{
  fchoice->font       = fonts[0];
  fchoice->bold       = FALSE;
  fchoice->italic     = FALSE;
  fchoice->fontcolor  = colors[1];
  fchoice->bgcolor    = colors[0];
  fchoice->scale      = scales[1];
}

Widget
create_fl_ex_nonex(parent, flitems, names, number,
                           label, class, callback)
  Widget      parent;
  FlatItems   *flitems;
  char       *names[];
  int         number;
  String      label;
  WidgetClass class;
  void        (*callback)();
{
  Widget caption, fl_ex_nonex;
  int    i;

  /* Create Exclusives or Nonexclusives widgets */
  caption = XtVaCreateManagedWidget("", captionWidgetClass,
                     parent,
                     XtNlabel, label,
                     NULL);
  for(i=0;i<number;i++) {
    (flitems[i]).label = (XtArgVal)names[i];
    (flitems[i]).selectProc = (XtArgVal)callback;
    if(class == flatNonexclusivesWidgetClass)
      (flitems[i]).unselectProc = (XtArgVal)callback;
    else
      (flitems[i]).unselectProc = (XtArgVal)NULL;
    (flitems[i]).clientData = (XtArgVal)names[i];
  }
  fl_ex_nonex = XtVaCreateManagedWidget("", class, caption,
                     XtNitems, flitems,
                     XtNnumItems, number,
                     XtNitemFields, FlatFields,
                     XtNnumItemFields, XtNumber(FlatFields),
                     NULL);
  return(fl_ex_nonex);
}
