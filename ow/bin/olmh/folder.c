#pragma ident "@(#)folder.c	1.7 92/10/06"
/*
 *      Copyright (C) 1991  Sun Microsystems, Inc
 *                 All rights reserved.
 *       Notice of copyright on some portions of this source
 *       code product does not indicate publication.
 *
 * RESTRICTED RIGHTS LEGEND: Use, duplication, or disclosure by
 * the U.S. Government is subject to restrictions as set forth
 * in subparagraph (c)(1)(ii) of the Rights in Technical Data
 * and Computer Software Clause at DFARS 252.227-7013 (Oct. 1988)
 * and FAR 52.227-19 (c) (June 1987).
 *
 *   Sun Microsystems, Inc., 2550 Garcia Avenue,
 *   Mountain View, California 94043.
 */
/*
 *
 *
 *		       COPYRIGHT 1987, 1989
 *		   DIGITAL EQUIPMENT CORPORATION
 *		       MAYNARD, MASSACHUSETTS
 *			ALL RIGHTS RESERVED.
 *
 * THE INFORMATION IN THIS SOFTWARE IS SUBJECT TO CHANGE WITHOUT NOTICE AND
 * SHOULD NOT BE CONSTRUED AS A COMMITMENT BY DIGITAL EQUIPMENT CORPORATION.
 * DIGITAL MAKES NO REPRESENTATIONS ABOUT THE SUITABILITY OF THIS SOFTWARE FOR
 * ANY PURPOSE.  IT IS SUPPLIED "AS IS" WITHOUT EXPRESS OR IMPLIED WARRANTY.
 *
 * IF THE SOFTWARE IS MODIFIED IN A MANNER CREATING DERIVATIVE COPYRIGHT
 * RIGHTS, APPROPRIATE LEGENDS MAY BE PLACED ON THE DERIVATIVE WORK IN
 * ADDITION TO THAT SET FORTH ABOVE.
 *
 * Permission to use, copy, modify, and distribute this software and its
 * documentation for any purpose and without fee is hereby granted, provided
 * that the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of Digital Equipment Corporation not be
 * used in advertising or publicity pertaining to distribution of the software
 * without specific, written prior permission.
 */

/* folder.c -- implement buttons relating to folders and other globals. */

#include <X11/Xos.h>
#include <sys/stat.h>

#ifndef SVR4
#include <sys/dir.h>
#else
#include <sys/dirent.h>
#endif /* not SVR4 */

#include <ctype.h>
#include "xmh.h"
#include "bboxint.h"
#include "tocintrnl.h"
#ifdef NOTDEF
#include <X11/Xaw/Cardinals.h>
#endif
static void DoSelectFolder();
extern void exit();
extern void free();

typedef struct {	/* client data structure for callbacks */
Scrn	scrn;		/* the xmh scrn of action */
Toc		toc;		/* the toc of the selected folder */
Toc		original_toc;	/* the toc of the current folder */
} DeleteDataRec, *DeleteData;


void CreateFolderMenu(Widget parent, String name);
static void AddFolderMenuEntry(Button button, char *entryname);
static void DeleteFolderMenuEntry(Button button, char *foldername);

/* Close this toc&view scrn.  If this is the last toc&view, quit xmh. */

/*ARGSUSED*/
void DoClose(Widget widget, XtPointer client_data, XtPointer call_data)
{
    Scrn	scrn = (Scrn) client_data;
    register int i, count;
    Toc		toc;
    XtCallbackRec	confirm_callbacks[2];

    count = 0;
    for (i=0 ; i<numScrns ; i++)
	if (scrnList[i]->kind == STtocAndView && scrnList[i]->mapped)
	    count++;

    confirm_callbacks[0].callback = (XtCallbackProc) DoClose;
    confirm_callbacks[0].closure = (XtPointer) scrn;
    confirm_callbacks[1].callback = (XtCallbackProc) NULL;
    confirm_callbacks[1].closure = (XtPointer) NULL;

    if (count <= 1) {

	for (i = numScrns - 1; i >= 0; i--)
	    if (scrnList[i] != scrn) {
		if (MsgSetScrn((Msg) NULL, scrnList[i], confirm_callbacks,
			       (XtCallbackList) NULL) == NEEDS_CONFIRMATION)
		    return;
	    }
	for (i = 0; i < numFolders; i++) {
	    toc = folderList[i];

	    if (TocConfirmCataclysm(toc, confirm_callbacks,
				    (XtCallbackList) NULL))
		return;
	}
/* 	if (MsgSetScrn((Msg) NULL, scrn))
 *	    return;
 * %%%
 *	for (i = 0; i < numFolders; i++) {
 *	    toc = folderList[i];
 *	    if (toc->scanfile && toc->curmsg)
 *		CmdSetSequence(toc, "cur", MakeSingleMsgList(toc->curmsg));
 *	}
 */
	XtUnmapWidget(scrn->parent);
	XtDestroyApplicationContext
	    (XtWidgetToApplicationContext(scrn->parent));
	exit(0);
    }
    else {
	if (MsgSetScrn((Msg) NULL, scrn, confirm_callbacks, 
		       (XtCallbackList) NULL) == NEEDS_CONFIRMATION)
	    return;
	DestroyScrn(scrn);	/* doesn't destroy first toc&view scrn */
    }
}

