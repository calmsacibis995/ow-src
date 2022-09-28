#ifndef lint
static char sccsid[] = "@(#)print.c 1.48 97/01/08";
#endif

/*
 * Copyright (c) 1997 by Sun Microsystems, Inc.
 */

#include <stdlib.h>
#include <sys/param.h>
#include "display.h"
#include "image.h"
#include "imagetool.h"
#include "state.h"
#include "ui_imagetool.h"
#include <xview/notice.h>
#include <xview/panel.h>

#define RASH_OPT	"rash "
#define WIDTH_OPT	"-W "
#define HEIGHT_OPT	"-H "
#define SCALE_OPT	"-s "
#define POSITION_OPT	"-l "
#define LANDSCAPE_OPT	"-r "
#define PRINT_OPT	"lp -s -Tpostscript "
#define PRINTER_OPT	"-d "
#define COPIES_OPT	"-n "
#define HEADER_OPT	"-o nobanner "
#define TITLE_OPT	"-t "

/*
 * Some of these are also defined in printui.c
 * because LGETs have to be in *.c files.
 */

#define DEFAULT_MARGIN   LGET ("1.0")
#define INCH_LEFT_MARGIN LGET ("inches from left")
#define CM_LEFT_MARGIN   LGET ("cm from left")
#define INCH_TOP_MARGIN  LGET ("inches from top")
#define CM_TOP_MARGIN    LGET ("cm from top")

#define SVR4_LIMIT	2048

extern float page_widths [];
extern float page_heights [];

typedef struct tnode{
      char *printer;
      struct tnode *left, *right;
} tnode;

static int      nprinters = 0;
static tnode   *root = NULL;
static int      delete_default = FALSE;

static int
set_lc_all (char *str)
{
   static char locale_str[128]; 

   if ((char *)getenv("LANG") == NULL)  /* ie. Eng. */
      return;
      
   if (str) {
      sprintf(locale_str, "%s=%s", "LC_ALL", str);
      if (putenv (locale_str)) 
         return 0;
   }
   else {  /* unset LC_ALL */
      sprintf(locale_str, "%s=%s", "LC_ALL", "");
      if (putenv(locale_str)) 
         return 0;
   }

}

tnode *
tree (p, printer, compare_func)
tnode	*p;
char	*printer;
int	(*compare_func)();
{
   char *cp, *strcpy();
   int cond;
 
   if (p == NULL) {
      p = (tnode*) malloc (sizeof (struct tnode));
      if ((cp = malloc (strlen (printer)+1)) != NULL)
         strcpy (cp, printer);
      p->printer = cp;
      p->left = p->right = NULL;
      }
   else 
      if ((cond = (*compare_func)(printer, p->printer)) == 0)
      ;
   else 
      if (cond < 0)
         p->left = tree (p->left, printer, compare_func);
   else
      p->right = tree (p->right, printer, compare_func);
       
   return(p);
}

/*
 * Retrieves output from lpstat -d
 * This call must be done after setting locale to C.
 */
static char *
get_default_printer()
{
    char   *ptr;
    FILE   *fp;
    char   *printer;
    char   *default_printer = NULL;

    /* get the default printer from lpstat -d command */
    /* the output looks like "system default destination: spitfire\n" */
    fp = popen("lpstat -d", "r");
    if (fp) {
      char message[80];
      fgets(message, sizeof(message), fp);
      if ((ptr = (char*)strchr(message, ':')) != NULL) {
	strtok(ptr, " ");
	printer = (char *)strtok((char *)NULL, "\n");
	if (printer)
	  default_printer = strdup (printer);
      }
      pclose(fp);
    } /* end if fp */

    return default_printer;

}

