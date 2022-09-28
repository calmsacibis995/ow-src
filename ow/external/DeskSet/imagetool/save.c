
#ifndef lint
static char sccsid[] = "@(#)save.c 1.65 96/06/18";
#endif

/*
 * Copyright (c) 1986, 1987, 1988 by Sun Microsystems, Inc.
 */

/* 
 * save.c - Functions used for saving...
 */

#include <unistd.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <xview/notice.h>
#include <xview/panel.h>
#include <xview/scrollbar.h>
#include "display.h"
#include "image.h"
#include "imagetool.h"
#include "state.h"
#include "ui_imagetool.h"
#include "ds_popup.h"

int
check_save_file (item, fname, save_type)
Panel_item  item;
char  	   *fname;
int         save_type;
{
    struct stat	file_info;
    FILE       *fp = NULL;
    int		status;
    char        error[1024];

    if (stat (fname, &file_info) == 0) {
      if (S_ISDIR (file_info.st_mode)) {
       sprintf (error, EGET ("Please enter a file name in the Save field."), fname);
       display_error (item, error);
       return (-2);
      }
    }

/*
 * First try opening it for reading to check if file exists.
 */ 
    if ((fp = fopen (fname, "r")) == NULL) {
      /*
       * Couldn't open if for reading, but file can be
       * 044, 004, 000 permissions so do a stat.
       */
      if (access (fname, F_OK) == 0) 
	  return (0); 
      /* 
       * File doesn't exist so open it for writing to check if
       * fname contains directory that doesn't exist. (/tmp/xxx/xxx/xx)
       */
      if ((fp = fopen (fname, "w")) == NULL) {
        sprintf (error, EGET ("Cannot save to %s."), fname);
        display_error (item, error);
        return (-1);
      }
      else
	unlink (fname);
    }
 
    if (fp)
      fclose (fp);

    return (0);
}

int
colors_to_depth (colors)
    char *colors;
{
    int depth = 8;

/* I18N_STRING
   B&W = short for black and white
 */
    if (strcmp (colors, LGET ("B&W")) == 0)
      depth = 1;
    else if (strcmp (colors, LGET ("256")) == 0)
      depth = 8;
    else if (strcmp (colors, LGET ("Millions")) == 0)
      depth = 24;

   return depth;
}

/*
 * Verify that a file format is selected.
 */
static int
verify_file_format (item)
  Panel_item  item;
{
  int  row;

  row = current_list_selection (saveasfc->format_list);

  if (row == -1) {
    display_error (item, EGET ("Choose a file format."));
    if (item != NULL)
      xv_set (item, PANEL_NOTIFY_STATUS, XV_ERROR, NULL);
  } 

  return row;   
}

CompTypes
string_to_compression (comp_str)
    char  *comp_str;
{
    CompTypes compression = NO_COMPRESS;

    if (strcmp (comp_str, LGET ("None")) == 0)
      compression = NO_COMPRESS;
    else if (strcmp (comp_str, LGET ("Unix")) == 0)
      compression = UNIX;
    else if (strcmp (comp_str, LGET ("Encoded")) == 0)
      compression = RUN_LENGTH;
    else if (strcmp (comp_str, LGET ("LZW")) == 0)

      compression = LZW;
    else if (strcmp (comp_str, LGET ("JPEG")) == 0)
      compression = JPEG;

    return compression;
}

/*
 * 1 - continue
 * 0 - don't continue.
 */
static int
confirm_data_loss (fname)
    char *fname;
{
    int status;

    status = notice_prompt (base_window->base_window, NULL,
		   NOTICE_MESSAGE_STRINGS, EGET ("Saving to the JFIF (JPEG) file format\nmay result in a loss of data.\nDo you want to continue?"), NULL,
		   NOTICE_BUTTON_YES, LGET ("Yes"),
		   NOTICE_BUTTON_NO, LGET ("No"),
		   NULL);
    if (status == NOTICE_NO) 
      return 0;
    else {
      if (access (fname, F_OK)  == 0)
        unlink (fname);
    }
      return 1;
}

/*
 * 1 - continue
 * 0 - don't continue.
 */
