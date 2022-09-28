#ifndef lint
static char sccsid[] = "@(#)ce_update_api.c	1.4 4/30/91 Copyright 1989 Sun Microsystems, Inc.";
#endif lint

/*
 * @(#)ce_update_api.c	1.15	1/29/91	CE Update API
 */
#include	<sys/errno.h>
#include	"ce_int.h"

extern MASTER_DB master_db;
extern	char	**db_paths;

BOOLEAN         write_in_progress = FALSE;
BOOLEAN		new_database = FALSE;
BOOLEAN		first_namespace = FALSE;

CE_DB          *write_db;

/*
 * writing to a CE database where database can be, USER_DB which is an
 * indication that the user database is to be written to SYSTEM_DB which is
 * an indication that the system database is to be written NETWORK_DB (2)
 * which is an indication that the network database is to be written
 * 
 * ce_start_write () will return 0 if successful, otherwise the following error
 * indications will be returned: CE_ERR_DB_LOCKED if the specified database
 * has already been locked for writing - this client cannot make any ce_add*,
 * ce_delete* or ce_modify* calls until the database is unlocked
 * CE_ERR_NO_PERMISSIONS if the client does not have write permission on the
 * database. CE_ERR_WRITE_IN_PROGRESS if the client has another CE database
 * open for writing by another ce_start_write () call CE_ERR_DB_NOT_LOADED if
 * the named database was not loaded
 */
int
ce_start_write(database)
char           *database;
{
	int             i, j;
	int		sts;
	int		db_type;
	int		db_type_array[3];
	char		*new_db_path;
	CE_NAMESPACE	curr_ns;
	static		char null_name[] = "";
	
	if (write_in_progress == TRUE)
		return (CE_ERR_WRITE_IN_PROGRESS);
	if (database == NULL)
		return (CE_ERR_DB_NOT_LOADED);
	
	for (i = 0; i < master_db.num_db; i++)	{
		if (strcmp(master_db.db_names[i], database) == 0)
			break;
		if (strcmp (master_db.db_names[i], user_db_name) == 0) {
			db_type_array[i] = USER_DB;
		} else if (strcmp (master_db.db_names[i], system_db_name) == 0) {
			db_type_array[i] = SYSTEM_DB;
		} else if (strcmp (master_db.db_names[i], network_db_name) == 0) {
			db_type_array[i] = NETWORK_DB;
		}
	}	
	if (i == master_db.num_db)	{
		if (strcmp (database, user_db_name) == 0) {
			db_type = USER_DB;
		} else if (strcmp (database, system_db_name) == 0) {
			db_type = SYSTEM_DB;
		} else if (strcmp (database, network_db_name) == 0) {

			db_type = NETWORK_DB;
		} else {
			return (CE_ERR_UNKNOWN_DATABASE_NAME);
		}
		new_database = TRUE;
		first_namespace = TRUE;
		write_in_progress = TRUE;
		write_db = (CE_DB *)malloc (sizeof (CE_DB));
		(void)memset ((char *)write_db, 0, sizeof (CE_DB));
		write_db->namespaces = NULL;
		/* ok, db_paths holds all ce database pathnames from CEPATH
		 * even if there were not any ce databases in the named
		 * paths. Now that we have to create a new database
		 * we will have to create it under a CEPATH named pathname
		 * or using the default. So first we look under the 
		 * CEPATH named pathname for this database type to see
		 * if anything is there 
		 */
		new_db_path = db_paths[db_type];
		write_db->db_path = null_name;
		write_db->db_name = database;

		if (new_db_path != NULL)	{
			if  ((sts = perform_db_op (NULL, NULL, SET_DB_FILENAME,
						      (int *)new_db_path)) != 0)
			return (CE_ERR_ERROR_READING_DB);
			if ((sts = perform_db_op (NULL, NULL, OPEN_DB_FOR_WRITE,
						  &write_db->dd)) != 0)
				return (sts);
		}
		else	{
			
			if ((sts = perform_db_op (NULL, db_type, OPEN_DB_FOR_WRITE,	
						  &write_db->dd)) != 0)	{
				return (sts);
			}
		}
		
		if ((sts = perform_db_op (NULL, db_type, TEST_OK_TO_WRITE, 
					  &write_db->dd)) != 0)	{
			perform_db_op (NULL, NULL, CLOSE_DB, &write_db->dd);
			return (sts);
		}
		/* add new db path to the write_db db_path */
		write_db->db_path = (char *) strdup (new_db_path);
		
		/* plug in the new database into its right place in the
		 * hierarchy of databases
		 */
		for (i = 0; i < master_db.num_db; i++)	{
			if (db_type < db_type_array [i])
				break;
		}
		if (i == 2)
			return (CE_ERR_INTERNAL_ERROR);
		/* move master_db...[i..1] to master_db...[i+1..2] */
		for (j = 2; j > i; j--)	{
			master_db.db_mtimes[j] = master_db.db_mtimes[j-1];
			master_db.db_names[j] = master_db.db_names[j-1];
			master_db.db_paths[j] = master_db.db_paths[j-1];
			master_db.ce_db_ptrs[j] = master_db.ce_db_ptrs[j-1];
		}
		master_db.db_mtimes[i] = NULL;
		master_db.db_names[i] = database;
		master_db.db_paths[i] = NULL;
		master_db.ce_db_ptrs[i] = write_db;

		
		master_db.num_db++;

		for (i = 0; i < master_db.num_db; i++)	{
			for (curr_ns = master_db.ce_db_ptrs[i]->namespaces;
			     curr_ns != NULL;
			     curr_ns = curr_ns->next_ns)
				curr_ns->db_id = i;
		}
		
		return (0);
	}
	else {
		write_in_progress = TRUE;
		write_db = master_db.ce_db_ptrs[i];
		if ((sts = perform_db_op (NULL, NULL, SET_DB_FILENAME, 
					  (int *)write_db->db_path)) != 0)
			return (sts);
		if ((sts = perform_db_op (NULL, NULL, OPEN_DB_FOR_WRITE,	
					  &write_db->dd)) != 0)	{
			switch (write_db->dd)	{
			case EACCES:
				return (CE_ERR_NO_PERMISSION_TO_WRITE);
			default:
				return (sts);
			}
		}

		if ((sts = perform_db_op (NULL, NULL, TEST_OK_TO_WRITE, 
					  &write_db->dd)) != 0)	{
			perform_db_op (NULL, NULL, CLOSE_DB, &write_db->dd);
			return (sts);
		}


		return (0);

	}
		
}
	

