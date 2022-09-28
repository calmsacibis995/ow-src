/* Copyright (c) 1993 by Sun Microsystems, Inc. */
#ident	"@(#)atool_xv.c	1.165	96/02/20 SMI"

#include <stdio.h>
#include <ctype.h>
#include <sys/param.h>
#include <sys/types.h>
#include <math.h>
#include <fcntl.h>
#include <string.h>

#include <xview/xview.h>
#include <xview/panel.h>
#include <xview/textsw.h>
#include <xview/xv_xrect.h>
#include <xview/cms.h>
#include <xview/notice.h>
#include <xview/svrimage.h>
#include <group.h>

#ifndef PROFILED
#include <desktop/ce.h>
#endif

#include <multimedia/libaudio.h>

#include "atool_i18n.h"
#include "atool_panel_impl.h"
#include "atool_debug.h"
#include "atool_ui.h"
#include "atool_sel.h"
#include "atool_ttce.h"
#include "version.h"
#include "holdbutton.h"
#include "meter.h"
#include "ds_colors.h"
#include "loadsave/loadsave_panel.h"
#include "segment/segment_canvas.h"
#include "config/config_panel.h"
#include "format/format_panel.h"

#ifndef PRE_493
#include "dstt_audio.h"
extern int ttstart_play(ptr_t, char*, char*);
extern int ttstart_record(ptr_t, char*, char*);
#endif

#ifdef DEBUG
char	*Version_String = NULL;	/* manual override for version string */
#endif

Attr_attribute	INSTANCE;
Attr_attribute	AUD_TOOL_KEY;	/* global key for local audio panel storage */

Notify_value	sigio_async(Notify_client client,
            	        int sig,
            	        Notify_signal_mode when);
Notify_value	sigio_sync(Notify_client client,
            	        Notify_event event,
            	        Notify_arg arg,
            	        Notify_event_type when);
Notify_value	timer_sync_handler(Notify_client client, int which);
Notify_value	sigint_sync(Notify_client client,
            	        int sig,
            	        Notify_signal_mode when);
Notify_value	sigpipe_sync(Notify_client client,
            	        Notify_event event,
            	        Notify_arg arg,
            	        Notify_event_type when);

Notify_value	atool_canvas_event(Xv_window win,
            	        Event *event,
            	        Notify_arg arg,
            	        Notify_event_type type);
void		atool_usage_proc();
Menu		gen_format_menu(Menu_item, Menu_generate);

#ifdef PRE_493
extern 		ptr_t PlayctlPanel_Init(ptr_t, char*);
extern 		ptr_t ReclevelPanel_Init(ptr_t, char*);
#else
void 		add_menu_accel(Menu menu, char *label, char *accel);
Menu		add_submenu_accel(Menu menu, char *label, char *accel);
#endif

/* 
 * set up signal catchers with notifier 
 */
init_intr(
	Xv_opaque	Base_frame,
	ptr_t		ap)
{
	/* set up asychronous catcher for SIGIO */
	/* XXX - synchronous again, until record stuff is straightened out */
	(void) notify_set_signal_func((Xv_opaque)ap, 
	    (Notify_func)sigio_async, SIGIO, NOTIFY_SYNC);

        /* Set up to a synchronous handler to post events to */
        (void) notify_set_event_func((Xv_opaque)ap,
            (Notify_func)sigio_sync, NOTIFY_SAFE);

	/* set up to catch ^C - so we can clean up ... */
	(void) notify_set_signal_func(Base_frame, 
	    (Notify_func)sigint_sync, SIGINT, NOTIFY_SYNC);
 	    /* this used to be NOTIFY_SAFE and not NOTIFY_SYNC - MCA */

	/* set up to catch SIGPIPE (will be ignored for now) */
	(void) notify_set_signal_func(Base_frame, 
	    (Notify_func)sigpipe_sync, SIGPIPE, NOTIFY_SYNC);
 	    /* this used to be NOTIFY_SAFE and not NOTIFY_SYNC - MCA */
}

static int	SCHEDASYNC = FALSE;

/* Reschedule the SIGIO handler synchronously */
schedule_async_handler(
	ptr_t			ap)
{
#ifdef notdef
	(void) notify_post_event((Xv_opaque)ap, SIGIO, NOTIFY_SAFE);
#endif
	SCHEDASYNC = TRUE;
}

/*
 * Asynchronous SIGIO handler.
 * Calls async_handler() to keep the audio data transfer running
 */
/*ARGSUSED*/
Notify_value
sigio_async(
	Notify_client		client,
	int			sig,
	Notify_signal_mode	when)
{
	int			err;
        struct atool_panel_data	*ap;

        ap = (struct atool_panel_data *) client;

	DBGOUT((9, "async_handler called at async level\n"));
	err = async_handler(ap);

	/* If data transfer was interrupted, schedule the synchronous handler */
	if (SCHEDASYNC = (err != AUDIO_SUCCESS)) {
#ifdef notdef
		DBGOUT((7, "async_handler rescheduling itself\n"));
		schedule_async_handler(ap);
#endif
	}
	return (NOTIFY_DONE);
}

/*
 * Synchronous SIGIO event handler.
 * Calls sync_handler() to keep the audio data transfer running
 */
/*ARGSUSED*/
Notify_value
sigio_sync(
	Notify_client		client,
	Notify_event		event,
	Notify_arg		arg,
	Notify_event_type	when)
{
        struct atool_panel_data	*ap;
 
        ap = (struct atool_panel_data *) client;

	DBGOUT((9, "sync_handler called at sync level\n"));
	if (sync_handler(ap) == TRUE) {
		DBGOUT((7, "sync_handler rescheduling itself\n"));
		schedule_async_handler(ap);
	}
	return (NOTIFY_DONE);
}

/* catch broken pipe, and throw it back. :-) */
/*ARGSUSED*/
Notify_value
sigpipe_sync(
	Notify_client		client,
	Notify_event		event,
	Notify_arg		arg,
	Notify_event_type	when)
{
	DBGOUT((1, "warning: broken pipe\n"));
	return (NOTIFY_DONE);
}

/*
 * Synchronous timer handler.
 * Calls update_handler() to keep the display updated and check for eof.
 */
/*ARGSUSED*/
Notify_value
timer_sync_handler(
	Notify_client		client,
	int			which)
{
	struct itimerval	timer;
        struct atool_panel_data *ap;

        ap = (struct atool_panel_data *) client;

	/* Update the display */
	update_handler(ap);

	/* XXX - execute the async handler if needed */
	sigio_block(ap);
	if (SCHEDASYNC) {
		DBGOUT((9, "sync_handler called from timer routine\n"));
		if (SCHEDASYNC = sync_handler(ap)) {
#ifdef notdef
			DBGOUT((7, "sync_handler rescheduling itself\n"));
			schedule_async_handler(ap);
#endif
		}
	}
	sigio_unblock(ap);

	/* reschedule the next timer, if not stopped */
	switch (ap->tstate) {
	case Playing:
	case Recording:
	case Loading:
	case Saving:
		DBGOUT((12, "timer rescheduled\n"));

		timer.it_value.tv_usec = ap->timer_delay; /* 20 Hz */
		timer.it_value.tv_sec = 0;
		timer.it_interval.tv_usec = 0;
		timer.it_interval.tv_sec = 0;
		(void) notify_set_itimer_func((Xv_opaque)ap,
		    timer_sync_handler, ITIMER_REAL,
		    &timer, ((struct itimerval *)0));
		break;
	default:
		DBGOUT((9, "i/o finished...timer not rescheduled\n"));
		ap->timer_delay = 0;
		(void) notify_set_itimer_func((Xv_opaque)ap,
		    NOTIFY_FUNC_NULL, ITIMER_REAL, NULL, NULL);
		break;
	}
	return (NOTIFY_DONE);
}

/* Schedule timer */
AudPanel_START_TIMER(
	ptr_t			sp,
	long			usec)
{
	struct itimerval	timer;
	struct atool_panel_data *ap = (struct atool_panel_data *)sp;

	DBGOUT((9, "AudPanel_START_TIMER: called\n"));
	timer.it_value.tv_usec = usec;		/* caller specified (20 Hz) */
	timer.it_value.tv_sec = 0;
	timer.it_interval.tv_usec = 0;		/* reschedule each time */
	timer.it_interval.tv_sec = 0;

	ap->timer_delay = usec;

	(void) notify_set_itimer_func((Xv_opaque)sp,
	    timer_sync_handler, ITIMER_REAL,
	    &timer, ((struct itimerval *)0));
}

AudPanel_STOP_TIMER(
	ptr_t			sp)
{
	struct atool_panel_data *ap = (struct atool_panel_data *)sp;
	struct itimerval	timer;

	DBGOUT((9, "AudPanel_STOP_TIMER: called\n"));

	ap->timer_delay = 0;

#ifdef notdef
	timer.it_value.tv_usec = 0;
	timer.it_value.tv_sec = 0;
	timer.it_interval.tv_usec = 0;
	timer.it_interval.tv_sec = 0;
	notify_set_itimer_func((Xv_opaque)sp, NOTIFY_FUNC_NULL, ITIMER_REAL,
			NULL, NULL);
#endif
}

/* catch ^C and clean up ... */

Notify_value
sigint_sync(
	Notify_client		client,
	int			sig,
	Notify_signal_mode	when)
{
        atool_Bframe_objects    *ip;
 
        ip = (atool_Bframe_objects *) xv_get(client, XV_KEY_DATA, INSTANCE);
	notify_post_destroy(ip->Bframe, DESTROY_PROCESS_DEATH, NOTIFY_SAFE);
	return (NOTIFY_DONE);
}

