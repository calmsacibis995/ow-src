#pragma ident	"@(#)RectButton.c	302.7	97/03/26 lib/libXol SMI"	/* button:src/RectButton.c 1.28	*/

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

/*	Copyright (c) 1990 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*
 ************************************************************
 *
 *  Description:
 *	This file contains the source for the OPEN LOOK(tm)
 *	rectangular button widget.
 *
 ************************************************************
 */


#include <stdio.h>
#include <ctype.h>

#include <X11/IntrinsicP.h>
#include <X11/StringDefs.h>

#include <Xol/OpenLookP.h>
#include <Xol/ButtonP.h>
#include <Xol/RectButtoP.h>
#include <Xol/ExclusiveP.h>
#include <Xol/NonexclusP.h>


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

static void Initialize (Widget request, Widget new, ArgList args, Cardinal *num_args);

					/* private procedures		*/
static Boolean	PreviewState(Widget w, XEvent *event, OlVirtualName vname, Boolean activate);
static void	RectButtonHandler(Widget w, OlVirtualEvent ve);
static void	MotionHandler(Widget w, OlVirtualEvent ve);
static void	ToggleState(Widget w);
static Boolean	SetState(Widget w, XEvent *event, OlVirtualName vname, Boolean activate);
static Boolean	SetDefault(Widget w, OlVirtualName vname);
static Boolean	RectEventChecksOut(int sb, OlVirtualName vname);
static void	RegisterConverters();
static int	GetShellBehavior(Widget w);

					/* class procedures		*/

static Boolean	ActivateWidget (Widget, OlVirtualName, XtPointer);
static Boolean	SetValues(Widget current, Widget request, Widget new,
			  ArgList args, Cardinal *num_args);
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

#define EXCLUSIVES 0
#define NONEXCLUSIVES 1
#define NEITHER 1

static Boolean false = FALSE;

/*
 *************************************************************************
 *
 * Define Translations and Actions
 *
 ***********************widget*translations*actions**********************
 */

#if 1
static char defaultTranslations[] = "\
	<FocusIn>:	OlAction() \n\
	<FocusOut>:	OlAction() \n\
	<Key>:		OlAction() \n\
	<BtnDown>:	OlAction() \n\
	<BtnUp>:	OlAction() \n\
	<Enter>:	OlAction() \n\
	<Leave>:	OlAction() \n\
	<BtnMotion>:	OlAction()";
#else
static char defaultTranslations[] = "#augment\n\
	<Enter>:	OlAction() \n\
	<Leave>:	OlAction() \n\
	<BtnMotion>:	OlAction()";
#endif

static OlEventHandlerRec
RBevents[] = {
	{ ButtonPress,	RectButtonHandler},
	{ ButtonRelease,RectButtonHandler},
	{ EnterNotify,	RectButtonHandler},
	{ LeaveNotify,	RectButtonHandler},
	{ MotionNotify, MotionHandler},
};

/*
 *************************************************************************
 *
 * Define Resource list associated with the Widget Instance
 *
 ****************************widget*resources*****************************
 */

static XtResource resources[] = { 

	{XtNbuttonType,
		XtCButtonType,
		XtROlDefine,
		sizeof(OlDefine),
		XtOffset(ButtonWidget,button.button_type),
		XtRImmediate,
		(XtPointer) ((OlDefine) OL_RECTBUTTON) },

	{XtNparentReset,
		XtCParentReset,
		XtRBoolean,
		sizeof(Boolean),
		XtOffset(RectButtonWidget, rect.parent_reset),
		XtRBoolean,
		(XtPointer) &false},
	};

/*
 *************************************************************************
 *
 * Define Class Record structure to be initialized at Compile time
 *
 ***************************widget*class*record***************************
 */

