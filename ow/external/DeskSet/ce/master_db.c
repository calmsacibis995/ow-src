/*	@(#)master_db.c	1.11	1/18/91
 * Routines that manipulate the CE master database
 */
#include <dlfcn.h>
#include <string.h>
#include <sys/param.h>
#include <pwd.h>
#include "ce_int.h"


static char cepathname[] = "CEPATH";
char	**db_paths;


/* init the master database structure */
void
init_master (master)
MASTER_DB	*master;
{
	int i;
	
	master->num_db = 0;
	master->db_mtimes = (time_t *)malloc (sizeof (time_t) * 3);
	memset(master->db_mtimes,0,sizeof(time_t) * 3);
	master->db_names = (char **)malloc (sizeof (char *) * 3);
	memset(master->db_names,0,sizeof(char *) * 3);
	master->db_paths = (char **)malloc (sizeof (char *) * 3);
	memset(master->db_paths,0,sizeof(char *) * 3);
	db_paths = (char **)malloc(sizeof(char *) * 3);
	memset(db_paths,0,sizeof(char *) * 3);
	master->ce_db_ptrs = (CE_DB **)malloc (sizeof (CE_DB *) * 3);
	memset(master->ce_db_ptrs, 0, sizeof(CE_DB *) * 3);

	for (i = 0; i < 3; i++)	{
		master->db_mtimes [i] = NULL;
		master->db_names [i] = NULL;
		master->db_paths [i] = NULL;
		db_paths [i] = NULL;
		master->ce_db_ptrs [i] = NULL;
	}
	
	return;
}

static char*
get_cepathdefault()
{
	static char *cepathdef;
	struct passwd *pw;

	if (cepathdef != NULL)
		return cepathdef;
		
	pw = (struct passwd*)getpwuid(getuid());
	if (pw != NULL && pw->pw_dir != NULL) {
		cepathdef = (char*)malloc(strlen(pw->pw_dir)+strlen("/.cetables:/etc/cetables:$OPENWINHOME/lib/cetables") + 1);
		sprintf(cepathdef, "%s/.cetables:/etc/cetables:$OPENWINHOME/lib/cetables", pw->pw_dir);
	}
	else {
		cepathdef = (char*)malloc(strlen("$HOME/.cetables:/etc/cetables:$OPENWINHOME/lib/cetables") + 1);
		strcpy(cepathdef, "$HOME/.cetables:/etc/cetables:$OPENWINHOME/lib/cetables");
	}
	return cepathdef;
}


void *
load_lib (lib_name)
char	*lib_name;
{
	char	*pathstr;
	void	*lib_handle;
	char    mgr_name[MAXPATHLEN + 1];
	char	fname[MAXPATHLEN];
	char	*lib_ent_name;
	char	*s;
	char	*colon;
	int	cepathlen = strlen(cepathname);
	

	/* does the library name use the CEPATH variable? */
	s = strchr(lib_name, '$');

	/* check to see if lib_name starts with $CEPATH */
	if (*lib_name == '$' && strncmp(lib_name, cepathname, cepathlen) &&
		! isalnum(lib_name[cepathlen + 1]) &&
		lib_name[cepathlen + 1] != '_')
	{
		if((pathstr = (char *)getenv(cepathname)) == NULL) {
			pathstr = get_cepathdefault();
		}

		/* get the library entry name */
		lib_ent_name = lib_name + cepathlen + 1;
	
		/* loop thru cetable names */
		do {
			colon = strchr(pathstr, ':');
			if (! colon) {
				colon = pathstr + strlen(pathstr);
			}

			strncpy(fname, pathstr, colon - pathstr);
			fname[colon-pathstr] = '\0';
			strcat(fname, lib_ent_name);
			/* expand aliases in the name */
			ce_expand_pathname(fname, mgr_name, sizeof mgr_name);
			
			if (lib_handle = dlopen(mgr_name, 1)) {
				return (lib_handle);
			}
			pathstr = colon + 1;
		} while (*colon == ':');

		return (NULL);
		
	} else {
		/* no CEPATH */
		ce_expand_pathname (lib_name, mgr_name, sizeof mgr_name);
		lib_handle = dlopen (mgr_name, 1);
		return (lib_handle);
	}
	
}


			
	
