#ident "@(#)create_panels.c 3.14 07/08/97 Copyr 1987 Sun Micro"

/*	Copyright (c) 1987, 1988, Sun Microsystems, Inc.  All Rights Reserved.
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
/*
 * Mailtool - creation of control panels
 */

#define NULLSTRING (char *) 0
#include <stdio.h>
#include <errno.h>
#include <signal.h>
#include <ctype.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#ifdef SVR4
#include <dirent.h>
#include <netdb.h>
#else
#include <sys/dir.h>
#endif SVR4

#include <xview/selection.h>
#include <xview/sel_svc.h>
#include <xview/sel_attrs.h>
#include <xview/panel.h>
#include <xview/text.h>
#include <xview/xview.h>
#include <xview/font.h>

#include "glob.h"
#include "tool.h"
#include "header.h"
#include "cmds.h"
#include "mle.h"
#include "create_panels.h"
#include "instrument.h"
#include "../maillib/assert.h"

#define DEBUG_FLAG mt_debugging
#include "debug.h"

extern int	mt_undel_list_show();
extern int	mt_props_proc();
extern int	mt_done_menu_proc();

extern Menu	mt_gen_vac_menu();
extern Menu_item canvas_gen_proc();

/* These are used to get and save menu defaults later */
Menu		mt_edit_menu;
Menu		mt_file_menu;
Menu		mt_view_menu;
Menu		mt_compose_menu;




/*
 * the inactivation list is a list of items which becom deactive when
 * there is no selection
 */
static struct inactivation_list	*mt_inactivation_list;


/* local function definitions */
static void mt_create_expert_row(struct header_data *hd, Panel panel, int row);
static void create_file_fillin(HeaderDataPtr, Panel panel, int right_pad);
static void mt_add_to_inactive_list(Xv_opaque handle, int panel_item);
static Menu create_reply_pulldown(struct header_data *hd);


static void
select_headers_from_menu(
	Menu		menu,
	Menu_item	menu_item,
	Event		*ie
)
{
	Event *event;
	struct header_data *hd;

	TRACK_BUTTON(menu, menu_item, "find");

	if (menu) {
		hd = mt_get_header_data(menu);
	} else {
		ASSERT(ie);
		hd = mt_get_header_data(event_window(ie));
	}
	mt_start_header_selection_proc(hd);
}





Menu
generate_headers_popup(
	struct header_data *hd
)
{
	Menu		menu;
	Menu_item	tmp_item;
	int		i;

	/*
	 * Create the menu.  The MENU_CLIENT_DATA determines how the menu
	 * items go active/inactive: 
	 *	0	Always Active
	 *	1	Active only when there is a selected or current msg
	 *	2	Active only when there is stuff in the Undelete list.
	 *
	 * This is hokey, but it removes positional dependencies
	 */

        /* STRING_EXTRACTION -
         *
         * The next group of functions have to do with the frame header
         * popup.  They are the same items as used in the rest of the tool,
         * but appear in a different collection here.
         *
         * The menu is pinnable and has the title "Messages"
         *
         * It has the following menu structure:
         *
         *      Delete
	 *	Move	>
         *      Print
	 *	Load In-Box
	 *	Undelete
         *      Reply 	>
         */

	menu = xv_create(XV_NULL, MENU,
		XV_KEY_DATA, KEY_HEADER_DATA, hd,
		MENU_PIN, TRUE, 

		MENU_ITEM,
			MENU_ACTION_ITEM, gettext("Delete"), mt_del_proc,
			XV_HELP_DATA, "mailtool:DeleteItem",
			MENU_CLIENT_DATA,	1,
			0,
		MENU_ITEM,
			MENU_ACTION_ITEM, gettext("Print"), mt_print_proc, 
#ifdef KYBDACC
			MENU_ACCELERATOR,	"coreset Print",
#endif
			XV_HELP_DATA, 		"mailtool:PrintItem", 
			MENU_CLIENT_DATA,	1,
			0,
		MENU_ITEM,
			MENU_ACTION_ITEM, gettext("Load In-Box"),
							mt_new_mail_proc, 
#ifdef KYBDACC
			MENU_ACCELERATOR,	"coreset Open",
#endif
			XV_HELP_DATA, 		"mailtool:OpenIntray", 
			MENU_CLIENT_DATA,	0,
			0,
		MENU_ITEM,
			MENU_ACTION_ITEM, gettext("Undelete"), mt_undel_last,
			MENU_INACTIVE,	TRUE,
			XV_HELP_DATA, "mailtool:UndeleteLastItem",
			MENU_CLIENT_DATA,	2,
			0,
		MENU_ITEM,
			MENU_PULLRIGHT_ITEM, gettext("Reply"), 
				create_reply_pulldown(hd),
			XV_HELP_DATA, 		"mailtool:ReplyPullright", 
			MENU_CLIENT_DATA,	1,
			0,
		MENU_TITLE_ITEM,	gettext("Messages"),
		MENU_GEN_PIN_WINDOW, MT_FRAME(hd), gettext("Messages"),
		0);

	mt_copy_header_data(MT_FRAME(hd), menu);

#ifdef KYBDACC
	xv_set(MT_FRAME(hd), FRAME_MENU_ADD, menu, NULL);
#endif

	for(i = 1; i <= (int)xv_get(menu, MENU_NITEMS); i++) {
		tmp_item = (Menu_item) xv_get(menu, MENU_NTH_ITEM, i);
		if ((int)xv_get(tmp_item, MENU_CLIENT_DATA) == 1)
			mt_add_to_inactive_list(tmp_item, FALSE);
	}

/* we always use the "expert" layout now... */
#ifdef NOVICELAYOUT
	if (mt_value("expertlayout")) {
#endif
		/*
	 	 * Add the Move menu item to the canvas popup menu
	 	 */
		tmp_item = (Menu_item)xv_create(XV_NULL, MENUITEM,
			MENU_PULLRIGHT_ITEM,gettext("Move"), mt_get_move_menu(),
			XV_HELP_DATA, "mailtool:MoveItem",
			MENU_CLIENT_DATA,	1,
			MENU_GEN_PROC, canvas_gen_proc,
			0);

		mt_add_to_inactive_list(tmp_item, FALSE);
		(void)xv_set(menu, MENU_INSERT, 2, tmp_item, 0);
#ifdef NOVICELAYOUT
	}
#endif

	return(menu);
}





