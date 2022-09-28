#pragma ident	"@(#)Stub.c	302.6	97/03/26 lib/libXol SMI"	/* stub:src/Stub.c	1.24	*/

/*
 *	Copyright (C) 1986,1991  Sun Microsystems, Inc
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
 * 		OPEN LOOK(TM) stub widget source code.  This widget
 * is a method-driven widget, i.e., an application programmer can
 * set the class procedures via the XtSetValues() procedure.  This
 * widget is extremely useful to prototype other widgets.
 *
 ******************************file*header********************************
 */


#include <X11/IntrinsicP.h>
#include <X11/EventI.h>
#include <X11/StringDefs.h>

#include <Xol/Error.h>
#include <Xol/OpenLookP.h>
#include <Xol/StubP.h>


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

static void	_Realize(Widget w, XtValueMask *value_mask, XSetWindowAttributes *attributes);		/* default realize procedure	*/
static void	_Initialize(Widget request, Widget new);		/* default initialize proc.	*/
static void	GetWindowAttributes(Widget w);	/* gets X Window attributes	*/
static long	SetEventMask(Widget w, Boolean have_exposures);	/* modifies the event Mask of
					 * the widget			*/

					/* class procedures		*/

static void	Destroy(Widget w);		/* destroys an instance		*/
static void	GetValuesHook(Widget w, ArgList args, Cardinal *num_args);	/* getting sub data attributes	*/
static void	Initialize(Widget request, Widget new, ArgList args, Cardinal *num_args);		/* initializes an instance	*/
static void	InitializeHook(Widget w, ArgList args, Cardinal *num_args);	/* initializes sub data		*/
static XtGeometryResult QueryGeometry(Widget w, XtWidgetGeometry *intended, XtWidgetGeometry *perferred_return); /* Preferred geometry		*/
static void	Realize(Widget w, XtValueMask *value_mask, XSetWindowAttributes *attributes);		/* realizes an instance		*/
static void	Redisplay(Widget w, XEvent *xevent, Region region);		/* instance exposure handling	*/
static void	Resize(Widget w);		/* resizes an instance		*/
static Boolean SetValues(Widget current, Widget request, Widget new, ArgList args, Cardinal *num_args);		/* setting instance attributes	*/
static void	SetValuesAlmost(Widget w, Widget new_widget, XtWidgetGeometry *request, XtWidgetGeometry *reply);	/* conflict resolution for
					 * setting instance attributes	*/
static Boolean SetValuesHook(Widget w, ArgList args, Cardinal *num_args);		/* setting sub data attributes	*/
static Widget  TraversalHandler(Widget shell, Widget w, OlDefine direction, Time time);	/* handles traversal	        */
static void 	StubQuerySCLocnProc( 	/* query_sc_locn_proc */
					const Widget           target,
					const Widget            supercaret,
                			const Dimension         sc_width,
                			const Dimension         sc_height,
                			unsigned int    *const  scale,
                			SuperCaretShape *const  shape,
                			Position        *const  x_center_return,
                			Position        *const  y_center_return);

					/* Activate procedure */
static Boolean Activate (Widget, OlVirtualName, XtPointer);
static Boolean AcceptFocus(Widget w, Time *timestamp);		/* AcceptFocus procedure */
static Widget RegisterFocus(Widget w);		/* RegisterFocus procedure */
static void HighlightHandler(Widget w, OlDefine highlight_type);		/* Highlight Handler */

					/* action procedures		*/
/* There are no action procedures */

					/* public procedures		*/

/* There are no public procedures */

/*
 *************************************************************************
 *
 * Define global/static variables and #defines, and
 * Declare externally referenced variables
 *
 *****************************file*variables******************************
 */

#define OFFSET(field) XtOffset(StubWidget, field)

/*
 *************************************************************************
 *
 * Define Translations and Actions
 *
 ***********************widget*translations*actions***********************
 */

/* There are no default translations or actions for this widget */

/*
 *************************************************************************
 *
 * Define Resource list associated with the Widget Instance
 *
 ****************************widget*resources*****************************
 */

