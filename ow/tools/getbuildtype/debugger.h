/*
**	Debugger Package
**	@(#)debugger.h	1.7 91/10/16
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



#ifndef _debugger_h_
#define _debugger_h_

extern int debug_state;

#define STACKSIZE	20

#define TRACE_WARNING	0x00000001
#define TRACE_MACROS	0x00000002
#define TRACE_FUNCTIONS	0x00000004
#define TRACE_DB	0x00000008

#define FNARGSLEN	255		/* Max len of printed arg list */

extern void Denter( char *function );
extern void Dprintf( int level, char *fmt, ... );
extern void Dleave( char *s );

#define RETURN(x)	Dleave( #x ); return x
#define sRETURN(x)	Dleave(  x );  return x

#endif

