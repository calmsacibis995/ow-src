#ident  "@(#)tool_support.c 3.24 96/12/02 Copyr 1987 Sun Micro"

/*	Copyright (c) 1987, 1988, Sun Microsystems, Inc.  All Rights Reserved.
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

/*
 * Mailtool - tool support 
 */

#include <stdio.h>
#include <errno.h>
#include <signal.h>
#include <ctype.h>
#include <string.h>
#include <memory.h>
#ifdef undef
#ifdef __STDC__
#include <stdarg.h>
#else
#include <sys/varargs.h>
#endif "__STDC__"
#endif undef
#include <stdarg.h>

#include <sys/types.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/file.h>

#include <xview/cursor.h>
#include <xview/selection.h>
#include <xview/sel_svc.h>
#include <xview/sel_attrs.h>
#include <xview/panel.h>
#include <xview/text.h>
#include <xview/font.h>
#include <xview/notice.h>
#include <xview/svrimage.h>
#include <xview/fullscreen.h>
#include <xview/xview.h>
#include <xview/scrollbar.h>
#include <X11/X.h>

#include "glob.h"
#include "tool.h"
#include "tool_support.h"
#include "mail.h"
#include "main.h"
#include "create_panels.h"
#include "cmds.h"
#include "../maillib/ck_strings.h"
#include "../maillib/charset.h"
#include "../maillib/global.h"
#include "graphics.h"
/* #include "select.h" */
#include "header.h"
#include "attach.h"

#include "instrument.h"
#include "mle.h"

#define	DAMPING 7

Seln_client	header_client;
extern int	(*default_textsw_notify)();

/* this structure keeps track of a selection */
struct mt_select {
	char *ms_buffer;
	char *ms_ptr;
	int ms_size;
	int ms_haveselection;
};


/* this structure is used to follow a read of a selection across
 * multiple reads
 */
struct mt_context {
	int mc_inuse;
	char *mc_ptr;
	char *mc_end;
	void *mc_headerdata;
};

enum mt_ranks { MR_PRIMARY, MR_SECONDARY, MR_SHELF, MR_SIZE };

static	int	mt_selection_start;
static	int	mt_selection_end;
static	struct	mt_select mt_sel_buffers[MR_SIZE];
extern	long	dclick_usec;

Xv_Cursor	copyletter_cursor;
Xv_Cursor	copyletters_cursor;
Xv_Cursor	moveletter_cursor;
Xv_Cursor	moveletters_cursor;
int	mt_glyphheight;

/*
 * Cursors
 */



static unsigned short    moveletter_image[] = {
#include "moveletter.cursor"
};

static unsigned short    moveletters_image[] = {
#include "moveletters.cursor"
};

static unsigned short    copyletter_image[] = {
#include "copyletter.cursor"
};

static unsigned short    copyletters_image[] = {
#include "copyletters.cursor"
};


extern Pixrect letter_pr;
extern Pixrect letters_pr;

static void	alert_failed();
static void	stretch_box_selection();
static void	drag_headermsg();
static void	mt_repaint_rgn();
static int	position_header();
static int	position_headersw();

extern char *makeheader();


#define DEBUG_FLAG mt_debugging
extern DEBUG_FLAG;
#include "debug.h"

static int position_headersw(struct header_data *hd);
static void mt_vs_warn1(Frame frame, char *fmt, va_list args);
static void alert_failed(int type, char *array[]);
static void mt_draw_diamond(int x, int y, struct header_data *hd);
static void mt_draw_arrow(int x, int y, struct header_data *hd);
static void mt_set_region(short low, short high, short select,
	struct header_data *hd);
static void xor_rgn(struct header_data *hd, int start, int stop);
static void mt_repaint_rgn(int start, int stop, int line,
	struct header_data *hd);
static void select_rgn(short low, short high, struct header_data *hd);
static void stretch_box_selection(Canvas canvas, Event *event);
static struct mt_select *rank_to_ms(Seln_rank rank);
static void reset_selection(Seln_rank rank);


/*
 * Repaint the header window starting from a specified line.  This
 * routine does primitive clipping so that we only paint the area
 * visible in the canvas window.
 */
void
force_repaint_from_line(int line, int force_clear, struct header_data *hd)
{
	Rectlist	dummy_rlist;
	int		top;	/* First visible line */
	int		bottom; /* Last visible line */

	/* XXX dipol - Occasionally after a resize SCROLLBAR_VIEW_START
	 * is incorrect.  This can result in various repaint problems.
	 * See XView bug 1064600.
	 *
	 * A work around would be to simply paint from "line" until the
	 * end of the canvas, but for large mailboxes that could be
	 * fairly expensive.  Live with the bug for now.  We may have to
	 * hack around it if XView won't fix it.
	 */
	top = (int)xv_get(hd->hd_scrollbar, SCROLLBAR_VIEW_START);

	/* SCROLLBAR_VIEW_LENGTH also appears to be wrong sometimes
	bottom = top + (int)xv_get(hd->hd_scrollbar, SCROLLBAR_VIEW_LENGTH);
	*/
	bottom = top + ((int)xv_get(hd->hd_canvas, XV_HEIGHT) /
					hd->hd_lineheight);

	/* SCROLLBAR_VIEW_ attributes start at 0.  Our linenos start at 1 */
	top++;
	bottom++;

	DP(("force_repaint_from_line(line=%d, clear=%d) top=%d, bottom=%d\n",
		line, force_clear, top, bottom));

	if (line > bottom) {
		/* Don't paint if request is off bottom of visible area */
		return;
	} else {
		/* Don't paint lines above top of visible area */
		line = line < top ? top : line;
	}

	dummy_rlist.rl_bound.r_top = (line - 1) * hd->hd_lineheight + 1;
	dummy_rlist.rl_bound.r_left = 0;
	dummy_rlist.rl_bound.r_width = (int) xv_get(hd->hd_canvas, XV_WIDTH);
	dummy_rlist.rl_bound.r_height =
				(bottom - line + 1) * hd->hd_lineheight + 1;

	DP(("  Paint rect: top=%d left=%d w=%d h=%d (lineheight = %d)\n",
		dummy_rlist.rl_bound.r_top, dummy_rlist.rl_bound.r_left,
		dummy_rlist.rl_bound.r_width, dummy_rlist.rl_bound.r_height,
		hd->hd_lineheight));

	if (force_clear) {
		/* We add 1 to r_top so we don't clip the selection box */
		XFillRectangle(hd->hd_display, hd->hd_drawable, hd->hd_cleargc,
			0, dummy_rlist.rl_bound.r_top + 1,
			dummy_rlist.rl_bound.r_width,
			dummy_rlist.rl_bound.r_height);
	}

	mt_repaint_headers(hd->hd_canvas, (Pixwin *) hd->hd_paintwin,
		&dummy_rlist, FALSE);
}

void
force_repaint_on_line(int line, struct header_data *hd)
{
	Rectlist	dummy_rlist;

	/* +1 so we don't clip selection box of message above */
	dummy_rlist.rl_bound.r_top = (line - 1) * hd->hd_lineheight + 1;
	dummy_rlist.rl_bound.r_left = 0;
	dummy_rlist.rl_bound.r_width = (int) xv_get(hd->hd_canvas, WIN_WIDTH);
	dummy_rlist.rl_bound.r_height = hd->hd_lineheight;
	
	mt_repaint_headers(hd->hd_canvas, (Pixwin *)hd->hd_paintwin,
		&dummy_rlist, TRUE);
}


void
mt_shift_nlines(int line, int n, struct header_data *hd)
{
	int	to_y, from_y;
	int	w, h;

	line--;

	/*
	 * Shift the header list n lines start with line "line" (1 based).
	 * If n>0 we shift down (for insert), if n<0 we shift up (for delete)
	 * This routines helps painting *look* faster.  We first shift the
	 * list image n lines.  We then paint over that.  The shift 
	 * looks fast and the user doesn't notice the second paint.
	 */
	to_y = (line + n) * hd->hd_lineheight + 1;

	from_y = line * hd->hd_lineheight + 1;

	w = (int)xv_get(hd->hd_canvas, CANVAS_WIDTH);

	h = (int)xv_get(hd->hd_canvas, CANVAS_HEIGHT) - from_y;

	/*
	 * Move header list image n lines
	 */
	XCopyArea(hd->hd_display, hd->hd_drawable, hd->hd_drawable,
		hd->hd_gc, 0, from_y, w, h, 0, to_y);

	if (n < 0) {
		/*
		 * Lines moved up.  Clear region at bottom of display
		 */
		XFillRectangle(hd->hd_display, hd->hd_drawable, hd->hd_cleargc,
			0, to_y + h, w, -n * hd->hd_lineheight);
	} else {
		/*
		 * Lines moved down.  Clear area "opened up" 
		 */
		XFillRectangle(hd->hd_display, hd->hd_drawable, hd->hd_cleargc,
			0, from_y, w, n * hd->hd_lineheight);
	}

	/*
	 * Normally we would only need to repaint the region we cleared
	 * above.  But if the list is partially obscured the copy
	 * will leave holes.  We therefore repaint everything.
	 */
	if (line == 0)
		line++;
	force_repaint_from_line(line, FALSE, hd);
}

void
mt_build_cursors(void)

