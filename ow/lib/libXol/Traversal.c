#pragma ident	"@(#)Traversal.c	302.11	97/03/26 lib/libXol SMI"	/* mouseless:Traversal.c 1.19	*/

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
 * Description:	Traversal Convenience Functions
 *
 *************************************************************************/


#include <libintl.h>
#include <stdio.h>

#include <X11/IntrinsicP.h>
#include <X11/StringDefs.h>

#include <Xol/Error.h>
#include <Xol/EventObjP.h>
#include <Xol/ManagerP.h>
#include <Xol/PrimitiveP.h>
#include <Xol/OlI18nP.h>
#include <Xol/OpenLookP.h>
#include <Xol/TextEdit.h>
#include <Xol/TextField.h>
#include <Xol/TextLine.h>
#include <Xol/ListPaneP.h>
#include <Xol/VendorI.h>
#include <Xol/array.h>


/*************************************************************************
 *
 * Define global/static variables and #defines, and
 * Declare externally referenced variables
 *
 *************************************************************************/

#define FOCUS_GADGET(fd)	(fd->focus_gadget)


/*************************************************************************
 *
 * Forward Procedure definitions listed by category:
 *		1. Private Procedures
 *		2. Action  Procedures
 *		3. Public  Procedures
 *
 *************************************************************************/

					/* private procedures */
					
static Widget GetRealFocusWidget (Widget);


/*************************************************************************
 *
 * Define Translations and Actions
 *
 ***********************widget*translations*actions**********************/

/*************************************************************************
 *
 * Private Procedures
 *
 ***************************private*procedures***************************/


