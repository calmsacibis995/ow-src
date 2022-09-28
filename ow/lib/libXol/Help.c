#pragma ident "@(#)Help.c	302.22    97/03/26 lib/libXol SMI"     /* help:src/Help.c 1.46 */
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
 *************************************************************************
 *
 * Description:
 *		This file contains the source code for the Help Widget
 *
 ******************************file*header********************************
 */


#include <ctype.h>
#include <libintl.h>
#include <stdio.h>
#include <unistd.h>

#include <X11/IntrinsicP.h>
#include <X11/ShellP.h>
#include <X11/StringDefs.h>
#include <X11/Xatom.h>

#include <Xol/EventObj.h>
#include <Xol/Flat.h>
#include <Xol/HelpP.h>
#include <Xol/Mag.h>
#include <Xol/OlI18nP.h>
#include <Xol/OpenLookP.h>
#include <Xol/RootShell.h>
#include <Xol/ScrolledWi.h>
#include <Xol/TextEdit.h>
#include <Xol/buffutil.h>
#include <Xol/textbuff.h>

typedef struct _HelpSubItem {
	XtPointer		id;		/* Gadget id or item_index*/
	OlStr			tag;
	OlDefine      		source_type;
	XtPointer 		source;
	struct _HelpSubItem *	next;
} HelpSubItem, *HelpSubItemPtr;

typedef struct _HelpItem {
	OlDefine     		id_type;
	XtPointer		id;
	OlStr			tag;
	OlDefine      		source_type;
	XtPointer 		source;
	HelpSubItem *		sub_items;
	struct _HelpItem *	next;
	OlStrRep		text_format;	/* To pass it to TextEdit */
} HelpItem, *HelpItemPtr;

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

						/* private procedures	*/

static Widget	CreateHelpTree(Widget w, OlStr tag, Window window, OlStrRep text_format);	/* Creates Help tree if needed	*/
static void	GetSubItemHelp(Widget w, HelpItemPtr p, HelpSubItemPtr *s, int x, int y);	/* gets help on a sub-item	*/
static void	HandleText(Widget w, int source_type, char *source);		/* Handles help message		*/
static void	LookupItemEntry(OlDefine id_type, XtPointer id, HelpItemPtr *p_return, HelpItemPtr **q_return);	/* looks up a hash table entry	*/
static void	LookupSubItemEntry(HelpItemPtr p, XtPointer id, HelpSubItemPtr *s_return, HelpSubItemPtr **n_return);	/* looks up a sub-object entry	*/
static void	RegisterSubItem(Widget widget, XtPointer id, OlStr tag, OlDefine source_type, XtPointer source, OlDefine id_type);	/* sub-objects and gadgets	*/

						/* class procedures	*/

static void	InitializeHook(Widget w, ArgList args, Cardinal *num_args);	/* Creates sub components	*/
static void	Destroy(Widget w);		/* Destroy this widget		*/
static void	WMMsgHandler(Widget w, XtPointer client_data, XtPointer call_data);		/* wm protocol handler		*/
static void	DestroyShell(Widget w, XtPointer client_data, XtPointer call_data);		/* popdown-help-when-shell-destroy */

						/* action procedures	*/

static void     _OlUnregisterHelp(Widget w, XtPointer client_data, XtPointer call_data);    /* unregisters help		*/


/*
 *************************************************************************
 *
 * Define global/static variables and #defines, and
 * Declare externally referenced variables
 *
 *****************************file*variables******************************
 */

#define HALF_DIM(id, d) (int)(id->core.d/(Dimension)2)

#define NOT_FOUND  "There is no help available for this object."

#define N_ATOM_HASH_ENT  127

#define HASH(Q) ((unsigned long)Q % N_ATOM_HASH_ENT)

static HelpItemPtr	HashTbl[N_ATOM_HASH_ENT];
static HelpWidget	help_widget	= (HelpWidget) NULL;
static Widget		previous_shell	= (Widget) NULL;

#define HorizontalPoints(W,P) \
	OlScreenPointToPixel(OL_HORIZONTAL,(P),XtScreenOfObject(W))
#define VerticalPoints(W,P) \
	OlScreenPointToPixel(OL_VERTICAL,(P),XtScreenOfObject(W))

/*
 *************************************************************************
 *
 * Define Resource list associated with the Widget Instance
 *
 ****************************widget*resources*****************************
 */

static XtResource
resources[] =
{
#define offset(FIELD) XtOffsetOf(HelpRec, FIELD)
	/* textFormat should be the first resource */
    {
	XtNtextFormat, XtCTextFormat, 
	XtROlStrRep,sizeof(OlStrRep), offset(help.text_format),
	XtRImmediate,(XtPointer)OL_MB_STR_REP
     },
    {
	XtNallowRootHelp, XtCAllowRootHelp, XtRBoolean, sizeof(Boolean),
	offset(help.allow_root_help), XtRImmediate, (XtPointer) False
    },
    {
	XtNorientation, XtCOrientation,
	XtROlDefine, sizeof(OlDefine), offset(rubber_tile.orientation),
	XtRString, (XtPointer)"horizontal"
    }

#undef	offset
};

