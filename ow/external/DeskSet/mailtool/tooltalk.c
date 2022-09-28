#ifndef lint
static 	char sccsid[] = "@(#)tooltalk.c 3.15 96/12/02 Copyr 1990 Sun Micro";
#endif

/*
 * Copyright (c) 1990 Sun Microsystems, Inc.
 *
 * Tool Talk interface routines for mailtool
 *
 */

#include <stdio.h>
#include <string.h>

#include <X11/X.h>
#include <xview/xview.h>
#include <xview/panel.h>
#include <xview/textsw.h>
#include <xview/font.h>
#include <xview/sel_pkg.h>
#include <desktop/tt_c.h>

#include "glob.h"
#include "tool.h"
#include "tool_support.h"
#include "header.h"
#include "mail.h"
#include "attach.h"

#include "ds_tooltalk.h"
#include "tooltalk.h"

#define DEBUG_FLAG mt_debugging
extern DEBUG_FLAG;
#include "debug.h"

extern void mt_text_insert();

/*
 * Return codes from message handling routines.
 */
typedef enum {
	HS_REJECT,	/* Reject the message */
	HS_REPLY,	/* All is OK. Reply to message */
	HS_FAIL,	/* Fail the message */
	HS_NO_REPLY,	/* Don't reply to the message */
} Handler_status;

typedef	struct tt_ops {
	struct tt_ops	*tt_next;
	char		*tt_op;
	Handler_status	(*tt_func)();
} Op_tt;


/*
 * Routines to handle messages
 */
#ifdef NEVER
static Handler_status handle_tt_launch(), handle_tt_dispatch_data();
static Handler_status handle_tt_move(), handle_tt_quit(), handle_tt_hide();
static Handler_status handle_tt_expose(), handle_tt_retrieve_data();
static Handler_status handle_tt_status();
static Handler_status handle_tt_compose();
static Handler_status handle_tt_compose2();
#endif
static Handler_status handle_tt_tlock();
static Handler_status handle_tt_close();

static Notify_value	receive_tt_message();

int	mt_tt_running = FALSE;	/* TRUE if tool talk is running */
static int	init_tt_retries = 1;	/* Number of times to attempt tooltalk 
					   initialization */
static Frame	application_frame;	/* Base frame of application */
static Op_tt	*mt_ttops;

static char	*new_locale = NULL;
static int	new_depth = -1;
static int	new_visual = -1;

/*
 * Mailtool has 2 ways to start up tool talk:
 *
 * If mailtool was started with the -tooltalk flag then it should come
 * up as a "service" and register with tooltalk at start up.  This
 * is handled by the 2 routines mt_start_tt_init() and mt_complete_tt_init().
 *
 * If mailtool is not a service (ie no -tooltalk) then we delay initializing
 * tooltalk until we have to. Typically this means initializing tooltalk
 * the first time somebody double clicks on an attachment.  This 
 * is handeld by mt_tt_init().
 */

/*
 * Connect with tooltalk.  This routine is used when mailtool is *not*
 * a service.  I.e. it was *not* started with -tooltalk.  In this case we
 * delay initializing tooltalk until we need it.  This routine may be
 * called multiple times in a row -- if tooltalk is already up then
 * it is a no-op.
 */
mt_tt_init(app_string, frame)

	char	*app_string;
	Frame	 frame;

{
	DP(("mt_tt_init: "));
	if (mt_tt_running) {
		DP(("tooltalk already running\n"));
		;
	} else {
		DP(("inititalizing tooltalk. Retries = %d\n", init_tt_retries));
		/* We don't need to pass in arguments since we are not
		 * a service and DISPLAY has already been set in out 
		 * environment.
		 * We limit the number of times we try to start up tooltalk
		 * since we don't want to keep doing it if it is busted.
		 */
		if (init_tt_retries > 0) {
			init_tt_retries--;
			/* mt_start_tt_init() will set mt_tt_running */
			mt_start_tt_init(app_string, FALSE, 0, NULL);
			mt_complete_tt_init(frame);
		}
	}

	return(mt_tt_running);
}

/*
 * Connect with tooltalk.  This should be called before xv_init().
 */
