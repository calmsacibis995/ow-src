#pragma ident "@(#)popup.c	1.7 92/10/06"
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
 *			  COPYRIGHT 1989
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

/* popup.c -- Handle pop-up widgets. */

#include "xmh.h"
#ifdef NOTDEF
#include <X11/Xaw/Cardinals.h>
#endif

static void verifyCallback();

typedef struct _PopupStatus {
	Widget popup;
	struct _LastInput lastInput;
} PopupStatusRec, *PopupStatus;


static void DeterminePopupPosition(Position *x_ptr, Position *y_ptr)
{
    Arg		args[3];
    Widget	source;
    Dimension	width, height;

    if (lastInput.win != -1) 
	source = XtWindowToWidget( XtDisplay(toplevel), lastInput.win);
    else
	source = toplevel;	/* %%% need to keep track of last screen */

    if (source != (Widget)NULL) {
	XtSetArg( args[0], XtNwidth, &width );
	XtSetArg( args[1], XtNheight, &height );
	XtGetValues( source, args, 2 );
	XtTranslateCoords( source, (Position) (width / 2),
			  (Position) (height / 2), x_ptr, y_ptr);
    } else {
	*x_ptr = lastInput.x;
	*y_ptr = lastInput.y;
    }
}

static Boolean PositionThePopup(Widget popup, Position x, Position y)
{
    /* Hack.  Fix up the position of the popup.  The xmh app defaults file
     * contains an Xmh*Geometry specification; the effects of that on 
     * popups, and the lack of any user-supplied geometry specification for
     * popups, are mitigated here, by giving the popup shell a position.
     * (Xmh*Geometry is needed in case there is no user-supplied default.)
     * Returns True if an explicit geometry was inferred; false if the
     * widget was repositioned to (x,y).
     */

    Arg		args[4];
    String 	top_geom, pop_geom;

    XtSetArg( args[0], XtNgeometry, &top_geom );
    XtGetValues( toplevel, args, 1 );
    XtSetArg( args[0], XtNgeometry, &pop_geom );
    XtGetValues( popup, args, 1 );

    if (pop_geom == NULL || pop_geom == top_geom) {
	/* if same db entry, then ... */
	XtSetArg( args[0], XtNgeometry, (String) NULL);
	XtSetArg( args[1], XtNx, x);
	XtSetArg( args[2], XtNy, y);
	XtSetArg( args[3], XtNwinGravity, SouthWestGravity);
	XtSetValues( popup, args, 4);
	return False;
    }
    return True;
}


static void CenterPopupPosition(Widget widget, Widget popup, Position px, Position py)
{
    Position	x, y;
    Position	nx, ny;
    Arg		args[3];

    if (widget == NULL) return;
    XtSetArg(args[0], XtNx, &x);
    XtSetArg(args[1], XtNy, &y);
    XtGetValues(popup, args, 2);
    if (x == px && y == py) {

	/* Program sets geometry.  Correct our earlier calculations. */

	nx = (GetWidth(widget) - GetWidth(popup)) / 2;
	ny = (GetHeight(widget) - GetHeight(popup)) / 2;
	if (nx < 0) nx = 0;
	if (ny < 0) ny = 0;
	XtTranslateCoords(widget, nx, ny, &x, &y);
	XtSetArg(args[0], XtNx, x);
	XtSetArg(args[1], XtNy, y);
	XtSetArg(args[2], XtNwinGravity, CenterGravity);
	XtSetValues(popup, args, 3);
    }
}
	 

/* Insure that the popup is wholly showing on the screen.
   Optionally center the widget horizontally and/or vertically
   on current position.
    Position    x, y;           * assert: current position = (x,y) *
 */

static void InsureVisibility(Widget popup, Widget popup_child, Position x, Position y, Boolean centerX, Boolean centerY)
{
    Position	root_x, root_y;
    Dimension	width, height, border;
    Arg		args[3];


    XtSetArg( args[0], XtNwidth, &width );
    XtSetArg( args[1], XtNheight, &height );
    XtSetArg( args[2], XtNborderWidth, &border );
    XtGetValues( popup, args, 3 );

    XtTranslateCoords(popup_child, (Position)0, (Position)0, &root_x, &root_y);
    if (centerX) root_x -= width/2 + border;
    if (centerY) root_y -= height/2 + border;
    if (root_x < 0) root_x = 0;
    if (root_y < 0) root_y = 0;
    border <<= 1;

    if ((int) (root_x + width + border) > WidthOfScreen(XtScreen(toplevel))) {
	root_x = WidthOfScreen(XtScreen(toplevel)) - width - border;
    }
    if ((int) (root_y + height + border) > HeightOfScreen(XtScreen(toplevel))) {
	root_y = HeightOfScreen(XtScreen(toplevel)) - height - border;
    }

    if (root_x != x || root_y != y) {
	XtSetArg( args[0], XtNx, root_x );
	XtSetArg( args[1], XtNy, root_y );
	XtSetValues( popup, args, 2 );
    }
}


