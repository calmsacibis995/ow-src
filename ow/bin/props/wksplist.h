/* Copyright */

#ifndef WKSPLIST_H
#define WKSPLIST_H

#pragma ident	"@(#)wksplist.h	1.2	92/10/29 SMI"


typedef enum {
	SUCCESS,
	NULL_LIST,
	NULL_ELEMENT,
	ALREADY_IN_LIST,
	LIST_NOT_EMPTY,
	ELEMENT_NOT_FOUND,
	DATA_NOT_FREED
} ListStatus;


typedef struct listElement {
	struct listElement	*previous;
	struct listElement	*next;
	void			*data;
} ListElement;

typedef struct headTail{
	ListElement		*head;
	ListElement		*tail;
} HeadTail;


extern ListElement	*createListElement(
				void);

extern HeadTail		*createList(
				void);

extern ListStatus	 appendAfter(
				HeadTail	*const	list,
				ListElement	*const	insertionPoint,
				ListElement	*const	newElement);

extern ListStatus	 prependBefore(
				HeadTail	*const	list,
				ListElement	*const	newElement,
				ListElement	*const	insertionPoint);

extern ListStatus	 deleteFromList(
				HeadTail	*const	list,
				ListElement	*const	elementToDelete);

extern ListStatus	 inList(
			const	HeadTail	*const	list,
			const	ListElement	*const	element);

#endif /* WKSPLIST_H */
