#ifndef lint
static 	char sccsid[] = "@(#)tooltalk.c 3.11 95/02/08 Copyr 1990 Sun Micro";
#endif

/*
 * Copyright (c) 1990 Sun Microsystems, Inc.
 *
 * Tool Talk interface routines for textedit.
 *
 */

#include <sys/param.h> /* MAXPATHLEN (include types.h if removed) */
#include <ds_verbose_malloc.h>

#ifdef SVR4
#include <dirent.h>   /* MAXNAMLEN */
#include <netdb.h>   /* MAXHOSTNAMLEN */
#else
#include <sys/dir.h>   /* MAXNAMLEN */
#endif

#include <stdio.h>
#include <string.h>

#include <X11/X.h>
#include <xview/xview.h>
#include <xview/panel.h>
#include <xview/textsw.h>
#include <xview/sel_pkg.h>
#include <xview/dragdrop.h>
#include <desktop/tt_c.h>

#include "textedit.h"
#include "ds_tooltalk.h"
#include "ds_popup.h"

extern Textsw			textsw;
extern Selection_requestor	Sel;
extern Xv_Server		My_server;

/*
 * Return codes from message handling routines.
 */
typedef enum {
	HS_REJECT,	/* Reject the message */
	HS_REPLY,	/* All is OK. Reply to message */
	HS_FAIL,	/* Fail the message */
	HS_NO_REPLY,	/* Don't reply to the message */
} Handler_status;

/*
 * Routines to handle messages
 */
Handler_status handle_tt_launch(), handle_tt_dispatch_data();
Handler_status handle_tt_move(), handle_tt_quit(), handle_tt_hide();
Handler_status handle_tt_expose(), handle_tt_retrieve_data();
Handler_status handle_tt_status();

static Notify_value	receive_tt_message();

int	tt_running = FALSE;	/* TRUE if tool talk is running */
static char	*orig_sender;		/* Sender of last message */
static char	*owner_process;		/* Controlling application */

static char   *new_locale = NULL;
static int    new_depth = -1;
static int    new_visual = -1;

Frame	application_frame=0;	/* Base frame of application */

static
modified_data(textsw)

	Textsw	textsw;

{
	/* Returns TRUE if textsw contains modified data */

	if (textsw == NULL || (int)xv_get(textsw, TEXTSW_LENGTH) == 0)
		return(FALSE);
	else
		return(edited);
}


start_tt_init(app_string, fetch_message, argc, argv)

	char	*app_string;
	int	fetch_message;
	int	argc;
	char	**argv;

{
	int rcode;

	/* Initialize tooltalk */
	if ((rcode = ds_tooltalk_init(app_string, argc, argv)) == 0)
		tt_running = TRUE;
	else {
		print_tt_emsg(MGET("Could not initialize Tool Talk"), rcode);
		return(-1);
	}

	/*
	 * Go get the "launch" message
	 */
	if (fetch_message) {
		receive_tt_message(NULL, NULL);
		if ( (new_locale != (char *) NULL) &&
		     (new_depth != -1) && (new_visual != -1) )
		   ds_tooltalk_set_argv (argc, argv, new_locale, new_depth,
					     new_visual);
	}

	return(0);
}

complete_tt_init(frame)

	Frame	frame;

{
	if (!tt_running)
		return;

	/* Save application frame so we can use it for footer messages */
	application_frame = frame;

	/* Set the call back routine to handle tooltalk messages */
	if (tt_running) {
		ds_tooltalk_set_callback(frame, receive_tt_message);
	}
	return;
}

quit_tt()

{
	/* Quit tooltalk */
	if (!tt_running)
		return;

	ds_tooltalk_quit();
	tt_running = FALSE;
	return;
}

/*
 * Receive incoming tooltalk messages and dispatch them to the appropriate
 * routines.
 */

