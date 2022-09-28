#ifndef lint
        static char sccsid[] = "@(#)ismngfcb.c	1.3\t06/11/93 Copyr 1988 Sun Micro";
#endif
/*
 * Copyright (c) 1988 by Sun Microsystems, Inc.
 */

/*
 * ismngfcb.c
 *
 * Description: 
 *	Manager of open FCB blocks.
 *
 * This module keeps track of usage of the FCB blocks and finds a victim
 * on LRU basis.
 * It also provides associative access to the FCB by their isfhandles.
 *
 */
#include "isam_impl.h"


#define FCBHASHSIZE	101		     /* Should be a prime for best hash */

#if  (MAXFCB_UNIXFD > FCBHASHSIZE)
/* 
 * Cause a syntax error. FCBHASHSIZE must be increased to be > MAXFCB_UNIXFD.
 * A good estimate is a prime approximately equal (2 * MAXFCB_UNIXFD).
 */
MUST INCREASE FCBHASHSIZE
#endif

struct hashtable {
    Bytearray	isfhandle;
    Fcb		*fcb;
    long	mrused;			     
} hashtable [FCBHASHSIZE];
#define unused(entry) ((entry).fcb == NULL)

int _hashisfhandle();

static mrused_last = 0;			     /* stamp generator */


/*
 * _mngfcb_insert(fcb, isfhandle)
 *
 * Insert new FCB entry.
 */

void
_mngfcb_insert(fcb, isfhandle)
    Fcb		*fcb;
    Bytearray	*isfhandle;
{
    int			hashval = _hashisfhandle(isfhandle);
    register int  	ind;
    int			ntries;

    /* Try to find an unused entry in the hash table. */
    ind = hashval;
    for (ntries = 0; ntries < FCBHASHSIZE; ntries++) {
	if (unused(hashtable[ind]))
	    break;
	if (++ind == FCBHASHSIZE)
	    ind = 0;			     /* Wrap the table */
    }

    if (ntries == FCBHASHSIZE) {
	_isfatal_error("FCB hash table overflow");
    }
	
    /*
     * Create an entry at the index ind.
     * Duplicate the file handle and mark the entry with the current stamp.
     */
    hashtable[ind].isfhandle = _bytearr_dup(isfhandle);
    hashtable[ind].fcb = fcb;
    hashtable[ind].mrused = mrused_last++;
}


/*
 * fcb = _mngfcb_find(isfhandle)
 *	
 * Return a pointer to the FCB, or NULL if the FCB is not found.
 * If the FCB is found, it is "touched" for the LRU algorithm purpose.
 */

Fcb *
_mngfcb_find(isfhandle)
    Bytearray	*isfhandle;
{
    int			hashval = _hashisfhandle(isfhandle);
    register int  	ind;
    int			ntries;

    /* Find the entry. */
    ind = hashval;
    for (ntries = 0; ntries < FCBHASHSIZE; ntries++) {
	if (_bytearr_cmp(&hashtable[ind].isfhandle, isfhandle) == 0)
	    break;
	if (++ind == FCBHASHSIZE)
	    ind = 0;			     /* Wrap the table */
    }

    if (ntries == FCBHASHSIZE) {
	return (NULL);			     /* Not found */
    } 
    else {

	/*
	 * Mark the entry with the current stamp.
	 */
	hashtable[ind].mrused = mrused_last++;
	return hashtable[ind].fcb;
    }
}

/*
 * _mngfcb_delete(isfname)
 *
 * Delete an entry.
 */

void
_mngfcb_delete(isfhandle)
    Bytearray	*isfhandle;
{
    int			hashval = _hashisfhandle(isfhandle);
    register int  	ind;
    int			ntries;

    /* Find the entry */
    ind = hashval;
    for (ntries = 0; ntries < FCBHASHSIZE; ntries++) {
	if (_bytearr_cmp(&hashtable[ind].isfhandle, isfhandle) == 0)
	    break;
	if (++ind == FCBHASHSIZE)
	    ind = 0;			     /* Wrap the table */
    }

    if (ntries == FCBHASHSIZE) {
	_isfatal_error("_mngfcb_delete cannot find entry");
    } 
    else {

	/*
	 * Clear the entry.
	 */
	_bytearr_free(&hashtable[ind].isfhandle);
	memset ((char *) &hashtable[ind], 0, sizeof(hashtable[ind]));
    }
}


/*
 * isfhandle = _mngfcb_victim()
 *
 * Find LRU used FCB.
 */

Bytearray *
_mngfcb_victim()
{
    int			victim_ind = -1;
    long		victim_time = 0;     /* Assign to shut up lint */
    register int	i;

    for (i = 0; i < FCBHASHSIZE; i++) {

	if (unused(hashtable[i]))	     /* Skip empty slots in table */
	    continue;
	
	if (victim_ind == -1 || victim_time > hashtable[i].mrused) {	
	    victim_ind = i;
	    victim_time = hashtable[i].mrused;
	}
    }
    return ((victim_ind == -1) ? NULL : &hashtable[victim_ind].isfhandle);
}


/*
 * _hashisfhandle(isfhandle)
 *
 * Hash isfhandle into an integer.
 */

Static int
_hashisfhandle(isfhandle)
    Bytearray		*isfhandle;
{
    register char	*p;
    register unsigned	h, g;
    register int	len;

    len = isfhandle->length;
    p = isfhandle->data;
    h = 0;

    while (len-- > 0) {
	h = (h << 4) + (*p++);
	if (g = h&0xf0000000) {
	    h = h ^ (g >> 24);
	    h = h ^ g;
	}
    }
    return (h % FCBHASHSIZE);
}
