#pragma ident	"@(#)Action.c	302.10	97/03/26 lib/libXol SMI"	/* mouseless:Action.c 1.30	*/

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
 *	This file contains the generic translations, generic action
 *	routines and convenience routines to manage the event handling
 *	within the toolkit.
 *
 ******************************file*header********************************
 */

#include <libintl.h>
#include <stdio.h>

#include <X11/IntrinsicP.h>
#include <X11/Xatom.h>

#include <Xol/Error.h>
#include <Xol/EventObjP.h>	/* For gadgets	*/
#include <Xol/Flat.h>
#include <Xol/ManagerP.h>
#include <Xol/OlI18nP.h>
#include <Xol/OpenLookP.h>
#include <Xol/PrimitiveP.h>
#include <Xol/TextEdit.h>
#include <Xol/TextField.h>
#include <Xol/TextLine.h>
#include <Xol/VendorI.h>


/*
 *************************************************************************
 *
 * Forward Procedure definitions listed by category:
 *		1. Private Procedures
 *		2. Action  Procedures 
 *		3. Public  Procedures 
 *
 **************************forward*declarations***************************
 */

					/* private procedures		*/

static OlEventHandlerProc
		GetEventHandler (WidgetClass, WidgetClass, int);
static Boolean	assoc_check_cycle (Widget, Widget);

					/* action procedures		*/

static void	HandleFocusChange (Widget, OlVirtualEvent);
static void	HandleKeyPress (Widget, OlVirtualEvent);


/*
 *************************************************************************
 *
 * Define global/static variables and #defines, and
 * Declare externally referenced variables
 *
 *****************************file*variables******************************
 */

			/* HACK to get the flattened widgets to know
			 * which sub-object is to receive focus when a
			 * mnemonic is pressed.
			 */
XtPointer	Ol_mnemonic_data = (XtPointer)NULL;

#define VENDOR		0
#define PRIMITIVE	1
#define MANAGER		2
#define	GADGET		3
#define	OTHER		4

#define ASSOC_LIST_STEP	64

#define	NULL_WIDGET	((Widget)NULL)
#define NULL_DATA	((XtPointer)NULL)
#define PCLASS(wc,field) (((PrimitiveWidgetClass)wc)->primitive_class.field)
#define MCLASS(wc,field) (((ManagerWidgetClass)wc)->manager_class.field)
#define GCLASS(wc,field) (((EventObjClass)wc)->event_class.field)

	/* Define Generic translation table and Generic Action Table	*/

const char
_OlGenericTranslationTable[] = "\
	<FocusIn>:	OlAction()	\n\
	<FocusOut>:	OlAction()	\n\
	<Key>:		OlAction()	\n\
	<BtnDown>:	OlAction()	\n\
	<BtnUp>:	OlAction()	\n\
";

	/* Define Generic Event Handler List	*/

OlEventHandlerRec
_OlGenericEventHandlerList[] = {
	{ FocusIn,	HandleFocusChange	},
	{ FocusOut,	HandleFocusChange	},
	{ KeyPress,	HandleKeyPress		},
	/* don't handle button presses or releases even though
	 * we've selected for them.
	 */
};
const Cardinal _OlGenericEventHandlerListSize =
			XtNumber(_OlGenericEventHandlerList);

static Cardinal		AssocWidgetListSize = 0;
static Cardinal		AssocWidgetListAllocSize = 0;
static WidgetList	LeaderList = NULL;
static WidgetList	FollowerList = NULL;
static Boolean		assoc_list_modified = FALSE;

static OlEventHandlerList	pre_consume_list = NULL;
static OlEventHandlerList	post_consume_list = NULL;
static int			pre_consume_list_size = 0;
static int			post_consume_list_size = 0;

/*
 *************************************************************************
 *
 * Private Procedures
 *
 ***************************private*procedures****************************
 */

/*
 *************************************************************************
 * GetEventHandler - this routine returns the event handler procedure
 * registered for a particular event type.
 ****************************procedure*header*****************************
 */