{
	int	x_hot;
	int	y_hot;
	Server_image	moveletter_pr;
	Server_image	moveletters_pr;
	Server_image	copyletter_pr;
	Server_image	copyletters_pr;


	x_hot = ICON_DEFAULT_WIDTH / 2 + 11;
	y_hot = ICON_DEFAULT_WIDTH / 2 + 1;

	moveletter_pr = (Server_image)xv_create(0, SERVER_IMAGE,
		SERVER_IMAGE_BITS,	moveletter_image,
		SERVER_IMAGE_DEPTH,	1,
		XV_WIDTH,		ICON_DEFAULT_WIDTH,
		XV_HEIGHT,		ICON_DEFAULT_HEIGHT,
		0);

	moveletter_cursor = xv_create(0, CURSOR,
				CURSOR_IMAGE,	moveletter_pr,
				CURSOR_XHOT,	x_hot,
				CURSOR_YHOT,	y_hot,
				CURSOR_OP,	PIX_SRC ^ PIX_DST,
				0);

	moveletters_pr = (Server_image)xv_create(0, SERVER_IMAGE,
		SERVER_IMAGE_BITS,      moveletters_image,
		SERVER_IMAGE_DEPTH,	1,
		XV_WIDTH,		ICON_DEFAULT_WIDTH,
		XV_HEIGHT,		ICON_DEFAULT_HEIGHT,
		0);

	moveletters_cursor = xv_create(0, CURSOR,
				CURSOR_IMAGE,	moveletters_pr,
				CURSOR_XHOT,	x_hot,
				CURSOR_YHOT,	y_hot,
				CURSOR_OP,	PIX_SRC ^ PIX_DST,
				0);

	copyletter_pr = (Server_image)xv_create(0, SERVER_IMAGE,
		SERVER_IMAGE_BITS,	copyletter_image,
		SERVER_IMAGE_DEPTH,	1,
		XV_WIDTH,		ICON_DEFAULT_WIDTH,
		XV_HEIGHT,              ICON_DEFAULT_HEIGHT,
		0);

	copyletter_cursor = xv_create(0, CURSOR,
				CURSOR_IMAGE,	copyletter_pr,
				CURSOR_XHOT,	x_hot,
				CURSOR_YHOT,	y_hot,
				CURSOR_OP,	PIX_SRC ^ PIX_DST,
				0);

	copyletters_pr = (Server_image)xv_create(0, SERVER_IMAGE,
		SERVER_IMAGE_BITS,	copyletters_image,
		SERVER_IMAGE_DEPTH,     1, 
		XV_WIDTH,               ICON_DEFAULT_WIDTH, 
		XV_HEIGHT,              ICON_DEFAULT_HEIGHT,  
		0);

	copyletters_cursor = xv_create(0, CURSOR,
				CURSOR_IMAGE,	copyletters_pr,
				CURSOR_XHOT,	x_hot,
				CURSOR_YHOT,	y_hot,
				CURSOR_OP,	PIX_SRC ^ PIX_DST,
				0);
}

/*
 * Load the headers into the header subwindow.
 */
void
mt_load_headers(struct header_data *hd)
{
	FILE           *f;
	register int    i, pos = 0;
	register struct msg *mp;
	int		lower_context, upper_context;
	int		new_start;

	for (mp = msg_methods.mm_first(hd->hd_folder); mp != NULL;
	     mp = msg_methods.mm_next(mp)) {
		mp->m_lineno = mp->mo_msg_number;
	}
	new_start = position_headersw(hd) - 1;

	DP(("mt_load_headers: setting VIEW_START to %d\n", new_start));

	scroll_set_view_len(hd, new_start, NULL);

	/* Set up the glyphs, and adjust the subwindow */
	mt_redo_glyphs(FIRST_NDMSG(CURRENT_FOLDER(hd)), hd);
}

/*
 * Repaint all headers after a specified message
 */
void
mt_append_headers(
	int make_visible,		/* TRUE to scroll to current msg */
	struct msg *msg,		/* Message to start repaint from */
	struct header_data *hd)
{
	register int    pos;
	register struct msg *mp;
	int		new_start;
	struct msg     *curmsg;

	curmsg = MT_CURMSG(hd) ? MT_CURMSG(hd) : msg_methods.mm_first(CURRENT_FOLDER(hd));
	if (! curmsg) return;

	if (make_visible) {
		int		top, target, nrows;

		top = (int)
			xv_get(hd->hd_canvas, CANVAS_HEIGHT)/hd->hd_lineheight +1;

		/* + 1 because TEXTSW_FIRST_LINE is zero based. */
		nrows = (int)
			xv_get(hd->hd_canvas, WIN_HEIGHT) / hd->hd_lineheight;

		target = curmsg->m_lineno + 1;

		/* the line number for the first new message */
		if (top <= target && target < top + nrows) {
		 	/* already visible. don't do anything */
			DP(("headersw correctly positioned:\n"));
			DP(("\ttop = %d,\n", top ));
			DP(("\ttarget = %d,\n", target));
			DP(("\tnrows = %d\n", nrows));
		} else {
			int		upper_context;
			pos = position_headersw(hd);
			DP(("positioning headersw:\n"));
			DP(("\ttop = %d,\n", top ));
			DP(("\ttarget = %d,\n", target));
			DP(("\tnrows = %d\n", nrows));
			new_start = position_headersw(hd) - 1;
		
			DP(("mt_append_headers: setting VIEW_START to %d\n",
				new_start));
			xv_set(hd->hd_scrollbar,
				SCROLLBAR_VIEW_START, new_start, 0);
		}
	}

	/* Paint the headers starting at the specified message */
	mt_redo_glyphs(msg, hd);

}

/*
 * used to make the current message be the top line in the window. fill out the
 * window from the top if too few remaining messages 
 */
static int
position_headersw(struct header_data *hd)
{
	int		height, start, nmsgs;
	struct msg	*curmsg;

	curmsg = MT_CURMSG(hd);
	if (curmsg == msg_methods.mm_first(hd->hd_folder)) {
		/* occurs when there is no new mail */
		return (1);
	}

	height = (int)xv_get(hd->hd_canvas, WIN_HEIGHT) / hd->hd_lineheight;

	nmsgs = hd->hd_folder ? hd->hd_folder->fo_num_msgs : 0;
	if (nmsgs <= height) {
		start = 1;
	} else if (nmsgs - curmsg->m_lineno >= height - 1) {
		/*
		 * there are more messages beyond curmsg than would fit in
		 * the window with one blank line at the bottom 
		 */

		/* position window with curmsg one line from top */
		start = curmsg->m_lineno - 1;
	} else {
		/*
		 * position window so that the last msg is at the bottom,
		 * followed by one blank line 
		 */
		start = nmsgs - height + 2;
	}
	return (start);
}






/*
 * Make sure the indicated message is visible. Scroll if it is not. 
 */
void
mt_update_headersw(struct msg *m, struct header_data *hd)
{

	int	top, height, target, new_start, canvas_height;

	if (m == NULL)
		return;

	top = (int) xv_get(hd->hd_scrollbar, SCROLLBAR_VIEW_START) + 1;

	height = (int) xv_get(hd->hd_canvas, WIN_HEIGHT) / hd->hd_lineheight;
	canvas_height = (int) xv_get(hd->hd_canvas, CANVAS_HEIGHT) /
						hd->hd_lineheight;
	target = m->m_lineno;

	if ((target < top) || (target > (top + height - 1)))
	{
		new_start = target - (height/2) - 1;

		/*
		 * Make sure we don't scroll off the top or bottom
		 */
		if (new_start < 0) {
			new_start = 0;
		} else if (new_start > canvas_height - height) {
			new_start = canvas_height - height;
		}
	
		xv_set(hd->hd_scrollbar, SCROLLBAR_VIEW_START, new_start, 0);

		DP(("mt_update_headersw: setting VIEW_START to %d\n",
			new_start));
	}
}

void
mt_save_curmsgs(struct header_data *hd)
{
	register struct	view_window_list	*base;

	if (CURRENT_FOLDER(hd) == NULL || CURRENT_FOLDER(hd)->fo_num_msgs == 0) {
		return;	/* no messages */
	}

	/*
	 * Loop through all currently displayed messages and make sure
	 * that changes have been saved.
	 */
	for (base = hd->hd_vwl; base != NULL; base = base->vwl_next) {
		if (xv_get(base->vwl_frame, XV_SHOW) &&
			(base->vwl_msg != NULL))
		{
			mt_save_msg(base);
		}
	}
}

/*
 * If the specified message 
 * has been modified, write it back out to the file
 * and tell mail to reload the message from the file.
 * If the user stores the message into a different
 * file then the filename for the textsw will change
 * so we have to check for that too. 
 */
