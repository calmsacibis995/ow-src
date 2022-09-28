#ifndef	_XOL_STRUTIL_H
#define	_XOL_STRUTIL_H

#pragma	ident	"@(#)strutil.h	302.1	92/03/26 include/Xol SMI"	/* olmisc:strutil.h 1.1 	*/

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


#ifdef	__cplusplus
extern "C" {
#endif


extern int	strmch(char* s1, char* s2);
extern int	strnmch(char* s1, char* s2, int n);
extern char*	strndup(char* s, int l);


#ifdef	__cplusplus
}
#endif


#endif	/* _XOL_STRUTIL_H */
