#ifndef lint
static  char sccsid[] = "@(#)garbage.c 3.6 92/10/30 Copyr 1991 Sun Microsystems, Inc.";
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

#include	<stdio.h>
#include	"rtable4.h"
#include	<sys/param.h>
#include	<rpc/rpc.h>
#include	<unistd.h>
#include	"util.h"
#include	"log.h"
#include	"debug.h"
#include	"tree.h"
#include	"list.h"
#include	"iappt.h"

extern Info *rtable_get_table();
extern char *rtable_get_owner();
extern char *pgname;

int error=0;
int fd=0;
char *command=NULL, *bak=NULL, *temp=NULL, *log=NULL;

static int
visit(d)
	caddr_t d;
{
	int stop=0;

	switch (((Appt *) d)->tag->tag)
	{
	/*	otherTags = events read in from files.
		Don't write to log.
	*/
	case otherTag:
		return(stop);

	/*	a little hack to get us off the
		totally hokey magic-time business.
	*/
	case appointment:
		if (magic_time(((Appt *)d)->appt_id.tick)) {
			((Appt *) d)->tag->showtime=0;
		}
		break;
	default:
		break;
	}
	if (append_log_internal(fd, (Appt *) d, add)==CERROR) {
        	error=1;
        	stop=1;
        }

	return(stop);
}

static void
cleanup()
{
	if(command!=NULL) {
		free(command);
		command=NULL;
	}
	if(bak!=NULL) {
		free(bak);
		bak=NULL;
	}
	if(temp!=NULL) {
		free(temp);
		temp=NULL;
	}
	if(log!=NULL) {
		free(log);
		log=NULL;
	}
}

extern void
collect_one(target)
	char *target;
{
	char *who = NULL;
	Table_Status status;
	Info *p_info=NULL, *new=NULL;
	Table_Args args;
	int stat;

	error=0;
	if (target==NULL) return;

	if(debug) {
		char buf[1024];
		FILE *fp;
		fprintf(stderr, "starting garbage collection\n");
#ifndef SVR4
		fprintf(stderr, "\npstat -s\n");
		fp = popen("pstat -s", "r");
		memset(buf, 0, sizeof(buf));
		fread(buf, sizeof(buf)-1, 1, fp);
		fprintf(stderr, "%s\n", buf);
		pclose(fp);
#endif
	}
	p_info = rtable_get_table(target);
	if (p_info==NULL) {
		fprintf(stderr, "%s: unable to create data structure\n",pgname);
		fprintf(stderr, "for garbage collection.\n");
		fprintf(stderr, "possible causes: internal cm failure\n");
		fprintf(stderr, "damage: none\n\n\n");
		return;
	}

	status	= rb_check_tree(APPT_TREE(p_info));
	log	= get_log(target);
	temp	= get_tmp(target);
	bak	= get_bak(target);
	if (status != ok || log==NULL || temp==NULL || bak==NULL) {
		fprintf(stderr, "%s: cannot acquire files to execute garbage collection.\n", pgname);
		fprintf(stderr, "possible causes: cannot find home directory.\n");
		fprintf(stderr, "\tNIS or your host server might be down.\n");
		fprintf(stderr, "damage: none\n\n");
		goto Cleanup;
	}

	/* Make sure that the temp file does not exist before garbage collect */
	who = rtable_get_owner(target);
	who = get_head(who, '@');
	unlink (temp);
	if ((stat = create_log(who, temp))==CERROR || stat == PWERROR) {
		fprintf(stderr, "%s: file error on %s during garbage collection\n", pgname, temp);
		if (stat == CERROR)
			fprintf(stderr, "possible causes: host server is down, disk is full, out of memory, file protections have changed.\n");
		else
			fprintf(stderr, "Reason: getpwnam() failed. No passwd entry for owner of %s\n", log);
		fprintf(stderr, "damage: none\n\n");
		goto Cleanup;
	}

	/* Keep the temp log file open during garbage collection. */
	if ((fd = open(temp, O_RDWR | O_APPEND | O_SYNC)) == -1)
	{
		fprintf(stderr, "%s: file error on %s data structure to file for garbage collection\n", pgname, temp);
		fprintf(stderr, "possible causes: host server is down, disk is full, out of memory, file protections have changed.\n");
		fprintf(stderr, "damage: none\n\n");
		goto Cleanup;
	}

	(void)append_access_log_internal (fd, GET_R_ACCESS(p_info),
					access_read);
	(void)append_access_log_internal (fd, GET_W_ACCESS(p_info),
					access_write);
	(void)append_access_log_internal (fd, GET_D_ACCESS(p_info),
					access_delete);
	(void)append_access_log_internal (fd, GET_X_ACCESS(p_info),
					access_exec);

	rtable_enumerate_up(p_info, visit);	/* dump the tree */
	if ((close(fd)==EOF) || error) {
		fprintf(stderr, "%s: Can't dump data structure to file for garbage collection\n", pgname);
		fprintf(stderr, "possible causes: host server is down, disk is full, out of memory, file protections have changed.\n");
		fprintf(stderr, "damage: none\n\n");
		goto Cleanup;
	}

	/* mv -f .callog .calbak; mv -f temp .callog */
	if (rename (log, bak) < 0) {
		perror ("rpc.cmsd: Can't backup callog to .calbak.\nreason:");
		goto Cleanup;
	}
	if (rename (temp, log) < 0) {
		perror("rpc.cmsd: Can't move .caltemp to callog.\nreason:");
		fprintf(stderr, "%s: you may recover %s from %s.\n", pgname, log, bak);
		goto Cleanup;
	}
	if(debug) {
		char buf[1024];
		FILE *fp;
		fprintf(stderr, "garbage collection completed successfully\n");
#ifndef SVR4
		fprintf(stderr, "\npstat -s\n");
		fp = popen("pstat -s", "r");
		fread(buf, sizeof(buf)-1, 1, fp); 
		fprintf(stderr, "%s\n", buf);
		pclose(fp);
#endif
	}

Cleanup:
	if (who != NULL)
		free(who);
	cleanup();
	return;
}
