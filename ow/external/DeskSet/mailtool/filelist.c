/*	Copyright (c) 1990, Sun Microsystems, Inc.  All Rights Reserved.
	Sun considers its source code as an unpublished, proprietary
	trade secret, and it is available only under strict license
	provisions.  This copyright notice is placed here only to protect
	Sun in the event the source is deemed a published work.  Dissassembly,
	decompilation, or other means of reducing the object code to human
	readable form is prohibited by the license agreement under which
	this code is provided to the user or company in possession of this
	copy.

	RESTRICTED RIGHTS LEGEND: Use, duplication, or disclosure by the 
	Government is subject to restrictions as set forth in subparagraph 
	(c)(1)(ii) of the Rights in Technical Data and Computer Software 
	clause at DFARS 52.227-7013 and in similar clauses in the FAR and 
	NASA FAR Supplement. */
#ifndef lint
#ifdef sccs
static  char sccsid[] = "@(#)filelist.c 3.12 07/08/97 Copyr 1990 Sun Micro";
#endif
#endif

/*
 * Mailtool - Mail Files popup
 */

#include <stdio.h>
#include <stdio.h>
#include <signal.h>
#ifdef SVR4
#include <unistd.h>
#include <sys/fcntl.h>
#endif SVR4
#include <string.h>
#include <errno.h>
#include <sys/file.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/param.h>

#include <xview/panel.h>
#include <xview/text.h>
#include <xview/font.h>
#include <xview/xview.h>
#include <xview/svrimage.h>
#ifdef FILECHOOSER
#include <xview/file_list.h>
#endif

#include "glob.h"
#include "tool.h"
#include "tool_support.h"
#include "mail.h"
#include "header.h"
#include "attach.h"
#include "ds_popup.h"
#include "mle.h"

#ifndef FILE_LIST

#define MT_INBOX_TYPE	1
#define MT_MAILBOX_TYPE	2
#define MT_DIR_TYPE	3
#define MT_GO_UP_TYPE	4

#define LISTBATCHSIZE	16
#define INSERT_LIST_ROW(r, i)      PANEL_LIST_INSERT,      (r),\
                        PANEL_LIST_STRING,      (r), string_list[(i)],\
                        PANEL_LIST_GLYPH,       (r), glyph_list[(i)],\
                        PANEL_LIST_MASK_GLYPH,  (r), glyph_list[(i)],\
                        PANEL_LIST_CLIENT_DATA, (r), data_list[(i)]

#endif /* !FILE_LIST */

Frame	mt_filelist_frame;
static  int	y_gap, x_gap;

unsigned short	mbox_image[] = {
#include "mbox.glyph"
};

unsigned short	folder_image[] = {
#include "folder.glyph"
};

Server_image	mbox_glyph, folder_glyph;

typedef struct	filelist_popup_data {
	Frame		fpd_frame;
	Panel		fpd_panel;
	Panel_item	fpd_list;
	Panel_item	fpd_name_field;
	Panel_item	fpd_load_button;
	Panel_item	fpd_save_button;
	Panel_item	fpd_create_button;
	Panel_item	fpd_delete_button;
#ifndef FILE_LIST
	char		*fpd_parentdir;
	char		*fpd_curdir;
	int		fpd_curdir_size;
	int		fpd_level;
	time_t		fpd_curdir_time;	/* Time curdir last modified */
#endif /* !FILE_LIST */
	struct header_data	*hd; /* point back to header data */
} Filelist_popup_data;

static	char	save_folder_dir[MAXPATHLEN];
#ifndef FILE_LIST
/* flag used in checking folder dir for Mail Files popup */
static	int     bad_directory = FALSE;
#endif


mt_select_list_item(list, name)

	Panel_item	list;
	char		*name;

{
	int	i;
	char	*string;

	for (i = 0; i < (int) xv_get(list, PANEL_LIST_NROWS); i++) {
		string = (char *)xv_get(list, PANEL_LIST_STRING, i);
		if (strcmp(name, string) == 0) {
			mt_select_and_notify(list, i);
			return;
		}
	}
}

mt_select_and_notify(list, row)

	Panel_item	list;
	int		row;

{
	char		*string;
	Xv_opaque	client_data;
	int		nrows;

	/*
	 * Select the specified row and call the notification proc for
	 * the list.  Remember first row is row 0.
	 */
	nrows = (int)xv_get(list, PANEL_LIST_NROWS);
	if (row < 0)
		row = 0;
	else if (row >= nrows)
		row = nrows - 1;

	xv_set(list, PANEL_LIST_SELECT, row, TRUE, 0);
	string = (char *)xv_get(list, PANEL_LIST_STRING, row);
	client_data = (Xv_opaque)xv_get(list, PANEL_LIST_CLIENT_DATA, row);

	/*
	 * Simulate a notification.
	 * The notify proc only looks at the ie_time field of the event
	 * so this is all we clear out.
	 */
#ifdef FILE_LIST
	list_notify_proc(list, "", string, client_data, PANEL_LIST_OP_SELECT,
					NULL, row);
#else
	list_notify_proc(list, string, client_data, PANEL_LIST_OP_SELECT,
					NULL, row);
#endif
}

mt_get_selected_list_row(list)

	Panel_item	list;

{
	int	i;
	char	*string;

	for (i = 0; i < (int) xv_get(list, PANEL_LIST_NROWS); i++) {
		if ((int)xv_get(list, PANEL_LIST_SELECTED, i))
			return(i);
	}

	return(-1);
}


#ifdef FILE_LIST

static File_list_op
file_filter_func (path, row)

	char		*path;
	File_list_row	*row;

{
	char	buf[64];
	char	*type;
	char	*from = "From ";
	int	bufsize;
	int	fd;

	if (row->stats.st_size == 0) {
		row->vals.glyph = mbox_glyph;
		row->vals.mask_glyph = mbox_glyph;
		return FILE_LIST_ACCEPT;
	}

	bufsize = 0;
	if ((fd = open(path, O_RDONLY)) >= 0) {
		bufsize = read(fd, buf, sizeof(buf));
		if (bufsize < -1)
			bufsize = 0;
												close(fd);
	}

	/*
	 * We don't pass in the file name since we only want to
	 * match on content.
	 */
	type = mt_get_data_type("", buf, bufsize);

	if (strcmp(type, "mail-file") == 0) {
		row->vals.glyph = mbox_glyph;
		row->vals.mask_glyph = mbox_glyph;
		return FILE_LIST_ACCEPT;
	} else if (strncmp(buf, from, 5) == 0)  {
		/* Double check.  CE may be hosed */
		row->vals.glyph = mbox_glyph;
		row->vals.mask_glyph = mbox_glyph;
		return FILE_LIST_ACCEPT;
	} else {
		return FILE_LIST_IGNORE;
	}
}

