#ifndef lint
#ifdef sccs
static  char sccsid[] = "@(#)cmdtool.c 15.69 96/11/07";
#endif
#endif

/*
 * Copyright (c) 1985, 1987 by Sun Microsystems, Inc.
 */

/*
 *  cmd/shelltool - run a process in a tty subwindow
 */

#include <stdio.h>

#ifndef SVR4
#include <strings.h>
#else
#include <string.h>
#endif SVR4

#include <signal.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/param.h>
#include <xview/attr.h>
#include <xview/defaults.h>
#include <xview/xview.h>
#include <xview/scrollbar.h>
#include <xview/tty.h>
#include <xview/termsw.h>
#include <xview/textsw.h>

#define	TEXTDOMAIN	"SUNW_WST_CMDTOOL"
/*
 * Using variable to keep textdomain name reduces object size, however
 * "xgettext" command require to have textdomain name as constant.
 */
#ifdef XGETTEXT
#define	MSG(msg)	dgettext(TEXTDOMAIN, msg)
#else
static char		*my_textdomain = TEXTDOMAIN;
#define MSG(msg)        dgettext(my_textdomain, msg)
#endif

#define MIN_LOGFILE_SIZE 8096  /* min. wraparound edit log size */
#define DEFAULT_LOGFILE_SIZE 100000  /* default wraparound edit log size */

extern	char *getenv();
extern char *dgettext();
extern void ttysw_updateutmp();

static void set_textdomain();
static void putenv_locale();

static unsigned short tty_image[258] = {
#include <images/terminal.icon>
};

static unsigned short tty_mask_image[258] = {
#include <images/terminal_mask.icon>
};

static unsigned short cmd_image[258] = {
#include <images/cmdtool.icon>
};

static unsigned short cmd_mask_image[258] = {
#include <images/cmdtool_mask.icon>
};

static unsigned short console_image[258] = {
#include <images/console.icon>
};

static unsigned short console_mask_image[258] = {
#include <images/console_mask.icon>
};


print_usage(am_cmdtool, toolname)
	int	 am_cmdtool;
	char	*toolname;
{
	char	*mode_spec = (am_cmdtool)
				? MSG("[-P frequency] [-M bytes]") :
				MSG("[-B boldstyle]");
	
	(void)fprintf(stderr,
		MSG("syntax: %s [-C] [-I initial_cmd] %s [program [args]]\n"),
		toolname, mode_spec);
	if (!am_cmdtool) {
	    (void)fprintf(stderr,
		MSG("\
-B	set bold style for this instance of %s\n\
	where 'boldstyle' is a number from 1 to 8\n"), toolname);
	}
	(void)fprintf(stderr,
		MSG("-C	redirect console output to this instance of %s\n"),
		toolname);
	(void)fprintf(stderr,
		MSG("-I	'initial_cmd' is first command 'program' executes\n"));
	(void)fprintf(stderr,
		MSG("-L	run the .login file when starting the shell subprocess\n"));
	if (am_cmdtool) {
	    (void)fprintf(stderr,
		MSG("\
-P	checkpoint frequency for this %s, where 'frequency' is number\n\
	of edits between checkpoints; a value of 0 means no checkpointing.\n\
-M      set the log to wrap-around after the indicated number of bytes.\n"),
		toolname);
	}
}

Notify_value
cmdtool_signal(client, sig, when)
    Notify_client client;
    int sig;
    Notify_signal_mode when;
{
    ttysw_updateutmp();
    exit(0);
}

