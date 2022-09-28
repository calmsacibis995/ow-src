
#ifndef lint
static char sccsid[] = "@(#)open.c 1.64 97/01/14";
#endif

/*
 * Copyright (c) 1986, 1987, 1988 by Sun Microsystems, Inc.
 */

#include <unistd.h>
#include <sys/stat.h>
#include <sys/param.h>
#include <xview/panel.h>
#include <xview/notice.h>
#include <xview/scrollbar.h>
#include "display.h"
#include "image.h"
#include "imagetool.h"
#include "state.h"
#include "props.h"
#include "ui_imagetool.h"
#include "ds_listbx.h"
#include "ds_popup.h"

extern int  choice_rows;
extern int  first_open;

extern ProgInfo *prog;

int 
check_open_file (fname)
     char *fname;
{
    struct stat file_info;
    char error[1024];

    if (stat (fname, &file_info) == 0) {
	if (S_ISDIR (file_info.st_mode)) {
	    sprintf (error, EGET ("%s is a folder.  Use File => Open to choose a single file."), fname);
	    display_error (base_window->base_window, error);
#ifndef FILECHOOSER
	    xv_set (openf->directory, PANEL_VALUE, fname, NULL);
	    xv_set (openf->file, PANEL_VALUE, "", NULL);
	    if (openas != NULL)
		xv_set (openas->file, PANEL_VALUE, "", NULL);
#endif
	    free (prog->directory);
	    prog->directory = malloc (strlen (fname) + 1);
	    strcpy (prog->directory, fname);
	    setactive ();
	    return (-2);
	}
	if (!S_ISREG (file_info.st_mode)) {
	    sprintf (error, EGET ("%s is not a regular file."), fname);
	    display_error (base_window->base_window, error);
	    setactive ();
	    return (-2);
	}

/*
**	if ( !( (file_info.st_uid == prog->uid && file_info.st_mode & S_IRUSR)
**	       || (file_info.st_gid == prog->gid && file_info.st_mode & S_IRGRP)
**	       || (file_info.st_mode & S_IROTH)
**	       )
**	    ) 
*/
	{
		/* Bug 1212993 - Just try it. It may not be that simple. */

		FILE *fp;
		if (fp = fopen(fname, "r")) {
			fclose(fp);
		} else {
			sprintf (error, EGET ("You do not have read permission for %s."), 
				 fname);
			display_error (base_window->base_window, error);
			setactive ();
			return (-2);
		}
	}

	if (file_info.st_size == 0) {
	    sprintf (error, EGET ("%s is an empty file."), fname);
	    display_error (base_window->base_window, error);
	    setactive ();
	    return (-2);
	}
    }


    /*
     * Return the number of bytes in the file.
     */

    return (file_info.st_size);
}


#ifdef FILECHOOSER
void
set_current_doc (fc, fname)
   File_chooser  fc;
   char         *fname;
{
   Xv_opaque     list;
   Scrollbar     sb;
   int           view_start, nrows, drows, greatest;

   list = xv_get (fc, FILE_CHOOSER_CHILD, FILE_CHOOSER_FILE_LIST_CHILD);
   list_entry_select_string (list, fname);

   view_start = current_list_selection (list);
   if (view_start != 0) {
     sb = (Scrollbar) xv_get (list, PANEL_LIST_SCROLLBAR); 
     drows = xv_get (list, PANEL_LIST_DISPLAY_ROWS);
     nrows = xv_get (list, PANEL_LIST_NROWS);
     greatest = nrows - drows;
     if (view_start > greatest)
       view_start = greatest;
     xv_set (sb, SCROLLBAR_VIEW_START, view_start, NULL);
   }

}

void
reread_file_choosers (op)
   int   op;
{
   char   *save_dir, *dir;

   if (op == SAVE)
     save_dir = (char *) xv_get (savefc, FILE_CHOOSER_DIRECTORY);
   else
     save_dir = (char *) xv_get (saveasfc->saveasfc, FILE_CHOOSER_DIRECTORY);

   if (openfc) {
     dir = (char *)xv_get (openfc, FILE_CHOOSER_DIRECTORY);
     if (strcmp (save_dir, dir) == 0) {
       xv_set (openfc, FILE_CHOOSER_UPDATE, NULL);
       if (current_image)
         set_current_doc (openfc, basename (current_image->file));
     }
   }

   if (openasfc) {
     dir = (char *)xv_get (openasfc->openasfc, FILE_CHOOSER_DIRECTORY);
     if (strcmp (save_dir, dir) == 0) {
       xv_set (openasfc->openasfc, FILE_CHOOSER_UPDATE, NULL);
       if (current_image)
         set_current_doc (openasfc->openasfc, basename (current_image->file));
     }
   }

}
#endif

