#ifndef lint
static 	char sccsid[] = "@(#)attach_canvas.c	3.36 97/02/10 - Copyr 1997 Sun Micro";
#endif

/*
 * Copyright (c) 1990 by Sun Microsystems, Inc.
 */

#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/param.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <xview/xview.h>
#include <xview/panel.h>
#include <xview/canvas.h>
#include <xview/textsw.h>
#include <xview/font.h>
#include <xview/scrollbar.h>
#include <xview/xv_xrect.h>
#include <xview/fullscreen.h>
#include <X11/X.h>
#include <X11/Xatom.h>

#include "glob.h"
#include "tool.h"
#include "tool_support.h"
#include "mail.h"
#include "header.h"
#include "attach.h"
#include "graphics.h"
#include "mle.h"
#include "../maillib/obj.h"
#include "ds_tooltalk.h"
#include "tooltalk.h"
#include "dstt.h"
#include "mail_dstt.h"

#define DEBUG_FLAG mt_debugging
extern int DEBUG_FLAG;
#include "debug.h"

static int KEY_ATTACH_LIST;

#define VWL_KEY   300   /* also in attach_panel.c, view.c */
extern long	mt_msgsizewarning;
extern long	mt_msgsizelimit;

static int paint_attach_icon(Attach_list *al, Attach_node *node, long);
static Attach_node *handle_attach_adjust_event(Event *event, Attach_list *al);
static Attach_node *handle_attach_select_event(Event *event, Attach_list *al);
static void set_dragdrop(Attach_list *);
static void attach_resize_proc(Canvas);
static int clear_attach_selection(Attach_list *al, Attach_node *node, long);
static char *attach_data_file(struct attach *at);


extern char *	getenv();
extern char *	new_messageid();


extern Xv_Cursor copyletter_cursor;
extern Xv_Cursor copyletters_cursor;
extern Xv_Cursor moveletter_cursor;
extern Xv_Cursor moveletters_cursor;

static unsigned short many_image_icon[] = {
	0x0000,0x0000, 0x0000,0x0000, 0x0000,0x0000, 0x0000,0x0000,
	0x0000,0x0000, 0x0000,0x0008, 0x0000,0x0004, 0x0000,0x0006,
	0x0000,0x0005, 0x0000,0x0005, 0x0000,0x0005, 0x0000,0x0005,
	0x0000,0x0005, 0x0000,0x0005, 0x0000,0x0005, 0x0000,0x0005,
	0x0000,0x0005, 0x0000,0x0005, 0x0000,0x0005, 0x0000,0x0005,
	0x0000,0x0005, 0x0000,0x0005, 0x0000,0x0005, 0x0000,0x0005,
	0x0000,0x0005, 0x0000,0x0005, 0x0000,0x0005, 0x0000,0x0005,
	0x2000,0x0005, 0x3FFF,0xFFFD, 0x0800,0x0001, 0x0FFF,0xFFFF
};


static unsigned short copy_image_bits[] = {
	0xC000,0xF000,0x7C00,0x7F80,0x3F00,0x3E3C,0x1F7C,0x1BDD,
	0x119D,0x031D,0x061D,0x07FD,0x07FD,0x07FD,0x0001,0x01FF
};
static unsigned short move_image_bits[] = {
	0xC000,0xF000,0x7C00,0x7F80,0x3F00,0x3E3C,0x1F7C,0x1BDC,
	0x119C,0x031C,0x061C,0x07FC,0x07FC,0x07FC,0x0000,0x0000
};



void
mt_create_attach_canvas(
	Frame	frame,
	Panel	panel,
	Attach_list	*al
)
{
	void	attach_repaint_proc();
	void	attach_canvas_event_proc();
	Notify_value	free_attach_canvas_data();
	Canvas	canvas;

	canvas = (Canvas)xv_create(frame, CANVAS,
		XV_WIDTH,		WIN_EXTEND_TO_EDGE,
		CANVAS_REPAINT_PROC,	attach_repaint_proc,
		CANVAS_RETAINED,	FALSE,
		CANVAS_AUTO_SHRINK,	FALSE,
		CANVAS_AUTO_CLEAR,	FALSE,
		CANVAS_X_PAINT_WINDOW,	TRUE,
		CANVAS_FIXED_IMAGE,	FALSE,
		WIN_EVENT_PROC,		attach_canvas_event_proc,
		XV_SHOW,		FALSE,
		XV_HELP_DATA,		"mailtool:AttachCanvas",
#ifdef OW_I18N
		WIN_USE_IM,		FALSE,
#endif
		0);

	mt_set_attach_list(canvas, al);

	mt_init_attach_list(al, frame, canvas, panel);

	notify_interpose_destroy_func(canvas, free_attach_canvas_data);

	set_dragdrop(al);

	(void)xv_create(canvas, SCROLLBAR,
		SCROLL_LINE_HEIGHT, al->al_h + al->al_y_gap,
		0);

	xv_set(canvas_paint_window(canvas),
		WIN_CONSUME_PICK_EVENTS,
			LOC_DRAG,
			ACTION_DRAG_MOVE,
			ACTION_DRAG_COPY,
			0,
		WIN_EVENT_PROC,		attach_canvas_event_proc,
		0);

	mt_set_attach_canvas_height(al);

}



static Notify_value
free_attach_canvas_data(
	Xv_object	object,
	int		status
)
{
	Attach_list	*al;

	DP(("free_attach_canvas_data: (%x %d)\n", object, status));

	if (status == DESTROY_CLEANUP) {

		al =  mt_get_attach_list(object);
		mt_destroy_attach_list(al);
	}

	return(notify_next_destroy_func(object, status));
}



void
mt_set_attach_canvas_height(
	Attach_list	*al
)
{
	if (al->al_canvas == NULL)
		return;

	if ((int)xv_get(al->al_canvas, XV_HEIGHT) != al->al_h +
							al->al_y_gap) {
		/* small additional pixels (5) for compose att. pane */
		(void)xv_set(al->al_canvas,
			XV_HEIGHT, 5 + al->al_h + al->al_y_gap,
			0);

	}
}


/* ARGSUSED */
static void
attach_repaint_proc(
	Canvas		canvas,
	Xv_Window	paint_window,
	Display		*display,
	Window		xwin,
	Xv_xrectlist	xrects
)
{
	Attach_list	*al;

	/*
	 * Handle attachment canvas repainting
	 */
	al = mt_get_attach_list(canvas);
	mt_repaint_attach_canvas(al, FALSE);
}




/* ARGSUSED */
static void
attach_resize_proc(
	Canvas		canvas
)
{
	Attach_list	*al;
	Rect		rect;
	int		result;


	rect.r_left = 0;
	rect.r_top = 0;
	rect.r_width = xv_get(canvas, WIN_WIDTH);
	rect.r_height = xv_get(canvas, WIN_HEIGHT);

	DP(("attach_resize_proc: %x %d/%d\n",
		canvas, rect.r_width, rect.r_height));

	al = mt_get_attach_list(canvas);
	if (! al) return;

	if (al->al_drop_site) {
		DP(("attach_resize_proc: setting the drop site\n"));

		result = xv_set(al->al_drop_site,
			DROP_SITE_DELETE_REGION, NULL,
			DROP_SITE_REGION, &rect,
			0);

		DP(("attach_resize_proc: result of drop site setting - %d\n",
			result));
	}

	/*
	 * Reposition the message item in the attachment panel.
	 * Strictly speaking this call should be associated with a resize of
	 * the attachment panel.  But this is convenient and it works.
	 */
	mt_attach_panel_message(al);
}