void
mt_set_undelete_inactive(
	struct header_data *hd,
	int	inactive
)
{
	Menu	menu;
	Menu_item	tmp_item;
	int	i;

	/*
	 * Set Undelete items inactive/active
	 */
	(void)xv_set(xv_get(mt_edit_menu, MENU_NTH_ITEM, 5),
				MENU_INACTIVE,	inactive,
				0);

	/*
	 * Loop through the canvas popup and find the Undelete item
	 */
	menu = (Menu)xv_get(hd->hd_canvas, WIN_MENU);
	for(i = 1; i <= (int)xv_get(menu, MENU_NITEMS); i++) {
		tmp_item = (Menu_item) xv_get(menu, MENU_NTH_ITEM, i);
		if ((int)xv_get(tmp_item, MENU_CLIENT_DATA) == 2)
			xv_set(tmp_item, MENU_INACTIVE, inactive, 0);
	}
}






static Menu
create_file_pulldown(
	struct header_data *hd
)
{
	Menu		menu;
	Menu_item	tmp_item;
	Menu    mt_mailfiles_menu;
	int	mt_filelist_show();
	int	itemnum;

        /* STRING_EXTRACTION -
         *
         * The file pulldown.  It has the following menu structure:
         *
         *      Load In-Box
         *      Print
         *      Save Changes
         *      Done
	 *	Mail Files... (in expert mode)
         *
         */

	/* KEY_HEADER_DATA needed by mt_commit_proc */
	menu = xv_create(XV_NULL, MENU, 
		XV_KEY_DATA, KEY_HEADER_DATA, hd,
		0);

	tmp_item = xv_create(XV_NULL, MENUITEM,
		MENU_STRING,		gettext("Load In-Box"),
#ifdef KYBDACC
		MENU_ACCELERATOR,	"coreset Open",
#endif
	        MENU_ACTION_PROC, 	mt_new_mail_proc, 
		XV_HELP_DATA, 		"mailtool:OpenIntray", 
		0);

	xv_set(menu, MENU_APPEND_ITEM, tmp_item, 0);

	define_sys_button(hd, "loadInBox", mt_new_mail_proc, 0,
		"mailtool:OpenIntray", 0);

	tmp_item = xv_create(XV_NULL, MENUITEM, 
		MENU_STRING, 		gettext("Print"),
#ifdef KYBDACC
		MENU_ACCELERATOR,	"coreset Print",
#endif
		MENU_ACTION_PROC, 	mt_print_proc, 
		XV_HELP_DATA, 		"mailtool:PrintItem", 
		0);

	mt_add_to_inactive_list(tmp_item, FALSE);

	xv_set(menu, MENU_APPEND_ITEM, 	tmp_item, 0);

	define_sys_button(hd, "print", mt_print_proc, 0,
		"mailtool:PrintItem", 0);

	xv_set(menu,
		MENU_ITEM,
			MENU_STRING, 		gettext("Save Changes"), 
#ifdef KYBDACC            
			MENU_ACCELERATOR,	"coreset Save",
#endif
			MENU_ACTION_PROC, 	mt_commit_proc, 
			XV_HELP_DATA, 		"mailtool:CommitChanges", 
			0,
		MENU_ITEM,
			MENU_STRING,		gettext("Done"),
			MENU_ACTION_PROC,	mt_done_menu_proc,
			XV_HELP_DATA,		"mailtool:Done",
			0,
		MENU_GEN_PIN_WINDOW, MT_FRAME(hd), gettext("File"),
		0);

	mt_copy_header_data(MT_FRAME(hd), menu);

	define_sys_button(hd, "saveChanges", mt_commit_proc, 0,
		"mailtool:CommitChanges", 0);
	define_sys_button(hd, "done", mt_done_menu_proc, 0,
		"mailtool:Done", 0);

#ifdef NOVICELAYOUT
	if (mt_value("expertlayout")) {
#endif
		/* Make novice popup available to expert users too */
		xv_set(menu,
			MENU_ITEM,
				MENU_STRING, 	gettext("Mail Files..."), 
				MENU_ACTION_PROC, mt_filelist_show, 
				XV_HELP_DATA, 	"mailtool:FileListShow",
				0,
			0);
#ifdef NOVICELAYOUT
	}
#endif

	return(menu);
}






view_short_header_proc(
	Menu		menu,
	Menu_item	menu_item
)
{
	struct header_data *hd;

	TRACK_BUTTON(menu, menu_item, "show_short");
	/* ZZZ: too bad that we can't use menu to get header data */
	hd = mt_get_header_data(menu);
	mt_view_messages(hd, TRUE);
}



