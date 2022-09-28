
/* external/DeskSet/mailtool/list.h */

/* @(#)list.h 1.1 94/05/05 Copyright 1994 Sun Microsystems */

#ifndef _mailtool_list_h
#define _mailtool_list_h

#include <stdarg.h>

typedef struct listnode {
	struct listnode *ln_next;
	struct listnode **ln_head;
	void *ln_data;
} ListNode;

typedef void * (*ListEnumerateFunc)(ListNode *ln, void *data, va_list va);


/* public functions */
ListNode *add_listnode(ListNode **list, void *data);
void rem_listnode(ListNode *ln); 
void *enumerate_listnode(ListNode **list, ListEnumerateFunc func, ...);


#endif _mailtool_list_h
