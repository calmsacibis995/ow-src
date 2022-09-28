#pragma ident	"@(#)Manager.c	302.7	97/03/26 lib/libXol SMI"	/* manager:Manager.c 1.35	*/

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

/*	Copyright (c) 1988 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*************************************************************************
 *
 * Description:	This file along with Manager.h and ManagerP.h implements
 *		the OPEN LOOK ManagerWidget.  This widget interacts with
 *		subclasses of the PrimitiveWidgetClass to provide
 *		keyboard traversal/acceleration.
 *
 *******************************file*header******************************/


#include <stdio.h>
#include <stdlib.h>

#include <X11/IntrinsicP.h>
#include <X11/Shell.h>
#include <X11/StringDefs.h>

#include <Xol/Error.h>
#include <Xol/ManagerP.h>
#include <Xol/OpenLookP.h>
#include <Xol/VendorI.h>

/*************************************************************************
 *
 * Define global/static variables and #defines, and
 * Declare externally referenced variables
 *
 *****************************file*variables*****************************/

#define BYTE_OFFSET	XtOffsetOf(ManagerRec, manager.dyn_flags)
static _OlDynResource dyn_res[] = {
{ { XtNbackground, XtCBackground, XtRPixel, sizeof(Pixel), 0,
	 XtRString, XtDefaultBackground }, BYTE_OFFSET, OL_B_MANAGER_BG, NULL },
{ { XtNinputFocusColor, XtCInputFocusColor, XtRPixel, sizeof(Pixel), 0,
	 XtRString, "Red" }, BYTE_OFFSET, OL_B_MANAGER_FOCUSCOLOR, NULL },
{ { XtNborderColor, XtCBorderColor, XtRPixel, sizeof(Pixel), 0,
	 XtRString, XtDefaultForeground }, BYTE_OFFSET,
	 OL_B_MANAGER_BORDERCOLOR, NULL },
};
#undef BYTE_OFFSET

/*************************************************************************
 *
 * Forward Procedure definitions listed by category:
 *		1. Private Procedures
 *		2. Class   Procedures
 *		3. Action  Procedures
 *		4. Public  Procedures
 *
 **************************forward*declarations**************************/

					/* private procedures		*/

					/* class procedures		*/
static Boolean	AcceptFocus(Widget w, Time *time);
static void	ClassInitialize(void);
static void	ClassPartInitialize(WidgetClass wc);
static void	Destroy(Widget w);
static void	GetValuesHook(Widget w, ArgList args, Cardinal *num_args);
static void	Initialize(Widget request, Widget new, ArgList args,
                           Cardinal *num_args);
static Boolean	SetValues(Widget current, Widget request, Widget new, ArgList args, Cardinal *num_args);

					/* action procedures		*/

					/* public procedures		*/

/*************************************************************************
 *
 * Define Translations and Actions
 *
 ***********************widget*translations*actions**********************/

/* See generic translations and actions */
/*************************************************************************
 *
 * Define Resource list associated with the Widget Instance
 *
 ****************************widget*resources****************************/


#define OFFSET(field)	XtOffsetOf(ManagerRec, manager.field)
static XtResource
resources[] = {
    { XtNborderWidth, XtCBorderWidth, XtRDimension, sizeof(Dimension),
	XtOffsetOf(ManagerRec, core.border_width),
	XtRImmediate, (XtPointer)0 },

  { XtNconsumeEvent, XtCCallback, XtRCallback, sizeof(XtCallbackList),
    OFFSET(consume_event), XtRCallback, (XtPointer)NULL
  },
  { XtNinputFocusColor, XtCInputFocusColor, XtRPixel, sizeof(Pixel),
    OFFSET(input_focus_color), XtRString, "Red"
  },
  { XtNreferenceName, XtCReferenceName, XtRString, sizeof(String),
    OFFSET(reference_name), XtRString, (XtPointer)NULL
  },
  { XtNreferenceWidget, XtCReferenceWidget, XtRWidget, sizeof(Widget),
    OFFSET(reference_widget), XtRWidget, (XtPointer)NULL
  },
  { XtNtraversalOn, XtCTraversalOn, XtRBoolean, sizeof(Boolean),
    OFFSET(traversal_on), XtRImmediate, (XtPointer)True
  },
  { XtNuserData, XtCUserData, XtRPointer, sizeof(XtPointer),
    OFFSET(user_data), XtRPointer, (XtPointer)NULL
  },
  { XtNunrealizeCallback, XtCCallback, XtRCallback, sizeof(XtCallbackList),
    OFFSET(unrealize_callbacks), XtRCallback, (XtPointer)NULL
  },
  { XtNpreSelect, XtCCallback, XtRCallback, sizeof(XtCallbackList),
    OFFSET(pre_select), XtRCallback, (XtPointer)NULL
  },
  { XtNpostSelect, XtCCallback, XtRCallback, sizeof(XtCallbackList),
    OFFSET(post_select), XtRCallback, (XtPointer)NULL
  },
};
#undef OFFSET


