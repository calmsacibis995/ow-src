#ifndef lint
static char sccsid[] = "@(#)menus.c 1.65 96/06/18";
#endif

/*
 * Copyright (c) 1986, 1987, 1988 by Sun Microsystems, Inc.
 */

#include <locale.h>
#include <netdb.h>
#include <unistd.h>
#include <sys/param.h>
#include <xview/panel.h>
#include <xview/scrollbar.h>
#include "display.h"
#include "image.h"
#include "imagetool.h"
#include "props.h"
#include "state.h"
#include "ui_imagetool.h"
#include "ds_popup.h"

#define DEFAULT_HELP_DIRECTORY   "$OPENWINHOME/lib/help"
#define HANDBOOK_FILE            "handbooks/imagetool.handbook.Z"
/*
 * menus.c - callback routines for menu items.
 */
extern int  choice_rows;

/*
 * Menu handler for `file_menu (Open...)'.
 */

Menu_item
open_callback(item, op)
	Menu_item	item;
	Menu_generate	op;
{

	switch (op) {
	case MENU_NOTIFY:
	 	setbusy ();
#ifdef FILECHOOSER
		if (!openfc) {
		  openfc = OpenfcObjectsInitialize (base_window->base_window);
  		  ds_position_popup (base_window->base_window, openfc, 
				     DS_POPUP_CENTERED);
       		  xv_set (openfc, FILE_CHOOSER_DIRECTORY, prog->directory, NULL);
		  if (current_image)
		    set_current_doc (openfc, basename (current_image->file));
		}
		xv_set (openfc, XV_SHOW, TRUE, NULL);
#else
		if (xv_get (openf->open, XV_SHOW) == FALSE) {
		   if (!positioned) {
  		     ds_position_popup (base_window->base_window, 
				        openf->open, DS_POPUP_LOR);
		     positioned = TRUE;
		   }
		}
		xv_set(openf->open, XV_SHOW, TRUE, NULL );
#endif
	 	setactive ();
		break;
	}
	return item;
}


/*
 * Menu handler for `file_menu (Open As...)'.
 */
Menu_item
openas_callback(item, op)
	Menu_item	item;
	Menu_generate	op;
{	

	switch (op) {
	case MENU_NOTIFY:
		setbusy ();
#ifdef FILECHOOSER
		if (!openasfc) {
		   openasfc = OpenasfcObjectsInitialize (NULL, base_window->base_window);
 		   set_openas_list ();	
		   ds_position_popup (base_window->base_window, 
				      openasfc->openasfc, DS_POPUP_CENTERED);
		   show_list_selection (openasfc->format_list);
		   xv_set (openasfc->openasfc, FILE_CHOOSER_DIRECTORY,
			   prog->directory, NULL);
		   if (current_image)
		     set_current_doc (openasfc->openasfc, basename (current_image->file));
		}
		xv_set(openasfc->openasfc, XV_SHOW, TRUE, NULL );
#else
		if (!openas) {
		   openas = OpenasObjectsInitialize (NULL, 
						     base_window->base_window);
		   set_openas_list ();
 		   if (xv_get (openas->openas, XV_SHOW) == FALSE) 
		     ds_position_popup (base_window->base_window, 
				        openas->openas, DS_POPUP_LOR);
		}
		xv_set(openas->openas, XV_SHOW, TRUE, NULL);
#endif
		setactive ();
		break;
	}
	return item;
}


/*
 * Menu handler for `file_menu (Save)'.
 */
