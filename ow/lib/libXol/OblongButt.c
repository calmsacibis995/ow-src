#pragma ident	"@(#)OblongButt.c	302.5	97/03/26 lib/libXol SMI" /* button:src/OblongButt.c	1.46 */

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

/* XOL SHARELIB - start */
/* This header file must be included before anything else */
#ifdef SHARELIB
#include <Xol/libXoli.h>
#endif
/* XOL SHARELIB - end */

/*
 ************************************************************
 *
 *  Description:
 *	This file contains the source for the OPEN LOOK(tm)
 *	OblongButton widget.
 *
 ************************************************************
 */

						/* #includes go here	*/

#include <libintl.h>

#include <X11/StringDefs.h>
#include <X11/IntrinsicP.h>

#include <Xol/OpenLookP.h>
#include <Xol/Font.h>
#include <Xol/ButtonP.h>
#include <Xol/OblongButP.h>
#include <Xol/OlI18nP.h>

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
static void	OblongButtonEH(Widget w, XtPointer client_data, XEvent *event,
			       Boolean *continue_to_dispatch);

					/* class procedures		*/
static void	GadgetInitialize(Widget request, Widget new,
				 ArgList args, Cardinal *num_args);

					/* action procedures		*/
static void	Highlight(Widget w);
static void	MenuHighlight(Widget w);
static void	MenuNormal(Widget w);
static void	MenuNotify(Widget w);
static void	Normal(Widget w);
static void	Notify(Widget w);
static void	SetDefault(Widget w, OlVirtualName activation_type);

					/* public procedures		*/

/*  There are no public procedures	*/

/*
 *************************************************************************
 *
 * Define global/static variables and #defines, and
 * Declare externally referenced variables
 *
 *****************************file*variables******************************
 */

static Boolean false = FALSE;

#define find_oblong_button_part(w) (_OlIsGadget(w) ?	\
		&((OblongButtonGadget)(w))->oblong :	\
		&((OblongButtonWidget)(w))->oblong)

/*
 *************************************************************************
 *
 * Define Translations and Actions
 *
 ***********************widget*translations*actions**********************
 */

static Boolean	ActivateWidget (Widget w, OlVirtualName activation_type, XtPointer call_data);
static void	ButtonHandler(Widget w, OlVirtualEvent ve);
static void	CrossingHandler(Widget w, OlVirtualEvent ve);
static void	MotionHandler(Widget w, OlVirtualEvent ve); /* drop it when simulation is available*/

/*
	\043augment
	#augment
*/
static char defaultTranslations[] = "\
	<FocusIn>:	OlAction() \n\
	<FocusOut>:	OlAction() \n\
	<Key>:		OlAction() \n\
	<BtnDown>:	OlAction() \n\
	<BtnUp>:	OlAction() \n\
\
	<Enter>:	OlAction() \n\
	<Leave>:	OlAction() \n\
	<BtnMotion>:	OlAction()";	/* see MotionHandler */

static OlEventHandlerRec event_procs[] =
{
	{ ButtonPress,	ButtonHandler    },
	{ ButtonRelease,ButtonHandler    },
	{ EnterNotify,	CrossingHandler  },
	{ LeaveNotify,	CrossingHandler  },
	{ MotionNotify, MotionHandler    },
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
		(XtPointer) ((OlDefine) OL_OBLONG) }
	};

static XtResource gadget_resources[] = {

	{XtNbuttonType,
		XtCButtonType,
		XtROlDefine,
		sizeof(OlDefine),
		XtOffset(ButtonGadget,button.button_type),
		XtRImmediate,
		(XtPointer) ((OlDefine) OL_OBLONG) }
        };

/*
 *************************************************************************
 *
 * Define Class Record structure to be initialized at Compile time
 *
 ***************************widget*class*record***************************
 */

OblongButtonClassRec oblongButtonClassRec = {
  {
    (WidgetClass) &(buttonClassRec),	/* superclass		  */	
    "OblongButton",			/* class_name		  */
    sizeof(OblongButtonRec),		/* size			  */
    NULL,				/* class_initialize	  */
    NULL,				/* class_part_initialize  */
    FALSE,				/* class_inited		  */
    NULL,				/* initialize		  */
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
    NULL,				/* set_values		  */
    NULL,				/* set_values_hook	  */
    XtInheritSetValuesAlmost,		/* set_values_almost	  */
    NULL,				/* get_values_hook	  */
    XtInheritAcceptFocus,		/* accept_focus		  */
    XtVersion,				/* version		  */
    NULL,				/* callback_private	  */
    defaultTranslations,		/* tm_table		  */
    NULL,				/* query_geometry	  */
  },  /* CoreClass fields initialization */
  {
	NULL,				/* reserved		*/
	XtInheritHighlightHandler,	/* highlight_handler	*/
	NULL,				/* traversal_handler	*/
	NULL,				/* register_focus	*/
	ActivateWidget,			/* activate		*/
	event_procs,			/* event_procs		*/
	XtNumber(event_procs),		/* num_event_procs	*/
	OlVersion,			/* version		*/
	NULL,				/* extension		*/
	{ NULL, 0 },			/* dyn_data		*/
	XtInheritTransparentProc,	/* transparent_proc	*/
	XtInheritSuperCaretQueryLocnProc,/* query_sc_locn_proc   */
  },	/* End of Primitive field initializations */
  {
    0,                                     /* field not used    */
  },  /* OblongButtonClass fields initialization */
};

