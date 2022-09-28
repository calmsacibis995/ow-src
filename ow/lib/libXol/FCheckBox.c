#pragma ident	"@(#)FCheckBox.c	302.12	97/03/26 lib/libXol SMI"	/* flat:FCheckBox.c 1.14.1.16	*/

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


#include <X11/IntrinsicP.h>
#include <X11/StringDefs.h>

#include <Xol/Error.h>
#include <Xol/FCheckBoxP.h>
#include <Xol/Font.h>
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

					/* class procedures		*/

static void     QueryFCBSuperCaretLocn(const   Widget          flat,
                                       const   Widget          supercaret,
                                       const   Dimension       width,
                                       const   Dimension       height,
                                       Cardinal        *const  scale,
                                       SuperCaretShape *const  shape,
                                       Position        *const  x_center,
                                       Position        *const  y_center
                );
static void	DrawItem(Widget w, FlatItem item, OlFlatDrawInfo *di);		/* Draws a single item		*/
static Cardinal	GetIndex (Widget, Position, Position, Boolean);
static void	Initialize(Widget request, Widget new, ArgList args, Cardinal *num_args);		/* Initializes instance		*/
static void	ItemDimensions(Widget w, FlatItem item, register Dimension *width, register Dimension *height);	/* determines sub-object's size	*/
static Boolean	ItemSetValues(Widget w, FlatItem current, FlatItem request, FlatItem new, ArgList args, Cardinal *num_args);	/* sub-object data monitoring	*/
static Boolean	SetValues(Widget current, Widget request, Widget new, ArgList args, Cardinal *num_args);		/* instance data monitoring	*/


/*
 *************************************************************************
 *
 * Define global/static variables and #defines, and
 * Declare externally referenced variables
 *
 *****************************file*variables******************************
 */

#define	PADDING		4	/* point size padding for default font	*/

#define W_CNAME(w)	(XtClass(w)->core_class.class_name)

#define FPART(w) (&((FlatWidget)(w))->flat)
#define FCPART(w) (&((FlatWidgetClass)XtClass(w))->flat_class)
#define FEPART(w)  (&((FlatExclusivesWidget)(w))->exclusives)
#define FECPART(w) (&((FlatExclusivesWidgetClass)XtClass(w))->exclusives_class)
#define FCBPART(w)  (&((FlatCheckBoxWidget)(w))->checkbox)
#define FCBCPART(w) (&((FlatCheckBoxWidgetClass)XtClass(w))->checkbox_class)
#define FEIPART(i)  (&((FlatExclusivesItem)(i))->exclusives)

				/* Define the resources for a sub-object*/

#define OFFSET(base, field)	XtOffsetOf(base, checkbox.item_part.field)

#define ITEM_RESOURCES(base)\
\
	{ XtNposition, XtCPosition, XtROlDefine, sizeof(OlDefine),\
	  OFFSET(base,position), XtRImmediate,\
	  (XtPointer) ((OlDefine) OL_LEFT) }
/* End of ITEM_RESOURCES macro definition */

/*
 *************************************************************************
 *
 * Define Translations and Actions
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
	{ XtNexclusives, XtCExclusives, XtRBoolean, sizeof(Boolean),
	  XtOffsetOf(FlatCheckBoxRec, exclusives.exclusive_settings),
	  XtRImmediate, (XtPointer) False },

	ITEM_RESOURCES(FlatCheckBoxRec)
};

				/* Define Resources for sub-objects	*/

#undef OFFSET
#define OFFSET(base,field) XtOffsetOf(base,checkbox.field)

static XtResource
item_resources[] = {
	ITEM_RESOURCES(FlatCheckBoxItemRec)
};

/*
 *************************************************************************
 *
 * Define Class Record structure to be initialized at Compile time
 *
 ***************************widget*class*record***************************
 */

