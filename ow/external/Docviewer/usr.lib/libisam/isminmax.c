#ifndef lint
        static char sccsid[] = "@(#)isminmax.c	1.3\t06/11/93 Copyr 1988 Sun Micro";
#endif

/*
 * Copyright (c) 1988 by Sun Microsystems, Inc.
 */

/*
 * isminmax.c 
 *
 * Description:
 *	NetISAM minimum and maximum values functions
 */

#include "isam_impl.h"

static unsigned char ismaxlongarr[LONGSIZE] = ISMAXLONG;
static unsigned char isminlongarr[LONGSIZE] = ISMINLONG;

static unsigned char ismaxshortarr[SHORTSIZE] = ISMAXSHORT;
static unsigned char isminshortarr[SHORTSIZE] = ISMINSHORT;

static unsigned char ismaxdoublearr[DOUBLESIZE] = ISMAXDOUBLE;
static unsigned char ismindoublearr[DOUBLESIZE] = ISMINDOUBLE;

static unsigned char ismaxfloatarr[FLOATSIZE] = ISMAXFLOAT;
static unsigned char isminfloatarr[FLOATSIZE] = ISMINFLOAT;

/* These two are used globally. */
long *ismaxlong = (long *)ismaxlongarr;
long *isminlong = (long *)isminlongarr;

static short *ismaxshort = (short *)ismaxshortarr;
static short *isminshort = (short *)isminshortarr;

static double *ismaxdouble = (double *)ismaxdoublearr;
static double *ismindouble = (double *)ismindoublearr;

static float *ismaxfloat = (float *)ismaxfloatarr;
static float *isminfloat = (float *)isminfloatarr;


/* 
 * _iskey_fillmax() 
 *
 * Fill key buffer with maximum values 
 */

void
_iskey_fillmax(pkeydesc2, keybuf)
    struct keydesc2	*pkeydesc2;
    register char	*keybuf;
{
    register int 	i;
    register struct keypart2 *ppart;
    int 		nparts;

    nparts = pkeydesc2->k2_nparts;
    ppart = pkeydesc2->k2_part;

    for (i = 0; i < nparts + 1;i++) {	     /* +1 is for recnum part */
	switch (ppart->kp2_type) {
	case CHARTYPE:
	    (void) memset (keybuf + ppart->kp2_offset, ISMAXCHAR, 
			   ppart->kp2_leng);
	    break;
	case BINTYPE:
	    (void) memset (keybuf + ppart->kp2_offset, ISMAXBIN, 
			   ppart->kp2_leng);
	    break;
	case LONGTYPE:
	    memcpy ( keybuf + ppart->kp2_offset,(char *)ismaxlong, LONGSIZE);
	    break;
	case SHORTTYPE:
	    memcpy ( keybuf + ppart->kp2_offset,(char *)ismaxshort, SHORTSIZE);
	    break;
	case FLOATTYPE:
	    memcpy ( keybuf + ppart->kp2_offset,(char *)ismaxfloat, FLOATSIZE);
	    break;
	case DOUBLETYPE:
	    memcpy ( keybuf + ppart->kp2_offset,(char *)ismaxdouble, DOUBLESIZE);
	    break;

	case CHARTYPE + ISDESC:
	    (void) memset (keybuf + ppart->kp2_offset, ISMINCHAR, 
			   ppart->kp2_leng);
	    break;
	case BINTYPE + ISDESC:
	    (void) memset (keybuf + ppart->kp2_offset, ISMINBIN, 
			   ppart->kp2_leng);
	    break;
	case LONGTYPE + ISDESC:
	    memcpy ( keybuf + ppart->kp2_offset,(char *)isminlong, LONGSIZE);
	    break;
	case SHORTTYPE + ISDESC:
	    memcpy ( keybuf + ppart->kp2_offset,(char *)isminshort, SHORTSIZE);
	    break;
	case FLOATTYPE + ISDESC:
	    memcpy ( keybuf + ppart->kp2_offset,(char *)isminfloat, FLOATSIZE);
	    break;
	case DOUBLETYPE + ISDESC:
	    memcpy ( keybuf + ppart->kp2_offset,(char *)ismindouble, DOUBLESIZE);
	    break;
	default:
	    _isfatal_error("_iskey_fillmax");
	}
	ppart++;
    }
}

/* 
 * _iskey_fillmin() 
 *
 * Fill key buffer with minimum values 
 */

void
_iskey_fillmin(pkeydesc2, keybuf)
    struct keydesc2	*pkeydesc2;
    register char	*keybuf;
{
    register int 	i;
    register struct keypart2 *ppart;
    int 		nparts;

    nparts = pkeydesc2->k2_nparts;
    ppart = pkeydesc2->k2_part;

    for (i = 0; i < nparts + 1;i++) {	     /* +1 is for recnum part */
	switch (ppart->kp2_type) {
	case CHARTYPE:
	    (void) memset (keybuf + ppart->kp2_offset, ISMINCHAR, 
			   ppart->kp2_leng);
	    break;
	case BINTYPE:
	    (void) memset (keybuf + ppart->kp2_offset, ISMINBIN, 
			   ppart->kp2_leng);
	    break;
	case LONGTYPE:
	    memcpy ( keybuf + ppart->kp2_offset,(char *)isminlong, LONGSIZE);
	    break;
	case SHORTTYPE:
	    memcpy ( keybuf + ppart->kp2_offset,(char *)isminshort, SHORTSIZE);
	    break;
	case FLOATTYPE:
	    memcpy ( keybuf + ppart->kp2_offset,(char *)isminfloat, FLOATSIZE);
	    break;
	case DOUBLETYPE:
	    memcpy ( keybuf + ppart->kp2_offset,(char *)ismindouble, DOUBLESIZE);
	    break;

	case CHARTYPE + ISDESC:
	    (void) memset (keybuf + ppart->kp2_offset, ISMAXCHAR, 
			   ppart->kp2_leng);
	    break;
	case BINTYPE + ISDESC:
	    (void) memset (keybuf + ppart->kp2_offset, ISMAXBIN, 
			   ppart->kp2_leng);
	    break;
	case LONGTYPE + ISDESC:
	    memcpy ( keybuf + ppart->kp2_offset,(char *)ismaxlong, LONGSIZE);
	    break;
	case SHORTTYPE + ISDESC:
	    memcpy ( keybuf + ppart->kp2_offset,(char *)ismaxshort, SHORTSIZE);
	    break;
	case FLOATTYPE + ISDESC:
	    memcpy ( keybuf + ppart->kp2_offset,(char *)ismaxfloat, FLOATSIZE);
	    break;
	case DOUBLETYPE + ISDESC:
	    memcpy ( keybuf + ppart->kp2_offset,(char *)ismaxdouble, DOUBLESIZE);
	    break;
	default:
	    _isfatal_error("_iskey_fillmin");
	}
	ppart++;
    }
}
