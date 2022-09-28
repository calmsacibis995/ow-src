#pragma ident	"@(#)Flat.c	302.19	97/03/26 lib/libXol SMI"	/* flat:Flat.c 1.39	*/

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
 *	This file contains the source code for the flat widget class.
 *	The flat widget class is not a class that's intended for
 *	instantiation; rather, it serves as a managing class for its
 *	instantiated subclasses.
 *
 ******************************file*header********************************
 */


#include <stdio.h>
#include <string.h>
#include <libintl.h>	

#include <X11/IntrinsicP.h>
#include <X11/StringDefs.h>

#include <Xol/Error.h>
#include <Xol/FlatP.h>
#include <Xol/OpenLookP.h>
#include <Xol/OlStrMthdsI.h>


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

				/* Define a structure for holding widths
				   and heights, needed for function
				   prototype */

typedef struct {
	Dimension	width;
	Dimension	height;
} W_and_H;

static void	CalcOffsets(FlatPart *fp, Dimension width, Dimension height);		/* Calculates bndng box offsets	*/
static void	CheckFont (Widget, FlatPart *, FlatItem);
static void	CheckLayoutParameters(Widget current, FlatPart *cfp, Widget new, FlatPart *fp);/* checks validity layout params*/
static void	ClassErrorMsg(WidgetClass wc, char *req_proc);	/* Generates Class Error messages*/
static void	DestroyAcceleratorStrings (FlatPart *);
static Cardinal	GetFocusItems (FlatPart *, Cardinal, int,
			Cardinal *, Cardinal **, Cardinal *, int);
static void	GetGCs (Widget, FlatPart *);
static Boolean	SetFocusToAnyItem (Widget, Time);
static Boolean	SetAccAndMnem (Widget, FlatItem, FlatItem);
static void	SetupRequiredResources (WidgetClass, FlatClassPart *,
						FlatClassPart *);
static void 	UpdateSuperCaret(FlatWidget fw);
static void	WarningMsg(Widget w, String error_type, String resource, char *value);		/* generates a warning message	*/

					/* class procedures		*/

static Boolean	AcceptFocus (Widget widget, Time *time);
static Boolean	ActivateWidget (Widget, OlVirtualName, XtPointer);
static void	AnalyzeItems (Widget, ArgList, Cardinal *);
static Dimension CalcItemSize(FlatPart *fp, W_and_H *w_h, int rule, int flag, Cardinal i, Cardinal rows, Cardinal cols);	/* Calculates item dimensions	*/
static void	ClassInitialize (void);
static void	ClassPartInitialize (WidgetClass);
static void	Destroy (Widget);
static void	DrawItem (Widget, FlatItem, OlFlatDrawInfo *);
static void	ExpandItem (Widget, FlatItem);
static void	GetDrawInfo (Widget, Cardinal, OlFlatDrawInfo *);
static Cardinal	GetIndex (Widget, Position, Position, Boolean);
static void	HighlightHandler (Widget, OlDefine);
static void	Initialize (Widget, Widget, ArgList, Cardinal *);
static Boolean	ItemAcceptFocus (Widget, FlatItem, Time);
static void	ItemHighlight (Widget, FlatItem, OlDefine);
static void	ItemInitialize (Widget, FlatItem, FlatItem,
						ArgList, Cardinal *);
static void	ItemDimensions (Widget, FlatItem, Dimension *,
						Dimension *);
static void	ItemsTouched (Widget, Widget, Widget,
					ArgList, Cardinal *);
static void	Layout(Widget w, Dimension *return_width, Dimension *return_height);		/* Determines container size	*/
static void	Redisplay(Widget w, XEvent *xevent, Region region);		/* Generic redisplay		*/
static void	Resize(Widget w);		/* relayouts after resize	*/
static Boolean	ItemSetValues(Widget w, FlatItem current, FlatItem request, FlatItem new, ArgList args, Cardinal *num_args);	/* for setting item state	*/
static Boolean	SetValues(Widget current, Widget request, Widget new, ArgList args, Cardinal *num_args);		/* instance data monitoring	*/
static void	TransparentProc (Widget, Pixel, Pixmap);
static Widget	TraversalHandler (Widget,Widget,OlVirtualName,Time);
static Cardinal	TraverseItems (Widget,Cardinal,OlVirtualName,Time);

static void     ItemQuerySCLocnProc(const   Widget          flat,
                                        const   Widget          supercaret,
                                        const   Dimension       width,
                                        const   Dimension       height,
                                        Cardinal        *const  scale,
                                        SuperCaretShape *const  shape,
                                        Position        *const  x_center,
                                        Position        *const  y_center
                );

					/* action procedures		*/

/* There are no action procedures */

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
#define PWIDGET(w, f)	(((PrimitiveWidget)w)->primitive.f)
#define PCLASS(wc, f)	(((PrimitiveWidgetClass)wc)->primitive_class.f)
#define CNAME(wc)	((wc)->core_class.class_name)
#define W_CNAME(w)	CNAME(XtClass(w))

#define ALLOC_STACK_ITEM(fcptr,iptr,stk)\
	auto char	stk[512];\
	FlatItem	iptr = (FlatItem)(fcptr->rec_size>sizeof(stk)?\
				XtMalloc(fcptr->rec_size) : stk)
#define FREE_STACK_ITEM(iptr,stk) if ((char*)iptr!=stk){XtFree((char *)iptr);}

#define CALL_TRANSPARENT_PROC(w)\
	{\
		WidgetClass wc = XtClass(w);\
		if (PCLASS(wc, transparent_proc) != (OlTransparentProc)NULL)\
		{\
			(*PCLASS(wc, transparent_proc))(w,\
				XtParent(w)->core.background_pixel,\
				XtParent(w)->core.background_pixmap);\
		}\
	}

#define DEF_TYPE(f,i)	(f->item_resources[i].default_type)

#define	LOCAL_SIZE	50

#define INDEX_IS_CACHED(d)	((d) != (XtPointer)NULL)
#define INDEX_TO_DATA(i)	((XtPointer)((i)+1))
#define DATA_TO_INDEX(i)	((Cardinal)((Cardinal)(i)-1))

#define GET_FC(w)	(((FlatWidget)(w))->primitive.font_color)

static const char	dup_key[] = "Duplicate Key Binding";
static const char	bad_key[] = "Invalid Key Binding";
static const char	unknown_key[] = "????";

					/* Define constants for width
					 * and height calculations	*/

#define ALL_ITEMS	0		/* every item			*/
#define COLUMN_ITEMS	1		/* All items in a column	*/
#define ROW_ITEMS	2		/* All items in a row		*/

#define DEFAULT_DIMENSION	4	/* in points		*/
#define	WIDTH			0
#define	HEIGHT			1

					/* Define some handy macros	*/

#define FPART(w)	(&((FlatWidget)(w))->flat)
#define FCPART(w)	(&((FlatWidgetClass)XtClass(w))->flat_class)

			/* Define a macro for the sub-object resources	*/

#define OFFSET(base,field)	XtOffsetOf(base,flat.item_part.field)

				/* Define the sub-object resources that
				 * are common to the both the container
				 * and the sub-objects.			*/

/*
 * The default value for backgroundPixmap should have been XtUnspecifiedPixmap,
 * but since that doesn't work well in static shared library, the
 * constant 2 is used. This assumes that XtUnspecifiedPixmap is 2,
 * which is true for Intrinsic Rel 3.0.
 */
#define INHERITED_ITEM_RESOURCES(base)\
\
	{XtNfont, XtCFont, XtROlFont, sizeof(OlFont), \
	  OFFSET(base, font), XtRString, (XtPointer)XtDefaultFont}, \
\
	{ XtNforeground, XtCForeground, XtRPixel, sizeof(Pixel),\
	  OFFSET(base, foreground), XtRString,\
	  (XtPointer) XtDefaultForeground },\
\
	{ XtNfontColor, XtCFontColor, XtRPixel, sizeof(Pixel),\
	  OFFSET(base, font_color), XtRString,\
	  (XtPointer) XtDefaultForeground },\
\
	{ XtNlabel, XtCLabel, XtROlStr, sizeof(OlStr),\
	  OFFSET(base, label), XtRImmediate, (XtPointer) NULL },\
\
	{ XtNlabelImage, XtCLabelImage, XtRPointer, sizeof(XImage *),\
	  OFFSET(base, label_image), XtRPointer, (XtPointer) NULL },\
\
	{ XtNlabelJustify, XtCLabelJustify, XtROlDefine, sizeof(OlDefine),\
	  OFFSET(base, label_justify), XtRImmediate,\
	  (XtPointer) OL_LEFT },\
\
	{ XtNlabelTile, XtCLabelTile, XtRBoolean, sizeof(Boolean),\
	  OFFSET(base, label_tile), XtRImmediate, (XtPointer) False },\
\
	{ XtNinputFocusColor, XtCInputFocusColor, XtRPixel,\
	  sizeof(Pixel), OFFSET(base, input_focus_color), XtRString,\
	  (XtPointer) XtDefaultForeground },\
\
	{ XtNbackground, XtCBackground, XtRPixel, sizeof(Pixel),\
	  OFFSET(base, background_pixel), XtRString,\
	  (XtPointer) XtDefaultBackground },\
\
	{ XtNbackgroundPixmap, XtCPixmap, XtRPixmap, sizeof(Pixmap),\
	  OFFSET(base, background_pixmap), XtRImmediate,\
	  (XtPointer) 2},\
\
	{ XtNtraversalOn, XtCTraversalOn, XtRBoolean, sizeof(Boolean),\
	  OFFSET(base, traversal_on), XtRImmediate, (XtPointer)True },\
\
	{ XtNuserData, XtCUserData, XtRPointer, sizeof(XtPointer),\
	  OFFSET(base, user_data), XtRPointer, (XtPointer) NULL }
/* End of INHERITED_ITEM_RESOURCES macro definition */

/*
 *************************************************************************
 *
 * Define Translations and Actions
 *
 ***********************widget*translations*actions***********************
 */

/* There are no translations or actions */

/*
 *************************************************************************
 *
 * Define Resource list associated with the Widget Instance
 *
 ****************************widget*resources*****************************
 */

			/* Define the resources for the container.
			 * Normally, the sub-object resources are
			 * included as part of the container's
			 * resources, but since all of the FlatItem
			 * resources already have corresponding
			 * fields in the container, we'll use those.	*/

static XtResource
resources[] = {

	{ XtNborderWidth, XtCBorderWidth, XtRDimension, sizeof(Dimension),
	  XtOffsetOf(FlatRec, core.border_width), XtRImmediate,
	  (XtPointer) 0 },

					/* Define layout resources	*/

	{ XtNgravity, XtCGravity, XtRGravity, sizeof(int),
	  XtOffsetOf(FlatRec, flat.gravity), XtRImmediate,
	  (XtPointer) ((int)CenterGravity) },

	{ XtNhPad, XtCHPad, XtRDimension, sizeof(Dimension),
	  XtOffsetOf(FlatRec, flat.h_pad), XtRImmediate,
	  (XtPointer) ((Dimension) 0)},

	{ XtNhSpace, XtCHSpace, XtRDimension, sizeof(Dimension),
	  XtOffsetOf(FlatRec, flat.h_space), XtRImmediate,
	  (XtPointer) ((Dimension) 0)},

	{ XtNitemGravity, XtCItemGravity, XtRGravity, sizeof(int),
	  XtOffsetOf(FlatRec, flat.item_gravity), XtRImmediate,
	  (XtPointer) ((int) NorthWestGravity) },

	{ XtNitemMaxHeight, XtCItemMaxHeight, XtRDimension, sizeof(Dimension),
	  XtOffsetOf(FlatRec, flat.item_max_height), XtRImmediate,
	  (XtPointer) ((Dimension) OL_IGNORE) },

	{ XtNitemMaxWidth, XtCItemMaxWidth, XtRDimension, sizeof(Dimension),
	  XtOffsetOf(FlatRec, flat.item_max_width), XtRImmediate,
	  (XtPointer) ((Dimension) OL_IGNORE) },

	{ XtNitemMinHeight, XtCItemMinHeight, XtRDimension, sizeof(Dimension),
	  XtOffsetOf(FlatRec, flat.item_min_height), XtRImmediate,
	  (XtPointer) ((Dimension) OL_IGNORE) },

	{ XtNitemMinWidth, XtCItemMinWidth, XtRDimension, sizeof(Dimension),
	  XtOffsetOf(FlatRec, flat.item_min_width), XtRImmediate,
	  (XtPointer) ((Dimension) OL_IGNORE) },

	{ XtNlayoutHeight, XtCLayoutHeight, XtROlDefine, sizeof(OlDefine),
	  XtOffsetOf(FlatRec, flat.layout_height), XtRImmediate,
	  (XtPointer) ((OlDefine) OL_MINIMIZE) },

	{ XtNlayoutType, XtCLayoutType, XtROlDefine, sizeof(OlDefine),
	  XtOffsetOf(FlatRec, flat.layout_type), XtRImmediate,
	  (XtPointer) ((OlDefine) OL_FIXEDROWS) },

	{ XtNlayoutWidth, XtCLayoutWidth, XtROlDefine, sizeof(OlDefine),
	  XtOffsetOf(FlatRec, flat.layout_width), XtRImmediate,
	  (XtPointer) ((OlDefine) OL_MINIMIZE) },

	{ XtNmeasure, XtCMeasure, XtRInt, sizeof(int),
	  XtOffsetOf(FlatRec, flat.measure), XtRImmediate,
	  (XtPointer) ((int)1) },

	{ XtNsameHeight, XtCSameHeight, XtROlDefine, sizeof(OlDefine),
	  XtOffsetOf(FlatRec, flat.same_height), XtRImmediate,
	  (XtPointer) ((OlDefine) OL_ALL) },

	{ XtNsameWidth, XtCSameWidth, XtROlDefine, sizeof(OlDefine),
	  XtOffsetOf(FlatRec, flat.same_width), XtRImmediate,
	  (XtPointer) ((OlDefine) OL_COLUMNS) },

	{ XtNvPad, XtCVPad, XtRDimension, sizeof(Dimension),
	  XtOffsetOf(FlatRec, flat.v_pad), XtRImmediate,
	  (XtPointer) ((Dimension) 0) },

	{ XtNvSpace, XtCVSpace, XtRDimension, sizeof(Dimension),
	  XtOffsetOf(FlatRec, flat.v_space), XtRImmediate,
	  (XtPointer) ((Dimension) 4) },

				/* Define other resources.
				 * Note, the order of XtNitems, XtNnumItems
				 * XtNitemFields and XtNnumItemFields
				 * is significant.			*/

	{ XtNnumItems, XtCNumItems, XtRCardinal, sizeof(Cardinal),
	  XtOffsetOf(FlatRec, flat.num_items), XtRImmediate,
	  (XtPointer) OL_IGNORE },

	{ XtNnumItemFields, XtCNumItemFields, XtRCardinal, sizeof(Cardinal),
	  XtOffsetOf(FlatRec, flat.num_item_fields), XtRImmediate,
	  (XtPointer) OL_IGNORE },

	{ XtNitemFields, XtCItemFields, OlRFlatItemFields, sizeof(String *),
	  XtOffsetOf(FlatRec, flat.item_fields), OlRFlatItemFields,
	  (XtPointer) NULL },

	{ XtNitems, XtCItems, OlRFlatItems, sizeof(XtPointer),
	  XtOffsetOf(FlatRec, flat.items), OlRFlatItems, (XtPointer) NULL },

	{ XtNitemsTouched, XtCItemsTouched, XtRBoolean, sizeof(Boolean),
	  XtOffsetOf(FlatRec, flat.items_touched), XtRImmediate,
	  (XtPointer) False },

				/* Include sub-object resources that
				 * are inherited from the container.	*/

	INHERITED_ITEM_RESOURCES(FlatRec)
};

				/* Define Resources for sub-objects	*/

