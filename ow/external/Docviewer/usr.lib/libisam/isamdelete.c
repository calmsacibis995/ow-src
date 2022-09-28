#ifndef lint
        static char sccsid[] = "@(#)isamdelete.c	1.3\t06/11/93 Copyr 1988 Sun Micro";
#endif
/*
 * Copyright (c) 1988 by Sun Microsystems, Inc.
 */

/*
 * isamdelete.c
 *
 * Description: _amdelete()
 *	Delete record from ISAM file.
 *	
 *
 */

#include "isam_impl.h"

void _delkeys();

/*
 * _amdelete(isfhandle, lockflag, record, recnum, errcode)
 *
 * _amdelete() deletes a record from ISAM file.
 *
 * Input params:
 *	clientid	Identifies the client
 *	isfhandle	Handle of ISAM file
 *	lockflag	Lock flag
 *	record		Extract key fields from this buffer
 *
 * Output params:
 *	errcode		error status of the operation
 *	recnum		record number of the record that was deleted
 *
 */

/* lockflag and clientid are not used */
/*ARGSUSED*/
int
_amdelete(isfhandle, record, recnum, errcode)
    Bytearray		*isfhandle;
    Recno		*recnum;
    char		*record;
    struct errcode	*errcode;
{
    Fcb			*fcb = NULL;
    char		recbuf[ISMAXRECLEN];
    Recno		recnum2;
    int			err;
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


    if ((err = _isprim_to_recno(fcb, record, &recnum2)) != ISOK) {
	_amseterrcode(errcode, err);
	goto ERROR;
    }

    /*
     * We must read the record first to be able to delete keys.
     */
    if (rec_read(fcb, recbuf, recnum2, &reclen) != ISOK) {
	_amseterrcode(errcode, ENOREC);
	goto ERROR;
    }

    if (rec_delete(fcb, recnum2) != ISOK) {
	_amseterrcode(errcode, ENOREC);
	goto ERROR;
    }

    fcb->nrecords--;

    /*
     * Delete associated entries from all indexes.
     */
    _delkeys(fcb, recbuf, recnum2);

    *recnum = recnum2;
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
    if (fcb) {
	(void)_isfcb_cntlpg_r2(fcb);
    }
    _isam_exithook();
    return (ISERROR);
}

