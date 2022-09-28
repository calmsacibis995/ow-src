#ifndef lint
static 	char sccsid[] = "@(#)delete_log.c 3.9 94/06/21 Copyr 1991 Sun Micro";
#endif

/*
 * Copyright (c) 1991 by Sun Microsystems, Inc.
 */

/*
 * The following routines maintain mailtool's transaction log.  This log
 * is used so that we can recover the undelete list if mailtool is terminated
 * abnormally while reading the spool file.
 *
 * If the user every has 2 mailtools looking at the spool file at the same
 * time then all bets are off.
 */

#include <fcntl.h>
#include <string.h>
#include <stdio.h>
#include <sys/param.h>
#include "header.h"
#include "delete_log.h"
#include "tool_support.h"
#include "../maillib/obj.h"

#define DEBUG_FLAG mt_debugging
extern int DEBUG_FLAG;
#include "debug.h"

#define MT_MBOX_STRING		"mbox"		/* Specify mailbox */
#define MT_DELETE_STRING	"delete"	/* Delete operation */
#define MT_UNDELETE_STRING	"undelete"	/* Undelete operation */
#define MT_LOG_FILE		"~/.mtdeletelog" /* Location of log file */

static FILE	*log_fp;			/* File pointer for log file */

static FILE *create_transaction_log(char *path, char *folder_path);



/*
 * Log a delete/undelete transaction.  If the log file does not exist
 * this will create it and open it.
 */
void
mt_log_transaction(
	struct folder_obj *folder,
	struct msg	*m,
	int		delete
)
{

	if (log_fp == NULL) {
		char path[MAXPATHLEN + 1];
		ds_expand_pathname(MT_LOG_FILE, path);
		log_fp = create_transaction_log(path, folder->fo_file_name);
	}

	save_to_transaction_log(log_fp, m, delete);

	return;
}




/*
 * Close and remove the transaction log
 */
void
mt_clear_transaction_log(
	void
)
{
	char	path[MAXPATHLEN + 1];

	if(log_fp != NULL) {
		fclose(log_fp);
		log_fp = NULL;
	}

	ds_expand_pathname(MT_LOG_FILE, path);
	unlink(path);
}





/*
 * Flush pending writes to the transaction log
 */
void
mt_flush_transaction_log(
	void
)
{
	DP(("Flushing transaction log\n"));

	if (log_fp)
		fflush(log_fp);
}





/*
 * Process the transaction log file.  This should be called the first
 * time we load in the spool file.
 *
 * Returns 0 if all is well
 *
 * Returns -1 if couldn't find a message specified by the file, or if the
 * file was otherwise corrupted.  Clears all selections.
 *
 * In either case this routine removes the file after it's done with it.
 */
void
mt_process_transaction_log(
	struct header_data	*hd
)
{
	FILE	*fp;
	int	rcode;
	char	path[MAXPATHLEN + 1];

	/* Open file and unlink it */
	ds_expand_pathname(MT_LOG_FILE, path);
	if ((fp = fopen(path, "r")) == NULL) {
		return;
	}


	DP(("Undelete file found. Selecting messages for deletion\n"));

	/* Selected all messages specified in file and delete them */
	if ((rcode = select_from_log(hd, fp)) >= 0) {
		/*
	 	 * It is important that we discard the old log before
		 * deleting messages since the first delete will create a
		 * new file and write into it.
	 	 */
		mt_clear_transaction_log();
		DP(("Deleting messages\n"));
		mt_do_del(hd, MSG_SELECTED);
	} else {
		/* File is hosed.  Nuke it */
		mt_clear_transaction_log();
	}

	fclose(fp);
}





/*
 * Select all messages specified by the transaction log specified by fp.
 *
 * Returns 0 if all went well
 *
 * Returns -1 if couldn't find a message specified by the log, or if the
 * log is otherwise corrupted.  Clears all selections on an error.
 */
