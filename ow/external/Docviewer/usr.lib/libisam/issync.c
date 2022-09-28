#ifndef lint
        static char sccsid[] = "@(#)issync.c	1.3\t06/11/93 Copyr 1988 Sun Micro";
#endif
/*
 * Copyright (c) 1988 by Sun Microsystems, Inc.
 */

/*
 * issync.c
 *
 * Description:
 *	Sync all kernel buffers to the disk.
 *
 * Note: issync() flushes changed kernel buffers that are local to
 * 	the application that issued the call.
 * 
 * See sync(2) UNIX manual for what actually happens if sync() is called.
 */

#include "isam_impl.h"
#include <sys/file.h>
#include <sys/time.h>

/*
 * int  issync()
 */

int 
issync()
{
    return iscntl(ALLISFD, ISCNTL_FSYNC);
}

/*
 * int  isfsync(fd)
 */

int 
isfsync(isfd)
    int		isfd;
{
    return iscntl(isfd, ISCNTL_FSYNC);
}


_issync()
{
    int		i;

    for (i = 0; i < MAXISFD; i++)
	(void)_isfsync(i);

    return (ISOK);
}

_isfsync(isfd)
    int		isfd;
{
    register Fab	*fab;
    Fcb                 *fcb;
    int			ret;

    /*
     * Get File Access Block.
     */
    if ((fab = _isfd_find(isfd)) == NULL) {
	_setiserrno2(ENOTOPEN, '9', '0');
	return (ISERROR);
    }

    /*
     * Check that the open mode was ISINPUT, or ISINOUT.
     */
    if (fab->openmode != OM_INPUT && fab->openmode != OM_INOUT) {
	_setiserrno2(ENOTOPEN, '9', '0');
	return (ISERROR);
    }

    _isam_entryhook();

    /*
     * Get FCB corresponding to the isfhandle handle.
     */
    if ((fcb = _openfcb(&fab->isfhandle, &fab->errcode)) == NULL) {
	_isam_exithook();
	ret = ISERROR;
    }
    else {

        if (fcb->datfd != -1)
	    (void)fsync(fcb->datfd);

        if (fcb->indfd != -1)
	    (void)fsync(fcb->indfd);

        if (fcb->varfd != -1)
	    (void)fsync(fcb->varfd);

        _amseterrcode(&fab->errcode, ISOK);
        _isam_exithook();
        ret = ISOK;
    }

    _seterr_errcode(&fab->errcode);

    return (ret);			     /* Successful write */
}
