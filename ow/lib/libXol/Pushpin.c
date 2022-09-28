#pragma ident	"@(#)Pushpin.c	302.9	97/03/26 lib/libXol SMI" /* menu:src/Pushpin.c	1.39*/

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
 *************************************************************************
 *
 * Description:
 * 		OPEN LOOK(TM) Pushpin widget source code.
 *
 ******************************file*header********************************
 */

#include <locale.h>
#include <libintl.h>

#include <X11/IntrinsicP.h>
#include <X11/StringDefs.h>
#include <X11/CoreP.h>
#include <X11/Shell.h>
#include <X11/ShellP.h>

#include <Xol/OpenLookP.h>
#include <Xol/PushpinP.h>
#include <Xol/OblongButP.h>
#include <Xol/ButtonP.h>
#include <Xol/OlI18nP.h>
#include <Xol/Menu.h>

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

static void	GetNewDimensions(PushpinWidget ppw, Boolean update_width, Boolean update_height);	/* Calc. new window size	*/
static void	GetNormalGC(PushpinWidget ppw);		/* Get pushpin's GC		*/
static void	previewCallback(Widget w, XtPointer client_data, XtPointer call_data);	/* size or draw preview pin	*/
static void	PreviewManager(PushpinWidget ppw, Boolean destroy_mode);	/* Caches XImages		*/
static void	PreviewPushpin(PushpinWidget ppw);	/* Previews Pushpin in another
					 * widget's window		*/

					/* class procedures		*/

static Boolean	ActivateWidget (Widget, OlVirtualName, XtPointer);
static void	ClassInitialize(void);	/* initializes widget class	*/
static void	Destroy(Widget w);		/* Destroys the pushpin's data	*/
static void	Initialize(Widget request, Widget new, ArgList args, Cardinal *num_args);		/* initializes pushpin instance	*/
static void	Redisplay(Widget w, XEvent *xevent, Region region);		/* handles redisplay of window	*/
static Boolean	SetValues(Widget current, Widget request, Widget new, ArgList args, Cardinal *num_args);		/* manages value changes	*/

					/* action procedures		*/

static void	Notify(Widget w, OlVirtualName button);		/* fires callback lists		*/
static void	SelectPin(Widget w, OlVirtualName button);		/* sets pushpin state		*/
static void	SetDefault (Widget);
static void	UnselectPin(Widget w);		/* Unsets the pushpin state	*/

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

#define WORKAROUND	1		/* do not remove this line	*/

typedef struct _PreviewCache {
	struct _PreviewCache *	next;
	Screen *		screen;
	int			reference_count;
	Widget			shell;
	Widget			oblong;
} PreviewCache;

static PreviewCache *	cache_list = (PreviewCache *) NULL;

#define OFFSET(field) XtOffset(PushpinWidget, field)

/*
 *************************************************************************
 *
 * Define Translations and Actions
 *
 * Since the pushpin can be on a menu, the translations must work for
 * the MENU button as well as the SELECT button.
 *
 ***********************widget*translations*actions***********************
 */

static void	HandleButton(Widget w, OlVirtualEvent ve);
static void	HandleCrossing(Widget w, OlVirtualEvent ve);
static void	HandleMotion(Widget w, OlVirtualEvent ve);
	
static char
translations[] = "\
	<FocusIn>:	OlAction() \n\
	<FocusOut>:	OlAction() \n\
	<Key>:		OlAction() \n\
	<BtnDown>:	OlAction() \n\
	<BtnUp>:	OlAction() \n\
\
	<Enter>:	OlAction() \n\
	<Leave>:	OlAction() \n\
	<BtnMotion>:	OlAction()";

static OlEventHandlerRec event_procs[] =
{
	{ ButtonPress,		HandleButton	},
	{ ButtonRelease,	HandleButton	},
	{ EnterNotify,		HandleCrossing	},
	{ LeaveNotify,		HandleCrossing	},
	{ MotionNotify,		HandleMotion	},
};

/*
 *************************************************************************
 *
 * Define Resource list associated with the Widget Instance
 *
 ****************************widget*resources*****************************
 */

