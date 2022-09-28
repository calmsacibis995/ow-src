/*static  char sccsid[] = "@(#)graphics.h 3.6 93/01/20 Copyr 1991 Sun Microsystems, Inc.";
 *
 *	graphics.h 
 *
 */

#define DARKGREY 	1
#define DIMGREY		2
#define GREY 		3
#define LIGHTGREY 	4
#define RED 		5

#define MIDGREY 150

typedef struct {
	int		foreground;	/* X Default Foreground Color */
	int		background;	/* X Default Background Color */
        int             xid;		/* XID */
        Xv_window       drawable;	/* X Drawable */
        Display         *display;	/* X Display */

	/*
	 * gc is a general purpose GC which is configured on the fly for
	 * infrequent operations.  For frequent operations we use one of
	 * the pre-configured GCs which we never change.
	 */
        GC              gc;		/* X Graphics Context */
        XGCValues       *gcvals;	/* X Graphics Context Values */
	GC		draw_gc;	/* GC for drawing  */
	GC		clear_gc;	/* GC for clearing */
	GC		invert_gc;	/* GC for inverting */

	int             screen_depth;   /* screen depth */
	XColor		colorcell_del[RED+1];/* rgb colors */
} XContext;

typedef enum {gr_solid, gr_short_dotted, gr_dotted,
              gr_dot_dashed, gr_short_dashed,
	      gr_long_dashed, gr_odd_dashed} GR_Line_Style;

typedef enum {gr_mono, gr_color} GR_depth;

/*	given an x, y, w, h, clear rectangular area		*/

extern void gr_clear_area(/* XContext *xc; int x, y, w, h */);	

/*	given an XContext and a source x, a source y, a length
	and height, draw a line.				*/

extern void gr_draw_line(/* XContext *xc;
	int x1, y1, x2, y2; GR_Line_Style style */);

/*	given an XContext and a rectangle defined by an x,y,w,h
	make that area black.					*/

extern void gr_make_black (/* XContext *xc; int x, y, w, h */);

/*	given an XContext and a rectangle defined by an x,y,w,h
	and a desired % fill, make that area gray.		*/

extern void gr_make_gray (/* XContext *xc; int x, y, w, h, percent */);

/*	given an XContext and a rectangle defined by an x,y,w,h,
	draw a box w/ line width linew.				*/

extern void gr_draw_box (/* XContext *xc; int x, y, w, h, linew */);

/*	given an XContext and a rectangle defined by an x,y,w,h
	and linew, where a box was drawn,
	remove the box.						*/

extern void gr_dissolve_box (/* XContext *xc; int x, y, w, h */);

/*	given an XContext and a rectangle defined by an x,y,w,h
	where a box was drawn, white out the contents of the
	box, but leave the box.					*/

extern void gr_clear_box (/* XContext *xc; int x, y, w, h */);

/*	given an XContext and a rectangle defined by an x,y,w,h
	where a box was drawn, toggle the color of the contents
	of the box.						*/

extern void gr_invert_box (/* XContext *xc; int x, y, w, h */);

/*	given an XContext and a rectangle define by an x,y,w,h
	where a box was drawn, turn color to gray.		*/

extern void gr_gray_box (/* XContext *xc; int x, y, w, h */);

/*      given an area of a certain length (in pixels), compute 
        how many characters of the string in that variable length
        font may be displayed in that area                      */

extern int gr_nchars(/* int area; char *str; Xv_Font font; */);

/*      given an area in pixels, a string, and a font
        return the offset into the area to lay down the string
	so that it will be centered.				*/

extern int gr_center (/* int area; char *str; Xv_Font font */);

/*	given an x, y, a string and a font
	paint the text of the string in desired font.		*/

extern void gr_text(/* XContext; int x, y; Xv_font pf */);

/*	given included data, data width, data height, a
	Pixmap to be filled up, and pointers to width and
	height variables to be filled upon error conditions,
	create a tile or stipple to be used for painting.	*/

extern Boolean gr_create_stipple(
	/* XContext *xc; char *data; int datawidth;
	int dataheight; Pixmap *stipple; unsigned int *width;
	unsigned int *height */);

/*	init the graphics package: create tiles and stipples	*/

extern Boolean gr_init(/* XContext *xc */);
extern void gr_make_rgbcolor(/* xc, x, y, w, h, r, g, ba */);
extern void gr_make_grayshade();
extern void gr_draw_rgb_box();
extern XContext	*gr_create_xcontext();
extern void	gr_free_xcontext();
extern void	gr_set_clip_rectangles();
extern void	gr_clear_clip_rectangles();
#define myrect_intersectsrect(r1,r2) \
        ((r1)->r_left<=(r2)->r_left+(r2)->r_width && \
         (r1)->r_top<=(r2)->r_top+(r2)->r_height &&  \
         (r2)->r_left<=(r1)->r_left+(r1)->r_width && \
         (r2)->r_top<=(r1)->r_top+(r1)->r_height)