void
mt_repaint_attach_canvas(
	Attach_list	*al,
	int		clear
)
{
	if (al->al_canvas == NULL)
		return; /* Nothing to repaint */
	/*
	 * Repaint the attachment canvas.  Clear it first if requested.
	 */
	if (clear) {
		XFillRectangle(al->al_display, al->al_drawable, al->al_cleargc,
		0, 0, (unsigned int)xv_get(al->al_canvas, CANVAS_WIDTH),
			(unsigned int)xv_get(al->al_canvas, CANVAS_HEIGHT));
	}

	mt_attach_panel_message(al);
	mt_traverse_attach_list(al, paint_attach_icon, 0);
}



void
mt_layout_attach_display(
	Attach_list	*al
)
{
	int	w, new_h, win_h;
	Canvas	canvas;

	/*
	 * Set the x,y locations of all attachment icons and make sure the
	 * canvas is the proper size
	 */
	canvas = al->al_canvas;
	(void)xv_set(canvas, CANVAS_WIDTH, (int)xv_get(canvas, XV_WIDTH), 0);

	w = (int)xv_get(canvas, CANVAS_WIDTH) - mt_get_scrollbar_width(canvas);
	new_h = mt_set_attach_display_xys(al, w);
	win_h = (int)xv_get(canvas, XV_HEIGHT);
	(void)xv_set(canvas, CANVAS_HEIGHT, MAX(new_h,win_h), NULL);
	return;
}



/*
 * trivial support routine to return a printable "name" for the attachment
 */
char *
mt_attach_name(
	struct attach *at
)
{
	char	*name;

	name = (char *)attach_methods.at_get(at, ATTACH_DATA_NAME);
	if (name == NULL)
		name = (char *)attach_methods.at_get(at, ATTACH_DATA_TYPE);
	if (name == NULL)
		return ("");
	return (name);
}



static int
paint_attach_icon(
	Attach_list	*al,
	Attach_node	*node,
	long		unused
)
{
	int	y;
	int	x;
	char	*label;
	GC	gc;
	char	buf[80];
	Font_string_dims	font_size;

	label = mt_attach_name(node->an_at);

	/*
	 * Display the attachment icon specified by node 
	 */
	if (node->an_selected)
		gc = al->al_cleargc;
	else
		gc = al->al_gc;

	y = node->an_y;
	x = node->an_x;
	mt_rop(al->al_display, al->al_drawable, gc, x, y, al->al_w, al->al_h,
			(Drawable)xv_get(node->an_icon, XV_XID));

	/*
	 * Label icon
	 */
	y += al->al_h + al->al_y_gap / 2;
	x = x + al->al_w / 2 - node->an_label_width / 2;
#ifdef OW_I18N
	if (mt_multibyte) {
		XmbDrawString(al->al_display, al->al_drawable,
			al->al_fontset,
			al->al_gc, x, y, label, node->an_label_length);
		XSetFont(al->al_display, al->al_gc, al->al_fontid);
	} else {
		mt_draw_text(al->al_display, al->al_drawable,
			al->al_gc, x, y, label, node->an_label_length);
	}
#else
	mt_draw_text(al->al_display, al->al_drawable,
		al->al_gc, x, y, label, node->an_label_length);
#endif

	sprintf(buf, "%d:", node->an_number);
	(void)xv_get(al->al_font, FONT_STRING_DIMS, buf, &font_size);
	x = node->an_x - font_size.width - (int)xv_get(al->al_font,
			FONT_DEFAULT_CHAR_WIDTH) / 2;
	y = node->an_y + font_size.height;

	mt_draw_text(al->al_display, al->al_drawable, al->al_gc,
		x, y, buf, strlen(buf));
	mt_draw_text(al->al_display, al->al_drawable, al->al_gc,
		x + 1, y, buf, strlen(buf));

	return(TRUE);
}



static
valid_attach_size(
	Attach_list	*al,
	int		length
)
{
	int	cancel;
        struct view_window_list *vwl;

	/* Do these valid checks only if in compose window, no need if in view window */
        vwl = (struct view_window_list *)
                xv_get(al->al_frame, XV_KEY_DATA, VWL_KEY);
        if (vwl) {
		return(TRUE);
        }

	/* STRING_EXTRACTION -
	 *
	 * The user has tried to add a very large attachment.  
	 * We check its size against two mailtool variables;
	 * if it is too big one notice will be put up.
	 * It will either be a notice saying "sorry, too big" with no
	 * options, or one saying "warning, too big" with
	 * a choice of adding the attachment or not.
	 */
	al->oversize_notice = FALSE;

	/* 
	 * 1) Check msgsizelimit, if exists check against length
	 * 2) Check msgsizewarning against length
	 */
	if (mt_msgsizelimit && length > mt_msgsizelimit) {
		mt_vs_warn(al->al_frame,
			gettext(
"Sorry, you cannot add this attachment because\n\
it is too large. It may not reach the recipient, and\n\
if it does it may overload the recipient's mailbox.\n\
Please use another method to transfer this data."));
		return(FALSE);
	} 
	if (mt_msgsizewarning && length > mt_msgsizewarning) {
		cancel = mt_vs_confirm(al->al_frame, FALSE,
			gettext("Cancel"), gettext("Add Attachment"),
			gettext(
"This attachment is very large.\n\
It may not reach the recipient,\n\
and if it does it may overload the\n\
recipient's mailbox.  Please consider\n\
using another method to transfer this data.\n\
You may:"));
		if (cancel) {
			return(FALSE);
		} else {
			/* Set flag so we won't flood user later w/2nd notice */
			al->oversize_notice = TRUE;
			return(TRUE);
		}
	}

	return(TRUE);
}

/*
 * we've got the type and data; add the attachment...
 * ZZZ: all "Attach_list" will be obsoleted by "msg".  (lau)
 *
 *	Return: 1	All is well
 *		0	Could not add the attachment
 */
int
add_attachment_memory(
	Attach_list *al,
	struct msg *msg,
	void *data,
	int length,
	char *type,
	char *label,
	int executable
)
{
	Attach_node	*an;


	an = mt_create_attachment(al, data, length, type, label, executable);

	if (an == NULL)
		return 0;

	mt_add_attachment(al, an, msg);

	return 1;
}

