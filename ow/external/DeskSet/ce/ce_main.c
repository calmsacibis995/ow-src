#ifndef lint
static char sccsid[] = "@(#)ce_main.c	3.8 11/07/96 Copyright 1989 Sun Microsystems, Inc.";
#endif lint

/*
 * @(#)ce_main.c 1/23/91 @(#)ce_main.c	1.18 API routines for CE
 */
#include <varargs.h>
#include <dlfcn.h>
#include <sys/param.h>
#include "ce_int.h"

#define NULL 0
MASTER_DB master_db;
extern char **db_paths;

static CE_DB    user_db;
static CE_DB    system_db;
static CE_DB    network_db;
static char	**all_db_paths = NULL;
static char	**all_db_names = NULL;


extern CE_NAMESPACE find_ns();
extern CE_ATTRIBUTE find_attr();
extern CE_AV   *find_av();

/* Access a Name Space */
CE_NAMESPACE
ce_get_namespace_id_from_db(database_type, namespace_name)
	int database_type;
	char           *namespace_name;
{
	CE_NAMESPACE    namespace;
	int i;

        for (i = 0; i < master_db.num_db; i++) 
		if (database_type == USER_DB && 
			strcmp(master_db.db_names[i], user_db_name) == 0) 
			break;
		else if (database_type == SYSTEM_DB && 
			strcmp(master_db.db_names[i], system_db_name) == 0) 
			break;
		else if (database_type == NETWORK_DB && 
			strcmp(master_db.db_names[i], network_db_name) == 0) 
			break;

	if (i == master_db.num_db) 
		return NULL;

	if (namespace = (CE_NAMESPACE) find_ns(master_db.ce_db_ptrs[i], namespace_name)) 
			return (namespace);
	return (NULL);
	
}

/* Access a Name Space */
CE_NAMESPACE
ce_get_namespace_id(namespace_name)
	char           *namespace_name;
{
	int             i;
	CE_NAMESPACE    namespace;

	for (i = 0; i < master_db.num_db; i++)
		if (namespace = (CE_NAMESPACE) find_ns(master_db.ce_db_ptrs[i], namespace_name))
			return (namespace);
	return (NULL);
	
}

/* Access an entry in a name space */
CE_ENTRY
ce_get_entry(va_alist)
va_dcl
{
	va_list         ap;
	CE_NAMESPACE    namespace;
	int             argcount;
	int             i;
	CE_ENTRY        entry;
	int             sts;
	void	        *result;
	int		db_index;
	CE_NAMESPACE	curr_ns;
	

	va_start(ap);
	/* get the namespace */
	namespace = va_arg(ap, CE_NAMESPACE);

	/* Fix for 1226588 - filemgr dumps core with -C option */
	if (namespace == NULL)
		return(NULL);

	/* get the argument count to get_entry */
	argcount = va_arg(ap, int);

	/*
	 * loop through entries, calling the namespace match routine with
	 * each
	 */
	db_index = namespace->db_id;
	curr_ns = namespace;
	do	{
		if (curr_ns)
			for (entry = curr_ns->entries; entry != NULL;
			     entry = entry->next_entry) {
				if (!is_ns_entry(entry->flags) &&
				    !is_deleted(entry->flags)) {
#if defined(__ppc)
					va_list         ap_pass;
					va_copy( ap_pass, ap );
#endif
					sts = 
						(*namespace->ns_mgr_ptr->match_func)
							(argcount,
#if !defined(__ppc)
							 ap,
#else
							 ap_pass,
#endif /* __ppc */
							 entry->match_size,
							 entry->match_vals.match_vals_val,
							 entry,
							 &result);
					if (sts == 0)
						return ((CE_ENTRY) result);
			}
		}
		
		db_index++;
		if (db_index < master_db.num_db)
			curr_ns = find_ns (master_db.ce_db_ptrs [db_index], namespace->ns_name);
	}
	while (db_index < master_db.num_db);

	va_end(ap);
	return (NULL);
}

