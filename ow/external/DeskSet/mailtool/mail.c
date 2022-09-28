#ifndef lint
static  char sccsid[] = "@(#)mail.c	3.52 97/01/30 -  Copyr 1997 Sun Micro";
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
 * mail.c -- interface to the mail library
 */


#include <stdio.h>
#ifdef SVR4
#include <unistd.h>
#endif SVR4
#include <ctype.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/param.h>
#ifdef SVR4
#include <sys/statvfs.h>
#endif SVR4
#include <sys/stat.h>
#include <sys/vfs.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <dirent.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <locale.h>

#include <xview/xview.h>
#include <xview/panel.h>
#include <xview/text.h>
#include <xview/font.h>
#include <xview/notice.h>

#include "glob.h"
#include "tool.h"
#include "tool_support.h"
#include "header.h"
#include "attach.h"
#include "mail.h"
#include "main.h"
#include "cmds.h"
#include "create_panels.h"
#include "mle.h"
#include "../maillib/ck_strings.h"
#include "../maillib/charset.h"
#include "../maillib/global.h"

#include "lcl.h"

EXTERN_FUNCTION(Tt_status _ttds_file_server, 
		(const char *filename, char **hostname));

#define TAB	'\t';
#define TABSTR	"\t"

char		*skipwhite2(char *);
char		*skipwhite_reverse(char *, char *);

static int	mt_numsgs;
int             mt_scandir = 1;	/* scan direction, forward to start */
char           *mt_mailbox;	/* name of user's mailbox */

static int	sort_dir_major; /* Sort folder list with dirs grouped first */

static enum mstate {
	S_NOT_INITIALIZED, S_NOMAIL, S_MAIL
}               mt_mstate = S_NOT_INITIALIZED;

struct msg     *mt_delp = NULL;	/* pointer to linked list of deleted messages */
int             mt_new_count;	/* number of new messages */

static	void	mt_set_state();
	void	mt_error_handle();
	void	mt_print_textsw();

static void mt_get_reply_address(struct msg *m, int replyall,
	struct reply_panel_data *ptr);
static void mt_insert_attachment_text(Textsw, struct attach *at,
	int indented, int insert_attachment);
static void mt_insert_msg(Textsw textsw, struct msg *include, int indented);

#define	SEC_PER_DAY	(24*60*60)

#define DEBUG_FLAG mt_debugging
#include "debug.h"

#define ICONVINFO_KEY 400 


/*
 * This function handles fm_write() error.  It displays the error and asks
 * the user if it is O.K. to discard any changes.  Default is NO.
 */
static int
mt_dont_discard(
	struct header_data *hd
)
{
	char *errmsg;

	errmsg = (char *) maillib_methods.ml_get(ML_ERRMSG);
	if (errmsg == NULL)
		return (1);
	else
	{
		return (mt_vs_confirm (MT_FRAME(hd), FALSE,
				gettext("Cancel"),
				gettext("Continue"),
				errmsg));
	}
}


int
mt_idle_mail(
	struct header_data *hd
)
{
	if( folder_methods.fm_write(CURRENT_FOLDER(hd)))
	{
		if (mt_dont_discard(hd))
			return(FALSE);
	}

	CLEAR_MSGS(hd);

	return(TRUE);
}


void
mt_stop_mail(
	int	doabort
)
{
   struct header_data *hd;

   for (hd = mt_header_data_list; hd != NULL; hd = hd->hd_next) {

	if (!doabort && hd && CURRENT_FOLDER(hd)) {
		(void) folder_methods.fm_write(CURRENT_FOLDER(hd));
	}

	if (hd && CURRENT_FOLDER(hd)) {
		mt_attach_clear_folder(hd);
		(void) folder_methods.fm_free(CURRENT_FOLDER(hd));
		CURRENT_FOLDER(hd) = NULL;

		hd->hd_folder = NULL;
		MT_CURMSG(hd) = NULL;
	}
   }
}




/*
 * Flag new mail when it arrives and check to see if the mail has been seen
 * but not necessarily read or deleted. After mail is seen the flag goes
 * down. Returns TRUE if new mail, FALSE if not. 
 */
int
mt_check_mail_box(
	struct header_data *hd,
	int	start_up	/* should it read the mailbox if 
				   there is new mail? */
)
{
	int new_mtype;
	enum mstate new_mail;
	char	folder_label[256];
	Frame 	frame = MT_FRAME(hd);

	/*
	 * new_mtype: 0 no new mail, 1 new mail, 2 new arrival mail.
	 */
	new_mtype = folder_methods.fm_newmail(mt_mailbox);

	mt_numsgs = 1;			/* Assume inbox has messages */
	switch(new_mtype) {
	case FM_NEW_ARRIVAL:		/* New mail has just arrived */
	case FM_NEW_MAIL:		/* The inbox contains new mail */
		new_mail = S_MAIL;
		break;
	case FM_NO_MAIL:		/* The inbox is empty */
		mt_numsgs = 0;
	case FM_NO_NEW_MAIL:		/* The inbox contains no new mail */
	default:	
		new_mail = S_NOMAIL;
		break;
	}

	if (start_up) {
		mt_set_state(hd, new_mail);
	} else if (new_mail == S_NOMAIL) {

		/* if the current folder is the inbox, and we are closed,
		 * then don't lower the flag until we open up
		 */
		if (!CURRENT_FOLDER(hd) ||
			strcmp(CURRENT_FOLDER(hd)->fo_file_name, mt_mailbox) != 0 &&
			xv_get(frame, FRAME_CLOSED) == 0)
		{

			/* we only go to "no mail" state if the access time is
			 * greater than the modification time.
			 */
			if (mt_mstate != S_NOMAIL)
			{
				MP(("No Mail\n\n"));
				mt_set_state(hd, S_NOMAIL);
			}
		}
	} else {

		if (mt_mstate != S_MAIL || new_mtype == FM_NEW_ARRIVAL)
		{
			MP(("New Mail\n\n"));
			mt_announce_mail();
		}

		if ((mt_system_mail_box || 
			!strcmp(MT_FOLDER(hd), name_none)) && 
			mt_value("suppressautoretrieve") == NULL && !mt_idle)
		{
			if (!start_up)
			{
				if (!strcmp(MT_FOLDER(hd), name_none)) {
					mt_new_folder(hd, "%", TRUE, FALSE, FALSE, TRUE);
				} else {
					mt_new_folder(hd, "%", TRUE, TRUE, FALSE, TRUE);
					/* We want to clear up title just
					 * in case title has "New Mail"
					 * - henley
					 */
					sprintf(folder_label, "%s - %s",
						name_Mail_Tool, MT_FOLDER(hd));
					xv_set(frame, XV_LABEL, folder_label, 0);
				}
				
				if ((int)xv_get(frame, FRAME_CLOSED)) {
					mt_set_state(hd, S_MAIL);
				}
			} else {
				mt_set_state(hd, S_MAIL);
			}
		} else {
			/* Get to here in BOTH cases of
			 * (1) in system mailbox 
			 * and suppressautoretrieve is set,
			 * (2) not in system mailbox, regardless if
			 * suppressautoretrieve is set or not.
			 * In both cases we want 
			 * to have "New Mail" displayed on title,
			 * one way is to temporarily set mt_mstate to
			 * be different from what we are passing into
			 * mt_set_state. - henley 
			 */
			if (new_mtype == FM_NEW_ARRIVAL)
				mt_mstate = S_NOMAIL;
			mt_set_state(hd, S_MAIL);
		}
	}

	/*
	 * If none of the above, nothing has changed.
	 */
	return (mt_mstate == S_MAIL);
}



/*
 * Set the icon and window label to reflect the
 * internal state (mail or no mail).
 */
