#pragma ident	"@(#)FButton.c	302.8	97/03/26 lib/libXol SMI"	/* flat:FButton.c 1.5	*/

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
 *	This file contains the source code for the flat compound button
 *	container.
 *
 ******************************file*header********************************
 */


#include <stdio.h>

#include <X11/IntrinsicP.h>
#include <X11/StringDefs.h>

#include <Xol/OpenLookP.h>
#include <Xol/FButtonP.h>
#include <Xol/Error.h>


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
#define FCPART(w)	(&((FlatWidgetClass)XtClass(w))->flat_class)
#define FBPART(w)	(&((FlatButtonWidget)(w))->button)
#define FCBCPART(w)	(&((FlatButtonWidgetClass)XtClass(w))->button_class)
#define FBIPART(i)	(&((FlatButtonItem)(i))->button)

#define CVT_JUSTIFY(j)	((j) == OL_LEFT ? TL_LEFT_JUSTIFY :\
		((j) == OL_CENTER ? TL_CENTER_JUSTIFY : TL_RIGHT_JUSTIFY))

			/* Define the resources for a sub-object that are
			 * interited from the flat widget container.	*/

#define OFFSET(base, field)	XtOffsetOf(base, button.item_part.field)

#define INHERITED_ITEM_RESOURCES(base)\
\
	{ XtNbuttonType, XtCButtonType, XtROlDefine, sizeof(OlDefine),\
	  OFFSET(base, button_type), XtRImmediate, (XtPointer)OL_OBLONG },\
\
	{ XtNclientData, XtCClientData, XtRPointer, sizeof(XtPointer),\
	  OFFSET(base, client_data), XtRPointer, (XtPointer) NULL },\
\
	{ XtNselectProc, XtCCallbackProc, XtRCallbackProc,\
	  sizeof(XtCallbackProc), OFFSET(base, select_proc),\
	  XtRCallbackProc, (XtPointer) NULL }
/* End of INHERITED_ITEM_RESOURCES macro definition */

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

static void	SetLabel (Widget w, FlatButtonItem item, OlgxAttrs **ppAttrs,
			  FlatLabel *lbl, OlSizeProc *sizeProc,
			  OlDrawProc *drawProc);

					/* class procedures		*/

static Boolean	ItemActivate (Widget, FlatItem, OlVirtualName,
					XtPointer);
static void	DrawItem (Widget, FlatItem, OlFlatDrawInfo *);
static void	Initialize (Widget, Widget, ArgList, Cardinal *);
static void	ItemDimensions (Widget, FlatItem,
					Dimension *, Dimension *);
static Boolean	SetValues (Widget,Widget,Widget,ArgList,Cardinal*);

					/* action procedures		*/

/* There are no action procedures */

					/* public procedures		*/

/* There are no public procedures */

/*
 *************************************************************************
 *
 ***********************widget*translations*actions***********************
 */

/*
 *************************************************************************
 *
 * Define Resource list associated with the Widget Instance
 *
 ****************************widget*resources*****************************
 */

static XtResource
resources[] = {
	{ XtNpostSelect, XtCCallback, XtRCallback, sizeof(XtCallbackList),
	  XtOffsetOf(FlatButtonRec, button.post_select), XtRCallback,
	  (XtPointer) NULL },

	INHERITED_ITEM_RESOURCES(FlatButtonRec)
};

	/* Specify resources that we want the flat class to manage
	 * internally if the application doesn't put them in their
	 * item fields list.						*/

static OlFlatReqRsc
required_resources[] = {
	{ XtNset, (OlFlatReqRscPredicateFunc)NULL },
	{ XtNbusy, (OlFlatReqRscPredicateFunc)NULL }
};

				/* Define Resources for sub-objects	*/

#undef OFFSET
#define OFFSET(base,field) XtOffsetOf(base,button.field)

static XtResource
item_resources[] = {
			/* Include resources that are not inherited
			 * from the flat widget container		*/

	{ XtNset, XtCSet, XtRBoolean, sizeof(Boolean),
	  OFFSET(FlatButtonItemRec, set), XtRBoolean, (XtPointer)&def_false },

	{ XtNbusy, XtCBusy, XtRBoolean, sizeof(Boolean),
	  OFFSET(FlatButtonItemRec, busy), XtRBoolean, (XtPointer)&def_false },

	INHERITED_ITEM_RESOURCES(FlatButtonItemRec)
};

/*
 *************************************************************************
 *
 * Define Class Record structure to be initialized at Compile time
 *
 ***************************widget*class*record***************************
 */

