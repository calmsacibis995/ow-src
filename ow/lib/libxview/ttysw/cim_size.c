#ifndef lint
#ifdef sccs
static char     sccsid[] = "@(#)cim_size.c 20.33 93/07/26";
#endif
#endif

/*
 *	(c) Copyright 1989-1993 Sun Microsystems, Inc. Sun design patents
 *	pending in the U.S. and foreign countries. See LEGAL_NOTICE
 *	file for terms of the license.
 */

/*
 * Character image initialization, destruction and size changing routines
 */

#include <sys/types.h>
#include <sys/time.h>
#include <sys/signal.h>
#include <stdio.h>
#include <string.h>
#include <pixrect/pixrect.h>
#include <pixrect/pixfont.h>
#include <xview/notify.h>
#include <xview/rect.h>
#include <xview/rectlist.h>
#include <xview/pixwin.h>
#include <xview/win_input.h>
#include <xview/win_notify.h>
#include <xview/pkg.h>
#include <xview/attrol.h>
#include <xview_private/i18n_impl.h>
#include <xview_private/tty_impl.h>
#include <xview_private/ttyansi.h>
#include <xview_private/charimage.h>
#include <xview_private/charscreen.h>
#include <xview_private/portable.h>

void          **image;		
char          **screenmode; 
int           *linelength;

static void   *lines_ptr;
static char   *mode_ptr; 

static void  **temp_image;
static char  **temp_mode; 
static int    *temp_linelength;
  
static void   *temp_lines_ptr;
static char     *temp_mode_ptr; 

static int      maxright, maxbottom;

/*
 * Initialize initial character image.
 */
Pkg_private int
xv_tty_imageinit(ttysw, window)
    Ttysw          *ttysw;
    Xv_object       window;
{
    void            xv_tty_imagealloc();
    int             maximagewidth, maximageheight;

    if (wininit(window, &maximagewidth, &maximageheight) == 0)
	return (0);
    ttysw_top = ttysw_left = 0;
    curscol = ttysw_left;
    cursrow = ttysw_top;
    maxright = x_to_col(maximagewidth);
    if (maxright > 255)
	maxright = 255;		/* line length is stored in a byte */
    maxbottom = y_to_row(maximageheight);
    (void) xv_tty_imagealloc(ttysw, FALSE);
    (void) ttysw_pclearscreen(0, ttysw_bottom + 1); /* +1 to get remnant at
						 * bottom */
    return (1);
}

/*
 * Allocate character image.
 */
Pkg_private void
xv_tty_imagealloc(ttysw, for_temp)
    Ttysw          *ttysw;
    int             for_temp;	/* decide which data structure to go into */
{
    register void **newimage;
    register char **newmode;
    register int    i;
    int             nchars;
    void           *line;
    register char  *bold;
    register int   *newlinelength;
    /*
     * Determine new screen dimensions
     */
    ttysw_right = x_to_col(winwidthp);
    ttysw_bottom = y_to_row(winheightp);

    /* Ensure has some non-zero dimension */
    if (ttysw_right < 1)
	ttysw_right = 1;
    if (ttysw_bottom < 1)
	ttysw_bottom = 1;

    /* Bound new screen dimensions */
    ttysw_right = (ttysw_right < maxright) ? ttysw_right : maxright;
    ttysw_bottom = (ttysw_bottom < maxbottom) ? ttysw_bottom : maxbottom;

    /* Let pty set terminal size */
    (void) xv_tty_new_size(ttysw, ttysw_right, ttysw_bottom);

    /* Allocate line array and character storage */
    nchars = ttysw_right * ttysw_bottom;
    if (!multibyte) {
        newimage = xv_calloc(1, ttysw_bottom * sizeof(char *));
        line = (char *)xv_calloc(1, nchars + ttysw_bottom);
    } else {
        newimage = xv_calloc(1, ttysw_bottom * sizeof(wchar_t *));
        line = (wchar_t *)xv_calloc(1, (nchars + ttysw_bottom) *
                                                sizeof(wchar_t));        
    }
    newlinelength = (int *)xv_calloc(1, ttysw_bottom * sizeof(int));
    newmode = (char **)xv_calloc(1, ttysw_bottom * sizeof(char *));
    bold = (char *)xv_calloc(1, nchars + ttysw_bottom);

    for (i = 0; i < ttysw_bottom; i++) {
        if (!multibyte) {
	    newimage[i] = (char *)line;
            line = (char *)line + (ttysw_right + 1);
        } else  {
	    newimage[i] = (wchar_t *)line;
            line = (wchar_t *)line + (ttysw_right + 1);
        }
	newmode[i] = bold;
	bold += ttysw_right + 1;
    }

    if (for_temp) {
	temp_image = newimage;
	temp_mode = newmode;
	temp_lines_ptr = newimage[0];
	temp_mode_ptr = newmode[0];
        temp_linelength = newlinelength;
    } else {
	image = newimage;
	screenmode = newmode;
	lines_ptr = newimage[0];
	mode_ptr = newmode[0];
        linelength = newlinelength;
    }
}