static Widget
GetRealFocusWidget(Widget w)
{
    OlRegisterFocusFunc focus_func = NULL;

    _OlGetClassField(w, register_focus, focus_func);
    return( (focus_func == NULL) ? NULL : (*focus_func)(w) );
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

/***************************function*header*******************************
 * OlCallAcceptFocus - Returns the result of calling `w's accept_focus 
 * 		       procedure
 *		       Returns `False' if `w's accept_focus proc. is null
 */
extern Boolean
OlCallAcceptFocus (Widget w, Time time)
{
    XtAcceptFocusProc accept_focus = XtClass(w)->core_class.accept_focus;
    Boolean retval;
    
    GetToken();
    retval = accept_focus != NULL && (*accept_focus)(w, &time);
    ReleaseToken();
    return retval;
} /* OlCallAcceptFocus() */

/***************************function*header*******************************
 * _OlCallHighlightHandler - Calls `w's highlight_handler class proc. if
 *			     it has one.  Also, set has_focus.
 */
extern void
_OlCallHighlightHandler (Widget w, OlDefine highlight_type)
{
    WidgetClass		wc = XtClass(w);
    OlHighlightProc	handler = NULL;

		/* to prevent the case: the current destroyed widget
			has the input focus */
    if (w->core.being_destroyed == True)
	return;

    if (XtIsVendorShell(w))
    {
	OlVendorClassExtension ext = _OlGetVendorClassExtension(wc);

	if (ext != NULL)
		handler = ext->highlight_handler;

    } else
    {
	    WidgetClass		wc_special = _OlClass(w);
	    Boolean		has_focus = (highlight_type == OL_IN);

	    if (wc_special == primitiveWidgetClass) {
		((PrimitiveWidget)w)->primitive.has_focus = has_focus;
		handler = ((PrimitiveWidgetClass)wc)->primitive_class.
							highlight_handler;
	    } else if (wc_special == managerWidgetClass) {
		((ManagerWidget)w)->manager.has_focus = has_focus;
		handler = ((ManagerWidgetClass)wc)->manager_class.
							highlight_handler;
	    } else if (wc_special == eventObjClass) {
		((EventObj)w)->event.has_focus = has_focus;
		handler = ((EventObjClass)wc)->event_class.highlight_handler;
	    }
    }

    if (handler != (OlHighlightProc)NULL) 
        (*handler)(w, highlight_type);

} /* _OlCallHighlightHandler() */

/***************************function*header*******************************
 * OlCanAcceptFocus - Returns True if 'w' can accept focus.
 */
/* ARGSUSED */
extern Boolean
OlCanAcceptFocus (Widget w, Time time)
{
    XWindowAttributes	win_att_return;
    Widget		win_anc;	/* window ancestor */
    Boolean		retval;

    GetToken();

    win_anc = (_OlIsGadget(w) == True) ? XtParent(w) : w;

    retval =
	(XtIsShell(w) ||
	 _OlMouseless(w) ||
	 XtIsSubclass(w, textEditWidgetClass) ||
	 XtIsSubclass(w, textLineWidgetClass) ||
	 XtIsSubclass(w, textFieldWidgetClass)) &&
	     (XtClass(w)->core_class.accept_focus != NULL &&
	      (w->core.being_destroyed == False) &&
	      (XtIsShell(w) || XtIsManaged(w) ||
	      (XtIsSubclass(w, textFieldWidgetClass) && XtParent(w) &&
	      XtIsSubclass(XtParent(w), listPaneWidgetClass))) &&
	      XtIsRealized(win_anc) && XtIsSensitive(w) &&
	      XGetWindowAttributes(XtDisplay(win_anc), XtWindow(win_anc),
				   &win_att_return) != 0 &&
	      win_att_return.map_state == IsViewable);

    ReleaseToken();
    return retval;
} /* END OF OlCanAcceptFocus() */

/***************************function*header*******************************
 * Gets the traversal_list from the VendorShell and deletes 'w' from it.
 * Reassigns focus if 'w' had it.
 */
extern void
_OlDeleteDescendant (Widget w)
{
    OlVendorPartExtension part_ext;
    OlFocusData *	focus_data = _OlGetFocusData(w, &part_ext);
    _OlArray		list;
    int			descendant_pos;

    /* ignore if widget is not descendant of a vendor shell */
    if (focus_data == NULL)
	return;					/* return silently */

    list = &(focus_data->traversal_list);	/* get list */
						/* get position within list */
    descendant_pos = _OlArrayFind(list, w);

    /* If the shell is being destroyed, return now */
    if (part_ext->vendor->core.being_destroyed == True)
	return;

    /* Move focus to next widget if this widget currently has focus */
    if (OlGetCurrentFocusWidget(w) == w)
    {
	(void)OlMoveFocus(w, OL_NEXTFIELD, CurrentTime);
    }

    if (FOCUS_GADGET(focus_data) == w)
    {
	FOCUS_GADGET(focus_data) = (Widget)NULL;
    }

    if (focus_data->current_focus_widget == w)
    {
	focus_data->current_focus_widget = NULL;
    }

    if (focus_data->initial_focus_widget == w)
    {
	focus_data->initial_focus_widget = NULL;
    }

    /* If descendant not in list, return now */
    if (descendant_pos == _OL_NULL_ARRAY_INDEX)
	return;

    _OlArrayDelete(list, descendant_pos);

} /* _OlDeleteDescendant() */

/***************************function*header*******************************
 * _OlFreeRefName - Free the reference name string
 *			this function is used in Traversal.c and Vendor.c
 *	The assumption is that the reference_name string is not NULL.
 */

void
_OlFreeRefName (Widget w)
{
	WidgetClass	wc_special = _OlClass(w);

	if (wc_special == primitiveWidgetClass)
	{
		XtFree(((PrimitiveWidget)w)->primitive.reference_name);
		((PrimitiveWidget)w)->primitive.reference_name = NULL;
		return;
	}
	if (wc_special == eventObjClass)
	{
		XtFree(((EventObj)w)->event.reference_name);
		((EventObj)w)->event.reference_name = NULL;
		return;
	}
	if (wc_special == managerWidgetClass)
	{
		XtFree(((ManagerWidget)w)->manager.reference_name);
		((ManagerWidget)w)->manager.reference_name = NULL;
		return;
	}
} /* END OF FreeRefName */

/***************************function*header*******************************
 * OlHasFocus - Returns True if `w' is a primitive that currently has focus
 */
extern Boolean
OlHasFocus (Widget w)
{
    return ( OlGetCurrentFocusWidget(w) == w );
} /* OlHasFocus() */

/***************************function*header*******************************
 * Gets the traversal_list from the VendorShell and inserts 'w' into it
 * before reference widget or at end if no reference given.
 */
extern void
_OlInsertDescendant (Widget w)
{
	Arg	args[2];
	String	ref_name = NULL;
	Widget	ref_widget = NULL;

	XtSetArg (args[0], XtNreferenceName,   (XtArgVal) &ref_name);
	XtSetArg (args[1], XtNreferenceWidget, (XtArgVal) &ref_widget);
	XtGetValues (w, args, 2);

	_OlUpdateTraversalWidget(w, ref_name, ref_widget, True);
} /* _OlInsertDescendant() */

/***************************function*header*******************************
 * OlMoveFocus - Moves focus relative to `w' in `direction'
 */

extern Widget
OlMoveFocus (Widget w, OlVirtualName direction, Time time)
{
    OlVendorClassExtension ext;
    Widget		ancestor;
    Widget		shell;
    Widget		retval;
    OlTraversalFunc	trav_func = NULL;

    GetToken();

    /* If mouseless mode is disabled, return NULL now */
    if (!_OlMouseless(w) &&
	!XtIsSubclass(w, textEditWidgetClass) &&
	!XtIsSubclass(w, textLineWidgetClass) &&
	!XtIsSubclass(w, textFieldWidgetClass)) {
	    ReleaseToken();
	    return (NULL);
    }

    /* adjust 'w'.  If widget would register some ancestor,
	use it here rather than 'w'.
	('ancestor' used temporarily here)
     */
    if ((ancestor = GetRealFocusWidget(w)) != NULL)
	w = ancestor;

    /* shell is needed below for intra- and inter-object traversal */
    shell = _OlGetShellOfWidget(w);

    switch(direction) {

    case OL_PREVFIELD :
    case OL_PREVIOUS :		/* backwards compatible */
    case OL_NEXTFIELD :
    case OL_NEXT :		/* backwards compatible */
    case OL_IMMEDIATE :
    case OL_CURRENT :		/* backwards compatible */
	if (XtIsVendorShell(shell) &&
	  ((ext = _OlGetVendorClassExtension(XtClass(shell))) != NULL))
	{
	    trav_func	= ext->traversal_handler;
	    ancestor	= shell;

	    /* re-map direction type for backwards compatibility */
	    direction	= (
			direction == OL_PREVIOUS ? OL_PREVFIELD :
			direction == OL_NEXT ? OL_NEXTFIELD :
			direction == OL_CURRENT ? OL_IMMEDIATE :
			direction);
	}
	break;

    case OL_MOVEUP :
    case OL_MOVEDOWN :
    case OL_MOVERIGHT :
    case OL_MOVELEFT :
    case OL_MULTIUP :
    case OL_MULTIDOWN :
    case OL_MULTIRIGHT :
    case OL_MULTILEFT :
	for (ancestor = w; ancestor != shell; ancestor = XtParent(ancestor))
	{
	    _OlGetClassField(ancestor, traversal_handler, trav_func);
	    if (trav_func != NULL)
		break;
	}

	/* if no handler found, use vendor shell's */
	if ((trav_func == NULL) &&
	  ((ext = _OlGetVendorClassExtension(XtClass(shell))) != NULL))
	    trav_func = ext->traversal_handler;

	break;

    default :
	OlWarning(dgettext(OlMsgsDomain,
			"OlMoveFocus: Invalid direction specified"));
	break;

    } /* end switch */

    retval = ((trav_func == NULL) ? NULL :
	      (*trav_func)(ancestor, w, direction, time));
    ReleaseToken();
    return retval;
} /* OlMoveFocus() */

/***************************function*header*******************************
 * return id of widget which currently has focus (if any)
 * within this window group
 */
Widget
OlGetCurrentFocusWidget (Widget w)
{
    OlFocusData *focus_data;

    GetToken();

    if ((focus_data = _OlGetFocusData(w, NULL)) == NULL)
    {
	OlWarning(dgettext(OlMsgsDomain,
	  "OlGetCurrentFocusWidget: shell is not descendant of VendorShell"));
	ReleaseToken();
	return (NULL);
    }

    ReleaseToken();
    return (focus_data->current_focus_widget);
}

/***************************procedure*header*****************************
 *
 * This routine is meant to be used by widgets that take input focus.
 *
 * When they are initialized, such widgets should call this function to
 * let their base window know that it contains at least one widget that
 * can take focus.   Widgets that can take focus should also call this
 * routine when they actually gain the input focus, so that if focus is
 * moved to another base window, the last widget to have focus in
 * this base window will receive it if focus is ever restored to the
 * base window.
 *
 * The `override_current' argument controls whether or not the current
 * focus widget is overridden by the call.  This argument should be
 * be False when widgets call this function from their initialize
 * procs. so that the first widget to register itself (ie. the first
 * widget created) or the application specified focus widget will get
 * input focus when the base window is realized.  The argument should
 * be "True" when a widget calls this function as a result of gaining
 * the input focus so that any current setting will be overridden.
 *
 * Note that the Primitive class does all of this work and more so
 * that subclasses of Primitive get it for free.
 */
void
_OlRegisterFocusWidget (Widget w, Boolean override_current)
{
    OlFocusData * focus_data = _OlGetFocusData(w, NULL);

    if (focus_data == NULL)
    {
	OlWarning(dgettext(OlMsgsDomain,
	  "_OlRegisterFocusWidget: widget is not descendant of VendorShell"));
	return;
    }

    if (focus_data->initial_focus_widget == NULL || override_current == True)
    {
	if (((w = GetRealFocusWidget(w)) != NULL) &&
	  (_OlArrayFind(&(focus_data->traversal_list), w) !=
		_OL_NULL_ARRAY_INDEX))
	  {
	    focus_data->initial_focus_widget = w;
	  }
    }
} /* END OF _OlRegisterFocusWidget() */

/***************************procedure*header*****************************
 _OlSetCurrentFocusWidget- called by OlAction due to FocusChange.

    For gadgets: OlAction has taken care of adjusting 'w' for gadgets.
    For FocusIn, 'w' is already correctly set to the gadget which
    receives focus.  For FocusOut, 'w' is not considered.  Instead,
    only current_focus_widget is considered.
*/
void
_OlSetCurrentFocusWidget (Widget w, OlDefine state)
{
    OlFocusData * focus_data = _OlGetFocusData(w, NULL);
    int i =0;
    Arg args[2];

    if (focus_data == NULL)
    {
	return;
    }

    if (state == OL_IN)
    {
	/* Establish current_focus_widget and initial_focus_widget:
	   Save widget that is registered to take focus
	   (initial_focus_widget) and call _OlRegisterFocusWidget.
	   If initial_focus_widget is changed by _OlRegisterFocusWidget,
	   set current_focus_widget to it; else set it to 'w'.
	 */
	Widget prev_widget = focus_data->initial_focus_widget;

	_OlRegisterFocusWidget(w, True);

	focus_data->current_focus_widget =
	    (prev_widget != focus_data->initial_focus_widget) ?
		    focus_data->initial_focus_widget : w;

	_OlCallHighlightHandler(w, state);	/* call highlight handler */

	 _OlVendorSetSuperCaretFocus(_OlGetShellOfWidget(w), w);

	if (focus_data->current_focus_widget)
	{
		focus_data->initial_focus_widget =
				focus_data->current_focus_widget;
	}

	 /* Make sure only one textedit or textline or textfield has the caret
                visible at any one time */
        if(focus_data->initial_focus_widget != prev_widget)
           if(prev_widget != NULL &&
                (XtIsSubclass(prev_widget, textLineWidgetClass) || 
		(XtIsSubclass(prev_widget, textEditWidgetClass) &&
                 XtParent(prev_widget) != focus_data->initial_focus_widget) ||
                (XtIsSubclass(prev_widget, textFieldWidgetClass) &&
                prev_widget != XtParent(focus_data->initial_focus_widget)))){
		Widget w;

		if(XtIsSubclass(prev_widget, textFieldWidgetClass)) 
			XtVaGetValues(prev_widget,
					XtNtextEditWidget, &w, NULL);
		else
			w = prev_widget;
                i=0;
                XtSetArg(args[i],XtNcursorVisible,FALSE); i++;
                XtSetValues(w,args,i);
        }

	/* The "->activate_on_focus" field will have been set when a
	   mnemonic was pressed and focus was moved here. Now that focus
	   has actually arrived, we can activate the widget.  Waiting
	   like this ensures that the focus move is done before we do
	   something that may move the focus again; an example is when the
	   user presses a mnemonic for a button that pops up a dialog box.
	 */
	if (focus_data->activate_on_focus == w)
	    (void) OlActivateWidget(w, OL_SELECTKEY, (XtPointer)0);
	focus_data->activate_on_focus = 0;

    } else {
	/* Consider gadgets: OlAction adjusts 'w' to be the object to
	   *receive* focus.  This is not used here for FocusOut since
	   current_focus_widget indicates the current widget with focus.
	   Correct parent/child relation was checked but broke due to
	   upwards of 3 FocusOut events being generated by the Intrinsics
	   for the shell.  This would NULLify current_focus_widget and
	   the highlight handler would never be called for the correct
	   object.
	*/
	if ((focus_data->current_focus_widget != NULL) &&
	  _OlIsGadget(focus_data->current_focus_widget))
	    w = focus_data->current_focus_widget;

	_OlCallHighlightHandler(w, OL_OUT);

	_OlVendorSetSuperCaretFocus(_OlGetShellOfWidget(w), (Widget)NULL);

	focus_data->current_focus_widget = NULL;
    }

} /* _OlSetCurrentFocusWidget */

/***************************procedure*header*****************************
 *
 * OlSetInputFocus - set X Focus to appropriate window.
 *
 * Keep focus on shell and use XtSetKeyboard to redirect focus to descendant.
 * If click-to-type focus model, set focus to the shell after call to
 * XtSetKeyboardFocus.
 */
void
OlSetInputFocus (Widget w, int revert_to, Time time)
{
    OlVendorPartExtension part_ext;
    OlFocusData *	focus_data;
    Boolean		is_gadget;
    Widget *		cfw;
    Widget *		fgadget;

    GetToken();

    if (w == (Widget)NULL)
    {
	OlWarning(dgettext(OlMsgsDomain,
			"OlSetInputFocus: null Widget id"));
	ReleaseToken();
	return;
    }
    else if ((focus_data = _OlGetFocusData(w, &part_ext)) ==
	     (OlFocusData*)NULL ||
	     (!XtIsShell(w) &&
	      !_OlMouseless(w) &&
	      !XtIsSubclass(w, textEditWidgetClass) &&
	      !XtIsSubclass(w, textLineWidgetClass) &&
	      !XtIsSubclass(w, textFieldWidgetClass)))
    {
	ReleaseToken();
	return;
    }

    cfw		= &(focus_data->current_focus_widget);
    fgadget	= &FOCUS_GADGET(focus_data);
    is_gadget	= _OlIsGadget(w);

    if (is_gadget == False)
    {
	*fgadget = NULL;
    }
    else
    {
		/* If *fgadget is non-NULL, then someone thinks
		 * that gadget has the focus.  We'll check that
		 * assumption against the "current focus widget."  If
		 * it passes and its parent is the same as this widget's
		 * parent, call _OlSetCurrentFocusWidget directly.
		 *
		 * This is not only an optimization, but it
		 * must be done since the server won't generate a focus
		 * In event on the gadget's parent since its already
		 * got focus.  Without a FocusIn event,
		 * _OlSetCurrentFocusWidget won't be called and the
		 * "current focus widget" won't get updated.  So
		 * prevent this, we'll short circuit the logic here.
		 * Q.E.D.
		 */

	if (*fgadget != (Widget)NULL	&&
	    *cfw == *fgadget		&&
	    (XtParent(w) == XtParent(*fgadget)))
	{
		if (*fgadget != w)
		{
			_OlSetCurrentFocusWidget(*fgadget,OL_OUT);
			*fgadget = w;
			_OlSetCurrentFocusWidget(w, OL_IN);
		}

			/* No need to go further since focus is already
			 * in this application.
			 */

		ReleaseToken();
		return;
	}

		/* If we're here, it's because we're moving focus between
		 * gadgets on two different parents, or *fgadget is
		 * NULL, or the current focus widget did not match
		 * *fgadget.
		 */

	*fgadget = w;
    }


    if (is_gadget == True)
	w = XtParent(w);
	
		/* We have to #ifdef out the check for the focus
		 * model because
		 * we don't explicitly set focus to an object, then
		 * we can't use keyboard traversal since the pointer
		 * certainly cannot be over the object we're traversing
		 * to (unless we warp the pointer -- and we know
		 * we can't do that).
		 *
		 * Furthermore, the workspace manager doesn't use the
		 * same resource name to update the focus model.  It
		 * uses (release 4.0) XtNpointerFocus and we're looking
		 * for XtNfocusModel.  YUCK!!
		 */
#if 0
    if ((focus_data->focus_model == OL_CLICK_TO_TYPE)) {
	    XSetInputFocus(XtDisplay(w), XtWindow(w), revert_to, time);
    }
#else
    XSetInputFocus(XtDisplay(w), XtWindow(w), revert_to, time);
#endif /* if 0 */

    ReleaseToken();

} /* END OF OlSetInputFocus() */

/***************************procedure*header*****************************
 *
 * OlUpdateTraversalWidget - insert "w" into the traversal list w.r.t.
 *	ref_name or ref_widget. The 4th parameter "insert_update"
 *	indicates whether it is in the insert (True) or update (False)
 *	mode. The initial step will be treated differently from this
 *	flag. the following is the logic and reason of the initial step:
 *
 *		find "w" from the traversal list;
 *		if (insert mode and found)
 *			return immediately;
 *		if (update mode)
 *			if (found)
 *				delete it from list before the update
 *			else
 *				can't do this update because this widget
 *				is not traversable. ---(A)
 *
 *	an example for (A) would be: an applic. tries to do
 *		XtSetValues(XtNreferenceName or XtNreferenceWidget) on
 *		a controlArea. If this check was not done, this manager
 *		widget will be in the traversal list.
 *
 *	an assumption here is that if set insert_update to True,
 *		you know you want this widget be in the list but
 *		not when you set insert_update to False.
 *
 *	this routine replaces _OlInserDescendant().
 *
 * ref_name IS NOT IMPLEMENTED YET.
 *
 ***************************procedure*header*****************************/
void
_OlUpdateTraversalWidget (Widget w, String ref_name,
			  Widget ref_widget, Boolean insert_update)
{
    OlFocusData *	focus_data = _OlGetFocusData(w, NULL);
    _OlArray		list;
    int			reference_pos,
			pos;

				/* Check for descendant of VendorShell. */
				/* If mouseless is disabled, we don't want */
			        /* to add widgets to the traversal list */
                                /* although we do allow text widgets on */
    if (focus_data == NULL || (!_OlMouseless(w) &&
			       !XtIsSubclass(w, textEditWidgetClass) &&
			       !XtIsSubclass(w, textLineWidgetClass) &&
			       !XtIsSubclass(w, textFieldWidgetClass)))
	return;			/* return silently */

    list = &(focus_data->traversal_list);

    pos = _OlArrayFind(list, w);
    if (insert_update == True)			/* insert mode */
    {
	if (pos != _OL_NULL_ARRAY_INDEX)	/* in the list, return now */
		return;
    }
    else					/* update mode */
    {
	if (pos != _OL_NULL_ARRAY_INDEX)	/* in the list, delete it */
		_OlDeleteDescendant(w);
	else					/* can't do this if not in  */
						/* list, see comments above */	
		return;
    }

    if (ref_name != NULL)	/* this has higher preference */
    {
	reference_pos = _OlArrayFindByName(list, ref_name);
	if (reference_pos == _OL_NULL_ARRAY_INDEX) /* delay this until */
		focus_data->resort_list = True;
	else
		_OlFreeRefName(w);
    }
    else			/* find ref_widget in traversal list */
    {
    	if (ref_widget == NULL)
    	{
		reference_pos = _OL_NULL_ARRAY_INDEX;
    	}
    	else
    	{
		reference_pos = _OlArrayFind(list, ref_widget);
		if (reference_pos == _OL_NULL_ARRAY_INDEX)
			OlWarning(dgettext(OlMsgsDomain,
	    			"Traversal: reference widget not found in \
traversal list"));
    	}
    }

    _OlArrayInsert(list, reference_pos, w);	/* insert widget into list */

		/* This should be done last, because w must be in
		   the traversal list before calling OlRegisterFocus. */

    _OlRegisterFocusWidget(w, False);	/* register widget with Shell */

} /* END OF _OlUpdateTraversalWidget() */

/***************************procedure*header*****************************
 *
 * _OlRemapDestinationWidget: assumed the evevt type already checked
 *		for KeyPress, KeyRelease, FocusIn, and FocusOut
 ***************************procedure*header*****************************/
extern Widget
_OlRemapDestinationWidget (Widget w, XEvent *xevent)
{
	Widget		gadget_parent;
	OlFocusData *	fd = _OlGetFocusData(w, NULL);

	if (fd == NULL || FOCUS_GADGET(fd) == NULL)
	{
		return (w);
	}

	gadget_parent	= XtParent(FOCUS_GADGET(fd));

	/* adjust 'w' for gadgets.  FOCUS_GADGET(fd) has been set to the
	   gadget to receive focus (see OlSetInputFocus).  The FocusChange
	   will occur on the parent, though.  If FOCUS_GADGET(fd) belongs
	   to 'w', set 'w' to it.  Adjustment is not needed for FocusOut
	   since current_focus_widget is used (see _OlSetCurrentFocusWidget)
	   but 'w' must be adjusted anyway for callbacks to be handled
	   properly.
	*/
			/* For a Keypress, if the object receiving the
			 * keypress is a descendant of the gadget's parent,
			 * then the pointer is probably in the 'w' widget,
			 * resulting in the keypress being sent to the
			 * gadget-parent's descendant instead of to the
			 * gadget's parent directly.  In this case, the
			 * destination object is set to the gadget.
			 */
	if (xevent->type == KeyPress || xevent->type == KeyRelease)
	{
		Widget	shell = _OlGetShellOfWidget(w);
		Widget	self = w;

		while (self != shell &&
		       self != gadget_parent)
		{
			self = XtParent(self);
		}

		if (gadget_parent == self)
		{
			w = FOCUS_GADGET(fd);
		}
	}
	else		/* else part must be FocusIn or FocusOut */
	{
		if (gadget_parent == w)
		{
		    w  = FOCUS_GADGET(fd);
		}
	}
	
	return (w);
} /* end of _OlRemapDestinationWidget */
