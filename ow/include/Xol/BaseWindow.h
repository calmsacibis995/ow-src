#ifndef	_XOL_BASEWINDOW_H
#define	_XOL_BASEWINDOW_H

#pragma	ident	"@(#)BaseWindow.h	302.1	92/03/26 include/Xol SMI"	/* basewindow:include/Xol/BaseWindow.h 1.5 	*/

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


#include <X11/Shell.h>


#ifdef	__cplusplus
extern "C" {
#endif


typedef struct _BaseWindowShellClassRec	       *BaseWindowShellWidgetClass;
typedef struct _BaseWindowShellRec 	       *BaseWindowShellWidget;


extern WidgetClass				baseWindowShellWidgetClass;


#ifdef	__cplusplus
}
#endif


#endif	/* _XOL_BASEWINDOW_H */
