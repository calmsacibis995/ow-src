#ifndef lint
#ifdef sccs
static char     sccsid[] = "@(#)csr_change.c 20.53 95/01/30";
#endif
#endif
/*
 *	(c) Copyright 1989 Sun Microsystems, Inc. Sun design patents 
 *	pending in the U.S. and foreign countries. See LEGAL NOTICE 
 *	file for terms of the license.
 */

/*
 * Character screen operations (except size change and init).
 */

#include <xview_private/tty_impl.h>
#include <sys/types.h>
#include <sys/time.h>
#include <string.h>
#include <X11/Xlib.h>
#include <pixrect/pixrect.h>
#include <pixrect/pixfont.h>

#ifdef __STDC__ 
#ifndef CAT
#define CAT(a,b)        a ## b 
#endif 
#endif
#include <pixrect/memvar.h>

#include <xview/rect.h>
#include <xview/rectlist.h>
#include <xview/pixwin.h>
#include <xview/ttysw.h>
#include <xview/sel_svc.h>

#include <xview_private/charimage.h>
#include <xview_private/charscreen.h>
#undef CTRL
#include <xview_private/ttyansi.h>
#include <xview_private/term_impl.h>
#include <xview/window.h>
#include <xview/pkg.h>
#include <xview/attrol.h>
#include <xview/server.h>
#include <xview/font.h>
Xv_private_data char *xv_shell_prompt;

#define TTYSW_HOME_CHAR	'A'

extern Xv_Window csr_pixwin;
extern int      cursor;		/* NOCURSOR, UNDERCURSOR, BLOCKCURSOR */

/* static */ void ttysw_fixup_display_mode();

static void ttysw_convert_string();
static void ttysw_paintCursor();

static int      caretx, carety, lxhome;
static short    charcursx, charcursy;

static  int     curs_width;

static int      boldstyle, inverse_mode, underline_mode;

extern struct timeval ttysw_bell_tv;	/* initialized to 1/10 second */

static u_short  ttysw_gray17_data[16] = {	/* really 16-2/3	 */
    0x8208, 0x2082, 0x0410, 0x1041, 0x4104, 0x0820, 0x8208, 0x2082,
    0x0410, 0x1041, 0x4104, 0x0820, 0x8208, 0x2082, 0x0410, 0x1041
};

mpr_static(ttysw_gray17_pr, 12, 12, 1, ttysw_gray17_data);

Pkg_private int /* Note: change to void */
ttysw_setboldstyle(new_boldstyle)
    int             new_boldstyle;
{
    if (new_boldstyle > TTYSW_BOLD_MAX
	|| new_boldstyle < TTYSW_BOLD_NONE)
	boldstyle = TTYSW_BOLD_NONE;
    else
	boldstyle = new_boldstyle;
    return boldstyle;
}

Pkg_private void
ttysw_set_inverse_mode(new_inverse_mode)
    int             new_inverse_mode;
{
    inverse_mode = new_inverse_mode;
}

Pkg_private void
ttysw_set_underline_mode(new_underline_mode)
    int             new_underline_mode;
{
    underline_mode = new_underline_mode;
}

Pkg_private int
ttysw_getboldstyle()
{
    return boldstyle;
}

/* NOT USED */
ttysw_get_inverse_mode()
{
    return inverse_mode;
}

/* NOT USED */
ttysw_get_underline_mode()
{
    return underline_mode;
}


Pkg_private void
ttysw_setleftmargin(left_margin)
{
    chrleftmargin = left_margin > 0 ? left_margin : 0;
}

/* NOT USED */
ttysw_getleftmargin()
{
    return chrleftmargin;
}

