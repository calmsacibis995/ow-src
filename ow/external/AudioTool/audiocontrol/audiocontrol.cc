/* Copyright (c) 1993 by Sun Microsystems, Inc. */
#ident	"@(#)audiocontrol.cc	1.27	96/02/20 SMI"

#include <stdio.h>
#include <stdlib.h>
#include <sys/param.h>
#include <xview/xview.h>

#include "audiocontrol.h"
#include "playctl/playctl.h"
#include "recordctl/recordctl.h"
#include "status/status.h"


#ifndef PRE_493
#include <dstt_audio.h>

extern "C" {
extern char*	ds_vendname();
extern char*	ds_relname();
}
extern int	audiocontrol_dstt_start(caddr_t);
extern void	print_tt_emsg(char *, Tt_status);
#endif /* !PRE_493 */

extern "C" {
extern void	xv_usage(char*);
}

char*		I18N_Message_File = I18N_DOMAIN;

#define TT_SLEEP_TIME (30 * 60)	// amount of time to sleep before quitting

// getopt globals
extern int		optind;
extern char*		optarg;

static char*		prog;
static char*		prog_opts =
#ifndef DEBUG
			    "d:";
#else /* DEBUG */
			    "DRd:";
#endif /* DEBUG */


// a struct to hold all the objects/state we're interested in
class AudioControlObjects {
public:
	Playctl*	playpanel;
	Recordctl*	recpanel;
	Statusctl*	statuspanel;

	caddr_t		mainpanel;
	int		exitpending;
	int		ttflag;
	int		debug;
	int		sleeping;

	AudioControlObjects();
	~AudioControlObjects() {};

};

// "global" pointer to all of our interesting state/objects
AudioControlObjects*	ACOP = NULL;


void
usage()
{
	fprintf(stderr, MGET("\tOpenWindows DeskSet Audio Control Panel\n"
	    "usage:\n"
	    "\t%s [-d dev] [generic-tool-arguments]\n"
	    "where:\n"
	    "\t-d dev\tSpecify audio device (default: /dev/audio)\n"),
	    prog);
}

void
audiocontrol_usage()
{
	/* Since this is called from xview, issue general tool help */
	usage();
	fprintf(stderr, "\n");
	xv_usage(prog);
	exit(0);
}

#ifndef PRE_493
extern "C" {
// used as a callback for dstt stuff.
void
audiocontrol_version(char **vend, char **name, char **ver)
{
        *vend = (char *)ds_vendname();
        *name = "Audio Control";
        *ver = (char *)ds_relname();
}
};

static void
scrunch_args(int *argcp, char **argv, char **remove, int n)
{	    
	*argcp = *argcp - n;
	memmove((char *) (remove), (char *) (remove + n),
	      sizeof(*remove) * (*argcp - (remove - argv) + 1));
}
#endif /* !PRE_493 */

static void
handle_quit_timer()
{
	if (ACOP->debug)
		fprintf(stderr, "Audio Control: timer expired, quitting.\n");
	exit(0);
}


// Certain 'exit' events unmap all frames and set a 30 minute timer before
// actually exiting
int
Audioctl_set_quit_timer(
	int			flag)
{
	static struct itimerval	itv;
	int was_sleeping;

	was_sleeping = ACOP->sleeping;

	ACOP->sleeping = flag;

	if (ACOP->debug) {
		fprintf(stderr, "Audio Control: set_quit_timer: %s\n",
		    flag ? "TRUE" : "FALSE");
	}

	// sleep for 30 minutes instead of just quitting
	itv.it_value.tv_sec = flag ? TT_SLEEP_TIME : 0;
	itv.it_interval.tv_sec = itv.it_value.tv_sec;
	itv.it_value.tv_usec = 0;
	itv.it_interval.tv_usec = itv.it_value.tv_usec;

	notify_set_itimer_func((Xv_opaque)ACOP->playpanel->Gethandle(),
	    (flag ? (Notify_func) handle_quit_timer : NOTIFY_FUNC_NULL),
	    ITIMER_REAL, &itv, NULL);

	return (was_sleeping);
}


