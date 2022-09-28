#ifndef lint
static 	char sccsid[] = "@(#)textedit_menu.c 1.2 93/01/06 Copyr 1990 Sun Micro";
#endif

#ifdef	MA_DEBUG
#define	DP	if(1) 
#else
#define	DP	if(0)
#endif

/*
 * Copyright (c) 1990 Sun Microsystems, Inc.
 *
 * these functions are for a workaround for an xview
 * problem where the window does not stay up afer a menu item is
 * selected from the panel/button/menus. (they work fine if you use
 * the same menus from the textsw).
 *
 */
#include <stdio.h>
#include <xview/xview.h>
#include <xview/textsw.h>

int             KEY_NOTIFY_PROC;
typedef	void	MF(Menu, Menu_item);

void
new_notify_proc(Menu menu, Menu_item menu_item)
{
	MF	*old_notify;

	old_notify = (MF *)xv_get(menu_item, XV_KEY_DATA, KEY_NOTIFY_PROC);

	if (old_notify)
	{
		old_notify(menu, menu_item);
	}

	(void) xv_set(menu, MENU_NOTIFY_STATUS, XV_ERROR, NULL);
}

attach_new_notify_proc(Menu menu)
{
	MF		*return_def;
	Menu            pullright;
	Menu_item       item;
	int             i;

	for (i = 1;; ++i)
	{
		item = xv_get(menu, MENU_NTH_ITEM, i);
		if (!item)
		{
			break;
		}

		if (pullright = (Menu) xv_get(item, MENU_PULLRIGHT))
		{
			attach_new_notify_proc(pullright);
		}
		else
		{
			return_def = (MF *)xv_get(item, MENU_NOTIFY_PROC);
			xv_set(item, MENU_NOTIFY_PROC, new_notify_proc,
			       XV_KEY_DATA, KEY_NOTIFY_PROC, return_def,
			       NULL);
		}
	}
}

workaround_init(Textsw textsw)
{
	int	i;
	Menu	file_panel_menu;
	Menu	display_panel_menu;
	Menu	edit_panel_menu;
	Menu	find_panel_menu;

	KEY_NOTIFY_PROC = xv_unique_key();

	file_panel_menu = (Menu) xv_get(textsw, TEXTSW_SUBMENU_FILE);
	display_panel_menu = (Menu) xv_get(textsw, TEXTSW_SUBMENU_VIEW);
	edit_panel_menu = (Menu) xv_get(textsw, TEXTSW_SUBMENU_EDIT);
	find_panel_menu = (Menu) xv_get(textsw, TEXTSW_SUBMENU_FIND);

	if (file_panel_menu)
	{
		attach_new_notify_proc(file_panel_menu);
	}
	if (display_panel_menu)
	{
		attach_new_notify_proc(display_panel_menu);
	}
	if (edit_panel_menu)
	{
		attach_new_notify_proc(edit_panel_menu);
	}
	if (find_panel_menu)
	{
		attach_new_notify_proc(find_panel_menu);
	}
}