/* Get an attribute handle */
CE_ATTRIBUTE
ce_get_attribute_id(namespace, attr_name)
	CE_NAMESPACE    namespace;
	char           *attr_name;
{
	CE_ATTRIBUTE    attribute;
	int 		db_index;
	CE_NAMESPACE	curr_ns;
	
	db_index = namespace->db_id;
	curr_ns = namespace;
	do	{

		if (curr_ns)	{
			attribute = find_attr(curr_ns, attr_name);
			if (attribute != NULL)
				return attribute;
		}
		
		db_index++;
		if (db_index < master_db.num_db)
			curr_ns = find_ns (master_db.ce_db_ptrs[db_index], namespace->ns_name);
		
	}
	while (db_index < master_db.num_db);
	return (CE_ATTRIBUTE)0;
}



/* get an attribute value */
char *
ce_get_attribute(namespace, entry, attribute)
	CE_NAMESPACE    namespace;
	CE_ENTRY        entry;
	CE_ATTRIBUTE    attribute;
{
	CE_AV          *this_av, *curr_av;
	CE_AV          *search_av;
	CE_ENT         *search_entry;
	char           *this_entry_cookie;
	int		db_index;
	CE_NAMESPACE	curr_ns;


	if (attribute == NULL)
		return NULL;

	this_av = find_av(entry, attribute);
	if (this_av == NULL) {

		/* does this entry have a cookie attribute? */
		for (search_av = entry->avs; search_av != NULL;
		     search_av = search_av->next_av)
			if (attr_cmp(search_av, chained_to_attr) == 0)
				break;
		if (search_av == NULL)
			return NULL;

		this_entry_cookie = search_av->a_val;

		
		for (db_index = namespace->db_id + 1; /* the next database */
		     db_index < master_db.num_db;
		     db_index++)	{
			
		       if (curr_ns = find_ns (master_db.ce_db_ptrs [db_index],
					   namespace->ns_name))	{
			       
			       for (search_entry = curr_ns->entries; search_entry != NULL;
				    search_entry = search_entry->next_entry)	{
				       if (search_entry == entry)
					       break;
				       if (match_cookie(namespace, search_entry, this_entry_cookie) == 0) {
					       return (ce_get_attribute(curr_ns, search_entry, attribute));
				       }
			       }
			       
		       }
	       }
		
	} else
		return this_av->a_val;

}

/* get an attribute type */
char *
ce_get_attribute_type(namespace, entry, attribute)
	CE_NAMESPACE    namespace;
	CE_ENTRY        entry;
	CE_ATTRIBUTE    attribute;
{
	CE_AV          *this_av, *curr_av;
	CE_AV          *search_av;
	CE_ENT         *search_entry;
	char           *this_entry_cookie;
	int		db_index;
	CE_NAMESPACE	curr_ns;

	if (attribute == NULL)
		return NULL;

	this_av = find_av(entry, attribute);
	if (this_av == NULL) {

		/* does this entry have a cookie attribute? */
		for (search_av = entry->avs; search_av != NULL;
		     search_av = search_av->next_av)
			if (attr_cmp(search_av, chained_to_attr) == 0)
				break;
		if (search_av == NULL)
			return NULL;

		this_entry_cookie = search_av->a_val;
		
		for (db_index = namespace->db_id + 1; /* the next database */
		     db_index < master_db.num_db;
		     db_index++)
		       if (curr_ns = find_ns (master_db.ce_db_ptrs [db_index],
					   namespace->ns_name))	{
			       
			       for (search_entry = curr_ns->entries; search_entry != NULL;
				    search_entry = search_entry->next_entry)	{
				       if (search_entry == entry)
					       break;
				       
				       if (match_cookie(namespace, search_entry, this_entry_cookie) == 0) {
					       return (ce_get_attribute_type
						       (curr_ns, 
							search_entry, attribute));
				       }
			       }
			       
		       }

	} else
		return this_av->a_type;

}