static XtResource
resources[] = { 

   { XtNpushpinIn, XtCCallback, XtRCallback, sizeof(XtCallbackList), 
      OFFSET(pushpin.in_callback), XtRCallback, (XtPointer) NULL },

   { XtNpushpinOut, XtCCallback, XtRCallback, sizeof(XtCallbackList), 
      OFFSET(pushpin.out_callback), XtRCallback, (XtPointer) NULL },

   { XtNpushpin, XtCPushpin, XtROlDefine, sizeof(OlDefine), 
      OFFSET(pushpin.pin_state), XtRImmediate, (XtPointer) OL_OUT },

   { XtNdefault, XtCDefault, XtRBoolean, sizeof(Boolean), 
      OFFSET(pushpin.is_default), XtRImmediate, (XtPointer) False },

   { XtNpreview, XtCPreview, XtRWidget, sizeof(Widget),
	OFFSET(pushpin.preview_widget), XtRWidget, (XtPointer) NULL },

   { XtNshellBehavior, XtCShellBehavior, XtRInt, sizeof(int), 
      OFFSET(pushpin.shell_behavior), XtRImmediate, (XtPointer) BaseWindow },

};

/*
 *************************************************************************
 *
 * Define Class Record structure to be initialized at Compile time
 *
 ***************************widget*class*record***************************
 */

PushpinClassRec
pushpinClassRec = {
  {
    (WidgetClass) &primitiveClassRec,	/* superclass		  */	
    "Pushpin",				/* class_name		  */
    sizeof(PushpinRec),			/* size			  */
    ClassInitialize,			/* class_initialize	  */
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
    Destroy,				/* destroy		  */
    NULL,				/* resize		  */
    Redisplay,				/* expose		  */
    SetValues,				/* set_values		  */
    NULL,				/* set_values_hook	  */
    XtInheritSetValuesAlmost,		/* set_values_almost	  */
    NULL,				/* get_values_hook	  */
    NULL,				/* accept_focus		  */
    XtVersion,				/* version		  */
    NULL,				/* callback_private	  */
    translations,			/* tm_table		  */
    XtInheritQueryGeometry,		/* query_geometry	  */
  },	/* CoreClass fields initialization */
  {					/* primitive_class fields */
    NULL,				/* reserved		*/
    NULL,				/* highlight_handler	*/
    NULL,				/* traversal_handler	*/
    NULL,				/* register_focus	*/
    ActivateWidget,			/* activate		*/
    event_procs,			/* event_procs		*/
    XtNumber(event_procs),		/* num_event_procs	*/
    OlVersion,				/* version		*/
    NULL				/* extension		*/
  },
  {
    NULL				/* field not used    	  */
  }  /* PushpinClass field initialization */
};

/*
 *************************************************************************
 *
 * Public Widget Class Definition of the Widget Class Record
 *
 *************************public*class*definition*************************
 */

WidgetClass pushpinWidgetClass = (WidgetClass) &pushpinClassRec;

/*
 *************************************************************************
 *
 * Private Procedures
 *
 ***************************private*procedures****************************
 */

/*
 *************************************************************************
 * GetNewDimensions - this routine calculates the new pushpin window
 * dimensions based on its "scale"
 ****************************procedure*header*****************************
 */
static void
GetNewDimensions(PushpinWidget ppw, Boolean update_width, Boolean update_height)
{
	Widget	parent = XtParent(ppw);
	Boolean	in_menu = False;

	if (update_width == True)
	    ppw->core.width = 
		(Dimension)(PushPinOut_Width(ppw->pushpin.pAttrs->ginfo));

	if (update_height == True) {
	    while (parent) {
		if (XtIsSubclass(parent, menuShellWidgetClass)) {
		    in_menu = True;
		    break;
		} else if ( XtIsVendorShell(parent) )
		    break;
		parent = XtParent(parent);
	    }
	    ppw->core.height = in_menu ?
		/* OLGX_TODO: need some way to ask OLGX the height of the
		 * default pushpin if pushpin belongs to a menu, eg. a
		 * DefaultPushPinOut_Height macro...for now, pick a number
		 * out of the air that seems large enough and add it in.
		 * BUT, note that this is not independent of point size.
		 */
		(Dimension)(PushPinOut_Height(ppw->pushpin.pAttrs->ginfo) + 3) :
		(Dimension)(PushPinOut_Height(ppw->pushpin.pAttrs->ginfo));
	}

} /* END OF GetNewDimensions() */