Menu_item
save_callback(item, op)
	Menu_item	item;
	Menu_generate	op;
{
        char       *tmp_file;
	Xv_opaque   doc_name;

	switch (op) {
	   case MENU_NOTIFY:
		setbusy ();
#ifdef FILECHOOSER
		if (!savefc) {
		  savefc = SavefcObjectsInitialize (base_window->base_window);
		  ds_position_popup (base_window->base_window, savefc, DS_POPUP_CENTERED);
		  xv_set (savefc, FILE_CHOOSER_DIRECTORY, prog->directory, NULL);
		}

		if (current_image) {
		  doc_name = xv_get (savefc, FILE_CHOOSER_CHILD, 
	     			     FILE_CHOOSER_DOCUMENT_NAME_CHILD);
                  xv_set (doc_name, PANEL_VALUE, basename (current_image->file), NULL);
 	        }

		xv_set (savefc, XV_SHOW, TRUE, NULL);
#else
/*
 * Need a save pop up now... 
 */
                save_newfile (NULL, current_image->file, SAVE);	
		free (tmp_file);
#endif
		setactive ();	
		break;
	   }
	return item;
}

/*
 * Menu handler for `file_menu (Save As...)',
 * and `file_menu (Save Selection As...)'.
 */
Menu_item
saveas_callback(item, op)
	Menu_item	item;
	Menu_generate	op;
{
	char       *str;
	Xv_opaque   doc_name;
	Xv_opaque   save_button;
	int         row;
	char        newname [MAXPATHLEN];
	
	switch (op) {
	case MENU_NOTIFY:
		setbusy ();
#ifdef FILECHOOSER
/*
 * Create saveas popup.
 * Set the current selection on format list to be
 *  the current file format loaded in.
 */
		if (!saveasfc) {
		   saveasfc = SaveasfcObjectsInitialize (NULL, base_window->base_window);
		   ds_position_popup (base_window->base_window, 
				       saveasfc->saveasfc, DS_POPUP_CENTERED);
		   set_saveas_list ();
	    	   row = current_list_selection (saveasfc->format_list);
 		   if (row != -1)
 		     check_image_format (saveasfc->format_list, NULL, NULL,
					 PANEL_LIST_OP_SELECT, NULL, row);
		   show_list_selection (saveasfc->format_list);
		   xv_set (saveasfc->saveasfc, 
			   FILE_CHOOSER_DIRECTORY, prog->directory, 
			   NULL);
	        }

		if (current_image) {
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
		  save_button = xv_get (saveasfc->saveasfc, FILE_CHOOSER_CHILD,
					FILE_CHOOSER_SAVE_BUTTON_CHILD);
		  xv_set (save_button, PANEL_INACTIVE, FALSE, NULL);
 	        }

                str = (char * )xv_get (item, MENU_STRING);
                if (strcmp (str, LGET ("Save As...")) == 0) 
		  xv_set (saveasfc->saveasfc, 
			  XV_LABEL, LGET ("Image Tool:  Save As"),
			  XV_SHOW, TRUE, NULL);
		else if (strcmp (str, LGET ("Save Page As Image...")) == 0)
		  xv_set (saveasfc->saveasfc,
			  XV_LABEL, LGET ("Image Tool:  Save Page As Image"),
			  XV_SHOW, TRUE, NULL);
			  
		else 
		  xv_set (saveasfc->saveasfc, 
			  XV_LABEL, LGET ("Image Tool:  Save Selection As"),
			  XV_SHOW, TRUE, NULL);
#else
		if (!saveas) {
		   saveas = SaveasObjectsInitialize (NULL,
						     base_window->base_window);
		   ds_position_popup (base_window->base_window, 
				      saveas->saveas, DS_POPUP_LOR);
		   set_saveas_list ();
		 }

		str = (char *)xv_get( item, MENU_STRING );
                if ( strcmp( str, LGET ("Save As...")) == 0 ) {
		  xv_set( saveas->saveas, XV_LABEL, LGET ("Save As"), NULL );
	          xv_set( saveas->save_selection_button, XV_SHOW, FALSE, NULL );
	          xv_set( saveas->save_button, XV_SHOW, TRUE, NULL );
		}
	        else {
		  xv_set( saveas->saveas, XV_LABEL, LGET ("Save Selection As"), NULL );
	          xv_set( saveas->save_button, XV_SHOW, FALSE, NULL );
	          xv_set( saveas->save_selection_button, XV_SHOW, TRUE, NULL );

                }

		xv_set(saveas->saveas, XV_SHOW, TRUE, NULL );
#endif
		setactive ();
		break;

	}
	return item;
}

