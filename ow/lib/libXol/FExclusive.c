#pragma ident	"@(#)FExclusive.c	302.10	97/03/26 lib/libXol SMI"	/* flat:FExclusive.c 1.47	*/

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

/*
 *************************************************************************
 *
 * Description:
 *	This file contains the source code for the flat exclusive
 *	containers.
 *
 ******************************file*header********************************
 */


#include <stdio.h>
#include <stdlib.h>

#include <X11/IntrinsicP.h>
#include <X11/StringDefs.h>

#include <Xol/Error.h>
#include <Xol/FExclusivP.h>
#include <Xol/Menu.h>
#include <Xol/OpenLookP.h>


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

static void	CallCallbacks(Widget w, FlatPart *fp, Cardinal item_index);	/* Calls callbacks for item	*/
static Boolean	CheckLayoutParameters(Widget current, FlatPart *cfp, Widget new, FlatPart *nfp);/* checks layout param. validity*/
static void	SetItemField (Widget, String);
static int	IsSet (Widget, Cardinal);

					/* class procedures		*/

static void	AnalyzeItems (Widget, ArgList, Cardinal *);
static void	ClassInitialize(void);	/* Initializes the class	*/
static void	DrawItem(Widget w, FlatItem item_rec, OlFlatDrawInfo *di);		/* Draws a single item		*/
static void	HighlightHandler (Widget, OlDefine);
static void	Initialize(Widget request, Widget new, ArgList args, Cardinal *num_args);		/* Initializes instance		*/
static Boolean	ItemActivate (Widget, FlatItem, OlVirtualName,
					XtPointer);
static void	ItemInitialize (Widget, FlatItem, FlatItem,
						ArgList, Cardinal *);
static void	ItemDimensions (Widget, FlatItem, Dimension *,
					Dimension *);
static void	ItemsTouched (Widget, Widget, Widget, ArgList,
					Cardinal *);
static Boolean	ItemSetValues (Widget, FlatItem, FlatItem,
					FlatItem, ArgList, Cardinal *);
static Boolean	SetValues(Widget current, Widget request, Widget new, ArgList args, Cardinal *num_args);		/* instance data monitoring	*/

					/* action procedures		*/

static void	ButtonHandler (Widget, OlVirtualEvent);
static void	SelectItem (Widget);
static void	SetDefault (Widget, Cardinal);
static void	SetToCurrent (Widget, Boolean, Cardinal);
static void	UnsetCurrent (Widget);

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

static const Boolean	def_false = (Boolean)False;

#define W_CNAME(w)	(XtClass(w)->core_class.class_name)
#define FPART(w)	(&((FlatWidget)(w))->flat)
#define FEPART(w)	(&((FlatExclusivesWidget)(w))->exclusives)

#define FECPART(w) (&((FlatExclusivesWidgetClass)XtClass(w))->exclusives_class)
#define FEIPART(i) (&((FlatExclusivesItem)(i))->exclusives)

				/* Define the resources that are
				 * common to both the sub-objects and
				 * its container.			*/

#define INHERITED_ITEM_RESOURCES(base)\
\
	{ XtNclientData, XtCClientData, XtRPointer, sizeof(XtPointer),\
	  OFFSET(base, client_data), XtRPointer, (XtPointer) NULL },\
\
	{ XtNselectProc, XtCCallbackProc, XtRCallbackProc,\
	  sizeof(XtCallbackProc), OFFSET(base, select_proc),\
	  XtRCallbackProc, (XtPointer) NULL },\
\
	{ XtNunselectProc, XtCCallbackProc, XtRCallbackProc,\
	  sizeof(XtCallbackProc), OFFSET(base, unselect_proc),\
	  XtRCallbackProc, (XtPointer) NULL }
/* End of INHERITED_ITEM_RESOURCES macro definition */

/*
 *************************************************************************
 *
 * Define Translations and Actions
 *
 ***********************widget*translations*actions***********************
 */

/* Note: since the 'augment' and 'override' directives don't work for
 * class translations, we have to copy the generic translations and then
 * append what we need.  See the ClassInitialize Procedure.
 */
#if Xt_augment_works_right
Olconst static char
translations[] = "#augment\
	<Enter>:	OlAction() \n\
	<Leave>:	OlAction() \n\
	<BtnMotion>:	OlAction()";
#endif

static OlEventHandlerRec
event_procs[] =
{
	{ ButtonPress,	ButtonHandler	},
	{ ButtonRelease,ButtonHandler	},
	{ EnterNotify,	ButtonHandler	},
	{ LeaveNotify,	ButtonHandler	},
	{ MotionNotify, ButtonHandler	}
};

/*
 *************************************************************************
 *
 * Define Resource list associated with the Widget Instance
 *
 ****************************widget*resources*****************************
 */

	/* Specify resources that we want the flat class to manage
	 * internally if the application doesn't put them in their
	 * item fields list.						*/

static OlFlatReqRsc
required_resources[] = {
	{ XtNset, (OlFlatReqRscPredicateFunc)NULL },
	{ XtNdefault, (OlFlatReqRscPredicateFunc)NULL }
};

