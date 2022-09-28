#ifndef lint
#ifdef sccs
static char     sccsid[] = "@(#)tty_init.c 20.83 97/05/21";
#endif
#endif

/*
 *	(c) Copyright 1989-1993 Sun Microsystems, Inc. Sun design patents
 *	pending in the U.S. and foreign countries. See LEGAL_NOTICE
 *	file for terms of the license.
 */

/*
 * Ttysw initialization, destruction and error procedures
 */

#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <errno.h>
#include <signal.h>

#include <xview_private/portable.h>	/* for XV* defines and termios */

#ifdef	XV_USE_SVR4_PTYS
#include <sys/stream.h>
#include <sys/stropts.h>
#include <sys/bufmod.h>
#endif	/* XV_USE_SVR4_PTYS */

#ifndef SVR4
#include <utmp.h>
#else
 /* #include <sys/sigaction.h> */  /* vmh - 7/16/90 The compiler chokes
  * on the redefinition of sigaction, and on siginfo_t. Removing this
  * seems ok, since we seem not to use anything from it. The file
  * compiles this way. */
#include <utmpx.h>
#endif
#include <stdio.h>
#include <pwd.h>
#include <ctype.h>
#include <fcntl.h>

#include <pixrect/pixrect.h>
#include <pixrect/pr_util.h>

#ifdef __STDC__ 
#ifndef CAT
#define CAT(a,b)        a ## b 
#endif 
#endif
#include <pixrect/memvar.h>

#include <xview_private/i18n_impl.h>

#ifdef SVR4
#include <xview/notify.h>
#ifdef INTERACTIVE
#include <X11/extensions/XInteractive.h>
#pragma weak XSolarisIASetProcessInfo
#endif
#endif

#include <xview/win_input.h>
#include <xview_private/win_keymap.h>
#include <xview/defaults.h>
#include <xview/ttysw.h>
#include <xview/font.h>
#include <xview/cursor.h>
#include <xview/scrollbar.h>
#include <xview_private/term_impl.h>
#include <xview_private/tty_impl.h>	/* for WE_TTYPARMS */
#include <xview_private/charscreen.h>
#include <xview_private/draw_impl.h>

#define TTYSLOT_NOTFOUND(n)		((n)<=0)	/* BSD returns 0; S5
							 * returns -1 */

#if defined(WITH_3X_LIBC) || defined(vax) || defined(SVR4)
/* 3.x - 4.0 libc transition code; old (pre-4.0) code must define the symbol */
#define jcsetpgrp(p)	setpgrp((p),(p))
#endif


extern char    *strncpy();
extern char    *strcpy();
extern long     lseek();
char           *textsw_checkpoint_undo();
extern void     ttysw_updateutmp();

/* static */ void ttysw_parseargs();
static int	ttyinit();

#ifndef SVR4
#ifndef TIOCUCNTL
#define	TIOCUCNTL	_IOW(t, 102, int)	/* pty: set/clr usr cntl mode */
#endif				/* TIOCUCNTL */
#ifndef TIOCTCNTL
#define TIOCTCNTL       _IOW(t, 32, int)	/* pty: set/clr intercept
						 * ioctl mode */
#endif				/* TIOCTCNTL */
#endif	/* !SVR4 */



char           *getenv();

extern Xv_opaque xv_pf_open();
Xv_private char *xv_font_monospace();

extern int      ttysel_use_seln_service;
static Ttysw_folio     ttysw_folio_global;
static int     ttysw_updated_utmp;


struct ttysw_createoptions {
    int             becomeconsole;	/* be the console */
    char          **argv;	/* args to be used in exec */
    char           *args[4];	/* scratch array if need to build argv */
};

Xv_Cursor       ttysw_cursor;	/* default (text) cursor) */
Xv_Cursor       ttysw_stop_cursor;	/* stop sign cursor (i.e., CTRL-S) */

static Defaults_pairs bold_style[] = {
    "None", TTYSW_BOLD_NONE,
    "Offset_X", TTYSW_BOLD_OFFSET_X,
    "Offset_Y", TTYSW_BOLD_OFFSET_Y,
    "Offset_X_and_Y", TTYSW_BOLD_OFFSET_X | TTYSW_BOLD_OFFSET_Y,
    "Offset_XY", TTYSW_BOLD_OFFSET_XY,
    "Offset_X_and_XY", TTYSW_BOLD_OFFSET_X | TTYSW_BOLD_OFFSET_XY,
    "Offset_Y_and_XY", TTYSW_BOLD_OFFSET_Y | TTYSW_BOLD_OFFSET_XY,
    "Offset_X_and_Y_and_XY", TTYSW_BOLD_OFFSET_X |
    TTYSW_BOLD_OFFSET_Y |
    TTYSW_BOLD_OFFSET_XY,
    "Invert", TTYSW_BOLD_INVERT,
    NULL, -1
};

