/*****************************************************************************
 * dragfile.c: Demonstrate "dragging" with the DropTarget widget.
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
#include <Xol/RubberTile.h>
#include <Xol/FooterPane.h>
#include <Xol/ScrollingL.h>
#include <Xol/ControlAre.h>
#include <Xol/OblongButt.h>
#include <Xol/DropTarget.h>
#include <X11/Xatom.h>
#include <X11/Xos.h>
#include <X11/Xutil.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>

OlListToken (*AddItem)();
void        (*DeleteItem)();
void        (*TouchItem)();
void        (*UpdateView)();
void        (*ViewItem)();

typedef struct File {
  String       filename;
  OlListToken  token;
  struct File *next;
} file;

Widget      remove;
file       *head = NULL;     /* head of the list */
OlListToken lasttoken = NULL;
char       *supported[] = {"TARGETS", "FILE_NAME", "STRING", "DELETE" };
String      directory = NULL;
enum { TARGETS, FILE_NAME, STRING, DELETE };

void
main(unsigned int argc, char **argv)
{
  int          n;
  Arg          wargs[10];
  Widget       toplevel, fp, form, sl, dt, ca, findcurrent;
  void         initList(), current_callback(), delete_callback();
  void         findcurrent_callback();
  void         dropsite_callback();
  Cursor       AcceptCursor, RejectCursor;
  XtAppContext app;

  OlToolkitInitialize((XtPointer)NULL);
  OlSetDefaultTextFormat(OL_MB_STR_REP);
  toplevel = XtAppInitialize(&app, "Dragfile", NULL, 0, &argc, 
			     argv, NULL, NULL, 0);
  /*
   * Get the command line argument, if any
   */
  directory = argv[1];
  /*
   * Create the FooterPanel and Form widgets.
   */
  fp = XtCreateManagedWidget("fp", footerPanelWidgetClass,
                             toplevel, NULL, 0);
  form = XtCreateManagedWidget("form", formWidgetClass,
                               fp, NULL, 0);
  /*
   * Create the DropTarget widget.
   */
  AcceptCursor = OlGetDupeDocDropCursor(form);
  RejectCursor = OlGetDupeDocNoDropCursor(form);
  n = 0;
  XtSetArg(wargs[n], XtNdndAcceptCursor, AcceptCursor); n++;
  XtSetArg(wargs[n], XtNdndRejectCursor, RejectCursor); n++;
  dt = XtCreateManagedWidget("dt", dropTargetWidgetClass,
                             form, wargs, n);
  /*
   * Create the ScrollingList widget
   */
  n = 0;
  XtSetArg(wargs[n], XtNviewHeight, 6); n++;
  XtSetArg(wargs[n], XtNselectable, FALSE); n++;
  sl = XtCreateManagedWidget("sl", scrollingListWidgetClass,
                             form, wargs, n);
  n = 0;
  XtSetArg(wargs[n], XtNapplAddItem,    &AddItem); n++;
  XtSetArg(wargs[n], XtNapplTouchItem,  &TouchItem); n++;
  XtSetArg(wargs[n], XtNapplUpdateView, &UpdateView); n++;
  XtSetArg(wargs[n], XtNapplDeleteItem, &DeleteItem); n++;
  XtSetArg(wargs[n], XtNapplViewItem,   &ViewItem); n++;
  XtGetValues(sl, wargs, n);
  XtAddCallback(sl, XtNuserMakeCurrent, current_callback, dt);
  /*
   * Create the ControlArea and Buttons
   */
  ca = XtCreateManagedWidget("ca", controlAreaWidgetClass,
                              fp, NULL, 0);
  remove = XtCreateManagedWidget("remove", oblongButtonWidgetClass,
                                 ca, NULL, 0);
  findcurrent = XtCreateManagedWidget("current", 
                                      oblongButtonWidgetClass,
                                      ca, NULL, 0);
  initList(sl, directory);
  XtAddCallback(remove, XtNselect, delete_callback, sl);
  XtAddCallback(findcurrent, XtNselect, findcurrent_callback, sl);
  XtAddCallback(dt, XtNownSelectionCallback, dropsite_callback, NULL);
  /*
   * Realize the widgets and enter the event loop.
   */
  XtRealizeWidget(toplevel);
  XtAppMainLoop(app);
}