void
mt_save_msg(struct view_window_list *vwl)
{
	struct stat     statb;
	struct header_data *hd;
	char            *s;
	int		nlines;
	int		length;
	char		*buffer;
	register struct msg *mp;
	int		rcode;
	int		first_at_is_text;
	register struct attach *at;

	if ((mp = vwl->vwl_msg) == NULL)
		return;

	DP(("mt_save_msg(%x): TEXTSW_MODIFIED %d, modified %d\n",
		vwl, xv_get(vwl->vwl_textsw, TEXTSW_MODIFIED),
		vwl->vwl_modified));

	if ((vwl->vwl_modified) || (vwl->vwl_only_attach_modified)) {
	 	if (vwl->vwl_modified) {
                   /* STRING_EXTRACTION -
                    *
                    * We are checking the message because the user has changed
                    * it.  The user can either save the changes or discard
                    * the changes.  The %d in message is the message number.
                    */
		   if (!mt_vs_confirm(vwl->vwl_textsw, TRUE,
		  		gettext("Save changes"),
		  		gettext("Discard changes"),
		  		gettext(
		        "Message %d has been modified.\nPlease confirm changes."),
		  	 	mp->mo_msg_number))
		   { 
		  	DP(("mt_save_msg: about to TEXTSW_FILE_CONTENTS\n"));
		  	xv_set(vwl->vwl_textsw, TEXTSW_FILE_CONTENTS, NULL, 0);
		  	vwl->vwl_modified = FALSE;
		  	return;
		   }
	 	}

		length = xv_get(vwl->vwl_textsw, TEXTSW_LENGTH);

		buffer = ck_malloc(length);
		rcode = xv_get(vwl->vwl_textsw, TEXTSW_CONTENTS, 0, buffer,
			length);

		/*
		 * Make sure getting TEXTSW_CONTENTS went well
		 */
		if (rcode <= 0 || rcode != length) {
                	/* STRING_EXTRACTION -
                 	 *
			 * We tried to get the contents of the View text pane
			 * but failed.  Since we can't get the contents of the
			 * text pane, we can't save any modifications the user
			 * may have made.
                 	 */
			mt_vs_warn(vwl->vwl_frame,
gettext("Could not get the contents of the View window.\nThis error is commonly caused by performing a\n\"Store as New File\" from the text pane menu and\nthen modifying the new file outside of mailtool.\nUnfortunately this error will cause all changes\nto this message to be lost."));
			ck_free(buffer);
			vwl->vwl_modified = FALSE;
			return;
		}

		if ((at = attach_methods.at_first (mp)) != NULL)
			first_at_is_text = mt_is_text_attachment(at);
		else
			first_at_is_text = FALSE;

		(void)msg_methods.mm_replace(mp, buffer, length);

		/* since we implement the text message as a body part for
		 * RFC-MIME, there will always be a text "at".
		 */
		if ((int) msg_methods.mm_get(mp, MSG_IS_ATTACHMENT) ||
		    first_at_is_text)
		{
			/* Get the size of the text body; it will be used
			 * for the text attachment size.
			 */
			nlines = (int) msg_methods.mm_get(mp, MSG_NUM_LINES);
			length = (int) msg_methods.mm_get(mp, MSG_CONTENT_LEN);

			if (first_at_is_text)
			{
				/* we modify the first text attachment */
				attach_methods.at_set(at, ATTACH_MODIFIED,
							TRUE);
			}
			else
			{
				/* the first attachment was not text type (i.e.
				 * the textsw was empty), user adds something
				 * to textsw, so a new attachment body part is
				 * created.
				 */
				at = attach_methods.at_create();
				attach_methods.at_set(at, ATTACH_DATA_TYPE,
							MT_CE_TEXT_TYPE);
				attach_methods.at_set(at, ATTACH_DATA_DESC,
							MT_CE_TEXT_TYPE);
				attach_methods.at_set(at, ATTACH_DATA_NAME,
							MT_CE_TEXT_TYPE);
				msg_methods.mm_set(mp, MSG_ATTACH_BODY, at, 1);
			}

			/* assign the contents to the attachment */
			attach_methods.at_set(at, ATTACH_BODY, mp->mo_body_ptr);
			attach_methods.at_set(at, ATTACH_CONTENT_LEN, length);
			attach_methods.at_set(at, ATTACH_LINE_COUNT, nlines);
			mp->mo_body_ptr = NULL;

			nlines = 0;
			length = 0;
			for (at = attach_methods.at_first(mp); at != NULL;
			     at = attach_methods.at_next(at))
			{
				length += (int) attach_methods.at_get(at,
							ATTACH_NUM_BYTES);
				nlines += (int) attach_methods.at_get(at,
							ATTACH_LINE_COUNT);
			}

			/* ZZZ: the lines count and byte count are not very
			 * accurate.  The byte count will be corrected when
			 * write to the mail file, but not the line count.
			 * (why line-count?  Too expensive!)
			 */
			msg_methods.mm_set(mp, MSG_CONTENT_LEN, length);
			msg_methods.mm_set(mp, MSG_NUM_LINES, nlines);
		}

		/* make sure it is now marked as "read" */
		mp->mo_new = 0;
		mp->mo_read = 1;
	
		/* the header might have changed. update it */
		ck_free(mp->m_header);
		mp->m_header = makeheader(mp);

		/* update the screen, but not if message is an attachment */
		if (mp->mo_folder != NULL) {
        		hd = mt_get_header_data(vwl->vwl_headerframe);
        		force_repaint_on_line(mp->m_lineno, hd);

			if (CURRENT_FOLDER(hd) &&
		    	    (mp != msg_methods.mm_last(CURRENT_FOLDER(hd)))) {
				force_repaint_on_line(
					msg_methods.mm_next(mp)->m_lineno, hd);
			}
		}

		/* 
		 * Just like in the case of discard, need to reset textsw
		 * so it will see future textsw_notify_procs again,
		 * esp. if this message is pinned.
		 */
                xv_set(vwl->vwl_textsw, TEXTSW_FILE_CONTENTS, NULL, 0);

		DP(("mt_save_msg: about to clear modified\n"));
		vwl->vwl_modified = 0;
	  	vwl->vwl_only_attach_modified = FALSE;
	}
}


/*
 * Load the message into the message subwindow.
 */
void
mt_update_msgsw(
	struct msg	*m,
	int             force_view,
	int             ask_save,
	int             make_visible,
	struct header_data *hd)
{
	struct stat     statb;

	mt_frame_msg(hd->hd_frame, FALSE, "");
	if (ask_save)
		mt_save_curmsgs(hd);

	if (m == NULL)
		return;

	/* select the message in question, so that the call 
	   to build the viewing window structure works */


	acquire_selection(SELN_PRIMARY);

	mt_clear_selected_glyphs(hd);
	m->mo_selected = 1;
	mt_activate_functions();
	force_repaint_on_line(m->m_lineno, hd);

	if (!xv_get(MT_FRAME(hd), FRAME_CLOSED) &&
		(force_view || mt_view_windows_up(hd)))
	{
		mt_create_views(hd, TRUE, MSG_SELECTED);
	}

	MT_CURMSG(hd) = m;
	if (make_visible) {
		mt_update_headersw(m, hd);
	}
}




void
mt_scroll_header(Xv_opaque win, int action)
{
	struct header_data *hd = mt_get_header_data(win);
	int win_height;
	int current;
	struct msg *m;

	win_height = (int) xv_get(hd->hd_canvas, WIN_HEIGHT) / hd->hd_lineheight;
	current = xv_get(hd->hd_scrollbar, SCROLLBAR_VIEW_START);


	switch (action) {
	case ACTION_GO_PAGE_FORWARD:
		current += win_height;
		break;

	case ACTION_GO_PAGE_BACKWARD:
		current -= win_height;
		break;

	case ACTION_GO_DOCUMENT_START:
		current = 0;
		break;

	case ACTION_GO_DOCUMENT_END:
		m = LAST_NDMSG(hd->hd_folder);
		if (m) {
			current = m->m_lineno - 1 + win_height/2;
		}
		break;
	}


	scroll_set_view_len(hd, current, NULL);

	DP(("mt_scroll_header: setting VIEW_START to %d\n", current));

}

		
/*
 * Resize the header canvas
 */
void
mt_resize_canvas(struct header_data *hd)
{
	int	new_height;
	int	win_height;
	int	view_start;
	int	lineno;
	struct msg *m;
	int	current;
	
	if ((m = LAST_NDMSG(hd->hd_folder)) != NULL) {
		lineno = m->m_lineno;
		new_height = m->m_lineno * hd->hd_lineheight + 2;
	} else {
		lineno = 1;
		new_height = hd->hd_lineheight;
	}

	current = xv_get(hd->hd_scrollbar, SCROLLBAR_VIEW_START);
	if (current >= lineno) {
		DP(("mt_resize_canvas: resetting VIEW_START from %d to %d\n", 
			current, lineno -1));
		current = lineno -1;
		scroll_set_view_len(hd, current, NULL);
	}

	/* don't shrink the canvas to be smaller than the current
	 * view window or we will get all sorts of horrible visual
	 * effects
	 */
	current *= hd->hd_lineheight;	/* convert to pixels */
	win_height = xv_get(hd->hd_canvas, XV_HEIGHT);

	/* add in the screen height */
	new_height += xv_get(hd->hd_canvas, XV_HEIGHT);

	DP(("mt_resize_canvas: new size %d\n", new_height));

	if (new_height >= hd->hd_maxlines * hd->hd_lineheight) {
		if (! hd->hd_toomanylines) {
			hd->hd_toomanylines = 1;
			mt_vs_warn(hd->hd_frame, gettext(
"Warning: the mail file you are viewing has too many lines\n\
in it to be viewable.  You will only be shown the top %d lines\n\
of the mail file.  You will need to delete some messages before\n\
you can view the rest of the mail file"), hd->hd_maxlines);
		}
		new_height = hd->hd_lineheight * hd->hd_maxlines;
	} else {
		hd->hd_toomanylines = 0;
	}

	xv_set(hd->hd_canvas, CANVAS_HEIGHT, new_height, 0);

        if (hd->hd_dropsite) {
		int width;
		int height;
		Rect rect;

                rect.r_left = 0;
                rect.r_top = 0;
                rect.r_width = xv_get(hd->hd_canvas, WIN_WIDTH);
                rect.r_height = xv_get(hd->hd_canvas, WIN_HEIGHT);
 
                DP(("header_resize_proc: canvas %x, width %d, height %d\n",
                        hd->hd_canvas, rect.r_width, rect.r_height));
 
                DP(("header_resize_proc: setting the drop site\n"));
 
                xv_set(hd->hd_dropsite,
                        DROP_SITE_DELETE_REGION, NULL,
                        DROP_SITE_REGION, &rect,
                        0);
        }
}



