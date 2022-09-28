#ifndef	_XOL_RUBBERTILP_H
#define	_XOL_RUBBERTILP_H

#pragma	ident	"@(#)RubberTilP.h	302.1	92/03/26 include/Xol SMI"	/* rubbertile:RubberTilP.h 1.3	*/

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


#include <Xol/ManagerP.h>
#include <Xol/RubberTile.h>

#include <X11/Intrinsic.h>
#include <X11/CoreP.h>
#include <X11/CompositeP.h>
#include <X11/ConstrainP.h>


#ifdef	__cplusplus
extern "C" {
#endif


/*
 * Constraint record:
 */

typedef struct	_RubberTileConstraintRec {
	/*
	 * Public:
	 */
	String			ref_name;
	Widget			ref_widget;
	Dimension		space;
	Dimension		weight;

	/*
	 * Private:
	 */
	Dimension		set_size;
}			RubberTileConstraintRec;

/*
 * Class strucure:
 */

typedef struct _RubberTileClassPart {
	int			no_class_fields;
} RubberTileClassPart;

typedef struct _RubberTileClassRec {
	CoreClassPart		core_class;
	CompositeClassPart	composite_class;
	ConstraintClassPart	constraint_class;
	ManagerClassPart	manager_class;
	RubberTileClassPart	rubber_tile_class;
}			RubberTileClassRec;

extern RubberTileClassRec	rubberTileClassRec;

/*
 * Instance structure:
 */

typedef struct _RubberTilePart {
	/*
	 * Public:
	 */
	OlDefine		orientation;

	/*
	 * Private:
	 */
	Widget *		managed_children;
	Cardinal		num_managed_children;
}			RubberTilePart;

typedef struct _RubberTileRec {
	CorePart		core;
	CompositePart		composite;
	ConstraintPart		constraint;
	ManagerPart		manager;
	RubberTilePart		rubber_tile;
}			RubberTileRec;


#ifdef	__cplusplus
}
#endif


#endif	/* _XOL_RUBBERTILP_H */
