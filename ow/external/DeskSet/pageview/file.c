#ifndef lint
static char *sccsid = "@(#)file.c 3.4 93/08/19";
#endif

/*
 * Copyright (c) 1990 - Sun Microsystems Inc.
 */

/*
 * file.c - file input for pageview.
 */

#include <sys/stat.h>
#include <unistd.h>
#include "ds_item.h"
#include "pageview.h"
#ifdef FILECHOOSER
#include <xview/file_chsr.h>
#endif

Frame file_chooser;

int
file_load(fname)
    char       *fname;
{
    FILE       *tmpfp;
    struct stat file_info;

    if (stat (fname, &file_info) == 0) {
       if (S_ISDIR (file_info.st_mode)) {
          notice_prompt (baseframe, NULL,
                  NOTICE_MESSAGE_STRINGS, EGET("Cannot load directory"),
                                          fname,
                                          NULL,
                  NOTICE_BUTTON, LGET("Ok"), 1,
                  NULL);
          setactive();
          return -2;
          }
       if (!S_ISREG (file_info.st_mode)) {
          notice_prompt (baseframe, NULL,
                  NOTICE_MESSAGE_STRINGS,
                                fname,
                                EGET("is not a regular file.\nCannot load"),
                                NULL,
                  NOTICE_BUTTON, LGET("Ok"), 1,
                  NULL);
          setactive();
          return -2;
          }
       if (access (fname, R_OK)) {
          notice_prompt (baseframe, NULL,
                  NOTICE_MESSAGE_STRINGS,
                                EGET("You don't have permission to read"),
                                fname,
                                NULL,
                  NOTICE_BUTTON, LGET("Ok"), 1,
                  NULL);
          setactive();
          return -2;
          }
       if (file_info.st_size == 0) {
          notice_prompt (baseframe, NULL,
                  NOTICE_MESSAGE_STRING, EGET("Empty file!"),
                  NOTICE_BUTTON, LGET("Ok"), 1,
                  NULL);
          setactive();
          return -2;
          }
       }

    if ((tmpfp = fopen(fname, "r")) == 0)
	return -1;

    newfile(fname, tmpfp);
    return 0;
}

#ifdef FILECHOOSER
static int
fc_load_callback (fc, path, file, client_data)
    File_chooser   fc;
    char          *path;
    char          *file;
    Xv_opaque      client_data;
{
    int		file_result;

    setbusy();
    if ((file_result = file_load (path)) == -1) {
	file_not_found(path);
	setactive();
        }
    else if (file_result == 0) {
       set_icon_label (path);
       edit_file(path);
       if (!(int) xv_get ((xv_get ((xv_get (fc, XV_OWNER)), XV_OWNER)),
				FRAME_CMD_PUSHPIN_IN))
	  xv_set (fc, WIN_SHOW, FALSE, NULL);
       setactive();
       }
}

#else

static void
load_callback(item, event)
    Panel_item  item;
    Event      *event;
{
    char       *dir = (char *) xv_get(xv_get(item, XV_KEY_DATA, UI1),
				      PANEL_VALUE);
    char       *file = (char *) xv_get(xv_get(item, XV_KEY_DATA, UI2),
				       PANEL_VALUE);
    char        tmpfname[1024];
    char	tmpdir [MAXPATHLEN];
    int		file_result;

    setbusy();
    ds_expand_pathname (dir, tmpdir);

    if (file[0] == '\0') 
       sprintf (tmpfname, DGET ("%s"), tmpdir);
    else if (dir[0] == '\0')
       sprintf (tmpfname, DGET ("%s"), file);
    else
       sprintf(tmpfname, DGET("%s/%s"), tmpdir, file);

    if ((file_result = file_load(tmpfname)) == -1) {
	file_not_found(tmpfname);
	setactive();
        }
    else if (file_result == 0) {
       set_icon_label (tmpfname);
       edit_file(tmpfname);
       if ((Panel_item_type) xv_get (item, PANEL_ITEM_CLASS) == PANEL_TEXT_ITEM)
          if ( !(int) xv_get ((xv_get ((xv_get (item, XV_OWNER)), XV_OWNER)), 
							FRAME_CMD_PUSHPIN_IN))
             xv_set ((xv_get ((xv_get (item, XV_OWNER)), XV_OWNER)), 
	  						WIN_SHOW, FALSE, NULL);

       setactive();
       }

}

#endif

Frame
init_file(parent)
    Frame       parent;
{
    Panel       	 panel;
    Panel_button_item 	 but;
    Panel_text_item 	 dir;
    Panel_text_item 	 file;
    char		 fr_label [100];
    char		*f_label;

    f_label = LGET ("File");
 
    sprintf (fr_label, "%s: %s", PV_Name, f_label);

#ifdef FILECHOOSER
    file_chooser = xv_create (parent, FILE_CHOOSER_OPEN_DIALOG,
			    XV_LABEL, fr_label,
			    FILE_CHOOSER_NOTIFY_FUNC, fc_load_callback,
			    FILE_CHOOSER_DIRECTORY, Directory,
#ifdef OW_I18N
			    WIN_USE_IM,	TRUE,
#endif
			    NULL);
#else
    file_chooser = (Frame) xv_create(parent, FRAME_CMD,
			      FRAME_LABEL, fr_label,
#ifdef OW_I18N
				 WIN_USE_IM, TRUE,
#endif
			      NULL);
    panel = (Panel) xv_get(file_chooser, FRAME_CMD_PANEL);

    dir = xv_create(panel, PANEL_TEXT,
		    PANEL_LABEL_STRING, LGET("Directory:"),
		    XV_Y, xv_row(panel, 0),
		    PANEL_VALUE_DISPLAY_LENGTH, 40,
		    XV_HELP_DATA, "pageview:directory",
		    NULL);

    if (Directory[0] != NULL)
       xv_set (dir, PANEL_VALUE, Directory, NULL);

    file = xv_create(panel, PANEL_TEXT,
		     PANEL_LABEL_STRING, LGET("File:"),
		     XV_Y, xv_row(panel, 1),
		     PANEL_VALUE_DISPLAY_LENGTH, 40,
		     XV_HELP_DATA, "pageview:filename",
		     PANEL_NOTIFY_LEVEL, PANEL_SPECIFIED,
		     PANEL_NOTIFY_STRING, "\r\n",
		     PANEL_NOTIFY_PROC, load_callback,
		     XV_KEY_DATA, UI1, dir,
		     NULL);

    xv_set (file, XV_KEY_DATA, UI2, file, NULL);

	ds_justify_items(panel, 0);

    but = xv_create(panel, PANEL_BUTTON,
		    PANEL_LABEL_STRING, LGET("Load File"),
		    XV_KEY_DATA, UI1, dir,
		    XV_KEY_DATA, UI2, file,
		    PANEL_NOTIFY_PROC, load_callback,
		    XV_X, 0,
		    XV_Y, xv_row(panel, 2),
		    XV_HELP_DATA, "pageview:loadfile",
		    NULL);
   
    xv_set (but, PANEL_NOTIFY_STATUS, XV_ERROR, NULL);
    xv_set (panel, PANEL_DEFAULT_ITEM, but, NULL);

    window_fit(panel);
    xv_set(file_chooser,
	   XV_WIDTH, xv_get(panel, XV_WIDTH),
	   XV_HEIGHT, xv_get(panel, XV_HEIGHT),
	   NULL);

    xv_set(but, XV_X, (xv_get(panel, XV_WIDTH) - xv_get(but, XV_WIDTH)) / 2,
	   NULL);
#endif

    return (file_chooser);
}