#define OFFSET(f)	XtOffsetOf(FlatExclusivesRec, f)
static XtResource
resources[] = {
				/* Declare resources to override the
				 * superclass's values			*/

	{ XtNhSpace, XtCHSpace, XtRDimension, sizeof(Dimension),
	  OFFSET(flat.h_space), XtRImmediate, (XtPointer) OL_IGNORE },

	{ XtNvSpace, XtCVSpace, XtRDimension, sizeof(Dimension),
	  OFFSET(flat.v_space), XtRImmediate, (XtPointer) OL_IGNORE },

	{ XtNsameWidth, XtCSameWidth, XtROlDefine, sizeof(OlDefine),
	  OFFSET(flat.same_width), XtRImmediate, (XtPointer) OL_COLUMNS },

					/* Declare Container resources	*/

	{ XtNdefault, XtCDefault, XtRBoolean, sizeof(Boolean),
	  OFFSET(exclusives.has_default), XtRImmediate, (XtPointer) FALSE },

	{ XtNdim, XtCDim, XtRBoolean, sizeof(Boolean),
	  OFFSET(exclusives.dim), XtRImmediate, (XtPointer) False },

	{ XtNexclusives, XtCExclusives, XtRBoolean, sizeof(Boolean),
	  OFFSET(exclusives.exclusive_settings),
	  XtRImmediate, (XtPointer) True },

	{ XtNnoneSet, XtCNoneSet, XtRBoolean, sizeof(Boolean),
	  OFFSET(exclusives.none_set), XtRImmediate,
	  (XtPointer) False },

	{ XtNpreview, XtCPreview, XtRWidget, sizeof(Widget),
	  OFFSET(exclusives.preview), XtRWidget, (XtPointer) NULL },

	{ XtNpreviewItem, XtCPreviewItem, XtRCardinal, sizeof(Cardinal),
	  OFFSET(exclusives.preview_item), XtRImmediate,
	  (XtPointer) OL_NO_ITEM },

	{ XtNpostSelect, XtCCallback, XtRCallback, sizeof(XtCallbackList),
	  OFFSET(exclusives.post_select), XtRCallback, (XtPointer) NULL },

			/* Include Resources common for the container
			 * and the sub-objects.
			 */

#undef OFFSET
#define OFFSET(base, field) \
			XtOffsetOf(base, exclusives.item_part.field)
	INHERITED_ITEM_RESOURCES(FlatExclusivesRec)
};


				/* Define Resources for sub-objects	*/

#undef OFFSET
#define OFFSET(base,field) XtOffsetOf(base,exclusives.field)

static XtResource
item_resources[] = {

			/* define resources that are not inherited from
			 * the container.				*/

	{ XtNset, XtCSet, XtRBoolean, sizeof(Boolean),
	  OFFSET(FlatExclusivesItemRec, set), XtRBoolean,
	  (XtPointer) &def_false },

	{ XtNdefault, XtCDefault, XtRBoolean, sizeof(Boolean),
	  OFFSET(FlatExclusivesItemRec, is_default), XtRBoolean,
	  (XtPointer) &def_false },

	INHERITED_ITEM_RESOURCES(FlatExclusivesItemRec)
};

/*
 *************************************************************************
 *
 * Define Class Record structure to be initialized at Compile time
 *
 ***************************widget*class*record***************************
 */

FlatExclusivesClassRec
flatExclusivesClassRec = {
    {
	(WidgetClass)&flatClassRec,		/* superclass		*/
	"FlatExclusives",			/* class_name		*/
	sizeof(FlatExclusivesRec),		/* widget_size		*/
	ClassInitialize,			/* class_initialize	*/
	NULL,					/* class_part_initialize*/
	FALSE,					/* class_inited		*/
	Initialize,				/* initialize		*/
	NULL,					/* initialize_hook	*/
	XtInheritRealize,			/* realize		*/
	NULL,					/* actions		*/
	0,					/* num_actions		*/
	resources,				/* resources		*/
	XtNumber(resources),			/* num_resources	*/
	NULLQUARK,				/* xrm_class		*/
	TRUE,					/* compress_motion	*/
	TRUE,					/* compress_exposure	*/
	TRUE,					/* compress_enterleave	*/
	FALSE,					/* visible_interest	*/
	NULL,					/* destroy		*/
	XtInheritResize,			/* resize		*/
	XtInheritExpose,			/* expose		*/
	SetValues,				/* set_values		*/
	NULL,					/* set_values_hook	*/
	XtInheritSetValuesAlmost,		/* set_values_almost	*/
	NULL,					/* get_values_hook	*/
	XtInheritAcceptFocus,			/* accept_focus		*/
	XtVersion,				/* version		*/
	NULL,					/* callback_offsets	*/
	NULL /* See ClassInitialize */,		/* tm_table		*/
	NULL,					/* query_geometry	*/
	NULL,					/* display_accelerator	*/
	NULL					/* extension		*/
    }, /* End of Core Class Part Initialization */
    {
	NULL,					/* reserved1		*/
	HighlightHandler,			/* highlight_handler	*/
	XtInheritTraversalHandler,		/* traversal_handler	*/
	NULL,					/* register_focus	*/
	XtInheritActivateFunc,			/* activate		*/
	event_procs,				/* event_procs		*/
	XtNumber(event_procs),			/* num_event_procs	*/
	OlVersion,				/* version		*/
	(XtPointer)NULL,			/* extension		*/
	{
		(_OlDynResourceList)NULL,	/* resources		*/
		(Cardinal)0			/* num_resources	*/
	},					/* dyn_data		*/
	XtInheritTransparentProc,		/* transparent_proc	*/
	XtInheritSuperCaretQueryLocnProc,	/* query_sc_locn_proc	*/
    },	/* End of Primitive Class Part Initialization */
    {
	item_resources,				/* item_resources	*/
	XtNumber(item_resources),		/* num_item_resources	*/
	required_resources,			/* required_resources	*/
	XtNumber(required_resources),		/* num_required_resources*/
	NULL,					/* quarked_items	*/
	(Cardinal)sizeof(FlatExclusivesItemPart),/* part_size		*/
	XtOffset(FlatExclusivesWidget,
		exclusives.item_part),		/* part_offset		*/
	XtOffset(FlatExclusivesItem,exclusives),/* part_in_rec_offset	*/
	sizeof(FlatExclusivesItemRec),		/* rec_size		*/
	True,					/* transparent_bg	*/
	AnalyzeItems,				/* analyze_items	*/
	DrawItem,				/* draw_item		*/
	NULL,					/* expand_item		*/
	XtInheritFlatGetDrawInfo,		/* get_draw_info	*/
	XtInheritFlatGetIndex,			/* get_index		*/
	XtInheritFlatItemAcceptFocus,		/* item_accept_focus	*/
	ItemActivate,				/* item_activate	*/
	ItemDimensions,				/* item_dimensions	*/
	(OlFlatItemGetValuesProc)NULL,		/* item_get_values	*/
	XtInheritFlatItemHighlight,		/* item_highlight	*/
	ItemInitialize,				/* item_initialize	*/
	ItemSetValues,				/* item_set_values	*/
	ItemsTouched,				/* items_touched	*/
	OlThisFlatClass,			/* layout_class		*/
	XtInheritFlatLayout,			/* layout		*/
	XtInheritFlatTraverseItems,		/* traverse_items	*/
	(XtPointer)NULL,			/* reserved2		*/
	(XtPointer)NULL,			/* reserved1		*/
	(XtPointer)NULL				/* extension		*/
    }, /* End of Flat Class Part Initialization */
    {
	True					/* allow_class_default	*/
    } /* End of FlatExclusives Class Part Initialization */
};

