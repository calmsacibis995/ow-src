#ifndef lint
        static char sccsid[] = "@(#)isbtree.c	1.3\t06/11/93 Copyr 1988 Sun Micro";
#endif

/*
 * Copyright (c) 1988 by Sun Microsystems, Inc.
 */

/*
 * isbtree.c
 *
 * Description:
 *	B-tree operations: SEARCH
 *	
 */

#include "isam_impl.h"

extern int _iskeycmp();

/* 
 * _isbtree_create() 
 *
 * Create a B-tree path object that will used in subsequent operations.
 */

Btree *
    _isbtree_create(fcb, pkeydesc2)
Fcb			*fcb;
Keydesc2		*pkeydesc2;
{
    register Btree	*p;
    
    p = (Btree *) _ismalloc(sizeof(*p));
    memset((char *)p, 0, sizeof(*p));
    
    p->fcb = fcb;
    p->keydesc2 = pkeydesc2;	
    
    return (p);
}


/* 
 * _isbtr_destroy() 
 *
 * Destroy B-tree path object 
 */

void
_isbtree_destroy(btree)
    register Btree *btree;
{
    register int	i;
    
    for (i = 0; i < btree->depth;i++) {
	_isdisk_unfix(btree->bufhdr[i]);
    }
    free((char *)btree);
}


/* 
 * _isbtree_search() 
 *
 * Descend the B-tree, position pointer on or before the matched key.
 */

void
_isbtree_search(btree, key)
    register Btree  	*btree;
char		*key;		     /* Search for this key */
{
    Keydesc2		*pkeydesc2 = btree->keydesc2;
    Blkno		rootblkno = pkeydesc2->k2_rootnode;
    int			keylength = pkeydesc2->k2_len;
    int			index;		     /* Index to tables in btree */
    /* Has value of 1, next 2, etc. */
    int			elevation;	     /* Level - leaves have value 0 */
    register char	*p;		     /* Pointer to key page */
    int			nkeys;		     /* Number of keys in the page */
    char		*key2;		     /* Equal or next lower key */
    int			curpos;		     /* index of key2 in key page */
    Blkno		blkno;
    
    /* Set comparison function. */
    _iskeycmp_set(pkeydesc2, pkeydesc2->k2_nparts + 1);	/* +1 for recno field */
    
    index = 0;
    blkno = rootblkno;
    do {
	btree->bufhdr[index] = 
	    _isdisk_fix(btree->fcb,  btree->fcb->indfd, blkno, ISFIXREAD);
	p = btree->bufhdr[index]->isb_buffer; /* pointer to buffer */
	
	/* Load some fields from the key page. */
	nkeys = ldshort(p+BT_NKEYS_OFF);     /* Number of keys in the page */
	elevation = ldshort(p+BT_LEVEL_OFF); /* Level of the page */
	
	/* Binary search in the key page to find equal or next lowere key. */
	key2 = _isbsearch(key, p+BT_KEYS_OFF, nkeys, keylength, _iskeycmp);
	
	curpos = (key2) ? ((key2 - p - BT_NKEYS_OFF) / keylength) : 0;
	
	btree->curpos[index] = 
	    (key2 == (char *)0 && elevation==0)? -1 : curpos;
	
	if (elevation > 0) 
	    blkno = ldblkno(p + ISPAGESIZE - (curpos + 1) * BLKNOSIZE);
	
	index++;
    } while (elevation > 0);
    
    btree->depth = index;
}

/* 
 * _isbtree_current() 
 *
 * Get pointer to the current key 
 */

char *
_isbtree_current(btree)
    register Btree	*btree;
{
    int			curpos;
    
    assert(btree->depth > 0);
    if ((curpos = btree->curpos[btree->depth - 1]) == -1)
	return (NULL);
    else
	return (btree->bufhdr[btree->depth - 1]->isb_buffer 
		+ BT_KEYS_OFF + curpos * btree->keydesc2->k2_len);
}

/* 
 * _isbtree_next()
 *
 * Get pointer to the next key 
 */

char *
_isbtree_next(btree)
    register Btree	*btree;
{
    int			curpos;
    int			depth = btree->depth;
    register char	*p;
    register int	level;
    Blkno		blkno;
    
    assert(depth > 0);
    
    /* 
     * Move up along the path, find first block where we can move to the right.
     */
    for (level = depth - 1; level >= 0; level--) {
	p = btree->bufhdr[level]->isb_buffer;
	
	if (btree->curpos[level] < ldshort(p + BT_NKEYS_OFF) - 1)
	    break;
    }
    
    if (level < 0) {
	/* Logical end of the index file. No next record. */
	return (NULL);
    }
    
    curpos = ++(btree->curpos[level]);
    
    while (++level < depth) {
	
	/* Get block number to block in next lower level. */
	if (level > 0)
	    blkno = ldblkno(p + ISPAGESIZE - (curpos + 1) * BLKNOSIZE);
	
	/* Unfix page in this level, fetch its right brother. */
	_isdisk_unfix(btree->bufhdr[level]);
	btree->bufhdr[level] = 
	    _isdisk_fix(btree->fcb, btree->fcb->indfd, blkno, ISFIXREAD);
	p = btree->bufhdr[level]->isb_buffer;
	
	curpos = btree->curpos[level] = 0;
    }
    
    return (p + BT_KEYS_OFF + curpos * btree->keydesc2->k2_len);
}