static void
view_full_header_proc(
	Menu		menu,
	Menu_item	menu_item

)
{
	struct header_data *hd;

	TRACK_BUTTON(menu, menu_item, "show_full");
	/* ZZZ: too bad that we can't use menu to get header data */
	hd = mt_get_header_data(menu);
	mt_view_messages(hd, FALSE);
}




#ifdef PEM
/* function to decrypt Privacy Enhanced Mail (PEM) */
mt_decrypt_msg_proc(
	Menu		menu,
	Menu_item	menu_item
)
{
	struct header_data *hd;

	TRACK_BUTTON(menu, menu_item, "decrypt_msg");
	/* ZZZ: too bad that we can't use menu to get header data */
	hd = mt_get_header_data(menu);
	mt_decrypt_PEM_messages(hd);
}

#endif PEM




static Menu
create_view_pulldown(
	struct header_data *hd
)
{
	Menu		menu;
	Menu		submenu;
	Menu_item	tmp_item;

	submenu = xv_create(XV_NULL, MENU, 
			XV_KEY_DATA, KEY_HEADER_DATA, hd,
			0);
	menu = xv_create(XV_NULL, MENU, 
			XV_KEY_DATA, KEY_HEADER_DATA, hd,
			0);

	/*
	 * Messages
	 */
        /* STRING_EXTRACTION -
         *
         * The view panel menu.  It has the following format:
         *
         *      Messages
         *              Abbreviated Header
         *              Full Header
         *      Previous
         *      Next
         *      Sort By
         *              Time and Date
         *              Sender
         *              Subject
	 *		Size
	 *		Message Number
         *      Find...
         */

	tmp_item = xv_create(XV_NULL, MENUITEM, 
		MENU_STRING,            gettext("Abbreviated Header"),
		MENU_ACTION_PROC,       view_short_header_proc,
		XV_HELP_DATA,           "mailtool:ViewAbbrev",
		0);

	xv_set(submenu, 
		MENU_APPEND_ITEM, 	tmp_item, 
		0);

	tmp_item = xv_create(XV_NULL, MENUITEM, 
		MENU_STRING,            gettext("Full Header"),
		MENU_ACTION_PROC,       view_full_header_proc,
		XV_HELP_DATA,           "mailtool:ViewFull",
		0);

	xv_set(submenu, 
		MENU_APPEND_ITEM, 	tmp_item, 
		0);

#ifdef PEM
	/* Add a menu item to decrypt Privacy Enhanced Mail (PEM) */

	tmp_item = xv_create(XV_NULL, MENUITEM,
		MENU_STRING,		gettext("Decrypt PEM Message"),
		MENU_ACTION_PROC,	mt_decrypt_msg_proc,
		XV_HELP_DATA,		"mailtool:ViewPEM",
		0);
	
	xv_set(submenu,
		MENU_APPEND_ITEM,	tmp_item,
		0);
	
	define_sys_button(hd, "decryptPEM", mt_decrypt_msg_proc,
		0, "mailtool:ViewPEM", 0);
#endif PEM

	tmp_item = xv_create(XV_NULL, MENUITEM, 
		MENU_PULLRIGHT_ITEM, 	gettext("Messages"), submenu, 
		XV_HELP_DATA, 		"mailtool:ViewMessages", 
		0);

	
	define_sys_button(hd, "abbreviatedHeader", view_short_header_proc,
		0, "mailtool:ViewAbbrev", 0);
	define_sys_button(hd, "fullHeader", view_full_header_proc,
		0, "mailtool:ViewFull", 0);
	define_sys_button(hd, "viewMessages", 0,
		submenu, "mailtool:ViewMessages", 0);

	
	xv_set(menu, 
		MENU_APPEND_ITEM, 	tmp_item, 
		0);

	mt_add_to_inactive_list(tmp_item, FALSE);


	/*
	 * Previous
	 */
	tmp_item = xv_create(XV_NULL, MENUITEM, 
		MENU_STRING, 		gettext("Previous"), 
		MENU_ACTION_PROC, 	mt_prev_proc, 
		XV_HELP_DATA, 		"mailtool:PreviousItem", 
		0);

	xv_set(menu, 
		MENU_APPEND_ITEM, 	tmp_item, 
		0);

	/*
	 * Next
	 */
	tmp_item = xv_create(XV_NULL, MENUITEM, 
		MENU_STRING, 		gettext("Next"), 
		MENU_ACTION_PROC, 	mt_next_proc, 
		XV_HELP_DATA, 		"mailtool:NextItem", 
		0);

	/*
	 * Make Next the default item
	 * Why is it 3 and not 2?  I dunno!  Maybe the pushpin counts as 1
	 */
	xv_set(menu, 
		MENU_APPEND_ITEM, 	tmp_item, 
		MENU_DEFAULT,		4,
		0);

	
	define_sys_button(hd, "previousMessage", mt_prev_proc,
		0, "mailtool:PreviousItem", 0);
	define_sys_button(hd, "nextMessage", mt_next_proc,
		0, "mailtool:NextItem", 0);


	/*
	 * Sort By
	 */
	submenu = xv_create(XV_NULL, MENU, 
                        XV_KEY_DATA, KEY_HEADER_DATA, hd,
			0);

	tmp_item = xv_create(XV_NULL, MENUITEM, 
		MENU_STRING, 		gettext("Time and Date"), 
		MENU_CLIENT_DATA,	 "date",
		MENU_ACTION_PROC,	 mt_sort_proc, 
		XV_HELP_DATA, 		"mailtool:SortDate", 
		0);

	xv_set(submenu, 
		MENU_APPEND_ITEM, 	tmp_item, 
		0);

	tmp_item = xv_create(XV_NULL, MENUITEM, 
		MENU_STRING, 		gettext("Sender"), 
		MENU_CLIENT_DATA,	 "from",
		MENU_ACTION_PROC,	 mt_sort_proc, 
		XV_HELP_DATA, 		"mailtool:SortSender", 
		0);

	xv_set(submenu, 
		MENU_APPEND_ITEM, 	tmp_item, 
		0);

	tmp_item = xv_create(XV_NULL, MENUITEM, 
		MENU_STRING, 		gettext("Subject"), 
		MENU_CLIENT_DATA,	 "subject",
		MENU_ACTION_PROC,	 mt_sort_proc, 
		XV_HELP_DATA, 		"mailtool:SortSubject", 
		0);

	xv_set(submenu, 
		MENU_APPEND_ITEM, 	tmp_item, 
		0);

	tmp_item = xv_create(XV_NULL, MENUITEM, 
		MENU_STRING, 		gettext("Size"), 
		MENU_CLIENT_DATA,	 "size",
		MENU_ACTION_PROC,	 mt_sort_proc, 
		XV_HELP_DATA, 		"mailtool:SortSize", 
		0);

	xv_set(submenu, 
		MENU_APPEND_ITEM, 	tmp_item, 
		0);

	tmp_item = xv_create(XV_NULL, MENUITEM, 
		MENU_STRING, 		gettext("Status"), 
		MENU_CLIENT_DATA,	 "status",
		MENU_ACTION_PROC,	 mt_sort_proc, 
		XV_HELP_DATA, 		"mailtool:SortStatus", 
		0);

	xv_set(submenu, 
		MENU_APPEND_ITEM, 	tmp_item, 
		0);

	tmp_item = xv_create(XV_NULL, MENUITEM, 
		MENU_STRING, 		gettext("Message Number"), 
		MENU_CLIENT_DATA,	 "msg_number",
		MENU_ACTION_PROC,	 mt_sort_proc, 
		XV_HELP_DATA, 		"mailtool:SortMsgno", 
		0);

	xv_set(submenu, 
		MENU_APPEND_ITEM, 	tmp_item, 
		0);

	tmp_item = xv_create(XV_NULL, MENUITEM, 
		MENU_PULLRIGHT_ITEM, 	gettext("Sort By"), submenu, 
		XV_HELP_DATA, 		"mailtool:SortBy", 
		0);
	
	xv_set(menu, 
		MENU_APPEND_ITEM, 	tmp_item, 
		0);

	define_sys_button(hd, "sortByTimeAndDate", mt_sort_proc,
		0, "mailtool:SortDate", "date");
	define_sys_button(hd, "sortBySender", mt_sort_proc,
		0, "mailtool:SortSender", "from");
	define_sys_button(hd, "sortBySubject", mt_sort_proc,
		0, "mailtool:SortSubject", "subject");
	define_sys_button(hd, "sortBySize", mt_sort_proc,
		0, "mailtool:SortSize", "size");
	define_sys_button(hd, "sortByStatus", mt_sort_proc,
		0, "mailtool:SortStatus", "msg_number");
	define_sys_button(hd, "sortByMsgno", mt_sort_proc,
		0, "mailtool:SortMsgno", "msg_number");

	define_sys_button(hd, "sortMenu", 0,
		submenu, "mailtool:SortBy", 0);

	/*
	 * Find...
	 */
	tmp_item = xv_create(XV_NULL, MENUITEM,
		MENU_STRING,            gettext("Find..."),
#ifdef KYBDACC
		MENU_ACCELERATOR,	"coreset Find",
#endif
		MENU_ACTION_PROC, select_headers_from_menu,
		XV_HELP_DATA, "mailtool:SelectItem",
		0);
	define_sys_button(hd, "findButton", select_headers_from_menu,
		0, "mailtool:SelectItem", 0);

	xv_set(menu,
		MENU_APPEND_ITEM,       tmp_item,
		MENU_GEN_PIN_WINDOW, MT_FRAME(hd), gettext("View"), 
		0);

	mt_copy_header_data(MT_FRAME(hd), menu);

	return(menu);
}




