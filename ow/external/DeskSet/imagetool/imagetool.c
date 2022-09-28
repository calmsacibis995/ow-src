#ifndef lint
static char sccsid[] = "@(#)imagetool.c 1.46 94/03/14";
#endif

/*
 * Copyright (c) 1986, 1987, 1988 by Sun Microsystems, Inc.
 */

/*
 * imagetool.c - Main program for ImageTool
 */

#include <netdb.h>
#include <stdlib.h>
#include <sys/param.h>
#include <sys/socket.h>
#include "display.h"
#include "image.h"
#include "imagetool.h"
#include "ui_imagetool.h"
#include "dstt.h"

/*
 * Global object definitions.
 */

BaseWindowObjects	*base_window;
OpenObjects		*openf;
PrintObjects   		*print = NULL;
SaveasObjects		*saveas = NULL;
OpenasObjects      	*openas = NULL;
PrintPreviewObjects	*print_preview = NULL;
ImageInfoObjects   	*imageinfo = NULL;
PsOptionsObjects	*ps_options = NULL;
PaletteObjects     	*palette = NULL;
PropsObjects       	*props = NULL;
CursorObjects      	*cursor = NULL;
PageviewObjects    	*pageview = NULL;
#ifdef FILECHOOSER
File_chooser            openfc;
File_chooser            savefc;
OpenasfcObjects         *openasfc;
SaveasfcObjects         *saveasfc;
#endif

ProgInfo		*prog;
XviewInfo		*xview_info;

Attr_attribute		 INSTANCE;
Attr_attribute		 DROPSITE;
Attr_attribute		 SELECTOR;

#ifndef XGETTEXT
char   			*MSGFILE_ERROR = DGET ("SUNW_DESKSET_IMAGETOOL_ERR");
char   			*MSGFILE_LABEL = DGET ("SUNW_DESKSET_IMAGETOOL_LABEL");
char   			*MSGFILE_MESSAGE = DGET ("SUNW_DESKSET_IMAGETOOL_MSG");
#endif

void
checknextarg (next_arg, cmd_option)
char 	*next_arg;
char 	*cmd_option;
{
    if (next_arg != (char *) NULL) 
       if (next_arg[0] != '-')
	  return;

    fprintf (stderr, MGET("%s: Need value for command line option %s\n"), 
				prog->name, cmd_option);
    exit (0);

}

/*
 * Parse the command line args before being
 * passed to xv_init().
 */
