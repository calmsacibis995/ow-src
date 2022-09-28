#ifndef lint
static  char sccsid[] = "@(#)generate_menus.c 3.10 93/02/11 Copyr 1985 Sun Micro";
#endif

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
 * Mailtool - building menus for the cmdpanel
 */

#include <stdio.h>
#include <errno.h>
#include <signal.h>
#include <ctype.h>
#include <values.h>
#include <xview/window_hs.h>
#include <sys/stat.h>
#include <sys/types.h>
#ifdef SVR4
#include <sys/kbd.h>
#include <sys/kbio.h>
#else
#include <sundev/kbd.h>
#endif SVR4

#include <xview/panel.h>
#include <xview/text.h>
#include <xview/font.h>
#include <xview/xview.h>

#include "glob.h"
#include "tool.h"
#include "tool_support.h"
#include "mle.h"
#include "cmds.h"
#include "../maillib/ck_strings.h"

#define		MT_OP_KEY	1
#define		MT_FIELD_KEY	2
#define		MT_FROM_CANVAS_POPUP_MENU  3

#define DEBUG_FLAG mt_debugging
extern DEBUG_FLAG;
#include "debug.h"

extern	Menu		mt_file_menu;

typedef struct history_menu_data {
	char	*hmd_file;		/* File name for menu */
	int	hmd_file_size;		/* length of hmd_file */
	int	hmd_age;		/* Lower the # the older the entry */
				/* add this in to store cached items */
	struct history_menu_data	*hmd_next;
} History_menu_data;

typedef struct history_menu_list {
	Menu	hml_menu;
	struct history_menu_list	*hml_next;
} History_menu_list;

static	History_menu_list	*mt_history_menu_list;

static	Menu_item	fillin_menu_item_gen_proc();
static	Menu_item	add_to_history_menu();
void			mt_props_dynamic_update_menus();
static	void		update_history_menu();

static  char 		*save_filemenu_string = NULL;


/*
 * set the value of the history file name.  If the buffer is big enough
 * then just copy it in; otherwise allocate a larger buffer.
 */
static void
set_history_file(hmd, string)
History_menu_data *hmd;
char *string;
{
	int len;

	len = strlen(string) + 1;

	if (hmd->hmd_file == NULL || len > hmd->hmd_file_size) {
		if (hmd->hmd_file) {
			free(hmd->hmd_file);
		}

		hmd->hmd_file = ck_malloc(len);
		hmd->hmd_file_size = len;
	}

	strcpy(hmd->hmd_file, string);
}


static void
add_history_menu(list_p, menu)

	History_menu_list	**list_p;
	Menu			menu;

{
	History_menu_list	*p;

	/*
	 * Add a menu to the list of history menus.  We need this
	 * list so that we can easily add a folder name to all menus.
	 */
	p = (History_menu_list *)malloc(sizeof(History_menu_list));
	if (p == NULL)
		return;

	p->hml_menu = menu;
	p->hml_next = *list_p;
	*list_p = p;
}


Menu
mt_get_move_menu()

{
	History_menu_list	*p;
	int			op;
	Menu			menu;

	for (p = mt_history_menu_list; p != NULL; p = p->hml_next) {
		op = (int)xv_get(p->hml_menu, XV_KEY_DATA, MT_OP_KEY);
		if (op == MT_MOVE_OP)
			return(p->hml_menu);
	}

	return(NULL);
}

mt_file_history_list_add(file, never_bump)

	char	*file;
	int	never_bump;

{
	History_menu_list	*list;

	/*
	 * Add a file name to all history menus
	 */
	list = mt_history_menu_list;
	for (; list != NULL; list = list->hml_next) {
		add_to_history_menu(list->hml_menu, file, never_bump);
	}
}

static
history_menu_action_proc(menu, menu_item)

Menu		menu;
Menu_item	menu_item;

{
	Panel_item	fillin;
	struct header_data *hd;

	hd = mt_get_header_data(menu);
	fillin = (Panel_item)xv_get(menu, XV_KEY_DATA, MT_FIELD_KEY);

	if (fillin != NULL)
		xv_set(fillin, PANEL_VALUE, xv_get(menu_item, MENU_STRING), 0);

	switch ((int)xv_get(menu, XV_KEY_DATA, MT_OP_KEY)) {

	case MT_LOAD_OP:
		mt_set_new_folder(hd);
		break;
#ifdef MULTIPLE_FOLDERS
	case MT_LOAD1_OP:
		mt_set_new_folder1(hd);
		break;
#endif
	case MT_MOVE_OP:
		mt_save_proc(menu, menu_item);
		break;
	case MT_COPY_OP:
		mt_copy_proc(menu, menu_item);
		break;
	case MT_OPEN_OP:
		mt_set_new_folder(hd);
		break;
	default:
		break;
	}
}


