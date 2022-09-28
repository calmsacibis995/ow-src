#ifdef lint
static  char sccsid[] = "@(#)ae_tt.c 1.11 93/01/28 Copyr 1991 Sun Microsystems, Inc.";
#endif

/*
 *  Copyright (c) 1987-1990 Sun Microsystems, Inc.
 *  All Rights Reserved.
 *
 *  Sun considers its source code as an unpublished, proprietary
 *  trade secret, and it is available only under strict license
 *  provisions.  This copyright notice is placed here only to protect
 *  Sun in the event the source is deemed a published work.  Dissassembly,
 *  decompilation, or other means of reducing the object code to human
 *  readable form is prohibited by the license agreement under which
 *  this code is provided to the user or company in possession of this
 *  copy.
 *
 *  RESTRICTED RIGHTS LEGEND: Use, duplication, or disclosure by the
 *  Government is subject to restrictions as set forth in subparagraph
 *  (c)(1)(ii) of the Rights in Technical Data and Computer Software
 *  clause at DFARS 52.227-7013 and in similar clauses in the FAR and
 *  NASA FAR Supplement.
 */ 

/*
 * ToolTalk interface functions for ae.
 */

#include <stdio.h>
#include <sys/param.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <xview/xview.h>
#include <xview/canvas.h>
#include <xview/panel.h>
#include <xview/cms.h>
#include <xview/font.h>
#include <xview/notice.h>
#include <xview/sel_pkg.h>
#include <ds_popup.h>
#include "gettext.h"
#include "ae_ui.h"
#include "ae.h"
#include "ae_tt.h"

#define PROP_FILENAME	".desksetdefaults"
#define PROP_FILENAME2	".cm.rc"
#define AE_MEDIA	"Sun_CM_Appointment"

extern char *cm_strdup(char *);
extern char *get_appt();

char *lastappt;
static Xv_opaque	frame;
static int	exposed = FALSE;
static Tt_message	quit_msg = NULL;

typedef enum
{
	tt_display,
	tt_edit
} type_t;

struct data_deposit
{
	Tt_message	msg;
	type_t		type;
	Data_t		data;
	char		*media;
	char		*buffid;
	char		*msgid;
} putback;

static void
clear_putback()
{
	putback.msg = 0;
	if (putback.media) {
		free(putback.media);
		putback.media = NULL;
	}

	if (putback.buffid) {
		free(putback.buffid);
		putback.buffid = NULL;
	}

	if (putback.msgid) {
		free(putback.msgid);
		putback.msgid = NULL;
	}
}

/*
 * Used by tooltalk.
 */
extern void
get_version(char **vender, char **name, char **version)
{
	if (debug)
		fprintf(stderr, "get_version called\n");

	*vender = "Sunsoft, Inc.";
	*name = "Mini-Appointment Editor";
	*version = "1.0";
}

static void
turnoff_timer()
{
	if (debug)
		fprintf(stderr, "turnoff_timer called\n");

	notify_set_itimer_func(frame,
		NOTIFY_FUNC_NULL, ITIMER_REAL, NULL, NULL);
}

static Notify_value
quit_after_sleep(client, fd)
	Notify_client client; int fd;
{
	if (debug)
		fprintf(stderr, "quit_after_sleep called\n");

	if (frame != NULL) {
		xv_destroy_safe(frame);
		frame = NULL;
	}
	exit(0);
}

/*
 * reset everything and set timer to quit after 3 minutes.
 */
extern void
timeout_quit()
{
	struct	itimerval	timeout;
	struct	itimerval	otimeout;

	/* reset environment */
	dstt_editor_register(AE_MEDIA,
		DISPLAY,	TRUE,
		EDIT,		TRUE,
		NULL);

	xv_set(frame, XV_SHOW, FALSE, FRAME_CMD_PUSHPIN_IN, FALSE, NULL);
	set_default_values(Ae_window);
	clear_putback();

	if (lastappt) {
		free(lastappt);
		lastappt = NULL;
	}

	data_changed = FALSE;
	exposed = FALSE;

	if (quit_msg)
		quit_msg = NULL;

	/* set timer to quit after 3 minutes */
	timeout.it_value.tv_sec = 180;
	timeout.it_value.tv_usec = 0;
	timeout.it_interval.tv_sec = 0;
	timeout.it_interval.tv_usec = 0;

	(void) notify_set_itimer_func(frame,
			quit_after_sleep,
			ITIMER_REAL,
			&timeout,
			&otimeout);
}

/*
 * position ae next to the invoking application.
 */