main(argc,argv)
	int argc;
	char **argv;
{
	int	am_cmdtool;
	Frame	base_frame;
	Tty	ttysw;
	Icon	tool_icon;
	char	*tool_name = argv[0];
	char	*shell_label;
	char	*cmd_label;
	char	*console_label;
	char	frame_label[150];
	char	icon_label[30];
        char    *tmp_label1, *tmp_label2;
	int	become_console = 0;
	int	run_login	= 0;
	char	*bold_name = 0;
	char	*sh_argv[2];
	char	*init_cmd = 0;
	int	len;
	char	*filename = (char *)strrchr(argv[0], '/');
	int	checkpoint = 0;
	int	edit_log_wraps_at = DEFAULT_LOGFILE_SIZE;
	int	tty_pid = 0;
	char	err_msg[50];
	char	cmdline2[50];
	Server_image  cmd_pixmap, cmd_mask_pixmap;

	
#ifdef GPROF
	if (argc > 1 && strcmp(argv[argc-1], "-gprof") == 0) {
	    moncontrol(1);
	    /* Pull the -gprof out of argc/v */
	    argc--;
	    argv[argc] = (char *)0;
	} else {
	    moncontrol(0);
	}
#endif
	
#ifdef DEBUG
	/* Doesn't exist any more */
	/* malloc_debug(0); */
#endif	

	setsid();
	
	/* This is required to initialize correctly */	
	xv_init(XV_INIT_ARGC_PTR_ARGV, &argc, argv, 
		XV_USE_LOCALE, TRUE, 0);

	set_textdomain();

	shell_label = MSG("shelltool");
	cmd_label = MSG("cmdtool");
	console_label = MSG(" (CONSOLE) - ");

	if (filename)
		filename++;
	else
		filename = argv[0];
	am_cmdtool = (strcmp(filename, "cmdtool") == 0);

	/*
	 *  Send the icon attr before argc, argv to give
	 *  commandline argument a chance to override.
	 *  A waste of space & time if commandline argument
	 *  is present.
	 */
	icon_label[0] = 0177;	/* del, highly unlikely as cmd arg */
	icon_label[1] = '\0';

        cmd_pixmap = (Server_image)xv_create(NULL, SERVER_IMAGE,
                   	                     XV_WIDTH,  64,
		                             XV_HEIGHT, 64,
		                             SERVER_IMAGE_BITS,      
		                             become_console ? console_image : 
                                             am_cmdtool ? cmd_image : tty_image,
		                             NULL);

        cmd_mask_pixmap = (Server_image)xv_create(NULL, SERVER_IMAGE,
		                                 XV_WIDTH,  64,
		                                 XV_HEIGHT, 64,
		                                 SERVER_IMAGE_BITS,
		                                 become_console ? console_mask_image : 
                                                 am_cmdtool ? cmd_mask_image : 
                                                              tty_mask_image,
		                                 NULL);

	tool_icon = (Icon)xv_create(NULL, ICON,
            WIN_RETAINED, TRUE,				    
            ICON_IMAGE, cmd_pixmap,
            ICON_MASK_IMAGE, cmd_mask_pixmap,
            ICON_LABEL, icon_label,
            ICON_TRANSPARENT,TRUE,
            NULL);

	argv++;
	argc--;

	base_frame = xv_create((Xv_window)NULL, FRAME,
			FRAME_ICON, tool_icon,
		        HELP_STRING_FILENAME,	"manpage_synopsis_help_index",
        		FRAME_WM_COMMAND_ARGC_ARGV, argc, argv,
			0);
			
	if (base_frame == NULL) {
	    fprintf(stderr, MSG("Cannot create base frame.  Process aborted.\n"));
	    exit(1);
	}		

	/* Get ttysw related args */
	sh_argv[0] = NULL;
	sh_argv[1] = NULL;

	if (am_cmdtool) {
	    checkpoint =
		defaults_get_integer_check("cmdtool.checkpointFrequency",
			"Term.CheckpointFrequency", 0, 0, (int)TEXTSW_INFINITY);
	    edit_log_wraps_at =
		defaults_get_integer_check("cmdtool.maxLogFileSize",
			"Term.MaxLogFileSize",
			DEFAULT_LOGFILE_SIZE, 0, (int)TEXTSW_INFINITY);
	}
	while (argc > 0 && **argv == '-') {
		switch (argv[0][1]) {
		case 'C':
			become_console = 1;
			break;
 		case 'h':
 		case '-':	
		case 'H':
		case '?':
			print_usage(am_cmdtool, tool_name);
			(void)xv_usage(tool_name);
			exit(1);
		case 'B':
			if (argc > 1) {
				argv++;
				argc--;
				bold_name = *argv;
			}
			break;
		case 'I':
			if (argc > 1) {
				argv++;
				argc--;
				init_cmd = *argv;
			}

			break;
		case 'P':
			if (argv[1] == NULL) {
			    print_usage(am_cmdtool, tool_name);
			    exit(1);
			}
			checkpoint = atoi(argv[1]);
			argc--, argv++;
			break;
		case 'M':
			if (argv[1] == NULL) {
			    print_usage(am_cmdtool, tool_name);
			    exit(1);
			}
			edit_log_wraps_at = atoi(argv[1]);
			argc--, argv++;
			break;
		case 'L':	/* jcb 5/10/90 runs .login on startup */
			run_login++;
			break;
		default:
			;
		}
		argv++;
		argc--;
	}

	if (argc == 0) {
		argv = sh_argv;
		if ((argv[0] = getenv("SHELL")) == NULL)
			argv[0] = "/bin/sh";
	}

	/* if the user wants to run .login format shell, prefix '-' to name */
	if( run_login && argv != NULL ) {	/* jcb 5/10/90 */
		strcpy( cmdline2, "-"  );
		strcat( cmdline2, argv[0] );
		argv[0]	= cmdline2;
	}

	/* If FRAME_LABEL wasn't set by cmdline argument, set it */
	if ((tmp_label1 = (char *)xv_get(base_frame, FRAME_LABEL)) == NULL) {
		(void)strncpy(frame_label,
		  am_cmdtool ? cmd_label : shell_label, sizeof(frame_label));
		if (become_console) {
			(void)strncat(frame_label, console_label,
				sizeof(frame_label));
		} else {
			(void)strncat(frame_label, " - ", sizeof(frame_label));
		}
		(void)strncat(frame_label, *argv, sizeof(frame_label));
		(void)xv_set(base_frame, FRAME_LABEL, frame_label, 0);
	}
	tool_icon = (Icon)xv_get(base_frame, FRAME_ICON);
	if (((tmp_label2 = (char *) xv_get(tool_icon, ICON_LABEL)) == NULL) ||
	    *tmp_label2 == 0177) {
	    if (tmp_label1) {
		(void)strncpy(icon_label, tmp_label1, sizeof(icon_label));
            } else if (become_console) {
		(void)strncpy(icon_label, MSG("Console"), sizeof(icon_label));
	    } else {
		(void)strncpy(icon_label, *argv, sizeof(icon_label));
	    }
	    (void)xv_set(tool_icon, ICON_LABEL, icon_label, 0);
	    xv_set(tool_icon, ICON_LABEL, icon_label, NULL);
	}
	

	/*
	 * We need to call putenv_locale() after the xv_init(), but
	 * before fork/exec the shell.
	 */
	putenv_locale(base_frame);


	ttysw = xv_create(base_frame, TERMSW,   WIN_IS_CLIENT_PANE,
		  TTY_ARGV,			argv,
		  TTY_QUIT_ON_CHILD_DEATH,	TRUE,
		  TTY_CONSOLE,			become_console,
		  0);

	if (!(defaults_exists("window.width", "Window.Width") ||
	      defaults_exists("window.height", "window.height") ||
	      defaults_exists("window.geometry", "Window.Geometry"))) {
	    int cols, rows;
	    
	    cols = defaults_get_integer_check("window.columns", "Window.Columns", 
					      80, 1, 999);
	    rows = defaults_get_integer_check("window.rows", "Window.Rows",
					      34, 1, 999);
/* Changed back from 35 to fix bug ID 1256498 */
	    
	    xv_set(ttysw,
		   WIN_COLUMNS, cols,
		   WIN_ROWS, rows,
		   0);
	    window_fit(base_frame);
	}

        if (!am_cmdtool)
		xv_set(ttysw, TERMSW_MODE, TTYSW_MODE_TYPE, 0);
	if (bold_name) {
		(void)xv_set(ttysw, TTY_BOLDSTYLE_NAME, bold_name, 0);
	}
	if (am_cmdtool) {

            /* if the user specified wraparound size (thru the
             * -M flag or defaults) is less than the minimum,
             * silently set it to SCRATCH_MIN_LEN
            */
	    if (edit_log_wraps_at == 0)
		    edit_log_wraps_at = TEXTSW_INFINITY;
	    else if (edit_log_wraps_at < MIN_LOGFILE_SIZE)
		    edit_log_wraps_at = MIN_LOGFILE_SIZE;

            /* Turn off AGAIN_RECORDING to save memory */
	    (void) xv_set(ttysw,
			  TEXTSW_CHECKPOINT_FREQUENCY, checkpoint,
			  TEXTSW_WRAPAROUND_SIZE, edit_log_wraps_at,
                          TEXTSW_AGAIN_RECORDING, FALSE,
			  0);
  
	}
	
	tty_pid = (int)xv_get(ttysw, TTY_PID);
#ifdef DEBUG
	(void)fprintf(stderr, "child pid = %d\n", tty_pid);
#endif DEBUG
	if (tty_pid == -1) {
	    strcpy(err_msg, (am_cmdtool) ? MSG("Command") : MSG("Shell"));
	    strcat(err_msg, MSG(" Tool: Out of swap space.  Cannot continue.\n"));
	    (void) ttysw_output(ttysw, err_msg, strlen(err_msg));        
	} else if (init_cmd && ((len = strlen(init_cmd)) > 0)) {
	    if (init_cmd[len-1] != '\n') {
		init_cmd[len] = '\n';
		len++;
	    }
	    (void)ttysw_input(ttysw, init_cmd, len);
	}

	/*
	 *  Do not always assume that SIGHUP and SIGQUIT signals should 
	 *  be caught.  In case we get started with nohup preceeding cmdtool,
	 *  both of these signals should be ignored.  If the disposition
	 *  of the SIGHUP handler was SIG_IGN before, do not try to come
	 *  up with a new disposition.  Same with SIGQUIT.
	 *
	 *  The test below will set the handler's disposition to SIG_IGN
	 *  in all cases, but this is OK, since if the disposition was not
	 *  SIG_IGN, then a new disposition will be installed anyway.
	 */
	if((int)signal(SIGHUP, SIG_IGN) != (int)SIG_IGN) {
	    notify_set_signal_func((Notify_client)ttysw, cmdtool_signal, 
				   SIGHUP, NOTIFY_ASYNC);
	}
	if((int)signal(SIGQUIT, SIG_IGN) != (int)SIG_IGN) {
	    notify_set_signal_func((Notify_client)ttysw, cmdtool_signal, 
				   SIGQUIT, NOTIFY_ASYNC);
	}
	notify_set_signal_func((Notify_client)ttysw, cmdtool_signal, SIGINT,
			       NOTIFY_ASYNC);
        notify_set_signal_func((Notify_client)ttysw, cmdtool_signal, SIGPIPE,
			       NOTIFY_ASYNC);
    	notify_set_signal_func((Notify_client)ttysw, cmdtool_signal, SIGTERM,
			       NOTIFY_ASYNC);

	xv_main_loop(base_frame);
	
	exit(0);
}


