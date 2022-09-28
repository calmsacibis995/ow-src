#ifndef lint
static char *sccsid = "@(#)pageview.c 3.27 93/09/22";
#endif

/*
 * Copyright (c) 1990 by Sun Microsystems, Inc.
 */

/*
 *	PageView is a page oriented viewer for PostScript documents
 *	that follow Adobe PostScript document structuring conventions
 */

#include <stdio.h>
#include <sys/param.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>
#include <unistd.h>
#include <values.h>

#include "pageview.h"
#include "emulator.h"
#include "emulator_dps.h"
#include "patchlevel.h"
#include "ds_tooltalk.h"

#include <X11/Xproto.h>

extern char *getenv();
extern double atof();
extern char *pagenames[];
extern char *xv_version;

extern char *ds_relname ();
extern Notify_value tooltalk_handler ();
extern int new_depth;
extern int new_visual;
extern char *new_locale;

#define DPS_EXTENSION	"Adobe-DPS-Extension"
#define MAJORVERSION 3
#define MINORVERSION 0
#define DPI_VALUES   6

char *MSGFILE_ERROR = "SUNW_DESKSET_PAGEVIEW_ERR";
char *MSGFILE_LABEL = "SUNW_DESKSET_PAGEVIEW_LABEL";
char *MSGFILE_MESSAGE = "SUNW_DESKSET_PAGEVIEW_MSG";
char *NONAME;

char       *ProgramName;
char	   *PV_Name;
int	    tooltalk_start = FALSE;
int	    tooltalk_ready = TRUE;
int         verbose = 0;
int         mono;
int         initpage;
char       *envDPI;
int         maxColorDPI;
int         dpi;
int         depth;
float       pagewidth = 8.5;
float       pageheight = 11.0;
int         pixw;
int         pixh;
int	    dps = FALSE;
int	    frozen = 0;
char        newsserver[128];
char        hostname[MAXHOSTNAMELEN + 1];
Display    *dsp;
Xv_Server   My_server;
DPSContext  dps_context;
XStandardColormap       *gray_cmap = NULL;
XStandardColormap       *rgb_cmap = NULL;
int         screen;
GC          gc;
GC          pixmap_gc = NULL;
int         CurrentPage;	/* current page */
FILE       *fp;
char        pathname[1024];
char        iconname[1024];
char	    Directory[1024];
char	   *icon_label = NULL;
int         standardin;
char        line[MAX_LINE_LENGTH];
Pixmap      pixmap = (Pixmap) 0;
int	    AAscale = 0;
int	    dpi_values [DPI_VALUES] = {72, 85, 100, 150, 300, 400};
int	    footer_set = FALSE;
int	    handling_error = FALSE;
int	    low_memory = FALSE;
char	   *standardinfile = NULL;

struct itimerval timeout;
Notice_Types pgv_notice = NONE;

PageViewConn NeWSblock, PSL1block;
PageViewConn *NeWSconn = &NeWSblock, *PSL1conn = &PSL1block;

double ctm_a,
       ctm_b,
       ctm_c,
       ctm_d;

int  ctm_tx,
     ctm_ty;


#define FileName (standardin?DGET("Standard Input"):pathname)

void
complain(reason)
    char       *reason;
{
    append_output(reason);
    append_output(DGET("\n"));
}


void
warning(reason)
    char       *reason;
{
    append_output(DGET("\n"));
    append_output(FileName);
    append_output( MGET(" is a non-standard PostScript file\n"));
    append_output(reason);
    append_output(DGET("\n"));
}

int
pgv_x_error_proc (dpy, xerror)
Display *dpy;
XErrorEvent *xerror;
{
    char msg [80];

/*
 * Check to see if we got a BadAlloc error from the XCreatePixmap call.
 * If so, alert the user, and quit. We'd like to create a smaller 
 * pixmap, but since that doesn't work (see bug id 1068169), we just
 * quit.
 */

    if ((xerror->error_code == BadAlloc) &&
	(xerror->request_code == X_CreatePixmap)) {

       handling_error = TRUE;
       return 0;

       }


/*
 * If we got some other X error, just print it out, and exit.
 */

    XGetErrorText(dpy, xerror->error_code, msg, 80);
    fprintf(stderr, MGET("\nX Error (intercepted): %s\n"), msg) ;
    fprintf(stderr, MGET("Major Request Code   : %d\n"), xerror->request_code) ;
    fprintf(stderr, MGET("Minor Request Code   : %d\n"), xerror->minor_code) ;
    fprintf(stderr, MGET("Resource ID (XID)    : %u\n"), xerror->resourceid) ;
    fprintf(stderr, MGET("Error Serial Number  : %u\n"), xerror->serial) ;
 
    exit (5);
}

/*
 * PumpBytes - sends data from the file fp, offsets from start to end to
 * the PostScript connection.
 */
void
PumpBytes(start, end)
    register    start,
                end;
{
    char	*buf;
    int		 num,
		 nitems;

    if (verbose)
	fprintf(stderr, MGET("PumpBytes from %d to %d.\n"), start, end);

    fseek(fp, start, 0);

    if (dps == TRUE) {
       char	buf [2048];
       char    *result;
       nitems = end - start; 
       num = 0;
       while (num < nitems) { 

	  result = fgets (buf, 2048, fp);

/*
 * DPSWritePostScript might flush the buffer, which means that we
 * might get an error, so we should check to see if the context is
 * NULL before we do the write.
 */

	  if ((dps_context == (DPSContext) NULL) || (result == (char *) NULL) ||
	      (pgv_notice == PS_ERROR))
	     return;
	  if ((num + strlen (buf)) > nitems) 
	     buf [nitems - num] = '\0';
          DPSWritePostScript (dps_context, buf, strlen (buf)); 
	  num += strlen (buf);
 	  }
       } 
    else {
       while (start < end) {
	   psio_putc(getc(fp), PostScript); 
	   start++;
           }
       }

}

