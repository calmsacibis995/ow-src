#ident  "@(#)cmds.c 3.46 97/05/08 Copyr 1987 Sun Micro"



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
 * Mailtool - tool command handling
 */

#include <stdio.h>
#include <errno.h>
#include <signal.h>
#include <fcntl.h>
#include <ctype.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <sysexits.h>
#ifdef	SVR4
#include <sys/kbd.h>
#include <sys/kbio.h>
#include <sys/wait.h>
#else
#include <sundev/kbd.h>
#endif	SVR4

#include <xview/sel_svc.h>
#include <xview/sel_attrs.h>
#include <xview/panel.h>
#include <xview/text.h>
#include <xview/font.h>
#include <xview/cursor.h>
#include <xview/window.h>
#include <xview/xview.h>
#include <xview/scrollbar.h>

#include "glob.h"
#include "tool.h"
#include "tool_support.h"
#include "../maillib/obj.h"
#include "../maillib/assert.h"
#include "instrument.h"
#include "header.h"
#include "attach.h"
#include "cmds.h"
#include "mail.h"
#include "create_panels.h"
#include "delete_log.h"
#include "mle.h"
#include "mail_dstt.h"

#define DEBUG_FLAG mt_debugging
#include "debug.h"

#ifdef PEM
#include "/home/internet/pem-5.0/src/h/pem.h"
#endif PEM

extern	list_t	*find_messageal(Attach_list *, list_t *);

/* external data */
extern Frame	mt_undel_frame;
extern long	mt_msgsizewarning;
extern long	mt_msgsizelimit;




/* local structure definitions */
struct sort_rec {
	int	sr_link;
	struct msg *sr_msg;
	int	sr_key;
	char	*sr_keystr;
};

#define	PRIMARY_PD_SELECTION	0x0010 
#define	PRIMARY_SELECTION	0x0001 


/* local routine definitions */
static int	mt_write_msg(struct msg *m, FILE *fp);
static int	print_msgs(struct header_data *hd, struct folder_obj	*folder,
			Msg_attr flag, int batch);
static int	mt_write_msg(struct msg *m, FILE *fp);
static int	mt_msg_fwrite(char *buffer, int len, FILE *fp);
static int	valid_message_size(struct reply_panel_data *rpd);
static char	*get_file(HeaderDataPtr);
static void	mt_do_sort(HeaderDataPtr, char *field_string);
static int	msort_compare_proc(struct sort_rec **ptr1,
			struct sort_rec **ptr2);
static void	clear_undel(HeaderDataPtr);
static void	mt_set_nomail(HeaderDataPtr);
static void	mt_destroy_views(HeaderDataPtr, int leave_n);
static Sub_error mt_submit_message(struct reply_panel_data *ptr, char *buf,
			int size, int suppressrecord);
static Sub_error mt_submit_pem_message(struct reply_panel_data *ptr, char *buf,
			int size, int suppressrecord, int pemopt);
static int	valid_folder(struct header_data *hd, char *path);
static void	filename_to_fullpath(char *filename, char *fullpath);




/*
 * Commit the current set of changes.
 */
void
mt_commit_proc_hd(
	struct header_data *hd
)
{
	/* Check first if in-box is empty */
	if ((strcmp(MT_FOLDER(hd), name_none) == 0)
	   || (strcmp(MT_FOLDER(hd), mt_mailbox) == 0))
		mt_new_folder(hd, "%", FALSE, FALSE, FALSE, FALSE);
	else
		mt_new_folder(hd, MT_FOLDER(hd), FALSE, FALSE, FALSE, FALSE);
}


/* ARGSUSED */
void
mt_commit_proc(
	Menu      	menu,
	Menu_item       menu_item
)
{
	struct header_data *hd;

	TRACK_BUTTON(menu, menu_item, "save_changes");
	hd = mt_get_header_data(menu);

	mt_commit_proc_hd(hd);
}




/* Compose a new mail message.
 */
void
mt_comp_proc(
	Menu		menu,
	Menu_item	item
)
{
	struct reply_panel_data *ptr;
	struct msg	*m;
	int             orig;
	int		lower_context;
	int		option;
	Msg_attr	flag;
	struct header_data *hd;


	/* ZZZ: too bad that we can't use menu to get the header data */
	hd = mt_get_header_data(menu);

	option = (int) xv_get(item, MENU_CLIENT_DATA);

	orig = (option > 1);

#ifdef INSTRUMENT
	if (orig) {
		TRACK_BUTTON(menu, item, "forwared_msg");
	} else {
		TRACK_BUTTON(menu, item, "compose_new_msg");
	}
#endif INSTRUMENT
	
	/* orig says whether we are to have the original text included */

	/* allocate a frame for the composition session */

	if (!(ptr = mt_get_compose_frame(hd, menu)))
		return;

	xv_set(ptr->reply_panel, PANEL_DEFAULT_ITEM, ptr->dest_fillin, 0);

	mt_save_curmsgs(hd);

	if (orig)
	{
		lower_context = (int)xv_get(ptr->replysw, TEXTSW_LOWER_CONTEXT);

		/* work around to suppress scrolling as you insert */
		(void) xv_set(ptr->replysw, TEXTSW_LOWER_CONTEXT, -1, 0);

		if (mt_any_selected(hd))
			flag = MSG_SELECTED;
		else
			flag = MSG_CURRENT;

		/* Now we look for selected messages */
	
		for( m = FIRST_NDMSG(CURRENT_FOLDER(hd)); m != NULL;
		     m = NEXT_NDMSG( m ) )
		{
			if (msg_methods.mm_get(m, flag))
			{
				if (m->mo_subject != NULL)
				{
					xv_set(ptr->subject_fillin, PANEL_VALUE,
						m->mo_subject, 0);
				}

				mt_include_msg(m, ptr->replysw, FALSE, ptr);
			}
		}

		/* Set insertion point to be top of textsw */
		(void)xv_set(ptr->replysw,
			TEXTSW_LOWER_CONTEXT, lower_context,
			TEXTSW_INSERTION_POINT, 0,
			0);
	}
	
	mt_display_reply(ptr, NULL);

}


/*
 * Copy the selected message to the specified file.
 */
/* ARGSUSED */
void
mt_copy_proc(
Menu		menu,
Menu_item	menu_item
)
{
	char	*file;
	Msg_attr flag;
	struct header_data *hd;

	TRACK_BUTTON(menu, menu_item, "copy_msg");

	hd = mt_get_header_data(menu);
	if (mt_any_selected(hd))
		flag = MSG_SELECTED;
	else
		flag = MSG_CURRENT;

	if ((file = get_file(hd)) != NULL) {
		/* ZZZ: we can't use "menu" to find the header data because
		 * menu is not bound with header data and it may be NULL.
		 */
		mt_do_save(hd, 0, flag, file);
	}

	return;
}

/* ARGSUSED */

/* 
 * Put the currently selected items to the shelf.
 */

void
mt_copyshelf_proc(
	Menu		menu,
	Menu_item	menu_item
)
{
	struct msg	*m;
	char           *file;
	struct header_data *hd;

	hd = mt_get_header_data(menu);
	if (mt_nomail) {
		mt_nomail_warning(hd);
		return;
	}

	TRACK_BUTTON(menu, menu_item, "copyshelf_msg");

	make_selection(hd, SELN_SHELF);
}


/* ARGSUSED */
/* put the currently selected items to the shelf, and then delete them */

void
mt_cut_proc(
	Menu		menu,
	Menu_item	menu_item
)
{
	struct header_data *hd;

	hd = mt_get_header_data(menu);
	if (!mt_any_selected(hd)) {
		/* there is nothing selected -- don't bother */
		return;
	}

	TRACK_BUTTON(menu, menu_item, "cut_msg");

	/* ZZZ: too bad that we can't use menu to get header data */

	make_selection(hd, SELN_SHELF);
	mt_do_del(hd, MSG_SELECTED);
}




void
mt_del_proc_hd(
	struct header_data *hd
)
{
	char           *trash;
	struct msg	*m;
	Msg_attr	flag;
	char		trash_path[MAXPATHLEN];

	if (mt_nomail) {
		mt_nomail_warning(hd);
		return;
	}

	if (mt_any_selected(hd))
		flag = MSG_SELECTED;
	else
		flag = MSG_CURRENT;

	/* save all of the mail items in the trash folder 
	 * (if there is one).
	 */

	if ((trash = mt_value("trash")))
		mt_expand_foldername(trash, trash_path);

	for( m = FIRST_NDMSG(hd->hd_folder); m != NULL; m = NEXT_NDMSG(m))
	{
		if (msg_methods.mm_get(m, flag))
		{
			if (trash || m != MT_CURMSG(hd))
			{
				/* no need to save current message if
				 * no trash folder
				 */
				mt_save_curmsgs(hd);
				if (trash) {
					(void)mt_copy_msg(hd, m, trash_path, 0);
				}
			}
		}
	}
	mt_do_del(hd, flag);
}



/* Delete the current message and display the next message.
 */

void
mt_del_proc(
	Menu		menu,
	Menu_item	menu_item,
	Event		*ie
)
{
	struct header_data *hd;

	TRACK_BUTTON(menu, menu_item, "delete_msg");

	if (menu) {
		hd = mt_get_header_data(menu);
	} else {
		ASSERT(ie);
		hd = mt_get_header_data(event_window(ie));
	}

	mt_del_proc_hd(hd);
}






void
mt_done_proc_hd(
	struct header_data *hd
)
{
	char           *p;
	Frame		frame = MT_FRAME(hd);


        /* STRING_EXTRACTION -
         *
         * "Saving changes and closing..." is a message that appears
         * in the window footer when the user has hit the "done" button.
         */

	mt_busy(MT_FRAME(hd), TRUE, gettext("Saving changes and closing..."), TRUE);
	(void)xv_set(frame, FRAME_RIGHT_FOOTER, "", 0);
	mt_save_curmsgs(hd);
	mt_set_state_no_mail(hd, FALSE); /* Assure the icon is in nomail state */
	(void) xv_set(frame, FRAME_CLOSED, TRUE, 0);
	(void) win_post_id(frame, WIN_RESIZE, NOTIFY_IMMEDIATE);
	(void) win_post_id(frame, WIN_REPAINT, NOTIFY_IMMEDIATE);

	mt_lose_selections(hd);

	mt_set_nomail(hd);

	if (!mt_idle_mail(hd)) {	/* can fail if can't problems writing to /tmp */
		mt_busy(MT_FRAME(hd), FALSE, "", TRUE);
		return;
	}
	mt_idle = TRUE;
	mt_load_from_folder = NULL;
	(void) strcpy(MT_FOLDER(hd), name_none);
	mt_set_state_no_mail(hd, TRUE); /* Assure the icon is updated properly */
	if (p = mt_value("trash"))
		mt_del_trash(hd, p);

	CLEAR_MSGS(hd);
	mt_busy(MT_FRAME(hd), FALSE, "", TRUE);

	TRACK_MESSAGE("done");
}




void
mt_del_trash(
	struct header_data *hd,
	char	*trash_name
)
{
	char		path[MAXPATHLEN + 1];
	struct stat	statb;

	mt_expand_foldername(trash_name, path);

	if (stat(path, &statb) == -1)
		return;

	if (S_ISDIR(statb.st_mode)) {
               	mt_vs_warn(MT_FRAME(hd),
gettext("Could not remove trash file\n%s\nbecause it is a directory.\n\n\
The .mailrc variable \"trash\" must be set to a file,\n\
not a directory.  Mail Tool will ignore \"trash\"\n\
for the remainder of this session"), path);
		mt_deassign("trash");
		return;
	} else {
		unlink(path);
	}

	return;
}




void
mt_done_menu_proc(
	Menu menu,
	Menu_item item,
	Event *ie
)
{
	struct header_data *hd;

	TRACK_BUTTON(menu, item, "done");

	if (menu) {
		hd = mt_get_header_data(menu);
	} else {
		ASSERT(ie);

		hd = mt_get_header_data(event_window(ie));
	}

	mt_done_proc_hd(hd);
}


