#ifndef	_XOL_HELPP_H
#define	_XOL_HELPP_H

#pragma	ident	"@(#)HelpP.h	302.1	92/03/26 include/Xol SMI"	/* help:include/Xol/HelpP.h 1.6 	*/

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

/*
 ************************************************************************
 *
 * Description:
 *		This is the "private" include file for the Help Widget
 *
 *****************************file*header********************************
 */


#include <Xol/Help.h>
#include <Xol/ManagerP.h>
#include <Xol/RubberTilP.h>

#include <X11/CompositeP.h>
#include <X11/ConstrainP.h>
#include <X11/CoreP.h>
#include <X11/Intrinsic.h>


#ifdef	__cplusplus
extern "C" {
#endif


/*
 ***********************************************************************
 *
 * Widget Private Data
 *
 ***********************************************************************
 */

			/* New fields for the widget class record	*/

typedef struct {
	int keep_compiler_happy;   /* No new procedures */
} HelpClassPart;

				/* Full class record declaration 	*/

typedef struct _HelpClassRec {
  	CoreClassPart		core_class;
	CompositeClassPart	composite_class;
	ConstraintClassPart	constraint_class;
	ManagerClassPart	manager_class;
	RubberTileClassPart	rubber_tile_class;
	HelpClassPart		help_class;
} HelpClassRec;

extern HelpClassRec helpClassRec;


/*
 ***********************************************************************
 *
 * Instance (widget) structure 
 *
 ***********************************************************************
 */

				/* New fields for the widget record	*/

typedef struct {
	Widget		text_widget;	/* Text Widget Id		*/
	Widget		mag_widget;	/* Magnifier Widget Id		*/
	Boolean		allow_root_help;/* Permit RootWindow Help	*/
	OlStrRep	text_format;	/* text_format for Text Widget	*/
} HelpPart;

					/* Full Widget declaration	*/
typedef struct _HelpRec {
	CorePart 	core;
	CompositePart 	composite;
	ConstraintPart	constraint;
	ManagerPart	manager;
	RubberTilePart	rubber_tile;
	HelpPart	help;
} HelpRec;


/*
 * Constraint record:
 */

typedef struct {
	int			no_fields;
} HelpConstraintPart;

typedef struct _HelpConstraintRec {
	RubberTileConstraintRec	rubber_tile;
	HelpConstraintPart	help;
} HelpConstraintRec, *HelpConstraint;


#ifdef	__cplusplus
}
#endif


#endif	/* _XOL_HELPP_H */
