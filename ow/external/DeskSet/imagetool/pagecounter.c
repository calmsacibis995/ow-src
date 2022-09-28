
#ifndef lint
static char sccsid[] = "@(#)pagecounter.c 1.5 93/08/25";
#endif

/*
 * Copyright (c) 1993 by Sun Microsystems, Inc.
 */

#include <stdlib.h>
#include <stdio.h>
#include <sys/param.h>
#include <sys/signal.h>
#include <X11/Xlib.h>
#include <DPS/dpsXclient.h>
#include "pagecounter_dps.h"
#include "tags.h"

#define	 PS_MAGIC	"%!"

/*
 * This application is used by imagetool. It is run via a fork/exec
 * and counts the number of pages in a postscript document. Also,
 * after each page, it informs the parent process so that the parent
 * can copy the rendered page onto a new pixmap (which is then used
 * on the page overview pop up).
 */

DPSContext		 dps_context;
GC			 dps_gc;
Display			*dpy;
Pixmap 			 pixmap;
char			 filename [MAXPATHLEN];
int			 pages = 0;
int			 done = 0;
int			 nargs;
struct itimerval	 timeout;
#ifdef DEBUG
FILE			*output_fd;
#endif
int			 stdin_fd;


/*
 * status_event_proc - event handler for the dps context. We only handle
 * 		       zombie and frozen events.
 */

void
status_event_proc (ctxt, code)
DPSContext ctxt;
int code;
{
    char input_buf [80];
    int  nbytes;
    extern void timeout_func ();

    if (code == PSFROZEN) {
       signal (SIGALRM, SIG_IGN);
       pages++;
#ifdef DEBUG
       fprintf (output_fd, "just found page: %d\n", pages);
#endif
       if (nargs == 6) {
          fwrite (RENDERED_PAGE_STRING, 1, strlen (RENDERED_PAGE_STRING), 
		  stdout);
          fflush (stdout);
	  nbytes = read (stdin_fd, input_buf, 80);
          if (strncmp (input_buf, CONTINUE_STRING, strlen (CONTINUE_STRING)) 
						!= 0) 
	     _exit (0);
	  }
       XDPSUnfreezeContext (dps_context); 
       signal (SIGALRM, timeout_func);
       setitimer (ITIMER_REAL, &timeout, (struct itimerval *) NULL);
       }
    else if (code == PSZOMBIE) 
       done = 1;
}

/*
 * text_proc - text handler for dps context. Handles any text returned
 *	       from the interpreter.
 */

void
text_proc (ctxt, buf, count)
DPSContext  ctxt;
char       *buf;
int         count;
{
    if (strncmp (buf, PAGES_STRING, strlen (PAGES_STRING)) ==  0) 
       done = 1;
}

/*
 * error_proc - error handler for the dps context. If we get any sort
 *		of error, just set done to true and return since we
 *		shouldn't be getting any sort of error!
 */

void
error_proc (ctxt, error_code, arg1, arg2)
DPSContext              ctxt;
DPSErrorCode            error_code;
long unsigned int       arg1;
long unsigned int       arg2;
{
    if (nargs == 2) {
       fprintf (stderr, "pagecounter: PostScript error found.\n");
       fprintf (stderr, 
		"             Number of pages reported may be incorrect.\n");
       }
    done = 1;
}

/*
 * exit_pagecounter - Called when we're going to exit.
 */

void
exit_pagecounter ()
{
    if (nargs == 6) {
       fwrite (PAGECOUNTER_END_STRING, 1, strlen (PAGECOUNTER_END_STRING), 
	       stdout);
       fflush (stdout);
       }
    XFreeGC (dpy, dps_gc);
    if (dps_context != (DPSContext) NULL)
       DPSDestroyContext (dps_context);

    if (nargs == 2) {
       XFreePixmap (dpy, pixmap);
       fprintf (stderr, "pagecounter found %d page(s) in %s\n", pages, 
		filename);
       exit (0);
       }
    else if (pages == 0)
       _exit (255);
    else
       _exit (pages);
}

/*
 * Xerror handler... it doesn't matter what error we get, just
 * return 255.
 */

