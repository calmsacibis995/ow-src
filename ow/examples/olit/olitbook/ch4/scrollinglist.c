/*****************************************************************************
 * scrollinglist.c: Demonstrate the ScrollingList widget.
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
#include <Xol/ScrollingL.h>
#include <Xol/ControlAre.h>
#include <Xol/OblongButt.h>
#include <dirent.h>

static OlListToken (*AddItem)(Widget, OlListToken, OlListToken, OlListItem);
static void        (*DeleteItem)(Widget, OlListToken);
static void        (*TouchItem)(Widget, OlListToken);
static void        (*UpdateView)(Widget, Boolean);
static void        (*ViewItem)(Widget, OlListToken);

typedef struct _file {
  String         filename;
  OlListToken    token;
  struct _file *next;
} file;

file *head = NULL;     /* head of the list */
OlListToken lasttoken = NULL;

static void current_callback(Widget, XtPointer, XtPointer);
static void delete_callback(Widget, XtPointer, XtPointer);
static void findcurrent_callback(Widget, XtPointer, XtPointer);
static void initList(Widget, String);

void
main(unsigned int argc, char **argv)
{
  XtAppContext app;
  Widget       toplevel, rt, sl, ca, delete, findcurrent;
  String       directory;

  OlToolkitInitialize((XtPointer)NULL);
  OlSetDefaultTextFormat(OL_MB_STR_REP);
  toplevel = XtAppInitialize(&app, "Scrollinglist", NULL, 0, &argc, 
			     argv, NULL, NULL, 0);
  /*
   * Get the command line argument, if any
   */
  directory = argv[1];
  rt = XtVaCreateManagedWidget("rt", rubberTileWidgetClass,
                               toplevel, NULL);
  /*
   * Create the ScrollingList widget
   */
  sl = XtVaCreateManagedWidget("sl", scrollingListWidgetClass, rt,
	  		       XtNviewHeight, 6,
			       XtNselectable, FALSE,
			       XtNtextFormat, OL_SB_STR_REP,
			       NULL);
  XtVaGetValues(sl,
  		XtNapplAddItem,    &AddItem,
		XtNapplTouchItem,  &TouchItem,
  		XtNapplUpdateView, &UpdateView,
  		XtNapplDeleteItem, &DeleteItem,
  		XtNapplViewItem,   &ViewItem,
		NULL);

  XtAddCallback(sl, XtNuserMakeCurrent, current_callback, NULL);
  /*
   * Create the ControlArea and Buttons
   */
  ca = XtVaCreateManagedWidget("ca", controlAreaWidgetClass,
                               rt, NULL);
  delete = XtVaCreateManagedWidget("Delete", 
                                   oblongButtonWidgetClass,
                                   ca, NULL);
  XtAddCallback(delete, XtNselect, delete_callback, sl);
  findcurrent = XtVaCreateManagedWidget("Current", 
                                        oblongButtonWidgetClass,
                                        ca, NULL);
  XtAddCallback(findcurrent, XtNselect, findcurrent_callback, sl);
  initList(sl, directory);

  XtRealizeWidget(toplevel);
  XtAppMainLoop(app);
}

void
initList(Widget sl, String directory)
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
current_callback(
  Widget    widget, 
  XtPointer client_data, 
  XtPointer call_data)
{
  OlListToken token = (OlListToken)call_data;
  OlListItem *newItem = OlListItemPointer(token);
  OlListItem *lastItem;

  if(lasttoken == token) {      /* unset current choice */
    newItem->attr &= ~OL_LIST_ATTR_CURRENT;
    (*TouchItem)(widget, token);
    lasttoken = NULL;
    return;
  }

  if(lasttoken) {
    lastItem = OlListItemPointer(lasttoken);
    if(lastItem->attr & OL_LIST_ATTR_CURRENT)
      lastItem->attr &= ~OL_LIST_ATTR_CURRENT;
    (*TouchItem)(widget, lasttoken);
  }
  newItem->attr |= OL_LIST_ATTR_CURRENT;
  (*TouchItem)(widget, token);
  lasttoken = token;
}

void
delete_callback(
  Widget    widget, 
  XtPointer client_data, 
  XtPointer call_data)
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

void
findcurrent_callback(
  Widget    widget, 
  XtPointer client_data, 
  XtPointer call_data)
{
  Widget sl = (Widget)client_data;

  if(lasttoken)
    (*ViewItem)(sl, lasttoken);
}