/*
 * Returns 0 if it is ok to write a database
 *	CE_ERR_NO_PERMISSION_TO_WRITE
 *		if the user does not have permissions to write the database
 *	CE_ERR_DB_NOT_LOADED
 *		if the named database is not loaded
 *	CE_ERR_WRITE_IN_PROGRESS
 *		if some other database write has not been committed
 *	CE_ERR_DB_LOCKED
 *		if the specified database has been locked for writing by another 
 *		program
 */
int
ce_test_ok_to_write (database, test_flag)
char	*database;
int	test_flag;

{
	int             i;
	int		sts;
	int		db_type;
	int		dd;
	char		*new_db_path;
	CE_DB		*test_db;

	if (database == NULL)
		return (CE_ERR_DB_NOT_LOADED);
	
	for (i = 0; i < master_db.num_db; i++)
		if (strcmp(master_db.db_names[i], database) == 0)
			break;
	if (i == master_db.num_db)	{
		if (strcmp (database, user_db_name) == 0) {
			db_type = USER_DB;
		} else if (strcmp (database, system_db_name) == 0) {
			db_type = SYSTEM_DB;
		} else if (strcmp (database, network_db_name) == 0) {
			db_type = NETWORK_DB;
		} else {
			return (CE_ERR_UNKNOWN_DATABASE_NAME);
		}

		/* ok, db_paths holds all ce database pathnames from CEPATH
		 * even if there were not any ce databases in the named
		 * paths. Now that we have to create a new database
		 * we will have to create it under a CEPATH named pathname
		 * or using the default. So first we look under the 
		 * CEPATH named pathname for this database type to see
		 * if anything is there 
		 */
		new_db_path = db_paths[db_type];
		if (new_db_path != NULL)	{
			if  ((sts = XXX_perform_db_op (NULL, NULL, SET_DB_FILENAME,
						      (int *)new_db_path)) != 0)
			return (CE_ERR_ERROR_READING_DB);
			if ((sts = XXX_perform_db_op (NULL, NULL, OPEN_DB_FOR_WRITE,
						      &dd)) != 0)	{
				switch (dd)	{
				case EACCES:
					return (CE_ERR_NO_PERMISSION_TO_WRITE);
				default:
					return (sts);
				}
			}

			if (test_flag == CE_FLAG_TEST_PERMISSIONS)	{
				XXX_perform_db_op (NULL, NULL, CLOSE_DB, &dd);
				return (0);
			}
			
			
		}
		else	{
			
			if ((sts = XXX_perform_db_op (NULL, db_type, OPEN_DB_FOR_WRITE,	
						  &dd)) != 0)	{
				switch (dd)	{
				case EACCES:
					return (CE_ERR_NO_PERMISSION_TO_WRITE);
				default:
					return (sts);
				}				
			}
			if (test_flag == CE_FLAG_TEST_PERMISSIONS)	{
				XXX_perform_db_op (NULL, NULL, CLOSE_DB, &dd);
				return (0);
				
			}
		}
 		
		if ((sts = XXX_perform_db_op (NULL, db_type, TEST_OK_TO_WRITE, 
					  &dd)) != 0)	{
			XXX_perform_db_op (NULL, NULL, CLOSE_DB, &dd);
			return (sts);
		}
		XXX_perform_db_op (NULL, NULL, UNLOCK_DB, &dd);
		XXX_perform_db_op (NULL, NULL, CLOSE_DB, &dd);
		return (0);
	}
	
	else {
		test_db = (CE_DB *)malloc (sizeof (CE_DB));
		
		(void) memcpy (test_db, master_db.ce_db_ptrs[i], sizeof (CE_DB));
		
		if ((sts = XXX_perform_db_op (NULL, NULL, SET_DB_FILENAME, 
					  (int *)test_db->db_path)) != 0)
			return (sts);
		if ((sts = XXX_perform_db_op (NULL, NULL, OPEN_DB_FOR_WRITE,	
					  &test_db->dd)) != 0)
		{
			switch (test_db->dd)	{
			case EACCES:
				return (CE_ERR_NO_PERMISSION_TO_WRITE);
			default:
				return (sts);
			}
		}
		if (test_flag == CE_FLAG_TEST_PERMISSIONS)	{
			XXX_perform_db_op (NULL, NULL, CLOSE_DB, &test_db->dd);
			return (0);
		}
		
		if ((sts = XXX_perform_db_op (NULL, NULL, TEST_OK_TO_WRITE, 
					  &test_db->dd)) != 0)	{
			XXX_perform_db_op (NULL, NULL, CLOSE_DB, &test_db->dd);
			return (sts);
		}
		XXX_perform_db_op (NULL, NULL, UNLOCK_DB, &test_db->dd);
		
		XXX_perform_db_op (NULL, NULL, CLOSE_DB, &test_db->dd);
		
		return (0);

	}
}

