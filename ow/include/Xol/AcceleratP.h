#ifndef	_XOL_ACCELERATP_H
#define	_XOL_ACCELERATP_H

#pragma	ident	"@(#)AcceleratP.h	302.2	92/11/06 include/Xol SMI"	/* mouseless:AcceleratP.h 1.3 	*/

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


#include <Xol/OpenLook.h>

#include <X11/Intrinsic.h>


#ifdef	__cplusplus
extern "C" {
#endif


typedef struct KeyEvent {
	Widget                  w;
	XtPointer               data;
	Boolean                 is_mnemonic;
	Boolean                 grabbed;
	Modifiers               modifiers;
	KeySym                  keysym;
}                       KeyEvent;

typedef struct OlAcceleratorList {
	KeyEvent*		base;
	Cardinal                nel;
	Boolean                 do_grabs;
}                       OlAcceleratorList;


#ifdef	__cplusplus
}
#endif


#endif	/* _XOL_ACCELERATP_H */
