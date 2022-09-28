#ifndef lint
#ifdef sccs
static char     sccsid[] = "@(#)cim_change.c 20.22 94/02/23";
#endif
#endif

/*
 *	(c) Copyright 1989-1993 Sun Microsystems, Inc. Sun design patents
 *	pending in the U.S. and foreign countries. See LEGAL_NOTICE
 *	file for terms of the license.
 */

/*
 * Character image manipulation (except size change) routines.
 */

#include <xview_private/i18n_impl.h>
#include <sys/types.h>
#include <pixrect/pixrect.h>
#include <xview_private/ttyansi.h>
#include <xview_private/charimage.h>
#include <xview_private/charscreen.h>
#include <xview_private/tty_impl.h>

char            boldify;

extern char    *strcpy();

/* static */ void ttysw_roll();
static void reverse();
/* static */ void ttysw_swap();

#define JF

static void
setLineLength(int row, int len)
{
    if (len > ttysw_right)
        linelength[row] = ttysw_right;
    else
        linelength[row] = len;
    if (!multibyte)
        ((char **)image)[row][linelength[row]] = NULL;
    else
        ((wchar_t **)image)[row][linelength[row]] = NULL;
}

Pkg_private void
ttysw_vpos(row, col)
    int             row, col;
{        
    register char  *bold = screenmode[row];

    while (linelength[row] <= col) {
        bold[linelength[row]] = MODE_CLEAR;
        if (!multibyte)
            ((char **)image)[row][linelength[row]] = ' ';
        else
            ((wchar_t **)image)[row][linelength[row]] = (wchar_t) ' ';
	linelength[row]++;
        }
    setLineLength(row, linelength[row]);
}

Pkg_private void
ttysw_bold_mode()
{
    boldify |= MODE_BOLD;
}

/* NOT USED */
ttysw_nobold_mode()
{
    boldify &= ~MODE_BOLD;
}

Pkg_private void
ttysw_underscore_mode()
{
    boldify |= MODE_UNDERSCORE;
}

/* NOT USED */
ttysw_nounderscore_mode()
{
    boldify &= ~MODE_UNDERSCORE;
}

Pkg_private void
ttysw_inverse_mode()
{
    boldify |= MODE_INVERT;
}

/* NOT USED */
ttysw_noinverse_mode()
{
    boldify &= ~MODE_INVERT;
}

Pkg_private void
ttysw_clear_mode()
{
    boldify = MODE_CLEAR;
}

Pkg_private void
ttysw_writePartialLine(s, curscolStart)
    void        *s;
    register int curscolStart;
{
    register char  *bold = screenmode[cursrow];
    register int    curscolTmp = curscolStart;
    int    c_sizefactor;

    if (linelength[cursrow] < curscolStart)
	(void) ttysw_vpos(cursrow, curscolStart);
    /*
     * Stick characters in line.
     */
    if (!multibyte) {
        register char *sTmp;
        for (sTmp = s; *sTmp != '\0'; sTmp++) {
	    ((char **)image)[cursrow][curscolTmp] = *sTmp;
	    bold[curscolTmp] = boldify;
	    curscolTmp++;
        }
    } else {

        register wchar_t *sTmp;
        if ((*(wchar_t *)s != '\0') &&
             (((wchar_t **)image)[cursrow][curscolTmp] == TTY_NON_WCHAR)) { 
            ((wchar_t **)image)[cursrow][curscolTmp-1] = ' ';
            bold[curscolTmp-1] = boldify;
        }

        for (sTmp = s; *sTmp != '\0'; sTmp++) {
	    ((wchar_t **)image)[cursrow][curscolTmp] = *sTmp;
	    bold[curscolTmp] = boldify;
            c_sizefactor = tty_character_size( *sTmp );
            while( --c_sizefactor > 0 ) {
                curscolTmp++;
                ((wchar_t **)image)[cursrow][curscolTmp] = TTY_NON_WCHAR;
                bold[curscolTmp] = boldify;
            }
	    curscolTmp++;
        }

        if ((linelength[cursrow]  >= curscolTmp) &&
              (((wchar_t **)image)[cursrow][curscolTmp] == TTY_NON_WCHAR)) {
            ((wchar_t **)image)[cursrow][curscolTmp] = ' ';
            bold[curscolTmp] = boldify;
            curscolTmp++;
        }

    }
    if (linelength[cursrow] < curscolTmp)
	setLineLength(cursrow, curscolTmp);
    (void) ttysw_pstring(s, boldify, curscolStart, cursrow, PIX_SRC);
}