static int
confirm_overwrite_file (fname)
    char *fname;
{
    int status;
    int value = 1;
    int base_length = strlen (basename (fname));
    int length = strlen (fname);
    char msg[2048];

    if ((base_length > 2) && 
	(fname[length - 1] == 'Z') && (fname[length - 2] == '.')) {
      fname[length - 2] = '\0';
      if (access (fname, F_OK) == 0) {
	sprintf (msg, EGET ("The file \"%s\" already exists.\nSaving a file using UNIX compression will overwrite this file.\nDo you want to overwrite the existing file \"%s\"?"), basename (fname), basename (fname));
	status = notice_prompt (base_window->base_window, NULL,
				NOTICE_MESSAGE_STRING, msg,
				NOTICE_BUTTON_NO, LGET ("Cancel"),
				NOTICE_BUTTON_YES, LGET ("Overwrite Existing File"),
				NULL);
	if (status == NOTICE_NO)
	  value = 0;
	else
	  value = 1;
      }
      fname[length - 2] = '.';
    }
    else {
      char cname[MAXPATHLEN];
      sprintf (cname, "%s.Z", fname);
      if (access (cname, F_OK) == 0) {
	sprintf (msg, EGET ("The file \"%s\" already exists.\nSaving a file using UNIX compression will overwrite this file.\nDo you want to overwrite the existing file \"%s\"?"), basename (cname), basename (cname));
	status = notice_prompt (base_window->base_window, NULL,
				NOTICE_MESSAGE_STRING, msg,
				NOTICE_BUTTON_NO, LGET ("Cancel"),
				NOTICE_BUTTON_YES, LGET ("Overwrite Existing File"),
				NULL);
	if (status == NOTICE_NO)
	  value = 0;
	else
	  value = 1;
	
      }
    }
    
    return value;

}

int
save_entire_ps_file (item, new_image, old_image)
Panel_item       item;
ImageInfo	*new_image;
ImageInfo	*old_image;
{
    mmap_handle_t	*file_ptr;
    int			 status = 0;

    if (strcmp (old_image->realfile, new_image->file) == 0)
      return (status);

    if (access (new_image->file, F_OK) == 0) {
      status = unlink (new_image->file);
      if (status != 0) {
	display_error (item, EGET ("Cannot save file--check permissions."));
	return (-2);
      }
    }
    file_ptr = fast_read (old_image->realfile);
    status = fast_write (new_image->file, file_ptr->addr, file_ptr->len, 0);
    fast_close (file_ptr);
    return (status);
}