/*
 * This is called from canvas menu's move item.
 * We set a flag here so that fillin_menu_item_gen_proc()
 * later will see if its MENU_NOTIFY is from this canvas menu.
 * If it is, then we will xv_get from textfield, else
 * we ignore all its other MENU_NOTIFYs.
 */

Menu_item
canvas_gen_proc(item, operation)

Menu_item	item;
Menu_generate	operation;

{
	Menu	move_menu;

	switch (operation) {

	case MENU_NOTIFY:
		move_menu = mt_get_move_menu(); 
		if (move_menu) {
                	(void) xv_set(move_menu, XV_KEY_DATA,
                        	MT_FROM_CANVAS_POPUP_MENU,
                        	TRUE, 0);
		}
		break;
	}

	return(item);
}


static Menu_item
fillin_menu_item_gen_proc(item, operation)

	Menu_item	item;
	Menu_generate	operation;
{
	History_menu_data	*p;
	Panel_item	fillin;
	Menu		menu;
	char		*string;
	char		*string_strip;
	int		len;
	
        register int    n;
	Menu_item	menu_item;

	/*
	 * Generate the item based on the contents of the Mail File field
	 */
	switch (operation) {

	/*
	 * We only gen the item on a display.  This means it gets gen'd every
	 * time the menu is displayed, plus anytime the Move/Copy or Load 
	 * is selected with this item as the default (it's gen'd because
	 * the button preview displays the default item).
	 *
	 * This means it is not gen'd when the user selects this item on
	 * the *pinned* version of the menu.  This is good, otherwise the
	 * item would gen *after* the item was selected -- bad news.
	 */
	/*
 	 * We check if MENU_NOTIFY is from the canvas menu's move item.
	 * If it is, then we will xv_get from textfield, else
	 * we ignore all other MENU_NOTIFYs.
 	 */
	case MENU_DISPLAY:
	case MENU_NOTIFY:
		menu = (Menu)xv_get(item, MENU_PARENT);

		if (operation == MENU_NOTIFY) {
		    if (!(int)xv_get(menu, XV_KEY_DATA, 
				MT_FROM_CANVAS_POPUP_MENU)) {
			break;
		    }
		    /* reset flag to false */
		    (void) xv_set(menu, XV_KEY_DATA, 
		  	  	MT_FROM_CANVAS_POPUP_MENU,
			  	FALSE, 0);
		}

		if (menu == NULL) {
			/*
			 * Hack around this XView bug 1041338.
			 * - bug fixed.
			fillin = file_fillin;
			 */
			return;
		} else {
			fillin = (Panel_item)
				xv_get(menu, XV_KEY_DATA, MT_FIELD_KEY);
		}
		p = (History_menu_data *)xv_get(item, MENU_CLIENT_DATA);
		string = (char *) xv_get(fillin, PANEL_VALUE);

		/* 
         	 * In this case, because we are overwriting something, 
	  	 * (in strip trailing blanks),
        	 * we should make a copy of the xv_get.	
		 */
		string = (char *)strdup(string);
        	string_strip = mt_strip_leading_blanks(string);
        	string_strip = mt_strip_trailing_blanks(string_strip);
		xv_set(fillin, PANEL_VALUE, string_strip, 0);
		ck_free(string);

		string = (char *) xv_get(fillin, PANEL_VALUE);

		set_history_file(p, string);

		if (p->hmd_file == NULL || *p->hmd_file == '\0') {
			/* STRING_EXTRACTION -
			 *
			 * If there is no entry yet in this menu,
			 * we print "entry" as a place holder
			 */
			xv_set(item,
				MENU_STRING, gettext("Entry"),
				MENU_INACTIVE, TRUE,
				0);
		} else {
			xv_set(item,
				MENU_STRING, p->hmd_file,
				MENU_INACTIVE, FALSE,
				0);
		}
		break;
	case MENU_DISPLAY_DONE:
	case MENU_NOTIFY_DONE:
		break;
	}

	return(item);
}


