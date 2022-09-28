#pragma ident	"@(#)Primitive.c	302.15	97/03/26 lib/libXol SMI"/*@(#)primitive:Primitive.c 1.41 */

/*
 *      Copyright (C) 1986,1991  Sun Microsystems, Inc
 *                      All rights reserved.
 *              Notice of copyright on this source code
 *              product does not indicate publication.
 *
 * RESTRICTED RIGHTS LEGEND: Use, duplication, or disclosure by
 * the U.S. Government is subject to restrictions as set forth
 * in subparagraph (c)(1)(ii) of the Rights in Technical Data
 * and Computer Software Clause at DFARS 252.227-7013 (Oct. 1988)
 * and FAR 52.227-19 (c) (June 1987).
 *
 *      Sun Microsystems, Inc., 2550 Garcia Avenue,
 *      Mountain View, California 94043.
 *
 */
/*	Copyright (c) 1990 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*************************************************************************
 *
 * Description:	This file along with Primitive.h and PrimitiveP.h implements
 *		the OPEN LOOK Primitive Widget.  This widget's class
 *		includes fields/procedures common to all primitive widgets.
 *
 *******************************file*header******************************/

						/* #includes go here	*/

#include <stdio.h>
#include <stdlib.h>

#include <X11/IntrinsicP.h>
#include <X11/StringDefs.h>
#include <X11/Shell.h>

#include <Xol/OpenLookP.h>
#include <Xol/PrimitiveP.h>
#include <Xol/Error.h>
#include <Xol/OlI18nP.h>
#include <Xol/OlgxP.h>
#include <Xol/Manager.h>
#include <Xol/Menu.h>

/*************************************************************************
 *
 * Define global/static variables and #defines, and
 * Declare externally referenced variables
 *
 *****************************file*variables*****************************/

#define NULL_CHAR	((char)NULL)
#define NULL_WIDGET	((Widget)NULL)
#define PCLASS(w, r) (((PrimitiveWidgetClass)XtClass((w)))->primitive_class.r)