Notify_value
audioctl_destroy_func(
	Notify_client	client,
	Destroy_status	status)
{
	switch (status) {
	case DESTROY_CHECKING:
		// for tooltalk launched tool, just hide & sleep for a while 
		// in case we get another tooltalk launch. to make this
		// work, go in to hiding and veto the destroy request.
		if (ACOP->ttflag) {
			if (ACOP->debug) {
				fprintf(stderr, 
				    "unmapping all panels and sleeping\n");
			}
			// unmap all panels and go to sleep.
			ACOP->recpanel->Unshow();
			ACOP->statuspanel->Unshow();
			ACOP->playpanel->Unshow();

			(void) Audioctl_set_quit_timer(TRUE);
			return ((Notify_value)notify_veto_destroy(client));
		}
		// if not any of the above fall through & really exit
		if (ACOP->debug) {
			fprintf(stderr, "quit request - really exitting\n");
		}
		break;

	case DESTROY_SAVE_YOURSELF:
		// XXX - save default menu items, etc.
		return (NOTIFY_DONE);

	case DESTROY_CLEANUP:
		return (notify_next_destroy_func(client, status));

	case DESTROY_PROCESS_DEATH:
	default:
		break;
	}
	return (NOTIFY_DONE);
}

AudioControlObjects::
AudioControlObjects():
	playpanel(0), recpanel(0), statuspanel(0), mainpanel(0),
	exitpending(FALSE), ttflag(FALSE), debug(FALSE), sleeping(FALSE)
{
}

AudioControlObjects*
AudioControl_Init(
	char*		devname,
	int		ttflag,
	int		debug)
{
	int		width;
	AudioControlObjects *ap = new AudioControlObjects;

	if (!ap) {
		return (NULL);
	}

	ap->ttflag = ttflag;
	ap->debug = debug;

	// Create all the panels
	ap->playpanel = Create_Playctl(devname, NULL);
	if (ap->playpanel == NULL) {
		delete ap;
		return (NULL);
	}
	notify_interpose_destroy_func((Xv_opaque)ap->playpanel->Gethandle(), 
				      (Notify_func)audioctl_destroy_func);

	ap->recpanel = Create_Recordctl(devname, 
					ap->playpanel->Gethandle());
	ap->statuspanel = Create_Statusctl(devname, 
					   ap->playpanel->Gethandle());

	// Now do some creative resizing:
	//   Set the play panel width hint to the widest window size.
	//   Then set the width hint for the other panels to match it.
	width = ap->playpanel->Getwidth();
	if (ap->recpanel->Getwidth() > width)
		width = ap->recpanel->Getwidth();
	if (ap->statuspanel->Getwidth() > width)
		width = ap->statuspanel->Getwidth();
	ap->playpanel->Setwidth(width);
	width = ap->playpanel->Getwidth();
	ap->recpanel->Setwidth(width);
	ap->statuspanel->Setwidth(width);

	ap->mainpanel = (caddr_t) ap->playpanel->Gethandle();
	return (ap);
}

int
main(
	int		argc,
	char**		argv)
{
	int		i;
	int		noplay = FALSE;
	char		bind_home[MAXPATHLEN];
	int		ttflag = FALSE;
	int		debug = FALSE;
	char* 		devname = NULL;
	Xv_opaque	server;
#ifndef PRE_493
	char**		args;
	int		nargs;
	Tt_status	ttstatus;
#endif /* !PRE_493 */

	// Get the program name
	prog = strrchr(argv[0], '/');
	if (prog == NULL)
		prog = argv[0];
	else
		prog++;

	// Initialize i18n
	(void) ds_expand_pathname("$OPENWINHOME/lib/locale", bind_home);
	(void) bindtextdomain(I18N_Message_File, bind_home);
	textdomain(I18N_Message_File);

#ifndef PRE_493
	for (args = argv, nargs = argc;
	    nargs && args && *args;
	    nargs--) {
                // Check if we've been started for tooltalk message alliance
                if (strcmp(*args,  "-message_alliance" ) == 0) {
			scrunch_args(&argc, argv, args, 1);
                        ttflag = TRUE;
                } else {
			args++;
		}
        }
 
	if (ttstatus = dstt_check_startup(audiocontrol_version, &argc, &argv)) {
		print_tt_emsg(MGET("Could not initialize Tool Talk"), ttstatus);
		ttflag = FALSE;
	}
#endif /* !PRE_493 */

	server = xv_init(XV_INIT_ARGC_PTR_ARGV, &argc, argv,
	    XV_USAGE_PROC, audiocontrol_usage, XV_USE_LOCALE, TRUE, 0);
#ifndef PRE_493
	xv_set(server, XV_APP_NAME, MGET("Audio Control"), NULL);
#endif

	// Parse start-up options
	while ((i = getopt(argc, argv, prog_opts)) != EOF) switch (i) {
	case 'd':			// device flag
		devname = optarg;
		break;
	case 'R':			// Record panel only
		if (!ttflag)		// Ignore -R if tooltalk-initiated
			noplay = TRUE;
		break;
	case 'D':
		debug = TRUE;
		break;
	default:
		usage();
		exit(1);
	}

	if ((ACOP = AudioControl_Init(devname, ttflag, debug)) == NULL) {
		exit(1);
	}
	// XXX - ordinarily this is invoked from a ToolTalk message
	if (noplay) {
		Audioctl_map_recpanel_only();
	}

#ifndef PRE_493
	if (ttstatus == 0) {
		audiocontrol_dstt_start(ACOP->mainpanel);
	}

	if (ACOP->ttflag) {
		// make sure play panel does NOT get mapped
		xv_set((Xv_opaque)ACOP->playpanel->Gethandle(), 
		       XV_SHOW, FALSE, NULL);
		// don't map any windows of started by tooltalk!
		notify_start();
	} else
#endif /* !PRE_493 */
		xv_main_loop((Xv_opaque)ACOP->mainpanel);

	if (ACOP->debug) {
		fprintf(stderr, "xv_main_loop() exitted, quitting.\n");
	}
	fflush(stderr);
	exit(0);
}