RectButtonClassRec rectButtonClassRec = {
  {
    (WidgetClass) &(buttonClassRec),	/* superclass		  */	
    "RectButton",			/* class_name		  */
    sizeof(RectButtonRec),		/* size			  */
    NULL,				/* class_initialize	  */
    NULL,				/* class_part_initialize  */
    FALSE,				/* class_inited		  */
    Initialize,				/* initialize		  */
    NULL,				/* initialize_hook	  */
    XtInheritRealize,			/* realize		  */
    NULL,				/* actions		  */
    0,					/* num_actions		  */
    resources,				/* resources		  */
    XtNumber(resources),		/* resource_count	  */
    NULLQUARK,				/* xrm_class		  */
    FALSE,				/* compress_motion	  */
    TRUE,				/* compress_exposure	  */
    TRUE,				/* compress_enterleave    */
    FALSE,				/* visible_interest	  */
    NULL,				/* destroy		  */
    XtInheritResize,			/* resize		  */
    XtInheritExpose,			/* expose		  */
    SetValues,				/* set_values		  */
    NULL,				/* set_values_hook	  */
    XtInheritSetValuesAlmost,		/* set_values_almost	  */
    NULL,				/* get_values_hook	  */
    XtInheritAcceptFocus,		/* accept_focus		  */
    XtVersion,				/* version		  */
    NULL,				/* callback_private	  */
    defaultTranslations,		/* tm_table		  */
    NULL,				/* query_geometry	  */
  },  /* CoreClass fields initialization */
  {					/* primitive class	*/
      NULL,				/* reserved1		*/
      XtInheritHighlightHandler,	/* highlight_handler 	*/
      NULL,				/* traversal_handler	*/
      NULL,				/* register_focus	*/
      ActivateWidget,			/* activate		*/
      RBevents,				/* event_procs		*/
      XtNumber(RBevents),		/* num_event_procs	*/
      OlVersion,			/* version             */
      NULL,				/* extension           */
      {NULL, 0},			/* dyn_data	       */
      NULL,				/* transparent_proc     */
      XtInheritSuperCaretQueryLocnProc,	/* query_sc_locn_proc   */
	
  },
  {
    0,                                     /* field not used    */
  },  /* ButtonClass fields initialization */
  {
    0,                                     /* field not used    */
  },  /* RectButtonClass fields initialization */
};

/*
 *************************************************************************
 *
 * Public Widget Class Definition of the Widget Class Record
 *
 *************************public*class*definition*************************
 */

WidgetClass rectButtonWidgetClass = (WidgetClass) &rectButtonClassRec;


/*
 *************************************************************************
 * Initialize: Computes the starting size based on scale.
 ****************************procedure*header*****************************
 */
/* ARGSUSED */
static void
Initialize(Widget request, Widget new, ArgList args, Cardinal *num_args)
                                        /* what the client asked for */
                                        /* what we're going to give him */
 
 
{
   RectButtonWidget w = (RectButtonWidget) new;
   w->rect.enterNotifyReceived = FALSE;
}



/*
 *************************************************************************
 *
 * Private Procedures
 *
 ***************************private*procedures****************************
 */

/*
 ************************************************************
 *
 * RectEventChecksOut - This function checks button vs. shell behavior
 *
 *********************function*header************************
 */
static Boolean RectEventChecksOut(int sb, OlVirtualName vname)
{
	if(sb==StayUpMenu) return TRUE; 	/* select or menu button okay */

	if(sb==PressDragReleaseMenu) {		/* menu button only */
		if(vname==OL_MENU || vname==OL_MENUKEY) return TRUE;
			else return FALSE;
	}
						/* other shell behaviors */

	if(vname==OL_MENU || vname==OL_MENUKEY) 
		return FALSE;

	return TRUE; 

}	/*  RectEventChecksOut  */

/*
 ************************************************************
 *
 * GetShellBehavior - This function gets context of button
 *
 *********************function*header************************
 */

static int GetShellBehavior(Widget w)
{
	ExclusivesWidget ew;
	NonexclusivesWidget new;
	Widget parent;

	parent = XtParent(w);

	if(XtIsSubclass(parent, exclusivesWidgetClass)) {
		ew = (ExclusivesWidget) parent;
		return ew->exclusives.shell_behavior;
	}

	if(XtIsSubclass(parent, nonexclusivesWidgetClass)) {
		new = (NonexclusivesWidget) parent;
		return new->nonexclusives.shell_behavior;
	}

	return (int) OtherBehavior;

}	/*  GetShellBehavior  */

/*
 *************************************************************************
 *
 * Class Procedures
 *
 ****************************class*procedures*****************************
 */

/*
 *************************************************************************
 * ActivateWidget - this routine is used to activate the callbacks of
 * this widget.
 ****************************procedure*header*****************************
 */
static Boolean
ActivateWidget(
	Widget		w,
	OlVirtualName	type,
	XtPointer	data)
{
	Boolean ret=FALSE;
	Boolean activate=TRUE;
	XEvent dummy_event;

	if (type != OL_SELECT && type != OL_SELECTKEY &&
	    type != OL_MENU && type != OL_MENUKEY)
		return FALSE;

	ret=PreviewState(w,&dummy_event,type,activate);
	if(ret==FALSE)
		return FALSE;
	ret=SetState(w,&dummy_event,type,activate);
		return ret;

} /* END OF ActivateWidget() */