#undef OFFSET
#define OFFSET(base,field)	XtOffsetOf(base, flat.field)

static XtResource
item_resources[] = {
	INHERITED_ITEM_RESOURCES(FlatItemRec),

#undef OFFSET
#define OFFSET(f)	XtOffsetOf(FlatItemRec, flat.f)

				/* Add resources that are not inherited
				 * from the container.			*/

	{ XtNmanaged, XtCManaged, XtRBoolean, sizeof(Boolean),
	  OFFSET(managed), XtRImmediate, (XtPointer) True },

	{ XtNmappedWhenManaged, XtCMappedWhenManaged, XtRBoolean,
	  sizeof(Boolean), OFFSET(mapped_when_managed),
	  XtRImmediate, (XtPointer) True },

	{ XtNsensitive, XtCSensitive, XtRBoolean, sizeof(Boolean),
	  OFFSET(sensitive), XtRImmediate, (XtPointer) True },

	{ XtNancestorSensitive, XtCSensitive, XtRBoolean, sizeof(Boolean),
	  OFFSET(ancestor_sensitive), XtRImmediate, (XtPointer) True },

	{ XtNaccelerator, XtCAccelerator, XtRString, sizeof(String),
	  OFFSET(accelerator), XtRString, (XtPointer) NULL },

	{ XtNacceleratorText, XtCAcceleratorText, XtRString, sizeof(String),
	  OFFSET(accelerator_text), XtRString, (XtPointer) NULL },

	{ XtNmnemonic, XtCMnemonic, OlRChar, sizeof(char),
	  OFFSET(mnemonic), XtRImmediate, (XtPointer) '\0' }
};

/*
 *************************************************************************
 *
 * Define Class Record structure to be initialized at Compile time
 *
 ***************************widget*class*record***************************
 */

FlatClassRec
flatClassRec = {
    {
	(WidgetClass)&primitiveClassRec,	/* superclass		*/
	"Flat",					/* class_name		*/
	sizeof(FlatRec),			/* widget_size		*/
	ClassInitialize,			/* class_initialize	*/
	ClassPartInitialize,			/* class_part_initialize*/
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
	Destroy,				/* destroy		*/
	Resize,					/* resize		*/
	Redisplay,				/* expose		*/
	SetValues,				/* set_values		*/
	NULL,					/* set_values_hook	*/
	XtInheritSetValuesAlmost,		/* set_values_almost	*/
	NULL,					/* get_values_hook	*/
	AcceptFocus,				/* accept_focus		*/
	XtVersion,				/* version		*/
	NULL,					/* callback_offsets	*/
	XtInheritTranslations,			/* tm_table		*/
	NULL,					/* query_geometry	*/
	NULL,					/* display_accelerator	*/
	NULL					/* extension		*/
    }, /* End of Core Class Part Initialization */
    {
	NULL,					/* reserved1		*/
	HighlightHandler,			/* highlight_handler	*/
	TraversalHandler,			/* traversal_handler	*/
	NULL,					/* register_focus	*/
	ActivateWidget,				/* activate		*/
	NULL,					/* event_procs		*/
	0,					/* num_event_procs	*/
	OlVersion,				/* version		*/
	NULL,					/* extension		*/
	{
		(_OlDynResourceList)NULL,	/* resources		*/
		(Cardinal)0			/* num_resources	*/
	},					/* dyn_data		*/
	TransparentProc,			/* transparent_proc	*/
        ItemQuerySCLocnProc,                /* query_sc_locn_proc   */
    },	/* End of Primitive Class Part Initialization */
    {
	item_resources,				/* item_resources	*/
	XtNumber(item_resources),		/* num_item_resources	*/
	NULL,					/* required_resources	*/
	(Cardinal)0,				/* num_required_resources*/
	NULL,					/* quarked_items	*/
	sizeof(FlatItemPart),			/* part_size		*/
	XtOffsetOf(FlatRec,flat.item_part),	/* part_offset		*/
	XtOffsetOf(FlatItemRec,flat),		/* part_in_rec_offset	*/
	sizeof(FlatItemRec),			/* rec_size		*/
	False,					/* transparent_bg	*/
	AnalyzeItems,				/* analyze_items	*/
	DrawItem,				/* draw_item		*/
	ExpandItem,				/* expand_item		*/
	GetDrawInfo,				/* get_draw_info	*/
	GetIndex,				/* get_index		*/
	ItemAcceptFocus,			/* item_accept_focus	*/
	(OlFlatItemActivateFunc)NULL,		/* item_activate	*/
	ItemDimensions,				/* item_dimensions	*/
	(OlFlatItemGetValuesProc)NULL,		/* item_get_values	*/
	ItemHighlight,				/* item_highlight	*/
	ItemInitialize,				/* item_initialize	*/
	ItemSetValues,				/* item_set_values	*/
	ItemsTouched,				/* items_touched	*/
	OlThisFlatClass,			/* layout_class		*/
	Layout,					/* layout		*/
	TraverseItems,				/* traverse_items	*/
	NULL,					/* reserved2		*/
	NULL,					/* reserved1		*/
	(XtPointer)NULL				/* extension		*/
    } /* End of Flat Class Part Initialization */
};

/*
 *************************************************************************
 *
 * Public Widget Class Definition of the Widget Class Record
 *
 *************************public*class*definition*************************
 */

WidgetClass flatWidgetClass	= (WidgetClass) &flatClassRec;

/*
 *************************************************************************
 *
 * Private Procedures
 *
 ***************************private*procedures****************************
 */

static void
UpdateSuperCaret(FlatWidget fw)
{
        PrimitivePart   *pp = &(fw->primitive);
        OlVendorPartExtension ext_part = pp->ext_part;

	if(pp->has_focus == FALSE)
		return;
        if(ext_part == (OlVendorPartExtension)NULL ||
            ext_part->supercaret == (Widget)NULL    ||
            pp->input_focus_feedback != OL_SUPERCARET)
                return;

        _OlCallUpdateSuperCaret(ext_part->supercaret, (Widget)fw);
}

/*
 *************************************************************************
 * CalcOffsets - this procedure calculates the x and y offsets of the
 * first sub-object.  This calculation is done by taking the bounding
 * box dimension and adding the horizontal and vertical paddings to it.
 * After calling the gravity procedure, the returned offsets are adjusted
 * to compensate for the previously-added paddings.
 ****************************procedure*header*****************************
 */
static void
CalcOffsets(FlatPart *fp, Dimension width, Dimension height)
{
	_OlDoGravity(fp->gravity, width, height,
			(fp->item_geom.bounding_width + (2 * fp->h_pad)),
			(fp->item_geom.bounding_height + (2 * fp->v_pad)),
			&fp->item_geom.x_offset, &fp->item_geom.y_offset);

			/* Adjust the offset so that it is the upper left
			 * had corner of the first item.		*/

	fp->item_geom.x_offset += fp->h_pad;
	fp->item_geom.y_offset += fp->v_pad;
} /* END OF CalcOffsets() */

/*
 *************************************************************************
 * CheckFont -
 ****************************procedure*header*****************************
 */
static void
CheckFont (Widget w, FlatPart *fp, FlatItem item)
{
	if (item->flat.font == (OlFont)NULL)
	{
		item->flat.font = fp->item_part.font;

		OlVaDisplayWarningMsg(XtDisplay(w),
			OleNinvalidResource, OleTflatState,
			OleCOlToolkitWarning,
			OleMinvalidResource_flatState,
			XtName(w), W_CNAME(w), XtNfont,
			(unsigned)item->flat.item_index,
			"container's font");
	}
} /* END OF CheckFont() */

/*
 *************************************************************************
 * CheckLayoutParameters - this routine checks the validity of various
 * layout parameters.  If any of the parameters are invalid, a warning
 * is generated and the values are set to a valid value. 
 ****************************procedure*header*****************************
 */
