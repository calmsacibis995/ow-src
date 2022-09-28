#ifndef	_XOL_MEMUTIL_H
#define	_XOL_MEMUTIL_H

#pragma	ident	"@(#)memutil.h	302.1	92/03/26 include/Xol SMI"	/* olmisc:memutil.h 1.1 	*/

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


#include <stdio.h>


#ifdef	__cplusplus
extern "C" {
#endif


#ifdef DEBUG
#	define PRINTF(x)		(void) fprintf x
#	define REGISTER_MALLOC(p)	(void) fprintf(stderr, \
		"%x malloced around %s at %d.\n", p, __FILE__, __LINE__);
#else
#	define PRINTF(x)
#	define REGISTER_MALLOC(p) 
#endif


#define FREE(p)         _Free(p, __FILE__, __LINE__)
#define CALLOC(n,s)     _Calloc(n, s, __FILE__, __LINE__)
#define MALLOC(p)       _Malloc(p, __FILE__, __LINE__)
#define REALLOC(p,s)    _Realloc(p, s, __FILE__, __LINE__)


extern char*	_Calloc(unsigned n, unsigned size, char* file, int line);
extern void	_Free(char* ptr, char* file, int line);
extern char*	_Malloc(unsigned size, char* file, int line);
extern char*	_Realloc(char* ptr, unsigned size, char* file, int line);
extern void	_SetMemutilDebug(int flag);


#ifdef	__cplusplus
}
#endif


#endif	/* _XOL_MEMUTIL_H */