/*
 * Menu handler for `saveas_depth_menu (Save As...)'.
 */
void
saveas_depth_callback(menu, menu_item)
	Menu	        menu;
	Menu_item	menu_item;
{
	char   *str;
	
	str = (char *)xv_get( menu_item, MENU_STRING );
        xv_set( saveas->depth_value, PANEL_LABEL_STRING, str, NULL );
}

#ifdef FILECHOOSER
/*
 * Menu handler for `saveas_colors_menu (Save As...)'.
 */
void
saveas_colors_callback(menu, menu_item)
	Menu	        menu;
	Menu_item	menu_item;
{
	char   *str;
	
	str = (char *)xv_get (menu_item, MENU_STRING);
        xv_set (saveasfc->colors_value, PANEL_LABEL_STRING, str, NULL);
	xv_set (menu, MENU_NOTIFY_STATUS, XV_ERROR, NULL);
      }

#endif

/*
 * Menu handler for `saveas_compression_menu (Save As...)'.
 */
void
saveas_compression_callback(menu, menu_item)
	Menu	        menu;
	Menu_item	menu_item;
{
	char   *str;
	
	str = (char *)xv_get( menu_item, MENU_STRING );
#ifdef FILECHOOSER
        xv_set (saveasfc->compression_value, PANEL_LABEL_STRING, str, NULL);
#else
        xv_set (saveas->compression_value, PANEL_LABEL_STRING, str, NULL);
#endif
	xv_set (menu, MENU_NOTIFY_STATUS, XV_ERROR, NULL);
}

/*
 * Menu handler for `file_menu (Print Preview...)'.
 */
Menu_item
print_preview_callback(item, op)
	Menu_item	item;
	Menu_generate	op;
{
	switch (op) {
	   case MENU_NOTIFY:
	        setbusy ();
		if (!print_preview) {
		   print_preview = PrintPreviewObjectsInitialize (NULL,
						base_window->base_window);
		   ds_position_popup (base_window->base_window, 
				      print_preview->print_prev, DS_POPUP_LOR);

#ifdef NEVER
/*
 * Create print popup when print_preview selected
 * so we can update print options if panning on preview.
 */
		   if (!print) {
		     print = PrintObjectsInitialize (NULL, 
						     base_window->base_window);
		     ds_position_popup (print_preview->print_prev,
					print->print, DS_POPUP_LOR);

		   if ((current_image->type_info->type == POSTSCRIPT) ||
		       (current_image->type_info->type == EPSF))
		     set_ps_print_options ();
		   else
		     set_image_print_options ();
		
		   }
#endif

		}
		xv_set (print_preview->print_prev, 
					XV_SHOW, TRUE, 
					FRAME_CMD_PUSHPIN_IN, TRUE,
					NULL ); 
		position_image ();
		setactive ();
		break;
	   }
	return item;
}

/*
 * Menu handler for `file_menu (Print One)'.
 */
Menu_item
print_one_callback(item, op)
	Menu_item	item;
	Menu_generate	op;
{
	switch (op) {
	   case MENU_NOTIFY:
	        if (printer_exists () == TRUE)
		   print_button_notify_proc (NULL, NULL);
		break;
	   }
	return item;
}
/*
 * Menu handler for `file_menu (Print...)'.
 */
