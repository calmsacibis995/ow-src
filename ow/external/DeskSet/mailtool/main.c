#ifndef lint
static  char sccsid[] = "@(#)main.c 3.18 94/02/09 Copyr 1985 Sun Micro";
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

static char ident[] =
"@(#)Copyright (c) 1987, 1988, Sun Microsystems, Inc.  All Rights Reserved.\n\
Sun considers its source code as an unpublished, proprietary\n\
trade secret, and it is available only under strict license\n\
provisions.  This copyright notice is placed here only to protect\n\
Sun in the event the source is deemed a published work.  Dissassembly,\n\
decompilation, or other means of reducing the object code to human\n\
readable form is prohibited by the license agreement under which\n\
this code is provided to the user or company in possession of this\n\
copy.\n\
\n\
RESTRICTED RIGHTS LEGEND: Use, duplication, or disclosure by the \n\
Government is subject to restrictions as set forth in subparagraph \n\
(c)(1)(ii) of the Rights in Technical Data and Computer Software \n\
clause at DFARS 52.227-7013 and in similar clauses in the FAR and \n\
NASA FAR Supplement. \n";

/*
 * Mailtool
 */

#include <stdio.h>
#ifdef SVR4
#include <unistd.h>
#endif SVR4
#include <sys/syscall.h>
#include <sys/file.h>
#include <sys/types.h>
#include <sys/param.h>

#include <xview/panel.h>
#include <xview/text.h>
#include <xview/font.h>
#include <xview/xview.h>

#include "mle.h"
#include "glob.h"
#include "tool.h"
#include "tool_support.h"
#include "mail.h"
#include "attach.h"
#include "main.h"
#include "instrument.h"
#include "delete_log.h"
#include "../maillib/ck_strings.h"


char    *mktemp(), *strcpy();

int	mt_aborting;		/* aborting, don't save messages */
int	mt_usersetsize;		/* user set tool size via command line args */
char	*mt_cmdname;		/* our name */
int	mt_no_classing_engine;

int	mt_tt_flag = 0;

char	*mt_cmdline_args;
char	mt_vacationfile[1024];

char	*name_none;
char	*name_mailtool;
char	*name_Mail_Tool;

static char mt_compiletime[128] = "";
static gid_t original_egid;

static 	void usage(char *name);
static  int	mt_x_error_proc();

extern	void	nomem();
extern	char	*ds_relname();

static void
mailtool_version(char **vend, char **name, char **ver)
{
        *vend = "Sun Microsystem, Inc.";
        *name = "Mailtool";
        *ver = "3.2";
}