/*
 *************************************************************************
 * GetNormalGC - acquires the Graphics Context for normal pushpin state
 ****************************procedure*header*****************************
 */
static void
GetNormalGC(PushpinWidget ppw)
{
						/* Destroy existing GCs	*/

	if (ppw->pushpin.pAttrs != (OlgxAttrs *) NULL)
		OlgxDestroyAttrs ((Widget)ppw, ppw->pushpin.pAttrs);

	ppw->pushpin.pAttrs = OlgxCreateAttrs ((Widget)ppw,
					      ppw->primitive.foreground,
					      (OlgxBG *)&(ppw->core.background_pixel),
					      False,
					      ppw->primitive.scale,
					      (OlStrRep)0, (OlFont)NULL);
} /* END OF GetNormalGC() */


/*
 *************************************************************************
 * previewCallback - Draw the pushpin in the preview button or determine
 * the size of a preview pin label.
 *************************************************************************
 */
/* ARGSUSED */
static void
previewCallback (Widget w, XtPointer client_data, XtPointer call_data)
{
    ButtonProcLbl	*lbl = (ButtonProcLbl *) call_data;
    int			state;

    switch (lbl->callbackType) {
    case OL_SIZE_PROC:
	lbl->width = (Dimension)(PushPinIn_Width(lbl->ginfo));
	lbl->height = (Dimension)(PushPinOut_Height(lbl->ginfo));
	break;

    case OL_DRAW_PROC:
	state = OLGX_PUSHPIN_IN;
	if (!OlgIs3d(w->core.screen))
	    /* clears some of button background too, but this is better
	     * than not being able to see the pushpin at all
	     */
	    state |= OLGX_ERASE;
	olgx_draw_pushpin(lbl->ginfo, lbl->win, 
	    lbl->x + (int)(lbl->width - PushPinIn_Width(lbl->ginfo))/2,
	    lbl->y + (int)(lbl->height - PushPinOut_Height(lbl->ginfo))/2,
	    state);
	break;
    }
}

/*
 *************************************************************************
 * PreviewManager - this routine creates the shell and the oblong widget
 * that will be used preview the pushpin.  These are cached on a per
 * screen basis
 ****************************procedure*header*****************************
 */
static void
PreviewManager(PushpinWidget ppw, Boolean destroy_mode)
	             	    
	       		             	/* Add or remove mode		*/
{
	PreviewCache *		self = (PreviewCache *) NULL;
	PreviewCache *		prev = (PreviewCache *) NULL;
	Screen *		screen = XtScreen((Widget) ppw);
	extern PreviewCache *	cache_list;

			/* Traverse the existing list to find a match	*/

	for (self = cache_list; self; self = self->next) {
		if (self->screen == screen)
			break;
		else
			prev = self;
	}

	if (destroy_mode == True) {		/* Delete mode		*/

		if (self == (PreviewCache *) NULL) {
			OlWarning(dgettext(OlMsgsDomain,
				"Pushpin trying to de-reference\
 non-existent XImages"));
			return;
		}

		if (--self->reference_count == 0) {

				/* Destroy the previewing widgets.
				 * Destroying the shell will result in
				 * the death of the oblong child	*/

			XtDestroyWidget(self->shell);

			if (prev)
				prev->next = self->next;
			else
				cache_list = self->next;

			XtFree((char *) self);
		}

		return;
	}

						/* Create a new node	*/

	if (self == (PreviewCache *) NULL) {
		Arg	args[5];
		int	a;

						/* Create a new cache	*/

		self			= XtNew(PreviewCache);
		self->reference_count	= 0;
		self->screen		= screen;
		self->next		= cache_list;
		cache_list		= self;

		ppw->pushpin.preview_cache = (XtPointer) self;

				/* Create Widgets needed for previewing	*/
		a = 0;
		XtSetArg(args[a], XtNmappedWhenManaged, False);		++a;
		XtSetArg(args[a], XtNfooterPresent, False);		++a;
		XtSetArg(args[a], XtNimFooterPresent, False);		++a;

		self->shell = XtAppCreateShell("pushpinHiddenShell",
					       "PushpinHiddenShell",
					       topLevelShellWidgetClass,
					       DisplayOfScreen(screen),
					       args, a);

		a = 0;
		XtSetArg(args[a], XtNlabelJustify, OL_CENTER);		++a;
		XtSetArg(args[a], XtNrecomputeSize, False);		++a;
		XtSetArg(args[a], XtNmappedWhenManaged, False);		++a;
		XtSetArg(args[a], XtNlabelType, OL_PROC);		++a;
		XtSetArg(args[a], XtNbuttonType, OL_OBLONG);		++a;
		self->oblong = XtCreateManagedWidget("pushpinHiddenOblong",
				buttonWidgetClass, self->shell,
				args, a);

				/* Add callback to draw pushpin */
		XtAddCallback (self->oblong, XtNlabelProc, previewCallback,
			       (XtPointer)0);
	}

	++(self->reference_count);
	ppw->pushpin.preview_cache = (XtPointer) self;

} /* END OF PreviewManager() */