/*ARGSUSED*/
void XmhClose(Widget w, XEvent *event, String *params, Cardinal *num_params)
{
    Scrn	scrn;
    if (event->type == ClientMessage &&
	event->xclient.data.l[0] != wm_delete_window)
	return;
    scrn = ScrnFromWidget(w);
    DoClose(w, (XtPointer) scrn, (XtPointer) NULL);
}

/* Open the selected folder in this screen. */

/* ARGSUSED*/
void DoOpenFolder(Widget widget, XtPointer client_data, XtPointer call_data)
{
    /* Invoked by the Folder menu entry "Open Folder"'s notify action. */

    Scrn	scrn = (Scrn) client_data;
    Toc		toc  = SelectedToc(scrn);
    TocSetScrn(toc, scrn);
}


/*ARGSUSED*/
void XmhOpenFolder(Widget w, XEvent *event, String *params, Cardinal *num_params)
{
    Scrn	scrn = ScrnFromWidget(w);

    /* This action may be invoked from folder menu buttons or from folder
     * menus, as an action procedure on an event specified in translations.
     * In this case, the action will open a folder only if that folder
     * was actually selected from a folder button or menu.  If the folder
     * was selected from a folder menu, the menu entry callback procedure,
     * which changes the selected folder, and is invoked by the "notify" 
     * action, must have already executed; and the menu entry "unhightlight"
     * action must execute after this action.
     *
     * This action does not execute if invoked as an accelerator whose
     * source widget is a menu button or a folder menu.  However, it 
     * may be invoked as a keyboard accelerator of any widget other than
     * the folder menu buttons or the folder menus.  In that case, it will
     * open the currently selected folder.
     *
     * If given a parameter, it will take it as the name of a folder to
     * select and open.
     */

    if (! UserWantsAction(w, scrn)) return;
    if (*num_params) SetCurrentFolderName(scrn, params[0]);
    DoOpenFolder(w, (XtPointer) scrn, (XtPointer) NULL);
}


/* Compose a new message. */

/*ARGSUSED*/
void DoComposeMessage(Widget widget, XtPointer client_data, XtPointer call_data)
{
    Scrn        scrn = NewCompScrn();
    Msg		msg = TocMakeNewMsg(DraftsFolder);
    MsgLoadComposition(msg);
    MsgSetTemporary(msg);
    MsgSetReapable(msg);
    MsgSetScrnForComp(msg, scrn);
    MapScrn(scrn);
}

   
/*ARGSUSED*/
void XmhComposeMessage(Widget w, XEvent *event, String *params, Cardinal *num_params)
{
    DoComposeMessage(w, (XtPointer) NULL, (XtPointer) NULL);
}


/* Make a new scrn displaying the given folder. */

/*ARGSUSED*/
void DoOpenFolderInNewWindow(Widget widget, XtPointer client_data, XtPointer call_data)
{
    Scrn	scrn = (Scrn) client_data;
    Toc 	toc = SelectedToc(scrn);
    scrn = CreateNewScrn(STtocAndView);
    TocSetScrn(toc, scrn);
    MapScrn(scrn);
}


/*ARGSUSED*/
void XmhOpenFolderInNewWindow(Widget w, XEvent *event, String *params, Cardinal *num_params)
{
    Scrn scrn = ScrnFromWidget(w);
    DoOpenFolderInNewWindow(w, (XtPointer) scrn, (XtPointer) NULL);
}


