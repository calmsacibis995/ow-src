#pragma ident	"@(#)FileChSh.c	1.12    97/03/26 lib/libXol SMI"    /* OLIT 493	*/

/*
 *	Copyright (C) 1992  Sun Microsystems, Inc
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

/**************************************************************************
 *
 *	Implementation of the file chooser shell widget.
 *		
 **************************************************************************/


/************************************************************************
 *
 *      Imported interfaces 
 *
 ************************************************************************/

#include <errno.h>      	/* errno */
#include <libintl.h>		/* dgettext() */
#include <stdlib.h>		/* fprintf() */
#include <stdio.h>		/* exit(), EXIT_SUCCESS */

#include <X11/IntrinsicP.h>
#include <X11/StringDefs.h>

#include <X11/CoreP.h>
#include <X11/CompositeP.h>
#include <X11/ShellP.h>		/* Shell/WMShell/VendorShell/TransientShell */
#include <X11/VendorP.h>

#include <Xol/EventObj.h>
#include <Xol/Datum.h>
#include <Xol/FileChInit.h>
#include <Xol/FileChP.h>
#include <Xol/OpenLookP.h>
#include <Xol/diags.h>		/* _OlRootWidget() */

#include <Xol/VendorI.h>

#include <Xol/FileChShP.h>	/* interface of this implementation */


/**************************************************************************
 *
 *	Forward Procedure definitions listed by category:
 *
 *		1. Action  Procedures
 *		2. Class   Procedures
 *		3. Private Procedures
 *
 **************************************************************************/

					/* action procedures */


					/* class procedures/methods */

static void	ClassInitialize(void);

static void	Initialize(Widget request, Widget c_new, ArgList args,
	Cardinal* num_args);

static void	Destroy(Widget widget);

static Boolean	SetValues(Widget oldw, Widget request, Widget neww,
	ArgList args, Cardinal* num_args);

static void	Realize(Widget wid, Mask* value_maskp, 
	XSetWindowAttributes* attributesp);

					/* class extension procedures */

static Boolean	ActivateWidget(Widget wid, OlVirtualName type, 
	XtPointer call_data);

static void	WMMsgHandler(Widget wed, OlDefine action, 
	OlWMProtocolVerify*  wmpv);

					/* callback procedures */

static void	pop_up_cb(Widget wid, XtPointer client_data, 
	XtPointer call_data);

static void	pop_down_cb(Widget wid, XtPointer client_data, 
	XtPointer call_data);

static void	post_select_cb(Widget wid, XtPointer client_data, 
	XtPointer call_data);

					/* even handler procedures */

static void	warp_pointer(Widget widget, XtPointer client_data, 
	XEvent* event, Boolean* continue_to_dispatch);

					/* private procedures */

static Boolean	callback_verify(const Widget shellw);

static void	post_header(const Widget wid);


/**************************************************************************
 *
 *	Define Translations and Actions
 *
 **************************************************************************/


/**************************************************************************
 *
 *	Define Resource list associated with the Widget Instance
 *
 **************************************************************************/


#define OFFSET(member)	\
	XtOffsetOf(FileChooserShellRec, file_chooser_shell.member)

static XtResource	resources[] =
{
/* I18N */

	{ XtNtextFormat, XtCTextFormat, XtROlStrRep,
		sizeof (OlStrRep), OFFSET(text_format),
		XtRCallProc, (XtPointer)_OlGetDefaultTextFormat },

/* State */
	{ XtNborderWidth, XtCBorderWidth, XtRDimension, 
		sizeof (Dimension), 
		XtOffsetOf(FileChooserShellRec, core.border_width),
		XtRImmediate, (XtPointer)0 },

	{ XtNoperation, XtCOperation, XtROlDefine,
		sizeof (OlDefine), OFFSET(operation),
		XtRImmediate, (XtPointer)OL_OPEN },

	{ XtNpointerWarping, XtCPointerWarping, XtRBoolean, 
		sizeof (Boolean), OFFSET(pointer_warping),
		XtRImmediate, (XtPointer)OL_POINTER_WARPING },

/* Component Access */

	{ XtNfileChooserWidget, XtCComponentWidget, XtRWidget,
		sizeof (Widget), OFFSET(file_chooser_widget),
		XtRImmediate, (XtPointer)NULL },

/* Callbacks */

	{ XtNverifyCallback, XtCCallback, XtRCallback,
		sizeof (XtCallbackList), OFFSET(verify_callback),
		XtRImmediate, (XtPointer)NULL }

}; /* end of resources */