static Defaults_pairs inverse_and_underline_mode[] = {
    "Enable", TTYSW_ENABLE,
    "Disable", TTYSW_DISABLE,
    "Same_as_bold", TTYSW_SAME_AS_BOLD,
    NULL, -1
};

Pkg_private int
ttysw_lookup_boldstyle(str)
    char           *str;
{
    int             bstyle;
    if (str && isdigit(*str)) {
	bstyle = atoi(str);
	if (bstyle < TTYSW_BOLD_NONE || bstyle > TTYSW_BOLD_MAX)
	    bstyle = -1;
	return bstyle;
    } else {
	return defaults_lookup(str, bold_style);
    }
}

Pkg_private int
ttysw_print_bold_options()
{
    Defaults_pairs *pbold;

    (void) fprintf(stderr, "Options for boldface are %d to %d or:\n",
		   TTYSW_BOLD_NONE, TTYSW_BOLD_MAX);
    for (pbold = bold_style; pbold->name; pbold++) {
	(void) fprintf(stderr, "%s\n", pbold->name);
    }
}

/*
 * Ttysw initialization.
 */
Pkg_private     Xv_opaque
ttysw_init_view_internal(parent, tty_view_public)
    Tty             parent;
    Tty_view        tty_view_public;
{
    Ttysw_view_handle ttysw_view = (Ttysw_view_handle)
    calloc(1, sizeof(Ttysw_view_object));
    Xv_Drawable_info *info;

    /* Allocate private object and set links so TTY_PRIVATE/PUBLIC works.  */
    if (ttysw_view == 0)
	return ((Xv_opaque) NULL);

    ((Xv_tty_view *) tty_view_public)->private_data = (Xv_opaque) ttysw_view;
    ttysw_view->public_self = (Tty_view) tty_view_public;
    ttysw_view->folio = TTY_PRIVATE_FROM_ANY_PUBLIC(parent);
    ttysw_view->folio->current_view_public = tty_view_public;
    ttysw_view->folio->view = ttysw_view;

    /* Bug alert: what about XV_HELP_DATA */


    if (xv_tty_imageinit(ttysw_view->folio, tty_view_public) == 0) {
	free((char *) ttysw_view);
	return ((Xv_opaque) NULL);
    }
    /* create stop cursor but don't show it */
    DRAWABLE_INFO_MACRO(tty_view_public, info);	/* define info */
    ttysw_stop_cursor = xv_get(xv_server(info), XV_KEY_DATA, CURSOR_STOP_PTR);
    if (!ttysw_stop_cursor) {
	ttysw_stop_cursor = xv_create(tty_view_public, CURSOR,
				      CURSOR_SRC_CHAR, OLC_STOP_PTR,
				      CURSOR_MASK_CHAR, 0,
				      0);
	xv_set(xv_server(info),
	       XV_KEY_DATA, CURSOR_STOP_PTR, ttysw_stop_cursor,
	       0);
    }
    xv_set(tty_view_public,
	   WIN_ROW_HEIGHT, xv_get(parent, WIN_ROW_HEIGHT),
	   WIN_RETAINED, xv_get(xv_screen(info), SCREEN_RETAIN_WINDOWS),
	   XV_HELP_DATA, "xview:ttysw",
	   0);

    return ((Xv_opaque) ttysw_view);

}

