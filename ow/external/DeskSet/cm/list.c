#ifndef lint
static  char sccsid[] = "@(#)list.c 3.1 92/04/03 Copyr 1991 Sun Microsystems, Inc.";
#endif

/*
 *  Copyright (c) 1987-1990 Sun Microsystems, Inc.
 *  All Rights Reserved.
 *
 *  Sun considers its source code as an unpublished, proprietary
 *  trade secret, and it is available only under strict license
 *  provisions.  This copyright notice is placed here only to protect
 *  Sun in the event the source is deemed a published work.  Dissassembly,
 *  decompilation, or other means of reducing the object code to human
 *  readable form is prohibited by the license agreement under which
 *  this code is provided to the user or company in possession of this
 *  copy.
 *
 *  RESTRICTED RIGHTS LEGEND: Use, duplication, or disclosure by the
 *  Government is subject to restrictions as set forth in subparagraph
 *  (c)(1)(ii) of the Rights in Technical Data and Computer Software
 *  clause at DFARS 52.227-7013 and in similar clauses in the FAR and
 *  NASA FAR Supplement.
 */ 

#include <stdio.h>
#include <sys/param.h>
#include "util.h"
#include "tree.h"
#include "list.h"

extern int debug;

typedef struct {
	Get_key get;
	Enumerate_proc enumerate;
	Compare_proc compare;
} Private;


extern	Lnode *
hc_lookup_node (hc_list,key)
Hc_list	*hc_list;
Key	key;
{
	Private		*private;
	register Lnode	*p_node;
	register Comparison	result;

	p_node = hc_list->root;
	private = (Private *) hc_list->private;
	while (p_node != NULL)
	{
		result = private->compare (key, p_node->data);
		if (result == greater)
		{
			p_node = hc_lookup_next (p_node);
		}
		else if (result == equal)
		{
			return (p_node);
		}
		else
		{
			break;
		}
	}
	return (NULL);
}

extern	Data
hc_lookup (hc_list,key)
Hc_list	*hc_list;
Key	key;
{
	Lnode	*p_node;

	p_node = hc_lookup_node (hc_list, key);
	if (p_node != NULL)
		return ((Data) p_node->data);
	return (NULL);
}

extern	int
hc_size (hc_list)
Hc_list	*hc_list;
{
	int		n = 0;
	register Lnode	*p_node;

	p_node = hc_list->root;
	while (p_node != NULL)
	{
		n++;
		p_node = hc_lookup_next (p_node);
	}
	return (n);
}


extern Hc_list *
hc_create(get, compare)
Get_key get;
Compare_proc compare;
{
	Private	*p;
	Lnode	*root = NULL;
	Hc_list	*list;

	p = (Private *) ckalloc (sizeof (*p));

	list = (Hc_list *) ckalloc (sizeof (*list));
	list->root = NULL;
	list->private = (caddr_t) p;

	p->get = get;
	p->enumerate = NULL;
	p->compare = compare;

	return (list);
}

extern void
hc_destroy(hc_list,destroy_func)
Hc_list	*hc_list;
int	(*destroy_func)();
{
	Private	*p;
	Lnode	*p_node, *p_next;

	if (hc_list == NULL)
		return;
	if ((p = (Private *) hc_list->private) != NULL)
		free (p);

	p_node = hc_list->root;
	while (p_node != NULL)
	{
		p_next = hc_lookup_next(p_node);
		if (p_node->data != NULL)
		{
			if (destroy_func != NULL)
				(*destroy_func) (p_node->data);
		}
		free (p_node);
		p_node = p_next;
	}
}

extern Rb_Status
hc_insert_node (hc_list,p_node,key)
Hc_list	*hc_list;
register Lnode *p_node;
Key	key;
{
	Private	*private;
	register Lnode	*p_curr;

	if (hc_list == NULL)
		return (rb_notable);
	private = (Private *) hc_list->private;

	p_curr = hc_list->root;
	while (p_curr != NULL)
	{
		if (private->compare (key, p_curr->data) == greater)
		{
			if (p_curr->rlink != NULL)
				p_curr = p_curr->rlink;
			else
			{
				/* Insert at end of the list */
				p_curr->rlink = p_node;
				p_node->llink = p_curr;
				return (rb_ok);
			}
		}
		else
		{
			/* Insert at head of the list */
			if ((p_node->llink = p_curr->llink) == NULL)
				break;

			/* Insert before the current node */
			p_curr->llink = p_node->llink->rlink = p_node;
			p_node->rlink = p_curr;
			return (rb_ok);
		}
	}

	/* Insert at head of the list */
	p_node->rlink = hc_list->root;
	if (p_node->rlink != NULL)
		p_node->rlink->llink = p_node;
	hc_list->root = p_node;
	return (rb_ok);
}