/* Create an attachment  */
Attach_node *
mt_create_attachment(
	Attach_list *al,
	void *data,
	int length,
	char *type,
	char *label,
	int executable
)
{
	struct attach	*at;
	Attach_node	*an;
	int		clear;
	char		*template;
	char		*s = NULL;	/* pointer to final label str */
	char		*t = NULL;
	char		*r = NULL;
	char		*mt_get_compression_type();
	extern char	*mt_get_attachment_name();

	/* Check if attachment is too big or will make message too big */
	if (!valid_attach_size(al, length))
		return NULL;
	
	/* fake up a label if one isn't provided */
	if (! label || *label == '\0')
	{
		template = mt_get_attachment_name(type);
		if (template != NULL)
		{
			/* if we have a template, we may need to do some
			 * work on it before we allow the label to be
			 * assigned.
			 */

			s = malloc(strlen(template)+10);
			r = malloc(strlen(template)+10);
			if (s && r)
			{
				strcpy(r, template);

				/*
				 * Check for %t string. If it exists,
				 * replace it with s, so sprintf can
				 * evaluate the string, and substitute
				 * data (such as attachment number) into
				 * label.
				 */

				t = strstr(r, "%t");
				if (t)
					*(t+1) = 's';

				sprintf(s, r, "");
				label = s;
			}
			else
				label = type;
		}
		else
			label = type;
	}

	at = attach_methods.at_create();
	attach_methods.at_set(at, ATTACH_DATA_TYPE, type);
	attach_methods.at_set(at, ATTACH_DATA_NAME, label);
	attach_methods.at_set(at, ATTACH_MMAP_BODY, data);
	attach_methods.at_set(at, ATTACH_CONTENT_LEN, length);
	attach_methods.at_set(at, ATTACH_EXECUTABLE, executable);

	attach_methods.at_set(at, ATTACH_ENCODE_INFO,
		              mt_get_compression_type(type));
	ck_free(s);
	ck_free(r);

	if ((an = mt_get_attach_node(at)) == NULL) {
		attach_methods.at_destroy(at);
	} else {
		mt_init_attach_node(al, an, at, NULL);
	}

	return an;
}

/* Add an attachment to the attachment list, and update display */
void
mt_add_attachment(
	Attach_list *al,
	Attach_node *an,
	struct msg *msg
)
{
	int	clear;

	/* Append the attachment to the end of message attachment list */
	msg_methods.mm_set(msg, MSG_ATTACH_BODY, an->an_at, MSG_LAST_ATTACH);

	/* Display attachment list */
	clear = mt_set_attach_display_xgap(al);
	mt_layout_attach_display(al);
	mt_repaint_attach_canvas(al, clear);

	return;
}

static void
attach_canvas_event_proc(
	Xv_Window	canvas_pw,
	Event		*event
)
{
	Attach_list	*al;
	Attach_node	*an;
	Canvas		canvas;
	static int	drag_x, drag_y;
	static int	copy;
	static int	looking_for_drag;
	Selection_requestor sel;
	Xv_drop_site	ds;
	Xv_server	server;
	int		length, format;
	Atom		type_atom;
	char		*type_string;
	void		*data;

	canvas = (Canvas)xv_get(canvas_pw, CANVAS_PAINT_CANVAS_WINDOW);

	if (! canvas) {
		/* we use the same routine for both the canvas and the
		 * canvas paint window
		 */
		canvas = canvas_pw;
	}

	al = mt_get_attach_list(canvas);


	/*
	 * Handle events on attachment canvas paint window
	 */
	switch (event_action(event)) {
	case ACTION_DRAG_MOVE:
	case ACTION_DRAG_COPY:

		DP(("attach_canvas_event_proc: got a drop!\n"));

		mt_received_drop(canvas, event, al);
		return;

	case ACTION_DRAG_PREVIEW:
		switch(event_id(event)) {
		case LOC_WINENTER:
			DP(("attach_canvas_event_proc: LOC_WINENTER\n"));
			break;
		case LOC_WINEXIT:
			DP(("attach_canvas_event_proc: LOC_WINEXIT\n"));
			break;
		case LOC_DRAG:
			DP(("attach_canvas_event_proc: LOC_DRAG\n"));
			break;
		}
		return;

	case ACTION_SELECT:
		if (event_is_down(event)) {
			if (handle_attach_select_event(event, al) != NULL) {
				/*
				 * We were over an icon.  Prepare for drag 
				 */
				drag_x = event_x(event);
				drag_y = event_y(event);
				copy = event_ctrl_is_down(event);
				looking_for_drag = TRUE;
			}
			mt_attach_footer_message(al);
			mt_update_attach_popups(al);
		} else {
			/* the button just went up... */
			looking_for_drag = FALSE;

			an = mt_find_attach_node(al, event_x(event),
						     event_y(event));
			if (an) {
				/*
	 		         * Deselect all and reselect the one we're over
	 		         */
				mt_traverse_attach_list(al,
						    clear_attach_selection, 0);
				mt_set_attach_selection(al, an);
				mt_attach_footer_message(al);
				mt_update_attach_popups(al);
			}
		}
		return;

	case ACTION_ADJUST:
		
		if (event_is_down(event)) {
			(void)handle_attach_adjust_event(event, al);
			mt_attach_footer_message(al);
			mt_update_attach_popups(al);
		}
		return;

	case WIN_RESIZE:
		DP(("attach_canvas_event_proc: resize\n"));
		attach_resize_proc(canvas);
		return;

	case WIN_REPAINT:
		DP(("attach_canvas_event_proc: repaint\n"));
		return;

	case LOC_DRAG:
		DP(("attach_event: LOC_DRAG/event_action %d\n",
			event_action(event)));

		if (looking_for_drag) {

		DP(("attach_event: LOC_DRAG + looking_for_drag (action %d)\n",
				event_action(event)));

			if ((abs(event_x(event) - drag_x) < DAMPING) &&
			    (abs(event_y(event) - drag_y) < DAMPING))
			{
				return;
			}

			looking_for_drag = FALSE;

			/* now do the drag */
			mt_drag_attachment(al, al->al_msg, copy, 1, NULL, NULL);
		}
	}

	return;
}


static Attach_node *
handle_attach_select_event(
	Event		*event,
	Attach_list	*al
)
{
	static Event	last_event;
	Attach_node	*node;
	void		mt_busy();

	/*
	 * Handle left click on attachment canvas 
	 */

	mt_frame_msg(al->al_errorframe, FALSE, "");
	node = mt_find_attach_node(al, event_x(event), event_y(event));
	if (node == NULL) {
		/* we clicked over the background.  clear all */
		mt_traverse_attach_list(al, clear_attach_selection, 0);
		return(NULL);
	}

	/*
	 * Check for double click.
	 */
	if (ds_is_double_click(&last_event, event) && node->an_selected) {
		mt_busy(al->al_headerframe, TRUE, NULL, TRUE);

	 	mt_invoke_application(al, node, TRUE);
		sleep(1); /* Dramatic pause for effect */

		mt_busy(al->al_headerframe, FALSE, NULL, TRUE);

		return(node);
	}

	last_event = *event;

	/*
	 * Single click.  Select node if it is not already selected.
	 */
	if (!node->an_selected) {
		mt_traverse_attach_list(al, clear_attach_selection, 0);
		mt_set_attach_selection(al, node);
	}

	return(node);
}



static Attach_node *
handle_attach_adjust_event(
	Event		*event,
	Attach_list	*al
)
{
	Attach_node	*node;

	/*
	 * Handle middle click on attachment canvas
	 */
	mt_frame_msg(al->al_errorframe, FALSE, "");
	node = mt_find_attach_node(al, event_x(event), event_y(event));
	if (node == NULL)
		return(node);

	node->an_selected = !node->an_selected;
	(void) paint_attach_icon(al, node, 0);
	return(node);
}



static int
clear_attach_selection(
	Attach_list	*al,
	Attach_node	*node,
	long		unused
)
{
	/* If node is selected, deselect it and update display */
	if (node->an_selected) {
		node->an_selected = FALSE;
		(void) paint_attach_icon(al, node, 0);
	}

	return(TRUE);
}