Notify_value
AudPanel_DESTROY_FUNC(
	Notify_client		client,
	Destroy_status		status)
{
	struct atool_panel_data	*ap;
	int			exit_status = 0;

	ap = AudPanel_KEYDATA((ptr_t)client);

	/* NOTE: AudPanel_Cleanup (and AudPanel_Stop) will get called
	 * twice: once for DESTROY_CHECKING, once for DESTROY_CLEANUP.
	 * this should not be a problem.
	 */
	if (status == DESTROY_CHECKING) {
		/* cleanup (ask for confirmation if file modified) */
	        if (!AudPanel_Cleanup(ap, TRUE)) {
		   notify_veto_destroy(client);
		   return NOTIFY_DONE;
		}
		/*
		 * for tooltalk launched tool, just hide & sleep for a while 
		 * in case we get another tooltalk launch. to make this work,
		 * go into hiding and veto the destroy request.
		 */
		if (ap->tt_started == TRUE) {
			set_tt_quit_timer(TRUE);
			AudPanel_Hide(ap);
			notify_veto_destroy(client);
			return NOTIFY_DONE;
		}
		/* if not any of the above, we'll be back */
		return (NOTIFY_DONE);
	}

	if (status == DESTROY_SAVE_YOURSELF) {
		/* XXX - should save default menu items, etc. */
		return (NOTIFY_DONE);
	}

	/* hide ourself so users can't see how long exiting takes. ;-) */
	xv_set((Xv_opaque)ap->panel, XV_SHOW, FALSE, 0);

	switch (status) {
	case DESTROY_CLEANUP:
		break;
	case DESTROY_PROCESS_DEATH:
	default:
		exit_status = 1;
		break;
	}

	/* if we've gotten this far, we're outta here ... */
	(void) AudPanel_Cleanup(ap, FALSE);

	/* always call this, just in case ... */
	quit_tt();

#ifndef PRE_493
	/* send quit msg to audiocontrol if one's running */
	dstt_send_quit();
#endif
	exit(exit_status);
}

/* Set LED meter on main panel */
void
AudPanel_SETLEVEL(
	ptr_t			sp,
	double			val,
	int			clip)
{
	atool_Bframe_objects	*ip;
	int			ival;

	/* XXX - no clipping indicator, so set meter to max if clipped */
	if (clip)
		val = 1.0;

	ip = (atool_Bframe_objects *)
	    xv_get((Xv_opaque)sp, XV_KEY_DATA, INSTANCE);
	ival = irint( ((double)xv_get(ip->VuMeter, PANEL_MAX_VALUE)) * val);
	xv_set(ip->VuMeter, PANEL_VALUE, ival, NULL);
}

/* Set the Audio Panel Pointer Label */
void
AudPanel_POINTERLABEL(
	struct atool_panel_data	*ap,
	char			*str)
{
	atool_Bframe_objects	*ip;

	ip = (atool_Bframe_objects *) 
	    xv_get((Xv_opaque) ap->panel, XV_KEY_DATA, INSTANCE);
	xv_set(ip->Time_label, PANEL_LABEL_STRING, str, NULL);
}

/* Set the Audio Panel Pointer Time */
void
AudPanel_POINTERTIME(
	struct atool_panel_data	*ap,
	char			*time_str)
{
	atool_Bframe_objects	*ip;
	char			str[40];	/* right adjusted string */

	ip = (atool_Bframe_objects *) 
	    xv_get((Xv_opaque) ap->panel, XV_KEY_DATA, INSTANCE);

	/* right adjust time and truncate to 10 characters */
	(void) sprintf(str, ap->Time_string, time_str);
	xv_set(ip->Time_val, PANEL_LABEL_STRING, str, NULL);
}

/* return release name */
char*
audiotool_release()
{
#ifdef DEBUG
	if (Version_String != NULL)
		return (Version_String);
#endif
	return ((char *)ds_relname());
}

#ifndef PRE_493
/* for dstt init */
void
audiotool_version(
	char	**vend,
	char	**name,
	char	**ver)
{
        *vend = (char *)ds_vendname();
        *name = "Audio Tool";
        *ver = audiotool_release();
}
#endif

/* Load up dynamic button icons */
static unsigned short	Play_button_bits[] = {
#include "icon/play_wide.icon"
};
static unsigned short	Stop_Play_button_bits[] = {
#include "icon/stop_wide.icon"
};
static unsigned short	Stop_Record_button_bits[] = {
#include "icon/stop.icon"
};

/* Callback for test_exec() */
static void
test_callback(
	caddr_t			apd,
	int			status)
{
	char	buf[2 * MAXPATHLEN];
	struct atool_panel_data	*ap = (struct atool_panel_data *)apd;

	if (status != 0) {
		(void) sprintf(buf, MGET(
		    "The audioconvert program cannot be located.\n"
		    "This program is used to perform audio format conversions "
		    "and is ordinarily installed in /usr/bin.\n"
		    "Install the SUNWaudio package or ask your "
		    "system administrator to make this program available.\n\n"
		    "You may continue now, but loading and saving of "
		    "compressed audio data will fail."));

		if (AudPanel_Choicenotice(ap, buf,
		    MGET("Quit"), MGET("Continue"), NULL) == 1)
			exit(1);
	}
}

