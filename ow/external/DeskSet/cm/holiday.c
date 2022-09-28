#ifndef lint
static  char sccsid[] = "@(#)holiday.c 2.10 91/04/12 Copyr 1991 Sun Microsystems, Inc.";
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

#include <sys/types.h>
#ifdef SVR4
#include <dirent.h>
#else
#include <sys/dir.h>
#endif /* SVR4 */
#include <rpc/rpc.h>
#include <stdio.h>
#include <pwd.h>
#include "util.h"
#include "log.h"
#include "token.h"
#include "appt.h"
#ifdef SVR4
#include <sys/param.h>
#include "holiday.i"
#endif "SVR4"

#define MAXCHAR 100

static void
parse_file(target, fname)
	char *target;
	char *fname;
{
	FILE 	*f; 
	int 	tick, lineno;
	char 	*x, *date, *what;
	Appt	*a;
	Token	h;
	Table_Args args;
	Standard_filter state[2];
	char 	line[MAXCHAR];
	static int special_key = 0;

	if (fname==NULL) return;
	seteuid(0);			/* temporary switch to root uid */
	f=fopen(fname, "r");		/* to read the *.cal files	*/
	seteuid(daemon_uid);		/* switch back to daemon uid	*/
	if (f==NULL) return;

	lineno = 0;
	while (fgets (line, sizeof(line), f)) {
		lineno++;
		h = (Token) string_to_handle(line, 0);
		date = filtered(h, state, date_filter, white_space);
		what = filtered(h, state, line_filter, nothing);
		destroy_handle(h); h=NULL;
		line[0]=NULL;	/* reuse line as a buffer */
		cm_strcat(line, date);
		cm_strcat(line, " 3:41 am");
		if ((date==NULL) || (tick=cm_getdate(line, NULL))<0) {
			sprintf (line, "cms holiday file: %s- %s %d \n", 
				fname, "possible error, line number", lineno);
			fprintf (stderr, line);
		}
		else if (what != NULL)  {
			a = make_appt();
			a->tag->tag = otherTag;
			/* Special appointments have negative key values. */
			a->appt_id.key = --special_key;
	   		a->appt_id.tick = tick;
	   		a->what = (char *) ckalloc (cm_strlen (what) + 1);
			if (cm_strlen(what)) {a->what=cm_strdup(what);}
			(void) rtable_insert_internal(target, a);
			free(what); 
			what=NULL;
		}
		if (date!=NULL) {
			free(date);
			date=NULL;
		}
	}
	fclose(f);
}


extern void
insert_holidays(target)
	char *target;
{
	DIR	*dirp;
	char	*dname, *dex, *name;
#ifdef SVR4
	struct	dirent *dp;
#else
	struct	direct *dp;
#endif SVR4
	char	buf[MAXNAMELEN];

	if (target==NULL) return;
#ifndef SVR4
        dex = (char *) re_comp("^.*\\.cal$");
#endif "SVR4"

	name = (char*)cm_target2name(target);
	dname = find_directory(name);
	seteuid(0);			/* temporary switch to root uid */
	dirp = opendir(dname);		/* to read the user's home dir	*/
	seteuid(daemon_uid);	/* switch back to daemon uid	*/
	if (dirp == NULL) return;
	while (dp = readdir(dirp)) {
#ifdef SVR4
                if (regex(holidaystring, dp->d_name)){
#else
                if (re_exec (dp->d_name)) {
#endif "SVR4"
			buf[0]=NULL;	
			(void) cm_strcat(buf, dname);
			(void) cm_strcat(buf, "/");
			(void) cm_strcat(buf, dp->d_name);
			parse_file(target, buf);   
		}
	}
	closedir(dirp);
	free(name);
	free(dname);
}


