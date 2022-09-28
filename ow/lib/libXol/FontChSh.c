#pragma ident "@(#)FontChSh.c	1.6    97/03/26 lib/libXol SMI"     /* OLIT */

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

/*
 *************************************************************************
 *
 * Description:	Open Look FontChooserShell widget.
 *		
 *******************************file*header*******************************
 */


#include <ctype.h>
#include <libintl.h>
#include <stdio.h>
#include <stdlib.h>

#include <X11/IntrinsicP.h>
#include <X11/StringDefs.h>

#include <Xol/OlI18nP.h>
#include <Xol/OpenLookP.h>
#include <Xol/FontChShP.h>
#include <Xol/FontCh.h>
#include <Xol/VendorI.h>


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
static void 	BringDownPopup(Widget w, Boolean blastpushpin);
static void	DisplayError(Widget w, Widget shellw,
				OlFCErrorCallbackStruct * err_cb);


					/* class procedures		*/
static Boolean	ActivateWidget (Widget,OlVirtualName,XtPointer);
static void	WMMsgHandler (Widget, OlDefine, OlWMProtocolVerify *);
static void	ClassInitialize(void);
static void	Initialize(Widget request, Widget new, ArgList args, Cardinal *num_args);
static void   	InitializeHook(Widget new, ArgList args, Cardinal *pnum_args);
static void 	Realize(Widget w, Mask *valueMask, XSetWindowAttributes *attributes);
static void 	Destroy(Widget widget);

static Boolean	SetValues(
			Widget		current,
			Widget		request,
			Widget		new,
			ArgList		args,
			Cardinal *	num_args);

					/* action procedures		*/

					/* public procedures		*/


/*
 *************************************************************************
 *
 * Define global/static variables and #defines, and
 * Declare externally referenced variables
 *
 *****************************file*variables******************************
 */

/*
 *************************************************************************
 *
 * Define Translations and Actions
 *
 ***********************widget*translations*actions**********************
 */


/*
 *************************************************************************
 *
 * Define Resource list associated with the Widget Instance
 *
 ****************************widget*resources*****************************
 */


static XtResource resources[] =
{
    /* XtNtextFormat should always be the first resource */
    {
	XtNtextFormat, XtCTextFormat,
	XtROlStrRep, sizeof(OlStrRep), XtOffset(FontChooserShellWidget,
						font_chooser.text_format),
	XtRCallProc, (XtPointer) _OlGetDefaultTextFormat
    }, {
	XtNfontChooserWidget, XtCFontChooserWidget,
	XtRPointer, sizeof(Widget), XtOffset(FontChooserShellWidget,
					     font_chooser.fc_composite),
	XtRPointer, (XtPointer) NULL
    },

};

/*
 *************************************************************************
 *
 * Define Class Extension Resource List
 *
 *************************************************************************
 */
#define OFFSET(field)	XtOffsetOf(OlVendorPartExtensionRec, field)

static XtResource
ext_resources[] = {
	{ XtNmenuButton, XtCMenuButton, XtRBoolean, sizeof(Boolean),
	  OFFSET(menu_button), XtRImmediate, (XtPointer)False },

	{ XtNpushpin, XtCPushpin, XtROlDefine, sizeof(OlDefine),
	  OFFSET(pushpin), XtRImmediate, (XtPointer)OL_OUT },

	{ XtNresizeCorners, XtCResizeCorners, XtRBoolean, sizeof(Boolean),
	  OFFSET(resize_corners), XtRImmediate, (XtPointer)False },

	{ XtNmenuType, XtCMenuType, XtROlDefine, sizeof(OlDefine),
	  OFFSET(menu_type), XtRImmediate, (XtPointer)OL_MENU_LIMITED },

	{ XtNfooterPresent, XtCFooterPresent, XtRBoolean, sizeof(Boolean),
	OFFSET(footer_present), XtRImmediate, (XtPointer)TRUE },

	{ XtNimFooterPresent, XtCImFooterPresent, XtRBoolean, sizeof(Boolean),
	OFFSET(im_footer_present), XtRImmediate, (XtPointer)False },

	{ XtNwinType, XtCWinType, XtROlDefine, sizeof(OlDefine),
	  OFFSET(win_type), XtRImmediate, (XtPointer)OL_WT_CMD },
};

