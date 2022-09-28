#ifndef	_XOL_STRMTHDSI_H
#define	_XOL_STRMTHDSI_H

#pragma	ident	"@(#)OlStrMthdsI.h	302.2	92/10/08 include/Xol SMI"	/* OLIT	*/

/*
 *	Copyright (C) 1986,1991  Sun Microsystems, Inc
 *			All rights reserved.
 *		Notice of copyright on this source code
 *		product does not indicate publication.
 *
 * RESTRICTED RIGHTS LEGEND: Use, duplication, or disclosure by
 * the U.S. Government is subject to restrictions as set forth
 * in subparagraph (c)(1)(ii) of the Rights in Technical Data
 * and Computer Software Clause at DFARS 252.227-7013 (Oct. 1988)
 * and FAR 52.227-19 (c) (June 1987).
 *
 *	Sun Microsystems, Inc., 2550 Garcia Avenue,
 *	Mountain View, California 94043.
 *
 */


#include <X11/Intrinsic.h>

#include <Xol/OpenLook.h>


#ifdef	__cplusplus
extern "C" {
#endif


#define NUM_SUPPORTED_REPS 3


typedef struct _OlStrMethods {
	/* string */
	OlStr			(*StrEmptyString)(void);
	int			(*StrNumBytes)(OlStr);
	int			(*StrNumUnits)(OlStr);

	/*
	 * number of storage units: e.g. how many bytes in SB and MB how many
	 * wide-chars in WC
	 */
	int			(*StrNumChars)(OlStr);
	OlStr			(*StrSubString)(OlStr, int);

	/* inter string operations */
	OlStr			(*StrCpy)(OlStr, OlStr);
	OlStr			(*StrNCpy)(OlStr, OlStr, int);
	OlStr			(*StrCat)(OlStr, OlStr);
	OlStr			(*StrNCat)(OlStr, OlStr, int);
	int			(*StrCmp)(OlStr, OlStr);
	int			(*StrNCmp)(OlStr, OlStr, int);

	/* text operations */
	int			(*StrLookup)(XIC ic, XKeyEvent* event,
		OlStr s, int nunits, KeySym*  keysym, Status*  status_ret);

	void			(*StrDraw)(Display* dpy, Drawable d, OlFont fs,
		GC gc, int x, int y, OlStr s, int nunits);

	void			(*StrDrawImage)(Display*  dpy, Drawable d,
		OlFont fs, GC gc, int x, int y, OlStr s, int nunits);

	int			(*StrWidth)(OlFont fs, OlStr s, int nunits);

	int			(*StrExtents)(OlFont fs, OlStr s, int nunits,
		XRectangle* overall_ink, XRectangle* overall_logical);

	/* conversion between rep type and compound text */
	char*			(*StrToCT)(Display* display, OlStr s,
		int* ct_len);

	OlStr*			(*StrFromCT)(Display*  display, char* ct_string);
	void			(*StrFreeList)(OlStr*  s);

	/* Printf operations */
	int			(*StrPrintf)(OlStr s, const char *, ...);

	/* String-to-integer conversions */
	int			(*StrAtoi)(OlStr s);
	double			(*StrAtof)(OlStr s);

}			OlStrMethods;

extern OlStrMethods	str_methods[];


#ifdef	__cplusplus
}
#endif


#endif	/* _XOL_STRMTHDSI_H */