/*************************************************************************
 *
 * Define Class Record structure to be initialized at Compile time
 *
 ***************************widget*class*record**************************/

ManagerClassRec managerClassRec =
{
   {
/* core_class fields      */
    /* superclass         */    (WidgetClass)&constraintClassRec,
    /* class_name         */    "Manager",
    /* widget_size        */    sizeof(ManagerRec),
    /* class_initialize   */    ClassInitialize,
    /* class_part_init    */    ClassPartInitialize,
    /* class_inited       */	FALSE,
    /* initialize         */    Initialize,
    /* initialize_hook	  */    NULL,
    /* realize            */    XtInheritRealize,
    /* actions	          */	/* See ClassInitialize */NULL,
    /* num_actions        */	/* See ClassInitialize */NULL,
    /* resources          */    resources,
    /* num_resources      */    XtNumber(resources),
    /* xrm_class          */    NULLQUARK,
    /* compress_motion	  */	TRUE,
    /* compress_exposure  */	TRUE,
    /* compress_enterleave*/	TRUE,
    /* visible_interest   */    FALSE,
    /* destroy            */    Destroy,
    /* resize             */    XtInheritResize,
    /* expose             */    XtInheritExpose,
    /* set_values         */    SetValues,
    /* set_values_hook    */    NULL,
    /* set_values_almost  */    XtInheritSetValuesAlmost,
    /* get_values_hook    */    GetValuesHook,
    /* accept_focus       */	AcceptFocus,
    /* Version            */    XtVersion,
    /* PRIVATE cb list    */    NULL,
    /* tm_table           */    /* See ClassInitialize */NULL,
    /* query_geom         */    XtInheritQueryGeometry,
    /* display_accelerator*/	XtInheritDisplayAccelerator,
    /* extension	  */	NULL
   },
   {
/* composite_class fields */
    /* geometry_manager   */    XtInheritGeometryManager,
    /* change_managed     */    XtInheritChangeManaged,
    /* insert_child	  */	XtInheritInsertChild,
    /* delete_child	  */	XtInheritDeleteChild,
    /* extension	  */	NULL
   },
   {
/* constraint_class fields */
    /* resources	  */	NULL,
    /* num_resources	  */	0,
    /* constraint_size	  */	0,
    /* initialize	  */	NULL,
    /* destroy		  */	NULL,
    /* set_values	  */	NULL,
    /* extension	  */	NULL
   },
   {
/* manager_class fields */     
    /* highlight_handler	*/	NULL,
    /* reserved                 */      NULL,
    /* reserved                 */      NULL,
    /* traversal_handler	*/	NULL,
    /* activate widget          */      NULL,
    /* event_procs		*/	/* See ClassInitialize */NULL,
    /* num_event_procs   	*/	/* See ClassInitialize */NULL,
    /* register_focus		*/	NULL,
    /* reserved                 */      NULL,
    /* version			*/	OlVersion,
    /* extension		*/	NULL,
    /* dyn_data			*/	{ dyn_res, XtNumber(dyn_res) },
    /* transparent_proc		*/	_OlDefaultTransparentProc,
    /* query_sc_locn_proc       */	NULL,
   }
};


/*************************************************************************
 *
 * Public Widget Class Definition of the Widget Class Record
 *
 *************************public*class*definition************************/

WidgetClass managerWidgetClass = (WidgetClass)&managerClassRec;


/*************************************************************************
 *
 * Private Procedures
 *
 ***************************private*procedures***************************/

static void
CallManagerSelectCBs(Widget container, Widget selectee, XEvent* initiator,
                     const char cb[], int reason)
{
 
        if (container == (Widget)NULL || XtIsVendorShell(container) ) 
	    return;
 
        if (XtIsSubclass(container, managerWidgetClass) &&
            XtHasCallbacks(container, cb) == XtCallbackHasSome) {
                OlManagerSelectCallbackStruct   cbdata;
 
                cbdata.reason   = reason;
                cbdata.event    = initiator;
                cbdata.selectee = selectee;
 
                XtCallCallbacks(container, cb, (XtPointer)&cbdata);
        } else
                CallManagerSelectCBs(XtParent(container), selectee,
                                     initiator, cb, reason);
}
 
