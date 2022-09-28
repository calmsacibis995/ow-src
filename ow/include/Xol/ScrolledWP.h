#ifndef	_XOL_SCROLLEDWP_H
#define	_XOL_SCROLLEDWP_H

#pragma	ident	"@(#)ScrolledWP.h	302.4	92/11/18 include/Xol SMI"	/* scrollwin:include/Xol/ScrolledWP.h 1.17 	*/

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


#include <Xol/BulletinBo.h>
#include <Xol/ManagerP.h>
#include <Xol/Scrollbar.h>
#include <Xol/ScrolledWi.h>

#include <X11/CoreP.h>
#include <X11/CompositeP.h>
#include <X11/ConstrainP.h>
#include <X11/IntrinsicP.h>


#ifdef	__cplusplus
extern "C" {
#endif


typedef struct {
	int			foo;
}			ScrolledWindowClassPart;

typedef struct _ScrolledWindowClassRec {
	CoreClassPart		core_class;
	CompositeClassPart	composite_class;
	ConstraintClassPart	constraint_class;
	ManagerClassPart	manager_class;
	ScrolledWindowClassPart	scrolled_window_class;
}			ScrolledWindowClassRec;

extern ScrolledWindowClassRec	scrolledWindowClassRec;

#if	defined(__STDC__) || defined(__cplusplus)
	typedef void	(*PFV)(Widget bbc, OlSWGeometries* gg);
#else	/* __STDC__ || __cplusplus */
	typedef void	(*PFV)();
#endif	/* __STDC__ || __cplusplus */

typedef struct _ScrolledWindowPart {
	/* resources */
	int			current_page;
	OlDefine		show_page;
	int			h_granularity;
	int			h_initial_delay;
	Widget			h_menu_pane;
	int			h_repeat_rate;
	XtCallbackList		h_slider_moved;
	int			v_granularity;
	int			v_initial_delay;
	Widget			v_menu_pane;
	int			v_repeat_rate;
	XtCallbackList		v_slider_moved;
	PFV			compute_geometries;
	Boolean			recompute_view_height;
	Boolean			recompute_view_width;
	Dimension		view_height;
	Dimension		view_width;
	Boolean			force_hsb;
	Boolean			force_vsb;
	OlDefine		align_horizontal;
	OlDefine		align_vertical;
	Boolean			vAutoScroll;
	Boolean			hAutoScroll;
	int			init_x;
	int			init_y;
	Pixel			foreground;

	/* private state */
	BulletinBoardWidget	bboard;
	ScrollbarWidget		hsb;
	ScrollbarWidget		vsb;
	Widget			bb_child;
	Dimension		vborder;
	Dimension		hborder;
	Dimension		child_width;
	Dimension		child_bwidth;
	Dimension		child_height;
	GC			gc;
}			ScrolledWindowPart;

typedef struct _ScrolledWindowRec {
	CorePart		core;
	CompositePart		composite;
	ConstraintPart		constraint;
	ManagerPart		manager;
	ScrolledWindowPart	scrolled_window;
}			ScrolledWindowRec;


#ifdef	__cplusplus
}
#endif


#endif	/* _XOL_SCROLLEDWP_H */