mt_start_tt_init(app_string, started_by_tt, argc, argv)

	char	*app_string;	/* argv[0] of application */
	int	started_by_tt;  /* TRUE if "-tt" was specified as an option */
	int	argc;
	char	**argv;

{
	int rcode;

	/* Initialize tooltalk */
	if ((rcode = ds_tooltalk_init(app_string, argc, argv)) == 0)
		mt_tt_running = TRUE;
	else {
		print_tt_emsg(gettext("Could not initialize Tool Talk"), rcode);
		return(-1);
	}

	/*
	 * If we were started by tool talk, go get the "launch" message so
	 * we can set DISPLAY in our environment.
	 */
	if (started_by_tt) {
		receive_tt_message(NULL, NULL);
		if ( (new_locale != (char *) NULL) &&
		     (new_depth != -1) && (new_visual != -1) )
		   ds_tooltalk_set_argv (argc, argv, new_locale, new_depth,
					     new_visual);
	}

	/*
	 * Initialize the local message handlers.
	 */
#ifdef NEVER
	mt_add_tt_op ("compose", handle_tt_compose);
	mt_add_tt_op ("compose2", handle_tt_compose2);
#endif
	mt_add_tt_op ("tlock", handle_tt_tlock);
	mt_add_tt_op ("rulock", handle_tt_close);

	return(0);
}

mt_add_tt_op (op, func)
char *op;
Handler_status (*func)();
{
	Op_tt *ops;

	ops = (Op_tt *) ck_malloc (sizeof(Op_tt));
	ops->tt_op = op;
	ops->tt_func = func;
	ops->tt_next = mt_ttops;
	mt_ttops = ops;
	return (0);
}

static Handler_status
mt_handle_tt_op (msg)
Tt_message msg;
{
	register char	*op;
	register Op_tt	*ops;

	op = tt_message_op (msg);
	if (op == NULL)
		return (HS_REJECT);

	for (ops = mt_ttops; ops != NULL; ops = ops->tt_next)
	{
		if (strcmp (op, ops->tt_op) == 0)
		{
			if (ops->tt_func != NULL)
				return ((*ops->tt_func) (msg));
			else
				return (HS_REJECT);
		}
	}

	return (HS_REJECT);
}

/*
 * Set the callback routine to handle tooltalk messages.  This should be
 * called before xv_main_loop()
 */
void
mt_complete_tt_init(frame)

	Frame	frame; /* Base frame of application */

{
	if (!mt_tt_running)
		return;

	/* Save application frame so we can use it for footer messages */
	application_frame = frame;

	/* Set the call back routine to handle tooltalk messages */
	if (mt_tt_running) {
		ds_tooltalk_set_callback(frame, receive_tt_message);
	}
	return;
}

/*
 * Terminate tooltalk session.  Should be called after xv_main_loop().
 */
void
mt_quit_tt()

{
	/* Quit tooltalk */
	if (!mt_tt_running)
		return;

	ds_tooltalk_quit();
	mt_tt_running = FALSE;
	return;
}

/*
 * Receive incoming tooltalk messages and dispatch them to the appropriate
 * routines.
 */
static Notify_value
receive_tt_message(client, fd)
	Notify_client client;
	int           fd;
{
	Tt_message	message;
	Tt_message	dstt_tt_message_receive(void);

	/*
	 * Receive the message and dispatch it
	 */
	if ((message = dstt_tt_message_receive()) != 0) {
		dispatch_message(message);
	}

	return NOTIFY_DONE;
}

static
dispatch_message(message)

	Tt_message	message;