/* static */ void
ttysw_fixup_display_mode(mode)
    char           *mode;
{

    if ((*mode & MODE_INVERT) && (inverse_mode != TTYSW_ENABLE)) {
	*mode &= ~MODE_INVERT;
	if (inverse_mode == TTYSW_SAME_AS_BOLD)
	    *mode |= MODE_BOLD;
    }
    if ((*mode & MODE_UNDERSCORE) && (underline_mode != TTYSW_ENABLE)) {
	*mode &= ~MODE_UNDERSCORE;
	if (underline_mode == TTYSW_SAME_AS_BOLD)
	    *mode |= MODE_BOLD;
    }
    if ((*mode & MODE_BOLD) && (boldstyle & TTYSW_BOLD_INVERT)) {
	*mode &= ~MODE_BOLD;
	*mode |= MODE_INVERT;
    }
}

static void
renderString(s, len, l2, row, col, x_home, y_home, op, mode)
void *s;
int len, l2, row, col, x_home, y_home, op;
char mode;
{
    if (mode & MODE_BOLD) {
        /* Clean up first */
	(void) ttysw_pclearline(col, col + l2, row);
	/* render the first one, the potential offset of the others */
	tty_newtext(csr_pixwin,
		   col_to_x(col) - x_home,
		   row_to_y(row) - y_home,
		   (mode & MODE_INVERT) ? PIX_NOT(PIX_SRC) : op, 
		   pixfont, s, len);

	if (boldstyle & TTYSW_BOLD_OFFSET_X)
	    tty_newtext(csr_pixwin,
		       col_to_x(col) - x_home + 1,
		       row_to_y(row) - y_home,
		 (mode & MODE_INVERT) ? PIX_NOT(PIX_SRC) & PIX_DST :
		  PIX_SRC | PIX_DST, pixfont, s, len);
	if (boldstyle & TTYSW_BOLD_OFFSET_Y)
	    tty_newtext(csr_pixwin,
		       col_to_x(col) - x_home,
		       row_to_y(row) - y_home + 1,
		 (mode & MODE_INVERT) ? PIX_NOT(PIX_SRC) & PIX_DST :
		 PIX_SRC | PIX_DST, pixfont, s, len);
	if (boldstyle & TTYSW_BOLD_OFFSET_XY)
	    tty_newtext(csr_pixwin,
		       col_to_x(col) - x_home + 1,
		       row_to_y(row) - y_home + 1,
		 (mode & MODE_INVERT) ? PIX_NOT(PIX_SRC) & PIX_DST :
		 PIX_SRC | PIX_DST, pixfont, s, len);
    } else {
	tty_newtext(csr_pixwin,
		   col_to_x(col) - x_home,
		   row_to_y(row) - y_home,
                   (mode & MODE_INVERT) ? PIX_NOT(PIX_SRC) : op, 
                    pixfont, s, len);
    }
    if (mode & MODE_UNDERSCORE) {
        if (multibyte) {
            struct pr_size str_size;
            struct pr_size xv_pf_textwidth_wc();
            str_size = xv_pf_textwidth_wc(len,pixfont,s);
            tty_background(csr_pixwin,
                       col_to_x(col), row_to_y(row) + chrheight - 1,
                       str_size.x, 1,
                       (mode & MODE_INVERT) ? PIX_NOT(PIX_SRC) : PIX_SRC);
        } else 
	    tty_background(csr_pixwin,
		       col_to_x(col), row_to_y(row) + chrheight - 1,
		       len * chrwidth, 1,
                       (mode & MODE_INVERT) ? PIX_NOT(PIX_SRC) : PIX_SRC);
    }
}


