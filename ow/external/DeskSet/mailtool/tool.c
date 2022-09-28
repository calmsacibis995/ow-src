#ifndef lint
static  char sccsid[] = "@(#)tool.c 3.41 96/06/19 Copyr 1985 Sun Micro";
#endif

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
 * Mailtool - tool creation, termination, handling
 */

#include <stdio.h>
#include <errno.h>
#include <signal.h>
#include <ctype.h>
#include <euc.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/param.h>

#include <xview/sel_svc.h>
#include <xview/scrollbar.h>
#include <xview/panel.h>
#include <xview/text.h>
#include <xview/font.h>
#include <xview/xview.h>
#include <xview/svrimage.h>
#include <xview/cms.h>
#include <xview/notice.h>
#include <xview/dragdrop.h>

#include <X11/Xlib.h>
#include <X11/Xutil.h>

#include "glob.h"
#include "tool.h"
#include "tool_support.h"
#include "graphics.h"
#include "mail.h"
#include "instrument.h"
#include "header.h"
#include "attach.h"
#include "create_panels.h"
#include "mle.h"
#include "destruct.h"
#include "delete_log.h"

#include "../maillib/ck_strings.h"
#include "../maillib/global.h"

#define DEBUG_FLAG mt_debugging
extern int DEBUG_FLAG;
#include "debug.h"


unsigned short   mt_mail_image[256] = {
#include "mail.icon"
};

unsigned short   mt_mail_image_mask[256] = {
#include "mail.mask.icon"
};

unsigned short   mt_nomail_image[256] = {
#include "nomail.icon"
};

unsigned short   mt_nomail_image_mask[256] = {
#include "nomail.mask.icon"
};

unsigned short   mt_emptymail_image[256] = {
#include "emptymail.icon"
};

unsigned short   mt_emptymail_image_mask[256] = {
#include "emptymail.mask.icon"
};

unsigned short   mt_reply_image[256] = {
#include "reply.icon"
};

unsigned short   mt_reply_image_mask[256] = {
#include "reply.mask.icon"
};

unsigned short   mt_compose_image[256] = {
#include "compose.icon"
};

unsigned short   mt_compose_image_mask[256] = {
#include "compose.mask.icon"
};

unsigned short   mt_unknown_image[256] = {
#include "dead.icon"
};

unsigned short   mt_unknown_image_mask[256] = {
#include "dead.mask.icon"
};


int KEY_HEADER_DATA;

static Notify_client mt_mailclient;


Frame	mt_frame;
struct header_data	*mt_header_data_list;

int ignore_exists = FALSE;

Xv_Font	mt_font;		/* the default font */
char	*mt_namestripe_right;
static int	mt_aboveline;
static  int	mt_headerlines;		/* lines of headers */
	int	mt_popuplines;		/* lines for popup composition window */
	int	mt_tool_width;		/* columns for the tool */

extern	char *mt_cmdline_args;

struct msg *mt_prevmsg;
int	mt_idle;
int	mt_nomail;
int	mt_iconic;
int	mt_system_mail_box;	/* true if current folder is system mail box */
int	mt_use_alerts, mt_debugging;
int	mt_bells, mt_flashes;
int	mt_destroying;
char	mt_multibyte;		/* TRUE if in a multibyte locale */
char	*mt_load_from_folder;
int 	(*default_textsw_notify)();
Xv_Font	textsw_font;


/* Variables for max message size */
long    mt_msgsizewarning = 200 * 1024;
long    mt_msgsizelimit = 0;


extern	Panel	panel1;
extern	Canvas	canvas1;
extern	int	mt_usersetsize;
extern int	mt_glyphheight;

int	mt_marginsize;

Server_image	compose_image, compose_image_mask;
Server_image	unknown_image, unknown_image_mask;
Server_image	mail_image, mail_image_mask;
Server_image	nomail_image, nomail_image_mask;
Server_image	emptymail_image, emptymail_image_mask;
Server_image	reply_image, reply_image_mask;

extern	Menu	mt_edit_menu, mt_file_menu, mt_view_menu, mt_compose_menu;

extern	struct	view_window_list *mt_create_new_view();

int	mt_memory_maximum;

static Notify_value mt_itimer(Notify_client client, int which);
static Notify_value mt_itimer_noce(Notify_client client, int which);
static Notify_value mt_destroy(Notify_client client, Destroy_status status);
static Notify_value mt_signal_func(Notify_client client, int sig);
static Notify_value mt_panel_event_proc(Notify_client client, Event *event,
	Notify_arg arg, Notify_event_type when);
static Notify_value mt_frame_event_proc(Notify_client client, Event *event,
	Notify_arg arg, Notify_event_type when);
static int compose_windows_modified(struct header_data *hd);
static void destroy_compose_windows(void);
static void stop_timer(void);
static void frame_layout_proc(struct header_data *hd);
static void save_state(struct header_data *hd);
static void initialize_state(struct header_data *hd);
static int set_menu_default(Menu menu, int default_item);
static void header_canvas_event_proc(Window canvas, Event *event,
	Notify_arg arg);
static Notify_value mt_handle_keys_proc(Window window, Event *event,
	Notify_arg arg);

static int KEY_FRAME_MSG;

/*
 * Icons
 */

Icon	mt_unknown_icon;

extern int layout_reply_panel();
extern int mt_header_interpose_proc();
extern int mt_glyph_interpose_proc();
extern Notify_value mt_canvas_interpose_proc();
extern int command_panel_help_interpose_proc();
extern int message_sw_help_interpose_proc();
extern void mt_header_seln_func_proc();
extern Seln_result mt_header_seln_reply_proc();
extern Seln_client header_client;
extern int	mt_tt_flag;

Icon mt_current_icon;


static void
mt_init_icons(void)
{

        mail_image = xv_create(0, SERVER_IMAGE,
                SERVER_IMAGE_BITS, mt_mail_image,
                SERVER_IMAGE_DEPTH, 1,
                XV_WIDTH, 64,
                XV_HEIGHT, 64,
                0);

        mail_image_mask = xv_create(0, SERVER_IMAGE,
                SERVER_IMAGE_BITS, mt_mail_image_mask,
                SERVER_IMAGE_DEPTH, 1,
                XV_WIDTH, 64,
                XV_HEIGHT, 64,
                0);

        nomail_image = xv_create(0, SERVER_IMAGE,
                SERVER_IMAGE_BITS, mt_nomail_image,
                SERVER_IMAGE_DEPTH, 1,
                XV_WIDTH, 64,
                XV_HEIGHT, 64,
                0);

        nomail_image_mask = xv_create(0, SERVER_IMAGE,
                SERVER_IMAGE_BITS, mt_nomail_image_mask,
                SERVER_IMAGE_DEPTH, 1,
                XV_WIDTH, 64,
                XV_HEIGHT, 64,
                0);

        emptymail_image = xv_create(0, SERVER_IMAGE,
                SERVER_IMAGE_BITS, mt_emptymail_image,
                SERVER_IMAGE_DEPTH, 1,
                XV_WIDTH, 64,
                XV_HEIGHT, 64,
                0);

        emptymail_image_mask = xv_create(0, SERVER_IMAGE,
                SERVER_IMAGE_BITS, mt_emptymail_image_mask,
                SERVER_IMAGE_DEPTH, 1,
                XV_WIDTH, 64,
                XV_HEIGHT, 64,
                0);

        reply_image = xv_create(0, SERVER_IMAGE,
                SERVER_IMAGE_BITS, mt_reply_image,
                SERVER_IMAGE_DEPTH, 1,
                XV_WIDTH, 64,
                XV_HEIGHT, 64,
                0);

        reply_image_mask = xv_create(0, SERVER_IMAGE,
                SERVER_IMAGE_BITS, mt_reply_image_mask,
                SERVER_IMAGE_DEPTH, 1,
                XV_WIDTH, 64,
                XV_HEIGHT, 64,
                0);

        compose_image = xv_create(0, SERVER_IMAGE,
                SERVER_IMAGE_BITS, mt_compose_image,
                SERVER_IMAGE_DEPTH, 1,
                XV_WIDTH, 64,
                XV_HEIGHT, 64,
                0);

        compose_image_mask = xv_create(0, SERVER_IMAGE,
                SERVER_IMAGE_BITS, mt_compose_image_mask,
                SERVER_IMAGE_DEPTH, 1,
                XV_WIDTH, 64,
                XV_HEIGHT, 64,
                0);

        unknown_image = xv_create(0, SERVER_IMAGE,
                SERVER_IMAGE_BITS, mt_unknown_image,
                SERVER_IMAGE_DEPTH, 1,
                XV_WIDTH, 64,
                XV_HEIGHT, 64,
                0);

        unknown_image_mask = xv_create(0, SERVER_IMAGE,
                SERVER_IMAGE_BITS, mt_unknown_image_mask,
                SERVER_IMAGE_DEPTH, 1,
                XV_WIDTH, 64,
                XV_HEIGHT, 64,
                0);


	mt_unknown_icon = xv_create(0,	ICON,
		ICON_IMAGE, unknown_image,
		ICON_MASK_IMAGE, unknown_image_mask,
		ICON_TRANSPARENT, TRUE,
		XV_LABEL, name_mailtool,
		0);

	/* Initialize the default icons used with attachments */
	mt_init_default_attach_icons();
	
}

void
mt_init_tool_storage(void)
{
	/* Initialize data */
	mt_namestripe_right = (char *)malloc(256);
	mt_namestripe_right[0] = '\0';
	mt_idle = TRUE;
	mt_iconic = TRUE;
	mt_nomail = TRUE;
	mt_use_alerts = TRUE;
	mt_debugging = FALSE;
	mt_destroying = FALSE;
	mt_load_from_folder = NULL;

	/* initialize all the cursors */

	mt_build_cursors();

	/* initialize icon structures */

	mt_init_icons();

	mt_current_icon = mt_unknown_icon;
}