static void
mt_set_state(
	struct header_data *hd,
	enum mstate     s
)
{
	Server_image	new_image = NULL;
	Server_image	new_image_mask = NULL;
	char	folder_label[256];
	int	nmsgs;
	int	update_icon;
	Frame 	frame = MT_FRAME(hd);

	/* Usually if the state doesn't change we don't do anything.  But
	 * for S_NOMAIL we must also check if the mailbox is empty or not.
	 */
	if (mt_mstate == s && mt_mstate != S_NOMAIL) return;

	DP(("mt_set_state: %s\n", s == S_NOMAIL ? "NOMAIL" :
		(s == S_MAIL ? "MAIL" : "????") ));

	switch (s) {
	case S_NOMAIL:
		if (mt_numsgs <= 0) {
			new_image = emptymail_image;
			new_image_mask = emptymail_image_mask;
		} else {
			new_image = nomail_image;
			new_image_mask = nomail_image_mask;
		}

		if (!mt_system_mail_box)
		{
        		sprintf(folder_label, "%s - %s",
				name_Mail_Tool, MT_FOLDER(hd));
        		xv_set(frame, XV_LABEL, folder_label, 0);
		}

		break;
	case S_MAIL:
		new_image = mail_image;
		new_image_mask = mail_image_mask;

		if (!mt_system_mail_box || mt_value("suppressautoretrieve"))
		{
			/* STRING_EXTRACTION -
			 *
			 * [New Mail] is the addition to the normal header
			 * line that we add when there is unread mail
			 * in ones mailbox.
			 */
        		sprintf(folder_label, "%s - %s %s",
				name_Mail_Tool, MT_FOLDER(hd),
				gettext("[New Mail]"));

        		xv_set(frame, XV_LABEL, folder_label, 0);

			if (mt_vacation_running()) {
				mt_vacation_label(TRUE);
			}
		}
		break;
	}

	/* Update the icon only if it has changed */
	if (new_image != NULL &&
	   (Server_image)xv_get(mt_current_icon, ICON_IMAGE) != new_image) {
		(void)xv_set(mt_current_icon, 
			ICON_IMAGE, new_image,
			ICON_MASK_IMAGE, new_image_mask,
			ICON_TRANSPARENT, TRUE,
			WIN_RETAINED,	TRUE,	/* Server repaints icon */
			0);
		(void)xv_set(frame, FRAME_ICON, mt_current_icon, 0);
	}

	mt_mstate = s;
}




/*
 * format and print a header string.  We use the Mush format:
 * 
 * %% -- a single %
 * %c -- number of bytes in the message
 * %C -- number of bytes in the message contents
 * %f -- entire from field
 * %l -- number of lines in the message
 * %L -- number of lines in the message contents
 * %i -- message id
 * %s -- subject of message
 * %t -- To: field (recipients)
 * %r -- recipient (sender, or to line if you sent)
 * %d -- date and time of the message
 * %n -- sender's real name (if any; otherwise use %r or %f)
 * %m -- message number
 * %?header? -- insert contents of header (if any)
 * 
 * You can also specify a field width in printf format: either
 * %25s (25 chars of subject), %.25s (no more than 25 chars),
 * or %10.25s (at least 10 and no more than 25 chars)
 *
 * leading '-' means to left justify the string.
 * 
 */
static char *
mprintf(
	u_char *fmt,
	struct msg *m
)
{
	char buffer[2048];
	char *buf;
	int buflen;
	char addbuffer[60];
	int leftjustify = 0;
	int width;
	int precision;
	char tmpbuffer[2048];
	char *tmp;
	char *string;
	char *freethis;
	char template[60];
	char *tp;
	int number;
	int len;
	char numtype;
	char *prefix;

	buf = buffer;
	buflen = sizeof buffer;

	/* subtract one for the null we will add at the end */
	for (buflen--; *fmt && buflen; fmt++ ) {

		/* deal with the easy case first */
		if (*fmt != '%') {
literal:
			*buf++ = *fmt;
			buflen--;
			continue;
		}

		freethis = NULL;
		prefix = NULL;
		tmp = tmpbuffer;
		leftjustify = 0;
		width = 0;
		precision = 0;
		numtype = 's';	/* default value */
		fmt++;

		if (*fmt == '-' ) {
			leftjustify = 1;
			fmt++;
		}

		if (isdigit(*fmt)) {
			while (isdigit(*fmt)) {
				width = width * 10 + *fmt - '0';
				fmt++;
			}
		}

		if (*fmt == '.') {
			fmt++;

			while (isdigit(*fmt)) {
				precision = precision * 10 + *fmt - '0';
				fmt++;
			}
		}

		/* now handle the other cases */
		switch (*fmt) {
		case '\0':
			/* a trailing %. not legal: just drop the % */
			goto end;

		case '%':
		default:
			/* a %%; add a literal % */
			goto literal;

		case 'C':	/* number of chars in message content */
			number = (int) msg_methods.mm_get(m, MSG_CONTENT_LEN);
			numtype = 'd';
			break;

		case 'c':	/* number of chars in message */
			number = (int) msg_methods.mm_get(m, MSG_NUM_BYTES);
			numtype = 'd';
			break;

		case 'f':	/* From: field */
			string = m->mo_from;
			break;

		case 'L':	/* number of lines in message content */
			number = (int) msg_methods.mm_get(m, MSG_CONTENT_LINES);
			numtype = 'd';
			break;

		case 'l':	/* number of lines */
			number = (int) msg_methods.mm_get(m, MSG_NUM_LINES);
			numtype = 'd';
			break;

		case 'i':	/* message id */
			freethis = string =
				msg_methods.mm_get(m, MSG_HEADER, "message-id");
			break;

		case 's':	/* subject */
			string = m->mo_subject;
			break;

		case 't':	/* to: field */
			string = m->mo_to;
			break;

		case 'd':	/* date and time */
			string = m->mo_unix_date;
			break;

		case 'm':	/* message number */
			number = m->mo_msg_number;
			numtype = 'd';
			break;

		case 'n':
			freethis = string =
				msg_methods.mm_get(m, MSG_HEADER, "from");
			if (string != NULL)
			{
				tp = strpbrk(string, "(<");
				if (tp == NULL)
				{
					/* no-op */
				}
				else if (*tp == '<')
				{
					/* From: Real Name <user@host> */

					/* strip the quotes if any */
					if (*string == '"')
					{
						char *ptr;
						ptr = strrchr(++string, '"');
						if (ptr)
							tp = ptr;
					}

					string = skipwhite2(string);
					tp = skipwhite_reverse(string, tp);
					*tp = '\0';
					if (*string != '\0')
						break;
				}
				else
				{
					/* From: user@host (Real Name) */
					string = tp;
					tp = strrchr(++string, ')');
					if (tp != NULL)
					{
						string = skipwhite2(string);
						tp = skipwhite_reverse(
								string, tp); 
						*tp = '\0';
						if (*string != '\0')
							break;
					}
				}
				ck_free(freethis);
				freethis = NULL;
			}

			/* No real name; use %f (set noshowto) or %r. */
			if (!mt_value("showto"))
			{
				string = m->mo_from;
				break;
			}
			/*FALLTHRU*/
		case 'r':
			/* hack alert: it's just too hard to right
			 * justify the leading "to"...
			 */
			leftjustify = 1;

			if (m->mo_to && is_sender(m)) {
				prefix = "To ";
				string = m->mo_to;

				if (width > 3) width -= 3;
				if (precision > 3) precision -= 3;
				strcpy(tmp, "To ");
				tmp += strlen(tmp);
			} else {
				string = m->mo_from;
			}
			break;

		case '?':	/* the header between the two '?' */
			tp = strchr((char *) ++fmt, '?');
			if (!tp) {
				/* no closing question mark.  just continue */
				fmt--;
				continue;
			}
			len = tp - (char *) fmt;
			strncpy(tmpbuffer, (char *) fmt, len);
			tmpbuffer[len] = '\0';

			freethis = string =
				msg_methods.mm_get(m, MSG_HEADER, tmpbuffer);

			fmt = (u_char *) tp;

			break;
		}


		tp = template;
		*tp++ = '%';

		if (leftjustify) {
			*tp++ = '-';
		}

		if (width) {
			sprintf(tp, "%d", width);
			tp += strlen(tp);
		}

		if (precision) {
			sprintf(tp, ".%d", precision);
			tp += strlen(tp);
		}

		*tp++ = numtype;
		*tp = '\0';

		switch (numtype) {
		case 'd':
			sprintf(tmp, template, number);
			break;
		case 's':
			if (string) {
				sprintf(tmp, template, string);
			} else {
				*tmp = '\0';
			}
			break;

		}

		len = strlen(tmpbuffer);
		if (len > buflen) len = buflen;
		memcpy(buf, tmpbuffer, len);
		buf += len;
		buflen -= len;

		if (freethis) free(freethis);
	}
end:
	*buf = '\0';

	return (ck_strdup(buffer));
}