/*
 *************************************************************************
 * PreviewPushpin - this routine is called when the Pushpin is being
 * previewed by another widget.
 ****************************procedure*header*****************************
 */
static void
PreviewPushpin(PushpinWidget ppw)
{
	PreviewCache *		preview_cache;
	register Widget		destw = ppw->pushpin.preview_widget;
	Arg			args[10];
	int			a = 0;

	if (!XtIsRealized(destw))
		return;

/*  WORKAROUND - this error check will not work when previewing
	in a gadget.

	if (XtScreen(destw) != XtScreen((Widget) ppw)) {
		OlWarning(dgettext(OlMsgsDomain,
			"Pushpin: Cannot Preview between different\
 Screens !!"));
		return;
	}
*/

	preview_cache = (PreviewCache *) ppw->pushpin.preview_cache;

				/* Set the attributes of the OblongButton */
	a = 0;
	XtSetArg(args[a], XtNforeground, ppw->primitive.foreground);
	++a;
	XtSetArg(args[a], XtNbackground, ppw->core.background_pixel);
	++a;
	XtSetArg(args[a], XtNscale, ppw->primitive.scale);
	++a;

		/* See if the hidden oblong is realized.  If it is not,
		 * realize it now.					*/

	if (XtIsRealized(preview_cache->shell) == False) {
		XtRealizeWidget(preview_cache->shell);
	}

	XtSetValues(preview_cache->oblong, args, a);

					/* Now Call the Button's
					 * Preview Routine		*/

	_OlButtonPreview((ButtonWidget) destw,
		(ButtonWidget) preview_cache->oblong);

} /* END OF PreviewPushpin() */

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
 * this widget. Currently, this routine is called by Menu.c with OL_SELECT
 ****************************procedure*header*****************************
 */
/* ARGSUSED */
static Boolean
ActivateWidget (Widget w, OlVirtualName type, XtPointer call_data)
{
	Boolean		consumed = False;
	PushpinPart	*pushpin = &(((PushpinWidget) w)->pushpin);

	if (type != OL_SELECTKEY)
		return (consumed);

	consumed = True;
	if (pushpin->pin_state == (OlDefine)OL_IN) {
		pushpin->pin_state = (OlDefine)OL_OUT;
		XtCallCallbacks(w, XtNpushpinOut, (XtPointer) NULL);
	} else {
		pushpin->pin_state = (OlDefine)OL_IN;
		XtCallCallbacks(w, XtNpushpinIn, (XtPointer) NULL);
	}
	return (consumed);
} /* END OF ActivateWidget() */

/*
 *************************************************************************
 * ClassInitialize - Register OlDefine string values.
 ****************************procedure*header*****************************
 */
