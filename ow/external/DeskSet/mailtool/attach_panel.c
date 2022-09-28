#ifdef lint
static 	char sccsid[] = "@(#)attach_panel.c	3.25 - 94/09/13 Copyr 1990 Sun Micro";
#endif

/*
 * Copyright (c) 1990 by Sun Microsystems, Inc.
 */

#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/param.h>
#include <sys/stat.h>

#include <xview/xview.h>
#include <xview/panel.h>
#include <xview/canvas.h>
#include <xview/font.h>
#include <xview/scrollbar.h>
#include <xview/xv_xrect.h>
#ifdef FILECHOOSER
#include <xview/file_chsr.h>
#endif

/* To get at vwl structure */
#include <xview/text.h>
#include "tool.h"
#include "tool_support.h"
#include "attach.h"
#include "graphics.h"
#include "mle.h"
#include "ds_popup.h"
#include "../maillib/obj.h"

#define DEBUG_FLAG mt_debugging
extern DEBUG_FLAG;
#include "debug.h"

/*
 * The 3 attachment popups' FRAME_CMD's KEY_DATA keys
 */
#define IS_POPUP_KEY	100
/*
 * Add attachment popup's FRAME_CMD_PANEL's KEY_DATA keys
 */
#define DIRECTORY_KEY	100
#define FILE_KEY	101
#define ATTACH_PANEL_KEY	102
/* To save view window's frame */
#define VIEW_FRAME_KEY	200
/* To save view window list */
#define VWL_KEY		300

#define FC_PARENT_KEY	301
#define FC_EXTEN_ITEM_KEY 302




static Menu_item single_selection_gen_proc(Menu_item mi, Menu_generate op);
static Menu_item multiple_selection_gen_proc(Menu_item mi, Menu_generate op);
static Menu_item undelete_item_gen_proc(Menu_item mi, Menu_generate op);
static Frame create_attach_rename_popup(Frame frame, Panel parent_panel);
static void select_all_attach_proc(Menu, Menu_item);
static void delete_attach_proc(Menu, Menu_item);
static void open_attach_proc(Menu, Menu_item);
static void attach_import_popup_proc(Menu, Menu_item);
static void attach_export_popup_proc(Menu, Menu_item);
static void attach_rename_popup_proc(Menu, Menu_item);
static void undelete_attach_proc(Menu, Menu_item);
#ifdef FILECHOOSER
static int add_attach_proc(File_chooser	fc, char *path, char *file,
		Xv_opaque client_data);
static int export_attach_proc(File_chooser fc, char *path, int exists);
#else
static void add_attach_proc(Panel_item item, Event *event);
static void export_attach_proc(Panel_item item, Event *event);
#endif
static Notify_value attach_popup_interpose_proc(Frame frame, Event *event,
		Notify_arg arg, Notify_event_type type);
static void rename_attach_proc(Panel_item item, Event *event);
static Frame create_attach_file_popup(Frame frame, Panel parent_panel, int add);
static void select_all_attachments(Attach_list *);
static void update_saveas_popup(Attach_list *, int nselected);
static void check_pushpin(Frame);
static void resize_attach_popup(Panel);
static void add_exten_item(File_chooser);




static Panel
mt_create_attach_panel(
	Frame	frame,
	Attach_list *al
)
{
	Panel	panel;
	Frame	popup;
	Panel_item	tmp_item;
	Menu		tmp_menu;
	char	*s;
	int	row_gap;

	/*
	 * Create an attachment list control panel
	 */
	panel = (Panel)xv_create(frame, PANEL,
			WIN_BORDER,	FALSE,
			XV_SHOW,	FALSE,
			XV_WIDTH,	600,	/* Kludge to get around layout
							bug */
			/* for Delete and Undelete */
                	XV_KEY_DATA, VIEW_FRAME_KEY, frame,
#ifdef OW_I18N
			WIN_USE_IM,	FALSE,
#endif
			0);
		
	tmp_item = (Panel_item)xv_create(panel, PANEL_MESSAGE,
		PANEL_LABEL_STRING,	gettext("Attachments"),
		PANEL_LABEL_BOLD,	TRUE,
		0);

	mt_scoot_item_right(panel, tmp_item, 2);

	/* Create File and View menus.  The menu's client data contains
	 * the panel the menu button belongs to. For items which bring
	 * up popups (ie Rename...) The item's client data contains the
	 * popup which the item brings up -- this is set in the ACTION_PROC
	 * for the menu item.
	 */

	/* STRING_EXTRACTION -
	 *
	 * Create the attachment File menu button. It contains:
	 *	Add...
	 * 	Open
	 *	Copy Out...
	 */
	tmp_menu = (Menu)xv_create(NULL, MENU,
		MENU_CLIENT_DATA,		panel,
		MENU_ITEM,
			MENU_ACTION_ITEM,	gettext("Add..."),
						attach_import_popup_proc,
			XV_HELP_DATA,		"mailtool:AddAttach",
			0,
		MENU_ITEM,
			MENU_ACTION_ITEM,	gettext("Open"),
					  	open_attach_proc,
			MENU_INACTIVE,		TRUE,
			MENU_GEN_PROC,		single_selection_gen_proc,
			XV_HELP_DATA,		"mailtool:OpenAttach",
			0,
		MENU_ITEM,
#ifdef FILECHOOSER
			MENU_ACTION_ITEM,	gettext("Save As..."),
#else
			MENU_ACTION_ITEM,	gettext("Copy Out..."),
#endif
						attach_export_popup_proc,
			MENU_GEN_PROC,		multiple_selection_gen_proc,
			XV_HELP_DATA,		"mailtool:ExportAttach",
			0,
		0);

	tmp_item = (Panel_item)xv_create(panel, PANEL_BUTTON,
		PANEL_LABEL_STRING,	gettext("File"),
		PANEL_ITEM_MENU,	tmp_menu,
		XV_HELP_DATA,		"mailtool:FileAttach",
		0);

	mt_scoot_item_right(panel, tmp_item, 1);

	/* STRING_EXTRACTION -
	 *
	 * Create the attachment Edit menu button. It contains:
	 *	Delete
	 * 	Undelete
	 *	Rename...
	 *	Select All
	 */
	tmp_menu = (Menu)xv_create(NULL, MENU,
		MENU_CLIENT_DATA,		panel,
		MENU_ITEM,
			MENU_ACTION_ITEM,	gettext("Delete"),
						delete_attach_proc,
			MENU_INACTIVE,		TRUE,
			MENU_GEN_PROC,		multiple_selection_gen_proc,
			XV_HELP_DATA,		"mailtool:DeleteAttach",
			0,
		MENU_ITEM,
			MENU_ACTION_ITEM,	gettext("Undelete"),
					  	undelete_attach_proc,
			MENU_INACTIVE,		TRUE,
			MENU_GEN_PROC,		undelete_item_gen_proc,
			XV_HELP_DATA,		"mailtool:UndeleteAttach",
			0,
		MENU_ITEM,
			MENU_ACTION_ITEM,	gettext("Rename..."),
						attach_rename_popup_proc,
			MENU_INACTIVE,		TRUE,
			MENU_GEN_PROC,		single_selection_gen_proc,
			XV_HELP_DATA,		"mailtool:RenameAttach",
			0,
		MENU_ITEM,
			MENU_ACTION_ITEM,	gettext("Select All"),
						select_all_attach_proc,
			XV_HELP_DATA,		"mailtool:SelectAttach",
			0,
		MENU_DEFAULT,	3,		/* Make Rename the default */
		0);

	tmp_item = (Panel_item)xv_create(panel, PANEL_BUTTON,
		PANEL_LABEL_STRING,	gettext("Edit"),
		PANEL_ITEM_MENU,	tmp_menu,
		XV_HELP_DATA,		"mailtool:EditAttach",
		0);

	/* We find and save a handle to this item in mt_create_attach_canvas */
	(void)xv_create(panel, PANEL_MESSAGE, 0);
	window_fit_height(panel);

	row_gap = (int)xv_get(panel, WIN_ROW_GAP);
	(void)xv_set(panel,
		XV_WIDTH,	WIN_EXTEND_TO_EDGE,
		XV_HEIGHT, (int)xv_get(panel, XV_HEIGHT) -
					row_gap / 2 - row_gap / 4,
		0);

	mt_set_attach_list(panel, al);
	return(panel);
}




