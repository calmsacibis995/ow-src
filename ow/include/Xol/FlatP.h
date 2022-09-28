#ifndef	_XOL_FLATP_H
#define	_XOL_FLATP_H

#pragma	ident	"@(#)FlatP.h	302.8	93/04/23 include/Xol SMI"	/* flat:FlatP.h 1.22 	*/

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


#include <Xol/Flat.h>
#include <Xol/OlgxP.h>
#include <Xol/OpenLook.h>
#include <Xol/PrimitiveP.h>

#include <X11/Intrinsic.h>


#ifdef	__cplusplus
extern "C" {
#endif


/*************************************************************************	
 * Define structures and names/types needed to support flat widgets
 *************************************************************************/

#if	defined(__STDC__) || defined(__cplusplus)

typedef Boolean		(*OlFlatReqRscPredicateFunc)(
	Widget		w,		/* flattened widget id */
	Cardinal	offset,		/* resource offset */
	String		name		/* require resource name */
);

#else	/* __STDC__ || __cplusplus */

typedef Boolean		(*OlFlatReqRscPredicateFunc)();

#endif	/* __STDC__ || __cplusplus */

typedef struct {
	String				name;		/* Resource name */
	OlFlatReqRscPredicateFunc	predicate;	/* Use it? */
	Cardinal			rsc_index;	/* private */
}			OlFlatReqRsc, *OlFlatReqRscList;

/* 
 * Define a structure that's used to extract information from the
 * application's list and store indexes of required resources. 
 */
typedef struct {
	Cardinal*		xlist; 		/* resource extractor list */
	Cardinal		num_xlist;	/* number of xlist elements */
	Cardinal*		rlist;		/* required resource extractor
						   list */
	Cardinal		num_rlist;	/* number of rlist elements */
	char*			rdata;		/* place to hold data */
}			OlFlatResourceInfo;

/*
 * Create a structure for holding transient geometry information.
 */
typedef struct {
	Cardinal	rows;		/* number of rows */
	Cardinal	cols;		/* number of columns */
	Position	x_offset;	/* x coordinate of first item */
	Position	y_offset;	/* y coordinate of first item */
	Dimension	bounding_width;	/* width tightly bounding items */
	Dimension	bounding_height;/* height tightly bounding items */
	Dimension*	col_widths;	/* array of column widths */
	Dimension*	row_heights;	/* array of row heights */
}			OlFlatGeometry;

/* Define a structure used to Draw Sub-objects */
typedef struct {
	Drawable	drawable;	/* place to draw */
	Screen*		screen;		/* drawable's screen */
	Pixel		background;	/* drawable's backgrnd */
	Position	x;		/* item's X coordinate */
	Position	y;		/* item's Y coordinate */
	Dimension	width;		/* item's width */
	Dimension	height;		/* item's height */
	Boolean		clipped;	/* is it clipped? */
	Dimension	clip_width;	/* clip width */
	Dimension	clip_height;	/* clip height */
} OlFlatDrawInfo;

/* Define a structure to cache information on a per screen basis */
typedef struct _OlFlatScreenCache {
	Screen*			screen;		/* screen id */
	Colormap		cmap;		/* colormap */
	struct _OlFlatScreenCache* next;	/* next node in list */
	Cardinal		count;		/* reference count */
	OlFont			font;		/* OLIT Font */
	OlgxAttrs*		alt_attrs;	/* scratch attributes */
	Pixel			alt_bg;		/* scratch attrs bg */
	Pixel			alt_fg;		/* scratch attrs fg */
	Dimension		check_width;	/* points cvted to pixels */
	Dimension		check_height;	/* points cvted to pixels */
} OlFlatScreenCache, *OlFlatScreenCacheList;

#define OL_JUST_LOOKING	1		/* OlFlatScreenManager flag */
#define OL_ADD_REF	2		/* OlFlatScreenManager flag */
#define OL_DELETE_REF	3		/* OlFlatScreenManager flag */

typedef union {
    OlgxTextLbl		text;
    OlgxPixmapLbl	pixmap;
} FlatLabel;

/*************************************************************************	
 * Define Expanded Sub-object Instance Structure
 *************************************************************************/

typedef struct {
	OlDefine	label_justify;	/* justification of the labels */
	Boolean		managed;	/* is this item managed? */
	Boolean		mapped_when_managed; /* mapped when managed? */
	Boolean		sensitive;	/* is this item sensitive? */
	Boolean		ancestor_sensitive; /* ancestor sensitive? */
	Boolean		label_tile;	/* tile the label? */
	Boolean		traversal_on;	/* is this item traversable? */
	OlMnemonic	mnemonic;	/* mnemonic */
	Pixmap		background_pixmap; /* Background pixmap */
	Pixel		background_pixel;/* Background color */
	Pixel		foreground;	/* foreground */
	Pixel		font_color;	/* font color */
	Pixel		input_focus_color;
	XtPointer	user_data;	/* application data hook	*/
	String		accelerator;	/* accelerator binding		*/
	String		qualifier_text;
	Boolean		meta_key;
	String		accelerator_text; /* accelerator display string	*/
	OlStr		label;		/* textual label		*/
	OlFont		font;
	OlStrRep	text_format;
	XImage *	label_image;	/* Image label			*/
	Cardinal	item_index;	/* the index of this item	*/
} FlatItemPart;

/* Item's Full Instance record declaration */
typedef struct {
	FlatItemPart	flat;
} FlatItemRec, *FlatItem;

/*************************************************************************	
 * Define Widget Instance Structure
 *************************************************************************/

/* Define new fields for the instance part */
typedef struct _FlatPart {
	/* layout resources */
	Dimension	h_pad;		/* horizontal margin padding */
	Dimension	h_space;	/* internal horizontal padding */
	Dimension	item_max_height;/* item's maximum allow. height */
	Dimension	item_max_width;	/* item's maximum allow. width */
	Dimension	item_min_height;/* item's minimum allow. height */
	Dimension	item_min_width;	/* item's minimum allow. width */
	Dimension	overlap;	/* overlap of each item */
	Dimension	v_pad;		/* vertical margin padding */
	Dimension	v_space;	/* internal vertical padding */
	int		gravity;	/* gravity for sub-object group */
	int		item_gravity;	/* gravity for each sub-object */
	int		measure;	/* layout related dimension */
	OlDefine	layout_height;	/* height boundary constraint */
	OlDefine	layout_type;	/* type of desired layout */
	OlDefine	layout_width;	/* width boundary constraint */
	OlDefine	same_height;	/* forcing of sub-object height */
	OlDefine	same_width;	/* forcing of sub-object width */

	/* Sub-object related resources */
	XtPointer	items;		/* sub-object list */
	String*		item_fields;	/* array of fields in item list */
	Cardinal	num_item_fields;/* Number of fields per item */
	Cardinal	num_items;	/* number of sub-object items */
	Cardinal	focus_item;	/* item with focus */
	Cardinal	last_focus_item;/* last item to have focus */
	OlFlatResourceInfo resource_info; /* resource information */
	OlFlatGeometry	item_geom;	/* calculated item geometry */
	FlatItemPart	item_part;	/* default item attributes */
	Boolean		items_touched;	/* application touched items */
	unsigned char	flags;		/* flags used to maintain state */
	struct _acc_text {
	    String	qual;
	    Boolean	meta;
	    String	acc;
	} *acc_text;			/* list of accelerator text */
	OlgxAttrs*	pAttrs;

#	define	OL_B_FLAT_RELAYOUT_HINT		(1<<0)	/* need a relayout */
#	define	OL_B_FLAT_ACCELERATOR_TEXT	(1<<1)	/* internal acc. text. */

} FlatPart;

/* Full instance record declaration */
typedef struct _FlatRec {
	CorePart		core;
	PrimitivePart		primitive;
	FlatPart		flat;
}			FlatRec;

/* 
 *Define new function pointers for the flat widget class.
 */

#if	defined(__STDC__) || defined(__cplusplus)

typedef void	(*OlFlatAnalyzeItemsProc)(
	Widget		w, 		/* container widget id */
	ArgList		args,		/* args that created items */
	Cardinal*	num_args	/* number of args */
);

typedef void	(*OlFlatDrawItemProc)(
	Widget		w,		/* container widget id */
	FlatItem	item,		/* expanded item */
	OlFlatDrawInfo*	draw_info	/* Information used in drawing */
);

typedef void	(*OlFlatExpandItemProc)(
	Widget		w,		/* flat widget container */
	FlatItem	item		/* expanded item */
);

typedef void (*OlFlatGetDrawInfoProc)(
	Widget		w,		/* Flat widget type */
	Cardinal	item_index,	/* Item to get drawing info. for */
	OlFlatDrawInfo*	draw_info	/* Information used in drawing */
);

typedef Cardinal	(*OlFlatGetIndexFunc)(
	Widget		w,		/* Flat widget type */
	Position	x,		/* X source location */
	Position	y,		/* Y source location */
	Boolean		ignore_sensitivity	/* don't look at sensitivity*/
);

typedef Boolean	(*OlFlatItemAcceptFocusFunc)(
	Widget		w, 		/* container widget id */
	FlatItem	item,		/* item to accept focus */
	Time		time		/* time when request was made */
);

typedef Boolean	(*OlFlatItemActivateFunc)(
	Widget		w,		/* container widget id */
	FlatItem	item,		/* item to activate */
	OlVirtualName	type,		/* activation type */
	XtPointer	data		/* activation data */
);

typedef void	(*OlFlatItemDimensionsProc)(
	Widget		w,		/* Flat widget container id */
	FlatItem	item, 		/* expanded item */
	Dimension*	width,		/* returned width */
	Dimension*	height		/* returned height */
);

typedef void	(*OlFlatItemGetValuesProc)(
	Widget		widget, 	/* flat widget container */
	FlatItem	item,		/* expanded item */
	ArgList		args,		/* item Args */
	Cardinal*	num_args	/* num item Args */
);

typedef void	(*OlFlatItemHighlightProc)(
	Widget		w,		/* flat widget id */
	FlatItem	item,		/* item being affected */
	OlDefine	type		/* highlight type */
);

typedef void	(*OlFlatItemInitializeProc)(
	Widget		w,		/* Flat widget container id */
	FlatItem	request,	/* expanded requested item */
	FlatItem	c_new,		/* expanded new item */
	ArgList		args,		/* args that created items */
	Cardinal*	num_args	/* number of args */
);

typedef Boolean	(*OlFlatItemSetValuesFunc)(
	Widget		w,		/* flat widget container */
	FlatItem	current,	/* item application has */
	FlatItem	request,	/* item application wants */
	FlatItem	c_new,		/* item application gets */
	ArgList		args,		/* item Args */
	Cardinal*	num_args	/* num item Args */
);

typedef void	(*OlFlatItemsTouchedProc)(
	Widget		current,	/* current flat widget or NULL */
	Widget		request,	/* request flat widget */
	Widget		c_new,		/* new flat widget */
	ArgList		args,		/* item Args */
	Cardinal*	num_args	/* num item Args */
);

typedef void	(*OlFlatLayoutProc)(
	Widget		w,		/* Flat widget container id */
	Dimension*	return_width,	/* returned container width */
	Dimension*	return_height	/* returned container height */
);

typedef Cardinal	(*OlFlatTraverseItemsFunc)(
	Widget		w,		/* flat widget container */
	Cardinal	item_index,	/* item index to start at */
	OlVirtualName	direction,	/* direction to move */
	Time		time		/* the time */
);

#else	/* __STDC__ || __cplusplus */

typedef void		(*OlFlatAnalyzeItemsProc)();
typedef void		(*OlFlatDrawItemProc)();
typedef void		(*OlFlatExpandItemProc)();
typedef void		(*OlFlatGetDrawInfoProc)();
typedef Cardinal	(*OlFlatGetIndexFunc)();
typedef Boolean		(*OlFlatItemAcceptFocusFunc)();
typedef Boolean		(*OlFlatItemActivateFunc)();
typedef void		(*OlFlatItemDimensionsProc)();
typedef void		(*OlFlatItemGetValuesProc)();
typedef void		(*OlFlatItemHighlightProc)();
typedef void		(*OlFlatItemInitializeProc)();
typedef Boolean		(*OlFlatItemSetValuesFunc)();
typedef void		(*OlFlatItemsTouchedProc)();
typedef void		(*OlFlatLayoutProc)();
typedef Cardinal	(*OlFlatTraverseItemsFunc)();

#endif	/* __STDC__ || __cplusplus */


/* Define inheritance procedures for the class */

#define XtInheritFlatDrawItem		((OlFlatDrawItemProc)_XtInherit)
#define XtInheritFlatGetDrawInfo	((OlFlatGetDrawInfoProc)_XtInherit)
#define XtInheritFlatGetIndex		((OlFlatGetIndexFunc)_XtInherit)
#define XtInheritFlatItemAcceptFocus	((OlFlatItemAcceptFocusFunc)_XtInherit)
#define XtInheritFlatItemActivate	((OlFlatItemActivateFunc)_XtInherit)
#define XtInheritFlatItemHighlight	((OlFlatItemHighlightProc)_XtInherit)
#define XtInheritFlatItemDimensions	((OlFlatItemDimensionsProc)_XtInherit)
#define XtInheritFlatLayout		((OlFlatLayoutProc)_XtInherit)
#define XtInheritFlatLayoutClass	((WidgetClass)_XtInherit)
#define OlThisFlatClass			((WidgetClass)OL_IGNORE)
#define XtInheritFlatTraverseItems	((OlFlatTraverseItemsFunc)_XtInherit)

#define OL_FLATCLASS(w,field) \
			(((FlatWidgetClass)XtClass(w))->flat_class.field)

#define _OlFlatDrawItem(w,ei,d)		(*OL_FLATCLASS(w,draw_item))(w,ei,d)
#define _OlFlatGetDrawInfo(w,i,d)	(*OL_FLATCLASS(w,get_draw_info))(w,i,d)
#define _OlFlatGetIndex(w,x,y,f)	(*OL_FLATCLASS(w,get_index))(w,x,y,f)
#define _OlFlatItemActivate(w,i,t,d) \
				(*OL_FLATCLASS(w,item_activate))(w,i,t,d)
#define _OlFlatItemAcceptFocus(w,i,t) \
				(*OL_FLATCLASS(w,item_accept_focus))(w,i,t)
#define _OlFlatItemDimensions(w,ei,iw,ih) \
			(*OL_FLATCLASS(w,item_dimensions))(w,ei,iw,ih)
#define _OlFlatItemHighlight(w,i,t)   (*OL_FLATCLASS(w,item_highlight))(w,i,t)
#define _OlFlatLayout(w,x,y)	      (*OL_FLATCLASS(w,layout))(w,x,y)
#define _OlFlatTraverseItems(w,i,d,t) (*OL_FLATCLASS(w,traverse_items))(w,i,d,t)

/*************************************************************************	
 * Define Widget Class Part and Class Rec
 *************************************************************************/

/* Define new fields for the class part */
typedef struct _FlatClassPart {
    XtResourceList		item_resources;	/* sub-object resources */
    Cardinal			num_item_resources;	/* number of item rsc */
    OlFlatReqRscList		required_resources;	/* manditory item rscs */
    Cardinal			num_required_resources;
    XrmQuarkList		quarked_items;	/* quarked names */
    Cardinal			part_size;	/* this item's part size*/
    Cardinal			part_offset;	/* default part in widget*/
    Cardinal			part_in_rec_offset;/* part's record offset*/
    Cardinal			rec_size;	/* Full Record size */
    Boolean			transparent_bg;	/* inherit parent's bg?*/
    OlFlatAnalyzeItemsProc	analyze_items;	/* check all items */
    OlFlatDrawItemProc		draw_item;	/* subclass's draw proc.*/
    OlFlatExpandItemProc	expand_item;	/* supplemental expansion*/
    OlFlatGetDrawInfoProc	get_draw_info;	/* returns drawing info */
    OlFlatGetIndexFunc		get_index;	/* gets an item's index */
    OlFlatItemAcceptFocusFunc	item_accept_focus; /* sets focus to items*/
    OlFlatItemActivateFunc	item_activate;	/* activates an item */
    OlFlatItemDimensionsProc	item_dimensions;/* item's width/height */
    OlFlatItemGetValuesProc	item_get_values;/* queries an item */
    OlFlatItemHighlightProc	item_highlight;	/* highlights an item */
    OlFlatItemInitializeProc	item_initialize;/* checking a new item */
    OlFlatItemSetValuesFunc	item_set_values;/* updates an item */
    OlFlatItemsTouchedProc	items_touched;	/* for new lists */
    WidgetClass			layout_class;	/* Flat to do layout */
    OlFlatLayoutProc		layout;		/* sub-object layout */
    OlFlatTraverseItemsFunc	traverse_items;	/* traverses items */
    XtPointer			reserved2;	/* GeometryManager */
    XtPointer			reserved1;	/* ChangedManaged */
    XtPointer			extension;	/* future extensions */
} FlatClassPart;

/* Full class record declaration */
typedef struct _FlatClassRec {
	CoreClassPart		core_class;
	PrimitiveClassPart	primitive_class;
	FlatClassPart		flat_class;
}			FlatClassRec;

/* External class record declaration */
extern FlatClassRec		flatClassRec;


/**************************************************************************	
 * Declare external routines and macros used by the flat widgets.
 * External routines exist for chained class procedures only.
 * Macros exist for non-chained class procedures, they provide a
 * way of checking the number of parameters when using a pointers
 * to functions.
 *************************************************************************/

/*
 * FlatCvt module
 */

#if	defined(__STDC__) || defined(__cplusplus)

extern void		_OlFlatAddConverters(void);

#else	/* __STDC__ || __cplusplus */

extern void		_OlFlatAddConverters();

#endif	/* __STDC__ || __cplusplus */


/*
 * FlatExpand module
 */

#if	defined(__STDC__) || defined(__cplusplus)

extern void		_OlDoGravity(
	int		gravity,	/* the desired gravity */
	Dimension	c_width,	/* container's width */
	Dimension	c_height,	/* container's height */
	Dimension	width,		/* interior item width */
	Dimension	height,		/* interior item height */
	Position*	x,		/* returned x offset */
	Position*	y		/* returned y offset */
);

extern Boolean		_OlFlatCheckItems(
	WidgetClass	wc,		/* subclass's id */
	Widget		current,	/* current flat widget or NULL */
	Widget		request,	/* request flat widget */
	Widget		c_new,		/* new flat widget */
	ArgList		args,		/* args that created items */
	Cardinal*	num_args	/* number of args */
);

extern void		_OlFlatPreviewItem(
	Widget		w,		/* container widget id */
	Cardinal	item_index,	/* Item to draw */
	Widget		preview,	/* destination preview widget */
	Cardinal 	preview_item	/* sub-object within preview widget */
);

extern void		_OlFlatRefreshItem(
	Widget		w,		/* container widget id */
	Cardinal	item_index,	/* Item to draw */
	Boolean		clear_area	/* should the area be cleared? */
);

extern OlFlatScreenCache*
			_OlFlatScreenManager(
	Widget		w,		/* any widget id */
	Cardinal	point_size,	/* point size of information */
	OlDefine	flag		/* OlFlatScreenManager flag */
);

extern void		_OlFlatSetupAttributes(
	Widget		w,		/* flat widget container */
	FlatItem	item,		/* expanded item */
	OlFlatDrawInfo*	di,		/* drawing information */
	OlgxAttrs**	ppAttrs,	/* returned pointer to attributes */
	FlatLabel**	ppLbl,		/* returned pointer to label */
	OlDrawProc*     ppDrawProc     /* returned ptr to lbl drawer */
);

extern void		_OlFlatSetupLabelSize(
	Widget		w,		/* Widget making request */
	FlatItem	item,		/* expanded item */
	FlatLabel**	ppLbl,		/* pointer to label structure */
	OlSizeProc*	ppSizeProc      /* pointer to size procedure */
);

#else	/* __STDC__ || __cplusplus */

extern void		_OlDoGravity();
extern Boolean		_OlFlatCheckItems();
extern void		_OlFlatPreviewItem();
extern void		_OlFlatRefreshItem();
extern OlFlatScreenCache* _OlFlatScreenManager();
extern void		_OlFlatSetupAttributes);
extern void		_OlFlatSetupLabelSize();

#endif	/* __STDC__ || __cplusplus */


/*
 * FlatState module
 */

#if	defined(__STDC__) || defined(__cplusplus)

extern void		_OlFlatExpandItem(
	Widget		w,		/* flat widget container */
	Cardinal	item_index,	/* item to be expanded */
	FlatItem	item		/* memory to expand into */
);

extern void		_OlFlatInitializeItems(
	Widget		c_w,		/* current flat widget or NULL */
	Widget		r_w,		/* request flat widget */
	Widget		w,		/* new flat widget */
	ArgList		args,		/* container Args */
	Cardinal*	num_args	/* num container Args */
);

#else	/* __STDC__ || __cplusplus */

extern void		_OlFlatExpandItem();
extern void		_OlFlatInitializeItems();

#endif	/* __STDC__ || __cplusplus */


#ifdef	__cplusplus
}
#endif


#endif	/* _XOL_FLATP_H */