{
	Tt_state	state;
	int		rcode;
	Handler_status	hstatus;
	ds_tt_msg_info	msg_info;

#ifdef DEBUG
	if (mt_debugging)
		dump_tt_message("Mailtool: got message", message);
#endif DEBUG

	switch (tt_message_state(message)) {
	
	case TT_SENT: 	/* We've been sent a message */
		break;

	case TT_HANDLED: /* Reply to a message we sent */
	case TT_STARTED: 
	case TT_FAILED:
	case TT_REJECTED: 
		handle_reply(message);
		return;

	case TT_CREATED: 
	case TT_QUEUED: 
	default:	
		goto EXIT;
	}

	/* Get information about the message */
	msg_info = ds_tooltalk_received_message(message);

	/* Dispatch message based on type */
	switch (msg_info.ds_tt_msg_type) {
#ifdef NEVER /* Mailtool is no longer a tooltalk1 service */
	case DS_TT_LAUNCH_MSG:	/* We've been started.  Set DISPLAY */
		hstatus = handle_tt_launch(message);
		break;
	case DS_TT_STATUS_MSG:	/* Generic info we may find interesting */
		hstatus = handle_tt_status(message);
		break;
	case DS_TT_DISPATCH_DATA_MSG: /* Get data off of selection service */
		hstatus = handle_tt_dispatch_data(message);
		break;
	case DS_TT_MOVE_MSG:		/* Move baseframe to a new position */
		hstatus = handle_tt_move(message);
		break;
	case DS_TT_QUIT_MSG:		/* Quit */
		hstatus = handle_tt_quit(message);
		break;
	case DS_TT_HIDE_MSG:		/* Hide ourselves */
		hstatus = handle_tt_hide(message);
		break;
	case DS_TT_EXPOSE_MSG:	/* Expose ourselves */
		hstatus = handle_tt_expose(message);
		break;
	case DS_TT_RETRIEVE_DATA_MSG:/* Send data back via selection service */
		hstatus = handle_tt_retrieve_data(message);
		break;
	case DS_TT_DEPARTING_MSG:	/* An app we started has quit */
		/* Since we don't start any apps, we should never get this */
		hstatus = HS_NO_REPLY;
		break;
#endif
	case DS_TT_NO_STD_MSG:
	default:
		/*
		 * Check for application specific messages here. 
		 */
		hstatus = mt_handle_tt_op(message);
		break;
	}

	/*
	 * Reply to the message based on the return status from the handler
	 */

	/* STRING_EXTRACTION -
	 * 
	 * The following 3 errors occur if we encounter an internal 
	 * Tool Talk error.
	 */
	switch (hstatus) {
	case HS_REPLY:
		/* Reply to message */
		if ((rcode = tt_message_reply(message)) != TT_OK) {
			print_tt_emsg(gettext("Could not reply to message"),
								rcode);
		};
		break;
	case HS_REJECT:
		/* Reject message */
		if ((rcode = tt_message_reject(message)) != TT_OK) {
			print_tt_emsg(gettext("Could not reject message"),
								rcode);
		};
		break;
	case HS_FAIL:
		/* Fail message */
		if ((rcode = tt_message_fail(message)) != TT_OK) {
			print_tt_emsg(gettext("Could not fail message"),
								rcode);
		};
		break;
	case HS_NO_REPLY:
		/* Do nothing */
		break;
	}

EXIT:
	if (message != 0)
		mt_tt_message_destroy(message);

	return;
}

save_message_handler(message, handler_p)

	Tt_message      message;
	char		**handler_p;

{
	int	nbytes;
	char	*new_handler;

	/*
	 * Save the handler of message. We must allocate the space
	 */
	new_handler = tt_message_handler(message);
	nbytes = strlen(new_handler) + 1;

	if (*handler_p == NULL) {
		*handler_p = (char *)malloc(nbytes);
	} else if (nbytes > (int) strlen(*handler_p)) {
		/* Realloc only if it is larger */
		*handler_p = (char *)realloc(*handler_p, nbytes);
	}

	if (*handler_p != NULL)
		strcpy(*handler_p, new_handler);

	return;
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
		*sender_p = (char *)malloc(nbytes);
	} else if (nbytes > (int) strlen(*sender_p)) {
		/* Realloc only if it is larger */
		*sender_p = (char *)realloc(*sender_p, nbytes);
	}

	if (*sender_p != NULL)
		strcpy(*sender_p, new_sender);

	return;
}

/*
 * Search all attachment nodes (ie in all active view and compose windows)
 * for one which matches either the specified message, or the specified
 * viewing application address.
 *
 * So, given a tooltalk reply message we can find the attachment which sent
 * the original request.  Also given a viewing application id, we can
 * find the attachment which was loaded into that application.
 */