/*
 * Clone a namespace into a lower level database - use this call in the
 * case where you have a valid namespace handle in hand and you
 * wish to create the "same" namespace in a different database
 */
int
ce_clone_namespace (namespace)
CE_NAMESPACE *namespace;
{
	CE_NAMESPACE    curr_ns;
	CE_NAMESPACE 	clone_ns;
	if (write_in_progress == FALSE)
		return (CE_ERR_WRITE_NOT_STARTED);

	if (write_db == NULL)
		return (CE_ERR_DB_NOT_LOADED);

	/* is this a new database we are writing to? */
	if (first_namespace == TRUE)	{
		if (add_first_ns ((*namespace)->ns_name, &clone_ns) != 0)
			return (CE_ERR_INTERNAL_ERROR);
		first_namespace = FALSE;
		write_db->num_ns++;
		if (!(clone_ns->ns_mgr_ptr = (NS_MGR_INFO *) malloc (sizeof 
							       (struct 
								ns_mgr_struct))))
		{
			return (CE_ERR_NO_MEMORY);
		}
		
		memcpy (clone_ns->ns_mgr_ptr, (*namespace)->ns_mgr_ptr,
			sizeof (NS_MGR_INFO));
		
		write_db->namespaces = clone_ns;
		*namespace = clone_ns;
		
		return (0);
	}
	for (curr_ns = write_db->namespaces; curr_ns != NULL;
	     curr_ns = curr_ns->next_ns) {
		if (strcmp(curr_ns->ns_name, (*namespace)->ns_name) == 0 &&
		    !is_deleted(curr_ns->flags))
			return (CE_ERR_NAMESPACE_EXISTS);
		if (is_last(curr_ns->flags)) 
			break;
	}
	if (curr_ns != NULL) {

		if (add_ns(curr_ns, (*namespace)->ns_name, &clone_ns) != 0)
			return (CE_ERR_INTERNAL_ERROR);
		if (!(clone_ns->ns_mgr_ptr = (NS_MGR_INFO *) malloc (sizeof 
							       (struct ns_mgr_struct))))
		{
			return (CE_ERR_NO_MEMORY);
		}
		
		memcpy (clone_ns->ns_mgr_ptr, (*namespace)->ns_mgr_ptr,
			sizeof (NS_MGR_INFO));
		*namespace = clone_ns;
		
		write_db->num_ns++;
	} 
	else
		return (CE_ERR_INTERNAL_ERROR);

	

	
	return (0);
	
}
/*
 * Adding a namespace This call will return 0 if successful - namespace_ptr
 * will point at a handle to the new namespace. If the call is not
 * succcessful, the following errors can be returned:
 * 
 * CE_ERR_NAMESPACE_EXISTS if the named namespace already exists in this
 * database CE_ERR_WRITE_NOT_STARTED if a successful ce_start_write () call
 * has not been made prior to this call
 */
int
ce_add_namespace(namespace_name, namespace_ptr)
char           *namespace_name;
CE_NAMESPACE   *namespace_ptr;

{
	CE_NAMESPACE    curr_ns;

	if (write_in_progress == FALSE)
		return (CE_ERR_WRITE_NOT_STARTED);

	if (write_db == NULL)
		return (CE_ERR_DB_NOT_LOADED);

	/* is this a new database we are writing to? */
	if (first_namespace == TRUE)	{
		if (add_first_ns (namespace_name, namespace_ptr) != 0)
			return (CE_ERR_INTERNAL_ERROR);
		first_namespace = FALSE;
		write_db->num_ns++;
		write_db->namespaces = *namespace_ptr;
		
		return (0);
	}
	
	for (curr_ns = write_db->namespaces; curr_ns != NULL;
	     curr_ns = curr_ns->next_ns)	{
		if (strcmp (curr_ns->ns_name, namespace_name) == 0 &&
		    !is_deleted(curr_ns->flags))
			return (CE_ERR_NAMESPACE_EXISTS);
		if (is_last(curr_ns->flags)) 
			break;
	}
	if (curr_ns != NULL) {

		if (add_ns(curr_ns, namespace_name, namespace_ptr) != 0)
			return (CE_ERR_INTERNAL_ERROR);
	} 
	else
		return (CE_ERR_INTERNAL_ERROR);
	write_db->num_ns++;
	
	return (0);
	
}

/*
 * Removing a namespace This call will return 0 if it is successful. If the
 * specified namespace was not in this database, a 0 error indication will be
 * returned. If the call is not successful, the following errors can be
 * returned:
 * 
 * CE_ERR_NAMESPACE_NOT_EMPTY if the specified namespace still has entries in it
 * in this database. All of the entries in a namespace in a database must be
 * explicitly deleted before the namespace is deleted.
 * CE_ERR_WRITE_NOT_STARTED if a successful ce_start_write () call has not
 * been made prior to this call
 */
int 
ce_remove_namespace(namespace)
	CE_NAMESPACE    namespace;