void
mt_update_folder_status(struct header_data *hd)
{
	int max;
	int lines;
	struct msg *m;

	if (MT_FRAME(hd))
	{
		lines = 0;
		if (CURRENT_FOLDER(hd)) {
			max = CURRENT_FOLDER(hd)->fo_num_msgs;
			m = LAST_NDMSG(CURRENT_FOLDER(hd));
			if (m) {
				lines = m->m_lineno;
			}
		} else {
			max = 0;
		}

		*mt_namestripe_right = '\0';
                /* STRING_EXTRACTION -
                 *
                 * This is the message for the namestripe (lower right in
                 * the header footer).  The format is total number of mail
                 * messages, number of new messages, and number of deleted
                 * messages.
                 */
		(void) sprintf(mt_namestripe_right,
			gettext("%d items, %d new, %d deleted"),
			max, mt_new_count, max - lines);

		xv_set(MT_FRAME(hd), FRAME_RIGHT_FOOTER, mt_namestripe_right, 0);
	}
}

/*
 * Announce the arrival of new mail.
 */
void
mt_announce_mail(void)
{
	static struct timeval tv = {0, 150000};
	int             bells, flashes;
	Rect		r;
	Drawable	xid;
	Icon		icon;
	GC		gc;
	Server_image	si = NULL;
	struct header_data *hd;
#ifdef	NOTIFY_PROG
	char		*prog, *sh;

	if ((prog = mt_value("script")) && (*prog != '\0')) {
		if (vfork() == 0) {
			if ((sh = mt_value("SHELL")) == NULL)
				sh = "/bin/sh";
			execlp(sh, "sh", "-c", prog, 0);
			exit(-1);
		}
	}
#endif	!NOTIFY_PROG


	bells = mt_bells;
	flashes = mt_flashes;
	if (bells <= 0 && flashes <= 0)
		return;

	/*
	 * Making it announce on first base frame's 
	 * header data struct for now
	 */
	hd = mt_header_data_list;

	if (flashes > 0) {
		if (xv_get(MT_FRAME(hd), FRAME_CLOSED)) {
			icon = (Icon)xv_get(MT_FRAME(hd), FRAME_ICON);
			si = (Server_image)xv_get(icon, ICON_IMAGE);
			xid = hd->hd_icon_drawable;
			r.r_top = 0;
			r.r_width = (int) icon_get(icon,
				(Icon_attribute) ICON_WIDTH);
			r.r_height = (int) icon_get(icon,
				(Icon_attribute) ICON_HEIGHT);
			gc = hd->hd_icongc;
		} else {
			xid = hd->hd_drawable;
			r = *((Rect *) xv_get(hd->hd_canvas,
					CANVAS_VIEWABLE_RECT, hd->hd_paintwin));
			gc = hd->hd_gc;
		}
	}

	for (;;) {
		if (flashes > 0) {
			mt_invert_region(hd->hd_display, xid, gc, 0,
				r.r_top, r.r_width, r.r_height,
				hd->hd_foreground, hd->hd_background);
			/* invert the icon; otherwise, flush buffered request */
			if (si != NULL)
				xv_set(icon, ICON_IMAGE, si, 0);
			else
				XFlush(hd->hd_display);
		}

		if (--bells >= 0)
		{
			XBell(hd->hd_display, 100);
			XFlush(hd->hd_display);
		}

		if (--flashes >= 0) {
			select(0, 0, 0, 0, &tv);
			mt_invert_region(hd->hd_display, xid, gc, 0,
				r.r_top, r.r_width, r.r_height,
				hd->hd_foreground, hd->hd_background);

			/* restore the icon */
			if (si != NULL)
				xv_set(icon, ICON_IMAGE, si, 0);
			else
				XFlush(hd->hd_display);
		}

		if (bells <= 0 && flashes <= 0)
			break;

		/* pause between bells and flashes */
		select(0, 0, 0, 0, &tv);
	}
}



/*
 * Request confirmation from the user. Use alerts.  
 */
/* VARARGS1 */
int
#ifdef __STDC__
mt_vs_confirm3(Frame frame, int optional, char *button1, char *button2, char *button3, char *fmt, ...)
#else
mt_vs_confirm3(frame, optional, button1, button2, button3, fmt, va_alist)
	Frame		frame;
	int		optional;
	char		*button1, *button2, *button3;
	char		*fmt;
	va_dcl
#endif "__STDC__"
{
	va_list         args;

	/*
	 * Put up a 3 button notice
	 */
	if (mt_value("expert") && optional)
		return(1);

	if (frame == (Frame) NULL)
		frame = MT_FRAME(mt_header_data_list);

	if (mt_use_alerts) {

		int	status, i;
		char	*array[100];
		Event	ie;
		char	buf[1024];
		char	*p;

		/* minor bug waiting to happen: we don't check for
		 * overflow of the buf array
		 */
#ifdef __STDC__
		va_start(args, fmt);
#else
		va_start(args);
#endif "__STDC__"
		vsprintf(buf, fmt, args);
		va_end(args);

		/* for each "line" ('\n') in the buffer, split it
		 * apart into a separate string
		 *
		 * I shouldn't have hard coded in 100 lines, but it seems
		 * to be a limit that is big enough to not worry about.
		 */
		for(p = buf, i = 0; i < (sizeof array/sizeof(char *)-1) ; i++) {
			
			array[i] = p;

			p = strchr(p, '\n');

			if(! p) break;

			*p++ = '\0';
		}
		array[i+1] = 0;

		status = notice_prompt(frame,
			&ie,
			NOTICE_MESSAGE_STRINGS_ARRAY_PTR, array,
			NOTICE_BUTTON_YES,	button1,
			NOTICE_BUTTON,	button2,	2,
			NOTICE_BUTTON_NO,	button3,
			0);
		switch (status) {
		case NOTICE_YES:
		case 2:
		case NOTICE_NO:
			return(status);

		case NOTICE_FAILED:
			alert_failed(2, array);
			return(0);
		}
	}
}




/*
 * Request confirmation from the user. Use alerts.  
 */
/* VARARGS1 */
int
#ifdef __STDC__
mt_vs_confirm(Frame frame, int optional, char *button1, char *button2, char *fmt, ...)
#else
mt_vs_confirm(frame, optional, button1, button2, fmt, va_alist)
	Frame		frame;
	int		optional;
	char		*button1, *button2;
	char		*fmt;
	va_dcl
#endif "__STDC__"
{
	va_list         args;

	if (mt_value("expert") && optional)
		return(TRUE);

	if (frame == (Frame) NULL)
		frame = MT_FRAME(mt_header_data_list);

	if (mt_use_alerts) {
		int	status, i;
		char	*array[100];
		char	buf[10240];
		Event   ie;
		char	*p;

		/* minor bug waiting to happen: we don't check for
		 * overflow of the buf array
		 */
#ifdef __STDC__
		va_start(args, fmt);
#else
		va_start(args);
#endif "__STDC__"
		vsprintf(buf, fmt, args);
		va_end(args);

		/* for each "line" ('\n') in the buffer, split it
		 * apart into a separate string
		 *
		 * I shouldn't have hard coded in 100 lines, but it seems
		 * to be a limit that is big enough to not worry about.
		 */
		for(p = buf, i = 0; i < (sizeof array/sizeof(char *)-1) ; i++) {
			
			array[i] = p;

			p = strchr(p, '\n');

			if(! p) break;

			*p++ = '\0';
		}
		array[i+1] = 0;

		status = notice_prompt(frame,
			&ie,
			NOTICE_MESSAGE_STRINGS_ARRAY_PTR, array,
			NOTICE_BUTTON_YES, button1,
			NOTICE_BUTTON_NO, button2,
			0);
		switch (status) {
		case NOTICE_YES:
			return (TRUE);
		case NOTICE_NO:
			return(FALSE);
		case NOTICE_FAILED:
			alert_failed(2, array);
			return(FALSE);
		}
	}
}



static void
mt_vs_warn1(Frame frame, char *fmt, va_list args)
{

	if (mt_use_alerts) {
		int	status;
		int	i;
		char	*array[100];
		char	buf[10240];
		char	*p;
		Event	ie;

		/* minor bug waiting to happen: we don't check for
		 * overflow of the buf array
		 */
		vsprintf(buf, fmt, args);

		/* for each "line" ('\n') in the buffer, split it
		 * apart into a separate string
		 *
		 * I shouldn't have hard coded in 100 lines, but it seems
		 * to be a limit that is big enough to not worry about.
		 */
		for(p = buf, i = 0; i < (sizeof array/sizeof(char *)-1) ; i++) {
			
			array[i] = p;

			p = strchr(p, '\n');

			if(! p) break;

			*p++ = '\0';
		}
		array[i+1] = 0;

		if (frame == (Frame) NULL)
			frame = MT_FRAME(mt_header_data_list);
                /* STRING_EXTRACTION -
                 *
                 * we are putting up a warning message.  Continue is
                 * the label of the button that allows us to run again.
                 */
		status = notice_prompt(frame,
			&ie,
			NOTICE_MESSAGE_STRINGS_ARRAY_PTR, array,
			NOTICE_BUTTON_YES, gettext("Continue"),
			0);
		switch (status) {
		case NOTICE_YES:
		case NOTICE_TRIGGERED:
			break;

		case NOTICE_FAILED:
			alert_failed(1, array);
			break;
		}
	}
}


