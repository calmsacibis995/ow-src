#ifndef	_XOL_BULLETINBP_H
#define	_XOL_BULLETINBP_H

#pragma	ident	"@(#)BulletinBP.h	302.1	92/03/26 include/Xol SMI"	/* bboard:include/Xol/BulletinBP.h 1.7 	*/

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


#include <Xol/BulletinBo.h>
#include <Xol/ManagerP.h>	/* include superclasses' header */

#include <X11/CoreP.h>
#include <X11/CompositeP.h>
#include <X11/ConstrainP.h>
#include <X11/IntrinsicP.h>


#ifdef	__cplusplus
extern "C" {
#endif


typedef struct {
	int			keep_compiler_happy;	/* No new procedures */
} BulletinBoardClassPart;

typedef struct _BulletinBoardClassRec {
	CoreClassPart		core_class;
	CompositeClassPart	composite_class;
	ConstraintClassPart	constraint_class;
	ManagerClassPart	manager_class;
	BulletinBoardClassPart	bulletin_class;
} BulletinBoardClassRec;

/* New fields for the Bulletin Board widget record */
typedef struct {
	OlDefine		layout;
} BulletinBoardPart;

/*
 * Full instance record declaration
 */
typedef struct _BulletinBoardRec {
	CorePart		core;
	CompositePart		composite;
	ConstraintPart		constraint;
	ManagerPart		manager;
	BulletinBoardPart	bulletin;
} BulletinBoardRec;


extern BulletinBoardClassRec	bulletinBoardClassRec;


#ifdef	__cplusplus
}
#endif


#endif	/* _XOL_BULLETINBP_H */
