/* Copyright (c) 1993 by Sun Microsystems, Inc. */
#ident	"@(#)atool_tt.c	1.26	93/02/10 SMI"

/*
 * Tool Talk interface routines for audiotool.
 */
#include <stdio.h>
#include <string.h>
#include <sys/param.h>
#include <xview/xview.h>
#include <xview/frame.h>
#include <xview/dragdrop.h>
#include <xview/svrimage.h>
#include <xview/icon_load.h>
#include <desktop/tt_c.h>

#ifndef PROFILED
#include <desktop/ce.h>
#endif

#include "atool_panel_impl.h"	/* to get a bunch of audio stuff */
#include "atool_i18n.h"
#include "atool_debug.h"
#include "atool_sel_impl.h"
#include "ds_tooltalk.h"	/* deskset tooltalk stuff */
#include "atool_ttce.h"

void	atool_tooltalk_set_argv(int, char**, char*, int, int);

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

static Notify_value atool_quit_timer();
static Notify_value	receive_tt_message(Notify_client client, int fd);

static int	tt_running = FALSE;	/* TRUE if tool talk is running */
static char	*orig_sender;		/* Sender of last message */
static char	*owner_process;		/* Controlling application */
static Frame	app_frame;		/* Base frame of application */

static char   *new_locale = NULL;
static int    new_depth = -1;
static int    new_visual = -1;
static int    sleeping = FALSE;		/* TRUE if we got QUIT and sleeping */

static Attr_attribute TT_TIMER_KEY = NULL;

void 
init_tt_keys(Xv_opaque frame)
{
	if (!TT_TIMER_KEY) {
		TT_TIMER_KEY = xv_unique_key();
	}
	/* Save application frame so we can use it for footer messages, etc. */
	if (!app_frame) {
		app_frame = frame;
	}
}

int
start_tt_init(char *app_string, int fetch_msg, int argc, char **argv)
{
	int rcode;

	/* Initialize tooltalk */
	if ((rcode = ds_tooltalk_init(app_string, argc, argv)) == 0)
	    tt_running = TRUE;
	else {
		print_tt_emsg(gettext("Could not initialize Tool Talk"), rcode);
		return (-1);
	}

	DBGOUT((D_DND, "tooltalk initialized\n"));

	/*
	 * If we were started by tool talk, go get the "launch" message so
	 * we can set DISPLAY in our environment.
	 */

	if (fetch_msg) {
		receive_tt_message(NULL, NULL);
		if ( (new_locale != (char *) NULL) &&
		    (new_depth != -1) && (new_visual != -1) )
			atool_tooltalk_set_argv(argc, argv, 
						 new_locale, new_depth,
						 new_visual);
		DBGOUT((D_DND, "started with -tooltalk, receiving first msg\n"));
	}

	return (0);
}

void
complete_tt_init(Xv_opaque frame)
{
	if (!tt_running)
		return;

	init_tt_keys(frame);

	/* Set the call back routine to handle tooltalk messages */
	if (tt_running) {
		DBGOUT((D_DND, "tooltalk: initialization complete\n"));
		ds_tooltalk_set_callback(frame, receive_tt_message);
	}
	return;
}

void
quit_tt(void)
{
	/* Quit tooltalk */
	if (!tt_running)
		return;

	ds_tooltalk_quit();
	tt_running = FALSE;
	DBGOUT((D_DND, "tooltalk: quitting.\n"));
	return;
}

/*
 * Receive incoming tooltalk messages and dispatch them to the appropriate
 * routines.
 */
