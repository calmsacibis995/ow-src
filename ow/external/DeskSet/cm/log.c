#ifndef lint
static  char sccsid[] = "@(#)log.c 3.14 96/09/13 Copyr 1991 Sun Microsystems, Inc.";
#endif

/*
 *  Copyright (c) 1987-1990 Sun Microsystems, Inc.
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
#include <errno.h>
#include <pwd.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <rpc/rpc.h>
#include "util.h"
#include "appt.h"
#include "log.h"

char		*current_owner;
extern char	*pgname;
static char	*spool_dir = DEFAULT_DIR;

int yyylineno;

yyyerror(s)
         char *s;
{
        (void)fprintf (stderr, "%s: %s\n", pgname, s);
        (void)fprintf (stderr, "at line %d\n", yyylineno);
/*
        exit(-1);
*/
}
 
extern int
report_err(s)
         char *s;
{
         (void)fprintf (stderr, s);
/*
         exit (-1);
*/
}

static void
periodtostr(i, q)
	Interval i; char *q;
{
	if (q==NULL) return;
	q[0]=NULL;
	switch (i) {
		case single:
			cm_strcpy (q, "single");
			break;
		case daily:
			cm_strcpy (q, "daily");
			break;
		case weekly:
			cm_strcpy (q, "weekly");
			break;
		case biweekly:
			cm_strcpy (q, "biweekly");
			break;
		case monthly:
			cm_strcpy (q, "monthly");
			break;
		case yearly:
			cm_strcpy (q, "yearly");
			break;
		case nthWeekday:
			cm_strcpy (q, "nthWeekday");
			break;
		case everyNthDay:
			cm_strcpy (q, "everyNthDay");
			break;
		case everyNthWeek:
			cm_strcpy (q, "everyNthWeek");
			break;
		case everyNthMonth:
			cm_strcpy (q, "everyNthMonth");
			break;
		case monThruFri:
			cm_strcpy (q, "monThruFri");
			break;
		case monWedFri:
			cm_strcpy (q, "monWedFri");
			break;
		case tueThur:
			cm_strcpy (q, "tueThur");
			break;
		case daysOfWeek:
			cm_strcpy (q, "daysOfWeek");
			break;
		default:
			cm_strcpy (q, "single");
			break;
	}
}

static
privacytostr(p, q)
	Privacy_Level p; char *q;
{
	if (q==NULL) return;
	q[0]=NULL;
	switch(p) {
	case public:
		cm_strcpy(q, "public");
		break;
	case private:
		cm_strcpy(q, "private");
		break;
	case semiprivate:
		cm_strcpy(q, "semiprivate");
		break;
	default:
		cm_strcpy(q, "public");
		break;
	}
}

static
apptstatustostr(p, q)
	Appt_Status p; char *q;
{
	if (q==NULL) return;
	q[0]=NULL;
	switch(p) {
	case active:
		cm_strcpy(q, "active");
		break;
	case pendingAdd:
		cm_strcpy(q, "pendingAdd");
		break;
	case pendingDelete:
		cm_strcpy(q, "pendingDelete");
		break;
	case committed:
		cm_strcpy(q, "committed");
		break;
	case cancelled:
		cm_strcpy(q, "cancelled");
		break;
	case completed:
		cm_strcpy(q, "completed");
		break;
	default:
		cm_strcpy(q, "active");
		break;
	}
}

static
tagstostr(p, q)
	Event_Type p; char *q;
{
	if (q==NULL) return;
	q[0]=NULL;
	switch(p) {
	case appointment:
		cm_strcpy(q, "appointment");
		break;
	case reminder:
		cm_strcpy(q, "reminder");
		break;
	case otherTag:
		cm_strcpy(q, "otherTag");
		break;
	case holiday:
		cm_strcpy(q, "holiday");
		break;
	case toDo:
		cm_strcpy(q, "toDo");
		break;
	default:
		cm_strcpy(q, "appointment");
		break;
	}
}
	
	
extern char *
find_directory(who)
	char *who;
{
        struct passwd   *pw=NULL;
        char            *buf=NULL;
 
        pw = getpwnam(who);
        if (pw == NULL) {
		fprintf (stderr, "Can't find home directory\n");
		return (NULL);
	}
	buf = (char *) ckalloc (cm_strlen(pw->pw_dir) + 2);
	cm_strcpy (buf, pw->pw_dir);
        return (buf);
}

