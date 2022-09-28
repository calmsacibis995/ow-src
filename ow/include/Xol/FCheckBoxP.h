#ifndef	_XOL_FCHECKBOXP_H
#define	_XOL_FCHECKBOXP_H

#pragma	ident	"@(#)FCheckBoxP.h	302.1	92/03/26 include/Xol SMI"	/* flat:FCheckBoxP.h 1.4 	*/

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
 *	This is the flat checkbox container's private header file.
 ************************************************************************	
 */


#include <Xol/FExclusivP.h>
#include <Xol/FCheckBox.h>


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
} FlatCheckBoxClassPart;

				/* Full class record declaration 	*/

typedef struct _FlatCheckBoxClassRec {
    CoreClassPart		core_class;
    PrimitiveClassPart		primitive_class;
    FlatClassPart		flat_class;
    FlatExclusivesClassPart	exclusives_class;
    FlatCheckBoxClassPart	checkbox_class;
} FlatCheckBoxClassRec;

				/* External class record declaration	*/

extern FlatCheckBoxClassRec	flatCheckBoxClassRec;

/*
 ************************************************************************	
 * Define Widget Instance Structure
 ************************************************************************	
 */
				/* Define Expanded sub-object instance	*/

typedef struct {
	OlDefine	position;	/* position of check box	*/
} FlatCheckBoxItemPart;

			/* Item's Full Instance record declaration	*/

typedef struct {
	FlatItemPart		flat;
	FlatExclusivesItemPart	exclusives;
	FlatCheckBoxItemPart	checkbox;
} FlatCheckBoxItemRec, *FlatCheckBoxItem;

			/* Define new fields for the instance part	*/
typedef struct {
			/* Must finish this in next load	*/

	FlatCheckBoxItemPart	item_part;
} FlatCheckBoxPart;

				/* Full instance record declaration	*/

typedef struct _FlatCheckBoxRec {
    CorePart		core;
    PrimitivePart	primitive;
    FlatPart		flat;
    FlatExclusivesPart	exclusives;
    FlatCheckBoxPart	checkbox;
} FlatCheckBoxRec;


#ifdef	__cplusplus
}
#endif


#endif	/* _XOL_FCHECKBOXP_H */
