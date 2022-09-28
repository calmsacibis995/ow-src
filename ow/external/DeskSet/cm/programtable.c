#ifndef lint
static  char sccsid[] = "@(#)programtable.c 3.2 92/07/09 Copyr 1991 Sun Microsystems, Inc.";
#endif
/* programtable.c */
/* implements the program table for this program number */
/* this module doesn't know anything about the specifics of any */
/* rtable except how many rtables are supported */
/*  - ie it doesn't include rtable*.h.  */
/* The actual program table entries are filled in by the rtable*.c's */

#include <rpc/rpc.h>
#include "rpcextras.h"

program_table ptable[] = {
	(struct rpcgen_table *)NULL, 0, /* rtable 0 no longer supported */
	(struct rpcgen_table *)NULL, 0, /* rtable 1 no longer supported */
	(struct rpcgen_table *)NULL, 0, /* rtable 2 filled in by rtable2.c */
	(struct rpcgen_table *)NULL, 0, /* rtable 3 filled in by rtable3.c */
	(struct rpcgen_table *)NULL, 0, /* rtable 4 filled in by rtable4.c */
	}; 
	
/* program_num is filled in from one of the rtable*.c's so that */
/* it can be declared from one of the rtable*.h's */
program_object po = {
	&ptable[0], 0, 0,
	};
	
program_handle program = &po;

program_handle newph() 
{
	extern void initrtable2();
	extern void initrtable3();
	extern void initrtable4();
	
	program->nvers = sizeof(ptable)/sizeof(ptable[0]);
	initrtable2(program);
	initrtable3(program); 
	initrtable4(program);
	return(program);
}

program_handle getph() 
{
	return(program);
}


