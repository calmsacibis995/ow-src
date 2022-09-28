/* @(#)parse_ascii.c	1.6 - 90/12/20 */

/*
 * parse_ascii.c
 *
 * routines to build CE database
 */
#include <stdio.h>
#include <sys/param.h>
#include "ce_int.h"
#include "y.tab.h"

/* Global CE database */
CE_DB ce_db;

/* file used by input routine in lex */
FILE *infile; 

/* flag that indicates whether or not we've built the db before */
bool_t db_built = FALSE;

/* global ptrs into CE database structure */
CE_NS 	*curr_ns;				/* ptr to current namespace */
CE_ENT 	*curr_entry;				/* ptr to current entry */
CE_ATTR *curr_attr;				/* ptr to current attribute */
CE_AV *curr_av;					/* ptr to current av */




BUILD_STATES curr_state = start_all;

extern int yyparse ();
extern char yytext [];
extern int yyleng;
extern char *progname;

/* reads in the database using yacc & lex */
int
build_db (db_name, db_type, ascii_path)
char	*db_name;
int	db_type;
char	*ascii_path;

{

	int sts;
	int dd = 0;
	
	/* only read & build the in-memory database if we haven't already done it */
	if ((infile = (FILE *) fopen (ascii_path, "r")) == NULL)
		return (-1);
	/* initialize CE DB structure */
	memset ((char *)&ce_db, 0, sizeof ce_db);

	curr_ns = ce_db.namespaces;
	yyparse ();

	ce_db.db_name = db_name;
	ce_db.db_path = (char *)strdup ("");
	
	if ((sts = perform_db_op (NULL, db_type, OPEN_DB_FOR_WRITE, &dd)) != 0)
		return (sts);
	if ((sts = perform_db_op (NULL, db_type, TEST_OK_TO_WRITE, &dd)) != 0) {
		perform_db_op (NULL, NULL, CLOSE_DB, &dd);
		return (sts);
	}
	
	if ((sts = perform_db_op (&ce_db, db_type, WRITE_DB, &dd)) != 0)
		return (sts);
	if ((sts = perform_db_op (&ce_db, db_type, UNLOCK_DB, &dd)) != 0)
		return (sts);
	perform_db_op (NULL, NULL, CLOSE_DB, &dd);
	free_ce_db (&ce_db);
	return (0);
	
}



void
yyerror (s)
char *s;

{
	extern int yylineno;
	printf ("YYERROR:%s, line %d, symbol %s\n", s, yylineno, yytext);
}

caddr_t 
add_name_space(name)
caddr_t name;
{
	extern int fns_build();
	extern int fns_lookup();
	extern int tns_build();
	extern int tns_match();


#ifdef DEBUG
	printf ("add_name_space:%s\n", (char *)name);
#endif

	/* if this is the first namespace, malloc mem for it; otherwise
	 * loop through list of namespaces, get to the end and malloc
	 * mem for end->next
	 */
	if (curr_ns == NULL)	{
		curr_ns = (CE_NS *) malloc (sizeof (CE_NS));
		ce_db.namespaces = curr_ns;
	}
	else	{
		while (curr_ns->next_ns != NULL)	{
			curr_ns = curr_ns->next_ns;
		}
		
		/* malloc mem for next namespace */
		unset_last (curr_ns->flags);
		curr_ns->next_ns = (CE_NS *) malloc (sizeof (CE_NS));
		
		curr_ns = curr_ns->next_ns;
	}
	
	/* init this name space table */
	memset (curr_ns, 0, sizeof (CE_NS));
	curr_ns->flags = 0;
	curr_ns->ns_name = (char *) strdup ((char *)name);
	ce_db.num_ns++;
	
	curr_ns->attr_def = NULL;
	curr_ns->rt_list = NULL;
	curr_attr = curr_ns->attr_def;
	curr_ns->entries = NULL;
	curr_ns->db_id = 0;
	curr_entry = curr_ns->entries;
	curr_ns->num_attrs = 0;
	curr_ns->num_ents = 0;
	curr_ns->next_ns = NULL;
	set_last(curr_ns->flags);
	

}

/* Set the state of the build process
 */
void 
set_state (state)
BUILD_STATES state;
{
	curr_state = state;
}

/* Add an attribute to the current namespace */
caddr_t 
add_attr (attr)
caddr_t attr;
{
	extern CE_ATTRIBUTE find_attr();
	extern int hashpjw();
	CE_ATTRIBUTE result;
	

	/* first check if this attr exists - if it does, then return a ptr
	 *  to its id
	 */

	if (result = find_attr (curr_ns, attr))
		return (caddr_t)&result->a_id;
	else	{
		/* if this is the first attr, malloc mem for it; otherwise
		 * loop through list of attrs, get to the end and malloc
		 * mem for end->next
		 */
		if (curr_attr == NULL)	{
			curr_attr = (CE_ATTR *) malloc (sizeof (CE_ATTR));
			curr_ns->attr_def = curr_attr;
		}
		else	{
			while (curr_attr->next_attr != NULL)	{
				curr_attr = curr_attr->next_attr;
			}
			unset_last (curr_attr->flags);
			
			curr_attr->next_attr = (CE_ATTR *) malloc (sizeof (CE_ATTR));
			curr_attr = curr_attr->next_attr;
		}
		

#ifdef DEBUG	  
	  printf ("add_attr:%s\n", (char *)attr);
#endif	
		memset (curr_attr, 0, sizeof (CE_ATTR));
		curr_attr->flags = 0;
		
		curr_attr->a_name = (char *) strdup ((char *)attr);
		curr_ns->num_attrs++;

		/* an attribute id is a hash of its name */
		curr_attr->a_id = curr_ns->num_attrs;
		
		set_last(curr_attr->flags);
		
		curr_attr->next_attr = NULL;

		
		return ((caddr_t)&curr_attr->a_id);
	}

}