Pkg_private     Xv_opaque
ttysw_init_folio_internal(tty_public)
    Tty             tty_public;
{
    extern          ttysw_eventstd();
    Ttysw_folio     ttysw;
    Xv_opaque       font = NULL;
    int             is_client_pane;
    char            *font_name = NULL;
#ifdef OW_I18N
    char	    *current_defaults_locale;
#endif

    /* Allocate private object and set links so TTY_PRIVATE/PUBLIC works.  */
    ttysw = (Ttysw_folio) calloc(1, sizeof(Ttysw));
    if (ttysw == 0)
	return ((Xv_opaque) NULL);

    if (!multibyte) {
        ttysw->ttysw_ibuf.cb_buf.SB = (char *) calloc(1, TTYSW_BUFLEN+1);
        ttysw->ttysw_obuf.cb_buf.SB = (char *) calloc(1, TTYSW_BUFLEN+1);
    } else {
        ttysw->ttysw_ibuf.cb_buf.WC = 
               (wchar_t *) calloc(1, (TTYSW_BUFLEN+1)*sizeof(wchar_t));
        ttysw->ttysw_obuf.cb_buf.WC = 
               (wchar_t *) calloc(1, (TTYSW_BUFLEN+1)*sizeof(wchar_t));
    }

    ((Xv_tty *) tty_public)->private_data = (Xv_opaque) ttysw;
    ttysw->public_self = (Tty) tty_public;
    ttysw_folio_global = ttysw;

    ttysw->ttysw_eventop = ttysw_eventstd;
    /* Following call only affect appearance of ttysw, not termsw */
    (void) ttysw_setboldstyle(
			      defaults_lookup(
					      (char *) defaults_get_string("term.boldStyle", "Term.BoldStyle", "Invert"),
					      bold_style));
    (void) ttysw_set_inverse_mode(
				  defaults_lookup(
						  (char *) defaults_get_string("term.inverseStyle", "Term.InverseStyle", "Enable"),
					       inverse_and_underline_mode));
    (void) ttysw_set_underline_mode(
				    defaults_lookup(
						    (char *) defaults_get_string("term.underlineStyle", "Term.UnderlineStyle", "Enable"),
					       inverse_and_underline_mode));

    ttysw->ttysw_ibuf.cb_rbp.SB = ttysw->ttysw_ibuf.cb_buf.SB;
    ttysw->ttysw_ibuf.cb_wbp.SB = ttysw->ttysw_ibuf.cb_buf.SB;
    if (multibyte) {
        ttysw->ttysw_ibuf.cb_ebp.WC =  
                   &ttysw->ttysw_ibuf.cb_buf.WC[TTYSW_BUFLEN];
        ttysw->ttysw_obuf.cb_rbp.WC = ttysw->ttysw_obuf.cb_buf.WC;
        ttysw->ttysw_obuf.cb_wbp.WC = ttysw->ttysw_obuf.cb_buf.WC;
        ttysw->ttysw_obuf.cb_ebp.WC = 
                   &ttysw->ttysw_obuf.cb_buf.WC[TTYSW_BUFLEN];
    } else {
        ttysw->ttysw_ibuf.cb_ebp.SB =  
                   &ttysw->ttysw_ibuf.cb_buf.SB[TTYSW_BUFLEN];
        ttysw->ttysw_obuf.cb_rbp.SB = ttysw->ttysw_obuf.cb_buf.SB;
        ttysw->ttysw_obuf.cb_wbp.SB = ttysw->ttysw_obuf.cb_buf.SB;
        ttysw->ttysw_obuf.cb_ebp.SB = 
                   &ttysw->ttysw_obuf.cb_buf.SB[TTYSW_BUFLEN];
    }

    ttysw->ttysw_kmtp = ttysw->ttysw_kmt;

    (void) ttysw_readrc(ttysw);

    xv_set(tty_public, XV_HELP_DATA, "xview:ttysw", 0);

    if (ttyinit(ttysw) == XV_ERROR) {
	free((char *) ttysw);
	return ((Xv_opaque) NULL);
    }
    ttysw_ansiinit(ttysw);

    /* initialize selection service code */
    (void) ttysw_setopt(ttysw, TTYOPT_SELSVC, ttysel_use_seln_service);
    if (ttysw_getopt((caddr_t) ttysw, TTYOPT_SELSVC)) {
	ttysel_init_client(ttysw);
    }
    (void) ttysw_mapsetim(ttysw);

    is_client_pane = (int) xv_get((Xv_object) tty_public, WIN_IS_CLIENT_PANE);

#ifdef OW_I18N
    if ((current_defaults_locale = defaults_get_locale()) != NULL)
	current_defaults_locale = xv_strsave(current_defaults_locale);
    defaults_set_locale(NULL, XV_LC_BASIC_LOCALE);

    font_name = xv_font_monospace();

    defaults_set_locale(current_defaults_locale, NULL);
    if (current_defaults_locale != NULL)
	xv_free(current_defaults_locale);

    if (font_name)
        font = xv_pf_open(font_name);
    else
	font = (Xv_opaque) 0;

    /*
     * if name is present, it has already been handled during the
     * creation of the "Window" superclass in window_init.
     */
    if (!font) {
        Xv_opaque       parent_font;
        int             scale, size;

        parent_font = (Xv_opaque) xv_get(tty_public, WIN_FONT);
        scale = (int) xv_get(parent_font, FONT_SCALE);
        if (scale > 0)
            font = (Xv_opaque) xv_find(tty_public, FONT,
                                FONT_FAMILY, FONT_FAMILY_DEFAULT_FIXEDWIDTH,
                                FONT_SCALE, scale,
                                NULL);
        else {
            size = (int) xv_get(parent_font, FONT_SIZE);
            font = (Xv_opaque) xv_find(tty_public, FONT,
                                FONT_FAMILY, FONT_FAMILY_DEFAULT_FIXEDWIDTH,
                                FONT_SIZE, size,
                                NULL);
        }
    }
 
    if (font == NULL)
        font = (Xv_opaque) xv_get(tty_public, WIN_FONT);
#else
    font_name = xv_font_monospace();
    if (font_name)
	font = xv_pf_open(font_name);
    else
	font = (Xv_opaque) 0;

    if (is_client_pane) {
	Xv_opaque       parent_font;
	int             scale, size;

	if (!font) {
	    parent_font = (Xv_opaque) xv_get(tty_public, WIN_FONT);
	    scale = (int) xv_get(parent_font, FONT_SCALE);
	    if (scale > 0) {
		font = (Xv_opaque) xv_find(tty_public, FONT,
				FONT_FAMILY, FONT_FAMILY_DEFAULT_FIXEDWIDTH,
		/* FONT_FAMILY,	FONT_FAMILY_SCREEN, */
		       FONT_SCALE, (scale > 0) ? scale : FONT_SCALE_DEFAULT,
					   0);
	    } else {
		size = (int) xv_get(parent_font, FONT_SIZE);
		font = (Xv_opaque) xv_find(tty_public, FONT,
				FONT_FAMILY, FONT_FAMILY_DEFAULT_FIXEDWIDTH,
		/* FONT_FAMILY,	FONT_FAMILY_SCREEN, */
			   FONT_SIZE, (size > 0) ? size : FONT_SIZE_DEFAULT,
					   0);
	    }
	}
    } else {
	if (!font) {
	    Xv_opaque       parent_font = (Xv_opaque) xv_get(tty_public, WIN_FONT);
	    int             scale = (int) xv_get(parent_font, FONT_SCALE);

	    if (scale > 0) {
	        font = (Xv_opaque) xv_find(tty_public, FONT,
				FONT_FAMILY, FONT_FAMILY_DEFAULT_FIXEDWIDTH,
	    			/* FONT_FAMILY,	FONT_FAMILY_SCREEN, */
		       		FONT_SCALE, (scale > 0) ? scale : FONT_SCALE_DEFAULT,
				0);
	    } else {
	        int             size = (int) xv_get(parent_font, FONT_SIZE);
	        font = (Xv_opaque) xv_find(tty_public, FONT,
				FONT_FAMILY, FONT_FAMILY_DEFAULT_FIXEDWIDTH,
	    			/* FONT_FAMILY,	FONT_FAMILY_SCREEN, */
			   	FONT_SIZE, (size > 0) ? size : FONT_SIZE_DEFAULT,
				0);
	    }
        }
    }
    if (!font)
	font = (Xv_opaque) xv_get(tty_public, WIN_FONT);
#endif

#ifdef OW_I18N
    xv_set(tty_public, WIN_FONT, font, NULL);
 
    ttysw->im_store = (wchar_t *)NULL;
    ttysw->im_attr = (XIMFeedback *)NULL;

    ttysw->preedit_state = FALSE;
    ttysw->im_len = 0;
    ttysw->implicit_commit = 0;
#endif

    xv_new_tty_chr_font(font);

    /* Set WIN_ROW_HEIGHT so that xv_set of WIN_ROWS will work when
     * Text.LineSpacing is set to a nonzero value.
     */
    xv_set(tty_public, WIN_ROW_HEIGHT, chrheight, 0);

    return ((Xv_opaque) ttysw);
}