static XtResource
resources[] = {
				/* Turn traversal off by default since
				 * there's no natural user feedback	*/
   { XtNtraversalOn, XtCTraversalOn, XtRBoolean, sizeof(Boolean),
	OFFSET(primitive.traversal_on), XtRImmediate, (XtPointer)False },

   { XtNwindow, XtCWindow, XtRWindow, sizeof(Window),
	OFFSET(stub.window), XtRWindow, (XtPointer) NULL },

   { XtNreferenceStub, XtCReferenceStub, XtRWidget, sizeof(Widget),
	OFFSET(stub.reference_stub), XtRWidget, (XtPointer) NULL },

   { XtNdestroy, XtCDestroy, XtRFunction, sizeof(XtWidgetProc),
	OFFSET(stub.destroy), XtRFunction, (XtPointer) NULL },

   { XtNexpose, XtCExpose, XtRFunction, sizeof(XtExposeProc),
	OFFSET(stub.expose), XtRFunction, (XtPointer) NULL },

   { XtNgetValuesHook, XtCGetValuesHook, XtRFunction, sizeof(XtArgsProc),
	OFFSET(stub.get_values_hook), XtRFunction, (XtPointer) NULL },

   { XtNinitialize, XtCInitialize, XtRFunction, sizeof(XtInitProc),
	OFFSET(stub.initialize), XtRImmediate, (XtPointer) _Initialize },

   { XtNinitializeHook, XtCInitializeHook, XtRFunction, sizeof(XtArgsProc),
	OFFSET(stub.initialize_hook), XtRFunction, (XtPointer) NULL },

   { XtNrealize, XtCRealize, XtRFunction, sizeof(XtRealizeProc),
	OFFSET(stub.realize), XtRImmediate, (XtPointer) _Realize },

   { XtNresize, XtCResize, XtRFunction, sizeof(XtWidgetProc),
	OFFSET(stub.resize), XtRFunction, (XtPointer) NULL },

   { XtNsetValues, XtCSetValues, XtRFunction, sizeof(XtSetValuesFunc),
	OFFSET(stub.set_values), XtRFunction, (XtPointer) NULL },

   { XtNsetValuesAlmost, XtCSetValuesAlmost, XtRFunction,
	sizeof(XtAlmostProc), OFFSET(stub.set_values_almost),
	XtRFunction, (XtPointer) NULL },

   { XtNsetValuesHook, XtCSetValuesHook, XtRFunction, sizeof(XtArgsFunc),
	OFFSET(stub.set_values_hook), XtRFunction, (XtPointer) NULL },

   { XtNqueryGeometry, XtCQueryGeometry, XtRFunction,
	sizeof(XtGeometryHandler), OFFSET(stub.query_geometry),
	XtRFunction, (XtPointer) NULL },

   { XtNactivateFunc, XtCActivateFunc, XtRFunction, sizeof(OlActivateFunc),
	OFFSET(stub.activate), XtRFunction, (XtPointer) NULL },

   { XtNacceptFocusFunc, XtCAcceptFocusFunc, XtRFunction, sizeof(XtAcceptFocusProc),
	OFFSET(stub.accept_focus), XtRFunction, (XtPointer) NULL },

   { XtNhighlightHandlerProc, XtCHighlightHandlerProc, XtRFunction,
	sizeof(OlHighlightProc), OFFSET(stub.highlight), XtRFunction,
	(XtPointer) NULL },

   { XtNregisterFocusFunc, XtCRegisterFocusFunc, XtRFunction,
	sizeof(OlRegisterFocusFunc), OFFSET(stub.register_focus), XtRFunction,
	(XtPointer) NULL },

   { XtNtraversalHandlerFunc, XtCTraversalHandlerFunc, XtRFunction,
	sizeof(OlTraversalFunc), OFFSET(stub.traversal_handler),
	XtRFunction, (XtPointer) NULL },

};

/*
 *************************************************************************
 *
 * Define Class Record structure to be initialized at Compile time
 *
 ***************************widget*class*record***************************
 */

