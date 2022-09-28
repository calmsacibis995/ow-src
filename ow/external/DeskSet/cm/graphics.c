#ifndef lint
static  char sccsid[] = "@(#)graphics.c 3.9 93/02/16 Copyr 1991 Sun Microsystems, Inc.";
#endif
/*	graphics.c */

#include <string.h>
#include <xview/xview.h>
#include <xview/canvas.h>
#include <xview/panel.h>
#include <xview/font.h>
#include <xview/cms.h>
#include "util.h"
#include "misc.h"
#include "graphics.h"

#define gray_data_75_width	8
#define gray_data_75_height	8
#define gray_data_50_width	8
#define gray_data_50_height	8
#define gray_data_25_width	8
#define gray_data_25_height	8
#define black_data_width	8
#define black_data_height	8

#define solid_list_length		2
#define short_dotted_list_length	2
#define dotted_list_length		2
#define dot_dashed_list_length		4
#define short_dashed_list_length	2
#define long_dashed_list_length		2
#define odd_dashed_list_length		3

static unsigned char solid[solid_list_length] = {1, 0};
static unsigned char short_dotted[short_dotted_list_length] = {1, 1};
static unsigned char dotted[dotted_list_length] = {3, 1};
static unsigned char dot_dashed[dot_dashed_list_length] = {3, 4, 3, 1};
static unsigned char short_dashed[short_dashed_list_length] = {4, 4};
static unsigned char long_dashed[long_dashed_list_length] = {4, 7};
static unsigned char odd_dashed[odd_dashed_list_length] = {1, 2, 3};

static unsigned char black_data[] = {
	0xFF,
	0xFF,
	0xFF,
	0xFF,
	0xFF,
	0xFF,
	0xFF,
	0xFF
};
static unsigned char gray_data_75[] = {
        0xDD,
        0xBB,
        0xEE,
        0xF7,
	0xDD,
	0xBB,
        0xEE,
        0xF7
};

static unsigned char gray_data_50[] = {
	0xAA, 
	0x55,
	0xAA, 
	0x55,
	0xAA, 
	0x55,
	0xAA, 
	0x55
};

static unsigned char gray_data_25[] = {
	0x88,
	0x22,
	0x44,
	0x11,
	0x88,
	0x22,
	0x44,
	0x11
};


Pixmap black_data_pixmap = NULL;
Pixmap gray_data_75_pixmap = NULL;
Pixmap gray_data_50_pixmap = NULL;
Pixmap gray_data_25_pixmap = NULL;

static unsigned char *dash_list[] = {
	solid,
        short_dotted,
        dotted,
        dot_dashed,
        short_dashed,
        long_dashed,
        odd_dashed,
};

extern void
gr_clear_area(xc, x, y, w, h)
	XContext *xc; int x, y, w, h;
{
        XFillRectangle(xc->display, xc->xid, xc->clear_gc, x, y, w, h);
}

extern void
gr_draw_line(xc, x1, y1, x2, y2, style, rect)
	XContext *xc; int x1, y1, x2, y2;
	GR_Line_Style style;
	Rect *rect;
{
	int dash_offset = 0;
	
	if (rect != NULL) {
		Rect	gr_rect;

		gr_rect.r_left = x1;
		gr_rect.r_top = y1;
		gr_rect.r_width = x2-x1;
		gr_rect.r_height = y2-y1;
		if (!myrect_intersectsrect(rect, &gr_rect))
                        return;
	}
	if (style==gr_solid) {
		XDrawLine(xc->display, xc->xid, xc->draw_gc, x1, y1, x2, y2);
	}
	else {
		/* Set up and paint */
		XSetForeground(xc->display, xc->gc, xc->foreground);
		XSetDashes(xc->display, xc->gc, dash_offset,
			(char *)dash_list[style], 
			short_dotted_list_length);
		XSetLineAttributes(xc->display, xc->gc, 0,
			LineOnOffDash, CapNotLast, JoinMiter);
		XDrawLine(xc->display, xc->xid, xc->gc, x1, y1, x2, y2);
	}
	
}
extern void
gr_make_rgbcolor(xc, x, y, w, h, r, g, b)
	XContext *xc;
        int x, y, w, h;
	int r, g, b;
{
        XColor colorcell_del;
 
	colorcell_del.red = (unsigned short)(r<<8);
	colorcell_del.green = (unsigned short)(g<<8);
	colorcell_del.blue = (unsigned short)(b<<8);
	XAllocColor(xc->display,
		xv_get(xv_get(xc->drawable, WIN_CMS), CMS_CMAP_ID),
                	&colorcell_del);


        XSetForeground((Display*)xc->display, (GC)xc->gc,
		       (unsigned long)colorcell_del.pixel);
        XSetFillStyle(xc->display, xc->gc, FillSolid);
        XFillRectangle(xc->display, xc->xid, xc->gc, x, y, w, h);
}

