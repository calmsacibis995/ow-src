#pragma ident "@(#)screen.c	1.10 92/10/19"
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
 *		        COPYRIGHT 1987, 1989
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

/* scrn.c -- management of scrns. */

#include "xmh.h"
#include <Xol/textbuff.h>
#include <X11/IntrinsicP.h>
#include <Xol/OpenLookP.h>
#include <Xol/TextEditP.h>

#ifdef SYSV
#ifndef bzero
#define bzero(a,b) memset(a,0,b)
#endif
#endif

OlStrRep        text_format;

static void fixSelection(Widget w, XtPointer client_data, XtPointer call_data);
static void EnableCallback(Widget w, XtPointer data, XtPointer junk);

XmhMenuEntryRec	folderMenu[] = {
    {"open",			DoOpenFolder},
    {"openInNew", 		DoOpenFolderInNewWindow},
    {"create",			DoCreateFolder},
    {"delete",			DoDeleteFolder},
    {"line",			(XtCallbackProc) NULL},
    {"close",			DoClose},
};

XmhMenuEntryRec	tocMenu[] = {
    {"inc",			DoIncorporateNewMail},
    {"commit",			DoCommit},
    {"pack",			DoPack},
    {"sort",			DoSort},
    {"rescan",			DoForceRescan},
    {"reverse",			DoReverseReadOrder},
};

XmhMenuEntryRec	messageMenu[] = {
    {"compose",			DoComposeMessage},
    {"next",			DoNextView},
    {"prev",			DoPrevView},
    {"delete",			DoDelete},
    {"move",			DoMove},
    {"copy",			DoCopy},
    {"unmark",			DoUnmark},
    {"viewNew",			DoViewNew},
    {"reply",			DoReply},
    {"forward",			DoForward},
    {"useAsComp",		DoTocUseAsComp},
    {"print",			DoPrint},
};

XmhMenuEntryRec	sequenceMenu[] = {
    {"pick",			DoPickMessages},
    {"openSeq",			DoOpenSeq},
    {"addToSeq",		DoAddToSeq},
    {"removeFromSeq",		DoRemoveFromSeq},
    {"deleteSeq",		DoDeleteSeq},
    {"line",			(XtCallbackProc) NULL},
    {"all",			DoSelectSequence},
};

XmhMenuEntryRec	viewMenu[] = {
    {"reply",			DoViewReply},
    {"forward",			DoViewForward},
    {"useAsComp",		DoViewUseAsComposition},
    {"edit",			DoEditView},
    {"save",			DoSaveView},
    {"print",			DoPrintView},
};

XmhMenuButtonDescRec	MenuBoxButtons[] = {
    {"folderButton",	"folderMenu",	XMH_FOLDER,	folderMenu,
	 XtNumber(folderMenu) },
    {"tocButton",	"tocMenu",	XMH_TOC,	tocMenu,
	 XtNumber(tocMenu) },
    {"messageButton",	"messageMenu",	XMH_MESSAGE,	messageMenu,
	 XtNumber(messageMenu) },
    {"sequenceButton",	"sequenceMenu",	XMH_SEQUENCE,	sequenceMenu,
	 XtNumber(sequenceMenu) },
    {"viewButton",	"viewMenu",	XMH_VIEW,	viewMenu,
	 XtNumber(viewMenu) },
};


/* Fill in the buttons for the view commands. */

static void FillViewButtons(Scrn scrn)
{
    ButtonBox buttonbox = scrn->viewbuttons;
    BBoxAddButton(buttonbox, "close", oblongButtonWidgetClass, True);
    BBoxAddButton(buttonbox, "reply", oblongButtonWidgetClass, True);
    BBoxAddButton(buttonbox, "forward", oblongButtonWidgetClass, True);
    BBoxAddButton(buttonbox, "useAsComp", oblongButtonWidgetClass, True);
    BBoxAddButton(buttonbox, "edit", oblongButtonWidgetClass, True);
    BBoxAddButton(buttonbox, "save", oblongButtonWidgetClass, False);
    BBoxAddButton(buttonbox, "print", oblongButtonWidgetClass, True);
}
    