/*
 *************************************************************************
 *
 * Define Class Record structure to be initialized at Compile time
 *
 ***************************widget*class*record***************************
 */

HelpClassRec
helpClassRec = {
  {
	(WidgetClass) &rubberTileClassRec,	/* superclass		*/
	"Help",					/* class_name		*/
	sizeof(HelpRec),			/* widget_size		*/
	NULL,					/* class_initialize	*/
	NULL,					/* class_part_initialize*/
	FALSE,					/* class_inited		*/
	NULL,					/* initialize		*/
	InitializeHook,				/* initialize_hook	*/
	XtInheritRealize,			/* realize		*/
	NULL,					/* actions		*/
	NULL,					/* num_actions		*/
	resources,				/* resources		*/
	XtNumber(resources),			/* num_resources	*/
	NULLQUARK,				/* xrm_class		*/
	TRUE,					/* compress_motion	*/
	TRUE,					/* compress_exposure	*/
	TRUE,					/* compress_enterleave	*/
	FALSE,					/* visible_interest	*/
	Destroy,				/* destroy		*/
	XtInheritResize,			/* resize		*/
	NULL,					/* expose		*/
	NULL,					/* set_values		*/
	NULL,					/* set_values_hook	*/
	XtInheritSetValuesAlmost,		/* set_values_almost	*/
	NULL,					/* get_values_hook	*/
	NULL,					/* accept_focus		*/
	XtVersion,				/* version		*/
	NULL,					/* callback_private	*/
	NULL,					/* tm_table		*/
	XtInheritQueryGeometry,			/* query_geometry	*/
	NULL,					/* display_accelerator	*/
	NULL					/* extension		*/
  },	/* End of CoreClass field initializations */
  {
	XtInheritGeometryManager,    		/* geometry_manager	*/
	XtInheritChangeManaged,			/* change_managed	*/
	XtInheritInsertChild,    		/* insert_child		*/
	XtInheritDeleteChild,    		/* delete_child		*/
	NULL    				/* extension         	*/
  },	/* End of CompositeClass field initializations */
  {
    	NULL,					/* resources		*/
    	0,					/* num_resources	*/
    	sizeof(HelpConstraintRec),		/* constraint_size	*/
    	NULL,					/* initialize		*/
    	NULL,					/* destroy		*/
    	NULL					/* set_values		*/
  },	/* End of ConstraintClass field initializations */
  {
    	NULL,					/* highlight_handler	*/
    	NULL,					/* reserved		*/
    	NULL,					/* reserved		*/
        NULL,					/* traversal_handler	*/
	NULL,					/* activate_widget	*/
	NULL,					/* event_procs		*/
	0,					/* num_event_procs	*/
	NULL,					/* register_focus	*/
    	NULL,					/* reserved		*/
	OlVersion,				/* version		*/
	NULL,					/* extension		*/
	{NULL,0},				/* dyn_data */
	NULL,					/* transparent proc 	*/
	NULL,					/* query_sc_locn_proc 	*/
  },	/* End of ManagerClass field initializations */
  {
	NULL					/* field not used	*/
  },	/* End of RubberTileClass field initializations */
  {
	NULL,					/* field not used	*/
  },	/* End of HelpClass field initializations */
};

/*
 *************************************************************************
 *
 * Public Widget Class Definition of the Widget Class Record
 *
 *************************public*class*definition*************************
 */

WidgetClass
helpWidgetClass = (WidgetClass)&helpClassRec;

/*
 *************************************************************************
 *
 * Private Procedures
 *
 ***************************private*procedures****************************
 */

/*
 *************************************************************************
 * CreateHelpTree - this routine creates the help widget tree if one does
 * not already exist.  The function returns the id of the shell 
 * containing the help widget.
 ****************************procedure*header*****************************
 */
static Widget
CreateHelpTree(Widget w, OlStr tag, Window window, OlStrRep text_format)
{
	Display *	display	= XtDisplayOfObject(w);
	Widget		shell;
	char * 		title = NULL;
	char *  	name;
	char *		class;
	char *		format = "%s:%s ";
	String  	help_string = dgettext(OlMsgsDomain,"Help");
	shell = (Widget) (help_widget != (HelpWidget)NULL ?
			_OlGetShellOfWidget((Widget)help_widget) : NULL);

	XtGetApplicationNameAndClass(display, &name, &class);

	title = (char *)XtMalloc((name ? strlen(name) : 0) +
				 strlen(format) +
				 (tag ? strlen(tag) : 0) +
				 strlen(help_string) + 1);
	sprintf(title, format,
		name != (char *)NULL ? name : "",
		tag != (char *)NULL ? tag : "");
	strcat(title, help_string);

	if (shell != (Widget)NULL)
	{
		XtVaSetValues(
			shell,
			XtNtitle, title,
			XtNwindowGroup,		(XtArgVal)window,
			(String)0
		);
	}
	else
	{
		shell = XtVaAppCreateShell(
			"helpShell",
			class,
			transientShellWidgetClass,
			display,
			XtNtextFormat,	text_format,
			XtNtitle, title,
			XtNwindowGroup,		(XtArgVal)window,
			XtNpushpin,		(XtArgVal)OL_IN,
			XtNwinType,		(XtArgVal)OL_WT_HELP,
			XtNwindowHeader,	(XtArgVal)True,
			XtNmenuButton,		(XtArgVal)False,
			XtNfooterPresent,	(XtArgVal)True,
			XtNimFooterPresent,	(XtArgVal)False,
			(String)0
		);
		OlAddCallback(shell, XtNwmProtocol, WMMsgHandler, NULL);
		XtVaCreateManagedWidget(
			"help",
			helpWidgetClass,
			shell,
			XtNtextFormat,		(XtArgVal)text_format,
			(String)0
		);
	}
	if(title)
		XtFree(title);
	return(shell);
} /* END OF CreateHelpTree() */