void
_OlManagerCallPreSelectCBs(Widget       selectee, XEvent* initiator)
{
        CallManagerSelectCBs(XtParent(selectee), selectee, initiator,
                             XtNpreSelect, OL_REASON_PRE_SELECT);
}
 
void
_OlManagerCallPostSelectCBs(Widget      selectee, XEvent* initiator)
{
        CallManagerSelectCBs(XtParent(selectee), selectee, initiator,
                             XtNpostSelect, OL_REASON_POST_SELECT);
}

/*************************************************************************
 *
 * Class Procedures
 *
 ****************************class*procedures****************************/

/***************************function*header*******************************
 * AcceptFocus - If I can accept focus, look thru list of children for
 *		one that can accept focus.
 */
static Boolean
AcceptFocus(Widget w, Time *time)
          	  		/* me */
        	     
{
    /* If I can accept focus, find a willing child to take it */
    if (OlCanAcceptFocus(w, *time))
    {
	WidgetList	child = ((CompositeWidget)w)->composite.children;
	Cardinal	i = ((CompositeWidget)w)->composite.num_children;

    	for ( ; i > 0; i--, child++)
    	{
	    if (XtIsManaged(*child) &&
	      (OlCallAcceptFocus(*child, *time) == True))
		return (True);
    	}
    }

   return (False);

} /* AcceptFocus() */


/*************************************************************************
 * ClassInitialize - Sets the generic action table, translation table and
 * event handlers.
 ***************************function*header*******************************/
static void
ClassInitialize(void)
{
	 ManagerWidgetClass wc = (ManagerWidgetClass)managerWidgetClass;

	 wc->core_class.actions	= (XtActionList)_OlGenericActionTable;
	 wc->core_class.num_actions	= (Cardinal)_OlGenericActionTableSize;
	 wc->core_class.tm_table	= (String)_OlGenericTranslationTable;
	 wc->manager_class.event_procs = (OlEventHandlerList)
					_OlGenericEventHandlerList;
	 wc->manager_class.num_event_procs = (Cardinal)
					_OlGenericEventHandlerListSize;
} /* END OF ClassInitialize() */

/*************************************************************************
 * ClassPartInitialize - Provides for inheritance of the class procedures
 ***************************function*header*******************************/

static void
ClassPartInitialize(WidgetClass wc)
{
        ManagerWidgetClass mc = (ManagerWidgetClass)wc;
	ManagerWidgetClass swc = (ManagerWidgetClass)
					mc->core_class.superclass;
	CompositeClassExtension	ext;

			/* Generate warning if version is less than 2.0	*/

	if (mc->manager_class.version != OlVersion && OlVersion < 2000)
	{
		OlVaDisplayWarningMsg((Display *)NULL,
			OleNinternal, OleTbadVersion,
			OleCOlToolkitWarning, OleMinternal_badVersion,
			mc->core_class.class_name,
			mc->manager_class.version, OlVersion);
	}

	/* Make every subclass accept objects.  It's the subclass'
	 * responsibility to remove this or change 'accepts_objects'
	 * if it does not want object children.
	 */
	ext			= XtNew(CompositeClassExtensionRec);
	ext->record_type	= NULLQUARK;
	ext->version		= XtCompositeExtensionVersion;
	ext->record_size	= sizeof(CompositeClassExtensionRec);
	ext->accepts_objects	= (Boolean)TRUE;
	ext->next_extension	= mc->composite_class.extension;

	mc->composite_class.extension = (XtPointer)ext;

	if (mc->manager_class.highlight_handler == XtInheritHighlightHandler)
		mc->manager_class.highlight_handler =
				swc->manager_class.highlight_handler;

	if (mc->manager_class.register_focus == XtInheritRegisterFocus)
		mc->manager_class.register_focus =
				swc->manager_class.register_focus;

	if (mc->manager_class.traversal_handler == XtInheritTraversalHandler)
		mc->manager_class.traversal_handler =
				swc->manager_class.traversal_handler;

	if (mc->manager_class.transparent_proc == XtInheritTransparentProc)
		mc->manager_class.transparent_proc =
				swc->manager_class.transparent_proc;

	if ((WidgetClass)mc == managerWidgetClass)
		return;

	if (mc->manager_class.dyn_data.num_resources == 0) {
		/* give the superclass's resource list to this class */
		mc->manager_class.dyn_data = swc->manager_class.dyn_data;
	}
	else {
		/* merge the two lists */
		_OlMergeDynResources(&(mc->manager_class.dyn_data),
				     &(swc->manager_class.dyn_data));
	}
} /* ClassPartInitialize() */

