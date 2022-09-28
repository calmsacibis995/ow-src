#ifndef	_XOL_CHANGEBAR_H
#define	_XOL_CHANGEBAR_H

#pragma	ident	"@(#)ChangeBar.h	302.1	92/03/26 include/Xol SMI"	/* changebar:ChangeBar.h 1.5 	*/

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


#include <Xol/OpenLook.h>

#include <X11/Intrinsic.h>


#ifdef	__cplusplus
extern "C" {
#endif


#define _OlChangeBarWidth(CB)	((CB)->width)
#define _OlChangeBarHeight(CB)	((CB)->height)
#define _OlChangeBarSpan(CB)	((CB)->width + (CB)->pad)

/*
 * Size/spacing of a change bar, in points
 */
#define CHANGE_BAR_WIDTH	3
#define CHANGE_BAR_HEIGHT	18
#define CHANGE_BAR_PAD		7

/*
 * Pick numbers not taken by the other (official) OlDefines
 */
#define OL_DIM				1000
#define OL_NORMAL			1001

#define OL_PROPAGATE_TO_CONTROL_AREA	0x0001
#define OL_PROPAGATE_TO_CATEGORY	0x0002
#define OL_PROPAGATE \
		(OL_PROPAGATE_TO_CONTROL_AREA|OL_PROPAGATE_TO_CATEGORY)

extern char		XtNallowChangeBars [];
extern char		XtNchangeBar	   [];

extern char		XtCAllowChangeBars [];
extern char		XtCChangeBar	   [];

typedef struct ChangeBar {
	GC			normal_GC; /* GC for regular change bar	*/
	GC			dim_GC;	   /* GC for dim change bar	*/
	Dimension		width;	   /* width of change bar	*/
	Dimension		height;	   /* height of change bar	*/
	Dimension		pad;	   /* padding next to change bar*/
}			ChangeBar;

/*
 * External functions:
 */

#if	defined(__STDC__) || defined(__cplusplus)

extern ChangeBar*	_OlCreateChangeBar(Widget w);

extern void		_OlDestroyChangeBar(Widget w, ChangeBar* cb);

extern void		_OlSetChangeBarState(Widget w, OlDefine state,
	unsigned int propagate);

extern void		_OlFlatSetChangeBarState(Widget w, Cardinal indx,
	OlDefine state, unsigned int propagate);

extern void		_OlDrawChangeBar(Widget w, ChangeBar* cb,
	OlDefine state, Boolean expose, Position x, Position y, Region region);

extern void		_OlGetChangeBarGCs(Widget w, ChangeBar* cb);

extern void		_OlFreeChangeBarGCs(Widget w, ChangeBar* cb);

extern Pixel		_OlContrastingColor(Widget w, Pixel pixel, int percent);

#else	/* __STDC__ || __cplusplus */

extern ChangeBar*	_OlCreateChangeBar();
extern void		_OlDestroyChangeBar();
extern void		_OlSetChangeBarState();
extern void		_OlFlatSetChangeBarState();
extern void		_OlDrawChangeBar();
extern void		_OlGetChangeBarGCs();
extern void		_OlFreeChangeBarGCs();
extern Pixel		_OlContrastingColor();

#endif	/* __STDC__ || __cplusplus */


#ifdef	__cplusplus
}
#endif


#endif	/* _XOL_CHANGEBAR_H */
