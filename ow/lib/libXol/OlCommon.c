#pragma ident	"@(#)OlCommon.c	302.13	97/03/26 lib/libXol SMI"	/* oltemporary:OlCommon.c 1.24	*/

/*
 *	Copyright (C) 1986,1992  Sun Microsystems, Inc
 *			All rights reserved.
 *		Notice of copyright on this source code
 *		product does not indicate publication.
 *
 * RESTRICTED RIGHTS LEGEND: Use, duplication, or disclosure by
 * the U.S. Government is subject to restrictions as set forth
 * in subparagraph (c)(1)(ii) of the Rights in Technical Data
 * and Computer Software Clause at DFARS 252.227-7013 (Oct. 1988)
 * and FAR 52.227-19 (c) (June 1987).
 *
 *	Sun Microsystems, Inc., 2550 Garcia Avenue,
 *	Mountain View, California 94043.
 *
 */

/*	Copyright (c) 1990 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*
 *************************************************************************
 *
 * Description:
 * 		This file contains routines that are common to
 *		OPEN LOOK (TM - AT&T) widgets.
 * 
 ****************************file*header**********************************
 */


#include <libintl.h>
#include <stdio.h>
#include <string.h>

#include <X11/IntrinsicP.h>
#include <X11/StringDefs.h>
#include <X11/Vendor.h>
#include <X11/Xatom.h>

#include <Xol/DrawAreaP.h>
#include <Xol/EventObjP.h>
#include <Xol/ManagerP.h>
#include <Xol/Notice.h>
#include <Xol/OlI18nP.h>
#include <Xol/OpenLookP.h>
#include <Xol/PrimitiveP.h>
#include <Xol/RootShell.h>
#include <Xol/Stub.h>
#include <Xol/TextFieldP.h>
#include <Xol/TextEditP.h>
#include <Xol/TextLineP.h>
#include <Xol/FontChP.h>
#include <Xol/FileChP.h>
#include <Xol/VendorI.h>
#include <Xol/Converters.h>


static Boolean          CvtStringToPixel (
        Display *               display,
        XrmValue *              args,
        Cardinal *              num_args,
        XrmValue *              from,
        XrmValue *              to,
        XtPointer *             converter_data
);

/*
 *************************************************************************
 *
 * Forward Procedure definitions listed by category:
 *		1. Private Procedures
 *		2. Class   Procedures
 *		3. Action  Procedures
 *		4. Public  Procedures
 *
 **************************forward*declarations***************************
 */

					/* private procedures		*/

static Widget	OlMidInitialize (String, String, XrmOptionDescRec*,
					Cardinal, int *, char*[]);


/*
 *************************************************************************
 *
 * Define global/static variables and #defines, and
 * Declare externally referenced variables
 *
 *****************************file*variables******************************
 */

						/* static variables	*/
static char *	OlApplicationTitle = (char *)NULL;
Widget	OlApplicationWidget = (Widget)NULL;

						/* global variables	*/
XrmName		_OlApplicationName = 0;
Display *	toplevelDisplay = (Display *)NULL;


/*
 *************************************************************************
 *
 * Private Procedures
 *
 ***************************private*procedures****************************
 */

/*
 *************************************************************************
 * _OlClass - returns one of the Open Look fundamental class types
 ****************************procedure*header*****************************
 */
WidgetClass
_OlClass(Widget w)
{
    WidgetClass wc;

    if (XtIsVendorShell(w))
	return ( (WidgetClass)vendorShellWidgetClass );

    for (wc = XtClass(w); wc; wc = wc->core_class.superclass)
	if ((wc == primitiveWidgetClass) ||
	    (wc == eventObjClass) ||
	    (wc == managerWidgetClass))
	    break;

    return (wc);
}

/*
 *************************************************************************
 * _OlClearWidget
 ****************************procedure*header*****************************
 */