static void
ClassInitialize(void)
{
	_OlAddOlDefineType ("out", OL_OUT);
	_OlAddOlDefineType ("in",  OL_IN);
	
} /* END OF ClassInitialize() */

/*
 *************************************************************************
 * Destroy - free the GCs stored in the pushpin widget.
 ****************************procedure*header*****************************
 */
static void
Destroy(Widget w)
{
	PushpinWidget ppw = (PushpinWidget) w;

					/* Destroy the Pushpin's GC	*/

	OlgxDestroyAttrs ((Widget)ppw, ppw->pushpin.pAttrs);

			/* remove this widget from the Preview list */

	PreviewManager(ppw, True);

						/* Remove All callbacks	*/

	XtRemoveAllCallbacks(w, XtNpushpinIn);
	XtRemoveAllCallbacks(w, XtNpushpinOut);

} /* END OF Destroy() */

/*
 *************************************************************************
 * Initialize - Initializes the instance record fields and resolves any
 * conflicts between the fields.  This routine registers a set of XImages
 * with the pushpin.
 ****************************procedure*header*****************************
 */
/* ARGSUSED */
static void
Initialize(Widget request, Widget new, ArgList args, Cardinal *num_args)
{
	PushpinPart *	pushpin = &(((PushpinWidget) new)->pushpin);

						/* Create the normal GC	*/

	pushpin->pAttrs = (OlgxAttrs *) NULL;
	GetNormalGC((PushpinWidget) new);

					/* Create the Preview cache.	*/

	pushpin->preview_cache = (XtPointer) NULL;
	PreviewManager((PushpinWidget) new, False);

			/* Initialize Widget data.  Set the window
			 * size to be big enough to fit the largest
			 * XImage for its point size.			*/

	GetNewDimensions((PushpinWidget) new,
			 new->core.width == (Dimension) 0,
			 new->core.height == (Dimension) 0);

	if (pushpin->pin_state != (OlDefine)OL_IN)
		pushpin->pin_state = (OlDefine)OL_OUT;

	pushpin->selected		= (Boolean)False;
	pushpin->preview_widget		= (Widget) NULL;

} /* END OF Initialize() */

/*
 *************************************************************************
 * Redisplay - this procedure redisplays the pushpin widget after
 * exposure events or when the state changes.
 ****************************procedure*header*****************************
 */
/* ARGSUSED */
static void
Redisplay(Widget w, XEvent *xevent, Region region)
	          		/* Pushpin widget id			*/
	               		/* unused */
	               		/* unused */
{
	PushpinWidget		ppw = (PushpinWidget)w;
	int			state = 0;
	register PushpinPart	*pushpin = &(ppw->pushpin);

	if (XtIsRealized(w) == False)
		return;

	if (pushpin->pin_state == (OlDefine)OL_IN) {
	    if (pushpin->selected == (Boolean)True) 
		state |= OLGX_PUSHPIN_OUT | OLGX_ERASE;
	    else
		state |= OLGX_PUSHPIN_IN;
	} else {
	    if (pushpin->selected == (Boolean)True) 
		state |= OLGX_PUSHPIN_IN | OLGX_ERASE;
	    else
		state |= OLGX_PUSHPIN_OUT;
	}
	if (pushpin->is_default == (Boolean)True)
	    state |= OLGX_DEFAULT;

	olgx_draw_pushpin(pushpin->pAttrs->ginfo, XtWindow(w), 0, 0, state);

} /* END OF Redisplay() */

/*
 *************************************************************************
 * SetValues - this routine manages changes to the pushpin fields caused
 * by an XtSetValues() call.  It returns a boolean specifying whether or
 * not a change requires a redisplay.
 ****************************procedure*header*****************************
 */
