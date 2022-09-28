#ifndef	_XOL_NOTICE_H
#define	_XOL_NOTICE_H

#pragma	ident	"@(#)Notice.h	302.1	92/03/26 include/Xol SMI"	/* notice:include/openlook/Notice.h 1.13 	*/

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



#include <X11/Shell.h>		/* include superclasses's header file */


#ifdef	__cplusplus
extern "C" {
#endif


/* 
 * Name			Type	Default	   Meaning
 * ----			----	-------	   -------
 * XtNtextArea		widget	NULL	control widget ID
 * XtNcontrolArea	widget	NULL	text widget ID
 * XtNemanateWidget	widget	parent	widget from which Notice emanates
 */

/* Class record pointer */
extern WidgetClass noticeShellWidgetClass;

/* C Widget type definition */
typedef struct _NoticeShellClassRec	*NoticeShellWidgetClass;
typedef struct _NoticeShellRec		*NoticeShellWidget;


#ifdef	__cplusplus
}
#endif


#endif	/* _XOL_NOTICE_H */