/*
 *************************************************************************
 * GetSubItemHelp - this routine gets help for a sub-item based on 
 * x and y coordinates.  Note: this routine does not check the value
 * of "p->sub_items" since the calling routine did so.
 ****************************procedure*header*****************************
 */
static void
GetSubItemHelp(Widget w, HelpItemPtr p, HelpSubItemPtr *s, int x, int y)
	      		  		/* The containing widget	*/
	           	  
	                  		/* returned subitem		*/
	   		  
	   		  
{
	XtPointer id;

	*s = (HelpSubItemPtr)NULL;

	if (XtIsSubclass(w, flatWidgetClass) == True)
	{
		id = (XtPointer)OlFlatGetItemIndex(w, (Position)x, (Position)y);

		if ((Cardinal)id != (Cardinal)OL_NO_ITEM)
		{
			HelpSubItemPtr * n;

			LookupSubItemEntry(p, id, s, &n);
		}
	}
	else
	{
		HelpSubItemPtr * n;

		id = (XtPointer) _OlWidgetToGadget(w, (Position)x,(Position)y);
		LookupSubItemEntry(p, id, s, &n);
	}
} /* END OF GetSubItemHelp() */

/*
 *************************************************************************
 * HandleText - This procedure populates the text widget
 ****************************procedure*header*****************************
 */
static void    
HandleText(Widget w, int source_type, char *source)
{

	XtVaSetValues (
		w,
	    	XtNsourceType,		(XtArgVal)source_type,
	    	XtNsource,		(XtArgVal)source,
	    	XtNeditType,		(XtArgVal)OL_TEXT_READ,
	    	XtNdisplayPosition,	(XtArgVal)0,
	    	XtNcursorPosition,	(XtArgVal)0,
	    	XtNselectStart,		(XtArgVal)0,
	    	XtNselectEnd,		(XtArgVal)0,
		(String)0
	);

} /* END OF HandleText() */

/*
 *************************************************************************
 * LookupItemEntry - this procedure looks up a hash table entry and returns
 * it.  It also returns the address of the previous 'next' pointer.
 ****************************procedure*header*****************************
 */
static void
LookupItemEntry(OlDefine id_type, XtPointer id, HelpItemPtr *p_return, HelpItemPtr **q_return)
{
	HelpItemPtr	p;
	HelpItemPtr *	q;

	q = &HashTbl[HASH(id)];
	for (p = *q; p != NULL; p = p-> next)
	{
		if (id == p-> id) 
		{
			if(id_type == p-> id_type)
				break;
			else if(id_type == OL_WIDGET_HELP){
						break;
			}
			else if(id_type == OL_FLAT_HELP){
						break;
			}
			else
				;
		}
		q = &p-> next;
	}

	*p_return = p;
	*q_return = q;
} /* END OF LookupItemEntry() */

/*
 *************************************************************************
 * LookupSubItemEntry - this procedure looks up a sub-object entry and
 * returns a pointer to it.  It also returns the address of the
 * previous 'next' pointer.
 ****************************procedure*header*****************************
 */
static void
LookupSubItemEntry(HelpItemPtr p, XtPointer id, HelpSubItemPtr *s_return, HelpSubItemPtr **n_return)
{
	HelpSubItemPtr		s;
	HelpSubItemPtr *	n;		/* next item pointer	*/

	for (n = &p->sub_items,s = *n; s != NULL; s = s->next)
	{
		if (id == s->id)
		{
			break;
		}
		n = &s->next;
	}

	*s_return = s;
	*n_return = n;
} /* END OF LookupSubItemEntry() */

/*
 *************************************************************************
 * RegisterSubItem - this routine registers sub-item help.  SubItems can
 * be sub-objects of a flat widget or they can be gadgets.
 ****************************procedure*header*****************************
 */
static void
RegisterSubItem(Widget widget, XtPointer id, OlStr tag, OlDefine source_type, XtPointer source, OlDefine id_type)
	      		       		/* The containing widget id	*/
	         	   
	      		    
	        	            
	         	       
	        	        