/* static */ int
ttysw_add_FNDELAY(fd)
    int             fd;
{
    int             fdflags;

#ifdef SVR4
    if ((fdflags = xv_fcntl(fd, F_GETFL, 0)) == -1)
#else
    if ((fdflags = fcntl(fd, F_GETFL, 0)) == -1)
#endif
	return (-1);
    fdflags |= FNDELAY;
#ifdef SVR4
    if (xv_fcntl(fd, F_SETFL, fdflags) == -1)
#else
    if (fcntl(fd, F_SETFL, fdflags) == -1)
#endif
	return (-1);
    return (0);
}


/* ARGSUSED */
Pkg_private int
ttysw_fork_it(ttysw0, argv, wfd)
    caddr_t         ttysw0;
    char          **argv;
    int             wfd;	/* No longer used. */
{
    struct ttysubwindow *ttysw = (struct ttysubwindow *) ttysw0;
    struct ttysw_createoptions opts;
    int		    offset	= 0;
    char	    appname[20];
    char	    *p;
    unsigned        ttysw_error_sleep = 1;
#ifdef INTERACTIVE
    int pids[2];
    int majorop, first_event, first_error;
    int majorv, minorv;
    Xv_Drawable_info *info;
    Display * dpy;
#endif

#ifndef SVR4
    struct sigvec   vec, ovec;
#else
    struct sigaction	vec, ovec;
    extern char *ptsname();

#define BSD_TTY_COMPAT /* yank this if csh ever gets ported properly */

#ifdef BSD_TTY_COMPAT
    struct termios tp;
#endif
 
#endif /* SVR4 */
    ttysw->ttysw_pidchild = fork();
    if (ttysw->ttysw_pidchild < 0)	/* fork failed */
	return (-1);
    if (ttysw->ttysw_pidchild) {	/* parent */
	if (ttysw_add_FNDELAY(ttysw->ttysw_pty)) {
	    perror("fcntl");
	    /* Press on, as child is already launched. */
	}
#ifdef INTERACTIVE
    DRAWABLE_INFO_MACRO(ttysw->public_self, info);	/* define info */
    dpy=xv_display(info);
    pids[0]=getpid();
    pids[1]=ttysw->ttysw_pidchild;
    if ( (XSolarisIASetProcessInfo!=0) &&
       (XQueryExtension(dpy, "SolarisIA",
          &majorop, &first_event, &first_error)==True))
          XSolarisIASetProcessInfo(dpy, pids, INTERACTIVE_INFO, 2);
#endif


#ifdef DEBUG
	sleep(3);
#endif				/* DEBUG */
	return (ttysw->ttysw_pidchild);
    }

    /* Set up the child characteristics */
#ifndef SVR4  	/* SunOS4.x code */
    vec.sv_handler = SIG_DFL;
    vec.sv_mask = vec.sv_onstack = 0;
    sigvec(SIGWINCH, &vec, 0);
    /*
     * Become session leader, change process group of child 
     * process (me at this point in code) so
     * its signal stuff doesn't affect the terminal emulator.
     */
    setsid();
    vec.sv_handler = SIG_IGN;
    vec.sv_mask = vec.sv_onstack = 0;
    sigvec(SIGTTOU, &vec, &ovec);

    close(ttysw->ttysw_tty);

    /* Make the following file descriptor be my controlling terminal */
    ttysw->ttysw_tty = open("/dev/tty", O_RDWR, 0);  /* open master tty* */
    sigvec(SIGTTOU, &ovec, 0);

#else  		/* SVR4 code */
    vec.sa_handler = SIG_DFL;
    sigemptyset(&vec.sa_mask);
    vec.sa_flags = SA_RESTART;
    sigaction(SIGWINCH, &vec, (struct sigaction *) 0);
    /*
     * Become session leader, change process group of child 
     * process (me at this point in code) so
     * its signal stuff doesn't affect the terminal emulator.
     */
    setsid();
    vec.sa_handler = SIG_IGN;
    sigemptyset(&vec.sa_mask);
    vec.sa_flags = SA_RESTART;
    sigaction(SIGTTOU, &vec, &ovec);

    if (unlockpt(ttysw->ttysw_pty) == -1)
        perror("unlockpt (2)");
    if ((ttysw->ttysw_tty = open(ptsname(ttysw->ttysw_pty),O_RDWR))<0)
        return -1;

    sigaction(SIGTTOU, &ovec, (struct sigaction *) 0);
#endif /* SVR4 */

    /*
     * Initialize file descriptors. Connections to servers are marked as
     * close-on-exec, and don't need any further cleanup.
     */
    (void) close(ttysw->ttysw_pty);
#ifndef SVR4
    ttysw->ttysw_tty = open(ttysw->tty_name, O_RDWR);  /* open /dev/ttyp* */
#endif
    (void) dup2(ttysw->ttysw_tty, 0);
    (void) dup2(ttysw->ttysw_tty, 1);
    (void) dup2(ttysw->ttysw_tty, 2);
    (void) close(ttysw->ttysw_tty);

    if (*argv == (char *) NULL || strcmp("-c", *argv) == 0) {
	/* Process arg list */
	int             argc;

	for (argc = 0; argv[argc]; argc++);
	ttysw_parseargs(&opts, &argc, argv);
	argv = opts.argv;
    } else if( *argv[0] == '-' ) {	/* jcb 5/10/90 -- to run .login file */
	    
	    /* have to change "-/bin/ksh" to "-ksh" */
	    p = (char *)strrchr( argv[0], '/' ); 

	    if( p != NULL ) {
		    strcpy( appname, "-" );
		    strncat( appname, ++p, 20 - strlen(appname) );
		    argv[0]	= appname;
	    }
	    offset++;
    }

#ifdef SVR4
#ifdef BSD_TTY_COMPAT
/*
 * ttcompat seems to leave things in a funny state and assumes
 * (seemingly) that login will fix things up.  Do it here.
 */
    if (ioctl (0, TCGETS, &tp) == -1)
        perror("ioctl TCGETS");
    else {
        tp.c_lflag |= ECHO;
        tp.c_oflag |= ONLCR;
        tp.c_iflag |= ICRNL;
    }
    if (ioctl (0, TCSETS, &tp) == -1)
        perror("ioctl TCSETS");
#endif /* BSD_TTY_COMPAT */
#endif /* SVR4 */


    /* restore various signals to their defaults */
    signal (SIGINT, SIG_DFL);
    signal (SIGQUIT, SIG_DFL);
    signal (SIGTERM, SIG_DFL);

    /* fork the shell, or whatever was specified on the cmdline */
    execvp(*argv+offset, argv);
    perror(*argv);
    /* Wait a few seconds so that message can be read on ttysw */
    sleep(ttysw_error_sleep);
    exit(1);
    /* NOTREACHED */
}

