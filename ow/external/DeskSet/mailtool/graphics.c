#ifndef lint
#ifdef sccs
static  char sccsid[] = "@(#)graphics.c 3.2 93/05/03 Copyr 1989 Sun Micro";
#endif
#endif

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


#include <X11/Xlib.h>
#include <X11/Xutil.h>

#include <xview/xview.h>
#include <xview/font.h>
#include <xview/canvas.h>
#include <xview/cms.h>
#include <xview/svrimage.h>

#include "header.h"

extern Xv_Font	mt_font;


mt_init_graphics(hd)

	struct header_data *hd;

{
	int		depth;	/* depth of the header canvas */
	XGCValues	gc_vals;
	int		gc_flags;
	Cms		cms;
	Server_image	icon_image;
	Icon		icon;
	

	/*
	 * Get the color map segment for the canvas and extract the foreground
	 * and background pixel values from it.
	 */
	cms = (Cms)xv_get(hd->hd_paintwin, WIN_CMS);
	hd->hd_foreground = (int) xv_get(cms, CMS_FOREGROUND_PIXEL);
	hd->hd_background = (int) xv_get(cms, CMS_BACKGROUND_PIXEL);

	hd->hd_display = (Display *)xv_get(hd->hd_paintwin, XV_DISPLAY);
	hd->hd_drawable = (Drawable) xv_get(hd->hd_paintwin, XV_XID);
	hd->hd_screen = DefaultScreen(hd->hd_display);

	icon = (Icon)xv_get(hd->hd_frame, FRAME_ICON);
	icon_image = (Server_image)xv_get(icon, ICON_IMAGE);
	hd->hd_icon_drawable = (Drawable)xv_get(icon_image, XV_XID);
	hd->hd_fontid = 
		((XFontStruct *)xv_get(hd->hd_textfont, FONT_INFO) )->fid;

 	gc_vals.font = hd->hd_fontid;
 	gc_vals.function = GXcopy;
 	gc_vals.foreground = hd->hd_foreground;
 	gc_vals.background = hd->hd_background;
 	gc_vals.fill_style = FillSolid;

 	gc_flags = GCFont | GCFunction | GCForeground | GCBackground |
		GCFillStyle;

	/*
	 * Create GC used for text and line drawing.
	 */
 	hd->hd_gc = XCreateGC(hd->hd_display, hd->hd_drawable,
		gc_flags, &gc_vals);

	/*
	 * Create GC used for manipulating the icon
	 */
 	hd->hd_icongc = XCreateGC(hd->hd_display, hd->hd_icon_drawable,
		gc_flags, &gc_vals);

	/*
	 * Create GC used for drawing attachments
	 */
 	hd->hd_attachgc = XCreateGC(hd->hd_display, hd->hd_drawable,
		gc_flags, &gc_vals);

	XSetFont(hd->hd_display, hd->hd_attachgc, hd->hd_fontid);


	/*
	 * Create GC used for clearing.  When we clear we actually just
	 * paint with the background color.
	 */
 	gc_vals.foreground = hd->hd_background;
 	gc_vals.background = hd->hd_foreground;
 	hd->hd_cleargc = XCreateGC(hd->hd_display, hd->hd_drawable,
		gc_flags, &gc_vals);
}