void
mt_props_dynamic_update_menus()
{
	History_menu_list	*p;
	char			*new_filemenu_string;
	struct header_data *hd;

	/* STRING_EXTRACTION -
	 *
	 * Since user has changed what is to be displayed
	 * as the menu items on the Move/Copy/Load menus
	 * (it was changed via the properties sheet),
	 * we now have to go ahead and update them
	 * (which could take a few seconds).
	 */

	/*
	 * Routine to dynamically update Move/Copy/Load Menus
	 */
	new_filemenu_string = mt_value("filemenu2");

	/* Make sure they both are not NULLs */
	if (!save_filemenu_string && !new_filemenu_string) 
		return;

	/* Make sure they both are strings, else
	 * automatically we need to update
	 */
	if (save_filemenu_string && new_filemenu_string) {
		if (strcoll(save_filemenu_string, new_filemenu_string) == NULL)
			return;
	}

	mt_busy(mt_frame, TRUE, gettext("Updating Move/Copy/Load menus..."), FALSE);
	for (p = mt_history_menu_list; p != NULL; p = p->hml_next) {
		(void) update_history_menu(p->hml_menu);
	}

	/* Save the new string for checking next time */
	if (save_filemenu_string) 
		free(save_filemenu_string);
	if (new_filemenu_string)
		save_filemenu_string = strdup(new_filemenu_string);
	else
		save_filemenu_string = NULL;
	mt_busy(mt_frame, FALSE, "", FALSE);
}

static void
update_history_menu(menu)

	Menu	menu;
{
	History_menu_data	*p;
	History_menu_data	*next_p;
	
        register int    n;
	Menu_item	menu_item;
	History_menu_data	*cache_menuitem_list;
  	Frame 		pinup_frame;
  	int 		pinned = 0;	
	int		stop_at;

	/*
	 * Dynamically update a menu.
	 * This used to be a menu_gen_proc, but it was too slow
	 * for visual feedback. Now it is called from Apply
	 * in property sheets.
	 */

 	pinup_frame = (Frame)xv_get(menu, MENU_PIN_WINDOW) ;
  	if (pinup_frame)
            pinned = (int)xv_get(pinup_frame, 
			      FRAME_CMD_PUSHPIN_IN) ;
  	if (pinned) {
            xv_set(pinup_frame, FRAME_CMD_PUSHPIN_IN, FALSE, 0);
            xv_set(pinup_frame, XV_SHOW, FALSE, 0);
	}

	/*
	 * Leave first menuitem alone, it has its own gen proc.
	 * If pinned, first item = 2, else = 1
	 */
	/*
	stop_at = (pinned) ? 2 : 1;
	*/
	stop_at = 2;

	/*
	 * Stop before first item
      	 * Remove all menuitems after first one, 
	 * saving the cached ones
	 */
	cache_menuitem_list = NULL;
	for (n = (int)xv_get(menu, MENU_NITEMS); n > stop_at; n--) {
		menu_item = xv_get(menu, MENU_NTH_ITEM, n);
	        p = (History_menu_data *)xv_get(menu_item, MENU_CLIENT_DATA);
	        if (p->hmd_age == -1) {
	            free(p);	
	        } else {
		    /* save cached items for later, since we start
		     * from bottom, we save in reverse order
		     */
		    if (cache_menuitem_list == NULL) {
		        p->hmd_next = NULL;
		    } else {
		        p->hmd_next = cache_menuitem_list;
		    }
		    cache_menuitem_list = p;
		}
		xv_set(menu, MENU_REMOVE_ITEM, menu_item, 0);
	        xv_destroy_safe(menu_item);
	}

	/* load in the new permanent menuitems */
	preload_file_menu(menu);

	/* load in the cached menuitems */
	for (p = cache_menuitem_list; p != NULL; p = next_p) {
	     next_p = p->hmd_next;
	     p->hmd_next = NULL;
	     menu_item = (Menu_item)xv_create(XV_NULL, MENUITEM,
	  	    MENU_STRING, p->hmd_file,
		    MENU_ACTION_PROC, history_menu_action_proc,
		    MENU_CLIENT_DATA, 	p,
		    XV_HELP_DATA,	"mailtool:RecentFiles",
		    0);
	     xv_set(menu, MENU_APPEND_ITEM, menu_item, 0);
     	}
        if (pinned) {
             xv_set(pinup_frame, XV_SHOW, TRUE, 0);
             xv_set(pinup_frame, FRAME_CMD_PUSHPIN_IN, TRUE, 0);
	}
}