/*ARGSUSED*/
void DestroyPopup(Widget widget, XtPointer client_data, XtPointer call_data)
{
    Widget		popup = (Widget) client_data;
    XtPopdown(popup);
    XtDestroyWidget(popup);
}


#define OKAY_NAME "okay"

/*ARGSUSED*/
void XmhPromptOkayAction(Widget w, XEvent *event, String *params, Cardinal *num_params)
{
    XtCallCallbacks(XtNameToWidget(XtParent(w), OKAY_NAME), XtNcallback,
		    XtParent(w));
}

static void
verifyCallback(Widget w, XtPointer client_data, XtPointer call_data)
{
    Boolean *bring_popup_down = (Boolean *)call_data;

    if (allowPopdown == False) {
	*bring_popup_down = False;
        allowPopdown = True;
    }
}

    /* String           question;                the prompting string */
    /* XtCallbackProc   okayCallback;            CreateFolder() */

void PopupPrompt(String question, XtCallbackProc okayCallback)
{
    Arg			args[4];
    static Widget	popup = NULL;
    Widget		upper_control, lower_control;
    Widget		label, prompt, button;
    Position		x, y;

#ifdef NOTDEF
    static String text_translations =
	"<Key>Return: XmhPromptOkayAction()\n\
         Ctrl<Key>R:  no-op(RingBell)\n\
         Ctrl<Key>S:  no-op(RingBell)\n";
    DeterminePopupPosition(&x, &y);
#endif
    if (popup) {
	XtPopup(popup, XtGrabNone);
	return;
    }
    XtSetArg(args[0], XtNallowShellResize, (XtArgVal)True);
    XtSetArg(args[1], XtNinput, (XtArgVal)True);
    XtSetArg(args[2], XtNuserData, (XtArgVal)1);
    popup = XtCreatePopupShell("prompt", popupWindowShellWidgetClass, toplevel,
			       args, 3);
    XtAddCallback(popup, XtNverify, verifyCallback, NULL);
    allowPopdown = True;

    XtSetArg(args[0], XtNupperControlArea, &upper_control);
    XtSetArg(args[1], XtNlowerControlArea, &lower_control);
    XtGetValues(popup, args, 2);
#ifdef NOTDEF
    positioned = PositionThePopup(popup, x, y);
#endif

    XtSetArg(args[0], XtNstring, question);
    label = XtCreateManagedWidget("label", staticTextWidgetClass,
					upper_control, args, 1);
    prompt = XtCreateManagedWidget("prompt", textFieldWidgetClass,
					upper_control, NULL, 0);

    button = XtCreateManagedWidget("apply", oblongButtonWidgetClass,
					lower_control, NULL, 0);
    XtAddCallback(button, XtNselect, okayCallback, upper_control);
    button = XtCreateManagedWidget("cancel", oblongButtonWidgetClass,
					lower_control, NULL, 0);
#ifdef NOTDEF
    XtSetArg(args[0], XtNresizable, True);
    XtSetValues( XtNameToWidget(dialog, "label"), args, 1);
    value = XtNameToWidget(dialog, "value");
    XtSetValues( value, args, 1);
    XtOverrideTranslations(value, XtParseTranslationTable(text_translations));


    XawDialogAddButton(dialog, OKAY_NAME, okayCallback, (XtPointer) dialog);
    XawDialogAddButton(dialog, "cancel", DestroyPopup, (XtPointer) popup);
    XtInstallAllAccelerators(popup, popup);
#endif
    XtRealizeWidget(popup);
    /*InsureVisibility(popup, dialog, x, y, positioned ? False : True, False);*/
    XDefineCursor(XtDisplay(popup), XtWindow(popup), app_resources.cursor);
    XtPopup(popup, XtGrabNone);
}

#undef OKAY_NAME

/* ARGSUSED */
static void FreePopupStatus( Widget w, XtPointer closure, XtPointer call_data )
{
    PopupStatus popup = (PopupStatus)closure;
    XtPopdown(popup->popup);
    XtDestroyWidget(popup->popup);
    XtFree((char *) closure);
}


