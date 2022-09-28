#ifndef lint
        static char sccsid[] = "@(#)isdelete.c	1.3\t06/11/93 Copyr 1988 Sun Micro";
#endif
/*
 * Copyright (c) 1988 by Sun Microsystems, Inc.
 */

/*
 * isdelete.c
 *
 * Description:
 *	Delete record specified by primary key from ISAM file. 
 */


#include "isam_impl.h"
#include <sys/time.h>
#define ZERO 0
/*
 * err = isdelete(isfd, record)
 *
 * Isdelete() deletes record specified by primary key.
 * All indexes of the ISAM file are updated.
 *
 * Current record position is not changed.
 * isrecnum is set to to the deleted record.
 *
 * Returns 0 if successful, or -1 of any error.
 *
 * Errors:
 *	ELOCKED The record or file has been locked by another process.
 *	ENOTOPEN isfd does not correspond to an open ISAM file, or the
 *		ISAM file was not opened with ISINOUT mode.
 *	ENOREC	The specified record does not exists, or the ISAM file
 *		has not primary key.
 *	EBADKEY The primary alows duplicates.
 */

int 
isdelete(isfd, record)
    int			isfd;
    char		*record;
{
    int			_am_delete();
    register Fab	*fab;
    int			ret;
    int			recnum;
    int			lockflag = 0;

    /*
     * Get File Access Block.
     */
    if ((fab = _isfd_find(isfd)) == NULL) {
	_setiserrno2(ENOTOPEN, '9', '0');
	return (ISERROR);
    }

    /*
     * Check that the open mode was  ISINOUT.
     */
    if (fab->openmode != OM_INOUT) {
	_setiserrno2(ENOTOPEN, '9', '0');
	return (ISERROR);
    }

    /*
     * Call the Access Method or RPC client function, depending whether
     * the file is local or remote.
     */

    if ((ret = _amdelete(&fab->isfhandle,
		      record, &recnum, &fab->errcode)) == ISOK) {
	isrecnum = recnum;		     /* Set isrecnum */
    }

    _seterr_errcode(&fab->errcode);

    return (ret);			     /* Successful write */
}