void
check_for_args (argc, argv)
    int		argc;
    char       *argv [];
{
    int		i;

    for (i = 0 ; i < argc; i++) {
/*
 * Check if we were started by tooltalk
 */

	if (strcmp (argv[i], DGET ("-tooltalk")) == 0) {
	   prog->tt_started = TRUE;
	   } 
		
/*
 * Check if the icon label is set on
 * the command line.
 */
	else if (strcmp (argv[i], DGET ("-WL")) == 0 ||
	         strcmp (argv[i], DGET ("-icon_label")) == 0) {

           if (strcmp (argv[i], DGET ("-WL")) == 0)
  	     checknextarg (argv [i+1], DGET ("-WL"));
           else if (strcmp (argv[i], DGET ("-icon_label")) == 0)
  	     checknextarg (argv [i+1], DGET ("-icon_label"));

	   xview_info->icon_label = (char *) malloc (strlen (argv[i+1]) + 1);
	   strcpy (xview_info->icon_label, argv [++i]);
	   xview_info->icon_label_set = TRUE;
	   } 
/*
 * Check if the frame label is set on
 * the command line.
 */
        else if (strcmp (argv[i], DGET ("-title")) == 0 ||
		 strcmp (argv[i], DGET ("-Wl")) == 0 ||
		 strcmp (argv[i], DGET ("-label")) == 0) {

	   if (strcmp (argv[i], DGET ("-title")) == 0)
  	     checknextarg (argv [i+1], DGET ("-title"));
	   else if (strcmp (argv[i], DGET ("-Wl")) == 0)
  	     checknextarg (argv [i+1], DGET ("-Wl"));
	   else if (strcmp (argv[i], DGET ("-label")) == 0)
  	     checknextarg (argv [i+1], DGET ("-label"));

	   xview_info->frame_label = (char *) malloc (strlen (argv[i+1]) + 1);
	   strcpy (xview_info->frame_label, argv [++i]);
	   xview_info->frame_label_set = TRUE;
	   }
/*
 * Check if the geometry is set on
 * the command line.
 */
        else if (strcmp (argv[i], DGET ("-geometry")) == 0 ||
		 strcmp (argv[i], DGET ("-Ws")) == 0) {

	   if (strcmp (argv[i], DGET ("-geometry")) == 0)
  	     checknextarg (argv [i+1], DGET ("-geometry"));
	   else if (strcmp (argv[i], DGET ("-Ws")) == 0)
  	     checknextarg (argv [i+1], DGET ("-Ws"));

	   xview_info->geometry_set = TRUE;
	   }
/*
 * Check if the depth is set on
 * the command line.
 */
        else if (strcmp (argv[i], DGET ("-depth")) == 0) {
  	   checknextarg (argv[i+1], DGET ("-depth"));
	   xview_info->depth = atoi (argv[i+1]);
	   if (xview_info->depth > 0 && xview_info->depth <= 24)
 	     xview_info->depth_set = TRUE;
	   }
/*
 * Check if the visual is set on
 * the command line.
 */
        else if (strcmp (argv[i], DGET ("-visual")) == 0) {

  	   checknextarg (argv [i+1], DGET ("-visual"));
	   xview_info->visual_set = TRUE;

	   if (strcmp (argv[i+1], DGET ("StaticGray")) == 0)
	     xview_info->visual = 0;
           else if (strcmp (argv[i+1], DGET ("GrayScale")) == 0)
	     xview_info->visual = 1;
           else if (strcmp (argv[i+1], DGET ("StaticColor")) == 0)
	     xview_info->visual = 2;
           else if (strcmp (argv[i+1], DGET ("PseudoColor")) == 0)
	     xview_info->visual = 3;
           else if (strcmp (argv[i+1], DGET ("TrueColor")) == 0)
	     xview_info->visual = 4;
           else if (strcmp (argv[i+1], DGET ("DirectColor")) == 0)
	     xview_info->visual = 5;
	   else
	     xview_info->visual_set = FALSE;
	   }
    }

}

void
usage ()
{
    fprintf (stderr, DGET ("Usage: %s %s"), "imagetool",
	DGET("[-usage] [-v] [-verbose] [-timeout] [imagefile]\n"));
}

char *
parseargs(argc, argv)
    int         argc;
    char       *argv[];
{
    extern char *xv_version;
    int          i;
    char        *s;
    char	*pathname = NULL;

    for (i = 1; i < argc; i++) {
	s = argv[i];
	if (s[0] == '-') {
	   if (!strcmp (DGET("-verbose"), s)) {
	      prog->verbose++;
	      }
	   else if (!strcmp (DGET("-usage"), s)) {
	      usage ();
	      exit (0);
	      }
	   else if (!strcmp (DGET ("-tooltalk"), s)) {
	      prog->tt_started = TRUE;
	      }
	   else if (!strcmp (DGET ("-timeout"), s)) {
	      prog->timeout = atoi(argv[i++]);
	      }
	   else if (!strcmp (DGET("-v"), s)) {
	      fprintf (stderr, MGET("%s version %s running on %s \n"),
					prog->name, prog->rel, xv_version);
	      exit (0);
	      } 
	   else {
	      fprintf (stderr, MGET("%s: Illegal switch `%s'\n"),
			prog->name, s);
	      usage ();
	      exit (1);
	      }
	   } 
	else {
	   pathname = malloc (strlen (s) + 1);
	   strcpy (pathname, s);
	   }
	}

    if (pathname != NULL) {
       if (pathname[0] != '/') {
	  char *tmp_file = malloc (strlen (pathname) + 1);
	  strcpy (tmp_file, pathname);
	  pathname = malloc (strlen (tmp_file) + strlen (prog->directory) + 2);
	  sprintf (pathname,"%s/%s", prog->directory, tmp_file);
	  free (tmp_file);
	  }
       else {
         char   *tmp_dir = strip_filename (pathname);
	 struct  stat file_info;

	 if (stat (tmp_dir, &file_info) == 0)
	   if (S_ISDIR (file_info.st_mode))
   	     prog->directory = tmp_dir;
       }
       return (pathname);
       }

    return ((char *) NULL);

}