/*
 ************************************************************
 *
 *  SetValues - This function compares the requested values
 *	to the current values, and sets them in the new
 *	widget.  It returns TRUE when the widget must be
 *	redisplayed.
 *
 *********************function*header************************
 */
static Boolean SetValues (Widget current, Widget request, Widget new,
			  ArgList args, Cardinal *num_args)
{
	RectButtonWidget bw = (RectButtonWidget) current;
	RectButtonWidget newbw = (RectButtonWidget) new;
	Boolean needs_redisplay = FALSE;
	Widget parent = XtParent(new);

	/*
	 *  Has the is_default resource changed?
	 */
	if (bw->button.is_default != newbw->button.is_default)  {

		/*
		 *  Notify the parent of the button that the button
		 *  default has changed.
		 */

		/*
		 *  This must set the old button widget to the
		 *  new value too because this call to XtSetValues
		 *  cannot access the value in the newbw or reqbw
		 *  since they are allocated in a strange way (see
		 *  the code for _XtSetValues).
		 */

		/*
		 *  Exclusives depends on having ONLY True or False
		 *  values to set at this point.
		 */
		if (newbw->button.is_default != FALSE)
			newbw->button.is_default = TRUE;

		bw->button.is_default = newbw->button.is_default;

		{
		Arg arg;

		XtSetArg(arg, XtNresetDefault, bw->button.is_default);
		XtSetValues(parent, &arg, 1);
		}

		needs_redisplay = TRUE;
		}

	/*
	 *  Has the set resource changed?
	 */
	if (bw->button.set != newbw->button.set)  {

		/*
		 *  Notify the parent of the button that the button
		 *  default has changed.
		 */

		if (newbw->rect.parent_reset == FALSE)  {
			/*
			 *  This must set the old button widget to the
			 *  new value too because this call to XtSetValues
			 *  cannot access the value in the newbw or reqbw
			 *  since they are allocated in a strange way (see
			 *  the code for _XtSetValues).
			 */
			/*
			 *  Exclusives depends on having ONLY True or False
			 *  values to set at this point.
			 */
			if (newbw->button.set != FALSE)
				newbw->button.set = TRUE;
	
			bw->button.set = newbw->button.set;
	
			{
			Arg arg;
	
			XtSetArg(arg, XtNresetSet, bw->button.set);
			XtSetValues(parent, &arg, 1);
			}
			}
		needs_redisplay = TRUE;
		}

	/*
	 *  Always reset the parent_reset value to False.
	 */
	newbw->rect.parent_reset = FALSE;

	return (XtIsRealized((Widget)newbw) && needs_redisplay);

}	/* SetValues */

/*
 *************************************************************************
 *
 * Action Procedures
 *
 ****************************action*procedures****************************
 */

/*
 ************************************************************
 *
 * MotionHandler: temporary code
 *
 *********************function*header************************
 */

static void MotionHandler (Widget w, OlVirtualEvent ve)
{
	static Boolean activate = FALSE;

	switch (ve->virtual_name)
	{
		case OL_MENU:
			ve->consumed = True;
			/*PreviewState(w,ve->xevent,ve->virtual_name,activate);*/
			break;
		case OL_MENUDEFAULT:
			ve->consumed = True;
			SetDefault (w, OL_UNKNOWN_BTN_INPUT);
			break;
	}
} /* MotionHandler */

/*
 ************************************************************
 *
 *  ToggleState - this function is called to change the state 
 *	of the button, were it to be selected.
 *
 *********************function*header************************
 */

static void ToggleState(Widget w)
{
	RectButtonWidget bw = (RectButtonWidget) w;
	Arg arg[2];

	if(w==(Widget)0) return;

	if(bw->button.set!=FALSE ) {
		XtSetArg(arg[0], XtNparentReset, TRUE);
		XtSetArg(arg[1], XtNset, FALSE);
		XtSetValues(w, arg, 2);
		return;
	}
				/* here if button originally FALSE */

	XtSetArg(arg[0], XtNparentReset, TRUE);
	XtSetArg(arg[1], XtNset, TRUE);
	XtSetValues(w, arg, 2);

	return;

} /* ToggleState */