StubClassRec
stubClassRec = {
  {
    (WidgetClass) &primitiveClassRec,	/* superclass		  */	
    "Stub",				/* class_name		  */
    sizeof(StubRec),			/* size			  */
    NULL,				/* class_initialize	  */
    NULL,				/* class_part_initialize  */
    FALSE,				/* class_inited		  */
    Initialize,				/* initialize		  */
    InitializeHook,			/* initialize_hook	  */
    Realize,				/* realize		  */
    NULL,				/* actions		  */
    NULL,				/* num_actions		  */
    resources,				/* resources		  */
    XtNumber(resources),		/* resource_count	  */
    NULLQUARK,				/* xrm_class		  */
    TRUE,				/* compress_motion	  */
    TRUE,				/* compress_exposure	  */
    TRUE,				/* compress_enterleave    */
    TRUE,				/* visible_interest	  */
    Destroy,				/* destroy		  */
    Resize,				/* resize		  */
    Redisplay,				/* expose		  */
    SetValues,				/* set_values		  */
    SetValuesHook,			/* set_values_hook	  */
    SetValuesAlmost,			/* set_values_almost	  */
    GetValuesHook,			/* get_values_hook	  */
    AcceptFocus,			/* accept_focus		  */
    XtVersion,				/* version		  */
    NULL,				/* callback_private	  */
    XtInheritTranslations,		/* tm_table		  */
    QueryGeometry,			/* query_geometry	  */
    NULL,				/* display_accelerator	  */
    NULL				/* extension		  */
  },	/* CoreClass fields initialization */
  {					/* primitive class     */
      NULL,				/* can_accept_focus    */
      (OlHighlightProc)HighlightHandler,/* highlight_handler   */
      (OlTraversalFunc)TraversalHandler,/* traversal_handler   */
      (OlRegisterFocusFunc)RegisterFocus,/* register_focus      */
      Activate,				/* activate            */
      NULL,				/* event_procs	       */
      0,				/* num_event_procs     */
      OlVersion,			/* version             */
      NULL,				/* extension           */
      {NULL, 0},			/* dyn data	       */
      NULL,				/* transparent_proc    */
      NULL,		/* query_sc_locn_proc  */
  },
  {
    NULL				/* field not used    	  */
  }  /* StubClass field initialization */
};

/*
 *************************************************************************
 *
 * Public Widget Class Definition of the Widget Class Record
 *
 *************************public*class*definition*************************
 */

WidgetClass stubWidgetClass = (WidgetClass) &stubClassRec;

/*
 *************************************************************************
 *
 * Private Procedures
 *
 ***************************private*procedures****************************
 */

/*
 *************************************************************************
 * _Realize - realizes a widget.  This involves creating the window for
 * the widget.  If a window is supplied, it is used as a window.  If
 * no window is supplied, one is created.
 ****************************procedure*header*****************************
 */
static void
_Realize(Widget w, XtValueMask *value_mask, XSetWindowAttributes *attributes)
{
	StubWidget	sw = (StubWidget) w;
	Window		window;

				/* If the application has not given us
				 * a window, create one			*/

	window		= sw->stub.window;
	sw->core.window = sw->stub.window;

	if (window == (Window) NULL) {
		XtCreateWindow(w, (unsigned int)InputOutput,
			(Visual *)CopyFromParent, *value_mask, attributes);
		return;
	}

				/* If the application has given us a
				 * window, make sure that no other
				 * widget owns this window		*/

	
	if (XtWindowToWidget(XtDisplay(w), window) != (Widget) NULL)
	{
		OlVaDisplayErrorMsg(XtDisplay(w), "", "", OleCOlToolkitError,
			"StubWidget Realize: Widget \"%s\" already \
owns window %lu (hex %lx)", XtWindowToWidget(XtDisplay(w), window)->core.name,
			window, window);
	}

	if (*value_mask & CWEventMask) {
		XSelectInput(XtDisplay(w), window, attributes->event_mask);
	}

} /* END OF _Realize() */

/*
 *************************************************************************
 * _Initialize - this is the default initialization procedure
 ****************************procedure*header*****************************
 */
/* ARGSUSED */
static void
_Initialize(Widget request, Widget new)
{
	GetWindowAttributes(new);
} /* END OF _Initialize() */

/*
 *************************************************************************
 * GetWindowAttributes -
 ****************************procedure*header*****************************
 */
static void
GetWindowAttributes(Widget w)
{
	XWindowAttributes xwa;
	Window		  window = ((StubWidget) w)->stub.window;

	if (window == (Window) NULL)
		return;

	XGetWindowAttributes(XtDisplay(w), window, &xwa);

	w->core.x		= (Position) xwa.x;
	w->core.y		= (Position) xwa.y;
	w->core.width		= (Dimension) xwa.width;
	w->core.height		= (Dimension) xwa.height;
	w->core.border_width	= (Dimension) xwa.border_width;

} /* END OF GetWindowAttributes() */

/*
 *************************************************************************
 * SetEventMask - this routine checks to see if the Stub Widget actually
 * needs to be checking for exposure events.  This explicit check is
 * necessary since the class part has an Expose procedure, even though
 * the widget instance part may not have an expose procedure.
 *
 * Note: this routine is only needed since the 'expose' class field is
 * non-NULL.  If the future the expose class field should be NULL and
 * exposure handling should be done with an event handler.  When this is
 * done, this procedure will not be needed.
 ****************************procedure*header*****************************
 */