FlatButtonClassRec
flatButtonClassRec = {
    {
	(WidgetClass)&flatClassRec,		/* superclass		*/
	"FlatButton",				/* class_name		*/
	sizeof(FlatButtonRec),			/* widget_size		*/
	NULL,					/* class_initialize	*/
	NULL,					/* class_part_initialize*/
	FALSE,					/* class_inited		*/
	Initialize,				/* initialize		*/
	NULL,					/* initialize_hook	*/
	XtInheritRealize,			/* realize		*/
	NULL,					/* actions		*/
	(Cardinal)0,				/* num_actions		*/
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
	XtInheritTranslations,			/* tm_table		*/
	NULL,					/* query_geometry	*/
	NULL,					/* display_accelerator	*/
	NULL					/* extension		*/
    }, /* End of Core Class Part Initialization */
    {
	NULL,					/* reserved1		*/
	XtInheritHighlightHandler,		/* highlight_handler	*/
	XtInheritTraversalHandler,		/* traversl_handler	*/
	NULL,					/* register_focus	*/
	XtInheritActivateFunc,			/* activate		*/
	NULL,					/* event_procs		*/
	0,					/* num_event_procs	*/
	OlVersion,				/* version		*/
	(XtPointer)NULL,			/* extension		*/
	{
		(_OlDynResourceList)NULL,	/* resources		*/
		(Cardinal)0			/* num_resources	*/
	},					/* dyn_data		*/
	XtInheritTransparentProc		/* transparent_proc	*/
    },	/* End of Primitive Class Part Initialization */
    {
	item_resources,				/* item_resources	*/
	XtNumber(item_resources),		/* num_item_resources	*/
	required_resources,			/* required_resources	*/
	XtNumber(required_resources),		/* num_required_resources*/
	NULL,					/* quarked_items	*/
	(Cardinal)sizeof(FlatButtonItemPart),	/* part_size		*/
	XtOffset(FlatButtonWidget, button.item_part),/* part_offset	*/
	XtOffset(FlatButtonItem,button),	/* part_in_rec_offset	*/
	sizeof(FlatButtonItemRec),		/* rec_size		*/
	True,					/* transparent_bg	*/
	(OlFlatAnalyzeItemsProc)NULL,		/* analyze_items	*/
	DrawItem,				/* draw_item		*/
	(OlFlatExpandItemProc)NULL,		/* expand_item		*/
	XtInheritFlatGetDrawInfo,		/* get_draw_info	*/
	XtInheritFlatGetIndex,			/* get_index		*/
	XtInheritFlatItemAcceptFocus,		/* item_accept_focus	*/
	ItemActivate,				/* item_activate	*/
	ItemDimensions,				/* item_dimensions	*/
	(OlFlatItemGetValuesProc)NULL,		/* item_get_values	*/
	XtInheritFlatItemHighlight,		/* item_highlight	*/
	(OlFlatItemInitializeProc)NULL,		/* item_initialize	*/
	(OlFlatItemSetValuesFunc)NULL,		/* item_set_values	*/
	(OlFlatItemsTouchedProc)NULL,		/* items_touched	*/
	OlThisFlatClass,			/* layout_class		*/
	XtInheritFlatLayout,			/* layout		*/
	XtInheritFlatTraverseItems,		/* traverse_items	*/
	(XtPointer)NULL,			/* reserved2		*/
	(XtPointer)NULL,			/* reserved1		*/
	(XtPointer)NULL				/* extension		*/
    }, /* End of Flat Class Part Initialization */
    {
	NULL					/* no_class_fields	*/
    } /* End of FlatButton Class Part Initialization */
};

/*
 *************************************************************************
 *
 * Public Widget Class Definition of the Widget Class Record
 *
 *************************public*class*definition*************************
 */

WidgetClass flatButtonWidgetClass = (WidgetClass) &flatButtonClassRec;

/*
 *************************************************************************
 *
 * Private Procedures
 *
 ***************************private*procedures****************************
 */

/*
 *************************************************************************
 * SetLabel - populate the label structure and select the
 *	proper sizing and drawing functions.
 ****************************procedure*header*****************************
 */