Attach_list *
find_attachment(message, handler_id, an_p)

	Tt_message	message; /* Message to match. NULL to ignore */
	char		*handler_id; /* Viewing app to match.  NULL to ignore */
	Attach_node	**an_p;

{
   struct reply_panel_data	*ptr;
   struct header_data	*hd;
   struct view_window_list	*vwl_p;
   Attach_list		*al;
   Attach_node		*an;
   struct attach		*at;
   extern struct header_data *mt_get_header_data();

   /*
    * Loop through all attachment lists looking for a node which matches
    * the specified criteria.  This sounds expensive, but
    * there will usually be just 1 view and 1 compose window.
    */
   
   /*
    * ZZZ: we should not use mt_frame to find the header list which
    * contains the view window list.  We should loop through all header
    * frames.
    */
   for (hd = mt_header_data_list; hd != NULL; hd = hd->hd_next) {

	/* First check view windows */
	for (vwl_p = hd->hd_vwl; vwl_p != NULL; vwl_p =  vwl_p->vwl_next) {

		al = vwl_p->vwl_al;

		for (at = attach_methods.at_first(al->al_msg); at != NULL;
					at = attach_methods.at_next(at)) {

			an = (Attach_node *)
				attach_methods.at_get(at, ATTACH_CLIENT_DATA);
			/*
			 * an will be NULL if it's the first text attachment
			 * which is displayed in the View window
			 */
			if (an == NULL) continue;

			if (an_match(message, handler_id, an)) {
				*an_p = an;
				return(al);
			}

		}
	}

	/* Next check compose windows */
	for (ptr = MT_RPD_LIST(hd); ptr != NULL;
						ptr =  ptr->next_ptr) {
		al = ptr->rpd_al;
		for (at = attach_methods.at_first(al->al_msg); at != NULL;
					at = attach_methods.at_next(at)) {
			an = (Attach_node *)
				attach_methods.at_get(at, ATTACH_CLIENT_DATA);
			/*
			 * an will be NULL if it's the first text attachment
			 * which is displayed in the View window
			 */
			if (an == NULL) continue;
			
			if (an_match(message, handler_id, an)) {
				*an_p = an;
				return(al);
			}
		}
	}
   }
   return(NULL);
}

static
an_match(message, handler_id, an)

	Tt_message	message;
	char		*handler_id;
	Attach_node	*an;
{

	if (handler_id != NULL && an->an_handler_id != NULL &&
		strcmp(an->an_handler_id, handler_id) == 0) {
		return(TRUE);
	}

	if (message != NULL && message == an->an_pending_ttmsg) {
		return(TRUE);
	}

	return(FALSE);
}

static
handle_reply(message)

	Tt_message message;

{
	ds_tt_msg_info	msg_info;
	Attach_node	*an;
	Attach_list	*al;
	int	state;
	
	/*
	 * We've received a reply to a message.  If it's a reply to a 
	 * LAUNCH or DISPATCH_DATA then we do something with it, else
	 * ignore it.
	 */

	/* Get information about the message */
	msg_info = ds_tooltalk_received_message(message);

#ifdef NEVER
	if (msg_info.ds_tt_msg_type != DS_TT_LAUNCH_MSG &&
	    msg_info.ds_tt_msg_type != DS_TT_DISPATCH_DATA_MSG)
		return;
#endif

	/* Find the attachment which sent this message */
	if ((al = find_attachment(message, NULL, &an)) == NULL)
		return;

	/* Get state of message which just came in */
	state = tt_message_state(message);

	switch (msg_info.ds_tt_msg_type) {

	case DS_TT_LAUNCH_MSG:	
		handle_launch_reply(al, an, message, state);
		break;

	case DS_TT_DISPATCH_DATA_MSG:
		handle_load_reply(al, an, message, state);
		break;

	case DS_TT_EXPOSE_MSG:
		handle_expose_reply(al, an, message, state);
		break;

	default:
		mt_tt_message_destroy(message);
		break;
	}

	an->an_ttstate = state;

	return;
}