Panel_item
mt_get_attach_msg_item(
	Panel	panel
)
{
	Panel_item	item;
	int		n;

	/* Find the message item in the attachment panel.  We want the
	 * second one.  Yes this is hokey.
	 */
	n = 0;
	PANEL_EACH_ITEM(panel, item) {
		if ((Panel_item_type)xv_get(item, PANEL_ITEM_CLASS) !=
						PANEL_MESSAGE_ITEM) {
			continue;
		}

		if (++n == 2)
			return(item);
	} PANEL_END_EACH;

	return(NULL);
}



void
mt_attach_footer_message(
	Attach_list	*al
)
{
	int	nselected;
	char	buf[80];

	nselected = mt_nattachments(al, TRUE);

	/* STRING_EXTRACTION -
	 *
	 * When the user selects attachments, display a message in the
	 * footer telling them how many are selected and the total size
	 * The first %d is the number of selected attachments.  The 
	 * second is the total size of the selected attachments.
	 *
	 * there are two messages here -- one if there is exactly
	 * one attachment, and one if there is more than one.
	 */
	if (nselected <= 0) {
		*buf = '\0';
	} else if (nselected == 1) {
		sprintf(buf, gettext("  1 attachment selected (%d bytes)"), 
			mt_attach_list_size(al, TRUE));
	} else {
		sprintf(buf, gettext("%3d attachments selected (%d bytes)"), 
			nselected, mt_attach_list_size(al, TRUE));
	}

	mt_frame_msg(al->al_errorframe, FALSE, buf);

	return;
}



#ifdef NOTUSED
void
mt_clear_attach_panel_message(al)

	Attach_list	*al;

{
	(void)xv_set(al->al_msg_item, PANEL_LABEL_STRING, "", 0);
	return;
}
#endif



void
mt_attach_panel_message(
	Attach_list	*al
)
{
	int	n;
	char	buf[80];
	Rect	*item_rect;
	Xv_Font	font;
	Font_string_dims	font_size;

	/*
	 * We scan the attachment list to get the number of attachments
	 * And then scan it again to get the total size.  This seems
	 * wasteful, but we usually have less than 10 or so attachments
	 * and this way we don't have to carry around extra info which
	 * risks getting out of date.
	 */
	n = mt_nattachments(al, FALSE);

	/* STRING_EXTRACTION -
	 *
	 * Display a message in the attachment panel which gives some
	 * information on the number and size of the attachments
	 */
	if (n <= 0)
		*buf = '\0';
	else if (n == 1)
		sprintf(buf, gettext("%3d attachment (%d bytes)"), n, 
			mt_attach_list_size(al, FALSE));
	else
		sprintf(buf, gettext("%3d attachments (%d bytes)"), n,
			mt_attach_list_size(al, FALSE));

	n = (int)xv_get(al->al_panel, XV_WIDTH) -
	    		(int)xv_get(al->al_panel, PANEL_ITEM_X_GAP);

	if (*buf != '\0') {
		/* Right justify message item on panel */
		font = (Xv_Font)xv_get(al->al_msg_item, PANEL_LABEL_FONT);
		(void)xv_get(font, FONT_STRING_DIMS, buf, &font_size);
		n = n - font_size.width;
	}

	(void)xv_set(al->al_msg_item,
		XV_X, n,
		PANEL_LABEL_STRING, buf,
		0);

	return;
}



/* Set the Open item active/inactive depending on if there is anything
 * selected
 */
static Menu_item
single_selection_gen_proc(
	Menu_item	mi,
	Menu_generate	op
)
{
	Menu		menu;
	Panel		owner_panel;
	Attach_list	*al;

	switch (op)  {

	case MENU_DISPLAY:
	case MENU_NOTIFY:
		/* Go back to the panel to get find the attachment list to
		 * operate on.
		 */
		menu = (Menu)xv_get(mi, MENU_PARENT);
		owner_panel = (Panel)xv_get(menu, MENU_CLIENT_DATA);
		al = mt_get_attach_list(owner_panel);

		/* Open is active only if 1 attachment is selected */
		if (mt_nattachments(al, TRUE) != 1)
			xv_set(mi, MENU_INACTIVE, TRUE, 0);
		else
			xv_set(mi, MENU_INACTIVE, FALSE, 0);
		break;
	}

	return(mi);
}



/* Set the item active/inactive depending on if there is anything
 * selected
 */
static Menu_item
multiple_selection_gen_proc(
	Menu_item	mi,
	Menu_generate	op
)
{
	Menu		menu;
	Panel		owner_panel;
	Attach_list	*al;

	switch (op)  {

	case MENU_DISPLAY:
	case MENU_NOTIFY:
		/* Go back to the panel to get find the attachment list to
		 * operate on.
		 */
		menu = (Menu)xv_get(mi, MENU_PARENT);
		owner_panel = (Panel)xv_get(menu, MENU_CLIENT_DATA);
		al = mt_get_attach_list(owner_panel);

		/* Item is active only if there is something selected */
		if (mt_nattachments(al, TRUE) <= 0)
			xv_set(mi, MENU_INACTIVE, TRUE, 0);
		else
			xv_set(mi, MENU_INACTIVE, FALSE, 0);
		break;
	}

	return(mi);
}