char *
makeheader(
	struct msg *m
)
{
	char *format;

	format = mt_value("header_format");

	if (!format) {
		if (mt_value("showto")) {
			format = "%3m %-18.40r %16.16d %4l/%-5c %-.100s";
		} else {
			format = "%3m %-18.40f %16.16d %4l/%-5c %-.100s";
		}
	}

	return(mprintf((u_char *) format, m));
}





/*
 * Get the headers for all messages from the Mail subprocess.
 *	change_current: 0 means no change, -1 means to change to
 *	new/unread/last message, or a message number to be changed to.
 */
void
mt_get_headers(
	struct header_data	*hd,
	struct msg     *start_msg,
	int             change_current
)
{
	int             n = 0, first = TRUE;
	register struct msg *mp;
	int		new_height, win_height;
	register struct msg	*first_new = NULL;
	register struct msg	*first_unread = NULL;
	int		lineno;
	char *ptr_holder;
	extern char *mm_enconv();
	static int locale_C=0;

	DP(("mt_get_headers()\n"));

	if (start_msg) {
		mp = NEXT_NDMSG(start_msg);
		lineno = start_msg->m_lineno;
	} else {
		mp = FIRST_NDMSG(CURRENT_FOLDER(hd));
		lineno = 0;
		mt_new_count = 0;
	}

	/* Performance enhancement - only use lcl when not in the C locale */
	if (!locale_C)
	   if (strcmp(setlocale(LC_CTYPE,NULL),"C")!=0)
	      locale_C=1;
	      else locale_C=2;

	for(; mp; mp = NEXT_NDMSG(mp))
	{
		mp->mo_selected = 0;
		mp->mo_current = 0;
		if (mp->mo_new) {
			if (first_new == NULL) {
				first_new = mp;
			}

			mt_new_count++;
		}

		if (!mp->mo_read) {
			if (first_unread == NULL) {
				first_unread = mp;
			}
		}

		if (mp->m_header) {
			free(mp->m_header);
		}


		   
		/* Allow for encoding convertion of the subject line */
		if ((mp->mo_subject) && (locale_C!=2)){
		   ptr_holder=mp->mo_subject;
		   mp->mo_subject=mm_enconv(mp->mo_subject,strlen(mp->mo_subject));
		   free(ptr_holder);
		}


		mp->m_header = makeheader(mp);
		mp->m_next = NULL;
		mp->mo_deleted = FALSE;
		mp->m_lineno = ++lineno;

		MP(( "m %d %c%c (%s)\n", mp->m_lineno,
			mp->mo_new ? 'N' : ' ',
			mp->mo_read ? 'R' : 'U',
			mp->mo_subject ));
	}


	if (change_current) {
		if (change_current > 0) {
			MT_CURMSG(hd) = NUM_TO_MSG(hd, change_current);

			/* ZZZ dipol: fix for bug 1066286, but why
			 * is the message number to damn big???
			 */
			if (MT_CURMSG(hd) == NULL)
				MT_CURMSG(hd) = LAST_NDMSG(CURRENT_FOLDER(hd));
		} else {
			if (first_new != NULL)
				MT_CURMSG(hd) = first_new;
			else if (first_unread != NULL)
				MT_CURMSG(hd) = first_unread;
			else
				MT_CURMSG(hd) = LAST_NDMSG(CURRENT_FOLDER(hd));
		}

		if (MT_CURMSG(hd) != NULL) {
			(MT_CURMSG(hd))->mo_selected = 1;
		}
		mt_activate_functions();
	}

	mt_resize_canvas(hd);
}

/*
 * Get the name of the current folder from Mail.
 */
void
mt_get_folder(
	struct header_data *hd
)
{
	char           *p;

	if (CURRENT_FOLDER(hd)) {
		(void)strcpy(MT_FOLDER(hd), CURRENT_FOLDER(hd)->fo_file_name);
	} else {
		(MT_FOLDER(hd))[0] = '\0';
	}
}



static int
strcmp_ptr(
	char **a,
	char **b
)
{
	if (sort_dir_major) {
		/*
		 * Not very efficient.  We may want to consider updating this
		 * data structure so we know what's a directory and what isn't
		 * without examining the name
		 */
		short isdir_a, isdir_b, n;

		n = strlen(*a);
		isdir_a = ((*a)[n - 1] == '/' || ((*a)[n - 1] == '@' &&
						  (*a)[n - 2] == '/'));

		n = strlen(*b);
		isdir_b = ((*b)[n - 1] == '/' || ((*b)[n - 1] == '@' &&
						  (*b)[n - 2] == '/'));

		if (isdir_a && !isdir_b)
			return(-1);
		else if (!isdir_a && isdir_b)
			return(1);
	}

	return(strcoll(*a,*b));
}

/*
 * Get the names of all the user's folders.
 */
