#ident "@(#)submission.c	3.18 05/23/94 Copyright 1987-1991 Sun Microsystems, Inc."

/*
 *  Copyright (c) 1987-1991 Sun Microsystems, Inc.
 *  All Rights Reserved.
 *
 *  Sun considers its source code as an unpublished, proprietary
 *  trade secret, and it is available only under strict license
 *  provisions.  This copyright notice is placed here only to protect
 *  Sun in the event the source is deemed a published work.  Dissassembly,
 *  decompilation, or other means of reducing the object code to human
 *  readable form is prohibited by the license agreement under which
 *  this code is provided to the user or company in possession of this
 *  copy.
 *
 *  RESTRICTED RIGHTS LEGEND: Use, duplication, or disclosure by the
 *  Government is subject to restrictions as set forth in subparagraph
 *  (c)(1)(ii) of the Rights in Technical Data and Computer Software
 *  clause at DFARS 52.227-7013 and in similar clauses in the FAR and
 *  NASA FAR Supplement.
 */ 

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/param.h>
#include <sys/mman.h>
#include <time.h>
#include <sys/stat.h>
#include <signal.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <unistd.h>
#include <pwd.h>
#include "folder.h"
#include "msg.h"
#include "misc.h"
#include "attach.h"
#include "submission.h"
#include "charset.h"
#include "hash.h"
#include "global.h"
#include "ck_strings.h"
#include "bool.h"
#ifdef PEM
#include "/home/internet/pem-5.0/src/h/pem.h"
#endif PEM

#define	DEBUG_FLAG	mt_debugging
extern DEBUG_FLAG;
#include "debug.h"

extern	char	*dgettext();

#define	MAXEXP		25

static struct submission *sub_create(char *);
static int	sub_set(struct submission *, Sub_attr, ...);
static void	*sub_get(struct submission *, Sub_attr, ...);
static Sub_error sub_done(struct submission *, struct msg *);
static void	sub_destroy(struct submission *);
extern struct link_obj *link_add();
extern struct link_obj *link_find();
extern int	link_free();
extern int	link_enumerate();
extern int	header_write();
extern char	*mt_value();
extern char	*skin_arpa();
extern char	*findend_of_field();
extern struct header_obj *header_set();
extern int	do_get_size();
extern int	do_buffer_write();

struct __submission_methods submission_methods = {
	sub_create,
	sub_set,
	sub_get,
	sub_done,
	sub_destroy,
};

extern char	DO_UUENCODE[];
extern char	Msg_Content_Type[];
extern char	Msg_MIME_Version[];

/* these are the characters that force an addressee to be treated
 * as a remote mail address instead of a file name
 */
static char metanet[] = "!^:%@";
static int isfileaddr();


static struct submission *
sub_create(char *errfile)
{
	struct submission *sub;

	if (errfile == NULL)
		return (NULL);

	sub = (struct submission *) malloc (sizeof(struct submission));
	if (sub == NULL) {
		goto end;
	}
	memset ((char *) sub, 0, sizeof(*sub));

	sub->sub_errfile = strdup (errfile);
	if (! sub->sub_errfile) {
		free(sub);
		sub = NULL;
		goto end;
	}

	/* we can do alot more initialization here if we want to */
	DP(("sub_create: sub %x, pid %d\n", sub, getpid()));

end:
	return (sub);
}

static int
namelen (p_link, p_len)
struct link_obj *p_link;
int *p_len;
{
	/* two extra bytes for the ", " after current name */
	*p_len += strlen (p_link->l_value) + 2;
	return (0);
}

static int
addname (p_link, p_nmlist)
struct link_obj *p_link;
char *p_nmlist;
{
	strcat (p_nmlist, p_link->l_value);
	strcat (p_nmlist, ", ");
	return (0);
}

/*
 * Make the final recipient name list which each name is separated by ", ".
 */
char *
make_nmlist (lo)
struct link_obj *lo;
{
	int len = 0;
	char *nmlist;

	/* get the total length of all names which will be separated by ", " */
	link_enumerate (lo, namelen, &len);
	nmlist = ck_malloc (len+1);
	*nmlist = '\0';

	/* construct the final name list; eliminate the last ", " */
	link_enumerate (lo, addname, nmlist);
	nmlist[len-2] = '\0';

	return (nmlist);
}

#ifdef	SENDMAIL
/*
 * This function expands the path; it is stolen from ds_expand_pathname().
 */