/* get the size of an attribute */
int
ce_get_attribute_size(namespace, entry, attribute)
	CE_NAMESPACE    namespace;
	CE_ENTRY        entry;
	CE_ATTRIBUTE    attribute;
{
	CE_AV          *this_av, *search_av, *curr_av;
	CE_ENT         *search_entry;
	char           *this_entry_cookie;
	int		db_index;
	CE_NAMESPACE	curr_ns;

	if (attribute == NULL)
		return NULL;

	this_av = find_av(entry, attribute);
	if (this_av == NULL) {

		/* does this entry have a cookie attribute? */
		for (search_av = entry->avs; search_av != NULL;
		     search_av = search_av->next_av)
			if (attr_cmp(search_av, chained_to_attr) == 0)
				break;
		if (search_av == NULL)
			return NULL;

		this_entry_cookie = search_av->a_val;

		for (db_index = namespace->db_id + 1;
		     db_index < master_db.num_db;
		     db_index++)
			if (curr_ns = find_ns (master_db.ce_db_ptrs [db_index],
					   namespace->ns_name))	{
			       
			       for (search_entry = curr_ns->entries; search_entry != NULL;
				    search_entry = search_entry->next_entry)	{
				       if (search_entry == entry)
					       break;
				       
				       if (match_cookie(namespace, 
							search_entry, 
							this_entry_cookie) == 0) {
					       return (ce_get_attribute_size
						       (curr_ns,
							search_entry, attribute));
				       }
			       }
			       
		       }
	} else
		return this_av->a_size;
}

/* get a namespace entry */
CE_ENTRY
ce_get_ns_entry(namespace)
	CE_NAMESPACE    namespace;
{
	CE_ENTRY        curr_entry;
	int 		db_index;
	CE_NAMESPACE	curr_ns;
	
	db_index = namespace->db_id;
	curr_ns = namespace;
	do	{
		if (curr_ns)
			for (curr_entry = curr_ns->entries; curr_entry != NULL;
			     curr_entry = curr_entry->next_entry)
				if (is_ns_entry(curr_entry->flags))
					return (curr_entry);
		db_index++;
		if (db_index < master_db.num_db)
			curr_ns = find_ns (master_db.ce_db_ptrs [db_index],
				   namespace->ns_name);
	}
	while (db_index < master_db.num_db);
	
	return (NULL);

}

/* map through a namespace */
void *
ce_map_through_namespaces(map_func, args)
void		*(*map_func) ();
void         	*args;
{
	CE_NAMESPACE    this_ns_handle;
	void            *result;
	int             i;

	for (i = 0; i < master_db.num_db; i++)
		for (this_ns_handle = master_db.ce_db_ptrs[i]->namespaces; this_ns_handle != NULL;
		     this_ns_handle = this_ns_handle->next_ns) {
			if (!is_connected(this_ns_handle->flags))
				if ((result = (*map_func) (this_ns_handle, args)) != NULL)
					return result;
		}
	return NULL;


}

/* map through entries */
void *
ce_map_through_entries(namespace, map_func, args)
CE_NAMESPACE    namespace;
void		*(*map_func) ();
void  	        *args;
{
	CE_ENTRY        curr_entry, s_e;
	void	        *result;
	int             i;
	int		db_index, next_index;
	CE_NAMESPACE	curr_ns, next_ns;
	CE_AV		*search_av;
	char		*cookie;
	
	
	/* loop through entries in this namespace */
	db_index = namespace->db_id;
	curr_ns = namespace;
	do	{
		if (!curr_ns)
			goto find_next_ns;
		
		for (curr_entry = curr_ns->entries;
		     curr_entry != NULL;
		     curr_entry = curr_entry->next_entry)	{
			
			/* don't map thru namespace entries */
			if (!is_ns_entry (curr_entry->flags) &&
			    !is_deleted (curr_entry->flags))	{
				if (!is_chained_to (curr_entry->flags))	{
						
					
					/* does this entry have a cookie attribute? */
					for (search_av = curr_entry->avs; search_av != NULL;
					     search_av = search_av->next_av)
						if (attr_cmp(search_av, chained_to_attr) == 0)
							break;
					if (search_av != NULL)	{
						cookie = search_av->a_val;
						
		
						for (next_index = db_index + 1; /* the next database */
						     next_index < master_db.num_db;
						     next_index++)
							if (next_ns = find_ns (master_db.ce_db_ptrs [next_index],
									       namespace->ns_name))	{
								
								for (s_e = next_ns->entries; s_e != NULL;
								     s_e = s_e->next_entry)
									if (match_cookie(namespace, s_e, cookie) == 0) 
										set_chained_to 
											(s_e->flags);
							}
					}						
					
					if (result = (*map_func) (namespace, curr_entry, args))
						return (result);
				}
			}
		}			
				
find_next_ns:		
		db_index++;
		if (db_index < master_db.num_db)
			curr_ns = find_ns (master_db.ce_db_ptrs [db_index], namespace->ns_name);
	}
	while (db_index < master_db.num_db);

	return NULL;
}