static void
CheckLayoutParameters(Widget current, FlatPart *cfp, Widget new, FlatPart *fp)
	      		        	/* Current widget id or NULL	*/
	          	    		/* Current Flat Part or NULL	*/
	      		    		/* New widget id		*/
	          	   		/* New Flat Part		*/
{
	String	error_type = (current == (Widget)NULL ?
				OleTinitialize : OleTsetValues);

			/* Define a macro to speed things up (typing that is)
			 * and make sure that there are no spaces after
			 * the commas when this is used.		*/

#define CHANGED(field)	(cfp == (FlatPart *)NULL || (cfp->field != fp->field))

#ifdef __STDC__
#define CLEANUP(field, r, value) \
	WarningMsg(new,error_type,#r,#value); fp->field = value;
#else
#define CLEANUP(field, r, value) \
	WarningMsg(new,error_type,"r","value"); fp->field = value;
#endif

	if (CHANGED(gravity))
	{
		switch (fp->gravity)
		{
		case EastGravity:			/* fall through	*/
		case WestGravity:			/* fall through	*/
		case CenterGravity:			/* fall through	*/
		case NorthGravity:			/* fall through	*/
		case NorthEastGravity:			/* fall through	*/
		case NorthWestGravity:			/* fall through	*/
		case SouthGravity:			/* fall through	*/
		case SouthEastGravity:			/* fall through	*/
		case SouthWestGravity:
			break;				/* Do Nothing	*/
		default:
			CLEANUP(gravity, XtNgravity, CenterGravity)
			break;
		}
	}

	if (CHANGED(item_gravity))
	{
		switch (fp->item_gravity)
		{
		case EastGravity:			/* fall through	*/
		case WestGravity:			/* fall through	*/
		case CenterGravity:			/* fall through	*/
		case NorthGravity:			/* fall through	*/
		case NorthEastGravity:			/* fall through	*/
		case NorthWestGravity:			/* fall through	*/
		case SouthGravity:			/* fall through	*/
		case SouthEastGravity:			/* fall through	*/
		case SouthWestGravity:
			break;				/* Do Nothing	*/
		default:
			CLEANUP(item_gravity,XtNitemGravity,NorthWestGravity)
			break;
		}
	}

	if (CHANGED(layout_height) &&
	    (fp->layout_height != (OlDefine)OL_IGNORE &&
	     fp->layout_height != (OlDefine)OL_MAXIMIZE &&
	     fp->layout_height != (OlDefine)OL_MINIMIZE))
	{
		CLEANUP(layout_height,XtNlayoutHeight,OL_MINIMIZE)
	}

	if (CHANGED(layout_type) &&
	    (fp->layout_type != (OlDefine)OL_FIXEDCOLS &&
	     fp->layout_type != (OlDefine)OL_FIXEDROWS))
	{
		CLEANUP(layout_type,XtNlayoutType,OL_FIXEDROWS)
	}

	if (CHANGED(layout_width) &&
	    (fp->layout_width != (OlDefine)OL_IGNORE &&
	     fp->layout_width != (OlDefine)OL_MAXIMIZE &&
	     fp->layout_width != (OlDefine)OL_MINIMIZE))
	{
		CLEANUP(layout_width,XtNlayoutWidth,OL_MINIMIZE)
	}

	if (CHANGED(measure) && fp->measure < 1)
	{
		CLEANUP(measure,XtNmeasure,1)
	}

	if (CHANGED(same_width) &&
	    (fp->same_width != (OlDefine)OL_NONE &&
	     fp->same_width != (OlDefine)OL_COLUMNS &&
	     fp->same_width != (OlDefine)OL_ALL))
	{
		CLEANUP(same_width,XtNsameWidth,OL_COLUMNS)
	}

	if (CHANGED(same_height) &&
	    (fp->same_height != (OlDefine)OL_NONE &&
	     fp->same_height != (OlDefine)OL_ROWS &&
	     fp->same_height != (OlDefine)OL_ALL))
	{
		CLEANUP(same_height,XtNsameHeight,OL_ALL)
	}

#undef CLEANUP
#undef CHANGED
} /* END OF CheckLayoutParameters() */

/*
 *************************************************************************
 * ClassErrorMsg - this routine generates a warning message from the
 * specified arguments.  This routine is called by checks being made in
 * the ClassPartInitialize procedure.
 ****************************procedure*header*****************************
 */
static void
ClassErrorMsg(WidgetClass wc, char *req_proc)
	           	   		/* The delinquent class		*/
	      		         	/* name of missing procedure	*/
{
	OlVaDisplayErrorMsg((Display *)NULL, OleNinvalidProcedure,
			OleTinheritanceProc, OleCOlToolkitError,
			OleMinvalidProcedure_inheritanceProc,
			CNAME(wc), req_proc);
} /* END OF ClassErrorMsg() */

/*
 *************************************************************************
 * DestroyAcceleratorStrings -
 ****************************procedure*header*****************************
 */
static void
DestroyAcceleratorStrings (FlatPart *fp)
{
	if (fp->acc_text != (struct _acc_text *)NULL)
	{
		Cardinal	i;

		for (i=0; i < fp->num_items; ++i)
		{
			XtFree((char *)(fp->acc_text[i].qual));
			XtFree((char *)(fp->acc_text[i].acc));
		}
		XtFree((char *)fp->acc_text);
		fp->acc_text = (struct _acc_text *)NULL;
	}
} /* END OF DestroyAcceleratorStrings() */

/*
 *************************************************************************
 * GetFocusItems - this routine initializes an array with the item
 * indecies of all items in the current row or column.  If the supplied
 * array is too small, a new one is allocated.
 * This routine also returns the focus_item's index relative to the
 * returned array.
 ****************************procedure*header*****************************
 */
static Cardinal
GetFocusItems (FlatPart *fp, Cardinal focus_item, int do_row, Cardinal *num_ret, Cardinal **array_ret, Cardinal *array, int array_size)
{
	Cardinal	rows = fp->item_geom.rows;
	Cardinal	cols = fp->item_geom.cols;
	Cardinal	r;			/* row of focus item	*/
	Cardinal	c;			/* column of focus item	*/
	Cardinal	i;
	Cardinal	start_index;
	Cardinal	item_index;
	Cardinal *	roci;			/* row or column items	*/
	Cardinal	adder;
	Cardinal	max_num;	/* max number of focus items	*/

				/* IF 'do_row' is true calculate the
				 * row that the focus_item is in
				 * and fill in the array; else
				 * calculate the column that the focus_item
				 * item is in and fill in the array.	*/

	if (fp->layout_type == OL_FIXEDCOLS)
	{
		r = focus_item/cols;
		c = focus_item - (r * cols);

		if (do_row) {
			adder		= 1;
			item_index	= r * cols;
		} else {
			adder		= cols;
			item_index	= c;
		}
	}
	else		/* OL_FIXEDROWS	*/
	{
		c = focus_item/rows;
		r = focus_item - (c * rows);

		if (do_row) {
			adder		= rows;
			item_index	= r;
		} else {
			adder		= 1;
			item_index	= c * rows;
		}
	}

	max_num = (do_row ? cols : rows);

	if (max_num > (Cardinal)array_size) {
		roci = (Cardinal *)XtMalloc(max_num * sizeof(Cardinal));
	} else {
		roci = array;
	}

	for (i=0, *num_ret=0; i < max_num && item_index < fp->num_items; ++i)
	{
		if (item_index == focus_item) {
			start_index = i;
		}
		roci[i] = item_index;
		item_index += adder;
		++(*num_ret);		/* count true number of items	*/
	}

	*array_ret = roci;
	return(start_index);
} /* END OF GetFocusItems() */

/*
 *************************************************************************
 * GetGCs - this routine creates the OlgxAttrs for the flat exclusives
 * container.  If it previously existed, it is deleted first.
 ****************************procedure*header*****************************
 */
static void
GetGCs (Widget w, FlatPart *fp)
{
	FlatWidget	fw = (FlatWidget)w;

	if (fp->pAttrs != (OlgxAttrs *) NULL)
	{
		OlgxDestroyAttrs (w, fp->pAttrs);
	}
	fp->pAttrs = OlgxCreateAttrs (w, GET_FC(w),
				     (OlgxBG *)&(fp->item_part.background_pixel),
					False, 
#ifdef sun
					fw->primitive.scale,
#else
					OL_DEFAULT_POINT_SIZE,
#endif
				      fp->item_part.text_format,
				      fp->item_part.font);
} /* END OF GetGCs()  */

/*
 *************************************************************************
 * SetFocusToAnyItem - sets focus to the first item willing to take it.
 * TRUE is returned if an item took focus, FALSE is returned otherwise.
 ****************************procedure*header*****************************
 */
static Boolean
SetFocusToAnyItem (Widget w, Time time)
{
	Boolean		ret_val = False;
	Cardinal	i;
	ALLOC_STACK_ITEM(FCPART(w), item, stack);

	for (i = 0; i < FPART(w)->num_items; ++i)
	{
		_OlFlatExpandItem(w, i, item);

		if (_OlFlatItemAcceptFocus(w, item, time) == True)
		{
			ret_val = True;
			break;
		}
	}
	FREE_STACK_ITEM(item,stack);
	return(ret_val);
} /* END OF SetFocusToAnyItem() */

/*
 *************************************************************************
 * SetAccAndMnem - This routine registers a new mnemonic and/or
 * accelerator for a sub-object.  If one previously exists, it's removed
 * first.  The routine returns a flag indicating whether a refresh is
 * necessary.
 ****************************procedure*header*****************************
 */
static Boolean
SetAccAndMnem (Widget w, FlatItem current, FlatItem new)
{
	FlatPart *	fp		= FPART(w);
	Boolean		redisplay	= False;
	Cardinal	i		= new->flat.item_index;
	XtPointer	data		= INDEX_TO_DATA(i);
	int		internal_list	= (fp->flags &
						OL_B_FLAT_ACCELERATOR_TEXT);
	OlDefine	status;

	if (current != (FlatItem)NULL &&
	    current->flat.mnemonic != new->flat.mnemonic &&
	    current->flat.mnemonic != '\0')
	{
		redisplay = True;
		_OlRemoveMnemonic(w, data, False, current->flat.mnemonic);
	}

	if (new->flat.mnemonic != '\0'
			&&
	    (current == (FlatItem)NULL ||
		current->flat.mnemonic != new->flat.mnemonic))
	{
		redisplay = True;

		if ((status = _OlAddMnemonic(w, data, new->flat.mnemonic))
					!= OL_SUCCESS)
		{
			OlVaDisplayWarningMsg(XtDisplay(w),"","",
				OleCOlToolkitWarning,
				"Widget \"%s\" (class %s): failed to add\
 mnemonic \"%c\" to item %d: cause = \"%s\"", XtName(w), W_CNAME(w),
				new->flat.mnemonic, (int)i,
				(status == OL_DUPLICATE_KEY ? dup_key :
				 (status == OL_BAD_KEY ? bad_key :
				 unknown_key)));
			new->flat.mnemonic = '\0';
		}
	}

			/* Unregister the old accelerator and free its
			 * string (provided we're internally managing it).
			 */

	if (current != (FlatItem)NULL &&
	    current->flat.accelerator != new->flat.accelerator)
	{
			/* If we're managing the accelerator text
			 * internally, free the current string.		*/

		if (internal_list)
		{
			XtFree((char *) fp->acc_text[i].qual);
			XtFree((char *) fp->acc_text[i].acc);
			fp->acc_text[i].qual = (String)NULL;
			fp->acc_text[i].meta = False;
			fp->acc_text[i].acc = (String)NULL;
			current->flat.qualifier_text = (String)NULL;
			current->flat.meta_key = False;
			current->flat.accelerator_text = (String)NULL;
		}
		_OlRemoveAccelerator(w, data, False, current->flat.accelerator);
		redisplay = True;
	}

	if (new->flat.accelerator != (String)NULL
			&&
	    (current == (FlatItem)NULL ||
		(current->flat.accelerator != new->flat.accelerator)))
	{
		redisplay = True;

		status = _OlAddAccelerator(w, data, new->flat.accelerator);

		if (status == OL_SUCCESS)
		{
			if (internal_list)
			{
				_OlMakeAcceleratorText(w, new->flat.accelerator,
				    &fp->acc_text[i].qual,
				    &fp->acc_text[i].meta,
				    &fp->acc_text[i].acc);
				new->flat.qualifier_text = fp->acc_text[i].qual;
				new->flat.meta_key = fp->acc_text[i].meta;
				new->flat.accelerator_text = 
				    fp->acc_text[i].acc;
			}
		}
		else
		{
			if (internal_list)
			{
				fp->acc_text[i].qual = (String)NULL;
				fp->acc_text[i].meta = False;
				fp->acc_text[i].acc = (String)NULL;
			}

			new->flat.qualifier_text = (String)NULL;
			new->flat.meta_key = False;
			new->flat.accelerator_text = (String)NULL;

			OlVaDisplayWarningMsg(XtDisplay(w),"","",
				OleCOlToolkitWarning,
				"Widget \"%s\" (class %s): failed to add\
 accelerator \"%s\" to item %d: cause = \"%s\"", XtName(w), W_CNAME(w),
				new->flat.accelerator, (int)i,
				(status == OL_DUPLICATE_KEY ? dup_key :
				 (status == OL_BAD_KEY ? bad_key :
						unknown_key)));
		}
	}

	return(redisplay);
} /* END OF SetAccAndMnem() */

/*
 *************************************************************************
 * SetupRequiredResources - this routine sets up the list used to specify
 * resources that should be set on each sub-object.  If the application
 * supplies an item fields list that does not contain any of these
 * resources, the flat superclass maintains them internally.  This
 * features permits flattened widget writers from worrying about tracking
 * resources that the application did not set in its list.  A typical
 * example is the XtNset or XtNcurrent.
 ****************************procedure*header*****************************
 */
static void
SetupRequiredResources (WidgetClass wc, FlatClassPart *fcp, FlatClassPart *sfcp)
{
		/* Append this subclass's required resources to those
		 * required by the superclasses.  If the
		 * subclass does not have a list, the superclass's list
		 * information is copied into the subclass.		*/

	if (fcp->num_required_resources != (Cardinal)0 &&
	    fcp->required_resources == (OlFlatReqRscList)NULL)
	{
		OlVaDisplayErrorMsg((Display *)NULL,"invalidRequiredResource",
			OleTnullList, OleCOlToolkitError,
			"WidgetClass \"%s\": has null %s resource field with\
 non-NULL %s field", CNAME(wc), "required_resources", "num_required_resources");
	}
	else if (fcp->num_required_resources == (Cardinal)0)
	{
		if (sfcp != (FlatClassPart *)NULL)
		{
			fcp->required_resources     = sfcp->required_resources;
			fcp->num_required_resources =
						sfcp->num_required_resources;
		}
	}
	else
	{
		Cardinal	super_num = (sfcp ?
					sfcp->num_required_resources : 0);
		Cardinal	num = fcp->num_required_resources + super_num;
		OlFlatReqRsc *	req_rsc;
		XrmQuarkList	qlist;
		Cardinal	i;
		Cardinal	j;
		XrmQuark	quark;
		OlFlatReqRsc *	new_list;
		Boolean		copied_into_super_list = False;

		new_list = (OlFlatReqRsc *)XtMalloc(num * sizeof(OlFlatReqRsc));

				/* copy superclass's list into new list	*/

		if (super_num != (Cardinal)0)
		{
			(void)memcpy((char *)new_list,
				(const char *) sfcp->required_resources,
				(int) (super_num * sizeof(OlFlatReqRsc)));
		}

			/* Loop over subclass's req. resources and if a
			 * resource is not in the superclass's req. list,
			 * include it the new list.			*/

		for (i=0, num = super_num,
		     req_rsc = fcp->required_resources,
		     qlist = fcp->quarked_items;
		     i < fcp->num_required_resources;
		     ++i, ++req_rsc)
		{
			quark = XrmStringToQuark(req_rsc->name);

			for (j=0; j < super_num; ++j)
			{
				/* If the quark signatures equal, then
				 * a superclass has already specified this
				 * resource as a required one, so copy the
				 * subclass' required_resource structure
				 * into the array since the subclass may
				 * have changed one of the fields.	*/

				if (quark == qlist[new_list[j].rsc_index])
				{
					if (memcmp((const char *)(new_list+j),
						(const char *)req_rsc,
						(int)sizeof(OlFlatReqRsc)))
					{
						req_rsc->rsc_index =
							new_list[j].rsc_index;

						copied_into_super_list = True;

						(void)memcpy(
						   (char *)(new_list+j),
						   (const char *)req_rsc,
						   (int)sizeof(OlFlatReqRsc));
					}
					continue;
				}
			}

				/* Check to see if this resource is
				 * a valid one.				*/

			for (j=0; j < fcp->num_item_resources; ++j)
			{
			    if (quark == qlist[j])
			    {
				char *	default_type = (char *)
					  fcp->item_resources[j].default_type;

				if (default_type != (char *)NULL &&
				    strcmp(XtRImmediate, default_type)!=0)
				{
					new_list[num] = *req_rsc;
					new_list[num].rsc_index = j;
					++num;
				}
				else
				{
					OlVaDisplayErrorMsg((Display *)NULL,
						"invalidRequiredResource",
						"badDefaultType",
						OleCOlToolkitError,
 "WidgetClass \"%s\": required resource \"%s\" cannot have NULL or XtRImmediate\
 default_type", CNAME(wc), req_rsc->name);
				}
				break;
			    }
			}

			if (j == fcp->num_item_resources)
			{
				OlVaDisplayErrorMsg((Display *)NULL,
					"invalidRequiredResource",
					"badValue", OleCOlToolkitError,
					"WidgetClass \"%s\": resource \"%s\"\
 is not a valid sub-object resource", CNAME(wc), req_rsc->name);
			}
		}

			/* Now that the two lists have been merged,
			 * free any extra memory.			*/

		if (num < (fcp->num_required_resources + super_num))
		{
			/* If the number of required_resources in the
			 * subclass equal the number of resources in the
			 * superclass, then if the subclass has not
			 * modified any of the superclass' resources,
			 * free the new_list and let the subclass use the
			 * same list as the superclass.			*/

			if (num == super_num &&
			    copied_into_super_list == (Boolean)False)
			{
				XtFree((char *)new_list);
				new_list = (OlFlatReqRsc *)(sfcp ?
					sfcp->required_resources : NULL);
			}
			else
			{
				new_list = (OlFlatReqRsc *)XtRealloc(
						(char *)new_list,
						num * sizeof(OlFlatReqRsc));
			}
		}

				/* Cache information in the subclass.	*/

		fcp->num_required_resources	= num;
		fcp->required_resources		= new_list;
	}
} /* END OF SetupRequiredResources() */

/*
 *************************************************************************
 * WarningMsg - this routine generates a warning message from the
 * specified arguments.
 ****************************procedure*header*****************************
 */
static void
WarningMsg(Widget w, String error_type, String resource, char *value)
	      	  		/* The culprit				*/
	      	           	/* Procedure name used in message	*/
	      	         	/* name of the resource			*/
	      	      		/* valid value (text form)		*/
{
	char *	instance_name;		/* Widget's instance name	*/
	char *	message;

	if (!strcmp(error_type, OleTinitialize))
	{
		message = OleMinvalidResource_initialize;
	}
	else
	{
		message = OleMinvalidResource_setValues;
	}

				/* Get the name from the quark since
				 * superclasses of the WindowObjClass
				 * do not have an instance name field	*/

	instance_name = XrmQuarkToString(w->core.xrm_name);

	OlVaDisplayWarningMsg(XtDisplay(w),
		OleNinvalidResource, error_type, OleCOlToolkitWarning,
		message, instance_name, W_CNAME(w), resource, value);
} /* END OF WarningMsg() */

/*
 *************************************************************************
 *
 * Class Procedures
 *
 ****************************class*procedures*****************************
 */

/*
 *************************************************************************
 * AcceptFocus - this routine checks to see if focus can be set to this
 * flat Widget.  This routine does not actually set the focus.  It instead
 * redirects the request to the sub-object procedure.
 ****************************procedure*header*****************************
 */
static Boolean
AcceptFocus (Widget w, Time *time)
{
	Boolean	ret_val = False;

	if (OlCanAcceptFocus(w, *time) == True)
	{
		FlatPart *		fp = FPART(w);
		FlatClassPart *		fcp = FCPART(w);
		ALLOC_STACK_ITEM(fcp, item, stack);
		extern XtPointer	Ol_mnemonic_data;
		Cardinal		i;

			/* If the Ol_mnemonic_data pointer is
			 * non-NULL (set in Action.c), then it points to
			 * a casted value that is the item to activate.
			 * Else, find the first sub-object willing to
			 * take focus.
			 */
		if (INDEX_IS_CACHED(Ol_mnemonic_data))
		{
			i = DATA_TO_INDEX(Ol_mnemonic_data);
			if (i < fp->num_items)
			{
				_OlFlatExpandItem(w, i, item);
				ret_val = _OlFlatItemAcceptFocus(w, item, *time);
			}
		}
		else
		{
			if ((i = fp->last_focus_item) != (Cardinal)OL_NO_ITEM)
			{
				_OlFlatExpandItem(w, i, item);
				ret_val = _OlFlatItemAcceptFocus(w, item, *time);
			}

			if (ret_val == False)
			{
				ret_val = SetFocusToAnyItem(w, *time);
			}
		}
		FREE_STACK_ITEM(item,stack);
	}
	return(ret_val);
} /* END OF AcceptFocus() */

/*
 *************************************************************************
 * ActivateWidget - this generic routine is used to activate sub-objects.
 * If the data field is non-NULL, then cast the data to a Cardinal since
 * it is the item index.  (This occurs when a sub-object is activated
 * via a mnemonic or an accelerator.)  If the data field is NULL, then
 * activate the focus item.
 * Once the object to be activated has been found, the subclass's
 * item activation procedure is called.
 ****************************procedure*header*****************************
 */
static Boolean
ActivateWidget (Widget w, OlVirtualName type, XtPointer data)
{
	FlatPart *		fp = FPART(w);
	Boolean			ret_val = False;
	Cardinal		item_index;

	if (data == (XtPointer)NULL)
	{
		item_index = fp->focus_item;
	}
	else
	{
		item_index = DATA_TO_INDEX(data);

		if (item_index >= fp->num_items)
		{
			item_index = (Cardinal)OL_NO_ITEM;
		}
	}

	if (item_index != (Cardinal)OL_NO_ITEM)
	{
		FlatClassPart *	fcp = FCPART(w);
		ALLOC_STACK_ITEM(fcp, item, stack);

		_OlFlatExpandItem(w, item_index, item);
		ret_val = _OlFlatItemActivate(w, item, type, data);

		FREE_STACK_ITEM(item,stack);
	}
	else
	{
		if (type == OL_SELECTKEY)
		{
			ret_val = TRUE;
			(void)OlCallAcceptFocus(w, CurrentTime);
		}
	}
	return(ret_val);
} /* END OF ActivateWidget() */

/*
 *************************************************************************
 * AnalyzeItems - this routine analyzes new items for this class.
 * This routine is called after all of the items have been initialized,
 * so we can assume that all items have valid values.  If this widget
 * is the focus widget, then the application probably touched the item
 * list, so let's pick an item to take focus.
 ****************************procedure*header*****************************
 */
/* ARGSUSED */
static void
AnalyzeItems (Widget w, ArgList args, Cardinal *num_args)
{
	if (PWIDGET(w, has_focus) == True)
	{
		(void) SetFocusToAnyItem(w, CurrentTime);
	}
} /* END OF AnalyzeItems() */

/*
 *************************************************************************
 * CalcItemSize - this routine calculates the height of one or more items.
 * the type of calculation depends on the rule passed in.
 ****************************procedure*header*****************************
 */
static Dimension
CalcItemSize(FlatPart *fp, W_and_H *w_h, int rule, int flag, Cardinal i, Cardinal rows, Cardinal cols)
	           	   		/* Flat Part address		*/
	       		      		/* Array of widths & heights	*/
	   		     		/* Type of desired calculation	*/
	   		     		/* WIDTH or HEIGHT		*/
	        	  		/* item number			*/
	        	     		/* number of rows in layout	*/
	        	     		/* number of columns in layout	*/
{
	Dimension	max;		/* maximum width or height	*/

	switch(rule)
	{
	case ALL_ITEMS:
		{
			Dimension tmp;	/* temporary calculation holder	*/

			for(i=0, max=0; i < fp->num_items; ++i)
			{
				tmp = (flag == WIDTH ?
						w_h[i].width : w_h[i].height);
				if (tmp > max)
					max = tmp;
			}
		}
		break;
	case COLUMN_ITEMS:

		if (flag == HEIGHT)
		{
			OlError(dgettext(OlMsgsDomain,
				"CalcItemSize: Rule COLUMN_ITEMS cannot\
 be called for height calculations"));
		}

			 /* if fixed columns, then row-major order	*/

		if (fp->layout_type == OL_FIXEDCOLS)
		{
			Cardinal r;			/* current row	*/

			for (max=0, r=0; r < rows && i < fp->num_items;
			     ++r, i += cols)
			{
				if (w_h[i].width > max)
					max = w_h[i].width;
			}
		}
		else {			/* fixed rows is column-major	*/
			Cardinal r;			/* current row	*/

			for (max=0, r=0, i *= rows;
			     r < rows && i < fp->num_items; ++r, ++i)
			{
				if (w_h[i].width > max)
					max = w_h[i].width;
			}
		}

		break;
	case ROW_ITEMS:

		if (flag == WIDTH)
		{
			OlError(dgettext(OlMsgsDomain,
				"CalcItemSize: Rule ROW_ITEMS cannot\
 be called for width calculations"));
		}

			/* This rule says find the maximum height
			 * for all items in a particular row.
			 * OL_FIXEDROWS implies column-major order;
			 * if fixed columns, then row-major order	*/

		if (fp->layout_type == OL_FIXEDCOLS)
		{
			Cardinal c;		/* current column	*/

			for (max=0, c=0, i *= cols;
			     c < cols && i < fp->num_items; ++c, ++i)
			{
				if (w_h[i].height > max)
					max = w_h[i].height;
			}
		}
		else {			/* fixed rows is column-major	*/
			Cardinal c;		/* current column	*/

			for (max=0, c=0; c < cols && i < fp->num_items;
			     ++c, i += rows)
			{
				if (w_h[i].height > max)
					max = w_h[i].height;
			}
		}
		break;
	default:
		OlError(dgettext(OlMsgsDomain, "CalcItemSize: unknown rule"));
		break;
	}

	return(max);
} /* END OF CalcItemSize() */

/*
 *************************************************************************
 * ClassInitialize - this routine initializes the Flat widgets by
 * registering some converters
 ****************************procedure*header*****************************
 */
static void
ClassInitialize(void)
{
	_OlFlatAddConverters();

			/* Define a macro to aid in the registering of
			 * OlDefine types.				*/

#ifdef __STDC__
#define REGISTER_OLDEFINE(d)	\
	_OlAddOlDefineType((String)#d,(OlDefine)d)
#else
#define REGISTER_OLDEFINE(d)	\
	_OlAddOlDefineType((String)"d",(OlDefine)d)
#endif

	REGISTER_OLDEFINE(OL_MAXIMIZE);
	REGISTER_OLDEFINE(OL_MINIMIZE);
	REGISTER_OLDEFINE(OL_IGNORE);
	REGISTER_OLDEFINE(OL_FIXEDROWS);
	REGISTER_OLDEFINE(OL_FIXEDCOLS);
	REGISTER_OLDEFINE(OL_ALL);
	REGISTER_OLDEFINE(OL_NONE);
	REGISTER_OLDEFINE(OL_ROWS);
	REGISTER_OLDEFINE(OL_COLUMNS);
	REGISTER_OLDEFINE(OL_LEFT);
	REGISTER_OLDEFINE(OL_RIGHT);
	REGISTER_OLDEFINE(OL_CENTER);

#undef REGISTER_OLDEFINE

} /* END OF ClassInitialize() */

/*
 *************************************************************************
 * ClassPartInitialize - this routine initializes the widget's class
 * part field.  It Quarkifies the classes item's resource names and puts
 * them into a quark list.
 ****************************procedure*header*****************************
 */
static void
ClassPartInitialize(WidgetClass wc)
	               			/* Widget Class thats a subclass
					 * of FlatWidgetClass		*/
{
	FlatClassPart * fcp;		/* this class's Flat Class Part	*/
	FlatClassPart * sfcp;		/* superclass's Flat Class Part	*/

	fcp = &(((FlatWidgetClass) wc)->flat_class);

			/* If this is the flatWidgetClass, quark its
			 * item resources and return.			*/

	if (wc == flatWidgetClass)
	{
		Cardinal	i;
		XrmQuarkList	qlist;
		XtResourceList	rsc = fcp->item_resources;

		qlist = (XrmQuarkList) XtMalloc((Cardinal)
			(sizeof(XrmQuark) * fcp->num_item_resources));
		fcp->quarked_items = qlist;

		for (i = 0; i < fcp->num_item_resources; ++i, ++qlist, ++rsc)
		{
		    *qlist  = XrmStringToQuark(rsc->resource_name);
		}

		fcp->layout_class = wc;

		SetupRequiredResources(wc, fcp, (FlatClassPart *)NULL);
		return;
	}

				/* Get the superclasses flat class part	*/

	sfcp = &(((FlatWidgetClass) wc->core_class.superclass)->flat_class);

			/* Now, set up the item resources for this class.
			 * The item resources for this class will be merged
			 * with those of the superclass.  If the subclass
			 * and superclass item resource have the same offset,
			 * the subclasses item resource will be used.	*/

	if (fcp->num_item_resources == (Cardinal)0 ||
	    fcp->item_resources != (XtResourceList)NULL)
	{
			/* If this subclass doesn't add any of it's
			 * own resources, simply copy the superclass's
			 * pointers into this class's fields.		*/

	    if (fcp->num_item_resources == (Cardinal)0)
	    {
		fcp->quarked_items	= sfcp->quarked_items;
		fcp->item_resources	= sfcp->item_resources;
		fcp->num_item_resources	= sfcp->num_item_resources;
	    }
	    else
	    {
		XrmQuarkList	qlist;
		XtResourceList	rsc;
		XtResourceList	fcp_list;
		XtResourceList	sfcp_list;
		XrmQuark	quark;
		Cardinal	i;
		Cardinal	j;
		Cardinal	num = fcp->num_item_resources +
					sfcp->num_item_resources;

				/* Allocate arrays large enough to fit
				 * both the superclass and the subclass
				 * item resources.			*/

		qlist = (XrmQuarkList) XtMalloc((Cardinal)
					(sizeof(XrmQuark) * num));
		fcp->quarked_items = qlist;

		rsc = (XtResourceList) XtMalloc((Cardinal)
					(sizeof(XtResource) * num));

				/* Copy the superclass item resources
				 * into the new list.			*/

		for (i = 0; i < sfcp->num_item_resources; ++i)
		{
			rsc[i] = sfcp->item_resources[i];
			qlist[i] = sfcp->quarked_items[i];
		}

				/* Now merge this class's item resources
				 * into the new list.			*/

		for (i = 0, num = sfcp->num_item_resources,
		     fcp_list = fcp->item_resources;
		     i < fcp->num_item_resources;
		     ++i, ++fcp_list)
		{
			quark = XrmStringToQuark(fcp_list->resource_name);

					/* Compare each new item resource
					 * against the superclass's	*/

			for (j=0, sfcp_list = sfcp->item_resources;
			     j < sfcp->num_item_resources;
			     ++j, ++sfcp_list)
			{
					/* If match, override the super's
					 * with the subclass's		*/
					 
				if (fcp_list->resource_offset ==
					sfcp_list->resource_offset &&
				    quark == sfcp->quarked_items[j])
				{
					rsc[j] = *fcp_list;
					break;
				}
			}

				/* If these equal, the item resource
				 * is not in the superclass list	*/

			if (j == sfcp->num_item_resources)
			{
				qlist[num] = quark;
				rsc[num] = *fcp_list;
				++num;
			}
		} /* end of merging in this subclass's resources	*/

				/* At this point the two lists are merged.
				 * Now deallocate extra memory.		*/

		if (num < (fcp->num_item_resources + sfcp->num_item_resources))
		{
			rsc = (XtResourceList)XtRealloc((char *) rsc,
						sizeof(XtResource) * num);
			qlist = (XrmQuarkList)XtRealloc((char *) qlist,
						sizeof(XrmQuark) * num);
		}

				/* Cache the results in this class.	*/

		fcp->quarked_items	= qlist;
		fcp->item_resources	= rsc;
		fcp->num_item_resources	= num;
	    }
	}
	else
	{
		OlVaDisplayErrorMsg((Display *)NULL,OleNinvalidResource,
			OleTnullList, OleCOlToolkitError,
			OleMinvalidResource_nullList, "",
			CNAME(wc), "item_resources", "num_item_resources");
	}

	SetupRequiredResources(wc, fcp, sfcp);

		/* Inherit procedures that need to be inherited.
		 * If a required class procedure is NULL, an error
		 * is generated.					*/

#ifdef __STDC__
#define CHECK_CLASS_FIELDS(field, inherit_it, type) \
	if (fcp->field == inherit_it) {fcp->field = sfcp->field;}\
	else if (fcp->field == (type) NULL) {ClassErrorMsg(wc,#type);}
#else
#define CHECK_CLASS_FIELDS(field, inherit_it, type) \
	if (fcp->field == inherit_it) {fcp->field = sfcp->field;}\
	else if (fcp->field == (type) NULL) {ClassErrorMsg(wc, "type");}
#endif

	CHECK_CLASS_FIELDS(draw_item, XtInheritFlatDrawItem,
				OlFlatDrawItemProc)
	CHECK_CLASS_FIELDS(get_draw_info, XtInheritFlatGetDrawInfo,
				OlFlatGetDrawInfoProc)
	CHECK_CLASS_FIELDS(get_index, XtInheritFlatGetIndex,
				OlFlatGetIndexFunc)
	CHECK_CLASS_FIELDS(item_accept_focus, XtInheritFlatItemAcceptFocus,
				OlFlatItemAcceptFocusFunc)
	CHECK_CLASS_FIELDS(item_activate, XtInheritFlatItemActivate,
				OlFlatItemActivateFunc)
	CHECK_CLASS_FIELDS(item_dimensions, XtInheritFlatItemDimensions,
				OlFlatItemDimensionsProc)
	CHECK_CLASS_FIELDS(item_highlight, XtInheritFlatItemHighlight,
				OlFlatItemHighlightProc)
	CHECK_CLASS_FIELDS(layout, XtInheritFlatLayout,
				OlFlatLayoutProc)
	CHECK_CLASS_FIELDS(traverse_items, XtInheritFlatTraverseItems,
				OlFlatTraverseItemsFunc)
#undef CHECK_CLASS_FIELDS

		/* If the subclass has specified any value other than
		 * XtInheritFlatLayoutClass, it means that this subclass
		 * wants to be the layout class.			*/

	fcp->layout_class = (fcp->layout_class == XtInheritFlatLayoutClass ?
				sfcp->layout_class : wc);

				/* Now check the record size	*/

	if (fcp->rec_size == (Cardinal)0 || fcp->rec_size < sfcp->rec_size)
	{
		OlVaDisplayErrorMsg((Display *)NULL, OleNinvalidItemRecord,
			OleTflatState, OleCOlToolkitError,
			OleMinvalidItemRecord_flatState, CNAME(wc),
			(unsigned)fcp->rec_size, (unsigned)sfcp->rec_size);
	}
} /* END OF ClassPartInitialize() */

/*
 *************************************************************************
 * Destroy - this procedure frees memory allocated by the instance part
 ****************************procedure*header*****************************
 */
static void
Destroy(Widget w)
{
	FlatPart * 	fp = FPART(w);
	FlatWidget	fw = (FlatWidget)w;

	DestroyAcceleratorStrings(fp);
	XtFree(fp->resource_info.rdata);
	XtFree((char *) fp->resource_info.rlist);
	XtFree((char *) fp->resource_info.xlist);
	XtFree((char *) fp->item_geom.col_widths);
	XtFree((char *) fp->item_geom.row_heights);
	XtFree((char *) fp->item_part.label);

				/* Free reference from screen cache	*/

#ifdef sun
	(void) _OlFlatScreenManager(w, fw->primitive.scale, OL_DELETE_REF);
#else
	(void) _OlFlatScreenManager(w, OL_DEFAULT_POINT_SIZE, OL_DELETE_REF);
#endif

	OlgxDestroyAttrs (w, fp->pAttrs);

#if MANAGED_ITEMS_SUPPORTED
	XtFree((char *) fp->managed_items);
#endif /* MANAGED_ITEMS_SUPPORTED */

} /* END OF Destroy() */

/*
 *************************************************************************
 * DrawItem - this class routine redisplays an item into a
 * specified drawable.
 ****************************procedure*header*****************************
 */
static void
DrawItem (Widget w, FlatItem item, OlFlatDrawInfo *di)
{
	FlatPart *		fp = FPART(w);
	OlgxAttrs *		item_attrs;
	OlDrawProc              drawProc;
	FlatLabel		*lbl;
	int			flags = 0;

	_OlFlatSetupAttributes(w, item, di, &item_attrs, &lbl, &drawProc);

	if (item->flat.label != (String)NULL && 
	    item->flat.text_format == OL_WC_STR_REP)
		flags |= OLGX_LABEL_IS_WCS;
	if ( !(item->flat.sensitive && item->flat.ancestor_sensitive) )
	    flags |= OLGX_INACTIVE;
	(*drawProc) (di->screen, di->drawable, fp->pAttrs,
		     di->x, di->y, di->width, di->height, (XtPointer)lbl,
		     OL_LABEL, flags);

} /* END OF DrawItem() */

/*
 *************************************************************************
 * ExpandItem - This procedure adds additional information to the
 * expanded item.  This procedure is automatically called during the
 * forward-chained item expansion before the application attributes and
 * the internally managed attributes are merged in.
 *
 * Note: only those resources that which cannot be adequately set and
 * maintained in the flat widget's Initialize and SetValues procedures
 * need to be set in this routine, e.g., when a widget's managed field is
 * changed, the SetValues procedure is not called.
 ****************************procedure*header*****************************
 */
static void
ExpandItem (Widget w, FlatItem item)
{
	FlatPart *	fp = FPART(w);
	FlatItemPart *	item_part = &item->flat;

			/* Set the resources that are not inherited from
			 * the container.				*/

	item_part->managed		= (Boolean)True;
	item_part->mapped_when_managed	= (Boolean)True;

			/* If we're internally managing the accelerator
			 * text strings, stick them into the item part	*/

	if (fp->flags & OL_B_FLAT_ACCELERATOR_TEXT)
	{
		item_part->qualifier_text =
				fp->acc_text[item_part->item_index].qual;
		item_part->meta_key =
				fp->acc_text[item_part->item_index].meta;
		item_part->accelerator_text =
				fp->acc_text[item_part->item_index].acc;
	}
} /* END OF ExpandItem() */

/*
 *************************************************************************
 * GetDrawInfo - given the index of a sub-object, this routine returns
 * the rectangle associated with it.  It's noted that the rectangle is
 * the row/column rectangle containing the entire sub-object, but the
 * sub-object may or may not fill the whole rectangle.
 * The returned x and y positions are relative to the NorthWest corner
 * of the container.  If the item's rectangle is clipped by the
 * container, the clipped flag is set to True and the 'clip_width' and
 * 'clip_height' fields are set to the usable width and height.
 * If the rectangle is not clipped, the 'clipped' flag is set to 
 * False and the 'clip_width' and 'clip_height' fields are set to
 * the normal width & height of the rectangle.
 *
 * The fields 'drawable,' 'screen' and 'background' are not modified by
 * this routine.
 ****************************procedure*header*****************************
 */
static void
GetDrawInfo (Widget w, Cardinal item_index, OlFlatDrawInfo *di)
{
	FlatPart *		fp = FPART(w);
	Cardinal		row;		/* item's row		*/
	Cardinal		col;		/* items' column	*/
	Boolean			do_gravity = False;
	register Cardinal	i;		/* loop counter		*/
	OlFlatGeometry *	igeo = &fp->item_geom;

	if (item_index >= fp->num_items)
	{
		OlVaDisplayWarningMsg(XtDisplay(w),
			OleNbadItemIndex, OleTflatState,
			OleCOlToolkitWarning,
			OleMbadItemIndex_flatState,
			XrmQuarkToString(w->core.xrm_name),
			W_CNAME(w), "GetDrawInfo", (unsigned)item_index);

		di->x		= (Position) 0;
		di->y		= (Position) 0;
		di->width	= (Dimension) 0;
		di->height	= (Dimension) 0;
		di->clipped	= (Boolean) False;
		di->clip_width	= (Dimension) 0;
		di->clip_height	= (Dimension) 0;
		return;
	}

	if (fp->layout_type == OL_FIXEDCOLS)
	{
		row = item_index/igeo->cols;
		col = item_index - (row * igeo->cols);
	}
	else
	{
		col = item_index/igeo->rows;
		row = item_index - (col * igeo->rows);
	}

				/* Determine the overall y position and
				 * then do a gravity calculation if
				 * necessary				*/

	di->y = igeo->y_offset + (Position) (row * fp->v_space) -
				(Position) (row * fp->overlap);

	if (fp->same_height == OL_ALL)
	{
		di->height	= igeo->row_heights[0];
		di->y		+= di->height * row;
	}
	else
	{
		di->height = (fp->same_height == OL_ROWS ?
				igeo->row_heights[row] :
			(do_gravity = True,
				igeo->row_heights[igeo->rows + item_index]));

		for (i=0; i < row; ++i)
		{
			di->y += (Position) igeo->row_heights[i];
		}
	}
				/* Determine the overall x position and
				 * then do a gravity calculation if
				 * necessary				*/

	di->x = igeo->x_offset + (Position)(col * fp->h_space) -
			(Position)(col * fp->overlap);

	if (fp->same_width == OL_ALL)
	{
		di->width = igeo->col_widths[0];
		di->x += di->width * col;
	}
	else
	{
		di->width = (fp->same_width == OL_COLUMNS ?
				igeo->col_widths[col] :
			(do_gravity = True,
				igeo->col_widths[igeo->cols + item_index]));

		for (i=0; i < col; ++i)
		{
			di->x += (Position) igeo->col_widths[i];
		}
	}

				/* Do we have to position this item within
				 * it's row and column?			*/

	if (do_gravity == True && fp->item_gravity != NorthWestGravity)
	{
		_OlDoGravity(fp->item_gravity, igeo->col_widths[col],
			igeo->row_heights[row], di->width, di->height,
			&di->x, &di->y);
	}

	di->clipped = ((Dimension)((Dimension) di->x + di->width)
					> w->core.width ||
		   	(Dimension) ((Dimension) di->y + di->height)
					> w->core.height ?
			True : False);

	if (di->clipped == False)
	{	
		di->clip_width	= di->width;
		di->clip_height	= di->height;
	}
	else
	{
		di->clip_width = ((Dimension)((Dimension) di->x + di->width)
					> w->core.width ?
				w->core.width - di->width : di->width);

		di->clip_height = ((Dimension)((Dimension) di->y + di->height)
					> w->core.height ?
				w->core.height - di->height : di->height);
	}
} /* END OF GetDrawInfo() */

/*
 *************************************************************************
 * GetIndex - this routine locates a particular sub-object based
 * on a coordinate pair (x,y).  If the x,y pair falls within an item, the
 * item index is returned, else, a OL_NO_ITEM value is returned.
 * The values of X and Y are relative to the interior of the container
 * at hand.
 * When determining which item a coordinate pair falls into, the
 * following rules are observed:
 *	1. an x coordinate is in an item if it's wholely contained within
 *	   that item or is directly on the left edge.
 *	2. a y coordinate is in an item if it's wholely contained within
 *	   that item or is directly on the top edge.
 *
 * If the (x,y) pair is outside the container, OL_NO_ITEM is returned.
 * The application also can request that the sensitivity of the item be
 * ignored.  Normally, an item is not considered valid if it's
 * insensitive.  If the 'ignore_sensitivity' flag is set, the item's id
 * will be returned.
 ****************************procedure*header*****************************
 */
static Cardinal
GetIndex (Widget w, Position x, Position y, Boolean ignore_sensitivity)
{
	FlatPart *		fp = FPART(w);
	Cardinal		item_index;
	Cardinal		row;		/* item's row		*/
	Cardinal		col;		/* items' column	*/
	Position		envelope;	/* horiz or vert location*/
	Position		adder;		/* constant value to add*/
	Position		worh;		/* width or height	*/
	Cardinal		row_mask;	/* row index mask	*/
	Cardinal		col_mask;	/* column index mask	*/
	OlFlatGeometry *	igeo;

	igeo = &fp->item_geom;

	if (x >= (Position)w->core.width || y >= (Position)w->core.height ||
	    x < (Position)0 || y < (Position)0 )
	{
		return((Cardinal)OL_NO_ITEM);
	}

	row_mask = (Cardinal) (fp->same_height == OL_ALL ? 0 : ~0);
	col_mask = (Cardinal) (fp->same_width == OL_ALL ? 0 : ~0);

						/* Find the item's row	*/

	adder		= (Position) fp->v_space - (Position) fp->overlap;
	envelope	= igeo->y_offset - adder;

	for (row=0; row < igeo->rows; ++row)
	{
		worh	= (Position) igeo->row_heights[row & row_mask];

		if ((envelope += adder + worh) > y)
		{
				/* The y coordinate is within the envelope,
				 * but is it between the rows ?		*/

			if (y < (envelope - worh))
			{
				row = igeo->rows;
			}
			break;
		}
	}

	if (row == igeo->rows)
	{
		return((Cardinal)OL_NO_ITEM);
	}

					/* Find the item's column	*/

	adder		= (Position) fp->h_space - (Position) fp->overlap;
	envelope	= igeo->x_offset - adder;

	for (col=0; col < igeo->cols; ++col)
	{
		worh	= (Position) igeo->col_widths[col & col_mask];

		if ((envelope += adder + worh) > x)
		{
				/* The x coordinate is within the envelope,
				 * but is it between the columns ?	*/

			if (x < (envelope - worh))
			{
				col = igeo->cols;
			}
			break;
		}
	}

	if (col == igeo->cols)
	{
		return((Cardinal)OL_NO_ITEM);
	}
		/* If we've reached this point, we've got the row and
		 * the column.  Now we have to get the index and then
		 * check to see if this item fills up it's cell.  If it
		 * doesn't, we've to to apply the item's gravity and
		 * check it again.					*/

	item_index = (fp->layout_type == OL_FIXEDCOLS ? row * igeo->cols + col :
				col * igeo->rows + row);

	if (item_index >= fp->num_items)
	{
		return((Cardinal)OL_NO_ITEM);
	}

	if (fp->same_height != OL_ALL || fp->same_width != OL_ALL)
	{
		Dimension item_width	= igeo->col_widths[
			(fp->same_width == OL_ALL ? 0 :
			 fp->same_width == OL_COLUMNS ? col :
						igeo->cols + item_index)];

		Dimension item_height	= igeo->row_heights[
			(fp->same_height == OL_ALL ? 0 :
			 fp->same_height == OL_ROWS ? row :
						igeo->rows + item_index)];

		Position  item_x;
		Position  item_y;

		_OlDoGravity(fp->item_gravity, 
			igeo->col_widths[(fp->same_width == OL_ALL ? 0 : col)],
			igeo->row_heights[(fp->same_height == OL_ALL ? 0 : row)],
			item_width, item_height,
			&item_x, &item_y);

		item_x += x;
		item_y += y;

		if (!(	x >= item_x && x < (item_x + (Position)item_width) &&
			y >= item_y && y < (item_y + (Position)item_height)))
		{
			item_index = (Cardinal)OL_NO_ITEM;
		}
	}

	if (item_index != (Cardinal) OL_NO_ITEM)
	{
		FlatClassPart *	fcp = FCPART(w);
		ALLOC_STACK_ITEM(fcp, item, stack);

		_OlFlatExpandItem(w, item_index, item);

		if (item->flat.mapped_when_managed == False ||
		    (ignore_sensitivity == False &&
		     (item->flat.sensitive == False ||
		      item->flat.ancestor_sensitive == False)))
		{
			item_index = (Cardinal) OL_NO_ITEM;
		}
		FREE_STACK_ITEM(item,stack);
	}
	return(item_index);
} /* END OF GetIndex() */

/*
 *************************************************************************
 * HighlightHandler - this routine is called whenever the flattened widget
 * container gains or looses focus.
 ****************************procedure*header*****************************
 */
static void
HighlightHandler (Widget w, OlDefine type)
{
	FlatPart *	fp = FPART(w);
	FlatClassPart *	fcp = FCPART(w);
	ALLOC_STACK_ITEM(fcp, item, stack);

	if (type == OL_IN)
	{
		Boolean	took_focus = False;

		/* If we get a focus in, use try to set focus to the
		 * last item that had it (since the focus out may have
		 * been caused by a pointer grab, e.g., the window
		 * manager grabbed the pointer when dragging the window).
		 * But if the focus out wasn't due to a grab, then we
		 * have to check to see if the last focus item still
		 * can take focus.  So in either case, we have to
		 * play it safe and formally request the item to 
		 * take focus.
		 */
		if (fp->last_focus_item != (Cardinal) OL_NO_ITEM)
		{
			_OlFlatExpandItem(w, fp->last_focus_item, item);
			took_focus = _OlFlatItemAcceptFocus(w, item,
								CurrentTime);
		}

		if (took_focus == False)
		{
			fp->last_focus_item = (Cardinal)OL_NO_ITEM;
			(void) SetFocusToAnyItem(w, CurrentTime);
		}
	}
	else if (type == OL_OUT && fp->focus_item != (Cardinal)OL_NO_ITEM)
	{
		_OlFlatExpandItem(w, fp->focus_item, item);

		fp->focus_item = (Cardinal)OL_NO_ITEM;

		_OlFlatItemHighlight(w, item, OL_OUT);
	}

	FREE_STACK_ITEM(item,stack);
} /* END OF HighlightHandler() */

/*
 *************************************************************************
 * Initialize - this procedure initializes the instance part of the widget
 ****************************procedure*header*****************************
 */
/* ARGSUSED */
static void
Initialize (Widget request, Widget new, ArgList args, Cardinal *num_args)
{
	FlatPart *		fp = FPART(new);
	FlatWidget		nfw = (FlatWidget)new;
	OlFlatGeometry *	igeo = &fp->item_geom;
	OlFlatScreenCache *	sc;
	OlStrRep 	tf = ((FlatWidget)new)->primitive.text_format; 

			/* If the items and itemFields converters have not
			 * modified the num_items and num_item_fields
			 * values, set them now.			*/

	if (fp->num_items == (Cardinal) OL_IGNORE)
	{
		fp->num_items = (Cardinal)0;
	}

	if (fp->num_item_fields == (Cardinal) OL_IGNORE)
	{
		fp->num_item_fields = (Cardinal)0;
	}

			/* Initialize non-application settable data	*/

	igeo->col_widths	= (Dimension *) NULL;
	igeo->row_heights	= (Dimension *) NULL;
	igeo->x_offset		= (Position) 0;
	igeo->y_offset		= (Position) 0;
	igeo->bounding_width	= (Dimension) 0;
	igeo->bounding_height	= (Dimension) 0;
	igeo->rows		= (Cardinal) 0;
	igeo->cols		= (Cardinal) 0;
	fp->overlap		= (Dimension) 0;
	fp->flags		= OL_B_FLAT_RELAYOUT_HINT;
	fp->acc_text		= (struct _acc_text *)NULL;
	fp->focus_item		= (Cardinal)OL_NO_ITEM;
	fp->last_focus_item	= (Cardinal)OL_NO_ITEM;
	fp->items_touched	= (fp->num_items != (Cardinal)0 ?
					True : False);
	fp->pAttrs		= (OlgxAttrs *) NULL;
	fp->resource_info.xlist		= (Cardinal *) NULL;
	fp->resource_info.num_xlist	= (Cardinal) 0;
	fp->resource_info.rlist		= (Cardinal *) NULL;
	fp->resource_info.num_rlist	= (Cardinal) 0;
	fp->resource_info.rdata		= (char *) NULL;

						/* Cache screen data	*/

#ifdef sun
	sc = _OlFlatScreenManager(new, nfw->primitive.scale, OL_ADD_REF);
#else
	sc = _OlFlatScreenManager(new, OL_DEFAULT_POINT_SIZE, OL_ADD_REF);
#endif

			/* If the container has no font, use the one
			 * in the screen cache.				*/

	if (PWIDGET(new, font) == (OlFont)NULL)
	{
		PWIDGET(new, font) = sc->font;
	}

		/* Initialize the default item part.  Since some of
		 * the item part fields exist in widget superclasses,
		 * we explicitly initialize them here because it's
		 * undefined in the Intrinsics that the creation and
		 * XtSetValues will initialize more than one field
		 * with the same name, i.e., it may only set the first
		 * field encountered and not initialize any fields
		 * in the subclass with the same resource name.		*/

	if (fp->item_part.label != (OlStr)NULL)
	{
		OlStr temp;

		temp = (OlStr)XtMalloc(
			(*str_methods[tf].StrNumBytes)(
					fp->item_part.label));
		(*str_methods[tf].StrCpy)(temp,fp->item_part.label);
		fp->item_part.label = temp;	
	}

	fp->item_part.mnemonic		= (OlMnemonic)0;
	fp->item_part.accelerator	= (String)NULL;
	fp->item_part.qualifier_text	= (String)NULL;
	fp->item_part.meta_key		= False;
	fp->item_part.accelerator_text	= (String)NULL;
	fp->item_part.sensitive		= (Boolean)True;
	fp->item_part.ancestor_sensitive= new->core.ancestor_sensitive;
	fp->item_part.background_pixel	= new->core.background_pixel;
	fp->item_part.background_pixmap	= new->core.background_pixmap;
	fp->item_part.font		= PWIDGET(new, font);
	fp->item_part.text_format	= tf;
	fp->item_part.font_color	= PWIDGET(new, font_color);
	fp->item_part.foreground	= PWIDGET(new, foreground);
	fp->item_part.input_focus_color	= PWIDGET(new, input_focus_color);
	fp->item_part.traversal_on	= PWIDGET(new, traversal_on);
	fp->item_part.user_data		= PWIDGET(new, user_data);

			/* Now that we've initialized the default
			 * item_part, do the container's background.	*/

	CALL_TRANSPARENT_PROC(new);

	GetGCs(new, fp);

				/* Check the layout parameters for
				 * each subclass so that we can do the
				 * layout.  Since it's initialization
				 * time, set the relayout hint to True	*/

	CheckLayoutParameters((Widget) NULL, (FlatPart *)NULL, new, fp);

				/* Call the layout convenience routine	*/

	(void) _OlFlatCheckItems(flatWidgetClass, (Widget)NULL,
				request, new, args, num_args);

} /* END OF Initialize() */

/*
 *************************************************************************
 * ItemAcceptFocus - this routine is called to set focus to a particular
 * sub-object.  Note this routine merely returns a Boolean value
 * indicating if the sub-object can accept focus or not.
 ****************************procedure*header*****************************
 */
static Boolean
ItemAcceptFocus (Widget w, FlatItem item, Time time)
{
	Boolean		can_take_focus;

	if (item->flat.sensitive == (Boolean)True		&&
	    item->flat.managed == (Boolean)True			&&
	    item->flat.traversal_on == (Boolean)True		&&
	    item->flat.mapped_when_managed == (Boolean)True	&&
	    (PWIDGET(w, has_focus) == True ||
	     OlCanAcceptFocus(w, time) == True))
	{
		Cardinal	old_focus_item;

				/* If the requested item can take focus
				 * and the flat container already has focus,
				 * call the unhighlight handler for the
				 * item that has focus and the highlight
				 * handler the new focus item.
				 * If the container does not have focus
				 * yet, set it to the container.
				 */

		can_take_focus			= True;
		old_focus_item			= FPART(w)->focus_item;
		FPART(w)->focus_item		= item->flat.item_index;
		FPART(w)->last_focus_item	= item->flat.item_index;

		if (PWIDGET(w, has_focus) == True)
		{
			if (old_focus_item != (Cardinal)OL_NO_ITEM)
			{
				FlatClassPart *	fcp = FCPART(w);
				ALLOC_STACK_ITEM(fcp, old_item, stack);

				_OlFlatExpandItem(w, old_focus_item, old_item);
				_OlFlatItemHighlight(w, old_item, OL_OUT);

				FREE_STACK_ITEM(old_item,stack);
			}

			_OlFlatItemHighlight(w, item, OL_IN);
		}
		else
		{
				/* Set focus to the container.  When the
				 * container gains focus, it's highlight
				 * handler will be called and the appropriate
				 * sub-object will be highlighted since we've
				 * set the container's focus_item field.
				 */
			OlSetInputFocus(w, RevertToNone, time);
		}
	}
	else
	{
		can_take_focus = False;
	}

	return(can_take_focus);
} /* END OF ItemAcceptFocus() */

/*
 *************************************************************************
 * ItemHighlight - this routine is called to highlight or unhighlight a
 * particular sub-object as it gains or loses focus.
 ****************************procedure*header*****************************
 */
/* ARGSUSED */
static void
ItemHighlight (Widget w, FlatItem item, OlDefine type)
{
	if(_OlInputFocusFeedback(w) == OL_SUPERCARET)
		return;
	_OlFlatRefreshItem(w, item->flat.item_index, True);
} /* END OF ItemHighlight() */

/*
 *************************************************************************
 * ItemInitialize -
 ****************************procedure*header*****************************
 */
/* ARGSUSED */
static void
ItemInitialize (Widget w, FlatItem request, FlatItem new, ArgList args, Cardinal *num_args)
{
	FlatPart *	fp = FPART(w);

	switch(new->flat.label_justify)
	{
	case OL_LEFT:		/* FALLTHROUGH	*/
	case OL_RIGHT:		/* FALLTHROUGH	*/
	case OL_CENTER:
		break;		/* good justification, do nothing*/
	default:
		OlVaDisplayWarningMsg((Display *)NULL,
			OleNinvalidResource,
			OleTflatState, OleCOlToolkitWarning,
			OleMinvalidResource_flatState,
			XtName(w), W_CNAME(w), XtNlabelJustify,
			(unsigned)new->flat.item_index, "OL_CENTER");

		new->flat.label_justify = (OlDefine)OL_CENTER;
		break;
	}

	CheckFont(w, fp, new);

	(void)SetAccAndMnem(w, (FlatItem)NULL, new);

} /* END OF ItemInitialize() */

/*
 *************************************************************************
 * ItemDimensions - this class routine determines the size of a single
 * sub-object.
 ****************************procedure*header*****************************
 */
static void
ItemDimensions (Widget w, FlatItem item, Dimension *width, Dimension *height)
{
	FlatPart *	fp = FPART(w);
	void		(*sizeProc)();
	FlatLabel	*lbl;

	_OlFlatSetupLabelSize(w, item, &lbl, &sizeProc);
	(*sizeProc)(XtScreen(w), fp->pAttrs, (XtPointer)lbl, width, height);

} /* END OF ItemDimensions() */

/*
 *************************************************************************
 * ItemsTouched - this procedure is called whenever a new list is given to
 * the flat widget or whenever the existing list is touched.
 * In this routine, we de-allocate any memory associated with the last
 * list and create any memory needed for the new list.
 ****************************procedure*header*****************************
 */
/* ARGSUSED */
static void
ItemsTouched (Widget current, Widget request, Widget new, ArgList args, Cardinal *num_args)
{ 
	FlatPart *	fp = FPART(new);
	Cardinal	i;
	int		has_acc;
	int		has_acc_text;

	if (fp->focus_item != (Cardinal) OL_NO_ITEM)
	{
		fp->last_focus_item	= (Cardinal)OL_NO_ITEM;
		fp->focus_item		= (Cardinal)OL_NO_ITEM;
	}

			/* Once we've set the focus item, reset the
			 * last focus item.				*/

	fp->last_focus_item = fp->focus_item;

			/* Since the user has added a new list or
			 * changed the old list, we must remove all
			 * mnemonics and accelerators associated
			 * with this flat widget.
			 */
	_OlRemoveAccelerator(new, (XtPointer)NULL, True, (String)NULL);
	_OlRemoveMnemonic(new, (XtPointer)NULL, True, (OlMnemonic)'\0');

			/* If the internally managed accelerator_text
			 * list is non-NULL, free the strings in the
			 * list and then the list.
			 */

	if (current != (Widget)NULL) {
		DestroyAcceleratorStrings(FPART(current));
	}

			/* Scan the new item fields looking for
			 * XtNaccelerator and XtNacceleratorText.	*/

	for (i=0, has_acc = has_acc_text = 0; i < fp->num_item_fields; ++i)
	{
		if (!strcmp(XtNaccelerator, fp->item_fields[i]))
		{
			has_acc = 1;
		}
		else if (!strcmp(XtNacceleratorText, fp->item_fields[i]))
		{
			has_acc_text = 1;
		}
	}

			/* If the application has specified an accelerator
			 * and did not specify an accelerator text string,
			 * we must maintain one internally.
			 */
	fp->flags = OL_B_FLAT_RELAYOUT_HINT;
	if (has_acc && !has_acc_text && fp->num_items > 0)
	{
		fp->flags	|= OL_B_FLAT_ACCELERATOR_TEXT;
		fp->acc_text	= (struct _acc_text *)XtCalloc(fp->num_items,
						    sizeof(struct _acc_text));
	}
	else
	{
		fp->flags	&= ~OL_B_FLAT_ACCELERATOR_TEXT;
		fp->acc_text	= (struct _acc_text *)NULL;
	}
} /* END OF ItemsTouched() */

/*
 *************************************************************************
 * Layout - this routine determines the logical layout of a flat
 * widget container based on the information given for it's sub-objects.
 * This procedure puts information into the FlatPart concerning:
 * 	- the rectangle needed to tightly fit around all sub-objects.
 *	- the description of the container's layout, including gravity.
 *
 * The layout is done in a two step process.  First the width and height
 * needed to tightly fit around the items is calculated.  These
 * dimensions are not affected by the container's actual size.  After
 * the tightly fitting box is calculated, the container's width and
 * height are calculated based on the XtNlayoutType resource (these
 * are the returned values).  Next the container's gravity is calculated
 * based on the widget's dimensions.  Therefore, if the container plans
 * to used the calculated container's width/height, the arguments
 * 'return_width' and 'return_height' should be pointers to the
 * widget's core width and core height fields.  If this is not done, 
 * the gravity calculations will be wrong.
 * In either case, the 'return_width' and 'return_height' should be
 * initialized outside this routine.
 *
 * This routine caches information about the layout in the fields
 * row_heights and col_widths.  These two arrays are symmetrical in
 * usage.  Therefore:
 *
 *	Value of XtNsameWidth	'col_widths' contents
 *
 *		OL_ALL		1 element in array containing the
 *				width of all columns
 *		OL_COLUMNS	'cols' elements in the array containing
 *				the width of each column
 *		OL_NONE		('cols' + 'number of managed items')
 *				elements, containing the width of
 *				each column in the first 'cols' locations
 *				and the width of each item in the
 *				remaining locations.
 *		
 *	Value of XtNsameHeight	'row_heights' contents
 *
 *		OL_ALL		1 element in array containing the
 *				height of all rows
 *		OL_ROWS		'rows' elements in the array containing
 *				the height of each row
 *		OL_NONE		('rows' + 'number of managed items')
 *				elements, containing the height of
 *				each row in the first 'rows' locations
 *				and the height of each item in the
 *				remaining locations.
 *		
 ****************************procedure*header*****************************
 */
static void
Layout(Widget w, Dimension *return_width, Dimension *return_height)
	      			  		/* Flat widget		*/
	           		             	/* returned width	*/
	           		              	/* returned height	*/
{
	FlatPart * 	fp = FPART(w);
	Cardinal	i;		/* loop counter			*/
	Dimension	width;		/* local width calculation	*/
	Dimension	height;		/* local height calculation	*/
	Cardinal	rows;		/* number of rows		*/
	Cardinal	cols;		/* number of columns		*/
	W_and_H			local_array[20];
	register W_and_H *	w_h_ptr;
	OlFlatGeometry *	igeo;		/* calc. item geometry	*/

	igeo = &fp->item_geom;

				/* Free the old information and
				 * re-initialize the geometry structure	*/

	XtFree((XtPointer) igeo->col_widths);
	XtFree((XtPointer) igeo->row_heights);

	igeo->col_widths	= (Dimension *) NULL;
	igeo->row_heights	= (Dimension *) NULL;
	igeo->x_offset		= (Position) 0;
	igeo->y_offset		= (Position) 0;
	igeo->bounding_width	= (Dimension) 0;
	igeo->bounding_height	= (Dimension) 0;
	igeo->rows		= (Cardinal) 0;
	igeo->cols		= (Cardinal) 0;


				/* If there are no items, return; else
				 * set the width and height array pointer*/

	if (fp->num_items == (Cardinal) 0)
	{
		return;
	}
	else if (fp->num_items > XtNumber(local_array))
	{
		w_h_ptr = (W_and_H *) XtMalloc(fp->num_items * sizeof(W_and_H));
	}
	else
	{
		w_h_ptr = local_array;
	}

			/* Before doing the layout, get the width
			 * and height of each item.
			 * Get the size calculation procedure from the
			 * class field.  We don't have to see if it is
			 * NULL since the ClassPartInitialize procedure
			 * did the check already.			*/

	{
	    FlatClassPart *	fcp = FCPART(w);
	    ALLOC_STACK_ITEM(fcp, item, stack);

	    for (i=0; i < fp->num_items; ++i)
	    {
		_OlFlatExpandItem(w, i, item);
		_OlFlatItemDimensions(w, item, &width, &height);

					/* Make sure width is valid	*/

		if (fp->item_min_width != (Dimension) OL_IGNORE &&
		    width < fp->item_min_width)
		{
			width = fp->item_min_width;
		}
		if (fp->item_max_width != (Dimension) OL_IGNORE && 
			   fp->item_max_width < width)
		{
			width = fp->item_max_width;
		}
					/* Make sure height is valid	*/

		if (fp->item_min_height != (Dimension) OL_IGNORE &&
		    height < fp->item_min_height)
		{
			height = fp->item_min_height;
		}
		if (fp->item_max_height != (Dimension) OL_IGNORE && 
			   fp->item_max_height < height)
		{
			height = fp->item_max_height;
		}

		w_h_ptr[i].width	= width;
		w_h_ptr[i].height	= height;
	    }
	    FREE_STACK_ITEM(item,stack);
	}

	if (fp->layout_type == OL_FIXEDCOLS)
	{
		if (fp->num_items <= (Cardinal) fp->measure)
		{
			cols = fp->num_items;
			rows = (Cardinal) 1;
		}
		else
		{
			cols = (Cardinal) fp->measure;
			rows = fp->num_items/cols +
				(fp->num_items % cols == 0 ? 0 : 1);
		}
	}
	else
	{
		if (fp->num_items <= (Cardinal) fp->measure)
		{
			rows = fp->num_items;
			cols = (Cardinal) 1;
		}
		else
		{
			rows = (Cardinal) fp->measure;
			cols = fp->num_items/rows +
				(fp->num_items % rows == 0 ? 0 : 1);
		}
	}

	igeo->rows = rows;		/* Cache the number of rows	*/
	igeo->cols = cols;		/* Cache the number of columns	*/

	if (fp->same_width == OL_ALL)
	{
					/* Allocate a single slot	*/

		igeo->col_widths = XtNew(Dimension);

		igeo->col_widths[0] = CalcItemSize(fp, w_h_ptr,
				ALL_ITEMS, WIDTH, 0, NULL, NULL);
		width = (Dimension) (cols * igeo->col_widths[0]);
	}
	else
	{
		Cardinal slots;

		slots = cols + (fp->same_width == OL_COLUMNS ?
					0 : fp->num_items);

		igeo->col_widths = (Dimension *)
					XtMalloc(slots * sizeof(Dimension));

					/* Fill in the maximum width of
					 * each column.			*/

		for (i=0, width = 0; i < cols; ++i)
		{
			igeo->col_widths[i] = CalcItemSize(fp,
				w_h_ptr, COLUMN_ITEMS, WIDTH, i, rows, cols);
			width += igeo->col_widths[i];
		}

				/* If 'same_width' equals OL_NONE,
				 * put the width of each item into the
				 * array.				*/

		if (fp->same_width == OL_NONE)
		{
			for(i=0; i < fp->num_items; ++i)
			{
				igeo->col_widths[i+cols] = w_h_ptr[i].width;
			}
		}
	} /* end of 'fp->same_width != OL_ALL' */

					/* Calculate the heights	*/

	if (fp->same_height == OL_ALL)
	{
					/* Allocate a single slot	*/

		igeo->row_heights = XtNew(Dimension);

		igeo->row_heights[0] = CalcItemSize(fp, w_h_ptr,
				ALL_ITEMS, HEIGHT, 0, NULL, NULL);
		height = (Dimension) (rows * igeo->row_heights[0]);
	}
	else
	{
		Cardinal slots;

		slots = rows + (fp->same_height == OL_ROWS ? 0 : fp->num_items);

		igeo->row_heights = (Dimension *)
					XtMalloc(slots * sizeof(Dimension));

					/* Fill in the maximum height of
					 * each row.			*/

		for (i=0, height = 0; i < rows; ++i)
		{
			igeo->row_heights[i] = CalcItemSize(fp,
				w_h_ptr, ROW_ITEMS, HEIGHT, i, rows, cols);
			height += igeo->row_heights[i];
		}

				/* If 'same_height' equals OL_NONE,
				 * put the height of each item into the
				 * array.				*/

		if (fp->same_height == OL_NONE)
		{
			for(i=0; i < fp->num_items; ++i)
			{
				igeo->row_heights[i+rows] = w_h_ptr[i].height;
			}
		}
	} /* end of 'fp->same_height != OL_ALL' */

	
	width	+= 2*(fp->h_pad) + (cols - 1)*(fp->h_space) -
					(cols - 1) * fp->overlap;
	height	+= 2*(fp->v_pad) + (rows - 1)*(fp->v_space) -
					(rows - 1) * fp->overlap;

				/* Set bounding width and height.  These
				 * values do not include the exterior
				 * padding.				*/

	igeo->bounding_width  = width - (2 * fp->h_pad);
	igeo->bounding_height = height - (2 * fp->v_pad);

				/* Free width and height array if
				 * necessary				*/

	if (w_h_ptr != local_array)
	{
		XtFree((char *) w_h_ptr);
	}

				/* Now, check the layout rules to see
				 * what the container size must be	*/

	switch(fp->layout_width)
	{
	case OL_MINIMIZE:
		*return_width = width;
		break;
	case OL_MAXIMIZE:
		*return_width = (width > w->core.width ? width : w->core.width);
		break;
	default:					/* OL_IGNORE	*/
		*return_width = w->core.width;
		break;
	}

	switch(fp->layout_height)
	{
	case OL_MINIMIZE:
		*return_height = height;
		break;
	case OL_MAXIMIZE:
		*return_height = (height > w->core.height ? height :
					w->core.height);
		break;
	default:					/* OL_IGNORE	*/
		*return_height = w->core.height;
		break;
	}

		/* Now update the gravity fields.  We'll assume the
		 * returned width and the returned height will be
		 * the container's dimensions.  (This is a safe
		 * assumption.)						*/

	CalcOffsets(fp, *return_width, *return_height);

} /* END OF Layout() */

/*
 *************************************************************************
 * Redisplay - this routine is a generic redisplay routine.  It 
 * loops over all of the sub-objects and calls a draw routine.
 * This routine does not have to check the class procedures for NULL since
 * the ClassPartInitialize procedure has already checked.
 ****************************procedure*header*****************************
 */
/* ARGSUSED */
static void
Redisplay(Widget w, XEvent *xevent, Region region)
	      		  			/* exposed widget	*/
	        	       			/* the exposure		*/
	      		       			/* compressed exposures	*/
{
	FlatPart *	fp = FPART(w);
	FlatClassPart *	fcp = FCPART(w);
	ALLOC_STACK_ITEM(fcp, item, stack);
	OlFlatDrawInfo	draw_info;
	register
	OlFlatDrawInfo *di = &draw_info;
	Cardinal	item_index;		/* item index to refresh*/
	Position	h_start;		/* Expose x position	*/
	Position	v_start;		/* Expose y position	*/
	Position	h_end;			/* Expose x + width pos.*/
	Position	v_end;			/* Expose y + height pos.*/
	Cardinal	row;			/* First row to draw at	*/
	Cardinal	col;			/* column loop counter	*/
	Cardinal	col_start;		/* first column to draw	*/
	Cardinal	col_stop;		/* stop drawing at this col.*/
	Position	x_start;		/* first column's x pos.*/
	Position	y_adder;		/* constant vertical incr.*/
	Position	x_adder;		/* constant horiz. incr.*/
	Boolean		do_gravity;
	Cardinal	row_mask;		/* Row index mask	*/
	Cardinal	col_mask;		/* Column index mask	*/
	OlFlatGeometry *	igeo;

	di->drawable	= (Drawable) XtWindow(w);
	di->screen	= XtScreen(w);
	di->background	= w->core.background_pixel;

			/* Put the event into a more usable form	*/

	h_start	= (Position) xevent->xexpose.x;
	v_start	= (Position) xevent->xexpose.y;
	h_end	= (Position) xevent->xexpose.width + h_start;
	v_end	= (Position) xevent->xexpose.height + v_start;

	igeo = &fp->item_geom;

				/* Initialize the constant incrementers	*/

	x_adder = (Position) fp->h_space - (Position) fp->overlap;
	y_adder = (Position) fp->v_space - (Position) fp->overlap;

	row_mask = (Cardinal) (fp->same_height == OL_ALL ? 0 : ~0);
	col_mask = (Cardinal) (fp->same_width == OL_ALL ? 0 : ~0);

			/* Calculate the columns to start refreshing at
			 * and the number of columns after that to
			 * refresh.					*/

	if (h_start <= igeo->x_offset &&
	    (igeo->x_offset + (Position)igeo->bounding_width) <= h_end)
	{
		col_start	= (Cardinal) 0;
		x_start		= igeo->x_offset;
		col_stop	= igeo->cols;
	}
	else
	{
		for (col=0, di->x = igeo->x_offset - x_adder,
		     col_start = igeo->cols, col_stop = igeo->cols;
		     col < igeo->cols; ++col)
		{
			di->x += x_adder + (Position)
				igeo->col_widths[col & col_mask];

			if (col_start == igeo->cols)
			{
				if (di->x >= h_start )
				{
					col_start = col;
					x_start = di->x - (Position)
					   igeo->col_widths[col & col_mask];
				}
			}
			else if (h_end <= di->x)
			{
				col_stop = col;
				if (di->x - (Position)
				   igeo->col_widths[col & col_mask] <= h_end)
				{
					++col_stop;
				}
				break;
			}
		}

		if (col_start == igeo->cols)
		{
			FREE_STACK_ITEM(item,stack);
			return;		/* exposure is to the left or
					 * right of all columns		*/
		}
	}

	do_gravity = (fp->same_width != OL_ALL || fp->same_height != OL_ALL ?
			       True : False);

					/* Now refresh the items	*/

	for (row=0, di->y = igeo->y_offset; row < igeo->rows;
	     di->y += y_adder + (Position)igeo->row_heights[row & row_mask],
	     ++row)
	{
					/* Increment to next row	*/

		di->height = igeo->row_heights[row & row_mask];

		if (v_start > (Position)(di->y + (Position)di->height))
		{
			continue;		/* Skip this row	*/
		}
		else if (v_end < di->y)
		{
			break;			/* all rows are done	*/
		}


					/* Loop through the columns	*/

		for (col=col_start, di->x = x_start; col < col_stop; ++col)
		{

			item_index = (fp->layout_type == OL_FIXEDCOLS ?
					row * igeo->cols + col :
					col * igeo->rows + row);

			if (item_index >= fp->num_items)
			{
				break;		/* go to next row	*/
			}

			di->width = (fp->same_width == OL_ALL ?
					igeo->col_widths[0] :
				 fp->same_width == OL_COLUMNS ?
					igeo->col_widths[col] :
					igeo->col_widths[igeo->cols +
								item_index]);

					/* Use last height if possible	*/

			if (fp->same_height == OL_NONE)
			{
				di->height =
					igeo->row_heights[igeo->rows +
								item_index];
			}

					/* Deal with clipping later	*/

			di->clipped	= False;
			di->clip_width	= di->width;
			di->clip_height	= di->height;

			_OlFlatExpandItem(w, item_index, item);

			if (item->flat.managed == True &&
			    item->flat.mapped_when_managed == True)
			{
			    if (do_gravity == False)
			    {
				_OlFlatDrawItem(w, item, di);
			    }
			    else
			    {
				Position ix;	/* item's x location	*/
				Position iy;	/* item's y location	*/

				_OlDoGravity(fp->item_gravity,
					igeo->col_widths[col & col_mask],
					igeo->row_heights[row & row_mask],
					di->width, di->height, &ix, &iy);

				ix	+= di->x;
				iy	+= di->y;

				ix	^= di->x;	/* Swap the x's	*/
				di->x	^= ix;
				ix	^= di->x;

				iy	^= di->y;	/* Swap the y's	*/
				di->y	^= iy;
				iy	^= di->y;

						/* Note: I'm ignoring the
						 * clip information for now*/

				_OlFlatDrawItem(w, item, di);

						/* Restore x & y	*/

				di->x = ix;
				di->y = iy;
			    }
			}

			di->x += x_adder + (Position)
					igeo->col_widths[col & col_mask];
		}
	}

	FREE_STACK_ITEM(item,stack);

} /* END OF Redisplay() */

/*
 *************************************************************************
 * Resize - this procedure repositions the items within a container after
 * the container has undergone a resize.
 ****************************procedure*header*****************************
 */
static void
Resize(Widget w)
	      	  		/* widget which has undergone a resize	*/
{
			/* The layout doesn't change, just our position	*/

	CalcOffsets(FPART(w), w->core.width, w->core.height);

} /* END OF Resize() */

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
	Boolean	 	redisplay = False;
	FlatPart *	fp = FPART(w);

				/* Set the mnemonic and accelerators	*/

	if (SetAccAndMnem(w, current, new) == True)
	{
		redisplay = True;
	}

#define DIFFERENT(field)	(new->flat.field != current->flat.field)

	if (DIFFERENT(mapped_when_managed) ||
	    DIFFERENT(background_pixel) ||
	    DIFFERENT(background_pixmap) ||
	    DIFFERENT(sensitive)	||
	    DIFFERENT(font_color)	||
	    DIFFERENT(font)		||
	    DIFFERENT(label)		||
	    DIFFERENT(label_justify)	||
	    DIFFERENT(label_image)	||
	    DIFFERENT(label_tile))
	{
		if (DIFFERENT(font))
		{
			CheckFont(w, fp, new);
		}
		redisplay = True;
	}
#undef DIFFERENT
	return(redisplay);
} /* END OF ItemSetValues() */

/*
 *************************************************************************
 * SetValues - this procedure monitors the changing of instance data.
 * Since the subclasses inherit the layout from this class, whenever this
 * class detects a change in any of its layout parameters, it sets a flag
 * in the instance part so that the subclasses merely have to check the
 * flag instead of checking all of the layout parameters.  This design
 * is called.
 ****************************procedure*header*****************************
 */
/* ARGSUSED */
static Boolean
SetValues(Widget current, Widget request, Widget new, ArgList args, Cardinal *num_args)
	      		        	/* What we had			*/
	      		        	/* What we want			*/
	      		    		/* What we get, so far		*/
	       		     
	          	         
{
	FlatWidget	nfw = (FlatWidget)new;
	FlatWidget	cfw = (FlatWidget)current;
	FlatPart *	nfp = FPART(new);
	FlatPart *	cfp = FPART(current);
	Boolean		redisplay = False;
	Boolean		get_new_attrs = False;
	OlStrRep tf = ((FlatWidget)new)->primitive.text_format; 

			/* As the real Flat Widget Class, we can
			 * set this flag to False.			*/

	nfp->flags &= ~OL_B_FLAT_RELAYOUT_HINT;

			/* Update the default item_part fields		*/

#define DIFFERENT(field)	(new->core.field != current->core.field)
#define TRACK_CORE_FIELD(f)\
	if (DIFFERENT(f))\
	{\
		nfp->item_part.f = new->core.f;\
		get_new_attrs = True;\
		redisplay = True;\
	}

	TRACK_CORE_FIELD(background_pixel);
	TRACK_CORE_FIELD(background_pixmap);
	TRACK_CORE_FIELD(ancestor_sensitive);
#undef TRACK_CORE_FIELD

			/* Now that we've updated the item_part's default
			 * background pixel and pixmap, adjust the
			 * container in accordance with it's transparency.
			 */

	if (DIFFERENT(background_pixel) ||
	    DIFFERENT(background_pixmap))
	{
		CALL_TRANSPARENT_PROC(new);
	}
#undef DIFFERENT


		/* The following fields should not be inherited from
		 * their widget container, so make sure they remain
		 * unchanged.						*/

#define KEEP_NEW_SAME_AS_CURRENT(f) \
	if (nfp->item_part.f != cfp->item_part.f)\
	{\
		nfp->item_part.f = cfp->item_part.f;\
	}
	
	KEEP_NEW_SAME_AS_CURRENT(sensitive);
	KEEP_NEW_SAME_AS_CURRENT(accelerator);
	KEEP_NEW_SAME_AS_CURRENT(qualifier_text);
	KEEP_NEW_SAME_AS_CURRENT(meta_key);
	KEEP_NEW_SAME_AS_CURRENT(accelerator_text);
	KEEP_NEW_SAME_AS_CURRENT(mnemonic);
#undef KEEP_NEW_SAME_AS_CURRENT

		/* If the font in the primitive's superclass changed,
		 * stick it's new value into default item_part.
		 * Remember, if the new font is NULL, use the one
		 * in the screen manager.
		 * Also, since the new font might be a different size,
		 * suggest a relayout be done.				*/

	if (PWIDGET(new, font) != PWIDGET(current, font))
	{
			/* If the container has no font, use the one
			 * in the screen cache.				*/

		if (PWIDGET(new, font) == (OlFont)NULL)
		{
			PWIDGET(new, font) = _OlFlatScreenManager(new,
#ifdef sun
						nfw->primitive.scale,
#else
						OL_DEFAULT_POINT_SIZE,
#endif
						OL_JUST_LOOKING)->font;
		}

		nfp->item_part.font = PWIDGET(new, font);
		nfp->flags |= OL_B_FLAT_RELAYOUT_HINT;
		redisplay = True;
		get_new_attrs = True;
	}

		/* The following fields exist in the Primitive superclass.
		 * If their values change, stick them into the default
		 * item_part and suggest a relayout and possibly get
		 * get new attrs.					*/

#define TRACK_PRIMITIVE_FIELD(f,redisplay_val,attr_val)\
	if (PWIDGET(new, f) != PWIDGET(current, f))\
	{\
		nfp->item_part.f = PWIDGET(new, f);\
		redisplay = redisplay_val;\
		get_new_attrs = attr_val;\
	}

	TRACK_PRIMITIVE_FIELD(font_color,True,True);
	TRACK_PRIMITIVE_FIELD(foreground,True,True);
	TRACK_PRIMITIVE_FIELD(input_focus_color,True,get_new_attrs);
	TRACK_PRIMITIVE_FIELD(traversal_on,redisplay,get_new_attrs);
	TRACK_PRIMITIVE_FIELD(user_data,redisplay,get_new_attrs);
#undef TRACK_PRIMITIVE_FIELD

#define DIFFERENT(field)	(nfp->field != cfp->field)

	if (DIFFERENT(item_part.label))
	{
		redisplay = True;

		if (nfp->item_part.label != (OlStr)NULL) {
			OlStr temp;

			temp = (OlStr)XtMalloc(
				(*str_methods[tf].StrNumBytes)(
					nfp->item_part.label));
			(*str_methods[tf].StrCpy)(
					temp,nfp->item_part.label);
			nfp->item_part.label = temp;	
		}

		if (cfp->item_part.label != (OlStr)NULL)
		{
			XtFree((char *)cfp->item_part.label);
			cfp->item_part.label = (OlStr)NULL;
		}

		nfp->flags |= OL_B_FLAT_RELAYOUT_HINT;
	}

	/* We'll try to do what we can in case of SetValues() on the
	 * "scale" resource. Note that we cannot support SetVals on
	 * this resource completly - as we cannot adjust the
	 * h_space & v_space resources accordingly ... due to OL_IGNORE
	 * being overwritten during Initialize() ...  -JMK
	 */
	if (nfw->primitive.scale != cfw->primitive.scale) {
	  	redisplay = True;
	    	nfp->flags |= OL_B_FLAT_RELAYOUT_HINT;
	      	nfp->items_touched = (Boolean)True;
		get_new_attrs = (Boolean)True;
	}

	if (XtIsSensitive(new) != XtIsSensitive(current) ||
	    DIFFERENT(item_part.label_tile) ||
	    DIFFERENT(item_part.label_image) ||
	    DIFFERENT(item_part.label_justify))
	{
		redisplay = True;
	}

	if (DIFFERENT(item_fields) ||
	    DIFFERENT(num_item_fields) ||
	    DIFFERENT(items) ||
	    DIFFERENT(num_items))
	{
		nfp->items_touched = (Boolean)True;
	}
#undef DIFFERENT

		/* If one of the parameters has changed, check the
		 * validity of the parameters.  Then, if a parameter
		 * is still different, suggest a relayout		*/

#define DIFFERENT(field)	(nfp->field != cfp->field)

#define PARAMETERS_HAVE_CHANGED \
	(DIFFERENT(gravity)		||\
	DIFFERENT(h_pad)		||\
	DIFFERENT(h_space)		||\
	DIFFERENT(item_gravity)		||\
	DIFFERENT(item_max_height)	||\
	DIFFERENT(item_max_width)	||\
	DIFFERENT(item_min_height)	||\
	DIFFERENT(item_min_width)	||\
	DIFFERENT(layout_height)	||\
	DIFFERENT(layout_type)		||\
	DIFFERENT(layout_width)		||\
	DIFFERENT(measure)		||\
	DIFFERENT(same_height)		||\
	DIFFERENT(same_width)		||\
	DIFFERENT(v_pad)		||\
	DIFFERENT(v_space))

	if (PARAMETERS_HAVE_CHANGED)
	{
		CheckLayoutParameters(current, cfp, new, nfp);

		/* Check the parameters again.  This allows us to avoid
		 * a relayout in the case when the new value was invalid
		 * and the widget set the value back to the current
		 * value.						*/

		if (PARAMETERS_HAVE_CHANGED)
		{
			nfp->flags |= OL_B_FLAT_RELAYOUT_HINT;
		}
	}
#undef PARAMETERS_HAVE_CHANGED
#undef DIFFERENT

	if (get_new_attrs == True)
	{
		GetGCs(new, nfp);
	}

			/* Call the convenience routine to 
			 * handle the sub-objects and do the layout
			 * if necessary.				*/

	if (_OlFlatCheckItems(flatWidgetClass, current, request, new,
			args, num_args) == True)
	{
		redisplay = True;
	}

	return(redisplay);
} /* END OF SetValues() */

/*
 *************************************************************************
 * TransparentProc - this routine maintains the "transparency" of the
 * flat widget container.   It is called whenever the parent of the flat
 * widget has its background changed via a SetValues.  It is also
 * called if the flat widget's background is updated.
 ****************************procedure*header*****************************
 */
static void
TransparentProc (Widget w, Pixel pixel, Pixmap pixmap)
{
			/* If the container's background should be
			 * transparent, get it from it's parent		*/

	if (FCPART(w)->transparent_bg == True)
	{
		unsigned long	mask = (unsigned long)0;

		if (w->core.background_pixel != pixel)
		{
			mask |= CWBackPixel;
			w->core.background_pixel = pixel;
		}

		if (w->core.background_pixmap != pixmap)
		{
			if (pixmap == XtUnspecifiedPixmap)
			{
				/* If the parent now has an unspecified
				 * pixmap, use the parents background
				 * pixel color and reset our pixmap	*/

				mask |= CWBackPixel;
				w->core.background_pixmap = pixmap;
			}
			else
			{
				mask |= CWBackPixmap;
				w->core.background_pixmap = ParentRelative;
			}
		}

		if (mask && XtIsRealized(w) == (Boolean)True)
		{
			XSetWindowAttributes	values;

			values.background_pixmap = w->core.background_pixmap;
			values.background_pixel = w->core.background_pixel;

			XChangeWindowAttributes(XtDisplay(w),
					XtWindow(w), mask, &values);
			_OlClearWidget(w, True);
		}
	}
} /* END OF TransparentProc() */

/*
 *************************************************************************
 * TraversalHandler - this routine is the external interface for moving
 * focus among flattened widget sub-objects.  This routine calls the
 * a flat class procedure to do the actual focus movement.
 ****************************procedure*header*****************************
 */
/* ARGSUSED */
static Widget
TraversalHandler (Widget w, Widget ignore, OlVirtualName dir, Time time)
{
	FlatPart *	fp = FPART(w);

	if (fp->focus_item != (Cardinal)OL_NO_ITEM)
	{
		(void)_OlFlatTraverseItems(w, fp->focus_item, dir, time);
	}

	return((fp->focus_item != (Cardinal)OL_NO_ITEM ? w : NULL));
} /* END OF TraversalHandler() */

/*
 *************************************************************************
 * TraversalItems - this routine sets focus sub-objects.  When this
 * routine is called, the FlatWidget already has the input focus
 ****************************procedure*header*****************************
 */
static Cardinal
TraverseItems (Widget w, Cardinal sfi, OlVirtualName dir, Time time)
{
	FlatPart *	fp = FPART(w);
	FlatClassPart *	fcp = FCPART(w);
	ALLOC_STACK_ITEM(fcp, item, stack);
	Cardinal	start_pos;
	Cardinal	i;			/* current position	*/
	Cardinal	array[LOCAL_SIZE];
	Cardinal *	roci;		/* row or column items		*/
	Cardinal	num;		/* # of items to take focus	*/
	Cardinal	multi;
	Cardinal	new_focus_item;

	new_focus_item = (Cardinal)OL_NO_ITEM;


					/* Set the starting item	*/
	switch(dir) {
	case OL_MOVERIGHT:	/* FALLTHROUGH */
	case OL_MOVEDOWN:
		start_pos = GetFocusItems(fp, sfi, (dir == OL_MOVERIGHT),
					&num, &roci, array, LOCAL_SIZE);
		i = (start_pos + 1) % num;
		break;
	case OL_MOVELEFT:	/* FALLTHROUGH */
	case OL_MOVEUP:
		start_pos = GetFocusItems(fp, sfi, (dir == OL_MOVELEFT),
					&num, &roci, array, LOCAL_SIZE);
		i = (start_pos == 0 ? num : start_pos) - 1;
		break;
	case OL_MULTIRIGHT:
		dir = OL_MOVERIGHT;
		/* FALLTHROUGH */
	case OL_MULTIDOWN:
		if (dir == OL_MULTIDOWN) {
			dir = OL_MOVEDOWN;
		}
		multi = _OlGetMultiObjectCount(w);
		start_pos = GetFocusItems(fp, sfi, (dir == OL_MOVERIGHT),
					&num, &roci, array, LOCAL_SIZE);
		i = ((start_pos + multi) >= num ? 0 : (start_pos + multi));
		break;
	case OL_MULTILEFT:
		dir = OL_MOVELEFT;
		/* FALLTHROUGH */
	case OL_MULTIUP:
		if (dir == OL_MULTIUP) {
			dir = OL_MOVEUP;
		}
		multi = _OlGetMultiObjectCount(w);
		start_pos = GetFocusItems(fp, sfi, (dir == OL_MOVELEFT),
					&num, &roci, array, LOCAL_SIZE);
		i = (start_pos < multi ? (num - 1) : (start_pos - multi));
		break;
	default:
		FREE_STACK_ITEM(item,stack);
		return (new_focus_item);
	}

	do {
		_OlFlatExpandItem(w, roci[i], item);

		if (_OlFlatItemAcceptFocus(w, item, time) == True)
		{
			new_focus_item = item->flat.item_index;
			break;
		}

		switch(dir){
		case OL_MOVEUP:
		case OL_MOVELEFT:
			if (i == 0)
				i = num - 1;
			else
				--i;
			break;
		default:	/* OL_MOVERIGHT	&& OL_MOVEDOWN	*/
			if (i == (num-1))
				i = 0;
			else
				++i;
			break;
		}

	} while (i != start_pos);

					/* Free the array if necessary	*/
	if (roci != array)
	{
		XtFree((char *)roci);
	}

	FREE_STACK_ITEM(item,stack);

	UpdateSuperCaret((FlatWidget)w);
	return(new_focus_item);
} /* END OF TraverseItems() */

/*
 *************************************************************************
 * ItemQuerySCLocnProc: handles the positioning of the supercaret
 * over the current flat focus item.
 ****************************procedure*header*****************************
 */
static void
ItemQuerySCLocnProc(const   Widget          flat,
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
	SuperCaretShape		rs = *shape;
        OlFlatDrawInfo          fidi;
 
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
                return; /* try again */
        }
 
        _OlFlatGetDrawInfo(flat, fp->focus_item, &fidi);
 
        *x_center = fidi.x;
        *y_center = fidi.y;
 
        switch (*shape) {
                case SuperCaretBottom:
                        *x_center += fidi.width / 2;
                        *y_center += fidi.height;
                        break;
 
                case SuperCaretLeft:
                	*y_center += fidi.height / 2;
                break;
        }
}

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
