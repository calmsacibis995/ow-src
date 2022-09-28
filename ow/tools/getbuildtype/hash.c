#pragma ident "@(#)hash.c	1.4 92/07/17	SMI"

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



 /***************************************************************************
 *									    *
 *			      Hash Table Module				    *
 *									    *
 ****************************************************************************
 *									    *
 *	This implementation uses the private function hash() to generate    *
 *	'unique' values for tokens, and 'seperate chaining' to resolve      *
 *	hash table collisions.						    *
 *									    *
 *	See figure 7.34 and 7.35 in Aho, Sethi, and Ullman.		    *
 *									    *
 ***************************************************************************/

 /***************************************************************************
 *	Copyright 1987 John Plocher  Freely distributable source code       *
 ***************************************************************************/

 /****************************************************************************
 *									     *
 *		(This item is declared in the user's program)		     *
 *									     *
 *  HASHTABLE *hashtable;						     *
 *	  :								     *
 *	  :								     *
 ******** : ******************************************************************
	  :
	  :
	  :
 ******** : ******************************************************************
 *   	  :								     *
 *	  :	(These items are allocated in dynamic memory)		     *
 *   	  :								     *
 *	  :			HASHTABLE				     *
 * +------:-------------------------------------------------------------+    *
 * |	  v								|    *
 * |	+---+---+---+---+---+---+---+---+---+---+---+---+---+		|    *
 * |	| : |\0 |\0 |\0 |\0 |\0 |\0 |\0 |\0 |\0 |\0 |\0 |\0 | ht_table	|    *
 * |	+-:-+---+---+---+---+---+---+---+---+---+---+---+---+		|    *
 * |	  : 								|    *
 * +------:-------------------------------------------------------------+    *
 *	  :								     *
 *	  :								     *
 *	  :			BUCKET					     *
 * +------:-------------------------------------------------------------+    *
 * |	  v                                                             |    *
 * |	+---+---+---+---+---+                            		|    *
 * |	| h | e | l | p |\0 |                             token		|    *
 * |	+---+---+---+---+---+---+---+---+                		|    *
 * |	| W | h | y |   | n | o | t |\0 |                 value		|    *
 * |	+---+---+---+---+---+---+---+---+                		|    *
 * |	| : | next							|    *
 * |	+-:-+								|    *
 * +------:-------------------------------------------------------------+    *
 *	  :								     *
 *	  :								     *
 *	  :			BUCKET					     *
 * +------:-------------------------------------------------------------+    *
 * |	  v								|    *
 * |	+---+---+---+---+---+---+---+                    		|    *
 * |	| M | o | n | d | a | y |\0 |                     token		|    *
 * |	+---+---+---+---+---+---+---+                    		|    *
 * |	| O | n | e |\0 |                                 value         |    *
 * |	+---+---+---+---+                                               |    *
 * |	|\0 | next							|    *
 * |	+---+								|    *
 * +--------------------------------------------------------------------+    *
 *									     *
 ****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "hashtable.h"

static unsigned long hash( char *token );


 /****************************************************************************
 *									     *
 *			     hprintout( )				     *
 *									     *
 *	Print out a graphic image of hashtable.  For DEBUGGING USE	     *
 *									     *
 ****************************************************************************/

void
hprintout( HASHTABLE ht ) {
   int		idx;	/* index into hashtable */
   BUCKET	bkt;	/* for probing chains   */

       for (idx=0; idx < HASHTABSIZE; idx++) {	/* for each chain */
		   bkt = ht->ht_table[idx];
		if (!bkt) {
		    continue;
		}
		while ( bkt ) {	        /* print each bucket */
	           printf("[%03d] #define %s\t\"%s\"\n",
			  idx, bkt->token, bkt->value);
	           bkt = bkt->next;
	       }
       }
}

 /****************************************************************************
 *									     *
 *			  HASHTABLE hcreate()				     *
 *									     *
 *	Allocate and initialize a hash table with HASHTABSIZE elements.	     *
 *									     *
 ****************************************************************************/