{
	BOOLEAN         ns_empty;
	CE_NAMESPACE    curr_ns;
	CE_ENTRY        curr_entry;

	if (write_in_progress == FALSE)
		return (CE_ERR_WRITE_NOT_STARTED);

	if (is_deleted (namespace->flags))
		return (0);
	
	/*
	 * check if there is any info about this namespace in this database
	 */
	for (curr_ns = write_db->namespaces; curr_ns != NULL;
	     curr_ns = curr_ns->next_ns)
		if (strcmp(curr_ns->ns_name, namespace->ns_name) == 0)
			break;
	/*
	 * if we didn't find it, then we can trivially say we deleted it!
	 */
	if (curr_ns == NULL)
		return (0);

	/* check if namespace has any entries in it */
	ns_empty = TRUE;
	for (curr_entry = curr_ns->entries; curr_entry != NULL;
	     curr_entry = curr_entry->next_entry) {
		if (!is_deleted(curr_entry->flags))
			ns_empty = FALSE;
		if (is_last(curr_entry->flags))
			break;
	}
	if (ns_empty = FALSE)
		return (CE_ERR_NAMESPACE_NOT_EMPTY);

	/* this namespace in this database is empty - "remove it */
	set_deleted(curr_ns->flags);
	return (0);

}

/*
 * Allocating an entry This call will return 0 if it is successful -
 * entry_ptr will point to the entry handle for the new entry. Otherwise, the
 * following errors will be returned:
 * 
 * CE_ERR_NAMESPACE_DOES_NOT_EXIST if the specified namespace does not exist in
 * this database. CE_ERR_WRITE_NOT_STARTED if a successful ce_start_write ()
 * call has not been made prior to this call
 */

int
ce_alloc_entry(namespace, entry_ptr)
CE_NAMESPACE    namespace;
CE_ENTRY       *entry_ptr;

{
	CE_NAMESPACE    curr_ns;
	CE_ENTRY        curr_entry;

	if (write_in_progress == FALSE)
		return (CE_ERR_WRITE_NOT_STARTED);
	    
	/* does this namespace exist in this database */
	for (curr_ns = write_db->namespaces; curr_ns != NULL;
	     curr_ns = curr_ns->next_ns)
		if (strcmp(curr_ns->ns_name, namespace->ns_name) == 0)
			break;
	if (curr_ns == NULL)
		return (CE_ERR_NAMESPACE_DOES_NOT_EXIST);

	if (is_deleted (curr_ns->flags))
		return (CE_ERR_NAMESPACE_DOES_NOT_EXIST);
	
	curr_entry = NULL;
	/* we have the namespace, add the entry */
	for (curr_entry = curr_ns->entries; curr_entry != NULL;
	     curr_entry = curr_entry->next_entry) {
		if (is_last(curr_entry->flags))
			break;
	}
	if (alloc_entry(curr_ns, curr_entry, entry_ptr) != 0)
		return (CE_ERR_INTERNAL_ERROR);
	else
		return (0);



}


/*
 * Allocating a namespace entry This call will return 0 if it is successful -
 * entry_ptr will point to the entry handle for the new entry. Otherwise, the
 * following errors will be returned:
 * 
 * CE_ERR_NAMESPACE_DOES_NOT_EXIST if the specified namespace does not exist in
 * this database. CE_ERR_WRITE_NOT_STARTED if a successful ce_start_write ()
 * call has not been made prior to this call CE_ERR_NS_ENTRY_EXISTS if this
 * namespace, in this database, already has a namespace entry
 */
int
ce_alloc_ns_entry(namespace, entry_ptr)
CE_NAMESPACE    namespace;
CE_ENTRY       *entry_ptr;	

{
	CE_NAMESPACE 	curr_ns;
	CE_ENTRY	curr_entry;
	
	if (write_in_progress == FALSE)
		return (CE_ERR_WRITE_NOT_STARTED);

	/* does this namespace exist in this database */
	for (curr_ns = write_db->namespaces; curr_ns != NULL;
	     curr_ns = curr_ns->next_ns)
		if (strcmp(curr_ns->ns_name, namespace->ns_name) == 0)
			break;
	if (curr_ns == NULL)
		return (CE_ERR_NAMESPACE_DOES_NOT_EXIST);

	if (is_deleted (curr_ns->flags))
		return (CE_ERR_NAMESPACE_DOES_NOT_EXIST);
	
	curr_entry = NULL;
	/* we have the namespace, add the entry */
	curr_entry = curr_ns->entries;
	if (curr_entry) {
		if (is_ns_entry(curr_entry->flags))
			return (CE_ERR_NS_ENTRY_EXISTS);
		if (alloc_ns_entry(curr_ns, curr_entry, entry_ptr) != 0)
			return (CE_ERR_INTERNAL_ERROR);
		else
			return (0);
	}
	else	{
		if (alloc_ns_entry (curr_ns, NULL, entry_ptr) != 0)
			return (CE_ERR_INTERNAL_ERROR);
		else
			return (0);
	}
	
}

/*
 * Adding an entry After a CE client calls ce_alloc_entry(), it is expected
 * to make one or more calls to ce_add_attribute to add attributes to the
 * entry. Finally, the client calls ce_add_entry () to add the entry to the
 * given namespace. This call will return 0 if it is successful - otherwise,
 * the following errors will be returned:
 * 
 * CE_ERR_WRITE_NOT_STARTED if the database write was not initiated
 * 
 * CE_ERR_ENTRY_NOT_ALLOCED if the entry pointer is NULL
 */
int
ce_add_entry(namespace, entry)
	CE_NAMESPACE    namespace;
	CE_ENTRY       entry;

{
	if (write_in_progress == FALSE)
		return (CE_ERR_WRITE_NOT_STARTED);
	if (entry == NULL)
		return (CE_ERR_ENTRY_NOT_ALLOCED);

	if (is_deleted (namespace->flags))
		return (CE_ERR_NAMESPACE_DOES_NOT_EXIST);
	
	/* say that this entry was added */
	set_added(entry->flags);
	return (0);

}