int
geometry_callback(Tt_message ttmsg, void *key, int status,
	int w, int h, int x, int y)
{
	Rect   rect;

	rect.r_width = w;
	rect.r_height = h;
	rect.r_left = x;
	rect.r_top = y;

	ds_position_popup_rect (&rect, frame, DS_POPUP_LOR);

	xv_set (frame, XV_SHOW, TRUE, NULL);
	exposed = TRUE;

	return(0);
}

/*
 * if key is TRUE, we had just sent out the last deposit
 * and so should quit.
 */
deposit_callback(Tt_message m,
	void *key,
	int status,
	char *media,
	Data_t type,
	void *data,
	int size,
	char *buffid,
	char *msgid)
{
	if (debug)
		fprintf(stderr, "deposit_callback called\n");

	switch (tt_message_state(m)) {
	case TT_HANDLED:
		if (debug)
			fprintf(stderr, "deposit was handled successfully\n");
		break;
	case TT_FAILED:
	case TT_REJECTED:
	default:
		if (debug)
			fprintf(stderr, "deposit failed: state %d\n",
				tt_message_state(m));
	}

	if ((int)key) {
		dstt_message_return(putback.msg, NULL, 0);

		/* reply the QUIT message */
		if (quit_msg)
			dstt_message_return(quit_msg, NULL, 0);

		timeout_quit();
	}
}

/*
 * Send data back to application which invokes us then quit.
 */
static void
send_last_reply(char *appt)
{
	int mark;

	if (putback.type == tt_edit) {
		if (!appt)
			appt = lastappt;
		if (!appt) {
			dstt_message_fail(putback.msg,
				dstt_status_user_request_cancel);
		} else
			dstt_message_return(putback.msg, appt, cm_strlen(appt));

		timeout_quit();

	} else { /* tt_display */
		if (appt) {
			mark = tt_mark();

			/* always use contents */
			dstt_deposit(deposit_callback,
				tt_message_sender(putback.msg),
				TRUE, putback.media, contents,
				appt, cm_strlen(appt), putback.buffid,
				putback.msgid);

			tt_release(mark);
		} else {
			dstt_message_return(putback.msg, NULL, 0);
			timeout_quit();
		}
	}
}

/*
 * Handle SET_ICONIFIED message.
 */
extern status_t
handle_dstt_set_iconified(Tt_message m, dstt_bool_t *iconify, char *msg, char buffid)
{
	xv_set(frame, XV_SHOW, !(*iconify), NULL);
	*iconify = !xv_get(frame, XV_SHOW);
	return(OK);
}

/*
 * Handle GET_ICONIFIED message.
 */
extern status_t
handle_dstt_get_iconified(Tt_message m, dstt_bool_t *iconify, char *msg, char buffid)
{
	*iconify = !xv_get(frame, XV_SHOW);
	return(OK);
}

/*
 * We get a QUIT message.  If it is sent by the same application
 * which invokes us, do the appropriate clean up and quit.
 */
extern status_t
handle_dstt_quit(Tt_message m, int silent, int force, char *msg)
{
	status_t ret_stat = OK;
	char *appt = NULL;
	int failit = FALSE;
	int val;

	if (debug)
		fprintf(stderr, "handle_dstt_quit: silent = %d, force = %d\n",
			silent, force);

	if (putback.msgid == NULL) {
		if (debug)
			fprintf(stderr,
				"handle_dstt_quit: msgid is 0, timeout_quit\n");

		dstt_set_status(m, dstt_status_invalid_message);
		return(FAIL);
	} else if (msg && strcmp(putback.msgid, msg)) { 

		/* only accept quit message from app that invokes us */
		if (debug)
			fprintf(stderr, "handle_dstt_quit: fail quit msg\n");

		dstt_set_status(m, dstt_status_invalid_message);
		return(FAIL);
	}

	/* QUIT message sent by application that invokes us */

	if (force || silent) {
		/* we are forced to quit; fail the original message */
		if (debug)
			fprintf(stderr,
				"handle_dstt_quit: fail msg and quit\n");

		dstt_message_fail(putback.msg, dstt_status_user_request_cancel);
		timeout_quit();
	} else {
		/* reply the original message before we quit */

		if (debug)
			fprintf(stderr,
				"handle_dstt_quit: send appt and quit\n");

		if (data_changed) {
			/* ask user if he wants to save changes */
			val = notice_prompt(frame, NULL,
				NOTICE_MESSAGE_STRINGS,
				MGET("You Have Made Changes That Have"),
				MGET("Not Been APPLIED. Do You Want"),
				MGET("To Save Your Changes?"),
				NULL,
				NOTICE_BUTTON_YES, LGET("Save Changes"),
				NOTICE_BUTTON_NO, LGET("Discard Changes"),
				NULL);

			if (val == NOTICE_YES) {
				/* save changes */
				if ((appt = get_appt()) == NULL) {
					notice_prompt(frame, NULL,
						NOTICE_MESSAGE_STRINGS,
						EGET("Cannot save changes"),
						NULL,
						NOTICE_BUTTON_YES,
						LGET("Continue"),
						NULL);
				}
			}
		}

		/* send last message */
		send_last_reply(appt);

		if (putback.type == tt_display && appt != NULL) {
			quit_msg = m;
			ret_stat = HOLD;
		}
	}

	return(ret_stat);
}

