#if !defined(lint) && defined(sccs)
static char sccsid[] = "@(#)vacation_ifc.c 3.9 94/09/13 Copyr 1990 Sun Micro";
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
#include <string.h>
#include <xview/window_hs.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/file.h>
#ifdef SVR4 
#include <unistd.h> 
#include <sys/kbd.h> 
#include <sys/kbio.h>
#else 
#include <sundev/kbd.h>
#endif SVR4
#include <xview/panel.h>
#include <xview/text.h>
#include <xview/font.h>
#include <xview/xview.h>

#include "ds_popup.h"
#include "glob.h"
#include "tool.h"
#include "tool_support.h"
#include "mle.h"
#include "instrument.h"

#include "../maillib/global.h"

#define FORWARDFILE	".forward"
#define BACKUPSUFFIX	"..BACKUP"

static Menu     mt_vacation_menu;	

static Frame		vacation_frame;
static Frame		vacation_panel;
static Textsw		vacation_textsw;
static Panel_item	vacation_button;
static void		vacation_not_start();

mt_update_vacation_data(panel_item, event)

Panel_item	panel_item;
Event		*event;

{
	char		forwardfile[256];
	char		messagefile[256];
	char		buf[256];
	FILE		*fwdfile;
	FILE		*bkupfile;
	Rect		*win_rect;
	Rect		*item_rect;
	int		fsize;
	int		answer;
	int		forwarding;
	int		lastchar;
	caddr_t		fwdptr;

	fwdfile = NULL;
	answer = FALSE;
	forwarding = FALSE;

	sprintf(forwardfile, "%s/%s", getenv("HOME"), FORWARDFILE);

	if (access(forwardfile, F_OK) == 0 && !mt_vacation_running()) {

                /* STRING_EXTRACTION -
                 *
                 * This confirmation window is brought up when the user
                 * tries to update the vacation status when the user is
                 * already using a .forward file.
                 */
		answer = mt_vs_confirm(vacation_frame, FALSE,
			gettext("Start Vacation"), gettext("Cancel"),
			gettext(
"You are already using the forwarding facility for\n\
something other than Vacation.  While Vacation is\n\
running, Vacation will be appended to this other\n\
forwarding activity. Is it still OK to start Vacation?"));

		if (!answer)
			return;

		if (mt_backup_forwardfile(forwardfile) < 0)
			return;

		/* A .forward file is currently in use. Merge vacation
		 * into this file, rather than overwrite it. To do so,
		 * set the appropriate variable to indicate mode.
		 */

		forwarding = TRUE;
	}

	sprintf(messagefile, "%s/.vacation.msg", getenv("HOME"));

	if ((int) xv_get(vacation_textsw, TEXTSW_MODIFIED) &&
	    (textsw_store_file(vacation_textsw, messagefile, 0, 0) != 0)) {

		vacation_not_start (vacation_frame, messagefile, errno);

		/* restore the original .forward file */
		if (answer)
			mt_recover_forwardfile(forwardfile);
		return;
	}

	/* since textsw always saves the original file (either from
	 * textsw_store_file or save from textsw menu), remove it
	 */
	strcat(messagefile, "%");
	(void) unlink(messagefile);

	if ((fwdfile = fopen(forwardfile, "w+")) == NULL) {
		vacation_not_start (vacation_frame, forwardfile, errno);

		/* restore the original .forward file */
		if (answer)
			mt_recover_forwardfile(forwardfile);
		return;
	}
	fchmod(fileno(fwdfile), 0644);

	strcpy(buf, forwardfile);
	strcat(buf, BACKUPSUFFIX);

	if (forwarding) {

		/* CREATE NEW .forward FILE
		 *
		 * The original .forward file has been renamed to the
		 * backup file name. We need to open the backup .forward
		 * file so we can copy from it.
		 */

		if ((bkupfile=fopen(buf,"r")) == NULL) {
			vacation_not_start (vacation_frame, bkupfile,
				errno);

			/* restore the original .forward file */
			if (answer)
				mt_recover_forwardfile(forwardfile);
				return;
			}

		/* COPY OLD .forward TO NEW .forward
		 *
		 * Using mmap is quite fast, so rather than do a while
		 * loop to copy line by line, we'll use mmap followed by
		 * a write.
		 */
	
		fsize=lseek(fileno(bkupfile), 0, SEEK_END);
		fwdptr=mmap(0, fsize, PROT_READ, MAP_PRIVATE,
				fileno(bkupfile), 0);
		write(fileno(fwdfile), fwdptr, fsize);
	
		/* RELEASE .forward FILE
		 *
		 * Un-mmap the new .forward file
		 */

		lastchar = fwdptr[fsize-1];
		munmap(fwdptr, fsize);

		/* APPEND VACATION LINE
		 *
		 * The new .forward file is still open, so append the
		 * new line below as the last line of the .forward file.
		 * Check to make sure last character in the file is a
		 * newline. If it's not, add one so our work goes on
		 * a separate line.
		 */

		if (lastchar != '\n') {
			fseek(fwdfile, 0, SEEK_END);
			fprintf(fwdfile, "\n");
		}

		/*
		 * Now, add the vacation line to the next line.
		 */

#ifdef SVR4
		fprintf(fwdfile, "\"|/usr/bin/vacation %s\"\n", glob.g_myname);
#else
		fprintf(fwdfile, "\"|/usr/ucb/vacation %s\"\n", glob.g_myname);
#endif SVR4
		}
	else {

		/* Create known backup file. The known backup
		 * file allows mailtool to differentiate between
		 * vacation being started from mailtool, and vacation
		 * being invoked (the Unix program as opposed to the
		 * MailTool Vacation menu item) via tty session.
		 */

		if ((bkupfile=fopen(buf,"a")) == NULL) {
			vacation_not_start (vacation_frame, bkupfile,
				errno);

			/* restore the original .forward file */
			if (answer)
				mt_recover_forwardfile(forwardfile);
				return;
			}

		fprintf(bkupfile, "User not using forward file\n");

		/* WRITE NEW .forward FILE
		 *
		 * There was no .forward file, so no appending
		 * must be done. Simply write the standard
		 * vacation line into the new .forward file.
		 */
#ifdef SVR4
		fprintf(fwdfile, "\\%s, \"|/usr/bin/vacation %s\"\n",
			glob.g_myname, glob.g_myname);
#else
		fprintf(fwdfile, "\\%s, \"|/usr/ucb/vacation %s\"\n",
			glob.g_myname, glob.g_myname);
#endif SVR4
		}

	fclose(bkupfile);
	fclose(fwdfile);

#ifdef SVR4
	system("/usr/bin/vacation -I");
#else
	system("/usr/ucb/vacation -I");
#endif SVR4

        /* STRING_EXTRACTION -
         *
         * "Change" is the action button at the bottom of the panel
         */
	xv_set(vacation_button,
		PANEL_LABEL_STRING, gettext("Change"),
		XV_HELP_DATA, "mailtool:VacationPopupChange",
		0);

	win_rect = (Rect *) xv_get(vacation_panel, WIN_RECT);
	item_rect = (Rect *) xv_get(vacation_button, PANEL_ITEM_RECT);

	xv_set(vacation_button, XV_X,
		win_rect->r_width/2 - item_rect->r_width/2, 0);

	mt_vacation_label(TRUE);
}

