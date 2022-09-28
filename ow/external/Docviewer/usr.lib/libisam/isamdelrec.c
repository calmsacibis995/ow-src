#ifndef lint
	   static char sccsid[] = "@(#)isamdelrec.c	1.3\t06/11/93 Copyr 1988 Sun Micro";
#endif
/*
 * Copyright (c) 1988 by Sun Microsystems, Inc.
 */

/*
 * isamdelrec.c
 *
 * Description: _amdelrec()
 *	Delete record from ISAM file.
 *	
 *
 */

#include "isam_impl.h"

void _delkeys();

/*
 * _amdelrec(isfhandle, recnum, errcode)
 *
 * _amdelrec() deletes a record from ISAM file.
 *
 * Input params:
 *	isfhandle	Handle of ISAM file
 *	recnum		record number of the record to be deleted
 *
 * Output params:
 *	errcode		error status of the operation
 *
 */

int
_amdelrec(isfhandle, recnum, errcode)
    Bytearray		*isfhandle;
    Recno		recnum;
    struct errcode	*errcode;
{
    Fcb			*fcb = NULL;
    char		recbuf[ISMAXRECLEN];
    int			reclen;
    int			(*rec_read)();
    int			(*rec_delete)();

    _isam_entryhook();

    /*
     * Get FCB corresponding to the isfhandle handle.
     */
    if ((fcb = _openfcb(isfhandle, errcode)) == NULL) {
	_isam_exithook();
	return (ISERROR);
    }

    rec_read = (fcb->varflag?_vlrec_read:_flrec_read);
    rec_delete = (fcb->varflag?_vlrec_delete:_flrec_delete);

    /*
     * Update information in FCB from CNTL page on the disk
     */
    (void)_isfcb_cntlpg_r2(fcb);

    /*
     * We must read the record first to be able to delete keys.
     */
    if (rec_read(fcb, recbuf, recnum, &reclen) != ISOK) {
	_amseterrcode(errcode, ENOREC);
	goto ERROR;
    }

    if (rec_delete(fcb, recnum) != ISOK) {
	_amseterrcode(errcode, ENOREC);
	goto ERROR;
    }

    fcb->nrecords--;

    /*
     * Delete associated entries from all indexes.
     */
    _delkeys(fcb, recbuf, recnum);

    _amseterrcode(errcode, ISOK);

    _issignals_mask();
    _isdisk_commit();
    _isdisk_sync();
    _isdisk_inval();

    /*
     * Update CNTL Page from the FCB.
     */
    (void)_isfcb_cntlpg_w2(fcb);
    _issignals_unmask();

    _isam_exithook();
    return (ISOK);

 ERROR:
    _isdisk_rollback();
    _isdisk_inval();

    /*
     * Restore FCB from CNTL page.
     */
    if (fcb) (void)_isfcb_cntlpg_r2(fcb);

    _isam_exithook();
    return (ISERROR);
}

/*
 * _isdelkeys()
 *
 * Delete key entry from all indexes.
 */

void
_delkeys(fcb, record, recnum)
    register Fcb	*fcb;
    char                *record;
    Recno             	recnum;
{
    int                         nkeys = fcb->nkeys;
    register int                i;

    for (i = 0; i < nkeys; i++) {
        _del1key(fcb, fcb->keys + i, record, recnum);
    }
}      