/* map through attributes */
void *
ce_map_through_attrs(namespace, entry, map_func, args)
CE_NAMESPACE    namespace;
CE_ENTRY        entry;
void		*(*map_func) ();
void	        *args;
{
	void            *result;
	int             i;
	CE_ATTRIBUTE    this_attr;
	CE_AV          *this_av;
	int		db_index;
	CE_NAMESPACE	curr_ns;
	char		*a_val;
	char	**tmp_attr_list;
	int	tmp_size;
	
	
	/* ok, loop thru all the attrs in this namespace,
	 * checking to see if this entry has an av with the same name as
	 * the attr names
	 * It's sorta backwards, I know,  but it's the only way I know to
	 * catch *all* the avs in this entry, allowing for chaining
	 */
	tmp_attr_list = (char **)malloc (sizeof (char *));
	*tmp_attr_list = (char *)strdup ("");
	db_index = namespace->db_id;
	curr_ns = namespace;
	
	do	{
		if (curr_ns)
			for (this_attr = curr_ns->attr_def;
			     this_attr != NULL;	
			     this_attr = this_attr->next_attr)	{
				if ((!(strstr (*tmp_attr_list, this_attr->a_name)))
				    && (a_val = ce_get_attribute (namespace, entry,
								  this_attr)))	{
					tmp_size = strlen (*tmp_attr_list) + 1;
					*tmp_attr_list = (char *)realloc(*tmp_attr_list, tmp_size +
						     (int) strlen (this_attr->a_name) +1);
					
					(void)strcat (*tmp_attr_list, 
						      this_attr->a_name);
					
					if ((result = (*map_func) (this_attr, a_val,
								   args)) != NULL)
						return result;
				}
				
			}
		
		db_index++;
		if (db_index < master_db.num_db)
			curr_ns = find_ns (master_db.ce_db_ptrs [db_index],
				   namespace->ns_name);
	}
	while (db_index < master_db.num_db);
	
	free (*tmp_attr_list);
	free (tmp_attr_list);
	
	return (NULL);

}


/* map through the attributes of a namespace */
void *
ce_map_through_ns_attrs(namespace, map_func, args)
CE_NAMESPACE    namespace;
void		*(*map_func) ();
void         	*args;
{
	CE_ENTRY        entry;

	if (entry = ce_get_ns_entry (namespace))
		return (ce_map_through_attrs (namespace, entry, map_func, args));
}


/* given a namespace handle, return its name */
char *
ce_get_namespace_name(namespace)
	CE_NAMESPACE    namespace;

{
	return (namespace->ns_name);
}

/* given an attribute handle, return the attribute name */
char *
ce_get_attribute_name(attribute)
	CE_ATTRIBUTE    attribute;
{
	return (attribute->a_name);
}

/* read the CE database - if it has been modified since the last call */
int
ce_begin(args)
	void           *args;	/* assumed to be NULL for now */

{

	CE_NS          *curr_ns;
	CE_ENT         *curr_entry;
	CE_AV          *curr_av;
	CE_ATTR        *curr_attr;
	int             mgr_id;
	int             status;
	char           *temp;
	int             size;
	int             i, j;
	CE_ATTRIBUTE    ns_mgr_attr;
	char           *shared_lib_path;
	char            mgr_name[MAXPATHLEN + 1];
	int             num_func;
	void          **func_ptrs;
	time_t		db_mtime;
	BOOLEAN		at_least_one = FALSE;
	
	

	/* init all structures */
	init_master(&master_db);

	if (load_databases (&master_db) != 0)
		return (CE_ERR_ERROR_READING_DB);
	
	if (master_db.num_db == 0)
		return (CE_ERR_ERROR_READING_DB);


	make_master_info(&master_db);

	return (load_ns_managers (&master_db));
	
}