void
#ifdef __STDC__
mt_vs_exit(Frame frame, char *fmt, ...)
#else
mt_vs_exit(frame, fmt, va_alist)
	Frame		frame;
	char		*fmt;
	va_dcl
#endif "__STDC__"
{
	va_list		args;

#ifdef __STDC__
	va_start(args, fmt);
#else
	va_start(args);
#endif "__STDC__"
	mt_vs_warn1(frame, fmt, args);
	va_end(args);

	exit(1);
}



/*
 * like mt_warn, but it takes an sprintf string instead
 */
/* VARARGS2 */
void
#ifdef __STDC__
mt_vs_warn(Frame frame, char *fmt, ...)
#else
mt_vs_warn(frame, fmt, va_alist)
	Frame		frame;
	char		*fmt;
	va_dcl
#endif "__STDC__"
{
	va_list         args;

#ifdef __STDC__
	va_start(args, fmt);
#else
	va_start(args);
#endif "__STDC__"
	mt_vs_warn1(frame, fmt, args);
	va_end(args);
}




static void
alert_failed(int type, char *array[])
{
	int	i;
	int	len;
	char	*str1, *str2;

	/* STRING_EXTRACTION -
	 *
	 * The follow messages appear on stderr ONLY if the window system
	 * warning could not be printed.
	 *
	 * The first string is displayed if a warning could not be put up.
	 *
	 * The next two are if a confirmation could not be asked for.
	 */
	switch(type) {
	case 1: /* warning */
		str1 = gettext(
	"Could not put up alert to warn user of the following condition:\n");
		str2 = "\n";
		break;
		
	case 2: /* confirmation */
		str1 = gettext(
"Could not put up alert to request confirmation of the following operation:\n");
		str2 = gettext(
	"\nProceeding by assuming that this operation was NOT confirmed\n");
		break;

	default:
		str1 = 0;
		str2 = 0;
	}


	(void) fprintf(stderr, str1);

	for (i = 0; array[i] != (char *) 0; i++) {
		(void) fprintf(stderr, array[i]);

		/* if the string hasn't a newline, put a newline out */
		len = strlen(array[i]);
		if (array[i][len -1] != '\n') {
			putc('\n', stderr);
		}
	}

	if (str2) {
		(void) fprintf(stderr, str2);
	}
}


void
mt_nomail_warning(struct header_data *hd)
{
	/* STRING_EXTRACTION -
	 *
	 * The user tried to do an operation (such as reply or delete),
	 * but the folder is empty.  We give a terse error message
	 * "No Mail" to say that we can't do the operation.
	 */
	mt_vs_warn(MT_FRAME(hd), gettext("No Mail"));
}



static void
mt_draw_diamond(int x, int y, struct header_data *hd)
{
	int	npts;
	XPoint	vlist[7];
	int	width;
	int	n1, n2;
	int	side_length;
	int	half_side_length;

	width = xv_get(hd->hd_textfont, FONT_COLUMN_WIDTH);

	/* do the "attachment" glyph */

	/* If width is even, make it odd */
	if ((width % 2) == 0)
		width--;

	n1 = width - 1;
	n2 = n1 / 2;

	side_length = n2 + 1;
	half_side_length = side_length / 2;

	/* Draw diamond */
	vlist[0].x = x + n2;
	vlist[0].y = y + 0;

	vlist[1].x = x + n1;
	vlist[1].y = y + n2;

	vlist[2].x = x + n2;
	vlist[2].y = y + n1;

	vlist[3].x = x + 0;
	vlist[3].y = y + n2;

	npts = 4;

	XDrawLines(hd->hd_display, hd->hd_drawable, hd->hd_gc, vlist, npts,
		CoordModeOrigin);

	XFillPolygon(hd->hd_display, hd->hd_drawable, hd->hd_gc, vlist, npts,
		Convex, CoordModeOrigin);
}



static void
mt_draw_arrow(int x, int y, struct header_data *hd)
{
	int	npts;
	XPoint	vlist[7];
	int	width;
	int	height;
	int	center;
	int	shaft_rise, shaft_w, head_rise, arrow_w;

	height = (int)xv_get(hd->hd_textfont, FONT_DEFAULT_CHAR_HEIGHT);
	width =  (int)xv_get(hd->hd_textfont, FONT_COLUMN_WIDTH) * 2;

	/* do the "current" glyph */

	center = height / 2;
	shaft_rise = height / 8;
	shaft_w = width / 2 + 1;
	head_rise = 4 * shaft_rise;
	arrow_w = width - 2;

	vlist[0].x = x + 1;
	vlist[0].y = y + center - shaft_rise;
	vlist[1].x = x + shaft_w;
	vlist[1].y = y + center - shaft_rise;

	vlist[2].x = x + shaft_w;
	vlist[2].y = y + center - head_rise;
	vlist[3].x = x + arrow_w;
	vlist[3].y = y + center;
	vlist[4].x = x + shaft_w;
	vlist[4].y = y + center + head_rise;

	vlist[5].x = x + shaft_w;
	vlist[5].y = y + center + shaft_rise;
	vlist[6].x = x + 1;
	vlist[6].y = y + center + shaft_rise;

	npts = 7;

	XDrawLines(hd->hd_display, hd->hd_drawable, hd->hd_gc, vlist, npts,
		CoordModeOrigin);
	XFillPolygon(hd->hd_display, hd->hd_drawable, hd->hd_gc, vlist, npts,
		Convex, CoordModeOrigin);
}


int
mt_has_attachments(struct msg *m)
{
	struct attach *at;

	if ((at = attach_methods.at_first(m)) == NULL)
		return(0);
	if (mt_is_text_attachment(at) && (attach_methods.at_next(at) == NULL))
		return(0);
	return(1);
	
}

void
mt_draw_glyph(struct msg *m, struct header_data *hd, int x, int y)
{
	int	height, width;
	struct attach *at;

	height = xv_get(hd->hd_textfont, FONT_DEFAULT_CHAR_HEIGHT);

	if (m->mo_current) {
		mt_draw_arrow(x, y, hd);
	} else if (m->mo_new) {
		mt_draw_text(hd->hd_display, hd->hd_drawable, hd->hd_gc,
			x, y + height - 2, "N", 1);
	} else if (!m->mo_read){
		mt_draw_text(hd->hd_display, hd->hd_drawable, hd->hd_gc,
			x, y + height - 2, "U", 1);
	}

	if (!m->mo_current && mt_has_attachments(m))
	{
		width = (int)xv_get(hd->hd_textfont, FONT_COLUMN_WIDTH);
		x += width + width / 4;
		y += width / 2;
		mt_draw_diamond(x, y, hd);
	}

}



void
mt_redo_glyphs(
	struct msg *beginning,	/* msg to repaint from.  NULL to start at top */
	struct header_data *hd)
{
	int	i;
	struct msg *m;

	mt_new_count = 0;

	for (m = FIRST_NDMSG(CURRENT_FOLDER(hd)); m != NULL; m = NEXT_NDMSG(m))
	{
		if (m->mo_new && !(m->mo_current))
		{
			mt_new_count++;
		}
	}


	mt_update_folder_status(hd);

	if (beginning)
		force_repaint_from_line(beginning->m_lineno, TRUE, hd);
	else
		force_repaint_from_line(1, TRUE, hd);
}




void
mt_clear_selected_glyphs(struct header_data *hd)
{
	struct msg	*m;

	for (m = FIRST_NDMSG(hd->hd_folder); m != NULL; m = NEXT_NDMSG(m))
	{
		if (m->mo_selected)
		{
			m->mo_selected = 0;
			mt_draw_selection_box(hd, m);
		}

	}

	if (!mt_any_selected(hd))
	{
		for (m = msg_methods.mm_first(hd->hd_folder); m != NULL;
		     m = msg_methods.mm_next(m))
		{
			if (m->mo_current) {
				return;
			}
		}

		if (m == NULL) {
			mt_deactivate_functions();
		}
	}
}


void
mt_toggle_glyph(int line, struct header_data *hd)
{
	struct msg	*m;
	int		previously_selected;

	/* determine what entry in the data structure has 
	   the right line on the screen. */

	m = LINE_TO_MSG(hd, line);

	if (m == NULL) {
		return;
	}

	/* we should have the right index now, make sure we have
	 * the selection
	 */

	acquire_selection(SELN_PRIMARY);
	
	/* toggle the data structure */

	previously_selected = m->mo_selected;
	m->mo_selected = !previously_selected;

	mt_draw_selection_box(hd, m);

	TRACK_TOGGLE_SELECT(m->mo_msg_number, m->mo_selected);

	if (!previously_selected) {
		mt_activate_functions();
	} else {
		/* if we have deselected the last mail item, deactivate all the 
		 * appropriate functions
		 */

		if (!mt_any_selected(hd))
		{

			for (m = msg_methods.mm_first(hd->hd_folder);
			     m != NULL; m = msg_methods.mm_next(m))
			{
				if (m->mo_current)
				{
					return;
				}
			}
	
			if (m == NULL) {
				mt_deactivate_functions();
			}
		}

	}
}