/* Set the Undelete item active/inactive depending on if there is anything
 * to undelete.
 */
static Menu_item
undelete_item_gen_proc(
	Menu_item	mi,
	Menu_generate	op
)
{
	Menu		menu;
	Panel		owner_panel;
	Attach_list	*al;

	switch (op)  {

	case MENU_DISPLAY:
	case MENU_NOTIFY:
		/* Go back to the panel to get find the attachment list to
		 * operate on.
		 */
		menu = (Menu)xv_get(mi, MENU_PARENT);
		owner_panel = (Panel)xv_get(menu, MENU_CLIENT_DATA);
		al = mt_get_attach_list(owner_panel);

		/* Set Undelete item's active state */
		if (mt_find_last_deleted(al) == NULL)
			xv_set(mi, MENU_INACTIVE, TRUE, 0);
		else
			xv_set(mi, MENU_INACTIVE, FALSE, 0);
		break;
	}

	return(mi);
}


static int
open_selected_attach(
	Attach_list	*al,
	Attach_node	*node,
	long		unused
)
{
	if (node->an_selected) {
		mt_invoke_application(al, node, TRUE);
	}

	return(TRUE);
}


static void
open_attach_proc(
	Menu		menu,
	Menu_item	menu_item
)
{
	Panel	owner_panel;
	Attach_list	*al;

	/*
	 * Open selected attachments.
	 */
	owner_panel = (Panel)xv_get(menu, MENU_CLIENT_DATA);
	al = mt_get_attach_list(owner_panel);
	mt_busy(al->al_headerframe, TRUE, NULL, TRUE);
	mt_traverse_attach_list(al, open_selected_attach, 0);
	sleep(1);	/* Dramatic pause for effect */
	mt_busy(al->al_headerframe, FALSE, NULL, TRUE);
	/* Keep unpinned frame from dismissing */
	(void)xv_set(menu, MENU_NOTIFY_STATUS, XV_ERROR, 0);
}



static void
attach_rename_popup_proc(
	Menu		menu,
	Menu_item	menu_item
)
{
	Frame	popup, owner_frame;
	Panel	owner_panel;
	Panel_item	item;
	char	*s;
	int	add;

	/*
	 * Create/display the rename attachment popup
	 */
	popup = (Frame)xv_get(menu_item, MENU_CLIENT_DATA);
	owner_panel = (Panel)xv_get(menu, MENU_CLIENT_DATA);
	owner_frame = (Frame)xv_get(owner_panel, XV_OWNER);

	if (popup == NULL) {
		popup = create_attach_rename_popup(owner_frame, owner_panel);
		(void)xv_set(menu_item, MENU_CLIENT_DATA, popup, 0);
		ds_position_popup(owner_frame, popup, DS_POPUP_LOR);
	}

	/* We set frame closed to hack around a bug where starting mailtool
	 * with -Wi causes the popup to come up iconic.
	 */
	(void)xv_set(popup, XV_SHOW, TRUE, WIN_FRONT, FRAME_CLOSED, FALSE, 0);
	/* Keep unpinned frame from dismissing */
	(void)xv_set(menu, MENU_NOTIFY_STATUS, XV_ERROR, 0);
}



static void
attach_import_popup_proc(
	Menu		menu,
	Menu_item	menu_item
)
{
	Frame	popup, owner_frame;
	Panel	owner_panel;

	/*
	 * Create/display the add attachment popup
	 */
	popup = (Frame)xv_get(menu_item, MENU_CLIENT_DATA);
	owner_panel = (Panel)xv_get(menu, MENU_CLIENT_DATA);
	owner_frame = (Frame)xv_get(owner_panel, XV_OWNER);

	if (popup == NULL) {
		popup = create_attach_file_popup(owner_frame, owner_panel,
									TRUE);
		(void)xv_set(menu_item, MENU_CLIENT_DATA, popup, 0);

		ds_position_popup(owner_frame, popup, DS_POPUP_LOR);
#ifdef FILECHOOSER
	} else {
		(void)xv_set(popup, FILE_CHOOSER_UPDATE, 0);
#endif
	}


	/* We set frame closed to hack around a bug where starting mailtool
	 * with -Wi causes the popup to come up iconic.
	 */
	(void)xv_set(popup, XV_SHOW, TRUE, WIN_FRONT, FRAME_CLOSED, FALSE, 0);
	/* Keep unpinned frame from dismissing */
	(void)xv_set(menu, MENU_NOTIFY_STATUS, XV_ERROR, 0);
}



static void
attach_export_popup_proc(
	Menu		menu,
	Menu_item	menu_item
)
{
	Frame	popup, owner_frame;
	Panel	owner_panel;
	char	*s;
	int	add;
#ifdef FILECHOOSER
	char	*name;
	Attach_list	*al;
	Attach_node	*an;
#endif

	/*
	 * Create/display the export attachment popup
	 */
	popup = (Frame)xv_get(menu_item, MENU_CLIENT_DATA);
	owner_panel = (Panel)xv_get(menu, MENU_CLIENT_DATA);
	owner_frame = (Frame)xv_get(owner_panel, XV_OWNER);
#ifdef FILECHOOSER
	al = mt_get_attach_list(owner_panel);
#endif

	if (popup == NULL) {
		popup = create_attach_file_popup(owner_frame, owner_panel,
									FALSE);
		(void)xv_set(menu_item, MENU_CLIENT_DATA, popup, 0);
		ds_position_popup(owner_frame, popup, DS_POPUP_LOR);
#ifdef FILECHOOSER
		al->al_saveas_fc = popup;
	} else {
		(void)xv_set(popup, FILE_CHOOSER_UPDATE, 0);
#endif
	}

#ifdef FILECHOOSER
	update_saveas_popup(al, mt_nattachments(al, TRUE));
#endif

	/* We set frame closed to hack around a bug where starting mailtool
	 * with -Wi causes the popup to come up iconic.
	 */
	(void)xv_set(popup, XV_SHOW, TRUE, WIN_FRONT, FRAME_CLOSED, FALSE, 0);
	/* Keep unpinned frame from dismissing */
	(void)xv_set(menu, MENU_NOTIFY_STATUS, XV_ERROR, 0);
}