static void
expand_pathname (path, buf)
char *path;
char *buf;
{
	register u_char *p, *b_p, *e_p;	/* String pointers */
	u_char *save_p;			/* Point in path before env var */
	u_char env[255];		/* Environment variable expansion */
	struct passwd *pw;		/* Password file entry */

	p = (u_char *) path;
	if (*p == '~')
	{
		p++;
		if (*p && *p != '/')
		{
			/* Somebody else's home directory? */
			if (b_p = (u_char *) strchr((char *) p, '/'))
				*b_p = '\0';
			if (pw = getpwnam((char *) p)) {
				(void) strcpy(buf, pw->pw_dir);
			} else {
				*buf = '~';
				(void) strcpy(buf + 1, (char *) p);
			}

			if (b_p)  {
				*b_p = '/';
				p = b_p;
			} else
				return;

		}
		else
			(void) strcpy(buf, getenv("HOME"));
		b_p = (u_char *) buf + strlen(buf);
	}
	else
		b_p = (u_char *) buf;

	while (*p)
		if (*p == '$')
		{
			/* Expand environment variable */
			save_p = p;
			e_p = env;
			p++;
			while ((isalnum(*p) || *p == '_') && e_p - env < 255)
				*e_p++ = *p++;
			*e_p = NULL;
			if (e_p = (u_char *) getenv((char *)env))
				while (*e_p)
					*b_p++ = *e_p++;
			else
			{
				p = save_p;
				*b_p++ = *p++;
			}
		}
		else 
			*b_p++ = *p++;
	*b_p = NULL;
}

/*
 * This function expands the folder name which MUST start with '+'.
 */
static void
expand_folder (path, fullpath)
char *path;
char *fullpath;
{
	char    buf[MAXPATHLEN];
	char    *folder;

	*fullpath = '\0';
	if ((folder = mt_value("folder")) == NULL)
	{
		/* folder not specified default to $HOME */
		strcpy (fullpath, getenv("HOME"));
	}
	else
	{
		/* Expand ~s, $VARS, etc */
		expand_pathname (folder, fullpath);
		if (*fullpath != '/')
		{
			sprintf (buf, "%s/%s", getenv("HOME"), fullpath);
			strcpy (fullpath, buf);
		}
	}
	strcat (fullpath, "/");
	strcat (fullpath, ++path);
}

/*
 * Get a name token from an address line
 */
char *
gettoken (p_buf, p_token, len)
register u_char *p_buf;
register u_char *p_token;
register int len;
{
	int	o_len;
	int	dquote;
	register int stop_at_del;
	register int nparan;
	register u_char c;

	while ((*p_buf == ',') || isspace (*p_buf))
		p_buf++;

	dquote = 0;
	nparan = 0;
	stop_at_del = 1;

	/* leave one byte for the null-terminator */
	o_len = --len;
	while ((c = *p_buf++) != '\0')
	{
		/* ignore delimiters between a pair of '"', but
		 * keep the double-quote in the token because sendmail
		 * needs it for e-mail address like "first,last"@host.
		 */
		if (c == '"')
		{
			stop_at_del = dquote;
			dquote = !dquote;
		}

		/* skip RFC 822's () */
		if ((c == '(') && ++nparan)
			continue;
		if ((c == ')') && nparan--)
			continue;
		if (nparan > 0)
			continue;

		if (stop_at_del && ((c == ',') || isspace(c)))
			break;
		if (--len >= 0)
			*p_token++ = c;
		else
		{
			*p_token = '\0';
			fprintf (stderr, "Token <%s> exceeds the max length\n",
				p_token - o_len);
			return ((char *) NULL);
		}
	}
	*p_token = '\0';

	if ((c == '\0') && (o_len == len))
		return ((char *) NULL);
	else
	{
		if (c == '\0') --p_buf;
		return ((char *)p_buf);
	}
}

/*
 * Recursively expand the address name.  We limit the expansion to some
 * fixed level to keep things from going haywire.
 */

