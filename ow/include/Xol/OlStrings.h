#ifndef	_XOL_OLSTRINGS_H
#define	_XOL_OLSTRINGS_H

#pragma	ident	"@(#)OlStrings.h	302.1	92/03/26 include/Xol SMI"	/* olcommon:include/Xol/OlStrings.h 1.45.1.16	*/

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


/*
 * Stylized names like XtNfoo, XtCfoo, XtRfoo should be defined
 * in the common file Xol/StringList, which is included by both
 * OlStrings.c and OlStrings.h, as follows:
 *
 *	STRING(XtN,foo);	no spaces inside the () for K&R CPP!
 *
 * This produces
 *
 *	 const char	XtNfoo[] = "foo";
 *
 * Other names that don't fit this style should be defined here
 * and declared in OlStrings.h.
 *
 * Use `const' to define the `STRING' marco if ANSI C.
 */
#ifdef	__STDC__

#	define	STRING(prefix, string) \
			extern const char	prefix ## string []

#	define	VSTRING(prefix, string, v) \
			extern const char	prefix ## string []

#	define	PSTRING(string, postfix) \
			extern const char	string ## postfix []
#else

#	define	STRING(prefix, string) \
			extern char	prefix/**/string/**/[]

#	define	VSTRING(prefix, string, v) \
			extern char	prefix/**/string/**/[]

#	define	PSTRING(string, postfix) \
			extern char	string/**/postfix/**/[]

#endif	/* __STDC__ */


#include <Xol/StringList>


#undef	STRING
#undef	VSTRING
#undef	PSTRING


#ifdef	__cplusplus
}
#endif


#endif	/* _XOL_OLSTRINGS_H */
