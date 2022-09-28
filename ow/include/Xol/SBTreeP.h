#ifndef _XOL_SBTREEP_H
#define _XOL_SBTREEP_H

#pragma	ident	"@(#)SBTreeP.h	1.4	92/11/09 include/Xol SMI"	/* OLIT	493 */

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
 *       Internal definitions for the Sorted Binary Tree module
 *
 ************************************************************************/


/************************************************************************
 *
 *      Imported interfaces 
 *
 ************************************************************************/

#include <sys/types.h>		/* size_t */

#include <Xol/Datum.h>


#ifdef	__cplusplus
extern "C" {
#endif


/************************************************************************
 *  
 *      Module Private Type Definitions
 *	(except when debugging)
 *
 ************************************************************************/

/*
 * Definition of the data structure to hold a node.
 */
typedef struct UUOlSBTNodeRec {
	_OlDatum		datum;
	cnt_t			reference_count;
	struct UUOlSBTNodeRec*	lesser_node;
	struct UUOlSBTNodeRec*	greater_node;
} *_OlSBTNode, _OlSBTNodeRec;


#ifdef	__cplusplus
}
#endif


/* end of SBTreeP.h */
#endif	/* _XOL_SBTREEP_H */