static Frame
create_attach_rename_popup(
	Frame	frame,
	Panel	parent_panel	/* Allows us to get to panels CLIENT_DATA */
)
{
	Panel_item	button_item;
	Frame	popup;
	int	row;
	Panel	panel;
	Panel_item	name_item;

	/*
	 * Create the rename attachment popup
	 */
	popup = (Frame)xv_create(frame, FRAME_CMD,
			WIN_IS_CLIENT_PANE,
			WIN_COLUMNS,	50,
			FRAME_SHOW_RESIZE_CORNER,	TRUE,
			FRAME_SHOW_FOOTER,	TRUE,
			XV_KEY_DATA,	IS_POPUP_KEY,	TRUE,
			0);

	/* Interpose so we can handle the resize event */
	(void)notify_interpose_event_func(popup,
			attach_popup_interpose_proc, NOTIFY_SAFE);


	/* Get panel and layout its fields */
	panel = (Panel)xv_get(popup, FRAME_CMD_PANEL);

	(void)xv_set(panel,
		XV_Y,	0,
		XV_X,	0,
		WIN_BORDER,	TRUE,
		XV_SHOW,	TRUE,
		XV_WIDTH,	WIN_EXTEND_TO_EDGE,
		PANEL_LAYOUT,	PANEL_VERTICAL,
		0);

	name_item = NULL;
	row = 0;
	(void)xv_create(panel, PANEL_MESSAGE,
		PANEL_LABEL_STRING,
			gettext("Rename selected attachment to:"),
		PANEL_LABEL_BOLD,	TRUE,
		XV_Y,			xv_row(panel, row++),
		0);

	name_item = (Panel_item)xv_create(panel, PANEL_TEXT,
		PANEL_LABEL_STRING,	gettext("Name:"),
		XV_Y,			xv_row(panel, row++),
		XV_HELP_DATA,		"mailtool:RenameAttachName",
		0);

	(void)xv_set(name_item,
		PANEL_NOTIFY_LEVEL,	PANEL_SPECIFIED,
		PANEL_NOTIFY_STRING,	"\r\n",
		PANEL_NOTIFY_PROC,	rename_attach_proc,
		0);

	button_item = xv_create(panel, PANEL_BUTTON,
		PANEL_LABEL_STRING,	gettext("Rename"),
		PANEL_NOTIFY_PROC,	rename_attach_proc,
		XV_HELP_DATA,		"mailtool:RenameAttachButton",
		0);
		mt_label_frame(popup, gettext("Rename Attachment"));

	/* Set key data on panel so we can get back to fields */
	(void)xv_set(panel,
		XV_KEY_DATA,		FILE_KEY,	name_item,
		XV_KEY_DATA,		ATTACH_PANEL_KEY, parent_panel,
		/* For Add attachment */
		XV_KEY_DATA,		VIEW_FRAME_KEY, frame,
		PANEL_DEFAULT_ITEM,	button_item,
		0);

	window_fit_height(panel);
	window_fit_height(popup);


	/* Center button and resize text fields */
	resize_attach_popup(panel);

	return(popup);
}



#ifdef FILECHOOSER

static Frame
create_attach_file_popup(
	Frame	frame,
	Panel	parent_panel,	/* Allows us to get to panels CLIENT_DATA */
	int	add		/* True to create the "Add" popup */
)
{
	File_chooser	popup;

	if (add) {
		popup = xv_create(frame, FILE_CHOOSER_OPEN_DIALOG,
				FILE_CHOOSER_NOTIFY_FUNC,	add_attach_proc,
				FILE_CHOOSER_CUSTOMIZE_OPEN,
					gettext("Add"), 
					gettext("Select a file and click Add"),
					FILE_CHOOSER_SELECT_FILES,
				XV_KEY_DATA,	VIEW_FRAME_KEY, frame,
				XV_KEY_DATA,	ATTACH_PANEL_KEY, parent_panel,
				XV_KEY_DATA,	IS_POPUP_KEY,	TRUE,
				0);
		mt_label_frame(popup, gettext("Add Attachment"));
	} else {
		popup = xv_create(frame, FILE_CHOOSER_SAVEAS_DIALOG,
				FILE_CHOOSER_NOTIFY_FUNC, export_attach_proc,
				XV_KEY_DATA,	VIEW_FRAME_KEY, frame,
				XV_KEY_DATA,	ATTACH_PANEL_KEY, parent_panel,
				XV_KEY_DATA,	IS_POPUP_KEY,	TRUE,
				0);

		mt_label_frame(popup, gettext("Save Attachment As"));
	}

	add_exten_item(popup); /* add the dotfile choice item */

	/* File_chooser is a subclass of Frame, so this narrow should be OK */
	return((Frame)popup);
}

#else /* !FILECHOOSER */

static Frame
create_attach_file_popup(
	Frame	frame,
	Panel	parent_panel,	/* Allows us to get to panels CLIENT_DATA */
	int	add		/* True to create the "Add" popup */
)
{
	Panel_item	button_item;
	Frame	popup;
	int	row;
	Panel	panel;
	Panel_item	directory_item, file_item;

	/*
	 * Create the add attachment popup
	 */
	popup = (Frame)xv_create(frame, FRAME_CMD,
			WIN_IS_CLIENT_PANE,
			WIN_COLUMNS,	50,
			FRAME_SHOW_RESIZE_CORNER,	TRUE,
			FRAME_SHOW_FOOTER,	TRUE,
			XV_KEY_DATA,	IS_POPUP_KEY,	TRUE,
			0);

	/* Interpose so we can handle the resize event */
	(void)notify_interpose_event_func(popup,
			attach_popup_interpose_proc, NOTIFY_SAFE);


	/* Get panel and layout its fields */
	panel = (Panel)xv_get(popup, FRAME_CMD_PANEL);

	(void)xv_set(panel,
		XV_Y,	0,
		XV_X,	0,
		WIN_BORDER,	TRUE,
		XV_SHOW,	TRUE,
		XV_WIDTH,	WIN_EXTEND_TO_EDGE,
		PANEL_LAYOUT,	PANEL_VERTICAL,
		0);

	file_item = NULL;
	row = 0;
	if (!add) {
		(void)xv_create(panel, PANEL_MESSAGE,
			PANEL_LABEL_STRING,
				gettext("Copy selected attachments to:"),
			PANEL_LABEL_BOLD,	TRUE,
			XV_Y,			xv_row(panel, row++),
			0);
	}

	directory_item = (Panel_item)xv_create(panel, PANEL_TEXT,
		PANEL_LABEL_STRING,	gettext("Directory:"),
		XV_Y,			xv_row(panel, row++),
		XV_HELP_DATA,		"mailtool:AddAttachDir",
		0);


	if (add) {
		file_item = (Panel_item)xv_create(panel, PANEL_TEXT,
			PANEL_LABEL_STRING,	gettext("File Name:"),
			XV_Y,			xv_row(panel, row++),
			XV_HELP_DATA,		"mailtool:AddAttachFile",
			0);

		/* Align text fields.  */
		ds_justify_items(panel, FALSE);

		(void)xv_set(file_item,
			PANEL_NOTIFY_LEVEL,	PANEL_SPECIFIED,
			PANEL_NOTIFY_STRING,	"\r\n",
			PANEL_NOTIFY_PROC,	add_attach_proc,
			0);
		button_item = xv_create(panel, PANEL_BUTTON,
			PANEL_LABEL_STRING,	gettext("Add"),
			PANEL_NOTIFY_PROC,	add_attach_proc,
			XV_HELP_DATA,		"mailtool:AddAttachButton",
			0);
		mt_label_frame(popup, gettext("Add Attachment"));
	} else {
		(void)xv_set(directory_item,
			PANEL_NOTIFY_LEVEL,	PANEL_SPECIFIED,
			PANEL_NOTIFY_STRING,	"\r\n",
			PANEL_NOTIFY_PROC,	export_attach_proc,
			XV_HELP_DATA,		"mailtool:ExportAttachDir",
			0);
		button_item = xv_create(panel, PANEL_BUTTON,
			PANEL_LABEL_STRING,	gettext("Export"),
			PANEL_NOTIFY_PROC,	export_attach_proc,
			XV_HELP_DATA,		"mailtool:ExportAttachButton",
			0);
		mt_label_frame(popup, gettext("Export Attachment"));
	}

	/* Set key data on panel so we can get back to fields */
	(void)xv_set(panel,
		XV_KEY_DATA,		DIRECTORY_KEY,	directory_item,
		XV_KEY_DATA,		FILE_KEY,	file_item,
		XV_KEY_DATA,		ATTACH_PANEL_KEY, parent_panel,
		/* For Rename attachment */
		XV_KEY_DATA,		VIEW_FRAME_KEY, frame,
		PANEL_DEFAULT_ITEM,	button_item,
		0);

	window_fit_height(panel);
	window_fit_height(popup);


	/* Center button and resize text fields */
	resize_attach_popup(panel);

	return(popup);
}
#endif

