#pragma ident	"@(#)wksplist.c	1.3	92/11/11 SMI"

/* Copyright */


/* Includes */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "wksplist.h"

#ifdef DBMALLOC
	#include "dbmalloc.h"
#endif


ListElement *
createListElement(
	void)
{
	ListElement	*newElement;


	newElement = malloc(sizeof(ListElement));
	
	if (newElement != NULL) {
		newElement->next     = NULL;
		newElement->previous = NULL;
		newElement->data     = NULL;
	}

	return newElement;
}


HeadTail *
createList(
	void)
{
	HeadTail	*headTail;
	

	headTail = malloc(sizeof(HeadTail));

	if (headTail != NULL) {
		headTail->head = headTail->tail = NULL;
	}
	
	return headTail;
}


ListStatus
appendAfter(
	HeadTail	*const	list,
	ListElement	*const	insertionPoint,
	ListElement	*const	newElement)
{
	ListStatus	 	status;


	if (list == NULL) {
		status = NULL_LIST;
		
	} else if (newElement == NULL) {
		status = NULL_ELEMENT;
		
	} else if (inList(list, newElement) == SUCCESS) {
		status = ALREADY_IN_LIST;
		
	} else if (insertionPoint == NULL) {
		/*
		 * If insertionPoint is NULL then the list should be empty and
		 * both it's head and tail should point to NULL.  Or the
		 * insertionPoint point is bad.
		 */
		 
		if (   (insertionPoint != list->head)
		    || (insertionPoint != list->tail))
		{
			status = LIST_NOT_EMPTY;
			
		} else { /* Add first element to an empty list. */
			list->head = list->tail = newElement;
			
			status = SUCCESS;
		}
	} else {
		if (insertionPoint == list->tail) {
			list->tail                 = newElement;
			insertionPoint->next       = newElement;
			newElement->previous       = insertionPoint;
			
			status = SUCCESS;
		} else {
			newElement->next           = insertionPoint->next;
			newElement->previous       = insertionPoint;
			newElement->next->previous = newElement;
			insertionPoint->next       = newElement;
			
			status = SUCCESS;
		}
	}
	
	return status;
}


ListStatus
prependBefore(
	HeadTail	*const	list,
	ListElement	*const	newElement,
	ListElement	*const	insertionPoint)
{
	ListStatus		status;


	if (list == NULL) {
		status = NULL_LIST;

	} else if (newElement == NULL) {
		status = NULL_ELEMENT;

	} else if (inList(list, newElement) == SUCCESS) {
		status = ALREADY_IN_LIST;

	} else if (insertionPoint == NULL) {
		/*
		 * If insertionPoint is NULL then the list should be empty and
		 * both it's head and tail should point to NULL.  Or
		 * the insertionPoint point is bad.
		 */
		 
		if (   (insertionPoint != list->head)
		    || (insertionPoint != list->tail))
		{
			status = LIST_NOT_EMPTY;

		} else {
			list->head = list->tail = newElement;
			
			status = SUCCESS;
		}
	} else {
		if (insertionPoint == list->head) {
			list->head                 = newElement;
			insertionPoint->previous   = newElement;
			newElement->next           = insertionPoint;

			status = SUCCESS;
		} else {
			newElement->next           = insertionPoint;
			newElement->previous       = insertionPoint->previous;
			newElement->previous->next = newElement;
			insertionPoint->previous   = newElement;

			status = SUCCESS;
		}
	}
	
	return status;
}


ListStatus
deleteFromList(
	HeadTail	*const list,
	ListElement	*const elementToDelete)
{
	ListStatus	 status;


	/* It's up to the user to free any data pointed to by a list element. */
	 
	elementToDelete->data = NULL;
	
	if (list == NULL) {
		status = NULL_LIST;

	} else if (elementToDelete == NULL) {
		status = NULL_ELEMENT;

	} else if (inList(list, elementToDelete) != SUCCESS) {
		status = ELEMENT_NOT_FOUND;

	} else {
		if (list->head == elementToDelete) {
			/* Handle deleting last item list */
			if (list->head->next == NULL) {
				list->head = NULL;
				list->tail = NULL;
			} else {
				list->head           = elementToDelete->next;
				list->head->previous = NULL;
			}

			status = SUCCESS;
		} else if (list->tail == elementToDelete) {
			list->tail           = elementToDelete->previous;
			list->tail->next     = NULL;

			status = SUCCESS;
		} else {
			elementToDelete->previous->next = elementToDelete->next;
			elementToDelete->next->previous = elementToDelete->previous;

			status = SUCCESS;
		}
	}
	
	if (status == SUCCESS) {
		free(elementToDelete);
	}

	return status;
}


ListStatus
inList(
	const HeadTail		*const	 list,
	const ListElement	*const	 element)
{
	ListStatus			 status;

	const ListElement		*ii;


	if (list == NULL) {
		status = NULL_LIST;
		
	} else if (element == NULL) {
		status = NULL_ELEMENT;
		
	} else {
		ii = list->head;
		while ((ii != element) && (ii != NULL)) {
			ii = ii->next;
		}
		
		if (ii == element) {
			status = SUCCESS;
		} else {
			status = ELEMENT_NOT_FOUND;
		}
	}

	return status;
}