static void
blast_selection_box(struct header_data *hd, struct msg *msg)
{
	GC gc;
	int box_x = mt_marginsize -1;
        int line;

	line = msg->m_lineno;

	if (msg->mo_selected) {
		gc = hd->hd_gc;
	} else {
		gc = hd->hd_cleargc;
	}

	XDrawRectangle(hd->hd_display, hd->hd_drawable, gc,
		box_x, (line -1) * hd->hd_lineheight + 1,
		(int) xv_get(hd->hd_paintwin, WIN_WIDTH) - box_x + 3,
			hd->hd_lineheight - 1);

	msg->mo_boxed = msg->mo_selected;
}

void
mt_draw_selection_box(struct header_data *hd, struct msg *msg)
{
	if (msg->mo_selected != msg->mo_boxed) {

		blast_selection_box(hd, msg);
	}
}


/* repaint header window */
void
mt_repaint_headers(Canvas canvas, Pixwin *pw, Rectlist *area, int clear)
{
	register int	line;
	register int	lines;
	register int	base;
	int	top;
	int	height;
	struct msg *msg;
	char	*s_p;
	Rectlist rl;
	struct header_data *hd;
	XRectangle  xrect;

	DP(("mt_repaint_headers: clear %d\n", clear));

	hd = mt_get_header_data(canvas);

	/* Not to repaint the header window because the header data may
	 * have faked data, especially when it is used for viewing
	 * recursive message.  See mt_set_attach_hd().
	 * Also don't repaint if we haven't been mapped yet.
	 */
	if (hd->hd_lineheight == 0 || !hd->hd_framemapped)
		return;

	if (area == NULL)
	{
		/* if area is not specified, repaint the viewable portion */
		(void) rl_initwithrect ((Rect *) xv_get (canvas,
			CANVAS_VIEWABLE_RECT,hd->hd_paintwin), &rl);
		area = &rl;
	}

	pw = (Pixwin *) xv_get(canvas, CANVAS_NTH_PAINT_WINDOW, 0);

	top = area->rl_bound.r_top;
	height = area->rl_bound.r_height;

	line = (top / hd->hd_lineheight) + 1;

	lines = height / hd->hd_lineheight;

	if (top % hd->hd_lineheight)
		lines++;

	if ((top + height) % hd->hd_lineheight)
		lines++;

	/* set index to entry for right starting line */
	if ((msg = LINE_TO_MSG(hd, line)) == NULL)
		return;

	xrect.x = area->rl_bound.r_left;
	xrect.y = area->rl_bound.r_top;
	xrect.width = area->rl_bound.r_width;
	xrect.height = area->rl_bound.r_height + 1;
	if (clear) {
		XFillRectangle(hd->hd_display, hd->hd_drawable,
			hd->hd_cleargc,
			xrect.x, xrect.y, xrect.width, xrect.height);
	}
	XSetClipRectangles(hd->hd_display, hd->hd_gc, 0, 0, 
		&xrect, 1, Unsorted);

	for (; lines > 0; lines--, line++) {
		/* write the line */

		if ((s_p = msg->m_header) == NULL)
			continue;

		base = ((line - 1) * hd->hd_lineheight) + 1;

		mt_draw_glyph(msg, hd, 1, base + 1);
#ifdef OW_I18N
		if (mt_multibyte) {
			XmbDrawString(hd->hd_display, hd->hd_drawable,
				hd->hd_textfontset,
				hd->hd_gc, mt_marginsize,
					base + 1 + mt_aboveline,
				s_p, strlen(s_p));
    DP(("mt_repaint_headers: hd %x, hd->hd_fontid %x, hd->file_fillin %x\n",
				hd, hd->hd_fontid, hd->hd_file_fillin));
			XSetFont(hd->hd_display, hd->hd_gc, hd->hd_fontid);
		} else {
			mt_draw_text(hd->hd_display, hd->hd_drawable, hd->hd_gc,
				mt_marginsize, base + 1 + mt_aboveline, s_p,
				strlen(s_p));

		}
#else
		mt_draw_text(hd->hd_display, hd->hd_drawable, hd->hd_gc,
			mt_marginsize, base + 1 + mt_aboveline, s_p,
			strlen(s_p));
#endif

		blast_selection_box(hd, msg);

		/* Get next non-deleted message. */
		msg = NEXT_NDMSG( msg );
		if (msg == NULL) break;
	}

        XSetClipMask(hd->hd_display, hd->hd_gc, None);
        XSync(hd->hd_display, 0);
}



/* repaint header window */
static void
header_repaint_proc(Canvas canvas, Pixwin *pw, Rectlist *area)
{
	mt_repaint_headers(canvas, pw, area, TRUE);
}




void
mt_clear_header_canvas(struct header_data *hd)

{
	XFillRectangle(hd->hd_display, hd->hd_drawable, hd->hd_cleargc, 0, 0,
		(int)xv_get(hd->hd_canvas, CANVAS_WIDTH),
		(int)xv_get(hd->hd_canvas, CANVAS_HEIGHT));
}





void
scroll_set_view_len(struct header_data *hd, int pos, unsigned long *objlen_ptr)
{
	int win_lines;
	int num_lines;
	struct msg *m;
	int viewlen;
	int objlen;

	win_lines = xv_get(hd->hd_canvas, XV_HEIGHT) / hd->hd_lineheight;

	m = LAST_NDMSG(hd->hd_folder);
	if (m) {
		objlen = m->m_lineno;
		num_lines = objlen - pos;
		if (objlen_ptr) *objlen_ptr = objlen;
	} else {
		objlen = 1;
		num_lines = win_lines;
	}


	viewlen = win_lines < num_lines ? win_lines : num_lines;
	if (viewlen < 1) viewlen = 1;

	if (viewlen >= hd->hd_maxlines) {
		viewlen = hd->hd_maxlines;
	}

	if (objlen > hd->hd_maxlines) objlen = hd->hd_maxlines;
	if (viewlen > hd->hd_maxlines) viewlen = hd->hd_maxlines;


	if (objlen_ptr) {
		xv_set(hd->hd_scrollbar,
			SCROLLBAR_VIEW_LENGTH, viewlen,
			SCROLLBAR_OBJECT_LENGTH, objlen,
			0);
	} else {
		xv_set(hd->hd_scrollbar,
			SCROLLBAR_VIEW_LENGTH, viewlen,
			SCROLLBAR_VIEW_START, pos,
			SCROLLBAR_OBJECT_LENGTH, objlen,
			0);
	}

DP(("set_view_len: pos %d, win_lines %d, num_lines %d, viewlen %d, objlen %d\n",
		pos, win_lines, num_lines, viewlen, objlen));

}


static void
scrollbar_compute_proc(Scrollbar sb, int pos, int length,
	Scroll_motion motion, unsigned long *offset, unsigned long *objlen)
{
	int start;
	struct header_data *hd;
	int win_lines;
	int num_lines;
	struct msg *m;

#ifdef DEBUG
	char *motstring;
	switch (motion) {
	case SCROLLBAR_ABSOLUTE:	motstring = "absolute";		break;
	case SCROLLBAR_POINT_TO_MIN:	motstring = "point_to_min";	break;
	case SCROLLBAR_PAGE_FORWARD:	motstring = "page_forward";	break;
	case SCROLLBAR_LINE_FORWARD:	motstring = "line_forward";	break;
	case SCROLLBAR_MIN_TO_POINT:	motstring = "min_to_point";	break;
	case SCROLLBAR_PAGE_BACKWARD:	motstring = "page_backward";	break;
	case SCROLLBAR_LINE_BACKWARD:	motstring = "line_backward";	break;
	case SCROLLBAR_TO_END: 		motstring = "to_end";		break;
	case SCROLLBAR_TO_START:	motstring = "to_start";		break;
	case SCROLLBAR_PAGE_ALIGNED:	motstring = "page_aligned";	break;
	case SCROLLBAR_NONE:		motstring = "none";		break;
	default:			motstring = "???";		break;
	}
#endif DEBUG

	hd = mt_get_header_data(sb);

	DP(("scrollbar_compute_proc(%x, %d, %d, %d/%s, %d, %d)\n",
		sb, pos, length, motion, motstring, *offset, *objlen));

	start = xv_get(sb, SCROLLBAR_VIEW_START);
	win_lines = xv_get(hd->hd_canvas, XV_HEIGHT) / hd->hd_lineheight;
	m = LAST_NDMSG(hd->hd_folder);
	num_lines = m ? m->m_lineno : 0;


	switch (motion) {
	case SCROLLBAR_LINE_FORWARD:
		DP(("line_forward: start %d\n", start));

		if (++start >= num_lines) {
			/* oops -- we don't want to go off screen */
			start--;
		}

		*offset = start;
		break;

	case SCROLLBAR_LINE_BACKWARD:
		DP(("line_backward: start %d\n", start));

		if (start > 0) start--;

		/* keep one line on the screen... */
		if (start > num_lines -1) start = num_lines -1;

		*offset = start;
		break;

	case SCROLLBAR_PAGE_BACKWARD:
		DP(("page_backward: start %d\n", start));

		start -= win_lines;
		if (start < 0) start = 0;

		*offset = start;
		break;

	case SCROLLBAR_PAGE_FORWARD:
		DP(("page_forward: start %d\n", start));

		start += win_lines;
		if (start > num_lines + win_lines -1) {
			start = num_lines + win_lines -1;
		}
		*offset = start;
		break;

	case SCROLLBAR_TO_END:
		start = num_lines - win_lines/2;
		if (start < 0) start = 0;
		*offset = start;
		break;

	default:
		(*hd->hd_old_compute_proc)(sb, pos, length, motion,
			offset, objlen);
		break;
	}

	scroll_set_view_len(hd, *offset, objlen);
	/* 
	 * For some reason in xview, if *objlen is 1,
	 * and *offset is > 0, we lose the only line.
	 * So for now, we won't let user scroll if there's only 
	 * one line.
	 */
	if (*objlen == 1)
		*offset = 0;

	DP(("scrollbar_compute_proc: after offset %d, objlen %d\n",
		*offset, *objlen));

}



