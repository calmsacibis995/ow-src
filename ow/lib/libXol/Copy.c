#pragma ident	"@(#)Copy.c	302.1	92/03/26 lib/libXol SMI"	/* olmisc:Copy.c 1.5	*/

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

/*
 *************************************************************************
 * Copyright 1987, 1988 by Digital Equipment Corporation, Maynard,
 * Massachusetts, and the Massachusetts Institute of Technology, Cambridge,
 * Massachusetts.
 *                         All Rights Reserved
 *************************************************************************
 */

/*
 *************************************************************************
 *
 * Description:
 *	This file contains two private externals used for setting/getting
 *	the state of a flat container's sub-object.
 *
 ******************************file*header********************************
 */


#include <stdio.h>
#include <string.h>

#include <X11/IntrinsicP.h>

#include <Xol/OpenLookP.h>


/*
 *************************************************************************
 *
 * Forward Procedure definitions listed by category:
 *		1. Public Procedures 
 *
 **************************forward*declarations***************************
 */

/*
 *************************************************************************
 *
 * Define global/static variables and #defines, and
 * Declare externally referenced variables
 *
 *****************************file*variables******************************
 */

/*
 *************************************************************************
 *
 * Public Procedures
 *
 ****************************public*procedures****************************
 */

/*
 *************************************************************************
 * _OlConvertToXtArgVal - this procedure fills in an XtArgVal with the
 * value from an arbitrary source type.  The size parameter indicates the
 * size of the source type.
 ****************************procedure*header*****************************
 */
void
_OlConvertToXtArgVal(const void *src, XtArgVal *dst, size_t size)
{
	if	(size == sizeof(long))     *dst = (XtArgVal)(*(long*)src);
	else if (size == sizeof(short))    *dst = (XtArgVal)(*(short*)src);
	else if (size == sizeof(char))	   *dst = (XtArgVal)(*(char*)src);
	else if (size == sizeof(char*))    *dst = (XtArgVal)(*(char**)src);
	else if (size == sizeof(XtPointer)) *dst = (XtArgVal)(*(XtPointer*)src);
	else if (size == sizeof(caddr_t))  *dst = (XtArgVal)(*(caddr_t*)src);
	else if (size == sizeof(XtArgVal)) *dst = *(XtArgVal*)src;
	else
	{
				/* Assume the XtArgVal currently contains
				 * the address of place to write	*/

		memmove((void*)*dst, src, size);
	}
} /* END OF _OlConvertToXtArgVal() */

#ifdef UNALIGNED

/*
 *************************************************************************
 * _OlCopyFromXtArgVal - this procedure copies an XtArgVal's value into
 * an arbitrary memory location.  The size parameter indicates the size
 * of the destination type.
 ****************************procedure*header*****************************
 */
void
_OlCopyFromXtArgVal(const XtArgVal src, void *dst, size_t size)
{
	if    (size == sizeof(long))		*(long *)dst = (long)src;
	else if (size == sizeof(short))		*(short *)dst = (short)src;
	else if (size == sizeof(char))		*(char *)dst = (char)src;
	else if (size == sizeof(char *))	*(char **)dst = (char *)src;
	else if (size == sizeof(XtPointer))	*(XtPointer *)dst =
								(XtPointer)src;
	else if (size == sizeof(caddr_t))	*(caddr_t *)dst = (caddr_t)src;
	else if (size == sizeof(XtArgVal))	*(XtArgVal *)dst = src;
	else if (size > sizeof(XtArgVal))
		memmove(dst, (const void*)src, size);
	else
		memmove(dst, (const void*)&src, size);
} /* END OF _OlCopyFromXtArgVal() */

/*
 *************************************************************************
 * _OlCopyToXtArgVal - this procedure fills in an XtArgVal with the
 * value from an arbitrary source type.  The size parameter indicates the
 * size of the source type.  The 'dst' parameter is an XtArgVal that
 * holds the address where we are to write to.
 ****************************procedure*header*****************************
 */
void
_OlCopyToXtArgVal(const void *src, XtArgVal *dst, size_t size)
{
	if	(size == sizeof(long))     *((long*)*dst) = *(long*)src;
	else if (size == sizeof(short))    *((short*)*dst) = *(short*)src;
	else if (size == sizeof(char))	   *((char*)*dst) = *(char*)src;
	else if (size == sizeof(char*))    *((char**)*dst) = *(char**)src;
	else if (size == sizeof(XtPointer)) *((XtPointer*)*dst) =
							*(XtPointer*)src;
	else if (size == sizeof(caddr_t))  *((caddr_t*)*dst) = *(caddr_t*)src;
	else if (size == sizeof(XtArgVal)) *((XtArgVal*)*dst)= *(XtArgVal*)src;
	else memmove((void*)*dst, src, size);
} /* END OF _OlCopyToXtArgVal() */

#else /* UNALIGNED */

/*
 *************************************************************************
 * _OlCopyFromXtArgVal - this procedure copies an XtArgVal's value into
 * an arbitrary memory location.  The size parameter indicates the size
 * of the destination type.
 ****************************procedure*header*****************************
 */
void
_OlCopyFromXtArgVal(const XtArgVal src, void *dst, size_t size)
{
	if (size > sizeof(XtArgVal))
	{
		memmove(dst, (const void*)src, size);
	}
	else
	{
		union {
			long		longval;
			short		shortval;
			char		charval;
			char *		charptr;
			XtPointer	xtptr;
			caddr_t		ptr;
		} u;

		void *p = (void*)&u;

		if	(size == sizeof(long))	     u.longval = (long)src;
		else if (size == sizeof(short))	     u.shortval = (short)src;
		else if (size == sizeof(char))	     u.charval = (char)src;
		else if (size == sizeof(char*))	     u.charptr = (char*)src;
		else if (size == sizeof(XtPointer))  u.xtptr = (XtPointer)src;
		else if (size == sizeof(caddr_t))    u.ptr = (caddr_t)src;
		else				     p = (void*)&src;

		memmove(dst, (const void*)p, size);
	}
} /* END OF _OlCopyFromXtArgVal */

/*
 *************************************************************************
 * _OlCopyToXtArgVal - this procedure fills in an XtArgVal with the
 * value from an arbitrary source type.  The size parameter indicates the
 * size of the source type.  The 'dst' parameter is an XtArgVal that
 * holds the address where we are to write to.
 ****************************procedure*header*****************************
 */
void
_OlCopyToXtArgVal(const void *src, XtArgVal *dst, size_t size)
{
	memmove((void*)*dst, src, size);
} /* END OF _OlCopyToXtArgVal() */

#endif /* UNALIGNED */
