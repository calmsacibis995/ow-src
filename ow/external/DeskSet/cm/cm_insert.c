#ifndef lint
static  char sccsid[] = "@(#)cm_insert.c 3.5 94/05/10 Copyr 1991 Sun Microsystems, Inc.";
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
#include <ctype.h>
#include <string.h>
#include <rpc/rpc.h>
#include <sys/resource.h>
#include <sys/wait.h>
#include "util.h"
#include "appt.h"
#include "table.h"
#include "timeops.h"

int debug = 0;
static char** grab();			/* get keyed data from cmdlin */
extern int cm_tty_lookup();		/* get list of appointments */
static char cm_target[256] = "";	/* target for table (user@host) */
static int cm_today = 0;		/* today's date (in epoch time) */
static char cm_date[256] = "";		/* appointment date */
static char cm_view[16] = "";		/* view span (day,week,month) */
static char cm_start[16] = "";		/* start time for appointment list*/
static char cm_end[16] = "";		/* end time for appointment list */
static char cm_what[1024] = "Appointment";
					/* what appointment is about */
static int cm_repeat = 0;		/* how often to repeat this */
static int cm_flash = 0;		/* flash reminder */
static int cm_beep = 0;			/* beep reminder */
static int cm_open = 0;			/* open reminder */
static int cm_mail = 0;			/* mail reminder */

static void cm_args();			/* parse command line */

main(argc, argv)
        int argc; char **argv;
{
	char* date = NULL;
	char* view = NULL;
	char* target = NULL;
	char* start = NULL;
	char* end = NULL;

	table_abbrev_set_cache_size(0);
	init_time();
	init_strings();

	cm_args(argc,argv);		/* parse command line */
	if (cm_strlen(cm_target)) target = cm_target;
	if (cm_strlen(cm_date)) date = cm_date;
	if (cm_strlen(cm_view)) view = cm_view;
	if (cm_strlen(cm_start)) start = cm_start;
	if (cm_strlen(cm_end)) end = cm_end;
	/* insert appt */	
	if (cm_tty_insert(&target,date,start,end,cm_what) != -1)
		/* list appts */	
		(void) cm_tty_lookup(&target,date,view);
        exit(0);
}

static void
cm_args(argc,argv)
int argc;
char **argv;
{

	while (++argv && *argv) {
		switch(*(*argv+1)) {
		case 't':
		case 'c':
			argv = grab(++argv,cm_target,'-');
			break;
		case 'd':
			argv = grab(++argv,cm_date,'-');
			break;
		case 'v':
			argv = grab(++argv,cm_view,'-');
			break;
		case 's':
			argv = grab(++argv,cm_start,'-');
			break;
		case 'e':
			argv = grab(++argv,cm_end,'-');
			break;
		case 'w':
			argv = grab(++argv,cm_what,'-');
			break;
		default:
			fprintf(stderr,"Usage:\n\tcm_insert [ -c calendar ] [-d <mm/dd/yy>] [ -v view ]\n");
			fprintf(stderr, "                   [-w what string] [-s <HH:MMam/pm>] [-e <HH:MMam/pm>]\n");
			exit(1);
		}
	}
}

static char**
grab(argv,buf,stop_key)
char**argv;				/* command line arguments */
char *buf;				/* buffer for keyed data */
char stop_key;
{
	if (!argv || !*argv) return(argv);
	cm_strcpy (buf,*argv++);
	while(argv && *argv) {
		if (*(*argv) == stop_key) break;
		cm_strcat(buf," ");
		cm_strcat(buf,*argv++);
	}
	argv--;
	return(argv);
}