/*
 * Include the selected message in the message composition window.
 */
/* ARGSUSED */
void
mt_include_proc(
	Menu		menu,
	Menu_item	menu_item
)
{
	Frame           frame;
	struct reply_panel_data *ptr;
	struct header_data	*hd;
	int		status;
	int		lower_context;
	struct msg	*m;
	Panel_item      item;
	Msg_attr	flag;
	int		upper_context;
	int		screen_line_count;


	item = (Panel) xv_get(menu, MENU_CLIENT_DATA);
	ptr = (struct reply_panel_data *)xv_get(
		xv_get(item, XV_OWNER), WIN_CLIENT_DATA);
	/* ZZZ: too bad that we can't use menu to find header data */
	hd = ptr->hd;

	if (mt_nomail) {
		mt_nomail_warning(hd);
		return;
	} else if (MT_CURMSG(hd) == NULL)  {
		/* warning displayed in mt_get_curselmsg */
		return;
	}

#ifdef INSTRUMENT
	if (xv_get(menu_item, MENU_CLIENT_DATA)) {
		TRACK_BUTTON(menu, menu_item, "include_bracketed");
	} else {
		TRACK_BUTTON(menu, menu_item, "include_indented");
	}
#endif INSTRUMENT

	if (mt_any_selected(hd))
		flag = MSG_SELECTED;
	else
		flag = MSG_CURRENT;

	mt_save_curmsgs(hd);


	frame = xv_get(ptr->replysw, XV_OWNER);

	/* Now we look for selected messages */

	/* work around to suppress scrolling as you insert */
	lower_context = (int)xv_get(ptr->replysw, TEXTSW_LOWER_CONTEXT);
	(void) xv_set(ptr->replysw, TEXTSW_LOWER_CONTEXT, -1, 0);

	for( m = FIRST_NDMSG(CURRENT_FOLDER(hd)); m != NULL; m = NEXT_NDMSG( m ) )
	{
		if (msg_methods.mm_get(m, flag))
		{
			mt_include_msg(m, ptr->replysw,
				(int)xv_get(menu_item, MENU_CLIENT_DATA), ptr);
		}
	}

	(void)xv_set(ptr->replysw,
		TEXTSW_LOWER_CONTEXT, lower_context,
		0);

	/* unfortunately, textsw_possibly_normalize moves the current
	 * insertion point to the *top* of the window.  We want it on
	 * the bottom so you actually get some context.
	 *
	 * here is the plan: get the screen line count, and set the
	 * upper context to two less than it.  This should keep things
	 * pretty much on screen...
	 */
	screen_line_count = textsw_screen_line_count(ptr->replysw);
	upper_context = xv_get(ptr->replysw, TEXTSW_UPPER_CONTEXT);
	xv_set(ptr->replysw, TEXTSW_UPPER_CONTEXT, screen_line_count -2, 0);

	/* Scroll to insertion point */
	textsw_possibly_normalize(ptr->replysw,
		(Textsw_index)xv_get(ptr->replysw, TEXTSW_INSERTION_POINT));

	/* reset upper context */
	xv_set(ptr->replysw, TEXTSW_UPPER_CONTEXT, upper_context, 0);
}




/*
 * Display the next message.
 */
/* ARGSUSED */
void
mt_next_proc(
	Menu		menu,
	Menu_item	menu_item
)
{
	struct msg	*nextmsg;
	int		force_view;
	struct header_data	*hd;

	hd = mt_get_header_data(menu);
	if (mt_nomail) {
		mt_nomail_warning(hd);
		return;
	}

	TRACK_BUTTON(menu, menu_item, "next");

	nextmsg = mt_next_msg(hd, MT_CURMSG(hd), FALSE);

	force_view = (menu != NULL);

	mt_update_msgsw(nextmsg, force_view, TRUE, TRUE,
		mt_get_header_data(MT_FRAME(hd)));
}




/*
 * Handles "new mail" and "done"
 */
void
mt_new_mail_proc(
	Menu      	menu,
	Menu_item          menu_item
)
{
	struct header_data	*hd;

	TRACK_BUTTON(menu, menu_item, "new_mail");
	hd = mt_get_header_data(menu);
	mt_new_folder(hd, "%", mt_system_mail_box, mt_system_mail_box,
		FALSE, TRUE);
}




/*
 * Display the previous message.
 */
/* ARGSUSED */
void
mt_prev_proc(

	Menu		menu,
	Menu_item	menu_item
)
{
	int savescandir;
	int	force_view;
	struct msg *nextmsg;
	struct header_data	*hd;

	hd = mt_get_header_data(menu);

	if (mt_nomail) {
		mt_nomail_warning(hd);
		return;
	}

	TRACK_BUTTON(menu, menu_item, "prev_msg");

	savescandir = mt_scandir;
	mt_scandir = -mt_scandir;	/* reverse scan direction */
	nextmsg = mt_next_msg(hd, MT_CURMSG(hd), FALSE);
	if (mt_scandir == -savescandir)	/* if no change, put it back */
		mt_scandir = savescandir;

	force_view = (menu != NULL);
	mt_update_msgsw(nextmsg, force_view, TRUE, TRUE, hd);
}




/*
 * View the selected message
 */
/* ARGSUSED */
void
mt_view_messages(
	struct header_data *hd,
	int	abbrev_headers	/* TRUE to display message with
				 * abbreviated headers
				 */
)
{
	int	flag;

	if (mt_nomail) {
		mt_nomail_warning(hd);
		return;
	}

	if (mt_any_selected(hd)) 
		flag = MSG_SELECTED;
	else
		flag = MSG_CURRENT;

	mt_save_curmsgs(hd);
	mt_create_views(hd, abbrev_headers, flag);
}




#ifdef PEM
/*
 * Decrypt the selected messages
 */
/* ARGSUSED */
void
mt_decrypt_PEM_messages(
	struct header_data *hd

)
{
	int	flag;

	if (mt_nomail) {
		mt_nomail_warning(hd);
		return;
	}

	if (mt_any_selected(hd))
		flag = MSG_SELECTED;
	else
		flag = MSG_CURRENT;
	
	mt_save_curmsgs(hd);
	mt_create_PEM_views(hd, FALSE, flag);
}

#endif PEM


/*
 * Print the selected message on a printer.
 */
/* ARGSUSED */
void
mt_print_proc(
	Menu		menu,
	Menu_item	menu_item
)
{
	Msg_attr		flag;
	struct header_data	*hd;
	int			n;
	char			s[256];

	n = 0;
	s[0] = '\0';

	TRACK_BUTTON(menu, menu_item, "print_msg");

	hd = mt_get_header_data(menu);
	if (mt_nomail) {
		mt_nomail_warning(hd);
		return;
	}

	if (mt_any_selected(hd))
		flag = MSG_SELECTED;
	else
		flag = MSG_CURRENT;

	mt_busy(MT_FRAME(hd), TRUE, gettext("Printing..."), FALSE);

	/* ZZZ: too bad that we can't use menu to find header data */
	n = print_msgs(hd, CURRENT_FOLDER(hd), flag, TRUE);
	if (n < 1) {
		mt_busy(MT_FRAME(hd), FALSE, "", FALSE);
	} else if (n == 1) {
		sprintf(s, gettext("Sent %d message to the printer"), n);
		mt_busy(MT_FRAME(hd), FALSE, s, FALSE);
	} else if (n > 1) {
		sprintf(s, gettext("Sent %d messages to the printer"), n);
		mt_busy(MT_FRAME(hd), FALSE, s, FALSE);
	}
}

/*
 * Print messages in folder based on flag:
 *
 *		MSG_SELECTED	prints selected messages
 *		MSG_CURRENT	prints current message
 *
 * batch is TRUE to batch all messages into 1 print job.  If batch is FALSE
 * then each message is printed in as a seperate print job.
 */
static int
print_msgs(
	struct header_data	*hd,
	struct folder_obj	*folder,
	Msg_attr		flag,
	int			batch
)
{
	struct msg	*m;
	char		form_feed = '\f';
	FILE		*fp;
	char		*print_method;
	int		status = 0;
	int		write_error = 0;
	int		n;
	Frame		frame;


	frame = MT_FRAME(hd);
	n = 0;

	if ((print_method = mt_value("printmail")) == NULL)
#ifdef SVR4
		print_method = "lp -s";
#else
		print_method = "lpr -p";
#endif SVR4

	fp = NULL;
	for( m = FIRST_NDMSG(folder); m != NULL; m = NEXT_NDMSG( m ) ) {

		if (!msg_methods.mm_get(m, flag))
			continue;

		/* Print message */
		if (fp == NULL) {
			/* 1st message of print job.  Open pipe */
			if ((fp = popen(print_method, "w")) == NULL) {
               			mt_vs_warn(frame,
                      		gettext("Could not execute print script\n%s"),
                       			print_method);
				goto EXIT;
			}
		} else {
			/* Subsequent message.  Write page break */
			fwrite(&form_feed, 1, 1, fp);
		}

		if (m == MT_CURMSG(hd))
			mt_save_curmsgs(hd);

		if (mt_write_msg(m, fp) < 0) {
			write_error = TRUE;
			goto EXIT;
		};

		if (!batch) {
			/* One print job per message.  Close pipe */
			status = pclose(fp);
			fp = NULL;
			if (status != 0)
				goto EXIT;
		}
		/* Keep track of number of messages printed */
		n++;
	}
	
EXIT:
	if (fp) 
		status = pclose(fp);

	if (status != 0) {
		mt_vs_warn(frame,
		gettext("Print script failed\nCould not print message"));
		return(-1);
	} else if (write_error) {
              	mt_vs_warn(frame,
			gettext("Write error\nCould not print message"));
		return(-1);
	} else {
		return(n);
	}
}





/*
 * Write a message to fp.  If a message has attachments then we only 
 * write out the first attachment if it is text.  We replace all other
 * attachments with a line telling the user that the message contains
 * attachments.
 */
static int
mt_write_msg(
	struct msg	*m,
	FILE		*fp
)
{
	struct attach	*at;
	int		rcode = 0;
	char		buf[80];
	int		n_attachments = 0;

	if ((at = attach_methods.at_first(m)) != NULL) {
		/* Message has attachments */

		/* Write header fields to print job */
		if (msg_methods.mm_copyheader(m, TRUE, mt_msg_fwrite, (int)fp))
			return(-1);

		/* If first attachment is text, print it */
		if (mt_is_text_attachment(at)) {
			/* output a blank line between the header and the body */
			if ((int)fwrite("\n", 1, 1, fp) < 1)
				return(-1);
			attach_methods.at_decode(at);
			if (attach_methods.at_copy(at, AT_BODY,
					msg_methods.mm_write_bytes, fp) != 0)
				return(-1);
		}

		/* Count the number of attachments */
		while ((at = mt_get_next_attach(at)) != NULL) {
			n_attachments++;
		}

		if (n_attachments > 0) {
			sprintf(buf,
	"\n\n\t<This message contains %d attachments (not printed)>\n\n",
				n_attachments);
			
			if ((int)fwrite(buf, strlen(buf), 1, fp) < 1)
				return(-1);
		}
	} else {
		if (msg_methods.mm_write_msg(m, MSG_ABBR_HDRS, fp) != 0)
			return(-1);
	}

	return(0);
}



static int
mt_msg_fwrite(
	char	*buffer,
	int	len,
	FILE	*fp
)
{
	if ((int)fwrite(buffer, len, 1, fp) < 1)
		return(-1);
	else
		return(0);
}




/*
 * Reply to the selected message.
 */