/* ARGSUSED */
static Boolean
SetValues(Widget current, Widget request, Widget new, ArgList args, Cardinal *num_args)
	               		/* Current Pushpin widget		*/
	               		/* What the application wants		*/
	           		/* What the application gets, so far...	*/
	       		     
	          	         
{
	PushpinWidget nppw = (PushpinWidget)new;
        PushpinWidget cppw = (PushpinWidget)current;
	PushpinPart *cpp = &(((PushpinWidget) current)->pushpin);
	PushpinPart *npp = &(((PushpinWidget) new)->pushpin);
	Boolean      redisplay = False;

	if (nppw->primitive.scale != cppw->primitive.scale) {
		GetNormalGC ((PushpinWidget) new);

			/* Always update the dimensions of the pushpin	*/

		GetNewDimensions((PushpinWidget) new, True, True);
		redisplay = True;
	}

	if (((PushpinWidget)new)->primitive.foreground !=
	    ((PushpinWidget)current)->primitive.foreground ||
	    new->core.background_pixel != current->core.background_pixel) {

		GetNormalGC((PushpinWidget) new);
		redisplay = True;
	}

	if (npp->is_default != cpp->is_default) {
		redisplay = True;
		_OlSetDefault(new, npp->is_default);
	}

	if (npp->preview_widget != (Widget) NULL) {
		PreviewPushpin((PushpinWidget) new);
		npp->preview_widget = (Widget) NULL;
	}

	if (npp->pin_state != cpp->pin_state) {
		if (npp->pin_state != OL_IN)	/* check validity	*/
			npp->pin_state = OL_OUT;
		if (npp->pin_state != cpp->pin_state)
			redisplay = True;
	}

	return (redisplay);

} /* END OF SetValues() */

/*
 *************************************************************************
 *
 * Action Procedures
 *
 ****************************action*procedures****************************
 */

/*
 *************************************************************************
 * Notify - fires the callbacks once the user decides final state of the
 * pushpin
 ****************************procedure*header*****************************
 */
/* ARGSUSED */
static void
Notify(Widget w, OlVirtualName button)
	          	  		/* Pushpin widget id		*/
	             	       		/* either OL_SELECT or OL_MENU	*/
{
	PushpinPart *pushpin = &(((PushpinWidget) w)->pushpin);

	if (button == OL_MENU &&
	    pushpin->shell_behavior != StayUpMenu	&&
	    pushpin->shell_behavior != PressDragReleaseMenu &&
	    pushpin->selected != True)
	{
		return;
	}

	pushpin->selected = False;

	ActivateWidget(w, OL_SELECTKEY, NULL);
} /* END OF Notify() */

/*
 *************************************************************************
 * SelectPin - this procedure is called when the user presses a valid
 * button on the pushpin or when a valid button is dragged over the 
 * the pushpin.  Valid buttons are the SELECT or MENU button.  But, the
 * MENU button is valid only if the pushpin is on a menu.
 ****************************procedure*header*****************************
 */
/* ARGSUSED */
static void
SelectPin(Widget w, OlVirtualName button)
	      		  		/* Pushpin widget id		*/
	             	       		/* either OL_SELECT or OL_MENU	*/
{
	PushpinPart *pushpin = &(((PushpinWidget) w)->pushpin);

				/* Give up the pointer grab		*/

	_OlUngrabPointer(w);

				/* Ignore selection of the pushpin by
				 * Menu button if the shell behavior
				 * is not "PressDragRelease" mode	*/
	
	if (button == OL_MENU &&
	    pushpin->shell_behavior != StayUpMenu	&&
	    pushpin->shell_behavior != PressDragReleaseMenu)
		return;

	if (pushpin->selected == True)
		return;

	pushpin->selected = True;
	if (XtIsRealized(w) == (Boolean)True) {
		XClearArea(XtDisplay(w), XtWindow(w), 0, 0,
				(unsigned int)0, (unsigned int)0, (Bool)False);
		Redisplay(w, (XEvent *)NULL, (Region) NULL);
		XFlush(XtDisplay(w));
	}
} /* END OF SelectPin() */

/*
 *************************************************************************
 * SetDefault - called when user uses the mouse to set the pushpin
 * to be a default in the menu.
 ****************************procedure*header*****************************
 */
