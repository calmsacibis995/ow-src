#ifndef lint
static  char sccsid[] = "@(#)view.c 3.20 07/08/97 Copyr 1987 Sun Micro";
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
 * Mailtool - Mail subprocess handling
 */

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <signal.h>
#include <rpcsvc/ypclnt.h>

#include <xview/panel.h>
#include <xview/text.h>
#include <xview/xview.h>
#include <xview/font.h>
#include <xview/scrollbar.h>
#include <xview/sel_svc.h>
#include <xview/sel_attrs.h>
#include <xview/dragdrop.h>

#include "glob.h"
#include "tool.h"
#include "tool_support.h"
#include "header.h"
#include "mail.h"
#include "mle.h"
#include "attach.h"
#include "cmds.h"
#include "ds_popup.h"
#include "create_panels.h"

#ifdef PEM
#include <fcntl.h>
#include "/home/internet/pem-5.0/src/h/pem.h"

extern char *makeheader();
#endif PEM

#define IS_POPUP_KEY	100

extern	Seln_client	header_client;

static void	(*cached_mt_view_window_notify)();

extern char	*strstr();
extern void mt_text_insert();
extern int at_has_coding();

#define VWL_KEY		300   /* also in attach_panel.c */
#define DEBUG_FLAG mt_debugging
extern DEBUG_FLAG;
#include "debug.h"

#ifdef OLD_AUDIO
extern caddr_t	mt_audio_panel_handle;
extern Frame	mt_audio_panel_owner;
#endif



static void
mt_view_window_notify_proc(textsw, attributes)
	Textsw		textsw;
	Attr_avlist	attributes;
{
	Attr_avlist	attrs;
	struct view_window_list *view;
	Textsw		parent;

	/*
	 * katin: this textsw is *different* from the one
	 * that we used in textsw_insert().  We need to get
	 * the WIN_PARENT of this textsw to get back to the
	 * one that we know about
	 */
	parent = xv_get(textsw, WIN_PARENT);
	view = (struct view_window_list *) xv_get(parent, WIN_CLIENT_DATA);

	for (attrs = attributes; *attrs; attrs = attr_next(attrs)) 
	{
		switch ((Textsw_action) (*attrs)) {
		case TEXTSW_ACTION_EDITED_MEMORY:
		case TEXTSW_ACTION_EDITED_FILE:
			/* mark this window has having been changed */
			DP(( "view_window_notify_proc/EDITED_%s\n",
				(*attrs == TEXTSW_ACTION_EDITED_MEMORY)?
				"MEMORY" : "FILE" ));
			view->vwl_modified = TRUE;
			break;

		case TEXTSW_ACTION_USING_MEMORY:
			/* mark this window has having been changed */
			DP(( "view_window_notify_proc/USING_MEMORY\n"));
			view->vwl_modified = TRUE;
			break;

#ifdef NEVER
		/*
		 * textsw "Store as New File" generates this event 
		 * and we've disabled "Load", so there is no reason
		 * for this event to mean the textsw has been modified
		 */
		case TEXTSW_ACTION_LOADED_FILE:
			/* mark this window has having been changed */
			DP(( "view_window_notify_proc/LOADED_FILE\n"));
			view->vwl_modified = TRUE;
			break;
#endif
		}
	}

	(*cached_mt_view_window_notify)(textsw, attributes);
}

/*
 * user is quitting a popup reply window. This takes care of removing the
 * window from the tools data structures 
 */
static Notify_value 
mt_destroy_view_proc(client, status)
	Notify_client	client;
	Destroy_status	status;
{

	struct	view_window_list	*vwl;
	struct	view_window_list	*parent;
	struct	msg			*m;
	struct	header_data		*hd;

	if (status == DESTROY_CHECKING) 
	{
		vwl = (struct view_window_list *)xv_get(client,WIN_CLIENT_DATA);
		hd = mt_get_header_data(vwl->vwl_headerframe);
		parent = vwl = hd->hd_vwl;

		while (vwl)
		{
			if (client != (Notify_client)vwl->vwl_frame) 
			{
				/* not the correct view window, get next one */
				parent = vwl;
				vwl = vwl->vwl_next;
			}
			else
			{
				
				/* scan thru the list to find the data for
				 * this frame.
				 */

				m = vwl->vwl_msg;
				
				if (m != NULL && !m->mo_deleted)
				{
					if (m->mo_selected || m->mo_current)
					{
						m->mo_selected = 0;
						m->mo_current = 0;
						m->mo_new = 0;
						m->mo_read = 1;

						force_repaint_on_line(
							m->m_lineno, hd);
					}
				}

				mt_clear_view_window(vwl);
				textsw_reset(vwl->vwl_textsw, 0, 0);

				/* found it.  splice it out of chain */
				if (parent == vwl) {
					hd->hd_vwl = vwl->vwl_next;
				} else {
					parent->vwl_next = vwl->vwl_next;
				}

				free(vwl);

				break;
			}
		}
	} 
	(void) xv_set(client, FRAME_NO_CONFIRM, TRUE, 0);
	return (notify_next_destroy_func(client, status));
}

mt_clear_views(hd)

	struct	header_data	*hd;

