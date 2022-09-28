#ifndef lint
        static char sccsid[] = "@(#)isclose.c	1.3\t06/11/93 Copyr 1988 Sun Micro";
#endif
/*
 * Copyright (c) 1988 by Sun Microsystems, Inc.
 */

/*
 * isclose.c
 *
 * Description:
 *	Close an ISAM file
 */

#include "isam_impl.h"
#include <sys/time.h>

/*
 * isclose(isfd)
 *
 * Isclose() closes the ISAM file associated with the file descriptor isfd.
 * There may be other ISAM file descriptors in use which are associated 
 * with the same ISAM file; are not effected by isclose().
 *
 * Isclose() returns -1 if an errors was detected, or 0 if the file was closed
 * successfully.
 *
 * Errors:
 *	ENOTOPEN isfd is not ISAM file descriptor of an open ISAM file.
 */

int
isclose(isfd)
    int		isfd;
{
    Fab		*fab;
    Fcb         *fcb;

    if ((fab = _isfd_find(isfd)) == NULL) {
	_setiserrno2(ENOTOPEN, '9', '0');
	return (ISERROR);
    }

    _isam_entryhook();

    /*
     * Get FCB corresponding to the isfhandle handle.
     */
    if ((fcb = _openfcb(&fab->isfhandle, &fab->errcode)) == NULL) {
	_isam_exithook();
	return (ISERROR);
    }

    /*
     * Delete FCB and remove it from FCB cache. Close UNIX fds.
     *
     * This is desirable when ISAM files are removed from other processes,
     * or simply by 'rm file.*'. 
     */
    (void) _watchfd_decr(_isfcb_nfds(fcb));
    _isfcb_close(fcb);
    _mngfcb_delete(&fab->isfhandle);

    _isam_exithook();

    _fab_destroy(fab);			     /* Deallocate Fab object */
    _isfd_delete(isfd);
    
    return (ISOK);
}