/* Note: whole string will be diplayed with mode. */
Pkg_private void
ttysw_pstring(s, mode, col, row, op)
    void          *s;          /* must be null-terminated */
    char           mode;
    register int   col, row;
    int            op;		/* PIX_SRC | PIX_DST (faster), or PIX_SRC */
{
    register int    x_home;
    register int    y_home;
    XFontStruct	   *x_font_info = (XFontStruct *)xv_get((Xv_opaque)pixfont,
                                                          FONT_INFO);

#ifdef OW_I18N
    x_home = x_font_info->min_bounds.lbearing;
    y_home = -chrbase;
#else
    x_home =  x_font_info->per_char ?
              x_font_info->per_char[TTYSW_HOME_CHAR -
                         x_font_info->min_char_or_byte2].lbearing
             : x_font_info->min_bounds.lbearing;
    y_home = -x_font_info_ascent;
#endif

    /* this is needed for correct caret rendering */
    lxhome = x_home;

    /* possibly use escape sequences ? */

    if (xv_get(XV_SERVER_FROM_WINDOW(csr_pixwin), SERVER_JOURNALLING))
        if (!multibyte) {
            if (strchr(s, xv_shell_prompt[0]))
	        xv_set(XV_SERVER_FROM_WINDOW(csr_pixwin), 
                         SERVER_JOURNAL_SYNC_EVENT, 1, 0);
        } else {
            if (wschr(s, xv_shell_prompt[0]))
	        xv_set(XV_SERVER_FROM_WINDOW(csr_pixwin), 
                         SERVER_JOURNAL_SYNC_EVENT, 1, 0);
        }

    if (delaypainting) {
	if (row == ttysw_bottom)
	    /* bottom of screen so end delaypainting. */
	    (void) ttysw_pdisplayscreen(1);
	return;
    }

    if (s == NULL)
	return;

    ttysw_fixup_display_mode(&mode);

    if (!multibyte) {
        renderString(s, strlen(s), strlen(s),
                    row, col, x_home, y_home, op, mode);
    } else {

        wchar_t buf[1024];
        ttysw_convert_string(buf, s);
        renderString(buf, wslen(buf), wslen(s),
                    row, col, x_home, y_home, op, mode);
    }

}

Pkg_private void
ttysw_pclearline(fromcol, tocol, row)
    int             fromcol, tocol, row;
{
    int	klu1284	= (fromcol == 0 ? 1 : 0 ); 

    if (delaypainting)
	return;
    (void) tty_background(csr_pixwin, 
			  col_to_x(fromcol)-klu1284, row_to_y(row),
			  col_to_x(tocol) - col_to_x(fromcol)+klu1284,
			  chrheight, PIX_CLR);
}

Pkg_private void
ttysw_pcopyline(tocol, fromcol, count, row)
    int  fromcol, tocol, count, row;
{
    int  pix_width = (count * chrwidth);
    if (delaypainting)
	return;
    (void) tty_copyarea(csr_pixwin,
		     col_to_x(fromcol), row_to_y(row), pix_width, chrheight,
			col_to_x(tocol), row_to_y(row));
    tty_synccopyarea(csr_pixwin);
}

Pkg_private void
ttysw_pclearscreen(fromrow, torow)
    int             fromrow, torow;
{
    if (delaypainting)
	return;
    (void) tty_background(csr_pixwin, col_to_x(ttysw_left)-1,
			  row_to_y(fromrow),
			  winwidthp+1, row_to_y(torow - fromrow), PIX_CLR);
}

Pkg_private void
ttysw_pcopyscreen(fromrow, torow, count)
    int             fromrow, torow, count;
{
    if (delaypainting)
	return;
    (void) tty_copyarea(csr_pixwin,
	col_to_x(ttysw_left)-1, row_to_y(fromrow), winwidthp+1, row_to_y(count),
        col_to_x(ttysw_left)-1, row_to_y(torow));
    tty_synccopyarea(csr_pixwin);
}