int
mt_set_attach_selection(
	Attach_list	*al,
	Attach_node	*node
)
{
	/* Select node and update display */
	if (!node->an_selected) {
		node->an_selected = TRUE;
		(void) paint_attach_icon(al, node, 0);
	}

	return(TRUE);
}



int
mt_delete_attach_selection(
	Attach_list	*al,
	Attach_node	*an,
	long		unused
)
{
	/* If node is selected, delete it */
	if (an->an_selected) {
		attach_methods.at_set(an->an_at, ATTACH_DELETED, TRUE);
		an->an_ndelete = ++(al->al_delete_cnt);

		/*
		 * If the current attachment is a message and there is a view
		 * window associated with it, take it down.
		 */
		if ((an->an_msg != NULL) && (al->al_hd != NULL))
		{
			struct view_window_list *vwl;

			vwl = al->al_hd->hd_vwl;
			while (vwl != NULL)
			{
				if (vwl->vwl_msg == an->an_msg)
				{
					view_dismiss_proc(vwl->vwl_frame);
					break;
				}
				vwl = vwl->vwl_next;
			}
		}

		/*
	 	 * If there is a TT application displaying this
	 	 * data, take it down
	 	 */
		if (an->an_handler_id) {
			mt_send_tt_quit(NULL, an->an_handler_id);
			free(an->an_handler_id);
			an->an_handler_id = NULL;
		}

		an->an_selected = FALSE;
	}

	return(TRUE);
}




/* Make sure the specified node is visible */
static void
scroll_to_node(
	Attach_list	*al,
	Attach_node	*an
)
{
	Scrollbar	scrollbar;
	int		line;

	scrollbar = (Scrollbar)xv_get(al->al_canvas, WIN_VERTICAL_SCROLLBAR);

	line = (an->an_y / (int)xv_get(scrollbar, SCROLL_LINE_HEIGHT));
	
	(void)xv_set(scrollbar, SCROLLBAR_VIEW_START, line, 0);
}




/* Undelete the last attachment which was deleted.  Returns TRUE if it
 * succeeds, else FALSE.
 */
int
mt_undelete_attach(
	Attach_list	*al
)
{
	Attach_node	*an, *mt_find_last_deleted();

	/* Find the last attachment which was deleted */
	if ((an = mt_find_last_deleted(al)) == NULL)
		return(FALSE);

	/* Recover node and adjust delete counts */
	attach_methods.at_set(an->an_at, ATTACH_DELETED, FALSE);
	al->al_delete_cnt--;
	an->an_ndelete = 0;

	/* Clear all selections and select undeleted node */
	mt_traverse_attach_list(al, clear_attach_selection, 0);
	an->an_selected = TRUE;

	/* Make sure node is visible */
	scroll_to_node(al, an);
	
	return(TRUE);
}





#ifdef NOTUSED
int
mt_get_string_width(
	Xv_Font	font,
	char	*string
)
{
	Font_string_dims	font_size;

	/*
	 * Return the length of a string in pixels
	 */
	(void)xv_get(font, FONT_STRING_DIMS, string, &font_size);
	return(font_size.width);
}
#endif NOTUSED




/*
 * Build a command string. Handles $FILES
 */
static void
build_command(
	char	*buf,
	char	*command,
	char	*files
)
{
	int	contains_FILE;

	/* Check for $FILES in command string.  If we find it then set
	 * it so the shell will expand it for us
	 */

	*buf = '\0';
	if (contains_FILE = (int)strstr(command, "$FILE")) {
		(void)sprintf(buf, "FILE=%s;", files);
	}

	(void)strcat(buf, command);

	/* No $FILES.  Just append files */
	if (!contains_FILE) {
		(void)strcat(buf, " ");
		(void)strcat(buf, files);
	}

	return;
}



static void
set_attach_hd(
	struct attach_list *al
)
{
	Xv_Window	   pw;
	struct header_data *hd;

	if (al->al_hd == NULL)
	{
		/* 
		 * Create a temporary header data for the recursive message.
		 */
		hd = (struct header_data*)ck_malloc(sizeof(struct header_data));
		memcpy((char *)hd, (char *)mt_get_header_data(al->al_headerframe),
			sizeof(struct header_data));

		hd->hd_vwl = NULL;
		hd->hd_folder = NULL;

		/*
		 * Is this right? should we be doing
		 * hd->hd_frame = al->al_headerframe 
		 * instead ? [henley]
		 */
		hd->hd_frame = al->al_frame;
		hd->hd_canvas = al->al_canvas;
		hd->hd_cmdpanel = al->al_panel;
		hd->hd_al = al;
		hd->hd_cache_vw = FALSE;

		/* We really don't need this because we don't repaint. */
		pw = canvas_paint_window(al->al_canvas);
		hd->hd_display = (Display *) xv_get(pw, XV_DISPLAY);
		hd->hd_drawable = (Drawable) xv_get(pw, XV_XID);

		/* Set line height to 0, so mt_repaint_headers() is no-op. */
		hd->hd_lineheight = 0;

		hd->hd_findframe = NULL;
		hd->hd_findselect = NULL;
		hd->hd_findforward = NULL;

		mt_window_header_data(hd, hd->hd_frame);
		mt_window_header_data(hd, hd->hd_canvas);
		mt_window_header_data(hd, hd->hd_cmdpanel);

		al->al_hd = hd;

		DP(("%s: Creating al_hd=%x for recursive msg\n", __FILE__, hd));
	}
}