{
	register struct	view_window_list *p;

	/* this functions takes all the existing view windows, 
	   and clears them so that they are ready for re-use
	*/

	for (p = hd->hd_vwl; p != NULL; p = p->vwl_next)
	{
		mt_clear_view_window(p);
	}
		
}

mt_view_windows_up(hd)
	struct header_data	*hd;

{
	register struct view_window_list *p;
	register int			n;

	/*
	 * Count the number of view windows currently up.  If we end
	 * up calling this routine lots which should switch to a global
	 * keeping track of this value
	 */
	n = 0;
	for (p = hd->hd_vwl; p != NULL; p = p->vwl_next)
	{
		if (xv_get(p->vwl_frame, XV_SHOW))
			n++;
	}

	return(n);
}

struct view_window_list*
mt_find_pinned_msg(hd, m)

	struct header_data	*hd;
	struct msg		*m;

{
	register struct	view_window_list	*p;

	/*
	 * Check and see if msg is in a pinned view window
	 */
	for (p = hd->hd_vwl; p != NULL; p = p->vwl_next)
	{
		if (p->vwl_msg == NULL)
			continue;

		if (p->vwl_msg == m &&
			xv_get(p->vwl_frame, FRAME_CMD_PUSHPIN_IN))
		{
			break;
		}
	}
	return(p);
}

static
view_window_at(hd, x, y)

	struct header_data *hd;
	register int	x, y;

{
	int	win_x, win_y;
	Rect	frame_rect;
	register struct	view_window_list	*p;

	/*
	 * Check and see if there is a view window within a couple of pixels
	 * of x, y
	 */
	for (p = hd->hd_vwl; p != NULL; p = p->vwl_next) {
		frame_get_rect(p->vwl_frame, &frame_rect);
		win_x = frame_rect.r_left;
		win_y = frame_rect.r_top;
		if (x < win_x + 5 && x > win_x - 5 && y < win_y + 5 && 
			y > win_y - 5)
			return(TRUE);
	}
	return(FALSE);
}

mt_create_views(hd, abbrev_headers, flag)

	struct header_data *hd;
	int	abbrev_headers;
	Msg_attr	flag;

{
		 struct view_window_list	*mt_create_new_view();
	register struct view_window_list	*p, *new_view, *add_p;
	register struct msg			*m;
		 struct msg			*mt_load_view();
		 int				view_x, view_y;
	register int				n_msgs = 0;
	register int				requested_y;
		 int				rcode;
		 Rect				frame_rect;

	/*
	 * The reuse strategy is controlled by the pushpins on the View
	 * popups.  If the pushpin is in, then the window is not re-used.
	 * If the pushpin is out, then the window is re-used
	 */


	/*
	 * Scan through list of view frames.  If the frame is pinned
	 * up, don't touch it.  If a frame is unpinned but is displaying
	 * a message which is also selected, don't touch it.  Otherwise
	 * mark the window as garbage
	 */
	for (p = hd->hd_vwl; p != NULL; p = p->vwl_next)
	{
		m = p->vwl_msg;

		/*
		 * If the window is pinned don't touch it.
		 */
		if (xv_get(p->vwl_frame, FRAME_CMD_PUSHPIN_IN))
			continue;

		textsw_reset(p->vwl_textsw, 0, 0);
		if (m == NULL) continue;

		/*
		 * If we are re-displaying current messages then don't
		 * reset the message state on current messages
		 */
		if (flag == MSG_CURRENT && m->mo_current) {
			mt_clear_view_window(p);
			m->mo_current = 1; /* Preserve that it is current */
			continue;
		}

		/*
		 * Unpinned window.  If visible (current), change the status
		 */
		if (xv_get(p->vwl_frame, XV_SHOW) && !m->mo_deleted) {
			m->mo_current = 0;
			force_repaint_on_line(m->m_lineno, hd);
		}

		mt_clear_view_window(p);
	}

	/* view_x and view_y are relative to screen origin */
	frame_get_rect(MT_FRAME(hd), &frame_rect);
	view_y = frame_rect.r_height + frame_rect.r_top;
	view_x = frame_rect.r_left;

	/*
	 * Scan through non-deleted messages and display selected ones
	 */
	for (m = FIRST_NDMSG(CURRENT_FOLDER(hd)); m != NULL; m = NEXT_NDMSG(m)) 
	{
		/*
		 * Skip over messages which are not selected
		 */
		if (!msg_methods.mm_get(m, flag))
			continue;

		/*
		 * Check if the message is already in a pinned view. If it
		 * is re-use the pinned view, else find an old view to use
		 */
		p = mt_find_pinned_msg(hd, m);
		if (p == NULL)
		{
			/*
		 	 * find a view window to use.
		 	 */
			add_p = NULL;
			for (p = hd->hd_vwl; p != NULL; p = p->vwl_next)
			{
				if (p->vwl_msg == NULL)
					break;
				add_p = p;
			}
		}

		/*
		 * If we couldn't find a view window, create a new one and
		 * place it into the list.
		 */
		if (p == NULL) 
		{
			if ((p = mt_create_new_view(hd->hd_frame, m)) == NULL)
			{
				/* STRING_EXTRACTION -
				 *
				 * You have pinned all the view windows, and
				 * tried to bring up another, and you are
				 * at the maximum.  This message is displayed.
				 */
				mt_vs_warn(MT_FRAME(hd),
					gettext(
"The maximum number of View windows has\n\
been reached.  Please dismiss some of\n\
the current View windows or select fewer\n\
messages for viewing"));
				break;
			}

			/*
			 * Put the window in a snazy spot
			 */
			mt_position_view_window(hd, &view_x, &view_y,
					p->vwl_frame);

			/*
			 * Add new window to the end of the list
			 * We must do this AFTER mt_position_view_window()
			 */
			if (add_p == NULL)
				hd->hd_vwl = p;
			else
				add_p->vwl_next = p;
		}

		if (m->mo_new && mt_new_count) {
			mt_new_count--;
		}

		/*
		 * Load message into view window and set status
		 */
		MT_CURMSG(hd) = mt_load_view(p, m, abbrev_headers);
		n_msgs++;
	}

	/*
	 * Hide unused windows
	 */
	mt_hide_view_garbage(hd);
	mt_update_folder_status(hd);

	return(n_msgs);
}