static 
handle_launch_reply(al, an, message, state)

	Attach_list	*al;
	Attach_node	*an;
	Tt_message message;
	int		state;
{
	Attach_node	*an2;
	void	*handler;

	/* Handle the reply to a launch message */

	switch (state) {

	case TT_FAILED:
	case TT_REJECTED:
		/* We tried to launch an app via tooltalk but it failed */
		an->an_pending_ttmsg = NULL;
		/* Start app via conventional methods */
		mt_invoke_application(al, an, FALSE);
		mt_tt_message_destroy(message);
		return;

	case TT_STARTED:
		/*
		 * We sent a launch and have been notified that tooltalk is
		 * starting up the application.
		 */
		return;
	}

	/* Launch completed successfully, now send the load */

	/* Check if another attachment had its data displayed in
	 * this app.  If we find one, reset it.
	 */
	if (find_attachment(NULL, tt_message_handler(message),
						&an2) != NULL) {
		free(an2->an_handler_id);
		an2->an_handler_id = NULL;
	}

	/* Save the id of the app which is displaying our data */
	if (an->an_handler_id)
		free(an->an_handler_id);

	save_message_handler(message, &(an->an_handler_id));

	/* If the launch caused tooltalk to start a new app send move */
	if (an->an_ttstate == TT_STARTED) {
		/* Looks terrible.  Leave out for now
		mt_send_tt_move(NULL, an->an_handler_id, al->al_frame);
		*/
	}

	/* Send load message */
	handler = ds_tooltalk_get_handler(an->an_open_tt);

	/* STRING_EXTRACTION -
	 * 
	 * Loading data into a launched attachment application
	 */
	if (al->al_errorframe) {
	    (void)xv_set(al->al_errorframe, FRAME_LEFT_FOOTER,
			 gettext("Loading data..."), 0);
	}
	an->an_pending_ttmsg = mt_send_tt_dispatch_data(NULL,
						an->an_handler_id, al, an);

	mt_tt_message_destroy(message);

}

static 
handle_load_reply(al, an, message, state)

	Attach_list	*al;
	Attach_node	*an;
	Tt_message message;
{
	/* Handle the reply to a load message */

	switch (state) {

	case TT_FAILED:
	case TT_REJECTED:
		/* Load operation failed */
		/* STRING_EXTRACTION -
	 	 * 
		 * Could not load data into an attachment viewer
	 	 */
		mt_frame_msg(al->al_errorframe, TRUE,
						gettext("Load failed"));
		break;

	default:
		/* Succesfully completed load */
		if (al->al_errorframe) {
		    (void)xv_set(al->al_errorframe, FRAME_LEFT_FOOTER, "", 0);
		}
		break;
	}


	/* Done with launch and load.  Now send an expose */
	an->an_pending_ttmsg = mt_send_tt_expose(NULL, an->an_handler_id);

	mt_tt_message_destroy(message);
	return;
}

static 
handle_expose_reply(al, an, message, state)

	Attach_list	*al;
	Attach_node	*an;
	Tt_message message;
{
	/* Handle the reply to a load message */
	an->an_pending_ttmsg = NULL;

	switch (state) {

	case TT_FAILED:
	case TT_REJECTED:
		/* Expose operation failed */

		mt_tt_message_destroy(message);

		if (an->an_handler_id && an->an_ttnretries < 1) {
			/* We've probably tried to send a load & expose 
			 * to an app which we thought was already running
			 * Apparently it isn't, so retry one more time
			 * from scratch
			 */
			an->an_ttnretries++;
			free(an->an_handler_id);
			an->an_handler_id = NULL;
			mt_invoke_application(al, an, TRUE);
			return;
		}
		return;

	default:
		break;
	}

	/* Succesfully completed expose */
	an->an_ttnretries = 0;
	mt_tt_message_destroy(message);

	return;
}

/*
 * The handle_tt_ routines handlie incomming tooltalk messages
 */

#ifdef NEVER
static Handler_status
handle_tt_launch(message)

	Tt_message message;