void
mt_header_seln_func_proc(char *client_data, Seln_function_buffer *args)
{
	Seln_holder	*holder;
	Menu	bogus_menu;
	struct header_data *hd;

	DP(("mt_header_seln_func_proc: called\n"));

	hd = (struct header_data *) client_data;
	switch (seln_figure_response(args, &holder)) {

	case SELN_IGNORE :
		/* do nothing */
		DP(("SELN_IGNORE\n"));
		break;

	case SELN_REQUEST:
		/* do nothing */
		DP(("SELN_REQUEST\n"));
		break;

	case SELN_SHELVE:
		DP(("SELN_SHELVE\n"));
		bogus_menu = mt_create_bogus_menu(hd);
		mt_copyshelf_proc(bogus_menu, NULL);
		break;

	case SELN_FIND:
		/* This makes no sense in the context
		 * of the mail headers
		 */
		DP(("SELN_FIND\n"));
		break;

	case SELN_DELETE:
		DP(("SELN_DELETE\n"));
		mt_del_proc_hd(hd);
		break;
	}

}



Seln_result
mt_header_seln_reply_proc(Seln_attribute item, Seln_replier_data *context,
	int length)
{
	char		*destbuf;
	int		will_fit;
	register struct mt_select *ms;
	register struct mt_context *mc;
	static struct mt_context fast_context;
	register Seln_result result = SELN_SUCCESS;
	struct header_data *hd;

	hd = (struct header_data *) context->client_data;
	ms = rank_to_ms(context->rank);

	DP(("mt_header_seln_reply_proc: called\n"));

#ifdef DEBUG
	switch (context->rank) {
	case SELN_PRIMARY:
		DP(("mt_header_seln_reply_proc: PRIMARY\n"));
		break;

	case SELN_SECONDARY:
		DP(("mt_header_seln_reply_proc: SECONDARY !!!\n"));
		break;

	case SELN_SHELF:
		DP(("mt_header_seln_reply_proc: SHELF\n"));
		break;
	}
#endif DEBUG

	if (! ms || !ms->ms_haveselection) {
		return (SELN_DIDNT_HAVE);
	}

	if (context->context) {
		DP((" Existing Context\n"));
		mc = (struct mt_context *) context->context;
	} else if (item == SELN_REQ_END_REQUEST) {
		/* don't set up state when cleared due to previous error */
		mc = NULL;
	} else {
		DP((" First Context\n"));
		if (fast_context.mc_inuse) {
			mc = ck_malloc(sizeof(*mc));
		} else {
			mc = &fast_context;
		}
		mc->mc_inuse = 1;
		context->context = (char *) mc;
		mc->mc_ptr = 0;

	}

	switch (item) {

	case	SELN_REQ_CONTENTS_ASCII:

		destbuf = (char *) context->response_pointer;
		will_fit =  length - 4;

		DP(("mt_header_seln_reply_proc: CONTENTS_ASCII size = %d\n",
			will_fit));

		if (! mc->mc_ptr) {
			/* first time through -- set up the pointers */
			if (context->rank != SELN_SHELF) {
				make_selection(hd, context->rank);
			}
			mc->mc_ptr = ms->ms_buffer;
			mc->mc_end = ms->ms_buffer + ms->ms_size;
		}


		if (mc->mc_end - mc->mc_ptr < will_fit) {
			/* there is enough room for the rest */
			will_fit = mc->mc_end - mc->mc_ptr;
		} else {
			result = SELN_CONTINUED;
		}

		memcpy(destbuf, mc->mc_ptr, will_fit);
		mc->mc_ptr += will_fit;
		destbuf += will_fit;
		while (((int) destbuf % 4) != 0) {
			*destbuf++ = NULL;
		}

		context->response_pointer = (caddr_t *) destbuf;

		if (result != SELN_CONTINUED)
		{
			*context->response_pointer++ = NULL;
			DP(("  SELN_SUCCESS: will_fit %d\n", will_fit));
		}
#ifdef DEBUG
		else {
			DP(("  SELN_CONTINUED: will_fit %d\n", will_fit));
		}
#endif DEBUG
		break;


	case	SELN_REQ_YIELD:

		DP(("mt_header_seln_reply_proc: YIELD\n"));
		ms = rank_to_ms(context->rank);
		ms->ms_haveselection = 0;
		reset_selection(context->rank);
		mt_clear_selected_glyphs(mt_get_header_data(MT_FRAME(hd)));
		break;

	case	SELN_REQ_BYTESIZE:

		*context->response_pointer++ = (char *)
			get_selection_size(hd, context->rank);

		DP(("mt_header_seln_reply_proc: BYTESIZE = %d\n",
			*(context->response_pointer -1)));

		break;

	case	SELN_REQ_FIRST:
		*context->response_pointer++ = (char *) 0;
		break;

	case	SELN_REQ_LEVEL:
		*context->response_pointer++ = (char *) SELN_LEVEL_FIRST;
		break;

	case	SELN_REQ_END_REQUEST:
		DP(("mt_header_seln_reply_proc: END_REQUEST\n"));

		if (context->rank != SELN_SHELF) {
			reset_selection(context->rank);
		}

		/* ignored by user; force mc_context to be freed */
		result = SELN_FAILED;
		break;

	default:
		result = SELN_UNRECOGNIZED;
		break;
	}

	switch (result) {
	case SELN_SUCCESS:
	case SELN_CONTINUED:
	case SELN_UNRECOGNIZED:
		break;

	default:
		/* free up the context */
		if (mc == &fast_context) {
			fast_context.mc_inuse = 0;
		} else {
			free(mc);
		}
		context->context = NULL;
		break;
	}
	return (result);

}



void
mt_frame_msg(Frame frame, int beep, char *string)
{
	DP(("mt_frame_msg: %s\n", string));

	if (frame) {
	    xv_set(frame, FRAME_LEFT_FOOTER, string, 0);

	    if (beep) {
		XBell((Display *)xv_get(frame, XV_DISPLAY), 100);
	    }
	}
}



Notify_value
mt_canvas_interpose_proc(Canvas canvas, Event *event, Notify_arg arg,
	Notify_event_type type)
{
	static Event	last_event;
	short	width;
	short	line;
	struct msg *m;
	Xv_font	pf;
	Font_string_dims	size;
	int	item_selected;
	char	folder_name[2048];
	Menu	bogus_menu;
	struct header_data *hd;

	hd = mt_get_header_data(canvas);
	if (event_is_up(event))
		(void)xv_set(MT_FRAME(hd), FRAME_LEFT_FOOTER, "", 0);

	line = event_y(event)/hd->hd_lineheight + 1;

	switch (event_action(event)) {

	case ACTION_ADJUST:
		if (event_is_down(event)) {
			acquire_selection(SELN_PRIMARY);
			mt_selection_start = mt_selection_end = line;
			mt_toggle_glyph(line, hd);
			hd->hd_selection_drag = TRUE;
		} else if (event_is_up(event))
		{

			if (hd->hd_selection_drag)
			{
				stretch_box_selection(canvas, event);
				hd->hd_selection_drag = FALSE;

				TRACK_DRAG_SELECT(mt_selection_start,
					mt_selection_end);
			}

		}
		return(NOTIFY_DONE);

	case ACTION_SELECT:
		if (event_is_down(event))
		{
			hd->hd_looking_for_dnd = 0;

			/* determine what message the select 
			   action happened on top of. */

			if ((m = LINE_TO_MSG(hd, (int) line)) == NULL)
			{
				return(NOTIFY_DONE);
			}

			/* if the click was within a 
			   certain time of the last 
			   left click, it is a shorthand 
			   for show.  Use the same 
			   time limit as for the 
			   folders processing. */

			if (ds_is_double_click(&last_event, event)) {
				last_event = *event;
				TRACK_SHOWMSG(m->mo_msg_number, "double_click");
				mt_update_msgsw(m, TRUE, TRUE, FALSE, hd);
				return(NOTIFY_DONE);
			}

			last_event = *event;

			/* determine if the selection is within a 
			   selected area.  This might mean that 
			   they will try to drag/drop the messages, 
			   they might not.  We will set a global 
			   so that we can look for this in the 
			   LOC_DRAG code later.  If there isn't 
			   enough mouse motion, the item isn't 
			   picked up, and the selection is just reset. */

			if (m->mo_selected)
			{
				hd->hd_looking_for_dnd = 1;
				hd->hd_last_x = event_x(event);
				hd->hd_last_y = event_y(event);
				hd->hd_copy = event_ctrl_is_down(event);
				return(NOTIFY_DONE);
			}

			/* a selection begin on our
			   glyph means that we should 
			   clear out any selection 
			   in the text area. */

			mt_clear_selected_glyphs(hd);
			acquire_selection(SELN_PRIMARY);
			mt_selection_start = mt_selection_end = line;
			select_rgn(mt_selection_start, mt_selection_end, hd);
			hd->hd_selection_drag = TRUE;
		}
		else if (event_is_up(event))
		{
			if (hd->hd_selection_drag)
			{
				stretch_box_selection(canvas, event);
				hd->hd_selection_drag = FALSE;

				TRACK_DRAG_SELECT(mt_selection_start,
					mt_selection_end);
			}

			if (hd->hd_looking_for_dnd) {
				hd->hd_looking_for_dnd = 0;

				mt_clear_selected_glyphs(hd);
				acquire_selection(SELN_PRIMARY);
				mt_selection_start = mt_selection_end = line;
				select_rgn(mt_selection_start,
					mt_selection_end, hd);
			}
		}
		return(NOTIFY_DONE);

	case ACTION_MENU:
		if (event_is_down(event))
		{
			menu_show( xv_get(hd->hd_canvas, WIN_MENU),
				canvas, event, 0);
		}
		return(NOTIFY_DONE);

	case LOC_DRAG:
		if (hd->hd_selection_drag)
		{
			stretch_box_selection(canvas, event);
		}
		else if (hd->hd_looking_for_dnd)
		{
			if ((abs(event_x(event) - hd->hd_last_x) > DAMPING) ||
				(abs(event_y(event) - hd->hd_last_y) > DAMPING))
			{
				drag_headermsg(hd);
				hd->hd_looking_for_dnd = 0;
			}
		}

		return(NOTIFY_DONE);

	case LOC_WINEXIT:
		if (hd->hd_selection_drag)
		{
			stretch_box_selection(canvas, event);
			hd->hd_selection_drag = FALSE;
			TRACK_DRAG_SELECT(mt_selection_start,
				mt_selection_end);

		}
		else if (hd->hd_looking_for_dnd)
		{
			hd->hd_looking_for_dnd = 0;
		}

		return(NOTIFY_DONE);

	case ACTION_COPY:
		if (event_is_down(event)) {
			bogus_menu = mt_create_bogus_menu(hd);
			mt_copyshelf_proc(bogus_menu, NULL);
		}
		return (NOTIFY_DONE);

	case ACTION_CUT:
		if (event_is_down(event)) {
			mt_del_proc_hd(hd);
		}
		return (NOTIFY_DONE);

	case ACTION_GO_CHAR_FORWARD:
		/* 
		 * Left and Right arrow keys are handled here.  All other
		 * function keys are handeled in mt_handle_keys_proc().
		 * We handle these here so that they will only work over
		 * the canvas.  When over the cmd panel they move the
		 * cursor in the text field.
		 */
		if (event_is_up(event)) {
			bogus_menu = mt_create_bogus_menu(hd);
			mt_next_proc(bogus_menu, NULL);
		}
		return(NOTIFY_DONE);

	case ACTION_GO_CHAR_BACKWARD:
		if (event_is_up(event)) {
			bogus_menu = mt_create_bogus_menu(hd);
			mt_prev_proc(bogus_menu, NULL);
		}
		return(NOTIFY_DONE);

	}

	return notify_next_event_func(canvas, (Notify_event) event, arg, type);
}