/* Add an av to an entry */
caddr_t 
add_av (name, type, value)
caddr_t name;
caddr_t type;
caddr_t value;
{
	int *attr_a_id;
	BOOLEAN namespace_entry = FALSE;
	int i;
	

	if (curr_state == in_ns_entries || 
	    curr_state == start_new_entry ||
	    curr_state == in_ns_attrs) {

#ifdef DEBUG
		printf("New entry\n");
#endif
		if (curr_state == in_ns_attrs)
			namespace_entry = TRUE;
		

		curr_state = in_an_entry;
		curr_ns->num_ents++;

		/* if this is the first entry, malloc mem for it;
		 * otherwise loop through entries until the end &
		 * malloc end->next
		 */
		if (curr_entry == NULL)	{
			curr_entry = (CE_ENT *) malloc (sizeof (CE_ENT));
			curr_ns->entries = curr_entry;
		}
		else	{
			while (curr_entry->next_entry != NULL)
				curr_entry = curr_entry->next_entry;
			unset_last (curr_entry->flags);
			curr_entry->next_entry = (CE_ENT *) malloc (sizeof
								    (CE_ENT));
			curr_entry = curr_entry->next_entry;

		}
		/* initialize the new entry */
		memset (curr_entry, 0, sizeof (CE_ENT));
		curr_entry->flags = 0;
		curr_entry->ent_id = curr_ns->num_ents;
		curr_entry->avs = NULL;
		curr_av = curr_entry->avs;
		curr_entry->match_size = 0;
		curr_entry->match_vals.match_vals_len = 0;
		curr_entry->match_vals.match_vals_val = NULL;
		
		set_last(curr_entry->flags);
		if (namespace_entry == TRUE)
			set_ns_entry (curr_entry->flags);
		
		curr_entry->next_entry = NULL;
		
	}
		
#ifdef DEBUG
	printf ("_add_av: %s %s %s\n", (char *) name, (char *) type, (char *) value);
#endif


	/* add the attr name to the table of attr names */
	attr_a_id = (int *)add_attr (name);

	/* check if the AV list for this entry has been created */
	if (curr_av == NULL)	{
		/* create it */
		curr_av = (CE_AV *) malloc (sizeof (CE_AV));
		curr_entry->avs = curr_av;
	}
	else	{
		/* loop through end of avs & malloc end->next */
		while (curr_av->next_av != NULL)
			curr_av = curr_av->next_av;
		unset_last (curr_av->flags);
		curr_av->next_av = (CE_AV *) malloc (sizeof (CE_AV));
		curr_av = curr_av->next_av;
	}
	memset (curr_av, 0, sizeof (CE_AV));
	curr_av->flags = 0;
	curr_av->a_id = (char *)strdup ((char *) name);
	
	curr_av->a_type = (char *) strdup ((char *) type);

	/* horrible hack: we hide the size of the value in the first
	 * four bytes of the value...
	 */
	curr_av->a_size = * (int *) value;
	for (i = 0; i <= curr_av->a_size; i++ ) {
		value[i] = value[i + sizeof (int)];
	}
	curr_av->a_val = (char *)malloc (curr_av->a_size + 1);
	memcpy (curr_av->a_val, (char *)value, curr_av->a_size);
	curr_av->a_val [curr_av->a_size] = '\0';
	free (value);
	
	set_last (curr_av->flags);
	curr_av->next_av = NULL;
}

/* allocates a ptr that points to the contents of yytext for an id
 */
caddr_t 
alloc_id ()
{
  char *loc_ptr;

  loc_ptr = (char *)strdup ((char *)yytext);
  return (caddr_t) loc_ptr;
}

/* allocates a ptr that points to the contents of yytext for an attribute value
 */
caddr_t
alloc_val ()
{
	char *loc_ptr;
	int len = 0;
	int i;

	len = yyleng;

	/* subtract 2 because we're not going to store the value delimiters */
	len = len - 2;

	/* malloc memory to store value */
	loc_ptr = (char *) malloc (len + 1 + sizeof (int));

	if (! loc_ptr) {
		fprintf(stderr, "%s: out of memory!  Aborting...\n",
			progname);
		exit (1);
	}

	/* horrible hack: we hide the length in the first four bytes.
	 * I *know* this is bad, but the yacc tables are too horible
	 * to fix right now...
	 */
	* (int *) loc_ptr = len;

	/* copy the bytes over */
	memcpy(loc_ptr + sizeof (int), yytext + 1, len);
	loc_ptr [len + sizeof (int)] = '\0';

	return (caddr_t) loc_ptr;

}




