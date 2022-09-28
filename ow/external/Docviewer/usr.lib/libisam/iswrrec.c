#ifndef lint
        static char sccsid[] = "@(#)iswrrec.c	1.3\t06/11/93 Copyr 1988 Sun Micro";
#endif
/*
 * Copyright (c) 1988 by Sun Microsystems, Inc.
 */

/*
 * iswrrec.c
 *
 * Description:
 *	Write a record to ISAM file. 
 */

#include "isam_impl.h"
#include <sys/time.h>

static int _amwrrec();

/*
 * isfd = iswrrec(isfd, recnum, record)
 *
 * Iswrrec() writes a record to ISAM file at specified recno position. 
 * All indexes of the ISAM file are updated.
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
 *	EDUPL	The write would result in a duplicate on a key that
 *		does not allow duplicates.
 *	ELOCKED The file has been locked by another process.
 *	ENOTOPEN isfd does not correspond to an open ISAM file, or the
 *		ISAM file was not opened with ISINOUT mode.
 *	EKEXISTS Record with recnum already exists.
 *	EBADARG recnum is negative.
 */

int 
iswrrec(isfd, recnum, record)
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
	_setiserrno2(EBADARG, '9' ,'0');
	return (ISERROR);
    }
   
    if ((ret = _amwrrec(&fab->isfhandle, record, reclen,
			recnum, &fab->errcode)) == ISOK) {
	isrecnum = recnum;		     /* Set isrecnum */
    }

    _seterr_errcode(&fab->errcode);

    return (ret);			     /* Successful write */
}

/*
 * _amwrrec(isfhandle, record, reclen, recnum, errcode)
 *
 * _amwrrec() rewrites a record in ISAM file.
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
_amwrrec(isfhandle, record, reclen, recnum, errcode)
    Bytearray		*isfhandle;
    char		*record;
    int			reclen;
    Recno		recnum;
    struct errcode	*errcode;
{
    Fcb			*fcb = NULL;
    int			err;
    int			(*rec_wrrec)();

    _isam_entryhook();

    /*
     * Get FCB corresponding to the isfhandle handle.
     */
    if ((fcb = _openfcb(isfhandle, errcode)) == NULL) {
	_isam_exithook();
	return (ISERROR);
    }

    rec_wrrec = (fcb->varflag?_vlrec_wrrec:_flrec_wrrec);

    /*
     * Update information in FCB from CNTL page on the disk
     */
    (void)_isfcb_cntlpg_r2(fcb);

    if ((err = rec_wrrec(fcb, record, recnum, reclen)) != ISOK) {
	_amseterrcode(errcode, err);
	goto ERROR;
    }

    /*
     * Update all keys.
     */
    if ((err = _addkeys(fcb, record, recnum)) != ISOK) {
	_amseterrcode(errcode, err);	
	goto ERROR;
    }

    fcb->nrecords++;

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