static int
file_compare_func(row1, row2)

	File_list_row	*row1, *row2;

{
	int	isdir1, isdir2;

	isdir1 = S_ISDIR(row1->stats.st_mode);
	isdir2 = S_ISDIR(row2->stats.st_mode);

	if (isdir1 && !isdir2) {
		return -1;
	} else if (isdir2 && !isdir1) {
		return 1;
	} else {
		return strcoll(row1->vals.string, row2->vals.string);
	}
}

#else

/*
 * set the value of the curdir file.  If the buffer is big enough
 * then just copy it in; otherwise allocate a larger buffer.
 */
static
set_curdir(fpd, string)
Filelist_popup_data *fpd;
char *string;
{
	int len;

	len = strlen(string) + 1;

	if (fpd->fpd_curdir == NULL || len > fpd->fpd_curdir_size) {
		if (fpd->fpd_curdir) {
			free(fpd->fpd_curdir);
		}

		if ((fpd->fpd_curdir = (char *)ck_malloc(len)) == NULL)
			return(-1);
		fpd->fpd_curdir_size = len;
	}

	strcpy(fpd->fpd_curdir, string);
	return(0);
}

#endif

Frame
mt_filelist_create(hd)

        struct header_data      *hd;
{
	Panel			panel;
	Filelist_popup_data	*fpd;
	Notify_value	filelist_event_proc();
	Menu		create_load_menu(), create_save_menu(),
			create_create_menu(), create_edit_menu();
	int			list_notify_proc();
	char			folder_dir[MAXPATHLEN];

#ifdef FILE_LIST
	/* Make sure the initial directory (folder directory) exists */
	mt_getfolderdir(folder_dir);
	if (mt_check_and_create(folder_dir, TRUE, MT_FRAME(hd), TRUE) < 0)
		return NULL;

        strcpy(save_folder_dir, folder_dir);
#endif

	fpd = (Filelist_popup_data *) malloc(sizeof(Filelist_popup_data));
	if (fpd == NULL)
		return(NULL);

	memset((char *)fpd, 0, sizeof(Filelist_popup_data));

	mbox_glyph = xv_create(0, SERVER_IMAGE,
		SERVER_IMAGE_BITS,	mbox_image,
		SERVER_IMAGE_DEPTH,	1,
		XV_WIDTH,		16,
		XV_HEIGHT,		16,
		0);

	folder_glyph = xv_create(0, SERVER_IMAGE,
		SERVER_IMAGE_BITS,	folder_image,
		SERVER_IMAGE_DEPTH,	1,
		XV_WIDTH,		16,
		XV_HEIGHT,		16,
		0);

	fpd->hd = hd;
	fpd->fpd_frame = xv_create(MT_FRAME(hd), FRAME_CMD, 
				WIN_IS_CLIENT_PANE,
				FRAME_CMD_PUSHPIN_IN,	TRUE,
				FRAME_SHOW_LABEL,	TRUE,
				FRAME_SHOW_FOOTER,	TRUE,
				FRAME_SHOW_RESIZE_CORNER,	TRUE,
				WIN_CLIENT_DATA,	fpd,
				XV_SHOW,		FALSE,
				0);

	mt_label_frame(fpd->fpd_frame, gettext("Mail Files"));

	(void)notify_interpose_event_func(fpd->fpd_frame,
					filelist_event_proc, NOTIFY_SAFE);

	panel = xv_get(fpd->fpd_frame, FRAME_CMD_PANEL);
	(void)xv_set(panel,
		XV_X,		0,
		XV_Y,		0,
		WIN_BORDER,	TRUE,
		0);

	fpd->fpd_panel = panel;

	y_gap = (int) xv_get(panel, PANEL_ITEM_Y_GAP);
	x_gap = (int) xv_get(panel, PANEL_ITEM_X_GAP);



	fpd->fpd_name_field = xv_create(panel, PANEL_TEXT,
			XV_X,		xv_col(panel, 0),
			XV_Y,		xv_row(panel, 0),
			PANEL_VALUE_DISPLAY_WIDTH, 10, /* For now. Set for
							  real in resize proc */
			PANEL_LABEL_STRING,	gettext("Name:"),
			XV_HELP_DATA,		"mailtool:FileListName",
			0);

	fpd->fpd_save_button = xv_create(panel, PANEL_BUTTON,
				XV_X,		xv_col(panel, 0),
				XV_Y,		xv_row(panel, 1),
				PANEL_LABEL_STRING, gettext("Save"),
				PANEL_ITEM_MENU,      create_save_menu(fpd),
				XV_HELP_DATA,	"mailtool:FileListSave",
				0);

	fpd->fpd_load_button = xv_create(panel, PANEL_BUTTON,
				XV_Y,		xv_row(panel, 1),
				PANEL_LABEL_STRING, gettext("Load"),
				PANEL_ITEM_MENU,       create_load_menu(fpd),
				XV_HELP_DATA,	"mailtool:FileListLoad",
				0);

	fpd->fpd_create_button = xv_create(panel, PANEL_BUTTON,
				XV_Y,		xv_row(panel, 1),
				PANEL_LABEL_STRING, gettext("Create"),
				PANEL_ITEM_MENU,    create_create_menu(fpd),
				XV_HELP_DATA,	"mailtool:FileListCreate",
				0);

	fpd->fpd_delete_button = xv_create(panel, PANEL_BUTTON,
				XV_Y,		xv_row(panel, 1),
				PANEL_LABEL_STRING, gettext("Edit"),
				PANEL_ITEM_MENU,     create_edit_menu(fpd),
				XV_HELP_DATA,	"mailtool:FileListEdit",
				0);

#ifdef FILE_LIST
	fpd->fpd_list = xv_create(panel, FILE_LIST,
			XV_X,		xv_col(panel, 0),
			XV_Y,		xv_row(panel, 2),
			PANEL_LIST_DISPLAY_ROWS,	8,
			PANEL_LIST_WIDTH,	200,	/* Set for real in
							   resize proc */
			PANEL_CHOOSE_ONE, TRUE,
			PANEL_CHOOSE_NONE, FALSE,
			PANEL_READ_ONLY, TRUE,
			PANEL_NOTIFY_PROC,	list_notify_proc,
			PANEL_CLIENT_DATA,	fpd,
			FILE_LIST_DIRECTORY,	folder_dir,
			FILE_LIST_SHOW_DIR,	TRUE,
			FILE_LIST_USE_FRAME,	TRUE,
			FILE_LIST_COMPARE_FUNC,	file_compare_func,
			FILE_LIST_FILTER_FUNC,	file_filter_func,
			XV_HELP_DATA,	"mailtool:FileListList",
			0);
#else
	fpd->fpd_list = xv_create(panel, PANEL_LIST,
			XV_X,		xv_col(panel, 0),
			XV_Y,		xv_row(panel, 2),
			PANEL_LIST_DISPLAY_ROWS,	8,
			PANEL_LIST_WIDTH,	200,	/* Set for real in
							   resize proc */
			PANEL_CHOOSE_ONE, TRUE,
			PANEL_CHOOSE_NONE, FALSE,
			PANEL_READ_ONLY, TRUE,
			PANEL_NOTIFY_PROC,	list_notify_proc,
			PANEL_LIST_TITLE,	"",
			PANEL_CLIENT_DATA,	fpd,
			XV_HELP_DATA,	"mailtool:FileListList",
			0);
#endif