{
	HelpItemPtr		p;
	HelpItemPtr *		q;
	HelpSubItemPtr		s;
	HelpSubItemPtr *	n;
						/* Get the widget node	*/

	LookupItemEntry(OL_WIDGET_HELP, (XtPointer)widget, &p, &q);

		/* If there's no widget node to hang the sub-object
		 * information, create a node.				*/

	if (p == NULL)
	{
		/* store text_format */
		*q = p = XtNew(HelpItem);
		p->text_format = OL_MB_STR_REP;
		p->id_type	= (OlDefine)OL_WIDGET_HELP;
		p->id		= (XtPointer)widget;
		p->source_type	= (OlDefine)OL_IGNORE;
		p->source	= (XtPointer)NULL;
		p->tag		= (OlStr)NULL;
		p->next		= (HelpItemPtr)NULL;
		p->sub_items	= (HelpSubItemPtr)NULL;
		XtAddCallback(widget, XtNdestroyCallback,
					   _OlUnregisterHelp, NULL);
	}

					/* Look up the sub-object	*/

	LookupSubItemEntry(p, id, &s, &n);

	if (s == NULL)
	{
		s		= XtNew(HelpSubItem);
		s->id		= id;
		s->next		= (HelpSubItemPtr)NULL;
		*n		= s;
	}
	s->source_type	= source_type;
	s->source	= source;
	s->tag		= tag;
} /* END OF RegisterSubItem() */

/*
 *************************************************************************
 *
 * Class Procedures
 *
 ****************************class*procedures*****************************
 */

/*
 *************************************************************************
 * InitializeHook - This creates the magnifying glass widget and the text
 * widget used by the help widget.
 ****************************procedure*header*****************************
 */
/* ARGSUSED */

static void 
InitializeHook(Widget w, ArgList args, Cardinal *num_args)
	      		  		/* The new help widget		*/
	   		      		/* creation arg list		*/
	          	         	/* Number of creation args	*/
{
	HelpWidget	help = (HelpWidget) w;
	Widget		sw = (Widget)NULL; /* scrolledWindow for HelpSW */

	if (help_widget == (HelpWidget) NULL) {
		help_widget = help;
	}
	else {
		char * application = XrmQuarkToString(_OlApplicationName);
		char *msg;

		if (msg = malloc(512)) {
			snprintf(msg, 512, dgettext(OlMsgsDomain,
				"Application '%1$s' creating more than one help widget"),
				application);
			OlWarning(msg);
			free(msg);
		}
	}

	help->help.mag_widget = XtVaCreateManagedWidget(
		"magnifier",
		magWidgetClass,
		w,
		XtNborderWidth,		(XtArgVal)0,
		XtNweight,		(XtArgVal)0,
		(String)0
	);
	sw = XtVaCreateManagedWidget(
		"HelpSW",
		scrolledWindowWidgetClass,
		w,
		XtNborderWidth,		(XtArgVal)0,
		XtNweight,		(XtArgVal)1,
		(String)0
	);

	/*
	 * MORE: When the XtVaTypedArg feature is fixed, use it instead
	 * of doing the conversion ourselves?
	 */

	help->help.text_widget = XtVaCreateManagedWidget(
		"Text",
		textEditWidgetClass,
		sw,
		XtNtextFormat,		(XtArgVal)help->help.text_format,
		XtNeditType,		(XtArgVal)OL_TEXT_READ,
		XtNwrapMode,		(XtArgVal)OL_WRAP_WHITE_SPACE,
		XtNsource,		(XtArgVal)"",
		XtNwidth,		(XtArgVal)HorizontalPoints(w,390),
		XtNlinesVisible,	(XtArgVal)10,
		XtNleftMargin,		(XtArgVal)HorizontalPoints(w,20),
		XtNrightMargin,		(XtArgVal)HorizontalPoints(w,20),
		XtNtopMargin,		(XtArgVal)VerticalPoints(w,20),
		XtNbottomMargin,	(XtArgVal)VerticalPoints(w,20),
		(String)0
	);
} /* END OF InitializeHook() */

/*
 *************************************************************************
 * Destroy - this procedure destroys the help widget.  It also removes
 * event handlers from the Help's shell
 ****************************procedure*header*****************************
 */
/* ARGSUSED */
static void
Destroy(Widget w)
	         			/* The help Widget	*/
{
			/* Remove the global help pointer	*/

	help_widget = (HelpWidget)NULL;
	if (previous_shell) {
		XtRemoveCallback(previous_shell, XtNdestroyCallback,
				DestroyShell, NULL);
		previous_shell = NULL;
	}

} /* END OF Destroy() */

/* ARGSUSED */
static void
DestroyShell (Widget w, XtPointer client_data, XtPointer call_data)
{
	if (previous_shell)
		_OlPopdownHelpTree(previous_shell);
}