static
attach_view_msg(
	Attach_list	*al,
	int		abbrev_headers,
	struct attach	*at
)
{
		 char				*buf;
		 char				*path;
		 struct view_window_list	*mt_create_new_view();
		 struct view_window_list	*mt_find_pinned_msg();
	register struct view_window_list	*p, *new_view, *add_p;
	register struct msg			*m;
	register struct attach_node		*an;
		 int				len, view_x, view_y;
	register int                            requested_y;
		 Rect				frame_rect;
		 struct header_data		*hd;

	/*
	 * The reuse strategy is controlled by the pushpins on the View
	 * popups.  If the pushpin is in, then the window is not re-used.
	 * If the pushpin is out, then the window is re-used
	 */


	/*
	 * Scan through list of view frames.  If the frame is pinned
	 * up, don't touch it.  If a frame is unpinned, use it.
	 */
	hd = al->al_hd;
	an = (struct attach_node *)
		attach_methods.at_get(at, ATTACH_CLIENT_DATA);

	if (an->an_msg == NULL)
	{
		/*
		 * Create a temporary message and use the same msg number.
		 */
		path = attach_data_file(at);
		an->an_buffer = (char *) ck_mmap (path, &len);
		an->an_msg = msg_methods.mm_create(FALSE);
		msg_methods.mm_read(an->an_msg, an->an_buffer,
				    an->an_buffer+len);
		m = (struct msg *)attach_methods.at_get(at, ATTACH_MSG);
		an->an_msg->mo_msg_number = m->mo_msg_number;
	}

	add_p = NULL;
	for (p = hd->hd_vwl; p != NULL; p = p->vwl_next)
	{
		/*
		 * If the window is pinned don't touch it.
		 */
		if (xv_get(p->vwl_frame, FRAME_CMD_PUSHPIN_IN))
		{
			add_p = p;
			continue;
		}

		m = p->vwl_msg;

		/*
		 * Found an unused one, or the window is displaying the current
		 * message, use it. 
		 */
		if ((m == NULL) || (m == an->an_msg))
			break;

		/*
		 * Found an unpinned window.  If visible (current), change the
		 * status.  Use it.
		 */
		textsw_reset(p->vwl_textsw, 0, 0);
		if (xv_get(p->vwl_frame, XV_SHOW) && !m->mo_deleted) {
			m->mo_current = 0;
		}

		mt_clear_view_window(p);
		add_p = p;
		break;
	}

	/* view_x and view_y are relative to screen origin */
	frame_get_rect(al->al_frame, &frame_rect);
	view_y = frame_rect.r_height + frame_rect.r_top;
	view_x = frame_rect.r_left;

	/*
	 * If we couldn't find a view window, create a new one and
	 * place it into the list.  And specify not to cache any view
	 * window when the view window is done.
	 */
	if (p == NULL) 
	{
		if ((p = mt_create_new_view(al->al_frame, an->an_msg)) == NULL)
		{
			/* STRING_EXTRACTION -
			 *
			 * You have pinned all the view windows, and
			 * tried to bring up another, and you are
			 * at the maximum.  This message is displayed.
			 */
			mt_vs_warn(al->al_headerframe, gettext(
"The maximum number of View windows has\n\
been reached.  Please dismiss some of\n\
the current View windows or select fewer\n\
messages for viewing"));
			return(0);
		}

		/*
		 * Put the window in a snazy spot
		 */
		mt_position_view_window(hd, &view_x, &view_y, p->vwl_frame);

		/*
		 * Add new window to the end of the list
		 * We must do this AFTER mt_position_view_window()
		 */
		if (add_p == NULL)
			hd->hd_vwl = p;
		else
			add_p->vwl_next = p;
	}

	DP(("%s: Creating a tmp msg=%x, vwl=%x, frame=%x\n", __FILE__,
		an->an_msg, p, p->vwl_frame));

	/*
	 * Load message into view window and set status
	 */
	(void)mt_load_view(p, an->an_msg, abbrev_headers);

	return(1);
}



void
mt_tt1_invoke_application(
	Attach_list	*al,
	Attach_node	*node,
	int		use_tooltalk	/* TRUE to use tooltalk */
)
{
	char	buf[MAXPATHLEN + 1];
	char	*default_editor = "textedit";	/*Should use filemgr default? */
	int	i;
	int	contains_FILE;
	char	*path;
	void	*handler;
	int	tt_error = FALSE;  /* used to flag tt send errors */


	/* 
	 * Start up the application for node
	 */


	/* Check if this attachment type knows tool talk */
	if (use_tooltalk && node->an_open_tt != NULL && 
	    mt_tt_init("mailtool", al->al_headerframe)) {
		/*
	 	 * Check if this attachment is in the process of being launched
		 * If it is, don't launch again.
		 *
		 * ZZZ dipol: We have a possibility of screwing up here -- if
		 * the tooltalk request never completes then this attachment
		 * will remain unlaunchable until the attachment list is
		 * cleared and reloaded.
		 */
		if (node->an_pending_ttmsg) {
			return;
		}

		if (node->an_handler_id != NULL) {
			/* We've already launched this app before.  Try
			 * and re-use it.  If this load fails we will end
			 * up calling mt_invoke_application again with
			 * an_handler_id == NULL to force us to start a
			 * new app
			 */
			node->an_pending_ttmsg = mt_send_tt_dispatch_data(
				NULL, node->an_handler_id, al, node);
			return;
		} else {
			handler = ds_tooltalk_get_handler(node->an_open_tt);
			if (handler) {
				node->an_pending_ttmsg = mt_send_tt_launch(handler, al->al_frame);
				if (node->an_pending_ttmsg)
					return;  /* tt successful */
				else
					tt_error = TRUE;
			} else 
				tt_error = TRUE;
		}

		if ((tt_error) &&
		    (node->an_application == NULL ||
		    *(node->an_application) == '\0')) {
			mt_vs_warn( al->al_headerframe,
			  gettext("Sorry, Mailtool could not launch application using tooltalk."));
			return;
		}
	}

	path = node->an_at->at_file;
	if (path == NULL) {
		/* File doesn't exist yet.  Create it from message */
		path = attach_data_file(node->an_at);
		if (path == NULL)
			path = "";
	}

	i = 0;
	if ((int) attach_methods.at_get(node->an_at, ATTACH_IS_MESSAGE)) {
		/* create a header data for the attachment list because we
		 * are going to use view window to view a message or multipart.
		 */
		set_attach_hd(al);
		attach_view_msg(al, MSG_FULL_HDRS, node->an_at);
		return;

	} else if ((node->an_application == NULL ||
		   *(node->an_application) == '\0')) {

		build_command(buf, default_editor, path);
	} else {
		if (attach_methods.at_get(node->an_at, ATTACH_EXECUTABLE)) {
			int answer;

			/* STRING_EXTRACTION -
			 *
			 * This message is put up when an executable icon
			 * is opened.  We are trying to confirm whether
			 * the user needs to run the application or not
			 */
			answer = mt_vs_confirm(al->al_headerframe, 0,
				gettext("run it"), gettext("don't run it"),
				gettext(
"The selected icon %s\n\
is really an executable.\n\
Do you want to run it?"), mt_attach_name(node->an_at));

			if (! answer) return;
		}
		build_command(buf, node->an_application, path);
	}

	DP(("mt_tt1_invoke_application: running <%s>\n", buf));

	if (vfork() == 0) {
		execl("/bin/sh", "sh", "-c", buf, 0);
		exit(0);
	}
}

/* Check the (alleged) string looking for NULLs  */
/* Returns 1 if the string is contains a NULL */
/* 0 otherwise. Also called in attach_ce.c */
/* DPT Feb 97 */

int mt_check_for_nulls(char *string, int count)
{
	int i;
	for (i = 0; i < count; i++)
		if (*string++ == '\0')
			return(1);
	return(0);
}

