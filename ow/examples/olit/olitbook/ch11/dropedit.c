/*****************************************************************************
 * dropedit.c: Demonstrate "dropping" with the  DropTarget widget.
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
#include <X11/Shell.h>
#include <Xol/OpenLook.h>
#include <Xol/TextEdit.h>
#include <Xol/Form.h>
#include <Xol/DropTarget.h>
#include <Xol/CheckBox.h>
#include <Xol/ScrolledWi.h>
#include <stdio.h>

typedef struct {
  Boolean               send_done;
  Boolean               delete;
  Widget                toplevel;
  Widget                te;
  OlDnDTriggerOperation operation;
} SelectionClientData;

static Atom TARGETS, STRING, FILE_NAME, DELETE;

void
main(unsigned int argc, char **argv)
{
  Widget              toplevel, form, te, sw, dt;
  Arg                 wargs[10];
  int                 n;
  void                dropsite_callback();
  Boolean             delete;
  SelectionClientData scd;
  XtAppContext        app;

  n = 0;
  OlToolkitInitialize((XtPointer)NULL);
  OlSetDefaultTextFormat(OL_MB_STR_REP);
  toplevel = XtAppInitialize(&app, "Dropedit", NULL, 0, &argc, 
			     argv, NULL, NULL, 0);
  /*
   * Create the Form widget.
   */
  form = XtCreateManagedWidget("form", formWidgetClass,
                               toplevel, wargs, n);
  /*
   * Create the DropTarget widget.
   */
  n = 0;
  XtSetArg(wargs[n], XtNdndPreviewHints, 
           OlDnDSitePreviewDefaultSite); n++;
  dt = XtCreateManagedWidget("dt", dropTargetWidgetClass,
                             form, wargs, n);
  /*
   * Create ScrolledWindow and TextEdit widgets.
   */
  n = 0;
  sw = XtCreateManagedWidget("sw", scrolledWindowWidgetClass,
                             form, wargs, n);
  XtSetArg(wargs[n], XtNwrapMode, OL_WRAP_OFF); n++;
  XtSetArg(wargs[n], XtNsourceType, OL_DISK_SOURCE); n++;
  te = XtCreateManagedWidget("te", textEditWidgetClass,
                             sw, wargs, n);
  scd.send_done = FALSE;
  scd.te        = te;
  scd.toplevel  = toplevel;
  scd.delete    = FALSE;
  /*
   * Intern the required atoms
   */
  STRING    = OlInternAtom(XtDisplay(toplevel), "STRING");
  FILE_NAME = OlInternAtom(XtDisplay(toplevel), "FILE_NAME");
  DELETE    = OlInternAtom(XtDisplay(toplevel), "DELETE");
  TARGETS   = OlInternAtom(XtDisplay(toplevel), "TARGETS");
  /*
   * This is required to make the drop site "droppable"
   */
  XtAddCallback(dt, XtNdndTriggerCallback, dropsite_callback, &scd);
  /*
   * Realize the widgets and enter the event loop.
   */
  XtRealizeWidget(toplevel);
  XtAppMainLoop(app);
}

void
dropsite_callback(w, client_data, call_data)
  Widget w;
  XtPointer client_data, call_data;
{
  void TriggerNotify();
  SelectionClientData *scd = (SelectionClientData *)client_data;
  OlDropTargetCallbackStruct *cd = 
      (OlDropTargetCallbackStruct *)call_data;

  switch (cd->reason) {
  case OL_REASON_DND_TRIGGER:
    scd->send_done = cd->send_done;
    scd->operation = cd->operation;
    TriggerNotify(cd, scd);
    break;
  default:
    break;
  }
}

void
TriggerNotify(cd, scd)
  OlDropTargetCallbackStruct *cd;
  SelectionClientData *scd;
{
  void get_targets();

  XtGetSelectionValue(cd->widget, cd->selection, TARGETS, 
                      get_targets, scd, cd->time);
}

