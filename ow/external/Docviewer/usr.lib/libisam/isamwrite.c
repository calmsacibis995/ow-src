#ifndef lint
        static char sccsid[] = "@(#)isamwrite.c	1.3\t06/11/93 Copyr 1988 Sun Micro";
#endif
/*
 * Copyright (c) 1988 by Sun Microsystems, Inc.
 */

/*
 * isamwrite.c
 *
 * Description: _amwrite()
 *	Write record to  ISAM file.
 *	
 *
 */

#include "isam_impl.h"

extern long *ismaxlong;

/*
 * _amwrite(isfhandle, record, reclen, curpos, 
 * 	    recnum, errcode)
 *
 * _amwrite() writes a new record to ISAM file.
 *
 * Input params:
 *	isfhandle	Handle of ISAM file
 *	record		record
 *	reclen		length of the record
 *	curpos		current record position
 *
 * Output params:
 *	curpos		new current position
 *	recnum		record number
 *	errcode		error status of the operation
 *
 */

int
_amwrite(isfhandle, record, reclen, curpos, recnum, errcode)
    Bytearray		*isfhandle;
    char		*record;
    int			reclen;
    Bytearray		*curpos;
    Recno		*recnum;
    struct errcode	*errcode;
{
    Fcb			*fcb = NULL;
    Recno		recnum2;
    Crp 		*crp;
    int			err;
    int			(*rec_write)();

    _isam_entryhook();

    /*
     * Get FCB corresponding to the isfhandle handle.
     */
    if ((fcb = _openfcb(isfhandle, errcode)) == NULL) {
	_isam_exithook();
	return (ISERROR);
    }

    rec_write = (fcb->varflag?_vlrec_write:_flrec_write);

    /*
     * Update information in FCB from CNTL page on the disk
     */
    (void)_isfcb_cntlpg_r2(fcb);

    if (rec_write(fcb, record, &recnum2, reclen) == ISERROR) {
	_isfatal_error("_amwrite() cannot write record");
    }

    /*
     * Update all keys.
     */
    if ((err = _addkeys2(fcb, record, recnum2, curpos)) != ISOK) {
	_amseterrcode(errcode, err);	
	goto ERROR;
    }
    
    fcb->nrecords++;
    
    *recnum = recnum2;
    _amseterrcode(errcode, ISOK);

    _issignals_mask();
    _isdisk_commit();
    _isdisk_sync();
    _isdisk_inval();

    /*
     * Return new current record position.
     */
    crp = (Crp *) curpos->data;

    crp->flag = CRP_ON;
    crp->recno = recnum2;

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
     * If error is not EDUPL position undefined.
     */
    if (errcode->iserrno != EDUPL) {
	((Crp *)curpos->data)->flag = CRP_UNDEF;
    }

    /*
     * Restore FCB from CNTL page.
     */
    if (fcb) (void)_isfcb_cntlpg_r2(fcb);

    _isam_exithook();
    return (ISERROR);
}


/*
 * _addkeys()
 *
 * Insert key entry to all indexes.
 *
 * Returns ISOK, or EDUPS.
 */

int _addkeys (fcb, record, recnum)
    register Fcb     	*fcb;
    char                *record;
    Recno	        recnum;
{
    int                         nkeys = fcb->nkeys;
    register int                i;
    int				err;

    for (i = 0; i < nkeys; i++) {
	if ((err = _add1key(fcb, fcb->keys + i, record, recnum, 
			    (char*)NULL)) != ISOK)
	    return (err);
    }

    return (ISOK);
}      

Static int
_addkeys2 (fcb, record, recnum, curpos)
    Fcb			*fcb;
    char                *record;
    Recno	        recnum;
    Bytearray		*curpos;
{
    int                	nkeys = fcb->nkeys;
    register int        i;
    int			err;
    Crp			*crp;
    int			keyid;
    Keydesc2		*keydesc2;

    crp = (Crp *)curpos->data;
    keyid = crp->keyid;

    for (i = 0; i < nkeys; i++) {
	keydesc2 = fcb->keys + i;
        if ((err =_add1key(fcb, keydesc2, record, recnum,
			      (keydesc2->k2_keyid == keyid) ?
			      crp->key : (char *) NULL)) != ISOK)
	    return (err);
    }

    return (ISOK);
}      
