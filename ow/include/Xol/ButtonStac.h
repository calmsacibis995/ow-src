#ifndef	_XOL_BUTTONSTAC_H
#define	_XOL_BUTTONSTAC_H

#pragma	ident	"@(#)ButtonStac.h	302.1	92/03/26 include/Xol SMI"	/* menu:include/Xol/ButtonStac.h 1.11	*/

/*
 *	Copyright (C) 1986,1992  Sun Microsystems, Inc
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

/**************************************************************************
 *
 * 	THIS FILE IS OBSOLETE AND WILL NOT BE PROVIDED IN THE NEXT RELEASE
 *
 **************************************************************************/


#include <Xol/MenuButton.h>

#include <X11/Intrinsic.h>


#ifdef	__cplusplus
extern "C" {
#endif


extern WidgetClass			buttonStackWidgetClass;

typedef struct _MenuButtonClassRec*	ButtonStackWidgetClass;
typedef struct _MenuButtonRec*		ButtonStackWidget;


extern WidgetClass			buttonStackGadgetClass;

typedef struct _MenuButtonGadgetClassRec* ButtonStackGadgetClass;
typedef struct _MenuButtonGadgetRec*	ButtonStackGadget;


#ifdef	__cplusplus
}
#endif


#endif	/* _XOL_BUTTONSTAC_H */