#ifdef PEM
/* Create view windows with decrypted PEM messages */
mt_create_PEM_views(hd, abbrev_headers, flag)

	struct header_data *hd;
	int	abbrev_headers;
	Msg_attr	flag;
{
		 struct view_window_list		*mt_create_new_view();
	register struct view_window_list		*p, *new_view, *add_p;
	register struct msg				*m;
		 struct msg				*mt_load_decrypted_view();
		 int					view_x, view_y;
	register int					n_msgs = 0;
		 int					rcode;
		 Rect					frame_rect;
	
	/*
	 * Scan through list of view frames. If the frame is pinned
	 * up and is displaying a message which isn't selected, don't
	 * touch it.  Otherwise mark the window as garbage, because
	 * even if a frame is displaying a selected message, we need
	 * to replace it with the decrypted message anyway.
	 */
	for (p = hd->hd_vwl; p != NULL; p = p->vwl_next) 
	{
		m = p->vwl_msg;

		/*
		 * If the window is pinned don't touch it.
		 */
		if (xv_get(p->vwl_frame, FRAME_CMD_PUSHPIN_IN))
			continue;
		
		textsw_reset(p->vwl_textsw, 0, 0);
		if (m == NULL) continue;

		/*
		 * If we are re-displaying current messages then don't
		 * reset the message state on current messages
		 */
		if (flag == MSG_CURRENT && m->mo_current) {
			mt_clear_view_window(p);
			m->mo_current = 1; /* Preserve that it is current */
			continue;
		}

		/*
		 * Unpinned window.  If visible (current), change the status
		 */
		if (xv_get(p->vwl_frame, XV_SHOW) && (!m->mo_deleted)) {
			m->mo_current = 0;
			force_repaint_on_line(m->m_lineno, hd);
		}

		mt_clear_view_window(p);
	}

	/* view_x and view_y are relative to screen origin */
	frame_get_rect(MT_FRAME(hd), &frame_rect);
	view_y = frame_rect.r_height + frame_rect.r_top;
	view_x = frame_rect.r_left;

	/*
	 * Scan through non-deleted messages and display selected ones
	 */
	for (m = FIRST_NDMSG(CURRENT_FOLDER(hd)); m != NULL; m = NEXT_NDMSG(m)) 
	{
		/*
		 * Skip over messages which are not selected
		 */
		if (!msg_methods.mm_get(m, flag))
			continue;
		
		/*
		 * Check if the message is already in a pinned view.  If it
		 * is, reuse the pinned view; otherwise, find an old view to
		 * use.
		 */
		p = mt_find_pinned_msg(hd, m);
		if (p == NULL) 
		{
			/*
			 * find a view window to use.
			 */
			add_p = NULL;
			for (p = hd->hd_vwl;p != NULL;p=p->vwl_next) {
				if (p->vwl_msg == NULL)
					break;
				add_p = p;
			}
		}

		/*
		 * If we couldn't find a view window, create a new one and
		 * place it in the list.
		 */
		if (p == NULL) 
		{
			if ((p = mt_create_new_view(hd->hd_frame, m)) == NULL)
			{
				/* STRING_EXTRACTION -
				 *
				 * You have pinned all the view windows and
				 * tried to bring up another, and you are
				 * at the maximum.  This message is displayed.
				 */
				mt_vs_warn(MT_FRAME(hd),
					gettext(
"The maximum number of View windows has\n\
been reached.  Please dismiss some of\n\
the current View windows or select fewer\n\
messages for viewing"));
				break;
			}

			/*
			 * Put the window in a snazzy spot
			 */
			mt_position_view_window(hd, &view_x, &view_y,
					p->vwl_frame);

			/*
			 * Add new window to the end of the list
			 * We must do this AFTER mt_position_view_window()
			 */
			if (add_p == NULL)
				hd->hd_vwl = p;
			else
				add_p->vwl_next = p;
		}

		if (m->mo_new && mt_new_count) {
			mt_new_count--;
		}

		/*
		 * Load message into view window and set status
		 */
		MT_CURMSG(hd) = mt_load_decrypted_view(p, m, abbrev_headers);
		n_msgs++;
	}

	/*
	 * Hide unused windows
	 */
	mt_hide_view_garbage(hd);
	mt_update_folder_status(hd);

	return(n_msgs);
}