static struct link_obj *
expand_address (sub, rclist, names)
struct submission *sub;
struct link_obj **rclist;
char *names;
{
	char *name;
	char *nmlist;
	char *newnames;
	char token[BUFSIZ];
	static int depth;

	/* strip off RFC 822 information: () comments, <>, etc */
	names = skin_arpa (names);
	if ((newnames = names) == NULL)
		return (*rclist);

	if (depth > MAXEXP) {
		maillib_methods.ml_warn (NULL,
			dgettext("SUNW_DESKSET_MAILLIB",
			"Expanding alias to depth larger than %d\n"),
			MAXEXP);
		return (*rclist);
	}
	depth++;
	while (names = gettoken (names, token, sizeof(token)))
	{
		if (token[0] == '\0')
			continue;

		if (token[0] == '|')
		{
			if (token[1] != '\0')
			{
				/* token is "|program", skip the pipe char */
				name = token + 1;
			}
			else
			{
				/* format was "| program", get program path */
				names = gettoken (names, token, sizeof(token));
				if (names == NULL)
					break;
				if (token[0] == '\0')
					continue;
				name = token;
			}

			/* allocate the program name for the link list entry */
			name = ck_strdup (name);
			if (link_add (&sub->sub_prog, name, strcmp) == NULL)
				ck_free (name);
		}
		else if (isfileaddr(token))
		{
			if (token[0] == '+')
			{
				name = ck_strdup (token);
				(void) expand_folder (name, token);
				ck_free (name);
			}

			/* allocate recording folder for the link list entry */
			name = ck_strdup (token);
			if (link_add (&sub->sub_rec, name, strcmp) == NULL)
			{
				/* can't add to link list because of duplicated
				 * folder; free the memory
				 */
				ck_free (name);
			}
		}
		else
		{
			/* expand the alias */
			nmlist = hash_method.hm_test (glob.g_alias, token);

			DP(("alias expansion: %s \"%s\"\n", token,
				nmlist ? nmlist : "<NULL>"));

			if (nmlist != NULL)
				*rclist = expand_address (sub, rclist, nmlist);
			else
			{
				/* check if the recipient name is duplicated */
				if (link_find (sub->sub_to, token, strcmp))
					continue;
				if (link_find (sub->sub_cc, token, strcmp))
					continue;
				if (link_find (sub->sub_bcc, token, strcmp))
					continue;

				name = ck_strdup (token);
				if (link_add (rclist, name, NULL) == NULL)
				{
					/* Can't add to link list; free name */
					ck_free (name);
				}
			}
		}
	}

	if (newnames)
		ck_free (newnames);

	/* may return the new list */
	depth--;
	return (*rclist);
}

/*
 * Save the outgoing mail on the passed file.
 */
static int
savemail(fi, dummy, fo)
	FILE *fi;
	void *dummy;
	FILE *fo;
{
	time_t now;
	char line[BUFSIZ];
#ifndef	FROM_STUFF
	extern char Msg_Content_Len[];
	extern char Msg_Lines[];
	struct stat statbuf;
	int neednl;
	long offset;
	register int first_nl = 1;
	register int nlines = 0;
#endif	FROM_STUFF


	(void) time(&now);
	fprintf(fo, "From %s %s", mt_value ("USER"), ctime(&now));

	/* check if the last line in message is terminated by LF */
	fseek(fi, -1L, 2);
	neednl = (fgetc(fi) != '\n') ? 1 : 0;

	rewind(fi);

	while (fgets(line, sizeof(line), fi) != NULL) {
#ifndef	FROM_STUFF
		if (!first_nl)
			nlines++;

		/* when no "From " stuffing is used, insert content length */
		if (first_nl && line[0] == '\n') {
			fstat (fileno(fi), &statbuf);
			fprintf (fo, "%s: %d\n", Msg_Content_Len,
				statbuf.st_size - ftell(fi) + neednl);

			/* Add X-Lines: header to line counts */
			fprintf (fo, "%s: %10.10s\n", Msg_Lines, "");
			offset = ftell(fo) - 11;

			first_nl = 0;
		}
#else
		if (strncmp(line, "From ", 5) == 0)
			fputc('>', fo);		/* hide any From lines */
#endif	FROM_STUFF

		fputs(line, fo);
	}

	/* output 1 or 2 LF's at the end of the message */
	do {
		fputc ('\n', fo);
	} while (--neednl >= 0);

	/* update the line count */
	if (!first_nl)
	{
		fseek (fo, offset, 0);
		fprintf (fo, "%d", nlines);
	}

	fflush(fo);
	if (ferror(fo) || ferror(fi)) {
		return (1);
	}

	return (0);

}

static int
lsavemail(struct link_obj *p_obj, void *fp)
{
	int retcode;

	if (folder_methods.fm_add(p_obj->l_value, savemail, fp, (void *) 0)) {

		/*
		 * STRING_EXTRACTION SUNW_DESKSET_MAILLIB
		 *
		 * This message is printed when an "address" on a message
		 * is really a file name (either because the address
		 * started with a '/' or a '+'), and we tried to save
		 * to the file and failed.
		 */
		maillib_methods.ml_warn(NULL,
			dgettext("SUNW_DESKSET_MAILLIB",
"Error while sending mail to file %s.\n\
The error was: %s\n"),
			p_obj->l_value, strerror(errno));
		return (1);
	}
	return (0);
}

