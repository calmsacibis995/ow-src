#ifndef	_XOL_STATICTEXP_H
#define	_XOL_STATICTEXP_H

#pragma	ident	"@(#)StaticTexP.h	302.2	92/09/29 include/Xol SMI"	/* statictext:include/Xol/StaticTexP.h 1.7.2.5 	*/

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

/*****************************************************************************
 **   
 **   Copyright (c) 1988 by Hewlett-Packard Company
 **   Copyright (c) 1988 by the Massachusetts Institute of Technology
 **   
 *****************************************************************************/


#include <Xol/OlgxP.h>
#include <Xol/PrimitiveP.h>	/* include superclasses's header */
#include <Xol/StaticText.h>

#include <X11/Intrinsic.h>
#include <X11/CoreP.h>


#ifdef	__cplusplus
extern "C" {
#endif


/* No new fields need to be defined for the StaticText widget class record */
typedef struct 
{
	int	make_compiler_happy;
} StaticTextClassPart;

/* Full class record declaration for StaticText class */
typedef struct _StaticTextClassRec {
	CoreClassPart      	core_class;
	PrimitiveClassPart	primitive_class;
	StaticTextClassPart	statictext_class;
} StaticTextClassRec;

/* New fields needed for instance record */
typedef struct _StaticTextPart {

	/* "Public" members (Can be set by resource manager). */
	OlStr       	input_string;	/* String sent to this widget. */
	OlDefine	alignment;	/* Alignment within the box */
	Boolean    	wrap;		/* Controls wrapping on spaces */
	Boolean		strip;		/* Controls stripping of blanks */
	int	       	gravity;	/* Controls extra space in window */
	int     	line_space;	/* Ratio of font height as dead space
					   between lines.  Can be less than zero
					   but not less than -1.0  */
	Dimension 	internal_height; /* Space from text to top and bottom 
					    highlights */
	Dimension 	internal_width; /* Space from left and right side 
					   highlights */
	Boolean		recompute_size;
	Boolean		selectable;

	/* Fields taken from XwPrimitive */
	Dimension	highlight_thickness;

	/* "Private" fields, used by internal widget code. */
	GC         	normal_GC; 	/* GC for text */
	GC         	hilite_GC; 	/* GC for highlighted text */
	GC         	cursor_GC; 	/* GC for x-or'ed cursor */
	XRectangle 	TextRect; 	/* The bounding box of the text, or clip 
					   rectangle of the window; whichever is 
					   smaller. */
	OlStr       	output_string; /* input_string after formatting */
	unsigned char	*selection_start;	/* ptr to start of selection */
	unsigned char	*selection_end;	/* ptr to end of selection */
	unsigned char	*oldsel_start;	/* old start of selection */
	unsigned char	*oldsel_end;	/* old end of selection */
	int		selection_mode;	/* chars, words, lines... */
	int		save_mode;	/* during resize */
	Time            time;           /* time of last key or button action */ 
	Position        ev_x, ev_y;     /* coords for key or button action */
	Position        old_x, old_y;   /* previous key or button coords */
	unsigned char	**line_table;	/* ptrs to start of each line */
	int		*line_lens;	/* length of each line */
	int		line_count;	/* count of line_table */
	OlStr		clip_contents;	/* contents of clipboard */
	_OlgxDevice	*devData;	/* Olg related stuff -we use the */
					/* inactive stipple therin.. */

} StaticTextPart;

/* Full instance record declaration */
typedef struct _StaticTextRec {
	CorePart		core;
	PrimitivePart		primitive;
	StaticTextPart		static_text;
}			StaticTextRec;


#ifdef	__cplusplus
}
#endif


#endif	/* _XOL_STATICTEXP_H */
