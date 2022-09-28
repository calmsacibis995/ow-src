#ifndef lint
        static char sccsid[] = "@(#)isprimtorec.c	1.3\t06/11/93 Copyr 1988 Sun Micro";
#endif
/*
 * Copyright (c) 1988 by Sun Microsystems, Inc.
 */

/*
 * isprimtorec.c
 *
 * Description: 
 *	Convert primary key value into record number by reading
 *	on B tree.
 *	
 *
 */

#include "isam_impl.h"

#define ZERO 0

int
_isprim_to_recno(fcb, record, recnum)
    Fcb			*fcb;
    char		*record;
    Recno		*recnum;
{
    char		keybuf[MAXKEYSIZE];
    Btree		*btree;
    char		*pkey = NULL;
    Keydesc2		*pkeydesc2 = fcb->keys;	/* Primary key */
    
    /*
     * Return error if there is no primary key.
     */
    if (pkeydesc2->k2_nparts == 0)
	return (ENOREC);
    
    /*
     * Return error if primary key allows duplicates.
     */
    if (ALLOWS_DUPS2(pkeydesc2)) 
	return (EBADKEY);
    
    memset((void *)keybuf, ZERO, pkeydesc2->k2_len);
    _iskey_extract(pkeydesc2, record, keybuf);
    
    btree = _isbtree_create(fcb, pkeydesc2);
 
    /*
     * Position pointer in the B-tree before the searched value. 
     */
    _isbtree_search(btree, keybuf);
    
    if ((pkey = _isbtree_next(btree)) == NULL ||
	memcmp(keybuf + RECNOSIZE, pkey + RECNOSIZE,
	       pkeydesc2->k2_len - RECNOSIZE) != 0) {
	_isbtree_destroy(btree);
	return (ENOREC);
    }
    
    *recnum = ldrecno(pkey + KEY_RECNO_OFF);
    _isbtree_destroy(btree);

    return (ISOK);
}