OblongButtonGadgetClassRec oblongButtonGadgetClassRec = {
  {
    (WidgetClass) &(buttonGadgetClassRec),/* superclass           */
    "OblongButton",                     /* class_name             */
    sizeof(OblongButtonRec),            /* size                   */
    NULL,                               /* class_initialize       */
    NULL,                               /* class_part_initialize  */
    FALSE,                              /* class_inited           */
    GadgetInitialize,                   /* initialize             */
    NULL,                               /* initialize_hook        */
    (XtProc)XtInheritRealize,           /* realize                */
    NULL,                               /* actions                */
    0,                                  /* num_actions            */
    gadget_resources,                   /* resources              */
    XtNumber(gadget_resources),         /* resource_count         */
    NULLQUARK,                          /* xrm_class              */
    FALSE,                              /* compress_motion        */
    TRUE,                               /* compress_exposure      */
    TRUE,                               /* compress_enterleave    */
    FALSE,                              /* visible_interest       */
    NULL,                               /* destroy                */
    XtInheritResize,                    /* resize                 */
    XtInheritExpose,                    /* expose                 */
    NULL,				/* set_values             */
    NULL,                               /* set_values_hook        */
    XtInheritSetValuesAlmost,           /* set_values_almost      */
    NULL,                               /* get_values_hook        */
    (XtProc)XtInheritAcceptFocus,	/* accept_focus: Xt doesn't use it but we do */
    XtVersion,                          /* version                */
    NULL,                               /* callback_private       */
    NULL,                               /* tm_table               */
    NULL,                               /* query_geometry         */
  },  /* CoreClass fields initialization */
  {
	NULL,				/* reserved		*/
	XtInheritHighlightHandler,	/* highlight_handler	*/
	NULL,				/* traversal_handler	*/
	NULL,				/* register_focus	*/
	ActivateWidget,			/* activate		*/
	NULL,				/* event_procs		*/
	0,				/* num_event_procs	*/
	OlVersion,			/* version		*/
	NULL,				/* extension		*/
	{NULL, 0},			/* dynd data		*/
	NULL,				/* transparent_proc	*/
	XtInheritSuperCaretQueryLocnProc,/* query_sc_locn_proc	*/
  },  /* EventObjClass fields initialization */
  {
    0,                                  /* field not used         */
  },  /* ButtonClass fields initialization */
  {
    0,                                  /* field not used         */
  },  /* OblongButtonClass fields initialization */
};


/*
 *************************************************************************
 *
 * Public Widget Class Definition of the Widget Class Record
 *
 *************************public*class*definition*************************
 */

WidgetClass oblongButtonWidgetClass = (WidgetClass) &oblongButtonClassRec;
WidgetClass oblongButtonGadgetClass = (WidgetClass) &oblongButtonGadgetClassRec;


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
 *  OblongButtonEH - This event handler is used to interpret events for the
 *	Oblong Button gadget.
 *
 ****************************procedure*header*****************************
 */
static void
OblongButtonEH(Widget w, XtPointer client_data, XEvent *event,
	       Boolean *continue_to_dispatch)
{
	OlVirtualEventRec	ve;

	OlLookupInputEvent(w, event, &ve, OL_DEFAULT_IE);

	switch (event->type)
	{
    		case ButtonPress:
    		case ButtonRelease:
				ButtonHandler(w, &ve);
				break;
    		case MotionNotify:
				MotionHandler(w, &ve);
				break;
    		case LeaveNotify:
    		case EnterNotify:
				CrossingHandler(w, &ve);
				break;
    		case Expose:
    		case GraphicsExpose:
			(*(XtSuperclass(w)->core_class.expose))
	    				(w, event, client_data);
			break;

    		default:
			OlWarning(dgettext(OlMsgsDomain,
				"received unhandled event in OblongButtonEH"));
			break;
    	}
}				/*  OblongButtonEH  */

