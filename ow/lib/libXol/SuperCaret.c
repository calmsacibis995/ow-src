#pragma ident "@(#)SuperCaret.c	1.14	97/03/26	lib/libXol SMI" /* OLIT */

/*
 *        Copyright (C) 1986,1991  Sun Microsystems, Inc
 *                    All rights reserved.
 *          Notice of copyright on this source code 
 *          product does not indicate publication. 
 * 
 * RESTRICTED RIGHTS LEGEND: Use, duplication, or disclosure by 
 * the U.S. Government is subject to restrictions as set forth 
 * in subparagraph (c)(1)(ii) of the Rights in Technical Data
 * and Computer Software Clause at DFARS 252.227-7013 (Oct. 1988) 
 * and FAR 52.227-19 (c) (June 1987).
 *
 *    Sun Microsystems, Inc., 2550 Garcia Avenue,
 *    Mountain View, California 94043.
 *
 */

#include <Xol/OpenLookP.h>
#include <Xol/SuperCaretP.h>
#include <X11/CompositeP.h>
#include <X11/extensions/shape.h>

#include <Xol/PrimitiveP.h>
#include <Xol/EventObjP.h>
#include <Xol/ManagerP.h>

#include <Xol/VendorI.h>

#ifndef	ClassMethod
#define	ClassMethod	static
#endif

#ifndef	Private
#define	Private		static
#endif

#ifndef	ToolkitInternal
#define	ToolkitInternal
#endif

#ifndef	PublicInterface
#define	PublicInterface
#endif

/*
 * Core Class Methods
 */

ClassMethod	void	SCSClassPartInitialize(WidgetClass wc);

ClassMethod	void	SCSInitialize(Widget	reqw,
				      Widget	neww,
				      ArgList	args,
				      Cardinal*	num_args
			);

ClassMethod	void	SCSDestroy(Widget	w);

ClassMethod	void	SCSRealize(Widget			w,
				   XtValueMask*			mask,
				   XSetWindowAttributes*	attrs
			);

ClassMethod	Boolean	SCSSetValues(Widget	oldw,
				     Widget	reqw,
				     Widget	neww,
				     ArgList	args,
				     Cardinal*	num_args
			);

ClassMethod	void	SCSExpose(Widget	w,
				  XEvent*	event,
				  Region 	region
			);


/* 
 * Composite Class Methods
 */

ClassMethod	void	SCSChangeManaged(Widget	w);


ClassMethod	
XtGeometryResult	SCSGeometryManager(Widget		w,
					   XtWidgetGeometry*	req,
					   XtWidgetGeometry*	rep
			);

ClassMethod	void	SCSInsertChild(Widget	w);

/*
 * Constraint Class Methods
 */

/*
 * Shell (Extension) Class Methods
 */

ClassMethod	
XtGeometryResult	SCSRootGeometryManager(Widget			w,
					       XtWidgetGeometry*	req,
					       XtWidgetGeometry*	rep
			);

/*
 * SuperCaret Class Methods
 */

ClassMethod	void	ShapeSuperCaret(SuperCaretShellWidget	newscsw,
					SuperCaretShellWidget	oldscsw
			);

ClassMethod	void	UpdateSuperCaret(SuperCaretShellWidget	newscsw,
					 SuperCaretShellWidget	oldscsw
			);

ClassMethod	void	ConfigureSuperCaret(SuperCaretShellWidget scsw,
					    unsigned int	  request_mode,
					    const Boolean	  override
		 	);

ClassMethod	void	PopupSuperCaret(SuperCaretShellWidget scsw,
					const Boolean	      override
			);

ClassMethod	void	HandleSuperCaretEvents(Widget	 w,
					       XtPointer clientd,
					       XEvent*	 event,
					       Boolean*	 continue_to_dispatch
			);

ClassMethod	void	HandleFocusWidgetEvents(Widget	  w,
					        XtPointer clientd,
					        XEvent*	  event,
					        Boolean*  continue_to_dispatch
			);

/*
 * Private definitions
 */

Private void	SetDefaultScale(Widget w, int closure, XrmValue* value);

Private void	SetDefaultVisual(Widget w, int closure, XrmValue* value);