void
mt_invoke_application(
	Attach_list	*al,
	Attach_node	*node,
	int		use_tooltalk	/* TRUE to use tooltalk	*/
)
{
	struct attach	*at;
	char		*launch_text;
	char		*sel_name;
	Atom		sel_atom;
	Xv_server	server;
	Selection	selection;
	list_t		*list = NULL;

	extern Edit_CB		ma_edit_cb;
	extern Display_CB	ma_display_cb;
	extern list_t		*find_messageid(char *);
	extern list_t		*find_messageal(Attach_list *, list_t *);

	/* STRING_EXTRACTION -
	 *
	 * Launching gets displayed when the user double clicks on an 
	 * attachment and mailtool starts up the application to display
	 * the attachment
	 */
	launch_text = gettext("Launching...");

	/* Launch message will be cleared when we get a reply */
	mt_frame_msg(al->al_errorframe, FALSE, launch_text);

	if (node->an_tt_media)
	{
		if (node->an_msgID)	/* Already dstt active */
		{
			list = find_messageid(node->an_msgID);
			dstt_raise(NULL, (void *)list, list->toolID,
				node->an_msgID, NULL);
		}
		else
		{
			char *def_type = "default";
			char *dtype = "";

			at = node->an_at;

			/*
			 * Decode the attachment from its mail message format
			 * into the real data.
			 */
			if (attach_methods.at_decode(at) != 0)
				return;
			
			if (at->at_dtype)
				dtype = at->at_dtype;
			else if (at->at_type)
				dtype = at->at_type;

			if ((at->at_buffer != NULL) && (strcmp(dtype, def_type) == 0)) {
			  if (at->at_buffer->bm_buffer != NULL) {
				if (mt_check_for_nulls(at->at_buffer->bm_buffer, at->at_buffer->bm_size)) {
					/* See comments in attach_ce.c */
					if(node->an_tt_media);
						free(node->an_tt_media);
					node->an_tt_media = NULL;
					mt_tt1_invoke_application(al, node, use_tooltalk);
					return;
				}
			  }
			}

			node->an_msgID = new_messageid(al, node, use_tooltalk);
			list = find_messageid(node->an_msgID);

			/*
			 * Convert data into suitable form for x_sel usage.
			 */
			
			/*
			 * Get Xv_Server Object associated with XView
			 * object. (i.e. get the X11 Server we are
			 * running on).
			 */
			server = XV_SERVER_FROM_WINDOW(al->al_frame);

			/*
			 * Take Xv_Server Object, create a unique atom
			 * which will live on that server.
			 */
			sel_atom = conjure_transient_selection(server,
					xv_get(server, XV_DISPLAY),
					"DESKSET_SELECTION_%d");

			/*
			 * Get the unique atom name.
			 */
			sel_name = (char *)xv_get(server,
					SERVER_ATOM_NAME, sel_atom);

			/*
			 * Use the unique atom to define a rank for
			 * the selection.
			 */
			selection = xv_create(al->al_frame, SELECTION_OWNER,
					SEL_RANK,	sel_atom,
					0);

			/*
			 * Given a selection, indicate we are the owner
			 * of said selection.
			 */
			(void)xv_set(selection,
					SEL_OWN,        TRUE,
					0);
			/*
			 * LoadOk is TRUE.
			 * No cleanup_proc.
			 * No sd_orig (or insert_selection is not needed)
			 * Attachment_node is node.
			 * Attachment_list is al.
			 *
			 * Set up data for selection.
			 */
			if (mt_attach_selection_data(selection, NULL, al, node,
				al->al_msg, TRUE, NULL, NULL))
			{

				/* Everything is okay and we can do an
				 * X selection.
				 */
				DP(("mt_invoke_application: Selection(%x) Atom(%x).\n", selection, sel_atom));
				list->sel = selection;
			}

			/*
			 * Check if this is a pending attachment (i.e. has
			 * pending flag set).
			 */
			if (node -> an_pending)
			{
 				if (list->sel)
				{
					dstt_edit(ma_edit_cb,
						(void *) list,
						node->an_tt_media,
						x_selection, sel_name, NULL,
						node->an_msgID, NULL);
				}
				else
				{
					dstt_edit(ma_edit_cb,
						(void *) list,
						node->an_tt_media, contents,
						attach_methods.at_get(node->an_at,
							ATTACH_BODY),
						(int)attach_methods.at_get(node->an_at,
							ATTACH_CONTENT_LEN),
						node->an_msgID,
						(char *)attach_methods.at_get(node->an_at,
							ATTACH_DATA_NAME));
				}

			}
			else	/* Pre-existing attachment - use dstt display */
			{
				if (list->sel)
				{
					dstt_display(ma_display_cb,
						(void *) list,
						node->an_tt_media,
						x_selection, sel_name, NULL,
						node->an_msgID,
						attach_methods.at_get(node->an_at,
								ATTACH_DATA_NAME));
				}
				else
				{
					dstt_display(ma_display_cb,
						(void *) list,
						node->an_tt_media, contents,
						attach_methods.at_get(node->an_at,
								ATTACH_BODY),
						(int)attach_methods.at_get(node->an_at,
								ATTACH_CONTENT_LEN),
						node->an_msgID,
						(char *)attach_methods.at_get(node->an_at,
								ATTACH_DATA_NAME));
				}
			}
		}
	}
	else
	{
		mt_tt1_invoke_application(al, node, use_tooltalk);
	}
}




static char *
unique_file(
	char	*path,
	char	*name
)
{
	char	*dir;
	char	*suffix;
	char	*tname;
	char	*justdir;
	int	n;

	/*
	 * Generate a unique path for file name.  We make a half-assed 
	 * attempt to preserve suffixes and prefixes so we don't screw the CE.
	 */
	if ((dir = getenv("TMPDIR")) == NULL)
		dir = "/tmp";

	/*
	 * Make sure the name doesn't conflict with Unix path and shell meta
	 * characters.  Bugid #1074831 and #1073876.
	 */
	name = tname = strdup(name);
	while (*tname != '\0')
	{
		if (strchr("$/ \\'\"[]()?*`<>;&|", *tname))
			*tname = '_';
		tname++;
	}

	/* Check if the name is unique as is */
	strcpy(path, dir);
	strcat(path, "/");
	strcat(path, name);
	if (access(path, F_OK) < 0)
	{
		free(name);
		return(path);
	}

	/* bug 1179878 retain name but add a new tmp directory */
	for (n = 0;; n++) {
		sprintf(path, "%s/tmp%d/%s", dir, n, name);
		if (access(path, F_OK) < 0) {
			justdir = malloc(strlen(dir) + 8);
			sprintf(justdir, "%s/tmp%d", dir, n);
			mkdir(justdir, 0700);
			free(justdir);
			break;	/* File does not exist */
		}
	}

	free(name);
	return(path);
}



static char *
attach_data_file(
	struct attach	*at
)
{
	char	path[MAXPATHLEN + 1];
	char	*label;

	/* Put attachment data into a file */
	label = mt_attach_name(at);
	if (*label == '\0')
		label = "NoName";

	/* Generate a unique temp file */
	unique_file(path, label);

	/*
	 * This decodes the data and puts it into path
	 * The data can live permanently outside of a message if
	 * ATTACH_DATA_EXT_FILE is specified, or stay as a temporay file
	 * with ATTACH_DATA_TMP_FILE.
	 */
	attach_methods.at_read(at, path, ATTACH_DATA_TMP_FILE);

	/* if the file is supposed to be executable, make it so... */
	if (attach_methods.at_get(at, ATTACH_EXECUTABLE)) {
		mode_t mask;
		struct stat buf;

		mask = umask(0);
		(void) umask(mask);

		/* the umask is normally the bits to be turned off.
		 * change this into the execute bits to turn on
		 */
		mask = ~mask & 0111;

		/* get the current mode */
		if (stat(path, &buf) == 0) {

			/* add in the proper execute bits */
			chmod(path, buf.st_mode | mask);
		}


	}

	return(at->at_file);
}



