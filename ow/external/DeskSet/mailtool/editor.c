#ifndef lint
static 	char sccsid[] = "@(#)editor.c 3.3 92/12/03 Copyr 1990 Sun Micro";
#endif

/*
 * Copyright (c) 1990 by Sun Microsystems, Inc.
 */

/*	Copyright (c) 1991, Sun Microsystems, Inc.  All Rights Reserved.
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
 * Mailtool - Custom editor routines
 */

#ifdef EDITOR

#include <xview/xview.h>
#include <xview/panel.h>
#include <xview/text.h>
#include <xview/font.h>
#include <xview/notice.h>
#include <xview/tty.h>
#include <xview/termsw.h>
#include <X11/X.h>

#include "glob.h"
#include "tool.h"
#include "tool_support.h"
#include "mle.h"
#include "editor.h"

#define DEBUG_FLAG mt_debugging
extern int DEBUG_FLAG;
#include "debug.h"


/*
 * Find which compose window has the ttysw, and check to see if the ttysw
 * is currently in use
 */
static struct reply_panel_data *
get_editor_owner(rpd_list, editor_inuse_p )

	struct reply_panel_data	*rpd_list;
	int	*editor_inuse_p;

{
	/*
	 * Loop through all compose windows looking for one with
	 * the tty subwindow
	 */
	for (; rpd_list != NULL; rpd_list = rpd_list->next_ptr) {
		/* Check if editor is running in this window */
		if (rpd_list->rpd_ttysw)
			break;
	}
	 
	/*
	 * If we found the ttysw, then check to see if it is in use
	 */
	if (rpd_list != NULL && rpd_list->rpd_editor_inuse)
		*editor_inuse_p = TRUE;
	else
		*editor_inuse_p = FALSE;
		
	/* Return window which has ttysw */
	return(rpd_list);
}

#ifdef NEVER
	/* Leave this out until the UI debates settle down.  This code keeps
	 * The Edit menu button active and switches the Custom Editor menu
	 * item to Quit Editor so that the user has an obvious way of
	 * exiting the editor
	 */
static void
set_menu_inactive(menu, inactive)

	Menu	menu;
	int	inactive;

{
	int	n;
	Menu_item	mi;

	/* Set all items in a menu active or inactive */
	for (n = (int)xv_get(menu, MENU_NITEMS); n > 0; n--) {
		mi = (Menu_item)xv_get(menu, MENU_NTH_ITEM, n);
		(void)xv_set(mi, MENU_INACTIVE, inactive, 0);
	}

	return;
}


/*
 * Set the compose window active or inactive
 */
static void
set_compose_inactive(rpd, inactive)

	struct reply_panel_data	*rpd;
	int	inactive;

{
	Panel_item	item;
	Menu		menu;
	Menu_item	editor_mi;

	/* Set all buttons in compose panel active/inactive */
	PANEL_EACH_ITEM(rpd->reply_panel, item) {
		if ((Panel_item_type)xv_get(item, PANEL_ITEM_CLASS) != 
		     PANEL_TEXT_ITEM)
			(void)xv_set(item, PANEL_INACTIVE, inactive, 0);
	} PANEL_END_EACH;

	/* Get the edit menu */
	menu = (Menu)xv_get(rpd->edit_item, PANEL_ITEM_MENU);
	editor_mi = (Menu_item)xv_get(menu, MENU_NTH_ITEM, 2);

	/* Set the contents of the Edit menu active/inactive */
	set_menu_inactive(menu, inactive);

	if (inactive) {
		/* Leave the Edit menu button and editor menu item active */
		(void)xv_set(rpd->edit_item, PANEL_INACTIVE, FALSE, 0);
		(void)xv_set(editor_mi,
			MENU_INACTIVE, FALSE,
			MENU_STRING, "Quit Editor",
			0);
	} else {
		/* Reset custom editor menu item label */
		(void)xv_set(editor_mi,
			MENU_INACTIVE, FALSE,
			MENU_STRING, "Custom Editor",
			0);
	}
	return;
}
#endif

/*
 * Set the compose window active or inactive
 */