/*
 * Create options calls
 */
/* static */ void
ttysw_parseargs(opts, argcptr, argv_base)
    struct ttysw_createoptions *opts;
    int            *argcptr;
    char          **argv_base;
{
    char           *shell;
    int             argc = *argcptr;
    char          **argv = argv_base;
    char          **args = argv;

    XV_BZERO((caddr_t) opts, sizeof(*opts));
    /* Get options */
    while (argc > 0) {
	if (strcmp(argv[0], "-C") == 0 ||
	    strcmp(argv[0], "CONSOLE") == 0) {
	    opts->becomeconsole = 1;
	    (void) xv_cmdline_scrunch(argcptr, argv_base, argv, 1);
	} else
	    argv++;
	argc--;
    }
    argv = args;
    opts->argv = opts->args;
    /* Determine what shell to run. */
    shell = getenv("SHELL");
    if (!shell || !*shell)
	shell = "/bin/sh";
    opts->args[0] = shell;
    /* Setup remainder of arguments */
    if (*argv == (char *) NULL) {
	opts->args[1] = (char *) NULL;
#ifndef someday			/* this should go away */
    } else if (strcmp("-c", *argv) == 0) {
	/*
	 * The '-c' flag tells the shell to run following arg
	 */
	opts->args[1] = argv[0];
	opts->args[2] = argv[1];
	(void) xv_cmdline_scrunch(argcptr, argv_base, argv, 2);
	opts->args[3] = (char *) NULL;
#endif
    } else {
	/*
	 * Run program not under shell
	 */
	opts->argv = argv;
    }
    return;
}