/* Create a new folder with the given name. */

static char *previous_label = NULL;
    /* Widget   widget;          the okay button of the dialog widget */
    /* XtPointer client_data; the lower control area of the popup window */
    /* XtPointer call_data; */
/*ARGSUSED*/
static void CreateFolder(Widget widget, XtPointer client_data, XtPointer call_data)
{
    Toc		toc;
    register int i;
    char	*name;
    Widget	control = (Widget) client_data;
    Widget	prompt_widget, label_widget, w;
    Arg		args[3];
    char 	str[300], *label;

#ifdef NOTDEF
    name = XawDialogGetValueString(dialog);
#endif
    prompt_widget = XtNameToWidget(control, "prompt");
    XtSetArg(args[0], XtNstring, &name);
    XtGetValues(prompt_widget, args, 1);

    for (i=0 ; name[i] > ' ' ; i++) ;
    name[i] = '\0';
    toc = TocGetNamed(name);
    if ((toc) || (i==0) || (name[0]=='/') || ((toc = TocCreateFolder(name))
					      == NULL)) {
	if (toc) 
	    (void) sprintf(str, dgettext(OlmhDomain,
			"Folder \"%s\" already exists.  Try again."), name);
	else if (name[0]=='/')
	    (void) sprintf(str, dgettext(OlmhDomain,
			"Please specify folders relative to \"%s\"."),
			   app_resources.mail_path);
	else 
	    (void) sprintf(str, dgettext(OlmhDomain,
			"Cannot create folder \"%s\".  Try again."), name);
	label = XtNewString(str);
	label_widget = XtNameToWidget(control, "label");
	XtSetArg(args[0], XtNstring, label);
	XtSetValues(label_widget, args, 1);
#ifdef NOTDEF
	XtSetArg(args[1], XtNvalue, "");
	XtSetValues(dialog, args, 2);
#endif
	if (previous_label)
	    XtFree(previous_label);
	previous_label = label;
	allowPopdown = False;
	return;
    }
    for (i = 0; i < numScrns; i++)
	if (scrnList[i]->folderbuttons) {
	    char	*first_slash_ptr, *last_slash_ptr, *parent_name;
	    Widget 	folder_widget, parent_widget;
	    Button	button;
	    if (first_slash_ptr = index(name, '/')) { /* if is subfolder */
		first_slash_ptr[0] = '\0';
                folder_widget = /* An immediate child of the control area */
                        XtNameToWidget(scrnList[i]->folderbuttons->inner,
                                        name);
#ifdef NOTDEF
		button = BBoxFindButtonNamed(scrnList[i]->folderbuttons,
					     name);
#endif
		first_slash_ptr[0] = '/';
		last_slash_ptr = rindex(name, '/');
		if (first_slash_ptr == last_slash_ptr) /* one level deep */
		    parent_widget = folder_widget;
		else {		/* more than one level deep */
		    last_slash_ptr[0] = '\0';
		    parent_name = XtMalloc(strlen(name) + 2);
		    strcpy(parent_name, "*");
		    strcat(parent_name, name);
		    parent_widget = XtNameToWidget(folder_widget, parent_name);
		    last_slash_ptr[0] = '/';
		    XtFree(parent_name);
		}

		if (XtClass(parent_widget) == oblongButtonWidgetClass) {
		    Widget menu_pane = XtParent(parent_widget);
		    XtUnmanageChild(parent_widget);
		    XtDestroyWidget(parent_widget);
		    last_slash_ptr[0] = '\0';
		    parent_widget = XtCreateManagedWidget(name,
				menuButtonWidgetClass, menu_pane, NULL, 0);
		    XtSetArg(args[0], XtNmenuPane, &parent_widget);
		    XtGetValues(parent_widget, args, 1);
		    w = XtCreateManagedWidget(name, oblongButtonWidgetClass,
				parent_widget, NULL, 0);
	            XtAddCallback(w, XtNselect, DoSelectFolder,
					(XtPointer)XrmStringToQuark(name));
		    last_slash_ptr[0] = '/';
		    w = XtCreateManagedWidget(name, oblongButtonWidgetClass,
				parent_widget, NULL, 0);
	            XtAddCallback(w, XtNselect, DoSelectFolder,
					(XtPointer)XrmStringToQuark(name));
		}
		else {
		    XtSetArg(args[0], XtNmenuPane, &parent_widget);
		    XtGetValues(parent_widget, args, 1);
		    w = XtCreateManagedWidget(name, oblongButtonWidgetClass,
				parent_widget, NULL, 0);
	            XtAddCallback(w, XtNselect, DoSelectFolder,
					(XtPointer)XrmStringToQuark(name));
		}
	    }
	    else {
		w = XtCreateManagedWidget(name, oblongButtonWidgetClass,
			scrnList[i]->folderbuttons->inner, NULL, 0);
	        XtAddCallback(w, XtNselect, DoSelectFolder,
					(XtPointer)XrmStringToQuark(name));
	    }
#ifdef NOTDEF
		BBoxAddButton(scrnList[i]->folderbuttons, name,
			      menuButtonWidgetClass, True);
#endif
	}
#ifdef NOTDEF
    DestroyPopup(widget, (XtPointer) XtParent(dialog), (XtPointer) NULL);
#endif
}