/*
 * Things to check just after loading in a new image.
 * prev_null_image = true is the prev image was null.
 */
void
check_popups (prev_null_image)
   int  prev_null_image;
{
   int        ps_file;
   Xv_opaque  doc_name;
   Window     new_wins[2];
  
   if (current_display == ps_display)
     ps_file = TRUE;
   else
     ps_file = FALSE;
  
/*
 * This is a kludge if prev_image is NULL and
 * just loaded ps_file because we do not get a
 * WIN_VIS event in this case.
 */
   if (prev_null_image && ps_file && current_props->show_palette) {
     XSync (current_display->xdisplay, 0);
     palette_callback (NULL, MENU_NOTIFY);
     first_open = FALSE; 
   }

/*
 * No matter what the type, display image info.
 */
    if (imageinfo &&
        (xv_get (imageinfo->imageinfo, XV_SHOW) == TRUE))
	  update_imageinfo (current_image);

/*
 * If print preview popup is showing, update the display.
 */
    if (print_preview && (xv_get(print_preview->print_prev, XV_SHOW)==TRUE)) {
	position_image();
    }

/*
 * If Save Selection As showing, unshow it because
 * nothing is selected now.
 */
    if (saveasfc && (xv_get (saveasfc->saveasfc, XV_SHOW) == TRUE)) {
      char  *label;
      label = (char *) xv_get (saveasfc->saveasfc, XV_LABEL);
      if (strcmp (label, LGET ("Image Tool:  Save Selection As")) == 0)
        xv_set (saveasfc->saveasfc, XV_SHOW, FALSE, NULL);
    }

/*
 * Just loaded image file...
 *   Unshow PS Options.
 *   Unshow Page Overview
 *   If Save Page As Image showing, change to Save As
 * 
 */
   if (ps_file == FALSE) {

     if (ps_options && xv_get (ps_options->ps_options, XV_SHOW))
       xv_set (ps_options->ps_options, XV_SHOW, FALSE, NULL);

     if (pageview && xv_get (pageview->pageview, XV_SHOW))
       xv_set (pageview->pageview, XV_SHOW, FALSE, NULL);

     if (saveasfc && (xv_get (saveasfc->saveasfc, XV_SHOW) == TRUE)) {
       char  *label;
       label = (char *) xv_get (saveasfc->saveasfc, XV_LABEL);
       if (strcmp (label, LGET ("Image Tool:  Save Page As Image")) == 0)
         xv_set (saveasfc->saveasfc, XV_LABEL, LGET ("Image Tool:  Save As"), NULL);
     }
   }

/*
 * Just loaded PS file...
 *   If Save As showing, change to Save Page As Image
 *   If PS Options showing, update the page overview if npages > 1.
 */
   else {

     if (saveasfc && (xv_get (saveasfc->saveasfc, XV_SHOW) == TRUE)) {
       char  *label;
       label = (char *) xv_get (saveasfc->saveasfc, XV_LABEL);
       if (strcmp (label, LGET ("Image Tool:  Save As")) == 0)
	 xv_set (saveasfc->saveasfc, 
		 XV_LABEL, LGET ("Image Tool:  Save Page As Image"), 
		 NULL);
     }

     if (pageview && 
	 (xv_get (pageview->pageview, XV_SHOW) == TRUE)) {
       if (current_image->pages > 1) 
         xv_set (pageview->vscroll,
	         SCROLLBAR_OBJECT_LENGTH, choice_rows,
	         NULL);
       else
	 xv_set (pageview->pageview, XV_SHOW, FALSE, NULL);
     }   

   }

/*
 * If Save/Save As is showing, then
 * update the document name with the newly
 * loaded document.
 */
   if (savefc) {
     doc_name = xv_get (savefc, FILE_CHOOSER_CHILD, 
	     		FILE_CHOOSER_DOCUMENT_NAME_CHILD);
     xv_set (doc_name, PANEL_VALUE, basename (current_image->file), NULL);
   }
   if (saveasfc) {
     char        newname [MAXPATHLEN];
     CompTypes   compression;
     char       *comp_str;
     int         length, null_set = FALSE;

     doc_name = xv_get (saveasfc->saveasfc, FILE_CHOOSER_CHILD, 
	     		FILE_CHOOSER_DOCUMENT_NAME_CHILD);
     comp_str = (char *) xv_get (saveasfc->compression_value,
				 PANEL_LABEL_STRING);
     compression = string_to_compression (comp_str);
     length = strlen (current_image->file);
     if ((length > 2) &&
	 (current_image->file [length - 1] == 'Z') &&
	 (current_image->file [length - 2] == '.') &&
	 (compression != UNIX)) {
       current_image->file [length - 2] = '\0';
       null_set = TRUE;
     }

     unique_filename (current_image->file, newname);
     if (null_set)
       current_image->file [length - 2] = '.';
     xv_set (doc_name, PANEL_VALUE, basename (newname), NULL);
   }

/*
 * Reset the zoom and angle choices
 */
 
    set_zoom_and_rotate ();

/*
 * Set WM_COLORMAP_WINDOWS property on base canvas.
 */
    new_wins[0] = current_display->win;
    new_wins[1] = xv_get (base_window->base_window, XV_XID);
    XSetWMColormapWindows (current_display->xdisplay, 
			   xv_get (base_window->base_window, XV_XID),
		           new_wins, 2);

}