#endif PEM

mt_position_view_window(hd, x_p, y_p, frame)

	struct header_data *hd;
	int	*x_p, *y_p;	/* x, y, relative to screen origin */
	Frame	frame;
{
	int	old_x = -9999, old_y = -9999;
	Rect	frame_rect;

	/*
	 * Find a good spot to put the view window.  Make sure it doesn't
	 * go off of the screen, or directly cover up another View window.
	 * Keep trying until we find a good spot.  We must also detect
	 * if we get stuck in the lower right corner.  If this happens jump
	 * to 0,0 relative to the screen.
	 */
	while (TRUE) {
		ds_force_popup_on_screen(x_p, y_p, MT_FRAME(hd), frame);
		if (view_window_at(hd, *x_p, *y_p)) {
			/*
			 * We have a view window under use.  Try again
			 */
			if (*x_p == old_x && *y_p == old_y) {
				/*
				 * ds_force_popup_on_screen() has tried a spot
				 * twice in a row.  We must be stuck in the 
				 * lower right hand corner.  Break out!
				 */
				old_x = *x_p;
				old_y = *y_p;
				*x_p = 0;
				*y_p = 0;
			} else {
				old_x = *x_p;
				old_y = *y_p;
				*x_p += 15;
				*y_p += 30;
			}
		} else 
			break;
	}
}

mt_hide_view_garbage(hd)
	struct header_data *hd;


{
	register struct view_window_list	*list;

	/*
	 * Loop through and take down any garbage windows. We could
	 * destroy them, but for now we'll just hide them.
	 */
	for (list = hd->hd_vwl; list != NULL; list = list->vwl_next)
	{
		if (list->vwl_msg == NULL)
			xv_set(list->vwl_frame, XV_SHOW, FALSE, 0);
	}
}


/* Return the current message.  Normally, the caller set the returned value
 * to "MT_CURMSG"
 */
struct msg *
mt_load_view(vwl, m, abbrev_headers)

	struct view_window_list	*vwl;
	struct msg		*m;
	int			abbrev_headers;
{
	char	header_msg[80];
	struct stat	statb;
	struct header_data *hd;
	int lower_context;
	int		i;
	Frame		popup;
	struct attach	*at;
	Scrollbar	scrollbar;
	int		again_recording;
	extern int mt_insert_textsw_single();

	/*
	 * Load a message into a view popup and set the message status
	 * and glyph
	 */
	vwl->vwl_garbage = FALSE;
	vwl->vwl_msg = m;

	vwl->vwl_al->al_msg = m;

	/* STRING_EXTRACTION -
	 *
	 * "View Message %d" is the title line for the view window.  %d
	 * is the message number.
	 */
	sprintf(header_msg, gettext("View Message %d"), m->mo_msg_number);
	mt_label_frame(vwl->vwl_frame, header_msg);

	/* Usually redunant, sometimes necessary */
	textsw_reset(vwl->vwl_textsw, 0, 0);
	mt_text_clear_error(vwl->vwl_textsw);

	/* Make sure scrollbar is positioned at top of textsw */
	scrollbar = (Scrollbar)xv_get(vwl->vwl_textsw, WIN_VERTICAL_SCROLLBAR);
	if ((int)xv_get(scrollbar, SCROLLBAR_VIEW_START) != 0) {
		(void)xv_set(scrollbar, SCROLLBAR_VIEW_START, 0, 0);
	}

	/* change the lower context so the screen doesn't scroll */
	lower_context = xv_get(vwl->vwl_textsw, TEXTSW_LOWER_CONTEXT);
	xv_set(vwl->vwl_textsw, TEXTSW_LOWER_CONTEXT, -1, 0);


	/* Cycle through and unshow all previous attachment popups, 
		if any, for attachments */
	i = 1;
	while ((popup = xv_get(vwl->vwl_frame, FRAME_NTH_SUBFRAME, i)) != NULL)
	{
		i++;
		(void)xv_set(popup, WIN_SHOW, FALSE, 0);
	}

	hd = mt_get_header_data(vwl->vwl_headerframe);

	/* Turn off "again recording" to improve insertion speed */
	again_recording = (int)xv_get(vwl->vwl_textsw, TEXTSW_AGAIN_RECORDING);
	(void)xv_set(vwl->vwl_textsw, TEXTSW_AGAIN_RECORDING, FALSE, 0);
	/*
	 * If the message has attachments then display them
	 */
	if ((at = attach_methods.at_first(m)) != NULL) {
		/* if the only body part is text, don't show it as attachment */
		if (mt_is_text_attachment(at) && !attach_methods.at_next(at))
			mt_show_attach_list(vwl->vwl_al, FALSE);
		else
			mt_show_attach_list(vwl->vwl_al, TRUE);

		if (vwl->vwl_textsw != NULL && mt_is_text_attachment(at) && 
		    find_at2msg(m) != NULL && !at_has_coding(at))
			mm_copyattach(m, abbrev_headers, 0, mt_insert_textsw_single, vwl->vwl_textsw, 1);
		else {
			mt_print_textsw_headers(hd, m, vwl->vwl_textsw, abbrev_headers);
			mt_text_insert(vwl->vwl_textsw, "\n", 1);
			mt_load_attachments(vwl->vwl_textsw, vwl->vwl_al, at, FALSE);
		}
	} else {
		mt_show_attach_list(vwl->vwl_al, FALSE);
		mt_print_textsw(hd, m, vwl->vwl_textsw, abbrev_headers);
	}
	(void)xv_set(vwl->vwl_textsw, TEXTSW_AGAIN_RECORDING, again_recording,
		     0);

	xv_set(vwl->vwl_textsw, TEXTSW_LOWER_CONTEXT, lower_context, 0);

	/*
	 * HACK ALERT:katin: we use an undocumented part of
	 * TEXTSW_FILE_CONTENTS -- if the file name is null, then
	 * it initializes a new view to the contents of the old view.
	 * All this is done to reset the "TEXTSW_MODIFIED" flag that got
	 * set by mt_print_textsw() as part of initializing the window.
	 */
	xv_set(vwl->vwl_textsw,
		TEXTSW_INSERTION_POINT, 0,
		TEXTSW_FIRST,	0,
		TEXTSW_FILE_CONTENTS, 0,
		NULL);

	MP(("mt_load_view %x: clearing modified\n", vwl));
	vwl->vwl_modified = FALSE;

	scrollbar_paint((Scrollbar)xv_get(vwl->vwl_textsw, TEXTSW_SCROLLBAR));

	xv_set(vwl->vwl_frame, XV_SHOW, TRUE, WIN_FRONT, 0);

        /* Not all messages have folder associated; an attachment which is
         * a message does not belong to any folder.
         */

	/*
	 * If we are reading a U or N message then we have modified the mbox
	 */
	if ((!m->mo_read || m->mo_new) && CURRENT_FOLDER(hd)) {
		CURRENT_FOLDER(hd)->fo_changed = 1;
	}

	m->mo_read = 1;
	m->mo_new = 0;
	m->mo_current = 1;
	m->mo_selected = 1;
	force_repaint_on_line(m->m_lineno, hd);
	return(m);
}

