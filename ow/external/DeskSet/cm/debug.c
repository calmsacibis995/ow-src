#ifndef lint
static  char sccsid[] = "@(#)debug.c 3.2 92/07/17 Copyr 1991 Sun Microsystems, Inc.";
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
#include "rtable4.h"
#include <sys/param.h>
#include <rpc/rpc.h>
#include "util.h"
#include "tree.h"
#include "list.h"
#include "iappt.h"

int debug, debug1;

int visits;
FILE *f;

static Boolean
visit(d)
	caddr_t d;
{
	switch (((Appt *)d)->tag->tag) {
		case otherTag:
		case appointment:
			print_tick(CM_TICK(d));
			fprintf(stderr,"%d(%d): appointment: %d %s\n",
				CM_TICK(d), KEY(d), ((Appt *) d)->what);  
			break;
		default:
			break;
	}
	visits++;
	return (false);
}

extern int
spill_tree (p_info) 
	Info	*p_info;
{
	Rb_Status status;

	visits=0;
	fprintf(stderr, "starting to spill tree to /tmp/tree.spill.\n");
	if ((f = fopen ("/tmp/tree.spill", "a")) == NULL) {
		perror("cal couldn't open /tmp/tree.spill for spill_tree");
		return (other);
	}
	status = rb_check_tree(APPT_TREE(p_info));
	if (status != rb_ok) {
		perror("cm received error from table_check");
		return(status);
	}
	rb_enumerate_up (APPT_TREE(p_info), visit);
	hc_enumerate_up (REPT_LIST(p_info), visit);
	fclose(f);
	fprintf(stderr, "%s %d\n", "tree spill completed successfully. entries=", visits);
	return(ok);
}