HASHTABLE
hcreate(void) {
	int idx;
	HASHTABLE newtable;
 
	newtable = (HASHTABLE)malloc( sizeof(struct HT));
	for (idx = 0; idx < HASHTABSIZE; idx++) {	/* no buckets yet */
		newtable->ht_table[idx] = NULL;
	}
	return newtable;
}


 /****************************************************************************
 *									     *
 *			     hdestroy( )				     *
 *									     *
 *	Unallocate all storage used by the hashtable (table and chains)      *
 *									     *
 ****************************************************************************/

static void
dispose( BUCKET bkt ) {
	if (bkt->token) free(bkt->token);
	if (bkt->value) free(bkt->value);
	free(bkt);
}

void
hdestroy( HASHTABLE ht ) {
	int idx;
	if ( ht ) {			/* valid call? */
		for (idx = 0; idx < HASHTABSIZE; idx ++) 
			if (ht->ht_table[idx]) {
				BUCKET bkt = ht->ht_table[idx];
				while (bkt) {		/* free the buckets */
					BUCKET freeme = bkt;
					bkt = bkt->next;/* and the contents */
					dispose(freeme);
				}
			}
		free( ht );				/* then the table */ 
	}
}

 /****************************************************************************
 *									     *
 *		  ht_lookup( hashtable, token )				     *
 *									     *
 *	Internal routine used by lookup and insertion functions.	     *
 *									     *
 *   Returns								     *
 *   -------	+-----------------------+                             	     *
 *      pointer | location of the	|                             	     *
 *		| desired bucket	|                                    *
 *		+-----------------------+				     *
 *	NULL	| Token not found       |				     *
 *		+-----------------------+				     *
 *									     *
 ****************************************************************************/

static BUCKET
ht_lookup(HASHTABLE ht, char *token) {
       long int	chain  = hash( token );			/* Get chain slot */
       BUCKET	tmpbkt = ht->ht_table[chain];
       while ( tmpbkt ) {				/*while there are bkts*/
	    if (strcmp(tmpbkt->token,token) == 0) { 	/* Found a match!   */
		   return tmpbkt;
	    }
	    tmpbkt = tmpbkt->next;			/* else hump along */
       }
       return NULL;
}

 /****************************************************************************
 *									     *
 *			hinsert( token, value ) : boolean		     *
 *									     *
 *	return	TRUE if token was added to the table,			     *
 *		FALSE if token was not added.  (new() failed)		     *
 *									     *
 ****************************************************************************/

int
hinsert( HASHTABLE ht, char *token, char *value ) {
	BUCKET bkt;
	if ( bkt = ht_lookup( ht, token )) {	  /* update existing entry */
	   free(bkt->value);
	   bkt->value = strdup(value);
	   return TRUE;
	} else {
	   long int	chain   = hash( token );
	   BUCKET	newbkt  = (BUCKET)malloc(sizeof(struct BKT));
	   if ( newbkt == NULL) {	          /* malloc()  ERROR */
	       return FALSE;
	   }
	   if ( ht->ht_table[chain] == NULL) {    /* on a NEW chain */
	       newbkt->next = NULL;
	   } else {				  /* front of existing chain */
	       newbkt->next = ht->ht_table[chain];	    /* Save old link */
	   }
	   newbkt->value       = strdup(value);
	   newbkt->token       = strdup(token);
	   ht->ht_table[chain] = newbkt;
	   return TRUE;
	}
}

 /****************************************************************************
 *									     *
 *			   int hdelete( token )				     *
 *									     *
 *	remove token from the hashtable.				     *
 *									     *
 ****************************************************************************/