/*************************************************************************
 * Destroy - Remove `w' from the traversal list
 ***************************function*header*******************************/

static void
Destroy(Widget w)
{
	ManagerWidget	mw = (ManagerWidget)w;

	_OlDestroyKeyboardHooks(w);
	if (mw->manager.reference_name)
		XtFree(mw->manager.reference_name);

	XtRemoveAllCallbacks(w, XtNpreSelect);
	XtRemoveAllCallbacks(w, XtNpostSelect);
}

/*************************************************************************
 * GetValuesHook - check for XtNreferenceWidget and XtNreferenceName
 *		and return info according to the traversal list
 */
static void
GetValuesHook(Widget w, ArgList args, Cardinal *num_args)
{
	_OlGetRefNameOrWidget(w, args, num_args);
}

/*************************************************************************
 * Initialize - Initialize non-resource class part members, etc.
 ***************************function*header*******************************/

static void
Initialize(Widget request, Widget new, ArgList args, Cardinal *num_args)
{
    ManagerWidget mw = (ManagerWidget) new;
    
    mw->manager.has_focus = False;	/* doesn't have focus */

    if (mw->manager.reference_name)
	mw->manager.reference_name = XtNewString(mw->manager.reference_name);

    _OlInitDynResources((Widget)mw, &(((ManagerWidgetClass)
			(mw->core.widget_class))-> manager_class.dyn_data));
    _OlCheckDynResources((Widget)mw, &(((ManagerWidgetClass)
			(mw->core.widget_class))-> manager_class.dyn_data),
			 args, *num_args);
}

static Boolean
SetValues(Widget current, Widget request, Widget new, ArgList args, Cardinal *num_args)
{
    ManagerWidget current_mw = (ManagerWidget) current;
    ManagerPart   *curPart = &current_mw->manager;
    ManagerWidget new_mw = (ManagerWidget) new;
    ManagerPart   *newPart = &new_mw->manager;

#define CHANGED(field)	(newPart->field != curPart->field)

		/* since not every manager is in the traversal list. 	 */
		/* setting up referenceName or referenceWidget for those */
		/* managers will cause warning(s). There will be no	 */
		/* checking in toolkit.					 */
    if (CHANGED(reference_name))	/* this has higher preference	 */
    {
	if (newPart->reference_name)
	{
		newPart->reference_name = XtNewString(
				newPart->reference_name);

		_OlUpdateTraversalWidget(new, newPart->reference_name,
				 NULL, False);
		
	}
	if (curPart->reference_name != NULL)
	{
		XtFree(curPart->reference_name);
		curPart->reference_name = NULL;
	}
    }
    else if (CHANGED(reference_widget))
    {
			/* no need to keep this info */
	if (curPart->reference_name != NULL)
	{
		XtFree(curPart->reference_name);
		curPart->reference_name = NULL;
	}
	_OlUpdateTraversalWidget(new, NULL,
				 newPart->reference_widget, False);
    }

	if (!XtIsSensitive(new) && (OlGetCurrentFocusWidget(new) == new)) {
		/*
		 * When it becomes insensitive, need to move focus elsewhere,
		 * if this widget currently has focus.
		 */
		OlMoveFocus(new, OL_IMMEDIATE, CurrentTime);
		newPart->has_focus = FALSE;
	}

    _OlCheckDynResources(new, &(((ManagerWidgetClass)(new->core.widget_class))->
			manager_class.dyn_data), args, *num_args);

    /* handle background transparency */
    if ((_OlDynResProcessing == FALSE) &&
	(new->core.background_pixel != current->core.background_pixel) ||
        (new->core.background_pixmap != current->core.background_pixmap)) {
		_OlDefaultTransparentProc(new, new->core.background_pixel,
					new->core.background_pixmap);
    }
    return (False);
}

/*************************************************************************
 *
 * Action Procedures
 *
 ****************************action*procedures***************************/

/*************************************************************************
 *
 * Public Procedures
 *
 ****************************public*procedures***************************/