char **
mt_get_folder_list(
	char           *path_string,
	char           *fdir_name,
	char          **strtabp,
	int            *acp,
	int		dir_first /* TRUE to group dirs at start of list */
)
{
	char          **av;
	char          **av_list;
	int             ac;
	int             ac_size;
	DIR	       *dirp = NULL;	/* Fix for 1235199 */
	struct dirent  *dp;
	char		path[MAXPATHLEN +1];
	char	       *path_entry;
	struct stat	statbuf;
	char	       *string_start;
	char	       *string_end;
	char	       *strings;
	int		string_size;
	int		retry_count = 0;

	strcpy(path, fdir_name);
	path_entry = &path[strlen(path)];

	DP(("mt_get_folder_list: fdir_name %s\n", fdir_name));

top:
	if (retry_count != 0) {
		if (retry_count > 5) goto error;

		/* the directory size changed; free the buffers */
		ck_free(string_start);
		if (av_list) {
			free(--av_list);
		}
	}

	retry_count++;

	ac = 0;
	av = NULL;
	av_list = NULL;
	string_start = NULL;
	ac_size = 0;
	string_size = 0;



	/* figure out how large the directory is */
	if (stat(fdir_name, &statbuf) < 0 ) {
		/* Malloc up minimal size.  Directory will appear empty */
		av_list = ck_malloc(3 * sizeof(char *));
		av_list++;
		goto error;
	};

	/* allocate a buffer large enough to hold it all
	 * This is actually much larger than we need so we have room
	 * adding stuff to names (like /).
	 *
	 * This routine used to look at statbuf.st_size, but this
	 * seems to be incorrect for automounted directories.  This
	 * was reported as bug # 1154438...
	 *
	 * So instead we will use a two pass algorithm; the first one
	 * to size the directory; the next to fill in the buffer.
	 */

	/* Fix for 1235199 - if dirp is non-NULL, close it first */
	if (dirp) {
		closedir(dirp);
		dirp = NULL;
	}

	dirp = opendir(fdir_name);
	if (! dirp) goto error;

	while (dp = readdir(dirp)) {
		/* Skip over everything which starts with a . */
		if (*(dp->d_name) == '.')
			continue;

		ac_size++;
		/* plus 4 -- for null termination,  '/' and '@' */
		string_size += strlen(dp->d_name) + 4;
	}

	string_start = ck_malloc(string_size + 2);
	string_end = &string_start[string_size + 1];
	strings = string_start;

	/* now go and set up the pointers as a list */
	av_list = ck_malloc( (ac_size + 3) * sizeof(char *) );

	/* save a spot for MENU_STRINGS token */
	av = ++av_list;


	/* rewind the directory */
	/* can't rewinddir() -- this is broken on automounted
	 * mount points.  See bug id # 1165889
	 */

	/* Fix for 1235199 - if dirp is non-NULL, close it first */
	if (dirp) {
		closedir(dirp);
		dirp = NULL;
	}

	dirp = opendir(fdir_name);
	if (! dirp) goto error;

	/* for each entry in the directory */
	while (dp = readdir(dirp)) {

		/* Skip over everything which starts with a . */
		if (*(dp->d_name) == '.')
			continue;

		/* sanity check for string buffer space */
		if (strings + strlen(dp->d_name) + 4 >= string_end) {
			/* someone must have added to the directory.
			 * go back and try again.
			 */
			goto top;
		}

		/* protect av against extra entries */
		if (ac++ >= ac_size) goto top;

		/* remember this name */
		*av++ = strings;
		strcpy(strings, dp->d_name);
		strings += strlen(dp->d_name);

		/* remember the type */
		strcpy(path_entry, dp->d_name);
		if (lstat(path, &statbuf) >= 0) {
			switch (statbuf.st_mode & S_IFMT) {
			case S_IFDIR:
				*strings++ = '/';
				break;
			case S_IFLNK:
				if (stat(path, &statbuf) >= 0 &&
					(statbuf.st_mode & S_IFMT) ==
					S_IFDIR)
				{
					/* a symbolic link to a
					 * directory
					 */
					*strings++ = '/';
				}

				*strings++ = '@';
				break;
			}
		}
		*strings++ = '\0';
	}

	/* terminate the list */
	*strings = '\0';

	/* Fix for 1235199 */
	if (dirp) {
		closedir(dirp);
		dirp = NULL;
	}

	/* sort the list */
	sort_dir_major = dir_first;
	qsort(av_list, ac, sizeof(char *), (int (*)())strcmp_ptr);

error:
	if (av_list == NULL && string_start != NULL) {
		free(string_start);
		string_start = NULL;
	}

	if (av_list == NULL) {
		ac = 0;
	}

	*strtabp = string_start;
	*acp = ac;
	return(av_list);
}




/*
 * Reply to the specified message. Put reply in the textsw. If "all", reply to
 * all recipients. If "orig", include original message. Called from
 * mt_reply_proc 
 */
void
mt_reply_msg(
	struct msg     *m,
	Textsw         textsw,
	int            all,
	int            orig,
	struct reply_panel_data	*ptr
)
{
	struct attach	*at;
	int		attachments;

	/*
	 * Check to see if the message has any attachments. 
	 */
	if ((at = attach_methods.at_first(m)) != NULL) {
		attachments = TRUE;
	} else {
		attachments = FALSE;
	}

	mt_text_clear_error(textsw);

	/* fill out the mail header info properly */

	mt_get_reply_address(m, all, ptr);

	mt_text_insert(textsw, "\n", 1);

	/* if an inclusion is to be made, go get it. */

	if (orig) {
		/*
		 * When replying to a message with attachments we only
		 * include the text of the message. Check to see if the
		 * first attachment is text.  If it is, include it
		 */
		if (attachments) {
			mt_insert_attachment_text(textsw, at, TRUE,
				mt_is_text_attachment(at));
		} else {
			mt_insert_msg(textsw, m, TRUE);
		}
	}
}


/*
 * Includes specified message.
 * Message is indented if "indent" is set.
 * Called from mt_include_proc. 
 */
void
mt_include_msg(
	struct msg     *m,
	Textsw          textsw,
	int             indented,
	struct reply_panel_data	*rpd
)
{
	char	       *string;
	int		message_text;	/* Does the message have any text */
	int		attachments;	/* Does the message have attachments */
	Frame		frame;
	struct attach	*newat;
	struct attach	*at;
	Attach_list	*al;

	message_text = TRUE;
	attachments = FALSE;

	al = rpd->rpd_al;

	/*
	 * Check to see if the message has any attachments.  If it does, 
	 * check to see if the first one is text (which should be loaded
	 * into the textsw).
	 */
	if ((at = attach_methods.at_first(m)) != NULL) {
		attachments = TRUE;
		if (!mt_is_text_attachment(at))
			message_text = FALSE;
	}

	mt_text_clear_error(textsw);

	/* STRING_EXTRACTION -
	 *
	 * These two message are included before and
	 * after any included text.
	 *
	 * The messages are included twice, but should
	 * be translated identically.
	 */
	if (!indented) {
		string = gettext("\n----- Begin Included Message -----\n\n");
		mt_text_insert(textsw, string, strlen(string));
	}

	if (attachments) {
		/* Load first text attachment into textsw */
		mt_insert_attachment_text(textsw, at, indented, message_text);

		if (message_text) {
			at = mt_get_next_attach(at);
		}
	} else {
		(void)mt_insert_msg(textsw, m, indented);
	}

	if (!indented) {
		string = gettext("\n----- End Included Message -----\n\n");
		mt_text_insert(textsw, string, strlen(string));
	}

	/* Load remaining attachments into attachment list */
	if (attachments) {
		/* Duplicate the attachments first, and put them into msg */
		while (at) {
			newat = attach_methods.at_dup (at);
			msg_methods.mm_set (al->al_msg, MSG_ATTACH_BODY, newat,
					MSG_LAST_ATTACH);
			at = attach_methods.at_next (at);
		}
		at = attach_methods.at_first (al->al_msg);

		mt_show_attach_list(al, TRUE);
		mt_load_attachments(NULL, al, at, TRUE);
		mt_layout_compose_window(rpd);
	}
}


/* this routine gets the return address information for
 * the current mail item, and places it in the control 
 * header for the composition window.  What gets filled in
 * is the To: field, Cc: field and the Subject: field.
 */
	
static void
mt_get_reply_address(
	struct msg	*m,
	int		replyall,
	struct reply_panel_data	*ptr
)
{
	char	*cc;
	char	*to;
	char	*subj;

	/* handle subject -- the easiest one */
	subj = msg_methods.mm_get (m, MSG_REPLY_SUBJECT);
	if (subj)
	{
		xv_set(ptr->subject_fillin, PANEL_VALUE, subj, 0);
		ck_free (subj);
	}

	/* handle the "to" line */
	to = msg_methods.mm_get (m, (replyall ?
				MSG_REPLY_TO_ALL : MSG_REPLY_TO_SENDER));
	xv_set(ptr->dest_fillin, PANEL_VALUE, to, 0);
	ck_free (to);

	/* handle the "cc"; only needed if we are repling to all line */
	if (replyall)
	{
		cc = msg_methods.mm_get (m, MSG_REPLY_TO_CC);
		if (cc)
		{
			if (xv_get(ptr->cc_fillin, PANEL_SHOW_ITEM) == FALSE)
				xv_get(ptr->cc_fillin, PANEL_SHOW_ITEM, TRUE,0);
			xv_set(ptr->cc_fillin, PANEL_VALUE, cc, 0);
			ck_free(cc);
		}
	}
}



