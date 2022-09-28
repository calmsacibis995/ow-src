/* Copyright (c) 1993 by Sun Microsystems, Inc. */

#ifndef _AUDIOTOOL_ATOOL_SEL_H
#define	_AUDIOTOOL_ATOOL_SEL_H

#ident	"@(#)atool_sel.h	1.12	93/03/03 SMI"

#ifndef OWV2

#include "atool_types.h"

/* what kind of sel xfer we're doing */
typedef enum {
	S_Paste = 1, S_Drop, S_Insert, S_Load, S_TTLoad, S_TTDisplay
} SelXferType;

extern int Audio_SENDDROP(Xv_opaque window);
extern void Audio_GETDROP(Xv_opaque win, Event *event);
extern void Audio_SAVEBACK(Xv_opaque, Audio_Object, int);
extern int Audio_GET_CLIPBOARD(Xv_opaque win);
extern void Audio_DRAG_PREVIEW(Xv_opaque window);
extern void Audio_OWN_CLIPBOARD(Xv_opaque win);
extern void Audio_OWN_PRIMARY_SELECTION(Xv_opaque win);
extern void Audio_Sel_INIT(ptr_t ap, Xv_opaque frame, Xv_opaque canvas,
			   Xv_opaque dropsite);

extern void discard_load(Xv_opaque win);

#endif /* OWV2 */

#endif /* !_AUDIOTOOL_ATOOL_SEL_H */