/* Create a new folder.  Requires the user to name the new folder. */

/*ARGSUSED*/
void DoCreateFolder(Widget widget, XtPointer client_data, XtPointer call_data)
{
    PopupPrompt(dgettext(OlmhDomain,"Create folder named:"), CreateFolder);
}


/*ARGSUSED*/
void XmhCreateFolder(Widget w, XEvent *event, String *params, Cardinal *num_params)
{
    DoCreateFolder(w, (XtPointer) NULL, (XtPointer) NULL);
}


/*ARGSUSED*/
void CancelDeleteFolder(Widget widget, XtPointer client_data, XtPointer call_data)
{
    DeleteData	deleteData = (DeleteData) client_data;

    TocClearDeletePending(deleteData->toc);

    /* When the delete request is made, the toc currently being viewed is
     * changed if necessary to be the toc under consideration for deletion.
     * Once deletion has been confirmed or cancelled, we revert to display
     * the toc originally under view, unless the toc originally under
     * view has been deleted.
     */

    if (deleteData->original_toc != NULL)
	TocSetScrn(deleteData->original_toc, deleteData->scrn);
    XtFree((char *) deleteData);
}


    /* Widget   widget;          unreliable; sometimes NULL */
    /* XtPointer        client_data;     data structure */
    /* XtPointer        call_data;       unused */
/*ARGSUSED*/
void CheckAndConfirmDeleteFolder(Widget widget, XtPointer client_data, XtPointer call_data)
{
    DeleteData  deleteData = (DeleteData) client_data;
    Scrn	scrn = deleteData->scrn;
    Toc		toc  = deleteData->toc;
    char	str[300];
    XtCallbackRec confirms[2];
    XtCallbackRec cancels[2];
    void CheckAndDeleteFolder();

    static XtCallbackRec yes_callbacks[] = {
	{CheckAndDeleteFolder,	(XtPointer) NULL},
	{(XtCallbackProc) NULL,	(XtPointer) NULL}
    };

    static XtCallbackRec no_callbacks[] = {
	{CancelDeleteFolder,	(XtPointer) NULL},
	{(XtCallbackProc) NULL,	(XtPointer) NULL}
    };

    /* Display the toc of the folder to be deleted. */

    TocSetScrn(toc, scrn);

    /* Check for pending delete, copy, move, or edits on messages in the
     * folder to be deleted, and ask for confirmation if they are found.
     */

    confirms[0].callback = (XtCallbackProc) CheckAndConfirmDeleteFolder;
    confirms[0].closure = client_data;
    confirms[1].callback = (XtCallbackProc) NULL;
    confirms[1].closure = (XtPointer) NULL;
    
    cancels[0].callback = (XtCallbackProc) CancelDeleteFolder;
    cancels[0].closure = client_data;
    cancels[1].callback = (XtCallbackProc) NULL;
    cancels[1].closure = (XtPointer) NULL;

    if (TocConfirmCataclysm(toc, confirms, cancels) ==	NEEDS_CONFIRMATION)
	return;

    /* Ask the user for confirmation on destroying the folder. */

    yes_callbacks[0].closure = client_data;
    no_callbacks[0].closure =  client_data;
    (void) sprintf(str, dgettext(OlmhDomain,
		"Are you sure you want to destroy %s?"), TocName(toc));
    PopupConfirm(scrn->tocwidget, str, yes_callbacks, no_callbacks);
}