#undef OFFSET


/**************************************************************************
 *
 *	Define Class Extension Resource List
 *
 **************************************************************************/

#define OFFSET(field)	XtOffsetOf(OlVendorPartExtensionRec, field)
static	XtResource	ext_resources[] = {

	{ XtNfooterPresent, XtCFooterPresent, XtRBoolean, 
		sizeof (Boolean), OFFSET(footer_present), 
		XtRImmediate, (XtPointer)TRUE },

	{ XtNpushpin, XtCPushpin, XtROlDefine, 
		sizeof (OlDefine), OFFSET(pushpin), 
		XtRImmediate, (XtPointer)OL_OUT },

	{ XtNresizeCorners, XtCResizeCorners, XtRBoolean, 
		sizeof(Boolean), OFFSET(resize_corners), 
		XtRImmediate, (XtPointer)TRUE },

	{ XtNwinType, XtCWinType, XtROlDefine, 
		sizeof (OlDefine), OFFSET(win_type), 
		XtRImmediate, (XtPointer)OL_WT_CMD },

}; /* end of ext_resources */
#undef OFFSET


/**************************************************************************
 *
 *	Define Class Extension Records
 *
 **************************************************************************/

static OlVendorClassExtensionRec vendor_extension_rec = {

	{
		NULL,			/* next_extension */
		NULLQUARK,		/* record_type */
		OlVendorClassExtensionVersion,
					/* version */
		sizeof (OlVendorClassExtensionRec)
					/* record_size */

	},	/* required OlClassExtensionRec header */

	ext_resources,			/* extension resources */
	XtNumber(ext_resources),	/* number of resources */
	NULL,				/* private resources*/
	NULL,				/* set shell's default */
	NULL,				/* ping subclass */
	NULL,				/* extension destroy */
	NULL,				/* extension initialize */
	NULL,				/* extension setvalues */
	NULL,				/* extension getvalues */
	XtInheritTraversalHandler,	/* traversal_handler */
	XtInheritHighlightHandler,	/* highlight_handler */
	ActivateWidget,			/* activate function */
	NULL,				/* event_procs */
	0,				/* num_event_procs */
	NULL,				/* instance data list */
	{ NULL, 0 },			/* dyn_data */
	NULL,				/* transparent_proc */
	WMMsgHandler,			/* wm msg handler */
	FALSE,				/* override_callback */

}; /* end of vendor_extension_rec */

static OlVendorClassExtensionRec	*vendor_extension = 
						&vendor_extension_rec;


/**************************************************************************
 *
 *	Define Class Record structure to be initialized at compile time
 *
 **************************************************************************/