/* ARGSUSED */
static void
WMMsgHandler (Widget w, XtPointer client_data, XtPointer call_data)
{
	OlWMProtocolVerify *st = (OlWMProtocolVerify *)call_data;

	if (st->msgtype == OL_WM_DELETE_WINDOW) {
		XtPopdown(w);
		if (previous_shell) {
			XtRemoveCallback(previous_shell, XtNdestroyCallback,
				DestroyShell, NULL);
			previous_shell = NULL;
		}
	}
}

/*
 *************************************************************************
 *
 * Action Procedures
 *
 ****************************action*procedures****************************
 */

/*
 *************************************************************************
 * OlUnregisterHelp - This procedure deletes data base entries for dying
 * widgets.
 ****************************procedure*header*****************************
 */
/* ARGSUSED */
static void
_OlUnregisterHelp(Widget w, XtPointer client_data, XtPointer call_data)
	      		  			/* widget to unregister	*/
	         	            		/* The id_type    	*/
	         	          		/* NULL          	*/
{
	HelpItemPtr	p;
	HelpItemPtr *	q;
	Boolean		is_gadget = _OlIsGadget(w);
	char *		warning = XtNewString(dgettext(OlMsgsDomain,
				"Attempt to destroy non-existent help item"));

	LookupItemEntry(OL_WIDGET_HELP, (XtPointer)
			(is_gadget == True ? XtParent(w) : w), &p, &q);
	
	if (p == (HelpItemPtr)NULL)
	{
		OlWarning(warning);
	}
	else
	{
		HelpSubItemPtr		self;
		HelpSubItemPtr *	next;

		if (is_gadget == False)
		{
			self = p->sub_items;
			next = &self->next;

				/* First free the sub-object help	*/
	
			while (self != NULL)
			{
				next = &self->next;
				XtFree((XtPointer) self);
				self = *next;
			}
	
			*q = p->next;
			XtFree((XtPointer)p);
		}
		else
		{
			LookupSubItemEntry(p, (XtPointer)w, &self, &next);

			if (self != (HelpSubItemPtr) NULL)
			{
				*next = self->next;
				XtFree((XtPointer)self);
			}
			else
			{
				OlWarning(warning);
			}
		}
	}
} /* END OF _OlUnregisterHelp */

/*
 *************************************************************************
 *
 * Public Procedures
 *
 ****************************public*procedures****************************
 */

/*
 *************************************************************************
 * OlRegisterHelp - 
 ****************************procedure*header*****************************
 */
void
OlRegisterHelp(
    OlDefine	id_type,
    XtPointer	id,
    OlStr	tag,
    OlDefine	source_type,
    XtPointer	source)
{
    HelpItemPtr		p;
    HelpItemPtr *	q;
    OlFlatHelpId * 	complex_id;

    GetToken();
    switch ((int)id_type)
    {
    case OL_FLAT_HELP:
	complex_id = (OlFlatHelpId *)id;

	if (XtIsSubclass(complex_id->widget, flatWidgetClass) == False)
	{
		OlWarning(dgettext(OlMsgsDomain,
			"OlRegisterHelp: widget id must be a subclass of\
 flatWidgetClass to use id type OL_FLAT_HELP"));
		ReleaseToken();
		return;
	}
	else
	{
		RegisterSubItem(complex_id->widget, (XtPointer)
				complex_id->item_index, tag, source_type,
				source, (OlDefine)id_type);
	}
	break;
    case OL_CLASS_HELP:					/* Fall Through	*/
    case OL_WINDOW_HELP:				/* Fall Through	*/
    case OL_WIDGET_HELP:
	if ((id_type == (OlDefine)OL_WIDGET_HELP) &&
		    _OlIsGadget((Widget)id) == True)
	{
		RegisterSubItem(XtParent((Widget)id), id,
				tag, source_type, source, (OlDefine)id_type);
		break;
	}

							/* Else	.......	*/
	LookupItemEntry(id_type, id, &p, &q);

	if (p == NULL)
	{
		p		= XtNew(HelpItem);
		p->next		= NULL;
		p->sub_items	= NULL;
		*q		= p;

		if (id_type == (OlDefine)OL_WIDGET_HELP )
		{
			XtAddCallback((Widget) id, XtNdestroyCallback,
					   _OlUnregisterHelp, NULL);
		}
	}

	p->text_format = OL_MB_STR_REP;
	p->tag		= tag;
	p->id_type	= id_type;
	p->id		= id;
	p->source_type	= source_type;
	p->source	= source;

	break;
    default:
	OlWarning(dgettext(OlMsgsDomain,
		"Unknown id_type found while registering help"));
    }

ReleaseToken();
} /* END OF OlRegisterHelp() */

/*
 *************************************************************************
 * _OlPopupHelpTree - this routine is in charge of popping up the help
 * widget tree
 ****************************procedure*header*****************************
 */