#ifdef PEM

struct msg *
mt_load_decrypted_view(vwl, m, abbrev_headers)

	struct view_window_list	*vwl;
	struct msg		*m;
	int			abbrev_headers;
{
	char	header_msg[80];
	struct stat	statb;
	struct header_data *hd;
	int lower_context;
	int		i;
	Frame		popup;
	struct attach	*at;
	Scrollbar	scrollbar;
	FILE		*fp, *fPEM;
	int 		options = 0;
	int 		algs = 0;
	int 		len, fddecrypt, fdPEM;
	char 		buf[80];
	char 		*msgbuf, *newbuf;
	int 		c = 0, ecode = 0, error = 0;
	char		*pemfile, *pemdecrypt;

	fp = NULL;
	/*
	 * Load a message into a view popup and set the message status
	 * and glyph
	 */
	vwl->vwl_garbage = FALSE;
	vwl->vwl_msg = m;

	vwl->vwl_al->al_msg = m;

	/* STRING_EXTRACTION -
	 *
	 * "View Message %d" is the title line for the view window.  %d
	 * is the message number.
	 */
	sprintf(header_msg, gettext("View Message %d"), m->mo_msg_number);
	mt_label_frame(vwl->vwl_frame, header_msg);

	/* Usually redunant, sometimes necessary */
	textsw_reset(vwl->vwl_textsw, 0, 0);

	/* Make sure scrollbar is positioned at top of textsw */
	scrollbar = (Scrollbar)xv_get(vwl->vwl_textsw, WIN_VERTICAL_SCROLLBAR);
	if ((int)xv_get(scrollbar, SCROLLBAR_VIEW_START) != 0) {
		(void)xv_set(scrollbar, SCROLLBAR_VIEW_START, 0, 0);
	}

	/* change the lower context so the screen doesn't scroll */
	lower_context = xv_get(vwl->vwl_textsw, TEXTSW_LOWER_CONTEXT);
	xv_set(vwl->vwl_textsw, TEXTSW_LOWER_CONTEXT, -1, 0);


	/* Cycle through and unshow all previous attachment popups, 
		if any, for attachments */
	i = 1;
	while ((popup = xv_get(vwl->vwl_frame, FRAME_NTH_SUBFRAME, i)) != NULL)
	{
		i++;
		(void)xv_set(popup, WIN_SHOW, FALSE, 0);
	}

	hd = mt_get_header_data(vwl->vwl_headerframe);
	/*
	 * If the message has attachments then display them
	 */
	if ((at = attach_methods.at_first(m)) != NULL) {
		mt_show_attach_list(vwl->vwl_al, TRUE);
		mt_print_textsw_headers(hd, m, vwl->vwl_textsw, abbrev_headers);
		mt_text_insert(vwl->vwl_textsw, "\n", 1);
		mt_load_attachments(vwl->vwl_textsw, vwl->vwl_al, at, FALSE);
	} else {
		mt_show_attach_list(vwl->vwl_al, FALSE);
		/* To decrypt the message, we need to write it
		 * out to a file, decrypt it, and read it back in
		 * to the message structure again.
		 */

		/* create temporary file for PEM message */
		pemfile = (char *) ck_strdup("/tmp/PEMfileXXXXXX");
		pemdecrypt = (char *) ck_strdup("/tmp/PEMdecryptedXXXXXX");
#ifdef SVR4
		mktemp(pemfile);
		fdPEM = open(pemfile, O_CREAT|O_RDWR|O_TRUNC, 0600);
		mktemp(pemdecrypt);
		fddecrypt = open(pemdecrypt, O_CREAT|O_RDWR|O_TRUNC, 0600);
#else
		fdPEM = mkstemp(pemfile);
		fddecrypt = mkstemp(pemdecrypt);
#endif SVR4
		(void) mt_copy_msg(hd, (void *) m, pemfile, 0);
		(void) unlink(pemfile);
		if ((fPEM = fdopen(fdPEM, "r")) == NULL) {
			sprintf(buf, "%s: can't read %s", "mailtool", pemfile);
			perror(buf);
			ecode = 1;
		}
		if (fddecrypt < 0) {
			ecode = 1;
			error = errno;
		} else {
			fp = fdopen (fddecrypt, "w");
			if (fp == NULL) {
				ecode = 1;
				error = errno;
				(void) unlink(pemdecrypt);
				close (fddecrypt);
			} else {
				c = rcc(fPEM, fp, &options, &algs);
				if (c) {
					fprintf(stderr, "%s: rcc failed\n",
								"mailtool");
					ecode = 1;
					(void) unlink(pemdecrypt);
				}
					
				fclose(fPEM);
				fclose(fp);
				close(fdPEM);
				close(fddecrypt);
			}

		}
		if (ecode != 1) {
			msgbuf = (char *)ck_mmap(pemdecrypt, &len);
			newbuf = (char *)ck_malloc(len);
			memset(newbuf, 0, len);
			memcpy(newbuf, msgbuf, len);
			(void) msg_methods.mm_replace(m, newbuf, len);
			(void) unlink(pemdecrypt);

			m->mo_new = 0;
			m->mo_read = 1;
			ck_free(m->m_header);
			m->m_header = makeheader(m);
		}
		mt_print_textsw(hd, m, vwl->vwl_textsw, abbrev_headers);
	}

	xv_set(vwl->vwl_textsw, TEXTSW_LOWER_CONTEXT, lower_context, 0);

	/*
	 * HACK ALERT:katin: we use an undocumented part of
	 * TEXTSW_FILE_CONTENTS -- if the file name is null, then
	 * it initializes a new view to the contents of the old view.
	 * All this is done to reset the "TEXTSW_MODIFIED" flag that got
	 * set by mt_print_textsw() as part of initializing the window.
	 */
	xv_set(vwl->vwl_textsw,
		TEXTSW_INSERTION_POINT, 0,
		TEXTSW_FIRST,	0,
		TEXTSW_FILE_CONTENTS, 0,
		NULL);

	MP(("mt_load_view %x: clearing modified\n", vwl));
	vwl->vwl_modified = FALSE;

	scrollbar_paint((Scrollbar)xv_get(vwl->vwl_textsw, TEXTSW_SCROLLBAR));

	xv_set(vwl->vwl_frame, XV_SHOW, TRUE, WIN_FRONT, 0);


	/*
	 * If we are reading a U or N message then we have modified the mbox
	 */
	if ((!m->mo_read || m->mo_new) && CURRENT_FOLDER(hd)) {
		CURRENT_FOLDER(hd)->fo_changed = 1;
	}

	m->mo_read = 1;
	m->mo_new = 0;
	m->mo_current = 1;
	m->mo_selected = 1;
	force_repaint_on_line(m->m_lineno, hd);
	return(m);
}