static void
mt_set_region(short low, short high, short select, struct header_data *hd)
{
	struct msg	*m;
	int		line;

	for (m = FIRST_NDMSG(hd->hd_folder); m != NULL; m = NEXT_NDMSG(m))
	{
		line = m->m_lineno;

		if ((line >= low) && (line <= high))
		{
			m->mo_selected = select;
			mt_draw_selection_box(hd, m);
		}
	}
}


static void
xor_rgn(struct header_data *hd, int start, int stop)
{
	struct msg *m;
	int tmp;

	/* make sure start is < stop */
	if (start > stop) {
		tmp = start;
		start = stop;
		stop = tmp;
	}

	m = LINE_TO_MSG(hd, start);

	while (m && m->m_lineno <= stop) {
		m->mo_selected = !m->mo_selected;
		m = NEXT_NDMSG(m);
	}
}



static void
mt_repaint_rgn(int start, int stop, int line, struct header_data *hd)
{
	int min;
	int max;
	struct msg *m;

	min = start;
	if (min > stop) min = stop;
	if (min > line) min = line;

	max = start;
	if (max < stop) max = stop;
	if (max < line) max = line;

	m = LINE_TO_MSG(hd, min);

	while (m && m->m_lineno <= max) {
		mt_draw_selection_box(hd, m);

		m = NEXT_NDMSG(m);
	}

}




static void
select_rgn(short low, short high, struct header_data *hd)

{
	mt_set_region(low, high, TRUE, hd);
	mt_activate_functions();
}



static void
stretch_box_selection(Canvas canvas, Event *event)
{
	short	line;
	struct	header_data *hd;
	struct msg *m;
	int 	previously_selected;

	hd = mt_get_header_data(canvas);
	line = event_y(event)/hd->hd_lineheight + 1;


	/* if we didn't move, don't do anything. */

	if (line == mt_selection_end)
		return;

	previously_selected = (int) mt_any_selected(hd);

	/* clear out any selection that no longer belong */

	/* Two cases - previous region was from start to
	   higher number or from start to lower number. */

	MP(("stretch_box_selection: start %d, end %d, line %d\n",
		mt_selection_start, mt_selection_end, line));

	if (mt_selection_start == mt_selection_end)
	{
		if (line < mt_selection_start) {
			xor_rgn(hd, mt_selection_start -1, line);
		} else {
			xor_rgn(hd, mt_selection_start +1, line);
		}
	}
	else if (mt_selection_start <= mt_selection_end)
	{
		/* 4 cases.  */
		if (line == mt_selection_start) {
			xor_rgn(hd, line +1, mt_selection_end);
		}
		else if (line < mt_selection_start)
		{

			/* 1st case - new endpoint a before previous start.
			 *
			 * Zero out from start + 1 to end,
			 * select from new endpoint thru start
			 */
			xor_rgn(hd, mt_selection_start +1, mt_selection_end);
			xor_rgn(hd, line, mt_selection_start-1);
		}
		else if (line < mt_selection_end)
		{

			/* Second case - new endpoint is 
			 * within previous selection.
			 *
			 * Zero out from new endpoint + 1 thru old endpoint.
			 */
			xor_rgn(hd, line + 1, mt_selection_end);
		}
		else
		{

			/* Third case - new endpoint is past 
			 * previous selection */
	
			/* select from old endpoint + 1 thru new endpoint. */

			xor_rgn(hd, mt_selection_end + 1, line);
		}
	}
	else
	{
		/* 3 cases.  */

		if (line == mt_selection_start) {
			xor_rgn(hd, line -1, mt_selection_end);
		}
		else if (line > mt_selection_start)
		{

			/* 1st case - new endpoint after
			   previous start. */
	
			/* Zero out from end to start - 1,
			   select from start thru new 
			   endpoint */

			xor_rgn(hd, mt_selection_end, mt_selection_start - 1);
			xor_rgn(hd, mt_selection_start +1, line);
		}

		else if (line > mt_selection_end)
		{

			/* Second case - new endpoint is 
			   within previous selection. */
	
			/* Zero out from new old endpoint
			   thru endpoint - 1  */

			xor_rgn(hd, mt_selection_end, line - 1);
		}
		else
		{

			/* Third case - new endpoint is past 
			   previous selection */
	
			/* select from new endpoint thru
			   old endpoint - 1 */

			xor_rgn(hd, line, mt_selection_end - 1);
		}
	}

	if (!previously_selected) {
		mt_activate_functions();
	} else {
		if (!mt_any_selected(hd))
		{
			for (m = msg_methods.mm_first(hd->hd_folder); m != NULL;
			     m = msg_methods.mm_next(m))
			{
				if (m->mo_current) {
					break;
				}
			}

			if (m == NULL) {
				mt_deactivate_functions();
			}
		}
	}

	mt_repaint_rgn(mt_selection_start, mt_selection_end, line, hd);

	mt_selection_end = line;
}


void
mt_label_frame(Frame frame, char *string)
{
	char    label[256];
	int	max;
 
	/*
	 * Label a popup frame.
	 */

	max = sizeof(label) - 3 - strlen(name_Mail_Tool);
	sprintf(label, "%s: %.*s", name_Mail_Tool, max, string);
 
	xv_set(frame,
		FRAME_LABEL,		label,
		FRAME_SHOW_LABEL,	TRUE,
	0);
}


void
mt_path_to_folder(char *path, char *new_folder)
{
	char	*rc_folder_dir;
	char	folder_dir[256];

	/*
	 * If path is preceded by the mail folder directory, then strip
	 * it off and put in a '+'
	 */

	if (getfolderdir(folder_dir) >= 0 &&
		strncmp(path, folder_dir, strlen(folder_dir)) == 0)
	{
		sprintf(new_folder, "+%s", &path[strlen(folder_dir) + 1]);
	} else {
		strcpy(new_folder, path);
	}
}



/*
 * turn a rank into a mt_select structure
 */
static struct mt_select *
rank_to_ms(Seln_rank rank)
{
	struct mt_select *ms = NULL;

	switch(rank) {
	case SELN_PRIMARY:
		ms = &mt_sel_buffers[MR_PRIMARY];
		break;
	case SELN_SHELF:
		ms = &mt_sel_buffers[MR_SHELF];
		break;
	case SELN_SECONDARY:
		ms = &mt_sel_buffers[MR_SECONDARY];
		break;
#ifdef DEBUG
	default:
		DP(("rank_to_ms: illegal rank %d\n", rank));
		mt_done(1);
#endif DEBUG
	}

	return (ms);

}



/*
 * a filter function that calls copymsg on every selected message
 */
static unsigned long
get_sel_msg(struct msg *m, va_list ap)
{
	typedef int (*gsm_func)();
	gsm_func func;
	struct mt_select *ms;

	func = va_arg(ap, gsm_func);
	ms = va_arg(ap, struct mt_select *);

	if (!m->mo_deleted && m->mo_selected) {
		return (msg_methods.mm_copymsg(m, 1, func, (int) ms, 0));
	}
	return (0);
}