FileChooserShellClassRec fileChooserShellClassRec = {

	{	/* core_class fields */

		/* superclass */	(WidgetClass)&transientShellClassRec,
		/* class_name */	"FileChooserShell",
		/* size */		sizeof (FileChooserShellRec),
		/* Class initializer */	ClassInitialize,
		/* class_part_initialize */
					NULL,
		/* Class initialized? */
					FALSE,
		/* initialize */	(XtInitProc)Initialize,
		/* initialize_hook */	NULL,
		/* realize */		Realize,
		/* actions */		NULL,
		/* num_actions */	0,
		/* resources */		resources,
		/* resource_count */	XtNumber(resources),
		/* xrm_class */		NULLQUARK,
		/* compress_motion */	TRUE,
		/* compress_exposure */	XtExposeCompressMaximal,
		/* compress_enterleave */
					TRUE,
		/* visible_interest */	FALSE,
		/* destroy */		Destroy,
		/* resize */		XtInheritResize,
		/* expose */		NULL,
		/* set_values */	NULL,
		/* set_values_hook */	NULL,
		/* set_values_almost */	XtInheritSetValuesAlmost,
		/* get_values_hook */	NULL,
		/* accept_focus */	XtInheritAcceptFocus,
		/* intrinsics version */
					XtVersion,
		/* callback offsets */	NULL,
		/* tm_table */		XtInheritTranslations,
		/* query_geometry */	NULL,

	}, {	/* composite_class fields */

		/* geometry_manager */	XtInheritGeometryManager,
		/* change_managed */	XtInheritChangeManaged,
		/* insert_child */	XtInheritInsertChild,
		/* delete_child */	XtInheritDeleteChild,
		/* extension */		NULL

	}, {	/* shell_class fields */

		/* extension */		NULL

	}, {	/* wm_shell_class fields */

		/* extension */		NULL

	}, {	/* vendor_shell_class fields */

		/* extension */		(XtPointer)&vendor_extension_rec

	}, {	/* transient_shell_class fields */

		/* extension */		NULL

	}, {	/* file_chooser_shell_class fields */

		/* extension */		NULL

	}
}; /* end of fileChooserShellClassRec */ 


/**************************************************************************
 *
 *	Public Widget Class Definition of the Widget Class Record
 *
 **************************************************************************/

WidgetClass fileChooserShellWidgetClass = 
				(WidgetClass)&fileChooserShellClassRec;


/************************************************************************
 *
 *      Implementation of this module's external functions
 *
 ************************************************************************/


/**************************************************************************
 *
 *  _OlFileChShPopDown --
 *
 *	Given a widget, find its shell and try the shell's verify callbacks.
 *	If the verify succeeds, call the dismiss callbacks.  Then check the 
 *	shell's pushpin state, and pop down the shell if pin is out.  
 *
 *	If override_pushpin is true, pull the pin out, call XtNverifyCallback,
 *	and pop down the shell without verifying.
 *
 **************************************************************************/

void
_OlFileChShPopDown(const Widget shellw, const Boolean override_pushpin)
{

	if (NULL == shellw || !XtIsTransientShell(shellw))
		/*NOOP*/;
	else if (TRUE == override_pushpin) {
		XtVaSetValues(shellw, XtNpushpin, OL_OUT, NULL);

		if (TRUE == callback_verify(shellw))
			XtPopdown(shellw);
	} else if (XtIsSubclass(shellw, fileChooserShellWidgetClass)) {
		OlVendorPartExtension	ve_part = 
			_OlGetVendorPartExtension(shellw);

		if (TRUE == callback_verify(shellw))
			switch (ve_part->pushpin) {
			case OL_IN:
				break;
			case OL_OUT:
			/*FALLTHROUGH*/
			default:
				XtPopdown(shellw);
				break;
			}
	}
} /* end of _OlFileChShPopDown() */


/************************************************************************
 *
 *      Implementation of this module's internal functions
 *
 ************************************************************************/
 

/**************************************************************************
 *
 *	Class Procedures
 *
 **************************************************************************/


/**************************************************************************
 *
 *  Initialize --
 * 
 **************************************************************************/