static void
set_compose_inactive(rpd, inactive)

	struct reply_panel_data	*rpd;
	int	inactive;

{
	Panel_item	item;
	Menu		menu;
	Menu_item	editor_mi;

	/* Set all buttons in compose panel active/inactive */
	PANEL_EACH_ITEM(rpd->reply_panel, item) {
		if ((Panel_item_type)xv_get(item, PANEL_ITEM_CLASS) != 
		     PANEL_TEXT_ITEM)
			(void)xv_set(item, PANEL_INACTIVE, inactive, 0);
	} PANEL_END_EACH;
	return;
}


/*
 * When the process (editor) in the ttysw exits, this routine is called
 */
static Notify_value
editor_done_proc(ttysw, pid, status, rusage)

	Tty	ttysw;
	int	pid;
	int	*status;
	struct rusage	*rusage;

{
	struct reply_panel_data	*rpd;

#ifdef DEBUG
	DP(("** Editor Notify wait3 proc called: "));
	if (WIFEXITED(*status)) {
		DP(("WIFEXITIED "));
		DP(("WEXITSTATUS(%d) ", WEXITSTATUS(*status)));
	}
	if (WIFSTOPPED(*status)) {
		DP(("WIFSTOPPED "));
		DP(("WSTOPSIG(%d) ", WSTOPSIG(*status)));
	}
	if (WIFSIGNALED(*status)) {
		DP(("WIFSIGNALED "));
		DP(("WTERMSIG(%d) ", WTERMSIG(*status)));
	}
	printf("\n");
#endif

	if (WIFEXITED(*status)) {
		/* The editor has exited */
		rpd = (struct reply_panel_data *)xv_get(ttysw, WIN_CLIENT_DATA);
		/* Load data from editor temp file */
		if (rpd->rpd_editor_file) {
			textsw_load_file(rpd->replysw, rpd->rpd_editor_file,
					 1, 0, 0);
		}
		/* Return compose window back to normal state */
		mt_stop_editor(rpd);
	}

	return(NOTIFY_DONE);
}


/*
 * Start an editor session in the compose window specified by rpd.
 * Returns FALSE if the edit could not be started.
 */
int
mt_start_editor(rpd)

	struct reply_panel_data	*rpd;

{
	struct reply_panel_data	*editor_owner;
	int	width, height, x, y;
	char	*argv[20];
	char	*editor;
	char	buf[128];
	int	child_pid;
	int	editor_inuse;
	char	*shell;

	/*
	 * Find which compose window has the ttysw and if it is in use
	 * or not.  We can only have one ttysw per process.  That's why
	 * we have to do all this.
	 */
	editor_owner = get_editor_owner(MT_RPD_LIST(rpd->hd),
						&editor_inuse);

	if (editor_inuse) {
		/* STRING_EXTRACTION -
		 * The user had a custom editor running in one compose window
		 * and tried to start one in another compose window
		 */
		mt_vs_warn(rpd->frame,
		gettext("Unable to start editor.\nYou are running an editor in another compose window.\nPlease exit that editor before starting this one"));
		return(FALSE);
	}
	
	/* Get editor to use */
	if ((editor = mt_value("VISUAL")) == NULL || *editor == '\0') {
		editor = MT_DEFAULT_EDITOR;
	}

	/* Get shell to use */
	if ((shell = getenv("SHELL")) == NULL)
		shell = MT_DEFAULT_SHELL;

	if (rpd->rpd_editor_file != NULL)
		ck_free(rpd->rpd_editor_file);

	/* Generate a temp file name */
	rpd->rpd_editor_file = tempnam(MT_DEFAULT_TMP, "MTe");

	if (rpd->rpd_editor_file == NULL) {
		/* STRING_EXTRACTION -
		 * Could not generate a temp file using tempnam(3) for use
		 * with the compose window custom editor
		 */
		mt_frame_msg(rpd->frame, TRUE,
			gettext("Could not generate temp file"));
		return(FALSE);
	}

	sprintf(buf, "%.100s %.20s", editor, rpd->rpd_editor_file);
	argv[0] = shell;
	argv[1] = "-c";
	argv[2] = buf;
	argv[3] = NULL;