static long
SetEventMask(Widget w, Boolean have_exposures)
	      	  			/* The Stub Widget		*/
	       	               		/* True if the Stub Widget has
					 * exposures selected		*/
{
	StubWidget	sw = (StubWidget) w;
        EventMask       mask = XtBuildEventMask(w);
 
        if (sw->stub.expose != (XtExposeProc) NULL)
                mask |= ExposureMask;
        else
                mask &= ~ExposureMask;

	if (mask & ExposureMask) {
		if (have_exposures == False && XtIsRealized(w) == True)
			XSelectInput(XtDisplay(w), XtWindow(w), (long)mask);
	}
	else if (XtIsRealized(w) == True) {
		XSelectInput(XtDisplay(w), XtWindow(w), (long)mask);
	}

	return((long)mask);

} /* END OF SetEventMask() */

/*
 *************************************************************************
 *
 * Class Procedures
 *
 ****************************class*procedures*****************************
 */

/*
 *************************************************************************
 * Destroy - 
 ****************************procedure*header*****************************
 */
static void
Destroy(Widget w)
{
	XtWidgetProc destroy = ((StubWidget) w)->stub.destroy;

	if (destroy != (XtWidgetProc) NULL)
		(*destroy)(w);
} /* END OF Destroy() */

/*
 *************************************************************************
 * GetValuesHook - 
 ****************************procedure*header*****************************
 */
static void
GetValuesHook(Widget w, ArgList args, Cardinal *num_args)
{
	XtArgsProc gvh = ((StubWidget) w)->stub.get_values_hook;

	if (gvh != (XtArgsProc) NULL)
		(*gvh)(w, args, num_args);
} /* END OF GetValuesHook() */

/*
 *************************************************************************
 * Initialize - Initializes the instance record fields and resolves any
 * conflicts between the fields.
 ****************************procedure*header*****************************
 */
/* ARGSUSED */
static void
Initialize(Widget request, Widget new, ArgList args, Cardinal *num_args)
{
	StubWidget	nw = (StubWidget) new;
	XtInitProc	initialize = nw->stub.initialize;
	Widget		ref = nw->stub.reference_stub;

					/* Take care of inheritance	*/

	if (ref != (Widget) NULL &&
	    XtIsSubclass(ref, stubWidgetClass) == (Boolean)True)
	{
		StubWidget rs = (StubWidget) ref;

		nw->stub.destroy		= rs->stub.destroy;
		nw->stub.expose			= rs->stub.expose;
		nw->stub.get_values_hook	= rs->stub.get_values_hook;
		nw->stub.initialize		= rs->stub.initialize;
		nw->stub.initialize_hook	= rs->stub.initialize_hook;
		nw->stub.realize		= rs->stub.realize;
		nw->stub.resize			= rs->stub.resize;
		nw->stub.set_values		= rs->stub.set_values;
		nw->stub.set_values_almost	= rs->stub.set_values_almost;
		nw->stub.set_values_hook	= rs->stub.set_values_hook;
		nw->stub.query_geometry		= rs->stub.query_geometry;
	}
					/* Now call the instance
					 * initialize procedure		*/

	if (initialize != (XtInitProc) NULL)
		(*initialize)(request, new,args,num_args);

} /* END OF Initialize() */

/*
 *************************************************************************
 * InitializeHook - 
 ****************************procedure*header*****************************
 */
static void
InitializeHook(Widget w, ArgList args, Cardinal *num_args)
{
	XtArgsProc initialize_hook = ((StubWidget) w)->stub.initialize_hook;

	if (initialize_hook != (XtArgsProc) NULL)
		(*initialize_hook)(w, args, num_args);
} /* END OF InitializeHook() */

/*
 *************************************************************************
 * QueryGeometry
 ****************************procedure*header*****************************
 */
static XtGeometryResult
QueryGeometry(Widget w, XtWidgetGeometry *intended, XtWidgetGeometry *perferred_return)
{
	XtGeometryResult (*qg)() = ((StubWidget) w)->stub.query_geometry;
	XtGeometryResult result = XtGeometryYes;

	if (qg != (XtGeometryHandler) NULL)
		result = (*qg)(w, intended, perferred_return);
	return(result);
} /* END OF QueryGeometry() */

/*
 *************************************************************************
 * Realize - realizes an instance
 ****************************procedure*header*****************************
 */
