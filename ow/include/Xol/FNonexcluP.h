#ifndef	_XOL_FNONEXCLUP_H
#define	_XOL_FNONEXCLUP_H

#pragma	ident	"@(#)FNonexcluP.h	302.1	92/03/26 include/Xol SMI"	/* flat:FNonexcluP.h 1.3 	*/

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
 * Description:
 *	This is the flat non-exclusive container's private header file.
 ************************************************************************	
 */


#include <Xol/FExclusivP.h>
#include <Xol/FNonexclus.h>


#ifdef	__cplusplus
extern "C" {
#endif


/*
 ************************************************************************	
 * Define Widget Class Part and Class Rec
 ************************************************************************	
 */

				/* Define new fields for the class part	*/

typedef struct {
    int		no_class_fields;
} FlatNonexclusivesClassPart;

				/* Full class record declaration 	*/

typedef struct _FlatNonexclusivesClassRec {
    CoreClassPart		core_class;
    PrimitiveClassPart		primitive_class;
    FlatClassPart		flat_class;
    FlatExclusivesClassPart	exclusives_class;
    FlatNonexclusivesClassPart	nonexclusives_class;
} FlatNonexclusivesClassRec;

				/* External class record declaration	*/

extern FlatNonexclusivesClassRec	flatNonexclusivesClassRec;

/*
 ************************************************************************	
 * Define Widget Instance Structure
 ************************************************************************	
 */

typedef struct {
	int		no_instance_fields;
} FlatNonexclusivesPart;

				/* Full instance record declaration	*/

typedef struct _FlatNonexclusivesRec {
    CorePart			core;
    PrimitivePart		primitive;
    FlatPart			flat;
    FlatExclusivesPart		exclusives;
    FlatNonexclusivesPart	nonexclusives;
} FlatNonexclusivesRec;


#ifdef	__cplusplus
}
#endif


#endif	/* _XOL_FNONEXCLUP_H */