/* Create a Audio Panel */
ptr_t
AudPanel_INIT(
	ptr_t			owner,
	ptr_t			apd,
	int			*argcp,
	char			**argv)
{
	atool_Bframe_objects	*ip = NULL;
	struct atool_panel_data	*ap;	/* struct data pointer */
	register char 		**args;	/* scan ahead of argv */
	int			nargs;
	int			i;
	int			w;
	Xv_opaque		server;
	Cms			cms;
	Xv_singlecolor		color;
	unsigned long		green;
	unsigned long		red;
	Panel_item	 	meter;
	Menu			new_menu;
	Menu			undo_menu;
	Menu			redo_menu;
	Menu_item		mi;
#ifndef PRE_493
	Tt_status		status;
#endif

	ap = (struct atool_panel_data *) apd;

	/*
	 * do an initial scan of arguments to find things we need before
	 * startup. we'll make another pass later on to get the rest.
	 */
	for (args = argv, nargs = *argcp; nargs && args && *args; nargs--) {
		if (!strcmp(*args, "-v")) {
			atool_xv_version(argv[0]); /* print version & exit */
			exit(0);
		} else if (!strcmp(*args, "-tooltalk")) {
			/* Check if we've been started by tooltalk */
                        ap->tt_type = TT_V3compat;
			ap->tt_started = TRUE;
			scrunch_args(argcp, argv, args, 1);
#ifndef PRE_493
		} else if (!strcmp(*args, "-message_alliance")) {
			/* check for new tooltalk interface */
                        ap->tt_type = TT_MediaExchange;
			ap->tt_started = TRUE;
			scrunch_args(argcp, argv, args, 1);
#endif
#ifdef DEBUG_PRINT
		} else if (!strncmp(*args, "-D", 2)) {
			/* to enable debugging ... */
			if ((*args)[2]) {
				/* make sure this is -dN (N=0-9) */
				if (isdigit((*args)[2])) {
					/* set debug level */
					set_debug_level(atoi(&((*args)[2])));
					scrunch_args(argcp, argv, args, 1);
				} else {
					/* nope, skip this one */
					args++;
				}
			} else {
				/* no digit, set to level 1 */
				set_debug_level(1);
				scrunch_args(argcp, argv, args, 1);
			}
#endif
#ifdef DEBUG
		} else if (!strncmp(*args, "-V", 2)) {
			/* to set version string ... */
			Version_String = strdup(&(*args)[2]);
			scrunch_args(argcp, argv, args, 1);
#endif
		} else {
			args++;
	    }
	}

        /* Initialize tooltalk.  Must be done before xv_init() */
	if (ap->tt_type == TT_V3compat) {
		DBGOUT((1, "** initing OLD Tooltalk interface\n"));
		/* reset this flag if it failed */
		if (start_tt_init(Progname, TRUE, *argcp, argv) == -1) {
			ap->tt_type = TT_None;
			if (ap->tt_started == TRUE) {
				ap->tt_started = FALSE;
			}
		}
	}

#ifndef PRE_493
	/* XXX - always init new TT interface? */
	if (ap->tt_type == TT_V3compat) {
		ap->tt_type = TT_Both;
	} else {
		ap->tt_type = TT_MediaExchange;
	}
	DBGOUT((1, "** initing dstt Media Exchange interface\n"));
	if (status = dstt_check_startup(audiotool_version, 
					argcp, &argv)) {
		print_tt_emsg(MGET("Could not initialize Tool Talk"),
			      status);
		if (ap->tt_type == TT_Both) {
			ap->tt_type = TT_V3compat;
		} else {
			ap->tt_type = TT_None;
			if (ap->tt_started == TRUE) {
				ap->tt_started = FALSE;
			}
		}
	}
	DBGOUT((1, "** done initing dstt Media Exchange.\n"));
#endif

	/*
         * Initialize XView.
         * As the default, use the current directory as a pointer to
         * the the Resource Database (.db) file. XView will search for
         * the file in the "./<current_locale>/app-defaults" directory.
         * Change the value of XV_LOCALE_DIR if the .db file resides
         * in some other location.
         */
	server = xv_init(
	    XV_INIT_ARGC_PTR_ARGV, argcp, argv, 
	    XV_USAGE_PROC, atool_usage_proc,
	    XV_USE_LOCALE, TRUE,
	    0);
#ifndef PRE_493
	xv_set(server, XV_APP_NAME, MGET("Audio Tool"), NULL);
#endif

#ifndef PROFILED
	ce_begin(NULL);
#endif

	/* Init the translated string cache */
	ap->Time_string = strdup(MGET("%10.10s"));
	ap->Play_string = strdup(MGET("Play"));
	ap->Stop_string = strdup(MGET("Stop"));
	ap->Rec_string = strdup(MGET("Rec"));
	ap->Cursor_string = strdup(MGET("Cursor:"));
	ap->Pointer_string = strdup(MGET("Pointer:"));
	ap->Length_string = strdup(MGET("Length: %6s             "));
	ap->Lengthsel_string = strdup(MGET("Length: %6s [%6s]"));
	ap->Space_string = strdup(MGET("Space remaining: %2d seconds"));

	/* Init global keys, if necessary */
	if (INSTANCE == 0)
		INSTANCE = xv_unique_key();
	if (AUD_TOOL_KEY == 0)
		AUD_TOOL_KEY = xv_unique_key();

	/* Initialize XView status panel */
	if ((ip = atool_Bframe_objects_initialize(NULL, (Xv_opaque)owner))
	    == NULL)
		return (NULL);
	ap->panel = (ptr_t) ip->Bframe;
	
	/* All the layout is done relative to the top panel's width */
	window_fit_width(ip->Audio_file_controls);

	/* adjust for poor height fit */
	i = xv_get(ip->CursorTime, XV_Y);
	i += xv_get(ip->PointerTime, XV_Y) +
	    xv_get(ip->PointerTime, XV_HEIGHT);
	xv_set(ip->Audio_file_controls, XV_HEIGHT, i, NULL);
	xv_set(ip->Left_border_controls, XV_Y, i, NULL);

	/* set audio display canvas shape, don't save canvas bitmap */
	xv_set(ip->Display_canvas, XV_Y, i,
	    XV_WIDTH, xv_get(ip->Audio_file_controls, XV_WIDTH) -
	    xv_get(ip->Level_control_panel, XV_WIDTH),
	    XV_HEIGHT, xv_get(ip->Audio_tool_controls, XV_Y) - i,
	    CANVAS_RETAINED, FALSE,
	    NULL);

	/* move right panel to right edge of canvas, below top row */
	ap->right_panel_x = xv_get(ip->Display_canvas, XV_X) +
	    xv_get(ip->Display_canvas, XV_WIDTH);
	xv_set(ip->Level_control_panel, XV_X, ap->right_panel_x,
	    XV_Y, xv_get(ip->Audio_file_controls, XV_HEIGHT), NULL);

	/*
	 * Guide doesn't provide a way of specifying icon image size on 
	 * creation.  Work around Guide, by destroying 64 X 64 button it 
	 * created, and create a new one that's 64 X 32, using same image.
	 */
	ap->play_button_image = 
	    (ptr_t) xv_get(ip->Play_button, PANEL_LABEL_IMAGE);
	xv_destroy((Xv_opaque)ap->play_button_image);
	ap->play_button_image = (ptr_t) xv_create(XV_NULL, SERVER_IMAGE,
		SERVER_IMAGE_BITS, Play_button_bits,
		SERVER_IMAGE_DEPTH, 1,
		XV_WIDTH, 64,
		XV_HEIGHT, 32,
		NULL);
	xv_set(ip->Play_button, PANEL_LABEL_IMAGE, 
	    (Xv_opaque) ap->play_button_image, NULL);

	/* call Fwd/RevButton_held proc when respective button is held down */
	Button_init(NULL, ip->Fwd_button, FwdButton_held, FwdButton_letgo);
	Button_init(NULL, ip->Rev_button, RevButton_held, RevButton_letgo);

	/* adjust for poor fit */
	i = (2 * xv_get(ip->Reverse, XV_Y)) +
	    xv_get(ip->Reverse, XV_HEIGHT);
	xv_set(ip->Audio_tool_controls, XV_HEIGHT, i, NULL);

	/* extend the lower panel to edge of right panel */
	xv_set(ip->Audio_tool_controls, XV_WIDTH,
	    xv_get(ip->Level_control_panel, XV_X) -
	    xv_get(ip->Left_border_controls, XV_WIDTH) + 1, NULL);

	/* if the frame is very wide, re-space the control buttons */
	xv_set(ip->Include, GROUP_LAYOUT, TRUE, NULL);
	w = xv_get(ip->Rec_button, XV_WIDTH);
	i = xv_get(ip->Done_button, XV_X) - xv_get(ip->Rec_button, XV_X) -
	    (3 * w);
	if (i > 0) {
		if (i > (2 * w))
			i = (2 * w);
		i /= 6;			/* 2 thin + 1 thick (4x) */
		xv_set(ip->FFRewPlay, GROUP_HORIZONTAL_SPACING,
		    i + xv_get(ip->FFRewPlay, GROUP_HORIZONTAL_SPACING),
		    GROUP_LAYOUT, TRUE, NULL);
		xv_set(ip->TransportControls, GROUP_HORIZONTAL_SPACING,
		    (4 * i) +
		    xv_get(ip->TransportControls, GROUP_HORIZONTAL_SPACING),
		    GROUP_LAYOUT, TRUE, NULL);
		xv_set(ip->Include,
		    XV_X, xv_get(ip->Rec_button, XV_X) + (3 * w), NULL);
	}

	/* initialize layout variables */
	ap->frame_width = xv_get(ip->Audio_file_controls, XV_WIDTH) +
	    xv_get(ip->Left_border_controls, XV_WIDTH);
	ap->lower_panel_width = xv_get(ip->Audio_tool_controls, XV_WIDTH);
	ap->lower_panel_y = xv_get(ip->Audio_tool_controls, XV_Y);
	ap->frame_height = ap->lower_panel_y +
	    xv_get(ip->Audio_tool_controls, XV_HEIGHT);

	/* set frame width and height */
	xv_set(ip->Bframe,
	    XV_WIDTH, ap->frame_width, XV_HEIGHT, ap->frame_height,
	    FRAME_MIN_SIZE, ap->frame_width, ap->frame_height, NULL);

	/* make panels extend automatically after resizes */
	xv_set(ip->Left_border_controls, XV_HEIGHT, WIN_EXTEND_TO_EDGE, NULL);
	xv_set(ip->Level_control_panel, XV_HEIGHT, WIN_EXTEND_TO_EDGE, NULL);
	xv_set(ip->Audio_tool_controls, XV_HEIGHT, WIN_EXTEND_TO_EDGE, NULL);

	/* XXX - devguide should do this ... */
	xv_set(xv_get(ip->Bframe, FRAME_ICON), ICON_TRANSPARENT, TRUE, 0);

	/* if color, find red & green colormap entries for the led meter */
	cms = ds_cms_create(ip->Bframe);
	if (cms != NULL) {
		color.red = 0;
		color.green = 255;
		color.blue = 0;
		green = ds_cms_index(cms, &color);
		color.red = 255;
		color.green = 0;
		red = ds_cms_index(cms, &color);
		xv_set(ip->Level_control_panel, WIN_CMS, cms, NULL);
		ds_set_colormap(ip->Bframe, cms,
		    DS_NULL_CMS_INDEX, DS_NULL_CMS_INDEX);
	}

	/* install the led meter where the placeholder is */
	meter = ip->VuMeter;
	i = ap->frame_width - (2 * xv_get(ip->Left_border_controls, XV_WIDTH)) -
	    ap->lower_panel_width - xv_get(meter, XV_X);
	ip->VuMeter = (Panel_meter)
	    xv_create(ip->Level_control_panel, PANEL_METER_ITEM, 
	    XV_X, xv_get(meter, XV_X),
	    XV_Y, xv_get(meter, XV_Y),
	    PANEL_DIRECTION, PANEL_VERTICAL,
	    PANEL_METER_HEIGHT, 
		xv_get(ip->Level_control_panel, XV_HEIGHT) -
		(2 * xv_get(meter, XV_Y)),
	    PANEL_METER_WIDTH, i,
	    PANEL_METER_LEDS, 10,
#ifdef notdef
	    PANEL_METER_DEBUG, TRUE, /* XXX - temp debugging */
#endif
	    NULL);
	xv_destroy(meter);	/* don't need the placeholder anymore */

	/* align the drop target with the right edge of the meter */
	xv_set(ip->drop_target, XV_X, xv_get(ip->Level_control_panel, XV_X) +
	    xv_get(ip->VuMeter, XV_X) + i - xv_get(ip->drop_target, XV_WIDTH),
	    NULL);

	/* setup the colors for the PANEL_METER_ITEM */
	if (cms != NULL) {
		xv_set(ip->VuMeter,
		    PANEL_METER_LED_COLOR, green,
		    PANEL_METER_OVERLD_COLOR, red,
		    NULL);
	}

	/* interpose on the destroy func for cleanup. */
	notify_interpose_destroy_func(ip->Bframe, AudPanel_DESTROY_FUNC);

	/* initialize signal catchers */
	init_intr(ip->Bframe, ap);

	/* blank out time cursor display */
	AudPanel_CURSORLABEL((ptr_t)ap, "");
	AudPanel_CURSORTIME((ptr_t)ap, "");

	/* get handle to record button */
	ap->record_button_image = 
	    (ptr_t) xv_get(ip->Rec_button, PANEL_LABEL_IMAGE);

	/* create the stop buttons */
	ap->stop_play_button_image = (ptr_t) xv_create(XV_NULL, SERVER_IMAGE,
		SERVER_IMAGE_BITS, Stop_Play_button_bits,
		SERVER_IMAGE_DEPTH, 1,
		XV_WIDTH, 64,
		XV_HEIGHT, 32,
		NULL);
	ap->stop_record_button_image = (ptr_t) xv_create(XV_NULL,SERVER_IMAGE,
		SERVER_IMAGE_BITS, Stop_Record_button_bits,
		SERVER_IMAGE_DEPTH, 1,
		XV_WIDTH, 32,
		XV_HEIGHT, 32,
		NULL);

	/* set the tool state */
	ap->tstate = Unloaded;
	ap->modified = FALSE;
	ap->empty = TRUE;

	/* init V3 Drag N Drop/SelSvc */
	Audio_Sel_INIT(ap, ip->Bframe, ip->Display_canvas, ip->drop_target);

	/* Initialize sub panels */
	ap->subp.segment = SegCanvas_Init((ptr_t)ip->Bframe, 
	    (ptr_t)ip->Display_canvas, Cursor_proc, Pointer_proc, 
	    Set_insert_proc, Rubberband_select_proc, Select_proc);
	if (ap->subp.segment == NULL)
		return (NULL);

	/* interpose on the canvas paint win to get kbd events, etc., 
	 * before the segment code can get to 'em.
	 */
	notify_interpose_event_func(canvas_paint_window(ip->Display_canvas),
		(Notify_func) atool_canvas_event, NOTIFY_SAFE);

	if ((ap->subp.file = FilePanel_Init((ptr_t)ip->Bframe,
	    Open_proc, Save_proc, Include_proc, File_filter_proc)) == NULL) {
		return (NULL);
	}

	if ((ap->subp.config = ConfigPanel_Init((ptr_t)ip->Bframe, 
	    Config_autoplay_load_proc, Config_autoplay_sel_proc,
	    Config_confirm_proc, Config_silence_proc, Config_threshold_proc,
	    Config_tempdir_proc, Config_apply_proc)) == NULL) {
		return (NULL);
	}

	if ((ap->subp.format = FormatPanel_Init((ptr_t)ip->Bframe)) == NULL) {
		return (NULL);
	}

	/* clear pointer label and time */
#ifdef PRE_493
	/* Audio Control panels compiled into this code */
	ap->subp.playctl = PlayctlPanel_Init((ptr_t)ip->Bframe, ap->devname);
	ap->subp.reclevel = ReclevelPanel_Init((ptr_t)ip->Bframe, ap->devname);
#endif

	AudPanel_POINTERLABEL((ptr_t)ap, "");
	AudPanel_POINTERTIME((ptr_t)ap, "");

	/* Save the address of local storage */
	xv_set(ip->Bframe, XV_KEY_DATA, AUD_TOOL_KEY, ap, NULL);

	/* associate INSTANCE & AUD_TOOL_KEY with canvas paint window
	 * so we can get to it in the event handler.
	 */
	xv_set(canvas_paint_window(ip->Display_canvas),
	    XV_KEY_DATA, AUD_TOOL_KEY, ap, 
	    XV_KEY_DATA, INSTANCE, ip,
	    NULL);

	/* destroy (hide) it for now, will bring it up when needed */
	AudPanel_DESTROY_DONE_BUTTON((ptr_t)ip->Bframe);

	/* Store menu handles and init File->New menu (so default works) */
	ap->edit_menu = (ptr_t) xv_get(ip->Edit_button, PANEL_ITEM_MENU);
	ap->file_menu = (ptr_t) xv_get(ip->File_button, PANEL_ITEM_MENU);

	/* Create the File->New menu so that the default action works */
	if (mi = (Menu_item) xv_find((Menu)ap->file_menu, MENUITEM,
	    XV_AUTO_CREATE, FALSE, MENU_STRING, MGET("New"), NULL)) {
		new_menu = gen_format_menu(mi, MENU_DISPLAY);

		/* must set the menu *after* setting the gen proc */
		xv_set(mi, MENU_GEN_PULLRIGHT, gen_format_menu,
		    MENU_PULLRIGHT, new_menu,
		    NULL);
	}

#ifndef PRE_493
	/* set up menu acclerators for relevant menu items */
	add_menu_accel((Xv_opaque)ap->file_menu,
				MGET("Open..."), "Meta+o");
	add_menu_accel((Xv_opaque)ap->file_menu,
				MGET("Save"), "Meta+s");
	add_menu_accel((Xv_opaque)ap->edit_menu,
				MGET("Cut"), "Meta+x");
	add_menu_accel((Xv_opaque)ap->edit_menu,
				MGET("Copy"), "Meta+c");
	add_menu_accel((Xv_opaque)ap->edit_menu,
				MGET("Paste"), "Meta+v");
	add_menu_accel((Xv_opaque)ap->edit_menu,
				MGET("Properties..."), "Meta+i");
	undo_menu = add_submenu_accel((Xv_opaque)ap->edit_menu,
				MGET("Undo"), "Meta+z");
	redo_menu = add_submenu_accel((Xv_opaque)ap->edit_menu,
				MGET("Redo"), "Shift+Meta+z");
	new_menu = add_submenu_accel((Xv_opaque)ap->file_menu,
				MGET("New"), "Meta+n");
#ifdef notdef
	add_menu_accel((Xv_opaque)ap->edit_menu,
				MGET("Select All"), "Meta+A");
#endif

	/* now make the frame aware of these suckkas */
	xv_set(ip->Bframe, FRAME_MENUS,
	    ap->edit_menu, ap->file_menu, new_menu, undo_menu, redo_menu, NULL,
	    NULL);
#endif

	/* Check to see whether audioconvert exists.  If not, complain. */
	if (!test_exec("audioconvert", test_callback, ap)) {
		test_callback((caddr_t)ap, 99);
	}

	return ((ptr_t) ip->Bframe);
}