/*ARGSUSED*/
void
_OlPopupHelpTree(Widget w, XtPointer client_data, XEvent *xevent, Boolean *continue_to_dispatch)
	      		  		/* Id of arbitrary widget	*/
	         	            	/* Unused			*/
{
	HelpItemPtr		p;
	HelpItemPtr *		q;
	HelpSubItemPtr		s = (HelpSubItemPtr)NULL;
	Widget			wid;
	Widget			shell = w;
	Window			win;
	Window 			window;
	int			win_x_return;
	int			win_y_return;
	int			root_x_return;
	int			root_y_return;
	Display *		dpy;
	OlDefine		source_type	= (OlDefine)OL_STRING_SOURCE;
	XtPointer		source;
	OlStr			tag		= (OlStr)NULL;
	void			(* func) ();
	Boolean			repeat;
	XrmValue		rmvalue;
	OlStrRep		text_format = OL_MB_STR_REP;
        OlDefine                id_type;
        XtPointer               id;
        OlFlatHelpId            flatid;
        Position                widget_x, widget_y;

	if (xevent->xany.type != ClientMessage ||
	    xevent->xclient.message_type !=
		OlInternAtom(xevent->xany.display, _OL_HELP_KEY_NAME))
	{
		return;
	}

	source	= (XtPointer)XtNewString(dgettext(
				    OlMsgsDomain, (const char *)NOT_FOUND));

	dpy	= XtDisplayOfObject(w);
	window	= RootWindowOfScreen(XtScreenOfObject(w));

	GetHelpKeyMessage(dpy, xevent, &win, 
		&win_x_return, &win_y_return, &root_x_return, &root_y_return);

			/* Does this window belong to a widget ?	*/

	if ((wid = XtWindowToWidget(dpy, win)) != NULL)
	{
		Widget	self;

		shell = _OlGetShellOfWidget(wid);

		window	= XtWindow(shell);

		for (self = wid; self != NULL; self = XtParent(self))
		{
			Arg args[2];

			LookupItemEntry(OL_WIDGET_HELP, (XtPointer)self,
						&p, &q);

			if (p != NULL)
			{
				if (p->sub_items != NULL)
				{
					GetSubItemHelp(wid, p, &s,
						win_x_return, win_y_return);

						/* If we've found subitem
						 * help or if this widget
						 * has its own help,
						 * break.		*/

					if (s != NULL ||
					    p->source_type !=
							(OlDefine)OL_IGNORE)
					{
						break;
					}
					else
					{
						p = (HelpItemPtr)NULL;
					}
				}
				else
				{
					break;
				}
			}
			else if (self == shell)
			{
				p = NULL;
				break;
			}
        /*      As we are going around the loop again, remember
                to add in the x and y coordinates of the widget
                we just looked at, as we are going to look at its parent next
        */
                        XtSetArg(args[0], XtNx, &widget_x);
                        XtSetArg(args[1], XtNy, &widget_y);
                        XtGetValues(self, args, 2);
                        win_x_return += widget_x;
                        win_y_return += widget_y;
		}

		if (p == NULL)
		{
			LookupItemEntry(OL_CLASS_HELP, (XtPointer) XtClass(wid),
					&p, &q);
		}
	}
	else
	{
				/* Look for help on the raw window	*/

		LookupItemEntry(OL_WINDOW_HELP, (XtPointer) win, &p, &q);
	}

	if (s != NULL)
	{
		source_type = s-> source_type;
		source      = s-> source;
		tag         = s-> tag;
	}
	else if (p != NULL)
	{
		source_type = p-> source_type;
		source      = p-> source;
		tag         = p-> tag;
		text_format = p-> text_format;
	}
	else{ 	/* If help window comes up from Root or Flat , text_format
			cannot be gotten from parent widget . This for-loop
			can get a 'p->text_format' from HashTable which
			registered by OlRegisterHelp() . The text_format will
			be passed to Help widget inside of CreateHelpTree() .
		*/
		int i = 0;
		for(i = 0; i < N_ATOM_HASH_ENT; i++){
			if(HashTbl[i] != 0){
				text_format = (HashTbl[i])->text_format;
			}
		}
	}

			/*
			 * Register destroy callback so that the help window
			 * goes away with the shell. Otherwise, dangling help
			 * windows could create problems for olwm when trying
			 * to terminate a session.
			 */
	if (previous_shell) {
		XtRemoveCallback(previous_shell, XtNdestroyCallback,
			DestroyShell, NULL);
	}
	previous_shell = shell;
	XtAddCallback(previous_shell, XtNdestroyCallback, DestroyShell, NULL);

			/* Create the Help widget if it does not exist */
	shell = CreateHelpTree(w, tag, window, text_format);
		/* Inform the Magnifier that it should take a snapshot	*/

	XtVaSetValues(
		help_widget->help.mag_widget,
		XtNmouseX,	(XtArgVal)root_x_return,
		XtNmouseY,	(XtArgVal)root_y_return,
		(String)0
	);
		
	do {
		char *path;
		char *mbstring;

		repeat = False;
		switch ((int)source_type) {
		case OL_DISK_SOURCE: 
			if(source != NULL){
				mbstring = XtMalloc( strlen((char *)source)+1);
				strcpy(mbstring,source);
			}
			else{
				mbstring = XtMalloc(1);
				mbstring = "";
			}
			if(mbstring[0] != '/')
				path = (XtPointer)XtResolvePathname( dpy, "help",
					mbstring, NULL, NULL , NULL, 0, NULL);
			else
				path = mbstring;
			if(path != NULL)
				source = path;
			else
				source = mbstring;
			if(access(source, 4) != 0) {
				source_type = (OlDefine)OL_STRING_SOURCE;
				source	= (XtPointer)XtNewString(dgettext(
					OlMsgsDomain, (const char *)NOT_FOUND));
			}
			HandleText( help_widget-> help.text_widget, 
						    source_type, source);
			XtFree((char *)mbstring);
			break;
		case OL_STRING_SOURCE:
				HandleText( help_widget-> help.text_widget, 
						    source_type, source);
			break;
		case OL_INDIRECT_SOURCE:
		case OL_TRANSPARENT_SOURCE:
			func = (void (*)()) source;
			if (func == NULL) {
				source_type = (OlDefine)OL_STRING_SOURCE;
				source	= (XtPointer)XtNewString(dgettext(
					OlMsgsDomain, (const char *)NOT_FOUND));
			} else {
                                /* If this was registered as OL_FLAT_HELP,
                                recreate the OlFlatHelpid to pass back to
                                the user routine */

                                if(s != NULL && XtIsSubclass( p->id, flatWidgetClass)) {
                                        id_type = OL_FLAT_HELP;
                                        flatid.widget = (Widget)(p->id);
                                        flatid.item_index = (Cardinal)s->id;
                                        id = (XtPointer)&flatid;
                                } else {
                                        id_type = p->id_type;
                                        id = p->id;
                                }
                                if (source_type ==
                                        (OlDefine)OL_TRANSPARENT_SOURCE)
                                {
                                        (*func) (id_type, id,
                                        win_x_return, win_y_return);
                                        return;
                                } else {
                                        (*func) (id_type, id,
                                        win_x_return, win_y_return,
                                        &source_type, &source);
                                        repeat = True;
                                }
			}
			break;
		default:
			OlWarning(dgettext(OlMsgsDomain,
				"Invalid help source found at popup of help"));
		}
	} while (repeat == True);

		/* Now realize the help widget's shell so we can set the
		 * the decoration hints on it.				*/

	if (((ShellWidget) shell)->shell.popped_up == False)
	{
		if (XtIsRealized(shell) == False)
		{
			XSizeHints	hints;
			Window		win;

			shell->core.managed = False;
			XtRealizeWidget(shell);
			shell->core.managed = True;

			/*
			 * MORE: Improve this hack. For now we limit
			 * the size of the window by guessing from the
			 * spec. We can do better, by e.g. querying the
			 * children for the smallest preferred size.
			 *
			 * MORE: Put this in a Realize procedure.
			 *
			 * Look at page 483 of the spec for the min width:
			 *	min_width	= -b + c + d + 60 + d
			 * Look at page 481-483 of the spec for the min
			 * height:
			 *	min_height	= a(483) + a(482)
			 *
			 * These values provide enough room for the
			 * magnifying glass (assuming the Mag widget is
			 * properly sized, which it currently isn't),
			 * plus a few characters of text within the
			 * prescribed margins.
			 */
			win = XtWindowOfObject(shell);
			if (!XGetNormalHints(dpy, win, &hints))
				hints.flags = 0;
			hints.flags      |= PMinSize;
			hints.min_width  = HorizontalPoints(shell,206);
			hints.min_height = VerticalPoints(shell,110);
			XSetNormalHints (dpy, win, &hints);
		}

		XtVaSetValues(
			shell,
			XtNpushpin,	(XtArgVal)OL_IN,
			(String)0
		);
		XtPopup(shell, XtGrabNone);
	}
	else
	{
		XtVaSetValues(
			shell,
			XtNtransientFor,	(XtArgVal)previous_shell,
			(String)0
		);
		XRaiseWindow(XtDisplay(shell), XtWindow(shell));
	}

} /* END OF _OlPopupHelpTree() */

