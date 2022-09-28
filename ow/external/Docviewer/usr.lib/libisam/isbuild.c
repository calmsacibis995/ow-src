#ifndef lint
        static char sccsid[] = "@(#)isbuild.c	1.3\t06/11/93 Copyr 1988 Sun Micro";
#endif
/*
 * Copyright (c) 1988 by Sun Microsystems, Inc.
 */

/*
 * isbuild.c
 *
 * Description:
 *	Create an ISAM file. 
 */


#include "isam_impl.h"
#include <netdb.h>
#include <sys/file.h>
#include <sys/time.h>

extern char _isam_version[];
static char *_version_ = _isam_version;

static int _ambuild();

/*
 * isfd = isbuild(isfname, recordlength, primkey, mode)
 *
 * Isbuild() determines on which machine the ISAM file is to be built,
 * checks the permissions for creating a file by this user running on
 * this client machine by using the access(2) UNIX call.
 * If the file is remote, it is created by the netisamd daemon running
 * on the machine hosting the ISAM file. The chown(2) is then used to
 * change the ownership of the file to the client.
 * All UNIX files created will have their permissions set to 0666, allowing
 * thus both read and write access to anybody.
 *
 * Isbuild() returns an ISAM file descriptor (isfd) is the call was successful, 
 * or a value of -1 if the call failed.
 *
 * Errors:
 *	EBADARG	Improper mode parameter
 *	EBADARG	isreclen >= recordlength and ISVARLEN specified
 *	E2BIG recordlength greater than system imposed limit (8196)
 *	EBADKEY Invalid key descriptor
 *	EBADFILE ISAM file is corrupted or it is not an NetISAM file
 *	EFNAME	Invalid ISAM file name 
 *	ETOOMANY Too many ISAM file descriptors are in use (128 is the limit)
 *
 * The following error numbers are "borrowed" from UNIX.
 *	EACCES	UNIX file system protection denies creation of the file
 *	EEXIST - ISAM file already exists
 *	EEXIST - A UNIX file with the same name exists
 */

int
isbuild(isfname, recordlength, primkey, mode)
    char		*isfname;
    int			recordlength;
    struct keydesc	*primkey;
    int			mode;
{
    char		hostname[MAXHOSTNAMELEN];
    char		localpath[MAXPATHLEN];
    int			remote;		     /* 1 if the file is remote */
    int			rdonly;		     /* 1 if file system is "ro" */
    Fab			*fab;
    Isfd		isfd;
    enum openmode	openmode;
    int			minreclen;	     /* Minimum record length */
    int			origumask;

    /*
     * Check if the user is allowed to access the ISAM file.
     * Use UNIX and NFS permissions.
     */

    /* 
     * Parse NFS filename, resolve all symbolic links, find the host
     * on which the ISAM file is, and the local path on that host.
     * The rdonly flag us not used. A probe access(2) call on the .rec file
     * is used to determine if the access to the file is allowed.
     */
    if (_parse_isfname(isfname, hostname, localpath, &remote, &rdonly,
		       (mode & ISNFS) == ISNFS) != ISOK) {
	_setiserrno2(EFNAME, '9', '0');
	return (NOISFD);
    }
    
    /* Get file open mode part of the mode parameter. */
    if ((openmode = _getopenmode(mode)) == OM_BADMODE) {
	_setiserrno2(EBADARG, '9', '0');
	return (NOISFD);
    }

    /*
     * Minimum record length.
     */

    minreclen = ((mode & ISLENMODE) == ISVARLEN) ? isreclen : recordlength;

    /* Check recordlength against system imposed limit. */
    if (recordlength > ISMAXRECLEN) {
	_setiserrno2(E2BIG, '9', '0');
	return (NOISFD);
    }

    /* Check that ssminreclen >= ISMINRECLEN. */
    if (minreclen < ISMINRECLEN) {
	_setiserrno2(EBADARG, '9', '0');
	return (NOISFD);
    }

    /* Check that minreclen <= recordlength */
    if (minreclen > recordlength) {
	_setiserrno2(EBADARG, '9', '0');
	return (NOISFD);
    }

    /* Create a Fab object. */
    if ((fab = _fab_new(hostname, localpath, remote, openmode,
			(Bool)((mode & ISLENMODE) == ISVARLEN), minreclen,
			recordlength)) == NULL) {
	return (NOISFD);		     /* iserrno is set by fab_new */
    }

    /* Get an ISAM file descriptor for this fab */
    if ((isfd = _isfd_insert(fab)) == NOISFD) {
	/* Table of ISAM file descriptors would overflow. */
	_fab_destroy(fab);
	_setiserrno2(ETOOMANY, '9', '0');
	return (NOISFD);
    }
    FAB_ISFDSET(fab, isfd);

    /*
     * Extract umask. It is send to the Acces Layer (which may reside
     * on a remote machine).
     */
    origumask = umask(0);
    (void)umask(origumask);

    /* 
     * Call lower layers.
     */

    if (_ambuild(fab->localpath, fab->openmode, fab->varlength,
		 fab->minreclen, fab->maxreclen, primkey, getuid(),
		 getgid(), origumask, &fab->isfhandle, &fab->curpos,
		 &fab->errcode)) {
	_seterr_errcode(&fab->errcode);
	_fab_destroy(fab);
	return (NOISFD);
    }

    return ((int)isfd);			     /* Successful isopen() */
}