#endif PEM

struct view_window_list *
mt_create_new_view(frame, m)

	Frame	frame;
	struct msg *m;

{
	int		n, view_dismiss_proc();
	Attach_list	*list;
	Notify_value	view_event_proc();
	struct header_data *hd;
	struct view_window_list	*vwl;
	Panel		cmdpanel;

	/*
	 * Make sure that we don't have too many view windows.
	 */
	hd = mt_get_header_data(frame);
	for (n = 0, vwl = hd->hd_vwl; vwl != NULL; vwl = vwl->vwl_next)
		n++;
	if (n >= MT_MAX_VIEWS)
		return(NULL);

	/*
	 * Create a view popup and it's associated data struct
	 */
	vwl = (struct view_window_list *)
			malloc(sizeof(struct view_window_list));

	if (vwl == NULL) {
		return(vwl);
	}

	vwl->vwl_modified = 0;
	vwl->vwl_only_attach_modified = 0;

	vwl->vwl_next = NULL;
	vwl->vwl_headerframe = frame;

	vwl->vwl_frame = xv_create(frame, FRAME_CMD,
		FRAME_CMD_PUSHPIN_IN,	FALSE,
		FRAME_SHOW_RESIZE_CORNER,	TRUE,
		FRAME_SHOW_LABEL,	TRUE,
		XV_SHOW,		FALSE,
		XV_WIDTH,		(int)xv_get(frame, XV_WIDTH),
		FRAME_DONE_PROC,	view_dismiss_proc,
		WIN_CLIENT_DATA,	vwl,
		/* used by check_if_from_view_window(),
		 * redundant with above, but
		 * better to have a specific key so we know
		 * when the attach. popups comes from view window 
		 * and when it comes from compose window
		 */
		XV_KEY_DATA,	VWL_KEY,	vwl,
		WIN_ROWS,		mt_popuplines + 1,
		0);