static void
ttysw_displayrow(row, leftcol)
    register int row, leftcol;
{
    register int	colstart, blanks, colfirst;
    register char	modefirst;
    register char  *modestart;

    colfirst = 0;
    colstart = leftcol;

    if (leftcol < linelength[row]) {
	modefirst = MODE_CLEAR;
	blanks = 1;

        if (!multibyte) {

            register char *strstart, *strfirst;

            strfirst = NULL;
	    for (strstart = ((char **)image)[row] + leftcol,
                 modestart = screenmode[row] + leftcol; *strstart;
	         strstart++, modestart++, colstart++) {

	        /* Find beginning of bold string */
	        if ((*modestart == modefirst)  &&
                    ((!blanks) || (*strstart == ' ')))               
		    continue;
            
                if (strfirst != NULL) {
		    char csave = *strstart;
		    *strstart = '\0';
		    (void) ttysw_pstring(strfirst, modefirst,
				       colfirst, row, PIX_SRC);
		    *strstart = csave;
	        }
	        colfirst = colstart;
	        strfirst = strstart;
	        modefirst = *modestart;
                blanks = 0;
	    }
            if (strfirst != NULL)
	        (void) ttysw_pstring(strfirst,modefirst,colfirst,row,PIX_SRC);

        } else {

            register wchar_t *strstart, *strfirst;

            strfirst = NULL;
	    for (strstart = ((wchar_t **)image)[row] + leftcol,
                 modestart = screenmode[row] + leftcol; *strstart;
	         strstart++, modestart++, colstart++) {

	        /* Find beginning of bold string */
	        if ((*modestart == modefirst)  &&
                    ((!blanks) ||(*strstart == ' ')))               
		    continue;

                if (strfirst != NULL) {
		    wchar_t csave = *strstart;
		    *strstart = (wchar_t)'\0';
		    (void) ttysw_pstring(strfirst, modefirst,
				       colfirst, row, PIX_SRC);
		    *strstart = csave;
	        }
	        colfirst = colstart;
	        strfirst = strstart;
	        modefirst = *modestart;
                blanks = 0;
	    }
            if (strfirst != NULL)
	    (void) ttysw_pstring(strfirst,modefirst,colfirst,row,PIX_SRC);
        }

    }
}

Pkg_private void
ttysw_pdisplayscreen(dontrestorecursor)
    int             dontrestorecursor;
{
    struct rect *rect;
    int row;

    delaypainting = 0;
    /* refresh the entire image. */
    rect = (struct rect *) xv_get(csr_pixwin, WIN_RECT);
    (void) tty_background(csr_pixwin, 0, 0,
			      rect->r_width+1, rect->r_height, PIX_CLR);

    for (row = ttysw_top; row < ttysw_bottom; row++)
		ttysw_displayrow(row, 0);

    if (!dontrestorecursor)
	/* The following has effect of restoring cursor. */
	(void) ttysw_removeCursor();
}