extern void
gr_draw_rgb_box(xc, x, y, w, h, shade, cms)
        XContext *xc; 
	int x, y, w, h, shade;
	Cms cms;
{
        XColor colorcell_del, rgb_db_ref;
	unsigned long pixel;
	int	gc_changed = 0;

	switch(shade) {
		case RED: 
			/*
			XGetGCValues(xc->display, xc->gc,
                	GCForeground|GCFillStyle, xc->gcvals);
			*/
			pixel = xc->colorcell_del[RED].pixel;
		break;
		case CMS_BACKGROUND_PIXEL:
			pixel = (unsigned long)xv_get(cms, 
					CMS_BACKGROUND_PIXEL);
	}

	XSetForeground((Display*)xc->display, (GC)xc->gc, pixel);
	XSetFillStyle(xc->display, xc->gc, FillSolid);

	XDrawRectangle(xc->display, xc->xid, xc->gc, x, y, w, h);
}

extern void
gr_make_grayshade(xc, x, y, w, h, shade)
	XContext *xc; int x, y, w, h, shade;
{
	XColor colorcell_del, rgb_db_ref;

	switch (shade) {
	case DARKGREY:
		XSetForeground((Display*)xc->display, (GC)xc->gc, (unsigned long)xc->colorcell_del[DARKGREY].pixel);
		break;
	case DIMGREY:
		XSetForeground((Display*)xc->display, (GC)xc->gc, (unsigned long)xc->colorcell_del[DIMGREY].pixel);
		break;
	case GREY:
		XSetForeground((Display*)xc->display, (GC)xc->gc, (unsigned long)xc->colorcell_del[GREY].pixel);
		break;
	case LIGHTGREY:
		XSetForeground((Display*)xc->display, (GC)xc->gc, (unsigned long)xc->colorcell_del[LIGHTGREY].pixel);
		break;
	}

	XSetFillStyle(xc->display, xc->gc, FillSolid);
	XFillRectangle(xc->display, xc->xid, xc->gc, x, y, w, h);
}

extern void
gr_make_black(xc, x, y, w, h)
	XContext *xc; int x, y, w, h;
{
	/* Set up and paint black */
	XSetForeground(xc->display, xc->gc,
		xv_get(xv_get(xc->drawable, WIN_CMS),
                        CMS_FOREGROUND_PIXEL));
	XSetFillStyle(xc->display, xc->gc, FillSolid);
	XFillRectangle(xc->display, xc->xid, xc->gc, x, y, w, h);
}

extern void
gr_make_gray (xc, x, y, w, h, percent)
	XContext *xc; int x, y, w, h, percent;
{
	/* Set up gray stipple */
	switch(percent) {
	case 25:
		XSetStipple(xc->display, xc->gc, gray_data_25_pixmap);
		break;	
	case 50:
		XSetStipple(xc->display, xc->gc, gray_data_50_pixmap);
		break;
	case 75:
		XSetStipple(xc->display, xc->gc, gray_data_75_pixmap);
		break;
	default:
		XSetStipple(xc->display, xc->gc, gray_data_25_pixmap);
		break;
	}

	XSetForeground(xc->display, xc->gc, xc->foreground);
	XSetFillStyle(xc->display, xc->gc, FillStippled);
	XFillRectangle(xc->display, xc->xid, xc->gc, x, y, w, h);
}
  
  
extern void
gr_draw_box (xc, x, y, w, h, rect) 
	XContext *xc; int x, y, w, h;
	Rect *rect;
{
	if (rect != NULL) {
		Rect	gr_rect;

		gr_rect.r_left = x;
		gr_rect.r_top = y;
		gr_rect.r_width = w;
		gr_rect.r_height = h;
		if (!myrect_intersectsrect(rect, &gr_rect))
                        return;
	}
	XDrawRectangle(xc->display, xc->xid, xc->draw_gc, x, y, w, h);
}
extern void
gr_draw_glyph(src_xc, dst_xc, pixmap, x, y, w, h) 
	XContext *src_xc, *dst_xc; 
	Pixmap pixmap;
	int x, y, w, h;
{
	XSetStipple(src_xc->display, src_xc->gc, pixmap); 
        XSetTSOrigin(src_xc->display, src_xc->gc, x, y); 
        XFillRectangle(src_xc->display, dst_xc->xid, src_xc->gc, x, y, w, h); 
}
	