static OlEventHandlerProc
GetEventHandler(WidgetClass wc, WidgetClass wc_special, int xevent_type)
{
	OlEventHandlerList	elist;
	Cardinal		num;

	if (wc_special == primitiveWidgetClass) {
		elist	= PCLASS(wc, event_procs);
		num	= PCLASS(wc, num_event_procs);
	} else if (wc_special == managerWidgetClass) {
		elist	= MCLASS(wc, event_procs);
		num	= MCLASS(wc, num_event_procs);
	} else if (wc_special == eventObjClass) {
		elist	= GCLASS(wc, event_procs);
		num	= GCLASS(wc, num_event_procs);
	} else {
		OlVendorClassExtension	ext = _OlGetVendorClassExtension(wc);

		if (ext) {
			elist	= ext->event_procs;
			num	= ext->num_event_procs;
		} else {
			elist	= (OlEventHandlerList)NULL;
			num	= 0;
		}
	}

	while(num != 0 && elist->type != xevent_type) {
		++elist;
		--num;
	}
	return( ((num != 0) ? elist->handler : NULL) );
} /* END OF GetEventHandler() */

/*
 *************************************************************************
 *
 * Action Procedures
 *
 ****************************action*procedures****************************
 */

/*
 *************************************************************************
 * HandleFocusChange - this routine is called whenever an object receives
 * a focus change event.
 ****************************procedure*header*****************************
 */
static void
HandleFocusChange(Widget w, OlVirtualEvent ve)
{

    /* return immediately if the mode is NotifyGrab or NotifyUngrab */
    if (ve->xevent->xfocus.mode == NotifyGrab ||
	ve->xevent->xfocus.mode == NotifyUngrab)
	return;

    switch(ve->xevent->xfocus.detail)
    {
    /* case NotifyPointer:		*/
    /* case NotifyPointerRoot:		*/
    /* case NotifyVirtual:		*/
    /* case NotifyNonlinearVirtual:	*/
    /* 	return;				*/
    /* 	break;				*/
						/* we like these types */
    case NotifyInferior:
    case NotifyDetailNone:
    case NotifyAncestor:
    case NotifyNonlinear:

	if (_OlMouseless(w) ||
	    XtIsSubclass(w, textEditWidgetClass) ||
	    XtIsSubclass(w, textFieldWidgetClass) ||
	    XtIsSubclass(w, textLineWidgetClass))
	    _OlSetCurrentFocusWidget(w, (ve->xevent->type == FocusIn) ?
				     OL_IN : OL_OUT);
	break;
    }
} /* END OF HandleFocusChange() */

/*
 *************************************************************************
 * HandleKeyPress - this routine is called whenever an object receives
 * a keypress
 ****************************procedure*header*****************************
 */
static void
HandleKeyPress(Widget w, OlVirtualEvent ve)
{
	Widget		d;	/* destination widget	*/
	XtPointer	data;

				/* Always consume the event	*/

	ve->consumed = TRUE;

	if ((d = _OlFetchMnemonicOwner(w, &data, ve))) {

		/*
		 * A mnemonic moves the focus to the widget and
		 * activates it: We (attempt to) set the focus here,
		 * but don't bother waiting to see if it was successful.
		 * We don't bother to check to see if the widget already
		 * has focus, since some widget (e.g., the flats) might
		 * have to move focus when a mnemonic is pressed.
		 */

		/* If the object being activated via a mnemonic is a
		 * flattened widget, set global flag so that the
		 * flattened widget's accept focus procedure knows which
		 * sub-object is to receive focus.
		 */
		if (XtIsSubclass(d, flatWidgetClass) == True)
		{
			Ol_mnemonic_data	= data;
			if (OlCallAcceptFocus(d, ve->xevent->xkey.time) == True || !_OlMouseless(w))
			{
				(void) OlActivateWidget(d, OL_SELECTKEY, data);
			}
			Ol_mnemonic_data = (XtPointer)NULL;
		}
		else
		{
			if (OlGetCurrentFocusWidget(d) == d ||
			    OlCallAcceptFocus(d, ve->xevent->xkey.time) == True || !_OlMouseless(w))
			{
				(void) OlActivateWidget(d, OL_SELECTKEY, data);
			}
		}
	} else if ((d = _OlFetchAcceleratorOwner(w, &data, ve))) {
		/*
		 * An accelerator just activates the widget without
		 * moving focus.
		 */
		(void) OlActivateWidget(d, OL_SELECTKEY, data);

	} else {
		switch(ve->virtual_name) {
		case OL_IMMEDIATE:
		case OL_PREVFIELD:
		case OL_NEXTFIELD:
		case OL_MOVERIGHT:
		case OL_MOVELEFT:
		case OL_MOVEUP:
		case OL_MOVEDOWN:
		case OL_MULTIRIGHT:
		case OL_MULTILEFT:
		case OL_MULTIUP:
		case OL_MULTIDOWN:
			(void) OlMoveFocus(w, ve->virtual_name,
					   ve->xevent->xkey.time);
			break;
		case OL_TOGGLEPUSHPIN:
		case OL_CANCEL:
		case OL_DEFAULTACTION:
			if ((d = _OlGetShellOfWidget(w)) != NULL_WIDGET)
			{
				(void) OlActivateWidget(d, ve->virtual_name,
							NULL_DATA);
			}
			break;
		case OL_HELP:
			_OlProcessHelpKey(w, ve->xevent);
			break;
		case OL_COPY:
		case OL_CUT:
			/* trap these two keys here, this allows an app does */
			/* copy/cut when the input focus is on other widget  */
			/* e.g, sweep a piece of text from a staticText, and */
			/*	then traverse to a text/textedit widget.     */
			/*	press OL_COPY and OL_PASTE. The highlighted  */
			/*	text from the staticText will be saved in the*/
			/*	clipboard after OL_COPY. The saved text will */
			/*	be showed on the text/textEdit after OL_PASTE*/

			/* Currently, we only handled the intra-process part */
			/* In the future, we should also allow this feature  */
			/* to the inter-process part.                        */

			/* note: the only way to get the selection owner is  */
			/*              thru a Xlib call                     */
			{
				Window win;

				if ((win = XGetSelectionOwner(XtDisplayOfObject(w),
						XA_PRIMARY)) != None)
				{
					d = XtWindowToWidget(XtDisplayOfObject(w), win);
					(void) OlActivateWidget(d,
							ve->virtual_name,
							NULL_DATA);
					break;
				}
			}
			/* FALLTHROUGH  `default' */
		default:
			if (_OlMouseless(w) &&
			    ve->virtual_name != OL_UNKNOWN_KEY_INPUT)
			{
				(void) OlActivateWidget(w, ve->virtual_name,
							NULL_DATA);
			}
			break;
		}
	}
} /* END OF HandleKeyPress() */