/* ARGSUSED */
Pkg_private void
ttysw_prepair(eventp)
	XEvent		*eventp;
{
	Tty_exposed_lines *exposed;
	Ttysw_folio	ttysw = TTY_PRIVATE_FROM_ANY_VIEW(csr_pixwin);
	register int	row;
	int		leftcol;
	int		display_cursor = FALSE;

	/* 
	 * Handles expose and graphics expose events for the ttysw.   
	 */

	/* Get the expose events, ignore textsw caret checking with -10000 */
	exposed = tty_calc_exposed_lines(csr_pixwin, eventp, -10000);

	leftcol = x_to_col(exposed->leftmost);

	/* 
	 * Check damage on for cursor:
	 * When the cursor is light, it actually appears
	 * in the lines above and below the cursrow
	 * so these lines have to be checked too.
	 */
	if(leftcol <= curscol+1) {
	  /* 
	   * cheak and use one column to right and left of curscol because 
	   * unhilighted cursor actually overlaps into adjoining columns.
	   * Need to repaint these characters so they look right after 
	   * erasing with ttysw_paintCursor().
	   */
	  leftcol = MIN(leftcol, curscol);
	  leftcol = MAX(leftcol, 0);
	  if((exposed->line_exposed[cursrow]) ||
	     ((cursor & LIGHTCURSOR) && exposed->line_exposed[cursrow+1] ||
	      (cursrow > 0 && exposed->line_exposed[cursrow-1]))) {

			ttysw_paintCursor(PIX_CLR);
			exposed->line_exposed[cursrow] = TRUE;
			display_cursor = TRUE;
	  }
	}

	if(ttysw->ttysw_primary.sel_made && !ttysw->ttysw_primary.sel_null) {
		/* 
		 * In this case, there is a primary selection when
		 * an expose event happened.  To be most efficient and
		 * visually appealing, we might want to only repaint
		 * damaged areas.  But because the primary selection
		 * is highlighted with exclusive-or, this has to be
		 * done very cleverly.  
		 * 
		 * The secondary selection is painted over the text
		 * not using xor so it appears correctly regardless.
		 * This selection might be repainted here if it becomes an 
		 * issue, but the thinking is that simultaneous selections 
		 * and exposures are relatively rare.
		 */
		struct textselpos *begin, *end;
		int	selected_lines_damaged = FALSE;
		Xv_private void ttysortextents();

		ttysortextents(&ttysw->ttysw_primary, &begin, &end);

		for (row = begin->tsp_row; row <= end->tsp_row; row++)
			if(exposed->line_exposed[row]) {
				/* there was damage to the selected areas. */
				selected_lines_damaged = TRUE;
				break;
			}

		for (row = ttysw_top; row < ttysw_bottom; row++) {
		    if((selected_lines_damaged &&
		        (row >= begin->tsp_row) && (row <= end->tsp_row)) ||
			   (row == cursrow)) {
		        /* because of xor, the line is cleared */
                        int sl;
                        if (!multibyte)
                            sl = strlen(((char **)image)[row]) + 1;
                        else
                            sl = wslen(((wchar_t **)image)[row]) + 1;
			(void) ttysw_pclearline(0, sl, row);
			ttysw_displayrow(row, 0);
		    } else
			if(exposed->line_exposed[row])
				ttysw_displayrow(row, leftcol);
		}

		if(selected_lines_damaged)
			ttyhiliteselection(&ttysw->ttysw_primary,
                                                SELN_PRIMARY);
		/* secondary selection is painted in caller */


	} else {

		/* The easy case: no selection, just repaint damaged lines. */

		for (row = ttysw_top; row < ttysw_bottom; row++) {
		    if(exposed->line_exposed[row])
				ttysw_displayrow(row, leftcol);
		}
	}

	if(display_cursor)
		(void) ttysw_removeCursor();

	tty_clear_clip_rectangles(csr_pixwin);

}

Pkg_private void
ttysw_drawCursor(yChar, xChar)
{
    charcursx = xChar;
    charcursy = yChar;
    caretx = col_to_x(xChar);
    carety = row_to_y(yChar);
    curs_width = chrwidth;

    if (delaypainting || cursor == NOCURSOR)
	return;

    if (multibyte) {
        int         offset;   
        if( xChar >= ttysw_right ) xChar = ttysw_right-1;
        if( xChar < ttysw_left ) xChar = ttysw_left;
        if( yChar >= ttysw_bottom ) yChar = ttysw_bottom-1;
        if( yChar < ttysw_top ) yChar = ttysw_top;
        tty_column_wchar_type( xChar , yChar , &curs_width , &offset );
        curs_width *= chrwidth;
        caretx -= offset*chrwidth;
    }

    (void) tty_background(csr_pixwin,
             caretx-lxhome, carety, curs_width, chrheight, PIX_NOT(PIX_DST));

    if (cursor & LIGHTCURSOR) {
	(void) tty_background(csr_pixwin,
			      caretx - lxhome - 1, carety - 1, curs_width + 2, 
			      chrheight + 2, PIX_NOT(PIX_DST));
	(void) ttysw_pos(xChar, yChar);
    }

#ifdef OW_I18N
    {
        Ttysw_folio	ttysw_folio = TTY_PRIVATE_FROM_ANY_VIEW(csr_pixwin);
        XPoint		loc;
        XVaNestedList	va_nested_list;
    
        /*
         * If preedit style is overTheSpot, update the XNSpotLocation for
         * the preedit region. 
         */
        if (ttysw_folio->ic && (ttysw_folio->xim_style & XIMPreeditPosition)){  
            loc.x = (short)caretx;
            loc.y = (short)(carety + chrbase);
            va_nested_list = XVaCreateNestedList(NULL, 
					     XNSpotLocation, &loc, 
					     NULL);
            XSetICValues(ttysw_folio->ic, XNPreeditAttributes, va_nested_list,
        	     NULL);
            XFree(va_nested_list);
        }
    }
#endif	
    
}