#define BYTE_OFFSET	XtOffsetOf(PrimitiveRec, primitive.dyn_flags)
static _OlDynResource dyn_res[] = {
{ { XtNbackground, XtCBackground, XtRPixel, sizeof(Pixel), 0,
	 XtRString, XtDefaultBackground }, BYTE_OFFSET, OL_B_PRIMITIVE_BG, NULL },
{ { XtNforeground, XtCForeground, XtRPixel, sizeof(Pixel), 0,
	 XtRString, XtDefaultForeground }, BYTE_OFFSET, OL_B_PRIMITIVE_FG, NULL },
{ { XtNfontColor, XtCFontColor, XtRPixel, sizeof(Pixel), 0,
	 XtRString, XtDefaultForeground }, BYTE_OFFSET,
	 OL_B_PRIMITIVE_FONTCOLOR, NULL },
{ { XtNinputFocusColor, XtCInputFocusColor, XtRPixel, sizeof(Pixel), 0,
	 XtRString, "Red" }, BYTE_OFFSET, OL_B_PRIMITIVE_FOCUSCOLOR, NULL },
{ { XtNborderColor, XtCBorderColor, XtRPixel, sizeof(Pixel), 0,
	 XtRString, XtDefaultForeground }, BYTE_OFFSET,
	 OL_B_PRIMITIVE_BORDERCOLOR, NULL },
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

					/* class procedures		*/
static Boolean	AcceptFocus(Widget w, Time *time);
static void	ClassInitialize(void);
static void	ClassPartInitialize(WidgetClass wc);
static void	Destroy(Widget w);
static void	GetValuesHook(Widget w, ArgList args, Cardinal *num_args);
static void	Initialize(Widget request, Widget new, ArgList args, Cardinal *num_args);
static Boolean	SetValues(Widget current, Widget request, Widget new, ArgList args, Cardinal *num_args);
static void 	PrimitiveQuerySCLocnProc(const Widget		target,
			const Widget		supercaret,
			const Dimension		sc_width,
			const Dimension		sc_height,
			unsigned int    *const	scale,	
			SuperCaretShape *const 	shape,
			Position        *const	x_center_return,
			Position        *const	y_center_return);

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


#define OFFSET(field)	XtOffsetOf(PrimitiveRec, primitive.field)
static XtResource resources[] = 
{
	/* This must be the first resource in the list */
  {XtNtextFormat, XtCTextFormat, XtROlStrRep,sizeof(OlStrRep),
   OFFSET(text_format),XtRCallProc,(XtPointer)_OlGetDefaultTextFormat
  },
  { XtNborderWidth, XtCBorderWidth, XtRDimension, sizeof(Dimension),
    XtOffset(PrimitiveWidget, core.border_width), XtRImmediate, (XtPointer)0
  },
  { XtNaccelerator, XtCAccelerator, XtRString, sizeof(String),
    OFFSET(accelerator), XtRString, (XtPointer) NULL
  },
  { XtNacceleratorText, XtCAcceleratorText, XtRString, sizeof(String),
    OFFSET(accelerator_text), XtRString, (XtPointer) NULL
  },
  { XtNconsumeEvent, XtCCallback, XtRCallback, sizeof(XtCallbackList),
    OFFSET(consume_event), XtRCallback, (XtPointer)NULL
  },
  { XtNfont, XtCFont, XtROlFont, sizeof(OlFont),
    OFFSET(font), XtRString,XtDefaultFont 
  },
  { XtNfontColor, XtCFontColor, XtRPixel, sizeof(Pixel),
    OFFSET(font_color), XtRString, XtDefaultForeground
  },
  { XtNforeground, XtCForeground, XtRPixel, sizeof(Pixel),
    OFFSET(foreground), XtRString, XtDefaultForeground
  },
  { XtNinputFocusColor, XtCInputFocusColor, XtRPixel, sizeof(Pixel),
    OFFSET(input_focus_color), XtRString, "Red"
  },
  { XtNmnemonic, XtCMnemonic, OlRChar, sizeof(char),
    OFFSET(mnemonic), XtRImmediate, (XtPointer) '\0'
  },
  { XtNreferenceName, XtCReferenceName, XtRString, sizeof(String),
    OFFSET(reference_name), XtRString, (XtPointer)NULL
  },
  { XtNreferenceWidget, XtCReferenceWidget, XtRWidget, sizeof(Widget),
    OFFSET(reference_widget), XtRWidget, (XtPointer)NULL
  },
  { XtNscale, XtCScale, XtROlScale, sizeof(int),
    OFFSET(scale), XtRImmediate, (XtPointer)OL_DEFAULT_POINT_SIZE
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
};
#undef OFFSET

/*************************************************************************
 *
 * Define Class Record structure to be initialized at Compile time
 *
 ***************************widget*class*record**************************/

PrimitiveClassRec primitiveClassRec =
{
	{
/* core_class fields      */
	 /* superclass         */    &widgetClassRec,
	 /* class_name         */    "Primitive",
	 /* widget_size        */    sizeof(PrimitiveRec),
	 /* class_initialize   */    ClassInitialize,
	 /* class_part_init    */    ClassPartInitialize,
	 /* class_inited       */    FALSE,
	 /* initialize         */    Initialize,
	 /* initialize_hook    */    (XtArgsProc)NULL,
	 /* realize            */    XtInheritRealize,
	 /* actions	       */    NULL,	/* See ClassInitialize */
	 /* num_actions        */    NULL,	/* See ClassInitialize */
	 /* resources          */    resources,
	 /* num_resources      */    XtNumber(resources),
	 /* xrm_class          */    NULLQUARK,
	 /* compress_motion    */    TRUE,
	 /* compress_exposure  */    TRUE,
	 /* compress_enterleave*/    TRUE,
	 /* visible_interest   */    FALSE,
	 /* destroy            */    Destroy,
	 /* resize             */    XtInheritResize,
	 /* expose             */    XtInheritExpose,
	 /* set_values         */    SetValues,
	 /* set values hook    */    (XtArgsFunc)NULL,
	 /* set values almost  */    XtInheritSetValuesAlmost,
	 /* get values hook    */    GetValuesHook,
	 /* accept_focus       */    AcceptFocus,
	 /* Version            */    XtVersion,
	 /* PRIVATE cb list    */    NULL,
	 /* tm_table           */    NULL,	/* See ClassInitialize */
	 /* query_geom         */    XtInheritQueryGeometry,
	 /* display_accelerator*/    XtInheritDisplayAccelerator,
	 /* extension	       */    NULL
	},
	{
/* primitive_class fields */     
	 /* reserved1		*/    NULL,
	 /* highlight_handler	*/    NULL,
	 /* traversal_handler	*/    NULL,
	 /* register_focus	*/    NULL,
	 /* activate		*/    NULL,
	 /* event_procs		*/    NULL,	/* See ClassInitialize */
	 /* num_event_procs	*/    NULL,	/* See ClassInitialize */
	 /* version		*/    OlVersion,
	 /* extension		*/    NULL,
	 /* dyn_data		*/    { dyn_res, XtNumber(dyn_res) },
	 /* transparent_proc	*/    NULL,
	 /* query_sc_locn_proc	*/    PrimitiveQuerySCLocnProc,
	}
};

/*************************************************************************
 *
 * Public Widget Class Definition of the Widget Class Record
 *
 *************************public*class*definition************************/

WidgetClass primitiveWidgetClass = (WidgetClass)&primitiveClassRec;

/*************************************************************************
 *
 * Class Procedures
 *
 ****************************class*procedures****************************/

/*************************************************************************
 * AcceptFocus - If this widget can accept focus then it is set here
 *		 FALSE is returned if focus could not be set
 ***************************function*header*******************************/

static Boolean
AcceptFocus(Widget w, Time *time)
{

    if (OlCanAcceptFocus(w, *time))
    {
	OlSetInputFocus(w, RevertToNone, *time);
	return (True);
    }

    return (False);

} /* AcceptFocus() */

/*************************************************************************
 * ClassInitialize - Sets up virtual translations
 ***************************function*header*******************************/

static void
ClassInitialize(void)
{
	PrimitiveWidgetClass pwc =
			(PrimitiveWidgetClass)primitiveWidgetClass;

			/* Register a converter for the mnemonics	*/

	 pwc->core_class.actions	= (XtActionList)_OlGenericActionTable;
	 pwc->core_class.num_actions	= (Cardinal)_OlGenericActionTableSize;
	 pwc->core_class.tm_table	= (String)_OlGenericTranslationTable;
	 pwc->primitive_class.event_procs = (OlEventHandlerList)
					_OlGenericEventHandlerList;
	 pwc->primitive_class.num_event_procs = (Cardinal)
					_OlGenericEventHandlerListSize;

} /* END OF ClassInitialize() */


/*************************************************************************
 * ClassPartInitialize - Provides for inheritance of the class procedures
 ***************************function*header*******************************/

static void
ClassPartInitialize(WidgetClass wc)
{
    PrimitiveWidgetClass	pc = (PrimitiveWidgetClass)wc;
    PrimitiveWidgetClass	sc = (PrimitiveWidgetClass)pc->core_class.superclass;
#ifdef SHARELIB
	void **__libXol__XtInherit = _libXol__XtInherit;
#undef _XtInherit
#define _XtInherit		(*__libXol__XtInherit)
#endif

			/* Generate warning if version is less than 2.0	*/

    if (pc->primitive_class.version != OlVersion &&
	pc->primitive_class.version < 2000)
    {
    	OlVaDisplayWarningMsg((Display *)NULL,
    			OleNinternal, OleTbadVersion,
    			OleCOlToolkitWarning, OleMinternal_badVersion,
    			pc->core_class.class_name,
    			pc->primitive_class.version, OlVersion);
    }

    if (pc->primitive_class.highlight_handler == XtInheritHighlightHandler)
	pc->primitive_class.highlight_handler =
				sc->primitive_class.highlight_handler;

    if (pc->primitive_class.register_focus == XtInheritRegisterFocus)
	pc->primitive_class.register_focus =
				sc->primitive_class.register_focus;

    if (pc->primitive_class.traversal_handler == XtInheritTraversalHandler)
	pc->primitive_class.traversal_handler =
				sc->primitive_class.traversal_handler;

    if (pc->primitive_class.activate == XtInheritActivateFunc)
	pc->primitive_class.activate = sc->primitive_class.activate;

    if (pc->primitive_class.transparent_proc == XtInheritTransparentProc)
	pc->primitive_class.transparent_proc =
				sc->primitive_class.transparent_proc;

    if(pc->primitive_class.query_sc_locn_proc == XtInheritSuperCaretQueryLocnProc)
	pc->primitive_class.query_sc_locn_proc =
				sc->primitive_class.query_sc_locn_proc;

    if (wc == primitiveWidgetClass)
		return;

    if (pc->primitive_class.dyn_data.num_resources == 0) {
		/* give the superclass's resource list to this class */
		pc->primitive_class.dyn_data = sc->primitive_class.dyn_data;
    }
    else {
	/* merge the two lists */
	_OlMergeDynResources(&(pc->primitive_class.dyn_data),
			     &(sc->primitive_class.dyn_data));
    } /* else */

    if (pc->primitive_class.query_sc_locn_proc == 
		XtInheritSuperCaretQueryLocnProc) {
	pc->primitive_class.query_sc_locn_proc =
		sc->primitive_class.query_sc_locn_proc;
    }
} /* ClassPartInitialize() */


/*************************************************************************
 * Destroy - Remove `w' from its managing ancestor's traversal list
 *************************************************************************/

static void
Destroy(Widget w)
{
	PrimitiveWidget	pw = (PrimitiveWidget)w;

	_OlDestroyKeyboardHooks(w);

	if (pw->primitive.accelerator)
		XtFree(pw->primitive.accelerator);
	if (pw->primitive.qualifier_text)
		XtFree(pw->primitive.qualifier_text);
	if (pw->primitive.accelerator_text)
		XtFree(pw->primitive.accelerator_text);
	if (pw->primitive.reference_name)
		XtFree(pw->primitive.reference_name);

}	/* Destroy() */

/*************************************************************************
 * GetValuesHook - check for XtNreferenceWidget and XtNreferenceName
 *		and return info according to the traversal list
 ***************************function*header*******************************/
static void
GetValuesHook(Widget w, ArgList args, Cardinal *num_args)
{
	_OlGetRefNameOrWidget(w, args, num_args);
}

/*************************************************************************
 * Initialize - Initializes primitive part
 ***************************function*header*******************************/

/* ARGSUSED */
static void
Initialize(Widget request, Widget new, ArgList args, Cardinal *num_args)
{
	PrimitiveWidget	pw	= (PrimitiveWidget)new;
	PrimitivePart *	pp	= &(pw->primitive);
 	int 		validScale;

	/* Make sure scale is supported by OLGX */
	validScale = OlgxGetValidScale(pp->scale);
	pp->scale = validScale;

	pp->has_focus = FALSE;		/* Can't have focus yet */

	if (pp->mnemonic)
		if (_OlAddMnemonic(new, (XtPointer)0, pp->mnemonic) !=
				OL_SUCCESS)
			pp->mnemonic = 0;

	if (pp->accelerator)
		if (_OlAddAccelerator(new, (XtPointer)0, pp->accelerator) !=
				OL_SUCCESS) {
			pp->accelerator = (String)NULL;
			pp->accelerator_text = (String)NULL;
		}

	pp->qualifier_text = (String)NULL;
	pp->meta_key = False;
	if (pp->accelerator) {
		pp->accelerator = XtNewString(pp->accelerator);
		if (!(pp->accelerator_text))
			_OlMakeAcceleratorText(new, pp->accelerator,
			    &pp->qualifier_text, &pp->meta_key,
			    &pp->accelerator_text);
		else
			pp->accelerator_text=XtNewString(pp->accelerator_text);
	} else if (pp->accelerator_text)
		pp->accelerator_text = XtNewString(pp->accelerator_text);

		/* add widget to traversal list */
	if (pp->reference_name)
		pp->reference_name = XtNewString(pp->reference_name);

	_OlUpdateTraversalWidget(new, pp->reference_name,
				 pp->reference_widget, True);

	_OlInitDynResources(new, &(((PrimitiveWidgetClass)
			(new->core.widget_class))->primitive_class.dyn_data));
	_OlCheckDynResources(new, 
		&(((PrimitiveWidgetClass)(new->core.widget_class))->
		primitive_class.dyn_data), args, *num_args);
	pp->ext_part = _OlGetVendorPartExtension(_OlGetShellOfWidget(new));
	pp->input_focus_feedback = _OlInputFocusFeedback(new);
} /* Initialize() */


/*************************************************************************
 * SetValues - Sets up internal values based on changes made to external
 *	       ones
 ***************************function*header*******************************/

/* ARGSUSED */
static Boolean
SetValues (Widget current, Widget request, Widget new, ArgList args, Cardinal *num_args)
{
	PrimitiveWidget	current_pw = (PrimitiveWidget)current;
	PrimitivePart	*curPart   = &current_pw->primitive;
	PrimitiveWidget	new_pw     = (PrimitiveWidget)new;
	PrimitivePart	*newPart   = &new_pw->primitive;

	Boolean		redisplay  = False;

	/* always reset text format */
	newPart->text_format = curPart->text_format;

#define CHANGED(field)	(newPart->field != curPart->field)

        /* Make sure scale is supported by OLGX */ 
        if (CHANGED(scale)) { 
            int validScale = OlgxGetValidScale(newPart->scale); 
            newPart->scale = validScale; 
            redisplay = True; 
        } 

	if (CHANGED(reference_name))	/* this has higher preference */
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
			/* no need to keep the info? */
		if (curPart->reference_name != NULL)
		{
			XtFree(curPart->reference_name);
			curPart->reference_name = NULL;
		}

		_OlUpdateTraversalWidget(new, NULL,
					 newPart->reference_widget, False);
	}

	if (CHANGED(mnemonic)) {
		if (curPart->mnemonic)
			_OlRemoveMnemonic(new, (XtPointer)0, False,
					curPart->mnemonic);
		if (newPart->mnemonic)
			if (_OlAddMnemonic(new, (XtPointer)0,
			    newPart->mnemonic) != OL_SUCCESS)
				newPart->mnemonic = 0;
		redisplay = True;
	}

	if (!XtIsSensitive(new) &&
	    (OlGetCurrentFocusWidget(new) == new)) {
		/*
		 * When it becomes insensitive, need to move focus elsewhere,
		 * if this widget currently has focus.
		 */
		OlMoveFocus(new, OL_IMMEDIATE, CurrentTime);
		newPart->has_focus = FALSE;
	}

	{
	Boolean changed_accelerator=CHANGED(accelerator);
	Boolean changed_accelerator_text=CHANGED(accelerator_text);

        if (changed_accelerator) {
		/*
		 * Remove the old accelerator
		 */
                if (curPart->accelerator)
                        _OlRemoveAccelerator(new, (XtPointer)0,
                                False, curPart->accelerator);

		/*
		 * Install the new accelerator
		 */
                if (newPart->accelerator) {
                        if(_OlAddAccelerator(new, (XtPointer)0,
                                        newPart->accelerator)==OL_SUCCESS) {
				/*
				 * succeeds, reset the accelerator text
				 */
                                newPart->accelerator =
					XtNewString(newPart->accelerator);
				if (!changed_accelerator_text)
					newPart->accelerator_text = (String)NULL;
			} else {
				/*
				 * fails, don't install the accelerator text
				 */
				newPart->accelerator = (String)NULL;
				newPart->accelerator_text = (String)NULL;
			}
                }

		/*
		 * Free the old accelerator string
		 */
                if (curPart->accelerator)
                        XtFree(curPart->accelerator);
        }	

	if (changed_accelerator || changed_accelerator_text) {
		/*
		 * Add the new accelerator text.
		 */
		newPart->qualifier_text = (String)NULL;
		newPart->meta_key = False;

                if (changed_accelerator_text && newPart->accelerator_text)
			newPart->accelerator_text =
				XtNewString(newPart->accelerator_text);
                else if (newPart->accelerator)
			_OlMakeAcceleratorText(new, newPart->accelerator,
			    &newPart->qualifier_text, &newPart->meta_key, 
			    &newPart->accelerator_text);
 
		/*
		 * Free the old accelerator text.
		 */
                if (curPart->qualifier_text)
                        XtFree(curPart->qualifier_text);

                if (curPart->accelerator_text)
                        XtFree(curPart->accelerator_text);
 
                redisplay = True;
        }	
	}

	/* handle dynamic resources */
	_OlCheckDynResources(new, 
		&(((PrimitiveWidgetClass)(new->core.widget_class))->
		primitive_class.dyn_data), args, *num_args);

	return (redisplay);
#undef CHANGED
}	/* SetValues() */

static void
PrimitiveQuerySCLocnProc(const Widget		w,
			const Widget		supercaret,
			const Dimension		sc_width,
			const Dimension		sc_height,
			unsigned int    *const	scale,	
			SuperCaretShape *const 	shape,
			Position        *const	x_center_return,
			Position        *const	y_center_return)
{
	PrimitiveWidget		target 	= (PrimitiveWidget)w;
	Widget		fwp	   = (XtIsShell(w) ? w : XtParent(w));
	PrimitiveWidgetClass fwwc = (PrimitiveWidgetClass)target->core.widget_class;
	SuperCaretShape rs = *shape;
	Dimension	fwp_width, fwp_height;

	if(!XtIsSubclass(w,primitiveWidgetClass) ||
		fwwc->primitive_class.query_sc_locn_proc == NULL) {
		*shape = SuperCaretNone;
		return;
	}
		
	fwp_width  = fwp->core.width,
	fwp_height = fwp->core.height;

	*x_center_return = (Position)0;
	*y_center_return = (Position)0;

	if (!(XtIsSubclass(w, managerWidgetClass) || XtIsShell(w))) {
		Widget	shell = _OlGetShellOfWidget(w);

		if(shell != NULL && XtIsSubclass(shell,menuShellWidgetClass))
			*shape = SuperCaretLeft;
		else
			*shape = (((int)fwp_width > (int)fwp_height)
					? SuperCaretBottom
					: SuperCaretLeft);
	} else 
		*shape = SuperCaretNone;

	if(target->primitive.scale != *scale || rs != *shape) {
		*scale = target->primitive.scale;
		return; /* try again */
	} 

	switch (*shape) {
		case SuperCaretBottom:
			*x_center_return += target->core.width / 2;
			*y_center_return += target->core.height;
			break;
		case SuperCaretLeft:
			*y_center_return += target->core.height / 2;
			break;
	}
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