void
current_callback(w, client_data, call_data)
  Widget w;
  XtPointer client_data, call_data;
{
  OlListToken token = (OlListToken)call_data;
  OlListItem *newItem = OlListItemPointer(token);
  OlListItem *lastItem;
  Widget      dt = (Widget)client_data;
  Arg         wargs[1];

  if(lasttoken == token) {      /* unset current choice */
    newItem->attr &= ~OL_LIST_ATTR_CURRENT;
    (*TouchItem)(w, token);
    lasttoken = NULL;
    XtSetArg(wargs[0], XtNfull, FALSE);
    XtSetValues(dt, wargs, 1);
    return;
  }

  if(lasttoken) {
    lastItem = OlListItemPointer(lasttoken);
    if(lastItem->attr & OL_LIST_ATTR_CURRENT)
      lastItem->attr &= ~OL_LIST_ATTR_CURRENT;
    (*TouchItem)(w, lasttoken);
  }
  newItem->attr |= OL_LIST_ATTR_CURRENT;
  (*TouchItem)(w, token);
  lasttoken = token;
  XtSetArg(wargs[0], XtNfull, TRUE);
  XtSetValues(dt, wargs, 1);
}

void
dropsite_callback(w, client_data, call_data)
  Widget w;
  XtPointer client_data, call_data;
{
  void OwnSelection();
  OlDropTargetCallbackStruct *cd = 
      (OlDropTargetCallbackStruct *)call_data;

  switch (cd->reason) {
  case OL_REASON_DND_OWNSELECTION:
    OwnSelection(cd);
    break;
  default:
    break;
  }
}

void
OwnSelection(cd)
  OlDropTargetCallbackStruct *cd;
{
  Atom    atom;
  Boolean ConvertSelection();
  void    CleanupTransaction();
  Arg     wargs[1];

  atom = OlDnDAllocTransientAtom(cd->widget);
  XtSetArg(wargs[0], XtNselectionAtom, atom);
  XtSetValues(cd->widget, wargs, 1);
  OlDnDOwnSelection(cd->widget, atom, cd->time, ConvertSelection,
                    NULL, (XtSelectionDoneProc)NULL, 
                    CleanupTransaction,
                    (XtPointer)NULL);
}

Boolean
ConvertSelection(w, selection, target, type, value, length, format)
  Widget w;
  Atom *selection, *target, *type;
  XtPointer *value;
  unsigned long *length;
  int *format;
{
  XrmValue source, dest;
  file *current, *findset();
  static Atom *targets;
  String file_contents, complete_path; 
  String get_file_contents(), get_pathname();
  struct stat info;
  Boolean first = TRUE;
  int  i;
  long size;

  if(first) {
    targets = (Atom *)XtMalloc(XtNumber(supported) * sizeof(Atom));
    for(i=0;i<XtNumber(supported);i++) {
      source.size = strlen(supported[i])+1;
      source.addr = supported[i];
      dest.size = sizeof(Atom);
      dest.addr = (char *)&targets[i];
      XtConvertAndStore(w, XtRString, &source, XtRAtom, &dest);
    }
    first = FALSE;
  }
  if(*target == targets[TARGETS]) {     /* TARGETS */
    *type = XA_ATOM;
    *value = (XtPointer)targets;
    *length = XtNumber(supported);
    *format = 32;
    return(TRUE);
  }
  if(*target == targets[FILE_NAME]) {   /* FILE_NAME */
    current = findset();
    if(current == NULL)
      return(FALSE);
    complete_path = get_pathname(directory, current->filename);
    if(stat(complete_path, &info) != 0) {  /* make sure file exists */
      perror(complete_path);
      return(FALSE);
    }
    *type = targets[STRING];
    *value = (XtPointer)complete_path;
    *length = strlen(complete_path);
    *format = 8;
    return(TRUE);
  }
  if(*target == targets[STRING]) {   /* STRING */
    current = findset();
    if(current == NULL)
      return(FALSE);
    complete_path = get_pathname(directory, current->filename);
    file_contents = get_file_contents(complete_path, &size);
    if(file_contents == NULL)
      return(FALSE);
    *type = targets[STRING];
    *length = size;
    *value = (XtPointer)file_contents;
    *format = 8;
    return(TRUE);
  }
  if(*target == targets[DELETE]) {   /* DELETE */
    /*
     * Remove the item from the list
     */
    XtCallCallbacks(remove, XtNselect, NULL);
    *type = targets[DELETE];
    *length = NULL;
    *value = (XtPointer)NULL;
    *format = 8;
    return(TRUE);
  }
  return(FALSE);
}