void
dps_status_handler (ctxt, code)
DPSContext ctxt;
int	   code;
{

/*
 * We only look for ZOMBIE events...
 */

    if (verbose) 
       fprintf (stderr, MGET ("%s: got ps status: %d\n"), ProgramName, code);

    if (code == PSZOMBIE) 
       pgv_notice = PS_ERROR;
    else if (code == PSFROZEN)
       frozen++;
}

void
dps_text_handler (ctxt, buf, count)
DPSContext  ctxt;
char	   *buf;
int	    count;
{
    char *tmp = malloc (count + 1);

    strncpy (tmp, buf, count);
    tmp[count] = '\0';
    append_output (tmp);
    if (verbose)
       fprintf (stderr, DGET ("%s"), tmp);
}

void
dps_error_handler (ctxt, error_code, arg1, arg2)
DPSContext		ctxt;
DPSErrorCode		error_code;
long unsigned int	arg1;
long unsigned int	arg2;
{
    char	*prefix = MGET ("%% [Error: ");
    char	*suffix = MGET (" ] %%\n");
    char	*infix = MGET (" Offending Command: ");
    char	*nameinfix = MGET ("User name too long; Name: ");
    char	*contextinfix = MGET ("Invalid context: ");
    char	*taginfix = MGET ("Unexpected wrap result tag: ");
    char	*typeinfix = MGET ("Unexpected wrap result type; tag: ");
    char 	*buf1;
    char	 buf2 [256];
    DPSBinObj	 ary;
    DPSBinObj	 elements;
    char 	*error_str;
    char	*error_name;
    int 	 error_count;
    int		 error_name_count;
    int  	 resync_flag;
    unsigned char tag;

    if (verbose) 
       fprintf (stderr, MGET ("%s: got ps error: %d\n"), ProgramName,
		error_code);

    switch (error_code) {
       case dps_err_ps: 
    	    buf1 = (char *) arg1;
 	    ary = (DPSBinObj) (buf1 + DPS_HEADER_SIZE);

  	    elements = (DPSBinObj) (((char *) ary) + ary->val.arrayVal);
	    error_name = (char *) (((char *) ary) + elements [1].val.nameVal);
	    error_name_count = elements [1].length; 

	    error_str = (char *) (((char *) ary) + elements [2].val.nameVal);
	    error_count = elements [2].length;

	    resync_flag = elements [3].val.booleanVal;

	    dps_text_handler (ctxt, prefix, strlen (prefix));
	    dps_text_handler (ctxt, error_name, error_name_count);
	    dps_text_handler (ctxt, infix, strlen (infix));
	    dps_text_handler (ctxt, error_str, error_count);
	    dps_text_handler (ctxt, suffix, strlen (suffix));
	
	    break;

       case dps_err_nameTooLong:
	    buf1 = (char *) arg1;
	    dps_text_handler (ctxt, prefix, strlen (prefix));
	    dps_text_handler (ctxt, nameinfix, strlen (nameinfix));
	    dps_text_handler (ctxt, buf1, arg2);
	    dps_text_handler (ctxt, suffix, strlen (suffix));
	    break;
	     
       case dps_err_invalidContext:
	    sprintf (buf2, DGET ("%s %s %d %s"), prefix, contextinfix, arg1,
		     suffix);
	    dps_text_handler (ctxt, buf2, strlen (buf2));
	    break;

       case dps_err_resultTagCheck:
	    tag = *((unsigned char *) arg1 + 1);
	    sprintf (buf2, DGET ("%s %s %d %s"), prefix, taginfix, tag, 
		     suffix);
	    dps_text_handler (ctxt, buf2, strlen (buf2));
	    break;

       case dps_err_resultTypeCheck:
	    tag = *((unsigned char *) arg1 + 1);
	    sprintf (buf2, DGET ("%s %s %d %s"), prefix, typeinfix, tag, 
		     suffix);
	    dps_text_handler (ctxt, buf2, strlen (buf2));
	    break;

       case dps_err_invalidAccess:
	    sprintf (buf2, MGET ("%s Invalid context access. %s"), prefix, 
		     suffix);
	    dps_text_handler (ctxt, buf2, strlen (buf2));
	    break;

       case dps_err_encodingCheck:
	    sprintf (buf2, MGET ("%s Invalid name/program encoding: %d/%d. %s"),
		     prefix, (int) arg1, (int) arg2, suffix);
	    dps_text_handler (ctxt, buf2, strlen (buf2));
	    break;
	   
       case dps_err_closedDisplay:
	    sprintf (buf2, MGET ("%s Broken display connection %d. %s"),
		     prefix, (int) arg1, suffix);
	    dps_text_handler (ctxt, buf2, strlen (buf2));
	    break;
       
       case dps_err_deadContext:
	    sprintf (buf2, MGET ("%s Dead context 0x0%x. %s"),
		     prefix, (int) arg1, suffix);
	    dps_text_handler (ctxt, buf2, strlen (buf2));
	    break;
       
       default:
	    buf1 = MGET ("Unknown error from DPS\n");
	    dps_text_handler (ctxt, buf1, strlen (buf1));
       
       }

    pgv_notice = PS_ERROR;
}