static Notify_value
receive_tt_message(Notify_client client, int fd)
{
	Tt_state	state;
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

	/* Get our own copy of the sender of this message */
	save_message_sender(incoming_msg, &orig_sender);

#ifdef notdef
	/* Frame will be NULL when we are explicitly called by init routine */
	if (app_frame != NULL)
		(void)xv_set(app_frame, FRAME_LEFT_FOOTER, "", 0);
#endif

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
		hstatus = handle_tt_dispatch_data(client, incoming_msg);
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
	case DS_TT_RETRIEVE_DATA_MSG:	/* Send data back via selection service */
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
			print_tt_emsg(gettext("Could not reply to message"),
								rcode);
		};
		break;
	case HS_REJECT:
		/* Reject message */
		if ((rcode = tt_message_reject(incoming_msg)) != TT_OK) {
			print_tt_emsg(gettext("Could not reject message"),
								rcode);
		};
		break;
	case HS_FAIL:
		/* Fail message */
		if ((rcode = tt_message_fail(incoming_msg)) != TT_OK) {
			print_tt_emsg(gettext("Could not fail message"),
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

	return (NOTIFY_DONE);
}

save_message_sender(Tt_message message, char **sender_p)
{
	unsigned int	nbytes;
	char		*new_sender;

	/*
	 * Save the sender of message. We must allocate the space
	 */
	new_sender = tt_message_sender(message);
	nbytes = strlen(new_sender) + 1;

	if (*sender_p == NULL) {
		*sender_p = (char *)malloc(nbytes);
	} else if (nbytes > strlen(*sender_p)) {
		/* Realloc only if it is larger */
		*sender_p = (char *)realloc(*sender_p, nbytes);
	}

	if (*sender_p != NULL)
		strcpy(*sender_p, new_sender);

	return;
}

/*
 * Check if message is from the application which "owns" us
 */
from_owner(Tt_message message)
{
	char	*sender;

	sender = tt_message_sender(message);

	if (owner_process == NULL) {
		return (TRUE);
	} if (strcmp(owner_process, sender) == 0) {
		return (TRUE);
	} else {
		return (FALSE);
	}
}


static Handler_status
handle_tt_launch(Tt_message message)
{
	char	*arg;
	char	*value;
	static int already_called = 0;
	struct atool_panel_data *ap = NULL;

	if (app_frame)	/* may not be set yet */
		ap = AudPanel_KEYDATA((ptr_t)app_frame);

	/*
	 * Application has just been started via a tool talk message.
	 * First argument is the DISPLAY we want to use.  Note that we
	 * must process this message before xv_init() is called
	 * otherwise there is no affect.
	 */

	if (already_called && !from_owner(message) && !sleeping) {
		DBGOUT((D_DND, "tooltalk: reject launch, not from owner.\n"));
		return (HS_REJECT);
	}

	if (ap && AudPanel_Ismodified(ap)) {
		DBGOUT((D_DND, "tooltalk: reject launch, modified data.\n"));
		return (HS_REJECT);
	}

	if (ap && AudPanel_Iscompose(ap)) {
		DBGOUT((D_DND, "tooltalk: reject launch, in compose.\n"));
		return (HS_REJECT);
	}

	/* turn off timer */
	set_tt_quit_timer(0);

	save_message_sender(message, &owner_process);

	/* Only set DISPLAY and LOCALE on the first call */
	if (already_called)
		return (HS_REPLY);

	already_called = TRUE;

	/* Get LOCALE */
	arg = (char *)tt_message_arg_val(message, 0);
	if (arg != NULL && *arg != '\0') {
		new_locale = (char *)malloc(strlen(arg) + 1);

		if (new_locale != NULL) {
			strcpy(new_locale, arg);
		}

		/* Get depth */
		tt_message_arg_ival (message, 1, &new_depth);

		/* Get visual */
		tt_message_arg_ival (message, 2, &new_visual);

	}

	DBGOUT((D_DND, "tooltalk: received launch message.\n"));
	return (HS_REPLY);
}

static Handler_status
handle_tt_dispatch_data(Xv_opaque frame, Tt_message message)
{
	char	*sel_name;
	AudioSelData *asdp;

	asdp = (AudioSelData*) xv_get(frame, XV_KEY_DATA, AUD_SEL_DATA_KEY);
	if (asdp == NULL) {
		DBGOUT((1, "tt_dispatch: can't get audio sel data\n"));
		return (HS_REJECT);
	}

	/*
	 * Load data into the audiotool.  The message contains the name of
	 * the selection to get the data from. 
	 *
	 * XXX - We may want to check
	 * if audiotool contains unsaved edits and warn the user
	 * before tossing out the old data.
	 */

	if (!from_owner(message)) {
		/*
		 * We have modified data and the message was not from the
		 * application that currently owns us.  Check with user to
		 * see what they want to do
		 */
		DBGOUT((D_DND, "tooltalk: rejected dispatch data, not owner.\n"));
		return (HS_REJECT);
	}

	if (AudPanel_Ismodified(asdp->ap)) {
		DBGOUT((D_DND, "tooltalk: rejected dispatch data, modified data.\n"));
		return (HS_REJECT);
	}

	if (AudPanel_Iscompose(asdp->ap)) {
		DBGOUT((D_DND, "tooltalk: rejected dispatch data, in compose.\n"));
		return (HS_REJECT);
	}

	sel_name = (char *)tt_message_arg_val(message, 0);

	/*
	 * Let the selsvc code handle the data transfer
	 */
	DBGOUT((D_DND, "tooltalk: got dispatch data, requesting load.\n"));
	init_selection_request(asdp, S_TTLoad, sel_name, FALSE);

	save_message_sender(message, &owner_process);

	return (HS_REPLY);
}

static Handler_status
handle_tt_move(Tt_message message)
{
	Rect	owner_rect;
	int	placement;
	int	i;

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
	DBGOUT((D_DND, "tooltalk: got move msg, repositioning to %dx%d+%d+%d\n",
		 owner_rect.r_width, owner_rect.r_height,
		 owner_rect.r_left, owner_rect.r_top));
	(void)xv_set(app_frame, XV_SHOW, FALSE, 0);
	ds_position_popup_rect(&owner_rect, app_frame, placement);
	(void)xv_set(app_frame, XV_SHOW, TRUE, 0);

	return (HS_REPLY);
}

static Handler_status
handle_tt_quit(Tt_message message)
{
	struct atool_panel_data *ap;
	AudioSelData *asdp;
	Rect			rect;

	/* first check if from owner. if not, reject */
	if (!from_owner(message)) {
		DBGOUT((D_DND, "tooltalk: quit msg NOT from owner\n"));
		return (HS_REJECT);
	}

	if (do_handle_tt_quit(app_frame) == FALSE) {
		return (HS_REJECT);
	}

	return (HS_REPLY);
}

int
do_handle_tt_quit(
	Xv_opaque		frame)
{
	struct atool_panel_data *ap;
	AudioSelData		*asdp;
	Rect			rect;
	char			msg[BUFSIZ];

	ap = AudPanel_KEYDATA((ptr_t)frame);

	/* 
	 * check if there's modified data. it's not clear what to do.
	 * options are:
	 *	- reject (assumes client app does the "right" thing).
	 *	- reject & break link
	 *	- post notice, if user cancels, reject & break link
	 *	- ask to save, cancel or quit
	 */
	if (AudPanel_Ismodified(ap)) {
		sprintf(msg, MGET(
		    "The calling application \"%s\" has broken the link.  "
		    "You may:\n"
		    "Quit, discarding the edited data, or\n"
		    "Continue, allowing the data to be saved elsewhere."),
		    ap->link_app[0] ? ap->link_app : MGET("(unknown)"));
		if (AudPanel_Choicenotice(ap, msg,
		    MGET("Quit"), MGET("Continue"), NULL) == 2) {
			/* 
			 * Continue: if linked, need to reset the current
			 * file to NULL so Save-back is disabled.
			 * XXX - this is not really the right place to do this.
			 */
			if (ap->link_app[0]) {
				ap->cfile[0] = NULL;
			}			       
			AudPanel_Breaklink(ap);
			DBGOUT((D_DND,
				"tooltalk: rejected quit, modified data.\n"));
			return (FALSE);
		}
	}

	/*
	 * at this point, we're definitely going to clear the data. make
	 * sure file is flagged as unmodified so we don't get further
	 * notification.
	 */
	AudPanel_Unmodify(ap);

	if (!TT_TIMER_KEY) {
		DBGOUT((D_DND, 
			"tooltalk: got quit msg, destroying frame ...\n"));
		AudPanel_Quit(ap);
	} else {
		set_tt_quit_timer(1);
		AudPanel_Hide(ap); /* clear data and dissappear */
		DBGOUT((D_DND,
			"tooltalk: got quit msg, hiding for 30 mins ...\n"));
	}

	return (TRUE);
}

void
set_tt_quit_timer(int flag)
{
	struct itimerval itv;

	sleeping = flag;

	/* sleep for 30 minutes instead of just quitting ... */
	itv.it_value.tv_sec = itv.it_interval.tv_sec = 
		flag ? TT_SLEEP_TIME : 0;
	itv.it_value.tv_usec = itv.it_interval.tv_usec = 0;

	notify_set_itimer_func(TT_TIMER_KEY,
			       (flag ? atool_quit_timer : NOTIFY_FUNC_NULL),
			       ITIMER_REAL, &itv, NULL);
}

static Notify_value
atool_quit_timer()
{
	struct atool_panel_data *ap;

	ap = AudPanel_KEYDATA((ptr_t)app_frame);

	DBGOUT((D_DND, "tooltalk: quit timer expired, exitting ...\n"));
	AudPanel_Quit(ap);

	return (XV_OK);
}

static Handler_status
handle_tt_hide(Tt_message message)
{
	struct atool_panel_data *ap;

	ap = AudPanel_KEYDATA((ptr_t)app_frame);
	DBGOUT((D_DND, "tooltalk: got hide msg, setting XV_SHOW to FALSE\n"));
	AudPanel_Unshow(ap);	/* just dissappear */
	return (HS_REPLY);
}

static Handler_status
handle_tt_expose(Tt_message message)
{
	struct atool_panel_data *ap;

	ap = AudPanel_KEYDATA((ptr_t)app_frame);
	DBGOUT((D_DND, "tooltalk: got expose msg, setting XV_SHOW to TRUE\n"));
	AudPanel_Show(ap);
	return (HS_REPLY);
}

static Handler_status
handle_tt_retrieve_data(Tt_message message)
{
	char *sel_name;

	sel_name = (char *)tt_message_arg_val(message, 0);

	/* Needs to return data via selection service */
	DBGOUT((D_DND, "tooltalk: got retrieve msg, ignoring\n"));
	return (HS_REPLY);
}

static Handler_status
handle_tt_status(Tt_message message)
{
	/* We don't do anything with status info */
	DBGOUT((D_DND, "tooltalk: got status msg, ignoring\n"));
	return (HS_REPLY);
}

print_tt_emsg(char *msg, int rcode)
{
#ifdef DEBUG_PRINT
	char	*s;

	/*
	 * Print out a tool talk error message.  Currently this is used
	 * mostly for debugging.  We need to come up with a formal way to
	 * handle this.
	 */
	switch (rcode) {
	case TT_OK:
		s = "TT_OK (%d): Call successful";
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
		s = "TT_ERR_NOMP (%d): "
		    "ttsession is not running -- is ToolTalk installed?";
		break;
	case TT_ERR_NOTHANDLER:
		s = "TT_ERR_NOTHANDLER (%d): "
		    "Action is allowed for handler only ";
		break;
	case TT_ERR_NUM:
		s = "TT_ERR_NUM (%d): Integer value is wildly out of range";
		break;
	case TT_ERR_OP:
		s = "TT_ERR_OP (%d): Syntactically invalid operation name";
		break;
	case TT_ERR_OTYPE:
		s = "TT_ERR_OTYPE (%d): Undefined objefct type";
		break;
	case TT_ERR_PATH:
		s = "TT_ERR_PATH (%d): Invalid path";
		break;
	case TT_ERR_POINTER:
		s = "TT_ERR_POINTER (%d): Invalid pointer";
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
	case TT_ERR_SCOPE:
		s = "TT_ERR_SCOPE (%d): Invalid Tt_scope";
		break;
	case TT_ERR_SESSION:
		s = "TT_ERR_SESSION (%d): "
		    "Session ID is not the name of an active session";
		break;
	case TT_ERR_VTYPE:
		s = "TT_ERR_VTYPE (%d): Invalid value type name";
		break;
	case TT_ERR_NO_VALUE:
		s = "TT_ERR_NO_VALUE (%d): No such property value";
		break;
	case TT_WRN_NOTFOUND:
		s = "TT_WRN_NOTFOUND (%d): Object does not exist";
		break;
	case TT_WRN_STOPPED:
		s = "TT_WRN_STOPPED (%d): Query halted by filter procedure";
		break;
	default:
		s = "Unknown error code %d";
		break;
#ifdef notdef
	case TT_ERR_OBJID:
		s = "TT_ERR_OBJID (%d): "
		    "Object ID does not refer to an existing object";
		break;
	case TT_ERR_ADDRESS:
		s = "TT_ERR_ADDRESS (%d): Invalid Tt_address";
		break;
	case TT_ERR_PROCID:
		s = "TT_ERR_PROCID (%d): Invalid process id";
		break;
	case TT_ERR_DISPOSITION:
		s = "TT_ERR_DISPOSITION (%d): Invalid Tt_disposition";
		break;
	case TT_ERR_NO_PROPERTY:
		s = "TT_ERR_NO_PROPERTY (%d): No such property";
		break;
	case TT_ERR_NO_MATCH:
		s = "TT_ERR_NO_MATCH (%d): Could not find handler for message";
		break;
	case TT_WRN_STALE_OBJID:
		s = "TT_WRN_STALE_OBJID (%d): Stale object id";
		break;
	case TT_WRN_SAME_OBJID:
		s = "TT_WRN_SAME_OBJID (%d): Same object id";
		break;
#endif
	}
	fprintf(stderr, "audiotool: %s: ", msg);
	fprintf(stderr, s, rcode);
	fprintf(stderr, "\n");
#endif /* DEBUG_PRINT */
}

#define DEFAULT_CE_FILE_TYPE "default-doc"

Server_image
get_ce_icon(
	char		*file_type)
{
#ifndef PROFILED
	char		fullpath[MAXPATHLEN+1];
	CE_NAMESPACE	t_name_space;
	CE_ENTRY	ttype_ent;
	CE_ATTRIBUTE	tns_icon;
	CE_ATTRIBUTE	tns_attr;

	if (!(t_name_space = ce_get_namespace_id("Types"))) {
		DBGOUT((D_DND, "can't get Types namespace\n"));
		return (NULL);
	}
	if (!(tns_attr = ce_get_attribute_id (t_name_space, "TYPE_ICON"))) {
		DBGOUT((D_DND, "can't get TYPE_ICON attr for Types NS\n"));
		return (NULL);
	}

	if (!(ttype_ent = ce_get_entry(t_name_space, 1, file_type))) {
		DBGOUT((D_DND, "can't get type entry for %s, trying %s\n",
			 file_type, DEFAULT_CE_FILE_TYPE));
		if (!(ttype_ent = ce_get_entry(t_name_space, 1, 
					       DEFAULT_CE_FILE_TYPE))) {
			DBGOUT((D_DND, "can't get type entry for %s\n",
				 DEFAULT_CE_FILE_TYPE));
		
			return (NULL);
		}
	}
	if (!(tns_icon = ce_get_attribute(t_name_space, ttype_ent, 
					  tns_attr))) {
		DBGOUT((D_DND, "can't get icon for %s\n", file_type));
		return (NULL);
	}

	/* now create the pixmap and set  */
	ds_expand_pathname(tns_icon, fullpath);
	return (icon_load_svrim(fullpath, "can't load icon file"));

#else /* PROFILED */
	return ((Server_image)NULL);
#endif /* PROFILED */
}
	 
/* our own copy 'cause libdeskset version broken */

void
atool_tooltalk_set_argv(int argc, char **argv, char *locale,
			      int depth, int visual)
{
	int 	 i;
	char	 arg_value [100];

	static char	 *visual_names [] = {
		"StaticGray", "GrayScale", 
		"StaticColor", "PseudoColor", 
		"TrueColor", "DirectColor" };

	/*
	 * Go through argv, and find the various command line args:
	 *  -display, -visual, and -lc_basiclocale.
	 * When they are found, substitute in the various values passed
	 * to this function.
	 */

	for (i = 1; i < argc; i++) {
		if (strcmp(argv[i], "-depth") == 0) {
			/* free (argv[i+1]); */
			sprintf (arg_value, "%d", depth);
			argv [i+1] = (char *) malloc (strlen (arg_value) + 1);
			strcpy (argv [i+1], arg_value);
			i++;
		}
		if (strcmp(argv[i], "-visual") == 0) {
			/* free (argv[i+1]); */
			argv [i+1] = (char *) malloc (strlen (visual_names [visual]) + 1);
			strcpy (argv [i+1], visual_names [visual]);
			i++;
		}
		if (strcmp(argv[i], "-lc_basiclocale") == 0) {
			/* free (argv[i+1]); */
			argv [i+1] = (char *) malloc (strlen (locale) + 1);
			strcpy (argv [i+1], locale);
			i++;
		}
	}

}