void
get_targets(wid, client_data, selection, type, value, length, format)
  Widget wid;
  XtPointer client_data;
  Atom *selection;
  Atom *type;
  XtPointer value;
  unsigned long *length;
  int *format;
{
  Atom *supported_atoms = (Atom *)value;
  int i;
  void get_file_name(), get_string();
  Boolean string_target, file_name_target, delete_target;
  SelectionClientData *scd = (SelectionClientData *)client_data;
 
  string_target = file_name_target = delete_target = FALSE;
  for(i=0;i<*length;i++) {
    if(FILE_NAME == supported_atoms[i])
      file_name_target = TRUE;
    else if(STRING == supported_atoms[i])
      string_target = TRUE;
    else if(DELETE == supported_atoms[i])
      delete_target = TRUE;
  }
  scd->delete = delete_target;
  /*
   * Prefer to get the FILE_NAME target if available.
   * If not available get STRING target.
   */
  if(file_name_target) {
    XtGetSelectionValue(wid, *selection, FILE_NAME, get_file_name,
                        scd, XtLastTimestampProcessed(XtDisplay(wid)));
  } else if(string_target) {
    XtGetSelectionValue(wid, *selection, STRING, get_string,
                        scd, XtLastTimestampProcessed(XtDisplay(wid)));
  } else
    OlWarning("source does not support STRING or FILE_NAME\n");
}

void
get_file_name(wid, client_data, selection, type, value, length, format)
  Widget wid;
  XtPointer client_data;
  Atom *selection;
  Atom *type;
  XtPointer value;
  unsigned long * length;
  int *format;
{
  Arg wargs[4];
  int n;
  void handle_done();
  SelectionClientData *scd = (SelectionClientData *)client_data;

  if(*length) {
    /*
     * The source may elect to specify that the filename they
     * are returning is of type STRING, so check of FILE_NAME
     * and STRING
     */
    if(*type == FILE_NAME || *type == STRING) {
      /*
       * Set the cursor and display positions to 0 before loading file
       */
      n = 0;
      XtSetArg(wargs[n], XtNcursorPosition, 0); n++;
      XtSetArg(wargs[n], XtNdisplayPosition, 0); n++;
      XtSetValues(scd->te, wargs, n);
      n = 0;
      XtSetArg(wargs[n], XtNsource, value); n++;
      XtSetValues(scd->te, wargs, n);
      n = 0;
      XtSetArg(wargs[n], XtNtitle, value); n++;
      XtSetValues(scd->toplevel, wargs, n);
    } else {
        OlWarning("get_data: not FILE_NAME or STRING\n");
    }
  }
  handle_done(wid, selection, scd);
}

void
handle_done(wid, selection, scd)
  Widget                wid;
  Atom                *selection;
  SelectionClientData *scd;
{
  void DoneCallback();

  /*
   * Only send the DELETE is it was a move operation and the
   * owner of the selection supports DELETE
   */
  if(scd->delete && (scd->operation == OlDnDTriggerMoveOp))
    XtGetSelectionValue(wid, *selection, DELETE, DoneCallback,
                        scd, XtLastTimestampProcessed(XtDisplay(wid)));
  else if(scd->send_done)
    OlDnDDragNDropDone(wid, *selection, 
                       XtLastTimestampProcessed(XtDisplay(wid)),
                       NULL, NULL);
}

void
get_string(wid, client_data, selection, type, value, length, format)
  Widget wid;
  XtPointer client_data;
  Atom *selection;
  Atom *type;
  XtPointer value;
  unsigned long * length;
  int *format;
{
  Arg wargs[4];
  void handle_done();
  int n;
  SelectionClientData *scd = (SelectionClientData *)client_data;

  if(*length) {
    if(*type == STRING) {
      OlTextEditInsert((TextEditWidget)scd->te, value, (int)*length);
    } else {
      OlWarning("unsupported type returned by source\n");
    }
  }
  handle_done(wid, selection, scd);
}

void
DoneCallback(w, client_data, selection, type, value, length, format)
  Widget          w;
  XtPointer       client_data;
  Atom            *selection;
  Atom            *type;
  XtPointer       value;
  unsigned long   *length;
  int             *format;
{
  SelectionClientData *scd = (SelectionClientData *)client_data;

  if(scd->send_done)
    OlDnDDragNDropDone(w, *selection,
                       XtLastTimestampProcessed(XtDisplay(w)),
                       NULL, NULL);
}