void
_OlClearWidget (Widget w, Boolean exposures)
{
	if (w != (Widget)NULL && XtIsRealized(w) == True)
	{
		if (_OlIsGadget(w) == True)
		{
			XClearArea(XtDisplayOfObject(w),
				XtWindowOfObject(w),
				(int)w->core.x, (int)w->core.y,
				(unsigned int)w->core.width,
				(unsigned int)w->core.height,
				(Bool)exposures);
		}
		else
		{
			XClearArea(XtDisplay(w), XtWindow(w),
				0,0,0,0, (Bool)exposures);
		}
	}
} /* END OF _OlClearWidget() */

/*
 * OlGetApplicationResources
 *
 * The \fIOlGetApplicationResources\fR procedure is used to
 * retrieve application resources. It currently is an indirect
 * interface to XtGetApplication Resources.
 *
 * OlRegisterDynamicCallback(3), OlUnregisterDynamicCallback(3)
 * OlCallDynamicCallbacks(3)
 *
 * Synopsis:
 *
 *#include <OpenLook.h>
 * ...
 */
void
OlGetApplicationResources (Widget w, XtPointer base, XtResource *resources, int num_resources, ArgList args, Cardinal num_args)
{
	XtGetApplicationResources(w, base, resources, num_resources,
                                        args, num_args);
} /* END OF OlGetApplicationResources */

/*
 *************************************************************************
 * _OlGetApplicationTitle - retrieves a saved title
 ****************************procedure*header*****************************
 */
/* ARGSUSED */
String
_OlGetApplicationTitle(Widget w)
{
#ifdef R3_FIX
    Widget shell;
    Arg arg[1];

    if (OlApplicationTitle != NULL)
	return (OlApplicationTitle);

    shell = _OlGetShellOfWidget(w);

    if (XtIsSubclass(shell, applicationShellWidgetClass)) {
	String argv[];

	XtSetArg(arg[0], XtNargv, &argv);
	XtGetValues(shell, arg, XtNumber(arg));
	return ((argv) ? argv[0] : NULL);

    } else if (XtIsSubclass(shell, wmShellWidgetClass)) {
	String title;

	XtSetArg(arg[0], XtNtitle, &title);
	XtGetValues(shell, arg, XtNumber(arg));
	return (title);

    } else
	return (NULL);

#else /* R3_FIX */
    return ((OlApplicationTitle == NULL) ?
	XrmQuarkToString(_OlApplicationName) : OlApplicationTitle);
#endif /* R3_FIX */
} /* END OF _OlGetApplicationTitle */

/*
 *************************************************************************
 * _OlGetRefNameOrWidget - this routine will be called by GetValuesHook()
 *	from Primitive, EventObj, and Manager. The purpose of this routine
 *	is to get XtNreferenceName and/or XtNreferenceWidget.
 ****************************procedure*header*****************************
 */
void
_OlGetRefNameOrWidget(Widget w, ArgList args, Cardinal *num_args)
{
#define IS_TRAVERSALABLE(w, flag)					     \
	{								     \
		WidgetClass	wc_special = _OlClass(w);		     \
		if (wc_special == primitiveWidgetClass)			     \
			flag = ((PrimitiveWidget)w)->primitive.traversal_on; \
		else if (wc_special == eventObjClass)			     \
			flag = ((EventObj)w)->event.traversal_on;	     \
		else if (wc_special == managerWidgetClass)		     \
			flag = ((ManagerWidget)w)->manager.traversal_on;     \
		else							     \
			flag = False;					     \
	}

	int		i,
			ref_res[2],
			wanted_res,
			pos;
	Boolean		traversalable;
	Widget		shell = _OlGetShellOfWidget(w),
			ref_widget;
	OlFocusData *	fd = _OlGetFocusData(shell, NULL);
	_OlArray	list;

	IS_TRAVERSALABLE(w, traversalable);
	if (traversalable == False)
		return;
	ref_res[0] = ref_res[1] = -1;
	wanted_res = 0;
	for (i = 0; i < *num_args; i++)
	{
		if (strcmp(args[i].name, XtNreferenceName) == 0 ||
		    strcmp(args[i].name, XtNreferenceWidget) == 0)
			ref_res[wanted_res++] = i;
	}
	if (wanted_res == 0)	/* didn't catch any */
		return;
	if (fd == NULL)
		return;

	list = &(fd->traversal_list);
	if ((pos = _OlArrayFind(list, w)) == _OL_NULL_ARRAY_INDEX)
		return;

	if (pos == list->num_elements - 1)	/* last item */
		return;

	for (i = pos+1; i < list->num_elements; i++)
	{
		Boolean		traversalable;

		ref_widget = (Widget) list->array[i];
		IS_TRAVERSALABLE(ref_widget, traversalable);
		if (traversalable == True)
			break;

		ref_widget = NULL;	/* set to NULL and keep looking */
	}
	if (ref_widget == NULL)
		return;

	for (i = 0; i < wanted_res; i++)
	{
		if (strcmp(args[i].name, XtNreferenceName) == 0)
		{
			char ** data = (char **)(args[i].value);

			*data = XtName(ref_widget);
		}
		else		/* must be XtNreferenceWidget */
		{
			Widget * data = (Widget *)(args[i].value);

			*data = ref_widget;
		}
	}
#undef IS_TRAVERSALABLE
} /* END OF _OlGetRefNameOrWidget() */