static void FillCompButtons(Scrn scrn)
{
    ButtonBox buttonbox = scrn->viewbuttons;
    BBoxAddButton(buttonbox, "close", oblongButtonWidgetClass, True);
    BBoxAddButton(buttonbox, "send", oblongButtonWidgetClass, True);
    BBoxAddButton(buttonbox, "reset", oblongButtonWidgetClass, True);
    BBoxAddButton(buttonbox, "compose", oblongButtonWidgetClass, True);
    BBoxAddButton(buttonbox, "save", oblongButtonWidgetClass, True);
    BBoxAddButton(buttonbox, "insert", oblongButtonWidgetClass, True);
}


static void MakeCommandMenu(Scrn scrn, XmhMenuButtonDesc mbd)
{
    register int i;
    Cardinal	 n;
    Widget	menu;
    ButtonBox	buttonbox = scrn->mainbuttons;
    XmhMenuEntry	e;
    Boolean	indent;
    WidgetClass	widgetclass;
    Arg		args[4];
    static XtCallbackRec callbacks[] = {
	{ (XtCallbackProc) NULL, (XtPointer) NULL},
	{ (XtCallbackProc) NULL, (XtPointer) NULL},
	{ (XtCallbackProc) NULL, (XtPointer) NULL},
    };

    /* Menus are created as childen of the Paned widget of the scrn in order
     * that they can be used both as pop-up and as pull-down menus.
     */

    n = 0;
    if (mbd->id == XMH_SEQUENCE) {
	XtSetArg(args[n], XtNallowShellResize, True); 	n++;
    }
#ifdef NOTDEF
    menu = XtCreatePopupShell(mbd->menu_name, simpleMenuWidgetClass,
			      scrn->widget, args, n);
#endif

    indent = (mbd->id == XMH_SEQUENCE || mbd->id == XMH_OPTION) ? True : False;
    e = mbd->entry;
    for (i=0; i < mbd->num_entries; i++, e++) {
	n = 0;
	if (e->function) {
	    callbacks[0].callback = e->function;
	    callbacks[0].closure  = (XtPointer) scrn;
	    callbacks[1].callback = (app_resources.sticky_menu
				     ? (XtCallbackProc) DoRememberMenuSelection
				     : (XtCallbackProc) NULL);
	    XtSetArg(args[n], XtNselect, callbacks);	n++;

	    if (indent) { XtSetArg(args[n], XtNleftMargin, 18);	n++; }
#ifdef NOTDEF
	    widgetclass = smeBSBObjectClass;
#endif
	    widgetclass = oblongButtonWidgetClass;
	} else 
	    continue;
#ifdef NOTDEF
	    widgetclass = smeLineObjectClass;
#endif
	menu = BBoxMenuOfButton( BBoxFindButtonNamed( buttonbox,
						mbd->button_name));
	XtCreateManagedWidget(e->name, widgetclass, menu, args, n);
    }

#ifdef NOTDEF
    AttachMenuToButton( BBoxFindButtonNamed( buttonbox, mbd->button_name),
		       menu, mbd->menu_name);
    if (mbd->id == XMH_OPTION && app_resources.reverse_read_order)
	ToggleMenuItem(XtNameToWidget(menu, "reverse"), True);
#endif
}


/* Create subwidgets for a toc&view window. */