/*
 * Ttysw cleanup.
 */
Pkg_private void
ttysw_done(ttysw_folio_private)
    Ttysw_folio     ttysw_folio_private;
{

    if (ttysw_folio_private->ttysw_ttyslot)
	(void) updateutmp("", ttysw_folio_private->ttysw_ttyslot,
			  ttysw_folio_private->ttysw_tty);

    ttysel_destroy(ttysw_folio_private);

    if (ttysw_folio_private->ttysw_pty)
	close(ttysw_folio_private->ttysw_pty);
    if (ttysw_folio_private->ttysw_tty)
	close(ttysw_folio_private->ttysw_tty);

#ifdef OW_I18N
    /*
     * Free the memory allocated for pre-edit text and attribute.
     */
    if ( ttysw_folio_private->im_store )
	free((char *) ttysw_folio_private->im_store);
    if ( ttysw_folio_private->im_attr )
	free((char *) ttysw_folio_private->im_attr);
#endif

    if (ttysw_folio_private->ttysw_ibuf.cb_buf.SB)
       free((char *) ttysw_folio_private->ttysw_ibuf.cb_buf.SB);
    if (ttysw_folio_private->ttysw_obuf.cb_buf.SB)
       free((char *) ttysw_folio_private->ttysw_obuf.cb_buf.SB);

    free((char *) ttysw_folio_private);
}

/*
 * Do tty/pty setup
 *
 * XXX:	This routine needs lots of cleanup for differences in pty conventions
 *	between systems.
 */