/* ARGSUSED */
void
mt_reply_proc(
	Menu		menu,
	Menu_item	item
)
{
	struct header_data	*hd;
	struct reply_panel_data *ptr;
	int             lower_context;
	struct msg	*m;
	int		option;
	Msg_attr	flag;

	option = (int) xv_get(item, MENU_CLIENT_DATA);

	hd = mt_get_header_data(menu);
	if (mt_nomail) {
		mt_nomail_warning(hd);
		return;
	}

#ifdef INSTRUMENT
	{
		char *string;

		switch (option) {
		case 0: string = "reply_sender"; break;
		case 1: string = "reply_all"; break;
		case 2: string = "reply_sender_include"; break;
		case 3: string = "reply_all_include"; break;
		default: string = "reply_ILLEGAL"; break;
		}

		TRACK_BUTTON(menu, item, string);
	}
#endif INSTRUMENT

	if (mt_any_selected(hd))
		flag = MSG_SELECTED;
	else
		flag = MSG_CURRENT;

	/* ZZZ: too bad that we can't use menu to get header data */
	mt_save_curmsgs(hd);

	for( m = FIRST_NDMSG(CURRENT_FOLDER(hd)); m != NULL; m = NEXT_NDMSG( m ) )
	{
		if (msg_methods.mm_get(m, flag))
		{
		
			/* allocate a frame for the composition session */
		
			if (!(ptr = mt_get_compose_frame(hd)))
				return;

			lower_context = (int)xv_get(ptr->replysw,
				TEXTSW_LOWER_CONTEXT);

			(void) xv_set(ptr->replysw,
				TEXTSW_LOWER_CONTEXT, -1, /* work around to
							   * suppress scrolling
							   * as you insert */
				TEXTSW_INSERTION_POINT, TEXTSW_INFINITY,
				0);

			mt_reply_msg(m, ptr->replysw,
				(option & 1), (option > 1), ptr);
		
			(void) xv_set(ptr->replysw,
				TEXTSW_LOWER_CONTEXT, lower_context,
				TEXTSW_INSERTION_POINT, 0, 0); 

			mt_display_reply(ptr, ptr->replysw);

			(void) xv_set(ptr->composing_icon,
					ICON_IMAGE, reply_image,
					ICON_MASK_IMAGE, reply_image_mask,
					XV_LABEL, name_mailtool, 
					0);
			xv_set(ptr->frame, FRAME_ICON, ptr->composing_icon, 0);
		}
	}
}




/*
 * Save the selected message in the specified file.
 */
/* ARGSUSED */
void
mt_save_proc(
	Menu		menu,
	Menu_item	menu_item
)
{
	Msg_attr flag;
	char	*file;
	struct header_data *hd;

	TRACK_BUTTON(menu, menu_item, "move_msg");

	hd = mt_get_header_data(menu);
	if (mt_any_selected(hd))
		flag = MSG_SELECTED;
	else
		flag = MSG_CURRENT;

	if ((file = get_file(hd)) != NULL) {
		mt_do_save(hd, 1, flag, file);
	}
	return;
}






/*
 * Sort messages in indicated folder. Remove duplicates.
 */
/* ARGSUSED */
void
mt_sort_proc(
	Menu		menu,
	Menu_item	item
)
{
	char	*field_string;
	struct header_data *hd;

	TRACK_BUTTON(menu, item, "sort_date_time");

	hd = mt_get_header_data(menu);
	if (!(field_string = (char *)xv_get(item, MENU_CLIENT_DATA)))
		return;

	mt_do_sort(hd, field_string);
}





/*
 * Undelete the most recently deleted message.
 */
void
mt_undel_last(
	Menu		menu,
	Menu_item	menu_item

)
{
	char	*s;
	struct msg *msg;
	struct header_data *hd;

	TRACK_BUTTON(menu, menu_item, "undelete_last");

	if (mt_delp == NULL) 
		return;

	msg = mt_delp;

	/* remove if from the undeletion list item */

	mt_remove_last_from_undel_list();

	/* return the message to the world */

	hd = mt_get_header_data(menu);
	mt_do_undel(hd, msg);

	/* fix the "%d deleted message */

	mt_update_folder_status(hd);

	mt_update_msgsw(msg, FALSE, TRUE, TRUE, hd);
}





static int
mt_delete_items(
	struct header_data	*hd,
	Msg_attr		 flag

)
{
	register struct msg *m;
	int		ndeleted;
	Rect		frame_rect;
	struct	view_window_list *vwl;
	struct	view_window_list *list_scanner;

	ndeleted = 0;
	vwl = hd->hd_vwl;
	for( m = FIRST_NDMSG(CURRENT_FOLDER(hd)); m != NULL; m = NEXT_NDMSG( m ) )
	{
		if (!msg_methods.mm_get(m, flag))
			continue;

		if (m->mo_new && mt_new_count)
		{
			mt_new_count--;
		}

		ndeleted++;
		mt_del_msg(m);

		/* Nuke the viewing window for this item */

		list_scanner = vwl;

		while (list_scanner)
		{
			if (list_scanner->vwl_msg == m)
			{
				/*
				 * Clear deleted message view window
				 */
				mt_clear_view_window(list_scanner);
				break;
			}
			else
			{
				list_scanner = list_scanner->vwl_next;
			}
		}

		if (mt_delp == NULL)
		{
			mt_set_undelete_inactive(hd, FALSE);
			if (mt_undel_frame) {
				xv_set(mt_undel_frame, XV_SHOW, FALSE, 0);
			}
		}

		m->m_next = mt_delp;
		mt_delp = m;
		mt_undel_list_add_member(hd, m);

		if (mt_system_mail_box)
			mt_log_transaction(CURRENT_FOLDER(hd), m, TRUE);
	}

	mt_resize_canvas(hd);

	return (ndeleted);
}





/*
 * Delete a message.
 * Called by "delete" and "save".
 */
/* ARGSUSED */
void
mt_do_del(
	struct header_data *hd,
	Msg_attr flag
)
{
	register struct msg *m;
	register struct msg *first_deleted;
	register struct msg *last_deleted;
	register int	target;
	register int	top;
	short		views_were_up;
	short		views_left_up;
	struct msg	*next_msg;
	int		first_line_onscreen;
	int		ndeleted;
	char		*s;
	int		savescandir;

	top = xv_get(hd->hd_scrollbar, SCROLLBAR_VIEW_START);

	first_deleted = NULL;
	first_line_onscreen = 0;
	views_were_up = FALSE;
	views_left_up = FALSE;

	/*
	 * Find the first item to delete, the last item to delete and
	 * the first item to delete which is currently visible on the screen
	 * Also check if any view windows were up, and if there will be
	 * any view windows up after the delete.
	 */
	for( m = FIRST_NDMSG(CURRENT_FOLDER(hd)); m != NULL; m = NEXT_NDMSG( m ) )
	{
		/*
		 * Look for a current message which is not going to be
		 * deleted
		 */
		if (!msg_methods.mm_get(m, flag))
		{
			if (m->mo_current)
				views_left_up = TRUE;
			continue;
		}

		/*
		 * Message n is going to be deleted
		 */
		if (first_deleted == NULL) {
			first_deleted = m;
		}
		last_deleted = m;

		target = m->m_lineno;


		if (!first_line_onscreen) {
			first_line_onscreen = target;
		}
	}

	if (first_deleted == NULL)
		return;

	views_were_up = mt_view_windows_up(hd);

	/*
	 * Delete the message(s) and go to the next message.  If there is no
	 * next message, then go to the previous message.
	 */
	ndeleted = mt_delete_items(hd, flag);
	next_msg = mt_next_msg(hd,
		mt_scandir > 0 ? last_deleted : first_deleted, FALSE);
	if (next_msg == NULL) {
		savescandir = mt_scandir;
		mt_scandir = -mt_scandir;	/* reverse scan direction */
		next_msg = mt_next_msg(hd,
			mt_scandir < 0 ? last_deleted : first_deleted, FALSE);
		if (mt_scandir == -savescandir) /* if no change, put it back */
			mt_scandir = savescandir;
	}

	if (first_line_onscreen) {
		if (ndeleted == 1) {
			mt_shift_nlines(first_line_onscreen + 1, -1, hd);
		} else {
			force_repaint_from_line(
				first_line_onscreen > 1 ?
				first_line_onscreen - 1 :
				first_line_onscreen, TRUE, hd);
		}
	}

	DP(("mt_do_del: num_msgs %d, last lineno %d\n",
		hd->hd_folder->fo_num_msgs,
		LAST_NDMSG(hd->hd_folder) ?
			LAST_NDMSG(hd->hd_folder)->m_lineno : -1));

	if (LAST_NDMSG(hd->hd_folder) == NULL) {
		mt_set_nomail(hd);
	} else {
		next_msg->mo_selected = 1;
		force_repaint_on_line(next_msg->m_lineno, hd);
		mt_update_headersw(next_msg, hd);
		if (views_were_up && !views_left_up)
			mt_create_views(hd, TRUE, MSG_SELECTED);
	}

	mt_hide_view_garbage(hd);
	mt_update_folder_status(hd);

        /* STRING_EXTRACTION -
         *
         * The user has just deleted 0, 1 or more than 1 mail message.
	 * Give them some feedback
         */
	if (ndeleted == 0) {
		s = gettext("No messages deleted");
	} if (ndeleted == 1)
		s = gettext("Message deleted");
	else {
		char	buf[LINE_SIZE];
		sprintf(buf, gettext("%d messages deleted"), ndeleted);
		s = buf;
	}

	mt_frame_msg(hd->hd_frame, FALSE, s);
}




static void
cmds_write_header_fields(
	struct msg *msg,
	Panel	panel
)
{
	Panel_item	item;
	char		*header;
	char		*ptr;
	char		*value;

	/*
	 * Write all fillin fields from the reply panel as header fields into
	 * the file specified by fp
	 */
	PANEL_EACH_ITEM(panel, item)
	{
		/*
		 * If it is a text item and it is visible then write it
		 */
		if (xv_get(item, PANEL_SHOW_ITEM) && (Panel_item_type)
		    xv_get(item, PANEL_ITEM_CLASS) == PANEL_TEXT_ITEM)
		{

			value = (char *)xv_get(item, PANEL_VALUE);

			header = (char *) strdup ((char *) xv_get(item,
				PANEL_LABEL_STRING));

			if (! header) goto end_of_loop;

			if (ptr = (char *) strrchr (header, ':')) {
				*ptr = '\0';
			}

			/* if the value is set, set it in the msg; if it
			 * is not set, then make sure the header is
			 * deleted.
			 */
			if (*value == '\0') {
				value = NULL;	/* mark for deletion */
			}
			msg_methods.mm_set(msg, MSG_HEADER, header, value);

			free (header);
		}
end_of_loop:;
	}
	PANEL_END_EACH;
}




static int
invalid_recipient(
	Panel_item	item
)
{
	char	*value;

	/* 
	 * Make sure that the field is visible and filled in.
	 */
	if (!(int)xv_get(item, XV_SHOW))
		return(TRUE);

	value = (char *)xv_get(item, PANEL_VALUE);

	while (*value == ' ')
		value++;

	return(*value == '\0');
}