/*ARGSUSED*/
static void
Initialize(
	Widget		request,	/* unused */
	Widget		c_new, 
	ArgList		args, 		/* unused */
	Cardinal*	num_argsp	/* unused */
)
{
	const FileChooserShellWidget	
				new_fcsw = (const FileChooserShellWidget)c_new;
				
	CorePart*		cp = &new_fcsw->core;
	FileChooserShellPart*	my = &new_fcsw->file_chooser_shell;
	Widget			fc;
	Widget			command_button;
	Widget			cancel_button;
	String			component_resource;
	Widget			initial_focus_widget;
	
	_OlStrSetMode(my->text_format);
	_OlVerifyOperationValue(&my->operation);
	my->top_widget = _OlRootWidget(c_new);

	fc = XtVaCreateManagedWidget("fcs_fc",
		fileChooserWidgetClass,	c_new, 
			XtNoperation,		my->operation,

			XtNtextFormat,		my->text_format,

			XtNbackground,		cp->background_pixel,
			XtNbackgroundPixmap,	cp->background_pixmap,
			XtNborderColor,		cp->border_pixel,
			XtNborderPixmap,	cp->border_pixmap,
			XtNborderWidth,		cp->border_width,
		NULL);
	my->file_chooser_widget = fc;

	XtAddCallback(c_new, XtNpopupCallback, pop_up_cb, NULL);
	XtAddCallback(c_new, XtNpopdownCallback, pop_down_cb, c_new);

	post_header(c_new);

	switch (my->operation) {
	case OL_OPEN:
	/*FALLTHROUGH*/
	case OL_INCLUDE:
		component_resource = XtNgotoTypeInWidget;
		break;
	case OL_SAVE:
	/*FALLTHROUGH*/
	case OL_SAVE_AS:
	/*FALLTHROUGH*/
	case OL_DO_COMMAND:
		component_resource = XtNdocumentNameTypeInWidget;
		break;
	}

	XtVaGetValues(fc, 
			XtNcancelButtonWidget, &cancel_button,
			component_resource, &initial_focus_widget,
		NULL);

	if (OL_OPEN != my->operation) {
		XtVaGetValues(fc, XtNcommandButtonWidget, &command_button, NULL);
		XtAddCallback(command_button, XtNpostSelect, post_select_cb, 
			c_new);
	}

	if (!_OlInArgList(XtNx, args, *num_argsp) &&
			!_OlInArgList(XtNy, args, *num_argsp))
		my->positioned = FALSE;
	else
		my->positioned = TRUE;

	XtVaSetValues(c_new, 
			XtNfocusWidget,		initial_focus_widget, 
			XtNminWidth,		197,	/*! hard-wired !*/
			XtNminHeight,		315,	/*! hard-wired !*/
		NULL);
	
} /* end of Initialize() */


/**************************************************************************
 *
 *   ActivateWidget -- 
 *
 *	Allows external forces to activate this widget indirectly
 *
 **************************************************************************/

/* ARGSUSED */
static Boolean
ActivateWidget(Widget w, OlVirtualName type, XtPointer call_data)
{
	Boolean			consumed = FALSE;
	OlVendorPartExtension	part = _OlGetVendorPartExtension(w);

	switch (type) {

	case OL_DEFAULTACTION:
		consumed = TRUE;
		
		if ((Widget)NULL != (w = _OlGetDefault(w)))
			(void) OlActivateWidget(w, OL_SELECTKEY, 
				(XtPointer)NULL);
		break;

	case OL_CANCEL:
		consumed = TRUE;
		
		if (OL_IN == part->pushpin)
			_OlSetPinState(w, OL_OUT);
		/*! call_cancel_callback()? !*/
		XtPopdown(w);

		break;

	case OL_TOGGLEPUSHPIN:
		consumed = TRUE;
		
		if (OL_OUT == part->pushpin)
			_OlSetPinState(w, OL_IN);
		else {
			_OlSetPinState(w, OL_OUT);
			XtPopdown(w);
		}

		break;

	default:
		_OlReport(NULL, dgettext(OlMsgsDomain, "ActivateWidget():  "
			"unknown activation type %d.\n"), type);
		break;
	}

	return consumed;
} /* end of ActivateWidget() */


static void
WMMsgHandler(Widget w, OlDefine action, OlWMProtocolVerify* wmpv)
{

	if (wmpv->msgtype == OL_WM_DELETE_WINDOW) {

		switch (action) {

		case OL_DEFAULTACTION:
		/*FALLTHROUGH*/
		case OL_DISMISS:
			_OlFileChShPopDown(w, TRUE);
			break;

		case OL_QUIT:
			XtUnmapWidget(w);
			exit(EXIT_SUCCESS);
			/*NOTREACHED*/
			break;

		case OL_DESTROY:
			XtDestroyWidget(w);
			break;
		}

	}
} /* end of WMMsgHandler() */


