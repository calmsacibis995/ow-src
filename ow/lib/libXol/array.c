#pragma ident	"@(#)array.c	302.3	97/03/26 lib/libXol SMI"	/* olmisc:array.c 1.5	*/

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
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*************************************************************************
 *
 * Description:	implements a generic linked array
 *
 *******************************file*header******************************/


#include <Xol/array.h>

#include <stdio.h>
#include <stdlib.h>


/*************************************************************************
 *
 * Define global/static variables and #defines, and
 * Declare externally referenced variables
 *
 *****************************file*variables*****************************/

/*************************************************************************
 *
 * Forward Procedure definitions arrayed by category:
 *		1. Private Procedures
 *		2. Public  Procedures
 *
 **************************forward*declarations**************************/

					/* private procedures		*/

					/* public procedures		*/

/*************************************************************************
 *
 * Private Procedures
 *
 ***************************private*procedures***************************/

/*************************************************************************
 *
 * Public Procedures
 *
 ***************************private*procedures***************************/

/***************************function*header*******************************
 * _OlArrayDelete - Deletes node at `pos' from `array'
 */
void
_OlArrayDelete (array, pos)
_OlArray		array;
int			pos;
{
    if ((array != NULL) && (array->num_elements != 0) &&
			(pos >= 0) && (pos < array->num_elements))
    {
	register int i;

	array->num_elements--;		/* one less element */

	for (i = pos; i < array->num_elements; i++) /* ripple down elements */
	{
	    array->array[i] = array->array[i + 1];
	}
    }
} /* _OlArrayDelete() */


/*
 * [WAS __OlArrayFind() WHICH YIELDS WARNINGS FOR ALL C++ LIBRARY USERS]
 *
 * UUOlArrayFind() - Returns position of 'data' in 'array'
 */
int
UUOlArrayFind(_OlArray array, _OlArrayElement data)
{

	if ((array != NULL) && (array->num_elements != 0)) {
		int		i;

		for (i = 0; i < array->num_elements; i++)
			if (array->array[i] == data)
				return (i);
	}

	return (_OL_NULL_ARRAY_INDEX);
}


/***************************function*header*******************************
 * _OlArrayFindByName - returns the position of "name" in "array"
 ***************************function*header*******************************/
int
_OlArrayFindByName (array, name)
 _OlArray		array;
 String			name;
{
	if (array != NULL && array->num_elements != 0)
	{
		register int	i;

		for (i = 0; i < array->num_elements; i++)
			if (!strcmp(XtName((Widget)array->array[i]), name))
				return (i);
	}
	return (_OL_NULL_ARRAY_INDEX);
} /* END OF _OlArrayFindByName() */


/*
 * [WAS __OlArrayInsert() WHICH YIELDS WARNINGS FOR ALL C++ LIBRARY USERS]
 *
 * UUOlArrayInsert() - Insert `data' into `array' before the node at
 * position `pos'
 *
 * The position of the new node is returned if there was not an
 * allocation problem, _OL_NULL_ARRAY_POS is returned otherwise
 */
void
UUOlArrayInsert(_OlArray array, int pos, _OlArrayElement data)
{
	int			i;

	if (array == NULL)
		return;

	if (array->num_elements == array->num_slots) {
		/* Alloc more space */
		array->num_slots += (array->num_slots / 2) + 2;
		array->array = (_OlArrayElement*)XtRealloc(
			(XtPointer)array->array,
			(unsigned)(array->num_slots * sizeof (_OlArrayElement)));
	}
	
	if ((pos < 0) || (pos > array->num_elements))
		pos = array->num_elements;	/* skew 'pos' */

	/* Ripple elements up one space from "pos" */
	for (i = array->num_elements; i > pos; i--) {
		array->array[i] = array->array[i - 1];
	}

	array->array[pos] = data;
	array->num_elements++;
}