static void
init_header_drop_site(struct header_data *hd)
{
	void *msg;

	msg = msg_methods.mm_create(1);
	if (! msg) {
failure:
		mt_vs_warn(hd->hd_frame, gettext(
"Internal error: could not allocate a drop site for main window.\n\
This means that you cannot drop folders on this window\n"));
		return;
	}

	hd->hd_al = mt_create_attach_list(hd->hd_frame, hd->hd_frame,
		msg, hd->hd_frame);

	if (! hd->hd_al) {
		msg_methods.mm_destroy(msg);
		goto failure;
	}

	mt_init_attach_list(hd->hd_al, hd->hd_frame, hd->hd_canvas, NULL);


	/* set up the drop site */
	hd->hd_dropsite = xv_create(hd->hd_canvas, DROP_SITE_ITEM,
		DROP_SITE_ID, 0,
		DROP_SITE_EVENT_MASK, DND_MOTION | DND_ENTERLEAVE,
		0);

	if (! hd->hd_dropsite) {
		mt_destroy_attach_list(hd->hd_al);
		msg_methods.mm_destroy(msg);
		goto failure;
	}

}


void
mt_cleanup_proc(int pid)
{
	(void)notify_set_wait3_func(mt_mailclient, notify_default_wait3, pid);
}





void
mt_finish_initializing(Frame frame)
{
	XFontStruct	*x_font_info;
	Xv_Font		mt_get_fixed_font();
	int		textsw_charwidth, textsw_charheight;
	void		mt_done_signal_proc();
	Xv_opaque	paintwin;
	struct		header_data *hd;
		
	hd = ck_malloc(sizeof (struct header_data));
	memset((void *)hd, '\0', sizeof(*hd));
	hd->hd_next = NULL;
	hd->hd_frame = frame;
	hd->hd_sys_button_list = NULL;
	hd->hd_binding_list = NULL;
	hd->hd_last_binding = NULL;
	hd->hd_bogus_menu = NULL;
	hd->hd_bogus_item = NULL;
	hd->hd_framemapped = FALSE;   /* Goes to TRUE as soon as we seen
					WIN_VISIBILITY_NOTIFY */

	/* have global list to point to first hd struct */
	mt_header_data_list = hd;

	/* This replaces global mt_folder */
	MT_FOLDER(hd) = calloc(256, sizeof (char));
	(void)strcpy(MT_FOLDER(hd), name_none);

	/* cache one view window */
	hd->hd_cache_vw = TRUE;

	mt_font = (Xv_Font)xv_get(frame, XV_FONT);

	mt_multibyte = multibyte; /* TRUE if we are in a multibyte locale */

	/* enters mt_frame in array of windows */

	/*
	 * Create the command panel.
	 */
        /* STRING_EXTRACTION -
         *
         * The error message if we cannot create a new panel.
         */
	hd->hd_cmdpanel = xv_create(frame, PANEL,
		WIN_ERROR_MSG, gettext("mailtool: unable to create panel\n"),
		XV_NAME,	name_Mail_Tool,
		WIN_CLIENT_DATA, 0,
		OPENWIN_SHOW_BORDERS, FALSE,
		XV_WIDTH,	20,	/* 20 is arbitrary; we'll size later */
		0);

	mt_set_header_data(hd);

        /*
         * Set ignore_free, we automatically had set alias_free via
         * the call #-clearaliases at startup and at props reset.
         */
        ignore_exists = FALSE;
        ignore_free();

        /* mt_init_mailtool_defaults
         * calls load() which calls commands() which
         * reads in .mailrc again and sets each variable FOUND
         * into internal memory/ hash table
         * (probably calls mt_assign)
         */
        mt_init_mailtool_defaults();


	mt_create_control_panel(hd);

	xv_set(hd->hd_cmdpanel, WIN_EVENT_PROC, mt_handle_keys_proc, 0);
        notify_interpose_event_func(hd->hd_cmdpanel, mt_panel_event_proc,
		NOTIFY_SAFE);

	if (! mt_usersetsize) {
		window_fit_height(frame);
	}

	/*
	 * Create the header canvas.
	 */
	hd->hd_canvas = xv_create(frame, CANVAS,
		CANVAS_REPAINT_PROC, 	header_repaint_proc,
		CANVAS_RETAINED, 	FALSE,
		CANVAS_AUTO_CLEAR,      FALSE,
		CANVAS_AUTO_SHRINK,     FALSE,
		CANVAS_AUTO_EXPAND,     TRUE,
		CANVAS_NO_CLIPPING,     TRUE, 
		CANVAS_FIXED_IMAGE,     TRUE,
#ifdef OW_I18N
		WIN_USE_IM,		FALSE,
#endif OW_I18N
		WIN_X,			0,
		WIN_BELOW,		hd->hd_cmdpanel,
		XV_HEIGHT,		WIN_EXTEND_TO_EDGE,
		OPENWIN_ADJUST_FOR_VERTICAL_SCROLLBAR,	TRUE,
		XV_HELP_DATA,           "mailtool:MessageList",
		0);

	hd->hd_paintwin = xv_get(hd->hd_canvas, CANVAS_NTH_PAINT_WINDOW, 0);

	hd->hd_scrollbar = xv_create(hd->hd_canvas, SCROLLBAR, 0);

	if (hd->hd_scrollbar) {
		hd->hd_old_compute_proc = (void (*)())
			xv_get(hd->hd_scrollbar, SCROLLBAR_COMPUTE_SCROLL_PROC);
		xv_set(hd->hd_scrollbar, SCROLLBAR_COMPUTE_SCROLL_PROC,
			scrollbar_compute_proc, 0);
	}


	mt_set_header_data(hd);

	xv_set(hd->hd_paintwin,
		WIN_EVENT_PROC,	mt_handle_keys_proc,
		WIN_CONSUME_PICK_EVENTS,
			LOC_DRAG,
			ACTION_DRAG_MOVE,
			ACTION_DRAG_COPY,
			LOC_DRAG,
			0,
		0);


	xv_set(hd->hd_canvas,
		WIN_EVENT_PROC, header_canvas_event_proc,
		0);

	/* new code: let the library get the font, instead of trying
	 * to outguess it
	 */
	textsw_font = NULL;

	{
#ifdef CODECENTER
		Frame tframe = xv_create(0, FRAME_BASE,
			WIN_IS_CLIENT_PANE,
			FRAME_SHOW_FOOTER,	TRUE,
			FRAME_SHOW_LABEL, TRUE,
			XV_SHOW, FALSE,
			0);
		

		Textsw textsw = xv_create(tframe, TEXTSW,
			TEXTSW_MEMORY_MAXIMUM, 200, /* Try to keep it small */
			WIN_USE_IM,	FALSE,		/* bug # 1152209 */
			WIN_IS_CLIENT_PANE, 0);
#else
		Textsw textsw = xv_create(frame, TEXTSW,
			TEXTSW_MEMORY_MAXIMUM, 200, /* Try to keep it small */
			WIN_USE_IM,	FALSE,		/* bug # 1152209 */
			WIN_IS_CLIENT_PANE, 0);
#endif

		/* we get the first view in the window; this is the
		 * first xview object that has the correct fixed
		 * width font that is going to be used in the view
		 * window.  We need this font to use in the
		 * header canvas
		 */
		if (textsw) {
			Xv_window xvw = (Xv_window)
				xv_get(textsw, OPENWIN_NTH_VIEW, 0);

			textsw_font = (Xv_Font) xv_get(xvw, XV_FONT);

			/* this seems to be used later, even though we never
			 * refer to it directly?!
			 * See bug # 1046734
			 */
#ifndef CODECENTER
			/* xview references free'd memory; this upsets
			 * codecenter...
			 */
			xv_destroy_safe(textsw);
#endif
		}
	}

	if (!textsw_font) {
		/* we're probably going to die momentarily; we
		 * either could not get a view window, or it did not
		 * have a font.  However, until we die just use the cmd
		 * window font
		 */
		 textsw_font = (Xv_Font) xv_get(hd->hd_cmdpanel, XV_FONT);
	}

	hd->hd_textfont = textsw_font;

#ifdef OW_I18N
        hd->hd_textfontset = (Xv_opaque)xv_get(hd->hd_textfont, FONT_SET_ID);
#endif OW_I18N


	textsw_charwidth = xv_get(textsw_font, FONT_COLUMN_WIDTH);
	textsw_charheight = xv_get(textsw_font, FONT_DEFAULT_CHAR_HEIGHT);

	x_font_info = (XFontStruct *)xv_get(textsw_font, FONT_INFO);
	hd->hd_lineheight = textsw_charheight + 2;
	mt_glyphheight = hd->hd_lineheight - 2;
	mt_marginsize = 3 * textsw_charwidth - textsw_charwidth / 2;
	mt_aboveline = x_font_info->ascent;

	/* ZZZ/katin: the 30000 is a voodoo constant in x11/news, and there
	 * is no way that I know of to query for it.  Sigh.
	 */
	hd->hd_maxlines = (30000 / hd->hd_lineheight) -1;

	DP(("mt_finish_initializing: hd_maxlines = %d\n", hd->hd_maxlines));

	mt_init_graphics(hd);

	xv_set(xv_get(hd->hd_canvas, WIN_VERTICAL_SCROLLBAR),
		SCROLL_LINE_HEIGHT, hd->hd_lineheight, 0);

	
	/* create the handle for linking the headers 
	 * into the selection service
	 */

	header_client =
		seln_create(mt_header_seln_func_proc,
			mt_header_seln_reply_proc, (char *) hd);
			

	xv_set(hd->hd_canvas,
		WIN_MENU, generate_headers_popup(hd),
		0);

	mt_undel_list_init(hd);

	xv_set(hd->hd_canvas, WIN_CONSUME_PICK_EVENTS, LOC_WINEXIT, 0, 0);



	/* layout the frame */
	frame_layout_proc(hd);


	/* set up a unique client - used to be mt_frame */
	mt_mailclient = (Notify_client)&(hd->hd_frame);

	/*
	 * Catch signals, install tool.
	 */
	(void)notify_set_signal_func(mt_mailclient, mt_signal_func,
		SIGTERM, NOTIFY_ASYNC);
	(void)notify_set_signal_func(mt_mailclient, mt_signal_func,
		SIGHUP, NOTIFY_ASYNC);
	(void)notify_set_signal_func(mt_mailclient, mt_signal_func,
		SIGXCPU, NOTIFY_ASYNC);
	(void)notify_set_signal_func(mt_mailclient, mt_signal_func,
		SIGQUIT, NOTIFY_ASYNC);
#ifndef DEBUG
	(void)notify_set_signal_func(mt_mailclient, mt_signal_func,
		SIGINT, NOTIFY_ASYNC);
#endif DEBUG

	(void)notify_set_signal_func(mt_mailclient,
		(Notify_func) mt_done_signal_proc,
		SIGUSR1, NOTIFY_SYNC);

	(void)signal(SIGPIPE, SIG_IGN);

	/*
	 * Kludge - we want the notifier to harvest all dead children
	 * so we tell it to wait for its own death.
	 */

	mt_cleanup_proc(getpid());

	notify_interpose_event_func(hd->hd_paintwin,
		mt_canvas_interpose_proc, NOTIFY_SAFE);

	initialize_state(hd);

	/* to have the tool come up open in a folder */
	if (mt_load_from_folder) {
		mt_new_folder(hd, mt_load_from_folder, FALSE, FALSE, FALSE, TRUE);
	}

	init_header_drop_site(hd);

	/* set timer intervals */
	mt_start_timer();

	/* catch going from icon to window */
	notify_interpose_event_func(frame, mt_frame_event_proc, NOTIFY_SAFE);

	/* catch when being destroyed */
	(void) notify_interpose_destroy_func(frame, mt_destroy);

	if (!mt_load_from_folder) {
		/* start up with correct icon */
		mt_check_mail_box(hd, TRUE);
	}
}