/*
 * Removing an entry This call will return 0 if it is successful even if this
 * entry is not stored in this database. Returns the following errors:
 * CE_ERR_NAMESPACE_DOES_NOT_EXIST if the specified namespace does not exist
 * in this database. CE_ERR_WRITE_NOT_STARTED if a successful ce_start_write
 * () call has not been made prior to this call
 */
int
ce_remove_entry(namespace, entry)
CE_NAMESPACE    namespace;
CE_ENTRY        entry;

{
	CE_NAMESPACE curr_ns;
	CE_ENTRY curr_entry;
	
	if (write_in_progress == FALSE)
		return (CE_ERR_WRITE_NOT_STARTED);

	/* check if this namespace exists in this database */
	for (curr_ns = write_db->namespaces; curr_ns != NULL;
	     curr_ns = curr_ns->next_ns)
		if (strcmp(curr_ns->ns_name, namespace->ns_name) == 0)
			break;
	if (curr_ns == NULL)
		return (CE_ERR_NAMESPACE_DOES_NOT_EXIST);

	if (is_deleted (curr_ns->flags))
		return (CE_ERR_NAMESPACE_DOES_NOT_EXIST);
	
	/*
	 * check if this entry is in the list of entries in this namespace in
	 * this database
	 */
	for (curr_entry = curr_ns->entries; curr_entry != NULL;
	     curr_entry = curr_entry->next_entry) {
		if (curr_entry == entry) {
			set_deleted(entry->flags);
			break;
		}
		if (is_last(curr_entry->flags))
			break;
	}
	return (0);

}

/*
 * Adding an attribute This call will return 0 if successful. If the entry is
 * stored in a different level database than the one being written to, the
 * new attribute is added to the database and chained to the entry at the
 * database where it is stored. Otherwise, the one of the following errors
 * will be returned:
 * 
 * CE_ERR_NAMESPACE_DOES_NOT_EXIST if the specified namespace does not exist in
 * this database. CE_ERR_WRITE_NOT_STARTED if a successful ce_start_write ()
 * call has not been made prior to this call CE_ERR_ATTRIBUTE_EXISTS if the
 * named attribute already exists in the same entry in the same namespace in
 * the same database
 */


int
ce_add_attribute(namespace, entry_ptr, attribute_name, attribute_type,
		 attribute_value, attribute_size)
	CE_NAMESPACE    namespace;
	CE_ENTRY        *entry_ptr;
	char           *attribute_name;
	char           *attribute_type;
	char           *attribute_value;
	int             attribute_size;

{
	CE_NAMESPACE 	curr_ns;
	CE_ENTRY	curr_entry, new_entry;
	CE_AV		*curr_av;
	char		*cookie;
	int		cookie_size;
	
	if (write_in_progress == FALSE)
		return (CE_ERR_WRITE_NOT_STARTED);

	/* check if this namespace exists in this database */
	for (curr_ns = write_db->namespaces; curr_ns != NULL;
	     curr_ns = curr_ns->next_ns)
		if (strcmp(curr_ns->ns_name, namespace->ns_name) == 0)
			break;

	if (curr_ns == NULL)
		return (CE_ERR_NAMESPACE_DOES_NOT_EXIST);

	if (is_deleted (curr_ns->flags))
		return (CE_ERR_NAMESPACE_DOES_NOT_EXIST);
	
	/*
	 * check if this entry is from this namespace in this database
	 */
	for (curr_entry = curr_ns->entries; curr_entry != NULL;
	     curr_entry = curr_entry->next_entry) {
		if (curr_entry == *entry_ptr) {
			/*
			 * add the attribute if it isn't already defined
			 */
			for (curr_av = curr_entry->avs; curr_av != NULL;
			     curr_av = curr_av->next_av) {
				if (!is_deleted(curr_av->flags))
					if (attr_cmp(curr_av, attribute_name) == 0)
						return (CE_ERR_ATTRIBUTE_EXISTS);
				if (is_last(curr_av->flags))
					break;
			}
			if (add_av(curr_ns, curr_entry, curr_av,
				   attribute_name, attribute_type,
				   attribute_value, attribute_size) != 0)
				return (CE_ERR_INTERNAL_ERROR);
			else
				return (0);
		}
		
		/*
		 * if this is the last entry in this namespace and we haven't
		 * found the input entry yet, we're gonna have to chain to it
		 */
		if (is_last(curr_entry->flags)) {
			break;
		}
	}
	

	if (ce_alloc_entry(curr_ns, &new_entry) == 0) {
		/*
		 * call the entry cookie function for this namespace
		 */
		get_cookie(curr_ns, *entry_ptr, &cookie);
		if (cookie == NULL)
			return (CE_ERR_INTERNAL_ERROR);
		
		cookie_size = strlen(cookie);
		/* add the cookie as an av */
		if (add_av(curr_ns, new_entry, NULL,
			   chained_to_attr, chained_to_type,
			   cookie, cookie_size) != 0)
			return (CE_ERR_INTERNAL_ERROR);

		/* add the new av to the new entry */
		curr_av = new_entry->avs;
		
		if (add_av(curr_ns, new_entry, curr_av,
			   attribute_name, attribute_type,
			   attribute_value, attribute_size) != 0)
			return (CE_ERR_INTERNAL_ERROR);
		
		/* now add the whole entry */
		if (ce_add_entry(curr_ns, new_entry) != 0)
			return (CE_ERR_INTERNAL_ERROR);
		*entry_ptr = new_entry;
		
		return (0);
	}
	else
		return (CE_ERR_NO_MEMORY);
	

}