static void
set_textdomain()
{
	char	pathname[MAXPATHLEN + 1];
	char	*owhome = getenv("OPENWINHOME");

	(void) sprintf(pathname, "%s/lib/locale", owhome ? owhome : "/usr/openwin");
	bindtextdomain(my_textdomain, pathname);
}


typedef struct _lc_table {
	Xv_generic_attr		 xv_attr;
	char			*env_name;
} lc_table;

static const lc_table ollocale[] = {
	{ XV_LC_BASIC_LOCALE,	"LC_CTYPE" },
	{ XV_LC_DISPLAY_LANG,	"LC_MESSAGES" },
	{ XV_LC_NUMERIC,	"LC_NUMERIC" },
	{ XV_LC_TIME_FORMAT,	"LC_TIME" },
	{ NULL,	NULL }};

static const char *none_ollocale[] = {
	"LC_COLLATE", "LC_MONETARY", NULL };

/*
 * Set the locale related environment variables just before fork/exec
 * the shell.  This should be a part of XView functionality.
 */
static void
putenv_locale(frame)
    Frame	frame;
{
    lc_table const	 *oll;
    const char		**noll;
    char		 *locale;
    char		 *exist;
    char		  basiclocale[MAXNAMELEN + 1];
    char		  env[MAXNAMELEN + 20];


    (void) strcpy(basiclocale, (char *) xv_get(frame, XV_LC_BASIC_LOCALE));
    if ((exist = getenv("LANG")) == NULL || strcmp(exist, basiclocale) != 0) {
	(void) sprintf(env, "LANG=%s", basiclocale);
	/*
	 * Remember, putenv(3C) will not copy the contents, but just a
	 * pointer, we need to have a dedicated storage for each
	 * string.
	 */
	(void) putenv(strdup(env));
    }

    /*
     * Export each categories when it necessary (different than $LANG
     * or $LC_* already existed with different value (since, we cann't
     * unsetenv).
     */
    for (oll = ollocale; oll->xv_attr != NULL; oll++) {
	locale = (char *) xv_get(frame, oll->xv_attr);
	if ((strcmp(locale, basiclocale) != 0)
	 || ((exist = getenv(oll->env_name)) && strcmp(locale, exist) != 0)) {
	     (void) sprintf(env, "%s=%s", oll->env_name, locale);
	     (void) putenv(strdup(env));
	}
    }

    /*
     * Take care the non OPEN LOOK locale categories.  What we should
     * do is unsetenv, however libc does not provide the functionality.
     */
   for (noll = none_ollocale; *noll; noll++) {
	if ((exist = getenv(*noll)) && strcmp(basiclocale, exist) != 0) {
		(void) sprintf(env, "%s=%s", *noll, basiclocale);
		(void) putenv(strdup(env));
	}
   }
}