/*
 * We get a DISPLAY message.
 * If we can handle this message, block off all other messages in order
 * to map one-to-one with the data.
 */
extern status_t
handle_dstt_display(Tt_message m,
	char *media,
	Data_t type,
	void *data,
	int size,
	char *msg,
	char *title)
{
	char *status_msg;
	status_t stat = HOLD;
	int mark;

	if (debug) {
		fprintf(stderr, "handle_dstt_display: size = %d\n", size);
		fprintf(stderr, "data = %s\n", (data ? data : "null"));
	}

	/* check media here */
	if (strcmp(media, AE_MEDIA)) {
		if (debug)
			fprintf(stderr, "ae: wrong media %s\n", media);
		stat = REJECT;
	} else {

		switch (type) {
		case contents:
			if (size > 0)
				load_data((char *)data, size, FALSE);
			break;
		case path:
			load_proc(Ae_window, (char *)data, FALSE);
			break;
		default:
			/* only handles types of contents and path */
			dstt_set_status(m, dstt_status_data_not_avail);
			stat = FAIL;
		}
	}

	if (stat == HOLD) {
		mark = tt_mark();

		turnoff_timer();

		/* turn off message handling */
		dstt_editor_register(AE_MEDIA,
			DISPLAY,	FALSE,
			EDIT,		FALSE,
			NULL);

		/* send this message so that the application that invokes
		 * us gets a handle to send more message to us.
		 */
		status_msg = (char *)dstt_set_status(0, dstt_status_req_rec);
		dstt_status(tt_message_sender(m), status_msg, msg,
			setlocale(LC_CTYPE, NULL));

		/* save current context */
		putback.msg = m;
		putback.type = tt_display;
		putback.media = cm_strdup(media);
		putback.buffid = 0;
		putback.msgid = msg ? cm_strdup(msg) : msg;
		putback.data = type;

		if (exposed == FALSE)
			dstt_get_geometry(geometry_callback, NULL,
				tt_message_sender(m), NULL, NULL);

		tt_release(mark);
	} else
		timeout_quit();

	return(stat);
}

/*
 * We get an EDIT message.
 * If we can handle this message, block off all other messages.
 */
extern status_t
handle_dstt_edit(Tt_message m,
	char *media,
	Data_t type,
	void *data,
	int size,
	char *msg,
	char *title)
{
	char *status_msg;
	status_t stat = HOLD;
	int mark;

	if (debug) {
		fprintf(stderr, "handle_dstt_edit: size = %d\n", size);
		fprintf(stderr, "data = %s\n", (data ? data : "null"));
	}

	/* check media here */
	if (strcmp(media, AE_MEDIA)) {
		if (debug)
			fprintf(stderr, "ae: wrong media %s\n", media);
		stat = REJECT;
	} else {

		switch (type) {
		case contents:
			if (size > 0)
				load_data((char *)data, size, FALSE);
			break;
		case path:
			load_proc(Ae_window, (char *)data, FALSE);
			break;
		default:
			/* only handles types of contents and path */
			dstt_set_status(m, dstt_status_data_not_avail);
			stat = FAIL;
		}
	}

	if (stat == HOLD) {
		mark = tt_mark();

		turnoff_timer();

		/* turn off message handling */
		dstt_editor_register(AE_MEDIA,
			DISPLAY,	FALSE,
			EDIT,		FALSE,
			NULL);

		/* send this message so that the application that invokes
		 * us gets a handle to send more message to us.
		 */
		status_msg = (char *)dstt_set_status(0, dstt_status_req_rec);
		dstt_status(tt_message_sender(m), status_msg, msg,
			setlocale(LC_CTYPE, NULL));

		/* save current context */
		clear_putback();
		putback.msg = m;
		putback.type = tt_edit;
		putback.media = cm_strdup(media);
		putback.buffid = 0;
		putback.msgid = msg ? cm_strdup(msg) : msg;
		putback.data = type;

		if (exposed == FALSE)
			dstt_get_geometry(geometry_callback, NULL,
				tt_message_sender(m), NULL, NULL);

		tt_release(mark);
	} else
		timeout_quit();

	return(stat);
}