#ifdef JF
Pkg_private void
ttysw_cim_scroll(n)
    register int    n;
{
    register int    new;

#ifdef DEBUG_LINES
    printf(" ttysw_cim_scroll(%d)	\n", n);
#endif
    if (n > 0) {		/* text moves UP screen	 */
	(void) delete_lines(ttysw_top, n);
    } else {			/* (n<0)	text moves DOWN	screen	 */
	new = ttysw_bottom + n;
	(void) ttysw_roll(ttysw_top, new, ttysw_bottom);
	(void) ttysw_pcopyscreen(ttysw_top, ttysw_top - n, new);
	(void) ttysw_cim_clear(ttysw_top, ttysw_top - n);
    }
}

#else
Pkg_private void
ttysw_cim_scroll(toy, fromy)
    int             fromy, toy;
{

    if (toy < fromy)		/* scrolling up */
	(void) ttysw_roll(toy, ttysw_bottom, fromy);
    else
	ttysw_swapregions(fromy, toy, ttysw_bottom - toy);
    if (fromy > toy) {
	(void) ttysw_pcopyscreen(fromy, toy, ttysw_bottom - fromy);
	(void) ttysw_cim_clear(ttysw_bottom - (fromy - toy), ttysw_bottom);
	/* move text up */
    } else {
	(void) ttysw_pcopyscreen(fromy, toy, ttysw_bottom - toy);
	(void) ttysw_cim_clear(fromy, ttysw_bottom - (toy - fromy));	/* down */
    }
}

#endif

Pkg_private void
ttysw_insert_lines(where, n)
    register int    where, n;
{
    register int    new = where + n;

#ifdef DEBUG_LINES
    printf(" ttysw_insert_lines(%d,%d) ttysw_bottom=%d	\n", where, n, ttysw_bottom);
#endif
    if (new > ttysw_bottom)
	new = ttysw_bottom;
    (void) ttysw_roll(where, new, ttysw_bottom);
    (void) ttysw_pcopyscreen(where, new, ttysw_bottom - new);
    (void) ttysw_cim_clear(where, new);
}

/* BUG ALERT:  Externally visible procedure without a valid XView prefix. */
Pkg_private void
delete_lines(where, n)
    register int    where, n;
{
    register int    new = where + n;

#ifdef DEBUG_LINES
    printf(" delete_lines(%d,%d)	\n", where, n);
#endif
    if (new > ttysw_bottom) {
	n -= new - ttysw_bottom;
	new = ttysw_bottom;
    }
    (void) ttysw_roll(where, ttysw_bottom - n, ttysw_bottom);
    (void) ttysw_pcopyscreen(new, where, ttysw_bottom - new);
    (void) ttysw_cim_clear(ttysw_bottom - n, ttysw_bottom);
}

/* static */ void
ttysw_roll(first, mid, last)
    int             first, last, mid;
{

    /* printf("first=%d, mid=%d, last=%d\n", first, mid, last); */
    reverse(first, last);
    reverse(first, mid);
    reverse(mid, last);
}

static void
reverse(a, b)
    int             a, b;
{

    b--;
    while (a < b)
	(void) ttysw_swap(a++, b--);
}

/* static */ void
ttysw_swapregions(a, b, n)
    int             a, b, n;
{

    while (n--)
	(void) ttysw_swap(a++, b++);
}

/* static */ void
ttysw_swap(a, b)
    int             a, b;
{
    char    *tmpbold = screenmode[a];
    void    *tmpline = image[a];
    int     tmp = linelength[a];

    image[a] = image[b];
    image[b] = tmpline;
    screenmode[a] = screenmode[b];
    screenmode[b] = tmpbold;
    linelength[a] = linelength[b];
    linelength[b] = tmp;
}

Pkg_private void
ttysw_cim_clear(a, b)
    int             a, b;
{
    register int    i;

    for (i = a; i < b; i++)
	setLineLength(i, 0);
    (void) ttysw_pclearscreen(a, b);
    if (a == ttysw_top && b == ttysw_bottom) {
	if (delaypainting)
	    (void) ttysw_pdisplayscreen(1);
	else
	    delaypainting = 1;
    }
}