/*
 * Modifying an attribute This call will return 0 if successful. If the entry
 * is stored in a different level database than the one being written to, the
 * new attribute value is added to the database and chained to the entry at
 * the database where it is stored. Otherwise, the one of the following
 * errors will be returned:
 * 
 * CE_ERR_NAMESPACE_DOES_NOT_EXIST if the specified namespace does not exist in
 * this database. CE_ERR_WRITE_NOT_STARTED if a successful ce_start_write ()
 * call has not been made prior to this call
 */
int
ce_modify_attribute(namespace, entry_ptr, attribute_name, new_attribute_type,
		   new_attribute_value, new_attribute_size)
	CE_NAMESPACE    namespace;
	CE_ENTRY        *entry_ptr;
	char           *attribute_name;
	char           *new_attribute_type;
	char           *new_attribute_value;
	int             new_attribute_size;

{
	CE_NAMESPACE 	curr_ns;
	CE_ENTRY	curr_entry, new_entry;
	char		*cookie;
	CE_AV		*curr_av;
	int		cookie_size;
	
	if (write_in_progress == FALSE)
		return (CE_ERR_WRITE_NOT_STARTED);

	/* check if this namespace exists in this database */
	for (curr_ns = write_db->namespaces; curr_ns != NULL;
	     curr_ns = curr_ns->next_ns)
		if (strcmp(curr_ns->ns_name, namespace->ns_name) == 0)
			break;

	if (curr_ns == NULL)
		return (CE_ERR_NAMESPACE_DOES_NOT_EXIST);

	/*
	 * check if this entry is from this namespace in this database
	 */
	for (curr_entry = curr_ns->entries; curr_entry != NULL;
	     curr_entry = curr_entry->next_entry) {
		if (curr_entry == *entry_ptr) {
			/*
			 * add the attribute if it isn't already defined
			 */
			for (curr_av = curr_entry->avs; curr_av != NULL;
			     curr_av = curr_av->next_av) {

				if (attr_cmp(curr_av, attribute_name) == 0) {
					/*
					 * modify the attribute in place
					 */
					curr_av->old_info = (OLD_INFO *)
						malloc(sizeof(OLD_INFO));
					curr_av->old_info->old_val =
						(char *)strdup (curr_av->a_val);
					curr_av->old_info->old_type =
						(char *)strdup (curr_av->a_type);
					curr_av->a_size = 
						new_attribute_size;

					curr_av->a_val = 
						(char *) strdup (new_attribute_value);
					curr_av->a_type = 
						(char *) strdup (new_attribute_type);


					set_modified(curr_av->flags);
					return (0);
					
				}
				if (is_last(curr_av->flags))
					break;
			}
			if (add_av(curr_ns, curr_entry, curr_av,
				   attribute_name, new_attribute_type,
				   new_attribute_value, new_attribute_size) != 0)
				return (CE_ERR_INTERNAL_ERROR);
			else
				return (0);
		}
		/*
		 * if this is the last entry in this namespace and we haven't
		 * found the input entry yet, we're gonna have to chain to it
		 */
		if (is_last(curr_entry->flags)) {
			break;
		}
	}
	
	if (ce_alloc_entry(curr_ns, &new_entry) == 0) {
		/*
		 * call the entry cookie function for this namespace
		 */
		get_cookie(curr_ns, *entry_ptr, &cookie);
		if (cookie == NULL)
			return (CE_ERR_INTERNAL_ERROR);
		
		cookie_size = strlen(cookie);
		/* add the cookie as an av */
		if (add_av(curr_ns, new_entry, NULL,
			   chained_to_attr, chained_to_type,
			   cookie, cookie_size) != 0)
			return (CE_ERR_INTERNAL_ERROR);

		/* add the new av to the new entry */
		curr_av = new_entry->avs;
		
		if (add_av(curr_ns, new_entry, curr_av,
			   attribute_name, new_attribute_type,
			   new_attribute_value, new_attribute_size) != 0)
			return (CE_ERR_INTERNAL_ERROR);
		
		/* now add the whole entry */
		if (ce_add_entry(curr_ns, new_entry) != 0)
			return (CE_ERR_INTERNAL_ERROR);
		*entry_ptr = new_entry;
		
		return (0);
	}
	else
		return (CE_ERR_NO_MEMORY);
	
}

/*
 * Removing an attribute This call will return 0 if successful - even if the
 * attribute or the entry are not stored in the specified database.
 * Otherwise, one of the following errors will be returned:
 * 
 * CE_ERR_WRITE_NOT_STARTED if a successful ce_start_write () call has not been
 * made prior to this call
 */
int
ce_remove_attribute(namespace, entry, attribute)
	CE_NAMESPACE    namespace;
	CE_ENTRY        entry;
	CE_ATTRIBUTE    attribute;

{
	CE_NAMESPACE curr_ns;
	CE_ENTRY	curr_entry;
	CE_AV		*curr_av;
	
	if (write_in_progress == FALSE)
		return (CE_ERR_WRITE_NOT_STARTED);

	for (curr_ns = write_db->namespaces; curr_ns != NULL;
	     curr_ns = curr_ns->next_ns)
		if (strcmp(curr_ns->ns_name, namespace->ns_name) == 0)
			break;
	if (curr_ns == NULL)
		return (0);
	if (is_deleted (curr_ns->flags))
		return (CE_ERR_NAMESPACE_DOES_NOT_EXIST);
	
	for (curr_entry = curr_ns->entries; curr_entry != NULL;
	     curr_entry = curr_entry->next_entry) {
		if (entry == curr_entry)
			break;
		if (is_last(curr_entry->flags))
			return (0);
	}
	if (curr_entry == NULL)
		return (0);

	/* now check if the named attribute is in this entry */
	for (curr_av = curr_entry->avs; curr_av != NULL;
	     curr_av = curr_av->next_av) {
		if (attr_cmp(curr_av, attribute->a_name) == 0) {
			/* set the removed flag */
			set_deleted(curr_av->flags);
			return (0);
		}
		if (is_last(curr_av->flags))
			return (0);
	}

}