static
lexec(struct link_obj *p_obj, FILE *fi)
{
	static char *Shell;		/* default is NULL */
	int fd;

	if ((Shell == NULL) && (Shell = mt_value ("SHELL")) == NULL)
		Shell = "/bin/sh";

	/* it is same as stdin */
	rewind (fi);

	switch(vfork()) {
	case 0:
		dup2 (fileno(fi), 0);
		for (fd = (int)sysconf(_SC_OPEN_MAX); --fd > 2; )
			(void)close(fd);

		/* child */
		execlp (Shell, Shell, "-c", (char *) p_obj->l_value, 0);
		/* shouldn't ever be reached... */
		exit(1);

		/*NOTREACHED*/

	case -1:
		/* error */
		return (1);

	default:
		return (0);
	}
	/*NOTREACHED*/
}

static void
expand_recipient (sub, msg, attr)
struct submission *sub;
struct msg *msg;
char *attr;
{
	char *nmlist;
	struct link_obj *newlist;
	struct link_obj **rclist;

	if (strcasecmp (attr, "to") == 0)
		rclist = &sub->sub_to;
	else if (strcasecmp (attr, "cc") == 0)
		rclist = &sub->sub_cc;
	else if (strcasecmp (attr, "bcc") == 0)
		rclist = &sub->sub_bcc;
	else
		return;

	nmlist = msg_methods.mm_get (msg, MSG_HEADER, attr);
	if (nmlist == NULL)
		return;

	/* The "newlist" is a modified "*rclist" which may become NULL
	 * if all entries are duplicated.
	 */
	newlist = (struct link_obj *) expand_address (sub, rclist, nmlist);
	if (newlist == NULL)
		header_delete (&msg->mo_hdr, attr, strlen(attr));
	else
	{
		ck_free (nmlist);
		nmlist = make_nmlist (newlist);
		/* replace the old header */
		header_set (&msg->mo_hdr, attr, nmlist);
	}

	ck_free (nmlist);
}

static void
save_dead (sub, fi)
struct submission *sub;
FILE *fi;
{
	int	n;
	char	*s;
	FILE	*fo;
	char	buf[BUFSIZ];

	if ((s = sub->sub_deadletter) == NULL)
		return;
	fo = fopen (s, "w");
	if (fo != NULL)
	{
		rewind (fi);
		while ((n = fread (buf, 1, sizeof(buf), fi)) > 0)
			fwrite (buf, n, 1, fo);
		fclose (fo);
	}
}

static int
lcount (p_obj, p_count)
struct link_obj *p_obj;
int *p_count;
{
	(*p_count)++;
	return (0);
}

static int
lset (p_obj, p_argv)
struct link_obj *p_obj;
char ***p_argv;
{
	**p_argv = (char *) p_obj->l_value;
	(*p_argv)++;
	return (0);
}

static char **
unpack(sub)
struct submission *sub;
{
	char	**args;
	char	*metoo;
	char	*verbose;
	int	extra = 0;
	register int	argc;
	register char	**argv;

	/* Compute the number of extra arguments we will need.  We need at
	 * least three extra -- one for "mail", one for "-i", and one for the
	 * terminating NULL pointer.  Additional spots may be needed
	 * to pass along -r and -f to the host mailer.
	 */
	link_enumerate(sub->sub_to, lcount, &extra);
	link_enumerate(sub->sub_cc, lcount, &extra);
	link_enumerate(sub->sub_bcc, lcount, &extra);
	if ((argc = extra) == 0)
		return (NULL);
	argc += 3;
	if (metoo = mt_value("metoo"))
		argc++;
	if (verbose = mt_value("verbose"))
		argc++;
	argv = (char **) ck_malloc(argc * sizeof(char *));

	argc = 0;
	argv[argc++] = "send-mail";
	argv[argc++] = "-i";

	if (metoo)
		argv[argc++] = "-m";

	if (verbose)
		argv[argc++] = "-v";

	args = &argv[argc];
	link_enumerate(sub->sub_to, lset, &args);
	link_enumerate(sub->sub_cc, lset, &args);
	link_enumerate(sub->sub_bcc, lset, &args);
	*args = NULL;

	return(argv);
}
#endif	SENDMAIL


static int
sub_set (struct submission *sub, Sub_attr attr, ...)
{
	va_list ap;
	char *string;

	va_start(ap, attr);

	switch (attr)
	{
	case SUB_RECORD:
		string = va_arg(ap, char *);
		if (string != NULL)
		{
			string = ck_strdup(string);
			link_add (&sub->sub_rec, string, strcmp);
		}
		break;

	case SUB_TMPFILE:
		string = va_arg(ap, char *);
		if (string != NULL)
		{
			ck_free(sub->sub_tmpfile);
			sub->sub_tmpfile = ck_strdup(string);
		}
		break;

	case SUB_WAIT:
		sub->sub_wait = va_arg(ap, int);
		break;

	case SUB_DEADLETTER:
		string = va_arg(ap, char *);
		if (string != NULL)
		{
			ck_free (sub->sub_deadletter);
			sub->sub_deadletter = ck_strdup(string);
		}
		break;

#ifdef PEM
	/* set up PEM encryption options */
	case SUB_PEM_NONE:
		sub->sub_pem_flags = NO_FLAG;
		break;

	case SUB_PEM_INT:
		sub->sub_pem_flags = INT_FLAG + AUTH_FLAG;
		break;
	
	case SUB_PEM_CONF:
		sub->sub_pem_flags = CONF_FLAG;
		break;
	
	case SUB_PEM_PRIV:
		sub->sub_pem_flags = INT_FLAG + AUTH_FLAG + CONF_FLAG;
		break;
#endif PEM

	default:
		return (-1);
	}

	return (0);
}

