#ifndef lint
        static char sccsid[] = "@(#)isldrec.c	1.3\t06/11/93 Copyr 1988 Sun Micro";
#endif

/*
 * Copyright (c) 1988 by Sun Microsystems, Inc.
 */
 
/*
 * isldrec.c
 *
 * Description:
 *	Load record from NetISAM data record into user structure
 *	A part of NetISAM Programmer tool kit.
 *	
 */

#include "isam_impl.h"

isldrec (p, s, r)
    struct istableinfo	*p;
    char		*s;		     /* Pointer to user structure */
    char		*r;		     /* NetISAM record */
{
    int			nfields = p->tbi_nfields;
    register struct isfldmap *pfldmap = p->tbi_fields;
    register int	i;
    register char	*src, *dst;

    for (i = 0; i < nfields; i++) {
	
	src = pfldmap->flm_recoff + r;
	dst = pfldmap->flm_bufoff + s;

	switch (pfldmap->flm_type) {
	case CHARTYPE:
	    ldchar (src,pfldmap->flm_length,dst);
	    dst [pfldmap->flm_length] = '\0';
	    break;
	case BINTYPE:
	    memcpy ((void *)dst,(const void *)src,pfldmap->flm_length);
	    break; 
	case LONGTYPE:
	    *(long *)dst = ldlong(src);
	    break;
	case SHORTTYPE:
	    *(short *)dst = ldshort(src);
	    break;
	case DOUBLETYPE:
	    *(double *)dst = lddbl(src);
	    break;
	case FLOATTYPE:
	    *(float *)dst = ldfloat(src);
	    break;
	default:
	    _isfatal_error("Bad conversion descriptor\n");
	}
	pfldmap++;
    }
}