#ifndef PRE_493
void
add_menu_accel(
	Menu		menu,
	char		*label,
	char		*accel)
{
	Menu_item	mi;

	if (mi = (Menu_item) xv_find(menu, MENUITEM,
	    XV_AUTO_CREATE, FALSE, MENU_STRING, label, NULL)) {
		xv_set(mi,  MENU_ACCELERATOR, accel, NULL);
	}
}

Menu
add_submenu_accel(
	Menu		menu,
	char		*label,
	char		*accel)
{
	Menu_item	mi;
	Menu_item	smi;
	Menu		m;
	int		(*gen_proc)();

	if (mi = (Menu_item) xv_find(menu, MENUITEM,
	    XV_AUTO_CREATE, FALSE, MENU_STRING, label, NULL)) {
		if (!(m = (Menu)xv_get(mi, MENU_PULLRIGHT))) {
			/* try to gen the pullright */
			if (gen_proc = (PFUNCI)
			    xv_get(mi, MENU_GEN_PULLRIGHT)) {
				m = (Menu)(*gen_proc)(mi, MENU_DISPLAY);
			} else {
				return (NULL);
			}
		}
		if (smi = (Menu_item)xv_get(m, MENU_NTH_ITEM, 1)) {
			xv_set(smi, MENU_ACCELERATOR, accel, NULL);
			return (m);
		}
	}
	return (NULL);
}
#endif /* 493 */

/* Set the Audio Panel Cursor Label */
void
AudPanel_CURSORLABEL(
	struct atool_panel_data	*ap,
	char			*str)
{
	atool_Bframe_objects	*ip;

	ip = (atool_Bframe_objects *) 
	    xv_get((Xv_opaque) ap->panel, XV_KEY_DATA, INSTANCE);

	xv_set(ip->Cursor_label, PANEL_LABEL_STRING, str, NULL);
}

/* Set the Audio Panel Cursor Time */
void
AudPanel_CURSORTIME(
	struct atool_panel_data	*ap,
	char			*time_str)
{
	atool_Bframe_objects	*ip;
	char			str[40];	/* right adjusted string */

	ip = (atool_Bframe_objects *) 
	    xv_get((Xv_opaque) ap->panel, XV_KEY_DATA, INSTANCE);

	/* right adjust time and truncate to 10 characters */
	(void) sprintf(str, ap->Time_string, time_str);
	xv_set(ip->Cursor_val, PANEL_LABEL_STRING, str, NULL);
}

/* Set the Audio Panel Label */
void
AudPanel_SETLABEL(
	ptr_t		sp,
	char		*str)
{
	xv_set((Xv_opaque)sp, FRAME_LABEL, str, 0);
}

/* Set the Audio Panel Label */
void
AudPanel_SETICONLABEL(
	ptr_t		sp,
	char		*str)
{
	Icon		icon;

	if (!(icon = (Icon) xv_get((Xv_opaque)sp, FRAME_ICON)))
	    return;		/* shouldn't happen ... */
	
	xv_set(icon, XV_LABEL, str, 0);
	xv_set((Xv_opaque)sp, FRAME_ICON, icon, NULL);
}

/* Set the Audio panel's left footer */
void
AudPanel_ERRORBELL(
	ptr_t		sp)
{
	xv_set((Xv_opaque)sp, WIN_ALARM, 0);
}

/* Set the Audio panel's left footer */
void
AudPanel_SETLEFTFOOTER(
	ptr_t		sp,
	char		*str)
{
	xv_set((Xv_opaque)sp, FRAME_LEFT_FOOTER, str, 0);
}

/* Set the Audio panel's right footer */
void
AudPanel_SETRIGHTFOOTER(
	ptr_t		sp,
	char		*str)
{
	xv_set((Xv_opaque)sp, FRAME_RIGHT_FOOTER, str, 0);
}

void
AudPanel_SHOWTYPEINFO(
	ptr_t			sp,
	char			*typestr)
{
	AudPanel_SETLEFTFOOTER(sp, (typestr != NULL) ? typestr : "");
}

/* is audiotool iconic? */
int
AudPanel_ISICON(
	ptr_t			sp)
{
	return ((int)xv_get((Xv_opaque)sp, FRAME_CLOSED));
}

/* set record button image */
void
AudPanel_SETRECORDBUTTON(
	ptr_t			sp,
	Xv_opaque		button_image,
	char			*label)
{
	atool_Bframe_objects	*ip;

	ip = (atool_Bframe_objects *) 
	    xv_get((Xv_opaque) sp, XV_KEY_DATA, INSTANCE);

	/* set button image and label */
	xv_set(ip->Rec_button,
	    PANEL_LABEL_IMAGE, (Xv_opaque) button_image, NULL);
	xv_set(ip->RecLabel, PANEL_LABEL_STRING, label, NULL);
}