Menu_item
print_callback(item, op)
	Menu_item	item;
	Menu_generate	op;
{
	switch (op) {
	   case MENU_NOTIFY:
		if (printer_exists () == TRUE) {
		   setbusy ();
		   if (!print) {
		      print = PrintObjectsInitialize (NULL, 
						      base_window->base_window);
	              ds_position_popup (base_window->base_window, 
				         print->print, DS_POPUP_LOR);

	              if (current_display == ps_display)
		   	 set_ps_print_options ();
	   	      else
		   	 set_image_print_options ();
		
		      if (print_preview != (PrintPreviewObjects *) NULL)
			 update_margins ();
		      }
	
		   xv_set (print->print, XV_SHOW, TRUE, 
				         NULL);
		   setactive ();
		   }
		break;
	   }
	return item;
}

/*
 * Menu handler for `view_menu (Palette...)'.
 */
Menu_item
palette_callback(item, op)
	Menu_item	item;
	Menu_generate	op;
{
	switch (op) {
	   case MENU_NOTIFY:
		if (!palette) {
		   palette = PaletteObjectsInitialize (NULL,
						     base_window->base_window);
		   set_zoom_and_rotate ();
		   cursor = CursorObjectsInitialize (NULL,
						     base_window->base_window);
		   ds_position_popup (base_window->base_window, 
				      palette->palette, DS_POPUP_RIGHT );
		   }

		xv_set (palette->palette, 
				XV_SHOW, TRUE, 
				FRAME_CMD_PUSHPIN_IN, TRUE,
				NULL );
		break;
	   }
	return item;
}

/*
 * Menu handler for `view_menu (Image Info...)'.
 */
Menu_item
imageinfo_callback(item, op)
	Menu_item	item;
	Menu_generate	op;
{
	switch (op) {
	   case MENU_NOTIFY:
		setbusy ();
		if (!imageinfo) {
		   imageinfo = ImageInfoObjectsInitialize (NULL, 
						base_window->base_window);
		   ds_position_popup (base_window->base_window, 
				      imageinfo->imageinfo, DS_POPUP_LOR );
		 }
		update_imageinfo (current_image);

		xv_set(imageinfo->imageinfo, 
				XV_SHOW, TRUE, 
				FRAME_CMD_PUSHPIN_IN, TRUE,
				NULL );
		setactive ();
		break;
	   }
	return item;
}

/*
 * Menu handler for `view_menu (Page View...)'.
 */
Menu_item
pageview_callback(item, op)
	Menu_item	item;
	Menu_generate	op;
{
	switch (op) {
	   case MENU_NOTIFY:
		setbusy ();
		if (!pageview) {
		   pageview = PageviewObjectsInitialize (NULL,
                                        base_window->base_window, 
				        current_image->pages);

		   if (current_state->reversed == TRUE)
		      reverse_pageview_pages (pageview, current_image->pages,
					      TRUE);
		   set_pageview_pages (current_state->current_page);
		   ds_position_popup (base_window->base_window,
				      pageview->pageview, DS_POPUP_LOR);
	  	   }

		else if (pageview->displayed == FALSE) {
		   pageview->pages = pageview_pages_create (pageview,
                                        pageview->controls2, 
					current_image->pages);
		   if (current_state->reversed == TRUE)
		      reverse_pageview_pages (pageview, current_image->pages,
					      TRUE);
		   set_pageview_pages (current_state->current_page);
		   }

		pageview->displayed = TRUE;
		xv_set (pageview->pageview, 
				FRAME_CMD_PUSHPIN_IN,	TRUE,
				XV_SHOW, 		TRUE, 
				NULL );
		setactive ();
		break;
	   }
	return item;
}

/*
 * Menu handler for `edit_menu (Properties...)'.
 */
Menu_item
props_callback(item, op)
	Menu_item	item;
	Menu_generate	op;
{
	switch (op) {
	   case MENU_NOTIFY:
		/*
		 * Create the props sheet if not already created.
		 * Set the props sheet to what is reflected in the
		 * resource database.
		 */
		if (!props) {
		   props = PropsObjectsInitialize (NULL,
						   base_window->base_window);
		   ds_position_popup (base_window->base_window, 
				      props->props, DS_POPUP_LOR);
		   set_props_sheet();
		}
		xv_set (props->props, XV_SHOW, TRUE, NULL );
		break;
	   }
	return item;
}