int
save_newfile (item, tmpfname, save_type)
Panel_item	 item;
char		*tmpfname;
int		 save_type;
{
    char	      *colors_str;
    char	      *comp_str;
    int		       row;
    int		       type;
    int 	       status;
    int		       save_depth;
    ImageInfo	      *tmp_image;
    StateInfo	      *new_state;
    struct stat	       file_info;
    XilMemoryStorage  *storage;
    XilDataType        datatype;
    unsigned int       width, height, nbands;
    unsigned int       scanline_stride, pixel_stride;
    unsigned char     *data;
    unsigned char     *tmp_data;
    float 	       mult[1], offset[1];
    int		       do_compression = TRUE;

    setbusy ();
/*
 * Make sure the fname is not a directory.
 */
    status = check_save_file (item, tmpfname, save_type);
    if (status != 0) {
      setactive();
      return (status);
    }
/*
 * Need to check some things on SaveAs.
 */

    switch (save_type) {
    case SAVEAS:
    case SAVESELECTIONAS:
    case SAVEPAGEASIMAGE:
/*
 * Make sure image format was selected.
 */
      row = verify_file_format (item);
      if (row == -1) {
        setactive();
	return (row);
      }
/*
 * Get saving file format and
 * confirm data loss if JFIF/JPEG.
 */
      type = xv_get (saveasfc->format_list, PANEL_LIST_CLIENT_DATA, row);
      if ((&all_types [type])->type == JFIF) {
        if ((status = confirm_data_loss (tmpfname)) == 0) {
	   setactive();
	   return (-1);
        }
      }
/*
 * Create a new ImageInfo with new filename.
 */
      tmp_image = init_image (tmpfname, tmpfname, 0, &all_types [type], 
				 (char *) NULL);
/*
 * Get the depth to save in.
 */
      colors_str = (char *)xv_get (saveasfc->colors_value, 
				   PANEL_LABEL_STRING);
      tmp_image->depth = colors_to_depth (colors_str);

/*
 * Get the compression method.
 */
      comp_str = (char *) xv_get (saveasfc->compression_value, 
				  PANEL_LABEL_STRING);
      tmp_image->compression = string_to_compression (comp_str);
      break;

/*
 * Regular SAVE.
 */
    case SAVE:
/*
 * Confirm data loss if JFIF/JPEG.
 */
       if ((current_image->type_info)->type == JFIF) {
         if ((status = confirm_data_loss()) == 0) {
	   setactive();
	   return (-1);
         }
       }
/*
 * Make a copy of the current image, using same
 * depth, compression, file format.
 * Insert the new file name in case it changed.
 */
	tmp_image = init_image(tmpfname, tmpfname, 0, current_image->type_info,
	    (char *)NULL);
	tmp_image->compression = current_image->compression;
	tmp_image->depth = current_image->depth;
	tmp_image->nbands = current_image->nbands;
	
      break;

    default:
      break;

    }  /* end switch */

/*
 * For UNIX compression, confirm file overwritten.
 */
    if (tmp_image->compression == UNIX) {
      if ((status = confirm_overwrite_file (tmpfname)) == 0) {
        setactive();
        return (-1);
      }
    }

/*
 * If current view is PS, save out to XIL image.
 * Fix later, PS not saving from original.
 * else make a copy of current_image->orig_image.
 * Note, if we're doing a SAVE of a ps file, then bypass all of
 * this stuff...
 */
    if ((current_display != ps_display) || (save_type != SAVE)) {
	/* 
	 * strip off ".Z" if any from file name if file saves to 
	 * UNIX compress format 
	 */
	if (tmp_image->compression == UNIX) {
	    int len = strlen(tmp_image->file);

	    if (len > 2) 
		if (tmp_image->file[len-1] == 'Z')  
		    if (tmp_image->file[len-2] == '.') 
			tmp_image->file[len-2] = '\0';
	}

	if (current_display == ps_display) {
	    if (prog->xil_opened == FALSE) {
		base_window_image_canvas_objects_create (base_window);
		prog->xil_opened = TRUE;
	    }
	    current_image->orig_image = create_image_from_ps(current_image);

	    current_image->saving = TRUE;
	    assign_display_func(current_image, tmp_image->depth);
	    current_image->saving = FALSE;

	    if (save_type == SAVESELECTIONAS) {
		XilImage	tmp_xilimage;

		/* the image return from crop_region is a child image */
		tmp_xilimage = crop_region(current_image,
			current_image->orig_image);
		if (tmp_xilimage == NULL) {
		    setactive();
		    return (-1);
		}

		tmp_image->orig_image = (current_image->display_func)
			(tmp_xilimage);
	    } else {
		tmp_image->orig_image = (current_image->display_func)
			(current_image->orig_image);
	    }
	} else {
	    current_image->saving = TRUE;
	    assign_display_func(current_image, tmp_image->depth);
	    current_image->saving = FALSE;

	    /* create a copy that only contains selected region */
	    if (save_type == SAVESELECTIONAS) {
		XilImage	tmp_xilimage;

		/* the image return from crop_region is a child image */
		tmp_xilimage = crop_region(current_image,
			current_image->dest_image);

		if (tmp_xilimage == NULL) {
		    setactive();
		    return (-1);
		}

		tmp_image->orig_image = (current_image->display_func)
			(tmp_xilimage);
	    } else {
		tmp_image->orig_image = (current_image->display_func)
			(current_image->dest_image);
	    }
	}

	/*
	 * Copy the cmap, width, height, nbands, offset, colors_compressed,
	 * bytes_per_line and rgborder from current image.
	 * 
	 */
	copy_image_data(tmp_image, current_image); 

	/* set the correct dimension */
	tmp_image->width = xil_get_width(tmp_image->orig_image);
	tmp_image->height = xil_get_height(tmp_image->orig_image);
	tmp_image->nbands = xil_get_nbands(tmp_image->orig_image);
	tmp_image->datatype = xil_get_datatype(tmp_image->orig_image);

	/* adjust bytes_per_line! */
	tmp_data = retrieve_data (tmp_image->orig_image, &pixel_stride,
				&scanline_stride);
	xil_import (tmp_image->orig_image, FALSE);
	tmp_image->bytes_per_line = scanline_stride;

	/*
	 * Rescale view_image back by multiplying the offset by -1.0.
	 * Can ps files have an offset? If so, then this is wrong!
	 */
	if (current_image->dithered24to8) {
	    mult[0] = 1.0; 
	    offset[0] = (float) tmp_image->offset * -1.0;
	    xil_rescale (tmp_image->orig_image, tmp_image->orig_image, mult, 
		       offset);
	}

	/*
	 * At this point the image we're saving should be in
	 * tmp_image->orig_image.
	 * The save_funcs should save whatever's in orig_image.
	 *
	 * Delete the file (if exists) even if it's permissions are 444
	 * because at this point the user said to overwrite even
	 * if the file is read-only.
	 */
	if (access(tmp_image->file, F_OK)==0 && tmp_image->compression!=UNIX) {
	    status = unlink (tmp_image->file);
	    if (status != 0) {
		display_error (item, EGET ("Cannot overwrite file."));
		setactive ();
		return (-2);
	    }
	}
	status = (*tmp_image->type_info->save_func) (tmp_image);

    } else {
	int length = strlen (tmp_image->file);
	if ((length > 2) && ((tmp_image->file[length - 1] != 'Z') ||
	    (tmp_image->file[length - 2] != '.'))) {
	    char *buf; 

	    buf = (char *) malloc (strlen (tmp_image->file) + 3);
	    sprintf(buf, "%s%s", tmp_image->file, ".Z");
	    free (tmp_image->file);
	    tmp_image->file = buf;
	    tmp_image->realfile = buf;
	}
	status = save_entire_ps_file (item, tmp_image, current_image);

	/*
	 * If we saved the entire file, it is already compressed, so set
	 * a flag to not do the compression.
	 */
	do_compression = FALSE;
    }
      
    if (status != 0) {
	display_error (item, EGET ("Cannot save file."));
	if (save_type != SAVE) {
	    xv_set(item, PANEL_NOTIFY_STATUS, XV_ERROR, NULL );
	}
	destroy_image (tmp_image);
	setactive ();
	return (-2);
    } 

    if ((tmp_image->compression == UNIX) && (do_compression == TRUE)) {
	char command[80];
	char filename[MAXPATHLEN];
	char *error_string;
	int status; 

	/*
	 * Delete the *.Z file before compressing or compress will prompt user
	 * asking if you wish to overwrite *.Z file.
	 */
	sprintf (filename, "%s.Z", tmp_image->file);
	if (access (filename, F_OK) == 0) {
	    status = unlink (filename);
	    if (status != 0) {
		display_error (item, EGET ("Cannot save file."));
		setactive ();
		return (-2);
	    }
	}
	sprintf(command, "%s %s", DGET ("/usr/bin/compress"), tmp_image->file);
	if ((status = system(command)) == 0) {
	    char *buf; 

	    if ((buf = (char*)malloc(strlen(tmp_image->file) + 3)) == NULL) {
		if (prog->verbose) {
		    fprintf( stderr, MGET ("cannot malloc memory\n"));
		}
	    } else {
		sprintf(buf, "%s%s", tmp_image->file, ".Z");
		free(tmp_image->file);
		tmp_image->file = buf;
		tmp_image->realfile = buf;
	    }
	} else if (status == 512) {
	    /* not sure this status return all the time for this type 
	     * compress - it did return from gif file
	     */
	    char    error[1024];

	    sprintf (error, EGET ("%s not compressed.  File would be larger if compressed."), tmpfname);
	    display_error (item, error);
	} else {
	    display_error (item, EGET ("File compression failed."));
	}
    }

    /*
     * New state.
     */
    new_state = init_state ();

    stat (tmp_image->file, &file_info);
    tmp_image->file_size = file_info.st_size;

    /*
     * Need to load in the `new' postscript file before doing the
     * set_current_display, so do it now.
     * Also, check here, if old image was ps, then turn of ps stuff.
     * If new image is ps, then we kill pagecounter when we load in
     * the new one. If not, then we have to kill it now.
     */
    if (((tmp_image->type_info)->type == POSTSCRIPT) ||
	((tmp_image->type_info)->type == EPSF)) {
	ps_load (tmp_image);
    } else if (((current_image->type_info)->type == POSTSCRIPT) ||
	((current_image->type_info)->type == EPSF)) {
	close_ps (TRUE, TRUE);
    }
	
    set_tool_options (current_image, tmp_image);

    if (current_image != (ImageInfo *) NULL) {
	destroy_image (current_image);
    }
    current_image = tmp_image;

    if (current_state != (StateInfo *) NULL) {
	free (current_state);
    }
    current_state = new_state; 

    set_current_display ();

    if (((current_image->type_info)->type != POSTSCRIPT) &&
	((current_image->type_info)->type != EPSF)) {
	assign_display_func(current_image, current_display->depth);
	check_canvas (current_image->width, current_image->height,
                     base_window->canvas, base_window->hscroll,
                     base_window->vscroll, NULL, NULL, image_display);
 
    } else { 	/* Is a postscript file */
       current_state->reversed = initial_reverse (TRUE);
       current_state->using_dsc = initial_dsc ();
       current_state->timeout_hit = initial_timeout ();
       current_state->zoom = prog->def_ps_zoom / 100.0;
    }
 
    /*
     * Take down popup if unpinned.
     */
    if (item != NULL) {
	if ((Panel_item_type)xv_get(item, PANEL_ITEM_CLASS) == PANEL_TEXT_ITEM) {
	    if ( !(int)xv_get((xv_get((xv_get(item, XV_OWNER)), XV_OWNER)),
		FRAME_CMD_PUSHPIN_IN)) {
		xv_set ((xv_get ((xv_get (item, XV_OWNER)), XV_OWNER)),
		    WIN_SHOW, FALSE, NULL);
	    }
	}
    }

    resize_canvas ();

    if (current_display == ps_display) {
	fit_frame_to_image (current_image);
    }

    display_new_image();
 
    /*
     * If we were started via tooltalk, tell other application that file
     * was saved.
     */
    if (prog->tt_started == TRUE)
	save_file_tt ();

    /*
     * Reset the scroll list on the saveas.
     */
    reread_file_choosers (save_type);  
    check_popups (FALSE);

    setactive();

    if (palette != NULL) {
	reset_select ();
	reset_pan ();
    }

    return XV_OK;
}                 