void PopupNotice( char *message, XtCallbackProc callback, XtPointer closure )
{
    PopupStatus popup_status = (PopupStatus)closure;
    static Widget notice_text, notice_box;
    Widget button;
    static Arg noticeArgs[] = {
	{XtNtextArea, (XtArgVal) &notice_text},
	{XtNcontrolArea, (XtArgVal) &notice_box},
    };
    Arg args[2];
    Position x, y;
    char command[65], label[256];

    if (popup_status == (PopupStatus)NULL) {
	popup_status = XtNew(PopupStatusRec);
	popup_status->lastInput = lastInput;
    }
    if (sscanf( message, "%64s", command ) != 1)
	(void) strcpy( command, "system" );
    else {
	int l = strlen(command);
	if (l && command[--l] == ':')
	    command[l] = '\0';
    }
    (void) sprintf( label, "%.64s command returned:", command );

#ifdef NOTDEF
    DeterminePopupPosition(&x, &y);
    XtSetArg( args[0], XtNallowShellResize, True );
    XtSetArg( args[1], XtNinput, True );
    popup_status->popup = XtCreatePopupShell( "notice",
			     transientShellWidgetClass, toplevel, args, 2 );
    PositionThePopup(popup_status->popup, x, y);
#endif

    popup_status->popup = XtCreatePopupShell( "notice",
			     noticeShellWidgetClass, toplevel, NULL, 0);

    XtGetValues(popup_status->popup, noticeArgs, XtNumber(noticeArgs));

    strcat(label, "\n\n");
    strcat(label, message);
    XtSetArg( args[0], XtNstring, (XtArgVal)label);
    XtSetValues(notice_text, args, 1);

    button = XtCreateManagedWidget("confirm", oblongButtonWidgetClass,
			     notice_box, NULL, 0);
    XtAddCallback(button, XtNselect, ((callback != (XtCallbackProc) NULL)
			    ? callback : (XtCallbackProc) FreePopupStatus),
			    (XtPointer) popup_status);

#ifdef NOTDEF
    XtSetArg( args[0], XtNlabel, label );
    XtSetArg( args[1], XtNvalue, message );
    dialog = XtCreateManagedWidget( "dialog", formWidgetClass,
				   popup_status->popup, args, 2);

    /* The text area of the dialog box will not be editable. */
    value = XtNameToWidget(dialog, "value");

    XtSetArg( args[0], XtNeditType, XawtextRead);
    XtSetArg( args[1], XtNdisplayCaret, False);
    XtSetValues( value, args, 2);

    XtOverrideTranslations(value, NoTextSearchAndReplace);

    XawDialogAddButton( dialog, "confirm",
		       ((callback != (XtCallbackProc) NULL)
		          ? callback : (XtCallbackProc) FreePopupStatus), 
		       (XtPointer) popup_status
		      );
#endif

    XtRealizeWidget( popup_status->popup );
#ifdef NOTDEF
    XtInstallAllAccelerators(popup_status->popup, popup_status->popup);
    InsureVisibility(popup_status->popup, dialog, x, y, False, False);
#endif
    XDefineCursor(XtDisplay(popup_status->popup),
		  XtWindow(popup_status->popup), app_resources.cursor);
    XtPopup(popup_status->popup, XtGrabNone);
}