/* (de)activate search buttons */
void
AudPanel_ACTIVATESEARCH(
	ptr_t			sp,
	int			activate)
{
	atool_Bframe_objects	*ip;

	ip = (atool_Bframe_objects *) 
	    xv_get((Xv_opaque) sp, XV_KEY_DATA, INSTANCE);

	xv_set(ip->Fwd_button, PANEL_INACTIVE, !activate, NULL);
	xv_set(ip->Rev_button, PANEL_INACTIVE, !activate, NULL);
}

void
AudPanel_ACTIVATEPLAYREC(
	ptr_t			sp,
	int			activate)
{
	atool_Bframe_objects	*ip;
	struct atool_panel_data	*ap;

	ip = (atool_Bframe_objects *) 
	    xv_get((Xv_opaque) sp, XV_KEY_DATA, INSTANCE);

	xv_set(ip->Play_button, PANEL_INACTIVE, !activate, NULL);
	xv_set(ip->Rec_button, PANEL_INACTIVE, !activate, NULL);

	/* If no audio control device, disable Volume button */
	ap = AudPanel_KEYDATA((ptr_t)sp);
	if (ap->ctldev == NULL)
		xv_set(ip->Volume_button, PANEL_INACTIVE, TRUE, NULL);
}

/* (de)activate all but the transport control buttons */
void
AudPanel_ACTIVATEBUTTONS(
	ptr_t			sp,
	int			activate)
{
	atool_Bframe_objects	*ip;
	Frame			menuframe;
	struct atool_panel_data	*ap;
	int			devact;

	ip = (atool_Bframe_objects *) 
	    xv_get((Xv_opaque) sp, XV_KEY_DATA, INSTANCE);

	/* Set a separate flag for the Volume button activation */
	devact = activate;
	if (activate) {
		ap = AudPanel_KEYDATA((ptr_t)sp);
		if (ap->ctldev == NULL)
			devact = FALSE;
	}

	/* enable/disable buttons and their menus */
	menuframe = (Frame) xv_get(
	    xv_get(ip->File_button, PANEL_ITEM_MENU), MENU_PIN_WINDOW);
	if (menuframe != NULL)
		xv_set(menuframe, FRAME_BUSY, !activate, NULL);
	xv_set(ip->File_button, PANEL_INACTIVE, !activate, NULL);

	menuframe = (Frame) xv_get(
	    xv_get(ip->Edit_button, PANEL_ITEM_MENU), MENU_PIN_WINDOW);
	if (menuframe != NULL)
		xv_set(menuframe, FRAME_BUSY, !activate, NULL);
	xv_set(ip->Edit_button, PANEL_INACTIVE, !activate, NULL);

	menuframe = (Frame) xv_get(
	    xv_get(ip->Volume_button, PANEL_ITEM_MENU), MENU_PIN_WINDOW);
	if (menuframe != NULL)
		xv_set(menuframe, FRAME_BUSY, !devact, NULL);
	xv_set(ip->Volume_button, PANEL_INACTIVE, !devact, NULL);

	if (ip->Done_button != NULL)
		xv_set(ip->Done_button, PANEL_INACTIVE, !activate, NULL);

	/* enable/disable canvas menu if pinned */
	menuframe = (Frame) xv_get(
	    xv_get(canvas_paint_window(ip->Display_canvas), WIN_MENU),
	    MENU_PIN_WINDOW);
	if (menuframe != NULL)
		xv_set(menuframe, FRAME_BUSY, !activate, NULL);

	/* enable/disable drop target */
	xv_set(ip->drop_target, FRAME_BUSY, !activate, NULL);
}

/* set play button image */
void
AudPanel_SETPLAYBUTTON(
	ptr_t			sp,
	Xv_opaque		button_image,
	char			*label)
{
	atool_Bframe_objects	*ip;

	ip = (atool_Bframe_objects *) 
	    xv_get((Xv_opaque) sp, XV_KEY_DATA, INSTANCE);

	/* set button image */
	xv_set(ip->Play_button, PANEL_LABEL_IMAGE, 
	    (Xv_opaque) button_image, NULL);
	/* and label */
	xv_set(ip->PlayLabel, PANEL_LABEL_STRING, label, NULL);
}

/* create done button for doing INSERT back to calling app */
void
AudPanel_CREATE_DONE_BUTTON(
	ptr_t			sp,
	char			*file_type)
{
	atool_Bframe_objects	*ip;
	Server_image		img;

	ip = (atool_Bframe_objects *)
	    xv_get((Xv_opaque)sp, XV_KEY_DATA, INSTANCE);

	/* create the done button */
	if (!ip->Done_button) {
		ip->Done_button = atool_Bframe_Done_button_create(ip,
		    ip->Audio_tool_controls);
		ip->done_label = atool_Bframe_done_label_create(ip,
		    ip->Audio_tool_controls);
	}
	if (img = get_ce_icon(file_type)) {
		xv_set(ip->Done_button, PANEL_LABEL_IMAGE, img, NULL);
	}
	xv_set(ip->Done_button, XV_SHOW, TRUE, PANEL_BUSY, FALSE, NULL);
	xv_set(ip->done_label, XV_SHOW, TRUE, NULL);

}

void
AudPanel_DESTROY_DONE_BUTTON(
	ptr_t			sp)
{
	atool_Bframe_objects	*ip;

	ip = (atool_Bframe_objects *)
	    xv_get((Xv_opaque)sp, XV_KEY_DATA, INSTANCE);

	if (ip->Done_button) {
		/* make sure button's in normal state */
		xv_set(ip->Done_button, 
		       PANEL_BUSY, TRUE, 
		       XV_SHOW, FALSE,
		       NULL);

		xv_set(ip->done_label, XV_SHOW, FALSE, NULL);
	}
}

/* set the done button back to normal */
void
AudPanel_RESET_DONE_BUTTON(
	ptr_t			sp)
{
	atool_Bframe_objects	*ip;

	ip = (atool_Bframe_objects *)
	    xv_get((Xv_opaque)sp, XV_KEY_DATA, INSTANCE);

	if (ip->Done_button) {
		xv_set(ip->Done_button, 
		       PANEL_BUSY, FALSE,
		       PANEL_INACTIVE, FALSE,
		       NULL);
	}
}

/* set the done button back to inactive */
void
AudPanel_INACTIVE_DONE_BUTTON(
	ptr_t			sp)
{
	atool_Bframe_objects	*ip;

	ip = (atool_Bframe_objects *)
	    xv_get((Xv_opaque)sp, XV_KEY_DATA, INSTANCE);

	if (ip->Done_button) {
		xv_set(ip->Done_button, 
		       PANEL_BUSY, FALSE,
		       PANEL_INACTIVE, TRUE,
		       NULL);
	}
}

/*
 * Post an alert box with up to 3 choices.
 * Return: 1 for l1, 2 for l2, 3 for l3.
 */
AudPanel_CHOICENOTICE(
	ptr_t			sp,
	char			*str,
	char			*l1,
	char			*l2,
	char			*l3)
{
	atool_Bframe_objects	*ip;
	int			result = 0; /* response to alert question */
	Xv_Notice		notice;

	ip = (atool_Bframe_objects *) 
	    xv_get((Xv_opaque) sp, XV_KEY_DATA, INSTANCE);

	notice = xv_create(ip->Bframe, NOTICE,
			   NOTICE_MESSAGE_STRING,	str, 
			   NOTICE_NO_BEEPING,		FALSE,
			   NOTICE_STATUS,		&result,
			   NOTICE_BUTTON,		l1, 1,
			   NOTICE_LOCK_SCREEN,		FALSE,
			   NULL);
	if (l3 != NULL) {
		xv_set(notice,
		   NOTICE_BUTTON, l1, 1,
		   NOTICE_BUTTON, l2, 2,
		   NOTICE_BUTTON, l3, 3,
		   NULL);
	} else if (l2 != NULL) {
		xv_set(notice,
		   NOTICE_BUTTON, l1, 1,
		   NOTICE_BUTTON, l2, 2,
		   NULL);
	}
	xv_set(notice, XV_SHOW, TRUE, NULL);
	xv_destroy_safe(notice);
	return (result);
}

/* Pop up the Audio Panel */
void
AudPanel_SHOW(
	ptr_t			sp)
{
	xv_set((Xv_opaque)sp, XV_SHOW, TRUE, FRAME_CLOSED, FALSE, 0);
}

/* Dismiss the Audio Panel */
void
AudPanel_UNSHOW(
	ptr_t			sp)
{
	Frame			menuframe;
	atool_Bframe_objects	*ip;

	ip = (atool_Bframe_objects *) 
	    xv_get((Xv_opaque) sp, XV_KEY_DATA, INSTANCE);

	/*
	 * XXX - workaround for XView bugid: 1100412
	 *       manually unpin all pinned menus
	 */
	menuframe = (Frame) xv_get(
	    xv_get(ip->File_button, PANEL_ITEM_MENU), MENU_PIN_WINDOW);
	if (menuframe != NULL)
		xv_set(menuframe, FRAME_CMD_PIN_STATE, FRAME_CMD_PIN_OUT,
		    XV_SHOW, FALSE, NULL);
	menuframe = (Frame) xv_get(
	    xv_get(ip->Edit_button, PANEL_ITEM_MENU), MENU_PIN_WINDOW);
	if (menuframe != NULL)
		xv_set(menuframe, FRAME_CMD_PIN_STATE, FRAME_CMD_PIN_OUT,
		    XV_SHOW, FALSE, NULL);
	menuframe = (Frame) xv_get(
	    xv_get(ip->Volume_button, PANEL_ITEM_MENU), MENU_PIN_WINDOW);
	if (menuframe != NULL)
		xv_set(menuframe, FRAME_CMD_PIN_STATE, FRAME_CMD_PIN_OUT,
		    XV_SHOW, FALSE, NULL);
	menuframe = (Frame) xv_get(
	    xv_get(canvas_paint_window(ip->Display_canvas), WIN_MENU),
	    MENU_PIN_WINDOW);
	if (menuframe != NULL)
		xv_set(menuframe, FRAME_CMD_PIN_STATE, FRAME_CMD_PIN_OUT,
		    XV_SHOW, FALSE, NULL);

	/* This should be all that is required */
	xv_set((Xv_opaque)sp, XV_SHOW, FALSE, 0);
}