/*ARGSUSED*/
void CheckAndDeleteFolder(Widget widget, XtPointer client_data, XtPointer call_data)
{
    DeleteData  deleteData = (DeleteData) client_data;
    Scrn	scrn = deleteData->scrn;
    Toc		toc =  deleteData->toc;
    XtCallbackRec confirms[2];
    XtCallbackRec cancels[2];
    int 	i;
    char	*foldername;
    Widget	parent_widget, this_widget;
    
    /* Check for changes occurring after the popup was first presented. */

    confirms[0].callback = (XtCallbackProc) CheckAndConfirmDeleteFolder;
    confirms[0].closure = client_data;
    confirms[1].callback = (XtCallbackProc) NULL;
    confirms[1].closure = (XtPointer) NULL;
    
    cancels[0].callback = (XtCallbackProc) CancelDeleteFolder;
    cancels[0].closure = client_data;
    cancels[1].callback = (XtCallbackProc) NULL;
    cancels[1].closure = (XtPointer) NULL;
    
    if (TocConfirmCataclysm(toc, confirms, cancels) == NEEDS_CONFIRMATION)
	return;

    /* Delete.  Restore the previously viewed toc, if it wasn't deleted. */

    foldername = TocName(toc);
    TocSetScrn(toc, (Scrn) NULL);
    TocDeleteFolder(toc);
    for (i=0 ; i<numScrns ; i++)
	if (scrnList[i]->folderbuttons) {

	    if (IsSubfolder(foldername)) {
		char parent_name[300], this_folder[300];
		char *c = index( strcpy(parent_name, foldername), '/');
		*c = '\0';

/* Since menus are built upon demand, and are a per-screen resource, 
 * resources, not all toc & view screens will have the same menus built.
 * So the menu entry deletion routines must be able to handle a button
 * whose menu field is null.  It would be better to share folder menus
 * between screens, but accelerators call action procedures which depend
 * upon being able to get the screen from the widget argument.
 */

#ifdef NOTDEF
		DeleteFolderMenuEntry
		    ( BBoxFindButtonNamed( scrnList[i]->folderbuttons,
					  parent_name), foldername);
#endif
		parent_widget = /* An immediate child of the control area */
			XtNameToWidget(scrnList[i]->folderbuttons->inner,
					parent_name);
		strcpy(this_folder, "*");
		strcat(this_folder, foldername);
		this_widget = XtNameToWidget(parent_widget, this_folder);
		XtUnmanageChild(this_widget);
		XtDestroyWidget(this_widget);
	    }
	    else {
#ifdef NOTDEF
		BBoxDeleteButton
		    (BBoxFindButtonNamed( scrnList[i]->folderbuttons,
					 foldername));
#endif
		this_widget = XtNameToWidget(
				scrnList[i]->folderbuttons->inner, foldername);
		XtUnmanageChild(this_widget);
		XtDestroyWidget(this_widget);
	    }

	    /* If we've deleted the current folder, show the Initial Folder */

	    if ((! strcmp(scrnList[i]->curfolder, foldername)) 
		&& (BBoxNumButtons(scrnList[i]->folderbuttons))
		&& (strcmp(foldername, app_resources.initial_folder_name)))
		TocSetScrn(InitialFolder, scrnList[i]);
	}
    XtFree(foldername);
    if (deleteData->original_toc != NULL) 
	TocSetScrn(deleteData->original_toc, scrn);
    XtFree((char *) deleteData);
}


/* Delete the selected folder.  Requires confirmation! */

/*ARGSUSED*/
void DoDeleteFolder(Widget w, XtPointer client_data, XtPointer call_data)
{
    Scrn	scrn = (Scrn) client_data;
    Toc		toc  = SelectedToc(scrn);
    DeleteData	deleteData;

    /* Prevent more than one confirmation popup on the same folder. 
     * TestAndSet returns true if there is a delete pending on this folder.
     */
    if (TocTestAndSetDeletePending(toc))	{
	PopupError(dgettext(OlmhDomain,
			"There is a delete pending on this folder."));
	return;
    }

    deleteData = XtNew(DeleteDataRec);
    deleteData->scrn = scrn;
    deleteData->toc = toc;
    deleteData->original_toc = CurrentToc(scrn);
    if (deleteData->original_toc == toc)
	deleteData->original_toc = (Toc) NULL;

    CheckAndConfirmDeleteFolder(w, (XtPointer) deleteData, (XtPointer) NULL);
}