static Notify_value
attach_popup_interpose_proc(
	Frame	frame,
	Event	*event,
	Notify_arg	arg,
	Notify_event_type	type
)
{
	if (event_action(event) == WIN_RESIZE) {
		/*
		 * Layout the panel.
		 */
		resize_attach_popup((Panel)xv_get(frame,
					FRAME_NTH_SUBWINDOW, 1));
	}

	return(notify_next_event_func(frame, (Notify_event)event, arg, type));
}

static void
resize_attach_popup(
	Panel	panel
)
{
	Panel_item	item;

	DP(("resize_attach_popup()\n"));

	/* Resize the text fields in panel */
	PANEL_EACH_ITEM(panel, item) {
		if ((Panel_item_type)xv_get(item, PANEL_ITEM_CLASS) ==
							PANEL_TEXT_ITEM) {
			mt_resize_fillin(panel, item);
		} else { /* Center button */
			ds_center_items(panel, -1, item, NULL);
		}
	} PANEL_END_EACH;
}


/* ARGSUSED */
static void
set_modified_if_from_view_window(
	Xv_opaque	window
)
{
	Frame		frame;
        struct view_window_list *vwl;

	frame = (Panel)xv_get(window, XV_KEY_DATA, VIEW_FRAME_KEY);
	vwl = (struct view_window_list *)
		xv_get(frame, XV_KEY_DATA, VWL_KEY);
	if (vwl) {
		vwl->vwl_only_attach_modified = TRUE;
	}
}


#ifdef FILECHOOSER

/* ARGSUSED */
static int
add_attach_proc(
	File_chooser	fc,
	char		*path,
	char		*file,
	Xv_opaque	client_data
)
{
	Attach_list	*al;
	Attach_node	*an;
	int		clear;
	Panel		attach_panel;	/* Panel which owns attach list */

	/* 
	 * Add a file to the attachment list
	 */

	/* If we are operating on a view window, mark it as modified */
	set_modified_if_from_view_window(fc);

	/* Get the attachment list we are working on and add attachment */
	attach_panel = (Panel)xv_get(fc, XV_KEY_DATA, ATTACH_PANEL_KEY);
	al = mt_get_attach_list(attach_panel);

	if ((an = mt_fcreate_attachment(al, path, NULL, FALSE)) == NULL) {
		return XV_ERROR;
	}

	mt_add_attachment(al, an, al->al_msg);

	return XV_OK;
}

#else /*! FILECHOOSER */

/* ARGSUSED */
static void
add_attach_proc(
	Panel_item	item,
	Event		*event
)
{
	Attach_list	*al;
	Attach_node	*an;
	Panel		panel;		/* Panel which owns item */
	Panel		attach_panel;	/* Panel which owns attach list */
	char		path[MAXPATHLEN + 1];
	char		*s;
	Panel_item	directory_item, file_item;
	int		clear;
	int		n;


	/* 
	 * Add a file to the attachment list
	 */
	panel = (Panel)xv_get(item, XV_OWNER);	/* Popup panel */

	set_modified_if_from_view_window(panel);

	attach_panel = (Panel)xv_get(panel, XV_KEY_DATA, ATTACH_PANEL_KEY);
	al = mt_get_attach_list(attach_panel);

	directory_item = (Panel_item)xv_get(panel, XV_KEY_DATA, DIRECTORY_KEY);
	file_item = (Panel_item)xv_get(panel, XV_KEY_DATA, FILE_KEY);

	/* Build path from text items */
	ds_expand_pathname(mt_strip_leading_blanks(
			      (char *)xv_get(directory_item, PANEL_VALUE)
			   ), 
			   path);

	/* avoid duplicate slashes */
	n = strlen(path);
	while (path[n - 1] == '/' ) {
		path[n - 1] = '\0';
		n--;
	}


	s = mt_strip_leading_blanks((char *) xv_get(file_item, PANEL_VALUE));
	if (s && *s) {
		(void)strcat(path, "/");
		(void)strcat(path, s);

		/* Allows complete path to be typed into Directory field */
		n = strlen(path);
		while (path[n - 1] == '/' ) {
			path[n - 1] = '\0';
			n--;
		}
	}

	if ((an = mt_fcreate_attachment(al, path, NULL, FALSE)) == NULL) {
		goto ERROR_EXIT;
	}

	mt_add_attachment(al, an, al->al_msg);

	/* 
	 * If this was the notify proc for a text field then we must check
	 * the pushpin ourselves.
	 */
	if ((Panel_item_type)xv_get(item, PANEL_ITEM_CLASS) == PANEL_TEXT_ITEM)
		check_pushpin((Frame)xv_get(panel, XV_OWNER));
	return;

ERROR_EXIT:
	/* Prevents popup from dismissing if unpinned */
	(void)xv_set(item, PANEL_NOTIFY_STATUS, XV_ERROR, 0);
	return;

}

