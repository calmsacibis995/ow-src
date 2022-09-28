
/*
**	Macro definition manager
**	@(#)define.h	1.3 91/10/16
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



#ifndef _define_h_
#define _define_h_

#include "hashtable.h"

extern void  dprintout( void );
extern int   define( char *macroname, char *value );
extern int   undef( char *macroname );
extern char *valueof( char *macroname );
extern int   newlevel( void );
extern int   endlevel( void );

#endif