mt_create_vacation_frame()

{
	Notify_value	vacation_event_proc();
	int		vacation_done_proc();

	vacation_frame = xv_create(mt_frame, FRAME_CMD, 
			FRAME_SHOW_RESIZE_CORNER, TRUE,
			FRAME_DONE_PROC, vacation_done_proc,
			0);

	(void)notify_interpose_event_func(vacation_frame,
			vacation_event_proc, NOTIFY_SAFE);

        /* STRING_EXTRACTION -
         *
         * Vacation Setup is the title of the vacation frame.
         */
	mt_label_frame(vacation_frame, gettext("Vacation Setup"));

	vacation_textsw = xv_create(vacation_frame, TEXTSW, 
				WIN_ROWS, 	15, 
				WIN_X,		0,
				WIN_Y,		0,
				0);
	vacation_panel = xv_get(vacation_frame, FRAME_CMD_PANEL); 

	xv_set(vacation_panel,
		WIN_ROWS, 	2, 
		WIN_COLUMNS,	50,
		WIN_X,		0,
		WIN_BELOW,	vacation_textsw,
		0);

	xv_set(vacation_textsw,
		WIN_WIDTH, 	xv_get(vacation_panel, WIN_WIDTH), 
		0);

        /* STRING_EXTRACTION -
         *
         * "Start" is the action button at the bottom of the frame where
         * you compose the vacation message.
         */
	vacation_button = xv_create(vacation_panel, PANEL_BUTTON,
				PANEL_LABEL_STRING, gettext("Start"),
				PANEL_NOTIFY_PROC, mt_update_vacation_data,
				XV_Y,		xv_row(vacation_panel, 0),
				XV_HELP_DATA, "mailtool:VacationPopupStart",
				0);

	(void)xv_set(vacation_panel, PANEL_DEFAULT_ITEM, vacation_button, 0);
	ds_center_items(vacation_panel, 0, vacation_button, 0);

	window_fit(vacation_frame);
}