/*
 * Committing a write This call will return 0 if the database has been
 * successfully written. Otherwise, one of the following errors will be
 * returned:
 * 
 * CE_ERR_WRITE_NOT_STARTED if a successful ce_start_write () call has not been
 * made prior to this call CE_ERR_INTERNAL_ERROR if the CE, for internal
 * reasons, was not able to write the database args is reserved for future
 * use. It must be 0 for now.
 */
int
ce_commit_write(args)
	int             args;

{
	CE_NAMESPACE 	curr_ns, prev_ns;
	CE_ENTRY	curr_entry, prev_entry;
	CE_ATTRIBUTE	curr_attr, prev_attr;
	CE_AV		*curr_av, *prev_av;
	int		sts;
	
	
	if (write_in_progress == FALSE)
		return (CE_ERR_WRITE_NOT_STARTED);

	/*
	 * here is the tricky part: loop thru namespaces if a namespace is a
	 * candidate for deletion, xdr_free it and connect its prev and next
	 * together else, if its new, welcome it to the fold by unsetting the
	 * new bit - in any event loop through the attrs in the namespace if
	 * an attr is new, welcome it to the fold, by unsetting the new bit
	 * the entries in the namespace if an entry is to be deleted xdr_free
	 * it, else, if its new, welcome it to the fold by unsetting its new
	 * bit. In any event, loop thru its avs if an av is to be removed,
	 * xdr_free it & connect its prev & next together if an av is new,
	 * welcome it if an av has been modified free up the old info
	 */
	prev_ns = NULL;
	
	for (curr_ns = write_db->namespaces; curr_ns != NULL;
	     curr_ns = curr_ns->next_ns) {
		if (is_deleted(curr_ns->flags)) {
			if (!prev_ns) {
				write_db->namespaces = curr_ns->next_ns;
				prev_ns = write_db->namespaces;
			}
			else	{
				prev_ns->next_ns = curr_ns->next_ns;

			}
			curr_ns->next_ns = NULL;			
			if (is_last(curr_ns->flags) && prev_ns)
				set_last(prev_ns->flags);

			/* XXX xdr_free(xdr_CE_NS, curr_ns); */
			free_name_space (curr_ns);
			
			write_db->num_ns--;
			if (!(curr_ns = prev_ns))
				break;
			
		} else {
			/* is it new? */
			unset_new(curr_ns->flags);
			/* free up namespace manager info, if any */
			if (curr_ns->ns_mgr_ptr)	{
				if (curr_ns->ns_mgr_ptr->shared_lib_name)
					free (curr_ns->ns_mgr_ptr->shared_lib_name);
				free (curr_ns->ns_mgr_ptr);
				curr_ns->ns_mgr_ptr = NULL;
			}
			
			/* ok, now loop through the attrs */
			for (curr_attr = curr_ns->attr_def; curr_attr != NULL;
			     curr_attr = curr_attr->next_attr)
				unset_new(curr_attr->flags);

			/* now through the entries */
			prev_entry = NULL;
			
			for (curr_entry = curr_ns->entries;
			     curr_entry != NULL;
			     curr_entry = curr_entry->next_entry) {
				unset_chained_to (curr_entry->flags);
				
				if (is_deleted(curr_entry->flags)) {
					if (!prev_entry)	{
						curr_ns->entries = 
							curr_entry->next_entry;
						prev_entry = curr_ns->entries;
					}
					else	{
						prev_entry->next_entry =
							curr_entry->next_entry;
					}
					
					curr_entry->next_entry = NULL;
					if (is_last(curr_entry->flags) && prev_entry)
						set_last(prev_entry->flags);
					/* xdr_free(xdr_CE_ENT, curr_entry); */
					free_entry (curr_entry);
					
					if (!(curr_entry = prev_entry))
						break;
					
				} 
				else {
					/* is it new? */
					if (is_new (curr_entry->flags))	{
						if (is_alloced (curr_entry->flags)
						    && !is_added (curr_entry->flags))	{
							
							/* get rid of it */
							if (!prev_entry)	{
								curr_ns->entries = 
									curr_entry->next_entry;
								prev_entry = curr_ns->entries;
							}
							else	{
								prev_entry->next_entry =
									curr_entry->next_entry;
							}
					
							curr_entry->next_entry = NULL;
							if (is_last(curr_entry->flags) && prev_entry)
								set_last(prev_entry->flags);
							/* xdr_free(xdr_CE_ENT, curr_entry); */
							free_entry (curr_entry);
							
							if (!(curr_entry = prev_entry))
								break;
							
						}
				
						else
							unset_new(curr_entry->flags);
					}

					if (curr_entry)	{
					
						/* now loop through the avs */
						prev_av = NULL;
					
						for (curr_av = curr_entry->avs;
						     curr_av != NULL;
						     curr_av = curr_av->next_av) {
							/*
							 * is it to be deleted?
							 */
							if (is_deleted(curr_av->flags)) {
								if (!prev_av)	{
									curr_entry->avs =
										curr_av->next_av;
									prev_av = curr_entry->avs;
								}
								else	{
									prev_av->next_av =
										curr_av->next_av;
								}
								curr_av->next_av = NULL;
								if (is_last(curr_av->flags)
								    && prev_av)
									set_last(prev_av
										 ->flags);
								/* xdr_free(xdr_CE_AV, curr_av); */
								free_av (curr_av);
								
								if (!(curr_av = prev_av))
									break;
								
							} else {
								/* is it new? */
								unset_new(curr_av->flags);
								if (is_modified(curr_av
										->flags)) {
									free(curr_av->old_info);
									curr_av->old_info = NULL;
									unset_modified (curr_av->flags);
									
								}
							}
							prev_av = curr_av;
							
						}
					}
					
				}

				prev_entry = curr_entry;
			}
		}

		prev_ns = curr_ns;
	}

	if ((sts = perform_db_op (write_db, NULL, WRITE_DB, 
				  &write_db->dd)) != 0)
		return (sts);
	if ((sts = perform_db_op (NULL, NULL, UNLOCK_DB,
				  &write_db->dd)) != 0)
		return (sts);
	perform_db_op (NULL, NULL, CLOSE_DB, &write_db->dd);
	write_in_progress = FALSE;
	
	
	return (0);
	
}