extern void
gr_dissolve_box (xc, x, y, w, h)
	XContext *xc;
	int x, y, w, h;
{
	XDrawRectangle(xc->display, xc->xid, xc->clear_gc, x, y, w, h);
}

extern void
gr_clear_box (xc, x, y, w, h)
	XContext *xc;
	int x, y, w, h;
{
	x+=1; y+=1; w-=1; h-=1; 

	XFillRectangle(xc->display, xc->xid, xc->clear_gc, x, y, w, h);
}
      
extern void
gr_invert_box(xc, x, y, w, h)
	XContext *xc;
	int x, y, w, h;
{
	x+=2; y+=2; w-=3; h-=3;	

	/*  XOR in background color */
	XFillRectangle(xc->display, xc->xid, xc->invert_gc, x, y, w, h);  
}

extern void
gr_gray_box(xc, x, y, w, h)
	XContext *xc;
	int x, y, w, h;
{
	x+=3; y+=3; w-=6; h-=6;
	gr_make_gray(xc, x, y, w, h, 25);
}
extern void
gr_text_rgb(xc, x, y, pf, str, shade, cms, rect)
	XContext *xc;
	int x, y;
	Xv_font pf;
	char *str;
	int shade;
	Cms cms;
	Rect *rect;
{
	XColor colorcell_del, rgb_db_ref;
	unsigned long pixel;
#ifdef OW_I18N
	XFontSet fontset;
#else
	XTextItem text[1];

	/* Set up XTextItem struct */
	text[0].chars	= str;
	text[0].nchars	= cm_strlen(str);
	text[0].delta	= 0;
	text[0].font	= xv_get(pf, XV_XID);
#endif
	
	if (rect != NULL) {
		Rect	gr_rect;
		Font_string_dims dims;

		xv_get(pf, FONT_STRING_DIMS, str, &dims);
		gr_rect.r_left = x;
		gr_rect.r_top = y - dims.height;
		gr_rect.r_width = dims.width;
		gr_rect.r_height = dims.height;
		if (!myrect_intersectsrect(rect, &gr_rect))
                        return;
	}

	switch(shade) {
		case RED: 
			XGetGCValues(xc->display, xc->gc,
                	GCForeground|GCFillStyle, xc->gcvals);
			pixel = xc->colorcell_del[RED].pixel;
			break;
		case CMS_BACKGROUND_PIXEL:
			pixel = (unsigned long)xv_get(cms, CMS_BACKGROUND_PIXEL);
			break;
	}
	XSetForeground((Display*)xc->display, (GC)xc->gc, 
			(unsigned long)pixel);
	/* Set up and paint */
#ifdef OW_I18N
	fontset = (XFontSet) xv_get(pf, FONT_SET_ID);
	XmbDrawString(xc->display, xc->xid, fontset, xc->gc, x, y, str, cm_strlen(str));
#else
	XDrawText(xc->display, xc->xid, xc->gc, x, y, text, 1);
#endif
}

extern void
gr_text(xc, x, y, pf, str, rect)
	XContext *xc;
	int x, y;
	Xv_font pf;
	char *str;
	Rect 	*rect;
{
#ifdef OW_I18N
	XFontSet fontset;
#else
	XTextItem text[1];

	/* Set up XTextItem struct */
	text[0].chars	= str;
	text[0].nchars	= cm_strlen(str);
	text[0].delta	= 0;
	text[0].font	= xv_get(pf, XV_XID);
#endif
	
	if (rect != NULL) {
		Font_string_dims dims;
		Rect	gr_rect;

		xv_get(pf, FONT_STRING_DIMS, str, &dims);
		gr_rect.r_left = x;
		gr_rect.r_top = y - dims.height;
		gr_rect.r_width = dims.width;
		gr_rect.r_height = dims.height;
		if (!myrect_intersectsrect(rect, &gr_rect))
                        return;
	}
#ifdef OW_I18N
	fontset = (XFontSet) xv_get(pf, FONT_SET_ID);
	XmbDrawString(xc->display, xc->xid, fontset, xc->draw_gc, x, y,
		      str, cm_strlen(str));
#else
	XDrawText(xc->display, xc->xid, xc->draw_gc, x, y, text, 1);
#endif
}