/*
 * Menu handler for `edit_menu (Properties...)'.
 */
Menu_item
ps_options_callback (item, op)
	Menu_item	item;
	Menu_generate	op;
{
	switch (op) {
	   case MENU_NOTIFY:
		if (!ps_options) {
		   ps_options = PsOptionsObjectsInitialize (NULL,
                                                base_window->base_window);
                   ds_position_popup (base_window->base_window,
                                      ps_options->ps_options, DS_POPUP_LOR );
                 }

                xv_set(ps_options->ps_options, XV_SHOW, TRUE, NULL );

		break;
	   }
	return item;
}

/*
 * Help Button callback
 */
void
help_callback( item, event )
    Panel_item    item;
    Event        *event;
{
    char   command [MAXPATHLEN + 2];
    char  *helppath;
    char  *helppath_copy;
    char  *helpdir = NULL;
    char  *display_str;
    char  *locale;
    char  *ptr;
    char   help_buffer[MAXPATHLEN];
    char   buffer[MAXHOSTNAMELEN + 6];
    int    screen_no;
    int    found = FALSE;

    setbusy();

    helppath = (char *) getenv ("HELPPATH");
    if (!helppath)
      helppath = DEFAULT_HELP_DIRECTORY;

    locale = (char *) xv_get (base_window->base_window, XV_LC_BASIC_LOCALE);
    helppath_copy = strdup (helppath);
/*
 * First search HELPPATH for locale specific help file.
 */
    if (locale) {
      helpdir = strtok (helppath_copy, ":");
      do {
        sprintf (help_buffer, "%s/%s/help/%s", helpdir, locale, HANDBOOK_FILE);
        if (access (help_buffer, F_OK) == 0) {
          found = TRUE;
          break;
        }
      } while (helpdir = strtok (NULL, ":"));
    }

/*
 * If locale specific help file not found,
 * fallback on helpdir/filename.
 */
    if (!found) {
      strcpy (helppath_copy, helppath);
      helpdir = strtok (helppath_copy, ":");
      do {
        sprintf (help_buffer, "%s/%s", helpdir, HANDBOOK_FILE);
        if (access (help_buffer, F_OK) == 0) {
          found = TRUE;
          break;
        }
      } while (helpdir = strtok (NULL, ":"));
    }

    free (helppath_copy);

    if (found == FALSE) {
      display_error (base_window->base_window, 
		     EGET ("Unable to start Help Viewer: Handbook not found."));
      setactive();
      return;
    }
/* 
 * Insure the the helpviewer comes up on the same display
 * as the client application.
 */
    display_str = DisplayString (current_display->xdisplay);

/*
 * display_str can be in the form: 
 *   ":0.0"               local
 *   "localhost:0.0"      local
 *   "unix:0.0"           local
 *   "hostname:0.0"       remote
 * Parse up to the ":" and do a putenv() if remote host.
 */
    ptr = display_str;
    while (*ptr)
      ptr++;
    while (*ptr != ':')
      ptr--;
    *ptr = NULL;

    if ((strcmp (display_str, prog->hostname) != 0 &&
         strcmp (display_str, "localhost") != 0 &&
         strcmp (display_str, "") != 0)) {
      *ptr = ':';
      sprintf (buffer, "DISPLAY=%s", display_str);
      putenv (buffer);
    }
    else
      *ptr = ':';

    if (getenv ("OPENWINHOME") != (char *) NULL)
      sprintf (command, DGET ("/bin/sh -c \"$OPENWINHOME/bin/helpopen %s > /dev/null 2>/dev/null\""), help_buffer);
    else
      sprintf (command, DGET ("/bin/sh -c \"/usr/openwin/bin/helpopen %s > /dev/null 2>/dev/null\""), help_buffer);
/*
 * Executute the command.
 */
    system (command);

    setactive();
}