/*
 * figure out the size of a message
 */
static int
get_sel_msg_size(char *buf, int len, struct mt_select *ms)
{
	ms->ms_size += len;
	return (0);
}


/*
 * copy a message into a buffer
 */
static int
get_sel_msg_copy(char *buf, int len, struct mt_select *ms)
{
	memcpy(ms->ms_ptr, buf, len);
	ms->ms_ptr += len;
	return (0);
}


/*
 * compute the size of the selection
 */
static int
get_selection_size(struct header_data *hd, Seln_rank rank)
{
	struct mt_select sel;

	sel.ms_size = 0;

	/* Figure out the size of the currently selected messages.
	 * we do this by enumerating all the messages, filtering
	 * them through get_sel_msg, and the passing each message
	 * to mm_copymsg() which will use getl_sel_msg_size() to
	 * add the size to ms.  Got it?
	 */
	folder_methods.fm_enumerate(CURRENT_FOLDER(hd), get_sel_msg,
		get_sel_msg_size, &sel);

	return (sel.ms_size);

}


/*
 * process a request to "make" a selection.  make a local copy
 * of it so we can pass it on later.
 *
 * return 0 for failure, 1 for success
 */
int
make_selection(struct header_data *hd, Seln_rank rank)
{
	struct mt_select *ms = rank_to_ms(rank);

	if (! acquire_selection(rank)) {
		/* make sure we got the selection */
		return (0);
	}

	/* free the old buffer */
	if (ms->ms_buffer) {
		free(ms->ms_buffer);
	}

	/* get the size of the selection */
	ms->ms_size = get_selection_size(hd, rank);

	/* allocate the buffer */
	ms->ms_buffer = malloc(ms->ms_size);
	ms->ms_ptr = ms->ms_buffer;
	if (! ms->ms_buffer) {
		return (0);
	}

	/* copy in the bits for the message */
	folder_methods.fm_enumerate(CURRENT_FOLDER(hd), get_sel_msg,
		get_sel_msg_copy, ms);

	return (1);

}


int
acquire_selection(Seln_rank rank)
{
	struct mt_select *ms = rank_to_ms(rank);

	if (!ms->ms_haveselection) {
		ms->ms_haveselection = (int) seln_acquire(header_client, rank);
	}
	return (ms->ms_haveselection);
}


static void
reset_selection(Seln_rank rank)
{
	struct mt_select *ms = rank_to_ms(rank);

	if (ms->ms_buffer) {
		free(ms->ms_buffer);
		ms->ms_buffer = ms->ms_ptr = NULL;
	}
}


void
mt_lose_selections(struct header_data *hd)
{
	seln_yield_all();
}



void
mt_busy(
	Frame	frame,		
	int	busy,		/* TRUE to set busy */
	char	*message,	/* String to put in left footer of frame */
	int	all_frames)	/* TRUE to busy all view & compose windows */
{
	struct header_data *hd;
	struct view_window_list	*vwl;
	struct reply_panel_data	*rpd;

	if (!frame)
		return;

	/* If there is a Message set it on baseframe's left footer */
	if (message != NULL)
		(void)xv_set(frame, FRAME_LEFT_FOOTER, message, 0);
		
	/* Busy the main baseframe, and all view and compose frames */
	(void)xv_set(frame, FRAME_BUSY, busy, 0);

	if (all_frames) {
		hd = mt_get_header_data(frame);
		for (rpd = MT_RPD_LIST(hd); rpd != NULL;
							rpd=rpd->next_ptr)
			(void)xv_set(rpd->frame, FRAME_BUSY, busy, 0);

		for (vwl = hd->hd_vwl; vwl != NULL; vwl = vwl->vwl_next)
			(void)xv_set(vwl->vwl_frame, FRAME_BUSY, busy, 0);
	}
}


/*
 * We ran into an unrecovarable error.  Print a message and exit.
 * don't even try and clean up -- there has been some sort of
 * internal error
 */
void
mt_abort(char *msg, int arg1, int arg2, int arg3, int arg4)
{
	fprintf(stderr, gettext("%s: Fatal error: aborting...\n"), mt_cmdname);
	fprintf(stderr, msg, arg1, arg2, arg3, arg4);

#ifdef DEBUG
	abort();
#endif
	exit(1);
}

static int
size_msg(char *data, int size, int *lenp)
{
	*lenp += size;
	return(0);
}


static int
size_msg_conv(char *data, int size, int *lenp)
{
        if (glob.g_iconvinfo != NULL)
                cs_methods.cs_copy2(glob.g_iconvinfo, size_msg, data,
                                size, (int)lenp);
        else
		*lenp += size;

	return(0);
}

static int
output_msg(char *data, int size, char **ptr)
{
	memcpy(*ptr, data, size);
	*ptr += size;
	return(0);
}

static int
output_msg_conv(char *data, int size, char **ptr)
{
        if (glob.g_iconvinfo != NULL)
                cs_methods.cs_copy2(glob.g_iconvinfo, output_msg, data,
                                size, (int)ptr);

        else {
		memcpy(*ptr, data, size);
		*ptr += size;
	}
	return(0);
}

static void
drag_headermsg(struct header_data *hd)
{

	Attach_list *al;
	struct msg *new;
	struct msg *m;
	void *data;
	char *tmp;
	int len;
	struct attach *at;
	Attach_node *an;
	extern void mt_drag_headermsg_cleanup();
	char    *intcode;

	new = msg_methods.mm_create(1);
	if (!new) {
		return;
	}

	al = mt_create_attach_list(hd->hd_frame, hd->hd_frame, new,
		hd->hd_frame);

	if (! al) {
		msg_methods.mm_destroy(new);
		return;
	}

	mt_init_attach_list(al, hd->hd_frame, hd->hd_canvas, NULL);

	/* OK.  we're going to run through each of the messages, and
	 * add all the selected ones to the message as an attachment.
	 * all those attachments will be marked as "selected", and then
	 * we will pass it to mt_drag_attachment() as if we were doing
	 * a normal dnd.
	 *
	 * buckle your seatbelts folks: we're in for quite a ride!
	 */

	for (m = FIRST_NDMSG(hd->hd_folder); m; m = NEXT_NDMSG(m)) {
		if (m->mo_selected) {
			/* get an in memory copy of the message... */
			len = 0;
			glob.g_iconvinfo = cs_methods.cs_translation_required(m, &intcode);
			msg_methods.mm_copymsg(m, 0, size_msg_conv, (int) &len, 1);

			/* allocate a buffer */
			data = ck_zmalloc(len);
			if (! data) goto cleanup;
			tmp = data;
			msg_methods.mm_copymsg(m, 0, output_msg_conv, (int) &tmp, 1);

			/* create an at and an */
			at = attach_methods.at_create();
			if (!at) goto cleanup;
			an = mt_get_attach_node(at);
			if (!an) goto cleanup;

			/* needed to fool mt_drag_attachment */
			an->an_selected = 1;

			/* set up the body of the message... */
			attach_methods.at_set(at, ATTACH_DATA_TYPE,
				"sun-deskset-message");
			attach_methods.at_set(at, ATTACH_MMAP_BODY, data);
			attach_methods.at_set(at, ATTACH_CONTENT_LEN, len);

			mt_init_attach_node(NULL, an, at, NULL);

			msg_methods.mm_set(new, MSG_ATTACH_BODY,	
				at, (void *) MSG_LAST_ATTACH);
			
		}
	}

	/* we've now got a message: pass it into the normal dnd code */
	mt_drag_attachment(al, new, hd->hd_copy, 0,
		mt_drag_headermsg_cleanup, al);
	return;

cleanup:
	/* OK, what now? */
	return;

}


char *
mt_strip_leading_blanks(char *s)
{
	/* 
	 * Return NULL string if NULL.
	 * Return empty string if empty string,
	 * because xv_set hates NULL strings
	 */
	if (!s) return (NULL);
	while (1) {
		if (!*s) return (s);
		if (*s == ' ') 
			s++;
		else
			return (s);
	}
}

char *
mt_strip_trailing_blanks(char *s)
{
	char *t;

	/* 
	 * Return NULL string if NULL.
	 * Return empty string if empty string,
	 * else returns string with trailing spaces removed.
	 */
	if ((!s) || (!*s)) return (s);
	t = s;

	/* Get to end of string */
	while (*t)
		t++;
	t--;

	/* Go backwards and skip all spaces */
	while (*t == ' ')
		t--;
	t++;
	*t = '\0';
	return (s);
}


Menu
mt_create_bogus_menu(struct header_data *hd)
{

	/*
	 * For each multiple frames, create a menu
	 * to pass in to different callbacks
	 */
        if (! hd->hd_bogus_menu) { 
		hd->hd_bogus_menu = xv_create(NULL, MENU,  
                              XV_KEY_DATA, KEY_HEADER_DATA, hd, 
                              0);
	}
	return (hd->hd_bogus_menu);
}


/*
 * the hd into the menuitem will prob. not be used by anyone
 */
Menu_item
mt_create_bogus_menu_item(struct header_data *hd)
{
        if (! hd->hd_bogus_item) { 
		hd->hd_bogus_item = xv_create(NULL, MENUITEM,  
                              XV_KEY_DATA, KEY_HEADER_DATA, hd, 
                              0);
	}
	return (hd->hd_bogus_item);
}
