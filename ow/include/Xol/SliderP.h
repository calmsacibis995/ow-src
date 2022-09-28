#ifndef	_XOL_SLIDERP_H
#define	_XOL_SLIDERP_H

#pragma	ident	"@(#)SliderP.h	302.4	93/01/20 include/Xol SMI"	/* slider:include/Xol/SliderP.h 1.11	*/

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

/***********************************************************************
 *
 * Slider Widget Private Data
 *
 ***********************************************************************/


#include <Xol/OlgxP.h>
#include <Xol/PrimitiveP.h>	/* include superclasses's header */
#include <Xol/Slider.h>

#include <X11/CoreP.h>
#include <X11/Intrinsic.h>


#ifdef	__cplusplus
extern "C" {
#endif


/* Types of operation */
#define	ANCHOR		1
#define	DIR_INC		2
#define	PAGE		4
#define	ELEV_OP		8
#define	DRAG_OP		16
#define KBD_OP		32	/* mouseless operation indicator */
#define	ANCHOR_TOP	(ANCHOR)
#define	ANCHOR_BOT	(ANCHOR | DIR_INC)
#define	PAGE_DEC	(PAGE)
#define	PAGE_INC	(PAGE | DIR_INC)
#define GRAN_DEC	(ELEV_OP)
#define GRAN_INC	(ELEV_OP | DIR_INC)
#define	DRAG_ELEV	(ELEV_OP | DRAG_OP)
#define	NOOP		255

/* Types of slider */
#define	SB_REGULAR	3
#define	SB_MINREG	7

/* New fields for the Slider widget class record */
typedef struct {
	int			empty;
}			SliderClassPart;

/* Full class record declaration */
typedef struct _SliderClassRec {
	CoreClassPart		core_class;
	PrimitiveClassPart	primitive_class;
	SliderClassPart		slider_class;
}			SliderClassRec;


extern SliderClassRec		sliderClassRec;


/* 
 * New fields for the Slider widget record.
 * This struct must be the same as the Gauge widget except the extra members
 * for the slider widgets at the end.
 */
typedef struct {
	/* Public */
	int			sliderMin;
	int			sliderMax;
	int			sliderValue;
	int			ticks;
	OlStr			minLabel;
	OlStr			maxLabel;
	Dimension		span;
	Dimension		leftMargin;	/* user-specified leftMargin */
	Dimension		rightMargin;	/* user-specified rightMargin */
	OlDefine		orientation;
	OlDefine		tickUnit;
	OlDefine		stoppos;
	OlDefine		dragtype;
	Boolean			endBoxes;
	Boolean			recompute_size;
	XtCallbackList		sliderMoved;
	Boolean			useSetValCallback;

	/* Private */
	GC			labelGC;
	OlgxAttrs*		pAttrs;
	int			numticks;
	XtIntervalId		timerid;
	Position		sliderPValue;
	Position		minTickPos;	/* position of minTick */
	Position		maxTickPos;	/* position of maxTick */
	Position*		ticklist;	/* list of tickmark positions */
	Dimension		leftPad;	/* copy from leftMargin, actual
						   val */
	Dimension		rightPad;	/* copy from rightMargin,
						   actual val */
	Position		elev_offset;
	unsigned char		type;
	unsigned char		opcode;
	unsigned char		anchlen;
	unsigned char		anchwidth;
	unsigned char		elevwidth;
	unsigned char		elevheight;

	/* stuffs used only in slider widgets */
	Boolean			warp_pointer;
	Position		dragbase;	/* lengthwise pos. of mouse 
						   ptr */
	Position		absx;
	Position		absy;
	int			granularity;
	int			repeatRate;
	int			initialDelay;
	Boolean			moving;
	
}		SliderPart;


/****************************************************************
 *
 * Full instance record declaration
 *
 ****************************************************************/

typedef struct _SliderRec {
	CorePart		core;
	PrimitivePart		primitive;
	SliderPart		slider;
}			SliderRec;


#ifdef	__cplusplus
}
#endif


#endif	/* _XOL_SLIDERP_H */