/*
 * support routine to output indented lines.  Not the fastest
 * method (because we do too many writes), but it works...
 */
static int
mt_insert_indent(
	char *buf,
	int len,
	Textsw textsw
)
{
	char *p;
	char *indent;
	char *bufend;
	int indentlen;
	extern char *findchar();
	int error;
	int insertlen;
	int	again_recording;

	indent = mt_value("indentprefix");
	if(! indent) indent = "> ";
	indentlen = strlen(indent);

	bufend = &buf[len];

	/* Turn off "again recording" to improve insertion speed */
	again_recording = (int)xv_get(textsw, TEXTSW_AGAIN_RECORDING);
	(void)xv_set(textsw, TEXTSW_AGAIN_RECORDING, FALSE, 0);
	error = 0;
	while (buf < bufend) {
		p = findchar('\n', buf, bufend);

		/* advance past the \n */
		if( p < bufend) p++;

		/* write the indent string and the text */
		mt_text_insert(textsw, indent, indentlen);
		mt_text_insert(textsw, buf, p - buf);

		/* advance the pointer */
		buf = p;
	}
	(void)xv_set(textsw, TEXTSW_AGAIN_RECORDING, again_recording, 0);

	return (error);
}
int
mt_insert_indent_single(
	char *buffer,
	int len,
	Textsw textsw
)
{
	if (glob.g_iconvinfo != NULL)
		cs_methods.cs_copy2(glob.g_iconvinfo, mt_insert_indent, buffer,
                                len, textsw);
	
	else
		mt_insert_indent(buffer, len, textsw);

	return (0);
}

int
mt_insert_textsw_single(
	char *buffer,
	int len,
	Textsw textsw
)
{
	if (glob.g_iconvinfo != NULL)
		cs_methods.cs_copy2(glob.g_iconvinfo, mt_insert_textsw, buffer,
                                len, textsw);
	
	else
		mt_text_insert(textsw, buffer, len);

	return (0);
}


/*
 * argument fitting routine...
 */
int
mt_insert_textsw(
	char *buffer,
	int len,
	Textsw textsw
)
{
	mt_text_insert(textsw, buffer, len);
	return (0);
}




static void
mt_insert_attachment_text(
	Textsw	textsw,
	struct attach	*at,
	int 	indented,
	int	use_attachment
)
{
	int	(*p_func)();
        struct reply_panel_data *ptr;

        if (find_at2msg(at->at_msg) != NULL && !at_has_coding(at)) { 
                /*  only one and it does not have any coding that needs to
                 *  be translated.
                 */

		if (indented) 
			p_func = mt_insert_indent_single;
		else
			p_func = mt_insert_textsw_single;
		mm_copyattach(at->at_msg, MSG_ABBR_HDRS, 0, p_func, textsw, 1);
	}
	else {
		if (indented) 
			p_func = mt_insert_indent;
		else 
			p_func = mt_insert_textsw;

		/* * Copy the message header, add the blank 
			line, and the contents.
	 	*/
		if (msg_methods.mm_copyheader(at->at_msg, MSG_ABBR_HDRS, p_func,
						textsw) == 0)
		{
			/*
		 	* Insert a text attachment into textsw
		 	*/
			if (use_attachment && attach_methods.at_decode(at) == 0)
			{
				(*p_func)("\n", 1, textsw);
				attach_methods.at_copy(at, AT_BODY, p_func,
					(void *) textsw);
			}
		}
	}

        ptr = (struct reply_panel_data *) xv_get(textsw, WIN_CLIENT_DATA);
	if (CURRENT_FOLDER(ptr->hd)
		&& CURRENT_FOLDER(ptr->hd)->fo_garbage)
		mt_error_handle (ptr->hd);
}

static void
mt_insert_msg(
	Textsw         textsw,
	struct msg     *include,
	int             indented
)
{
        struct reply_panel_data *ptr;

        ptr = (struct reply_panel_data *) xv_get(textsw, WIN_CLIENT_DATA);
	if (indented) {
		if (attach_methods.at_first(include) != NULL && find_at2msg(include) != NULL) 
			msg_methods.mm_copymsg(include, MSG_ABBR_HDRS, mt_insert_textsw_single, textsw, 1);
		else
			msg_methods.mm_copymsg(include, MSG_ABBR_HDRS, mt_insert_indent, textsw, 1);
	} else {
		mt_print_textsw(ptr->hd, include, textsw, MSG_ABBR_HDRS);
	}

	if (CURRENT_FOLDER(ptr->hd)
		&& CURRENT_FOLDER(ptr->hd)->fo_garbage)
		mt_error_handle (ptr->hd);
}

/*
 * A wrapper for writing out normal message or multipart message.
 * This is needed for bugid 1065427.  ZZZ: the real way to do is to
 * make mm_write_msg() a wrapper.  It wasn't done so because
 * mm_write_msg() was used everywhere.
 */
static int
copy_msg(
	struct msg *m,
	int ignore_header,
	void *arg
)
{
	if ((int) msg_methods.mm_get(m, MSG_IS_ATTACHMENT))
		return (msg_methods.mm_write_attach(m, ignore_header, arg));
	else
		return (msg_methods.mm_write_msg(m, ignore_header, arg));
}

/*
 * Save a message in a file or folder.
 * Return FALSE on failure, TRUE on success.
 */
int
mt_copy_msg(
	struct header_data *hd,
	struct msg     *m,
	char           *file,
	int		give_warn
)
{
	char	*path;
	int	retval;
	char tmp_path[MAXPATHLEN];

	/* Check for the + even though we should never see one at this point */
	if (*file == '+') {
		mt_expand_foldername(file, tmp_path);
		path = tmp_path;
	} else {
		path = file;
	}

	/* STRING_EXTRACTION -
	 * The user has asked us to copy a message out to
	 * a mail file.  We print several error messages
	 * if we cannot do it.
	 *
	 * "Cannot write %s:" is printed with the file name
	 * when we have a hard write error.
	 *
	 * "%s: Cannot complete write..." is printed when
	 * we had a partial write -- only some of the
	 * bytes got written.  Again, the argument is the
	 * file name.
	 */
	retval = folder_methods.fm_add(path, copy_msg, (void *) m,
			(int) mt_value("alwaysignore") ?
			(void *) MSG_SAVE_HDRS : (void *) MSG_FULL_HDRS);
	switch(retval) {
	case 0:	/* success */
		return(TRUE);

	case 1:	/* error */
		if (give_warn) {
			mt_vs_warn(MT_FRAME(hd),
				gettext("Cannot write %s: %s\n"),
				file, strerror(errno));
		}
		return(FALSE);
		
	case 2:	/* EOF */
		if (give_warn) {
			mt_vs_warn(MT_FRAME(hd), gettext(
			"%s: Cannot complete write.  Out of space?\n"), file);
		}
		return(FALSE);

	case 3:	/* lock failed; user already warned */
		return (FALSE);
	}
}

/*
 * Print a message.
 */
void
mt_print_textsw(
	struct header_data *hd,
	struct msg     *m,
	Textsw         textsw,
	int             ign
)
{
	if (attach_methods.at_first(m) != NULL && find_at2msg(m) != NULL)
		msg_methods.mm_copymsg(m, ign, mt_insert_textsw_single, textsw, 1);
	else
		msg_methods.mm_copymsg(m, ign, mt_insert_textsw, textsw, 1);

	if (CURRENT_FOLDER(hd)
		&& CURRENT_FOLDER(hd)->fo_garbage) {
		mt_error_handle(hd);
	}
}