#ifdef MULTIPLE_FOLDERS

struct header_data *
mt_finish_initializing1(frame)

	Frame	frame;
{
	XFontStruct	*x_font_info;
	Xv_Font		mt_get_fixed_font();
	int		textsw_charwidth, textsw_charheight;
	void		mt_done_signal_proc();
	Xv_opaque	paintwin;
	struct		header_data *hd;
	struct		header_data *hd_ptr;
	Seln_client 	header_client1;
		
	hd = ck_malloc(sizeof (struct header_data));
	memset((void *)hd, '\0', sizeof(*hd));
	hd->hd_next = NULL;
	hd->hd_frame = frame;
	hd->hd_sys_button_list = NULL;
	hd->hd_binding_list = NULL;
	hd->hd_last_binding = NULL;
	hd->hd_bogus_menu = NULL;
	hd->hd_bogus_item = NULL;

	/* 
	 * Insert into global hd list. 
	 * There should be at least one hd_ptr 
	 */
	for (hd_ptr = mt_header_data_list; hd_ptr->hd_next != NULL;
              hd_ptr = hd_ptr->hd_next);
	hd_ptr->hd_next = hd;

	/* This replaces global mt_folder */
	/* set by mt_set_folder, later called by mt_new_folder1 */
	MT_FOLDER(hd) = calloc(256, sizeof (char));
	(void)strcpy(MT_FOLDER(hd), name_none);

	/* cache one view window */
	hd->hd_cache_vw = TRUE;

	/*
	 * Create the command panel.
	 */
        /* STRING_EXTRACTION -
         *
         * The error message if we cannot create a new panel.
         */
	hd->hd_cmdpanel = xv_create(frame, PANEL,
		WIN_ERROR_MSG, gettext("mailtool: unable to create panel\n"),
		XV_NAME,	name_Mail_Tool,
		WIN_CLIENT_DATA, 0,
		OPENWIN_SHOW_BORDERS, FALSE,
		XV_WIDTH,	20,	/* 20 is arbitrary; we'll size later */
		0);

	mt_set_header_data(hd);

        /*
         * Set ignore_free, we automatically had set alias_free via
         * the call #-clearaliases at startup and at props reset.
         */
        ignore_exists = FALSE;
        ignore_free();

        /* mt_init_mailtool_defaults
         * calls load() which calls commands() which
         * reads in .mailrc again and sets each variable FOUND
         * into internal memory/ hash table
         * (probably calls mt_assign)
         */
        mt_init_mailtool_defaults();


	mt_create_control_panel1(hd);

	xv_set(hd->hd_cmdpanel, WIN_EVENT_PROC, mt_handle_keys_proc, 0);
        notify_interpose_event_func(hd->hd_cmdpanel, mt_panel_event_proc,
		NOTIFY_SAFE);

	/* True if user set -Ws or -size */
	if (! mt_usersetsize) {
		window_fit_height(frame);
	}

	/*
	 * Create the header canvas.
	 */
	hd->hd_canvas = xv_create(frame, CANVAS,
		CANVAS_REPAINT_PROC, 	header_repaint_proc,
		CANVAS_RETAINED, 	FALSE,
		CANVAS_AUTO_CLEAR,      FALSE,
		CANVAS_AUTO_SHRINK,     FALSE,
		CANVAS_AUTO_EXPAND,     TRUE,
		CANVAS_NO_CLIPPING,     TRUE, 
		CANVAS_FIXED_IMAGE,     TRUE,
#ifdef OW_I18N
		WIN_USE_IM,		FALSE,
#endif OW_I18N
		WIN_X,			0,
		WIN_BELOW,		hd->hd_cmdpanel,
		XV_HEIGHT,		WIN_EXTEND_TO_EDGE,
		OPENWIN_ADJUST_FOR_VERTICAL_SCROLLBAR,	TRUE,
		XV_HELP_DATA,           "mailtool:MessageList",
		0);

	hd->hd_paintwin = xv_get(hd->hd_canvas, CANVAS_NTH_PAINT_WINDOW, 0);

	hd->hd_scrollbar = xv_create(hd->hd_canvas, SCROLLBAR, 0);

	if (hd->hd_scrollbar) {
		hd->hd_old_compute_proc = (void (*)())
			xv_get(hd->hd_scrollbar, SCROLLBAR_COMPUTE_SCROLL_PROC);
		xv_set(hd->hd_scrollbar, SCROLLBAR_COMPUTE_SCROLL_PROC,
			scrollbar_compute_proc, 
			0);
	}


	mt_set_header_data(hd);

	xv_set(hd->hd_paintwin,
		WIN_EVENT_PROC,	mt_handle_keys_proc,
		WIN_CONSUME_PICK_EVENTS,
			LOC_DRAG,
			ACTION_DRAG_MOVE,
			ACTION_DRAG_COPY,
			LOC_DRAG,
			0,
		0);


	xv_set(hd->hd_canvas,
		WIN_EVENT_PROC, header_canvas_event_proc,
		0);

	/* new code: let the library get the font, instead of trying
	 * to outguess it
	 */
	textsw_font = NULL;

	{
		Textsw textsw = xv_create(frame, TEXTSW,
			WIN_IS_CLIENT_PANE, 0);

		/* we get the first view in the window; this is the
		 * first xview object that has the correct fixed
		 * width font that is going to be used in the view
		 * window.  We need this font to use in the
		 * header canvas
		 */
		if (textsw) {
			Xv_window xvw = (Xv_window)
				xv_get(textsw, OPENWIN_NTH_VIEW, 0);

			textsw_font = (Xv_Font) xv_get(xvw, XV_FONT);

			/* this seems to be used later, even though we never
			 * refer to it directly?!
			 * See bug # 1046734
			 */
			xv_destroy_safe(textsw);
		}
	}

	if (!textsw_font) {
		/* we're probably going to die momentarily; we
		 * either could not get a view window, or it did not
		 * have a font.  However, until we die just use the cmd
		 * window font
		 */
		 textsw_font = (Xv_Font) xv_get(hd->hd_cmdpanel, XV_FONT);
	}

	hd->hd_textfont = textsw_font;

#ifdef OW_I18N
        hd->hd_textfontset = (void *)xv_get(hd->hd_textfont, FONT_SET_ID);
#endif OW_I18N


	textsw_charwidth = xv_get(textsw_font, FONT_COLUMN_WIDTH);
	textsw_charheight = xv_get(textsw_font, FONT_DEFAULT_CHAR_HEIGHT);

	x_font_info = (XFontStruct *)xv_get(textsw_font, FONT_INFO);
	hd->hd_lineheight = textsw_charheight + 2;

	/* ZZZ/katin: the 30000 is a voodoo constant in x11/news, and there
	 * is no way that I know of to query for it.  Sigh.
	 */
	hd->hd_maxlines = (30000 / hd->hd_lineheight) -1;

	DP(("mt_finish_initializing1: hd_maxlines = %d\n", hd->hd_maxlines));

	mt_init_graphics(hd);

	xv_set(xv_get(hd->hd_canvas, WIN_VERTICAL_SCROLLBAR),
		SCROLL_LINE_HEIGHT, hd->hd_lineheight, 0);

	
	/* create the handle for linking the headers 
	 * into the selection service
	 */

	header_client1 =
		seln_create(mt_header_seln_func_proc,
			mt_header_seln_reply_proc, (char *) hd);

	xv_set(hd->hd_canvas,
		WIN_MENU, generate_headers_popup(hd),
		0);
/*
	mt_undel_list_init(hd);
*/

	xv_set(hd->hd_canvas, WIN_CONSUME_PICK_EVENTS, LOC_WINEXIT, 0, 0);



	/* layout the frame */
	frame_layout_proc(hd);


	/* set up a unique client - used to be mt_frame */
/*
	mt_mailclient = (Notify_client)&(hd->hd_frame);
*/

	/*
	 * Catch signals, install tool.
	 */
/*
	(void)notify_set_signal_func(mt_mailclient, mt_signal_func,
		SIGTERM, NOTIFY_ASYNC);
	(void)notify_set_signal_func(mt_mailclient, mt_signal_func,
		SIGHUP, NOTIFY_ASYNC);
	(void)notify_set_signal_func(mt_mailclient, mt_signal_func,
		SIGXCPU, NOTIFY_ASYNC);
	(void)notify_set_signal_func(mt_mailclient, mt_signal_func,
		SIGQUIT, NOTIFY_ASYNC);
#ifndef DEBUG
	(void)notify_set_signal_func(mt_mailclient, mt_signal_func,
		SIGINT, NOTIFY_ASYNC);
#endif DEBUG

	(void)notify_set_signal_func(mt_mailclient,
		(Notify_func) mt_done_signal_proc,
		SIGUSR1, NOTIFY_SYNC);

	(void)signal(SIGPIPE, SIG_IGN);
*/
	/*
	 * Kludge - we want the notifier to harvest all dead children
	 * so we tell it to wait for its own death.
	 */
/*
	mt_cleanup_proc(getpid());
*/

	notify_interpose_event_func(hd->hd_paintwin,
		mt_canvas_interpose_proc, NOTIFY_SAFE);

/* lots of stuff here - read from .mailtool-init */
	initialize_state(hd);

	/* to have the tool come up open in a folder */
/*- this is called from mt_new_folder1
	if (mt_load_from_folder) {
		mt_new_folder(hd, mt_load_from_folder, FALSE, FALSE, FALSE, TRUE);
	}
*/

	init_header_drop_site(hd);

	/* set timer intervals */
	mt_start_timer();

	/* catch going from icon to window */
	notify_interpose_event_func(frame, mt_frame_event_proc, NOTIFY_SAFE);

	/* catch when being destroyed */
/* -have to create my own here just destroy one 
	(void) notify_interpose_destroy_func(frame, mt_destroy);
*/

/*
	if (!mt_load_from_folder) {
		/ * start up with correct icon * /
		mt_check_mail_box(hd, TRUE);
	}
*/
	return ((struct header_data *)hd);
}
#endif MULTIPLE_FOLDERS



