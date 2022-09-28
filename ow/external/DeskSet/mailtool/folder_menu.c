#ifndef lint
#ifdef sccs
static  char sccsid[] = "@(#)folder_menu.c 3.10 92/12/14 Copyr 1987 Sun Micro";
#endif
#endif

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

/*
 * Mailtool - building folder menu and processing its events
 */

#include <stdio.h>
#include <errno.h>
#include <signal.h>
#include <ctype.h>
#include <math.h>
#include <xview/window_hs.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/param.h>
#ifdef SVR4
#include <sys/kbd.h>
#include <sys/kbio.h>
#else
#include <sundev/kbd.h>
#endif SVR4

#include <xview/cursor.h>
#include <xview/selection.h>
#include <xview/sel_svc.h>
#include <xview/sel_attrs.h>
#include <xview/panel.h>
#include <xview/text.h>
#include <xview/font.h>
#include <xview/notice.h>
#include <xview/svrimage.h>
#include <xview/fullscreen.h>
#include <xview/xview.h>
#include <X11/X.h>

#include "glob.h"
#include "tool.h"
#include "tool_support.h"
#include "mail.h"
#include "cmds.h"
#include "mle.h"

#define DEBUG_FLAG mt_debugging
#include "debug.h"

char    *strcpy();

struct Menu_client_data {
	char           *fdir_name;	/* name of dir */
	time_t          fdir_time;	/* time fdir last modified */
	char           *path_string;	/* the concatenation of the items
					 * leading to this menu */
	char	       *strtab;		/* string table for pullright menu */
};

static Menu 	mt_folder_menu_build();
extern	char	*get_fdir_name();
static caddr_t  pullright_action_proc();

/* used Menu on frame menu over icon */
Menu
mt_folder_menu_gen_proc(m, operation)
     Menu       m;
     Menu_generate   operation;
{
	/*
	 * Generate the folder menu
	 */
	switch (operation) {
	case MENU_DISPLAY:
  		mt_get_folder_menu(m, "", "");
		break;
	case MENU_DISPLAY_DONE:
	case MENU_NOTIFY:
	case MENU_NOTIFY_DONE:
		break;
	}

  	return (m);
}


/*
 * Returns a folder menu, building it if necessary
 */
Menu
mt_get_folder_menu(m, path_string, item_string)
	Menu            m;
	char           *path_string, *item_string;
{
	struct Menu_client_data *menu_data;
	struct stat     statb;
	char	*s;
	int	len;

	if (!mt_menu_is_empty(m)) {
	   menu_data  = (struct Menu_client_data *) (
			xv_get(m, MENU_CLIENT_DATA));

	   /* See if folder path has changed,
	    * if it did, clear out menu, and start over
	    */
	   s = get_fdir_name("");	
	   if (strcmp(s, menu_data->fdir_name) == 0) {
		if (stat(menu_data->fdir_name, &statb) < 0 ||
			statb.st_mtime == menu_data->fdir_time) 
		{
			return (m);
		}
		/*
		 * Menu out of date.  We must rebuild it.  Because of the
		 * way XView menu gen procs work we don't destroy the menu, we
		 * just destroy all items in the menu and reuse it.
		 */
		menu_data->fdir_time = statb.st_mtime;
		mt_clear_menu(m, TRUE, FALSE);
		mt_folder_menu_build(m, menu_data);
		return (m);
	   } else {
	    /* folder did change, so clear out menu, and start over*/
		mt_clear_menu(m, TRUE, TRUE);
	   }
	} 
	/* If we get here, either this is first time and there is no
	 * menu, or the folder path has changed and we have to start
	 * from scratch
	 */
	menu_data = (struct Menu_client_data *)(
		malloc(sizeof(struct Menu_client_data)));
	s = (char *)(malloc(strlen(path_string)+strlen(item_string)+1));

	menu_data->strtab = NULL;

	(void)strcpy(s, path_string);
	(void)strcat(s, item_string);
	len = strlen(s);
	if (len > 0 && s[len - 1] == '@') /* symbolic link */
		s[len - 1] = '\0';
	menu_data->path_string = s;
	s = get_fdir_name(s);	/* new storage */
	menu_data->fdir_name = s;

	if (stat(s, &statb) >= 0) {
		menu_data->fdir_time = statb.st_mtime;
	} else {
		menu_data->fdir_time = 0;
	}

	mt_folder_menu_build(m, menu_data);
	return (m);
}


static Menu
pullright_gen_proc(mi, operation)
	Menu_item       mi;
	Menu_generate   operation;
{
	Menu            pull_right, parent;
	struct Menu_client_data *menu_data;
	struct header_data      *hd;

	parent = (Menu) (xv_get(mi, MENU_PARENT));
	/* Make child menu also inherit hd */
        hd = mt_get_header_data(parent);

	if ((pull_right = (Menu)xv_get(mi, MENU_CLIENT_DATA)) == NULL) {
		pull_right = xv_create(XV_NULL, MENU, 
                	XV_KEY_DATA, KEY_HEADER_DATA, hd,
			0);
	}

	switch (operation) {
	case MENU_DISPLAY:
		/* menu_data for parent menu */
		menu_data = (struct Menu_client_data *)xv_get(
			parent, MENU_CLIENT_DATA);

		/* mt_get_folder_menu will build menu first
		 * time, and then return it subsequent times
		 * unless directory has been more recently
		 * modified
		 */
		pull_right = mt_get_folder_menu(
			pull_right,
			menu_data->path_string,
			(char *)xv_get(mi, MENU_STRING));
		break;

	case MENU_DISPLAY_DONE:
	case MENU_NOTIFY:
	case MENU_NOTIFY_DONE:
		break;
	}
	return (pull_right);
}