void
set_openas_list()
{
    int        i;
    Xv_opaque  list;

#ifdef FILECHOOSER
    list = openasfc->format_list;
#else
    list = openas->format_list;
#endif

/*
 * Set the openas list, to the list in all_types, adding client
 * data, so we can easily determine the choice when the user selects
 * something.
 */

    for (i = 0; i < ntypes; i++) 
        xv_set (list,
			PANEL_LIST_INSERT, i,
			PANEL_LIST_STRING, i, all_types[i].popup_list_name,
			PANEL_LIST_CLIENT_DATA, i, i,
			NULL);

    xv_set (list, 
		PANEL_LIST_SORT, PANEL_FORWARD,
		NULL);
}

/*
 * Generic open for open and openas...
 */

int
open_newfile (item, path, realpath, open_type, data, size)
Panel_item	 item;
char		*path;
char		*realpath;
int		 open_type;
char		*data;
int		 size;
{
    char	*type = NULL;
    int          retry = FALSE;
    int		 file_size = size;
    TypeInfo	*file_type;
    TypeInfo	*save_file_type;
    int	         status;
    int          second_try = FALSE;
    ImageInfo	*new_image;
    StateInfo	*new_state;
    XilImage	 tmp_image;
    int		 compressed = FALSE;
    int          prev_null_image = FALSE;
    char         error[1024];
    
    setbusy();

/*
 * Determine if we can load in the file, if we have a file.
 */

    if (data == (char *) NULL) {
      if ((file_size = check_open_file (realpath)) < 0) {
	if (item != NULL)
	  xv_set (item, PANEL_NOTIFY_STATUS, XV_ERROR, NULL);
	setactive ();
#ifndef FILECHOOSER
	free (path);
#endif
	return (-1);
      }
    }

RETRY:
/*
 * If open_type is -1, then we need to type the file.
 * Otherwise, the user used the Open As popup, and we already
 * know the type.
 */

    if (open_type != -1) {
       int length;
/*
 * File type was passed in...
 * First check if *.Z file.
 */
       file_type = &all_types [open_type];
       if ((length = strlen (realpath)) > 2) {
         if ((realpath[length - 1] == 'Z') && (realpath[length - 2] == '.'))
           compressed = TRUE;
       }
     }
    else {
/*
 * We don't know the type yet..
 * if ce_okay == -1, need to init ce.
 * if ce_okay == FALSE, use open as.
 * else type the file.
 */
      if (prog->ce_okay == -1) 
	prog->ce_okay = init_ce ();

      if (prog->ce_okay == FALSE) {
	sprintf (error, EGET ("File format not found.  Use Open As option to choose file format."), path);
	display_error (base_window->base_window, error);
	if (item != NULL)
	  xv_set (item, PANEL_NOTIFY_STATUS, XV_ERROR, NULL);
	setactive ();
	return (-1);
      }
      else {

	type = (char *) type_file (realpath, &compressed, data);
	if (strcmp (type, DGET ("compress")) == 0) {
	  if (write_tmpfile (&(prog->compfile), data, basename (path), 
			     file_size) == -1)
	    return (-1);
	  
	  type = (char *) type_file (prog->compfile, &compressed, 
				     (char *) NULL);
	}

	if (prog->verbose)
          fprintf (stderr, MGET ("File Type is %s\n"), type);


/*
 * If file_type is unknown, then we don't know the type, so ask the user
 * to use the open as pop up instead.
 */

	if (strcmp (type, LGET ("unknown")) == 0) {
 	  sprintf (error, EGET ("File format not found.  Use Open As option to choose file format."), realpath);
          display_error (base_window->base_window, error);	 
	  if (item != NULL)
	    xv_set( item, PANEL_NOTIFY_STATUS, XV_ERROR, NULL );
#ifndef FILECHOOSER
          free (path);
#endif
          setactive ();
          return  (-1);
	}
	
/*
 * Check if we even support loading this type of file.
 */

	file_type = str_to_ftype (type);
	if (file_type == NULL) {
	  display_error (base_window->base_window,
			 EGET ("File not opened: Unsupported format."));
	  if (item != NULL)
	    xv_set( item, PANEL_NOTIFY_STATUS, XV_ERROR, NULL );
#ifndef FILECHOOSER
	  free (path);
#endif
	  setactive ();
	  return  (-1);
	}
      }
    }

/*
 * Init image objects if not already done so and not a PS file.
 */
    if ((file_type->type != POSTSCRIPT) && (file_type->type != EPSF) && 
	(prog->xil_opened == FALSE)) {
	 base_window_image_canvas_objects_create (base_window);
	 prog->xil_opened = TRUE;
    }

/*
 * If we were reading a PostScript file, we can stop now...
 */

    if ((current_image != (ImageInfo *) NULL) && 
        (current_display != (DisplayInfo *) NULL) &&
	(current_display == ps_display))
       close_ps (TRUE, FALSE);

/*
 * Check for unknown file type here... Here's what we should be doing...
 * If the file isn't one of the `known' ones (postscript, gif, raster, tiff),
 * then we should check the classing engine for a IMAGE_LOAD value 
 * and attempt to load the file using that.. 
 */

/*
    if (file_type == (TypeInfo *) NULL) {
      
      create a new file_type structure (do a realloc).. add to list.

      add it to saveas list...
*/
    
/*
 * At this point, it looks like we probably will read in the file,
 * so create a new ImageInfo structure, and pass to appropriate
 * load function.
 */

    new_image = init_image (path, realpath, file_size, file_type, data);
    if (compressed == TRUE)
       new_image->compression = UNIX;

/*
 * We need a new state.. mainly for postscript files. Create one here.
 */

    new_state = init_state (( *new_image->type_info).type);

    status = (*new_image->type_info->load_func) (new_image);

/*
 * If status isn't OK, then we hit a problem, so, leave things the
 * the way they are and return.
 */
 
    if (status != 0) {

/*
 * open_type was given to us (Open As) and first time retrying, but failed.
 * Type ourself and retry if our open_type is different than given open_type.
 */
       if ((status == -1) && (open_type != -1) && (retry == FALSE)) {	 
	 save_file_type = file_type;

	 if (prog->ce_okay == -1) 
	   prog->ce_okay = init_ce ();

	 if (prog->ce_okay == TRUE) {

	   /* Type again */
	   type = (char *) type_file (realpath, &compressed, data);
	   retry = TRUE;
	   if (strcmp (type, LGET ("unknown")) == 0) 
	     retry = FALSE;
	   
	   /* Check if unsupported format */
	   if (retry) {
  	     file_type = str_to_ftype (type);
	     if (file_type == NULL)
	       retry = FALSE;
           }

	   /* Check if we already tried this format */
	   if ((retry) && (open_type == (int) file_type->type))
	     retry = FALSE;

           /* Set the open_type to what we think it is */
	   if (retry) {
	     open_type = (int) file_type->type;
	   }

	 }
	 else 
	   retry = FALSE;

	 if (retry == FALSE) {
	   sprintf (error, EGET ("Error opening file as a %s."), save_file_type->popup_list_name);
           display_error (base_window->base_window, error);
	 }
       }
	 
/*
 * If status is -1 then no notice appeared, yet so put one up now.
 */
       else if (status == -1) {
         sprintf (error, EGET ("Error opening %s."), realpath);
         display_error (base_window->base_window, error);
	 if (second_try)
	   retry = FALSE;
       }

       if (item != NULL)
          xv_set( item, PANEL_NOTIFY_STATUS, XV_ERROR, NULL );       
       destroy_image (new_image);
       free (new_state);

       if (retry) {
	 second_try = TRUE;
         goto RETRY;
       }

/*
 * If we had a ps file open, go back to it. Note that if we
 * return -1 from open_psfile, we're screwed!
 */
       if ((current_image == NULL) && (ps_display->canvas != NULL))
          current_display = ps_display;

       if ((current_image != (ImageInfo *) NULL) && 
           (current_display != (DisplayInfo *) NULL) &&
	   (current_display == ps_display)) 
	  restart_ps ();

       if (status == -1) {
         sprintf (error, EGET ("Error opening %s."), realpath);
         display_error (base_window->base_window, error);
	 }

       setactive ();
       return (-2);
       }

    set_tool_options (current_image, new_image);
/*
 * Display message if we opened the file as something else.
 */
    if ((status == 0) && (retry == TRUE) && (file_type != NULL)) {
      sprintf (error, EGET ("Error opening file as a %s.  Opened as a %s."), save_file_type->popup_list_name, file_type->popup_list_name);
      display_error (base_window->base_window, error);
    }


    if (current_image != (ImageInfo *) NULL) {
       destroy_image (current_image);
       }
    else
      prev_null_image = TRUE;

    current_image = new_image;
    if (current_state != (StateInfo *) NULL) {
       free (current_state);
       }

    current_state = new_state;
    set_current_display ();

/*
 * Make image available for XIL processing.
 * Dither image for current depth.
 */
    if (current_display != ps_display) {
	extern void assign_display_func();

	assign_display_func(current_image, current_display->depth);

	(current_image->revert_func)();
/*
	check_canvas (current_image->width, current_image->height,
   		     base_window->canvas, base_window->hscroll,
   		     base_window->vscroll, NULL, NULL, image_display);
*/
    } else {
	current_state->reversed = initial_reverse (TRUE);       
	current_state->using_dsc = initial_dsc ();
	current_state->timeout_hit = initial_timeout ();
	current_state->zoom = prog->def_ps_zoom / 100.0;
    }
/*
 * Unshow popup if unpinned.
 */
    if (item != NULL) {
	if ((Panel_item_type)xv_get(item, PANEL_ITEM_CLASS)==PANEL_TEXT_ITEM) {
	    if (!(int)xv_get((xv_get((xv_get(item, XV_OWNER)), XV_OWNER)),
		FRAME_CMD_PUSHPIN_IN)) {
		xv_set ((xv_get ((xv_get (item, XV_OWNER)), XV_OWNER)),
		    WIN_SHOW, FALSE, NULL);
	    }
	}
    }

/*
 * Fit frame to image then re-position palette if showing.
 */
    resize_canvas ();
    fit_frame_to_image (current_image);
    repaint_image (NULL, NULL, NULL, NULL, NULL);

/*
 * Check which popups should be showing with
 * this newly loaded image.
 */
    check_popups (prev_null_image);

    setactive();

    return (0);
}

