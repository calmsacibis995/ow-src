#ifndef	_XOL_ARRAY_H
#define	_XOL_ARRAY_H

#pragma	ident	"@(#)array.h	302.1	92/03/26 include/Xol SMI"	/* olmisc:array.h 1.7 	*/

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

/*************************************************************************
 *
 * Description:	OLIT interface to a generic array
 *
 *************************************************************************/


#include <Xol/OpenLook.h>

#include <X11/Intrinsic.h>


#ifdef	__cplusplus
extern "C" {
#endif


#define _OL_NULL_ARRAY		((XtPointer*)0)
#define _OL_NULL_ARRAY_INDEX	-1
#define _OL_ARRAY_IS_EMPTY(A)	((A)->num_elements == 0)
#define _OL_ARRAY_INITIAL	{ 0 , 0 , 0 }


typedef XtPointer	_OlArrayElement;

typedef struct {
	_OlArrayElement*		array;
	Cardinal		num_elements;
	Cardinal		num_slots;
}			_OlArrayRec, *_OlArray;



#define _OlArrayFree(arrayPtr) \
	    { \
		XtFree((char*)(arrayPtr)->array); \
		_OlArrayInit(arrayPtr); \
	    }
	    
#define _OlArrayInit(arrayPtr) \
	    { \
		(arrayPtr)->num_elements = 0; \
		(arrayPtr)->num_slots = 0; \
		(arrayPtr)->array = NULL; \
	    }
	    
#define	_OlArrayAppend(arrayPtr, data) \
		_OlArrayInsert((arrayPtr), (arrayPtr)->num_elements, data)
		
#define	_OlArrayUniqueAppend(arrayPtr, data) \
	    if (_OlArrayFind(arrayPtr,data) == _OL_NULL_ARRAY_INDEX) \
		_OlArrayInsert(arrayPtr, (arrayPtr)->num_elements,data); \
	    else
	    
#define	_OlArrayInsert(arrayPtr, indx, data) \
		UUOlArrayInsert(arrayPtr, indx, (_OlArrayElement)(data))
		
#define	_OlArrayFind(arrayPtr, data) \
		UUOlArrayFind(arrayPtr, (_OlArrayElement)(data))
		
#define _OlArrayItem(arrayPtr, indx)	((arrayPtr)->array[indx])
#define _OlArraySize(arrayPtr)		(arrayPtr)->num_elements


#if	defined(__STDC__) || defined(__cplusplus)

extern void	_OlArrayDelete(_OlArray array, int pos);
extern int	_OlArrayFindByName(_OlArray array, String name);

/* 
 * WERE __OlArrayFind() AND __OlArrayInsert() 
 * WHICH YIELD WARNINGS FOR ALL C++ LIBRARY USERS 
 */
extern int	UUOlArrayFind(_OlArray array, _OlArrayElement data);
extern void	UUOlArrayInsert(_OlArray array, int pos, _OlArrayElement data);

#else	/* __STDC__ || __cplusplus */

extern void	_OlArrayDelete();
extern int	_OlArrayFindByName();
extern int	UUOlArrayFind();
extern void	UUOlArrayInsert();

#endif	/* __STDC__ || __cplusplus */


#ifdef	__cplusplus
}
#endif


#endif	/* _XOL_ARRAY_H */