static char *
get_fname (dir,fname,who)
	char *dir; char *fname; char *who;
{
	char *buf;

/*
** Bug 1265008 - insufficient checking of cm user name creates vulnerability.
** We must check that the user name does not contain a / character
*/
	if (strchr(who, '/'))
		return(NULL);

	buf = (char *) ckalloc (cm_strlen(dir) + cm_strlen(fname) + cm_strlen(who) + 3);
	if (buf != NULL)
		sprintf (buf, "%s/%s.%s", dir, fname, who);

    return (buf);
}

extern char *
get_log(who)
	char *who;
{
	return (get_fname (spool_dir, DEFAULT_LOG, who));
}
 
extern char *
get_bak(who)
	char *who;
{
	return (get_fname (spool_dir, DEFAULT_BAK, who));
}
 
extern char *
get_tmp(who)
	char *who;
{
	return (get_fname (spool_dir, DEFAULT_TMP, who));
}

extern void
unlock_log(fd)
	int fd;
{
	lockf(fd, F_ULOCK, 0);
}

extern int
lock_log(fd)
	int fd;
{
	int i;
	int tries=3;
	int locked=0;
	
	for (i=0; i<tries; i++) {
		locked = lockf(fd, F_TLOCK, 0);
		if (locked) {
			sleep(1);
		}
		else return(locked);
	}
	return(locked);
}

/* return value:
 * 0		- file created successfully
 * FILEERROR	- file exists already
 * PWERROR	- cannot get passwd entry of owner
 * CERROR	- something goes wrong, e.g. set_mode failed.
 */
extern int
create_log(owner, file)
	char *owner;
	char *file;
{
	struct tm *tm;
	int	uid;
	int	fd;
	int	tmval;
	char	*tmstr;
	FILE	*f;
	struct passwd *pw;

	pw = getpwnam (owner);
	if (pw == NULL)
		return(PWERROR);
	uid = pw->pw_uid;

	/* Read by owner and Read/Write by group (gid must be daemon) */
	fd = open(file, O_WRONLY|O_CREAT|O_EXCL, DEFAULT_MODE);
	if (fd < 0) {
		if (errno == EEXIST)
			return(FILEERROR);
		else
			return(CERROR);
	}

	if (set_mode(file, uid, daemon_gid, DEFAULT_MODE) < 0) {
		close(fd);
		unlink(file);
		return(CERROR);
	}

	tmval = time((time_t *) 0);
	tm = localtime((time_t*)&tmval);
	tmstr  = asctime(tm);
	tmstr[24] = NULL;		/* strip off CR */

	if ((f = fdopen(fd, "w")) == NULL) {
		fprintf (stderr, "%s: fdopen failed\n", pgname);
		close(fd);
                return(CERROR);
	}
	if (fputs("Version: 1\n", f)==EOF) {
		perror(pgname);
		(void) fclose(f);
		return(CERROR);
	}
	if (fputs("**** start of log on ", f)==EOF) {
		perror(pgname);
		(void) fclose(f);
		return(CERROR);
	}
	if (fputs(tmstr, f)==EOF) {
		perror(pgname);
		(void) fclose(f);
		return(CERROR);
	}
	if (fputs(" ****\n\n", f)==EOF) {
		perror(pgname);
		(void) fclose(f);
		return(CERROR);
	}
	if (fclose(f)==EOF) {
		perror(pgname);
		return(CERROR);
	}
	return(0);
}

/* return value:
 * 0		- file loaded successfully
 * FILEERROR	- file does not exist
 * CERROR	- something goes wrong
 */