/*  given an area of a certain length (in pixels), compute
    where to lay down a string such that it's centered */
    
extern int
gr_center(area, str, font)
     int area; char *str; Xv_Font font;
{
	Font_string_dims dims;
	int i, l, w, first = 1;
	char *buf;

	w = 0;
	l = cm_mbstrlen(str);
	for (i=0; i<l; i++) {
		if ( first ) {
			buf = cm_mbchar(str);
			first = 0;
		} else {
			buf = cm_mbchar((char *)NULL);
		}
		xv_get(font, FONT_STRING_DIMS, buf, &dims);
		w+=dims.width;
	}
	return ((area - w)/2);
}

/*	given an area of a certain length (in pixels), compute
	how many bytes of the string in that variable length
	font may be displayed in that area			*/

extern int
gr_nchars(area, str, font)
	int area; char *str; Xv_Font font;
{
	Font_string_dims dims;
	char *buf;
	int i, l, w=0, n=0;
	int first = 1;

	xv_get(font, FONT_STRING_DIMS, str, &dims);
	if (dims.width <= area) {
		return(strlen(str));
	}
	l = cm_mbstrlen(str);
	for (i=0; i<l; i++) {
		if ( first ) {
			buf = cm_mbchar(str);
			first = 0;
		} else {
			buf = cm_mbchar((char *)NULL);
		}
		xv_get(font, FONT_STRING_DIMS, buf, &dims);
		w+=dims.width;
		if (w <= area)
			n += mblen(buf, MB_LEN_MAX);
		else break;
	}
	return(n);
}



/*	given an XContext, pattern data, pattern width & height,
	create a stipple and return it in the stipple struct
	provided along with its width and height			*/

extern Boolean
gr_create_stipple(xc, data, datawidth, dataheight, stipple, width, height)
	XContext *xc; char *data; int datawidth, dataheight;
	Pixmap *stipple; unsigned int *width, *height; 
{

	Boolean ok = true;

	if ((*stipple = XCreateBitmapFromData(xc->display,
		xc->xid,
	/*	RootWindow(xc->display, DefaultScreen(xc->display)),  */
		data, datawidth, dataheight)) == NULL) {
			ok = false;
		}
	else {
		*width	= datawidth;
		*height = dataheight;
	}
	return(ok);
}

/*	init the graphics package: create tiles & stipples	*/

extern Boolean
gr_init(xc)
	XContext *xc;
{
	unsigned int width, height;
	Boolean ok = true;
	XColor	rgb_db_ref;


	ok = gr_create_stipple(xc, black_data,
		black_data_width, black_data_height,
		&black_data_pixmap, &width, &height);
	ok = gr_create_stipple(xc, gray_data_75,
		gray_data_75_width, gray_data_75_height,
		&gray_data_75_pixmap, &width, &height);
	ok = gr_create_stipple(xc, gray_data_50,
		gray_data_50_width, gray_data_50_height,
		&gray_data_50_pixmap, &width, &height);
	ok = gr_create_stipple(xc, gray_data_25,
		gray_data_25_width, gray_data_25_height,
		&gray_data_25_pixmap, &width, &height);

	ok = XAllocNamedColor(xc->display,
                xv_get(xv_get(xc->drawable, WIN_CMS), CMS_CMAP_ID),
                "dark slate grey", &xc->colorcell_del[DARKGREY], &rgb_db_ref);
	ok = XAllocNamedColor(xc->display,
                xv_get(xv_get(xc->drawable, WIN_CMS), CMS_CMAP_ID),
                "dim grey", &xc->colorcell_del[DIMGREY], &rgb_db_ref);
	ok = XAllocNamedColor(xc->display,
                xv_get(xv_get(xc->drawable, WIN_CMS), CMS_CMAP_ID),
                "grey", &xc->colorcell_del[GREY], &rgb_db_ref);
	ok = XAllocNamedColor(xc->display,
                xv_get(xv_get(xc->drawable, WIN_CMS), CMS_CMAP_ID),
                "light grey", &xc->colorcell_del[LIGHTGREY], &rgb_db_ref);
	ok = XAllocNamedColor(xc->display,
		xv_get(xv_get(xc->drawable, WIN_CMS), CMS_CMAP_ID),
		"red", &xc->colorcell_del[RED], &rgb_db_ref);

	return(ok);
}

