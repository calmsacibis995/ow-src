#ifndef	_XOL_CATEGORYP_H
#define	_XOL_CATEGORYP_H

#pragma	ident	"@(#)CategoryP.h	302.3	92/09/29 include/Xol SMI"	/* category:CategoryP.h 1.5 	*/

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


#include <Xol/Category.h>
#include <Xol/ChangeBar.h>
#include <Xol/ManagerP.h>
#include <Xol/OlgxP.h>


#ifdef	__cplusplus
extern "C" {
#endif


/*
 * Space (in points) between
 *
 *	- top of CATEGORY label, AbbrevMenuButton widget, page label
 *	- left edge of CATEGORY label and left edge of Category widget
 *	- CATEGORY and AbbrevMenuButton widget
 *	- AbbrevMenuButton widget and page label
 *	- bottom of CATEGORY label, AbbrevMenuButton widget, page label
 *	- footer and edge of Category widget
 *	- left and right footers (1/2 that distance, actually)
 *	- left edge and change bar
 */
#define CATEGORY_TOP_MARGIN		10
#define CATEGORY_LEFT_MARGIN		10
#define CATEGORY_SPACE1			10
#define CATEGORY_SPACE2			10
#define CATEGORY_BOTTOM_MARGIN		10
#define CATEGORY_FOOTER_EDGE		10
#define CATEGORY_INTER_FOOTER	 	4
#define CATEGORY_FOOTER_HEIGHT		22
#define CATEGORY_CHANGE_BAR_SPACE	4

#define _CATEGORY_INTERNAL_CHILD	0x001

#define _CATEGORY_B_DYNAMIC_BACKGROUND	0x001
#define _CATEGORY_B_DYNAMIC_FONTCOLOR	0x002


/*
 * Constraint record:
 */
typedef struct	_CategoryConstraintRec {
	/*
	 * Public:
	 */
	String			page_label;
	int			gravity;
	Boolean			_default;
	Boolean			changed;
	Boolean			available_when_unmanaged;
	Boolean			query_child;

	/*
	 * Private:
	 */
	Boolean			window_gravity_set;
}			CategoryConstraintRec;

/*
 * Class strucure:
 */
typedef struct _CategoryClassPart {
	int			no_class_fields;
}			CategoryClassPart;

typedef struct _CategoryClassRec {
	CoreClassPart		core_class;
	CompositeClassPart	composite_class;
	ConstraintClassPart	constraint_class;
	ManagerClassPart	manager_class;
	CategoryClassPart	category_class;
}			CategoryClassRec;

/*
 * Instance structure:
 */

typedef struct MenuItem {
	String			label;
	Boolean			set;
	XtPointer		user_data;
}			MenuItem;

typedef struct _CategoryPart {
	/*
	 * Public:
	 */
	struct {
		OlDefine		width;
		OlDefine		height;
	}			layout;
	struct {
		Dimension		width;
		Dimension		height;
	}			page;
	String			category_label;
	XFontStruct *		category_font;
	XFontStruct *		font;
	Pixel			font_color;
	Boolean			show_footer;
	String			left_foot;
	String			right_foot;
	XtCallbackList		new_page;
	Widget			lower_control_area;

	/*
	 * Private:
	 */
	Widget			page_choice;
	Widget			next_page;
	Widget			delayed_set_page_child;
	MenuItem *		page_list;
	Cardinal		first_page_child;
	Cardinal		page_list_size;
	Cardinal		current_page;
	GC			category_font_GC;
	GC			font_GC;
	OlgxAttrs *		attrs;
	ChangeBar *		cb;
	XtOrderProc		insert_position;
	unsigned char		flags;
	unsigned char		dynamics;
}			CategoryPart;

typedef struct _CategoryRec {
	CorePart		core;
	CompositePart		composite;
	ConstraintPart		constraint;
	ManagerPart		manager;
	CategoryPart		category;
}			CategoryRec;


extern CategoryClassRec		categoryClassRec;


#ifdef	__cplusplus
}
#endif


#endif	/* _XOL_CATEGORYP_H */