static int	/* Returns XV_ERROR or XV_OK */
ttyinit(ttysw)
    Ttysw          *ttysw;
{
    int		    tmpfd;
    int		    pty = 0, tty = 0;
    int             on = 1;
#ifndef SVR4
    int             ptynum = 0;
    char            linebuf[20], *line = &linebuf[0];
    char	    *ptyp = "pqrstuvwxyzPQRST";
    struct stat     stb;

    /*
     * find unopened pty
     */
needpty:
    while (*ptyp) {
	(void) strcpy(line, "/dev/ptyXX");
	line[strlen("/dev/pty")] = *ptyp;
	line[strlen("/dev/ptyp")] = '0';
	if (stat(line, &stb) < 0)
	    break;
	while (ptynum < 16) {
	    line[strlen("/dev/ptyp")] = "0123456789abcdef"[ptynum];
	    pty = open(line, O_RDWR);
	    if (pty > 0)
		goto gotpty;
	    ptynum++;
	}
	ptyp++;
	ptynum = 0;
    }
    (void) fprintf(stderr, XV_MSG("All pty's in use\n"));
    return XV_ERROR;
    /* NOTREACHED */
gotpty:
    line[strlen("/dev/")] = 't'; /* change "/dev/pty" to "/dev/tty" */
    strcpy(ttysw->tty_name, line);
    tty = open(line, O_RDWR); /* open /dev/ttyp* */
    if (tty < 0) {
	ptynum++;
	(void) close(pty);
	goto needpty;
    }
#else /* SVR4 */
    extern char *ptsname();
     
    /*
     * Open /dev/ptmx to get master clone device, go through the
     * ritual to get a slave device and set it up as a tty.
     */
    if ((pty = open("/dev/ptmx", O_RDWR)) < 0) {
	perror("Unable to open /dev/ptmx");
	return XV_ERROR;
    }

    /* grantpt is used to prevent other processes from grabbing the
     * tty that goes with the pty that we have opened.  It is a mandatory
     * step in the SVR4 pty-tty initialization.
     * Note: The /dev filesystem must be mounted read-write.
     */
    if (grantpt(pty) == -1) {
        perror("grantpt");
	return XV_ERROR;
    }

    if (unlockpt(pty) == -1) {
        perror("unlockpt");
	return XV_ERROR;
    }

#ifdef SB_NO_DROPS /* defined as result of including new bufmod.h */

    if(ioctl(pty, I_PUSH, "bufmod") == -1) { /* some buffering ... */

      /*EMPTY*/
      /* we can't push bufmod... this means we're probably 
	 running on a generic SVR4 system - we can ignore this
	 error since bufmod is used for performance reasons
	 only
	 */
      
    }
    
    else {
      struct timeval timeval;
      struct strioctl cmd;
      unsigned int chunk;
      extern int tty_has_new_bufmod;
      /*
	note that we're not using SB_SEND_ON_WRITE | SB_DEFER_CHUNK.
	Turns out the shell (or someone down the pty) does an ioctl
	when sending out each prompt. Since this flushes any partially
	filled chunk automatically, we really don't need this.  
	*/
      
      chunk = SB_NO_DROPS | SB_NO_PROTO_CVT | SB_NO_HEADER  ;
      
      
      cmd.ic_timout 	= 0;
      cmd.ic_cmd 	= SBIOCSFLAGS;
      cmd.ic_len 	= sizeof(u_long);
      cmd.ic_dp 	= (char *) & chunk;
      
      if(ioctl(pty, I_STR, &cmd) < 0) {
	
	/* perror("SBIOCSFLAGS");
	   If we pushed bufmod, but this ioctl fails it means
	   we're most likely running on a system with the old bufmod
	   ie for released OSs this must be Jupiter.  We treat this
	   error silently so developers and users of the Mars trees
	   don't get confused... treat it like bufmod wasn't there
	   at all
	   */
	
	goto backoff;
      }
      
      /*
	now we need to set the maximum delay time for a packet.  After 
	some ad-hoc experiments, I've chosen to use 5 100 Hz ticks.  This may
	need to be revisted later - it works pretty nicely for now.
	*/
      
      
      timeval.tv_usec = 50000;
      timeval.tv_sec = 0;
      
      cmd.ic_cmd = SBIOCSTIME;
      cmd.ic_timout = 0;
      cmd.ic_len = sizeof(timeval);
      cmd.ic_dp = (char *)& timeval;
      
      if(ioctl(pty, I_STR, &cmd) < 0) {
	perror("SBIOCSTIME"); /* these are legit errors - if we have new
				 bufmod this should have worked */
	goto backoff;
      }
      
      /*
	I have made the chunk size the same as the buffer used in the 
	ttysw.  One could experiement more here, but this works.
	*/

      if (!multibyte)
          chunk = TTYSW_BUFLEN;
      else
          chunk = TTYSW_BUFLEN * sizeof(wchar_t);
      
      cmd.ic_cmd = SBIOCSCHUNK;
      cmd.ic_len = sizeof(int);
      cmd.ic_dp = (char *) & chunk;
      
      if(ioctl(pty, I_STR, &cmd) < 0) {
	perror("SBIOCSCHUNK");
	goto backoff;
      }
      
      /*
	we certainly don't want to truncate any packets, so
	set the snap length to zero
	*/
      
      chunk = 0;
      cmd.ic_cmd = SBIOCSSNAP;
      cmd.ic_len = sizeof(int);
      cmd.ic_dp = (char *) & chunk;
      
      if(ioctl(pty, I_STR, &cmd) < 0) {
	perror("SBIOCSSNAP");
	goto backoff;
      }
      
      /*
	set (gack!) global flag so tty routines
	know not to do user-land spin buffering
	*/
      
      tty_has_new_bufmod = 1;
      goto ok;
      
    backoff:
      /*
	something didn't work out, so pull bufmod off the stream
	and continue as if it weren't there.....
	*/
      
      if (ioctl(pty, I_POP, 0) == -1) { /* bufmod not working or wrong
					   version... */
	perror("I_POP bufmod");
      }
      
    }

  ok:

#endif SB_NO_DROPS


    if (ioctl(pty, I_PUSH, "pckt") == -1) { /* must use getmsg for read */
        perror("push pckt");
	return XV_ERROR;
    }

    if ((tty = open(ptsname(pty),O_RDWR))<0)
        return XV_ERROR;
    if (ioctl(tty, I_PUSH, "ptem") == -1) {
        perror("push ptem");
	return XV_ERROR;
    }
    if (ioctl(tty, I_PUSH, "ldterm") == -1) {
        perror("push ldterm");
	return XV_ERROR;
    }
#ifdef BSD_TTY_COMPAT
    if (ioctl(tty, I_PUSH, "ttcompat") == -1) { /* for csh */
        perror("push ttcompat");
	return XV_ERROR;
    }
#endif
 
#endif /* SVR4 */

    if (ttysw_restoreparms(tty))
	(void) putenv(WE_TTYPARMS_E);

    /*
     * Copy stdin.  Set stdin to tty so ttyslot in updateutmp will think this
     * is the control terminal.  Restore state. Note: ttyslot should have
     * companion ttyslotf(fd).
     */
    tmpfd = dup(0);
    (void) close(0);
    (void) dup(tty);
    ttysw->ttysw_ttyslot = updateutmp((char *)0, 0, tty);
    (void) close(0);
    (void) dup(tmpfd);
    (void) close(tmpfd);
    ttysw->ttysw_tty = tty;
    ttysw->ttysw_pty = pty;
#if defined(sun)  && ! defined(SVR4)
    if (ioctl(ttysw->ttysw_pty, TIOCTCNTL, &on) == -1) {
	perror(XV_MSG("TTYSW - setting TIOCTCNTL to 1 failed"));
	return XV_ERROR;
    }
#else				/* sun */
#ifndef SVR4
    if (ioctl(ttysw->ttysw_pty, TIOCPKT, &on) < 0) {
	perror(XV_MSG("TTYSW - setting TIOCPKT to 1 failed"));
	return XV_ERROR;
    }
#endif
#endif 				/* sun */

    return XV_OK;
}