/* Busy the Audio Panel */
void
AudPanel_BUSY(
	ptr_t			sp)
{
	xv_set((Xv_opaque)sp, FRAME_BUSY, TRUE, 0);
}

/* Un-Busy the Audio Panel */
void
AudPanel_UNBUSY(
	ptr_t			sp)
{
	xv_set((Xv_opaque)sp, FRAME_BUSY, FALSE, 0);
}

/* Get audio panel data from an Xview object handle in Audio Tool */
struct atool_panel_data  *
AudPanel_KEYDATA(
	ptr_t			obj)
{
	atool_Bframe_objects	*ip;
	struct atool_panel_data	*ap;

	ip = (atool_Bframe_objects *)
	    xv_get((Xv_opaque)obj, XV_KEY_DATA, INSTANCE);
	ap = (struct atool_panel_data *) 
		xv_get(ip->Bframe, XV_KEY_DATA, AUD_TOOL_KEY);
	return (ap);
}

void
AudPanel_SETFILEMENU(
	ptr_t			sp,
	int			can_save,
	int			can_saveas,
	int			can_load)
{
	struct atool_panel_data	*ap;
	Menu_item		mi;

	ap = AudPanel_KEYDATA((ptr_t)sp);
	if (ap->file_menu) {
		/* first set save item */
		mi = (Menu_item) xv_find((Menu)ap->file_menu, MENUITEM,
		    XV_AUTO_CREATE, FALSE, MENU_STRING, MGET("Save"), NULL);
		if (mi) {
			xv_set(mi, MENU_INACTIVE, can_save ? FALSE : TRUE, 0);
		}

		mi = (Menu_item) xv_find((Menu)ap->file_menu, MENUITEM,
		    XV_AUTO_CREATE, FALSE,
		    MENU_STRING, MGET("Save As..."), NULL);
		if (mi) {
			xv_set(mi, MENU_INACTIVE, can_saveas ? FALSE : TRUE, 0);
		}

		mi = (Menu_item) xv_find((Menu)ap->file_menu, MENUITEM,
		    XV_AUTO_CREATE, FALSE, MENU_STRING, MGET("Load..."), NULL);
		if (mi) {
			xv_set(mi, MENU_INACTIVE, can_load ? FALSE : TRUE, 0);
		}
	}
}

void
AudPanel_SETEDITMENU(
	ptr_t			sp, 
	int			has_sel,
	int			can_paste, 
	int			can_undo, 
	int			can_redo)
{
	struct atool_panel_data	*ap;
	Menu_item		mi;

	ap = AudPanel_KEYDATA((ptr_t)sp);

	if (ap->edit_menu) {
		/* first set save item */
		mi = (Menu_item) xv_find((Menu)ap->edit_menu, MENUITEM,
					 XV_AUTO_CREATE, FALSE,
					 MENU_STRING, MGET("Cut"),
					 NULL);
		if (mi) {
			xv_set(mi, MENU_INACTIVE, has_sel ? FALSE : TRUE, 0);
		}

		mi = (Menu_item) xv_find((Menu)ap->edit_menu, MENUITEM,
					 XV_AUTO_CREATE, FALSE,
					 MENU_STRING, MGET("Copy"),
					 NULL);
		if (mi) {
			xv_set(mi, MENU_INACTIVE, has_sel ? FALSE : TRUE, 0);
		}

		mi = (Menu_item) xv_find((Menu)ap->edit_menu, MENUITEM,
					 XV_AUTO_CREATE, FALSE,
					 MENU_STRING, MGET("Undo"),
					 NULL);
		if (mi) {
			xv_set(mi, MENU_INACTIVE, can_undo ? FALSE : TRUE, 0);
		}
		mi = (Menu_item) xv_find((Menu)ap->edit_menu, MENUITEM,
					 XV_AUTO_CREATE, FALSE,
					 MENU_STRING, MGET("Redo"),
					 NULL);
		if (mi) {
			xv_set(mi, MENU_INACTIVE, can_redo ? FALSE : TRUE, 0);
		}
		mi = (Menu_item) xv_find((Menu)ap->edit_menu, MENUITEM,
					 XV_AUTO_CREATE, FALSE,
					 MENU_STRING, MGET("Paste"),
					 NULL);
		if (mi) {
			/*
			 * XXX - can always try to paste since it's a real pain
			 * to figure out if the clipboard has audio data
			 */
			xv_set(mi, MENU_INACTIVE,
			    /* !can_paste ? TRUE : */ FALSE, NULL);
		}
	}
}

/*
 * Menu handler for `File_menu (Save as...)'.
 */
Menu_item
file_saveas_event(
	Menu_item		item,
	Menu_generate		op)
{
	struct atool_panel_data	*ap;
	
	if (op == MENU_NOTIFY) {
		ap = AudPanel_KEYDATA((ptr_t)item);
		AudPanel_BUSY(ap->panel);
		FilePanel_Show(ap->subp.file, LS_SaveAs);
		AudPanel_UNBUSY(ap->panel);
	}
	return (item);
}

/*
 * Menu handler for `File_menu (Save)'.
 */
Menu_item
file_save_event(
	Menu_item		item,
	Menu_generate		op)
{
	struct atool_panel_data	*ap;
	
	if (op == MENU_NOTIFY) {
		ap = AudPanel_KEYDATA((ptr_t)item);
		if ((ap->cpath[0] != NULL) || (ap->link_app[0] != NULL)) {
			(void) AudPanel_Savefile(ap, ap->cpath, FALSE, FALSE,
			    &ap->save_hdr);
		} else {
			AudPanel_Errormessage(ap, MGET("Specify a filename"));
			AudPanel_BUSY(ap->panel);
			FilePanel_Show(ap->subp.file, LS_Save);
			AudPanel_UNBUSY(ap->panel);
		}
	}
	return (item);
}

/*
 * Menu handler for `File_menu (Open...)'.
 */
Menu_item
file_load_event(
	Menu_item		item,
	Menu_generate		op)
{
	struct atool_panel_data	*ap;
	
	if (op == MENU_NOTIFY) {
		ap = AudPanel_KEYDATA((ptr_t)item);
		AudPanel_BUSY(ap->panel);
		FilePanel_Show(ap->subp.file, LS_Open);
		AudPanel_UNBUSY(ap->panel);
	}		
	return (item);
}

/*
 * Menu handler for `File_menu (Include...)'.
 */
Menu_item
file_insert_event(
	Menu_item		item,
	Menu_generate		op)
{
	struct atool_panel_data	*ap;
	
	if (op == MENU_NOTIFY) {
		ap = AudPanel_KEYDATA((ptr_t)item);
		AudPanel_BUSY(ap->panel);
		FilePanel_Show(ap->subp.file, LS_Include);
		AudPanel_UNBUSY(ap->panel);
	}
	return (item);
}

/*
 * Menu handler for `Edit_menu (Clear)'.
 */
Menu_item
edit_clear_event(
	Menu_item		item,
	Menu_generate		op)
{
	struct atool_panel_data	*ap;

	if (op == MENU_NOTIFY) {
		ap = AudPanel_KEYDATA((ptr_t)item);
		AudPanel_Clear(ap);
	}
	return (item);
}

/*
 * Menu handler for `Edit_menu (Select All)'.
 */
Menu_item
edit_select_all_event(
	Menu_item		item,
	Menu_generate		op)
{
	struct atool_panel_data	*ap;
	
	if (op == MENU_NOTIFY) {
		ap = AudPanel_KEYDATA((ptr_t)item);
		AudPanel_Selectall(ap);
	}
	return (item);
}

/*
 * Menu handler for `Edit_menu (Undo)'.
 */
Menu_item
edit_undo_event(
	Menu_item		item,
	Menu_generate		op)
{
	struct atool_panel_data	*ap;
	
	/* get audio panel data struct */
	ap = AudPanel_KEYDATA((ptr_t)item);

	switch (op) {
	case MENU_DISPLAY:
	  	/* need to see if undo is possible */
	  	if (AudPanel_Canundo(ap))
		    xv_set(item, MENU_INACTIVE, FALSE, NULL);
		else
		    xv_set(item, MENU_INACTIVE, TRUE, NULL);
		break;

	case MENU_NOTIFY:
		AudPanel_Undo(ap);
		break;
	case MENU_DISPLAY_DONE:
	case MENU_NOTIFY_DONE:
		break;
	}
	return (item);
}

/*
 * Menu handler for `Edit_menu (Redo)'.
 */
Menu_item
edit_redo_event(
	Menu_item		item,
	Menu_generate		op)
{
	struct atool_panel_data	*ap;
	
	ap = AudPanel_KEYDATA((ptr_t)item);
	switch (op) {
	case MENU_DISPLAY:
	  	/* see if redo is possible */
	  	if (AudPanel_Canredo(ap))
		    xv_set(item, MENU_INACTIVE, FALSE, NULL);
		else
		    xv_set(item, MENU_INACTIVE, TRUE, NULL);
		break;

	case MENU_NOTIFY:
		AudPanel_Redo(ap);
		break;
	case MENU_DISPLAY_DONE:
	case MENU_NOTIFY_DONE:
		break;
	}
	return (item);
}

/*
 * Menu handler for `Edit_Undo_Menu (Undo All)'.
 */
Menu_item
edit_undo_all_event(
	Menu_item		item,
	Menu_generate		op)
{
	struct atool_panel_data	*ap;
	
	if (op == MENU_NOTIFY) {
		ap = AudPanel_KEYDATA((ptr_t)item);
		AudPanel_Undoall(ap);
	}
	return (item);
}

