/*	@(#)graphics.h 3.1 IEI SMI	*/

/*
 * Copyright (c) 1989 by Sun Microsystems, Inc.
 */

/*	Copyright (c) 1989, 1990, Sun Microsystems, Inc.  All Rights Reserved.
	Sun considers its source code as an unpublished, proprietary
	trade secret, and it is available only under strict license
	provisions.  This copyright notice is placed here only to protect
	Sun in the event the source is deemed a published work.  Dissassembly,
	decompilation, or other means of reducing the object code to human
	readable form is prohibited by the license agreement under which
	this code is provided to the user or company in possession of this
	copy.

	RESTRICTED RIGHTS LEGEND: Use, duplication, or disclosure by the 
	Government is subject to restrictions as set forth in subparagraph 
	(c)(1)(ii) of the Rights in Technical Data and Computer Software 
	clause at DFARS 52.227-7013 and in similar clauses in the FAR and 
	NASA FAR Supplement. */

#define mt_draw_text(display, drawable, gc, x, y, string, string_len) \
	{ \
	XDrawString(display, drawable, gc, x, y, string, string_len); \
	}


#define mt_invert_region(display, drawable, gc, x, y, w, h, fg, bg) \
	{ \
	XSetForeground(display, gc, (fg ^ bg)); \
	XSetFunction(display, gc, GXxor); \
	XFillRectangle(display, drawable, gc, x, y, w, h); \
	XSetFunction(display, gc, GXcopy); \
	XSetForeground(display, gc, fg); \
	}

#define mt_rop(display, tgt_drawable, gc, x, y, w, h, src_drawable) \
	{ \
	XSetStipple(display, gc, src_drawable); \
	XSetFillStyle(display, gc, FillOpaqueStippled); \
	XSetTSOrigin(display, gc, x, y); \
	XFillRectangle(display, tgt_drawable, gc, x, y, w, h); \
	XSetFillStyle(display, gc, FillSolid); \
	}
	