	/* Size panel around items */
	window_fit(fpd->fpd_panel);

	/* Make sure panel did not shrink too much */
	if ((int)xv_get(fpd->fpd_panel, WIN_COLUMNS) < 45) {
		(void)xv_set(fpd->fpd_panel, WIN_COLUMNS, 45, 0);
	}

	/*
	 * Panel dimensions are now set.  Make sure all items in panel are
	 * layed out nicely
	 */
	filelist_resize(fpd->fpd_frame, TRUE);

	window_fit(fpd->fpd_frame);

	/* From now on panel grows with frame */
	xv_set(panel,
		XV_WIDTH,	WIN_EXTEND_TO_EDGE,
		XV_HEIGHT,	WIN_EXTEND_TO_EDGE,
		0);

#ifndef FILE_LIST
	mt_getfolderdir(folder_dir);
	mt_init_list(fpd, folder_dir);
	if (!bad_directory) {
	   /* 
            * Save string to check in future if it changes 
            */
           strcpy(save_folder_dir, folder_dir);
	}
#endif
	return(fpd->fpd_frame);
}

#ifndef FILE_LIST
mt_init_list(fpd, directory)

	Filelist_popup_data 	*fpd;
	char		*directory;	/* Full path to initial directory */

{
	char	*p;
	char	c;
	int	level;
	char	buf[MAXPATHLEN];

	/*
	 * Point the list to the specified directory and load the directory's
	 * contents
	 */

	*buf = '\0';
	/* Split out the last component of the path */
	if ((p = strrchr(directory, '/')) == NULL)
		return(-1);	/* This routine requires a full path */

	p++;
	c = *p;
	*p = '\0';
	/* Parent directory */
	if ((fpd->fpd_parentdir = (char *)strdup(directory)) == NULL)
		return(-1);
	*p = c;

	/* Current directory */
	if (set_curdir(fpd, p) < 0) {
		free(fpd->fpd_parentdir);
		return(-1);
	}

	fpd->fpd_curdir_size = strlen(fpd->fpd_curdir) + 1;
	fpd->fpd_level = 0;
	strcpy(buf, directory);
	strcat(buf, "/");
	(void)xv_set(fpd->fpd_list, PANEL_LIST_TITLE, fpd->fpd_curdir, 0);
	load_directory(fpd, buf, 0);
	if (bad_directory) return;
	mt_select_and_notify(fpd->fpd_list, 0);
}
#endif

mt_filelist_show(menu, menu_item)

	Menu		menu;
	Menu_item       menu_item;
{
	Filelist_popup_data	*fpd;
        struct header_data      *hd;

        hd = mt_get_header_data(menu);
	if (!mt_filelist_frame) {

             /* save_folder_dir is initally saved in mt_filelist_create() */
		mt_busy(MT_FRAME(hd), TRUE, "", FALSE);
		mt_filelist_frame = mt_filelist_create(hd);
		mt_busy(MT_FRAME(hd), FALSE, NULL, FALSE);
#ifdef FILE_LIST
		if (mt_filelist_frame == NULL) {
			return;
		}
#else
		if (bad_directory) {
			bad_directory = FALSE; /* reset flag */
			return;
		}
#endif
		ds_position_popup(MT_FRAME(hd), mt_filelist_frame, DS_POPUP_LOR);
	} else {
		/*
		 * Now make sure it is up to date 
		 */
		fpd = (Filelist_popup_data *)xv_get(mt_filelist_frame, 
						WIN_CLIENT_DATA);
#ifdef FILE_LIST
		xv_set(fpd->fpd_list, FILE_LIST_UPDATE, 0);
#else
		reload_current_dir(fpd);
		if (bad_directory) {
			bad_directory = FALSE; /* reset flag */
			return;
		}
#endif
	}

	xv_set(mt_filelist_frame, XV_SHOW, TRUE, WIN_FRONT, 0);
}

void
mt_props_dynamic_update_mailfiles_popup()
{
	Filelist_popup_data	*fpd;
        char    new_folder_dir[MAXPATHLEN];

	/* Called by mt_props_apply_proc(), 
	 * it checks for folder variable changes.
	 */
	if (mt_filelist_frame) {
		fpd = (Filelist_popup_data *)xv_get(mt_filelist_frame, 
						WIN_CLIENT_DATA);
           	/* Check if folder changed */
		mt_getfolderdir(new_folder_dir);
                if (strcmp(save_folder_dir, new_folder_dir)) {
                	strcpy(save_folder_dir, new_folder_dir);
#ifdef FILE_LIST
			xv_set(fpd->fpd_list,
				FILE_LIST_DIRECTORY, new_folder_dir,
				0);
#else
                	mt_init_list( fpd, new_folder_dir);
#endif
		}
	}
}


static Notify_value
filelist_event_proc(frame, event, arg, type)

	Frame	frame;
	Event	*event;
	Notify_arg	arg;
	Notify_event_type	type;
{
	Notify_value	value;

	value = notify_next_event_func(frame, (Notify_event)event, arg, type);

	if (event_action(event) == WIN_RESIZE)
		filelist_resize(mt_filelist_frame, TRUE);

	return(value);

}

static
filelist_resize(frame, panel_at_top)
	Frame	frame;
	int	panel_at_top;