#endif /* !FILECHOOSER */

#ifdef FILECHOOSER

static char *
get_fc_filename(
	File_chooser	fc
)
{
	Panel_item	file_field;
	char		*value;

	/* Get the file name from the file chooser popup */

	file_field = (Panel_item)xv_get(fc, FILE_CHOOSER_CHILD,
					    FILE_CHOOSER_DOCUMENT_NAME_CHILD);

	if (file_field && !(int)xv_get(file_field, PANEL_INACTIVE) &&
	    (value = (char *)xv_get(file_field, PANEL_VALUE)))
		return value;
	else
		return NULL;
}


/* ARGSUSED */
static int
export_attach_proc(
	File_chooser	fc,
	char		*path,
	int		exists
)
{
	Attach_list	*al;
	Attach_node	*an;
	Panel		attach_panel;	/* Panel which owns attach list */
	int		nselected;
	int		ncopied = 0;
	char		msg_buf[256];


	(void)xv_set(fc, FRAME_LEFT_FOOTER, "", 0);

	/* 
	 * Add a file to the attachment list
	 */
	attach_panel = (Panel)xv_get(fc, XV_KEY_DATA, ATTACH_PANEL_KEY);
	al = mt_get_attach_list(attach_panel);

	if ((nselected = mt_nattachments(al, TRUE)) == 0) {
		mt_vs_warn(al->al_frame,
gettext("Please select the attachment you would like to save"));
		return XV_ERROR;
	} 

	if (get_fc_filename(fc) == NULL) {
		/*
		 * User has not specified a file name.  Assume we are
		 * copying to a directory. Copy all selected attachments.
		 */
		if ((ncopied = export_attachments(al, path)) <= 0)
			goto ERROR_EXIT;

	} else if (nselected > 1) {
		mt_vs_warn(al->al_frame,
gettext("You are trying to save multiple attachments to a single file.\nPlease specify a directory instead and try again."));
		goto ERROR_EXIT;
	} else {
		/* Saving 1 attachment to a file */
		if ((an = mt_get_next_selected_node(al, NULL)) == NULL) {
			mt_vs_warn(al->al_frame,
gettext("Internal error: Could not get selected attachment.\nPlease try again"));
		}

		if (attach_methods.at_read(an->an_at, path, ATTACH_DATA_FILE)) {
			mt_vs_warn(al->al_frame,
gettext("Internal error: Could not save attachment.\nPlease try again"));
			goto ERROR_EXIT;
		}

		ncopied = 1;

	}

	if (ncopied == 1) {
		strcpy(msg_buf, gettext("Attachment saved"));
	} else {
		(void)sprintf(msg_buf, gettext("%d attachments saved"),
								ncopied);
	}

	(void)xv_set(fc, FRAME_LEFT_FOOTER, msg_buf, 0);
	return XV_OK;

ERROR_EXIT:
	(void)xv_set(fc, FRAME_LEFT_FOOTER, gettext("No attachments saved"));
	return XV_ERROR;

}

#else /* !FILECHOOSER */

/* ARGSUSED */
static void
export_attach_proc(
	Panel_item	item,
	Event		*event
)
{
	Attach_list	*al;
	Panel		panel;		/* Panel which owns item */
	Frame		popup_frame;	/* Frame which owns panel */
	Panel		attach_panel;	/* Panel which owns attach list */
	char		path[MAXPATHLEN + 1];
	char		msg_buf[80];
	Panel_item	directory_item;
	int		ncopied;

	/* 
	 * Add a file to the attachment list
	 */
	panel = (Panel)xv_get(item, XV_OWNER);	/* Popup panel */
	attach_panel = (Panel)xv_get(panel, XV_KEY_DATA, ATTACH_PANEL_KEY);
	al = mt_get_attach_list(attach_panel);
	popup_frame = (Frame)xv_get(panel, XV_OWNER); /* Popup frame */

	(void)xv_set(popup_frame,
		FRAME_LEFT_FOOTER, "",
		FRAME_BUSY, TRUE,
		0);

	directory_item = (Panel_item)xv_get(panel, XV_KEY_DATA, DIRECTORY_KEY);

	/* Handle ~ and $VARS */
	ds_expand_pathname(mt_strip_leading_blanks(
			    (char *)xv_get(directory_item, PANEL_VALUE)
			   ),
			   path);

	if (*path == '\0') {
		mt_vs_warn(al->al_frame,
			gettext("Please specify a destintion directory"));
		goto ERROR_EXIT;
	}

	if ((ncopied = export_attachments(al, path)) <= 0)
		goto ERROR_EXIT;

	if (ncopied == 1)
		(void)sprintf(msg_buf, gettext("%d attachment exported"),
								ncopied);
	else
		(void)sprintf(msg_buf, gettext("%d attachments exported"),
								ncopied);

	(void)xv_set(popup_frame,
		FRAME_LEFT_FOOTER, msg_buf,
		FRAME_BUSY, FALSE,
		0);

	/* 
	 * If this was the notify proc for a text field then we must check
	 * the pushpin ourselves.
	 */
	if ((Panel_item_type)xv_get(item, PANEL_ITEM_CLASS) == PANEL_TEXT_ITEM)
		check_pushpin((Frame)xv_get(panel, XV_OWNER));
	return;

ERROR_EXIT:
	/* Prevents popup from dismissing if unpinned */
	(void)xv_set(item, PANEL_NOTIFY_STATUS, XV_ERROR, 0);
	(void)xv_set(popup_frame, FRAME_BUSY, FALSE, 0);
	return;
}


#endif /* !FILECHOOSER */

static
export_attachments(
	Attach_list	*al,
	char		*path
)
{
	Attach_node	*node;
	int		ncopied;

	/* Copy attachments! */
	if ((node = mt_get_next_selected_node(al, NULL)) == NULL) {
		mt_vs_warn(al->al_frame,
gettext("Please select the attachments you would like to export"));
		return(0);
	}

	/* Make sure destination directory is OK */
	if (access(path, W_OK) < 0) {
		mt_vs_warn(al->al_frame,
gettext("Could not copy attachments to\n%s\n%s"), path, strerror(errno));
		return(0);

	}

	ncopied = 0;
	while (node != NULL) {
		if (export_attachment(al->al_frame, node, path, TRUE) > -1)
			ncopied++;
		node = mt_get_next_selected_node(al, node);
	}

	return(ncopied);
}

