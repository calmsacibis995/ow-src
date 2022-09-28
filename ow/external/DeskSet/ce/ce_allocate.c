#ifndef lint
static char sccsid[] = "@(#)ce_allocate.c	3.1 04/03/92 Copyright 1989 Sun Microsystems, Inc.";
#endif lint

/*	@(#)ce_allocate.c	1.3	1/15/91
 * Routines to allocate/build new CE structures during database update
 */
#include	"ce_int.h"
extern	char	*strdup ();

int
add_ns(namespace, ns_name, namespace_ptr)
CE_NAMESPACE    namespace;
char           *ns_name;
CE_NAMESPACE   *namespace_ptr;
{
	CE_NAMESPACE	curr_ns;
	
	if (!(curr_ns = (CE_NAMESPACE) malloc(sizeof(CE_NS))))
		return (CE_ERR_NO_MEMORY);
	(void)memset(curr_ns, 0, sizeof (CE_NS));

	if (!(curr_ns->ns_name = (char *)strdup(ns_name)))
		return (CE_ERR_NO_MEMORY);
	
		
	curr_ns->num_attrs = 0;
	curr_ns->attr_def = NULL;
	curr_ns->num_ents = 0;
	curr_ns->entries = NULL;
	curr_ns->next_ns = namespace->next_ns;
	curr_ns->flags = 0;


	/* the new namespace is the last namespace */
	unset_last(namespace->flags);
	set_last(curr_ns->flags);
	set_new(curr_ns->flags);
	namespace->next_ns = curr_ns;
	*namespace_ptr = curr_ns;
	
	return (0);

}

int
add_first_ns(ns_name, namespace_ptr)
char           *ns_name;
CE_NAMESPACE   *namespace_ptr;
{
	CE_NAMESPACE	curr_ns;
	
	if (!(curr_ns = (CE_NAMESPACE) malloc(sizeof(CE_NS))))
		return (CE_ERR_NO_MEMORY);
	(void)memset(curr_ns, 0, sizeof (CE_NS));

	if (!(curr_ns->ns_name = (char *)strdup(ns_name)))
		return (CE_ERR_NO_MEMORY);
	
		
	curr_ns->num_attrs = 0;
	curr_ns->attr_def = NULL;
	curr_ns->num_ents = 0;
	curr_ns->entries = NULL;
	curr_ns->next_ns = NULL;
	curr_ns->flags = 0;


	/* the new namespace is the last namespace */
	set_last(curr_ns->flags);
	set_new(curr_ns->flags);
	*namespace_ptr = curr_ns;
	
	return (0);

}

int
alloc_entry(namespace, entry, entry_ptr)
	CE_NAMESPACE    namespace;
	CE_ENTRY        entry;
	CE_ENTRY      *entry_ptr;

{
	CE_ENTRY	curr_entry;
	
	if (!(curr_entry = (CE_ENTRY) malloc(sizeof(CE_ENT))))
		return (CE_ERR_NO_MEMORY);
	(void)memset (curr_entry, 0, sizeof (CE_ENT));
	
	curr_entry->avs = NULL;
	curr_entry->match_size = 0;
	curr_entry->match_vals.match_vals_val = NULL;
	curr_entry->flags = 0;

	
	/* is this the first entry in the namespace? */
	if (!entry) {
		namespace->entries = curr_entry;
		namespace->num_ents++;
		curr_entry->ent_id = namespace->num_ents;
		set_last(curr_entry->flags);
		set_new(curr_entry->flags);
		curr_entry->next_entry = NULL;
		set_alloced(curr_entry->flags);
		/* XXX	curr_entry->db_id = namespace->db_id; */
		*entry_ptr = curr_entry;

		return (0);
	} else {
		/*
		 * make the new entry the last entry in this
		 * namespace
		 */
		namespace->num_ents++;
		curr_entry->ent_id = namespace->num_ents;
		curr_entry->db_id = entry->db_id;
		curr_entry->next_entry = entry->next_entry;
		set_last(curr_entry->flags);
		set_new(curr_entry->flags);
		unset_last(entry->flags);
		set_alloced(curr_entry->flags);
		
		entry->next_entry = curr_entry;
		*entry_ptr = curr_entry;
		
		return (0);
	}

}

int
alloc_ns_entry(namespace, entry, entry_ptr)
	CE_NAMESPACE    namespace;
 	CE_ENTRY        entry;
	CE_ENTRY      *entry_ptr;