main(
	int	argc,
	char	**argv
)
{
	int	i;
	char	**av = argv;
	char	*s;
	char	*display;
	char	*relname;
	char	*ds_hostname(), *hostname;
	Xv_Server	server;



	/* fix for 1094191:
	 *
	 * we have to be set-gid to group "mail" when opening and storing
	 * folders.  But we don't want to do everything as group mail.
	 * here we record our original gid, and set the effective gid
	 * back the the real gid.  We'll set it back when we're dealing
	 * with folders...
	 */
	original_egid = getegid();
	setgid(getgid());

	/* OK, this is a gross hack, but it needs to be done.  
	   The problem here is that if the user specifies the -W 
	   option in the command line args, the tool is supposed 
	   to (ugh) ignore the .mailrc info.  OK.  Fine.  We 
	   have to scan thru the argc/argv list to see if the 
	   parameter is set, before the call to xv_init eats the 
	   arguments out of argc/argv.  We then save whether the 
	   info was set, and then decide later (in frame 
	   creation) whether or not to set it again. */

	mt_usersetsize = FALSE;
	while (*av)
	{
		if (!strcmp(*av, "-Ws") || !strcmp(*av, "-size")) {
			mt_usersetsize = TRUE;
		} else if  (strcmp(*av, "-message_alliance") == 0) {
			/* Started by tooltalk */
			mt_tt_flag++;
		}
		av++;
	}

	/* 
	 * The ds_tooltalk_init function now rendez-vous with the
	 * the correct ttsession by checking the command line
	 * for any option that sets the Display. If none are
	 * found, then we join the ttsession on the local machine.
	 *
	 * ZZZ [dipol] The only reason we call mt_start_tt_init() 
	 * here is to support tooltalk folder locking.  Need to move
	 * that to the new protocol
	 */
	mt_start_tt_init("mailtool", FALSE, argc, argv);
	dstt_check_startup(mailtool_version, &argc, &argv);

	/* connect to window server */
	server = xv_init(XV_INIT_ARGC_PTR_ARGV, &argc, argv,
		XV_USE_LOCALE, TRUE,
		XV_X_ERROR_PROC,	mt_x_error_proc,
		0);

	{
		/* set up localization stuff */
		char buffer[MAXPATHLEN];
		char *s;

		/* FIX ME... We need a path, and to check for buffer
		 * overflow.
		 */
		ds_expand_pathname("$OPENWINHOME/lib/locale", buffer);
		bindtextdomain("SUNW_DESKSET_MAILTOOL", buffer);
		textdomain("SUNW_DESKSET_MAILTOOL");
	}


        /* STRING_EXTRACTION -
         * 
         * These four place holder strings are translated once but used
         * Throughout the code.  "Mail Tool" is a nice, pretty title used
         * for the program in the header pane.  "mailtool" is used when
         * we wish to be more terse.  "[None]" is used as the name of
         * the folder to display when we do not have a current folder.
         */
	/* we now make a composite name for the tool, combining
	 * "Mail Tool" with a release identifier
	 */
	relname = ds_relname();
	hostname = ds_hostname(xv_get(server, XV_DISPLAY));
	i = strlen(hostname);
	if (*hostname != '\0' && hostname[i - 1] == ' ')
		hostname[i - 1] = '\0';	/* Strip trailing space */

        s = gettext("Mail Tool");
	name_Mail_Tool = ck_malloc(strlen(relname) + i + strlen(s) + 2);

	sprintf(name_Mail_Tool, "%s %s%s", s, relname, hostname);

        name_mailtool = gettext("mailtool");
        name_none = gettext("[None]");

	mt_init_tool_storage();

	mt_cmdname = argv[0];
	argc--;
	argv++;

	/* Initialize the classing engine */
	if (mt_init_ce() < 0) {
		mt_no_classing_engine = 1;
	}

	/* Specify the customerized error handling functions, and
	 * encoding/decoding mapping functions for maillib.
	 */
	maillib_methods.ml_set(ML_WARN, (void *) mt_vs_warn);
	maillib_methods.ml_set(ML_ERROR, (void *) mt_vs_exit);
	maillib_methods.ml_set(ML_CONFIRM, (void *) mt_vs_confirm);
	maillib_methods.ml_set(ML_ENCODE, (void *) mt_get_encode);
	maillib_methods.ml_set(ML_DECODE, (void *) mt_get_decode);

	/*
	 * Determine user's mailbox.
	 */
	init_globals();

	/* 
	 * ZZZ: mt_init_mailtool_defaults() is also called by
	 * mt_finish_initializing(), which is called by mt_start_tool().
	 * Since we need to initialize some variables from .mailrc before
	 * then, we call it here first.  This results in the .mailrc being
	 * read in twice and should be fixed.
	 */
	mt_init_mailtool_defaults();

#ifdef DEBUG
	/* mt_init_mailtool_defaults() sets the value of MAIL */
	if (!mt_value("MAIL")) {
		fprintf(stderr, "MAIL is not set in main()\n");
		exit(1);
	}
#endif DEBUG

	mt_mailbox = ck_strdup(mt_value("MAIL"));

	/* Make sure DISPLAY is set in our environment so all children pick
	 * it up.
	 */
	display = DisplayString((Display *)xv_get(server, XV_DISPLAY));
	s = (char *)malloc(strlen(display) + 10);
	if (s != NULL) {
		strcpy(s, "DISPLAY=");
		strcat(s, display);
		putenv(s);
	}

	TRACK_STARTUP();

	mt_start_tool(argc, argv);
	mt_done(0);

	/* Quit tooltalk */
	mt_quit_tt();

	exit(0);
}