void
MakePaper()
{
    double      mag;
    XGCValues   gc_vals;
    unsigned long gc_mask;
    unsigned long enable_mask, disable_mask, next_mask;

    if (dps == TRUE) {
       enable_mask = PSZOMBIEMASK ;
       disable_mask = PSFROZENMASK | PSRUNNINGMASK | PSNEEDSINPUTMASK ;
       next_mask = 0;
       }

    if (pageview.aa && AAscale) {
	mag = (dpi*AAscale)/72.0;
	pixw = pagewidth * (dpi*AAscale);
	pixh = pageheight * (dpi*AAscale);
    } else {
	mag = dpi / 72.0;
	pixw = pagewidth * dpi;
	pixh = pageheight * dpi;
    }

    if (pageview.orient < 2) {
	int         tmp = pixw;
	pixw = pixh;
	pixh = tmp;
    }

    switch (pageview.orient) {
    case 0:			/* left */
	ctm_a = 0.0;
	ctm_b = ctm_c = -mag;
	ctm_d = 0.0;
	ctm_tx = pixw;
	ctm_ty = pixh;
	break;
    case 1:			/* right */
	ctm_a = 0.0;
	ctm_b = ctm_c = mag;
	ctm_d = 0.0;
	ctm_tx = ctm_ty = 0;
	break;
    case 2:			/* upsidedown */
	ctm_a = -mag;
	ctm_b = ctm_c = 0.0;
	ctm_d = mag;
	ctm_tx = pixw;
	ctm_ty = 0;
	break;
    case 3:			/* upright */
	ctm_a = mag;
	ctm_b = ctm_c = 0.0;
	ctm_d = -mag;
	ctm_tx = 0;
	ctm_ty = pixh;
	break;
    }

    if (verbose)
	fprintf(stderr, MGET("raster dims: %dx%d, %d dpi\n"), pixw, pixh, dpi);

    if (pageview.aa && AAscale)
	mono = 1;
    else
	mono = (dpi > maxColorDPI);

    if (pixmap) {
	XFreePixmap(dsp, pixmap);
	pixmap = (Pixmap) NULL;
	if ((dps == TRUE) && (dps_context != (DPSContext) NULL)) {
           DPSDestroySpace (DPSSpaceFromContext (dps_context));
	   dps_context = (DPSContext) NULL;
	   }
	}

    if (low_memory == FALSE) {
       pixmap = XCreatePixmap(dsp,
			      RootWindow(dsp, screen),
			      pixw,
			      pixh,
			      mono ? 1 : depth );
   
       XSync(dsp, 0);
       }
    else 
       xv_set (canvas, CANVAS_WIDTH,  pixw,
		       CANVAS_HEIGHT, pixh,
		       XV_WIDTH,      pixw,
		       XV_HEIGHT,     pixh,
		       NULL);

/*
 * We're handling an Xerror... quit!
 */

    if (handling_error == TRUE) {
       notice_prompt (baseframe, NULL,
	     NOTICE_MESSAGE_STRINGS,
		EGET ("Unable to allocate sufficient memory for display"),
		EGET ("at this time.  PageView cannot continue!"),
		NULL,
	     NOTICE_BUTTON_NO, LGET ("Ok"),
	     NULL);
       exit (5);
       }

/*
 * Clear out the new pixmap. This solves the problem where a file
 * has a postscript error and the last viewed page still is displayed.
 */

    if (low_memory == FALSE) {
       if (pixmap_gc != (GC) NULL)
	  XFreeGC (dsp, pixmap_gc);
	
       gc_vals.function = GXclear;
       gc_mask = GCFunction;
       pixmap_gc = XCreateGC (dsp, pixmap, gc_mask, &gc_vals);
       XFillRectangle (dsp, pixmap, pixmap_gc, 0, 0, pixw, pixh);
     
       if (dps == TRUE) {
	  if ((mono == 1) && (depth != 1)) {
	     XStandardColormap	gray_map;
	     XStandardColormap	rgb_map;

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
	     XSync (dsp, 0);
	     dps_context = XDPSCreateContext (dsp, pixmap, pixmap_gc, 0, 0, 0, 
					      &gray_map, &rgb_map, 0, 
					      dps_text_handler, 
					      dps_error_handler, NULL);
	     }
	  else {
	     XCopyGC (dsp, gc, GCForeground | GCBackground, pixmap_gc);
	     XSync (dsp, 0);
	     dps_context = XDPSCreateContext (dsp, pixmap, pixmap_gc, 0, 0, 0, 
					      gray_cmap, rgb_cmap, 0, 
					      dps_text_handler,
				              dps_error_handler, NULL); 
	     }
 	  if (dps_context == NULL) {
	     fprintf (stderr, MGET ("%s: Can't contact the DPS interpreter.\n"),
                                                        ProgramName);
             exit(1);
     	     }
	  DPSSetContext (dps_context);
   	  XDPSSetStatusMask (dps_context, enable_mask, disable_mask, next_mask);
	  XDPSRegisterStatusProc (dps_context, dps_status_handler);
	  }
       }
    else {
       XFillRectangle (dsp, win, low_memory_gc, 0, 0, pixw, pixh);
       if ((dps == TRUE) && (dps_context == NULL)) {
	  XSync (dsp, 0);
	  dps_context = XDPSCreateContext (dsp, win, low_memory_gc, 0, 0, 0, 
					   gray_cmap, rgb_cmap, 0, 
					   dps_text_handler, dps_error_handler,
					   NULL);
 	  if (dps_context == NULL) {
	     fprintf (stderr, MGET ("%s: Can't contact the DPS interpreter.\n"),
                                                        ProgramName);
             exit(1);
     	     }
	  DPSSetContext (dps_context);
   	  XDPSSetStatusMask (dps_context, enable_mask, disable_mask, next_mask);
	  XDPSRegisterStatusProc (dps_context, dps_status_handler);
	  }
       }

}