extern int
start_log(target, file)
	char *target;
	char *file;	/* .callog | .caltemp */
{
	struct tm *tm;
	char	*user;
	int	uid;
	int	fd;
	int	tmval;
	char	*tmstr;
	FILE	*f;
	extern void setinput();

	if (target == NULL)
		return(CERROR);
	else {
		/* ZZZ: we are blindly assumed that it is user's machine. */ 
		struct stat info;
		struct passwd *pw;

		/* format of calendar name: identifier.name
		 * if this is a personal calendar, identifier
		 * is a user login name.
		 */
		user = get_head(target, '.');
		if (pw = getpwnam(user)) {
			/*
			 * check ownership only if identifier
			 * of the calendar name is a user name.
			 */
			uid = pw->pw_uid;

			/* If the file ownership is incorrect; the file
			 * will be renamed to prevent a Trojan horse.
			 * Also, self-corrects the file permission.
	 	 	 */
			if (check_mode(file, uid, daemon_gid, DEFAULT_MODE)<0) {
				free(user);
				return(CERROR);
			}
		}
		free(user);
	}

	f = fopen(file, "r");
	if (f == NULL) {
		if (errno == ENOENT)
			return(FILEERROR);
		else
			return(CERROR);
	}

	setinput(f);
	/* THIS IS GROSS, COMMUNICATING THRU A GLOBAL */
	if (current_owner != NULL) {
		free(current_owner);
		current_owner = NULL;
	}
	current_owner = (char *)cm_strdup(target);
	if (yyyparse() == -1) {
		fprintf(stderr, "%s: unable to parse %s\n", pgname, file);
		yywrap(f);
		return(CERROR);
	}
	yywrap(f);
	return(0);
}

extern int
append_log (file, appt, action)
	char *file; Appt *appt; Transaction action;
{
	int	f;
	int	ecode;
	char	buff[256];

	if ((f = open(file, O_RDWR | O_APPEND | O_SYNC)) != -1) {
		ecode = append_log_internal(f, appt, action);
		if (ecode!=0)
			perror(pgname);
		if (close(f)==EOF) {
			perror(pgname);
			return(CERROR);
		}
		if (ecode==0)
			return (0);
	}
	cm_strcpy(buff, pgname);
	cm_strcat(buff, ": ");
	cm_strcat(buff, file);
	perror(buff);
	return(ecode);
}

extern int
append_log_internal (f, appt, action)
	int f; Appt *appt; Transaction action;
{
	char *tmstr;
	FILE *fptr;
	int file_size = 0, nbytes_written = 0, nbytes_towrite = 0;
	char buf[BUFSIZ*3], buf2[BUFSIZ*2]; /* 1 BUFSIZ for what fields, 
					       1 BUFSIZ for mailto field,
					       and 1 BUFSIZ for the rest */

	buf[0] = NULL;
	tmstr = ctime (&appt->appt_id.tick);
	tmstr[24] = NULL;		/* strip off CR */

	if ((fptr = fdopen(f, "a")) != NULL)
		file_size = ftell(fptr);
	switch (action) {
	case add:
		sprintf(buf, "(add \"%s\" key: %d ", tmstr, appt->appt_id.key);
		if (appt->what) {
			char *temp = cr_to_str(appt->what);
			sprintf(buf2, "what: \"%s\" ", temp);
			cm_strcat(buf, buf2);
			free(temp);
		}
		if (appt->client_data) {
			sprintf(buf2, "details: \"%s\" ", appt->client_data);
			cm_strcat(buf, buf2);
		}
		if (appt->duration) {
			sprintf(buf2, "duration: %d ", appt->duration);
			cm_strcat(buf, buf2);
		}

		tmstr[0]=NULL;  
		periodtostr (appt->period.period, tmstr);
		sprintf(buf2, "period: %s ", tmstr);
		cm_strcat(buf, buf2);

		if (appt->period.nth != 0) {
			sprintf (buf2, "nth: %d ", appt->period.nth);
			cm_strcat(buf, buf2);
		}
		if (appt->period.enddate != 0) {
			tmstr = ctime (&(appt->period.enddate));
			tmstr[24] = NULL; /* strip off CR */
			sprintf(buf2, "enddate: \"%s\" ", tmstr);
			cm_strcat(buf, buf2);
		}

		sprintf(buf2, "ntimes: %d ", appt->ntimes);
		cm_strcat(buf, buf2);

		if (appt->exception != NULL) {
			struct Except *e = appt->exception;
			cm_strcat(buf, "exceptions: (");
			while(e != NULL) {
				sprintf(buf2, "%d ", e->ordinal);
				cm_strcat(buf, buf2);
				e = e->next;
			}
			cm_strcat(buf, ") ");
		}
/*
		if (appt->mailto != NULL && cm_strlen(appt->mailto)) {
			sprintf(buf2, "mailto: \"%s\" ", appt->mailto);
			cm_strcat(buf, buf2);
		}
*/
		if (appt->author != NULL) {
			sprintf(buf2, "author: \"%s\" ", appt->author);
			cm_strcat(buf, buf2);
		}
		if (appt->attr != NULL) {
			struct Attribute *item = appt->attr;
			cm_strcat(buf, "attributes: (");
			while(item != NULL) {
				sprintf(buf2, "(\"%s\",\"%s\",\"%s\")",
					item->attr, item->value, item->clientdata);
				cm_strcat(buf, buf2);
				item = item->next;
			}
			cm_strcat(buf, ") ");
		}
		if (appt->tag != NULL) {
			struct Tag *item = appt->tag;
			cm_strcat(buf, "tags: (");
			while(item != NULL) {
				tmstr[0]=NULL;
				tagstostr(item->tag, tmstr);
				sprintf(buf2, "(%s , %d)", tmstr, item->showtime);
				cm_strcat(buf, buf2);
				item = item->next;
			}
			cm_strcat(buf, ") ");
		}


		tmstr[0]=NULL;
		apptstatustostr(appt->appt_status, tmstr);
		sprintf(buf2, "apptstat: %s ", tmstr);
		cm_strcat(buf, buf2);

		tmstr[0]=NULL;
		privacytostr(appt->privacy, tmstr);
		sprintf(buf2, "privacy: %s )\n", tmstr);
		cm_strcat(buf, buf2);

		nbytes_towrite = cm_strlen(buf);
		nbytes_written = write(f, buf, nbytes_towrite);
		if (nbytes_written != nbytes_towrite) {
			ftruncate(f, file_size);
			errno = ENOSPC;
			return (SPACEERROR);	
		}
		
		break;    
	case cm_remove:
		sprintf(buf, "(remove \"%s\" key: %d)\n", tmstr, appt->appt_id.key);
		nbytes_towrite = cm_strlen(buf);
		nbytes_written = write(f, buf, nbytes_towrite);
		if (nbytes_written != nbytes_towrite) {
			ftruncate(f, file_size);
			errno = ENOSPC;
			return (SPACEERROR);	
		}
		break;    
	default:
		break;    
	}
	return(0);
}

