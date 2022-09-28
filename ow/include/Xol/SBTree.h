#ifndef _XOL_SBTREE_H
#define _XOL_SBTREE_H

#pragma	ident	"@(#)SBTree.h	1.5	92/12/10 include/Xol SMI"	/* OLIT	493 */

/*
 *	Copyright (C) 1992  Sun Microsystems, Inc
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


/************************************************************************
 *
 *      Interface of the Sorted Binary Tree module
 *
 ************************************************************************/


/************************************************************************
 *
 *      Imported interfaces 
 *
 ************************************************************************/

#include <sys/types.h>		/* boolean_t */

#include <Xol/Datum.h>


#ifdef	__cplusplus
extern "C" {
#endif


/************************************************************************
 *
 *      Module Public Type Definitions
 *
 ************************************************************************/

typedef	struct UUOlSBTNodeRec*	_OlSBTree;

/* Expose the data structure internals if debugging. */
#ifdef DEBUG
#include <Xol/SBTreeP.h>
#endif


/************************************************************************
 *
 *      External Public Interface Declarations
 *
 ************************************************************************/

extern void		_OlSBTreeConstruct(_OlSBTree* topp,
	_OlDatumComparisonFunc cmp_func);

extern void		_OlSBTreeDestruct(_OlSBTree* topp);

extern void		_OlSBTreeInsert(_OlSBTree* topp, const _OlDatum datum);

extern boolean_t	_OlSBTreeIsEmpty(const _OlSBTree top);

#ifdef	NOT_YET
extern void		_OlSBTreeTraverse(const _OlSBTree top, 
	_OlDatumTraversalFunc tr_func, cnt_t nargs, ...);
#endif

extern void		_OlSBTreeTraverse(const _OlSBTree top, 
	_OlDatumTraversalFunc tr_func, void* arg1, void* arg2, void* arg3, 
		void* arg4);

#ifdef	DEBUG
extern void		_OlSBTreePrint(const _OlSBTree top);

extern void	_OlSBTreeTest(const int argc,
	const char *const argv[]);
#endif	/* DEBUG */


#ifdef	__cplusplus
}
#endif


/* end of SBTree.h */
#endif	/* _XOL_SBTREE_H */