static void
SetDefault (Widget w)
{
	PushpinWidget ppw = (PushpinWidget) w;

	if ((ppw->pushpin.shell_behavior != PressDragReleaseMenu &&
	     ppw->pushpin.shell_behavior != StayUpMenu) ||
	    ppw->pushpin.is_default == True)
		return;

	ppw->pushpin.is_default = True;
	if (XtIsRealized(w) == (Boolean)True) {
		XClearArea(XtDisplay(w), XtWindow(w), 0, 0,
			(unsigned int)0, (unsigned int)0, (Bool)False);
		Redisplay(w, (XEvent *)NULL, (Region) NULL);
		XFlush(XtDisplay(w));
	}

	_OlSetDefault(w, True);
} /* END OF SetDefault() */

/*
 *************************************************************************
 * UnselectPin - this procedure is called when the user leaves the
 * pushpin's window.  If the pushpin has been selected, its unselected.
 * No callbacks are made.
 ****************************procedure*header*****************************
 */
/* ARGSUSED */
static void
UnselectPin(Widget w)
	            		/* Pushpin widget id			*/
{
	PushpinPart *pushpin = &(((PushpinWidget) w)->pushpin);

				/* Ignore selection of the pushpin by
				 * Menu button if the shell behavior
				 * is not "PressDragRelease" mode	*/
	
	if (pushpin->selected == (Boolean)False)
		return;

#ifdef oldcode /* this never happened */
	if ((*num_params != (Cardinal)0)	&&
	    pushpin->shell_behavior != StayUpMenu	&&
	    pushpin->shell_behavior != PressDragReleaseMenu)
		return;
#endif

	pushpin->selected = False;
	if (XtIsRealized(w) == (Boolean)True) {
		XClearArea(XtDisplay(w), XtWindow(w), 0, 0,
			(unsigned int)0, (unsigned int)0, (Bool)False);
		Redisplay(w, (XEvent *)NULL, (Region) NULL);
		XFlush(XtDisplay(w));
	}
} /* END OF UnselectPin() */

static void
HandleButton(Widget w, OlVirtualEvent ve)
{
	if (ve->xevent->type == ButtonPress)
	{
		_OlUngrabPointer(w);
	}

	switch (ve->virtual_name)
	{
		case OL_SELECT:	/* FALLTHROUGH */
		case OL_MENU:
			ve->consumed = True;
			if (ve->xevent->type == ButtonPress)
				SelectPin(w, ve->virtual_name);
			else
				Notify(w, ve->virtual_name);
			break;
		case OL_MENUDEFAULT:
			if (ve->xevent->type == ButtonPress)
			{
				ve->consumed = True;
				SetDefault(w);
			}
			break;
		default:
			if (ve->xevent->type == ButtonRelease)
			{
				ve->consumed = True;
				UnselectPin(w);
			}
			break;
	}
} /* end of HandleButton() */

static void
HandleCrossing(Widget w, OlVirtualEvent ve)
{
	switch (ve->virtual_name)
	{
		case OL_SELECT:	/* FALLTHROUGH */
		case OL_MENU:
			ve->consumed = True;
			if (ve->xevent->type == EnterNotify)
				SelectPin(w, ve->virtual_name);
			else
				UnselectPin(w);
			break;
		case OL_MENUDEFAULT:
			ve->consumed = True;
			if (ve->xevent->type == EnterNotify)
				SetDefault(w);
			else
				UnselectPin(w);
			break;
		default:
			if (ve->xevent->type == LeaveNotify)
			{
				ve->consumed = True;
				UnselectPin(w);
			}
			break;
	}
} /* end of HandleCrossing() */

static void
HandleMotion(Widget w, OlVirtualEvent ve)
{
	switch (ve->virtual_name)
	{
		case OL_SELECT:	/* FALLTHROUGH */
		case OL_MENU:
			ve->consumed = True;
			SelectPin(w, ve->virtual_name);
			break;
		case OL_MENUDEFAULT:
			ve->consumed = True;
			SetDefault(w);
			break;
	}
} /* end of HandleMotion() */

/*
 *************************************************************************
 *
 * Public Procedures
 *
 ****************************public*procedures****************************
 */

/* There are no public procedures */