/*
 *************************************************************************
 *
 * Public Procedures
 *
 ****************************public*procedures****************************
 */

/*
 *************************************************************************
 * OlAction - this is the generic action procedure used by all OPEN LOOK
 * widgets in their translation tables.
 ****************************procedure*header*****************************
 */
/* ARGSUSED */
void
OlAction(Widget w, XEvent *xevent, String *params, Cardinal *num_params)
{
	extern Widget 		_OlRemapDestinationWidget ( Widget,
								    XEvent *);
	WidgetClass		wc_special;
	WidgetClass		wc;
	OlEventHandlerProc	handler;
	OlVirtualEventRec	ve;
	OlEventHandlerList	elist;
	int			i;

	GetToken();
	switch (xevent->type)
	{
		case KeyPress:		/* FALLTHROUGH */
		case KeyRelease:	/* FALLTHROUGH */
		case FocusIn:		/* FALLTHROUGH */
		case FocusOut:
				/* switch to gadget id if necessary */
			w = _OlRemapDestinationWidget(w, xevent);
			break;
		default:
			break;
	}


	wc_special = _OlClass(w);
	if (wc_special != primitiveWidgetClass &&
	    wc_special != eventObjClass &&
	    wc_special != vendorShellWidgetClass &&
	    wc_special != managerWidgetClass)
	{
		ReleaseToken();
		return;
	}

	OlLookupInputEvent(w, xevent, &ve, OL_DEFAULT_IE);

	/*** CHECK APPLICATION EVENT PROCS ***/
	/*
	 * NOTE: If a spring loaded shell is mapped, the application event
	 *       handler will be caused more than once. The application
	 *	 can recognize the extra copies by comparing the serial
	 *	 numbers of consecutive events.
	 */
	for (i = pre_consume_list_size, elist = pre_consume_list;
		 (i > 0) && (elist->type != xevent->type); i--, elist++) ;
	if (i)
		(*(elist->handler))(w, &ve);

	if (OlHasCallbacks(w, XtNconsumeEvent) == XtCallbackHasSome)
	{
		OlCallCallbacks(w, XtNconsumeEvent, (XtPointer)&ve);
	}

	wc = XtClass(w);

	while (ve.consumed == FALSE) {

		handler = GetEventHandler(wc, wc_special, xevent->type);

		if (handler) {
			(*handler)(w, &ve);
		}

		if (wc == primitiveWidgetClass || wc == eventObjClass ||
		    wc == vendorShellWidgetClass || wc == managerWidgetClass)
		{
			if (ve.consumed == FALSE) {
				/* check application event procs */
				for (i=post_consume_list_size,
				     elist=post_consume_list;
		 		     (i > 0) && (elist->type != xevent->type);
				 	i--, elist++) ;
				if (i)
					(*(elist->handler))(w, &ve);
			}

			ve.consumed = TRUE;
		} else {
			wc = wc->core_class.superclass;
		}
	}
	ReleaseToken();
} /* END OF OlAction() */