/*
 * We get a MODIFIED message.
 * If cm's properties are changed, we might need to update ourselves.
 */
extern status_t
handle_dstt_modified(Tt_message m,
	char *type,
	char *filename)
{
	char *ptr;

	if (debug)
		fprintf(stderr, "handle_dstt_modified: type = %s, file = %s\n",
			type, (filename ? filename : "null"));

	/* check media here */
	if (strcmp(type, "File"))
		return(REJECT);

	/* check filename: we want either .desksetdefaults or .cm.rc */
	if (filename) {
		ptr = strrchr(filename, '/');
		if (ptr)
			ptr++;
		else
			ptr = filename;

		if ((strcmp(ptr, PROP_FILENAME) == 0) ||
		    (strcmp(ptr, PROP_FILENAME2) == 0))
			update_props();
	}

	return(OK);
}

/*
 * User hits the "attach" button, we need to send data back to
 * the application that invokes us.
 */
extern void
ae_tt_send_appt(char *appt, int lastmsg)
{
	int	size, mark;

	if (debug)
		fprintf(stderr, "ae_tt_send_appt: appt = %d\n",
			appt ? appt : "");

	if (lastmsg)
		send_last_reply(appt);
	else {
		mark = tt_mark();

		dstt_deposit(deposit_callback, tt_message_sender(putback.msg),
			FALSE, putback.media, contents, appt,
			appt ? cm_strlen(appt) : 0, putback.buffid,
			putback.msgid);

		tt_release(mark);
	}
}

/*
 * User wants to quit ae.
 */
static void
quit_handler(Notify_client frame, Destroy_status status)
{
	int val;
	int pinned;
	char *appt = NULL;

	if (debug)
		fprintf(stderr, "quit_handler called\n");

	if (!putback.msg && !file)
		exit(0);

	/* check if we need to save data first */
	if (data_changed == TRUE) {
		val = notice_prompt(frame, NULL,
			NOTICE_MESSAGE_STRINGS,
			MGET("You Have Made Changes That Have"),
			MGET("Not Been APPLIED. Do You Want"),
			MGET("To Save Your Changes?"),
			NULL,
			NOTICE_BUTTON, LGET("Save Changes"), 1,
			NOTICE_BUTTON, LGET("Discard Changes"), 2,
			NOTICE_BUTTON, LGET("Cancel"), 3,
			NULL);

		switch (val) {
		case 1: /* save changes */
			if ((appt = get_appt()) == NULL)
				return;
			break;
		case 2: /* discard changes */
			if (lastappt) {
				free(lastappt);
				lastappt = NULL;
			}
			break;
		case 3: /* cancel */
			return;
		}
	}

	if (file) {
		if (appt)
			save_appt(appt);
		timeout_quit();
	} else
		send_last_reply(appt);
}

/*
 * Initialize dstt.  Register messages ae can handle and
 * the corresponding message handlers.
 */
extern void
ae_dstt_start(Xv_opaque bframe, int tt_flag)
{
	frame = bframe;

	/* set up for tt */
	dstt_xview_desktop_callback(frame, NULL);

	if (tt_flag) {
		dstt_xview_desktop_callback(frame,
			QUIT,		handle_dstt_quit,
			SET_ICONIFIED,	handle_dstt_set_iconified,
			GET_ICONIFIED,	handle_dstt_get_iconified,
			NULL);

		dstt_xview_desktop_register(frame,
			QUIT,		TRUE,
			SET_ICONIFIED,	TRUE,
			NULL);

		dstt_editor_callback(AE_MEDIA,
			DISPLAY,	handle_dstt_display,
			EDIT,		handle_dstt_edit,
			NULL);

		dstt_editor_register(AE_MEDIA,
			DISPLAY,	TRUE,
			EDIT,		TRUE,
			NULL);
	} else {
		dstt_editor_register(AE_MEDIA,
			DISPLAY,	FALSE,
			EDIT,		FALSE,
			NULL);
	}

	dstt_notice_callback(NULL,
		MODIFIED,	handle_dstt_modified,
		NULL);

	dstt_notice_register(NULL,
		MODIFIED,	TRUE,
		NULL);

	clear_putback();
	dstt_xview_start_notifier();

	/* set up cleanup routine */
	xv_set(Ae_window->window, FRAME_DONE_PROC, quit_handler, NULL);

	lastappt = NULL;
}