/*
 *************************************************************************
 *
 * Define Class Extension Records
 *
 *************************************************************************
 */
static OlVendorClassExtensionRec
vendor_extension_rec = {
    {
        NULL,                                   /* next_extension       */
        NULLQUARK,                              /* record_type          */
        OlVendorClassExtensionVersion,          /* version              */
        sizeof(OlVendorClassExtensionRec)       /* record_size          */
    },  /* End of OlClassExtension header       */
        ext_resources,                          /* resources            */
        XtNumber(ext_resources),                /* num_resources        */
        NULL,                                   /* private              */
        NULL,                                   /* set_default          */
	NULL,					/* get_default 		*/
	NULL,					/* destroy		*/
        NULL,                                   /* initialize           */
        NULL,                                   /* set_values           */
        NULL,                                   /* get_values           */
        XtInheritTraversalHandler,              /* traversal_handler    */
        XtInheritHighlightHandler,  		/* highlight_handler    */
        ActivateWidget, 			/* activate function    */
        NULL,              			/* event_procs          */
        0,              			/* num_event_procs      */
        NULL,					/* part_list            */
	{ NULL, 0 },				/* dyn_data		*/
	NULL,					/* transparent_proc	*/
	WMMsgHandler,				/* wm_proc		*/
	FALSE,					/* override_callback	*/
}, *vendor_extension = &vendor_extension_rec;

/*
 *************************************************************************
 *
 * Define Class Record structure to be initialized at Compile time
 *
 ***************************widget*class*record***************************
 */

FontChooserShellClassRec fontChooserShellClassRec = {
  {
/* core_class fields      */
    /* superclass         */    (WidgetClass) &transientShellClassRec,
    /* class_name         */    "FontChooserShell",
    /* size               */    sizeof(FontChooserShellRec),
    /* Class Initializer  */    ClassInitialize,
    /* class_part_initialize*/	NULL,
    /* Class init'ed ?    */	FALSE,
    /* initialize         */    (XtInitProc) Initialize,
    /* initialize_hook    */	InitializeHook,
    /* realize            */    Realize,
    /* actions            */    NULL,
    /* num_actions        */    0,
    /* resources          */    resources,
    /* resource_count     */	XtNumber(resources),
    /* xrm_class          */    NULLQUARK,
    /* compress_motion    */    FALSE,
    /* compress_exposure  */    TRUE,
    /* compress_enterleave*/	FALSE,
    /* visible_interest   */    FALSE,
    /* destroy            */    Destroy,
    /* resize             */    XtInheritResize,
    /* expose             */    NULL,
    /* set_values         */    NULL,
    /* set_values_hook    */	NULL,			
    /* set_values_almost  */	XtInheritSetValuesAlmost,  
    /* get_values_hook    */	NULL,			
    /* accept_focus       */    XtInheritAcceptFocus,
    /* intrinsics version */	XtVersion,
    /* callback offsets   */    NULL,
    /* tm_table           */    XtInheritTranslations,
    /* query_geometry     */    NULL,
  },{
/* composite_class fields */
    /* geometry_manager   */    XtInheritGeometryManager,
    /* change_managed     */    XtInheritChangeManaged,
    /* insert_child	  */	XtInheritInsertChild,
    /* delete_child	  */	XtInheritDeleteChild,
    /* extension	  */    NULL
  },{
/* shell_class fields 	*/
    /* dummy		*/	0
  },{
/* wm_shell_class fields*/
    /* dummy		*/	0
  },{
/* vendor_shell_class fields*/
    /* dummy           */	(XtPointer) &vendor_extension_rec
  },{
/* tranisent_shell_class fields*/
    /* dummy		*/	0
  },{
/* font_chooser_shell_class fields*/
    /* dummy		*/	0
  }
};


/*
 *************************************************************************
 *
 * Public Widget Class Definition of the Widget Class Record
 *
 *************************public*class*definition*************************
 */

WidgetClass fontChooserShellWidgetClass = (WidgetClass)
					    &fontChooserShellClassRec;

/*
 *************************************************************************
 *
 * Private Procedures
 *
 ***************************private*procedures****************************
 */