	if (vwl->vwl_frame == NULL)
		goto ERROR_EXIT;

	/*
	 * Create the textsw.  We set the WIN_ROW twice to hack around a bug.
	 * If we don't set it to the first value, then some textsws will not
	 * fill up their frame when the user brings up multiple views at once.
	 * This may be an XView problem, but I can't reproduce in a smaller
	 * program.
	 * Also, for second setting, do xv_set instead of in xv_create
	 * so that it is based on the fixed-width font 
	 * being used in the textsw.
	 * Another bug in xview, setting it in frame requires adding one to it.
	 */

	/*
	 * We don't need the command panel, but XView expects it to
	 * be there, so we minimize it and set XV_SHOW to FALSE.
	 */
	cmdpanel = xv_get(vwl->vwl_frame, FRAME_CMD_PANEL);
	window_fit(cmdpanel);

	xv_set(cmdpanel, XV_SHOW, FALSE, 0);

	vwl->vwl_textsw = xv_create(vwl->vwl_frame, TEXTSW,
		WIN_IS_CLIENT_PANE,
		XV_X,			0,
		XV_Y,			0,
		TEXTSW_FIRST, 		0,
		TEXTSW_INSERTION_POINT,	0,
		TEXTSW_MEMORY_MAXIMUM, 	mt_memory_maximum,
		TEXTSW_DISABLE_LOAD, TRUE, /* Disable "Load" as allowing it
					      is asking for trouble */
		XV_WIDTH,		WIN_EXTEND_TO_EDGE,
		WIN_CLIENT_DATA,	vwl,
		XV_LEFT_MARGIN,		2,
		XV_RIGHT_MARGIN,	0,
		0);

	if (vwl->vwl_textsw == NULL) {
		window_destroy(vwl->vwl_frame);
		goto ERROR_EXIT;
	}

	xv_set(vwl->vwl_textsw,
		WIN_ROWS,	mt_popuplines,
		0);
 
	MP(("textsw borders: left %d, right %d\n",
		xv_get(vwl->vwl_textsw, XV_LEFT_MARGIN),
		xv_get(vwl->vwl_textsw, XV_RIGHT_MARGIN)));

	window_fit_height(vwl->vwl_frame);

	vwl->vwl_msg = m;

	vwl->vwl_al = mt_create_attach_list(vwl->vwl_frame,
		vwl->vwl_headerframe, vwl->vwl_msg, vwl->vwl_headerframe);

	DP(("mt_create_new_view: vwl %x, client_data %x, textsw %x, frame %x\n",
		vwl, xv_get(vwl->vwl_textsw, WIN_CLIENT_DATA),
		vwl->vwl_textsw, vwl->vwl_frame));

	cached_mt_view_window_notify = (void (*) ())xv_get(
			vwl->vwl_textsw, TEXTSW_NOTIFY_PROC);

	(void) xv_set(vwl->vwl_textsw, TEXTSW_NOTIFY_PROC,
		mt_view_window_notify_proc, 0);

	notify_interpose_destroy_func(vwl->vwl_frame, mt_destroy_view_proc);

	/* Catch fram resize */
	notify_interpose_event_func(vwl->vwl_frame, view_event_proc,
		NOTIFY_SAFE);

	return(vwl);

ERROR_EXIT:
	free(vwl);
	return(NULL);
}



view_dismiss_proc(frame)

Frame			frame;

