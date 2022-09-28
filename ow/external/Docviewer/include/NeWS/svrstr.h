#ifndef _NEWS_SVRSTR_H
#define _NEWS_SVRSTR_H


#ident "@(#)svrstr.h	1.2 06/11/93 NEWS SMI"



/*
 * Structures dealing with save/restore.
 * Copyright (c) 1989 by Sun Microsystems, Inc.
 */


/*
 * A save log entry gives the key and old value of a field that was changed
 * in some object.
 */

struct saveent {
    struct object key,
                value,
                obj;
    long        level;
};

#define SAVESETSIZE 20
#define SEARCHLIMIT 10

/* A save log is just a long list of saveents.  The list is constructed out of "struct saveset"s.  This way we cut down on the number of allocations. */

struct saveset {
    long        used;		/* The number of entries used */
    long        level;		/* The save level at the end of the saveset
				 * list */
    struct saveset *next;	/* The next block in this saveset */
    struct saveent entries[SAVESETSIZE];
};

extern struct saveset *current_savelog;

#endif /* _NEWS_SVRSTR_H */
