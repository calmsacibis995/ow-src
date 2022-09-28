#ifndef lint
static char sccsid[] = "@(#)ce_search.c	3.1 04/03/92 Copyright 1989 Sun Microsystems, Inc.";
#endif lint

/* @(#)ce_search.c @(#)ce_search.c	1.3 9/3/90
 * Utilities to search for namespaces, entries and attributes in the
 * CE in memory database
 */
#include "ce_int.h"
#define NULL 0

/*
 * Return ptr to attribute  in an attribute array, if the attribute
 * exists, or NULL otherwise
 */

CE_ATTRIBUTE
find_attr (namespace, attr_name)
CE_NAMESPACE namespace;
char * attr_name;
{
	CE_ATTRIBUTE 	curr_attr;
	
	if (attr_name == NULL)	
		return NULL;

	if (namespace->attr_def == NULL)
		return NULL;
	else	
	    /* search for the attribute */
	    for (curr_attr = namespace->attr_def; curr_attr != NULL;
		 curr_attr = curr_attr->next_attr)
		    if (strcmp (attr_name, curr_attr->a_name) == 0)
			    /* found the attribute, return its id */
			    return (curr_attr);
	    return NULL;
    
}

CE_NAMESPACE
find_ns (db_p, ns_name)
CE_DB	*db_p;
char *ns_name;
{
	CE_NAMESPACE namespace;

	if (!db_p)
		return (NULL);
	
	if (ns_name == NULL)
		return NULL;

	for (namespace = db_p->namespaces; namespace != NULL;
	     namespace = namespace->next_ns)
		if (!is_deleted (namespace->flags))
			if (strcmp (ns_name, namespace->ns_name) == 0)      
				return namespace;

	return NULL;
}

int
attr_cmp (av, attr_name)
CE_AV	*av;
char	*attr_name;
{
	return (strcmp (av->a_id, attr_name));
	
}

CE_AV *
find_av (entry, attribute)
     CE_ENTRY entry;
     CE_ATTRIBUTE attribute;
{
	CE_AV	*av;
	
  
	if (entry->avs == NULL)
		return NULL;
	else    {
		/* loop through avs, looking for a match on attribute ids */
		for (av = entry->avs; av != NULL; av = av->next_av)
			if (!is_deleted (av->flags))
				if (attr_cmp (av, attribute->a_name) == 0)
					return av;
	}
	return NULL;
}