static int
select_from_log(
	struct header_data	*hd,
	FILE			*fp
)
{
	char	line_buf[BUFSIZ];
	int	msgno;
	int	select;
	char	*msgdate;
	char	*msgid;
	char	*s;
	char	*seperators = "^\n";
	struct msg *m, *find_msg();

	/* Clear all selections */
	mt_clear_selected_glyphs(hd);
	
	/*
	 * Parse file. No this isn't the most efficient way to do this,
	 * but since this code only executes on an exception we trade off 
	 * speed for simplicity
	 */
	while (fgets(line_buf, BUFSIZ, fp)) {

		/* Skip comments and blank lines */
		if (*line_buf == '#' || *line_buf == '\n')
			continue;

		/* Format of the line should be "operation msgno date msgid" ie:
   delete^127^Tue Apr 9 15:34:41 1991^<9102270331.AA06492@skylark.Eng.Sun.COM>
		 */

		/* Get operation */
		if ((s = strtok(line_buf, seperators)) == NULL)
			goto ERROR_EXIT;

		/* If the operation is delete, then we want to select the
		 * message.  If it is undelete then we want to de-select the
		 * message.
		 */
		if (strcmp(s, MT_DELETE_STRING) == 0)
			select = 1;
		else if (strcmp(s, MT_UNDELETE_STRING) == 0)
			select = 0;
		else if (strcmp(s, MT_MBOX_STRING) == 0) {
			/* Mailbox operator. Get mbox name */
			if ((s = strtok(NULL, seperators)) == NULL)
				goto ERROR_EXIT;

			if (strcmp(s, hd->hd_folder->fo_file_name) == 0)
				continue;	/* Log is for this folder */
			else
				goto ERROR_EXIT;
		} else
			goto ERROR_EXIT;	/* Unknown operator */

		/* Get message number */
		if ((s = strtok(NULL, seperators)) == NULL)
			goto ERROR_EXIT;

		msgno = atoi(s);

		/* Get message date */
		if ((s = strtok(NULL, seperators)) == NULL)
			goto ERROR_EXIT;

		/* '*' means message has no date */
		if (*s == '*' && *(s + 1) == '\0')
			msgdate = NULL;
		else
			msgdate = s;

		/* Get message id */
		if ((s = strtok(NULL, seperators)) == NULL)
			goto ERROR_EXIT;

		/* '*' means message has no id */
		if (*s == '*' && *(s + 1) == '\0')
			msgid = NULL;
		else
			msgid = s;

		/* Find message */
		m = find_msg(hd->hd_folder, msgno, msgdate, msgid);
		if (m == NULL) {
			/* 
			 * Couldn't find this message.  Assume undelete file is
			 * corrupted. Clear all selections.
			 */
			 goto ERROR_EXIT;
		}

		/* Found message.  Selected it */
		m->mo_selected = select;

		DP(("Found and %s message %d %s\n",
		     select ? "selected" : "deselected",
		     msgno,
		     msgid ? msgid : "<null msgid>"));
	}

	return(0);

ERROR_EXIT:
	/* Something is wrong with the file.  De-select everything */
	DP(("File is hosed. Clearing selections\n"));
	mt_clear_selected_glyphs(hd);
	return(-1);
}




/*
 * Search through all messages looking for one which matches a specified
 * message number, message date and message id.
 *
 * Message id is the value of the "Message-Id" header field
 */