int
saveas_file (item, event)
Panel_item	 item;
Event		*event;
{
    char	*dir = (char *) xv_get (saveas->directory, PANEL_VALUE);
    char	*file = (char *) xv_get (saveas->file, PANEL_VALUE);
    char        *tmpfname;

    tmpfname = make_pathname (dir, file);
    return (save_newfile (item, tmpfname, SAVEAS));
}

int
saveselectionas_file (item, event)
Panel_item	 item;
Event		*event;
{
    char	*dir = (char *) xv_get (saveas->directory, PANEL_VALUE);
    char	*file = (char *) xv_get (saveas->file, PANEL_VALUE);
    char        *tmpfname;
   
    tmpfname = make_pathname (dir, file);
    return (save_newfile (item, tmpfname, SAVESELECTIONAS));

}

#ifdef FILECHOOSER
int
savefc_callback (fc, path, stats)
    File_chooser  fc;
    char         *path;
    struct stat  *stats;
{

    return (save_newfile (fc, path, SAVE));	
}
#endif
#ifdef FILECHOOSER
int
saveasfc_callback (fc, path, stats)
    File_chooser  fc;
    char         *path;
    struct stat  *stats;
{
    char  *label = (char *)xv_get (fc, XV_LABEL);

    /* clear footer mesage */
#ifdef FILECHOOSER
    xv_set(saveasfc->saveasfc, FRAME_LEFT_FOOTER, "", NULL);
#else
    xv_set(saveas->saveas, FRAME_LEFT_FOOTER, "", NULL);
#endif