static Menu
create_edit_pulldown(
	struct header_data *hd
)
{
	Menu		menu;
	Menu		submenu;
	Menu_item	tmp_item;

	menu = xv_create(XV_NULL, MENU,
			XV_KEY_DATA, KEY_HEADER_DATA, hd,
			0);

        /* STRING_EXTRACTION -
         *
         * The edit menu
         *
         *      Cut
         *      Copy
         *      Delete
         *      Undelete
         *              Last
         *              From List...
         *
         *      Properties...
         */

	tmp_item = xv_create(XV_NULL, MENUITEM, 
		MENU_STRING, 		gettext("Cut"), 
#ifdef KYBDACC
		MENU_ACCELERATOR,	"coreset Cut",
#endif
		MENU_ACTION_PROC, 	mt_cut_proc, 
		XV_HELP_DATA, 		"mailtool:CutItem", 
		0);

	mt_add_to_inactive_list(tmp_item, FALSE);

	xv_set(menu, 
		MENU_APPEND_ITEM, 	tmp_item, 
		0);

	tmp_item = xv_create(XV_NULL, MENUITEM, 
		MENU_STRING, 		gettext("Copy"), 
#ifdef KYBDACC
		MENU_ACCELERATOR,	"coreset Copy",
#endif
		MENU_ACTION_PROC, 	mt_copyshelf_proc, 
		XV_HELP_DATA, 		"mailtool:CopyItem", 
		0);

	mt_add_to_inactive_list(tmp_item, FALSE);

	xv_set(menu, 
		MENU_APPEND_ITEM, 	tmp_item, 
		0);

	tmp_item = xv_create(XV_NULL, MENUITEM, 
		MENU_STRING, 		gettext("Delete"), 
		MENU_ACTION_PROC, 	mt_del_proc, 
		XV_HELP_DATA, 		"mailtool:DeleteItem", 
		0);

	mt_add_to_inactive_list(tmp_item, FALSE);

	xv_set(menu, 
		MENU_APPEND_ITEM, 	tmp_item, 
		MENU_DEFAULT,		4,
		0);

	/*
	 * Undelete
	 */
	tmp_item = xv_create(XV_NULL, MENUITEM, 
		MENU_STRING, 		gettext("Last"), 
		MENU_ACTION_PROC, 	mt_undel_last, 
		XV_HELP_DATA, 		"mailtool:UndeleteLastItem", 
		0);

	submenu = xv_create(XV_NULL, MENU, 
		XV_KEY_DATA, KEY_HEADER_DATA, hd,
		0);
	xv_set(submenu, 
		MENU_APPEND_ITEM, 	tmp_item, 
		0);

	tmp_item = xv_create(XV_NULL, MENUITEM, 
		MENU_STRING,		gettext("From List..."),
		MENU_ACTION_PROC,	mt_undel_list_show,
		XV_HELP_DATA, 		"mailtool:UndeleteList", 
		0);
	
	xv_set(submenu, 
		MENU_APPEND_ITEM, 	tmp_item, 
		0);

	tmp_item = xv_create(XV_NULL, MENUITEM, 
		MENU_PULLRIGHT_ITEM, 	gettext("Undelete"), submenu, 
		XV_HELP_DATA, 		"mailtool:UndeletePullright", 
		MENU_INACTIVE,		(mt_delp == NULL),
		0);
	
	xv_set(menu, 
		MENU_APPEND_ITEM, 	tmp_item, 
		0);

	mt_menu_append_blank_item(menu);

	tmp_item = xv_create(XV_NULL, MENUITEM, 
		MENU_STRING, 		gettext("Properties..."), 
#ifdef KYBDACC
		MENU_ACCELERATOR,	"coreset Props",
#endif
		MENU_ACTION_PROC, 	mt_props_proc, 
		XV_HELP_DATA, 		"mailtool:PropsItem", 
		0);

	xv_set(menu,
		MENU_APPEND_ITEM, 	tmp_item,
		MENU_GEN_PIN_WINDOW, MT_FRAME(hd), gettext("Edit"),
		0);

	mt_copy_header_data(MT_FRAME(hd), menu);

	define_sys_button(hd, "cutMessage", mt_cut_proc, 0,
		"mailtool:CutItem", 0);

	define_sys_button(hd, "copyMessage", mt_copyshelf_proc, 0,
		"mailtool:CopyItem", 0);

	define_sys_button(hd, "deleteMessage", mt_del_proc, 0,
		"mailtool:DeleteItem", 0);

	define_sys_button(hd, "undeleteLastMessage", mt_undel_last, 0,
		"mailtool:UndeleteLastItem", 0);

	define_sys_button(hd, "undeleteFromList", mt_undel_list_show, 0,
		"mailtool:UndeleteList", 0);

	define_sys_button(hd, "undeleteMenu", 0, submenu,
		"mailtool:UndeletePullright", 0);

	define_sys_button(hd, "properties", mt_props_proc, 0,
		"mailtool:PropsItem", 0);

	return(menu);
}