void
InitLaserWriter()
{
    char        banner[1024];
    int		tag;
    int		tmp_width,
		tmp_height;


    /*
     * We open 2 connections to the NeWS server. the first is
     * used to do the actual Adobe-compliant rendering of the
     * document. The second is used to access NeWS-specific features
     * such as Anti-Aliasing imaging and damage repair.
     */
    pageview_ps_close ();

    if (dps == FALSE) {
       if ((PSL1conn->input = ps_open_server(newsserver)) == NULL) {
	   fprintf(stderr, 
		   MGET("%s: No PostScript interpreter (NeWS/DPS) found!"),
		   ProgramName);
	   exit(1);
           }
       PSL1conn->output = psio_getassoc(PSL1conn->input);

    /*
     * Initialize the NeWS connection
     */
       set_current(NeWSconn);
       ps_AAscale(win, &AAscale);
       if (pageview.aa && AAscale) {
	   recreate_AAcanvas(ON);
	   ps_AAon(win);
       } else {
	   recreate_AAcanvas(OFF);
	   ps_AAoff(win);
       }
       ps_synch();
       if (ps_read_tag(&tag) <= 0) {
	  warning(
		MGET("Unknown response received from NeWS Server.. Aborting."));
          exit(-42);
          }
       }

    if (AAscale == 0) 
        props_AAinactivate();
    MakePaper ();

    /*
     * Initialize the PostScript connection and start the
     * psuedo-printer server loop.
     */

    if (dps == TRUE) {
       if (pageview.orient < 2) {
          tmp_width = (int) ( (float) pixh * 72.0 / (float) dpi );
          tmp_height = (int) ( (float) pixw * 72.0 / (float) dpi );
	  }
       else {
          tmp_width = (int) ( (float) pixw * 72.0 / (float) dpi );
          tmp_height = (int) ( (float) pixh * 72.0 / (float) dpi );
	  }
       dps_start_serverloop (ctm_a, ctm_b, ctm_c, ctm_d, ctm_tx, ctm_ty,
			     tmp_width, tmp_height, CONVENTIONS_METHOD);
       }
    else {
       set_current(PSL1conn);

       if (low_memory == TRUE)
          ps_start_serverloop(win, ctm_a, ctm_b, ctm_c, ctm_d, ctm_tx, ctm_ty,
			      CONVENTIONS_METHOD, pathname);
       else 
          ps_start_serverloop(pixmap, ctm_a, ctm_b, ctm_c, ctm_d, ctm_tx, 
			      ctm_ty, CONVENTIONS_METHOD, pathname);
       }

    sprintf(banner, 
	    MGET("\nPageView V%d.%d\nImaging: %s\nat %d dpi on %s paper\n"),
    	    MAJORVERSION, MINORVERSION, pathname, dpi, 
	    pagenames[pageview.pagesize]);
    append_output(banner);
    if (EndProlog > 0)
	PumpBytes(0, EndProlog);

    if (BeginSetup > 0) {
	if (EndSetup > 0) {
	    PumpBytes(BeginSetup, EndSetup);
	} else {
	    fprintf(stderr, MGET("%s: document missing %%%%EndSetup.\n"),
		    ProgramName);
	    exit(1);
	}
    }
}

void
dps_timeout_func (sig)
int	sig;
{
    char the_output [1024];

    if (verbose) {
       fprintf (stderr, MGET ("\n*** Job Timed out!\n"));
       fprintf (stderr, 
	        MGET ("*** Page %d may have a PostScript error such as a\n"),
	        CurrentPage);
       fprintf (stderr, 
	        MGET ("*** string or procedure that does not end.\n"));
       }
    notice_prompt (baseframe, NULL,
               NOTICE_MESSAGE_STRINGS,
                 EGET ("Job timed out!"),
                 EGET ("This page may have a PostScript error such as"),
                 EGET ("string or procedure that does not end."),
                 EGET (" "),
                 EGET ("You can change the job timeout value"),
                 EGET ("in the Edit->Properties PopUp window."),
                 EGET (" "),
		 EGET ("PageView cannot continue."),
                 NULL,
               NOTICE_BUTTON_NO, LGET ("Ok"),
               NULL);
    DPSDestroySpace (DPSSpaceFromContext (dps_context));
    dps_context = (DPSContext) NULL;
    exit (-42);
}