    if (strcmp (label, LGET ("Image Tool:  Save Page As Image")) == 0)
      return (save_newfile (fc, path, SAVEPAGEASIMAGE));
    else if (strcmp (label, LGET ("Image Tool:  Save Selection As")) == 0)
      return (save_newfile (fc, path, SAVESELECTIONAS));
    else
      return (save_newfile (fc, path, SAVEAS));

}
#endif

int
save_format_supported (type_info)
    TypeInfo *type_info;
{
    int  i;
    int  value = FALSE;

    for (i = 0; i < ntypes; i++) {
      if ((all_types[i].save_func != NULL) &&
          (type_info->type == all_types[i].type)) {
        value = TRUE;
        break;
      }
    }

    return value;
	      
}

void
set_saveas_list ()
{
    int		i, row = 0;
    Xv_opaque   list;
#ifdef FILECHOOSER
    list = saveasfc->format_list;
#else
    list = saveas->format_list;
#endif
/*
 * Set the saveas list, to the list in all_types, adding client
 * data, so we can easily determine the choice when the user selects
 * something.
 */

    for (i = 0; i < ntypes; i++)
	if (all_types[i].save_func != NULL) {
	   xv_set (list,
			PANEL_LIST_INSERT, row,
			PANEL_LIST_STRING, row, all_types[i].popup_list_name,
			PANEL_LIST_CLIENT_DATA, row, i,
			NULL);
	   row++;
	   }

    xv_set (list,
		PANEL_LIST_SORT, PANEL_FORWARD,
		NULL);

}