static Menu
create_reply_pulldown(
        struct header_data *hd
)
{
	Menu		menu;
	Menu_item	item;

        /* STRING_EXTRACTION -
         *
         * The compose menu
         *
         *      New
         *      Reply
         *              To Sender
         *              To All
         *              To Sender, Include
         *              To All, Include
         *      Forward
         *
         *      Vacation
         */

	menu = xv_create(XV_NULL, MENU,
		XV_KEY_DATA, KEY_HEADER_DATA, hd,
		MENU_ITEM,
			MENU_ACTION_ITEM, gettext("To Sender"), mt_reply_proc, 
			MENU_CLIENT_DATA, 0,
			XV_HELP_DATA, "mailtool:ReplySenderItem",
			0,
		MENU_ITEM,
			MENU_ACTION_ITEM, gettext("To All"), mt_reply_proc, 
			MENU_CLIENT_DATA, 1,
			XV_HELP_DATA, "mailtool:ReplyAllItem",
			0,
		MENU_ITEM,
			MENU_ACTION_ITEM, gettext("To Sender, Include"),
				mt_reply_proc, 
			MENU_CLIENT_DATA, 2,
			XV_HELP_DATA, "mailtool:ReplySenderIncludeItem",
			0,
		MENU_ITEM,
			MENU_ACTION_ITEM, gettext("To All, Include"),
				mt_reply_proc, 
			MENU_CLIENT_DATA, 3,
			XV_HELP_DATA, "mailtool:ReplyAllIncludeItem",
			0,
		0);

	define_sys_button(hd, "replySender", mt_reply_proc, 0,
		"mailtool:ReplySenderItem", 0);

	define_sys_button(hd, "replyAll", mt_reply_proc, 0,
		"mailtool:ReplyAllItem", 1);

	define_sys_button(hd, "replySenderInclude", mt_reply_proc, 0,
		"mailtool:ReplySenderIncludeItem", 2);

	define_sys_button(hd, "replyAllInclude", mt_reply_proc, 0,
		"mailtool:ReplyAllIncludeItem", 3);

	return(menu);
}