/*
 ************************************************************
 *
 *  PreviewState - this function is called on a select or menu button 
 *	down event.  It previews the appearance of the button if it
 *	were to be selected, including unsetting the appearance of
 *	any other set button.
 *
 *********************function*header************************
 */

static Boolean PreviewState(Widget w, XEvent *event, OlVirtualName vname, Boolean activate)
{
	RectButtonWidget bw = (RectButtonWidget) w;
	ExclusivesWidget ew;
	NonexclusivesWidget new;
	Widget parent,looks_set,set_child;
	Boolean noneset;
	int parent_type=NEITHER,sb;
	XCrossingEvent *xce;

	if(!XtIsSensitive(w)) 
		return TRUE;

	if(bw->button.busy)
		return TRUE;

					/* filter out pointer crossings */
	if(!activate) {

	if(event->type==EnterNotify || event->type==LeaveNotify) {
		xce = (XCrossingEvent *) &(event->xcrossing);
		if(xce->mode!=NotifyNormal) {
			return FALSE; 
		}
	}
	if(vname!=OL_UNKNOWN_INPUT) {	/* value with Enter/LeaveNotify */
		sb= GetShellBehavior(w);
		if(!RectEventChecksOut(sb,vname)) { 
			return FALSE;
		}
	}
	}



	parent = XtParent(w);

	if(XtIsSubclass(parent, exclusivesWidgetClass)) {
		ew = (ExclusivesWidget) parent;
		parent_type=EXCLUSIVES;
		looks_set= ew->exclusives.looks_set;
		set_child= ew->exclusives.set_child;
		noneset= ew->exclusives.noneset;
	}
	else if(XtIsSubclass(parent, nonexclusivesWidgetClass)) {
		new = (NonexclusivesWidget) parent;
		parent_type=NONEXCLUSIVES;
	}

	if(parent_type==NONEXCLUSIVES || parent_type==NEITHER) {
		ToggleState(w);
		return TRUE;
	}

					/* if here, EXCLUSIVES is parent */
					
					/* if !nonset two buttons to do */

	if(w==set_child && w==looks_set && noneset) {

		ToggleState(w);
		ew->exclusives.looks_set=(Widget)0;
		
		return TRUE;
	}

	if(w==set_child && w!=looks_set && noneset) return TRUE;

	if(w==set_child && w==looks_set && !noneset) return TRUE;

	if(w==set_child && w!=looks_set && !noneset) {

		ToggleState(looks_set);
		ToggleState(w);
		ew->exclusives.looks_set=w;
		return TRUE;
	}

	if(w!=set_child && w==looks_set && noneset)  return TRUE;

	if(w!=set_child && w!=looks_set && noneset) {

		ToggleState(looks_set);
		ToggleState(w);
		ew->exclusives.looks_set=w;
		return TRUE;
	}

	if(w!=set_child && w==looks_set && !noneset) return TRUE;

	if(w!=set_child && w!=looks_set && !noneset) {
		ToggleState(looks_set);
		ToggleState(w);
		ew->exclusives.looks_set=w;
		return TRUE;
	}
	
	return TRUE;	/* should not get here */

}	/* PreviewState */

/*
 ************************************************************
 *
 *  RectButtonHandler - this function is called by OPEN LOOK
 *	for requested events.
 *
 *********************function*header************************
 */

static void
RectButtonHandler(Widget w, OlVirtualEvent ve)
{
RectButtonWidget bw = (RectButtonWidget) w;

	Boolean activate=FALSE;

	switch(ve->xevent->type) {

		case EnterNotify:
			bw->rect.enterNotifyReceived = TRUE;

		case LeaveNotify:
			if((ve->virtual_name==OL_SELECT
				|| ve->virtual_name==OL_MENU) &&
				bw->rect.enterNotifyReceived)
			ve->consumed = PreviewState(w,ve->xevent,
				ve->virtual_name,activate);
			if (ve->xevent->type == LeaveNotify)
				bw->rect.enterNotifyReceived = FALSE;
			break;

		case ButtonPress:

		(void) _OlUngrabPointer(w);

		if(ve->virtual_name==OL_MENUDEFAULT)
			ve->consumed = SetDefault(w,OL_MENUDEFAULT);

		else if(ve->virtual_name==OL_SELECT
			|| ve->virtual_name==OL_MENU) {
			ve->consumed =
			PreviewState(w,ve->xevent,ve->virtual_name,activate);
		}
			break;

		case ButtonRelease:

			if (ve->virtual_name == OL_SELECT ||
				ve->virtual_name == OL_MENU) 
			ve->consumed =
			SetState(w,ve->xevent,ve->virtual_name,activate);

			break;
	}
}