/*
 *************************************************************************
 *
 * Public Widget Class Definition of the Widget Class Record
 *
 *************************public*class*definition*************************
 */

WidgetClass flatExclusivesWidgetClass = (WidgetClass)&flatExclusivesClassRec;

/*
 *************************************************************************
 *
 * Private Procedures
 *
 ***************************private*procedures****************************
 */

/*
 *************************************************************************
 * CallCallbacks - this routine calls the callbacks for the given item.
 * the 'set' indicates whether the set or unset callbacks should be
 * called.
 ****************************procedure*header*****************************
 */
static void
CallCallbacks(Widget w, FlatPart *fp, Cardinal item_index)
{
	Arg			args[4];
	XtCallbackProc		select_proc;
	XtCallbackProc		unselect_proc;
	XtCallbackProc		callback;
	XtPointer		client_data;
	Boolean			set;

					/* Get the Item's callbacks	*/

	XtSetArg(args[0], XtNselectProc, &select_proc);
	XtSetArg(args[1], XtNunselectProc, &unselect_proc);
	XtSetArg(args[2], XtNclientData, &client_data);
	XtSetArg(args[3], XtNset, &set);
	OlFlatGetValues(w, item_index, args, 4);

	callback = (set == True ? select_proc : unselect_proc);

						/* Call the callbacks	*/

	_OlManagerCallPreSelectCBs(w, (XEvent *)NULL);

	if (callback != (XtCallbackProc) NULL)
	{
		OlFlatCallData	call_data;

				/* Set up the call data structure	*/

		call_data.item_index		= item_index;
		call_data.items			= fp->items;
		call_data.num_items		= fp->num_items;
		call_data.item_fields		= fp->item_fields;
		call_data.num_item_fields	= fp->num_item_fields;

		(* callback)(w->core.self, client_data, &call_data);
	}

	_OlManagerCallPostSelectCBs(w, (XEvent *)NULL);

	if (XtHasCallbacks(w, XtNpostSelect) == XtCallbackHasSome)
	{
		XtCallCallbacks(w, XtNpostSelect, NULL);
	}
} /* END OF CallCallbacks() */

/*
 *************************************************************************
 * CheckLayoutParameters - this routine checks the validity of various
 * layout parameters.  If any of the parameters are invalid, a warning
 * is generated and the values are set to a valid value.
 ****************************procedure*header*****************************
 */
static Boolean
CheckLayoutParameters(Widget current, FlatPart *cfp, Widget new, FlatPart *nfp)
	      		        	/* Current widget or NULL	*/
	          	    		/* Current Flat Part or NULL	*/
	      		    		/* New widget id		*/
	          	    		/* New Flat Part		*/
{
	String		error_type;
	String		error_msg;
	Boolean		relayout = False;

	if (current == (Widget) NULL)
	{
		error_type = OleTinitialize;
		error_msg  = OleMinvalidResource_initialize;
	}
	else
	{
		error_type = OleTsetValues;
		error_msg  = OleMinvalidResource_setValues;
	}

			/* Define a macro to speed things up (typing that is)
			 * and make sure that there are no spaces after
			 * the commas when this is used.		*/

#define CLEANUP(field, rsc, string, type, value) \
	OlVaDisplayWarningMsg((Display *)NULL, OleNinvalidResource,\
		error_type, OleCOlToolkitWarning, error_msg,\
		XtName(new), W_CNAME(new), rsc, string);\
	nfp->field = (type)value;\
	if (current == (Widget)NULL || nfp->field != cfp->field){\
		relayout = True;\
	}

	if (XtClass(new) == flatExclusivesWidgetClass)
	{
		if (nfp->h_space != (Dimension)0 &&
		    nfp->h_space != (Dimension)OL_IGNORE)
		{
			CLEANUP(h_space, "XtNhSpace", "zero", Dimension, 0)
		}

		if (nfp->v_space != (Dimension)0 &&
		    nfp->v_space != (Dimension)OL_IGNORE)
		{
			CLEANUP(v_space, "XtNvSpace", "zero", Dimension, 0)
		}

		if (nfp->same_width != (OlDefine) OL_COLUMNS &&
		    nfp->same_width != (OlDefine) OL_ALL)
		{
			CLEANUP(same_width, "XtNsameWidth", "OL_COLUMNS",
				OlDefine, OL_COLUMNS);
		}

		if (nfp->same_height != (OlDefine) OL_ROWS &&
		    nfp->same_height != (OlDefine) OL_ALL)
		{
			CLEANUP(same_height, "XtNsameHeight", "OL_ALL",
				OlDefine, OL_ALL);
		}
	}

#undef CLEANUP
	return(relayout);
} /* END OF CheckLayoutParameters() */

/*
 *************************************************************************
 * SetItemField - this routine is used to make an item a 'set' item or
 * make an item be the 'default' item.
 ****************************procedure*header*****************************
 */
static void
SetItemField (Widget w, String name)
{
	FlatPart *	fp = FPART(w);
	Cardinal	i;
	Arg		args[2];
	Boolean		managed;
	Boolean		mapped_when_managed;

	XtSetArg(args[0], XtNmanaged, &managed);
	XtSetArg(args[1], XtNmappedWhenManaged, &mapped_when_managed);

	for (i=0; i < fp->num_items; ++i)
	{
		managed			= False;
		mapped_when_managed	= False;
		OlFlatGetValues(w, i, args, 2);

		if (managed == True &&
		    mapped_when_managed == True)
		{
			XtSetArg(args[0], name, True);
			OlFlatSetValues(w, i, args, 1);
			break;
		}
	}
} /* END OF SetItemField() */

/*
 *************************************************************************
 * IsSet - this routine returns a zero value if the item is set and a
 * non-zero value if the item is set.
 ****************************procedure*header*****************************
 */
