#ifndef lint
        static char sccsid[] = "@(#)isstrec.c	1.3\t06/11/93 Copyr 1988 Sun Micro";
#endif

/*
 * Copyright (c) 1988 by Sun Microsystems, Inc.
 */

/*
 * isstrec.c
 *
 * Description:
 *	Store record from user structure into NetISAM data record
 *	A part of NetISAM Programmer tool kit.
 *	
 */

#include "isam_impl.h"

isstrec (p, s, r)
    struct istableinfo	*p;
    char		*s;		     /* Pointer to user structure */
    char		*r;		     /* NetISAM record */
{
    int			nfields = p->tbi_nfields;
    register struct isfldmap *pfldmap = p->tbi_fields;
    register int	i;
    register char	*src, *dst;

    for (i = 0; i < nfields; i++) {
	
	dst = pfldmap->flm_recoff + r;
	src = pfldmap->flm_bufoff + s;

	switch (pfldmap->flm_type) {
	case CHARTYPE:
	    stchar (src,dst,pfldmap->flm_length);
	    break;
	case BINTYPE:
	    memcpy ((void *)dst,(const void *)src,pfldmap->flm_length);
	    break; 
	case LONGTYPE:
	    stlong (*(long *)src,dst);
	    break;
	case SHORTTYPE:
	    stshort (*(short *)src,dst);
	    break;
	case DOUBLETYPE:
	    stdbl (*(double *)src,dst);
	    break;
	case FLOATTYPE:
	    stfloat (*(float *)src,dst);
	    break;
	default:
	    _isfatal_error("Bad conversion descriptor\n");
	}
	pfldmap++;
    }
}