static Menu
create_composition_pulldown(
        struct header_data *hd
)
{
	Menu		menu, sub_menu;
	Menu_item	tmp_item;

	menu = xv_create(XV_NULL, MENU,
			XV_KEY_DATA, KEY_HEADER_DATA, hd,
			0);

	tmp_item = xv_create(XV_NULL, MENUITEM, 
		MENU_ACTION_ITEM, gettext("New"), mt_comp_proc,
#ifdef KYBDACC
		MENU_ACCELERATOR,	"coreset New",
#endif
		MENU_CLIENT_DATA, 0,
		XV_HELP_DATA,		"mailtool:ComposePullright",
		0);
		
	xv_set(menu, MENU_APPEND_ITEM, 	tmp_item, 0);

	define_sys_button(hd, "composeNew", mt_comp_proc, 0,
		"mailtool:ComposePullright", 0);

	sub_menu = create_reply_pulldown(hd);

	tmp_item = xv_create(XV_NULL, MENUITEM, 
		MENU_PULLRIGHT_ITEM, 	gettext("Reply"), sub_menu,
		XV_HELP_DATA,		"mailtool:ReplyPullright",
		0);
			
	xv_set(menu, MENU_APPEND_ITEM, 	tmp_item, 0);
	mt_add_to_inactive_list(tmp_item, FALSE);

	define_sys_button(hd, "replyMenu", 0, sub_menu,
		"mailtool:ReplyPullright", 0);

	tmp_item = xv_create(XV_NULL, MENUITEM, 
		MENU_ACTION_ITEM,	gettext("Forward"), mt_comp_proc,
		MENU_CLIENT_DATA,	2,
		XV_HELP_DATA,		"mailtool:ForwardPullright",
		0);
		
	xv_set(menu, MENU_APPEND_ITEM, 	tmp_item, 0);
	mt_add_to_inactive_list(tmp_item, FALSE);

	define_sys_button(hd, "forwardMessage", mt_comp_proc, 0,
		"mailtool:ForwardPullright", 2);

	mt_menu_append_blank_item(menu);


	/*
	 * Vacation
	 */
	sub_menu = xv_create(XV_NULL, MENU,
		MENU_GEN_PROC, mt_gen_vac_menu,
		0);

	tmp_item = xv_create(XV_NULL, MENUITEM, 
		MENU_PULLRIGHT_ITEM, 	gettext("Vacation"), sub_menu, 
		XV_HELP_DATA, "mailtool:VacationMenu",
		0);

	define_sys_button(hd, "vacationMenu", 0, sub_menu,
		"mailtool:VacationMenu", 0);

	xv_set(menu,
		MENU_APPEND_ITEM, 	tmp_item,
		MENU_GEN_PIN_WINDOW, MT_FRAME(hd), gettext("Compose"),
		0);

	mt_copy_header_data(MT_FRAME(hd), menu);

	return(menu);
}





/*
 * create control panel using standard buttons, simplified layout.
 * Panel is 3 rows high, not scrollable 
 */
void
mt_create_control_panel(
	struct header_data *hd
)
{
	Menu            menu;
	Menu_item	menu_item;
	int		n, charwidth;
	int     	mt_filelist_show();

	charwidth = xv_get(mt_font, FONT_COLUMN_WIDTH);

	/*
	 * Scrunch buttons closer together
	 */

        /* STRING_EXTRACTION -
         *
         * The names of the top level header panel.  It has the following
         * buttons:
         *
         *      File   View   Edit   Compose   Mail File:
         */
	xv_set(hd->hd_cmdpanel,
		XV_KEY_DATA, KEY_HEADER_DATA, hd,
		WIN_COLUMN_GAP,
			(int)xv_get(hd->hd_cmdpanel, WIN_COLUMN_GAP) / 2,
		WIN_ROW_GAP,
			(int)xv_get(hd->hd_cmdpanel, WIN_ROW_GAP) / 3,
		PANEL_ITEM_X_GAP,
			(int)xv_get(hd->hd_cmdpanel,PANEL_ITEM_X_GAP) / 2,
		PANEL_ITEM_Y_GAP,
			(int)xv_get(hd->hd_cmdpanel,PANEL_ITEM_Y_GAP) / 3,
		WIN_COLUMNS,		200,
		0);

	/* create the pulldown menus for the "file" button 
	 * on the top level of the command panel.
	 */
	mt_file_menu = create_file_pulldown(hd);
#ifdef KYBDACC
	xv_set(MT_FRAME(hd), FRAME_MENU_ADD, mt_file_menu, NULL);
#endif
	define_sys_button(hd, "file", 0, mt_file_menu, "mailtool:FileButton", 0);
	bind_button(hd, 0, 0, gettext("File"), "file");

	mt_view_menu = create_view_pulldown(hd);
#ifdef KYBDACC
	xv_set(MT_FRAME(hd), FRAME_MENU_ADD, mt_view_menu, NULL);
#endif
	define_sys_button(hd, "view", 0, mt_view_menu, "mailtool:ViewButton", 0);
	bind_button(hd, 0, 1, gettext("View"), "view");

	mt_edit_menu = create_edit_pulldown(hd);
#ifdef KYBDACC
	xv_set(MT_FRAME(hd), FRAME_MENU_ADD, mt_edit_menu, NULL);
#endif
	define_sys_button(hd, "edit", 0, mt_edit_menu, "mailtool:EditButton", 0);
	bind_button(hd, 0, 2, gettext("Edit"), "edit");

	mt_compose_menu = create_composition_pulldown(hd);
#ifdef KYBDACC
	xv_set(MT_FRAME(hd), FRAME_MENU_ADD, mt_compose_menu, NULL);
#endif
	define_sys_button(hd, "compose", 0, mt_compose_menu,
		"mailtool:ComposeButton", 0);
	bind_button(hd, 0, 3, gettext("Compose"), "compose");

	define_sys_button(hd, "mailfiles", mt_filelist_show, 0,
		"mailtool:MailFilesButon", 0);

#ifdef NOVICELAYOUT
	if (mt_value("expertlayout")) {
#endif
		create_file_fillin(hd, hd->hd_cmdpanel, 2 * charwidth);
		mt_create_expert_row(hd, hd->hd_cmdpanel, 1);
#ifdef NOVICELAYOUT
	} else {
		hd->hd_file_fillin = NULL;

		bind_button(hd, 0, 4, gettext("Mail Files..."), "mailfiles");
	}
#endif

	commit_buttons(hd);
}