static void
Realize(Widget w, XtValueMask *value_mask, XSetWindowAttributes *attributes)
{
	XtRealizeProc realize = ((StubWidget) w)->stub.realize;

			/* Because we're using the 'expose' class procedure
			 * to handle exposures, we must check to see if the
			 * an Exposure event mask is actually needed.  When
			 * an event handler is used for exposure handling,
			 * this check is no longer needed.
			 */
	attributes->event_mask = SetEventMask(w, False);

	if (realize != (XtRealizeProc) NULL)
	{
		(*realize) (w, value_mask, attributes);
	}
	else
	{
		OlVaDisplayErrorMsg(XtDisplay(w), "", "", OleCOlToolkitError,
			"StubWidget: No Realize Proc. for widget \"%s\"",
			w->core.name);
	}
} /* END OF Realize() */

/*
 *************************************************************************
 * Redisplay - this routine handles exposures on the instance. In here
 * we check to see if we need to track exposure events.  We have to do
 * this often, since the addition of an event handler will cause the
 * Intrinsics to select on ExposureMask since every Stub widget has a
 * class expose procedure.  By checking to see if we need the ExposureMask
 * in here, we can guarantee that we only get one exposure before
 * we turn it off.
 *
 * Note: using the 'expose' class field (i.e., specifying this procedure as
 * the class' expose procedure) is sub-optimal since it causes the
 * Intrinsics to believe that evetn stub widget is interested in exposures.
 * This is not always true since some stub widgets may not have the XtNexpose
 * resource set and may not have added any exposure event handlers of their
 * own.  In this case, we must do as explained above (i.e., check to see if
 * we're really interested in exposures and turn the interest off if we're
 * not).
 * The correct way to deal with exposures is to add an event handler if the
 * XtNexpose resource is set.  The event handler would trap exposures and
 * call the application's procedures (specified with XtNexpose) after doing
 * appropriate exposure compression.  An additional resource should be
 * added (XtNcompression) to indicate the type of compression desired.
 * Also, a new class field should be added to the stub widget class.  This
 * new class field would be the exposure compression routine.
 ****************************procedure*header*****************************
 */
static void
Redisplay(Widget w, XEvent *xevent, Region region)
	          		/* Stub widget id			*/
	               		/* unused */
	               		/* unused */
{
	XtExposeProc expose = ((StubWidget) w)->stub.expose;

	if (expose != (XtExposeProc) NULL)
		(*expose)(w, xevent, region);
	else
		(void)SetEventMask(w, True);
} /* END OF Redisplay() */

/*
 *************************************************************************
 * Resize - 
 ****************************procedure*header*****************************
 */
static void
Resize(Widget w)
{
	XtWidgetProc resize = ((StubWidget) w)->stub.resize;

	if (resize != (XtWidgetProc) NULL)
		(*resize)(w);
} /* END OF Resize() */

/*
 *************************************************************************
 * SetValues - this updates the attributes of the Stub Widget.
 ****************************procedure*header*****************************
 */
/* ARGSUSED */
static Boolean
SetValues(Widget current, Widget request, Widget new, ArgList args, Cardinal *num_args)
	               		/* Current Stub widget		*/
	               		/* What the application wants		*/
	           		/* What the application gets, so far...	*/
	       		     
	          	         
{
	XtSetValuesFunc set_values = ((StubWidget)new)->stub.set_values;

	if (set_values != (XtSetValuesFunc) NULL)
		return((*set_values)(current, request, new, args, num_args));
	else
		return(False);
} /* END OF SetValues() */

/*
 *************************************************************************
 * SetValuesAlmost - set sub-widget data.
 ****************************procedure*header*****************************
 */
static void
SetValuesAlmost(Widget w, Widget new_widget, XtWidgetGeometry *request, XtWidgetGeometry *reply)
	      		     		/* geom. requested on this one	*/
	      		              	/* geometry so far		*/
	                           	/* original request		*/
	                         	/* reply to the request		*/
{
	XtAlmostProc sva = ((StubWidget) w)->stub.set_values_almost;

	if (sva != (XtAlmostProc) NULL)
		(*sva)(w, new_widget, request, reply);
} /* END OF SetValuesAlmost() */