/************************************************************************/
static unsigned char sc_bottom_bits[] = {
	0x00, 0x00, 0x60, 0x00, 0x60, 0x00, 0xf0, 0x00, 0xf0, 0x00, 0xf8, 0x01,
   	0xf8, 0x01, 0xfc, 0x03, 0xfc, 0x03, 0xfe, 0x07, 0x00, 0x00, 0x00, 0x00,
   	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
static unsigned char sc_bottom_mask_bits[] = {
	0x60, 0x00, 0xf0, 0x00, 0xf0, 0x00, 0xf8, 0x01, 0xf8, 0x01, 0xfc, 0x03,
   	0xfc, 0x03, 0xfe, 0x07, 0xff, 0x0f, 0xff, 0x0f, 0xff, 0x0f, 0x00, 0x00,
   	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
static unsigned int sc_bottom_width = 16;
static unsigned int sc_bottom_height = 16;
static unsigned int sc_bottom_mask_width = 16;
static unsigned int sc_bottom_mask_height = 16;

static unsigned char sc_left_bits[] = {
	0x00, 0x00, 0x02, 0x00, 0x0e, 0x00, 0x3e, 0x00, 0xfe, 0x00, 0xfe, 0x03,
   	0xfe, 0x03, 0xfe, 0x00, 0x3e, 0x00, 0x0e, 0x00, 0x02, 0x00, 0x00, 0x00,
   	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
static unsigned char sc_left_mask_bits[] = {
	0x07, 0x00, 0x0f, 0x00, 0x3f, 0x00, 0xff, 0x00, 0xff, 0x03, 0xff, 0x07,
   	0xff, 0x07, 0xff, 0x03, 0xff, 0x00, 0x3f, 0x00, 0x0f, 0x00, 0x07, 0x00,
   	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
static unsigned int sc_left_width = 16;
static unsigned int sc_left_height = 16;
static unsigned int sc_left_mask_width = 16;
static unsigned int sc_left_mask_height = 16;

static Dimension sc_left_clip_width = 11;
static Dimension sc_left_clip_height = 12;
static Dimension sc_bottom_clip_width = 12;
static Dimension sc_bottom_clip_height = 11 ;

Private	char	SCCMName[] = "XolSCCM";

/************************************************************************
 *
 *	Resources
 *
 ************************************************************************/

Private
XtResource	resources[] = {
	{
		XtNsupercaretShape, XtCSuperCaretShape, XtRInt,
		sizeof(unsigned int), 
		XtOffsetOf(SuperCaretShellRec,
		supercaret.supercaret_shape),
		XtRImmediate, (XtPointer)SuperCaretNone,
	},
	{
		XtNisShaped, XtCIsShaped, XtRBoolean, sizeof(Boolean),
		XtOffsetOf(SuperCaretShellRec, supercaret.is_shaped),
		XtRImmediate, (XtPointer)TRUE,
	},
	{
		XtNfocusWidget, XtCFocusWidget, XtRWidget, sizeof(Widget),
		XtOffsetOf(SuperCaretShellRec, supercaret.focus_widget),
		XtRImmediate, (XtPointer)NULL,
	},
	{
		XtNscale, XtCScale, XtROlScale, sizeof(unsigned int),
		XtOffsetOf(SuperCaretShellRec, supercaret.scale),
		XtRCallProc, (XtPointer)SetDefaultScale,
	},
	{
		XtNvisibility, XtCReadOnly, XtRInt, sizeof(unsigned int),
		XtOffsetOf(SuperCaretShellRec, supercaret.visibility),
		XtRImmediate, (XtPointer)VisibilityFullyObscured,
	},
	{
		XtNfocusVisibility, XtCReadOnly, XtRInt, sizeof(unsigned int),
		XtOffsetOf(SuperCaretShellRec,
			   supercaret.focus_visibility),
		XtRImmediate, (XtPointer)VisibilityIndeterminate,
	},
	{
		XtNunrealizeCallback, XtCCallback, XtRWidget,
		sizeof(XtCallbackList),
		XtOffsetOf(SuperCaretShellRec,
			   supercaret.unrealize_callbacks
		),
		XtRImmediate, (XtPointer)NULL,
	},
	{
		XtNforeground, XtCForeground, XtRPixel, sizeof(Pixel),
		XtOffsetOf(SuperCaretShellRec, supercaret.foreground),
		XtRString, XtDefaultForeground
	},
	{
		XtNvisual, XtCVisual, XtRVisual, sizeof(Visual*),
		XtOffsetOf(SuperCaretShellRec, shell.visual),
		XtRCallProc, (XtPointer)SetDefaultVisual
	},
};

/************************************************************************
 *
 *	Shell Class Extension
 *
 ************************************************************************/

Private
ShellClassExtensionRec	superCaretShellClassExtRec = {
	/* next_extension	*/ (XtPointer)NULL,
	/* record_type		*/ NULLQUARK,
	/* version		*/ XtShellExtensionVersion,
	/* record_size		*/ sizeof(ShellClassExtensionRec),
	/* root_geometry_manager*/ SCSRootGeometryManager
};

/************************************************************************
 *
 *	SuperCaretShell Class Record
 *
 ************************************************************************/

Private
SuperCaretShellClassRec superCaretShellRec = {
	{ /* CoreClassPart		*/
	  /* superclass			*/ (WidgetClass)&overrideShellClassRec,
	  /* class_name			*/ "SuperCaretShell",
	  /* widget_size		*/ sizeof(SuperCaretShellRec),
	  /* class_initialize		*/ (XtProc)NULL,
	  /* class_part_initialize	*/ SCSClassPartInitialize,
	  /* class_inited		*/ False,
	  /* initialize			*/ SCSInitialize,
	  /* initialize_hook		*/ (XtArgsProc)NULL,
	  /* realize			*/ SCSRealize,
	  /* actions			*/ (XtActionList)NULL,
	  /* num_actions		*/ (Cardinal)0,
	  /* resources			*/ resources,
	  /* num_resources		*/ XtNumber(resources),
	  /* xrm_class			*/ NULLQUARK,
	  /* compress_motion		*/ True,
	  /* compress_expose		*/ True,
	  /* compress_enterleave	*/ True,
	  /* visible_interest		*/ False,
	  /* destroy			*/ SCSDestroy,
	  /* resize			*/ (XtWidgetProc)NULL,
	  /* expose			*/ SCSExpose,
	  /* set_values			*/ SCSSetValues,
	  /* set_values_hook		*/ (XtArgsFunc)NULL,
	  /* set_values_almost		*/ XtInheritSetValuesAlmost,
	  /* get_values_hook		*/ (XtArgsProc)NULL,
	  /* accept_focus		*/ (XtAcceptFocusProc)NULL,
	  /* version			*/ XtVersion,
	  /* callback_private		*/ (XtPointer)NULL,
	  /* tm_table			*/ (String)NULL,
	  /* query_geometry		*/ (XtGeometryHandler)NULL,
	  /* display_accelerator	*/ (XtStringProc)NULL,
	  /* extension			*/ (XtPointer)NULL,
	},

	{ /* CompositeClassPart		*/
	  /* geometry_manager		*/ SCSGeometryManager,
	  /* change_managed		*/ SCSChangeManaged,
	  /* insert_child		*/ SCSInsertChild,
	  /* delete_child		*/ (XtWidgetProc)NULL,
	  /* extension			*/ (XtPointer)NULL,
	},

	{ /* ShellClassPart		*/
	  /* extension			*/ (XtPointer)&superCaretShellClassExtRec, 
	},

	{ /* OverrideShellClassPart	*/
	  /* extension			*/ (XtPointer)NULL,
	},

	{ /* SuperCaretShellClassPart	*/
	  /* shape_supercaret		*/ ShapeSuperCaret,
	  /* update_supercaret		*/ UpdateSuperCaret,
	  /* configure_supercaret	*/ ConfigureSuperCaret,
	  /* popup_supercaret		*/ PopupSuperCaret,
	  /* handle_supercaret_events	*/ HandleSuperCaretEvents,
	  /* supercaret_event_mask	*/ (VisibilityChangeMask),
	  /* handle_focus_widget_events	*/ HandleFocusWidgetEvents,
	  /* focus_widget_event_mask	*/ (VisibilityChangeMask | 
					    StructureNotifyMask
					   ),
	  /* extension			*/ (XtPointer)NULL,
	},
};

PublicInterface WidgetClass
superCaretShellWidgetClass = (WidgetClass)&superCaretShellRec;

/************************** Private Definitions **************************/

/*
 *
 * used by the configure and  popup  class methods to send client
 * messages to itself through the server to synchronize with certain event
 * processing by the client
 *
 * note that this structure is identical to an XClientMessage!
 */

typedef	struct	_SCClientMessageData {
	int		type;
	unsigned long	serial;
	Bool		send_event;
	Display*	display;
	Window		window;	
	Atom		message_type;
	int		format;
	
	long		action;
	long		supercaret;
	long		focus_widget;
	long		data1;
	long		data2;
} SCClientMessageData,  *SCClientMessageDataPtr;

#define	In(f, v1, v2)	f = ((((v1) << 16) & 0xffff0000) | ((v2) & 0xffff))
#define	Out(f, v1, v2)	v1 = (((f) >> 16) & 0xffff); v2 = ((f) & 0xffff)

#define	SCSWC_SCSC	scswc->supercaret_shell_class
#define	SUPER_SCSC	super->supercaret_shell_class

/************************** Private Functions ***************************/

/************************************************************************
 *
 *	ProcessSCClientMessages
 *
 ************************************************************************/

Private void
ProcessSCClientMessages(const SuperCaretShellWidget	scsw,
			XClientMessageEvent*		sccmevent)
{
	SuperCaretShellWidget		sscsw;
	SuperCaretShellPart*		sscsp;
	SCClientMessageDataPtr		sccmdatap;
	Widget				focus_widget;
	SuperCaretShellWidgetClass	scswc;

	if ((sccmdatap = (SCClientMessageDataPtr)sccmevent)->message_type !=
	    XInternAtom(sccmdatap->display, SCCMName, False)) return;

	sscsw = (SuperCaretShellWidget)(sccmdatap->supercaret);
	sscsp = &(scsw->supercaret);
	
	focus_widget = (Widget)(sccmdatap->focus_widget);

	if (sscsw->core.being_destroyed)
		return;

	scswc = (SuperCaretShellWidgetClass)XtClass((Widget)scsw);

	if (sccmdatap->action == (long)XtPopup) {
			(*SCSWC_SCSC.popup_supercaret)(scsw, (Boolean)True);
	} else {
		if (sccmdatap->action == (long)XConfigureWindow) {
			(*SCSWC_SCSC.configure_supercaret)
				(scsw,
				 sccmdatap->data1,
				 (Boolean)True
				);
		}
	}
}

/************************************************************************
 *
 *	_OlCallUpdateSuperCaret
 *
 ************************************************************************/

ToolkitInternal void
_OlCallUpdateSuperCaret(Widget	w, Widget	target)
{
	SuperCaretShellWidget	   scsw = (SuperCaretShellWidget)w;
	SuperCaretShellWidgetClass scswc;
	SuperCaretShellRec	   oldscsr;
	SuperCaretShellWidget	   oldscsw = scsw;
	Cardinal		   size;

	scswc = (SuperCaretShellWidgetClass)XtClass((Widget)scsw);

	if (scsw->supercaret.focus_widget != target) {
		size = scswc->core_class.widget_size;

		if (size > sizeof(SuperCaretShellRec))
			oldscsw = (SuperCaretShellWidget)XtMalloc(size);
		else
			oldscsw = &oldscsr;

		memcpy((char *)oldscsw, (char *)scsw, size);

		scsw->supercaret.focus_widget = target;
	}

	(*SCSWC_SCSC.update_supercaret)(scsw, oldscsw);

	if (oldscsw != &oldscsr && oldscsw != scsw)
		XtFree((char *)oldscsw);
}

/************************************************************************
 *
 *	GetQueryLocnProc
 *
 ************************************************************************/

Private SuperCaretQueryLocnProc
GetQueryLocnProc(const Widget target)
{
	SuperCaretQueryLocnProc	query_proc = (SuperCaretQueryLocnProc)NULL;

#define	GETQP(wct, wcp)	((wct)XtClass(target))->wcp.query_sc_locn_proc

	if (XtIsSubclass(target, primitiveWidgetClass))
		query_proc = GETQP(PrimitiveWidgetClass, primitive_class);
	else
		if (XtIsSubclass(target, eventObjClass))
			query_proc = GETQP(EventObjClass, event_class);
	else
		if(XtIsSubclass(target, managerWidgetClass))
			query_proc = GETQP(ManagerWidgetClass, manager_class);

	return (query_proc);
}

/************************************************************************
 *
 *	ShapeIt
 *
 ************************************************************************/

Private void
ShapeIt(SuperCaretShellWidget   scsw)
{
	Display *dpy;
	Window win;
	SuperCaretShellPart*	scsp = &(scsw->supercaret);
	int index;

	if (!(scsp->is_shaped && XtIsRealized((Widget)scsw)) ||
	    scsp->supercaret_shape == SuperCaretNone) return;

	dpy = XtDisplay(scsw);
	win = XtWindowOfObject((Widget)scsw);

	switch(scsp->supercaret_shape) {
		case SuperCaretBottom:
			if(scsp->sc_bottom_mask == NULL)
				scsp->sc_bottom_mask = 
					XCreateBitmapFromData(dpy, 
						win,
                                                (char *)sc_bottom_mask_bits,
                                                sc_bottom_mask_width,
                                                sc_bottom_mask_height);
			if(scsp->sc_bottom == NULL)
				scsp->sc_bottom = 
					XCreateBitmapFromData(dpy, 
						win,
                                              	(char *)sc_bottom_bits,
                                                sc_bottom_width,
                                                sc_bottom_height);
			XShapeCombineMask(dpy, win, 
				ShapeBounding, 0,0, scsp->sc_bottom_mask, ShapeSet);
			XShapeCombineMask(dpy, win, 
				ShapeClip, 0,0, scsp->sc_bottom, ShapeSet);
			break;
		case SuperCaretLeft:
			if(scsp->sc_left_mask == NULL)
				scsp->sc_left_mask = 
					XCreateBitmapFromData(dpy, 
						win,
                                                (char *)sc_left_mask_bits,
                                                sc_left_mask_width,
                                                sc_left_mask_height);
			if(scsp->sc_left == NULL)
				scsp->sc_left = 
					XCreateBitmapFromData(dpy, 
						win,
                                              	(char *)sc_left_bits,
                                                sc_left_width,
                                                sc_left_height);
			XShapeCombineMask(dpy, win, 
				ShapeBounding, 0,0, scsp->sc_left_mask, ShapeSet);
			XShapeCombineMask(dpy, win, 
				ShapeClip, 0, 0, scsp->sc_left, ShapeSet);
			break;
	}
			
}


/************************************************************************
 *
 *	NewGCs
 *
 ************************************************************************/

Private void
NewGCs(SuperCaretShellWidget scsw)
{
	SuperCaretShellPart*	scsp = &(scsw->supercaret);
	unsigned long		mask = (GCForeground | GCBackground);
	XGCValues		values;
	Pixel			bp   = OlBlackPixel((Widget)scsw),
				wp   = OlWhitePixel((Widget)scsw);

	if (scsp->gc != (GC)NULL) {
		XtReleaseGC((Widget)scsw, scsp->gc);
		scsp->gc = NULL;
	}

	values.foreground = bp;
	values.background = wp;
	scsp->gc = XtGetGC((Widget)scsw, mask, &values);

}


/************************************************************************
 *
 *	FocusWidgetDestroyed
 *
 ************************************************************************/

Private void	
FocusWidgetDestroyed(Widget w, XtPointer clientd, XtPointer calld)
{
	SuperCaretShellWidget scsw = (SuperCaretShellWidget)clientd;

	if (scsw->supercaret.focus_widget == w) {
		_OlCallUpdateSuperCaret((Widget)scsw, (Widget)NULL);
	}
}

/************************************************************************
 *
 *	SetDefaultScale
 *
 ************************************************************************/

Private void	
SetDefaultScale(Widget w, int closure, XrmValue* value)
{
	Widget	    ssw = _OlGetScreenShellOfWidget(w);
	static	int def = 9;

	value->size = sizeof(int);

	if (ssw != (Widget)NULL) {
		Arg	arg;
		
		XtSetArg(arg, XtNscale, &(def));
		XtGetValues(ssw, &arg, 1);
	}

	value->addr = (XtPointer)&def;
}

/************************************************************************
 *
 *	SetDefaultVisual
 *
 ************************************************************************/

Private void	
SetDefaultVisual(Widget w, int closure, XrmValue* value)
{
	static	Visual*	visual;

	visual = OlVisualOfObject(XtParent(w));

	if (visual = CopyFromParent)
		visual = DefaultVisualOfScreen(XtScreen(w));
	
	value->size = sizeof(Visual*);
	value->addr = (XtPointer)&visual;
}

/**************************** Class Methods *****************************/

/************************************************************************
 *
 *	SCSClassPartInitialize
 *
 ************************************************************************/

ClassMethod void
SCSClassPartInitialize(WidgetClass wc)
{
	SuperCaretShellWidgetClass	scswc,
					super;

	scswc = (SuperCaretShellWidgetClass)wc;
	super = (SuperCaretShellWidgetClass)wc->core_class.superclass;

#define	INHERIT(f, type)						\
	if (scswc->supercaret_shell_class.f == XtInherit ## type)	\
		scswc->supercaret_shell_class.f = 			\
			super->supercaret_shell_class.f

	INHERIT(shape_supercaret,SuperCaretShapeProc);
	INHERIT(update_supercaret,SuperCaretUpdateProc);
	INHERIT(configure_supercaret,SuperCaretConfigureProc);
	INHERIT(popup_supercaret,SuperCaretPopupProc);
#undef	INHERIT

	if (SCSWC_SCSC.handle_supercaret_events == (XtEventHandler)NULL) {
		SCSWC_SCSC.handle_supercaret_events =
			 SUPER_SCSC.handle_supercaret_events;
		SCSWC_SCSC.supercaret_event_mask =
			SUPER_SCSC.supercaret_event_mask;

	}
	if (SCSWC_SCSC.handle_focus_widget_events == (XtEventHandler)NULL) {
		SCSWC_SCSC.handle_focus_widget_events =
			SUPER_SCSC.handle_focus_widget_events;
		SCSWC_SCSC.focus_widget_event_mask =
			SUPER_SCSC.focus_widget_event_mask;

	}
}

/************************************************************************
 *
 *	SCSInitialize
 *
 ************************************************************************/

ClassMethod void
SCSInitialize(Widget	reqw,
	      Widget	neww,
	      ArgList	args,
	      Cardinal*	num_args)
{
	SuperCaretShellWidget		reqscsw = (SuperCaretShellWidget)reqw;
	SuperCaretShellWidget		newscsw = (SuperCaretShellWidget)neww;
	SuperCaretShellPart*		newscsp = &(newscsw->supercaret);
	SuperCaretShellPart*		reqscsp = &(reqscsw->supercaret);
	SuperCaretShellWidgetClass	scswc;
	Widget				t;

	scswc = (SuperCaretShellWidgetClass)XtClass(neww);

	newscsp->in_setvalues 	      = (Boolean)False;
	newscsp->in_update_supercaret = (Boolean)False;

	t = _OlGetDisplayShellOfWidget(neww);

	if (t != (Widget)NULL && newscsp->is_shaped == TRUE) {
		XtVaGetValues(t,
			      XtNshapeExtensionPresent,
			      &(newscsp->is_shaped),
			      NULL
		);
	}     

	newscsw->supercaret.pending_configures = 0;
	newscsw->supercaret.mapped	       = (Boolean)False;
	newscsw->supercaret.anticipated	       = VisibilityUnobscured;

	neww->core.x		= (Position)0;
	neww->core.y		= (Position)0;
	neww->core.width	= (Dimension)1;
	neww->core.height	= (Dimension)1;
	neww->core.border_width	= (Dimension)0;
	
	newscsw->supercaret.current_parent = XtParent(neww);

	newscsp->sc_left = 
	newscsp->sc_left_mask = 
	newscsp->sc_bottom = 
	newscsp->sc_bottom_mask =  
	newscsp->bitmap_left =
	newscsp->bitmap_bottom = 
	newscsp->left 		=
	newscsp->bottom		= NULL;

	newscsp->gc = (GC)NULL;
	if(newscsp->is_shaped == FALSE)
		NewGCs(newscsw);

	XtAddEventHandler(neww,
			  SCSWC_SCSC.supercaret_event_mask, 
			  !(newscsp->is_shaped),
			  SCSWC_SCSC.handle_supercaret_events,
			  (XtPointer)NULL
	);

	newscsp->actual_sc_scale = newscsp->scale;

	(*SCSWC_SCSC.update_supercaret)(newscsw, reqscsw);
 }

/************************************************************************
 *
 *	SCSDestroy
 *
 ************************************************************************/

ClassMethod void
SCSDestroy(Widget w)
{
	SuperCaretShellWidget		scsw = (SuperCaretShellWidget)w;
	SuperCaretShellPart*		scsp = &(scsw->supercaret);
	SuperCaretShellWidgetClass	scswc;
	Display				*dpy = XtDisplay(w);

	scswc = (SuperCaretShellWidgetClass)XtClass(w);

	if (scsp->gc != (GC)NULL)
		XtReleaseGC(w, scsp->gc);

	if(scsp->sc_bottom_mask != (Pixmap)NULL)
		XFreePixmap(dpy,scsp->sc_bottom_mask);

	if(scsp->sc_left != (Pixmap)NULL)
		XFreePixmap(dpy,scsp->sc_left);
	if(scsp->sc_left_mask != (Pixmap)NULL)
		XFreePixmap(dpy,scsp->sc_left_mask);

	if(scsp->sc_bottom != (Pixmap)NULL)
		XFreePixmap(dpy,scsp->sc_bottom);

	if(scsp->bottom != (Pixmap)NULL)
		XFreePixmap(dpy,scsp->bottom);
	if(scsp->left != (Pixmap)NULL)
		XFreePixmap(dpy,scsp->left);

	if(scsp->bitmap_bottom != (Pixmap)NULL)
		XFreePixmap(dpy,scsp->bitmap_bottom);
	if(scsp->bitmap_left != (Pixmap)NULL)
		XFreePixmap(dpy,scsp->bitmap_left);


	if (scsp->focus_widget != (Widget)NULL) {
		Widget	fw = scsp->focus_widget;

		XtRemoveCallback(fw,
				 XtNdestroyCallback,
				 FocusWidgetDestroyed,
				 (XtPointer)w
		);

		if (!XtIsWidget(fw) && XtIsRectObj(fw)) fw = XtParent(fw);

		XtRemoveEventHandler(fw,
				     SCSWC_SCSC.focus_widget_event_mask,
				     (Boolean)False,
				     SCSWC_SCSC.handle_focus_widget_events,
				     (XtPointer)w
		);
	}

	XtRemoveEventHandler(w,
			     SCSWC_SCSC.supercaret_event_mask,
			     !(scsp->is_shaped),
			     SCSWC_SCSC.handle_supercaret_events,
			     (XtPointer)NULL
	);
}

/************************************************************************
 *
 *	SCSRealize
 *
 ************************************************************************/

ClassMethod void
SCSRealize(Widget			w,
	   XtValueMask*			mask,
	   XSetWindowAttributes*	attrs)
{
	SuperCaretShellWidget	    scsw   = (SuperCaretShellWidget)w;
	SuperCaretShellPart*	    scsp   = &(scsw->supercaret);
	SuperCaretShellWidgetClass  scswc;
	Window win = (Window)NULL;
	Display *dpy = XtDisplay(w);

	scswc = (SuperCaretShellWidgetClass)XtClass(w);

	if (scsp->focus_widget == NULL)
		return;

	*mask |= CWOverrideRedirect;
	attrs->override_redirect  = scsw->shell.override_redirect;

	*mask |= CWSaveUnder;
	attrs->save_under  = scsw->shell.save_under;

	*mask |= CWEventMask;
	attrs->event_mask = XtBuildEventMask(w);

	*mask |= CWColormap;
	attrs->colormap = w->core.colormap;

	if (scsp->is_shaped) {
		*mask |= (CWBackPixel | CWBorderPixel);
		attrs->background_pixel = OlBlackPixel(w);
		attrs->border_pixel = OlWhitePixel(w);
	} else {
		*mask |= CWBackPixmap;
		attrs->background_pixmap = None;
		*mask &= ~(CWBackPixel);
	}

	if (!XtIsRealized(scsp->current_parent))
		XtRealizeWidget(scsp->current_parent);

	win = scsw->core.window = XCreateWindow(XtDisplay(w),
				          XtWindow(scsp->current_parent),
					  w->core.x, w->core.y,
					  w->core.width, w->core.height,
					  w->core.border_width,
					  w->core.depth,
					  InputOutput,
					  scsw->shell.visual,
					  *mask,
					  attrs
			    );

	if(scsp->is_shaped == FALSE) {
		scsp->bitmap_left = XCreateBitmapFromData(dpy, win,
                       		                (char *)sc_left_mask_bits,
                               		        sc_left_mask_width,
                                       		sc_left_mask_height);

		scsp->bitmap_bottom = XCreateBitmapFromData(dpy, win,
                                       (char *)sc_bottom_mask_bits,
                                       sc_bottom_mask_width,
                                       sc_bottom_mask_height);
				
	}

	if(scsp->is_shaped)
		ShapeIt(scsw);
}

/************************************************************************
 *
 *	SCSSetValues
 *
 ************************************************************************/

ClassMethod Boolean
SCSSetValues(Widget	oldw,
	     Widget	reqw,
	     Widget	neww,
	     ArgList	args,
	     Cardinal*	num_args)
{
#define	Changed(f)		(neww->f != oldw->f)
#define	ReadOnly(f)		(neww->f = oldw->f)

	SuperCaretShellWidget	   oldscsw = (SuperCaretShellWidget)oldw;
	SuperCaretShellPart*	   oldscsp = &(oldscsw->supercaret);
	SuperCaretShellWidget	   newscsw = (SuperCaretShellWidget)neww;
	SuperCaretShellPart*	   newscsp = &(newscsw->supercaret);
	SuperCaretShellWidgetClass scswc;
	Boolean			   redisplay  = (Boolean)False;

	newscsp->in_setvalues = (Boolean)True;

	scswc = (SuperCaretShellWidgetClass)XtClass(neww);

	ReadOnly(core.x);
	ReadOnly(core.y);
	ReadOnly(core.width);
	ReadOnly(core.height);
	ReadOnly(core.border_width);

	/* undo CoreSetValues */

	if (XtIsRealized(neww)) {
		Mask			window_mask = 0;
		XSetWindowAttributes	attrs;

		if(Changed(core.colormap)) {
			window_mask    |= CWColormap;
			attrs.colormap  = ReadOnly(core.colormap);
		}
		if(Changed(core.border_pixel)) {
			window_mask	   |= CWBorderPixel;
			attrs.border_pixel  = ReadOnly(core.border_pixel);
		}
		if(Changed(core.border_pixmap)) {
			window_mask         |= CWBorderPixmap;
			attrs.border_pixmap  = None;
		}
		if(Changed(core.background_pixel)) {
			window_mask	       |= CWBackPixel;
			attrs.background_pixel  = 
					ReadOnly(core.background_pixel);
		}
		if(Changed(core.background_pixmap)) {
			window_mask    	       |= CWBackPixmap;
			attrs.background_pixel  = None;
		}

		if (window_mask != 0) {
			XChangeWindowAttributes(XtDisplay(neww),
					        XtWindow(neww),
					        window_mask,
					        &attrs
			);
		}
	} else {
		ReadOnly(core.colormap);
		ReadOnly(core.border_pixel);
		ReadOnly(core.background_pixel);
	}

	ReadOnly(core.background_pixmap);
	ReadOnly(core.border_pixmap);

	(*SCSWC_SCSC.update_supercaret)(newscsw, oldscsw);

	newscsp->in_setvalues = (Boolean)False;

	return (redisplay);

#undef	Changed
#undef	ReadOnly
}

/************************************************************************
 *
 *	SCSExpose
 *
 ************************************************************************/

ClassMethod void
SCSExpose(Widget	w,
	  XEvent*	event,
	  Region 	region)
{
	SuperCaretShellWidget	scsw = (SuperCaretShellWidget)w;
	SuperCaretShellPart*	scsp = &(scsw->supercaret);
	Display *dpy = XtDisplay(w);
	Window win = XtWindow(w);

	if (!scsp->is_shaped) {
		switch(scsp->supercaret_shape) {
			case SuperCaretBottom:
				if(scsp->bottom == (Pixmap)NULL) {
					scsp->bottom =
						XCreatePixmapFromBitmapData(dpy, win,
                                              		(char *)sc_bottom_bits,
                                                	sc_bottom_width,
                                                	sc_bottom_height,
                                                	OlBlackPixel((Widget)scsw),
                                                	OlWhitePixel((Widget)scsw),
							w->core.depth	
                                               		);
				}
					
				XSetClipMask(dpy, scsp->gc, scsp->bitmap_bottom);
				XCopyArea(XtDisplay(w),
						scsp->bottom,
						XtWindow(w),
						scsp->gc,
						0,0,
						w->core.width,
						w->core.height,
						0,0);
				break;
			case SuperCaretLeft:
				if(scsp->left == (Pixmap)NULL)	{
					scsp->left =
						XCreatePixmapFromBitmapData(dpy, win,
                                              		(char *)sc_left_bits,
                                                	sc_left_width,
                                                	sc_left_height,
                                                	OlBlackPixel((Widget)scsw),
                                                	OlWhitePixel((Widget)scsw),
							w->core.depth	
                                               		);
				}
				XSetClipMask(dpy, scsp->gc, scsp->bitmap_left);
				XCopyArea(XtDisplay(w),
						scsp->left,
						XtWindow(w),
						scsp->gc,
						0,0,
						w->core.width,
						w->core.height,
						0,0);
				break;

		}
	}
}

/************************************************************************
 *
 *	SCSChangeManaged
 *
 ************************************************************************/

ClassMethod void	
SCSChangeManaged(Widget	w)
{
	SuperCaretShellWidget	scsw = (SuperCaretShellWidget)w;
	unsigned int		i;

	for (i = 0; i < scsw->composite.num_children; i++)
		XtDestroyWidget(scsw->composite.children[i]);
}

/************************************************************************
 *
 *	SCSGeometryManager
 *
 ************************************************************************/

ClassMethod XtGeometryResult	
SCSGeometryManager(Widget		w,
		   XtWidgetGeometry*	req,
		   XtWidgetGeometry*	rep)
{
	SuperCaretShellWidget	scsw = (SuperCaretShellWidget)XtParent(w);
	XtGeometryResult res = XtGeometryNo;

	XtDestroyWidget(w);

	return (res);
}


/************************************************************************
 *
 *	SCSInsertChild
 *
 ************************************************************************/

ClassMethod void
SCSInsertChild(Widget	w)
{
	SuperCaretShellWidget	scsw = (SuperCaretShellWidget)XtParent(w);

	XtDestroyWidget(w);
}

/************************************************************************
 *
 *	SCSRootGeometryManager
 *
 ************************************************************************/

ClassMethod XtGeometryResult	
SCSRootGeometryManager(Widget			w,
		       XtWidgetGeometry*	req,
		       XtWidgetGeometry*	rep)
{
#define AQuery		(req->request_mode & XtCWQueryOnly)
#define	Requested(x)	((req->request_mode & (x)) == (x))

	SuperCaretShellWidget		scsw 	    = (SuperCaretShellWidget)w;
	SuperCaretShellPart*		scsp 	    = &(scsw->supercaret);
	SuperCaretShellWidgetClass	scswc;
	XtGeometryResult		res  	    = XtGeometryDone;

	scswc = (SuperCaretShellWidgetClass)XtClass(w);

	if (!XtIsRealized(w)) res = XtGeometryYes;

	if (!AQuery) {
		if (Requested(CWX)) 
			w->core.x = req->x;

		if (Requested(CWY))
			w->core.y = req->y;

		if (Requested(CWWidth))
			w->core.width = req->width;

		if (Requested(CWHeight))
			w->core.height = req->height;

	} else
		res = XtGeometryYes;

	if (Requested(CWBorderWidth)) {
		   req->border_width = w->core.border_width;

		res = XtGeometryAlmost;
	}

	if (Requested(CWStackMode)) {
		if (req->stack_mode != Above)
			res = XtGeometryAlmost;

		req->stack_mode = Above;

	}

	if (!AQuery && res == XtGeometryDone && req->request_mode != 0) {
		(*SCSWC_SCSC.configure_supercaret)
			((SuperCaretShellWidget)w,
			 req->request_mode,
			 (Boolean)False
			);
	}
		
	if (AQuery || res == XtGeometryAlmost) *rep = *req;

	return (res);

#undef	Requested
#undef	AQuery
}

/************************************************************************
 *
 *	ShapeSuperCaret
 *
 ************************************************************************/

ClassMethod void
ShapeSuperCaret(SuperCaretShellWidget	newscsw,
		SuperCaretShellWidget   oldscsw)
{
	Widget			w       = (Widget)newscsw;
	SuperCaretShellPart*	newscsp = &(newscsw->supercaret);
	SuperCaretShellPart*	oldscsp = &(oldscsw->supercaret);
	Dimension		width   = w->core.width,
				height  = w->core.height;
	Boolean			shape_changed = (newscsp->supercaret_shape !=
						 oldscsp->supercaret_shape);

	newscsp->actual_sc_scale = newscsp->scale;

	switch(newscsp->supercaret_shape) {
		case SuperCaretNone:
			break;
		case SuperCaretBottom:
			width = (Dimension)sc_bottom_clip_width;
			height = (Dimension)sc_bottom_clip_height;
			break;
		case SuperCaretLeft:
			width = (Dimension)sc_left_clip_width;
			height = (Dimension)sc_left_clip_height;
			break;
	}
		
	if (newscsp->supercaret_shape != SuperCaretNone) {
		if (!(newscsp->in_setvalues	     &&
		      newscsp->in_update_supercaret) &&
		    (width  != w->core.width         ||
		     height != w->core.height)) {
			XtWidgetGeometry req;
			XtGeometryResult res;

			req.request_mode = CWWidth | CWHeight;
			req.width	 = width;
			req.height	 = height;

			do {
				res = XtMakeGeometryRequest(w, &req, &req);
			} while (res != XtGeometryYes);
		} else {	/* in setvalues so this is o.k ... */
			w->core.width  = width;
			w->core.height = height;
		}

		if(newscsp->is_shaped)
			ShapeIt(newscsw);
	}
}

/************************************************************************
 *
 *	UpdateSuperCaret
 *
 ************************************************************************/

ClassMethod void
UpdateSuperCaret(SuperCaretShellWidget	newscsw, SuperCaretShellWidget  oldscsw)
{
	SuperCaretShellPart*		newscsp	     = &(newscsw->supercaret);
	SuperCaretShellPart*		oldscsp	     = &(oldscsw->supercaret);
	SuperCaretShellWidgetClass	scswc;
	SuperCaretQueryLocnProc	query_proc = NULL; 

	enum {
		NoOperation,
		ChangeFocus,
		GainFocus,
		LooseFocus,
		StateChange
	}			state_change = NoOperation;
			
	if(newscsp->focus_widget != NULL)
		query_proc = GetQueryLocnProc(newscsp->focus_widget);

	newscsp->focus_widget = (query_proc == NULL? 
							(Widget)NULL: newscsp->focus_widget);

	newscsp->in_update_supercaret = (Boolean)True;

	/* decide on the cause of the update */

	if (newscsp->focus_widget != (Widget)NULL &&
	    oldscsp->focus_widget == (Widget)NULL)
		state_change = GainFocus;
	else
		if (newscsp->focus_widget == (Widget)NULL &&
		    oldscsp->focus_widget != (Widget)NULL) {
			state_change = LooseFocus;
			if (XtIsRealized((Widget)newscsw))
			    XtPopdown((Widget)newscsw);
			newscsw->supercaret.mapped = (Boolean)False;
		} else if (newscsp->focus_widget != oldscsp->focus_widget)
				state_change = ChangeFocus;
			else
				if (newscsp->focus_widget != (Widget)NULL) {
					if (!newscsp->is_shaped) {
						if (XtIsRealized((Widget)newscsw))
						    XtPopdown((Widget)newscsw);
						newscsp->mapped = (Boolean)False;
					}

					state_change = StateChange;
				}

	if (state_change == NoOperation) {
		newscsp->in_update_supercaret = (Boolean)False;
		return;
	}

	scswc = (SuperCaretShellWidgetClass)XtClass((Widget)newscsw);

	if (state_change == ChangeFocus || state_change == LooseFocus) {
		Widget			ofw = oldscsp->focus_widget;

		XtRemoveCallback(ofw,
				 XtNdestroyCallback,
				 FocusWidgetDestroyed,
				 (XtPointer)newscsw
		);

		if (!XtIsWidget(ofw) && XtIsRectObj(ofw)) ofw = XtParent(ofw);

		XtRemoveEventHandler(ofw,
				     SCSWC_SCSC.focus_widget_event_mask,
				     (Boolean)False,
				     SCSWC_SCSC.handle_focus_widget_events,
				     (XtPointer)newscsw
		);

		newscsp->focus_visibility = VisibilityIndeterminate;
	}

	if (state_change != LooseFocus) {
		unsigned int			depth;
		Colormap			colormap;
		Visual*				visual;
		Boolean				needs_new_gcs = (Boolean)False;

		depth    = OlDepthOfObject(newscsp->focus_widget);
		colormap = OlColormapOfObject(newscsp->focus_widget);
		visual	 = OlVisualOfObject(newscsp->focus_widget);

#define	VISUALID(v)	XVisualIDFromVisual(v)
		if (state_change      != StateChange	      &&
		    (depth    	      != newscsw->core.depth  ||
		     newscsw->core.colormap == CopyFromParent ||
		     (newscsw->shell.visual == CopyFromParent ||
		      visual		    == CopyFromParent ||
		      VISUALID(visual)      != 
		      VISUALID(newscsw->shell.visual)))) {
			newscsw->core.depth    = depth;
			newscsw->core.colormap = colormap;
			newscsw->shell.visual  = visual;

			if (XtIsRealized((Widget)newscsw)) {
			    XtPopdown((Widget)newscsw);
			    XtUnrealizeWidget((Widget)newscsw);
			}
			newscsw->supercaret.mapped = (Boolean)False;
			needs_new_gcs              = (Boolean)True;
			newscsp->anticipated	   = VisibilityIndeterminate;
		}
#undef	VISUALID

		if (colormap != newscsw->core.colormap) {
			newscsw->core.colormap = colormap;
			needs_new_gcs	       = (Boolean)True;
		}

		{
			Position		x,
						x_center = (Position)0,
						y,
						y_center = (Position)0;
			Dimension		width,
						height;
			unsigned int		old_scale,
						scale;
			SuperCaretShape		old_shape;
			XtWidgetGeometry	req;
			XtGeometryResult	res;
			Boolean			done = (Boolean)False;


			req.request_mode = 0;

			scale = old_scale = newscsw->supercaret.scale;

			do {
				width     = newscsw->core.width;
				height    = newscsw->core.height;
				old_shape = newscsp->supercaret_shape;

				(*query_proc)(newscsp->focus_widget,
					      (Widget)newscsw,
					      newscsw->core.width,
					      newscsw->core.height,
					      &scale,
					      &(newscsp->supercaret_shape),
					      &x_center,
					      &y_center
				);

				newscsp->scale = scale;

				(*SCSWC_SCSC.shape_supercaret)
					    (newscsw, oldscsw);

				done = newscsp->supercaret_shape == 
				       	        SuperCaretNone		  ||
				       (width     == newscsw->core.width  &&
					height    == newscsw->core.height &&
					old_scale == scale	  	  && 
					old_shape == 
					        newscsp->supercaret_shape
				       );

				if (old_scale != newscsp->scale)
					old_scale = scale;
			} while (!done);

			if (newscsp->supercaret_shape == SuperCaretNone) {
				if(XtIsRealized((Widget)newscsw))
				    XtPopdown((Widget)newscsw);
				newscsp->in_update_supercaret = (Boolean)False;
				newscsw->supercaret.mapped = (Boolean)False;
				return;
			} else 
				if (state_change == ChangeFocus || 
						state_change == GainFocus) {
					XWindowAttributes	attrs;
					Widget			fw = newscsp->focus_widget;

					XtAddCallback(fw,
			      			XtNdestroyCallback,
			      			FocusWidgetDestroyed,
			      			(XtPointer)newscsw);

					if (!XtIsWidget(fw) && 
					    XtIsRectObj(fw)) fw = XtParent(fw);

					XtAddEventHandler(fw,
				  		SCSWC_SCSC.focus_widget_event_mask,
				  		(Boolean)False,
				  		SCSWC_SCSC.handle_focus_widget_events,
				  		(XtPointer)newscsw);

					XGetWindowAttributes(XtDisplay(fw),
				     			XtWindow(fw),
				     			&attrs);

					newscsp->focus_visibility = VisibilityIndeterminate;
					newscsp->map_state	  = attrs.map_state;
				}

			if (state_change != StateChange) {
				req.request_mode = CWStackMode;
				req.stack_mode   = Above;
			}

			{
			Position root_sx= 0,root_sy=0, root_x=0, root_y=0;
			Widget	shell = _OlGetShellOfWidget(newscsp->focus_widget);

			XtTranslateCoords(shell,0,0,&root_sx,&root_sy);
			XtTranslateCoords(newscsp->focus_widget,x_center,
						y_center,&root_x,&root_y);
			x = root_x - root_sx - newscsw->core.width/2;
			y = root_y - root_sy - newscsw->core.height/2;

			}

#define	Changed(f)	(newscsw->core.f != (f))

			if (!newscsp->in_setvalues &&
			    XtIsRealized((Widget)newscsw)) {
				if (Changed(x)) {
					req.request_mode |= CWX;
					req.x		  = x;
				}
				if (Changed(y)) {
					req.request_mode |= CWY;
					req.y		  = y;
				}
				if (Changed(width)) {
					req.request_mode |= CWWidth;
					req.width	  = 
						newscsw->core.width;
				}
				if (Changed(height)) {
					req.request_mode |= CWHeight;
					req.height	  = 
						newscsw->core.height;
				}
			} else {	/* in setvalues so this is ok */
				newscsw->core.x      = x;
				newscsw->core.y      = y;
			}
#undef	Changed


			if (!XtIsRealized((Widget)newscsw))
				XtRealizeWidget((Widget)newscsw);

			/*
			 * even if we are in setvalues we need to reparent
			 * and restack ....
			 */

			if (req.request_mode != 0) do {
				res = XtMakeGeometryRequest((Widget)newscsw,
							    &req, &req
				      );
			} while (res == XtGeometryAlmost);
		}

		if (!newscsp->is_shaped && needs_new_gcs) {
			NewGCs(newscsw);
		}

		if (state_change != ChangeFocus ||
		    (oldscsp->supercaret_shape == SuperCaretNone &&
		     newscsp->supercaret_shape != oldscsp->supercaret_shape)) {
			(*SCSWC_SCSC.popup_supercaret)(newscsw, (Boolean)False);
		}

	}
#undef	SCSChanged

	newscsp->in_update_supercaret = (Boolean)False;
}


/************************************************************************
 *
 *	PopupSuperCaret
 *
 ************************************************************************/

ClassMethod void
PopupSuperCaret(SuperCaretShellWidget	scsw,
		const Boolean		override)
{
	SuperCaretShellPart*	scsp = &(scsw->supercaret);

	if (scsp->is_shaped || override) {
		XtPopup((Widget)scsw, XtGrabNone);
		scsp->mapped      = (Boolean)True;
		scsp->anticipated = VisibilityUnobscured;
	} else {
		SCClientMessageData		sccmdata;
		Display*			dpy = XtDisplay((Widget)scsw);
		Window				win = XtWindow((Widget)scsw);
		SuperCaretShellWidgetClass      scswc;
 
                scswc = (SuperCaretShellWidgetClass)XtClass((Widget)scsw);

		sccmdata.type		= ClientMessage;
		sccmdata.display	= dpy;
		sccmdata.window		= win;
		sccmdata.format		= 32;
		sccmdata.message_type	= XInternAtom(dpy, SCCMName, False);
		sccmdata.action		= (long)XtPopup;
		sccmdata.supercaret	= (long)scsw;
		sccmdata.focus_widget	= (long)(scsp->focus_widget);
		sccmdata.data1		= (long)0;
		sccmdata.data2		= (long)0;

		XSendEvent(dpy, win, (Bool)True,
			   NoEventMask, (XEvent *)&sccmdata
		);
	}
}

/************************************************************************
 *
 *	ConfigureSuperCaret
 *
 ************************************************************************/

ClassMethod void
ConfigureSuperCaret(SuperCaretShellWidget	scsw,
		    unsigned int		request_mode,
		    const Boolean		override)
{
#define	Requested(f)	((request_mode & (f)) == (f))
	SuperCaretShellPart*	scsp = &(scsw->supercaret);
	XWindowChanges		changes;

	if (Requested(CWX))
		changes.x = scsw->core.x;
	else
		request_mode &= ~CWX;

	if (Requested(CWY))
		changes.y = scsw->core.y;
	else
		request_mode &= ~CWY;

	if (Requested(CWWidth))
		changes.width = scsw->core.width;

	if (Requested(CWHeight))
		changes.height = scsw->core.height;

	if (Requested(CWBorderWidth))
		changes.border_width = scsw->core.border_width;

	if (Requested(CWStackMode))
		changes.stack_mode = Above;
	else
		request_mode &= ~CWStackMode;

	if (scsp->is_shaped || override) {
		if (!(scsp->is_shaped) && scsp->mapped) {
			XtUnmapWidget((Widget)scsw);
			scsp->mapped      = (Boolean)False;
			scsp->anticipated = VisibilityUnobscured;
		}

		if (request_mode != 0) {
			XConfigureWindow(XtDisplay((Widget)scsw),
					 XtWindow((Widget)scsw),
					 request_mode,
					 &changes
			);
		}

		if (scsp->pending_configures)
			scsp->pending_configures--;

		if (!(scsp->is_shaped) &&
		    scsp->pending_configures == 0) {
			XtMapWidget((Widget)scsw);
			scsp->mapped	  = (Boolean)True;
			scsp->anticipated = VisibilityUnobscured;
		}
	} else {
		SCClientMessageData		sccmdata;
		Display*			dpy = XtDisplay((Widget)scsw);
		Window				win = XtWindow((Widget)scsw);
                SuperCaretShellWidgetClass      scswc;
 
                scswc = (SuperCaretShellWidgetClass)XtClass((Widget)scsw);

		sccmdata.type		= ClientMessage;
		sccmdata.display	= dpy;
		sccmdata.window		= win;
		sccmdata.format		= 32;
		sccmdata.message_type	= XInternAtom(dpy, SCCMName, False);
		sccmdata.action		= (long)XConfigureWindow;
		sccmdata.supercaret	= (long)scsw;
		sccmdata.focus_widget	= (long)(scsp->focus_widget);
		sccmdata.data1		= (long)(request_mode);
		sccmdata.data2		= (long)0;

		XSendEvent(dpy, win, (Bool)True,
			   NoEventMask, (XEvent *)&sccmdata
		);

		scsp->pending_configures++;
	}
#undef	Requested
}

/************************************************************************
 *
 *	HandleSuperCaretEvents
 *
 ************************************************************************/


ClassMethod void	
HandleSuperCaretEvents(Widget   w,     XtPointer closure,
		       XEvent*  event, Boolean*  continue_to_dispatch)
{
	SuperCaretShellWidget	scsw = (SuperCaretShellWidget)w;
	SuperCaretShellPart*	scsp = &(scsw->supercaret);

	if (event->type		      ==  VisibilityNotify &&
	    event->xvisibility.window == XtWindow((Widget)scsw)) {
			scsp->visibility = event->xvisibility.state;
	} else if (event->type == ClientMessage && 
		   event->xany.window == XtWindow((Widget)scsw)) {
			ProcessSCClientMessages(scsw, &(event->xclient));
	}
}

/************************************************************************
 *
 *	HandleFocusWidgetEvents
 *
 ************************************************************************/


ClassMethod void	
HandleFocusWidgetEvents(Widget  w,     XtPointer  closure,
		       XEvent*  event, Boolean*   continue_to_dispatch)
{
	SuperCaretShellWidget	scsw = (SuperCaretShellWidget)closure;
	SuperCaretShellPart*	scsp = &(scsw->supercaret);
	int			prev_fw_vis,
				prev_fw_ms;

	if (scsp->focus_widget != w	     ||
	    (!XtIsWidget(scsp->focus_widget) &&
	     XtIsRectObj(scsp->focus_widget) &&
	     XtParent(scsp->focus_widget) != w)) return;

	prev_fw_vis = scsp->focus_visibility;
	prev_fw_ms  = scsp->map_state;

	if (event->type		      ==  VisibilityNotify &&
	    event->xvisibility.window == XtWindowOfObject(scsp->focus_widget)) {
			if (event->xvisibility.state == VisibilityFullyObscured)
				scsp->map_state = IsUnviewable;
			else
				scsp->map_state = IsViewable;
			scsp->focus_visibility = event->xvisibility.state;
	} else if (event->type == MapNotify) {
		if (scsp->focus_visibility == VisibilityUnobscured ||
		    scsp->focus_visibility == VisibilityPartiallyObscured)
			scsp->map_state = IsViewable;
		else
			scsp->map_state = IsUnviewable;
	} else if (event->type == UnmapNotify) {
		scsp->map_state = IsUnmapped;
		if (XtIsRealized((Widget)scsw))
		    XtPopdown((Widget)scsw);
		scsp->mapped = (Boolean)False;
	}

	if (((prev_fw_vis	    != scsp->focus_visibility)  &&
	     scsp->focus_visibility != VisibilityFullyObscured)	&&
	    ((prev_fw_ms  	    != scsp->map_state)	      	&&
	     scsp->map_state 	    == IsViewable)) {
		SuperCaretShellWidgetClass	scswc;

		scswc = (SuperCaretShellWidgetClass)XtClass((Widget)scsw);

		(*SCSWC_SCSC.popup_supercaret)(scsw, (Boolean)False);
	}
}

/************************** Public Functions ****************************/

/************************************************************************
 *
 *	_OlCreateSuperCaret
 *
 ************************************************************************/

extern	WidgetClass menuShellWidgetClass;
 
ToolkitInternal Widget
_OlCreateSuperCaret(const Widget vendor,
		    const ArgList args,
		    const Cardinal num_args)
{
	Widget	sc = (Widget)NULL;

	if (XtIsSubclass(vendor, vendorShellWidgetClass))
		sc = XtCreatePopupShell("supercaret",
					superCaretShellWidgetClass,
				        vendor, NULL, (Cardinal)0
		     );

	return (sc);
}

/************************************************************************
 *
 *	_OlGetSuperCaret
 *
 ************************************************************************/

ToolkitInternal Widget
_OlGetSuperCaret(const Widget vendor)
{
	if (XtIsSubclass(vendor, vendorShellWidgetClass) ||
	    XtIsSubclass(vendor, menuShellWidgetClass)) {
		unsigned int i;
		Widget	     w;

		for (i = 0; i < vendor->core.num_popups; i++)
			if (XtIsSubclass((w = vendor->core.popup_list[i]),
					 superCaretShellWidgetClass))
				return(w);
	}

	return ((Widget)NULL);
}

/************************************************************************
 *
 *	OlSetSuperCaretFocusWidget
 *	OlConfiguredWidget
 *	OlMoveWidget
 *	OlResizeWidget
 *	OlConfigureWidget
 *
 ************************************************************************/

PublicInterface void
OlSetSuperCaretFocusWidget(const Widget	new_focus_widget)
{
	Widget	w;

	if ((w = _OlGetShellOfWidget(new_focus_widget)) != (Widget)NULL) {
                OlVendorPartExtension ext_part;
                SuperCaretShellWidget   scsw;
 
                ext_part = _OlGetVendorPartExtension(w);
 
                if (ext_part != (OlVendorPartExtension)NULL) {               
                    scsw = (SuperCaretShellWidget)ext_part->supercaret;
 
                    if (scsw != (SuperCaretShellWidget)NULL)
                                _OlCallUpdateSuperCaret((Widget)scsw, new_focus_widget);
		}
	}
}

PublicInterface void
OlWidgetConfigured(const Widget	w)
{
	Widget shell;

	if ((shell = _OlGetShellOfWidget(w)) != (Widget)NULL) {
		OlVendorPartExtension ext_part;
		SuperCaretShellWidget	scsw;

		ext_part = _OlGetVendorPartExtension(shell);

		if (ext_part != (OlVendorPartExtension)NULL) {
	            scsw = (SuperCaretShellWidget)ext_part->supercaret;

		    if (scsw != (SuperCaretShellWidget)NULL &&
			w == scsw->supercaret.focus_widget)
				_OlCallUpdateSuperCaret((Widget)scsw, w);
		}
	}
}

#define	Unchanged(f)	old ## f == f

PublicInterface void
OlMoveWidget(const Widget   w,
	     const Position x,
	     const Position y)
{
	RectObj	 ro     = (RectObj)w;
	Position oldx   = ro->rectangle.x;
	Position oldy   = ro->rectangle.y;
	Widget	 shell;

	XtConfigureWidget(w,
			  x,
			  y,
			  ro->rectangle.width,
			  ro->rectangle.height,
			  ro->rectangle.border_width
	);

	if (Unchanged(x) && Unchanged(y)) return;

	if ((shell = _OlGetShellOfWidget(w)) != (Widget)NULL) {
                OlVendorPartExtension ext_part;
                SuperCaretShellWidget   scsw;
 
                ext_part = _OlGetVendorPartExtension(shell);
 
                if (ext_part != (OlVendorPartExtension)NULL) {               
                    scsw = (SuperCaretShellWidget)ext_part->supercaret;
 
                    if (scsw != (SuperCaretShellWidget)NULL &&
                        w == scsw->supercaret.focus_widget)
                                _OlCallUpdateSuperCaret((Widget)scsw, w);
                }
	}
}

PublicInterface void
OlResizeWidget(const Widget    w,
	       const Dimension width,
	       const Dimension height,
	       const Dimension border_width)
{
	RectObj	  ro		  = (RectObj)w;
	Dimension oldwidth	  = ro->rectangle.width;
	Dimension oldheight	  = ro->rectangle.height;
	Dimension oldborder_width = ro->rectangle.border_width;
	Widget	  shell;

	XtConfigureWidget(w,
			  ro->rectangle.x,
			  ro->rectangle.y,
			  width,
			  height,
			  border_width
	);

	if (Unchanged(width)	&&
	    Unchanged(height)	&& 
	    Unchanged(border_width)) return;

	if ((shell = _OlGetShellOfWidget(w)) != (Widget)NULL) {
                OlVendorPartExtension ext_part;
                SuperCaretShellWidget   scsw;
 
                ext_part = _OlGetVendorPartExtension(shell);
 
                if (ext_part != (OlVendorPartExtension)NULL) {               
                    scsw = (SuperCaretShellWidget)ext_part->supercaret;
 
                    if (scsw != (SuperCaretShellWidget)NULL &&
                        w == scsw->supercaret.focus_widget)
                                _OlCallUpdateSuperCaret((Widget)scsw, w);
                }
	}
}

PublicInterface void
OlConfigureWidget(const Widget	  w,
		  const Position  x,
		  const Position  y,
		  const Dimension width,
		  const Dimension height,
		  const Dimension border_width)
{
	RectObj	  ro		  = (RectObj)w;
	Position  oldx		  = ro->rectangle.x;
	Position  oldy		  = ro->rectangle.y;
	Dimension oldwidth	  = ro->rectangle.width;
	Dimension oldheight	  = ro->rectangle.height;
	Dimension oldborder_width = ro->rectangle.border_width;
	Widget	  shell;

	XtConfigureWidget(w, x, y, width, height, border_width);


	if (Unchanged(x)	&&
	    Unchanged(y)	&&
	    Unchanged(width)	&&
	    Unchanged(height)	&& 
	    Unchanged(border_width)) return;

	if ((shell = _OlGetShellOfWidget(w)) != (Widget)NULL) {
                OlVendorPartExtension ext_part;
                SuperCaretShellWidget   scsw;
 
                ext_part = _OlGetVendorPartExtension(shell);
 
                if (ext_part != (OlVendorPartExtension)NULL) {               
                    scsw = (SuperCaretShellWidget)ext_part->supercaret;
 
                    if (scsw != (SuperCaretShellWidget)NULL &&
                        w == scsw->supercaret.focus_widget)
                                _OlCallUpdateSuperCaret((Widget)scsw, w);
                }
	}
}

#undef	Unchanged