{
	int	panel_h, panel_w;
	int	rows;
	int	y, control_height;
	Rect *	rect;
	Filelist_popup_data	*fpd;
	struct header_data *hd;

	fpd = (Filelist_popup_data *) xv_get(frame, WIN_CLIENT_DATA);

	/*
	 * The frame has been re-sized.  Layout the pop-up.  If we didn't
	 * do this then the bottom panel would change in size instead of
	 * the canvas
	 */
	panel_h = (int)xv_get(fpd->fpd_panel, XV_HEIGHT);
	panel_w = (int)xv_get(fpd->fpd_panel, XV_WIDTH);

	/*
	 * Compute the height of the area required by the fillin field and
	 * the buttons
	 */
	rect = (Rect *)xv_get(fpd->fpd_name_field, PANEL_ITEM_RECT);
	control_height = 2 * rect->r_height + 2 * y_gap;

	rows = (panel_h - control_height) / (int)xv_get(fpd->fpd_list,
						PANEL_LIST_ROW_HEIGHT);
	rows -= 3;

	if (rows < 0)
		rows = 1;

	/*
	 * Set the list box dimensions.  Subtract the width of the scrollbar.
	 * We actually subtract the width of the headr canvas scrollbar since
	 * I'm not sure how to get ahold of the list box scrollbar, but they
	 * should be the same width.
	 */
	hd = fpd->hd;
	xv_set(fpd->fpd_list,
		XV_X,			x_gap / 2,
		XV_Y,			panel_at_top ? control_height : 0,
		PANEL_LIST_DISPLAY_ROWS,	rows,
		PANEL_LIST_WIDTH,
			panel_w - mt_get_scrollbar_width(hd->hd_canvas) - x_gap,
		0);

	if (panel_at_top)
		y = y_gap / 2;
	else
		y = panel_h - 2 * (rect->r_height + x_gap);

	xv_set(fpd->fpd_name_field,	
		XV_X,	x_gap / 2,
		XV_Y,	y,
		0);

	mt_resize_fillin(fpd->fpd_panel, fpd->fpd_name_field);

	y += rect->r_height + y_gap / 2;
	xv_set(fpd->fpd_save_button,
		XV_Y,	y,
		0);
	ds_center_items(fpd->fpd_panel, -1,  fpd->fpd_save_button,
		fpd->fpd_load_button, fpd->fpd_create_button,
		fpd->fpd_delete_button, NULL);
}

static Menu
create_load_menu(fpd)

Filelist_popup_data	*fpd;

{
	Menu	menu;
	int	mt_load_proc();

	menu = (Menu) xv_create(NULL, MENU,
		MENU_CLIENT_DATA,	fpd,
		MENU_ITEM,
			MENU_STRING,	gettext("Mail File"),
			MENU_NOTIFY_PROC,	mt_load_proc,
			MENU_VALUE,	0,
			XV_HELP_DATA,	"mailtool:FileListLoadFile",
			0,
		MENU_ITEM,
			MENU_STRING,	gettext("In-Box"),
			MENU_NOTIFY_PROC,	mt_load_proc,
			MENU_VALUE,	2,
			XV_HELP_DATA,	"mailtool:FileListLoadInbox",
			0,
		0);
	
	return(menu);
}

/*
 * Load the selected file
 */
/* ARGSUSED */
mt_load_proc(menu, menu_item)

Menu		menu;
Menu_item	menu_item;
{
	char	buf[MAXPATHLEN + 1];
	Filelist_popup_data	*fpd;

	fpd = (Filelist_popup_data *) xv_get(menu, MENU_CLIENT_DATA);

	switch ((int)xv_get(menu_item, MENU_VALUE)) {
	case 0:
	case 1:
		if (get_file(fpd->fpd_name_field, fpd->fpd_frame,buf) > -1) {
			if (buf[0] == '%' && buf[1] == '\0') {
				mt_new_folder(fpd->hd, "%", mt_system_mail_box,
					mt_system_mail_box, FALSE, TRUE);
			} else {
				mt_new_folder(fpd->hd, buf, FALSE, FALSE, FALSE, TRUE);
			}
		}
		break;
	case 2:
		mt_new_folder(fpd->hd, "%", mt_system_mail_box, mt_system_mail_box, FALSE, TRUE);
		break;
	}

	return;
}

static Menu
create_save_menu(fpd)

Filelist_popup_data	*fpd;

{
	Menu	menu;
	int	mt_list_save_proc();

	menu = (Menu) xv_create(NULL, MENU,
		MENU_CLIENT_DATA,	fpd,
		MENU_ITEM,
			MENU_STRING,	gettext("Move Message into Mail File"),
			MENU_NOTIFY_PROC,	mt_list_save_proc,
			MENU_VALUE,	0,
			XV_HELP_DATA,	"mailtool:FileListMove",
			0,
		MENU_ITEM,
			MENU_STRING,	gettext("Copy Message into Mail File"),
			MENU_NOTIFY_PROC,	mt_list_save_proc,
			MENU_VALUE,	1,
			XV_HELP_DATA,	"mailtool:FileListCopy",
			0,
		0);
	
	return(menu);
}

/*
 * Save the selected message in the specified file.
 */
/* ARGSUSED */
mt_list_save_proc(menu, menu_item)

Menu		menu;
Menu_item	menu_item;
{
	Msg_attr flag;
	char	buf[MAXPATHLEN + 1];
	int	delete;
	struct header_data	*hd;
	Filelist_popup_data	*fpd;
        char    full_path[MAXPATHLEN];

	fpd = (Filelist_popup_data *) xv_get(menu, MENU_CLIENT_DATA);

	hd = fpd->hd;
	if (mt_any_selected(hd))
		flag = MSG_SELECTED;
	else
		flag = MSG_CURRENT;

	delete = ((int)xv_get(menu_item, MENU_VALUE) == 0);
	if (get_file(fpd->fpd_name_field, fpd->fpd_frame, buf) != -1) {
		if (strcmp(buf, "%") == 0) 
			mt_vs_warn(fpd->fpd_frame, gettext(
"Sorry, you may not save messages to the word \"In-Box\".\nYou have to specify %s."), mt_value("MAIL"));
		else {
			/* 
			 * This check for new file is already
			 * done by mt_do_save, but we do it here
			 * again to update the list
			 */
			mt_expand_foldername(buf, full_path);
			if (access(full_path, F_OK) != 0) {
			   /* New file */
				mt_do_save(hd, delete, flag, buf);
#ifdef FILE_LIST
				xv_set(fpd->fpd_list, FILE_LIST_UPDATE, 0);
#else
				reload_current_dir(fpd);
#endif
				mt_select_list_item(fpd->fpd_list, buf);
			} else {
				mt_do_save(hd, delete, flag, buf);
			}
		}
	}

	return;
}

static Menu
create_create_menu(fpd)

Filelist_popup_data	*fpd;