/*
 *************************************************************************
 * _OlGetShellOfWidget - this routine starts at the given widget looking to
 * see if the widget is a shell widget.  If it is not, it searches up the
 * widget tree until it finds a shell or until a NULL widget is
 * encountered.  The procedure returns either the located shell widget or
 * NULL.
 ****************************procedure*header*****************************
 */
Widget
_OlGetShellOfWidget(register Widget w)
	                  		/* Widget to begin search at	*/
{
	while(w != (Widget) NULL && !XtIsShell(w))
		w = w->core.parent;
	return(w);
} /* END OF _OlGetShellOfWidget() */

/*
 *************************************************************************
 * _OlSetApplicationTitle - saves a title for later use
 ****************************procedure*header*****************************
 */
void
_OlSetApplicationTitle(char *title)
{
	if (OlApplicationTitle != NULL)
		XtFree(OlApplicationTitle);

	OlApplicationTitle = (title == NULL) ? 
			NULL : strcpy(XtMalloc(strlen(title) + 1), title);

} /* END OF _OlSetApplicationTitle */

/****************************************************************************
 * _OlSetWMProtocol- 
 */
void
_OlSetWMProtocol(Display *display, Window window, Atom property)
{
	Atom *		atoms;
	int		num_atoms;
	int		i;
	Atom		atom = OlInternAtom(display, WM_PROTOCOLS_NAME);

	atoms = GetAtomList(display, window, atom, &num_atoms, False);

	if ((atoms != NULL) && (num_atoms > 0))  {
		for (i = 0; i < num_atoms; i++)  {
			if (atoms[i] == property)  {
				XtFree((char *)atoms);
				return;		/* property already set */
			}
		}
	}

	XChangeProperty(display, window, atom, XA_ATOM,
			32, PropModeAppend, (unsigned char *) &property, 1);

	if (atoms != NULL)
		XtFree((char *)atoms);

}

static OlStrRep _ol_default_text_format = OL_SB_STR_REP;
 
/*
 *************************************************************************
 * _OlGetDefaultTextFormat() - this is a XtResourceDefaultProc, used in
 * widgets with a XtNtextFormat resource to obtain the default value.
 ****************************procedure*header*****************************
 */
void
_OlGetDefaultTextFormat(Widget w, int offset, XrmValue * value)
{
        value->addr = (XtPointer) &_ol_default_text_format;
}

/*
 *********************************************************************
 * _OlGetDefaultFocusColor() - this is an XtResourceDefaultProc used
 * by text input widgets (and widgets containing text input widgets)
 * to determine the Default color for the input focus.
 * The default caret should be the FontColor (a la OPEN LOOK) unless
 * XtNmouseless = TRUE && XtNinputFocusFeedback=OL_INPUTFOCUSCOLOR,
 * in which case it's Red.
 *********************************************************************
 */