/*
 *************************************************************************
 * OlActivateWidget - this calls a widget's class procedure to activate
 * it.  If the widget to be activated is not busy and sensitive and its
 * ancenstors are sensitive, TRUE is returned; else FALSE is returned.
 ****************************procedure*header*****************************
 */
Boolean
OlActivateWidget (Widget w, OlVirtualName type, XtPointer data)
{
	static int		level=0;
	WidgetClass		wc;
	WidgetClass		wc_special;
	OlActivateFunc		activate;
	OlVendorClassExtension	ext;
	Boolean			ret_val;


	if (w == NULL_WIDGET) {
		return(False);
	} else if (XtIsSensitive(w) == FALSE) {
		return(False);
	}

	GetToken();
	++level;		/* record the number of recursions	*/

				/* If we get this far, it's ok to activate
				 * the widget.				*/

	wc = XtClass(w);
	wc_special = _OlClass(w);

	if (wc_special == vendorShellWidgetClass) {
		ext = _OlGetVendorClassExtension(wc);
		activate = (OlActivateFunc) (ext ? ext->activate : NULL);
	} else if (wc_special == primitiveWidgetClass) {
		activate = ((PrimitiveWidgetClass)
					wc)->primitive_class.activate;
	} else if (wc_special == eventObjClass) {
		activate = ((EventObjClass) wc)->event_class.activate;
	} else if (wc_special == managerWidgetClass) {
		activate = ((ManagerWidgetClass) wc)->manager_class.activate;
	} else {
		activate = (OlActivateFunc)NULL;
	}

	ret_val = ((activate != NULL) && (XtIsShell(w) || XtIsManaged(w))) ?
			(*activate)(w, type, data) : False;

	/* try the asociated widget list */
	if (ret_val == FALSE) {
		Widget *l;
		Widget *f;
		Widget *e;

		/* from now on, want to know if list has been modified */
		if (level == 1)
			assoc_list_modified = FALSE;

		for (l=LeaderList, e=LeaderList+AssocWidgetListSize; l < e; l++)
			if (*l == w) {
				f = FollowerList + (l - LeaderList);
				do {
					ret_val= OlActivateWidget(*f,type,data);
					if ((ret_val == TRUE) ||
					    (assoc_list_modified == TRUE))
					{
						--level;
						ReleaseToken();
						return(TRUE);
					}
					f++;
					l++;
				}
				while ((l < e) && (*l == w));
			}
	}

	--level;
	ReleaseToken();
	return(ret_val);
} /* END OF OlActivateWidget() */

/*
 *************************************************************************
 * OlAssociateWidget - 
 * NOTE: The FollowerList and the LeaderList are two parallel arrays. They
 *	 are kept in separate arrays, because in the future we may want to
 *	 have a function returing a list of followers given the leader.
 ****************************procedure*header*****************************
 */