Menu
mt_short_file_menu_create(hd, frame, op, title)

        struct header_data *hd;
	Frame	frame;
	int	op;
	char	*title;
{
	Menu	menu;
	Menu_item	menu_item;
	char	*token;
	char	*string;

	menu = (Menu)xv_create(XV_NULL, MENU,
		XV_KEY_DATA,	MT_OP_KEY, 	op,
		XV_KEY_DATA,	MT_FIELD_KEY, 	hd->hd_file_fillin,
		XV_KEY_DATA,	KEY_HEADER_DATA, hd,
		0);

	/*
	 * If it has a title, then it is pinnable.
	 */
	if (title != NULL) {
		xv_set(menu, MENU_GEN_PIN_WINDOW, frame, title, 0);
	}

	/*
	 * Add first item which will be the contents of the text field
	 */
	menu_item = add_to_history_menu(menu, gettext("Entry"), TRUE);
	xv_set(menu_item, MENU_GEN_PROC, fillin_menu_item_gen_proc, 0);

	/*
	 * Save contents of the filemenu .mailrc variable
	 * to check if changed later
	 */
	if (save_filemenu_string) free (save_filemenu_string);
	if (!(string = mt_value("filemenu2")))
		save_filemenu_string = NULL;
	else
		save_filemenu_string = strdup(string);
	preload_file_menu(menu);

	add_history_menu(&mt_history_menu_list, menu);

	return(menu);
}


Menu
mt_mail_file_menu_create(hd)

        struct header_data *hd;
{
	Menu_item	tmp_item;
	Menu		menu;
	char		*token;

	menu = xv_create(XV_NULL, MENU,
                        XV_KEY_DATA, KEY_HEADER_DATA, hd,
			MENU_GEN_PROC, mt_folder_menu_gen_proc,
			0);
	return(menu);
}

static
preload_file_menu(menu)

	Menu	menu;

{
	char	*buf, *token;
	int	nitems = 0;

	/*
	 * Add values specified in .mailrc 
	 */
	if ((token = mt_value("filemenu2")) != NULL)
	{
		/*
		 * Copy value into a buffer since strtok is destructive
		 */
		if ((buf = (char *)malloc(strlen(token) + 1)) == NULL)
			return(-1);
		strcpy(buf, token);
		if ((token = (char *) strtok(buf, " "))) {
			add_to_history_menu(menu, token, TRUE);
			nitems++;
			while (token = (char *) strtok(NULL, " ")) {
			   add_to_history_menu(menu, token, TRUE);
			   nitems++;
			}
		}
		free(buf);

		if (nitems > 0)
			(void) add_to_history_menu(menu, "", TRUE);
	}

	return(0);
}

static Menu_item
add_to_history_menu(menu, file, never_bump)

	Menu	menu;
	char	*file;	     /* File name to add to menu */
	short	never_bump;  /* TRUE if file should never be bumped from menu */