static struct msg *
find_msg(
	struct folder_obj	*folder,
	int			msgno,
	char			*msgdate,
	char			*msgid
)
{
	struct msg	*m;
	char		*msgid_field = NULL;

	/* Loop through all messages in a folder */
	for (m = msg_methods.mm_first(folder); m != NULL;
	     m = msg_methods.mm_next(m)) {

		/*
		 * Check if the message number matches. If the specified
		 * message number is < 0 then don't check it.
		 */
		if (msgno >= 0 && msgno != m->mo_msg_number)
			continue;

		/* Check date.  First case is if no date. */
		if (msgdate == NULL) {
			if (m->mo_unix_date == NULL ||
			    *(m->mo_unix_date) == '\0') {
				break;	/* No date.  Match */
			} else {
				continue;
			}
		}

		if (strcmp(msgdate, m->mo_unix_date) != 0)
			continue;

		/*
		 * We've matched the message number and date, now check
		 * the message id
		 * This call will allocate the string which we must free.  We
		 * should only hit this case once per call so it ain't that bad
		 */
		msgid_field = msg_methods.mm_get(m, MSG_HEADER, "message-id");

		/* Check for lack of message id */
		if (msgid == NULL) {
			if ((msgid_field == NULL || *msgid_field == '\0')) {
				break; /* Match */
			} else {
				free(msgid_field);
				continue;
			}
		}

		if (msgid_field != NULL && strcmp(msgid, msgid_field) == 0)
			break;	/* Match */

		if (msgid_field != NULL)
			free(msgid_field);
	}

	if (msgid_field != NULL)
		free(msgid_field);

	return (m);
}




/*
 * Create a new log file.
 */
static FILE *
create_transaction_log(
	char	*path,
	char	*folder_path
)
{

	int fd;
	FILE	*fp;

	/* Open file. This will fail if file exists */
	if ((fd = open(path, O_EXCL | O_CREAT | O_WRONLY, 0600)) < 0)
		return(NULL);

	if ((fp = fdopen(fd, "w")) == NULL)
		return(NULL);

	fputs("#\n# Mailtool delete transaction log\n#\n", fp);
	fputs(MT_MBOX_STRING, fp);
	fputs("^", fp);
	fputs(folder_path, fp);
	fputs("\n", fp);

	return(fp);
}





/*
 * Write a transation to the log file. 
 */
static
save_to_transaction_log(
	FILE		*fp,
	struct msg	*m,
	int		delete
)
{
	static int	flush_count;
	char	*linebuf;
	char	*msgid_field = NULL;
	char	*opstring;
	char	buf[BUFSIZ];

	if (fp == NULL)
		return(-1);

	/* Get the msgid for the specified message */
	msgid_field = msg_methods.mm_get(m, MSG_HEADER, "message-id");

	/* Get operation */
	if (delete)
		opstring = MT_DELETE_STRING;
	else
		opstring = MT_UNDELETE_STRING;
	
	/* Format of the line should be "operation msgdate msglen msgid" ie:
	 */
	sprintf(buf, "%d", m->mo_msg_number);
	linebuf = (char *) ck_malloc(strlen(opstring)
				+ strlen(buf)
				+ (m->mo_unix_date ? strlen(m->mo_unix_date) : 1)
				+ (msgid_field ? strlen(msgid_field) : 1)
				+ 5);
	sprintf(linebuf, "%s^%d^%s^%s\n", opstring, m->mo_msg_number,
		(m->mo_unix_date == NULL ||
		*m->mo_unix_date == '\0') ? "*" : m->mo_unix_date,
		(msgid_field == NULL ||
		*msgid_field == '\0') ? "*" : msgid_field);

	if (msgid_field != NULL)
		free(msgid_field);

	/* Put a null terminator at BUFSIZ-1 if the string is greater
	 * than or equal to BUFSIZ so that the string that 
	 * we save isn't greater than BUFSIZ, which would cause 
	 * select_from_log() to break. 
	 */
	if (strlen(linebuf) >= (size_t) BUFSIZ) {
		linebuf[BUFSIZ-1] = '\0';
	}
	fputs(linebuf, fp);
	ck_free(linebuf);

	/*
	 * Make sure we flush out the data at least once every 5 transactions
	 * We also try to flush once a minute (see mt_itimer()).
	 */
	if (++flush_count >= 5) {
		flush_count = 0;
		mt_flush_transaction_log();
	}

	return(m->mo_msg_number);
}