/* ARGSUSED */
void
mt_do_deliver(
	Menu		menu,
	Menu_item	menu_item
)
{
	Panel_item	panel_item;
	Panel           panel;
	struct reply_panel_data *ptr;
	int		length;
	Sub_error	ecode;
	char		*buf;
	int		suppressrecord;
	list_t		*list = NULL;
	int		stay_up;
	int		make_iconic;

	panel_item = (Panel) xv_get(menu, MENU_CLIENT_DATA);
	panel = xv_get(panel_item, XV_OWNER);
	ptr = (struct reply_panel_data *)xv_get(panel, WIN_CLIENT_DATA);

	ptr->deliver_flags = (int) xv_get(menu_item, MENU_CLIENT_DATA);

	stay_up = ptr->deliver_flags > 0;
	make_iconic = ptr->deliver_flags == 1;

	suppressrecord = !(int)xv_get(ptr->rpd_record_item,
					PANEL_TOGGLE_VALUE, 0);

        /* STRING_EXTRACTION -
         *
         * "Please specify a recipient." -- the user tried to send a message
         * without having any to, cc, or bcc fields filled in.
         */
	if (invalid_recipient(ptr->dest_fillin) &&
	    invalid_recipient(ptr->cc_fillin) &&
	    invalid_recipient(ptr->bcc_fillin)) {
		mt_vs_warn(ptr->frame,
			gettext("Please specify a recipient."));
		return;
	}

	if (!valid_message_size(ptr)) {
		return;
	}

	/*
	 * Look for any dstt applications that may be applicable
	 * to this message.
	 */
	while (list = find_messageal(ptr->rpd_al, list))
	{
		/* STRING_EXTRACTION -
		 *
		 * "There is an open window on an attachment contained
		 *  in this message. If you SELECT Deliver the last
		 *  saved version of the attachment will be delivered.
		 * 
		 *  Do you want to cancel the delivery and continue
		 *  to edit the attachment or deliver the message
		 *  as requested?"
		 *
		 * If an active dstt session is detected, tell the user
		 * to conclude their work before delivering it.
		 */
		if(!list->delete)
		{
			if (mt_vs_confirm(ptr->frame, FALSE,
				gettext("Cancel"), gettext("Deliver"),
				gettext(
"There is an open window on an attachment contained\n"
"in this message.  If you SELECT Deliver the last\n"
"saved version of the attachment will be delivered.\n"
"\n"
"Do you wish to cancel the delivery and continue\n"
"to edit the attachment or deliver the message as requested.")))
			{
				/* the user didn't want to deliver */
				return;
			}

			/* we're done checking; only ask once */
			break;
		}
	}

	buf = NULL;
	if ((length = xv_get(ptr->replysw, TEXTSW_LENGTH)) > 0)
	{
		buf = (char *) ck_zmalloc (length);
		if (buf == NULL) {
			ecode = SUB_NO_SWAP;
			goto ERROR_EXIT;
		}
		xv_get(ptr->replysw, TEXTSW_CONTENTS, 0, buf, length);
	}

	/*
	 * STRING_EXTRACTION -
	 *
	 * Sending message... is put up in the footer of the compose
	 * window after deliver is hit
	 */
	xv_set(ptr->frame,
		FRAME_BUSY, TRUE,
		FRAME_LEFT_FOOTER, gettext("Sending message..."),
		0);

	ecode = mt_submit_message(ptr, buf, length, suppressrecord);
	(void) ck_unmap(buf);

	if (ecode == SUB_OK) {

		/* normal case */
		if (!stay_up) {
			xv_set(ptr->frame, XV_SHOW, FALSE, 0);
		} else if (make_iconic) {
			xv_set(ptr->frame, FRAME_CLOSED, TRUE, 0);
		}

		return;
	}

ERROR_EXIT:
	/* clear the message; re-enable the window */
	xv_set(ptr->frame, FRAME_BUSY, FALSE, 0);
	xv_set(ptr->frame, FRAME_LEFT_FOOTER, "", 0);

	switch (ecode) {

	case SUB_LOG_ERR:
		/*
		 * If ecode is SUB_LOG_ERR, then there was a problem
		 * writing to the log file, and mt_submit_message
		 * already handled the error message to the user.  For
		 * other errors, we handle them in ERROR_EXIT or
		 * INT_ERROR_EXIT.
		 */
		DP(("mt_do_deliver: ecode was SUB_LOG_ERR\n"));
		break;

	case SUB_NO_SWAP:
		/* STRING_EXTRACTION -
		 *
		 * There was a problem with either having enough memory or
		 * forking a child process to  the message
		 * for delivery.  This is the message we display
		 * to let the user know
		 * we croaked.
		 */
		mt_vs_warn(MT_FRAME(ptr->hd),
			gettext(
				"Unable to deliver this message because\n"
				"a memory allocation failed.\n"
				"Try quitting some other applications\n"
				"and then deliver this message again."));
		break;


	case SUB_FORK:
		/* STRING_EXTRACTION -
		 *
		 */
		mt_vs_warn(MT_FRAME(ptr->hd),
			gettext(
				"Unable to deliver this message because\n"
				"a new process could not be created.\n"
				"Try quitting some other applications\n"
				"and then deliver this message again."));
		break;

	case SUB_NO_TMP_SPACE:
		/* STRING_EXTRACTION -
		 *
		 */
		mt_vs_warn(MT_FRAME(ptr->hd),
			gettext(
				"Unable to deliver this message because\n"
				"there was insufficient temporary space.\n"
				"Try freeing up some temporary space\n"
				"and then deliver this message again."));
		break;

	case SUB_CHARSET:
		/* STRING_EXTRACTION -
		 *
		 */
		mt_vs_warn(MT_FRAME(ptr->hd),
			gettext(
				"Unable to deliver this message because\n"
				"a character set conversion failed.\n"
				"\n"
				"Have your system administrator check\n"
				"the translation modules in /usr/lib/iconv\n"
				"and the charset config control file in\n"
				"/usr/lib/locale/%s/LC_CTYPE/mailcodetab."),
				setlocale(LC_CTYPE, NULL));
		break;


	case SUB_INTERNAL:
	default:
		/* STRING_EXTRACTION -
		 *
		 * Somehow ptr->reply_msg becomes a NULL pointer.
		 * It is an internal error.
		 */
		mt_vs_warn(MT_FRAME(ptr->hd),
			gettext(
				"Unable to deliver the message because\n"
				"Mailtool has detected an internal error.\n"
				"Please save any changes and quit Mailtool."));
		break;
	}

	return;

}



#ifdef PEM
/* ARGSUSED */
void
mt_do_deliver_pem(
	Menu		menu,
	Menu_item	menu_item
)
{
	Panel_item	panel_item;
	Panel           panel;
	struct reply_panel_data *ptr;
	int		intact = FALSE;
	int		stay_up = TRUE;
	int		make_iconic = FALSE;
	int		length;
	Sub_error	ecode;
	char		*buf;
	int		suppressrecord;
	int		i;
	int		options;
	Menu		parent_menu, pullright_item;

	options = (int) xv_get(menu_item, MENU_CLIENT_DATA);

	stay_up = 0;
	make_iconic = 0;
	intact = 0;
	/* Since we're in a pullright menu, we need to get the pullright
	 * item and then its parent menu */
	pullright_item = xv_get(menu, MENU_PARENT);
	parent_menu = (Menu) xv_get(pullright_item, MENU_PARENT);
	panel_item = (Panel) xv_get(parent_menu, MENU_CLIENT_DATA);

	panel = xv_get(panel_item, XV_OWNER);
	ptr = (struct reply_panel_data *)xv_get(panel, WIN_CLIENT_DATA);

	suppressrecord = !(int)xv_get(ptr->rpd_record_item,
					PANEL_TOGGLE_VALUE, 0);

        /* STRING_EXTRACTION -
         *
         * "Please specify a recipient." -- the user tried to send a message
         * without having any to, cc, or bcc fields filled in.
         */
	if (invalid_recipient(ptr->dest_fillin) &&
	    invalid_recipient(ptr->cc_fillin) &&
	    invalid_recipient(ptr->bcc_fillin)) {
		mt_vs_warn(ptr->frame,
			gettext("Please specify a recipient."));
		return;
	}

	if (!valid_message_size(ptr))
		return;

	buf = NULL;
	if ((length = xv_get(ptr->replysw, TEXTSW_LENGTH)) > 0)
	{
		buf = (char *) ck_zmalloc (length);
		if (buf == NULL)
			goto ERROR_EXIT;
		xv_get(ptr->replysw, TEXTSW_CONTENTS, 0, buf, length);
	}
	ecode = mt_submit_pem_message(ptr, buf, length, suppressrecord, options);

	fail -- this code is now broken -- it was not updated
	for the new error cases...

	(void) ck_unmap(buf);
	if (ecode < 0)
		goto ERROR_EXIT;
	if (ecode > 0)
		goto INT_ERROR_EXIT;

	if (!stay_up)
	{
		/* delivery success; remove the dead letter when compose
		 * window is destroyed.  If the compose window is cached,
		 * at least truncate the dead letter.
		 */

		/* Unmap compose frame so clearing it is not visible */
		(void)xv_set(ptr->frame, XV_SHOW, FALSE, 0);
		mt_clear_compose_window(ptr);
		ptr->rpd_rmdead = TRUE;
		if (ptr->rpd_dead_letter)
			truncate(ptr->rpd_dead_letter, 0);
		xv_destroy_safe(ptr->frame);
	}

	if (!intact) {
		mt_clear_compose_window(ptr);
		ptr->inuse = FALSE;
	}

	if (make_iconic)
		xv_set(ptr->frame, FRAME_CLOSED, TRUE, 0);

	return;

ERROR_EXIT:
        /* STRING_EXTRACTION -
         *
         * There was a problem to fork a child process to  the message
	 * for delivery.  This is the message we display to let the user know
	 * we croaked.
         */
	mt_vs_warn(MT_FRAME(ptr->hd),
		gettext("Unable to deliver this message.\nTry quitting some other applications\nand then deliver this message again."));

	return;

INT_ERROR_EXIT:
        /* STRING_EXTRACTION -
         *
         * Somehow ptr->reply_msg becomes a NULL pointer.  It is an internal
	 * error.
         */
	mt_vs_warn(MT_FRAME(ptr->hd),
		gettext("Unable to deliver the message because\nMailtool has detected an internal error.\nPlease save any changes and quit Mailtool."));
	return;

}
#endif PEM




/*
 * Check if the message is big.  If it is, warn user and see if they still
 * want to send it.
 */
static int
valid_message_size(
	struct reply_panel_data *rpd
)
{
	int	text_length;
	int	attachments;
	int	cancel = 0;
	char	*alert_text;

	attachments = mt_attachment_in_msg(rpd->reply_msg);
	text_length = (int)xv_get(rpd->replysw, TEXTSW_LENGTH);

	if (mt_msgsizelimit && 
		((mt_attach_list_size(rpd->rpd_al, FALSE) + text_length) >
		mt_msgsizelimit))
	{
		/* STRING_EXTRACTION -
		 *
		 * These messages are printed when a message or an attachment
		 * are larger than some internal threshhold. The user is not
		 * given a choice here. If this appears the next notice
		 * warning the user about the same thing will not appear.
		 */
		if (attachments) {
                   mt_vs_warn(rpd->frame,
                        gettext("Sorry, you cannot send this message because\nit is too large. It may not reach the recipient, and\nif it does it may overload the recipient's mailbox.\nPlease consider removing some of the larger attachments\nand using another method to transfer them."));
		} else {
                   mt_vs_warn(rpd->frame,
                        gettext("Sorry, you cannot send this message because\nit is too large. It may not reach the recipient, and\nif it does it may overload the recipient's mailbox.\nPlease consider removing some of the message text"));
		}
                return(FALSE);
        }

	if (mt_msgsizewarning &&
		(mt_attach_list_size(rpd->rpd_al, FALSE) + text_length >
		mt_msgsizewarning))
	{
		/* STRING_EXTRACTION -
		 *
		 * These messages are printed when a message or an attachment
		 * are larger than some internal threshhold.  There are two
		 * actions that are put in the popup with the text: either
		 * Cancel or Deliver.
		 *
		 */
		if (attachments) {
		   /* 
    		    * Make sure oversize notice hasn't appeared already
		    * if it has, skip rest and return (TRUE)
		    */
		   if (rpd->rpd_al->oversize_notice) {
			return(TRUE);
		   } else {
			alert_text = gettext("This message is very large.  It may not reach the\nrecipient, and if it does it may overload the\nrecipient's mailbox.  Please consider removing some\nof the larger attachments and using another\nmethod to transfer them.  You may:");
		   }
		} else {
			alert_text = gettext("This message is very large.  It may not reach the\nrecipient, and if it does it may overload the\nrecipient's mailbox.  Please consider removing\nsome of the message text.  You may:");
		}

		/* Fix bugid 1065569 - non-optional confirmation */
		cancel = mt_vs_confirm(rpd->frame, FALSE,
			gettext("Cancel"), gettext("Deliver"), alert_text);
	}

	if (cancel)
		return(FALSE);
	else
		return(TRUE);
}

