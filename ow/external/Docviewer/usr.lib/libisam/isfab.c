#ifndef lint
        static char sccsid[] = "@(#)isfab.c	1.3\t06/11/93 Copyr 1988 Sun Micro";
#endif
/*
 * Copyright (c) 1988 by Sun Microsystems, Inc.
 */

/*
 * isfab.c
 *
 * Description:
 *	The ISAM file access block functions.
 *
 */

#include "isam_impl.h"


/*
 * fab = *fab_new(hostname, localpath, remote, mode)
 *
 * Fab_new() creates an File access block (fab object) that is used
 * for all subsequent operations in this file. Return a pointer to 
 * the fab object, or NULL in the case of an error.
 */

Fab *
_fab_new(hostname, localpath, remote, openmode,
	 varlen, minreclen, maxreclen)
    char		*hostname;	     /* IP address of ISAM file host */
    char		*localpath;	     /* Local path on the host */
    int			remote;		     /* 1 if remote, 0 local */
    enum openmode	openmode;
    Bool		varlen;		     /* 0/1 flag */
    int			minreclen, maxreclen;
{
    register Fab	*fab;

    /* Allocate memory for the fab object. */
    fab = (Fab *) _ismalloc(sizeof(*fab));
    memset((char *)fab, 0, sizeof(*fab));

    /* Set fields in the fab objects. */
    fab->openmode = openmode;
    fab->varlength = varlen;
    fab->minreclen = minreclen;
    fab->maxreclen = maxreclen;
    fab->isamhost = _isallocstring(hostname);
    fab->localpath = _isallocstring(localpath);

    if (fab == NULL ||
	fab->isamhost == NULL ||
	fab->localpath == NULL) {
      iserrno = EFATAL;
      return (NULL);
     }

    return (fab);
}

void
_fab_destroy(fab)
    register Fab	*fab;
{
    assert(fab != NULL);
    assert(fab->isamhost != NULL);
    assert(fab->localpath != NULL);

    _isfreestring(fab->isamhost);
    _isfreestring(fab->localpath);

    _bytearr_free(&fab->isfhandle);
    _bytearr_free(&fab->curpos);

    free((char *)fab);
}