void
mt_parse_tool_args(
	int	argc,
	char	**argv
)
{
	char	*iv = NULL;
	int	expert = 0;
	char	small_buf[32];
	int	size;
	int	i;
	char	*args;
	extern	char *xv_version;
	int	advance;

	/* figure out how large a cmd buffer we may need */
        /*
         * Start off at 2, extra space for blank at end,
         * and NULL .
         */

	for(i = 0, size = 2; i < argc; i++) {
		if(argv[i]) {
			size += strlen(argv[i]);
		}
                /* Account for space in front */
		size++;
	}

	mt_cmdline_args = ck_malloc(size);
	*mt_cmdline_args = '\0';
	args = mt_cmdline_args;

	while (argc > 0) {
		if (argv[0][0] != '-') {
			usage(mt_cmdname);
		}

		advance = 1;

		switch (argv[0][1]) {
		case 'M':
			/* all mailtool switches will begin with -M */
			if (argv[0][3] != '\0')
				goto error;
			switch (argv[0][2]) {
			case 'x':
				expert++;
				break;

			case 'i':
				if (argc < 2)
					goto error;
				iv = argv[1];

				advance = 2;
				break;
			case 'f':
				if (argc < 2)
					goto error;

				advance = 2;
				mt_load_from_folder = argv[1];
				break;
			case 'd':
				mt_debugging = TRUE;
				break;
			default:
				goto error;
			}
			break;
		case 'x':	/* for backwards compatibility */
			if (argv[0][2] != '\0')
				goto error;
			expert++;
			break;
		case 'i':	/* for backwards compatibility */
			if (argv[0][2] != '\0')
				goto error;
			else if (argc < 2)
				goto error;
			iv = argv[1];
			argc--;
			argv++;
			break;
		case 'v':
			fprintf(stderr, "%s version %s (%s) running on %s\n",
				mt_cmdname, ds_relname(), mt_compiletime,
				xv_version);
			exit(0);
		default:
			/*
			 * Started by tooltalk.  Already handled before
			 * xv_init()
			 */
			if (strcmp(argv[0], "-message_alliance") == 0)
				break;
		error:
			usage(mt_cmdname);
		}

		while(advance--) {
                        sprintf(args, " %s", argv[0]);
			args += strlen(args);
			argc--;
			argv++;
		}
	}

        *args++ = ' ';
        *args = '\0';

	if (iv)
		mt_assign("retrieveinterval", iv);
	if (expert)
		mt_assign("expert", "");

}

static void
usage(
	char	*name
)
{
	/* STRING_EXTRACTION -
	 * 
	 * If the user passed mailtool an option it doesn't know
	 * about, mailtool prints out this usage line.
	 */
	(void)fprintf(stderr,
	gettext("Usage: %s [-Mx] [-Mi interval] [-Mf mailfile] [generic-tool-arguments] [-v]\n"), name);
	exit(1);
}


void
mt_done(
	int i
)
{
	/*
	 * Destroy compose windows was already done by the notify proc 
	 * mt_destroy, so no need to do it again here
	 */

	/*
	 * Remove all tmp files 
	 */
	mt_cleanup_tmpfiles();

	TRACK_EXIT(i);
	
	exit(i);
}

void
mt_cleanup_tmpfiles(void)


{
	(void) unlink(mt_vacationfile);

	/* Controlled termination.  Clear up log */
	(void) mt_clear_transaction_log();
}


/*
 * Decide if an X error is serious enough to exit.
 *
 * Returns 1 if error is fatal and we should exit.
 * Otherwise returns 0
 */
static int
mt_fatal_error(
	XErrorEvent *error
)
{
	switch(error->error_code) {

	case Success:

	/* Frequently generate by DnD when target app vanishes */
	case BadWindow:
	case BadAtom:
	case BadDrawable:
		return 0;	/* Non fatal error */
	default:
		return 1;	/* Fatal error */
	}
}


/*
 * X error handler
 */
static int
mt_x_error_proc(
	Display *display,
	XErrorEvent *error
)
{
	char msg[256];

	/* There are two types of error handlers in Xlib; one to handle fatal
	 * I/O errors, and one to handle error events from the event server.
	 * This function handles the latter, non-fatal errors.  
	 *
	 * If we consider one of these event errors fatal then we
	 * let the toolkit print out the error message and exit.
	 * For the relatively common errors (like BadWindow) we
	 * print out a message and continue.
	 *
	 * Note: you cannot directly or indirectly perform any operations
	 * on the server while in this error handler so we must simply
	 * print to stderr.
	 */
	if (mt_fatal_error(error)) {
		/*
	 	 * Returning XV_ERROR causes xview to call the default
		 * error proc and exit
	 	 */
		return XV_ERROR;
	} else {

        	/* STRING_EXTRACTION -
         	 * 
		 * When mailtool encounters a non-fatal X error it decides
		 * whether it should print out the error and keep on going
		 * or let the toolkit print out the error and exit. This
		 * is the message mailtool prints on stderr if it decides
		 * to continue execution.  The majority of it is technical
		 * X stuff.
	 	 */
		XGetErrorText(display, error->error_code, msg, 256);
  		(void)fprintf(stderr, gettext("\nMail Tool: X Error (intercepted): %s\nMajor Request Code   : %d\nMinor Request Code   : %d\nResource ID (XID)    : %u\nError Serial Number  : %u\n") , msg, error->request_code,
        error->minor_code, error->resourceid, error->serial);
  		(void)fprintf(stderr,
				gettext("\nMail Tool: Ignoring X Error\n"));

		/* Returning XV_OK causes XView to continue execution */
		return XV_OK;
	}
}




gid_t
mt_getegid(
	void
)
{
	return (original_egid);
}