/*ARGSUSED*/
void XmhDeleteFolder(Widget w, XEvent *event, String *params, Cardinal *num_params)
{
    Scrn	scrn = ScrnFromWidget(w);
    DoDeleteFolder(w, (XtPointer) scrn, (XtPointer) NULL);
}


/*-----	Notes on MenuButtons as folder buttons ---------------------------
 *
 * I assume that the name of the button is identical to the name of the folder.
 * Only top-level folders have buttons.
 * Only top-level folders may have subfolders.
 * Top-level folders and their subfolders may have messages.
 *
 */

static char filename[500];	/* for IsFolder() and for callback */
static int  flen = 0;		/* length of a substring of filename */


/* Function name:	IsFolder
 * Description:		determines if a file is an mh subfolder.
 */
#ifndef SVR4
static int IsFolder(struct direct *ent)
#else /* SVR4 */
static int IsFolder(struct dirent *ent)
#endif
{
    register int i, len;
    char *name = ent->d_name;
    struct stat buf;

    /* mh does not like subfolder names to be strings of digits */

    if (isdigit(name[0]) || name[0] == '#') {
	len = strlen(name);
	for(i=1; i < len && isdigit(name[i]); i++)
	    ;
	if (i == len) return FALSE;
    }
    else if (name[0] == '.')
	return FALSE;

    (void) sprintf(filename + flen, "/%s", name);
    if (stat(filename, &buf) /* failed */) return False;
    return (buf.st_mode & S_IFMT) == S_IFDIR;
}


/* menu entry selection callback for folder menus. */

    /* Widget   w;               the menu entry object */
    /* XtPointer        closure;         foldername */
    /* XtPointer        data;    */
/*ARGSUSED*/
static void DoSelectFolder(Widget w, XtPointer closure, XtPointer data)
{
    Scrn	scrn = ScrnFromWidget(w);
    SetCurrentFolderName(scrn, (char *) XrmQuarkToString((XrmQuark)closure));
}

/*ARGSUSED*/
void FreeMenuData(Widget w, XtPointer client_data, XtPointer call_data)
{
    XtFree((char*) client_data);
}

/* Function name:	AddFolderMenuEntry
 * Description:	
 *	Add an entry to a menu.  If the menu is not already created,
 *	create it, including the (already existing) new subfolder directory.
 * 	If the menu is already created,	add the new entry.
 */

    /* Button   button;          the corresponding menu button */
    /* char     *entryname;      the new entry, relative to MailDir */
static void AddFolderMenuEntry(Button button, char *entryname)
{
    Arg		args[4];
    char *	name;
    char *	c;
    char        tmpname[300];
    char *	label;
    static XtCallbackRec callbacks[] = {
	{ DoSelectFolder,		(XtPointer) NULL },
	{ (XtCallbackProc) NULL,	(XtPointer) NULL}
    };
    static XtCallbackRec destroyCallbacks[] = {
	{ FreeMenuData,			(XtPointer) NULL },
	{ (XtCallbackProc) NULL,	(XtPointer) NULL}
    };

    /* The menu must be created before we can add an entry to it. */

    if (button->menu == NULL || button->menu == NoMenuForButton) {
	CreateFolderMenu((Widget)button->widget, NULL);
	return;
    }
    name = XtNewString(entryname);
    callbacks[0].closure = (XtPointer) name;
    destroyCallbacks[0].closure = (XtPointer) name;
    XtSetArg(args[0], XtNcallback, callbacks);			/* ONE */
    XtSetArg(args[1], XtNdestroyCallback, destroyCallbacks);	/* TWO */

    /* When a subfolder and its parent folder have identical names,
     * the widget name of the subfolder's menu entry must be unique.
     */
    label = entryname;
    c = index( strcpy(tmpname, entryname), '/');
    if (c) {
	*c = '\0';
	label = ++c;
	if (strcmp(tmpname, c) == 0) {
	    c--;
	    *c = '_';
	}
	name = c;
    }
    XtSetArg(args[2], XtNlabel, label);				/* THREE */
    /* XtCreateManagedWidget(name, smeBSBObjectClass, button->menu, */
    XtCreateManagedWidget(name, oblongButtonWidgetClass, button->menu,
			  args, 3);
}