/*ARGSUSED*/
static Notify_value
receive_tt_message( client, fd )
	Notify_client client;
	int           fd;
{
	int		rcode;
	Handler_status	hstatus;
	ds_tt_msg_info	msg_info;
	Tt_message      incoming_msg;

	/*
	 * Receive the message
	 */
	 if ((incoming_msg = tt_message_receive()) == 0)
		goto EXIT;


	switch (tt_message_state(incoming_msg)) {
	
	case TT_SENT: 	/* We've been sent a message */
		break;
	/*
	 * Any other state is from a reply to one of our messages.  Since we
	 * only send out the "I'm departing" message we can ignore all replies.
	 */
	case TT_FAILED:
	case TT_REJECTED: 
	case TT_CREATED: 
	case TT_HANDLED: 
	case TT_QUEUED: 
	case TT_STARTED: 
	default:	
		goto EXIT;
	}

	turnoff_timer();

	/* Get our own copy of the sender of this message */
	save_message_sender(incoming_msg, &orig_sender);

	/* Frame will be NULL when we are explicitly called by init routine */
	if (application_frame != NULL)
		(void)xv_set(application_frame, FRAME_LEFT_FOOTER, "", 0);

	/* Get information about the message */
	msg_info = ds_tooltalk_received_message(incoming_msg);

	/* Dispatch message based on type */
	switch (msg_info.ds_tt_msg_type) {
	case DS_TT_LAUNCH_MSG:	/* We've been started.  Set DISPLAY */
		hstatus = handle_tt_launch(incoming_msg);
		break;
	case DS_TT_STATUS_MSG:	/* Generic info we may find interesting */
		hstatus = handle_tt_status(incoming_msg);
		break;
	case DS_TT_DISPATCH_DATA_MSG: /* Get data off of selection service */
		hstatus = handle_tt_dispatch_data(incoming_msg);
		break;
	case DS_TT_MOVE_MSG:		/* Move baseframe to a new position */
		hstatus = handle_tt_move(incoming_msg);
		break;
	case DS_TT_QUIT_MSG:		/* Quit */
		hstatus = handle_tt_quit(incoming_msg);
		break;
	case DS_TT_HIDE_MSG:		/* Hide ourselves */
		hstatus = handle_tt_hide(incoming_msg);
		break;
	case DS_TT_EXPOSE_MSG:	/* Expose ourselves */
		hstatus = handle_tt_expose(incoming_msg);
		break;
	case DS_TT_RETRIEVE_DATA_MSG:/* Send data back via selection service */
		hstatus = handle_tt_retrieve_data(incoming_msg);
		break;
	case DS_TT_DEPARTING_MSG:	/* An app we started has quit */
		/* Since we don't start any apps, we should never get this */
		hstatus = HS_NO_REPLY;
		break;
	case DS_TT_NO_STD_MSG:
	default:
		/*
		 * Check for application specific messages here.  We have
		 * none, so reject all other messages.
		 */
		hstatus = HS_REJECT;
		break;
	}

	/*
	 * Reply to the message based on the return status from the handler
	 */
	switch (hstatus) {
	case HS_REPLY:
		/* Reply to message */
		if ((rcode = tt_message_reply(incoming_msg)) != TT_OK) {
			print_tt_emsg(MGET("Could not reply to message"),
								rcode);
		};
		break;
	case HS_REJECT:
		/* Reject message */
		if ((rcode = tt_message_reject(incoming_msg)) != TT_OK) {
			print_tt_emsg(MGET("Could not reject message"),
								rcode);
		};
		break;
	case HS_FAIL:
		/* Fail message */
		if ((rcode = tt_message_fail(incoming_msg)) != TT_OK) {
			print_tt_emsg(MGET("Could not fail message"),
								rcode);
		};
		break;
	case HS_NO_REPLY:
		/* Do nothing */
		break;
	}

EXIT:
	if (incoming_msg != 0)
		tt_message_destroy(incoming_msg);

	return NOTIFY_DONE;
}

save_message_sender(message, sender_p)

	Tt_message      message;
	char		**sender_p;

{
	int	nbytes;
	char	*new_sender;

	/*
	 * Save the sender of message. We must allocate the space
	 */
	new_sender = tt_message_sender(message);
	nbytes = strlen(new_sender) + 1;

	if (*sender_p == NULL) {
		*sender_p = (char *)MALLOC(nbytes);
	} else if (nbytes > (int) strlen(*sender_p)) {
		/* Realloc only if it is larger */
		*sender_p = (char *)REALLOC(*sender_p, nbytes);
	}

	if (*sender_p != NULL)
		strcpy(*sender_p, new_sender);

	return;
}

/*
 * Check if message is from the application which "owns" us
 */
from_owner(message)

	Tt_message	message;
{
	char	*sender;

	sender = tt_message_sender(message);

	if (owner_process == NULL) {
		return(TRUE);
	} if (strcmp(owner_process, sender) == 0) {
		return(TRUE);
	} else {
		return(FALSE);
	}
}

static Handler_status
handle_tt_launch(message)

	Tt_message message;

{
	char	*arg;
	char	*value;

	/*
	 * Application has just been started via a tool talk message.
	 */

	if (modified_data(textsw))
		return(HS_REJECT);

	save_message_sender(message, &owner_process);

	/* Get LOCALE */
	arg = (char *)tt_message_arg_val(message, 0);
	if (arg != NULL && *arg != '\0') {
		new_locale = (char *)MALLOC(strlen(arg) + 1);

		if (new_locale != NULL) {
			strcpy(new_locale, arg);
		}

		/* Get depth */
		tt_message_arg_ival (message, 1, &new_depth);

		/* Get visual */
		tt_message_arg_ival (message, 2, &new_visual);
	}

	return(HS_REPLY);
}

