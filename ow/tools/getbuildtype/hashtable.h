/*
**	Hashtable module
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



#ifndef _hashtable_h_
#define _hashtable_h_

#pragma ident "@(#)hashtable.h	1.4 92/06/28	SMI"

#define TRUE		  1
#define FALSE		  0
#define	HASHTABSIZE	211

typedef struct BKT {
    char *token;
    char *value;
    struct BKT *next;
} *BUCKET;

typedef struct HT {
    BUCKET ht_table[HASHTABSIZE+1];
} *HASHTABLE;

extern HASHTABLE  hcreate  (void);
extern void       hdestroy ( HASHTABLE ht );

extern int        hinsert  ( HASHTABLE ht, char *token, char *value );
extern int        hdelete  ( HASHTABLE ht, char *token );

extern int        hlookup  ( HASHTABLE ht, char *token );
extern char      *hvalue   ( HASHTABLE ht, char *token );

extern void       hprintout( HASHTABLE ht );

#endif /* _hashtable_h_ */