FlatCheckBoxClassRec
flatCheckBoxClassRec = {
    {
	(WidgetClass)&flatExclusivesClassRec,	/* superclass		*/
	"FlatCheckBox",				/* class_name		*/
	sizeof(FlatCheckBoxRec),		/* widget_size		*/
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
	XtInheritTransparentProc,		/* transparent_proc	*/
        QueryFCBSuperCaretLocn,                 /* query_sc_locn_proc   */
    },	/* End of Primitive Class Part Initialization */
    {
	item_resources,				/* item_resources	*/
	XtNumber(item_resources),		/* num_item_resources	*/
	NULL,					/* required_resources	*/
	(Cardinal)0,				/* num_required_resources*/
	NULL,					/* quarked_items	*/
	(Cardinal)sizeof(FlatCheckBoxItemPart),	/* part_size		*/
	XtOffset(FlatCheckBoxWidget, checkbox.item_part),/* part_offset	*/
	XtOffset(FlatCheckBoxItem,checkbox),	/* part_in_rec_offset	*/
	sizeof(FlatCheckBoxItemRec),		/* rec_size		*/
	True,					/* transparent_bg	*/
	(OlFlatAnalyzeItemsProc)NULL,		/* analyze_items	*/
	DrawItem,				/* draw_item		*/
	(OlFlatExpandItemProc)NULL,		/* expand_item		*/
	XtInheritFlatGetDrawInfo,		/* get_draw_info	*/
	GetIndex,				/* get_index		*/
	XtInheritFlatItemAcceptFocus,		/* item_accept_focus	*/
	XtInheritFlatItemActivate,		/* item_activate	*/
	ItemDimensions,				/* item_dimensions	*/
	(OlFlatItemGetValuesProc)NULL,		/* item_get_values	*/
	XtInheritFlatItemHighlight,		/* item_highlight	*/
	(OlFlatItemInitializeProc)NULL,		/* item_initialize	*/
	ItemSetValues,				/* item_set_values	*/
	(OlFlatItemsTouchedProc)NULL,		/* items_touched	*/
	OlThisFlatClass,			/* layout_class		*/
	XtInheritFlatLayout,			/* layout		*/
	XtInheritFlatTraverseItems,		/* traverse_items	*/
	(XtPointer)NULL,			/* reserved2		*/
	(XtPointer)NULL,			/* reserved1		*/
	(XtPointer)NULL				/* extension		*/
    }, /* End of Flat Class Part Initialization */
    {
	False					/* allow_class_default	*/
    }, /* End of FlatExclusives Class Part Initialization */
    {
	NULL					/* no_class_fields	*/
    } /* End of FlatCheckBox Class Part Initialization */
};

/*
 *************************************************************************
 *
 * Public Widget Class Definition of the Widget Class Record
 *
 *************************public*class*definition*************************
 */

WidgetClass flatCheckBoxWidgetClass =
			(WidgetClass) &flatCheckBoxClassRec;

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
 * Class Procedures
 *
 ****************************class*procedures*****************************
 */

/*
 *************************************************************************
 * DrawItem - this routine draws a single instance of a checkbox
 * sub-object
 ****************************procedure*header*****************************
 */
static void
DrawItem(Widget w, FlatItem item, OlFlatDrawInfo *di)
	      			  		/* container widget id	*/
	        		     		/* expanded item	*/
	                	   		/* Drawing information	*/
{
	FlatPart *			fp = FPART(w);
	FlatExclusivesPart *		fexcp = FEPART(w);
	Dimension			padding;
	Dimension			label_width;
	int				int_x, check_y;
	FlatCheckBoxItem		cb_item = (FlatCheckBoxItem)item;
	FlatExclusivesItemPart *	item_part = &cb_item->exclusives;
	OlgxAttrs *			item_attrs;
	FlatLabel			*lbl;
	OlDrawProc			drawProc;
	int				flags, cb_flags;
	unsigned long			save_fc = NULL;
	OlFlatScreenCache *	sc = _OlFlatScreenManager(w,
#ifdef sun
					((FlatWidget)w)->primitive.scale,
#else
					OL_DEFAULT_POINT_SIZE, 
#endif
					OL_JUST_LOOKING);

	if (item->flat.mapped_when_managed == False)
	{
		return;
	}

	/* Get attrs and label information */
	_OlFlatSetupAttributes (w, item, di, &item_attrs, &lbl, &drawProc);

				/* Now put in the label or the image if
				 * it's to the left of the check box.	*/

	padding	= sc->check_width + 
			OlgxScreenPointToPixel(OL_HORIZONTAL, 1, item_attrs->scr) 
		             * PADDING;
	label_width	= di->width - padding;
	int_x		= (int) di->x;

	flags = cb_flags = 0;
	if (item->flat.sensitive == (Boolean)False ||
	    XtIsSensitive(w) == (Boolean)False || fexcp->dim == (Boolean)True)
	{
	    flags |= OLGX_INACTIVE;
	    cb_flags |= OLGX_INACTIVE;
	}
	if (item->flat.text_format == OL_WC_STR_REP)
	    flags |= OLGX_LABEL_IS_WCS;

	if (cb_item->checkbox.position == (OlDefine)OL_LEFT)
	{
		(*drawProc) (di->screen, di->drawable, fp->pAttrs,
			     int_x, di->y, label_width, di->height, lbl,
			     OL_LABEL, flags);
		int_x = (int)di->x + (int) (di->width - sc->check_width);
	}

	if ((item->flat.item_index == fexcp->current_item &&
					item_part->set == False) ||
	    (fexcp->current_item != item->flat.item_index &&
					item_part->set == True))
	{
	    cb_flags = OLGX_CHECKED;
	}

	check_y = di->y + (int)(di->height - sc->check_height) / 2;


	if (item->flat.foreground != item->flat.font_color) {
	    save_fc = olgx_get_single_color(item_attrs->ginfo, OLGX_BLACK);
	    olgx_set_single_color(item_attrs->ginfo, OLGX_BLACK,
		                  (unsigned long)item->flat.foreground, 
				  OLGX_SPECIAL);
	}
	olgx_draw_check_box(item_attrs->ginfo, di->drawable, int_x, check_y, 
			    cb_flags);
	if (save_fc)
	    olgx_set_single_color(item_attrs->ginfo, OLGX_BLACK, save_fc, 
		                  OLGX_SPECIAL);

				/* Now put in the label or the image if
				 * it's to the right of the check box.	*/

	if (cb_item->checkbox.position != (OlDefine)OL_LEFT)
	{
		int_x += (int)padding;
		(*drawProc) (di->screen, di->drawable, fp->pAttrs,
			     int_x, di->y, label_width, di->height, lbl,
			     OL_LABEL, flags);
	}

} /* END OF DrawItem() */