static
export_attachment(
	Frame		frame,
	Attach_node	*node,
	char		*directory
)
{
	char		*label;
	char		*attach_path;
	char	dest_path[MAXPATHLEN];

	label = mt_attach_name(node->an_at);

	strcpy(dest_path, directory);
	strcat(dest_path, "/");
	strcat(dest_path, label);

	if (access(dest_path, F_OK) == 0) {
		if (mt_vs_confirm(frame, FALSE,
			gettext("Cancel"), gettext("Overwrite"),
			gettext("%s already exists. Overwrite?"), dest_path)) {

			return(-1);
		}
	}

	/*
	 * Decode attachment directly into destination file
	 */
	if (attach_methods.at_read(node->an_at, dest_path, ATTACH_DATA_FILE))
		return(-1);

	return(0);
}

/* ARGSUSED */
static void
select_all_attach_proc(
	Menu		menu,
	Menu_item	menu_item
)
{
	Attach_list	*al;
	Panel		parent_panel;

	/* Select all nodes */
	parent_panel = (Panel)xv_get(menu, MENU_CLIENT_DATA);
	al = mt_get_attach_list(parent_panel);
	select_all_attachments(al);
	/* Keep unpinned frame from dismissing */
	(void)xv_set(menu, MENU_NOTIFY_STATUS, XV_ERROR, 0);
}



/*
 * wrapper for mt_set_attach_selection to make things typesafe...
 */
static int
set_attach_selection(
	Attach_list *al,
	Attach_node *an,
	long unused
)
{
	return (mt_set_attach_selection(al, an));
}



static void
select_all_attachments(
	Attach_list	*al
)
{
	/* Select all attachments and update footer message */
	mt_traverse_attach_list(al, set_attach_selection, 0);
	mt_attach_footer_message(al);
	mt_update_attach_popups(al);
	return;
}



static void
delete_attach_proc(
	Menu		menu,
	Menu_item	menu_item
)
{
	Attach_list	*al;
	Panel		parent_panel;

	/* Delete all selected attachments */
	parent_panel = (Panel)xv_get(menu, MENU_CLIENT_DATA);

	set_modified_if_from_view_window(parent_panel);

	al = mt_get_attach_list(parent_panel);
	mt_traverse_attach_list(al, mt_delete_attach_selection, 0);

	/* Redisplay list */
	mt_set_attach_display_xgap(al);
	mt_layout_attach_display(al);
	mt_repaint_attach_canvas(al, TRUE);
	mt_attach_footer_message(al);
	mt_update_attach_popups(al);
	/* Keep unpinned frame from dismissing */
	(void)xv_set(menu, MENU_NOTIFY_STATUS, XV_ERROR, 0);
}



static void
undelete_attach_proc(
	Menu		menu,
	Menu_item	menu_item
)
{
	Attach_list	*al;
	Panel		parent_panel;
	int	mt_delete_attach_selection();

	/* Delete all selected attachments */
	parent_panel = (Panel)xv_get(menu, MENU_CLIENT_DATA);

	set_modified_if_from_view_window(parent_panel);

	al = mt_get_attach_list(parent_panel);

	mt_undelete_attach(al);

	/* Redisplay list */
	mt_set_attach_display_xgap(al);
	mt_layout_attach_display(al);
	mt_repaint_attach_canvas(al, TRUE);
	mt_attach_footer_message(al);
	mt_update_attach_popups(al);
	/* Keep unpinned frame from dismissing */
	(void)xv_set(menu, MENU_NOTIFY_STATUS, XV_ERROR, 0);
}


/* ARGSUSED */
static void
rename_attach_proc(
	Panel_item	item,
	Event		*event
)
{
	Attach_list	*al;
	Panel		panel;		/* Panel which owns item */
	Frame		popup_frame;	/* Frame which owns panel */
	Panel		attach_panel;	/* Panel which owns attach list */
	char		*name;
	Attach_node	*node;
	Panel_item	name_item;

	/* 
	 * Add a file to the attachment list
	 */
	panel = (Panel)xv_get(item, XV_OWNER);	/* Popup panel */

	set_modified_if_from_view_window(panel);

	attach_panel = (Panel)xv_get(panel, XV_KEY_DATA, ATTACH_PANEL_KEY);
	al = mt_get_attach_list(attach_panel);
	popup_frame = (Frame)xv_get(panel, XV_OWNER); /* Popup frame */

	(void)xv_set(popup_frame,
		FRAME_LEFT_FOOTER, "",
		FRAME_BUSY, TRUE,
		0);

	name_item = (Panel_item)xv_get(panel, XV_KEY_DATA, FILE_KEY);
	name = mt_strip_leading_blanks(
		(char *)xv_get(name_item, PANEL_VALUE)
	       );

	if (*name == '\0') {
		mt_vs_warn(al->al_frame,
			gettext("Please specify a name"));
		goto ERROR_EXIT;
	}

	node = mt_get_next_selected_node(al, NULL);

	if (node == NULL) {
		mt_vs_warn(al->al_frame,
			gettext("Please select an attachment to rename"));
		goto ERROR_EXIT;
	}

	if (mt_get_next_selected_node(al, node)) {
		mt_vs_warn(al->al_frame,
			gettext("Please select only one attachment to rename"));
		goto ERROR_EXIT;

	}

	mt_rename_attachment(al, node, name);

	(void)xv_set(popup_frame,
		FRAME_LEFT_FOOTER, gettext("Attachment renamed"),
		FRAME_BUSY, FALSE,
		0);

	/* 
	 * If this was the notify proc for a text field then we must check
	 * the pushpin ourselves.
	 */
	if ((Panel_item_type)xv_get(item, PANEL_ITEM_CLASS) == PANEL_TEXT_ITEM)
		check_pushpin((Frame)xv_get(panel, XV_OWNER));

	return;

ERROR_EXIT:
	/* Prevents popup from dismissing if unpinned */
	(void)xv_set(item, PANEL_NOTIFY_STATUS, XV_ERROR, 0);
	(void)xv_set(popup_frame, FRAME_BUSY, FALSE, 0);
	return;
}



long
mt_attach_height(
	Attach_list	*al
)
{
	long	height;

	/* 
	 * Report the height in pixels of the attachment panel and canvas
	 * If they are not visible return 0.
	 */
	if (al->al_panel == NULL || !(int)xv_get(al->al_panel, XV_SHOW))
		height = 0;
	else
		height = (int)xv_get(al->al_panel, XV_HEIGHT) +
			 (int)xv_get(al->al_canvas, XV_HEIGHT);

	return(height);
}