Pkg_private void
xv_tty_free_image_and_mode()
{

    if (lines_ptr) {
         xv_free((char *)lines_ptr);
	 lines_ptr = NULL;
    }

    if (image) {
        xv_free((void **)image);
	image = NULL;
    }
    
    if (mode_ptr) {
	xv_free((char *)mode_ptr);
	mode_ptr = NULL;
    }

    if (screenmode) {
	xv_free((char **)screenmode);
	screenmode = NULL;
    }

    if (linelength) {
        xv_free((int *)linelength);
        linelength = NULL;
    }

}



/*
 * Called when screen changes size.  This will let lines get longer (or
 * shorter perhaps) but won't re-fold older lines.
 */
Pkg_private void
ttysw_imagerepair(ttysw_view)
    Ttysw_view_handle ttysw_view;
{
    Ttysw_folio     ttysw = TTY_FOLIO_FROM_TTY_VIEW_HANDLE(ttysw_view);
    void            **oldimage;
    register int    oldrow, row;
    int             oldbottom = ttysw_bottom;
    int             topstart;
    int             i;
    register int    sl;

    /* Get new image and image description */
    (void) xv_tty_imagealloc(ttysw, TRUE);

    /* Find out where last line of text is (actual oldbottom). */
    for (row = oldbottom; row > ttysw_top; row--) {
	if (linelength[row-1]) {
	    oldbottom = row;
	    break;
	}
    }

    /*
     * Try to preserve bottom (south west gravity) text. This wouldn't work
     * well for vi and other programs that know about the size of the
     * terminal but aren't notified of changes. However, it should work in
     * many cases  for straight tty programs like the shell.
     */
    if (oldbottom > ttysw_bottom)
	topstart = oldbottom - ttysw_bottom;
    else
	topstart = 0;

    /* Fill in new screen from old */
    ttysw->ttysw_lpp = 0;

    for (oldrow = topstart, row = 0; oldrow < oldbottom; oldrow++, row++) {
        if (!multibyte) {
	    sl = strlen(((char **)image)[oldrow]);
	    if (sl > ttysw_right) sl = ttysw_right;
	    XV_BCOPY(((char **)image)[oldrow],((char **)temp_image)[row], sl);
        } else {
	    sl = wslen(((wchar_t **)image)[oldrow]);
	    if (sl > ttysw_right) sl = ttysw_right;
	    XV_BCOPY(((wchar_t **)image)[oldrow],((wchar_t **)temp_image)[row], 
                                    sl*sizeof(wchar_t));
        }
        XV_BCOPY(screenmode[oldrow], temp_mode[row], sl);
	temp_linelength[row] = sl;
    }
    xv_tty_free_image_and_mode();

    image = temp_image;
    screenmode = temp_mode;
    lines_ptr = temp_lines_ptr;
    mode_ptr = temp_mode_ptr;
    linelength = temp_linelength;

    /*
     * Move the cursor to its new position in the new coordinate system. If
     * the window is shrinking, and thus "topstart" is the number of rows by
     * which it is shrinking, the row number is decreased by that number of
     * rows; if the window is growing, and thus "topstart" is zero, the row
     * number is unchanged. The column number is unchanged, unless the old
     * column no longer exists (in which case the cursor is placed at the
     * rightmost column).
     */
    ttysw_pos(curscol, cursrow - topstart);
}
