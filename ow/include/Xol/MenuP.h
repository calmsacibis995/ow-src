#ifndef	_XOL_MENUP_H
#define	_XOL_MENUP_H

#pragma	ident	"@(#)MenuP.h	302.4	92/11/17 include/Xol SMI"	/* menu:include/Xol/MenuP.h 1.22 	*/

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


#include <Xol/Menu.h>
#include <Xol/OpenLookP.h>

#include <X11/CoreP.h>
#include <X11/CompositeP.h>
#include <X11/IntrinsicP.h>
#include <X11/ShellP.h>
#include <X11/VendorP.h>


#ifdef	__cplusplus
extern "C" {
#endif


/* New fields for the Menu widget class record	*/
typedef struct _MenuClass {
	int			no_new_fields;
}			MenuShellClassPart;

/* Full class record declaration 	*/
typedef struct _MenuClassRec {
	CoreClassPart		core_class;
	CompositeClassPart	composite_class;
	ShellClassPart		shell_class;
	WMShellClassPart	wm_shell_class;
	VendorShellClassPart	vendor_shell_class;
	TransientShellClassPart	transient_shell_class;
	MenuShellClassPart	menu_shell_class;
}			MenuShellClassRec;


extern MenuShellClassRec	menuShellClassRec;


/***************************************
 *
 *  Instance (widget) structure 
 *
 **************************************/

/* new function typedef */
#if	defined(__STDC__) || defined(__cplusplus)
typedef void	(*OlRevertButtonProc)(Widget widget, Boolean status);
#else	/* __STDC__ || __cplusplus */
typedef void	(*OlRevertButtonProc)();
#endif	/* __STDC__ || __cplusplus */

/* New fields for the Menu widget record */
typedef struct {
					/* New Resources		*/

    OlRevertButtonProc	revert_button;	/* unhighlight procedure for
					 * MenuButtons			*/
    ShellBehavior	shell_behavior;	/* Current state of the Menu	*/
    Boolean		augment_parent;	/* Does menu augment parent's
					   event handler list ??	*/
    Boolean		attach_query;	/* prevent Menus from being
					   attached to this shell menu	*/
    Boolean		application_menu;/* is this an appliation's menu*/
    Boolean		prevent_stayup;	/* Stay-up mode not allowed	*/
    Boolean		pushpin_default;/* is the pushpin the default	*/
    Widget		title_widget;	/* id of the title widget	*/
    Widget		pushpin;	/* Pushpin widget id		*/
    Widget		parent_cache;	/* Cache of parent widget't id	*/
    CompositeWidget	pane;		/* Pane to take on child Widgets*/
    MenuShellWidget	root;		/* Root of Menu Cascade		*/
    MenuShellWidget	next;		/* Next menu in cascade		*/
    Widget		emanate;	/* Widget from which menu emanates */
    GC			shadow_GC;	/* Drop Shadow's GC		*/
    GC			backdrop_GC;	/* GC for underneath copy	*/
    Pixmap		backdrop_right;	/* Pixmap under Menu		*/
    Pixmap		backdrop_bottom;/* Pixmap under Menu		*/
    Pixel		foreground;	/* Foreground for the menu	*/
    Position		post_x;		/* Pointer x coor. at posting	*/
    Position		post_y;		/* Pointer y coor. at posting	*/
    Position		press_x; 	/* Need to store this as pointer may get */
    Position		press_y; 	/*  	warped			*/
    Dimension		shadow_right;	/* Pixel width of right shadow	*/
    Dimension		shadow_bottom;	/* Pixel height of bottom shadow*/
    Widget		revert_focus;	/* widget to revert focus */
    OlStrRep		text_format;
}		MenuShellPart;

/*
 * Widget Instance declaration
 */
typedef struct _MenuShellRec {
	CorePart		core;
	CompositePart		composite;
	ShellPart		shell;
	WMShellPart		wm;
	VendorShellPart		vendor;
	TransientShellPart	transient;
	MenuShellPart		menu;
}			 MenuShellRec;


/*
 * function prototype section
 */

#if	defined(__STDC__) || defined(__cplusplus)

/* 
 * Check to see if position is within a "click" threshold,
 * specified in points
 */
extern Boolean		_OlIsMenuMouseClick(
	MenuShellWidget	menu,	/* menu in question */
	int		x,	/* coordinate */
	int		y	/* y coordinate */
);

/* 
 * Propagate a new menu state down a menu cascade 
 */
extern void		_OlPropagateMenuState(
	MenuShellWidget	menu,		/* start point in cascade */
	ShellBehavior	new_state
);

#else	/* __STDC__ || __cplusplus */

extern Boolean		_OlIsMenuMouseClick();
extern void		_OlPropagateMenuState();

#endif	/* __STDC__ || __cplusplus */


#ifdef	__cplusplus
}
#endif


#endif	/* _XOL_MENUP_H */