/*
 * Callback for Open popup.
 */
int
open_file (item, event)
Panel_item	 item;
Event		*event;     
{
    char  *dir = (char *) xv_get (openf->directory, PANEL_VALUE);
    char  *file = (char *) xv_get (openf->file, PANEL_VALUE);
    char  *tmpfname;
    int    status;
    
    tmpfname = make_pathname (dir, file);
    status = open_newfile (item, tmpfname, tmpfname, -1, (char *) NULL, 0);
   
/*
 * If started via tooltalk, but just loaded in a new file,
 * break tooltalk connection.
 */

    if ((prog->tt_started == TRUE) && (status == 0)) 
       break_tt ();

    return (status);
}

#ifdef FILECHOOSER
/*
 * Callback for Open popup.
 */
int
openfc_callback (fc, path, file, client_data)
    File_chooser   fc;
    char          *path;
    char          *file;
    Xv_opaque      client_data;
{
    int  status;

    status = open_newfile (fc, path, path, -1, (char *) NULL, 0);

#ifdef NEVER
    if (prog->ce_okay)  
      status = open_newfile (fc, path, path, -1, (char *) NULL, 0);
    else {
      sprintf (error, EGET ("File format not found.  Use Open As option to choose file format."), path);
      display_error (base_window->base_window, error);
      if (fc != NULL)
        xv_set (fc, PANEL_NOTIFY_STATUS, XV_ERROR, NULL);
      setactive ();
      return (-1);
    }
#endif
 

/*
 * If started via tooltalk, but just loaded in a new file,
 * break tooltalk connection.
 */

    if ((prog->tt_started == TRUE) && (status == 0)) 
       break_tt ();

    return (status);
}
#endif
#ifdef FILECHOOSER
int
openasfc_callback (fc, path, file, client_data)
    File_chooser  fc;
    char         *path;
    char         *file;
    Xv_opaque     client_data;
{
    int  row, type;
    int  status;

    row = current_list_selection (openasfc->format_list);
    if (row == -1) {
      display_error (fc, EGET ("Choose a file format."));
      xv_set (fc, PANEL_NOTIFY_STATUS, XV_ERROR, NULL);
      setactive ();
      return (-1);
    }

/* 
 * Get the format to open in.
 */ 
    type = xv_get (openasfc->format_list, PANEL_LIST_CLIENT_DATA, row);

    status = open_newfile (fc, path, path, type, (char *) NULL, 0);

/*
 * If started via tooltalk, but just loaded in a new file,
 * break tooltalk connection.
 */

    if ((prog->tt_started == TRUE) && (status == 0)) 
       break_tt ();

    return (status);
}