/*
 * Allocate and initialize an XContext
 */
XContext *
gr_create_xcontext(win, drawable, depth)

	Xv_Window	win;	
	Xv_Drawable	drawable;	
	GR_depth	depth; /* gr_mono for 1 bit drawables */

{
	XContext	*xc;
	Cms		cms = NULL;
	XGCValues	gc_vals;

	/* X Drawing Stuff */
	if ((xc = (XContext *) ckalloc(sizeof(XContext))) == NULL)
		return NULL;

	xc->display = (Display *) xv_get(win, XV_DISPLAY);
	xc->drawable = drawable; 

	if (depth == gr_mono) {
		xc->foreground = 1;
		xc->background = 0;
	} else {
		if ((cms = (Cms) xv_get(win, WIN_CMS)) != NULL) {
			xc->foreground = (int)xv_get(cms, CMS_FOREGROUND_PIXEL);
			xc->background = (int)xv_get(cms, CMS_BACKGROUND_PIXEL);
		} else {
			xc->foreground = (int)BlackPixel(xc->display,
						 DefaultScreen(xc->display));
			xc->background = (int)WhitePixel(xc->display,
						 DefaultScreen(xc->display));
		}
	}

	xc->xid = (int) xv_get(drawable, XV_XID);
        xc->screen_depth = xv_get(win, WIN_DEPTH);

	/*
	 * Create general purpose gc.  This gc is changed as needed by
	 * the drawing routines.
	 */
	xc->gcvals = (XGCValues *) ckalloc(sizeof(XGCValues));
	xc->gcvals->foreground = xc->foreground;
	xc->gcvals->background = xc->background;

	xc->gcvals->fill_style = FillOpaqueStippled;
	xc->gc = XCreateGC(
		xc->display,
		xc->xid,
		GCForeground|GCBackground|GCFillStyle,
		xc->gcvals);


	/*
	 * Specialized GCs. We create a couple of specialized GCs to increase
	 * the speed of common operations.  This way we don't need to change
	 * the GC for every operation.
	 */

	/* GC used for clearing */
	gc_vals.foreground = xc->background;
	xc->clear_gc = XCreateGC(xc->display, xc->xid, GCForeground, &gc_vals);

	/* Create GC used for inverting */
	gc_vals.function = GXequiv;
	xc->invert_gc = XCreateGC(xc->display, xc->xid,
				 GCForeground | GCFunction, &gc_vals);

	/* Create GC used for drawing */
	gc_vals.function = GXcopy;
	gc_vals.foreground = xc->foreground;
	xc->draw_gc = XCreateGC(xc->display, xc->xid, GCForeground | GCFunction,
				&gc_vals);
	XSetLineAttributes(xc->display, xc->draw_gc, 0,
			   LineSolid, CapNotLast, JoinMiter);

	return xc;
}

void
gr_free_xcontext(xc)

	XContext	*xc;

{
	/* Free up associated GCs */
	XFreeGC(xc->display, xc->gc);
	XFreeGC(xc->display, xc->draw_gc);
	XFreeGC(xc->display, xc->clear_gc);
	XFreeGC(xc->display, xc->invert_gc);

	/* Free associated data */
	free(xc->gcvals);
	free(xc);

	return;
}

/*
 * Set the clip mask for all gcs the graphics package uses
 */
void
gr_set_clip_rectangles(xc, x, y, rectangles, n, ordering)

	XContext	*xc;
	int		x, y;
	XRectangle	*rectangles;
	int		n;
	int		ordering;

{
	XSetClipRectangles(xc->display, xc->gc, x, y, rectangles, n, ordering);
	XSetClipRectangles(xc->display, xc->draw_gc,
			   x, y, rectangles, n, ordering);
	XSetClipRectangles(xc->display, xc->clear_gc,
			   x, y, rectangles, n, ordering);
	XSetClipRectangles(xc->display, xc->invert_gc,
			   x, y, rectangles, n, ordering);
	return;
}

/*
 * Clear the clip masks of all gcs
 */
void
gr_clear_clip_rectangles(xc)

	XContext	*xc;
{
	xc->gcvals->clip_mask = None;
	XSetClipMask(xc->display, xc->gc, None);
	XSetClipMask(xc->display, xc->draw_gc, None);
	XSetClipMask(xc->display, xc->clear_gc, None);
	XSetClipMask(xc->display, xc->invert_gc, None);
	return;
}