{
	register int	n;
	register Menu_item	menu_item, bump_item;
	static int	counter;
	unsigned	smallest;
	char	*string;
	int	max_menu_length;
	History_menu_data	*p;
	int	stop_at;

	MP(("add_to_history_menu(%x, %s, %d)\n", menu, file, never_bump));

	/*
	 * Add file to the menu of most used files.  If the menu is full
	 * bump off the least recently used file
	 */
	while (*file && (*file == ' '))
		file++;

	if (menu == NULL || file == NULL)
		return(0);
	if (strcmp(file, "%") == 0)
		return(0);

	/* The bigger the counter, the more recently the files been used */
	counter++;

	/*
	 * Make sure file name is not already in the menu and find the least
	 * recently used item. If the file is already in the menu,
	 * update it's age.
	 * We search from bottom up.
	 */
	smallest = ~0;
	bump_item = NULL;
	stop_at = (int)xv_get(menu, MENU_PIN) ? 2 : 1;
	for (n = (int)xv_get(menu, MENU_NITEMS); n > stop_at; n--) {
		menu_item = xv_get(menu, MENU_NTH_ITEM, n);
		p = (History_menu_data *)xv_get(menu_item, MENU_CLIENT_DATA);
		/* This field used for dynamic updating, to save the cached items */
		p->hmd_next = NULL; 

		if (strcoll(file, p->hmd_file)==0) {
			/* File is already in list */
			if (p->hmd_age >= 0)
				p->hmd_age = counter;
			return(menu_item);
		}
		/*
		 * Keep track of the least recently used item.
		 * never_bump items have a negative hmd_age.
		 * We never bump them.
		 */
		if (p->hmd_age > 0 && p->hmd_age <= smallest) {
			smallest = p->hmd_age;
			bump_item = menu_item;
		}
	}

	/*
	 * Get the max menu length.  We do this each time just in case
	 * the value has changed
	 */
	if ((string = mt_value("filemenusize")) && *string != '\0') {
		if ((max_menu_length = atoi(string)) <= 0)
			max_menu_length = DEFMAXFILES;
	} else {
		max_menu_length = DEFMAXFILES;
	}

	/*
	 * Check if we will overflow menu size. Don't count topmost item
	 * and white space after permanent entries.
	 * Don't count pin if menu has one.
	 */
	n = (int)xv_get(menu, MENU_NITEMS) - 2;
	if ((int)xv_get(menu, MENU_PIN))
		 n -= 1;
	if (n >= max_menu_length) {
		/*
		 * Too many items. Replace the least recently used member.
		 * Save its client data cuz we'll reuse it.
		 */
		if (bump_item == NULL)
			return(0);
		p = (History_menu_data *)xv_get(bump_item, MENU_CLIENT_DATA);
		xv_set(menu, MENU_REMOVE_ITEM, bump_item, 0);
	} else {
		p = (History_menu_data *) malloc(sizeof(History_menu_data));
		if (p == NULL) return(0);

		memset((char *) p, 0, sizeof(History_menu_data));
	}

	/*
	 * New entry into menu.  Initialize data and append to end of menu
	 */
	set_history_file(p, file);

	p->hmd_age = never_bump ? ~0 : counter;
	if (mt_value("sortfilemenu") && *p->hmd_file == '\0') {
		/* this is a blank item.  Make sure it is bumpable */
		p->hmd_age = 1;
	}

	menu_item = (Menu_item)xv_create(XV_NULL, MENUITEM,
			MENU_STRING, p->hmd_file,
			MENU_ACTION_PROC, history_menu_action_proc,
			MENU_CLIENT_DATA, 	p,
			XV_HELP_DATA,	"mailtool:RecentFiles",
			0);

	/*
	 * If it's the "white space" item it get's no feedback
	 */
	if (*p->hmd_file == '\0')
		xv_set(menu_item, MENU_FEEDBACK, FALSE, 0);


	if (mt_value("sortfilemenu")) {
		/* insert the menu alphabetically */
		n = (int)xv_get(menu, MENU_PIN) ? 3 : 2;
		stop_at = (int)xv_get(menu, MENU_NITEMS);

		MP(("add_to_history_menu: n %d, stop_at %d\n", n, stop_at));

		if (*p->hmd_file == '\0') {
			/* null insert -- just put it at the beginning */
			MP(("add_to_history_menu: null name insert\n"));
			xv_set(menu, MENU_INSERT, n -1, menu_item, 0);
		} else {
			char *s;
			Menu_item old_item;

			/* find the right place in the list to insert 
			 * alphabetically
			 */
			while (n <= stop_at) {

				old_item = xv_get(menu, MENU_NTH_ITEM, n);
				s = (char *) xv_get(old_item, MENU_STRING);

				if (*s != '\0' && strcoll(p->hmd_file, s) < 0)
				{
					break;
				}

				n++;
			}

			if (n > stop_at) {
				MP(("add_to_history_menu: appending\n"));
				xv_set(menu, MENU_APPEND_ITEM, menu_item, 0);
			} else {
				MP(("add_to_history_menu: insert before %s\n",
					s));
				xv_set(menu, MENU_INSERT, n-1, menu_item, 0);
			}

		}
	} else {
		/* just put the new one at the end */
		xv_set(menu, MENU_APPEND_ITEM, menu_item, 0);
	}
	return(menu_item);
}

Menu
mt_clear_menu(menu, free_item_data, free_menu_data)

	Menu	menu;
	int	free_item_data;
	int	free_menu_data;

{
	char	*data;
	Menu	pullright_menu;
	Menu_item	menu_item;
	register int	n;

	/*
	 * Loop through menu and remove and destroy all items.  If an item
	 * has a pullright, descend and destroy it.  Make sure to free any
	 * client data if asked to do so.
	 */
	for (n = (int)xv_get(menu, MENU_NITEMS); n > 0; n--) {
		menu_item = xv_get(menu, MENU_NTH_ITEM, n);
		if (pullright_menu = (Menu)xv_get(menu_item, MENU_PULLRIGHT)) {
			mt_clear_menu(pullright_menu, free_item_data,
							free_menu_data);
			xv_destroy_safe(pullright_menu);
		}

		if (free_item_data &&
		    (data = (char *)xv_get(menu_item, MENU_CLIENT_DATA))) {
			free(data);
		}
		xv_set(menu, MENU_REMOVE_ITEM, menu_item, 0);
		xv_destroy_safe(menu_item);
	}

	if (free_menu_data && (data = (char *)xv_get(menu, MENU_CLIENT_DATA))) {
		free(data);
		xv_set(menu, MENU_CLIENT_DATA, NULL, 0);
	}

	return(menu);
}

mt_menu_is_empty(menu)

	Menu	menu;
{
	return ((int)xv_get(menu, MENU_NITEMS) < 1);
}
