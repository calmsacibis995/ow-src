#ifndef lint
        static char sccsid[] = "@(#)isapplmw.c	1.3\t06/11/93 Copyr 1988 Sun Micro";
#endif
/*
 * Copyright (c) 1988 by Sun Microsystems, Inc.
 */

/*
 * isapplmw.c
 *
 * Description:
 *	Write Application magic string
 */


#include "isam_impl.h"
#include <sys/file.h>
#include <sys/time.h>

/*
 * string = isapplmw(isfd)
 *
 * Isapplmw() writes an application specific 'magic string' into ISAM
 * file. The 'file' program (using /etc/magic) may be used to report
 * this magic string.
 *
 */

int 
_isapplmw(isfd, magicstring)
    int			isfd;
    char		*magicstring;
{
    register Fab	*fab;
    int			ret;
    Fcb                 *fcb;
    char                cntl_page[CP_NKEYS_OFF];

    /*
     * Get File Access Block.
     */
    if ((fab = _isfd_find(isfd)) == NULL) {
	_setiserrno2(ENOTOPEN, '9', '0');
	return (ISERROR);
    }

    /*
     * Check that the open mode was ISOUTPUT, or ISINOUT.
     */
    if (fab->openmode != OM_OUTPUT && fab->openmode != OM_INOUT) {
	_setiserrno2(ENOTOPEN, '9', '0');
	return (ISERROR);
    }

    /*
     * Check the length of the magic string
     */
    if ((int)strlen(magicstring) > CP_APPLMAGIC_LEN) {
	_setiserrno2(EBADARG, '9', '0');
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

    /* Write the new data */
    
    _isseekpg(fcb->datfd, ISCNTLPGOFF);
    (void)read(fcb->datfd, cntl_page, sizeof(cntl_page));
    strncpy(cntl_page + CP_APPLMAGIC_OFF, magicstring, CP_APPLMAGIC_LEN);
    _isseekpg(fcb->datfd, ISCNTLPGOFF);
    (void)write(fcb->datfd, cntl_page, sizeof(cntl_page));

    _amseterrcode(&fab->errcode, ISOK);
    _isam_exithook();
    ret = ISOK;

    _seterr_errcode(&fab->errcode);

    return (ret);			     /* Successful write */
}
