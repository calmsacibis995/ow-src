#ifndef lint
        static char sccsid[] = "@(#)isdlink.c	1.3\t06/11/93 Copyr 1988 Sun Micro";
#endif

/*
 * Copyright (c) 1988 by Sun Microsystems, Inc.
 */

/*
 * isdlink.c
 *
 * Description:
 *	Double link list functions
 */

#include "isam_impl.h"

/* _isdln_base_insert () - Insert element into list (at the front) -----------*/
void _isdln_base_insert (base,l,e)
	register char		*base;
	register struct dlink	*l,*e;
{
	e->dln_forward = l->dln_forward;
	l->dln_forward = (char *)e - base;
	
	e->dln_backward = (char *)l - base;
	((struct dlink *)(base + e->dln_forward))->dln_backward = (char *)e - base;
}

/* _isdln_base_append () - Append element to list (at the end) -------------*/
void _isdln_base_append (base,l,e)
	register char		*base;
	register struct dlink	*l,*e;
{
	e->dln_backward = l->dln_backward;
	l->dln_backward = (char *)e - base;
	
	e->dln_forward = (char *)l - base;
	((struct dlink *)(base + e->dln_backward))->dln_forward = (char *)e - base;
}

/* _isdln_base_remove () - Remove element from list -------------------------*/
void _isdln_base_remove (base,e)
	register char		*base;
	register struct dlink	*e;
{
	((struct dlink *)(base + e->dln_backward))->dln_forward = e->dln_forward;
	((struct dlink *)(base + e->dln_forward))->dln_backward = e->dln_backward;
}

/* _isdln_base_first () - Return first element of the list -------------------*/
struct dlink * _isdln_base_first(base,l)
	register char		*base;
	register struct dlink    	*l;
{
	return (((struct dlink *)(base + l->dln_forward))); 
}

/* _isdln_base_next () - Return next element in the list --------------------*/
struct dlink * _isdln_base_next(base,l)
	register char		*base;
	register struct dlink    	*l;
{
	return (((struct dlink *)(base + l->dln_forward))); 
}

/* _isdln_base_prev () - Return previous element in the list ----------------*/
struct dlink * _isdln_base_prev(base,l)
	register char		*base;
	register struct dlink    	*l;
{
	return  (((struct dlink *)(base + l->dln_backward))); 
}

/* _isdln_base_makeempty () - Make head of empty list -----------------------*/
void _isdln_base_makeempty(base,l)
	register char		*base;
	register struct dlink    	*l;
{
	l->dln_forward = l->dln_backward = (char *)l - base;
}

/* _isdln_base_isempty () - Test if list is empty---------------------------*/
int _isdln_base_isempty(base,l)
	register char		*base;
	register struct dlink    	*l;
{
	return (l->dln_forward == (char *)l - base);
}