static void *
sub_get(struct submission *sub, Sub_attr attr, ...)
{
	va_list ap;

	va_start(ap, attr);

	switch (attr)
	{
	case SUB_RECORD:
		return ((void *) sub->sub_rec);
	case SUB_TMPFILE:
		return ((void *) sub->sub_tmpfile);
	case SUB_WAIT:
		return ((void *) sub->sub_wait);
	case SUB_DEADLETTER:
		return ((void *) sub->sub_deadletter);
	case SUB_PID:
		return ((void *) sub->sub_pid);
	case SUB_ERROR:
		return ((void *) sub->sub_error);
	default:
		return ((void *) NULL);
	}
}

/*
 * Construct the file which contains all the body parts, and is ready to send
 * to sendmail.
 */
static Sub_error
compose_msg(FILE *ofp, struct submission *sub, struct msg *msg)
{
	int	ecode;
#ifdef	RFC_MIME
	char	*buf;
	int	size;
#endif	RFC_MIME

	if (msg == NULL) {
		return (SUB_INTERNAL);
	}

#ifdef	SENDMAIL
	expand_recipient (sub, msg, "To");
	expand_recipient (sub, msg, "Cc");
	expand_recipient (sub, msg, "Bcc");
#endif	SENDMAIL

	DP(("compose_msg: mo_format %d\n", msg->mo_format));

	if (msg_methods.mm_get(msg, MSG_NUM_PARTS) == 0)
	{

		void *info;

		/* Not a multipart or attachment */
#ifdef	RFC_MIME
		if (!(bool) maillib_methods.ml_get(ML_SENDV3))
		{
			struct attach *at;

			/* Send plain text in RFC format. */
			header_set(&msg->mo_hdr, Msg_MIME_Version,
					ATTACH_VERSION);

			at = attach_methods.at_create();
			if (! at) {
				return (SUB_NO_SWAP);
			}

			attach_methods.at_set(at, ATTACH_DATA_CLASS,
						ATTACH_TEXT_TYPE);
			attach_methods.at_set(at, ATTACH_DATA_SUBCLASS,
						ATTACH_PLAIN_SUBTYPE);
			attach_methods.at_set(at, ATTACH_BODY,
						msg->mo_body_ptr);
			attach_methods.at_set(at, ATTACH_CONTENT_LEN,
						msg->mo_len);

			ecode = header_write(ofp, msg->mo_hdr, FALSE);
			if (ecode) {
				attach_methods.at_destroy(at);
				return (SUB_NO_TMP_SPACE);
			}

			/* We are faking it */
			msg->mo_format = MSG_OTHER;
			attach_methods.at_set(at, ATTACH_MSG, msg);

			ecode = attach_methods.at_save(at, 
				msg_methods.mm_write_bytes, ofp,
				AT_SAVE_SUBMIT|AT_SAVE_NOMARKER);

			attach_methods.at_destroy(at);

			if (ecode) {
				return (SUB_NO_TMP_SPACE);
			}
			msg->mo_format = MSG_TEXT;
		}
		else
#endif	RFC_MIME
		{

			/* search the message to see if it contains any
			 * eight bit characters.  If not, label it as us-ascii.
			 */
	
			DP(("compose_msg: before cs_msg_label\n"));
			ecode = cs_methods.cs_msg_label(msg, &info);
			DP(("compose_msg: cs_msg_label was %d, info %x\n",
				ecode, info));
			if (ecode) return (SUB_CHARSET);
	
			/* if it is not an attachment, just write out
			 * all headers.  User may include any customerized
			 * content-type.  If you specify "attachment", you
			 * may screw up the recipient!
			 */
			if (header_write(ofp, msg->mo_hdr, TRUE)) {
				return (SUB_NO_TMP_SPACE);
			}
	
			DP(("compose_msg: before write_bytes\n"));
			if (info) {
				ecode = cs_methods.cs_copy(info,
					msg_methods.mm_write_bytes,
					msg->mo_body_ptr, msg->mo_len, (int) ofp);
			} else {
				ecode = msg_methods.mm_write_bytes(
					msg->mo_body_ptr, msg->mo_len, ofp);
			}
			if (ecode) {
				return (SUB_NO_TMP_SPACE);
			}
	

			/* if the last line in the message is not terminated
			 * by a LF, append one.  Don't rely on sendmail to do
			 * so.
			 */
			if ((msg->mo_len > 0) &&
			    (msg->mo_body_ptr[msg->mo_len-1] != '\n'))
			{
				ecode = msg_methods.mm_write_bytes("\n",1, ofp);
				if (ecode) {
					return (SUB_NO_TMP_SPACE);
				}
			}
		}
	}
	else
	{
		/* if it is an attachment, always overwrite any existing
		 * content-type because we don't trust it.
		 */
#ifdef	RFC_MIME
		bool	sendv3;

		if (msg->mo_type)
		{
			ck_free (msg->mo_type);
			msg->mo_type = NULL;
		}
		if (sendv3 = (bool) maillib_methods.ml_get(ML_SENDV3))
		{
			if (msg->mo_parameter)
			{
				ck_free (msg->mo_parameter);
				msg->mo_parameter = NULL;
			}
			msg->mo_type = ck_strdup (ATTACHMENT_TYPE);
			msg->mo_format = MSG_ATTACHMENT;
		}
		else
		{
			/* generate the magic boundary cookie set.  */
			msg->mo_format = MSG_MULTIPART;
			msg_methods.mm_set(msg, MSG_BOUNDARY_MARKER, 0);
			msg->mo_type = ck_strdup (ATTACH_MULTIPART_TYPE);
			msg->mo_subtype = ck_strdup (ATTACH_MIXED_SUBTYPE);
		}

		size = 1;
		ecode = co_methods.co_write(&msg->mo_ctype,
			co_methods.co_size, &size);
		if (ecode != 0) {
			return (SUB_INTERNAL);
		}

		buf = malloc(size);
		if (buf == NULL) return (SUB_NO_SWAP);
		buf[0] = '\0';

		ecode = co_methods.co_write(&msg->mo_ctype,
			co_methods.co_concat, buf);
		if (ecode != 0) {
			free(buf);
			return (SUB_NO_TMP_SPACE);
		}

		header_set (&msg->mo_hdr, Msg_Content_Type, buf);
		if (!sendv3)
		{
			header_set(&msg->mo_hdr, Msg_MIME_Version,
					ATTACH_VERSION);
		}

		/* For multipart, we don't need to write out the blank line
		 * after the msg header because the boundary marker has a
		 * leading blank line.  Otherwise, we need a blank line.
		 */
		ecode = header_write (ofp, msg->mo_hdr, sendv3 ? TRUE : FALSE);
		if (ecode) {
			return (SUB_NO_TMP_SPACE);
		}
#else
		header_set (&msg->mo_hdr, Msg_Content_Type, ATTACHMENT_TYPE);
		ecode = header_write (ofp, msg->mo_hdr, TRUE);
		if (ecode) {
			return (SUB_NO_TMP_SPACE);
		}
#endif	RFC_MIME


		DP(("compose_msg: just before enum of at_save\n"));

		ecode = msg_methods.mm_enumerate(msg, attach_methods.at_save,
			 (int) msg_methods.mm_write_bytes,
			 (int) ofp, AT_SAVE_SUBMIT, 0);
		if (ecode != 0) {
			return (SUB_NO_TMP_SPACE);
		}
#ifdef	RFC_MIME
		if (!sendv3)
		{
			/* Add the optional ending boundary marker */
			ecode = attach_methods.at_copy (
					attach_methods.at_first (msg),
					AT_END_MARKER,
					msg_methods.mm_write_bytes, ofp);
			if (ecode != 0) {
				return (SUB_NO_TMP_SPACE);
			}
		}
#endif	RFC_MIME
	}

	if (fflush (ofp) != 0) {
		return (SUB_NO_TMP_SPACE);
	}
	return (ecode);
}