void
_OlGetDefaultFocusColor(Widget w, int offset, XrmValue *value)
{

    if (_OlMouseless(w) && _OlInputFocusFeedback(w) == OL_INPUT_FOCUS_COLOR) {
        XrmValue from, to;
        static String caretColor = "Red";
        static Pixel caretPixel;

        from.addr = caretColor;
        from.size = strlen(caretColor) + 1;
        to.addr   = NULL;

        if (!XtConvertAndStore(w, XtRString, &from, XtRPixel, &to)) {
            OlWarning("_GetDefaultCaretColor: failure to convert caret color to Pixel");
        } else {
            caretPixel = (*(Pixel*)to.addr);
            value->addr = (caddr_t)&caretPixel;
	    return;
        }
    }  
    /* OPEN LOOK default caret is Black (color of font) */
    if (XtIsSubclass(w, textLineWidgetClass)) {
        TextLineWidget tlw = (TextLineWidget)w;
        value->addr = (caddr_t)&tlw->primitive.font_color;

    } else if (XtIsSubclass(w, textEditWidgetClass)) {
	TextEditWidget tw = (TextEditWidget)w;
        value->addr = (caddr_t)&tw->primitive.font_color;

    } else if (XtIsSubclass(w, textFieldWidgetClass)) {
	TextFieldWidget tfw = (TextFieldWidget)w;
        value->addr = (caddr_t)&tfw->textField.font_color;

    } else if (XtIsSubclass(w, fontChooserWidgetClass)) {
	FontChooserWidget fw = (FontChooserWidget)w;
	value->addr = (caddr_t)&fw->font_chooser.font_color;
 
    } else if (XtIsSubclass(w, fileChooserWidgetClass)) {
        FileChooserWidget fw = (FileChooserWidget)w;
        value->addr = (caddr_t)&fw->file_chooser.font_color;

    } else
	value->addr = XtDefaultForeground;
}
 
/*
 *************************************************************************
 *
 * Public Procedures
 *
 ****************************public*procedures****************************
 */


/*******************************************************************************
 *
 * OlInitialize() - this is the obsolete convenience routine formerly used by 
 * applications to initialize the Xt(TM) and OLIT(TM) toolkits
 *
 *******************************************************************************/
Widget
OlInitialize(
	const char*		shell_name,	/* initial shell instance name */
	const char*		classname,	/* application class */
	XrmOptionDescRec*	urlist,
	Cardinal		num_urs,
	Cardinal*		argc,
	char*			argv[])
{
	if (OlApplicationWidget != (Widget)NULL)
	{
		OlWarning(dgettext(OlMsgsDomain,
			"OlInitialize: toolkit already initialized"));
	}
	else
	{
		OlPreInitialize((char *)classname, 
					urlist, num_urs, (int *)argc, argv);

		OlApplicationWidget = OlMidInitialize((char *)shell_name, 
						      (char *)classname,
						      urlist, num_urs,
						      (int *)argc, argv);

		OlPostInitialize((char *)classname, 
				urlist, num_urs, (int *)argc, argv);
	}

	return (OlApplicationWidget);
}


/*
 *************************************************************************
 * OlPreInitialize - this routines initializes the OPEN LOOK (TM) parts
 * that must be initialized prior to the Xt (TM) toolkit.
 ****************************procedure*header*****************************
 */
/*ARGSUSED*/
void
OlPreInitialize(char *classname, XrmOptionDescRec *urlist, Cardinal num_urs, int *argc, char **argv)
{
	OlToolkitInitialize((XtPointer)NULL);

	_OlApplicationName = XrmStringToName( argv[0] );
} /* END OF OlPreInitialize() */

/*
 *************************************************************************
 * OlMidInitialize - Equivalent to XtInitialize()
 ****************************procedure*header*****************************
 */