/* ARGSUSED */
static caddr_t
pullright_action_proc(m, mi)
	Menu            m;
	Menu_item       mi;
{

	char	s[MAXPATHLEN+1];
	struct Menu_client_data	*menu_data;
	int		len;
	struct header_data      *hd;

        hd = mt_get_header_data(m);
	*s = '\0';
	menu_data = (struct Menu_client_data *)xv_get(
		m, MENU_CLIENT_DATA);
	(void)strcpy(s, menu_data->path_string);
	(void)strcat(s, (char *)xv_get(mi, MENU_STRING));
	/*
	 * value is result of concatting the string(s) from
	 * menus to the left, stored in path_string field of
	 * MENU_CLIENT_DATA, with that of this item's string. 
	 */
	len = strlen(s);
	if (s[len - 1] == '@') /* symbolic link */
		s[len - 1] = '\0';
	(void) panel_set_value(hd->hd_file_fillin, s);

#ifdef NOTUSED
	/* this is old SUNVIEW code; this can't happen with
	 * a window manager any more...
	 */
	if (mt_idle) {
		/*
		 * selection in folder pullright came from frame menu open
		 * and switch to specified folder 
		 */
		mt_load_from_folder = s;
		xv_set(MT_FRAME(hd), FRAME_CLOSED, FALSE, 0);
	}
#endif NOTUSED
}

static Menu
mt_folder_menu_build(m, menu_data)
	Menu	m;
	struct Menu_client_data *menu_data;
{
	register int    i;
	int             ac;
	char          **av;
	char          *av_tmp;
	char          **mt_get_folder_list();
	Menu_item	mi;
	char	      **sptr;
	int		width;
	int		maxwidth;
	double		ncols;
	void		*attr_array[ATTR_STANDARD_SIZE];
	void		**attr_ptr;
	void		**attr_end;
	struct header_data *hd;

	hd = (struct header_data *) xv_get(m, 
		XV_KEY_DATA, KEY_HEADER_DATA);
	mt_busy(MT_FRAME(hd), TRUE, NULL, FALSE);

	/* free up the old string allocation */
	ck_free(menu_data->strtab);

	av = mt_get_folder_list(menu_data->path_string, menu_data->fdir_name,
		&menu_data->strtab, &ac, FALSE);
	/*
	 * Select aspect ratio for menu.
	 * This is essentially a table lookup to
	 * avoid taking a sqrt.
	 */
	if  (ac == 0) {
                /* STRING_EXTRACTION -
                 *
                 * Check first directory is readable/open-able.
		 *
                 * Then, if your folder directory is empty, or a folder subdirectory
                 * is empty, this is the name that will be put in the menu
                 * in lieu of any real entries.
                 */
		if (!av) {
			av_tmp = gettext("Cannot read directory!");
			av = &av_tmp;
		} else {
			av[0] = gettext("Empty Directory!");
		}
		ac = 1;
	}

	/* figure out how large the largest entry is; we will use this to
	 * adjust the height of the popup menu
	 *
	 * this doesn't work for I18N level 4 stuff (since a char may take
	 * more or less than one space on the screen), but it works
	 * well enough to get roughly square screens.
	 * 
	 */
	
	maxwidth = 0;
	attr_ptr = attr_array;
	attr_end = &attr_array[ATTR_STANDARD_SIZE - 20];
	for (i = 0, sptr = av; i < ac; i++, sptr++) {
		char *s = *sptr;

		width = strlen(s);
		maxwidth = MAX(maxwidth, width);


		*attr_ptr++ = (void *) MENU_ITEM;


		*attr_ptr++ = (void *) MENU_STRING;
		*attr_ptr++ = (void *) s;
		*attr_ptr++ = (void *) MENU_ACTION_PROC;
		*attr_ptr++ = (void *) pullright_action_proc;
		*attr_ptr++ = (void *) XV_KEY_DATA;
		*attr_ptr++ = (void *) XV_HELP;
		*attr_ptr++ = (void *) "mailtool:FolderMenu";

		if (s[width - 1] == '/' || 
			(s[width - 1] == '@' && s[width - 2] == '/')) 
		{
			*attr_ptr++ = (void *) MENU_GEN_PULLRIGHT;
			*attr_ptr++ = (void *) pullright_gen_proc;
		}

		/* terminate the MENU_ITEM */
		*attr_ptr++ = (void *) 0;

		if (attr_ptr >= attr_end || i >= ac -1) {
			/* terminate the ATTR_LIST */
			*attr_ptr = 0;
			xv_set(m, ATTR_LIST, attr_array, 0);
			attr_ptr = attr_array;
		}
	}

	/* this is not strictly true, but for the sake of argument
	 * assume that a column is twice as high as it is wide.  Try
	 * to get somewhere close to a square.
	 *
	 * the algorithm is: (2 * nelements)/ncols == width * ncols
	 *
	 * solving for ncols: ncols = sqrt(2 * nelements/width)
	 */

	ncols = 2 * ac;
	ncols /= maxwidth;
	ncols = sqrt(ncols) + 0.5;	/* round off? */

	DP(("mt_folder_menu_build: ncols %f\n", ncols));

	xv_set(m,
	      MENU_NCOLS, (int) ncols,
	      MENU_CLIENT_DATA, menu_data,
	      0);

	ck_free(&av[-1]);

	mt_busy(MT_FRAME(hd), FALSE, NULL, FALSE);
	return (m);
}

/*
 * generates full path name for folder (sub)directory. returns new storage.
 * path_string is the result of concatenating the items in the pullright
 * folder menu. It is always relative to the folders directory, if any. 
 */
static char *
get_fdir_name(path_string)
	char           *path_string;
{
	char            line[MAXPATHLEN];

	mt_expand_foldername(path_string, line);
	return (mt_savestr(line));
}