void
mt_show_attach_list(
	Attach_list	*al,
	int		show
)
{
	Panel	panel;
	int	height;

	DP(("mt_show_attach_list: show %d, oldshow %d\n",
		show, al->al_panel ? xv_get(al->al_panel, XV_SHOW) : 0));

	/* 
	 * Show/hide the attachment panel and canvas
	 */

	if (!show && al->al_panel == NULL)
		return;

	if (show && al->al_panel == NULL) {
		/* If UI components don't exist create them */
		panel = mt_create_attach_panel(al->al_frame, al);
		mt_create_attach_canvas(al->al_frame, panel, al);
	}

	if (show == (int)xv_get(al->al_panel, XV_SHOW))
		return;

	(void)xv_set(al->al_panel, XV_SHOW, show, 0);
	(void)xv_set(al->al_canvas, XV_SHOW, show, 0);

	return;
}



int
mt_attach_list_visible(
	Attach_list	*al
)
{
	if (al->al_canvas == NULL) {
		return(FALSE);
	} else {
		return((int)xv_get(al->al_canvas, XV_SHOW));
	}
}




void
mt_layout_attach_panes(
	Attach_list	*al,
	Xv_Window	window
)
{
	if (al->al_canvas == NULL) {
		return; /* No attachment list yet */
	}

	/*
	 * Put the panel below the specified window, and put the attachment
	 * canvas below the panel
	 */
	(void)xv_set(al->al_panel, WIN_BELOW, window, 0);
	(void)xv_set(al->al_canvas, WIN_BELOW, al->al_panel, 0);
	return;
}


static void
check_pushpin(
	Frame	frame
)
{
	/*
	 * If the pushpin is out hide the frame. We have to do this
	 * since PANEL_TEXT notif procs don't cause upinned popups
	 * to disappear
	 */
	if (!(int)xv_get(frame, FRAME_CMD_PUSHPIN_IN))
		(void)xv_set(frame, WIN_SHOW, FALSE, 0);
}




void
mt_update_attach_popups(
	Attach_list	*al
)
{
	int	nselected;

	/*
	 * Update the attachment popups based on the attachments selected
	 */
	nselected = mt_nattachments(al, TRUE);

	/* Update Save As popup */
	update_saveas_popup(al, nselected);

	return;
}



static void
update_saveas_popup(
	Attach_list	*al,
	int		nselected
)
{
	Panel_item	file_field, save_button;
	Attach_node	*an;
	char	*file_name = "";
	int	field_inactive = FALSE;
	int	button_inactive = FALSE;

#ifdef FILECHOOSER
	if (al->al_saveas_fc != NULL) {
		file_field = (Panel_item)xv_get(al->al_saveas_fc,
			FILE_CHOOSER_CHILD, FILE_CHOOSER_DOCUMENT_NAME_CHILD);

		save_button = (Panel_item)xv_get(al->al_saveas_fc,
			FILE_CHOOSER_CHILD, FILE_CHOOSER_SAVE_BUTTON_CHILD);

		if (nselected == 1) {
			if ((an = mt_get_next_selected_node(al, NULL)) !=
								 NULL) {
				file_name = mt_attach_name(an->an_at);
			}
		} else if (nselected == 0) {
			button_inactive = TRUE;
		}

		(void)xv_set(al->al_saveas_fc,
			     FILE_CHOOSER_SAVE_TO_DIR, nselected != 1, 0);

		xv_set(al->al_saveas_fc, FILE_CHOOSER_DOC_NAME, file_name, 0);

		if (button_inactive !=
		                    (int)xv_get(save_button, PANEL_INACTIVE)) {
			(void)xv_set(save_button, PANEL_INACTIVE,
						  button_inactive, 0);
		}
	}
#endif

	return;
}


#ifdef FILECHOOSER
static void
add_exten_item( 
     File_chooser fc
)
{
    Panel panel;
    int item_width;
    int item_height;
    int frame_width;
    int frame_height;
    Panel_item item;
    void show_dot_files_proc();
    int  fc_exten_func();
 
    
    panel = xv_get(fc, FRAME_CMD_PANEL);
    
    item = xv_create(panel, PANEL_CHOICE,
                      PANEL_LABEL_STRING,       gettext("Hidden Files:"),
                      PANEL_CHOICE_STRINGS,     gettext("Hide"),
						gettext("Show"), NULL,
                      PANEL_NOTIFY_PROC,        show_dot_files_proc,
                      XV_KEY_DATA,              FC_PARENT_KEY, fc,
                      NULL);
    
    item_width = (int) xv_get(item, XV_WIDTH);
    item_height = (int) xv_get(item, XV_HEIGHT);
 
    
    /*
     * Adjust Frame default size to make room for
     * the extension item.
     */  
    frame_width = (int) xv_get(fc, XV_WIDTH);
    frame_height = (int) xv_get(fc, XV_HEIGHT);
    xv_set(fc,
           XV_WIDTH,  MAX(frame_width, (item_width + xv_cols(panel, 4))),
           XV_HEIGHT, frame_height + item_height,
           NULL);


    /*
     * Adjust Frame Min Size.  provide for at least 2
     * columns on either side of the extension item.
     */  
    xv_get(fc, FRAME_MIN_SIZE, &frame_width, &frame_height);
    xv_set(fc,
           FRAME_MIN_SIZE,
                MAX( frame_width, (item_width + xv_cols(panel, 4))),
                frame_height + item_height,
           NULL);


    /* Tell File Chooser to reserve layout space for it */
    xv_set(fc,
           FILE_CHOOSER_EXTEN_HEIGHT,   item_height,
           FILE_CHOOSER_EXTEN_FUNC,     fc_exten_func,
           XV_KEY_DATA,                 FC_EXTEN_ITEM_KEY, item,
           NULL);
}

static void
show_dot_files_proc( 
     Panel_choice_item item,
     int value,
     Event *event
)
{
    File_chooser fc = xv_get(item, XV_KEY_DATA, FC_PARENT_KEY);

    xv_set(fc, FILE_CHOOSER_SHOW_DOT_FILES, value, NULL);
}

/*
 * FILE_CHOOSER_EXTEN_FUNC, layout extension items within the
 * given extension rect.
 */
static int
fc_exten_func( 
     File_chooser fc,
     Rect *frame_rect,
     Rect *exten_rect,
     int left_edge,
     int right_edge,
     int max_height
)
{
    Panel_item item = (Panel_item) xv_get(fc, XV_KEY_DATA, FC_EXTEN_ITEM_KEY);
    int item_width;

    item_width = (int) xv_get(item, XV_WIDTH);

    /*
     * show item centered in frame.
     */  
    xv_set(item,
           XV_X,        (frame_rect->r_width - item_width) / 2,
           XV_Y,        exten_rect->r_top,
           PANEL_PAINT, PANEL_NONE,
           NULL);

    return -1;
}

#endif
