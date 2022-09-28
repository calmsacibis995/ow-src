#ifndef lint
#ifdef sccs
static char     sccsid[] = "@(#)ttytl.c 20.43 93/12/10";
#endif
#endif

/*
 *	(c) Copyright 1989 Sun Microsystems, Inc. Sun design patents 
 *	pending in the U.S. and foreign countries. See LEGAL NOTICE 
 *	file for terms of the license.
 */

/*
 * Flavor of ttysw that knows about tool windows and allows tty based
 * programs to set/get data about the tool window (common routines).
 */

#include <stdio.h>
#ifdef SVR4
#include <sys/types.h>
#endif
#include <sys/file.h>
#include <sys/signal.h>
#include <xview/xview.h>
#include <xview/icon.h>
#include <xview/icon_load.h>
#include <xview/wmgr.h>
#include <xview/frame.h>
#include <xview/ttysw.h>
#include <xview/font.h>
#include <xview_private/tty_impl.h>
#include <xview_private/term_impl.h>
#include <xview_private/charscreen.h>

extern char    *strncpy();


/* BUG ALERT: This entire procedure should be rewritten! */
/* BUG ALERT: No XView prefix */
Pkg_private int
ttytlsw_escape(ttysw_view_public, c, ac, av)
    Tty_view        ttysw_view_public;
    char            c;
    register int    ac, *av;
{
    Tty             ttysw_public;
    Xv_object       frame_public;
    char            buf[150];
    char           *p, *text;
    struct rect     rect, orect;
    Ttysw_folio     ttysw, ttytlsw;
    Xv_Font         font;

    ttytlsw = ttysw = TTY_PRIVATE_FROM_ANY_PUBLIC(ttysw_view_public);
    ttysw_public = TTY_PUBLIC(ttysw);
    if (c != 't')
	return (ttysw_ansi_escape(ttysw_view_public, (wchar_t)c, ac, av));
    frame_public = xv_get(ttysw_public, WIN_FRAME);
    switch (av[0]) {
      case 1:			/* open */
	xv_set(frame_public, FRAME_CLOSED, FALSE, 0);
	break;
      case 2:			/* close */
	xv_set(frame_public, FRAME_CLOSED, TRUE, 0);
	break;
      case 3:			/* move */
	/*
	 * no more interactive moves if (ac == 1 && !xv_get(frame_public,
	 * FRAME_CLOSED)) { wmgr_move(frame_public); break; }
	 */
	(void) win_getrect(frame_public, &rect);
	orect = rect;
	if (av[1] < 0)
	    av[1] = rect.r_top;
	if (ac < 3 || av[2] < 0)
	    av[2] = rect.r_left;
	rect.r_top = av[1];
	rect.r_left = av[2];
	if (!xv_get(frame_public, FRAME_CLOSED))
	    wmgr_completechangerect(frame_public, &rect, &orect,
				    0, 0);
	else
	    (void) win_setrect(frame_public, &rect);
	break;
      case 4:			/* stretch */
	/*
	 * no more interactive stretches if (ac == 1 && !xv_get(frame_public,
	 * FRAME_CLOSED)) { wmgr_stretch(frame_public); break; }
	 */
	(void) win_getrect(frame_public, &rect);
	orect = rect;
	if (av[1] < 0)
	    av[1] = rect.r_height;
	if (ac < 3 || av[2] < 0)
	    av[2] = rect.r_width;
	rect.r_width = av[2];
	rect.r_height = av[1];
	if (!xv_get(frame_public, FRAME_CLOSED))
	    wmgr_completechangerect(frame_public, &rect, &orect,
				    0, 0);
	else
	    (void) win_setrect(frame_public, &rect);
	break;
      case 5:			/* top */
	wmgr_top(frame_public);
	break;
      case 6:			/* bottom */
	wmgr_bottom(frame_public);
	break;
      case 7:			/* refresh */
	wmgr_refreshwindow(frame_public);
	break;
      case 8:			/* stretch, size in chars */
	/*
	 * no more itneractive stretches if (ac == 1 && !xv_get(frame_public,
	 * FRAME_CLOSED)) { wmgr_stretch(frame_public); break; }
	 */
	(void) win_getrect(frame_public, &rect);
	orect = rect;
	if (av[1] <= 0)
	    av[1] = (int) xv_get(frame_public, WIN_ROWS);
	if (ac < 3 || av[2] <= 0)
	    av[2] = (int) xv_get(frame_public, WIN_COLUMNS);

        font = (Xv_Font) xv_get(ttysw_public, WIN_FONT);
	/* width is av[2] times width of one character plus width of borders
	   plus width of scrollbar plus margin width, if any plus a little
	   bit so that when characters are
	   rendered there is a rounding up to the next character so that
	   all characters will fit. */
#ifdef OW_I18N
	/* FONT_COLUMN_WIDTH is ifdef'ed OW_I18N */
	if (multibyte) {
	    rect.r_width = ((av[2]+1) * (int)xv_get(font,FONT_COLUMN_WIDTH))
		+ (2 * FRAME_BORDER_WIDTH) - 2;
	} else {
	    rect.r_width = ((av[2]+1) * (int) xv_get(font,
						     FONT_DEFAULT_CHAR_WIDTH))
		+ (2 * FRAME_BORDER_WIDTH) - 2;
	}
#else
        rect.r_width = ((av[2]+1) * (int) xv_get(font, 
						 FONT_DEFAULT_CHAR_WIDTH))
	    + (2 * FRAME_BORDER_WIDTH) - 2;
#endif /* OW_I18N */
	if (TTY_IS_TERMSW(ttysw)) {
	    Textsw_view textsw_view;
	    textsw_view = ttysw->current_view_public; /* Textsw really need the
						       * public view handle */
	    if (textsw_view) {
		int before=rect.r_width;
		Xv_opaque scrollbar;
		scrollbar = xv_get(textsw_view, WIN_VERTICAL_SCROLLBAR);
		if (scrollbar) {
		    rect.r_width += (int)xv_get(scrollbar,XV_WIDTH);
		}
		rect.r_width += (int)xv_get(textsw_view,XV_LEFT_MARGIN);
		rect.r_width += (int)xv_get(textsw_view,XV_RIGHT_MARGIN);
	    }
	}
        rect.r_height = ((av[1]+1) * (int) xv_get(font, 
						  FONT_DEFAULT_CHAR_HEIGHT))
                                + FRAME_BORDER_WIDTH - 2;
	if (!xv_get(frame_public, FRAME_CLOSED))
	    wmgr_completechangerect(frame_public, &rect, &orect,
				    0, 0);
	else
	    (void) win_setrect(frame_public, &rect);
	break;
      case 11:			/* report open or iconic */
	if (!xv_get(frame_public, FRAME_CLOSED))
	    p = "\33[1t";
	else
	    p = "\33[2t";
	(void) ttysw_input_it(ttysw, p, 4);
	break;
      case 13:			/* report position */
	(void) win_getrect(frame_public, &rect);
	(void) sprintf(buf, "\33[3;%d;%dt", rect.r_top, rect.r_left);
	(void) ttysw_input_it(ttysw, buf, strlen(buf));
	break;
      case 14:			/* report size */
	(void) win_getrect(frame_public, &rect);
	(void) sprintf(buf, "\33[4;%d;%dt", rect.r_height, rect.r_width);
	(void) ttysw_input_it(ttysw, buf, strlen(buf));
	break;
      case 18:			/* report size in chars */
	{
		int rows, columns;
		if (ttysw_getopt((caddr_t) ttysw, TTYOPT_TEXT)) {
			rows = textsw_screen_line_count(TTY_PUBLIC(ttysw));
			columns = textsw_screen_column_count(TTY_PUBLIC(ttysw));
		}
		else {
			rows = y_to_row(winheightp);
			columns = x_to_col(winwidthp);
		}
		(void) sprintf(buf, "\33[8;%d;%dt", rows, columns);
	}
	(void) ttysw_input_it(ttysw, buf, strlen(buf));
	break;
      case 20:{		/* report icon label */
	    Icon            icon = (Icon) xv_get(xv_get(TTY_PUBLIC(ttytlsw),
							WIN_FRAME),
						 FRAME_ICON);

	    (void) ttysw_input_it(ttysw, "\33]L", 3);
	    if (0 == (text = (char *) xv_get(icon, ICON_LABEL))) {
		text = (char *) xv_get(frame_public, FRAME_LABEL);
	    }
	    if (text)
		(void) ttysw_input_it(ttysw, text, strlen(text));
	    (void) ttysw_input_it(ttysw, "\33\\", 2);
	    break;
	}
      case 21:			/* report name stripe */
	(void) ttysw_input_it(ttysw, "\33]l", 3);
	if (text = (char *) xv_get(frame_public, FRAME_LABEL))
	    (void) ttysw_input_it(ttysw, text, strlen(text));
	(void) ttysw_input_it(ttysw, "\33\\", 2);
	break;
      default:
	return (TTY_OK);
    }
    return (TTY_DONE);
}

