#ifndef lint
        static char sccsid[] = "@(#)isaddprimary.c	1.3\t06/11/93 Copyr 1988 Sun Micro";
#endif
/*
 * Copyright (c) 1988 by Sun Microsystems, Inc.
 */

/*
 * isaddprimary.c
 *
 * Description:
 *	Add secondary index to ISAM file
 */


#include "isam_impl.h"
#include <sys/time.h>

/*
 * err = isaddprimary(isfd, keydesc)
 *
 * Isaddprimary() is used to add primary index to ISAM file. 
 *
 * This call is not part of the X/OPEN sprec.
 *
 * Errors:
 *	EBADKEY	error in keydesc	
 *	EDUPL	there are duplicate keys and the keydesc does not allow
 *		duplicate keys
 *	EKEXISTS key with the same key descriptor already exists
 *	EKEXISTS the ISAM file already has a primary index.
 *	ENOTEXCL ISAM file is not open in exclusive mode
 *	ENOTOPEN the ISAM file is not open in ISINOUT mode.
 *	EACCES	Cannot create index file because of UNIX error.
 */

int 
isaddprimary(isfd, keydesc)
    int			isfd;
    struct keydesc	*keydesc;
{
    int			_am_addprimary();
    register Fab	*fab;
    int			ret;

    /*
     * Get File Access Block.
     */
    if ((fab = _isfd_find(isfd)) == NULL) {
	_setiserrno2(ENOTOPEN, '9', '0');
	return (ISERROR);
    }

    /*
     * Check that the open mode was ISOUTPUT
     */
    if (fab->openmode != OM_INOUT) {
	_setiserrno2(ENOTOPEN, '9', '0');
	return (ISERROR);
    }

    ret = _am_addprimary(fab, keydesc);
    _seterr_errcode(&fab->errcode);

    return (ret);			     /* Successful write */
}

Static int _am_addprimary(fab, keydesc)
    register Fab	*fab;
    struct keydesc	*keydesc;
{
    return (_amaddprimary(&fab->isfhandle, keydesc, &fab->errcode));
}
