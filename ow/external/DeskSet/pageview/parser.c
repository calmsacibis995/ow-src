#ident "@(#)parser.c	3.9 93/08/25 PAGEVIEW SMI"

/*
 * Copyright (c) 1990 - Sun Microsystems Inc.
 */

/*
 * parser.c - parse PostScript for structuring conventions comments.
 */

#include <stdio.h>
#include <sys/types.h>
#include <ctype.h>
#include "pageview.h"
#include "emulator.h"
#include "emulator_dps.h"

long       *pagestart = NULL;	/* array of file pointer at %%Page comments */
long        EndProlog;		/* file pointer at the %%EndProlog comment */
long        BeginSetup;		/* file pointer at the %%BeginSetup comment */
long        EndSetup;		/* file pointer at the %%EndSetup comment */
long        Trailer;		/* file pointer at the %%Trailer comment */
int         NumberOfPages = 0;	/* number of pages */
int	    inEPSFDocument; 	/* bool to control EPSF comment semantics */
int         inImportedDocument;	/* bool to control EPSF comment semantics */

char	   *fixed_file = NULL;

extern int  frozen;

struct {
    char       *pattern;
    char       *fixprog;
    char       *cmdstring;
}           fixers[] = {
    {
	DGET("% FrameMaker PostScript Prolog 2.0"), DGET("fixframe"), 
	"%s < %s > %s"
    },
    {
	DGET("%  Copyright 1987 Interleaf, Inc."), DGET("fixinterleaf"), 
	"%s %s %s"
    },
};

#define NFIXERS sizeof fixers / sizeof fixers[0]


static void
FixFile(fixprog, cmdstring)
    char       *fixprog;
    char       *cmdstring;
{
    FILE       *tmpfp;
    char       *s;

    if (fixed_file == (char *) NULL)
       fixed_file = (char *) mktemp(DGET("/tmp/fixed.XXXXXX"));

    s = (char *) malloc(strlen(pathname) + strlen(fixed_file)
					 + strlen(fixprog) + 7);

    sprintf(s, cmdstring, fixprog, pathname, fixed_file);

    if (verbose) {
	fprintf(stderr, MGET("%s: trying to fix file using:\n %% %s\n"),
			ProgramName, s);
    }

    if (system(s)) {
	fprintf(stderr, MGET("%s: Fatal Error: Couldn't find %s.\n"),
			ProgramName, fixprog);
	unlink(fixed_file);
	free(s);
	return;
    }
    if ((tmpfp = fopen(fixed_file, "r")) == 0) {
	fprintf(stderr, MGET("%s: failed to fix document.\n"), ProgramName);
	return;
    }
    fclose(fp);
    fp = tmpfp;
    free(s);
}

/*
 * Scan the file looking for the structuring comments that should be in it.
 * I check for %%EndProlog, %%BeginSetup, %%EndSetup, %%Page, and %%Trailer.
 */