{
	char	*arg;
	char	*value;
	static int already_called;

	/*
	 * Application has just been started via a tool talk message.
	 * First argument is the DISPLAY we want to use.  Note that we
	 * must process this message before xv_init() is called
	 * otherwise there is no affect.
	 */

	/* Only get LOCALE, DEPTH and VISUAL on the first call */
	if (already_called)
		return(HS_REPLY);

	already_called = TRUE;

	/* Get LOCALE */
	arg = (char *)tt_message_arg_val(message, 0);
	if (arg != NULL && *arg != '\0') {
		new_locale = (char *) malloc(strlen(arg) + 1);

		if (new_locale != NULL) {
			strcpy(new_locale, arg);
		}
	}

	/* Get depth */
	tt_message_arg_ival (message, 1, &new_depth);

	/* Get visual */
	tt_message_arg_ival (message, 2, &new_visual);

	return(HS_REPLY);
}

static Handler_status
handle_tt_dispatch_data(message)

	Tt_message message;

{
	char	*sel_name;
	Atom	transient_selection;

	/*
	 * Load data into the textsw.  The message contains the name of
	 * the selection to get the data from.  We may want to check
	 * if the textsw contains unsaved edits and warn the user
	 * before tossing out the old data.
	 */
	sel_name = (char *)tt_message_arg_val(message, 0);
	return(HS_REPLY);
}

static Handler_status
handle_tt_move(message)

	Tt_message message;

