#ifndef lint
static 	char sccsid[] = "@(#)tooltalk_send.c 3.7 94/05/04 Copyr 1990 Sun Micro";
#endif

/*
 * Copyright (c) 1990 Sun Microsystems, Inc.
 *
 * Tool Talk interface routines for mailtool for sending messages
 *
 */

#include <stdio.h>
#include <string.h>

#include <X11/X.h>
#include <xview/xview.h>
#include <xview/panel.h>
#include <xview/textsw.h>
#include <xview/sel_pkg.h>
#include <xview/font.h>
#include <desktop/tt_c.h>

#include "glob.h"
#include "tool.h"
#include "tool_support.h"
#include "attach.h"
#include "ds_tooltalk.h"
#include "tooltalk.h"
#include "ds_popup.h"

#define DEBUG_FLAG mt_debugging
extern int DEBUG_FLAG;
#include "debug.h"

extern int	mt_tt_running;

/* WARNGING! Tooltalk must be initialized before using any of these routines! */

Tt_message
mt_send_tt_launch(handler, frame)

	void	*handler;	/* Application to send message to */
	Frame	frame;		/* Use frame to determine display and locale */


{
	Tt_message	tt_msg;
	char		*value;
	int		rcode;
	int		ivalue;

	DP(("mt_send_tt_launch(handler = %s)\n",
		handler ? handler : "<null>"));

	if (!mt_tt_running)
		return(NULL);

	tt_msg = ds_tooltalk_create_message(handler, DS_TT_LAUNCH_MSG, NULL);

	/* Return NULL to flag that something is wrong */
	if (!tt_msg) return(NULL);

	value = (char *)xv_get(frame, XV_LC_BASIC_LOCALE);
	tt_message_arg_add(tt_msg, TT_IN, "string", value);

	ivalue = (int) xv_get (frame, WIN_DEPTH);
	tt_message_iarg_add(tt_msg, TT_IN, "int", ivalue);

	ivalue = (int) xv_get (frame, XV_VISUAL_CLASS);
	tt_message_iarg_add(tt_msg, TT_IN, "int", ivalue);

	if ((rcode = tt_message_send(tt_msg)) != TT_OK) {
		print_tt_emsg("Could not send message: ", rcode);
	}

#ifdef DEBUG
	if (mt_debugging)
		dump_tt_message("Mailtool sent", tt_msg);
#endif DEBUG

	return(tt_msg);
}

/*
 * The following routines are used to send various tooltalk messages and
 * are usually called after a launch message.  The caller can either
 * send the message to any handler, or to a specific process.  For example:
 *
 *	handler == "textedit", handler_id == NULL would send the message to
 *	some running textedit.
 *
 *	handler == NULL, handler_id == "id string" would send the 
 * 	message to the particular process identified by the handler_id.
 *
 * The handler_id is usually gotten by using the tt_message_handler() call.
 * 
 */
Tt_message
mt_send_tt_dispatch_data(handler, handler_id, al, an)

	void		*handler;
	char		*handler_id;
	Attach_list	*al;
	Attach_node	*an;

{
	Tt_message	tt_msg;
	char		*sel_name;
	Atom		sel_atom;
	Xv_server	server;
	Selection	selection;
	int		mail_sel_convert_proc;
	int		rcode;

	DP(("mt_send_tt_dispatch_data(handler = %s handler_id = %s)\n",
		handler ? handler : "<null>",
		handler_id ? handler_id : "<null>"));

	if (!mt_tt_running)
		return(NULL);

	server = XV_SERVER_FROM_WINDOW(al->al_frame);

	/*
	 * 5 may 91, katin: it's critical to grab the display here,
	 * otherwise we could have two different processes grab
	 * the same atom...
	 */
	XGrabServer(al->al_display);

	sel_atom = conjure_transient_selection(server,
		xv_get(server, XV_DISPLAY), "DESKSET_SELECTION_%d");
	sel_name = (char *)xv_get(server, SERVER_ATOM_NAME, sel_atom);

	DP(("mt_send_tt_dispatch_data: sel name %s/%d\n", 
		sel_name ? sel_name : "<null>", sel_atom));

	selection = xv_create(al->al_frame, SELECTION_OWNER,
		SEL_RANK,	sel_atom,
		0);

	if (!selection) {
		XUngrabServer(al->al_display);
		XFlush (al->al_display);

		/* STRING_EXTRACTION -
		 * 
		 * User has double clicked on an attachment and we hit an
		 * internal selection service error which prevents us from
		 * loading the attachment data into the viewing application
		 */
		mt_frame_msg(al->al_errorframe, TRUE,
		          gettext("Load failed: Could not create selection"));
		return(NULL);
	}

	(void)xv_set(selection,
		SEL_OWN,	TRUE,
		0);

	XUngrabServer(al->al_display);
	XFlush (al->al_display);

	tt_msg = ds_tooltalk_create_message(handler, DS_TT_DISPATCH_DATA_MSG,
									NULL);

	if (handler_id != NULL) {
		tt_message_handler_set(tt_msg, handler_id);
		tt_message_address_set (tt_msg, TT_HANDLER);
	}

	tt_message_arg_add(tt_msg, TT_IN, "string", sel_name);

	/*
	 * Set selection 
	 */
	mt_attach_selection_data(selection, NULL, al, an, al->al_msg, TRUE,
				 NULL, NULL);

	if ((rcode = tt_message_send(tt_msg)) != TT_OK) {
		print_tt_emsg("Could not send message: ", rcode);
	}

#ifdef DEBUG
	if (mt_debugging)
		dump_tt_message("Mailtool sent", tt_msg);
#endif DEBUG

	return(tt_msg);
}

