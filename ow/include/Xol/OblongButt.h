#ifndef	_XOL_OBLONGBUTT_H
#define	_XOL_OBLONGBUTT_H

#pragma	ident	"@(#)OblongButt.h	302.1	92/03/26 include/Xol SMI"	/* button:include/openlook/OblongButt.h 1.10 	*/

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

/*	Copyright (c) 1988 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#include <Xol/Button.h>


#ifdef	__cplusplus
extern "C" {
#endif


/*
 *  oblongButtonWidgetClass is defined in  OblongButton.c
 */
extern WidgetClass				oblongButtonWidgetClass;
typedef struct _OblongButtonClassRec *		OblongButtonWidgetClass;
typedef struct _OblongButtonRec *		OblongButtonWidget;

/*
 *  oblongButtonGadgetClass is defined in  OblongButton.c
 */
extern WidgetClass				oblongButtonGadgetClass;
typedef struct _OblongButtonGadgetClassRec *	OblongButtonGadgetClass;
typedef struct _OblongButtonGadgetRec *		OblongButtonGadget;


#ifdef	__cplusplus
}
#endif


#endif	/* _XOL_OBLONGBUTT_H */