Attach_node*
mt_fcreate_attachment(
	Attach_list	*al,
	char		*path,
	char		*label,
	int		tmpfile	/*TRUE if path points to a temporary file*/
)
{
	Attach_node	*an;
	struct stat	statb;
	struct attach	*at, *mt_create_at();

	/* STRING_EXTRACTION -
	 *
	 * Add attachment to list.  Return node if successfull, else NULL
	 * The first argument is the pathname; the second is the system error
	 * string
	 */

	/* Make sure we can read file */
	/* Stat and check for other error conditions */
	if (access(path, R_OK) < 0 || stat(path, &statb) < 0) {
		mt_vs_warn(al->al_errorframe,
			gettext("Could not add attachment\n%s\n%s"),
				path, strerror(errno));
		return(NULL);
	};

	/* STRING_EXTRACTION -
	 *
	 * The user has just added an attachment via the Add... popup.
	 * The following messages come up if the file is a directory
	 * or some special file (ie a device, pipe, etc).
	 */
	/* Make sure the attachment is not a directory.  */
	if (S_ISDIR(statb.st_mode)) {
		mt_vs_warn(al->al_errorframe,
gettext("Sorry, you may not add directories to the attachment list.\n\
You may add the contents of the directory by\nadding the files individually."));
		return(NULL);
	}

	/* Make sure it is not a device, pipe, etc */
	if (!S_ISREG(statb.st_mode)) {
		mt_vs_warn(al->al_errorframe,
	gettext("The file you specified is not a data file and\n\
		 cannot be added as an attachment."));
		return(NULL);
	}

	/* Check if attachment is too big or will make message too big */
	if (!valid_attach_size(al, statb.st_size))
		return(NULL);

	/*
	 * If tmpfile is TRUE then 'path' will be removed when the
	 * at is destroyed.
	 */
	if ((at = mt_create_at(path, label, tmpfile)) == NULL) {
		mt_vs_warn(al->al_errorframe,
			gettext("Could not add attachment\n%s"), path);
		return(NULL);
	};

	/* Allocate a new node */
	if ((an = mt_get_attach_node(at)) == NULL) {
		mt_vs_warn(al->al_errorframe,
			gettext("Could not add attachment: Out of memory"));
		attach_methods.at_destroy(at);
		return(NULL);
	}

	if (label && *label == '\0')
		label = NULL;

	mt_init_attach_node(al, an, at, &statb);

	return(an);
}



/*
 * actually initiate the drag
 *
 * Note:
 * Dragging out of the mailtool header list creates a dummy message
 * with attachments -- one attachment for each message being
 * dragged -- and uses this routine to do the DnD. We need to free
 * up the message and attachments when we are done.  That is what
 * cleanup_proc is for .  In the typical attachment DnD case
 * cleanup_proc is NULL since there is no dummy copy to get rid of.
 */
void
mt_drag_attachment(
	Attach_list	*al,
	struct msg	*msg,
	int	copy,
	int	loadok,
	void	(*cleanup_proc)(), /* Additional cleanup-proc.  May be NULL */
	void	*cleanup_arg
)
{ 

	Server_image	feedback_image;
	Pixmap		feedback_pixmap;
	int		items_selected;
	int		rcode;
	Attach_node	*an;
	Display 	*dpy;
	Dnd		dnd;
	struct		attach *at;
	static		Server_image document_multiple;
	static		Server_image scratch_image;
	static		Server_image copy_image;
	static		Server_image move_image;
	static		Pixmap document_pixmap;
	static		Pixmap scratch_pixmap;
	static		Pixmap copy_pixmap;
	static		Pixmap move_pixmap;
	static		Xv_Cursor feedback_cursor;
	static		GC gc;
	char		*framemsg;

	/* find a selected node.  this is kind of broken because
	 * the current dnd doesn't allow multiple object drags:
	 * we arbitrarily select the first one...
	 */

	items_selected = 0;
	feedback_image = NULL;
	at = attach_methods.at_first(msg);
	while (at) {

		an = (Attach_node *)
			attach_methods.at_get(at, ATTACH_CLIENT_DATA);

		if (an && an->an_selected) {
			items_selected++;
			if (! feedback_image) feedback_image = an->an_icon;
		}

		at = attach_methods.at_next(at);
	}

	if (items_selected == 0) {
		/* what? no one is selected?  Oh, well, just go away */
		return;
	}

	if (document_multiple == NULL) {

		document_multiple = xv_create(0, SERVER_IMAGE,
			XV_WIDTH, 32,
			XV_HEIGHT, 32,
			SERVER_IMAGE_BITS, many_image_icon,
			0);

		if (! document_multiple) {
			/* allocation failed... */
			return;
		}

		document_pixmap =
			xv_get(document_multiple, SERVER_IMAGE_PIXMAP);
	}

	if (scratch_image == NULL) {
		scratch_image = xv_create(0, SERVER_IMAGE,
			XV_WIDTH, 32 + 3,
			XV_HEIGHT, 32 + 4,
			0);

		if (! scratch_image) {
			/* allocation failed... */
			return;
		}

		scratch_pixmap = xv_get(scratch_image, SERVER_IMAGE_PIXMAP);
	}

	dpy = al->al_display;
	feedback_pixmap = xv_get(feedback_image, SERVER_IMAGE_PIXMAP);

	if (gc == NULL) {
		gc = XCreateGC(dpy, feedback_pixmap, 0, 0);

		if (! gc) return;
	}

	/* clear the icon */
	XSetFunction(dpy, gc, GXclear);
	XCopyArea(dpy, scratch_pixmap, scratch_pixmap,
		gc,
		0, 0,
		32 + 3, 32 + 4,
		0, 0);

	/* copy the icon to the scratch area */
	XSetFunction(dpy, gc, GXcopy);
	XCopyArea(dpy, feedback_pixmap, scratch_pixmap,
		gc,
		0, 0,
		32, 32,
		0, 0);


	/* OR in the multiple image plane */
	if (items_selected > 1) {

		/* or in the multiple icon */
		XSetFunction(dpy, gc, GXor);
		XCopyArea(dpy, document_pixmap, scratch_pixmap,
			gc,
			0, 0,
			32, 32,
			3, 4);
	}

	if (! copy_pixmap) {
		copy_image = xv_create(0, SERVER_IMAGE,
			XV_WIDTH, 16,
			XV_HEIGHT, 16,
			SERVER_IMAGE_BITS, copy_image_bits,
			0);

		if (! copy_image) return;

		copy_pixmap = xv_get(copy_image, SERVER_IMAGE_PIXMAP);
	}

	if (! move_pixmap) {
		move_image = xv_create(0, SERVER_IMAGE,
			XV_WIDTH, 16,
			XV_HEIGHT, 16,
			SERVER_IMAGE_BITS, move_image_bits,
			0);

		if (! move_image) return;

		move_pixmap = xv_get(move_image, SERVER_IMAGE_PIXMAP);
	}

	/* XOR in the proper cursor... */
	XSetFunction(dpy, gc, GXor);
	XCopyArea(dpy, copy? copy_pixmap : move_pixmap, scratch_pixmap,
		gc,
		0, 0,
		16, 16,
		20, 20);


	/* create and set the cursor */
	if (! feedback_cursor) {
		feedback_cursor = xv_create(0, CURSOR,
			CURSOR_XHOT, 20,
			CURSOR_YHOT, 20,
			CURSOR_OP, PIX_SRC^PIX_DST,
			0);

		if (! feedback_cursor) return;
	}
	xv_set(feedback_cursor, CURSOR_IMAGE, scratch_image, 0);

	dnd = xv_create(al->al_canvas, DRAGDROP,
		DND_TYPE,	copy ? DND_COPY : DND_MOVE,
		DND_CURSOR,	feedback_cursor,
		0);

	DP(("drag_attachment: copy %d, dnd %x, items_selected %d, canvas %x\n",
		copy, dnd, items_selected, al->al_canvas));

	if (! dnd) return;

	if (! mt_attach_selection_data(dnd, NULL, al, NULL, msg, loadok,
	       cleanup_proc, cleanup_arg))
	{
		xv_destroy(dnd);
		return;
	}

	/* This has been moved to before the dnd_send_drop() because
	 * send_drop is "instantaneous" if we are dropping in the same
	 * process.  Therefore when you see the DND_OK later then
	 * you don't want to overwrite the "Drag and Drop completed"
	 * message.
	 */
	mt_frame_msg(al->al_errorframe, FALSE,
		gettext("Data transfer in progress..."));
	framemsg = NULL;

	rcode = dnd_send_drop(dnd);

	switch (rcode) {
	case XV_OK:
		DP(("drag attachment: OK\n"));
		break;

	case DND_TIMEOUT:
		DP(("drag attachment: timed out\n"));
		framemsg = gettext("Data Transfer: timed out");
		break;

	case DND_ILLEGAL_TARGET:
		DP(("drag attachment: illegal target\n"));
		framemsg = gettext("Drag and Drop: illegal target");
		break;

	case DND_SELECTION:
		DP(("drag attachment: bad selection\n"));
		framemsg = gettext("Data Transfer: bad selection");
		break;

	case DND_ROOT:
		DP(("drag attachment: root window\n"));
		framemsg = gettext("Drag and Drop on root window");
		break;

	case XV_ERROR:
		DP(("drag attachment: dnd failed\n"));
		framemsg = gettext("Drag and Drop failed");
		break;
	}

	if (framemsg) {
		mt_frame_msg(al->al_errorframe, FALSE, framemsg);
	}

	if (rcode != XV_OK && cleanup_proc != NULL) {
	        extern void mt_attach_remove_cleanup_proc();
		/* Cleanup on error */
		(*cleanup_proc)(al);
		/* remove the cleanup proc */
		mt_attach_remove_cleanup_proc(dnd);
	}

	DP(("drag_attachment: done\n"));
}