void
GotoPage(n)
    int         n;
{
    char         output[MAXOUTPUT + 1];
    char	 prolog_file[MAXPATHLEN];
    int          error;
    int          tag;
    int          done;
    int		 no_input = 0;
    int		 prolog_exist = 0;
    char	*line1 = LGET ("Use File->Load... to load a PostScript file");
    char	*line2 = LGET ("or use File Manager to drag and drop one from");
    char	*line3 = DGET ("$OPENWINHOME/share/images/PostScript");
    char	*locale;
#ifdef OW_I18N
    FILE	*fp_tmp;
#endif

    if (verbose)
	printf( MGET("Page: %d\n"), n);

    if (((PostScript == NULL) && (dps == FALSE)) ||
	((dps_context == NULL) && (dps == TRUE)))
	InitLaserWriter();

    CurrentPage = n;
    update_page_slider();
    set_right_footer(CurrentPage, NumberOfPages);

    if ((pgv_notice == PS_ERROR) && (dps == TRUE)) {
       DPSDestroySpace (DPSSpaceFromContext (dps_context));
       dps_context = (DPSContext) NULL;
       return;
       }

    if (dps == FALSE) 
       ps_start_page();
    else
       dps_start_page();

    if (fp == 0) {
        locale = (char *)xv_get(baseframe, XV_LC_DISPLAY_LANG);
#ifdef OW_I18N
	ds_expand_pathname("$OPENWINHOME/lib/locale/", prolog_file);
	strcat(prolog_file, locale);
	strcat(prolog_file, "/print/prolog.ps");
	if ( (fp_tmp = fopen(prolog_file, "r")) != NULL ) {
		prolog_exist = 1;
	}
	if ( strcmp("C", locale) && prolog_exist ) {
		/* Not in C locale */
        	if (dps == FALSE)  {
	 	   fclose (fp_tmp);
		   ps_no_page_i18n(line1, line2, line3, locale);
		   }
		else {
		   struct stat file_info;
		   int file_size;
		   FILE *real_fp = fp;
		
/*
 * PostScript `file' operator will be disabled.. therefore, must
 * read prolog file here, and send to dps.
 */

		   if (stat (prolog_file, &file_info) != 0)
		      file_size = MAXINT;
		   else
		      file_size = file_info.st_size;
		   
		   fp = fp_tmp;
		   PumpBytes (0, file_size);
		   fp = real_fp;
		   fclose (fp_tmp);
		   dps_no_page_i18n(line1, line2, line3);
		   }
	} else {
		/* Displaying in C locale, then do not include prolog.ps file */
	   	if (dps == FALSE)
		   ps_no_page(line1, line2, line3, locale);
		else
		   dps_no_page(line1, line2, line3, locale);
	}
#else
	if (dps == FALSE)
	   ps_no_page(line1, line2, line3, locale);
	else
	   dps_no_page(line1, line2, line3, locale);
#endif
    } else {
	if (reverse == TRUE)
	   PumpBytes(pagestart[NumberOfPages - n], 
		     pagestart[NumberOfPages - n + 1]);
	else
	   PumpBytes(pagestart[n - 1], pagestart[n]);
	}


    if (dps == FALSE) {
       ps_synch();

       while ((ps_check_input () == 0) && (no_input < pageview.timeout)) {
	   sleep (1);
	   no_input++;
	   }	

       if (no_input == pageview.timeout) {
          char the_output [1024];

          pageview_ps_close ();
          if (low_memory == FALSE)
             update_page();
          append_output (MGET ("\n*** Job Timed out!\n"));
          sprintf (the_output, 
	       MGET ("*** Page %d may have a PostScript error such as a\n"), 
	       CurrentPage);
          append_output (the_output);
          append_output (MGET ("*** string or procedure that does not end.\n")); 
          if (verbose) {
             fprintf (stderr, MGET ("\n*** Job Timed out!\n"));
             fprintf (stderr, 
	           MGET ("*** Page %d may have a PostScript error such as a\n"),
	           CurrentPage);
             fprintf (stderr, 
	           MGET ("*** string or procedure that does not end.\n"));
             }
          pgv_notice = TIMEOUT;
          return ;
          }
    
       done = False;
       while (!done) {
	   if (ps_peek_tag(&tag) <= 0) {
	      warning(
	 	MGET("Unknown response received from NeWS Server.. Aborting."));
	      exit(-42);
	      }
	   switch (tag) {
	   case DONE_TAG:
	       ps_skip_input_value();
	       if (low_memory == FALSE)
	          update_page();
	       done = True;
	       break;
	   case OUTPUT_TAG:
	       ps_getoutput(output);
	       append_output(output);
	       break;
	   case ERROR_TAG:
	       ps_geterror(output);
	       append_output(output);
	       pageview_ps_close ();
	       if (low_memory == FALSE)
	          update_page();
	       done = True;
	       pgv_notice = PS_ERROR;
	       break;
	   default:
	       warning(
		MGET("Unknown response received from NeWS Server.. Aborting."));
	       exit(-42);
	       }
	   }
       }

    else {
       if (pgv_notice == PS_ERROR) {
          DPSDestroySpace (DPSSpaceFromContext (dps_context));
          dps_context = (DPSContext) NULL;
          }
       else {
	  signal (SIGALRM, dps_timeout_func);
	  setitimer (ITIMER_REAL, &timeout, (struct itimerval *) NULL);
          DPSWaitContext (dps_context);
          if (pgv_notice == PS_ERROR) {
             DPSDestroySpace (DPSSpaceFromContext (dps_context));
             dps_context = (DPSContext) NULL;
             }
          else {
             dps_synch (&done);
             if (done != DONE_TAG) {
	        warning(
	        MGET("Unknown response received from DPS Server.. Aborting."));
	        exit(-42);
	        }
	     }
	  signal (SIGALRM, SIG_IGN);
	  }
       if (low_memory == FALSE)
	  update_page();
       }
}


void
CopyBytes(outfp, start, end)
    FILE       *outfp;
    int         start,
                end;
{
    fseek(fp, start, 0);
    while (start < end) {
	putc(getc(fp), outfp);
	start++;
    }
}


void
PrintPage(pfp, wholefile)
    FILE       *pfp;
    int         wholefile;
{
    if (wholefile) {
	if (verbose)
	    fprintf(stderr, MGET("%s: printing %s.\n"), ProgramName, FileName);
	CopyBytes(pfp, 0, pagestart[NumberOfPages]);
    } else {
	if (verbose)
	    fprintf(stderr, MGET("%s: printing page %d of %s.\n"),
		    ProgramName, CurrentPage, FileName);
	if (EndProlog > 0)
	    CopyBytes(pfp, 0, EndProlog);
	if (BeginSetup > 0) {
	    fprintf(pfp, MGET("%%%%BeginSetup\n"));
	    CopyBytes(pfp, BeginSetup, EndSetup);
	}
	fprintf(pfp, MGET("%%%%Page: %d\n"), CurrentPage);
	if (reverse == TRUE)
	   CopyBytes(pfp, pagestart[NumberOfPages - CurrentPage], 
		     pagestart[NumberOfPages - CurrentPage + 1]);
	else
	   CopyBytes(pfp, pagestart[CurrentPage - 1], pagestart[CurrentPage]);
    }
}