/*
 *************************************************************************
 * GetIndex - this routine returns the index of an item when given an
 * X and Y coordinate pair. 
 ****************************procedure*header*****************************
 */
static Cardinal
GetIndex (Widget w, Position x, Position y, Boolean ignore_sensitivity)
{
	FlatPart	  *	fp = FPART(w);
	OlFlatScreenCache *	sc = _OlFlatScreenManager(w,
#ifdef sun
					((FlatWidget)w)->primitive.scale,
#else
					OL_DEFAULT_POINT_SIZE, 
#endif
					OL_JUST_LOOKING);
	Cardinal	item_index;

			/* Use the Flat Class procedure from the to
			 * tell us which item the x,y pair is over.	*/

	item_index = (* flatClassRec.flat_class.get_index)(w, x, y,
					ignore_sensitivity);

	if (item_index != (Cardinal) OL_NO_ITEM)
	{
		OlFlatDrawInfo	di;
		Dimension	margin;
		OlDefine	position;
		Arg		args[1];

		_OlFlatGetDrawInfo(w, item_index, &di);

		margin = (Dimension)(di.height - sc->check_height) /
						(Dimension)2;

		XtSetArg(args[0], XtNposition, &position);
		OlFlatGetValues(w, item_index, args, 1);

		if (y <= (di.y + (Position) margin) ||
		    y > (di.y + (Position)(margin + sc->check_height)))
		{
			item_index = (Cardinal)OL_NO_ITEM;
		}
		else if (position == (OlDefine)OL_LEFT &&
			 x <= (di.x + (Position)
				((Position) di.width - sc->check_width)))
		{
			item_index = (Cardinal) OL_NO_ITEM;
		}
		else if (position == (OlDefine)OL_RIGHT &&
			 x > (di.x + (Position) sc->check_width))
		{
			item_index = (Cardinal) OL_NO_ITEM;
		}
	}
	return(item_index);
} /* END OF GetIndex() */

/*
 *************************************************************************
 * Initialize - this procedure initializes the instance part of the widget
 ****************************procedure*header*****************************
 */
static void
Initialize(Widget request, Widget new, ArgList args, Cardinal *num_args)
	      	        	/* What the application requested	*/
	      	    		/* What the application gets, so far...	*/
	       		     
	          	         
{
	(void) _OlFlatCheckItems(flatCheckBoxWidgetClass, (Widget)NULL,
			request, new, args, num_args);
} /* END OF Initialize() */

/*
 *************************************************************************
 * ItemDimensions - this routine determines the size of a single sub-object
 ****************************procedure*header*****************************
 */