static int
IsSet (Widget w, Cardinal item_index)
{
	FlatExclusivesPart *	fexcp = FEPART(w);
	int			is_set;

	if (fexcp->exclusive_settings == True)
	{
		is_set = (fexcp->set_item == item_index);
	}
	else
	{
		Arg	args[1];
		Boolean	value;

		XtSetArg(args[0], XtNset, &value);
		OlFlatGetValues(w, item_index, args, 1);

		is_set = (value == True);
	}
	return(is_set);
} /* END OF IsSet() */

/*
 *************************************************************************
 *
 * Class Procedures
 *
 ****************************class*procedures*****************************
 */

/*
 *************************************************************************
 * AnalyzeItems - this routine analyzes new items for this class.
 * This routine is called after all of the items have been initialized,
 * so we can assume that all items have valid values.
 * Also, the ItemInitialize routine has already found the set item and
 * the default item (if applicable), provided the application set them.
 * If not, this routine will choose them.
 ****************************procedure*header*****************************
 */
/* ARGSUSED */
static void
AnalyzeItems (Widget w, ArgList args, Cardinal *num_args)
{
	FlatExclusivesPart *	fexcp = FEPART(w);
	
				/* Now, if none set is False, we have to
				 * set an item to be true if no items
				 * are set				*/

	if (fexcp->set_item == (Cardinal) OL_NO_ITEM &&
	    fexcp->exclusive_settings == True &&
	    fexcp->none_set == False)
	{
		SetItemField(w, XtNset);
	}


			/* If a default is allowed, attempt to set
			 * one.  Note, we turn off the 'has_default'
			 * flag since the ItemSetValues will set it
			 * to TRUE if an item becomes the default.	*/

	if (fexcp->has_default == True &&
	    fexcp->default_item == (Cardinal) OL_NO_ITEM)
	{
		fexcp->has_default = False;

		if (fexcp->allow_instance_default == True)
		{
			SetItemField(w, XtNdefault);
		}
	}
} /* END OF AnalyzeItems() */

/*
 *************************************************************************
 * ClassInitialize - this procedure initializes the virtual translations
 ****************************procedure*header*****************************
 */
static void
ClassInitialize(void)
{
	const char * new_productions = "\
		<Enter>:	OlAction() \n\
		<Leave>:	OlAction() \n\
		<BtnMotion>:	OlAction() \n";

	char *	translations = (char *)XtMalloc(
			strlen(_OlGenericTranslationTable) +
			strlen(new_productions) + 1);

	sprintf(translations, "%s%s", new_productions,
				_OlGenericTranslationTable);
	flatExclusivesWidgetClass->core_class.tm_table = translations;

} /* END OF ClassInitialize() */

/*
 *************************************************************************
 * DrawItem - this routine draws a single item for the container.
 ****************************procedure*header*****************************
 */
static void
DrawItem(Widget w, FlatItem item_rec, OlFlatDrawInfo *di)
	      			  		/* container widget id	*/
	        		         	/* expanded item	*/
	                	   		/* Drawing information	*/
{
	Cardinal			item_index =
						item_rec->flat.item_index;
	FlatExclusivesPart *		fexcp = FEPART(w);
	Boolean				draw_selected;
	FlatExclusivesItemPart *	item_part = FEIPART(item_rec);
	int				flags = 0;
	OlgxAttrs *			item_attrs;
	OlDrawProc			drawProc;
	FlatLabel			*lbl;

	if (item_rec->flat.mapped_when_managed == False)
	{
		return;
	}

					/* Determine if we should draw
					 * the selected box	*/

	if (fexcp->exclusive_settings == True)
	{
		draw_selected = (Boolean)
			((item_index == fexcp->current_item &&
			!(fexcp->none_set == True &&
			fexcp->current_item == fexcp->set_item))
					||
			(fexcp->current_item == (Cardinal) OL_NO_ITEM &&
		     	item_part->set == True) ? True : False);
	}
	else
	{
		draw_selected = (Boolean)
			((item_index == fexcp->current_item &&
			  item_part->set == False)
					||
	     		(fexcp->current_item != item_index &&
			item_part->set == True) ? True : False);
	}

	if (fexcp->preview != (Widget)NULL)
	{
		draw_selected = (draw_selected == True ? False : True);
	}

	if (draw_selected == True)
	{
	        flags |= OLGX_INVOKED;
	}

					/* Draw the default box		*/

	if (fexcp->default_item == item_index || item_part->is_default == True)
	{
	        flags |= OLGX_DEFAULT;
	}

	if (fexcp->dim == True || 
	    item_rec->flat.sensitive == (Boolean)False || !XtIsSensitive(w))
	{
	        flags |= OLGX_INACTIVE;
	}

	/* Get GCs and label information */
	_OlFlatSetupAttributes (w, item_rec, di, &item_attrs, &lbl, &drawProc);

	if (item_rec->flat.label != (String)NULL) {
	    if (item_rec->flat.text_format == OL_WC_STR_REP)
		flags |= OLGX_LABEL_IS_WCS;
	} else {
	    if (lbl->pixmap.type == PL_IMAGE)
		flags |= OLGX_LABEL_IS_XIMAGE;
	    else
		flags |= OLGX_LABEL_IS_PIXMAP;
	}

	/* Draw the button */
	(*drawProc)(di->screen, di->drawable, item_attrs, di->x, di->y,
	    di->width, di->height, lbl, OL_RECTBUTTON, flags);

} /* END OF DrawItem() */

/*
 *************************************************************************
 * HighlightHandler - this routine is called whenever the flattened widget
 * container gains or looses focus.
 * We need this routine because the flattened exclusives widgets do
 * want want to remember the last_focus_item.  Therefore, we'll let
 * our superclass do the really work, and then we'll unset this field.
 ****************************procedure*header*****************************
 */
static void
HighlightHandler (Widget w, OlDefine type)
{
	FlatWidgetClass	fwc = (FlatWidgetClass)
			  flatExclusivesWidgetClass->core_class.superclass;

	(*fwc->primitive_class.highlight_handler)(w, type);

	if (type == OL_OUT)
	{
		FPART(w)->last_focus_item = (Cardinal)OL_NO_ITEM;
	}
} /* END OF HighlightHandler() */