/*
 * Pops down help tree, if it is mapped.
 */
/* ARGSUSED */
void		
_OlPopdownHelpTree (Widget w)
{
	Widget shell;

	if ((help_widget) &&
	    (shell = _OlGetShellOfWidget((Widget)help_widget)) &&
	    (((ShellWidget) shell)->shell.popped_up == True))
		XtPopdown(shell);

	if (previous_shell) {
		XtRemoveCallback(previous_shell, XtNdestroyCallback,
				DestroyShell, NULL);
		previous_shell = NULL;
	}
} /* END OF _OlPopdownHelpTree() */


/*
 *************************************************************************
 * _OlProcessHelpKey - takes a keypress and pops up the help window
 * using the information from the keypress.
 *
 * Note: this routine's implementation for Pointer-based help depends on
 * the window manager's implementation for Pointer-based help since
 * the OPEN LOOK ICCCM extensions don't specify the contents of the
 * help ClientMessage -- yuck!!!
 ****************************procedure*header*****************************
 */
void
_OlProcessHelpKey (Widget w, XEvent *xevent)
{
	Arg			args[1];
	OlDefine		help_model = (OlDefine)OL_POINTER;
	XClientMessageEvent	xcm;
	int			root_x;
	int			root_y;
	int			x;
	int			y;
	Window			help_window;
	OlStrRep		text_format = OL_MB_STR_REP;


	if (w == (Widget)NULL ||
	    xevent == (XEvent *)NULL ||
	    xevent->type != KeyPress)
	{
		return;
	}

	XtSetArg(args[0], XtNhelpModel, &help_model);
	OlGetApplicationValues(w, args, 1);

	root_x		= xevent->xkey.x_root;
	root_y		= xevent->xkey.y_root;
	x		= root_x;
	y		= root_y;
	help_window	= xevent->xkey.root;

	if (help_model == (OlDefine)OL_POINTER)
	{
		Window		tmp_window;
		Window		child_window = xevent->xkey.root;


			/* find leaf-most window of RootWindow under
			 * the pointer.					*/

		do {
			tmp_window	= help_window;
			help_window	= child_window;
			if (!XTranslateCoordinates(xevent->xkey.display,
					tmp_window, help_window,
					x, y, &x, &y, &child_window))
			{
				return;
			}
		} while (child_window != (Window)None);

			/* If no widget is associated with this window,
			 * return since the pointer is probably over
			 * another client and we won't deal with
			 * them now.					*/

		if (XtWindowToWidget(xevent->xkey.display, help_window)
			== (Widget)NULL)
		{
			return;
		}

		if (help_window == xevent->xkey.root)
		{
			/* Force creation of the Help Tree so we can
			 * check to see if this application allows help
			 * for the RootWindow.				*/

		{	/* This is the same policy of _OlPopupHelpTree() .
				If Help window comes up from Root , it does 
				not have any text_format data . This for-loop
				can get a p->text_format from HashTable in 
				order to pass it to Help widget . This p
				should be set in OlRegisterHelp .
			*/
			int i = 0;
			for(i = 0; i < N_ATOM_HASH_ENT; i++){
				if(HashTbl[i] != 0)
					text_format = (HashTbl[i])->text_format;
			}
		}

			(void) CreateHelpTree(w, (XtPointer)NULL, help_window,
					text_format);

			if (help_widget != (HelpWidget)NULL &&
			    help_widget->help.allow_root_help == False)
			{
				return;
			}
			x = xevent->xkey.x_root;
			y = xevent->xkey.y_root;
		}
	}
	else	/* focus-based */
	{
		Window	ignore_child;
		Widget	fw;

		if ((fw = OlGetCurrentFocusWidget(w)) == (Widget)NULL &&
		    (fw = XtWindowToWidget(xevent->xkey.display,help_window))
			== (Widget)NULL)
		{
			fw = w;
		}

		if (_OlIsGadget(fw) == True)
		{
			help_window = XtWindowOfObject(fw);
			x = (int)(fw->core.x + HALF_DIM(fw, width));
			y = (int)(fw->core.y + HALF_DIM(fw, height));
		}
		else if (XtIsSubclass(w, flatWidgetClass) != True)
		{
			help_window = XtWindowOfObject(fw);
			x = HALF_DIM(fw, width);
			y = HALF_DIM(fw, height);
		}
		else		/* flat widget	*/
		{
			Cardinal	i = OlFlatGetFocusItem(fw);

			help_window = XtWindow(fw);

			if (i != (Cardinal)OL_NO_ITEM)
			{
				Position	xp;
				Position	yp;
				Dimension	width;
				Dimension	height;

				OlFlatGetItemGeometry(fw, i, &xp, &yp,
							&width, &height);

				x = (int)(xp + width/(Dimension)2);
				y = (int)(yp + height/(Dimension)2);
			}
			else
			{
				x = HALF_DIM(fw, width);
				y = HALF_DIM(fw, height);
			}
		}


		XTranslateCoordinates(xevent->xkey.display,
				XtWindowOfObject(fw),
				RootWindowOfScreen(XtScreenOfObject(fw)),
				x, y, &root_x, &root_y, &ignore_child);
	}

	/*
	 * Forge a client-message for help.  The fields are
	 * undocumented, window manager implementation dependent --
	 * YUCK!!!
	 */

	xcm.type		= ClientMessage;
	xcm.serial		= xevent->xkey.serial;
	xcm.send_event		= xevent->xkey.send_event;
	xcm.display		= xevent->xkey.display;
	xcm.window		= xevent->xkey.window;
	xcm.message_type	= OlInternAtom(xevent->xany.display,
					       _OL_HELP_KEY_NAME);
	xcm.format		= 32;
	xcm.data.l[0]		= help_window;
	xcm.data.l[1]		= x;
	xcm.data.l[2]		= y;
	xcm.data.l[3]		= root_x;
	xcm.data.l[4]		= root_y;

	_OlPopupHelpTree(w, NULL, (XEvent *)&xcm, (Boolean*)NULL);
} /* END OF _OlProcessHelpKey() */