/*
 * Build and start the tool.
 */
void
mt_start_tool(int argc, char **argv)
{
	int             sigwinched();
	int             ondeath();
	char		tool_label[50];
	static struct itimerval itv;
        struct header_data *hd;
	Frame		frame;
 
	mt_parse_tool_args(argc, argv);

	/*
	 * Create the tool.
	 */

	sprintf(tool_label, "%s - %s", name_Mail_Tool, name_none);

        /* STRING_EXTRACTION -
         *
         * another error message: we tried to create the base frame and
         * failed.
         */
	mt_frame = frame = xv_create(0, FRAME_BASE,
		WIN_IS_CLIENT_PANE,
		WIN_ERROR_MSG, gettext("mailtool: unable to create window\n"),
		XV_LABEL, tool_label,
		FRAME_SHOW_FOOTER,	TRUE,
		FRAME_SHOW_LABEL, TRUE,
		FRAME_CLOSED, (mt_iconic && !(int)mt_load_from_folder) ? TRUE : FALSE,
		FRAME_ICON, mt_current_icon,
		FRAME_ARGC_PTR_ARGV, &argc, argv,
		WIN_CMD_LINE, mt_cmdline_args,
		/* So we can get WIN_VISIBILITY_NOTIFY */
		WIN_CONSUME_X_EVENT_MASK, VisibilityChangeMask,
		0);

	mt_iconic = (int)xv_get(frame, FRAME_CLOSED);
	mt_finish_initializing(frame);

        hd = mt_get_header_data(frame);

	/* Complete tool talk initialization */
	mt_complete_tt_init(MT_FRAME(hd));
	mailtool_dstt_start(MT_FRAME(hd));
	dstt_set_app("MailTool");

	/*
	 * If we were started by -tooltalk then we come up unmapped
	 * and we don't check for new mail
	 */
	if (mt_tt_flag) {
		stop_timer();
		mt_start_self_destruct(atoi(mt_value("selfdestruct")));
		(void)notify_start();	/* Doesn't map baseframe */
	} else {
		xv_main_loop(MT_FRAME(hd));
	}

	/* this is a secret, awful flag to mt_stop_mail to not try and
	 * get the header_data, since the frame has already been destroyed...
	 * But we can pass or use hd, since we never explicitly destroy it
	 */
	mt_frame = NULL;
	mt_stop_mail(mt_aborting);
}

void
mt_done_signal_proc(void)
{
	struct header_data *hd;

	/* ZZZ: when we have multiple header frames, this should be
	 * a for loop that loops through all the frames...
	 */
	for (hd = mt_header_data_list; hd != NULL; hd = hd->hd_next) {
		mt_done_proc_hd(hd);
	}
}


static int
ignore_test_proc(char *key, char *value, int foo)
{
	ignore_exists = TRUE;
	return(1);
}


/*
 * resetting these variables after reading .mailrc will immediately affect
 * behavior of mailtool, so this procedure is called from both mt_start_tool
 * and from mt_mailrc_proc 
 */

void
mt_init_mailtool_defaults(void)
{
	long            atol();
	int             i;
	char           *p;
	char           *old_string;
	char           *new_string;
	char           *token;

	char		buf[MAXPATHLEN];
	extern char    *Getf();
	extern char    *libpath();

	if (! mt_value("MAIL")) {
#ifdef	SVR4
		sprintf(buf, "/var/mail/%s", glob.g_myname);
#else
		sprintf(buf, "/usr/spool/mail/%s", glob.g_myname);
#endif	SVR4
		mt_assign("MAIL", buf);
	}

	/* 
	 * The "save" variable is supposed to be set by default.
	 * I can't find any better place to set it, so here it will go.
	 */
	mt_assign("save", "");

#ifdef undef
	/* Once again we back out tooltalk locking by default... */
	/*
	 * ttlock is set by default so that mailtool uses ToolTalk to
	 * lock its mail files unless "set nottlock" is specified in
	 * one of mailtool's startup files.
	 */
	mt_assign("ttlock", "");
#endif undef

	/* load the personal startup file */
	load(Getf("MAILRC"));

	while ((p = strchr(mt_cmdname, '/')) != NULL)
		mt_cmdname = ++p;	/* strip off directories in
					 * name of command in order
					 * to conserve space in
					 * namestripe */

	mt_memory_maximum = TEXTSW_INFINITY;

	if (mt_value("selfdestruct") == NULL)
		mt_assign("selfdestruct", "86400");

	/* 
	 * Set max size of messages 
	 * msgsizewarning is threshold in getting a warning notice
	 * msgsizelimit is threshold in not delivering a mail message
	 */
	if (p = mt_value("msgsizewarning")) {
		long t;
		t = atol(p);

		if (t >= 0) {
			mt_msgsizewarning = t;
		}
	}

	if (p = mt_value("msgsizelimit")) {
		long t;
		t = atol(p);

		if (t >= 0) {
			mt_msgsizelimit = t;
		}
	}

	/*
	 * Get the sizes of the various subwindows.
	 */
	mt_headerlines = 15;
	mt_popuplines = 30;
	mt_tool_width = 80;
	if ((p = mt_value("headerlines")) && (i = atoi(p)) > 0)
		mt_headerlines = i;
	if ((p = mt_value("popuplines")) && (i = atoi(p)) > 0)
		mt_popuplines = i;
	if ((p = mt_value("toolcols")) && (i = atoi(p)) > 0)
		mt_tool_width = i;
	mt_bells = 0;
	mt_flashes = 0;
		if (p = mt_value("bell")) {
		mt_bells = atoi(p);
		if (mt_bells <= 0)
			mt_bells = 0;
	}
	if (p = mt_value("flash")) {
		mt_flashes = atoi(p);
		if (mt_flashes <= 0)
			mt_flashes = 0;
	}

	ignore_exists = FALSE;
	ignore_enumerate(ignore_test_proc, NULL);

        /* Set defaults only if there isn't any ignores and
         * variable ignorenothing is not set.
         * We used to have hard coded defaults for ignore,
         * but now they are a subset of the mail rc file below.
         * Since we are doing this here, we don't have to
         * do this again in props code
         */
        if ((ignore_exists == FALSE) &&
            (mt_value("ignorenothing") == NULL)) {
#ifdef SVR4
        load(libpath("mailx.rc"));
#else   
        load(libpath("Mail.rc"));
#endif SVR4
        }

        /*
         * For V3 on, we are using a new variable for filemenu
	 * because we now strip out the leading '+',
         * yet we need the old
         * filemenu var to remain compatible with pre V3 releases
         */
        /*
         * Create the filemenu2 variable only if it doesn't exist
	 * and filemenu exists. Also, strip out leading '+'.
	 * If filemenu2 exists then let it be, change and strip nothing
         */
        /*
	 * old_string is for strtok since strtok is destructive
	 * new_string is for new variable filemenu2
         */
        if ((mt_value("filemenu2") == NULL) &&
	    (old_string = mt_value("filemenu"))) {
		old_string = strdup(old_string);
		new_string = (char *)malloc(strlen(old_string) + 1);

		if (token = (char *) strtok(old_string, " ")) {
		    if (*token == '+') token++;
		    strcpy(new_string, token);
		    while (token = (char *) strtok(NULL, " ")) {
		    	if (*token == '+') token++;
		    	strcat(new_string, " ");
		    	strcat(new_string, token);
		    }
                    mt_assign("filemenu2", new_string);
		}
		free(old_string);
		free(new_string);
	}
}