/*
 * Print a message headers into textsw
 */
void
mt_print_textsw_headers(
	struct header_data *hd,
	struct msg     *m,
	Textsw         textsw,
	int            ign
)
{
	msg_methods.mm_copyheader(m, ign, mt_insert_textsw, textsw);

	if (CURRENT_FOLDER(hd)
		&& CURRENT_FOLDER(hd)->fo_garbage)
		mt_error_handle(hd);
}


void
mt_error_handle(
	struct header_data *hd
)
{
	mt_vs_warn(MT_FRAME(hd), gettext(
"Mail Tool is confused about the state of your Mail File.\nThis could occur if you did a save in another instance\nof Mail or Mailtool while this Mail Tool was running.\n\nPlease Quit this Mail Tool"));

	mt_aborting = TRUE;
}

/*
 * Delete the specified message.
 */
void
mt_del_msg(
	struct msg *m
)
{
	/*
	 * set flag to signify that a change has been made
	 */
	m->mo_selected = 0;
	m->mo_current = 0;
	msg_methods.mm_set (m, MSG_DELETED, TRUE);

	/* shifting the line number for the rest of undeleted messages */
	while (m = NEXT_NDMSG(m))
	{
		m->m_lineno--;
	}
}

/*
 * Delete all messages in the current folder and write out and reload
 * the it
 */
void
mt_truncate_current_folder(
	struct header_data *hd
)
{
	register struct msg	*m;

	/*
	 * Delete all messages in the folder
	 */
	for (m = FIRST_NDMSG(CURRENT_FOLDER(hd)); m != NULL; m = NEXT_NDMSG(m))
	{
		m->mo_selected = 0;
		m->mo_current = 0;
		msg_methods.mm_set(m, MSG_DELETED, TRUE);
	}

	/*
	 * Reload the current empty folder
	 */
	mt_new_folder(hd, MT_FOLDER(hd), FALSE, FALSE, FALSE, FALSE);
}



/*
 * Undelete the specified message.
 */
void
mt_undel_msg(
	struct msg *m
)
{
	struct msg *prev_msg;
	register int lineno;

	msg_methods.mm_set (m, MSG_DELETED, FALSE);

	/* get the proper line number for the undeleted message */
	if ((prev_msg = PREV_NDMSG(m)) == NULL)
		lineno = 0;
	else
		lineno = prev_msg->m_lineno;
	do
	{
		m->m_lineno = ++lineno;
	} while (m = NEXT_NDMSG(m)); 
}



/*
 * fcode is the value returned by fm_lock() which always passes -1 or > 0
 * to this function.  -1 means that the locking failed due to the system
 * limitation or an internal error.  > 0 is implementation dependent which
 * if lockf(3) is used it will be a pid of the lock holder process.
 *
 * This function should return FM_AGAIN, FM_NOLOCK, or FM_ABORT.
 */
static int
mt_notify_lock(
	struct folder_obj *folder,
	int fcode,
	time_t *stime
)
{
	int	commit;
	int	first;
	int	errcode;
	char	*servername;

	if (first = (*stime == 0))
		(void) time (stime);

	if (fcode > 0)
	{
		/* there is a local process accessing the folder, we
		 * ask the user what to do.
		 */
		if (!first)
			commit = NOTICE_YES;
		else
		{
			commit = mt_vs_confirm3(mt_frame, FALSE,
				gettext("Save Changes"),
				gettext("Ignore"),
				gettext("Cancel"),
				gettext("This mail file has been changed by another mail reader.\nDo you wish to ask that mail reader to save the changes?"));
		}

		if (commit == NOTICE_NO)	/* Cancel */
			return (FM_ABORT);
		if (commit != NOTICE_YES)	/* Ignore */
			return (FM_NOLOCK);
						/* Ask to save changes */
		if ((time((time_t) NULL) - *stime) > 60)
		{
			/* the process refuses to give up the lock in
			 * 1 minute, ignore the exclusive access.
			 */
			return (FM_NOLOCK);
		}
		else
		{
			/* ask the lock holder to release the folder */
			folder_methods.fm_lock(folder, FM_NOTIFY,
				(void *)fcode);
			sleep (5);
			return (FM_AGAIN);
		}
	}
#ifdef	TOOLTALK_LOCK
	else if (folder->fo_ttlocked && (fcode == -2))
	{
		/*
		 * We use _ttds_file_server() to get the hostname
		 * where the mail file is located as well as the
		 * error condition when we tried to access rpc.ttdbserverd.
		 * Since there's a chance that this call will succeed even
		 * though tool talk locking failed, we print an error
		 * message in any case.
		 *
		 * TT_ERR_DBEXIST means we were able to get to the file
		 * server, but rpc.ttdbserverd isn't installed. Administrative
		 * action must be taken in order for file scoping to work.
		 *
		 * TT_ERR_DBAVAIL means we weren't able to reach the file
		 * server due to the server being down, network problems,
		 * or something similar.  It's possible that it will work
		 * if we try again later.
		 *
		 * TT_ERR_FILE means the file name specified is invalid
		 * (NULL, too long, etc.).  It's not clear that this would
		 * ever happen since we do some checking on the file before
		 * getting to this point, so we print just the generic
		 * message in this case.
		 *
		 * ZZZ: We should figure out a way to make the split
		 * between mailtool and maillib cleaner here.  Mailtool
		 * has to know too much about maillib's use of tooltalk
		 * locking here to print out the error messge.
		 */

		errcode = _ttds_file_server(folder->fo_file_name, &servername);
		switch (errcode) {

			case TT_ERR_DBEXIST:
				/* STRING_EXTRACTION -
				 *
				 * This error message appears when the
				 * user tries to open a mail file, but
				 * the ToolTalk database is not installed
				 * on the machine where the file is
				 * located.
				 */
				commit = mt_vs_confirm(mt_frame, FALSE,
					gettext("Continue"),
					gettext("Cancel"),
					gettext(
"Mail Tool is unable to lock the mail file\n\
%s for exclusive\n\
reading. This is because the ToolTalk\n\
database is not installed on the machine\n\
%s (where the mail file is located).\n\
\n\
In order to fix this problem, the System\n\
Administrator for the machine %s\n\
must install ToolTalk on that machine.\n\
\n\
You may continue with opening this mail file,\n\
but if another Mail Tool is reading this\n\
mail file, you may confuse its state. Do\n\
you wish to continue?"),
					folder->fo_file_name,
					servername,
					servername);
				break;

			case TT_ERR_DBAVAIL:
				/* STRING_EXTRACTION -
				 *
				 * This error message appears when the
				 * user tries to open a mail file, but
				 * the ToolTalk server is not able to
				 * communicate with the machine where
				 * the file is located.
				 */
				commit = mt_vs_confirm(mt_frame, FALSE,
					gettext("Continue"),
					gettext("Cancel"),
					gettext(
"Mail Tool is unable to lock the mail file\n\
%s for exclusive\n\
reading. This is because it can't communicate\n\
with the machine %s,\n\
(where the file is located).\n\
\n\
It is possible that trying again later will\n\
fix the problem.\n\
\n\
You may continue opening this mail file,\n\
but if another Mail Tool is reading this\n\
mail file, you may confuse its state. Do\n\
you wish to continue?"),
					folder->fo_file_name,
					servername);
				break;

			case TT_ERR_FILE:
			default:
				/* STRING_EXTRACTION -
				 *
				 * This error message appears when the
				 * user tries to open a mail file, but
				 * for some reason the ToolTalk server
				 * was not able to acquire the lock on
				 * the mail file.
				 */
				commit = mt_vs_confirm(mt_frame, FALSE,
					gettext("Continue"),
					gettext("Cancel"),
					gettext(
"Mail Tool is unable to lock the mail file\n\
%s exclusively.\n\
If you choose to continue opening this mail\n\
file, Mail Tool will not lock the file for\n\
exclusive reading.  If another Mail Tool is\n\
reading this mail file, you may confuse its state.\n\
Do you wish to continue?\n"),
					folder->fo_file_name);
				break;
		}
		if (!commit)			/* Cancel */
			return (FM_ABORT);
		else				/* Ignore */
			return (FM_NOLOCK);
	}
#endif	TOOLTALK_LOCK
	else
	{
		/* there is a remote mail reader accessing the folder, we can't
		 * ask it to give up the folder.  It may not happen when
		 * tool-talk is ready.
		 */

		commit = mt_vs_confirm(mt_frame, FALSE,
			gettext("Continue"),
			gettext("Cancel"),
			gettext(
"This mail file cannot be locked for exclusive reading.\n\
If another mailtool is reading this mail file, you\n\
may confuse its state.  Do you wish to continue?")); 

		if (!commit)			/* Cancel */
			return (FM_ABORT);
		else				/* Ignore */
			return (FM_NOLOCK);
	}
}

