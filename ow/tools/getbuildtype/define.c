/*
**	Macro definition manager
**	@(#)define.c	1.3 91/10/16
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
#include <stdlib.h>
#include "define.h"

#define HTSTACKSIZE	5
#define DEF_ERROR	-2
#define UNINITIALIZED	-1


static HASHTABLE htstack[HTSTACKSIZE];
static int ht_tos = UNINITIALIZED;

int
define( char *macroname, char *value )
{
	if (ht_tos == UNINITIALIZED)
		newlevel();

	return hinsert(htstack[ht_tos], macroname, value);
}

int
undef( char *macroname )
{
	int probelevel = ht_tos;

	if (ht_tos == UNINITIALIZED)
		return FALSE;
	while ( UNINITIALIZED < probelevel ) {
		if ( hdelete(htstack[probelevel], macroname) == TRUE )
			return TRUE;
		probelevel--;
	}
	return FALSE;
}

char *
valueof( char *macroname )
{
	int probelevel = ht_tos;

	if (ht_tos == UNINITIALIZED)
		return FALSE;
	while ( UNINITIALIZED < probelevel ) {
		char *value;
		if ( (value = hvalue(htstack[probelevel], macroname)))
			return value;
		probelevel--;
	}
	return NULL;
}


int
newlevel( void )
{
	if (ht_tos < HTSTACKSIZE) {
		htstack[++ht_tos] = hcreate();
		return ht_tos;
	} else {
		return DEF_ERROR;
	}
}

int
endlevel( void )
{
	if (ht_tos < HTSTACKSIZE && ht_tos > UNINITIALIZED) {
		hdestroy(htstack[ht_tos--]);
		return ht_tos;
	} else {
		return DEF_ERROR;
	}
}

void 
dprintout( void )
{
	int idx;
	for (idx = ht_tos; idx > UNINITIALIZED; idx--) {
		hprintout(htstack[idx]);
	}
}


#ifdef MAIN

void
main( void )
{
    int cc;

    newlevel();
    define("tuesday","Day 2");
    define("TUESDAY","Day 2+");
    define("Tuesday","Day 2++");
    newlevel();
    define("Tuesday","Day 2-+");
    define("Wednesday","Day 3");
    printf("Defines after setup:\n");
    dprintout();
    define("monday","The day after Sunday");
    printf("Insert monday,  Code for \"monday\" is %s\n",valueof("monday"));
    dprintout();
    newlevel();
    define("monday","The other day after Sunday");
    printf("Insert monday,  Code for \"monday\" is %s\n",valueof("monday"));
    dprintout();
    endlevel();
    printf("Lookup monday,  Code for \"monday\" is %s\n",valueof("monday"));
    define("monday","A second value");
    printf("Insert monday,  Code for \"monday\" is %s\n",valueof("monday"));
    dprintout();
    undef("monday");
    printf("Delete monday,  Code for \"tuesday\" is %s\n",valueof("tuesday"));
    dprintout();
    endlevel();
    if (valueof("wed"))
        printf("\"wed\" was found\n");
    else
        printf("\"wed\" was not found\n");
    endlevel();
}
#endif