/*
 ************************************************************
 *
 *  SetState - this function is called on a select or menu button up
 *	event.  It invokes the users select and unselect callback(s) 
 *	and changes the state of the rectangular button from 
 *	set to unset or unset to set, as well as unsetting any
 *	previously set button.
 *
 *********************function*header************************
 */
 
static Boolean
SetState(Widget w, XEvent *event, OlVirtualName vname, Boolean activate)
{
	RectButtonWidget	bw = (RectButtonWidget) w;
	ExclusivesWidget	ew;
	Widget          	parent = XtParent(w);
	Widget          	set_child;
	int             	parent_type = NEITHER;
	int             	sb;
	Boolean         	ret;

	if (!XtIsSensitive(w))
		return TRUE;

	if (!activate) {
		if (!(RectEventChecksOut(GetShellBehavior(w), vname)))
			return FALSE;
	}

	if (XtIsSubclass(parent, exclusivesWidgetClass)) {
		ew = (ExclusivesWidget)parent;
		parent_type = EXCLUSIVES;
		set_child = ew->exclusives.set_child;
	} else 
		if (XtIsSubclass(parent, nonexclusivesWidgetClass)) {
			parent_type = NONEXCLUSIVES;
		}
	
	/* only case with no change of state or actions */
	if (parent_type == EXCLUSIVES && w == set_child && 
		ew->exclusives.noneset == FALSE) {

		return TRUE;
	}
	
	/* all other cases state will change state and have actions */
	if (!bw->button.set) {

		_OlManagerCallPreSelectCBs(w, event);

		if (bw->button.unselect != (XtCallbackList)NULL)
			XtCallCallbacks(w, XtNunselect, (XtPointer)NULL);
		if (parent_type == EXCLUSIVES)
			ew->exclusives.set_child = (Widget)0;

		_OlManagerCallPostSelectCBs(w, event);

		return TRUE;
	}
	
	if (bw->button.set) {

		_OlManagerCallPreSelectCBs(w, event);

		if (parent_type == EXCLUSIVES) {
			if (set_child != (Widget) 0) {
				if (((RectButtonWidget)set_child)->
						button.unselect !=
						(XtCallbackList)NULL)
					XtCallCallbacks(
						set_child,
						XtNunselect,
						(XtPointer)NULL);
			}
			ew->exclusives.set_child = w;
		}

		if (XtHasCallbacks(w, XtNselect) == XtCallbackHasSome)
			XtCallCallbacks(w, XtNselect, (XtPointer)NULL);

		_OlManagerCallPostSelectCBs(w, event);

		if (XtHasCallbacks(w, XtNpostSelect) == XtCallbackHasSome)
			XtCallCallbacks(w, XtNpostSelect, (XtPointer)NULL);

		return TRUE;
	}
} /* END OF SetState() */

/*
 ************************************************************
 *
 *  SetDefault - this function is called on a button up
 *	event.  It invokes the users callback and changes
 *	the state of the  button back to default.
 *
 *********************function*header************************
 */
static Boolean SetDefault(Widget w, OlVirtualName vname)
{
	RectButtonWidget bw = (RectButtonWidget) w;
	ShellBehavior sb;
	ExclusivesWidget ew;
	NonexclusivesWidget new;
	Widget parent;
	Arg arg;

	if(!XtIsSensitive(w)) return TRUE;

	parent = XtParent(w);

	if(XtIsSubclass(parent, exclusivesWidgetClass)) {
		ew = (ExclusivesWidget) parent;
		sb = ew->exclusives.shell_behavior;
	}

	else if(XtIsSubclass(parent, nonexclusivesWidgetClass)) {
		new = (NonexclusivesWidget) parent;
		sb = new->nonexclusives.shell_behavior;
	}

        if (sb != PressDragReleaseMenu &&
            sb != StayUpMenu &&
            sb != PinnedMenu &&
            sb != UnpinnedMenu)
                return FALSE;


        if (bw->button.is_default == FALSE) {
		XtSetArg(arg, XtNdefault, TRUE);
                XtSetValues(w, &arg, 1);

                _OlSetDefault(w, True);
		return TRUE;
	}
}

/*
 *************************************************************************
 *
 * Public Procedures
 *
 ****************************public*procedures****************************
 */