static struct folder_obj *
mt_folder_open(
	char *path
)
{
	time_t	stime = 0;
	int	(*notify_proc)();
#ifdef SVR4
        struct statvfs statbuf;
#else
	struct statfs statbuf;
#endif SVR4
	struct folder_obj *folder;


	/* We use tooltalk to lock the folder. If we don't use tooltalk
	 * then we use lockf(), but lockf() on an NFS mounted file system
	 * will upset mmap().  A quick hack is to not use lockf()  on any
	 * NFS mounted file. since mailx doesn't do tooltalk locking now,
	 * it is possible to couurpt the spool file when using mailx while
	 * mailtool has a mail file loaded.
	 *
	 * There was a bug in tooltalk in OWV3.0.1 where file scoped messages
	 * did not work, but it was fixed in OWV3.1.
	 *
	 * ZZZ: We should find a solution where mailtool and mailx work
	 * well together.
	 */
	if (!strcmp (path, "%"))
		path = mt_value("MAIL");

#ifdef	TOOLTALK_LOCK
	if (mt_value("ttlock"))
		notify_proc = mt_notify_lock;
	else {
#endif  TOOLTALK_LOCK
#ifdef SVR4
		statbuf.f_files=0;
		/* f_files in SVR4 is ulong; but SUNOS is long */
		if ((statvfs (path, &statbuf) < 0) || (statbuf.f_files == -1))
#else
		if ((statfs (path, &statbuf) < 0) || (statbuf.f_files == -1))
#endif SVR4
			notify_proc = NULL;
		else
			notify_proc = mt_notify_lock;
	}

	folder = folder_methods.fm_read(path, notify_proc, &stime,
		mt_getegid());
	return (folder);
}

/*
 * Switch to the specified folder.  If we can't load the specified folder
 * than we re-load the original folder.  We return the number of messages
 * in the loaded folder.  Return -1 if we couldn't save out the original
 * folder, or we have some other fatal error.
 *
 * If n is not negative than good_switch will be TRUE if the loaded folder
 * is the one specified by path. It will be FALSE if we couldn't load the
 * specified folder and had to reload the original folder.
 *
 * ZZZ: Yes this is hokey and needs to be cleaned up.
 */
int
mt_set_folder(
	struct header_data *hd,
	char           *path,
	int		*good_switch_p
)
{
	char *folder_path = NULL;
	register char  *p;

	DP(("mt_set_folder(%s)\n", path ));

	*good_switch_p = TRUE;

	if (CURRENT_FOLDER(hd)) {
		/* If the write failed, stay in the same folder  */
		if (folder_methods.fm_write(CURRENT_FOLDER(hd)) != 0)
		{
			if (mt_dont_discard(hd))
				return (-1);
		}

		folder_path = strdup (CURRENT_FOLDER(hd)->fo_file_name);
		mt_attach_clear_folder(hd);
		folder_methods.fm_free(CURRENT_FOLDER(hd));
	}

	CURRENT_FOLDER(hd) = mt_folder_open (path);

	if (!CURRENT_FOLDER(hd)) {
		/* Failed to open the new folder, then re-open the old one */
		if (folder_path) {
			CURRENT_FOLDER(hd) = mt_folder_open (folder_path);
		}
		*good_switch_p = FALSE;
	}

	if (folder_path && CURRENT_FOLDER(hd))
	{
		/* we are switching to different folder, set current msg to
		 * NULL.  Otherwise, we pick the last msg as current msg, but
		 * this can be overriden by the calling function.
		 */
		if (strcmp(folder_path, CURRENT_FOLDER(hd)->fo_file_name) != 0)
			MT_CURMSG(hd) = NULL;
		else if (MT_CURMSG(hd))
			MT_CURMSG(hd) = CURRENT_FOLDER(hd)->fo_last_msg;
	}

	if (folder_path) {
		free (folder_path);
		folder_path = NULL;
	}

	if (!CURRENT_FOLDER(hd))
		return (-1);

	if (!CURRENT_FOLDER(hd)->fo_last_msg)
		return(0);
	return (CURRENT_FOLDER(hd)->fo_last_msg->mo_msg_number);
}

/* incorporates mail into system mail box */
int
mt_incorporate_mail(
	struct header_data *hd
)
{
	int	status;

	DP(("mt_incorporate_mail: called\n"));

	if (!CURRENT_FOLDER(hd)) return (0);

	if ((status = folder_methods.fm_reread(CURRENT_FOLDER(hd))) < 0) {
		DP(("mt_incorporate_mail: fm_reread failed\n"));
		return (-1);
	}

	return (status);
}


/*
 * Find the next message after msg.  If none after,
 * use specified message if not deleted.  Otherwise,
 * find first one before msg.  If none, return 0.
 */
struct msg *
mt_next_msg(
	struct header_data *hd,
	struct msg	*msg,
	int             consider_selection
)
{
	struct msg	*m;

	if (! msg) msg = msg_methods.mm_first(CURRENT_FOLDER(hd));
	if (! msg) return(NULL);

	for( m = msg;;) {

		if( mt_scandir > 0 ) {
			m = msg_methods.mm_next(m);
		} else {
			m = msg_methods.mm_prev(m);
		}

		if (!m) {
			if (!msg->mo_deleted &&
				(!consider_selection ||
				(consider_selection && msg->mo_selected))) {
				
				return(msg);
			} else {
				return(NULL);
			}
		}

		if (!m->mo_deleted &&
			(!consider_selection ||
			(consider_selection && m->mo_selected)))
		{
			return (m);
		}

	}

#ifdef GOBACKWARD

	/* In trying to be compatible with /usr/ucb/mail, mailtool would go
	 * to the previous message if there was no next message.  But people
	 * found this counter-intuitive, so now mailtool does nothing if
	 * there is no next message.  The following code enclosed by the
	 * #ifdef GOBACKWARD can isn't used anymore, but I'm keeping it in
	 * for now just in case we need to put it back in for some reason.
	 */

	/* forwards didn't work; now go backwards (relative to mt_scandir) */
	for( m = msg;;) {

		if( mt_scandir > 0 ) {
			m = msg_methods.mm_prev(m);
		} else {
			m = msg_methods.mm_next(m);
		}

		if( ! m ) break;

		if (!m->mo_deleted &&
			(!consider_selection ||
			(consider_selection && m->mo_selected)))
		{
			if (mt_value("allowreversescan"))
				mt_scandir = -mt_scandir;
			return (m);
		}

	}
	return (NULL);

#endif /* GOBACKWARD */
}