void
MakePageTable(ParsingFixedFile)
    int         ParsingFixedFile;
{
    int         i;
    int		done = FALSE;
    int		file_size;
    int		tmp_width, tmp_height;
    int		file_posn;
    char       *Title = NULL;  
			   /* the string after the %%Title:       comment */
    char       *Creator = NULL;   
			   /*  "     "     "    "  %%Creator:        "    */
    char       *CreatDate = NULL; 
			   /*  "     "     "    "  %%CreationDate:   "    */
    char       *For = NULL;   
			   /*  "     "     "    "  %%For:            "    */

    if (fp == 0)
	return;

    inImportedDocument = 0;
    inEPSFDocument = 0;
    EndProlog = -1;
    BeginSetup = -1;
    EndSetup = -1;
    Trailer = -1;

    if ((NumberOfPages > 0) && (pagestart != NULL))
       free (pagestart);

    NumberOfPages = 0;

    fseek(fp, 0, 2);
    file_size = ftell (fp);

    fseek(fp, 0, 0);
    fgets(line, sizeof(line), fp);
    if ((line[0] != '%') || (line[1] != '!')) {
	warning(MGET("It is missing '%!'"));
	for (i = 0; i < ((int) strlen(line)) - 1; i++) {
	    if (!isprint(line[i])) {
		complain(MGET("and it looks like a binary file."));
                xv_set (baseframe, FRAME_LEFT_FOOTER, 
                                MGET ("Unrecognized file type"), NULL); 
		footer_set = TRUE;
		break;
	    }
	}
    }
    while (fgets(line, sizeof(line), fp)) {
	if (line[0] != '%')
	    continue;
	if (line[1] == '!') {

	    if ((strncmp(line + 2, DGET("PS-Adobe-"), 9) == 0)
		    && (strncmp(line + 14, DGET(" EPSF-"), 6) == 0)) {
		inEPSFDocument++;
		if (verbose)
		    printf(MGET("***start of imported document: %s"), line);
	    } else {
		if (verbose)
		    warning(MGET("Found non-EPSF imported file, going on..."));
	    }
	} else if (line[1] == '%') {
	    /* Adobe Structuring Conventions Comment */

	    if (inEPSFDocument != 0) {
		if (strncmp(line, DGET("%%Trailer"), 9) == 0) {
		    inEPSFDocument--;
		    if (verbose)
			printf(MGET("***end of imported document\n"));
		}
	    } else if (inImportedDocument != 0) {
		if (strncmp(line, DGET("%%EndDocument"), 13) == 0) {
		    inImportedDocument--;
		    if (verbose)
			printf(MGET("***end of imported document\n"));
		}
	    } else {
		switch (line[2]) {
		case 'P':
		    if (strncmp(line + 3, DGET("age:"), 4) == 0) {

/*
 * If we're using the SHOWPAGE_METHOD, and we found a %%Page: comment
 * then, let's break out of this loop since we're going to check the
 * whole file for `showpages'.
 */

			if (pageview.method == SHOWPAGE_METHOD)
			   fseek (fp, 0, 2);
			else {
			   if (!NumberOfPages)
			      pagestart = (long *) malloc (sizeof(long));
			   else
			      pagestart = (long *) realloc (pagestart,
					 (NumberOfPages + 1) * sizeof(long));
			   pagestart[NumberOfPages] = ftell(fp);
			   NumberOfPages++;
			}
		    }
		    break;
		case 'E':
		    if (strncmp(line + 3, DGET("ndProlog"), 8) == 0)
			EndProlog = ftell(fp);
		    else if (strncmp(line + 3, DGET("ndSetup"), 7) == 0)
			EndSetup = ftell(fp);
		    break;
		case 'B':
		    if (strncmp(line + 3, DGET("eginDocument"), 12) == 0) {
		       inImportedDocument++;
		       if (verbose)
		          printf(MGET("***start of imported document: %s"), 
									line);
		       }
		    else if (strncmp(line + 3, DGET("eginSetup"), 9) == 0)
			BeginSetup = ftell(fp);
		    break;
		case 'T':
		    if (strncmp(line + 3, DGET("railer"), 6) == 0)
			Trailer = ftell(fp);
		    else if (strncmp(line + 3, DGET("itle:"), 5) == 0) {
			if (verbose) {
			    Title = (char *) malloc(strlen(line) - 7);
			    strcpy(Title, &line[8]);
			}
		    }
		    break;
		case 'C':
		    if (verbose) {
			if (strncmp(line + 3, DGET("reator:"), 7) == 0) {
			    Creator = (char *) malloc(strlen(line) - 9);
			    strcpy(Creator, &line[10]);
			} else if (strncmp(line + 3, DGET("reationDate:"), 12) 
									== 0) {
			    CreatDate = (char *) malloc(strlen(line) - 14);
			    strcpy(CreatDate, &line[15]);
			}
		    }
		    break;
		case 'F':
		    if (verbose) {
			if (strncmp(line + 3, DGET("or:"), 3) == 0) {
			    For = (char *) malloc(strlen(line) - 5);
			    strcpy(For, &line[6]);
			}
		    }
		    break;
		default:
		    break;
		}
	    }
	} else if (!ParsingFixedFile) {
	    int         i;
	    for (i = 0; i < NFIXERS; i++) {
		if (!strncmp(line, fixers[i].pattern,
			    strlen(fixers[i].pattern))) {
		    FixFile(fixers[i].fixprog, fixers[i].cmdstring);
		    MakePageTable(1);	/* recurse */
		    return;
		}
	    }
	}
    }

    if (pageview.method == SHOWPAGE_METHOD) {
	PageViewConn PSL2block;
	PageViewConn *PSL2conn = &PSL2block;
	int page_loc;
	int tag;
	char output [MAXOUTPUT + 1];
	unsigned long enable_mask, disable_mask, next_mask;
	PageViewConn *tmp = get_current ();

	enable_mask = PSZOMBIEMASK | PSFROZENMASK ;
	disable_mask = PSRUNNINGMASK | PSNEEDSINPUTMASK ;
	next_mask = 0;

	pagestart = (long *) malloc (sizeof (long));
	pagestart [0] = 0;

	if (dps == FALSE) {
	   if ((PSL2conn->input = ps_open_server (newsserver)) == NULL) {
	       fprintf (stderr, 
		        MGET("%s: No PostScript interpreter (NeWS/DPS) found!"),
			ProgramName);
	       exit(1);
	       }

	   PSL2conn->output = psio_getassoc(PSL2conn->input);

	   set_current (PSL2conn);
	   }
	else {
	   pageview_ps_close ();
	   if (mono == 1) {
	      XStandardColormap  gray_map;
              XStandardColormap  rgb_map;
 
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
					       &gray_map, &rgb_map, 0, NULL, 
                                               dps_error_handler, NULL);
	      }
	   else { 
	      XSync (dsp, 0);
	      dps_context = XDPSCreateContext (dsp, pixmap, pixmap_gc, 0, 0, 0, 
					       gray_cmap, rgb_cmap, 0, NULL,
                                               dps_error_handler, NULL);
	      }
           if (dps_context == NULL) {
              fprintf (stderr, MGET ("%s: Can't contact the DPS interpreter.\n"),
                                                        ProgramName);
              exit(1);
              }
           DPSSetContext (dps_context);
           XDPSSetStatusMask (dps_context, enable_mask, disable_mask, 
			      next_mask);
           XDPSRegisterStatusProc (dps_context, dps_status_handler);
	   }

	if (ParsingFixedFile) {
	   if (dps == FALSE) {
	      if (low_memory == TRUE)
	         ps_start_serverloop (win, ctm_a, ctm_b, ctm_c, ctm_d, ctm_tx, 
				      ctm_ty, SHOWPAGE_METHOD, fixed_file);
	      else
	         ps_start_serverloop (pixmap, ctm_a, ctm_b, ctm_c, ctm_d, 
				      ctm_tx, ctm_ty, SHOWPAGE_METHOD, 
				      fixed_file);
	      }
	   else {
	      tmp_width = (int) ( (float) pixw * 72.0 / (float) dpi );
              tmp_height = (int) ( (float) pixh * 72.0 / (float) dpi );
	      dps_start_serverloop (ctm_a, ctm_b, ctm_c, ctm_d, ctm_tx, 
				    ctm_ty, tmp_width, tmp_height, 
				    SHOWPAGE_METHOD);
	      }
	   }
	else {
	   if (dps == FALSE) {
	      if (low_memory == TRUE)
	         ps_start_serverloop (win, ctm_a, ctm_b, ctm_c, ctm_d, ctm_tx, 
				      ctm_ty, SHOWPAGE_METHOD, pathname);
	      else
	         ps_start_serverloop (pixmap, ctm_a, ctm_b, ctm_c, ctm_d,
				      ctm_tx, ctm_ty, SHOWPAGE_METHOD, 
				      pathname);
	      }
	   else {
	      if (low_memory == TRUE)
	         dps_start_serverloop (ctm_a, ctm_b, ctm_c, ctm_d, ctm_tx, 
				       ctm_ty, pixw, pixh, SHOWPAGE_METHOD);
	      else
	         dps_start_serverloop (ctm_a, ctm_b, ctm_c, ctm_d, ctm_tx, 
				       ctm_ty, pixw, pixh, SHOWPAGE_METHOD);
	      }				
	   }

	NumberOfPages = 0;

	if (dps == FALSE) {
	   while (done == FALSE) {
	      if (ps_peek_tag (&tag) <= 0) {
	         if (ParsingFixedFile)
	  	    unlink (fixed_file); 
	         warning (
		   MGET("Unknown response received from NeWS Server.. Aborting."));
	         exit (-42);
	         }

	      switch (tag) {
	         case DONE_TAG:
		      ps_skip_input_value ();
		      done = TRUE;
		      break;

	         case SHOWPAGE_TAG:
		      ps_getpageloc (&page_loc);
		      NumberOfPages++;
		      pagestart = (long *) realloc (pagestart,
					   (NumberOfPages + 1) * sizeof (long));	
		      pagestart [NumberOfPages] = page_loc;
		      break;

	         case ERROR_TAG:
		      ps_geterror (output);
		      append_output (output);
		      done = TRUE;
		      
	         case OUTPUT_TAG:
		      ps_getoutput (output);
		      append_output (output);
		      break;

	         default:
		      if (ParsingFixedFile)
		         unlink (fixed_file);
		      warning (
	        MGET("Unknown response received from NeWS Server.. Aborting."));
		      exit (-42);
		      	
	         }

	      }
              pageview_ps_close ();
	      set_current (tmp);
	   }

	else {
	   frozen = 0;
	   PumpBytes (0, file_size);
	   if (dps_context == (DPSContext) NULL)
	      goto DPSERROR;
	   DPSWritePostScript (dps_context, "stop\n", strlen ("stop\n"));
	   while (frozen == 0) {
	      sleep (1);
	      if (dps_context == (DPSContext) NULL)
	         goto DPSERROR;
	      XDPSGetContextStatus (dps_context);
	      }
	   if (dps_context == (DPSContext) NULL)
	      goto DPSERROR;
	   XDPSUnfreezeContext (dps_context);
	   while (frozen == 1) {
	      sleep (1);
	      if (dps_context == (DPSContext) NULL)
	         goto DPSERROR;
	      XDPSGetContextStatus (dps_context);
	      }
	   if (dps_context == (DPSContext) NULL)
	      goto DPSERROR;
	   XDPSUnfreezeContext (dps_context);
	   DPSWaitContext (dps_context);
	   if (dps_context != (DPSContext) NULL) {
              dps_synch (&done);
       	      if (done != DONE_TAG) {
                 warning(
               MGET("Unknown response received from DPS Server.. Aborting."));
                 exit(-42);
                 }  
	      dps_getpages (&NumberOfPages);
	      if (NumberOfPages > 0) {
                 pagestart = (long *) realloc (pagestart,
					   (NumberOfPages + 1) * sizeof (long));	
	         for (i = 1; i <= NumberOfPages; i++) {
		     if (dps_context == (DPSContext) NULL) {
			NumberOfPages = i - 2;
			if (NumberOfPages < 1)
			   NumberOfPages = 1;
			break;
			}
	             dps_getpageloc (i, &file_posn);
		     pagestart [i] = file_size - (file_posn - 5);
		     }
	         }
	      }
           pageview_ps_close ();
	   }
	}