#ifdef MULTIPLE_FOLDERS
/*
 * 
 * 
 */
void
mt_create_control_panel1(
	struct header_data *hd
)
{
	Menu            menu;
	Menu_item	menu_item;
	int		n, charwidth;
	int     	mt_filelist_show();

	charwidth = xv_get(mt_font, FONT_COLUMN_WIDTH);

	/*
	 * Scrunch buttons closer together
	 */

        /* STRING_EXTRACTION -
         *
         * The names of the top level header panel.  It has the following
         * buttons:
         *
         *      File   View   Edit   Compose   Mail File:
         */
	xv_set(hd->hd_cmdpanel,
		XV_KEY_DATA, KEY_HEADER_DATA, hd,
		WIN_COLUMN_GAP,
			(int)xv_get(hd->hd_cmdpanel, WIN_COLUMN_GAP) / 2,
		WIN_ROW_GAP,
			(int)xv_get(hd->hd_cmdpanel, WIN_ROW_GAP) / 3,
		PANEL_ITEM_X_GAP,
			(int)xv_get(hd->hd_cmdpanel,PANEL_ITEM_X_GAP) / 2,
		PANEL_ITEM_Y_GAP,
			(int)xv_get(hd->hd_cmdpanel,PANEL_ITEM_Y_GAP) / 3,
		WIN_COLUMNS,		200,
		0);

	/* create the pulldown menus for the "file" button 
	 * on the top level of the command panel.
	 */
	menu = create_file_pulldown(hd);
#ifdef KYBDACC
        xv_set(MT_FRAME(hd), FRAME_MENU_ADD, mt_file_menu, NULL);
#endif
	define_sys_button(hd, "file", 0, menu, "mailtool:FileButton", 0);
	bind_button(hd, 0, 0, gettext("File"), "file");

	menu = create_view_pulldown(hd);
#ifdef KYBDACC
        xv_set(MT_FRAME(hd), FRAME_MENU_ADD, mt_view_menu, NULL);
#endif
	define_sys_button(hd, "view", 0, menu, "mailtool:ViewButton", 0);
	bind_button(hd, 0, 1, gettext("View"), "view");

	menu = create_edit_pulldown(hd);
#ifdef KYBDACC
        xv_set(MT_FRAME(hd), FRAME_MENU_ADD, mt_edit_menu, NULL);
#endif
	define_sys_button(hd, "edit", 0, menu, "mailtool:EditButton", 0);
	bind_button(hd, 0, 2, gettext("Edit"), "edit");

	menu = create_composition_pulldown(hd);
#ifdef KYBDACC
        xv_set(MT_FRAME(hd), FRAME_MENU_ADD, mt_compose_menu, NULL);
#endif
	define_sys_button(hd, "compose", 0, menu,
		"mailtool:ComposeButton", 0);
	bind_button(hd, 0, 3, gettext("Compose"), "compose");

/* global here */
	define_sys_button(hd, "mailfiles", mt_filelist_show, 0,
		"mailtool:MailFilesButon", 0);

#ifdef NOVICELAYOUT
	if (mt_value("expertlayout")) {
#endif
		create_file_fillin(hd, hd->hd_cmdpanel, 2 * charwidth);
		mt_create_expert_row(hd, hd->hd_cmdpanel, 1);
#ifdef NOVICELAYOUT
	} else {
		hd->hd_file_fillin = NULL;

		bind_button(hd, 0, 4, gettext("Mail Files..."), "mailfiles");
	}
#endif

	commit_buttons(hd);
}
#endif MULTIPLE_FOLDERS






static void
mt_add_to_inactive_list(
	Xv_opaque	handle,
	int		panel_item
)
{
	struct inactivation_list	*base;
	struct inactivation_list	*new_entry;

	base = mt_inactivation_list;
	
	new_entry = (struct inactivation_list *) malloc(sizeof(struct inactivation_list));
	new_entry->handle = handle;
	new_entry->panel_item = panel_item;
	new_entry->next_ptr = base;
	mt_inactivation_list = new_entry;
}