/*
 *************************************************************************
 *
 * BringDownPopup(w, blastpushpin)
 *
 * Given a widget, find its shell and check the 
 * shell's pushpin state, and pop down the shell if pin is out.  
 *
 * If blastpushpin is true, pull the pin out, call dismiss callbacks,
 * and pop down the shell without verifying.
 *
 ****************************procedure*header*****************************
 */

static void
BringDownPopup(Widget w, Boolean blastpushpin)
{
    Widget 			shellw;
    OlVendorPartExtension	part;

    for(shellw = w; shellw; shellw = shellw->core.parent) {
	if (XtIsSubclass(shellw, fontChooserShellWidgetClass))
	break;
    }

    if (!shellw) {
	OlWarning (dgettext(OlMsgsDomain,
	"FontChooserShell widget can't pop down!"));
	return;
    }

    if (blastpushpin == True) {
	Arg args[1];
	XtSetArg(args[0], XtNpushpin, OL_OUT);
	XtSetValues(shellw, args, 1);
	XtPopdown(shellw);
	return;
    }

    part = _OlGetVendorPartExtension(shellw);

    switch (part->pushpin) {
    case OL_IN:
	break;
    case OL_OUT:
	default:
	XtPopdown(shellw);
	break;
    }
}

/*
 *************************************************************************
 *
 * DisplayError(w, fontchoosershell)
 *
 *	In case of error in the FontChooser, display an error message
 *	in the shell footer.
 ****************************procedure*header*****************************
 */

static void
DisplayError(Widget w, Widget shellw, OlFCErrorCallbackStruct * err_cb)
{
    char	*err_msg;

    if (err_cb->reason != OL_REASON_ERROR)
		return;

	if (err_msg = malloc(1024)) {
		switch (err_cb->error_num) {
			case OL_FC_ERR_NO_FONTS_FOUND:
				strncpy(err_msg,
					dgettext(OlMsgsDomain, "No fonts were found."), 1024);
				break;
			case OL_FC_ERR_NO_INITIAL_FONT:
				snprintf(err_msg, 1024,
					dgettext(OlMsgsDomain, "Could not find XtNinitialFont: %s"),
						(err_cb->font_name != NULL)? err_cb->font_name : "");
				break;
			case OL_FC_ERR_BAD_FONT_SEARCH_SPEC:
				snprintf(err_msg, 1024, dgettext(OlMsgsDomain,
					"XtNfontSearchSpec: %s, is not in XLFD format"),
						(err_cb->font_name != NULL)? err_cb->font_name : "");
				break;
			case OL_FC_ERR_MISSING_CHARSETS:
				snprintf(err_msg, 1024, dgettext(OlMsgsDomain,
					"Could not find all components of the font: %s (missing charsets)"),
						(err_cb->font_name != NULL)? err_cb->font_name : "");
				break;
			default:
				strncpy(err_msg, dgettext(OlMsgsDomain, "Unknown internal error."), 1024);
		}
		XtVaSetValues(shellw, XtNleftFooterString, err_msg, NULL);
		free(err_msg);
	}
}

/*
 *************************************************************************
 *
 * Class Procedures
 *
 ****************************class*procedures*****************************
 */

/*
 *************************************************************************
 *
 *  Initialize
 *
 *	Add an event handler to intercept client messages.
 * 
 ****************************procedure*header*****************************
 */

/* ARGSUSED */
static void 
Initialize (Widget request, Widget new, ArgList args, Cardinal *num_args)
{
    FontChooserShellWidget	new_fcsw = (FontChooserShellWidget) new;
    FontChooserShellPart *	fcsp     = &(new_fcsw->font_chooser);
} /* Initialize */

/*
 *************************************************************************
 *
 * InitializeHook:
 *
 *	Create fontChooser widget as a child
 *	Set up "post-select" routine to check pushpin.
 * 
 ****************************procedure*header*****************************
 */