static void MakeTocAndView(Scrn scrn)
{
    register int	i;
    XmhMenuButtonDesc	mbd;
    ButtonBox		buttonbox;
    char		*name;
    Arg			args[1];
#ifdef NOTDEF
    static XawTextSelectType sarray[] = {XawselectLine,
					XawselectPosition,
					XawselectAll,
					XawselectNull};
    static Arg args[] = {
	{ XtNselectTypes,	(XtArgVal) sarray},
	{ XtNdisplayCaret,	(XtArgVal) False}
    };
#endif

    scrn->mainbuttons   = BBoxCreate(scrn, "menuBox");
    scrn->folderlabel   = CreateTitleBar(scrn, "folderTitlebar");
    scrn->folderbuttons = BBoxCreate(scrn, "folders");
    scrn->toclabel      = CreateTitleBar(scrn, "tocTitlebar");
    XtSetArg(args[0], XtNweight, (XtArgVal) app_resources.toc_percentage);
    scrn->tocwidget	= CreateTextSW(scrn, "toc", args, 1);
    XtAddCallback(scrn->tocwidget, XtNconsumeEvent, fixSelection,
		(XtPointer) scrn);
/*
    XtAddCallback(scrn->tocwidget, XtNmotionVerification, fixSelection,
		(XtPointer) scrn);
 */
    if (app_resources.command_button_count > 0)
	scrn->miscbuttons = BBoxCreate(scrn, "commandBox");
    scrn->viewlabel     = CreateTitleBar(scrn, "viewTitlebar");
    XtSetArg(args[0], XtNweight, (XtArgVal) 50);
    scrn->viewwidget    = CreateTextSW(scrn, "view", args, (Cardinal) 1);

    /* the command buttons and menus */

    buttonbox = scrn->mainbuttons;
    mbd = MenuBoxButtons;
    for (i=0; i < XtNumber(MenuBoxButtons); i++, mbd++) {
	name = mbd->button_name;
	BBoxAddButton(buttonbox, name, menuButtonWidgetClass, True);
	MakeCommandMenu(scrn, mbd);
    }
    /* Disable the Sequence menu for OLIT (no time - only a demo!) */
    BBoxDisable( BBoxFindButtonNamed
	(buttonbox, MenuBoxButtons[XMH_SEQUENCE].button_name));

    /* the folder buttons; folder menus are created on demand. */

    buttonbox = scrn->folderbuttons;
    for (i=0 ; i<numFolders ; i++) {
	name = TocName(folderList[i]);
	if (! IsSubfolder(name))
	    CreateFolderMenu(BBoxBBox(buttonbox), name);
	    /* BBoxAddButton(buttonbox, name, menuButtonWidgetClass, True); */
    }

    /* the optional miscellaneous command buttons */

    if (app_resources.command_button_count > 0) {
	char	name[30];
	if (app_resources.command_button_count > 500)
	    app_resources.command_button_count = 500;
	for (i=1; i <= app_resources.command_button_count; i++) {
	    sprintf(name, "button%d", i);
	    BBoxAddButton(scrn->miscbuttons, name, oblongButtonWidgetClass,
							True);
	}
    }

    XtOverrideTranslations(scrn->parent, 
        XtParseTranslationTable("<Message>WM_PROTOCOLS: XmhClose()\n"));

    if (app_resources.mail_waiting_flag) {
	static Arg arglist[] = {XtNiconPixmap, NULL};
	arglist[0].value = (XtArgVal) NoMailPixmap;
	XtSetValues(scrn->parent, arglist, XtNumber(arglist));
    }
}