int
mt_start_vacation_proc(menu, menu_item)

Menu		menu;
Menu_item	menu_item;

{
	char		messagefile[256];
	int		vacation_running;
	Rect		*win_rect;
	Rect		*item_rect;
	FILE		*d_ptr;

	sprintf(messagefile, "%s/.vacation.msg", getenv("HOME"));

	vacation_running = mt_vacation_running();

	TRACK_BUTTON(menu, menu_item, "start_vacation");

	if (!vacation_frame) {
		mt_create_vacation_frame();
		ds_position_popup(mt_frame, vacation_frame, DS_POPUP_LOR);
	}

	if (vacation_running)
	{
		xv_set(vacation_button,
			PANEL_LABEL_STRING, gettext("Change"),
			XV_HELP_DATA,   "mailtool:VacationPopupChange",
			0);
	}
	else
	{
                /* STRING_EXTRACTION -       
                 *                           
                 * "Start" is the button label at the bottom that will enable
                 * a vacation.               
                 */
		xv_set(vacation_button,
			PANEL_LABEL_STRING, gettext("Start"),
			XV_HELP_DATA,   "mailtool:VacationPopupStart",
			0);
	}

	if (access(messagefile, F_OK) != 0)
	{
		if ((d_ptr = fopen(messagefile, "w")) == NULL)
		{
			vacation_not_start (vacation_frame, messagefile, errno);
			return (-1);
		}
		fchmod (fileno(d_ptr), 0644);

                /* STRING_EXTRACTION -
		 *
		 * This string is the text of the vacation message that
		 * will be returned when mail is received and vacation
		 * is turned on.  This will form the actual text
		 * of a returned message.
		 */
		fprintf(d_ptr, gettext(
"Subject:  I am on vacation\n\
Precedence: junk\n\
\n\
Your mail regarding \"$SUBJECT\" will be read when I return.\n\
\n\
If you have something urgent, please contact...\n"));

		fclose(d_ptr);
	}

	xv_set(vacation_textsw, TEXTSW_FILE, messagefile, 0);

	win_rect = (Rect *) xv_get(vacation_panel, WIN_RECT);
	item_rect = (Rect *) xv_get(vacation_button, PANEL_ITEM_RECT);

	xv_set(vacation_button, XV_X,
		win_rect->r_width/2 - item_rect->r_width/2, 0);


	xv_set(vacation_frame, XV_SHOW, TRUE, 0);
	return (0);
}

int
mt_stop_vacation_proc(menu, menu_item)

Menu		menu;
Menu_item	menu_item;