/**************************************************************************
 *
 *  ClassInitialize --
 * 
 **************************************************************************/

static void
ClassInitialize(void)
{

	vendor_extension->header.record_type = OlXrmVendorClassExtension;

	_OlAddOlDefineType("open", OL_OPEN);
	_OlAddOlDefineType("save", OL_SAVE);
	_OlAddOlDefineType("save_as", OL_SAVE_AS);
	_OlAddOlDefineType("include", OL_INCLUDE);
	_OlAddOlDefineType("do_command", OL_DO_COMMAND);

} /* end of ClassInitialize() */


/**************************************************************************
 *
 *  Realize --
 * 
 **************************************************************************/

static void
Realize(Widget wid, Mask* value_maskp, XSetWindowAttributes* attributesp)
{
	FileChooserShellWidget	fcsw = (FileChooserShellWidget)wid;
	FileChooserShellPart*	my = &fcsw->file_chooser_shell;
	XWMHints*		hintp = &fcsw->wm_shell.wm_hints;
	XtRealizeProc		super_realize;
	Widget			parent;

	super_realize = XtSuperclass(wid)->core_class.realize;

	if ((XtRealizeProc)NULL != super_realize) {
		(*super_realize)(wid, value_maskp, attributesp);
	} else
		_OlAbort(NULL, dgettext(OlMsgsDomain, "Realize():  "
			"FileChooserShell's superclass has no realize "
			"procedure."));

	hintp->window_group = XtWindow(my->top_widget);
	hintp->flags |= WindowGroupHint;
	XSetWMHints(XtDisplay(wid), XtWindow(wid), hintp);

	if (!my->positioned)
		XtVaSetValues(wid, 
				XtNx,		0,
				XtNy,		0,
			NULL);
} /* end of Realize() */


/**************************************************************************
 *
 *  Destroy --
 * 
 **************************************************************************/

static void
Destroy(Widget widget)
{
	FileChooserShellWidget	fcsw = (FileChooserShellWidget)widget;
	FileChooserShellPart*	my = &(fcsw->file_chooser_shell);

	XtRemoveAllCallbacks(widget, XtNverifyCallback);
	_OlStrDestruct(&my->header);
} /* end of Destroy() */


/**************************************************************************
 *
 *  SetValues --
 * 
 **************************************************************************/