{
	Menu	menu;
	int	dummy_proc(), create_proc(), change_name_proc();

	menu = (Menu) xv_create(NULL, MENU,
		MENU_CLIENT_DATA,	fpd,
		MENU_ITEM,
			MENU_STRING,	gettext("Mail File"),
			MENU_NOTIFY_PROC,	create_proc,
			MENU_VALUE,	0,
			XV_HELP_DATA,	"mailtool:FileListCreateFile",
			0,
		MENU_ITEM,
			MENU_STRING,	gettext("Directory"),
			MENU_NOTIFY_PROC,	create_proc,
			MENU_VALUE,	1,
			XV_HELP_DATA,	"mailtool:FileListCreateDir",
			0,
		0);
	
	return(menu);
}

static
create_proc(menu, menu_item)

	Menu		menu;
	Menu_item	menu_item;

{
	char	buf[MAXPATHLEN + 1], *name;
	Filelist_popup_data	*fpd;
	int	fd;
	int	directory;

	if ((int)xv_get(menu_item, MENU_VALUE) == 1)
		directory = TRUE;
	else
		directory = FALSE;

	fpd = (Filelist_popup_data *) xv_get(menu, MENU_CLIENT_DATA);

	if (get_file(fpd->fpd_name_field, fpd->fpd_frame, buf) != -1) {
		if (strcmp(buf, "%") == 0)  {
			mt_vs_warn(fpd->fpd_frame, gettext(
"Your In-Box already exists"));
			return;
		}

		if (create_file(buf, fpd->fpd_frame, directory) < 0) {
			mt_vs_warn(fpd->fpd_frame, gettext(
				"Could not create \n%s"), buf);
		} else {
			/*
	 	 	 * Force a redisplay of the current directory so new
			 * file appears. This is very ineficient -- we'll fix
			 * it later.
	 	 	 */
#ifdef FILE_LIST
			xv_set(fpd->fpd_list, FILE_LIST_UPDATE, 0);
#else
			reload_current_dir(fpd);
#endif
			if ((name = strrchr(buf, '/')) == NULL)
				name = buf;
			else
				name++;
			mt_select_list_item(fpd->fpd_list, name);
		}
	}
}

static
create_file(file, frame, directory)

	char	*file;
	Frame	frame;
	int	directory;

{
	char	full_path[MAXPATHLEN];
	int	rcode;

	mt_expand_foldername(file, full_path);
	return(mt_check_and_create(full_path, directory, frame, FALSE));
}

static
change_name_proc(menu, menu_item)

	Menu		menu;
	Menu_item	menu_item;

{
	char	old_path[MAXPATHLEN];
	char	new_path[MAXPATHLEN];
	char	*old_name;
	char	*new_name;
	Filelist_popup_data	*fpd;
	int	row;

	fpd = (Filelist_popup_data *) xv_get(menu, MENU_CLIENT_DATA);

	row = mt_get_selected_list_row(fpd->fpd_list);
	old_name = (char *)xv_get(fpd->fpd_list, PANEL_LIST_STRING, row);
	new_name = (char *)xv_get(fpd->fpd_name_field, PANEL_VALUE);

	if (strcmp(old_name, "In-Box") == 0)  {
		mt_vs_warn(fpd->fpd_frame, gettext(
"Sorry, you may not change the name of your In-Box"));
		return;
	}

	if (strcmp(new_name, "In-Box") == 0)  {
		mt_vs_warn(fpd->fpd_frame, gettext(
"Sorry, you may not change the name to In-Box"));
		return;
	}

	if (*new_name == '\0') {
		mt_vs_warn(fpd->fpd_frame, gettext(
			"Please enter the new name in the Name field"));
		return;
	}


	if (strcmp(new_name, old_name) == 0) {
		mt_vs_warn(fpd->fpd_frame, gettext(
			"The file is already named %s.\n\
Please enter a new name in the Name field."), old_name);
		return;
	}

	/* We are just using old_path as a temp buffer here */
	if (get_file(fpd->fpd_name_field, fpd->fpd_frame, old_path) < 0) {
		return;
	}


	mt_expand_foldername(old_path, new_path);

	if (access(new_path, F_OK) == 0) {
		mt_vs_warn(fpd->fpd_frame, gettext(
			"You already have a file named %s.\n\
Please enter a new name in the Name field."), new_path);
		return;
	}

#ifdef FILE_LIST
	strcpy(old_path, (char *)xv_get(fpd->fpd_list, FILE_LIST_DIRECTORY));
	strcat(old_path, "/");
	strcat(old_path, old_name);
#else
	strcpy(old_path, fpd->fpd_parentdir);
	strcat(old_path, fpd->fpd_curdir);
	strcat(old_path, "/");
	strcat(old_path, old_name);
#endif

	if (rename(old_path, new_path) < 0) {
		mt_vs_warn(fpd->fpd_frame, gettext(
			"Could not rename %s to %s\n%s"),
			old_path, new_path, strerror(errno));
		return;
	}

	/*
  	 * Update scrolling list display
  	 */
	 xv_set(fpd->fpd_list, PANEL_LIST_STRING, row, new_name, 0);
}

static Menu
create_edit_menu(fpd)

Filelist_popup_data	*fpd;

{
	Menu	menu;
	int	dummy_proc(), delete_file_proc();

	menu = (Menu) xv_create(NULL, MENU,
		MENU_CLIENT_DATA,	fpd,
		MENU_ITEM,
			MENU_STRING,	gettext("Delete"),
			MENU_NOTIFY_PROC,	delete_file_proc,
			MENU_VALUE,	0,
			XV_HELP_DATA,	"mailtool:FileListDelete",
			0,
		MENU_ITEM,
			MENU_STRING,	gettext("Empty"),
			MENU_NOTIFY_PROC,	delete_file_proc,
			MENU_VALUE,	1,
			XV_HELP_DATA,	"mailtool:FileListEmpty",
			0,
		MENU_ITEM,
			MENU_STRING,	gettext("Rename"),
			MENU_NOTIFY_PROC,	change_name_proc,
			MENU_VALUE,	2,
			XV_HELP_DATA,	"mailtool:FileListRename",
			0,
		0);
	
	return(menu);
}

static
delete_file_proc(menu, menu_item)

	Menu		menu;
	Menu_item	menu_item;