/* ARGSUSED */
static Widget
OlMidInitialize (String name, String classname, XrmOptionDescRec *options, Cardinal num_options, int *argc, char **argv)
{
	Widget		root;
	XtAppContext	app_con;
	extern void	_SetDefaultAppCon ( XtAppContext );

	root = XtAppInitialize(&app_con, classname, options, num_options,
			       argc, argv, (String *)NULL, 
						(ArgList)NULL, (Cardinal)0);
    
	_SetDefaultAppCon(app_con);
	return (root);
}

/*
 *************************************************************************
 * OlPostInitialize - this routines initializes the OPEN LOOK (TM) parts
 * that must be initialized after the OPEN LOOK "pre-initialize"
 * procedure and after the Xt (TM) toolkit has been initialized.
 ****************************procedure*header*****************************
 */
/* ARGSUSED */
void
OlPostInitialize(char *classname, XrmOptionDescRec *urlist, Cardinal num_urs, int *argc, char **argv)
{
} /* END OF OlPostInitialize() */

/*
 * OlReplayBtnEvent
 *
 * The \fIOlReplayBtnEvent\fR procedure is used to replay a button press
 * event to the next window (towards the root) that is interested in
 * button events.  This provides a means of propagating events up
 * a window tree.
 *
 * See also:
 *
 * LookupOlInputEvent(3)
 *
 * Synopsis:
 *
 * ...
 */
void
OlReplayBtnEvent (Widget w, XtPointer client_data, XEvent *event)
{

	Display *	dpy = XtDisplayOfObject(w);
	Window 		win = XtWindowOfObject(w);
	Window			root;
	Window			parent;
	Window *		children;
	unsigned int		nchildren;
	XWindowAttributes	window_attributes;

	XUngrabPointer(XtDisplayOfObject(w), CurrentTime);

	do
	{
		XQueryTree(dpy, win, &root, &parent, &children, &nchildren);
		if (children != NULL)
			XFree((char *)children);
		XGetWindowAttributes(dpy, parent, &window_attributes);

		event-> xany.window = parent;
		event-> xbutton.x += window_attributes.x;
		event-> xbutton.y += window_attributes.y;

		if (!(window_attributes.all_event_masks & ButtonPressMask ||
		      window_attributes.your_event_mask & ButtonPressMask))
			win = parent;
	} while (win == parent && win != root);

	if (win != parent)
		XSendEvent(XtDisplayOfObject(w), parent, True,
				ButtonPressMask, event);

} /* END OF OlReplayBtnEvent */

/*
 *************************************************************************
 * OlUpdateDisplay - force the appearance of the given widget to be
 *			updated right away. i.e., process all pending
 *			exposure events immediately.
 ****************************procedure*header*****************************
 */
void
OlUpdateDisplay (Widget w)
{
	Display *	dpy = XtDisplayOfObject(w);
	Window		window = XtWindow(w);
	XEvent		xevent;

		/* flush the event queue */
	XSync(dpy, False);

		/* peel off Expose events manually and dispatch them */
	GetToken();
	while (XCheckWindowEvent(dpy, window, ExposureMask, &xevent) == True)
		XtDispatchEvent(&xevent);
	ReleaseToken();

} /* END OF OlUpdateDisplay */

/*
 *************************************************************************
 * OlWhitePixel - return the White pixel for the colormap associated
 *                      with the given widget.
 ****************************procedure*header*****************************
 */
Pixel
OlWhitePixel (Widget w)
{
	XrmValue from, to;
	static String white = "White";
	Pixel retval;

	GetToken();
	if (!XtIsWidget(w))
		w = _XtWindowedAncestor(w);	/* in case of gadgets */

	from.addr = white;
	from.size = strlen(white) + 1;
	to.addr = NULL;

	if (!XtConvertAndStore(
			w,
			XtRString,
			&from,
			XtRPixel,
			&to
	)) {
		OlWarning(dgettext(OlMsgsDomain,
			"OlWhitePixel: failure with CvtStringToPixel"));
		retval = WhitePixelOfScreen(XtScreen(w));
		ReleaseToken();
		return retval;
	}
	
	retval = (*(Pixel *)(to.addr));
	ReleaseToken();
	return retval;
}