{
	char		forwardfile[256];

	TRACK_BUTTON(menu, menu_item, "stop_vacation");

	sprintf(forwardfile, "%s/%s", getenv("HOME"), FORWARDFILE);
	unlink(forwardfile);
	mt_recover_forwardfile(forwardfile);

	mt_vacation_label(FALSE);

}

Menu
mt_gen_vac_menu(m, operation)

Menu		m;
Menu_generate	operation;

{
	int		i;
	Menu_item	start_item;
	Menu_item	stop_item;
	int		vacation_running;

	vacation_running = mt_vacation_running();

	switch (operation) {

	case MENU_DISPLAY:
		TRACK_BUTTON(m, 0, "vacation_menu");

		mt_vacation_menu = xv_create(XV_NULL, MENU, 0);
		
		/* STRING_EXTRACTION -
		 *
		 * Start/Change and Stop are menu labels.  Start/change
		 * starts up the vacation edit menu.  Stop disables
		 * automatice replies to received mail.
		 */
		start_item = xv_create(XV_NULL, MENUITEM,
			MENU_STRING,	gettext("Start/Change..."),
			MENU_ACTION_PROC, mt_start_vacation_proc,
			XV_HELP_DATA,   "mailtool:VacationStart",
			0);
	
		stop_item = xv_create(XV_NULL, MENUITEM,
			MENU_STRING,	gettext("Stop"),
			MENU_INACTIVE, !vacation_running,
			MENU_ACTION_PROC, mt_stop_vacation_proc,
			XV_HELP_DATA,   "mailtool:VacationStop",
			0);

		if (!vacation_running)
		{
			xv_set(mt_vacation_menu,
				MENU_APPEND_ITEM, start_item,
				0);

			xv_set(mt_vacation_menu,
				MENU_APPEND_ITEM, stop_item,
				0);
		}
		else
		{

			xv_set(mt_vacation_menu,
				MENU_APPEND_ITEM, stop_item,
				0);

			xv_set(mt_vacation_menu,
				MENU_APPEND_ITEM, start_item,
				0);
		}

		break;

	}

	return(mt_vacation_menu);
}

mt_vacation_label(on)

	int	on;

{
	char	label_buf[1024];
	char	*p, *label;
	char	already_on;
	char	*vacation_text;

        /* STRING_EXTRACTION -
         *
         * If you are running the vacation program, "[Vacation]" is displayed
         * at the end of your frame title.
         */
        vacation_text = gettext("[Vacation]");

	label = (char *)xv_get(mt_frame, FRAME_LABEL);

	if ((p = strrchr(label, '[')) == NULL || strcmp(p, vacation_text) != 0)
		already_on = FALSE;
	else
		already_on = TRUE;

	if (on && already_on || !on && !already_on)
		return;

	if (on) {
		strcpy(label_buf, label);
		strcat(label_buf, " ");
		strcat(label_buf, vacation_text);
	} else {
		*p = '\0';
		strcpy(label_buf, label);
	}

	xv_set(mt_frame, FRAME_LABEL, label_buf, 0);
}

static void
vacation_resize(frame)

	Frame	frame;

{
	int	n;

	/*
	 * The frame has been re-sized.  Layout the pop-up.  If we didn't
	 * do this then the bottom panel would change in size instead of
	 * the textsw
	 */
	(void)xv_set(vacation_panel, WIN_ROWS, 1, 0);

	n = (int)xv_get(frame, XV_HEIGHT) -
		 2*(int)xv_get(frame, XV_TOP_MARGIN) -
		 (int)xv_get(vacation_panel, XV_HEIGHT);

	xv_set(vacation_textsw, XV_HEIGHT, n, 0);
	xv_set(vacation_panel, WIN_BELOW, vacation_textsw, 0);

	ds_center_items(vacation_panel, 0, vacation_button, 0);
}

static Notify_value
vacation_event_proc(frame, event, arg, type)

	Frame		frame;
	Event		*event;
	Notify_arg	arg;
	Notify_event_type	type;

