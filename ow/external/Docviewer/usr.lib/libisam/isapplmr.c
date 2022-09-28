#ifndef lint
        static char sccsid[] = "@(#)isapplmr.c	1.3\t06/11/93 Copyr 1988 Sun Micro";
#endif
/*
 * Copyright (c) 1988 by Sun Microsystems, Inc.
 */

/*
 * isapplmr.c
 *
 * Description:
 *	Read Application magic string
 */


#include "isam_impl.h"
#include <sys/file.h>
#include <sys/time.h>
#include "isam_impl.h"

/*
 * string = isapplmr(isfd)
 *
 * Isapplmr() returns the aplication magic string written to the ISAM file
 * by isapplmw() function. No magic string is returned as "". The value
 * of -1 returned indicates an error (iserrno is set).
 *
 */

int 
_isapplmr(isfd, buffer)
    int			isfd;
    char		*buffer;
{
    register Fab	*fab;
    Fcb                 *fcb = NULL;
    char                cntl_page[CP_NKEYS_OFF];

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
	return (ISERROR);
    }

    _isseekpg(fcb->datfd, ISCNTLPGOFF);
    (void)read(fcb->datfd, cntl_page, sizeof(cntl_page));
    strncpy(buffer, cntl_page + CP_APPLMAGIC_OFF, CP_APPLMAGIC_LEN);

    _amseterrcode(&fab->errcode, ISOK);
    _isam_exithook();

    _seterr_errcode(&fab->errcode);

    return (ISOK);			     /* Successful write */
}