/*
 * Aborting a write This call will return 0 in all cases.
 */
int 
ce_abort_write(args)
	int             args;

{
	CE_NAMESPACE	curr_ns, prev_ns;
	CE_ENTRY	curr_entry, prev_entry;
	CE_ATTRIBUTE	curr_attr, prev_attr;
	CE_AV		*curr_av, *prev_av;
	int		sts;

	if (write_in_progress == FALSE)
		return (0);
	
	prev_ns = NULL;

	curr_ns = write_db->namespaces;
	while (curr_ns != NULL)	{
		if (is_new(curr_ns->flags)) {
			if (!prev_ns)	{
				write_db->namespaces = curr_ns->next_ns;
				prev_ns = write_db->namespaces;
			}
			else
				prev_ns->next_ns = curr_ns->next_ns;

			curr_ns->next_ns = NULL;

			if (is_last(curr_ns->flags) && prev_ns)
				set_last(prev_ns->flags);
			/* xdr_free(xdr_CE_NS, curr_ns); */
			free_name_space (curr_ns);
			
			curr_ns = prev_ns;
		} else {
			/* was it to be deleted? */
			unset_deleted(curr_ns->flags);
			/* ok, now loop through the attrs */
			prev_attr = NULL;

			for (curr_attr = curr_ns->attr_def; curr_attr != NULL;
			     curr_attr = curr_attr->next_attr) {

				if (is_new(curr_attr->flags)) {
					if (!prev_attr)	{
						curr_ns->attr_def = curr_attr->
							next_attr;
						prev_attr = curr_ns->attr_def;
					}
					else
						prev_attr->next_attr =
							curr_attr->next_attr;

					curr_attr->next_attr = NULL;

					if (is_last(curr_attr->flags) && prev_attr)
						set_last(prev_attr->flags);
					/* xdr_free(xdr_CE_ATTR, curr_attr); */
					free_attr (curr_attr);
					
					curr_attr = prev_attr;
				}
				prev_attr = curr_attr;
			}

			/* now through the entries */
			prev_entry = NULL;
			for (curr_entry = curr_ns->entries;
			     curr_entry != NULL;
			     curr_entry = curr_entry->next_entry) {
				if (is_new(curr_entry->flags)) {
					if (!prev_entry)	{
						curr_ns->entries =
							curr_entry->next_entry;
						prev_entry = curr_ns->entries;
					}
					else
						prev_entry->next_entry =
							curr_entry->next_entry;

					curr_entry->next_entry = NULL;
					if (is_last(curr_entry->flags) && prev_entry)
						set_last(prev_entry->flags);
					/* xdr_free(xdr_CE_ENT, curr_entry); */
					free_entry (curr_entry);
					
					curr_entry = prev_entry;
				} else {
					/* was it to be deleted? */
					unset_deleted(curr_entry->flags);
					/* now loop through the avs */
					prev_av = NULL;
					
					for (curr_av = curr_entry->avs;
					     curr_av != NULL;
					     curr_av = curr_av->next_av) {
						/*
						 * is it to be deleted?
						 */
						if (is_new(curr_av->flags)) {
							if (!prev_av)	{
								curr_entry->avs =
									curr_av->next_av;
								prev_av = curr_entry->avs;
							}
							else
								prev_av->next_av =
									curr_av->next_av;
							curr_av->next_av = NULL;
							if (is_last(curr_av->flags) 
							    && prev_av)
								set_last(prev_av
								   ->flags);
							/* xdr_free(xdr_CE_AV, curr_av);*/
							free_av (curr_av);
							
							curr_av = prev_av;
						} else {
							/*
							 * was it to be
							 * deleted?
							 */
							unset_deleted(curr_av
								   ->flags);
							if (is_modified(curr_av
								 ->flags)) {
								curr_av->a_val = curr_av->old_info->old_val;
								curr_av->a_type = curr_av->old_info->old_type;
								curr_av->a_size = strlen(curr_av->a_val);
								free(curr_av->old_info);
								curr_av->old_info = NULL;
								unset_modified 
									(curr_av->flags);
								
										
							}
						}
						prev_av = curr_av;

					}
				}

				prev_entry = curr_entry;
			}
		}

		prev_ns = curr_ns;
		if (curr_ns)
			curr_ns = curr_ns->next_ns;
		
	}
	if ((sts = perform_db_op (NULL, NULL, UNLOCK_DB,
				  &write_db->dd)) != 0)
		return (sts);
	perform_db_op (NULL, NULL, CLOSE_DB, &write_db->dd);
	write_in_progress = FALSE;
	
	return (0);
	
}