void
mt_deactivate_functions(
	void
)
{
	struct inactivation_list	*base;

	base = mt_inactivation_list;
	while (base)
	{
		if (base->panel_item)
		{
			xv_set(base->handle, PANEL_INACTIVE, TRUE, 0);
		}
		else /* a menu item */
		{
			xv_set(base->handle, MENU_INACTIVE, TRUE, 0);
		}
		base = base->next_ptr;
	}

}




void
mt_activate_functions(
	void
)
{
	struct inactivation_list	*base;

	base = mt_inactivation_list;

	while (base)
	{
		if (base->panel_item)
		{
			xv_set(base->handle, PANEL_INACTIVE, FALSE, 0);
		}
		else /* a menu item */
		{
			xv_set(base->handle, MENU_INACTIVE, FALSE, 0);
		}
		base = base->next_ptr;
	}

}





Menu_item
mt_menu_append_blank_item(
	Menu	menu
)
{
	Menu_item	tmp_item;

	tmp_item = menu_create_item(
		MENU_STRING, 		"", 
		MENU_FEEDBACK,	FALSE,
		0);

	menu_set(menu, 
		MENU_APPEND_ITEM, 	tmp_item, 
		0);

	return(tmp_item);
}





static void
reposition_callback(
	Panel_item button,
	int x,
	int y
)
{
	int width;
	Xv_opaque parent;
	struct header_data *hd;

	parent = (Xv_opaque) xv_get(button, PANEL_PARENT_PANEL);
	hd = mt_get_header_data(parent);
	xv_set(button, XV_X, x, XV_Y, y, 0);
	width = xv_get(button, XV_WIDTH) + xv_get(parent, WIN_ROW_GAP);

	xv_set(hd->hd_file_fillin, XV_X, x + width, XV_Y, y, 0);
}





static void
create_file_fillin(
	struct header_data *hd,
	Panel	panel,
	int	right_pad
)
{
        Panel_item      tmp_item;
	Menu		menu;
	extern Menu	mt_mail_file_menu_create();
	int		x_gap;

	menu = mt_mail_file_menu_create(hd);
	tmp_item = xv_create(panel, PANEL_ABBREV_MENU_BUTTON,
		PANEL_ITEM_MENU,     menu,
		PANEL_LABEL_STRING,	gettext("Mail File:"),
		PANEL_LABEL_BOLD,	TRUE,
		XV_HELP_DATA, "mailtool:MailFileMenu",
		0);

	set_button_callback(hd, 0, 6, reposition_callback, tmp_item);

	hd->hd_file_fillin = xv_create(panel, PANEL_TEXT,
		PANEL_VALUE_DISPLAY_LENGTH, 	20,
		XV_HELP_DATA, "mailtool:MailFile",
		0);
}





void
mt_resize_fillin(
	Panel		panel,
	Panel_item	fillin
)
{
	int	width;
	int	n;

	if (fillin == NULL)
		return;

	/*
	 * Set the display width of the fillin field to extend to the
	 * right edge of the panel.  This will vary depending on how the
	 * user resizes the tool
	 */
	width = (int)xv_get(panel, XV_WIDTH) -
			(int)xv_get(fillin, PANEL_VALUE_X) - 5;

	n = width / (int)xv_get(mt_font, FONT_COLUMN_WIDTH);

	if (n < 5)
		n = 5;
	else if (n > (int)xv_get(fillin, PANEL_VALUE_STORED_LENGTH))
		n = (int)xv_get(fillin, PANEL_VALUE_STORED_LENGTH);

	(void)xv_set(fillin, PANEL_VALUE_DISPLAY_LENGTH, n, 0);

	return;
}





static void
create_accel(
        struct header_data *hd,
	int row,
	int col,
	char *name,
	char *label,
	int op,
	char *help
)
{

	Menu menu;

	menu = mt_short_file_menu_create(hd, MT_FRAME(hd), op, name);

	define_sys_button(hd, name, 0, menu, help, 0);
	bind_button(hd, row, col, label, name);
}




static void
mt_create_expert_row(
        struct header_data *hd,
	Panel	panel,
	int	row
)
{
	char *name;

	/*
	 * Create the row of accelerator buttons beneath the  File, View, Edit
	 * and Compose menu buttons.
	 */

        /* STRING_EXTRACTION -
         *
         * The names of the row of accelerator buttons beneath 
	 * the  File, View, Edit and Compose menu buttons..  
	 * It has the following buttons:
         *
         *      Done   Next   Delete   Reply
         *      
         */
	bind_button(hd, 1, 0, gettext("Done"), "done");
	bind_button(hd, 1, 1, gettext("Next"), "nextMessage");
	bind_button(hd, 1, 2, gettext("Delete"), "deleteMessage");
	bind_button(hd, 1, 3, gettext("Reply"), "replyMenu");

        /* STRING_EXTRACTION -
         *
         * Move List, Copy List, and Load List are the names
	 * bound to the pinnable menus for the move, copy, and
	 * load buttons.
         *
         */
	create_accel(hd, 1, 6, gettext("Move List"), gettext("Move"),
		MT_MOVE_OP, "mailtool:MoveAccel");

	create_accel(hd, 1, 7, gettext("Copy List"), gettext("Copy"),
		MT_COPY_OP, "mailtool:CopyAccel");

	create_accel(hd, 1, 8, gettext("Load List"), gettext("Load"),
		MT_LOAD_OP, "mailtool:LoadAccel");
#ifdef MULTIPLE_FOLDERS
	create_accel(hd, 1, 9, "Load List1", gettext("Load New"), MT_LOAD1_OP,
		"mailtool:LoadAccel");
#endif

}
