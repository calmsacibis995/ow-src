#ifndef _XOL_RING_H
#define _XOL_RING_H

#pragma ident	"@(#)Ring.h	1.6    92/12/10 include/Xol SMI"    /* OLIT	493	*/

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
 *      Interface of the Ring module
 *
 *	Fixed maximum size, no shrink, lifo insertion, fifo deletion, 
 *	unique elements, unsorted buffer.
 *
 ************************************************************************/


/************************************************************************
 *
 *      Imported interfaces 
 *
 ************************************************************************/

#include <sys/types.h>		/* boolean_t, size_t */

#include <Xol/Datum.h>


#ifdef	__cplusplus
extern "C" {
#endif


/************************************************************************
 *
 *      Module Public Type Definitions
 *
 ************************************************************************/

typedef struct UUOlRing		*_OlRing, _OlRingRec;

/* Expose the data structure internals if debugging. */
#ifdef DEBUG
	#include <Xol/RingP.h>
#endif


/************************************************************************
 *
 *      External Public Interface Declarations
 *
 ************************************************************************/

extern void		_OlRingConstruct(_OlRing* ringp, size_t size);

extern void		_OlRingDestruct(_OlRing* ringp);

extern void		_OlRingInsert(_OlRing* ringp, const _OlDatum datum);

extern boolean_t	_OlRingIsEmpty(const _OlRing ring);

extern void		_OlRingTraverse(const _OlRing ring, 
	_OlDatumTraversalFunc tr_func, void* arg1, void* arg2, void* arg3, 
	void* arg4);

#ifdef	DEBUG
extern void		_OlRingPrint(const _OlRing ring);

extern void	_OlRingTest(const int argc, const char *const argv[]);
#endif	/* DEBUG */


#ifdef	__cplusplus
}
#endif


/* end of Ring.h */
#endif	/* _XOL_RING_H */