Xv_private void
ttysw_updateutmp()
{
    if(ttysw_updated_utmp == 1)
        updateutmp("", ttysw_folio_global->ttysw_ttyslot,
                          ttysw_folio_global->ttysw_tty);
}

/* BUG ALERT: No XView prefix */
/* static */ int
updateutmp(username, ttyslotuse, ttyfd)
    char           *username;
    int             ttyslotuse, ttyfd;
{
    /*
     * Update /etc/utmp
     */
#ifdef SVR4
  struct utmpx    utmp;
#else
  struct utmp     utmp;
#endif
  struct passwd  *passwdent;
  extern struct  passwd *getpwuid();
  int             f;
  char           *ttyn;
  extern char    *ttyname();
  int 	   	 len, i;

    memset(&utmp, 0, sizeof (utmp));
    /*
     * Get login name and put in utmp.ut_name
     */
    if ((passwdent = getpwuid(getuid())) == (struct passwd *) NULL) {
        (void) fprintf(stderr, XV_MSG("couldn't find user name\n"));
        return (0);
    }
    username = passwdent->pw_name;

#ifdef SVR4
    utmp.ut_user[0] = '\0';	/* Set incase *username is 0 */
    (void) strncpy(utmp.ut_user, username, sizeof(utmp.ut_user));
#else
    utmp.ut_name[0] = '\0';	/* Set incase *username is 0 */
    (void) strncpy(utmp.ut_name, username, sizeof(utmp.ut_name));
#endif

    /*
     * Get tty name
     */
    ttyn = ttyname(ttyfd);
    if (ttyn == (char *) NULL)
#ifdef SVR4
	ttyn = "/dev/pts/??";
#else
	ttyn = "/dev/tty??";
#endif
    /*
     * Copy line name minus the preceding "/dev/" into utmp.ut_line
     */
    (void) strncpy(utmp.ut_line, (ttyn+sizeof("/dev/")-1), sizeof(utmp.ut_line));
#ifdef SVR4
        /*
         * Make a unique ID
         */
                len = strlen(utmp.ut_line);
                if (len > 0)
                        len--;
                for (i = 0; i < 4; i++)
                        utmp.ut_id[i] = len - i < 0 ? 0 : utmp.ut_line[len-i];

#endif /* SVR4 */

    /*
     * Set host to be empty
     */
    (void) strncpy(utmp.ut_host, "", sizeof(utmp.ut_host));
    /*
     * Get start time
     */
#ifdef SVR4
    (void) time((time_t *) (&utmp.ut_tv));
    utmp.ut_type = (ttyslotuse == 0) ? USER_PROCESS : DEAD_PROCESS;
    if(utmp.ut_type == USER_PROCESS)
	ttysw_updated_utmp = 1;
    else
	ttysw_updated_utmp = 2;

    if ((utmp.ut_pid = getpid()) == 0)
	return;
    pututxline(&utmp);
    return((ttyslotuse == 0) ? 1 : 0); 
#else
    (void) time((time_t *) (&utmp.ut_time));
    /*
     * Put utmp in /etc/utmp
     */
    if (ttyslotuse == 0)
	ttyslotuse = ttyslot();

    if (TTYSLOT_NOTFOUND(ttyslotuse)) {
	(void) fprintf(stderr,
		 XV_MSG("Cannot find slot in /etc/ttys for updating /etc/utmp.\n"));
	(void) fprintf(stderr, 
	XV_MSG("Commands like \"who\" will not work.\n"));
	(void) fprintf(stderr, 
		XV_MSG("Add tty[qrs][0-f] to /etc/ttys file.\n"));
	return (0);
    }
    if ((f = open("/etc/utmp", 1)) >= 0) {
	(void) lseek(f, (long) (ttyslotuse * sizeof(utmp)), 0);
	(void) write(f, (char *) &utmp, sizeof(utmp));
	(void) close(f);
    } else {
	(void) fprintf(stderr, 
	XV_MSG("make sure that you can write /etc/utmp!\n"));
	return (0);
    }
    return (ttyslotuse);
#endif
}

