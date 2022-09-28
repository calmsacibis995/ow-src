#pragma ident	"@(#)Ring.c	1.7    97/03/26 lib/libXol SMI"    /* OLIT	493	*/

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
 * 	Implementation of the Ring module
 *
 ************************************************************************/


/************************************************************************
 *
 *      Imported interfaces 
 *
 ************************************************************************/

#include <stdio.h>		/* NULL, printf(), scanf(), putchar(), EOF */
#include <errno.h>		/* errno */
#include <sys/types.h>		/* boolean_t */

#include <Xol/Datum.h>
#include <Xol/diags.h>		/* _OL_MALLOC(), _OL_FREE() */

#include <Xol/RingP.h>
#include <Xol/Ring.h>		/* Interface of this module */


/************************************************************************
 *
 *      Forward Declarations
 *
 ************************************************************************/

static boolean_t	in(const _OlRing ring, const _OlDatum datum);


/************************************************************************
 *
 *      Implementation of this module's public functions
 *
 ************************************************************************/


/************************************************************************
 *
 *      _OlRingConstruct -- Initialize the ring
 *
 ************************************************************************/

void
_OlRingConstruct(_OlRing* ringp, size_t size)
{
	_OlRing		ring;
	size_t		i;
	
	_OL_MALLOC(ring, _OlRingRec);
	ring->size = size;
	ring->count = 0;
	ring->first = 0;
	ring->last = 0;

	_OL_CALLOC(ring->data, ring->size, _OlDatum);
	for (i = 0; i < ring->size; ++i) ring->data[i] = (_OlDatum)NULL;
	
	*ringp = ring;
}


/************************************************************************
 *
 *      _OlRingDestruct -- Destroy the ring
 *
 ************************************************************************/

void
_OlRingDestruct(_OlRing* ringp)
{
	if (NULL != ringp && NULL != *ringp) {
		_OlRing		ring = *ringp;
		size_t		i;

		for (i = 0; i < ring->count; ++i) 
			_OlDatumDestruct(&ring->data[i]);

		_OL_FREE(ring->data);
		_OL_FREE(ring);
	}
}


/************************************************************************
 *
 *      _OlRingInsert -- Insert datum into the ring
 *
 ************************************************************************/

void
_OlRingInsert(_OlRing* ringp, const _OlDatum datum)
{
	
	if (NULL == ringp || NULL == *ringp)
		_OlAbort(NULL, "Invalid ring pointer passed to "
			"_OlRingInsert().\n");
		/*NOTREACHED*/
	else if ((*ringp)->count > (*ringp)->size)
		_OlAbort(NULL, "Ring data found corrupted in "
			"_OlRingInsert().\n");
		/*NOTREACHED*/
	else if (!in(*ringp, datum)) {
		_OlRing		ring = *ringp;
		_OlDatum	new_datum;
		
		_OlDatumConstruct(&new_datum, datum);

		if (ring->count < ring->size) {
			ring->last = ring->count++;
			ring->data[ring->last] = new_datum;
		} else {
			/* count == size */
			ring->last = ++ring->last % ring->size;
			ring->first = ++ring->first % ring->size;
			
			_OlDatumDestruct(&ring->data[ring->last]);
			ring->data[ring->last] = new_datum;
		}
	}		
}


/************************************************************************
 *
 *      _OlRingIsEmpty -- B_TRUE if ring is empty
 *
 ************************************************************************/

boolean_t
_OlRingIsEmpty(const _OlRing ring)
{

	if (NULL == ring)
		_OlAbort(NULL, "NULL ring pointer passed to "
			"_OlRingIsEmpty().\n");
		/*NOTREACHED*/
	else
		return (boolean_t)((size_t)0 == ring->count);
}


/************************************************************************
 *
 *      _OlRingTraverse -- most recent-first traversal of the ring
 *
 ************************************************************************/

/*ARGSUSED2*/
void
_OlRingTraverse(const _OlRing ring, _OlDatumTraversalFunc tr_func, void* arg1,
	void* arg2, void* arg3, void* arg4)
{

	if (NULL == ring)
		_OlAbort(NULL, "NULL ring pointer passed to "
			"_OlRingPrint().\n");
		/*NOTREACHED*/
	else {
		int	i, n;
		
		for (i = ring->last, n = 0; n < ring->count;  
				i = (--i + ring->size) % ring->size, ++n)
			(*tr_func)(ring->data[i], arg1, arg2, arg3, arg4);
	}
}


#ifdef	DEBUG
/************************************************************************
 *
 *      _OlRingPrint -- most recent-first dump of the ring
 *
 ************************************************************************/

void
_OlRingPrint(const _OlRing ring)
{

	(void) printf("\nRing:  (newest ... oldest)\n");
	_OlRingTraverse(ring, (_OlDatumTraversalFunc)_OlDatumPrint, NULL, NULL, 
		NULL, NULL);
}
#endif	/* DEBUG */


/************************************************************************
 *
 *      Implementation of the module's private functions
 *
 ************************************************************************/
 

/************************************************************************
 *
 *      in -- datum is found in the ring
 *
 ************************************************************************/

static boolean_t
in(const _OlRing ring, const _OlDatum datum)
{
	boolean_t	found;
	size_t		i;

	for (found = B_FALSE, i = 0; !found && i < ring->count; ++i)
		found = (0 == _OlDatumCompare(ring->data[i], datum, NULL));
	
	return found;
}


#ifdef	DEBUG
/************************************************************************
 *
 *	Self-test
 *
 ************************************************************************/

/*ARGSUSED1*/
void
_OlRingTest(
	const int		argc, 
	const char* const	argv[]		/* unused */
)
{
	_OlRing			ring = NULL;
	_OlDatumRec		value_buffer;
	char			string[64];
	int			i;
	int			nc = 0;
	const char *const	test_data[] = { "black", "black", "blue", 
					"green", "red", "green", "red", "red" };

	errno = 0;

	value_buffer.type = _OL_DATUM_TYPE_STRING;

	_OlRingConstruct(&ring, 3);
	(void) printf("Ring is%s empty.\n", _OlRingIsEmpty(ring) ? "" : " not");

	for (i = 0; value_buffer.content = (void*)test_data[i],
				i < XtNumber(test_data);
			++i) {
		_OlRingInsert(&ring, (_OlDatum)&value_buffer);
		_OlRingPrint(ring);
	}
	_OlRingDestruct(&ring);

	if (argc > 1) {
		_OlRingConstruct(&ring, 3);
		
		(void) printf("Enter one string:  ");
		while  ((nc = scanf("%s", &string)) != EOF) {
			if (nc != 1)
				_OlAbort(NULL, "error -- read %d integers; "
					"expected 1.\n", nc);
			else {
				value_buffer.content = (void*)string;
				_OlRingInsert(&ring,(_OlDatum)&value_buffer);
				_OlRingPrint(ring);
			}
			(void) printf("\nEnter one string:  ");
		}
		(void) putchar('\n');

		_OlRingDestruct(&ring);
	}
}
#endif	/* DEBUG */


/* end of Ring.c */