/* return 0 if CE databases have been unchanged since the last call to ce_begin
 * return 1 if they have 
 */
int
ce_db_changed ()
{
	int i;
	int status;
	time_t db_mtime;
	int	dd = 0;
	
	for (i = 0; i < master_db.num_db; i++)	{
		if ((status = perform_db_op (NULL, NULL,
					     SET_DB_FILENAME, 
					     (int *) master_db.ce_db_ptrs[i]->
					     db_path)) != 0)
			return (1);
		if ((status = perform_db_op (NULL, NULL, OPEN_DB_FOR_READ,
					     &dd)) != 0)
			return (1);
		
		if (master_db.db_mtimes[i] != 
		    (time_t)get_db_mtime (&dd))	{
			/* close the database */
			status = perform_db_op (NULL, NULL, CLOSE_DB, &dd);
			return (1);
		}
		else	
			/* close the database */
			status = perform_db_op (NULL, NULL, CLOSE_DB, &dd);				
	}
	return (0);
}

int
ce_end ()
{
	int i;

	free (master_db.db_mtimes);
	
	for (i = 0; i < master_db.num_db; i++)	{
		free_ce_db (master_db.ce_db_ptrs [i]);
		/* XXX free (master_db.db_mtimes [i]); */
		/* XXX free (master_db.db_names [i]); */
		/* free (master_db.db_paths [i]); */
		/* XXX free (master_db.ce_db_ptrs [i]); */
	}
	master_db.num_db = 0;
	
	return (0);
	
}

int
ce_get_entry_db_info (namespace, entry, name_ptr, path_ptr)
CE_NAMESPACE namespace;
CE_ENTRY entry;
char	**name_ptr;
char	**path_ptr;

{
	CE_ENTRY        curr_entry;
	void	        *result;
	int             i;
	int		db_index;
	CE_NAMESPACE	curr_ns;

	if (!name_ptr && !path_ptr)
		return (CE_ERR_WRONG_ARGUMENTS);
	
	/* loop through entries in this namespace */
	db_index = namespace->db_id;
	curr_ns = namespace;
	do	{
		if (curr_ns)
			for (curr_entry = curr_ns->entries;
			     curr_entry != NULL;
			     curr_entry = curr_entry->next_entry)	{

				if (entry == curr_entry)	{
					*name_ptr = master_db.db_names [db_index];
					*path_ptr = master_db.ce_db_ptrs [db_index]->
						db_path;
					return (0);
					
				}
				
				
			}
		db_index++;
		if (db_index < master_db.num_db)
			curr_ns = find_ns (master_db.ce_db_ptrs [db_index], namespace->ns_name);
	}
	while (db_index < master_db.num_db);

	return (CE_ERR_WRONG_ARGUMENTS);
	
}

int
ce_get_dbs (num_db, db_names, db_pathnames)
int *num_db;
char ***db_names;
char ***db_pathnames;


{
	int i, j;
	extern char *get_default_db_path() ;

	if (all_db_paths == NULL)
		all_db_paths = (char **)malloc (sizeof (char *) * 3);
	if (all_db_names == NULL)
		all_db_names = (char **)malloc (sizeof (char *) * 3);

	j = 0;
	
	for (i = USER_DB; i <= NETWORK_DB; i++)	{

		if (i == USER_DB)
			all_db_names [j] = user_db_name;
		else if (i == SYSTEM_DB)
			all_db_names [j] = system_db_name;
		else if (i == NETWORK_DB)
			all_db_names [j] = network_db_name;
		if (db_paths[i])	{
			all_db_paths [j] = db_paths [i];
		}
		else
			all_db_paths [j] = (char *) get_default_db_path (i);
		j++;
		
	}
			
	*num_db = j;
	*db_names = all_db_names;
	*db_pathnames = all_db_paths;
	return (0);
}

		