/*
 * Menu handler for `Edit_Redo_Menu (Redo All)'.
 */
Menu_item
edit_redo_all_event(
	Menu_item		item,
	Menu_generate		op)
{
	struct atool_panel_data	*ap;
	
	if (op == MENU_NOTIFY) {
		ap = AudPanel_KEYDATA((ptr_t)item);
		AudPanel_Redoall(ap);
	}
	return (item);
}

/*
 * Menu handler for `Edit_menu (Copy)'.
 */
Menu_item
edit_copy_event(
	Menu_item		item,
	Menu_generate		op)
{
	struct atool_panel_data	*ap;
	
	if (op == MENU_NOTIFY) {
		ap = AudPanel_KEYDATA((ptr_t)item);
		AudPanel_Copy(ap);
	}
	return (item);
}

/*
 * Menu handler for `Edit_menu (Paste)'.
 */
Menu_item
edit_paste_event(
	Menu_item		item,
	Menu_generate		op)
{
	struct atool_panel_data	*ap;
	
	if (op == MENU_NOTIFY) {
		ap = AudPanel_KEYDATA((ptr_t)item);
		AudPanel_Paste(ap);
	}
	return (item);
}


#ifdef notdef
/*
 * Menu handler for `Edit_menu (Again)'.
 */
Menu_item
edit_again_event(
	Menu_item		item,
	Menu_generate		op)
{
	struct atool_panel_data	*ap;
	
	if (op == MENU_NOTIFY) {
		ap = AudPanel_KEYDATA((ptr_t)item);
		AudPanel_Again(ap);
	}
	return (item);
}
#endif

/*
 * Menu handler for `Edit_menu (Cut)'.
 */
Menu_item
edit_cut_event(
	Menu_item		item,
	Menu_generate		op)
{
	struct atool_panel_data	*ap;
	
	if (op == MENU_NOTIFY) {
		ap = AudPanel_KEYDATA((ptr_t)item);
		AudPanel_Cut(ap);
	}
	return (item);
}

/*
 * Menu handler for `Edit_Delete_menu (Delete Selection)'.
 */
Menu_item
edit_delete_event(
	Menu_item		item,
	Menu_generate		op)
{
	struct atool_panel_data	*ap;
	
	if (op == MENU_NOTIFY) {
		ap = AudPanel_KEYDATA((ptr_t)item);
		AudPanel_Delete(ap);
	}
	return (item);
}

/*
 * Menu handler for `Edit_Delete_menu (Cut Unselected)'.
 */
Menu_item
edit_delete_unselect_event(
	Menu_item		item,
	Menu_generate		op)
{
	struct atool_panel_data	*ap;
	
	if (op == MENU_NOTIFY) {
		ap = AudPanel_KEYDATA((ptr_t)item);
		AudPanel_Deleteunselected(ap);
	}
	return (item);
}

/*
 * Menu handler for `Edit_Delete_menu (Cut All Silence)'.
 */
Menu_item
edit_delete_all_silence_event(
	Menu_item		item,
	Menu_generate		op)
{
	struct atool_panel_data	*ap;
	
	if (op == MENU_NOTIFY) {
		ap = AudPanel_KEYDATA((ptr_t)item);
		AudPanel_Deleteallsilence(ap);
	}
	return (item);
}

/*
 * Menu handler for `Edit_Delete_menu (Cut Silent Ends)'.
 */
Menu_item
edit_delete_silent_ends_event(
	Menu_item		item,
	Menu_generate		op)
{
	struct atool_panel_data	*ap;
	
	if (op == MENU_NOTIFY) {
		ap = AudPanel_KEYDATA((ptr_t)item);
		AudPanel_Deletesilentends(ap);
	}
	return (item);
}

/*
 * Menu handler for `Edit_menu (Properties...)'.
 */
Menu_item
edit_menu_props_select(
	Menu_item		item,
	Menu_generate		op)
{
	struct atool_panel_data	*ap;
	
	if (op == MENU_NOTIFY) {
		ap = AudPanel_KEYDATA((ptr_t)item);
		ConfigPanel_Show(ap->subp.config);
	}
	return (item);
}

/*
 * Menu handler for `Volume_menu (Play...)'.
 */
Menu_item
volume_play_select(
	Menu_item		item,
	Menu_generate		op)
{
	struct atool_panel_data	*ap;
	
	if (op == MENU_NOTIFY) {
		ap = AudPanel_KEYDATA((ptr_t)item);
#ifdef PRE_493
		PlayctlPanel_Show(ap->subp.playctl);
#else
		/* start audio control panel via tooltalk */
		ttstart_play(ap, ap->devname,
		    MGET("Audio Tool: Play Volume"));
#endif
	}
	return (item);
}

/*
 * Menu handler for `Volume_menu (Record...)'.
 */
Menu_item
volume_record_select(
	Menu_item		item,
	Menu_generate		op)
{
	struct atool_panel_data	*ap;
	
	if (op == MENU_NOTIFY) {
		ap = AudPanel_KEYDATA((ptr_t)item);
#ifdef PRE_493
		ReclevelPanel_Show(ap->subp.reclevel);
#else
		/* start audio control panel via tooltalk */
		ttstart_record(ap, ap->devname, 
		    MGET("Audio Tool: Record Volume"));
#endif
	}
	return (item);
}

/*
 * Event callback function for `Bframe'.
 */
Notify_value
bframe_event(
	Xv_window		win,
	Event			*event,
	Notify_arg		arg,
	Notify_event_type	type)
{
	struct atool_panel_data	*ap;
	atool_Bframe_objects	*ip;
	int			i;
	int			hdelta;		/* height delta */
	int			wdelta;		/* width delta */
	
	ip = (atool_Bframe_objects *) xv_get(win, XV_KEY_DATA, INSTANCE);
	ap = (struct atool_panel_data *) 
		xv_get(ip->Bframe, XV_KEY_DATA, AUD_TOOL_KEY);
	
	switch (event_action(event)) {
	case WIN_RESIZE:
#ifdef notdef
		/* 
		 * XXX - this was a "performance" hack suggested by the 
		 * deskset folks that has the side effect of causing resize
		 * problems with the level meters (may be due to new xview
		 * bug?)....
		 */
		/* ignore if it's a a synthetic resize event */
		if (event_xevent(event)->xconfigure.send_event)
			break;
#endif

		/* calculate the size changes and save new sizes */
		i = xv_get(ip->Bframe, XV_WIDTH);
		wdelta = i - ap->frame_width;
		ap->frame_width = i;
		i = xv_get(ip->Bframe, XV_HEIGHT);
		hdelta = i - ap->frame_height;
		ap->frame_height = i;

		/* move right panel out, move bottom panel down and extend it */
		xv_set(ip->Level_control_panel,
		    XV_X, ap->right_panel_x + wdelta, NULL);
		xv_set(ip->VuMeter,
		    PANEL_METER_HEIGHT,
		    xv_get(ip->Level_control_panel, XV_HEIGHT) -
		    (2 * xv_get(ip->VuMeter, XV_Y)), NULL);

		xv_set(ip->Audio_tool_controls,
		    XV_Y, ap->lower_panel_y + hdelta,
		    XV_WIDTH, ap->lower_panel_width + wdelta, NULL);

		SegCanvas_Resize(ap->subp.segment, hdelta, wdelta);

		/* move time indicators and the drop target */
		xv_set(ip->Times,
		    XV_X, xv_get(ip->Times, XV_X) + wdelta, NULL);
		xv_set(ip->drop_target,
		    XV_X, xv_get(ip->drop_target, XV_X) + wdelta, NULL);

		/* recalculate the canvas drop region */
		Audio_INITDROPREGION(ap->panel);

		/* save new positions and dimension */
		ap->lower_panel_width =
		    xv_get(ip->Audio_tool_controls, XV_WIDTH);
		ap->lower_panel_y =
		    xv_get(ip->Audio_tool_controls, XV_Y);
		ap->right_panel_x =
		    xv_get(ip->Level_control_panel, XV_X);
		break;

        case ACTION_STOP:
		AudPanel_Stop(ap); /* XXX - stop DnD if in progress */
		break;
	}
	return (notify_next_event_func(win, (Notify_event) event, arg, type));
}

/*
 * Event callback function for `Display_canvas'.
 */
Notify_value
display_canvas_event(
	Xv_window		win,
	Event			*event,
	Notify_arg		arg,
	Notify_event_type	type)
{
	return (notify_next_event_func(win, (Notify_event) event, arg, type));
}

/*
 * Repaint callback function for `Display_canvas'.
 */
void
repaint_display(
	Canvas			canvas,
	Xv_window		paint_window,
	Display			*display,
	Window			xid,
	Xv_xrectlist		*rects)
{
	/* XXX - Redraw canvas */
}

/*
 * Notify callback function for `Rev_button'.
 */
void
rev_button_notify(
	Panel_item		item,
	Event			*event)
{
	struct atool_panel_data	*ap;
	struct Button_state	*bp;

	/* get button and audio panel data */
	bp = Button_data(item);
	ap = AudPanel_KEYDATA((ptr_t)item);

	/* button was held down, action done in *_held proc */
	if (Button_helddown(bp))
		return;

	/* move play pointer backwards to next marker */
	AudPanel_Findmarker(ap, FALSE);
}

/*
 * Notify callback function for `Rec_button'.
 */
void
Rec_button_notify(
	Panel_item		item,
	Event			*event)
{
	struct atool_panel_data	*ap;
	
	/* get audio panel data struct */
	ap = AudPanel_KEYDATA((ptr_t)item);

	/* start recording or stop if already recording */
	AudPanel_Record(ap);
}


/*
 * Notify callback function for `Play_button'.
 */