char       *
basename(path)
    char       *path;
{
    register char *p = strrchr(path, '/');

    if (p == 0)
	p = path;
    else
	p++;
    return p;
}


void
usage()
{
    fprintf(stderr, DGET("usage %s: %s\n\t\t%s\n\t\t%s\n\t\t%s\n"), ProgramName,
	    DGET("[-mono] [-w paperwidth] [-h paperheight] [-dpi dots/inch]"),
	    DGET("[-mcd colordensity] [-page pagenumber] [-dir directory]"),
	    DGET("[-left|right|upsidedown] [-aa] [-timeout timeout_value]"),
	    DGET("[-low_memory] [-usage] [-v (ersion)] [-verbose] [psfile | -] "));
}


void
checknextarg (next_arg, cmd_option)
char 	*next_arg;
char 	*cmd_option;
{
    if (next_arg != (char *) NULL) 
       if (next_arg[0] != '-')
	  return;

    fprintf (stderr, MGET("%s: Need value for command line option %s\n"), 
				ProgramName, cmd_option);
    exit (0);

}

void
check_for_tooltalk (argc, argv)
    int		argc;
    char       *argv [];
{
 
    char       *pv;
    char       *rel;
    int		i;

    if ((ProgramName = strrchr(argv[0], '/')) == 0)
	ProgramName = argv[0];
    else
	ProgramName++;

    rel = ds_relname ();
    pv = LGET ("PageView");
    PV_Name = (char *) malloc ( strlen (pv) +
				strlen (rel) + 2);

    sprintf (PV_Name, "%s %s", pv, rel);

    for (i = 0 ; i < argc  && tooltalk_start == FALSE ; i++) {
	if ( strcmp (argv[i], "-tooltalk") == 0)
	   tooltalk_start = TRUE;
	else if (strcmp (argv[i], "-verbose") == 0)
	   verbose = 1;
	else if (strcmp (argv[i], "-WL") == 0) {
	   checknextarg (argv [i+1], DGET("-WL"));
	   icon_label = (char *) malloc (strlen (argv[i+1]));
	   strcpy (icon_label, argv [++i]);
	   }
        }


/*
 * Initialize tooltalk environment. If we can't for some reason,
 * set tooltalk_ready to FALSE so we don't try and do any other
 * tooltalk operations.
 */

    if (tooltalk_start == TRUE) 
       if (ds_tooltalk_init (ProgramName, argc, argv) != 0) {
          if (verbose)
             fprintf (stderr, 
			MGET("%s couldn't init Tooltalk.\n"), ProgramName);
          tooltalk_ready = FALSE;
          }


/*
 * If we were started via tooltalk, and the init worked, then call
 * the tooltalk_handler to get the 'launch' message before we do
 * any xview initialization (in case the DISPLAY or LOCALE env. variables
 * need to be set).
 */
 
    if (tooltalk_start == TRUE)
       if (tooltalk_ready == TRUE) {
          tooltalk_handler ( (Notify_client) NULL, 0);
	  if ( (new_locale != (char *) NULL) &&
	       (new_depth != -1) && (new_visual != -1) )
	     ds_tooltalk_set_argv (argc, argv, new_locale, new_depth, 
					new_visual);	
	  }	   
       else {
	  fprintf (stderr, MGET("%s can't be started via tooltalk.\n"),
						ProgramName);
	  exit (1);
	  }
 
}