/*
 * Do the save or copy.
 */
void
mt_do_save(
	struct header_data *hd,
	int             del,
	Msg_attr	flag,
	char		*file
)
{
	char		full_path[MAXPATHLEN];
	char		*s;
	struct msg	*m;
	int		n;
	int		new_file;

	if (mt_nomail) {
		mt_nomail_warning(hd);
		return;
	}

	mt_busy(MT_FRAME(hd), TRUE, "", FALSE);

	mt_file_history_list_add(file, FALSE);

	/* Check if changes to current message should be saved.
	 */
	mt_save_curmsgs(hd);

	/* Now we look for selected messages */
	
	mt_expand_foldername(file, full_path);

	/* Check if file exists so we can put up an intelligent message */
	if (access(full_path, F_OK) == 0)
		new_file = FALSE;	/* File exists */
	else
		new_file = TRUE;	/* File does not exist */

	if (new_file && (s = strrchr(full_path, '/')) != NULL) {
		int ret;

		/* File doesn't exist.  Make sure parent directories exist */
		*s = '\0';
	        /* STRING_EXTRACTION -
       	  	 *
                 * We tried to create the mail file directory, but
                 * something went wrong.  We give the name of the directory.
         	 */
		/* This means something wrong with path */
		ret = mt_check_and_create(full_path, TRUE, MT_FRAME(hd), TRUE);
		if (ret == -2) {
                	mt_vs_warn(MT_FRAME(hd),
                         gettext("Could not create\n%s"),
                         file);
			goto EXIT;
		}
		/* This means user canceled create option */
		if (ret == -1) {
			goto EXIT;
		}
		*s = '/';
	}
		
	n = 0;
	for(m = FIRST_NDMSG(CURRENT_FOLDER(hd)); m != NULL; m = NEXT_NDMSG( m ) ) {

		if (msg_methods.mm_get(m, flag)) {

			if (!mt_copy_msg(hd, m, full_path, 1)) {
				del = 0;	/* don't delete if copy fails */
				break;
			} else {
				n++;
			}
		}
	}

	/* STRING_EXTRACTION -
	 *
	 * The user has just saved one or more messages to a file.  This
	 * tells them that the operation is completed.
	 *
	 * The message is concatenated with the file name that was saved to.
	 */
	if (n == 0) {
		strcpy(full_path, gettext("No messages saved"));
		goto EXIT;
	} if (n == 1)
		sprintf(full_path, gettext("Message saved to %s"), file);
	else {
		sprintf(full_path, gettext("%d messages saved to %s"), n, file);
	}

	/* STRING_EXTRACTION -
	 *
	 * If the save operation created a new file tell the user by
	 * appending this string to the "Message saved to" message
	 */
	if (new_file)
		strcat(full_path, gettext(" (new file)"));

	if (del) {
		mt_do_del(hd, flag);
	}
EXIT:
	mt_busy(MT_FRAME(hd), FALSE, full_path, FALSE);
	return;
}



/*
 * Sort messages in current folder
 */
/* ARGSUSED */
static void
mt_do_sort(
	struct header_data *hd,
	char	*field_string
)
{
	register struct msg *prev;
	register struct msg *m;
	register int	i;
	register int	n;
	struct sort_rec	*msg_array;
	char	*mt_get_char_field();
	char	*field, *p;
	register struct sort_rec *mp;


	/* make sure there is a current folder to sort... */
	if (! hd->hd_folder) {
		mt_nomail_warning(hd);
		return;
	}

	if ((n = hd->hd_folder->fo_num_msgs) < 2) {
		/* there is 1 message in the folder */
		return;
	}

	mt_lose_selections(hd);

        /* STRING_EXTRACTION -
         *
         * The sorting message appear in footer of the window while
	 * the mailbox is being sorted.
         */
	mt_busy(MT_FRAME(hd), TRUE, gettext("Sorting messages..."), TRUE);
	mt_save_curmsgs(hd);

	/* Use "stable" list-merge sort from Knuth (5.2.4) Algorithm L which
	 * needs 2 artificial records R(0) and R(n+1), and the "link fields."
	 */
	msg_array = (struct sort_rec *)calloc(n+2, sizeof(struct sort_rec));

	/* initialize msg_array */
	mp = &msg_array[1];
	for (m = msg_methods.mm_first(hd->hd_folder); m != NULL;
	     m = msg_methods.mm_next(m)) {

		/*
		 * Get field value.  First try getting it as an integer.
		 * If that doesn't work, then we get it as a string.
		 */
		if ((mp->sr_key = mt_get_num_field(field_string, m)) == -1) {
			mp->sr_keystr = mt_get_char_field(field_string, m,TRUE);
		}
		mp->sr_msg = m;
		mp++;
	}
	msort(msg_array, n, sizeof(struct sort_rec),
	      ((caddr_t) &msg_array->sr_link - (caddr_t)msg_array),
	      msort_compare_proc);

	/* save the sorted messages to the linked list and mark the folder
	 * being changed.
	 */
	mp = msg_array;
	i = mp[0].sr_link;
	m = mp[i].sr_msg;
	hd->hd_folder->fo_first_msg = m;
	m->mo_prev = NULL;
	while ((i = mp[i].sr_link) != 0)
	{
		ck_free(mp->sr_keystr);
		prev = m;
		m = mp[i].sr_msg;
		prev->mo_next = m;
		m->mo_prev = prev;
	}
	m->mo_next = NULL;
	hd->hd_folder->fo_last_msg = m;
	hd->hd_folder->fo_changed = 1;
	
	cfree(msg_array);

	/* set the line number for all non-deleted messages after the sort */
	i = 0;
	for (m = msg_methods.mm_first(hd->hd_folder); m != NULL;
	     m = msg_methods.mm_next(m))
	{
		if (!m->mo_deleted)
			m->m_lineno = ++i;
	}

	mt_busy(MT_FRAME(hd), FALSE, "", TRUE);
	mt_repaint_headers(hd->hd_canvas, (Pixwin *) hd->hd_paintwin,
		NULL, TRUE);
}




/*
 * Undelete the indicated message.
 */
/* ARGSUSED */
void
mt_do_undel(
	struct header_data *hd,
	struct	msg	*mptr
)
{
	int	index;
	int	top;

	mt_save_curmsgs(hd);

	if (mt_delp == mptr) {
		mt_delp = mt_delp->m_next;
	} else {
		struct	msg	*m, *next;
		for (m = mt_delp, next = m->m_next; next != NULL; ) {
			if (next == mptr) {
				m->m_next = next->m_next; /* splice it out */
				break;
			}
			m = next;
			next = next->m_next;
		}
	}
	if (mt_delp == NULL) { /* last deleted message */
		mt_set_undelete_inactive(hd, TRUE);
		if (mt_undel_frame)
			xv_set(mt_undel_frame, XV_SHOW, FALSE, 0);
	}

	if (mt_system_mail_box)
		mt_log_transaction(hd->hd_folder, mptr, FALSE);
		
	mt_undel_msg(mptr);

	mt_nomail = FALSE;

	mt_resize_canvas(hd);

	top = (int) xv_get(hd->hd_scrollbar, SCROLLBAR_VIEW_START);

	top++;	/* SCROLLBAR_VIEW_START starts at 0.  m_lineno starts at 1 */

	mt_shift_nlines(mptr->m_lineno, 1, hd);

	mt_frame_msg(hd->hd_frame, FALSE, "");
}