void
play_button_notify(
	Panel_item		item,
	Event			*event)
{
	struct atool_panel_data	*ap;
	
	/* get audio panel data struct */
	ap = AudPanel_KEYDATA((ptr_t)item);

	if (ap->tstate == Playing) {
		/* stop play, switch buttons back */
		AudPanel_Stop(ap);
	} else {
		AudPanel_Showtypeinfo(ap);

		/* start play, switch play button to stop button */
		AudPanel_Play(ap);
	}
}

/*
 * Notify callback function for `Fwd_button'.
 */
void
fwd_button_notify(
	Panel_item		item,
	Event			*event)
{
	struct atool_panel_data	*ap;
	struct Button_state	*bp;
	
	/* get button and audio panel data */
	bp = Button_data(item);
	ap = AudPanel_KEYDATA((ptr_t)item);

	/* button was held down, action done in *_held proc */
	if (Button_helddown(bp))
		return;

	/* move play pointer forward to next marker */
	AudPanel_Findmarker(ap, TRUE);
}


/*
 * Notify callback function for `Done_button'.
 */
void
done_button_notify(
	Panel_item		item,
	Event			*event)
{
	struct atool_panel_data	*ap;
	
	/* get audio panel data struct */
	ap = AudPanel_KEYDATA((ptr_t)item);

	/*
	 * Check if there's data to send back.
	 * If not, just break link and unload.
	 */
	if (!ap->modified) {
		AudPanel_Stop(ap);
		AudPanel_Finishsaveback(ap, TRUE);
	} else {
		/* this does the INSERT_SELECTION for DnD/SelSvc/ToolTalk */
		if (AudPanel_Savefile(ap, ap->cpath, FALSE, TRUE,
		    &ap->save_hdr)) {
			/* make sure button stay busy 'till xfer is complete */
			xv_set(item, PANEL_BUSY, TRUE, NULL);
		}
	}
}

/*
 * Menu handler for `Edit_menu (---------)'.
 */
Menu_item
menu_null_event(
	Menu_item		item,
	Menu_generate		op)
{
	/* for blank menu items */
	return (item);
}


/*
 * Menu handler for `Canvas_menu (Reset)'.
 *
 * stop play, reset ptr to beginning, clear selection.
 */
Menu_item
reset_menu_notify(
	Menu_item		item,
	Menu_generate		op)
{
	struct atool_panel_data	*ap;
	
	if (op == MENU_NOTIFY) {
		ap = AudPanel_KEYDATA((ptr_t)item);
		AudPanel_Reset(ap);
	}
	return (item);
}

/* snagged from xview code.... */
scrunch_args(
	int		*argcp,
	char		**argv,
	char		**remove,
	int		n)
{	    
	*argcp = *argcp - n;
	memmove((char *) (remove), (char *) (remove + n),
	      sizeof (*remove) * (*argcp - (remove - argv) + 1));
}

atool_xv_version(
	char		*progname)
{
	printf(MGET("%s: version %s.%s (%s)\n"),
	       progname, ATOOL_VERSION, PATCHLEVEL, XV_VERSION_STRING);
}

/*
 * Events destined for segment canavs. we get the first crack.
 */
Notify_value
atool_canvas_event(
	Xv_window		win,
	Event			*event,
	Notify_arg		arg,
	Notify_event_type	type)
{
	struct atool_panel_data	*ap;
	atool_Bframe_objects	*ip; 

	ip = (atool_Bframe_objects *) xv_get(win, XV_KEY_DATA, INSTANCE);
	ap = (struct atool_panel_data *) 
		xv_get(ip->Bframe, XV_KEY_DATA, AUD_TOOL_KEY);

	switch (event_action(event)) {
	case ACTION_DRAG_PREVIEW:
		/* Drag entered over segment canvas */
		Audio_DRAG_PREVIEW(ip->Display_canvas);
		break;

        case ACTION_DRAG_COPY:
        case ACTION_DRAG_MOVE:
		/* Drop on segment canvas */
		/* XXX - for now, only perform copies */
		event->action = ACTION_DRAG_COPY;
		Audio_GETDROP(ip->Display_canvas, event);
		break;
	case ACTION_MENU:
		if (event_is_down(event)) {
			Menu menu = (Menu) xv_get(win, WIN_MENU);

			if (menu) {
				menu_show(menu, win, event, 0);
			}
		}
		break;
	case ACTION_AGAIN:
		if (event_is_up(event)) {
			AudPanel_Again(ap);
		}
		break;
	case ACTION_STOP:
		AudPanel_Stop(ap);
		break;
	case ACTION_UNDO:
		if (event_is_up(event)) {
			AudPanel_Undo(ap);
		}
		break;
	case ACTION_PASTE:
		if (event_is_up(event)) {
			AudPanel_Paste(ap);
		}
		break;
	case ACTION_COPY:
		if (event_is_up(event)) {
			AudPanel_Copy(ap);
		}
		break;
	case ACTION_CUT:
		if (event_is_up(event)) {
			AudPanel_Cut(ap);
		}
		break;
	case ACTION_PROPS:
		if (event_is_up(event)) {
			ConfigPanel_Show(ap->subp.config);
		}
		break;
	case LOC_WINENTER:
		if (ap->tstate != Unloaded && !ap->empty) {
			(void) AudPanel_CURSORLABEL(ap, ap->Cursor_string);
		}
		break;
	case LOC_WINEXIT:
		/* blank out time cursor display */
		AudPanel_CURSORLABEL(ap, "");
		AudPanel_CURSORTIME(ap, "");
		break;
	default:
		break;		/* pass it up ... */
	}
	return (notify_next_event_func(win, (Notify_event) event, arg, type));
}

/*
 * Callback for the forward button letgo
 */
void
FwdButton_letgo(
	void*			bp)
{
	struct atool_panel_data	*ap;

	ap = AudPanel_KEYDATA((ptr_t)Button_xvid(bp));

	if (ap->laststate == Stopped)
		AudPanel_Stop(ap);

	/* change speed */
	AudPanel_Playspeed(ap, NORMAL_SPEED);
}

/*
 * Callback for the forward button being held down
 */
void
FwdButton_held(
	void*			bp)
{
	struct itimerval	timer;
	struct atool_panel_data	*ap;

	ap = AudPanel_KEYDATA((ptr_t)Button_xvid(bp));

	/* change speed */
	AudPanel_Playspeed(ap, FASTFWD_SPEED);

	/* switch to play mode */
	if ((ap->laststate = ap->tstate) == Stopped)
		AudPanel_Play(ap);
}


/*
 * Callback for the reverse button being let go
 */
void
RevButton_letgo(
	void*			bp)
{
	struct atool_panel_data	*ap;

	ap = AudPanel_KEYDATA((ptr_t)Button_xvid(bp));

	if (ap->laststate == Stopped)
		AudPanel_Stop(ap);

	/* change speed */
	AudPanel_Playspeed(ap, NORMAL_SPEED);
}

/*
 * Callback for the reverse button being held down
 */
void
RevButton_held(
	void*			bp)
{
	struct itimerval	timer;
	struct atool_panel_data	*ap;
	double			end;	/* end of play range */

	ap = AudPanel_KEYDATA((ptr_t)Button_xvid(bp));

	/* change speed */
	AudPanel_Playspeed(ap, FASTREV_SPEED);

	/* change tool to play mode */
	if ((ap->laststate = ap->tstate) == Stopped) {
		/* get end of play range */
		end = (SegCanvas_Selection(ap->subp.segment)) ?
		    ap->end : list_getlength(ap->lp);

		if (ap->insert == 0.0) {
			/* there's no where to reverse to */
			return;
		}
		AudPanel_Play(ap);
	}
}

/* usage message */
void
atool_usage_proc()
{
	fprintf(stderr, MGET("\tOpenWindows DeskSet Audio Tool\n"
	    "usage:\n"
	    "\t%s [-p] [-d dev] [generic-tool-arguments] [file ...]\n"
	    "where:\n"
	    "\t-p\tPlay files loaded on command line\n"
	    "\t-d dev\tSpecify audio device (default: /dev/audio)\n"
	    "\tfile\tLoad the specified file(s) at startup\n"
	    "\t\tIf more than one file is specified on the command line,\n"
	    "\t\tall files will be concatenated together and treated as\n"
	    "\t\ta single new (Untitled) audio file\n"
	    "\n"),
	    Progname);

	/* Since this is called from xview, issue general tool help */
	xv_usage(Progname);
	exit(0);
}

/* 
 * This is what gets called when an item from the File->New menu is selected.
 * Unload the current file and set the record header to the selected format.
 */
int
New_format_proc(
	ptr_t			dp,
	Audio_hdr		*hdr)
{
	struct atool_panel_data	*ap;
	char			buf[BUFSIZ];
	char			*label;

	ap = (struct atool_panel_data*)dp;
	if (AudPanel_Canplay(ap, hdr) == FALSE) {
		AudPanel_Alert(ap, MGET(
		    "The selected audio format is incompatible "
		    "with the audio device.\n"
		    "This operation will be cancelled."));
		return (FALSE);
	}

	/* Don't break link */
	if (AudPanel_Unloadfile(ap, FALSE, hdr) != TRUE) {
		return (FALSE);
	}
	return (TRUE);
}

Menu
gen_format_menu(
	Menu_item		item,
	Menu_generate		op)
{
	struct atool_panel_data	*ap;
	Menu			menu;

	menu = (Menu) xv_get(item, MENU_PULLRIGHT);
	if (op == MENU_DISPLAY) {
		ap = AudPanel_KEYDATA((ptr_t)item);
		menu = do_create_format_menu(menu,
		    AudPanel_FormatPanel_hndl(ap),
		    New_format_proc,
		    AudPanel_Canplay,
		    &(ap->rec_hdr),
		    ap,
		    MGET("New Format"));
	}
	return (menu);
}
