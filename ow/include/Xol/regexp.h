#ifndef	_XOL_REGEXP_H
#define	_XOL_REGEXP_H

#pragma	ident	"@(#)regexp.h	302.2	93/03/31 include/Xol SMI"	/* olmisc:regexp.h 1.1 	*/

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


#if	defined(__STDC__) || defined(__cplusplus)

extern char* strexp(char* string, char* curp, char* expression);
extern char* strrexp(char* string, char* curp, char* expression);
extern char* streexp(void);
extern char *mbstrexp(char *string, char *curp, char *exp); 
extern char *mbstrrexp(char *string, char *curp, char *exp); 
extern wchar_t *wcstrexp(wchar_t *string, wchar_t *curp, wchar_t *exp); 
extern wchar_t *wcstrrexp(wchar_t *string, wchar_t *curp, wchar_t *exp); 

#else	/* __STDC__ || __cplusplus */

extern char*	strexp();
extern char*	strrexp();
extern char*	streexp();
extern char *mbstrexp(); 
extern char *mbstrrexp(); 
extern wchar_t *wcstrexp(); 
extern wchar_t *wcstrrexp(); 

#endif	/* __STDC__ || __cplusplus */


#ifdef	__cplusplus
}
#endif


#endif	/* _XOL_REGEXP_H */