static void
initialize_printers()
{
    static int   lpstated = FALSE;
    int          def_printer_exists = False;
    FILE 	*fp;
    char	 buf [256];
    char	*first_printer = NULL;
    char	*printer;
    char	*ptr;
    char	*buf_ptr;

    if (lpstated == TRUE)
      return;

    set_lc_all ("C");
/*
 * First try to get default printer from
 * $PRINTER or $LPDEST.
 */
    if (!prog->def_printer) 
      prog->def_printer = (char *)getenv ("PRINTER");
   
    if (!prog->def_printer) 
      prog->def_printer = (char *)getenv ("LPDEST");
/*
 * Then try to get default printer from lpstat -d
 */
    if (!prog->def_printer) 
      prog->def_printer = get_default_printer();

/*
 * Read in all printers from 'lpstat -v' command.
 */
    fp = popen ("lpstat -v", "r");
    if (fp) {
      while (fgets (buf, sizeof (buf), fp) != NULL) {

	if ((buf_ptr = strstr(buf, "for ")) != NULL) {

	  strtok (buf_ptr, " ");
	  printer = (char *) strtok (NULL, " ");
	  if ((int) strlen (printer) >= 1 && printer[strlen (printer) - 1] == ':')
	    printer[strlen(printer) - 1] = NULL;
	  
	  if (printer) {
	    nprinters++;
	    
	    if (first_printer == NULL)
	      first_printer = strdup (printer);
	    
	    if (!def_printer_exists && (prog->def_printer) &&
		(strcoll (prog->def_printer, printer) == 0)) {
	      def_printer_exists = True;
	      /* continue; */
	    }
	    root = tree (root, printer, strcmp);
	  }
	}
      }  
      pclose (fp);
    } 
    
/*
 * Check if prog->def_printer actually is set up on the system or
 * if we have a def_printer configured.
 * If not, fall back on lpstat -d output or first printer that comes up
 * when doing an 'lpstat -v'.
 */
    if (def_printer_exists == False || prog->def_printer == NULL) {
      prog->def_printer = get_default_printer();
      if (prog->def_printer == NULL && first_printer)
        prog->def_printer = strdup (first_printer);
    }

    if (first_printer) 
      free (first_printer);

    set_lc_all (NULL);

    lpstated = TRUE;

}

int
printer_exists ()
{
    if (prog->def_printer != (char *) NULL)
       return (TRUE);
/*
 * There can be no default printer, but still have printers
 * configured on your system.
 */
    initialize_printers();

    if (nprinters > 0)
      return TRUE;
    else {
      notice_prompt (base_window->base_window, NULL,
                  NOTICE_MESSAGE_STRINGS, 
			EGET ("Print service has not been configured for this system.\nPlease contact your system administrator for information\non enabling print service."),
                        NULL,
                  NOTICE_BUTTON, LGET ("Continue"), 1,
                  NULL);
      return (FALSE);
    }
}

void
units_notify_proc (item, value, event)
Panel_item	 item;
int		 value;
Event		*event;
{

    if (value == INCHES) {
       xv_set (print->top_margin_text, 
			PANEL_LABEL_STRING, INCH_TOP_MARGIN,
			NULL);
       xv_set (print->left_margin_text,
			PANEL_LABEL_STRING, INCH_LEFT_MARGIN,
			NULL);
       }
    else {
       xv_set (print->top_margin_text, 
			PANEL_LABEL_STRING, CM_TOP_MARGIN,
			NULL);
       xv_set (print->left_margin_text,
			PANEL_LABEL_STRING, CM_LEFT_MARGIN,
			NULL);
       }

    set_margin_value (value);
/* 
 * Update print preview if showing.
 */
    if (print_preview && 
        (xv_get (print_preview->print_prev, XV_SHOW) == TRUE))
          position_image();
}

void
page_size_notify (item, string, client_data, op, event, row)
    Panel_item     item;
    char          *string;
    caddr_t        client_data;
    Panel_list_op  op;
    Event         *event;
    int            row;
{
/* 
 * Update print preview if showing.
 */
    if (op == PANEL_LIST_OP_SELECT &&
        print_preview && 
        (xv_get (print_preview->print_prev, XV_SHOW) == TRUE))
          position_image();
}
   
void
size_notify_proc (item, event)
Panel_item	 item;
Event		*event;
{
    int value = xv_get (item, PANEL_VALUE);

    xv_set (print->size_slider, PANEL_VALUE, value, NULL);
/* 
 * Update print preview if showing.
 */
    if (print_preview && 
        (xv_get (print_preview->print_prev, XV_SHOW) == TRUE))
          position_image();
}