/*ARGSUSED1*/
static Boolean
SetValues(
	Widget		oldw,
	Widget		request,	/* unused */
	Widget		neww,
	ArgList		args,
	Cardinal*	num_args
)
{
	FileChooserShellWidget	old_fcsw = (FileChooserShellWidget)oldw;
	CorePart*		old_core = &old_fcsw->core;
	FileChooserShellPart*	old = &old_fcsw->file_chooser_shell;
	FileChooserShellWidget	new_fcsw = (FileChooserShellWidget)neww;
	CorePart*		new_core = &new_fcsw->core;
	FileChooserShellPart*	new = &new_fcsw->file_chooser_shell;

	#define CORE_HAS_CHANGED(member) \
		(old_core->member != new_core->member)

	#define HAS_CHANGED(member) \
		(old->member != new->member)

	#define DEFEAT_CHANGE(member) \
		if (HAS_CHANGED(member)) { \
			new->member = old->member; \
			_OlReport(NULL, \
				dgettext(OlMsgsDomain, "SetValues():  " \
					"ignored attempt to set the value of " \
					"an initialization time resource " \
					#member ".\n")); \
		}

	/* IG */
	DEFEAT_CHANGE(text_format);
	DEFEAT_CHANGE(operation);

	/* G */
	DEFEAT_CHANGE(file_chooser_widget);

	/* 
	 * Dispatch resource changes 
	 */
	if (CORE_HAS_CHANGED(background_pixel))
		XtVaSetValues(new->file_chooser_widget, 
			XtNbackground,		new_core->background_pixel, 
		NULL);
	
	if (CORE_HAS_CHANGED(background_pixmap))
		XtVaSetValues(new->file_chooser_widget, 
			XtNbackgroundPixmap,	new_core->background_pixmap, 
		NULL);
	
	if (CORE_HAS_CHANGED(border_pixel))
		XtVaSetValues(new->file_chooser_widget, 
			XtNborderColor,		new_core->border_pixel, 
		NULL);
	
	if (CORE_HAS_CHANGED(border_pixmap))
		XtVaSetValues(new->file_chooser_widget, 
			XtNborderPixmap,	new_core->border_pixmap, 
		NULL);
	
	if (CORE_HAS_CHANGED(border_width))
		XtVaSetValues(new->file_chooser_widget, 
			XtNborderWidth,		new_core->border_width, 
		NULL);
	
	return TRUE;

	#undef	CORE_HAS_CHANGED
	#undef	HAS_CHANGED
	#undef	DEFEAT_CHANGE

} /* end of SetValues() */


/**************************************************************************
 *
 *	Callback Procedures
 *
 **************************************************************************/


/************************************************************************
 *
 *  pop_up_cb -- 
 *
 ************************************************************************/

/*ARGSUSED2*/
static void
pop_up_cb(
	Widget			wid,
	XtPointer		client_data,
	XtPointer		call_data	/* unused */
)
{
	const FileChooserShellWidget	fcs = (FileChooserShellWidget)wid;
	FileChooserShellPart*		my = &fcs->file_chooser_shell;
	Widget				focus_widget = NULL;

	XtVaGetValues(wid, XtNfocusWidget, &focus_widget, NULL);
	if (NULL == focus_widget)
		focus_widget = wid;

	if (my->pointer_warping) {
		Window			root_return;	/* unused */
		Window			child_return;	/* unused */
		int			win_x_return;	/* unused */
		int			win_y_return;	/* unused */
		unsigned int		mask_return;	/* unused */

		if (!XtIsRealized(wid))
			XtRealizeWidget(wid);

		/* save pointer position for later restoral */
		(void) XQueryPointer(XtDisplay(wid), XtWindow(wid), 
			&root_return, &child_return,
			&my->root_x, &my->root_y, 
			&win_x_return, &win_y_return, 
			&mask_return);

		/*
		 * StructureNotifyMask must be selected for by PopupWindowSehll
		 * widget: so use Raw.
		 */
		XtAddRawEventHandler(wid, StructureNotifyMask, FALSE, 
			warp_pointer, (XtPointer)focus_widget);

		my->pointer_warped = TRUE;
	}
}/* pop_up_cb() */


/************************************************************************
 *
 *  pop_down_cb -- 
 *
 ************************************************************************/

/*ARGSUSED*/
static void
pop_down_cb(
	Widget			wid,		/* unused */
	XtPointer		client_data,
	XtPointer		call_data	/* unused */
)
{
	const FileChooserShellWidget	fcs = 
					(FileChooserShellWidget)client_data;
	FileChooserShellPart*	my = &fcs->file_chooser_shell;

	if (my->pointer_warping && my->pointer_warped)
		/* restore position of pointer */
		XWarpPointer(XtDisplay(fcs), None,
			RootWindowOfScreen(XtScreen(fcs)), 0, 0, 0, 0,
			my->root_x, my->root_y);
} /* pop_down_cb() */


/************************************************************************
 *
 *  post_select_cb -- 
 *
 ************************************************************************/

/*ARGSUSED*/
static void
post_select_cb(
	Widget			wid,		/* unused */
	XtPointer		client_data,
	XtPointer		call_data	/* unused */
)
{

	_OlFileChShPopDown((Widget)client_data, FALSE);
} /* post_select_cb() */


/**************************************************************************
 *
 *	Event Handling Procedures
 *
 **************************************************************************/


/************************************************************************
 *
 *  warp_pointer -- 
 *
 ************************************************************************/

/*ARGSUSED3*/
static void
warp_pointer(
	Widget		widget,
	XtPointer	client_data,
	XEvent*		event,
	Boolean*	continue_to_dispatch	/* unused */
)
{
	const Widget		focus_widget = (Widget)client_data;
	int			dest_x,
				dest_y;
	Dimension		height,
				width;

	if (event->type != MapNotify ||	
			event->xmap.window != XtWindowOfObject(widget))
		return;

	XtVaGetValues(focus_widget, 
			XtNwidth,	&width, 
			XtNheight,	&height, 
		NULL);

	dest_x = width / 2;
	dest_y = (height >= 1 ? height / 2 : 0);

	if(_OlIsGadget(focus_widget)) { 
		/*
		 * Transform to parents coordinates, as a gadget does not have 
		 * its own window. 
		 */
		Position x, y;

		XtVaGetValues(focus_widget,
				XtNx,	&x, 
				XtNy,	&y, 
			NULL);

		dest_x += x;
		dest_y += y;
	}

	XWarpPointer(XtDisplay(widget), None, XtWindowOfObject(focus_widget), 
		0, 0, 0, 0, dest_x, dest_y);
        OlCallAcceptFocus(focus_widget, CurrentTime);

	XtRemoveRawEventHandler(widget, StructureNotifyMask, FALSE, warp_pointer, 
		client_data);
} /* end of warp_pointer() */


/**************************************************************************
 *
 *	Private Procedures
 *
 **************************************************************************/


/************************************************************************
 *
 *  callback_verify -- 
 * 
 ************************************************************************/

static Boolean
callback_verify(const Widget shellw)
{
	const FileChooserShellWidget	fcs = (FileChooserShellWidget)shellw;
	const FileChooserShellPart*	my = &fcs->file_chooser_shell;
	OlFileChShVerifyCallbackStruct	fcs_cdata = { OL_REASON_VERIFY, NULL };
		
	fcs_cdata.operation = my->operation;
	fcs_cdata.accept_verify = TRUE;

	XtCallCallbackList((Widget)fcs, my->verify_callback, &fcs_cdata);

	return fcs_cdata.accept_verify;
} /* end of callback_verify() */


/************************************************************************
 *
 *  post_header -- 
 * 
 ************************************************************************/

static void
post_header(const Widget wid)
{
	FileChooserShellWidget	fcsw = (FileChooserShellWidget)wid;
	FileChooserShellPart*	my = &(fcsw->file_chooser_shell);
	OlStr			top_header;
	String*			argv;
	String			label_resource;
	OlStr			label;

	XtVaGetValues(my->top_widget, XtNshellTitle, &top_header, NULL);
	
	/*! I18N !*/
	if (NULL == top_header || 0 == strcmp(top_header, "")) {
		XtVaGetValues(my->top_widget, XtNargv, &argv, NULL);
		top_header = argv[0];
	}	

	switch (my->operation) {
	case OL_OPEN:
		label_resource = XtNopenLabel;
		break;
	case OL_SAVE:
		label_resource = XtNsaveLabel;
		break;
	case OL_SAVE_AS:
		label_resource = XtNsaveAsLabel;
		break;
	case OL_INCLUDE:
		label_resource = XtNincludeLabel;
		break;
	case OL_DO_COMMAND:
		label_resource = XtNcommandLabel;
		break;
	}

	XtVaGetValues(my->file_chooser_widget, label_resource, &label, NULL);
	
	_OL_CALLOC(my->header, _OlStrlen(top_header) + 2 + _OlStrlen(label) + 1, 
		char);
	(void) strcpy(my->header, top_header);
	(void) strcat(my->header, ": ");
	(void) strcat(my->header, label);

	XtVaSetValues(wid, XtNshellTitle, my->header, NULL);
} /* end of post_header() */


/* end of FileChSh.c */