/*
 * Destroy all nodes in an attachment list and clear the attachment canvas
 */
void
mt_clear_attachments(
	Attach_list	*al
)
{
	Scrollbar	scrollbar;

	/* Make sure attachment list exists before we clear it */
	if (al->al_canvas == NULL)
		return;

	/* Destroy all attachments in the list */
	mt_destroy_attach_nodes(al);

	/* Scroll to top of canvas so we are positioned correctly for next
	 * time.
	 */
	scrollbar = (Scrollbar)xv_get(al->al_canvas, WIN_VERTICAL_SCROLLBAR);
	(void)xv_set(scrollbar, SCROLLBAR_VIEW_START, 0, 0);

	/* Force a repaint so attachment list is cleared */
	mt_repaint_attach_canvas(al, TRUE);

	return;
}




/*
 * Load attachments from an existing message into a textsw and attachment list
 */
void
mt_load_attachments(
	Textsw	textsw,
	Attach_list	*al,
	struct attach	*at,
	int	skip_deleted
)
{
	Attach_node	*node;
	char *tmp_holder;

	/*
	 * If the first attachment is text, load it into the textsw.
	 */
	if (textsw != NULL && mt_is_text_attachment(at)) {
		if (!attach_methods.at_get(at, ATTACH_DELETED)) {
			extern int mt_insert_textsw();

			if (attach_methods.at_decode(at) != 0) {
				return;
			}

			if (attach_methods.at_copy(at, AT_BODY,
				mt_insert_textsw, (void *) textsw) != 0)
			{
				return;
			}
		}
		if (skip_deleted)
			at = mt_get_next_attach(at);
		else
			at = attach_methods.at_next(at);

		/* don't show the attachment pane if there is no attachment */ 
		if (at == NULL) {
			return;
		}
	}

	while (at != NULL) {
		if ((node = mt_get_attach_node(at)) == NULL) {
			return;
		}

		/*Attachment name encoding convertion */
		if (at->at_name){
                   tmp_holder=at->at_name;
                   at->at_name=(char *)mm_enconv(at->at_name,strlen(at->at_name));
                   free(tmp_holder);
		}

		mt_init_attach_node(al, node, at, NULL);
		if (skip_deleted)
			at = mt_get_next_attach(at);
		else {
			/* Mark the deleted attachment as selected */
			if (attach_methods.at_get(at, ATTACH_DELETED))
				node->an_selected = 1;
			at = attach_methods.at_next(at);
		}
	}

	/* Reconstruct the delete list from selected attachments */
	if (!skip_deleted) {
		mt_traverse_attach_list(al, mt_delete_attach_selection, 0);
	}

	/* Display attachment list */
	mt_set_attach_display_xgap(al);
	mt_layout_attach_display(al);
	mt_repaint_attach_canvas(al, FALSE);
}



/*
 * Get next attachment skipping delete ones
 */
struct attach *
mt_get_next_attach(
	struct attach	*at
)
{
	do {
		at = attach_methods.at_next(at);
	} while (at != NULL && attach_methods.at_get(at, ATTACH_DELETED));

	return(at);
}



int
mt_is_text_attachment(
	struct attach	*at
)
{
	/*
	 * Check if the attachment is of a type which should be
	 * displayed in the view or compose textsw
	 */
	return ((int) attach_methods.at_get(at, ATTACH_IS_TEXT));
}



void
mt_rename_attachment(
	Attach_list	*al,
	Attach_node	*node,
	char		*label
)
{
	int	clear;
	Font_string_dims	font_size;

	attach_methods.at_set(node->an_at, ATTACH_DATA_NAME, label);
	node->an_label_length = strlen(label);
	(void)xv_get(al->al_font, FONT_STRING_DIMS, label, &font_size);
	node->an_label_width  = font_size.width;

	mt_set_attach_display_xgap(al);
	mt_layout_attach_display(al);
	scroll_to_node(al, node);
	mt_repaint_attach_canvas(al, TRUE);
}



/* set up the dragndrop list for the canvas */
static void
set_dragdrop(
	Attach_list	*al
)
{

	al->al_drop_site = xv_create(al->al_canvas, DROP_SITE_ITEM,
		DROP_SITE_ID,	1,
		DROP_SITE_EVENT_MASK, DND_MOTION | DND_ENTERLEAVE,
		0);

	if (!al->al_drop_site) {
		mt_vs_warn(al->al_errorframe, gettext(
"Internal error: could not allocate a drop site for this window.\n\
This means that you cannot use drag and drop within this window.\n"));
		return;
	}
}


Attach_list *
mt_get_attach_list(
	Xv_opaque win
)
{
	Attach_list *al;

	if (! KEY_ATTACH_LIST) {
		mt_abort("mt_get_attach_list: null KEY_ATTACH_LIST\n",
			0, 0, 0, 0);
	}

	al = (Attach_list *) xv_get(win, XV_KEY_DATA, KEY_ATTACH_LIST);

	if (! al) {
		mt_abort("mt_get_attach_list: null Attach_list\n", 0, 0, 0, 0);
	}

	return (al);
}



void
mt_set_attach_list(
	Xv_opaque win,
	Attach_list *al
)
{
	if (! KEY_ATTACH_LIST) {
		KEY_ATTACH_LIST = xv_unique_key();
	}

	xv_set(win, XV_KEY_DATA, KEY_ATTACH_LIST, al, 0);
}