/*
 *************************************************************************
 *
 * Class Procedures
 *
 ****************************class*procedures*****************************
 */


/*
 ************************************************************
 *
 * Initialize - This routine adds the event handler to the
 *	widget.
 *
 *********************function*header************************
 */
static void
GadgetInitialize(Widget request, Widget new, ArgList args, Cardinal *num_args)
{
    _OlAddEventHandler(new,
		       ExposureMask |
		       ButtonPressMask | ButtonReleaseMask |
		       ButtonMotionMask |
		       LeaveWindowMask | EnterWindowMask,
		       FALSE, OblongButtonEH, NULL);

}				/*  Initialize  */


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
 *  Highlight - this function changes the state of the oblong
 *	button to the highlight state.  It draws the button
 *	in the highlight state.
 *
 *********************function*header************************
 */
/* changed from an action to a convenience routine */
static void
Highlight(Widget w)
{
	OblongButtonWidget bw = (OblongButtonWidget)w;
	ButtonPart *bp;

	bp = find_button_part(bw);

	if (_OlIsGadget(bw))
		(void) _OlUngrabPointer(XtParent(w));
	else
		(void) _OlUngrabPointer(w);

	if (!bp->set && !bp->busy)  {
		_OlPopdownTrailingCascade(w, False);
		_OlDrawHighlightButton((ButtonWidget)bw);
		bp->set = TRUE;
	}

}	/* Highlight */


/*
 ************************************************************
 *
 *  Normal - this function changes the state of the oblong
 *	button to the normal state.  It draws the button
 *	in the normal or default state.
 *
 *********************function*header************************
 */
static void
Normal(Widget w)
{
	OblongButtonWidget bw = (OblongButtonWidget)w;
	ButtonPart *bp;

	bp = find_button_part(bw);
	
	if (bp->set)  {
		_OlDrawNormalButton((ButtonWidget)bw);
		bp->set = FALSE;
	}

}	/* Normal */


/*
 ************************************************************
 *
 *  Notify - this function is called on a button one up
 *	event.  It invokes the users callback and changes
 *	the state of the oblong button back to normal.
 *
 *
 *********************function*header************************
 */
/*
 *changed from an action to a convenience routine
 *rm event, params, and num_params
 */
static void
Notify(Widget w)
{
	OblongButtonWidget bw = (OblongButtonWidget)w;
	ButtonPart *bp;

	bp = find_button_part(bw);

	if (bp->busy || bp->internal_busy)  {
		(void) _OlBeepDisplay((Widget)bw, 1);
	}
	else if (bp->set)  {
		bp->set = FALSE;
		bp->internal_busy = TRUE;
		_OlDrawNormalButton((ButtonWidget)bw);
		XFlush(XtDisplayOfObject(w));
		bp->internal_busy = FALSE;

		_OlManagerCallPreSelectCBs(w, (XEvent *)NULL);

		if(XtHasCallbacks(w, XtNselect) == XtCallbackHasSome) {
			XtCallCallbacks((Widget)bw, XtNselect, NULL);
		}

		_OlManagerCallPostSelectCBs(w, (XEvent *)NULL);

		if (XtHasCallbacks(w, XtNpostSelect) == XtCallbackHasSome) {
			XtCallCallbacks((Widget)bw, XtNpostSelect, NULL);
		}

		_OlDrawNormalButton((ButtonWidget)bw);
	}

}	/* Notify */

/*
 ************************************************************
 *
 *  MenuHighlight - this function changes the state of the oblong
 *	button to the highlight state.  It draws the button
 *	in the highlight state.
 *
 *********************function*header************************
 */
static void
MenuHighlight(Widget w)
{
	OblongButtonWidget bw = (OblongButtonWidget)w;
	OlDefine sb;
	ButtonPart *bp;

	bp = find_button_part(bw);

	sb = bp->shell_behavior;

        if (sb == PressDragReleaseMenu ||
		sb == StayUpMenu ||
		sb == PinnedMenu ||
		sb == UnpinnedMenu)  {

		Highlight(w);
	}

}	/* MenuHighlight */

/*
 ************************************************************
 *
 *  MenuNormal - this function changes the state of the oblong
 *	button to the normal state.  It draws the button
 *	in the normal or default state.
 *
 *********************function*header************************
 */
static void
MenuNormal(Widget w)
{
	OblongButtonWidget bw = (OblongButtonWidget)w;
	int sb;
	ButtonPart *bp;

	bp = find_button_part(bw);

	sb = bp->shell_behavior;

        if (sb == PressDragReleaseMenu ||
		sb == StayUpMenu ||
		sb == PinnedMenu ||
		sb == UnpinnedMenu)  {
		Normal(w);
	}

}	/* MenuNormal */