/* BUG ALERT: No XView prefix */
Pkg_private int
ttytlsw_string(ttysw_public, type, c)
    Tty             ttysw_public;
    unsigned int    type, c;
{
    Ttysw_folio     ttysw = TTY_PRIVATE_FROM_ANY_PUBLIC(ttysw_public);
    Ttysw          *ttytlsw = ttysw;
    char            iconlabel[33];
    int             cont;

    if (type != ']')
	return (ttysw_ansi_string(ttysw_public, type, c));
    switch (ttytlsw->hdrstate) {
      case HS_BEGIN:
	switch (c) {
	  case 'l':
	    ttytlsw->nameptr = ttytlsw->namebuf;
	    ttytlsw->hdrstate = HS_HEADER;
	    break;
	  case 'I':
	    ttytlsw->nameptr = ttytlsw->namebuf;
	    ttytlsw->hdrstate = HS_ICONFILE;
	    break;
	  case 'L':
	    ttytlsw->nameptr = ttytlsw->namebuf;
	    ttytlsw->hdrstate = HS_ICON;
	    break;
	  case '\0':
	    break;
	  default:
	    ttytlsw->hdrstate = HS_FLUSH;
	    break;
	}
	break;
      case HS_HEADER:
      case HS_ICONFILE:
      case HS_ICON:
        cont = TRUE;

        if (multibyte) {
           if (iswprint(c)) {
	      if (ttytlsw->nameptr <
		&ttytlsw->namebuf[sizeof(ttytlsw->namebuf) - (MB_CUR_MAX+1)])
                  ttytlsw->nameptr += wctomb(ttytlsw->nameptr, c);
              cont = FALSE;
           }
        } else {              
	    if ((c >= ' ' && c <= '~') || ((c & 0x80) != 0) ) {
	        if (ttytlsw->nameptr <
		    &ttytlsw->namebuf[sizeof(ttytlsw->namebuf) - 1]) 
		    *ttytlsw->nameptr++ = c;
                cont = FALSE;
            }
        }

        if ((cont)  && (c == '\0')) {
	    *ttytlsw->nameptr = '\0';
	    switch (ttytlsw->hdrstate) {
	      case HS_HEADER: {
                    char namestripe[150];
		    (void) strncpy(namestripe, ttytlsw->namebuf,
			       sizeof(namestripe));
		    (void) xv_set(xv_get(TTY_PUBLIC(ttysw), WIN_FRAME),
			      FRAME_LABEL, namestripe, 0);
                }
		break;
	      case HS_ICONFILE:{
		    char            err[IL_ERRORMSG_SIZE];
		    struct pixrect *mpr;
		    Icon            icon;
		    if ((mpr = icon_load_mpr(ttytlsw->namebuf, err)) ==
			(struct pixrect *) 0) {
			xv_error((Xv_opaque)mpr,
				 ERROR_STRING, err,
				 ERROR_PKG, TTY,
				 0);
		    } else {
			Frame	frame = xv_get(TTY_PUBLIC(ttysw), WIN_FRAME);
			Icon	current_icon = xv_get(frame, FRAME_ICON);

			if (current_icon)  {
			    xv_set(current_icon, ICON_IMAGE, mpr, NULL);
			}
			else  {
			    icon = icon_create( ICON_IMAGE, mpr, 0 );
			    (void) xv_set(xv_get(TTY_PUBLIC(ttysw), WIN_FRAME),
				          FRAME_ICON, icon, 0);
			}
		    }

		    break;
		}
	      case HS_ICON: {
		Frame		frame = 
				xv_get(TTY_PUBLIC(ttysw), WIN_FRAME);
                (void) strncpy(iconlabel, ttytlsw->namebuf,
                               sizeof(iconlabel));
                (void) xv_set(xv_get(frame, FRAME_ICON),
                              ICON_LABEL, iconlabel, 0);
		}
		break;
	      default:{
		}
	    }
	    ttytlsw->hdrstate = HS_BEGIN;
	}
	break;
      case HS_FLUSH:
	if (c == '\0')
	    ttytlsw->hdrstate = HS_BEGIN;
	break;
      default:
	return (ttysw_ansi_string(ttysw_public, type, c));
    }
    return (TTY_DONE);
}