{
	Rect    owner_rect;
	int     placement;
	int     i;

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

static Handler_status
handle_tt_quit(message)

	Tt_message message;

{
	(void)xv_destroy_safe(application_frame);
	return(HS_REPLY);
}

static Handler_status
handle_tt_hide(message)

	Tt_message message;

{
	(void)xv_set(application_frame, XV_SHOW, FALSE, 0);
	return(HS_REPLY);
}

static Handler_status
handle_tt_expose(message)

	Tt_message message;

{
	(void)xv_set(application_frame, XV_SHOW, TRUE, 0);
	return(HS_REPLY);
}

static Handler_status
handle_tt_retrieve_data(message)

	Tt_message message;

{
	char *sel_name;

	sel_name = (char *)tt_message_arg_val(message, 0);

	/* Needs to return data via selection service */
	return(HS_REPLY);
}

static Handler_status
handle_tt_status(message)

	Tt_message message;

{
	/* We don't do anything with status info */
	return(HS_REPLY);
}

static Handler_status
handle_tt_compose2(message)

	Tt_message message;

{
	struct reply_panel_data	*ptr, *mt_get_compose_frame();
	char *to, *cc, *subject, *body_sel_name, *attach_sel_name;
	Xv_server	server;
	Atom		transient_selection;
	struct header_data *hd;

	to = (char *)tt_message_arg_val(message, 0);
	subject = (char *)tt_message_arg_val(message, 1);
	cc = (char *)tt_message_arg_val(message, 2);
	body_sel_name = (char *)tt_message_arg_val(message, 3);
	attach_sel_name = (char *)tt_message_arg_val(message, 4);

	/*
	 * Assume mt_frame for now in processing tt requests
	 */
        hd = mt_get_header_data(mt_frame);
	ptr = mt_get_compose_frame(hd);

	if (to != NULL)
		(void)xv_set(ptr->dest_fillin, PANEL_VALUE, to, 0);
	if (cc != NULL)
		(void)xv_set(ptr->cc_fillin, PANEL_VALUE, cc, 0);
	if (subject != NULL)
		(void)xv_set(ptr->subject_fillin, PANEL_VALUE, subject, 0);

	server = XV_SERVER_FROM_WINDOW(ptr->frame);

	if (body_sel_name) {
		/* Stuff data into the compose window */
		transient_selection = (Atom)xv_get(server,
						SERVER_ATOM, body_sel_name);
		mt_received_synthetic_drop(ptr->replysw, ptr->rpd_al,
				transient_selection);
	}

	if (attach_sel_name) {
		mt_show_attach_list(ptr->rpd_al, TRUE);
		mt_layout_compose_window(ptr);
		/* Add attachments */
		transient_selection = (Atom)xv_get(server,
						SERVER_ATOM, attach_sel_name);
		mt_received_synthetic_drop(ptr->rpd_al->al_canvas, ptr->rpd_al,
				transient_selection);
	}

	/* Display compose window */
	mt_display_reply(ptr, NULL);

	return(HS_REPLY);
}

static Handler_status
handle_tt_compose(message)

	Tt_message message;

{
	struct reply_panel_data	*ptr, *mt_get_compose_frame();
	char *to, *cc, *subject, *body;
	struct header_data *hd;

	/*
	 * Assume mt_frame for now in processing tt requests
	 */
        hd = mt_get_header_data(mt_frame);

	to = (char *)tt_message_arg_val(message, 0);
	subject = (char *)tt_message_arg_val(message, 1);
	cc = (char *)tt_message_arg_val(message, 2);
	body = (char *)tt_message_arg_val(message, 3);

	ptr = mt_get_compose_frame(hd);

	if (to != NULL)
		(void)xv_set(ptr->dest_fillin, PANEL_VALUE, to, 0);
	if (cc != NULL)
		(void)xv_set(ptr->cc_fillin, PANEL_VALUE, cc, 0);
	if (subject != NULL)
		(void)xv_set(ptr->subject_fillin, PANEL_VALUE, subject, 0);
	if (body != NULL) {
		mt_text_clear_error(ptr->replysw);
		mt_text_insert(ptr->replysw, body, strlen(body));
	}
	mt_display_reply(ptr, NULL);
	return(HS_REPLY);
}
#endif

/*
 * Tool-Talk handler for lock test
 */
static Handler_status
handle_tt_tlock (message)

	Tt_message message;

{
	return (HS_REPLY);
}

/*
 * Tool-Talk handler for save changes
 */
static Handler_status
handle_tt_close (message)

	Tt_message message;

{
	mt_done_signal_proc();
	return (HS_NO_REPLY);
}


/*
 * Print out a tool talk error message.  Currently this is used
 * mostly for debugging.  We need to come up with a formal way to
 * handle this.
 */

print_tt_emsg(msg, rcode)

	char	*msg;
	int	rcode;

{
	char	*s, *get_tt_emsg();

	s = get_tt_emsg(rcode);

    	fputs( "mailtool: ", stderr );
    	fputs( msg, stderr );
	fputs(": ", stderr);
	fprintf(stderr, s, rcode);
    	fputs( "\n", stderr );

}

char *
get_tt_emsg(rcode)

	int	rcode;

{
	char	*s;

	switch(rcode) {

	case TT_OK:
		s ="TT_OK (%d): Call successful";
		break;
	case TT_ERR_CLASS:
		s = "TT_ERR_CLASS (%d): Tt_class value is invalid";
		break;
	case TT_ERR_DBAVAIL:
		s = "TT_ERR_DBAVAIL (%d): Cannot access database";
		break;
	case TT_ERR_DBEXIST:
		s = "TT_ERR_DBEXIST (%d): Database does not exist";
		break;
	case TT_ERR_FILE:
		s = "TT_ERR_FILE (%d): Cannot create or access file";
		break;
	case TT_ERR_INVALID:
		s = "TT_ERR_INVALID (%d): Invalid link";
		break;
	case TT_ERR_MODE:
		s = "TT_ERR_MODE (%d): Invalid Tt_mode";
		break;
	case TT_ERR_NOMP:
		s = "TT_ERR_NOMP (%d): ttsession is not running";
		break;
	case TT_ERR_NOTHANDLER:
		s = "TT_ERR_NOTHANDLER (%d): Action is allowed for handler only ";
		break;
	case TT_ERR_NUM:
		s = "TT_ERR_NUM (%d): Integer value is wildly out of range";
		break;
	case TT_ERR_OBJID:
		s = "TT_ERR_OBJID (%d): Object ID does not refer to an existing object ";
		break;
	case TT_ERR_OP:
		s = "TT_ERR_OP (%d): Syntactically invalid operation name";
		break;
	case TT_ERR_OTYPE:
		s = "TT_ERR_OTYPE (%d): Undefined objefct type";
		break;
	case TT_ERR_ADDRESS:
		s = "TT_ERR_ADDRESS (%d): Invalid Tt_address";
		break;
	case TT_ERR_PATH:
		s = "TT_ERR_PATH (%d): Invalid path";
		break;
	case TT_ERR_POINTER:
		s = "TT_ERR_POINTER (%d): Invalid pointer";
		break;
	case TT_ERR_PROCID:
		s = "TT_ERR_PROCID (%d): Invalid process id";
		break;
	case TT_ERR_PROPLEN:
		s = "TT_ERR_PROPLEN (%d): Property value too long";
		break;
	case TT_ERR_PROPNAME:
		s = "TT_ERR_PROPNAME (%d): Syntactically invalid property name";
		break;
	case TT_ERR_PTYPE:
		s = "TT_ERR_PTYPE (%d): Undefined process type";
		break;
	case TT_ERR_READONLY:
		s = "TT_ERR_READONLY (%d): Attribute is read-only";
		break;
	case TT_ERR_DISPOSITION:
		s = "TT_ERR_DISPOSITION (%d): Invalid Tt_disposition";
		break;
	case TT_ERR_SCOPE:
		s = "TT_ERR_SCOPE (%d): Invalid Tt_scope";
		break;
	case TT_ERR_SESSION:
		s = "TT_ERR_SESSION (%d): Session ID is not the name of an active session";
		break;
	case TT_ERR_VTYPE:
		s = "TT_ERR_VTYPE (%d): Invalid value type name";
		break;
	case TT_ERR_NO_VALUE:
		s = "TT_ERR_NO_VALUE (%d): No such property value";
		break;
	case TT_ERR_NO_MATCH:
		s = "TT_ERR_NO_MATCH (%d): Could not find handler for message";
		break;
	case TT_WRN_NOTFOUND:
		s = "TT_WRN_NOTFOUND (%d): Object does not exist";
		break;
	case TT_WRN_STOPPED:
		s = "TT_WRN_STOPPED (%d): Query halted by fileter procedure";
		break;
	case TT_WRN_STALE_OBJID:
		s = "TT_WRN_STALE_OBJID (%d): Stale object id";
		break;
	case TT_WRN_SAME_OBJID:
		s = "TT_WRN_SAME_OBJID (%d): Same object id";
		break;
	case TT_ERR_UNIMP:
		s = "TT_ERR_UNIMP (%d):";
		break;
	case TT_ERR_OVERFLOW:
		s = "TT_ERR_OVERFLOW (%d):";
		break;
	case TT_ERR_PTYPE_START:
		s = "TT_ERR_PTYPE_START (%d):";
		break;
	case TT_ERR_APPFIRST:
		s = "TT_ERR_APPFIRST (%d):";
		break;
	default:
		s = "Unknown error code %d";
		break;

	}


	return(s);
}


#ifdef DEBUG
dump_tt_message(string, message)

	char		*string;
	Tt_message      message;

{
	int	status;
	char	*state_string;
	char	*get_tt_emsg();
	char	*op_string;
	char	*handler_string;
	char	buf[256];

	switch (tt_message_state(message)) {
	case TT_SENT:
		state_string = "TT_SENT";
		break;
	case TT_FAILED:
		state_string = "TT_FAILED";
		break;
	case TT_REJECTED:
		state_string = "TT_REJECTED";
		break;
	case TT_CREATED:
		state_string = "TT_CREATED";
		break;
	case TT_HANDLED:
		state_string = "TT_HANDLED";
		break;
	case TT_QUEUED:
		state_string = "TT_QUEUED";
		break;
	case TT_STARTED:
		state_string = "TT_STARTED";
		break;
	default:
		state_string = "Unknown State";
		break;
	}

	if ((status = tt_message_status(message)) == TT_OK)
		*buf = '\0';
	else
		sprintf(buf, get_tt_emsg(status), status);

	op_string = tt_message_op(message);
	handler_string = tt_message_handler(message);
	printf("%-20s: %8d %-15s %s %s %s\n", string, message,
		op_string ? op_string : "<null op string>",
		state_string, buf,
		handler_string ? handler_string : "<null handler string>");

}
#endif DEBUG

mt_tt_message_destroy(message)

	Tt_message      message;

{

#ifdef DEBUG
	if (mt_debugging)
		dump_tt_message("Mailtool destroying", message);
#endif

	return(tt_message_destroy(message));
}