DPSERROR:

    if (ParsingFixedFile)
	unlink (fixed_file);

    /* There were no %%Page comments so assume single page. */
    if (NumberOfPages == 0) {

/*
 * If we used the SHOWPAGE_METHOD, then we didn't see any showpages!
 * So, assume the end of the page is the end of the file.
 */

	if (pageview.method == SHOWPAGE_METHOD) {
	   NumberOfPages++;
	   pagestart = (long *) realloc (pagestart,
					 (NumberOfPages + 1) * sizeof (long));	
	   pagestart [NumberOfPages] = file_size ;
	   }

	else {
	   
	   pagestart = (long *) malloc(sizeof(long));
	   if (EndSetup > 0)
	       pagestart[NumberOfPages++] = EndSetup;
	   else if (EndProlog > 0)
	       pagestart[NumberOfPages++] = EndProlog;
	   else
	       pagestart[NumberOfPages++] = 0;
   
	   if (verbose)
	       warning(MGET("It is missing '%%Page' comment(s)"));
	
	   pgv_notice = NO_CONVENTIONS;
	   }
    }

    if (EndProlog < 0) 
	if (verbose)
	    warning(MGET("It is missing '%%EndProlog' comment"));

    if (pageview.method == SHOWPAGE_METHOD) {
       if (BeginSetup > EndProlog)
	  EndProlog = BeginSetup;

       if ((EndSetup > EndProlog) && (EndSetup > 0)){
	  if (EndSetup < pagestart[1])
	     pagestart[0] = EndSetup;
	  }
       else if (EndProlog > 0)
	  if (EndProlog < pagestart [1])
	     pagestart[0] = EndProlog;
       }