void
complete_init_prog ()
{
    char  *appname = LGET ("Image Tool");
    char  *s;

    prog->name = malloc (strlen (appname) + 1);
    strcpy (prog->name, appname);

    prog->directory = getcwd (NULL, MAXPATHLEN);
    if (prog->directory == NULL ) {
       fprintf (stderr, MGET ("%s:  Can't get current directory!\n"),
		prog->name);
       exit(1);
       }

    prog->sb_right = TRUE;
    s = (char *) defaults_get_string (DGET ("openwindows.scrollbarplacement"),
				      DGET ("OpenWindows.ScrollbarPlacement"),
				      NULL);
    if (s) {
      if (strcasecmp (s, "left") == 0)
	prog->sb_right = FALSE;
    }

}

ProgInfo *
pre_init_prog ()
{
    ProgInfo	*tmp;
    char	*tmpdir;
    char	 tmp_filename [MAXPATHLEN];
    extern char	*ds_relname ();

    tmp = (ProgInfo *) calloc (1, sizeof (ProgInfo));
    
    tmp->rel = malloc (strlen (ds_relname ()) + 1);
    strcpy (tmp->rel, ds_relname ());

    tmp->hostname = malloc (MAXHOSTNAMELEN);
    gethostname(tmp->hostname, MAXHOSTNAMELEN);

    tmp->uid = geteuid();
    tmp->gid = getegid();
    
    tmp->news_opened = FALSE;
    tmp->xil_opened = FALSE;
    tmp->frame_mapped = FALSE;
    tmp->ce_okay = -1;   /* CE not initalized yet */
    tmp->def_ps_zoom = 100;

/*
 * Make temporary file names.
 */

    tmpdir = getenv (DGET("TMPDIR"));
    if (tmpdir == (char *) NULL)
       tmpdir = DGET ("/tmp");
         
    sprintf (tmp_filename, "%s/imagetool%d", tmpdir, getpid ());
    tmp->file_template = (char *) malloc (strlen (tmp_filename) + 1);
    strcpy (tmp->file_template, tmp_filename);

    tmp->def_printer = NULL;

    /* default to timeout after 60 seconds */
    tmp->timeout = 60;
    tmp->verbose = 0;
    return (tmp);
}

void
make_newsserver ()
{
    char		*xdisplay;
    char		*host;
    char		 tmp_server [256];
    unsigned int 	 port;
    struct hostent	*hp;

   
    xdisplay = DisplayString (ps_display->xdisplay);
    if (xdisplay[0] == ':') {
       host = malloc (strlen (prog->hostname) + 1);
       strcpy (host, prog->hostname);
       sscanf (xdisplay, ":%u", &port);
       } 
    else {
       host = malloc (strlen (xdisplay) + 1);
       sscanf (xdisplay, "%[^:]:%u", host, &port);
       if ((strcmp (host, LGET ("unix")) == 0) ||
           (strcmp (host, LGET ("localhost")) == 0)) {
	   free (host);
           host = malloc (strlen (prog->hostname) + 1);
           strcpy (host, prog->hostname);
	   }
       }

    port += 2000;
    hp = gethostbyname (host);
 
/*
 * Check if we got the info..
 */
 
    if (hp == (struct hostent *) NULL) {
       char *x;
       char  addr[4];
       int   i;

       x = strtok (host, ".");
       addr [0] = (char) atoi (x);
       for (i = 1; i < 4; i++) {
           x = strtok (NULL, ".");
           addr [i] = (char) atoi (x);
           }
 
       hp = gethostbyaddr (addr, 4, AF_INET);
       if (hp == (struct hostent *) NULL)  {
          fprintf(stderr, MGET("%s: Can't get host information.. aborting.\n"),
                                                         prog->name);
          exit(1);
          }
 
       free (host);
       host = malloc (strlen (hp->h_name) + 1);	
       strcpy (host, hp->h_name);
       } 
 
    sprintf(tmp_server, "%lu.%u;%s\n",
            ntohl(*(u_long *) hp->h_addr), port, host);

    prog->newsserver = malloc (strlen (tmp_server) + 1);
    strcpy (prog->newsserver, tmp_server);

/*
 * We can now also determine if we were started remotely.
 */

    if (strcmp (host, prog->hostname) != 0)
       prog->remote = TRUE;

    endhostent();
    free (host);
}

void
tool_info (vend, name, ver)
char    **vend;
char    **name;
char    **ver;
{
    extern char *xv_version;
    extern char *ds_vendname();

    *vend = (char *)strdup (ds_vendname());
    if (prog->name == (char *)NULL)
      *name = prog->name;
    else
      *name = prog->name;
    *ver = xv_version;
}