{
	Notify_value	value;

	value = notify_next_event_func(frame, (Notify_event) event, arg, type);

	if (event_action(event) == WIN_RESIZE)
		vacation_resize(frame);

	return(value);
}

static int
vacation_done_proc (frame)

	Frame	frame;
{
       /* The pushpin has been pulled out, so the edit status
	* of the textsw must be checked for pending edits.
	*/
	int	modified;
	int	answer;

	modified = (int)xv_get(vacation_textsw, TEXTSW_MODIFIED);

	if (!modified)
		{
		xv_set(frame, XV_SHOW, FALSE, NULL);
		}

       /* The textsw has modified data in it, so ask the user
	* to confirm the removal of the data.
	*/

	else
		{
		answer = mt_vs_confirm(frame, FALSE,
			gettext("Continue"),
			gettext("Cancel"),
			gettext(
"The vacation mail message has been modified.\n\
Do you wish to continue with editing the message\n\
or discard the edits and not start vacation?"));
		if (!answer) {
			xv_set(frame, XV_SHOW, FALSE, NULL);
			textsw_reset(vacation_textsw, 0, 0);
			return(answer);
			}
		else {
			xv_set(frame, FRAME_CMD_PIN_STATE, TRUE, NULL);
			return(answer);
			}
		}
}

static void
vacation_not_start (frame, filename, error)

	Frame	frame;
	char	*filename;
	int	error;

{
        /* STRING_EXTRACTION -
         *
         * This message is displayed when the vacation file could not
         * be read or written.  The first argument is the name of the file;
         * the second is the system error message.
         */
	mt_vs_warn(frame, gettext(
"Could not open\n\
%s\n\
%s\n\
Vacation not started"),
		filename,
		strerror(error));
}

mt_vacation_running()

{
	char	buf[256];
	FILE	*fp;
	int	retval = FALSE;

	sprintf(buf, "%s/%s", getenv("HOME"), FORWARDFILE);

	if ((fp = fopen(buf, "r")) == NULL)
		return(FALSE);
	
	buf[sizeof buf -1] = '\0';
	while (fgets(buf, (sizeof buf) - 1, fp) != NULL) {
		if ((strstr(buf, "/usr/bin/vacation")) ||
		    (strstr(buf, "/usr/ucb/vacation"))) {
			retval = TRUE;
			break;
		}
	}

	fclose(fp);
	return(retval);
}


mt_backup_forwardfile(file)

	char	*file;

{
	char	buf[256];

	strcpy(buf, file);
	strcat(buf, BACKUPSUFFIX);

	if (rename(file, buf) < 0) {
                /* STRING_EXTRACTION -
                 *
                 * We tried to make a backup copy of your .forward file, but
                 * it failed.  The first %s is the name of the rename
                 * target; the second %s is the system error string.
                 */
		mt_vs_warn(vacation_frame,
			gettext(
"Could not move .forward file to\n\
%s\n\
%s\n\
Vacation not started."),
			buf,
			strerror(errno));
	
		return(-1);
	}

	return(0);
}

mt_recover_forwardfile(file)

	char	*file;

{
	char	buf[256];
	FILE	*fp;

	strcpy(buf, file);
	strcat(buf, BACKUPSUFFIX);

	if (rename(buf, file) < 0) {

                /* STRING_EXTRACTION -
                 *
                 * We tried to restore your original .forward file, but could
                 * not.  The first %s is the name of the original .forward file,                 * the second %s is the the system error string.
                 */
		mt_vs_warn(vacation_frame,
			gettext(
"Could not recover .forward file from\n\
%s\n\
%s\n\
The most usual cause for this error is\n\
starting Vacation outside of Mailtool."),
			buf,
			strerror(errno));
	
		return(-1);
	}

	if ((fp = fopen(file, "r")) == NULL)
		return(-1);
	
	buf[sizeof file -1] = '\0';
	while (fgets(buf, (sizeof buf) - 1, fp) != NULL) {
		if (strstr(buf, "User not using forward file")) {
			unlink(file);
			break;
		}
	}

	fclose(fp);
	return(0);
}
