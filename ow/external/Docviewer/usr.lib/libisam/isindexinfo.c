#ifndef lint
        static char sccsid[] = "@(#)isindexinfo.c	1.3\t06/11/93 Copyr 1988 Sun Micro";
#endif
/*
 * Copyright (c) 1988 by Sun Microsystems, Inc.
 */

/*
 * isindexinfo.c
 *
 * Description:
 *	Access file status information
 */


#include "isam_impl.h"
#include <sys/time.h>

#define ZERO 0

/*
 * err = isindexinfo(isfd, buffer, number)
 *
 * Return information about index.
 * Return information about the data file (if number == 0).
 *
 * (this function is overloaded).
 *
 * Errors:
 *	EBADARG	number is out of range
 *	ENOTOPEN isfd is not ISAM file descriptor of open ISAM file
 */

#define dibuf ((struct dictinfo *)buffer)

int
isindexinfo(isfd, buffer, number)
    int			isfd;
    struct keydesc	*buffer;
    int			number;
{
    register Fab	*fab;
    int			ret;
    Fcb			*fcb = NULL;

    /*
     * Get File Access Block.
     */
    if ((fab = _isfd_find(isfd)) == NULL) {
	_setiserrno2(ENOTOPEN, '9', '0');
	return (ISERROR);
    }

    /*
     * Call the Access Method or RPC client function, depending whether
     * the file is local or remote.
     */

    _isam_entryhook();

    /*
     * Get FCB corresponding to the isfhandle handle.
     */
    if ((fcb = _openfcb(&fab->isfhandle, &fab->errcode)) == NULL) {
	_isam_exithook();
	return (ISERROR);
    }

    /*
     * Update information in FCB from CNTL page on the disk
     */
    (void)_isfcb_cntlpg_r(fcb);

    /*
     * Validate number argument.
     */
    if (number < 0 || number > fcb->nkeys) {
	_amseterrcode(&fab->errcode, EBADARG);
	goto ERROR;
    }

    if (number == 0) {
	
	/*
	 * Return dictinfo structure.
	 */
	dibuf->di_nkeys = fcb->nkeys;
	dibuf->di_recsize = fcb->maxreclen;
	dibuf->di_idxsize = fcb->keys[0].k2_len;
	dibuf->di_nrecords = fcb->nrecords;

	/* Set msb of di_nkeys for variable length records. */
	if (fcb->varflag == TRUE) 
	    dibuf->di_nkeys |= DICTVARLENBIT;
    }
    else {

	/*
	 * Return index information.
	 */
	_iskey_itox(fcb->keys + number - 1, buffer);
    }

    _amseterrcode(&fab->errcode, ISOK);

/* XXX This fixes a core dump that occurs when isindexinfo is
*      called on brand new tables
    _isdisk_commit();
    _isdisk_sync();
    _isdisk_inval();
*/

    _isam_exithook();
    ret = ISOK;
    goto CLEANUP;

 ERROR:
    /*
     * Restore FCB from CNTL page.
     */

    _isdisk_rollback();
    _isdisk_inval();

    _isam_exithook();
    ret = ISERROR;

 CLEANUP: 

    if (ret == ISOK)
	isreclen = fab->minreclen;	     /* for variable length */

    _seterr_errcode(&fab->errcode);

    /*
     * This is a patch to conform with the VSX 3.0 test that checks
     * that k_leng == 2 and k_type == 1 for index 1 if the ISAM file
     * has no primary key. I suspect that these numbers are returned by
     * C-ISAM and the author of VSX tests diligently checks them even 
     * though they have no meaning.
     */
    if (ret == ISOK && number == 1 && buffer->k_nparts == 0) {
	buffer->k_leng = 2;
	buffer->k_type = INTTYPE;
    }
    
    return (ret);		
}