/*
 * I added this so that if the Prolog (or Setup) ends before the
 * start of the first page, we won't miss loading any postscript
 * that was placed (incorrectly) in between these two sections.
 * Also, if the Prolog ends before the start of the Setup, reset
 * the Prolog to end where the Setup starts so we don't miss any
 * Postscript in between the two.
 */


    if (BeginSetup > EndProlog)
       EndProlog = BeginSetup;

    if (EndSetup > EndProlog)
       EndSetup = pagestart [0];
    else
       EndProlog = pagestart [0];

/* 
 * Also, need to check if we got the BeginSetup, but not the EndSetup...
 */

    if ((BeginSetup > 0) && (BeginSetup > EndSetup))
       EndSetup = pagestart [0];

/* 
 * If we're not using the SHOWPAGE_METHOD, then we haven't yet set the
 * end-of-last-page marker so here we set it to %%Trailer or eof if missing. 
 */

    if (pageview.method != SHOWPAGE_METHOD) {
       pagestart = (long *) realloc(pagestart,
				 (NumberOfPages + 1) * sizeof(long));
       if (Trailer > 0)
	   pagestart[NumberOfPages] = Trailer;
       else {
	   fseek(fp, 0, 2);	/* eof */
	   pagestart[NumberOfPages] = ftell(fp);
	   if (verbose)
	       warning(MGET("It is missing '%%Trailer' comment"));
           }
       }

    if (verbose) {
	int         i;

	printf(MGET("%d Pages\n"), NumberOfPages);
	for (i = 0; i < NumberOfPages; i++)
	    printf(MGET("Page %d starts at offset %d\n"), i + 1, pagestart[i]);
	printf(MGET("Page %d ends at offset %d\n"),
	       NumberOfPages, pagestart[NumberOfPages]);

	if (Title) {
	    printf(MGET("Title: %s"), Title);
	    free(Title);
	}
	if (Creator) {
	    printf(MGET("Creator: %s"), Creator);
	    free(Creator);
	}
	if (CreatDate) {
	    printf(MGET("CreationDate: %s"), CreatDate);
	    free(CreatDate);
	}
	if (For) {
	    printf(MGET("For: %s"), For);
	    free(For);
	}
	printf(MGET("BeginSetup = %d\nEndSetup = %d\n"), BeginSetup, EndSetup);
	printf(MGET("EndProlog = %d\nTrailer = %d\n"), EndProlog, Trailer);
    }
}
