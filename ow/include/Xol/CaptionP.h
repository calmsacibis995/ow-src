#ifndef	_XOL_CAPTIONP_H
#define	_XOL_CAPTIONP_H

#pragma	ident	"@(#)CaptionP.h	302.4	92/10/14 include/Xol SMI"	/* caption:include/openlook/CaptionP.h 1.18 	*/

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


#include <Xol/ManagerP.h>	/* include superclasses' header */
#include <Xol/Caption.h>
#include <Xol/OlgxP.h>


#ifdef	__cplusplus
extern "C" {
#endif


typedef struct {
	int			make_compiler_happy;
} CaptionClassPart;

/* Full class record declaration */
typedef struct _CaptionClassRec {
	CoreClassPart		core_class;
	CompositeClassPart	composite_class;
	ConstraintClassPart	constraint_class;
	ManagerClassPart	manager_class;
	CaptionClassPart	caption_class;
} CaptionClassRec;

extern CaptionClassRec captionClassRec;

/* New fields for the Caption widget record */

typedef struct {
	/* resources */
	Pixel			fontcolor;
	OlStrRep		text_format;
	OlFont			font;
	OlStr			label;
	OlDefine		position;
	OlDefine		alignment;
	Dimension		space;
	OlMnemonic		mnemonic;
	Boolean			recompute_size;

	/* internal data */
	GC			normal_GC;	/* GC for text */
	GC			hilite_GC;	/* GC for highlighted text */
	Dimension		caption_len;	/* len of text in chars */
	Dimension		caption_width;	/* width of text in pixels */
	Dimension		caption_height;	/* height of caption in pixels */
	Position		caption_x;	/* where text is placed */
	Position		caption_y;	/* where text is placed */
	OlgxAttrs*		pAttrs;		/* drawing attributes */
	unsigned char		dyn_flags;
}			CaptionPart;

/*
 * Full instance record declaration
 */
typedef struct _CaptionRec {
	CorePart		core;
	CompositePart		composite;
	ConstraintPart		constraint;
	ManagerPart		manager;
	CaptionPart		caption;
}			CaptionRec;

/* dynamics resource bit masks */
#define OL_B_CAPTION_FONTCOLOR	(1 << 0)


#ifdef	__cplusplus
}
#endif


#endif	/* _XOL_CAPTIONP_H */