/* ARGSUSED */
static Notify_value
mt_panel_event_proc(Notify_client client, Event *event, Notify_arg arg,
	Notify_event_type when)
{
	Notify_value	return_value;
	struct header_data *hd;
	

	/* Cast event to a Notify_event since this is what
	 * notify_next_event_func() expects.  Should event be of type
	 * Notify_event instead of Event *?  Dunno.  XView Manual says
	 * that notify_interpose_event_func()s are passed an Event *
	 * so that's what we do.
	 */
	return_value = notify_next_event_func(client, (Notify_event)event,
						arg, NOTIFY_SAFE);

	hd = mt_get_header_data(client);

	/*
	 * Handles function keys hit on the command panel or the header canvas
	 */

	/*
	 * Handle up click only
	 */
	if (event_is_down(event)) {
		goto end;
	}

	DP(("mt_panel_event_proc: event_action %d (%#x)\n",
		event_action(event),
		event_action(event)));

	switch (event_action(event)) {

	case ACTION_FIND_FORWARD:
	case ACTION_FIND_BACKWARD:
		mt_start_header_selection_proc(hd);
		break;

	case ACTION_PROPS:
		mt_props_proc();
		break;

	}

end:
	return(return_value);
}


/* ARGSUSED */
static Notify_value
mt_frame_event_proc(Notify_client client, Event *event, Notify_arg arg,
	Notify_event_type when)
{
	Notify_value			return_value;
	struct	header_data		*hd;
	struct	view_window_list	*vwl;
	

	/* Cast event to a Notify_event since this is what
	 * notify_next_event_func() expects.  Should event be of type
	 * Notify_event instead of Event *?  Dunno.  XView Manual says
	 * that notify_interpose_event_func()s are passed an Event *
	 * so that's what we do.
	 */
	return_value = notify_next_event_func(client, (Notify_event)event,
						arg, NOTIFY_SAFE);


	hd = mt_get_header_data(client);

	switch(event_action(event)) {

	case WIN_REPARENT_NOTIFY:
		/* Base frame is being mapped. Make sure self destruct is off */
		DP(("mt_frame_event_proc: WIN_REPARENT_NOTIFY\n"));
		mt_stop_self_destruct();
		break;

	case ACTION_OPEN:
		DP(("mt_frame_event_proc: ACTION_OPEN. mt_idle %d\n", mt_idle));
		if (mt_idle) {
			/*
			 * If window was just opened and we are idle
			 * then read new mail.
			 */
			mt_new_folder(hd, "%", TRUE, FALSE, FALSE, TRUE);
		}
		for (vwl = hd->hd_vwl; vwl != NULL; vwl = vwl->vwl_next)
		{
			mt_iconify_attach_node(vwl->vwl_al, FALSE);
		}
		break;

	case ACTION_CLOSE:
		DP(("mt_frame_event_proc: ACTION_CLOSE.\n"));
		mt_set_state_no_mail(hd, TRUE);
		for (vwl = hd->hd_vwl; vwl != NULL; vwl = vwl->vwl_next)
		{
			mt_iconify_attach_node(vwl->vwl_al, TRUE);
		}
		break;

	case WIN_RESIZE:
		/* 
		 * mt_resize_canvas will make sure that the amount of whitespace
		 * at the bottom of the canvas does not exceed the canvas
		 * window height.
		 */
		DP(("mt_frame_event_proc: WIN_RESIZE\n"));

		/* Make sure this is not a synthetic resize event.  Synthetic
		 * resize events are generate when the window is moved
		 */
		if (event_xevent(event)->xconfigure.send_event == 0) {
			mt_resize_canvas(hd);
			xv_set(hd->hd_cmdpanel, XV_WIDTH, WIN_EXTEND_TO_EDGE,
				0);
			mt_resize_fillin(hd->hd_cmdpanel, hd->hd_file_fillin);
		}
		break;

	case WIN_VISIBILITY_NOTIFY:
		hd->hd_framemapped = TRUE;
		break;
	}
	return(return_value);
}

/*
 * SIGTERM and SIGXCPU handler.
 * Save mailbox and exit.
 */
/* ARGSUSED */
static Notify_value
mt_signal_func(Notify_client client, int sig)
{
	struct header_data *hd;

	if (!mt_idle) {
		mt_stop_mail(0);
	}
	if (sig == SIGXCPU) {
		/* for debugging */
#ifdef SVR4
		sigset_t nullmask;
		(void)sigemptyset(&nullmask);
		(void)sigprocmask(SIG_SETMASK, &nullmask, (sigset_t *) NULL);
#else
		(void)sigsetmask(0);
#endif "SVR4"
		abort();
	}
	mt_done(sig == SIGTERM ? 0 : 1);
}




static Notify_value
mt_destroy(Notify_client client, Destroy_status status)
{
	Notify_value	val;
	struct itimerval itv;
	int	commit;
	struct header_data *hd;
	char	*p;

	hd = mt_get_header_data(client);

	switch (status) {

	case DESTROY_SAVE_YOURSELF:
		save_state(hd);
		val = notify_next_destroy_func(client, status);
		break;

	case DESTROY_CHECKING:
		/* STRING_EXTRACTION -
		 *
		 * We are about to exit, but there is at least one Compose 
		 * Window still around that has been modified.  We give the user
		 * two choices: quit mailtool anyway, or cancel the exit
		 * and continue running.
		 */
		if (compose_windows_modified(hd) &&
			!mt_vs_confirm(client, FALSE,
			gettext("Quit"),
			gettext("Cancel"),
			gettext(
			"You have unsaved edits in one or more Compose Windows.\n\
			You will lose your edits if you quit Mail Tool now.\n\
			Do you wish to:"))) 
		{
			(void)notify_veto_destroy(client);
			return(NOTIFY_DONE);
		}

		if (!mt_aborting)
		{
			mt_save_curmsgs(hd);
		}
		if (!mt_aborting && folder_methods.fm_modified(CURRENT_FOLDER(hd)))
		{
                        /* STRING_EXTRACTION -
                         *
                         * We are about to exit, but the current mailbox has
                         * not yet been changed.  We give three choices:
                         * To save the changes and exit, to discard the changes
                         * and exit, or to cancel the exit and continue
                         * running.
                         */
			commit = mt_vs_confirm3(MT_FRAME(hd), FALSE,
				gettext("Save Changes"),
				gettext("Discard Changes"),
				gettext("Cancel"),
				gettext(
"The current mail box has been changed\n\
Do you wish to save the changes?"));

			if (commit == NOTICE_NO) {
				(void)notify_veto_destroy(client);
				return(NOTIFY_DONE);
			} else if (commit == NOTICE_YES) {
				mt_aborting = FALSE;	/* Save */
			} else {
				mt_aborting = TRUE;	/* Discard */
			}
				
		} else {
			mt_aborting = TRUE;
		}

		/* turn off the "new mail" timer */
		stop_timer();

		if (mt_aborting) {
			val = notify_next_destroy_func(client, status);
		} else {
			if (p = mt_value("trash"))
				mt_del_trash(hd, p);
			val = notify_next_destroy_func(client, status);
		}

		/*
		 * Now we will be called with DESTROY_CLEANUP
		 */
		break;
	case DESTROY_CLEANUP:
		/*
		 * In this case
		 * the routines called will loop thru
		 * mt_header_data_list and cleanup
		 */
		TRACK_MESSAGE("mt_destroy: quiting");
		destroy_compose_windows();
		mt_cleanup_tmpfiles();
		val = notify_next_destroy_func(client, status);
		break;
	case DESTROY_PROCESS_DEATH:
		/*
		 * In this case
		 * the routines called will loop thru
		 * mt_header_data_list and cleanup
		 */
		mt_stop_mail(0);
		destroy_compose_windows();
		mt_cleanup_tmpfiles();
		val = notify_next_destroy_func(client, status);
		break;
	default:
		val = notify_next_destroy_func(client, status);
		notify_remove(mt_mailclient);
		break;
	}

	return (val);
}

static int
compose_windows_modified(struct header_data *hd)
{
	struct reply_panel_data *ptr;

	/*
	 * Check to see whether there are any compose windows up that
	 * have been modified.
	 */

	ptr = MT_RPD_LIST(hd);
	while (ptr)
	{
		/* 
		 * return TRUE as soon as we hit a compose window that
		 * has been modified.
		 */
		if ((int)xv_get(ptr->replysw, TEXTSW_MODIFIED)) 
			return(TRUE);
		ptr = ptr->next_ptr;
	}
	return(FALSE);
}

static void
destroy_compose_windows(void)
{
	struct reply_panel_data *ptr;
	struct header_data 	*hd;

	/*
	 * run thru and destroy all the composition windows
	 * This will not only destroy them.  Set mt_destroying so that the
	 * compose window's destroy proc knows that any necessary
	 * confirmation has taken place.  This will also cause messages
	 * to be appended into dead.letter instead of overwritting it.
	 */
	for (hd = mt_header_data_list; hd != NULL; hd = hd->hd_next) {
		mt_destroying = TRUE;
		ptr = MT_RPD_LIST(hd);
		while (ptr)
		{
			xv_destroy_safe(ptr->frame);
			ptr = ptr->next_ptr;
		}
	}
}


/* timers */

/*
 * Stop the timer to look for new mail.
 */
void
stop_timer(void)
{
	static struct itimerval itv = { {0, 0}, {0, 0} };

	(void)notify_set_itimer_func(mt_mailclient, mt_itimer,
		ITIMER_REAL, &itv, 0);
	return;
}

/*
 * Start the timer to look for new mail.
 *
 * This routine is called from one of two places: either at startup
 * from mt_finish_initializing, or from mt_props_apply_proc when something
 * has changed.
 */
void
mt_start_timer(void)
{
	static struct itimerval itv;
	char           *p;
	int             interval = 0;
	static		oldinterval = 0;
	extern int	mt_no_classing_engine;

	if (p = mt_value("retrieveinterval"))
		interval = atoi(p);
	if (interval == 0)
		interval = 5*60;	/* 5 minutes */

	itv.it_interval.tv_sec = interval;
	itv.it_value.tv_sec = interval;


	if (oldinterval == 0) {
		/* first time through the routine */

		if (mt_no_classing_engine) {
			static struct itimerval tv = { {0, 0}, {0, 1} };

			/*
			 * Set up timer to go off once and immediately.
			 */
			notify_set_itimer_func(
				(Notify_client)&mt_no_classing_engine,
				mt_itimer_noce, ITIMER_VIRTUAL, &tv, 0);
		}

	}



	/* either first time through the routine, or the interval
	 * has changed
	 */
	if (oldinterval != interval) {
		DP(("mt_start_time: oldinterval %d, interval %d\n",
			oldinterval, interval));

		(void)notify_set_itimer_func(mt_mailclient, mt_itimer,
			ITIMER_REAL, &itv, 0);

		oldinterval = interval;
	}
}