#ifdef NEVER
int
longest_colors_value()
{
   char *save = (char *) xv_get (saveasfc->colors_value, PANEL_LABEL_STRING);
   int   longest;
   int   i, nitems;
   Menu  colors_menu;
   char *str;

   longest = xv_get (saveasfc->colors_value, XV_WIDTH);
   colors_menu = (Menu) xv_get (saveasfc->colors, PANEL_ITEM_MENU);
   nitems = xv_get (colors_menu, MENU_NITEMS);

   
   
   for (i = nitems; i > 0; i--) {
     str = (char *) xv_get ( xv_get (colors_menu, MENU_NTH_ITEM, i), MENU_STRING);
     xv_set (saveasfc->colors_value, PANEL_LABEL_STRING, str, NULL);
     longest = Max (longest, xv_get (saveasfc->colors_value, XV_WIDTH));
   }

   xv_set (saveasfc->colors_value, PANEL_LABEL_STRING, save, NULL);

   return longest;

}
#endif

/*
 * File chooser resize func for saveas.
 */
int
saveas_resize_func (fc, frame_rect, exten_rect, left_edge, right_edge, max_height)
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
    Rect      *list_rect;
    int        panel_x_gap;
    int        panel_y_gap;
    int        longest;
    Xv_opaque  file_list, panel, doc_name;

    sb = (Scrollbar) xv_get (saveasfc->format_list, PANEL_LIST_SCROLLBAR);
    sb_width = (int) xv_get (sb, XV_WIDTH);
    value_x = (int) xv_get (saveasfc->format_list, PANEL_VALUE_X);
    item_x = (int) xv_get (saveasfc->format_list, XV_X);
/*
 * Fit the format list to be same as file list.
 */
    file_list = xv_get (saveasfc->saveasfc, FILE_CHOOSER_CHILD, 
			FILE_CHOOSER_FILE_LIST_CHILD);
    doc_name = xv_get (saveasfc->saveasfc, FILE_CHOOSER_CHILD,
		       FILE_CHOOSER_DOCUMENT_NAME_CHILD);
    /*
     * Figure out how wide the list should be.  Take the rightmost
     * alignment position, subtract out the space used for the label,
     * the left edge, and the little space between the abel and the
     * panel value.  Also, PANEL_LIST_WIDTH does not include the 
     * scrollbar width, so subtract that also.
     */
    list_width = right_edge - left_edge - (value_x - item_x) - sb_width;

    xv_set (saveasfc->format_list,
	   XV_X,		xv_get (file_list, XV_X),
	   XV_Y,		exten_rect->r_top,
	   PANEL_LIST_WIDTH,	xv_get (file_list, PANEL_LIST_WIDTH),
	   PANEL_PAINT, 	PANEL_NONE,
	   NULL);

    list_rect = (Rect *) xv_get (saveasfc->format_list, XV_RECT);
    panel = xv_get (saveasfc->saveasfc, FRAME_CMD_PANEL);
    panel_x_gap = xv_get (panel, PANEL_ITEM_X_GAP);
    panel_y_gap = xv_get (panel, PANEL_ITEM_Y_GAP);
    longest = 48;  /* fix later */
    xv_set (saveasfc->compression, 
	    XV_X, xv_get (doc_name, XV_X),
	    XV_Y, xv_get (saveasfc->format_list, XV_Y) + list_rect->r_height + 30,
	    NULL);
    xv_set (saveasfc->compression_value, 
	    XV_X, xv_get (saveasfc->compression, XV_X) +
	          xv_get (saveasfc->compression, XV_WIDTH) +
	          panel_x_gap,
	    XV_Y, xv_get (saveasfc->compression, XV_Y),
	    NULL);

    xv_set (saveasfc->colors,
	    XV_X, right_edge - 30 - xv_get (saveasfc->colors, XV_WIDTH) -
	          longest,
	    XV_Y, xv_get (saveasfc->compression_value, XV_Y),
	    NULL);
    xv_set (saveasfc->colors_value,
	    XV_X, xv_get (saveasfc->colors, XV_X) +
  	          xv_get (saveasfc->colors, XV_WIDTH) +
	          panel_x_gap,
	    XV_Y, xv_get (saveasfc->colors, XV_Y),
	    NULL);
	    
    return -1;

}