// Called at startup if only the record panel should be mapped
// XXX - needs to be changed if called after xv_main_loop()
void
Audioctl_map_recpanel_only()
{
	ACOP->exitpending = TRUE;
	ACOP->mainpanel = (caddr_t) ACOP->recpanel->Gethandle();
}

// Open the audiocontrol panel if closed, in order to display popups
void
Audioctl_open_playpanel()
{
	if (xv_get((Xv_opaque)ACOP->playpanel->Gethandle(), FRAME_CLOSED))
		Audioctl_show_playpanel();
}

// Open the play panel if closed, or else bring it to the top
void
Audioctl_show_playpanel()
{
	ACOP->playpanel->Show();
	ACOP->exitpending = FALSE;
}

// Display the record panel if unmapped, or else bring it to the top
void
Audioctl_show_recpanel()
{
	ACOP->recpanel->Show();
}

// Return TRUE if tooltalk-started
int
Audioctl_tt_started()
{
       return (ACOP->ttflag);
}

// Called from record panel code when it is being unmapped
void
Audioctl_recpanel_exit()
{
	if (ACOP->ttflag) {
		// only go to sleep/quit if playpanel is gone. 
		if (!xv_get((Xv_opaque)ACOP->playpanel->Gethandle(), XV_SHOW)) {
			(void) Audioctl_set_quit_timer(TRUE);
		}
	} else if (ACOP->exitpending) {
		fflush(stderr);
		exit(0);
	}
}

// this is called when we get a tooltalk quit request. 
// if we're "sleeping" (all panels unmapped), then really exit. if not,
// reset the ttflag such that a quit request from the base frame
// really exits.
void
Audioctl_dstt_quit()
{
	if (ACOP->ttflag) {
		if (ACOP->debug) {
			fprintf(stderr, "Audio Control: tooltalk exit req.\n");
		}
		// reset ttflag so destroy func will really exit.
		ACOP->ttflag = FALSE;
		if (ACOP->sleeping) {
			xv_destroy_safe(
				(Xv_opaque)ACOP->playpanel->Gethandle());
		}
	} else {
		if (ACOP->debug)
			fprintf(stderr,
	    "Audio Control: ignoring tooltalk exit: (not tt started)\n");
	}
}

// Display the status panel if unmapped, or else bring it to the top
void
Audioctl_show_statuspanel()
{
#ifdef DEBUGxx
static int d = 0;
	if (ACOP && ACOP->debug) {
		d = !d;
		if (d) {
			fprintf(stderr, "switching to /dev/audioamd\n");
			Audioctl_set_device("/dev/audioamd");
		} else {
			fprintf(stderr, "switching to /dev/audio\n");
			Audioctl_set_device("/dev/audio");
		}
	}
#endif /* DEBUG */
	ACOP->statuspanel->Show();
}

// Set the device name for all panels
void
Audioctl_set_device(
	char*		devname)
{
	ACOP->playpanel->Setdevice(devname);
	ACOP->recpanel->Setdevice(devname);
	ACOP->statuspanel->Setdevice(devname);
}

// Return an opaque pointer to the play panel object
void*
Audioctl_get_playpanel()
{
	return ((void*) ACOP->playpanel);
}

// Return an opaque pointer to the record panel object
void*
Audioctl_get_recpanel()
{
	return ((void*) ACOP->recpanel);
}