{
	char	buf[MAXPATHLEN + 1];
	Filelist_popup_data	*fpd;
	int	fd;
	int	delete;
	struct header_data *hd;
	int     row;

	fpd = (Filelist_popup_data *) xv_get(menu, MENU_CLIENT_DATA);
	delete = !(int)xv_get(menu_item, MENU_VALUE);
	row = mt_get_selected_list_row(fpd->fpd_list);

	if (get_file(fpd->fpd_name_field, fpd->fpd_frame, buf) != -1) {
		if (strcmp(buf, "%") == 0)  {
			if (delete)
				mt_vs_warn(fpd->fpd_frame, gettext(
"Sorry, you may not delete your In-Box"));
			else
				mt_vs_warn(fpd->fpd_frame, gettext(
"Sorry, you may not empty your In-Box"));
			return;
		}

		hd = fpd->hd;
		if (delete_file(hd, buf, fpd->fpd_frame, delete) == 0) {
			/*
	 	 	 * Force a redisplay of the current directory
	 	 	 */
			if (delete) {
				xv_set(fpd->fpd_list,
					PANEL_LIST_DELETE, row, 0);
				mt_select_and_notify(fpd->fpd_list, row);
			}
		}
	}
}

static
delete_file(hd, file, frame, delete)

	struct header_data *hd;
	char	*file;
	Frame	frame;
	int	delete;

{
	char	full_path[MAXPATHLEN];
	int	rcode;
	int	cancel;
	char	*button_string;
	struct stat	stat_buf;
	int	fd;


	mt_expand_foldername(file, full_path);

	if (stat(full_path, &stat_buf) == -1) {
		mt_vs_warn(frame, gettext("%s\nDoes not exist."), full_path);
		return(0);
	}

	/*
	 * Check if it's a directory.  Note that the rmdir will fail if the
	 * directory is not empty
	 */
	if (stat_buf.st_mode & S_IFDIR) {
		if (rmdir(full_path) < 0) {
			if (errno == ENOTEMPTY) {
mt_vs_warn(frame, gettext("You may not delete directory %s since it contains files\nPlease remove all files contained in this directory and try again"),file);
			} else {
				mt_vs_warn(frame, gettext("Could not delete directory %s\n%s"), full_path, strerror(errno));

			}
			return(-1);
		} else
			return(0);
	}

	button_string = (delete ? gettext("Delete Mail File") :
		gettext("Empty Mail File"));
	cancel = mt_vs_confirm(frame, FALSE,
		gettext("Cancel"), button_string, gettext(
"If you continue with this operation you\n will lose all messages contained in\n %s\nand you will not be able to recover them.\nDo you wish to Cancel this operation?"), file);

	if (cancel)
		return(-1);

	if (delete) {
		if (strcmp(full_path, MT_FOLDER(hd)) == 0) {
			/*
			 * Remove the file which is currently displayed
			 * For now switch to In-Box before we unlink the file
			 * We should probably go to an idle state.
			 */
			mt_new_folder(hd, "%", mt_system_mail_box,
					mt_system_mail_box, FALSE, TRUE);
		}
		rcode = unlink(full_path);
	} else {
		if (strcmp(full_path, MT_FOLDER(hd)) == 0) {
			/*
			 * Emptying the file which is currently displayed
			 * Do it the hard way 
			 */
			mt_truncate_current_folder(hd);
		} else {
			/* Truncate the file */
			if ((fd = open(full_path, O_RDWR | O_TRUNC)) < 0) {
				rcode = -1;
			} else {
				rcode = 0;
				close(fd);
			}
		}
	}

	if (rcode < 0) {
		mt_vs_warn(frame, gettext("Could not delete\n%s\n%s"),
			full_path, strerror(errno));
	}

	return(rcode);
}

#ifndef FILE_LIST
load_directory(fpd, path, level)

	Filelist_popup_data	*fpd;
	char		*path;	/* Must have a trailing / */
	int		level;

{
	register int	n, list_n, len;
	register char	*s;
	static char		**av;
	static char		*strtab;
	struct stat	statb;
	int		ac;
	char		**mt_get_folder_list();
	int		type;
	Server_image	glyph_list[LISTBATCHSIZE];
	int		data_list[LISTBATCHSIZE];
	char		*string_list[LISTBATCHSIZE];
	Server_image	glyph;
	int		index;
	Panel_item	list;
	char		*p;

	list = fpd->fpd_list;

	while ((stat(path, &statb) < 0) &&		/* Doesn't exist */
	       (fpd->fpd_level != 0)) {			/* Aren't at top */

		/*
		 * Change the buffer contain only the directory beneath
		 * HOME.
		 */

		strcpy(path, fpd->fpd_curdir);

		/*
		 * Remove the last-most directory from path.
		 */

		if ((p = strrchr(path, '/')) != NULL)
			*p = '\0';
		else
			*path = '\0';

		/*
		 * Indicate path level ascension.
		 */

		fpd->fpd_level--;
		set_curdir(fpd, path);

		/*
		 * Now, rebuild the directory path.
		 */

		strcpy(path, fpd->fpd_parentdir);
		strcat(path, fpd->fpd_curdir);
		strcat(path, "/");

		}
			
	if (stat(path, &statb) < 0) {			/* Still doesn't */
		if (!mt_check_and_create(path, TRUE, fpd->fpd_frame, TRUE)) {

			bad_directory = TRUE;
			return;
		}
	}

	/* Save modify time */
	fpd->fpd_curdir_time = statb.st_mtime;

	/* Clear list */
	(void)xv_set(list, PANEL_SHOW_ITEM, FALSE, 0);
	mt_clear_list(list);

	/* Free old string table if any */
	if (strtab)
		free(strtab);

	/* Build the list of folders */
	ac = 0;
	av = mt_get_folder_list(NULL, path, &strtab, &ac, TRUE);

	/* Insert first item */
	if (level == 0) {
		xv_set(list,
			PANEL_LIST_INSERT, 0,
			PANEL_LIST_STRING, 0, gettext("In-Box"),
			PANEL_LIST_GLYPH, 0, mbox_glyph,
			PANEL_LIST_CLIENT_DATA, 0 , MT_INBOX_TYPE,
			0);
	} else {
		xv_set(list,
			PANEL_LIST_INSERT, 0,
			PANEL_LIST_STRING, 0, gettext(".. (Go up 1 level)"),
			PANEL_LIST_GLYPH, 0, folder_glyph,
			PANEL_LIST_CLIENT_DATA, 0, MT_GO_UP_TYPE,
			0);
	}

	/* Insert folders */
	index = 0;
	for (n = 0, list_n = 1; n < ac; n++) {
		s = av[n];
		len = strlen(s);

		if (s[len - 1] == '@') {
			/* symbolic link */
			s[len - 1] = '\0';
			len--;
		}

		if (s[len - 1] == '/') {
			glyph = folder_glyph;
			type = MT_DIR_TYPE;
			s[len - 1] = '\0';
		} else {
			glyph = mbox_glyph;
			type = MT_MAILBOX_TYPE;
		}

		/*
		 * Batch up the inserts to improve speed
		 */
		string_list[index] = av[n];
		glyph_list[index] = glyph;
		data_list[index] = type;

		if (index == LISTBATCHSIZE - 1) {
			mt_batch_insert(list, list_n, index + 1,
				string_list, glyph_list, data_list);
			list_n += (index + 1);
			index = 0;
		} else {
			index++;
		}
	}

	if (index > 0) {
		mt_batch_insert(list, list_n, index,
				string_list, glyph_list, data_list);
	}

	if (av != NULL)
		ck_free(&av[-1]);

	(void)xv_set(list, PANEL_SHOW_ITEM, TRUE, 0);
}

 dummy_proc()

 {

 }