void PopupConfirm(Widget center_widget, String question, XtCallbackList affirm_callbacks, XtCallbackList negate_callbacks)
{
    Arg		args[2];
    Widget	popup;
    Widget	dialog;
    Widget	button;
    Position	x, y;
    static Widget notice_text, notice_box;
    static Arg noticeArgs[] = {
        {XtNtextArea, (XtArgVal) &notice_text},
        {XtNcontrolArea, (XtArgVal) &notice_box},
    };
    static XtCallbackRec callbacks[] = {
	{DestroyPopup,		(XtPointer) NULL},
	{(XtCallbackProc) NULL,	(XtPointer) NULL}
    };
    static Arg	shell_args[] = {
	{ XtNallowShellResize,	(XtArgVal) True},
	{ XtNinput,		(XtArgVal) True},
    };


    popup = XtCreatePopupShell( "confirm", noticeShellWidgetClass,
                                toplevel, shell_args, 2);

    XtGetValues(popup, noticeArgs, XtNumber(noticeArgs));

    XtSetArg(args[0], XtNstring, (XtArgVal)question);
    XtSetValues(notice_text, args, 1);

    button = XtCreateManagedWidget("yes", oblongButtonWidgetClass,
                                 notice_box, NULL, 0);
    XtAddCallback(button, XtNselect, DestroyPopup, (XtPointer) popup);
    if (affirm_callbacks)
	XtAddCallbacks(button, XtNselect, affirm_callbacks);


    button = XtCreateManagedWidget("no", oblongButtonWidgetClass,
                                 notice_box, NULL, 0);
    XtAddCallback(button, XtNselect, DestroyPopup, (XtPointer) popup);
    if (negate_callbacks)
	XtAddCallbacks(button, XtNselect, negate_callbacks);

    XtRealizeWidget(popup);

#ifdef NOTDEF
    DeterminePopupPosition(&x, &y);
    popup = XtCreatePopupShell("confirm", transientShellWidgetClass,
			       toplevel, shell_args, XtNumber(shell_args));
    PositionThePopup(popup, x, y); 

    XtSetArg(args[0], XtNlabel, question);
    dialog = XtCreateManagedWidget("dialog", formWidgetClass, popup, args,
				   1);
    
    callbacks[0].closure = (XtPointer) popup;
    XtSetArg(args[0], XtNcallback, callbacks);
    button = XtCreateManagedWidget("yes", oblongButtonWidgetClass, dialog, 
				   args, 1);
    if (affirm_callbacks)
	XtAddCallbacks(button, XtNcallback, affirm_callbacks);


    button = XtCreateManagedWidget("no", oblongButtonWidgetClass, dialog, 
				   args, 0);
    XtAddCallback(button, XtNcallback, DestroyPopup, (XtPointer) popup);
    if (negate_callbacks)
	XtAddCallbacks(button, XtNcallback, negate_callbacks);

    XtRealizeWidget(popup);
    XtInstallAllAccelerators(popup, popup);
    CenterPopupPosition(center_widget, popup, x, y);
    InsureVisibility(popup, dialog, x, y, False, False);
#endif
    XDefineCursor(XtDisplay(popup), XtWindow(popup), app_resources.cursor);
    XtPopup(popup, XtGrabNone);
}


void PopupError(String message)
{
    Arg		args[3];
    Widget	error_popup, button;
    Position	x, y;
    Boolean	positioned;
    static Widget notice_text, notice_box;
    static Arg noticeArgs[] = {
	{XtNtextArea, (XtArgVal) &notice_text},
	{XtNcontrolArea, (XtArgVal) &notice_box},
    };
#ifdef NOTDEF
    static XtCallbackRec callbacks[] = {
	{DestroyPopup,		(XtPointer) NULL},
	{(XtCallbackProc) NULL,	(XtPointer) NULL}
    };

    DeterminePopupPosition(&x, &y);
#endif

    XtSetArg(args[0], XtNallowShellResize, True);
    XtSetArg(args[1], XtNinput, True);

#ifdef NOTDEF
    error_popup = XtCreatePopupShell("error", transientShellWidgetClass,
			       toplevel, args, 2);
    positioned = PositionThePopup(error_popup, x, y);

    XtSetArg(args[0], XtNlabel, message);
    dialog = XtCreateManagedWidget("dialog", formWidgetClass, error_popup,
				   args, 1);
    callbacks[0].closure = (XtPointer) error_popup;
    XtSetArg(args[0], XtNcallback, callbacks);

    XawDialogAddButton(dialog, "OK", DestroyPopup, (XtPointer) error_popup);
#endif

    error_popup = XtCreatePopupShell( "error", noticeShellWidgetClass,
				toplevel, args, 2);

    XtGetValues(error_popup, noticeArgs, XtNumber(noticeArgs));

    XtSetArg( args[0], XtNstring, (XtArgVal)message);
    XtSetValues(notice_text, args, 1);

    button = XtCreateManagedWidget("OK", oblongButtonWidgetClass,
			     notice_box, NULL, 0);
    XtAddCallback(button, XtNselect, DestroyPopup, (XtPointer) error_popup);

    XtRealizeWidget(error_popup);

#ifdef NOTDEF
    XtInstallAllAccelerators(error_popup, error_popup);
	This is taken care of by the OLIT toolkit
    InsureVisibility(error_popup, dialog, x, y,
		     positioned ? False : True, positioned ? False : True);
#endif
    XDefineCursor(XtDisplay(error_popup), XtWindow(error_popup), 
		  app_resources.cursor);
    XtPopup(error_popup, XtGrabNone);
}