void
mt_set_state_no_mail(
	struct header_data *hd,
	int	check_mbox
)
{
	/*
	 * Hack to allow close code to turn off new mail flag
	 */
	mt_set_state(hd, S_NOMAIL);

	/* This call will cause the empty tray to display if the inbox
	   is empty */
	if (check_mbox)
		mt_check_mail_box(hd, FALSE);
}


int
get_time(
	struct msg     *m
)
{
	register char	*s;
	int		val;
	int		mon, day, hours, minutes, seconds, year;
	static char	*months_of_year[12] = {
				"Jan", "Feb", "Mar", "Apr", "May", "Jun",
			 	"Jul", "Aug", "Sep", "Oct",  "Nov", "Dec" };


	if ((s = m->mo_unix_date) == NULL)
		return (0);
	s += 4;					/* skip over day of week */
	for (mon = 0; mon < 12; mon++)
		if (strncmp(s, months_of_year[mon], 3) == 0) 
			break;
	day = atoi(s + 3);
	hours = atoi(s + 7);
	minutes = atoi(s + 10);
#ifdef	SVR4
	if (s[12] != ':')
	{
		seconds = 0;			/* SVR4 does not use seconds */
		val = 13;
	}
	else
	{
		seconds = atoi(s + 13);		/* SunOS uses seconds */
		val = 16;
	}
	if (isdigit((u_char)s[val]))
		year = atoi(s + val) - 1970;
	else if (s = strchr(s + val, ' '))	/* skip the optional timezone */
		year = atoi(s + 1) - 1970;
	else
		year = 0;
#else
	seconds = atoi(s + 13);
	if (isdigit(s[16]))
		year = atoi(s + 16) - 1970;
	else if (s = strchr(s + 16, ' '))	/* skip the optional timezone */
		year = atoi(s + 1) - 1970;
	else
		year = 0;
#endif	SVR4

	/*
	 * convert to pseudo-seconds. For the purposes of sorting, all that
	 * is necessary is to make sure that the number monotonically
	 * increases with the date,not that the number be accurate 
	 */
	 val = seconds + (60 * (minutes - 1)) + (3600 * (hours - 1))
		+ (SEC_PER_DAY * (day - 1))
		+ (SEC_PER_DAY * 31 * mon)
		+ (SEC_PER_DAY * 31 * 12 * year); 
	return (val);
}




/*
 * Get the first non-deleted message.
 */
struct msg *
FIRST_NDMSG(
	struct folder_obj *folder
)
{
	struct msg *p_msg;

	if (folder == NULL) {
		return (NULL);
	}
	else {
		p_msg = folder->fo_first_msg;
		while ((p_msg != NULL) && p_msg->mo_deleted)
			p_msg = p_msg->mo_next;
		return (p_msg);
	}
}

/*
 * Get the last non-deleted message.
 */
struct msg *
LAST_NDMSG(
	struct folder_obj *folder
)
{
	struct msg *p_msg;

	if (folder == NULL) {
		return (NULL);
	} else {
		p_msg = folder->fo_last_msg;
		while (p_msg != NULL && p_msg->mo_deleted)
			p_msg = p_msg->mo_prev;
		return (p_msg);
	}
}

/*
 * Get the next non-deleted message.
 */
struct msg *
NEXT_NDMSG(
	struct msg *m
)
{
	if (m == NULL)
		return (NULL);
	while ((m = m->mo_next) != NULL) {
		if (!m->mo_deleted)
			break;
	}
	return (m);
}

/*
 * Get the previous non-deleted message.
 */
struct msg *
PREV_NDMSG(
	struct msg *m
)
{
	if (m == NULL)
		return (NULL);
	while ((m = m->mo_prev) != NULL) {
		if (!m->mo_deleted)
			break;
	}
	return (m);
}

void 
CLEAR_MSGS(
	struct header_data *hd
)
{
	mt_attach_clear_folder(hd);
	folder_methods.fm_free(CURRENT_FOLDER(hd));
	CURRENT_FOLDER(hd) = NULL;
	MT_CURMSG(hd) = NULL;
}

struct msg *
NUM_TO_MSG(
	struct header_data *hd,
	int num
)
{
	struct msg *m;

	for (m = msg_methods.mm_first(CURRENT_FOLDER(hd)); m != NULL;
	     m = msg_methods.mm_next(m))
	{
		if (m->mo_msg_number == num)
		{
			return(m);
		}
	}

	return (NULL);
}

/*
 * Translate a line number to a message.  Start at the top or bottom of
 * the list -- whatever is closer.
 */
struct msg *
LINE_TO_MSG(
	struct header_data *hd,
	int line
)
{
	register struct msg *m;
	struct folder_obj	*f;

	f = CURRENT_FOLDER(hd);

	if (f == NULL) return(NULL);

	if (line < f->fo_num_msgs / 2) {
		for (m = FIRST_NDMSG(f); m != NULL; m = NEXT_NDMSG(m)) {
			if (--line <= 0)
				break;
		}
	} else if ((m = LAST_NDMSG(f)) != NULL) {
		if ((line = m->m_lineno - line + 1) <= 0) {
			m = NULL;
		} else {
			for (; m != NULL; m = PREV_NDMSG(m)) {
				if (--line <= 0)
					break;
			}
		}
	}

	return (m);
}



void
nomem(void)
{
	struct header_data *hd;

	/* STRING_EXTRACTION -
	 *
	 * "%s: Out of memory" is printed when a malloc() call fails.
	 * the %s is the name of the mailtool program.
	 */
	(void)fprintf(stderr, gettext("%s: Out of memory!\n"), mt_cmdname);
	mt_stop_mail(0);
	mt_done(1);
}


static int
mt_text_error(
	Textsw win,
	int clear
)
{
	static int CLEAR_KEY;
	time_t last_time;
	time_t this_time;

	if (! CLEAR_KEY) {
		CLEAR_KEY = xv_unique_key();
	}

	if (clear) {
		xv_set(win, XV_KEY_DATA_REMOVE, CLEAR_KEY, 0);
		return (0);
	} else {
		last_time = (time_t) xv_get(win, XV_KEY_DATA, CLEAR_KEY);
		this_time = time(NULL);

		/* ZZZ:katin -- hard coded time constant of 30 seconds
		 * between warnings.  Don't warn more often than once
		 * every 30 seconds
		 */
		if (! last_time || last_time + 30 < this_time) {
			xv_set(win, XV_KEY_DATA, CLEAR_KEY, this_time, 0);
			return (0);
		}

		return (1);
	}
}

void
mt_text_clear_error(
	Textsw textsw
)
{
    (void) mt_text_error(textsw, 1);
}


static int
mt_text_set_error(
	Textsw textsw
)
{
	    if (! mt_text_error(textsw, 0)) {

		    /* STRING_EXTRACTION -
		     *
		     * This message is printed when textsw_insert doesn't
		     * add all the characters to the textsw.
		     */
		    mt_vs_warn(textsw, gettext(
"Warning: some characters in this message could not be\n\
displayed in this window.  This usually happens when\n\
the message contains characters that are not found in\n\
the character set of your current locale.  These characters\n\
will be skipped over and will not be displayed."));

	    }
}


void
mt_text_insert(
	Textsw textsw,
	char *data,
	int len
)
{

	int result;

	while (len >= 0) {
		result = textsw_insert(textsw, data, len);
		if (result == len) return;

		mt_text_set_error(textsw);


		/* skip over the offending character */
		data += result + 1;
		len -= result + 1;

	}
}
