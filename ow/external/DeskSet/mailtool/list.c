
/* list.c -- trivial list maintainer */
#ident  "@(#)list.c 3.7 95/11/01 Copyright 1994 Sun Microsystems"


#include <stdio.h>
#include "../maillib/ck_strings.h"
#include "list.h"


/* local functions */
static ListNode *alloc_listnode();
static void free_listnode(ListNode *ln);


/*
 * add an element to a list
 *
 * list is a pointer to the list head pointer (it will be updated if
 * necessary); ln is the elemement to be added
 */
ListNode *
add_listnode(ListNode **list, void *data)
{
	ListNode *ln;

	ln = alloc_listnode();

	ln->ln_head = list;
	ln->ln_data = data;
	ln->ln_next = *list;
	*list = ln;

	return (ln);
}



/*
 * remove an element from the list
 *
 * ln is the element to be removed
 */
void
rem_listnode(ListNode *ln)
{
	ListNode *prev;
	ListNode **list;

	list = ln->ln_head;

	if (*list == ln) {
		/* we are at the head of the list */
		*list = ln->ln_next;
	} else {
		/* walk the list, remembering the "last" element */
		prev = *list;
		for( prev = *list ; prev ; prev = prev->ln_next) {
			if (prev->ln_next == ln) {
				prev->ln_next = ln->ln_next;
			}
		}
	}

	free_listnode(ln);
}


/*
 * walk through the list element by element
 *
 * the list is passed in, as well as the function to call.  A variable
 * number of args are captured via the stdarg interface and passed
 * to the enumeration function
 */
void *
enumerate_listnode(ListNode **list, ListEnumerateFunc func, ...)
{
	ListNode *ln;
	ListNode *next;
	void *result;
	va_list ap;

#ifdef __ppc
	va_list ap2;

	va_start(ap2, func);
#else
	va_start(ap, func);
#endif

	for (ln = *list; ln; ln = next) {
		next = ln->ln_next;	/* protect against removals */
#ifdef __ppc
		va_copy( ap, ap2 );
#endif
		result = (*func)(ln, ln->ln_data, ap);

		if (result) return (result);

	}

	return (NULL);
}



static ListNode *freelist = NULL;
/*
 * allocate a new list node
 */
static ListNode *
alloc_listnode()
{
	ListNode *ln;

	if (freelist != NULL) {
		ln = freelist;
		rem_listnode(ln);
	} else {
		ln = ck_malloc(sizeof *ln);
	}

	return (ln);
}


static void
free_listnode(ListNode *ln)
{
	add_listnode(&freelist, ln);
}