int
parseargs(argc, argv)
    int         argc;
    char       *argv[];
{
    int         i,
		j,
                n,
		size;
    char       *s;
    char       *rel;
    char       *pv;
    char       *arg_ptr;

    standardin = 0;
    envDPI = getenv(DGET("DPI"));
    dpi = (envDPI ? atoi(envDPI) : dpi_values [DEFAULT_DPI]);
    maxColorDPI = 100;

    mono = 0;
    initpage = 1;
    pageview.pagesize = DEFAULT_PAGESIZE;
    if (dpi != dpi_values [DEFAULT_DPI]) {
       for (j = 0; j < DPI_VALUES; j++)
           if (dpi == dpi_values [j]) {
              pageview.dpi = j;
	      break;
	      }
       }
    else
       pageview.dpi = DEFAULT_DPI;
    pageview.orient = DEFAULT_ORIENT;
    pageview.aa = DEFAULT_AA;
    pageview.timeout = DEFAULT_TIMEOUT;
    pageview.method = DEFAULT_METHOD;
    pathname[0] = 0;
    Directory[0] = 0;

    for (i = 1; i < argc; i++) {
	s = argv[i];
	if (s[0] == '-') {
	    n = strlen(s);
	    if (!strncmp("-", s, n))
		standardin++;
	    else if (!strncmp (DGET ("-v"), s, n)) {
		fprintf (stderr, MGET("%s version 3.0.%1d running on %s \n"),
					ProgramName, PATCHLEVEL, xv_version);
		exit (0);
	    } else if (!strncmp(DGET("-verbose"), s, n)) {
		verbose++;
	    } else if (!strncmp(DGET("-dpi"), s, n)) {
		checknextarg (argv[i+1], DGET("-dpi"));
		dpi = atoi(argv[++i]);
		for (j = 0; j < DPI_VALUES; j++)
		    if (dpi == dpi_values [j]) {
		       pageview.dpi = j;
		       break;
		       }
	    } else if (!strncmp(DGET("-mcd"), s, n)) {
		checknextarg (argv [i+1], DGET("-mcd"));
		maxColorDPI = atoi(argv[++i]);
	    } else if (!strncmp(DGET("-w"), s, n)) {
		checknextarg (argv [i+1], DGET("-w"));
		pagewidth = (float) atof(argv[++i]);
	    } else if (!strncmp(DGET("-h"), s, n)) {
		checknextarg (argv [i+1], DGET("-h"));
		pageheight = (float) atof(argv[++i]);
	    } else if (!strncmp (DGET("-dir"), s, n)) {
		checknextarg (argv [i+1], DGET("-dir"));
		strcpy (Directory, argv [++i]);
	    } else if (!strncmp(DGET("-low_memory"), s, n)) {
		low_memory = TRUE;
	    } else if (!strncmp(DGET("-left"), s, n)) {
		pageview.orient = 0;
	    } else if (!strncmp(DGET("-right"), s, n)) {
		pageview.orient = 1;
	    } else if (!strncmp(DGET("-upsidedown"), s, n)) {
		pageview.orient = 2;
	    } else if (!strncmp(DGET("-mono"), s, n)) {
		mono = 1;
	    } else if (!strncmp(DGET("-aa"), s, n)) {
		pageview.aa=1;
	    } else if (!strncmp(DGET("-page"), s, n)) {
		checknextarg (argv [i+1], DGET("-page"));
		initpage = atoi(argv[++i]);
		if (initpage < 1)
	 	   initpage = 1;
	    } else if (!strncmp(DGET("-timeout"), s, n)) {
		checknextarg (argv [i+1], DGET("-timeout"));
		pageview.timeout = atoi(argv[++i]);
	    } else if (!strncmp(DGET("-tooltalk"), s, n)) 
		continue;
	    else if (!strncmp(DGET("-usage"), s, n)) {
		usage();
		exit(0);
	    } else if (!strncmp(DGET("-version"), s, n)) {
		fprintf (stderr, MGET("%s version 3.0.%1d running on %s \n"), 
					ProgramName, PATCHLEVEL, xv_version);	
		exit (0);
	    } else {
		fprintf(stderr, MGET("%s: Illegal switch `%s'\n"),
			ProgramName, s);
		usage();
		exit(1);
	    }
	} else {
	    if (standardin)
		fprintf(stderr, MGET("%s: Ignoring `%s'\n"), ProgramName, s);
	    else
		strcpy(pathname, s);
	}
    }

    if (mono)
	maxColorDPI = 0;

    if (Directory[0] == NULL)
       if (getcwd (Directory, 1024) == (char *) NULL) {
	  fprintf (stderr, MGET ("%s:  Can't get current directory!\n"),
			  ProgramName);
	  exit(1);
	  }

    if (standardin) {
	standardinfile = (char *) mktemp(DGET("/tmp/ps.XXXXXX"));

	if ((fp = fopen(standardinfile, "w")) == NULL) {
	    fprintf(stderr, MGET("%s: Can't create temp file `%s'\n"),
		    ProgramName, standardinfile);
	    exit(1);
	}
	while (!feof(stdin)) {
	    fgets(line, MAX_LINE_LENGTH, stdin);
	    fputs(line, fp);
	}
	fclose(fp);
	fp = fopen(standardinfile, "r");
	strcpy(pathname, standardinfile);
    } else if (pathname[0]) {
	if (pathname[0] != '/') {
	   char *tmp_file = malloc (strlen (pathname) + 2);
	   strcpy (tmp_file, pathname);
	   sprintf (pathname,"%s/%s", Directory, tmp_file);
	   free (tmp_file);
	   }
	if ((fp = fopen(pathname, "r")) == 0) {
	    fprintf(stderr, MGET("%s: Can't open %s\n"), ProgramName, pathname);
	    exit(1);
	   }
    } else {
	strcpy(pathname, NONAME);
	CurrentPage = 1;
	NumberOfPages = 1;
	fp = 0;
    }

}

void
newfile(newfname, newfp)
    char       *newfname;
    FILE       *newfp;
{
    standardin = 0;
    if (newfname)
	strcpy(pathname, newfname);
    fclose(fp);
    fp = newfp;
    CurrentPage = 1;
    MakePageTable(0);
    pageview_ps_close ();
    reverse = FALSE;
    footer_set = FALSE;
    GotoPage(CurrentPage);
    if (low_memory == FALSE)
       home_page();
}