#endif

#ifdef FILE_LIST
list_notify_proc(item, dir, file, client_data, op, event, row)
#else
list_notify_proc(item, file, client_data, op, event, row)
#endif

	Panel_item	item;
#ifdef FILE_LIST
	char            *dir;
#endif
	char		*file;
	Xv_opaque	client_data;
	Panel_list_op	op;
	Event		*event;	/* May be NULL if simulated */
	int		row;

{
	Frame	frame;
	Filelist_popup_data	*fpd;
#ifdef FILE_LIST
	File_list_row_type      row_type;
#else
	static  Event	last_event;
#endif
	char	buf[MAXPATHLEN + 1];
	char	*p;

	frame = (Frame)xv_get((Panel)xv_get(item, PANEL_PARENT_PANEL),
								XV_OWNER);
	fpd = (Filelist_popup_data *) xv_get(frame, WIN_CLIENT_DATA);

#ifdef FILE_LIST
	row_type = xv_get(item, FILE_LIST_ROW_TYPE, row);
#endif

	switch(op) {
	case PANEL_LIST_OP_SELECT:
#ifdef FILE_LIST
		if (row_type != FILE_LIST_DOTDOT_TYPE)
			xv_set(fpd->fpd_name_field, PANEL_VALUE, file, 0);
#else
		if (client_data != MT_GO_UP_TYPE)
			xv_set(fpd->fpd_name_field, PANEL_VALUE, file, 0);
		else
			xv_set(fpd->fpd_name_field, PANEL_VALUE, "", 0);

		if (event != NULL && ds_is_double_click(&last_event, event)) {
			(void)xv_set(fpd->fpd_frame, FRAME_BUSY, TRUE, 0);
			switch (client_data) {

			case MT_INBOX_TYPE:
				mt_new_folder(fpd->hd, "%", mt_system_mail_box,
						mt_system_mail_box, FALSE, TRUE);
				break;
			case MT_MAILBOX_TYPE:
				if (get_file(fpd->fpd_name_field,
						fpd->fpd_frame, buf) > -1) {
					mt_new_folder(fpd->hd, buf, FALSE, FALSE, FALSE, TRUE);
				}
				break;
			case MT_DIR_TYPE:
				descend(fpd, file);
				break;
			case MT_GO_UP_TYPE:
				ascend(fpd, file);
				break;
			}
			(void)xv_set(fpd->fpd_frame, FRAME_BUSY, FALSE, 0);
		}

		if (event != NULL)
			last_event = *event;
#endif
		break;

#ifdef FILE_LIST
	case PANEL_LIST_OP_DBL_CLICK:
		if (row_type == FILE_LIST_FILE_TYPE) {
			(void)xv_set(fpd->fpd_frame, FRAME_BUSY, TRUE, 0);
			if (strcmp(file, "In-Box") == 0) {
				mt_new_folder(fpd->hd, "%", mt_system_mail_box,
					      mt_system_mail_box, FALSE, TRUE);
			} else {
				if (get_file(fpd->fpd_name_field,
					     fpd->fpd_frame, buf) > -1) {
					mt_new_folder(fpd->hd, buf, FALSE,
							FALSE, FALSE, TRUE);
				}
			}
			(void)xv_set(fpd->fpd_frame, FRAME_BUSY, FALSE, 0);
		}
		break;
#endif

	case PANEL_LIST_OP_DESELECT:
	case PANEL_LIST_OP_VALIDATE:
	case PANEL_LIST_OP_DELETE:
	default:
		break;
	}
}

#ifndef FILE_LIST
descend(fpd, string)

	Filelist_popup_data	*fpd;
	char		*string;

{
	char	*dir;
	int	level;
	char	buf[MAXPATHLEN + 1];

	strcpy(buf, fpd->fpd_curdir);
	if (*buf != '\0')
		strcat (buf, "/");
	strcat(buf, string);
	if (set_curdir(fpd, buf) < 0)
		return;
	fpd->fpd_level++;

	/*
	 * Build full path to directory
	 */
	strcpy(buf, fpd->fpd_parentdir);
	strcat(buf, fpd->fpd_curdir);
	strcat(buf, "/");
	(void)xv_set(fpd->fpd_list, PANEL_LIST_TITLE, fpd->fpd_curdir, 0);
	load_directory(fpd, buf, fpd->fpd_level);
	mt_select_and_notify(fpd->fpd_list, 0);
}

ascend(fpd, string)

	Filelist_popup_data	*fpd;
	char		*string;

{
	char	*dir;
	char	*p;
	int	level;
	char	buf[MAXPATHLEN + 1];

	strcpy(buf, fpd->fpd_curdir);
	/* remove lastmost directory name from path */
	if ((p = strrchr(buf, '/')) != NULL)
		*p = '\0';
	else
		*buf = '\0';

	fpd->fpd_level--;
	set_curdir(fpd, buf);

	/*
	 * Build full path to directory
	 */
	strcpy(buf, fpd->fpd_parentdir);
	strcat(buf, fpd->fpd_curdir);
	strcat(buf, "/");
	(void)xv_set(fpd->fpd_list, PANEL_LIST_TITLE, fpd->fpd_curdir, 0);
	load_directory(fpd, buf, fpd->fpd_level);
	mt_select_and_notify(fpd->fpd_list, 0);
}

static
reload_current_dir(fpd)

	Filelist_popup_data	*fpd;