/*
 * Check for new mail when timer expires. 
 */
/* ARGSUSED */
static Notify_value
mt_itimer(Notify_client client, int which)
{
	static time_t last_flush;
	time_t          now;
	Frame		*p_frame;
	/* mt_mailclient is address of the frame */
	struct header_data *hd;

	DP(("mt_itimer: called\n"));

	p_frame = (Frame *) client;
	hd = mt_get_header_data(*p_frame);

#ifdef INSTRUMENT
	track_timer();
#endif INSTRUMENT

	/* Try to flush the delete transaction log once a minute
	 * We also flush once every 5 transactions (see delete_log.c)
	 * just in case the user checks for mail rarely.
	 */
	now = time(NULL);
	if (now - last_flush > 60) {
		last_flush = now;
		mt_flush_transaction_log();
	}

	mt_check_mail_box(hd, FALSE);
 	return (NOTIFY_DONE);
}




/* put up a message when there is no classing engine */
/* ARGSUSED */
static Notify_value
mt_itimer_noce(Notify_client client, int which)
{

	mt_vs_warn(NULL, gettext(
"%s: Could not initialize the Classing Engine\n\
You must have the Classing Engine installed to use Attachments\n"),
mt_cmdname);

	return (NOTIFY_DONE);
}



static void
frame_layout_proc(struct header_data *hd)
{
	if (!mt_usersetsize)
	{
		int charwidth;
		int width;
		int height;
		int paintheight;
		int paintwidth;
		int deltawidth;
		int deltaheight;

		charwidth = xv_get(textsw_font, FONT_COLUMN_WIDTH);

		/* OK. here's what's going on.  We really want to
		 * set the size of the pw, but we can't.  So instead
		 * we set the size of the canvas, look to see how
		 * much we missed by, and size it again
		 */

		width = xv_get(hd->hd_canvas, XV_WIDTH);
		paintwidth = xv_get(hd->hd_paintwin, XV_WIDTH);
		deltawidth = width - paintwidth;

		height = xv_get(hd->hd_canvas, XV_HEIGHT);
		paintheight = xv_get(hd->hd_paintwin, XV_HEIGHT);
		deltaheight = height - paintheight;

		MP(("width %d, paintwidth %d, deltawidth %d\n",
			width, paintwidth, deltawidth));
		MP(("height %d, paintheight %d, deltaheight %d\n",
			height, paintheight, deltaheight));

		width = mt_tool_width * charwidth + deltawidth + 4;
		height = mt_headerlines * hd->hd_lineheight + deltaheight + 2;

		MP(("setting width/height to %d/%d\n", width, height));
		MP(("canvas margins: left %d, right %d\n",
			xv_get(hd->hd_canvas, XV_LEFT_MARGIN),
			xv_get(hd->hd_canvas, XV_RIGHT_MARGIN)));
		MP(("paintwin margins: left %d, right %d\n",
			xv_get(hd->hd_paintwin, XV_LEFT_MARGIN),
			xv_get(hd->hd_paintwin, XV_RIGHT_MARGIN)));

		xv_set(hd->hd_canvas,
			XV_WIDTH, width,
			XV_HEIGHT, height,
			0);

		xv_set(hd->hd_cmdpanel, XV_WIDTH, width - 10, 0);

		/* The base panel is the correct default width.  We
		 * resize the fillin field so that it uses up all available
		 * space.  But the panel may be truncating the rightmost
		 * button in some locales (like German).  So we do a final
		 * window fit to let the panel grow if it needs to.
		 */
		mt_resize_fillin(hd->hd_cmdpanel, hd->hd_file_fillin);
		window_fit_width(hd->hd_cmdpanel);

		window_fit(hd->hd_frame);
	}

	(void)xv_set(hd->hd_cmdpanel,
		XV_WIDTH,	WIN_EXTEND_TO_EDGE,
		0);

	(void)xv_set(hd->hd_canvas,
		XV_WIDTH,	WIN_EXTEND_TO_EDGE,
		0);

}



int
mt_get_scrollbar_width(Canvas canvas)
{
	Rect	*rect;

	rect = (Rect *)xv_get((Scrollbar)xv_get(canvas, WIN_VERTICAL_SCROLLBAR),
				XV_RECT);
	return(rect->r_width);
}


static void
save_state(struct header_data *hd)
{
	FILE	*outfile;
	char	buffer[256];
	char	huge_buffer[1024];
	struct reply_panel_data *c_ptr= MT_RPD_LIST(hd);
	struct view_window_list *v_ptr;
	Rect	win_rect;
	int	ixloc, iyloc;
	Window	child;


	/* write out lines to describe the first visible 
	   viewing window. */

	huge_buffer[0] = NULL;

	v_ptr = hd->hd_vwl;
	while (v_ptr)
	{
		if (xv_get(v_ptr->vwl_frame, XV_SHOW))
		{
			/* viewing window location and size */

			frame_get_rect(v_ptr->vwl_frame, &win_rect);

			sprintf(buffer, "viewwin xloc %d yloc %d width %d height %d\n", 
				win_rect.r_left, 
				win_rect.r_top, 
				win_rect.r_width,
				win_rect.r_height);
			strcat(huge_buffer, buffer);
			break;
		}

		v_ptr = v_ptr->vwl_next;
	}

	/* write out lines to describe all of the composition 
	   windows whether they are visible or not */

	while (c_ptr)
	{
		/* frame location and size */

		/*
		 * ZZZ dipol: This gets bogus values if the frame is iconic
		 */
		frame_get_rect(c_ptr->frame, &win_rect);

		sprintf(buffer, "compwin xloc %d yloc %d width %d height %d ", 
			win_rect.r_left, 
			win_rect.r_top, 
			win_rect.r_width,
			win_rect.r_height);
		strcat(huge_buffer, buffer);

		/*
		 * This is the only known way of getting valid coordinates
		 * for an icon.  sigh
		 */
		XTranslateCoordinates((Display *)xv_get(MT_FRAME(hd), XV_DISPLAY), 
			xv_get(xv_get(c_ptr->frame, FRAME_ICON), XV_XID), 
			xv_get(xv_get(xv_get(c_ptr->frame, FRAME_ICON), XV_ROOT), XV_XID), 
			0, 0, &ixloc, &iyloc, &child);

		sprintf(buffer, "ixloc %d iyloc %d ", 
			ixloc,
			iyloc);
		strcat(huge_buffer, buffer);

		/* is it iconic? */

		if (xv_get(c_ptr->frame, FRAME_CLOSED))
			strcat(huge_buffer, " iconic ");

		/* menu item states */

		sprintf(buffer, 
			" deldef %d ", 
			xv_get(xv_get(c_ptr->deliver_item, PANEL_ITEM_MENU), MENU_DEFAULT));
		strcat(huge_buffer, buffer);

#ifdef EDITOR
		if (c_ptr->edit_item != NULL) {
			sprintf(buffer, 
				" editdef %d ", 
				xv_get(xv_get(c_ptr->edit_item,
					      PANEL_ITEM_MENU), MENU_DEFAULT));
			strcat(huge_buffer, buffer);
		}
#endif

#ifdef NEVER
		/* fillin_states */

		if (xv_get(c_ptr->bcc_fillin, PANEL_SHOW_ITEM))
			strcat(huge_buffer, " bccfillin ");
#endif

		strcat(huge_buffer, "\n");

		c_ptr = c_ptr->next_ptr;
	}

	/* save out base frame menu defaults */

	sprintf(buffer,
	"basewin filedef %d viewdef %d editdef %d compdef %d repdef %d\n",
		xv_get(mt_file_menu, MENU_DEFAULT),
		xv_get(mt_view_menu, MENU_DEFAULT),
		xv_get(mt_edit_menu, MENU_DEFAULT),
		xv_get(mt_compose_menu, MENU_DEFAULT),
		xv_get(xv_get(xv_get(mt_compose_menu, MENU_NTH_ITEM, 3),
			MENU_PULLRIGHT), MENU_DEFAULT));

	strcat(huge_buffer, buffer);

	sprintf(buffer, "%s/.mailtool-init", getenv("HOME"));

	outfile = fopen(buffer, "w");

	if (!outfile)
	{
                /* STRING_EXTRACTION -
                 *
                 * We are responding to a save-workspace action, but we
                 * could not open the file we want to save the window
                 * arrangement into.
                 */
		mt_vs_warn(MT_FRAME(hd),
			gettext("could not open save_self file: %s\n"),
			strerror(errno));
		return;
	}
	fprintf(outfile, huge_buffer);
	fclose(outfile);
}