extern Rb_Status
hc_insert (hc_list,data,key)
Hc_list	*hc_list;
Data	data;
Key	key;
{
	Lnode	*p_node;

	if (hc_list == NULL)
		return (rb_notable);
	p_node = (Lnode *) ckalloc (sizeof(*p_node));
	p_node->data = data;
	return (hc_insert_node (hc_list, p_node, key));
}

extern Lnode *
hc_delete_node (hc_list,p_node)
Hc_list *hc_list;
Lnode	*p_node;
{
	if (p_node->llink == NULL)
		hc_list->root = p_node->rlink;
	else
		p_node->llink->rlink = p_node->rlink;
	if (p_node->rlink != NULL)
		p_node->rlink->llink = p_node->llink;
	return (p_node);
}

extern Lnode *
hc_delete (hc_list,key)
Hc_list	*hc_list;
Key	key;
{
	register Lnode	*p_node;
	register Private *private;

	p_node = hc_list->root;
	private = (Private *) hc_list->private;
	while (p_node != NULL)
	{
		if (private->compare (key, p_node->data) == equal)
		{
			(void) hc_delete_node (hc_list, p_node);
			return (p_node);
		}
		p_node = hc_lookup_next(p_node);
	}
	return (NULL);
}

extern Data
hc_lookup_smallest (hc_list)
Hc_list	*hc_list;
{
	if ((hc_list == NULL) || (hc_list->root == NULL))
		return (NULL);
	return ((Data) hc_list->root->data);
}

extern Data
hc_lookup_next_larger (hc_list, key)
Hc_list	*hc_list;
Key	key;
{
	register Lnode	*p_node;
	register Private *private;

	p_node = hc_list->root;
	private = (Private *) hc_list->private;
	while (p_node != NULL)
	{
		if (private->compare (key, p_node->data) == less)
			return ((Data) p_node->data);
		p_node = hc_lookup_next(p_node);
	}
	return (NULL);
}

extern Data
hc_lookup_largest (hc_list)
Hc_list	*hc_list;
{
	register Lnode	*p_node;

	if ((hc_list == NULL) || (hc_list->root == NULL))
		return (NULL);

	p_node = hc_list->root;
	while (p_node->rlink != NULL)
		p_node = hc_lookup_next (p_node);
	return ((Data) p_node->data);
}

extern Data
hc_lookup_next_smaller (hc_list,key)
Hc_list	*hc_list;
Key	key;
{
	register Lnode	*p_node;
	register Private *private;

	p_node = hc_list->root;
	private = (Private *) hc_list->private;
	while (p_node != NULL)
	{
		if (private->compare (key, p_node->data) != greater)
		{
			if (p_node->llink == NULL)
				return (NULL);
			else
				return ((Data) p_node->llink->data);
		}
		p_node = hc_lookup_next(p_node);
	}
	return (NULL);
}

extern	Rb_Status
hc_check_list (hc_list)
Hc_list	*hc_list;
{
	if ((hc_list == NULL) || (hc_list->root == NULL))
		return (rb_notable);
	return (rb_ok);
}


extern void
hc_enumerate_down(hc_list, doit)
Hc_list	*hc_list;
Enumerate_proc doit;
{
	register Lnode	*p_node;

	p_node = hc_list->root;
	while (p_node != NULL)
	{
		if (doit (p_node->data))
			return;
		p_node = p_node->llink;
	}
}

extern void
hc_enumerate_up(hc_list, doit)
Hc_list	*hc_list;
Enumerate_proc doit;
{
	register Lnode	*p_node;

	p_node = hc_list->root;
	while (p_node != NULL)
	{
		if (doit (p_node->data))
			return;
		p_node = hc_lookup_next (p_node);
	}
}