/*
 *************************************************************************
 * Initialize - this procedure initializes the instance part of the widget
 ****************************procedure*header*****************************
 */
/* ARGSUSED */
static void
Initialize(Widget request, Widget new, ArgList args, Cardinal *num_args)
	      	        	/* What the application requested	*/
	      	    		/* What the application gets, so far...	*/
	       		     
	          	         
{
	FlatWidget			fw = (FlatWidget)new;
	FlatPart *			fp = FPART(new);
	FlatExclusivesPart *		fexcp = FEPART(new);
	FlatExclusivesClassPart *	fexccp = FECPART(new);
	Widget				shell = _OlGetShellOfWidget(new);

			/* Insure the 'is_default' item_part field is 
			 * always FALSE.				*/

	fexcp->item_part.is_default = (Boolean)False;

					/* Initialize the instance data	*/

	fexcp->menu_descendant	= (shell != (Widget)NULL &&
			XtIsSubclass(shell, menuShellWidgetClass) == True ?
				True : False);

			/* Set up the horizontal and vertical spacing	*/

	if (fp->h_space == (Dimension) OL_IGNORE)
	{
		fp->h_space = (Dimension)
			(fexcp->exclusive_settings == True ? 0 :
#ifdef sun
				OlScreenPointToPixel(OL_HORIZONTAL,
					(fw->primitive.scale / 2),
					XtScreen(new)));
#else
				OlScreenPointToPixel(OL_HORIZONTAL,
					(OL_DEFAULT_POINT_SIZE / 2),
					XtScreen(new)));
#endif
	}

	if (fp->v_space == (Dimension) OL_IGNORE)
	{
		fp->v_space = (Dimension)
			(fexcp->exclusive_settings == True ? 0 :
#ifdef sun
				OlScreenPointToPixel(OL_VERTICAL,
					(fw->primitive.scale / 2),
					XtScreen(new)));
#else
				OlScreenPointToPixel(OL_VERTICAL,
					(OL_DEFAULT_POINT_SIZE / 2),
					XtScreen(new)));
#endif
	}

						/* Set the item overlap	*/

	fp->overlap = OlgIs3d(new->core.screen) ? 0 
			: OlgxScreenPointToPixel(OL_HORIZONTAL, 1, fp->pAttrs->scr);

			/* Modify the XtNnoneSet resource if this is not
			 * an exclusive settings			*/

	if (fexcp->exclusive_settings == False &&
	    fexcp->none_set == False)
	{
		fexcp->none_set = True;
	}
	
				/* See if this instance is permitted to
				 * have a default sub-object		*/

	fexcp->allow_instance_default = (Boolean)
			(fexccp->allow_class_default == False ||
			 fexcp->menu_descendant == False ? False : True);

	(void) CheckLayoutParameters((Widget)NULL, (FlatPart *)NULL, new, fp);

				/* Call the layout convenience routine	*/

	(void) _OlFlatCheckItems(flatExclusivesWidgetClass, (Widget)NULL,
				request, new, args, num_args);

} /* END OF Initialize() */

/*
 *************************************************************************
 * ItemActivate - this routine is used to activate this widget.
 ****************************procedure*header*****************************
 */
/* ARGSUSED */
static Boolean
ItemActivate (Widget w, FlatItem item, OlVirtualName type, XtPointer data)
{
	FlatExclusivesPart *	fexcp = FEPART(w);
	Boolean			ret_val;
	Cardinal		current = fexcp->current_item;

	switch(type) {
	case OL_SELECTKEY:
		ret_val = TRUE;
		fexcp->current_item = item->flat.item_index;

		if (current != (Cardinal)OL_NO_ITEM) {
			_OlFlatRefreshItem(w, current, True);
		}
		SelectItem(w);
		break;
	case OL_MENUDEFAULTKEY:
		ret_val = TRUE;
		SetDefault(w, item->flat.item_index);
		break;
	default:
		ret_val = FALSE;
	}
	return(ret_val);
} /* END OF ItemActivate() */

/*
 *************************************************************************
 * ItemInitialize - this procedure checks a single item to see if
 * it have valid values.
 ****************************procedure*header*****************************
 */
/* ARGSUSED */
static void
ItemInitialize (Widget w, FlatItem request, FlatItem new, ArgList args, Cardinal *num_args)
{
	FlatExclusivesPart *		fexcp = FEPART(w);
	FlatExclusivesItemPart *	item_part = FEIPART(new);
	Cardinal			item_index = new->flat.item_index;

				/* Determine if this item can be the
				 * default item for the setting.	*/

	if (item_part->is_default == True)
	{
		if (fexcp->default_item != (Cardinal) OL_NO_ITEM)
		{
			item_part->is_default = False;

			OlVaDisplayWarningMsg(XtDisplay(w),
				OleNtooManyDefaults, OleTflatState,
				OleCOlToolkitWarning,
				OleMtooManyDefaults_flatState,
				XtName(w), W_CNAME(w),
				(unsigned)item_index,
				(unsigned)fexcp->default_item);
		}
		else if (fexcp->allow_instance_default == True)
		{
			fexcp->default_item	= item_index;
			fexcp->has_default	= True;
		}
		else
		{
			fexcp->default_item	= (Cardinal)OL_NO_ITEM;
			fexcp->has_default	= False;
		}
	}

					/* Determine if this item
					 * can be in the set state.	*/

	if (fexcp->exclusive_settings == True &&
	    item_part->set == True)
	{
		if (fexcp->set_item == (Cardinal) OL_NO_ITEM)
		{
			fexcp->set_item = item_index;
		}
		else if (fexcp->set_item != item_index)
		{
			item_part->set = False;

			OlVaDisplayWarningMsg(XtDisplay(w),
				OleNtooManySet, OleTflatState,
				OleCOlToolkitWarning,
				OleMtooManySet_flatState,
				XtName(w), W_CNAME(w),
				(unsigned)item_index,
				(unsigned)fexcp->set_item);
		}
	}
} /* END OF ItemInitialize() */

/*
 *************************************************************************
 * ItemDimensions - this routine determines the size of a single sub-object
 ****************************procedure*header*****************************
 */
