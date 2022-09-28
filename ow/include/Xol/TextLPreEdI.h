#ifndef _XOL_TEXTLPREEDI_H
#define _XOL_TEXTLPREEDI_H
#pragma ident	"@(#)TextLPreEdI.h	1.3	92/10/06 lib/libXol SMI"	/* OLIT	*/

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
 *
 */

#include <Xol/OpenLook.h>

#ifdef  __cplusplus
extern "C" {
#endif

extern int _OlTLPreeditStartCallbackFunc(XIC, XPointer , XPointer);
extern void _OlTLPreeditEndCallbackFunc(XIC, XPointer , XPointer );
extern void _OlTLPreeditDrawCallbackFunc(XIC, XPointer, XIMPreeditDrawCallbackStruct *);
extern void _OlTLPreeditCaretCallbackFunc(XIC, XPointer, XIMPreeditCaretCallbackStruct *);

#ifdef  __cplusplus
}
#endif

#endif	/* _XOL_TEXTLPREEDI_H */
