#ifndef _XOL_PARSEACCI_H
#define _XOL_PARSEACCI_H
#pragma ident	"@(#)ParseAccI.h	1.1	92/10/09 lib/libXol SMI"	/* OLIT	*/

/*
 *        Copyright (C) 1986,1991  Sun Microsystems, Inc
 *                    All rights reserved.
 *          Notice of copyright on this source code 
 *          product does not indicate publication. 
 * 
 * RESTRICTED RIGHTS LEGEND: Use, duplication, or disclosure by 
 * the U.S. Government is subject to restrictions as set forth 
 * in subparagraph (c)(1)(ii) of the Rights in Technical Data
 * and Computer Software Clause at DFARS 252.227-7013 (Oct. 1988) 
 * and FAR 52.227-19 (c) (June 1987).
 *
 *    Sun Microsystems, Inc., 2550 Garcia Avenue,
 *    Mountain View, California 94043.
 */

typedef struct acceleratorValue {
    KeySym keysym;
    BtnSym btnsym;
    Boolean error;
    Boolean some;
    Boolean none;
} AcceleratorValue;

extern Boolean _OlParseKeyOrBtnSyntax(Display *dpy, char *resourceString,
				      Boolean is_key, BtnSym *btnsym,
				      KeySym *keysym, Modifiers *modifiers);

#endif /* _XOL_PARSEACCI_H */