static void
ItemDimensions(Widget w, FlatItem item_rec, register Dimension *width, register Dimension *height)
	      			  	/* Widget making request	*/
	        		         	/* expanded item	*/
	                    	      	/* returned width		*/
	                    	       	/* returned height		*/
{
	FlatPart *		fp = FPART(w);
	void			(*sizeProc)();
	FlatLabel		*lbl;

	/* Get label information */
	_OlFlatSetupLabelSize (w, item_rec, &lbl, &sizeProc);

	OlgxSizeRectButton(XtScreen (w), fp->pAttrs, (XtPointer)lbl, 
			   sizeProc, 0, width, height);

} /* END OF ItemDimensions() */

/*
 *************************************************************************
 * ItemsTouched - this procedure is called whenever a new list is given to
 * the flat widget or whenever the existing list is touched.
 ****************************procedure*header*****************************
 */
/* ARGSUSED */
static void
ItemsTouched (Widget current, Widget request, Widget new, ArgList args, Cardinal *num_args)
{
	FlatExclusivesPart *	fexcp = FEPART(new);

	fexcp->preview_item	= (Cardinal) OL_NO_ITEM;
	fexcp->current_item	= (Cardinal) OL_NO_ITEM;
	fexcp->set_item		= (Cardinal) OL_NO_ITEM;
	fexcp->default_item	= (Cardinal) OL_NO_ITEM;
} /* END OF ItemsTouched() */

/*
 *************************************************************************
 * ItemSetValues - this routine is called whenever the application does
 * an XtSetValues on the container, requesting that an item be updated.
 * If the item is to be refreshed, the routine returns True.
 ****************************procedure*header*****************************
 */
/* ARGSUSED */
static Boolean
ItemSetValues (Widget w, FlatItem current, FlatItem request, FlatItem new, ArgList args, Cardinal *num_args)
{
	Cardinal			item_index = new->flat.item_index;
	Boolean				redisplay = False;
	FlatExclusivesPart *		fexcp = FEPART(w);
	FlatExclusivesItemPart *	npart = FEIPART(new);
	FlatExclusivesItemPart *	cpart = FEIPART(current);

#define DIFFERENT(field)	(npart->field != cpart->field)

		/* If the 'set' state changed, update the internal
		 * fields.  Note, some times its ok to redraw
		 * the item without indirectly causing an XClearArea
		 * request.  When this optimization can occur, a call
		 * to _OlFlatRefreshItem is done instead of
		 * setting the 'redisplay' flag to True.		*/

	if (DIFFERENT(set))
	{
	    if (fexcp->exclusive_settings == (Boolean)False)
	    {
		if (npart->set == True)
		{
			_OlFlatRefreshItem(w, item_index, False);
		}
		else
		{
			redisplay = True;
		}
	    }
	    else				/* Exclusives setting	*/
	    {
		if (npart->set == True)
		{
				/* If there's no currently set item,
				 * make this one be the set one		*/

			if (fexcp->set_item == (Cardinal)OL_NO_ITEM)
			{
				fexcp->set_item = item_index;
				_OlFlatRefreshItem(w, item_index, False);
			}
			else if (fexcp->set_item != item_index)
			{
				/* There is an item currently set, so
				 * make 'item_index' be the new set
				 * item and then unset the old one	*/

				Arg		args[1];
				Cardinal	old = fexcp->set_item;

						/* set the new item	*/

				fexcp->set_item = item_index;

					/* Unset the previous item	*/

				XtSetArg(args[0], XtNset, False);
				OlFlatSetValues(w, old, args, 1);


					/* Now refresh this item without
					 * generating an exposure.	*/

				_OlFlatRefreshItem(w, item_index, False);
			}
		}
		else			/* npart->set == False */
		{
			if (fexcp->set_item == item_index)
			{
				/* If none_set is true, we can unset this
				 * item; else, ignore the request.	*/

				if (fexcp->none_set == True)
				{
					fexcp->set_item = (Cardinal)OL_NO_ITEM;
					redisplay = True;
				}
			}
			else
			{
				redisplay = True;
			}
		}
	    }
	} /* end of DIFFERENT(set) */

	if (DIFFERENT(is_default))
	{
		if (fexcp->allow_instance_default == (Boolean)True)
		{
			redisplay = True;

			if (fexcp->default_item == (Cardinal)OL_NO_ITEM)
			{
				fexcp->default_item	= item_index;
				fexcp->has_default	= True;
				_OlSetDefault(w, True);
			}
			else if (fexcp->default_item == item_index)
			{
				fexcp->has_default	= False;
				fexcp->default_item	= (Cardinal)OL_NO_ITEM;
				_OlSetDefault(w, False);
			}
			else if (npart->is_default == True)
			{
				/* Case where this item wants to
				 * be the default and some other item
				 * is the current (but not for long)
				 * default item.  Set the index of the
				 * new default item and then turn off
				 * the old default item.		*/

				Cardinal	old = fexcp->default_item;
				Arg		args[1];

				fexcp->default_item = item_index;

				XtSetArg(args[0], XtNdefault, False);
				OlFlatSetValues(w, old, args, 1);
			}
		}
		else
		{
			npart->is_default = False;
		}
	}

#undef DIFFERENT
	return(redisplay);
} /* END OF ItemSetValues() */

/*
 *************************************************************************
 * SetValues - this procedure monitors the changing of instance data
 ****************************procedure*header*****************************
 */
