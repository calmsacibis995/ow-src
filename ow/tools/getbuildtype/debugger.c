/*
**	Debugger package
**	@(#)debugger.c	1.5 91/10/16
*/

/*
**   ----------------------------------------------------------------- 
**          Copyright (C) 1986,1990  Sun Microsystems, Inc
**                      All rights reserved. 
**            Notice of copyright on this source code 
**            product does not indicate publication. 
**   
**                    RESTRICTED RIGHTS LEGEND: 
**   Use, duplication, or disclosure by the Government is subject 
**   to restrictions as set forth in subparagraph (c)(1)(ii) of 
**   the Rights in Technical Data and Computer Software clause at 
**   DFARS 52.227-7013 and in similar clauses in the FAR and NASA 
**   FAR Supplement. 
**   ----------------------------------------------------------------- 
*/



#include <stdio.h>
#include <string.h>
#include <stdarg.h>

#include "debugger.h"

extern void Dprintf( int level, char *fmt, ...);

int debug_state;

static char *fnamestack[STACKSIZE];
static int _tos	= 0;
static char tabs[80];

static void
pushfn( char *name )
{
	static int Overflow = 0;
	if (_tos == 0) *tabs='\0';
	if (_tos <= STACKSIZE) {
		fnamestack[ _tos ] = name;
	} else if ( Overflow == 0 ) {
		Overflow = 1;	/* Make sure we only print one of these messages */
		Dprintf(TRACE_WARNING, "Warning: function name stack overflow\n");
	}
	_tos++;
}

static char *
popfn( void )
{
	char *fn;
	_tos--;
	if (_tos <= STACKSIZE) {
		fn = fnamestack[ _tos ];
	} else {
		fn = "Unknown";
	}
	return fn;
}

void
Denter( char *function )
{
	pushfn( function );
	Dprintf(TRACE_FUNCTIONS, "Enter: %s\n", function);
	(void)strcat(tabs, " ");
}

void
Dleave( char *s )
{
	char *f = popfn();
	if (*tabs)
		tabs[ strlen(tabs) -1 ] = '\0';
	Dprintf(TRACE_FUNCTIONS, "Leave: %s ==> %s\n", f, s);
}

void
Dprintf( int level, char *fmt, ... )
{
    va_list args;
	va_start(args, fmt);
	if (level & debug_state) {
		(void)fprintf( stderr, "%s", tabs);
		if (level & TRACE_WARNING)
			(void)fprintf( stderr, "Warning: ");
		(void)vfprintf(stderr, fmt, args);
	}
	va_end(args);
}