void
mt_send_tt_move(handler, handler_id, frame)

	void	*handler;
	char	*handler_id;
	Frame	frame;

{
	Tt_message	tt_msg;
	int		rcode;
	Rect		frame_rect;

	if (!mt_tt_running)
		return;

	frame_get_rect(frame, &frame_rect);

	tt_msg = ds_tooltalk_create_message(handler, DS_TT_MOVE_MSG, NULL);

	if (handler_id != NULL) {
		tt_message_handler_set(tt_msg, handler_id);
		tt_message_address_set (tt_msg, TT_HANDLER);
	}

	tt_message_iarg_add(tt_msg, TT_IN, "int", (int) frame_rect.r_left);
	tt_message_iarg_add(tt_msg, TT_IN, "int", (int) frame_rect.r_top);
	tt_message_iarg_add(tt_msg, TT_IN, "int", (int) frame_rect.r_width);
	tt_message_iarg_add(tt_msg, TT_IN, "int", (int) frame_rect.r_height);
	tt_message_iarg_add(tt_msg, TT_IN, "int", (int) DS_POPUP_LOR);

	if ((rcode = tt_message_send(tt_msg)) != TT_OK) {
		print_tt_emsg("Could not send message: ", rcode);
	}

#ifdef DEBUG
	if (mt_debugging)
		dump_tt_message("Mailtool sent", tt_msg);
#endif DEBUG

	return;
}

Tt_message
mt_send_tt_expose(handler, handler_id)

	void	*handler;
	char	*handler_id;

{
	Tt_message	tt_msg;
	int		rcode;

	DP(("mt_send_tt_expose(handler = %s handler_id = %s)\n",
		handler ? handler : "<null>",
		handler_id ? handler_id : "<null>"));

	if (!mt_tt_running)
		return(NULL);

	tt_msg = ds_tooltalk_create_message(handler, DS_TT_EXPOSE_MSG, NULL);

	if (handler_id != NULL) {
		tt_message_handler_set(tt_msg, handler_id);
		tt_message_address_set (tt_msg, TT_HANDLER);
	}

	if ((rcode = tt_message_send(tt_msg)) != TT_OK) {
		print_tt_emsg("Could not send message: ", rcode);
	}

#ifdef DEBUG
	if (mt_debugging)
		dump_tt_message("Mailtool sent", tt_msg);
#endif DEBUG

	return (tt_msg);;
}

void
mt_send_tt_hide(handler, handler_id)

	void	*handler;
	char	*handler_id;

{
	Tt_message	tt_msg;
	int		rcode;

	if (!mt_tt_running)
		return;

	tt_msg = ds_tooltalk_create_message(handler, DS_TT_HIDE_MSG, NULL);

	if (handler_id != NULL) {
		tt_message_handler_set(tt_msg, handler_id);
		tt_message_address_set (tt_msg, TT_HANDLER);
	}

	if ((rcode = tt_message_send(tt_msg)) != TT_OK) {
		print_tt_emsg("Could not send message: ", rcode);
	}

#ifdef DEBUG
	if (mt_debugging)
		dump_tt_message("Mailtool sent", tt_msg);
#endif DEBUG

	return;
}

void
mt_send_tt_quit(handler, handler_id)

	void	*handler;
	char	*handler_id;

{
	Tt_message	tt_msg;
	int		rcode;

	if (!mt_tt_running)
		return;

	tt_msg = ds_tooltalk_create_message(handler, DS_TT_QUIT_MSG, NULL);

	if (handler_id != NULL) {
		tt_message_handler_set(tt_msg, handler_id);
		tt_message_address_set (tt_msg, TT_HANDLER);
	}

	if ((rcode = tt_message_send(tt_msg)) != TT_OK) {
		print_tt_emsg("Could not send message: ", rcode);
	}

#ifdef DEBUG
	if (mt_debugging)
		dump_tt_message("Mailtool sent", tt_msg);
#endif DEBUG

	return;
}