void
mt_new_folder(
	struct header_data *hd,
	char           *file,
	int             quietly,
	int             incorporate,
	int             no_views,
	int             addtohistorylist
	/*
	 * quietly is used in conjunction with auto-retrieval, which is
	 * currently not implemented. It gets new messages, but leave msgsw
	 * viewing the message that it is currently displaying 
	 * addtohistorylist separates out calls from mt_commit_proc 
	 *   (filename expanded, no need to add to history list) from
	 *   other functions calling mt_new_folder.
	 */

	 /* no_views means that the code is not to pop up any 
	    associated views */
)
{

	static int	processed_log;
	int		msgno;
	int             n, goto_system_mail_box;
	char		path[MAXPATHLEN];
	char		s[128];
	char		folder_label[128];
	char		*p;
	struct msg	*old_curmsg;
	int		saving;
	int		good_switch;

	DP(( "mt_new_folder( %s, %s, %s, %s)\n",
		file,
		incorporate ? "incorporate" : "no_incorporate",
		quietly ? "quietly" : "no_quietly",
		no_views ? "noviews" : "no_noviews" ));

	if (strcmp(file, "%") == 0) {
		strcpy(path, "%");

        /*
         * Fix for bug 1144897 - if opening user's mailbox
         * get the current status of it. This prevents
         * the spurious beeps if new mail has just arrived.
         */
        folder_methods.fm_newmail(mt_mailbox);

	} else {
		mt_expand_foldername(file, path);
	}

	if (!valid_folder(hd, path))
		return;

	if (!no_views && mt_view_windows_up(hd) <= 0)
		no_views = TRUE;

	/* retain the current message if not switching folder */
	msgno = -1;
	if (no_views)
	{
		struct msg *m;
		/* find the first selected message if no view window is up */
		if (m = mt_any_selected(hd))
			msgno = m->m_lineno;
	}
	else if (MT_CURMSG(hd))
		msgno = (MT_CURMSG(hd))->m_lineno;

	if (!incorporate && !mt_nomail) {
		/* We are not simply incorporating new mail.  Make sure
		 * message modifications are saved and prepare view windows
		 * for re-use
		 */
		mt_save_curmsgs(hd);
		mt_clear_views(hd);
	}

	if (addtohistorylist)
		mt_file_history_list_add(file, FALSE);

	/* Undo the bug fix #1025098: don't change mt_system_mail_box flag, but
	 * change "quietly" to true if headersw is not empty.  -lau
	 */

	goto_system_mail_box = (strcmp(file, "%") == 0);
	quietly = (quietly && strcmp(MT_FOLDER(hd), name_none) &&
		mt_system_mail_box && goto_system_mail_box &&
		msg_methods.mm_last(hd->hd_folder) &&
		(msg_methods.mm_last(hd->hd_folder)->m_lineno > 0));

	(void)xv_set(hd->hd_frame, FRAME_RIGHT_FOOTER, "", 0);

	if (incorporate && strcmp(MT_FOLDER(hd), name_none))
	{
		struct msg *last;

		/*
		 * 06/24/93 dlp Bug ID 1125999
		 * If the folder is null at this point we can't proceed. This
		 * occurs if we have a corrupted mail file. If we have a corrupted
		 * mail file, and we corrupted it in this session, we'll SEGV
		 * later because we depend on hd_folder being real.
		 */
		if (!hd->hd_folder) {
		    if (strcmp(file, "%") == 0)
                	sprintf(s, gettext("Could not load In-Box"), file);
		    else
                	sprintf(s, gettext("Could not load %s"), file);
		    mt_frame_msg(MT_FRAME(hd), TRUE, s);
		    return;
		}

		/*
		 * if in system mail box, and asking to get mail from system
		 * mail box, simply incorporate new mail without committing
		 * existing changes. 
		 */

		last = LAST_NDMSG(hd->hd_folder);
		old_curmsg = MT_CURMSG(hd);

                /* STRING_EXTRACTION -
                 *
                 * This status message is put up in the window footer pane
                 */
		mt_busy(MT_FRAME(hd), TRUE, gettext("Retrieving new mail..."), FALSE);
		if (mt_incorporate_mail(hd) < 0) {
			mt_busy(MT_FRAME(hd), FALSE, "", FALSE);
			return;
		}

		mt_get_headers(hd, last, !quietly ? msgno : 0);

		mt_nomail = FALSE;

		/* Paint headers */
		mt_append_headers(!quietly, last, hd);

		if (!quietly && !no_views) {
			/* mt_load_headers has
			 * already positioned
			 * the header subwindow
			 */
			mt_update_msgsw(MT_CURMSG(hd), TRUE, FALSE, FALSE, hd);
		}

		if (quietly) {
			MT_CURMSG(hd) = old_curmsg;
		}

		mt_scandir = 1;
		mt_busy(MT_FRAME(hd), FALSE, "", FALSE);

		if (hd->hd_folder->fo_garbage)
		{
			/*
			 * The skipping garbage message occurs if
			 * the user runs Mail while inside of
			 * mailtool, deletes some messages, and
			 * commits, receives some more messages,
			 * and then does an incorporate. It is
			 * caused by the fact that the place that
			 * the Mail that mailtool is attached to
			 * thought was the end of the spool file
			 * is now in the middle of a message. If
			 * the user runs Mail, delets some
			 * messages, commits, and then does an
			 * incorporate before any new messages
			 * have arrived, then Mail will think
			 * there is new mail because the spool
			 * file has been written, but there won't
			 * be any. In this case, mt_get_headers
			 * will not find any new messages and
			 * will return FALSE.
			 */

			mt_error_handle(hd);

#ifdef INSTRUMENT
			{
				char buffer[90];
				sprintf(buffer, "incorporating %d for %d",
					msg_methods.mm_last(hd->hd_folder)->mo_msg_number -
					last->mo_msg_number,
					msg_methods.mm_last(hd->hd_folder)->mo_msg_number);
				TRACK_MESSAGE(buffer);
			}
#endif INSTRUMENT
		}
		return;
	}
	s[0] = '\0';
	
	if (strcmp(MT_FOLDER(hd), name_none) != 0) {
		saving = 1;
	} else {
		saving = 0;
	}

 	if (goto_system_mail_box) {
                /* STRING_EXTRACTION -
                 *
                 * The next four status messages all go into the footer pane.
                 * The %s for the Loading message is the name of the new folder
                 * that is being read.
                 */
		if (saving) {
                        sprintf(s, gettext(
				"Saving changes and retrieving new mail..."));
                } else {
                        sprintf(s, gettext("Retrieving new mail..."));
                }
        } else {
                if (saving) {
                        sprintf(s, gettext( "Saving changes and loading %s..."),
				file);
                } else {
                        sprintf(s, gettext("Loading %s..."), file);
                }       
        }               

	mt_busy(MT_FRAME(hd), TRUE, s, FALSE);

	xv_set(hd->hd_scrollbar, SCROLLBAR_VIEW_START, 0, 0);

	/* BugId 1065286: mailtool is no longer in idle even though
	 * switching folder is unsuccess.  So, incoporating new mail from an
	 * initially non-existing folder will be success.
	 */
	mt_idle = FALSE;

	/*
	 * If mt_set_folder() can't load the folder specified by "path" then
	 * it will reload the current folder.  In either case n will be 
	 * positive so we need good_switch to tell us if the new folder
	 * was actually loaded.  Note, mt_set_folder() may change "MT_CURMSG".
	 */
	if ((n = mt_set_folder(hd, path, &good_switch)) >= 0) {
		clear_undel(hd);

		/* Check if we successfully switched to the system mbox */
		if (good_switch)
			mt_system_mail_box = goto_system_mail_box;

		mt_get_folder(hd);
		mt_prevmsg = NULL;
		mt_scandir = 1;

		if (n == 0) { /* no mail */
			mt_set_nomail(hd);
			mt_new_count = 0;
		} else {

			DP(("MT_CURMSG=%x, msgno=%d, new msgno=%d\n",
				MT_CURMSG(hd), msgno, MT_CURMSG(hd) ? msgno : -1));

			mt_nomail = FALSE;
			/* sets MT_CURMSG
			   (a.k.a. hd->hd_curmsg) */
			(void) mt_get_headers(hd, NULL, MT_CURMSG(hd) ? msgno : -1);

			mt_load_headers(hd);
			if (!no_views) {

				/*
			 	* mt_load_headers has already positioned
			 	* the header subwindow
			 	*/
				mt_update_msgsw(MT_CURMSG(hd), TRUE, FALSE, FALSE,
						hd);
			}
		}

        	/*
         	* If it's the first time we've loaded the system mail box, then
         	* check for and process the delete transaction log.  This will
         	* restore the undelete list
         	*/
        	if (mt_system_mail_box && !processed_log) {
                	processed_log = TRUE;
                	mt_process_transaction_log(hd);
        	} else if (good_switch) {
                	mt_clear_transaction_log();
        	}
	}

	sprintf(folder_label, "%s - %s", name_Mail_Tool, MT_FOLDER(hd));
	xv_set(MT_FRAME(hd), XV_LABEL, folder_label, 0);

	if (mt_vacation_running())
		mt_vacation_label(TRUE);

	mt_busy(MT_FRAME(hd), FALSE, "", FALSE);

	if (!good_switch || n < 0) {
                /* STRING_EXTRACTION -
                 *
                 * For some reason we could not load the specified mail file.
		 * Display a message in the frame footer.  The %s is the name
		 * of the file we could not load.
		 */
		if (strcmp(file, "%") == 0)
                	sprintf(s, gettext("Could not load In-Box"), file);
		else
                	sprintf(s, gettext("Could not load %s"), file);
		mt_frame_msg(MT_FRAME(hd), TRUE, s);
	}
	mt_update_folder_status(hd);

#ifdef INSTRUMENT
	{
		char buffer[90];

		sprintf(buffer, "new_folder %.50s msgs %d new %d",
			MT_FOLDER(hd),msg_methods.mm_last(hd->hd_folder)->mo_msg_number,
			mt_new_count);
		TRACK_MESSAGE(buffer);
	}
#endif INSTRUMENT
}






#ifdef MULTIPLE_FOLDERS

void
mt_new_folder1(
	struct header_data *old_hd,
	char           *file,
	int             quietly,
	int             incorporate,
	int             no_views,
	int             addtohistorylist
	/*
	 * quietly is used in conjunction with auto-retrieval, which is
	 * currently not implemented. It gets new messages, but leave msgsw
	 * viewing the message that it is currently displaying 
	 * addtohistorylist separates out calls from mt_commit_proc 
	 *   (filename expanded, no need to add to history list) from
	 *   other functions calling mt_new_folder.
	 */

	 /* no_views means that the code is not to pop up any 
	    associated views */
)
{

	static int	processed_log;
	int		msgno;
	int             n, goto_system_mail_box;
	char		path[MAXPATHLEN];
	char		s[128];
	char		folder_label[128];
	char		*p;
	struct msg	*old_curmsg;
	int		saving;
	int		good_switch;
        struct          header_data *new_hd;
	Frame		frame;
	char            tool_label[50];

	DP(( "mt_new_folder( %s, %s, %s, %s)\n",
		file,
		incorporate ? "incorporate" : "no_incorporate",
		quietly ? "quietly" : "no_quietly",
		no_views ? "noviews" : "no_noviews" ));

	if (strcmp(file, "%") == 0) {
		strcpy(path, "%");
	} else {
		mt_expand_foldername(file, path);
	}

	if (!valid_folder(old_hd, path))
		return;

	if (addtohistorylist)
		mt_file_history_list_add(file, FALSE);

	(void)xv_set(old_hd->hd_frame, FRAME_RIGHT_FOOTER, "", 0);

	mt_busy(MT_FRAME(old_hd), TRUE, s, FALSE);

	/* BugId 1065286: mailtool is no longer in idle even though
	 * switching folder is unsuccess.  So, incoporating new mail from an
	 * initially non-existing folder will be success.
	 */
	mt_idle = FALSE;

	/*
	 * From this moment on, create NEW hd and use it
	 */
	sprintf(tool_label, "%s - %s", name_Mail_Tool, name_none);

	frame = xv_create(0, FRAME_BASE,
		WIN_IS_CLIENT_PANE,
		WIN_ERROR_MSG, gettext("mailtool: unable to create window\n"),
		XV_LABEL, tool_label,
		FRAME_SHOW_FOOTER,	TRUE,
		FRAME_SHOW_LABEL, TRUE,
		FRAME_ICON, mt_current_icon,
		XV_SHOW, TRUE,
		0);

	/* prob. pas in old_hd later */
	new_hd = mt_finish_initializing1(frame);


	/*
	 * If mt_set_folder() can't load the folder specified by "path" then
	 * it will reload the current folder.  In either case n will be 
	 * positive so we need good_switch to tell us if the new folder
	 * was actually loaded.  Note, mt_set_folder() may change "MT_CURMSG".
	 */
	if ((n = mt_set_folder(new_hd, path, &good_switch)) >= 0) {
		mt_get_folder(new_hd);
		mt_prevmsg = NULL;
		mt_scandir = 1;

		/* sets MT_CURMSG
		   (a.k.a. hd->hd_curmsg) */
		(void) mt_get_headers(new_hd, NULL, MT_CURMSG(new_hd) ? msgno : -1);

		mt_load_headers(new_hd);
		if (!no_views) {
			/*
			 * mt_load_headers has already positioned
			 * the header subwindow
			 */
			mt_update_msgsw(MT_CURMSG(new_hd), TRUE, FALSE, FALSE, new_hd);
		}

        	/*
         	* If it's the first time we've loaded the system mail box, then
         	* check for and process the delete transaction log.  This will
         	* restore the undelete list
         	*/
	}

	sprintf(folder_label, "%s - %s", name_Mail_Tool, MT_FOLDER(new_hd));
	xv_set(MT_FRAME(new_hd), XV_LABEL, folder_label, 0);

	mt_busy(MT_FRAME(old_hd), FALSE, "", FALSE);

	if (!good_switch || n < 0) {
                /* STRING_EXTRACTION -
                 *
                 * For some reason we could not load the specified mail file.
		 * Display a message in the frame footer.  The %s is the name
		 * of the file we could not load.
		 */
		if (strcmp(file, "%") == 0)
                	sprintf(s, gettext("Could not load In-Box"), file);
		else
                	sprintf(s, gettext("Could not load %s"), file);
		mt_frame_msg(MT_FRAME(old_hd), TRUE, s);
	}
	mt_update_folder_status(old_hd);
	mt_update_folder_status(new_hd);

        if (!xv_get(frame, XV_SHOW)) {
		(void) xv_set(new_hd->hd_cmdpanel, XV_SHOW, TRUE, 0);
		(void) xv_set(frame, XV_SHOW, TRUE, 0);
		window_fit(frame);
	}


#ifdef INSTRUMENT
	{
		char buffer[90];

		sprintf(buffer, "new_folder %.50s msgs %d new %d",
			MT_FOLDER(hd),msg_methods.mm_last(hd->hd_folder)->mo_msg_number,
			mt_new_count);
		TRACK_MESSAGE(buffer);
	}
#endif INSTRUMENT
}
#endif MULTIPLE_FOLDERS





static void
clear_undel(
	struct header_data *hd
)
{
	/*
	 * Clear undelete list and counters
	 */
	mt_undel_list_clear();
	mt_delp = NULL;
	mt_set_undelete_inactive(hd, TRUE);
	if (mt_undel_frame)
		window_set(mt_undel_frame, XV_SHOW, FALSE, 0);
}



static void
mt_set_nomail(
	struct header_data *hd
)
{

	mt_nomail = TRUE;
	mt_destroy_views(hd, 1);
	mt_hide_view_garbage(hd);
	mt_update_folder_status(hd);
	mt_clear_header_canvas(hd);
}




static int
msort_compare_proc(
	struct	sort_rec	**ptr1,
	struct	sort_rec	**ptr2
)
{
	char	*keystr1, *keystr2;
	char	savebyte1, savebyte2;
	int	value;

	keystr1 = (*ptr1)->sr_keystr;
	keystr2 = (*ptr2)->sr_keystr;
	if (keystr1 != NULL)
	{
		value = strcoll(keystr1, keystr2);
		return(value);
	}
	else {
		unsigned int	key1, key2;
		key1 = (unsigned int)(*ptr1)->sr_key;
		key2 = (unsigned int)(*ptr2)->sr_key;
		return (key1 < key2 ? -1
			: key1 > key2 ? 1 : 0);
	}
}




