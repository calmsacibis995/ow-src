#ifndef	_XOL_OLCENV_H
#define	_XOL_OLCENV_H

#pragma	ident	"@(#)OlCEnv.h	302.5	94/03/04 include/Xol SMI"    /* oltemporary:OpenLook.h 1.52	*/

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
/*	All Rights Reserved	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*************************************************************************
 *
 *	Support for K&R C, ANSI C, C++ variants, and others
 *
 ************************************************************************/

#ifdef	__cplusplus
extern "C" {
#endif

/************************************************************************
 *
 *	Global flags
 *
 ************************************************************************/

#ifdef	__STDC__
	#ifndef	FUNCPROTO
	#define	FUNCPROTO	1
	#endif
#endif	/* __STDC__ */

/************************************************************************
 *
 *	All of the following definitions are ABSOLETE.
 *	They are provided for backward source compatibility only.
 *	These definitions will be removed by the next major release.
 *
 ************************************************************************/

/* 
 * Define `OLconst' as `const' if ANSI C or C++, otherwise define it as
 * ` ' (i.e nothing).  Header file code uses `OLconst' to support K&R
 * and ANSI C/C++ compilers.
 */
#if	defined(__STDC__) || defined(__cplusplus)
#	define	OLconst	const
#	ifndef CONST
#		define CONST const
#	endif
#else	/* __STDC__ || __cplusplus */
#	define	OLconst
#	ifndef CONST
#		define CONST
#	endif
#endif	/* __STDC__ && __cplusplus */

/* 
 * Define two macros for function Prototypes.
 * NOTE: since ANSI C and C++ treat externs for parameterless functions
 * differently, declare a parameter function in the following way:
 *
 * For functions with parameters:
 *	fooWithParams OL_ARGS((char *int));
 * For functions with no parameters:
 *	fooNoParams OL_NO_ARGS();
 *
 * If token OlNeedFunctionPrototypes is defined and is zero, we undefine.
 * Else we set redefine it to have the value of one.
 */
#if	!defined(OlNeedFunctionPrototypes) || (OlNeedFunctionPrototypes != 0)

#	ifdef	OlNeedFunctionPrototypes
#	undef	OlNeedFunctionPrototypes
#	endif	/* OlNeedFunctionPrototypes */

#	if	defined(__STDC__) || defined(__cplusplus)
#		define	OlNeedFunctionPrototypes 1
#		define	OL_ARGS(x)	x
#		define	OL_NO_ARGS()	(void)
#	else	/* __STDC__ || __cpluslus */
#		undef	OlNeedFunctionPrototypes
#		define	OL_ARGS(x)	()
#		define	OL_NO_ARGS()	()
#	endif	/* __STDC__ || __cpluslus */

#else	/* !OlNeedFunctionPrototypes || OlNeedFunctionPrototypes != 0 */

#	undef	OlNeedFunctionPrototypes
#	define	OL_ARGS(x)	()
#	define	OL_NO_ARGS()	()

#endif /* !OlNeedFunctionPrototypes || OlNeedFunctionPrototypes != 0 */

/* 
 * Now add three new macros to be used when defining functions.
 * These macros look at the OlNeedFunctionPrototype token to
 * determine how the macro is expanded.  Below illustrates how
 * to use the macro.
 *
 *	FooBar OLARGLIST((num1, string, flag))
 *		OLARG(int,	num1)
 *		OLARG(String,	string)
 *		OLGRA(int,	flag)
 *	{
 *		body of FooBar...
 *	}
 *
 * For variable argument functions use the following syntax:
 *
 *	FooBar OLARGLIST((num1, string, OLVARGLIST))
 *		OLARG(int,	num1)
 *		OLARG(String,	string)
 *		OLVARGS
 *	{
 *		body of FooBar...
 *	}
 *
 * Also add two macros for enveloping function prototype sections.
 */
#if	OlNeedFunctionPrototypes
#	define	OLARGLIST(list)	(
#	define	OLARG(t,a)	t a,
#	define	OLGRA(t,a)	t a)
#	define	OLVARGLIST	...	/* ellided parameter */
#	define	OLVARGS		...)
#else	/* OlNeedFunctionPrototypes */
#	define	OLARGLIST(list)	list
#	define	OLARG(t,a)	t a;
#	define	OLGRA(t,a)	t a;
#	define	OLVARGLIST	va_alist
#	define	OLVARGS		va_dcl
#endif	/* OlNeedFunctionPrototypes */

#ifdef	__cplusplus
#	define	OLBeginFunctionPrototypeBlock	extern "C" {
#	define	OLEndFunctionPrototypeBlock	}
#else	/* __cplusplus */
#	define	OLBeginFunctionPrototypeBlock
#	define	OLEndFunctionPrototypeBlock
#endif	/* __cplusplus */

/************************************************************************
 *
 *	All of the previous definitions are ABSOLETE.
 *	They are provided for backward source compatibility only.
 *	These definitions will be removed by the next major release.
 *
 ************************************************************************/

/************************************************************************
 *
 *	Support for a certain code interpreter
 *
 ************************************************************************/
 
#if	defined(SABER) || defined(__CODECENTER__)
#	ifdef	__STDC__
		#include <stdarg.h>
		#undef	va_start
		#define	va_start(list, arg) \
					list = (char*)&arg + sizeof(arg)
		#undef	__builtin_va_arg_incr
		#define	__builtin_va_arg_incr(list) \
					(((list) += 1) -1)
#	endif	/* __STDC__ */
#endif	/* SABER || __CODECENTER__ */


#ifdef	__cplusplus
}
#endif

#endif	/* _XOL_OLCENV_H */