static Handler_status
handle_tt_dispatch_data(message)

	Tt_message message;

{
	char	*sel_name;
	Atom	transient_selection;

	if (!from_owner(message))
		return(HS_REJECT);

	sel_name = (char *)tt_message_arg_val(message, 0);
	transient_selection = (Atom)xv_get(My_server, SERVER_ATOM, sel_name);

	if (transient_selection == NULL)
		return(HS_FAIL);

	/*
	 * Let the dragNdrop code handle the data transfer
	 */
	(void)xv_set(Sel, SEL_RANK, transient_selection, 0);
	load_from_dragdrop();

	save_message_sender(message, &owner_process);

	return(HS_REPLY);
}

static Handler_status
handle_tt_move(message)

	Tt_message message;

{
	Rect	owner_rect;
	int	placement;
	int	i;

	if (!from_owner(message))
		return(HS_REJECT);

	i = 0;
	owner_rect.r_left = (int)tt_message_arg_val(message, i++);
	owner_rect.r_top = (int)tt_message_arg_val(message, i++);
	owner_rect.r_width = (int)tt_message_arg_val(message, i++);
	owner_rect.r_height = (int)tt_message_arg_val(message, i++);

	placement = (int)tt_message_arg_val(message, i++);

	/*
	 * ZZZ dipol
	 * We hide the window before the move to get around XView bug 1048933.
	 */
	(void)xv_set(application_frame, XV_SHOW, FALSE, 0);
	ds_position_popup_rect(&owner_rect, application_frame, placement);
	(void)xv_set(application_frame, XV_SHOW, TRUE, 0);

	return(HS_REPLY);
}

static	int	quit_flag = FALSE;

Notify_value
quit_after_sleep()
{
	(void) xv_destroy_safe(application_frame);
	quit_flag = TRUE;
}

timeout_quit(client)
Xv_opaque	client;
{
	struct	itimerval	timeout;
	struct	itimerval	otimeout;

	if(quit_flag || client != application_frame)
	{
		return(NOTIFY_DONE);
	}
	timeout.it_value.tv_sec = 180;
	timeout.it_value.tv_usec = 0;
	timeout.it_interval.tv_sec = 0;
	timeout.it_interval.tv_usec = 0;

	(void) notify_set_itimer_func(application_frame,
			quit_after_sleep,
			ITIMER_REAL,
			&timeout,
			&otimeout);

	xv_set(application_frame, XV_SHOW, FALSE, NULL);
	return(notify_veto_destroy(application_frame));
}

turnoff_timer()
{
	if(application_frame)
	{
		(void) notify_set_itimer_func(application_frame,
			NOTIFY_FUNC_NULL,
			ITIMER_REAL,
			NULL,
			NULL);
	}
	quit_flag = FALSE;	
}

static Handler_status
handle_tt_quit(message)

	Tt_message message;

{
	if (!from_owner(message))
		return(HS_REJECT);

/* 	textsw_reset(textsw, 0, 0); 			*/
	if(xv_get(application_frame, FRAME_CMD_PIN_STATE) == FRAME_CMD_PIN_OUT)
	{
		timeout_quit(application_frame);
	}
	return(HS_REPLY);
}

static Handler_status
handle_tt_hide(message)

	Tt_message message;

{
	if (!from_owner(message))
		return(HS_REJECT);

	(void)xv_set(application_frame, XV_SHOW, FALSE, 0);
	return(HS_REPLY);
}

/*ARGSUSED*/
static Handler_status
handle_tt_expose(message)

	Tt_message message;

{
	if (!from_owner(message))
		return(HS_REJECT);

	(void)xv_set(application_frame, XV_SHOW, TRUE, 0);
	return(HS_REPLY);
}

/*ARGSUSED*/
static Handler_status
handle_tt_retrieve_data(message)

	Tt_message message;

{
	char *sel_name;

	if (!from_owner(message))
		return(HS_REJECT);

	sel_name = (char *)tt_message_arg_val(message, 0);

	/* Needs to return data via selection service */
	return(HS_REPLY);
}

/*ARGSUSED*/
static Handler_status
handle_tt_status(message)

	Tt_message message;

{
	if (!from_owner(message))
		return(HS_REJECT);

	/* We don't do anything with status info */
	return(HS_REPLY);
}

print_tt_emsg(msg, rcode)

	char	*msg;
	int	rcode;