/*
 *************************************************************************
 * OlBlackPixel - return the Black pixel for the colormap associated
 *                      with the given widget.
 ****************************procedure*header*****************************
 */
Pixel
OlBlackPixel (Widget w)
{
	XrmValue from, to;
	static String black = "Black";
	Pixel retval;

    	GetToken();
	if (!XtIsWidget(w))
		w = _XtWindowedAncestor(w);	/* in case of gadgets */

	from.addr = black;
	from.size = strlen(black) + 1;
	to.addr = NULL;

	if (!XtConvertAndStore(
			w,
			XtRString,
			&from,
			XtRPixel,
			&to
	)) {
		OlWarning(dgettext(OlMsgsDomain,
			"OlBlackPixel: failure with CvtStringToPixel"));
		retval = BlackPixelOfScreen(XtScreen(w));
		ReleaseToken();
		return retval;
	}
	
	retval = (*(Pixel *)(to.addr));
	ReleaseToken();
	return retval;
}


/*
 *************************************************************************
 * OlColormapOfObject - return the Colormap associated with the given
 *                      widget (or windowed ancestor).
 ****************************procedure*header*****************************
 */
Colormap
OlColormapOfObject (Widget object)
{
    Colormap retval;

    GetToken();
    retval = (XtIsWidget(object) ? object->core.colormap :
                                (_XtWindowedAncestor(object))->core.colormap);
    ReleaseToken();
    return retval;
}

/*
 *************************************************************************
 * OlVisualOfObject - return the Visual associated with the given
 *                      widget (or windowed ancestor).
 ****************************procedure*header*****************************
 */
Visual *
OlVisualOfObject (Widget object)
{
	Visual *visual;
	Widget obj = object;

	if (!obj) {
		OlWarning(dgettext(OlMsgsDomain,
				"OlVisualOfObject: failed to get visual"));

		visual=(Visual *) NULL;
		return visual;
	}

    	GetToken();
	for (; obj && !XtIsShell(obj) && !XtIsSubclass(obj, drawAreaWidgetClass); obj = XtParent(obj));

	if (!obj)
		visual=DefaultVisualOfScreen(object->core.screen);
	else if (XtIsShell(obj))
		visual=((ShellWidget)obj)->shell.visual;
	else
		visual=((DrawAreaWidget)obj)->draw_area.visual;

    	ReleaseToken();
	return visual;
}

/*
 *************************************************************************
 * OlDepthOfObject - return the Depth associated with the given
 *                      widget (or windowed ancestor).
 ****************************procedure*header*****************************
 */

int
OlDepthOfObject (Widget object)
{
    int retval;

    GetToken();
    retval = (XtIsWidget(object) ? object->core.depth :
				(_XtWindowedAncestor(object))->core.depth);
    ReleaseToken();
    return retval;
}

/*
 *************************************************************************
 * OlGetDefaultFont - return the Default Font.
 ****************************procedure*header*****************************
 */
OlFont
OlGetDefaultFont (Widget w)
{
	XrmValue from, to;
	OlFont retval;

    	GetToken();
	if (!XtIsWidget(w))
		w = _XtWindowedAncestor(w);     /* in case of gadgets */

	from.addr = XtDefaultFont;
	from.size = strlen(XtDefaultFont) + 1;
	to.addr = NULL;

	if (!XtConvertAndStore(
		w,
		XtRString,
		&from,
		XtROlFont,
		&to
	)) {
		OlWarning(dgettext(OlMsgsDomain,
		      "OlGetDefaultFont: failure with CvtStringToOlFont"));
		retval = ((OlFont)NULL);
    		ReleaseToken();
		return retval;
		
	}

	retval =  (*(OlFont *)(to.addr));
    	ReleaseToken();
	return retval;
}

/*
 *************************************************************************
 * OlSetDefaultTextFormat() - set the default text format.
 ****************************procedure*header*****************************
 */
void
OlSetDefaultTextFormat(OlStrRep format)
{
        _ol_default_text_format = format;
}
