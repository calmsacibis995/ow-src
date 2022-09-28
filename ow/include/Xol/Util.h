#ifndef	_XOL_UTIL_H
#define	_XOL_UTIL_H

#pragma	ident	"@(#)Util.h	302.1	92/03/26 include/Xol SMI"	/* olmisc:Util.h 1.1 	*/

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

/*	Copyright (c) 1990 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#include <Xol/OpenLookP.h>

#include <X11/Intrinsic.h>

#include <string.h>


#ifdef	__cplusplus
extern "C" {
#endif


#define ABS(x) 		((x) < 0 ? -(x) : (x))
#define NABS(x) 	((x) > 0 ? -(x) : (x))

#define	STREQU(A,B)	(strcmp((A),(B)) == 0)

#define	Strdup(S)	strcpy(XtMalloc((unsigned)Strlen(S) + 1), S)

#define Array(P,T,N) \
	((N)? \
		  ((P)? \
			  (T*)XtRealloc((P), sizeof(T) * (N)) \
			: (T*)XtMalloc(sizeof(T) * (N)) \
		  ) \
		: ((P)? (XtFree(P),(T*)0) : (T*)0) \
	)


#ifndef	MAX
#define MAX(x,y)	_OlMax(x,y)
#endif

#ifndef	MIN
#define MIN(x,y)	_OlMin(x,y)
#endif

#define	Strlen(string)	_OlStrlen(string)


#ifdef	__cplusplus
}
#endif


#endif	/* _XOL_UTIL_H */