static void
mt_destroy_views(
	struct header_data *hd,
	int	leave_n
)
{
	register int		n;
	struct view_window_list *vwl;

	/* Destroy all but leave_n popup viewing windows */

	if (hd->hd_vwl != NULL) {
		mt_save_curmsgs(hd);
		
		/*
		 * The view destroy proc handles managing "hd->hd_vwl"
		 * -- ugh.  We *must* use xv_destroy() here and not
		 * xv_destroy_safe().  If we use xv_destroy_safe() then
		 * the destroy_proc is not called until we return to the
		 * notifier, so hd->hd_vwl is not changed.  It is modified
		 * to clean up the fix for bug 1069263.
		 */
		for (n = -leave_n, vwl = hd->hd_vwl; vwl != NULL;
		     vwl = vwl->vwl_next)
			n++;

		while (hd->hd_vwl && --n >= 0) {
			xv_destroy(hd->hd_vwl->vwl_frame);
		}
	}


	/*
	 * Make sure all remaining windows are unpinned
	 */
	mt_clear_views(hd);
}





/*
 * support routine to tell if there is a selected message
 */
unsigned long
check_for_selected(
	struct msg *m,
	va_list ap
)
{
	return((!m->mo_deleted && m->mo_selected) ? (unsigned long) m : NULL);
}





/*
 * check all messages to see if any are selected; return the first selected msg.
 */
struct msg *
mt_any_selected(
	struct header_data *hd
)
{
	return ((struct msg *) folder_methods.fm_enumerate(CURRENT_FOLDER(hd),
		check_for_selected));
}	





void
mt_set_new_folder(
	struct header_data *hd
)
{
	char	*folder;
	TRACK_BUTTON(menu, menu_item, "open_folder");

	if ((folder = get_file(hd)) == NULL)
		return;
	if (strcmp(folder, mt_mailbox) == 0)
		mt_new_folder(hd, "%", FALSE, FALSE, FALSE, TRUE);
	else
		mt_new_folder(hd, folder, FALSE, FALSE, FALSE, TRUE);
}





#ifdef MULTIPLE_FOLDERS
void
mt_set_new_folder1(
	struct header_data *hd
)
{
	char	*folder;
	TRACK_BUTTON(menu, menu_item, "open_folder");

	if ((folder = get_file(hd)) == NULL)
		return;
	if (strcmp(folder, mt_mailbox) == 0)
		mt_new_folder1(hd, "%", FALSE, FALSE, FALSE, TRUE);
	else
		mt_new_folder1(hd, folder, FALSE, FALSE, FALSE, TRUE);
}
#endif




int
mt_attachment_in_msg(
	struct msg *msg
)
{
	register struct attach *at;

	/* check if there is any non-deleted attachment in the message */
	for (at = attach_methods.at_first (msg); at != NULL;
	     at = attach_methods.at_next (at))
	{
		if (!(int) attach_methods.at_get (at, ATTACH_DELETED))
			return (1);
	}
	return (0);
}


Notify_value
reap_submit(
	struct reply_panel_data *rpd,
	int pid,
	int *status,
	struct rusage *rusage
)
{
	int	intact;
	int	destroy_win;
	int	exit_status = WEXITSTATUS(*status);

	destroy_win = (rpd->deliver_flags == 0);
	intact = (rpd->deliver_flags == 3);

	DP(("reap_submit: pid %d, status %x, deliver_error %d\n",
		pid, *status, rpd->rpd_deliver_error));

	if (! WIFEXITED(*status)) {
		DP(("reap_submit: process %d didn't exit\n", pid));
		return (NOTIFY_IGNORED);
	}

	if (exit_status != 0) {
		/* make the window show back up */
		xv_set(rpd->frame,
			FRAME_LEFT_FOOTER, "",
			FRAME_BUSY, FALSE,
			FRAME_CLOSED, FALSE,
			XV_SHOW, TRUE,
			0);
	}

	if (exit_status != 0) {
		/* unsuccessfull exit! */
		char *sendmail;

		DP(("reap_submit: process %d, error status %d\n", pid,
			exit_status));

		sendmail = mt_value("sendmail");
		if (sendmail == NULL) {
			sendmail = "/usr/lib/sendmail";
		}

                /* STRING_EXTRACTION -
		 *
		 * These three messages are printed when the mail
		 * delivery program (typically sendmail) fails.
		 * the %s is the name of the program that
		 * we tried to use to send the mail.  The %d
		 * is the actual error code returned
		 */

		/* put up a notice here... */
		switch (exit_status) {
		case EX_NOUSER:
		case EX_NOHOST:
			mt_vs_warn(rpd->frame, gettext(
"There was an error returned to Mail Tool from\n\
the delivery program \"%s\".\n\
\n\
The error (%d) indicates that some of the addresses\n\
in the message are incorrect, and did not refer to\n\
any known users in the system.\n\
\n\
Please check the addresses and try again."),
				sendmail, exit_status);
			break;


		case EX_CANTCREAT:
		case EX_IOERR:
		case EX_TEMPFAIL:
			mt_vs_warn(rpd->frame, gettext(
"There was an error returned to Mail Tool from\n\
the delivery program \"%s\".\n\
\n\
The error (%d) indicates that you have exhausted\n\
a system resource needed to deliver mail, usually\n\
either swap space or space in /tmp.  Generally\n\
this can be cured by exiting some programs from\n\
your desktop.\n\
\n\
Your mail may have not been delivered."),
				sendmail, exit_status);
			break;

		default:
			mt_vs_warn(rpd->frame, gettext(
"There was an error returned to Mail Tool from\n\
the delivery program \"%s\".\n\
The error code was %d.\n\
\n\
Your mail may have not been delivered."),
				sendmail, exit_status);
			break;
		    
		}
	} else {

		if (destroy_win)
		{
			/* delivery success; remove the dead letter when
			 * compose window is destroyed.  If the compose
			 * window is cached, at least truncate the dead
			 * letter.
			 */

			xv_set(rpd->frame,
				FRAME_LEFT_FOOTER, "",
				FRAME_BUSY, FALSE,
				0);

			mt_clear_compose_window(rpd);
			rpd->rpd_rmdead = TRUE;
			if (rpd->rpd_dead_letter)
				truncate(rpd->rpd_dead_letter, 0);
			xv_destroy_safe(rpd->frame);

		} else {

			xv_set(rpd->frame,
				FRAME_LEFT_FOOTER, "",
				FRAME_BUSY, FALSE,
				0);

			if (!intact) {
				mt_clear_compose_window(rpd);
				rpd->inuse = FALSE;
			}
		}
	}

	return (NOTIFY_DONE);

}





/*
 * Submit a message for delivery.  Returns:
 *		1	Fatal internal error
 *		0	Everything is AOK
 *		-1	Couldn't fork -- probably out of swap
 */
static Sub_error
mt_submit_message(
	struct reply_panel_data	*ptr,
	char	*buf,
	int	size,
	int	suppressrecord
)
{
	struct attach	*at = 0;
	Sub_error	ecode;
	int		attachment;
	char		*file;
	struct msg	*msg;
	struct submission *sub;
	char		fullpath[MAXPATHLEN+1];
        int		fd;


	attachment = mt_attachment_in_msg(ptr->reply_msg);

	if ((msg = ptr->reply_msg) == NULL)
		return(1);
	msg_methods.mm_set(msg, MSG_IS_ATTACHMENT, attachment);

	cmds_write_header_fields(msg, ptr->reply_panel);

	if (attachment) {
		/* Attachments */
		/* Submit text as first body part */
		if (size > 0)
		{
			at = attach_methods.at_create();
			attach_methods.at_set(at, ATTACH_DATA_TYPE,
					MT_CE_TEXT_TYPE);
			attach_methods.at_set(at, ATTACH_DATA_DESC,
					MT_CE_TEXT_TYPE);
			attach_methods.at_set(at, ATTACH_DATA_NAME,
					MT_CE_TEXT_TYPE);
			attach_methods.at_set(at, ATTACH_BODY, buf);
			attach_methods.at_set(at, ATTACH_CONTENT_LEN, size);
			msg_methods.mm_set(msg, MSG_ATTACH_BODY, at, 1);
		}

	} else {
		/* No attachments.  Text only message */
		msg_methods.mm_set(msg, MSG_CONTENT_LEN, size);
		msg_methods.mm_set(msg, MSG_TEXT_BODY, buf);
	}

	ecode = SUB_OK;
	sub = submission_methods.sub_create ("/dev/console");
	if (sub == NULL) {
		ecode = SUB_NO_SWAP;
	} else {
		/* determine if to wait for the completion of submission,
		 * and/or to record the message
		 */

		/* never wait -- look for the error code in the wait3 proc */
		submission_methods.sub_set(sub, SUB_WAIT, mt_value("sendwait"));

		if (suppressrecord) {
			file = NULL;
		} else if (file = mt_value ("record")) {
			mt_expand_foldername (file, fullpath);
			file = fullpath;

                        /*
                         * Check to see if we can open the file.  If we can't
                         * prompt the user with an error message so that
                         * he/she will have an idea of what went wrong.
                         */
                        fd = open (file, O_CREAT|O_RDWR, 0600);
                        if (fd < 0)
                        {
				char *openerror;

                                switch (errno)
                                {
                                case ENOENT:
                                case ENOTDIR:
                                        openerror = (char *) gettext("Mail Tool was not able to deliver your message\nbecause it cannot access the Logged Messages File:\n%s\nPlease check the path of your log file in the Properties\nand change it if necessary.");
                                        break;
                                case EISDIR:
                                        openerror = (char *) gettext("Mail Tool was not able to deliver your message because\nthe name you have assigned for your Logged Messages File:\n%s\nis a directory.  Please assign it to a file name\nin the Properties.");
                                        break;
                                case EACCES:
                                case EROFS:
                                        openerror = (char *) gettext("Mail Tool was not able to deliver your message\nbecause it does not have permission to create the Logged Messages File:\n%s\nin the path specified.  Please change the Logged Message File in the Properties.");
                                        break;
                                default:
                                        openerror = (char *) gettext("Mail Tool was not able to deliver your message\nbecause it could not save the message in the Logged Message File:\n%s\nPlease make sure the path is correct and change it if necessary in the Properties.");
                                        break;
                                }
                                mt_vs_warn(ptr->frame, openerror, file);
                                ecode = SUB_LOG_ERR;
                                errno = 0;
                        } else {
				close(fd);
			}

		}

                if (ecode == SUB_OK) {
			if (file != NULL)
				submission_methods.sub_set (sub, SUB_RECORD, 
							file);

			if (ptr->rpd_dead_letter != NULL)
			{
				submission_methods.sub_set (sub, SUB_DEADLETTER,
							ptr->rpd_dead_letter);
			}

#ifdef PEM
			submission_methods.sub_set(sub, SUB_PEM_NONE);
#endif PEM

			ecode = submission_methods.sub_done(sub, msg);

			if (ecode == SUB_OK) {
				int	pid;

				ptr->rpd_deliver_error =
					(int) submission_methods.sub_get(sub,
					SUB_ERROR);

				pid = (int) submission_methods.sub_get(sub,
					SUB_PID);

			DP(("mt_submit_message: deliver_error %d, pid %d\n",
					ptr->rpd_deliver_error,
					pid));

				if (pid != 0) {
					typedef Notify_value (*nproc)(
						Notify_client nclient,
						int pid,
						int *status,
						struct rusage *rusage);
					nproc proc;

					if (ptr->rpd_deliver_error != 0) {

						/* keep the compose win
						 * visible; don't wait
						 * for sendmail
						 */
						ecode = SUB_LOG_ERR;
						proc = (nproc) notify_default_wait3;
					} else {
						proc = (nproc) reap_submit;
					}
					notify_set_wait3_func(
						(Notify_client) ptr, proc, pid);
				}
			}
			submission_methods.sub_destroy(sub);
		}
	}

	if (attachment)
	{
		if (at != 0) {
			/* destroy the specially created "text" attachment */
			msg_methods.mm_set(msg, MSG_DESTROY_ATTACH, 1);
		}
	}

	return(ecode);
}



