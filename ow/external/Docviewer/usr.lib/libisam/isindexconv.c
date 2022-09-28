#ifndef lint
        static char sccsid[] = "@(#)isindexconv.c	1.3\t06/11/93 Copyr 1988 Sun Micro";
#endif
/*
 * Copyright (c) 1988 by Sun Microsystems, Inc.
 */

/*
 * isindexconv.c
 *
 * Description:
 *	Encode/decode key descriptor
 */

#include "isam_impl.h"

void
stkey(pkeydesc2, p)
    Keydesc2		*pkeydesc2;
    char		*p;
{
    int                         i;
    register struct keypart2   	*ppart;
    register char               *pp;
    int                         nparts;

    stshort(pkeydesc2->k2_flags, p + K2_FLAGS_OFF);
    stshort(pkeydesc2->k2_nparts, p + K2_NPARTS_OFF);
    stshort(pkeydesc2->k2_len, p + K2_LEN_OFF);
    stblkno(pkeydesc2->k2_rootnode, p + K2_ROOT_OFF);
    stlong((long)pkeydesc2->k2_keyid, p + K2_KEYID_OFF);

    ppart = pkeydesc2->k2_part;
    pp    = p + K2_KEYPART_OFF;
    nparts = pkeydesc2->k2_nparts + 1;	     /* +1 for recno part */

    if (ALLOWS_DUPS2(pkeydesc2))	     /* +1 for dups serial number*/
	nparts++;
 
    for (i = 0; i < nparts;i++) {
        stshort(ppart->kp2_start, pp);
        pp += SHORTSIZE;
 
        stshort(ppart->kp2_leng, pp);
        pp += SHORTSIZE;
 
        stshort(ppart->kp2_type, pp);
        pp += SHORTSIZE;
 
        stshort(ppart->kp2_offset, pp);
        pp += SHORTSIZE;
        
        ppart++;
    }
}      

void
ldkey(pkeydesc2,p)
    register struct keydesc2    *pkeydesc2;
    register char               *p;
{
    int                         i;
    register struct keypart2    *ppart;
    register char               *pp;
    int                         nparts;

    memset ((char *) pkeydesc2, 0, sizeof (*pkeydesc2));
    pkeydesc2->k2_flags = ldshort(p + K2_FLAGS_OFF);
    pkeydesc2->k2_nparts = ldshort(p + K2_NPARTS_OFF);
    pkeydesc2->k2_len = ldshort(p + K2_LEN_OFF);
    pkeydesc2->k2_rootnode = ldblkno(p + K2_ROOT_OFF);
    pkeydesc2->k2_keyid = ldlong(p + K2_KEYID_OFF);

    ppart = pkeydesc2->k2_part;
    pp    = p + K2_KEYPART_OFF;
    nparts = pkeydesc2->k2_nparts + 1;	     /* +1 for recno part */

    if (ALLOWS_DUPS2(pkeydesc2))	     /* +1 for dups serial number*/
	nparts++;

    for (i = 0; i < nparts;i++) {
        ppart->kp2_start = ldunshort(pp);
        pp += SHORTSIZE;

        ppart->kp2_leng = ldshort(pp);
        pp += SHORTSIZE;

        ppart->kp2_type = ldshort(pp);
        pp += SHORTSIZE;

        ppart->kp2_offset = ldunshort(pp);
        pp += SHORTSIZE;
        
        ppart++;
    }
}