void
print_slider_notify_proc (item, value, event)
Panel_item	 item;
int		 value;
Event		*event;
{
    xv_set (print->size, PANEL_VALUE, value, NULL);
/* 
 * Update print preview if showing.
 */
    if (event_is_up (event)) {

      if (print_preview && 
          (xv_get (print_preview->print_prev, XV_SHOW) == TRUE))
            position_image();
    }
}

void
orientation_notify_proc (item, value, event)
Panel_item	 item;
int		 value;
Event		*event;
{
/* 
 * Update print preview if showing.
 */
    if (print_preview && 
        (xv_get (print_preview->print_prev, XV_SHOW) == TRUE))
          position_image();
}

void
position_notify_proc (item, value, event)
Panel_item	 item;
int		 value;
Event		*event;
{

    if (value == CENTERED) {
       xv_set (print->left_margin, PANEL_INACTIVE, TRUE, NULL);
       xv_set (print->left_margin_text, PANEL_INACTIVE, TRUE, NULL);
       xv_set (print->top_margin, PANEL_INACTIVE, TRUE, NULL);
       xv_set (print->top_margin_text, PANEL_INACTIVE, TRUE, NULL);
       xv_set (print->units, PANEL_INACTIVE, TRUE, NULL);
       }
    else {
       xv_set (print->left_margin, PANEL_INACTIVE, FALSE, NULL);
       xv_set (print->left_margin_text, PANEL_INACTIVE, FALSE, NULL);
       xv_set (print->top_margin, PANEL_INACTIVE, FALSE, NULL);
       xv_set (print->top_margin_text, PANEL_INACTIVE, FALSE, NULL);
       xv_set (print->units, PANEL_INACTIVE, FALSE, NULL);
       }

/* 
 * Update print preview if showing.
 */
    if (print_preview && 
        (xv_get (print_preview->print_prev, XV_SHOW) == TRUE))
          position_image();
}

void
page_range_notify_proc (item, value, event)
Panel_item	 item;
int		 value;
Event		*event;
{

/*
 * If new choice is "This page (as image)" turn on all of the image
 * ops, turn off page order.
 */

    if (value == 0) {
       xv_set (print->orientation, PANEL_INACTIVE, FALSE, NULL);
       xv_set (print->size, PANEL_INACTIVE, FALSE, NULL);
       xv_set (print->size_text, PANEL_INACTIVE, FALSE, NULL);
       xv_set (print->size_slider, PANEL_INACTIVE, FALSE, NULL);
       xv_set (print->size_percent, PANEL_INACTIVE, FALSE, NULL);
       xv_set (print->position, PANEL_INACTIVE, FALSE, NULL);
       if (xv_get (print->position, PANEL_VALUE) == MARGINS) {
          xv_set (print->top_margin, PANEL_INACTIVE, FALSE, NULL);
          xv_set (print->top_margin_text, PANEL_INACTIVE, FALSE, NULL);
          xv_set (print->left_margin, PANEL_INACTIVE, FALSE, NULL);
          xv_set (print->left_margin_text, PANEL_INACTIVE, FALSE, NULL);
          xv_set (print->units, PANEL_INACTIVE, FALSE, NULL);
          } 
       }

/*
 * if new choice is all, turn off image ops, turn on page range
 */

    else {   /* value == 1 */
       xv_set (print->orientation, PANEL_INACTIVE, TRUE, NULL);
       xv_set (print->size, PANEL_INACTIVE, TRUE, NULL);
       xv_set (print->size_text, PANEL_INACTIVE, TRUE, NULL);
       xv_set (print->size_slider, PANEL_INACTIVE, TRUE, NULL);
       xv_set (print->size_percent, PANEL_INACTIVE, TRUE, NULL);
       xv_set (print->position, PANEL_INACTIVE, TRUE, NULL);
       xv_set (print->top_margin, PANEL_INACTIVE, TRUE, NULL);
       xv_set (print->top_margin_text, PANEL_INACTIVE, TRUE, NULL);
       xv_set (print->left_margin, PANEL_INACTIVE, TRUE, NULL);
       xv_set (print->left_margin_text, PANEL_INACTIVE, TRUE, NULL);
       xv_set (print->units, PANEL_INACTIVE, TRUE, NULL);
       }

}