#ifdef	SENDMAIL
#endif	SENDMAIL

static Sub_error
sub_done(struct submission *sub, struct msg *msg)
{
	int	fd;
	int	ecode;
	FILE	*fp;
	char	*sendmail;
	char	**argv;
	Sub_error error_code;
#ifdef PEM
	int	fdencrypt;
	int	options;
	char	*pemfile;
	FILE	*fPEM;
#endif PEM

	/* make sure pid is cleared out */
	sub->sub_pid = 0;

	/* Create the temporary file to hold the composed message;
	 * we can't pipe to sendmail directly because of recording
	 * and +folder features.
	 */
	if (sub->sub_tmpfile != NULL)
	{
		fd = open (sub->sub_tmpfile, O_CREAT|O_RDWR|O_TRUNC,
			   0600);
	}
	else
	{
		sub->sub_tmpfile = ck_strdup ("/tmp/MTzXXXXXX");
#ifdef SVR4
		mktemp (sub->sub_tmpfile);
		fd = open (sub->sub_tmpfile, O_CREAT|O_RDWR|O_TRUNC,
				0600);
#else
		fd = mkstemp(sub->sub_tmpfile);
#endif "SVR4"
	}

	/* remove the tmp file immediately for security reason */
	ecode = errno;
	(void) unlink (sub->sub_tmpfile);

	if (fd < 0)
	{
		errno = ecode;
		return (SUB_NO_TMP_SPACE);
	}

	if ((fp = fdopen (fd, "r+")) == NULL)
	{
		return (SUB_NO_SWAP);
	}

#ifdef PEM
	options = sub->sub_pem_flags;
#endif PEM
	/* put all body parts into a single temp file */
	error_code = compose_msg(fp, sub, msg);
	if (error_code != SUB_OK)
	{
		sub_destroy(sub);
		return (error_code);
	}

	/* ready to send the mail; fp becomes stdin */
	rewind (fp);
#ifdef PEM
	/* only do PEM if one of the option flags is set */
	if (options != NO_FLAG) {
		/* create temporary file for PEM encrypted message */
		pemfile = ck_strdup("/tmp/PEMencryptXXXXXX");
#ifdef SVR4
		mktemp (pemfile);
		fdencrypt = open(pemfile, O_CREAT|O_RDWR|O_TRUNC, 0600);
#else
		fdencrypt = mkstemp(pemfile);
#endif SVR4
		if (fdencrypt < 0) {
			perror (pemfile);
			(void) unlink (pemfile);
			return (SUB_CRYPT);
		}
		(void) unlink (pemfile);

		if ((fPEM = fdopen (fdencrypt, "r+")) == NULL) {
			(void) perror (pemfile);
			close (fdencrypt);
			return(SUB_NO_SWAP);
		}

		if (scc(fp, fPEM, options, 0) < 0) {
			perror((char *)
				gettext("mailtool: can't apply PEM"));
			return(SUB_CRYPT);
		}


		fclose(fp);
		fp = fPEM;
	}
#endif PEM

#ifndef	SENDMAIL
	if ((sub->sub_pid = fork()) == 0) {

		(void) signal (SIGHUP, SIG_IGN);
		(void) signal (SIGINT, SIG_IGN);
		(void) signal (SIGQUIT, SIG_IGN);
		(void) signal (SIGTERM, SIG_IGN);

		dup2 (fileno(fp), 0);

                for (fd = (int)sysconf(_SC_OPEN_MAX); --fd > 2; )
			(void)close(fd);


		/*
		 * -t tells Mail to interpret the headers in the
		 * message, rather than requiring them to be supplied
		 * as arguments to the program. This allows Mail to pick
		 * up the "To:" line in the message and figure our who
		 * to send to.
		 */

		execlp ("Mail", "Mail", "-t", 0);
		(void)perror("Mail");
		exit (3);
	}
#else

	/* record the mail message or to any +folder specified
	 * in "to:", "cc:" or "bcc:", or execute any program.
	 */
	if (sub->sub_rec != NULL)
	{
		if (link_enumerate(sub->sub_rec, lsavemail, fp)) {
			/* there was an error while saving to a file */
			DP(("sub_done: lsavemail failed; ret SUB_LOG_ERR\n"));
			fclose(fp);
			return (SUB_LOG_ERR);
		}
		rewind (fp);
	}
	if (sub->sub_prog != NULL)
	{
		if (link_enumerate(sub->sub_prog, lexec, fp)) {
			/* there was an error while saving to a file */
			DP(("sub_done: lexec failed; returning SUB_FORK\n"));
			fclose(fp);
			return (SUB_FORK);
		}
		rewind (fp);
	}

	if ((sub->sub_pid = fork()) == 0) {

		DP(("sub_done: child pid %d.\n", getpid()));

		(void) signal (SIGHUP, SIG_IGN);
		(void) signal (SIGINT, SIG_IGN);
		(void) signal (SIGQUIT, SIG_IGN);
		(void) signal (SIGTERM, SIG_IGN);

#ifdef	DEBUG
		if (sub->sub_wait && DEBUG_FLAG)
		{
			DP(("submit proc: %d.  Wait 30 sec...\n", getpid()));
			sleep(30);
		}
#endif	DEBUG

		dup2(fileno(fp), 0);

                for (fd = (int)sysconf(_SC_OPEN_MAX); --fd > 2; )
			(void)close(fd);

		/* Fix BugId 1060009: we can't use -t on sendmail because BugId
		 * 1048909 that has problem with NFS directory and .forward.
		 */
		argv = unpack(sub);
		if (argv == NULL) {
			exit(0);
		}

		/* determine the send mail program */
		if ((sendmail = mt_value ("sendmail")) == NULL) {
			sendmail = "/usr/lib/sendmail";
		}

#ifdef DEBUG
		{
			int i;
			DP(("about to execvp %s:\n", sendmail));
			for (i = 0; argv[i]; i++) {
				DP(("    %s\n", argv[i]));
			}
			DP(("\n"));
		}
#endif

		execvp(sendmail, argv);
		(void)perror(sendmail);
		save_dead(sub, fp);
		exit(3);
	}
#endif	SENDMAIL

	fclose(fp);

	if (sub->sub_pid < 0) {
		return (SUB_FORK);
	}

	return (0);
}