int
xerror_handler (dpy, event)
Display		*dpy;
XErrorEvent	*event;
{
    if (nargs == 2) {
       char buffer[128];
       char mesg[128];
       char number[32];
       char *mtype = "XlibMessage";

       fprintf (stderr, "pagecounter: received XError.\n");
       XGetErrorText (dpy, event->error_code, buffer, 128);
       XGetErrorDatabaseText (dpy, mtype,  "XError",
                              "X Error (intercepted)", mesg, 128);
       fprintf (stderr, "%s:  %s\n  ", mesg, buffer);
       XGetErrorDatabaseText (dpy, mtype,  "MajorCode",
                              "Request Major code %d", mesg, 128);
       fprintf (stderr, mesg, event->request_code);
       sprintf (number, "%d", event->request_code);
       XGetErrorDatabaseText (dpy, "XRequest", number, "", buffer, 128);
       fprintf (stderr,  " (%s)" , buffer);
       fputs ("\n", stderr);
       XGetErrorDatabaseText (dpy, mtype, "MinorCode",
                              "Request Minor code", mesg, 128);
       fprintf (stderr, mesg, event->minor_code);
       fputs ("\n", stderr);
       XGetErrorDatabaseText (dpy, mtype, "ResourceID",
                              "ResourceID 0x%x", mesg, 128);
       fprintf (stderr, mesg, event->resourceid);
       fputs ("\n", stderr);
       XGetErrorDatabaseText (dpy, mtype, "ErrorSerial",
                              "Error Serial #%d", mesg, 128);
       fprintf (stderr, mesg, event->serial);
       fputs ("\n", stderr);
       XGetErrorDatabaseText (dpy, mtype, "CurrentSerial",
                             "Current Serial #%d", mesg, 128);
       fprintf (stderr, mesg, NextRequest (dpy));
       fputs ("\n", stderr);

       if (pages != 0)
	  fprintf (stderr, "\nFound %d pages before X error.\n", pages);

       exit (0);
       }

/*
 * If we were forked, return 255 to denote the error.
 */

    else
       _exit (255);
}

/*
 * SIGPIPE handler - just exit since we lost the connection with
 * imagetool.
 */

void
sigpipe_handler (sig)
int	sig;
{
    _exit (255);
}

/*
 * timeout_func - time out handler.
 */

void
timeout_func (sig)
int	sig;
{

/*
 * If we got a timeout, then up the number of pages by 1 (since we
 * timed out on one page) and return.
 */

    pages++;
    if (nargs == 6) {
       char input_buf [80];
       int nbytes;
       fwrite (RENDERED_PAGE_STRING, 1, strlen (RENDERED_PAGE_STRING), stdout);
       fflush (stdout);
       nbytes = read (stdin_fd, input_buf, 80);
       if (strncmp (input_buf, CONTINUE_STRING, strlen (CONTINUE_STRING)) != 0) 
	  _exit (0);
       }
    else {
       fprintf (stderr, "pagecounter: PostScript job timed out.\n");
       fprintf (stderr, 
		"             Number of pages reported may be incorrect.\n");
       }
    exit_pagecounter ();
}

/*
 * This application is invoked with the following command line arguments if
 * it is started from imagetool:
 *  	argv[0] - the name of this application.
 *	argv[1] - file name (full path)
 *	argv[2] - small pixmap id on which to render
 *	argv[3] - DisplayString (used in XOpenDisplay)
 *	argv[4] - page width (ie. 8.5 inches)
 *	argv[5] - page height (ie. 11.0 inches)
 *
 * If the user runs this from the command line, then all we need is the
 * filename:
 *  	argv[0] - the name of this application.
 *	argv[1] - file name (full path)
 *
 * We then create the gc, dps context, and away we go...
 */