Boolean
OlAssociateWidget (Widget leader, Widget follower, Boolean disable_traversal)
{
	register Widget *l;
	register Widget *f;
	register Widget *e;

	if ((leader == NULL) || (follower == NULL)) {
		OlWarning(dgettext(OlMsgsDomain,
			"OlAssociateWidget: NULL leader or follower"));
		return(FALSE);
	}

	GetToken();
	if (AssocWidgetListSize == AssocWidgetListAllocSize) {
		AssocWidgetListAllocSize += ASSOC_LIST_STEP;
		l = (WidgetList)XtRealloc((char *)LeaderList,
				AssocWidgetListAllocSize * sizeof(Widget));
		f = (WidgetList)XtRealloc((char *)FollowerList,
				AssocWidgetListAllocSize * sizeof(Widget));
		if ((l == NULL) || (f == NULL)) {
			AssocWidgetListAllocSize -= ASSOC_LIST_STEP;
			OlWarning(dgettext(OlMsgsDomain,
				"OlAssociateWidget: Not enough memory"));
			ReleaseToken();
			return(FALSE);
		}

		LeaderList = l;
		FollowerList = f;
	}

	/*
	 * The follower widget cannot already be a follower widget in the
	 * table.
	 */
	for (f=FollowerList, e=FollowerList+AssocWidgetListSize; f < e; f++)
		if (*f == follower) {
			OlWarning(dgettext(OlMsgsDomain,
				"OlAssociateWidget: bad follower widget"));
			ReleaseToken();
			return(FALSE);
		}

	/*
	 * See if the leader widget is already in the list. If so, add the new
	 * entry right next to the existing entries.
	 */
	for (l=LeaderList, e=LeaderList+AssocWidgetListSize; l < e; l++)
		if (*l == leader) {
			/* find the last entry with the same leader */
			do l++;
			while ((l < e) && (*l == leader));

			if (l < e) {
				/* shift down the remaining entries */
				memmove((XtPointer)(l + 1), (XtPointer)l,
					(int)(e - l) * sizeof(Widget));

				/* also the follower list */
				f = FollowerList + (l - LeaderList);
				memmove((XtPointer)(f + 1), (XtPointer)f,
					(int)(e - l) * sizeof(Widget));
			}
			break;
		}

	/* add an entry */
	f = FollowerList + (l - LeaderList);
	*l = leader;
	*f = follower;
	AssocWidgetListSize++;

	/* check for cycle */
	if (assoc_check_cycle(*l, *f) == TRUE) {
		OlWarning(dgettext(OlMsgsDomain,
			"OlAssociateWidget: cycle not allowed"));
		OlUnassociateWidget(*f);
		ReleaseToken();
		return(FALSE);
	}

	assoc_list_modified = TRUE;

	if (disable_traversal == TRUE) {
		static Arg arg[] = {
			{ XtNtraversalOn, FALSE },
		};

		XtSetValues(follower, arg, XtNumber(arg));
	}

	/* done */
	ReleaseToken();
	return(TRUE);
}

/*
 *************************************************************************
 * OlUnassociateWidget - 
 ****************************procedure*header*****************************
 */
void
OlUnassociateWidget(Widget follower)
{
	register Widget *l;
	register Widget *f;
	register Widget *e;
	
	GetToken();
	for (f=FollowerList, e=FollowerList+AssocWidgetListSize; f < e; f++)
		if (*f == follower) {

			assoc_list_modified = TRUE;

			/* shift up the remaining entries */
			(void)memcpy((XtPointer)f, (XtPointer)(f+1),
				 (int)(e - f) * sizeof(Widget));

			/* also the leader list */
			l = LeaderList + (f - FollowerList);
			(void)memcpy((XtPointer)l, (XtPointer)(l+1),
				 (int)(e - f) * sizeof(Widget));

			AssocWidgetListSize--;
			ReleaseToken();
			return;
		}
	ReleaseToken();
}

/* ARGSUSED */
Boolean
OlSetAppEventProc (Widget w, OlDefine listtype, OlEventHandlerList list, Cardinal count)
{
	OlEventHandlerList *ptr;
	int *listsize;

	GetToken();
	switch(listtype) {
	case OL_PRE:
		ptr = &pre_consume_list;
		listsize = &pre_consume_list_size;
		break;
	case OL_POST:
		ptr = &post_consume_list;
		listsize = &post_consume_list_size;
		break;
	default:
		ReleaseToken();
		return(FALSE);
	}

	if (*ptr)
		XtFree((char*) *ptr);

	if ((*ptr = (OlEventHandlerList)XtMalloc(sizeof(OlEventHandlerRec) *
			count)) == NULL) {
		*ptr = NULL;
		*listsize = 0;
		ReleaseToken();
		return(FALSE);
	}

	(void) memcpy(*ptr, list, sizeof(OlEventHandlerRec) * count);
	*listsize = count;
	ReleaseToken();
	return(TRUE);
}

/*
 *************************************************************************
 * assoc_check_cycle -  This function checks for cycles after an entry is
 *			is added to the table. It basically traverses all the
 *			connected nodes in the graph starting at the follower
 *			node and if it finds the leader node, then a cycle
 *			is found.
 ****************************procedure*header*****************************
 */
static Boolean
assoc_check_cycle(Widget target, Widget w)
{
	register Widget *l;
	register Widget *f;
	register Widget *e;

	for (l=LeaderList, e=LeaderList+AssocWidgetListSize; l < e; l++)
		if (*l == w) {
			f = FollowerList + (l - LeaderList);
			do {
				if (*f == target)
					return(TRUE);
				if (assoc_check_cycle(target, *f) == TRUE)
					return(TRUE);
				l++;
				f++;
			} while ((l < e) && (*l == w));
			return(FALSE);
		}
	return(FALSE);
}