static void
InitializeHook(Widget new, ArgList args, Cardinal *pnum_args)
{
    FontChooserShellWidget	new_fcsw = (FontChooserShellWidget)new;
    FontChooserShellPart *	fcsp     = &(new_fcsw->font_chooser);
    static XtCallbackRec cb[] = {
	{ (XtCallbackProc) BringDownPopup, (XtPointer) False },
	{ NULL, NULL}
    };

/*
**  Make font_chooser child
*/
    fcsp->fc_composite = XtCreateManagedWidget(	"fontchooser",
				fontChooserWidgetClass, 
				(Widget) new_fcsw, args, *pnum_args);
    XtAddCallback(fcsp->fc_composite, XtNapplyCallback,
	    (XtCallbackProc) BringDownPopup, (XtPointer) False);
    XtAddCallback(fcsp->fc_composite, XtNrevertCallback,
	    (XtCallbackProc) BringDownPopup, (XtPointer) False);
    XtAddCallback(fcsp->fc_composite, XtNcancelCallback,
	    (XtCallbackProc) BringDownPopup, (XtPointer) True);
    XtAddCallback(fcsp->fc_composite, XtNerrorCallback,
	    (XtCallbackProc) DisplayError, (XtPointer) new);
}

/*
 *************************************************************************
 * ActivateWidget - this procedure allows external forces to activate this
 * widget indirectly.
 ****************************procedure*header*****************************
*/
/* ARGSUSED */
static Boolean
ActivateWidget (Widget w, OlVirtualName type, XtPointer call_data)
{
	Boolean			consumed = False;
	OlVendorPartExtension	part;

	part = _OlGetVendorPartExtension(w);
	switch (type)
	{
		case OL_DEFAULTACTION:
			consumed = TRUE;
			if ((w = _OlGetDefault(w)) != (Widget)NULL)
				(void) OlActivateWidget(w, OL_SELECTKEY,
						(XtPointer)NULL);
			break;
		case OL_CANCEL:
			consumed = TRUE;
			if (part->pushpin == OL_IN)
				_OlSetPinState(w, OL_OUT);
			XtPopdown(w);
			break;
		case OL_TOGGLEPUSHPIN:
			consumed = TRUE;
			if (part->pushpin == OL_OUT)
				_OlSetPinState(w, OL_IN);
			else
			{
				_OlSetPinState(w, OL_OUT);
				XtPopdown(w);
			}
			break;
		default:
			break;
	}
	return(consumed);
} /* END OF ActivateWidget() */

static void
WMMsgHandler (Widget w, OlDefine action, OlWMProtocolVerify *wmpv)
{
	if (wmpv->msgtype == OL_WM_DELETE_WINDOW) {
		switch(action) {
		case OL_DEFAULTACTION:
		case OL_DISMISS:
			BringDownPopup(w, True);
			break;
		case OL_QUIT:
			XtUnmapWidget(w);
			exit(EXIT_SUCCESS);
			break;
		case OL_DESTROY:
			XtDestroyWidget(w);
			break;
		}
	}
}

static void
ClassInitialize(void)
{
	vendor_extension->header.record_type = OlXrmVendorClassExtension;
}

static void 
Realize(Widget w, Mask *valueMask, XSetWindowAttributes *attributes)
{
        FontChooserShellWidget fcsw = (FontChooserShellWidget)w;
	Window		win;
	Display *	dpy;
	XWMHints *	hintp = &fcsw->wm.wm_hints;

	w->core.border_width = 0;

	(*fontChooserShellClassRec.core_class.superclass->core_class.realize)
		(w, valueMask, attributes);

	if(w->core.parent) {
		Widget	ptr;

		win = XtWindow(w);
		dpy = XtDisplay(w);
		for (	ptr = w->core.parent; 
			ptr->core.parent;
			ptr = ptr->core.parent) {
			}
		hintp->window_group = XtWindow(ptr);
		hintp->flags |=  WindowGroupHint;
		XSetWMHints(dpy, win, hintp);
	}
}

static void
Destroy(Widget widget)
{
}

/*
**  SetValues() -- Should look for changes to the callback lists and add
**  buttons as necessary.
*/

static Boolean
SetValues(
	Widget		current,
	Widget		request,
	Widget		new,
	ArgList		args,
	Cardinal *	num_args)
{
    FontChooserShellWidget	fcsw = (FontChooserShellWidget) new;
    FontChooserShellWidget	ofcsw = (FontChooserShellWidget) current;

    fcsw->font_chooser.text_format = ofcsw->font_chooser.text_format;
    fcsw->font_chooser.fc_composite = ofcsw->font_chooser.fc_composite;

    return FALSE;
}