int
main (argc, argv)
int	  argc;
char	**argv;
{
    unsigned long	 enable_mask, 
			 disable_mask, 
			 next_mask;
    double 		 page_width = 8.5;
    double		 page_height = 11.0;
    float		 ctm_a;
    int 		 xposn, 
			 yposn,
			 status,
			 filename_length;
    unsigned long	 pixmap_width = 51;
    unsigned long	 pixmap_height = 66;
    unsigned long	 pixmap_depth,
			 border_width,
			 gc_mask,
			 old_bg;
    Window		 win;
    XGCValues		 gc_vals;
    XStandardColormap	 gray_map,
			 rgb_map;
    FILE 		*fp;
    char 		*result;
    char 		 buf [2048];
    XEvent 		 event;

#ifdef DEBUG
    output_fd = fopen ("/tmp/pcc.out", "w");
    fprintf (output_fd, "opened file\n");
#endif
    nargs = argc;

    if ((argc != 6) && (argc != 2)) {
       fprintf (stderr, "Usage: pagecounter postscript-file\n");
       _exit (0);
       }

    strcpy (filename, argv[1]);

#ifdef DEBUG
    fprintf (output_fd, "filename: %s\n", filename);
    fprintf (output_fd, "argc is: %d\n", argc);
#endif
    if (argc == 6) {
       pixmap = strtoul (argv[2], (char **) NULL, 10);

       dpy = XOpenDisplay (argv[3]);

       if (dpy == (Display *) NULL) 
          _exit (0);
	
       XSetErrorHandler (xerror_handler);
#ifdef DEBUG
       fprintf (output_fd, "argv 4: %s  argv 5: %s\n", argv[4], argv[5]);
#endif

       page_width = atof (argv[4]);
       page_height = atof (argv[5]);
	
       status = XGetGeometry (dpy, pixmap, &win, &xposn, &yposn, &pixmap_width,
			      &pixmap_height, &border_width, &pixmap_depth);

#ifdef DEBUG
       fprintf (output_fd, "page_width: %f, page_height: %f\n", page_width, 
			    page_height);
       fprintf (output_fd, "pixmap width: %u, pixmap height: %u\n",
			    pixmap_width, pixmap_height);
#endif
       if (status == 0)
          _exit (0);
       if ((page_width < 0) || (page_height < 0))
	  _exit (0);
       }
    else {
       dpy = XOpenDisplay ((char *) NULL);
       if (dpy == (Display *) NULL) {
	  fprintf (stderr, "pagecounter: Couldn't open Display.\n");
	  exit (2);
	  }

       XSetErrorHandler (xerror_handler);
       pixmap = XCreatePixmap (dpy, RootWindow (dpy, DefaultScreen (dpy)), 
			       pixmap_width, pixmap_height, 1);
       if (pixmap == (Pixmap) NULL) {
	  fprintf (stderr, "pagecounter: Couldn't create Pixmap.\n");
	  exit (2);
	  }
       }

    stdin_fd = fileno (stdin);
     
    gray_map.colormap = None;
    gray_map.red_max = 1;
    gray_map.red_mult = -1;
    gray_map.base_pixel = 1;
    rgb_map.colormap = None;
    rgb_map.red_max = 0;
    rgb_map.green_max = 0;
    rgb_map.blue_max = 0;
    rgb_map.red_mult = 0;
    rgb_map.green_mult = 0;
    rgb_map.blue_mult = 0;
    rgb_map.base_pixel = 0;
    gc_mask = GCFunction;
    gc_vals.function = GXcopy;

    dps_gc = XCreateGC (dpy, pixmap, gc_mask, &gc_vals);

    if (dps_gc == (GC) NULL) {
       if (argc == 2) {
	  fprintf (stderr, "pagecounter: Couldn't create GC.\n");
	  exit (2);
	  }
       else
          _exit (0);
       }

    dps_context = XDPSCreateContext (dpy, pixmap, dps_gc, 0, 0, 0, &gray_map, 
				     &rgb_map, 0, text_proc, error_proc, NULL);

    if (dps_context == (DPSContext) NULL) {
       if (argc == 2) {
	  fprintf (stderr, "pagecounter: Couldn't create DPSContext.\n");
	  exit (2);
	  }
       else
          _exit (0);
       }

    XDPSSetEventDelivery (dpy, dps_event_pass_through);

    DPSSetContext (dps_context);

    enable_mask = PSZOMBIEMASK | PSFROZENMASK; 
    disable_mask = PSRUNNINGMASK | PSNEEDSINPUTMASK ;
    next_mask = 0;

    XDPSSetStatusMask (dps_context, enable_mask, disable_mask, next_mask);
    XDPSRegisterStatusProc (dps_context, status_event_proc);

    fp = fopen (filename, "r");
    if (fp == (FILE *) NULL) {
       if (argc == 2) {
	  fprintf (stderr, "pagecounter: Couldn't open %s.\n", filename);
	  exit (2);
	  }
       else
          _exit (0);
       }

    if (argc == 2) {
       while (1) {
          filename_length = strlen (filename);
          if ((filename [filename_length - 3] == '.') &&
	      (filename [filename_length - 2] == 'p') &&
	      (filename [filename_length - 1] == 's'))
	     break;
          result = fgets (buf, 2048, fp);
          if (strncmp (buf, PS_MAGIC, strlen (PS_MAGIC)) == 0) {
	     fseek (fp, 0, 0);
	     break;
	     }
          fprintf (stderr, 
		   "pagecounter: %s does not appear to be a postscript file.\n", 
		   filename);
          exit (2);
          }
       }

/*
 * Before we do anything, set a timeout value of 60 seconds (1 minutes)
 * Before we attempt to render each page, set the timer, and if the page
 * takes longer than 60 seconds, then there's probably some sort of error
 * in the file so it would timeout anyway.
 */

    (timeout.it_value).tv_sec = 60;
    (timeout.it_value).tv_usec = 0;
    (timeout.it_interval).tv_sec = 0;
    (timeout.it_interval).tv_usec = 0;

    ctm_a = ((page_width * 6.0) / 8.5) / 83.0;
    dps_setup (ctm_a, 0.0, 0.0, -ctm_a, 0.0, (float) pixmap_height, 
	       (int) (page_width * 83.0), (int) (page_height * 83.0), 83);

    dps_count_pages ();

#ifdef DEBUG
    fprintf (output_fd, "About to start loop\n");
    fflush (output_fd);
#endif
    while (1) {
       result = fgets (buf, 2048, fp);
       if (dps_context == (DPSContext) NULL) {
	  fclose (fp);
	  if (argc == 2) {
             fprintf (stderr, "pagecounter: Error writing to dps context\n");
             exit (2);
	     }
	  else
	     _exit (0);
	  }
       if (result == (char *) NULL)
	  break;
       DPSWritePostScript (dps_context, buf, strlen (buf));
       }

    fclose (fp);
    dps_stop_rendering ();
    DPSstop (dps_context);

/*
 * Set up signal handlers.. for alarm (timeout) and broken pipe (if nargs = 6).
 */

    if (nargs == 6)
       signal (SIGPIPE, sigpipe_handler);
    signal (SIGALRM, timeout_func);
    setitimer (ITIMER_REAL, &timeout, (struct itimerval *) NULL);

    while (done == 0) {
       XNextEvent (dpy, &event);
       if (XDPSIsDPSEvent (&event)) 
	  XDPSDispatchEvent (&event);
       }

    signal (SIGALRM, SIG_IGN);
    exit_pagecounter ();
}