/*
 *************************************************************************
 * SetValuesHook - set sub-widget data.  In here we check to see if
 * the Stub Widget really needs to select on Expose events.  The expose
 * mask will only be selected if the widget has an expose procedure for
 * it's instance part.  We don't update the select mask if there is no
 * expose procedure since this happens in only two ways:
 *	1. if the widget never had an expose procedure, or
 *	2. if the expose procedure was set to NULL.
 * If (1) is the case, the Redisplay() function will be called on the
 * first expose and will unselect the ExposureMask when the first
 * exposure is generated on the widget's window.  If (2) is the case,
 * the same argument applies.
 ****************************procedure*header*****************************
 */
static Boolean
SetValuesHook(Widget w, ArgList args, Cardinal *num_args)
{
	Boolean		redisplay = False;
	XtArgsFunc	svh = ((StubWidget) w)->stub.set_values_hook;

	if (svh != (XtArgsFunc) NULL)
	{
		redisplay = (*svh)(w, args, num_args);
	}

	if (((StubWidget) w)->stub.expose != (XtExposeProc) NULL &&
	    XtIsRealized(w) == True)
	{
		(void) SetEventMask(w, False);
	}
	return(redisplay);
} /* END OF SetValuesHook() */

static Boolean
Activate (Widget w, OlVirtualName activation_type, XtPointer data)
{
	OlActivateFunc act = ((StubWidget) w)->stub.activate;
	
	if (!XtIsSensitive(w))
		return(TRUE);
	else {
		if (act == NULL)
			act = ((PrimitiveWidgetClass)primitiveWidgetClass)->
				primitive_class.activate;
		
		if (act != NULL)
			return((*act)(w, activation_type, data));
		else
			return(FALSE);
	}
}

static Boolean
AcceptFocus(Widget w, Time *timestamp)
{
	XtAcceptFocusProc accept_focus = ((StubWidget) w)->stub.accept_focus;
	
	if (accept_focus == NULL)
		accept_focus = ((PrimitiveWidgetClass)primitiveWidgetClass)->
				core_class.accept_focus;

	if (accept_focus)
		return((*accept_focus)(w, timestamp));
	else
		return(FALSE);
}

static void
HighlightHandler(Widget w, OlDefine highlight_type)
{
	OlHighlightProc highlight = ((StubWidget) w)->stub.highlight;
	
	if (highlight == NULL)
		highlight = ((PrimitiveWidgetClass)primitiveWidgetClass)->
				primitive_class.highlight_handler;

	if (highlight)
		(*highlight)(w, highlight_type);
}

static Widget
RegisterFocus(Widget w)
{
	OlRegisterFocusFunc reg_focus = ((StubWidget) w)->stub.register_focus;

	if (reg_focus == NULL)
		reg_focus = ((PrimitiveWidgetClass)primitiveWidgetClass)->
				primitive_class.register_focus;

	if (reg_focus)
		return((*reg_focus)(w));
	else
		return(NULL);
}

static Widget
TraversalHandler(Widget shell, Widget w, OlDefine direction, Time time)
{
	OlTraversalFunc traversal_func =((StubWidget)w)->stub.traversal_handler;

	if (traversal_func == NULL)
		traversal_func = ((PrimitiveWidgetClass)primitiveWidgetClass)->
				primitive_class.traversal_handler;

	if (traversal_func)
		return((*traversal_func)(shell, w, direction, time));
	else
		return(NULL);
}

static void
StubQuerySCLocnProc(
		const Widget            w,
		const Widget            supercaret,
                const Dimension         sc_width,
                const Dimension         sc_height,
                unsigned int    *const  scale,
                SuperCaretShape *const  shape,
                Position        *const  x_center_return,
                Position        *const  y_center_return)
{
	SuperCaretQueryLocnProc	query_sc_locn_proc = 
			((StubWidget)w)->stub.query_sc_locn_proc;

	if(query_sc_locn_proc = (SuperCaretQueryLocnProc)NULL)
		query_sc_locn_proc = ((PrimitiveWidgetClass)primitiveWidgetClass)->
						primitive_class.query_sc_locn_proc;

	if(query_sc_locn_proc != (SuperCaretQueryLocnProc)NULL)
			(*query_sc_locn_proc)(
						w,
                				supercaret, 
                				sc_width,
                				sc_height,
                				scale,
                				shape, 
                				x_center_return, 
                				y_center_return);
	return;
}

/*
 *************************************************************************
 *
 * Action Procedures
 *
 ****************************action*procedures****************************
 */

/* There are no action procedures */

/*
 *************************************************************************
 *
 * Public Procedures
 *
 ****************************public*procedures****************************
 */

/* There are no public procedures */