	if (textsw_store_file(rpd->replysw, rpd->rpd_editor_file, 0, 0) != 0) {
		/* Could not save out file */
		mt_vs_warn(rpd->frame,
		gettext("Unable to save data to temp file. Cannot start editor"));
		return(FALSE);
	}

	/* Display the command on the footer */
	mt_frame_msg(rpd->frame, FALSE, buf);

	/* Get dimesnions of textsw so we can lay ttysw over it */
	width =	(int)xv_get(rpd->replysw, XV_WIDTH);
	height = (int)xv_get(rpd->replysw, XV_HEIGHT);
	x = (int)xv_get(rpd->replysw, XV_X);
	y = (int)xv_get(rpd->replysw, XV_Y);

	/* make items in compose command panel inactive */
	set_compose_inactive(rpd, TRUE);

	if (editor_owner != rpd) {
		if (editor_owner != NULL) {
			/*
			 * Another compose window owns the ttysw but is not
			 * using it Nuke it
			 */
			(void)xv_destroy_safe(editor_owner->rpd_ttysw);
			editor_owner->rpd_ttysw = NULL;
		}

		/*
		 * Create new ttysw
	 	 * Would like this to be a generic TTY window, but those
		 * are way too flakey (crash when vi echos terminal
		 * initialization sequence)
	 	 */
		rpd->rpd_ttysw = (Tty)xv_create(rpd->frame, TERMSW,
				TTY_ARGV, TTY_ARGV_DO_NOT_FORK,
				0);
	}

	/* This compose window has the editor */
	rpd->rpd_editor_inuse = TRUE;

	/* Initialize ttysw */
	(void)xv_set(rpd->rpd_ttysw,
		XV_X,		x,
		XV_Y,		y,
		XV_WIDTH,	width,
		XV_HEIGHT,	height,
		XV_SHOW,	TRUE,
		WIN_CLIENT_DATA,	rpd,
		TTY_ARGV,	argv,
		0);

	/*
	 * Switch to TTY mode to turn off scrolling and force the focus
	 * into the window
	 */
	xv_set(rpd->rpd_ttysw,
		TERMSW_MODE, TTYSW_MODE_TYPE,
		WIN_SET_FOCUS,
		0);

	(void)xv_set(rpd->replysw, XV_SHOW, FALSE, 0);

	/* Get notified when editor exits */
	child_pid = (int)xv_get(rpd->rpd_ttysw, TTY_PID);
	notify_set_wait3_func(rpd->rpd_ttysw, editor_done_proc, child_pid);


	/* STRING_EXTRACTION -
	 * The user has entered the customer editor in the compose window.
	 * Before delivering the message they must exit the editor.
	 */
	mt_frame_msg(rpd->frame, FALSE,
			gettext("Exit editor to activate compose window"));

	return(TRUE);
}

/*
 * Terminate the editing session, set the compose window back to normal
 * and clean up the temp file
 */
void
mt_stop_editor(rpd)

	struct reply_panel_data	*rpd;

{
	int	child_pid;

	/* Make all buttons active */
	set_compose_inactive(rpd, FALSE);

	/* Make sure the process is dead */
	child_pid = (int)xv_get(rpd->rpd_ttysw, TTY_PID);

	if (child_pid > 1)
		kill(child_pid, SIGTERM);

	/* Hide tty window and show textsw */
	(void)xv_set(rpd->rpd_ttysw,
		XV_SHOW, FALSE,
		TTY_ARGV, TTY_ARGV_DO_NOT_FORK,
		0);

	(void)xv_set(rpd->replysw,
		XV_SHOW, TRUE,
		WIN_SET_FOCUS,
		0);

	rpd->rpd_editor_inuse = FALSE;

	/* Remove the temp file and free storage */

	if (rpd->rpd_editor_file) {
		unlink(rpd->rpd_editor_file);
		ck_free(rpd->rpd_editor_file);
		rpd->rpd_editor_file = NULL;
	}

	(void)xv_set(rpd->frame, FRAME_LEFT_FOOTER, "", 0);
}

#endif /* EDITOR */
