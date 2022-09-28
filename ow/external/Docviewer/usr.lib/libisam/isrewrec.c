#ifndef lint
        static char sccsid[] = "@(#)isrewrec.c	1.3\t06/11/93 Copyr 1988 Sun Micro";
#endif
/*
 * Copyright (c) 1988 by Sun Microsystems, Inc.
 */

/*
 * isrewrec.c
 *
 * Description:
 *	Rewrite a record in ISAM file. 
 */


#include "isam_impl.h"
#include <sys/time.h>

static int _amrewrec(), changekeys();

/*
 * err = isrewrec(isfd, recnum, record)
 *
 * Isrewrec() modifies a record in ISAM file. The record is identified 
 * by its record number. All indexes of the ISAM file are updated.
 *
 * Current record position is not changed.
 * isrecnum is set to recnum.
 *
 * If the ISAM file is for variable length records, the isreclen variable
 * must be set to indicate the actual length of the record, which must
 * be between the minimum and maximum length, as specified in isbuild().
 *
 * Returns 0 if successful, or -1 of any error.
 *
 * Errors:
 *	EDUPL	The write woul result in a duplicate on a key that
 *		does not allow duplicates.
 *	ELOCKED The file has been locked by another process.
 *	ENOTOPEN isfd does not correspond to an open ISAM file, or the
 *		ISAM file was not opened with ISINOUT mode.
 *	ENOREC	Record with record number recnum does not exist.
 */

int 
isrewrec(isfd, recnum, record)
    int			isfd;
    long		recnum;
    char		*record;
{
    register Fab	*fab;
    int			reclen;
    int			ret;

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
     * Determine record length. Check it against min and max record length.
     */
    reclen = (fab->varlength == TRUE) ? isreclen : fab->minreclen;
    if (reclen < fab->minreclen || reclen > fab->maxreclen) {
	_setiserrno2(EBADARG, '9', '0');
	return (ISERROR);
    }

    if ((ret = _amrewrec(&fab->isfhandle,  record,
			 reclen, recnum, &fab->errcode)) == ISOK) {
	isrecnum = recnum;		     /* Set isrecnum */
    }

    _seterr_errcode(&fab->errcode);

    return (ret);			     /* Successful write */
}

/*
 * _amrewrec(isfhandle, record, reclen, recnum, errcode)
 *
 * _amrewrec() rewrites a record in ISAM file.
 *
 * Input params:
 *	isfhandle	Handle of ISAM file
 *	record		record
 *	reclen		length of the record
 *	recnum		record number of record to be deleted
 *
 * Output params:
 *	errcode		error status of the operation
 *
 */

static int
_amrewrec(isfhandle, record, reclen, recnum, errcode)
    Bytearray		*isfhandle;
    char		*record;
    int			reclen;
    Recno		recnum;
    struct errcode	*errcode;
{
    Fcb			*fcb;
    char		oldrecbuf[ISMAXRECLEN];
    int			err;
    int			reclen2;
    int			(*rec_read)();
    int			(*rec_rewrite)();

    _isam_entryhook();

    /*
     * Get FCB corresponding to the isfhandle handle.
     */
    if ((fcb = _openfcb(isfhandle, errcode)) == NULL) {
	_isam_exithook();
	return (ISERROR);
    }

    rec_read = (fcb->varflag?_vlrec_read:_flrec_read);
    rec_rewrite = (fcb->varflag?_vlrec_rewrite:_flrec_rewrite);

    /*
     * Update information in FCB from CNTL page on the disk
     */
    (void)_isfcb_cntlpg_r2(fcb);

    /*
     * We must read the record first to be able to delete keys.
     */
    if (rec_read(fcb, oldrecbuf, recnum, &reclen2) != ISOK) {
	_amseterrcode(errcode, ENOREC);
	goto ERROR;
    }

    if (rec_rewrite(fcb, record, recnum, reclen) != ISOK) {
	_amseterrcode(errcode, ENOREC);
	goto ERROR;
    }

    /*
     * Update all keys.
     */
    if ((err = _changekeys(fcb, record, oldrecbuf, recnum)) != ISOK) {
	_amseterrcode(errcode, err);	
	goto ERROR;
    }

    _amseterrcode(errcode, ISOK);

    _issignals_mask();
    _isdisk_commit();
    _isdisk_sync();
    _isdisk_inval();

    /*
     * Update control page.
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
     (void)_isfcb_cntlpg_r2(fcb);   

    _isam_exithook();
    return (ISERROR);
}

static int
_changekeys (fcb, record, oldrecord, recnum)
    Fcb			*fcb;
    char                *record;
    char                *oldrecord;
    Recno	        recnum;
{
    int                	nkeys = fcb->nkeys;
    register int        i;
    int			err;

    for (i = 0; i < nkeys; i++) {
        if ((err =_change1key(fcb, fcb->keys + i, record, oldrecord, recnum,
			      (char *)NULL)) 
	    != ISOK)
	    return (err);
    }

    return (ISOK);
}      
