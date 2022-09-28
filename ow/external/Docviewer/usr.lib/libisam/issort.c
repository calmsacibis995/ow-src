#ifndef lint
        static char sccsid[] = "@(#)issort.c	1.3\t06/11/93 Copyr 1988 Sun Micro";
#endif

/*
 * Copyright (c) 1988 by Sun Microsystems, Inc.
 */

/*
 * issort.c
 *
 * Description:
 *	ISAM sort package (sort in memory)
 */

#include "isam_impl.h"

extern char *_isunix_malloc();

/* 
 * _issort_create() 
 *
 * Create a sorter
 */

Issort *
_issort_create(reclen,nrecs,compfunc)
    int		reclen;
    int		nrecs;
    int		(*compfunc)();
{	
    Issort	*p;

    p = (Issort *)_ismalloc(sizeof(*p));     /* Allocate new sorter */
    memset((char *)p, 0,sizeof(*p));

    p->ist_reclength = reclen;		     /* Record length in bytes */
    p->ist_allocrecs = nrecs;		     /* Maximum number of records */
					     /* that can inserted */
    p->ist_nrecs = 0;			     /* Current number of records */
    p->ist_currec;			     /* Current position */
    p->ist_compf = compfunc;		     /* Comparison function */

    p->ist_array = _ismalloc((unsigned)(reclen * nrecs)); /* Allocate array */
					     /* for storing records */

    return(p);
}

/* 
 * _issort_destroy() 
 *
 * Destroy a sorter 
 */

void
_issort_destroy(srt)
    Issort	*srt;
{
    assert(srt->ist_array != (char *) 0);

    (void) free(srt->ist_array);
    (void) free((char *)srt);
}

/* 
 * _issort_insert() 
 *
 * Insert record to sorter 
 */

void
_issort_insert(srt,record)
    register Issort	*srt;
    char		*record;
{
    assert(srt->ist_nrecs < srt->ist_allocrecs);

    memcpy(srt->ist_array + srt->ist_nrecs * srt->ist_reclength,record,
	   srt->ist_reclength);

    srt->ist_currec = srt->ist_nrecs++;
}

/* 
 * _issort_sort() 
 *
 *  Sort records 
 */

void
_issort_sort(srt)
    Issort	*srt;
{
    if (srt->ist_nrecs > 1)
	qsort(srt->ist_array,srt->ist_nrecs,srt->ist_reclength,srt->ist_compf);

    _issort_rewind(srt);		     /* Rewind for subsequent reads */
}

/* 
 * _issort_rewind() 
 * 
 * Rewind sorter 
 */

void
_issort_rewind(srt)
    Issort	*srt;
{
    srt->ist_currec = 0;
}

/* 
 * _issort_read() 
 * 
 * Read record from sorter 
 */

char *
_issort_read(srt)
    register Issort	*srt;
{
    return((srt->ist_currec < srt->ist_nrecs) ?
	(srt->ist_array + srt->ist_currec++ * srt->ist_reclength) :
	    (char *) 0);
}