void
margin_notify_proc (item, event)
Panel_item	 item;
Event		*event;
{
    double	 x;
    char 	*value = (char *) xv_get (item, PANEL_VALUE);

    if (check_float_value (value, &x) != 0)
       xv_set (item, PANEL_VALUE, DEFAULT_MARGIN, NULL);
    panel_advance_caret (print->controls);
/* 
 * Update print preview if showing.
 */
    if (print_preview && 
        (xv_get (print_preview->print_prev, XV_SHOW) == TRUE))
          position_image();
}

void
print_cancel_notify_proc (item, event)
Panel_item	 item;
Event		*event;
{
    xv_set (print->print, 
		FRAME_CMD_PUSHPIN_IN, FALSE,
		XV_SHOW,	      FALSE,
		NULL);
}

void
get_print_options (left_margin, top_margin, centered, orientation, scale, 
		   page_range)
double	*left_margin;
double	*top_margin;
int	*centered;
int	*orientation;
float	*scale;
int	*page_range;
{
    char	*value;
    double	 x;
    int		 size;

    *centered = CENTERED;
    *left_margin = 1.0;
    *top_margin = 1.0;
    *scale = 1.0;
    *page_range = ALL_PAGES;
    *orientation = PORTRAIT;

    if (print != (PrintObjects *) NULL) {
       if (xv_get (print->page_range, PANEL_INACTIVE) == FALSE)
	  *page_range = xv_get (print->page_range, PANEL_VALUE);
	  
       if (xv_get (print->orientation, PANEL_INACTIVE) == FALSE) 
	  *orientation = xv_get (print->orientation, PANEL_VALUE);
 
       if (xv_get (print->size, PANEL_INACTIVE) == FALSE) {
	  size = xv_get (print->size, PANEL_VALUE);
	  *scale = ( (float) size) / 100.0;
	  }

       if (xv_get (print->position, PANEL_INACTIVE) == FALSE) {
 	  *centered = xv_get (print->position, PANEL_VALUE);
	  if (*centered == MARGINS) {
	     value = (char *) xv_get (print->top_margin, PANEL_VALUE);

             if (check_float_value (value, &x) != 0)
       	 	xv_set (print->top_margin, PANEL_VALUE, DEFAULT_MARGIN, NULL);
	     else
	        *top_margin = x;

	     value = (char *) xv_get (print->left_margin, PANEL_VALUE);

             if (check_float_value (value, &x) != 0)
       	 	xv_set (print->left_margin, PANEL_VALUE, DEFAULT_MARGIN, NULL);
	     else
	        *left_margin = x;
	
	     if (xv_get (print->units, PANEL_VALUE) == CM) {
		*top_margin /= 2.54;
		*left_margin /= 2.54;
		}
	     }
	  }
       }
}
   
char *
get_printer_options (pagewidth, pageheight, copies, header) 
float	*pagewidth;
float	*pageheight;
int	*copies;
int	*header;
{
    int	 	 row;
    char	*printer;

    initialize_printers();
    printer = prog->def_printer;
    *pagewidth = page_widths [DEFAULT_PAGE_SIZE];
    *pageheight = page_heights [DEFAULT_PAGE_SIZE];
    *copies = 1;
    *header = HEADER_PAGE;
    
    if (print != (PrintObjects *) NULL) {
       row = current_list_selection (print->printer);
       if (row == -1)
	 return NULL;
       printer = (char *) xv_get (print->printer, PANEL_LIST_STRING, row);

       row = current_list_selection (print->page_size);
       *pagewidth = page_widths [row];
       *pageheight = page_heights [row];

       *copies = xv_get (print->copies, PANEL_VALUE);
       *header = xv_get (print->header, PANEL_VALUE);
       }
    else if (root != NULL && printer == NULL) {
/*
 * Pick first printer from list here.
 */
      printer = root->printer;
    }

    return (printer);
}

void
print_error (image)
ImageInfo	*image;
{
     display_error (base_window->base_window, 
	            EGET ("Print request failed: Could not create temporary file for printout."));
	  
     if (image != (ImageInfo *) NULL) {
        image->cmap = NULL;
        destroy_image (image);
     }

     setactive ();
}
  
