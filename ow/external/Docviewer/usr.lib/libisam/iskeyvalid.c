#ifndef lint
        static char sccsid[] = "@(#)iskeyvalid.c	1.3\t06/11/93 Copyr 1988 Sun Micro";
#endif
/*
 * Copyright (c) 1988 by Sun Microsystems, Inc.
 */

/*
 * iskeydesc.c
 *
 * Description:
 *	Functions dealing with the keydesc structure.
 *	
 *
 */

#include "isam_impl.h"

/*
 * _validate_keydesc(keydesc, minreclen)
 *
 * Validate key descriptor. minreclen is needed to make suree that all
 * the key parts have a meaningful offset.
 *
 */

int
_validate_keydesc(keydesc, minreclen)
    register struct keydesc	*keydesc;
    int				minreclen;
{
    int 		nparts;
    register int 	i;
    int			type, start, length;
    int			keylen = 0;

    nparts =  keydesc->k_nparts;

    if (nparts <= 0 || nparts > NPARTS)
	return (ISERROR);

    for (i = 0; i < nparts;i++) {

	type = keydesc->k_part[i].kp_type & ~ISDESC;
	start = keydesc->k_part[i].kp_start;
	length = keydesc->k_part[i].kp_leng;

	if(_check_typelen(type, length) == ISERROR)
	    return (ISERROR);

	if (type < MINTYPE || type >= MAXTYPE)
	    return (ISERROR);

	if (start < 0 || start + length > minreclen)
	    return (ISERROR);

	keylen += length;
    }

    if(keylen > MAXKEYSIZE)
	return (ISERROR);

    return (ISOK);
}

/*
 * _check_typelen()
 *
 * Check length against the length of the corresponding type.
 */

static int
_check_typelen(type, length)
    int		type;
    int		length;
{
    switch (type) {
    case INTTYPE:
	return ((length == INTSIZE) ? ISOK : ISERROR);
    case LONGTYPE:
	return ((length == LONGSIZE) ? ISOK : ISERROR);
    case FLOATTYPE:
	return ((length == FLOATSIZE) ? ISOK : ISERROR);
    case DOUBLETYPE:
	return ((length == DOUBLESIZE) ? ISOK : ISERROR);
    case CHARTYPE:
    case BINTYPE:
	return ((length > 0) ? ISOK : ISERROR);
    default:
	return (ISERROR);
    }
}
    