/* Function name:	CreateFolderMenu
 * Description:	
 *	Menus are created for folder buttons if the folder has at least one
 *	subfolder.  For the directory given by the concatentation of 
 *	app_resources.mail_path, '/', and the name of the button, 
 *	CreateFolderMenu creates the menu whose entries are
 *	the subdirectories which do not begin with '.' and do not have
 *	names which are all digits, and do not have names which are a '#'
 *	followed by all digits.  The first entry is always the name of the
 *	parent folder.  Remaining entries are alphabetized.
 */

void CreateFolderMenu(Widget parent, String name)
{
#ifndef SVR4
    struct direct **namelist;
#else /* SVR4 */
    char **namelist;
#endif
    register int i, n, length;
    Widget	w, menupane;
    Arg		args[1];
    Button	button;
    extern	alphasort();
    char	directory[500];

    n = strlen(app_resources.mail_path);
    (void) strncpy(directory, app_resources.mail_path, n);
    directory[n++] = '/';
    /* (void) strcpy(directory + n, button->name); */
    (void) strcpy(directory + n, name);
    flen = strlen(directory);		/* for IsFolder */
    (void) strcpy(filename, directory);	/* for IsFolder */
#ifndef SVR4
    n = scandir(directory, &namelist, IsFolder, alphasort);
#else /* SVR4 */
    n = ScanDir(directory, &namelist, IsFolder);
#endif    
    if (n <= 0) {
	/* no subfolders, therefore no menu */
	/* BBoxAddButton(buttonbox, name, oblongButtonWidgetClass, True); */
	w = XtCreateManagedWidget(name, oblongButtonWidgetClass, parent,
								NULL, 0);
	XtAddCallback(w, XtNselect, DoSelectFolder,
				(XtPointer)XrmStringToQuark(name));
#ifdef NOTDEF
	button = BBoxFindButtonNamed(buttonbox, name);
	button->menu = NoMenuForButton;
#endif
	return;
    }

    w = XtCreateManagedWidget(name, menuButtonWidgetClass, parent, NULL, 0);
    XtSetArg(args[0], XtNmenuPane, &menupane);
    XtGetValues(w, args, 1);

#ifdef NOTDEF
    BBoxAddButton(buttonbox, name, menuButtonWidgetClass, True);
    button = BBoxFindButtonNamed(buttonbox, name);
#endif
#ifdef NOTDEF
    button->menu = XtCreatePopupShell("menu", simpleMenuWidgetClass,
				      button->widget, (ArgList) NULL, 0);
#endif

    /* The first entry is always the parent folder */

    w = XtCreateManagedWidget(name, oblongButtonWidgetClass, menupane, NULL, 0);
    XtAddCallback(w, XtNselect, DoSelectFolder,
				(XtPointer)XrmStringToQuark(name));
#ifdef NOTDEF
    AddFolderMenuEntry(button, button->name);
#endif

    /* Build the menu by adding all the current entries to the new menu. */

    length = strlen(name);
    (void) strncpy(directory, name, length);
    directory[length++] = '/';
    for (i=0; i < n; i++) {
#ifndef SVR4
	(void) strcpy(directory + length, namelist[i]->d_name);
#else /* SVR4 */
	(void) strcpy(directory + length, namelist[i]);
#endif
	free((char *) namelist[i]);
	CreateFolderMenu(menupane, directory);
    }
    free((char *) namelist);
}


/* Function:	DeleteFolderMenuEntry
 * Description:	Remove a subfolder from a menu.
 */

static void DeleteFolderMenuEntry(Button button, char *foldername)
{
    char *	c;
    Arg		args[2];
    char *	subfolder;
    int		n;
    char	tmpname[300];
    Widget	entry;
    
    if (button == NULL || button->menu == NULL) return;
    XtSetArg(args[0], XtNnumChildren, &n);
    XtSetArg(args[1], XtNlabel, &c);
    XtGetValues(button->menu, args, 2);
    if ((n <= 3 && c) || n <= 2) {
	XtDestroyWidget(button->menu);	
	button->menu = NoMenuForButton;
	return;
    }

    c = index( strcpy(tmpname, foldername), '/');
    if (c) {
	*c = '\0';
	subfolder = ++c;
	if (strcmp(button->name, subfolder) == 0) {
	    c--;
	    *c = '_';
	    subfolder = c;
	}
	if ((entry = XtNameToWidget(button->menu, subfolder)) != NULL)
	    XtDestroyWidget(entry);
    }
}