main(argc, argv)
int	argc;
char	**argv;
{
    char		*input_file;
    Xv_opaque	 	 server;
    extern int	  	 xerror_handler ();
    extern status_t	 load_from_tt ();
    extern status_t	 quit_tt ();
    int			 i;
    char                 bind_home[MAXPATHLEN];

/*
 *  Initialize program info..
 */

    prog = pre_init_prog ();

/*
 * Init ps variables
 */

    init_ps_vars ();

/*
 * Initialize xview defaults
 */

    xview_info = init_xview ();

/*
 * Need to check and see if we were started with several different
 * command line options before we do the Xview initialization (such
 * as -tooltalk, -depth, -geometry, etc.).
 */

    check_for_args (argc, argv);
 
/*
 * Do tooltalk init - only if started with -tooltalk command line option.
 */

    if (prog->tt_started == TRUE)
       dstt_check_startup (tool_info, &argc, &argv);

/*
 * Initialize XView.
 */

    server = xv_init (XV_INIT_ARGC_PTR_ARGV, &argc, argv, 
			 XV_USE_LOCALE, TRUE,
			 XV_X_ERROR_PROC, xerror_handler,
			 NULL);

    INSTANCE = xv_unique_key();
    DROPSITE = xv_unique_key();
    SELECTOR = xv_unique_key();

    ds_expand_pathname ("$OPENWINHOME/lib/locale", bind_home);
    bindtextdomain (MSGFILE_ERROR, bind_home);
    bindtextdomain (MSGFILE_LABEL, bind_home);
    bindtextdomain (MSGFILE_MESSAGE, bind_home);

/*
 * Finish initialization with prog/gettext.
 */
    complete_init_prog ();

/*
 * Create base frame and load pop up only...
 */

    base_window = BaseWindowObjectsInitialize (NULL, NULL);
#ifndef FILECHOOSER
    openf = OpenObjectsInitialize (NULL, base_window->base_window);
#endif

    input_file = parseargs (argc, argv);

/* 
 * Initialize the resoure database
 * before we create the canvas.
 */
    read_props();

/*
 * Initialize the Drag Drop package.
 */

    dragdrop_init (server); 
	
    set_labels (LGET ("(None)"));

    if (input_file != (char *) NULL) {
#ifdef FILECHOOSER
       if (openfc_callback (NULL, input_file, NULL, NULL) < 0)
	  if (ps_display->canvas == NULL)
            current_display = image_display;
#else
       set_load_args (input_file);
       if (open_file (openf->open_button, NULL) < 0)
	  current_display = image_display;
#endif
       }
    else
       current_display = image_display;

/*
 * Tooltalk stuff (if started by tooltalk):
 *   a: Set callbacks for desktop.
 *   b: Set callbacks for editing.
 *   c: Start receiving desktop messages.
 *   d: Start receiving edit messages.
 *   e: Start tt notifier.
 */
 
    if (prog->tt_started == TRUE) {
       dstt_xview_desktop_callback (base_window->base_window,
					QUIT,	       quit_tt,
					NULL);
       dstt_xview_desktop_register (base_window->base_window, 
					QUIT,	       TRUE,
					NULL);

       for (i = 0; i < ntypes; i++)
	   if (all_types[i].media_type != (char *) NULL) {
              dstt_editor_callback (all_types[i].media_type, DISPLAY, 
			            load_from_tt, NULL);
              dstt_editor_register (all_types[i].media_type, DISPLAY, TRUE, 
							     NULL);
              }
  
       dstt_xview_start_notifier ();

/*
 * Turn control over to XView.
 */

       xv_set (server, SERVER_SYNC_AND_PROCESS_EVENTS, NULL);
       XFlush (current_display->xdisplay);
       notify_start ();

       dstt_stopped ();
       }

    else
       xv_main_loop (base_window->base_window);

/*
    if (prog->xil_opened)
      xil_close (image_display->state);
*/

    close_ps (TRUE, TRUE);
    if (prog->datafile != (char *) NULL)
       unlink (prog->datafile);
    if (prog->compfile != (char *) NULL)
       unlink (prog->compfile);
    if (prog->uncompfile != (char *) NULL)
       unlink (prog->uncompfile);
    if (prog->printfile != (char *) NULL)
       unlink (prog->printfile);
    if (prog->rashfile != (char *) NULL)
       unlink (prog->rashfile);

    exit (0);
}