/*
 * _ambuild(isfname, openmode, varflag, minlen, maxlen,
 *          primkey, owner, group, umask, isfhandle, curpos, errcode)
 *
 * _ambuild() creates a new ISAM file with the name isfname. 
 *
 * Input params:
 * isfname ISAM file name
 * varflag is 0/1 flag set to 1 if the file is for variable lengths records
 * minlen  minimum length of record in bytes
 * maxlen  maximum length of record in bytes
 * primkey definition of the primary key
 * owner, group set the ownership of the file to this user and group
 * umask  application's value of umask
 * 
 * Output params:
 * isfhandle a file handle to be used in subsequent operations on the file
 * curpos  initial current record position
 * errcode {iserrno, isstat1-4} 
 *
 * _ambuild() returns 0 if successful, or -1 to indicate an error.
 */
#define FDNEEDED	3		     /* Needs at most 3 UNIX fds to open ISAM file */

/* ARGSUSED */
static int
_ambuild(isfname, openmode, varflag, minlen, maxlen, primkey, 
	 owner, group, umask, isfhandle, curpos, errcode)
    char		*isfname;
    enum openmode	openmode;
    Bool		varflag;
    int			minlen, maxlen;
    struct keydesc	*primkey;
    int			owner, group;
    Bytearray		*isfhandle;
    Bytearray		*curpos;
    struct errcode	*errcode;
    int			umask;
{
    Fcb			*fcb = NULL;
    Bytearray		*isfhandle2;
    Bytearray		isfhandle0;
    Keydesc2		keydesc2;
    int			err;
    Crp			*crp;

    _isam_entryhook();

    /*
     * Validate the primary key descriptor.
     */
    if (!USE_PHYS_ORDER(primkey) && 
	_validate_keydesc(primkey, minlen) == ISERROR) {
	_amseterrcode(errcode, EBADKEY);
	goto ERROR;
    }

    /*
     * Make isfhandle0.
     */
    isfhandle0 = _makeisfhandle(isfname);

    /* 
     * Check that there is not entry with the same name in FCB cache. 
     */
    if ((fcb = _mngfcb_find(&isfhandle0)) != NULL) {
	fcb = _mngfcb_find(&isfhandle0);
	(void) _watchfd_decr(_isfcb_nfds(fcb));
	_isfcb_close(fcb);
	_mngfcb_delete(&isfhandle0);
    }

    /*
     * Check that there are UNIX file descriptors available.	
     */
    while (_watchfd_check() < FDNEEDED) {
	/*
	 * Find victim (LRU FCB) and close it.
	 */
	if((isfhandle2 = _mngfcb_victim()) == NULL)
	    _isfatal_error ("_openfcb() cannot find LRU victim");

	fcb = _mngfcb_find(isfhandle2);
	(void) _watchfd_decr(_isfcb_nfds(fcb));
	_isfcb_close(fcb);
	_mngfcb_delete(isfhandle2);
    }

    /*
     * Create UNIX files, return isfhandle and File Control Block (fcb).
     */
    if ((fcb = _isfcb_create(isfname, 1, (primkey->k_nparts != 0), (int)varflag, 
			     owner, group, umask, errcode)) == NULL) {
	goto ERROR;
    }
    
    /* 
     * Add length info to the FCB. 
     */
    _isfcb_setreclength(fcb, varflag, minlen, maxlen);
      
    if (!USE_PHYS_ORDER(primkey)) { 
	/*
	 * Convert key descriptor to internal form.
	 */
	_iskey_xtoi (&keydesc2, primkey);
	
	/*
	 * Create index structure.
	 */
	if ((err = _create_index(fcb , &keydesc2)) != ISOK) { 
	    _amseterrcode(errcode, err);
	    goto ERROR;
	}
	
	/*
	 * Add primary key descriptor to FCB.
	 */
	if (_isfcb_primkeyadd(fcb, &keydesc2) == ISERROR) {	   
	    _amseterrcode(errcode, ETOOMANY);
	    goto ERROR;
	}
    }

    /*
     * Initial current record position.
     */
    if (FCB_NOPRIMARY_KEY(fcb)) {
	/* Use physical order. */
	crp = (Crp *) _ismalloc(sizeof(*crp));
	memset ((char *) crp, 0, sizeof(*crp));

	crp->keyid = PHYS_ORDER;
	crp->flag = CRP_BEFOREANY;

	curpos->length = sizeof(*crp);
	curpos->data = (char *) crp;
    }
    else {
	/* 
	 * Use primary key order. 
	 */

	crp = (Crp *) _ismalloc((unsigned)(sizeof(*crp) + fcb->keys[0].k2_len));
	memset((char *) crp, 0, (sizeof(*crp) + fcb->keys[0].k2_len)); 

	crp->keyid = fcb->keys[0].k2_keyid;
	crp->flag = CRP_BEFOREANY;

	_iskey_fillmin(&fcb->keys[0], crp->key);

	curpos->length = sizeof(*crp) + fcb->keys[0].k2_len;
	curpos->data = (char *) crp;
	
	/*
	 * Set full key length as the number of bytes to match in key comparison
	 */
	crp->matchkeylen = fcb->keys[0].k2_len - RECNOSIZE;
	
	if (ALLOWS_DUPS2(&fcb->keys[0]))
	    crp->matchkeylen -= DUPIDSIZE;
    }

    _amseterrcode(errcode, ISOK);

    /*
     * Register the number of UNIX fd consumed.
     */
    (void) _watchfd_incr(_isfcb_nfds(fcb));

    /*
     * Insert new entry into FCB cache.
     */
    _mngfcb_insert(fcb, &isfhandle0);
    *isfhandle = isfhandle0;

    /* Commit all work in disk cache. */
    _issignals_mask();
    _isdisk_commit();
    _isdisk_sync();
    _isdisk_inval();

    /* 
     * Create Control Page (CNTLPAGE). 
     */
    if (_isfcb_cntlpg_w(fcb) == ISERROR) {
	_issignals_unmask();
	goto ERROR;
    }
    
    _issignals_unmask();
    _isam_exithook();
    return (ISOK);

 ERROR:
    if (fcb != NULL) {
	_isfcb_remove(fcb);
	_isfcb_close(fcb);
    }

    _bytearr_free(&isfhandle0);

    _isam_exithook();
    return (ISERROR);
}