#ifdef PEM

/*
 * Submit a message for delivery.  Returns:
 *		1	Fatal internal error
 *		0	Everything is AOK
 *		-1	Couldn't fork -- probably out of swap
 */
static Sub_error
mt_submit_pem_message(
	struct reply_panel_data	*ptr,
	char	*buf,
	int	size,
	int	suppressrecord,
	int	pemopt
)
{
	struct attach	*at = 0;
	Sub_error	ecode;
	int		attachment;
	char		*file;
	struct msg	*msg;
	struct submission *sub;
	char fullpath[MAXPATHLEN+1];


	attachment = mt_attachment_in_msg(ptr->reply_msg);

	if ((msg = ptr->reply_msg) == NULL)
		return(1);
	msg_methods.mm_set(msg, MSG_IS_ATTACHMENT, attachment);

	cmds_write_header_fields(msg, ptr->reply_panel);

	if (attachment) {
		/* Attachments */
		/* Submit text as first body part */
		at = attach_methods.at_create();
		attach_methods.at_set(at, ATTACH_DATA_TYPE, MT_CE_TEXT_TYPE);
		attach_methods.at_set(at, ATTACH_DATA_DESC, MT_CE_TEXT_TYPE);
		attach_methods.at_set(at, ATTACH_DATA_NAME, MT_CE_TEXT_TYPE);
		attach_methods.at_set(at, ATTACH_BODY, buf);
		attach_methods.at_set(at, ATTACH_CONTENT_LEN, size);
		msg_methods.mm_set(msg, MSG_ATTACH_BODY, at, 1);

	} else {
		/* No attachments.  Text only message */
		msg_methods.mm_set(msg, MSG_CONTENT_LEN, size);
		msg_methods.mm_set(msg, MSG_TEXT_BODY, buf);
	}

	ecode = SUB_OK;
	sub = submission_methods.sub_create ("/dev/console");
	if (sub == NULL) {
		ecode = SUB_NO_SWAP;
	} else {
		/* determine if to wait for the completion of submission,
		 * and/or to record the message
		 */
		submission_methods.sub_set(sub, SUB_WAIT, mt_value("sendwait"));
		if (suppressrecord)
			file = NULL;
		else if (file = mt_value ("record"))
		{
			mt_expand_foldername (file, fullpath);
			file = fullpath;
		}

		if (file != NULL)
			submission_methods.sub_set (sub, SUB_RECORD, file);

		if (ptr->rpd_dead_letter != NULL)
		{
			submission_methods.sub_set (sub, SUB_DEADLETTER,
						ptr->rpd_dead_letter);
		}

		switch (pemopt) {
			case 0:
				submission_methods.sub_set (sub, SUB_PEM_INT);
				break;
			case 1:
				submission_methods.sub_set (sub, SUB_PEM_CONF);
				break;
			case 2:
				submission_methods.sub_set (sub, SUB_PEM_PRIV);
				break;
			default:
				submission_methods.sub_set (sub, SUB_PEM_NONE);
		}			


		ecode = submission_methods.sub_done (sub, msg);
		submission_methods.sub_destroy (sub);
	}

	if (attachment)
	{
		if (at != 0) {
			/* destroy the specially created "text" attachment */
			msg_methods.mm_set(msg, MSG_DESTROY_ATTACH, 1);
		}
	}

	return(ecode);
}

#endif PEM










/*
 * Make sure that path points to a valid mail file (folder).
 */
static int
valid_folder(
	struct header_data *hd,
	char	*path
)
{
	struct stat	stat_buf;

	if  (strcmp(path, "%") ==  0)
		return(TRUE);

	if (stat(path, &stat_buf) == -1) {
                /* STRING_EXTRACTION -
                 *
                 * A stat() function call failed.  The first %s is the name
                 * of the file that we tried to examine.  The second %s is
                 * the system error message (from strerror).
                 */
                mt_vs_warn(MT_FRAME(hd),
                        gettext("Cannot access file\n%s\n%s"),
                        path,
                        strerror(errno));
		return(FALSE);
	} else if (!(stat_buf.st_mode & S_IFREG)) {
                /* STRING_EXTRACTION -
                 *
                 * We examined the file, and it was not a "regular" file
                 * (which means it was a directory or character or block
                 * special device (!), or perhaps a socket.  In any case,
                 * this error message probably won't ever really occur...
                 *
                 * %s is the file name.
                 */
                mt_vs_warn(MT_FRAME(hd),
                        gettext("%s\nis not a valid mail file."),
                        path);

		return(FALSE);
	}else if (!(stat_buf.st_mode & S_IREAD)) {
                /* STRING_EXTRACTION -
                 *                
                 * No read permission on the file.  %s is the file name
                 */               
                mt_vs_warn(MT_FRAME(hd),
                        gettext("Cannot read\n%s\nPermission denied"),
                        path);

		return(FALSE);
	}
	return(TRUE);
}





/*
 * Get the filename that the user has typed into the text field.
 */
static char *
get_file(
	struct header_data *hd
)
{
	static	displayed_message;
	char	*filename;

	filename = (char *)xv_get(hd->hd_file_fillin, PANEL_VALUE);
	if (filename == NULL || *filename == '\0')
	{
                /* STRING_EXTRACTION -
                 *
                 * You tried to do a folder operation without a folder name
                 * entered.  This is the error message.
                 */
                mt_vs_warn(MT_FRAME(hd), gettext(
                        "No Mail File specified.  Please enter one."));

		filename = NULL;
	} else {
                /* STRING_EXTRACTION -
                 *
                 * Tell user "In-Box" is invalid Mail File here.
                 */
                if (strcmp(filename, gettext("In-Box")) == 0) {
                        mt_vs_warn(MT_FRAME(hd), gettext(
"Sorry, you may not specify the word \"In-Box\" as a Mail File here.\nYou have to specify %s."), mt_value("MAIL"));
                        return(NULL);
                }

		/*
		 * Check for leading plus and tell the user that they don't
		 * need it.  We only tell them once.
		 */
		if (*filename == '+') {
			if (!displayed_message) {
				mt_vs_warn(MT_FRAME(hd), 
gettext("The leading + is no longer required for accessing Mail Files.\nAll path names are considered relative to the Mail File directory.\nContinuing operation..."));

				displayed_message = TRUE;
			}
			/*
			filename++;
			*/
		}
	}

	return(filename);
}





/*
 * Expand a folder path into a full path. ~ and $VARS are expanded.
 * Relative paths are considered relative to the Mail File direcotry.
 * Leading +'s are handled for backwards compatibility.
 */
void
mt_expand_foldername(
	char	*file,
	char	*full_path
)
{
	char	tmp_path[MAXPATHLEN];

	if (strcmp(file, "%") == 0) {
		strcpy(full_path, file);
		return;
	}

	ds_expand_pathname(file, tmp_path);

	/* if the pathname begins with "/" or "./" then use it as is */
	if (*tmp_path == '/' || tmp_path[0] == '.' && tmp_path[1] == '/') {
		(void)strcpy(full_path, tmp_path);
	} else {
		/* prepend the folder path to it */
		filename_to_fullpath(tmp_path, full_path);
	}

}





/*
 * Convert a relative path to a full path.  Paths are assumed to be
 * relative to the folder directory.  The relative path is assumed
 * to be full expanded (ie all $VARS resolved).
 * If "folder" is not defined this defaults to the home directory
 */
static void
filename_to_fullpath(
	char           *path,
	char		*full_path
)
{
	/* Get folder directory */
	mt_getfolderdir(full_path);

	/* Append file name */
	if (full_path[strlen(full_path) - 1] != '/')
		(void) strcat(full_path, "/");

	/* Check for + */
	if (*path == '+')
		path++;
	(void) strcat(full_path, path);
}





/*
 * Get the current folder directory
 */
void
mt_getfolderdir(
	char *path
)
{
	char	buf[MAXPATHLEN];
	char	*folder;

	*path = '\0';
	if ((folder = mt_value("folder")) == NULL) {
		/* folder not specified default to $HOME and return error */
		strcpy(path, getenv("HOME"));
		return;
	}

	/* Expand ~s, $VARS, etc */
	ds_expand_pathname(folder, buf);

	if (*buf == '/') {
		strcpy(path, buf);
	} else {
		sprintf(path, "%s/%s", mt_value("HOME"), buf);
	}
}






static int
check_and_create(
	char	*path,		/* Full Path to create */
	int	directory,	/* True to create a directory */
	Frame	frame,		/* Parent frame for alerts */
	int	confirm,	/* True to ask for confirmation */
	int	first_call	/* True if first call in recursion */
)
{
	char	*p;
	struct stat	stat_buf;
	int	rcode;
	int	fd;
	static int	confirm_done;

	DP(("check_and_create(%s, %d, %x, %d, %d)\n",
		path, directory, frame, confirm, first_call));


	/* Only confirm the first directory created */
	if (first_call)
		confirm_done = FALSE;

	/*
	 * Check and possibly create all components of a path. This
	 * routine returns 0 if path is good or the user had us create
	 * it, -1 if the user said don't create it or an errno 
	 * value, -2 if
	 * the path is bogus and we couldn't create it 
	 */

	/*
	 * Check if path is good
	 */
	if (stat(path, &stat_buf) != -1) {
		if (directory) {
			if (stat_buf.st_mode & S_IFDIR)
				return(0);
			else
				return(ENOTDIR);
		} else {
			if (stat_buf.st_mode & S_IFDIR)
				return(0);
			else
				return(EISDIR);
		}
	}

	if (errno == ENOENT) {
		/*
		 * File does not exist.  Check and create parent directory.
		 * -1 is already used later, must distinguish error
		 * messages to routines that calls this routine
		 */
		/* empty dir, strrchr failed */
		if (!(p = strrchr(path, '/')))
			return (-2); 

		if (p != path) {
			*p = '\0';
			rcode = check_and_create(path, 1, frame,
				confirm, FALSE);
			*p = '/';

			if (rcode != 0) {
				return(rcode);
			}
		}

		/*
		 * Ask user if they would like us to create the file
		 */
		if (confirm && !confirm_done) {
			/* STRING_EXTRACTION -
			 *
			 * The user specified a mail file (that is,
			 * folder) directory that did not exist.
			 * Before we go ahead and create the directory,
			 * we check with the user.
			 *
			 */
			rcode = mt_vs_confirm(frame, FALSE,
				gettext("Create"), gettext("Cancel"),
				gettext("%s\ndoes not exist.  Create it?"),
				path);
			confirm_done = TRUE;
		} else {
			rcode = TRUE;
		}

		if (!rcode ) {
			return(-1);
		}
		
		/*
		 * Path to parent is good.  Create file (or directory)
		 */
		if (directory) {
			if (mkdir(path, 0755) == -1)
				return(errno);
		} else {
			if ((fd = open(path, O_CREAT, 0644)) == -1)
				return(errno);
			close(fd);
		}

		return(0);
	}
	return(errno);
}





int
mt_check_and_create(
	char	*path,		/* Full Path to create */
	int	directory,	/* True to create a directory */
	Frame	frame,		/* Parent frame for alerts */
	int	confirm		/* True to ask for a confirmation */
)
{
	/*
	 * Check and possibly create all components of a path. This
	 * routine returns 0 if path is good or the user had us create
	 * it, -1 if the user said don't create it or an errno value if
	 * -2 if the path is bogus and we couldn't create it 
	 */
	return (check_and_create(path, directory, frame, confirm, TRUE));
}