static void
initialize_state(struct header_data *hd)
{
	FILE	*outfile;
	char	buffer[256];
	char	*s_ptr;
	int	viewwin_done = FALSE;
	int	x, y, width, height, iconic, default_menu;
	struct	reply_panel_data *c_ptr;
	struct	view_window_list *v_ptr;
	Rect	win_rect, *icon_rectp;
	int	ixloc, iyloc;

	ixloc = -1;
	iyloc = -1;

	sprintf(buffer, "%s/.mailtool-init", getenv("HOME"));
	outfile = fopen(buffer, "r");

	if (!outfile)
		return;

	while (fgets(buffer, 256, outfile))
	{
		s_ptr = (char *) strtok(buffer, " ");

		if (s_ptr)
		{
			if (!strcmp(s_ptr, "viewwin"))
			{
				if (viewwin_done)
				{
					DP(("extra view win definition\n"));
				}
				else
				{
					
					v_ptr = mt_create_new_view( MT_FRAME(hd),
								NULL );
					frame_get_rect(
						v_ptr->vwl_frame, &win_rect);

					v_ptr->vwl_next = hd->hd_vwl;
					hd->hd_vwl = v_ptr;

					while (s_ptr = (char *)
						strtok(NULL, " "))
					{
						if (!strcmp(s_ptr, "xloc"))
						{
							x = atoi(strtok(NULL,
								" "));
							win_rect.r_left = x;
						}
						else if (!strcmp(s_ptr, "yloc"))
						{
							y = atoi(strtok(NULL,
								" "));
							win_rect.r_top = y;
						}
						else if (!strcmp(s_ptr,
							"width"))
						{
							width =
								atoi(
								strtok(NULL,
								" "));
							win_rect.r_width =
								width;
						}
						else if (!strcmp(s_ptr,
							"height"))
						{
							height = atoi(
								strtok(NULL,
								" "));
							win_rect.r_height =
								height;
						}

					}
					frame_set_rect(v_ptr->vwl_frame,
						&win_rect);
					viewwin_done = TRUE;
				}
			}
			else if (!strcmp(s_ptr, "compwin"))
			{
				/* create a new composition window. */

				c_ptr = mt_create_new_compose_frame(hd);

				/* add the composition frame to the internal
				 * list
				 */
				c_ptr->next_ptr = MT_RPD_LIST(hd);
				MT_RPD_LIST(hd) = c_ptr;

				frame_get_rect(c_ptr->frame, &win_rect);

				xv_set(c_ptr->bcc_fillin, PANEL_SHOW_ITEM, FALSE, 0);
				while (s_ptr = (char *) strtok(NULL, " "))
				{
					if (!strcmp(s_ptr, "xloc"))
					{
						x = atoi(strtok(NULL, " "));
						win_rect.r_left = x;
					}
					else if (!strcmp(s_ptr, "yloc"))
					{
						y = atoi(strtok(NULL, " "));
						win_rect.r_top = y;
					}
					else if (!strcmp(s_ptr, "width"))
					{
						width = atoi(strtok(NULL, " "));
						win_rect.r_width = width;
					}
					else if (!strcmp(s_ptr, "height"))
					{
						height = atoi(strtok(NULL, " "));
						win_rect.r_height = height;
					}
					else if (!strcmp(s_ptr, "ixloc"))
					{
						ixloc = atoi(strtok(NULL, " "));
					}
					else if (!strcmp(s_ptr, "iyloc"))
					{
						iyloc = atoi(strtok(NULL, " "));
					}
					else if (!strcmp(s_ptr, "deldef"))
					{
						default_menu =
							atoi(strtok(NULL, " "));
						set_menu_default(xv_get(
							c_ptr->deliver_item,
							PANEL_ITEM_MENU),
							default_menu);
					}
					else if (!strcmp(s_ptr, "editdef"))
					{
						default_menu =
							atoi(strtok(NULL, " "));
#ifdef EDITOR
						if (c_ptr->edit_item != NULL) {
							set_menu_default(xv_get(
							c_ptr->edit_item,
							PANEL_ITEM_MENU),
							default_menu);
						}
#endif
					}
					else if (!strcmp(s_ptr, "iconic"))
					{
						xv_set(c_ptr->frame,
							FRAME_CLOSED, TRUE, 0);
					}
#ifdef NEVER
					else if (!strcmp(s_ptr, "bccfillin"))
					{
						xv_set(c_ptr->bcc_fillin,
							PANEL_SHOW_ITEM, TRUE,
							0);
					}
#endif

				}

				mt_compose_frame_layout_proc(c_ptr->frame);
				frame_set_rect(c_ptr->frame, &win_rect);

				/*
				 * Set the compose icon location
				 */

/*
**	Do not do this. It sets the icon to 0,0 every time.
**	Allow the icon to be set normally. Bug ID 1243831
**
**				icon_rectp = (Rect *)xv_get(c_ptr->frame,
**						FRAME_CLOSED_RECT);
**				icon_rectp->r_left = ixloc;
**				icon_rectp->r_top = iyloc;
**
**				(void)xv_set(c_ptr->frame, FRAME_CLOSED_RECT,
**						icon_rectp, 0);
*/

				 /* Display closed frames */
				 if (xv_get(c_ptr->frame, FRAME_CLOSED))
				 	xv_set(c_ptr->frame, XV_SHOW, TRUE, 0);
			}
			else if (!strcmp(s_ptr, "basewin"))
			{
				while (s_ptr = (char *) strtok(NULL, " "))
				{
					if (!strcmp(s_ptr, "filedef"))
					{
						default_menu =
							atoi(strtok(NULL, " "));

						set_menu_default(mt_file_menu,
							default_menu);
					}
					else if (!strcmp(s_ptr, "viewdef"))
					{
						default_menu =
							atoi(strtok(NULL, " "));

						set_menu_default(mt_view_menu,
							default_menu);
					}
					else if (!strcmp(s_ptr, "editdef"))
					{
						default_menu =
							atoi(strtok(NULL, " "));

						set_menu_default(mt_edit_menu,
							default_menu);
					}
					else if (!strcmp(s_ptr, "compdef"))
					{
						default_menu =
							atoi(strtok(NULL, " "));

						set_menu_default(mt_compose_menu,
							default_menu);
					}
					else if (!strcmp(s_ptr, "repdef"))
					{
						default_menu =
							atoi(strtok(NULL, " "));

						set_menu_default(xv_get(
							xv_get(mt_compose_menu,
							MENU_NTH_ITEM, 3),
							MENU_PULLRIGHT),
							default_menu);
					}
				}
			}
		}
	}

	fclose(outfile);
}




static int
set_menu_default(Menu menu, int default_item)
{
	int	nitems;

	if (menu == NULL || default_item < 0)
		return;

	/* Set the default on a menu.  Check for out of bounds value */

	nitems = (int)xv_get(menu, MENU_NITEMS);

	if (default_item < 1 || default_item > nitems)
		return(-1);

	xv_set(menu, MENU_DEFAULT, default_item, 0);

	return(default_item);
}




static void
header_canvas_event_proc(Window canvas, Event *event, Notify_arg arg)
{
	struct header_data *hd = mt_get_header_data(canvas);

	switch(event_action(event)) {
	case ACTION_DRAG_MOVE:
	case ACTION_DRAG_COPY:

		DP(("header_canvas_event_proc: got a drop\n"));
		mt_received_drop(canvas, event, hd->hd_al);
	}

}




static Notify_value
mt_handle_keys_proc(Window window, Event *event, Notify_arg arg)
{
	Menu	bogus_menu;
	struct header_data *hd = mt_get_header_data(window);

	/*
	 * Handles function keys hit on the command panel or the header canvas
	 */

	/*
	 * Handle up click only
	 */
	if (event_is_down(event)) {
		goto end;
	}

	DP(("mt_handle_keys_proc: event_action %d (%#x)\n",
		event_action(event),
		event_action(event)));

	switch (event_action(event)) {

	case ACTION_FIND_FORWARD:
	case ACTION_FIND_BACKWARD:
		mt_start_header_selection_proc(hd);
		break;

	case ACTION_UNDO:
		bogus_menu = mt_create_bogus_menu(hd);
		mt_undel_last(bogus_menu, NULL);
		break;

	case '\023':		/* ctrl-s*/
	case 'S':		/* shift-s */
		if (event_meta_is_down(event)) {
			bogus_menu = mt_create_bogus_menu(hd);
			mt_copy_proc(bogus_menu, NULL);
		}
		break;
#ifndef KYBDACC
	case ACTION_STORE:	/* Meta-s */
		bogus_menu = mt_create_bogus_menu(hd);
		mt_save_proc(bogus_menu, NULL);
		break;
#endif
	case ACTION_PROPS:
		mt_props_proc();
		break;

	case ACTION_GO_COLUMN_BACKWARD:
		bogus_menu = mt_create_bogus_menu(hd);
		mt_prev_proc(bogus_menu, NULL);
		break;

	case ACTION_GO_COLUMN_FORWARD:
	case ACTION_GO_LINE_FORWARD:
		bogus_menu = mt_create_bogus_menu(hd);
		mt_next_proc(bogus_menu, NULL);
		break;

	case ACTION_GO_PAGE_FORWARD:
	case ACTION_GO_PAGE_BACKWARD:
	case ACTION_GO_DOCUMENT_START:
	case ACTION_GO_DOCUMENT_END:
		mt_scroll_header(window, event_action(event));
		break;
	}

end:
	return(NOTIFY_DONE);
}

static struct header_data *
try_header_data(Xv_opaque win)
{
	struct header_data *hd;

	if (! KEY_HEADER_DATA) {
		mt_abort("mt_get_header_data: NULL KEY_HEADER_DATA\n",
			0, 0, 0, 0);
	}

	hd = (struct header_data *) xv_get(win, XV_KEY_DATA, KEY_HEADER_DATA);

	return (hd);
}


struct header_data *
mt_get_header_data(Xv_opaque win)
{
	struct header_data *hd;

	hd = try_header_data(win);

	if (hd == NULL) {
		mt_abort("mt_get_header_data: NULL header_data\n", 0, 0, 0, 0);
	}

	return (hd);
}


void
mt_window_header_data(struct header_data *hd, Xv_opaque win)
{
	if (! KEY_HEADER_DATA) {
		KEY_HEADER_DATA = xv_unique_key();
	}

	if (win) {
		xv_set(win, XV_KEY_DATA, KEY_HEADER_DATA, hd, 0);
	}
}

void
mt_set_header_data(struct header_data *hd)
{
	mt_window_header_data(hd, hd->hd_frame);
	mt_window_header_data(hd, hd->hd_canvas);
	mt_window_header_data(hd, hd->hd_cmdpanel);
	mt_window_header_data(hd, hd->hd_paintwin);
	mt_window_header_data(hd, hd->hd_scrollbar);
}


void
mt_copy_header_data(Xv_opaque win1, Xv_opaque win2)
{
	struct header_data *hd;

	hd = mt_get_header_data(win1);
	mt_window_header_data(hd, win2);
}