int
hdelete( HASHTABLE ht, char *token ) {

       if ( ht ) {
	   BUCKET bkt;
	   if (bkt = ht_lookup(ht,token)) {		      /* got one! */
	       int chain = hash( token );
	       if (ht->ht_table[chain] == bkt) {	       /* at head */
		   ht->ht_table[chain] = bkt->next;
		   dispose( bkt );
	       } else { 				     /* elsewhere */
		   /*******************************************************
		   * At this point, back pointers would really be useful  *
		   * in removing an item.  ASSUMPTION:  delete will be	  *
		   * an infrequent operation, so the space/time tradeoff  *
		   * of this code -vs- another pointer field falls to	  *
		   * the side of more code and less space.  Sorry.	  *
		   ********************************************************
		   * We know that the bucket MUST be on this chain.	  *
		   * because hash( token ) pointed here  AND		  *
		   * 	  ht_lookup( token) suceeded     		  *
		   *******************************************************/
		   BUCKET tmp = ht->ht_table[chain];
		   /*
		   **	+-----+   +------+   +------+   +------+
		   **   |     |-->|      |-->|      |-->|      |-->@
		   **	+-----+   +------+   +------+   +------+
		   **     tmp                  bkt
		   */
		   while ( tmp->next && tmp->next != bkt ) {
		       tmp = tmp->next;
		   }
		   /*
		   **	+-----+   +------+   +------+   +------+
		   **   |     |-->|      |-->|      |-->|      |-->@
		   **	+-----+   +------+   +------+   +------+
		   **               tmp        bkt
		   */
		   tmp->next = tmp->next->next;
		   /*			   +----------+
		   **	+-----+   +------+ | +------+ | +------+
		   **   |     |-->|      |-+ |      | +>|      |-->@
		   **	+-----+   +------+   +------+   +------+
		   **               tmp        bkt
		   */
		   dispose( bkt );
	       }
	       return TRUE;
	    } /* else not found */
	} /* else invalid hash table */
	return FALSE;
}

 /****************************************************************************
 *									     *
 *			  hlookup( token ) : boolean			     *
 *									     *
 *	return TRUE if token is in the table, FALSE if not		     *
 *									     *
 ****************************************************************************/

int
hlookup( HASHTABLE ht, char *token ) {
	return	ht_lookup( ht, token)
		? TRUE
		: FALSE;
}


 /****************************************************************************
 *									     *
 *			  char *hvalue( token )				     *
 *									     *
 *	return the value associated with the token if token is in the table, *
 *	       NULL if it is not in the table				     *
 *									     *
 ****************************************************************************/

char *
hvalue( HASHTABLE ht, char *token ) {
	BUCKET bkt;
        if ( bkt = ht_lookup( ht, token )) {
		return bkt->value;
	} else {
		return NULL;
	}
}

 /****************************************************************************
 *                                                                           *
 *                        int hash( token )                                  *
 *                                                                           *
 *        Produce a hash value for the token string.  This function should   *
 *      avoid primary clustering (the exclusive use of even/odd locations,   *
 *      or any other phenomenom which makes the use of certain locations     *
 *      more likely than others).                                            *
 *                                                                           *
 *      The function should be fast and repeatable.                          *
 *                                                                           *
 *      This implementation is from p436 of 'Compilers' by Aho, et al        *
 *      It is from P.J. Weinberger's C compiler, which tested well on many   *
 *      different table sizes.                                               *
 *                                                                           *
 ****************************************************************************/

static unsigned long
hash( char *token ) {
    char	       *p;
    unsigned long	h = 0,
			g;
    for ( p=token; *p != '\0'; p++ ) {
        h = ( h << 4 ) + (*p);
        if (g = h & 0xf0000000L) {
            h = h ^ (g >> 24);
            h = h ^ g;
        }
    }
    return h & HASHTABSIZE;
}

#ifdef MAIN

#define PRINTTABLE 1

void
main(void) {
    HASHTABLE ht;
    int cc;

    ht = hcreate();
    cc = hinsert(ht,"tuesday","Day 2");
    cc = hinsert(ht,"TUESDAY","Day 2+");
    cc = hinsert(ht,"Tuesday","Day 2++");
    cc = hinsert(ht,"Wednesday","Day 3");
    if (PRINTTABLE) printf("Hashtable after setup:\n");
    if (PRINTTABLE) hprintout(ht);
    cc = hinsert(ht,"monday","The day after Sunday");
    printf("Insert monday,  Code for \"monday\" is %s\n",hvalue(ht,"monday"));
    if (PRINTTABLE) hprintout(ht);
    cc = hinsert(ht,"monday","A second value");
    printf("Insert monday,  Code for \"monday\" is %s\n",hvalue(ht,"monday"));
    if (PRINTTABLE) hprintout(ht);
    cc = hdelete(ht,"monday");
    printf("Delete monday,  Code for \"tuesday\" is %s\n",hvalue(ht,"tuesday"));
    if (PRINTTABLE) hprintout(ht);
    if (hlookup(ht,"wed")) {
        printf("\"wed\" was found\n");
    } else {
        printf("\"wed\" was not found\n");
    }
    hdestroy(ht);
}
#endif