main(argc, argv)
    int         argc;
    char       *argv[];
{
    Frame       frame;
    char        host[128];
    char	bind_home [MAXPATHLEN];
    char       *xdisplay;
    struct hostent *hp;
    u_int       port;
    Rect	rect;
    Rect       *screen_rect;
  
/*
 *  Ignore SIGPIPE, since we may not be able to print to a printer,
 *  and we don't want to exit just because of that...
 */
 
    signal (SIGPIPE, SIG_IGN);

/*
 * Need to check and see if we were started with the -tooltalk option
 * before doing and XView initialization.
 */

    check_for_tooltalk (argc, argv);

    My_server = xv_init(XV_INIT_ARGC_PTR_ARGV, &argc, argv, 
			XV_USE_LOCALE,	TRUE,
			XV_X_ERROR_PROC, pgv_x_error_proc,
			NULL);
/*
 * Added to support internationalization...
 */

    ds_expand_pathname ("$OPENWINHOME/lib/locale", bind_home);
    bindtextdomain (MSGFILE_ERROR, bind_home);
    bindtextdomain (MSGFILE_LABEL, bind_home);
    bindtextdomain (MSGFILE_MESSAGE, bind_home);
    NONAME = LGET("(None)");

    parseargs(argc, argv);

    if ((low_memory == TRUE) && (pageview.aa == 1))
       low_memory = FALSE;

    UI1 = xv_unique_key();
    UI2 = xv_unique_key();
    UI3 = xv_unique_key();
    UI4 = xv_unique_key();
    UI5 = xv_unique_key();
    UI6 = xv_unique_key();
    UI7 = xv_unique_key();
    UI8 = xv_unique_key();
    UI9 = xv_unique_key();
    UI10 = xv_unique_key();
    UI11 = xv_unique_key();
    UI12 = xv_unique_key();
    UI13 = xv_unique_key();
    UI14 = xv_unique_key();

    frame = init_frame();

    xv_set (baseframe, FRAME_WM_COMMAND_ARGC_ARGV, (argc - 1), &argv [1], NULL);

/*
 * Do the gethostname even if dps is true, cause we might need it for dnd.
 */

    gethostname(hostname, MAXHOSTNAMELEN);

    if (dps == FALSE) {
       xdisplay = DisplayString(dsp);
       if (xdisplay[0] == ':') {
	   strcpy(host, hostname);
	   sscanf(xdisplay, ":%u", &port);
       } else {
	   sscanf(xdisplay, "%[^:]:%u", host, &port);
	   if (strcmp(host, LGET("unix")) == NULL)
	       strcpy(host, hostname);
       }
       port += 2000;
       hp = gethostbyname(host);
   
/*
 * Check if we got the info..
 */

       if (hp == (struct hostent *) NULL) {
	  int i;
          char *x;
          char  addr[4];
   
          x = strtok (host, ".");
          addr [0] = (char) atoi (x);
          for (i = 1; i < 4; i++) {
	      x = strtok (NULL, ".");
	      addr [i] = (char) atoi (x);
	      }

          hp = gethostbyaddr (addr, 4, AF_INET);
          if (hp == (struct hostent *) NULL)  {
	     fprintf(stderr, 
			MGET("%s: Can't get host information.. aborting.\n"),
							    ProgramName);
	     exit(1);
	     }

          strcpy (host, hp->h_name);
          }

       sprintf(newsserver, "%lu.%u;%s\n",
	       ntohl(*(u_long *) hp->h_addr), port, host);

       if (verbose)
	   fprintf(stderr, MGET("%s: connecting to %s and %s\n"),
		   ProgramName, DisplayString(dsp), newsserver);

    /*
     * Establish permanent NeWS connection. The PostScript rendering
     * connection is make later and is document-specific.
     */
       if ((NeWSconn->input = ps_open_server(newsserver)) == NULL) {
	  fprintf (stderr, 
		   MGET("%s: No PostScript interpreter (NeWS/DPS) found!"),
		   ProgramName);
	  exit(1);
          }
       NeWSconn->output = psio_getassoc(NeWSconn->input);
       set_current(NeWSconn);
       ps_initconnection();
       PostScript = PostScriptInput = NULL;
       endhostent ();
       }
    else {
       (timeout.it_value).tv_sec = pageview.timeout;
       (timeout.it_value).tv_usec = 0;
       (timeout.it_interval).tv_sec = 0;
       (timeout.it_interval).tv_usec = 0;
       }

    if (verbose)
	fprintf(stderr, MGET("%s: reading %s\n"), ProgramName, pathname);
    MakePageTable(0);
    CurrentPage = (initpage <= NumberOfPages) ? initpage : NumberOfPages;
    edit_file(pathname);
    GotoPage(CurrentPage);
    setactive ();
    set_icon_label(FileName);

/*
 * Set the callback for tooltalk messages if we init'ed tooltalk ok.
 */

    if ((tooltalk_ready == TRUE) && (tooltalk_start == TRUE)) {
       ds_tooltalk_set_callback (baseframe, tooltalk_handler);

/*
 * Place the window outside of the visible region. Get the screen
 * height and add 5.
 * Then, load the data, and move it to the correct location
 * before showing it.
 */

       frame_get_rect (baseframe, &rect);
       screen_rect = (Rect *) xv_get (baseframe, WIN_SCREEN_RECT);
       rect.r_top = screen_rect->r_height + 5;
       frame_set_rect (baseframe, &rect);
       }

    xv_main_loop(frame);

    unlink (dnd_file);
    if (standardinfile != NULL)
       unlink(standardinfile);
    unlink (edit_tmpfile);

/*
 * Send a departing message to any tooltalk senders.
 * If we didn't get any tooltalk messages, this won't send any
 * messages.
 */

    if ((tooltalk_ready == TRUE) && (tooltalk_start == TRUE)){
       ds_tooltalk_send_departing_message ();

/*
 * Exit tooltalk gracefully.
 */

       ds_tooltalk_quit ();
       }

    exit(0);
}

static PageViewConn *current_pgview_conn;

set_current(f)
PageViewConn *f;
{
    current_pgview_conn = f;
    PostScriptInput = f->input;
    PostScript = f->output;
}


PageViewConn *
get_current()
{
    return current_pgview_conn;
}

void
pageview_ps_close ()
{
    if (dps == FALSE) {
       if (PostScript != (PSFILE *) NULL) {
          ps_close_PostScript ();
          PostScript = (PSFILE *) NULL;
          current_pgview_conn->output = (PSFILE *) NULL;
          }
       }
    else {
       if (dps_context != (DPSContext) NULL) {
          DPSDestroySpace (DPSSpaceFromContext (dps_context));
          dps_context = (DPSContext) NULL;
	  }
       }
}