static void
ttysw_paintCursor(op)
	int op;
{
	int y;
	int height;
	/* Erase or xor all bits used in light and normal cursor. */
	y = carety-1;
	height = chrheight + 2;
	if(y<0) {
		/* work around xnews server bug. */
		y=0;
		height--;
	}	
        (void) tty_background(csr_pixwin,
                              caretx - lxhome - 1, y, curs_width + 2, height, 
			      op);
}

Pkg_private void
ttysw_removeCursor()
{
    if (delaypainting || cursor == NOCURSOR)
	return;
    (void) tty_background(csr_pixwin,
             caretx-lxhome, carety, curs_width, chrheight, PIX_NOT(PIX_DST));
    if (cursor & LIGHTCURSOR)
	ttysw_paintCursor(PIX_NOT(PIX_DST));
}

Pkg_private void
ttysw_restoreCursor()	/* BUG ALERT: unnecessary routine */
{
    (void) ttysw_removeCursor();
}

Pkg_private void
ttysw_screencomp()	/* BUG ALERT: unnecessary routine */
{
}

Pkg_private void
ttysw_blinkscreen()
{
    struct timeval  now;
    static struct timeval lastblink;

    (void) gettimeofday(&now, NULL);
    if (now.tv_sec - lastblink.tv_sec > 1) {
	Xv_object       window = (Xv_object) csr_pixwin;
	(void) win_bell(window, ttysw_bell_tv, csr_pixwin);
	lastblink = now;
    }
}

Pkg_private void
ttysw_pselectionhilite(r, sel_rank)
    struct rect    *r;
    Seln_rank       sel_rank;
{
    struct rect     rectlock;

    rectlock = *r;
    rect_marginadjust(&rectlock, 1);
    if (sel_rank == SELN_PRIMARY)
	(void) tty_background(csr_pixwin, r->r_left, r->r_top,
			      r->r_width, r->r_height, PIX_NOT(PIX_DST));
    else
	(void) xv_replrop(csr_pixwin,
			  r->r_left, r->r_top,
			  r->r_width, r->r_height,
			  PIX_SRC | PIX_DST, &ttysw_gray17_pr, 0, 0);
}

/*
 *      Tty-subwindow stores screen image in a global void **image.
 *      For a character which has larger size than ascii characters,
 *      image data array is padded with TTY_NON_WCHAR so that
 *      image data array and the actual screen get coincident.
 *      This function converts image arrary to a normal wchar array
 *      by eliminating TTY_NON_WCHAR.
 */
static void
ttysw_convert_string( str , ttystr )
    wchar_t        *str;
    wchar_t        *ttystr;
{
    register wchar_t *strtmp = str;
    register wchar_t *ttystrtmp  = ttystr;

    while( *ttystrtmp ) {
        if( *ttystrtmp != TTY_NON_WCHAR )
                *strtmp++ = *ttystrtmp++;
        else
                ttystrtmp++;
    }
    *strtmp = (wchar_t)'\0';
}


Pkg_private int
tty_character_size(c)
wchar_t c;
{
    static wchar_t  str[2] = {(wchar_t)'\0',(wchar_t)'\0'};
    if( c == (wchar_t)'\0' ) return 1;
    str[0] = c;
    return( wscol(str) );
}
