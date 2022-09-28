#ifndef lint
        static char sccsid[] = "@(#)iskeycmp.c	1.3\t06/11/93 Copyr 1988 Sun Micro";
#endif

/*
 * Copyright (c) 1988 by Sun Microsystems, Inc.
 */

/*
 * iskeycmp.c
 *
 * Description:
 *	ISAM index comparison functions
 */

#include "isam_impl.h"

static struct keypart2 *_curtab;		     /* Current comparison */
					     /* descriptor table */
static int _ncurtab;			     /* Number of entries */

/*
 * _iskeycmp_set()
 *
 * Set key decriptor and number of parts for subsequent key comparison.s
 */

void
_iskeycmp_set (pkeydesc2, nparts)
    Keydesc2		*pkeydesc2;	
    int			nparts;		     /* Use only so many parts */
{
    _ncurtab = nparts;
    _curtab = pkeydesc2->k2_part;
    assert(_ncurtab <= pkeydesc2->k2_nparts + 1); /* + 1 for recno */
}

/*
 * Return number that is > 0 if l > r,
 *			 = 0 if l = r,
 *			 < 0 if l < r.
 */

int
_iskeycmp(lkey, rkey)
    char    *lkey, *rkey;
{
    int		   	     i, ret;
    register struct keypart2 *p;
    register   char   	     *l, *r;
    register long	     llong, rlong;
    double		     ldouble, rdouble;

    ret = 0;
    for (i = 0, p = _curtab; ret == 0 && i < _ncurtab;i++, p++) {
	
	l = lkey + p->kp2_offset;
	r = rkey + p->kp2_offset;

	switch (p->kp2_type) {
	case CHARTYPE:
	case BINTYPE:
	    ret = memcmp(l, r, p->kp2_leng);
	    break;

	case LONGTYPE:
	    llong = ldlong(l);
	    rlong = ldlong(r);

	    if (llong > rlong)
		ret = 1;
	    else if (llong < rlong)
		ret = -1;
	    break;

	case SHORTTYPE:
	    llong = (long)ldshort(l);
	    rlong = (long)ldshort(r);

	    if (llong > rlong)
		ret = 1;
	    else if (llong < rlong)
		ret = -1;
	    break;

	case DOUBLETYPE:
	    ldouble = lddbl(l);
	    rdouble = lddbl(r);

	    if (ldouble > rdouble)
		ret = 1;
	    else if (ldouble < rdouble)
		ret = -1;
	    break;

	case FLOATTYPE:
	    ldouble = (double)ldfloat(l);
	    rdouble = (double)ldfloat(r);

	    if (ldouble > rdouble)
		ret = 1;
	    else if (ldouble < rdouble)
		ret = -1;
	    break;

	case CHARTYPE + ISDESC:
	case BINTYPE + ISDESC:
	    ret = memcmp(r, l, p->kp2_leng);
	    break;

	case LONGTYPE + ISDESC:
	    llong = ldlong(l);
	    rlong = ldlong(r);

	    if (llong > rlong)
		ret = -1;
	    else if (llong < rlong)
		ret = 1;
	    break;

	case SHORTTYPE + ISDESC:
	    llong = (long)ldshort(l);
	    rlong = (long)ldshort(r);

	    if (llong > rlong)
		ret = -1;
	    else if (llong < rlong)
		ret = 1;
	    break;

	case DOUBLETYPE + ISDESC:
	    ldouble = lddbl(l);
	    rdouble = lddbl(r);

	    if (ldouble > rdouble)
		ret = -1;
	    else if (ldouble < rdouble)
		ret = 1;
	    break;

	case FLOATTYPE + ISDESC:
	    ldouble = (double)ldfloat(l);
	    rdouble = (double)ldfloat(r);

	    if (ldouble > rdouble)
		ret = -1;
	    else if (ldouble < rdouble)
		ret = 1;
	    break;

	default:
	    _isfatal_error("Bad data conversion descriptor");
	    break;
	}
    }
    return (ret);
}