int
load_databases(master)
MASTER_DB *master;
{
	char	*pathstr;
	char	fname[MAXPATHLEN];
	int	i, j, dd, status;
	char	*colon;
	long	db_mtime;
	char	*this_db_path;
	int	db_type;

	if((pathstr = (char *)getenv(cepathname)) == NULL) {
		pathstr = get_cepathdefault();
	}
	
	/* loop thru cetable names */
	i = 0;
	j = 0;
	do {
		colon = strchr(pathstr, ':');

		if (! colon) {
			colon = pathstr + strlen(pathstr);
		}

		strncpy(fname, pathstr, colon - pathstr);
		fname[colon-pathstr] = '\0';
		strcat(fname, "/cetables");
		/* add the db pathname to our list of db pathnames for
		 * possible later use
		 */
		if (!(this_db_path = (char *)strdup (fname)))
			return (CE_ERR_NO_MEMORY);
		
		if  ((status = perform_db_op (NULL, NULL, SET_DB_FILENAME,
			(int *)fname)) != 0)
		{
			return (CE_ERR_ERROR_READING_DB);
		}
		
		
		status = perform_db_op(NULL, NULL, OPEN_DB_FOR_READ, &dd);

		if (status != 0) {
			db_paths[j++] = this_db_path;
			goto cont;
		}

		db_mtime = (time_t) get_db_mtime (&dd);	

		status = perform_db_op (NULL, NULL, TEST_OK_TO_READ, &dd);
		if (status != 0) {
			return (status);
		}

		/* got this far, malloc mem for the new db */
		master->ce_db_ptrs [i] = (CE_DB *)malloc (sizeof (CE_DB));
		memset (master->ce_db_ptrs [i], 0, sizeof (CE_DB));
		
		status = perform_db_op (master->ce_db_ptrs [i], NULL, READ_DB,
			&dd);

		if (status != 0) {
			if (status == CE_ERR_BAD_DATABASE_FILE)	{
				/* perform_db_op would have removed the bad file */
				db_paths[j++] = this_db_path;
				free (master->ce_db_ptrs[i]);
				goto cont;
			}
			goto error_quit;
		} else {
			perform_db_op (NULL, NULL, CLOSE_DB, &dd);
			master->num_db++;
			master->ce_db_ptrs[i]->db_path = (char *)
				strdup (fname);
			master->db_names[i] = master->ce_db_ptrs [i]->db_name;
			master->db_paths[i] = master->ce_db_ptrs [i]->db_path;
			
			db_type = -1;
			
			if (strcmp (master->db_names[i], user_db_name) == 0) {
				db_type = USER_DB;
			} else if (strcmp (master->db_names[i], system_db_name) == 0) {
				db_type = SYSTEM_DB;
			} else if (strcmp (master->db_names[i], network_db_name) == 0) {
				db_type = NETWORK_DB;
			}
			j++;
			if (db_type != -1)
			{
			/* sats: Added this to free db_paths[2] before overwriting */
				if(db_paths[db_type] != NULL)
				   free(db_paths[db_type]);
				db_paths[db_type] = this_db_path;
			}
			
			master->db_mtimes[i] = db_mtime;
		}
		i++;
cont:;
		
		
		pathstr = colon + 1;
	} while (*colon == ':');
	return (0);
	
error_quit:
	for (i = 0; i < master->num_db; i++) {
		
		if (master->ce_db_ptrs [i])
			free (master->ce_db_ptrs [i]);
		if (master->db_mtimes [i])
			free (master->db_mtimes [i]);
		if (master->db_names [i])
			free (master->db_names [i]);
		if (master->db_paths [i])
			free (master->db_paths [i]);
	}
	return (CE_ERR_ERROR_READING_DB);
	
}

/* loop thru all the namespaces in all the db's connecting all the 
 * namespaces whose information resides in more than one database
 * This info will come in useful in eliminating redundancy
 * when namespaces are being mapped thru
 */
void
make_master_info(master_db)
	MASTER_DB      *master_db;
{
	int             i, j;

	CE_DB		*master_list;
	CE_DB          *curr_db;
	CE_NS          *curr_ns, *new_ns;
	CE_ATTR        *curr_last_attr, *new_last_attr;
	CE_ENTRY        curr_last_entry;



	/* loop thru all db's setting the id of each ns in each db with
	 * the id of the db
	 */
	for (i = 0; i < master_db->num_db; i++)
		for (curr_ns = master_db->ce_db_ptrs [i]->namespaces;
		     curr_ns != NULL;
		     curr_ns = curr_ns->next_ns)
			/* set the ns db_id's */
			curr_ns->db_id = i;
	
	for (i = 0; i < master_db->num_db; i++) {

		master_list = master_db->ce_db_ptrs[i];

		for (j = i + 1; j < master_db->num_db; j++) {
			
			curr_db = master_db->ce_db_ptrs[j];

			for (curr_ns = master_list->namespaces; curr_ns != NULL;
			     curr_ns = curr_ns->next_ns)	
				
				if (!is_connected(curr_ns->flags))
					for (new_ns = curr_db->namespaces; new_ns != NULL;
					     new_ns = new_ns->next_ns)	
						if (!is_connected(new_ns->flags))
							if (strcmp(curr_ns->ns_name, new_ns->ns_name)
							    == 0) {

								/*
								 * set the
								 * connected
								 * bit on the
								 * namespace
								 * info we we
								 * just
								 * connected
								 */
								set_connected(new_ns->flags);


							}
		}

	}
}