/*
 ************************************************************
 *
 *  MenuNotify - this function is called on a button one up
 *	event.  It invokes the users callback and changes
 *	the state of the oblong button back to normal.
 *
 *********************function*header************************
 */
static void
MenuNotify(Widget w)
{
	OblongButtonWidget bw = (OblongButtonWidget)w;
	int sb;
	ButtonPart *bp;

	bp = find_button_part(bw);

	sb = bp->shell_behavior;

        if (sb == PressDragReleaseMenu ||
		sb == StayUpMenu ||
		sb == PinnedMenu ||
		sb == UnpinnedMenu)  {
		Notify(w);
	}

}	/* MenuNotify */


/*
 ************************************************************
 *
 *  SetDefault - this function is called on a button one up
 *	event.  It invokes the users callback and changes
 *	the state of the oblong button back to normal.
 *
 *********************function*header************************
 */
/*
	changed from an action to a convenience,
	now, the 2nd parameter is a OlVirtualName
 */
static void
SetDefault(Widget w, OlVirtualName activation_type)
{
	OblongButtonWidget bw = (OblongButtonWidget)w;
        ShellBehavior           sb;
	ButtonPart *bp;
        static Arg              notify[] = {
                { XtNdefault,   (XtArgVal) True }
        };

	bp = find_button_part(bw);

        sb = bp->shell_behavior;

        if (sb != PressDragReleaseMenu &&
            sb != StayUpMenu &&
            sb != PinnedMenu &&
            sb != UnpinnedMenu)
                return;

	if (activation_type == OL_MENUDEFAULT ||
	    activation_type == OL_MENUDEFAULTKEY) {

	if (_OlIsGadget(bw))
		(void) _OlUngrabPointer(XtParent(w));
	else
		(void) _OlUngrabPointer(w);
	}

        if (bp->is_default == FALSE) {
                XtSetValues(w, notify, 1);
                _OlSetDefault(w, True);
	}

}	/* SetDefault */

/*
 *************************************************************************
 * ActivateWidget - this routine is used to activate the callbacks of
 * this widget.
 ****************************procedure*header*****************************
 */
static Boolean
ActivateWidget (Widget w, OlVirtualName activation_type, XtPointer call_data)
{
	Boolean consumed = False;
	
	switch (activation_type)
	{
		case OL_SELECT:
		case OL_SELECTKEY:
			consumed = True;
			Highlight (w);
			Notify (w);
			break;
		case OL_MENUDEFAULT:
		case OL_MENUDEFAULTKEY:
			consumed = True;
			SetDefault (w, activation_type);
			break;
	}
	return (consumed);
	
} /* END OF ActivateWidget() */

static void
ButtonHandler (Widget w, OlVirtualEvent ve)
{
	switch (ve->virtual_name)
	{
		case OL_SELECT:
			ve->consumed = True;
			if (ve->xevent->type == ButtonPress)
				Highlight (w);
			else
				Notify (w);
			break;
		case OL_MENU:
			ve->consumed = True;
			if (ve->xevent->type == ButtonPress)
				MenuHighlight (w);
			else
				MenuNotify (w);
			break;
		case OL_MENUDEFAULT:
			ve->consumed = True;
			SetDefault (w, ve->virtual_name);
			break;
		default:
			if (ve->xevent->type == ButtonRelease)
			{
				ve->consumed = True;
				Normal (w);
			}
			break;
	}
} /* end of ButtonHandler */

static void
CrossingHandler (Widget w, OlVirtualEvent ve)
{
	switch (ve->virtual_name)
	{
		case OL_SELECT:
			ve->consumed = True;
			if (ve->xevent->type == EnterNotify)
				Highlight (w);
			else
				Normal (w);
			break;
		case OL_MENU:
			ve->consumed = True;
			if (ve->xevent->type == EnterNotify)
				MenuHighlight(w);
			else
				MenuNormal(w);
			break;
		default:
			if (ve->xevent->type == LeaveNotify)
			{
				ve->consumed = True;
				Normal (w);
			}
			break;
	}
} /* end of CrossingHandler */

static void
MotionHandler (Widget w, OlVirtualEvent ve)
{
	switch (ve->virtual_name)
	{
		case OL_MENU:
			ve->consumed = True;
			/*
			MenuHighlight (w);
			*/
			break;
		case OL_MENUDEFAULT:
			ve->consumed = True;
			SetDefault (w, OL_UNKNOWN_BTN_INPUT);
			break;
	}
} /* end of MotionHandler */

/*
 *************************************************************************
 *
 * Public Procedures
 *
 ****************************public*procedures****************************
 */

/*  There are no public procedures  */