static void fixSelection(Widget w, XtPointer client_data, XtPointer call_data)
{
    TextEditWidget ctx = (TextEditWidget)w;
    OlVirtualEvent ve = (OlVirtualEvent)call_data;
    TextLocation text_location;
    Scrn scrn = (Scrn) client_data;
    static TextPosition previous_select_start = -1;
    static TextLine previous_line = -1;
    TextPosition select_start, select_end;
    TextLine line_of_display;
    int len;
    Arg args[3];
    Cardinal ac;
    TextBuffer *text_buffer;
    OlTextBufferPtr mltext_buffer;

    if (ve->virtual_name == OL_ADJUST || ve->virtual_name == OL_MENU) {
	/* Disable the 'normal' behaviour of ADJUST and MENU */
	ve->consumed = True;
	return;
    }
    else if (ve->virtual_name != OL_SELECT)
	/* Don't modify anything else except SELECT */
	return;

    /*
     * OK, this is SELECT - consume the event
     */
    ve->consumed = True;

    XtVaGetValues((Widget)ctx, XtNtextFormat , &text_format, 0);

    if(text_format == OL_SB_STR_REP){
            text_buffer = OlTextEditTextBuffer(ctx);
            text_location = LocationOfPosition(text_buffer,
                        _PositionFromXY(w, ve->xevent->xbutton.x,
                                           ve->xevent->xbutton.y));
            text_location.offset = 0;
            select_start = PositionOfLocation(text_buffer, text_location);
    }
    else{

            mltext_buffer = (OlTextBufferPtr)OlTextEditOlTextBuffer(ctx);
            OlLocationOfPosition(mltext_buffer,
                        _PositionFromXY(w, ve->xevent->xbutton.x,
                           ve->xevent->xbutton.y), &text_location);
            text_location.offset = 0;
            select_start = OlPositionOfLocation(mltext_buffer, &text_location);
    }
#ifdef NOTDEF
    if (previous_select_start == select_start)
	return;

    previous_select_start = select_start;
#endif

/*    len = text_buffer->lines.p[text_location.line].buffer->used; */

    if(text_format == OL_SB_STR_REP){
            text_location.offset = LastCharacterInTextBufferLine(text_buffer,
                                                text_location.line);
            select_end = PositionOfLocation(text_buffer, text_location);
    }
    else{
            text_location.offset = OlLastCharInTextBufferLine(mltext_buffer,
                                                text_location.line);
            select_end = OlPositionOfLocation(mltext_buffer, &text_location);
    }

    OlTextEditSetCursorPosition((TextEditWidget)w, select_start,
					select_end, select_start);
}


static void MakeView(Scrn scrn)
{
    XtOverrideTranslations(scrn->parent, 
        XtParseTranslationTable("<Message>WM_PROTOCOLS: XmhCloseView()\n"));
    scrn->viewlabel = CreateTitleBar(scrn, "viewTitlebar");
    scrn->viewwidget = CreateTextSW(scrn, "view", (ArgList)NULL, (Cardinal)0);
    scrn->viewbuttons = BBoxCreate(scrn, "viewButtons");
    FillViewButtons(scrn);
}


static void MakeComp(Scrn scrn)
{
    XtOverrideTranslations(scrn->parent, 
        XtParseTranslationTable("<Message>WM_PROTOCOLS: XmhCloseView()\n"));
    scrn->viewlabel = CreateTitleBar(scrn, "composeTitlebar");
    scrn->viewwidget = CreateTextSW(scrn, "comp", (ArgList)NULL, (Cardinal)0);
    scrn->viewbuttons = BBoxCreate(scrn, "compButtons");
    FillCompButtons(scrn);
}


/* Create a scrn of the given type. */