#endif

/*
 * Callback for OpenAs popup.
 */
int
openas_file (item, event)
Panel_item	 item;
Event		*event;     
{
    char  *dir = (char *) xv_get (openas->directory, PANEL_VALUE);
    char  *file = (char *) xv_get (openas->file, PANEL_VALUE);
    char  *tmpfname;
    int	   row;
    int	   type;
    int    status;
   
    row = current_list_selection (openas->format_list);

    if (row == -1) {
       display_error (openas->openas, EGET ("Choose a file format."));
       xv_set (item, PANEL_NOTIFY_STATUS, XV_ERROR, NULL);
       return (-1); ;
       }

    type = xv_get (openas->format_list, PANEL_LIST_CLIENT_DATA, row);

    tmpfname = make_pathname (dir, file);

    status = open_newfile (item, tmpfname, tmpfname, type, (char *) NULL);

/*
 * If started via tooltalk, but just loaded in a new file,
 * break tooltalk connection.
 */

    if ((prog->tt_started == TRUE) && (status == 0)) 
       break_tt ();

    return (status);
}

#ifdef FILECHOOSER
/*
 * File chooser resize func for openas.
 */
int
openas_resize_func (fc, frame_rect, exten_rect, left_edge, right_edge, max_height)
    File_chooser  fc;
    Rect         *frame_rect;
    Rect         *exten_rect;
    int           left_edge;
    int           right_edge;
    int           max_height;
{
    Scrollbar  sb;
    int        sb_width;
    int        value_x;
    int        item_x;
    int        list_width;
    Xv_opaque  file_list;

    sb = (Scrollbar) xv_get (openasfc->format_list, PANEL_LIST_SCROLLBAR);
    sb_width = (int) xv_get (sb, XV_WIDTH);
    value_x = (int) xv_get (openasfc->format_list, PANEL_VALUE_X);
    item_x = (int) xv_get (openasfc->format_list, XV_X);
    
/*
 * Fit the format list to be same as file list.
 */
    file_list = xv_get (openasfc->openasfc, FILE_CHOOSER_CHILD, 
			FILE_CHOOSER_FILE_LIST_CHILD);

    /*
     * Figure out how wide the list should be.  Take the rightmost
     * alignment position, subtract out the space used for the label,
     * the left edge, and the little space between the abel and the
     * panel value.  Also, PANEL_LIST_WIDTH does not include the 
     * scrollbar width, so subtract that also.
     */
/*    list_width = right_edge - left_edge - (value_x - item_x) - sb_width; */

    xv_set (openasfc->format_list,
	   XV_X,		xv_get (file_list, XV_X),
	   XV_Y,		exten_rect->r_top,
	   PANEL_LIST_WIDTH,	xv_get (file_list, PANEL_LIST_WIDTH),
	   PANEL_PAINT, 	PANEL_NONE,
	   NULL);

    return -1;

}
#endif