{
	char	*s;

	/*
	 * Print out a tool talk error message.  Currently this is used
	 * mostly for debugging.  We need to come up with a formal way to
	 * handle this.
	 */
	switch(rcode) {

	case TT_OK:
		s = MGET("TT_OK (%d): Call successful");
		break;
	case TT_ERR_CLASS:
		s = MGET("TT_ERR_CLASS (%d): Tt_class value is invalid");
		break;
	case TT_ERR_DBAVAIL:
		s = MGET("TT_ERR_DBAVAIL (%d): Cannot access database");
		break;
	case TT_ERR_DBEXIST:
		s = MGET("TT_ERR_DBEXIST (%d): Database does not exist");
		break;
	case TT_ERR_FILE:
		s = MGET("TT_ERR_FILE (%d): Cannot create or access file");
		break;
	case TT_ERR_INVALID:
		s = MGET("TT_ERR_INVALID (%d): Invalid link");
		break;
	case TT_ERR_MODE:
		s = MGET("TT_ERR_MODE (%d): Invalid Tt_mode");
		break;
	case TT_ERR_NOMP:
		s = MGET("TT_ERR_NOMP (%d): ttsession is not running -- is Tool Talk installed?");
		break;
	case TT_ERR_NOTHANDLER:
		s = MGET("TT_ERR_NOTHANDLER (%d): Action is allowed for handler only ");
		break;
	case TT_ERR_NUM:
		s = MGET("TT_ERR_NUM (%d): Integer value is wildly out of range");
		break;
	/*
	case TT_ERR_OBJID:
		s = MGET("TT_ERR_OBJID (%d): Object ID does not refer to an existing object ");
		break;
	*/
	case TT_ERR_OP:
		s = MGET("TT_ERR_OP (%d): Syntactically invalid operation name");
		break;
	case TT_ERR_OTYPE:
		s = MGET("TT_ERR_OTYPE (%d): Undefined object type");
		break;
	/*
	case TT_ERR_ADDRESS:
		s = MGET("TT_ERR_ADDRESS (%d): Invalid Tt_address");
		break;
	*/
	case TT_ERR_PATH:
		s = MGET("TT_ERR_PATH (%d): Invalid path");
		break;
	case TT_ERR_POINTER:
		s = MGET("TT_ERR_POINTER (%d): Invalid pointer");
		break;
	/*
	case TT_ERR_PROCID:
		s = MGET("TT_ERR_PROCID (%d): Invalid process id");
		break;
	*/
	case TT_ERR_PROPLEN:
		s = MGET("TT_ERR_PROPLEN (%d): Property value too long");
		break;
	case TT_ERR_PROPNAME:
		s = MGET("TT_ERR_PROPNAME (%d): Syntactically invalid property name");
		break;
	case TT_ERR_PTYPE:
		s = MGET("TT_ERR_PTYPE (%d): Undefined process type");
		break;
	case TT_ERR_READONLY:
		s = MGET("TT_ERR_READONLY (%d): Attribute is read-only");
		break;
	/*
	case TT_ERR_DISPOSITION:
		s = MGET("TT_ERR_DISPOSITION (%d): Invalid Tt_disposition");
		break;
	*/
	case TT_ERR_SCOPE:
		s = MGET("TT_ERR_SCOPE (%d): Invalid Tt_scope");
		break;
	case TT_ERR_SESSION:
		s = MGET("TT_ERR_SESSION (%d): Session ID is not the name of an active session");
		break;
	case TT_ERR_VTYPE:
		s = MGET("TT_ERR_VTYPE (%d): Invalid value type name");
		break;
	case TT_ERR_NO_VALUE:
		s = MGET("TT_ERR_NO_VALUE (%d): No such property value");
		break;
	/*
	case TT_ERR_NO_MATCH:
		s = MGET("TT_ERR_NO_MATCH (%d): Could not find handler for message");
		break;
	*/
	case TT_WRN_NOTFOUND:
		s = MGET("TT_WRN_NOTFOUND (%d): Object does not exist");
		break;
	case TT_WRN_STOPPED:
		s = MGET("TT_WRN_STOPPED (%d): Query halted by fileter procedure");
		break;
	/*
	case TT_WRN_STALE_OBJID:
		s = MGET("TT_WRN_STALE_OBJID (%d): Stale object id");
		break;
	case TT_WRN_SAME_OBJID:
		s = MGET("TT_WRN_SAME_OBJID (%d): Same object id");
		break;
	*/
	default:
		s = MGET("Unknown error code %d");
		break;

	}

    	fputs( MGET("textedit: "), stderr );
    	fputs( msg, stderr );
	fputs(": ", stderr);
	fprintf(stderr, s, rcode);
    	fputs( "\n", stderr );
}