void
print_button_notify_proc (item, event)
Panel_item	 item;
Event		*event;
{
    double 	 	 left_margin;
    double 	 	 top_margin;
    double		 bottom_margin;
    double	  	 scale_height;
    double	 	 scale_width;
    float		 scale_factor;
    float		 tmpf;
    float		 page_height;
    float		 page_width;
    int			 orientation;
    int			 position;
    int			 range;
    int		 	 copies;
    int			 header;
    char		*printer;
    char		*file_to_print;
    char		 tmp_buf [80];
    char                 error [1024];

    FILE		*popen_file;
    FILE		*out_file;
    int			 bytes_to_write;
    int		 	 bytes_written;
    char		 write_buf [SVR4_LIMIT];

    char		 print_cmd [2048];
    char		 print_cmd2 [2048];
    char		*ow;
    ImageInfo		*tmp_image = NULL;  
    unsigned int	 width, height, nbands;
    XilDataType	 	 datatype;
    float		 mult [1], offset [1];
    int			 status;
    int			 print_as_image = TRUE;
    int			 i, j = 0;

    setbusy ();

/* 
 * Determine if we supposed to print a ps file or print as an image.
 */

    get_print_options (&left_margin, &top_margin, &position, &orientation,
                       &scale_factor, &range);

    printer = get_printer_options (&page_width, &page_height, &copies, &header);
    if (printer == NULL && nprinters == 0) {
        notice_prompt (base_window->base_window, NULL,
                   NOTICE_MESSAGE_STRINGS,
                   EGET ("Print service has not been configured for this system.\nPlease contact your system administrator for information\non enabling print service."),
                   NULL,
                   NOTICE_BUTTON, LGET ("Continue"), 1,
                   NULL);
	setactive();
        return;
     }

/*
 * If a postscript doc, and printing all pages, just print the file.
 * If only a selected page, then prepend some ps to the front of the
 * the file which will cause only the selected page to get printed out.
 * Note that if it's only one page, then no other setting must have been
 * done (ie. no scale/no margins/etc.).
 */

    if (current_display == ps_display) {
       if (range != ALL_PAGES) {
          if (((position == CENTERED) ||
	       ((left_margin == 0.0) && (top_margin == 0.0))) &&
	      (scale_factor == 1.0)) {

/*
 * Ok, we can just add out "extra" stuff to the file and print it out
 */

	     mmap_handle_t	*file_ptr;

	     if (prog->printfile == (char *) NULL)
	        make_tmpfile (&(prog->printfile), DGET ("print"));
	     if (current_image->compression == UNIX)
	        file_ptr = fast_read (prog->uncompfile);
	     else	
	        file_ptr = fast_read (current_image->realfile);
	     status = fast_write (prog->printfile, file_ptr->addr, 
		file_ptr->len, current_state->reversed ?
		current_image->pages - current_state->current_page + 1:
		current_state->current_page);
	     fast_close (file_ptr);
	     if (status != 0) {
	        print_error (NULL);
	        return;
	        }
   
	     file_to_print = prog->printfile;
	     print_as_image = FALSE;
	     }
	  }
       else
	  print_as_image = FALSE;
       }
	  
/*
 * If we're not printing all pages (ie. not a ps file with the the
 * print all pages selection made) , then do all of this to determine
 * where to print the image on the page.
 */

    if (print_as_image == TRUE) {

/*
 * Create a tmp_image (ImageInfo *) and fill correctly. Then do the
 * rast_save to save out a temporary raster file in /tmp (check 
 * TMPDIR first).  Then do the rash and pipe to lp.
 */

/*
 * First, check if the current file has been altered... if not
 * then we can just use that file (if a sun raster file), and forget about 
 * the tmp file. Use prog->printfile.
 */

       if ((current_image->realfile != (char *) NULL) &&
	   ((current_state->undo).op == NO_UNDO) &&
	   (current_image->type_info->type == RASTER)) 

/*
 * Even though its a raster file, there is a chance that we could have
 * data, compressed data or a compressed file. Therefore, call file_to_open
 * to get the right file (it will write out the file and/or uncompress if
 * necessary).
 */
	  file_to_print = (char *) file_to_open (current_image->file,
						 current_image->realfile,
						 current_image->compression,
						 current_image->data,
						 current_image->file_size);
       else {
	  if (prog->printfile == (char *) NULL)
	     make_tmpfile (&(prog->printfile), DGET ("print"));
          tmp_image = init_image (prog->printfile, prog->printfile, 0, 
				  str_to_ftype (DGET ("sun-raster")), 
				  (char *) NULL);

          tmp_image->depth = current_display->depth;

/*
 * If we're viewing a ps file, then have to save it out to an xil image.
 */

	  if (current_display == ps_display) {
	     if (prog->xil_opened == FALSE) {
	       base_window_image_canvas_objects_create (base_window);
	       prog->xil_opened = TRUE;
	     }
	     tmp_image->orig_image = create_image_from_ps (tmp_image);
	     if (current_display->depth == 8 || current_display->depth == 4)
       	       save_colormap (tmp_image);
       	       compress_colors (tmp_image);
	     }
	  else {
	     u_char       *tmp_data;
	     unsigned int  pixel_stride, scanline_stride;

             copy_image_data (tmp_image, current_image);
             xil_get_info (current_image->view_image, &width, &height,
			      &nbands, &datatype);
             tmp_image->orig_image = xil_create (image_display->state, width,
					         height, nbands, datatype);
	     tmp_data = retrieve_data (tmp_image->orig_image, &pixel_stride,
				       &scanline_stride);
	     xil_import (tmp_image->orig_image, FALSE);
	     tmp_image->bytes_per_line = scanline_stride;
             xil_copy (current_image->view_image, tmp_image->orig_image);
             if (tmp_image->offset != 0) {
	        mult [0] = 1.0;
	        offset [0] = (float) tmp_image->offset * -1.0;
	        xil_rescale (tmp_image->orig_image, tmp_image->orig_image, mult,
			      offset);
                }
	     }
          
          status = rast_save (tmp_image); 
          if (status != 0) {
	     print_error (tmp_image);
	     return;
	     }

	  file_to_print = prog->printfile;
          }

/*
 * Start building command ...
 */

       ow = getenv (DGET ("OPENWINHOME"));
       if (ow == (char *) NULL)
          ow = DGET ("/usr/openwin");

#ifdef JOE
       sprintf (print_cmd, "%/home/joew/DeskSet/V3.3/external/DeskSet/snapshot/rash-g ");
#else
       sprintf (print_cmd, "%s/bin/%s", ow, RASH_OPT);
#endif

/*
 * Check orientation next, cause that affects top and left margins.
 */

       if (orientation == LANDSCAPE) {
          tmpf = page_height;
          page_height = page_width;
          page_width = tmpf;
          strcat (print_cmd, LANDSCAPE_OPT);
          }

/*
 * Check if centered, or positioned
 */

       if (position != CENTERED) {

/*
 * Figure out what bottom margin is.. We always do everything in inches
 * for now...
 */
          bottom_margin = page_height - top_margin - 
			  (current_image->height / current_display->res_y);
   
          sprintf (tmp_buf, "%s%5.2fin %5.2fin ", POSITION_OPT,
		  	     left_margin, bottom_margin);

          strcat (print_cmd, tmp_buf);
          }

/*
 * Get scale factors, or absolute height/width...
 * Note: Must have scale factor in here for this to work!
 */

       scale_height = current_image->height / current_display->res_y * scale_factor;
       scale_width = current_image->width / current_display->res_y * scale_factor;

       sprintf (tmp_buf, "%s%5.2fin %5.2fin ", SCALE_OPT, scale_width, 
				   scale_height);
       strcat (print_cmd, tmp_buf);

/*
 * Add file to be rash-ed
 */

       strcat (print_cmd, file_to_print);

/*
 * Ok, create temp file for rash output and append file names to command.
 */

       if (prog->rashfile == (char *) NULL)
          make_tmpfile (&(prog->rashfile), DGET ("rash"));

/*
 * Check for any "&" in string in case filename has a '&' in it.
 * Place a '\'in front if the '&'.
 */
       j = 0;
       for (i = 0; i < (int) strlen (print_cmd); i++) {
         if (print_cmd[i] == '&')
	   print_cmd2[j++] = '\\'; 
         print_cmd2[j++] = print_cmd[i];
       }
       print_cmd2[j] = '\0';
       strcpy (print_cmd, print_cmd2);

/* 
 * Done with rash setup.. do it and write to rash file.
 * Note, we use popen/fread/fwrite so we can determine if the 
 * write is failing (probably a file system full message).
 */

       popen_file = popen (print_cmd, "r");
       if (popen_file == (FILE *) NULL) {
	  print_error (tmp_image);
	  return ;
  	  }
       out_file = fopen (prog->rashfile, "w");
       if (out_file == (FILE *) NULL) {
	  print_error (tmp_image);
	  pclose (popen_file);
	  return ;
  	  }
	  
       while ((bytes_to_write = fread (write_buf, 1, SVR4_LIMIT, popen_file))
				!= 0) {
	  bytes_written = fwrite (write_buf, 1, bytes_to_write, out_file);
  	  if (bytes_written != bytes_to_write) {
	     print_error (tmp_image);
	     pclose (popen_file);
	     fclose (out_file);
	     unlink (prog->rashfile);
	     return;
	     }
	  }

       fclose (out_file);
       pclose (popen_file);

       }

/*
 * Now, start print cmd.
 */

    sprintf (print_cmd, "%s %s%s %s%s ", PRINT_OPT, PRINTER_OPT, printer,
		TITLE_OPT, basename (current_image->file));
/*
 * Get # of copies..
 */

    if (copies != 1) {
       sprintf (tmp_buf, "%s%d ", COPIES_OPT, copies);
       strcat (print_cmd, tmp_buf);
       }

/*
 * Print header page??
 */

    if (header == NO_HEADER_PAGE)
       strcat (print_cmd, HEADER_OPT);

/*
 * If we're printing an entire ps file, add filename.
 * Note, check here if we have a compressed file, or data, or compressed
 * data, and use the correct file name. Note that if we have data
 * we may have not written out the data, so do it now.
 */

    if (current_display == ps_display) {
       if (range == ALL_PAGES) {
          if (current_image->compression == UNIX)
	     file_to_print = prog->uncompfile;
          else if (current_image->data != (char *) NULL) {
	     if (write_tmpfile (&(prog->uncompfile), current_image->data, 
			        basename (current_image->file),
                                current_image->file_size) == -1) {
	        print_error (tmp_image);
	        return ;
	        }
             file_to_print = prog->uncompfile;
	     }
          else
	     file_to_print = current_image->realfile;
          }
       strcat (print_cmd, file_to_print);
       }
    else
       strcat (print_cmd, prog->rashfile);
/*
 * Check for any "&" in string in case filename has a '&' in it.
 * Place a '\'in front if the '&'.
 */
    j = 0;
    for (i = 0; i < (int) strlen (print_cmd); i++) {
      if (print_cmd[i] == '&')
	print_cmd2[j++] = '\\'; 
      print_cmd2[j++] = print_cmd[i];
    }
    print_cmd2[j] = '\0';
    strcpy (print_cmd, print_cmd2);

/*
 * Do it!
 */

    if (system (print_cmd) != 0) {
      sprintf (error, EGET ("Unable to print %s.  Check console window for error messages."), current_image->file);
      display_error (base_window->base_window, error);
    }

/*
 * Avoid cmap from being freed by setting to NULL.
 * The cmap was copied from current_image.
 */
    if (tmp_image != (ImageInfo *) NULL) {
       tmp_image->cmap = NULL;
       destroy_image (tmp_image);
    }
  
    setactive ();

}


void 
treelist (p, i, item)
tnode		*p;
int		*i;
Panel_item	 item;
{
   if (p != NULL) {
      treelist (p->left, i, item);
      xv_set(item, PANEL_LIST_STRING,  (*i)++, p->printer, 0) ;
      treelist (p->right, i, item);
   }
}
 
void
set_printer_list (item)
Panel_item	item;
{
    int n = nprinters;

    initialize_printers();
    if (prog->def_printer) 
      xv_set(item, PANEL_LIST_STRING, 0, prog->def_printer,
		    PANEL_LIST_SELECT, 0, TRUE,
		    NULL);
    treelist (root, &n, item);

    if (delete_default && prog->def_printer)
      list_delete_entry (item, prog->def_printer);
}