char *
get_shared_lib_path (master_db, ns, db_level)
MASTER_DB	*master_db;
CE_NS		*ns;
int		db_level;
{
	CE_ENT	*entry;
	CE_NS	*curr_ns;
	int	i;
	CE_ATTRIBUTE	ns_mgr_attr;
	char		*shared_lib_path;
	
	ns_mgr_attr = NULL;
	shared_lib_path = NULL;
	
	for (i = db_level; i < master_db->num_db; i++)
		for (curr_ns = master_db->ce_db_ptrs [i]->namespaces;
		     curr_ns != NULL;
		     curr_ns = curr_ns->next_ns)	
			if (strcmp (curr_ns->ns_name, ns->ns_name) == 0)
				for (entry = curr_ns->entries;
				     entry != NULL;
				     entry = entry->next_entry)
					if (is_ns_entry(entry->flags))	{
						ns_mgr_attr = ce_get_attribute_id (curr_ns, "NS_MANAGER");
						if (ns_mgr_attr)	{
							shared_lib_path = (char *)
								ce_get_attribute(curr_ns, entry, ns_mgr_attr);
							if (shared_lib_path)
								return (shared_lib_path);
						}
						
						
					}


	return (NULL);
}

int
load_ns_managers (master_db)
MASTER_DB	*master_db;
{
	CE_NS          *curr_ns, *next_ns;
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
	int             num_func;
	void          **func_ptrs;

	for (i = 0; i < master_db->num_db; i++)
		for (curr_ns = master_db->ce_db_ptrs[i]->namespaces; curr_ns != NULL;
		     curr_ns = curr_ns->next_ns) {

			/*
			 * loop through entries if an entry is a namespace
			 * entry, look for the namespace manager name, if
			 * there is one & we haven't already seen one, get
			 * the ns manager name
			 */
			if (!curr_ns->ns_mgr_ptr) {
				/*
				 * malloc mem for the name
				 * space mgr info
				 */
				
				curr_ns->ns_mgr_ptr = (NS_MGR_INFO *)
					malloc(sizeof(NS_MGR_INFO));
				/* sats: Added the memset */
				memset(curr_ns->ns_mgr_ptr,0,
						sizeof(NS_MGR_INFO));
				curr_ns->ns_mgr_ptr->ns_mgr_handle = NULL;
				curr_ns->ns_mgr_ptr->build_func = NULL;
				curr_ns->ns_mgr_ptr->match_func = NULL;

				/*
				 * Find the name space
				 * manager name for this
				 * namespace dlopen () it &
				 * dlsym () its match and
				 * build routines
				 */
				
				shared_lib_path = get_shared_lib_path (master_db, curr_ns,
								       i);
				if (shared_lib_path == NULL)
					return (CE_ERR_ERROR_READING_DB);
				if ((curr_ns->ns_mgr_ptr->ns_mgr_handle
				     = load_lib (shared_lib_path)) == NULL)
					return (CE_ERR_ERROR_READING_DB);
				
				
				if (!(curr_ns->ns_mgr_ptr->init_ns_mgr =
				       (int (*)() )dlsym(curr_ns->ns_mgr_ptr->ns_mgr_handle, "init_mgr")))
					return (CE_ERR_ERROR_READING_DB);

				/*
				 * call the init function to
				 * get the other function
				 * ptrs for this namespace
				 * manager
				 */
				status = (*curr_ns->ns_mgr_ptr->init_ns_mgr) 
					(&num_func, &func_ptrs);
				if (status == 0)
					return (CE_ERR_ERROR_READING_DB);
				
				/*
				 * check if we have the right
				 * number of function ptrs
				 */
				if (num_func < 4)
					return (CE_ERR_ERROR_READING_DB);
				
				/*
				 * check if all func ptrs
				 * have something in them
				 */
				for (j = 0; j < 4; j++)
					if (!(func_ptrs[i]))
						return (CE_ERR_ERROR_READING_DB);

				/* assign the func ptrs */
				curr_ns->ns_mgr_ptr->build_func = (int (*) ()) func_ptrs[0];
				curr_ns->ns_mgr_ptr->match_func = (int (*) ()) func_ptrs[1];
				curr_ns->ns_mgr_ptr->get_entry_cookie =
					(int (*) ()) func_ptrs[2];
				curr_ns->ns_mgr_ptr->match_entry_cookie =
					(int (*) ()) func_ptrs[3];
				/* sats: so that this is not leaked */
				free(func_ptrs);
			}
			

			/*
			 * now loop through the entries, calling the build
			 * function for all those that aren't namespace
			 * entries
			 */
			for (curr_entry = curr_ns->entries; curr_entry != NULL;
			     curr_entry = curr_entry->next_entry) {
				if (!(is_ns_entry(curr_entry->flags))) {
					status = (*curr_ns->ns_mgr_ptr->build_func)
						(curr_ns,
						 curr_entry,
						 &curr_entry->match_vals.match_vals_val,
						 &curr_entry->match_vals.match_vals_len);
					curr_entry->match_size =
						curr_entry->match_vals.match_vals_len;

				}
			}
		}
	return (0);
	
}