{
	struct	view_window_list	*vwl;
	struct	msg			*m;
	struct	header_data		*hd;
	int				i;
	Frame				popup;

	/* This event caused the frame to disappear 
	   We remove the arrow that indicates that 
	   it is visible */

	/* scan thru the list to find the data for this frame. */
	
	vwl = (struct view_window_list *) xv_get(frame, WIN_CLIENT_DATA);
	hd = mt_get_header_data(vwl->vwl_headerframe);

	while (vwl)
	{
		if (vwl->vwl_frame == frame)
		{
			m = vwl->vwl_msg;

			if (m != NULL && !m->mo_deleted)
			{
				if (m->mo_selected || m->mo_current)
				{
					m->mo_current = 0;
					force_repaint_on_line(m->m_lineno, hd);
				}
			}
			mt_save_msg(vwl);
			break;
		}

		vwl = vwl->vwl_next;
	}

	/* if this results in no glyphs selected, and no current 
	   pointer, deactivate the right things. */

	if (!mt_any_selected(hd))
	{
		for (m = msg_methods.mm_first(CURRENT_FOLDER(hd)); m != NULL;
		     m = msg_methods.mm_next(m))
		{
			if (m->mo_current) {
				break;
			}
		}

		if (m == NULL)
		{
			mt_deactivate_functions();
		}
	}

	/* 
	 * If it is the only window and caching a view window is turned on
	 * just hide it, else destroy it.  Note caching is turned off when
	 * viewing an attachment which is a message.
	 */
	if ((hd->hd_vwl->vwl_next == NULL) && hd->hd_cache_vw) {

		DP(("%s: Hide last view window vwl=%x\n", __FILE__, vwl));

		mt_clear_view_window(vwl);
		xv_set(frame, XV_SHOW, FALSE, 0);
		/* Cycle through and unshow all popups for attachments */
		i = 1;
		while ((popup = xv_get(frame, FRAME_NTH_SUBFRAME, i++)) != NULL)
		{
			/* See if it's a popup */
			if (!(int)xv_get(popup, XV_KEY_DATA, IS_POPUP_KEY))
				continue;
			/* If already unmapped, no need to touch it */
			if (!(int)xv_get(popup, XV_SHOW))
				continue;
			if ((int)xv_get(popup, FRAME_CMD_PUSHPIN_IN))
				xv_set(popup, FRAME_CMD_PUSHPIN_IN, FALSE,NULL);
			(void)xv_set(popup, WIN_SHOW, FALSE, 0);
		}
	} else {
		/*
		 * The frame's interposed destroy proc will clean up
		 */
		DP(("%s: Destroy view window=%x\n", __FILE__, vwl->vwl_frame));
		xv_destroy_safe(vwl->vwl_frame);
	}

}

static Notify_value
view_event_proc(client, event, arg, when)
	Notify_client	client;
	Event		*event;
	Notify_arg	arg;
	Notify_event_type when;
{
	struct	view_window_list *view_p;
	Notify_value	return_value;
	Textsw	textsw;
	void	resize_view_window();

	/*
	 * Get the textsw's CLIENT_DATA,
	 * Textsw is now SUBWINDOW 2 since we didn't destroy cmd_frame's
	 * panel during textsw creation time.
	 */

	if (event_action(event) == WIN_RESIZE) {
		textsw = (Textsw)xv_get(client, FRAME_NTH_SUBWINDOW, 2);
		view_p = (struct view_window_list *)
			xv_get(textsw, WIN_CLIENT_DATA);

		if (view_p != NULL) {
			resize_view_window(view_p);
		}
	}

	return_value = notify_next_event_func(client, (Notify_event) event, 
						arg, NOTIFY_SAFE);
	return(return_value);
}

static void
resize_view_window(view_p)

	struct view_window_list *view_p;

{
	int		frame_h;
	int		height;


	frame_h = (int)xv_get(view_p->vwl_frame, XV_HEIGHT);

	mt_set_attach_canvas_height(view_p->vwl_al);
	height = mt_attach_height(view_p->vwl_al);

	if ((height = frame_h - height) < 1)
		height = 1;

	(void)xv_set(view_p->vwl_textsw,
		XV_HEIGHT, height,
		0);

	mt_layout_attach_panes(view_p->vwl_al, view_p->vwl_textsw);
}

/* 
 * Clear a view window.  This does everything to make a view window ready
 * to be re-used except reset the textsw!  This is done right before a
 * message is loaded into the window to avoid unecessary flashing.
 */
mt_clear_view_window(vwl)

	struct view_window_list *vwl;
{
	struct view_window_list *tmp_vwl;

	if (vwl->vwl_msg == NULL)
		return;	/* Already cleared */

	/* Empty the attachment list */
	mt_clear_attachments(vwl->vwl_al);

#ifdef OLD_AUDIO
	if (mt_audio_panel_owner == vwl->vwl_frame) {
		AudPanel_Unshow(mt_audio_panel_handle);
		mt_audio_panel_owner = NULL;
	}
#endif
	(void)xv_set(vwl->vwl_frame, FRAME_CMD_PUSHPIN_IN, FALSE, 0);
	vwl->vwl_msg->mo_current = 0;
	vwl->vwl_al->al_msg = NULL;

	/* If the current view window (parent window) contains recursive
	 * messages, all view windows (child windows) for recursive messages
	 * will be destroyed.
	 */
	if (vwl->vwl_al->al_hd) {

		DP(("%s: Current view has recursive vwl=%x, free al_hd=%x\n",
		    __FILE__, vwl->vwl_al->al_hd->hd_vwl, vwl->vwl_al->al_hd));

		tmp_vwl = vwl->vwl_al->al_hd->hd_vwl;
		while (tmp_vwl != NULL) {

			DP(("%s: Destroy frame=%x\n", __FILE__,
				tmp_vwl->vwl_frame));

			xv_destroy(tmp_vwl->vwl_frame);
			tmp_vwl = tmp_vwl->vwl_next;
		}

		ck_free(vwl->vwl_al->al_hd);
		vwl->vwl_al->al_hd = NULL;
	}

	vwl->vwl_msg = NULL;
}