{
	char	*p;
	char	*dir;
	int	level;
	int	selected_row;
	char	buf[MAXPATHLEN + 1];
	struct stat	statb;
	Panel_item	list;

	list = fpd->fpd_list;
	
	if ((selected_row = mt_get_selected_list_row(list)) < 0)
		selected_row = 0;

	strcpy(buf, fpd->fpd_parentdir);
	strcat(buf, fpd->fpd_curdir);
 	strcat(buf, "/");

	while ((stat(buf, &statb) < 0) &&		/* Doesn't exist */
	       (fpd->fpd_level != 0)) {			/* Aren't at top */

		/*
		 * First, reduce what we are testing to working directory
		 * beneath the HOME directory.
		 */

		strcpy(buf, fpd->fpd_curdir);

		/*
		 * Remove the last-most directory.
		 */

		if ((p = strrchr(buf, '/')) != NULL)
			*p = '\0';
		else
			*buf = '\0';

		fpd->fpd_level--;
		set_curdir(fpd, buf);

		/*
		 * Rebuild the directory string to the full directory
		 * minus the previously, non-existing directory.
		 */

		strcpy(buf, fpd->fpd_parentdir);
		strcat(buf, fpd->fpd_curdir);
		strcat(buf, "/");

		/*
		 * Finish the ascent to the previous directory.
		 */

		(void)xv_set(	fpd->fpd_list,
				PANEL_LIST_TITLE,
				fpd->fpd_curdir,
				0);

		}

	if (stat(buf, &statb) < 0) {			/* Still doesn't */
		if (!mt_check_and_create(buf, TRUE, fpd->fpd_frame, TRUE)) {

			bad_directory = TRUE;
			return;
		}
	}

	if (fpd->fpd_curdir_time == statb.st_mtime) {
		/* Directory has not changed.  Don't bother */
		return;
	}

	load_directory(fpd, buf, fpd->fpd_level);
	if (bad_directory) return;
	mt_select_and_notify(list, selected_row);
	return;
}
#endif

static
get_file(text_field, frame, buf )

	Panel_item	text_field;	/* Field to get file name from */
	Frame		frame;		/* Frame to own error messages */
	char		*buf;		/* filled in with file name */

{
	char	*file;
	char	*file_strip;
	char	*p;
	Filelist_popup_data	*fpd;

	/*
	 * Get the value from the file list text field.  Relative 
	 * paths are relative to the filelist's current directory.
	 */
	if (*(file = (char *)xv_get(text_field, PANEL_VALUE)) == '\0') {
		mt_vs_warn(frame,
			gettext("No Name specified.  Please enter one."));
		return(-1);
	}

	/* 
         * In this case, because we are overwriting something, 
	 * (in strip trailing blanks),
         * we should make a copy of the xv_get.	
	 */
	file = (char *)strdup(file);
        file_strip = mt_strip_leading_blanks(file);
        file_strip = mt_strip_trailing_blanks(file_strip);
	xv_set(text_field, PANEL_VALUE, file_strip, 0);
	ck_free(file);

	file = (char *)xv_get(text_field, PANEL_VALUE);

	if (*file == '+' || *file == '/' || *file == '~') {
		/* File is not relative to current directory */
		strcpy(buf, file);
	} else if (strcmp(file, gettext("In-Box")) == 0) {
		strcpy(buf, "%");
	} else {
		/*
		 * Build path relative to current filelist directory
		 */
		fpd = (Filelist_popup_data *)xv_get(frame, WIN_CLIENT_DATA);

#ifdef FILE_LIST
		strcpy(buf, (char *)xv_get(fpd->fpd_list, FILE_LIST_DIRECTORY));
		strcat(buf, "/");
		strcat(buf, file);
#else
		/*
		 * fpd_curdir contains the last component of the folder
		 * path (so we can display it as the list title).  We must
		 * strip this off.
		 */
		if ((p = strchr(fpd->fpd_curdir, '/')) == NULL)
			p = "";
		else
			p++;

		strcpy(buf, p);
		if (*p != '\0')
			strcat(buf, "/");
		strcat(buf, file);
#endif
	}

	return(0);
}



#ifndef FILE_LIST
mt_batch_insert(list, row, n, string_list, glyph_list, data_list)

	Panel_item	list;
	int		row;
	int		n;
	char		*string_list[];
	Server_image	glyph_list[];
	caddr_t		data_list[];

{
	int	i = 0;

	/*
	 * Insert a bunch of rows into a scrolling list.  This may look
	 * crazy, but it more than doubles insertion speed.  I tried 
	 * various schemes using PANEL_LIST_INSERT_STRINGS/GLYPHS, but
	 * they didn't work as well.  This code was snagged from the
	 * Guide file chooser and then modified a bit.
	 */
	while (i < n) {
		if ((n - i) > 15) {
			xv_set(list,
				INSERT_LIST_ROW(row, i),
				INSERT_LIST_ROW(row+1, i+1),
				INSERT_LIST_ROW(row+2, i+2),
				INSERT_LIST_ROW(row+3, i+3),
				INSERT_LIST_ROW(row+4, i+4),
				INSERT_LIST_ROW(row+5, i+5),
				INSERT_LIST_ROW(row+6, i+6),
				INSERT_LIST_ROW(row+7, i+7),
				INSERT_LIST_ROW(row+8, i+8),
				INSERT_LIST_ROW(row+9, i+9),
				INSERT_LIST_ROW(row+10, i+10),
				INSERT_LIST_ROW(row+11, i+11),
				INSERT_LIST_ROW(row+12, i+12),
				INSERT_LIST_ROW(row+13, i+13),
				INSERT_LIST_ROW(row+14, i+14),
				INSERT_LIST_ROW(row+15, i+15),
			NULL);
			i += 16;
			row += 16;
		} else if ((n - i) > 7) {
			xv_set(list,
				INSERT_LIST_ROW(row, i),
				INSERT_LIST_ROW(row+1, i+1),
				INSERT_LIST_ROW(row+2, i+2),
				INSERT_LIST_ROW(row+3, i+3),
				INSERT_LIST_ROW(row+4, i+4),
				INSERT_LIST_ROW(row+5, i+5),
				INSERT_LIST_ROW(row+6, i+6),
				INSERT_LIST_ROW(row+7, i+7),
			NULL);
			i += 8;
			row += 8;
		} else if ((n - i) > 3) {
			xv_set(list,
				INSERT_LIST_ROW(row, i),
				INSERT_LIST_ROW(row+1, i+1),
				INSERT_LIST_ROW(row+2, i+2),
				INSERT_LIST_ROW(row+3, i+3),
			NULL);
			i += 4;
			row += 4;
		} else if ((n - i) > 1) {
			xv_set(list,
				INSERT_LIST_ROW(row, i),
				INSERT_LIST_ROW(row+1, i+1),
			NULL);
			i += 2;
			row += 2;
		} else {
			xv_set(list,
				INSERT_LIST_ROW(row, i),
			NULL);
			i += 1;
			row += 1;
		}
	}
}

#endif