file *
findset()
{
  file       *filep;
  OlListItem *fileItem;

  filep = head;
  while(filep) {
    fileItem = OlListItemPointer(filep->token);
    if(fileItem->attr & OL_LIST_ATTR_CURRENT)
      return(filep);
    else
      filep = filep->next;
  }
  return((file *)NULL);
}

String
get_pathname(directory, filename)
  String directory, filename;
{
  static char complete_path[1024];      /* Long enough, hopefully */

  if(directory)
    sprintf(complete_path, "%s/%s", directory, filename);
  else
    sprintf(complete_path, "%s", filename);
  return(complete_path);
}

String
get_file_contents(path, size)
  String path;
  long *size;
{
  int retval;
  struct stat info;
  String file_contents;
  int fd;

  retval = stat(path, &info);
  if(retval != 0) {
    perror(path);
    return(NULL);
  }
  
  /* 
   * Since we don't have a done_proc the Intrinsics will 
   * free this storage 
   */
  file_contents = (String)XtMalloc(info.st_size);
  if((fd = open(path, O_RDONLY)) < 0) {
    perror(path);
    return(NULL);
  }
  retval = read(fd, file_contents, info.st_size);
  if(retval != info.st_size) {
    perror(path);
    return(NULL);
  }
  *size = (long)info.st_size;
  return(file_contents);
}

void
CleanupTransaction(w, selection, state, timestamp, closure)
  Widget                  w;
  Atom                    selection;
  OlDnDTransactionState   state;
  Time                    timestamp;
  XtPointer               closure;
{
  switch (state) {
    case OlDnDTransactionDone:
    case OlDnDTransactionRequestorError:
    case OlDnDTransactionRequestorWindowDeath:
      OlDnDFreeTransientAtom(w, selection);
      OlDnDDisownSelection(w, selection, 
                           XtLastTimestampProcessed(XtDisplay(w)));
      break;
    case OlDnDTransactionBegins:
    case OlDnDTransactionEnds:
      break;
  }
}

void
initList(sl, directory)
  Widget sl;
  String directory;
{
  OlListItem     item;
  short          count = 0;
  DIR           *dirp;
  struct dirent *dp;
  file          *filep, *prevp;

  if(directory == NULL)
    directory = ".";
  dirp = opendir(directory);
  if(dirp == NULL) {
    perror(directory);
    exit(-1);
  }
  (*UpdateView)(sl, FALSE);
  for (dp = readdir(dirp); dp != NULL; dp = readdir(dirp)) {
    filep = (file *)XtCalloc(1, sizeof(file));
    if(head == NULL)
      head = prevp = filep;
    else {
      prevp->next = filep;
      prevp = filep;
    }
    filep->next = NULL;
    item.label_type = OL_STRING;
    item.attr = count;
    item.label = filep->filename = XtNewString(dp->d_name);
    item.mnemonic = NULL;
    filep->token = (*AddItem)(sl, 0, 0, item);
    count++;
  }
  closedir(dirp);
  (*UpdateView)(sl, TRUE);
}

void
findcurrent_callback(w, client_data, call_data)
  Widget w;
  XtPointer client_data, call_data;
{
  Widget sl = (Widget)client_data;

  if(lasttoken)
    (*ViewItem)(sl, lasttoken);
}

void
delete_callback(w, client_data, call_data)
  Widget    w;
  XtPointer client_data, call_data;
{
  Widget      sl = (Widget)client_data;
  file       *filep, *prevp;
  OlListItem *fileItem;

  filep = prevp = head;
  while(filep) {
    fileItem = OlListItemPointer(filep->token);
    if(fileItem->attr & OL_LIST_ATTR_CURRENT) {
      (*DeleteItem)(sl, filep->token);
      if(filep == head) {
        head = filep->next;
        XtFree((char *)filep);
        filep = prevp = head;
      } else {
        prevp->next = filep->next;
        XtFree((char *)filep);
        filep = prevp->next;
      }
    } else {
      prevp = filep;
      filep = filep->next;
    }
  }
  lasttoken = NULL;
}