/* Function Name:	PopupFolderMenu
 * Description:		This action should alwas be taken when the user
 *	selects a folder button.  A folder button represents a folder 
 *	and zero or more subfolders.  The menu of subfolders is built upon
 *	the first reference to it, by this routine.  If there are no 
 *	subfolders, this routine will mark the folder as having no 
 *	subfolders, and no menu will be built.  In that case, the menu
 *	button emulates a command button.  When subfolders exist,
 *	the menu will popup, using the menu button action PopupMenu.
 */

/*ARGSUSED*/
void XmhPopupFolderMenu(Widget w, XEvent *event, String *vector, Cardinal *count)
{
    Button	button;
    Scrn	scrn;

    scrn = ScrnFromWidget(w);
    if ((button = BBoxFindButton(scrn->folderbuttons, w)) == NULL)
	return;
    if (button->menu == NULL)
	CreateFolderMenu((Widget)button->widget, NULL);

    if (button->menu == NoMenuForButton)
	LastMenuButtonPressed = w;
    else {
	XtCallActionProc(button->widget, "PopupMenu", (XEvent *) NULL,
			 (String *) NULL, (Cardinal) 0);
	XtCallActionProc(button->widget, "reset", (XEvent *) NULL,
			 (String *) NULL, (Cardinal) 0);
    }
}


/* Function Name:	XmhSetCurrentFolder
 * Description:		This action procedure allows menu buttons to 
 *	emulate toggle widgets in their function of folder selection.
 *	Therefore, mh folders with no subfolders can be represented
 * 	by a button instead of a menu with one entry.  Sets the currently
 *	selected folder.
 */

/*ARGSUSED*/
void XmhSetCurrentFolder(Widget w, XEvent *event, String *vector, Cardinal *count)
{
    Button	button;
    Scrn	scrn;

    /* The MenuButton widget has a button grab currently active; the
     * currently selected folder will be updated if the user has released
     * the mouse button while the mouse pointer was on the same menu button
     * widget that orginally activated the button grab.  This mechanism is
     * insured by the XmhPopupFolderMenu action setting LastMenuButtonPressed.
     * The action XmhLeaveFolderButton, and it's translation in the application
     * defaults file, bound to LeaveWindow events, insures that the menu
     * button behaves properly when the user moves the pointer out of the 
     * menu button window.
     *
     * This action is for menu button widgets only.
     */

    if (w != LastMenuButtonPressed)
	return;
    scrn = ScrnFromWidget(w);
    if ((button = BBoxFindButton(scrn->folderbuttons, w)) == NULL)
	return;
    SetCurrentFolderName(scrn, button->name);
}


/*ARGSUSED*/
void XmhLeaveFolderButton(Widget w, XEvent *event, String vector, Cardinal *count)
{
    LastMenuButtonPressed = NULL;
}


void Push(Stack *stack_ptr, char *data)
{
    Stack	new = XtNew(StackRec);
    new->data = data;
    new->next = *stack_ptr;
    *stack_ptr = new;
}

char * Pop(Stack *stack_ptr)
{
    Stack	top;
    char 	*data = NULL;

    if ((top = *stack_ptr) != NULL) {
	data = top->data;
	*stack_ptr = top->next;
	XtFree((char *) top);
    }
    return data;
}

/* Parameters are taken as names of folders to be pushed on the stack.
 * With no parameters, the currently selected folder is pushed.
 */

/*ARGSUSED*/
void XmhPushFolder(Widget w, XEvent *event, String *params, Cardinal *count)
{
    Scrn	scrn = ScrnFromWidget(w);
    int		i;

    for (i=0; i < *count; i++) 
	Push(&scrn->folder_stack, params[i]);

    if (*count == 0 && scrn->curfolder)
	Push(&scrn->folder_stack, scrn->curfolder);
}

/* Pop the stack & take that folder to be the currently selected folder. */

/*ARGSUSED*/
void XmhPopFolder(Widget w, XEvent *event, String *params, Cardinal *count)
{
    Scrn	scrn = ScrnFromWidget(w);
    char	*folder;

    if ((folder = Pop(&scrn->folder_stack)) != NULL)
	SetCurrentFolderName(scrn, folder);
}