/* ARGSUSED */
static Boolean
SetValues(Widget current, Widget request, Widget new, ArgList args, Cardinal *num_args)
	      	        		/* What we had			*/
	      	        		/* What we want			*/
	      	    			/* What we get, so far		*/
	       		     
	          	         
{
	Boolean			redisplay = False;
	FlatPart *		fp	= FPART(new);
	FlatExclusivesPart *	fexcp	= FEPART(new);
	FlatPart *		cfp	= FPART(current);
	FlatExclusivesPart *	cfexcp	= FEPART(current);

				/* Make sure the layout parameters
				 * are valid.				*/

	if (CheckLayoutParameters(current, cfp, new, fp) == True)
	{
		redisplay = True;
	}


#define DIFFERENT(field)	(fexcp->field != cfexcp->field)

	if (DIFFERENT(exclusive_settings) &&
	    fp->items_touched == False)
	{
		fexcp->exclusive_settings = cfexcp->exclusive_settings;

		OlVaDisplayWarningMsg(XtDisplay(new), "","",
			OleCOlToolkitWarning, "Widget \"%s\" (class %s):\
 cannot change resource XtNexclusives without XtNitemsTouched == True",
			XtName(new), W_CNAME(new));
	}

			/* Modify the XtNnoneSet resource if this is not
			 * an exclusive settings			*/

	if (fexcp->exclusive_settings == False &&
	    DIFFERENT(none_set) &&
	    fexcp->none_set == False)
	{
		fexcp->none_set = True;
	}

			/* Always make sure the field 'is_default' in the
			 * item_part is False.  Once we've done this
			 * check, it's safe for us to check to compare
			 * the new item with the current one.		*/

	if (fexcp->item_part.is_default != (Boolean)False)
	{
		fexcp->item_part.is_default = False;
	}

	if (DIFFERENT(has_default))
	{
		if (fexcp->allow_instance_default == False)
		{
				/* Don't issue a warning here since
				 * there are many existing applications
				 * that misuse this resource.		*/

			fexcp->has_default = False;
		}
		else if (fp->items_touched == False)
	   	{
			if (fexcp->default_item == (Cardinal) OL_NO_ITEM)
			{
				/* Unset the 'has_default' flag since
				 * the item's SetValue's proc will set
				 * it back to TRUE if an item becomes the
				 * default.				*/

				fexcp->has_default = False;
				SetItemField(new, XtNdefault);
			}
			else
			{
				Arg args[1];
				XtSetArg(args[0], XtNdefault, False);
				OlFlatSetValues(new, fexcp->default_item,
						args, 1);
			}
		}
		else
		{
			/* EMPTY */

			/* Do nothing here since the 'AnalyzeItems'
			 * procedure will pick a default.		*/
		}
	}

	if (DIFFERENT(preview) && fexcp->preview != (Widget)NULL)
	{
		_OlFlatPreviewItem(new, fexcp->default_item,
					fexcp->preview, fexcp->preview_item);
		fexcp->preview		= (Widget)NULL;
		fexcp->preview_item	= (Cardinal)OL_NO_ITEM;
	}

	if (DIFFERENT(dim))
	{
		redisplay = True;
	}

				/* Do the layout as the last thing	*/

	if (_OlFlatCheckItems(flatExclusivesWidgetClass, current, request, new,
				args, num_args) == True)
	{
		redisplay = True;
	}

#undef DIFFERENT
	return(redisplay);
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
 * ButtonHandler - this routine handles all button press and release
 * events.
 ****************************procedure*header*****************************
 */
static void
ButtonHandler (Widget w, OlVirtualEvent ve)
{
	Position	x;
	Position	y;
	Cardinal	item_index;

	switch(ve->xevent->type) {
	case ButtonPress:
	case EnterNotify:
	case MotionNotify:
		if (ve->xevent->type == MotionNotify)
		{
			x = ve->xevent->xmotion.x;
			y = ve->xevent->xmotion.y;
		}
		else if (ve->xevent->type == ButtonPress)
		{
			_OlUngrabPointer(w);
			x = ve->xevent->xbutton.x;
			y = ve->xevent->xbutton.y;
		}
		else	/* EnterNotify */
		{
			x = ve->xevent->xcrossing.x;
			y = ve->xevent->xcrossing.y;
		}

		item_index = _OlFlatGetIndex(w, x, y, (Boolean)False);

		switch(ve->virtual_name) {
		case OL_SELECT:
			SetToCurrent(w, False, item_index);
			break;
		case OL_MENU:
			SetToCurrent(w, True, item_index);
			break;
		case OL_MENUDEFAULT:
			SetDefault(w, item_index);
			break;
		default:
			UnsetCurrent(w);
			break;
		}
		break;
	case ButtonRelease:
		item_index = _OlFlatGetIndex(w,
					(Position) ve->xevent->xbutton.x,
					(Position) ve->xevent->xbutton.y,
					(Boolean)False);

		switch(ve->virtual_name) {
		case OL_SELECT:
			SetToCurrent(w, False, item_index);
			SelectItem(w);
			break;
		case OL_MENU:
			SetToCurrent(w, True, item_index);
			SelectItem(w);
			break;
		default:
			UnsetCurrent(w);
			break;
		}
		break;
	case LeaveNotify:
		UnsetCurrent(w);
		break;
	}
} /* END OF ButtonHandler() */

/*
 *************************************************************************
 * SelectItem - this routine selects the current item and activates its
 * callbacks.
 * Note: SetValues calls this routine with NULL values for xevent and
 * params.
 ****************************procedure*header*****************************
 */
static void
SelectItem (Widget w)
{
	Arg			args[1];
	FlatPart *		fp = FPART(w);
	FlatExclusivesPart *	fexcp = FEPART(w);
	Cardinal		current = fexcp->current_item;
	Cardinal		previous;


			/* If there's no current item, exit immediately	*/

	if (current == (Cardinal) OL_NO_ITEM)
	{
		return;
	}

	previous		= fexcp->set_item;
	fexcp->current_item	= (Cardinal) OL_NO_ITEM;

	if (fexcp->none_set == False)
	{
			/* This can be true only when we have an
			 * exclusives type setting.			*/

		if (current != previous)
		{
				/* Set the new Item -- this will unset
				 * the old item for us.  After we've
				 * set the items, we can call the
				 * callbacks.				*/

			XtSetArg(args[0], XtNset, True);
			OlFlatSetValues(w, current, args, 1);

			if (previous != (Cardinal) OL_NO_ITEM)
			{
				CallCallbacks(w, fp, previous);
			}

			CallCallbacks(w, fp, current);
		}
	}
	else if (fexcp->exclusive_settings == True)
	{
		if (current == previous)
		{
				/* Unset the current item and call
				 * its callback procedure.		*/

			XtSetArg(args[0], XtNset, False);
			OlFlatSetValues(w, current, args, 1);

			CallCallbacks(w, fp, current);
		}
		else				/* current != previous	*/
		{
			if (previous == (Cardinal) OL_NO_ITEM)
			{
				/* Set the new item and call its
				 * callbacks.				*/

				XtSetArg(args[0], XtNset, True);
				OlFlatSetValues(w, current, args, 1);

				CallCallbacks(w, fp, current);
			}
			else	/* current & previous are both valid	*/
			{
				/* Set the new item -- this will unset
				 * the old item for us.  After we've
				 * set the items, we can call the
				 * callbacks.				*/

				XtSetArg(args[0], XtNset, True);
				OlFlatSetValues(w, current, args, 1);

				CallCallbacks(w, fp, previous);
				CallCallbacks(w, fp, current);
			}
		}
	}
	else					/* Non-exclusives type	*/
	{
		XtSetArg(args[0], XtNset, (IsSet(w, current) ? False : True));
		OlFlatSetValues(w, current, args, 1);

		CallCallbacks(w, fp, current);
	}
} /* END OF SelectItem() */

/*
 *************************************************************************
 * SetDefault - this routine sets a setting to be the default item
 * whenever the user uses the "SET MENU DEFAULT" button or when the user
 * uses the keyboard.
 ****************************procedure*header*****************************
 */
static void
SetDefault (Widget w, Cardinal item_index)
{
	FlatExclusivesPart *	fexcp = FEPART(w);

	if (fexcp->allow_instance_default == True)
	{
		if (item_index != (Cardinal)OL_NO_ITEM &&
		    item_index != fexcp->default_item)
		{
			Arg	args[1];

			XtSetArg(args[0], XtNdefault, True);
			OlFlatSetValues(w, item_index, args, 1);
		}
	}
} /* END OF SetDefault() */

/*
 *************************************************************************
 * SetToCurrent - this routine mkaes the item under the pointer equal
 * to the current item.
 ****************************procedure*header*****************************
 */
static void
SetToCurrent (Widget w, Boolean is_menu_cmd, Cardinal item_index)
{
	FlatExclusivesPart *	fexcp = FEPART(w);
	Cardinal		old_current;

			/* If the command invoking this routine is a
			 * menu-related command and we're not on a menu,
			 * return.
			 */
	if (is_menu_cmd == True && fexcp->menu_descendant == False)
	{
		return;
	}

			/* Get the index of the new current item	*/

	old_current = fexcp->current_item;
	fexcp->current_item = item_index;

	if (old_current == fexcp->current_item)
	{
		return;
	}
					/* Update visuals of the items	*/

	if (fexcp->none_set == False)		/* Can only happen with
						 * with exclusives	*/
	{
		if (!(fexcp->current_item == fexcp->set_item &&
		      (old_current == (Cardinal) OL_NO_ITEM ||
		       old_current == fexcp->set_item)))
		{
					/* Turn off the set visual for
					 * the old current item		*/

			if (old_current != (Cardinal) OL_NO_ITEM)
			{
				_OlFlatRefreshItem(w, old_current, True);
			}
			else if (fexcp->set_item != (Cardinal) OL_NO_ITEM)
			{
				/* If the old current item is OL_NO_ITEM,
				 * turn off the set item		*/

				_OlFlatRefreshItem(w, fexcp->set_item, True);
			}

					/* Turn on the visual for the
					 * new current item.		*/

			if (fexcp->current_item != (Cardinal) OL_NO_ITEM)
			{
				_OlFlatRefreshItem(w, fexcp->current_item,
							False);
			}
			else if (fexcp->set_item != (Cardinal) OL_NO_ITEM)
			{
				/* If the new current item is
				 * OL_NO_ITEM, turn on the set item.	*/

				_OlFlatRefreshItem(w, fexcp->set_item, False);
			}
		}
	}
	else if (fexcp->exclusive_settings == True)
	{
		Cardinal	other_item;

		other_item = (old_current != (Cardinal) OL_NO_ITEM ?
				old_current :
			fexcp->set_item != (Cardinal) OL_NO_ITEM ?
				fexcp->set_item : fexcp->current_item);

		if (other_item != fexcp->current_item)
		{
			_OlFlatRefreshItem(w, other_item, True);
		}

		if (fexcp->current_item != (Cardinal) OL_NO_ITEM)
		{
			_OlFlatRefreshItem(w, fexcp->current_item,
				(Boolean) (fexcp->current_item ==
				fexcp->set_item ? True : False));
		}
		else if (other_item != fexcp->set_item &&
			 fexcp->set_item != (Cardinal) OL_NO_ITEM)
		{
			_OlFlatRefreshItem(w, fexcp->set_item, False);
		}
	}
	else				/* Non-exclusives type	*/
	{
		if (old_current != (Cardinal) OL_NO_ITEM)
		{
			_OlFlatRefreshItem(w, old_current, (Boolean)
				(IsSet(w, old_current) ? False : True));
		}

		if (fexcp->current_item != (Cardinal) OL_NO_ITEM)
		{
			_OlFlatRefreshItem(w, fexcp->current_item, (Boolean)
				(IsSet(w, fexcp->current_item) ? True : False));
		}
	}
} /* END OF SetToCurrent() */

/*
 *************************************************************************
 * UnsetCurrent - if there is a current item, this routine unsets it.
 ****************************procedure*header*****************************
 */
static void
UnsetCurrent (Widget w)
{
	FlatExclusivesPart *	fexcp = FEPART(w);
	Cardinal		old_current = fexcp->current_item;

	if (old_current == (Cardinal) OL_NO_ITEM)
	{
		return;
	}

	fexcp->current_item = (Cardinal) OL_NO_ITEM;

	if (fexcp->exclusive_settings == True)
	{
		if (old_current != fexcp->set_item)
		{
			_OlFlatRefreshItem(w, old_current, True);
		
			if (fexcp->set_item != (Cardinal) OL_NO_ITEM)
			{
				_OlFlatRefreshItem(w, fexcp->set_item, False);
			}
		}
		else
		{
			_OlFlatRefreshItem(w, old_current,
				(Boolean) (fexcp->none_set == False ?
						False : True));
		}
	}
	else {
		_OlFlatRefreshItem(w, old_current, (Boolean)
				(IsSet(w, old_current) ? False : True));
	}
} /* END OF UnsetCurrent() */

/*
 *************************************************************************
 *
 * Public Procedures
 *
 ****************************public*procedures****************************
 */

/* There are no public procedures */