static void
sub_destroy(struct submission *sub)
{
	DP(("sub_destroy: sub %x, pid %d\n", sub, getpid()));
	if (sub == NULL)
		return;

	/* the temp file should be removed by sub_done() */
	ck_free (sub->sub_tmpfile);
	sub->sub_tmpfile = NULL;

	/* free up the dead letter memory space. */
	ck_free (sub->sub_deadletter);
	sub->sub_deadletter = NULL;

	/* we don't remove the error file; the caller should do it */
	ck_free (sub->sub_errfile);
	sub->sub_errfile = NULL;

	if (sub->sub_to)
	{
		link_enumerate (sub->sub_to, link_free, ck_free);
		sub->sub_to = NULL;
	}
	if (sub->sub_cc)
	{
		link_enumerate (sub->sub_cc, link_free, ck_free);
		sub->sub_cc = NULL;
	}
	if (sub->sub_bcc)
	{
		link_enumerate (sub->sub_bcc, link_free, ck_free);
		sub->sub_bcc = NULL;
	}
	if (sub->sub_rec)
	{
		link_enumerate (sub->sub_rec, link_free, ck_free);
		sub->sub_rec = NULL;
	}
	if (sub->sub_prog)
	{
		link_enumerate (sub->sub_prog, link_free, ck_free);
		sub->sub_prog = NULL;
	}

	ck_free (sub);
}

