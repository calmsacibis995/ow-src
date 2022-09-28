#ifndef	_XOL_LISTPANEP_H
#define	_XOL_LISTPANEP_H

#pragma	ident	"@(#)ListPaneP.h	302.4	92/10/05 include/Xol SMI"	/* scrolllist:include/Xol/ListPaneP.h 1.29 	*/

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

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#include <Xol/ListPane.h>		/* include public header file */
#include <Xol/OlgxP.h>			/* drawing pkg header */
#include <Xol/PrimitiveP.h>		/* include superclasses's header */
#include <Xol/ScrollingL.h>		/* container's header file */
#include <Xol/array.h>			/* for list-offset array */

#include <X11/CoreP.h>
#include <X11/Intrinsic.h>


#ifdef	__cplusplus
extern "C" {
#endif


/* dynamic resource bit masks */
#define OL_B_LISTPANE_BG			(1 << 0)
#define OL_B_LISTPANE_FONTCOLOR			(1 << 1)


typedef struct _OlHeadRec {
	int		offset;		/* offset of head in internal list*/
	_OlArrayRec	offsets;	/* offsets of items & item count */
}			*_OlHead;

#define ABS_DELTA(x1, x2)   (x1 < x2 ? x2 - x1 : x1 - x2)

/*
 *	Class structure
 */

/* New fields for the ListPane widget class record */
typedef struct {
	int			no_class_fields;	/* make compiler happy */
}			ListPaneClassPart;

/* Full class record declaration */
typedef struct _ListPaneClassRec {
	CoreClassPart		core_class;
	PrimitiveClassPart	primitive_class;
	ListPaneClassPart	list_pane_class;
}			ListPaneClassRec;

/* Class record variable */
externalref ListPaneClassRec	listPaneClassRec;


/*
 *	Instance (widget) structure
 */

/* New fields for the ListPane widget record */
typedef struct {

	/* "PUBLIC" (resource) members */
	OlApplAddItemProc	applAddItem;
	OlApplDeleteItemProc	applDeleteItem;
	OlApplEditCloseProc	applEditClose;
	OlApplEditOpenProc	applEditOpen;
	OlApplTouchItemProc	applTouchItem;
	OlApplUpdateViewProc	applUpdateView;
	OlApplViewItemProc	applViewItem;

	Boolean		recompute_width;/* resize or live with geometry */
	Boolean		selectable;
	Cardinal	view_height;	/* # of items in view */

	/* 
	 * The following 2 resources have been added to allow the app to specify
	 * preferred max and min widths of the ScrollingList.  This gives the 
	 * application much needed flexibility in adjusting the ScrollingList to
	 * better fit into a particular space. If these are not set to a positive
	 * value by the application, then they default to 0 and are ignored.
	 * (hence original static sizing behavior is preserved).
	 */
	Dimension	pref_max_width;	/* application's preferred max width*/
	Dimension	pref_min_width;	/* application's preferred min width*/
	Dimension   space;          /* space between text & glyph */
	Dimension   item_height;    /* (constant)height of each item */
	Boolean     align;          /* align second component */
	OlDefine    position;       /* position of string w.r.t glyph */
	OlDefine	slist_mode;     /* Mode ? exlc/nonexcl/... */

	/* PRIVATE members */
	Cardinal	actualViewHeight;	/* (actual) # of items in view */
	OlgxAttrs*	attr_focus;	    /* drawing attrs for focus_item */
	OlgxAttrs*	attr_normal;	/* drawing attrs */
	unsigned char	dyn_flags;	/* dynamic resources dirty bits */
	Boolean		editing;        /* item being editted (?) */
	int		focus_item;	        /* list offset of item with "focus" */
	GC		gc_inverted;
	GC		gc_normal;
	struct _OlHeadRec	head;	/* head of list */
	int		initial_motion;	    /* direction of initial motion */
	Cardinal	items_selected;	/* number of items selected */
	Dimension	max_height;     /* height of tallest item (in pixels) */
	Dimension	max_width;      /* width of widest item (in pixels) */
	Boolean		own_clipboard;  /* CLIPBOARD ownership */
	Boolean		own_selection;  /* selection ownership */
	int		prev_index;         /* for motion: previous index */
	unsigned long	repeat_rate;/* for auto scolling */
	int		scroll;             /* pointer w/i pane for auto scroll */
	int		search_item;        /* item last searched from keyboard */
	int		start_index;        /* for motion: start index */
	int		top_item;           /* list offset of top item in view */
	OlSlistItemPtr first_item;  /* first item in view */
	OlSlistItemPtr last_item;   /* last item in view */
	OlSlistItemPtr *scrolling_list_items; /* list of all items in widget */
	OlSlistItemPtr *viewable_items; /* list of viewable items in widget */
	int 	num_items;          /* total number of items in Slist */
	int		current_item;      /* last current item */
	int		*current_items;    /* for non-exclusive - all current items */
	int		num_current_items; /* number of current Items */
	Time	last_click_time;	/* next four fields are for double-click */
	int 	last_x_root;
	int 	last_y_root;
	Window  last_root;
	Widget		text_field;     /* editable text field */
	XtIntervalId	timer_id;	/* timer for auto-scroll */
	Boolean		update_view;	/* view is locked/unlocked */
    
	/* 
	 *item spacing: these are pixel dimensions derived from device-
	 * independent dimensions.  they are fixed so are calculated once.
	 */
	/* margin between top (bottom) of * pane and top (bottom) item: */
	Dimension	vert_margin;
	Dimension	horiz_margin;	/* horiz margin between pane & item */

	/*
	 * total vertical padding.
	 * therefore: pad + label height = item height
	 */
	Dimension	vert_pad;
    
	/* 
	 * total horizontal padding.
	 * therefore: pad + label width = item width 
	 */
	Dimension	horiz_pad;
	Dimension	sb_width_pad;	/* width of Scrollbar + it's 
									XtNxOffset from Pane*/
    Dimension       aligned_pos;
    Dimension       max_comp_width; /* Max(widths_of_second_component) */
} ListPanePart;

/* Full instance record declaration */
typedef struct _ListPaneRec {
	CorePart		core;
	PrimitivePart		primitive;
	ListPanePart		list_pane;
}			ListPaneRec;


#if	defined(__STDC__) || defined(__cplusplus)

extern void		_OlPrintList(Widget slw);

#else	/* __STDC__ || __cplusplus */

extern void		_OlPrintList();

#endif	/* __STDC__ || __cplusplus */


#ifdef	__cplusplus
}
#endif


#endif	/* _XOL_LISTPANEP_H */