extern int
append_access_log(file, types, lists)
char *file;
int  *types;
Access_Entry *lists[];
{
	int ecode, f;
	char buff[256];

	if ((f = open(file, O_RDWR | O_APPEND | O_SYNC)) != -1) {
		while (*types != 0)
		{
			ecode = append_access_log_internal(f,
						*lists++, *types++);
			if (ecode!=0) {
				perror(pgname);
				break;
			}
		}
		if (close(f)==EOF) {
			perror(pgname);
			return(CERROR);
		}
		return(ecode);
	}
	cm_strcpy(buff, pgname);
	cm_strcat(buff, ": ");
	cm_strcat(buff, file);
	perror(buff);
	return(CERROR);
}

extern int
append_access_log_internal(f, p, type)
	int f; Access_Entry *p; int type;
{
	char	*p_type;
	char buf[BUFSIZ*4]; /* the list couldnt possibly be bigger than this
				could it ? */
	FILE *fptr;
	int file_size = 0, nbytes_written = 0, nbytes_towrite = 0;

	if (type == access_read)
		p_type = "read";
	else if (type == access_write)
		p_type = "write";
	else if (type == access_delete)
		p_type = "delete";
	else if (type == access_exec)
		p_type = "exec";
	else
		return(0);

	if ((fptr = fdopen(f, "a")) != NULL)
                file_size = ftell(fptr);
	sprintf(buf, "(access %s ", p_type);

	while(p!=NULL) {
		cm_strcat(buf, "\"");
		cm_strcat(buf, p->who);
		cm_strcat(buf, "\" ");
		p = p->next;
	}
	cm_strcat(buf, ")\n");

	nbytes_towrite = cm_strlen(buf);
	nbytes_written = write(f, buf, nbytes_towrite);
	if (nbytes_written != nbytes_towrite) {
		ftruncate(f, file_size);
		errno = ENOSPC;
		return (SPACEERROR);	
	}
	return(0);
}