struct link_obj *
link_add (head, p_value, p_cmp)
struct link_obj **head;
caddr_t	p_value;
int (*p_cmp)();
{
	struct link_obj *lo;
	struct link_obj *p_prev;
	struct link_obj *p_head = *head;

	lo = (struct link_obj *) ck_malloc (sizeof(struct link_obj));
	lo->l_value = p_value;
	lo->l_next = NULL;

	p_prev = NULL;
	while (p_head != NULL)
	{
		if (p_cmp != NULL)
		{
			if ((*p_cmp) (p_head->l_value, p_value) == 0)
			{
				ck_free (lo);
				return (NULL);
			}
		}
		p_prev = p_head;
		p_head = p_head->l_next;
	}

	if (p_prev == NULL)
		*head = lo;
	else
		p_prev->l_next = lo;

	return (lo);
}

struct link_obj *
link_find (p_head, p_value, p_cmp)
struct link_obj *p_head;
caddr_t p_value;
int (*p_cmp)();
{
	while (p_head != NULL)
	{
		if ((*p_cmp) (p_head->l_value, p_value) == 0)
			return (p_head);

		p_head = p_head->l_next;
	}
	return (NULL);
}

int
link_free (p_head, p_free)
struct link_obj *p_head;
void (*p_free)();
{
	if (p_free)
		(*p_free) (p_head->l_value);
	ck_free (p_head);
	return (0);
}

int
link_enumerate (p_head, p_func, param)
struct link_obj *p_head;
int (*p_func)();
caddr_t param;
{
	int	r;
	struct link_obj *p_next;

	while (p_head != NULL)
	{
		p_next = p_head->l_next;
		if ((r = (*p_func) (p_head, param)) != 0)
			return (r);
		p_head = p_next;
	}
	return (0);
}

/*
 * Determine if the passed address is a local "send to file" address.
 * If any of the network metacharacters precedes any slashes, it can't
 * be a filename.  We cheat with .'s to allow path names like ./...
 */
static int
isfileaddr(name)
	char *name;
{
	register char *cp;
 
	if (strchr(name, '@'))
		return(0);
	if (*name == '+')
		return(1);
	for (cp = name; *cp; cp++) {
		if (*cp == '.')
			continue;
		if (strchr(metanet, *cp))
			return(0);
		if (*cp == '/')
			return(1);
	}
	return(0);
}