static void
adjustPosition(fromcol, tocol, row, len)
    int *fromcol, *tocol, row, len;
{
    if (multibyte) {
        if( ((wchar_t **)image)[row][*fromcol] == TTY_NON_WCHAR ) {
            while( *fromcol > 0 && 
                    ((wchar_t **)image)[row][*fromcol] == TTY_NON_WCHAR )
                (*fromcol)--;
        }

        if( ((wchar_t **)image)[row][*tocol] == TTY_NON_WCHAR ) {
            while( *tocol < len && 
                    ((wchar_t **)image)[row][*tocol] == TTY_NON_WCHAR )
                 (*tocol)++;
        }
    }
}

Pkg_private void
ttysw_deleteChar(fromcol, tocol, row)
    int             fromcol, tocol, row;
{
    char           *bold = screenmode[row];
    int             len = linelength[row];

    if (fromcol >= tocol)
	return;

    adjustPosition(&fromcol, &tocol, row, len);

    if (tocol < len) {
	/*
	 * There's a fragment left at the end
	 */
	int gap = tocol - fromcol;
	register char  *am = bold + fromcol;
        register char  *bm = bold + tocol;
        if (!multibyte) {
            register char *a = ((char **)image)[row] + fromcol;
            register char *b = ((char **)image)[row] + tocol;
	    while (*a++ = *b++)
                *am++ = *bm++;
        } else {
            register wchar_t *a = ((wchar_t **)image)[row] + fromcol;
            register wchar_t *b = ((wchar_t **)image)[row] + tocol;
	    while (*a++ = *b++)
                *am++ = *bm++;
        }
	setLineLength(row, len - gap);
	(void) ttysw_pcopyline(fromcol, tocol, len - tocol, row);
	(void) ttysw_pclearline(len - gap, len, row);
    } else if (fromcol < len) {
	setLineLength(row, fromcol);
	(void) ttysw_pclearline(fromcol, len, row);
    }
}

Pkg_private void
ttysw_insertChar(fromcol, tocol, row)
    int fromcol;
    int tocol;
    int row;
{
    register char  *bold = screenmode[row];
    int             len = linelength[row];
    register int    i;
    int             delta, newlen, slug, rightextent;

    if (fromcol >= tocol || fromcol >= len)
	return;

    delta = tocol - fromcol;
    newlen = len + delta;
    if (newlen > ttysw_right)
	newlen = ttysw_right;
    if (tocol > ttysw_right)
	tocol = ttysw_right;

    if (!multibyte) {
        for (i = newlen; i >= tocol; i--) {
	    ((char **)image)[row][i] = ((char **)image)[row][i - delta];
	    bold[i] = bold[i - delta];
        }
        for (i = fromcol; i < tocol; i++) {
	    ((char *)image[row])[i] = ' ';
	    bold[i] = MODE_CLEAR;
        }
    } else {
        for (i = newlen; i >= tocol; i--) {
	    ((wchar_t **)image)[row][i] = ((wchar_t **)image)[row][i - delta];
	    bold[i] = bold[i - delta];
        }
        for (i = fromcol; i < tocol; i++) {
	    ((wchar_t **)image)[row][i] = (wchar_t) ' ';
	    bold[i] = MODE_CLEAR;
        }
    }
    setLineLength(row, newlen);
    rightextent = len + (tocol - fromcol);
    slug = len - fromcol;
    if (rightextent > ttysw_right)
	slug -= rightextent - ttysw_right;
    (void) ttysw_pcopyline(tocol, fromcol, slug, row);
    (void) ttysw_pclearline(fromcol, tocol, row);
}

Pkg_private void
tty_column_wchar_type( xChar , yChar , cwidth , offset )
    int         xChar;
    int         yChar;
    int         *cwidth;        /* character width (RETURN) */
    int         *offset;        /* offset of charcter (RETURN) */
{
    wchar_t           *line = ((wchar_t **)image)[yChar];
    register wchar_t  c = line[xChar];

    *offset = 0;
    if( c == TTY_NON_WCHAR ) {
        while( c == TTY_NON_WCHAR ) {
                c = line[--xChar];
                (*offset) ++;
        }
    }    

    *cwidth = tty_character_size(c);

}

Pkg_private int
tty_get_nchars( colstart , colend , row )
    int                 colstart;
    register int        colend;
    int                 row;
{
    register    int     nchar = 0;
    int         i;

    if( colend == TTY_LINE_INF_INDEX )   /* up to end of line */
        colend = linelength[row] - 1 ;

    if (multibyte) {
        int i;
        register int nchar = 0;
        for( i = colstart; i<= colend ; i++ ) {
            if( ((wchar_t **)image)[row][i] == TTY_NON_WCHAR )
                continue;
        nchar++;
        }
        return nchar;
    }

    return(colend - colstart + 1);
}
