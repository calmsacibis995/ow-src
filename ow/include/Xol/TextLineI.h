#ifndef _XOL_TEXTLINEI_H
#define _XOL_TEXTLINEI_H
#pragma ident	"@(#)TextLineI.h	1.3	92/10/22 lib/libXol SMI"	/* OLIT	*/

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
#include <Xol/OlStrMthdsI.h>
#include <Xol/TextLBuffI.h>

#ifdef  __cplusplus
extern "C" {
#endif

	/*** Function Type Identifiers - to be moved to OpenLookP.h ***/

#define ClassMethod	static
#define Private		static
#define WidgetInternal
#define PublicInterface

	/*** Convenient interface to str_methods members ***/

#define _OLStrEmptyString(format)       str_methods[format].StrEmptyString()
#define _OLStrNumBytes(format, str)     str_methods[format].StrNumBytes(str)
#define _OLStrNumUnits(format, str)     str_methods[format].StrNumUnits(str)
#define _OLStrNumChars(format, str)     str_methods[format].StrNumChars(str)
#define _OLStrCpy(format, s1, s2)       str_methods[format].StrCpy(s1, s2)
#define _OLStrNCpy(format, s1, s2, len) str_methods[format].StrNCpy(s1, s2, len)
#define _OLStrCat(format, s1, s2)       str_methods[format].StrCat(s1, s2)
#define _OLStrNCat(format, s1, s2, len) str_methods[format].StrNCat(s1, s2, len)
#define _OLStrCmp(format, s1, s2)       str_methods[format].StrCmp(s1, s2)
#define _OLStrNCmp(format, s1, s2, len) str_methods[format].StrNCmp(s1, s2, len)
#define _OLStrToCT(format, dpy, s, len) str_methods[format].StrToCT(dpy, s, len)
#define _OLStrFromCT(format, dpy, s)	str_methods[format].StrFromCT(dpy, s)
#define _OLStrFreeList(format, s)	str_methods[format].StrFreeList(s)

#define _OLStrExtents(format, font, s, len, ink, log) \
			str_methods[format].StrExtents(font, s, len, ink, log)
#define _OLStrDrawImage(format, dpy, win, font, gc, x, y, s, len) \
			str_methods[format].StrDrawImage(dpy, win, font, gc, x, y, s, len)
#define _OLStrDraw(format, dpy, win, font, gc, x, y, s, len) \
			str_methods[format].StrDraw(dpy, win, font, gc, x, y, s, len)

	/*** Opcodes for Widget-Drawing routine ***/

#define	TLDrawLeftArrow			(1L)
#define	TLDrawInvokedLeftArrow		(1L << 1)
#define	TLDrawRightArrow		(1L << 2)
#define	TLDrawInvokedRightArrow		(1L << 3)
#define	TLDrawText			(1L << 4)
#define	TLDrawCaret			(1L << 5)
#define	TLDrawUnderline			(1L << 6)
#define	TLDrawLabel			(1L << 7)
#define	TLMoveCaret			(1L << 8)
#define	TLClearLeftArrow		(1L << 9)
#define	TLClearRightArrow		(1L << 10)
#define	TLClearUnderline		(1L << 11)

#define TLCreateLeftArrow	(TLClearLeftArrow | TLDrawLeftArrow | TLDrawUnderline)
#define TLDestroyLeftArrow	(TLClearLeftArrow | TLDrawUnderline)
#define TLCreateRightArrow	(TLClearRightArrow | TLDrawRightArrow | TLDrawUnderline)
#define	TLDestroyRightArrow	(TLClearRightArrow | TLDrawUnderline)

	/*** Feedback indicators for Text-Drawing routine ***/
#define	TLNormalFeedback		(0L)
#define	TLInvFeedback			(1L)	

	/* 
	  Flag to Widget-Drawing routine to indicate that it needs to:
		o Clear the widget from "start_pos" till end_of_display
		o Print text from "start_pos" till end_of_text, bounded by 
			the widget size
	*/
#define TLEndOfDisplay			(-1)


	/*** WidgetInternal Macros ... ***/

#define _OLTLNumArrowChars(tlp) 	\
     (tlp->leftarrow_present ? \
       (TextScrollButton_Width(tlp->pAttrs->ginfo)/ tlp->max_char_width + \
         TextScrollButton_Width(tlp->pAttrs->ginfo)% tlp->max_char_width ? 1:0) : \
     0)

#define _OLTLStartOfDisplay(tlp) (_OLTLNumArrowChars(tlp) + tlp->char_offset)
#define _OLTLWarn(mssg)    	 OlWarning(dgettext(OlMsgsDomain, mssg))


	/*** WidgetInternal functions ***/

extern 	OlStr 	_OlTLGetSubString(TLBuffer 	*b, 
				  int 		start, 
				  int 		end, 
				  OlStrRep 	format, 
				  PositionTable *posTable
				 );

extern 	void 	_OlTLDrawWidget(TextLineWidget 	w, 
				int 		start_char_pos, 
				int		end_char_pos,
				unsigned long 	what
			       );

extern unsigned long _OlTLSetupArrowsAfterCursorChange(TextLineWidget w);
extern unsigned long _OlTLSetupArrowsAfterTextChange(TextLineWidget w);

#ifdef  __cplusplus
}
#endif

#endif	/* _XOL_TEXTLINEI_H */