{
	CE_ENTRY	curr_entry;
	
	if (!(curr_entry = (CE_ENTRY) malloc(sizeof(CE_ENT))))
		return (CE_ERR_NO_MEMORY);
	
	(void)memset (curr_entry, 0, sizeof (CE_ENT));
	
	curr_entry->avs = NULL;
	curr_entry->match_size = 0;
	curr_entry->match_vals.match_vals_val = NULL;
	curr_entry->flags = 0;
/* XXX	curr_entry->db_id = entry->db_id; */
	
	/* is this the first entry in the namespace? */
	if (!entry) {
		namespace->entries = curr_entry;
		namespace->num_ents++;
		curr_entry->ent_id = namespace->num_ents;
		set_last(curr_entry->flags);
		set_new(curr_entry->flags);
		curr_entry->next_entry = NULL;
		set_alloced(curr_entry->flags);
		set_ns_entry(curr_entry->flags);
		*entry_ptr = curr_entry;
		
		return (0);
	} else {
		/*
		 * make the new entry the first entry in the
		 * namespace
		 */
		/*
		 * make the old entry the next entry in this
		 * namespace
		 */
		namespace->num_ents++;
		curr_entry->ent_id = 1;
		entry->ent_id = namespace->num_ents;
		
		curr_entry->next_entry = entry;
		namespace->entries = curr_entry;
		set_new(curr_entry->flags);
		set_alloced(curr_entry->flags);
		set_ns_entry(curr_entry->flags);
		*entry_ptr = curr_entry;

		return (0);
	}
}

int
add_attr(namespace, attr_name, attr_ptr)
	CE_NAMESPACE    namespace;
	char           *attr_name;
	CE_ATTR        **attr_ptr;

{
	CE_ATTR        	*curr_attr;
	CE_ATTR		*new_attr;
	
	/* is this attr in the namespace already? */
	for (curr_attr = namespace->attr_def; curr_attr != NULL;
	     curr_attr = curr_attr->next_attr) {
		if (strcmp(curr_attr->a_name, attr_name) == 0) {
			*attr_ptr = curr_attr;
			return (0);
		}
		if (is_last(curr_attr->flags))
			break;
	}

	/* we have come this far, we have to make a new attr */
	if (!(new_attr = (CE_ATTR *) malloc(sizeof(CE_ATTR))))
		return (CE_ERR_NO_MEMORY);
	
	(void)memset (new_attr, 0, sizeof (CE_ATTR));
	
	if (!(new_attr->a_name = strdup (attr_name)))
		return (CE_ERR_NO_MEMORY);
	
	namespace->num_attrs++;

	if (curr_attr != NULL) {
		new_attr->a_id = curr_attr->a_id + 1;
		new_attr->next_attr = curr_attr->next_attr;
		curr_attr->next_attr = new_attr;
		unset_last(curr_attr->flags);
		set_last(new_attr->flags);
		set_new(new_attr->flags);
	} else {
		new_attr->next_attr = NULL;
		new_attr->a_id = namespace->num_attrs;
		set_last(new_attr->flags);
		set_new(new_attr->flags);
		namespace->attr_def = new_attr;
	}
	*attr_ptr = new_attr;
	return (0);

}



int
add_av(namespace, entry, av, attr_name, attr_type, attr_value, attr_size)
	CE_NAMESPACE    namespace;
	CE_ENTRY        entry;
	CE_AV          *av;
	char           *attr_name;
	char           *attr_type;
	char           *attr_value;
	int             attr_size;

{
	CE_AV          *new_av;
	CE_ATTR        *new_attr;

	if (!(new_av = (CE_AV *) malloc(sizeof(CE_AV))))
		return (CE_ERR_NO_MEMORY);
	
	(void)memset (new_av, 0, sizeof (CE_AV));
	
	if (!(new_av->a_type = strdup (attr_type)))
		return (CE_ERR_NO_MEMORY);
	if (!(new_av->a_val = strdup (attr_value)))
		return (CE_ERR_NO_MEMORY);
	
	new_av->a_size = attr_size;


	if (add_attr(namespace, attr_name, &new_attr) == 0)	{
		if (!(new_av->a_id = strdup (new_attr->a_name)))
			return (CE_ERR_NO_MEMORY);
	}
	else
		return (CE_ERR_NO_MEMORY);

	/*
	 * is the passed in av NULL, in which case we're adding the
	 * first av to the entry
	 */
	if (!av) {
		entry->avs = new_av;
		set_last(new_av->flags);
		set_new(new_av->flags);
	} else {
		new_av->next_av = av->next_av;
		av->next_av = new_av;
		set_last(new_av->flags);
		set_new(new_av->flags);
		unset_last(av->flags);
	}
	return (0);

}