Scrn CreateNewScrn(ScrnKind kind)
{
    int i;
    Scrn scrn;
    static Arg arglist[] = {
	{ XtNgeometry,	(XtArgVal) NULL},
	{ XtNinput,	(XtArgVal) True}
    };

    for (i=0 ; i<numScrns ; i++)
	if (scrnList[i]->kind == kind && !scrnList[i]->mapped)
	    return scrnList[i];
    switch (kind) {
       case STtocAndView: arglist[0].value =
			   (XtArgVal)app_resources.toc_geometry;	break;
       case STview:	  arglist[0].value =
			   (XtArgVal)app_resources.view_geometry;	break;
       case STcomp:	  arglist[0].value =
			   (XtArgVal)app_resources.comp_geometry;	break;
       case STpick:	  arglist[0].value =
			   (XtArgVal)app_resources.pick_geometry;	break;
    }

    numScrns++;
    scrnList = (Scrn *)
	XtRealloc((char *) scrnList, (unsigned) numScrns*sizeof(Scrn));
    scrn = scrnList[numScrns - 1] = XtNew(ScrnRec);
    bzero((char *)scrn, sizeof(ScrnRec));
    scrn->kind = kind;
    if (numScrns == 1) scrn->parent = toplevel;
    else scrn->parent = XtCreatePopupShell(
				   progName, topLevelShellWidgetClass,
				   toplevel, arglist, XtNumber(arglist));
    scrn->widget =
	XtCreateManagedWidget(progName, rubberTileWidgetClass, scrn->parent,
			      (ArgList) NULL, (Cardinal) 0);

    switch (kind) {
	case STtocAndView: 	MakeTocAndView(scrn); break;
	case STview:		MakeView(scrn); break;
	case STcomp:		MakeComp(scrn);	break;
    }

    if (kind != STpick) {
	int	theight, min, max;
	Arg	args[1];

	DEBUG0("Realizing...")
	XtRealizeWidget(scrn->parent);
	DEBUG0(" done.\n")

	switch (kind) {
	  case STtocAndView:
	    BBoxLockSize(scrn->mainbuttons);
	    BBoxLockSize(scrn->folderbuttons);
	    theight = GetHeight(scrn->tocwidget) + GetHeight(scrn->viewwidget);
	    theight = app_resources.toc_percentage * theight / 100;
#ifdef NOTDEF
	    XawPanedGetMinMax((Widget) scrn->tocwidget, &min, &max);
	    XawPanedSetMinMax((Widget) scrn->tocwidget, theight, theight);
	    XawPanedSetMinMax((Widget) scrn->tocwidget, min, max);
#endif
	    if (scrn->miscbuttons)
		BBoxLockSize(scrn->miscbuttons);

	    /* fall through */

	  case STview:

	    /* Install accelerators; not active while editing in the view */

	    XtSetArg(args[0], XtNtranslations, &(scrn->edit_translations));
	    XtGetValues(scrn->viewwidget, args, (Cardinal) 1);
	    XtInstallAllAccelerators(scrn->widget, scrn->widget);
	    if (kind == STtocAndView)
		XtInstallAllAccelerators(scrn->tocwidget, scrn->widget);
	    XtInstallAllAccelerators(scrn->viewwidget, scrn->widget);
	    XtSetArg(args[0], XtNtranslations, &(scrn->read_translations));
	    XtGetValues(scrn->viewwidget, args, (Cardinal) 1);

	    if (kind == STview)
		BBoxLockSize(scrn->viewbuttons);
	    break;

	  case STcomp:
	    BBoxLockSize(scrn->viewbuttons);
	    XtInstallAllAccelerators(scrn->widget, scrn->widget);
	    break;
	}

	InitBusyCursor(scrn);
	XDefineCursor(XtDisplay(scrn->parent), XtWindow(scrn->parent),
		      app_resources.cursor);
	XSetWMProtocols(XtDisplay(toplevel), XtWindow(scrn->parent),
			&wm_delete_window, 1);
    }
    scrn->mapped = (numScrns == 1);
    return scrn;
}


Scrn NewViewScrn(void)
{
    return CreateNewScrn(STview);
}

Scrn NewCompScrn(void)
{
    Scrn scrn;
    scrn = CreateNewScrn(STcomp);
    scrn->assocmsg = (Msg)NULL;
    return scrn;
}

void ScreenSetAssocMsg(Scrn scrn, Msg msg)
{
    scrn->assocmsg = msg;
}

/* Destroy the screen.  If unsaved changes are in a msg, too bad. */

void DestroyScrn(Scrn scrn)
{
    if (scrn->mapped) {
	scrn->mapped = False;
	XtPopdown(scrn->parent);
	TocSetScrn((Toc) NULL, scrn);
	MsgSetScrnForce((Msg) NULL, scrn);
	lastInput.win = -1;
    }
}


