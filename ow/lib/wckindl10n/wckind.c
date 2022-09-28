#ifndef lint
#ifdef sccs
static char     sccsid[] = "@(#)wckind.c 1.1 93/05/14; SMI";
#endif
#endif

/*
 * (c) Copyright 1993 Sun Microsystems, Inc. Sun design patents
 * pending in the U.S. and foreign countries. See LEGAL_NOTICE file
 * for terms of the license.
 */
    
/*
 * wckind.c:  This is the set of functions to helping the word
 * selection support in the OpenWindows X toolkit for "iso_8859_1"
 * family of locales (ie, "fr", "it"...).  However, SunOS 5.x will
 * have a this kind of feature in future release, therefor this
 * function is considered as TEMPORARY SOLUTION.  Should never be
 * publish any of the information about this functions but SMI
 * internal.
 */


#include        <stdlib.h>
#include        <wctype.h>

_l10n_wckind(wchar_t wc)
{
	return (iswalnum(wc) || wc == L'_');
}
