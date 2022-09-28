#ifndef	_XOL_LINKEDLIST_H
#define	_XOL_LINKEDLIST_H

#pragma	ident	"@(#)linkedList.h	302.1	92/03/26 include/Xol SMI"	/* olmisc:linkedList.h 1.1 	*/

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
 * Description:	interface to a generic linked list
 *
 *******************************file*header******************************/


#ifdef	__cplusplus
extern "C" {
#endif


typedef void*		_OlPointer;

typedef struct _ol_list_node {
	_OlPointer              data;
	struct _ol_list_node   *next;
	struct _ol_list_node   *prev;
}                      *_OlListPos;

typedef struct _ol_list {
	struct _ol_list_node   *first;
	struct _ol_list_node   *end;
}                      *_OlList;


#define _OL_NULL_LIST		((_OlList)0)
#define _OL_NULL_LIST_POS	((_OlListPos)0)

#define _OL_LIST_END(l)		((l)->end)
#define _OL_LIST_FIRST(l)	((l)->first)
#define _OL_LIST_GET(l, p)	((p)->next->data)
#define _OL_LIST_IS_EMPTY(l)	((l)->first->next == _OL_NULL_LIST_POS)
#define _OL_LIST_NEXT(l, p)	((p)->next)
#define _OL_LIST_PREV(l, p)	((p)->prev)


extern void		_OlListDelete(_OlList list, _OlListPos pos);
extern void		_OlListDestroy(_OlList list);
extern _OlListPos	_OlListFind(_OlList list, _OlPointer data);

extern _OlListPos	_OlListInsert(_OlList list, _OlListPos pos,
	_OlPointer data);

extern _OlList		_OlListNew(void);


#ifdef	__cplusplus
}
#endif


#endif	/* _XOL_LINKEDLIST_H */