void MapScrn(Scrn scrn)
{
    if (!scrn->mapped) {
	XtPopup(scrn->parent, XtGrabNone);
	scrn->mapped = True;
    }
}


Scrn ScrnFromWidget(Widget w)           /* heavily used, should be efficient */
{
    register int i;
    while (w && (XtClass(w) != applicationShellWidgetClass) &&
	   (XtClass(w) != topLevelShellWidgetClass))
	w = XtParent(w);
    if (w) {
	for (i=0 ; i<numScrns ; i++) {
	    if (w == (Widget) scrnList[i]->parent)
		return scrnList[i];
	}
    }
    Punt(dgettext(OlmhDomain,"ScrnFromWidget failed!"));
    /*NOTREACHED*/
}
 

/* Figure out which buttons should and shouldn't be enabled in the given
 * screen.  This should be called whenever something major happens to the
 * screen.
 */


/*ARGSUSED*/
static void EnableCallback(Widget w, XtPointer data, XtPointer junk)
{
  EnableProperButtons( (Scrn) data);
}  


#define SetButton(buttonbox, name, value) \
    if (value) BBoxEnable(BBoxFindButtonNamed(buttonbox, name)); \
    else BBoxDisable(BBoxFindButtonNamed(buttonbox, name));


void EnableProperButtons(Scrn scrn)
{
    int value, changed, reapable;
    Button	button;

    if (scrn) {
	switch (scrn->kind) {
	  case STtocAndView:
	    button = BBoxFindButtonNamed
		(scrn->mainbuttons, MenuBoxButtons[XMH_TOC].button_name);
	    value = TocCanIncorporate(scrn->toc);
	    SendMenuEntryEnableMsg(button, "inc", value);

	    button = BBoxFindButtonNamed
		(scrn->mainbuttons, MenuBoxButtons[XMH_SEQUENCE].button_name);
	    value = TocHasSequences(scrn->toc);
	    SendMenuEntryEnableMsg(button, "openSeq", value);
	    SendMenuEntryEnableMsg(button, "addToSeq", value);
	    SendMenuEntryEnableMsg(button, "removeFromSeq", value);
	    SendMenuEntryEnableMsg(button, "deleteSeq", value);

	    button = BBoxFindButtonNamed
		 (scrn->mainbuttons, MenuBoxButtons[XMH_VIEW].button_name);
	    value = (scrn->msg != NULL && !MsgGetEditable(scrn->msg));
	    SendMenuEntryEnableMsg(button, "edit", value);
	    SendMenuEntryEnableMsg(button, "save",
				   scrn->msg != NULL && !value);
	    break;
	  case STview:
	    value = (scrn->msg != NULL && !MsgGetEditable(scrn->msg));
	    SetButton(scrn->viewbuttons, "edit", value);
	    SetButton(scrn->viewbuttons, "save", scrn->msg != NULL && !value);
	    break;
	  case STcomp:
	    if (scrn->msg != NULL) {
		changed = MsgChanged(scrn->msg);
		reapable = MsgGetReapable(scrn->msg);
		SetButton(scrn->viewbuttons, "send", changed || !reapable);
		SetButton(scrn->viewbuttons, "save", changed || reapable);
		SetButton(scrn->viewbuttons, "insert",
			  scrn->assocmsg != NULL ? True : False);

		if (!changed) 
		    MsgSetCallOnChange(scrn->msg, EnableCallback,
				       (XtPointer) scrn);
		else 
		    MsgSetCallOnChange(scrn->msg, (XtCallbackProc) NULL,
				       (XtPointer) NULL);

	    } else {
		BBoxDisable( BBoxFindButtonNamed(scrn->viewbuttons, "send"));
		BBoxDisable( BBoxFindButtonNamed(scrn->viewbuttons, "save"));
		BBoxDisable( BBoxFindButtonNamed(scrn->viewbuttons, "insert"));
	    }
	    break;
	}
    }
}