static void
SetLabel (Widget w, FlatButtonItem item, OlgxAttrs **ppAttrs, FlatLabel *lbl, OlSizeProc *sizeProc, OlDrawProc *drawProc)
{
	FlatPart *	fp = FPART(w);
	FlatWidget	fw = (FlatWidget)w;
	int		has_focus = (item->flat.item_index == fp->focus_item);
	Cardinal	psize = fw->primitive.scale;

	*ppAttrs = fp->pAttrs;

	lbl->text.label		= item->flat.label;
	lbl->text.flags		= 0;
	lbl->text.font		= item->flat.font;
	lbl->text.text_format	= item->flat.text_format;
	lbl->text.justification	= CVT_JUSTIFY(item->flat.label_justify);
	lbl->text.mnemonic	= item->flat.mnemonic;
	lbl->text.qualifier	= (unsigned char *)item->flat.qualifier_text;
	lbl->text.meta		= item->flat.meta_key;
	lbl->text.accelerator	= (unsigned char *)item->flat.accelerator_text;

	if (has_focus)
	{
	    OlFlatScreenCache *	sc = _OlFlatScreenManager(w, psize,
						OL_JUST_LOOKING);
	    Pixel	bg;
	    Pixel	fg;
	    Pixel	ifc = item->flat.input_focus_color;

	    if (ifc == item->flat.font_color ||
		ifc == item->flat.background_pixel)
	    {
		    fg = item->flat.background_pixel;
		    bg = item->flat.foreground;
	    }
	    else
	    {
		    fg = item->flat.font_color;
		    bg = ifc;
	    }

	    if (sc->alt_attrs == (OlgxAttrs *)NULL ||
		sc->alt_bg != bg ||
		sc->alt_fg != fg)
	    {
		    if (sc->alt_attrs != (OlgxAttrs *)NULL)
		    {
			    OlgxDestroyAttrs(w, sc->alt_attrs);
		    }
		    sc->alt_bg = bg;
		    sc->alt_fg = fg;
		    sc->alt_attrs = OlgxCreateAttrs (w, sc->alt_fg,
				    (OlgxBG *) &(sc->alt_bg), False, psize,
				    item->flat.text_format, item->flat.font);
	    }
	    *ppAttrs = sc->alt_attrs;
	}

	if (sizeProc) {
		*sizeProc = OlgxSizeTextLabel;
	}
	if (drawProc) {
		*drawProc = OlgxDrawTextButton;
	}
} /* END OF SetLabel() */

/*
 *************************************************************************
 *
 * Class Procedures
 *
 ****************************class*procedures*****************************
 */

/*
 *************************************************************************
 * ItemActivate - this routine is used to activate this widget.
 ****************************procedure*header*****************************
 */
/* ARGSUSED */
static Boolean
ItemActivate (Widget w, FlatItem item, OlVirtualName type, XtPointer data)
{
	Boolean	ret_val = False;

	(void)printf("\"%s\"'s ItemActivate procedure not implemented\n",
			XtName(w));
	return(ret_val);
} /* END OF ItemActivate() */

/*
 *************************************************************************
 * DrawItem - this routine draws a single instance of a button
 * sub-object
 ****************************procedure*header*****************************
 */
static void
DrawItem(Widget w, FlatItem item, OlFlatDrawInfo *di)
	      			  		/* container widget id	*/
	        		     		/* expanded item	*/
	                	   		/* Drawing information	*/
{
	FlatLabel			lbl;
	OlDrawProc			drawProc;
	FlatButtonItem			b_item = (FlatButtonItem)item;
	int				flags = 0;
	OlgxAttrs *			attrs;

	SetLabel(w, b_item, &attrs, &lbl, NULL, &drawProc);

	if (b_item->button.set == True) {
		flags |= OLGX_INVOKED;
	}
	if ( !(b_item->flat.sensitive && b_item->flat.ancestor_sensitive) )
	    flags |= OLGX_INACTIVE;
	if (item->flat.text_format == OL_WC_STR_REP)
	    flags |= OLGX_LABEL_IS_WCS;

	(*drawProc)(di->screen, di->drawable, attrs,
		    di->x, di->y, di->width, 0,
		    (XtPointer)&lbl, OL_OBLONG, flags);
} /* END OF DrawItem() */

/*
 *************************************************************************
 * Initialize - this procedure initializes the instance part of the widget
 ****************************procedure*header*****************************
 */
static void
Initialize (Widget request, Widget new, ArgList args, Cardinal *num_args)
{
	(void) _OlFlatCheckItems(flatButtonWidgetClass, (Widget)NULL,
			request, new, args, num_args);
} /* END OF Initialize() */

/*
 *************************************************************************
 * ItemDimensions - this routine determines the size of a single
 * sub-object
 ****************************procedure*header*****************************
 */
static void
ItemDimensions (Widget w, FlatItem item, register Dimension *width, register Dimension *height)
{
	FlatLabel	      	lbl;
	OlSizeProc              sizeProc;
	OlgxAttrs *		attrs;

	SetLabel(w, (FlatButtonItem)item, &attrs, &lbl, &sizeProc, NULL);

	OlgxSizeOblongButton(XtScreen(w), attrs, (XtPointer)&lbl, sizeProc,
			     0, width, height);
} /* END OF ItemDimensions() */

/*
 *************************************************************************
 * SetValues - this procedure monitors the changing of instance data
 ****************************procedure*header*****************************
 */
static Boolean
SetValues (Widget current, Widget request, Widget new, ArgList args, Cardinal *num_args)
{
	Boolean	redisplay = False;

	if (_OlFlatCheckItems(flatButtonWidgetClass, current, request, new,
				args, num_args) == True)
	{
		redisplay = True;
	}

	return(redisplay);
} /* END OF SetValues() */

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