static void
ItemDimensions(Widget w, FlatItem item, register Dimension *width, register Dimension *height)
	      			  	/* Widget making request	*/
	        		     	/* expanded item		*/
	                    	      	/* returned width		*/
	                    	       	/* returned height		*/
{
	FlatPart *		fp = FPART(w);
	OlSizeProc		sizeProc;
	FlatLabel		*lbl;
	OlFlatScreenCache *	sc = _OlFlatScreenManager(w,
#ifdef sun
					((FlatWidget)w)->primitive.scale,
#else
					OL_DEFAULT_POINT_SIZE, 
#endif
					OL_JUST_LOOKING);

	/* Get label size */
	_OlFlatSetupLabelSize (w, item, &lbl, &sizeProc);

	(*sizeProc) (XtScreen (w), fp->pAttrs, lbl, width, height);

					/* Add the horizontal padding.	*/

	*width	+= OlgxScreenPointToPixel(OL_HORIZONTAL, 1, fp->pAttrs->scr) *
			PADDING + sc->check_width;
		
			/* If the checkbox is taller, use its height	*/

	if (*height < sc->check_height)
	{
		*height = sc->check_height;
	}

} /* END OF ItemDimensions() */

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
	FlatCheckBoxItem	ncb = (FlatCheckBoxItem)new;
	FlatCheckBoxItem	ccb = (FlatCheckBoxItem)current;

	return((Boolean)(ncb->checkbox.position != ccb->checkbox.position ?
				True : False));
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
	Boolean				redisplay = False;
	FlatCheckBoxPart *		fchbp = FCBPART(new);
	FlatCheckBoxPart *		cfchbp = FCBPART(current);

	if (fchbp->item_part.position != cfchbp->item_part.position)
	{
		redisplay = True;
	}

	if (_OlFlatCheckItems(flatCheckBoxWidgetClass, current, request, new,
				args, num_args) == True)
	{
		redisplay = True;
	}

	return(redisplay);
} /* END OF SetValues() */

/*
 *************************************************************************
 * QueryFCBSuperCaretLocn: handles the positioning of the supercaret
 * over the current flat focus item.
 ****************************procedure*header*****************************
 */
#define ALLOC_STACK_ITEM(fcptr,iptr,stk)				  \
        unsigned char	stk[512];					  \
        FlatItem       	iptr = (FlatItem)(fcptr->rec_size > sizeof(stk) ? \
                                (FlatItem)XtMalloc(fcptr->rec_size)     : \
				(FlatItem)stk)
#define FREE_STACK_ITEM(iptr,stk) \
	if ((unsigned char *)iptr != stk) XtFree((char *)iptr)

static void
QueryFCBSuperCaretLocn(const   Widget          flat,
                       const   Widget          supercaret,
                       const   Dimension       width,
                       const   Dimension       height,
                       Cardinal        *const  scale,
                       SuperCaretShape *const  shape,
                       Position        *const  x_center,
                       Position        *const  y_center
                )
{
        FlatWidget              fw  = (FlatWidget)flat;
        FlatPart      *const    fp  = FPART(flat);
        FlatClassPart *const    fcp = FCPART(flat);
        OlFlatDrawInfo          fidi;
	ALLOC_STACK_ITEM	(fcp, item, stack_item);
	FlatExclusivesPart*	fexcp   = FEPART(flat);
	FlatCheckBoxItem	cb_item = (FlatCheckBoxItem)item;
	SuperCaretShape		rs = *shape;
	OlFlatScreenCache*	sc;
	Position		chx = (Position)0,
				chy;

        if (fp->focus_item == OL_NO_ITEM) {
                *shape = SuperCaretNone;
                return;
        }
 
	switch (fp->layout_type) {
		case OL_FIXEDROWS:
		case OL_ROWS:
			*shape = SuperCaretBottom;
			break;
		case OL_FIXEDCOLS:
		case OL_COLUMNS:
			*shape = SuperCaretLeft;
			break;
	}
 
        if (fw->primitive.scale != *scale || rs != *shape) {
                *scale = fw->primitive.scale;
		FREE_STACK_ITEM(item, stack_item);
                return; /* try again */
        }
 
	sc  = _OlFlatScreenManager(flat ,fw->primitive.scale, OL_JUST_LOOKING);

        _OlFlatGetDrawInfo(flat, fp->focus_item, &fidi);
 
	*x_center = fidi.x;
	*y_center = fidi.y;

	_OlFlatExpandItem(flat, fp->focus_item, item);

	if (cb_item->checkbox.position == (OlDefine)OL_LEFT)
		chx = fidi.width - sc->check_width;

	chy = fidi.height - (sc->check_height / 2);

	*x_center += chx;
	*y_center += chy;

        switch (*shape) {
                case SuperCaretBottom:
                        *x_center += sc->check_width / 2;
                        *y_center += (sc->check_height / 2) + (height / 6);
                        break;
 
                case SuperCaretLeft:
			*x_center -= width / 6;
                break;
        }

	FREE_STACK_ITEM(item, stack_item);
}

#undef	ALLOC_STACK_ITEM
#undef	FREE_STACK_ITEM

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
