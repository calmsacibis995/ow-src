#ifndef lint
        static char sccsid[] = "@(#)isfd.c	1.3\t06/11/93 Copyr 1988 Sun Micro";
#endif
/*
 * Copyright (c) 1988 by Sun Microsystems, Inc.
 */

/*
 * isfd.c
 *
 * Description:
 *	The ISAM file descriptors (isfd) are used as index to a table of 
 *	pointers to File Access Block objects. Isfd.c maintains the table
 *	of the pointers (isfdtab).
 *
 */

#include "isam_impl.h"

static Fab *isfdtab[MAXISFD];		     /* Table of pointers */


/*
 * _isfd_insert(fab)
 *
 * Insert a pointer to an Fab object to table of ISAM file descriptors.
 * Return an ISAM file descriptor, or NOISFD if the table is full.
 */

Isfd
_isfd_insert(fab)
    Fab			*fab;
{
    register Isfd	i;

    for (i = 0; i < MAXISFD; i++) {
	if (isfdtab[i] == NULL)		     /* Empty entry found */
	    break;
    }

    if (i == MAXISFD)
	return (NOISFD);		     /* isfdtab is full */

    isfdtab[i] = fab;
    return (i);
}


/* 
 * _isfd_find(isfd) 
 *
 * Return a pointer to Fab object associated with the ISAM file 
 * descriptor isfd. If isfd is not a file descriptor of an open ISAM file,
 * return NULL.
 */

Fab *
_isfd_find(isfd)
    register Isfd	isfd;
{
    if (isfd < 0 || isfd >= MAXISFD || isfdtab[isfd] == NULL)
	return (NULL);
    else
	return (isfdtab[isfd]);
}

/*
 * _isfd_delete(isfd)
 *
 * Delete an entry from isfdtab. No check is made the entry exists.
 */

void
_isfd_delete(isfd)
    register Isfd	isfd;
{
    if (isfd >= 0 && isfd < MAXISFD)
	isfdtab[isfd] = NULL;
}

