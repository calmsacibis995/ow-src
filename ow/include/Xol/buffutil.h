#ifndef	_XOL_BUFFERUTIL_H
#define	_XOL_BUFFERUTIL_H

#pragma	ident	"@(#)buffutil.h	302.1	92/03/26 include/Xol SMI"	/* olmisc:buffutil.h 1.3	*/

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


	/* shouldn't check for SEEK_SET because it defined in unistd.h  */
	/* in SVR3.2 and X11/Xos.h will include this header file if USG */


#include <Xol/OpenLook.h>

#include <X11/Intrinsic.h>

#include <stdio.h>


#ifdef	__cplusplus
extern "C" {
#endif


#define LNMIN       8
#define LNINCRE    24

#define BufferFilled(b)  (b-> size == b-> used)
#define BufferLeft(b)    (b-> size - b-> used)
#define BufferEmpty(b)   (b-> used == 0)

#define Bufferof(type) \
	struct  { \
		int	size; \
		int	used; \
		int	esize; \
		type*	p; \
	}


typedef char BufferElement;

typedef struct _Buffer {
	int                     size;
	int                     used;
	int                     esize;
	BufferElement*		p;
}  Buffer;


#if	defined(__STDC__) || defined(__cplusplus)

extern Buffer*	AllocateBuffer(int element_size, int initial_size);
extern Buffer*	CopyBuffer(Buffer* buffer);
extern void	FreeBuffer(Buffer* buffer);
extern void	GrowBuffer(Buffer* buffer, int increment);
extern int 	InsertIntoBuffer(Buffer* target, Buffer* source, int offset);
extern int	ReadFileIntoBuffer(FILE* fp, Buffer* buffer);
extern int	ReadStringIntoBuffer(Buffer* sp, Buffer* buffer);

extern void	strclose(Buffer* sp);
extern int	strgetc(Buffer* sp);
extern Buffer*	stropen(char* string);

#else	/* __STDC__ || __cplusplus */

extern Buffer*	AllocateBuffer();
extern Buffer*	CopyBuffer();
extern void	FreeBuffer();
extern void	GrowBuffer();
extern int 	InsertIntoBuffer();
extern int	ReadFileIntoBuffer();
extern int	ReadStringIntoBuffer();

extern void	strclose();
extern int	strgetc();
extern Buffer*	stropen();

#endif	/* __STDC__ || __cplusplus */


#ifdef	__cplusplus
}
#endif


#endif	/* _XOL_BUFFERUTIL_H */
